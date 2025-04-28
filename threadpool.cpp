#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <functional>
#include <memory>
#include <tuple>

class FunctionalBase {
public:
    virtual void operator()() = 0;
    virtual ~FunctionalBase() {}
};

template <typename F>
class Functional : public FunctionalBase {
private:
    F func;
public:
    Functional(F&& f): func(std::move(f)) {}
    void operator()() override {
        func();
    }
};


class Threadpool {
private:
    std::mutex mutex;
    std::queue<std::unique_ptr<FunctionalBase>> queue;
    std::condition_variable cv;
    bool running = true;
    std::vector<std::thread> threads;
private:
    void executor() {
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
public:
    Threadpool(int n = std::thread::hardware_concurrency()) {
        for(int i = 0; i < n; ++i) {
            threads.emplace_back(&Threadpool::executor, this);
        }
    }
    Threadpool(const Threadpool&) = delete;
    Threadpool& operator=(const Threadpool&) = delete;

    template <typename F, typename ...Args>
    void enqueue(F&& func, Args&&... args)
    requires (!std::is_member_function_pointer_v<std::decay_t<F>>)
    {
        std::lock_guard<std::mutex> lk(mutex);
        auto task = [f = std::forward<F>(func), ...args = std::forward<Args>(args)]() mutable {
            f(std::forward<Args>(args)...);
        };

        queue.push(std::make_unique<Functional<decltype(task)>>(std::move(task)));
        cv.notify_one();
    }

    template <typename F, typename T, typename ...Args>
    void enqueue(F&& func, T&& obj, Args&&... args) 
    requires std::is_member_function_pointer_v<std::decay_t<F>>
    {
        std::lock_guard<std::mutex> lk(mutex);
        auto task = [f = std::forward<F>(func), o = std::forward<T>(obj), ...args = std::forward<Args>(args)]() mutable {
            (std::forward<decltype(o)>(o)->*f) (std::forward<decltype(args)>(args)...);
        };
        auto ptr = std::make_unique<Functional<decltype(task)>>(std::move(task));
        queue.emplace(ptr.release());
        cv.notify_one();
    }

    ~Threadpool() {
        {
            std::lock_guard<std::mutex> lk(mutex);
            running = false;
        }
        cv.notify_all();

        for(std::thread& thread : threads) {
            thread.join();
        }
    }
};