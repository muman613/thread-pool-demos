//
// Created by Michael Uman on 11/6/20.
//

#ifndef FOOBAR_FOOQUEUE_H
#define FOOBAR_FOOQUEUE_H

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace foo {
    namespace utils {
        template<typename T>
        class fooQueue {

        using queue_type = std::queue<T>;

        public:
            fooQueue() = default;
            ~fooQueue() = default;;

            void push(const T & item) {
              std::lock_guard<std::mutex> lock(mutex_);
              q_.push(item);
              cond_.notify_one();
            }

            T pop() {
              std::unique_lock<std::mutex> lock(mutex_);
              while (q_.empty()) {
                cond_.wait(lock);
              }
              T val = q_.front();
              q_.pop();
              return val;
            }

            bool try_pop(T & val, std::chrono::microseconds to)
            {
              std::unique_lock<std::mutex> lock(mutex_);
              if (!cond_.wait_for(lock, to, [&]() { return !q_.empty(); }))
                return false;
              val = std::move(q_.front());
              q_.pop();
              return true;
            }

            std::size_t size() const {
              std::unique_lock<std::mutex> lock(mutex_);
              return q_.size();
            }

            bool empty() const {
              return size() == 0;
            }

        private:
            mutable std::mutex      mutex_;
            std::condition_variable cond_;
            queue_type              q_;
        };
    }
}

#endif //FOOBAR_FOOQUEUE_H
