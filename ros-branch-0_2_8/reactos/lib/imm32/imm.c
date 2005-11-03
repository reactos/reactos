/*
 * IMM32 library
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2002 Codeweavers, Aric Stewart
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/debug.h"
#include "imm.h"
#include "winnls.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

typedef struct tagInputContextData
{
        LPBYTE  CompositionString;
        LPBYTE  CompositionReadingString;
        LPBYTE  ResultString;
        LPBYTE  ResultReadingString;
        DWORD   dwCompStringSize;
        DWORD   dwCompReadStringSize;
        DWORD   dwResultStringSize;
        DWORD   dwResultReadStringSize;
        HWND    hwnd;
        BOOL    bOpen;
        BOOL    bRead;
} InputContextData; 

static InputContextData *root_context = NULL;
static HWND hwndDefault = NULL;
static HANDLE hImeInst;
static const WCHAR WC_IMECLASSNAME[] = {'W','i','n','e','I','M','E','C','l','a','s','s',0};

static LRESULT CALLBACK IME_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static VOID IMM_PostResult(InputContextData *data)
{
    int i;

    for (i = 0; i < data->dwResultStringSize / sizeof (WCHAR); i++)
        SendMessageW(data->hwnd, WM_IME_CHAR, ((WCHAR*)data->ResultString)[i], 1);
}

static void IMM_Register(void)
{
    WNDCLASSW wndClass;
    ZeroMemory(&wndClass, sizeof(WNDCLASSW));
    wndClass.style = CS_GLOBALCLASS | CS_IME;
    wndClass.lpfnWndProc = IME_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hCursor = NULL;
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW +1);
    wndClass.lpszClassName = WC_IMECLASSNAME;
    RegisterClassW(&wndClass);
}

static void IMM_Unregister(void)
{
    UnregisterClassW(WC_IMECLASSNAME, NULL);
}


BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    TRACE("%p, %lx, %p\n",hInstDLL,fdwReason,lpReserved);
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            hImeInst = hInstDLL;
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


/***********************************************************************
 *		ImmAssociateContext (IMM32.@)
 */
HIMC WINAPI ImmAssociateContext(HWND hWnd, HIMC hIMC)
{
    InputContextData *data = (InputContextData*)hIMC;

    FIXME("(%p, %p): semi-stub\n",hWnd,hIMC);

    if (!data)
        return FALSE;

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
    if (data->hwnd == hWnd)
        return (HIMC)data;

    if (IsWindow(data->hwnd))
    {
        /*
         * Post a message that your context is switching
         */
        SendMessageW(data->hwnd, WM_IME_SETCONTEXT, FALSE, ISC_SHOWUIALL);
    }

    data->hwnd = hWnd;

    if (IsWindow(data->hwnd))
    {
        /*
         * Post a message that your context is switching
         */
        SendMessageW(data->hwnd, WM_IME_SETCONTEXT, TRUE, ISC_SHOWUIALL);
    }

    /*
     * TODO: We need to keep track of the old context associated
     * with a window and return it for now we will return NULL;
     */
    return (HIMC)NULL;
}

/***********************************************************************
 *		ImmConfigureIMEA (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEA(
  HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
  FIXME("(%p, %p, %ld, %p): stub\n",
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
  FIXME("(%p, %p, %ld, %p): stub\n",
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

    new_context = HeapAlloc(GetProcessHeap(),0,sizeof(InputContextData));
    ZeroMemory(new_context,sizeof(InputContextData));

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
        if (data->dwCompStringSize)
            HeapFree(GetProcessHeap(),0,data->CompositionString);
        if (data->dwCompReadStringSize)
            HeapFree(GetProcessHeap(),0,data->CompositionReadingString);
        if (data->dwResultStringSize)
            HeapFree(GetProcessHeap(),0,data->ResultString);
        if (data->dwResultReadStringSize)
            HeapFree(GetProcessHeap(),0,data->ResultReadingString);

        HeapFree(GetProcessHeap(),0,data);
    }
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
  FIXME("(%p, %p, %s, %ld, %s, %p): stub\n",
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
  FIXME("(%p, %p, %s, %ld, %s, %p): stub\n",
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
  FIXME("(%p, %ld, %p, %ld): stub\n",
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
  FIXME("(%p, %ld, %p, %ld): stub\n",
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
  FIXME("(%p, %ld, %p): stub\n", hIMC, dwBufLen, lpCandidate);
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
    LONG rc; 
    LPBYTE wcstring=NULL;

    FIXME("(%p, %ld, %p, %ld): stub\n",
            hIMC, dwIndex, lpBuf, dwBufLen);

    if (dwBufLen > 0)
        wcstring = HeapAlloc(GetProcessHeap(),0,dwBufLen * 2); 
 
    rc = ImmGetCompositionStringW(hIMC, dwIndex, wcstring, dwBufLen*2 );

    if ((rc > dwBufLen) || (rc == 0))
    {
        if (wcstring)
            HeapFree(GetProcessHeap(),0,wcstring);

        return rc;
    }
 
    rc = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)wcstring, (rc / sizeof(WCHAR)),
                             lpBuf, dwBufLen, NULL, NULL);

    HeapFree(GetProcessHeap(),0,wcstring);

    return rc;
}

/***********************************************************************
 *		ImmGetCompositionStringW (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringW(
  HIMC hIMC, DWORD dwIndex,
  LPVOID lpBuf, DWORD dwBufLen)
{
  InputContextData *data = (InputContextData*)hIMC;

  FIXME("(%p, 0x%lx, %p, %ld): stub\n",
    hIMC, dwIndex, lpBuf, dwBufLen
  );

    if (!data)
       return FALSE;

    if (dwIndex == GCS_RESULTSTR)
    {
        data->bRead = TRUE;

        if (dwBufLen >= data->dwResultStringSize)
            memcpy(lpBuf,data->ResultString,data->dwResultStringSize);
        
        return data->dwResultStringSize;
    }

    if (dwIndex == GCS_RESULTREADSTR)
    {
        if (dwBufLen >= data->dwResultReadStringSize)
            memcpy(lpBuf,data->ResultReadingString,
                    data->dwResultReadStringSize);
        
        return data->dwResultReadStringSize;
    }   

    if (dwIndex == GCS_COMPSTR)
    {
        if (dwBufLen >= data->dwCompStringSize)
            memcpy(lpBuf,data->CompositionString,data->dwCompStringSize);
        
        return data->dwCompStringSize;
    }

    if (dwIndex == GCS_COMPREADSTR)
    {
        if (dwBufLen >= data->dwCompReadStringSize)
            memcpy(lpBuf,data->CompositionReadingString,
                    data->dwCompReadStringSize);
        
        return data->dwCompReadStringSize;
    }   

    return 0;
}

/***********************************************************************
 *		ImmGetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionWindow(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
  FIXME("(%p, %p): stub\n", hIMC, lpCompForm);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetContext (IMM32.@)
 */
HIMC WINAPI ImmGetContext(HWND hWnd)
{
    FIXME("(%p): stub\n", hWnd);
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
  FIXME("(%p, %p, %s, %p, %ld, %d): stub\n",
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
  FIXME("(%p, %p, %s, %p, %ld, %d): stub\n",
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
  FIXME("(%p, %p, %p): stub\n",
    hIMC, lpfdwConversion, lpfdwSentence
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetDefaultIMEWnd (IMM32.@)
 */
HWND WINAPI ImmGetDefaultIMEWnd(HWND hWnd)
{
  FIXME("(%p): semi-stub\n", hWnd);

  if ((!hwndDefault) && (root_context))
  {
        static const WCHAR name[] = {'I','M','E',0};
        IMM_Register();

        hwndDefault = CreateWindowW( WC_IMECLASSNAME,
                       name,WS_POPUPWINDOW,0,0,0,0,0,0,hImeInst,0);
  }

  return (HWND)hwndDefault;
}

/***********************************************************************
 *		ImmGetDescriptionA (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionA(
  HKL hKL, LPSTR lpszDescription, UINT uBufLen)
{
  FIXME("(%p, %s, %d): stub\n",
    hKL, debugstr_a(lpszDescription), uBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetDescriptionW (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionW(HKL hKL, LPWSTR lpszDescription, UINT uBufLen)
{
  FIXME("(%p, %s, %d): stub\n",
    hKL, debugstr_w(lpszDescription), uBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetGuideLineA (IMM32.@)
 */
DWORD WINAPI ImmGetGuideLineA(
  HIMC hIMC, DWORD dwIndex, LPSTR lpBuf, DWORD dwBufLen)
{
  FIXME("(%p, %ld, %s, %ld): stub\n",
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
  FIXME("(%p, %ld, %s, %ld): stub\n",
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
  FIXME("(%p, %s, %d): stub\n",
    hKL, debugstr_a(lpszFileName), uBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetIMEFileNameW (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameW(
  HKL hKL, LPWSTR lpszFileName, UINT uBufLen)
{
  FIXME("(%p, %s, %d): stub\n",
    hKL, debugstr_w(lpszFileName), uBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmGetOpenStatus(HIMC hIMC)
{
  InputContextData *data = (InputContextData*)hIMC;

  FIXME("(%p): semi-stub\n", hIMC);

  if (!data) return FALSE;

  return data->bOpen;
}

/***********************************************************************
 *		ImmGetProperty (IMM32.@)
 */
DWORD WINAPI ImmGetProperty(HKL hKL, DWORD fdwIndex)
{
    DWORD rc = 0;
    FIXME("(%p, %ld): semi-stub\n", hKL, fdwIndex);

    switch (fdwIndex)
    {
        case IGP_PROPERTY:
            rc = IME_PROP_UNICODE | IME_PROP_SPECIAL_UI;
            break;
        case IGP_SETCOMPSTR:
            rc = SCS_CAP_COMPSTR;
            break;
        case IGP_SELECT:
            rc = SELECT_CAP_CONVERSION | SELECT_CAP_SENTENCE;
            break;
        case IGP_GETIMEVERSION:
            rc = IMEVER_0400;
            break;
        case IGP_UI:
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
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  GetVersionExA( &version );
  switch(version.dwPlatformId)
  {
  case VER_PLATFORM_WIN32_WINDOWS:
      return VK_PROCESSKEY;
  case VER_PLATFORM_WIN32_NT:
      return 0;
  default:
      FIXME("%ld not supported\n",version.dwPlatformId);
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
  FIXME("(%p): semi-stub\n", hKL);
  /*
   * Dead key locales will return TRUE here when they should not
   * There is probibly a more proper way to check this.
   */
  return (root_context != NULL);
}

/***********************************************************************
 *		ImmIsUIMessageA (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageA(
  HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
  FIXME("(%p, %d, %d, %ld): stub\n",
    hWndIME, msg, wParam, lParam
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmIsUIMessageW (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageW(
  HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
  FIXME("(%p, %d, %d, %ld): stub\n",
    hWndIME, msg, wParam, lParam
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmNotifyIME (IMM32.@)
 */
BOOL WINAPI ImmNotifyIME(
  HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
  FIXME("(%p, %ld, %ld, %ld): stub\n",
    hIMC, dwAction, dwIndex, dwValue
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmRegisterWordA (IMM32.@)
 */
BOOL WINAPI ImmRegisterWordA(
  HKL hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszRegister)
{
  FIXME("(%p, %s, %ld, %s): stub\n",
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
  FIXME("(%p, %s, %ld, %s): stub\n",
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
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
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
  FIXME("(%p, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
  FIXME("(%p, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetCompositionStringA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionStringA(
  HIMC hIMC, DWORD dwIndex,
  LPCVOID lpComp, DWORD dwCompLen,
  LPCVOID lpRead, DWORD dwReadLen)
{
  FIXME("(%p, %ld, %p, %ld, %p, %ld): stub\n",
    hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetCompositionStringW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionStringW(
	HIMC hIMC, DWORD dwIndex,
	LPCVOID lpComp, DWORD dwCompLen,
	LPCVOID lpRead, DWORD dwReadLen)
{
    InputContextData *data = (InputContextData*)hIMC;

    FIXME("(%p, %ld, %p, %ld, %p, %ld): semi-stub\n",
        hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen
        );

    if (!data)
        return FALSE;

    if ((dwIndex == SCS_SETSTR)&&(dwCompLen || dwReadLen))
        SendMessageW(data->hwnd, WM_IME_STARTCOMPOSITION, 0, 0);

    if (dwIndex == SCS_SETSTR)
    {
        INT send_comp = 0;


        if (lpComp && dwCompLen)
        {
/*            if (data->dwCompStringSize)
                HeapFree(GetProcessHeap(),0,data->CompositionString);
            data->dwCompStringSize = dwCompLen;
            data->CompositionString = HeapAlloc(GetProcessHeap(),0,dwCompLen); 
            memcpy(data->CompositionString,lpComp,dwCompLen);
            send_comp |= GCS_COMPSTR;
*/
            data->bRead = FALSE;

            if (data->dwResultStringSize)
                HeapFree(GetProcessHeap(),0,data->ResultString);
            data->dwResultStringSize= dwCompLen;
            data->ResultString= HeapAlloc(GetProcessHeap(),0,dwCompLen); 
            memcpy(data->ResultString,lpComp,dwCompLen);
            send_comp |= GCS_RESULTSTR;
        }

        if (lpRead && dwReadLen)
        {
            if (data->dwCompReadStringSize)
                HeapFree(GetProcessHeap(),0,data->CompositionReadingString);
            data->dwCompReadStringSize= dwReadLen;
            data->CompositionReadingString = HeapAlloc(GetProcessHeap(), 0,
                                                        dwReadLen);
            memcpy(data->CompositionReadingString,lpRead,dwReadLen);
            send_comp |= GCS_COMPREADSTR;
        }

        if (send_comp)
            SendMessageW(data->hwnd, WM_IME_COMPOSITION, 0, send_comp);

        SendMessageW(data->hwnd, WM_IME_ENDCOMPOSITION, 0, 0);

        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *		ImmSetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionWindow(
  HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
  FIXME("(%p, %p): stub\n", hIMC, lpCompForm);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmSetConversionStatus(
  HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence)
{
  FIXME("(%p, %ld, %ld): stub\n",
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
    FIXME("Semi-Stub\n");
    if (data)
    {
        data->bOpen = fOpen;
        SendMessageW(data->hwnd, WM_IME_NOTIFY, IMN_SETOPENSTATUS, 0);
        return TRUE;
    }
    else
        return FALSE;
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
  FIXME("(%p, %ld): stub\n", hWnd, dwHotKeyID);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmUnregisterWordA (IMM32.@)
 */
BOOL WINAPI ImmUnregisterWordA(
  HKL hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszUnregister)
{
  FIXME("(%p, %s, %ld, %s): stub\n",
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
  FIXME("(%p, %s, %ld, %s): stub\n",
    hKL, debugstr_w(lpszReading), dwStyle, debugstr_w(lpszUnregister)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/*
 * The window proc for the default IME window
 */
static LRESULT WINAPI IME_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                          LPARAM lParam)
{
    TRACE("Incomming Message 0x%x  (0x%08x, 0x%08x)\n", uMsg, (UINT)wParam,
           (UINT)lParam);

    switch(uMsg)
    {
        case WM_NCCREATE:
            return TRUE;

        case WM_IME_COMPOSITION:
            TRACE("IME message %s, 0x%x, 0x%x\n",
                    "WM_IME_COMPOSITION", (UINT)wParam, (UINT)lParam);
            break;
        case WM_IME_STARTCOMPOSITION:
            TRACE("IME message %s, 0x%x, 0x%x\n",
                    "WM_IME_STARTCOMPOSITION", (UINT)wParam, (UINT)lParam);
            break;
        case WM_IME_ENDCOMPOSITION:
            TRACE("IME message %s, 0x%x, 0x%x\n",
                    "WM_IME_ENDCOMPOSITION", (UINT)wParam, (UINT)lParam);
            /* 
             *   if the string has not been read, then send it as
             *   WM_IME_CHAR messages
             */
            if (!root_context->bRead)
                IMM_PostResult(root_context);
            break;
        case WM_IME_SELECT:
            TRACE("IME message %s, 0x%x, 0x%x\n","WM_IME_SELECT",
                (UINT)wParam, (UINT)lParam);
            break;
    }
    return 0;
}
