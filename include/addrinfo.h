#ifndef ADDRINFO_H
#define ADDRINFO_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <iostream>
#include <optional>

#include "exceptions.h"



class AddrInfo {
private:
    addrinfo *result = 0;
    class Iterator {
    private:
        addrinfo* currentPtr;
    public:
        Iterator(addrinfo* ptr);
        Iterator& operator++();
        Iterator operator++(int);
        const addrinfo& operator*();
        bool operator==(const Iterator& other);
        bool operator!=(const Iterator& other);
    };
public:
    using iterator = Iterator;
public:
    AddrInfo(const std::optional<std::reference_wrapper<const std::string>> host, int port = 0, bool onlyIPv4 = false);

    AddrInfo(const AddrInfo& other);
    AddrInfo& operator=(const AddrInfo& other) = delete;

    Iterator begin();
    Iterator end();

    ~AddrInfo();
};

#endif