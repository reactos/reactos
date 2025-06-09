/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for floating point conversion
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <apitest.h>

/* Real floating-integer conversions: The fractional part is discarded (truncated towards zero).
 * If the resulting value can be represented by the target type, that value is used.
 * Otherwise, the behavior is undefined.
 */

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

    // float to unsigned long cast (positive values)
    ok_eq_ulong(cast_float_to_ulong(0.0f), 0ul);
    ok_eq_ulong(cast_float_to_ulong(1.0f), 1ul);
    ok_eq_ulong(cast_float_to_ulong(0.5f), 0ul);
    ok_eq_ulong(cast_float_to_ulong(0.999999f), 0ul);
    ok_eq_ulong(cast_float_to_ulong(2147483648.0f), 2147483648ul); // 0x80000000
    ok_eq_ulong(cast_float_to_ulong(4294967150.0f), 4294967040ul); // 0xFFFFFF00

    // float to unsigned long cast (negative values)
    ok_eq_ulong(cast_float_to_ulong(-0.0f), 0ul);
    ok_eq_ulong(cast_float_to_ulong(-0.5f), 0ul);

    // float to long long cast
    ok_eq_longlong(cast_float_to_longlong(0.0f), 0ll);
    ok_eq_longlong(cast_float_to_longlong(1.0f), 1ll);
    ok_eq_longlong(cast_float_to_longlong(-1.0f), -1ll);
    ok_eq_longlong(cast_float_to_longlong(0.5f), 0ll);
    ok_eq_longlong(cast_float_to_longlong(-0.5f), 0ll);
    ok_eq_longlong(cast_float_to_longlong(0.999999f), 0ll);
    ok_eq_longlong(cast_float_to_longlong(-0.999999f), 0ll);
    ok_eq_longlong(cast_float_to_longlong(9223371761976868863.9999f), 9223371487098961920ll);
    ok_eq_longlong(cast_float_to_longlong(-9223371761976868863.9999f), -9223371487098961920ll);

    // float to unsigned long long cast (positive values)
    ok_eq_ulonglong(cast_float_to_ulonglong(0.0f), 0ull);
    ok_eq_ulonglong(cast_float_to_ulonglong(1.0f), 1ull);
    ok_eq_ulonglong(cast_float_to_ulonglong(0.5f), 0ull);
    ok_eq_ulonglong(cast_float_to_ulonglong(0.999999f), 0ull);
    ok_eq_ulonglong(cast_float_to_ulonglong(9223371487098961920.0f), 9223371487098961920ull); // 0x7FFFFF8000000000
    ok_eq_ulonglong(cast_float_to_ulonglong(9223372036854775808.0f), 9223372036854775808ull); // 0x8000000000000000
    ok_eq_ulonglong(cast_float_to_ulonglong(18446743523953737727.9f), 18446742974197923840ull); // 0xFFFFFF0000000000

    // float to unsigned long long cast (negative values)
    ok_eq_ulonglong(cast_float_to_ulonglong(-0.0f), 0ull);
    ok_eq_ulonglong(cast_float_to_ulonglong(-0.5f), 0ull);
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

    // double to unsigned long cast (positive values)
    ok_eq_ulong(cast_double_to_ulong(0.0), 0ul);
    ok_eq_ulong(cast_double_to_ulong(1.0), 1ul);
    ok_eq_ulong(cast_double_to_ulong(0.5), 0ul);
    ok_eq_ulong(cast_double_to_ulong(0.999999999), 0ul);
    ok_eq_ulong(cast_double_to_ulong(2147483648.0), 2147483648ul); // 0x80000000
    ok_eq_ulong(cast_double_to_ulong(4294967295.0), 4294967295ul); // 0xFFFFFFFF

    // double to unsigned long cast (negative values)
    ok_eq_ulong(cast_double_to_ulong(-0.0), 0ul);
    ok_eq_ulong(cast_double_to_ulong(-0.5), 0ul);

    // double to long long cast
    ok_eq_longlong(cast_double_to_longlong(0.0), 0ll);
    ok_eq_longlong(cast_double_to_longlong(1.0), 1ll);
    ok_eq_longlong(cast_double_to_longlong(-1.0), -1ll);
    ok_eq_longlong(cast_double_to_longlong(0.5), 0ll);
    ok_eq_longlong(cast_double_to_longlong(-0.5), 0ll);
    ok_eq_longlong(cast_double_to_longlong(0.999999), 0ll);
    ok_eq_longlong(cast_double_to_longlong(-0.999999), 0ll);
    ok_eq_longlong(cast_double_to_longlong(9223372036854775295.9), 9223372036854774784ll);
    ok_eq_longlong(cast_double_to_longlong(-9223372036854775295.9), -9223372036854774784ll);

    // double to unsigned long long cast (positive values)
    ok_eq_ulonglong(cast_double_to_ulonglong(0.0), 0ull);
    ok_eq_ulonglong(cast_double_to_ulonglong(1.0), 1ull);
    ok_eq_ulonglong(cast_double_to_ulonglong(0.5), 0ull);
    ok_eq_ulonglong(cast_double_to_ulonglong(0.999999), 0ull);
    ok_eq_ulonglong(cast_double_to_ulonglong(9223372036854774784.0), 9223372036854774784ull); // 0x7FFFFFFFFFFFFC00
    ok_eq_ulonglong(cast_double_to_ulonglong(9223372036854775808.0), 9223372036854775808ull); // 0x8000000000000000
    ok_eq_ulonglong(cast_double_to_ulonglong(18446744073709550591.9), 18446744073709549568ull); // 0xFFFFFFFFFFFFF800

    // float to unsigned long long cast (negative values)
    ok_eq_ulonglong(cast_double_to_ulonglong(-0.0), 0ull);
    ok_eq_ulonglong(cast_double_to_ulonglong(-0.5), 0ull);
}

START_TEST(floatconv)
{
    Test_float();
    Test_double();
}
