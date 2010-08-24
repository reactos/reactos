/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetSysColors
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

#define TEST(x) ok(x, #x)
#define RTEST(x) ok(x, #x)

#define NUM_SYSCOLORS 31

void Test_SetSysColors()
{
	INT i;
	INT nElements[NUM_SYSCOLORS];
	COLORREF crOldColors[NUM_SYSCOLORS];
	COLORREF crColors[3] = {RGB(212, 208, 200),2,3};

	/* First save the Old colors */
	for (i = 0; i < NUM_SYSCOLORS; i++)
	{
		nElements[i] = i;
		crOldColors[i] = GetSysColor(i);
	}

	TEST((UINT)SetSysColors(0, nElements, crColors) == 1);
	RTEST((UINT)SetSysColors(1, nElements, crColors) == 1);
	RTEST((UINT)SetSysColors(2, nElements, crColors) == 1);

	/* try more than NUM_SYSCOLORS */
	RTEST((UINT)SetSysColors(55, nElements, crColors) == 1);

	/* restore old SysColors */
	SetSysColors(NUM_SYSCOLORS, nElements, crOldColors);
}

START_TEST(SetSysColors)
{
    Test_SetSysColors();
}

