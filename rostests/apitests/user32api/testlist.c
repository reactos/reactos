#ifndef _USER32TESTLIST_H
#define _USER32TESTLIST_H

#include "user32api.h"

/* include the tests */
#include "tests/ScrollDC.c"
#include "tests/ScrollWindowEx.c"

/* The List of tests */
TESTENTRY TestList[] =
{
	{ L"ScrollDC", Test_ScrollDC },
	{ L"ScrollWindowEx", Test_ScrollWindowEx }
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TESTENTRY);
}

#endif /* _USER32TESTLIST_H */

/* EOF */
