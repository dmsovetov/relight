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

#include "GenerateUv.h"
#include "FbxLoader.h"

#ifdef WIN32
	#include <Windows.h>
	#include <gl/GL.h>
#endif

// ** GenerateUv::GenerateUv
GenerateUv::GenerateUv( renderer::Hal* hal ) : m_hal( hal )
{
    m_meshVertexLayout = m_hal->createVertexDeclaration( "P3:N:T0:T1", sizeof( SceneVertex ) );

	m_simpleScene = scene::Scene::create();
	m_simpleScene->setRenderer( new scene::Renderer( hal ) );

	scene::MeshPtr mesh = createMeshFromFile( "Assets/models/dungeon/Column7.fbx" );
//	scene::MeshPtr mesh = createMeshFromFile( "Assets/models/Barrel01.fbx" );

	scene::SceneObjectPtr object = scene::SceneObject::create(); 
	object->attach<scene::Transform>();
	object->attach<scene::MeshRenderer>( mesh );

	m_simpleScene->add( object );
}

// ** GenerateUv::handleUpdate
void GenerateUv::handleUpdate( platform::Window* window )
{
    if( !m_hal->clear() ) {
        return;
    }

	const bool kShowMesh  = true;
    const bool kShowSeams = true;
    const bool kShowFaces = true;

	m_simpleScene->update( 0.1f );
	if( kShowMesh ) {
		m_simpleScene->render( f32( window->width() ) / window->height() );
	}

    SceneTriMesh::Dcel dcel = m_loadedTriMesh->dcel();

    glMatrixMode( GL_PROJECTION );
    glLoadMatrixf( m_simpleScene->camera()->proj( f32( window->width() ) / window->height() ).m );

    glMatrixMode( GL_MODELVIEW );
    glLoadMatrixf( m_simpleScene->camera()->view().m );

    glDepthFunc( GL_LEQUAL );
    glLineWidth( 3.0f );
    for( int i = 0; i < dcel.edgeCount(); i++ )
    {
		if( !kShowMesh ) {
			continue;
		}

        const SceneTriMesh::Dcel::Edge* edge = dcel.edge( i );

        glBegin( GL_LINES );
            if( edge->isBoundary() )
                glColor3f( 0, 1, 0 );
            else
                glColor3f( 1, 0, 0 );
            glVertex3fv( &m_loadedVertices[edge->m_vertex].position.x );
            glVertex3fv( &m_loadedVertices[edge->m_next->m_vertex].position.x );
        glEnd();
    }

	int   w = m_generator.width();
	int   h = m_generator.height();

    glColor3f( 1, 1, 1 );
    glBegin( GL_LINE_STRIP );
        glVertex3f( 0, 0, 0 );
        glVertex3f( w, 0, 0 );
        glVertex3f( w, 0, h );
        glVertex3f( 0, 0, h );
        glVertex3f( 0, 0, 0 );
    glEnd();

    for( int i = 0; i < m_generator.packer().rectCount(); i++ ) {
        const SceneRectanglePacker::Rect& rect = m_generator.packer().rect( i );

        glColor3f( 0, 1, 1 );
        glBegin( GL_LINE_STRIP );
            glVertex3f( rect.x,              i * 0.1 * 0.0, rect.y );
            glVertex3f( rect.x + rect.width, i * 0.1 * 0.0, rect.y );
            glVertex3f( rect.x + rect.width, i * 0.1 * 0.0, rect.y + rect.height );
            glVertex3f( rect.x,              i * 0.1 * 0.0, rect.y + rect.height );
            glVertex3f( rect.x,              i * 0.1 * 0.0, rect.y );
        glEnd();
    }

    m_hal->present();
}

// ** GenerateUv::createMeshFromFile
scene::MeshPtr GenerateUv::createMeshFromFile( CString fileName )
{
    fbx::FbxLoader loader;

    if( !loader.load( fileName ) ) {
        return scene::MeshPtr();
    }

	// ** Convert loaded mesh to scene mesh
	for( int i = 0, n = loader.vertexBuffer().size(); i < n; i++ ) {
        const fbx::Vertex& rv = loader.vertexBuffer()[i];
        SceneVertex        sv;

        sv.position.x  = rv.position[0]; sv.position.y  = rv.position[1]; sv.position.z  = rv.position[2];
        sv.normal.x    = rv.normal[0];   sv.normal.y    = rv.normal[1];   sv.normal.z    = rv.normal[2];

        sv.uv[0].x	   = rv.uv[0][0];    sv.uv[0].y		= rv.uv[0][1];
        sv.uv[1].x	   = rv.uv[1][0];    sv.uv[1].y		= rv.uv[1][1];

	//	sv.position.x *= 0.01f;
	//	sv.position.y *= 0.01f;
	//	sv.position.z *= 0.01f;

		m_loadedVertices.push_back( sv );
	}

	for( int i = 0, n = loader.indexBuffer().size(); i < n; i++ ) {
		m_loadedIndices.push_back( loader.indexBuffer()[i] );
	}

	// ** Process the mesh.
    m_loadedTriMesh = new SceneTriMesh( m_loadedVertices, m_loadedIndices );
	SceneMeshIndexer sceneMeshIndexer;

	m_generator.generate( *m_loadedTriMesh, m_loadedVertices, m_loadedIndices );
   
	// ** Create buffers
    renderer::VertexBuffer* vertexBuffer = m_hal->createVertexBuffer( m_meshVertexLayout, m_loadedVertices.size(), false );
    renderer::IndexBuffer*	indexBuffer  = m_hal->createIndexBuffer( m_loadedIndices.size(), false );

	// ** Upload data
    memcpy( vertexBuffer->lock(), &m_loadedVertices[0], m_loadedVertices.size() * sizeof( m_loadedVertices[0] ) );
    vertexBuffer->unlock();

    memcpy( indexBuffer->lock(), &m_loadedIndices[0], m_loadedIndices.size() * sizeof( m_loadedIndices[0] ) );
    indexBuffer->unlock();

	// ** Create mesh
	scene::MeshPtr mesh = scene::Mesh::create();
	mesh->addChunk( vertexBuffer, indexBuffer );

	return mesh;
}

// ** UvGenerator::generate
void UvGenerator::generate( const SceneTriMesh& input, SceneTriMesh::Vertices& vertices, SceneTriMesh::Indices& indices )
{
	m_inputIndices = input.indices();
	m_inputVertices = input.vertices();

	// ** Flatten mesh
	SceneTriMesh		 mesh( m_inputVertices, m_inputIndices );
	SceneMeshIndexer	 sceneMeshIndexer;

	ChartBuilder::Result charts = mesh.charts( ChartBuilder() );

    for( int i = 0; i < charts.m_charts.size(); i++ )
    {
        const Chart& chart = charts.m_charts[i];

        float minx = FLT_MAX, maxx = -FLT_MAX;
        float miny = FLT_MAX, maxy = -FLT_MAX;

        for( int j = 0; j < chart.faceCount(); j++ ) {
            Face face = chart.face( j );

            Vec2 v[3];
            face.flatten( chart.normal().ordinal(), v[0], v[1], v[2] );

            for( int k = 0; k < 3; k++ ) {
                minx = min( minx, v[k].x );
                maxx = max( maxx, v[k].x );
                miny = min( miny, v[k].y );
                maxy = max( maxy, v[k].y );
            }
        }

        float w = maxx - minx;
        float h = maxy - miny;

        for( int j = 0; j < chart.faceCount(); j++ ) {
            Face face = chart.face( j );

            Vec2 v[3];
            face.flatten( chart.normal().ordinal(), v[0], v[1], v[2] );

            for( int k = 0; k < 3; k++ ) {
                SceneVertex vtx = face.vertex( k );

                if( w > h ) {
                    vtx.uv[0] = Vec2( v[k].x - minx, v[k].y - miny );
                } else {
                    vtx.uv[0] = Vec2( v[k].y - miny, v[k].x - minx );
                }
                sceneMeshIndexer += vtx;
            }
        }
    }

    m_inputVertices = sceneMeshIndexer.vertexBuffer();
    m_inputIndices  = sceneMeshIndexer.indexBuffer();

	charts = mesh.charts( AngleChartBuilder<SceneTriMesh>() );

    for( int i = 0; i < charts.m_charts.size(); i++ )
    {
		const Chart& chart = charts.m_charts[i];
		Vec2	 min, max;

		chart.calculateUvRect( min, max );

		float w = max.x - min.x;
        float h = max.y - min.y;

		m_packer.add( max( w, h ), min( w, h ) );
	}

    int w = 1;
    int h = 1;
    bool expandWidth = true;

    while( !m_packer.place( w, h ) ) {
        if( expandWidth ) {
            w += 1;
            expandWidth = false;
        } else {
            h += 1;
            expandWidth = true;
        }
    }

    m_width = w;
    m_height = h;

    sceneMeshIndexer = SceneMeshIndexer();

    for( int i = 0; i < charts.m_charts.size(); i++ )
    {
        const Chart& chart = charts.m_charts[i];
        const SceneRectanglePacker::Rect& rect = m_packer.rect( i );

        Vec3 chartColor = Vec3::randomDirection();

        for( int j = 0; j < chart.faceCount(); j++ ) {
            Face face = chart.face( j );

            for( int k = 0; k < 3; k++ ) {
                SceneVertex vtx = face.vertex( k );

             //   vtx.normal = chartColor * 0.5f + 0.5f;
                vtx.position = Vec3( rect.x + vtx.uv[0].x, 0, rect.y + vtx.uv[0].y );
			//	vtx.position = Vec3( vtx.uv[0].x, i * 0.1, vtx.uv[0].y );
                sceneMeshIndexer += vtx;
            }
        }
    }

    vertices = sceneMeshIndexer.vertexBuffer();
    indices  = sceneMeshIndexer.indexBuffer();
}