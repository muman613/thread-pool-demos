//
// Created by Michael Uman on 2/8/21.
//

#include "fooSimpleThreadPool.h"

namespace foo {
namespace utils {
fooSimpleThreadPool::fooSimpleThreadPool(std::size_t thr_cnt)
    : thread_count(thr_cnt) {
  thread_vec.reserve(thread_count);

  run_threads = true;
  for (size_t i = 0; i < thread_vec.capacity(); i++) {
    auto fn = std::bind(&fooSimpleThreadPool::threadFunc, this);

    thread_vec.emplace_back(std::thread(fn));
  }
}

fooSimpleThreadPool::~fooSimpleThreadPool() {
  stop_threads();

  for (auto &thread : thread_vec) {
    thread.join();
  }
}

void fooSimpleThreadPool::threadFunc() {
  while (run_threads) {
    jobFunc fn;
    if (jobs.try_pop(fn, std::chrono::microseconds(1000))) {
      auto result = fn();
    }
  }
}

void fooSimpleThreadPool::stop_threads() {
  run_threads = false;
}

void fooSimpleThreadPool::queue_job(fooSimpleThreadPool::jobFunc fn) {
  jobs.push(std::move(fn));
}

bool fooSimpleThreadPool::jobs_pending() { return !jobs.empty(); }

std::size_t fooSimpleThreadPool::count() const { return thread_count; }
} // namespace utils
} // namespace foo