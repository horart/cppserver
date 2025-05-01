#ifndef THREADPOOL_H
#define THREADPOOL_H


#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <memory>
#include <tuple>

class FunctionalBase {
public:
    virtual void operator()() = 0;
    virtual ~FunctionalBase() {}
};

class ThreadPool {
private:
    std::mutex mutex;
    std::queue<std::unique_ptr<FunctionalBase>> queue;
    std::condition_variable cv;
    bool running = true;
    std::vector<std::thread> threads;
private:
    void executor();
public:
    ThreadPool(int n = std::thread::hardware_concurrency());
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename F, typename ...Args>
    void enqueue(F&& func, Args&&... args)
        requires (!std::is_member_function_pointer_v<std::decay_t<F>>);

    template <typename F, typename T, typename ...Args>
    void enqueue(F&& func, T&& obj, Args&&... args) 
        requires std::is_member_function_pointer_v<std::decay_t<F>>;

    ~ThreadPool();
};

#include "../src/threadpool.tpp"

#endif