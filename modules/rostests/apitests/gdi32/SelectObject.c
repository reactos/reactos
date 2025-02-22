/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SelectObject
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

HDC hdc1, hdc2;

static void
Test_SelectObject()
{
	HGDIOBJ hOldObj, hNewObj;
//	PGDI_TABLE_ENTRY pEntry;
//	PDC_ATTR pDc_Attr;
//	HANDLE hcmXform;

	/* Get the Dc_Attr for later testing */
//	pEntry = &GdiHandleTable[GDI_HANDLE_GET_INDEX(hdc1)];
//	pDc_Attr = pEntry->UserData;
//	ok(pDc_Attr != NULL, "Skipping tests.\n");
//	if (pDc_Attr == NULL) return;

	/* Test incomplete dc handle doesn't work */
	SetLastError(ERROR_SUCCESS);
	hNewObj = GetStockObject(GRAY_BRUSH);
	hOldObj = SelectObject((HDC)GDI_HANDLE_GET_INDEX(hdc1), hNewObj);
	ok_err(ERROR_INVALID_HANDLE);
	ok(hOldObj == NULL, "\n");
//	ok(pDc_Attr->hbrush == GetStockObject(WHITE_BRUSH), "\n");
	SelectObject(hdc1, hOldObj);

	/* Test incomplete hobj handle works */
	hNewObj = GetStockObject(GRAY_BRUSH);
	hOldObj = SelectObject(hdc1, (HGDIOBJ)GDI_HANDLE_GET_INDEX(hNewObj));
	ok(hOldObj == GetStockObject(WHITE_BRUSH), "\n");
//	ok(pDc_Attr->hbrush == hNewObj, "\n");
	SelectObject(hdc1, hOldObj);

	/* Test wrong hDC handle type */
	SetLastError(ERROR_SUCCESS);
	hNewObj = GetStockObject(GRAY_BRUSH);
	hdc2 = (HDC)((UINT_PTR)hdc1 & ~GDI_HANDLE_TYPE_MASK);
	hdc2 = (HDC)((UINT_PTR)hdc2 | GDI_OBJECT_TYPE_PEN);
	hOldObj = SelectObject(hdc2, hNewObj);
	ok_err(ERROR_INVALID_HANDLE);
	ok(hOldObj == NULL, "\n");
//	RTEST(pDc_Attr->hbrush == GetStockObject(WHITE_BRUSH));

	/* Test wrong hobj handle type */
	SetLastError(ERROR_SUCCESS);
	hNewObj = GetStockObject(GRAY_BRUSH);
	hNewObj = (HGDIOBJ)((UINT_PTR)hNewObj & ~GDI_HANDLE_TYPE_MASK);
	hNewObj = (HGDIOBJ)((UINT_PTR)hNewObj | GDI_OBJECT_TYPE_PEN);
	hOldObj = SelectObject(hdc1, hNewObj);
	ok_err(ERROR_SUCCESS);
	ok(hOldObj == NULL, "\n");
//	RTEST(pDc_Attr->hbrush == GetStockObject(WHITE_BRUSH));

	SetLastError(ERROR_SUCCESS);
	hNewObj = (HGDIOBJ)0x00761234;
	hOldObj = SelectObject(hdc1, hNewObj);
	ok(hOldObj == NULL, "\n");
	ok_err(ERROR_SUCCESS);
	SelectObject(hdc1, hOldObj);

	/* Test DC */
	SetLastError(ERROR_SUCCESS);
	hOldObj = SelectObject(hdc1, GetDC(NULL));
	ok(hOldObj == NULL, "\n");
	ok_err(ERROR_SUCCESS);


	/* Test CLIOBJ */

	/* Test PATH */

	/* Test PALETTE */
	SetLastError(ERROR_SUCCESS);
	hNewObj = GetStockObject(DEFAULT_PALETTE);
	hOldObj = SelectObject(hdc1, hNewObj);
	RTEST(hOldObj == NULL);
	ok_err(ERROR_INVALID_FUNCTION);

	/* Test COLORSPACE */

	/* Test FONT */

	/* Test PFE */

	/* Test BRUSH */
	hNewObj = GetStockObject(GRAY_BRUSH);
	hOldObj = SelectObject(hdc1, hNewObj);
	RTEST(hOldObj == GetStockObject(WHITE_BRUSH));
//	RTEST(pDc_Attr->hbrush == hNewObj);
	RTEST(GDI_HANDLE_GET_TYPE(hOldObj) == GDI_OBJECT_TYPE_BRUSH);
	SelectObject(hdc1, hOldObj);

	/* Test DC_BRUSH */
	hNewObj = GetStockObject(DC_BRUSH);
	hOldObj = SelectObject(hdc1, hNewObj);
//	RTEST(pDc_Attr->hbrush == hNewObj);
	SelectObject(hdc1, hOldObj);

	/* Test BRUSH color xform */
//	hcmXform = (HANDLE)pDc_Attr->hcmXform;


	/* Test EMF */

	/* test METAFILE */
	SetLastError(ERROR_SUCCESS);
	hNewObj = CreateMetaFile(NULL);
	ok(hNewObj != 0, "failed to create a meta dc\n");
	hOldObj = SelectObject(hdc1, hNewObj);
	RTEST(hOldObj == NULL);
	ok_err(ERROR_SUCCESS);

	/* Test ENHMETAFILE */

	/* Test EXTPEN */

	/* Test METADC */
}

static void
Test_Bitmap()
{
    HBITMAP hbmp, hbmpInvalid, hbmpOld;
	BYTE bmBits[4] = {0};
	HDC hdcTmp;

	hbmp = CreateBitmap(2, 2, 1, 1, &bmBits);
	hbmpInvalid = CreateBitmap(2, 2, 1, 4, &bmBits);
	if (!hbmp || !hbmpInvalid)
	{
	    printf("couldn't create bitmaps, skipping\n");
	    return;
	}

    hbmpOld = SelectObject(hdc1, hbmp);
    ok(GDI_HANDLE_GET_TYPE(hbmpOld) == GDI_OBJECT_TYPE_BITMAP, "wrong type\n");

	/* Test invalid BITMAP */
    ok(SelectObject(hdc1, hbmpInvalid) == NULL, "should fail\n");

    /* Test if we get the right bitmap back */
    hbmpOld = SelectObject(hdc1, hbmpOld);
    ok(hbmpOld == hbmp, "didn't get the right bitmap back.\n");

    /* Test selecting bitmap into 2 DCs */
    hbmpOld = SelectObject(hdc1, hbmp);
    ok(SelectObject(hdc2, hbmp) == NULL, "Should fail.\n");

    /* Test selecting same bitmap twice */
    hbmpOld = SelectObject(hdc1, hbmp);
    ok(hbmpOld == hbmp, "didn't get the right bitmap back.\n");
    SelectObject(hdc1, GetStockObject(DEFAULT_BITMAP));

    /* Test selecting and then deleting the DC */
    hdcTmp = CreateCompatibleDC(NULL);
    hbmpOld = SelectObject(hdcTmp, hbmp);
    ok(hbmpOld == GetStockObject(DEFAULT_BITMAP), "didn't get the right bitmap back.\n");
    DeleteDC(hdcTmp);
    hbmpOld = SelectObject(hdc1, hbmp);
    ok(hbmpOld == GetStockObject(DEFAULT_BITMAP), "didn't get the right bitmap back.\n");

    DeleteObject(hbmp);
    DeleteObject(hbmpInvalid);
}

static void
Test_Pen()
{
    HPEN hpen, hpenOld;

	/* Test PEN */
	hpen = GetStockObject(GRAY_BRUSH);
	hpenOld = SelectObject(hdc1, hpen);
	ok(hpenOld == GetStockObject(WHITE_BRUSH), "Got wrong pen.\n");
//	RTEST(pDc_Attr->hbrush == hpen);
	ok(GDI_HANDLE_GET_TYPE(hpenOld) == GDI_OBJECT_TYPE_BRUSH, "wrong type.\n");
	SelectObject(hdc1, hpenOld);
}

static void
Test_Region()
{
    HRGN hrgn, hrgnOld;

	/* Test REGION */
	SetLastError(ERROR_SUCCESS);
	hrgn = CreateRectRgn(0,0,0,0);
	hrgnOld = SelectObject(hdc1, hrgn);
	ok((UINT_PTR)hrgnOld == NULLREGION, "\n");
	DeleteObject(hrgn);

	hrgn = CreateRectRgn(0,0,10,10);
	ok((UINT_PTR)SelectObject(hdc1, hrgn) == SIMPLEREGION, "\n");
	hrgnOld = CreateRectRgn(5,5,20,20);
	ok(CombineRgn(hrgn, hrgn, hrgnOld, RGN_OR) == COMPLEXREGION, "\n");
	DeleteObject(hrgnOld);
	ok((UINT_PTR)SelectObject(hdc1, hrgn) == SIMPLEREGION, "\n"); // ??? Why this?
	DeleteObject(hrgn);
//	ok(IsHandleValid(hrgn) == TRUE, "\n");
	ok_err(ERROR_SUCCESS);
}

START_TEST(SelectObject)
{
	hdc1 = CreateCompatibleDC(NULL);
	hdc2 = CreateCompatibleDC(NULL);
	if (!hdc1 || !hdc2)
	{
	    printf("couldn't create DCs, skipping all tests\n");
	    return;
	}

    Test_SelectObject();
    Test_Bitmap();
    Test_Pen();
    Test_Region();
}

