#include <stdio.h>
#include <memory>

#include "Bitmap.hpp"
#include "BlockBitmap.hpp"
#include "BlockData.hpp"
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

    auto bmp = std::make_shared<Bitmap>( argv[1] );
    auto bb = std::make_shared<BlockBitmap>( bmp, Channels::RGB );
    bmp.reset();

    auto bd = std::make_shared<BlockData>( bb, false );
    bb.reset();

    auto out = bd->Decode();
    out->Write( "out.png" );

    return 0;
}
