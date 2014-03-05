#include <chrono>

#include "Timing.hpp"

// https://connect.microsoft.com/VisualStudio/feedback/details/719443
#ifdef _WIN32
#  include <windows.h>
static double s_freq;
#endif

void InitTiming()
{
#ifdef _WIN32
    uint64 freq;
    QueryPerformanceFrequency( (LARGE_INTEGER*)&freq );
    s_freq = 1.0 / freq;
#endif
}

uint64 GetTime()
{
#ifdef _WIN32
    uint64 ret;
    QueryPerformanceCounter( (LARGE_INTEGER*)&ret );
    return uint64( ret * s_freq * 1000000 );
#else
    return std::chrono::time_point_cast<std::chrono::microseconds>( std::chrono::high_resolution_clock::now() ).time_since_epoch().count();
#endif
}
