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

#include "AbstractImporter.h"

#include <string> /** @todo remove once file callbacks are <string>-free */
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once file callbacks are <string>-free */
#include <Corrade/PluginManager/Manager.hpp>
#include <Corrade/Utility/Assert.h>
#include <Corrade/Utility/Path.h>

#include "Magnum/FileCallback.h"
#include "Magnum/Trade/AnimationData.h"
#include "Magnum/Trade/ArrayAllocator.h"
#include "Magnum/Trade/CameraData.h"
#include "Magnum/Trade/ImageData.h"
#include "Magnum/Trade/LightData.h"
#include "Magnum/Trade/MaterialData.h"
#include "Magnum/Trade/MeshData.h"
#include "Magnum/Trade/SceneData.h"
#include "Magnum/Trade/SkinData.h"
#include "Magnum/Trade/TextureData.h"

#ifdef MAGNUM_BUILD_DEPRECATED
#include <string> /* for object2DName() etc., not going to change those */
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/Triple.h>
#include <Corrade/Utility/DebugStl.h> /* for errors in object2DForName() etc. */

/* This is a header-only tool, meaning no link-time dependency on SceneTools */
/** @todo once this compat is dropped, drop the header-only implementation */
#include "Magnum/SceneTools/Implementation/convertToSingleFunctionObjects.h"

#define _MAGNUM_NO_DEPRECATED_MESHDATA /* So it doesn't yell here */
#define _MAGNUM_NO_DEPRECATED_OBJECTDATA /* So it doesn't yell here */

#include "Magnum/Trade/MeshData2D.h"
#include "Magnum/Trade/MeshData3D.h"
#include "Magnum/Trade/MeshObjectData2D.h"
#include "Magnum/Trade/MeshObjectData3D.h"
#endif

#ifndef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
#include "Magnum/Trade/configure.h"
#endif

namespace Corrade { namespace PluginManager {

template class MAGNUM_TRADE_EXPORT Manager<Magnum::Trade::AbstractImporter>;

}}

namespace Magnum { namespace Trade {

using namespace Containers::Literals;

Containers::StringView AbstractImporter::pluginInterface() {
    return MAGNUM_TRADE_ABSTRACTIMPORTER_PLUGIN_INTERFACE ""_s;
}

#ifndef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
Containers::Array<Containers::String> AbstractImporter::pluginSearchPaths() {
    const Containers::Optional<Containers::String> libraryLocation = Utility::Path::libraryLocation(&pluginInterface);
    return PluginManager::implicitPluginSearchPaths(
        #ifndef MAGNUM_BUILD_STATIC
        libraryLocation ? *libraryLocation : Containers::String{},
        #else
        {},
        #endif
        #ifdef CORRADE_IS_DEBUG_BUILD
        MAGNUM_PLUGINS_IMPORTER_DEBUG_DIR,
        #else
        MAGNUM_PLUGINS_IMPORTER_DIR,
        #endif
        #ifdef CORRADE_IS_DEBUG_BUILD
        "magnum-d/"
        #else
        "magnum/"
        #endif
        "importers"_s);
}
#endif

AbstractImporter::AbstractImporter() = default;

AbstractImporter::AbstractImporter(PluginManager::Manager<AbstractImporter>& manager): PluginManager::AbstractManagingPlugin<AbstractImporter>{manager} {}

AbstractImporter::AbstractImporter(PluginManager::AbstractManager& manager, const Containers::StringView& plugin): PluginManager::AbstractManagingPlugin<AbstractImporter>{manager, plugin} {}

#ifdef MAGNUM_BUILD_DEPRECATED
/* These twp needed because of the Array<CachedScenes> member */
AbstractImporter::AbstractImporter(AbstractImporter&&) noexcept = default;
AbstractImporter::~AbstractImporter() = default;
#endif

void AbstractImporter::setFlags(ImporterFlags flags) {
    CORRADE_ASSERT(!isOpened(),
        "Trade::AbstractImporter::setFlags(): can't be set while a file is opened", );
    _flags = flags;
    doSetFlags(flags);
}

void AbstractImporter::doSetFlags(ImporterFlags) {}

void AbstractImporter::addFlags(ImporterFlags flags) {
    setFlags(_flags|flags);
}

void AbstractImporter::clearFlags(ImporterFlags flags) {
    setFlags(_flags & ~flags);
}

void AbstractImporter::setFileCallback(Containers::Optional<Containers::ArrayView<const char>>(*callback)(const std::string&, InputFileCallbackPolicy, void*), void* const userData) {
    CORRADE_ASSERT(!isOpened(), "Trade::AbstractImporter::setFileCallback(): can't be set while a file is opened", );
    CORRADE_ASSERT(features() & (ImporterFeature::FileCallback|ImporterFeature::OpenData), "Trade::AbstractImporter::setFileCallback(): importer supports neither loading from data nor via callbacks, callbacks can't be used", );

    _fileCallback = callback;
    _fileCallbackUserData = userData;
    doSetFileCallback(callback, userData);
}

void AbstractImporter::doSetFileCallback(Containers::Optional<Containers::ArrayView<const char>>(*)(const std::string&, InputFileCallbackPolicy, void*), void*) {}

bool AbstractImporter::openData(Containers::ArrayView<const void> data) {
    CORRADE_ASSERT(features() & ImporterFeature::OpenData,
        "Trade::AbstractImporter::openData(): feature not supported", {});

    /* We accept empty data here (instead of checking for them and failing so
       the check doesn't be done on the plugin side) because for some file
       formats it could be valid (e.g. OBJ or JSON-based formats). */
    close();
    doOpenData(Containers::Array<char>{const_cast<char*>(static_cast<const char*>(data.data())), data.size(), Implementation::nonOwnedArrayDeleter}, {});
    return isOpened();
}

#ifdef MAGNUM_BUILD_DEPRECATED
void AbstractImporter::doOpenData(Containers::ArrayView<const char>) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::openData(): feature advertised but not implemented", );
}
#endif

void AbstractImporter::doOpenData(Containers::Array<char>&& data, const DataFlags) {
    #ifndef MAGNUM_BUILD_DEPRECATED
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::openData(): feature advertised but not implemented", );
    static_cast<void>(data);
    #else
    CORRADE_IGNORE_DEPRECATED_PUSH
    doOpenData(data);
    CORRADE_IGNORE_DEPRECATED_POP
    #endif
}

bool AbstractImporter::openMemory(Containers::ArrayView<const void> memory) {
    CORRADE_ASSERT(features() & ImporterFeature::OpenData,
        "Trade::AbstractImporter::openMemory(): feature not supported", {});

    /* We accept empty data here (instead of checking for them and failing so
       the check doesn't be done on the plugin side) because for some file
       formats it could be valid (e.g. OBJ or JSON-based formats). */
    close();
    doOpenData(Containers::Array<char>{const_cast<char*>(static_cast<const char*>(memory.data())), memory.size(), Implementation::nonOwnedArrayDeleter}, DataFlag::ExternallyOwned);
    return isOpened();
}

bool AbstractImporter::openState(const void* const state, const Containers::StringView filePath) {
    CORRADE_ASSERT(features() & ImporterFeature::OpenState,
        "Trade::AbstractImporter::openState(): feature not supported", {});

    close();
    doOpenState(state, filePath);
    return isOpened();
}

bool AbstractImporter::openState(const void* const state) {
    return openState(state, {});
}

void AbstractImporter::doOpenState(const void*, const Containers::StringView) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::openState(): feature advertised but not implemented", );
}

bool AbstractImporter::openFile(const Containers::StringView filename) {
    close();

    /* If file loading callbacks are not set or the importer supports handling
       them directly, call into the implementation */
    if(!_fileCallback || (doFeatures() & ImporterFeature::FileCallback)) {
        doOpenFile(filename);

    /* Otherwise, if loading from data is supported, use the callback and pass
       the data through to openData(). Mark the file as ready to be closed once
       opening is finished. */
    } else if(doFeatures() & ImporterFeature::OpenData) {
        /* This needs to be duplicated here and in the doOpenFile()
           implementation in order to support both following cases:
            - plugins that don't support FileCallback but have their own
              doOpenFile() implementation (callback needs to be used here,
              because the base doOpenFile() implementation might never get
              called)
            - plugins that support FileCallback but want to delegate the actual
              file loading to the default implementation (callback used in the
              base doOpenFile() implementation, because this branch is never
              taken in that case) */
        const Containers::Optional<Containers::ArrayView<const char>> data = _fileCallback(filename, InputFileCallbackPolicy::LoadTemporary, _fileCallbackUserData);
        if(!data) {
            Error() << "Trade::AbstractImporter::openFile(): cannot open file" << filename;
            return isOpened();
        }
        /** @todo it might be useful to use LoadPermanent and DataFlag::Owned
            here, but it kinda depends on the plugin if it wants to keep a copy
            of the data... which means it's probably the most straightforward
            if we just provide an explicit doOpenFile() implementation there.

            Same in doOpenFile() below. */
        doOpenData(Containers::Array<char>{const_cast<char*>(data->data()), data->size(), Implementation::nonOwnedArrayDeleter}, {});
        _fileCallback(filename, InputFileCallbackPolicy::Close, _fileCallbackUserData);

    /* Shouldn't get here, the assert is fired already in setFileCallback() */
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

    return isOpened();
}

void AbstractImporter::doOpenFile(const Containers::StringView filename) {
    CORRADE_ASSERT(features() & ImporterFeature::OpenData, "Trade::AbstractImporter::openFile(): not implemented", );

    /* If callbacks are set, use them. This is the same implementation as in
       openFile(), see the comments there for details. */
    if(_fileCallback) {
        const Containers::Optional<Containers::ArrayView<const char>> data = _fileCallback(filename, InputFileCallbackPolicy::LoadTemporary, _fileCallbackUserData);
        if(!data) {
            Error() << "Trade::AbstractImporter::openFile(): cannot open file" << filename;
            return;
        }
        doOpenData(Containers::Array<char>{const_cast<char*>(data->data()), data->size(), Implementation::nonOwnedArrayDeleter}, {});
        _fileCallback(filename, InputFileCallbackPolicy::Close, _fileCallbackUserData);

    /* Otherwise open the file directly */
    } else {
        Containers::Optional<Containers::Array<char>> data = Utility::Path::read(filename);
        if(!data) {
            Error() << "Trade::AbstractImporter::openFile(): cannot open file" << filename;
            return;
        }

        doOpenData(*Utility::move(data), DataFlag::Owned|DataFlag::Mutable);
    }
}

void AbstractImporter::close() {
    if(isOpened()) {
        doClose();
        CORRADE_INTERNAL_ASSERT(!isOpened());
    }
}

Int AbstractImporter::defaultScene() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::defaultScene(): no file opened", -1);
    const Int id = doDefaultScene();
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doSceneCount(),
        "Trade::AbstractImporter::defaultScene(): implementation-returned index" << id << "out of range for" << doSceneCount() << "entries", {});
    return id;
}

Int AbstractImporter::doDefaultScene() const { return -1; }

UnsignedInt AbstractImporter::sceneCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::sceneCount(): no file opened", 0);
    return doSceneCount();
}

UnsignedInt AbstractImporter::doSceneCount() const { return 0; }

UnsignedLong AbstractImporter::objectCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::objectCount(): no file opened", {});
    return doObjectCount();
}

UnsignedLong AbstractImporter::doObjectCount() const { return 0; }

Int AbstractImporter::sceneForName(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::sceneForName(): no file opened", -1);
    const Int id = doSceneForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doSceneCount(),
        "Trade::AbstractImporter::sceneForName(): implementation-returned index" << id << "out of range for" << doSceneCount() << "entries", {});
    return id;
}

Int AbstractImporter::doSceneForName(Containers::StringView) { return -1; }

Long AbstractImporter::objectForName(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::objectForName(): no file opened", {});
    const Long id = doObjectForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedLong(id) < doObjectCount(),
        "Trade::AbstractImporter::objectForName(): implementation-returned index" << id << "out of range for" << doObjectCount() << "entries", {});
    return id;
}

Long AbstractImporter::doObjectForName(Containers::StringView) { return -1; }

Containers::String AbstractImporter::sceneName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::sceneName(): no file opened", {});
    CORRADE_ASSERT(id < doSceneCount(), "Trade::AbstractImporter::sceneName(): index" << id << "out of range for" << doSceneCount() << "entries", {});
    Containers::String name = doSceneName(id);
    CORRADE_ASSERT(name.isSmall() || !name.deleter(),
        "Trade::AbstractImporter::sceneName(): implementation is not allowed to use a custom String deleter", {});
    return name;
}

Containers::String AbstractImporter::doSceneName(UnsignedInt) { return {}; }

Containers::String AbstractImporter::objectName(const UnsignedLong id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::objectName(): no file opened", {});
    CORRADE_ASSERT(id < doObjectCount(), "Trade::AbstractImporter::objectName(): index" << id << "out of range for" << doObjectCount() << "entries", {});
    Containers::String name = doObjectName(id);
    CORRADE_ASSERT(name.isSmall() || !name.deleter(),
        "Trade::AbstractImporter::objectName(): implementation is not allowed to use a custom String deleter", {});
    return name;
}

Containers::String AbstractImporter::doObjectName(UnsignedLong) { return {}; }

Containers::Optional<SceneData> AbstractImporter::scene(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::scene(): no file opened", {});
    CORRADE_ASSERT(id < doSceneCount(), "Trade::AbstractImporter::scene(): index" << id << "out of range for" << doSceneCount() << "entries", {});
    Containers::Optional<SceneData> scene = doScene(id);
    CORRADE_ASSERT(!scene || (
        (!scene->_data.deleter() || scene->_data.deleter() == static_cast<void(*)(char*, std::size_t)>(Implementation::nonOwnedArrayDeleter)) &&
        (!scene->_fields.deleter() || scene->_fields.deleter() == static_cast<void(*)(SceneFieldData*, std::size_t)>(Implementation::nonOwnedArrayDeleter))),
        "Trade::AbstractImporter::scene(): implementation is not allowed to use a custom Array deleter", {});
    return scene;
}

Containers::Optional<SceneData> AbstractImporter::doScene(UnsignedInt) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::scene(): not implemented", {});
}

Containers::Optional<SceneData> AbstractImporter::scene(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::scene(): no file opened", {});
    const Int id = doSceneForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::scene(): scene" << name << "not found among" << doSceneCount() << "entries";
        return {};
    }
    return scene(id); /* not doScene(), so we get the range checks also */
}

SceneField AbstractImporter::sceneFieldForName(const Containers::StringView name) {
    const SceneField out = doSceneFieldForName(name);
    CORRADE_ASSERT(out == SceneField{} || isSceneFieldCustom(out),
        "Trade::AbstractImporter::sceneFieldForName(): implementation-returned" << out << "is neither custom nor invalid", {});
    return out;
}

SceneField AbstractImporter::doSceneFieldForName(Containers::StringView) {
    return {};
}

Containers::String AbstractImporter::sceneFieldName(const SceneField name) {
    CORRADE_ASSERT(isSceneFieldCustom(name),
        "Trade::AbstractImporter::sceneFieldName():" << name << "is not custom", {});
    Containers::String out = doSceneFieldName(name);
    CORRADE_ASSERT(out.isSmall() || !out.deleter(),
        "Trade::AbstractImporter::sceneFieldName(): implementation is not allowed to use a custom String deleter", {});
    return out;
}

Containers::String AbstractImporter::doSceneFieldName(SceneField) { return {}; }

UnsignedInt AbstractImporter::animationCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::animationCount(): no file opened", {});
    return doAnimationCount();
}

UnsignedInt AbstractImporter::doAnimationCount() const { return 0; }

Int AbstractImporter::animationForName(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::animationForName(): no file opened", {});
    const Int id = doAnimationForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doAnimationCount(),
        "Trade::AbstractImporter::animationForName(): implementation-returned index" << id << "out of range for" << doAnimationCount() << "entries", {});
    return id;
}

Int AbstractImporter::doAnimationForName(Containers::StringView) { return -1; }

Containers::String AbstractImporter::animationName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::animationName(): no file opened", {});
    CORRADE_ASSERT(id < doAnimationCount(), "Trade::AbstractImporter::animationName(): index" << id << "out of range for" << doAnimationCount() << "entries", {});
    Containers::String name = doAnimationName(id);
    CORRADE_ASSERT(name.isSmall() || !name.deleter(),
        "Trade::AbstractImporter::animationName(): implementation is not allowed to use a custom String deleter", {});
    return name;
}

Containers::String AbstractImporter::doAnimationName(UnsignedInt) { return {}; }

Containers::Optional<AnimationData> AbstractImporter::animation(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::animation(): no file opened", {});
    CORRADE_ASSERT(id < doAnimationCount(), "Trade::AbstractImporter::animation(): index" << id << "out of range for" << doAnimationCount() << "entries", {});
    Containers::Optional<AnimationData> animation = doAnimation(id);
    /** @todo maybe this should also disallow custom interpolators? since thise
        would be dangling on plugin unload */
    CORRADE_ASSERT(!animation ||
        ((!animation->_data.deleter() || animation->_data.deleter() == static_cast<void(*)(char*, std::size_t)>(Implementation::nonOwnedArrayDeleter) || animation->_data.deleter() == ArrayAllocator<char>::deleter) &&
        (!animation->_tracks.deleter() || animation->_tracks.deleter() == static_cast<void(*)(AnimationTrackData*, std::size_t)>(Implementation::nonOwnedArrayDeleter))),
        "Trade::AbstractImporter::animation(): implementation is not allowed to use a custom Array deleter", {});
    return animation;
}

Containers::Optional<AnimationData> AbstractImporter::doAnimation(UnsignedInt) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::animation(): not implemented", {});
}

Containers::Optional<AnimationData> AbstractImporter::animation(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::animation(): no file opened", {});
    const Int id = doAnimationForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::animation(): animation" << name << "not found among" << doAnimationCount() << "entries";
        return {};
    }
    return animation(id); /* not doAnimation(), so we get the checks also */
}

AnimationTrackTarget AbstractImporter::animationTrackTargetForName(const Containers::StringView name) {
    const AnimationTrackTarget out = doAnimationTrackTargetForName(name);
    CORRADE_ASSERT(out == AnimationTrackTarget{} || isAnimationTrackTargetCustom(out),
        "Trade::AbstractImporter::animationTrackTargetForName(): implementation-returned" << out << "is neither custom nor invalid", {});
    return out;
}

AnimationTrackTarget AbstractImporter::doAnimationTrackTargetForName(Containers::StringView) {
    return {};
}

Containers::String AbstractImporter::animationTrackTargetName(const AnimationTrackTarget name) {
    CORRADE_ASSERT(isAnimationTrackTargetCustom(name),
        "Trade::AbstractImporter::animationTrackTargetName():" << name << "is not custom", {});
    Containers::String out = doAnimationTrackTargetName(name);
    CORRADE_ASSERT(out.isSmall() || !out.deleter(),
        "Trade::AbstractImporter::animationTrackTargetName(): implementation is not allowed to use a custom String deleter", {});
    return out;
}

Containers::String AbstractImporter::doAnimationTrackTargetName(AnimationTrackTarget) { return {}; }

UnsignedInt AbstractImporter::lightCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::lightCount(): no file opened", {});
    return doLightCount();
}

UnsignedInt AbstractImporter::doLightCount() const { return 0; }

Int AbstractImporter::lightForName(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::lightForName(): no file opened", {});
    const Int id = doLightForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doLightCount(),
        "Trade::AbstractImporter::lightForName(): implementation-returned index" << id << "out of range for" << doLightCount() << "entries", {});
    return id;
}

Int AbstractImporter::doLightForName(Containers::StringView) { return -1; }

Containers::String AbstractImporter::lightName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::lightName(): no file opened", {});
    CORRADE_ASSERT(id < doLightCount(), "Trade::AbstractImporter::lightName(): index" << id << "out of range for" << doLightCount() << "entries", {});
    Containers::String name = doLightName(id);
    CORRADE_ASSERT(name.isSmall() || !name.deleter(),
        "Trade::AbstractImporter::lightName(): implementation is not allowed to use a custom String deleter", {});
    return name;
}

Containers::String AbstractImporter::doLightName(UnsignedInt) { return {}; }

Containers::Optional<LightData> AbstractImporter::light(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::light(): no file opened", {});
    CORRADE_ASSERT(id < doLightCount(), "Trade::AbstractImporter::light(): index" << id << "out of range for" << doLightCount() << "entries", {});
    return doLight(id);
}

Containers::Optional<LightData> AbstractImporter::doLight(UnsignedInt) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::light(): not implemented", {});
}

Containers::Optional<LightData> AbstractImporter::light(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::light(): no file opened", {});
    const Int id = doLightForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::light(): light" << name << "not found among" << doLightCount() << "entries";
        return {};
    }
    return light(id); /* not doLight(), so we get the range checks also */
}

UnsignedInt AbstractImporter::cameraCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::cameraCount(): no file opened", {});
    return doCameraCount();
}

UnsignedInt AbstractImporter::doCameraCount() const { return 0; }

Int AbstractImporter::cameraForName(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::cameraForName(): no file opened", {});
    const Int id = doCameraForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doCameraCount(),
        "Trade::AbstractImporter::cameraForName(): implementation-returned index" << id << "out of range for" << doCameraCount() << "entries", {});
    return id;
}

Int AbstractImporter::doCameraForName(Containers::StringView) { return -1; }

Containers::String AbstractImporter::cameraName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::cameraName(): no file opened", {});
    CORRADE_ASSERT(id < doCameraCount(), "Trade::AbstractImporter::cameraName(): index" << id << "out of range for" << doCameraCount() << "entries", {});
    Containers::String name = doCameraName(id);
    CORRADE_ASSERT(name.isSmall() || !name.deleter(),
        "Trade::AbstractImporter::cameraName(): implementation is not allowed to use a custom String deleter", {});
    return name;
}

Containers::String AbstractImporter::doCameraName(UnsignedInt) { return {}; }

Containers::Optional<CameraData> AbstractImporter::camera(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::camera(): no file opened", {});
    CORRADE_ASSERT(id < doCameraCount(), "Trade::AbstractImporter::camera(): index" << id << "out of range for" << doCameraCount() << "entries", {});
    return doCamera(id);
}

Containers::Optional<CameraData> AbstractImporter::doCamera(UnsignedInt) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::camera(): not implemented", {});
}

Containers::Optional<CameraData> AbstractImporter::camera(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::camera(): no file opened", {});
    const Int id = doCameraForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::camera(): camera" << name << "not found among" << doCameraCount() << "entries";
        return {};
    }
    return camera(id); /* not doCamera(), so we get the range checks also */
}

#ifdef MAGNUM_BUILD_DEPRECATED
struct AbstractImporter::CachedScenes {
    UnsignedInt object2DCount{};
    UnsignedInt object3DCount{};
    Containers::Array<Containers::Optional<SceneData>> scenes;
};

void AbstractImporter::populateCachedScenes() {
    if(_cachedScenes) return;

    _cachedScenes.emplace();
    _cachedScenes->scenes = Containers::Array<Containers::Optional<SceneData>>{sceneCount()};

    UnsignedLong newObjectOffset = objectCount();
    for(UnsignedInt i = 0; i != _cachedScenes->scenes.size(); ++i) {
        _cachedScenes->scenes[i] = scene(i);
        if(_cachedScenes->scenes[i]) {
            /* Convert the scene so that each object has only either a mesh
               (potentially with a material and a skin), a camera or a light.
               The tool requires SceneField::Parent to be present, however if
               it's not then we treat the scene as empty in the backwards
               compatibility code path anyway, so just skip the processing
               altogether in that case. */
            if(_cachedScenes->scenes[i]->hasField(SceneField::Parent))
                _cachedScenes->scenes[i] = SceneTools::Implementation::convertToSingleFunctionObjects(*_cachedScenes->scenes[i], Containers::arrayView({SceneField::Mesh, SceneField::Camera, SceneField::Light}), Containers::arrayView({SceneField::Skin}), newObjectOffset);

            /* Return the 2D/3D object count based on which scenes are 2D and
               which not. The objectCount() provided by the importer is ignored
               except for the above, also because it doesn't take into account
               the restriction for unique-functioning objects. */
            if(_cachedScenes->scenes[i]->is2D())
                _cachedScenes->object2DCount = Math::max(_cachedScenes->object2DCount, UnsignedInt(_cachedScenes->scenes[i]->mappingBound()));
            if(_cachedScenes->scenes[i]->is3D())
                _cachedScenes->object3DCount = Math::max(_cachedScenes->object3DCount, UnsignedInt(_cachedScenes->scenes[i]->mappingBound()));

            /* Ensure the newly added objects for each scene don't overlap each
               other */
            newObjectOffset = Math::max(newObjectOffset, _cachedScenes->scenes[i]->mappingBound());
        }
    }

    /* If there are scenes but no objects (because for example all scenes
       failed to import), use the dimension-less object count at least, and
       assume the scene was 3D. Otherwise this may cause unexpected assertions
       in code that expected proper object count to be reported even if a scene
       contains errors.

       Not ideal, especially regarding the 3D assumption, but better than
       nothing. */
    if(!_cachedScenes->scenes.isEmpty() && !_cachedScenes->object2DCount && !_cachedScenes->object3DCount)
        _cachedScenes->object3DCount = objectCount();
}

UnsignedInt AbstractImporter::object2DCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::object2DCount(): no file opened", {});
    CORRADE_IGNORE_DEPRECATED_PUSH
    return doObject2DCount();
    CORRADE_IGNORE_DEPRECATED_POP
}

UnsignedInt AbstractImporter::doObject2DCount() const {
    /* I know, I know. A cleaner option would be to populate this during
       openFile() / openData() but that would mean the backwards compatibility
       overhead is there even if not using the deprecated APIs, which is way
       worse than using this nasty hack in two places. */
    const_cast<AbstractImporter&>(*this).populateCachedScenes();
    return _cachedScenes->object2DCount;
}

Int AbstractImporter::object2DForName(const std::string& name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::object2DForName(): no file opened", {});
    CORRADE_IGNORE_DEPRECATED_PUSH
    const Int id = doObject2DForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doObject2DCount(),
        "Trade::AbstractImporter::object2DForName(): implementation-returned index" << id << "out of range for" << doObject2DCount() << "entries", {});
    return id;
    CORRADE_IGNORE_DEPRECATED_POP
}

Int AbstractImporter::doObject2DForName(const std::string& name) {
    /* Alias to the new interface. If it returns an ID that's larger than
       reported 2D object count, then it's probably for a 3D object instead
       -- ignore it in that case. Ideally this would be solved by checking if
       the ID is actually present in a 2D scene (and same in doObject2DName())
       but that's a lot of extra code for just a backwards compatibility
       feature that almost nobody needs. The only pre-existing 2D importer is
       PrimitiveImporter, which is rarely used. */
    const Long id = doObjectForName(name);
    CORRADE_IGNORE_DEPRECATED_PUSH
    return id < doObject2DCount() ? id : -1;
    CORRADE_IGNORE_DEPRECATED_POP
}

std::string AbstractImporter::object2DName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::object2DName(): no file opened", {});
    CORRADE_IGNORE_DEPRECATED_PUSH
    CORRADE_ASSERT(id < doObject2DCount(), "Trade::AbstractImporter::object2DName(): index" << id << "out of range for" << doObject2DCount() << "entries", {});
    return doObject2DName(id);
    CORRADE_IGNORE_DEPRECATED_POP
}

std::string AbstractImporter::doObject2DName(const UnsignedInt id) {
    /* Alias to the new interface if the ID is known to the new interface,
       return an empty string for objects that got newly added in order to make
       them single-functioning */
    if(id < doObjectCount()) return doObjectName(id);

    populateCachedScenes();
    for(UnsignedInt i = 0; i != _cachedScenes->scenes.size(); ++i) {
        if(!_cachedScenes->scenes[i] ||
           !_cachedScenes->scenes[i]->is2D() ||
           _cachedScenes->scenes[i]->mappingBound() <= id)
            continue;

        if(Containers::Optional<Long> parent = _cachedScenes->scenes[i]->parentFor(id))
            return doObjectName(*parent);
    }

    return "";
}

CORRADE_IGNORE_DEPRECATED_PUSH
Containers::Pointer<ObjectData2D> AbstractImporter::object2D(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::object2D(): no file opened", {});
    CORRADE_ASSERT(id < doObject2DCount(), "Trade::AbstractImporter::object2D(): index" << id << "out of range for" << doObject2DCount() << "entries", {});
    return doObject2D(id);
}

Containers::Pointer<ObjectData2D> AbstractImporter::doObject2D(const UnsignedInt id) {
    /* The code is mostly the same between doObject2D() and doObject3D(),
       except that the 2D variant has no lights. Attempting to unify the common
       code would be more complex than just having a slightly modified copy. */

    populateCachedScenes();

    /* Find the first 2D scene with this object, which we'll detect from the
       mapping bound reported for the scene, whether it's 2D or 3D, and a
       presence of a parent attribute. If a parent attribute is not present, it
       means the object isn't a part of this scene, in which case we skip it.
       It could also mean isn't a part of the hierarchy and is standalone
       (skyboxes, scene-wide properties), which the legacy API had no way to
       deal with anyway so ignoring those is fine. */
    std::size_t sceneCandidate = ~std::size_t{};
    for(std::size_t i = 0; i != _cachedScenes->scenes.size(); ++i) {
        const Containers::Optional<SceneData>& scene = _cachedScenes->scenes[i];
        if(scene && scene->is2D() && id < scene->mappingBound() && scene->parentFor(id)) {
            sceneCandidate = i;
            break;
        }
    }

    if(sceneCandidate == ~std::size_t{}) {
        Error{} << "Trade::AbstractImporter::object2D(): object" << id << "not found in any 2D scene hierarchy";
        return {};
    }

    const SceneData& scene = *_cachedScenes->scenes[sceneCandidate];

    ObjectFlags2D flags;
    Containers::Optional<Matrix3> transformation = scene.transformation2DFor(id);
    Containers::Optional<Containers::Triple<Vector2, Complex, Vector2>> trs = scene.translationRotationScaling2DFor(id);
    /* If the object has neither a TRS nor a transformation field, assign an
       empty TRS transform. Not a matrix, because a TRS is more flexible and
       thus more desired. */
    if(!transformation && !trs)
        trs.emplace(Vector2{}, Complex{}, Vector2{1.0f});
    if(trs)
        flags |= ObjectFlag2D::HasTranslationRotationScaling;

    std::vector<UnsignedInt> children; /* not const so we can move it */
    {
        Containers::Array<UnsignedLong> childrenArray = scene.childrenFor(id);
        children = {childrenArray.begin(), childrenArray.end()};
    }
    const Containers::Array<Containers::Pair<UnsignedInt, Int>> mesh = scene.meshesMaterialsFor(id);
    const Containers::Array<UnsignedInt> camera = scene.camerasFor(id);
    const Containers::Array<UnsignedInt> skin = scene.skinsFor(id);
    const Containers::Optional<const void*> importerState = scene.importerStateFor(id);

    /* All these should have at most 1 item as the SceneData got processed to
       have each object contain either just one mesh or one camera (materials
       are implicitly shared with a mesh, skins also). Thus it doesn't matter
       in which order we decide on the legacy object type. */
    CORRADE_INTERNAL_ASSERT(camera.size() + mesh.size() <= 1);

    if(!mesh.isEmpty()) {
        return Containers::pointer(flags & ObjectFlag2D::HasTranslationRotationScaling ?
            new MeshObjectData2D{Utility::move(children),
                trs->first(), trs->second(), trs->third(),
                mesh.front().first(), mesh.front().second(),
                skin.isEmpty() ? -1  : Int(skin.front()),
                importerState ? *importerState : nullptr} :
            new MeshObjectData2D{Utility::move(children),
                *transformation,
                mesh.front().first(), mesh.front().second(),
                skin ? Int(*skin) : -1,
                importerState ? *importerState : nullptr});
    }

    ObjectInstanceType2D instanceType;
    UnsignedInt instance;
    if(camera) {
        instanceType = ObjectInstanceType2D::Camera;
        instance = *camera;
    } else {
        instanceType = ObjectInstanceType2D::Empty;
        instance = UnsignedInt(-1); /* Old APIs, you suck! */
    }

    return Containers::pointer(flags & ObjectFlag2D::HasTranslationRotationScaling ?
        new ObjectData2D{Utility::move(children),
            trs->first(), trs->second(), trs->third(),
            instanceType, instance,
            importerState ? *importerState : nullptr} :
        new ObjectData2D{Utility::move(children),
            *transformation,
            instanceType, instance,
            importerState ? *importerState : nullptr});
}

Containers::Pointer<ObjectData2D> AbstractImporter::object2D(const std::string& name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::object2D(): no file opened", {});
    const Int id = doObject2DForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::object2D(): object" << name << "not found among" << doObject2DCount() << "entries";
        return {};
    }
    return object2D(id); /* not doObject2D(), so we get the range checks also */
}
CORRADE_IGNORE_DEPRECATED_POP

UnsignedInt AbstractImporter::object3DCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::object3DCount(): no file opened", {});
    CORRADE_IGNORE_DEPRECATED_PUSH
    return doObject3DCount();
    CORRADE_IGNORE_DEPRECATED_POP
}

UnsignedInt AbstractImporter::doObject3DCount() const {
    /* I know, I know. A cleaner option would be to populate this during
       openFile() / openData() but that would mean the backwards compatibility
       overhead is there even if not using the deprecated APIs, which is way
       worse than using this nasty hack in two places. */
    const_cast<AbstractImporter&>(*this).populateCachedScenes();
    return _cachedScenes->object3DCount;
}

Int AbstractImporter::object3DForName(const std::string& name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::object3DForName(): no file opened", {});
    CORRADE_IGNORE_DEPRECATED_PUSH
    const Int id = doObject3DForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doObject3DCount(),
        "Trade::AbstractImporter::object3DForName(): implementation-returned index" << id << "out of range for" << doObject3DCount() << "entries", {});
    CORRADE_IGNORE_DEPRECATED_POP
    return id;
}

Int AbstractImporter::doObject3DForName(const std::string& name) {
    /* Alias to the new interface. If it returns an ID that's larger than
       reported 3D object count, then it's probably for a 2D object instead
       -- ignore it in that case. Ideally this would be solved by checking if
       the ID is actually present in a 3D scene (and same in doObject3DName())
       but that's a lot of extra code for just a backwards compatibility
       feature that almost nobody needs. The only pre-existing 2D importer is
       PrimitiveImporter, which is rarely used. */
    const Long id = doObjectForName(name);
    CORRADE_IGNORE_DEPRECATED_PUSH
    return id < doObject3DCount() ? id : -1;
    CORRADE_IGNORE_DEPRECATED_POP
}

std::string AbstractImporter::object3DName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::object3DName(): no file opened", {});
    CORRADE_IGNORE_DEPRECATED_PUSH
    CORRADE_ASSERT(id < doObject3DCount(), "Trade::AbstractImporter::object3DName(): index" << id << "out of range for" << doObject3DCount() << "entries", {});
    return doObject3DName(id);
    CORRADE_IGNORE_DEPRECATED_POP
}

std::string AbstractImporter::doObject3DName(const UnsignedInt id) {
    /* Alias to the new interface if the ID is known to the new interface,
       return an empty string for objects that got newly added in order to make
       them single-functioning */
    if(id < doObjectCount()) return doObjectName(id);

    populateCachedScenes();
    for(UnsignedInt i = 0; i != _cachedScenes->scenes.size(); ++i) {
        if(!_cachedScenes->scenes[i] ||
           !_cachedScenes->scenes[i]->is3D() ||
           _cachedScenes->scenes[i]->mappingBound() <= id)
            continue;

        if(Containers::Optional<Long> parent = _cachedScenes->scenes[i]->parentFor(id))
            return doObjectName(*parent);
    }

    return "";
}

CORRADE_IGNORE_DEPRECATED_PUSH /* Clang doesn't warn, but GCC does */
Containers::Pointer<ObjectData3D> AbstractImporter::object3D(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::object3D(): no file opened", {});
    CORRADE_ASSERT(id < doObject3DCount(), "Trade::AbstractImporter::object3D(): index" << id << "out of range for" << doObject3DCount() << "entries", {});
    return doObject3D(id);
}

Containers::Pointer<ObjectData3D> AbstractImporter::doObject3D(const UnsignedInt id) {
    /* The code is mostly the same between doObject2D() and doObject3D(),
       except that the 2D variant has no lights. Attempting to unify the common
       code would be more complex than just having a slightly modified copy. */

    populateCachedScenes();

    /* Find the first 3D scene with this object, which we'll detect from the
       mapping bound reported for the scene, whether it's 2D or 3D, and a
       presence of a parent attribute. If a parent attribute is not present, it
       means the object isn't a part of this scene, in which case we skip it.
       It could also mean isn't a part of the hierarchy and is standalone
       (skyboxes, scene-wide properties), which the legacy API had no way to
       deal with anyway so ignoring those is fine. */
    std::size_t sceneCandidate = ~std::size_t{};
    for(std::size_t i = 0; i != _cachedScenes->scenes.size(); ++i) {
        const Containers::Optional<SceneData>& scene = _cachedScenes->scenes[i];
        if(scene && scene->is3D() && id < scene->mappingBound() && scene->parentFor(id)) {
            sceneCandidate = i;
            break;
        }
    }

    if(sceneCandidate == ~std::size_t{}) {
        Error{} << "Trade::AbstractImporter::object3D(): object" << id << "not found in any 3D scene hierarchy";
        return {};
    }

    const SceneData& scene = *_cachedScenes->scenes[sceneCandidate];

    ObjectFlags3D flags;
    Containers::Optional<Matrix4> transformation = scene.transformation3DFor(id);
    Containers::Optional<Containers::Triple<Vector3, Quaternion, Vector3>> trs = scene.translationRotationScaling3DFor(id);
    /* If the object has neither a TRS nor a transformation field, assign an
       empty TRS transform. Not a matrix, because a TRS is more flexible and
       thus more desired. */
    if(!transformation && !trs)
        trs.emplace(Vector3{}, Quaternion{}, Vector3{1.0f});
    if(trs)
        flags |= ObjectFlag3D::HasTranslationRotationScaling;

    std::vector<UnsignedInt> children; /* not const so we can move it */
    {
        Containers::Array<UnsignedLong> childrenArray = scene.childrenFor(id);
        children = {childrenArray.begin(), childrenArray.end()};
    }
    const Containers::Array<Containers::Pair<UnsignedInt, Int>> mesh = scene.meshesMaterialsFor(id);
    const Containers::Array<UnsignedInt> camera = scene.camerasFor(id);
    const Containers::Array<UnsignedInt> skin = scene.skinsFor(id);
    const Containers::Array<UnsignedInt> light = scene.lightsFor(id);
    const Containers::Optional<const void*> importerState = scene.importerStateFor(id);

    /* All these should have at most 1 item as the SceneData got processed to
       have each object contain either just one mesh, one camera or one light
       (materials are implicitly shared with a mesh, skins also). Thus it
       doesn't matter in which order we decide on the legacy object type. */
    CORRADE_INTERNAL_ASSERT(camera.size() + light.size() + mesh.size() <= 1);

    if(!mesh.isEmpty()) {
        return Containers::pointer(flags & ObjectFlag3D::HasTranslationRotationScaling ?
            new MeshObjectData3D{Utility::move(children),
                trs->first(), trs->second(), trs->third(),
                mesh.front().first(), mesh.front().second(),
                skin.isEmpty() ? -1  : Int(skin.front()),
                importerState ? *importerState : nullptr} :
            new MeshObjectData3D{Utility::move(children),
                *transformation,
                mesh.front().first(), mesh.front().second(),
                skin ? Int(*skin) : -1,
                importerState ? *importerState : nullptr});
    }

    ObjectInstanceType3D instanceType;
    UnsignedInt instance;
    if(camera) {
        instanceType = ObjectInstanceType3D::Camera;
        instance = *camera;
    } else if(light) {
        instanceType = ObjectInstanceType3D::Light;
        instance = *light;
    } else {
        instanceType = ObjectInstanceType3D::Empty;
        instance = UnsignedInt(-1); /* Old APIs, you suck! */
    }

    return Containers::pointer(flags & ObjectFlag3D::HasTranslationRotationScaling ?
        new ObjectData3D{Utility::move(children),
            trs->first(), trs->second(), trs->third(),
            instanceType, instance,
            importerState ? *importerState : nullptr} :
        new ObjectData3D{Utility::move(children),
            *transformation,
            instanceType, instance,
            importerState ? *importerState : nullptr});
}

Containers::Pointer<ObjectData3D> AbstractImporter::object3D(const std::string& name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::object3D(): no file opened", {});
    const Int id = doObject3DForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::object3D(): object" << name << "not found among" << doObject3DCount() << "entries";
        return {};
    }
    return object3D(id); /* not doObject3D(), so we get the range checks also */
}
CORRADE_IGNORE_DEPRECATED_POP
#endif

UnsignedInt AbstractImporter::skin2DCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::skin2DCount(): no file opened", {});
    return doSkin2DCount();
}

UnsignedInt AbstractImporter::doSkin2DCount() const { return 0; }

Int AbstractImporter::skin2DForName(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::skin2DForName(): no file opened", {});
    const Int id = doSkin2DForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doSkin2DCount(),
        "Trade::AbstractImporter::skin2DForName(): implementation-returned index" << id << "out of range for" << doSkin2DCount() << "entries", {});
    return id;
}

Int AbstractImporter::doSkin2DForName(Containers::StringView) { return -1; }

Containers::String AbstractImporter::skin2DName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::skin2DName(): no file opened", {});
    CORRADE_ASSERT(id < doSkin2DCount(), "Trade::AbstractImporter::skin2DName(): index" << id << "out of range for" << doSkin2DCount() << "entries", {});
    Containers::String name = doSkin2DName(id);
    CORRADE_ASSERT(name.isSmall() || !name.deleter(),
        "Trade::AbstractImporter::skin2DName(): implementation is not allowed to use a custom String deleter", {});
    return name;
}

Containers::String AbstractImporter::doSkin2DName(UnsignedInt) { return {}; }

Containers::Optional<SkinData2D> AbstractImporter::skin2D(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::skin2D(): no file opened", {});
    CORRADE_ASSERT(id < doSkin2DCount(), "Trade::AbstractImporter::skin2D(): index" << id << "out of range for" << doSkin2DCount() << "entries", {});
    Containers::Optional<SkinData2D> skin = doSkin2D(id);
    CORRADE_ASSERT(!skin || (
        (!skin->_jointData.deleter() || skin->_jointData.deleter() == static_cast<void(*)(UnsignedInt*, std::size_t)>(Implementation::nonOwnedArrayDeleter)) &&
        (!skin->_inverseBindMatrixData.deleter() || skin->_inverseBindMatrixData.deleter() == static_cast<void(*)(Matrix3*, std::size_t)>(Implementation::nonOwnedArrayDeleter))),
        "Trade::AbstractImporter::skin2D(): implementation is not allowed to use a custom Array deleter", {});
    return skin;
}

Containers::Optional<SkinData2D> AbstractImporter::doSkin2D(UnsignedInt) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::skin2D(): not implemented", {});
}

Containers::Optional<SkinData2D> AbstractImporter::skin2D(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::skin2D(): no file opened", {});
    const Int id = doSkin2DForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::skin2D(): skin" << name << "not found among" << doSkin2DCount() << "entries";
        return {};
    }
    return skin2D(id); /* not doSkin2D(), so we get the range checks also */
}

UnsignedInt AbstractImporter::skin3DCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::skin3DCount(): no file opened", {});
    return doSkin3DCount();
}

UnsignedInt AbstractImporter::doSkin3DCount() const { return 0; }

Int AbstractImporter::skin3DForName(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::skin3DForName(): no file opened", {});
    const Int id = doSkin3DForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doSkin3DCount(),
        "Trade::AbstractImporter::skin3DForName(): implementation-returned index" << id << "out of range for" << doSkin3DCount() << "entries", {});
    return id;
}

Int AbstractImporter::doSkin3DForName(Containers::StringView) { return -1; }

Containers::String AbstractImporter::skin3DName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::skin3DName(): no file opened", {});
    CORRADE_ASSERT(id < doSkin3DCount(), "Trade::AbstractImporter::skin3DName(): index" << id << "out of range for" << doSkin3DCount() << "entries", {});
    Containers::String name = doSkin3DName(id);
    CORRADE_ASSERT(name.isSmall() || !name.deleter(),
        "Trade::AbstractImporter::skin3DName(): implementation is not allowed to use a custom String deleter", {});
    return name;
}

Containers::String AbstractImporter::doSkin3DName(UnsignedInt) { return {}; }

Containers::Optional<SkinData3D> AbstractImporter::skin3D(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::skin3D(): no file opened", {});
    CORRADE_ASSERT(id < doSkin3DCount(), "Trade::AbstractImporter::skin3D(): index" << id << "out of range for" << doSkin3DCount() << "entries", {});
    Containers::Optional<SkinData3D> skin = doSkin3D(id);
    CORRADE_ASSERT(!skin || (
        (!skin->_jointData.deleter() || skin->_jointData.deleter() == static_cast<void(*)(UnsignedInt*, std::size_t)>(Implementation::nonOwnedArrayDeleter)) &&
        (!skin->_inverseBindMatrixData.deleter() || skin->_inverseBindMatrixData.deleter() == static_cast<void(*)(Matrix4*, std::size_t)>(Implementation::nonOwnedArrayDeleter))),
        "Trade::AbstractImporter::skin3D(): implementation is not allowed to use a custom Array deleter", {});
    return skin;
}

Containers::Optional<SkinData3D> AbstractImporter::doSkin3D(UnsignedInt) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::skin3D(): not implemented", {});
}

Containers::Optional<SkinData3D> AbstractImporter::skin3D(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::skin3D(): no file opened", {});
    const Int id = doSkin3DForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::skin3D(): skin" << name << "not found among" << doSkin3DCount() << "entries";
        return {};
    }
    return skin3D(id); /* not doSkin3D(), so we get the range checks also */
}

UnsignedInt AbstractImporter::meshCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::meshCount(): no file opened", {});
    return doMeshCount();
}

UnsignedInt AbstractImporter::doMeshCount() const { return 0; }

UnsignedInt AbstractImporter::meshLevelCount(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::meshLevelCount(): no file opened", {});
    CORRADE_ASSERT(id < doMeshCount(), "Trade::AbstractImporter::meshLevelCount(): index" << id << "out of range for" << doMeshCount() << "entries", {});
    const UnsignedInt out = doMeshLevelCount(id);
    CORRADE_ASSERT(out, "Trade::AbstractImporter::meshLevelCount(): implementation reported zero levels", {});
    return out;
}

UnsignedInt AbstractImporter::doMeshLevelCount(UnsignedInt) { return 1; }

Int AbstractImporter::meshForName(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::meshForName(): no file opened", {});
    const Int id = doMeshForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doMeshCount(),
        "Trade::AbstractImporter::meshForName(): implementation-returned index" << id << "out of range for" << doMeshCount() << "entries", {});
    return id;
}

Int AbstractImporter::doMeshForName(Containers::StringView) { return -1; }

Containers::String AbstractImporter::meshName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::meshName(): no file opened", {});
    CORRADE_ASSERT(id < doMeshCount(), "Trade::AbstractImporter::meshName(): index" << id << "out of range for" << doMeshCount() << "entries", {});
    Containers::String name = doMeshName(id);
    CORRADE_ASSERT(name.isSmall() || !name.deleter(),
        "Trade::AbstractImporter::meshName(): implementation is not allowed to use a custom String deleter", {});
    return name;
}

Containers::String AbstractImporter::doMeshName(UnsignedInt) { return {}; }

Containers::Optional<MeshData> AbstractImporter::mesh(const UnsignedInt id, const UnsignedInt level) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::mesh(): no file opened", {});
    CORRADE_ASSERT(id < doMeshCount(), "Trade::AbstractImporter::mesh(): index" << id << "out of range for" << doMeshCount() << "entries", {});
    #ifndef CORRADE_NO_ASSERT
    /* Check for the range only if requested level is nonzero, as
       meshLevelCount() is expected to return >= 1. This is done to prevent
       random assertions and messages from a doMeshLevelCount() to be printed
       (which are unlikely, but let's be consistent with what image*D() does). */
    if(level) {
        const UnsignedInt levelCount = doMeshLevelCount(id);
        CORRADE_ASSERT(levelCount, "Trade::AbstractImporter::mesh(): implementation reported zero levels", {});
        CORRADE_ASSERT(level < levelCount, "Trade::AbstractImporter::mesh(): level" << level << "out of range for" << levelCount << "entries", {});
    }
    #endif
    Containers::Optional<MeshData> mesh = doMesh(id, level);
    CORRADE_ASSERT(!mesh || (
        (!mesh->_indexData.deleter() || mesh->_indexData.deleter() == static_cast<void(*)(char*, std::size_t)>(Implementation::nonOwnedArrayDeleter) || mesh->_indexData.deleter() == ArrayAllocator<char>::deleter) &&
        (!mesh->_vertexData.deleter() || mesh->_vertexData.deleter() == static_cast<void(*)(char*, std::size_t)>(Implementation::nonOwnedArrayDeleter) || mesh->_vertexData.deleter() == ArrayAllocator<char>::deleter) &&
        (!mesh->_attributes.deleter() || mesh->_attributes.deleter() == static_cast<void(*)(MeshAttributeData*, std::size_t)>(Implementation::nonOwnedArrayDeleter))),
        "Trade::AbstractImporter::mesh(): implementation is not allowed to use a custom Array deleter", {});
    return mesh;
}

Containers::Optional<MeshData> AbstractImporter::doMesh(UnsignedInt, UnsignedInt) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::mesh(): not implemented", {});
}

Containers::Optional<MeshData> AbstractImporter::mesh(const Containers::StringView name, const UnsignedInt level) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::mesh(): no file opened", {});
    const Int id = doMeshForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::mesh(): mesh" << name << "not found among" << doMeshCount() << "entries";
        return {};
    }
    return mesh(id, level); /* not doMesh(), so we get the checks also */
}

MeshAttribute AbstractImporter::meshAttributeForName(const Containers::StringView name) {
    const MeshAttribute out = doMeshAttributeForName(name);
    CORRADE_ASSERT(out == MeshAttribute{} || isMeshAttributeCustom(out),
        "Trade::AbstractImporter::meshAttributeForName(): implementation-returned" << out << "is neither custom nor invalid", {});
    return out;
}

MeshAttribute AbstractImporter::doMeshAttributeForName(Containers::StringView) {
    return {};
}

Containers::String AbstractImporter::meshAttributeName(const MeshAttribute name) {
    CORRADE_ASSERT(isMeshAttributeCustom(name),
        "Trade::AbstractImporter::meshAttributeName():" << name << "is not custom", {});
    Containers::String out = doMeshAttributeName(name);
    CORRADE_ASSERT(out.isSmall() || !out.deleter(),
        "Trade::AbstractImporter::meshAttributeName(): implementation is not allowed to use a custom String deleter", {});
    return out;
}

Containers::String AbstractImporter::doMeshAttributeName(MeshAttribute) { return {}; }

#ifdef MAGNUM_BUILD_DEPRECATED
UnsignedInt AbstractImporter::mesh2DCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::mesh2DCount(): no file opened", {});
    CORRADE_IGNORE_DEPRECATED_PUSH
    return doMesh2DCount();
    CORRADE_IGNORE_DEPRECATED_POP
}

UnsignedInt AbstractImporter::doMesh2DCount() const { return 0; }

Int AbstractImporter::mesh2DForName(const std::string& name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::mesh2DForName(): no file opened", {});
    CORRADE_IGNORE_DEPRECATED_PUSH
    return doMesh2DForName(name);
    CORRADE_IGNORE_DEPRECATED_POP
}

Int AbstractImporter::doMesh2DForName(const std::string&) { return -1; }

std::string AbstractImporter::mesh2DName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::mesh2DName(): no file opened", {});
    CORRADE_IGNORE_DEPRECATED_PUSH
    CORRADE_ASSERT(id < doMesh2DCount(), "Trade::AbstractImporter::mesh2DName(): index" << id << "out of range for" << doMesh2DCount() << "entries", {});
    return doMesh2DName(id);
    CORRADE_IGNORE_DEPRECATED_POP
}

std::string AbstractImporter::doMesh2DName(UnsignedInt) { return {}; }

CORRADE_IGNORE_DEPRECATED_PUSH
Containers::Optional<MeshData2D> AbstractImporter::mesh2D(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::mesh2D(): no file opened", {});
    CORRADE_ASSERT(id < doMesh2DCount(), "Trade::AbstractImporter::mesh2D(): index" << id << "out of range for" << doMesh2DCount() << "entries", {});
    return doMesh2D(id);
}

Containers::Optional<MeshData2D> AbstractImporter::doMesh2D(UnsignedInt) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::mesh2D(): not implemented", {});
}
CORRADE_IGNORE_DEPRECATED_POP

UnsignedInt AbstractImporter::mesh3DCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::mesh3DCount(): no file opened", {});
    CORRADE_IGNORE_DEPRECATED_PUSH
    return doMesh3DCount();
    CORRADE_IGNORE_DEPRECATED_POP
}

UnsignedInt AbstractImporter::doMesh3DCount() const {
    return doMeshCount();
}

Int AbstractImporter::mesh3DForName(const std::string& name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::mesh3DForName(): no file opened", {});
    CORRADE_IGNORE_DEPRECATED_PUSH
    return doMesh3DForName(name);
    CORRADE_IGNORE_DEPRECATED_POP
}

Int AbstractImporter::doMesh3DForName(const std::string& name) {
    return doMeshForName(name);
}

std::string AbstractImporter::mesh3DName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::mesh3DName(): no file opened", {});
    CORRADE_IGNORE_DEPRECATED_PUSH
    CORRADE_ASSERT(id < doMesh3DCount(), "Trade::AbstractImporter::mesh3DName(): index" << id << "out of range for" << doMesh3DCount() << "entries", {});
    return doMesh3DName(id);
    CORRADE_IGNORE_DEPRECATED_POP
}

std::string AbstractImporter::doMesh3DName(const UnsignedInt id) {
    return doMeshName(id);
}

CORRADE_IGNORE_DEPRECATED_PUSH
Containers::Optional<MeshData3D> AbstractImporter::mesh3D(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::mesh3D(): no file opened", {});
    CORRADE_ASSERT(id < doMesh3DCount(), "Trade::AbstractImporter::mesh3D(): index" << id << "out of range for" << doMesh3DCount() << "entries", {});
    return doMesh3D(id);
}

Containers::Optional<MeshData3D> AbstractImporter::doMesh3D(const UnsignedInt id) {
    Containers::Optional<MeshData> out = doMesh(id, 0);
    if(out) return MeshData3D{*out};
    return Containers::NullOpt;
}
CORRADE_IGNORE_DEPRECATED_POP
#endif

UnsignedInt AbstractImporter::materialCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::materialCount(): no file opened", {});
    return doMaterialCount();
}

UnsignedInt AbstractImporter::doMaterialCount() const { return 0; }

Int AbstractImporter::materialForName(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::materialForName(): no file opened", {});
    const Int id = doMaterialForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doMaterialCount(),
        "Trade::AbstractImporter::materialForName(): implementation-returned index" << id << "out of range for" << doMaterialCount() << "entries", {});
    return id;
}

Int AbstractImporter::doMaterialForName(Containers::StringView) { return -1; }

Containers::String AbstractImporter::materialName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::materialName(): no file opened", {});
    CORRADE_ASSERT(id < doMaterialCount(), "Trade::AbstractImporter::materialName(): index" << id << "out of range for" << doMaterialCount() << "entries", {});
    Containers::String name = doMaterialName(id);
    CORRADE_ASSERT(name.isSmall() || !name.deleter(),
        "Trade::AbstractImporter::materialName(): implementation is not allowed to use a custom String deleter", {});
    return name;
}

Containers::String AbstractImporter::doMaterialName(UnsignedInt) { return {}; }

#ifndef MAGNUM_BUILD_DEPRECATED
Containers::Optional<MaterialData>
#else
Implementation::OptionalButAlsoPointer<MaterialData>
#endif
AbstractImporter::material(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::material(): no file opened", {});
    CORRADE_ASSERT(id < doMaterialCount(), "Trade::AbstractImporter::material(): index" << id << "out of range for" << doMaterialCount() << "entries", {});

    Containers::Optional<MaterialData> material = doMaterial(id);
    CORRADE_ASSERT(!material || (
        (!material->_data.deleter() || material->_data.deleter() == static_cast<void(*)(MaterialAttributeData*, std::size_t)>(Implementation::nonOwnedArrayDeleter)) &&
        (!material->_layerOffsets.deleter() || material->_layerOffsets.deleter() == static_cast<void(*)(UnsignedInt*, std::size_t)>(Implementation::nonOwnedArrayDeleter))),
        "Trade::AbstractImporter::material(): implementation is not allowed to use a custom Array deleter", {});

    /* GCC 4.8 and clang-cl needs an explicit conversion here */
    #ifdef MAGNUM_BUILD_DEPRECATED
    return Implementation::OptionalButAlsoPointer<MaterialData>{Utility::move(material)};
    #else
    return material;
    #endif
}

Containers::Optional<MaterialData> AbstractImporter::doMaterial(UnsignedInt) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::material(): not implemented", {});
}

#ifndef MAGNUM_BUILD_DEPRECATED
Containers::Optional<MaterialData>
#else
Implementation::OptionalButAlsoPointer<MaterialData>
#endif
AbstractImporter::material(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::material(): no file opened", {});
    const Int id = doMaterialForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::material(): material" << name << "not found among" << doMaterialCount() << "entries";
        return {};
    }
    return material(id); /* not doMaterial(), so we get the range checks also */
}

UnsignedInt AbstractImporter::textureCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::textureCount(): no file opened", {});
    return doTextureCount();
}

UnsignedInt AbstractImporter::doTextureCount() const { return 0; }

Int AbstractImporter::textureForName(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::textureForName(): no file opened", {});
    const Int id = doTextureForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doTextureCount(),
        "Trade::AbstractImporter::textureForName(): implementation-returned index" << id << "out of range for" << doTextureCount() << "entries", {});
    return id;
}

Int AbstractImporter::doTextureForName(Containers::StringView) { return -1; }

Containers::String AbstractImporter::textureName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::textureName(): no file opened", {});
    CORRADE_ASSERT(id < doTextureCount(), "Trade::AbstractImporter::textureName(): index" << id << "out of range for" << doTextureCount() << "entries", {});
    Containers::String name = doTextureName(id);
    CORRADE_ASSERT(name.isSmall() || !name.deleter(),
        "Trade::AbstractImporter::textureName(): implementation is not allowed to use a custom String deleter", {});
    return name;
}

Containers::String AbstractImporter::doTextureName(UnsignedInt) { return {}; }

Containers::Optional<TextureData> AbstractImporter::texture(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::texture(): no file opened", {});
    CORRADE_ASSERT(id < doTextureCount(), "Trade::AbstractImporter::texture(): index" << id << "out of range for" << doTextureCount() << "entries", {});
    return doTexture(id);
}

Containers::Optional<TextureData> AbstractImporter::doTexture(UnsignedInt) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::texture(): not implemented", {});
}

Containers::Optional<TextureData> AbstractImporter::texture(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::texture(): no file opened", {});
    const Int id = doTextureForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::texture(): texture" << name << "not found among" << doTextureCount() << "entries";
        return {};
    }
    return texture(id); /* not doTexture(), so we get the range checks also */
}

UnsignedInt AbstractImporter::image1DCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image1DCount(): no file opened", {});
    return doImage1DCount();
}

UnsignedInt AbstractImporter::doImage1DCount() const { return 0; }

UnsignedInt AbstractImporter::image1DLevelCount(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image1DLevelCount(): no file opened", {});
    CORRADE_ASSERT(id < doImage1DCount(), "Trade::AbstractImporter::image1DLevelCount(): index" << id << "out of range for" << doImage1DCount() << "entries", {});
    const UnsignedInt out = doImage1DLevelCount(id);
    CORRADE_ASSERT(out, "Trade::AbstractImporter::image1DLevelCount(): implementation reported zero levels", {});
    return out;
}

UnsignedInt AbstractImporter::doImage1DLevelCount(UnsignedInt) { return 1; }

Int AbstractImporter::image1DForName(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image1DForName(): no file opened", {});
    const Int id = doImage1DForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doImage1DCount(),
        "Trade::AbstractImporter::image1DForName(): implementation-returned index" << id << "out of range for" << doImage1DCount() << "entries", {});
    return id;
}

Int AbstractImporter::doImage1DForName(Containers::StringView) { return -1; }

Containers::String AbstractImporter::image1DName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image1DName(): no file opened", {});
    CORRADE_ASSERT(id < doImage1DCount(), "Trade::AbstractImporter::image1DName(): index" << id << "out of range for" << doImage1DCount() << "entries", {});
    Containers::String name = doImage1DName(id);
    CORRADE_ASSERT(name.isSmall() || !name.deleter(),
        "Trade::AbstractImporter::image1DName(): implementation is not allowed to use a custom String deleter", {});
    return name;
}

Containers::String AbstractImporter::doImage1DName(UnsignedInt) { return {}; }

Containers::Optional<ImageData1D> AbstractImporter::image1D(const UnsignedInt id, const UnsignedInt level) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image1D(): no file opened", {});
    CORRADE_ASSERT(id < doImage1DCount(), "Trade::AbstractImporter::image1D(): index" << id << "out of range for" << doImage1DCount() << "entries", {});
    #ifndef CORRADE_NO_ASSERT
    /* Check for the range only if requested level is nonzero, as
       image*DLevelCount() is expected to return >= 1. This is done to prevent
       random assertions and messages from a doImage*DLevelCount() to be
       printed (e.g., many plugins delegate image loading and assert an access
       to the manager for that), which may be confusing */
    if(level) {
        const UnsignedInt levelCount = doImage1DLevelCount(id);
        CORRADE_ASSERT(levelCount, "Trade::AbstractImporter::image1D(): implementation reported zero levels", {});
        CORRADE_ASSERT(level < levelCount, "Trade::AbstractImporter::image1D(): level" << level << "out of range for" << levelCount << "entries", {});
    }
    #endif
    Containers::Optional<ImageData1D> image = doImage1D(id, level);
    CORRADE_ASSERT(!image || !image->_data.deleter() || image->_data.deleter() == static_cast<void(*)(char*, std::size_t)>(Implementation::nonOwnedArrayDeleter) || image->_data.deleter() == ArrayAllocator<char>::deleter, "Trade::AbstractImporter::image1D(): implementation is not allowed to use a custom Array deleter", {});
    return image;
}

Containers::Optional<ImageData1D> AbstractImporter::doImage1D(UnsignedInt, UnsignedInt) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::image1D(): not implemented", {});
}

Containers::Optional<ImageData1D> AbstractImporter::image1D(const Containers::StringView name, const UnsignedInt level) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image1D(): no file opened", {});
    const Int id = doImage1DForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::image1D(): image" << name << "not found among" << doImage1DCount() << "entries";
        return {};
    }
    /* not doImage1D(), so we get the range checks also */
    return image1D(id, level);
}

UnsignedInt AbstractImporter::image2DCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image2DCount(): no file opened", {});
    return doImage2DCount();
}

UnsignedInt AbstractImporter::doImage2DCount() const { return 0; }

UnsignedInt AbstractImporter::image2DLevelCount(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image2DLevelCount(): no file opened", {});
    CORRADE_ASSERT(id < doImage2DCount(), "Trade::AbstractImporter::image2DLevelCount(): index" << id << "out of range for" << doImage2DCount() << "entries", {});
    const UnsignedInt out = doImage2DLevelCount(id);
    CORRADE_ASSERT(out, "Trade::AbstractImporter::image2DLevelCount(): implementation reported zero levels", {});
    return out;
}

UnsignedInt AbstractImporter::doImage2DLevelCount(UnsignedInt) { return 1; }

Int AbstractImporter::image2DForName(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image2DForName(): no file opened", {});
    const Int id = doImage2DForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doImage2DCount(),
        "Trade::AbstractImporter::image2DForName(): implementation-returned index" << id << "out of range for" << doImage2DCount() << "entries", {});
    return id;
}

Int AbstractImporter::doImage2DForName(Containers::StringView) { return -1; }

Containers::String AbstractImporter::image2DName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image2DName(): no file opened", {});
    CORRADE_ASSERT(id < doImage2DCount(), "Trade::AbstractImporter::image2DName(): index" << id << "out of range for" << doImage2DCount() << "entries", {});
    Containers::String name = doImage2DName(id);
    CORRADE_ASSERT(name.isSmall() || !name.deleter(),
        "Trade::AbstractImporter::image2DName(): implementation is not allowed to use a custom String deleter", {});
    return name;
}

Containers::String AbstractImporter::doImage2DName(UnsignedInt) { return {}; }

Containers::Optional<ImageData2D> AbstractImporter::image2D(const UnsignedInt id, const UnsignedInt level) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image2D(): no file opened", {});
    CORRADE_ASSERT(id < doImage2DCount(), "Trade::AbstractImporter::image2D(): index" << id << "out of range for" << doImage2DCount() << "entries", {});
    #ifndef CORRADE_NO_ASSERT
    /* Check for the range only if requested level is nonzero, as
       image*DLevelCount() is expected to return >= 1. This is done to prevent
       random assertions and messages from a doImage*DLevelCount() to be
       printed (e.g., many plugins delegate image loading and assert an access
       to the manager for that), which may be confusing */
    if(level) {
        const UnsignedInt levelCount = doImage2DLevelCount(id);
        CORRADE_ASSERT(levelCount, "Trade::AbstractImporter::image2D(): implementation reported zero levels", {});
        CORRADE_ASSERT(level < levelCount, "Trade::AbstractImporter::image2D(): level" << level << "out of range for" << levelCount << "entries", {});
    }
    #endif
    Containers::Optional<ImageData2D> image = doImage2D(id, level);
    CORRADE_ASSERT(!image || !image->_data.deleter() || image->_data.deleter() == static_cast<void(*)(char*, std::size_t)>(Implementation::nonOwnedArrayDeleter) || image->_data.deleter() == ArrayAllocator<char>::deleter, "Trade::AbstractImporter::image2D(): implementation is not allowed to use a custom Array deleter", {});
    return image;
}

Containers::Optional<ImageData2D> AbstractImporter::doImage2D(UnsignedInt, UnsignedInt) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::image2D(): not implemented", {});
}

Containers::Optional<ImageData2D> AbstractImporter::image2D(const Containers::StringView name, const UnsignedInt level) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image2D(): no file opened", {});
    const Int id = doImage2DForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::image2D(): image" << name << "not found among" << doImage2DCount() << "entries";
        return {};
    }
    /* not doImage2D(), so we get the range checks also */
    return image2D(id, level);
}

UnsignedInt AbstractImporter::image3DCount() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image3DCount(): no file opened", {});
    return doImage3DCount();
}

UnsignedInt AbstractImporter::doImage3DCount() const { return 0; }

UnsignedInt AbstractImporter::image3DLevelCount(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image3DLevelCount(): no file opened", {});
    CORRADE_ASSERT(id < doImage3DCount(), "Trade::AbstractImporter::image3DLevelCount(): index" << id << "out of range for" << doImage3DCount() << "entries", {});
    const UnsignedInt out = doImage3DLevelCount(id);
    CORRADE_ASSERT(out, "Trade::AbstractImporter::image3DLevelCount(): implementation reported zero levels", {});
    return out;
}

UnsignedInt AbstractImporter::doImage3DLevelCount(UnsignedInt) { return 1; }

Int AbstractImporter::image3DForName(const Containers::StringView name) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image3DForName(): no file opened", {});
    const Int id = doImage3DForName(name);
    CORRADE_ASSERT(id == -1 || UnsignedInt(id) < doImage3DCount(),
        "Trade::AbstractImporter::image3DForName(): implementation-returned index" << id << "out of range for" << doImage3DCount() << "entries", {});
    return id;
}

Int AbstractImporter::doImage3DForName(Containers::StringView) { return -1; }

Containers::String AbstractImporter::image3DName(const UnsignedInt id) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image3DName(): no file opened", {});
    CORRADE_ASSERT(id < doImage3DCount(), "Trade::AbstractImporter::image3DName(): index" << id << "out of range for" << doImage3DCount() << "entries", {});
    Containers::String name = doImage3DName(id);
    CORRADE_ASSERT(name.isSmall() || !name.deleter(),
        "Trade::AbstractImporter::image3DName(): implementation is not allowed to use a custom String deleter", {});
    return name;
}

Containers::String AbstractImporter::doImage3DName(UnsignedInt) { return {}; }

Containers::Optional<ImageData3D> AbstractImporter::image3D(const UnsignedInt id, const UnsignedInt level) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image3D(): no file opened", {});
    CORRADE_ASSERT(id < doImage3DCount(), "Trade::AbstractImporter::image3D(): index" << id << "out of range for" << doImage3DCount() << "entries", {});
    #ifndef CORRADE_NO_ASSERT
    /* Check for the range only if requested level is nonzero, as
       image*DLevelCount() is expected to return >= 1. This is done to prevent
       random assertions and messages from a doImage*DLevelCount() to be
       printed (e.g., many plugins delegate image loading and assert an access
       to the manager for that), which may be confusing */
    if(level) {
        const UnsignedInt levelCount = doImage3DLevelCount(id);
        CORRADE_ASSERT(levelCount, "Trade::AbstractImporter::image3D(): implementation reported zero levels", {});
        CORRADE_ASSERT(level < levelCount, "Trade::AbstractImporter::image3D(): level" << level << "out of range for" << levelCount << "entries", {});
    }
    #endif
    Containers::Optional<ImageData3D> image = doImage3D(id, level);
    CORRADE_ASSERT(!image || !image->_data.deleter() || image->_data.deleter() == static_cast<void(*)(char*, std::size_t)>(Implementation::nonOwnedArrayDeleter) || image->_data.deleter() == ArrayAllocator<char>::deleter, "Trade::AbstractImporter::image3D(): implementation is not allowed to use a custom Array deleter", {});
    return image;
}

Containers::Optional<ImageData3D> AbstractImporter::doImage3D(UnsignedInt, UnsignedInt) {
    CORRADE_ASSERT_UNREACHABLE("Trade::AbstractImporter::image3D(): not implemented", {});
}

Containers::Optional<ImageData3D> AbstractImporter::image3D(const Containers::StringView name, const UnsignedInt level) {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::image3D(): no file opened", {});
    const Int id = doImage3DForName(name);
    if(id == -1) {
        Error{} << "Trade::AbstractImporter::image3D(): image" << name << "not found among" << doImage3DCount() << "entries";
        return {};
    }
    /* not doImage3D(), so we get the range checks also */
    return image3D(id, level);
}

const void* AbstractImporter::importerState() const {
    CORRADE_ASSERT(isOpened(), "Trade::AbstractImporter::importerState(): no file opened", {});
    return doImporterState();
}

const void* AbstractImporter::doImporterState() const { return nullptr; }

Debug& operator<<(Debug& debug, const ImporterFeature value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(!packed)
        debug << "Trade::ImporterFeature" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(v) case ImporterFeature::v: return debug << (packed ? "" : "::") << Debug::nospace << #v;
        _c(OpenData)
        _c(OpenState)
        _c(FileCallback)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << (packed ? "" : "(") << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << (packed ? "" : ")");
}

Debug& operator<<(Debug& debug, const ImporterFeatures value) {
    return Containers::enumSetDebugOutput(debug, value, debug.immediateFlags() >= Debug::Flag::Packed ? "{}" : "Trade::ImporterFeatures{}", {
        ImporterFeature::OpenData,
        ImporterFeature::OpenState,
        ImporterFeature::FileCallback});
}

Debug& operator<<(Debug& debug, const ImporterFlag value) {
    debug << "Trade::ImporterFlag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(v) case ImporterFlag::v: return debug << "::" #v;
        _c(Quiet)
        _c(Verbose)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const ImporterFlags value) {
    return Containers::enumSetDebugOutput(debug, value, "Trade::ImporterFlags{}", {
        ImporterFlag::Quiet,
        ImporterFlag::Verbose});
}

}}
