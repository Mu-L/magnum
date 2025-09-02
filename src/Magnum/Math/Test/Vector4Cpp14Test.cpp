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

#include <Corrade/TestSuite/Tester.h>

#include "Magnum/Math/Vector4.h"

namespace Magnum { namespace Math { namespace Test { namespace {

struct Vector4Cpp14Test: TestSuite::Tester {
    explicit Vector4Cpp14Test();

    void accessConstexpr();
};

/* What's a typedef and not a using differs from the typedefs in root Magnum
   namespace, or is not present there at all */
using Magnum::Vector4;

Vector4Cpp14Test::Vector4Cpp14Test() {
    addTests({&Vector4Cpp14Test::accessConstexpr});
}

constexpr Vector4 populate() {
    Vector4 a;
    a.x() = 6.0f;
    a.y() = 3.0f;
    a.z() = -1.0f;
    a.w() = 2.0f;
    a.r() *= 0.5f;
    a.g() -= 1.0f;
    a.b() += 1.0f;
    a.a() /= 2.0f;
    return a;
}

void Vector4Cpp14Test::accessConstexpr() {
    constexpr Vector4 a = populate();
    CORRADE_COMPARE(a, (Vector4{3.0f, 2.0f, 0.0f, 1.0f}));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Math::Test::Vector4Cpp14Test)
