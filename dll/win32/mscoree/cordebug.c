/*
 *
 * Copyright 2011 Alistair Leslie-Hughes
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

#include "windef.h"
#include "winbase.h"

#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"
#include "shellapi.h"
#include "mscoree.h"
#include "corhdr.h"
#include "metahost.h"
#include "cordebug.h"
#include "wine/list.h"
#include "mscoree_private.h"
#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL( mscoree );

typedef struct DebugProcess
{
    ICorDebugProcess ICorDebugProcess_iface;

    CorDebug *cordebug;

    DWORD dwProcessID;
    HANDLE handle;
    HANDLE thread;

    LONG ref;
} DebugProcess;

static inline CorDebug *impl_from_ICorDebug( ICorDebug *iface )
{
    return CONTAINING_RECORD(iface, CorDebug, ICorDebug_iface);
}

static inline CorDebug *impl_from_ICorDebugProcessEnum( ICorDebugProcessEnum *iface )
{
    return CONTAINING_RECORD(iface, CorDebug, ICorDebugProcessEnum_iface);
}

static inline DebugProcess *impl_from_ICorDebugProcess( ICorDebugProcess *iface )
{
    return CONTAINING_RECORD(iface, DebugProcess, ICorDebugProcess_iface);
}

/* ICorDebugProcess Interface */
static HRESULT WINAPI cordebugprocess_QueryInterface(ICorDebugProcess *iface,
                REFIID riid, void **ppvObject)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_ICorDebugProcess ) ||
         IsEqualGUID( riid, &IID_ICorDebugController ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &This->ICorDebugProcess_iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ICorDebugProcess_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI cordebugprocess_AddRef(ICorDebugProcess *iface)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p ref=%lu\n", This, ref);

    return ref;
}

static ULONG WINAPI cordebugprocess_Release(ICorDebugProcess *iface)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p ref=%lu\n", This, ref);

    if (ref == 0)
    {
        if(This->handle)
            CloseHandle(This->handle);

        if(This->thread)
            CloseHandle(This->thread);

        if(This->cordebug)
            ICorDebug_Release(&This->cordebug->ICorDebug_iface);

        free(This);
    }

    return ref;
}

static HRESULT WINAPI cordebugprocess_Stop(ICorDebugProcess *iface, DWORD dwTimeoutIgnored)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_Continue(ICorDebugProcess *iface, BOOL fIsOutOfBand)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    TRACE("%p\n", This);

    if(This->thread)
        ResumeThread(This->thread);

    return S_OK;
}

static HRESULT WINAPI cordebugprocess_IsRunning(ICorDebugProcess *iface, BOOL *pbRunning)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_HasQueuedCallbacks(ICorDebugProcess *iface,
                ICorDebugThread *pThread, BOOL *pbQueued)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_EnumerateThreads(ICorDebugProcess *iface,
                ICorDebugThreadEnum **ppThreads)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_SetAllThreadsDebugState(ICorDebugProcess *iface,
                CorDebugThreadState state, ICorDebugThread *pExceptThisThread)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_Detach(ICorDebugProcess *iface)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_Terminate(ICorDebugProcess *iface, UINT exitCode)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    BOOL ret = TRUE;

    TRACE("%p\n", This);

    if(This->handle)
    {
        ret = TerminateProcess(This->handle, exitCode);
        CloseHandle(This->handle);
        This->handle = NULL;
    }
    return ret ? S_OK : E_FAIL;
}

static HRESULT WINAPI cordebugprocess_CanCommitChanges(ICorDebugProcess *iface,
                ULONG cSnapshots, ICorDebugEditAndContinueSnapshot * pSnapshots[],
                ICorDebugErrorInfoEnum **pError)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_CommitChanges(ICorDebugProcess *iface,
                ULONG cSnapshots, ICorDebugEditAndContinueSnapshot * pSnapshots[],
                ICorDebugErrorInfoEnum **pError)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_GetID(ICorDebugProcess *iface, DWORD *pdwProcessId)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    TRACE("%p\n", This);

    if(!pdwProcessId)
        return E_INVALIDARG;

    *pdwProcessId = This->dwProcessID;

    return S_OK;
}

static HRESULT WINAPI cordebugprocess_GetHandle(ICorDebugProcess *iface, HPROCESS *phProcessHandle)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    TRACE("%p\n", This);

    if(!phProcessHandle)
        return E_INVALIDARG;

    *phProcessHandle = This->handle;

    return S_OK;
}

static HRESULT WINAPI cordebugprocess_GetThread(ICorDebugProcess *iface, DWORD dwThreadId,
                ICorDebugThread **ppThread)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_EnumerateObjects(ICorDebugProcess *iface,
                ICorDebugObjectEnum **ppObjects)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_IsTransitionStub(ICorDebugProcess *iface,
                CORDB_ADDRESS address, BOOL *pbTransitionStub)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_IsOSSuspended(ICorDebugProcess *iface,
                DWORD threadID, BOOL *pbSuspended)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_GetThreadContext(ICorDebugProcess *iface,
                DWORD threadID, ULONG32 contextSize, BYTE context[])
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_SetThreadContext(ICorDebugProcess *iface,
                DWORD threadID, ULONG32 contextSize, BYTE context[])
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_ReadMemory(ICorDebugProcess *iface,
                CORDB_ADDRESS address, DWORD size, BYTE buffer[],
                SIZE_T *read)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_WriteMemory(ICorDebugProcess *iface,
                CORDB_ADDRESS address, DWORD size, BYTE buffer[],
                SIZE_T *written)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_ClearCurrentException(ICorDebugProcess *iface,
                DWORD threadID)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_EnableLogMessages(ICorDebugProcess *iface,
                BOOL fOnOff)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_ModifyLogSwitch(ICorDebugProcess *iface,
                WCHAR *pLogSwitchName, LONG lLevel)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_EnumerateAppDomains(ICorDebugProcess *iface,
                ICorDebugAppDomainEnum **ppAppDomains)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_GetObject(ICorDebugProcess *iface,
                ICorDebugValue **ppObject)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_ThreadForFiberCookie(ICorDebugProcess *iface,
                DWORD fiberCookie, ICorDebugThread **ppThread)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI cordebugprocess_GetHelperThreadID(ICorDebugProcess *iface,
                DWORD *pThreadID)
{
    DebugProcess *This = impl_from_ICorDebugProcess(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}


/***************************************/
static const ICorDebugProcessVtbl cordebugprocessVtbl = {
    cordebugprocess_QueryInterface,
    cordebugprocess_AddRef,
    cordebugprocess_Release,
    cordebugprocess_Stop,
    cordebugprocess_Continue,
    cordebugprocess_IsRunning,
    cordebugprocess_HasQueuedCallbacks,
    cordebugprocess_EnumerateThreads,
    cordebugprocess_SetAllThreadsDebugState,
    cordebugprocess_Detach,
    cordebugprocess_Terminate,
    cordebugprocess_CanCommitChanges,
    cordebugprocess_CommitChanges,
    cordebugprocess_GetID,
    cordebugprocess_GetHandle,
    cordebugprocess_GetThread,
    cordebugprocess_EnumerateObjects,
    cordebugprocess_IsTransitionStub,
    cordebugprocess_IsOSSuspended,
    cordebugprocess_GetThreadContext,
    cordebugprocess_SetThreadContext,
    cordebugprocess_ReadMemory,
    cordebugprocess_WriteMemory,
    cordebugprocess_ClearCurrentException,
    cordebugprocess_EnableLogMessages,
    cordebugprocess_ModifyLogSwitch,
    cordebugprocess_EnumerateAppDomains,
    cordebugprocess_GetObject,
    cordebugprocess_ThreadForFiberCookie,
    cordebugprocess_GetHelperThreadID
};


static HRESULT CorDebugProcess_Create(CorDebug *cordebug, IUnknown** ppUnk, LPPROCESS_INFORMATION lpProcessInformation)
{
    DebugProcess *This;

    This = malloc(sizeof *This);
    if ( !This )
        return E_OUTOFMEMORY;

    if(!DuplicateHandle(GetCurrentProcess(), lpProcessInformation->hProcess,
                    GetCurrentProcess(), &This->handle, 0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        ERR("Failed to duplicate process handle\n");
        free(This);
        return E_FAIL;
    }
    if(!DuplicateHandle(GetCurrentProcess(), lpProcessInformation->hThread,
                    GetCurrentProcess(), &This->thread, 0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        CloseHandle(This->handle);

        ERR("Failed to duplicate thread handle\n");
        free(This);
        return E_FAIL;
    }

    This->ICorDebugProcess_iface.lpVtbl = &cordebugprocessVtbl;
    This->ref = 1;
    This->cordebug = cordebug;
    This->dwProcessID = lpProcessInformation->dwProcessId;

    if(This->cordebug)
        ICorDebug_AddRef(&This->cordebug->ICorDebug_iface);

    *ppUnk = (IUnknown*)&This->ICorDebugProcess_iface;

    return S_OK;
}

/* ICorDebugProcessEnum Interface */
static HRESULT WINAPI process_enum_QueryInterface(ICorDebugProcessEnum *iface, REFIID riid, void **ppvObject)
{
    CorDebug *This = impl_from_ICorDebugProcessEnum(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_ICorDebugProcessEnum ) ||
         IsEqualGUID( riid, &IID_ICorDebugEnum ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &This->ICorDebugProcessEnum_iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ICorDebugProcessEnum_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI process_enum_AddRef(ICorDebugProcessEnum *iface)
{
    CorDebug *This = impl_from_ICorDebugProcessEnum(iface);
    TRACE("%p ref=%lu\n", This, This->ref);

    return ICorDebug_AddRef(&This->ICorDebug_iface);
}

static ULONG WINAPI process_enum_Release(ICorDebugProcessEnum *iface)
{
    CorDebug *This = impl_from_ICorDebugProcessEnum(iface);
    TRACE("%p ref=%lu\n", This, This->ref);

    return ICorDebug_Release(&This->ICorDebug_iface);
}

static HRESULT WINAPI process_enum_Skip(ICorDebugProcessEnum *iface, ULONG celt)
{
    CorDebug *This = impl_from_ICorDebugProcessEnum(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI process_enum_Reset(ICorDebugProcessEnum *iface)
{
    CorDebug *This = impl_from_ICorDebugProcessEnum(iface);
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI process_enum_Clone(ICorDebugProcessEnum *iface, ICorDebugEnum **ppEnum)
{
    CorDebug *This = impl_from_ICorDebugProcessEnum(iface);
    FIXME("stub %p %p\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI process_enum_GetCount(ICorDebugProcessEnum *iface, ULONG *pcelt)
{
    CorDebug *This = impl_from_ICorDebugProcessEnum(iface);
    TRACE("stub %p %p\n", This, pcelt);

    if(!pcelt)
        return E_INVALIDARG;

    *pcelt = list_count(&This->processes);

    return S_OK;
}

static HRESULT WINAPI process_enum_Next(ICorDebugProcessEnum *iface, ULONG celt,
            ICorDebugProcess * processes[], ULONG *pceltFetched)
{
    CorDebug *This = impl_from_ICorDebugProcessEnum(iface);
    FIXME("stub %p %ld %p %p\n", This, celt, processes, pceltFetched);
    return E_NOTIMPL;
}

static const struct ICorDebugProcessEnumVtbl processenum_vtbl =
{
    process_enum_QueryInterface,
    process_enum_AddRef,
    process_enum_Release,
    process_enum_Skip,
    process_enum_Reset,
    process_enum_Clone,
    process_enum_GetCount,
    process_enum_Next
};

/*** IUnknown methods ***/
static HRESULT WINAPI CorDebug_QueryInterface(ICorDebug *iface, REFIID riid, void **ppvObject)
{
    CorDebug *This = impl_from_ICorDebug( iface );

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_ICorDebug ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &This->ICorDebug_iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ICorDebug_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI CorDebug_AddRef(ICorDebug *iface)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p ref=%lu\n", This, ref);

    return ref;
}

static ULONG WINAPI CorDebug_Release(ICorDebug *iface)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p ref=%lu\n", This, ref);

    if (ref == 0)
    {
        if(!list_empty(&This->processes))
            ERR("Processes haven't been removed Correctly\n");

        if(This->runtimehost)
            ICLRRuntimeHost_Release(This->runtimehost);

        if(This->pCallback2)
            ICorDebugManagedCallback2_Release(This->pCallback2);

        if(This->pCallback)
            ICorDebugManagedCallback_Release(This->pCallback);

        free(This);
    }

    return ref;
}

/*** ICorDebug methods ***/
static HRESULT WINAPI CorDebug_Initialize(ICorDebug *iface)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    FIXME("stub %p\n", This);
    return S_OK;
}

static HRESULT WINAPI CorDebug_Terminate(ICorDebug *iface)
{
    struct CorProcess *cursor, *cursor2;
    CorDebug *This = impl_from_ICorDebug( iface );
    TRACE("stub %p\n", This);

    LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, &This->processes, struct CorProcess, entry)
    {
        if(cursor->pProcess)
        {
            ICorDebugProcess_Terminate(cursor->pProcess, 0);
            ICorDebugProcess_Release(cursor->pProcess);
        }

        list_remove(&cursor->entry);
        free(cursor);
    }

    return S_OK;
}

static HRESULT WINAPI CorDebug_SetManagedHandler(ICorDebug *iface, ICorDebugManagedCallback *pCallback)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    HRESULT hr;
    ICorDebugManagedCallback2 *pCallback2;

    TRACE("%p (%p)\n", This, pCallback);

    if(!pCallback)
        return E_INVALIDARG;

    hr = ICorDebugManagedCallback_QueryInterface(pCallback, &IID_ICorDebugManagedCallback2, (void**)&pCallback2);
    if(hr == S_OK)
    {
        if(This->pCallback2)
            ICorDebugManagedCallback2_Release(This->pCallback2);

        if(This->pCallback)
            ICorDebugManagedCallback_Release(This->pCallback);

        This->pCallback = pCallback;
        This->pCallback2 = pCallback2;

        ICorDebugManagedCallback_AddRef(This->pCallback);
    }
    else
    {
        WARN("Debugging without interface ICorDebugManagedCallback2 is currently not supported.\n");
    }

    return hr;
}

static HRESULT WINAPI CorDebug_SetUnmanagedHandler(ICorDebug *iface, ICorDebugUnmanagedCallback *pCallback)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %p\n", This, pCallback);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_CreateProcess(ICorDebug *iface, LPCWSTR lpApplicationName,
            LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
            LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
            DWORD dwCreationFlags, PVOID lpEnvironment,LPCWSTR lpCurrentDirectory,
            LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
            CorDebugCreateProcessFlags debuggingFlags, ICorDebugProcess **ppProcess)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    ICorDebugProcess *pDebugProcess;
    HRESULT hr;

    TRACE("stub %p %s %s %p %p %d %ld %p %s %p %p %d %p\n", This, debugstr_w(lpApplicationName),
            debugstr_w(lpCommandLine), lpProcessAttributes, lpThreadAttributes,
            bInheritHandles, dwCreationFlags, lpEnvironment, debugstr_w(lpCurrentDirectory),
            lpStartupInfo, lpProcessInformation, debuggingFlags, ppProcess);

    if(CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes,
            bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory,
            lpStartupInfo, lpProcessInformation))
    {
        hr = CorDebugProcess_Create(This, (IUnknown**)&pDebugProcess, lpProcessInformation);
        if(hr == S_OK)
        {
            struct CorProcess *new_process = malloc(sizeof(CorProcess));

            new_process->pProcess = pDebugProcess;
            list_add_tail(&This->processes, &new_process->entry);

            ICorDebugProcess_AddRef(pDebugProcess);
            *ppProcess = pDebugProcess;

            if(This->pCallback)
                ICorDebugManagedCallback_CreateProcess(This->pCallback, pDebugProcess);
        }
        else
        {
            TerminateProcess(lpProcessInformation->hProcess, 0);
        }
    }
    else
        hr = E_FAIL;

    return hr;
}

static HRESULT WINAPI CorDebug_DebugActiveProcess(ICorDebug *iface, DWORD id, BOOL win32Attach,
            ICorDebugProcess **ppProcess)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %ld %d %p\n", This, id, win32Attach, ppProcess);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_EnumerateProcesses( ICorDebug *iface, ICorDebugProcessEnum **ppProcess)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    TRACE("stub %p %p\n", This, ppProcess);

    if(!ppProcess)
        return E_INVALIDARG;

    *ppProcess = &This->ICorDebugProcessEnum_iface;
    ICorDebugProcessEnum_AddRef(*ppProcess);

    return S_OK;
}

static HRESULT WINAPI CorDebug_GetProcess(ICorDebug *iface, DWORD dwProcessId, ICorDebugProcess **ppProcess)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %ld %p\n", This, dwProcessId, ppProcess);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_CanLaunchOrAttach(ICorDebug *iface, DWORD dwProcessId,
            BOOL win32DebuggingEnabled)
{
    CorDebug *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %ld %d\n", This, dwProcessId, win32DebuggingEnabled);
    return S_OK;
}

static const struct ICorDebugVtbl cordebug_vtbl =
{
    CorDebug_QueryInterface,
    CorDebug_AddRef,
    CorDebug_Release,
    CorDebug_Initialize,
    CorDebug_Terminate,
    CorDebug_SetManagedHandler,
    CorDebug_SetUnmanagedHandler,
    CorDebug_CreateProcess,
    CorDebug_DebugActiveProcess,
    CorDebug_EnumerateProcesses,
    CorDebug_GetProcess,
    CorDebug_CanLaunchOrAttach
};

HRESULT CorDebug_Create(ICLRRuntimeHost *runtimehost, IUnknown** ppUnk)
{
    CorDebug *This;

    This = malloc(sizeof *This);
    if ( !This )
        return E_OUTOFMEMORY;

    This->ICorDebug_iface.lpVtbl = &cordebug_vtbl;
    This->ICorDebugProcessEnum_iface.lpVtbl = &processenum_vtbl;
    This->ref = 1;
    This->pCallback = NULL;
    This->pCallback2 = NULL;
    This->runtimehost = runtimehost;

    list_init(&This->processes);

    if(This->runtimehost)
        ICLRRuntimeHost_AddRef(This->runtimehost);

    *ppUnk = (IUnknown*)&This->ICorDebug_iface;

    return S_OK;
}
