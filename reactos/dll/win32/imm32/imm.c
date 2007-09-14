/*
 * IMM32 library
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/debug.h"
#include "imm.h"
#include "ddk/imm.h"
#include "winnls.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

#define FROM_IME 0xcafe1337

static void (*pX11DRV_ForceXIMReset)(HWND);

typedef struct tagIMCCInternal
{
    DWORD dwLock;
    DWORD dwSize;
} IMCCInternal;

typedef struct tagInputContextData
{
        BOOL            bInternalState;
        BOOL            bRead;
        BOOL            bInComposition;
        HFONT           textfont;

        DWORD           dwLock;
        INPUTCONTEXT    IMC;
} InputContextData;

static InputContextData *root_context = NULL;
static HWND hwndDefault = NULL;
static HANDLE hImeInst;
static const WCHAR WC_IMECLASSNAME[] = {'I','M','E',0};
static ATOM atIMEClass = 0;

/* MSIME messages */
static UINT WM_MSIME_SERVICE;
static UINT WM_MSIME_RECONVERTOPTIONS;
static UINT WM_MSIME_MOUSE;
static UINT WM_MSIME_RECONVERTREQUEST;
static UINT WM_MSIME_RECONVERT;
static UINT WM_MSIME_QUERYPOSITION;
static UINT WM_MSIME_DOCUMENTFEED;

/*
 * prototypes
 */
static LRESULT WINAPI IME_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                          LPARAM lParam);
static void UpdateDataInDefaultIMEWindow(HWND hwnd, BOOL showable);
static void ImmInternalPostIMEMessage(UINT, WPARAM, LPARAM);
static void ImmInternalSetOpenStatus(BOOL fOpen);
static HIMCC updateResultStr(HIMCC old, LPWSTR resultstr, DWORD len);

static VOID IMM_PostResult(InputContextData *data)
{
    unsigned int i;
    LPCOMPOSITIONSTRING compstr;
    LPBYTE compdata;
    LPWSTR ResultStr;
    HIMCC newCompStr;

    TRACE("Posting result as IME_CHAR\n");
    compdata = ImmLockIMCC(root_context->IMC.hCompStr);
    compstr = (LPCOMPOSITIONSTRING)compdata;
    ResultStr = (LPWSTR)(compdata + compstr->dwResultStrOffset);

    for (i = 0; i < compstr->dwResultStrLen; i++)
        ImmInternalPostIMEMessage (WM_IME_CHAR, ResultStr[i], 1);

    ImmUnlockIMCC(root_context->IMC.hCompStr);

    /* clear the buffer */
    newCompStr = updateResultStr(root_context->IMC.hCompStr, NULL, 0);
    ImmDestroyIMCC(root_context->IMC.hCompStr);
    root_context->IMC.hCompStr = newCompStr;
}

static void IMM_Register(void)
{
    WNDCLASSW wndClass;
    ZeroMemory(&wndClass, sizeof(WNDCLASSW));
    wndClass.style = CS_GLOBALCLASS | CS_IME | CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = (WNDPROC) IME_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hImeInst;
    wndClass.hCursor = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
    wndClass.hIcon = NULL;
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW +1);
    wndClass.lpszMenuName   = 0;
    wndClass.lpszClassName = WC_IMECLASSNAME;
    atIMEClass = RegisterClassW(&wndClass);
}

static void IMM_Unregister(void)
{
    if (atIMEClass) {
        UnregisterClassW(WC_IMECLASSNAME, NULL);
    }
}

static void IMM_RegisterMessages(void)
{
    WM_MSIME_SERVICE = RegisterWindowMessageA("MSIMEService");
    WM_MSIME_RECONVERTOPTIONS = RegisterWindowMessageA("MSIMEReconvertOptions");
    WM_MSIME_MOUSE = RegisterWindowMessageA("MSIMEMouseOperation");
    WM_MSIME_RECONVERTREQUEST = RegisterWindowMessageA("MSIMEReconvertRequest");
    WM_MSIME_RECONVERT = RegisterWindowMessageA("MSIMEReconvert");
    WM_MSIME_QUERYPOSITION = RegisterWindowMessageA("MSIMEQueryPosition");
    WM_MSIME_DOCUMENTFEED = RegisterWindowMessageA("MSIMEDocumentFeed");
}


BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    HMODULE x11drv;

    TRACE("%p, %x, %p\n",hInstDLL,fdwReason,lpReserved);
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            hImeInst = hInstDLL;
            IMM_RegisterMessages();
            x11drv = GetModuleHandleA("winex11.drv");
            if (x11drv) pX11DRV_ForceXIMReset = (void *)GetProcAddress( x11drv, "ForceXIMReset");
            break;
        case DLL_PROCESS_DETACH:
            if (hwndDefault)
            {
                DestroyWindow(hwndDefault);
                hwndDefault = 0;
            }
            IMM_Unregister();
            break;
    }
    return TRUE;
}

/* for posting messages as the IME */
static void ImmInternalPostIMEMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND target = GetFocus();
    if (!target)
       PostMessageW(root_context->IMC.hWnd,msg,wParam,lParam);
    else
       PostMessageW(target, msg, wParam, lParam);
}

static LRESULT ImmInternalSendIMENotify(WPARAM notify, LPARAM lParam)
{
    HWND target;

    target = root_context->IMC.hWnd;
    if (!target) target = GetFocus();

    if (target)
       return SendMessageW(target, WM_IME_NOTIFY, notify, lParam);

    return 0;
}

static void ImmInternalSetOpenStatus(BOOL fOpen)
{
    TRACE("Setting internal state to %s\n",(fOpen)?"OPEN":"CLOSED");

   root_context->IMC.fOpen = fOpen;
   root_context->bInternalState = fOpen;

   if (fOpen == FALSE)
   {
        ShowWindow(hwndDefault,SW_HIDE);
        ImmDestroyIMCC(root_context->IMC.hCompStr);
        root_context->IMC.hCompStr = NULL;
    }
    else
        ShowWindow(hwndDefault, SW_SHOWNOACTIVATE);

   ImmInternalSendIMENotify(IMN_SETOPENSTATUS, 0);
}

static int updateField(DWORD origLen, DWORD origOffset, DWORD currentOffset,
                       LPBYTE target, LPBYTE source, DWORD* lenParam,
                       DWORD* offsetParam, BOOL wchars )
{
     if (origLen > 0 && origOffset > 0)
     {
        int truelen = origLen;
        if (wchars)
            truelen *= sizeof(WCHAR);

        memcpy(&target[currentOffset], &source[origOffset], truelen);

        *lenParam = origLen;
        *offsetParam = currentOffset;
        currentOffset += truelen;
     }
     return currentOffset;
}

static HIMCC updateCompStr(HIMCC old, LPWSTR compstr, DWORD len)
{
    /* we need to make sure the CompStr, CompClaus and CompAttr fields are all
     * set and correct */
    int needed_size;
    HIMCC   rc;
    LPBYTE newdata = NULL;
    LPBYTE olddata = NULL;
    LPCOMPOSITIONSTRING new_one;
    LPCOMPOSITIONSTRING lpcs = NULL;
    INT current_offset = 0;

    TRACE("%s, %i\n",debugstr_wn(compstr,len),len);

    if (old != NULL)
    {
        olddata = ImmLockIMCC(old);
        lpcs = (LPCOMPOSITIONSTRING)olddata;
    }

    needed_size = sizeof(COMPOSITIONSTRING) + len * sizeof(WCHAR) +
                  len + sizeof(DWORD) * 2;

    if (lpcs != NULL)
    {
        needed_size += lpcs->dwCompReadAttrLen;
        needed_size += lpcs->dwCompReadClauseLen;
        needed_size += lpcs->dwCompReadStrLen * sizeof(DWORD);
        needed_size += lpcs->dwResultReadClauseLen;
        needed_size += lpcs->dwResultReadStrLen * sizeof(DWORD);
        needed_size += lpcs->dwResultClauseLen;
        needed_size += lpcs->dwResultStrLen * sizeof(DWORD);
        needed_size += lpcs->dwPrivateSize;
    }
    rc = ImmCreateIMCC(needed_size);
    newdata = ImmLockIMCC(rc);
    new_one = (LPCOMPOSITIONSTRING)newdata;

    new_one->dwSize = needed_size;
    current_offset = sizeof(COMPOSITIONSTRING);
    if (lpcs != NULL)
    {
        current_offset = updateField(lpcs->dwCompReadAttrLen,
                                     lpcs->dwCompReadAttrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadAttrLen,
                                     &new_one->dwCompReadAttrOffset, FALSE);

        current_offset = updateField(lpcs->dwCompReadClauseLen,
                                     lpcs->dwCompReadClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadClauseLen,
                                     &new_one->dwCompReadClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwCompReadStrLen,
                                     lpcs->dwCompReadStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadStrLen,
                                     &new_one->dwCompReadStrOffset, TRUE);

        /* new CompAttr, CompClause, CompStr, dwCursorPos */
        new_one->dwDeltaStart = 0;

        current_offset = updateField(lpcs->dwResultReadClauseLen,
                                     lpcs->dwResultReadClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultReadClauseLen,
                                     &new_one->dwResultReadClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwResultReadStrLen,
                                     lpcs->dwResultReadStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultReadStrLen,
                                     &new_one->dwResultReadStrOffset, TRUE);

        current_offset = updateField(lpcs->dwResultClauseLen,
                                     lpcs->dwResultClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultClauseLen,
                                     &new_one->dwResultClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwResultStrLen,
                                     lpcs->dwResultStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultStrLen,
                                     &new_one->dwResultStrOffset, TRUE);

        current_offset = updateField(lpcs->dwPrivateSize,
                                     lpcs->dwPrivateOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwPrivateSize,
                                     &new_one->dwPrivateOffset, FALSE);
    }

    /* set new data */
    /* CompAttr */
    new_one->dwCompAttrLen = len;
    if (len > 0)
    {
        new_one->dwCompAttrOffset = current_offset;
        memset(&newdata[current_offset],ATTR_INPUT,len);
        current_offset += len;
    }

    /* CompClause */
    if (len > 0)
    {
        new_one->dwCompClauseLen = sizeof(DWORD) * 2;
        new_one->dwCompClauseOffset = current_offset;
        *(DWORD*)(&newdata[current_offset]) = 0;
        current_offset += sizeof(DWORD);
        *(DWORD*)(&newdata[current_offset]) = len;
        current_offset += sizeof(DWORD);
    }

    /* CompStr */
    new_one->dwCompStrLen = len;
    if (len > 0)
    {
        new_one->dwCompStrOffset = current_offset;
        memcpy(&newdata[current_offset],compstr,len*sizeof(WCHAR));
    }

    /* CursorPos */
    new_one->dwCursorPos = len;

    ImmUnlockIMCC(rc);
    if (lpcs)
        ImmUnlockIMCC(old);

    return rc;
}

static HIMCC updateResultStr(HIMCC old, LPWSTR resultstr, DWORD len)
{
    /* we need to make sure the ResultStr and ResultClause fields are all
     * set and correct */
    int needed_size;
    HIMCC   rc;
    LPBYTE newdata = NULL;
    LPBYTE olddata = NULL;
    LPCOMPOSITIONSTRING new_one;
    LPCOMPOSITIONSTRING lpcs = NULL;
    INT current_offset = 0;

    TRACE("%s, %i\n",debugstr_wn(resultstr,len),len);

    if (old != NULL)
    {
        olddata = ImmLockIMCC(old);
        lpcs = (LPCOMPOSITIONSTRING)olddata;
    }

    needed_size = sizeof(COMPOSITIONSTRING) + len * sizeof(WCHAR) +
                  sizeof(DWORD) * 2;

    if (lpcs != NULL)
    {
        needed_size += lpcs->dwCompReadAttrLen;
        needed_size += lpcs->dwCompReadClauseLen;
        needed_size += lpcs->dwCompReadStrLen * sizeof(DWORD);
        needed_size += lpcs->dwCompAttrLen;
        needed_size += lpcs->dwCompClauseLen;
        needed_size += lpcs->dwCompStrLen * sizeof(DWORD);
        needed_size += lpcs->dwResultReadClauseLen;
        needed_size += lpcs->dwResultReadStrLen * sizeof(DWORD);
        needed_size += lpcs->dwPrivateSize;
    }
    rc = ImmCreateIMCC(needed_size);
    newdata = ImmLockIMCC(rc);
    new_one = (LPCOMPOSITIONSTRING)newdata;

    new_one->dwSize = needed_size;
    current_offset = sizeof(COMPOSITIONSTRING);
    if (lpcs != NULL)
    {
        current_offset = updateField(lpcs->dwCompReadAttrLen,
                                     lpcs->dwCompReadAttrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadAttrLen,
                                     &new_one->dwCompReadAttrOffset, FALSE);

        current_offset = updateField(lpcs->dwCompReadClauseLen,
                                     lpcs->dwCompReadClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadClauseLen,
                                     &new_one->dwCompReadClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwCompReadStrLen,
                                     lpcs->dwCompReadStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompReadStrLen,
                                     &new_one->dwCompReadStrOffset, TRUE);

        current_offset = updateField(lpcs->dwCompAttrLen,
                                     lpcs->dwCompAttrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompAttrLen,
                                     &new_one->dwCompAttrOffset, FALSE);

        current_offset = updateField(lpcs->dwCompClauseLen,
                                     lpcs->dwCompClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompClauseLen,
                                     &new_one->dwCompClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwCompStrLen,
                                     lpcs->dwCompStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwCompStrLen,
                                     &new_one->dwCompStrOffset, TRUE);

        new_one->dwCursorPos = lpcs->dwCursorPos;
        new_one->dwDeltaStart = 0;

        current_offset = updateField(lpcs->dwResultReadClauseLen,
                                     lpcs->dwResultReadClauseOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultReadClauseLen,
                                     &new_one->dwResultReadClauseOffset, FALSE);

        current_offset = updateField(lpcs->dwResultReadStrLen,
                                     lpcs->dwResultReadStrOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwResultReadStrLen,
                                     &new_one->dwResultReadStrOffset, TRUE);

        /* new ResultClause , ResultStr */

        current_offset = updateField(lpcs->dwPrivateSize,
                                     lpcs->dwPrivateOffset,
                                     current_offset, newdata, olddata,
                                     &new_one->dwPrivateSize,
                                     &new_one->dwPrivateOffset, FALSE);
    }

    /* set new data */
    /* ResultClause */
    if (len > 0)
    {
        new_one->dwResultClauseLen = sizeof(DWORD) * 2;
        new_one->dwResultClauseOffset = current_offset;
        *(DWORD*)(&newdata[current_offset]) = 0;
        current_offset += sizeof(DWORD);
        *(DWORD*)(&newdata[current_offset]) = len;
        current_offset += sizeof(DWORD);
    }

    /* ResultStr */
    new_one->dwResultStrLen = len;
    if (len > 0)
    {
        new_one->dwResultStrOffset = current_offset;
        memcpy(&newdata[current_offset],resultstr,len*sizeof(WCHAR));
    }
    ImmUnlockIMCC(rc);
    if (lpcs)
        ImmUnlockIMCC(old);

    return rc;
}



/***********************************************************************
 *		ImmAssociateContext (IMM32.@)
 */
HIMC WINAPI ImmAssociateContext(HWND hWnd, HIMC hIMC)
{
    InputContextData *data = (InputContextData*)hIMC;

    WARN("(%p, %p): semi-stub\n", hWnd, hIMC);

    if (!hIMC)
        return NULL;

    /*
     * WINE SPECIFIC! MAY CONFLICT
     * associate the root context we have an XIM created
     */
    if (hWnd == 0x000)
    {
        root_context = (InputContextData*)hIMC;
    }

    /*
     * If already associated just return
     */
    if (data->IMC.hWnd == hWnd)
        return hIMC;

    if (IsWindow(data->IMC.hWnd))
    {
        /*
         * Post a message that your context is switching
         */
        SendMessageW(data->IMC.hWnd, WM_IME_SETCONTEXT, FALSE, ISC_SHOWUIALL);
    }

    data->IMC.hWnd = hWnd;

    if (IsWindow(data->IMC.hWnd))
    {
        /*
         * Post a message that your context is switching
         */
        SendMessageW(data->IMC.hWnd, WM_IME_SETCONTEXT, TRUE, ISC_SHOWUIALL);
    }

    /*
     * TODO: We need to keep track of the old context associated
     * with a window and return it for now we will return NULL;
     */
    return NULL;
}

/***********************************************************************
 *              ImmAssociateContextEx (IMM32.@)
 */
BOOL WINAPI ImmAssociateContextEx(HWND hWnd, HIMC hIMC, DWORD dwFlags)
{
    FIXME("(%p, %p, %d): stub\n", hWnd, hIMC, dwFlags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		ImmConfigureIMEA (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEA(
  HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
  FIXME("(%p, %p, %d, %p): stub\n",
    hKL, hWnd, dwMode, lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmConfigureIMEW (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEW(
  HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
  FIXME("(%p, %p, %d, %p): stub\n",
    hKL, hWnd, dwMode, lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmCreateContext (IMM32.@)
 */
HIMC WINAPI ImmCreateContext(void)
{
    InputContextData *new_context;

    new_context = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(InputContextData));

    return (HIMC)new_context;
}

/***********************************************************************
 *		ImmDestroyContext (IMM32.@)
 */
BOOL WINAPI ImmDestroyContext(HIMC hIMC)
{
    InputContextData *data = (InputContextData*)hIMC;

    TRACE("Destroying %p\n",hIMC);

    if (hIMC)
    {
        ImmDestroyIMCC(root_context->IMC.hCompStr);
        ImmDestroyIMCC(root_context->IMC.hCandInfo);
        ImmDestroyIMCC(root_context->IMC.hGuideLine);
        ImmDestroyIMCC(root_context->IMC.hPrivate);
        ImmDestroyIMCC(root_context->IMC.hMsgBuf);

        if (data->textfont)
        {
            DeleteObject(data->textfont);
            data->textfont = NULL;
        }

        HeapFree(GetProcessHeap(),0,data);
    }
    return TRUE;
}

/***********************************************************************
 *		ImmDisableIME (IMM32.@)
 */
BOOL WINAPI ImmDisableIME(DWORD idThread)
{
    FIXME("(%d): stub\n", idThread);
    return TRUE;
}

/***********************************************************************
 *		ImmEnumRegisterWordA (IMM32.@)
 */
UINT WINAPI ImmEnumRegisterWordA(
  HKL hKL, REGISTERWORDENUMPROCA lpfnEnumProc,
  LPCSTR lpszReading, DWORD dwStyle,
  LPCSTR lpszRegister, LPVOID lpData)
{
  FIXME("(%p, %p, %s, %d, %s, %p): stub\n",
    hKL, lpfnEnumProc,
    debugstr_a(lpszReading), dwStyle,
    debugstr_a(lpszRegister), lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmEnumRegisterWordW (IMM32.@)
 */
UINT WINAPI ImmEnumRegisterWordW(
  HKL hKL, REGISTERWORDENUMPROCW lpfnEnumProc,
  LPCWSTR lpszReading, DWORD dwStyle,
  LPCWSTR lpszRegister, LPVOID lpData)
{
  FIXME("(%p, %p, %s, %d, %s, %p): stub\n",
    hKL, lpfnEnumProc,
    debugstr_w(lpszReading), dwStyle,
    debugstr_w(lpszRegister), lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmEscapeA (IMM32.@)
 */
LRESULT WINAPI ImmEscapeA(
  HKL hKL, HIMC hIMC,
  UINT uEscape, LPVOID lpData)
{
  FIXME("(%p, %p, %d, %p): stub\n",
    hKL, hIMC, uEscape, lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmEscapeW (IMM32.@)
 */
LRESULT WINAPI ImmEscapeW(
  HKL hKL, HIMC hIMC,
  UINT uEscape, LPVOID lpData)
{
  FIXME("(%p, %p, %d, %p): stub\n",
    hKL, hIMC, uEscape, lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetCandidateListA (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListA(
  HIMC hIMC, DWORD deIndex,
  LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
  FIXME("(%p, %d, %p, %d): stub\n",
    hIMC, deIndex,
    lpCandList, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetCandidateListCountA (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListCountA(
  HIMC hIMC, LPDWORD lpdwListCount)
{
  FIXME("(%p, %p): stub\n", hIMC, lpdwListCount);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetCandidateListCountW (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListCountW(
  HIMC hIMC, LPDWORD lpdwListCount)
{
  FIXME("(%p, %p): stub\n", hIMC, lpdwListCount);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetCandidateListW (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListW(
  HIMC hIMC, DWORD deIndex,
  LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
  FIXME("(%p, %d, %p, %d): stub\n",
    hIMC, deIndex,
    lpCandList, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetCandidateWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCandidateWindow(
  HIMC hIMC, DWORD dwBufLen, LPCANDIDATEFORM lpCandidate)
{
  FIXME("(%p, %d, %p): stub\n", hIMC, dwBufLen, lpCandidate);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
  FIXME("(%p, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
  FIXME("(%p, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetCompositionStringA (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringA(
  HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    CHAR *buf;
    LONG rc = 0;
    InputContextData *data = (InputContextData*)hIMC;
    LPCOMPOSITIONSTRING compstr;
    LPBYTE compdata;

    TRACE("(%p, 0x%x, %p, %d)\n", hIMC, dwIndex, lpBuf, dwBufLen);

    if (!data)
       return FALSE;

    if (!data->IMC.hCompStr)
       return FALSE;

    compdata = ImmLockIMCC(data->IMC.hCompStr);
    compstr = (LPCOMPOSITIONSTRING)compdata;

    if (dwIndex == GCS_RESULTSTR && compstr->dwResultStrLen > 0 &&
        compstr->dwResultStrOffset > 0)
    {
        LPWSTR ResultStr = (LPWSTR)(compdata + compstr->dwResultStrOffset);

        TRACE("GSC_RESULTSTR %p %i\n",ResultStr,
                                    compstr->dwResultStrLen);

        buf = HeapAlloc( GetProcessHeap(), 0, compstr->dwResultStrLen * 3 );
        rc = WideCharToMultiByte(CP_ACP, 0, ResultStr,
                                 compstr->dwResultStrLen , buf,
                                 compstr->dwResultStrLen * 3, NULL, NULL);
        if (dwBufLen >= rc)
            memcpy(lpBuf,buf,rc);

        data->bRead = TRUE;
        HeapFree( GetProcessHeap(), 0, buf );
    }
    else if (dwIndex == GCS_COMPSTR && compstr->dwCompStrLen > 0 &&
              compstr->dwCompStrOffset > 0)
    {
        LPWSTR CompString = (LPWSTR)(compdata + compstr->dwCompStrOffset);

        TRACE("GSC_COMPSTR %p %i\n", CompString, compstr->dwCompStrLen);

        buf = HeapAlloc( GetProcessHeap(), 0, compstr->dwCompStrLen * 3 );
        rc = WideCharToMultiByte(CP_ACP, 0, CompString,
                                 compstr->dwCompStrLen, buf,
                                 compstr->dwCompStrLen * 3, NULL, NULL);
        if (dwBufLen >= rc)
            memcpy(lpBuf,buf,rc);
        HeapFree( GetProcessHeap(), 0, buf );
    }
    else if (dwIndex == GCS_COMPATTR && compstr->dwCompAttrLen > 0 &&
             compstr->dwCompAttrOffset > 0)
    {
        LPWSTR Compattr = (LPWSTR)(compdata + compstr->dwCompAttrOffset);
        TRACE("GSC_COMPATTR %p %i\n", Compattr , compstr->dwCompAttrLen);

        rc = compstr->dwCompAttrLen;
        if (dwBufLen >= rc)
            memcpy(lpBuf,Compattr,rc);
    }
    else if (dwIndex == GCS_COMPCLAUSE && compstr->dwCompClauseLen > 0 &&
             compstr->dwCompClauseOffset > 0)
    {
        LPWSTR Compclause = (LPWSTR)(compdata + compstr->dwCompClauseOffset);
        TRACE("GSC_COMPCLAUSE %p %i\n", Compclause, compstr->dwCompClauseLen);

        rc = compstr->dwCompClauseLen;
        if (dwBufLen >= compstr->dwCompClauseLen)
            memcpy(lpBuf,Compclause,rc);
    }
    else if (dwIndex == GCS_RESULTCLAUSE && compstr->dwResultClauseLen > 0 &&
             compstr->dwResultClauseOffset > 0)
    {
        LPWSTR Resultclause = (LPWSTR)(compdata + compstr->dwResultClauseOffset);
        TRACE("GSC_RESULTCLAUSE %p %i\n", Resultclause, compstr->dwResultClauseLen);

        rc = compstr->dwResultClauseLen;
        if (dwBufLen >= compstr->dwResultClauseLen)
            memcpy(lpBuf,Resultclause,rc);
    }
    else if (dwIndex == GCS_CURSORPOS)
    {
        TRACE("GSC_CURSORPOS\n");
        rc = compstr->dwCursorPos;
    }
    else if (dwIndex == GCS_DELTASTART)
    {
        TRACE("GCS_DELTASTART\n");
        rc = compstr->dwDeltaStart;
    }
    else
    {
        FIXME("Unhandled index 0x%x\n",dwIndex);
    }

    ImmUnlockIMCC(data->IMC.hCompStr);

    return rc;
}

/***********************************************************************
 *		ImmGetCompositionStringW (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringW(
  HIMC hIMC, DWORD dwIndex,
  LPVOID lpBuf, DWORD dwBufLen)
{
    LONG rc = 0;
    InputContextData *data = (InputContextData*)hIMC;
    LPCOMPOSITIONSTRING compstr;
    LPBYTE compdata;

    TRACE("(%p, 0x%x, %p, %d)\n", hIMC, dwIndex, lpBuf, dwBufLen);

    if (!data)
       return FALSE;

    if (!data->IMC.hCompStr)
       return FALSE;

    compdata = ImmLockIMCC(data->IMC.hCompStr);
    compstr = (LPCOMPOSITIONSTRING)compdata;

    if (dwIndex == GCS_RESULTSTR && compstr->dwResultStrLen > 0 &&
        compstr->dwResultStrOffset > 0)
    {
        LPWSTR ResultStr = (LPWSTR)(compdata + compstr->dwResultStrOffset);
        data->bRead = TRUE;
        rc =  compstr->dwResultStrLen * sizeof(WCHAR);

        if (dwBufLen >= rc)
            memcpy(lpBuf,ResultStr,rc);
    }
    else if (dwIndex == GCS_RESULTREADSTR && compstr->dwResultReadStrLen > 0 &&
             compstr->dwResultReadStrOffset > 0)
    {
        LPWSTR ResultReadString = (LPWSTR)(compdata + compstr->dwResultReadStrOffset);

        rc = compstr->dwResultReadStrLen * sizeof(WCHAR);
        if (dwBufLen >= rc)
            memcpy(lpBuf,ResultReadString,rc);
    }
    else if (dwIndex == GCS_COMPSTR && compstr->dwCompStrLen > 0 &&
              compstr->dwCompStrOffset > 0)
    {
        LPWSTR CompString = (LPWSTR)(compdata + compstr->dwCompStrOffset);
        rc = compstr->dwCompStrLen * sizeof(WCHAR);
        if (dwBufLen >= rc)
            memcpy(lpBuf,CompString,rc);
    }
    else if (dwIndex == GCS_COMPATTR && compstr->dwCompAttrLen > 0 &&
             compstr->dwCompAttrOffset > 0)
    {

        LPWSTR Compattr = (LPWSTR)(compdata + compstr->dwCompAttrOffset);

        rc = compstr->dwCompAttrLen;
        if (dwBufLen >= rc)
            memcpy(lpBuf,Compattr,rc);
    }
    else if (dwIndex == GCS_COMPCLAUSE && compstr->dwCompClauseLen > 0 &&
             compstr->dwCompClauseOffset > 0)
    {
        LPWSTR Compclause = (LPWSTR)(compdata + compstr->dwCompClauseOffset);

        rc = compstr->dwCompClauseLen;
        if (dwBufLen >= compstr->dwCompClauseLen)
            memcpy(lpBuf,Compclause,rc);
    }
    else if (dwIndex == GCS_COMPREADSTR && compstr->dwCompReadStrLen > 0 &&
              compstr->dwCompReadStrOffset > 0)
    {
        LPWSTR CompReadString = (LPWSTR)(compdata + compstr->dwCompReadStrOffset);

        rc = compstr->dwCompReadStrLen * sizeof(WCHAR);

        if (dwBufLen >= rc)
            memcpy(lpBuf,CompReadString,rc);
    }
    else if (dwIndex == GCS_CURSORPOS)
    {
        TRACE("GSC_CURSORPOS\n");
        rc = compstr->dwCursorPos;
    }
    else if (dwIndex == GCS_DELTASTART)
    {
        TRACE("GCS_DELTASTART\n");
        rc = compstr->dwDeltaStart;
    }
    else
    {
        FIXME("Unhandled index 0x%x\n",dwIndex);
    }   

    ImmUnlockIMCC(data->IMC.hCompStr);

    return rc;
}

/***********************************************************************
 *		ImmGetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionWindow(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
    InputContextData *data = (InputContextData*)hIMC;

    TRACE("(%p, %p)\n", hIMC, lpCompForm);

    if (!data)
        return FALSE;

    memcpy(lpCompForm,&(data->IMC.cfCompForm),sizeof(COMPOSITIONFORM));
    return 1;
}

/***********************************************************************
 *		ImmGetContext (IMM32.@)
 *
 */
HIMC WINAPI ImmGetContext(HWND hWnd)
{
    TRACE("%p\n", hWnd);

    if (!root_context)
        return NULL;

    root_context->IMC.hWnd = hWnd;
    return (HIMC)root_context;
}

/***********************************************************************
 *		ImmGetConversionListA (IMM32.@)
 */
DWORD WINAPI ImmGetConversionListA(
  HKL hKL, HIMC hIMC,
  LPCSTR pSrc, LPCANDIDATELIST lpDst,
  DWORD dwBufLen, UINT uFlag)
{
  FIXME("(%p, %p, %s, %p, %d, %d): stub\n",
    hKL, hIMC, debugstr_a(pSrc), lpDst, dwBufLen, uFlag
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetConversionListW (IMM32.@)
 */
DWORD WINAPI ImmGetConversionListW(
  HKL hKL, HIMC hIMC,
  LPCWSTR pSrc, LPCANDIDATELIST lpDst,
  DWORD dwBufLen, UINT uFlag)
{
  FIXME("(%p, %p, %s, %p, %d, %d): stub\n",
    hKL, hIMC, debugstr_w(pSrc), lpDst, dwBufLen, uFlag
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmGetConversionStatus(
  HIMC hIMC, LPDWORD lpfdwConversion, LPDWORD lpfdwSentence)
{
    TRACE("(%p, %p, %p): best guess\n", hIMC, lpfdwConversion, lpfdwSentence);
    if (lpfdwConversion)
        *lpfdwConversion = IME_CMODE_NATIVE;
    if (lpfdwSentence)
        *lpfdwSentence = IME_SMODE_NONE;
    return TRUE;
}

/***********************************************************************
 *		ImmGetDefaultIMEWnd (IMM32.@)
 */
HWND WINAPI ImmGetDefaultIMEWnd(HWND hWnd)
{
  FIXME("(%p - %p %p ): semi-stub\n", hWnd,hwndDefault, root_context);

  if (hwndDefault == NULL)
  {
        static const WCHAR the_name[] = {'I','M','E','\0'};

        IMM_Register();
        hwndDefault = CreateWindowExW( WS_EX_TOOLWINDOW, WC_IMECLASSNAME,
                the_name, WS_POPUP, 0, 0, 1, 1, 0, 0,
                hImeInst, 0);

        TRACE("Default created (%p)\n",hwndDefault);
  }

  return (HWND)hwndDefault;
}

/***********************************************************************
 *		ImmGetDescriptionA (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionA(
  HKL hKL, LPSTR lpszDescription, UINT uBufLen)
{
  WCHAR *buf;
  DWORD len;

  TRACE("%p %p %d\n", hKL, lpszDescription, uBufLen);

  /* find out how many characters in the unicode buffer */
  len = ImmGetDescriptionW( hKL, NULL, 0 );

  /* allocate a buffer of that size */
  buf = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof (WCHAR) );
  if( !buf )
  return 0;

  /* fetch the unicode buffer */
  len = ImmGetDescriptionW( hKL, buf, len + 1 );

  /* convert it back to ASCII */
  len = WideCharToMultiByte( CP_ACP, 0, buf, len + 1,
                             lpszDescription, uBufLen, NULL, NULL );

  HeapFree( GetProcessHeap(), 0, buf );

  return len;
}

/***********************************************************************
 *		ImmGetDescriptionW (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionW(HKL hKL, LPWSTR lpszDescription, UINT uBufLen)
{
  static const WCHAR name[] = { 'W','i','n','e',' ','X','I','M',0 };

  FIXME("(%p, %p, %d): semi stub\n", hKL, lpszDescription, uBufLen);

  if (!uBufLen) return lstrlenW( name );
  lstrcpynW( lpszDescription, name, uBufLen );
  return lstrlenW( lpszDescription );
}

/***********************************************************************
 *		ImmGetGuideLineA (IMM32.@)
 */
DWORD WINAPI ImmGetGuideLineA(
  HIMC hIMC, DWORD dwIndex, LPSTR lpBuf, DWORD dwBufLen)
{
  FIXME("(%p, %d, %s, %d): stub\n",
    hIMC, dwIndex, debugstr_a(lpBuf), dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetGuideLineW (IMM32.@)
 */
DWORD WINAPI ImmGetGuideLineW(HIMC hIMC, DWORD dwIndex, LPWSTR lpBuf, DWORD dwBufLen)
{
  FIXME("(%p, %d, %s, %d): stub\n",
    hIMC, dwIndex, debugstr_w(lpBuf), dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetIMEFileNameA (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameA(
  HKL hKL, LPSTR lpszFileName, UINT uBufLen)
{
  FIXME("(%p, %p, %d): stub\n", hKL, lpszFileName, uBufLen);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetIMEFileNameW (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameW(
  HKL hKL, LPWSTR lpszFileName, UINT uBufLen)
{
  FIXME("(%p, %p, %d): stub\n", hKL, lpszFileName, uBufLen);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmGetOpenStatus(HIMC hIMC)
{
  InputContextData *data = (InputContextData*)hIMC;

    if (!data)
        return FALSE;
  FIXME("(%p): semi-stub\n", hIMC);

  return data->IMC.fOpen;
}

/***********************************************************************
 *		ImmGetProperty (IMM32.@)
 */
DWORD WINAPI ImmGetProperty(HKL hKL, DWORD fdwIndex)
{
    DWORD rc = 0;
    TRACE("(%p, %d)\n", hKL, fdwIndex);

    switch (fdwIndex)
    {
        case IGP_PROPERTY:
            TRACE("(%s)\n", "IGP_PROPERTY");
            rc = IME_PROP_UNICODE | IME_PROP_AT_CARET;
            break;
        case IGP_CONVERSION:
            FIXME("(%s)\n", "IGP_CONVERSION");
            rc = IME_CMODE_NATIVE;
            break;
        case IGP_SENTENCE:
            FIXME("%s)\n", "IGP_SENTENCE");
            rc = IME_SMODE_AUTOMATIC;
            break;
        case IGP_SETCOMPSTR:
            TRACE("(%s)\n", "IGP_SETCOMPSTR");
            rc = 0;
            break;
        case IGP_SELECT:
            TRACE("(%s)\n", "IGP_SELECT");
            rc = SELECT_CAP_CONVERSION | SELECT_CAP_SENTENCE;
            break;
        case IGP_GETIMEVERSION:
            TRACE("(%s)\n", "IGP_GETIMEVERSION");
            rc = IMEVER_0400;
            break;
        case IGP_UI:
            TRACE("(%s)\n", "IGP_UI");
            rc = 0;
            break;
        default:
            rc = 0;
    }
    return rc;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleA (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleA(
  HKL hKL, UINT nItem, LPSTYLEBUFA lpStyleBuf)
{
  FIXME("(%p, %d, %p): stub\n", hKL, nItem, lpStyleBuf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleW (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleW(
  HKL hKL, UINT nItem, LPSTYLEBUFW lpStyleBuf)
{
  FIXME("(%p, %d, %p): stub\n", hKL, nItem, lpStyleBuf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmGetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
  FIXME("(%p, %p): stub\n", hIMC, lpptPos);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetVirtualKey (IMM32.@)
 */
UINT WINAPI ImmGetVirtualKey(HWND hWnd)
{
  OSVERSIONINFOA version;
  FIXME("(%p): stub\n", hWnd);
  GetVersionExA( &version );
  switch(version.dwPlatformId)
  {
  case VER_PLATFORM_WIN32_WINDOWS:
      return VK_PROCESSKEY;
  case VER_PLATFORM_WIN32_NT:
      return 0;
  default:
      FIXME("%d not supported\n",version.dwPlatformId);
      return VK_PROCESSKEY;
  }
}

/***********************************************************************
 *		ImmInstallIMEA (IMM32.@)
 */
HKL WINAPI ImmInstallIMEA(
  LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText)
{
  FIXME("(%s, %s): stub\n",
    debugstr_a(lpszIMEFileName), debugstr_a(lpszLayoutText)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}

/***********************************************************************
 *		ImmInstallIMEW (IMM32.@)
 */
HKL WINAPI ImmInstallIMEW(
  LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText)
{
  FIXME("(%s, %s): stub\n",
    debugstr_w(lpszIMEFileName), debugstr_w(lpszLayoutText)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}

/***********************************************************************
 *		ImmIsIME (IMM32.@)
 */
BOOL WINAPI ImmIsIME(HKL hKL)
{
  TRACE("(%p): semi-stub\n", hKL);
  /*
   * FIXME: Dead key locales will return TRUE here when they should not
   * There is probably a more proper way to check this.
   */
  return (root_context != NULL);
}

/***********************************************************************
 *		ImmIsUIMessageA (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageA(
  HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL rc = FALSE;

    TRACE("(%p, %x, %ld, %ld)\n", hWndIME, msg, wParam, lParam);
    if ((msg >= WM_IME_STARTCOMPOSITION && msg <= WM_IME_KEYLAST) ||
        (msg >= WM_IME_SETCONTEXT && msg <= WM_IME_KEYUP) ||
        (msg == WM_MSIME_SERVICE) ||
        (msg == WM_MSIME_RECONVERTOPTIONS) ||
        (msg == WM_MSIME_MOUSE) ||
        (msg == WM_MSIME_RECONVERTREQUEST) ||
        (msg == WM_MSIME_RECONVERT) ||
        (msg == WM_MSIME_QUERYPOSITION) ||
        (msg == WM_MSIME_DOCUMENTFEED))

    {
        if (!hwndDefault)
            ImmGetDefaultIMEWnd(NULL);

        if (hWndIME == NULL)
            PostMessageA(hwndDefault, msg, wParam, lParam);

        rc = TRUE;
    }
    return rc;
}

/***********************************************************************
 *		ImmIsUIMessageW (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageW(
  HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL rc = FALSE;
    TRACE("(%p, %d, %ld, %ld): stub\n", hWndIME, msg, wParam, lParam);
    if ((msg >= WM_IME_STARTCOMPOSITION && msg <= WM_IME_KEYLAST) ||
        (msg >= WM_IME_SETCONTEXT && msg <= WM_IME_KEYUP) ||
        (msg == WM_MSIME_SERVICE) ||
        (msg == WM_MSIME_RECONVERTOPTIONS) ||
        (msg == WM_MSIME_MOUSE) ||
        (msg == WM_MSIME_RECONVERTREQUEST) ||
        (msg == WM_MSIME_RECONVERT) ||
        (msg == WM_MSIME_QUERYPOSITION) ||
        (msg == WM_MSIME_DOCUMENTFEED))
        rc = TRUE;
    return rc;
}

/***********************************************************************
 *		ImmNotifyIME (IMM32.@)
 */
BOOL WINAPI ImmNotifyIME(
  HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
    BOOL rc = FALSE;

    TRACE("(%p, %d, %d, %d)\n",
        hIMC, dwAction, dwIndex, dwValue);

    if (!root_context)
        return rc;

    switch(dwAction)
    {
        case NI_CHANGECANDIDATELIST:
            FIXME("%s\n","NI_CHANGECANDIDATELIST");
            break;
        case NI_CLOSECANDIDATE:
            FIXME("%s\n","NI_CLOSECANDIDATE");
            break;
        case NI_COMPOSITIONSTR:
            switch (dwIndex)
            {
                case CPS_CANCEL:
                    TRACE("%s - %s\n","NI_COMPOSITIONSTR","CPS_CANCEL");
                    {
                        HIMCC newCompStr;
                        if (pX11DRV_ForceXIMReset)
                            pX11DRV_ForceXIMReset(root_context->IMC.hWnd);

                        newCompStr = updateCompStr(root_context->IMC.hCompStr, NULL, 0);
                        ImmDestroyIMCC(root_context->IMC.hCompStr);
                        root_context->IMC.hCompStr = newCompStr;

                        ImmInternalPostIMEMessage(WM_IME_COMPOSITION, 0,
                                                  GCS_COMPSTR);
                        rc = TRUE;
                    }
                    break;
                case CPS_COMPLETE:
                    TRACE("%s - %s\n","NI_COMPOSITIONSTR","CPS_COMPLETE");
                    if (hIMC != (HIMC)FROM_IME && pX11DRV_ForceXIMReset)
                        pX11DRV_ForceXIMReset(root_context->IMC.hWnd);
                    {
                        HIMCC newCompStr;
                        DWORD cplen;
                        LPWSTR cpstr;
                        LPCOMPOSITIONSTRING cs = NULL;
                        LPBYTE cdata = NULL;

                        /* clear existing result */
                        newCompStr = updateResultStr(root_context->IMC.hCompStr, NULL, 0);
                        ImmDestroyIMCC(root_context->IMC.hCompStr);
                        root_context->IMC.hCompStr = newCompStr;

                        cdata = ImmLockIMCC(root_context->IMC.hCompStr);
                        cs = (LPCOMPOSITIONSTRING)cdata;
                        cplen = cs->dwCompStrLen;
                        cpstr = (LPWSTR)&(cdata[cs->dwCompStrOffset]);
                        ImmUnlockIMCC(root_context->IMC.hCompStr);
                        if (cplen > 0)
                        {
                            WCHAR param = cpstr[0];
                            newCompStr = updateResultStr(root_context->IMC.hCompStr, cpstr, cplen);
                            ImmDestroyIMCC(root_context->IMC.hCompStr);
                            root_context->IMC.hCompStr = newCompStr;
                            newCompStr = updateCompStr(root_context->IMC.hCompStr, NULL, 0);
                            ImmDestroyIMCC(root_context->IMC.hCompStr);
                            root_context->IMC.hCompStr = newCompStr;

                            root_context->bRead = FALSE;

                            ImmInternalPostIMEMessage(WM_IME_COMPOSITION, 0,
                                                  GCS_COMPSTR);

                            ImmInternalPostIMEMessage(WM_IME_COMPOSITION,
                                            param,
                                            GCS_RESULTSTR|GCS_RESULTCLAUSE);
                        }

                        ImmInternalPostIMEMessage(WM_IME_ENDCOMPOSITION, 0, 0);
                        root_context->bInComposition = FALSE;
                    }
                    break;
                case CPS_CONVERT:
                    FIXME("%s - %s\n","NI_COMPOSITIONSTR","CPS_CONVERT");
                    break;
                case CPS_REVERT:
                    FIXME("%s - %s\n","NI_COMPOSITIONSTR","CPS_REVERT");
                    break;
                default:
                    ERR("%s - %s (%i)\n","NI_COMPOSITIONSTR","UNKNOWN",dwIndex);
                    break;
            }
            break;
        case NI_IMEMENUSELECTED:
            FIXME("%s\n", "NI_IMEMENUSELECTED");
            break;
        case NI_OPENCANDIDATE:
            FIXME("%s\n", "NI_OPENCANDIDATE");
            break;
        case NI_SELECTCANDIDATESTR:
            FIXME("%s\n", "NI_SELECTCANDIDATESTR");
            break;
        case NI_SETCANDIDATE_PAGESIZE:
            FIXME("%s\n", "NI_SETCANDIDATE_PAGESIZE");
            break;
        case NI_SETCANDIDATE_PAGESTART:
            FIXME("%s\n", "NI_SETCANDIDATE_PAGESTART");
            break;
        default:
            ERR("Unknown\n");
    }

    return rc;
}

/***********************************************************************
 *		ImmRegisterWordA (IMM32.@)
 */
BOOL WINAPI ImmRegisterWordA(
  HKL hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszRegister)
{
  FIXME("(%p, %s, %d, %s): stub\n",
    hKL, debugstr_a(lpszReading), dwStyle, debugstr_a(lpszRegister)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmRegisterWordW (IMM32.@)
 */
BOOL WINAPI ImmRegisterWordW(
  HKL hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszRegister)
{
  FIXME("(%p, %s, %d, %s): stub\n",
    hKL, debugstr_w(lpszReading), dwStyle, debugstr_w(lpszRegister)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmReleaseContext (IMM32.@)
 */
BOOL WINAPI ImmReleaseContext(HWND hWnd, HIMC hIMC)
{
  FIXME("(%p, %p): stub\n", hWnd, hIMC);

    return TRUE;
}

/***********************************************************************
 *		ImmSetCandidateWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCandidateWindow(
  HIMC hIMC, LPCANDIDATEFORM lpCandidate)
{
  FIXME("(%p, %p): stub\n", hIMC, lpCandidate);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
    InputContextData *data = (InputContextData*)hIMC;
    TRACE("(%p, %p)\n", hIMC, lplf);

    if (!data)
        return FALSE;

    memcpy(&data->IMC.lfFont.W,lplf,sizeof(LOGFONTA));
    MultiByteToWideChar(CP_ACP, 0, lplf->lfFaceName, -1, data->IMC.lfFont.W.lfFaceName,
                        LF_FACESIZE);

    ImmInternalSendIMENotify(IMN_SETCOMPOSITIONFONT, 0);

    if (data->textfont)
    {
        DeleteObject(data->textfont);
        data->textfont = NULL;
    }

    data->textfont = CreateFontIndirectW(&data->IMC.lfFont.W);
    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
    InputContextData *data = (InputContextData*)hIMC;
    TRACE("(%p, %p)\n", hIMC, lplf);

    if (!data)
        return FALSE;

    memcpy(&data->IMC.lfFont.W,lplf,sizeof(LOGFONTW));
    ImmInternalSendIMENotify(IMN_SETCOMPOSITIONFONT, 0);

    if (data->textfont)
    {
        DeleteObject(data->textfont);
        data->textfont = NULL;
    }
    data->textfont = CreateFontIndirectW(&data->IMC.lfFont.W);
    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionStringA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionStringA(
  HIMC hIMC, DWORD dwIndex,
  LPCVOID lpComp, DWORD dwCompLen,
  LPCVOID lpRead, DWORD dwReadLen)
{
    DWORD comp_len;
    DWORD read_len;
    WCHAR *CompBuffer = NULL;
    WCHAR *ReadBuffer = NULL;
    BOOL rc;

    TRACE("(%p, %d, %p, %d, %p, %d): stub\n",
            hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    comp_len = MultiByteToWideChar(CP_ACP, 0, lpComp, dwCompLen, NULL, 0);
    if (comp_len)
    {
        CompBuffer = HeapAlloc(GetProcessHeap(),0,comp_len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpComp, dwCompLen, CompBuffer, comp_len);
    }

    read_len = MultiByteToWideChar(CP_ACP, 0, lpRead, dwReadLen, NULL, 0);
    if (read_len)
    {
        ReadBuffer = HeapAlloc(GetProcessHeap(),0,read_len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpRead, dwReadLen, ReadBuffer, read_len);
    }

    rc =  ImmSetCompositionStringW(hIMC, dwIndex, CompBuffer, comp_len,
                                   ReadBuffer, read_len);

    HeapFree(GetProcessHeap(), 0, CompBuffer);
    HeapFree(GetProcessHeap(), 0, ReadBuffer);

    return rc;
}

/***********************************************************************
 *		ImmSetCompositionStringW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionStringW(
	HIMC hIMC, DWORD dwIndex,
	LPCVOID lpComp, DWORD dwCompLen,
	LPCVOID lpRead, DWORD dwReadLen)
{
     DWORD flags = 0;
     WCHAR wParam  = 0;

     TRACE("(%p, %d, %p, %d, %p, %d): stub\n",
             hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);


     if (hIMC != (HIMC)FROM_IME)
         FIXME("PROBLEM: This only sets the wine level string\n");

     /*
      * Explanation:
      *  this sets the composition string in the imm32.dll level
      *  of the composition buffer. we cannot manipulate the xim level
      *  buffer, which means that once the xim level buffer changes again
      *  any call to this function from the application will be lost
      */

     if (lpRead && dwReadLen)
         FIXME("Reading string unimplemented\n");

     /*
      * app operating this api to also receive the message from xim
      */

    if (dwIndex == SCS_SETSTR)
    {
        HIMCC newCompStr;
        if (!root_context->bInComposition)
        {
            ImmInternalPostIMEMessage(WM_IME_STARTCOMPOSITION, 0, 0);
            root_context->bInComposition = TRUE;
        }

        flags = GCS_COMPSTR;

        if (dwCompLen && lpComp)
        {
            newCompStr = updateCompStr(root_context->IMC.hCompStr, (LPWSTR)lpComp, dwCompLen / sizeof(WCHAR));
            ImmDestroyIMCC(root_context->IMC.hCompStr);
            root_context->IMC.hCompStr = newCompStr;

             wParam = ((const WCHAR*)lpComp)[0];
             flags |= GCS_COMPCLAUSE | GCS_COMPATTR | GCS_DELTASTART;
        }
        else
        {
            newCompStr = updateCompStr(root_context->IMC.hCompStr, NULL, 0);
            ImmDestroyIMCC(root_context->IMC.hCompStr);
            root_context->IMC.hCompStr = newCompStr;
        }
    }

     UpdateDataInDefaultIMEWindow(hwndDefault,FALSE);

     ImmInternalPostIMEMessage(WM_IME_COMPOSITION, wParam, flags);

     return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionWindow(
  HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
    BOOL reshow = FALSE;
    InputContextData *data = (InputContextData*)hIMC;

    TRACE("(%p, %p)\n", hIMC, lpCompForm);
    TRACE("\t%x, (%i,%i), (%i,%i - %i,%i)\n",lpCompForm->dwStyle,
          lpCompForm->ptCurrentPos.x, lpCompForm->ptCurrentPos.y, lpCompForm->rcArea.top,
          lpCompForm->rcArea.left, lpCompForm->rcArea.bottom, lpCompForm->rcArea.right);

    if (!data)
        return FALSE;

    memcpy(&data->IMC.cfCompForm,lpCompForm,sizeof(COMPOSITIONFORM));

    if (IsWindowVisible(hwndDefault))
    {
        reshow = TRUE;
        ShowWindow(hwndDefault,SW_HIDE);
    }

    /* FIXME: this is a partial stub */

    if (reshow)
        ShowWindow(hwndDefault,SW_SHOWNOACTIVATE);

    ImmInternalSendIMENotify(IMN_SETCOMPOSITIONWINDOW, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmSetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmSetConversionStatus(
  HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence)
{
  FIXME("(%p, %d, %d): stub\n",
    hIMC, fdwConversion, fdwSentence
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmSetOpenStatus(HIMC hIMC, BOOL fOpen)
{
    InputContextData *data = (InputContextData*)hIMC;

    TRACE("%p %d\n", hIMC, fOpen);

    if (hIMC == (HIMC)FROM_IME)
    {
        ImmInternalSetOpenStatus(fOpen);
        ImmInternalSendIMENotify(IMN_SETOPENSTATUS, 0);
        return TRUE;
    }

    if (!data)
        return FALSE;

    if (fOpen != data->bInternalState)
    {
        if (fOpen == FALSE && pX11DRV_ForceXIMReset)
            pX11DRV_ForceXIMReset(data->IMC.hWnd);

        if (fOpen == FALSE)
            ImmInternalPostIMEMessage(WM_IME_ENDCOMPOSITION,0,0);
        else
            ImmInternalPostIMEMessage(WM_IME_STARTCOMPOSITION,0,0);

        ImmInternalSetOpenStatus(fOpen);
        ImmInternalSetOpenStatus(!fOpen);

        if (data->IMC.fOpen == FALSE)
            ImmInternalPostIMEMessage(WM_IME_ENDCOMPOSITION,0,0);
        else
            ImmInternalPostIMEMessage(WM_IME_STARTCOMPOSITION,0,0);

        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *		ImmSetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmSetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
  FIXME("(%p, %p): stub\n", hIMC, lpptPos);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSimulateHotKey (IMM32.@)
 */
BOOL WINAPI ImmSimulateHotKey(HWND hWnd, DWORD dwHotKeyID)
{
  FIXME("(%p, %d): stub\n", hWnd, dwHotKeyID);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmUnregisterWordA (IMM32.@)
 */
BOOL WINAPI ImmUnregisterWordA(
  HKL hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszUnregister)
{
  FIXME("(%p, %s, %d, %s): stub\n",
    hKL, debugstr_a(lpszReading), dwStyle, debugstr_a(lpszUnregister)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmUnregisterWordW (IMM32.@)
 */
BOOL WINAPI ImmUnregisterWordW(
  HKL hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszUnregister)
{
  FIXME("(%p, %s, %d, %s): stub\n",
    hKL, debugstr_w(lpszReading), dwStyle, debugstr_w(lpszUnregister)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetImeMenuItemsA (IMM32.@)
 */
DWORD WINAPI ImmGetImeMenuItemsA( HIMC hIMC, DWORD dwFlags, DWORD dwType,
   LPIMEMENUITEMINFOA lpImeParentMenu, LPIMEMENUITEMINFOA lpImeMenu,
    DWORD dwSize)
{
  FIXME("(%p, %i, %i, %p, %p, %i): stub\n", hIMC, dwFlags, dwType,
    lpImeParentMenu, lpImeMenu, dwSize);
  return 0;
}

/***********************************************************************
*		ImmGetImeMenuItemsW (IMM32.@)
*/
DWORD WINAPI ImmGetImeMenuItemsW( HIMC hIMC, DWORD dwFlags, DWORD dwType,
   LPIMEMENUITEMINFOW lpImeParentMenu, LPIMEMENUITEMINFOW lpImeMenu,
   DWORD dwSize)
{
  FIXME("(%p, %i, %i, %p, %p, %i): stub\n", hIMC, dwFlags, dwType,
    lpImeParentMenu, lpImeMenu, dwSize);
  return 0;
}

/***********************************************************************
*		ImmLockIMC(IMM32.@)
*/
LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC hIMC)
{
    InputContextData *data = (InputContextData*)hIMC;

    if (!data)
        return NULL;
    data->dwLock++;
    return &data->IMC;
}

/***********************************************************************
*		ImmUnlockIMC(IMM32.@)
*/
BOOL WINAPI ImmUnlockIMC(HIMC hIMC)
{
    InputContextData *data = (InputContextData*)hIMC;
    data->dwLock--;
    return (data->dwLock!=0);
}

/***********************************************************************
*		ImmGetIMCLockCount(IMM32.@)
*/
DWORD WINAPI ImmGetIMCLockCount(HIMC hIMC)
{
    InputContextData *data = (InputContextData*)hIMC;
    return data->dwLock;
}

/***********************************************************************
*		ImmCreateIMCC(IMM32.@)
*/
HIMCC  WINAPI ImmCreateIMCC(DWORD size)
{
    IMCCInternal *internal;
    int real_size = size + sizeof(IMCCInternal);

    internal = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, real_size);
    if (internal == NULL)
        return NULL;

    internal->dwSize = size;
    return  (HIMCC)internal;
}

/***********************************************************************
*       ImmDestroyIMCC(IMM32.@)
*/
HIMCC WINAPI ImmDestroyIMCC(HIMCC block)
{
    HeapFree(GetProcessHeap(),0,block);
    return NULL;
}

/***********************************************************************
*		ImmLockIMCC(IMM32.@)
*/
LPVOID WINAPI ImmLockIMCC(HIMCC imcc)
{
    IMCCInternal *internal;
    internal = (IMCCInternal*) imcc;

    internal->dwLock ++;
    return internal + 1;
}

/***********************************************************************
*		ImmUnlockIMCC(IMM32.@)
*/
BOOL WINAPI ImmUnlockIMCC(HIMCC imcc)
{
    IMCCInternal *internal;
    internal = (IMCCInternal*) imcc;

    internal->dwLock --;
    return (internal->dwLock!=0);
}

/***********************************************************************
*		ImmGetIMCCLockCount(IMM32.@)
*/
DWORD WINAPI ImmGetIMCCLockCount(HIMCC imcc)
{
    IMCCInternal *internal;
    internal = (IMCCInternal*) imcc;

    return internal->dwLock;
}

/***********************************************************************
*		ImmReSizeIMCC(IMM32.@)
*/
HIMCC  WINAPI ImmReSizeIMCC(HIMCC imcc, DWORD size)
{
    IMCCInternal *internal,*newone;
    int real_size = size + sizeof(IMCCInternal);

    internal = (IMCCInternal*) imcc;

    newone = HeapReAlloc(GetProcessHeap(), 0, internal, real_size);
    newone->dwSize = size;

    return newone;
}

/***********************************************************************
*		ImmGetIMCCSize(IMM32.@)
*/
DWORD WINAPI ImmGetIMCCSize(HIMCC imcc)
{
    IMCCInternal *internal;
    internal = (IMCCInternal*) imcc;

    return internal->dwSize;
}

/*****
 * Internal functions to help with IME window management
 */
static void PaintDefaultIMEWnd(HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT rect;
    HDC hdc = BeginPaint(hwnd,&ps);
    LPCOMPOSITIONSTRING compstr;
    LPBYTE compdata = NULL;
    HMONITOR monitor;
    MONITORINFO mon_info;
    INT offX=0, offY=0;

    GetClientRect(hwnd,&rect);
    FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));

    compdata = ImmLockIMCC(root_context->IMC.hCompStr);
    compstr = (LPCOMPOSITIONSTRING)compdata;

    if (compstr->dwCompStrLen && compstr->dwCompStrOffset)
    {
        SIZE size;
        POINT pt;
        HFONT oldfont = NULL;
        LPWSTR CompString;

        CompString = (LPWSTR)(compdata + compstr->dwCompStrOffset);
        if (root_context->textfont)
            oldfont = SelectObject(hdc,root_context->textfont);


        GetTextExtentPoint32W(hdc, CompString, compstr->dwCompStrLen, &size);
        pt.x = size.cx;
        pt.y = size.cy;
        LPtoDP(hdc,&pt,1);

        /*
         * How this works based on tests on windows:
         * CFS_POINT: then we start our window at the point and grow it as large
         *    as it needs to be for the string.
         * CFS_RECT:  we still use the ptCurrentPos as a starting point and our
         *    window is only as large as we need for the string, but we do not
         *    grow such that our window exceeds the given rect.  Wrapping if
         *    needed and possible.   If our ptCurrentPos is outside of our rect
         *    then no window is displayed.
         * CFS_FORCE_POSITION: appears to behave just like CFS_POINT
         *    maybe becase the default MSIME does not do any IME adjusting.
         */
        if (root_context->IMC.cfCompForm.dwStyle != CFS_DEFAULT)
        {
            POINT cpt = root_context->IMC.cfCompForm.ptCurrentPos;
            ClientToScreen(root_context->IMC.hWnd,&cpt);
            rect.left = cpt.x;
            rect.top = cpt.y;
            rect.right = rect.left + pt.x;
            rect.bottom = rect.top + pt.y;
            offX=offY=10;
            monitor = MonitorFromPoint(cpt, MONITOR_DEFAULTTOPRIMARY);
        }
        else /* CFS_DEFAULT */
        {
            /* Windows places the default IME window in the bottom left */
            HWND target = root_context->IMC.hWnd;
            if (!target) target = GetFocus();

            GetWindowRect(target,&rect);
            rect.top = rect.bottom;
            rect.right = rect.left + pt.x + 20;
            rect.bottom = rect.top + pt.y + 20;
            offX=offY=10;
            monitor = MonitorFromWindow(target, MONITOR_DEFAULTTOPRIMARY);
        }

        if (root_context->IMC.cfCompForm.dwStyle == CFS_RECT)
        {
            RECT client;
            client =root_context->IMC.cfCompForm.rcArea;
            MapWindowPoints( root_context->IMC.hWnd, 0, (POINT *)&client, 2 );
            IntersectRect(&rect,&rect,&client);
            /* TODO:  Wrap the input if needed */
        }

        if (root_context->IMC.cfCompForm.dwStyle == CFS_DEFAULT)
        {
            /* make sure we are on the desktop */
            mon_info.cbSize = sizeof(mon_info);
            GetMonitorInfoW(monitor, &mon_info);

            if (rect.bottom > mon_info.rcWork.bottom)
            {
                int shift = rect.bottom - mon_info.rcWork.bottom;
                rect.top -= shift;
                rect.bottom -= shift;
            }
            if (rect.left < 0)
            {
                rect.right -= rect.left;
                rect.left = 0;
            }
            if (rect.right > mon_info.rcWork.right)
            {
                int shift = rect.right - mon_info.rcWork.right;
                rect.left -= shift;
                rect.right -= shift;
            }
        }

        SetWindowPos(hwnd, HWND_TOPMOST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE);

        TextOutW(hdc, offX,offY, CompString, compstr->dwCompStrLen);

        if (oldfont)
            SelectObject(hdc,oldfont);
    }

    ImmUnlockIMCC(root_context->IMC.hCompStr);

    EndPaint(hwnd,&ps);
}

static void UpdateDataInDefaultIMEWindow(HWND hwnd, BOOL showable)
{
    LPCOMPOSITIONSTRING compstr;

    if (root_context->IMC.hCompStr)
        compstr = ImmLockIMCC(root_context->IMC.hCompStr);
    else
        compstr = NULL;

    if (compstr == NULL || compstr->dwCompStrLen == 0)
        ShowWindow(hwndDefault,SW_HIDE);
    else if (showable)
        ShowWindow(hwndDefault,SW_SHOWNOACTIVATE);

    RedrawWindow(hwnd,NULL,NULL,RDW_ERASENOW|RDW_INVALIDATE);

    if (compstr != NULL)
        ImmUnlockIMCC(root_context->IMC.hCompStr);
}

/*
 * The window proc for the default IME window
 */
static LRESULT WINAPI IME_WindowProc(HWND hwnd, UINT msg, WPARAM wParam,
                                          LPARAM lParam)
{
    LRESULT rc = 0;

    TRACE("Incoming Message 0x%x  (0x%08x, 0x%08x)\n", msg, (UINT)wParam,
           (UINT)lParam);

    switch(msg)
    {
        case WM_PAINT:
            PaintDefaultIMEWnd(hwnd);
            return FALSE;

        case WM_NCCREATE:
            return TRUE;

        case WM_CREATE:
            SetWindowTextA(hwnd,"Wine Ime Active");
            return TRUE;

        case WM_SETFOCUS:
            if (wParam)
                SetFocus((HWND)wParam);
            else
                FIXME("Received focus, should never have focus\n");
            break;
        case WM_IME_COMPOSITION:
            TRACE("IME message %s, 0x%x, 0x%x (%i)\n",
                    "WM_IME_COMPOSITION", (UINT)wParam, (UINT)lParam,
                     root_context->bRead);
            if (lParam & GCS_RESULTSTR)
                    IMM_PostResult(root_context);
            else
                 UpdateDataInDefaultIMEWindow(hwnd,TRUE);
            break;
        case WM_IME_STARTCOMPOSITION:
            TRACE("IME message %s, 0x%x, 0x%x\n",
                    "WM_IME_STARTCOMPOSITION", (UINT)wParam, (UINT)lParam);
            root_context->IMC.hWnd = GetFocus();
            ShowWindow(hwndDefault,SW_SHOWNOACTIVATE);
            break;
        case WM_IME_ENDCOMPOSITION:
            TRACE("IME message %s, 0x%x, 0x%x\n",
                    "WM_IME_ENDCOMPOSITION", (UINT)wParam, (UINT)lParam);
            ShowWindow(hwndDefault,SW_HIDE);
            break;
        case WM_IME_SELECT:
            TRACE("IME message %s, 0x%x, 0x%x\n","WM_IME_SELECT",
                (UINT)wParam, (UINT)lParam);
            break;
        case WM_IME_CONTROL:
            TRACE("IME message %s, 0x%x, 0x%x\n","WM_IME_CONTROL",
                (UINT)wParam, (UINT)lParam);
            rc = 1;
            break;
        case WM_IME_NOTIFY:
            TRACE("!! IME NOTIFY\n");
            break;
       default:
            TRACE("Non-standard message 0x%x\n",msg);
    }
    /* check the MSIME messages */
    if (msg == WM_MSIME_SERVICE)
    {
            TRACE("IME message %s, 0x%x, 0x%x\n","WM_MSIME_SERVICE",
                (UINT)wParam, (UINT)lParam);
            rc = FALSE;
    }
    else if (msg == WM_MSIME_RECONVERTOPTIONS)
    {
            TRACE("IME message %s, 0x%x, 0x%x\n","WM_MSIME_RECONVERTOPTIONS",
                (UINT)wParam, (UINT)lParam);
    }
    else if (msg == WM_MSIME_MOUSE)
    {
            TRACE("IME message %s, 0x%x, 0x%x\n","WM_MSIME_MOUSE",
                (UINT)wParam, (UINT)lParam);
    }
    else if (msg == WM_MSIME_RECONVERTREQUEST)
    {
            TRACE("IME message %s, 0x%x, 0x%x\n","WM_MSIME_RECONVERTREQUEST",
                (UINT)wParam, (UINT)lParam);
    }
    else if (msg == WM_MSIME_RECONVERT)
    {
            TRACE("IME message %s, 0x%x, 0x%x\n","WM_MSIME_RECONVERT",
                (UINT)wParam, (UINT)lParam);
    }
    else if (msg == WM_MSIME_QUERYPOSITION)
    {
            TRACE("IME message %s, 0x%x, 0x%x\n","WM_MSIME_QUERYPOSITION",
                (UINT)wParam, (UINT)lParam);
    }
    else if (msg == WM_MSIME_DOCUMENTFEED)
    {
            TRACE("IME message %s, 0x%x, 0x%x\n","WM_MSIME_DOCUMENTFEED",
                (UINT)wParam, (UINT)lParam);
    }
    /* DefWndProc if not an IME message */
    else if (!rc && !((msg >= WM_IME_STARTCOMPOSITION && msg <= WM_IME_KEYLAST) ||
                      (msg >= WM_IME_SETCONTEXT && msg <= WM_IME_KEYUP)))
        rc = DefWindowProcW(hwnd,msg,wParam,lParam);

    return rc;
}
