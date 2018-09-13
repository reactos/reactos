#include "pch.h"
#include "stddef.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Query thread bits
/----------------------------------------------------------------------------*/

//
// Page size used for paging the result sets (the LDAP server core is a sync process)
// therefore getting it to return smaller result blobs is better for us
//

#define PAGE_SIZE                   64
#define MAX_RESULT                  10000


//
// When the query is issued we always pull back at least ADsPath and objectClass
// as properites (these are required for the viewer to work).  Therefore these define
// the mapping of these values to the returned column set.
//

#define PROPERTYMAP_ADSPATH         0
#define PROPERTYMAP_OBJECTCLASS     1
#define PROPERTYMAP_USER            2

#define INDEX_TO_PROPERTY(i)        (i+PROPERTYMAP_USER)


//
// THREADDATA this is the state structure for the thread, it encapsulates
// the parameters and other junk required to the keep the thread alive.
//

typedef struct
{
    LPTHREADINITDATA ptid;    
    INT              cProperties;            // number of properties in aProperties
    LPWSTR*          aProperties;            // array of string pointers for "property names"
    INT              cColumns;               // number of columsn in view
    INT*             aColumnToPropertyMap;   // mapping from display column index to property name
} THREADDATA, * LPTHREADDATA;


//
// Helper macro to send messages for the fg view, including the reference
// count
// 

#define SEND_VIEW_MESSAGE(ptid, uMsg, lParam) \
        SendMessage(GetParent(ptid->hwndView), uMsg, (ptid)->dwReference, lParam)

//
// Function prototypes for the query thread engine.
//

HRESULT QueryThread_IssueQuery(LPTHREADDATA ptd);
HRESULT QueryThread_BuildPropertyList(LPTHREADDATA ptd);
VOID QueryThread_FreePropertyList(LPTHREADDATA ptd);


/*-----------------------------------------------------------------------------
/ Helper functions
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ QueryThread_GetFilter
/ ----------------------
/   Construct the LDAP filter we are going to use for this query.
/
/ In:
/   ppQuery -> receives the full filter
/   pBaseFilter -> filter string to use as base
/   fShowHidden = show hidden objects?
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

VOID _GetFilter(LPWSTR pFilter, UINT* pcchFilterLen, LPWSTR pBaseFilter, BOOL fShowHidden)
{
    HRESULT hres;

    TraceEnter(TRACE_QUERYTHREAD, "_GetFilter");

    if ( pFilter )
        *pFilter = TEXT('\0');

    PutStringElementW(pFilter, pcchFilterLen, L"(&");

    if ( !fShowHidden )
        PutStringElementW(pFilter, pcchFilterLen, c_szShowInAdvancedViewOnly);

    PutStringElementW(pFilter, pcchFilterLen, pBaseFilter);
    PutStringElementW(pFilter, pcchFilterLen, L")");

    TraceLeave();
}

HRESULT QueryThread_GetFilter(LPWSTR* ppFilter, LPWSTR pBaseFilter, BOOL fShowHidden)
{
    HRESULT hres;
    UINT cchFilterLen = 0;

    TraceEnter(TRACE_QUERYTHREAD, "QueryThread_GetFilter");

     _GetFilter(NULL, &cchFilterLen, pBaseFilter, fShowHidden);

    hres = LocalAllocStringLenW(ppFilter, cchFilterLen);
    FailGracefully(hres, "Failed to allocate buffer for query string");
    
    _GetFilter(*ppFilter, NULL, pBaseFilter, fShowHidden);
    
    hres = S_OK;

exit_gracefully:
   
    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ Background query thread, this does the work of issuing the query and then
/ populating the view.
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ QueryThread
/ -----------
/   Thread function sits spinning its wheels waiting for a query to be
/   received from the outside world.  The main result viewer communicates
/   with this code by ThreadSendMessage.
/
/ In:
/   pThreadParams -> structure that defines out thread information
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
DWORD WINAPI QueryThread(LPVOID pThreadParams)
{
    HRESULT hresCoInit;
    MSG msg;
    LPTHREADINITDATA pThreadInitData = (LPTHREADINITDATA)pThreadParams;
    THREADDATA td;

    ZeroMemory(&td, SIZEOF(td));

    td.ptid = pThreadInitData;
    //td.cProperties = 0;
    //td.aProperties = NULL;
    //td.cColumns = 0;
    //td.aColumnToPropertyMap = NULL;
    
    hresCoInit = CoInitialize(NULL);
    FailGracefully(hresCoInit, "Failed to CoInitialize");

    GetActiveWindow();                                      // ensure we have amessage queue

    QueryThread_IssueQuery(&td);

    while ( GetMessage(&msg, NULL, 0, 0) > 0 )
    {
        switch ( msg.message )
        {
            case RVTM_STOPQUERY:
                TraceMsg("RVTM_STOPQUERY received - ignoring");
                break;

            case RVTM_REFRESH:
            {
                td.ptid->dwReference = (DWORD)msg.wParam;
                QueryThread_IssueQuery(&td);
                break;
            }
            
            case RVTM_SETCOLUMNTABLE:
            {
                if ( td.ptid->hdsaColumns )
                    DSA_DestroyCallback(td.ptid->hdsaColumns, FreeColumnCB, NULL);

                td.ptid->dwReference = (DWORD)msg.wParam;
                td.ptid->hdsaColumns = (HDSA)msg.lParam;        

                QueryThread_FreePropertyList(&td);
                QueryThread_IssueQuery(&td);
                break;
            }
                                          
            default:
                break;
        }
    }

exit_gracefully:

    QueryThread_FreePropertyList(&td);
    QueryThread_FreeThreadInitData(&td.ptid);

    if ( SUCCEEDED(hresCoInit) )
        CoUninitialize();

    InterlockedIncrement(&GLOBAL_REFCOUNT);
    ExitThread(0);
    return 0;               // BOGUS: not never called
}


/*-----------------------------------------------------------------------------
/ QueryThread_FreeThreadInitData
/ ------------------------------
/   Release the THREADINITDATA structure that we are given when the thread
/   is created.
/
/ In:
/   pptid ->-> thread init data structure to be released 
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID QueryThread_FreeThreadInitData(LPTHREADINITDATA* pptid)
{
    LPTHREADINITDATA ptid = *pptid;

    TraceEnter(TRACE_QUERYTHREAD, "QueryThread_FreeThreadInitData");

    if ( ptid )
    {
        LocalFreeStringW(&ptid->pQuery);
        LocalFreeStringW(&ptid->pScope);

        if ( ptid->hdsaColumns )
            DSA_DestroyCallback(ptid->hdsaColumns, FreeColumnCB, NULL);

        LocalFreeStringW(&ptid->pServer);
        LocalFreeStringW(&ptid->pUserName);
        LocalFreeStringW(&ptid->pPassword);

        LocalFree((HLOCAL)ptid);
        *pptid = NULL;
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ QueryThread_CheckForStopQuery
/ -----------------------------
/   Peek the message queue looking for a stop query message, if we
/   can find one then we must bail out.
/
/ In:
/   ptd -> thread data structure
/
/ Out:
/   fStopQuery
/----------------------------------------------------------------------------*/
BOOL QueryThread_CheckForStopQuery(LPTHREADDATA ptd)
{
    BOOL fStopQuery = FALSE;
    MSG msg;

    TraceEnter(TRACE_QUERYTHREAD, "QueryThread_CheckForStopQuery");

    while ( PeekMessage(&msg, NULL, RVTM_FIRST, RVTM_LAST, PM_REMOVE) )
    {
        TraceMsg("Found a RVTM_ message in queue, stopping query");
        fStopQuery = TRUE;
    }

    TraceLeaveValue(fStopQuery);
}


/*-----------------------------------------------------------------------------
/ QueryThread_IssueQuery
/ ----------------------
/   Issue a query using the IDirectorySearch interface, this is a more performant
/   to the wire interface that issues the query directly.   The code binds to 
/   the scope object (the specified path) and then issues the LDAP query
/   pumping the results into the viewer as required.
/
/ In:
/   ptd -> thread information structurre
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT QueryThread_IssueQuery(LPTHREADDATA ptd)
{
    HRESULT hres;
    DWORD dwres;
    LPTHREADINITDATA ptid = ptd->ptid;
    LPWSTR pQuery = NULL;
    INT cItems, iColumn;
    INT cMaxResult = MAX_RESULT;
    BOOL fStopQuery = FALSE;
    IDirectorySearch* pDsSearch = NULL;
    LPWSTR pszTempPath = NULL;
    IDsDisplaySpecifier *pdds = NULL;
    ADS_SEARCH_HANDLE hSearch = NULL;
    ADS_SEARCHPREF_INFO prefInfo[3];
    ADS_SEARCH_COLUMN column;
    HDPA hdpaResults = NULL;
    LPQUERYRESULT pResult = NULL;
    WCHAR szBuffer[2048];               // MAX_URL_LENGHT
    INT resid;
    LPWSTR pColumnData = NULL;
    HKEY hkPolicy = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_QUERYTHREAD, "QueryThread_IssueQuery");    

    // The foreground gave us a query so we are going to go and issue
    // it now, having done this we will then be able to stream the 
    // result blobs back to the caller. 

    hres = QueryThread_GetFilter(&pQuery, ptid->pQuery, ptid->fShowHidden);
    FailGracefully(hres, "Failed to build LDAP query from scope, parameters + filter");

    Trace(TEXT("Query is: %s"), W2T(pQuery));
    Trace(TEXT("Scope is: %s"), W2T(ptid->pScope));
    
    // Get the IDsDisplaySpecifier interface:

    hres = CoCreateInstance(CLSID_DsDisplaySpecifier, NULL, CLSCTX_INPROC_SERVER, IID_IDsDisplaySpecifier, (void **)&pdds);
    FailGracefully(hres, "Failed to get the IDsDisplaySpecifier object");

    hres = pdds->SetServer(ptid->pServer, ptid->pUserName, ptid->pPassword, DSSSF_DSAVAILABLE);
    FailGracefully(hres, "Failed to server information");

    // initialize the query engine, specifying the scope, and the search parameters

    hres = QueryThread_BuildPropertyList(ptd);
    FailGracefully(hres, "Failed to build property array to query for");

    hres = ADsOpenObject(ptid->pScope, ptid->pUserName, ptid->pPassword, ADS_SECURE_AUTHENTICATION,
                            IID_IDirectorySearch, (LPVOID*)&pDsSearch);

    FailGracefully(hres, "Failed to get the IDirectorySearch interface for the given scope");

    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;     // sub-tree search
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;

    prefInfo[1].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;     // async
    prefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
    prefInfo[1].vValue.Boolean = TRUE;

    prefInfo[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;         // paged results
    prefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[2].vValue.Integer = PAGE_SIZE;

    hres = pDsSearch->SetSearchPreference(prefInfo, ARRAYSIZE(prefInfo));
    FailGracefully(hres, "Failed to set search preferences");

    hres = pDsSearch->ExecuteSearch(pQuery, ptd->aProperties, ptd->cProperties, &hSearch);
    FailGracefully(hres, "Failed in ExecuteSearch");

    // pick up the policy value which defines the max results we are going to use

    dwres = RegOpenKey(HKEY_CURRENT_USER, DS_POLICY, &hkPolicy);
    if ( ERROR_SUCCESS == dwres )
    {
        DWORD dwType, cbSize;

        dwres = RegQueryValueEx(hkPolicy, TEXT("QueryLimit"), NULL, &dwType, NULL, &cbSize);
        if ( (ERROR_SUCCESS == dwres) && (dwType == REG_DWORD) && (cbSize == SIZEOF(cMaxResult)) )
        {
            RegQueryValueEx(hkPolicy, TEXT("QueryLimit"), NULL, NULL, (LPBYTE)&cMaxResult, &cbSize);
        }

        RegCloseKey(hkPolicy);
    }
    

    // issue the query, pumping the results to the foreground UI which
    // will inturn populate the the list view

    Trace(TEXT("Result limit set to %d"), cMaxResult);

    for ( cItems = 0 ; cItems < cMaxResult; cItems++ )
    {
        hres = pDsSearch->GetNextRow(hSearch);

        fStopQuery = QueryThread_CheckForStopQuery(ptd);
        
        Trace(TEXT("fStopQuery %d, hr %08x"), fStopQuery, hres);

        if ( fStopQuery || (hres == S_ADS_NOMORE_ROWS) )
            break;

        FailGracefully(hres, "Failed in GetNextRow");

        // We have a result, lets ensure that we have posted the blob
        // we are building before we start constructing a new one.  We
        // send pages of items to the fg thread for it to add to the
        // result view, if the blob returns FALSE then we must tidy the
        // DPA before continuing.
        
        if ( ((cItems % 10) == 0) && hdpaResults )          // 10 is a nice block size
        {
            TraceMsg("Posting results blob to fg thread");
            
            if ( !SEND_VIEW_MESSAGE(ptid, DSQVM_ADDRESULTS, (LPARAM)hdpaResults) )
                DPA_DestroyCallback(hdpaResults, FreeQueryResultCB, (LPVOID)ptd->cColumns);

            hdpaResults = NULL;
        }

        if ( !hdpaResults )
        {
            hdpaResults = DPA_Create(PAGE_SIZE);
            TraceAssert(hdpaResults);

            if ( !hdpaResults )
                ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate result DPA");
        }

        // Add the result we have to the result blob, the first
        // two things we need are the class and the ADsPath of the
        // object, then loop over the properties to generate the
        // column data

        pResult = (LPQUERYRESULT)LocalAlloc(LPTR, SIZEOF(QUERYRESULT)+(SIZEOF(COLUMNVALUE)*(ptd->cColumns-1)));
        TraceAssert(pResult);

        if ( pResult )
        {
            // Get the ADsPath and ObjectClass of the object, these must remain UNICODE as
            // they are used later for binding to the object.  All other display information
            // can be fixed up later.

            pResult->iImage = -1;

            // get the ADsPath.  If the provider is GC: then replace with LDAP: so that
            // when we interact with this object we are kept happy.

            hres = pDsSearch->GetColumn(hSearch, c_szADsPath, &column);
            FailGracefully(hres, "Failed to get the ADsPath column");

            hres = StringFromSearchColumn(&column, &pResult->pPath);
            pDsSearch->FreeColumn(&column);

            Trace(TEXT("Object path: %s"), W2T(pResult->pPath));

            if ( SUCCEEDED(hres) &&
                    ((pResult->pPath[0]== L'G') && (pResult->pPath[1] == L'C')) )
            {
                TraceMsg("Replacing provider with LDAP:");

                hres = LocalAllocStringLenW(&pszTempPath, lstrlenW(pResult->pPath)+2);
                if ( SUCCEEDED(hres) )
                {
                    StrCpyW(pszTempPath, c_szLDAP);
                    StrCatW(pszTempPath, pResult->pPath+3);           // skip GC:

                    LocalFreeStringW(&pResult->pPath);
                    pResult->pPath = pszTempPath;
                }

                Trace(TEXT("New path is: %s"), W2T(pResult->pPath));
            }

            FailGracefully(hres, "Failed to get ADsPath from column");

            // get the objectClass

            hres = pDsSearch->GetColumn(hSearch, c_szObjectClass, &column);
            FailGracefully(hres, "Failed to get the objectClass column");

            hres = ObjectClassFromSearchColumn(&column, &pResult->pObjectClass);
            pDsSearch->FreeColumn(&column);
            FailGracefully(hres, "Failed to get object class from column");

            // Now ensure that we have the icon cache, and then walk the list of columns
            // getting the text that represents those.

            if ( SUCCEEDED(pdds->GetIconLocation(pResult->pObjectClass, DSGIF_GETDEFAULTICON, szBuffer, ARRAYSIZE(szBuffer), &resid)) )
            {
                pResult->iImage = Shell_GetCachedImageIndex(W2T(szBuffer), resid, 0x0);
                Trace(TEXT("Image index from shell is: %d"), pResult->iImage);
            }

            for ( iColumn = 0 ; iColumn < ptd->cColumns ; iColumn++ )
            {
                LPWSTR pProperty = ptd->aProperties[ptd->aColumnToPropertyMap[iColumn]];    
                TraceAssert(pProperty);

                pResult->aColumn[iColumn].iPropertyType = PROPERTY_ISUNDEFINED;     // empty column

                hres = pDsSearch->GetColumn(hSearch, pProperty, &column);
                if ( (hres != E_ADS_COLUMN_NOT_SET) && FAILED(hres) )
                {
                    Trace(TEXT("Failed to get column %d with code %08x"), iColumn, hres);
                    hres = E_ADS_COLUMN_NOT_SET;
                }

                if ( hres != E_ADS_COLUMN_NOT_SET )
                {
                    LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(ptid->hdsaColumns, iColumn);
                    TraceAssert(pColumn);

                    switch ( pColumn->iPropertyType )
                    {
                        case PROPERTY_ISUNKNOWN:
                        case PROPERTY_ISSTRING:
                        {                            
                            // we are treating the property as a string, therefore convert the search
                            // column to a string value and convert as required.

                            pResult->aColumn[iColumn].iPropertyType = PROPERTY_ISSTRING;

                            if ( pColumn->fHasColumnHandler )
                            {
                                // we have the CLSID for a column handler, therefore lets CoCreate it
                                // and pass it to the ::GetText method.

                                if ( !pColumn->pColumnHandler )
                                {
                                    TraceGUID("Attempting to create IDsQueryColumnHandler from GUID: ", pColumn->clsidColumnHandler);

                                    hres = CoCreateInstance(pColumn->clsidColumnHandler, NULL, CLSCTX_INPROC_SERVER, 
                                                                IID_IDsQueryColumnHandler, (LPVOID*)&pColumn->pColumnHandler);
                                    if ( SUCCEEDED(hres) )
                                        hres = pColumn->pColumnHandler->Initialize(0x0, ptid->pServer, ptid->pUserName, ptid->pPassword);

                                    if ( FAILED(hres) )
                                    {
                                        TraceMsg("Failed to CoCreate the column handler, marking the column as not having one");
                                        pColumn->fHasColumnHandler = FALSE;
                                        pColumn->pColumnHandler = NULL;
                                    }
                                }                                        

                                // if pColumnHandler != NULL, then call its ::GetText method, this is the string we should
                                // then place into the column.

                                if ( pColumn->pColumnHandler )
                                {
                                    pColumn->pColumnHandler->GetText(&column, szBuffer, ARRAYSIZE(szBuffer));
                                    LocalAllocStringW2T(&pResult->aColumn[iColumn].pszText, szBuffer);
                                }
                            }
                            else
                            {
                                // if we were able to convert the column value to a string,
                                // then lets pass it to the column handler (if there is one
                                // to get the display string), or just copy this into the column
                                // structure (thunking accordingly).
                        
                                if ( SUCCEEDED(StringFromSearchColumn(&column, &pColumnData)) )
                                {
                                    LocalAllocStringW2T(&pResult->aColumn[iColumn].pszText, pColumnData);
                                    LocalFreeStringW(&pColumnData);
                                }
                            }

                            break;
                        }
                        
                        case PROPERTY_ISBOOL:                                   // treat the BOOL as a number
                        case PROPERTY_ISNUMBER:
                        {
                            // its a number, therefore lets pick up the number value from the
                            // result and store that.

                            pResult->aColumn[iColumn].iPropertyType = PROPERTY_ISNUMBER;
                            pResult->aColumn[iColumn].iValue = column.pADsValues->Integer;
                            break;
                        }
                    }

                    pDsSearch->FreeColumn(&column);
                }
            }        

            if ( -1 == DPA_AppendPtr(hdpaResults, pResult) )
            {
                FreeQueryResult(pResult, ptd->cColumns);
                LocalFree((HLOCAL)pResult);
            }

            pResult = NULL;
        }
    }

    hres = S_OK;

exit_gracefully:

    Trace(TEXT("cItems %d, (hdpaResults %08x (%d))"), cItems, hdpaResults, hdpaResults ? DPA_GetPtrCount(hdpaResults):0);

    if ( hdpaResults )
    {
        // As we send bunches of results to the fg thread check to see if we have a 
        // DPA with any pending items in it, if we do then lets ensure we post that
        // off, if that succedes (the msg returns TRUE) then we are done, otherwise
        // hdpaResults needs to be free'd

        Trace(TEXT("Posting remaining results to fg thread (%d)"), DPA_GetPtrCount(hdpaResults));

        if ( SEND_VIEW_MESSAGE(ptid, DSQVM_ADDRESULTS, (LPARAM)hdpaResults) )
            hdpaResults = NULL;
    }

    if ( !fStopQuery )
    {
        SEND_VIEW_MESSAGE(ptid, DSQVM_FINISHED, (cItems == MAX_RESULT));
    }

    if ( pResult )
    {
        FreeQueryResult(pResult, ptd->cColumns);
        LocalFree((HLOCAL)pResult);
    }

    if ( hSearch && pDsSearch )
    {
        pDsSearch->CloseSearchHandle(hSearch);
    }

    LocalFreeStringW(&pQuery);

    DoRelease(pDsSearch);
    DoRelease(pdds);

    QueryThread_FreePropertyList(ptd);               // its void when we issue a new query

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ QueryThread_BuildPropertyList
/ -----------------------------
/   Given the column DSA construct the property maps and the property
/   list we are going to query for.  Internaly we always query for
/   ADsPath and objectClass, so walk the columns and work out
/   how many extra properties above this we have, then build an
/   array of property names containing the unique properties.
/
/   We also construct an index table that maps from a column index
/   to a property name.
/
/ In:
/   ptd -> thread information structurre
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT QueryThread_BuildPropertyList(LPTHREADDATA ptd)
{
    HRESULT hres;
    LPTHREADINITDATA ptid = ptd->ptid;
    INT i, j;
    USES_CONVERSION;

    TraceEnter(TRACE_QUERYTHREAD, "QueryThread_BuildPropertyList");

    // Walk the list of columns and compute the properties that are unique to this
    // query and generate a table for them.  First count the property table
    // based on the columns DSA

    ptd->cProperties = PROPERTYMAP_USER;
    ptd->aProperties = NULL;
    ptd->cColumns = DSA_GetItemCount(ptid->hdsaColumns);
    ptd->aColumnToPropertyMap = NULL;

    for ( i = 0 ; i < DSA_GetItemCount(ptid->hdsaColumns); i++ )
    {
        LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(ptid->hdsaColumns, i);
        TraceAssert(pColumn);

        if ( StrCmpW(pColumn->pProperty, c_szADsPath) &&
                 StrCmpW(pColumn->pProperty, c_szObjectClass) )
        {
           ptd->cProperties++;
        }
    }
       
    Trace(TEXT("cProperties %d"), ptd->cProperties);
        
    ptd->aProperties = (LPWSTR*)LocalAlloc(LPTR, SIZEOF(LPWSTR)*ptd->cProperties);
    ptd->aColumnToPropertyMap = (INT*)LocalAlloc(LPTR, SIZEOF(INT)*ptd->cColumns);

    if ( !ptd->aProperties || !ptd->aColumnToPropertyMap )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate property array / display array");
    
    ptd->aProperties[PROPERTYMAP_ADSPATH] = c_szADsPath;
    ptd->aProperties[PROPERTYMAP_OBJECTCLASS] = c_szObjectClass;

    for ( j = PROPERTYMAP_USER, i = 0 ; i < ptd->cColumns; i++ )
    {
        LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(ptid->hdsaColumns, i);
        TraceAssert(pColumn);

        if ( !StrCmpW(pColumn->pProperty, c_szADsPath) )
        {
           ptd->aColumnToPropertyMap[i] = PROPERTYMAP_ADSPATH;
        }
        else if ( !StrCmpW(pColumn->pProperty, c_szObjectClass) )
        {
           ptd->aColumnToPropertyMap[i] = PROPERTYMAP_OBJECTCLASS;
        }
        else
        {
            ptd->aProperties[j] = pColumn->pProperty;
            ptd->aColumnToPropertyMap[i] = j++;
        }

        Trace(TEXT("Property: %s"), ptd->aProperties[ptd->aColumnToPropertyMap[i]]);
    }

    hres = S_OK;

exit_gracefully:

    if ( FAILED(hres) )
        QueryThread_FreePropertyList(ptd);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ QueryThread_FreePropertyList
/ ----------------------------
/   Release a previously allocated property list assocaited with the
/   given thread.
/
/ In:
/   ptd -> thread information structurre
/
/ Out:
/   VOID
/----------------------------------------------------------------------------*/
VOID QueryThread_FreePropertyList(LPTHREADDATA ptd)
{
    TraceEnter(TRACE_QUERYTHREAD, "QueryThread_FreePropertyList");

    if ( ptd->aProperties )
        LocalFree((HLOCAL)ptd->aProperties);
    if ( ptd->aColumnToPropertyMap )
        LocalFree((HLOCAL)ptd->aColumnToPropertyMap);

    ptd->cProperties = 0;    
    ptd->aProperties = NULL;
    ptd->cColumns = 0;
    ptd->aColumnToPropertyMap = NULL;
    
    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CQueryThreadCH
/ --------------
/   Query thread column handler, this is a generic one used to convert
/   properties based on the CLSID we are instantiated with.
/----------------------------------------------------------------------------*/

class CQueryThreadCH : public IDsQueryColumnHandler, CUnknown
{
    private:
        CLSID _clsid;
        IADsPathname *_padp;
        IDsDisplaySpecifier *_pdds;

        DWORD _dwFlags;
        LPWSTR _pszServer;
        LPWSTR _pszUserName;
        LPWSTR _pszPassword;

    public:
        CQueryThreadCH(REFCLSID rCLSID);
        ~CQueryThreadCH();

        // *** IUnknown ***
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // *** IDsQueryColumnHandler ***
        STDMETHOD(Initialize)(THIS_ DWORD dwFlags, LPCWSTR pszServer, LPCWSTR pszUserName, LPCWSTR pszPassword);
        STDMETHOD(GetText)(ADS_SEARCH_COLUMN* psc, LPWSTR pszBuffer, INT cchBuffer);
};

//
// constructor
//

CQueryThreadCH::CQueryThreadCH(REFCLSID rCLSID) :
    _padp(NULL),
    _pdds(NULL),
    _clsid(rCLSID),
    _dwFlags(0),
    _pszServer(NULL),
    _pszUserName(NULL),
    _pszPassword(NULL)
{
    TraceEnter(TRACE_QUERYTHREAD, "CQueryThreadCH::CQueryThreadCH");
    TraceGUID("CLSID of property: ", rCLSID);
    TraceLeave();
}

CQueryThreadCH::~CQueryThreadCH()
{
    TraceEnter(TRACE_QUERYTHREAD, "CQueryThreadCH::~CQueryThreadCH");

    DoRelease(_padp);       // free the name cracker
    DoRelease(_pdds);

    LocalFreeStringW(&_pszServer);
    LocalFreeStringW(&_pszUserName);
    LocalFreeStringW(&_pszPassword);

    TraceLeave();
}

//
// Handler IUnknown interface
//

#undef  CLASS_NAME
#define CLASS_NAME CQueryThreadCH
#include "unknown.inc"

STDMETHODIMP CQueryThreadCH::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IDsQueryColumnHandler, (IDsQueryColumnHandler*)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}

//
// Handle creating an instance of IDsFolderProperties for talking to WAB
//

STDAPI CQueryThreadCH_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CQueryThreadCH *pqtch = new CQueryThreadCH(*poi->pclsid);
    if ( !pqtch )
        return E_OUTOFMEMORY;

    HRESULT hres = pqtch->QueryInterface(IID_IUnknown, (void **)ppunk);
    pqtch->Release();
    return hres;
}


/*-----------------------------------------------------------------------------
/ IDsQueryColumnHandler
/----------------------------------------------------------------------------*/

STDMETHODIMP CQueryThreadCH::Initialize(THIS_ DWORD dwFlags, LPCWSTR pszServer, LPCWSTR pszUserName, LPCWSTR pszPassword)
{
    TraceEnter(TRACE_QUERYTHREAD, "CQueryThread::Initialize");

    LocalFreeStringW(&_pszServer);
    LocalFreeStringW(&_pszUserName);
    LocalFreeStringW(&_pszPassword);

    // copy new parameters away

    _dwFlags = dwFlags;

    HRESULT hres = LocalAllocStringW(&_pszServer, pszServer);    
    if ( SUCCEEDED(hres) )
        hres = LocalAllocStringW(&_pszUserName, pszUserName);
        if ( SUCCEEDED(hres) )
        hres = LocalAllocStringW(&_pszPassword, pszPassword);

    DoRelease(_pdds)                                // discard previous IDisplaySpecifier object

    TraceLeaveResult(hres);
}

STDMETHODIMP CQueryThreadCH::GetText(ADS_SEARCH_COLUMN* psc, LPWSTR pszBuffer, INT cchBuffer)
{
    HRESULT hres;
    LPWSTR pValue = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_QUERYTHREAD, "CQueryThreadCH::GetText");

    if ( !psc || !pszBuffer )
        ExitGracefully(hres, E_UNEXPECTED, "Bad parameters passed to handler");

    pszBuffer[0] = L'\0'; 

    if ( IsEqualCLSID(_clsid, CLSID_PublishedAtCH) || IsEqualCLSID(_clsid, CLSID_MachineOwnerCH) )
    {
        BOOL fPrefix = IsEqualCLSID(_clsid, CLSID_PublishedAtCH);
        LPCWSTR pszPath = psc->pADsValues[0].DNString;
        TraceAssert(pszPath != NULL);

        // convert the ADsPath into its canonical form which is easier for the user
        // to understand, CoCreate IADsPathname now instead of each time we call
        // PrettyifyADsPathname.

        if ( !_padp )
        {
            hres = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (LPVOID*)&_padp);
            FailGracefully(hres, "Failed to get IADsPathname interface");
        }

        if ( FAILED(GetDisplayNameFromADsPath(pszPath, pszBuffer, cchBuffer, _padp, fPrefix)) )
        {
            TraceMsg("Failed to get display name from path");
            StrCpyNW(pszBuffer, pszPath, cchBuffer);
        }
                                                                    
        hres = S_OK;
    }
    else if ( IsEqualCLSID(_clsid, CLSID_ObjectClassCH) )
    {        
        // get a string from the search column, and then look up the friendly name of the
        // class from its display specifier

        hres = ObjectClassFromSearchColumn(psc, &pValue);
        FailGracefully(hres, "Failed to get object class from psc");

        if ( !_pdds )
        {
            DWORD dwFlags = 0;

            hres = CoCreateInstance(CLSID_DsDisplaySpecifier, NULL, CLSCTX_INPROC_SERVER, IID_IDsDisplaySpecifier, (void **)&_pdds);
            FailGracefully(hres, "Failed to get IDsDisplaySpecifier interface");

            hres = _pdds->SetServer(_pszServer, _pszUserName, _pszPassword, DSSSF_DSAVAILABLE);
            FailGracefully(hres, "Failed when setting server for display specifier object");                
        }

        _pdds->GetFriendlyClassName(pValue, pszBuffer, cchBuffer);
    }
    else if ( IsEqualCLSID(_clsid, CLSID_MachineOwnerCH) )
    {
        // convert the DN of the user object into a string that we can display
    }
    else if ( IsEqualCLSID(_clsid, CLSID_MachineRoleCH) )
    {
        // convert the userAccountControl value into something we can display for the user

        if ( psc->dwADsType == ADSTYPE_INTEGER )
        {
            INT iType = psc->pADsValues->Integer;           // pick out the type

            if ( (iType >= 4096) && (iType <= 8191) )
            {
                TraceMsg("Returning WKS/SRV string");
                LoadStringW(GLOBAL_HINSTANCE, IDS_WKSORSERVER, pszBuffer, cchBuffer);
            }
            else if ( iType >= 8192 )
            {
                TraceMsg("Returning DC string");
                LoadStringW(GLOBAL_HINSTANCE, IDS_DC, pszBuffer, cchBuffer);
            }
            else
            {
                Trace(TEXT("Unknown type %x"), iType);
            }
        }
    }
    else
    {
        ExitGracefully(hres, E_UNEXPECTED, "m_clsid specifies column type not supported");
    }

    hres = S_OK;

exit_gracefully:

    LocalFreeStringW(&pValue);

    TraceLeaveResult(hres);
}
