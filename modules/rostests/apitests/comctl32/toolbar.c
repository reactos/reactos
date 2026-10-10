/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for toolbar window class v6
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include "wine/test.h"
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <undocuser.h>
#include <msgtrace.h>
#include <user32testhelpers.h>

HANDLE _CreateV5ActCtx(void)
{
    ACTCTXW ActCtx = {sizeof(ACTCTX)};
    WCHAR buffer[MAX_PATH] , *separator;

    ok (GetModuleFileNameW(NULL, buffer, MAX_PATH), "GetModuleFileName failed\n");
    separator = wcsrchr(buffer, L'\\');
    if (separator)
        wcscpy(separator + 1, L"comctl32v5.manifest");

    ActCtx.lpSource = buffer;

    return CreateActCtxW(&ActCtx);;
}


void TestVersionMessage(void)
{
    HWND hwnd;
    int version;

    hwnd = CreateWindowExW(0, TOOLBARCLASSNAMEW, L"Test", 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed\n");

    version = SendMessageW(hwnd, CCM_GETVERSION, 0, 0);
    ok(version == 6, "Got %d, expected 6\n", version);

    version = SendMessageW(hwnd, CCM_SETVERSION, 5, 0);
    ok(version == 6, "Got %d, expected 6\n", version);

    version = SendMessageW(hwnd, CCM_GETVERSION, 0, 0);
    ok(version == 6, "Got %d, expected 6\n", version);

    version = SendMessageW(hwnd, CCM_SETVERSION, 7, 0);
    ok(version == 6, "Got %d, expected 6\n", version);

    version = SendMessageW(hwnd, CCM_GETVERSION, 0, 0);
    ok(version == 6, "Got %d, expected 6\n", version);

    version = SendMessageW(hwnd, CCM_SETVERSION, 4, 0);
    ok(version == 6, "Got %d, expected 6\n", version);

    version = SendMessageW(hwnd, CCM_GETVERSION, 0, 0);
    ok(version == 6, "Got %d, expected 6\n", version);

    DestroyWindow(hwnd);
}

void TestSetButtonSize(void)
{
    HWND hwnd;
    LRESULT bsize;

    hwnd = CreateWindowExW(0, TOOLBARCLASSNAMEW, L"Test", 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed\n");

    bsize = SendMessageW(hwnd, TB_GETBUTTONSIZE, 0, 0);
    ok(bsize == 0x160017, "Expected 0x160017 got %lx\n", bsize);

    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, MAKELONG(0, 0));
    bsize = SendMessageW(hwnd, TB_GETBUTTONSIZE, 0, 0);
    ok(bsize == 0x160018, "Expected 0x160018 got %lx\n", bsize);

    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, 0x10001);
    bsize = SendMessageW(hwnd, TB_GETBUTTONSIZE, 0, 0);
    ok(bsize == 0x160017, "Expected 0x160017 got %lx\n", bsize);

    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, 0x100001);
    bsize = SendMessageW(hwnd, TB_GETBUTTONSIZE, 0, 0);
    ok(bsize == 0x160017, "Expected 0x160017 got %lx\n", bsize);

    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, 0x160017);
    bsize = SendMessageW(hwnd, TB_GETBUTTONSIZE, 0, 0);
    ok(bsize == 0x160017, "Expected 0x160017 got %lx\n", bsize);

    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, 0x170017);
    bsize = SendMessageW(hwnd, TB_GETBUTTONSIZE, 0, 0);
    ok(bsize == 0x170017, "Expected 0x170017 got %lx\n", bsize);

    DestroyWindow(hwnd);
}

void TestPadding(void)
{
    HWND hwnd;
    LRESULT bsize;

    hwnd = CreateWindowExW(0, TOOLBARCLASSNAMEW, L"Test", 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed\n");

    bsize = SendMessageW(hwnd, TB_GETPADDING, 0, 0);
    ok(bsize == 0x60007, "Expected 0x60007 got %lx\n", bsize);

    SendMessageW(hwnd, TB_SETPADDING, 0, 0x10001);
    SendMessageW(hwnd, TB_SETBITMAPSIZE, 0, 0x10001);
    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, 0x10001);

    bsize = SendMessageW(hwnd, TB_GETPADDING, 0, 0);
    ok(bsize == 0x10001, "Expected 0x10001 got %lx\n", bsize);

    bsize = SendMessageW(hwnd, TB_GETBUTTONSIZE, 0, 0);
    ok(bsize == 0x20002 || bsize == 0xe0002, "Expected 0x20002 got %lx\n", bsize);

#if 0 /* Luna specific */
    SetWindowTheme(hwnd, L"TaskBand", NULL);

    bsize = SendMessageW(hwnd, TB_GETBUTTONSIZE, 0, 0);
    ok(bsize == 0x20002, "Expected 0x20002 got %lx\n", bsize);

    bsize = SendMessageW(hwnd, TB_GETPADDING, 0, 0);
    ok(bsize == 0x10001, "Expected 0x10001 got %lx\n", bsize);

    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, 0x10001);

    bsize = SendMessageW(hwnd, TB_GETBUTTONSIZE, 0, 0);
    ok(bsize == 0x160006, "Expected 0x160006 got %lx\n", bsize);

    bsize = SendMessageW(hwnd, TB_GETPADDING, 0, 0);
    ok(bsize == 0x10001, "Expected 0x10001 got %lx\n", bsize);

    SendMessageW(hwnd, TB_SETPADDING, 0, 0x10001);
    SendMessageW(hwnd, TB_SETBITMAPSIZE, 0, 0x10001);
    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, 0x10001);

    bsize = SendMessageW(hwnd, TB_GETPADDING, 0, 0);
    ok(bsize == 0x10001, "Expected 0x10001 got %lx\n", bsize);

    bsize = SendMessageW(hwnd, TB_GETBUTTONSIZE, 0, 0);
    ok(bsize == 0x160006, "Expected 0x160006 got %lx\n", bsize);

    SendMessageW(hwnd, TB_SETBITMAPSIZE, 0, 0x10001);
    SendMessageW(hwnd, TB_SETPADDING, 0, 0x20002);
    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, 0x10001);

    bsize = SendMessageW(hwnd, TB_GETBUTTONSIZE, 0, 0);
    ok(bsize == 0x170007, "Expected 0x170007 got %lx\n", bsize);

    SendMessageW(hwnd, TB_SETBITMAPSIZE, 0, 0x20002);
    SendMessageW(hwnd, TB_SETPADDING, 0, 0x20002);
    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, 0x10001);

    bsize = SendMessageW(hwnd, TB_GETBUTTONSIZE, 0, 0);
    ok(bsize == 0x170008, "Expected 0x170008 got %lx\n", bsize);

    SendMessageW(hwnd, TB_SETBITMAPSIZE, 0, 0x100010);
    SendMessageW(hwnd, TB_SETPADDING, 0, 0x20002);
    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, 0x10001);

    bsize = SendMessageW(hwnd, TB_GETBUTTONSIZE, 0, 0);

    /* With a big enough image size the button size is bitmap size + pading + theme content margins */
    ok(bsize == 0x1a0016, "Expected 0x1a0016 got %lx\n", bsize);
#endif

    DestroyWindow(hwnd);
}

void TestButtonSpacing(void)
{
    HWND hwnd;
    TBMETRICS metrics;
    LRESULT lres;

    hwnd = CreateWindowExW(0, TOOLBARCLASSNAMEW, L"Test", 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed\n");

    /* TB_GETMETRICS without a valid structure returns nothing */
    ZeroMemory(&metrics, sizeof(metrics));
    lres = SendMessageW(hwnd, TB_GETMETRICS, 0, (LPARAM)&metrics);
    ok(lres == 0, "Got %lu result\n", lres);
    ok(metrics.dwMask == 0, "Got %lu\n", metrics.dwMask);
    ok(metrics.cxPad == 0, "Got %d\n", metrics.cxPad);
    /* Retry getting just the padding, while metrics.cbSize is invalid */
    metrics.dwMask = TBMF_PAD;
    lres = SendMessageW(hwnd, TB_GETMETRICS, 0, (LPARAM)&metrics);
    ok(lres == 0, "Got %lu result\n", lres);
    ok(metrics.cxPad == 0, "Got %d\n", metrics.cxPad);

    /* TB_GETMETRICS with a valid structure returns meaningful data */
    ZeroMemory(&metrics, sizeof(metrics));
    metrics.cbSize = sizeof(metrics);
    metrics.dwMask = TBMF_PAD|TBMF_BARPAD|TBMF_BUTTONSPACING;
    lres = SendMessageW(hwnd, TB_GETMETRICS, 0, (LPARAM)&metrics);
    ok(lres == 0, "Got %lu result\n", lres);
    ok(metrics.dwMask == (TBMF_PAD|TBMF_BARPAD|TBMF_BUTTONSPACING), "Got %lu\n", metrics.dwMask);
    ok(metrics.cxPad == 7, "Got %d\n", metrics.cxPad);
    ok(metrics.cyPad == 6, "Got %d\n", metrics.cyPad);
    ok(metrics.cxButtonSpacing == 0, "Got %d\n", metrics.cxButtonSpacing);
    ok(metrics.cyButtonSpacing == 0, "Got %d\n", metrics.cyButtonSpacing);

    /* TB_SETMETRICS without a valid structure sets nothing */
    FillMemory(&metrics, sizeof(metrics), 0xDEADBEEF);
    metrics.cbSize = 0; // Ensure the structure is invalidated
    metrics.dwMask = TBMF_PAD|TBMF_BARPAD|TBMF_BUTTONSPACING; // Claim we set data
    lres = SendMessageW(hwnd, TB_SETMETRICS, 0, (LPARAM)&metrics);
    ok(lres == 0, "Got %lu result\n", lres);

    /* The previous TB_SETMETRICS call should have not modified anything */
    ZeroMemory(&metrics, sizeof(metrics));
    metrics.cbSize = sizeof(metrics);
    metrics.dwMask = TBMF_PAD|TBMF_BARPAD|TBMF_BUTTONSPACING;
    lres = SendMessageW(hwnd, TB_GETMETRICS, 0, (LPARAM)&metrics);
    ok(lres == 0, "Got %lu result\n", lres);
    ok(metrics.dwMask == (TBMF_PAD|TBMF_BARPAD|TBMF_BUTTONSPACING), "Got %lu\n", metrics.dwMask);
    ok(metrics.cxPad == 7, "Got %d\n", metrics.cxPad);
    ok(metrics.cyPad == 6, "Got %d\n", metrics.cyPad);
    ok(metrics.cxButtonSpacing == 0, "Got %d\n", metrics.cxButtonSpacing);
    ok(metrics.cyButtonSpacing == 0, "Got %d\n", metrics.cyButtonSpacing);

    /* TB_SETMETRICS with a valid structure sets the data */
    ZeroMemory(&metrics, sizeof(metrics));
    metrics.cbSize = sizeof(metrics);
    metrics.dwMask = TBMF_PAD|TBMF_BARPAD|TBMF_BUTTONSPACING;
    metrics.cxPad = metrics.cyPad = 2;
    metrics.cxBarPad = metrics.cyBarPad = 3;
    metrics.cxButtonSpacing = metrics.cyButtonSpacing = 5;
    lres = SendMessageW(hwnd, TB_SETMETRICS, 0, (LPARAM)&metrics);
    ok(lres == 0, "Got %lu result\n", lres);

    /* The previous TB_SETMETRICS call should have modified the metrics */
    ZeroMemory(&metrics, sizeof(metrics));
    metrics.cbSize = sizeof(metrics);
    metrics.dwMask = TBMF_PAD|TBMF_BARPAD|TBMF_BUTTONSPACING;
    lres = SendMessageW(hwnd, TB_GETMETRICS, 0, (LPARAM)&metrics);
    ok(lres == 0, "Got %lu result\n", lres);
    ok(metrics.dwMask == (TBMF_PAD|TBMF_BARPAD|TBMF_BUTTONSPACING), "Got %lu\n", metrics.dwMask);
    ok(metrics.cxPad == 2, "Got %d\n", metrics.cxPad);
    ok(metrics.cyPad == 2, "Got %d\n", metrics.cyPad);
    ok(metrics.cxBarPad == 3, "Got %d\n", metrics.cxBarPad);
    ok(metrics.cyBarPad == 3, "Got %d\n", metrics.cyBarPad);
    ok(metrics.cxButtonSpacing == 5, "Got %d\n", metrics.cxButtonSpacing);
    ok(metrics.cyButtonSpacing == 5, "Got %d\n", metrics.cyButtonSpacing);

    DestroyWindow(hwnd);
}

void TestV5VersionMessage(void)
{
    HWND hwnd;
    int version;

    hwnd = CreateWindowExW(0, TOOLBARCLASSNAMEW, L"Test", 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed\n");

    version = SendMessageW(hwnd, CCM_GETVERSION, 0, 0);
    ok(version == 0, "Got %d, expected 0\n", version);

    version = SendMessageW(hwnd, CCM_SETVERSION, 6, 0);
    ok(version == -1, "Got %d, expected -1\n", version);

    version = SendMessageW(hwnd, CCM_SETVERSION, 7, 0);
    ok(version == -1, "Got %d, expected -1\n", version);

    version = SendMessageW(hwnd, CCM_SETVERSION, 5, 0);
    ok(version == 0, "Got %d, expected -1\n", version);

    version = SendMessageW(hwnd, CCM_GETVERSION, 0, 0);
    ok(version == 5, "Got %d, expected 5\n", version);

    version = SendMessageW(hwnd, CCM_SETVERSION, 4, 0);
    ok(version == 5, "Got %d, expected -1\n", version);

    version = SendMessageW(hwnd, CCM_GETVERSION, 0, 0);
    ok(version == 4, "Got %d, expected 5\n", version);

    version = SendMessageW(hwnd, CCM_SETVERSION, 3, 0);
    ok(version == 4, "Got %d, expected -1\n", version);

    version = SendMessageW(hwnd, CCM_GETVERSION, 0, 0);
    ok(version == 3, "Got %d, expected 5\n", version);

    DestroyWindow(hwnd);
}

START_TEST(toolbarv6)
{
    HANDLE hV5ActCtx;

    LoadLibraryW(L"comctl32.dll");

    TestVersionMessage();
    TestSetButtonSize();
    TestPadding();
    TestButtonSpacing();

    hV5ActCtx = _CreateV5ActCtx();
    ok (hV5ActCtx != INVALID_HANDLE_VALUE, "CreateActCtxW failed\n");
    if (hV5ActCtx)
    {
        ULONG_PTR cookie;
        BOOL bActivated = ActivateActCtx(hV5ActCtx, &cookie);
        if (bActivated)
        {
            TestV5VersionMessage();
            DeactivateActCtx(0, cookie);
        }
    }
}
