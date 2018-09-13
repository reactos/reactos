/*----------------------------------------------------------------------------
/ Title;
/   strings.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   Useful string manipulation functions.
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
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

    MDTraceEnter(TRACE_COMMON_STR, "LocalAllocString");

    MDTraceAssert(ppResult);
    MDTraceAssert(pString);

    if ( !ppResult || !pString )
        ExitGracefully(hr, E_INVALIDARG, "Bad arguments");

    *ppResult = (LPTSTR)LocalAlloc(LPTR, StringByteSize(pString) );

    if ( !*ppResult )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate buffer");

    lstrcpy(*ppResult, pString);
    hr = S_OK;                          //  success

exit_gracefully:

    MDTraceLeaveResult(hr);
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

    MDTraceEnter(TRACE_COMMON_STR, "LocalAllocStringLen");

    MDTraceAssert(ppResult);

    if ( !ppResult || cLen == 0 )
        ExitGracefully(hr, E_INVALIDARG, "Bad arguments (length or buffer)");

    *ppResult = (LPTSTR)LocalAlloc(LPTR, (cLen+1) * SIZEOF(TCHAR));

    hr = *ppResult ? S_OK:E_OUTOFMEMORY;

exit_gracefully:

    MDTraceLeaveResult(hr);
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
    MDTraceEnter(TRACE_COMMON_STR, "LocalFreeString");
    MDTraceAssert(ppString);

    if ( ppString )
    {
        if ( *ppString )
            LocalFree((HLOCAL)*ppString);

        *ppString = NULL;
    }

    MDTraceLeave();
}


/*-----------------------------------------------------------------------------
/ LocalQueryString
/ ------------------
/   Hit the registry returning the wide version of the given string,
/   we dynamically allocate the buffer to put the result into,
/   this should be free'd by calling LocalFreeString.
/
/ In:
/   hkey = key to query from
/   pSubKey -> pointer to sub key identifier
/   ppString -> receives the string point
/
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
HRESULT LocalQueryString(LPTSTR* ppResult, HKEY hk, LPCTSTR lpSubKey)
{
    HRESULT hr = S_OK;
    DWORD dwSize;

    MDTraceEnter(TRACE_COMMON_STR, "LocalQueryString");

    MDTraceAssert(hk);
    MDTraceAssert(ppResult);

    *ppResult = NULL;

    // get the size of the buffer required, then add on a character to cope
    // with the terminator.

    if ( ERROR_SUCCESS != RegQueryValueEx(hk, lpSubKey, NULL, NULL, NULL, &dwSize) )
        ExitGracefully(hr, E_INVALIDARG, "Failed to query for key size");

    dwSize += SIZEOF(TCHAR);

    // Allocate the buffer, and read the string into it.

    *ppResult = (LPTSTR)LocalAlloc(LPTR, dwSize);

    if ( !*ppResult )
        ExitGracefully(hr, E_INVALIDARG, "Failed to allocate the buffer");

    if ( ERROR_SUCCESS != RegQueryValueEx(hk, lpSubKey, NULL, NULL, (LPBYTE)*ppResult, &dwSize) )
        ExitGracefully(hr, E_FAIL, "Failed to fill the buffer");

    hr = S_OK;                  // success

exit_gracefully:

    if ( FAILED(hr) )
        LocalFreeString(ppResult);

    MDTraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ StrRetFromString
/ -----------------
/   Package a WIDE string into a LPSTRRET structure.
/
/ In:
/   pStrRet -> receieves the newly allocate string
/   pString -> string to be copied.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
HRESULT StrRetFromString(LPSTRRET lpStrRet, LPCTSTR pString)
{
    HRESULT hr;

    MDTraceEnter(TRACE_COMMON_STR, "StrRetFromString");
    MDTrace(TEXT("pStrRet %08x, lpszString -%s-"), lpStrRet, pString);

    MDTraceAssert(lpStrRet);
    MDTraceAssert(pString);

    // This assumes we always build UNICODE, fix if not

#ifdef UNICODE

    lpStrRet->pOleStr = (LPTSTR)SHAlloc(StringByteSize(pString));

    if ( !lpStrRet->pOleStr )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate buffer for string");

    lpStrRet->uType = STRRET_OLESTR;
    lstrcpy(lpStrRet->pOleStr, pString);
#else
    lpStrRet->uType = STRRET_CSTR;
    lstrcpy( lpStrRet->cStr, pString );
    goto exit_gracefully;
#endif

    hr = S_OK;                              // success

exit_gracefully:

    MDTraceLeaveResult(hr);
}
