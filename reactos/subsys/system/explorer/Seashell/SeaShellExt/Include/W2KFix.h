#ifndef __W2KFIX_H__
#define __W2KFIX_H__

#if (_WIN32_IE < 0x0501)
typedef struct _SHELLDETAILS
{
    int     fmt;            // LVCFMT_* value (header only)
    int     cxChar;         // Number of "average" characters (header only)
    STRRET  str;            // String information
} SHELLDETAILS, *LPSHELLDETAILS;
// {93F2F68C-1D1B-11d3-A30E-00C04F79ABD1}
DEFINE_GUID(IID_IShellFolder2,  0x93f2f68c, 0x1d1b, 0x11d3, 0xa3, 0xe, 0x0, 0xc0, 0x4f, 0x79, 0xab, 0xd1);

#define SHCDF_UPDATEITEM        0x00000001      // this flag is a hint that the file has changed since the last call to GetItemData

typedef struct {
    GUID fmtid;
    DWORD pid;
} SHCOLUMNID, *LPSHCOLUMNID;
typedef const SHCOLUMNID* LPCSHCOLUMNID;

typedef struct {
    ULONG   dwFlags ;            // combination of SHCDF_ flags.
    DWORD   dwFileAttributes ;   // file attributes.
    ULONG   dwReserved ;         // reserved for future use.
    WCHAR*  pwszExt ;            // address of file name extension
    WCHAR   wszFile[MAX_PATH] ;  // Absolute path of file.
} SHCOLUMNDATA, *LPSHCOLUMNDATA ;
typedef const SHCOLUMNDATA* LPCSHCOLUMNDATA ;

#define MAX_COLUMN_NAME_LEN 80
#define MAX_COLUMN_DESC_LEN 128
typedef struct {
    ULONG   dwFlags ;             // initialization flags
    ULONG   dwReserved ;          // reserved for future use.
    WCHAR   wszFolder[MAX_PATH];  // fully qualified folder path (or empty if multiple folders)
} SHCOLUMNINIT, *LPSHCOLUMNINIT;
typedef const SHCOLUMNINIT* LPCSHCOLUMNINIT;

typedef struct {
    SHCOLUMNID  scid;                           // OUT the unique identifier of this column
    VARTYPE     vt;                             // OUT the native type of the data returned
    DWORD       fmt;                            // OUT this listview format (LVCFMT_LEFT, usually)
    UINT        cChars;                         // OUT the default width of the column, in characters
    DWORD       csFlags;                        // OUT SHCOLSTATE flags
    WCHAR wszTitle[MAX_COLUMN_NAME_LEN];        // OUT the title of the column
    WCHAR wszDescription[MAX_COLUMN_DESC_LEN];  // OUT full description of this column
} SHCOLUMNINFO, *LPSHCOLUMNINFO ;
typedef const SHCOLUMNINFO* LPCSHCOLUMNINFO ;

//-------------------------------------------------------------------------
//
// IEnumExtraSearch interface
//
//  IShellFolder2::EnumSearches member returns an IEnumExtraSearch object.
//
//-------------------------------------------------------------------------

typedef struct tagEXTRASEARCH
{
    GUID    guidSearch;
    WCHAR   wszFriendlyName[80];
    WCHAR   wszUrl[2084];
}EXTRASEARCH, *LPEXTRASEARCH;

typedef struct IEnumExtraSearch *LPENUMEXTRASEARCH;

#undef  INTERFACE
#define INTERFACE       IEnumExtraSearch

DECLARE_INTERFACE_(IEnumExtraSearch, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IEnumExtraSearch methods ***
    STDMETHOD(Next)  (THIS_ ULONG celt, EXTRASEARCH *rgelt, ULONG *pceltFetched) PURE;
    STDMETHOD(Skip)  (THIS_ ULONG celt) PURE;
    STDMETHOD(Reset) (THIS) PURE;
    STDMETHOD(Clone) (THIS_ IEnumExtraSearch **ppenum) PURE;
};

//--------------------------------------------------------------------------
// IShellFolder2
//
// [member functions]
//
// IShellFolder2::GetDefaultSearchGUID(LPGUID pGuid)
//   Returns the guid of the search that is to be invoked when user clicks
//   on the search toolbar button
//
// IShellFolder2::EnumSearches(IEnumExtraSearch **ppenum)
//   gives an enumerator of the searches to be added to the search menu
//--------------------------------------------------------------------------

// IShellFolder2::GetDefaultColumnState values
typedef enum {
    SHCOLSTATE_TYPE_STR     = 0x00000001,
    SHCOLSTATE_TYPE_INT     = 0x00000002,
    SHCOLSTATE_TYPE_DATE    = 0x00000003,
    SHCOLSTATE_TYPEMASK     = 0x0000000F,
    SHCOLSTATE_ONBYDEFAULT  = 0x00000010,   // should on by default in details view
    SHCOLSTATE_SLOW         = 0x00000020,   // will be slow to compute, do on a background thread
    SHCOLSTATE_EXTENDED     = 0x00000040,   // provided by a handler, not the folder
    SHCOLSTATE_SECONDARYUI  = 0x00000080,   // not displayed in context menu, but listed in the "More..." dialog
    SHCOLSTATE_HIDDEN       = 0x00000100,   // not displayed in the UI
} SHCOLSTATE;

#undef  INTERFACE
#define INTERFACE       IShellFolder2

DECLARE_INTERFACE_(IShellFolder2, IShellFolder)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IShellFolder methods ***
    STDMETHOD(ParseDisplayName)(THIS_ HWND hwnd, LPBC pbc, LPOLESTR pszDisplayName,
                                ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes) PURE;
    STDMETHOD(EnumObjects)(THIS_ HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList) PURE;
    STDMETHOD(BindToObject)(THIS_ LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv) PURE;
    STDMETHOD(BindToStorage)(THIS_ LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv) PURE;
    STDMETHOD(CompareIDs)(THIS_ LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) PURE;
    STDMETHOD(CreateViewObject)(THIS_ HWND hwnd, REFIID riid, void **ppv) PURE;
    STDMETHOD(GetAttributesOf)(THIS_ UINT cidl, LPCITEMIDLIST * apidl, ULONG * drgfInOut) PURE;
    STDMETHOD(GetUIObjectOf)(THIS_ HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                             REFIID riid, UINT * prgfInOut, void **ppv) PURE;
    STDMETHOD(GetDisplayNameOf)(THIS_ LPCITEMIDLIST pidl, DWORD uFlags, STRRET *psr) PURE;
    STDMETHOD(SetNameOf)(THIS_ HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName,
                         DWORD uFlags, LPITEMIDLIST *ppidl) PURE;

    // *** IShellFolder2 methods ***
    STDMETHOD(GetDefaultSearchGUID)(THIS_ GUID *pguid) PURE;
    STDMETHOD(EnumSearches)(THIS_ IEnumExtraSearch **ppenum) PURE;
    STDMETHOD(GetDefaultColumn) (THIS_ DWORD dwRes, ULONG *pSort, ULONG *pDisplay) PURE;
    STDMETHOD(GetDefaultColumnState)(THIS_ UINT iColumn, DWORD *pcsFlags) PURE;
    STDMETHOD(GetDetailsEx)(THIS_ LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv) PURE;
    STDMETHOD(GetDetailsOf)(THIS_ LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd) PURE;
    STDMETHOD(MapColumnToSCID)(THIS_ UINT iColumn, SHCOLUMNID *pscid) PURE;
};

#undef  INTERFACE
#define INTERFACE   IShellDetails

DECLARE_INTERFACE_(IShellDetails, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellDetails methods ***
    STDMETHOD(GetDetailsOf)(THIS_ LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails) PURE;
    STDMETHOD(ColumnClick)(THIS_ UINT iColumn) PURE;
};


#endif


#endif //__W2KFIX_H__