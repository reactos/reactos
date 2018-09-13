#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Helper functions
/----------------------------------------------------------------------------*/

HRESULT _GetQueryString(LPWSTR pQuery, UINT* pLen, LPWSTR pPrefixQuery, HWND hDlg, LPPAGECTRL aCtrl, INT iCtrls);
HRESULT _GetFilterQueryString(LPWSTR pFilter, UINT* pLen, HWND hwndFilter, HDSA hdsaColumns);


/*-----------------------------------------------------------------------------
/ Query paremeter helpers
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ ClassListAlloc
/ --------------
/   Construct a class list allocation based on the list of classes
/   we are given.
/
/ In:
/   ppClassList -> receives a class list 
/   cClassList / cClassList = array of classes to allocat from
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI ClassListAlloc(LPDSQUERYCLASSLIST* ppDsQueryClassList, LPWSTR* aClassNames, INT cClassNames)
{
    HRESULT hres;
    DWORD cbStruct, offset;
    LPDSQUERYCLASSLIST pDsQueryClassList = NULL;
    INT i;
    USES_CONVERSION;

    TraceEnter(TRACE_FORMS, "ClassListAlloc");

    if ( !ppDsQueryClassList || !aClassNames || !cClassNames )
        ExitGracefully(hres, E_FAIL, "Bad parameters (no class list etc)");

    // Walk the list of classes working out the size of the structure
    // we are going to generate, this consists of the array of 
    // classes.

    cbStruct = SIZEOF(DSQUERYCLASSLIST)+(cClassNames*SIZEOF(DWORD));
    offset = cbStruct;

    for ( i = 0 ; i < cClassNames ; i++ )
    {
        TraceAssert(aClassNames[i]);
        cbStruct += StringByteSizeW(aClassNames[i]);
    }

    // Allocate the structure using the task allocator, then fill
    // it in copying all the strings into the data blob.

    Trace(TEXT("Allocating class structure %d"), cbStruct);

    pDsQueryClassList = (LPDSQUERYCLASSLIST)CoTaskMemAlloc(cbStruct);
    TraceAssert(pDsQueryClassList);

    if ( !pDsQueryClassList )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate class list structure");

    pDsQueryClassList->cbStruct = cbStruct;
    pDsQueryClassList->cClasses = cClassNames;

    for ( i = 0 ; i < cClassNames ; i++ )
    {
        Trace(TEXT("Adding class: %s"), W2T(aClassNames[i]));
        pDsQueryClassList->offsetClass[i] = offset;
        StringByteCopyW(pDsQueryClassList, offset, aClassNames[i]);
        offset += StringByteSizeW(aClassNames[i]);
    }

    hres = S_OK;

exit_gracefully:

    TraceAssert(pDsQueryClassList);
    *ppDsQueryClassList = pDsQueryClassList;

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ QueryParamsAlloc
/ ----------------
/   Construct a block we can pass to the DS query handler which contains
/   all the parameters for the query.
/
/ In:
/   ppDsQueryParams -> receives the parameter block
/   pQuery -> LDAP query string to be used
/   hInstance = hInstance to write into parameter block
/   iColumns = number of columns
/   pColumnInfo -> column info structure to use
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI QueryParamsAlloc(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery, HINSTANCE hInstance, LONG iColumns, LPCOLUMNINFO aColumnInfo)
{
    HRESULT hres;
    LPDSQUERYPARAMS pDsQueryParams = NULL;
    LONG cbStruct;
    LONG i;

    TraceEnter(TRACE_FORMS, "QueryParamsAlloc");

    if ( !pQuery || !iColumns || !ppDsQueryParams )
        ExitGracefully(hres, E_INVALIDARG, "Failed to build query parameter block");

    // Compute the size of the structure we need to be using

    cbStruct  = SIZEOF(DSQUERYPARAMS) + (SIZEOF(DSCOLUMN)*iColumns);
    cbStruct += StringByteSizeW(pQuery);

    for ( i = 0 ; i < iColumns ; i++ )
    {
        if ( aColumnInfo[i].pPropertyName ) 
            cbStruct += StringByteSizeW(aColumnInfo[i].pPropertyName);
    }

    pDsQueryParams = (LPDSQUERYPARAMS)CoTaskMemAlloc(cbStruct);

    if ( !pDsQueryParams )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate parameter block");

    // Structure allocated so lets fill it with data

    pDsQueryParams->cbStruct = cbStruct;
    pDsQueryParams->dwFlags = 0;
    pDsQueryParams->hInstance = hInstance;
    pDsQueryParams->iColumns = iColumns;
    pDsQueryParams->dwReserved = 0;

    cbStruct  = SIZEOF(DSQUERYPARAMS) + (SIZEOF(DSCOLUMN)*iColumns);

    pDsQueryParams->offsetQuery = cbStruct;
    StringByteCopyW(pDsQueryParams, cbStruct, pQuery);
    cbStruct += StringByteSizeW(pQuery);

    for ( i = 0 ; i < iColumns ; i++ )
    {
        pDsQueryParams->aColumns[i].dwFlags = 0;
        pDsQueryParams->aColumns[i].fmt = aColumnInfo[i].fmt;
        pDsQueryParams->aColumns[i].cx = aColumnInfo[i].cx;
        pDsQueryParams->aColumns[i].idsName = aColumnInfo[i].idsName;
        pDsQueryParams->aColumns[i].dwReserved = 0;

        if ( aColumnInfo[i].pPropertyName ) 
        {
            pDsQueryParams->aColumns[i].offsetProperty = cbStruct;
            StringByteCopyW(pDsQueryParams, cbStruct, aColumnInfo[i].pPropertyName);
            cbStruct += StringByteSizeW(aColumnInfo[i].pPropertyName);
        }
        else
        {
            pDsQueryParams->aColumns[i].offsetProperty = aColumnInfo[i].iPropertyIndex;
        }
    }

    hres = S_OK;              // success

exit_gracefully:

    if ( FAILED(hres) && pDsQueryParams )
    {
        CoTaskMemFree(pDsQueryParams); 
        pDsQueryParams = NULL;
    }

    *ppDsQueryParams = pDsQueryParams;

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ QueryParamsAddQueryString
/ -------------------------
/   Given an existing DS query block appened the given LDAP query string into
/   it. We assume that the query block has been allocated by IMalloc (or CoTaskMemAlloc).
/
/ In:
/   ppDsQueryParams -> receives the parameter block
/   pQuery -> LDAP query string to be appended
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI QueryParamsAddQueryString(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery)
{
    HRESULT hres;
    LPWSTR pOriginalQuery = NULL;
    LPDSQUERYPARAMS pDsQuery = *ppDsQueryParams;
    INT cbQuery, i;

    TraceEnter(TRACE_FORMS, "QueryParamsAddQueryString");

    if ( pQuery )
    {
        if ( !pDsQuery )
            ExitGracefully(hres, E_INVALIDARG, "No query to append to");

        // Work out the size of the bits we are adding, take a copy of the
        // query string and finally re-alloc the query block (which may cause it
        // to move).
       
        cbQuery = StringByteSizeW(pQuery) + StringByteSizeW(L"(&)");
        Trace(TEXT("DSQUERYPARAMS being resized by %d bytes"), cbQuery);

        hres = LocalAllocStringW(&pOriginalQuery, (LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery));
        FailGracefully(hres, "Failed to take copy of original query string");

        pDsQuery = (LPDSQUERYPARAMS)CoTaskMemRealloc(pDsQuery, pDsQuery->cbStruct+cbQuery);
        if ( !pDsQuery )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to re-alloc control block");
        
        *ppDsQueryParams = pDsQuery;

        // Now move everything above the query string up, and fix all the
        // offsets that reference those items (probably the property table),
        // finally adjust the size to reflect the change

        MoveMemory(ByteOffset(pDsQuery, pDsQuery->offsetQuery+cbQuery), 
                   ByteOffset(pDsQuery, pDsQuery->offsetQuery), 
                   (pDsQuery->cbStruct - pDsQuery->offsetQuery));
                
        for ( i = 0 ; i < pDsQuery->iColumns ; i++ )
        {
            if ( pDsQuery->aColumns[i].offsetProperty > pDsQuery->offsetQuery )
            {
                Trace(TEXT("Fixing offset of property at index %d"), i);
                pDsQuery->aColumns[i].offsetProperty += cbQuery;
            }
        }

        StrCpyW((LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery), L"(&");
        StrCatW((LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery), pOriginalQuery);
        StrCatW((LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery), pQuery);        
        StrCatW((LPWSTR)ByteOffset(pDsQuery, pDsQuery->offsetQuery), L")");

        pDsQuery->cbStruct += cbQuery;
    }

    hres = S_OK;

exit_gracefully:

    LocalFreeStringW(&pOriginalQuery);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ Form to query string helper functions
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ GetQueryString
/ --------------
/   Build the form parmaeters into a LDAP query string using the given table.
/
/ In:
/   ppQuery -> receives the string pointer
/   pPrefixQuery -> string placed at head of query / = NULL if none
/   hDlg = handle for the dialog to get the data from
/   aCtrls / iCtrls = control information for the window
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI GetQueryString(LPWSTR* ppQuery, LPWSTR pPrefixQuery, HWND hDlg, LPPAGECTRL aCtrls, INT iCtrls)
{
    HRESULT hres;
    UINT cLen = 0;

    TraceEnter(TRACE_FORMS, "GetQueryString");

    hres = _GetQueryString(NULL, &cLen, pPrefixQuery, hDlg, aCtrls, iCtrls);
    FailGracefully(hres, "Failed 1st pass (compute string length)");

    if ( cLen )
    {
        hres = LocalAllocStringLenW(ppQuery, cLen);
        FailGracefully(hres, "Failed to alloc buffer for query string");

        hres = _GetQueryString(*ppQuery, &cLen, pPrefixQuery, hDlg, aCtrls, iCtrls);
        FailGracefully(hres, "Failed 2nd pass (fill buffer)");
    }

    hres = cLen ? S_OK:S_FALSE;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ _GetQueryString
/ ---------------
/   Build the string from the controls or just return the buffer size required.
/
/ In:
/   pQuery -> filled with query string / = NULL
/   pLen = updated to reflect the required string length
/   pPrefixQuery -> string placed at head of query / = NULL if none
/   hDlg = handle for the dialog to get the data from
/   aCtrls / iCtrls = control information for the window
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT _GetQueryString(LPWSTR pQuery, UINT* pLen, LPWSTR pPrefixQuery, HWND hDlg, LPPAGECTRL aCtrl, INT iCtrls)
{
    HRESULT hres;
    INT i;
    TCHAR szBuffer[MAX_PATH];
    USES_CONVERSION;

    TraceEnter(TRACE_FORMS, "_GetQueryString");

    if ( !hDlg || (!aCtrl && iCtrls) )
        ExitGracefully(hres, E_INVALIDARG, "No dialog or controls list");

    Trace(TEXT("Checking %d controls"), iCtrls);

    PutStringElementW(pQuery, pLen, pPrefixQuery);

    for ( i = 0 ; i < iCtrls; i++ )
    {
        if ( GetDlgItemText(hDlg, aCtrl[i].nIDDlgItem, szBuffer, ARRAYSIZE(szBuffer)) )
        {
            Trace(TEXT("Property %s, value %s"), W2T(aCtrl[i].pPropertyName), szBuffer);
            GetFilterString(pQuery, pLen, aCtrl[i].iFilter, aCtrl[i].pPropertyName, T2W(szBuffer));
        }
    }

    Trace(TEXT("Resulting query is -%s- (%d)"), pQuery ? W2T(pQuery):TEXT("<no buffer>"), pLen ? *pLen:0);

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);    
}


/*-----------------------------------------------------------------------------
/ GetFilterString
/ ---------------
/   Given a property, a property and its filter generate a suitable filter
/   string that map returning it into the given buffer via PutStringElement.
/
/ In:
/   pFilter, pLen = buffer information that we are returning int
/   iFilter = condition to be applied
/   pProperty -> property name
/   pValue -> value
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

struct
{
    BOOL fNoValue;
    BOOL fFixWildcard;
    LPWSTR pPrefix;
    LPWSTR pOperator;
    LPWSTR pPostfix;
}
filter_info[] =
{

//
// The server today does not support contains searches, therefore
// for consistency map that to a STARTSWITH, NOTSTARTSWITH
//

#if 0
    0, 1, L"(",  L"=*",   L"*)",     // CONTAIN
    0, 1, L"(!", L"=*",   L"*)",     // NOTCONTAINS
#else 
    0, 1, L"(",  L"=",    L"*)",     // CONTAINS
    0, 1, L"(!", L"=",    L"*)",     // NOTCONTAINS
#endif

    0, 1, L"(",  L"=",    L"*)",     // STARTSWITH
    0, 1, L"(",  L"=*",   L")",      // ENDSWITH
    0, 0, L"(",  L"=",    L")",      // IS
    0, 0, L"(!", L"=",    L")",      // ISNOT
    0, 0, L"(",  L">=",   L")",      // GREATEREQUAL
    0, 0, L"(",  L"<=",   L")",      // LESSEQUAL
    1, 0, L"(",  L"=*)",  NULL,      // DEFINED
    1, 0, L"(!", L"=*)",  NULL,      // UNDEFINED

    1, 0, L"(",  L"=TRUE)",  NULL,   // TRUE
    1, 0, L"(!", L"=TRUE)",  NULL,   // FALSE
};

STDAPI GetFilterString(LPWSTR pFilter, UINT* pLen, INT iFilter, LPWSTR pProperty, LPWSTR pValue)
{
    HRESULT hres;
    USES_CONVERSION;

    TraceEnter(TRACE_VIEW, "GetFilterString");

    // Check to see if the value we have contains a wildcard, if it does then just 
    // make it is exact assuming the user knows what they are doing - ho ho ho!

    if ( pValue && filter_info[iFilter-FILTER_FIRST].fFixWildcard )
    {
        if ( wcschr(pValue, L'*') )
        {
            TraceMsg("START/ENDS contains wildcards, making is exactly"); 
            iFilter = FILTER_IS;
        }
    }

    // Fix the condition to index into the our array then 
    // put the string elements down

    iFilter -= FILTER_FIRST;                     // compensate for non-zero index

    if ( iFilter >= ARRAYSIZE(filter_info) )
        ExitGracefully(hres, E_FAIL, "Bad filter value");

    PutStringElementW(pFilter, pLen, filter_info[iFilter].pPrefix);
    PutStringElementW(pFilter, pLen, pProperty);
    PutStringElementW(pFilter, pLen, filter_info[iFilter].pOperator);

    if ( !filter_info[iFilter].fNoValue )
        PutStringElementW(pFilter, pLen, pValue);

    PutStringElementW(pFilter, pLen, filter_info[iFilter].pPostfix);

    Trace(TEXT("Filter is: %s"), pFilter ? W2T(pFilter):TEXT("<none>"));

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ GetPatternString
/ ----------------
/   Given a string wrap in suitable wildcards to do the filtering of
/   results.
/
/ In:
/   pPattern, pLen = buffer information that we are returning int
/   iFilter = condition to be applied
/   pValue -> value
/
/ Out:
/   VOID
/----------------------------------------------------------------------------*/

struct
{
    LPTSTR pPrefix;
    LPTSTR pPostfix;
}
pattern_info[] =
{
    TEXT("*"), TEXT("*"),     // CONTAIN
    TEXT("*"), TEXT("*"),     // NOTCONTAINS
    TEXT(""),  TEXT("*"),     // STARTSWITH
    TEXT("*"), TEXT(""),      // ENDSWITH
    TEXT(""),  TEXT(""),      // IS
    TEXT(""),  TEXT(""),      // ISNOT
};

STDAPI GetPatternString(LPTSTR pFilter, UINT* pLen, INT iFilter, LPTSTR pValue)
{
    HRESULT hres;

    TraceEnter(TRACE_VIEW, "GetFilterString");

    iFilter -= FILTER_FIRST;                     // compensate for non-zero index

    if ( iFilter >= ARRAYSIZE(pattern_info) )
        ExitGracefully(hres, E_FAIL, "Bad filter value");

    PutStringElement(pFilter, pLen, pattern_info[iFilter].pPrefix);
    PutStringElement(pFilter, pLen, pValue);
    PutStringElement(pFilter, pLen, pattern_info[iFilter].pPostfix);

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ Dialog helper functions
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ EnablePageControls
/ ------------------
/   Enable/Disable the controls on a query form.
/
/ In:
/   hDlg = handle for the dialog to get the data from
/   aCtrls / iCtrls = control information for the window
/   fEnable = TRUE/FALSE to enable disable window controls
/
/ Out:
/   VOID
/----------------------------------------------------------------------------*/
STDAPI_(VOID) EnablePageControls(HWND hDlg, LPPAGECTRL aCtrl, INT iCtrls, BOOL fEnable)
{
    HRESULT hres;
    INT i;
    HWND hwndCtrl;

    TraceEnter(TRACE_FORMS, "EnablePageControls");

    if ( !hDlg || (!aCtrl && iCtrls) )
        ExitGracefully(hres, E_INVALIDARG, "No dialog or controls list");

    Trace(TEXT("%s %d controls"), fEnable ? TEXT("Enabling"):TEXT("Disabling"),iCtrls);

    for ( i = 0 ; i < iCtrls; i++ )
    {
        hwndCtrl = GetDlgItem(hDlg, aCtrl[i].nIDDlgItem);

        if  ( hwndCtrl )
            EnableWindow(hwndCtrl, fEnable);
    }

exit_gracefully:

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ ResetPageControls
/ ------------------
/   Reset all the form controls back to their default state.
/
/ In:
/   hDlg = handle for the dialog to get the data from
/   aCtrls / iCtrls = control information for the window
/
/ Out:
/   VOID
/----------------------------------------------------------------------------*/
STDAPI_(VOID) ResetPageControls(HWND hDlg, LPPAGECTRL aCtrl, INT iCtrls)
{
    HRESULT hres;
    INT i;

    TraceEnter(TRACE_FORMS, "ResetPageControls");

    if ( !hDlg || (!aCtrl && iCtrls) )
        ExitGracefully(hres, E_INVALIDARG, "No dialog or controls list");

    for ( i = 0 ; i < iCtrls; i++ )
        SetDlgItemText(hDlg, aCtrl[i].nIDDlgItem, TEXT(""));

exit_gracefully:

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ SetDlgItemFromProperty
/ ----------------------
/   Given an IPropertyBag interface set the control with the text for 
/   that property.  We assume the property is a string.
/
/ In:
/   ppb -> IPropertyBag
/   pszProperty -> property to read
/   hwnd, id = control information
/   pszDefault = default text / = NULL if not important
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI SetDlgItemFromProperty(IPropertyBag* ppb, LPCWSTR pszProperty, HWND hwnd, INT id, LPCWSTR pszDefault)
{
    HRESULT hres;
    VARIANT variant;
    USES_CONVERSION;

    TraceEnter(TRACE_FORMS, "SetDlgItemFromProperty");

    VariantInit(&variant);

    if ( ppb && SUCCEEDED(ppb->Read(pszProperty, &variant, NULL)) )
    {
        if ( V_VT(&variant) == VT_BSTR )
        {
            pszDefault = V_BSTR(&variant);
            Trace(TEXT("property contained: %s"), W2CT(pszDefault));                
        }
    }

    if ( pszDefault )
        SetDlgItemText(hwnd, id, W2CT(pszDefault));

    VariantClear(&variant);

    TraceLeaveResult(S_OK);
}


/*-----------------------------------------------------------------------------
/ Query Persistance
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ PersistQuery
/ ------------
/   Persist a query into a IPersistQuery object
/
/ In:
/   pPersistQuery = query to persist into
/   fRead = read?
/   pSectionName = section name to use when persisting
/   hDlg = DLG to persist from
/   aCtrls / iCtrls = ctrls to be persisted
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDAPI PersistQuery(IPersistQuery* pPersistQuery, BOOL fRead, LPCTSTR pSection, HWND hDlg, LPPAGECTRL aCtrl, INT iCtrls)
{
    HRESULT hres = S_OK;
    TCHAR szBuffer[MAX_PATH];
    INT i;
    USES_CONVERSION;

    TraceEnter(TRACE_IO, "PersistQuery");

    if ( !pPersistQuery || !hDlg || (!aCtrl && iCtrls) )
        ExitGracefully(hres, E_INVALIDARG, "No data to persist");

    for ( i = 0 ; i < iCtrls ; i++ )
    {
        if ( fRead )
        {
            if ( SUCCEEDED(pPersistQuery->ReadString(pSection, W2T(aCtrl[i].pPropertyName), szBuffer, ARRAYSIZE(szBuffer))) )
            {
                Trace(TEXT("Reading property: %s,%s as %s"), pSection, W2T(aCtrl[i].pPropertyName), szBuffer);
                SetDlgItemText(hDlg, aCtrl[i].nIDDlgItem, szBuffer);
            }
        }
        else
        {
            if ( GetDlgItemText(hDlg, aCtrl[i].nIDDlgItem, szBuffer, ARRAYSIZE(szBuffer)) )
            {
                Trace(TEXT("Writing property: %s,%s as %s"), pSection, W2T(aCtrl[i].pPropertyName), szBuffer);
                hres = pPersistQuery->WriteString(pSection, W2T(aCtrl[i].pPropertyName), szBuffer);
                FailGracefully(hres, "Failed to write control data");
            }
        }
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}
