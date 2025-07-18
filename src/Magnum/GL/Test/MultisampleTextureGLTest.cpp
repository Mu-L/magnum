/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Compare/Container.h>

#include "Magnum/GL/Context.h"
#include "Magnum/GL/Extensions.h"
#include "Magnum/GL/ImageFormat.h"
#include "Magnum/GL/MultisampleTexture.h"
#include "Magnum/GL/OpenGLTester.h"
#include "Magnum/GL/TextureFormat.h"
#include "Magnum/Math/Vector3.h"
#include "Magnum/Math/Functions.h"

namespace Magnum { namespace GL { namespace Test { namespace {

struct MultisampleTextureGLTest: OpenGLTester {
    explicit MultisampleTextureGLTest();

    void construct2D();
    void construct2DArray();
    void constructMove();

    void wrap2D();
    void wrap2DArray();
    void wrapCreateIfNotAlready2D();
    void wrapCreateIfNotAlready2DArray();

    void label2D();
    void label2DArray();

    void bind2D();
    void bind2DArray();

    void bindImage2D();
    void bindImage2DArray();

    void storage2D();
    void storage2DArray();

    void view2D();
    void view2DOnArray();
    void view2DArray();
    void view2DArrayOnNonArray();

    void invalidateImage2D();
    void invalidateImage2DArray();

    void invalidateSubImage2D();
    void invalidateSubImage2DArray();
};

MultisampleTextureGLTest::MultisampleTextureGLTest() {
    addTests({&MultisampleTextureGLTest::construct2D,
              &MultisampleTextureGLTest::construct2DArray,

              &MultisampleTextureGLTest::constructMove,

              &MultisampleTextureGLTest::wrap2D,
              &MultisampleTextureGLTest::wrap2DArray,
              &MultisampleTextureGLTest::wrapCreateIfNotAlready2D,
              &MultisampleTextureGLTest::wrapCreateIfNotAlready2DArray,

              &MultisampleTextureGLTest::label2D,
              &MultisampleTextureGLTest::label2DArray,

              &MultisampleTextureGLTest::bind2D,
              &MultisampleTextureGLTest::bind2DArray,

              &MultisampleTextureGLTest::bindImage2D,
              &MultisampleTextureGLTest::bindImage2DArray,

              &MultisampleTextureGLTest::storage2D,
              &MultisampleTextureGLTest::storage2DArray,

              &MultisampleTextureGLTest::view2D,
              &MultisampleTextureGLTest::view2DOnArray,
              &MultisampleTextureGLTest::view2DArray,
              &MultisampleTextureGLTest::view2DArrayOnNonArray,

              &MultisampleTextureGLTest::invalidateImage2D,
              &MultisampleTextureGLTest::invalidateImage2DArray,

              &MultisampleTextureGLTest::invalidateSubImage2D,
              &MultisampleTextureGLTest::invalidateSubImage2DArray});
}

using namespace Containers::Literals;

void MultisampleTextureGLTest::construct2D() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isVersionSupported(Version::GLES310))
        CORRADE_SKIP("OpenGL ES 3.1 is not supported.");
    #endif

    {
        MultisampleTexture2D texture;

        MAGNUM_VERIFY_NO_GL_ERROR();
        CORRADE_VERIFY(texture.id() > 0);
    }

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void MultisampleTextureGLTest::construct2DArray() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isExtensionSupported<Extensions::OES::texture_storage_multisample_2d_array>())
        CORRADE_SKIP(Extensions::OES::texture_storage_multisample_2d_array::string() << "is not supported.");
    #endif

    {
        MultisampleTexture2DArray texture;

        MAGNUM_VERIFY_NO_GL_ERROR();
        CORRADE_VERIFY(texture.id() > 0);
    }

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void MultisampleTextureGLTest::constructMove() {
    /* Move constructor tested in AbstractTexture, here we just verify there
       are no extra members that would need to be taken care of */
    CORRADE_COMPARE(sizeof(MultisampleTexture2D), sizeof(AbstractTexture));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<MultisampleTexture2D>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<MultisampleTexture2D>::value);
}

void MultisampleTextureGLTest::wrap2D() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isVersionSupported(Version::GLES310))
        CORRADE_SKIP("OpenGL ES 3.1 is not supported.");
    #endif

    GLuint id;
    glGenTextures(1, &id);

    /* Releasing won't delete anything */
    {
        auto texture = MultisampleTexture2D::wrap(id, ObjectFlag::DeleteOnDestruction);
        CORRADE_COMPARE(texture.release(), id);
    }

    /* ...so we can wrap it again */
    MultisampleTexture2D::wrap(id);
    glDeleteTextures(1, &id);
}

void MultisampleTextureGLTest::wrap2DArray() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isExtensionSupported<Extensions::OES::texture_storage_multisample_2d_array>())
        CORRADE_SKIP(Extensions::OES::texture_storage_multisample_2d_array::string() << "is not supported.");
    #endif

    GLuint id;
    glGenTextures(1, &id);

    /* Releasing won't delete anything */
    {
        auto texture = MultisampleTexture2DArray::wrap(id, ObjectFlag::DeleteOnDestruction);
        CORRADE_COMPARE(texture.release(), id);
    }

    /* ...so we can wrap it again */
    MultisampleTexture2DArray::wrap(id);
    glDeleteTextures(1, &id);
}

void MultisampleTextureGLTest::wrapCreateIfNotAlready2D() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isVersionSupported(Version::GLES310))
        CORRADE_SKIP("OpenGL ES 3.1 is not supported.");
    #endif

    /* Make an object and ensure it's created */
    MultisampleTexture2D texture;
    texture.bind(0);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(texture.flags(), ObjectFlag::Created|ObjectFlag::DeleteOnDestruction);

    /* Wrap into another object without ObjectFlag::Created being set, which is
       a common usage pattern to make non-owning references. Then calling an
       API that internally does createIfNotAlready() shouldn't assert just
       because Created isn't set but the object is bound, instead it should
       just mark it as such when it discovers it. */
    MultisampleTexture2D wrapped = MultisampleTexture2D::wrap(texture.id());
    CORRADE_COMPARE(wrapped.flags(), ObjectFlags{});

    #ifndef MAGNUM_TARGET_WEBGL
    wrapped.label();
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(wrapped.flags(), ObjectFlag::Created);
    #else
    CORRADE_SKIP("No API that would call createIfNotAlready() on WebGL, can't test.");
    #endif
}

void MultisampleTextureGLTest::wrapCreateIfNotAlready2DArray() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isExtensionSupported<Extensions::OES::texture_storage_multisample_2d_array>())
        CORRADE_SKIP(Extensions::OES::texture_storage_multisample_2d_array::string() << "is not supported.");
    #endif

    /* Make an object and ensure it's created */
    MultisampleTexture2DArray texture;
    texture.bind(0);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(texture.flags(), ObjectFlag::Created|ObjectFlag::DeleteOnDestruction);

    /* Wrap into another object without ObjectFlag::Created being set, which is
       a common usage pattern to make non-owning references. Then calling an
       API that internally does createIfNotAlready() shouldn't assert just
       because Created isn't set but the object is bound, instead it should
       just mark it as such when it discovers it. */
    MultisampleTexture2DArray wrapped = MultisampleTexture2DArray::wrap(texture.id());
    CORRADE_COMPARE(wrapped.flags(), ObjectFlags{});

    #ifndef MAGNUM_TARGET_WEBGL
    wrapped.label();
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(wrapped.flags(), ObjectFlag::Created);
    #else
    CORRADE_SKIP("No API that would call createIfNotAlready() on WebGL, can't test.");
    #endif
}

void MultisampleTextureGLTest::label2D() {
    /* No-Op version is tested in AbstractObjectGLTest */
    if(!Context::current().isExtensionSupported<Extensions::KHR::debug>() &&
       !Context::current().isExtensionSupported<Extensions::EXT::debug_label>())
        CORRADE_SKIP("Required extension is not available");

    MultisampleTexture2D texture;
    CORRADE_COMPARE(texture.label(), "");
    MAGNUM_VERIFY_NO_GL_ERROR();

    /* Test the string size gets correctly used, instead of relying on null
       termination */
    texture.setLabel("MyTexture!"_s.exceptSuffix(1));
    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE(texture.label(), "MyTexture");
    MAGNUM_VERIFY_NO_GL_ERROR();
}

void MultisampleTextureGLTest::label2DArray() {
    /* No-Op version is tested in AbstractObjectGLTest */
    if(!Context::current().isExtensionSupported<Extensions::KHR::debug>() &&
       !Context::current().isExtensionSupported<Extensions::EXT::debug_label>())
        CORRADE_SKIP("Required extension is not available");

    MultisampleTexture2DArray texture;

    CORRADE_COMPARE(texture.label(), "");
    MAGNUM_VERIFY_NO_GL_ERROR();

    /* Test the string size gets correctly used, instead of relying on null
       termination */
    texture.setLabel("MyTexture!"_s.exceptSuffix(1));
    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE(texture.label(), "MyTexture");
    MAGNUM_VERIFY_NO_GL_ERROR();
}

void MultisampleTextureGLTest::bind2D() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isVersionSupported(Version::GLES310))
        CORRADE_SKIP("OpenGL ES 3.1 is not supported.");
    #endif

    MultisampleTexture2D texture;
    texture.bind(15);

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::unbind(15);

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::bind(7, {&texture, nullptr, &texture});

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::unbind(7, 3);

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void MultisampleTextureGLTest::bind2DArray() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isExtensionSupported<Extensions::OES::texture_storage_multisample_2d_array>())
        CORRADE_SKIP(Extensions::OES::texture_storage_multisample_2d_array::string() << "is not supported.");
    #endif

    MultisampleTexture2DArray texture;
    texture.bind(15);

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::unbind(15);

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::bind(7, {&texture, nullptr, &texture});

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::unbind(7, 3);

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void MultisampleTextureGLTest::bindImage2D() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::shader_image_load_store>())
        CORRADE_SKIP(Extensions::ARB::shader_image_load_store::string() << "is not supported.");
    #else
    if(!Context::current().isExtensionSupported<Extensions::OES::texture_storage_multisample_2d_array>())
        CORRADE_SKIP(Extensions::OES::texture_storage_multisample_2d_array::string() << "is not supported.");
    if(!Context::current().isVersionSupported(Version::GLES310))
        CORRADE_SKIP("OpenGL ES 3.1 is not supported.");
    #endif

    MultisampleTexture2D texture;
    /* Mesa software implementation supports only 1 sample so we have to clamp */
    texture.setStorage(Math::min(4, MultisampleTexture2D::maxColorSamples()),
            TextureFormat::RGBA8, Vector2i{32})
        .bindImage(2, ImageAccess::ReadWrite, ImageFormat::RGBA8);

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::unbindImage(2);

    MAGNUM_VERIFY_NO_GL_ERROR();

    #ifndef MAGNUM_TARGET_GLES
    AbstractTexture::bindImages(1, {&texture, nullptr, &texture});

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::unbindImages(1, 3);

    MAGNUM_VERIFY_NO_GL_ERROR();
    #endif
}

void MultisampleTextureGLTest::bindImage2DArray() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::shader_image_load_store>())
        CORRADE_SKIP(Extensions::ARB::shader_image_load_store::string() << "is not supported.");
    #else
    if(!Context::current().isExtensionSupported<Extensions::OES::texture_storage_multisample_2d_array>())
        CORRADE_SKIP(Extensions::OES::texture_storage_multisample_2d_array::string() << "is not supported.");
    if(!Context::current().isVersionSupported(Version::GLES310))
        CORRADE_SKIP("OpenGL ES 3.1 is not supported.");
    #endif

    MultisampleTexture2DArray texture;
    /* Mesa software implementation supports only 1 sample so we have to clamp */
    texture.setStorage(Math::min(4, MultisampleTexture2DArray::maxColorSamples()),
            TextureFormat::RGBA8, {32, 32, 4})
        .bindImage(2, 1, ImageAccess::ReadWrite, ImageFormat::RGBA8);

    MAGNUM_VERIFY_NO_GL_ERROR();

    texture.bindImageLayered(3, ImageAccess::ReadWrite, ImageFormat::RGBA8);

    AbstractTexture::unbindImage(2);
    AbstractTexture::unbindImage(3);

    MAGNUM_VERIFY_NO_GL_ERROR();

    #ifndef MAGNUM_TARGET_GLES
    AbstractTexture::bindImages(1, {&texture, nullptr, &texture});

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::unbindImages(1, 3);

    MAGNUM_VERIFY_NO_GL_ERROR();
    #endif
}

void MultisampleTextureGLTest::storage2D() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isVersionSupported(Version::GLES310))
        CORRADE_SKIP("OpenGL ES 3.1 is not supported.");
    #endif

    MultisampleTexture2D texture;
    /* Mesa software implementation supports only 1 sample so we have to clamp */
    texture.setStorage(Math::min(4, MultisampleTexture2D::maxColorSamples()),
        TextureFormat::RGBA8, {16, 16});

    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE(texture.imageSize(), Vector2i(16, 16));

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void MultisampleTextureGLTest::storage2DArray() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isExtensionSupported<Extensions::OES::texture_storage_multisample_2d_array>())
        CORRADE_SKIP(Extensions::OES::texture_storage_multisample_2d_array::string() << "is not supported.");
    #endif

    MultisampleTexture2DArray texture;
    /* Mesa software implementation supports only 1 sample so we have to clamp */
    texture.setStorage(Math::min(4, MultisampleTexture2DArray::maxColorSamples()),
        TextureFormat::RGBA8, {16, 16, 5});

    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE(texture.imageSize(), Vector3i(16, 16, 5));

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void MultisampleTextureGLTest::view2D() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_storage_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_storage_multisample::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_view>())
        CORRADE_SKIP(Extensions::ARB::texture_view::string() << "is not supported.");
    #else
    if(!Context::current().isExtensionSupported<Extensions::EXT::texture_view>() &&
       !Context::current().isExtensionSupported<Extensions::OES::texture_view>())
        CORRADE_SKIP("Neither" << Extensions::EXT::texture_view::string() << "nor" << Extensions::OES::texture_view::string() << "is supported.");
    #endif

    MultisampleTexture2D texture;
    /* Mesa software implementation supports only 1 sample so we have to clamp */
    texture.setStorage(Math::min(4, MultisampleTexture2D::maxColorSamples()),
        TextureFormat::RGBA8, {32, 8});

    auto view = MultisampleTexture2D::view(texture, TextureFormat::RGBA8);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(view.imageSize(), (Vector2i{32, 8}));
}

void MultisampleTextureGLTest::view2DOnArray() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_storage_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_storage_multisample::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_view>())
        CORRADE_SKIP(Extensions::ARB::texture_view::string() << "is not supported.");
    #else
    if(!Context::current().isExtensionSupported<Extensions::OES::texture_storage_multisample_2d_array>())
        CORRADE_SKIP(Extensions::OES::texture_storage_multisample_2d_array::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::EXT::texture_view>() &&
       !Context::current().isExtensionSupported<Extensions::OES::texture_view>())
        CORRADE_SKIP("Neither" << Extensions::EXT::texture_view::string() << "nor" << Extensions::OES::texture_view::string() << "is supported.");
    #endif

    MultisampleTexture2DArray texture;
    /* Mesa software implementation supports only 1 sample so we have to clamp */
    texture.setStorage(Math::min(4, MultisampleTexture2D::maxColorSamples()),
        TextureFormat::RGBA8, {32, 8, 7});

    auto view = MultisampleTexture2D::view(texture, TextureFormat::RGBA8, 4);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(view.imageSize(), (Vector2i{32, 8}));
}

void MultisampleTextureGLTest::view2DArray() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_storage_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_storage_multisample::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_view>())
        CORRADE_SKIP(Extensions::ARB::texture_view::string() << "is not supported.");
    #else
    if(!Context::current().isExtensionSupported<Extensions::OES::texture_storage_multisample_2d_array>())
        CORRADE_SKIP(Extensions::OES::texture_storage_multisample_2d_array::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::EXT::texture_view>() &&
       !Context::current().isExtensionSupported<Extensions::OES::texture_view>())
        CORRADE_SKIP("Neither" << Extensions::EXT::texture_view::string() << "nor" << Extensions::OES::texture_view::string() << "is supported.");
    #endif

    MultisampleTexture2DArray texture;
    /* Mesa software implementation supports only 1 sample so we have to clamp */
    texture.setStorage(Math::min(4, MultisampleTexture2D::maxColorSamples()),
        TextureFormat::RGBA8, {32, 8, 7});

    auto view = MultisampleTexture2DArray::view(texture, TextureFormat::RGBA8, 4, 3);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(view.imageSize(), (Vector3i{32, 8, 3}));
}

void MultisampleTextureGLTest::view2DArrayOnNonArray() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_storage_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_storage_multisample::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_view>())
        CORRADE_SKIP(Extensions::ARB::texture_view::string() << "is not supported.");
    #else
    if(!Context::current().isExtensionSupported<Extensions::OES::texture_storage_multisample_2d_array>())
        CORRADE_SKIP(Extensions::OES::texture_storage_multisample_2d_array::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::EXT::texture_view>() &&
       !Context::current().isExtensionSupported<Extensions::OES::texture_view>())
        CORRADE_SKIP("Neither" << Extensions::EXT::texture_view::string() << "nor" << Extensions::OES::texture_view::string() << "is supported.");
    #endif

    MultisampleTexture2D texture;
    /* Mesa software implementation supports only 1 sample so we have to clamp */
    texture.setStorage(Math::min(4, MultisampleTexture2D::maxColorSamples()),
        TextureFormat::RGBA8, {32, 8});

    auto view = MultisampleTexture2DArray::view(texture, TextureFormat::RGBA8);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(view.imageSize(), (Vector3i{32, 8, 1}));
}

void MultisampleTextureGLTest::invalidateImage2D() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isVersionSupported(Version::GLES310))
        CORRADE_SKIP("OpenGL ES 3.1 is not supported.");
    #endif

    MultisampleTexture2D texture;
    /* Mesa software implementation supports only 1 sample so we have to clamp */
    texture.setStorage(Math::min(4, MultisampleTexture2D::maxColorSamples()),
        TextureFormat::RGBA8, {16, 16});
    texture.invalidateImage();

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void MultisampleTextureGLTest::invalidateImage2DArray() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isExtensionSupported<Extensions::OES::texture_storage_multisample_2d_array>())
        CORRADE_SKIP(Extensions::OES::texture_storage_multisample_2d_array::string() << "is not supported.");
    #endif

    MultisampleTexture2DArray texture;
    /* Mesa software implementation supports only 1 sample so we have to clamp */
    texture.setStorage(Math::min(4, MultisampleTexture2DArray::maxColorSamples()),
        TextureFormat::RGBA8, {16, 16, 5});
    texture.invalidateImage();

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void MultisampleTextureGLTest::invalidateSubImage2D() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isVersionSupported(Version::GLES310))
        CORRADE_SKIP("OpenGL ES 3.1 is not supported.");
    #endif

    MultisampleTexture2D texture;
    /* Mesa software implementation supports only 1 sample so we have to clamp */
    texture.setStorage(Math::min(4, MultisampleTexture2D::maxColorSamples()),
        TextureFormat::RGBA8, {16, 16});
    texture.invalidateSubImage({3, 4}, {5, 6});

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void MultisampleTextureGLTest::invalidateSubImage2DArray() {
    #ifndef MAGNUM_TARGET_GLES
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_multisample>())
        CORRADE_SKIP(Extensions::ARB::texture_multisample::string() << "is not supported.");
    #else
    if(!Context::current().isExtensionSupported<Extensions::OES::texture_storage_multisample_2d_array>())
        CORRADE_SKIP(Extensions::OES::texture_storage_multisample_2d_array::string() << "is not supported.");
    #endif

    MultisampleTexture2DArray texture;
    /* Mesa software implementation supports only 1 sample so we have to clamp */
    texture.setStorage(Math::min(4, MultisampleTexture2DArray::maxColorSamples()),
        TextureFormat::RGBA8, {16, 16, 5});
    texture.invalidateSubImage({3, 4, 1}, {5, 6, 3});

    MAGNUM_VERIFY_NO_GL_ERROR();
}

}}}}

CORRADE_TEST_MAIN(Magnum::GL::Test::MultisampleTextureGLTest)
