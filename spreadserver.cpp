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

#include "addrinfo.cpp"
#include "threadpool.cpp"

const size_t BUFFER_SIZE = 1024;

struct Buffer {
    std::unique_ptr<char[]> data;
    size_t size = 0;
    size_t desiredSize = 0;
    size_t start = 0;
    Buffer() {
        data = std::make_unique<char[]>(BUFFER_SIZE);
        size = desiredSize = 0;
    } 
};

class Socket {
public:
    int fd;
    std::unique_ptr<Buffer> buffer;
public:
    Socket(): fd(-1), buffer(std::make_unique<Buffer>()) {}
    Socket(int fd): fd(fd), buffer(std::make_unique<Buffer>()) {}
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    
    Socket(Socket&& other) noexcept: fd(other.fd) {
        other.fd = -1;
        buffer = std::move(other.buffer);
    }
    Socket& operator=(Socket&& other) noexcept {
        std::swap(fd, other.fd);
        buffer->data.swap(other.buffer->data);
        buffer->size = other.buffer->size;
        return *this;
    }

    void sendall(const char* buf, size_t len, int flags = 0) {
        size_t sent = 0;
        while(sent < len) {
            int r = ::send(fd, buf+sent, len-sent, flags);
            if(r == -1) {
                throw NetworkException(strerror(errno));
            }
            sent += r;
        }
    }

    bool recvn(char* buf, size_t n, int flags = 0) {
        size_t got = 0;
        while(got < n) {
            int r = ::recv(fd, buf+got, n-got, 0);
            if(r == 0) {
                return false;
            }
            if(r == -1) {
                throw NetworkException(strerror(errno));
            }
            got += r;
        }
        return true;
    }

    operator int() {
        return fd;
    }

    ~Socket() {
        if(fd != -1) {
            if(close(fd)) {
                std::cerr << "Failed to close a socket: " << strerror(errno);
            }
        }
    }
};


class Server {
private:
    Socket serverfd;
    Threadpool toProcess;
    Threadpool toSend;
    std::unordered_map<int, std::shared_ptr<Socket>> clients;

private:
    void parseAndEnqueue(std::shared_ptr<Socket> client) {
        *(client->buffer->data.get() + client->buffer->start + client->buffer->size) = 0;
        
        while(true) {
            if(client->buffer->desiredSize && client->buffer->size >= client->buffer->desiredSize) {
                std::unique_ptr<Buffer> buf = std::make_unique<Buffer>();
                buf.swap(client->buffer);
                char* const dataEndPtr = buf->data.get() + buf->start + buf->size;
                char* ptr = buf->data.get() + buf->start + buf->desiredSize;
                char* cb = client->buffer->data.get();
                while(ptr != dataEndPtr) {
                    *cb++ = *ptr++;
                }
                *cb = 0;
                
                *(buf->data.get() + buf->start + buf->desiredSize) = 0;
                client->buffer->size = cb - client->buffer->data.get();
                toProcess.enqueue(&Server::process, this, std::move(buf), client);
            }
            else if(client->buffer->desiredSize == 0) {
                char* ptr = client->buffer->data.get();
                char* const endPtr = client->buffer->data.get() + client->buffer->size;
                while(ptr != endPtr && !('0' <= *ptr && *ptr <= '9')) {
                    ++ptr;
                }
                char* const firstDigit = ptr;
                if(ptr == endPtr) {
                    client->buffer->start = client->buffer->size = 0;
                    return;
                }
                else {
                    client->buffer->desiredSize = strtol(ptr, &ptr, 10);
                    client->buffer->start = ptr - client->buffer->data.get();
                    client->buffer->size -= client->buffer->start;
                }
            }
            else {
                return;
            }
        }
    }
public:
    Server(const addrinfo& addr, int maxConnections = 10): toProcess(), toSend(1) {
        serverfd = socket(addr.ai_family, addr.ai_socktype, addr.ai_protocol);
        if(serverfd == -1) {
            throw NetworkException(strerror(errno));
        }
        int one = 1;
        if(
            setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1
         || bind(serverfd, addr.ai_addr, addr.ai_addrlen) == -1 
         || listen(serverfd, maxConnections) == -1) {
            throw NetworkException(strerror(errno));
        }
    }
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void run() {
        int epollfd = epoll_create1(0);
        if(epollfd == -1) {
            throw NetworkException(strerror(errno));
        }
        epoll_event ev;
        ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;
        ev.data.fd = serverfd;
        if(epoll_ctl(epollfd, EPOLL_CTL_ADD, serverfd, &ev)) {
            throw NetworkException(strerror(errno));
        }
        epoll_event events[1024];
        while(1) {
            int numberOfEvents = epoll_wait(epollfd, events, 1024, 1000);
            std::cout << numberOfEvents << '\n';
            for(int i = 0; i < numberOfEvents; ++i) {
                if(events[i].data.fd == serverfd) {
                    if(events[i].events & EPOLLHUP || events[i].events & EPOLLERR) {
                        std::cerr << strerror(errno);
                        continue;
                    }
                    sockaddr clientaddr;
                    socklen_t clientaddrLen;
                    int clientfd = accept(serverfd, &clientaddr, &clientaddrLen);
                    if(clientfd == -1) {
                        std::cerr << "couldn't accept: " << strerror(errno);
                    }
                    ev.data.fd = clientfd;
                    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev)) {
                        std::cerr << "couldn't add to poll: " << strerror(errno);
                        close(clientfd);
                    }

                    continue;
                }
                
                if(events[i].events & EPOLLHUP || events[i].events & EPOLLERR) {
                    std::cerr << "Connection lost with client: " << strerror(errno);
                    if(!clients.erase(events[i].data.fd)) {
                        close(events[i].data.fd);
                    }
                    continue;
                }

                std::shared_ptr<Socket> client;
                if(auto it = clients.find(events[i].data.fd); it == clients.end()) {
                    client = clients.emplace((int) (events[i].data.fd), std::make_shared<Socket>((int) events[i].data.fd)).first->second;
                }
                else {
                    client = it->second;
                }

                int bytesGot = recv(
                    events[i].data.fd,
                    client->buffer->data.get() + client->buffer->start + client->buffer->size,
                    BUFFER_SIZE - client->buffer->size,
                    0
                );
                if(bytesGot <= 0) {
                    clients.erase(*client);
                }

                client->buffer->size += bytesGot;
                parseAndEnqueue(client);

                
                
            }
        }
    }

    void process(std::unique_ptr<Buffer> buf, std::shared_ptr<Socket> client) {
        std::unique_ptr<Buffer> b = std::make_unique<Buffer>();
        char* p;
        int i = strtol(buf->data.get() + buf->start, &p, 10);
        size_t len = sprintf(b->data.get(), "%x", i);
        b->desiredSize = len;
        toSend.enqueue(&Server::send, this, std::move(b), client);
    }

    void send(std::unique_ptr<Buffer> buf, std::shared_ptr<Socket> client) {
        client->sendall(buf->data.get() + buf->start, buf->desiredSize);
    }
    
};


int main() {
    AddrInfo ai(std::nullopt, 1234, true);
    Server a(*ai.begin());
    a.run();
}