
#include "include/headers/thPool.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <signal.h>
#include <string>
#include <unistd.h>

int sock;

void receiveMessages(void *args) {
  int socket_fd = *((int *)args);
  char buffer[1024];
  while (true) {
    memset(buffer, 0, sizeof(buffer));
    int bytesReceived = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
      std::cerr << "Connection closed or error occurred" << std::endl;
      break;
    }
    buffer[bytesReceived] = '\0';
    std::cout << buffer << std::endl;
    if (strcmp(buffer, "exit") == 0) {
      return;
    }
  }
}

void sendMessages(void *args) {
  int socket_fd = *((int *)args);
  char message[256];
  while (true) {
    if (fgets(message, sizeof(message), stdin) == nullptr) {
      std::cerr << "Error reading input" << std::endl;
      break;
    }
    size_t len = strlen(message);
    if (len > 0 && message[len - 1] == '\n') {
      message[len - 1] = '\0';
    }
    send(socket_fd, message, strlen(message), 0);
  }
}

int main(int argc, char *argv[]) {

  int socket_fd; // Global variable
  struct sockaddr_in server_addr;
  if (argc != 3) {
    printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    return -1;
  }

  int port = atoi(argv[2]);
  // Creating socket
  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "Socket creation error" << std::endl;
    return -1;
  }
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(argv[1]);
  server_addr.sin_port = htons(port);

  // Connecting to server
  if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    std::cerr << "Connection Failed" << std::endl;
    return -1;
  }

  thPool pool(2);
  pool.add_task_in_queue(
      [socket_fd]() { receiveMessages((void *)&socket_fd); });
  pool.add_task_in_queue([socket_fd]() { sendMessages((void *)&socket_fd); });

  pool.wait_for();
  return 0;
}
