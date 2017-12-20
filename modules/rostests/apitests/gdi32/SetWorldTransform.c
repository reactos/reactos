/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetWorldTransform
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

void Test_SetWorldTransform()
{
	HDC hdcScreen, hdc;
	XFORM xform;
	BOOL result;
	//PGDI_TABLE_ENTRY pEntry;
	//DC_ATTR* pdcattr;

	/* Create a DC */
	hdcScreen = GetDC(NULL);
	hdc = CreateCompatibleDC(hdcScreen);
	ReleaseDC(NULL, hdcScreen);
	SetGraphicsMode(hdc, GM_ADVANCED);

    /* Set identity transform */
    xform.eM11 = 1;
    xform.eM12 = 0;
    xform.eM21 = 0;
    xform.eM22 = 1;
    xform.eDx = 0;
    xform.eDy = 0;
    result = SetWorldTransform(hdc, &xform);
    ok(result == 1, "\n");

    /* Something invalid */
    xform.eM22 = 0;
    result = SetWorldTransform(hdc, &xform);
    ok(result == 0, "\n");

	//pEntry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hdc);
	//pdcattr = pEntry->UserData;

	DeleteDC(hdc);
}

START_TEST(SetWorldTransform)
{
    Test_SetWorldTransform();
}

