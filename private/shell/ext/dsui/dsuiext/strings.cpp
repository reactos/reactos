#include "pch.h"
#include <urlmon.h>
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Internal only string APIs
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ StringToDWORD
/ -------------
/   Scan the string converting it to a DWORD, cope with hex and decimal alike,
/   more than likely we will receive a hex number though.
/
/ In:
/   pString -> string to parse
/
/ Out:
/   DWORD
/----------------------------------------------------------------------------*/
DWORD StringToDWORD(LPWSTR pString)
{
    DWORD dwResult = 0x0;
    USES_CONVERSION;

    TraceEnter(TRACE_COMMONAPI, "StringToDWORD");
    Trace(TEXT("pString %s"), W2T(pString));

    // Is the leading sequence 0x?  If so then lets parse as hex, otherwise
    // we can pass to StrToInt.

    if ( pString[0] == L'0' && pString[1] == L'x' )
    {
        for ( pString += 2; *pString; pString++ )
        {
            WCHAR ch = *pString;
        
            if ( InRange(ch, L'0', L'9') )
            {
                dwResult = (dwResult << 4) | (ch - L'0');
            }
            else if ( InRange(ch | (L'a'-L'A'), L'a', L'f') )
            {
                dwResult = (dwResult << 4) | (ch - L'a' + 10);
            }
            else
            {
                break;          // tread non 0-9, A-F as end of string
            }
        }
    }
    else
    {
        dwResult = (DWORD)StrToIntW(pString);
    }

    Trace(TEXT("DWORD result is %08x"), dwResult);

    TraceLeaveValue(dwResult);
}


/*-----------------------------------------------------------------------------
/ StringToURL
/ -----------
/   Convert a string to URL format, mashing the characters as required.
/
/ In:
/   pString -> string to be converted
/   ppResult -> receives a pointer to the new string (free using LocalFreeString).
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT StringToURL(LPCTSTR pString, LPTSTR* ppResult)
{
    HRESULT hr;
    TCHAR szEncodedURL[INTERNET_MAX_URL_LENGTH];
    DWORD dwLen = ARRAYSIZE(szEncodedURL);
    int i;

    TraceEnter(TRACE_COMMONAPI, "StringToURL");
    TraceAssert(pString);
    TraceAssert(ppResult);

    *ppResult = NULL;               // incase of failure

    if ( !InternetCanonicalizeUrl(pString, szEncodedURL, &dwLen, 0) )
        ExitGracefully(hr, E_FAIL, "Failed to convert URL to encoded format");

    hr = LocalAllocString(ppResult, szEncodedURL);
    FailGracefully(hr, "Failed to allocate copy of URL");

    hr = S_OK;                      // success

exit_gracefully:

    if ( FAILED(hr) && *ppResult )
        LocalFreeString(ppResult);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ Exported APIs
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ StringDPA_InsertString
/ ----------------------
/   Make a copy of the given string and place it into the DPA.  It can then
/   be accessed using the StringDPA_GetString, or free'd using the 
/   StringDPA_Destroy/StringDPA_DeleteString.
/
/ In:
/   hdpa = DPA to put string into
/   i = index to insert at
/   pString -> string to be inserted
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

STDAPI StringDPA_InsertStringA(HDPA hdpa, INT i, LPCSTR pString)
{
    HRESULT hr;
    LPSTR pStringCopy = NULL;

    TraceEnter(TRACE_COMMONAPI, "StringDPA_InsertStringA");
    TraceAssert(hdpa);
    TraceAssert(pString);

    if ( hdpa && pString )
    {
        hr = LocalAllocStringA(&pStringCopy, pString);
        FailGracefully(hr, "Failed to allocate string copy");

        if ( -1 == DPA_InsertPtr(hdpa, i, pStringCopy) )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to add string to DPA");
    }

    hr = S_OK;

exit_gracefully:

    if ( FAILED(hr) )
        LocalFreeStringA(&pStringCopy);

    TraceLeaveResult(hr);
}

STDAPI StringDPA_InsertStringW(HDPA hdpa, INT i, LPCWSTR pString)
{
    HRESULT hr;
    LPWSTR pStringCopy = NULL;

    TraceEnter(TRACE_COMMONAPI, "StringDPA_InsertStringW");
    TraceAssert(hdpa);
    TraceAssert(pString);

    if ( hdpa && pString )
    {
        hr = LocalAllocStringW(&pStringCopy, pString);
        FailGracefully(hr, "Failed to allocate string copy");

        if ( -1 == DPA_InsertPtr(hdpa, i, pStringCopy) )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to add string to DPA");
    }

    hr = S_OK;

exit_gracefully:

    if ( FAILED(hr) )
        LocalFreeStringW(&pStringCopy);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ StringDPA_AppendString
/ ----------------------
/   Make a copy of the given string and place it into the DPA.  It can then
/   be accessed using the StringDPA_GetString, or free'd using the 
/   StringDPA_Destroy/StringDPA_DeleteString.
/
/ In:
/   hdpa = DPA to put string into
/   pString -> string to be append
/   pres = resulting index
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

STDAPI StringDPA_AppendStringA(HDPA hdpa, LPCSTR pString, PUINT_PTR pres)
{
    HRESULT hr;
    INT ires = 0;
    LPSTR pStringCopy = NULL;

    TraceEnter(TRACE_COMMONAPI, "StringDPA_AppendStringA");
    TraceAssert(hdpa);
    TraceAssert(pString);

    if ( hdpa && pString )
    {
        hr = LocalAllocStringA(&pStringCopy, pString);
        FailGracefully(hr, "Failed to allocate string copy");

        ires = DPA_AppendPtr(hdpa, pStringCopy);
        if ( -1 == ires )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to add string to DPA");

        if ( pres )
            *pres = ires;
    }

    hr = S_OK;

exit_gracefully:

    if ( FAILED(hr) )
        LocalFreeStringA(&pStringCopy);

    TraceLeaveResult(hr);
}

STDAPI StringDPA_AppendStringW(HDPA hdpa, LPCWSTR pString, PUINT_PTR pres)
{
    HRESULT hr;
    INT ires = 0;
    LPWSTR pStringCopy = NULL;

    TraceEnter(TRACE_COMMONAPI, "StringDPA_AppendStringW");
    TraceAssert(hdpa);
    TraceAssert(pString);

    if ( hdpa && pString )
    {
        hr = LocalAllocStringW(&pStringCopy, pString);
        FailGracefully(hr, "Failed to allocate string copy");

        ires = DPA_AppendPtr(hdpa, pStringCopy);
        if ( -1 == ires )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to add string to DPA");

        if ( pres )
            *pres = ires;
    }

    hr = S_OK;

exit_gracefully:

    if ( FAILED(hr) )
        LocalFreeStringW(&pStringCopy);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ StringDPA_DeleteString
/ ----------------------
/   Delete the specified index from the DPA, freeing the string element
/   that we have dangling from the index.
/
/ In:
/   hdpa -> handle to DPA to be destroyed
/   index = index of item to free
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
STDAPI_(VOID) StringDPA_DeleteString(HDPA hdpa, INT index)
{
    TraceEnter(TRACE_COMMONAPI, "StringDPA_DeleteString");

    if ( hdpa && (index < DPA_GetPtrCount(hdpa)) )
    {
// BUGBUG: assumes LocalAllocString uses LocalAlloc (fair enough I guess)            
        LocalFree((HLOCAL)DPA_FastGetPtr(hdpa, index));
        DPA_DeletePtr(hdpa, index);
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ StringDPA_Destroy
/ -----------------
/   Take the given string DPA and destory it.
/
/ In:
/   pHDPA -> handle to DPA to be destroyed
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

INT _DestroyStringDPA(LPVOID pItem, LPVOID pData)
{
// BUGBUG: assumes that LocalAllocString does just that, 
// BUGBUG: to store the string.
    LocalFree((HLOCAL)pItem);
    return 1;
}

STDAPI_(VOID) StringDPA_Destroy(HDPA* pHDPA)
{
    TraceEnter(TRACE_COMMONAPI, "StringDPA_Destroy");
    
    if ( pHDPA && *pHDPA )
    {
        DPA_DestroyCallback(*pHDPA, _DestroyStringDPA, NULL);
        *pHDPA = NULL;
    }

    TraceLeave();
}


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

STDAPI LocalAllocStringA(LPSTR* ppResult, LPCSTR pString)
{
    HRESULT hr;

    TraceEnter(TRACE_COMMONAPI, "LocalAllocStringA");

    *ppResult = NULL;

    if ( pString )
    {
        *ppResult = (LPSTR)LocalAlloc(LPTR, StringByteSizeA(pString));

        if ( !*ppResult )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to get string buffer");

        StrCpyA(*ppResult, pString);
    }

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}

STDAPI LocalAllocStringW(LPWSTR* ppResult, LPCWSTR pString)
{
    HRESULT hr;

    TraceEnter(TRACE_COMMONAPI, "LocalAllocStringW");

    *ppResult = NULL;

    if ( pString )
    {
        *ppResult = (LPWSTR)LocalAlloc(LPTR, StringByteSizeW(pString));

        if ( !*ppResult )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to get string buffer");

        StrCpyW(*ppResult, pString);
    }

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}


/*----------------------------------------------------------------------------
/ LocalAllocStringLen
/ -------------------
/   Given a length return a buffer of that size.
/
/ In:
/   ppResult -> receives the pointer to the string
/   cLen = length in characters to allocate
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

STDAPI LocalAllocStringLenA(LPSTR* ppResult, UINT cLen)
{
    HRESULT hr;

    TraceEnter(TRACE_COMMONAPI, "LocalAllocStringLenA");

    if ( !ppResult )
        ExitGracefully(hr, E_INVALIDARG, "Bad length, or NULL pointer");

    *ppResult = (LPSTR)LocalAlloc(LPTR, (cLen+1)*SIZEOF(CHAR));
    hr = (*ppResult) ? S_OK:E_OUTOFMEMORY;

exit_gracefully:

    TraceLeaveResult(hr);
}

STDAPI LocalAllocStringLenW(LPWSTR* ppResult, UINT cLen)
{
    HRESULT hr;

    TraceEnter(TRACE_COMMONAPI, "LocalAllocStringLenW");

    if ( !ppResult )
        ExitGracefully(hr, E_INVALIDARG, "Bad length, or NULL pointer");

    *ppResult = (LPWSTR)LocalAlloc(LPTR, (cLen+1)*SIZEOF(WCHAR));
    hr = (*ppResult) ? S_OK:E_OUTOFMEMORY;

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

VOID LocalFreeStringA(LPSTR* ppString)
{
    TraceEnter(TRACE_COMMONAPI, "LocalFreeStringA");
    LocalFreeStringW((LPWSTR*)ppString);
    TraceLeave();
}

VOID LocalFreeStringW(LPWSTR* ppString)
{
    TraceEnter(TRACE_COMMONAPI, "LocalFreeStringW");
    TraceAssert(ppString);

    if ( ppString )
    {
        if ( *ppString )
            LocalFree((HLOCAL)*ppString);

        *ppString = NULL;
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ LocalQueryString
/ ------------------
/   Hit the registry returning the wide version of the given string,
/   we dynamically allocate the buffer to put the result into,
/   this should be free'd by calling LocalFreeString.
/
/ In:
/   ppString -> receives the string point
/   hkey = key to query from
/   pSubKey -> pointer to sub key identifier
/   
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

STDAPI _LocalQueryString(LPTSTR* ppResult, HKEY hKey, LPCTSTR pSubKey)
{
    HRESULT hr;
    DWORD dwSize;

    TraceEnter(TRACE_COMMONAPI, "_LocalQueryString");

    *ppResult = NULL;

    if ( ERROR_SUCCESS != RegQueryValueEx(hKey, pSubKey, NULL, NULL, NULL, &dwSize) )
        ExitGracefully(hr, E_FAIL, "Failed when querying for key size");

    dwSize += SIZEOF(TCHAR);               
    *ppResult = (LPTSTR)LocalAlloc(LPTR, dwSize);

    if ( !*ppResult )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate buffer for value");

    if ( ERROR_SUCCESS != RegQueryValueEx(hKey, pSubKey, NULL, NULL, (LPBYTE)*ppResult, &dwSize) )
        ExitGracefully(hr, E_FAIL, "Failed to read key value into buffer");

    hr = S_OK;

exit_gracefully:

    if ( FAILED(hr) )
        LocalFreeString(ppResult);

    TraceLeaveResult(hr);
}

// Query string as ANSI, converting to ANSI if build UNICODE

STDAPI LocalQueryStringA(LPSTR* ppResult, HKEY hKey, LPCTSTR pSubKey)
{
    HRESULT hr;
    LPTSTR pResult = NULL;
    USES_CONVERSION;
    
    TraceEnter(TRACE_COMMONAPI, "LocalQueryStringA");

    *ppResult = NULL;       // incase of failure

#ifdef UNICODE
    hr = _LocalQueryString(&pResult, hKey, pSubKey);
    FailGracefully(hr, "Failed to read the UNICODE version of string");

    hr = LocalAllocStringW2A(ppResult, pResult);
    FailGracefully(hr, "Failed to allocate ANSI version of string");
#else
    hr = _LocalQueryString(ppResult, hKey, pSubKey);
    FailGracefully(hr, "Failed to get key value");
#endif

exit_gracefully:

    if ( FAILED(hr) )
        LocalFreeStringA(ppResult);

    LocalFreeString(&pResult);

    TraceLeaveResult(hr);
}

// Query string as UNICODE, converting to UNICODE if built ANSI

STDAPI LocalQueryStringW(LPWSTR* ppResult, HKEY hKey, LPCTSTR pSubKey)
{
    HRESULT hr;
    LPTSTR pResult = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_COMMONAPI, "LocalQueryStringW");

    *ppResult = NULL;                   // incase of failure

#ifdef UNICODE
    hr = _LocalQueryString(ppResult, hKey, pSubKey);
    FailGracefully(hr, "Falied to get key value");
#else
    hr = _LocalQueryString(&pResult, hKey, pSubKey);
    FailGracefully(hr, "Failed to query key as ANSI string");

    hr = LocalAllocStringA2W(ppResult, pResult);
    FailGracefully(hr, "Failed to allocate UNICODE version of string");
#endif

exit_gracefully:

    if ( FAILED(hr) )
        LocalFreeStringW(ppResult);

    LocalFreeString(&pResult);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ LocalAllocStringA2W / W2A
/ -------------------------
/   Alloc a string converting using MultiByteToWideChar or vice versa.  This
/   allows in place thunking of strings without extra buffer usage.
/
/ In:
/   ppResult -> receives the string point
/   pString -> source string
/   
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

STDAPI LocalAllocStringA2W(LPWSTR* ppResult, LPCSTR pString)
{
    HRESULT hr;
    INT iLen;

    TraceEnter(TRACE_COMMONAPI, "LocalAllocStringA2W");

    if ( !ppResult && !pString )
        ExitGracefully(hr, E_INVALIDARG, "Bad args for thunked allocate");

    iLen = MultiByteToWideChar(CP_ACP, 0, pString, -1, NULL, 0);

    hr = LocalAllocStringLenW(ppResult, iLen);
    FailGracefully(hr, "Failed to allocate buffer for string");

    MultiByteToWideChar(CP_ACP, 0, pString, -1, *ppResult, iLen+1);

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}

STDAPI LocalAllocStringW2A(LPSTR* ppResult, LPCWSTR pString)
{
    HRESULT hr;
    INT iLen;

    TraceEnter(TRACE_COMMONAPI, "LocalAllocStringW2A");

    if ( !ppResult && !pString )
        ExitGracefully(hr, E_INVALIDARG, "Bad args for thunked allocate");

    iLen = WideCharToMultiByte(CP_ACP, 0, pString, -1, NULL, 0, NULL, NULL);
    
    hr = LocalAllocStringLenA(ppResult, iLen);
    FailGracefully(hr, "Failed to allocate buffer for string");

    WideCharToMultiByte(CP_ACP, 0, pString, -1, *ppResult, iLen+1, NULL, NULL);

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ PutStringElement
/ -----------------
/   Add a string to the given buffer, always updating the cLen to indicate
/   how many characters would have been added
/
/ In:
/   pBuffer -> buffer to append to
/   pLen -> length value (updated)
/   pString -> string to add to buffer
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
STDAPI_(VOID) PutStringElementA(LPSTR pBuffer, UINT* pLen, LPCSTR pElement)
{
    TraceEnter(TRACE_COMMONAPI, "PutStringElementA");

    if ( pElement )
    {
        if ( pBuffer )
            lstrcatA(pBuffer, pElement);

        if ( pLen )
            *pLen += lstrlenA(pElement);
    }

    TraceLeave();
}

STDAPI_(VOID) PutStringElementW(LPWSTR pBuffer, UINT* pLen, LPCWSTR pElement)
{
    TraceEnter(TRACE_COMMONAPI, "PutStringElementW");

    if ( pElement )
    {
        if ( pBuffer )
            StrCatW(pBuffer, pElement);

        if ( pLen )
            *pLen += lstrlenW(pElement);
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ GetStringElement
/ ----------------
/   Extract the n'th element from the given string.  Each element is assumed
/   to be terminated with either a "," or a NULL.
/
/ In:
/   pString -> string to parse
/   index = element to retrieve
/   pBuffer, cchBuffer = buffer to fill 
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

STDAPI GetStringElementA(LPSTR pString, INT index, LPSTR pBuffer, INT cchBuffer)
{
    HRESULT hr = E_FAIL;
    USES_CONVERSION;

    TraceEnter(TRACE_COMMONAPI, "GetStringElement");
    Trace(TEXT("pString %s, index %d"), A2T(pString), index);

    *pBuffer = '\0';

    for ( ; index > 0 ; index-- )
    {
        while ( (*pString != ',') && (*pString != '\0') )
            pString++;

        if ( *pString == ',' )
            pString++;
    }

    if ( !index )
    {
        while ( *pString == ' ' )
            pString++;

        while ( cchBuffer-- && (*pString != ',') && (*pString != '\0') )
            *pBuffer++ = *pString++;
    
        if ( cchBuffer )
            *pBuffer = '\0';

        hr = cchBuffer ? S_OK:E_FAIL;
    }

    TraceLeaveResult(hr);
}

STDAPI GetStringElementW(LPWSTR pString, INT index, LPWSTR pBuffer, INT cchBuffer)
{
    HRESULT hr = E_FAIL;
    USES_CONVERSION;

    TraceEnter(TRACE_COMMONAPI, "GetStringElement");
    Trace(TEXT("pString %s, index %d"), W2T(pString), index);

    *pBuffer = L'\0';

    for ( ; index > 0 ; index-- )
    {
        while ( *pString != L',' && *pString != L'\0' )
            pString++;

        if ( *pString == L',' )
            pString++;
    }

    if ( !index )
    {
        while ( *pString == L' ' )
            pString++;

        while ( cchBuffer-- && (*pString != L',') && (*pString != L'\0') )
            *pBuffer++ = *pString++;
    
        if ( cchBuffer )
            *pBuffer = L'\0';

        hr = cchBuffer ? S_OK:E_FAIL;
    }

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ FormatMsgResource
/ -----------------
/   Load a string resource and pass it to format message, allocating a buffer
/   as we go.
/
/ In:
/   ppString -> receives the string point
/   hInstance = module handle for template string
/   uID = template string
/   ... = format parameters
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI FormatMsgResource(LPTSTR* ppString, HINSTANCE hInstance, UINT uID, ...)
{
    HRESULT hr;
    TCHAR szBuffer[MAX_PATH];
    va_list va;
    
    TraceEnter(TRACE_COMMONAPI, "FormatMsgResource");

    va_start(va, uID);

    if ( !LoadString(hInstance, uID, szBuffer, ARRAYSIZE(szBuffer)) )
        ExitGracefully(hr, E_FAIL, "Failed to load template string");

    if ( !FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER, 
                        (LPVOID)szBuffer, 0, 0, 
                        (LPTSTR)ppString, SIZEOF(ppString), 
                        &va) )
    {
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to format the message");
    }

    Trace(TEXT("Resulting string: %s"), *ppString);
    hr = S_OK;                                          // success

exit_gracefully:
    
    va_end(va);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ FormatMsgBox
/ ------------
/   Call FormatMessage and MessageBox together having built a suitable
/   string to display to the user.
/
/ In:
/   ppString -> receives the string point
/   hInstance = module handle for template string
/   uID = template string
/   ... = format parameters
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI_(INT) FormatMsgBox(HWND hWnd, HINSTANCE hInstance, UINT uidTitle, UINT uidPrompt, UINT uType, ...)
{
    INT iResult = -1;                   // failure
    LPTSTR pPrompt = NULL;
    TCHAR szTitle[MAX_PATH];
    TCHAR szBuffer[MAX_PATH];
    va_list va;
    
    TraceEnter(TRACE_COMMONAPI, "FormatMsgBox");

    va_start(va, uType);

    LoadString(hInstance, uidTitle, szTitle, ARRAYSIZE(szTitle));
    LoadString(hInstance, uidPrompt, szBuffer, ARRAYSIZE(szBuffer));

    if ( FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER, 
                       (LPVOID)szBuffer, 0, 0, 
                       (LPTSTR)&pPrompt, SIZEOF(pPrompt), 
                       &va) )
    {
        Trace(TEXT("Title: %s"), szTitle);
        Trace(TEXT("Prompt: %s"), pPrompt);

        iResult = MessageBox(hWnd, pPrompt, szTitle, uType);
        LocalFree(pPrompt);
    }

    Trace(TEXT("Result is %d"), iResult);

    va_end(va);

    TraceLeaveValue(iResult);
}


/*-----------------------------------------------------------------------------
/ FormatDirectoryName
/ -------------------
/   Collect the directory name and format it using a text resource specified.
/
/ In:
/   ppString = receives the string pointer for the result
/   clisdNamespace = namespace instance
/   hInstance = instance handle to load resource from
/   uID = resource ID for string
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI FormatDirectoryName(LPTSTR* ppString, HINSTANCE hInstance, UINT uID)
{
    HRESULT hr;
    TCHAR szBuffer[MAX_PATH];
    LPTSTR pDisplayName = NULL;
    HKEY hKey = NULL;

    TraceEnter(TRACE_COMMONAPI, "FormatDirectoryName");

    // No IDsFolder then lets ensure that we have one

    hr = GetKeyForCLSID(CLSID_MicrosoftDS, NULL, &hKey);
    FailGracefully(hr, "Failed to open namespace's registry key");

    hr = LocalQueryString(&pDisplayName, hKey, NULL);
    FailGracefully(hr, "Failed to get the namespace display name");

    Trace(TEXT("Display name is: %s"), pDisplayName);

    if ( hInstance )
    {
        hr = FormatMsgResource(ppString, hInstance, uID, pDisplayName);
        FailGracefully(hr, "Failed to format from resource");
    }
    else
    {
        *ppString = pDisplayName;
        pDisplayName = NULL;
    }

    hr = S_OK;                   // success

exit_gracefully:

    LocalFreeString(&pDisplayName);

    if ( hKey )
        RegCloseKey(hKey);

    TraceLeaveResult(hr);
}
