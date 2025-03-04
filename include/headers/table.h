#ifndef TABLE_H
#define TABLE_H

#include <pthread.h>
#include <string>
#include <sys/select.h>
#include <unordered_map>
#include <vector>

using namespace std;

class Table {
private:
  vector<int> clients;
  vector<int> active_clients;
  vector<int> quiz_clients;
  pthread_mutex_t client_mutex;
  unordered_map<int, int> total_score;
  unordered_map<int, string> nicknames;
  fd_set activ_fd;
  bool flag;

public:
  Table();

  static void send_question(Table *, const char *);
  static void receive_ans(Table *, const char *);
  static void quiz_preparation(Table *);
  static void choose_nickname(Table *);
  static void choose_quizzes(Table *);

  void display_scores();
  static void client_comm(Table *, int client_fd, fd_set activ_fd);
  int get_client_id();
  void close_all_clients();
  void add_client(int fd);
  bool set_client_active(int fd);
  void set_client_inactive(int fd);
  void set_activ_fd(fd_set);
  fd_set get_activ_fd();
  void remove_client(int fd);
  void broadcast_message(const char *);

  void multicast_message(const char *, vector<int>);
};

#endif // !TABLE_H
