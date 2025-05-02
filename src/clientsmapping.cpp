#include <mutex>
#include <memory>

#include "socket.h"

#include "clientsmapping.h"

ClientsMapping::ClientsMapping() {}
ClientsMapping::ClientsMapping(std::shared_ptr<BufferPool> bp): bp(bp) {}

void ClientsMapping::init(std::shared_ptr<BufferPool> bp) {
    this->bp = bp;
}

// Returns true if the fd was present in the mapping, false - otherwise
bool ClientsMapping::disconnect(int fd) {
    std::lock_guard<std::mutex> lk(mutex);
    return clients.erase(fd);
}

// Returns the associated socket, if there is no such one - creates it
std::shared_ptr<BufferedSocket> ClientsMapping::getOrCreateClient(int fd) {
    std::lock_guard<std::mutex> lk(mutex);
    auto it = clients.find(fd);

    if(it == clients.end()) {
        it = clients.emplace(fd, std::make_shared<BufferedSocket>(fd, bp->fetch())).first;
    }
    return it->second;
}