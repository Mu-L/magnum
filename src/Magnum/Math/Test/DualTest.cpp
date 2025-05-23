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

#include <new>
#include <Corrade/Containers/ArrayView.h> /* arraySize() */
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>

#include "Magnum/Math/Dual.h"
#include "Magnum/Math/Quaternion.h"
#include "Magnum/Math/Vector2.h"
#include "Magnum/Math/StrictWeakOrdering.h"

namespace Magnum { namespace Math { namespace Test { namespace {

struct DualTest: TestSuite::Tester {
    explicit DualTest();

    void construct();
    void constructDefault();
    void constructZero();
    void constructNoInit();
    void constructConversion();
    void constructCopy();

    void data();

    void compare();

    void promotedNegated();
    void addSubtract();
    void multiplyDivide();
    void multiplyDivideScalar();
    void multiplyDivideDifferentType();

    void conjugated();
    void sqrt();

    void sincos();
    void sincosWithBase();

    void strictWeakOrdering();

    void subclassTypes();
    void subclass();

    void debug();
};

/* What's a typedef and not a using differs from the typedefs in root Magnum
   namespace, or is not present there at all */
typedef Math::Dual<Float> Dual;
using Magnum::Vector2;
typedef Math::Dual<Vector2> DualVector2;
using Magnum::Deg;
using Magnum::Rad;
using Magnum::Constants;

using namespace Literals;

DualTest::DualTest() {
    addTests({&DualTest::construct,
              &DualTest::constructDefault,
              &DualTest::constructZero,
              &DualTest::constructNoInit,
              &DualTest::constructConversion,
              &DualTest::constructCopy,

              &DualTest::data,

              &DualTest::compare,

              &DualTest::promotedNegated,
              &DualTest::addSubtract,
              &DualTest::multiplyDivide,
              &DualTest::multiplyDivideScalar,
              &DualTest::multiplyDivideDifferentType,

              &DualTest::conjugated,
              &DualTest::sqrt,

              &DualTest::sincos,
              &DualTest::sincosWithBase,

              &DualTest::strictWeakOrdering,

              &DualTest::subclassTypes,
              &DualTest::subclass,

              &DualTest::debug});
}

void DualTest::construct() {
    constexpr Dual a = {2.0f, -7.5f};
    CORRADE_COMPARE(a.real(), 2.0f);
    CORRADE_COMPARE(a.dual(), -7.5f);

    constexpr Dual d(3.0f);
    CORRADE_COMPARE(d.real(), 3.0f);
    CORRADE_COMPARE(d.dual(), 0.0f);

    CORRADE_VERIFY(std::is_nothrow_constructible<Dual, Float, Float>::value);
}

void DualTest::constructDefault() {
    constexpr Dual a;
    constexpr Math::Dual<Math::Quaternion<Float>> b;
    CORRADE_COMPARE(a, Dual(0.0f, 0.0f));
    CORRADE_COMPARE(b, Math::Dual<Math::Quaternion<Float>>({{0.0f, 0.0f, 0.0f}, 1.0f}, {{0.0f, 0.0f, 0.0f}, 1.0f}));

    CORRADE_VERIFY(std::is_nothrow_default_constructible<Dual>::value);
}

void DualTest::constructZero() {
    constexpr Dual a{ZeroInit};
    constexpr Math::Dual<Math::Quaternion<Float>> b{ZeroInit};
    CORRADE_COMPARE(a, Dual(0.0f, 0.0f));
    CORRADE_COMPARE(b, Math::Dual<Math::Quaternion<Float>>({{0.0f, 0.0f, 0.0f}, 0.0f}, {{0.0f, 0.0f, 0.0f}, 0.0f}));

    CORRADE_VERIFY(std::is_nothrow_constructible<Dual, ZeroInitT>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<Math::Dual<Math::Quaternion<Float>>, ZeroInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<ZeroInitT, Dual>::value);
    CORRADE_VERIFY(!std::is_convertible<ZeroInitT, Math::Dual<Math::Quaternion<Float>>>::value);
}

void DualTest::constructNoInit() {
    Dual a{2.0f, -7.5f};
    Math::Dual<Math::Quaternion<Float>> b{{{3.0f, 0.1f, 1.0f}, 1.0f}, {{0.1f, 0.0f, 1.0f}, 0.3f}};
    new(&a) Dual{Magnum::NoInit};
    new(&b) Math::Dual<Math::Quaternion<Float>>{Magnum::NoInit};
    {
        /* Explicitly check we're not on Clang because certain Clang-based IDEs
           inherit __GNUC__ if GCC is used instead of leaving it at 4 like
           Clang itself does */
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__*100 + __GNUC_MINOR__ >= 601 && __OPTIMIZE__
        CORRADE_EXPECT_FAIL("GCC 6.1+ misoptimizes and overwrites the value.");
        #endif
        CORRADE_COMPARE(a, Dual(2.0f, -7.5f));
        CORRADE_COMPARE(b, (Math::Dual<Math::Quaternion<Float>>{{{3.0f, 0.1f, 1.0f}, 1.0f}, {{0.1f, 0.0f, 1.0f}, 0.3f}}));
    }

    CORRADE_VERIFY(std::is_nothrow_constructible<Dual, Magnum::NoInitT>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<Math::Dual<Math::Quaternion<Float>>, Magnum::NoInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<Magnum::NoInitT, Dual>::value);
    CORRADE_VERIFY(!std::is_convertible<Magnum::NoInitT, Math::Dual<Math::Quaternion<Float>>>::value);
}

void DualTest::constructConversion() {
    typedef Math::Dual<Int> Duali;

    constexpr Dual a{1.3f, 2.7f};
    constexpr Duali b{a};

    CORRADE_COMPARE(b, (Duali{1, 2}));

    /* Implicit conversion is not allowed */
    CORRADE_VERIFY(!std::is_convertible<Dual, Duali>::value);

    CORRADE_VERIFY(std::is_nothrow_constructible<Dual, Duali>::value);
}

void DualTest::constructCopy() {
    constexpr Dual a(2.0f, 3.0f);
    constexpr Dual b(a);
    CORRADE_COMPARE(b, Dual(2.0f, 3.0f));

    #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    CORRADE_VERIFY(std::is_trivially_copy_constructible<Dual>::value);
    CORRADE_VERIFY(std::is_trivially_copy_assignable<Dual>::value);
    #endif
    CORRADE_VERIFY(std::is_nothrow_copy_constructible<Dual>::value);
    CORRADE_VERIFY(std::is_nothrow_copy_assignable<Dual>::value);
}

void DualTest::data() {
    constexpr Dual ca{1.5f, -3.5f};
    constexpr Float real = ca.real();
    constexpr Float dual = ca.dual();
    CORRADE_COMPARE(real, 1.5f);
    CORRADE_COMPARE(dual, -3.5f);

    Dual a{1.5f, -3.5f};
    a.real() = 2.0f;
    a.dual() = -3.5f;
    CORRADE_COMPARE(a, (Dual{2.0f, -3.5f}));

    /* Not constexpr anymore, as it has to reinterpret to return a
       correctly-sized array */
    CORRADE_COMPARE(*ca.data(), 1.5f);
    CORRADE_COMPARE(a.data()[1], -3.5f);

    /* It actually returns an array */
    CORRADE_COMPARE(Containers::arraySize(a.data()), 2);
    CORRADE_COMPARE(Containers::arraySize(ca.data()), 2);
}

void DualTest::compare() {
    CORRADE_VERIFY(Dual(1.0f, 1.0f+TypeTraits<Float>::epsilon()/2) == Dual(1.0f, 1.0f));
    CORRADE_VERIFY(Dual(1.0f, 1.0f+TypeTraits<Float>::epsilon()*2) != Dual(1.0f, 1.0f));
    CORRADE_VERIFY(Dual(1.0f+TypeTraits<Float>::epsilon()/2, 1.0f) == Dual(1.0f, 1.0f));
    CORRADE_VERIFY(Dual(1.0f+TypeTraits<Float>::epsilon()*2, 1.0f) != Dual(1.0f, 1.0f));

    /* Compare to real part only */
    CORRADE_VERIFY(Dual(1.0f, 0.0f) == 1.0f);
    CORRADE_VERIFY(Dual(1.0f, 3.0f) != 1.0f);
}

void DualTest::promotedNegated() {
    CORRADE_COMPARE(+Dual(1.0f, -6.5f), Dual(1.0f, -6.5f));
    CORRADE_COMPARE(-Dual(1.0f, -6.5f), Dual(-1.0f, 6.5f));
}

void DualTest::addSubtract() {
    Dual a(2.0f, -7.5f);
    Dual b(-3.3f, 0.2f);
    Dual c(-1.3f, -7.3f);

    CORRADE_COMPARE(a + b, c);
    CORRADE_COMPARE(c - b, a);
}

void DualTest::multiplyDivide() {
    Dual a(1.5f, -4.0f);
    Dual b(-2.0f, 0.5f);
    Dual c(-3.0f, 8.75f);

    CORRADE_COMPARE(a*b, c);
    CORRADE_COMPARE(c/b, a);
}

void DualTest::multiplyDivideScalar() {
    Dual a{1.5f, -4.0f};
    Dual b{-3.0f, 8.0f};

    CORRADE_COMPARE(a*-2.0f, b);
    CORRADE_COMPARE(-2.0f*a, b);
    CORRADE_COMPARE(b/-2.0f, a);
}

void DualTest::multiplyDivideDifferentType() {
    DualVector2 a{{1.5f, 2.0f}, {-4.0f, 1.3f}};
    Dual b{-2.0f, 0.5f};
    DualVector2 c{{-3.0f, -4.0f}, {8.75f, -1.6f}};
    DualVector2 d{{-2.0f/1.5f, -1.0f}, {-7.25f/2.25f, 3.6f/4.0f}};

    CORRADE_COMPARE(a*b, c);
    CORRADE_COMPARE(b*a, c);
    CORRADE_COMPARE(c/b, a);
    CORRADE_COMPARE(b/a, d);
}

void DualTest::conjugated() {
    CORRADE_COMPARE(Dual(1.0f, -6.5f).conjugated(), Dual(1.0f, 6.5f));
}

void DualTest::sqrt() {
    CORRADE_COMPARE(Math::sqrt(Dual(16.0f, 2.0f)), Dual(4.0f, 0.25f));
}

void DualTest::sincos() {
    Containers::Pair<Dual, Dual> result{
        {0.5f, 0.8660254037844386f*Constants::pi()/2},
        {0.8660254037844386f, -0.5f*Constants::pi()/2}};
    CORRADE_COMPARE(Math::sincos(Math::Dual<Deg>(30.0_degf, 90.0_degf)), result);
    CORRADE_COMPARE(Math::sincos(Math::Dual<Rad>(Rad(Constants::pi()/6), Rad(Constants::pi()/2))), result);
}

void DualTest::sincosWithBase() {
    /* Verify that the functions can be called with Dual<Unit<Deg, T>> and Dual<Unit<Rad, T>> */
    CORRADE_VERIFY(std::is_same<decltype(2*Math::Dual<Deg>{15.0_degf}), Math::Dual<Unit<Math::Deg, Float>>>::value);
    CORRADE_VERIFY(std::is_same<decltype(2*Math::Dual<Rad>{Rad{Constants::pi()/12}}), Math::Dual<Unit<Math::Rad, Float>>>::value);

    Containers::Pair<Dual, Dual> result{
        {0.5f, 0.8660254037844386f*Constants::pi()/2},
        {0.8660254037844386f, -0.5f*Constants::pi()/2}};
    CORRADE_COMPARE(Math::sincos(2*Math::Dual<Deg>(15.0_degf, 45.0_degf)), result);
    CORRADE_COMPARE(Math::sincos(2*Math::Dual<Rad>(Rad(Constants::pi()/12), Rad(Constants::pi()/4))), result);
}

void DualTest::strictWeakOrdering() {
    StrictWeakOrdering o;
    const Dual a{1.0f, 2.0f};
    const Dual b{2.0f, 3.0f};
    const Dual c{1.0f, 3.0f};

    CORRADE_VERIFY( o(a, b));
    CORRADE_VERIFY(!o(b, a));
    CORRADE_VERIFY( o(a, c));
    CORRADE_VERIFY(!o(c, a));
    CORRADE_VERIFY( o(c, b));
    CORRADE_VERIFY(!o(b, c));
    CORRADE_VERIFY(!o(a, a));
}

template<class T> class BasicDualVec2: public Math::Dual<Math::Vector2<T>> {
    public:
        template<class ...U> constexpr BasicDualVec2(U&&... args): Math::Dual<Math::Vector2<T>>{args...} {}

        _MAGNUM_DUAL_SUBCLASS_IMPLEMENTATION(BasicDualVec2, Math::Vector2, T)
        _MAGNUM_DUAL_SUBCLASS_MULTIPLICATION_IMPLEMENTATION(BasicDualVec2, Math::Vector2)
};

_MAGNUM_DUAL_OPERATOR_IMPLEMENTATION(BasicDualVec2, Math::Vector2, T)

typedef BasicDualVec2<Float> DualVec2;

void DualTest::subclassTypes() {
    const DualVec2 a;
    CORRADE_VERIFY(std::is_same<decltype(+a), DualVec2>::value);
    CORRADE_VERIFY(std::is_same<decltype(-a), DualVec2>::value);
    CORRADE_VERIFY(std::is_same<decltype(a + a), DualVec2>::value);
    CORRADE_VERIFY(std::is_same<decltype(a - a), DualVec2>::value);
    CORRADE_VERIFY(std::is_same<decltype(a*a), DualVec2>::value);
    CORRADE_VERIFY(std::is_same<decltype(a/a), DualVec2>::value);

    DualVec2 b;
    CORRADE_VERIFY(std::is_same<decltype(b += a), DualVec2&>::value);
    CORRADE_VERIFY(std::is_same<decltype(b -= a), DualVec2&>::value);

    const Dual c;
    CORRADE_VERIFY(std::is_same<decltype(a*c), DualVec2>::value);
    CORRADE_VERIFY(std::is_same<decltype(c*a), DualVec2>::value);
    CORRADE_VERIFY(std::is_same<decltype(a/c), DualVec2>::value);
    CORRADE_VERIFY(std::is_same<decltype(c/a), DualVec2>::value);

    CORRADE_VERIFY(std::is_same<decltype(a*5.0f), DualVec2>::value);
    CORRADE_VERIFY(std::is_same<decltype(5.0f*a), DualVec2>::value);
    CORRADE_VERIFY(std::is_same<decltype(a/5.0f), DualVec2>::value);
}

void DualTest::subclass() {
    const DualVec2 a{Vector2{1.5f,  2.0f}, Vector2{-4.0f,  1.3f}};
    const DualVec2 b{Vector2{3.0f, -1.2f}, Vector2{ 0.2f, -1.0f}};
    const DualVec2 c{Vector2{4.5f,  0.8f}, Vector2{-3.8f,  0.3f}};
    const DualVec2 d{Vector2{4.5f, -2.4f}, Vector2{-11.7f, -3.56f}};

    CORRADE_COMPARE(+a, (DualVec2{Vector2{1.5f, 2.0f}, Vector2{-4.0f, 1.3f}}));
    CORRADE_COMPARE(-a, (DualVec2{Vector2{-1.5f, -2.0f}, Vector2{4.0f, -1.3f}}));
    CORRADE_COMPARE(a + b, c);
    CORRADE_COMPARE(c - b, a);
    CORRADE_COMPARE(a*b, d);
    CORRADE_COMPARE(d/b, a);

    /* No need to test in-place operators as the other ones are implemented
       using them */

    const Dual e{-2.0f, 0.5f};
    const DualVec2 f{Vector2{-3.0f, -4.0f}, Vector2{8.75f, -1.6f}};
    const DualVec2 g{Vector2{-2.0f/1.5f, -1.0f}, Vector2{-7.25f/2.25f, 3.6f/4.0f}};
    CORRADE_COMPARE(a*e, f);
    CORRADE_COMPARE(e*a, f);
    CORRADE_COMPARE(f/e, a);
    CORRADE_COMPARE(e/a, g);

    const DualVec2 h{Vector2{-3.0f, -4.0f}, Vector2{8.0f, -2.6f}};
    CORRADE_COMPARE(a*-2.0f, h);
    CORRADE_COMPARE(-2.0f*a, h);
    CORRADE_COMPARE(h/-2.0f, a);
}

void DualTest::debug() {
    Containers::String out;

    Debug{&out} << Dual(2.5f, -0.3f);
    CORRADE_COMPARE(out, "Dual(2.5, -0.3)\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Math::Test::DualTest)
