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

#ifndef __Relight_Baker_H__
#define __Relight_Baker_H__

#include "../Relight.h"

namespace relight {

namespace bake {

    //! Baker class is a base class for all light bakers (direct, indirect, ambient occlusion, etc)
    class Baker {
    public:

                                //! Constructs a new Baker instance.
                                Baker( const Scene* scene, Progress* progress );

        virtual                 ~Baker( void );

        //! Bakes a light data to an instance lightmap.
        virtual RelightStatus   bakeMesh( const Mesh* mesh );

        //! Bakes all scene light data to a set of textures.
        virtual RelightStatus   bake( void );

    protected:

        //! Bakes a data to a given lumel.
        virtual void            bakeLumel( Lumel& lumel );

        //! Bakes a data to lumels corresponding to this face.
        void                    bakeFace( const Mesh* mesh, Index index );

    protected:

        //! Baking progress
        Progress*               m_progress;

        //! Scene to bake.
        const Scene*            m_scene;
    };

} // namespace bake

} // namespacce relight

#endif  /*  !defined( __Relight_Baker_H__ ) */
