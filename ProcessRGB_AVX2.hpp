#ifndef __PROCESSRGB_AVX2_HPP__
#define __PROCESSRGB_AVX2_HPP__

#ifdef __SSE4_1__

#include "Types.hpp"

uint64 ProcessRGB_AVX2( const uint8* src );

#endif

#endif
