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

#include "Lightmapping.h"
#include "FbxLoader.h"

#ifdef WIN32
	#include <Windows.h>
	#include <gl/GL.h>
#endif

#define GLSL( ... ) #__VA_ARGS__

// ** Lightmap size
const int k_LightmapMaxSize = 128;
const int k_LightmapMinSize = 16;

// ** Background workers
const int k_Workers      = 8;

// ** Debug flags
const bool k_DebugShowStatic = false;

// ** Blue sky color
const relight::Rgb k_BlueSky        = relight::Rgb( 0.52734375f, 0.8046875f, 0.91796875f );

// ** Black sky
const relight::Rgb k_BlackSky       = relight::Rgb( 0, 0, 0 );

// ** Sun light color
const relight::Rgb k_SunColor           = relight::Rgb( 0.75f, 0.74609375f, 0.67578125f );
const float        k_SunColorIntensity  = 1.5f;

// ** Dark blue light
const relight::Rgb  k_DarkLightColor        = relight::Rgb( 0.0737915039f, 0.166036054f, 0.395522416f );
const float         k_DarkLightIntensity    = 1.5f;

// ** Scene ambient color
const relight::Rgb k_AmbientColor = relight::Rgb( 0.34f, 0.34f, 0.34f );

// ** Indirect light settings
const relight::IndirectLightSettings k_IndirectLight = relight::IndirectLightSettings::fast( /*k_BlueSky*/k_BlackSky, k_AmbientColor );

// ** Lightmapping::Lightmapping
Lightmapping::Lightmapping( renderer::Hal* hal ) : m_hal( hal )
{
    using namespace uscene;

	// ** Create the vertex layout
    m_meshVertexLayout = m_hal->createVertexDeclaration( "P3:N:T0:T1", sizeof( SceneVertex ) );

	// ** Create the ambient shader
	m_shaderAmbient = m_hal->createShader(
		GLSL(
			uniform mat4 u_mvp;
			varying vec2 v_tex0;
			varying vec2 v_tex1;

			void main()
			{
				v_tex0 = gl_MultiTexCoord0.xy;
				v_tex1 = gl_MultiTexCoord1.xy;
				// Transforming The Vertex
				gl_Position = u_mvp * gl_Vertex;
			} ),
		GLSL(
			uniform sampler2D u_diffuse;
			uniform sampler2D u_lightmap;
			uniform vec3	  u_diffuseColor;
			varying vec2	  v_tex0;
			varying vec2	  v_tex1;

			void main()
			{
				// Setting Each Pixel To Red
				gl_FragColor = texture2D( u_diffuse, v_tex0 ) * texture2D( u_lightmap, v_tex1 ) * u_diffuseColor;
			} ) );

    m_assets = Assets::parse( "Assets/assets" );
//    m_scene  = Scene::parse( m_assets, "Assets/Crypt/Scenes/Simple.scene" );
//    m_scene  = Scene::parse( m_assets, "Assets/Crypt/Demo/NoTerrain.scene" );
	m_scene = Scene::parse( m_assets, "Assets/Demo/Demo7.scene" );
//	m_scene = Scene::parse( m_assets, "Assets/Simple.scene" );

    if( !m_scene ) {
        printf( "Failed to create scene\n" );
        return;
    }

    m_relight      = relight::Relight::create();
    m_relightScene = m_relight->createScene();
    m_relightScene->begin();

    int totalLightmapPixels = 0;

    float maxArea = -FLT_MAX;
    float scale = 170.0f;

    // ** Add objects
    for( int i = 0; i < m_scene->sceneObjectCount(); i++ ) {
        SceneObject*        sceneObject = m_scene->sceneObject( i );
        Transform*          transform   = sceneObject->transform();
        Mesh*               mesh        = sceneObject->mesh();
        Renderer*           renderer    = sceneObject->renderer();
		Light*				light		= sceneObject->light();

		if( light ) {
			m_relightScene->addLight( relight::Light::createPointLight( transform->position(), light->range(), light->color(), light->intensity() ) );
			continue;
		}

        if( !mesh ) {
            continue;
        }

        bool addToRelight = sceneObject->isStatic();
        bool isSolid      = false;

        if( renderer->materials()[0]->shader() == "diffuse" ) {
            m_solidRenderList.push_back( sceneObject );
            isSolid = true;
        }
        else if( renderer->materials()[0]->shader() == "additive" ) {
            m_additiveRenderList.push_back( sceneObject );
            addToRelight = false;
        }
        else {
            m_transparentRenderList.push_back( sceneObject );
        }

        SceneMeshInstance* instance = new SceneMeshInstance;

        sceneObject->setUserData( instance );

        instance->m_transform = affineTransform( transform );
        instance->m_lightmap  = NULL;
        instance->m_mesh      = findMesh( mesh->asset(), renderer, isSolid );
        instance->m_dirty     = false;

        if( !addToRelight ) {
            continue;
        }

        float area = instance->m_mesh->m_mesh->area();
		maxArea	   = math::max2( maxArea, instance->m_mesh->m_mesh->area() );
        int   size = math::nextPowerOf2( ceil( k_LightmapMinSize + (k_LightmapMaxSize - k_LightmapMinSize) * (area / 170.0f) ) );

        totalLightmapPixels += size * size;

        instance->m_lm = m_relight->createLightmap( size, size );
        instance->m_pm = m_relight->createPhotonmap( size, size );

        relight::Mesh* m = m_relightScene->addMesh( instance->m_mesh->m_mesh, instance->m_transform, instance->m_mesh->m_material );
        m->setUserData( instance );

        instance->m_lm->addMesh( m );
        instance->m_pm->addMesh( m );
    }
    m_relightScene->end();

    printf( "%d instances added to relight scene, maximum mesh area %2.4f (%d lightmap pixels used)\n", m_relightScene->meshCount(), maxArea, totalLightmapPixels );

    printf( "Emitting photons...\n" );
    m_relight->emitPhotons( m_relightScene, k_IndirectLight );
    printf( "Done!\n" );

    struct Bake : public relight::Job {
    public:

        virtual void execute( relight::JobData* data ) {
            data->m_relight->bakeDirectLight( data->m_scene, data->m_mesh, data->m_worker, data->m_iterator );
            data->m_relight->bakeIndirectLight( data->m_scene, data->m_mesh, data->m_worker, k_IndirectLight, data->m_iterator );
        }
    };

    typedef ThreadWorker LmWorker;

    // ** Bake scene
    m_rootWorker = new LmWorker;
    for( int i = 0; i < k_Workers; i++ ) {
        m_relightWorkers.push_back( new LmWorker );
    }
    m_relight->bake( m_relightScene, new Bake, m_rootWorker, m_relightWorkers );
}

// ** Lightmapping::affineTransform
relight::Matrix4 Lightmapping::affineTransform( const uscene::Transform *transform )
{
    if( !transform ) {
        return relight::Matrix4();
    }

    relight::Vec3 position = relight::Vec3( transform->position()[0], transform->position()[1], transform->position()[2] );
    relight::Vec3 scale    = relight::Vec3( transform->scale()[0], transform->scale()[1], transform->scale()[2] );
    relight::Quat rotation = relight::Quat( transform->rotation()[0], transform->rotation()[1], transform->rotation()[2], transform->rotation()[3] );

    return affineTransform( transform->parent() ) * relight::Matrix4::affineTransform( position, rotation, scale ) * relight::Matrix4::scale( -1, 1, 1 );
}

// ** Lightmapping::createTexture
renderer::Texture* Lightmapping::createTexture( const relight::Texture* texture )
{
    if( m_textures.count( texture ) ) {
        return m_textures[texture];
    }

    renderer::Texture2D* texture2d = m_hal->createTexture2D( texture->width(), texture->height(), texture->channels() == 3 ? renderer::PixelRgb8 : renderer::PixelRgba8 );
    texture2d->setData( 0, texture->pixels() );

    m_textures[texture] = texture2d;

    return texture2d;
}

// ** Lightmapping::findTexture
relight::Texture* Lightmapping::findTexture( const uscene::Asset* asset, bool solid )
{
    if( m_relightTextures.count( asset ) ) {
        return m_relightTextures[asset];
    }

    relight::Texture* texture = relight::Texture::createFromFile( asset->fileName().c_str() );

    if( solid && texture->channels() == 4 ) {
        texture->convertToRgb();
    }

    if( texture ) {
        m_relightTextures[asset] = texture;
    }

    return texture;
}

// ** Lightmapping::findMesh
SceneMesh* Lightmapping::findMesh( const uscene::Asset* asset, const uscene::Renderer* renderer, bool solid )
{
    if( m_meshes.count( asset ) ) {
        return &m_meshes[asset];
    }

    const uscene::MeshAsset* meshAsset = static_cast<const uscene::MeshAsset*>( asset );

    fbx::FbxLoader loader;
    if( !loader.load( asset->fileName().c_str() ) ) {
        return NULL;
    }

    relight::VertexBuffer vertices;
    relight::IndexBuffer  indices;
    relight::Matrix4      transform = relight::Matrix4::scale( meshAsset->scale(), meshAsset->scale(), meshAsset->scale() );

    for( int i = 0; i < loader.vertexBuffer().size(); i++ ) {
        const fbx::Vertex& fbxVertex = loader.vertexBuffer()[i];
        relight::Vertex    v;

        v.m_position = relight::Vec3( fbxVertex.position[0], fbxVertex.position[1], fbxVertex.position[2] );
        v.m_normal   = relight::Vec3( fbxVertex.normal[0], fbxVertex.normal[1], fbxVertex.normal[2] );

        for( int j = 0; j < relight::Vertex::TotalUvLayers; j++ ) {
            v.m_uv[j] = relight::Uv( fbxVertex.uv[j][0], fbxVertex.uv[j][1] );
        }

        v.m_position = transform * v.m_position;
        v.m_normal   = transform * v.m_normal;
        v.m_normal.normalize();

        vertices.push_back( v );
    }

    for( int i = 0; i < loader.indexBuffer().size(); i++ ) {
        indices.push_back( loader.indexBuffer()[i] );
    }

    SceneMesh sceneMesh;

    // ** Create texture
    if( renderer->materials().size() ) {
        uscene::Material* material  = renderer->materials()[0];
        const float*      diffuse   = material->color( uscene::Material::Diffuse );
        relight::Texture* texture   = findTexture( material->texture( uscene::Material::Diffuse ), solid );
        sceneMesh.m_material        = texture ? new relight::TexturedMaterial( texture, diffuse ) : new relight::Material( diffuse );
        sceneMesh.m_diffuse         = createTexture( texture );
    }

    // ** Create Relight mesh
    sceneMesh.m_mesh = relight::Mesh::create();
    sceneMesh.m_mesh->addFaces( vertices, indices );

    // ** Create HAL vertex & index buffers
    createBuffersFromMesh( sceneMesh );

    m_meshes[asset] = sceneMesh;

    return &m_meshes[asset];
}

// ** Lightmapping::createBuffersFromMesh
void Lightmapping::createBuffersFromMesh( SceneMesh& mesh )
{
    mesh.m_vertexBuffer = m_hal->createVertexBuffer( m_meshVertexLayout, mesh.m_mesh->vertexCount(), false );
    SceneVertex* vertices = ( SceneVertex* )mesh.m_vertexBuffer->lock();
    for( int i = 0; i < mesh.m_mesh->vertexCount(); i++ ) {
        const relight::Vertex& rv = mesh.m_mesh->vertex( i );
        SceneVertex&           sv = vertices[i];

        sv.x  = rv.m_position.x; sv.y  = rv.m_position.y; sv.z  = rv.m_position.z;
        sv.nx = rv.m_normal.x;   sv.ny = rv.m_normal.y;   sv.nz = rv.m_normal.z;
        sv.u0 = rv.m_uv[0].x;    sv.v0 = rv.m_uv[0].y;
        sv.u1 = rv.m_uv[1].x;    sv.v1 = rv.m_uv[1].y;
    }
    mesh.m_vertexBuffer->unlock();

    mesh.m_indexBuffer = m_hal->createIndexBuffer( mesh.m_mesh->indexCount(), false );
    memcpy( mesh.m_indexBuffer->lock(), mesh.m_mesh->indexBuffer(), mesh.m_mesh->indexCount() * sizeof( relight::Index ) );
    mesh.m_indexBuffer->unlock();
}

// ** Lightmapping::handleUpdate
void Lightmapping::handleUpdate( platform::Window* window )
{
    if( !m_scene ) {
        return;
    }
    
    using namespace relight;

//	uscene::Transform* camera1		= const_cast<uscene::SceneObject*>( m_scene->findSceneObject( "Camera1" ) )->transform();
//	uscene::Transform* camera2		= const_cast<uscene::SceneObject*>( m_scene->findSceneObject( "Camera2" ) )->transform();
//	uscene::Transform* mainCamera	= const_cast<uscene::SceneObject*>( m_scene->findSceneObject( "Main Camera" ) )->transform();

//	m_matrixView = Matrix4::lookAt( mainCamera->position(), Vec3( 0, 0, 0 ), Vec3( 0, 1, 0 ) );
	m_matrixView = Matrix4::lookAt( Vec3( 15, 15, 15 ), Vec3( 0, 0, 0 ), Vec3( 0, 1, 0 ) );
	m_matrixProj = Matrix4::perspective( 60.0f, window->width() / float( window->height() ), 0.01f, 1000.0f );

    uscene::SceneSettings*	settings = m_scene->settings();
    const float*			ambient  = settings->ambient();
    const float*			fog      = settings->fogColor();
    Rgba					ambientColor( ambient[0], ambient[1], ambient[2], 1.0f );
    Rgba					fogColor( fog[0], fog[1], fog[2], 1.0f );

    if( !m_hal->clear( fogColor ) ) {
        return;
    }

    m_hal->setFog( renderer::FogExp2, settings->fogDensity() * 7, fogColor, 0, 300 );

    {
        renderObjects( m_solidRenderList );
    }

    {
        m_hal->setAlphaTest( renderer::Greater, 0.5f );
        renderObjects( m_transparentRenderList );
        m_hal->setAlphaTest( renderer::CompareDisabled );
    }

    {
        m_hal->setDepthTest( false, renderer::Less );
        m_hal->setBlendFactors( renderer::BlendOne, renderer::BlendOne );

        renderObjects( m_additiveRenderList );

        m_hal->setDepthTest( true, renderer::Less );
        m_hal->setBlendFactors( renderer::BlendDisabled, renderer::BlendDisabled );
    }

    m_hal->present();
}

// ** Lightmapping::renderObjects
void Lightmapping::renderObjects( const uscene::SceneObjectArray& objects )
{
	using namespace relight;

	m_hal->setShader( m_shaderAmbient );
    for( int i = 0; i < objects.size(); i++ ) {
        uscene::SceneObject* object   = objects[i];
        SceneMeshInstance*   instance = reinterpret_cast<SceneMeshInstance*>( object->userData() );

        if( !instance ) {
            continue;
        }

		Matrix4    mvp   = m_matrixProj * Matrix4::scale( -1, 1, 1 ) * m_matrixView * instance->m_transform;
		const Rgb& color = instance->m_mesh->m_material->color();

		m_shaderAmbient->setVec3( m_shaderAmbient->findUniformLocation( "u_diffuseColor" ), Vec3( color.r, color.g, color.b ) );
		m_shaderAmbient->setMatrix( m_shaderAmbient->findUniformLocation( "u_mvp" ), mvp );
		m_shaderAmbient->setInt( m_shaderAmbient->findUniformLocation( "u_diffuse" ), 0 );
		m_shaderAmbient->setInt( m_shaderAmbient->findUniformLocation( "u_lightmap" ), 1 );

        const relight::Rgb& diffuse = instance->m_mesh->m_material->color();

		m_hal->setTexture( 0, instance->m_mesh->m_diffuse );
		m_hal->setTexture( 1, instance->m_lightmap );

        m_hal->setVertexBuffer( instance->m_mesh->m_vertexBuffer );
        m_hal->renderIndexed( renderer::PrimTriangles, instance->m_mesh->m_indexBuffer, 0, instance->m_mesh->m_indexBuffer->size() );

        if( instance->m_dirty ) {
            relight::Lightmap* lm = instance->m_lm;

			if( instance->m_lightmap == NULL ) {
				instance->m_lightmap = m_hal->createTexture2D( lm->width(), lm->height(), renderer::PixelRgb32F );
			}

            float* pixels = lm->toRgb32F();
            instance->m_lightmap->setData( 0, pixels );
            delete[]pixels;

            instance->m_dirty = false;
        }
    }

    m_hal->setVertexBuffer( NULL );
}

// ** ThreadWorker::worker
void ThreadWorker::worker( void* userData )
{
    relight::JobData* data = reinterpret_cast<relight::JobData*>( userData );
    data->m_job->execute( data );
    delete data;
}

// ** ThreadWorker::ThreadWorker
ThreadWorker::ThreadWorker( void )
{
}

// ** ThreadWorker::push
void ThreadWorker::push( relight::Job* job, relight::JobData* data )
{
    data->m_worker = this;
	m_thread = thread::Thread::create();
	m_thread->start( dcThisMethod( ThreadWorker::worker ), data );
}

// ** ThreadWorker::wait
void ThreadWorker::wait( void )
{
	m_thread->wait();
}

// ** ThreadWorker::notify
void ThreadWorker::notify( const relight::Mesh* instance, int step, int stepCount )
{
    reinterpret_cast<SceneMeshInstance*>( instance->userData() )->m_dirty = true;
}