/*
 *  @doc    INTERNAL
 *
 *  @module DIMM.CXX -- Handlers for Active IMM (component formerly known as Dynamic IMM)
 *
 *
 *  Owner: <nl>
 *      Ben Westbrook <nl>
 *      Chris Thrasher <nl>
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#ifndef NO_IME

#ifndef X_IMM_H_
#define X_IMM_H_
#include "imm.h"
#endif

#ifndef X_DIMM_H_
#define X_DIMM_H_
#include "dimm.h"
#endif

static IActiveIMMApp * s_pActiveIMM;
// BUGBUG Presumably this will live in a header file.
const IID IID_IActiveIMMAppPostNT4 = {0xc839a84c, 0x8036, 0x11d3, {0x92, 0x70, 0x00, 0x60, 0xb0, 0x67, 0xb8, 0x6e}  };

BOOL HasActiveIMM() { return s_pActiveIMM != NULL; }
IActiveIMMApp * GetActiveIMM() { return s_pActiveIMM; }

// returns TRUE iff plResult is set in lieu of call to DefWindowProc
BOOL DIMMHandleDefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    if (HasActiveIMM())
    {
        if (GetActiveIMM()->OnDefWindowProc(hWnd, Msg, wParam, lParam, plResult) == S_OK)
        {
            return TRUE;
        }
    }

    return FALSE;
}

HRESULT EnsureLoadedDIMM()
{
    static BOOL fFailedCoCreate = FALSE;
    HRESULT hr;

    if (HasActiveIMM())
        return S_OK;

    // since the DIMM typically won't be installed on a system,
    // try to avoid constant calls to CoCreate
    if (fFailedCoCreate)
        return E_FAIL;

    LOCK_GLOBALS;

    // Need to check again after locking globals.
    if (HasActiveIMM())
        return S_OK;

    if (   g_dwPlatformID != VER_PLATFORM_WIN32_NT
        || g_dwPlatformVersion < 0x00050000)
    {
        hr = CoCreateInstance(CLSID_CActiveIMM, NULL, CLSCTX_INPROC_SERVER,
                              IID_IActiveIMMApp, (void**)&s_pActiveIMM);
    }
    else
    {
        hr = CoCreateInstance(CLSID_CActiveIMM, NULL, CLSCTX_INPROC_SERVER,
                              IID_IActiveIMMAppPostNT4, (void**)&s_pActiveIMM);
    }

    fFailedCoCreate = FAILED(hr);

    return hr;
}

HRESULT ActivateDIMM()
{
    if (FAILED(EnsureLoadedDIMM()))
        return E_FAIL;

    return GetActiveIMM()->Activate(TRUE);
}

HRESULT DeactivateDIMM()
{
    if (HasActiveIMM())
    {
        // BUGBUG: assuming here the correct thread is matching an original Begin() call.
        // Could add some debug code to tls to try to catch this....thinking not worth
        // the effort currently. (benwest)

        // Consider adding a cookie (threadid) to the interface if this becomes an issue?

        return GetActiveIMM()->Deactivate();
    }

    return E_FAIL;
}

HRESULT FilterClientWindowsDIMM(ATOM *aaWindowClasses, UINT uSize)
{
    if (FAILED(EnsureLoadedDIMM()))
    {
        return E_FAIL;
    }

    return GetActiveIMM()->FilterClientWindows(aaWindowClasses, uSize);
}

// Called during CServer shutdown, globals are already locked
void DeinitDIMM()
{
    ClearInterface(&s_pActiveIMM);
}

#endif // NO_IME
