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
set(CMAKE_FOLDER "Magnum/ShaderTools")

find_package(Corrade REQUIRED PluginManager)

# Files shared between main library and unit test library
set(MagnumShaderTools_SRCS )

# Files compiled with different flags for main library and unit test library
set(MagnumShaderTools_GracefulAssert_SRCS
    AbstractConverter.cpp
    Stage.cpp)

set(MagnumShaderTools_HEADERS
    AbstractConverter.h
    ShaderTools.h
    Stage.h

    visibility.h)

set(MagnumShaderTools_PRIVATE_HEADERS
    Implementation/spirv.h)

if(NOT CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
                   ${CMAKE_CURRENT_BINARY_DIR}/configure.h)
endif()

# # Objects shared between main and test library
# add_library(MagnumShaderToolsObjects OBJECT
#     ${MagnumShaderTools_SRCS}
#     ${MagnumShaderTools_HEADERS}
#     ${MagnumShaderTools_PRIVATE_HEADERS})
# target_include_directories(MagnumShaderToolsObjects PUBLIC $<TARGET_PROPERTY:Magnum,INTERFACE_INCLUDE_DIRECTORIES>)
# if(NOT MAGNUM_BUILD_STATIC)
#     target_compile_definitions(MagnumShaderToolsObjects PRIVATE "MagnumShaderToolsObjects_EXPORTS")
# endif()
# if(NOT MAGNUM_BUILD_STATIC OR MAGNUM_BUILD_STATIC_PIC)
#     set_target_properties(MagnumShaderToolsObjects PROPERTIES POSITION_INDEPENDENT_CODE ON)
# endif()

# Main ShaderTools library
add_library(MagnumShaderTools ${SHARED_OR_STATIC}
#     $<TARGET_OBJECTS:MagnumShaderToolsObjects>
    ${MagnumShaderTools_GracefulAssert_SRCS}
    ${MagnumShaderTools_HEADERS}
    ${MagnumShaderTools_PRIVATE_HEADERS})
set_target_properties(MagnumShaderTools PROPERTIES DEBUG_POSTFIX "-d")
if(NOT MAGNUM_BUILD_STATIC)
    set_target_properties(MagnumShaderTools PROPERTIES VERSION ${MAGNUM_LIBRARY_VERSION} SOVERSION ${MAGNUM_LIBRARY_SOVERSION})
elseif(MAGNUM_BUILD_STATIC_PIC)
    set_target_properties(MagnumShaderTools PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_link_libraries(MagnumShaderTools PUBLIC
    Magnum
    Corrade::PluginManager)

install(TARGETS MagnumShaderTools
    RUNTIME DESTINATION ${MAGNUM_BINARY_INSTALL_DIR}
    LIBRARY DESTINATION ${MAGNUM_LIBRARY_INSTALL_DIR}
    ARCHIVE DESTINATION ${MAGNUM_LIBRARY_INSTALL_DIR})
install(FILES ${MagnumShaderTools_HEADERS} DESTINATION ${MAGNUM_INCLUDE_INSTALL_DIR}/ShaderTools)

if(MAGNUM_WITH_SHADERCONVERTER)
    find_package(Corrade REQUIRED Main)

    add_executable(magnum-shaderconverter shaderconverter.cpp)
    target_link_libraries(magnum-shaderconverter PRIVATE
        Corrade::Main
        Magnum
        MagnumShaderTools
        ${MAGNUM_SHADERCONVERTER_STATIC_PLUGINS})

    install(TARGETS magnum-shaderconverter DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})

    # Magnum shaderconverter target alias for superprojects
    add_executable(Magnum::shaderconverter ALIAS magnum-shaderconverter)
endif()

if(MAGNUM_BUILD_TESTS)
    # Library with graceful assert for testing
    add_library(MagnumShaderToolsTestLib ${SHARED_OR_STATIC} ${EXCLUDE_FROM_ALL_IF_TEST_TARGET}
#         $<TARGET_OBJECTS:MagnumShaderToolsObjects>
        ${MagnumShaderTools_GracefulAssert_SRCS})
    set_target_properties(MagnumShaderToolsTestLib PROPERTIES DEBUG_POSTFIX "-d")
    target_compile_definitions(MagnumShaderToolsTestLib PRIVATE
        "CORRADE_GRACEFUL_ASSERT" "MagnumShaderTools_EXPORTS")
    if(MAGNUM_BUILD_STATIC_PIC)
        set_target_properties(MagnumShaderToolsTestLib PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()
    target_link_libraries(MagnumShaderToolsTestLib PUBLIC
        Magnum
        Corrade::PluginManager)

    add_subdirectory(Test ${EXCLUDE_FROM_ALL_IF_TEST_TARGET})
endif()

# Magnum ShaderTools target alias for superprojects
add_library(Magnum::ShaderTools ALIAS MagnumShaderTools)
