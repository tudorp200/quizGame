
#ifndef THPOOL_H
#define THPOOL_H

#include <functional>
#include <pthread.h>
#include <queue>
#include <vector>
class thPool {
private:
  std::queue<std::function<void()>> tasks; // queue for every task
  std::vector<pthread_t> threads;          // a vector of threads
  int count;                               // threads intialized at first
  pthread_cond_t cv;                       // variable for the sync of threads
  pthread_cond_t cv_workers; // condition variable for signaling that there are
                             // no threads processing
  pthread_mutex_t mtx;       // for preventing data_racing
  bool ok;                   // when thPool should be closed
  int working;               // how many threads work at a time

  static bool add_thread(thPool *); // add a thread to the thread pool
  static bool rm_thread(thPool *);  // remove a thread from the thread pool
  static void *execute(void *);     // execute a task from the queue
  static bool
  check_for_resize(thPool *); // check to see if there are enough threads for
                              // processing the queue of tasks

public:
  thPool(int ct_threads); // initializing the thread pool
  void wait_for();        // wait for all the threads to execute their task
  ~thPool(); // destroying all the variables(mutex, condition variables, queue,
             // vector of threads)
  void add_task_in_queue(
      std::function<void()>); // add a task in queue to be proccesed
};

#endif // !THPOOL_H
