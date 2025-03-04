#include "headers/table.h"
#include "headers/json_parser.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <pthread.h>
#include <stdio.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

Table::Table() {
  client_mutex = PTHREAD_MUTEX_INITIALIZER;
  clients.reserve(100);
  active_clients.reserve(100);
  quiz_clients.reserve(100);
}

void Table::set_activ_fd(fd_set active_fd) { this->activ_fd = active_fd; }

fd_set Table::get_activ_fd() { return activ_fd; }

void Table::send_question(Table *table, const char *question) {
  pthread_mutex_lock(&table->client_mutex);
  for (int client : table->quiz_clients) {
    table->total_score.insert({client, 0});
  }
  pthread_mutex_unlock(&table->client_mutex);
  table->multicast_message(question, table->quiz_clients);
}

void Table::receive_ans(Table *table, const char *answer) {
  std::unordered_map<int, bool> ct;
  pthread_mutex_lock(&table->client_mutex);
  for (int client : table->quiz_clients) {
    ct[client] = false;
  }
  pthread_mutex_unlock(&table->client_mutex);
  struct timeval timeout;
  timeout.tv_sec = 10; // 10 seconds to respond
  timeout.tv_usec = 0;

  fd_set read_fds;

  while (true) {
    FD_ZERO(&read_fds);

    pthread_mutex_lock(&table->client_mutex);
    for (auto &it : ct) {
      if (!it.second) {
        FD_SET(it.first, &read_fds);
      }
    }
    pthread_mutex_unlock(&table->client_mutex);

    int nfds = table->clients.size() + 3;
    int ready = select(nfds + 1, &read_fds, nullptr, nullptr, &timeout);

    if (ready < 0) {
      perror("[server] Error in select while waiting for client responses");
      break;
    } else if (ready == 0) {
      pthread_mutex_lock(&table->client_mutex);
      for (auto &it : ct) {
        if (!it.second) {
          send(it.first, "\nIncorrect\n", strlen("\nIncorrect\n"), 0);
        }
      }
      pthread_mutex_unlock(&table->client_mutex);
      break;
    } else {
      pthread_mutex_lock(&table->client_mutex);
      for (auto &it : ct) {
        int client = it.first;
        auto temp = find(table->quiz_clients.begin(), table->quiz_clients.end(),
                         client);
        if (FD_ISSET(client, &read_fds) && temp != table->quiz_clients.end()) {
          char buffer[256];
          memset(buffer, 0, sizeof(buffer));
          int bytes_received = recv(client, buffer, sizeof(buffer), 0);
          if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("[server] Client %d answered: %d\n", client, bytes_received);

            // Evaluate response
            if (strncmp(buffer, "exit", strlen("exit")) == 0) {
              printf("[server] Client %d opted out of the quiz.\n", client);
              send(client, "You have exited the quiz but remain connected.\n",
                   strlen("You have exited the quiz but remain connected.\n"),
                   0);
              table->quiz_clients.erase(find(table->quiz_clients.begin(),
                                             table->quiz_clients.end(),
                                             client));
              ct[client] = 1;
              table->total_score.erase(client);
              continue;

            } else if (strncmp(buffer, answer, strlen(answer)) == 0) {
              table->total_score[client] += 1;
              send(client, "\nCorrect\n", strlen("\nCorrect\n"), 0);
            } else {

              send(client, "\nIncorrect\n", strlen("\nIncorrect\n"), 0);
            }
            ct[client] = true; // Mark client as having responded
          } else if (bytes_received == 0) {
            // Client disconnected
            printf("[server] Client %d disconnected.\n", client);
            close(client);
            FD_CLR(client, &table->activ_fd);
            table->remove_client(client);
            ct.erase(client);
          }
        }
      }
      pthread_mutex_unlock(&table->client_mutex);
    }

    bool all_responded = true;
    pthread_mutex_lock(&table->client_mutex);
    for (auto &it : ct) {
      if (!it.second) {
        all_responded = false;
        break;
      }
    }
    pthread_mutex_unlock(&table->client_mutex);

    if (all_responded) {
      printf("[server] All clients have responded the question. Ending the "
             "quiz.\n");

      break;
    }
  }
}

void Table::display_scores() {
  std::vector<std::pair<int, int>> sorted_scores(total_score.begin(),
                                                 total_score.end());

  std::sort(sorted_scores.begin(), sorted_scores.end(),
            [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
              return a.second > b.second;
            });

  std::string msg = "Score: \n";
  for (auto &it : sorted_scores) {
    msg += nicknames[it.first] + ": " + std::to_string(it.second) + " points\n";
  }
  total_score.clear();
  multicast_message(msg.c_str(), quiz_clients);
  msg.clear();
}

void Table::quiz_preparation(Table *table) {
  table->broadcast_message("Do you want to join the quiz?(y|n)");

  std::unordered_map<int, bool> ct;
  pthread_mutex_lock(&table->client_mutex);
  for (int client : table->clients) {
    ct[client] = false;
  }
  pthread_mutex_unlock(&table->client_mutex);
  struct timeval timeout;
  timeout.tv_sec = 10; // 10 seconds to respond
  timeout.tv_usec = 0;

  fd_set read_fds;

  while (true) {
    FD_ZERO(&read_fds);

    pthread_mutex_lock(&table->client_mutex);
    for (auto &it : ct) {
      if (!it.second) {
        FD_SET(it.first, &read_fds);
      }
    }
    pthread_mutex_unlock(&table->client_mutex);

    int nfds = table->clients.size() + 3;
    int ready = select(nfds + 1, &read_fds, nullptr, nullptr, &timeout);

    if (ready < 0) {
      perror("[server] Error in select while waiting for client responses");
      break;
    } else if (ready == 0) {
      pthread_mutex_lock(&table->client_mutex);
      for (auto &it : ct) {
        if (!it.second) {

          send(it.first, "\nIncorrect\n", strlen("\nIncorrect\n"), 0);
        }
      }
      pthread_mutex_unlock(&table->client_mutex);
      break;
    } else {
      pthread_mutex_lock(&table->client_mutex);
      for (auto &it : ct) {
        int client = it.first;
        if (FD_ISSET(client, &read_fds)) {
          char buffer[256];
          memset(buffer, 0, sizeof(buffer));
          int bytes_received = recv(client, buffer, sizeof(buffer), 0);
          if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("[server] Client %d answered: %d\n", client, bytes_received);

            // Evaluate response
            if (strncmp(buffer, "y", 1) == 0) {
              const char succes_msg[30] = "You will join the quiz!\n";
              send(client, succes_msg, strlen(succes_msg), 0);
              table->quiz_clients.push_back(client);
            } else {
              const char failing_msg[30] = "You will not join the quiz!\n";
              send(client, failing_msg, strlen(failing_msg), 0);
            }
            ct[client] = true; // Mark client as having responded
          } else if (bytes_received == 0) {
            // Client disconnected
            printf("[server] Client %d disconnected.\n", client);
            close(client);
            FD_CLR(client, &table->activ_fd);
            table->remove_client(client);
            ct.erase(client);
          }
        }
      }
      pthread_mutex_unlock(&table->client_mutex);
    }

    bool all_responded = true;
    pthread_mutex_lock(&table->client_mutex);
    for (auto &it : ct) {
      if (!it.second) {
        all_responded = false;
        break;
      }
    }
    pthread_mutex_unlock(&table->client_mutex);

    if (all_responded) {
      printf("[server] All clients have responded the question.\n");

      break;
    }
  }
}

void Table::choose_nickname(Table *table) {
  table->multicast_message("Choose a nickname:", table->quiz_clients);

  std::unordered_map<int, bool> ct;
  pthread_mutex_lock(&table->client_mutex);
  for (int client : table->quiz_clients) {
    ct[client] = false;
  }
  pthread_mutex_unlock(&table->client_mutex);
  struct timeval timeout;
  timeout.tv_sec = 10; // 10 seconds to respond
  timeout.tv_usec = 0;

  fd_set read_fds;

  while (true) {
    FD_ZERO(&read_fds);

    pthread_mutex_lock(&table->client_mutex);
    for (auto &it : ct) {
      if (!it.second) {
        FD_SET(it.first, &read_fds);
      }
    }
    pthread_mutex_unlock(&table->client_mutex);

    int nfds = table->clients.size() + 3;
    int ready = select(nfds + 1, &read_fds, nullptr, nullptr, &timeout);

    if (ready < 0) {
      perror("[server] Error in select while waiting for client responses");
      break;
    } else if (ready == 0) {
      pthread_mutex_lock(&table->client_mutex);
      for (auto &it : ct) {
        if (!it.second) {
          table->nicknames[it.first] = "Guest";
        }
      }
      pthread_mutex_unlock(&table->client_mutex);
      break;
    } else {
      pthread_mutex_lock(&table->client_mutex);
      for (auto &it : ct) {
        int client = it.first;
        if (FD_ISSET(client, &read_fds)) {
          char buffer[256];
          memset(buffer, 0, sizeof(buffer));
          int bytes_received = recv(client, buffer, sizeof(buffer), 0);
          if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("[server] Client %d answered: %d\n", client, bytes_received);

            // Evaluate response
            table->nicknames[it.first] = buffer;
            ct[client] = true; // Mark client as having responded
          } else if (bytes_received == 0) {
            // Client disconnected
            printf("[server] Client %d disconnected.\n", client);
            close(client);
            FD_CLR(client, &table->activ_fd);
            table->remove_client(client);
            ct.erase(client);
          }
        }
      }
      pthread_mutex_unlock(&table->client_mutex);
    }

    bool all_responded = true;
    pthread_mutex_lock(&table->client_mutex);
    for (auto &it : ct) {
      if (!it.second) {
        all_responded = false;
        break;
      }
    }
    pthread_mutex_unlock(&table->client_mutex);

    if (all_responded) {
      printf("[server] All clients have responded the question.\n");

      break;
    }
  }
}

void Table::client_comm(Table *table, int client_fd, fd_set activ_fd) {
  char buffer[256];
  memset(buffer, 0, sizeof(buffer));
  int read_bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
  if (read_bytes > 0) {
    buffer[read_bytes] = '\0';
    printf("[server] Received message from client %d: %s\n", client_fd, buffer);

    if (strncmp(buffer, "broadcast", 9) == 0) {
      char msg[] = "Broadcast message: Hello to all clients!";
      table->broadcast_message(msg);
    } else if (strncmp(buffer, "exit", 4) == 0) {
      printf("[server] Client %d disconnected.\n", client_fd);
      send(client_fd, buffer, strlen(buffer), 0);
      table->remove_client(client_fd);
      table->set_client_inactive(client_fd);
    } else if (strncmp(buffer, "start", 5) == 0) {
      // show time
      table->quiz_preparation(table);
      table->choose_nickname(table);
      table->multicast_message("The quiz will start in 5 seconds!\n",
                               table->quiz_clients);
      sleep(5);
      std::string filepath = "/tmp/quizzes/quiz1.json";
      json_instance q(filepath);
      json_instance::populate(&q);
      int quiz_size = q.quiz_size();
      for (int i = 0; i < quiz_size; ++i) {
        send_question(table, q.get_nth_question(i).c_str());
        receive_ans(table, q.get_nth_answer(i).c_str());
      }
      table->display_scores();
      table->quiz_clients.clear();
    } else {
      send(client_fd, buffer, strlen(buffer), 0);
    }
  } else if (read_bytes == 0) {
    printf("[server] Client %d disconnected unexpectedly.\n", client_fd);

    close(client_fd);
    FD_CLR(client_fd, &activ_fd);
    table->remove_client(client_fd);
    table->set_client_inactive(client_fd);
  }
}

bool Table::set_client_active(int fd) {
  pthread_mutex_lock(&client_mutex);
  auto it = find(active_clients.begin(), active_clients.end(), fd);
  if (it == active_clients.end()) {
    active_clients.push_back(fd);
    pthread_mutex_unlock(&client_mutex);
    return true;
  }
  pthread_mutex_unlock(&client_mutex);
  return false;
}

void Table::set_client_inactive(int fd) {
  pthread_mutex_lock(&client_mutex);
  auto it = find(active_clients.begin(), active_clients.end(), fd);
  if (it != active_clients.end()) {
    active_clients.erase(it);
    printf("Client %d set to inactive.\n", fd);
  } else {
    printf("Did not find the client %d in active list!\n", fd);
  }
  pthread_mutex_unlock(&client_mutex);
}

void Table::add_client(int fd) {
  pthread_mutex_lock(&client_mutex);
  FD_SET(fd, &activ_fd);
  clients.push_back(fd);
  printf("Client %d added to the client list.\n", fd);
  pthread_mutex_unlock(&client_mutex);
}

void Table::remove_client(int fd) {
  pthread_mutex_lock(&client_mutex);

  auto it = find(clients.begin(), clients.end(), fd);
  if (it != clients.end()) {
    close(fd);
    FD_CLR(fd, &activ_fd);
    clients.erase(it);
    printf("Client %d removed from the client list.\n", fd);
  } else {
    printf("Did not find the client %d in client list!\n", fd);
  }

  pthread_mutex_unlock(&client_mutex);
}

void Table::broadcast_message(const char *msg) {
  pthread_mutex_lock(&client_mutex);
  for (auto &client : clients) {
    if (send(client, msg, strlen(msg), 0) == -1) {
      fprintf(stderr, "Error send() to client %d\n", client);
    }
  }
  pthread_mutex_unlock(&client_mutex);
}

void Table::multicast_message(const char *msg, vector<int> cl) {
  pthread_mutex_lock(&client_mutex);
  for (auto &client : cl) {
    if (send(client, msg, strlen(msg), 0) == -1) {
      fprintf(stderr, "Error send() to client %d\n", client);
    }
  }
  pthread_mutex_unlock(&client_mutex);
}

void Table::close_all_clients() {
  pthread_mutex_lock(&client_mutex);
  for (auto &client : clients) {
    close(client);
  }
  printf("All client connections closed.\n");
  pthread_mutex_unlock(&client_mutex);
}
