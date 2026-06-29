/*
 * IMAP Transport
 *
 * Copyright 2008 Hans Leidekker for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winuser.h"
#include "objbase.h"
#include "mimeole.h"
#include "wine/debug.h"

#include "inetcomm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcomm);

typedef struct
{
    InternetTransport InetTransport;
    ULONG refs;
} IMAPTransport;

static HRESULT WINAPI IMAPTransport_QueryInterface(IIMAPTransport *iface, REFIID riid, void **ppv)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IInternetTransport) ||
        IsEqualIID(riid, &IID_IIMAPTransport))
    {
        *ppv = iface;
        IIMAPTransport_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    FIXME("no interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI IMAPTransport_AddRef(IIMAPTransport *iface)
{
    IMAPTransport *This = (IMAPTransport *)iface;
    return InterlockedIncrement((LONG *)&This->refs);
}

static ULONG WINAPI IMAPTransport_Release(IIMAPTransport *iface)
{
    IMAPTransport *This = (IMAPTransport *)iface;
    ULONG refs = InterlockedDecrement((LONG *)&This->refs);
    if (!refs)
    {
        TRACE("destroying %p\n", This);
        if (This->InetTransport.Status != IXP_DISCONNECTED)
            InternetTransport_DropConnection(&This->InetTransport);
        if (This->InetTransport.pCallback) ITransportCallback_Release(This->InetTransport.pCallback);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refs;
}

static HRESULT WINAPI IMAPTransport_GetServerInfo(IIMAPTransport *iface,
    LPINETSERVER pInetServer)
{
    IMAPTransport *This = (IMAPTransport *)iface;

    TRACE("(%p)\n", pInetServer);
    return InternetTransport_GetServerInfo(&This->InetTransport, pInetServer);
}

static IXPTYPE WINAPI IMAPTransport_GetIXPType(IIMAPTransport *iface)
{
    TRACE("()\n");
    return IXP_IMAP;
}

static HRESULT WINAPI IMAPTransport_IsState(IIMAPTransport *iface,
    IXPISSTATE isstate)
{
    FIXME("(%d): stub\n", isstate);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_InetServerFromAccount(
    IIMAPTransport *iface, IImnAccount *pAccount, LPINETSERVER pInetServer)
{
    IMAPTransport *This = (IMAPTransport *)iface;

    TRACE("(%p, %p)\n", pAccount, pInetServer);
    return InternetTransport_InetServerFromAccount(&This->InetTransport, pAccount, pInetServer);
}

static HRESULT WINAPI IMAPTransport_Connect(IIMAPTransport *iface,
    LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging)
{
    IMAPTransport *This = (IMAPTransport *)iface;
    HRESULT hr;

    TRACE("(%p, %s, %s)\n", pInetServer, fAuthenticate ? "TRUE" : "FALSE", fCommandLogging ? "TRUE" : "FALSE");

    hr = InternetTransport_Connect(&This->InetTransport, pInetServer, fAuthenticate, fCommandLogging);
    return hr;
}

static HRESULT WINAPI IMAPTransport_HandsOffCallback(IIMAPTransport *iface)
{
    IMAPTransport *This = (IMAPTransport *)iface;

    TRACE("()\n");
    return InternetTransport_HandsOffCallback(&This->InetTransport);
}

static HRESULT WINAPI IMAPTransport_Disconnect(IIMAPTransport *iface)
{
    FIXME("(): stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_DropConnection(IIMAPTransport *iface)
{
    IMAPTransport *This = (IMAPTransport *)iface;

    TRACE("()\n");
    return InternetTransport_DropConnection(&This->InetTransport);
}

static HRESULT WINAPI IMAPTransport_GetStatus(IIMAPTransport *iface,
    IXPSTATUS *pCurrentStatus)
{
    IMAPTransport *This = (IMAPTransport *)iface;

    TRACE("()\n");
    return InternetTransport_GetStatus(&This->InetTransport, pCurrentStatus);
}

static HRESULT WINAPI IMAPTransport_InitNew(IIMAPTransport *iface,
    LPSTR pszLogFilePath, IIMAPCallback *pCallback)
{
    IMAPTransport *This = (IMAPTransport *)iface;

    TRACE("(%s, %p)\n", debugstr_a(pszLogFilePath), pCallback);

    if (!pCallback)
        return E_INVALIDARG;

    if (pszLogFilePath)
        FIXME("not using log file of %s, use Wine debug logging instead\n",
            debugstr_a(pszLogFilePath));

    IIMAPCallback_AddRef(pCallback);
    This->InetTransport.pCallback = (ITransportCallback *)pCallback;
    This->InetTransport.fInitialised = TRUE;

    return S_OK;
}

static HRESULT WINAPI IMAPTransport_NewIRangeList(IIMAPTransport *iface,
    IRangeList **pprlNewRangeList)
{
    FIXME("(%p): stub\n", pprlNewRangeList);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Capability(IIMAPTransport *iface,
    DWORD *pdwCapabilityFlags)
{
    FIXME("(%p): stub\n", pdwCapabilityFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Select(IIMAPTransport *iface,
    WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler, LPSTR lpszMailboxName)
{
    FIXME("(%Id, %Id, %p, %s): stub\n", wParam, lParam, pCBHandler, debugstr_a(lpszMailboxName));
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Examine(IIMAPTransport *iface,
    WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler, LPSTR lpszMailboxName)
{
    FIXME("(%Id, %Id, %p, %s): stub\n", wParam, lParam, pCBHandler, debugstr_a(lpszMailboxName));
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Create(IIMAPTransport *iface,
    WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler, LPSTR lpszMailboxName)
{
    FIXME("(%Id, %Id, %p, %s): stub\n", wParam, lParam, pCBHandler, debugstr_a(lpszMailboxName));
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Delete(IIMAPTransport *iface,
    WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler, LPSTR lpszMailboxName)
{
    FIXME("(%Id, %Id, %p, %s): stub\n", wParam, lParam, pCBHandler, debugstr_a(lpszMailboxName));
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Rename(IIMAPTransport *iface, WPARAM wParam, LPARAM lParam,
    IIMAPCallback *pCBHandler, LPSTR lpszMailboxName, LPSTR lpszNewMailboxName)
{
    FIXME("(%Id, %Id, %p, %s, %s): stub\n", wParam, lParam, pCBHandler,
          debugstr_a(lpszMailboxName), debugstr_a(lpszNewMailboxName));
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Subscribe(IIMAPTransport *iface,
    WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler, LPSTR lpszMailboxName)
{
    FIXME("(%Id, %Id, %p, %s): stub\n", wParam, lParam, pCBHandler, debugstr_a(lpszMailboxName));
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Unsubscribe(IIMAPTransport *iface,
    WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler, LPSTR lpszMailboxName)
{
    FIXME("(%Id, %Id, %p, %s): stub\n", wParam, lParam, pCBHandler, debugstr_a(lpszMailboxName));
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_List(IIMAPTransport *iface, WPARAM wParam, LPARAM lParam,
    IIMAPCallback *pCBHandler, LPSTR lpszMailboxNameReference, LPSTR lpszMailboxNamePattern)
{
    FIXME("(%Id, %Id, %p, %s, %s): stub\n", wParam, lParam, pCBHandler,
          debugstr_a(lpszMailboxNameReference), debugstr_a(lpszMailboxNamePattern));
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Lsub(IIMAPTransport *iface, WPARAM wParam, LPARAM lParam,
    IIMAPCallback *pCBHandler, LPSTR lpszMailboxNameReference, LPSTR lpszMailboxNamePattern)
{
    FIXME("(%Id, %Id, %p, %s, %s): stub\n", wParam, lParam, pCBHandler,
          debugstr_a(lpszMailboxNameReference), debugstr_a(lpszMailboxNamePattern));
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Append(IIMAPTransport *iface,
    WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler, LPSTR lpszMailboxName,
    LPSTR lpszMessageFlags, FILETIME ftMessageDateTime, LPSTREAM lpstmMessageToSave)
{
    FIXME("(%Id, %Id, %p, %s, %s, %p): stub\n", wParam, lParam, pCBHandler,
          debugstr_a(lpszMailboxName), debugstr_a(lpszMessageFlags), lpstmMessageToSave);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Close(IIMAPTransport *iface,
    WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler)
{
    FIXME("(%Id, %Id, %p): stub\n", wParam, lParam, pCBHandler);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Expunge(IIMAPTransport *iface,
    WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler)
{
    FIXME("(%Id, %Id, %p): stub\n", wParam, lParam, pCBHandler);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Search(IIMAPTransport *iface,
    WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler, LPSTR lpszSearchCriteria,
    boolean bReturnUIDs, IRangeList *pMsgRange, boolean bUIDRangeList)
{
    FIXME("(%Id, %Id, %p, %s, %d, %p, %d): stub\n", wParam, lParam, pCBHandler,
          debugstr_a(lpszSearchCriteria), bReturnUIDs, pMsgRange, bUIDRangeList);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Fetch(IIMAPTransport *iface, WPARAM wParam, LPARAM lParam,
    IIMAPCallback *pCBHandler, IRangeList *pMsgRange, boolean bUIDMsgRange, LPSTR lpszFetchArgs)
{
    FIXME("(%Id, %Id, %p, %p, %d, %s): stub\n", wParam, lParam, pCBHandler, pMsgRange,
          bUIDMsgRange, debugstr_a(lpszFetchArgs));
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Store(IIMAPTransport *iface, WPARAM wParam, LPARAM lParam,
    IIMAPCallback *pCBHandler, IRangeList *pMsgRange, boolean bUIDRangeList, LPSTR lpszStoreArgs)
{
    FIXME("(%Id, %Id, %p, %p, %d, %s): stub\n", wParam, lParam, pCBHandler, pMsgRange,
          bUIDRangeList, debugstr_a(lpszStoreArgs));
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Copy(IIMAPTransport *iface, WPARAM wParam, LPARAM lParam,
    IIMAPCallback *pCBHandler, IRangeList *pMsgRange, boolean bUIDRangeList, LPSTR lpszMailboxName)
{
    FIXME("(%Id, %Id, %p, %p, %d, %s): stub\n", wParam, lParam, pCBHandler, pMsgRange,
          bUIDRangeList, debugstr_a(lpszMailboxName));
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Noop(IIMAPTransport *iface,
    WPARAM wParam, LPARAM lParam, IIMAPCallback *pCBHandler)
{
    FIXME("(%Id, %Id, %p): stub\n", wParam, lParam, pCBHandler);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_ResizeMsgSeqNumTable(IIMAPTransport *iface, DWORD dwSizeOfMbox)
{
    FIXME("(%lu): stub\n", dwSizeOfMbox);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_UpdateSeqNumToUID(IIMAPTransport *iface,
    DWORD dwMsgSeqNum, DWORD dwUID)
{
    FIXME("(%lu, %lu): stub\n", dwMsgSeqNum, dwUID);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_RemoveSequenceNum(IIMAPTransport *iface, DWORD dwDeletedMsgSeqNum)
{
    FIXME("(%lu): stub\n", dwDeletedMsgSeqNum);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_MsgSeqNumToUID(IIMAPTransport *iface, DWORD dwMsgSeqNum,
    DWORD *pdwUID)
{
    FIXME("(%lu, %p): stub\n", dwMsgSeqNum, pdwUID);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_GetMsgSeqNumToUIDArray(IIMAPTransport *iface,
    DWORD **ppdwMsgSeqNumToUIDArray, DWORD *pdwNumberOfElements)
{
    FIXME("(%p, %p): stub\n", ppdwMsgSeqNumToUIDArray, pdwNumberOfElements);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_GetHighestMsgSeqNum(IIMAPTransport *iface, DWORD *pdwHighestMSN)
{
    FIXME("(%p): stub\n", pdwHighestMSN);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_ResetMsgSeqNumToUID(IIMAPTransport *iface)
{
    FIXME("(): stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_SetDefaultCBHandler(IIMAPTransport *iface, IIMAPCallback *pCBHandler)
{
    FIXME("(%p): stub\n", pCBHandler);
    return E_NOTIMPL;
}

static HRESULT WINAPI IMAPTransport_Status(IIMAPTransport *iface, WPARAM wParam, LPARAM lParam,
    IIMAPCallback *pCBHandler, LPSTR pszMailboxName, LPSTR pszStatusCmdArgs)
{
    FIXME("(%Id, %Id, %p, %s, %s): stub\n", wParam, lParam, pCBHandler,
          debugstr_a(pszMailboxName), debugstr_a(pszStatusCmdArgs));
    return E_NOTIMPL;
}

static const IIMAPTransportVtbl IMAPTransportVtbl =
{
    IMAPTransport_QueryInterface,
    IMAPTransport_AddRef,
    IMAPTransport_Release,
    IMAPTransport_GetServerInfo,
    IMAPTransport_GetIXPType,
    IMAPTransport_IsState,
    IMAPTransport_InetServerFromAccount,
    IMAPTransport_Connect,
    IMAPTransport_HandsOffCallback,
    IMAPTransport_Disconnect,
    IMAPTransport_DropConnection,
    IMAPTransport_GetStatus,
    IMAPTransport_InitNew,
    IMAPTransport_NewIRangeList,
    IMAPTransport_Capability,
    IMAPTransport_Select,
    IMAPTransport_Examine,
    IMAPTransport_Create,
    IMAPTransport_Delete,
    IMAPTransport_Rename,
    IMAPTransport_Subscribe,
    IMAPTransport_Unsubscribe,
    IMAPTransport_List,
    IMAPTransport_Lsub,
    IMAPTransport_Append,
    IMAPTransport_Close,
    IMAPTransport_Expunge,
    IMAPTransport_Search,
    IMAPTransport_Fetch,
    IMAPTransport_Store,
    IMAPTransport_Copy,
    IMAPTransport_Noop,
    IMAPTransport_ResizeMsgSeqNumTable,
    IMAPTransport_UpdateSeqNumToUID,
    IMAPTransport_RemoveSequenceNum,
    IMAPTransport_MsgSeqNumToUID,
    IMAPTransport_GetMsgSeqNumToUIDArray,
    IMAPTransport_GetHighestMsgSeqNum,
    IMAPTransport_ResetMsgSeqNumToUID,
    IMAPTransport_SetDefaultCBHandler,
    IMAPTransport_Status
};

HRESULT WINAPI CreateIMAPTransport(IIMAPTransport **ppTransport)
{
    HRESULT hr;
    IMAPTransport *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->InetTransport.u.vtblIMAP = &IMAPTransportVtbl;
    This->refs = 0;
    hr = InternetTransport_Init(&This->InetTransport);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, This);
        return hr;
    }

    *ppTransport = (IIMAPTransport *)&This->InetTransport.u.vtblIMAP;
    IIMAPTransport_AddRef(*ppTransport);

    return S_OK;
}

static HRESULT WINAPI IMAPTransportCF_QueryInterface(IClassFactory *iface,
    REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppv = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI IMAPTransportCF_AddRef(IClassFactory *iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI IMAPTransportCF_Release(IClassFactory *iface)
{
    return 1; /* non-heap based object */
}

static HRESULT WINAPI IMAPTransportCF_CreateInstance(IClassFactory *iface,
    LPUNKNOWN pUnk, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;
    IIMAPTransport *pImapTransport;

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    hr = CreateIMAPTransport(&pImapTransport);
    if (FAILED(hr))
        return hr;

    hr = IIMAPTransport_QueryInterface(pImapTransport, riid, ppv);
    IIMAPTransport_Release(pImapTransport);

    return hr;
}

static HRESULT WINAPI IMAPTransportCF_LockServer(IClassFactory *iface, BOOL fLock)
{
    FIXME("(%d), stub!\n",fLock);
    return S_OK;
}

static const IClassFactoryVtbl IMAPTransportCFVtbl =
{
    IMAPTransportCF_QueryInterface,
    IMAPTransportCF_AddRef,
    IMAPTransportCF_Release,
    IMAPTransportCF_CreateInstance,
    IMAPTransportCF_LockServer
};
static const IClassFactoryVtbl *IMAPTransportCF = &IMAPTransportCFVtbl;

HRESULT IMAPTransportCF_Create(REFIID riid, LPVOID *ppv)
{
    return IClassFactory_QueryInterface((IClassFactory *)&IMAPTransportCF, riid, ppv);
}
