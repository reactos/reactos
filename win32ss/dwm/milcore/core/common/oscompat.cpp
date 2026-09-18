// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        Contains logic for running on several OS versions
//

#include "precomp.hpp"

typedef BOOL (WINAPI* UpdateLayeredWindowIndirectPtr)(
    __in HWND hWnd,
    __in const UPDATELAYEREDWINDOWINFO* pULWInfo);

static volatile UpdateLayeredWindowIndirectPtr s_pfnUpdateLayeredWindowIndirect = NULL;
static HRESULT volatile s_hrULWFound = WGXERR_NOTINITIALIZED;

//+----------------------------------------------------------------------------
//
//  Function:
//      LoadUpdateLayeredWindowIndirectProc
//
//  Synopsis:
//      Attempt to get proc address for UpdateLayeredWindowIndirect
//
//  Notes:
//      Results are deterministic and stored thread safely in statics.
//
//-----------------------------------------------------------------------------

void
LoadUpdateLayeredWindowIndirectProc(
    )
{
    HMODULE hUser32 = TW32(0, GetModuleHandle(_T("user32.dll")));
    UpdateLayeredWindowIndirectPtr pfnUpdateLayeredWindowIndirect = NULL;

    if (hUser32)
    {
        pfnUpdateLayeredWindowIndirect =
        s_pfnUpdateLayeredWindowIndirect =
            reinterpret_cast<UpdateLayeredWindowIndirectPtr>TW32(0, GetProcAddress(
                hUser32,
                "UpdateLayeredWindowIndirect"
                ));

        // Make sure result is written before result status is set.
        MemoryBarrier();
    }
    else
    {
        // No need to update s_pfnUpdateLayeredWindowIndirect when user32
        // wasn't available.
    }

    s_hrULWFound = (pfnUpdateLayeredWindowIndirect) ?
        S_OK :
        // GetProcAddress could set ERROR_PROC_NOT_FOUND or GetModuleHandle
        // may set ERROR_MOD_NOT_FOUND.  Since our callers may rely on
        // ERROR_PROC_NOT_FOUND result return it explicity.
        HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}


//+----------------------------------------------------------------------------
//
//  Function:
//      OSSupportsUpdateLayeredWindowIndirect
//
//  Synopsis:
//      Return true if OS supports UpdateLayeredWindowIndirect
//
//-----------------------------------------------------------------------------

bool
OSSupportsUpdateLayeredWindowIndirect(
    )
{
    if (s_hrULWFound == WGXERR_NOTINITIALIZED)
    {
        LoadUpdateLayeredWindowIndirectProc();
    }

    return SUCCEEDED(s_hrULWFound);
}


//+----------------------------------------------------------------------------
//
//  Function:  UpdateLayeredWindowEx
//
//  Synopsis:  Call UpdateLayeredWindow or UpdateLayeredWindowIndirect as
//             required by parameters.  If UpdateLayeredWindowIndirect is
//             needed (ULW_EX_NORESIZE requested), but not available return
//             HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND).  prcDirty is ignored
//             when UpdateLayeredWindowIndirect is not available.
//
//-----------------------------------------------------------------------------
HRESULT
UpdateLayeredWindowEx(
    __in HWND hWnd,
    __in_opt HDC hdcDst,
    __in_ecount_opt(1) CONST POINT *pptDst,
    __in_ecount_opt(1) CONST SIZE *psize,
    __in_opt HDC hdcSrc,
    __in_ecount_opt(1) CONST POINT *pptSrc,
    COLORREF crKey,
    __in_ecount_opt(1)CONST BLENDFUNCTION *pblend,
    DWORD dwFlags,
    __in_ecount_opt(1) CONST RECT *prcDirty
    )
{
    HRESULT hr = S_OK;
    wpf::util::DpiAwarenessScope<HWND> dpiScope(hWnd);

    if (s_hrULWFound == WGXERR_NOTINITIALIZED)
    {
        LoadUpdateLayeredWindowIndirectProc();
    }

    //
    // Use UpdateLayeredWindowIndirect when ever it is present
    //
    POINT xy = {};
    const POINT *pXY = pptDst;
    if (pptDst && (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD))
    {
        // UpdateLayeredWindowIndirect expects parent's coordinates for child windows
        HWND parentHwnd = GetParent(hWnd);
        if (parentHwnd)
        {
            xy = *pptDst;
            IFCW32(ScreenToClient(parentHwnd, &xy));
            pXY = &xy;
        }
    }

    if (SUCCEEDED(s_hrULWFound))
    {
        UPDATELAYEREDWINDOWINFO ulwi;

        ulwi.cbSize = sizeof(ulwi);
        ulwi.hdcDst = hdcDst;
        ulwi.pptDst = pXY;
        ulwi.psize = psize;
        ulwi.hdcSrc = hdcSrc;
        ulwi.pptSrc = pptSrc;
        ulwi.crKey = crKey;
        ulwi.pblend = pblend;
        ulwi.dwFlags = dwFlags;
        ulwi.prcDirty = prcDirty;

        #pragma prefast(suppress: __WARNING_DEREF_NULL_PTR, "GetProcAddress is stable so s_pfnUpdateLayeredWindowIndirect will not be NULL if s_hrULWFound is S_OK.") 
        IFCW32(s_pfnUpdateLayeredWindowIndirect(
            hWnd,
            &ulwi
            ));
    }
    else
    {
        //
        // Fallback to UpdateLayeredWindow
        //
        // If prcDirty is specified it will be ignored.
        //
        // If ULW_EX_NORESIZE is used we must use UpdateLayeredWindowIndirect
        // to avoid threading issues with resizing; so fail.
        //

        if (dwFlags & ULW_EX_NORESIZE)
        {
            IFC(s_hrULWFound);
        }

        //
        // Note: ULW shouldn't modify pptDst, psize, pptSrc, or pblend, but
        //  make a local copy to respect the prototype specification anyway.
        //

        #pragma warning( push )
        #pragma warning( disable : 4238 ) // class rvalue used as lvalue
        IFCW32(UpdateLayeredWindow(
            hWnd,
            hdcDst,
            (pXY) ? &POINT(*pXY) : NULL,
            (psize) ? &SIZE(*psize) : NULL,
            hdcSrc,
            (pptSrc) ? &POINT(*pptSrc) : NULL,
            crKey,
            (pblend) ? &BLENDFUNCTION(*pblend) : NULL,
            dwFlags
            ));
        #pragma warning( pop )
    }

Cleanup:
    
    if ((hr == WGXERR_WIN32ERROR || hr == HRESULT_FROM_WIN32(ERROR_MR_MID_NOT_FOUND)) && IsWindow(hWnd))
    {
        // If the window we are presenting to is still legitimate, and
        // the error is just a generic win32 error, we expect that this
        // is just ULW complaining about the device behind our DC in a
        // multimon scenario.  In this case, GDI will update the
        // matching meta sprite surface, but leave any other untouched.
        // This is a nice perf benefit on XP SP2, which doesn't have
        // dirty rect support.
        // IMPORTANT: The window contents may be stale until another present is
        // triggered.
        // We make this check in both hardware and software rendering, which
        // fixes bugs DevDiv 585995 and 582643.
        //
        // If the error is ERROR_MR_MID_NOT_FOUND we will also ignore it.  A Win7
        // regression causes this error message (which indicates that no suitable 
        // error message was found to return) to be returned.  Previously on down-
        // level OSes no error code was set, which we would safely ignore.
        hr = S_OK;
    }
    
    // If another process is calling PrintWindow on the hwnd, it will temporarily
    // set a redirection bitmap on the window.  A call to UpdateLayerWindow when
    // a redirection bitmap is set will return the following error, which should
    // not bring down the WPF app.  We return an error to signal that this is
    // recoverable and we should attempt to Present again.
    if (hr == E_INVALIDARG)
    {
        hr = WGXERR_NEED_REATTEMPT_PRESENT;
    }
        
    RRETURN(hr);
}


class CDisableWow64FsRedirectionHelper
{
    //
    // Illegal allocation operators
    //
    //   These are declared, but not defined such that any use that gets around
    //   the private protection will generate a link time error.
    //

    __allocator __out_bcount(cb) void * operator new(size_t cb);
    __allocator __out_bcount(cb) void * operator new[](size_t cb);
    __out_bcount(cb) void * operator new(size_t cb, __out_bcount(cb) void * pv);
    
    typedef BOOLEAN (WINAPI FAR* LPWOW64DISABLEWOW64FSREDIRECTION)( PVOID * Wow64FsDisableRedirection );
    typedef BOOLEAN (WINAPI FAR* LPWOW64REVERTWOW64FSREDIRECTION)( PVOID Wow64FsRevertRedirection );
    typedef BOOLEAN (WINAPI FAR* LPISWOW64PROCESS)( HANDLE hProcess, PBOOL bIsWow64Process );
    
    volatile LPWOW64DISABLEWOW64FSREDIRECTION m_pfnWow64DisableWow64FsRedirection;
    volatile LPWOW64REVERTWOW64FSREDIRECTION m_pfnWow64RevertWow64FsRedirection;

    volatile bool m_fInitialized;

public:
    CDisableWow64FsRedirectionHelper()
        : m_pfnWow64DisableWow64FsRedirection(NULL),
          m_pfnWow64RevertWow64FsRedirection(NULL),
          m_fInitialized(false)
    {
    }
        
    HRESULT Disable(PVOID *ppOldValue)
    {
        HRESULT hr = S_OK;
        
        if (!m_fInitialized)
        {
            HINSTANCE hInstKernel32 = NULL;
            BOOL bIsWow64 = FALSE;
            
            IFCW32(hInstKernel32 = GetModuleHandle(TEXT("kernel32.dll")));

            LPISWOW64PROCESS pfnIsWow64Process
                = (LPISWOW64PROCESS)
                GetProcAddress(hInstKernel32, "IsWow64Process");

            if (pfnIsWow64Process)
            {
                IFCW32(pfnIsWow64Process(GetCurrentProcess(),&bIsWow64));

                if (bIsWow64)
                {
                    m_pfnWow64DisableWow64FsRedirection
                        = (LPWOW64DISABLEWOW64FSREDIRECTION)
                        GetProcAddress(hInstKernel32, "Wow64DisableWow64FsRedirection");
                    m_pfnWow64RevertWow64FsRedirection
                        = (LPWOW64REVERTWOW64FSREDIRECTION)
                        GetProcAddress(hInstKernel32, "Wow64RevertWow64FsRedirection");

                    Assert(!m_pfnWow64DisableWow64FsRedirection == !m_pfnWow64RevertWow64FsRedirection);
                }
            }

            // Make sure result is written before result status is set.
            MemoryBarrier();
            
            m_fInitialized = true;
        }

        if (m_pfnWow64DisableWow64FsRedirection)
        {
            IFCW32(m_pfnWow64DisableWow64FsRedirection(ppOldValue));
        }
        
      Cleanup:
        RRETURN(hr);
    }
            
    HRESULT Revert(PVOID pOldValue)
    {
        HRESULT hr = S_OK;
        
        if (m_pfnWow64RevertWow64FsRedirection)
        {
            IFCW32(m_pfnWow64RevertWow64FsRedirection(pOldValue));
        }

      Cleanup:
        RRETURN(hr);
    }
};

static CDisableWow64FsRedirectionHelper s_DisableHelper;


//+------------------------------------------------------------------------
//
//  Function:   CDisableWow64FsRedirection::DisableRedirection
//
//  Synopsis:   Disables Wow64 redirection for this thread until the dtor
//              is called.
//
//              Ignores errors.
//
//-------------------------------------------------------------------------
CDisableWow64FsRedirection::CDisableWow64FsRedirection()
{
    m_hr = s_DisableHelper.Disable(&m_pOldValue);
}


//+------------------------------------------------------------------------
//
//  Function:   CDisableWow64FsRedirection::~CDisableWow64FsRedirection
//
//  Synopsis:   Dtor
//
//-------------------------------------------------------------------------
CDisableWow64FsRedirection::~CDisableWow64FsRedirection()
{
    if (SUCCEEDED(m_hr))
    {
        IGNORE_HR(s_DisableHelper.Revert(m_pOldValue));
    }
}




