//================================================================================
//		File:	Utils .CPP
//		Date: 	6/26/98
//		Desc:	contains implementation of utility functions that can be useful
//
//================================================================================
#include "stdafx.h"

#ifndef _X_UTILS_H_
#define _X_UTILS_H_
#include "utils.h"
#endif


extern HINSTANCE   g_hInstance;


//+------------------------------------------------------------------------
//
//  Function:   GetLastWin32Error
//
//  Synopsis:   Returns the last Win32 error, converted to an HRESULT.
//
//-------------------------------------------------------------------------

HRESULT
GetLastWin32Error( )
{
#ifdef WIN16
    return E_FAIL;
#else
    // Win 95 can return 0, even when there's an error.
    DWORD dw = GetLastError();
    return dw ? HRESULT_FROM_WIN32(dw) : E_FAIL;
#endif
}

//+-----------------------------------------------------------
//
//  method : GetStringResourceValue
//
//  Synopsis : helper function to retrieve a bstr from the resource
//      string table 
//
//-------------------------------------------------------------
HRESULT
GetResourceStringValue(UINT idr, BSTR * pbstr )
{
    TCHAR chBuffer[50];
    int   cch = 50;

    assert(pbstr);
    *pbstr = NULL;

    if (!LoadString(g_hInstance, idr, (LPTSTR)&chBuffer, cch))
        return (GetLastWin32Error());

#ifndef UNICODE
    WCHAR awch[50];

    if (MultiByteToWideChar(CP_ACP, 0, 
                            (LPCSTR)&chBuffer, 
                            -1, 
                            (LPWSTR)&awch, 
                            ARRAY_SIZE(awch)))
    {
         *pbstr = SysAllocString(awch);
    }

#else
	*pbstr = SysAllocString( chBuffer );
#endif

    return (!*pbstr) ? E_OUTOFMEMORY : S_OK;
}