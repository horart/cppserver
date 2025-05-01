#ifndef BUFFER_H
#define BUFFER_H


#include <queue>
#include <memory>
#include <mutex>

class BufferPool;

extern inline const size_t BUFFER_SIZE = 1024;

struct Buffer {
    std::unique_ptr<char[]> data;
    size_t size = 0;
    size_t desiredSize = 0;
    size_t start = 0;
    Buffer();
};

struct BufferDeleter {
    std::weak_ptr<BufferPool> bp;
    BufferDeleter();
    BufferDeleter(std::weak_ptr<BufferPool>);
    void operator()(Buffer* buffer);
};


class BufferPool : std::enable_shared_from_this<BufferPool> {
friend class BufferDeleter;
public:
    using BufferPtr = std::unique_ptr<Buffer, BufferDeleter>;
private:
    std::queue<BufferPtr> buffers;
    std::mutex mutex;
    BufferDeleter deleter;
private:
    BufferPool() = default;
    void put(BufferPtr);
    void init(int n);
public:
    BufferPool(const BufferPool&) = delete;
    BufferPool& operator=(const BufferPool&) = delete;

    BufferPtr fetch();

    static std::shared_ptr<BufferPool> create(int n);
};

#endif