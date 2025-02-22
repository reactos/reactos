/*
 * PROJECT:         ReactOS CRT regression tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            rostests/regtests/crt/iofuncs.c
 * PURPOSE:         Tests for input/output functions of the CRT
 * PROGRAMMERS:     Gregor Schneider
 */

#include <stdio.h>
#include <wine/test.h>

#define _CRT_NON_CONFORMING_SWPRINTFS

struct _testData
{
    double val;
    int prec;
    char* exp;
    char* exp2;
} ECVTTESTDATA[] =
{
    {   45.0,           2,      "+4.50E+001",           "+4.5000E+001"          },  /*0*/
    {   0.0001,         1,      "+1.0E-004",            "+1.000E-004"           },
    {   0.0001,         -10,    "+1.000000E-004",       "+1.000000E-004"        },
    {   0.0001,         10,     "+1.0000000000E-004",   "+1.000000000000E-004"  },
    {   -111.0001,      5,      "-1.11000E+002",        "-1.1100010E+002"       },
    {   111.0001,       5,      "+1.11000E+002",        "+1.1100010E+002"       },  /*5*/
    {   3333.3,         2,      "+3.33E+003",           "+3.3333E+003"          },
    {   999999999999.9, 3,      "+1.000E+012",          "+1.00000E+012"         },
    {   0.0,            5,      "+0.00000E+000",        "+0.0000000E+000"       },
    {   0.0,            0,      "+0E+000",              "+0.00E+000"            },
    {   0.0,            -1,     "+0.000000E+000",       "+0.0E+000"             },  /*10*/
    {   -123.0001,      0,      "-1E+002",              "-1.23E+002"            },
    {   -123.0001,      -1,     "-1.230001E+002",       "-1.2E+002"             },
    {   -123.0001,      -2,     "-1.230001E+002",       "-1E+002"               },
    {   -123.0001,      -3,     "-1.230001E+002",       "-1.230001E+002"        },
    {   99.99,          1,      "+1.0E+002",            "+9.999E+001"           },  /*15*/
    {   0.0063,         2,      "+6.30E-003",           "+6.3000E-003"          },
    {   0.0063,         3,      "+6.300E-003",          "+6.30000E-003"         },
    {   0.09999999996,  2,      "+1.00E-001",           "+1.0000E-001"          },
    {   0.6,            1,      "+6.0E-001",            "+6.000E-001"           },
    {   0.6,            0,      "+6E-001",              "+6.00E-001"            },  /*20*/
    {   0.4,            0,      "+4E-001",              "+4.00E-001"            },
    {   0.49,           0,      "+5E-001",              "+4.90E-001"            },
    {   0.51,           0,      "+5E-001",              "+5.10E-001"            }
};

void Test_ofuncs()
{
    int i;
    char* buf = NULL;

    /* Test exponential format */
    buf = malloc(30 * sizeof(char));
    if (buf == NULL)
    {
        printf("Memory full, exiting\n");
        return;
    }
    for (i = 0; i < sizeof(ECVTTESTDATA)/sizeof(ECVTTESTDATA[0]); i++)
    {
        sprintf(buf, "%-+.*E", ECVTTESTDATA[i].prec, ECVTTESTDATA[i].val);
        ok(!strcmp(buf, ECVTTESTDATA[i].exp),
            "sprintf exp test %d failed: got %s, expected %s\n",
            i, buf, ECVTTESTDATA[i].exp);
    }
    for (i = 0; i < sizeof(ECVTTESTDATA)/sizeof(ECVTTESTDATA[0]); i++)
    {
        sprintf(buf, "%-+.*E", ECVTTESTDATA[i].prec + 2, ECVTTESTDATA[i].val);
        ok(!strcmp(buf, ECVTTESTDATA[i].exp2),
            "sprintf exp +2 prec test %d failed: got %s, expected %s\n",
            i, buf, ECVTTESTDATA[i].exp2);
    }

    /* Test with negative number to be rounded */
    sprintf(buf, "%-+.*E", ECVTTESTDATA[18].prec + 2, -ECVTTESTDATA[18].val);
    ok(!strcmp(buf, "-1.0000E-001"), "Negative number sprintf rounding failed: got %s, expected %s\n",
        buf, "-1.0000E-001");
    free(buf);
}

void Test_ifuncs()
{
    double var;
    char cnum[] = "12.3";
    wchar_t wnum[] = L"12.3";

    /* Test sscanf behaviour */
    sscanf(cnum, "%lf", &var);
    ok(var == 12.3, "sscanf double conversion failed: got %f\n", var);
    swscanf(wnum, L"%lf", &var);
    ok(var == 12.3, "swscanf double conversion failed: got %f\n", var);
}

START_TEST(iofuncs)
{
    Test_ofuncs();
    Test_ifuncs();
}

