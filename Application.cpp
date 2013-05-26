#include <stdio.h>
#include <memory>

#include "Debug.hpp"

struct DebugCallback_t : public DebugLog::Callback
{
    void OnDebugMessage( const char* msg ) override
    {
        fprintf( stderr, "%s\n", msg );
    }
} DebugCallback;

int main( int argc, char** argv )
{
    DebugLog::AddCallback( &DebugCallback );

    return 0;
}
