#include "pch.h"
#include "lm.h"
#include "ntdsapi.h"
#include "dsgetdc.h"
#include "dsrole.h"
#include "security.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Locals & helper functions
/----------------------------------------------------------------------------*/

// BUGBUG: ChandanaS says use LocalFree when building for Win95 as the 
// BUGBUG: NetApiBufferFree is not available, she will fix this in
// BUGBUG: time.

#if !defined(UNICODE) 
#define NetApiBufferFree(x) LocalFree(x)
#endif


/*-----------------------------------------------------------------------------
/ _StringFromSearchColumnArray
/ ----------------------------
/   Given an ADS_SEARCH_COLUMN attempt to get the string version of that
/   property.
/
/ In:
/   pColumn -> ADS_SEARCH_COLUMN structure to be unpicked
/   i = index for the column to be fetched
/   pBuffer, pLen = updated accordingly
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
VOID _StringFromSearchColumnArray(PADS_SEARCH_COLUMN pColumn, INT i, LPWSTR pBuffer, UINT* pLen)
{
    LPWSTR pValue;
    TCHAR szBuffer[MAX_PATH];
    USES_CONVERSION;

    TraceEnter(TRACE_DS, "_StringFromSearchColumnArray");

    switch ( pColumn->dwADsType )
    {
        case ADSTYPE_DN_STRING:
        case ADSTYPE_CASE_EXACT_STRING:
        case ADSTYPE_CASE_IGNORE_STRING:
        case ADSTYPE_PRINTABLE_STRING:
        case ADSTYPE_NUMERIC_STRING:
            PutStringElementW(pBuffer, pLen, pColumn->pADsValues[i].DNString);
            break;

        case ADSTYPE_BOOLEAN:
            PutStringElementW(pBuffer, pLen, (pColumn->pADsValues[i].Boolean) ? L"1":L"0");
            break;
            
        case ADSTYPE_INTEGER:    
            wsprintf(szBuffer, TEXT("%d"), (INT)pColumn->pADsValues[i].Integer);
            PutStringElementW(pBuffer, pLen, T2W(szBuffer));
            break;

        case ADSTYPE_OCTET_STRING:
        {
            for ( ULONG j = 0; j < pColumn->pADsValues[i].OctetString.dwLength; j++) 
            {
                wsprintf(szBuffer, TEXT("%02x"), ((LPBYTE)pColumn->pADsValues[i].OctetString.lpValue)[j]);
                PutStringElementW(pBuffer, pLen, T2W(szBuffer));
            }

            break;
        }

        case ADSTYPE_LARGE_INTEGER:
            wsprintf(szBuffer, TEXT("%e"), (double)pColumn->pADsValues[i].Integer);
            PutStringElementW(pBuffer, pLen, T2W(szBuffer));
            break;

        default:
            break;
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ StringFromSearchColumn
/ ----------------------
/   Given an ADS_SEARCH_COLUMN attempt to get the string version of that
/   property.
/
/ In:
/   pColumn -> ADS_SEARCH_COLUMN structure to be unpicked
/   pBuffer, pLen = the buffer to be filled (NULL accepted for both)
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
VOID _StringFromSearchColumn(PADS_SEARCH_COLUMN pColumn, LPWSTR pBuffer, UINT* pLen)
{
    DWORD index;

    TraceEnter(TRACE_DS, "_StringFromSearchColumn");

    if ( pBuffer )
        pBuffer[0] = TEXT('\0');

    for ( index = 0 ; index != pColumn->dwNumValues; index++ )
    {
        if ( index > 0 )
            PutStringElementW(pBuffer, pLen, L", ");

        _StringFromSearchColumnArray(pColumn, index, pBuffer, pLen);
    }

    TraceLeave();
}

STDAPI StringFromSearchColumn(PADS_SEARCH_COLUMN pColumn, LPWSTR* ppBuffer)
{
    HRESULT hr;
    UINT len = 0;

    TraceEnter(TRACE_DS, "StringFromSearchColumn");

    _StringFromSearchColumn(pColumn, NULL, &len);

    if ( len )
    {
        hr = LocalAllocStringLenW(ppBuffer, len);
        FailGracefully(hr, "Failed to allocate buffer for string");

        _StringFromSearchColumn(pColumn, *ppBuffer, NULL);
        Trace(TEXT("Resulting string: %s"), *ppBuffer);
    }

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ ObjectClassFromSearchColumn
/ ----------------------------
/   Given an ADS_SEARCH_COLUMN extract the object class from it.  Object class
/   is a multi-value property therefore we need to try and find which element
/   is the real class name.
/
/   All object have a base class "top", therefore we check the last element
/   of the property array, if that is "top" then we use the first element,
/   otherwise the last.
/
/ In:
/   pBuffer, cchBuffer = buffer to be filled
/   pColumn -> ADS_SEARCH_COLUMN structure to be unpicked
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI ObjectClassFromSearchColumn(PADS_SEARCH_COLUMN pColumn, LPWSTR* ppBuffer)
{
    HRESULT hr;
    WCHAR szBuffer[MAX_PATH];
    ULONG i;
    
    TraceEnter(TRACE_DS, "ObjectClassFromSearchColumn");
    
    szBuffer[0] = TEXT('\0');
    _StringFromSearchColumnArray(pColumn, 0, szBuffer, NULL);

    if ( !StrCmpIW(szBuffer, L"top") )
    {
        szBuffer[0] = TEXT('\0');
        _StringFromSearchColumnArray(pColumn, pColumn->dwNumValues-1, szBuffer, NULL);
    }

    hr = LocalAllocStringW(ppBuffer, szBuffer);
    FailGracefully(hr, "Failed to get alloc string buffer");

    // hr = S_OK;                       // success

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ GetArrayContents
/ ----------------
/   Given a VARIANT call the callback function with each element that we
/   see in it.  If the VARIANT is an array then call the callback in the 
/   correct order to give sensible results.
/
/ In:
/   pVariant -> VARAINT to be unpacked
/   pCB, pData -> callback to be called for each item
/
/ Out:
/   HRESULT   
/----------------------------------------------------------------------------*/

INT _GetArrayCompareCB(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    HRESULT hr;
    WCHAR szBuffer[MAX_PATH];
    LONG i = 0;

    TraceEnter(TRACE_DS, "_GetArrayCompareCB");

    hr = GetStringElementW((BSTR)p1, 0, szBuffer, ARRAYSIZE(szBuffer));
    FailGracefully(hr, "Failed to get the position value");

    i = StringToDWORD(szBuffer);

    hr = GetStringElementW((BSTR)p2, 0, szBuffer, ARRAYSIZE(szBuffer));
    FailGracefully(hr, "Failed to get the position value");

exit_gracefully:

    TraceLeaveValue(i - StringToDWORD(szBuffer));
}

STDAPI GetArrayContents(LPVARIANT pVariant, LPGETARRAYCONTENTCB pCB, LPVOID pData)
{
    HRESULT hr;
    LONG arrayMin, arrayMax, i;
    WCHAR szBuffer[MAX_PATH];
    VARIANT varElement;
    HDPA hdpa = NULL;
    LPWSTR pValue;
    DWORD dwIndex;

    TraceEnter(TRACE_DS, "GetArrayContents");

    VariantInit(&varElement);

    switch ( V_VT(pVariant) )
    {
        case VT_BSTR:
        {
            hr = GetStringElementW(V_BSTR(pVariant), 0, szBuffer, ARRAYSIZE(szBuffer));
            FailGracefully(hr, "Failed to get the position value");

            dwIndex = StringToDWORD(szBuffer);

            pValue = wcschr(V_BSTR(pVariant), TEXT(','));        // NB: can return NULL (eg. not found)
            TraceAssert(pValue);

            if ( pValue )
            {
                hr = (*pCB)(dwIndex, pValue+1, pData);
                FailGracefully(hr, "Failed when calling with VT_BSTR");
            }

            break;
        }

        case VT_VARIANT | VT_ARRAY:
        {
            // read the VARIANTs into the DPA, don't worry about order just pick up
            // the contents of the array

            if ( (V_ARRAY(pVariant))->rgsabound[0].cElements < 1 )
                ExitGracefully(hr, E_FAIL, "Array less than 1 element in size");

            SafeArrayGetLBound(V_ARRAY(pVariant), 1, (LONG*)&arrayMin);
            SafeArrayGetUBound(V_ARRAY(pVariant), 1, (LONG*)&arrayMax);

            hdpa = DPA_Create(arrayMax-arrayMin);

            if ( !hdpa )
                ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate DPA");

            Trace(TEXT("arrayMin %d, arrayMax %d"), arrayMin, arrayMax);

            for ( i = arrayMin; i <= arrayMax; i++ )
            {
                hr = SafeArrayGetElement(V_ARRAY(pVariant), (LONG*)&i, &varElement);
                FailGracefully(hr, "Failed to look up in variant array");

                if ( V_VT(&varElement) == VT_BSTR )
                {
                    hr = StringDPA_AppendStringW(hdpa, V_BSTR(&varElement), NULL);
                    FailGracefully(hr, "Failed to add the string to the DPA");
                }

                VariantClear(&varElement);
            }

            // now sort the DPA based on the first element.  then pass them 
            // out the the caller, skipping the leading character
            
            if ( DPA_GetPtrCount(hdpa) > 0 )
            {
                DPA_Sort(hdpa, _GetArrayCompareCB, NULL);

                for ( i = 0 ; i != DPA_GetPtrCount(hdpa); i++ )
                {
                    hr = GetStringElementW(StringDPA_GetStringW(hdpa, i), 0, szBuffer, ARRAYSIZE(szBuffer));
                    FailGracefully(hr, "Failed to get the position value");

                    dwIndex = StringToDWORD(szBuffer);

                    pValue = wcschr((BSTR)DPA_FastGetPtr(hdpa, i), TEXT(','));        // nb: can be null one exit
                    TraceAssert(pValue);

                    if ( pValue )
                    {
                        hr = (*pCB)(dwIndex, pValue+1, pData);
                        FailGracefully(hr, "Failed when calling with VT_BSTR (from array)");
                    }
                }        
            }

            break;
        }

        case VT_EMPTY:
        {
            TraceMsg("VARIANT is empty");
            break;
        }
    }

    hr = S_OK;

exit_gracefully:

    VariantClear(&varElement);
    StringDPA_Destroy(&hdpa);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ GetDisplayNameFromADsPath
/ -------------------------
/   Convert the ADsPath to its display name with a suitable prefix.
/
/ In:
/   pszPath -> ADsPath to be displayed
/   pszBuffer, cchBuffer = buffer to return the name into
/   padp -> IADsPathname for increased perf
/   fPrefix = add the NTDS:// or not.
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

#define NAME_PREFIX         L"ntds://"
#define CCH_NAME_PREFIX     7

#define CHECK_WIN32(err)    ((err) == ERROR_SUCCESS)

STDAPI GetDisplayNameFromADsPath(LPCWSTR pszPath, LPWSTR pszBuffer, INT cchBuffer, IADsPathname *padp, BOOL fPrefix)
{
    HRESULT hres;
    BSTR bstrName = NULL;
    PDS_NAME_RESULTW pDsNameResult = NULL;
    DWORD dwError;
    INT i;
    USES_CONVERSION;

    TraceEnter(TRACE_DS, "GetDisplayNameFromADsPath");

    if ( !padp )
    {
        hres = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (LPVOID*)&padp);
        FailGracefully(hres, "Failed to get IADsPathname interface");
    }
    else
    {
        padp->AddRef();
    }

    if ( pszPath )
    {
        hres = padp->Set((LPWSTR)pszPath, ADS_SETTYPE_FULL);
        if ( SUCCEEDED(hres) )
        {
            hres = padp->Retrieve(ADS_FORMAT_X500_DN, &bstrName);
            FailGracefully(hres, "Failed to retreieve the X500 DN version");
        }
        else
        {
            bstrName = SysAllocString(pszPath);
            if ( !bstrName )
                ExitGracefully(hres, E_OUTOFMEMORY, "Failed to clone the string");
        }
    }
    else
    {
        hres = padp->Retrieve(ADS_FORMAT_X500_DN, &bstrName);
        FailGracefully(hres, "Failed to retreieve the X500 DN version");
    }

    //
    // try to syntatically crack the name we have
    //

    dwError = DsCrackNamesW(NULL, DS_NAME_FLAG_SYNTACTICAL_ONLY, DS_UNKNOWN_NAME, DS_CANONICAL_NAME, 
                                        1, &bstrName,  &pDsNameResult);

    if ( !CHECK_WIN32(dwError) || !CHECK_WIN32(pDsNameResult->rItems->status) )
        ExitGracefully(hres, E_FAIL, "Failed to crack the name");

    i = lstrlenW(pDsNameResult->rItems->pName)+(fPrefix ? CCH_NAME_PREFIX:0);
    if ( i > cchBuffer )
        ExitGracefully(hres, E_FAIL, "Buffer too small");

    *pszBuffer = L'\0';

    if ( fPrefix )
        StrCatW(pszBuffer, NAME_PREFIX);

    StrCatW(pszBuffer, pDsNameResult->rItems->pName);
    
    if ( pszBuffer[i-1] == L'/' )
        pszBuffer[i-1] = L'\0';             // trim trailing
    
    hres = S_OK;

exit_gracefully:

    if ( pDsNameResult )
        DsFreeNameResultW(pDsNameResult);

    DoRelease(padp);
    SysFreeString(bstrName);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CheckDsPolicy
/ -------------
/   Check under HKCU,Software\Policies\Microsoft\Windows\Directory UI 
/   for the given key/value which are assumed to be DWORD values.
/
/ In:
/   pSubKey = sub key to be opened / = NULL
/   pValue = value name to be checked
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI_(DWORD) CheckDsPolicy(LPCTSTR pSubKey, LPCTSTR pValue)
{
    DWORD dwFlag = 0;
    TCHAR szBuffer[MAX_PATH];
    DWORD dwType, cbSize;
    HKEY hKey = NULL;

    TraceEnter(TRACE_DS, "CheckDsPolicy");

    // format the key, this is stored under HKCU, if the user gives a sub
    // key then lets ensure that we look under that

    StrCpy(szBuffer, TEXT("Software\\Policies\\Microsoft\\Windows\\Directory UI"));

    if ( pSubKey )
    {
        StrCat(szBuffer, TEXT("\\"));
        StrCat(szBuffer, pSubKey);
    }

    Trace(TEXT("Directopy policy key is: %s"), szBuffer);

    // Open the key and then query for the value, ensuring that the value is
    // stored in a DWORD.

    if ( CHECK_WIN32(RegOpenKey(HKEY_CURRENT_USER, szBuffer, &hKey)) )
    {
        if ( (CHECK_WIN32(RegQueryValueEx(hKey, pValue, NULL, &dwType, NULL, &cbSize))) && 
              (dwType == REG_DWORD) && 
                (cbSize == SIZEOF(dwFlag)) )
        {
            RegQueryValueEx(hKey, pValue, NULL, NULL, (LPBYTE)&dwFlag, &cbSize);
            Trace(TEXT("Policy value %s is %08x"), pValue, dwFlag);
        }
    }

    if ( hKey )
        RegCloseKey(hKey);

    TraceLeaveValue(dwFlag);
}


/*-----------------------------------------------------------------------------
/ ShowDirectoryUI
/ ---------------
/   Check to see if we should make the directory UI visible.  This we do
/   by seeing if the machine and user is logged into a valid DS.
/
/   RichardW added an new variable to the environement block "USERDNSDOMAIN"
/   which if present we will show the UI, otherwise not.  This is not the
/   perfect solution, but works.
/
/ In:
/ Out:
/   BOOL
/----------------------------------------------------------------------------*/
STDAPI_(BOOL) ShowDirectoryUI(VOID)
{
#ifdef WINNT
    BOOL fResult = FALSE;

    TraceEnter(TRACE_DS, "ShowDirectoryUI");

    if ( GetEnvironmentVariable(TEXT("USERDNSDOMAIN"), NULL, 0) )
    {
        TraceMsg("USERDNSDOMAIN defined in environment, therefore returning TRUE");
        fResult = TRUE;
    }

    if ( !fResult )
    {
        DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pInfo;
        DWORD dwError = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (BYTE**)&pInfo);
        if ( CHECK_WIN32(dwError) )
        {
            if ( pInfo->DomainNameDns )
            {
                TraceMsg("Machine domain is DNS, therefore we assume DS is available");
                fResult = TRUE;
            }

            DsRoleFreeMemory(pInfo);
        }
    }

    TraceLeaveResult(fResult);
#else
    TraceEnter(TRACE_DS, "ShowDirectoryUI");
    TraceMsg("*** Returning TRUE always ***");
    TraceLeaveValue(TRUE);
#endif
}
