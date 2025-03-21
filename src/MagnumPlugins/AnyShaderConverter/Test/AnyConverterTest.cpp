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
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/String.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/Path.h>

#include "Magnum/ShaderTools/AbstractConverter.h"
#include "Magnum/ShaderTools/Stage.h"

#include "configure.h"

namespace Magnum { namespace ShaderTools { namespace Test { namespace {

struct AnyConverterTest: TestSuite::Tester {
    explicit AnyConverterTest();

    void validateFile();
    void validateFilePluginLoadFailed();
    void validateFileUnknown();
    void validateFileNotSupported();
    void validateFilePreprocessNotSupported();
    void validateFilePropagateFlags();
    void validateFilePropagateInputVersion();
    void validateFilePropagateOutputVersion();
    void validateFilePropagatePreprocess();
    void validateFilePropagateConfiguration();
    void validateFilePropagateConfigurationUnknown();

    void validateData();
    void validateDataPluginLoadFailed();
    void validateDataNoFormatSet();
    void validateDataNotSupported();
    void validateDataPreprocessNotSupported();
    void validateDataPropagateFlags();
    void validateDataPropagateInputVersion();
    void validateDataPropagateOutputVersion();
    void validateDataPropagatePreprocess();
    void validateDataPropagateConfiguration();
    void validateDataPropagateConfigurationUnknown();

    void convertFileToFile();
    void convertFileToFilePluginLoadFailed();
    void convertFileToFileUnknownInput();
    void convertFileToFileUnknownOutput();
    void convertFileToFileNotSupported();
    void convertFileToFilePreprocessNotSupported();
    void convertFileToFileDebugInfoNotSupported();
    void convertFileToFileOptimizationNotSupported();
    void convertFileToFilePropagateFlags();
    void convertFileToFilePropagateInputVersion();
    void convertFileToFilePropagateOutputVersion();
    void convertFileToFilePropagatePreprocess();
    void convertFileToFilePropagateDebugInfo();
    void convertFileToFilePropagateOptimization();
    void convertFileToFilePropagateConfiguration();
    void convertFileToFilePropagateConfigurationUnknown();

    void convertFileToData();
    void convertFileToDataPluginLoadFailed();
    void convertFileToDataUnknown();
    void convertFileToDataNoFormatSet();
    void convertFileToDataNotSupported();
    void convertFileToDataPreprocessNotSupported();
    void convertFileToDataDebugInfoNotSupported();
    void convertFileToDataOptimizationNotSupported();
    void convertFileToDataPropagateFlags();
    void convertFileToDataPropagateInputVersion();
    void convertFileToDataPropagateOutputVersion();
    void convertFileToDataPropagatePreprocess();
    void convertFileToDataPropagateDebugInfo();
    void convertFileToDataPropagateOptimization();
    void convertFileToDataPropagateConfiguration();
    void convertFileToDataPropagateConfigurationUnknown();

    void convertDataToData();
    void convertDataToDataPluginLoadFailed();
    void convertDataToDataNoInputFormatSet();
    void convertDataToDataNoOutputFormatSet();
    void convertDataToDataNotSupported();
    void convertDataToDataPreprocessNotSupported();
    void convertDataToDataDebugInfoNotSupported();
    void convertDataToDataOptimizationNotSupported();
    void convertDataToDataPropagateFlags();
    void convertDataToDataPropagateInputVersion();
    void convertDataToDataPropagateOutputVersion();
    void convertDataToDataPropagatePreprocess();
    void convertDataToDataPropagateDebugInfo();
    void convertDataToDataPropagateOptimization();
    void convertDataToDataPropagateConfiguration();
    void convertDataToDataPropagateConfigurationUnknown();

    void detectValidate();
    void detectValidateExplicitFormat();
    void detectConvert();
    void detectConvertExplicitFormat();

    /* Explicitly forbid system-wide plugin dependencies. Tests that need those
       have their own manager. */
    PluginManager::Manager<AbstractConverter> _manager{"nonexistent"};
};

const struct {
    const char* name;
    ConverterFlags flags;
    bool quiet;
} PropagateConfigurationUnknownData[]{
    {"", {}, false},
    {"quiet", ConverterFlag::Quiet, true}
};

constexpr struct {
    const char* name;
    const char* filename;
    const char* plugin;
} DetectValidateData[]{
    {"SPIR-V", "flat.spv", "SpirvShaderConverter"},
    {"SPIR-V assembly uppercase", "DOOM.SPVASM", "SpirvAssemblyShaderConverter"},
    {"SPIR-V assembly weird", "test.asm.rahit", "SpirvAssemblyShaderConverter"},
    {"GLSL explicit", "phong.glsl", "GlslShaderConverter"},
    {"GLSL implicit", "phong.frag", "GlslShaderConverter"},
};

constexpr struct {
    const char* name;
    const char* from;
    const char* to;
    const char* plugin;
} DetectConvertData[]{
    {"SPIR-V to SPIR-V", "flat.spv", "optimized.spv", "SpirvShaderConverter"},
    {"SPIR-V assembly to SPIR-V", "a.spvasm", "b.spv", "SpirvAssemblyToSpirvShaderConverter"},
    {"SPIR-V to GLSL", "phong.frag.spv", "phong.glsl", "SpirvToGlslShaderConverter"}
};

AnyConverterTest::AnyConverterTest() {
    addTests({&AnyConverterTest::validateFile,
              &AnyConverterTest::validateFilePluginLoadFailed,
              &AnyConverterTest::validateFileUnknown,
              &AnyConverterTest::validateFileNotSupported,
              &AnyConverterTest::validateFilePreprocessNotSupported,
              &AnyConverterTest::validateFilePropagateFlags,
              &AnyConverterTest::validateFilePropagateInputVersion,
              &AnyConverterTest::validateFilePropagateOutputVersion,
              &AnyConverterTest::validateFilePropagatePreprocess,
              &AnyConverterTest::validateFilePropagateConfiguration});

    addInstancedTests({&AnyConverterTest::validateFilePropagateConfigurationUnknown},
        Containers::arraySize(PropagateConfigurationUnknownData));

    addTests({&AnyConverterTest::validateData,
              &AnyConverterTest::validateDataPluginLoadFailed,
              &AnyConverterTest::validateDataNoFormatSet,
              &AnyConverterTest::validateDataNotSupported,
              &AnyConverterTest::validateDataPreprocessNotSupported,
              &AnyConverterTest::validateDataPropagateFlags,
              &AnyConverterTest::validateDataPropagateInputVersion,
              &AnyConverterTest::validateDataPropagateOutputVersion,
              &AnyConverterTest::validateDataPropagatePreprocess,
              &AnyConverterTest::validateDataPropagateConfiguration});

    addInstancedTests({&AnyConverterTest::validateDataPropagateConfigurationUnknown},
        Containers::arraySize(PropagateConfigurationUnknownData));

    addTests({&AnyConverterTest::convertFileToFile,
              &AnyConverterTest::convertFileToFilePluginLoadFailed,
              &AnyConverterTest::convertFileToFileUnknownInput,
              &AnyConverterTest::convertFileToFileUnknownOutput,
              &AnyConverterTest::convertFileToFileNotSupported,
              &AnyConverterTest::convertFileToFilePreprocessNotSupported,
              &AnyConverterTest::convertFileToFileDebugInfoNotSupported,
              &AnyConverterTest::convertFileToFileOptimizationNotSupported,
              &AnyConverterTest::convertFileToFilePropagateFlags,
              &AnyConverterTest::convertFileToFilePropagateInputVersion,
              &AnyConverterTest::convertFileToFilePropagateOutputVersion,
              &AnyConverterTest::convertFileToFilePropagatePreprocess,
              &AnyConverterTest::convertFileToFilePropagateDebugInfo,
              &AnyConverterTest::convertFileToFilePropagateOptimization,
              &AnyConverterTest::convertFileToFilePropagateConfiguration});

    addInstancedTests({&AnyConverterTest::convertFileToFilePropagateConfigurationUnknown},
        Containers::arraySize(PropagateConfigurationUnknownData));

    addTests({&AnyConverterTest::convertFileToData,
              &AnyConverterTest::convertFileToDataPluginLoadFailed,
              &AnyConverterTest::convertFileToDataUnknown,
              &AnyConverterTest::convertFileToDataNoFormatSet,
              &AnyConverterTest::convertFileToDataNotSupported,
              &AnyConverterTest::convertFileToDataPreprocessNotSupported,
              &AnyConverterTest::convertFileToDataDebugInfoNotSupported,
              &AnyConverterTest::convertFileToDataOptimizationNotSupported,
              &AnyConverterTest::convertFileToDataPropagateFlags,
              &AnyConverterTest::convertFileToDataPropagateInputVersion,
              &AnyConverterTest::convertFileToDataPropagateOutputVersion,
              &AnyConverterTest::convertFileToDataPropagatePreprocess,
              &AnyConverterTest::convertFileToDataPropagateDebugInfo,
              &AnyConverterTest::convertFileToDataPropagateOptimization,
              &AnyConverterTest::convertFileToDataPropagateConfiguration});

    addInstancedTests({&AnyConverterTest::convertFileToDataPropagateConfigurationUnknown},
        Containers::arraySize(PropagateConfigurationUnknownData));

    addTests({&AnyConverterTest::convertDataToData,
              &AnyConverterTest::convertDataToDataPluginLoadFailed,
              &AnyConverterTest::convertDataToDataNoInputFormatSet,
              &AnyConverterTest::convertDataToDataNoOutputFormatSet,
              &AnyConverterTest::convertDataToDataNotSupported,
              &AnyConverterTest::convertDataToDataPreprocessNotSupported,
              &AnyConverterTest::convertDataToDataDebugInfoNotSupported,
              &AnyConverterTest::convertDataToDataOptimizationNotSupported,
              &AnyConverterTest::convertDataToDataPropagateFlags,
              &AnyConverterTest::convertDataToDataPropagateInputVersion,
              &AnyConverterTest::convertDataToDataPropagateOutputVersion,
              &AnyConverterTest::convertDataToDataPropagatePreprocess,
              &AnyConverterTest::convertDataToDataPropagateDebugInfo,
              &AnyConverterTest::convertDataToDataPropagateOptimization,
              &AnyConverterTest::convertDataToDataPropagateConfiguration});

    addInstancedTests({&AnyConverterTest::convertDataToDataPropagateConfigurationUnknown},
        Containers::arraySize(PropagateConfigurationUnknownData));

    addInstancedTests({&AnyConverterTest::detectValidate},
        Containers::arraySize(DetectValidateData));

    addTests({&AnyConverterTest::detectValidateExplicitFormat});

    addInstancedTests({&AnyConverterTest::detectConvert},
        Containers::arraySize(DetectConvertData));

    addTests({&AnyConverterTest::detectConvertExplicitFormat});

    /* Load the plugin directly from the build tree. Otherwise it's static and
       already loaded. */
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_INTERNAL_ASSERT_OUTPUT(_manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    /* Create the output directory if it doesn't exist yet */
    CORRADE_INTERNAL_ASSERT_OUTPUT(Utility::Path::make(ANYSHADERCONVERTER_TEST_OUTPUT_DIR));
}

void AnyConverterTest::validateFile() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    Containers::String filename = Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl");

    /* Make it print a warning so we know it's doing something */
    CORRADE_COMPARE(converter->validateFile(Stage::Fragment, filename),
        Containers::pair(true, Utility::format("WARNING: {}:10: 'reserved__identifier' : identifiers containing consecutive underscores (\"__\") are reserved", filename)));
}

void AnyConverterTest::validateFilePluginLoadFailed() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_COMPARE(converter->validateFile({}, "file.glsl"),
        Containers::pair(false, Containers::String{}));
    #ifndef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin GlslShaderConverter is not static and was not found in nonexistent\n"
        "ShaderTools::AnyConverter::validateFile(): cannot load the GlslShaderConverter plugin\n");
    #else
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin GlslShaderConverter was not found\n"
        "ShaderTools::AnyConverter::validateFile(): cannot load the GlslShaderConverter plugin\n");
    #endif
}

void AnyConverterTest::validateFileUnknown() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_COMPARE(converter->validateFile({}, "dead.cg"),
        Containers::pair(false, Containers::String{}));
    CORRADE_COMPARE(out, "ShaderTools::AnyConverter::validateFile(): cannot determine the format of dead.cg\n");
}

void AnyConverterTest::validateFileNotSupported() {
    CORRADE_SKIP("No plugin that would support just validation exists.");
}

void AnyConverterTest::validateFilePreprocessNotSupported() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("SpirvToolsShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("SpirvToolsShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setDefinitions({
        {"DEFINE", "hahahahah"}
    });

    Containers::String out;
    Error redirectError{&out};
    CORRADE_COMPARE(converter->validateFile({}, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.spv")),
        Containers::pair(false, Containers::String{}));
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::validateFile(): SpirvToolsShaderConverter does not support preprocessing\n");
}

void AnyConverterTest::validateFilePropagateFlags() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    Containers::String filename = Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl");

    /* With this, the warning should turn into an error. The converter should
       also print the verbose info. */
    converter->setFlags(ConverterFlag::Verbose|ConverterFlag::WarningAsError);

    Containers::String out;
    Debug redirectDebug{&out};
    CORRADE_COMPARE(converter->validateFile(Stage::Fragment, filename),
        Containers::pair(false, Utility::format("WARNING: {}:10: 'reserved__identifier' : identifiers containing consecutive underscores (\"__\") are reserved", filename)));
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::validateFile(): using GlslShaderConverter (provided by GlslangShaderConverter)\n");
}

void AnyConverterTest::validateFilePropagateInputVersion() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    /* This is an invalid version. We have to supply a valid file path because
       the version gets checked in doValidateData(), called from
       AbstractConverter::doValidateFile() with the file contents. */
    converter->setInputFormat(Format::Glsl, "100");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_COMPARE(converter->validateFile(Stage::Fragment, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl")),
        Containers::pair(false, Containers::String{}));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::validateData(): input format version should be one of supported GLSL #version strings but got 100\n");
}

void AnyConverterTest::validateFilePropagateOutputVersion() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    /* This is an invalid version. We have to supply a valid file path because
       the version gets checked in doValidateData(), called from
       AbstractConverter::doValidateFile() with the file contents. */
    converter->setOutputFormat(Format::Glsl, "opengl4.0");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_COMPARE(converter->validateFile(Stage::Fragment, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl")),
        Containers::pair(false, Containers::String{}));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::validateData(): output format should be Spirv or Unspecified but got ShaderTools::Format::Glsl\n");
}

void AnyConverterTest::validateFilePropagatePreprocess() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    Containers::String filename = Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl");

    /* Check that undefining works properly -- if it stays defined, the source
       won't compile */
    converter->setDefinitions({
        {"SHOULD_BE_UNDEFINED", "really"},
        {"SHOULD_BE_UNDEFINED", nullptr},
        {"reserved__identifier", "different__but_also_wrong"}
    });

    CORRADE_COMPARE(converter->validateFile(Stage::Fragment, filename),
        Containers::pair(true, Utility::format("WARNING: {}:10: 'different__but_also_wrong' : identifiers containing consecutive underscores (\"__\") are reserved", filename)));
}

void AnyConverterTest::validateFilePropagateConfiguration() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    Containers::String filename = Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "version-not-first.glsl");

    {
        CORRADE_COMPARE(converter->validateFile(Stage::Fragment, filename),
            Containers::pair(false, Utility::format("ERROR: {}:2: '#version' : must occur first in shader \nERROR: 1 compilation errors.  No code generated.", filename)));
    } {
        converter->configuration().setValue("permissive", true);
        /* Lol stupid thing, apparently it has two differently worded messages
           for the same thing? Dumpster fire. */
        CORRADE_COMPARE(converter->validateFile(Stage::Fragment, filename),
            Containers::pair(true, Containers::String{"WARNING: 0:0: '#version' : Illegal to have non-comment, non-whitespace tokens before #version"}));
    }
}

void AnyConverterTest::validateFilePropagateConfigurationUnknown() {
    auto&& data = PropagateConfigurationUnknownData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");
    converter->configuration().setValue("noSuchOption", "isHere");
    /* So it doesn't warn about anything */
    converter->setDefinitions({{"reserved__identifier", "sorry"}});
    converter->setFlags(data.flags);

    Containers::String out;
    Warning redirectWarning{&out};
    CORRADE_COMPARE(converter->validateFile(Stage::Fragment, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl")),
        Containers::pair(true, Containers::String{}));
    if(data.quiet)
        CORRADE_COMPARE(out, "");
    else
        CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::validateFile(): option noSuchOption not recognized by GlslangShaderConverter\n");
}

void AnyConverterTest::validateData() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Glsl);

    /* Make it print a warning so we know it's doing something */
    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(data);
    CORRADE_COMPARE(converter->validateData(Stage::Fragment, *data),
        Containers::pair(true, Containers::String{"WARNING: 0:10: 'reserved__identifier' : identifiers containing consecutive underscores (\"__\") are reserved"}));
}

void AnyConverterTest::validateDataPluginLoadFailed() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Glsl);

    Containers::String out;
    Error redirectError{&out};
    CORRADE_COMPARE(converter->validateData({}, {}),
        Containers::pair(false, Containers::String{}));
    #ifndef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin GlslShaderConverter is not static and was not found in nonexistent\n"
        "ShaderTools::AnyConverter::validateData(): cannot load the GlslShaderConverter plugin\n");
    #else
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin GlslShaderConverter was not found\n"
        "ShaderTools::AnyConverter::validateData(): cannot load the GlslShaderConverter plugin\n");
    #endif
}

void AnyConverterTest::validateDataNoFormatSet() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_COMPARE(converter->validateData({}, "dead.cg"),
        Containers::pair(false, Containers::String{}));
    CORRADE_COMPARE(out, "ShaderTools::AnyConverter::validateData(): no input format specified\n");
}

void AnyConverterTest::validateDataNotSupported() {
    CORRADE_SKIP("No plugin that would support just validation exists.");
}

void AnyConverterTest::validateDataPreprocessNotSupported() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("SpirvToolsShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("SpirvToolsShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Spirv);

    converter->setDefinitions({
        {"DEFINE", "hahahahah"}
    });

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.spv"));
    CORRADE_VERIFY(data);

    Containers::String out;
    Error redirectError{&out};
    CORRADE_COMPARE(converter->validateData({}, *data),
        Containers::pair(false, Containers::String{}));
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::validateData(): SpirvToolsShaderConverter does not support preprocessing\n");
}

void AnyConverterTest::validateDataPropagateFlags() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Glsl);

    /* With this, the warning should turn into an error. The converter should
       also print the verbose info. */
    converter->setFlags(ConverterFlag::Verbose|ConverterFlag::WarningAsError);

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(data);

    Containers::String out;
    Debug redirectDebug{&out};
    CORRADE_COMPARE(converter->validateData(Stage::Fragment, *data),
        Containers::pair(false, Containers::String{"WARNING: 0:10: 'reserved__identifier' : identifiers containing consecutive underscores (\"__\") are reserved"}));
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::validateData(): using GlslShaderConverter (provided by GlslangShaderConverter)\n");
}

void AnyConverterTest::validateDataPropagateInputVersion() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Glsl);

    /* This is an invalid version. We have to supply a valid file path because
       the version gets checked in doValidateData(), called from
       AbstractConverter::doValidateFile() with the file contents. */
    converter->setInputFormat(Format::Glsl, "100");

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(data);

    Containers::String out;
    Error redirectError{&out};
    CORRADE_COMPARE(converter->validateData(Stage::Fragment, *data),
        Containers::pair(false, Containers::String{}));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::validateData(): input format version should be one of supported GLSL #version strings but got 100\n");
}

void AnyConverterTest::validateDataPropagateOutputVersion() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Glsl);

    /* This is an invalid version. We have to supply a valid file path because
       the version gets checked in doValidateData(), called from
       AbstractConverter::doValidateFile() with the file contents. */
    converter->setOutputFormat(Format::Glsl, "opengl4.0");

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(data);

    Containers::String out;
    Error redirectError{&out};
    CORRADE_COMPARE(converter->validateData(Stage::Fragment, *data),
        Containers::pair(false, Containers::String{}));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::validateData(): output format should be Spirv or Unspecified but got ShaderTools::Format::Glsl\n");
}

void AnyConverterTest::validateDataPropagatePreprocess() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Glsl);

    /* Check that undefining works properly -- if it stays defined, the source
       won't compile */
    converter->setDefinitions({
        {"SHOULD_BE_UNDEFINED", "really"},
        {"SHOULD_BE_UNDEFINED", nullptr},
        {"reserved__identifier", "different__but_also_wrong"}
    });

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(data);

    CORRADE_COMPARE(converter->validateData(Stage::Fragment, *data),
        Containers::pair(true, Containers::String{"WARNING: 0:10: 'different__but_also_wrong' : identifiers containing consecutive underscores (\"__\") are reserved"}));
}

void AnyConverterTest::validateDataPropagateConfiguration() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");
    converter->setInputFormat(Format::Glsl);

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "version-not-first.glsl"));
    CORRADE_VERIFY(data);

    {
        CORRADE_COMPARE(converter->validateData(Stage::Fragment, *data),
            Containers::pair(false, Containers::String{"ERROR: 0:2: '#version' : must occur first in shader \nERROR: 1 compilation errors.  No code generated."}));
    } {
        converter->configuration().setValue("permissive", true);
        /* Lol stupid thing, apparently it has two differently worded messages
           for the same thing? Dumpster fire. */
        CORRADE_COMPARE(converter->validateData(Stage::Fragment, *data),
            Containers::pair(true, Utility::format("WARNING: 0:0: '#version' : Illegal to have non-comment, non-whitespace tokens before #version")));
    }
}

void AnyConverterTest::validateDataPropagateConfigurationUnknown() {
    auto&& data = PropagateConfigurationUnknownData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");
    converter->setInputFormat(Format::Glsl);
    converter->configuration().setValue("noSuchOption", "isHere");
    /* So it doesn't warn about anything */
    converter->setDefinitions({{"reserved__identifier", "sorry"}});
    converter->setFlags(data.flags);

    Containers::Optional<Containers::Array<char>> shaderData = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(shaderData);

    Containers::String out;
    Warning redirectWarning{&out};
    CORRADE_COMPARE(converter->validateData(Stage::Fragment, *shaderData),
        Containers::pair(true, Containers::String{}));
    if(data.quiet)
        CORRADE_COMPARE(out, "");
    else
        CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::validateData(): option noSuchOption not recognized by GlslangShaderConverter\n");
}

void AnyConverterTest::convertFileToFile() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    Containers::String inputFilename = Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl");
    Containers::String outputFilename = Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.spv");
    if(Utility::Path::exists(outputFilename))
        CORRADE_VERIFY(Utility::Path::remove(outputFilename));

    /* Make it print a warning so we know it's doing something */
    Containers::String out;
    Warning redirectWarning{&out};
    CORRADE_VERIFY(converter->convertFileToFile(Stage::Fragment, inputFilename, outputFilename));
    CORRADE_VERIFY(Utility::Path::exists(outputFilename));
    CORRADE_COMPARE(out, Utility::format(
        "ShaderTools::GlslangConverter::convertDataToData(): compilation succeeded with the following message:\n"
        "WARNING: {}:10: 'reserved__identifier' : identifiers containing consecutive underscores (\"__\") are reserved\n", inputFilename));
}

void AnyConverterTest::convertFileToFilePluginLoadFailed() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToFile({}, "file.spv", "file.glsl"));
    #ifndef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin SpirvToGlslShaderConverter is not static and was not found in nonexistent\n"
        "ShaderTools::AnyConverter::convertFileToFile(): cannot load the SpirvToGlslShaderConverter plugin\n");
    #else
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin SpirvToGlslShaderConverter was not found\n"
        "ShaderTools::AnyConverter::convertFileToFile(): cannot load the SpirvToGlslShaderConverter plugin\n");
    #endif
}

void AnyConverterTest::convertFileToFileUnknownInput() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToFile({}, "dead.cg", "whatever.osl"));
    CORRADE_COMPARE(out, "ShaderTools::AnyConverter::convertFileToFile(): cannot determine the format of dead.cg\n");
}

void AnyConverterTest::convertFileToFileUnknownOutput() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToFile({}, "file.spv", "whatever.osl"));
    CORRADE_COMPARE(out, "ShaderTools::AnyConverter::convertFileToFile(): cannot determine the format of whatever.osl\n");
}

void AnyConverterTest::convertFileToFileNotSupported() {
    CORRADE_SKIP("No plugin that would support just conversion exists.");
}

void AnyConverterTest::convertFileToFilePreprocessNotSupported() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("SpirvToolsShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("SpirvToolsShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setDefinitions({
        {"DEFINE", "hahahahah"}
    });

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToFile({}, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.spv"),
    Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.spvasm")));
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertFileToFile(): SpirvToolsShaderConverter does not support preprocessing\n");

    /* It should fail for the flag as well */
    out = {};
    converter->setDefinitions({});
    converter->setFlags(ConverterFlag::PreprocessOnly);
    CORRADE_VERIFY(!converter->convertFileToFile({}, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.spv"),
    Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.spvasm")));
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertFileToFile(): SpirvToolsShaderConverter does not support preprocessing\n");
}

void AnyConverterTest::convertFileToFileDebugInfoNotSupported() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("SpirvToolsShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("SpirvToolsShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setDebugInfoLevel("1");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToFile({}, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.spv"),
    Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.spvasm")));
    /** @todo it once may support that, in which case we need to find another
        victim */
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertFileToFile(): SpirvToolsShaderConverter does not support controlling debug info output\n");
}

void AnyConverterTest::convertFileToFileOptimizationNotSupported() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setOptimizationLevel("1");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToFile({}, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"),
    Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.spv")));
    /** @todo it once may support that, in which case we need to find another
        victim */
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertFileToFile(): GlslangShaderConverter does not support optimization\n");
}

void AnyConverterTest::convertFileToFilePropagateFlags() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    Containers::String filename = Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl");

    /* With this, the warning should turn into an error. The converter should
       also print the verbose info. */
    converter->setFlags(ConverterFlag::Verbose|ConverterFlag::WarningAsError);

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Debug redirectDebug{&out};
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToFile(Stage::Fragment, filename, Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.spv")));
    CORRADE_COMPARE(out, Utility::format(
        "ShaderTools::AnyConverter::convertFileToFile(): using GlslToSpirvShaderConverter (provided by GlslangShaderConverter)\n"
        "ShaderTools::GlslangConverter::convertDataToData(): compilation failed:\n"
        "WARNING: {}:10: 'reserved__identifier' : identifiers containing consecutive underscores (\"__\") are reserved\n", filename));
}

void AnyConverterTest::convertFileToFilePropagateInputVersion() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    /* This is an invalid version */
    converter->setInputFormat(Format::Glsl, "100");

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToFile(Stage::Fragment, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"), Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.spv")));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::convertDataToData(): input format version should be one of supported GLSL #version strings but got 100\n");
}

void AnyConverterTest::convertFileToFilePropagateOutputVersion() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    /* This is an invalid version */
    converter->setOutputFormat(Format::Spirv, "opengl4.0");

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToFile(Stage::Fragment, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"), Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.spv")));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::convertDataToData(): output format version target should be opengl4.5 or vulkanX.Y but got opengl4.0\n");
}

void AnyConverterTest::convertFileToFilePropagatePreprocess() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    /* Check that undefining works properly -- if it stays defined, the source
       won't compile */
    converter->setDefinitions({
        {"SHOULD_BE_UNDEFINED", "really"},
        {"SHOULD_BE_UNDEFINED", nullptr},
        {"reserved__identifier", "different__but_also_wrong"}
    });

    Containers::String inputFilename = Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl");
    Containers::String outputFilename = Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.spv");
    if(Utility::Path::exists(outputFilename))
        CORRADE_VERIFY(Utility::Path::remove(outputFilename));

    /* Make it print a warning so we know it's doing something */
    Containers::String out;
    Warning redirectWarning{&out};
    CORRADE_VERIFY(converter->convertFileToFile(Stage::Fragment, inputFilename, outputFilename));
    CORRADE_VERIFY(Utility::Path::exists(outputFilename));
    CORRADE_COMPARE(out, Utility::format(
        "ShaderTools::GlslangConverter::convertDataToData(): compilation succeeded with the following message:\n"
        "WARNING: {}:10: 'different__but_also_wrong' : identifiers containing consecutive underscores (\"__\") are reserved\n", inputFilename));
}

void AnyConverterTest::convertFileToFilePropagateDebugInfo() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    /* This is an invalid level */
    converter->setDebugInfoLevel("2");

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToFile(Stage::Fragment, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"), Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.spv")));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::convertDataToData(): debug info level should be 0, 1 or empty but got 2\n");
}

void AnyConverterTest::convertFileToFilePropagateOptimization() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("SpirvToolsShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("SpirvToolsShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    /* This is an invalid level */
    converter->setOptimizationLevel("2");

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToFile(Stage::Fragment, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.spv"), Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.spv")));
    CORRADE_COMPARE(out,
        "ShaderTools::SpirvToolsConverter::convertDataToData(): optimization level should be 0, 1, s, legalizeHlsl or empty but got 2\n");
}

void AnyConverterTest::convertFileToFilePropagateConfiguration() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    Containers::String input = Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "version-not-first.glsl");
    Containers::String output = Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.spv");

    {
        Containers::String out;
        Error redirectError{&out};
        CORRADE_VERIFY(!converter->convertFileToFile(Stage::Fragment, input, output));
        CORRADE_COMPARE(out,
            Utility::format("ShaderTools::GlslangConverter::convertDataToData(): compilation failed:\nERROR: {}:2: '#version' : must occur first in shader \nERROR: 1 compilation errors.  No code generated.\n", input));
    } {
        converter->configuration().setValue("permissive", true);
        /* Lol stupid thing, apparently it has two differently worded messages
           for the same thing? Dumpster fire. */
        Containers::String out;
        Warning redirectWarning{&out};
        CORRADE_VERIFY(converter->convertFileToFile(Stage::Fragment, input, output));
        CORRADE_COMPARE(out,
            "ShaderTools::GlslangConverter::convertDataToData(): compilation succeeded with the following message:\nWARNING: 0:0: '#version' : Illegal to have non-comment, non-whitespace tokens before #version\n");
    }
}

void AnyConverterTest::convertFileToFilePropagateConfigurationUnknown() {
    auto&& data = PropagateConfigurationUnknownData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");
    converter->configuration().setValue("noSuchOption", "isHere");
    /* So it doesn't warn about anything */
    converter->setDefinitions({{"reserved__identifier", "sorry"}});
    converter->setFlags(data.flags);

    Containers::String out;
    Warning redirectWarning{&out};
    CORRADE_VERIFY(converter->convertFileToFile(Stage::Fragment, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"), Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.glsl")));
    if(data.quiet)
        CORRADE_COMPARE(out, "");
    else
        CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertFileToFile(): option noSuchOption not recognized by GlslangShaderConverter\n");
}

void AnyConverterTest::convertFileToData() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setOutputFormat(Format::Spirv);

    Containers::String inputFilename = Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl");

    /* Make it print a warning so we know it's doing something */
    Containers::String out;
    Warning redirectWarning{&out};
    CORRADE_VERIFY(converter->convertFileToData(Stage::Fragment, inputFilename));
    CORRADE_COMPARE(out, Utility::format(
        "ShaderTools::GlslangConverter::convertDataToData(): compilation succeeded with the following message:\n"
        "WARNING: {}:10: 'reserved__identifier' : identifiers containing consecutive underscores (\"__\") are reserved\n", inputFilename));
}

void AnyConverterTest::convertFileToDataPluginLoadFailed() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    converter->setOutputFormat(Format::Wgsl);

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToData({}, "file.spv"));
    #ifndef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin SpirvToWgslShaderConverter is not static and was not found in nonexistent\n"
        "ShaderTools::AnyConverter::convertFileToData(): cannot load the SpirvToWgslShaderConverter plugin\n");
    #else
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin SpirvToWgslShaderConverter was not found\n"
        "ShaderTools::AnyConverter::convertFileToData(): cannot load the SpirvToWgslShaderConverter plugin\n");
    #endif
}

void AnyConverterTest::convertFileToDataUnknown() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToData({}, "dead.cg"));
    CORRADE_COMPARE(out, "ShaderTools::AnyConverter::convertFileToData(): cannot determine the format of dead.cg\n");
}

void AnyConverterTest::convertFileToDataNoFormatSet() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToData({}, "file.spv"));
    CORRADE_COMPARE(out, "ShaderTools::AnyConverter::convertFileToData(): no output format specified\n");
}

void AnyConverterTest::convertFileToDataNotSupported() {
    CORRADE_SKIP("No plugin that would support just conversion exists.");
}

void AnyConverterTest::convertFileToDataPreprocessNotSupported() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("SpirvToolsShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("SpirvToolsShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setOutputFormat(Format::SpirvAssembly);

    converter->setDefinitions({
        {"DEFINE", "hahahahah"}
    });

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToData({}, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.spv")));
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertFileToData(): SpirvToolsShaderConverter does not support preprocessing\n");

    /* It should fail for the flag as well */
    out = {};
    converter->setDefinitions({});
    converter->setFlags(ConverterFlag::PreprocessOnly);
    CORRADE_VERIFY(!converter->convertFileToData({}, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.spv")));
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertFileToData(): SpirvToolsShaderConverter does not support preprocessing\n");
}

void AnyConverterTest::convertFileToDataDebugInfoNotSupported() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("SpirvToolsShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("SpirvToolsShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setOutputFormat(Format::SpirvAssembly);

    converter->setDebugInfoLevel("1");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToData({}, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.spv")));
    /** @todo it once may support that, in which case we need to find another
        victim */
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertFileToData(): SpirvToolsShaderConverter does not support controlling debug info output\n");
}

void AnyConverterTest::convertFileToDataOptimizationNotSupported() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setOutputFormat(Format::Spirv);

    converter->setOptimizationLevel("1");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToData({}, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl")));
    /** @todo it once may support that, in which case we need to find another
        victim */
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertFileToData(): GlslangShaderConverter does not support optimization\n");
}

void AnyConverterTest::convertFileToDataPropagateFlags() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    Containers::String filename = Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl");

    converter->setOutputFormat(Format::Spirv);

    /* With this, the warning should turn into an error. The converter should
       also print the verbose info. */
    converter->setFlags(ConverterFlag::Verbose|ConverterFlag::WarningAsError);

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Debug redirectDebug{&out};
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToData(Stage::Fragment, filename));
    CORRADE_COMPARE(out, Utility::format(
        "ShaderTools::AnyConverter::convertFileToData(): using GlslToSpirvShaderConverter (provided by GlslangShaderConverter)\n"
        "ShaderTools::GlslangConverter::convertDataToData(): compilation failed:\n"
        "WARNING: {}:10: 'reserved__identifier' : identifiers containing consecutive underscores (\"__\") are reserved\n", filename));
}

void AnyConverterTest::convertFileToDataPropagateInputVersion() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    /* This is an invalid version */
    converter->setInputFormat(Format::Glsl, "100");

    converter->setOutputFormat(Format::Spirv);

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToData(Stage::Fragment, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl")));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::convertDataToData(): input format version should be one of supported GLSL #version strings but got 100\n");
}

void AnyConverterTest::convertFileToDataPropagateOutputVersion() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    /* This is an invalid version */
    converter->setOutputFormat(Format::Spirv, "opengl4.0");

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToData(Stage::Fragment, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl")));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::convertDataToData(): output format version target should be opengl4.5 or vulkanX.Y but got opengl4.0\n");
}

void AnyConverterTest::convertFileToDataPropagatePreprocess() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setOutputFormat(Format::Spirv);

    /* Check that undefining works properly -- if it stays defined, the source
       won't compile */
    converter->setDefinitions({
        {"SHOULD_BE_UNDEFINED", "really"},
        {"SHOULD_BE_UNDEFINED", nullptr},
        {"reserved__identifier", "different__but_also_wrong"}
    });

    Containers::String inputFilename = Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl");

    /* Make it print a warning so we know it's doing something */
    Containers::String out;
    Warning redirectWarning{&out};
    CORRADE_VERIFY(converter->convertFileToData(Stage::Fragment, inputFilename));
    CORRADE_COMPARE(out, Utility::format(
        "ShaderTools::GlslangConverter::convertDataToData(): compilation succeeded with the following message:\n"
        "WARNING: {}:10: 'different__but_also_wrong' : identifiers containing consecutive underscores (\"__\") are reserved\n", inputFilename));
}

void AnyConverterTest::convertFileToDataPropagateDebugInfo() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setOutputFormat(Format::Spirv);

    /* This is an invalid level */
    converter->setDebugInfoLevel("2");

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToData(Stage::Fragment, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl")));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::convertDataToData(): debug info level should be 0, 1 or empty but got 2\n");
}

void AnyConverterTest::convertFileToDataPropagateOptimization() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("SpirvToolsShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("SpirvToolsShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setOutputFormat(Format::Spirv);

    /* This is an invalid level */
    converter->setOptimizationLevel("2");

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToData(Stage::Fragment, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.spv")));
    CORRADE_COMPARE(out,
        "ShaderTools::SpirvToolsConverter::convertDataToData(): optimization level should be 0, 1, s, legalizeHlsl or empty but got 2\n");
}

void AnyConverterTest::convertFileToDataPropagateConfiguration() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setOutputFormat(Format::Spirv);

    Containers::String input = Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "version-not-first.glsl");

    {
        Containers::String out;
        Error redirectError{&out};
        CORRADE_VERIFY(!converter->convertFileToData(Stage::Fragment, input));
        CORRADE_COMPARE(out,
            Utility::format("ShaderTools::GlslangConverter::convertDataToData(): compilation failed:\nERROR: {}:2: '#version' : must occur first in shader \nERROR: 1 compilation errors.  No code generated.\n", input));
    } {
        converter->configuration().setValue("permissive", true);
        /* Lol stupid thing, apparently it has two differently worded messages
           for the same thing? Dumpster fire. */
        Containers::String out;
        Warning redirectWarning{&out};
        CORRADE_VERIFY(converter->convertFileToData(Stage::Fragment, input));
        CORRADE_COMPARE(out,
            "ShaderTools::GlslangConverter::convertDataToData(): compilation succeeded with the following message:\nWARNING: 0:0: '#version' : Illegal to have non-comment, non-whitespace tokens before #version\n");
    }
}

void AnyConverterTest::convertFileToDataPropagateConfigurationUnknown() {
    auto&& data = PropagateConfigurationUnknownData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");
    converter->setOutputFormat(Format::Spirv);
    converter->configuration().setValue("noSuchOption", "isHere");
    /* So it doesn't warn about anything */
    converter->setDefinitions({{"reserved__identifier", "sorry"}});
    converter->setFlags(data.flags);

    Containers::String out;
    Warning redirectWarning{&out};
    CORRADE_VERIFY(converter->convertFileToData(Stage::Fragment, Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl")));
    if(data.quiet)
        CORRADE_COMPARE(out, "");
    else
        CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertFileToData(): option noSuchOption not recognized by GlslangShaderConverter\n");
}

void AnyConverterTest::convertDataToData() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Glsl);
    converter->setOutputFormat(Format::Spirv);

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(data);

    /* Make it print a warning so we know it's doing something */
    Containers::String out;
    Warning redirectWarning{&out};
    CORRADE_VERIFY(converter->convertDataToData(Stage::Fragment, *data));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::convertDataToData(): compilation succeeded with the following message:\n"
        "WARNING: 0:10: 'reserved__identifier' : identifiers containing consecutive underscores (\"__\") are reserved\n");
}

void AnyConverterTest::convertDataToDataPluginLoadFailed() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Hlsl);
    converter->setOutputFormat(Format::Wgsl);

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertDataToData({}, {}));
    #ifndef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin HlslToWgslShaderConverter is not static and was not found in nonexistent\n"
        "ShaderTools::AnyConverter::convertDataToData(): cannot load the HlslToWgslShaderConverter plugin\n");
    #else
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin HlslToWgslShaderConverter was not found\n"
        "ShaderTools::AnyConverter::convertDataToData(): cannot load the HlslToWgslShaderConverter plugin\n");
    #endif
}

void AnyConverterTest::convertDataToDataNoInputFormatSet() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertDataToData({}, {}));
    CORRADE_COMPARE(out, "ShaderTools::AnyConverter::convertDataToData(): no input format specified\n");
}

void AnyConverterTest::convertDataToDataNoOutputFormatSet() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Spirv);

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertDataToData({}, {}));
    CORRADE_COMPARE(out, "ShaderTools::AnyConverter::convertDataToData(): no output format specified\n");
}

void AnyConverterTest::convertDataToDataNotSupported() {
    CORRADE_SKIP("No plugin that would support just conversion exists.");
}

void AnyConverterTest::convertDataToDataPreprocessNotSupported() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("SpirvToolsShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("SpirvToolsShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Spirv);
    converter->setOutputFormat(Format::SpirvAssembly);

    converter->setDefinitions({
        {"DEFINE", "hahahahah"}
    });

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.spv"));
    CORRADE_VERIFY(data);

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertDataToData({}, *data));
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertDataToData(): SpirvToolsShaderConverter does not support preprocessing\n");

    /* It should fail for the flag as well */
    out = {};
    converter->setDefinitions({});
    converter->setFlags(ConverterFlag::PreprocessOnly);
    CORRADE_VERIFY(!converter->convertDataToData({}, *data));
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertDataToData(): SpirvToolsShaderConverter does not support preprocessing\n");
}

void AnyConverterTest::convertDataToDataDebugInfoNotSupported() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("SpirvToolsShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("SpirvToolsShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Spirv);
    converter->setOutputFormat(Format::SpirvAssembly);

    converter->setDebugInfoLevel("1");

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.spv"));
    CORRADE_VERIFY(data);

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertDataToData({}, *data));
    /** @todo it once may support that, in which case we need to find another
        victim */
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertDataToData(): SpirvToolsShaderConverter does not support controlling debug info output\n");
}

void AnyConverterTest::convertDataToDataOptimizationNotSupported() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Glsl);
    converter->setOutputFormat(Format::Spirv);

    converter->setOptimizationLevel("1");

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(data);

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertDataToData({}, *data));
    /** @todo it once may support that, in which case we need to find another
        victim */
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertDataToData(): GlslangShaderConverter does not support optimization\n");
}

void AnyConverterTest::convertDataToDataPropagateFlags() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Glsl);
    converter->setOutputFormat(Format::Spirv);

    /* With this, the warning should turn into an error. The converter should
       also print the verbose info. */
    converter->setFlags(ConverterFlag::Verbose|ConverterFlag::WarningAsError);

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(data);

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Debug redirectDebug{&out};
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertDataToData(Stage::Fragment, *data));
    CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertDataToData(): using GlslToSpirvShaderConverter (provided by GlslangShaderConverter)\n"
        "ShaderTools::GlslangConverter::convertDataToData(): compilation failed:\n"
        "WARNING: 0:10: 'reserved__identifier' : identifiers containing consecutive underscores (\"__\") are reserved\n");
}

void AnyConverterTest::convertDataToDataPropagateInputVersion() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    /* This is an invalid version */
    converter->setInputFormat(Format::Glsl, "100");

    converter->setOutputFormat(Format::Spirv);

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(data);

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertDataToData(Stage::Fragment, *data));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::convertDataToData(): input format version should be one of supported GLSL #version strings but got 100\n");
}

void AnyConverterTest::convertDataToDataPropagateOutputVersion() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Glsl);

    /* This is an invalid version */
    converter->setOutputFormat(Format::Spirv, "opengl4.0");

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(data);

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertDataToData(Stage::Fragment, *data));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::convertDataToData(): output format version target should be opengl4.5 or vulkanX.Y but got opengl4.0\n");
}

void AnyConverterTest::convertDataToDataPropagatePreprocess() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Glsl);
    converter->setOutputFormat(Format::Spirv);

    /* Check that undefining works properly -- if it stays defined, the source
       won't compile */
    converter->setDefinitions({
        {"SHOULD_BE_UNDEFINED", "really"},
        {"SHOULD_BE_UNDEFINED", nullptr},
        {"reserved__identifier", "different__but_also_wrong"}
    });

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(data);

    /* Make it print a warning so we know it's doing something */
    Containers::String out;
    Warning redirectWarning{&out};
    CORRADE_VERIFY(converter->convertDataToData(Stage::Fragment, *data));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::convertDataToData(): compilation succeeded with the following message:\n"
        "WARNING: 0:10: 'different__but_also_wrong' : identifiers containing consecutive underscores (\"__\") are reserved\n");
}

void AnyConverterTest::convertDataToDataPropagateDebugInfo() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Glsl);
    converter->setOutputFormat(Format::Spirv);

    /* This is an invalid level */
    converter->setDebugInfoLevel("2");

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(data);

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertDataToData(Stage::Fragment, *data));
    CORRADE_COMPARE(out,
        "ShaderTools::GlslangConverter::convertDataToData(): debug info level should be 0, 1 or empty but got 2\n");
}

void AnyConverterTest::convertDataToDataPropagateOptimization() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("SpirvToolsShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("SpirvToolsShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Spirv);
    converter->setOutputFormat(Format::Spirv);

    /* This is an invalid level */
    converter->setOptimizationLevel("2");

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.spv"));
    CORRADE_VERIFY(data);

    /* We have to supply a valid file path because the version gets checked in
       doConvertDataToData(), called from AbstractConverter::doConvertFileToFile()
       with the file contents. */
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertDataToData(Stage::Fragment, *data));
    CORRADE_COMPARE(out,
        "ShaderTools::SpirvToolsConverter::convertDataToData(): optimization level should be 0, 1, s, legalizeHlsl or empty but got 2\n");
}

void AnyConverterTest::convertDataToDataPropagateConfiguration() {
    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");

    converter->setInputFormat(Format::Glsl);
    converter->setOutputFormat(Format::Spirv);

    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "version-not-first.glsl"));
    CORRADE_VERIFY(data);

    {
        Containers::String out;
        Error redirectError{&out};
        CORRADE_VERIFY(!converter->convertDataToData(Stage::Fragment, *data));
        CORRADE_COMPARE(out,
            "ShaderTools::GlslangConverter::convertDataToData(): compilation failed:\nERROR: 0:2: '#version' : must occur first in shader \nERROR: 1 compilation errors.  No code generated.\n");
    } {
        converter->configuration().setValue("permissive", true);
        /* Lol stupid thing, apparently it has two differently worded messages
           for the same thing? Dumpster fire. */
        Containers::String out;
        Warning redirectWarning{&out};
        CORRADE_VERIFY(converter->convertDataToData(Stage::Fragment, *data));
        CORRADE_COMPARE(out,
            "ShaderTools::GlslangConverter::convertDataToData(): compilation succeeded with the following message:\nWARNING: 0:0: '#version' : Illegal to have non-comment, non-whitespace tokens before #version\n");
    }
}

void AnyConverterTest::convertDataToDataPropagateConfigurationUnknown() {
    auto&& data = PropagateConfigurationUnknownData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    PluginManager::Manager<AbstractConverter> manager{MAGNUM_PLUGINS_SHADERCONVERTER_INSTALL_DIR};
    #ifdef ANYSHADERCONVERTER_PLUGIN_FILENAME
    CORRADE_VERIFY(manager.load(ANYSHADERCONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif

    if(manager.load("GlslangShaderConverter") < PluginManager::LoadState::Loaded)
        CORRADE_SKIP("GlslangShaderConverter plugin can't be loaded.");

    Containers::Pointer<AbstractConverter> converter = manager.instantiate("AnyShaderConverter");
    converter->setInputFormat(Format::Glsl);
    converter->setOutputFormat(Format::Spirv);
    converter->configuration().setValue("noSuchOption", "isHere");
    /* So it doesn't warn about anything */
    converter->setDefinitions({{"reserved__identifier", "sorry"}});
    converter->setFlags(data.flags);

    Containers::Optional<Containers::Array<char>> shaderData = Utility::Path::read(Utility::Path::join(ANYSHADERCONVERTER_TEST_DIR, "file.glsl"));
    CORRADE_VERIFY(shaderData);

    Containers::String out;
    Warning redirectWarning{&out};
    CORRADE_VERIFY(converter->convertDataToData(Stage::Fragment, *shaderData));
    if(data.quiet)
        CORRADE_COMPARE(out, "");
    else
        CORRADE_COMPARE(out,
        "ShaderTools::AnyConverter::convertDataToData(): option noSuchOption not recognized by GlslangShaderConverter\n");
}

void AnyConverterTest::detectValidate() {
    auto&& data = DetectValidateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_COMPARE(converter->validateFile({}, data.filename),
        Containers::pair(false, Containers::String{}));
    #ifndef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
    CORRADE_COMPARE(out, Utility::format(
        "PluginManager::Manager::load(): plugin {0} is not static and was not found in nonexistent\n"
        "ShaderTools::AnyConverter::validateFile(): cannot load the {0} plugin\n", data.plugin));
    #else
    CORRADE_COMPARE(out, Utility::format(
        "PluginManager::Manager::load(): plugin {0} was not found\n"
        "ShaderTools::AnyConverter::validateFile(): cannot load the {0} plugin\n", data.plugin));
    #endif
}

void AnyConverterTest::detectValidateExplicitFormat() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    /* It should pick up this format and not bother with the extension */
    converter->setInputFormat(Format::Hlsl);

    Containers::String out;
    Error redirectError{&out};
    CORRADE_COMPARE(converter->validateFile({}, "file.spv"),
        Containers::pair(false, Containers::String{}));
    #ifndef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin HlslShaderConverter is not static and was not found in nonexistent\n"
        "ShaderTools::AnyConverter::validateFile(): cannot load the HlslShaderConverter plugin\n");
    #else
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin HlslShaderConverter was not found\n"
        "ShaderTools::AnyConverter::validateFile(): cannot load the HlslShaderConverter plugin\n");
    #endif
}

void AnyConverterTest::detectConvert() {
    auto&& data = DetectConvertData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToFile({}, data.from, Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, data.to)));
    #ifndef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
    CORRADE_COMPARE(out, Utility::format(
        "PluginManager::Manager::load(): plugin {0} is not static and was not found in nonexistent\n"
        "ShaderTools::AnyConverter::convertFileToFile(): cannot load the {0} plugin\n", data.plugin));
    #else
    CORRADE_COMPARE(out, Utility::format(
        "PluginManager::Manager::load(): plugin {0} was not found\n"
        "ShaderTools::AnyConverter::convertFileToFile(): cannot load the {0} plugin\n", data.plugin));
    #endif
}

void AnyConverterTest::detectConvertExplicitFormat() {
    Containers::Pointer<AbstractConverter> converter = _manager.instantiate("AnyShaderConverter");

    /* It should pick up this format and not bother with the extension */
    converter->setInputFormat(Format::Hlsl);
    converter->setOutputFormat(Format::Wgsl);

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertFileToFile({}, "file.spv", Utility::Path::join(ANYSHADERCONVERTER_TEST_OUTPUT_DIR, "file.glsl")));
    #ifndef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin HlslToWgslShaderConverter is not static and was not found in nonexistent\n"
        "ShaderTools::AnyConverter::convertFileToFile(): cannot load the HlslToWgslShaderConverter plugin\n");
    #else
    CORRADE_COMPARE(out,
        "PluginManager::Manager::load(): plugin HlslToWgslShaderConverter was not found\n"
        "ShaderTools::AnyConverter::convertFileToFile(): cannot load the HlslToWgslShaderConverter plugin\n");
    #endif
}

}}}}

CORRADE_TEST_MAIN(Magnum::ShaderTools::Test::AnyConverterTest)
