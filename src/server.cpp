#include "../include/headers/table.h"
#include "../include/headers/thPool.h"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
using namespace std;

const int port = 2020;
int server_socket;
Table admin_table;
fd_set working_fd, activ_fd;

void sig_handler(int signo) {
  if (signo == SIGINT || signo == SIGTSTP) {
    cout << "Exiting..." << '\n';
    admin_table.close_all_clients();
    close(server_socket);
    exit(0);
  }
}

int main() {
  thPool pool(3);     // Thread pool
  sockaddr_in server; // Server socket
  sockaddr_in from;   // Client socket
  socklen_t c_size;
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("Error creating socket\n");
    exit(1);
  }

  int on = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) <
      0) {
    perror("Error setsockopt\n");
    exit(1);
  }

  // sever to be nonblocking
  if (fcntl(server_socket, F_SETFL, O_NONBLOCK) < 0) {
    perror("Error fcntl\n");
    exit(1);
  }

  cout << "Socket created successfully.\n";

  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(port);

  if (bind(server_socket, (sockaddr *)&server, sizeof(server)) == -1) {
    perror("Error binding socket\n");
    close(server_socket);
    exit(1);
  }

  if (listen(server_socket, 10) == -1) {
    perror("Error listening on socket\n");
    close(server_socket);
    exit(1);
  }

  signal(SIGINT, sig_handler);
  signal(SIGTSTP, sig_handler);

  FD_ZERO(&activ_fd);
  FD_SET(server_socket, &activ_fd);
  int nfds = server_socket;
  admin_table.set_activ_fd(activ_fd);
  cout << "Server is listening on port: " << port << "\n";

  while (true) {
    // working_fd = admin_table.get_activ_fd();
    working_fd = admin_table.get_activ_fd();
    // updating the working_fd from the table
    // multiplexing with select
    int ready_fds = select(nfds + 1, &working_fd, nullptr, nullptr, nullptr);
    if (ready_fds < 0) {
      printf("%d", nfds);
      perror("Error in select");
      break;
    }
    for (int i = 0; i <= nfds; ++i) {
      if (FD_ISSET(i, &working_fd)) {
        if (i == server_socket) {
          printf("Hello from the non-blocking server_socket!\n");
          // accept client
          while (true) {
            c_size = sizeof(from);
            int client_fd = accept(server_socket, (sockaddr *)&from, &c_size);
            if (client_fd < 0) {
              if (errno != EWOULDBLOCK) {
                perror("Error in accept");
              }
              break;
            }

            cout << "New client connected: " << client_fd << "\n";

            // set client nonblock
            if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
              perror("Error setting client socket to non-blocking");
              close(client_fd);
              continue;
            }

            // add the client to active_fds
            admin_table.add_client(client_fd);
            if (client_fd > nfds) {
              nfds = client_fd;
            }
          }
        } else {
          // client socket was connected earlier
          int client_fd = i;
          if (admin_table.set_client_active(client_fd)) {
            pool.add_task_in_queue([client_fd]() {
              Table::client_comm(&admin_table, client_fd, activ_fd);
              admin_table.set_client_inactive(client_fd);
            });
          }
        }
      }
    }
  }
  pool.wait_for();
  close(server_socket);
  return 0;
}
