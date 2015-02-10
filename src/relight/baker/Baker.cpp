/**************************************************************************

 The MIT License (MIT)

 Copyright (c) 2015 Dmitry Sovetov

 https://github.com/dmsovetov

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 **************************************************************************/

#include "../BuildCheck.h"

#include "Baker.h"
#include "../Mesh.h"
#include "../Lightmap.h"
#include "../Scene.h"

#define RELIGHT_BAKE_FACES  (0)

namespace relight {

namespace bake {

// ** Baker::Baker
Baker::Baker( const Scene* scene ) : m_scene( scene )
{

}

// ** Baker::bakeInstance
RelightStatus Baker::bakeInstance( const Instance* instance )
{
    Lightmap* lightmap = instance->lightmap();
    if( !lightmap ) {
        return RelightInvalidCall;
    }

    int     width    = lightmap->width();
    int     height   = lightmap->height();
    Lumel*  lumels   = lightmap->lumels();
    int     progress = 0;

#if RELIGHT_BAKE_FACES
    const SubMesh&      sub     = instance->mesh()->submesh( 0 );
    const IndexBuffer&  indices = sub.m_indices;

    for( int i = 0; i < sub.m_totalFaces; i++ ) {
        bakeFace( instance, i );
        printf( "Baking face %d/%d\n", i + 1, sub.m_totalFaces );
    }
#else
    for( int j = 0; j < height; j++ ) {
        for( int i = 0; i < width; i++ ) {
            Lumel& lumel = lumels[j*width + i];

            if( lumel ) {
                bakeLumel( lumel );
            }
        }

        if( int( float( j ) / height * 100 ) != progress ) {
            progress = float( j ) / height * 100;
            printf( "Baking %d%%\n", progress );
        }
    }
#endif

    return RelightSuccess;
}

// ** Baker::bakeFace
void Baker::bakeFace( const Instance* instance, int index )
{
    Face      face      = instance->mesh()->face( index );
    Lightmap* lightmap  = instance->lightmap();

    // ** Calculate UV bounds
    Uv min, max;
    face.uvRect( min, max );

    int uStart = min.u * lightmap->width();
    int uEnd   = max.u * lightmap->width();
    int vStart = min.v * lightmap->height();
    int vEnd   = max.v * lightmap->height();

    // ** Initialize lumels
    for( int v = vStart; v <= vEnd; v++ ) {
        for( int u = uStart; u <= uEnd; u++ ) {
            Lumel& lumel = lightmap->lumel( u, v );
            if( !lumel ) {
                continue;
            }

            Uv uv( (u + 0.5f) / float( lightmap->width() ), (v + 0.5f) / float( lightmap->height() ) );
            Barycentric barycentric;

            if( !face.isUvInside( uv, barycentric, Vertex::Lightmap ) ) {
                continue;
            }

            bakeLumel( lumel );
        }
    }
}

// ** Baker::bake
RelightStatus Baker::bake( void )
{
    for( int i = 0, n = m_scene->instanceCount(); i < n; i++ ) {
        bakeInstance( m_scene->instance( i ) );
    }

    return RelightSuccess;
}

// ** Baker::bakeLumel
void Baker::bakeLumel( Lumel& lumel )
{

}

} // namespace bake

} // namespace relight
