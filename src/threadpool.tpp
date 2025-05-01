#include <type_traits>
#include <memory>
#include <utility>

#include "threadpool.h"


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

template <typename F, typename ...Args>
void ThreadPool::enqueue(F&& func, Args&&... args)
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
void ThreadPool::enqueue(F&& func, T&& obj, Args&&... args) 
    requires std::is_member_function_pointer_v<std::decay_t<F>>
{
    std::lock_guard<std::mutex> lk(mutex);
    auto task = [f = std::forward<F>(func), o = std::forward<T>(obj), ...args = std::forward<Args>(args)]() mutable {
        (std::forward<decltype(o)>(o)->*f) (std::forward<decltype(args)>(args)...);
    };
    using LambdaType = decltype(task);
    auto ptr = std::make_unique<Functional<LambdaType>>(std::move(task));
    queue.push(std::move(ptr));
    cv.notify_one();
}