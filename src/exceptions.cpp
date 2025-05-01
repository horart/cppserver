#include <string>
#include <stdexcept>

#include "exceptions.h"

NetworkException::NetworkException(std::string m) : msg(m) {}

const char* NetworkException::what() const noexcept {
    return msg.c_str();
}
