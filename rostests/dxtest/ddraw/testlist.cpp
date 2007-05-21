#ifndef _DDRAWTESTLIST_H
#define _DDRAWTESTLIST_H

#include "ddrawtest.h"
#include "debug.cpp"

/* include the tests */
#include "tests/CreateDDraw.cpp"
#include "tests/DisplayModes.cpp"
#include "tests/CreateSurface.cpp"

/* The List of tests */
TEST TestList[] =
{
	{ "IDirectDraw: COM Stuff", Test_CreateDDraw },
	{ "IDirectDraw: Display Modes", Test_DisplayModes },
	{ "IDirectDraw: Available Video Memory", Test_GetAvailableVidMem },
	{ "IDirectDraw: GetFourCC", Test_GetFourCCCodes },
	{ "IDirectDraw: Cooperative Levels", Test_SetCooperativeLevel },
	{ "IDirectDraw: CreateSurface", Test_CreateSurface },
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TEST);
}

#endif /* _DDRAWTESTLIST_H */

/* EOF */
