#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <optional>

struct NetworkException : public std::exception {
    std::string msg;
    NetworkException(std::string m) : msg(m) {}
    const char* what() const noexcept override {
        return msg.c_str();
    }
};

class AddrInfo {
private:
    addrinfo *result = 0;

    class Iterator {
    private:
        addrinfo* currentPtr;
    public:
        Iterator(addrinfo* ptr) : currentPtr(ptr) {}
        Iterator& operator++() {
            currentPtr = currentPtr->ai_next;
            return *this;
        }
        Iterator operator++(int) {
            Iterator copy = *this;
            currentPtr = currentPtr->ai_next;
            return copy;
        }
        const addrinfo& operator*() {
            return *currentPtr;
        }
        bool operator==(const Iterator& other) {
            return currentPtr == other.currentPtr;
        }
        bool operator!=(const Iterator& other) {
            return !(*this == other);
        }
    };
public:
    using iterator = Iterator;
public:
    AddrInfo(const std::optional<std::reference_wrapper<const std::string>> host, int port = 0, bool onlyIPv4 = false) {
        addrinfo hints {0};
        hints.ai_family = onlyIPv4 ? AF_INET : AF_UNSPEC;
        int error = getaddrinfo(host.has_value() ? host->get().c_str() : nullptr, port ? std::to_string(port).c_str() : 0, &hints, &result);
        if(error) {
            throw NetworkException(gai_strerror(error));
        }        
    }
    
    AddrInfo(const AddrInfo& other) = delete;
    AddrInfo& operator=(const AddrInfo& other) = delete;

    Iterator begin() {
        return {result};
    }
    Iterator end() {
        return {nullptr};
    }

    ~AddrInfo() {
        if(result) {
            freeaddrinfo(result);
        }
    }
};

// int main() {
//     AddrInfo ai(std::nullopt, 80);
//     for(const addrinfo& aii : ai) {
//         const sockaddr& sa = *aii.ai_addr;
//         if(sa.sa_family == AF_INET) {
//             char ip[INET_ADDRSTRLEN];
//             inet_ntop(AF_INET, &((sockaddr_in*) &sa)->sin_addr, ip, INET_ADDRSTRLEN);
//             std::cout << ip << '\n';
//         }
//         else if(sa.sa_family == AF_INET6) {
//             char ip[INET6_ADDRSTRLEN];
//             inet_ntop(AF_INET6, &((sockaddr_in6*) &sa)->sin6_addr, ip, INET6_ADDRSTRLEN);
//             std::cout << ip << '\n';
//         }
//     }
// }