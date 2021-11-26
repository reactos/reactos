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
#include <imm.h>
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
#if defined(__i386__) && !defined(__MINGW32__)
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
    CHARFORMAT2W char_format;
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
        *ppvObject = &This->ITextHost_iface;
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

static HDC __thiscall ITextHostImpl_TxGetDC(ITextHost *iface)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetDC(%p)\n", This);
    return NULL;
}

static INT __thiscall ITextHostImpl_TxReleaseDC(ITextHost *iface, HDC hdc)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxReleaseDC(%p)\n", This);
    return 0;
}

static BOOL __thiscall ITextHostImpl_TxShowScrollBar(ITextHost *iface, INT fnBar, BOOL fShow)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxShowScrollBar(%p, fnBar=%d, fShow=%d)\n",
                This, fnBar, fShow);
    return FALSE;
}

static BOOL __thiscall ITextHostImpl_TxEnableScrollBar(ITextHost *iface, INT fuSBFlags, INT fuArrowflags)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxEnableScrollBar(%p, fuSBFlags=%d, fuArrowflags=%d)\n",
               This, fuSBFlags, fuArrowflags);
    return FALSE;
}

static BOOL __thiscall ITextHostImpl_TxSetScrollRange(ITextHost *iface, INT fnBar, LONG nMinPos,
                                                      INT nMaxPos, BOOL fRedraw)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetScrollRange(%p, fnBar=%d, nMinPos=%d, nMaxPos=%d, fRedraw=%d)\n",
               This, fnBar, nMinPos, nMaxPos, fRedraw);
    return FALSE;
}

static BOOL __thiscall ITextHostImpl_TxSetScrollPos(ITextHost *iface, INT fnBar, INT nPos, BOOL fRedraw)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetScrollPos(%p, fnBar=%d, nPos=%d, fRedraw=%d)\n",
               This, fnBar, nPos, fRedraw);
    return FALSE;
}

static void __thiscall ITextHostImpl_TxInvalidateRect(ITextHost *iface, LPCRECT prc, BOOL fMode)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxInvalidateRect(%p, prc=%p, fMode=%d)\n",
               This, prc, fMode);
}

static void __thiscall ITextHostImpl_TxViewChange(ITextHost *iface, BOOL fUpdate)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxViewChange(%p, fUpdate=%d)\n",
               This, fUpdate);
}

static BOOL __thiscall ITextHostImpl_TxCreateCaret(ITextHost *iface, HBITMAP hbmp, INT xWidth, INT yHeight)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxCreateCaret(%p, nbmp=%p, xWidth=%d, yHeight=%d)\n",
               This, hbmp, xWidth, yHeight);
    return FALSE;
}

static BOOL __thiscall ITextHostImpl_TxShowCaret(ITextHost *iface, BOOL fShow)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxShowCaret(%p, fShow=%d)\n",
               This, fShow);
    return FALSE;
}

static BOOL __thiscall ITextHostImpl_TxSetCaretPos(ITextHost *iface, INT x, INT y)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetCaretPos(%p, x=%d, y=%d)\n", This, x, y);
    return FALSE;
}

static BOOL __thiscall ITextHostImpl_TxSetTimer(ITextHost *iface, UINT idTimer, UINT uTimeout)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetTimer(%p, idTimer=%u, uTimeout=%u)\n",
              This, idTimer, uTimeout);
    return FALSE;
}

static void __thiscall ITextHostImpl_TxKillTimer(ITextHost *iface, UINT idTimer)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxKillTimer(%p, idTimer=%u)\n", This, idTimer);
}

static void __thiscall ITextHostImpl_TxScrollWindowEx(ITextHost *iface, INT dx, INT dy, LPCRECT lprcScroll,
                                                      LPCRECT lprcClip, HRGN hRgnUpdate, LPRECT lprcUpdate,
                                                      UINT fuScroll)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxScrollWindowEx(%p, %d, %d, %p, %p, %p, %p, %d)\n",
              This, dx, dy, lprcScroll, lprcClip, hRgnUpdate, lprcUpdate, fuScroll);
}

static void __thiscall ITextHostImpl_TxSetCapture(ITextHost *iface, BOOL fCapture)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetCapture(%p, fCapture=%d)\n", This, fCapture);
}

static void __thiscall ITextHostImpl_TxSetFocus(ITextHost *iface)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetFocus(%p)\n", This);
}

static void __thiscall ITextHostImpl_TxSetCursor(ITextHost *iface, HCURSOR hcur, BOOL fText)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxSetCursor(%p, hcur=%p, fText=%d)\n",
              This, hcur, fText);
}

static BOOL __thiscall ITextHostImpl_TxScreenToClient(ITextHost *iface, LPPOINT lppt)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxScreenToClient(%p, lppt=%p)\n", This, lppt);
    return FALSE;
}

static BOOL __thiscall ITextHostImpl_TxClientToScreen(ITextHost *iface, LPPOINT lppt)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxClientToScreen(%p, lppt=%p)\n", This, lppt);
    return FALSE;
}

static HRESULT __thiscall ITextHostImpl_TxActivate(ITextHost *iface, LONG *plOldState)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxActivate(%p, plOldState=%p)\n", This, plOldState);
    return E_NOTIMPL;
}

static HRESULT __thiscall ITextHostImpl_TxDeactivate(ITextHost *iface, LONG lNewState)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxDeactivate(%p, lNewState=%d)\n", This, lNewState);
    return E_NOTIMPL;
}

static HRESULT __thiscall ITextHostImpl_TxGetClientRect(ITextHost *iface, LPRECT prc)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetClientRect(%p, prc=%p)\n", This, prc);
    return E_NOTIMPL;
}

static HRESULT __thiscall ITextHostImpl_TxGetViewInset(ITextHost *iface, LPRECT prc)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetViewInset(%p, prc=%p)\n", This, prc);
    return E_NOTIMPL;
}

static HRESULT __thiscall ITextHostImpl_TxGetCharFormat(ITextHost *iface, const CHARFORMATW **ppCF)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetCharFormat(%p, ppCF=%p)\n", This, ppCF);
    *ppCF = (CHARFORMATW *)&This->char_format;
    return S_OK;
}

static HRESULT __thiscall ITextHostImpl_TxGetParaFormat(ITextHost *iface, const PARAFORMAT **ppPF)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetParaFormat(%p, ppPF=%p)\n", This, ppPF);
    return E_NOTIMPL;
}

static COLORREF __thiscall ITextHostImpl_TxGetSysColor(ITextHost *iface, int nIndex)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetSysColor(%p, nIndex=%d)\n", This, nIndex);
    return E_NOTIMPL;
}

static HRESULT __thiscall ITextHostImpl_TxGetBackStyle(ITextHost *iface, TXTBACKSTYLE *pStyle)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetBackStyle(%p, pStyle=%p)\n", This, pStyle);
    return E_NOTIMPL;
}

static HRESULT __thiscall ITextHostImpl_TxGetMaxLength(ITextHost *iface, DWORD *pLength)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetMaxLength(%p, pLength=%p)\n", This, pLength);
    return E_NOTIMPL;
}

static HRESULT __thiscall ITextHostImpl_TxGetScrollBars(ITextHost *iface, DWORD *pdwScrollBar)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetScrollBars(%p, pdwScrollBar=%p)\n",
               This, pdwScrollBar);
    return E_NOTIMPL;
}

static HRESULT __thiscall ITextHostImpl_TxGetPasswordChar(ITextHost *iface, WCHAR *pch)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetPasswordChar(%p, pch=%p)\n", This, pch);
    return E_NOTIMPL;
}

static HRESULT __thiscall ITextHostImpl_TxGetAcceleratorPos(ITextHost *iface, LONG *pch)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetAcceleratorPos(%p, pch=%p)\n", This, pch);
    return E_NOTIMPL;
}

static HRESULT __thiscall ITextHostImpl_TxGetExtent(ITextHost *iface, LPSIZEL lpExtent)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetExtent(%p, lpExtent=%p)\n", This, lpExtent);
    return E_NOTIMPL;
}

static HRESULT __thiscall ITextHostImpl_OnTxCharFormatChange(ITextHost *iface, const CHARFORMATW *pcf)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to OnTxCharFormatChange(%p, pcf=%p)\n", This, pcf);
    return E_NOTIMPL;
}

static HRESULT __thiscall ITextHostImpl_OnTxParaFormatChange(ITextHost *iface, const PARAFORMAT *ppf)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to OnTxParaFormatChange(%p, ppf=%p)\n", This, ppf);
    return E_NOTIMPL;
}

/* This must return S_OK for the native ITextServices object to
   initialize. */
static HRESULT __thiscall ITextHostImpl_TxGetPropertyBits(ITextHost *iface, DWORD dwMask, DWORD *pdwBits)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxGetPropertyBits(%p, dwMask=0x%08x, pdwBits=%p)\n",
              This, dwMask, pdwBits);
    *pdwBits = 0;
    return S_OK;
}

static HRESULT __thiscall ITextHostImpl_TxNotify(ITextHost *iface, DWORD iNotify, void *pv)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxNotify(%p, iNotify=%d, pv=%p)\n", This, iNotify, pv);
    return E_NOTIMPL;
}

static HIMC __thiscall ITextHostImpl_TxImmGetContext(ITextHost *iface)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxImmGetContext(%p)\n", This);
    return 0;
}

static void __thiscall ITextHostImpl_TxImmReleaseContext(ITextHost *iface, HIMC himc)
{
    ITextHostTestImpl *This = impl_from_ITextHost(iface);
    TRACECALL("Call to TxImmReleaseContext(%p, himc=%p)\n", This, himc);
}

/* This function must set the variable pointed to by *lSelBarWidth.
   Otherwise an uninitialized value will be used to calculate
   positions and sizes even if E_NOTIMPL is returned. */
static HRESULT __thiscall ITextHostImpl_TxGetSelectionBarWidth(ITextHost *iface, LONG *lSelBarWidth)
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
#if defined(__i386__) && !defined(__MINGW32__)
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

static void hf_to_cf(HFONT hf, CHARFORMAT2W *cf)
{
    LOGFONTW lf;

    GetObjectW(hf, sizeof(lf), &lf);
    lstrcpyW(cf->szFaceName, lf.lfFaceName);
    cf->yHeight = MulDiv(abs(lf.lfHeight), 1440, GetDeviceCaps(GetDC(NULL), LOGPIXELSY));
    if (lf.lfWeight > FW_NORMAL) cf->dwEffects |= CFE_BOLD;
    if (lf.lfItalic) cf->dwEffects |= CFE_ITALIC;
    if (lf.lfUnderline) cf->dwEffects |= CFE_UNDERLINE;
    if (lf.lfStrikeOut) cf->dwEffects |= CFE_SUBSCRIPT;
    cf->bPitchAndFamily = lf.lfPitchAndFamily;
    cf->bCharSet = lf.lfCharSet;
}

/*************************************************************************/
/* Conformance test functions. */

/* Initialize the test texthost structure */
static BOOL init_texthost(ITextServices **txtserv, ITextHost **ret)
{
    ITextHostTestImpl *dummyTextHost;
    IUnknown *init;
    HRESULT result;
    HFONT hf;

    dummyTextHost = CoTaskMemAlloc(sizeof(*dummyTextHost));
    if (dummyTextHost == NULL) {
        win_skip("Insufficient memory to create ITextHost interface\n");
        return FALSE;
    }
    dummyTextHost->ITextHost_iface.lpVtbl = &itextHostVtbl;
    dummyTextHost->refCount = 1;
    memset(&dummyTextHost->char_format, 0, sizeof(dummyTextHost->char_format));
    dummyTextHost->char_format.cbSize = sizeof(dummyTextHost->char_format);
    dummyTextHost->char_format.dwMask = CFM_ALL2;
    hf = GetStockObject(DEFAULT_GUI_FONT);
    hf_to_cf(hf, &dummyTextHost->char_format);

    /* MSDN states that an IUnknown object is returned by
       CreateTextServices which is then queried to obtain a
       ITextServices object. */
    result = pCreateTextServices(NULL, &dummyTextHost->ITextHost_iface, &init);
    ok(result == S_OK, "Did not return S_OK when created (result =  %x)\n", result);
    if (result != S_OK) {
        CoTaskMemFree(dummyTextHost);
        win_skip("CreateTextServices failed.\n");
        return FALSE;
    }

    result = IUnknown_QueryInterface(init, pIID_ITextServices, (void**)txtserv);
    ok((result == S_OK) && (*txtserv != NULL), "Querying interface failed (result = %x, txtserv = %p)\n", result, *txtserv);
    IUnknown_Release(init);
    if (!((result == S_OK) && (*txtserv != NULL))) {
        CoTaskMemFree(dummyTextHost);
        win_skip("Could not retrieve ITextServices interface\n");
        return FALSE;
    }

    *ret = &dummyTextHost->ITextHost_iface;
    return TRUE;
}

static void test_TxGetText(void)
{
    ITextServices *txtserv;
    ITextHost *host;
    HRESULT hres;
    BSTR rettext;

    if (!init_texthost(&txtserv, &host))
        return;

    hres = ITextServices_TxGetText(txtserv, &rettext);
    ok(hres == S_OK, "ITextServices_TxGetText failed (result = %x)\n", hres);
    SysFreeString(rettext);

    ITextServices_Release(txtserv);
    ITextHost_Release(host);
}

static void test_TxSetText(void)
{
    ITextServices *txtserv;
    ITextHost *host;
    HRESULT hres;
    BSTR rettext;
    WCHAR settext[] = {'T','e','s','t',0};

    if (!init_texthost(&txtserv, &host))
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

    /* Null-pointer should behave the same as empty-string */

    hres = ITextServices_TxSetText(txtserv, 0);
    ok(hres == S_OK, "ITextServices_TxSetText failed (result = %x)\n", hres);

    hres = ITextServices_TxGetText(txtserv, &rettext);
    ok(hres == S_OK, "ITextServices_TxGetText failed (result = %x)\n", hres);
    ok(SysStringLen(rettext) == 0,
                 "String returned of wrong length (expected 0, got %d)\n", SysStringLen(rettext));

    SysFreeString(rettext);
    ITextServices_Release(txtserv);
    ITextHost_Release(host);
}

#define CHECK_TXGETNATURALSIZE(res,width,height,hdc,rect,string) \
    _check_txgetnaturalsize(res, width, height, hdc, rect, string, __LINE__)
static void _check_txgetnaturalsize(HRESULT res, LONG width, LONG height, HDC hdc, RECT rect, LPCWSTR string, int line)
{
    RECT expected_rect = rect;
    LONG expected_width, expected_height;

    DrawTextW(hdc, string, -1, &expected_rect, DT_LEFT | DT_CALCRECT | DT_NOCLIP | DT_EDITCONTROL | DT_WORDBREAK);
    expected_width = expected_rect.right - expected_rect.left;
    expected_height = expected_rect.bottom - expected_rect.top;
    ok_(__FILE__,line)(res == S_OK, "ITextServices_TxGetNaturalSize failed: 0x%08x.\n", res);
    ok_(__FILE__,line)(width >= expected_width && width <= expected_width + 1,
                       "got wrong width: %d, expected: %d {+1}.\n", width, expected_width);
    ok_(__FILE__,line)(height == expected_height, "got wrong height: %d, expected: %d.\n",
                       height, expected_height);
}

static void test_TxGetNaturalSize(void)
{
    ITextServices *txtserv;
    ITextHost *host;
    HRESULT result;
    SIZEL extent;
    static const WCHAR test_text[] = {'T','e','s','t','S','o','m','e','T','e','x','t',0};
    LONG width, height;
    HDC hdcDraw;
    HWND hwnd;
    RECT rect;
    CHARFORMAT2W cf;
    LRESULT lresult;
    HFONT hf;

    if (!init_texthost(&txtserv, &host))
        return;

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP | WS_VISIBLE,
                           0, 0, 100, 100, 0, 0, 0, NULL);
    hdcDraw = GetDC(hwnd);
    SetMapMode(hdcDraw,MM_TEXT);
    GetClientRect(hwnd, &rect);

    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_ALL2;
    hf = GetStockObject(DEFAULT_GUI_FONT);
    hf_to_cf(hf, &cf);
    result = ITextServices_TxSendMessage(txtserv, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf, &lresult);
    ok(result == S_OK, "ITextServices_TxSendMessage failed: 0x%08x.\n", result);
    SelectObject(hdcDraw, hf);

    result = ITextServices_TxSetText(txtserv, test_text);
    ok(result == S_OK, "ITextServices_TxSetText failed: 0x%08x.\n", result);

    extent.cx = -1; extent.cy = -1;
    width = rect.right - rect.left;
    height = 0;
    result = ITextServices_TxGetNaturalSize(txtserv, DVASPECT_CONTENT, hdcDraw, NULL, NULL,
                                            TXTNS_FITTOCONTENT, &extent, &width, &height);
    todo_wine CHECK_TXGETNATURALSIZE(result, width, height, hdcDraw, rect, test_text);

    ReleaseDC(hwnd, hdcDraw);
    DestroyWindow(hwnd);
    ITextServices_Release(txtserv);
    ITextHost_Release(host);
}

static void test_TxDraw(void)
{
    ITextServices *txtserv;
    ITextHost *host;
    HDC tmphdc = GetDC(NULL);
    DWORD dwAspect = DVASPECT_CONTENT;
    HDC hicTargetDev = NULL; /* Means "default" device */
    DVTARGETDEVICE *ptd = NULL;
    void *pvAspect = NULL;
    HRESULT result;
    RECTL client = {0,0,100,100};


    if (!init_texthost(&txtserv, &host))
        return;

    todo_wine {
        result = ITextServices_TxDraw(txtserv, dwAspect, 0, pvAspect, ptd,
                                      tmphdc, hicTargetDev, &client, NULL,
                                      NULL, NULL, 0, 0);
        ok(result == S_OK, "TxDraw failed (result = %x)\n", result);
    }

    ITextServices_Release(txtserv);
    ITextHost_Release(host);
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
    ITextServices *txtserv;
    ITextHost *host;
    HRESULT hres;
    IRichEditOle *reole, *txtsrv_reole;
    ITextDocument *txtdoc, *txtsrv_txtdoc;
    ITextDocument2Old *txtdoc2old, *txtsrv_txtdoc2old;
    ULONG refcount;

    if(!init_texthost(&txtserv, &host))
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

    hres = IRichEditOle_QueryInterface(txtsrv_reole, &IID_ITextDocument2Old, (void **)&txtdoc2old);
    ok(hres == S_OK, "IRichEditOle_QueryInterface: 0x%08x\n", hres);
    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 3, "got wrong ref count: %d\n", refcount);
    refcount = get_refcount((IUnknown *)txtsrv_reole);
    ok(refcount == 3, "got wrong ref count: %d\n", refcount);

    ITextDocument2Old_Release(txtdoc2old);
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

    /* ITextDocument2Old */
    hres = ITextServices_QueryInterface(txtserv, &IID_ITextDocument2Old, (void **)&txtsrv_txtdoc2old);
    ok(hres == S_OK, "ITextServices_QueryInterface: 0x%08x\n", hres);
    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 2, "got wrong ref count: %d\n", refcount);
    refcount = get_refcount((IUnknown *)txtsrv_txtdoc2old);
    ok(refcount == 2, "got wrong ref count: %d\n", refcount);

    hres = ITextDocument2Old_QueryInterface(txtsrv_txtdoc2old, &IID_IRichEditOle, (void **)&reole);
    ok(hres == S_OK, "ITextDocument2Old_QueryInterface: 0x%08x\n", hres);
    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 3, "got wrong ref count: %d\n", refcount);
    refcount = get_refcount((IUnknown *)txtsrv_txtdoc2old);
    ok(refcount == 3, "got wrong ref count: %d\n", refcount);

    IRichEditOle_Release(reole);
    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 2, "got wrong ref count: %d\n", refcount);
    ITextDocument2Old_Release(txtsrv_txtdoc2old);
    refcount = get_refcount((IUnknown *)txtserv);
    ok(refcount == 1, "got wrong ref count: %d\n", refcount);

    ITextServices_Release(txtserv);
    ITextHost_Release(host);
}

static void test_default_format(void)
{
    ITextServices *txtserv;
    ITextHost *host;
    HRESULT result;
    LRESULT lresult;
    CHARFORMAT2W cf2;
    const CHARFORMATW *host_cf;
    DWORD expected_effects;

    if (!init_texthost(&txtserv, &host))
        return;

    cf2.cbSize = sizeof(CHARFORMAT2W);
    result = ITextServices_TxSendMessage(txtserv, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf2, &lresult);
    ok(result == S_OK, "ITextServices_TxSendMessage failed: 0x%08x.\n", result);

    ITextHostImpl_TxGetCharFormat(host, &host_cf);
    ok(!lstrcmpW(host_cf->szFaceName, cf2.szFaceName), "got wrong font name: %s.\n", wine_dbgstr_w(cf2.szFaceName));
    ok(cf2.yHeight == host_cf->yHeight, "got wrong yHeight: %d, expected %d.\n", cf2.yHeight, host_cf->yHeight);
    expected_effects = (cf2.dwEffects & ~(CFE_AUTOCOLOR | CFE_AUTOBACKCOLOR));
    ok(host_cf->dwEffects == expected_effects, "got wrong dwEffects: %x, expected %x.\n", cf2.dwEffects, expected_effects);
    ok(cf2.bPitchAndFamily == host_cf->bPitchAndFamily, "got wrong bPitchAndFamily: %x, expected %x.\n",
       cf2.bPitchAndFamily, host_cf->bPitchAndFamily);
    ok(cf2.bCharSet == host_cf->bCharSet, "got wrong bCharSet: %x, expected %x.\n", cf2.bCharSet, host_cf->bCharSet);

    ITextServices_Release(txtserv);
    ITextHost_Release(host);
}

static void test_TxGetScroll(void)
{
    ITextServices *txtserv;
    ITextHost *host;
    HRESULT ret;

    if (!init_texthost(&txtserv, &host))
        return;

    ret = ITextServices_TxGetHScroll(txtserv, NULL, NULL, NULL, NULL, NULL);
    ok(ret == S_OK, "ITextSerHices_GetVScroll failed: 0x%08x.\n", ret);

    ret = ITextServices_TxGetVScroll(txtserv, NULL, NULL, NULL, NULL, NULL);
    ok(ret == S_OK, "ITextServices_GetVScroll failed: 0x%08x.\n", ret);

    ITextServices_Release(txtserv);
    ITextHost_Release(host);
}

START_TEST( txtsrv )
{
    ITextServices *txtserv;
    ITextHost *host;

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

    if (init_texthost(&txtserv, &host))
    {
        ITextServices_Release(txtserv);
        ITextHost_Release(host);

        test_TxGetText();
        test_TxSetText();
        test_TxGetNaturalSize();
        test_TxDraw();
        test_QueryInterface();
        test_default_format();
        test_TxGetScroll();
    }
    if (wrapperCodeMem) VirtualFree(wrapperCodeMem, 0, MEM_RELEASE);
}
