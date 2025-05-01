#include <mutex>
#include <memory>

#include "bufferpool.h"

Buffer::Buffer() {
    data = std::make_unique<char[]>(BUFFER_SIZE);
    size = desiredSize = 0;
}

BufferDeleter::BufferDeleter(): BufferDeleter(std::weak_ptr<BufferPool>()) {}
BufferDeleter::BufferDeleter(std::weak_ptr<BufferPool> bp): bp(bp) {}
void BufferDeleter::operator()(Buffer* ptr) {
    std::shared_ptr<BufferPool> pool = bp.lock();
    if(pool) {
        ptr->desiredSize = ptr->size = ptr->start = 0;
        pool->put(BufferPool::BufferPtr(ptr, *this));
    }
    else {
        delete ptr;
    }
}

void BufferPool::init(int n) {
    deleter = BufferDeleter(weak_from_this());
    for(int i = 0; i < n; ++i) {
        buffers.emplace(new Buffer, deleter);
    }
}

BufferPool::BufferPtr BufferPool::fetch() {
    std::lock_guard<std::mutex> lk(mutex);
    if(buffers.empty()) {
        return BufferPtr(new Buffer, deleter);
    }
    
    BufferPtr toRet = std::move(buffers.front());
    buffers.pop();
    return toRet;
}

void BufferPool::put(BufferPtr buffer) {
    std::lock_guard<std::mutex> lk(mutex);
    buffers.push(std::move(buffer));
}

std::shared_ptr<BufferPool> BufferPool::create(int n) {
    std::shared_ptr<BufferPool> pool(new BufferPool);
    pool->init(n);
    return pool;
}