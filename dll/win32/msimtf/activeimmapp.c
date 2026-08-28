/*
 *  ActiveIMMApp Interface
 *
 *  Copyright 2008  CodeWeavers, Aric Stewart
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winuser.h"
#include "winerror.h"
#include "objbase.h"
#include "dimm.h"
#include "imm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msimtf);

typedef struct tagActiveIMMApp {
    IActiveIMMApp IActiveIMMApp_iface;
    IActiveIMMMessagePumpOwner IActiveIMMMessagePumpOwner_iface;
    LONG refCount;
} ActiveIMMApp;

static inline ActiveIMMApp *impl_from_IActiveIMMApp(IActiveIMMApp *iface)
{
    return CONTAINING_RECORD(iface, ActiveIMMApp, IActiveIMMApp_iface);
}

static void ActiveIMMApp_Destructor(ActiveIMMApp* This)
{
    TRACE("\n");
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI ActiveIMMApp_QueryInterface (IActiveIMMApp* iface,
        REFIID iid, LPVOID *ppvOut)
{
    ActiveIMMApp *This = impl_from_IActiveIMMApp(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IActiveIMMApp))
    {
        *ppvOut = &This->IActiveIMMApp_iface;
    }
    else if (IsEqualIID(iid, &IID_IActiveIMMMessagePumpOwner))
    {
        *ppvOut = &This->IActiveIMMMessagePumpOwner_iface;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveIMMApp_AddRef(IActiveIMMApp* iface)
{
    ActiveIMMApp *This = impl_from_IActiveIMMApp(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI ActiveIMMApp_Release(IActiveIMMApp* iface)
{
    ActiveIMMApp *This = impl_from_IActiveIMMApp(iface);
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        ActiveIMMApp_Destructor(This);
    return ret;
}

static HRESULT WINAPI ActiveIMMApp_AssociateContext(IActiveIMMApp* iface,
        HWND hWnd, HIMC hIME, HIMC *phPrev)
{
    *phPrev = ImmAssociateContext(hWnd,hIME);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_ConfigureIMEA(IActiveIMMApp* This,
        HKL hKL, HWND hwnd, DWORD dwMode, REGISTERWORDA *pData)
{
    BOOL rc;

    rc = ImmConfigureIMEA(hKL, hwnd, dwMode, pData);
    if (rc)
        return E_FAIL;
    else
        return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_ConfigureIMEW(IActiveIMMApp* This,
        HKL hKL, HWND hWnd, DWORD dwMode, REGISTERWORDW *pData)
{
    BOOL rc;

    rc = ImmConfigureIMEW(hKL, hWnd, dwMode, pData);
    if (rc)
        return E_FAIL;
    else
        return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_CreateContext(IActiveIMMApp* This,
        HIMC *phIMC)
{
    *phIMC = ImmCreateContext();
    if (*phIMC)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_DestroyContext(IActiveIMMApp* This,
        HIMC hIME)
{
    BOOL rc;

    rc = ImmDestroyContext(hIME);
    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_EnumRegisterWordA(IActiveIMMApp* This,
        HKL hKL, LPSTR szReading, DWORD dwStyle, LPSTR szRegister,
        LPVOID pData, IEnumRegisterWordA **pEnum)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveIMMApp_EnumRegisterWordW(IActiveIMMApp* This,
        HKL hKL, LPWSTR szReading, DWORD dwStyle, LPWSTR szRegister,
        LPVOID pData, IEnumRegisterWordW **pEnum)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveIMMApp_EscapeA(IActiveIMMApp* This,
        HKL hKL, HIMC hIMC, UINT uEscape, LPVOID pData, LRESULT *plResult)
{
    *plResult = ImmEscapeA(hKL, hIMC, uEscape, pData);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_EscapeW(IActiveIMMApp* This,
        HKL hKL, HIMC hIMC, UINT uEscape, LPVOID pData, LRESULT *plResult)
{
    *plResult = ImmEscapeW(hKL, hIMC, uEscape, pData);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetCandidateListA(IActiveIMMApp* This,
        HIMC hIMC, DWORD dwIndex, UINT uBufLen, CANDIDATELIST *pCandList,
        UINT *puCopied)
{
    *puCopied = ImmGetCandidateListA(hIMC, dwIndex, pCandList, uBufLen);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetCandidateListW(IActiveIMMApp* This,
        HIMC hIMC, DWORD dwIndex, UINT uBufLen, CANDIDATELIST *pCandList,
        UINT *puCopied)
{
    *puCopied = ImmGetCandidateListW(hIMC, dwIndex, pCandList, uBufLen);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetCandidateListCountA(IActiveIMMApp* This,
        HIMC hIMC, DWORD *pdwListSize, DWORD *pdwBufLen)
{
   *pdwBufLen = ImmGetCandidateListCountA(hIMC, pdwListSize);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetCandidateListCountW(IActiveIMMApp* This,
        HIMC hIMC, DWORD *pdwListSize, DWORD *pdwBufLen)
{
   *pdwBufLen = ImmGetCandidateListCountA(hIMC, pdwListSize);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetCandidateWindow(IActiveIMMApp* This,
        HIMC hIMC, DWORD dwIndex, CANDIDATEFORM *pCandidate)
{
    BOOL rc;
    rc = ImmGetCandidateWindow(hIMC,dwIndex,pCandidate);
    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_GetCompositionFontA(IActiveIMMApp* This,
        HIMC hIMC, LOGFONTA *plf)
{
    BOOL rc;
    rc = ImmGetCompositionFontA(hIMC,plf);
    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_GetCompositionFontW(IActiveIMMApp* This,
        HIMC hIMC, LOGFONTW *plf)
{
    BOOL rc;
    rc = ImmGetCompositionFontW(hIMC,plf);
    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_GetCompositionStringA(IActiveIMMApp* This,
        HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LONG *plCopied, LPVOID pBuf)
{
    *plCopied = ImmGetCompositionStringA(hIMC, dwIndex, pBuf, dwBufLen);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetCompositionStringW(IActiveIMMApp* This,
        HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LONG *plCopied, LPVOID pBuf)
{
    *plCopied = ImmGetCompositionStringW(hIMC, dwIndex, pBuf, dwBufLen);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetCompositionWindow(IActiveIMMApp* This,
        HIMC hIMC, COMPOSITIONFORM *pCompForm)
{
    BOOL rc;

    rc = ImmGetCompositionWindow(hIMC,pCompForm);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_GetContext(IActiveIMMApp* This,
        HWND hwnd, HIMC *phIMC)
{
    *phIMC = ImmGetContext(hwnd);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetConversionListA(IActiveIMMApp* This,
        HKL hKL, HIMC hIMC, LPSTR pSrc, UINT uBufLen, UINT uFlag,
        CANDIDATELIST *pDst, UINT *puCopied)
{
    *puCopied = ImmGetConversionListA(hKL, hIMC, pSrc, pDst, uBufLen, uFlag);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetConversionListW(IActiveIMMApp* This,
        HKL hKL, HIMC hIMC, LPWSTR pSrc, UINT uBufLen, UINT uFlag,
        CANDIDATELIST *pDst, UINT *puCopied)
{
    *puCopied = ImmGetConversionListW(hKL, hIMC, pSrc, pDst, uBufLen, uFlag);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetConversionStatus(IActiveIMMApp* This,
        HIMC hIMC, DWORD *pfdwConversion, DWORD *pfdwSentence)
{
    BOOL rc;

    rc = ImmGetConversionStatus(hIMC, pfdwConversion, pfdwSentence);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_GetDefaultIMEWnd(IActiveIMMApp* This,
        HWND hWnd, HWND *phDefWnd)
{
    *phDefWnd = ImmGetDefaultIMEWnd(hWnd);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetDescriptionA(IActiveIMMApp* This,
        HKL hKL, UINT uBufLen, LPSTR szDescription, UINT *puCopied)
{
    *puCopied = ImmGetDescriptionA(hKL, szDescription, uBufLen);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetDescriptionW(IActiveIMMApp* This,
        HKL hKL, UINT uBufLen, LPWSTR szDescription, UINT *puCopied)
{
    *puCopied = ImmGetDescriptionW(hKL, szDescription, uBufLen);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetGuideLineA(IActiveIMMApp* This,
        HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LPSTR pBuf,
        DWORD *pdwResult)
{
    *pdwResult = ImmGetGuideLineA(hIMC, dwIndex, pBuf, dwBufLen);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetGuideLineW(IActiveIMMApp* This,
        HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LPWSTR pBuf,
        DWORD *pdwResult)
{
    *pdwResult = ImmGetGuideLineW(hIMC, dwIndex, pBuf, dwBufLen);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetIMEFileNameA(IActiveIMMApp* This,
        HKL hKL, UINT uBufLen, LPSTR szFileName, UINT *puCopied)
{
    *puCopied = ImmGetIMEFileNameA(hKL, szFileName, uBufLen);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetIMEFileNameW(IActiveIMMApp* This,
        HKL hKL, UINT uBufLen, LPWSTR szFileName, UINT *puCopied)
{
    *puCopied = ImmGetIMEFileNameW(hKL, szFileName, uBufLen);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetOpenStatus(IActiveIMMApp* This,
        HIMC hIMC)
{
    return ImmGetOpenStatus(hIMC);
}

static HRESULT WINAPI ActiveIMMApp_GetProperty(IActiveIMMApp* This,
        HKL hKL, DWORD fdwIndex, DWORD *pdwProperty)
{
    *pdwProperty = ImmGetProperty(hKL, fdwIndex);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetRegisterWordStyleA(IActiveIMMApp* This,
        HKL hKL, UINT nItem, STYLEBUFA *pStyleBuf, UINT *puCopied)
{
    *puCopied = ImmGetRegisterWordStyleA(hKL, nItem, pStyleBuf);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetRegisterWordStyleW(IActiveIMMApp* This,
        HKL hKL, UINT nItem, STYLEBUFW *pStyleBuf, UINT *puCopied)
{
    *puCopied = ImmGetRegisterWordStyleW(hKL, nItem, pStyleBuf);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetStatusWindowPos(IActiveIMMApp* This,
        HIMC hIMC, POINT *pptPos)
{
    BOOL rc;
    rc = ImmGetStatusWindowPos(hIMC, pptPos);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_GetVirtualKey(IActiveIMMApp* This,
        HWND hWnd, UINT *puVirtualKey)
{
    *puVirtualKey = ImmGetVirtualKey(hWnd);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_InstallIMEA(IActiveIMMApp* This,
        LPSTR szIMEFileName, LPSTR szLayoutText, HKL *phKL)
{
    *phKL = ImmInstallIMEA(szIMEFileName,szLayoutText);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_InstallIMEW(IActiveIMMApp* This,
        LPWSTR szIMEFileName, LPWSTR szLayoutText, HKL *phKL)
{
    *phKL = ImmInstallIMEW(szIMEFileName,szLayoutText);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_IsIME(IActiveIMMApp* This,
        HKL hKL)
{
    return ImmIsIME(hKL);
}

static HRESULT WINAPI ActiveIMMApp_IsUIMessageA(IActiveIMMApp* This,
        HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return ImmIsUIMessageA(hWndIME,msg,wParam,lParam);
}

static HRESULT WINAPI ActiveIMMApp_IsUIMessageW(IActiveIMMApp* This,
        HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return ImmIsUIMessageW(hWndIME,msg,wParam,lParam);
}

static HRESULT WINAPI ActiveIMMApp_NotifyIME(IActiveIMMApp* This,
        HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
    BOOL rc;

    rc = ImmNotifyIME(hIMC,dwAction,dwIndex,dwValue);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_RegisterWordA(IActiveIMMApp* This,
        HKL hKL, LPSTR szReading, DWORD dwStyle, LPSTR szRegister)
{
    BOOL rc;

    rc = ImmRegisterWordA(hKL,szReading,dwStyle,szRegister);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_RegisterWordW(IActiveIMMApp* This,
        HKL hKL, LPWSTR szReading, DWORD dwStyle, LPWSTR szRegister)
{
    BOOL rc;

    rc = ImmRegisterWordW(hKL,szReading,dwStyle,szRegister);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_ReleaseContext(IActiveIMMApp* This,
        HWND hWnd, HIMC hIMC)
{
    BOOL rc;

    rc = ImmReleaseContext(hWnd,hIMC);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_SetCandidateWindow(IActiveIMMApp* This,
        HIMC hIMC, CANDIDATEFORM *pCandidate)
{
    BOOL rc;

    rc = ImmSetCandidateWindow(hIMC,pCandidate);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_SetCompositionFontA(IActiveIMMApp* This,
        HIMC hIMC, LOGFONTA *plf)
{
    BOOL rc;

    rc = ImmSetCompositionFontA(hIMC,plf);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_SetCompositionFontW(IActiveIMMApp* This,
        HIMC hIMC, LOGFONTW *plf)
{
    BOOL rc;

    rc = ImmSetCompositionFontW(hIMC,plf);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_SetCompositionStringA(IActiveIMMApp* This,
        HIMC hIMC, DWORD dwIndex, LPVOID pComp, DWORD dwCompLen,
        LPVOID pRead, DWORD dwReadLen)
{
    BOOL rc;

    rc = ImmSetCompositionStringA(hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_SetCompositionStringW(IActiveIMMApp* This,
        HIMC hIMC, DWORD dwIndex, LPVOID pComp, DWORD dwCompLen,
        LPVOID pRead, DWORD dwReadLen)
{
    BOOL rc;

    rc = ImmSetCompositionStringW(hIMC,dwIndex,pComp,dwCompLen,pRead,dwReadLen);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_SetCompositionWindow(IActiveIMMApp* This,
        HIMC hIMC, COMPOSITIONFORM *pCompForm)
{
    BOOL rc;

    rc = ImmSetCompositionWindow(hIMC,pCompForm);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_SetConversionStatus(IActiveIMMApp* This,
        HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence)
{
    BOOL rc;

    rc = ImmSetConversionStatus(hIMC,fdwConversion,fdwSentence);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_SetOpenStatus(IActiveIMMApp* This,
        HIMC hIMC, BOOL fOpen)
{
    BOOL rc;

    rc = ImmSetOpenStatus(hIMC,fOpen);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_SetStatusWindowPos(IActiveIMMApp* This,
        HIMC hIMC, POINT *pptPos)
{
    BOOL rc;

    rc = ImmSetStatusWindowPos(hIMC,pptPos);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_SimulateHotKey(IActiveIMMApp* This,
        HWND hwnd, DWORD dwHotKeyID)
{
    BOOL rc;

    rc = ImmSimulateHotKey(hwnd,dwHotKeyID);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_UnregisterWordA(IActiveIMMApp* This,
        HKL hKL, LPSTR szReading, DWORD dwStyle, LPSTR szUnregister)
{
    BOOL rc;

    rc = ImmUnregisterWordA(hKL,szReading,dwStyle,szUnregister);

    if (rc)
        return S_OK;
    else
        return E_FAIL;

}

static HRESULT WINAPI ActiveIMMApp_UnregisterWordW(IActiveIMMApp* This,
        HKL hKL, LPWSTR szReading, DWORD dwStyle, LPWSTR szUnregister)
{
    BOOL rc;

    rc = ImmUnregisterWordW(hKL,szReading,dwStyle,szUnregister);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_Activate(IActiveIMMApp* This,
        BOOL fRestoreLayout)
{
    FIXME("Stub\n");
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_Deactivate(IActiveIMMApp* This)
{
    FIXME("Stub\n");
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_OnDefWindowProc(IActiveIMMApp* This,
        HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    FIXME("Stub (%p %x %Ix %Ix)\n",hWnd,Msg,wParam,lParam);
    return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_FilterClientWindows(IActiveIMMApp* This,
        ATOM *aaClassList, UINT uSize)
{
    FIXME("Stub\n");
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetCodePageA(IActiveIMMApp* This,
        HKL hKL, UINT *uCodePage)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveIMMApp_GetLangId(IActiveIMMApp* This,
        HKL hKL, LANGID *plid)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveIMMApp_AssociateContextEx(IActiveIMMApp* This,
        HWND hWnd, HIMC hIMC, DWORD dwFlags)
{
    BOOL rc;

    rc = ImmAssociateContextEx(hWnd,hIMC,dwFlags);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_DisableIME(IActiveIMMApp* This,
        DWORD idThread)
{
    BOOL rc;

    rc = ImmDisableIME(idThread);

    if (rc)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI ActiveIMMApp_GetImeMenuItemsA(IActiveIMMApp* This,
        HIMC hIMC, DWORD dwFlags, DWORD dwType,
        IMEMENUITEMINFOA *pImeParentMenu, IMEMENUITEMINFOA *pImeMenu,
        DWORD dwSize, DWORD *pdwResult)
{
    *pdwResult = ImmGetImeMenuItemsA(hIMC,dwFlags,dwType,pImeParentMenu,pImeMenu,dwSize);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_GetImeMenuItemsW(IActiveIMMApp* This,
        HIMC hIMC, DWORD dwFlags, DWORD dwType,
        IMEMENUITEMINFOW *pImeParentMenu, IMEMENUITEMINFOW *pImeMenu,
        DWORD dwSize, DWORD *pdwResult)
{
    *pdwResult = ImmGetImeMenuItemsW(hIMC,dwFlags,dwType,pImeParentMenu,pImeMenu,dwSize);
    return S_OK;
}

static HRESULT WINAPI ActiveIMMApp_EnumInputContext(IActiveIMMApp* This,
        DWORD idThread, IEnumInputContext **ppEnum)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

static const IActiveIMMAppVtbl ActiveIMMAppVtbl =
{
    ActiveIMMApp_QueryInterface,
    ActiveIMMApp_AddRef,
    ActiveIMMApp_Release,

    ActiveIMMApp_AssociateContext,
    ActiveIMMApp_ConfigureIMEA,
    ActiveIMMApp_ConfigureIMEW,
    ActiveIMMApp_CreateContext,
    ActiveIMMApp_DestroyContext,
    ActiveIMMApp_EnumRegisterWordA,
    ActiveIMMApp_EnumRegisterWordW,
    ActiveIMMApp_EscapeA,
    ActiveIMMApp_EscapeW,
    ActiveIMMApp_GetCandidateListA,
    ActiveIMMApp_GetCandidateListW,
    ActiveIMMApp_GetCandidateListCountA,
    ActiveIMMApp_GetCandidateListCountW,
    ActiveIMMApp_GetCandidateWindow,
    ActiveIMMApp_GetCompositionFontA,
    ActiveIMMApp_GetCompositionFontW,
    ActiveIMMApp_GetCompositionStringA,
    ActiveIMMApp_GetCompositionStringW,
    ActiveIMMApp_GetCompositionWindow,
    ActiveIMMApp_GetContext,
    ActiveIMMApp_GetConversionListA,
    ActiveIMMApp_GetConversionListW,
    ActiveIMMApp_GetConversionStatus,
    ActiveIMMApp_GetDefaultIMEWnd,
    ActiveIMMApp_GetDescriptionA,
    ActiveIMMApp_GetDescriptionW,
    ActiveIMMApp_GetGuideLineA,
    ActiveIMMApp_GetGuideLineW,
    ActiveIMMApp_GetIMEFileNameA,
    ActiveIMMApp_GetIMEFileNameW,
    ActiveIMMApp_GetOpenStatus,
    ActiveIMMApp_GetProperty,
    ActiveIMMApp_GetRegisterWordStyleA,
    ActiveIMMApp_GetRegisterWordStyleW,
    ActiveIMMApp_GetStatusWindowPos,
    ActiveIMMApp_GetVirtualKey,
    ActiveIMMApp_InstallIMEA,
    ActiveIMMApp_InstallIMEW,
    ActiveIMMApp_IsIME,
    ActiveIMMApp_IsUIMessageA,
    ActiveIMMApp_IsUIMessageW,
    ActiveIMMApp_NotifyIME,
    ActiveIMMApp_RegisterWordA,
    ActiveIMMApp_RegisterWordW,
    ActiveIMMApp_ReleaseContext,
    ActiveIMMApp_SetCandidateWindow,
    ActiveIMMApp_SetCompositionFontA,
    ActiveIMMApp_SetCompositionFontW,
    ActiveIMMApp_SetCompositionStringA,
    ActiveIMMApp_SetCompositionStringW,
    ActiveIMMApp_SetCompositionWindow,
    ActiveIMMApp_SetConversionStatus,
    ActiveIMMApp_SetOpenStatus,
    ActiveIMMApp_SetStatusWindowPos,
    ActiveIMMApp_SimulateHotKey,
    ActiveIMMApp_UnregisterWordA,
    ActiveIMMApp_UnregisterWordW,

    ActiveIMMApp_Activate,
    ActiveIMMApp_Deactivate,
    ActiveIMMApp_OnDefWindowProc,
    ActiveIMMApp_FilterClientWindows,
    ActiveIMMApp_GetCodePageA,
    ActiveIMMApp_GetLangId,
    ActiveIMMApp_AssociateContextEx,
    ActiveIMMApp_DisableIME,
    ActiveIMMApp_GetImeMenuItemsA,
    ActiveIMMApp_GetImeMenuItemsW,
    ActiveIMMApp_EnumInputContext
};

static inline ActiveIMMApp *impl_from_IActiveIMMMessagePumpOwner(IActiveIMMMessagePumpOwner *iface)
{
    return CONTAINING_RECORD(iface, ActiveIMMApp, IActiveIMMMessagePumpOwner_iface);
}

static HRESULT WINAPI ActiveIMMMessagePumpOwner_QueryInterface(IActiveIMMMessagePumpOwner* iface,
        REFIID iid, LPVOID *ppvOut)
{
    ActiveIMMApp *This = impl_from_IActiveIMMMessagePumpOwner(iface);
    return IActiveIMMApp_QueryInterface(&This->IActiveIMMApp_iface, iid, ppvOut);
}

static ULONG WINAPI ActiveIMMMessagePumpOwner_AddRef(IActiveIMMMessagePumpOwner* iface)
{
    ActiveIMMApp *This = impl_from_IActiveIMMMessagePumpOwner(iface);
    return IActiveIMMApp_AddRef(&This->IActiveIMMApp_iface);
}

static ULONG WINAPI ActiveIMMMessagePumpOwner_Release(IActiveIMMMessagePumpOwner* iface)
{
    ActiveIMMApp *This = impl_from_IActiveIMMMessagePumpOwner(iface);
    return IActiveIMMApp_Release(&This->IActiveIMMApp_iface);
}

static HRESULT WINAPI ActiveIMMMessagePumpOwner_Start(IActiveIMMMessagePumpOwner* iface)
{
    ActiveIMMApp *This = impl_from_IActiveIMMMessagePumpOwner(iface);
    FIXME("(%p)->(): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveIMMMessagePumpOwner_End(IActiveIMMMessagePumpOwner* iface)
{
    ActiveIMMApp *This = impl_from_IActiveIMMMessagePumpOwner(iface);
    FIXME("(%p)->(): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveIMMMessagePumpOwner_OnTranslateMessage(IActiveIMMMessagePumpOwner* iface,
        const MSG *msg)
{
    ActiveIMMApp *This = impl_from_IActiveIMMMessagePumpOwner(iface);
    FIXME("(%p)->(%p): stub\n", This, msg);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveIMMMessagePumpOwner_Pause(IActiveIMMMessagePumpOwner* iface,
        DWORD *cookie)
{
    ActiveIMMApp *This = impl_from_IActiveIMMMessagePumpOwner(iface);
    FIXME("(%p)->(%p): stub\n", This, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveIMMMessagePumpOwner_Resume(IActiveIMMMessagePumpOwner* iface,
        DWORD cookie)
{
    ActiveIMMApp *This = impl_from_IActiveIMMMessagePumpOwner(iface);
    FIXME("(%p)->(%lu): stub\n", This, cookie);
    return E_NOTIMPL;
}

static const IActiveIMMMessagePumpOwnerVtbl ActiveIMMMessagePumpOwnerVtbl =
{
    ActiveIMMMessagePumpOwner_QueryInterface,
    ActiveIMMMessagePumpOwner_AddRef,
    ActiveIMMMessagePumpOwner_Release,
    ActiveIMMMessagePumpOwner_Start,
    ActiveIMMMessagePumpOwner_End,
    ActiveIMMMessagePumpOwner_OnTranslateMessage,
    ActiveIMMMessagePumpOwner_Pause,
    ActiveIMMMessagePumpOwner_Resume,
};

HRESULT ActiveIMMApp_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    ActiveIMMApp *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ActiveIMMApp));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->IActiveIMMApp_iface.lpVtbl = &ActiveIMMAppVtbl;
    This->IActiveIMMMessagePumpOwner_iface.lpVtbl = &ActiveIMMMessagePumpOwnerVtbl;
    This->refCount = 1;

    TRACE("returning %p\n",This);
    *ppOut = (IUnknown *)&This->IActiveIMMApp_iface;
    return S_OK;
}
