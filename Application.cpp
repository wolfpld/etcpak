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

void Usage()
{
    fprintf( stderr, "Usage: etcpak input.png [-q quality]\n" );
}

int main( int argc, char** argv )
{
    DebugLog::AddCallback( &DebugCallback );

    int quality = 0;

    if( argc != 2 && argc != 4 )
    {
        Usage();
        return 1;
    }
    if( argc == 4 )
    {
        if( strcmp( argv[2], "-q" ) != 0 )
        {
            Usage();
            return 1;
        }

        quality = atoi( argv[3] );
    }

    auto bmp = std::make_shared<Bitmap>( argv[1] );
    auto bb = std::make_shared<BlockBitmap>( bmp, Channels::RGB );
    bmp.reset();

    auto bd = std::make_shared<BlockData>( bb, quality == 0 ? false : true );
    bb.reset();

    auto out = bd->Decode();
    out->Write( "out.png" );

    return 0;
}
