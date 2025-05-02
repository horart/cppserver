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
#include <csignal>
#include <functional>

#include "threadpool.h"
#include "socket.h"
#include "addrinfo.h"
#include "exceptions.h"
#include "clientsmapping.h"

#include "server.h"


BufferPool::BufferPtr Server::getBuffer() {
    return bp->fetch();
}

void Server::parseAndEnqueue(std::shared_ptr<Socket> client) {
    *(client->buffer->data.get() + client->buffer->start + client->buffer->size) = 0;
    
    while(true) {
        if(client->buffer->desiredSize && client->buffer->size >= client->buffer->desiredSize) {
            BufferPool::BufferPtr buf = getBuffer();
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
            char* ptr = client->buffer->data.get() + client->buffer->start;
            char* const endPtr = client->buffer->data.get() + client->buffer->start + client->buffer->size;
            while(ptr != endPtr && !('0' <= *ptr && *ptr <= '9')) {
                ++ptr;
            }
            if(ptr == endPtr) {
                client->buffer->start = client->buffer->size = 0;
                return;
            }
            else {
                client->buffer->desiredSize = strtol(ptr, &ptr, 10);
                const size_t oldStart = client->buffer->start;
                client->buffer->start = ptr - client->buffer->data.get();
                client->buffer->size -= client->buffer->start - oldStart;
            }
        }
        else {
            return;
        }
    }
}

Server::Server(const addrinfo& addr, int maxConnections): toProcess(), toSend(1), bp(BufferPool::create(BUFFER_NUMBER)) {
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
    clients.init(bp);
}

void Server::run() {
    running = true;
    Socket epollfd = epoll_create1(0);
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
        if(running == false) {
            break;
        }
        int numberOfEvents = epoll_wait(epollfd, events, 1024, 1000);
        for(int i = 0; i < numberOfEvents; ++i) {
            if(events[i].data.fd == serverfd) {
                if(events[i].events & EPOLLHUP || events[i].events & EPOLLERR) {
                    std::cerr << strerror(errno);
                    continue;
                }
                sockaddr clientaddr;
                socklen_t clientaddrLen = sizeof(clientaddr);
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
                if(!clients.disconnect(events[i].data.fd)) {
                    close(events[i].data.fd);
                }
                continue;
            }

            std::shared_ptr<Socket> client = clients.getOrCreateClient(events[i].data.fd);

            size_t avail = BUFFER_SIZE - client->buffer->start - client->buffer->size;
            if(avail == 0) {
                sendMessageTooLong(client);
                client->buffer = getBuffer();
            }
            int bytesGot = recv(
                events[i].data.fd,
                client->buffer->data.get() + client->buffer->start + client->buffer->size,
                avail,
                0
            );
            if(bytesGot <= 0) {
                clients.disconnect(*client);
            }

            client->buffer->size += bytesGot;
            parseAndEnqueue(client);
        }
    }
}

void Server::sendMessageTooLong(std::shared_ptr<Socket> client) {
    auto msg = "Message too long\n";
    client->sendall(msg, sizeof(msg));
}

void Server::sigint(int) {
    running = false;
}

void Server::process(BufferPool::BufferPtr buf, std::shared_ptr<Socket> client) {
    std::string_view q(buf->data.get() + buf->start, buf->desiredSize);
    if(q.starts_with("quit")) {
        clients.disconnect(*client);
        return;
    }
    BufferPool::BufferPtr b = getBuffer();
    char* p;
    int i = strtol(buf->data.get() + buf->start, &p, 10);
    size_t len = sprintf(b->data.get(), "%x", i);
    b->size = len;
    toSend.enqueue(&Server::send, this, std::move(b), client);
}

void Server::send(BufferPool::BufferPtr buf, std::shared_ptr<Socket> client) {
    client->sendall(buf->data.get() + buf->start, buf->size);
}
