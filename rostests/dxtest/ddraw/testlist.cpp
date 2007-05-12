#ifndef _DDRAWTESTLIST_H
#define _DDRAWTESTLIST_H

#include "ddrawtest.h"

/* include the tests */
#include "tests/CreateDDraw.cpp"

/* The List of tests */
TEST TestList[] =
{
	{ "CreateDDraw", Test_CreateDDraw }
};

/* The function that gives us the number of tests */
extern "C" INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TEST);
}

#endif /* _DDRAWTESTLIST_H */

/* EOF */
