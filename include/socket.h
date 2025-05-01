#ifndef SOCKET_H
#define SOCKET_H

#include "errno.h"
#include "string.h"
#include <sys/socket.h>

#include <memory>

#include "exceptions.h"
#include "bufferpool.h"
#include <iostream>


class Socket {
    public:
        int fd;
        BufferPool::BufferPtr buffer;
    public:
        Socket();
        Socket(int fd, BufferPool::BufferPtr buffer = nullptr);
        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;
        
        Socket(Socket&& other) noexcept;
        Socket& operator=(Socket&& other) noexcept;
    
        void sendall(const char* buf, size_t len, int flags = 0);
    
        operator int();
    
        ~Socket();
    };
    

#endif