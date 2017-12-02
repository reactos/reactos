/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetSysColors
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

#define NUM_SYSCOLORS 31

void Test_SetSysColors()
{
	INT i;
	INT nElements[NUM_SYSCOLORS+1];
	COLORREF crOldColors[NUM_SYSCOLORS];
	COLORREF crColors[NUM_SYSCOLORS+1];

	/* First save the Old colors */
	for (i = 0; i < NUM_SYSCOLORS; i++)
	{
		nElements[i] = i;
		crOldColors[i] = GetSysColor(i);
	}

	for (i = 0; i < NUM_SYSCOLORS+1; i++)
		crColors[i] = RGB(i, 255-i, i*3);
	nElements[NUM_SYSCOLORS] = nElements[0];

	SetLastError(0xdeadbeef);
	ok(SetSysColors(-1, nElements, crColors) == FALSE, "Expected FALSE, got TRUE\n");
	ok(GetLastError() == ERROR_NOACCESS, "Expected ERROR_NOACCESS, got %ld\n", GetLastError());
	ok(SetSysColors(0, nElements, crColors) == TRUE, "Expected TRUE, got FALSE\n");
	ok(SetSysColors(0, NULL, crColors) == TRUE, "Expected TRUE, got FALSE\n");
	ok(SetSysColors(0, nElements, NULL) == TRUE, "Expected TRUE, got FALSE\n");
	ok(SetSysColors(1, NULL, crColors) == FALSE, "Expected FALSE, got TRUE\n");
	ok(GetLastError() == ERROR_NOACCESS, "Expected ERROR_NOACCESS, got %ld\n", GetLastError());
	ok(SetSysColors(1, nElements, NULL) == FALSE, "Expected FALSE, got TRUE\n");
	ok(GetLastError() == ERROR_NOACCESS, "Expected ERROR_NOACCESS, got %ld\n", GetLastError());
	ok(SetSysColors(1, nElements, crColors) == TRUE, "Expected TRUE, got FALSE\n");
	ok(SetSysColors(NUM_SYSCOLORS, nElements, crColors) == TRUE, "Expected TRUE, got FALSE\n");
	for (i = 0; i < NUM_SYSCOLORS; i++)
		ok(GetSysColor(nElements[i]) == crColors[i], "Expected %06lx, got %06lx\n", crColors[i], GetSysColor(nElements[i]));

	/* try more than NUM_SYSCOLORS */
	ok(SetSysColors(NUM_SYSCOLORS+1, nElements, crColors) == TRUE, "Expected TRUE, got FALSE\n");
	nElements[NUM_SYSCOLORS] = 10000;
	ok(SetSysColors(NUM_SYSCOLORS+1, nElements, crColors) == TRUE, "Expected TRUE, got FALSE\n");

	/* restore old SysColors */
	SetSysColors(NUM_SYSCOLORS, nElements, crOldColors);
}

START_TEST(SetSysColors)
{
    Test_SetSysColors();
}

