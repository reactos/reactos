//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       strings.cpp
//
//  Useful string manipulation functions.
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ LocalAllocString
/ ------------------
/   Allocate a string, and initialize it with the specified contents.
/
/ In:
/   ppResult -> recieves pointer to the new string
/   pString -> string to initialize with
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT LocalAllocString(LPTSTR* ppResult, LPCTSTR pString)
{
    HRESULT hr;

    TraceEnter(TRACE_COMMON_STR, "LocalAllocString");

    TraceAssert(ppResult);
    TraceAssert(pString);

    if ( !ppResult || !pString )
        ExitGracefully(hr, E_INVALIDARG, "Bad arguments");

    *ppResult = (LPTSTR)LocalAlloc(LPTR, StringByteSize(pString) );

    if ( !*ppResult )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate buffer");

    lstrcpy(*ppResult, pString);
    hr = S_OK;                          //  success

exit_gracefully:

    TraceLeaveResult(hr);
}


/*----------------------------------------------------------------------------
/ LocalAllocStringLen
/ ---------------------
/   Given a length return a buffer of that size.
/
/ In:
/   ppResult -> receives the pointer to the string
/   cLen = length in characters to allocate
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT LocalAllocStringLen(LPTSTR* ppResult, UINT cLen)
{
    HRESULT hr;

    TraceEnter(TRACE_COMMON_STR, "LocalAllocStringLen");

    TraceAssert(ppResult);

    if ( !ppResult || cLen == 0 )
        ExitGracefully(hr, E_INVALIDARG, "Bad arguments (length or buffer)");

    *ppResult = (LPTSTR)LocalAlloc(LPTR, (cLen+1) * SIZEOF(TCHAR));

    hr = *ppResult ? S_OK:E_OUTOFMEMORY; 

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ LocalFreeString
/ -----------------
/   Release the string pointed to be *ppString (which can be null) and
/   then reset the pointer back to NULL.   
/
/ In:
/   ppString -> pointer to string pointer to be free'd
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void LocalFreeString(LPTSTR* ppString)
{
    if ( ppString )
    {
        if ( *ppString )
            LocalFree((HLOCAL)*ppString);

        *ppString = NULL;
    }
}


//*************************************************************
//
//  SizeofStringResource
//
//  Purpose:    Find the length (in chars) of a string resource
//
//  Parameters: HINSTANCE hInstance - module containing the string
//              UINT idStr - ID of string
//
//
//  Return:     UINT - # of chars in string, not including NULL
//
//  Notes:      Based on code from user32.
//
//*************************************************************
UINT
SizeofStringResource(HINSTANCE hInstance,
                     UINT idStr)
{
    UINT cch = 0;
    HRSRC hRes = FindResource(hInstance, (LPTSTR)((LONG)(((USHORT)idStr >> 4) + 1)), RT_STRING);
    if (NULL != hRes)
    {
        HGLOBAL hStringSeg = LoadResource(hInstance, hRes);
        if (NULL != hStringSeg)
        {
            LPWSTR psz = (LPWSTR)LockResource(hStringSeg);
            if (NULL != psz)
            {
                idStr &= 0x0F;
                while(true)
                {
                    cch = *psz++;
                    if (idStr-- == 0)
                        break;
                    psz += cch;
                }
            }
        }
    }
    return cch;
}


//*************************************************************
//
//  LoadStringAlloc
//
//  Purpose:    Loads a string resource into an alloc'd buffer
//
//  Parameters: ppszResult - string resource returned here
//              hInstance - module to load string from
//              idStr - string resource ID
//
//  Return:     same as LoadString
//
//  Notes:      On successful return, the caller must
//              LocalFree *ppszResult
//
//*************************************************************
int
LoadStringAlloc(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr)
{
    int nResult = 0;
    UINT cch = SizeofStringResource(hInstance, idStr);
    if (cch)
    {
        cch++; // for NULL
        *ppszResult = (LPTSTR)LocalAlloc(LPTR, cch * sizeof(TCHAR));
        if (*ppszResult)
            nResult = LoadString(hInstance, idStr, *ppszResult, cch);
    }
    return nResult;
}


//*************************************************************
//
//  String formatting functions
//
//*************************************************************

DWORD
FormatStringID(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr, ...)
{
    DWORD dwResult;
    va_list args;
    va_start(args, idStr);
    dwResult = vFormatStringID(ppszResult, hInstance, idStr, &args);
    va_end(args);
    return dwResult;
}

DWORD
FormatString(LPTSTR *ppszResult, LPCTSTR pszFormat, ...)
{
    DWORD dwResult;
    va_list args;
    va_start(args, pszFormat);
    dwResult = vFormatString(ppszResult, pszFormat, &args);
    va_end(args);
    return dwResult;
}

DWORD
vFormatStringID(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr, va_list *pargs)
{
    DWORD dwResult = 0;
    LPTSTR pszFormat = NULL;
    if (LoadStringAlloc(&pszFormat, hInstance, idStr))
    {
        dwResult = vFormatString(ppszResult, pszFormat, pargs);
        LocalFree(pszFormat);
    }
    return dwResult;
}

DWORD
vFormatString(LPTSTR *ppszResult, LPCTSTR pszFormat, va_list *pargs)
{
    return FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                         pszFormat,
                         0,
                         0,
                         (LPTSTR)ppszResult,
                         1,
                         pargs);
}


//*************************************************************
//
//  GetSystemErrorText
//
//  Purpose:    Retrieve error text for a win32 error value
//
//  Parameters: ppszResult - string resource returned here
//              dwErr - error ID
//
//  Return:     same as FormatMessage
//
//  Notes:      On successful return, the caller must
//              LocalFree *ppszResult
//
//*************************************************************
DWORD
GetSystemErrorText(LPTSTR *ppszResult, DWORD dwErr)
{
    return FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                         NULL,
                         dwErr,
                         0,
                         (LPTSTR)ppszResult,
                         0,
                         NULL);
}
