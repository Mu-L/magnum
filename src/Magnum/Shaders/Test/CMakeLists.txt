#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021, 2022, 2023, 2024, 2025
#             Vladimír Vondruš <mosra@centrum.cz>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

# IDE folder in VS, Xcode etc. CMake 3.12+, older versions have only the FOLDER
# property that would have to be set on each target separately.
set(CMAKE_FOLDER "Magnum/Shaders/Test")

corrade_add_test(ShadersDistanceFieldVectorTest DistanceFieldVectorTest.cpp LIBRARIES MagnumShaders)
corrade_add_test(ShadersFlatTest FlatTest.cpp LIBRARIES MagnumShaders)
corrade_add_test(ShadersGenericTest GenericTest.cpp LIBRARIES MagnumShaders)
corrade_add_test(ShadersMeshVisualizerTest MeshVisualizerTest.cpp LIBRARIES MagnumShaders)
corrade_add_test(ShadersPhongTest PhongTest.cpp LIBRARIES MagnumShaders)
corrade_add_test(ShadersLineTest LineTest.cpp LIBRARIES MagnumShadersTestLib)
target_compile_definitions(ShadersLineTest PRIVATE "CORRADE_GRACEFUL_ASSERT")
corrade_add_test(ShadersVectorTest VectorTest.cpp LIBRARIES MagnumShaders)

if(MAGNUM_TARGET_GL)
    corrade_add_test(ShadersGLShaderWrapperTest GLShaderWrapperTest.cpp LIBRARIES MagnumShaders)

    # There's an underscore between GL and Test to disambiguate from GLTest,
    # which is a common suffix used to mark tests that need a GL context. Ugly,
    # I know.
    corrade_add_test(ShadersDistanceFieldVectorGL_Test DistanceFieldVectorGL_Test.cpp LIBRARIES MagnumShaders)
    corrade_add_test(ShadersFlatGL_Test FlatGL_Test.cpp LIBRARIES MagnumShadersTestLib)
    corrade_add_test(ShadersGenericGL_Test GenericGL_Test.cpp LIBRARIES MagnumShaders)
    corrade_add_test(ShadersMeshVisualizerGL_Test MeshVisualizerGL_Test.cpp LIBRARIES MagnumShadersTestLib)
    corrade_add_test(ShadersPhongGL_Test PhongGL_Test.cpp LIBRARIES MagnumShadersTestLib)
    corrade_add_test(ShadersVectorGL_Test VectorGL_Test.cpp LIBRARIES MagnumShaders)
    corrade_add_test(ShadersVertexColorGL_Test VertexColorGL_Test.cpp LIBRARIES MagnumShaders)

    if(NOT MAGNUM_TARGET_GLES2)
        corrade_add_test(ShadersLineGL_Test LineGL_Test.cpp LIBRARIES MagnumShaders)
    endif()

    if(MAGNUM_BUILD_GL_TESTS)
        # Otherwise CMake complains that Corrade::PluginManager is not found,
        # wtf
        find_package(Corrade REQUIRED PluginManager)

        if(CORRADE_TARGET_EMSCRIPTEN OR CORRADE_TARGET_ANDROID)
            set(SHADERS_TEST_DIR ".")
        else()
            set(SHADERS_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
        endif()

        if(NOT MAGNUM_BUILD_PLUGINS_STATIC)
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                set(ANYIMAGEIMPORTER_PLUGIN_FILENAME $<TARGET_FILE:AnyImageImporter>)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                set(TGAIMPORTER_PLUGIN_FILENAME $<TARGET_FILE:TgaImporter>)
            endif()
        endif()

        # First replace ${} variables, then $<> generator expressions
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
                    ${CMAKE_CURRENT_BINARY_DIR}/configure.h.in)
        file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/configure.h
            INPUT ${CMAKE_CURRENT_BINARY_DIR}/configure.h.in)

        set(ShadersDistanceFieldVectorGLTest_SRCS DistanceFieldVectorGLTest.cpp)
        if(CORRADE_TARGET_IOS)
            list(APPEND ShadersDistanceFieldVectorGLTest_SRCS TestFiles VectorTestFiles)
        endif()
        corrade_add_test(ShadersDistanceFieldVectorGLTest ${ShadersDistanceFieldVectorGLTest_SRCS}
            LIBRARIES
                MagnumDebugTools
                MagnumMeshTools
                MagnumPrimitives
                MagnumShadersTestLib
                MagnumOpenGLTester
            FILES
                TestFiles/vector-distancefield.tga

                VectorTestFiles/defaults.tga
                VectorTestFiles/defaults-distancefield.tga
                VectorTestFiles/smooth0.1-2D.tga
                VectorTestFiles/smooth0.1-3D.tga
                VectorTestFiles/smooth0.2-2D.tga
                VectorTestFiles/smooth0.2-3D.tga
                VectorTestFiles/outline2D.tga
                VectorTestFiles/outline3D.tga
                VectorTestFiles/multidraw2D-distancefield.tga
                VectorTestFiles/multidraw3D-distancefield.tga)
        target_include_directories(ShadersDistanceFieldVectorGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
        if(MAGNUM_BUILD_PLUGINS_STATIC)
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                target_link_libraries(ShadersDistanceFieldVectorGLTest PRIVATE AnyImageImporter)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                target_link_libraries(ShadersDistanceFieldVectorGLTest PRIVATE TgaImporter)
            endif()
        else()
            # So the plugins get properly built when building the test
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                add_dependencies(ShadersDistanceFieldVectorGLTest AnyImageImporter)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                add_dependencies(ShadersDistanceFieldVectorGLTest TgaImporter)
            endif()
        endif()

        set(ShadersFlatGLTest_SRCS FlatGLTest.cpp)
        if(CORRADE_TARGET_IOS)
            list(APPEND ShadersFlatGLTest_SRCS TestFiles FlatTestFiles)
        endif()
        corrade_add_test(ShadersFlatGLTest ${ShadersFlatGLTest_SRCS}
            LIBRARIES
                MagnumDebugTools
                MagnumMeshTools
                MagnumPrimitives
                MagnumShadersTestLib
                MagnumOpenGLTester
            FILES
                TestFiles/diffuse-alpha-texture.tga
                TestFiles/diffuse-texture.tga
                TestFiles/alpha-mask1.0.tga
                TestFiles/skinning.tga
                TestFiles/skinning-default.tga
                TestFiles/skinning-instanced.tga
                TestFiles/skinning-multi.tga

                FlatTestFiles/colored2D.tga
                FlatTestFiles/colored3D.tga
                FlatTestFiles/defaults.tga
                FlatTestFiles/instanced2D.tga
                FlatTestFiles/instanced3D.tga
                FlatTestFiles/instanced-textured2D.tga
                FlatTestFiles/instanced-textured3D.tga
                FlatTestFiles/textured2D.tga
                FlatTestFiles/textured3D.tga
                FlatTestFiles/textured2D-alpha.tga
                FlatTestFiles/textured3D-alpha.tga
                FlatTestFiles/textured2D-alpha-mask0.5.tga
                FlatTestFiles/textured3D-alpha-mask0.5.tga
                FlatTestFiles/vertexColor2D.tga
                FlatTestFiles/vertexColor3D.tga
                FlatTestFiles/multidraw2D.tga
                FlatTestFiles/multidraw3D.tga
                FlatTestFiles/multidraw-textured2D.tga
                FlatTestFiles/multidraw-textured3D.tga)
        target_include_directories(ShadersFlatGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
        if(MAGNUM_BUILD_PLUGINS_STATIC)
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                target_link_libraries(ShadersFlatGLTest PRIVATE AnyImageImporter)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                target_link_libraries(ShadersFlatGLTest PRIVATE TgaImporter)
            endif()
        else()
            # So the plugins get properly built when building the test
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                add_dependencies(ShadersFlatGLTest AnyImageImporter)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                add_dependencies(ShadersFlatGLTest TgaImporter)
            endif()
        endif()

        corrade_add_test(ShadersGLShaderWrapperGLTest GLShaderWrapperGLTest.cpp LIBRARIES MagnumShaders MagnumOpenGLTester)

        set(ShadersMeshVisualizerGLTest_SRCS MeshVisualizerGLTest.cpp)
        if(CORRADE_TARGET_IOS)
            list(APPEND ShadersMeshVisualizerGLTest_SRCS FlatTestFiles MeshVisualizerTestFiles)
        endif()
        corrade_add_test(ShadersMeshVisualizerGLTest ${ShadersMeshVisualizerGLTest_SRCS}
            LIBRARIES
                MagnumDebugTools
                MagnumMeshTools
                MagnumPrimitives
                MagnumShadersTestLib
                MagnumOpenGLTester
            FILES
                MeshVisualizerTestFiles/bitangents-from-tangents.tga
                MeshVisualizerTestFiles/defaults-instancedobjectid2D.tga
                MeshVisualizerTestFiles/defaults-instancedobjectid3D.tga
                MeshVisualizerTestFiles/defaults-objectid2D.tga
                MeshVisualizerTestFiles/defaults-objectid3D.tga
                MeshVisualizerTestFiles/defaults-primitiveid2D.tga
                MeshVisualizerTestFiles/defaults-primitiveid3D.tga
                MeshVisualizerTestFiles/defaults-tbn.tga
                MeshVisualizerTestFiles/defaults-vertexid2D.tga
                MeshVisualizerTestFiles/defaults-vertexid3D.tga
                MeshVisualizerTestFiles/defaults-wireframe2D.tga
                MeshVisualizerTestFiles/defaults-wireframe3D.tga
                MeshVisualizerTestFiles/instancedobjectid2D.tga
                MeshVisualizerTestFiles/instancedobjectid3D.tga
                MeshVisualizerTestFiles/objectid-tn.tga
                MeshVisualizerTestFiles/objectid2D.tga
                MeshVisualizerTestFiles/objectid3D.tga
                MeshVisualizerTestFiles/objectidtexture2D.tga
                MeshVisualizerTestFiles/objectidtexture3D.tga
                MeshVisualizerTestFiles/primitiveid-tn.tga
                MeshVisualizerTestFiles/primitiveid2D.tga
                MeshVisualizerTestFiles/primitiveid3D.tga
                MeshVisualizerTestFiles/tbn-wide.tga
                MeshVisualizerTestFiles/tbn.tga
                MeshVisualizerTestFiles/vertexid-tn.tga
                MeshVisualizerTestFiles/vertexid2D.tga
                MeshVisualizerTestFiles/vertexid3D.tga
                MeshVisualizerTestFiles/wireframe-nogeo2D.tga
                MeshVisualizerTestFiles/wireframe-nogeo3D.tga
                MeshVisualizerTestFiles/wireframe-instancedobjectid2D.tga
                MeshVisualizerTestFiles/wireframe-instancedobjectid3D.tga
                MeshVisualizerTestFiles/wireframe-nogeo-instancedobjectid2D.tga
                MeshVisualizerTestFiles/wireframe-nogeo-instancedobjectid3D.tga
                MeshVisualizerTestFiles/wireframe-objectidtexture2D.tga
                MeshVisualizerTestFiles/wireframe-objectidtexture3D.tga
                MeshVisualizerTestFiles/wireframe-perspective.tga
                MeshVisualizerTestFiles/wireframe-primitiveid-tn.tga
                MeshVisualizerTestFiles/wireframe-tn-smooth.tga
                MeshVisualizerTestFiles/wireframe-vertexid2D.tga
                MeshVisualizerTestFiles/wireframe-vertexid3D.tga
                MeshVisualizerTestFiles/wireframe-wide2D.tga
                MeshVisualizerTestFiles/wireframe-wide3D.tga
                MeshVisualizerTestFiles/wireframe2D.tga
                MeshVisualizerTestFiles/wireframe3D.tga
                MeshVisualizerTestFiles/instanced-wireframe2D.tga
                MeshVisualizerTestFiles/instanced-wireframe3D.tga
                MeshVisualizerTestFiles/instanced-wireframe-nogeo2D.tga
                MeshVisualizerTestFiles/instanced-wireframe-nogeo3D.tga
                MeshVisualizerTestFiles/instanced-wireframe-tbn3D.tga
                MeshVisualizerTestFiles/instanced-vertexid2D.tga
                MeshVisualizerTestFiles/instanced-vertexid3D.tga
                MeshVisualizerTestFiles/instanced-instancedobjectid2D.tga
                MeshVisualizerTestFiles/instanced-instancedobjectid3D.tga
                MeshVisualizerTestFiles/instanced-instancedobjectidtexture2D.tga
                MeshVisualizerTestFiles/instanced-instancedobjectidtexture3D.tga
                MeshVisualizerTestFiles/instanced-objectidtexture2D.tga
                MeshVisualizerTestFiles/instanced-objectidtexture3D.tga
                MeshVisualizerTestFiles/multidraw-wireframe2D.tga
                MeshVisualizerTestFiles/multidraw-wireframe3D.tga
                MeshVisualizerTestFiles/multidraw-wireframe-tbn3D.tga
                MeshVisualizerTestFiles/multidraw-wireframe-nogeo2D.tga
                MeshVisualizerTestFiles/multidraw-wireframe-nogeo3D.tga
                MeshVisualizerTestFiles/multidraw-vertexid2D.tga
                MeshVisualizerTestFiles/multidraw-vertexid3D.tga
                MeshVisualizerTestFiles/multidraw-instancedobjectid2D.tga
                MeshVisualizerTestFiles/multidraw-instancedobjectid3D.tga
                MeshVisualizerTestFiles/multidraw-objectidtexture2D.tga
                MeshVisualizerTestFiles/multidraw-objectidtexture3D.tga
                MeshVisualizerTestFiles/skinning.tga
                MeshVisualizerTestFiles/skinning-default.tga
                MeshVisualizerTestFiles/skinning-instanced.tga
                MeshVisualizerTestFiles/skinning-multi.tga)
        target_include_directories(ShadersMeshVisualizerGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
        if(MAGNUM_BUILD_PLUGINS_STATIC)
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                target_link_libraries(ShadersMeshVisualizerGLTest PRIVATE AnyImageImporter)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                target_link_libraries(ShadersMeshVisualizerGLTest PRIVATE TgaImporter)
            endif()
        else()
            # So the plugins get properly built when building the test
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                add_dependencies(ShadersMeshVisualizerGLTest AnyImageImporter)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                add_dependencies(ShadersMeshVisualizerGLTest TgaImporter)
            endif()
        endif()

        set(ShadersPhongGLTest_SRCS PhongGLTest.cpp)
        if(CORRADE_TARGET_IOS)
            list(APPEND ShadersPhongGLTest_SRCS TestFiles PhongTestFiles FlatTestFiles)
        endif()
        corrade_add_test(ShadersPhongGLTest ${ShadersPhongGLTest_SRCS}
            LIBRARIES
                MagnumDebugTools
                MagnumMeshTools
                MagnumPrimitives
                MagnumShadersTestLib
                MagnumOpenGLTester
            FILES
                TestFiles/alpha-texture.tga
                TestFiles/ambient-texture.tga
                TestFiles/diffuse-alpha-texture.tga
                TestFiles/diffuse-texture.tga
                TestFiles/normal-texture.tga
                TestFiles/specular-texture.tga
                TestFiles/alpha-mask1.0.tga
                TestFiles/skinning.tga
                TestFiles/skinning-default.tga
                TestFiles/skinning-instanced.tga
                TestFiles/skinning-multi.tga

                PhongTestFiles/colored.tga
                PhongTestFiles/defaults.tga
                PhongTestFiles/double-sided.tga
                PhongTestFiles/instanced.tga
                PhongTestFiles/instanced-textured.tga
                PhongTestFiles/instanced-normal.tga
                PhongTestFiles/low-light-angle.tga
                PhongTestFiles/shininess-no-specular.tga
                PhongTestFiles/shininess0-overflow.tga
                PhongTestFiles/shininess0.tga
                PhongTestFiles/shininess10.tga
                PhongTestFiles/shininess80.tga
                PhongTestFiles/textured-ambient.tga
                PhongTestFiles/textured-diffuse-alpha.tga
                PhongTestFiles/textured-diffuse-alpha-mask0.5.tga
                PhongTestFiles/textured-diffuse-alpha.tga
                PhongTestFiles/textured-diffuse.tga
                PhongTestFiles/textured-diffuse-transformed.tga
                PhongTestFiles/textured-normal.tga
                PhongTestFiles/textured-normal-left.tga
                PhongTestFiles/textured-normal0.0.tga
                PhongTestFiles/textured-normal0.5.tga
                PhongTestFiles/textured-specular.tga
                PhongTestFiles/textured.tga
                PhongTestFiles/vertexColor.tga
                PhongTestFiles/light-directional-intensity2.tga
                PhongTestFiles/light-directional.tga
                PhongTestFiles/light-none.tga
                PhongTestFiles/light-point-attenuated-specular.tga
                PhongTestFiles/light-point-intensity10-range1.0.tga
                PhongTestFiles/light-point-range1.5.tga
                PhongTestFiles/light-point-specular-color.tga
                PhongTestFiles/light-point.tga
                PhongTestFiles/multidraw.tga
                PhongTestFiles/multidraw-textured.tga

                # For zero lights test (equivalency to Flat3D)
                FlatTestFiles/textured3D-alpha-mask0.5.tga)
        target_include_directories(ShadersPhongGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
        if(MAGNUM_BUILD_PLUGINS_STATIC)
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                target_link_libraries(ShadersPhongGLTest PRIVATE AnyImageImporter)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                target_link_libraries(ShadersPhongGLTest PRIVATE TgaImporter)
            endif()
        else()
            # So the plugins get properly built when building the test
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                add_dependencies(ShadersPhongGLTest AnyImageImporter)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                add_dependencies(ShadersPhongGLTest TgaImporter)
            endif()
        endif()

        set(ShadersVectorGLTest_SRCS VectorGLTest.cpp)
        if(CORRADE_TARGET_IOS)
            list(APPEND ShadersVectorGLTest_SRCS TestFiles VectorTestFiles)
        endif()
        corrade_add_test(ShadersVectorGLTest ${ShadersVectorGLTest_SRCS}
            LIBRARIES
                MagnumDebugTools
                MagnumMeshTools
                MagnumPrimitives
                MagnumShadersTestLib
                MagnumOpenGLTester
            FILES
                TestFiles/vector.tga

                VectorTestFiles/defaults.tga
                VectorTestFiles/vector2D.tga
                VectorTestFiles/vector3D.tga
                VectorTestFiles/multidraw2D.tga
                VectorTestFiles/multidraw3D.tga)
        target_include_directories(ShadersVectorGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
        if(MAGNUM_BUILD_PLUGINS_STATIC)
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                target_link_libraries(ShadersVectorGLTest PRIVATE AnyImageImporter)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                target_link_libraries(ShadersVectorGLTest PRIVATE TgaImporter)
            endif()
        else()
            # So the plugins get properly built when building the test
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                add_dependencies(ShadersVectorGLTest AnyImageImporter)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                add_dependencies(ShadersVectorGLTest TgaImporter)
            endif()
        endif()

        set(ShadersVertexColorGLTest_SRCS VertexColorGLTest.cpp)
        if(CORRADE_TARGET_IOS)
            list(APPEND ShadersVertexColorGLTest_SRCS FlatTestFiles VertexColorTestFiles)
        endif()
        corrade_add_test(ShadersVertexColorGLTest ${ShadersVertexColorGLTest_SRCS}
            LIBRARIES
                MagnumDebugTools
                MagnumMeshTools
                MagnumPrimitives
                MagnumShadersTestLib
                MagnumOpenGLTester
            FILES
                FlatTestFiles/defaults.tga

                VertexColorTestFiles/vertexColor2D.tga
                VertexColorTestFiles/vertexColor3D.tga
                VertexColorTestFiles/multidraw2D.tga
                VertexColorTestFiles/multidraw3D.tga)
        target_include_directories(ShadersVertexColorGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
        if(MAGNUM_BUILD_PLUGINS_STATIC)
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                target_link_libraries(ShadersVertexColorGLTest PRIVATE AnyImageImporter)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                target_link_libraries(ShadersVertexColorGLTest PRIVATE TgaImporter)
            endif()
        else()
            # So the plugins get properly built when building the test
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                add_dependencies(ShadersVertexColorGLTest AnyImageImporter)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                add_dependencies(ShadersVertexColorGLTest TgaImporter)
            endif()
        endif()

        corrade_add_test(ShadersGLBenchmark ShadersGLBenchmark.cpp
            LIBRARIES
                MagnumDebugTools
                MagnumMeshTools
                MagnumPrimitives
                MagnumShaders
                MagnumOpenGLTester
            FILES
                BenchmarkFiles/trivial.tga)
        target_include_directories(ShadersGLBenchmark PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
        if(MAGNUM_BUILD_PLUGINS_STATIC)
            if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                target_link_libraries(ShadersGLBenchmark PRIVATE AnyImageImporter)
            endif()
            if(MAGNUM_WITH_TGAIMPORTER)
                target_link_libraries(ShadersGLBenchmark PRIVATE TgaImporter)
            endif()
        endif()

        if(CORRADE_TARGET_IOS)
            set_source_files_properties(
                TestFiles
                VectorTestFiles
                FlatTestFiles
                MeshVisualizerTestFiles
                PhongTestFiles
                VectorTestFiles
                VertexColorTestFiles
                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        endif()

        if(NOT MAGNUM_TARGET_GLES2)
            set(ShadersLineGLTest_SRCS LineGLTest.cpp)
            if(CORRADE_TARGET_IOS)
                list(APPEND ShadersLineGLTest_SRCS TestFiles LineTestFiles)
            endif()
            corrade_add_test(ShadersLineGLTest ${ShadersLineGLTest_SRCS}
                LIBRARIES
                    MagnumDebugTools
                    MagnumMeshTools
                    MagnumPrimitives
                    MagnumShadersTestLib
                    MagnumOpenGLTester
                FILES
                    LineTestFiles/caps-butt-joins-bevel.tga
                    LineTestFiles/caps-butt-joins-miter-flat.tga
                    LineTestFiles/caps-round-joins-miter.tga
                    LineTestFiles/caps-square-joins-bevel.tga
                    LineTestFiles/caps-square-joins-miter-flat.tga
                    LineTestFiles/caps-square-joins-miter-limit-36.tga
                    LineTestFiles/caps-square-joins-miter-limit-70deg.tga
                    LineTestFiles/caps-square-joins-miter-limit-91deg.tga
                    LineTestFiles/caps-square-joins-miter.tga
                    LineTestFiles/caps-triangle-joins-bevel.tga
                    LineTestFiles/cube3D-caps-butt-joins-bevel.tga
                    LineTestFiles/cube3D-caps-square-joins-miter.tga
                    LineTestFiles/cube3D-depth.tga
                    LineTestFiles/cube3D-flat-single-pixel.tga
                    LineTestFiles/defaults.tga
                    LineTestFiles/instanced.tga
                    LineTestFiles/multidraw.tga
                    LineTestFiles/perspective3D.tga
                    LineTestFiles/vertex-color.tga)
            target_include_directories(ShadersLineGLTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
            if(MAGNUM_BUILD_PLUGINS_STATIC)
                if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                    target_link_libraries(ShadersLineGLTest PRIVATE AnyImageImporter)
                endif()
                if(MAGNUM_WITH_TGAIMPORTER)
                    target_link_libraries(ShadersLineGLTest PRIVATE TgaImporter)
                endif()
            else()
                # So the plugins get properly built when building the test
                if(MAGNUM_WITH_ANYIMAGEIMPORTER)
                    add_dependencies(ShadersLineGLTest AnyImageImporter)
                endif()
                if(MAGNUM_WITH_TGAIMPORTER)
                    add_dependencies(ShadersLineGLTest TgaImporter)
                endif()
            endif()

            if(CORRADE_TARGET_IOS)
                set_source_files_properties(
                    LineTestFiles
                    PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
            endif()
        endif()
    endif()
endif()
