#ifndef __query_h
#define __query_h

//
// Resource IDs used for the menus
//

// File menu

#define DSQH_FILE_CONTEXT_FIRST     (CQID_MINHANDLERMENUID + 0x0000)
#define DSQH_FILE_CONTEXT_LAST      (CQID_MINHANDLERMENUID + 0x0fff)

#define DSQH_FILE_OPENCONTAINER     (CQID_MINHANDLERMENUID + 0x1001)
#define DSQH_FILE_PROPERTIES        (CQID_MINHANDLERMENUID + 0x1002)
#define DSQH_FILE_CREATESHORTCUT    (CQID_MINHANDLERMENUID + 0x1003)
#define DSQH_FILE_SAVEQUERY         (CQID_MINHANDLERMENUID + 0x1004)

// Edit menu

#define DSQH_EDIT_SELECTALL         (CQID_MINHANDLERMENUID + 0x1100)
#define DSQH_EDIT_INVERTSELECTION   (CQID_MINHANDLERMENUID + 0x1101)

// View menu

#define DSQH_VIEW_FILTER            (CQID_MINHANDLERMENUID + 0x1200)
#define DSQH_VIEW_LARGEICONS        (CQID_MINHANDLERMENUID + 0x1201)
#define DSQH_VIEW_SMALLICONS        (CQID_MINHANDLERMENUID + 0x1202)
#define DSQH_VIEW_LIST              (CQID_MINHANDLERMENUID + 0x1203)
#define DSQH_VIEW_DETAILS           (CQID_MINHANDLERMENUID + 0x1204)
#define DSQH_VIEW_ARRANGEICONS      (CQID_MINHANDLERMENUID + 0x1205)
#define DSQH_VIEW_REFRESH           (CQID_MINHANDLERMENUID + 0x1206)
#define DSQH_VIEW_PICKCOLUMNS       (CQID_MINHANDLERMENUID + 0x1207)

#define DSQH_VIEW_ARRANGEFIRST      (CQID_MINHANDLERMENUID + 0x1280)
#define DSQH_VIEW_ARRANGELAST       (CQID_MINHANDLERMENUID + 0x12FF)

// Help menu

#define DSQH_HELP_CONTENTS          (CQID_MINHANDLERMENUID + 0x1300)
#define DSQH_HELP_WHATISTHIS        (CQID_MINHANDLERMENUID + 0x1301)
#define DSQH_HELP_ABOUT             (CQID_MINHANDLERMENUID + 0x1302)

// Extra background verbs

#define DSQH_BG_SELECT              (CQID_MINHANDLERMENUID + 0x1400)

// Filter verbs

#define DSQH_CLEARFILTER            (CQID_MINHANDLERMENUID + 0x1500)
#define DSQH_CLEARALLFILTERS        (CQID_MINHANDLERMENUID + 0x1501)


//
// CDsQueryHandler global information
//

//
// The bg thread communicates with the view using the following messages
//

#define DSQVM_ADDRESULTS            (WM_USER+0)         // lParam = HDPA containing results
#define DSQVM_FINISHED              (WM_USER+1)         // lParam = fMaxResult


//
// Column DSA contains these items
//

#define PROPERTY_ISUNDEFINED        0x00000000          // property is undefined
#define PROPERTY_ISUNKNOWN          0x00000001          // only operator is exacly
#define PROPERTY_ISSTRING           0x00000002          // starts with, ends with, is exactly, not equal
#define PROPERTY_ISNUMBER           0x00000003          // greater, less, equal, not equal
#define PROPERTY_ISBOOL             0x00000004          // equal, not equal

#define DEFAULT_WIDTH               20
#define DEFAULT_WIDTH_DESCRIPTION   40

typedef struct
{
    INT iPropertyType;                  // type of property
    union
    {
        LPTSTR pszText;                 // iPropertyType == PROPERTY_ISSTRING
        INT iValue;                     // iPropertyType == PROPERTY_ISNUMBER
    };
} COLUMNVALUE, * LPCOLUMNVALUE;

typedef struct
{
    BOOL fHasColumnHandler:1;           // column handler specified?
    LPWSTR pProperty;                   // property name
    LPTSTR pHeading;                    // column heading
    INT cx;                             // width of column (% of view)
    INT fmt;                            // formatting information
    INT iPropertyType;                  // type of property
    UINT idOperator;                    // currently selected operator
    COLUMNVALUE filter;                 // the filter applied
    CLSID clsidColumnHandler;           // CLSID and IDsQueryColumnHandler objects
    IDsQueryColumnHandler* pColumnHandler;
} COLUMN, * LPCOLUMN;

typedef struct
{
    LPWSTR pObjectClass;                // object class (UNICODE)
    LPWSTR pPath;                       // directory object (UNICODE)
    INT iImage;                         // image / == -1 if none
    COLUMNVALUE aColumn[1];             // column data
} QUERYRESULT, * LPQUERYRESULT;

STDAPI CDsQuery_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

//
// The outside world commmunicates with the thread using messages (sent via PostThreadMessage).
//

#define RVTM_FIRST                  (WM_USER)
#define RVTM_LAST                   (WM_USER+32)

#define RVTM_STOPQUERY              (WM_USER)           // wParam = 0, lParam =0
#define RVTM_REFRESH                (WM_USER+1)         // wParam = 0, lParam = 0
#define RVTM_SETCOLUMNTABLE         (WM_USER+2)         // wParam = 0, lParam = HDSA columns


//
// THREADINITDATA strucutre, this is passed when the query thread is being
// created, it contains all the parameters required to issue the query,
// and populate the view.
//

typedef struct
{
    DWORD  dwReference;             // reference value for query
    LPWSTR pQuery;                  // base filter to be applied
    LPWSTR pScope;                  // scope to search
    LPWSTR pServer;                 // server to target
    LPWSTR pUserName;               // user name and password to authenticate with
    LPWSTR pPassword;
    BOOL   fShowHidden:1;           // show hidden objects in results
    HWND   hwndView;                // handle of our result view to be filled
    HDSA   hdsaColumns;             // column table
} THREADINITDATA, * LPTHREADINITDATA;


//
// Query thread, this is passed the THREADINITDATA structure
//

DWORD WINAPI QueryThread(LPVOID pThreadParams);
VOID QueryThread_FreeThreadInitData(LPTHREADINITDATA* ppTID);

STDAPI CQueryThreadCH_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);


//
// Scope logic
//

#define OBJECT_NAME_FROM_SCOPE(pDsScope)\
            ((LPWSTR)ByteOffset(pDsScope, pDsScope->dwOffsetADsPath))

#define OBJECT_CLASS_FROM_SCOPE(pDsScope)\
            ((LPWSTR)(!pDsScope->dwOffsetClass ? NULL : ByteOffset(pDsScope, pDsScope->dwOffsetClass)))

typedef struct
{
    CQSCOPE cq;                         // all scopes must have this as a header
    INT     iIndent;                    // indent
    DWORD   dwOffsetADsPath;            // offset to scope
    DWORD   dwOffsetClass;              // offset to class of scope / = 0 if none 
    WCHAR   szStrings[1];               // string data (all UNICODE)
} DSQUERYSCOPE, * LPDSQUERYSCOPE;

typedef struct
{
    HWND hwndFrame;                     // frame window to display message boxes on
    LPWSTR pDefaultScope;               // scope for this object
    LPWSTR pServer;                     // server to target
    LPWSTR pUserName;                   // user name and password to authenticate with
    LPWSTR pPassword;
} SCOPETHREADDATA, * LPSCOPETHREADDATA;


#define GC_OBJECTCLASS L"domainDNS" // objectClass used for GC objects

HRESULT GetGlobalCatalogPath(LPCWSTR pszServer, LPWSTR pszPath, INT cchBuffer);
HRESULT AddScope(HWND hwndFrame, INT index, INT iIndent, LPWSTR pPath, LPWSTR pObjectClass, BOOL fSelect);
HRESULT AllocScope(LPCQSCOPE* ppScope, INT iIndent, LPWSTR pPath, LPWSTR pObjectClass);
DWORD WINAPI AddScopesThread(LPVOID pThreadParams);


//
// helpers for all to use
//

VOID MergeMenu(HMENU hMenu, HMENU hMenuToInsert, INT iIndex);
HRESULT BindToPath(LPWSTR pszPath, REFIID riid, LPVOID* ppObject);
 
INT FreeQueryResultCB(LPVOID pItem, LPVOID pData);
VOID FreeQueryResult(LPQUERYRESULT pResult, INT cColumns);
VOID FreeColumnValue(LPCOLUMNVALUE pColumnValue);
INT FreeColumnCB(LPVOID pItem, LPVOID pData);
VOID FreeColumn(LPCOLUMN pColumn);
DWORD PropertyIsFromAttribute(LPCWSTR pszAttributeName, IDsDisplaySpecifier *pdds);
BOOL MatchPattern(LPTSTR pString, LPTSTR pPattern);
HRESULT EnumClassAttributes(IDsDisplaySpecifier *pdds, LPCWSTR pszObjectClass, LPDSENUMATTRIBUTES pcbEnum, LPARAM lParam);
HRESULT GetFriendlyAttributeName(IDsDisplaySpecifier *pdds, LPCWSTR pszObjectClass, LPCWSTR pszAttributeName, LPWSTR pszBuffer, UINT cch);

HRESULT GetColumnHandlerFromProperty(LPCOLUMN pColumn, LPWSTR pProperty);
HRESULT GetPropertyFromColumn(LPWSTR* ppProperty, LPCOLUMN pColumn);
STDAPI ADsPathToIdList(LPITEMIDLIST* ppidl, LPWSTR pPath, LPWSTR pObjectClass, BOOL fRelative);


#endif
