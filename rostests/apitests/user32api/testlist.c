#ifndef _USER32TESTLIST_H
#define _USER32TESTLIST_H

#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))

#include "user32api.h"

/* include the tests */
#include "tests/GetSystemMetrics.c"
#include "tests/InitializeLpkHooks.c"
#include "tests/ScrollDC.c"
#include "tests/ScrollWindowEx.c"
#include "tests/RealGetWindowClass.c"

/* The List of tests */
TESTENTRY TestList[] =
{
    { L"GetSystemMetrics", Test_GetSystemMetrics },
    { L"InitializeLpkHooks", Test_InitializeLpkHooks },
    { L"ScrollDC", Test_ScrollDC },
    { L"ScrollWindowEx", Test_ScrollWindowEx },
    { L"RealGetWindowClass", Test_RealGetWindowClass },
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
    return ARRAY_SIZE(TestList);
}

#endif /* _USER32TESTLIST_H */

/* EOF */
