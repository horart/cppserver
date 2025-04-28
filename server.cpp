#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <thread>
#include <utility>
#include <string>
#include <stdexcept>
#include <atomic>

#include "addrinfo.cpp"

class Server {
private:
    class Socket {
    public:
        int fd;
    public:
        Socket(): fd(-1) {}
        Socket(int fd): fd(fd) {}
        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;
        
        Socket(Socket&& other): fd(other.fd) {
            other.fd = -1;
        }
        Socket& operator=(Socket&& other) {
            std::swap(fd, other.fd);
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

    Socket serverfd;
    std::atomic<int> state = 0;
public:
    Server(const addrinfo& addr, int maxConnections = 10) {
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
        sockaddr clientAddr;
        socklen_t clientAddrSize;
        int clientfd;
        while((clientfd = accept(serverfd, &clientAddr, &clientAddrSize)) != -1) {
            std::thread(&Server::processClient, this, Socket(clientfd)).detach();
        }
        throw NetworkException(strerror(errno));
    }

    void processClient(Socket client) {
        std::string msg = "Hello there\n";
        ptrdiff_t ptr = 0;
        client.sendall(msg.c_str(), msg.size(), 0);
        char buf;
        while(client.recvn(&buf, 1)) {
            if(buf == 'F') {
                std::string s = std::to_string(state);
                s += '\n';
                client.sendall(s.c_str(), s.size());
            }
            else if(buf == 'I') {
                ++state;
                std::string s = "OK\n";
                client.sendall(s.c_str(), s.size());
            }
        }
    }
};

int main() {
    AddrInfo ai(std::nullopt, 1234, true);
    Server a(*ai.begin());
    a.run();
}