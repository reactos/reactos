/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for Raw Accelerators
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shlwapi_undoc.h>

typedef PRAWACCEL (WINAPI *FN_SHLoadRawAccelerators)(HINSTANCE, PCSTR);
typedef BOOL (WINAPI *FN_SHQueryRawAccelerator)(const RAWACCEL*, BYTE, BYTE, UINT, PUINT);
typedef BOOL (WINAPI *FN_SHQueryRawAcceleratorMsg)(const RAWACCEL*, const MSG*, PUINT);

static FN_SHLoadRawAccelerators g_fnSHLoadRawAccelerators = NULL;
static FN_SHQueryRawAccelerator g_fnSHQueryRawAccelerator = NULL;
static FN_SHQueryRawAcceleratorMsg g_fnSHQueryRawAcceleratorMsg = NULL;

START_TEST(RawAccelerators)
{
    HMODULE hSHLWAPI = GetModuleHandleA("shlwapi");
    g_fnSHLoadRawAccelerators = (FN_SHLoadRawAccelerators)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(385));
    g_fnSHQueryRawAccelerator = (FN_SHQueryRawAccelerator)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(386));
    g_fnSHQueryRawAcceleratorMsg = (FN_SHQueryRawAcceleratorMsg)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(387));
    if (!g_fnSHLoadRawAccelerators ||
        !g_fnSHQueryRawAccelerator ||
        !g_fnSHQueryRawAcceleratorMsg)
    {
        skip("Some functions not found (%p, %p, %p)\n", g_fnSHLoadRawAccelerators, g_fnSHQueryRawAccelerator,
             g_fnSHQueryRawAcceleratorMsg);
        return;
    }

    PRAWACCEL pRawAccels = g_fnSHLoadRawAccelerators(GetModuleHandleA(NULL), MAKEINTRESOURCEA(1));
    ok(pRawAccels != NULL, "SHLoadRawAccelerators failed\n");

    if (!pRawAccels)
    {
        skip("pRawAccels was null\n");
        return;
    }

    UINT nCmd = 0xDEAD;
    BYTE fVirt = FVIRTKEY;
    BOOL ret = g_fnSHQueryRawAccelerator(pRawAccels, fVirt, fVirt, L'A', &nCmd);
    ok_int(ret, TRUE);
    ok_int(nCmd, 100);

    nCmd = 0xDEAD;
    fVirt = FVIRTKEY;
    ret = g_fnSHQueryRawAccelerator(pRawAccels, fVirt, fVirt, L'B', &nCmd);
    ok_int(ret, FALSE);
    ok_int(nCmd, 0);

    nCmd = 0xDEAD;
    fVirt = FVIRTKEY | FSHIFT;
    ret = g_fnSHQueryRawAccelerator(pRawAccels, fVirt, fVirt, VK_RETURN, &nCmd);
    ok_int(ret, TRUE);
    ok_int(nCmd, 101);

    nCmd = 0xDEAD;
    fVirt = FVIRTKEY | FCONTROL;
    ret = g_fnSHQueryRawAccelerator(pRawAccels, fVirt, fVirt, VK_SPACE, &nCmd);
    ok_int(ret, TRUE);
    ok_int(nCmd, 102);

    fVirt = FVIRTKEY | FCONTROL;
    ret = g_fnSHQueryRawAccelerator(pRawAccels, fVirt, fVirt, VK_SPACE, NULL);
    ok_int(ret, TRUE);

    MSG msg = { NULL };
    msg.message = WM_KEYDOWN;
    msg.wParam = L'A';
    nCmd = 0xDEAD;
    ret = g_fnSHQueryRawAcceleratorMsg(pRawAccels, &msg, &nCmd);
    ok_int(ret, TRUE);
    ok_int(nCmd, 100);
}
