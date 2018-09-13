/*
 * @(#)oawrap.hxx 1.0 07/09/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * OLE Automation wrapper functions
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "oawrap.hxx"

static CRITICAL_SECTION g_csOAWrap;
static PVARFORMAT g_pfnVarFormat = 0;
static HMODULE g_hOleAut = NULL;
static BOOL g_fCritSecInitialized = FALSE;

#define ENTER_OAW_CRITICAL_SECTION EnterCriticalSection(&g_csOAWrap)
#define LEAVE_OAW_CRITICAL_SECTION LeaveCriticalSection(&g_csOAWrap)

LPVOID GetProcAddressFromOleAut(
    LPCSTR szFunction)
{
    LPVOID pRet;

    if (NULL == g_hOleAut)
    {
        // Initialize the global critical section if necessary
        if (!g_fCritSecInitialized)
        {
            InitializeCriticalSection(&g_csOAWrap);
            g_fCritSecInitialized = TRUE;
        }

        // Get a handle to OLEAUT32.DLL
        ENTER_OAW_CRITICAL_SECTION;
        g_hOleAut = GetModuleHandle(TEXT("OLEAUT32.DLL"));
        LEAVE_OAW_CRITICAL_SECTION;
    }

    if (g_hOleAut)
    {
        ENTER_OAW_CRITICAL_SECTION;
        pRet = GetProcAddress(g_hOleAut, szFunction);
        LEAVE_OAW_CRITICAL_SECTION;

        Assert(pRet && "You're probably using an old version of OLEAUT32.DLL.  Couldn't get entry point address");
    }
    else
    {
        Assert(FALSE && "Could not get handle to OLEAUT32.DLL - may need to use LoadLibrary to call it");
        pRet = NULL;    
    }

    return pRet;
}


HRESULT 
WrapVarFormat(
    LPVARIANT pvarIn,  
    LPOLESTR pstrFormat, 
    int iFirstDay, 
    int iFirstWeek, 
    ULONG dwFlags, 
    BSTR *pbstrOut)
{
    HRESULT hr;
    
    if (NULL == g_pfnVarFormat)
    {
        // We need to get a pointer to OLEAUT32.DLL

        // Yes, the string must be ANSI
        g_pfnVarFormat = (PVARFORMAT) GetProcAddressFromOleAut("VarFormat");

        if (NULL == g_pfnVarFormat)
            return E_FAIL;
    }

    Assert(g_pfnVarFormat && "NULL function pointer - shouldn't ever happen");

    return g_pfnVarFormat(pvarIn,  pstrFormat, iFirstDay, iFirstWeek, dwFlags, pbstrOut);
}
