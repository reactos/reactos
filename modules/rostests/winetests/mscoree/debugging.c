/*
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
#include <stdio.h>

#include "windows.h"
#include "ole2.h"
#include "corerror.h"
#include "mscoree.h"
#include "corhdr.h"

#include "wine/test.h"

#include "initguid.h"
#include "cordebug.h"

static HMODULE hmscoree;

static HRESULT (WINAPI *pCreateDebuggingInterfaceFromVersion)(int, LPCWSTR, IUnknown **);

const WCHAR v2_0[] = {'v','2','.','0','.','5','0','7','2','7',0};

static HRESULT WINAPI ManagedCallback2_QueryInterface(ICorDebugManagedCallback2 *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_ICorDebugManagedCallback2, riid))
    {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ManagedCallback2_AddRef(ICorDebugManagedCallback2 *iface)
{
    return 2;
}

static ULONG WINAPI ManagedCallback2_Release(ICorDebugManagedCallback2 *iface)
{
    return 1;
}

static HRESULT WINAPI ManagedCallback2_FunctionRemapOpportunity(ICorDebugManagedCallback2 *iface,
                ICorDebugAppDomain *pAppDomain, ICorDebugThread *pThread,
                ICorDebugFunction *pOldFunction, ICorDebugFunction *pNewFunction,
                ULONG32 oldILOffset)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback2_CreateConnection(ICorDebugManagedCallback2 *iface,
                ICorDebugProcess *pProcess, CONNID dwConnectionId, WCHAR *pConnName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback2_ChangeConnection(ICorDebugManagedCallback2 *iface,
                ICorDebugProcess *pProcess, CONNID dwConnectionId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback2_DestroyConnection(ICorDebugManagedCallback2 *iface,
                ICorDebugProcess *pProcess, CONNID dwConnectionId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback2_Exception(ICorDebugManagedCallback2 *iface,
                ICorDebugAppDomain *pAppDomain, ICorDebugThread *pThread,
                ICorDebugFrame *pFrame, ULONG32 nOffset,
                CorDebugExceptionCallbackType dwEventType, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback2_ExceptionUnwind(ICorDebugManagedCallback2 *iface,
                ICorDebugAppDomain *pAppDomain, ICorDebugThread *pThread,
                CorDebugExceptionUnwindCallbackType dwEventType, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback2_FunctionRemapComplete(ICorDebugManagedCallback2 *iface,
                ICorDebugAppDomain *pAppDomain, ICorDebugThread *pThread,
                ICorDebugFunction *pFunction)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback2_MDANotification(ICorDebugManagedCallback2 *iface,
                ICorDebugController *pController, ICorDebugThread *pThread,
                ICorDebugMDA *pMDA)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static struct ICorDebugManagedCallback2Vtbl managedCallback2Vtbl = {
    ManagedCallback2_QueryInterface,
    ManagedCallback2_AddRef,
    ManagedCallback2_Release,
    ManagedCallback2_FunctionRemapOpportunity,
    ManagedCallback2_CreateConnection,
    ManagedCallback2_ChangeConnection,
    ManagedCallback2_DestroyConnection,
    ManagedCallback2_Exception,
    ManagedCallback2_ExceptionUnwind,
    ManagedCallback2_FunctionRemapComplete,
    ManagedCallback2_MDANotification
};

static ICorDebugManagedCallback2 ManagedCallback2 = { &managedCallback2Vtbl };

static HRESULT WINAPI ManagedCallback_QueryInterface(ICorDebugManagedCallback *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_ICorDebugManagedCallback, riid))
    {
        *ppv = iface;
        return S_OK;
    }
    else if(IsEqualGUID(&IID_ICorDebugManagedCallback2, riid))
    {
        *ppv = (void**)&ManagedCallback2;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ManagedCallback_AddRef(ICorDebugManagedCallback *iface)
{
    return 2;
}

static ULONG WINAPI ManagedCallback_Release(ICorDebugManagedCallback *iface)
{
    return 1;
}

static HRESULT WINAPI ManagedCallback_Breakpoint(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugThread *pThread, ICorDebugBreakpoint *pBreakpoint)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_StepComplete(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugThread *pThread, ICorDebugStepper *pStepper, CorDebugStepReason reason)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_Break(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugThread *thread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_Exception(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugThread *pThread, BOOL unhandled)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_EvalComplete(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugThread *pThread, ICorDebugEval *pEval)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_EvalException(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugThread *pThread, ICorDebugEval *pEval)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_CreateProcess(ICorDebugManagedCallback *iface, ICorDebugProcess *pProcess)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_ExitProcess(ICorDebugManagedCallback *iface, ICorDebugProcess *pProcess)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_CreateThread(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugThread *thread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_ExitThread(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugThread *thread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_LoadModule(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugModule *pModule)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_UnloadModule(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugModule *pModule)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_LoadClass(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugClass *c)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_UnloadClass(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugClass *c)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_DebuggerError(ICorDebugManagedCallback *iface, ICorDebugProcess *pProcess,
                    HRESULT errorHR, DWORD errorCode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_LogMessage(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugThread *pThread, LONG lLevel, WCHAR *pLogSwitchName, WCHAR *pMessage)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_LogSwitch(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugThread *pThread, LONG lLevel, ULONG ulReason,
                    WCHAR *pLogSwitchName, WCHAR *pParentName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_CreateAppDomain(ICorDebugManagedCallback *iface, ICorDebugProcess *pProcess,
                    ICorDebugAppDomain *pAppDomain)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_ExitAppDomain(ICorDebugManagedCallback *iface, ICorDebugProcess *pProcess,
                    ICorDebugAppDomain *pAppDomain)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_LoadAssembly(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugAssembly *pAssembly)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_UnloadAssembly(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugAssembly *pAssembly)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_ControlCTrap(ICorDebugManagedCallback *iface, ICorDebugProcess *pProcess)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_NameChange(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugThread *pThread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_UpdateModuleSymbols(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugModule *pModule, IStream *pSymbolStream)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_EditAndContinueRemap(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugThread *pThread, ICorDebugFunction *pFunction, BOOL fAccurate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ManagedCallback_BreakpointSetError(ICorDebugManagedCallback *iface, ICorDebugAppDomain *pAppDomain,
                    ICorDebugThread *pThread, ICorDebugBreakpoint *pBreakpoint, DWORD dwError)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ICorDebugManagedCallbackVtbl managedCallbackVtbl = {
    ManagedCallback_QueryInterface,
    ManagedCallback_AddRef,
    ManagedCallback_Release,
    ManagedCallback_Breakpoint,
    ManagedCallback_StepComplete,
    ManagedCallback_Break,
    ManagedCallback_Exception,
    ManagedCallback_EvalComplete,
    ManagedCallback_EvalException,
    ManagedCallback_CreateProcess,
    ManagedCallback_ExitProcess,
    ManagedCallback_CreateThread,
    ManagedCallback_ExitThread,
    ManagedCallback_LoadModule,
    ManagedCallback_UnloadModule,
    ManagedCallback_LoadClass,
    ManagedCallback_UnloadClass,
    ManagedCallback_DebuggerError,
    ManagedCallback_LogMessage,
    ManagedCallback_LogSwitch,
    ManagedCallback_CreateAppDomain,
    ManagedCallback_ExitAppDomain,
    ManagedCallback_LoadAssembly,
    ManagedCallback_UnloadAssembly,
    ManagedCallback_ControlCTrap,
    ManagedCallback_NameChange,
    ManagedCallback_UpdateModuleSymbols,
    ManagedCallback_EditAndContinueRemap,
    ManagedCallback_BreakpointSetError
};

static ICorDebugManagedCallback ManagedCallback = { &managedCallbackVtbl };

static BOOL init_functionpointers(void)
{
    hmscoree = LoadLibraryA("mscoree.dll");

    if (!hmscoree)
    {
        win_skip("mscoree.dll not available\n");
        return FALSE;
    }

    pCreateDebuggingInterfaceFromVersion = (void *)GetProcAddress(hmscoree, "CreateDebuggingInterfaceFromVersion");

    if (!pCreateDebuggingInterfaceFromVersion)
    {
        win_skip("functions not available\n");
        FreeLibrary(hmscoree);
        return FALSE;
    }

    return TRUE;
}

#define check_process_enum(core, e) _check_process_enum(__LINE__, core, e)
static void _check_process_enum(unsigned line, ICorDebug *pCorDebug, ULONG nExpected)
{
    HRESULT hr;
    ICorDebugProcessEnum *pProcessEnum = NULL;

    hr = ICorDebug_EnumerateProcesses(pCorDebug, NULL);
    ok_(__FILE__,line) (hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr);

    hr = ICorDebug_EnumerateProcesses(pCorDebug, &pProcessEnum);
    ok_(__FILE__,line) (hr == S_OK, "expected S_OK got %08lx\n", hr);
    if(hr == S_OK)
    {
        ULONG cnt;

        hr = ICorDebugProcessEnum_GetCount(pProcessEnum, &cnt);
        ok_(__FILE__,line) (hr == S_OK, "expected S_OK got %08lx\n", hr);
        ok_(__FILE__,line) (cnt == nExpected, "expected %ld got %ld\n", nExpected, cnt);

        ICorDebugProcessEnum_Release(pProcessEnum);
    }
}

static void test_createDebugger(void)
{
    HRESULT hr;
    IUnknown *pUnk;
    ICorDebug *pCorDebug;

    hr = pCreateDebuggingInterfaceFromVersion(0, v2_0, &pUnk);
    ok(hr == E_INVALIDARG, "CreateDebuggingInterfaceFromVersion returned %08lx\n", hr);

    hr = pCreateDebuggingInterfaceFromVersion(1, v2_0, &pUnk);
    ok(hr == E_INVALIDARG, "CreateDebuggingInterfaceFromVersion returned %08lx\n", hr);

    hr = pCreateDebuggingInterfaceFromVersion(2, v2_0, &pUnk);
    ok(hr == E_INVALIDARG, "CreateDebuggingInterfaceFromVersion returned %08lx\n", hr);

    hr = pCreateDebuggingInterfaceFromVersion(4, v2_0, &pUnk);
    ok(hr == E_INVALIDARG, "CreateDebuggingInterfaceFromVersion returned %08lx\n", hr);

    hr = pCreateDebuggingInterfaceFromVersion(3, v2_0, NULL);
    ok(hr == E_INVALIDARG, "CreateDebuggingInterfaceFromVersion returned %08lx\n", hr);

    hr = pCreateDebuggingInterfaceFromVersion(3, v2_0, &pUnk);
    if(hr == S_OK)
    {
        hr = IUnknown_QueryInterface(pUnk, &IID_ICorDebug, (void**)&pCorDebug);
        ok(hr == S_OK, "expected S_OK got %08lx\n", hr);
        if(hr == S_OK)
        {
            hr = ICorDebug_Initialize(pCorDebug);
            ok(hr == S_OK, "expected S_OK got %08lx\n", hr);
            if(hr == S_OK)
            {
                hr = ICorDebug_SetManagedHandler(pCorDebug, NULL);
                ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr);

                hr = ICorDebug_SetManagedHandler(pCorDebug, &ManagedCallback);
                ok(hr == S_OK, "expected S_OK got %08lx\n", hr);

                /* We should have no processes */
                check_process_enum(pCorDebug, 0);
            }

            ICorDebug_Release(pCorDebug);
        }
        IUnknown_Release(pUnk);
    }
    else
    {
        skip(".NET 2.0 or mono not installed.\n");
    }
}

START_TEST(debugging)
{
    CoInitialize(NULL);

    if (!init_functionpointers())
        return;

    test_createDebugger();

    FreeLibrary(hmscoree);
    CoUninitialize();
}
