//
// Created by Michael Uman on 2/8/21.
//

#ifndef FOOBAR_FOOSIMPLETHREADPOOL_H
#define FOOBAR_FOOSIMPLETHREADPOOL_H

#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <fooQueue.h>

namespace foo {
    namespace utils {
        class fooSimpleThreadPool
        {
            using threadVec = std::vector<std::thread>;
            using jobFunc = std::function<bool()>;
            using jobQueue = foo::utils::fooQueue<jobFunc>;

            threadVec thread_vec;
            std::size_t thread_count{0};
            std::atomic_bool run_threads{false};

            jobQueue jobs;

            void threadFunc();

        public:
            explicit fooSimpleThreadPool( std::size_t thread_count = 8 );

            virtual ~fooSimpleThreadPool();

            void stop_threads();

            void queue_job( jobFunc fn );

            /**
             * Test if there are any jobs in the jobs queue.
             *
             * @return true if the jobs queue is not empty.
             */
            bool jobs_pending();

            /**
             * Get # of threads in this pool.
             *
             * @return # of threads
             */
            std::size_t count() const;

        };
    }
}

#endif //FOOBAR_FOOSIMPLETHREADPOOL_H
