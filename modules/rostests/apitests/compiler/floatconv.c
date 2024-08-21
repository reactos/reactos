/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for floating point conversion
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <apitest.h>

/* Note: There are 2 behaviors for float to unsigned integer conversion:
 * 1. The old behavior, which exists only on x86 and CL versions up to somewhere
 *    between 19.40.33811 and 19.41.33923:
 *     - If a negative float is cast to an unsigned integer, the result is ULONG_MAX
 *       for uint32 and ULLONG_MAX for uint64.
 *     - If a float is cast to an unsigned long and the value is larger than ULONG_MAX,
 *       the result is ULONG_MAX.
 * 2. The new behavior (all x64 and x86 versions after the ones mentioned above):
 *     - If a negative float is cast to an unsigned integer, the result is the same as
         first casting to the signed type, then casting to the unsigned type.
 *     - If a float is cast to an unsigned integer and the value is too large for the type,
 *       the result is 0 for uint32 and 0x8000000000000000 for uint64.
 * In the old version, the float to unsigned conversion was inlined and used _ftoul2 and
 * a comparison against 0x43e0000000000000 (9223372036854775808.0) to check for overflow.
 * In the new version, the float to unsigned conversion is done by a call to _ftoul2_legacy,
 * which checks for overflow by comparing against 0x5f800000 (18446744073709551616.0) and
 * then forwards the call to either _ftol2 or _ftoul2.
 */

#if defined(_M_IX86) && defined(__VS_PROJECT__) && (_MSC_FULL_VER < 194133923)
#define OLD_BEHAVIOR
#endif

#ifdef OLD_BEHAVIOR
#define ULONG_OVERFLOW ULONG_MAX
#define ULONGLONG_OVERFLOW ULLONG_MAX
#else
#define ULONG_OVERFLOW 0ul
#define ULONGLONG_OVERFLOW 0x8000000000000000ull
#endif

#ifdef __GNUC__
#define todo_gcc todo_ros
#else
#define todo_gcc
#endif

__declspec(noinline)
long cast_float_to_long(float f)
{
    return (long)f;
}

__declspec(noinline)
unsigned long cast_float_to_ulong(float f)
{
    return (unsigned long)f;
}

__declspec(noinline)
long cast_double_to_long(double d)
{
    return (long)d;
}

__declspec(noinline)
unsigned long cast_double_to_ulong(double d)
{
    return (unsigned long)d;
}

__declspec(noinline)
long long cast_float_to_longlong(float f)
{
    return (long long)f;
}

__declspec(noinline)
unsigned long long cast_float_to_ulonglong(float f)
{
    return (unsigned long long)f;
}

__declspec(noinline)
long long cast_double_to_longlong(double d)
{
    return (long long)d;
}

__declspec(noinline)
unsigned long long cast_double_to_ulonglong(double d)
{
    return (unsigned long long)d;
}

void Test_float(void)
{
    // float to long cast
    ok_eq_long(cast_float_to_long(0.0f), 0l);
    ok_eq_long(cast_float_to_long(1.0f), 1l);
    ok_eq_long(cast_float_to_long(-1.0f), -1l);
    ok_eq_long(cast_float_to_long(0.5f), 0l);
    ok_eq_long(cast_float_to_long(-0.5f), 0l);
    ok_eq_long(cast_float_to_long(0.999999f), 0l);
    ok_eq_long(cast_float_to_long(-0.999999f), 0l);
    ok_eq_long(cast_float_to_long(2147483500.0f), 2147483520l);
    ok_eq_long(cast_float_to_long(2147483583.999f), 2147483520l);
    ok_eq_long(cast_float_to_long(-2147483583.999f), -2147483520l);
    ok_eq_long(cast_float_to_long(2147483584.0f), LONG_MIN); // -2147483648
    ok_eq_long(cast_float_to_long(2147483648.0f), LONG_MIN); // -2147483648
    ok_eq_long(cast_float_to_long(-2147483648.0f), LONG_MIN); // -2147483648
    ok_eq_long(cast_float_to_long(10000000000.0f), LONG_MIN);
    ok_eq_long(cast_float_to_long(-10000000000.0f), LONG_MIN);

    // float to unsigned long cast (positive values)
    ok_eq_ulong(cast_float_to_ulong(0.0f), 0ul);
    ok_eq_ulong(cast_float_to_ulong(1.0f), 1ul);
    ok_eq_ulong(cast_float_to_ulong(0.5f), 0ul);
    ok_eq_ulong(cast_float_to_ulong(0.999999f), 0ul);
    ok_eq_ulong(cast_float_to_ulong(2147483648.0f), 2147483648ul); // 0x80000000
    ok_eq_ulong(cast_float_to_ulong(4294967150.0f), 4294967040ul); // 0xFFFFFF00
    ok_eq_ulong(cast_float_to_ulong(4294967294.0f), ULONG_OVERFLOW);

    // float to unsigned long cast (negative values)
    ok_eq_ulong(cast_float_to_ulong(-0.0f), 0ul);
    ok_eq_ulong(cast_float_to_ulong(-0.5f), 0ul);
    ok_eq_ulong(cast_float_to_ulong(-1.0f), ULONG_MAX);
#ifdef OLD_BEHAVIOR
    ok_eq_ulong(cast_float_to_ulong(-10.0f), ULONG_MAX);
    ok_eq_ulong(cast_float_to_ulong(-1147483648.0f), ULONG_MAX);
    ok_eq_ulong(cast_float_to_ulong(-2147483648.0f), ULONG_MAX);
#else
    ok_eq_ulong(cast_float_to_ulong(-10.0f), (unsigned long)-10);
    ok_eq_ulong(cast_float_to_ulong(-1147483648.0f), (unsigned long)-1147483648ll);
    ok_eq_ulong(cast_float_to_ulong(-2147483648.0f), (unsigned long)-2147483648ll);
#endif

    // float to long long cast
    ok_eq_longlong(cast_float_to_longlong(0.0f), 0ll);
    ok_eq_longlong(cast_float_to_longlong(1.0f), 1ll);
    ok_eq_longlong(cast_float_to_longlong(-1.0f), -1ll);
    ok_eq_longlong(cast_float_to_longlong(0.5f), 0ll);
    ok_eq_longlong(cast_float_to_longlong(-0.5f), 0ll);
    ok_eq_longlong(cast_float_to_longlong(0.999999f), 0ll);
    ok_eq_longlong(cast_float_to_longlong(-0.999999f), 0ll);
    ok_eq_longlong(cast_float_to_longlong(9223371761976868863.9999f), 9223371487098961920ll);
    ok_eq_longlong(cast_float_to_longlong(9223371761976868864.0f), LLONG_MIN);
    ok_eq_longlong(cast_float_to_longlong(-9223371761976868863.9999f), -9223371487098961920ll);
    ok_eq_longlong(cast_float_to_longlong(-9223371761976868864.0f), LLONG_MIN);
    ok_eq_longlong(cast_float_to_longlong(100000000000000000000.0f), LLONG_MIN);
    ok_eq_longlong(cast_float_to_longlong(-100000000000000000000.0f), LLONG_MIN);

    // float to unsigned long long cast (positive values)
    ok_eq_ulonglong(cast_float_to_ulonglong(0.0f), 0ull);
    ok_eq_ulonglong(cast_float_to_ulonglong(1.0f), 1ull);
    ok_eq_ulonglong(cast_float_to_ulonglong(0.5f), 0ull);
    ok_eq_ulonglong(cast_float_to_ulonglong(0.999999f), 0ull);
    ok_eq_ulonglong(cast_float_to_ulonglong(9223371487098961920.0f), 9223371487098961920ull); // 0x7FFFFF8000000000
    ok_eq_ulonglong(cast_float_to_ulonglong(9223372036854775808.0f), 9223372036854775808ull); // 0x8000000000000000
    ok_eq_ulonglong(cast_float_to_ulonglong(18446743523953737727.9f), 18446742974197923840ull); // 0xFFFFFF0000000000
    todo_gcc ok_eq_ulonglong(cast_float_to_ulonglong(18446743523953737728.0f), ULONGLONG_OVERFLOW); // 0x8000000000000000 / 0xFFFFFFFFFFFFFFFF
    todo_gcc ok_eq_ulonglong(cast_float_to_ulonglong(20000000000000000000.0f), ULONGLONG_OVERFLOW); // 0x8000000000000000 / 0xFFFFFFFFFFFFFFFF

    // float to unsigned long long cast (negative values)
    ok_eq_ulonglong(cast_float_to_ulonglong(-0.0f), 0ull);
    ok_eq_ulonglong(cast_float_to_ulonglong(-0.5f), 0ull);
    ok_eq_ulonglong(cast_float_to_ulonglong(-1.0f), 18446744073709551615ull);
#ifdef OLD_BEHAVIOR
    ok_eq_ulonglong(cast_float_to_ulonglong(-10.0f), ULLONG_MAX);
    ok_eq_ulonglong(cast_float_to_ulonglong(-1147483648.0f), ULLONG_MAX);
    ok_eq_ulonglong(cast_float_to_ulonglong(-2147483648.0f), ULLONG_MAX);
    ok_eq_ulonglong(cast_float_to_ulonglong(-9223371761976868863.9f), ULLONG_MAX);
    ok_eq_ulonglong(cast_float_to_ulonglong(-9223371761976868864.0f), ULLONG_MAX);
    ok_eq_ulonglong(cast_float_to_ulonglong(-9223372036854775808.0f), ULLONG_MAX);
#else
    ok_eq_ulonglong(cast_float_to_ulonglong(-10.0f), (unsigned long long)-10);
    ok_eq_ulonglong(cast_float_to_ulonglong(-1147483648.0f), (unsigned long long)-1147483648ll);
    ok_eq_ulonglong(cast_float_to_ulonglong(-2147483648.0f), (unsigned long long)-2147483648ll);
    ok_eq_ulonglong(cast_float_to_ulonglong(-9223371761976868863.9f), (unsigned long long)-9223371487098961920);
    ok_eq_ulonglong(cast_float_to_ulonglong(-9223371761976868864.0f), (unsigned long long)(-9223372036854775807ll - 1)); // 0x8000000000000000 / ULONGLONG_OVERFLOW
    ok_eq_ulonglong(cast_float_to_ulonglong(-9223372036854775808.0f), (unsigned long long)(-9223372036854775807ll - 1)); // 0x8000000000000000 / ULONGLONG_OVERFLOW
#endif
    ok_eq_ulonglong(cast_float_to_ulonglong(-100000000000000000000.0f), ULONGLONG_OVERFLOW);
}

void Test_double(void)
{
    // double to long cast
    ok_eq_long(cast_double_to_long(0.0), 0l);
    ok_eq_long(cast_double_to_long(1.0), 1l);
    ok_eq_long(cast_double_to_long(-1.0), -1l);
    ok_eq_long(cast_double_to_long(0.5), 0l);
    ok_eq_long(cast_double_to_long(-0.5), 0l);
    ok_eq_long(cast_double_to_long(0.999999999), 0l);
    ok_eq_long(cast_double_to_long(-0.999999999), 0l);
    ok_eq_long(cast_double_to_long(2147483647.99999), 2147483647l);
    ok_eq_long(cast_double_to_long(-2147483647.99999), -2147483647l);
    ok_eq_long(cast_double_to_long(2147483648.0), LONG_MIN); // -2147483648
    ok_eq_long(cast_double_to_long(-2147483648.0), LONG_MIN); // -2147483648
    ok_eq_long(cast_double_to_long(10000000000.0), LONG_MIN);
    ok_eq_long(cast_double_to_long(-10000000000.0), LONG_MIN);

    // double to unsigned long cast (positive values)
    ok_eq_ulong(cast_double_to_ulong(0.0), 0ul);
    ok_eq_ulong(cast_double_to_ulong(1.0), 1ul);
    ok_eq_ulong(cast_double_to_ulong(0.5), 0ul);
    ok_eq_ulong(cast_double_to_ulong(0.999999999), 0ul);
    ok_eq_ulong(cast_double_to_ulong(2147483648.0), 2147483648ul); // 0x80000000
    ok_eq_ulong(cast_double_to_ulong(4294967295.0), 4294967295ul); // 0xFFFFFFFF
    ok_eq_ulong(cast_double_to_ulong(4294967296.0), ULONG_OVERFLOW);

    // double to unsigned long cast (negative values)
    ok_eq_ulong(cast_double_to_ulong(-0.0), 0ul);
    ok_eq_ulong(cast_double_to_ulong(-0.5), 0ul);
    ok_eq_ulong(cast_double_to_ulong(-1.0), ULONG_MAX);
#ifdef OLD_BEHAVIOR
    ok_eq_ulong(cast_double_to_ulong(-10.0), ULONG_MAX);
    ok_eq_ulong(cast_double_to_ulong(-1147483648.0), ULONG_MAX);
    ok_eq_ulong(cast_double_to_ulong(-2147483648.0), ULONG_MAX);
#else
    ok_eq_ulong(cast_double_to_ulong(-10.0), (unsigned long)-10);
    ok_eq_ulong(cast_double_to_ulong(-1147483648.0), (unsigned long)-1147483648ll);
    ok_eq_ulong(cast_double_to_ulong(-2147483648.0), (unsigned long)-2147483648ll);
#endif

    // double to long long cast
    ok_eq_longlong(cast_double_to_longlong(0.0), 0ll);
    ok_eq_longlong(cast_double_to_longlong(1.0), 1ll);
    ok_eq_longlong(cast_double_to_longlong(-1.0), -1ll);
    ok_eq_longlong(cast_double_to_longlong(0.5), 0ll);
    ok_eq_longlong(cast_double_to_longlong(-0.5), 0ll);
    ok_eq_longlong(cast_double_to_longlong(0.999999), 0ll);
    ok_eq_longlong(cast_double_to_longlong(-0.999999), 0ll);
    ok_eq_longlong(cast_double_to_longlong(9223372036854775295.9), 9223372036854774784ll);
    ok_eq_longlong(cast_double_to_longlong(9223372036854775296.0), LLONG_MIN);
    ok_eq_longlong(cast_double_to_longlong(-9223372036854775295.9), -9223372036854774784ll);
    ok_eq_longlong(cast_double_to_longlong(-9223372036854775296.0), LLONG_MIN);
    ok_eq_longlong(cast_double_to_longlong(100000000000000000000.0), LLONG_MIN);
    ok_eq_longlong(cast_double_to_longlong(-100000000000000000000.0), LLONG_MIN);

    // double to unsigned long long cast (positive values)
    ok_eq_ulonglong(cast_double_to_ulonglong(0.0), 0ull);
    ok_eq_ulonglong(cast_double_to_ulonglong(1.0), 1ull);
    ok_eq_ulonglong(cast_double_to_ulonglong(0.5), 0ull);
    ok_eq_ulonglong(cast_double_to_ulonglong(0.999999), 0ull);
    ok_eq_ulonglong(cast_double_to_ulonglong(9223372036854774784.0), 9223372036854774784ull); // 0x7FFFFFFFFFFFFC00
    ok_eq_ulonglong(cast_double_to_ulonglong(9223372036854775808.0), 9223372036854775808ull); // 0x8000000000000000
    ok_eq_ulonglong(cast_double_to_ulonglong(18446744073709550591.9), 18446744073709549568ull); // 0xFFFFFFFFFFFFF800
    todo_gcc ok_eq_ulonglong(cast_double_to_ulonglong(18446744073709550592.0), ULONGLONG_OVERFLOW); // 0x8000000000000000 / 0xFFFFFFFFFFFFFFFF
    todo_gcc ok_eq_ulonglong(cast_double_to_ulonglong(18446744073709551616.0), ULONGLONG_OVERFLOW); // 0x8000000000000000 / 0xFFFFFFFFFFFFFFFF
    todo_gcc ok_eq_ulonglong(cast_double_to_ulonglong(20000000000000000000.0), ULONGLONG_OVERFLOW); // 0x8000000000000000 / 0xFFFFFFFFFFFFFFFF

    // float to unsigned long long cast (negative values)
    ok_eq_ulonglong(cast_double_to_ulonglong(-0.0), 0ull);
    ok_eq_ulonglong(cast_double_to_ulonglong(-0.5), 0ull);
    ok_eq_ulonglong(cast_double_to_ulonglong(-1.0), 18446744073709551615ull);
#ifdef OLD_BEHAVIOR
    ok_eq_ulonglong(cast_double_to_ulonglong(-10.0), ULLONG_MAX);
    ok_eq_ulonglong(cast_double_to_ulonglong(-1147483648.0), ULLONG_MAX);
    ok_eq_ulonglong(cast_double_to_ulonglong(-2147483648.0), ULLONG_MAX);
    ok_eq_ulonglong(cast_double_to_ulonglong(-9223371761976868863.9), ULLONG_MAX);
    ok_eq_ulonglong(cast_double_to_ulonglong(-9223371761976868864.0), ULLONG_MAX);
    ok_eq_ulonglong(cast_double_to_ulonglong(-9223372036854775808.0), ULLONG_MAX);
#else
    ok_eq_ulonglong(cast_double_to_ulonglong(-10.0), (unsigned long long)-10);
    ok_eq_ulonglong(cast_double_to_ulonglong(-1147483648.0), (unsigned long long)-1147483648ll);
    ok_eq_ulonglong(cast_double_to_ulonglong(-2147483648.0), (unsigned long long)-2147483648ll);
    ok_eq_ulonglong(cast_double_to_ulonglong(-9223372036854775000.0), (unsigned long long)-9223372036854774784ll);
    ok_eq_ulonglong(cast_double_to_ulonglong(-9223372036854775808.0), (unsigned long long)(-9223372036854775807ll - 1));
#endif
    ok_eq_ulonglong(cast_double_to_ulonglong(-100000000000000000000.0), ULONGLONG_OVERFLOW);
}

START_TEST(floatconv)
{
    Test_float();
    Test_double();
}
