
#include "precomp.h"
#include "resource_1bpp.h"

#define ROS_HGDI_ERROR (HANDLE)~(ULONG_PTR)0 // This makes MSVC compiler happy

static void test_LoadImage_1bpp(void)
{
    HDC hdc1, hdc2;
    HBITMAP hBmp1, hBmp2;
    BITMAP bitmap1, bitmap2;
    BITMAPINFO bmi;
    UINT size;
    HGLOBAL hMem;
    LPVOID lpBits;
    BYTE img[4 * 4] = { 0 };
    INT result;
    HGDIOBJ res_obj;

    hdc1 = CreateCompatibleDC(NULL);
    /* Load bitmap with BITMAPINFOHEADER (40 bytes) */
    hBmp1 = LoadImageW(GetModuleHandle(NULL), MAKEINTRESOURCEW(201), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    res_obj = SelectObject(hdc1, hBmp1);
    if (res_obj == ROS_HGDI_ERROR || res_obj == NULL)
    {
        skip("Could not load 1 BPP bitmap\n");
        goto Cleanup;
    }
    GetObject(hBmp1, sizeof(BITMAP), &bitmap1);

    ok(bitmap1.bmBitsPixel == 1, "Should have been '1', but got '%d'\n", bitmap1.bmBitsPixel);
    ok(bitmap1.bmWidth == 4, "Should have been '4', but got '%d'\n", bitmap1.bmWidth);
    ok(bitmap1.bmHeight == 4, "Should have been '4', but got '%d'\n", bitmap1.bmHeight);

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = bitmap1.bmWidth;
    bmi.bmiHeader.biHeight      = bitmap1.bmHeight;
    bmi.bmiHeader.biPlanes      = bitmap1.bmPlanes;
    bmi.bmiHeader.biBitCount    = bitmap1.bmBitsPixel;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = 0;

    /* Get the size of the bitmap */
    size = ((bitmap1.bmWidth * bitmap1.bmBitsPixel + 31) / 32) * 4 * bitmap1.bmHeight;
    ok(size == 16, "Expected 16, but size is %d\n", size);

    hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    lpBits = GlobalLock(hMem);
    result = GetDIBits(hdc1, hBmp1, 0, bitmap1.bmHeight, lpBits, &bmi, DIB_RGB_COLORS);
    if (!result)
    {
        skip("GetDIBits failed for 1 BPP bitmap\n");
        goto Cleanup;
    }

    /* Get bytes from bitmap (we know its 4x4 1BPP */
    memcpy(img, lpBits, 16);

    ok(img[0] == 0x60, "Got 0x%02x, expected x60\n", img[0]);
    ok(img[1] == 0, "Got 0x%02x, expected 0\n", img[1]);
    ok(img[2] == 0, "Got 0x%02x, expected 0\n", img[2]);
    ok(img[3] == 0, "Got 0x%02x, expected 0\n", img[3]);

    ok(img[4] == 0x90, "Got 0x%02x, expected x90\n", img[4]);
    ok(img[5] == 0, "Got 0x%02x, expected 0x60\n", img[5]);
    ok(img[6] == 0, "Got 0x%02x, expected 0\n", img[6]);
    ok(img[7] == 0, "Got 0x%02x, expected 0\n", img[7]);

    ok(img[8] == 0x90, "Got 0x%02x, expected x90\n", img[8]);
    ok(img[9] == 0, "Got 0x%02x, expected 0\n", img[9]);
    ok(img[10] == 0, "Got 0x%02x, expected 0\n", img[10]);
    ok(img[11] == 0, "Got 0x%02x, expected 0\n", img[11]);

    ok(img[12] == 0x60, "Got 0x%02x, expected x60\n", img[12]);
    ok(img[13] == 0, "Got 0x%02x, expected 0x60\n", img[13]);
    ok(img[14] == 0, "Got 0x%02x, expected 0\n", img[14]);
    ok(img[15] == 0, "Got 0x%02x, expected 0\n", img[15]);

    hdc2 = CreateCompatibleDC(NULL);
    /* Load bitmap with BITMAPCOREHEADER (12 bytes) */
    hBmp2 = LoadImageW(GetModuleHandle(NULL), MAKEINTRESOURCEW(202), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    res_obj = SelectObject(hdc2, hBmp2);
    if (res_obj == ROS_HGDI_ERROR || res_obj == NULL)
    {
        skip("Could not load 1 BPP bitmap\n");
        goto Cleanup;
    }
    GetObject(hBmp2, sizeof(BITMAP), &bitmap2);
    ok(bitmap2.bmBitsPixel == 1, "Should have been '1', but got %d\n", bitmap2.bmBitsPixel);
    ok(bitmap2.bmWidth == 4, "Should have been '4', but got '%d'\n", bitmap2.bmWidth);
    ok(bitmap2.bmHeight == 4, "Should have been '4', but got '%d'\n", bitmap2.bmHeight);

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = bitmap2.bmWidth;
    bmi.bmiHeader.biHeight      = bitmap2.bmHeight;
    bmi.bmiHeader.biPlanes      = bitmap2.bmPlanes;
    bmi.bmiHeader.biBitCount    = bitmap2.bmBitsPixel;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = 0;

    /* Get the size of the bitmap */
    size = ((bitmap2.bmWidth * bitmap2.bmBitsPixel + 31) / 32) * 4 * bitmap2.bmHeight;
    ok(size == 16, "Expected 16, but size is %d\n", size);

    result = GetDIBits(hdc2, hBmp2, 0, bitmap2.bmHeight, lpBits, &bmi, DIB_RGB_COLORS); // Check for success per Timo
    if (!result)
    {
        skip("GetDIBits failed for 1 BPP bitmap\n");
        goto Cleanup;
    }

    /* Clear img array for new test */
    memset(img, 0, 16);

    /* Get bytes from bitmap (we know its 4x4 1BPP */
    memcpy(img, lpBits, 16);

    ok(img[0] == 0x60 || broken(img[0] == 0) /* Vista Testbot */, "Got 0x%02x, expected x60\n", img[0]);
    ok(img[1] == 0, "Got 0x%02x, expected 0\n", img[1]);
    ok(img[2] == 0, "Got 0x%02x, expected 0\n", img[2]);
    ok(img[3] == 0, "Got 0x%02x, expected 0\n", img[3]);

    ok(img[4] == 0x90 || broken(img[4] == 0xf0) /* Vista Testbot */, "Got 0x%02x, expected x90\n", img[4]);
    ok(img[5] == 0, "Got 0x%02x, expected 0x60\n", img[5]);
    ok(img[6] == 0, "Got 0x%02x, expected 0\n", img[6]);
    ok(img[7] == 0, "Got 0x%02x, expected 0\n", img[7]);

    ok(img[8] == 0x90 || broken(img[8] == 0xf0) /* Vista Testbot */, "Got 0x%02x, expected x90\n", img[8]);
    ok(img[9] == 0, "Got 0x%02x, expected 0\n", img[9]);
    ok(img[10] == 0, "Got 0x%02x, expected 0\n", img[10]);
    ok(img[11] == 0, "Got 0x%02x, expected 0\n", img[11]);

    ok(img[12] == 0x60 || broken(img[12] == 0xf0) /* Vista Testbot */, "Got 0x%02x, expected x60\n", img[12]);
    ok(img[13] == 0, "Got 0x%02x, expected 0x60\n", img[13]);
    ok(img[14] == 0, "Got 0x%02x, expected 0\n", img[14]);
    ok(img[15] == 0, "Got 0x%02x, expected 0\n", img[15]);

Cleanup:
    if (hMem) GlobalUnlock(hMem);
    if (hMem) GlobalFree(hMem);
    if (hdc1) DeleteDC(hdc1);
    if (hdc2) DeleteDC(hdc2);
}

static void test_LoadImage_DataFile(void)
{
    static const struct
    {
        int result;
        LPCWSTR file;
        int res_id;
        UINT lr;
        BOOL same_handle;
        BOOL after_unload; /* LR_SHARED stays valid */
    }
    tests[] =
    {
        { 1, L"shell32.dll", 2,        0,         0, 0 },
        { 1, L"shell32.dll", 2,        LR_SHARED, 1, 1 },
        { 0, L"shell32.dll", 0xfff0,   0,         1, 0 }, /* Icon should not exist */
        { 1, L"regedit.exe", 100,      0,         0, 0 },
        { 1, L"regedit.exe", 100,      LR_SHARED, 1, 1 }
    };

    SIZE_T i;
    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        HANDLE handle1, handle2;
        HMODULE hMod = LoadLibraryExW(tests[i].file, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (!((SIZE_T)hMod & 3))
        {
            skip("Could not load library as datafile %ls\n", tests[i].file);
            continue;
        }

        handle1 = LoadImage(hMod, MAKEINTRESOURCE(tests[i].res_id), IMAGE_ICON, 0, 0, tests[i].lr);
        ok(!!handle1 == !!tests[i].result, "Failed to load %ls,-%d from %p\n", tests[i].file, tests[i].res_id, hMod);

        handle2 = LoadImage(hMod, MAKEINTRESOURCE(tests[i].res_id), IMAGE_ICON, 0, 0, tests[i].lr);
        ok(!!(handle1 == handle2) == !!tests[i].same_handle, "Shared handles don't match\n");

        FreeLibrary(hMod);

        handle1 = LoadImage(hMod, MAKEINTRESOURCE(tests[i].res_id), IMAGE_ICON, 0, 0, tests[i].lr);
        ok(!!handle1 == !!tests[i].after_unload, "LR_%x handle should %sload after FreeLibrary\n", tests[i].lr, tests[i].after_unload ? "" : "not ");
    }
}

static void test_LoadIcon_SystemIds(void)
{
    static const WORD icomap[][2] = {
        { 100, (WORD)(SIZE_T)IDI_APPLICATION },
        { 101, (WORD)(SIZE_T)IDI_WARNING },
        { 102, (WORD)(SIZE_T)IDI_QUESTION },
        { 103, (WORD)(SIZE_T)IDI_ERROR },
        { 104, (WORD)(SIZE_T)IDI_INFORMATION },
        { 105, (WORD)(SIZE_T)IDI_WINLOGO }
    };
    HINSTANCE hInst = GetModuleHandleW(L"USER32");
    typedef BOOL (WINAPI*SHAIE)(HICON, HICON);
    SHAIE pfnSHAreIconsEqual;
    HMODULE hSHLWAPI = LoadLibraryA("SHLWAPI");
    if (!hSHLWAPI)
    {
        skip("Could not initialize\n");
        return;
    }
    pfnSHAreIconsEqual = (SHAIE)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(548));
    if (!pfnSHAreIconsEqual)
    {
        FreeLibrary(hSHLWAPI);
        skip("Could not initialize\n");
        return;
    }

    for (UINT i = 0; i < _countof(icomap); i++)
    {
        HICON hIcoRes = LoadIconW(hInst, MAKEINTRESOURCEW(icomap[i][0]));
        HICON hIcoSys = LoadIconW(NULL, MAKEINTRESOURCEW(icomap[i][1]));
        ok(hIcoRes && pfnSHAreIconsEqual(hIcoRes, hIcoSys), "SysIcon %d must be resource %d\n", icomap[i][1], icomap[i][0]);
    }
    FreeLibrary(hSHLWAPI);
}

START_TEST(LoadImage)
{
    char path[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    HANDLE handle;

    char **test_argv;
    int argc = winetest_get_mainargs( &test_argv );

    /* Now check its behaviour regarding Shared icons/cursors */
    handle = LoadImageW( GetModuleHandle(NULL), L"TESTCURSOR", IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE );
    ok(handle != 0, "\n");

    if (argc >= 3)
    {
        HANDLE arg;
        HICON hCopy;
        HBITMAP hbmp;
        HDC hdc, hdcScreen;
        ICONINFO ii;

        sscanf (test_argv[2], "%Iu", (ULONG_PTR*) &arg);

        ok(handle != arg, "Got same handles\n");

        /* Try copying it */
        hCopy = CopyIcon(arg);
        ok(hCopy != NULL, "\n");
        ok(DestroyIcon(hCopy), "\n");

        hCopy = CopyImage(arg, IMAGE_CURSOR, 0, 0, 0);
        ok(hCopy != NULL, "\n");
        ok(DestroyIcon(hCopy), "\n");
        /* Unlike the original, this one is not shared */
        ok(!DestroyIcon(hCopy), "\n");

        hCopy = CopyImage(arg, IMAGE_CURSOR, 0, 0, LR_COPYFROMRESOURCE);
        ok(hCopy != NULL, "\n");
        ok(DestroyIcon(hCopy), "\n");
        /* Unlike the original, this one is not shared */
        ok(!DestroyIcon(hCopy), "\n");

        hCopy = CopyImage(arg, IMAGE_CURSOR, 0, 0, LR_COPYFROMRESOURCE | LR_SHARED);
        ok(hCopy != NULL, "\n");
        ok(DestroyIcon(hCopy), "\n");
        /* This one is shared */
        ok(DestroyIcon(hCopy), "\n");

        hCopy = CopyImage(arg, IMAGE_CURSOR, 0, 0, LR_SHARED);
        ok(hCopy != NULL, "\n");
        ok(DestroyIcon(hCopy), "DestroyIcon should succeed.\n");
        /* This one is shared */
        ok(DestroyIcon(hCopy) == 0, "DestroyIcon should fail.\n");

        /* Try various usual functions */
        hdcScreen = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
        ok(hdcScreen != NULL, "\n");
        hdc = CreateCompatibleDC(hdcScreen);
        ok(hdc != NULL, "\n");
        hbmp = CreateCompatibleBitmap(hdcScreen, 64, 64);
        ok(hbmp != NULL, "\n");
        hbmp = SelectObject(hdc, hbmp);
        ok(hbmp != NULL, "\n");

        ok(DrawIcon(hdc, 0, 0, arg), "\n");
        hbmp = SelectObject(hdc, hbmp);
        DeleteObject(hbmp);
        DeleteDC(hdc);
        DeleteDC(hdcScreen);

        ok(GetIconInfo(arg, &ii), "\n");
        ok(ii.hbmMask != NULL, "\n");
        DeleteObject(ii.hbmMask);
        if(ii.hbmColor) DeleteObject(ii.hbmColor);

        /* LOAD_LIBRARY_AS_DATAFILE */
        test_LoadImage_DataFile();

        return;
    }

    /* Start child process */
    sprintf( path, "%s LoadImage %Iu", test_argv[0], (ULONG_PTR)handle );
    memset( &si, 0, sizeof(si) );
    si.cb = sizeof(si);
    CreateProcessA( NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi );
    WaitForSingleObject (pi.hProcess, INFINITE);

    test_LoadIcon_SystemIds();
    test_LoadImage_1bpp();
}
