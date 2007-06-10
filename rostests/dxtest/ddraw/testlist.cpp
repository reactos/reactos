#ifndef _DDRAWTESTLIST_H
#define _DDRAWTESTLIST_H

#include "ddrawtest.h"
#include "debug.cpp"

/* include the tests */
#include "DDraw/create.cpp"
#include "DDraw/display_modes.cpp"
#include "Surface/create.cpp"
#include "Surface/private_data.cpp"
#include "Surface/misc.cpp"

/* The List of tests */
TEST TestList[] =
{
	{ "IDirectDraw: COM Stuff", Test_CreateDDraw },
	{ "IDirectDraw: GetDeviceIdentifier", Test_GetDeviceIdentifier },
	{ "IDirectDraw: Display Frequency", Test_GetMonitorFrequency },
	{ "IDirectDraw: Display Modes", Test_DisplayModes },
	{ "IDirectDraw: Available Video Memory", Test_GetAvailableVidMem },
	{ "IDirectDraw: GetFourCC", Test_GetFourCCCodes },
	{ "IDirectDraw: Cooperative Levels", Test_SetCooperativeLevel },
    { "IDirectDrawSurface: Creation", Test_CreateSurface },
	{ "IDirectDrawSurface: Private Data", Test_PrivateData },
	{ "IDirectDrawSurface: Miscellaneous Tests", Test_Misc },
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TEST);
}

#endif /* _DDRAWTESTLIST_H */

/* EOF */
