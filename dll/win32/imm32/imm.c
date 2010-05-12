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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/debug.h"
#include "imm.h"
#include "ddk/imm.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

typedef struct tagIMCCInternal
{
    DWORD dwLock;
    DWORD dwSize;
} IMCCInternal;

#define MAKE_FUNCPTR(f) typeof(f) * p##f
typedef struct _tagImmHkl{
    struct list entry;
    HKL         hkl;
    HMODULE     hIME;
    IMEINFO     imeInfo;
    WCHAR       imeClassName[17]; /* 16 character max */
    ULONG       uSelected;

    /* Function Pointers */
    MAKE_FUNCPTR(ImeInquire);
    MAKE_FUNCPTR(ImeConfigure);
    MAKE_FUNCPTR(ImeDestroy);
    MAKE_FUNCPTR(ImeEscape);
    MAKE_FUNCPTR(ImeSelect);
    MAKE_FUNCPTR(ImeSetActiveContext);
    MAKE_FUNCPTR(ImeToAsciiEx);
    MAKE_FUNCPTR(NotifyIME);
    MAKE_FUNCPTR(ImeRegisterWord);
    MAKE_FUNCPTR(ImeUnregisterWord);
    MAKE_FUNCPTR(ImeEnumRegisterWord);
    MAKE_FUNCPTR(ImeSetCompositionString);
    MAKE_FUNCPTR(ImeConversionList);
    MAKE_FUNCPTR(ImeProcessKey);
    MAKE_FUNCPTR(ImeGetRegisterWordStyle);
    MAKE_FUNCPTR(ImeGetImeMenuItems);
} ImmHkl;
#undef MAKE_FUNCPTR

typedef struct tagInputContextData
{
        DWORD           dwLock;
        INPUTCONTEXT    IMC;

        ImmHkl          *immKbd;
        HWND            imeWnd;
        UINT            lastVK;
} InputContextData;

typedef struct _tagTRANSMSG {
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
} TRANSMSG, *LPTRANSMSG;

typedef struct _tagIMMThreadData {
    HIMC defaultContext;
    HWND hwndDefault;
} IMMThreadData;

static DWORD tlsIndex = 0;
static struct list ImmHklList = LIST_INIT(ImmHklList);

/* MSIME messages */
static UINT WM_MSIME_SERVICE;
static UINT WM_MSIME_RECONVERTOPTIONS;
static UINT WM_MSIME_MOUSE;
static UINT WM_MSIME_RECONVERTREQUEST;
static UINT WM_MSIME_RECONVERT;
static UINT WM_MSIME_QUERYPOSITION;
static UINT WM_MSIME_DOCUMENTFEED;

static const WCHAR szwWineIMCProperty[] = {'W','i','n','e','I','m','m','H','I','M','C','P','r','o','p','e','r','t','y',0};

static const WCHAR szImeFileW[] = {'I','m','e',' ','F','i','l','e',0};
static const WCHAR szLayoutTextW[] = {'L','a','y','o','u','t',' ','T','e','x','t',0};
static const WCHAR szImeRegFmt[] = {'S','y','s','t','e','m','\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\','C','o','n','t','r','o','l','\\','K','e','y','b','o','a','r','d',' ','L','a','y','o','u','t','s','\\','%','0','8','l','x',0};


#define is_himc_ime_unicode(p)  (p->immKbd->imeInfo.fdwProperty & IME_PROP_UNICODE)
#define is_kbd_ime_unicode(p)  (p->imeInfo.fdwProperty & IME_PROP_UNICODE)

static BOOL IMM_DestroyContext(HIMC hIMC);

static inline WCHAR *strdupAtoW( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

static inline CHAR *strdupWtoA( const WCHAR *str )
{
    CHAR *ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static DWORD convert_candidatelist_WtoA(
        LPCANDIDATELIST lpSrc, LPCANDIDATELIST lpDst, DWORD dwBufLen)
{
    DWORD ret, i, len;

    ret = FIELD_OFFSET( CANDIDATELIST, dwOffset[lpSrc->dwCount] );
    if ( lpDst && dwBufLen > 0 )
    {
        *lpDst = *lpSrc;
        lpDst->dwOffset[0] = ret;
    }

    for ( i = 0; i < lpSrc->dwCount; i++)
    {
        LPBYTE src = (LPBYTE)lpSrc + lpSrc->dwOffset[i];

        if ( lpDst && dwBufLen > 0 )
        {
            LPBYTE dest = (LPBYTE)lpDst + lpDst->dwOffset[i];

            len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)src, -1,
                                      (LPSTR)dest, dwBufLen, NULL, NULL);

            if ( i + 1 < lpSrc->dwCount )
                lpDst->dwOffset[i+1] = lpDst->dwOffset[i] + len * sizeof(char);
            dwBufLen -= len * sizeof(char);
        }
        else
            len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)src, -1, NULL, 0, NULL, NULL);

        ret += len * sizeof(char);
    }

    if ( lpDst )
        lpDst->dwSize = ret;

    return ret;
}

static DWORD convert_candidatelist_AtoW(
        LPCANDIDATELIST lpSrc, LPCANDIDATELIST lpDst, DWORD dwBufLen)
{
    DWORD ret, i, len;

    ret = FIELD_OFFSET( CANDIDATELIST, dwOffset[lpSrc->dwCount] );
    if ( lpDst && dwBufLen > 0 )
    {
        *lpDst = *lpSrc;
        lpDst->dwOffset[0] = ret;
    }

    for ( i = 0; i < lpSrc->dwCount; i++)
    {
        LPBYTE src = (LPBYTE)lpSrc + lpSrc->dwOffset[i];

        if ( lpDst && dwBufLen > 0 )
        {
            LPBYTE dest = (LPBYTE)lpDst + lpDst->dwOffset[i];

            len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)src, -1,
                                      (LPWSTR)dest, dwBufLen);

            if ( i + 1 < lpSrc->dwCount )
                lpDst->dwOffset[i+1] = lpDst->dwOffset[i] + len * sizeof(WCHAR);
            dwBufLen -= len * sizeof(WCHAR);
        }
        else
            len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)src, -1, NULL, 0);

        ret += len * sizeof(WCHAR);
    }

    if ( lpDst )
        lpDst->dwSize = ret;

    return ret;
}

static IMMThreadData* IMM_GetThreadData(void)
{
    IMMThreadData* data = TlsGetValue(tlsIndex);
    if (!data)
    {
        data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                         sizeof(IMMThreadData));
        TlsSetValue(tlsIndex,data);
        TRACE("Thread Data Created\n");
    }
    return data;
}

static void IMM_FreeThreadData(void)
{
    IMMThreadData* data = TlsGetValue(tlsIndex);
    if (data)
    {
        IMM_DestroyContext(data->defaultContext);
        DestroyWindow(data->hwndDefault);
        HeapFree(GetProcessHeap(),0,data);
        TRACE("Thread Data Destroyed\n");
    }
}

static HMODULE LoadDefaultWineIME(void)
{
    char buffer[MAX_PATH], libname[32], *name, *next;
    HMODULE module = 0;
    HKEY hkey;

    TRACE("Attempting to fall back to wine default IME\n");

    strcpy( buffer, "x11" );  /* default value */
    /* @@ Wine registry key: HKCU\Software\Wine\Drivers */
    if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Drivers", &hkey ))
    {
        DWORD type, count = sizeof(buffer);
        RegQueryValueExA( hkey, "Ime", 0, &type, (LPBYTE) buffer, &count );
        RegCloseKey( hkey );
    }

    name = buffer;
    while (name)
    {
        next = strchr( name, ',' );
        if (next) *next++ = 0;

        snprintf( libname, sizeof(libname), "wine%s.drv", name );
        if ((module = LoadLibraryA( libname )) != 0) break;
        name = next;
    }

    return module;
}

/* ImmHkl loading and freeing */
#define LOAD_FUNCPTR(f) if((ptr->p##f = (LPVOID)GetProcAddress(ptr->hIME, #f)) == NULL){WARN("Can't find function %s in ime\n", #f);}
static ImmHkl *IMM_GetImmHkl(HKL hkl)
{
    ImmHkl *ptr;
    WCHAR filename[MAX_PATH];

    TRACE("Seeking ime for keyboard %p\n",hkl);

    LIST_FOR_EACH_ENTRY(ptr, &ImmHklList, ImmHkl, entry)
    {
        if (ptr->hkl == hkl)
            return ptr;
    }
    /* not found... create it */

    ptr = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ImmHkl));

    ptr->hkl = hkl;
    if (ImmGetIMEFileNameW(hkl, filename, MAX_PATH)) ptr->hIME = LoadLibraryW(filename);
    if (!ptr->hIME)
        ptr->hIME = LoadDefaultWineIME();
    if (ptr->hIME)
    {
        LOAD_FUNCPTR(ImeInquire);
        if (!ptr->pImeInquire || !ptr->pImeInquire(&ptr->imeInfo, ptr->imeClassName, NULL))
        {
            FreeLibrary(ptr->hIME);
            ptr->hIME = NULL;
        }
        else
        {
            LOAD_FUNCPTR(ImeDestroy);
            LOAD_FUNCPTR(ImeSelect);
            if (!ptr->pImeSelect || !ptr->pImeDestroy)
            {
                FreeLibrary(ptr->hIME);
                ptr->hIME = NULL;
            }
            else
            {
                LOAD_FUNCPTR(ImeConfigure);
                LOAD_FUNCPTR(ImeEscape);
                LOAD_FUNCPTR(ImeSetActiveContext);
                LOAD_FUNCPTR(ImeToAsciiEx);
                LOAD_FUNCPTR(NotifyIME);
                LOAD_FUNCPTR(ImeRegisterWord);
                LOAD_FUNCPTR(ImeUnregisterWord);
                LOAD_FUNCPTR(ImeEnumRegisterWord);
                LOAD_FUNCPTR(ImeSetCompositionString);
                LOAD_FUNCPTR(ImeConversionList);
                LOAD_FUNCPTR(ImeProcessKey);
                LOAD_FUNCPTR(ImeGetRegisterWordStyle);
                LOAD_FUNCPTR(ImeGetImeMenuItems);
                /* make sure our classname is WCHAR */
                if (!is_kbd_ime_unicode(ptr))
                {
                    WCHAR bufW[17];
                    MultiByteToWideChar(CP_ACP, 0, (LPSTR)ptr->imeClassName,
                                        -1, bufW, 17);
                    lstrcpyW(ptr->imeClassName, bufW);
                }
            }
        }
    }
    list_add_head(&ImmHklList,&ptr->entry);

    return ptr;
}
#undef LOAD_FUNCPTR

static void IMM_FreeAllImmHkl(void)
{
    ImmHkl *ptr,*cursor2;

    LIST_FOR_EACH_ENTRY_SAFE(ptr, cursor2, &ImmHklList, ImmHkl, entry)
    {
        list_remove(&ptr->entry);
        if (ptr->hIME)
        {
            ptr->pImeDestroy(1);
            FreeLibrary(ptr->hIME);
        }
        HeapFree(GetProcessHeap(),0,ptr);
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
    TRACE("%p, %x, %p\n",hInstDLL,fdwReason,lpReserved);
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            IMM_RegisterMessages();
            tlsIndex = TlsAlloc();
            if (tlsIndex == TLS_OUT_OF_INDEXES)
                return FALSE;
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            IMM_FreeThreadData();
            break;
        case DLL_PROCESS_DETACH:
            IMM_FreeThreadData();
            IMM_FreeAllImmHkl();
            TlsFree(tlsIndex);
            break;
    }
    return TRUE;
}

/* for posting messages as the IME */
static void ImmInternalPostIMEMessage(InputContextData *data, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND target = GetFocus();
    if (!target)
       PostMessageW(data->IMC.hWnd,msg,wParam,lParam);
    else
       PostMessageW(target, msg, wParam, lParam);
}

static LRESULT ImmInternalSendIMENotify(InputContextData *data, WPARAM notify, LPARAM lParam)
{
    HWND target;

    target = data->IMC.hWnd;
    if (!target) target = GetFocus();

    if (target)
       return SendMessageW(target, WM_IME_NOTIFY, notify, lParam);

    return 0;
}

static HIMCC ImmCreateBlankCompStr(void)
{
    HIMCC rc;
    LPCOMPOSITIONSTRING ptr;
    rc = ImmCreateIMCC(sizeof(COMPOSITIONSTRING));
    ptr = ImmLockIMCC(rc);
    memset(ptr,0,sizeof(COMPOSITIONSTRING));
    ptr->dwSize = sizeof(COMPOSITIONSTRING);
    ImmUnlockIMCC(rc);
    return rc;
}

/***********************************************************************
 *		ImmAssociateContext (IMM32.@)
 */
HIMC WINAPI ImmAssociateContext(HWND hWnd, HIMC hIMC)
{
    HIMC old = NULL;
    InputContextData *data = hIMC;

    TRACE("(%p, %p):\n", hWnd, hIMC);

    if (!IMM_GetThreadData()->defaultContext)
        IMM_GetThreadData()->defaultContext = ImmCreateContext();

    /*
     * If already associated just return
     */
    if (hIMC && data->IMC.hWnd == hWnd)
        return hIMC;

    if (hWnd)
    {
        old = RemovePropW(hWnd,szwWineIMCProperty);

        if (old == NULL)
            old = IMM_GetThreadData()->defaultContext;
        else if (old == (HIMC)-1)
            old = NULL;

        if (hIMC != IMM_GetThreadData()->defaultContext)
        {
            if (hIMC == NULL) /* Meaning disable imm for that window*/
                SetPropW(hWnd,szwWineIMCProperty,(HANDLE)-1);
            else
                SetPropW(hWnd,szwWineIMCProperty,hIMC);
        }

        if (old)
        {
            InputContextData *old_data = old;
            if (old_data->IMC.hWnd == hWnd)
                old_data->IMC.hWnd = NULL;
        }
    }

    if (!hIMC)
        return old;

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

    return old;
}


/*
 * Helper function for ImmAssociateContextEx
 */
static BOOL CALLBACK _ImmAssociateContextExEnumProc(HWND hwnd, LPARAM lParam)
{
    HIMC hImc = (HIMC)lParam;
    ImmAssociateContext(hwnd,hImc);
    return TRUE;
}

/***********************************************************************
 *              ImmAssociateContextEx (IMM32.@)
 */
BOOL WINAPI ImmAssociateContextEx(HWND hWnd, HIMC hIMC, DWORD dwFlags)
{
    TRACE("(%p, %p, %d): stub\n", hWnd, hIMC, dwFlags);

    if (!IMM_GetThreadData()->defaultContext)
        IMM_GetThreadData()->defaultContext = ImmCreateContext();

    if (dwFlags == IACE_DEFAULT)
    {
        ImmAssociateContext(hWnd,IMM_GetThreadData()->defaultContext);
        return TRUE;
    }
    else if (dwFlags == IACE_IGNORENOCONTEXT)
    {
        if (GetPropW(hWnd,szwWineIMCProperty))
            ImmAssociateContext(hWnd,hIMC);
        return TRUE;
    }
    else if (dwFlags == IACE_CHILDREN)
    {
        EnumChildWindows(hWnd,_ImmAssociateContextExEnumProc,(LPARAM)hIMC);
        return TRUE;
    }
    else
    {
        ERR("Unknown dwFlags 0x%x\n",dwFlags);
        return FALSE;
    }
}

/***********************************************************************
 *		ImmConfigureIMEA (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEA(
  HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);

    TRACE("(%p, %p, %d, %p):\n", hKL, hWnd, dwMode, lpData);

    if (dwMode == IME_CONFIG_REGISTERWORD && !lpData)
        return FALSE;

    if (immHkl->hIME && immHkl->pImeConfigure)
    {
        if (dwMode != IME_CONFIG_REGISTERWORD || !is_kbd_ime_unicode(immHkl))
            return immHkl->pImeConfigure(hKL,hWnd,dwMode,lpData);
        else
        {
            REGISTERWORDW rww;
            REGISTERWORDA *rwa = lpData;
            BOOL rc;

            rww.lpReading = strdupAtoW(rwa->lpReading);
            rww.lpWord = strdupAtoW(rwa->lpWord);
            rc = immHkl->pImeConfigure(hKL,hWnd,dwMode,&rww);
            HeapFree(GetProcessHeap(),0,rww.lpReading);
            HeapFree(GetProcessHeap(),0,rww.lpWord);
            return rc;
        }
    }
    else
        return FALSE;
}

/***********************************************************************
 *		ImmConfigureIMEW (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEW(
  HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);

    TRACE("(%p, %p, %d, %p):\n", hKL, hWnd, dwMode, lpData);

    if (dwMode == IME_CONFIG_REGISTERWORD && !lpData)
        return FALSE;

    if (immHkl->hIME && immHkl->pImeConfigure)
    {
        if (dwMode != IME_CONFIG_REGISTERWORD || is_kbd_ime_unicode(immHkl))
            return immHkl->pImeConfigure(hKL,hWnd,dwMode,lpData);
        else
        {
            REGISTERWORDW *rww = lpData;
            REGISTERWORDA rwa;
            BOOL rc;

            rwa.lpReading = strdupWtoA(rww->lpReading);
            rwa.lpWord = strdupWtoA(rww->lpWord);
            rc = immHkl->pImeConfigure(hKL,hWnd,dwMode,&rwa);
            HeapFree(GetProcessHeap(),0,rwa.lpReading);
            HeapFree(GetProcessHeap(),0,rwa.lpWord);
            return rc;
        }
    }
    else
        return FALSE;
}

/***********************************************************************
 *		ImmCreateContext (IMM32.@)
 */
HIMC WINAPI ImmCreateContext(void)
{
    InputContextData *new_context;
    LPGUIDELINE gl;
    LPCANDIDATEINFO ci;

    new_context = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(InputContextData));

    /* Load the IME */
    new_context->immKbd = IMM_GetImmHkl(GetKeyboardLayout(0));

    if (!new_context->immKbd->hIME)
    {
        TRACE("IME dll could not be loaded\n");
        HeapFree(GetProcessHeap(),0,new_context);
        return 0;
    }

    /* the HIMCCs are never NULL */
    new_context->IMC.hCompStr = ImmCreateBlankCompStr();
    new_context->IMC.hMsgBuf = ImmCreateIMCC(0);
    new_context->IMC.hCandInfo = ImmCreateIMCC(sizeof(CANDIDATEINFO));
    ci = ImmLockIMCC(new_context->IMC.hCandInfo);
    memset(ci,0,sizeof(CANDIDATEINFO));
    ci->dwSize = sizeof(CANDIDATEINFO);
    ImmUnlockIMCC(new_context->IMC.hCandInfo);
    new_context->IMC.hGuideLine = ImmCreateIMCC(sizeof(GUIDELINE));
    gl = ImmLockIMCC(new_context->IMC.hGuideLine);
    memset(gl,0,sizeof(GUIDELINE));
    gl->dwSize = sizeof(GUIDELINE);
    ImmUnlockIMCC(new_context->IMC.hGuideLine);

    /* Initialize the IME Private */
    new_context->IMC.hPrivate = ImmCreateIMCC(new_context->immKbd->imeInfo.dwPrivateDataSize);

    if (!new_context->immKbd->pImeSelect(new_context, TRUE))
    {
        TRACE("Selection of IME failed\n");
        IMM_DestroyContext(new_context);
        return 0;
    }
    SendMessageW(GetFocus(), WM_IME_SELECT, TRUE, (LPARAM)GetKeyboardLayout(0));

    new_context->immKbd->uSelected++;
    TRACE("Created context %p\n",new_context);

    return new_context;
}

static BOOL IMM_DestroyContext(HIMC hIMC)
{
    InputContextData *data = hIMC;

    TRACE("Destroying %p\n",hIMC);

    if (hIMC)
    {
        data->immKbd->uSelected --;
        data->immKbd->pImeSelect(hIMC, FALSE);
        SendMessageW(data->IMC.hWnd, WM_IME_SELECT, FALSE, (LPARAM)GetKeyboardLayout(0));

        if (IMM_GetThreadData()->hwndDefault == data->imeWnd)
            IMM_GetThreadData()->hwndDefault = NULL;
        DestroyWindow(data->imeWnd);

        ImmDestroyIMCC(data->IMC.hCompStr);
        ImmDestroyIMCC(data->IMC.hCandInfo);
        ImmDestroyIMCC(data->IMC.hGuideLine);
        ImmDestroyIMCC(data->IMC.hPrivate);
        ImmDestroyIMCC(data->IMC.hMsgBuf);

        HeapFree(GetProcessHeap(),0,data);
    }
    return TRUE;
}

/***********************************************************************
 *		ImmDestroyContext (IMM32.@)
 */
BOOL WINAPI ImmDestroyContext(HIMC hIMC)
{
    if (hIMC != IMM_GetThreadData()->defaultContext)
        return IMM_DestroyContext(hIMC);
    else
        return FALSE;
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
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %s, %d, %s, %p):\n", hKL, lpfnEnumProc,
        debugstr_a(lpszReading), dwStyle, debugstr_a(lpszRegister), lpData);
    if (immHkl->hIME && immHkl->pImeEnumRegisterWord)
    {
        if (!is_kbd_ime_unicode(immHkl))
            return immHkl->pImeEnumRegisterWord((REGISTERWORDENUMPROCW)lpfnEnumProc,
                (LPCWSTR)lpszReading, dwStyle, (LPCWSTR)lpszRegister, lpData);
        else
        {
            LPWSTR lpszwReading = strdupAtoW(lpszReading);
            LPWSTR lpszwRegister = strdupAtoW(lpszRegister);
            BOOL rc;

            rc = immHkl->pImeEnumRegisterWord((REGISTERWORDENUMPROCW)lpfnEnumProc,
                                              lpszwReading, dwStyle, lpszwRegister,
                                              lpData);

            HeapFree(GetProcessHeap(),0,lpszwReading);
            HeapFree(GetProcessHeap(),0,lpszwRegister);
            return rc;
        }
    }
    else
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
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %s, %d, %s, %p):\n", hKL, lpfnEnumProc,
        debugstr_w(lpszReading), dwStyle, debugstr_w(lpszRegister), lpData);
    if (immHkl->hIME && immHkl->pImeEnumRegisterWord)
    {
        if (is_kbd_ime_unicode(immHkl))
            return immHkl->pImeEnumRegisterWord(lpfnEnumProc, lpszReading, dwStyle,
                                            lpszRegister, lpData);
        else
        {
            LPSTR lpszaReading = strdupWtoA(lpszReading);
            LPSTR lpszaRegister = strdupWtoA(lpszRegister);
            BOOL rc;

            rc = immHkl->pImeEnumRegisterWord(lpfnEnumProc, (LPCWSTR)lpszaReading,
                                              dwStyle, (LPCWSTR)lpszaRegister, lpData);

            HeapFree(GetProcessHeap(),0,lpszaReading);
            HeapFree(GetProcessHeap(),0,lpszaRegister);
            return rc;
        }
    }
    else
        return 0;
}

static inline BOOL EscapeRequiresWA(UINT uEscape)
{
        if (uEscape == IME_ESC_GET_EUDC_DICTIONARY ||
            uEscape == IME_ESC_SET_EUDC_DICTIONARY ||
            uEscape == IME_ESC_IME_NAME ||
            uEscape == IME_ESC_GETHELPFILENAME)
        return TRUE;
    return FALSE;
}

/***********************************************************************
 *		ImmEscapeA (IMM32.@)
 */
LRESULT WINAPI ImmEscapeA(
  HKL hKL, HIMC hIMC,
  UINT uEscape, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %d, %p):\n", hKL, hIMC, uEscape, lpData);

    if (immHkl->hIME && immHkl->pImeEscape)
    {
        if (!EscapeRequiresWA(uEscape) || !is_kbd_ime_unicode(immHkl))
            return immHkl->pImeEscape(hIMC,uEscape,lpData);
        else
        {
            WCHAR buffer[81]; /* largest required buffer should be 80 */
            LRESULT rc;
            if (uEscape == IME_ESC_SET_EUDC_DICTIONARY)
            {
                MultiByteToWideChar(CP_ACP,0,lpData,-1,buffer,81);
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
            }
            else
            {
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
                WideCharToMultiByte(CP_ACP,0,buffer,-1,lpData,80, NULL, NULL);
            }
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmEscapeW (IMM32.@)
 */
LRESULT WINAPI ImmEscapeW(
  HKL hKL, HIMC hIMC,
  UINT uEscape, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %d, %p):\n", hKL, hIMC, uEscape, lpData);

    if (immHkl->hIME && immHkl->pImeEscape)
    {
        if (!EscapeRequiresWA(uEscape) || is_kbd_ime_unicode(immHkl))
            return immHkl->pImeEscape(hIMC,uEscape,lpData);
        else
        {
            CHAR buffer[81]; /* largest required buffer should be 80 */
            LRESULT rc;
            if (uEscape == IME_ESC_SET_EUDC_DICTIONARY)
            {
                WideCharToMultiByte(CP_ACP,0,lpData,-1,buffer,81, NULL, NULL);
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
            }
            else
            {
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
                MultiByteToWideChar(CP_ACP,0,buffer,-1,lpData,80);
            }
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmGetCandidateListA (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListA(
  HIMC hIMC, DWORD dwIndex,
  LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
    InputContextData *data = hIMC;
    LPCANDIDATEINFO candinfo;
    LPCANDIDATELIST candlist;
    DWORD ret = 0;

    TRACE("%p, %d, %p, %d\n", hIMC, dwIndex, lpCandList, dwBufLen);

    if (!data || !data->IMC.hCandInfo)
       return 0;

    candinfo = ImmLockIMCC(data->IMC.hCandInfo);
    if ( dwIndex >= candinfo->dwCount ||
         dwIndex >= (sizeof(candinfo->dwOffset) / sizeof(DWORD)) )
        goto done;

    candlist = (LPCANDIDATELIST)((LPBYTE)candinfo + candinfo->dwOffset[dwIndex]);
    if ( !candlist->dwSize || !candlist->dwCount )
        goto done;

    if ( !is_himc_ime_unicode(data) )
    {
        ret = candlist->dwSize;
        if ( lpCandList && dwBufLen >= ret )
            memcpy(lpCandList, candlist, ret);
    }
    else
        ret = convert_candidatelist_WtoA( candlist, lpCandList, dwBufLen);

done:
    ImmUnlockIMCC(data->IMC.hCandInfo);
    return ret;
}

/***********************************************************************
 *		ImmGetCandidateListCountA (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListCountA(
  HIMC hIMC, LPDWORD lpdwListCount)
{
    InputContextData *data = hIMC;
    LPCANDIDATEINFO candinfo;
    DWORD ret, count;

    TRACE("%p, %p\n", hIMC, lpdwListCount);

    if (!data || !lpdwListCount || !data->IMC.hCandInfo)
       return 0;

    candinfo = ImmLockIMCC(data->IMC.hCandInfo);

    *lpdwListCount = count = candinfo->dwCount;

    if ( !is_himc_ime_unicode(data) )
        ret = candinfo->dwSize;
    else
    {
        ret = sizeof(CANDIDATEINFO);
        while ( count-- )
            ret += ImmGetCandidateListA(hIMC, count, NULL, 0);
    }

    ImmUnlockIMCC(data->IMC.hCandInfo);
    return ret;
}

/***********************************************************************
 *		ImmGetCandidateListCountW (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListCountW(
  HIMC hIMC, LPDWORD lpdwListCount)
{
    InputContextData *data = hIMC;
    LPCANDIDATEINFO candinfo;
    DWORD ret, count;

    TRACE("%p, %p\n", hIMC, lpdwListCount);

    if (!data || !lpdwListCount || !data->IMC.hCandInfo)
       return 0;

    candinfo = ImmLockIMCC(data->IMC.hCandInfo);

    *lpdwListCount = count = candinfo->dwCount;

    if ( is_himc_ime_unicode(data) )
        ret = candinfo->dwSize;
    else
    {
        ret = sizeof(CANDIDATEINFO);
        while ( count-- )
            ret += ImmGetCandidateListW(hIMC, count, NULL, 0);
    }

    ImmUnlockIMCC(data->IMC.hCandInfo);
    return ret;
}

/***********************************************************************
 *		ImmGetCandidateListW (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListW(
  HIMC hIMC, DWORD dwIndex,
  LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
    InputContextData *data = hIMC;
    LPCANDIDATEINFO candinfo;
    LPCANDIDATELIST candlist;
    DWORD ret = 0;

    TRACE("%p, %d, %p, %d\n", hIMC, dwIndex, lpCandList, dwBufLen);

    if (!data || !data->IMC.hCandInfo)
       return 0;

    candinfo = ImmLockIMCC(data->IMC.hCandInfo);
    if ( dwIndex >= candinfo->dwCount ||
         dwIndex >= (sizeof(candinfo->dwOffset) / sizeof(DWORD)) )
        goto done;

    candlist = (LPCANDIDATELIST)((LPBYTE)candinfo + candinfo->dwOffset[dwIndex]);
    if ( !candlist->dwSize || !candlist->dwCount )
        goto done;

    if ( is_himc_ime_unicode(data) )
    {
        ret = candlist->dwSize;
        if ( lpCandList && dwBufLen >= ret )
            memcpy(lpCandList, candlist, ret);
    }
    else
        ret = convert_candidatelist_AtoW( candlist, lpCandList, dwBufLen);

done:
    ImmUnlockIMCC(data->IMC.hCandInfo);
    return ret;
}

/***********************************************************************
 *		ImmGetCandidateWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCandidateWindow(
  HIMC hIMC, DWORD dwIndex, LPCANDIDATEFORM lpCandidate)
{
    InputContextData *data = hIMC;

    TRACE("%p, %d, %p\n", hIMC, dwIndex, lpCandidate);

    if (!data || !lpCandidate)
        return FALSE;

    if ( dwIndex >= (sizeof(data->IMC.cfCandForm) / sizeof(CANDIDATEFORM)) )
        return FALSE;

    *lpCandidate = data->IMC.cfCandForm[dwIndex];

    return TRUE;
}

/***********************************************************************
 *		ImmGetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
    LOGFONTW lfW;
    BOOL rc;

    TRACE("(%p, %p):\n", hIMC, lplf);

    rc = ImmGetCompositionFontW(hIMC,&lfW);
    if (!rc || !lplf)
        return FALSE;

    memcpy(lplf,&lfW,sizeof(LOGFONTA));
    WideCharToMultiByte(CP_ACP, 0, lfW.lfFaceName, -1, lplf->lfFaceName,
                        LF_FACESIZE, NULL, NULL);
    return TRUE;
}

/***********************************************************************
 *		ImmGetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
    InputContextData *data = hIMC;

    TRACE("(%p, %p):\n", hIMC, lplf);

    if (!data || !lplf)
        return FALSE;

    *lplf = data->IMC.lfFont.W;

    return TRUE;
}


/* Helpers for the GetCompositionString functions */

static INT CopyCompStringIMEtoClient(InputContextData *data, LPBYTE source, INT slen, LPBYTE target, INT tlen,
                                     BOOL unicode )
{
    INT rc;

    if (is_himc_ime_unicode(data) && !unicode)
        rc = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)source, slen, (LPSTR)target, tlen, NULL, NULL);
    else if (!is_himc_ime_unicode(data) && unicode)
        rc = MultiByteToWideChar(CP_ACP, 0, (LPSTR)source, slen, (LPWSTR)target, tlen) * sizeof(WCHAR);
    else
    {
        int dlen = (unicode)?sizeof(WCHAR):sizeof(CHAR);
        memcpy( target, source, min(slen,tlen)*dlen);
        rc = slen*dlen;
    }

    return rc;
}

static INT CopyCompAttrIMEtoClient(InputContextData *data, LPBYTE source, INT slen, LPBYTE ssource, INT sslen,
                                   LPBYTE target, INT tlen, BOOL unicode )
{
    INT rc;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        rc = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)ssource, sslen, NULL, 0, NULL, NULL);
        if (tlen)
        {
            const BYTE *src = source;
            LPBYTE dst = target;
            int i, j = 0, k = 0;

            if (rc < tlen)
                tlen = rc;
            for (i = 0; i < sslen; ++i)
            {
                int len;

                len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)ssource + i, 1,
                                          NULL, 0, NULL, NULL);
                for (; len > 0; --len)
                {
                    dst[j++] = src[k];

                    if (j >= tlen)
                        goto end;
                }
                ++k;
            }
        end:
            rc = j;
        }
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        rc = MultiByteToWideChar(CP_ACP, 0, (LPSTR)ssource, sslen, NULL, 0);
        if (tlen)
        {
            const BYTE *src = source;
            LPBYTE dst = target;
            int i, j = 0;

            if (rc < tlen)
                tlen = rc;
            for (i = 0; i < sslen; ++i)
            {
                if (IsDBCSLeadByte(((LPSTR)ssource)[i]))
                    continue;

                dst[j++] = src[i];

                if (j >= tlen)
                    break;
            }
            rc = j;
        }
    }
    else
    {
        memcpy( target, source, min(slen,tlen));
        rc = slen;
    }

    return rc;
}

static INT CopyCompClauseIMEtoClient(InputContextData *data, LPBYTE source, INT slen, LPBYTE ssource, INT sslen,
                                     LPBYTE target, INT tlen, BOOL unicode )
{
    INT rc;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        if (tlen)
        {
            int i;

            if (slen < tlen)
                tlen = slen;
            tlen /= sizeof (DWORD);
            for (i = 0; i < tlen; ++i)
            {
                ((DWORD *)target)[i] = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)ssource,
                                                          ((DWORD *)source)[i],
                                                          NULL, 0,
                                                          NULL, NULL);
            }
            rc = sizeof (DWORD) * i;
        }
        else
            rc = slen;
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        if (tlen)
        {
            int i;

            if (slen < tlen)
                tlen = slen;
            tlen /= sizeof (DWORD);
            for (i = 0; i < tlen; ++i)
            {
                ((DWORD *)target)[i] = MultiByteToWideChar(CP_ACP, 0, (LPSTR)ssource,
                                                          ((DWORD *)source)[i],
                                                          NULL, 0);
            }
            rc = sizeof (DWORD) * i;
        }
        else
            rc = slen;
    }
    else
    {
        memcpy( target, source, min(slen,tlen));
        rc = slen;
    }

    return rc;
}

static INT CopyCompOffsetIMEtoClient(InputContextData *data, DWORD offset, LPBYTE ssource, BOOL unicode)
{
    int rc;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        rc = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)ssource, offset, NULL, 0, NULL, NULL);
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        rc = MultiByteToWideChar(CP_ACP, 0, (LPSTR)ssource, offset, NULL, 0);
    }
    else
        rc = offset;

    return rc;
}

static LONG ImmGetCompositionStringT( HIMC hIMC, DWORD dwIndex, LPVOID lpBuf,
                                      DWORD dwBufLen, BOOL unicode)
{
    LONG rc = 0;
    InputContextData *data = hIMC;
    LPCOMPOSITIONSTRING compstr;
    LPBYTE compdata;

    TRACE("(%p, 0x%x, %p, %d)\n", hIMC, dwIndex, lpBuf, dwBufLen);

    if (!data)
       return FALSE;

    if (!data->IMC.hCompStr)
       return FALSE;

    compdata = ImmLockIMCC(data->IMC.hCompStr);
    compstr = (LPCOMPOSITIONSTRING)compdata;

    switch (dwIndex)
    {
    case GCS_RESULTSTR:
        TRACE("GCS_RESULTSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwResultStrOffset, compstr->dwResultStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPSTR:
        TRACE("GCS_COMPSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPATTR:
        TRACE("GCS_COMPATTR\n");
        rc = CopyCompAttrIMEtoClient(data, compdata + compstr->dwCompAttrOffset, compstr->dwCompAttrLen,
                                     compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen,
                                     lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPCLAUSE:
        TRACE("GCS_COMPCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwCompClauseOffset,compstr->dwCompClauseLen,
                                       compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTCLAUSE:
        TRACE("GCS_RESULTCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwResultClauseOffset,compstr->dwResultClauseLen,
                                       compdata + compstr->dwResultStrOffset, compstr->dwResultStrLen,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTREADSTR:
        TRACE("GCS_RESULTREADSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwResultReadStrOffset, compstr->dwResultReadStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTREADCLAUSE:
        TRACE("GCS_RESULTREADCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwResultReadClauseOffset,compstr->dwResultReadClauseLen,
                                       compdata + compstr->dwResultStrOffset, compstr->dwResultStrLen,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADSTR:
        TRACE("GCS_COMPREADSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwCompReadStrOffset, compstr->dwCompReadStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADATTR:
        TRACE("GCS_COMPREADATTR\n");
        rc = CopyCompAttrIMEtoClient(data, compdata + compstr->dwCompReadAttrOffset, compstr->dwCompReadAttrLen,
                                     compdata + compstr->dwCompReadStrOffset, compstr->dwCompReadStrLen,
                                     lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADCLAUSE:
        TRACE("GCS_COMPREADCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwCompReadClauseOffset,compstr->dwCompReadClauseLen,
                                       compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_CURSORPOS:
        TRACE("GCS_CURSORPOS\n");
        rc = CopyCompOffsetIMEtoClient(data, compstr->dwCursorPos, compdata + compstr->dwCompStrOffset, unicode);
        break;
    case GCS_DELTASTART:
        TRACE("GCS_DELTASTART\n");
        rc = CopyCompOffsetIMEtoClient(data, compstr->dwDeltaStart, compdata + compstr->dwCompStrOffset, unicode);
        break;
    default:
        FIXME("Unhandled index 0x%x\n",dwIndex);
        break;
    }

    ImmUnlockIMCC(data->IMC.hCompStr);

    return rc;
}

/***********************************************************************
 *		ImmGetCompositionStringA (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringA(
  HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    return ImmGetCompositionStringT(hIMC, dwIndex, lpBuf, dwBufLen, FALSE);
}


/***********************************************************************
 *		ImmGetCompositionStringW (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringW(
  HIMC hIMC, DWORD dwIndex,
  LPVOID lpBuf, DWORD dwBufLen)
{
    return ImmGetCompositionStringT(hIMC, dwIndex, lpBuf, dwBufLen, TRUE);
}

/***********************************************************************
 *		ImmGetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionWindow(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
    InputContextData *data = hIMC;

    TRACE("(%p, %p)\n", hIMC, lpCompForm);

    if (!data)
        return FALSE;

    *lpCompForm = data->IMC.cfCompForm;
    return 1;
}

/***********************************************************************
 *		ImmGetContext (IMM32.@)
 *
 */
HIMC WINAPI ImmGetContext(HWND hWnd)
{
    HIMC rc = NULL;

    TRACE("%p\n", hWnd);
    if (!IMM_GetThreadData()->defaultContext)
        IMM_GetThreadData()->defaultContext = ImmCreateContext();

    rc = GetPropW(hWnd,szwWineIMCProperty);
    if (rc == (HIMC)-1)
        rc = NULL;
    else if (rc == NULL)
        rc = IMM_GetThreadData()->defaultContext;

    if (rc)
    {
        InputContextData *data = rc;
        data->IMC.hWnd = hWnd;
    }
    TRACE("returning %p\n", rc);

    return rc;
}

/***********************************************************************
 *		ImmGetConversionListA (IMM32.@)
 */
DWORD WINAPI ImmGetConversionListA(
  HKL hKL, HIMC hIMC,
  LPCSTR pSrc, LPCANDIDATELIST lpDst,
  DWORD dwBufLen, UINT uFlag)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %s, %p, %d, %d):\n", hKL, hIMC, debugstr_a(pSrc), lpDst,
                dwBufLen, uFlag);
    if (immHkl->hIME && immHkl->pImeConversionList)
    {
        if (!is_kbd_ime_unicode(immHkl))
            return immHkl->pImeConversionList(hIMC,(LPCWSTR)pSrc,lpDst,dwBufLen,uFlag);
        else
        {
            LPCANDIDATELIST lpwDst;
            DWORD ret = 0, len;
            LPWSTR pwSrc = strdupAtoW(pSrc);

            len = immHkl->pImeConversionList(hIMC, pwSrc, NULL, 0, uFlag);
            lpwDst = HeapAlloc(GetProcessHeap(), 0, len);
            if ( lpwDst )
            {
                immHkl->pImeConversionList(hIMC, pwSrc, lpwDst, len, uFlag);
                ret = convert_candidatelist_WtoA( lpwDst, lpDst, dwBufLen);
                HeapFree(GetProcessHeap(), 0, lpwDst);
            }
            HeapFree(GetProcessHeap(), 0, pwSrc);

            return ret;
        }
    }
    else
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
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %s, %p, %d, %d):\n", hKL, hIMC, debugstr_w(pSrc), lpDst,
                dwBufLen, uFlag);
    if (immHkl->hIME && immHkl->pImeConversionList)
    {
        if (is_kbd_ime_unicode(immHkl))
            return immHkl->pImeConversionList(hIMC,pSrc,lpDst,dwBufLen,uFlag);
        else
        {
            LPCANDIDATELIST lpaDst;
            DWORD ret = 0, len;
            LPSTR paSrc = strdupWtoA(pSrc);

            len = immHkl->pImeConversionList(hIMC, (LPCWSTR)paSrc, NULL, 0, uFlag);
            lpaDst = HeapAlloc(GetProcessHeap(), 0, len);
            if ( lpaDst )
            {
                immHkl->pImeConversionList(hIMC, (LPCWSTR)paSrc, lpaDst, len, uFlag);
                ret = convert_candidatelist_AtoW( lpaDst, lpDst, dwBufLen);
                HeapFree(GetProcessHeap(), 0, lpaDst);
            }
            HeapFree(GetProcessHeap(), 0, paSrc);

            return ret;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmGetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmGetConversionStatus(
  HIMC hIMC, LPDWORD lpfdwConversion, LPDWORD lpfdwSentence)
{
    InputContextData *data = hIMC;

    TRACE("%p %p %p\n", hIMC, lpfdwConversion, lpfdwSentence);

    if (!data)
        return FALSE;

    if (lpfdwConversion)
        *lpfdwConversion = data->IMC.fdwConversion;
    if (lpfdwSentence)
        *lpfdwSentence = data->IMC.fdwSentence;

    return TRUE;
}

/***********************************************************************
 *		ImmGetDefaultIMEWnd (IMM32.@)
 */
HWND WINAPI ImmGetDefaultIMEWnd(HWND hWnd)
{
    TRACE("Default is %p\n",IMM_GetThreadData()->hwndDefault);
    return IMM_GetThreadData()->hwndDefault;
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
UINT WINAPI ImmGetIMEFileNameA( HKL hKL, LPSTR lpszFileName, UINT uBufLen)
{
    LPWSTR bufW = NULL;
    UINT wBufLen = uBufLen;
    UINT rc;

    if (uBufLen && lpszFileName)
        bufW = HeapAlloc(GetProcessHeap(),0,uBufLen * sizeof(WCHAR));
    else /* We need this to get the number of byte required */
    {
        bufW = HeapAlloc(GetProcessHeap(),0,MAX_PATH * sizeof(WCHAR));
        wBufLen = MAX_PATH;
    }

    rc = ImmGetIMEFileNameW(hKL,bufW,wBufLen);

    if (rc > 0)
    {
        if (uBufLen && lpszFileName)
            rc = WideCharToMultiByte(CP_ACP, 0, bufW, -1, lpszFileName,
                                 uBufLen, NULL, NULL);
        else /* get the length */
            rc = WideCharToMultiByte(CP_ACP, 0, bufW, -1, NULL, 0, NULL,
                                     NULL);
    }

    HeapFree(GetProcessHeap(),0,bufW);
    return rc;
}

/***********************************************************************
 *		ImmGetIMEFileNameW (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameW(HKL hKL, LPWSTR lpszFileName, UINT uBufLen)
{
    HKEY hkey;
    DWORD length;
    DWORD rc;
    WCHAR regKey[sizeof(szImeRegFmt)/sizeof(WCHAR)+8];

    wsprintfW( regKey, szImeRegFmt, (ULONG_PTR)hKL );
    rc = RegOpenKeyW( HKEY_LOCAL_MACHINE, regKey, &hkey);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        return 0;
    }

    length = 0;
    rc = RegGetValueW(hkey, NULL, szImeFileW, RRF_RT_REG_SZ, NULL, NULL, &length);

    if (rc != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        SetLastError(rc);
        return 0;
    }
    if (length > uBufLen * sizeof(WCHAR) || !lpszFileName)
    {
        RegCloseKey(hkey);
        if (lpszFileName)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }
        else
            return length / sizeof(WCHAR);
    }

    RegGetValueW(hkey, NULL, szImeFileW, RRF_RT_REG_SZ, NULL, lpszFileName, &length);

    RegCloseKey(hkey);

    return length / sizeof(WCHAR);
}

/***********************************************************************
 *		ImmGetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmGetOpenStatus(HIMC hIMC)
{
  InputContextData *data = hIMC;

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
    ImmHkl *kbd;

    TRACE("(%p, %d)\n", hKL, fdwIndex);
    kbd = IMM_GetImmHkl(hKL);

    if (kbd && kbd->hIME)
    {
        switch (fdwIndex)
        {
            case IGP_PROPERTY: rc = kbd->imeInfo.fdwProperty; break;
            case IGP_CONVERSION: rc = kbd->imeInfo.fdwConversionCaps; break;
            case IGP_SENTENCE: rc = kbd->imeInfo.fdwSentenceCaps; break;
            case IGP_SETCOMPSTR: rc = kbd->imeInfo.fdwSCSCaps; break;
            case IGP_SELECT: rc = kbd->imeInfo.fdwSelectCaps; break;
            case IGP_GETIMEVERSION: rc = IMEVER_0400; break;
            case IGP_UI: rc = 0; break;
            default: rc = 0;
        }
    }
    return rc;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleA (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleA(
  HKL hKL, UINT nItem, LPSTYLEBUFA lpStyleBuf)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %d, %p):\n", hKL, nItem, lpStyleBuf);
    if (immHkl->hIME && immHkl->pImeGetRegisterWordStyle)
    {
        if (!is_kbd_ime_unicode(immHkl))
            return immHkl->pImeGetRegisterWordStyle(nItem,(LPSTYLEBUFW)lpStyleBuf);
        else
        {
            STYLEBUFW sbw;
            UINT rc;

            rc = immHkl->pImeGetRegisterWordStyle(nItem,&sbw);
            WideCharToMultiByte(CP_ACP, 0, sbw.szDescription, -1,
                lpStyleBuf->szDescription, 32, NULL, NULL);
            lpStyleBuf->dwStyle = sbw.dwStyle;
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleW (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleW(
  HKL hKL, UINT nItem, LPSTYLEBUFW lpStyleBuf)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %d, %p):\n", hKL, nItem, lpStyleBuf);
    if (immHkl->hIME && immHkl->pImeGetRegisterWordStyle)
    {
        if (is_kbd_ime_unicode(immHkl))
            return immHkl->pImeGetRegisterWordStyle(nItem,lpStyleBuf);
        else
        {
            STYLEBUFA sba;
            UINT rc;

            rc = immHkl->pImeGetRegisterWordStyle(nItem,(LPSTYLEBUFW)&sba);
            MultiByteToWideChar(CP_ACP, 0, sba.szDescription, -1,
                lpStyleBuf->szDescription, 32);
            lpStyleBuf->dwStyle = sba.dwStyle;
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmGetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmGetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
    InputContextData *data = hIMC;

    TRACE("(%p, %p)\n", hIMC, lpptPos);

    if (!data || !lpptPos)
        return FALSE;

    *lpptPos = data->IMC.ptStatusWndPos;

    return TRUE;
}

/***********************************************************************
 *		ImmGetVirtualKey (IMM32.@)
 */
UINT WINAPI ImmGetVirtualKey(HWND hWnd)
{
  OSVERSIONINFOA version;
  InputContextData *data = ImmGetContext( hWnd );
  TRACE("%p\n", hWnd);

  if ( data )
      return data->lastVK;

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
    LPWSTR lpszwIMEFileName;
    LPWSTR lpszwLayoutText;
    HKL hkl;

    TRACE ("(%s, %s)\n", debugstr_a(lpszIMEFileName),
                         debugstr_a(lpszLayoutText));

    lpszwIMEFileName = strdupAtoW(lpszIMEFileName);
    lpszwLayoutText = strdupAtoW(lpszLayoutText);

    hkl = ImmInstallIMEW(lpszwIMEFileName, lpszwLayoutText);

    HeapFree(GetProcessHeap(),0,lpszwIMEFileName);
    HeapFree(GetProcessHeap(),0,lpszwLayoutText);
    return hkl;
}

/***********************************************************************
 *		ImmInstallIMEW (IMM32.@)
 */
HKL WINAPI ImmInstallIMEW(
  LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText)
{
    INT lcid = GetUserDefaultLCID();
    INT count;
    HKL hkl;
    DWORD rc;
    HKEY hkey;
    WCHAR regKey[sizeof(szImeRegFmt)/sizeof(WCHAR)+8];

    TRACE ("(%s, %s):\n", debugstr_w(lpszIMEFileName),
                          debugstr_w(lpszLayoutText));

    /* Start with 2.  e001 will be blank and so default to the wine internal IME */
    count = 2;

    while (count < 0xfff)
    {
        DWORD disposition = 0;

        hkl = (HKL)MAKELPARAM( lcid, 0xe000 | count );
        wsprintfW( regKey, szImeRegFmt, (ULONG_PTR)hkl);

        rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, regKey, 0, NULL, 0, KEY_WRITE, NULL, &hkey, &disposition);
        if (rc == ERROR_SUCCESS && disposition == REG_CREATED_NEW_KEY)
            break;
        else if (rc == ERROR_SUCCESS)
            RegCloseKey(hkey);

        count++;
    }

    if (count == 0xfff)
    {
        WARN("Unable to find slot to install IME\n");
        return 0;
    }

    if (rc == ERROR_SUCCESS)
    {
        rc = RegSetValueExW(hkey, szImeFileW, 0, REG_SZ, (LPBYTE)lpszIMEFileName,
                            (lstrlenW(lpszIMEFileName) + 1) * sizeof(WCHAR));
        if (rc == ERROR_SUCCESS)
            rc = RegSetValueExW(hkey, szLayoutTextW, 0, REG_SZ, (LPBYTE)lpszLayoutText,
                                (lstrlenW(lpszLayoutText) + 1) * sizeof(WCHAR));
        RegCloseKey(hkey);
        return hkl;
    }
    else
    {
        WARN("Unable to set IME registry values\n");
        return 0;
    }
}

/***********************************************************************
 *		ImmIsIME (IMM32.@)
 */
BOOL WINAPI ImmIsIME(HKL hKL)
{
    ImmHkl *ptr;
    TRACE("(%p):\n", hKL);
    ptr = IMM_GetImmHkl(hKL);
    return (ptr && ptr->hIME);
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
        if (!IMM_GetThreadData()->hwndDefault)
            ImmGetDefaultIMEWnd(NULL);

        if (hWndIME == NULL)
            PostMessageA(IMM_GetThreadData()->hwndDefault, msg, wParam, lParam);

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
    TRACE("(%p, %d, %ld, %ld):\n", hWndIME, msg, wParam, lParam);
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
    InputContextData *data = hIMC;

    TRACE("(%p, %d, %d, %d)\n",
        hIMC, dwAction, dwIndex, dwValue);

    if (!data || ! data->immKbd->pNotifyIME)
        return FALSE;

    return data->immKbd->pNotifyIME(hIMC,dwAction,dwIndex,dwValue);
}

/***********************************************************************
 *		ImmRegisterWordA (IMM32.@)
 */
BOOL WINAPI ImmRegisterWordA(
  HKL hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszRegister)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %s, %d, %s):\n", hKL, debugstr_a(lpszReading), dwStyle,
                    debugstr_a(lpszRegister));
    if (immHkl->hIME && immHkl->pImeRegisterWord)
    {
        if (!is_kbd_ime_unicode(immHkl))
            return immHkl->pImeRegisterWord((LPCWSTR)lpszReading,dwStyle,
                                            (LPCWSTR)lpszRegister);
        else
        {
            LPWSTR lpszwReading = strdupAtoW(lpszReading);
            LPWSTR lpszwRegister = strdupAtoW(lpszRegister);
            BOOL rc;

            rc = immHkl->pImeRegisterWord(lpszwReading,dwStyle,lpszwRegister);
            HeapFree(GetProcessHeap(),0,lpszwReading);
            HeapFree(GetProcessHeap(),0,lpszwRegister);
            return rc;
        }
    }
    else
        return FALSE;
}

/***********************************************************************
 *		ImmRegisterWordW (IMM32.@)
 */
BOOL WINAPI ImmRegisterWordW(
  HKL hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszRegister)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %s, %d, %s):\n", hKL, debugstr_w(lpszReading), dwStyle,
                    debugstr_w(lpszRegister));
    if (immHkl->hIME && immHkl->pImeRegisterWord)
    {
        if (is_kbd_ime_unicode(immHkl))
            return immHkl->pImeRegisterWord(lpszReading,dwStyle,lpszRegister);
        else
        {
            LPSTR lpszaReading = strdupWtoA(lpszReading);
            LPSTR lpszaRegister = strdupWtoA(lpszRegister);
            BOOL rc;

            rc = immHkl->pImeRegisterWord((LPCWSTR)lpszaReading,dwStyle,
                                          (LPCWSTR)lpszaRegister);
            HeapFree(GetProcessHeap(),0,lpszaReading);
            HeapFree(GetProcessHeap(),0,lpszaRegister);
            return rc;
        }
    }
    else
        return FALSE;
}

/***********************************************************************
 *		ImmReleaseContext (IMM32.@)
 */
BOOL WINAPI ImmReleaseContext(HWND hWnd, HIMC hIMC)
{
  static int shown = 0;

  if (!shown) {
     FIXME("(%p, %p): stub\n", hWnd, hIMC);
     shown = 1;
  }
  return TRUE;
}

/***********************************************************************
*              ImmRequestMessageA(IMM32.@)
*/
LRESULT WINAPI ImmRequestMessageA(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    InputContextData *data = hIMC;

    TRACE("%p %ld %ld\n", hIMC, wParam, wParam);

    if (data && IsWindow(data->IMC.hWnd))
        return SendMessageA(data->IMC.hWnd, WM_IME_REQUEST, wParam, lParam);

     return 0;
}

/***********************************************************************
*              ImmRequestMessageW(IMM32.@)
*/
LRESULT WINAPI ImmRequestMessageW(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    InputContextData *data = hIMC;

    TRACE("%p %ld %ld\n", hIMC, wParam, wParam);

    if (data && IsWindow(data->IMC.hWnd))
        return SendMessageW(data->IMC.hWnd, WM_IME_REQUEST, wParam, lParam);

     return 0;
}

/***********************************************************************
 *		ImmSetCandidateWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCandidateWindow(
  HIMC hIMC, LPCANDIDATEFORM lpCandidate)
{
    InputContextData *data = hIMC;

    TRACE("(%p, %p)\n", hIMC, lpCandidate);

    if (!data || !lpCandidate)
        return FALSE;

    TRACE("\t%x, %x, (%i,%i), (%i,%i - %i,%i)\n",
            lpCandidate->dwIndex, lpCandidate->dwStyle,
            lpCandidate->ptCurrentPos.x, lpCandidate->ptCurrentPos.y,
            lpCandidate->rcArea.top, lpCandidate->rcArea.left,
            lpCandidate->rcArea.bottom, lpCandidate->rcArea.right);

    if ( lpCandidate->dwIndex >= (sizeof(data->IMC.cfCandForm) / sizeof(CANDIDATEFORM)) )
        return FALSE;

    data->IMC.cfCandForm[lpCandidate->dwIndex] = *lpCandidate;
    ImmNotifyIME(hIMC, NI_CONTEXTUPDATED, 0, IMC_SETCANDIDATEPOS);
    ImmInternalSendIMENotify(data, IMN_SETCANDIDATEPOS, 1 << lpCandidate->dwIndex);

    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
    InputContextData *data = hIMC;
    TRACE("(%p, %p)\n", hIMC, lplf);

    if (!data || !lplf)
        return FALSE;

    memcpy(&data->IMC.lfFont.W,lplf,sizeof(LOGFONTA));
    MultiByteToWideChar(CP_ACP, 0, lplf->lfFaceName, -1, data->IMC.lfFont.W.lfFaceName,
                        LF_FACESIZE);
    ImmNotifyIME(hIMC, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT);
    ImmInternalSendIMENotify(data, IMN_SETCOMPOSITIONFONT, 0);

    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
    InputContextData *data = hIMC;
    TRACE("(%p, %p)\n", hIMC, lplf);

    if (!data || !lplf)
        return FALSE;

    data->IMC.lfFont.W = *lplf;
    ImmNotifyIME(hIMC, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT);
    ImmInternalSendIMENotify(data, IMN_SETCOMPOSITIONFONT, 0);

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
    InputContextData *data = hIMC;

    TRACE("(%p, %d, %p, %d, %p, %d):\n",
            hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    if (!data)
        return FALSE;

    if (!(dwIndex == SCS_SETSTR ||
          dwIndex == SCS_CHANGEATTR ||
          dwIndex == SCS_CHANGECLAUSE ||
          dwIndex == SCS_SETRECONVERTSTRING ||
          dwIndex == SCS_QUERYRECONVERTSTRING))
        return FALSE;

    if (!is_himc_ime_unicode(data))
        return data->immKbd->pImeSetCompositionString(hIMC, dwIndex, lpComp,
                        dwCompLen, lpRead, dwReadLen);

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
    DWORD comp_len;
    DWORD read_len;
    CHAR *CompBuffer = NULL;
    CHAR *ReadBuffer = NULL;
    BOOL rc;
    InputContextData *data = hIMC;

    TRACE("(%p, %d, %p, %d, %p, %d):\n",
            hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    if (!data)
        return FALSE;

    if (!(dwIndex == SCS_SETSTR ||
          dwIndex == SCS_CHANGEATTR ||
          dwIndex == SCS_CHANGECLAUSE ||
          dwIndex == SCS_SETRECONVERTSTRING ||
          dwIndex == SCS_QUERYRECONVERTSTRING))
        return FALSE;

    if (is_himc_ime_unicode(data))
        return data->immKbd->pImeSetCompositionString(hIMC, dwIndex, lpComp,
                        dwCompLen, lpRead, dwReadLen);

    comp_len = WideCharToMultiByte(CP_ACP, 0, lpComp, dwCompLen, NULL, 0, NULL,
                                   NULL);
    if (comp_len)
    {
        CompBuffer = HeapAlloc(GetProcessHeap(),0,comp_len);
        WideCharToMultiByte(CP_ACP, 0, lpComp, dwCompLen, CompBuffer, comp_len,
                            NULL, NULL);
    }

    read_len = WideCharToMultiByte(CP_ACP, 0, lpRead, dwReadLen, NULL, 0, NULL,
                                   NULL);
    if (read_len)
    {
        ReadBuffer = HeapAlloc(GetProcessHeap(),0,read_len);
        WideCharToMultiByte(CP_ACP, 0, lpRead, dwReadLen, ReadBuffer, read_len,
                            NULL, NULL);
    }

    rc =  ImmSetCompositionStringA(hIMC, dwIndex, CompBuffer, comp_len,
                                   ReadBuffer, read_len);

    HeapFree(GetProcessHeap(), 0, CompBuffer);
    HeapFree(GetProcessHeap(), 0, ReadBuffer);

    return rc;
}

/***********************************************************************
 *		ImmSetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionWindow(
  HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
    BOOL reshow = FALSE;
    InputContextData *data = hIMC;

    TRACE("(%p, %p)\n", hIMC, lpCompForm);
    TRACE("\t%x, (%i,%i), (%i,%i - %i,%i)\n",lpCompForm->dwStyle,
          lpCompForm->ptCurrentPos.x, lpCompForm->ptCurrentPos.y, lpCompForm->rcArea.top,
          lpCompForm->rcArea.left, lpCompForm->rcArea.bottom, lpCompForm->rcArea.right);

    if (!data)
        return FALSE;

    data->IMC.cfCompForm = *lpCompForm;

    if (IsWindowVisible(IMM_GetThreadData()->hwndDefault))
    {
        reshow = TRUE;
        ShowWindow(IMM_GetThreadData()->hwndDefault,SW_HIDE);
    }

    /* FIXME: this is a partial stub */

    if (reshow)
        ShowWindow(IMM_GetThreadData()->hwndDefault,SW_SHOWNOACTIVATE);

    ImmInternalSendIMENotify(data, IMN_SETCOMPOSITIONWINDOW, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmSetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmSetConversionStatus(
  HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence)
{
    DWORD oldConversion, oldSentence;
    InputContextData *data = hIMC;

    TRACE("%p %d %d\n", hIMC, fdwConversion, fdwSentence);

    if (!data)
        return FALSE;

    if ( fdwConversion != data->IMC.fdwConversion )
    {
        oldConversion = data->IMC.fdwConversion;
        data->IMC.fdwConversion = fdwConversion;
        ImmNotifyIME(hIMC, NI_CONTEXTUPDATED, oldConversion, IMC_SETCONVERSIONMODE);
        ImmInternalSendIMENotify(data, IMN_SETCONVERSIONMODE, 0);
    }
    if ( fdwSentence != data->IMC.fdwSentence )
    {
        oldSentence = data->IMC.fdwSentence;
        data->IMC.fdwSentence = fdwSentence;
        ImmNotifyIME(hIMC, NI_CONTEXTUPDATED, oldSentence, IMC_SETSENTENCEMODE);
        ImmInternalSendIMENotify(data, IMN_SETSENTENCEMODE, 0);
    }

    return TRUE;
}

/***********************************************************************
 *		ImmSetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmSetOpenStatus(HIMC hIMC, BOOL fOpen)
{
    InputContextData *data = hIMC;

    TRACE("%p %d\n", hIMC, fOpen);

    if (!data)
        return FALSE;

    if (data->imeWnd == NULL)
    {
        /* create the ime window */
        data->imeWnd = CreateWindowExW( WS_EX_TOOLWINDOW,
                    data->immKbd->imeClassName, NULL, WS_POPUP, 0, 0, 1, 1, 0,
                    0, data->immKbd->hIME, 0);
        SetWindowLongPtrW(data->imeWnd, IMMGWL_IMC, (LONG_PTR)data);
        IMM_GetThreadData()->hwndDefault = data->imeWnd;
    }

    if (!fOpen != !data->IMC.fOpen)
    {
        data->IMC.fOpen = fOpen;
        ImmNotifyIME( hIMC, NI_CONTEXTUPDATED, 0, IMC_SETOPENSTATUS);
        ImmInternalSendIMENotify(data, IMN_SETOPENSTATUS, 0);
    }

    return TRUE;
}

/***********************************************************************
 *		ImmSetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmSetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
    InputContextData *data = hIMC;

    TRACE("(%p, %p)\n", hIMC, lpptPos);

    if (!data || !lpptPos)
        return FALSE;

    TRACE("\t(%i,%i)\n", lpptPos->x, lpptPos->y);

    data->IMC.ptStatusWndPos = *lpptPos;
    ImmNotifyIME( hIMC, NI_CONTEXTUPDATED, 0, IMC_SETSTATUSWINDOWPOS);
    ImmInternalSendIMENotify(data, IMN_SETSTATUSWINDOWPOS, 0);

    return TRUE;
}

/***********************************************************************
 *              ImmCreateSoftKeyboard(IMM32.@)
 */
HWND WINAPI ImmCreateSoftKeyboard(UINT uType, UINT hOwner, int x, int y)
{
    FIXME("(%d, %d, %d, %d): stub\n", uType, hOwner, x, y);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *              ImmDestroySoftKeyboard(IMM32.@)
 */
BOOL WINAPI ImmDestroySoftKeyboard(HWND hSoftWnd)
{
    FIXME("(%p): stub\n", hSoftWnd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              ImmShowSoftKeyboard(IMM32.@)
 */
BOOL WINAPI ImmShowSoftKeyboard(HWND hSoftWnd, int nCmdShow)
{
    FIXME("(%p, %d): stub\n", hSoftWnd, nCmdShow);
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
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %s, %d, %s):\n", hKL, debugstr_a(lpszReading), dwStyle,
            debugstr_a(lpszUnregister));
    if (immHkl->hIME && immHkl->pImeUnregisterWord)
    {
        if (!is_kbd_ime_unicode(immHkl))
            return immHkl->pImeUnregisterWord((LPCWSTR)lpszReading,dwStyle,
                                              (LPCWSTR)lpszUnregister);
        else
        {
            LPWSTR lpszwReading = strdupAtoW(lpszReading);
            LPWSTR lpszwUnregister = strdupAtoW(lpszUnregister);
            BOOL rc;

            rc = immHkl->pImeUnregisterWord(lpszwReading,dwStyle,lpszwUnregister);
            HeapFree(GetProcessHeap(),0,lpszwReading);
            HeapFree(GetProcessHeap(),0,lpszwUnregister);
            return rc;
        }
    }
    else
        return FALSE;
}

/***********************************************************************
 *		ImmUnregisterWordW (IMM32.@)
 */
BOOL WINAPI ImmUnregisterWordW(
  HKL hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszUnregister)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %s, %d, %s):\n", hKL, debugstr_w(lpszReading), dwStyle,
            debugstr_w(lpszUnregister));
    if (immHkl->hIME && immHkl->pImeUnregisterWord)
    {
        if (is_kbd_ime_unicode(immHkl))
            return immHkl->pImeUnregisterWord(lpszReading,dwStyle,lpszUnregister);
        else
        {
            LPSTR lpszaReading = strdupWtoA(lpszReading);
            LPSTR lpszaUnregister = strdupWtoA(lpszUnregister);
            BOOL rc;

            rc = immHkl->pImeUnregisterWord((LPCWSTR)lpszaReading,dwStyle,
                                            (LPCWSTR)lpszaUnregister);
            HeapFree(GetProcessHeap(),0,lpszaReading);
            HeapFree(GetProcessHeap(),0,lpszaUnregister);
            return rc;
        }
    }
    else
        return FALSE;
}

/***********************************************************************
 *		ImmGetImeMenuItemsA (IMM32.@)
 */
DWORD WINAPI ImmGetImeMenuItemsA( HIMC hIMC, DWORD dwFlags, DWORD dwType,
   LPIMEMENUITEMINFOA lpImeParentMenu, LPIMEMENUITEMINFOA lpImeMenu,
    DWORD dwSize)
{
    InputContextData *data = hIMC;
    TRACE("(%p, %i, %i, %p, %p, %i):\n", hIMC, dwFlags, dwType,
        lpImeParentMenu, lpImeMenu, dwSize);
    if (data->immKbd->hIME && data->immKbd->pImeGetImeMenuItems)
    {
        if (!is_himc_ime_unicode(data) || (!lpImeParentMenu && !lpImeMenu))
            return data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                (IMEMENUITEMINFOW*)lpImeParentMenu,
                                (IMEMENUITEMINFOW*)lpImeMenu, dwSize);
        else
        {
            IMEMENUITEMINFOW lpImeParentMenuW;
            IMEMENUITEMINFOW *lpImeMenuW, *parent = NULL;
            DWORD rc;

            if (lpImeParentMenu)
                parent = &lpImeParentMenuW;
            if (lpImeMenu)
            {
                int count = dwSize / sizeof(LPIMEMENUITEMINFOA);
                dwSize = count * sizeof(IMEMENUITEMINFOW);
                lpImeMenuW = HeapAlloc(GetProcessHeap(), 0, dwSize);
            }
            else
                lpImeMenuW = NULL;

            rc = data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                parent, lpImeMenuW, dwSize);

            if (lpImeParentMenu)
            {
                memcpy(lpImeParentMenu,&lpImeParentMenuW,sizeof(IMEMENUITEMINFOA));
                lpImeParentMenu->hbmpItem = lpImeParentMenuW.hbmpItem;
                WideCharToMultiByte(CP_ACP, 0, lpImeParentMenuW.szString,
                    -1, lpImeParentMenu->szString, IMEMENUITEM_STRING_SIZE,
                    NULL, NULL);
            }
            if (lpImeMenu && rc)
            {
                unsigned int i;
                for (i = 0; i < rc; i++)
                {
                    memcpy(&lpImeMenu[i],&lpImeMenuW[1],sizeof(IMEMENUITEMINFOA));
                    lpImeMenu[i].hbmpItem = lpImeMenuW[i].hbmpItem;
                    WideCharToMultiByte(CP_ACP, 0, lpImeMenuW[i].szString,
                        -1, lpImeMenu[i].szString, IMEMENUITEM_STRING_SIZE,
                        NULL, NULL);
                }
            }
            HeapFree(GetProcessHeap(),0,lpImeMenuW);
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
*		ImmGetImeMenuItemsW (IMM32.@)
*/
DWORD WINAPI ImmGetImeMenuItemsW( HIMC hIMC, DWORD dwFlags, DWORD dwType,
   LPIMEMENUITEMINFOW lpImeParentMenu, LPIMEMENUITEMINFOW lpImeMenu,
   DWORD dwSize)
{
    InputContextData *data = hIMC;
    TRACE("(%p, %i, %i, %p, %p, %i):\n", hIMC, dwFlags, dwType,
        lpImeParentMenu, lpImeMenu, dwSize);
    if (data->immKbd->hIME && data->immKbd->pImeGetImeMenuItems)
    {
        if (is_himc_ime_unicode(data) || (!lpImeParentMenu && !lpImeMenu))
            return data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                lpImeParentMenu, lpImeMenu, dwSize);
        else
        {
            IMEMENUITEMINFOA lpImeParentMenuA;
            IMEMENUITEMINFOA *lpImeMenuA, *parent = NULL;
            DWORD rc;

            if (lpImeParentMenu)
                parent = &lpImeParentMenuA;
            if (lpImeMenu)
            {
                int count = dwSize / sizeof(LPIMEMENUITEMINFOW);
                dwSize = count * sizeof(IMEMENUITEMINFOA);
                lpImeMenuA = HeapAlloc(GetProcessHeap(), 0, dwSize);
            }
            else
                lpImeMenuA = NULL;

            rc = data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                (IMEMENUITEMINFOW*)parent,
                                (IMEMENUITEMINFOW*)lpImeMenuA, dwSize);

            if (lpImeParentMenu)
            {
                memcpy(lpImeParentMenu,&lpImeParentMenuA,sizeof(IMEMENUITEMINFOA));
                lpImeParentMenu->hbmpItem = lpImeParentMenuA.hbmpItem;
                MultiByteToWideChar(CP_ACP, 0, lpImeParentMenuA.szString,
                    -1, lpImeParentMenu->szString, IMEMENUITEM_STRING_SIZE);
            }
            if (lpImeMenu && rc)
            {
                unsigned int i;
                for (i = 0; i < rc; i++)
                {
                    memcpy(&lpImeMenu[i],&lpImeMenuA[1],sizeof(IMEMENUITEMINFOA));
                    lpImeMenu[i].hbmpItem = lpImeMenuA[i].hbmpItem;
                    MultiByteToWideChar(CP_ACP, 0, lpImeMenuA[i].szString,
                        -1, lpImeMenu[i].szString, IMEMENUITEM_STRING_SIZE);
                }
            }
            HeapFree(GetProcessHeap(),0,lpImeMenuA);
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
*		ImmLockIMC(IMM32.@)
*/
LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC hIMC)
{
    InputContextData *data = hIMC;

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
    InputContextData *data = hIMC;
    data->dwLock--;
    return (data->dwLock!=0);
}

/***********************************************************************
*		ImmGetIMCLockCount(IMM32.@)
*/
DWORD WINAPI ImmGetIMCLockCount(HIMC hIMC)
{
    InputContextData *data = hIMC;
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
    return  internal;
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
    internal = imcc;

    internal->dwLock ++;
    return internal + 1;
}

/***********************************************************************
*		ImmUnlockIMCC(IMM32.@)
*/
BOOL WINAPI ImmUnlockIMCC(HIMCC imcc)
{
    IMCCInternal *internal;
    internal = imcc;

    internal->dwLock --;
    return (internal->dwLock!=0);
}

/***********************************************************************
*		ImmGetIMCCLockCount(IMM32.@)
*/
DWORD WINAPI ImmGetIMCCLockCount(HIMCC imcc)
{
    IMCCInternal *internal;
    internal = imcc;

    return internal->dwLock;
}

/***********************************************************************
*		ImmReSizeIMCC(IMM32.@)
*/
HIMCC  WINAPI ImmReSizeIMCC(HIMCC imcc, DWORD size)
{
    IMCCInternal *internal,*newone;
    int real_size = size + sizeof(IMCCInternal);

    internal = imcc;

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
    internal = imcc;

    return internal->dwSize;
}

/***********************************************************************
*		ImmGenerateMessage(IMM32.@)
*/
BOOL WINAPI ImmGenerateMessage(HIMC hIMC)
{
    InputContextData *data = hIMC;

    TRACE("%i messages queued\n",data->IMC.dwNumMsgBuf);
    if (data->IMC.dwNumMsgBuf > 0)
    {
        LPTRANSMSG lpTransMsg;
        DWORD i;

        lpTransMsg = ImmLockIMCC(data->IMC.hMsgBuf);
        for (i = 0; i < data->IMC.dwNumMsgBuf; i++)
            ImmInternalPostIMEMessage(data, lpTransMsg[i].message, lpTransMsg[i].wParam, lpTransMsg[i].lParam);

        ImmUnlockIMCC(data->IMC.hMsgBuf);

        data->IMC.dwNumMsgBuf = 0;
    }

    return TRUE;
}

/***********************************************************************
*       ImmTranslateMessage(IMM32.@)
*       ( Undocumented, call internally and from user32.dll )
*/
BOOL WINAPI ImmTranslateMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lKeyData)
{
    InputContextData *data;
    HIMC imc = ImmGetContext(hwnd);
    BYTE state[256];
    UINT scancode;
    LPVOID list = 0;
    UINT msg_count;
    UINT uVirtKey;
    static const DWORD list_count = 10;

    TRACE("%p %x %x %x\n",hwnd, msg, (UINT)wParam, (UINT)lKeyData);

    if (imc)
        data = imc;
    else
        return FALSE;

    if (!data->immKbd->hIME || !data->immKbd->pImeToAsciiEx)
        return FALSE;

    GetKeyboardState(state);
    scancode = lKeyData >> 0x10 & 0xff;

    list = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, list_count * sizeof(TRANSMSG) + sizeof(DWORD));
    ((DWORD*)list)[0] = list_count;

    if (data->immKbd->imeInfo.fdwProperty & IME_PROP_KBD_CHAR_FIRST)
    {
        WCHAR chr;

        if (!is_himc_ime_unicode(data))
            ToAscii(data->lastVK, scancode, state, &chr, 0);
        else
            ToUnicodeEx(data->lastVK, scancode, state, &chr, 1, 0, GetKeyboardLayout(0));
        uVirtKey = MAKELONG(data->lastVK,chr);
    }
    else
        uVirtKey = data->lastVK;

    msg_count = data->immKbd->pImeToAsciiEx(uVirtKey, scancode, state, list, 0, imc);
    TRACE("%i messages generated\n",msg_count);
    if (msg_count && msg_count <= list_count)
    {
        UINT i;
        LPTRANSMSG msgs = (LPTRANSMSG)((LPBYTE)list + sizeof(DWORD));

        for (i = 0; i < msg_count; i++)
            ImmInternalPostIMEMessage(data, msgs[i].message, msgs[i].wParam, msgs[i].lParam);
    }
    else if (msg_count > list_count)
        ImmGenerateMessage(imc);

    HeapFree(GetProcessHeap(),0,list);

    data->lastVK = VK_PROCESSKEY;

    return (msg_count > 0);
}

/***********************************************************************
*		ImmProcessKey(IMM32.@)
*       ( Undocumented, called from user32.dll )
*/
BOOL WINAPI ImmProcessKey(HWND hwnd, HKL hKL, UINT vKey, LPARAM lKeyData, DWORD unknown)
{
    InputContextData *data;
    HIMC imc = ImmGetContext(hwnd);
    BYTE state[256];

    TRACE("%p %p %x %x %x\n",hwnd, hKL, vKey, (UINT)lKeyData, unknown);

    if (imc)
        data = imc;
    else
        return FALSE;

    if (!data->immKbd->hIME || !data->immKbd->pImeProcessKey)
        return FALSE;

    GetKeyboardState(state);
    if (data->immKbd->pImeProcessKey(imc, vKey, lKeyData, state))
    {
        data->lastVK = vKey;
        return TRUE;
    }

    data->lastVK = VK_PROCESSKEY;
    return FALSE;
}

/***********************************************************************
*		ImmDisableTextFrameService(IMM32.@)
*/
BOOL WINAPI ImmDisableTextFrameService(DWORD idThread)
{
    FIXME("Stub\n");
    return FALSE;
}
