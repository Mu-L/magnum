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

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Compare/Container.h>

#include "Magnum/Image.h"
#include "Magnum/ImageView.h"
#include "Magnum/GL/Context.h"
#include "Magnum/GL/Extensions.h"
#include "Magnum/GL/BufferImage.h"
#include "Magnum/GL/ImageFormat.h"
#include "Magnum/GL/OpenGLTester.h"
#include "Magnum/GL/PixelFormat.h"
#include "Magnum/GL/RectangleTexture.h"
#include "Magnum/GL/TextureFormat.h"
#include "Magnum/Math/Color.h"
#include "Magnum/Math/Range.h"

namespace Magnum { namespace GL { namespace Test { namespace {

struct RectangleTextureGLTest: OpenGLTester {
    explicit RectangleTextureGLTest();

    void compressedBlockSize();

    void construct();
    void constructMove();
    void wrap();
    void wrapCreateIfNotAlready();

    void label();

    void bind();
    void bindImage();

    template<class T> void sampling();
    void samplingSrgbDecode();
    void samplingBorderInteger();
    void samplingSwizzle();
    void samplingDepthStencilMode();

    void storage();

    void view();

    void image();
    void imageBuffer();
    void imageQueryView();
    void subImage();
    void subImageBuffer();
    void subImageQuery();
    void subImageQueryView();
    void subImageQueryBuffer();
    /* View query assertions tested in AbstractTextureGLTest */

    void compressedImage();
    void compressedImageBuffer();
    void compressedImageQueryView();
    void compressedSubImage();
    void compressedSubImageBuffer();
    void compressedSubImageQuery();
    void compressedSubImageQueryView();
    void compressedSubImageQueryBuffer();
    /* View query assertions tested in AbstractTextureGLTest */

    void invalidateImage();
    void invalidateSubImage();
};

struct GenericSampler {
    typedef Magnum::SamplerFilter Filter;
    typedef Magnum::SamplerWrapping Wrapping;
};
struct GLSampler {
    typedef GL::SamplerFilter Filter;
    typedef GL::SamplerWrapping Wrapping;
};

constexpr UnsignedByte Data[]{
    0, 0, 0, 0, 0, 0, 0, 0,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

const struct {
    const char* name;
    Containers::ArrayView<const UnsignedByte> data;
    PixelStorage storage;
    Containers::ArrayView<const UnsignedByte> dataSparse;
    std::size_t offset;
} PixelStorageData[]{
    {"default pixel storage",
        Containers::arrayView(Data).exceptPrefix(8), {},
        Containers::arrayView(Data).exceptPrefix(8), 0},
    #if !defined(MAGNUM_TARGET_GLES2) || !defined(MAGNUM_TARGET_WEBGL)
    {"skip Y",
        Containers::arrayView(Data).exceptPrefix(8), PixelStorage{}.setSkip({0, 1, 0}),
        Containers::arrayView(Data), 8}
    #endif
};

RectangleTextureGLTest::RectangleTextureGLTest() {
    addTests({&RectangleTextureGLTest::compressedBlockSize,

              &RectangleTextureGLTest::construct,
              &RectangleTextureGLTest::constructMove,
              &RectangleTextureGLTest::wrap,
              &RectangleTextureGLTest::wrapCreateIfNotAlready,

              &RectangleTextureGLTest::label,

              &RectangleTextureGLTest::bind,
              &RectangleTextureGLTest::bindImage,

              &RectangleTextureGLTest::sampling<GenericSampler>,
              &RectangleTextureGLTest::sampling<GLSampler>,
              &RectangleTextureGLTest::samplingSrgbDecode,
              &RectangleTextureGLTest::samplingBorderInteger,
              &RectangleTextureGLTest::samplingSwizzle,
              &RectangleTextureGLTest::samplingDepthStencilMode,

              &RectangleTextureGLTest::storage,

              &RectangleTextureGLTest::view});

    addInstancedTests({
        &RectangleTextureGLTest::image,
        &RectangleTextureGLTest::imageBuffer,
        &RectangleTextureGLTest::imageQueryView,
        &RectangleTextureGLTest::subImage,
        &RectangleTextureGLTest::subImageBuffer,
        &RectangleTextureGLTest::subImageQuery,
        &RectangleTextureGLTest::subImageQueryView,
        &RectangleTextureGLTest::subImageQueryBuffer},
        Containers::arraySize(PixelStorageData));

    addTests({&RectangleTextureGLTest::compressedImage,
              &RectangleTextureGLTest::compressedImageBuffer,
              &RectangleTextureGLTest::compressedImageQueryView,
              &RectangleTextureGLTest::compressedSubImage,
              &RectangleTextureGLTest::compressedSubImageBuffer,
              &RectangleTextureGLTest::compressedSubImageQuery,
              &RectangleTextureGLTest::compressedSubImageQueryView,
              &RectangleTextureGLTest::compressedSubImageQueryBuffer,

              &RectangleTextureGLTest::invalidateImage,
              &RectangleTextureGLTest::invalidateSubImage});
}

using namespace Containers::Literals;

void RectangleTextureGLTest::compressedBlockSize() {
    /* For uncompressed formats returns zero */
    CORRADE_COMPARE(RectangleTexture::compressedBlockSize(TextureFormat::RGBA8), Vector2i{});
    CORRADE_COMPARE(RectangleTexture::compressedBlockDataSize(TextureFormat::RGBA8), 0);

    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_SKIP("No rectangle texture compression format exists.");
}

void RectangleTextureGLTest::construct() {
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");

    {
        RectangleTexture texture;

        MAGNUM_VERIFY_NO_GL_ERROR();
        CORRADE_VERIFY(texture.id() > 0);
    }

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void RectangleTextureGLTest::constructMove() {
    /* Move constructor tested in AbstractTexture, here we just verify there
       are no extra members that would need to be taken care of */
    CORRADE_COMPARE(sizeof(RectangleTexture), sizeof(AbstractTexture));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<RectangleTexture>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<RectangleTexture>::value);
}

void RectangleTextureGLTest::wrap() {
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");

    GLuint id;
    glGenTextures(1, &id);

    /* Releasing won't delete anything */
    {
        auto texture = RectangleTexture::wrap(id, ObjectFlag::DeleteOnDestruction);
        CORRADE_COMPARE(texture.release(), id);
    }

    /* ...so we can wrap it again */
    RectangleTexture::wrap(id);
    glDeleteTextures(1, &id);
}

void RectangleTextureGLTest::wrapCreateIfNotAlready() {
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");

    /* Make an object and ensure it's created */
    RectangleTexture texture;
    texture.bind(0);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(texture.flags(), ObjectFlag::Created|ObjectFlag::DeleteOnDestruction);

    /* Wrap into another object without ObjectFlag::Created being set, which is
       a common usage pattern to make non-owning references. Then calling an
       API that internally does createIfNotAlready() shouldn't assert just
       because Created isn't set but the object is bound, instead it should
       just mark it as such when it discovers it. */
    RectangleTexture wrapped = RectangleTexture::wrap(texture.id());
    CORRADE_COMPARE(wrapped.flags(), ObjectFlags{});

    wrapped.label();
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(wrapped.flags(), ObjectFlag::Created);
}

void RectangleTextureGLTest::label() {
    /* No-Op version is tested in AbstractObjectGLTest */
    if(!Context::current().isExtensionSupported<Extensions::KHR::debug>() &&
       !Context::current().isExtensionSupported<Extensions::EXT::debug_label>())
        CORRADE_SKIP("Required extension is not available");

    RectangleTexture texture;
    CORRADE_COMPARE(texture.label(), "");
    MAGNUM_VERIFY_NO_GL_ERROR();

    /* Test the string size gets correctly used, instead of relying on null
       termination */
    texture.setLabel("MyTexture!"_s.exceptSuffix(1));
    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE(texture.label(), "MyTexture");
    MAGNUM_VERIFY_NO_GL_ERROR();
}

void RectangleTextureGLTest::bind() {
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");

    RectangleTexture texture;
    texture.bind(15);

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::unbind(15);

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::bind(7, {&texture, nullptr, &texture});

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::unbind(7, 3);

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void RectangleTextureGLTest::bindImage() {
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::shader_image_load_store>())
        CORRADE_SKIP(Extensions::ARB::shader_image_load_store::string() << "is not supported.");

    RectangleTexture texture;
    texture.setStorage(TextureFormat::RGBA8, Vector2i{32})
        .bindImage(2, ImageAccess::ReadWrite, ImageFormat::RGBA8);

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::unbindImage(2);

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::bindImages(1, {&texture, nullptr, &texture});

    MAGNUM_VERIFY_NO_GL_ERROR();

    AbstractTexture::unbindImages(1, 3);

    MAGNUM_VERIFY_NO_GL_ERROR();
}

template<class T> void RectangleTextureGLTest::sampling() {
    setTestCaseTemplateName(std::is_same<T, GenericSampler>::value ?
        "GenericSampler" : "GLSampler");

    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");

    RectangleTexture texture;
    texture.setMinificationFilter(T::Filter::Linear)
           .setMagnificationFilter(T::Filter::Linear)
           .setWrapping(T::Wrapping::ClampToBorder)
           .setBorderColor(Color3(0.5f))
           .setMaxAnisotropy(Sampler::maxMaxAnisotropy())
           .setCompareMode(SamplerCompareMode::CompareRefToTexture)
           .setCompareFunction(SamplerCompareFunction::GreaterOrEqual);

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void RectangleTextureGLTest::samplingSrgbDecode() {
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::EXT::texture_sRGB_decode>())
        CORRADE_SKIP(Extensions::EXT::texture_sRGB_decode::string() << "is not supported.");

    RectangleTexture texture;
    texture.setSrgbDecode(false);

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void RectangleTextureGLTest::samplingBorderInteger() {
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::EXT::texture_integer>())
        CORRADE_SKIP(Extensions::EXT::texture_integer::string() << "is not supported.");

    RectangleTexture a;
    a.setWrapping(SamplerWrapping::ClampToBorder)
     .setBorderColor(Vector4i(1, 56, 78, -2));
    RectangleTexture b;
    b.setWrapping(SamplerWrapping::ClampToBorder)
     .setBorderColor(Vector4ui(35, 56, 78, 15));

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void RectangleTextureGLTest::samplingSwizzle() {
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_swizzle>())
        CORRADE_SKIP(Extensions::ARB::texture_swizzle::string() << "is not supported.");

    RectangleTexture texture;
    texture.setSwizzle<'b', 'g', 'r', '0'>();

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void RectangleTextureGLTest::samplingDepthStencilMode() {
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::stencil_texturing>())
        CORRADE_SKIP(Extensions::ARB::stencil_texturing::string() << "is not supported.");

    RectangleTexture texture;
    texture.setDepthStencilMode(SamplerDepthStencilMode::StencilIndex);

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void RectangleTextureGLTest::storage() {
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");

    RectangleTexture texture;
    texture.setStorage(TextureFormat::RGBA8, Vector2i(32));

    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE(texture.imageSize(), Vector2i(32));

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void RectangleTextureGLTest::view() {
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_storage>())
        CORRADE_SKIP(Extensions::ARB::texture_storage::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_view>())
        CORRADE_SKIP(Extensions::ARB::texture_view::string() << "is not supported.");

    RectangleTexture texture;
    texture.setStorage(TextureFormat::RGBA8, {32, 8});

    auto view = RectangleTexture::view(texture, TextureFormat::RGBA8);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(view.imageSize(), (Vector2i{32, 8}));
}

void RectangleTextureGLTest::image() {
    auto&& data = PixelStorageData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");

    RectangleTexture texture;
    texture.setImage(TextureFormat::RGBA8, ImageView2D{
        data.storage,
        PixelFormat::RGBA, PixelType::UnsignedByte, Vector2i(2),
        data.dataSparse});

    MAGNUM_VERIFY_NO_GL_ERROR();

    Image2D image = texture.image({data.storage,
        PixelFormat::RGBA, PixelType::UnsignedByte});

    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE(image.flags(), ImageFlag2D{});
    CORRADE_COMPARE(image.size(), Vector2i(2));
    CORRADE_COMPARE_AS(Containers::arrayCast<UnsignedByte>(image.data()).exceptPrefix(data.offset),
        data.data,
        TestSuite::Compare::Container);
}

void RectangleTextureGLTest::imageBuffer() {
    auto&& data = PixelStorageData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");

    RectangleTexture texture;
    texture.setImage(TextureFormat::RGBA8, BufferImage2D{
        data.storage,
        PixelFormat::RGBA, PixelType::UnsignedByte, Vector2i(2),
        data.dataSparse,
        BufferUsage::StaticDraw});

    MAGNUM_VERIFY_NO_GL_ERROR();

    BufferImage2D image = texture.image({data.storage,
        PixelFormat::RGBA, PixelType::UnsignedByte}, BufferUsage::StaticRead);
    const auto imageData = image.buffer().data();

    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE(image.size(), Vector2i(2));
    CORRADE_COMPARE_AS(Containers::arrayCast<UnsignedByte>(imageData).exceptPrefix(data.offset),
        data.data,
        TestSuite::Compare::Container);
}

void RectangleTextureGLTest::imageQueryView() {
    auto&& data = PixelStorageData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");

    RectangleTexture texture;
    texture.setImage(TextureFormat::RGBA8, ImageView2D{
        data.storage,
        PixelFormat::RGBA, PixelType::UnsignedByte, Vector2i(2),
        data.dataSparse});

    MAGNUM_VERIFY_NO_GL_ERROR();

    Containers::Array<char> imageData{data.offset + 2*2*4};
    MutableImageView2D image{data.storage, PixelFormat::RGBA, PixelType::UnsignedByte, Vector2i{2}, imageData, ImageFlag2D::Array};
    texture.image(image);

    MAGNUM_VERIFY_NO_GL_ERROR();

    /* Doesn't matter what flags are set, they stay untouched */
    CORRADE_COMPARE(image.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(image.size(), Vector2i(2));
    CORRADE_COMPARE_AS(Containers::arrayCast<UnsignedByte>(image.data()).exceptPrefix(data.offset),
        data.data,
        TestSuite::Compare::Container);
}

constexpr UnsignedByte Zero[4*4*4]{};
constexpr UnsignedByte SubDataComplete[]{
    0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0, 0, 0, 0,
    0, 0, 0, 0, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0, 0, 0, 0,
    0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0, 0, 0, 0, 0
};

void RectangleTextureGLTest::subImage() {
    auto&& data = PixelStorageData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");

    RectangleTexture texture;
    texture.setImage(TextureFormat::RGBA8,
        ImageView2D(PixelFormat::RGBA, PixelType::UnsignedByte, Vector2i(4), Zero));
    texture.setSubImage(Vector2i(1), ImageView2D{
        data.storage,
        PixelFormat::RGBA, PixelType::UnsignedByte, Vector2i(2),
        data.dataSparse});

    MAGNUM_VERIFY_NO_GL_ERROR();

    Image2D image = texture.image({PixelFormat::RGBA, PixelType::UnsignedByte});

    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE(image.size(), Vector2i(4));
    CORRADE_COMPARE_AS(Containers::arrayCast<UnsignedByte>(image.data()),
        Containers::arrayView(SubDataComplete), TestSuite::Compare::Container);
}

void RectangleTextureGLTest::subImageBuffer() {
    auto&& data = PixelStorageData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");

    RectangleTexture texture;
    texture.setImage(TextureFormat::RGBA8,
        ImageView2D(PixelFormat::RGBA, PixelType::UnsignedByte, Vector2i(4), Zero));
    texture.setSubImage(Vector2i(1), BufferImage2D{
        data.storage,
        PixelFormat::RGBA, PixelType::UnsignedByte, Vector2i(2),
        data.dataSparse,
        BufferUsage::StaticDraw});

    MAGNUM_VERIFY_NO_GL_ERROR();

    BufferImage2D image = texture.image({PixelFormat::RGBA, PixelType::UnsignedByte}, BufferUsage::StaticRead);
    const auto imageData = image.buffer().data();

    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE(image.size(), Vector2i(4));
    CORRADE_COMPARE_AS(Containers::arrayCast<UnsignedByte>(imageData),
        Containers::arrayView(SubDataComplete),
        TestSuite::Compare::Container);
}

void RectangleTextureGLTest::subImageQuery() {
    auto&& data = PixelStorageData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::get_texture_sub_image>())
        CORRADE_SKIP(Extensions::ARB::get_texture_sub_image::string() << "is not supported.");

    RectangleTexture texture;
    texture.setStorage(TextureFormat::RGBA8, Vector2i{4})
           .setSubImage({}, ImageView2D{PixelFormat::RGBA, PixelType::UnsignedByte, Vector2i{4}, SubDataComplete});

    MAGNUM_VERIFY_NO_GL_ERROR();

    Image2D image = texture.subImage(Range2Di::fromSize(Vector2i{1}, Vector2i{2}),
        {data.storage,
        PixelFormat::RGBA, PixelType::UnsignedByte});

    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE(image.flags(), ImageFlag2D{});
    CORRADE_COMPARE(image.size(), Vector2i{2});
    CORRADE_COMPARE_AS(Containers::arrayCast<UnsignedByte>(image.data()).exceptPrefix(data.offset),
        data.data,
        TestSuite::Compare::Container);
}

void RectangleTextureGLTest::subImageQueryView() {
    auto&& data = PixelStorageData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::get_texture_sub_image>())
        CORRADE_SKIP(Extensions::ARB::get_texture_sub_image::string() << "is not supported.");

    RectangleTexture texture;
    texture.setStorage(TextureFormat::RGBA8, Vector2i{4})
           .setSubImage({}, ImageView2D{PixelFormat::RGBA, PixelType::UnsignedByte, Vector2i{4}, SubDataComplete});

    MAGNUM_VERIFY_NO_GL_ERROR();

    Containers::Array<char> imageData{data.offset + 2*2*4};
    MutableImageView2D image{data.storage, PixelFormat::RGBA, PixelType::UnsignedByte, Vector2i{2}, imageData, ImageFlag2D::Array};
    texture.subImage(Range2Di::fromSize(Vector2i{1}, Vector2i{2}), image);

    MAGNUM_VERIFY_NO_GL_ERROR();

    /* Doesn't matter what flags are set, they stay untouched */
    CORRADE_COMPARE(image.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(image.size(), Vector2i{2});
    CORRADE_COMPARE_AS(Containers::arrayCast<UnsignedByte>(image.data()).exceptPrefix(data.offset),
        data.data,
        TestSuite::Compare::Container);
}

void RectangleTextureGLTest::subImageQueryBuffer() {
    auto&& data = PixelStorageData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");
    if(!Context::current().isExtensionSupported<Extensions::ARB::get_texture_sub_image>())
        CORRADE_SKIP(Extensions::ARB::get_texture_sub_image::string() << "is not supported.");

    RectangleTexture texture;
    texture.setStorage(TextureFormat::RGBA8, Vector2i{4})
           .setSubImage({}, ImageView2D{PixelFormat::RGBA, PixelType::UnsignedByte, Vector2i{4}, SubDataComplete});

    MAGNUM_VERIFY_NO_GL_ERROR();

    BufferImage2D image = texture.subImage(Range2Di::fromSize(Vector2i{1}, Vector2i{2}),
        {data.storage,
        PixelFormat::RGBA, PixelType::UnsignedByte}, BufferUsage::StaticRead);
    const auto imageData = image.buffer().data();

    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE(image.size(), Vector2i{2});
    CORRADE_COMPARE_AS(Containers::arrayCast<UnsignedByte>(imageData).exceptPrefix(data.offset),
        data.data,
        TestSuite::Compare::Container);
}

void RectangleTextureGLTest::compressedImage() {
    CORRADE_SKIP("No rectangle texture compression format exists.");
}

void RectangleTextureGLTest::compressedImageBuffer() {
    CORRADE_SKIP("No rectangle texture compression format exists.");
}

void RectangleTextureGLTest::compressedImageQueryView() {
    CORRADE_SKIP("No rectangle texture compression format exists.");
}

void RectangleTextureGLTest::compressedSubImage() {
    CORRADE_SKIP("No rectangle texture compression format exists.");
}

void RectangleTextureGLTest::compressedSubImageBuffer() {
    CORRADE_SKIP("No rectangle texture compression format exists.");
}

void RectangleTextureGLTest::compressedSubImageQuery() {
    CORRADE_SKIP("No rectangle texture compression format exists.");
}

void RectangleTextureGLTest::compressedSubImageQueryView() {
    CORRADE_SKIP("No rectangle texture compression format exists.");
}

void RectangleTextureGLTest::compressedSubImageQueryBuffer() {
    CORRADE_SKIP("No rectangle texture compression format exists.");
}

void RectangleTextureGLTest::invalidateImage() {
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");

    RectangleTexture texture;
    texture.setStorage(TextureFormat::RGBA8, Vector2i(32));
    texture.invalidateImage();

    MAGNUM_VERIFY_NO_GL_ERROR();
}

void RectangleTextureGLTest::invalidateSubImage() {
    if(!Context::current().isExtensionSupported<Extensions::ARB::texture_rectangle>())
        CORRADE_SKIP(Extensions::ARB::texture_rectangle::string() << "is not supported.");

    RectangleTexture texture;
    texture.setStorage(TextureFormat::RGBA8, Vector2i(32));
    texture.invalidateSubImage(Vector2i(4), Vector2i(16));

    MAGNUM_VERIFY_NO_GL_ERROR();
}

}}}}

CORRADE_TEST_MAIN(Magnum::GL::Test::RectangleTextureGLTest)
