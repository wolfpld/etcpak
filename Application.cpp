#include <stdio.h>
#include <memory>

#include "Bitmap.hpp"
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

    if( argc != 2 )
    {
        fprintf( stderr, "Usage: etcpak input.png\n" );
        return 1;
    }

    Bitmap b( argv[1] );

    return 0;
}
