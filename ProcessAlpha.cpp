#include "ProcessAlpha.hpp"

uint64_t ProcessAlpha( const uint8_t* src )
{
    uint64_t d = 0;

    {
        bool solid = true;
        const uint8_t* ptr = src + 1;
        const uint8_t ref = *src;
        for( int i=1; i<16; i++ )
        {
            if( ref != *ptr++ )
            {
                solid = false;
                break;
            }
        }
        if( solid )
        {
            return ref;
        }
    }

    return 0;
}
