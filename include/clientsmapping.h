#ifndef CLIENTSMAPPING_H
#define CLIENTSMAPPING_H

#include <unordered_map>
#include <memory>

#include "socket.h"


class ClientsMapping {
private:
    std::unordered_map<int, std::shared_ptr<Socket>> clients;
    std::shared_ptr<BufferPool> bp;
    std::mutex mutex;
public:
    ClientsMapping();
    ClientsMapping(std::shared_ptr<BufferPool>);
    void init(std::shared_ptr<BufferPool>);
    // Returns true if the fd was present in the mapping, false - otherwise
    bool disconnect(int fd);

    // Returns the associated socket, if there is no such one - creates it
    std::shared_ptr<Socket> getOrCreateClient(int fd);
};

#endif