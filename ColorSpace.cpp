#include <new>
#include <math.h>

#include "Math.hpp"
#include "ColorSpace.hpp"

namespace Color
{

    XYZ::XYZ( float _x, float _y, float _z )
        : x( _x )
        , y( _y )
        , z( _z )
    {
    }

    XYZ::XYZ( const v3b& rgb )
    {
        const float r = rgb.x / 255.f;
        const float g = rgb.y / 255.f;
        const float b = rgb.z / 255.f;

        const float rl = sRGB2linear( r );
        const float gl = sRGB2linear( g );
        const float bl = sRGB2linear( b );

        x = 0.4124f * rl + 0.3576f * gl + 0.1805f * bl;
        y = 0.2126f * rl + 0.7152f * gl + 0.0722f * bl;
        z = 0.0193f * rl + 0.1192f * gl + 0.9505f * bl;
    }


    Lab::Lab()
        : L( 0 )
        , a( a )
        , b( b )
    {
    }

    Lab::Lab( float L, float a, float b )
        : L( L )
        , a( a )
        , b( b )
    {
    }

    static float labfunc( float t )
    {
        const float p1 = (6.f/29.f)*(6.f/29.f)*(6.f/29.f);
        const float p2 = (1.f/3.f)*(29.f/6.f)*(29.f/6.f);
        const float p3 = (4.f/29.f);

        if( t > p1 )
        {
            return pow( t, 1.f/3.f );
        }
        else
        {
            return p2 * t + p3;
        }
    }

    static const XYZ white( v3b( 255, 255, 255 ) );
    static const v3f rwhite( 1.f / white.x, 1.f / white.y, 1.f / white.z );

    Lab::Lab( const XYZ& xyz )
    {
        L = 116 * labfunc( xyz.y * rwhite.y ) - 16;
        a = 500 * ( labfunc( xyz.x * rwhite.x ) - labfunc( xyz.y * rwhite.y ) );
        b = 200 * ( labfunc( xyz.y * rwhite.y ) - labfunc( xyz.z * rwhite.z ) );
    }

    Lab::Lab( const v3b& rgb )
    {
        new(this) Lab( XYZ( rgb ) );
    }

}
