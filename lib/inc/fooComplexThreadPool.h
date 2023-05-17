//
// Created by Michael Uman on 2/8/21.
//

#ifndef FOOBAR_FOOCOMPLEXTHREADPOOL_H
#define FOOBAR_FOOCOMPLEXTHREADPOOL_H

#include <atomic>
#include <fooQueue.h>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

//#define EXTRA_DEBUG_THREAD 1

namespace foo {

namespace utils {
template <typename T> struct jobDescription {
  std::string jobName{"unused"};
  std::thread::id jobThread{};
  std::size_t jobId{SIZE_MAX};
  T data{};

  jobDescription(const std::string name, T initial)
      : jobName(name), data{initial} {}

  jobDescription() = default;
};

template <typename T> class ComplexThreadPool {
public:
  using jobFunc = std::function<T(jobDescription<T>)>;
  using jobPair = std::pair<jobFunc, jobDescription<T>>;
  using jobVec = std::vector<jobDescription<T>>;

private:
  fooQueue<jobPair> jobs;
  std::vector<std::thread> thread_pool;
  std::size_t thread_count{SIZE_MAX};
  std::atomic_bool run_threads{false};
  std::atomic_size_t last_jid{0};

  std::mutex out_mutex;
  jobVec result_vec;

  void threadFunc() {
    while (run_threads) {
      jobPair jp;

      if (jobs.try_pop(jp, std::chrono::microseconds(1000))) {
        auto fn = jp.first;
        auto jd = jp.second;
        jd.jobThread = std::this_thread::get_id();
        jd.jobId = ++last_jid;

        // Call thread function
        jd.data = fn(jd);
        {
          std::lock_guard<std::mutex> guard(out_mutex);
          result_vec.push_back(std::move(jd));
        }
      }
    }
  }

public:
  ComplexThreadPool(std::size_t num_threads = 8) : thread_count(num_threads) {
    thread_pool.reserve(thread_count);

    run_threads = true;
    for (std::size_t i = 0; i < thread_count; i++) {
      auto fn = std::bind(&ComplexThreadPool<T>::threadFunc, this);
      thread_pool.emplace_back(std::thread(fn));
    }
  }

  virtual ~ComplexThreadPool() { stop_threads(); }

  void queue_job(jobPair jp) { jobs.push(jp); }

  void stop_threads() {
    run_threads = false;
    for (auto &thread : thread_pool) {
      thread.join();
    }
  }

  /**
   * Check if there are any jobs still in the job queue.
   *
   * @return true if there are jobs.
   */
  bool jobs_pending() { return !jobs.empty(); }

  std::size_t result_count() {
    std::lock_guard<std::mutex> guard(out_mutex);
    return result_vec.size();
  }

  jobVec get_results() {
    std::lock_guard<std::mutex> guard(out_mutex);
    return result_vec;
  }
};
template <typename T>
std::ostream &operator<<(std::ostream &os, const jobDescription<T> &jd) {
  os << "jobDescription( name : \"" << jd.jobName
     << "\", thr id : " << jd.jobThread << ", job id : " << jd.jobId << " )";
  return os;
}
} // namespace utils
} // namespace foo

#endif // FOOBAR_FOOCOMPLEXTHREADPOOL_H
