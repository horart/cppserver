#include "addrinfo.h"
#include "server.h"

#include <iostream>

int main() {
    AddrInfo ai(std::nullopt, 1234, true);
    Server a(*ai.begin());
    a.run();
}