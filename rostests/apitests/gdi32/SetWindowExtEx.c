/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetWindowExtEx
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>

#include <wingdi.h>
#include <winuser.h>

#define TEST(x) ok(x, #x"\n")
#define RTEST(x) ok(x, #x"\n")

void Test_SetWindowExtEx()
{
    HDC hDC;
    BOOL ret;
    SIZE WindowExt, ViewportExt;
	//PGDI_TABLE_ENTRY pEntry;
	//DC_ATTR* pDC_Attr;

    hDC = CreateCompatibleDC(0);
	ok(hDC != NULL, "CreateCompatibleDC failed. Skipping tests.\n");
	if (hDC == NULL) return;

    SetLastError(0);
    ret = SetWindowExtEx(0, 0, 0, NULL);
    ok_err(ERROR_INVALID_HANDLE);
    TEST(ret == 0);

    SetLastError(0);
    ret = SetWindowExtEx((HDC)0x1234, 0, 0, NULL);
    ok_err(ERROR_INVALID_HANDLE);
    TEST(ret == 0);

    SetLastError(0);
    ret = SetWindowExtEx((HDC)0x10000, 0, 0, NULL);
    ok_err(ERROR_INVALID_PARAMETER);
    TEST(ret == 0);

    SetLastError(0);
    ret = SetWindowExtEx((HDC)0x210000, 0, 0, NULL); // GDILoObjType_LO_ALTDC_TYPE
    ok_err(ERROR_INVALID_HANDLE);
    TEST(ret == 0);

    SetLastError(0);
    ret = SetWindowExtEx((HDC)0x260000, 0, 0, NULL); // GDILoObjType_LO_METAFILE16_TYPE
    ok_err(ERROR_INVALID_HANDLE);
    TEST(ret == 0);

    SetLastError(0);
    ret = SetWindowExtEx((HDC)0x460000, 0, 0, NULL); // GDILoObjType_LO_METAFILE_TYPE
    ok_err(ERROR_INVALID_HANDLE);
    TEST(ret == 0);

    SetLastError(0);
    ret = SetWindowExtEx((HDC)0x660000, 0, 0, NULL); // GDILoObjType_LO_METADC16_TYPE
    ok_err(ERROR_INVALID_HANDLE);
    TEST(ret == 0);

    SetLastError(0);
    ret = SetWindowExtEx(hDC, 0, 0, NULL);
    ok_err(0);
    TEST(ret == 1);

    /* Test 16 bit handle */
    SetLastError(0);
    ret = SetWindowExtEx((HDC)((ULONG_PTR)hDC & 0xffff), 0, 0, NULL);
    ok_err(ERROR_INVALID_HANDLE);
    TEST(ret == 0);

    WindowExt.cx = 1234;
    WindowExt.cy = 6789;
    SetLastError(0);
    ret = SetWindowExtEx(0, 0, 0, &WindowExt);
    ok_err(ERROR_INVALID_HANDLE);
    TEST(ret == 0);
    TEST(WindowExt.cx == 1234);
    TEST(WindowExt.cy == 6789);

    DeleteDC(hDC);

    /* Test with a deleted DC */
    SetLastError(0);
    ret = SetWindowExtEx(hDC, 0, 0, NULL);
    ok_err(ERROR_INVALID_PARAMETER);
    TEST(ret == 0);

    hDC = CreateCompatibleDC(0);
	ok(hDC != NULL, "CreateCompatibleDC failed. Skipping tests.\n");
	if (hDC == NULL) return;

	//pEntry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hDC);
	//pDC_Attr = pEntry->UserData;
	//ASSERT(pDC_Attr);

    /* Test setting 0 extents without changing the map mode (MM_TEXT) */
    ret = SetWindowExtEx(hDC, 0, 0, &WindowExt);
    TEST(ret == 1);
    TEST(WindowExt.cx == 1);
    TEST(WindowExt.cy == 1);

    /* Test setting proper extents without changing the map mode (MM_TEXT) */
    WindowExt.cx = WindowExt.cy = 0;
    ret = SetWindowExtEx(hDC, 10, 20, &WindowExt);
    TEST(ret == 1);
    TEST(WindowExt.cx == 1);
    TEST(WindowExt.cy == 1);

    /* Values should not be changed */
    WindowExt.cx = WindowExt.cy = 0;
    ret = SetWindowExtEx(hDC, 40, 30, &WindowExt);
    TEST(ret == 1);
    TEST(WindowExt.cx == 1);
    TEST(WindowExt.cy == 1);

    /* Check the viewport */
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(ViewportExt.cx == 1);
    TEST(ViewportExt.cy == 1);

    /* Test setting in isotropic mode with 0 extents */
    SetMapMode(hDC, MM_ISOTROPIC);
    WindowExt.cx = WindowExt.cy = 0;
    ret = SetWindowExtEx(hDC, 0, 0, &WindowExt);
    TEST(ret == 0);
    //TEST(WindowExt.cx == 3600);
    //TEST(WindowExt.cy == 2700);
    ret = SetWindowExtEx(hDC, 100, 0, &WindowExt);
    TEST(ret == 0);
    ret = SetWindowExtEx(hDC, 0, 100, &WindowExt);
    TEST(ret == 0);

    /* Test setting in isotropic mode */
    ret = SetWindowExtEx(hDC, 21224, 35114, &WindowExt);
    TEST(ret == 1);
    //TEST(WindowExt.cx == 3600);
    //TEST(WindowExt.cy == 2700);

    /* Values should be changed */
    ret = SetWindowExtEx(hDC,
                         4 * GetDeviceCaps(GetDC(0), HORZRES),
                         -4 * GetDeviceCaps(GetDC(0), VERTRES),
                         &WindowExt);
    TEST(ret == 1);
    TEST(WindowExt.cx == 21224);
    TEST(WindowExt.cy == 35114);

    /* Check the viewport, should be the same */
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));

    /* again isotropic mode with 1:1 res */
    ret = SetWindowExtEx(hDC, 123, 123, &WindowExt);
    TEST(ret == 1);
    TEST(WindowExt.cx == 4 * GetDeviceCaps(GetDC(0), HORZRES));
    TEST(WindowExt.cy == -4 * GetDeviceCaps(GetDC(0), VERTRES));

    /* Test flXform */
    //TEST(pDC_Attr->flXform & PAGE_EXTENTS_CHANGED);

    /* Check the viewport from the dcattr, without going through gdi */
    //TEST(pDC_Attr->szlViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    //TEST(pDC_Attr->szlViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));

    /* Check the viewport with gdi, should not be the same */
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(ViewportExt.cx ==  GetDeviceCaps(GetDC(0), VERTRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));

    /* Test flXform */
    //TEST(pDC_Attr->flXform & PAGE_EXTENTS_CHANGED);

    /* again isotropic mode with 3:1 res */
    ret = SetWindowExtEx(hDC, 300, 100, &WindowExt);
    TEST(ret == 1);
    TEST(WindowExt.cx == 123);
    TEST(WindowExt.cy == 123);

    /* Check the viewport now, should not be the same */
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(ViewportExt.cx ==  GetDeviceCaps(GetDC(0), VERTRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES) / 3);

    /* again isotropic mode with 1:3 res */
    SetViewportExtEx(hDC, 6000, 3000, 0);
    ret = SetWindowExtEx(hDC, 200, 600, &WindowExt);
    TEST(ret == 1);
    TEST(WindowExt.cx == 300);
    TEST(WindowExt.cy == 100);

    /* Check the viewport now, should not be the same */
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(ViewportExt.cx == 1000);
    TEST(ViewportExt.cy == 3000);

    /* Test setting in anisotropic mode */
    SetMapMode(hDC, MM_ANISOTROPIC);
    ret = SetWindowExtEx(hDC, 80, 60, &WindowExt);
    TEST(ret == 1);
    TEST(WindowExt.cx == 200);
    TEST(WindowExt.cy == 600);

    /* Values should be changed */
    ret = SetWindowExtEx(hDC, 500, 500, &WindowExt);
    TEST(ret == 1);
    TEST(WindowExt.cx == 80);
    TEST(WindowExt.cy == 60);

    /* Check the viewport */
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(ViewportExt.cx == 1000);
    TEST(ViewportExt.cy == 3000);

    /* Test setting in low metric mode */
    SetMapMode(hDC, MM_LOMETRIC);
    ret = SetWindowExtEx(hDC, 120, 90, &WindowExt);
    TEST(ret == 1);
    //TEST(WindowExt.cx == 3600);
    //TEST(WindowExt.cy == 2700);

    /* Values should not be changed */
    WindowExt.cx = WindowExt.cy = 0;
    ret = SetWindowExtEx(hDC, 900, 700, &WindowExt);
    TEST(ret == 1);
    //TEST(WindowExt.cx == 3600);
    //TEST(WindowExt.cy == 2700);

    /* Check the viewport */
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));

    /* Test setting in high metric mode */
    SetMapMode(hDC, MM_HIMETRIC);
    ret = SetWindowExtEx(hDC, 120, 90, &WindowExt);
    TEST(ret == 1);
    //TEST(WindowExt.cx == 36000);
    //TEST(WindowExt.cy == 27000);

    /* Values should not be changed */
    WindowExt.cx = WindowExt.cy = 0;
    ret = SetWindowExtEx(hDC, 500, 300, &WindowExt);
    TEST(ret == 1);
    //TEST(WindowExt.cx == 36000);
    //TEST(WindowExt.cy == 27000);

    /* Check the viewport */
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));

    /* Test setting in low english mode */
    SetMapMode(hDC, MM_LOENGLISH);
    ret = SetWindowExtEx(hDC, 320, 290, &WindowExt);
    TEST(ret == 1);
    //TEST(WindowExt.cx == 1417);
    //TEST(WindowExt.cy == 1063);

    /* Values should not be changed */
    WindowExt.cx = WindowExt.cy = 0;
    ret = SetWindowExtEx(hDC, 560, 140, &WindowExt);
    TEST(ret == 1);
    //TEST(WindowExt.cx == 1417);
    //TEST(WindowExt.cy == 1063);

    /* Check the viewport */
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));

    /* Test setting in high english mode */
    SetMapMode(hDC, MM_HIENGLISH);
    ret = SetWindowExtEx(hDC, 320, 290, &WindowExt);
    TEST(ret == 1);
    //TEST(WindowExt.cx == 14173);
    //TEST(WindowExt.cy == 10630);

    /* Values should not be changed */
    WindowExt.cx = WindowExt.cy = 0;
    ret = SetWindowExtEx(hDC, 1560, 1140, &WindowExt);
    TEST(ret == 1);
    //TEST(WindowExt.cx == 14173);
    //TEST(WindowExt.cy == 10630);

    /* Check the viewport */
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));

    /* Test setting in twips mode */
    SetMapMode(hDC, MM_TWIPS);
    ret = SetWindowExtEx(hDC, 3320, 3290, &WindowExt);
    TEST(ret == 1);
    //TEST(WindowExt.cx == 20409);
    //TEST(WindowExt.cy == 15307);

    /* Values should not be changed */
    WindowExt.cx = WindowExt.cy = 0;
    ret = SetWindowExtEx(hDC, 4560, 4140, &WindowExt);
    TEST(ret == 1);
    //TEST(WindowExt.cx == 20409);
    //TEST(WindowExt.cy == 15307);

    /* Check the viewport */
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));

    /* test manually modifying the dcattr, should go to tests for GetViewportExtEx */
    SetMapMode(hDC, MM_ISOTROPIC);
    ret = SetWindowExtEx(hDC, 420, 4140, &WindowExt);
    //pDC_Attr->szlWindowExt.cx = 0;
    GetViewportExtEx(hDC, &ViewportExt);
    //TEST(pDC_Attr->szlWindowExt.cx == 0);
    //TEST(ViewportExt.cx == 0);

    DeleteDC(hDC);
}

START_TEST(SetWindowExtEx)
{
    Test_SetWindowExtEx();
}

