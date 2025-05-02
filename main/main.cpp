#include "addrinfo.h"
#include "server.h"

#include <iostream>
#include "csignal"

int main() {
    AddrInfo ai(std::nullopt, 1234, true);
    std::signal(SIGINT, &Server::sigint);
    Server a(*ai.begin());
    a.run();
}