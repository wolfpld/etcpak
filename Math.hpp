#ifndef __DARKRL__MATH_HPP__
#define __DARKRL__MATH_HPP__

#include "Types.hpp"

template<typename T>
inline T AlignPOT( T val )
{
    if( val == 0 ) return 1;
    val--;
    for( unsigned int i=1; i<sizeof( T ) * 8; i <<= 1 )
    {
        val |= val >> i;
    }
    return val + 1;
}

inline int CountSetBits( uint32 val )
{
    val -= ( val >> 1 ) & 0x55555555;
    val = ( ( val >> 2 ) & 0x33333333 ) + ( val & 0x33333333 );
    val = ( ( val >> 4 ) + val ) & 0x0f0f0f0f;
    val += val >> 8;
    val += val >> 16;
    return val & 0x0000003f;
}

inline int CountLeadingZeros( uint32 val )
{
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    return 32 - CountSetBits( val );
}

inline float sRGB2linear( float v )
{
    return v * ( v * ( v * 0.305306011f + 0.682171111f ) + 0.012522878f );
}

inline float sqrtfast( float v )
{
    union
    {
        int i;
        float f;
    } u;

    u.f = v;
    u.i -= 1 << 23;
    u.i >>= 1;
    u.i += 1 << 29;
    return u.f;
}

inline float linear2sRGB( float v )
{
    float s1 = sqrtfast( v );
    float s2 = sqrtfast( s1 );
    float s3 = sqrtfast( s2 );
    return 0.585122381f * s1 + 0.783140355f * s2 - 0.368262736f * s3;
}

template<class T>
inline T SmoothStep( T x )
{
    return x*x*(3-2*x);
}

#endif
