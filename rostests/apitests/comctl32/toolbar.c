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

HANDLE _CreateV5ActCtx()
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


void TestVersionMessage()
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

void TestV5VersionMessage()
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

START_TEST(toolbar)
{
    HANDLE hV5ActCtx;

    LoadLibraryW(L"comctl32.dll");

    TestVersionMessage();

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