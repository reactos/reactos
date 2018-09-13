//+---------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//   File:      mshtmlsrv.cxx
//
//  Contents:   MSHTML Server Side Application Object and DLL entry points
//
//  Classes:    CServerApp
//
//  History:   03-Sep-98   tomfakes  Created
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_MSHTMSRV_HXX_
#define X_MSHTMSRV_HXX_
#include "mshtmsrv.hxx"
#endif


DeclareTag(tagIISCalls, "TridentOnServer", "Raw IIS Calls");

//
// Misc stuff to keep the linker happy
//
DWORD               g_dwFALSE = 0;          // Used for assert to fool the compiler
EXTERN_C HANDLE     g_hProcessHeap = NULL;  // g_hProcessHeap is set by the CRT in dllcrt0.c
HINSTANCE           g_hInstance = NULL;

CServerApp        * g_pApp = NULL;

// _purecall() is here to avoid linking unwanted CRT code such as assert etc.
extern "C" {
int __cdecl  _purecall(void)
{
    return(FALSE);
}
};



//+----------------------------------------------------------------------------
//
// Function: DllMain
//
//+----------------------------------------------------------------------------
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{

    return TRUE;    
}

//+----------------------------------------------------------------------------
//
// IIS Filter Entry Points
//
//+----------------------------------------------------------------------------


//+----------------------------------------------------------------------------
//
// Function: GetFilterVersion
//
//+----------------------------------------------------------------------------
BOOL WINAPI GetFilterVersion(HTTP_FILTER_VERSION *pVer)
{
    HRESULT     hr;

    TraceTag((tagIISCalls, "GetFilterVersion, Server Version: %08lX", pVer->dwServerFilterVersion));

    hr = THR(CServerApp::AddRefApp());
    if (!hr)
        return g_pApp->GetIISFilter()->GetVersion(pVer);
    else
        return FALSE;
}


//+----------------------------------------------------------------------------
//
// Function: HttpFilterProc
//
//+----------------------------------------------------------------------------
DWORD WINAPI HttpFilterProc(HTTP_FILTER_CONTEXT *pfc, DWORD dwNotifType, LPVOID pvNotification)
{
    TraceTag((tagIISCalls, "HttpFilterProc"));

    Assert(g_pApp);
    return g_pApp->GetIISFilter()->FilterProc(pfc, dwNotifType, pvNotification);
}


//+----------------------------------------------------------------------------
//
// Function: TerminateFilter
//
//+----------------------------------------------------------------------------
BOOL WINAPI TerminateFilter(DWORD dwFlags)
{
    TraceTag((tagIISCalls, "TerminateFilter"));

    Assert(g_pApp);
    return g_pApp->GetIISFilter()->Terminate(dwFlags);
}


//+----------------------------------------------------------------------------
//
// IIS Extension Entry Points
//
//+----------------------------------------------------------------------------

/*---------------------------------------------------------------------*
GetExtensionVersion

*/
BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO *pextver) 
{
    HRESULT     hr;

    TraceTag((tagIISCalls, "GetExtensionVersion"));

    hr = THR(CServerApp::AddRefApp());
    if (!hr)
        return g_pApp->GetIISExtension()->GetVersion(pextver);
    else
        return FALSE;
}

/*---------------------------------------------------------------------*
TerminateExtension

*/

BOOL __stdcall TerminateExtension(DWORD flag) 
{
    TraceTag((tagIISCalls, "TerminateExtension"));
    Assert(g_pApp);
	return g_pApp->GetIISExtension()->Terminate(flag);
}

/*---------------------------------------------------------------------*
HttpExtensionProc

*/

DWORD __stdcall HttpExtensionProc(EXTENSION_CONTROL_BLOCK *pECB) 
{
    TraceTag((tagIISCalls, "HttpExtensionProc"));

    Assert(g_pApp);
	return g_pApp->GetIISExtension()->ExtensionProc(pECB);
}
