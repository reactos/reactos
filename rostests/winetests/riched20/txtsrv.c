/*
 * Unit test suite for windowless rich edit controls
 *
 * Copyright 2008 Maarten Lankhorst
 * Copyright 2008 Austin Lund
 * Copyright 2008 Dylan Smith
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

#define COBJMACROS
#define CONST_VTABLE

#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <richedit.h>
#include <tom.h>
#include <richole.h>
#include <initguid.h>
#include <textserv.h>
#include <wine/test.h>
#include <oleauto.h>
#include <limits.h>

static HMODULE hmoduleRichEdit;
static IID *pIID_ITextServices;
static IID *pIID_ITextHost;
static IID *pIID_ITextHost2;
static PCreateTextServices pCreateTextServices;

/* Define C Macros for ITextServices calls. */

/* Use a special table for x86 machines to convert the thiscall
 * calling convention.  This isn't needed on other platforms. */
#ifdef __i386__
static ITextServicesVtbl itextServicesStdcallVtbl;
#define TXTSERV_VTABLE(This) (&itextServicesStdcallVtbl)
#else /* __i386__ */
#define TXTSERV_VTABLE(This) (This)->lpVtbl
#endif /* __i386__ */

#define ITextServices_TxSendMessage(This,a,b,c,d) TXTSERV_VTABLE(This)->TxSendMessage(This,a,b,c,d)
#define ITextServices_TxDraw(This,a,b,c,d,e,f,g,h,i,j,k,l) TXTSERV_VTABLE(This)->TxDraw(This,a,b,c,d,e,f,g,h,i,j,k,l)
#define ITextServices_TxGetHScroll(This,a,b,c,d,e) TXTSERV_VTABLE(This)->TxGetHScroll(This,a,b,c,d,e)
#define ITextServices_TxGetVScroll(This,a,b,c,d,e) TXTSERV_VTABLE(This)->TxGetVScroll(This,a,b,c,d,e)
#define ITextServices_OnTxSetCursor(This,a,b,c,d,e,f,g,h,i) TXTSERV_VTABLE(This)->OnTxSetCursor(This,a,b,c,d,e,f,g,h,i)
#define ITextServices_TxQueryHitPoint(This,a,b,c,d,e,f,g,h,i,j) TXTSERV_VTABLE(This)->TxQueryHitPoint(This,a,b,c,d,e,f,g,h,i,j)
#define ITextServices_OnTxInplaceActivate(This,a) TXTSERV_VTABLE(This)->OnTxInplaceActivate(This,a)
#define ITextServices_OnTxInplaceDeactivate(This) TXTSERV_VTABLE(This)->OnTxInplaceDeactivate(This)
#define ITextServices_OnTxUIActivate(This) TXTSERV_VTABLE(This)->OnTxUIActivate(This)
#define ITextServices_OnTxUIDeactivate(This) TXTSERV_VTABLE(This)->OnTxUIDeactivate(This)
#define ITextServices_TxGetText(This,a) TXTSERV_VTABLE(This)->TxGetText(This,a)
#define ITextServices_TxSetText(This,a) TXTSERV_VTABLE(This)->TxSetText(This,a)
#define ITextServices_TxGetCurTargetX(This,a) TXTSERV_VTABLE(This)->TxGetCurTargetX(This,a)
#define ITextServices_TxGetBaseLinePos(This,a) TXTSERV_VTABLE(This)->TxGetBaseLinePos(This,a)
#define ITextServices_TxGetNaturalSize(This,a,b,c,d,e,f,g,h) TXTSERV_VTABLE(This)->TxGetNaturalSize(This,a,b,c,d,e,f,g,h)
#define ITextServices_TxGetDropTarget(This,a) TXTSERV_VTABLE(This)->TxGetDropTarget(This,a)
#define ITextServices_OnTxPropertyBitsChange(This,a,b) TXTSERV_VTABLE(This)->OnTxPropertyBitsChange(This,a,b)
#define ITextServices_TxGetCachedSize(This,a,b) TXTSERV_VTABLE(This)->TxGetCachedSize(This,a,b)

/* Set the WINETEST_DEBUG environment variable to be greater than 1 for verbose
 * function call traces of ITextHost. */
#define TRACECALL if(winetest_debug > 1) trace

/************************************************************************/
/* ITextHost implementation for conformance testing. */

typedef struct ITextHostTestImpl
{
    ITextHost ITextHost_iface;
    LONG refCount;
} ITextHostTestImpl;

static inline ITextHostTestImpl *impl_from_ITextHost(ITextHost *iface)
{
    return CONTAINING_RECORD(iface, ITextHostTestImpl, ITextHost_iface);
}

static HRESULT WINAPI ITextHostImpl_QueryInterface(ITextHost *iface,
                                                   REFIID riid,
                                                   LPVOID *ppvObject)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, pIID_ITextHost)) {
        *ppvObject = This;
        ITextHost_AddRef((ITextHost *)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI ITextHostImpl_AddRef(ITextHost *iface)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);
    return refCount;
}

static ULONG WINAPI ITextHostImpl_Release(ITextHost *iface)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    if (!refCount)
    {
        CoTaskMemFree(This);
        return 0;
    } else {
        return refCount;
    }
}

static HDC WINAPI ITextHostImpl_TxGetDC(ITextHost *iface)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetDC(%p)\n", This);
    return NULL;
}

static INT WINAPI ITextHostImpl_TxReleaseDC(ITextHost *iface,
                                            HDC hdc)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxReleaseDC(%p)\n", This);
    return 0;
}

static BOOL WINAPI ITextHostImpl_TxShowScrollBar(ITextHost *iface,
                                                 INT fnBar,
                                                 BOOL fShow)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxShowScrollBar(%p, fnBar=%d, fShow=%d)\n",
                This, fnBar, fShow);
    return FALSE;
}

static BOOL WINAPI ITextHostImpl_TxEnableScrollBar(ITextHost *iface,
                                                   INT fuSBFlags,
                                                   INT fuArrowflags)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxEnableScrollBar(%p, fuSBFlags=%d, fuArrowflags=%d)\n",
               This, fuSBFlags, fuArrowflags);
    return FALSE;
}

static BOOL WINAPI ITextHostImpl_TxSetScrollRange(ITextHost *iface,
                                                  INT fnBar,
                                                  LONG nMinPos,
                                                  INT nMaxPos,
                                                  BOOL fRedraw)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetScrollRange(%p, fnBar=%d, nMinPos=%d, nMaxPos=%d, fRedraw=%d)\n",
               This, fnBar, nMinPos, nMaxPos, fRedraw);
    return FALSE;
}

static BOOL WINAPI ITextHostImpl_TxSetScrollPos(ITextHost *iface,
                                                INT fnBar,
                                                INT nPos,
                                                BOOL fRedraw)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetScrollPos(%p, fnBar=%d, nPos=%d, fRedraw=%d)\n",
               This, fnBar, nPos, fRedraw);
    return FALSE;
}

static void WINAPI ITextHostImpl_TxInvalidateRect(ITextHost *iface,
                                                  LPCRECT prc,
                                                  BOOL fMode)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxInvalidateRect(%p, prc=%p, fMode=%d)\n",
               This, prc, fMode);
}

static void WINAPI ITextHostImpl_TxViewChange(ITextHost *iface, BOOL fUpdate)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxViewChange(%p, fUpdate=%d)\n",
               This, fUpdate);
}

static BOOL WINAPI ITextHostImpl_TxCreateCaret(ITextHost *iface,
                                               HBITMAP hbmp,
                                               INT xWidth, INT yHeight)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxCreateCaret(%p, nbmp=%p, xWidth=%d, yHeight=%d)\n",
               This, hbmp, xWidth, yHeight);
    return FALSE;
}

static BOOL WINAPI ITextHostImpl_TxShowCaret(ITextHost *iface, BOOL fShow)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxShowCaret(%p, fShow=%d)\n",
               This, fShow);
    return FALSE;
}

static BOOL WINAPI ITextHostImpl_TxSetCaretPos(ITextHost *iface,
                                               INT x, INT y)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetCaretPos(%p, x=%d, y=%d)\n", This, x, y);
    return FALSE;
}

static BOOL WINAPI ITextHostImpl_TxSetTimer(ITextHost *iface,
                                            UINT idTimer, UINT uTimeout)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetTimer(%p, idTimer=%u, uTimeout=%u)\n",
              This, idTimer, uTimeout);
    return FALSE;
}

static void WINAPI ITextHostImpl_TxKillTimer(ITextHost *iface, UINT idTimer)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxKillTimer(%p, idTimer=%u)\n", This, idTimer);
}

static void WINAPI ITextHostImpl_TxScrollWindowEx(ITextHost *iface,
                                                  INT dx, INT dy,
                                                  LPCRECT lprcScroll,
                                                  LPCRECT lprcClip,
                                                  HRGN hRgnUpdate,
                                                  LPRECT lprcUpdate,
                                                  UINT fuScroll)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxScrollWindowEx(%p, %d, %d, %p, %p, %p, %p, %d)\n",
              This, dx, dy, lprcScroll, lprcClip, hRgnUpdate, lprcUpdate, fuScroll);
}

static void WINAPI ITextHostImpl_TxSetCapture(ITextHost *iface, BOOL fCapture)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetCapture(%p, fCapture=%d)\n", This, fCapture);
}

static void WINAPI ITextHostImpl_TxSetFocus(ITextHost *iface)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetFocus(%p)\n", This);
}

static void WINAPI ITextHostImpl_TxSetCursor(ITextHost *iface,
                                             HCURSOR hcur,
                                             BOOL fText)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetCursor(%p, hcur=%p, fText=%d)\n",
              This, hcur, fText);
}

static BOOL WINAPI ITextHostImpl_TxScreenToClient(ITextHost *iface,
                                                  LPPOINT lppt)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxScreenToClient(%p, lppt=%p)\n", This, lppt);
    return FALSE;
}

static BOOL WINAPI ITextHostImpl_TxClientToScreen(ITextHost *iface,
                                                  LPPOINT lppt)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxClientToScreen(%p, lppt=%p)\n", This, lppt);
    return FALSE;
}

static HRESULT WINAPI ITextHostImpl_TxActivate(ITextHost *iface,
                                               LONG *plOldState)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxActivate(%p, plOldState=%p)\n", This, plOldState);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextHostImpl_TxDeactivate(ITextHost *iface,
                                                 LONG lNewState)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxDeactivate(%p, lNewState=%d)\n", This, lNewState);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextHostImpl_TxGetClientRect(ITextHost *iface,
                                                    LPRECT prc)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetClientRect(%p, prc=%p)\n", This, prc);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextHostImpl_TxGetViewInset(ITextHost *iface,
                                                   LPRECT prc)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetViewInset(%p, prc=%p)\n", This, prc);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextHostImpl_TxGetCharFormat(ITextHost *iface,
                                                    const CHARFORMATW **ppCF)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetCharFormat(%p, ppCF=%p)\n", This, ppCF);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextHostImpl_TxGetParaFormat(ITextHost *iface,
                                                    const PARAFORMAT **ppPF)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetParaFormat(%p, ppPF=%p)\n", This, ppPF);
    return E_NOTIMPL;
}

static COLORREF WINAPI ITextHostImpl_TxGetSysColor(ITextHost *iface,
                                                   int nIndex)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetSysColor(%p, nIndex=%d)\n", This, nIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextHostImpl_TxGetBackStyle(ITextHost *iface,
                                                   TXTBACKSTYLE *pStyle)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetBackStyle(%p, pStyle=%p)\n", This, pStyle);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextHostImpl_TxGetMaxLength(ITextHost *iface,
                                                   DWORD *pLength)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetMaxLength(%p, pLength=%p)\n", This, pLength);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextHostImpl_TxGetScrollBars(ITextHost *iface,
                                                    DWORD *pdwScrollBar)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetScrollBars(%p, pdwScrollBar=%p)\n",
               This, pdwScrollBar);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextHostImpl_TxGetPasswordChar(ITextHost *iface,
                                                      WCHAR *pch)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetPasswordChar(%p, pch=%p)\n", This, pch);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextHostImpl_TxGetAcceleratorPos(ITextHost *iface,
                                                        LONG *pch)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetAcceleratorPos(%p, pch=%p)\n", This, pch);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextHostImpl_TxGetExtent(ITextHost *iface,
                                                LPSIZEL lpExtent)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetExtent(%p, lpExtent=%p)\n", This, lpExtent);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextHostImpl_OnTxCharFormatChange(ITextHost *iface,
                                                         const CHARFORMATW *pcf)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to OnTxCharFormatChange(%p, pcf=%p)\n", This, pcf);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextHostImpl_OnTxParaFormatChange(ITextHost *iface,
                                                         const PARAFORMAT *ppf)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to OnTxParaFormatChange(%p, ppf=%p)\n", This, ppf);
    return E_NOTIMPL;
}

/* This must return S_OK for the native ITextServices object to
   initialize. */
static HRESULT WINAPI ITextHostImpl_TxGetPropertyBits(ITextHost *iface,
                                                      DWORD dwMask,
                                                      DWORD *pdwBits)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetPropertyBits(%p, dwMask=0x%08x, pdwBits=%p)\n",
              This, dwMask, pdwBits);
    *pdwBits = 0;
    return S_OK;
}

static HRESULT WINAPI ITextHostImpl_TxNotify(ITextHost *iface, DWORD iNotify,
                                             void *pv)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxNotify(%p, iNotify=%d, pv=%p)\n", This, iNotify, pv);
    return E_NOTIMPL;
}

static HIMC WINAPI ITextHostImpl_TxImmGetContext(ITextHost *iface)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxImmGetContext(%p)\n", This);
    return 0;
}

static void WINAPI ITextHostImpl_TxImmReleaseContext(ITextHost *iface, HIMC himc)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxImmReleaseContext(%p, himc=%p)\n", This, himc);
}

/* This function must set the variable pointed to by *lSelBarWidth.
   Otherwise an uninitialized value will be used to calculate
   positions and sizes even if E_NOTIMPL is returned. */
static HRESULT WINAPI ITextHostImpl_TxGetSelectionBarWidth(ITextHost *iface,
                                                           LONG *lSelBarWidth)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetSelectionBarWidth(%p, lSelBarWidth=%p)\n",
                This, lSelBarWidth);
    *lSelBarWidth = 0;
    return E_NOTIMPL;
}

static ITextHostVtbl itextHostVtbl = {
    ITextHostImpl_QueryInterface,
    ITextHostImpl_AddRef,
    ITextHostImpl_Release,
    ITextHostImpl_TxGetDC,
    ITextHostImpl_TxReleaseDC,
    ITextHostImpl_TxShowScrollBar,
    ITextHostImpl_TxEnableScrollBar,
    ITextHostImpl_TxSetScrollRange,
    ITextHostImpl_TxSetScrollPos,
    ITextHostImpl_TxInvalidateRect,
    ITextHostImpl_TxViewChange,
    ITextHostImpl_TxCreateCaret,
    ITextHostImpl_TxShowCaret,
    ITextHostImpl_TxSetCaretPos,
    ITextHostImpl_TxSetTimer,
    ITextHostImpl_TxKillTimer,
    ITextHostImpl_TxScrollWindowEx,
    ITextHostImpl_TxSetCapture,
    ITextHostImpl_TxSetFocus,
    ITextHostImpl_TxSetCursor,
    ITextHostImpl_TxScreenToClient,
    ITextHostImpl_TxClientToScreen,
    ITextHostImpl_TxActivate,
    ITextHostImpl_TxDeactivate,
    ITextHostImpl_TxGetClientRect,
    ITextHostImpl_TxGetViewInset,
    ITextHostImpl_TxGetCharFormat,
    ITextHostImpl_TxGetParaFormat,
    ITextHostImpl_TxGetSysColor,
    ITextHostImpl_TxGetBackStyle,
    ITextHostImpl_TxGetMaxLength,
    ITextHostImpl_TxGetScrollBars,
    ITextHostImpl_TxGetPasswordChar,
    ITextHostImpl_TxGetAcceleratorPos,
    ITextHostImpl_TxGetExtent,
    ITextHostImpl_OnTxCharFormatChange,
    ITextHostImpl_OnTxParaFormatChange,
    ITextHostImpl_TxGetPropertyBits,
    ITextHostImpl_TxNotify,
    ITextHostImpl_TxImmGetContext,
    ITextHostImpl_TxImmReleaseContext,
    ITextHostImpl_TxGetSelectionBarWidth
};

static ITextServices *txtserv = NULL;
static ITextHostTestImpl *dummyTextHost;
static void *wrapperCodeMem = NULL;

#include "pshpack1.h"

/* Code structure for x86 byte code */
typedef struct
{
    BYTE pop_eax;  /* popl  %eax  */
    BYTE push_ecx; /* pushl %ecx  */
    BYTE push_eax; /* pushl %eax  */
    BYTE jmp_func; /* jmp   $func */
    DWORD func;
} THISCALL_TO_STDCALL_THUNK;

typedef struct
{
    BYTE pop_eax;               /* popl  %eax */
    BYTE pop_ecx;               /* popl  %ecx */
    BYTE push_eax;              /* pushl %eax */
    BYTE mov_vtable_eax[2];     /* movl (%ecx), %eax */
    BYTE jmp_eax[2];            /* jmp *$vtablefunc_offset(%eax) */
    int  vtablefunc_offset;
} STDCALL_TO_THISCALL_THUNK;

#include "poppack.h"

static void setup_thiscall_wrappers(void)
{
#ifdef __i386__
    void** pVtable;
    void** pVtableEnd;
    THISCALL_TO_STDCALL_THUNK *thunk;
    STDCALL_TO_THISCALL_THUNK *thunk2;

    wrapperCodeMem = VirtualAlloc(NULL,
                                  (sizeof(ITextHostVtbl)/sizeof(void*) - 3)
                                    * sizeof(THISCALL_TO_STDCALL_THUNK)
                                  +(sizeof(ITextServicesVtbl)/sizeof(void*) - 3)
                                    * sizeof(STDCALL_TO_THISCALL_THUNK),
                                  MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    thunk = wrapperCodeMem;

    /* Wrap all ITextHostImpl methods with code to perform a thiscall to
     * stdcall conversion. The thiscall calling convention places the This
     * pointer in ecx on the x86 platform, and the stdcall calling convention
     * pushes the This pointer on the stack as the first argument.
     *
     * The byte code does the conversion then jumps to the real function.
     *
     * Each wrapper needs to be modified so that the function to jump to is
     * modified in the byte code. */

    /* Skip QueryInterface, AddRef, and Release native actually
     * defined them with the stdcall calling convention. */
    pVtable = (void**)&itextHostVtbl + 3;
    pVtableEnd = (void**)(&itextHostVtbl + 1);
    while (pVtable != pVtableEnd) {
        /* write byte code to executable memory */
        thunk->pop_eax = 0x58;  /* popl  %eax  */
        thunk->push_ecx = 0x51; /* pushl %ecx  */
        thunk->push_eax = 0x50; /* pushl %eax  */
        thunk->jmp_func = 0xe9; /* jmp   $func */
        /* The address needs to be relative to the end of the jump instructions. */
        thunk->func = (char*)*pVtable - (char*)(&thunk->func + 1);
        *pVtable = thunk;
        pVtable++;
        thunk++;
    }

    /* Setup an ITextServices standard call vtable that will call the
     * native thiscall vtable when the methods are called. */

    /* QueryInterface, AddRef, and Release should be called directly on the
     * real vtable since they use the stdcall calling convention. */
    thunk2 = (STDCALL_TO_THISCALL_THUNK *)thunk;
    pVtable = (void**)&itextServicesStdcallVtbl + 3;
    pVtableEnd = (void**)(&itextServicesStdcallVtbl + 1);
    while (pVtable != pVtableEnd) {
        /* write byte code to executable memory */
        thunk2->pop_eax = 0x58;               /* popl  %eax */
        thunk2->pop_ecx = 0x59;               /* popl  %ecx */
        thunk2->push_eax = 0x50;              /* pushl %eax */
        thunk2->mov_vtable_eax[0] = 0x8b;     /* movl (%ecx), %eax */
        thunk2->mov_vtable_eax[1] = 0x01;
        thunk2->jmp_eax[0] = 0xff;            /* jmp *$vtablefunc_offset(%eax) */
        thunk2->jmp_eax[1] = 0xa0;
        thunk2->vtablefunc_offset = (char*)pVtable - (char*)&itextServicesStdcallVtbl;
        *pVtable = thunk2;
        pVtable++;
        thunk2++;
    }
#endif /* __i386__ */
}

/*************************************************************************/
/* Conformance test functions. */

/* Initialize the test texthost structure */
static BOOL init_texthost(void)
{
    IUnknown *init;
    HRESULT result;

    dummyTextHost = CoTaskMemAlloc(sizeof(*dummyTextHost));
    if (dummyTextHost == NULL) {
        skip("Insufficient memory to create ITextHost interface\n");
        return FALSE;
    }
    dummyTextHost->ITextHost_iface.lpVtbl = &itextHostVtbl;
    dummyTextHost->refCount = 1;

    /* MSDN states that an IUnknown object is returned by
       CreateTextServices which is then queried to obtain a
       ITextServices object. */
    result = (*pCreateTextServices)(NULL, &dummyTextHost->ITextHost_iface, &init);
    ok(result == S_OK, "Did not return S_OK when created (result =  %x)\n", result);
    if (result != S_OK) {
        CoTaskMemFree(dummyTextHost);
        skip("CreateTextServices failed.\n");
        return FALSE;
    }

    result = IUnknown_QueryInterface(init, pIID_ITextServices,
                                     (void **)&txtserv);
    ok((result == S_OK) && (txtserv != NULL), "Querying interface failed (result = %x, txtserv = %p)\n", result, txtserv);
    IUnknown_Release(init);
    if (!((result == S_OK) && (txtserv != NULL))) {
        CoTaskMemFree(dummyTextHost);
        skip("Could not retrieve ITextServices interface\n");
        return FALSE;
    }

    return TRUE;
}

static void free_texthost(void)
{
    ITextServices_Release(txtserv);
    CoTaskMemFree(dummyTextHost);
}

static void test_TxGetText(void)
{
    HRESULT hres;
    BSTR rettext;

    if (!init_texthost())
        return;

    hres = ITextServices_TxGetText(txtserv, &rettext);
    ok(hres == S_OK, "ITextServices_TxGetText failed (result = %x)\n", hres);

    free_texthost();
}

static void test_TxSetText(void)
{
    HRESULT hres;
    BSTR rettext;
    WCHAR settext[] = {'T','e','s','t',0};

    if (!init_texthost())
        return;

    hres = ITextServices_TxSetText(txtserv, settext);
    ok(hres == S_OK, "ITextServices_TxSetText failed (result = %x)\n", hres);

    hres = ITextServices_TxGetText(txtserv, &rettext);
    ok(hres == S_OK, "ITextServices_TxGetText failed (result = %x)\n", hres);

    ok(SysStringLen(rettext) == 4,
                 "String returned of wrong length (expected 4, got %d)\n", SysStringLen(rettext));
    ok(memcmp(rettext,settext,SysStringByteLen(rettext)) == 0,
                 "String returned differs\n");

    SysFreeString(rettext);
    free_texthost();
}

static void test_TxGetNaturalSize(void) {
    HRESULT result;
    BOOL ret;

    /* This value is used when calling TxGetNaturalSize.  MSDN says
       that this is not supported however a null pointer cannot be
       used as it will cause a segmentation violation.  The values in
       the structure being pointed to are required to be INT_MAX
       otherwise calculations can give wrong values. */
    const SIZEL psizelExtent = {INT_MAX,INT_MAX};

    static const WCHAR oneA[] = {'A',0};

    /* Results of measurements */
    LONG xdim, ydim;

    /* The device context to do the tests in */
    HDC hdcDraw;

    /* Variables with the text metric information */
    INT charwidth_caps_text[26];
    TEXTMETRICA tmInfo_text;

    if (!init_texthost())
        return;

    hdcDraw = GetDC(NULL);
    SaveDC(hdcDraw);

    /* Populate the metric strucs */
    SetMapMode(hdcDraw,MM_TEXT);
    GetTextMetricsA(hdcDraw, &tmInfo_text);
    SetLastError(0xdeadbeef);
    ret = GetCharWidth32A(hdcDraw,'A','Z',charwidth_caps_text);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
        win_skip("GetCharWidth32 is not available\n");
        goto cleanup;
    }

    /* Make measurements in MM_TEXT */
    SetMapMode(hdcDraw,MM_TEXT);
    xdim = 0; ydim = 0;

    result = ITextServices_TxSetText(txtserv, oneA);
    ok(result == S_OK, "ITextServices_TxSetText failed (result = %x)\n", result);
    if (result != S_OK) {
        skip("Could not set text\n");
        goto cleanup;
    }

    SetLastError(0xdeadbeef);
    result = ITextServices_TxGetNaturalSize(txtserv, DVASPECT_CONTENT,
                                            hdcDraw, NULL, NULL,
                                            TXTNS_FITTOCONTENT, &psizelExtent,
                                            &xdim, &ydim);
    todo_wine ok(result == S_OK || broken(result == E_FAIL), /* WINXP Arabic Language */
        "TxGetNaturalSize gave unexpected return value (result = %x)\n", result);
    if (result == S_OK) {
    todo_wine ok(ydim == tmInfo_text.tmHeight,
                 "Height calculated incorrectly (expected %d, got %d)\n",
                 tmInfo_text.tmHeight, ydim);
    /* The native DLL adds one pixel extra when calculating widths. */
    todo_wine ok(xdim >= charwidth_caps_text[0] && xdim <= charwidth_caps_text[0] + 1,
                 "Width calculated incorrectly (expected %d {+1}, got %d)\n",
                 charwidth_caps_text[0], xdim);
    } else
        skip("TxGetNaturalSize measurements not performed (xdim = %d, ydim = %d, result = %x, error = %x)\n",
             xdim, ydim, result, GetLastError());

cleanup:
    RestoreDC(hdcDraw,1);
    ReleaseDC(NULL,hdcDraw);
    free_texthost();
}

static void test_TxDraw(void)
{
    HDC tmphdc = GetDC(NULL);
    DWORD dwAspect = DVASPECT_CONTENT;
    HDC hicTargetDev = NULL; /* Means "default" device */
    DVTARGETDEVICE *ptd = NULL;
    void *pvAspect = NULL;
    HRESULT result;
    RECTL client = {0,0,100,100};

    if (!init_texthost())
        return;

    todo_wine {
        result = ITextServices_TxDraw(txtserv, dwAspect, 0, pvAspect, ptd,
                                      tmphdc, hicTargetDev, &client, NULL,
                                      NULL, NULL, 0, 0);
        ok(result == S_OK, "TxDraw failed (result = %x)\n", result);
    }

    free_texthost();

}

DEFINE_GUID(expected_iid_itextservices, 0x8d33f740, 0xcf58, 0x11ce, 0xa8, 0x9d, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5);
DEFINE_GUID(expected_iid_itexthost, 0x13e670f4,0x1a5a,0x11cf,0xab,0xeb,0x00,0xaa,0x00,0xb6,0x5e,0xa1);
DEFINE_GUID(expected_iid_itexthost2, 0x13e670f5,0x1a5a,0x11cf,0xab,0xeb,0x00,0xaa,0x00,0xb6,0x5e,0xa1);

static void test_IIDs(void)
{
    ok(IsEqualIID(pIID_ITextServices, &expected_iid_itextservices),
       "unexpected value for IID_ITextServices: %s\n", wine_dbgstr_guid(pIID_ITextServices));
    ok(IsEqualIID(pIID_ITextHost, &expected_iid_itexthost),
       "unexpected value for IID_ITextHost: %s\n", wine_dbgstr_guid(pIID_ITextHost));
    ok(IsEqualIID(pIID_ITextHost2, &expected_iid_itexthost2),
       "unexpected value for IID_ITextHost2: %s\n", wine_dbgstr_guid(pIID_ITextHost2));
}

/* Outer IUnknown for COM aggregation tests */
struct unk_impl {
    IUnknown IUnknown_iface;
    LONG ref;
    IUnknown *inner_unk;
};

static inline struct unk_impl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct unk_impl, IUnknown_iface);
}

static HRESULT WINAPI unk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    struct unk_impl *This = impl_from_IUnknown(iface);

    return IUnknown_QueryInterface(This->inner_unk, riid, ppv);
}

static ULONG WINAPI unk_AddRef(IUnknown *iface)
{
    struct unk_impl *This = impl_from_IUnknown(iface);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI unk_Release(IUnknown *iface)
{
    struct unk_impl *This = impl_from_IUnknown(iface);

    return InterlockedDecrement(&This->ref);
}

static const IUnknownVtbl unk_vtbl =
{
    unk_QueryInterface,
    unk_AddRef,
    unk_Release
};

static void test_COM(void)
{
    struct unk_impl unk_obj = {{&unk_vtbl}, 19, NULL};
    struct ITextHostTestImpl texthost = {{&itextHostVtbl}, 1};
    ITextServices *textsrv;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = pCreateTextServices(&unk_obj.IUnknown_iface, &texthost.ITextHost_iface,
                             &unk_obj.inner_unk);
    ok(hr == S_OK, "CreateTextServices failed: %08x\n", hr);
    hr = IUnknown_QueryInterface(unk_obj.inner_unk, pIID_ITextServices, (void**)&textsrv);
    ok(hr == S_OK, "QueryInterface for IID_ITextServices failed: %08x\n", hr);
    refcount = ITextServices_AddRef(textsrv);
    ok(refcount == unk_obj.ref, "CreateTextServices just pretends to support COM aggregation\n");
    refcount = ITextServices_Release(textsrv);
    ok(refcount == unk_obj.ref, "CreateTextServices just pretends to support COM aggregation\n");
    refcount = ITextServices_Release(textsrv);
    ok(refcount == 19, "Refcount should be back at 19 but is %u\n", refcount);

    IUnknown_Release(unk_obj.inner_unk);
}

static ULONG get_refcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static void test_QueryInterface(void)
{
    HRESULT hres;
    IRichEditOle *reole, *txtsrv_reole;
    ITextDocument *txtdoc, *txtsrv_txtdoc;
    ULONG refcount;

    if(!init_texthost())
        return;

    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 1, "got wrong ref count: %d\n", refcount);

    /* IID_IRichEditOle */
    hres = ITextServices_QueryInterface(txtserv, &IID_IRichEditOle, (void **)&txtsrv_reole);
    ok(hres == S_OK, "ITextServices_QueryInterface: 0x%08x\n", hres);
    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 2, "got wrong ref count: %d\n", refcount);
    refcount = get_refcount((IUnknown *)txtsrv_reole);
    ok(refcount == 2, "got wrong ref count: %d\n", refcount);

    hres = IRichEditOle_QueryInterface(txtsrv_reole, &IID_ITextDocument, (void **)&txtdoc);
    ok(hres == S_OK, "IRichEditOle_QueryInterface: 0x%08x\n", hres);
    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 3, "got wrong ref count: %d\n", refcount);
    refcount = get_refcount((IUnknown *)txtsrv_reole);
    ok(refcount == 3, "got wrong ref count: %d\n", refcount);

    ITextDocument_Release(txtdoc);
    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 2, "got wrong ref count: %d\n", refcount);
    IRichEditOle_Release(txtsrv_reole);
    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 1, "got wrong ref count: %d\n", refcount);

    /* IID_ITextDocument */
    hres = ITextServices_QueryInterface(txtserv, &IID_ITextDocument, (void **)&txtsrv_txtdoc);
    ok(hres == S_OK, "ITextServices_QueryInterface: 0x%08x\n", hres);
    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 2, "got wrong ref count: %d\n", refcount);
    refcount = get_refcount((IUnknown *)txtsrv_txtdoc);
    ok(refcount == 2, "got wrong ref count: %d\n", refcount);

    hres = ITextDocument_QueryInterface(txtsrv_txtdoc, &IID_IRichEditOle, (void **)&reole);
    ok(hres == S_OK, "ITextDocument_QueryInterface: 0x%08x\n", hres);
    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 3, "got wrong ref count: %d\n", refcount);
    refcount = get_refcount((IUnknown *)txtsrv_txtdoc);
    ok(refcount == 3, "got wrong ref count: %d\n", refcount);

    IRichEditOle_Release(reole);
    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 2, "got wrong ref count: %d\n", refcount);
    ITextDocument_Release(txtsrv_txtdoc);
    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 1, "got wrong ref count: %d\n", refcount);

    free_texthost();
}

START_TEST( txtsrv )
{
    setup_thiscall_wrappers();

    /* Must explicitly LoadLibrary(). The test has no references to functions in
     * RICHED20.DLL, so the linker doesn't actually link to it. */
    hmoduleRichEdit = LoadLibraryA("riched20.dll");
    ok(hmoduleRichEdit != NULL, "error: %d\n", (int) GetLastError());

    pIID_ITextServices = (IID*)GetProcAddress(hmoduleRichEdit, "IID_ITextServices");
    pIID_ITextHost = (IID*)GetProcAddress(hmoduleRichEdit, "IID_ITextHost");
    pIID_ITextHost2 = (IID*)GetProcAddress(hmoduleRichEdit, "IID_ITextHost2");
    pCreateTextServices = (void*)GetProcAddress(hmoduleRichEdit, "CreateTextServices");

    test_IIDs();
    test_COM();

    if (init_texthost())
    {
        free_texthost();

        test_TxGetText();
        test_TxSetText();
        test_TxGetNaturalSize();
        test_TxDraw();
        test_QueryInterface();
    }
    if (wrapperCodeMem) VirtualFree(wrapperCodeMem, 0, MEM_RELEASE);
}
