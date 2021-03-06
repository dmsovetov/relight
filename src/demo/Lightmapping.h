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

#ifndef __Relight_Demo_Lightmapping__
#define __Relight_Demo_Lightmapping__

#include "RelightDemo.h"
#include <uscene/src/uScene.h>

// ** struct SceneMesh
struct SceneMesh {
    relight::Mesh*          m_mesh;
    relight::Material*      m_material;
	Rgba					m_diffuseColor;
	Rgba					m_tintColor;
    
    renderer::Texture*      m_diffuse;
    renderer::IndexBuffer*  m_indexBuffer;
    renderer::VertexBuffer* m_vertexBuffer;
};

// ** struct SceneMeshInstance
struct SceneMeshInstance {
    const SceneMesh*            m_mesh;

    Matrix4						m_transform;
    renderer::Texture2D*        m_lightmap;
    relight::Lightmap*          m_lm;
    relight::Photonmap*         m_pm;
    bool                        m_dirty;
};

//! Relight background worker.
class ThreadWorker : public relight::Worker {
public:

                    //! Constructs a ThreadWorker instance.
                    ThreadWorker( void );

    //! Pushes a new job to this worker.
    virtual void    push( relight::Job* job, relight::JobData* data );

    //! Waits for completion of this worker.
    virtual void    wait( void );

    virtual void    notify( const relight::Mesh* instance, int step, int stepCount );

private:

    //! Thread worker callback.
    static void     worker( void* userData );

private:

    //! Thread worker data
    struct ThreadData {
        relight::JobData*   m_data;
        relight::Progress*  m_progress;
    };

    //! Thread handle.
	thread::Thread*	m_thread;
};

// ** class Lightmapping
class Lightmapping : public platform::WindowDelegate {
public:

                        Lightmapping( renderer::Hal* hal );

    virtual void        handleUpdate( platform::Window* window );

private:

    SceneMesh*          findMesh( const uscene::Asset* asset, const uscene::Renderer* renderer, bool solid );
    relight::Texture*   findTexture( const uscene::Asset* asset, bool solid );
    Matrix4				affineTransform( const uscene::Transform* transform );
	void				extractTransform( const uscene::Transform* transform, Vec3& position, Quat& rotation, Vec3& scale ) const;
    void                createBuffersFromMesh( SceneMesh& mesh );
    renderer::Texture*  createTexture( const relight::Texture* texture );
    renderer::Texture2D* loadLightmapFromFile( const std::string& fileName );

private:

    typedef std::map<const uscene::Asset*,      SceneMesh>           Meshes;
    typedef std::map<const relight::Texture*,   renderer::Texture*>  Textures;
    typedef std::map<const uscene::Asset*,      relight::Texture*>   RelightTextures;

    renderer::Hal*                  m_hal;
    renderer::VertexDeclaration*    m_meshVertexLayout;

	scene::ScenePtr					m_simpleScene;
	scene::Renderer*				m_renderer;

	Matrix4							m_matrixView;
	Matrix4							m_matrixProj;

    uscene::Assets*                 m_assets;
    uscene::Scene*                  m_scene;

    relight::Relight*               m_relight;
    relight::Scene*                 m_relightScene;
    relight::Worker*                m_rootWorker;
    relight::Workers                m_relightWorkers;

    Meshes                          m_meshes;
    Textures                        m_textures;
    RelightTextures                 m_relightTextures;
};

#endif /* defined(__Relight_Demo_Lightmapping__) */
