# C++ General-Purpose Server

## Requirements:
* C++20

## Building
```
cmake .
cd build
make
```
## Usage
* Binary executable `build/server` contains a demo.
* The library is contained in `build/libServerLib.a`

## Classes
* `AddrInfo` - wrapper for the Unix counterpart
* `BufferPool` - pre-allocated expanding-on-need buffer pool
* `Socket` - wrapper for the Unix counterpart
* `BufferedSocket` - socket with a buffer
* `ThreadPool` - threadpool with a fixed number of threads running
* `Server` - virtual class with server logic

## Example
```c++
#include <memory>
#include <csignal>
class MyServer : public Server {
    void parseAndEnqueue(std::shared_ptr<BufferedSocket>) override {
        ...
    }
    void sendMessageTooLong(std::shared_ptr<BufferedSocket>) override {
        ...
    }
    void process(BufferPool::BufferPtr buf, std::shared_ptr<BufferedSocket> override) {
        ...
    }
    void send(BufferPool::BufferPtr buf, std::shared_ptr<BufferedSocket> override) {
        ...
    }
};

int main() {
    AddrInfo ai;
    MyServer server(*ai.begin());
    std::signal(SIGINT, &Server::sigint);
    server.run();
}

```
