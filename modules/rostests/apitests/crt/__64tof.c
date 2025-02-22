/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for __i64tod/u64tod/i64tos/u64tos on ARM
 * COPYRIGHT:       Copyright 2021 Stanislav Motylkov <x86corez@gmail.com>
 */

#include <apitest.h>

typedef struct _I64TOD_TEST_DATA
{
    long long given;
    union
    {
        double value;
        unsigned long long raw;
    } expected;
} I64TOD_TEST_DATA;

typedef struct _U64TOD_TEST_DATA
{
    unsigned long long given;
    union
    {
        double value;
        unsigned long long raw;
    } expected;
} U64TOD_TEST_DATA;

typedef struct _I64TOS_TEST_DATA
{
    long long given;
    union
    {
        float value;
        unsigned int raw;
    } expected;
} I64TOS_TEST_DATA;

typedef struct _U64TOS_TEST_DATA
{
    unsigned long long given;
    union
    {
        float value;
        unsigned int raw;
    } expected;
} U64TOS_TEST_DATA;

START_TEST(__64tof)
{
    I64TOD_TEST_DATA i64tod[] =
    {
        { 1383034209LL, 1383034209.0 }, /* test 32bit number */
        { -1383034209LL, -1383034209.0 }, /* test negative 32bit number */
        { 354056757614LL, 354056757614.0 }, /* test 64bit int */
        { -354056757614LL, -354056757614.0 }, /* test negative 64bit int */
        { 18446744073709550000LL, -1616.0 }, /* test 20bit in float */
        { 0x8000000000000000LL, -9223372036854775800.0 }, /* test big 64bit int */
        { 0xFFFFFFFFFFFFFFFFLL, -1.0 }, /* test -1 */
        { 0LL, +0.0 }, /* test 0 */
    };
    U64TOD_TEST_DATA u64tod[] =
    {
        { 1383034209ULL, 1383034209.0 }, /* test 32bit number */
        { 354056757614ULL, 354056757614.0 }, /* test 64bit int */
        { 18445937028656326656ULL, 18445937028656326656.0 }, /* test unsigned 64bit */
        { 18446744073709550000ULL, 18446744073709550000.0 }, /* test 20bit in float */
        { 18446744073709551615ULL, 18446744073709552000.0 }, /* test big 64bit number */
        { 0ULL, +0.0 }, /* test 0 */
    };
    I64TOS_TEST_DATA i64tos[] =
    {
        { 1383034LL, 1383034.0f }, /* test 32bit number */
        { -1383034LL, -1383034.0f }, /* test negative 32bit number */
        { 354056765440LL, 354056765440.0f }, /* test 64bit int */
        { -354056765440LL, -354056765440.0f }, /* test negative 64bit int */
        { 18446744073709550000LL, -1616.0f }, /* test 20bit in float */
        { 0x8000000000000000LL, -9223372036854775800.0f }, /* test big 64bit int */
        { 0xFFFFFFFFFFFFFFFFLL, -1.0f }, /* test -1 */
        { 0LL, +0.0f }, /* test 0 */
    };
    U64TOS_TEST_DATA u64tos[] =
    {
        { 1383034ULL, 1383034.0f }, /* test 32bit number */
        { 354056765440ULL, 354056765440.0f }, /* test 64bit int */
        { 18445937032174764032ULL, 18445937032174764032.0f }, /* test unsigned 64bit */
        { 18446744073709550000ULL, 18446744073709550000.0f }, /* test 20bit in float */
        { 18446744073709551615ULL, 18446744073709552000.0f }, /* test big 64bit number */
        { 0ULL, +0.0f }, /* test 0 */
    };

    unsigned int i;

    for (i = 0; i < _countof(i64tod); ++i)
    {
        double actual;

        actual = (double)i64tod[i].given;
        ok(actual == i64tod[i].expected.value, "(i64tod) %d: Expected %lf, but %lld -> %lf\n",
           i, i64tod[i].expected.value, i64tod[i].given, actual);
    }

    for (i = 0; i < _countof(u64tod); ++i)
    {
        double actual;

        actual = (double)u64tod[i].given;
        ok(actual == u64tod[i].expected.value, "(u64tod) %d: Expected %lf, but %llu -> %lf\n",
           i, u64tod[i].expected.value, u64tod[i].given, actual);
    }

    for (i = 0; i < _countof(i64tos); ++i)
    {
        float actual;

        actual = (float)i64tos[i].given;
        ok(actual == i64tos[i].expected.value, "(i64tos) %d: Expected %f, but %lld -> %f\n",
           i, i64tos[i].expected.value, i64tos[i].given, actual);
    }

    for (i = 0; i < _countof(u64tos); ++i)
    {
        float actual;

        actual = (float)u64tos[i].given;
        ok(actual == u64tos[i].expected.value, "(u64tos) %d: Expected %f, but %llu -> %f\n",
           i, u64tos[i].expected.value, u64tos[i].given, actual);
    }
}
