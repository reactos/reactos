/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for __dtoi64/dtou64/stoi64/stou64 on ARM
 * COPYRIGHT:       Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

#include <apitest.h>

typedef struct _DTOI64_TEST_DATA
{
    union
    {
        double value;
        unsigned long long raw;
    } given;
    long long expected;
} DTOI64_TEST_DATA;

typedef struct _DTOU64_TEST_DATA
{
    union
    {
        double value;
        unsigned long long raw;
    } given;
    unsigned long long expected;
} DTOU64_TEST_DATA;

typedef struct _STOI64_TEST_DATA
{
    union
    {
        float value;
        unsigned int raw;
    } given;
    long long expected;
} STOI64_TEST_DATA;

typedef struct _STOU64_TEST_DATA
{
    union
    {
        float value;
        unsigned int raw;
    } given;
    unsigned long long expected;
} STOU64_TEST_DATA;

START_TEST(__fto64)
{
    DTOI64_TEST_DATA dtoi64[] =
    {
        { 1383034209.0, 1383034209LL }, /* test 32bit number */
        { -1383034209.0, -1383034209LL }, /* test negative 32bit number */
        { 1383034209.1383034209, 1383034209LL }, /* test rounding 32bit */
        { -1383034209.1383034209, -1383034209LL }, /* test negative rounding 32bit */
        { 1383034209.83034209, 1383034209LL }, /* test rounding up 32bit */
        { -1383034209.83034209, -1383034209LL }, /* test negative rounding up 32bit */
        { 354056757614.0, 354056757614LL }, /* test 64bit int */
        { -354056757614.0, -354056757614LL }, /* test negative 64bit int */
        { 354056757614.83034209, 354056757614LL }, /* test 64bit rounding */
        { 18445937028656326656.0, 0x8000000000000000LL }, /* test unsigned 64bit */
        { 1.0000001, 1LL },
        { 0.0000001, 0LL },
        { -0.0000001, 0LL },

        /* special values tests */

        { -0.0, 0LL }, /* test -0 */
        { +0.0, 0LL }, /* test +0 */
        { .given.raw = 0x7FF0000000000000ULL, 0x8000000000000000LL }, /* test +INFINITY */
        { .given.raw = 0xFFF0000000000000ULL, 0x8000000000000000LL }, /* test -INFINITY */
        { .given.raw = 0x7FF0000000000001ULL, 0x8000000000000000LL }, /* test NaN1 */
        { .given.raw = 0x7FF8000000000001ULL, 0x8000000000000000LL }, /* test NaN2 */
        { .given.raw = 0x7FFFFFFFFFFFFFFFULL, 0x8000000000000000LL }, /* test NaN3 */
        { .given.raw = 0x7FF80000000000F1ULL, 0x8000000000000000LL }, /* test NaN4 */
    };
    DTOU64_TEST_DATA dtou64[] =
    {
        { 1383034209.0, 1383034209ULL }, /* test 32bit number */
        { -1383034209.0, 18446744072326517407ULL }, /* test negative 32bit number */
        { 1383034209.1383034209, 1383034209ULL }, /* test rounding 32bit */
        { -1383034209.1383034209, 18446744072326517407ULL }, /* test negative rounding 32bit */
        { 1383034209.83034209, 1383034209ULL }, /* test rounding up 32bit */
        { -1383034209.83034209, 18446744072326517407ULL }, /* test negative rounding up 32bit */
        { 354056757614.0, 354056757614ULL }, /* test 64bit int */
        { -354056757614.0, 18446743719652794002ULL }, /* test negative 64bit int */
        { 354056757614.83034209, 354056757614ULL }, /* test 64bit rounding */
        { 18445937028656326656.0, 18445937028656326656ULL }, /* test unsigned 64bit */
        { 1.0000001, 1ULL },
        { 0.0000001, 0ULL },
        { -0.0000001, 0ULL },

        /* special values tests */

        { -0.0, 0ULL }, /* test -0 */
        { +0.0, 0ULL }, /* test +0 */
        { .given.raw = 0x7FF0000000000000ULL, 0x8000000000000000LL }, /* test +INFINITY */
        { .given.raw = 0xFFF0000000000000ULL, 0x8000000000000000LL }, /* test -INFINITY */
        { .given.raw = 0x7FF0000000000001ULL, 0x8000000000000000LL }, /* test NaN1 */
        { .given.raw = 0x7FF8000000000001ULL, 0x8000000000000000LL }, /* test NaN2 */
        { .given.raw = 0x7FFFFFFFFFFFFFFFULL, 0x8000000000000000LL }, /* test NaN3 */
        { .given.raw = 0x7FF80000000000F1ULL, 0x8000000000000000LL }, /* test NaN4 */
    };
    STOI64_TEST_DATA stoi64[] =
    {
        { 1383034.0f, 1383034LL }, /* test 32bit number */
        { -1383034.0f, -1383034LL }, /* test negative 32bit number */
        { 1383034.1383034209f, 1383034LL }, /* test rounding 32bit */
        { -1383034.1383034209f, -1383034LL }, /* test negative rounding 32bit */
        { 1383034.83034209f, 1383034LL }, /* test rounding up 32bit */
        { -1383034.83034209f, -1383034LL }, /* test negative rounding up 32bit */
        { 354056765440.0f, 354056765440LL }, /* test 64bit int */
        { -354056765440.0f, -354056765440LL }, /* test negative 64bit int */
        { 3000000.75f, 3000000LL }, /* test 64bit rounding */
        { 18445937032174764032.0f, 0x8000000000000000LL }, /* test unsigned 64bit */
        { 1.0000001f, 1LL },
        { 0.0000001f, 0LL },
        { -0.0000001f, 0LL },

        /* special values tests */

        { -0.0f, 0LL }, /* test -0 */
        { +0.0f, 0LL }, /* test +0 */
        { .given.raw = 0x7F800000U, 0x8000000000000000LL }, /* test +INFINITY */
        { .given.raw = 0xFF800000U, 0x8000000000000000LL }, /* test -INFINITY */
        { .given.raw = 0x7F800001U, 0x8000000000000000LL }, /* test NaN1 */
        { .given.raw = 0x7FC00001U, 0x8000000000000000LL }, /* test NaN2 */
        { .given.raw = 0x7F8FFFFFU, 0x8000000000000000LL }, /* test NaN3 */
        { .given.raw = 0x7F8000F1U, 0x8000000000000000LL }, /* test NaN4 */
    };
    STOU64_TEST_DATA stou64[] =
    {
        { 1383034.0f, 1383034ULL }, /* test 32bit number */
        { -1383034.0f, 18446744073708168582ULL }, /* test negative 32bit number */
        { 1383034.1383034209f, 1383034ULL }, /* test rounding 32bit */
        { -1383034.1383034209f, 18446744073708168582ULL }, /* test negative rounding 32bit */
        { 1383034.83034209f, 1383034ULL }, /* test rounding up 32bit */
        { -1383034.83034209f, 18446744073708168582ULL }, /* test negative rounding up 32bit */
        { 354056765440.0f, 354056765440ULL }, /* test 64bit int */
        { -354056765440.0f, 18446743719652786176ULL }, /* test negative 64bit int */
        { 3000000.75f, 3000000ULL }, /* test 64bit rounding */
        { 18445937032174764032.0f, 18445937032174764032ULL }, /* test unsigned 64bit */
        { 1.0000001f, 1ULL },
        { 0.0000001f, 0ULL },
        { -0.0000001f, 0ULL },

        /* special values tests */

        { -0.0f, 0LL }, /* test -0 */
        { +0.0f, 0LL }, /* test +0 */
        {.given.raw = 0x7F800000U, 0x8000000000000000LL }, /* test +INFINITY */
        {.given.raw = 0xFF800000U, 0x8000000000000000LL }, /* test -INFINITY */
        {.given.raw = 0x7F800001U, 0x8000000000000000LL }, /* test NaN1 */
        {.given.raw = 0x7FC00001U, 0x8000000000000000LL }, /* test NaN2 */
        {.given.raw = 0x7F8FFFFFU, 0x8000000000000000LL }, /* test NaN3 */
        {.given.raw = 0x7F8000F1U, 0x8000000000000000LL }, /* test NaN4 */
    };

    unsigned int i;

    for (i = 0; i < _countof(dtoi64); ++i)
    {
        long long actual;

        actual = (long long)dtoi64[i].given.value;
        ok(actual == dtoi64[i].expected, "(dtoi64) %d: Expected %lld, but %lf -> %lld\n",
           i, dtoi64[i].expected, dtoi64[i].given.value, actual);
    }

    for (i = 0; i < _countof(dtou64); ++i)
    {
        unsigned long long actual;

        actual = (unsigned long long)dtou64[i].given.value;
        ok(actual == dtou64[i].expected, "(dtou64) %d: Expected %llu, but %lf -> %llu\n",
           i, dtou64[i].expected, dtou64[i].given.value, actual);
    }

    for (i = 0; i < _countof(stoi64); ++i)
    {
        long long actual;

        actual = (long long)stoi64[i].given.value;
        ok(actual == stoi64[i].expected, "(stoi64) %d: Expected %lld, but %f -> %lld\n",
           i, stoi64[i].expected, stoi64[i].given.value, actual);
    }

    for (i = 0; i < _countof(stou64); ++i)
    {
        unsigned long long actual;

        actual = (unsigned long long)stou64[i].given.value;
        ok(actual == stou64[i].expected, "(stou64) %d: Expected %llu, but %f -> %llu\n",
           i, stou64[i].expected, stou64[i].given.value, actual);
    }
}
