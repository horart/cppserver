#include "errno.h"
#include "string.h"
#include <sys/socket.h>
#include <unistd.h>

#include <memory>

#include "exceptions.h"
#include "bufferpool.h"
#include <iostream>

#include "socket.h"

Socket::Socket(): fd(-1), buffer(nullptr) {}
Socket::Socket(int fd, BufferPool::BufferPtr buffer): fd(fd), buffer(std::move(buffer)) {}
    
Socket::Socket(Socket&& other) noexcept: fd(other.fd) {
    other.fd = -1;
    buffer = std::move(other.buffer);
}
Socket& Socket::operator=(Socket&& other) noexcept {
    std::swap(fd, other.fd);
    std::swap(buffer, other.buffer);
    return *this;
}

void Socket::sendall(const char* buf, size_t len, int flags) {
    size_t sent = 0;
    while(sent < len) {
        int r = ::send(fd, buf+sent, len-sent, flags);
        if(r == -1) {
            throw NetworkException(strerror(errno));
        }
        sent += r;
    }
}

Socket::operator int() {
    return fd;
}

Socket::~Socket() {
    if(fd != -1) {
        if(close(fd)) {
            std::cerr << "Failed to close a socket: " << strerror(errno);
        }
    }
}