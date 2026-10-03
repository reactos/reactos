/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for double <-> string conversion in ucrtbase. These reach the big-integer
 *              division in sdk/lib/ucrt (multiply_64_32), whose GCC inline assembly used to
 *              fault on i386 when the compiler picked a memory operand addressed through EDX.
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <pseh/pseh2.h>
#include <ndk/mmfuncs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <errno.h>

/*
 * Expected strings are the exact decimal expansions of the binary values (none is a tie, where
 * implementations may legitimately round differently).
 */
static const struct
{
    const char *Format;
    double Value;
    const char *Expected;
} PrintfCases[] =
{
    { "%.2f",   1234.567,                  "1234.57" },
    { "%.15g",  0.1,                       "0.1" },
    { "%.17g",  0.1,                       "0.10000000000000001" },
    { "%.20f",  1.0 / 3.0,                 "0.33333333333333331483" },
    { "%.0f",   1e22,                      "10000000000000000000000" },
    { "%.0f",   1e23,                      "99999999999999991611392" },
    { "%e",     1e300,                     "1.000000e+300" },
    { "%.25e",  1e-5,                      "1.0000000000000000818030539e-05" },
    { "%.30f",  123456789.12345679,        "123456789.123456791043281555175781250000" },
    { "%.17g",  1.7976931348623157e308,    "1.7976931348623157e+308" },
    { "%.17g",  4.9406564584124654e-324,   "4.9406564584124654e-324" },
    { "%.6f",   2.718281828459045,         "2.718282" },
    { "%.10f",  1e15 / 7.0,                "142857142857142.8437500000" },
};

static
void
TestPrintf(void)
{
    char Buffer[512];
    ULONG Index;

    for (Index = 0; Index < _countof(PrintfCases); Index++)
    {
        StartSeh()
            sprintf(Buffer, PrintfCases[Index].Format, PrintfCases[Index].Value);
            ok(strcmp(Buffer, PrintfCases[Index].Expected) == 0,
               "sprintf(\"%s\", %.17g) = \"%s\", expected \"%s\"\n",
               PrintfCases[Index].Format,
               PrintfCases[Index].Value,
               Buffer,
               PrintfCases[Index].Expected);
        EndSeh(STATUS_SUCCESS);
    }
}

static
void
TestEcvtFcvt(void)
{
    char Buffer[64];
    int DecimalPoint, Sign;
    errno_t ErrorCode;

    /* 1234.567 to 6 significant digits: "123457", decimal point after 4 digits */
    StartSeh()
        DecimalPoint = Sign = 0x55;
        ErrorCode = _ecvt_s(Buffer, sizeof(Buffer), 1234.567, 6, &DecimalPoint, &Sign);
        ok(ErrorCode == 0, "_ecvt_s returned %d\n", ErrorCode);
        ok(strcmp(Buffer, "123457") == 0, "_ecvt_s digits \"%s\"\n", Buffer);
        ok(DecimalPoint == 4 && Sign == 0, "_ecvt_s decpt %d sign %d\n", DecimalPoint, Sign);
    EndSeh(STATUS_SUCCESS);

    /* 1234.567 to 2 digits after the decimal point: "123457", decimal point after 4 digits */
    StartSeh()
        DecimalPoint = Sign = 0x55;
        ErrorCode = _fcvt_s(Buffer, sizeof(Buffer), 1234.567, 2, &DecimalPoint, &Sign);
        ok(ErrorCode == 0, "_fcvt_s returned %d\n", ErrorCode);
        ok(strcmp(Buffer, "123457") == 0, "_fcvt_s digits \"%s\"\n", Buffer);
        ok(DecimalPoint == 4 && Sign == 0, "_fcvt_s decpt %d sign %d\n", DecimalPoint, Sign);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        DecimalPoint = Sign = 0x55;
        ErrorCode = _ecvt_s(Buffer, sizeof(Buffer), -0.000123456, 3, &DecimalPoint, &Sign);
        ok(ErrorCode == 0, "_ecvt_s returned %d\n", ErrorCode);
        ok(strcmp(Buffer, "123") == 0, "_ecvt_s digits \"%s\"\n", Buffer);
        ok(DecimalPoint == -3 && Sign == 1, "_ecvt_s decpt %d sign %d\n", DecimalPoint, Sign);
    EndSeh(STATUS_SUCCESS);
}

static
void
TestStrtod(void)
{
    char *End;
    double Value;

    StartSeh()
        Value = strtod("123456789012345678901234567890", &End);
        ok(Value == 1.2345678901234568e+29, "strtod = %.17g\n", Value);
        ok(*End == '\0', "strtod stopped at \"%s\"\n", End);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Value = strtod("1.7976931348623157e308", &End);
        ok(Value == DBL_MAX, "strtod(DBL_MAX) = %.17g\n", Value);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Value = strtod("4.9406564584124654e-324", &End);
        ok(Value == 4.9406564584124654e-324 && Value != 0.0,
           "strtod(denormal min) = %.17g\n", Value);
    EndSeh(STATUS_SUCCESS);

    /* 1e23 is not exactly representable, and its neighbours are what makes it a hard case */
    StartSeh()
        Value = strtod("1e23", &End);
        ok(Value == 1e23, "strtod(1e23) = %.17g\n", Value);
        Value = strtod("1e22", &End);
        ok(Value == 1e22, "strtod(1e22) = %.17g\n", Value);
    EndSeh(STATUS_SUCCESS);
}

START_TEST(float_conversion)
{
    TestPrintf();
    TestEcvtFcvt();
    TestStrtod();
}
