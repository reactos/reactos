#line 2 "SetSysColors.c"

#include "../gditest.h"

#define NUM_SYSCOLORS 31

BOOL Test_SetSysColors(INT* passed, INT* failed)
{
	INT i;
	INT nElements[NUM_SYSCOLORS];
	COLORREF crOldColors[NUM_SYSCOLORS];
	COLORREF crColors[3] = {RGB(212, 208, 200),2,3};

return TRUE; // This is because codeblocks doesn't like changing syscolors ;-(

	/* First save the Old colors */
	for (i = 0; i < NUM_SYSCOLORS; i++)
	{
		nElements[i] = i;
		crOldColors[i] = GetSysColor(i);
	}

	TEST((UINT)SetSysColors(0, nElements, crColors) == 1);
	TEST((UINT)SetSysColors(1, nElements, crColors) == 1);
	TEST((UINT)SetSysColors(2, nElements, crColors) == 1);
	
	/* try more than NUM_SYSCOLORS */
	TEST((UINT)SetSysColors(55, nElements, crColors) == 1);

	/* restore old SysColors */
	SetSysColors(NUM_SYSCOLORS, nElements, crOldColors);

	return TRUE;
}
