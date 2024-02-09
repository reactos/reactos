
#include "precomp.h"

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
}
