#include "pch.h"
#include "stddef.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Helper functions (exported)
/----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
/ MergeMenu
/ ---------
/   Merge two menus together, taking the first popup menu and merging it into
/   the target.  We use the caption from the pop-up menu as the caption
/   for the target.
/
/ In:
/   hMenu = handle of menu to merge into
/   hMenuToInsert = handle of menu to get the popup from
/   iIndex = index to insert at
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID MergeMenu(HMENU hMenu, HMENU hMenuToInsert, INT iIndex)
{
    TCHAR szBuffer[MAX_PATH];
    HMENU hPopupMenu = NULL;

    TraceEnter(TRACE_HANDLER|TRACE_VIEW, "MergeMenu");
    
    hPopupMenu = CreatePopupMenu();
    
    if ( hPopupMenu )
    {
        GetMenuString(hMenuToInsert, 0, szBuffer, ARRAYSIZE(szBuffer), MF_BYPOSITION);
        InsertMenu(hMenu, iIndex, MF_BYPOSITION|MF_POPUP, (UINT_PTR)hPopupMenu, szBuffer);

        Shell_MergeMenus(hPopupMenu, GetSubMenu(hMenuToInsert, 0), 0x0, 0x0, 0x7fff, 0);
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ BindToPath
/ ----------
/   Given a namespace path bind to it returning the shell object
/
/ In:
/   pszPath -> path to bind to
/   riid = interface to request
/   ppvObject -> receives the object pointer
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT BindToPath(LPWSTR pszPath, REFIID riid, LPVOID* ppObject)
{
    HRESULT hres;
    IShellFolder* psfDesktop = NULL;
    LPITEMIDLIST pidl = NULL;

    TraceEnter(TRACE_VIEW, "BindToPath");

    hres = CoCreateInstance(CLSID_ShellDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IShellFolder, (LPVOID*)&psfDesktop);
    FailGracefully(hres, "Failed to get IShellFolder for the desktop object");

    hres = psfDesktop->ParseDisplayName(NULL, NULL, pszPath, NULL, &pidl, NULL);
    FailGracefully(hres, "Failed when getting root path of DS");
    
    if ( ILIsEmpty(pidl) )
    {
        TraceMsg("PIDL is desktop, therefore just QIing for interface");
        hres = psfDesktop->QueryInterface(riid, ppObject);
    }
    else
    {
        TraceMsg("Binding to IDLIST via BindToObject");
        hres = psfDesktop->BindToObject(pidl, NULL, riid, ppObject);
    }

exit_gracefully:

    if ( FAILED(hres) )
        *ppObject = NULL;

    DoRelease(psfDesktop);           
    DoILFree(pidl);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ GetColumnHandlerFromProperty
/ ----------------------------
/   Given a COLUMN structure allocate the property name appending the
/   CLSID of the handler if we have one.
/
/ In:
/   pColumn -> column value to decode
/   pProperty -> property value parse
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetColumnHandlerFromProperty(LPCOLUMN pColumn, LPWSTR pProperty)
{
    HRESULT hres;
    LPWSTR pPropertyTemp;
    LPWSTR pColumnHandlerCLSID;
    USES_CONVERSION;

    TraceEnter(TRACE_VIEW, "GetColumnHandlerFromProperty");
    Trace(TEXT("pProperty is: %s"), W2T(pProperty));

    if ( !pProperty )
        pProperty = pColumn->pProperty;
    
    // if we find a ',' then we must parse the GUID as it may be a CLSID for a column handler.

    pColumnHandlerCLSID = wcschr(pProperty, L',');

    if ( pColumnHandlerCLSID )
    {
        // attempt to extract the CLSID form the property name

        *pColumnHandlerCLSID++ = L'\0';           // terminate the property name

        if ( GetGUIDFromStringW(pColumnHandlerCLSID, &pColumn->clsidColumnHandler) )
        {
            TraceGUID("CLSID for handler is:", pColumn->clsidColumnHandler);
            pColumn->fHasColumnHandler = TRUE;
        }
        else
        {
            TraceMsg("**** Failed to parse CLSID from property name ****");
        }

        // we truncated the string, so lets re-alloc the buffer with the
        // new string value.

        if ( SUCCEEDED(LocalAllocStringW(&pPropertyTemp, pProperty)) )
        {
            LocalFreeStringW(&pColumn->pProperty);
            pColumn->pProperty = pPropertyTemp;
        }

        Trace(TEXT("Property name is now: %s"), W2T(pColumn->pProperty));
    }
    else
    {
        // now CLSID, so just allocate the property string if we need to.

        if ( pColumn->pProperty != pProperty )
        {
            if ( SUCCEEDED(LocalAllocStringW(&pPropertyTemp, pProperty)) )
            {
                LocalFreeStringW(&pColumn->pProperty);
                pColumn->pProperty = pPropertyTemp;
            }
        }
    }

    TraceLeaveResult(S_OK);
}


/*-----------------------------------------------------------------------------
/ GetPropertyFromColumn
/ ---------------------
/   Given a COLUMN structure allocate the property name appending the
/   CLSID of the handler if we have one.
/
/ In:
/   ppProperty -> receives a pointer to the property string
/   pColumn -> column value to decode
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetPropertyFromColumn(LPWSTR* ppProperty, LPCOLUMN pColumn)
{
    HRESULT hres;
    TCHAR szGUID[GUIDSTR_MAX+1];
    USES_CONVERSION;

    TraceEnter(TRACE_VIEW, "GetPropertyFromColumn");

    if ( !pColumn->fHasColumnHandler )
    {
        // just copy the property name

        hres = LocalAllocStringW(ppProperty, pColumn->pProperty);
        FailGracefully(hres, "Failed to allocate property");
    }
    else
    {
        // allocate a buffer and then place the property name and the
        // column handler CLSID.

        hres = LocalAllocStringLenW(ppProperty, lstrlenW(pColumn->pProperty)+GUIDSTR_MAX+1);  // nb: +2 for "," 
        FailGracefully(hres, "Failed to allocate buffer for property name");

        GetStringFromGUID(pColumn->clsidColumnHandler, szGUID, ARRAYSIZE(szGUID));

        StrCpyW(*ppProperty, pColumn->pProperty);
        StrCatW(*ppProperty, L",");
        StrCatW(*ppProperty, T2W(szGUID));
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ FreeColumn / FreeColumnValue
/ ----------------------------
/   A column consists of the header and filter information including the underlying
/   property value.  
/
/   A COLUMNVALUE is the typed information for the column which must be freed
/   based the iPropertyType value.
/
/ In:
/   pColumn -> LPCOLUMN structure to released
/   or
/   pColumnValue ->LPCOLUMNVALUE structure to be released
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

VOID FreeColumnValue(LPCOLUMNVALUE pColumnValue)
{
    TraceEnter(TRACE_VIEW, "FreeColumnValue");

    switch ( pColumnValue->iPropertyType )
    {
        case PROPERTY_ISUNDEFINED:
        case PROPERTY_ISBOOL:
        case PROPERTY_ISNUMBER:
            break;

        case PROPERTY_ISUNKNOWN:
        case PROPERTY_ISSTRING:
            LocalFreeString(&pColumnValue->pszText);
            break;

        default:
            Trace(TEXT("iPropertyValue is %d"), pColumnValue->iPropertyType);
            TraceAssert(FALSE);                       
            break;
    }

    pColumnValue->iPropertyType = PROPERTY_ISUNDEFINED;         // no value

    TraceLeave();
}

INT FreeColumnCB(LPVOID pItem, LPVOID pData)
{
    FreeColumn((LPCOLUMN)pItem);
    return 1;
}

VOID FreeColumn(LPCOLUMN pColumn)
{
    TraceEnter(TRACE_VIEW, "FreeQueryResult");

    if ( pColumn )
    {
        LocalFreeStringW(&pColumn->pProperty);
        LocalFreeString(&pColumn->pHeading);
        FreeColumnValue(&pColumn->filter);
        DoRelease(pColumn->pColumnHandler);
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ FreeQueryResult
/ ---------------
/   Given a QUERYRESULT structure free the elements within it
/
/ In:
/   pResult -> result blob to be released
/   cColumns = number of columns to be released
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

INT FreeQueryResultCB(LPVOID pItem, LPVOID pData)
{
    FreeQueryResult((LPQUERYRESULT)pItem, PtrToUlong(pData));
    return 1;
}

VOID FreeQueryResult(LPQUERYRESULT pResult, INT cColumns)
{
    INT i;

    TraceEnter(TRACE_VIEW, "FreeQueryResult");

    if ( pResult )
    {
        LocalFreeStringW(&pResult->pObjectClass);
        LocalFreeStringW(&pResult->pPath);

        for ( i = 0 ; i < cColumns ; i++ )
            FreeColumnValue(&pResult->aColumn[i]);
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ PropertyIsFromAttribute
/ -----------------------
/   Get the property is value from the specified attribute.
/
/ In:
/   pszAttributeName -> attribute name
/   pdds -> IDsDisplaySpecifier
/
/ Out:
/   DWORD dwType
/----------------------------------------------------------------------------*/
DWORD PropertyIsFromAttribute(LPCWSTR pszAttributeName, IDsDisplaySpecifier *pdds)
{   
    DWORD dwResult = PROPERTY_ISUNKNOWN;
    USES_CONVERSION;

    TraceEnter(TRACE_CORE, "PropertyIsFromAttribute");
    Trace(TEXT("Fetching attribute type for: %s"), W2CT(pszAttributeName));

    switch ( pdds->GetAttributeADsType(pszAttributeName) )
    {
        case ADSTYPE_DN_STRING:
        case ADSTYPE_CASE_EXACT_STRING:
        case ADSTYPE_CASE_IGNORE_STRING:
        case ADSTYPE_PRINTABLE_STRING:
        case ADSTYPE_NUMERIC_STRING:
            TraceMsg("Property is a string");
            dwResult = PROPERTY_ISSTRING;
            break;

        case ADSTYPE_BOOLEAN:
            TraceMsg("Property is a BOOL");
            dwResult = PROPERTY_ISBOOL;
            break;

        case ADSTYPE_INTEGER:
            TraceMsg("Property is a number");
            dwResult = PROPERTY_ISNUMBER;
            break;

        default:
            TraceMsg("Property is UNKNOWN");
            break;
    }

    TraceLeaveValue(dwResult);
}


/*-----------------------------------------------------------------------------
/ MatchPattern
/ ------------
/   Given two strings, one a string the other a pattern match the two
/   using standard wildcarding "*" == any number of characters, "?" means
/   single character skip
/
/ In:
/   pString = string to compare
/   pPattern = pattern to compare against
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
BOOL MatchPattern(LPTSTR pString, LPTSTR pPattern)
{                                                                              
    TCHAR c, p, l;

    for ( ;; ) 
    {
        switch (p = *pPattern++ ) 
        { 
            case 0:                                 // end of pattern
                return *pString ? FALSE : TRUE;     // if end of pString TRUE

            case TEXT('*'):
            {
                while ( *pString ) 
                {                                   // match zero or more char
                    if ( MatchPattern(pString++, pPattern) )
                        return TRUE;
                }

                return MatchPattern(pString, pPattern);
            }
                                                                               
            case TEXT('?'):
            {
                if (*pString++ == 0)                // match any one char
                    return FALSE;                   // not end of pString
 
                break;
            }

            default:
            {
                if ( *pString++ != p ) 
                    return FALSE;                   // not a match

                break;
            }
        }
    }
}



/*-----------------------------------------------------------------------------
/ EnumClassAttrbutes
/ ------------------
/   This is a wrapper around the attribute enum functions exposed in
/   the IDsDisplaySpecifier interface.
/
/   We read the attributes into a DPA, then sort them add in the 
/   extra columns exposed from this UI.
/
/ In:
/   pdds -> IDsDisplaySpecifier object
/   pszObjectClass = object class to enumerate
/   pcbEnum, lParam = enumeration callback
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

typedef struct
{
    LPWSTR pszName;
    LPWSTR pszDisplayName;
    DWORD  dwFlags;
} CLASSATTRIBUTE, * LPCLASSATTRIBUTE;

INT _FreeAttribute(LPCLASSATTRIBUTE pca)
{
    LocalFreeStringW(&pca->pszName);
    LocalFreeStringW(&pca->pszDisplayName);
    LocalFree(pca);
    return 1;
}

INT _FreeAttributeCB(LPVOID pv1, LPVOID pv2)
{
    return _FreeAttribute((LPCLASSATTRIBUTE)pv1);
}

HRESULT _AddAttribute(HDPA hdpa, LPCWSTR pszName, LPCWSTR pszDisplayName, DWORD dwFlags)
{
    HRESULT hres;
    LPCLASSATTRIBUTE pca = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_CORE, "_AddAttribute");
    Trace(TEXT("Adding %s (%s)"), W2CT(pszDisplayName), W2CT(pszName));

    pca = (LPCLASSATTRIBUTE)LocalAlloc(LPTR, SIZEOF(CLASSATTRIBUTE));
    if ( !pca )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate CLASSATTRIBUTE");

    // pca->pszName = NULL;
    // pca->pszDisplayName = NULL;
    pca->dwFlags = dwFlags;

    hres = LocalAllocStringW(&pca->pszName, pszName);
    FailGracefully(hres, "Failed to copy the name");

    hres = LocalAllocStringW(&pca->pszDisplayName, pszDisplayName);
    FailGracefully(hres, "Failed to copy the name");

    if ( -1 == DPA_AppendPtr(hdpa, pca) )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to append the record to the DPA");

    hres = S_OK;

exit_gracefully:

    if ( FAILED(hres) && pca )
        _FreeAttribute(pca);

    TraceLeaveResult(hres);
}

HRESULT _AddAttributeCB(LPARAM lParam, LPCWSTR pszName, LPCWSTR pszDisplayName, DWORD dwFlags)
{
    return _AddAttribute((HDPA)lParam, pszName, pszDisplayName, dwFlags);
}

INT _CompareAttributeCB(LPVOID pv1, LPVOID pv2, LPARAM lParam)
{
    LPCLASSATTRIBUTE pca1 = (LPCLASSATTRIBUTE)pv1;
    LPCLASSATTRIBUTE pca2 = (LPCLASSATTRIBUTE)pv2;
    return StrCmpIW(pca1->pszDisplayName, pca2->pszDisplayName);
} 

HRESULT EnumClassAttributes(IDsDisplaySpecifier *pdds, LPCWSTR pszObjectClass, LPDSENUMATTRIBUTES pcbEnum, LPARAM lParam)
{
    HRESULT hres;
    HDPA hdpaAttributes = NULL;
    WCHAR szBuffer[MAX_PATH];
    INT i;

    TraceEnter(TRACE_CORE, "EnumClassAttributes");

    hdpaAttributes = DPA_Create(16);
    if ( !hdpaAttributes )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate the DPA");

    //
    // add the stock properties for objectClass and ADsPath
    //

    LoadStringW(GLOBAL_HINSTANCE, IDS_OBJECTCLASS, szBuffer, ARRAYSIZE(szBuffer));
    hres = _AddAttribute(hdpaAttributes, c_szObjectClassCH, szBuffer, DSECAF_NOTLISTED);
    FailGracefully(hres, "Failed to add the ObjectClass default property");
    
    LoadStringW(GLOBAL_HINSTANCE, IDS_ADSPATH, szBuffer, ARRAYSIZE(szBuffer));
    hres = _AddAttribute(hdpaAttributes, c_szADsPathCH, szBuffer, DSECAF_NOTLISTED);
    FailGracefully(hres, "Failed to add the ObjectClass default property");

    //
    // now call the IDsDisplaySpecifier object to enumerate the properites correctly
    //

    TraceMsg("Calling IDsDisplaySpecifier::EnumClassAttributes");

    hres = pdds->EnumClassAttributes(pszObjectClass, _AddAttributeCB, (LPARAM)hdpaAttributes);
    FailGracefully(hres, "Failed to add the attributes");

    //
    // now sort and return them all to the caller via their callback funtion
    //

    Trace(TEXT("Sorting %d attributes, to return to the caller"), DPA_GetPtrCount(hdpaAttributes));
    DPA_Sort(hdpaAttributes, _CompareAttributeCB, NULL);

    for ( i = 0 ; i < DPA_GetPtrCount(hdpaAttributes) ; i++ )
    {
        LPCLASSATTRIBUTE pca = (LPCLASSATTRIBUTE)DPA_FastGetPtr(hdpaAttributes, i);
        TraceAssert(pca);
                                       
        hres = pcbEnum(lParam, pca->pszName, pca->pszDisplayName, pca->dwFlags);
        FailGracefully(hres, "Failed in cb to original caller");
    }

    hres = S_OK;

exit_gracefully:

    if ( hdpaAttributes )
        DPA_DestroyCallback(hdpaAttributes, _FreeAttributeCB, NULL);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ GetFriendlyAttributeName
/ ------------------------
/   Trim the column handler information if needed, and call the
/   friendly attribute name functions.
/
/ In:
/   pdds -> IDsDisplaySpecifier object
/   pszObjectClass, pszAttributeName => attribute info to look up
/   pszBuffer, chh => return buffer
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetFriendlyAttributeName(IDsDisplaySpecifier *pdds, LPCWSTR pszObjectClass, LPCWSTR pszAttributeName, LPWSTR pszBuffer, UINT cch)
{
    HRESULT hres = S_OK;
    WCHAR szAttributeName[MAX_PATH];
    USES_CONVERSION;

    TraceEnter(TRACE_CORE, "GetFriendlyAttributeName");

    //
    // trim off the attribute suffix if we have one (eg: the GUID for the column handler)
    //

    if ( wcschr(pszAttributeName, L',') )
    {
        TraceMsg("Has column handler information");

        StrCpyW(szAttributeName, pszAttributeName);
        LPWSTR pszSeperator = wcschr(szAttributeName, L',');
        *pszSeperator = L'\0';

        pszAttributeName = szAttributeName;
    }

    //
    // pick off some special cases before passing onto the COM object to process
    //

    Trace(TEXT("Looking up name for: %s"), W2CT(pszAttributeName));

    if ( !StrCmpIW(pszAttributeName, c_szADsPath) )
    {
        LoadStringW(GLOBAL_HINSTANCE, IDS_ADSPATH, pszBuffer, cch);
    }
    else if ( !StrCmpIW(pszAttributeName, c_szObjectClass) )
    {
        LoadStringW(GLOBAL_HINSTANCE, IDS_OBJECTCLASS, pszBuffer, cch);
    }
    else
    {
        hres = pdds->GetFriendlyAttributeName(pszObjectClass, pszAttributeName, pszBuffer, cch);
    }                

    TraceLeaveResult(hres);
}
