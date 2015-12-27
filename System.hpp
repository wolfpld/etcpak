#ifndef __DARKRL__SYSTEM_HPP__
#define __DARKRL__SYSTEM_HPP__

#include <thread>

#include "Types.hpp"

class System
{
public:
    System() = delete;

    static uint CPUCores();
    static void SetThreadName( std::thread& thread, const char* name );
};

#endif
