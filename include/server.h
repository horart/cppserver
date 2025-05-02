#ifndef SERVER_H
#define SERVER_H


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>

#include <thread>
#include <utility>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <mutex>
#include <string_view>

#include "threadpool.h"
#include "socket.h"
#include "addrinfo.h"
#include "exceptions.h"
#include "clientsmapping.h"
#include "bufferpool.h"


extern inline const int BUFFER_NUMBER = 128;

class Server {
private:
    Socket serverfd;
    std::shared_ptr<BufferPool> bp;
    inline static std::atomic<bool> running = false;


protected:
    ClientsMapping clients;
    ThreadPool toProcess;
    ThreadPool toSend;


protected:
    // This method is called when there is a complete message to process
    virtual void process(BufferPool::BufferPtr buf, std::shared_ptr<BufferedSocket> client);
    // This method is called when there is a need to send. 
    virtual void send(BufferPool::BufferPtr buf, std::shared_ptr<BufferedSocket> client);
    // This method is called when there is new data to the stream
    virtual void parseAndEnqueue(std::shared_ptr<BufferedSocket> client);
    // This method is called when the buffer is overflown by new client message
    virtual void sendMessageTooLong(std::shared_ptr<BufferedSocket> client);

protected:
    BufferPool::BufferPtr getBuffer();

public:
    Server() = delete;
    Server(const addrinfo& addr, int maxConnections = 10);
    
    void run();
    static void sigint(int);
};

#endif