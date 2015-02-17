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

#ifndef __Relight_Scene_Material_H__
#define __Relight_Scene_Material_H__

#include "../Relight.h"
#include "../Vector.h"

namespace relight {

    /*!
     Materials are used to represent a surface light reflection settings.
     */
    class Material {
    public:


                        //! Constructs a Material instance.
                        /*!
                         \param color Surface diffuse color.
                         */
                        Material( const Color& color );
        virtual         ~Material( void ) {}

        //! Returns a material diffuse color.
        const Color&    color( void ) const;

        //! Sets a diffuse color.
        void            setColor( const Color& value );

        //! Returns a surface color at a given UV coordinates.
        virtual Color   colorAt( const Uv& uv ) const;

    private:

        //! Diffuse color.
        Color           m_color;
    };

    /*!
     Material texture image.
     */
    class Texture {
    public:

        //! Returns texture width.
        int                 width( void ) const;

        //! Returns texture height.
        int                 height( void ) const;

        //! Returns a texture pixel buffer.
        const PixelBuffer&  pixels( void ) const;

        //! Returns a texture color at a given UV coordinates.
        const Color&        colorAt( const Uv& uv ) const;

        //! Creates a new Texture instance.
        /*!
         \param width Texture width.
         \param height Texture height.
         \param pixels Texture RGB image data.
         */
        static Texture*     create( int width, int height, const PixelBuffer& pixels );

        //! Creates a new Texture instance from a TGA file.
        /*!
         \param fileName Texture file name.
         */
        static Texture*     createFromFile( const char* fileName );

    private:

                            //! Constructs a Texture instance.
                            Texture( int width, int height, const PixelBuffer& pixels );

    private:

        //! Texture width.
        int                 m_width;

        //! Texture height.
        int                 m_height;

        //! Texture pixels.
        PixelBuffer         m_pixels;
    };

    /*!
     Textured material.
     */
    class TexturedMaterial : public Material {
    public:

                        //! Constructs a TexturedMaterial instance.
                        TexturedMaterial( const Texture* texture, const Color& color );

        //! Returns a material diffuse color multiplied by a texture color.
        virtual Color   colorAt( const Uv& uv ) const;

    private:

        //! Material texture.
        const Texture*  m_texture;
    };

} // namespace relight

#endif  /*  !defined(__Relight_Scene_Material_H__) */