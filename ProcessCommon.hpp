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

#endif
