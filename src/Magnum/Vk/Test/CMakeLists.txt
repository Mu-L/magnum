#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021, 2022, 2023, 2024, 2025
#             Vladimír Vondruš <mosra@centrum.cz>
#   Copyright © 2016 Jonathan Hale <squareys@googlemail.com>
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
set(CMAKE_FOLDER "Magnum/Vk/Test")

# Inputs to configure.h needed only by Vulkan tests
if(MAGNUM_BUILD_VK_TESTS)
    # Otherwise CMake complains that Corrade::PluginManager is not found, wtf
    find_package(Corrade REQUIRED PluginManager)

    if(CORRADE_TARGET_ANDROID)
        set(VK_TEST_DIR ".")
    else()
        set(VK_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if(NOT MAGNUM_BUILD_PLUGINS_STATIC)
        if(MAGNUM_WITH_ANYIMAGEIMPORTER)
            set(ANYIMAGEIMPORTER_PLUGIN_FILENAME $<TARGET_FILE:AnyImageImporter>)
        endif()
        if(MAGNUM_WITH_TGAIMPORTER)
            set(TGAIMPORTER_PLUGIN_FILENAME $<TARGET_FILE:TgaImporter>)
        endif()
    endif()
endif()

# The file is needed by VkShaderTest as well, so it's created always, not just
# for MAGNUM_BUILD_VK_TESTS. First replace ${} variables, then $<> generator
# expressions.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h.in)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/configure.h
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/configure.h.in)

corrade_add_test(VkBufferTest BufferTest.cpp LIBRARIES MagnumVkTestLib)
corrade_add_test(VkCommandBufferTest CommandBufferTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkCommandPoolTest CommandPoolTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkDescriptorPoolTest DescriptorPoolTest.cpp LIBRARIES MagnumVkTestLib)
corrade_add_test(VkDescriptorSetTest DescriptorSetTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkDescriptorSetLayoutTest DescriptorSetLayoutTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkDescriptorTypeTest DescriptorTypeTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkDeviceTest DeviceTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkDevicePropertiesTest DevicePropertiesTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkDeviceFeaturesTest DeviceFeaturesTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkExtensionsTest ExtensionsTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkExtensionPropertiesTest ExtensionPropertiesTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkFenceTest FenceTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkFramebufferTest FramebufferTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkHandleTest HandleTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkImageTest ImageTest.cpp LIBRARIES MagnumVkTestLib)
corrade_add_test(VkImageViewTest ImageViewTest.cpp LIBRARIES MagnumVkTestLib)
corrade_add_test(VkInstanceTest InstanceTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkIntegrationTest IntegrationTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkLayerPropertiesTest LayerPropertiesTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkMemoryTest MemoryTest.cpp LIBRARIES MagnumVkTestLib)
corrade_add_test(VkMeshTest MeshTest.cpp LIBRARIES MagnumVkTestLib)
corrade_add_test(VkMeshLayoutTest MeshLayoutTest.cpp LIBRARIES MagnumVkTestLib)
corrade_add_test(VkPipelineTest PipelineTest.cpp LIBRARIES MagnumVkTestLib)
corrade_add_test(VkPipelineLayoutTest PipelineLayoutTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkPixelFormatTest PixelFormatTest.cpp LIBRARIES MagnumVkTestLib)
corrade_add_test(VkQueueTest QueueTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkResultTest ResultTest.cpp LIBRARIES MagnumVk)
corrade_add_test(VkRenderPassTest RenderPassTest.cpp LIBRARIES MagnumVkTestLib)
corrade_add_test(VkSamplerTest SamplerTest.cpp LIBRARIES MagnumVkTestLib)

corrade_add_test(VkShaderTest ShaderTest.cpp
    LIBRARIES MagnumVk
    FILES ShaderTestFiles/vert-frag.spv)
target_include_directories(VkShaderTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)

corrade_add_test(VkShaderSetTest ShaderSetTest.cpp LIBRARIES MagnumVkTestLib)
corrade_add_test(VkVertexFormatTest VertexFormatTest.cpp LIBRARIES MagnumVkTestLib)

corrade_add_test(VkStructureHelpersTest StructureHelpersTest.cpp)
target_include_directories(VkStructureHelpersTest PRIVATE $<TARGET_PROPERTY:MagnumVk,INTERFACE_INCLUDE_DIRECTORIES>)

add_library(VkAssertTestObjects OBJECT AssertTest.cpp)
target_include_directories(VkAssertTestObjects PRIVATE $<TARGET_PROPERTY:MagnumVk,INTERFACE_INCLUDE_DIRECTORIES>)
corrade_add_test(VkAssertTest
    $<TARGET_OBJECTS:VkAssertTestObjects>
    ${PROJECT_SOURCE_DIR}/src/dummy.cpp # XCode workaround, see file comment for details
    LIBRARIES MagnumVk)
# WILL_FAIL doesn't work for abort() on desktop, test this only on embedded
# then. Oh well. Also the tests could be just one executable added multiple
# times with different arguments, but corrade_add_test() doesn't support that,
# so I'm doing it at least via an OBJECT library.
if(CORRADE_TARGET_ANDROID)
    corrade_add_test(VkAssertTestFailAssertSuccess
        $<TARGET_OBJECTS:VkAssertTestObjects>
        ${PROJECT_SOURCE_DIR}/src/dummy.cpp # XCode workaround, see file comment for details
        ARGUMENTS --fail-on-assert-success true
        LIBRARIES MagnumVk)
    set_tests_properties(VkAssertTestFailAssertSuccess PROPERTIES
        PASS_REGULAR_EXPRESSION "Call a = r failed with Vk::Result::ErrorFragmentedPool at ")
    corrade_add_test(VkAssertTestFailAssertSuccessOr
        $<TARGET_OBJECTS:VkAssertTestObjects>
        ${PROJECT_SOURCE_DIR}/src/dummy.cpp # XCode workaround, see file comment for details
        ARGUMENTS --fail-on-assert-success-or true
        LIBRARIES MagnumVk)
    set_tests_properties(VkAssertTestFailAssertSuccessOr PROPERTIES
        PASS_REGULAR_EXPRESSION "Call a = r failed with Vk::Result::ErrorExtensionNotPresent at ")
    corrade_add_test(VkAssertTestFailAssertVkSuccess
        $<TARGET_OBJECTS:VkAssertTestObjects>
        ${PROJECT_SOURCE_DIR}/src/dummy.cpp # XCode workaround, see file comment for details
        ARGUMENTS --fail-on-assert-vk-success true
        LIBRARIES MagnumVk)
    set_tests_properties(VkAssertTestFailAssertVkSuccess PROPERTIES
        PASS_REGULAR_EXPRESSION "Call a = s failed with Vk::Result::ErrorFragmentedPool at ")
    corrade_add_test(VkAssertTestFailAssertVkSuccessOr
        $<TARGET_OBJECTS:VkAssertTestObjects>
        ${PROJECT_SOURCE_DIR}/src/dummy.cpp # XCode workaround, see file comment for details
        ARGUMENTS --fail-on-assert-vk-success-or true
        LIBRARIES MagnumVk)
    set_tests_properties(VkAssertTestFailAssertVkSuccessOr PROPERTIES
        PASS_REGULAR_EXPRESSION "Call a = s failed with Vk::Result::ErrorExtensionNotPresent at ")
endif()

# The same as above, but using CORRADE_STANDARD_ASSERT
corrade_add_test(VkAssertStandardTest AssertTest.cpp LIBRARIES MagnumVk)
target_compile_definitions(VkAssertStandardTest PRIVATE
    "CORRADE_STANDARD_ASSERT")

corrade_add_test(VkAssertDisabledTest AssertDisabledTest.cpp LIBRARIES MagnumVk)
# The same as above, but using CORRADE_STANDARD_ASSERT and NDEBUG
corrade_add_test(VkAssertStandardDisabledTest AssertDisabledTest.cpp LIBRARIES MagnumVk)
target_compile_definitions(VkAssertStandardDisabledTest PRIVATE
    "CORRADE_STANDARD_ASSERT" "NDEBUG")

corrade_add_test(VkVersionTest VersionTest.cpp LIBRARIES MagnumVk)

if(MAGNUM_BUILD_VK_TESTS)
    corrade_add_test(VkBufferVkTest BufferVkTest.cpp LIBRARIES MagnumVkTestLib MagnumVulkanTester)
    corrade_add_test(VkCommandBufferVkTest CommandBufferVkTest.cpp LIBRARIES MagnumVulkanTester)
    corrade_add_test(VkCommandPoolVkTest CommandPoolVkTest.cpp LIBRARIES MagnumVulkanTester)
    corrade_add_test(VkDescriptorPoolVkTest DescriptorPoolVkTest.cpp LIBRARIES MagnumVkTestLib MagnumVulkanTester)
    corrade_add_test(VkDescriptorSetVkTest DescriptorSetVkTest.cpp LIBRARIES MagnumVk MagnumVulkanTester)
    corrade_add_test(VkDescriptorSetLayoutVkTest DescriptorSetLayoutVkTest.cpp LIBRARIES MagnumVk MagnumVulkanTester)
    corrade_add_test(VkDeviceVkTest DeviceVkTest.cpp LIBRARIES MagnumVkTestLib MagnumVulkanTester)
    corrade_add_test(VkDevicePropertiesVkTest DevicePropertiesVkTest.cpp LIBRARIES  MagnumVkTestLib MagnumVulkanTester)
    corrade_add_test(VkExtensionPropertiesVkTest ExtensionPropertiesVkTest.cpp LIBRARIES MagnumVkTestLib)
    corrade_add_test(VkFenceVkTest FenceVkTest.cpp LIBRARIES MagnumVk MagnumVulkanTester)
    corrade_add_test(VkFramebufferVkTest FramebufferVkTest.cpp LIBRARIES MagnumVk MagnumVulkanTester)
    corrade_add_test(VkLayerPropertiesVkTest LayerPropertiesVkTest.cpp LIBRARIES MagnumVkTestLib)
    corrade_add_test(VkImageVkTest ImageVkTest.cpp LIBRARIES MagnumVkTestLib MagnumDebugTools MagnumVulkanTester)
    corrade_add_test(VkImageViewVkTest ImageViewVkTest.cpp LIBRARIES MagnumVk MagnumVulkanTester)
    corrade_add_test(VkInstanceVkTest InstanceVkTest.cpp LIBRARIES MagnumVkTestLib)
    corrade_add_test(VkMemoryVkTest MemoryVkTest.cpp LIBRARIES MagnumVk MagnumVulkanTester)

    corrade_add_test(VkMeshVkTest MeshVkTest.cpp
        LIBRARIES MagnumVkTestLib MagnumDebugTools MagnumVulkanTester
        FILES
            MeshTestFiles/flat.spv
            MeshTestFiles/flat.tga
            MeshTestFiles/noop.spv
            MeshTestFiles/noop.tga
            MeshTestFiles/vertexcolor.spv
            MeshTestFiles/vertexcolor.tga)
    target_include_directories(VkMeshVkTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
    if(MAGNUM_BUILD_PLUGINS_STATIC)
        if(MAGNUM_WITH_ANYIMAGEIMPORTER)
            target_link_libraries(VkMeshVkTest PRIVATE AnyImageImporter)
        endif()
        if(MAGNUM_WITH_TGAIMPORTER)
            target_link_libraries(VkMeshVkTest PRIVATE TgaImporter)
        endif()
    else()
        # So the plugins get properly built when building the test
        if(MAGNUM_WITH_ANYIMAGEIMPORTER)
            add_dependencies(VkMeshVkTest AnyImageImporter)
        endif()
        if(MAGNUM_WITH_TGAIMPORTER)
            add_dependencies(VkMeshVkTest TgaImporter)
        endif()
    endif()

    corrade_add_test(VkPipelineVkTest PipelineVkTest.cpp
        LIBRARIES MagnumVkTestLib MagnumVulkanTester
        FILES triangle-shaders.spv compute-noop.spv)
    target_include_directories(VkPipelineVkTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)

    corrade_add_test(VkPipelineLayoutVkTest PipelineLayoutVkTest.cpp LIBRARIES MagnumVk MagnumVulkanTester)
    corrade_add_test(VkQueueVkTest QueueVkTest.cpp LIBRARIES MagnumVk MagnumVulkanTester)
    corrade_add_test(VkRenderPassVkTest RenderPassVkTest.cpp LIBRARIES MagnumVkTestLib MagnumVulkanTester)
    corrade_add_test(VkSamplerVkTest SamplerVkTest.cpp LIBRARIES MagnumVkTestLib MagnumVulkanTester)

    corrade_add_test(VkShaderVkTest ShaderVkTest.cpp
        LIBRARIES MagnumVk MagnumVulkanTester
        FILES triangle-shaders.spv)
    target_include_directories(VkShaderVkTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)

    corrade_add_test(VkVersionVkTest VersionVkTest.cpp LIBRARIES MagnumVk)
endif()
