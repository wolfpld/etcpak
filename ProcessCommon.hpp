#ifndef __PROCESSCOMMON_HPP__
#define __PROCESSCOMMON_HPP__

#include <assert.h>
#include <stddef.h>

template<class T>
static size_t GetLeastError( const T* err, size_t num )
{
    size_t idx = 0;
    for( size_t i=1; i<num; i++ )
    {
        if( err[i] < err[idx] )
        {
            idx = i;
        }
    }
    return idx;
}

static inline size_t GetBufId( size_t i, size_t base )
{
    assert( i < 16 );
    assert( base < 4 );
    if( base % 2 == 0 )
    {
        return base * 2 + 1 - i / 8;
    }
    else
    {
        return base * 2 + 1 - ( ( i / 2 ) % 2 );
    }
}

#endif
