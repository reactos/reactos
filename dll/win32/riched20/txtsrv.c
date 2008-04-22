/*
 * RichEdit - functions and interfaces around CreateTextServices
 *
 * Copyright 2005, 2006, Maarten Lankhorst
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

#include "config.h"
#include "wine/port.h"

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#define COBJMACROS

#include "editor.h"
#include "ole2.h"
#include "richole.h"
#include "imm.h"
#include "textserv.h"
#include "wine/debug.h"
#include "editstr.h"

#ifdef __i386__  /* thiscall functions are i386-specific */

#define THISCALL(func) __thiscall_ ## func
#define DEFINE_THISCALL_WRAPPER(func) \
   extern typeof(func) THISCALL(func); \
   __ASM_GLOBAL_FUNC(__thiscall_ ## func, \
                   "popl %eax\n\t" \
                   "pushl %ecx\n\t" \
                   "pushl %eax\n\t" \
                   "jmp " __ASM_NAME(#func) )
#else /* __i386__ */

#define THISCALL(func) func
#define DEFINE_THISCALL_WRAPPER(func) /* nothing */

#endif /* __i386__ */

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

typedef struct ITextServicesImpl {
   const ITextServicesVtbl *lpVtbl;
   ITextHost *pMyHost;
   LONG ref;
   CRITICAL_SECTION csTxtSrv;
} ITextServicesImpl;

static const ITextServicesVtbl textservices_Vtbl;

/******************************************************************
 *        CreateTextServices (RICHED20.4)
 */
HRESULT WINAPI CreateTextServices(IUnknown  * pUnkOuter,
                                  ITextHost * pITextHost,
                                  IUnknown  **ppUnk)
{
   ITextServicesImpl *ITextImpl;
   TRACE("%p %p --> %p\n", pUnkOuter, pITextHost, ppUnk);
   if (pITextHost == NULL)
      return E_POINTER;

   ITextImpl = CoTaskMemAlloc(sizeof(*ITextImpl));
   if (ITextImpl == NULL)
      return E_OUTOFMEMORY;
   InitializeCriticalSection(&ITextImpl->csTxtSrv);
   ITextImpl->csTxtSrv.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ITextServicesImpl.csTxtSrv");
   ITextImpl->ref = 1;
   ITextHost_AddRef(pITextHost);
   ITextImpl->pMyHost = pITextHost;
   ITextImpl->lpVtbl = &textservices_Vtbl;

   if (pUnkOuter)
   {
      FIXME("Support aggregation\n");
      return CLASS_E_NOAGGREGATION;
   }

   *ppUnk = (IUnknown *)ITextImpl;
   return S_OK;
}

#define ICOM_THIS_MULTI(impl,field,iface) \
	            impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

static HRESULT WINAPI fnTextSrv_QueryInterface(ITextServices * iface,
                                               REFIID riid,
                                               LPVOID * ppv)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);
   TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppv);
   *ppv = NULL;
   if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ITextServices))
      *ppv = (LPVOID)This;

   if (*ppv)
   {
      IUnknown_AddRef((IUnknown *)(*ppv));
      TRACE ("-- Interface = %p\n", *ppv);
      return S_OK;
   }
   FIXME("Unknown interface: %s\n", debugstr_guid(riid));
   return E_NOINTERFACE;
}

static ULONG WINAPI fnTextSrv_AddRef(ITextServices *iface)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);
   DWORD ref = InterlockedIncrement(&This->ref);

   TRACE("(%p/%p)->() AddRef from %d\n", This, iface, ref - 1);
   return ref;
}

static ULONG WINAPI fnTextSrv_Release(ITextServices *iface)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);
   DWORD ref = InterlockedDecrement(&This->ref);

   TRACE("(%p/%p)->() Release from %d\n", This, iface, ref + 1);

   if (!ref)
   {
      ITextHost_Release(This->pMyHost);
      This->csTxtSrv.DebugInfo->Spare[0] = 0;
      DeleteCriticalSection(&This->csTxtSrv);
      CoTaskMemFree(This);
   }
   return ref;
}

HRESULT WINAPI fnTextSrv_TxSendMessage(ITextServices *iface,
                                       UINT msg,
                                       WPARAM wparam,
                                       LPARAM lparam,
                                       LRESULT* plresult)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_TxDraw(ITextServices *iface,
                                DWORD dwDrawAspect,
                                LONG lindex,
                                void* pvAspect,
                                DVTARGETDEVICE* ptd,
                                HDC hdcDraw,
                                HDC hdcTargetDev,
                                LPCRECTL lprcBounds,
                                LPCRECTL lprcWBounds,
                                LPRECT lprcUpdate,
                                BOOL (CALLBACK * pfnContinue)(DWORD),
                                DWORD dwContinue,
                                LONG lViewId)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_TxGetHScroll(ITextServices *iface,
                                      LONG* plMin,
                                      LONG* plMax,
                                      LONG* plPos,
                                      LONG* plPage,
                                      BOOL* pfEnabled)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_TxGetVScroll(ITextServices *iface,
                                      LONG* plMin,
                                      LONG* plMax,
                                      LONG* plPos,
                                      LONG* plPage,
                                      BOOL* pfEnabled)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_OnTxSetCursor(ITextServices *iface,
                                       DWORD dwDrawAspect,
                                       LONG lindex,
                                       void* pvAspect,
                                       DVTARGETDEVICE* ptd,
                                       HDC hdcDraw,
                                       HDC hicTargetDev,
                                       LPCRECT lprcClient,
                                       INT x, INT y)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_TxQueryHitPoint(ITextServices *iface,
                                         DWORD dwDrawAspect,
                                         LONG lindex,
                                         void* pvAspect,
                                         DVTARGETDEVICE* ptd,
                                         HDC hdcDraw,
                                         HDC hicTargetDev,
                                         LPCRECT lprcClient,
                                         INT x, INT y,
                                         DWORD* pHitResult)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_OnTxInplaceActivate(ITextServices *iface,
                                             LPCRECT prcClient)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_OnTxInplaceDeactivate(ITextServices *iface)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_OnTxUIActivate(ITextServices *iface)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_OnTxUIDeactivate(ITextServices *iface)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_TxGetText(ITextServices *iface,
                                   BSTR* pbstrText)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_TxSetText(ITextServices *iface,
                                   LPCWSTR pszText)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_TxGetCurrentTargetX(ITextServices *iface,
                                             LONG* x)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_TxGetBaseLinePos(ITextServices *iface,
                                          LONG* x)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_TxGetNaturalSize(ITextServices *iface,
                                          DWORD dwAspect,
                                          HDC hdcDraw,
                                          HDC hicTargetDev,
                                          DVTARGETDEVICE* ptd,
                                          DWORD dwMode,
                                          const SIZEL* psizelExtent,
                                          LONG* pwidth,
                                          LONG* pheight)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_TxGetDropTarget(ITextServices *iface,
                                         IDropTarget** ppDropTarget)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_OnTxPropertyBitsChange(ITextServices *iface,
                                                DWORD dwMask,
                                                DWORD dwBits)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

HRESULT WINAPI fnTextSrv_TxGetCachedSize(ITextServices *iface,
                                         DWORD* pdwWidth,
                                         DWORD* pdwHeight)
{
   ICOM_THIS_MULTI(ITextServicesImpl, lpVtbl, iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxSendMessage)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxDraw)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetHScroll)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetVScroll)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxSetCursor)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxQueryHitPoint)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxInplaceActivate)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxInplaceDeactivate)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxUIActivate)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxUIDeactivate)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetText)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxSetText)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetCurrentTargetX)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetBaseLinePos)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetNaturalSize)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetDropTarget)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxPropertyBitsChange)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetCachedSize)

static const ITextServicesVtbl textservices_Vtbl =
{
   fnTextSrv_QueryInterface,
   fnTextSrv_AddRef,
   fnTextSrv_Release,
   THISCALL(fnTextSrv_TxSendMessage),
   THISCALL(fnTextSrv_TxDraw),
   THISCALL(fnTextSrv_TxGetHScroll),
   THISCALL(fnTextSrv_TxGetVScroll),
   THISCALL(fnTextSrv_OnTxSetCursor),
   THISCALL(fnTextSrv_TxQueryHitPoint),
   THISCALL(fnTextSrv_OnTxInplaceActivate),
   THISCALL(fnTextSrv_OnTxInplaceDeactivate),
   THISCALL(fnTextSrv_OnTxUIActivate),
   THISCALL(fnTextSrv_OnTxUIDeactivate),
   THISCALL(fnTextSrv_TxGetText),
   THISCALL(fnTextSrv_TxSetText),
   THISCALL(fnTextSrv_TxGetCurrentTargetX),
   THISCALL(fnTextSrv_TxGetBaseLinePos),
   THISCALL(fnTextSrv_TxGetNaturalSize),
   THISCALL(fnTextSrv_TxGetDropTarget),
   THISCALL(fnTextSrv_OnTxPropertyBitsChange),
   THISCALL(fnTextSrv_TxGetCachedSize)
};
