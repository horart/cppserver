#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <optional>

#include "exceptions.h"

#include "addrinfo.h"

    
AddrInfo::Iterator::Iterator(addrinfo* ptr) : currentPtr(ptr) {}
AddrInfo::Iterator& AddrInfo::Iterator::operator++() {
    currentPtr = currentPtr->ai_next;
    return *this;
}
AddrInfo::Iterator AddrInfo::Iterator::operator++(int) {
    Iterator copy = *this;
    currentPtr = currentPtr->ai_next;
    return copy;
}
const addrinfo& AddrInfo::Iterator::operator*() {
    return *currentPtr;
}
bool AddrInfo::Iterator::operator==(const Iterator& other) {
    return currentPtr == other.currentPtr;
}
bool AddrInfo::Iterator::operator!=(const Iterator& other) {
    return !(*this == other);
}


AddrInfo::AddrInfo(const std::optional<std::reference_wrapper<const std::string>> host, int port, bool onlyIPv4) {
    addrinfo hints {};
    hints.ai_family = onlyIPv4 ? AF_INET : AF_UNSPEC;
    int error = getaddrinfo(host.has_value() ? host->get().c_str() : nullptr, port ? std::to_string(port).c_str() : 0, &hints, &result);
    if(error) {
        throw NetworkException(gai_strerror(error));
    }        
}

AddrInfo::Iterator AddrInfo::begin() {
    return {result};
}
AddrInfo::Iterator AddrInfo::end() {
    return {nullptr};
}

AddrInfo::~AddrInfo() {
    if(result) {
        freeaddrinfo(result);
    }
}

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