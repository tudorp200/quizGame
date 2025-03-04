
#include "headers/thPool.h"
#include <cstdio>
#include <iostream>
#include <pthread.h>
#include <utility>

bool thPool::add_thread(thPool *pool) {
  pthread_t th;
  if (pthread_create(&th, nullptr, &execute, pool)) {
    return 0;
  }
  pool->threads.push_back(th);
  return 1;
}

bool thPool::check_for_resize(thPool *pool) {
  if (pool->threads.size() * 4 < pool->tasks.size())
    return 1; // for adding a thread
  else
    return 0;
}

thPool::thPool(int nr_threads) : ok(false), working(0) {
  if (pthread_mutex_init(&mtx, nullptr)) {
    perror("Error initializing the mutex\n");
  }
  if (pthread_cond_init(&cv, nullptr)) {
    perror("Error initializing the cv\n");
  }
  if (pthread_cond_init(&cv_workers, nullptr)) {
    perror("Error initializing the cv\n");
  }

  for (int i = 0; i < nr_threads; ++i) {
    if (!add_thread(this)) {
      perror("Error at creating a thread!\n");
    }
  }
}

void thPool::wait_for() {
  if (pthread_mutex_lock(&mtx)) {
    perror("Error locking the mutex!\n");
  }
  while (!tasks.empty() || (!this->ok && this->working) ||
         (this->ok && threads.size())) {
    pthread_cond_wait(&cv_workers, &mtx);
  }
  pthread_mutex_unlock(&mtx);
}

thPool::~thPool() {

  if (pthread_mutex_lock(&mtx)) {
    std::cerr << "Error locking the mutex!\n";
  }
  // destoying task queue
  while (!tasks.empty()) {
    tasks.pop();
  } // it shouldn t be something in queue in this moment
  ok = 1;

  pthread_cond_broadcast(&cv); // send a signal to all the threads

  if (pthread_mutex_unlock(&mtx)) {
    perror("Error unlocking the mutex\n");
  }
  for (auto &th : threads) {
    if (pthread_join(th, nullptr)) {
      perror("Error at pthread_join()\n");
    }
  }
  if (pthread_mutex_destroy(&mtx)) {
    perror("Error: can t destroy mutex\n");
  }
  if (pthread_cond_destroy(&cv)) {
    perror("Error: can t destroy condition variable\n");
  }
  if (pthread_cond_destroy(&cv_workers)) {
    perror("Error: can t destroy condition variable \n");
  }
}

void thPool::add_task_in_queue(std::function<void()> f) {
  if (pthread_mutex_lock(&mtx)) {
    perror("Error locking the mutex!\n");
  }
  tasks.push(f);
  pthread_cond_signal(&cv);
  if (pthread_mutex_unlock(&mtx)) {
    perror("Error unlocking the mutex\n");
  }
}

void *thPool::execute(void *arg) {
  auto *pool = static_cast<thPool *>(arg);

  while (true) {
    std::function<void()> task;

    pthread_mutex_lock(&pool->mtx);
    while (pool->tasks.empty() && !pool->ok) {
      pthread_cond_wait(&pool->cv, &pool->mtx);
    }

    if (pool->ok && pool->tasks.empty()) {
      pthread_mutex_unlock(&pool->mtx);
      break;
    }

    if (check_for_resize(pool))
      add_thread(pool);

    task = std::move(pool->tasks.front());
    pool->tasks.pop();

    pool->working++;

    pthread_mutex_unlock(&pool->mtx);

    task();

    pthread_mutex_lock(&pool->mtx);
    pool->working--;
    if (pool->tasks.empty() && pool->working == 0) {
      pthread_cond_signal(&pool->cv_workers);
    }
    pthread_mutex_unlock(&pool->mtx);
  }

  return nullptr;
}
