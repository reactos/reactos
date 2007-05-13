#ifndef _DDRAWTESTLIST_H
#define _DDRAWTESTLIST_H

#include "ddrawtest.h"
#include "debug.cpp"

/* include the tests */
#include "tests/CreateDDraw.cpp"
#include "tests/DisplayModes.cpp"

/* The List of tests */
TEST TestList[] =
{
	{ "DirectDrawCreate(Ex)", Test_CreateDDraw },
	{ "IDirectDraw::SetCooperativeLevel", Test_SetCooperativeLevel },
	// { "IDirectDraw::EnumDisplayModes/SetDisplayMode", Test_DisplayModes } // uncomment this test if you have enough time and patience
};

/* The function that gives us the number of tests */
extern "C" INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TEST);
}

#endif /* _DDRAWTESTLIST_H */

/* EOF */
