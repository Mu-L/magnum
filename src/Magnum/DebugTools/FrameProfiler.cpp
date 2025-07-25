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

#include "FrameProfiler.h"

#include <chrono>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/StaticArray.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/Utility/Format.h>

#include "Magnum/Math/Functions.h"
#ifdef MAGNUM_TARGET_GL
#include "Magnum/GL/TimeQuery.h"
#ifndef MAGNUM_TARGET_GLES
#include "Magnum/GL/PipelineStatisticsQuery.h"
#endif
#endif

namespace Magnum { namespace DebugTools {

using namespace Containers::Literals;

FrameProfiler::Measurement::Measurement(const Containers::StringView name, const Units units, void(*const begin)(void*), UnsignedLong(*const end)(void*), void* const state): _name{Containers::String::nullTerminatedGlobalView(name)}, _end{nullptr}, _state{state}, _units{units}, _delay{0} {
    _begin.immediate = begin;
    _query.immediate = end;
}

FrameProfiler::Measurement::Measurement(const Containers::StringView name, const Units units, const UnsignedInt delay, void(*const begin)(void*, UnsignedInt), void(*const end)(void*, UnsignedInt), UnsignedLong(*const query)(void*, UnsignedInt, UnsignedInt), void* const state): _name{Containers::String::nullTerminatedGlobalView(name)}, _state{state}, _units{units}, _delay{delay} {
    CORRADE_ASSERT(delay >= 1, "DebugTools::FrameProfiler::Measurement: delay can't be zero", );
    _begin.delayed = begin;
    _end = end;
    _query.delayed = query;
}

FrameProfiler::FrameProfiler() noexcept = default;

FrameProfiler::FrameProfiler(Containers::Array<Measurement>&& measurements, UnsignedInt maxFrameCount) noexcept {
    setup(Utility::move(measurements), maxFrameCount);
}

FrameProfiler::FrameProfiler(const std::initializer_list<Measurement> measurements, const UnsignedInt maxFrameCount): FrameProfiler{Containers::array(measurements), maxFrameCount} {}

FrameProfiler::FrameProfiler(FrameProfiler&& other) noexcept:
    _enabled{other._enabled},
    #ifndef CORRADE_NO_ASSERT
    _beginFrameCalled{other._beginFrameCalled},
    #endif
    _maxFrameCount{other._maxFrameCount},
    _measuredFrameCount{other._measuredFrameCount},
    _measurements{Utility::move(other._measurements)},
    _data{Utility::move(other._data)}
{
    /* For all state pointers that point to &other patch them to point to this
       instead, to account for 90% of use cases of derived classes */
    for(Measurement& measurement: _measurements)
        if(measurement._state == &other) measurement._state = this;
}

FrameProfiler& FrameProfiler::operator=(FrameProfiler&& other) noexcept {
    using Utility::swap;
    swap(_enabled, other._enabled);
    #ifndef CORRADE_NO_ASSERT
    swap(_beginFrameCalled, other._beginFrameCalled);
    #endif
    swap(_maxFrameCount, other._maxFrameCount);
    swap(_measuredFrameCount, other._measuredFrameCount);
    swap(_measurements, other._measurements);
    swap(_data, other._data);

    /* For all state pointers that point to &other patch them to point to this
       instead, to account for 90% of use cases of derived classes */
    for(Measurement& measurement: _measurements)
        if(measurement._state == &other) measurement._state = this;

    /* And the same the other way to avoid the other instance accidentally
       affecting our measurements */
    for(Measurement& measurement: other._measurements)
        if(measurement._state == this) measurement._state = &other;

    return *this;
}

void FrameProfiler::setup(Containers::Array<Measurement>&& measurements, const UnsignedInt maxFrameCount) {
    CORRADE_ASSERT(maxFrameCount >= 1, "DebugTools::FrameProfiler::setup(): max frame count can't be zero", );

    _maxFrameCount = maxFrameCount;
    _measurements = Utility::move(measurements);
    arrayReserve(_data, maxFrameCount*_measurements.size());

    #ifndef CORRADE_NO_ASSERT
    for(const Measurement& measurement: _measurements) {
        /* Max frame count is always >= 1, so even if _delay is 0 the condition
           makes sense and we don't need to do a max() */
        CORRADE_ASSERT(maxFrameCount >= measurement._delay,
            "DebugTools::FrameProfiler::setup(): max delay" << measurement._delay << "is larger than max frame count" << maxFrameCount, );
    }
    #endif

    /* Reset to a clean slate in case we did some other measurements before,
       then either disable or enable based on whether we have anything to
       measure */
    reset();
    _measurements.isEmpty() ? disable() : enable();
}

void FrameProfiler::setup(const std::initializer_list<Measurement> measurements, const UnsignedInt maxFrameCount) {
    setup(Containers::array(measurements), maxFrameCount);
}

void FrameProfiler::reset() {
    #ifndef CORRADE_NO_ASSERT
    _beginFrameCalled = false;
    #endif
    _measuredFrameCount = 0;
    arrayClear(_data);

    /* Wipe out no longer relevant moving sums from all measurements, and
       delayed measurement indices as well (tho for these it's not so
       important) */
    for(Measurement& measurement: _measurements) {
        measurement._movingSum = 0;
        measurement._current = 0;
    }
}

void FrameProfiler::enable() {
    reset();
    _enabled = true;
}

void FrameProfiler::disable() {
    _enabled = false;
}

void FrameProfiler::beginFrame() {
    if(!_enabled)
        return;

    CORRADE_ASSERT(!_beginFrameCalled, "DebugTools::FrameProfiler::beginFrame(): expected end of frame", );
    #ifndef CORRADE_NO_ASSERT
    _beginFrameCalled = true;
    #endif

    /* For all measurements call the begin function */
    for(const Measurement& measurement: _measurements) {
        if(!measurement._delay)
            measurement._begin.immediate(measurement._state);
        else
            measurement._begin.delayed(measurement._state, measurement._current);
    }
}

UnsignedInt FrameProfiler::delayedCurrentData(UnsignedInt delay) const {
    CORRADE_INTERNAL_ASSERT(delay >= 1);
    return (_measuredFrameCount - delay) % _maxFrameCount;
}

void FrameProfiler::endFrame() {
    if(!_enabled)
        return;

    CORRADE_ASSERT(_beginFrameCalled, "DebugTools::FrameProfiler::endFrame(): expected begin of frame", );
    #ifndef CORRADE_NO_ASSERT
    _beginFrameCalled = false;
    #endif

    /* If we don't have all frames yet, enlarge the array */
    if(++_measuredFrameCount <= _maxFrameCount)
        arrayAppend(_data, NoInit, _measurements.size());

    /* Wrap up measurements for this frame */
    for(std::size_t i = 0; i != _measurements.size(); ++i) {
        Measurement& measurement = _measurements[i];
        const UnsignedInt measurementDelay = Math::max(1u, measurement._delay);

        /* If we're wrapping around, subtract the oldest data from the moving
           average so we can reuse the memory for currently queried data */
        if(_measuredFrameCount > _maxFrameCount + measurementDelay - 1) {
            UnsignedLong& currentMeasurementData = _data[delayedCurrentData(measurementDelay)*_measurements.size() + i];
            CORRADE_INTERNAL_ASSERT(measurement._movingSum >= currentMeasurementData);
            measurement._movingSum -= currentMeasurementData;
        }

        /* Simply save the data if not delayed */
        if(!measurement._delay)
            _data[delayedCurrentData(measurementDelay)*_measurements.size() + i] = measurement._query.immediate(measurement._state);

        /* For delayed measurements call the end function for current frame and
           then save the data for the delayed frame */
        else {
            measurement._end(measurement._state, measurement._current);

            /* The slot from which we just retrieved a delayed value will be
               reused for a a new value next frame */
            const UnsignedInt previous = (measurement._current + 1) % measurement._delay;
            if(_measuredFrameCount >= measurement._delay) {
                _data[delayedCurrentData(measurementDelay)*_measurements.size() + i] =
                    measurement._query.delayed(measurement._state, previous, measurement._current);
            }
            measurement._current = previous;
        }
    }

    /* Process the new data if we have enough frames even for the largest
       delay */
    for(std::size_t i = 0; i != _measurements.size(); ++i) {
        Measurement& measurement = _measurements[i];
        const UnsignedInt measurementDelay = Math::max(1u, measurement._delay);

        /* If we have enough frames, add the new measurement to the moving
           sum */
        if(_measuredFrameCount >= measurementDelay) {
            const UnsignedLong data = _data[delayedCurrentData(measurementDelay)*_measurements.size() + i];
            CORRADE_INTERNAL_ASSERT(_measurements[i]._movingSum + data >= _measurements[i]._movingSum);
            _measurements[i]._movingSum += data;
        }
    }
}

Containers::StringView FrameProfiler::measurementName(const UnsignedInt id) const {
    CORRADE_ASSERT(id < _measurements.size(),
        "DebugTools::FrameProfiler::measurementName(): index" << id << "out of range for" << _measurements.size() << "measurements", {});
    return _measurements[id]._name;
}

FrameProfiler::Units FrameProfiler::measurementUnits(const UnsignedInt id) const {
    CORRADE_ASSERT(id < _measurements.size(),
        "DebugTools::FrameProfiler::measurementUnits(): index" << id << "out of range for" << _measurements.size() << "measurements", {});
    return _measurements[id]._units;
}

UnsignedInt FrameProfiler::measurementDelay(const UnsignedInt id) const {
    CORRADE_ASSERT(id < _measurements.size(),
        "DebugTools::FrameProfiler::measurementDelay(): index" << id << "out of range for" << _measurements.size() << "measurements", {});
    return Math::max(_measurements[id]._delay, 1u);
}

bool FrameProfiler::isMeasurementAvailable(const UnsignedInt id) const {
    CORRADE_ASSERT(id < _measurements.size(),
        "DebugTools::FrameProfiler::measurementDelay(): index" << id << "out of range for" << _measurements.size() << "measurements", {});
    return _measuredFrameCount >= Math::max(_measurements[id]._delay, 1u);
}

UnsignedLong FrameProfiler::measurementData(const UnsignedInt id, const UnsignedInt frame) const {
    CORRADE_ASSERT(id < _measurements.size(),
        "DebugTools::FrameProfiler::measurementData(): index" << id << "out of range for" << _measurements.size() << "measurements", {});
    CORRADE_ASSERT(frame < _maxFrameCount,
        "DebugTools::FrameProfiler::measurementData(): frame" << frame << "out of range for max" << _maxFrameCount << "frames", {});
    CORRADE_ASSERT(_measuredFrameCount >= Math::max(_measurements[id]._delay, 1u) && frame <= _measuredFrameCount - Math::max(_measurements[id]._delay, 1u),
        "DebugTools::FrameProfiler::measurementData(): frame" << frame << "of measurement" << id << "not available yet (delay" << Math::max(_measurements[id]._delay, 1u) << Debug::nospace << "," << _measuredFrameCount << "frames measured so far)", {});

    /* We're returning data from the previous maxFrameCount. If the full range
       is not available, cap that only to the count of actually measured frames
       minus the delay. */
    return _data[((_measuredFrameCount - Math::min(_maxFrameCount + Math::max(_measurements[id]._delay, 1u) - 1, _measuredFrameCount) + frame) % _maxFrameCount)*_measurements.size() + id];
}

Double FrameProfiler::measurementMeanInternal(const Measurement& measurement) const {
    return Double(measurement._movingSum)/
        Math::min(_measuredFrameCount - Math::max(measurement._delay, 1u) + 1, _maxFrameCount);
}

Double FrameProfiler::measurementMean(const UnsignedInt id) const {
    CORRADE_ASSERT(id < _measurements.size(),
        "DebugTools::FrameProfiler::measurementMean(): index" << id << "out of range for" << _measurements.size() << "measurements", {});
    CORRADE_ASSERT(_measuredFrameCount >= Math::max(_measurements[id]._delay, 1u), "DebugTools::FrameProfiler::measurementMean(): measurement data available after" << Math::max(_measurements[id]._delay, 1u) - _measuredFrameCount << "more frames", {});

    return measurementMeanInternal(_measurements[id]);
}

namespace {

/* Based on Corrade/TestSuite/Implementation/BenchmarkStats.h */

void printValue(Utility::Debug& out, const Double mean, const Double divisor, const char* const unitPrefix, const char* const units) {
    out << Debug::boldColor(Debug::Color::Green)
        << Utility::format("{:.2f}", mean/divisor) << Debug::resetColor
        << Debug::nospace << unitPrefix << Debug::nospace << units;
}

void printTime(Utility::Debug& out, const Double mean) {
    if(mean >= 1000000000.0)
        printValue(out, mean, 1000000000.0, " ", "s");
    else if(mean >= 1000000.0)
        printValue(out, mean, 1000000.0, " m", "s");
    else if(mean >= 1000.0)
        printValue(out, mean, 1000.0, " µ", "s");
    else
        printValue(out, mean, 1.0, " n", "s");
}

void printCount(Utility::Debug& out, const Double mean, Double multiplier, const char* const units) {
    if(mean >= multiplier*multiplier*multiplier)
        printValue(out, mean, multiplier*multiplier*multiplier, " G", units);
    else if(mean >= multiplier*multiplier)
        printValue(out, mean, multiplier*multiplier, " M", units);
    else if(mean >= multiplier)
        printValue(out, mean, multiplier, " k", units);
    else
        printValue(out, mean, 1.0, std::strlen(units) ? " " : "", units);
}

}

void FrameProfiler::printStatisticsInternal(Debug& out) const {
    out << Debug::boldColor(Debug::Color::Default) << "Last"
        << Debug::boldColor(Debug::Color::Cyan)
        << Math::min(_measuredFrameCount, _maxFrameCount)
        << Debug::boldColor(Debug::Color::Default) << "frames:";

    for(const Measurement& measurement: _measurements) {
        out << Debug::newline << " " << Debug::boldColor(Debug::Color::Default)
            << measurement._name << Debug::nospace << ":" << Debug::resetColor;

        /* If this measurement is not available yet, print a placeholder */
        if(_measuredFrameCount < Math::max(measurement._delay, 1u)) {
            const char* units = nullptr;
            switch(measurement._units) {
                case Units::Count:
                case Units::RatioThousandths:
                    units = "";
                    break;
                case Units::Nanoseconds:
                    units = "s";
                    break;
                case Units::Bytes:
                    units = "B";
                    break;
                case Units::PercentageThousandths:
                    units = "%";
                    break;
            }
            CORRADE_INTERNAL_ASSERT(units);

            out << Debug::color(Debug::Color::Blue) << "-.--"
                << Debug::resetColor;
            if(units[0] != '\0') out << units;

        /* Otherwise format the value */
        } else {
            const Double mean = measurementMeanInternal(measurement);
            switch(measurement._units) {
                case Units::Nanoseconds:
                    printTime(out, mean);
                    continue;
                case Units::Bytes:
                    printCount(out, mean, 1024.0, "B");
                    continue;
                case Units::Count:
                    printCount(out, mean, 1000.0, "");
                    continue;
                case Units::RatioThousandths:
                    printCount(out, mean/1000.0, 1000.0, "");
                    continue;
                case Units::PercentageThousandths:
                    printValue(out, mean, 1000.0, " ", "%");
                    continue;
            }

            CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
        }
    }
}

Containers::String FrameProfiler::statistics() const {
    Containers::String out;
    /* While both GCC and Clang do the right thing here and call Debug
       destructor before the String destructor, causing the printed contents to
       be correctly flushed, MSVC doesn't, leading to the output being empty.
       This only seems to be fixed with MSVC 2022 and /permissive-, without the
       flag it's broken too. No idea what's going on, maybe it's some stupid
       interaction of return value move elision and the other destructor or
       whatever. Ugh. */
    {
        Debug d{&out, Debug::Flag::NoNewlineAtTheEnd|Debug::Flag::DisableColors};
        printStatisticsInternal(d);
    }
    return out;
}

void FrameProfiler::printStatistics(const UnsignedInt frequency) const {
    Debug::Flags flags;
    if(!Debug::isTty()) flags |= Debug::Flag::DisableColors;
    printStatistics(Debug{flags}, frequency);
}

void FrameProfiler::printStatistics(Debug& out, const UnsignedInt frequency) const {
    if(!isEnabled() || _measuredFrameCount % frequency != 0)
        return;

    /* If on a TTY and we printed at least something already, scroll back up to
       overwrite previous output */
    if(out.isTty() && _measuredFrameCount > frequency)
        out << Debug::nospace << "\033[" << Debug::nospace
            << _measurements.size() + 1 << Debug::nospace << "A\033[J"
            << Debug::nospace;

    printStatisticsInternal(out);

    /* Unconditionally finish with a newline so the TTY scrollback works
       correctly */
    if(out.flags() & Debug::Flag::NoNewlineAtTheEnd)
        out << Debug::newline;
}

Debug& operator<<(Debug& debug, const FrameProfiler::Units value) {
    debug << "DebugTools::FrameProfiler::Units" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(v) case FrameProfiler::Units::v: return debug << "::" #v;
        _c(Nanoseconds)
        _c(Bytes)
        _c(Count)
        _c(RatioThousandths)
        _c(PercentageThousandths)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

#ifdef MAGNUM_TARGET_GL
struct FrameProfilerGL::State {
    UnsignedShort cpuDurationIndex = 0xffff,
        gpuDurationIndex = 0xffff,
        frameTimeIndex = 0xffff;
    #ifndef MAGNUM_TARGET_GLES
    UnsignedShort vertexFetchRatioIndex = 0xffff,
        primitiveClipRatioIndex = 0xffff;
    #endif
    UnsignedLong frameTimeStartFrame[2];
    UnsignedLong cpuDurationStartFrame;

    enum: std::size_t { QueryCount = 3 };
    Containers::StaticArray<QueryCount, GL::TimeQuery> timeQueries{DirectInit, NoCreate};
    #ifndef MAGNUM_TARGET_GLES
    Containers::StaticArray<QueryCount, GL::PipelineStatisticsQuery> verticesSubmittedQueries{DirectInit, NoCreate};
    Containers::StaticArray<QueryCount, GL::PipelineStatisticsQuery> vertexShaderInvocationsQueries{DirectInit, NoCreate};
    Containers::StaticArray<QueryCount, GL::PipelineStatisticsQuery> clippingInputPrimitivesQueries{DirectInit, NoCreate};
    Containers::StaticArray<QueryCount, GL::PipelineStatisticsQuery> clippingOutputPrimitivesQueries{DirectInit, NoCreate};
    #endif
};

FrameProfilerGL::FrameProfilerGL(): _state{InPlaceInit} {}

FrameProfilerGL::FrameProfilerGL(const Values values, const UnsignedInt maxFrameCount): FrameProfilerGL{}
{
    setup(values, maxFrameCount);
}

FrameProfilerGL::FrameProfilerGL(FrameProfilerGL&&) noexcept = default;

FrameProfilerGL& FrameProfilerGL::operator=(FrameProfilerGL&&) noexcept = default;

FrameProfilerGL::~FrameProfilerGL() = default;

void FrameProfilerGL::setup(const Values values, const UnsignedInt maxFrameCount) {
    UnsignedShort index = 0;
    Containers::Array<Measurement> measurements;
    if(values & Value::FrameTime) {
        /* Fucking hell, STL. When I first saw std::chrono back in 2010 I
           should have flipped the table and learn carpentry instead. BUT NO,
           I'm still suffering this abomination a decade later! */
        arrayAppend(measurements, InPlaceInit,
            "Frame time"_s, Units::Nanoseconds, UnsignedInt(Containers::arraySize(_state->frameTimeStartFrame)),
            [](void* state, UnsignedInt current) {
                /* Using steady_clock even though it might not be as precise as
                   high_resolution_clock in order to avoid the delta being
                   negative between frames, causing an underflow and an assert
                   similar to what used to happen for PrimitiveClipRatio */
                static_cast<State*>(state)->frameTimeStartFrame[current] = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            },
            [](void*, UnsignedInt) {},
            [](void* state, UnsignedInt previous, UnsignedInt current) {
                auto& self = *static_cast<State*>(state);
                return self.frameTimeStartFrame[current] -
                    self.frameTimeStartFrame[previous];
            }, _state.get());
        _state->frameTimeIndex = index++;
    }
    if(values & Value::CpuDuration) {
        arrayAppend(measurements, InPlaceInit,
            "CPU duration"_s, Units::Nanoseconds,
            [](void* state) {
                /* Using steady_clock even though it might not be as precise as
                   high_resolution_clock in order to avoid the delta being
                   negative between frames, causing an underflow and an assert
                   similar to what used to happen for PrimitiveClipRatio */
                static_cast<State*>(state)->cpuDurationStartFrame = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            },
            [](void* state) {
                /* libc++ 10 needs an explicit cast to UnsignedLong */
                return UnsignedLong(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - static_cast<State*>(state)->cpuDurationStartFrame);
            }, _state.get());
        _state->cpuDurationIndex = index++;
    }
    if(values & Value::GpuDuration) {
        for(GL::TimeQuery& q: _state->timeQueries)
            q = GL::TimeQuery{GL::TimeQuery::Target::TimeElapsed};
        arrayAppend(measurements, InPlaceInit,
            "GPU duration"_s, Units::Nanoseconds,
            UnsignedInt(_state->timeQueries.size()),
            [](void* state, UnsignedInt current) {
                static_cast<State*>(state)->timeQueries[current].begin();
            },
            [](void* state, UnsignedInt current) {
                static_cast<State*>(state)->timeQueries[current].end();
            },
            [](void* state, UnsignedInt previous, UnsignedInt) {
                return static_cast<State*>(state)->timeQueries[previous].result<UnsignedLong>();
            }, _state.get());
        _state->gpuDurationIndex = index++;
    }
    #ifndef MAGNUM_TARGET_GLES
    if(values & Value::VertexFetchRatio) {
        for(GL::PipelineStatisticsQuery& q: _state->verticesSubmittedQueries)
            q = GL::PipelineStatisticsQuery{GL::PipelineStatisticsQuery::Target::VerticesSubmitted};
        for(GL::PipelineStatisticsQuery& q: _state->vertexShaderInvocationsQueries)
            q = GL::PipelineStatisticsQuery{GL::PipelineStatisticsQuery::Target::VertexShaderInvocations};
        arrayAppend(measurements, InPlaceInit,
            "Vertex fetch ratio"_s, Units::RatioThousandths,
            UnsignedInt(_state->verticesSubmittedQueries.size()),
            [](void* state, UnsignedInt current) {
                static_cast<State*>(state)->verticesSubmittedQueries[current].begin();
                static_cast<State*>(state)->vertexShaderInvocationsQueries[current].begin();
            },
            [](void* state, UnsignedInt current) {
                static_cast<State*>(state)->verticesSubmittedQueries[current].end();
                static_cast<State*>(state)->vertexShaderInvocationsQueries[current].end();
            },
            [](void* state, UnsignedInt previous, UnsignedInt) {
                /* Avoid division by zero if a frame doesn't have any draws */
                const auto submitted = static_cast<State*>(state)->verticesSubmittedQueries[previous].result<UnsignedLong>();
                if(!submitted) return UnsignedLong{};

                return static_cast<State*>(state)->vertexShaderInvocationsQueries[previous].result<UnsignedLong>()*1000/submitted;
            }, _state.get());
        _state->vertexFetchRatioIndex = index++;
    }
    if(values & Value::PrimitiveClipRatio) {
        for(GL::PipelineStatisticsQuery& q: _state->clippingInputPrimitivesQueries)
            q = GL::PipelineStatisticsQuery{GL::PipelineStatisticsQuery::Target::ClippingInputPrimitives};
        for(GL::PipelineStatisticsQuery& q: _state->clippingOutputPrimitivesQueries)
            q = GL::PipelineStatisticsQuery{GL::PipelineStatisticsQuery::Target::ClippingOutputPrimitives};
        arrayAppend(measurements, InPlaceInit,
            "Primitives clipped"_s, Units::PercentageThousandths,
            UnsignedInt(_state->clippingInputPrimitivesQueries.size()),
            [](void* state, UnsignedInt current) {
                static_cast<State*>(state)->clippingInputPrimitivesQueries[current].begin();
                static_cast<State*>(state)->clippingOutputPrimitivesQueries[current].begin();
            },
            [](void* state, UnsignedInt current) {
                static_cast<State*>(state)->clippingInputPrimitivesQueries[current].end();
                static_cast<State*>(state)->clippingOutputPrimitivesQueries[current].end();
            },
            [](void* state, UnsignedInt previous, UnsignedInt) {
                /* Avoid division by zero if a frame doesn't have any draws */
                const auto input = static_cast<State*>(state)->clippingInputPrimitivesQueries[previous].result<UnsignedLong>();
                if(!input) return UnsignedLong{};

                /* If we have more output primitives than input, it's because
                   a triangle got split into multiple. To avoid an underflow,
                   return zero as well. A corresponding test case is in
                   FrameProfilerGLTest::primitiveClipRatioNegative(). */
                const auto output = static_cast<State*>(state)->clippingOutputPrimitivesQueries[previous].result<UnsignedLong>();
                if(input < output) return UnsignedLong{};

                return 100000 - static_cast<State*>(state)->clippingOutputPrimitivesQueries[previous].result<UnsignedLong>()*100000/input;
            }, _state.get());
        _state->primitiveClipRatioIndex = index++;
    }
    #endif
    setup(Utility::move(measurements), maxFrameCount);
}

auto FrameProfilerGL::values() const -> Values {
    Values values;
    if(_state->frameTimeIndex != 0xffff) values |= Value::FrameTime;
    if(_state->cpuDurationIndex != 0xffff) values |= Value::CpuDuration;
    if(_state->gpuDurationIndex != 0xffff) values |= Value::GpuDuration;
    #ifndef MAGNUM_TARGET_GLES
    if(_state->vertexFetchRatioIndex != 0xffff) values |= Value::VertexFetchRatio;
    if(_state->primitiveClipRatioIndex != 0xffff) values |= Value::PrimitiveClipRatio;
    #endif
    return values;
}

bool FrameProfilerGL::isMeasurementAvailable(const Value value) const {
    const UnsignedShort* index = nullptr;
    switch(value) {
        case Value::FrameTime: index = &_state->frameTimeIndex; break;
        case Value::CpuDuration: index = &_state->cpuDurationIndex; break;
        case Value::GpuDuration: index = &_state->gpuDurationIndex; break;
        #ifndef MAGNUM_TARGET_GLES
        case Value::VertexFetchRatio: index = &_state->vertexFetchRatioIndex; break;
        case Value::PrimitiveClipRatio: index = &_state->primitiveClipRatioIndex; break;
        #endif
    }
    CORRADE_INTERNAL_ASSERT(index);
    CORRADE_ASSERT(*index < measurementCount(),
        "DebugTools::FrameProfilerGL::isMeasurementAvailable():" << value << "not enabled", {});
    return isMeasurementAvailable(*index);
}

Double FrameProfilerGL::frameTimeMean() const {
    CORRADE_ASSERT(_state->frameTimeIndex < measurementCount(),
        "DebugTools::FrameProfilerGL::frameTimeMean(): not enabled", {});
    return measurementMean(_state->frameTimeIndex);
}

Double FrameProfilerGL::cpuDurationMean() const {
    CORRADE_ASSERT(_state->cpuDurationIndex < measurementCount(),
        "DebugTools::FrameProfilerGL::cpuDurationMean(): not enabled", {});
    return measurementMean(_state->cpuDurationIndex);
}

Double FrameProfilerGL::gpuDurationMean() const {
    CORRADE_ASSERT(_state->gpuDurationIndex < measurementCount(),
        "DebugTools::FrameProfilerGL::gpuDurationMean(): not enabled", {});
    return measurementMean(_state->gpuDurationIndex);
}

#ifndef MAGNUM_TARGET_GLES
Double FrameProfilerGL::vertexFetchRatioMean() const {
    CORRADE_ASSERT(_state->vertexFetchRatioIndex < measurementCount(),
        "DebugTools::FrameProfilerGL::vertexFetchRatioMean(): not enabled", {});
    return measurementMean(_state->vertexFetchRatioIndex);
}

Double FrameProfilerGL::primitiveClipRatioMean() const {
    CORRADE_ASSERT(_state->primitiveClipRatioIndex < measurementCount(),
        "DebugTools::FrameProfilerGL::primitiveClipRatioMean(): not enabled", {});
    return measurementMean(_state->primitiveClipRatioIndex);
}
#endif

namespace {

constexpr const char* FrameProfilerGLValueNames[] {
    "FrameTime",
    "CpuDuration",
    "GpuDuration",
    "VertexFetchRatio",
    "PrimitiveClipRatio"
};

}

Debug& operator<<(Debug& debug, const FrameProfilerGL::Value value) {
    debug << "DebugTools::FrameProfilerGL::Value" << Debug::nospace;

    const UnsignedInt bit = Math::log2(UnsignedShort(value));
    if(1 << bit == UnsignedShort(value))
        return debug << "::" << Debug::nospace << FrameProfilerGLValueNames[bit];

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedShort(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const FrameProfilerGL::Values value) {
    return Containers::enumSetDebugOutput(debug, value, "DebugTools::FrameProfilerGL::Values{}", {
        FrameProfilerGL::Value::FrameTime,
        FrameProfilerGL::Value::CpuDuration,
        FrameProfilerGL::Value::GpuDuration,
        #ifndef MAGNUM_TARGET_GLES
        FrameProfilerGL::Value::VertexFetchRatio,
        FrameProfilerGL::Value::PrimitiveClipRatio
        #endif
        });
}
#endif

}}

namespace Corrade { namespace Utility {

using namespace Containers::Literals;
using namespace Magnum;

#ifdef MAGNUM_TARGET_GL
Containers::StringView ConfigurationValue<DebugTools::FrameProfilerGL::Value>::toString(const DebugTools::FrameProfilerGL::Value value, ConfigurationValueFlags) {
    const UnsignedInt bit = Math::log2(UnsignedShort(value));
    if(1 << bit == UnsignedShort(value))
        return DebugTools::FrameProfilerGLValueNames[bit];
    return "";
}

DebugTools::FrameProfilerGL::Value ConfigurationValue<DebugTools::FrameProfilerGL::Value>::fromString(const Containers::StringView value, ConfigurationValueFlags) {
    for(std::size_t i = 0; i != Containers::arraySize(DebugTools::FrameProfilerGLValueNames); ++i)
        if(DebugTools::FrameProfilerGLValueNames[i] == value)
            return DebugTools::FrameProfilerGL::Value(1 << i);

    return DebugTools::FrameProfilerGL::Value{};
}

Containers::String ConfigurationValue<DebugTools::FrameProfilerGL::Values>::toString(const DebugTools::FrameProfilerGL::Values value, ConfigurationValueFlags) {
    Containers::String out;

    /* Create an array of strings where each is non-empty only if given bit is
       set and then join them together to have just one allocation */
    const char* names[Containers::arraySize(DebugTools::FrameProfilerGLValueNames)]{};
    for(std::size_t i = 0; i != Containers::arraySize(DebugTools::FrameProfilerGLValueNames); ++i) {
        if(value & DebugTools::FrameProfilerGL::Value(1 << i))
            names[i] = DebugTools::FrameProfilerGLValueNames[i];
    }

    return " "_s.joinWithoutEmptyParts(names);
}

DebugTools::FrameProfilerGL::Values ConfigurationValue<DebugTools::FrameProfilerGL::Values>::fromString(const Containers::StringView value, ConfigurationValueFlags) {
    const Containers::Array<Containers::StringView> bits = value.splitOnWhitespaceWithoutEmptyParts();

    DebugTools::FrameProfilerGL::Values values;
    for(const Containers::StringView bit: bits)
        for(std::size_t i = 0; i != Containers::arraySize(DebugTools::FrameProfilerGLValueNames); ++i)
            if(DebugTools::FrameProfilerGLValueNames[i] == bit)
                values |= DebugTools::FrameProfilerGL::Value(1 << i);

    return values;
}
#endif

}}
