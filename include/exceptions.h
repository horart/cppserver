#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>

struct NetworkException : public std::exception {
    std::string msg;
    NetworkException(std::string m);
    const char* what() const noexcept override;
};



#endif