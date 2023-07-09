#ifndef __dsquery_h
#define __dsquery_h
#ifndef __dsqueryp_h            ;internal
#define __dsqueryp_h            ;internal

//
// query handler ID for dsquery.
//

DEFINE_GUID(CLSID_DsQuery, 0x8a23e65e, 0x31c2, 0x11d0, 0x89, 0x1c, 0x0, 0xa0, 0x24, 0xab, 0x2d, 0xbb);

//
// standard forms shipped in dsquery.dll
//

DEFINE_GUID(CLSID_DsFindObjects, 0x83ee3fe1, 0x57d9, 0x11d0, 0xb9, 0x32, 0x0, 0xa0, 0x24, 0xab, 0x2d, 0xbb);
DEFINE_GUID(CLSID_DsFindPeople, 0x83ee3fe2, 0x57d9, 0x11d0, 0xb9, 0x32, 0x0, 0xa0, 0x24, 0xab, 0x2d, 0xbb);
DEFINE_GUID(CLSID_DsFindPrinter, 0xb577f070, 0x7ee2, 0x11d0, 0x91, 0x3f, 0x0, 0xaa, 0x0, 0xc1, 0x6e, 0x65);
DEFINE_GUID(CLSID_DsFindComputer, 0x16006700, 0x87ad, 0x11d0, 0x91, 0x40, 0x0, 0xaa, 0x0, 0xc1, 0x6e, 0x65);
DEFINE_GUID(CLSID_DsFindVolume, 0xc1b3cbf1, 0x886a, 0x11d0, 0x91, 0x40, 0x0, 0xaa, 0x0, 0xc1, 0x6e, 0x65);
DEFINE_GUID(CLSID_DsFindContainer, 0xc1b3cbf2, 0x886a, 0x11d0, 0x91, 0x40, 0x0, 0xaa, 0x0, 0xc1, 0x6e, 0x65);
DEFINE_GUID(CLSID_DsFindAdvanced, 0x83ee3fe3, 0x57d9, 0x11d0, 0xb9, 0x32, 0x0, 0xa0, 0x24, 0xab, 0x2d, 0xbb);

//
// admin forms
//

DEFINE_GUID(CLSID_DsFindDomainController, 0x538c7b7e, 0xd25e, 0x11d0, 0x97, 0x42, 0x0, 0xa0, 0xc9, 0x6, 0xaf, 0x45);
DEFINE_GUID(CLSID_DsFindFrsMembers, 0x94ce4b18, 0xb3d3, 0x11d1, 0xb9, 0xb4, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0);

;begin_internal
#define IID_IDsQueryHandler CLSID_DsQuery
DEFINE_GUID(IID_IDsQueryColumnHandler, 0xc072999e, 0xfa49, 0x11d1, 0xa0, 0xaf, 0x00, 0xc0, 0x4f, 0xa3, 0x1a, 0x86);
;end_internal

#ifndef GUID_DEFS_ONLY      ;both


//
// DSQUERYINITPARAMS
// -----------------
//  This structured is used when creating a new query view.
//

#define DSQPF_NOSAVE                 0x00000001 // = 1 => remove save verb
#define DSQPF_SAVELOCATION           0x00000002 // = 1 => pSaveLocation contains directory to save queries into
#define DSQPF_SHOWHIDDENOBJECTS      0x00000004 // = 1 => show objects marked as "hidden" in results
#define DSQPF_ENABLEADMINFEATURES    0x00000008 // = 1 => show admin verbs, property pages etc
#define DSQPF_ENABLEADVANCEDFEATURES 0x00000010 // = 1 => set the advanced flag for the property pages
#define DSQPF_HASCREDENTIALS         0x00000020 // = 1 => pServer, pUserName & pPassword are valid
;begin_internal
#define DSQPF_RETURNALLRESULTS       0x80000000 // = 1 => return all results on OK, not just selection
;end_internal

typedef struct
{
    DWORD  cbStruct;
    DWORD  dwFlags;
    LPWSTR pDefaultScope;           // -> Active Directory path to use as scope / == NULL for none
    LPWSTR pDefaultSaveLocation;    // -> Directory to save queries into / == NULL default location
    LPWSTR pUserName;               // -> user name to authenticate with
    LPWSTR pPassword;               // -> password for authentication
    LPWSTR pServer;                 // -> server to use for obtaining trusts etc
} DSQUERYINITPARAMS, * LPDSQUERYINITPARAMS;


//
// DSQUERYPARAMS
// -------------
//  The DS query handle takes a packed structure which contains the
//  columns and query to be issued.
//

#define CFSTR_DSQUERYPARAMS         TEXT("DsQueryParameters")

#define DSCOLUMNPROP_ADSPATH        ((LONG)(-1))
#define DSCOLUMNPROP_OBJECTCLASS    ((LONG)(-2))

typedef struct
{
    DWORD dwFlags;                  // flags for this column
    INT   fmt;                      // list view form information
    INT   cx;                       // default column width
    INT   idsName;                  // resource ID for the column dispaly name
    LONG  offsetProperty;           // offset to BSTR defining column ADs property name
    DWORD dwReserved;               // reserved field
} DSCOLUMN, * LPDSCOLUMN;

typedef struct
{
    DWORD     cbStruct;
    DWORD     dwFlags;
    HINSTANCE hInstance;            // instance handle used for string extraction
    LONG      offsetQuery;          // offset to LDAP filter string
    LONG      iColumns;             // column count
    DWORD     dwReserved;           // reserved field for this query
    DSCOLUMN  aColumns[1];          // array of column descriptions
} DSQUERYPARAMS, * LPDSQUERYPARAMS;


//
// CF_DSQUERYSCOPE
// ---------------
//  A clipboard format the puts a string version of the scope into a
//  storage medium via GlobalAlloc.
//
#define CFSTR_DSQUERYSCOPE         TEXT("DsQueryScope")


//
// DSQPM_GETCLASSLIST
// ------------------
//  This page message is sent to the form pages to retrieve the list of classes
//  that the pages are going to query from.  This is used by the feild selector
//  and the property well to build its list of display classes.
//

typedef struct
{
    DWORD   cbStruct;
    LONG    cClasses;               // number of classes in array
    DWORD   offsetClass[1];         // offset to the class names (UNICODE)
} DSQUERYCLASSLIST, * LPDSQUERYCLASSLIST;

;begin_internal
#define DSQPM_GCL_FORPROPERTYWELL   0x8000 // == 1 => for property well
;end_internal

#define DSQPM_GETCLASSLIST          (CQPM_HANDLERSPECIFIC+0) // wParam == flags, lParam = LPLPDSQUERYCLASSLIST


//
// DSQPM_HELPTOPICS
// ----------------
//  This page message is sent to the form pages to allow them to handle the
//  "Help Topics" verb.
//

#define DSQPM_HELPTOPICS            (CQPM_HANDLERSPECIFIC+1) // wParam = 0, lParam = hWnd parent


;begin_internal

//-----------------------------------------------------------------------------
// Internal form helper functions
//-----------------------------------------------------------------------------

// filter types

#define FILTER_FIRST                0x0100
#define FILTER_LAST                 0x0200

#define FILTER_CONTAINS             0x0100
#define FILTER_NOTCONTAINS          0x0101
#define FILTER_STARTSWITH           0x0102
#define FILTER_ENDSWITH             0x0103
#define FILTER_IS                   0x0104
#define FILTER_ISNOT                0x0105
#define FILTER_GREATEREQUAL         0x0106
#define FILTER_LESSEQUAL            0x0107
#define FILTER_DEFINED              0x0108
#define FILTER_UNDEFINED            0x0109
#define FILTER_ISTRUE               0x010A
#define FILTER_ISFALSE              0x010B

// structures

typedef struct
{
    INT    fmt;
    INT    cx;
    UINT   idsName;
    LONG   iPropertyIndex;
    LPWSTR pPropertyName;
} COLUMNINFO, * LPCOLUMNINFO;

typedef struct
{
    UINT   nIDDlgItem;
    LPWSTR pPropertyName;
    INT    iFilter;
} PAGECTRL, * LPPAGECTRL;

// form APIs - private

STDAPI ClassListAlloc(LPDSQUERYCLASSLIST* ppDsQueryClassList, LPWSTR* aClassNames, INT cClassNames);
STDAPI QueryParamsAlloc(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery, HINSTANCE hInstance, LONG iColumns, LPCOLUMNINFO aColumnInfo);
STDAPI QueryParamsAddQueryString(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery);
STDAPI GetFilterString(LPWSTR pFilter, UINT* pLen, INT iFilter, LPWSTR pProperty, LPWSTR pValue);
STDAPI GetQueryString(LPWSTR* ppQuery, LPWSTR pPrefixQuery, HWND hDlg, LPPAGECTRL aCtrls, INT iCtrls);
STDAPI GetPatternString(LPTSTR pFilter, UINT* pLen, INT iFilter, LPTSTR pValue);
STDAPI_(VOID) ResetPageControls(HWND hDlg, LPPAGECTRL aCtrl, INT iCtrls);
STDAPI_(VOID) EnablePageControls(HWND hDlg, LPPAGECTRL aCtrl, INT iCtrls, BOOL fEnable);
STDAPI PersistQuery(IPersistQuery* pPersistQuery, BOOL fRead, LPCTSTR pSection, HWND hDlg, LPPAGECTRL aCtrl, INT iCtrls);
STDAPI SetDlgItemFromProperty(IPropertyBag* ppb, LPCWSTR pszProperty, HWND hwnd, INT id, LPCWSTR pszDefault);


//---------------------------------------------------------------------------//
//
// IDsQueryColumnHandler
// =====================
//  This interface is used by the query result view to allow the form to replace
//  the contents of the form columns.
//
//  If the property name is property,{CLSID}, we CoCreateInstance the GUID
//  asking for the IDsQueryColumnHandler which we then call for each
//  string property we are going to place into the result view.
//
//  The handler only gets called when the results are being unpacked from
//  the server, subsequent filtering, sort etc of the view doesn't
//  invole this handler.
//
//  However perf should be considered when implementing this object.
//
//---------------------------------------------------------------------------//

#undef  INTERFACE
#define INTERFACE   IDsQueryColumnHandler

DECLARE_INTERFACE_(IDsQueryColumnHandler, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // **** IDsQueryColumnHandler ****
    STDMETHOD(Initialize)(THIS_ DWORD dwFlags, LPCWSTR pszServer, LPCWSTR pszUserName, LPCWSTR pszPassword) PURE;
    STDMETHOD(GetText)(THIS_ ADS_SEARCH_COLUMN* psc, LPWSTR pszBuffer, INT cchBuffer) PURE;
};

//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
//
// IDsQuery
// ========
//
//---------------------------------------------------------------------------//

#undef  INTERFACE
#define INTERFACE   IDsQueryHandler

//
// flags passed to IDsQueryHandler::UpdateView
//

#define DSQRVF_REQUERY          0x00000000  
#define DSQRVF_ITEMSDELETED     0x00000001  // pdon -> array of items to remove from the view
#define DSQRVF_OPMASK           0x00000fff 

DECLARE_INTERFACE_(IDsQueryHandler, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // **** IDsQuery ****
    STDMETHOD(UpdateView)(THIS_ DWORD dwType, LPDSOBJECTNAMES pdon) PURE;
};

//---------------------------------------------------------------------------//


;end_internal


;begin_both
#endif  // GUID_DEFS_ONLY
#endif
;end_both

