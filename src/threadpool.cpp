#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <functional>
#include <memory>
#include <tuple>

#include "threadpool.h"


void ThreadPool::executor() {
    std::unique_lock<std::mutex> lk(mutex);
    for(;;) {
        cv.wait(lk, [this] {
            return !queue.empty() || !running;
        });
        
        if(!running && queue.empty()) {
            return;
        }
        std::unique_ptr<FunctionalBase> task = std::move(queue.front());
        queue.pop();
        lk.unlock();
        (*task)();
        lk.lock();
    }
}

ThreadPool::ThreadPool(int n) {
    for(int i = 0; i < n; ++i) {
        threads.emplace_back(&ThreadPool::executor, this);
    }
}


ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lk(mutex);
        running = false;
    }
    cv.notify_all();

    for(std::thread& thread : threads) {
        thread.join();
    }
}