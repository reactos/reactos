#ifndef _DCIMAN32TESTLIST_H
#define _DCIMAN32TESTLIST_H

#include "dciman32api.h"

/* include the tests */
#include "tests/DCICreatePrimary.c"










/* The List of tests */
TESTENTRY TestList[] =
{
    { L"DCICreatePrimary", Test_DCICreatePrimary }
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TESTENTRY);
}

#endif

/* EOF */
