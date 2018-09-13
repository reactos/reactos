
#ifndef _INC_DOCFIND
#define _INC_DOCFIND

// for the OLEDB query stuff
#define OLEDBVER 0x0250 // enable ICommandTree interface
#include <oledberr.h>
#include <oledb.h>
#include <cmdtree.h>
#include <oledbdep.h>
#include <query.h>
#include <stgprop.h>
#include <ntquery.h>

#include <idhidden.h>

// Forward references
typedef struct _DOCFIND DOCFIND;
typedef DOCFIND * LPDOCFIND;
typedef struct _dfHeader DFHEADER;
typedef struct _DFFolderListItem DFFolderListItem;

// Define some options that are used between filter and search code
#define DFOO_INCLUDESUBDIRS 0x0001  // Include sub directories.
#define DFOO_REGULAR        0x0004  //
#define DFOO_CASESEN        0x0008  //
#define DFOO_USEOLEDB       0x0010  // Search path(s) support OLEDB query.
#define DFOO_SAVERESULTS    0x0100  // Save results in save file
#define DFOO_SHOWALLOBJECTS 0x1000  // Show all files
#define DFOO_SHOWEXTENSIONS 0x2000  // Should we show extensions.

// BUGBUG:: Keep from reworking old find code...
#define DFOO_OLDFINDCODE    0x8000  // Are we using the old find code?

// Some error happended on the get next file...


#define GNF_ERROR -1
#define GNF_DONE        0
#define GNF_MATCH       1
#define GNF_NOMATCH     2
#define GNF_ASYNC       3

// Define a FACILITY That we can check errors for...
#define FACILITY_SEARCHCOMMAND      99

// reg location where we store bad paths that ci should not have indexed
#define CI_SPECIAL_FOLDERS TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Search\\SpecialFolders")

//===========================================================================
// IDFEnum: Interface definition
//===========================================================================
// Declare Shell Docfind Enumeration interface.

#undef  INTERFACE
#define INTERFACE       IDFEnum

DECLARE_INTERFACE_(IDFEnum, IUnknown)     //IDFENUM
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IDFEnum methods (sortof standerd iterator methos ***
    // Which are extended to handle async query case.
    STDMETHOD(Next)(THIS_ LPITEMIDLIST *ppidl,
                   int *pcObjectSearched, int *pcFoldersSearched, BOOL *pfContinue,
                   int *pState, HWND hwnd) PURE;
    STDMETHOD (Skip)(THIS_ int celt) PURE;
    STDMETHOD (Reset)(THIS) PURE;
    STDMETHOD (StopSearch)(THIS) PURE;
    STDMETHOD_(BOOL,FQueryIsAsync)(THIS) PURE;
    STDMETHOD (GetAsyncCount)(THIS_ DBCOUNTITEM *pdwTotalAsync, int *pnPercentComplete, BOOL *pfQueryDone) PURE;
    STDMETHOD (GetItemIDList)(THIS_ UINT iItem, LPITEMIDLIST *ppidl) PURE;
    STDMETHOD (GetExtendedDetailsOf)(THIS_ LPCITEMIDLIST pidl, UINT iCol, LPSHELLDETAILS pdi) PURE;
    STDMETHOD (GetExtendedDetailsULong)(THIS_ LPCITEMIDLIST pidl, UINT iCol, ULONG *pul) PURE;
    STDMETHOD (GetItemID)(THIS_ UINT iItem, DWORD *puWorkID) PURE;
    STDMETHOD (SortOnColumn)(THIS_ UINT iCol, BOOL fAscending) PURE;
};

// We overloaded Async case when we are in mixed (some async some sync mode)
#define DF_QUERYISMIXED     ((BOOL)42)


//===========================================================================
// IDocFindBrowser: Interface definition
//===========================================================================
DECLARE_INTERFACE_(IDocFindBrowser, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IDocFindBrowser methods ***

    STDMETHOD(DeferProcessUpdateDir)() PURE;
    STDMETHOD(EndDeferProcessUpdateDir)() PURE;

#if 0 // Not currently used
    // *** IDocFindBrowser methods ***
    STDMETHOD(DFBSave)(THIS) PURE;
#endif
};


//===========================================================================
// IDocFindFileFilter: Interface definition
//===========================================================================
// Declare Shell Docfind Filter interface.

typedef interface IDocFindFolder IDocFindFolder;

#undef  INTERFACE
#define INTERFACE       IDocFindFileFilter
DECLARE_INTERFACE_(IDocFindFileFilter, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IDocFindFileFilter methods ***
    STDMETHOD(GetIconsAndMenu)(THIS_ HWND hwndDlg, HICON *phiconSmall, HICON
            *phiconLarge, HMENU *phmenu) PURE;
    STDMETHOD(GetStatusMessageIndex)(THIS_ UINT uContext, UINT *puMsgIndex) PURE;
    STDMETHOD(GetFolderMergeMenuIndex)(THIS_ UINT *puBGMainMergeMenu, UINT *puBGPopupMergeMenu) PURE;

    STDMETHOD(FFilterChanged)(THIS) PURE;
    STDMETHOD(GenerateTitle)(THIS_ LPTSTR *ppszTile, BOOL fFileName) PURE;
    STDMETHOD(PrepareToEnumObjects)(THIS_ DWORD * pdwFlags) PURE;
    STDMETHOD(ClearSearchCriteria)(THIS) PURE;
    STDMETHOD(EnumObjects)(THIS_ IShellFolder *psf, LPITEMIDLIST pidlStart,
            DWORD grfFlags, int iColSort, LPTSTR pszProgressText, IRowsetWatchNotify *prwn, 
            IDFEnum **ppdfenum) PURE;
    STDMETHOD(GetDetailsOf)(THIS_ HDPA hdpaPidf, LPCITEMIDLIST pidl, UINT *piColumn, LPSHELLDETAILS pdi) PURE;
    STDMETHOD(FDoesItemMatchFilter)(THIS_ LPTSTR pszFolder, WIN32_FIND_DATA *pfinddata,
            IShellFolder *psf, LPITEMIDLIST pidl) PURE;
    STDMETHOD(SaveCriteria)(THIS_ IStream * pstm, WORD fCharType) PURE;   // BUGBUG:: Should convert to stream
    STDMETHOD(RestoreCriteria)(THIS_ IStream * pstm, int cCriteria, WORD fCharType) PURE;
    STDMETHOD(DeclareFSNotifyInterest)(THIS_ HWND hwndDlg, UINT uMsg) PURE;
    STDMETHOD(GetColSaveStream)(THIS_ WPARAM wParam, LPSTREAM *ppstm) PURE;
    STDMETHOD(GenerateQueryRestrictions)(THIS_ LPWSTR *ppwszQuery, DWORD *pdwGQRFlags) PURE;
    STDMETHOD(ReleaseQuery)(THIS) PURE;
    STDMETHOD(UpdateField)(THIS_ BSTR szField, VARIANT vValue) PURE;
    STDMETHOD(ResetFieldsToDefaults)(THIS) PURE;
    STDMETHOD(GetItemContextMenu)(THIS_ HWND hwndOwner, IDocFindFolder* pdfFolder, IContextMenu** ppcm) PURE;
    STDMETHOD(GetDefaultSearchGUID)(THIS_ IShellFolder2 *psf2, LPGUID lpGuid) PURE;
    STDMETHOD(EnumSearches)(THIS_ IShellFolder2 *psf2, LPENUMEXTRASEARCH *ppenum) PURE;
    STDMETHOD(GetSearchFolderClassId)(THIS_ LPGUID lpGuid) PURE;
    STDMETHOD(GetNextConstraint)(THIS_ VARIANT_BOOL fReset, BSTR *pName, VARIANT *pValue, VARIANT_BOOL *pfFound) PURE;
    STDMETHOD(GetQueryLanguageDialect)(THIS_ ULONG * pulDialect);
    STDMETHOD(GetWarningFlags)(THIS_ DWORD *pdwWarningFlags);
};


// Define the flags that GenerateQueryRestrictions may return
typedef enum {
    GQR_MAKES_USE_OF_CI =   0x0001, // some constraint makes resonable use of Content index
    GQR_REQUIRES_CI     =   0x0002, // The query requires the CI to work
    GQR_BYBASS_CI       =   0x0004, // The query should bybass CI.
} GQR_FLAGS;

//  Docfind UI warning bits.
#define DFW_DEFAULT                0x00000000  
#define DFW_IGNORE_CISCOPEMISMATCH 0x00000001 // CI query requested search scopes beyond indexed scopes
#define DFW_IGNORE_INDEXNOTCOMPLETE 0x00000002 // ci not done indexing

// try to share code for converting fields between docfind2 and netfind.c
typedef struct _cdffuf
{
    LPCWSTR     pwszField;
    VARTYPE     vt;
    int         cdffufe;
} CDFFUF;

int CDFFilter_UpdateFieldChangeType(BSTR szField, VARIANT vValue, const CDFFUF *pcdffuf, VARIANT *pvConvertedValue, 
                                    LPTSTR *ppszValue);


// need this in docfind2 and docfindx.cpp so put into header.
enum
{
    IDFCOL_NAME = 0,
    IDFCOL_PATH,
    IDFCOL_RANK,
    IDFCOL_SIZE,
    IDFCOL_TYPE,
    IDFCOL_MODIFIED,
} ;

#ifdef IF_ADD_MORE_COLS
//
#define IDFCOL_FIRST_QUERY  IDFCOL_RANK     // The columns after this point are only valid for querys...
#endif

#define ESFITEM_ICONOVERLAYSET    0x00000001
typedef struct _ESFItem
{
    DWORD       dwMask;
    DWORD       dwState;    // State of the item;
    int         iIcon;
    // Allocate the pidl at end as variable length
    ITEMIDLIST idl;      // the pidl

} ESFItem;

typedef struct _ESFSaveStateItem
{
    DWORD       dwState;    // State of the item;
    DWORD       dwItemID;   // Only used for Async support...
} ESFSaveStateItem;

typedef ESFItem CDFItem; // TODO: kill this eventually

// Currently the state above is LVIS_SELECTED and LVIS_FOCUSED (low two bits)
// Add a bit to use in the processing of updatedir
#define CDFITEM_STATE_MAYBEDELETE    0x80000000L
#define CDFITEM_STATE_MASK           (LVIS_SELECTED)    // Which states we will hav LV have us track

int DFSortIDToICol(int uCmd);

// Helper function to get the width of a char, in the font in the given hwnd.
UINT GetControlCharWidth(HWND hwnd);


// Definition of the data items that we cache per directory.
typedef struct _DFFolderListItem    // DFFLI
{
    IShellFolder *      psf;        // Cache of MRU items
    int                 cbPidl;     // The length of the pidl...
    int                 iImage;     // Image to show item with...
    BOOL                fUpdateDir:1; // Was this node touched by an updatedir...
    BOOL                fDeleteDir:1; // Was this directory removed from the list?
    // Allocate the pidl at end as variable length
    ITEMIDLIST idl;      // the pidl
} DFFolderListItem;


typedef struct _HIDDENDOCFINDDATA //  hdfd
{
    WORD    cb;
    IDLHID  id;
    WORD    iFolder;    // index to the folder DPA
    WORD    wFlags;     // 
    UINT    uRow;       // Which row in the CI;
    DWORD   dwItemID;   // Only used for Async support...
    ULONG   ulRank;     // The rank returned by CI...
} HIDDENDOCFINDDATA;

#define DFDF_NONE               0x0000
#define DFDF_EXTRADATA          0x0001

typedef UNALIGNED HIDDENDOCFINDDATA * PHIDDENDOCFINDDATA;
typedef const UNALIGNED HIDDENDOCFINDDATA * PCHIDDENDOCFINDDATA;


#define DF_IFOLDER(pidl)  (((PCHIDDENDOCFINDDATA) ILFindHiddenID((pidl), IDLHID_DOCFINDDATA))->iFolder)


STDAPI_(LPITEMIDLIST) DocFind_AppendDocFindData(LPITEMIDLIST pidl, int iFolder, WORD wFlags, UINT uRow, DWORD dwItemID, ULONG ulRank);
STDAPI DocFind_CreateIDList(IShellFolder *psf, WIN32_FIND_DATA *pfd, int iFolder, WORD wFlags, UINT uRow, DWORD dwItemID, ULONG ulRank, LPITEMIDLIST *ppidl);

#define DocFind_AppendIFolder(pidl, iFolder) DocFind_AppendDocFindData((pidl), (iFolder), DFDF_NONE, 0, 0, 0)

typedef struct _dfpagelist
{
    int id;             // Id of template in resource file.
    DLGPROC pfn;        // pointer to dialog proc
} DFPAGELIST;

typedef struct
{
    TC_ITEMHEADER   tci;
    HWND            hwndPage;
    UINT            state;
} TC_DFITEMEXTRA;

#define CB_DFITEMEXTRA (SIZEOF(TC_DFITEMEXTRA)-SIZEOF(TC_ITEMHEADER))

// FILEFILTER flags values                                                /* ;Internal */
#define FFLT_INCLUDESUBDIRS 0x0001      // Include subdirectories in search /* ;Internal */
#define FFLT_SAVEPATH       0x0002      // Save path in FILEINFOs         /* ;Internal */ // unused!
#define FFLT_REGULAR        0x0004      // Use Regular expressions        /* ;Internal */
#define FFLT_CASESEN        0x0008      // Do case sensitive search       /* ;Internal */
#define FFLT_EXCSYSFILES    0x0010      // Should exclude system files    /* ;Internal */ // unused!

//
// Define structure that will be saved out to disk.
//
#define DOCFIND_SIG     (TEXT('D') | (TEXT('F') << 8))
typedef struct _dfHeaderWin95
{
    WORD    wSig;       // Signature
    WORD    wVer;       // Version
    DWORD   dwFlags;    // Flags that controls the sort
    WORD    wSortOrder; // Current sort order
    WORD    wcbItem;    // Size of the fixed portion of each item.
    DWORD   oCriteria;  // Offset to criterias in list
    long    cCriteria;  // Count of Criteria
    DWORD   oResults;   // Starting location of results in file
    long    cResults;   // Count of items that have been saved to file
    UINT    ViewMode;   // The view mode of the file...
} DFHEADER_WIN95;
typedef struct _dfHeader
{
    WORD    wSig;       // Signature
    WORD    wVer;       // Version
    DWORD   dwFlags;    // Flags that controls the sort
    WORD    wSortOrder; // Current sort order
    WORD    wcbItem;    // Size of the fixed portion of each item.
    DWORD   oCriteria;  // Offset to criterias in list
    long    cCriteria;  // Count of Criteria
    DWORD   oResults;   // Starting location of results in file
    long    cResults;   // Count of items that have been saved to file
    UINT    ViewMode;   // The view mode of the file...
    DWORD   oHistory;   // IPersistHistory::Save offset
} DFHEADER;

// The check in Win95/NT4 would fail to read the DFHEADER structure if
// the wVer field was > 3.  This is lame since the DFHEADER struct is
// backwards compiatible (that's why it uses offsets).  So we either
// go through the pain of revving the stream format in a backwards
// compatible way (not impossible, just a pain in the brain), or simply
// rev the version and add our new fields and call the Win95/NT4 problem
// a bug and punt.  I'm leaning towards "bug" as this is a rarely used feature.
#define DF_CURFILEVER_WIN95  3
#define DF_CURFILEVER        4

// define the format of the column information.
typedef struct _dfCriteria
{
    WORD    wNum;       // Criteria number (cooresponds to dlg item id)
    WORD    cbText;     // size of text including null char (DavePl: code using this now assumes byte count)
} DFCRITERIA;

// Formats for saving find criteria.
#define DFC_FMT_UNICODE   1
#define DFC_FMT_ANSI      2

// This is a subset of fileinfo structure
typedef struct _dfItem
{
    WORD    flags;          // FIF_ bits
    WORD    timeLastWrite;
    WORD    dateLastWrite;
    WORD    dummy;              // 16/32 bit compat.
                                //the compiler adds this padding
                                // remove and use if needed
    DWORD   dwSize;     // size of the file
    WORD    cbPath;     // size of the text (0 implies use previous files)
    WORD    cbName;     // Size of name including NULL.
} DFITEM;



EXTERN_C TCHAR const s_szDocPathMRU[];
EXTERN_C TCHAR const s_szDocSpecMRU[];
EXTERN_C TCHAR const s_szDocContainsMRU[];
EXTERN_C const UINT s_auMapDFColToFSCol[];


BOOL_PTR CALLBACK DocFind_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Message handlers

HANDLE DocFind_UpdateMRUItem(HANDLE hMRU, HWND hwndDlg, int iDlgItem,
        LPCTSTR szSection, LPTSTR pszInitString, LPCTSTR pszAddIfEmpty);

EXTERN_C void DocFind_EnableMonitoringNewItems(HWND hwndDlg, BOOL fEnable);

// functions in docfind2.c
#ifdef __cplusplus
extern "C" {
#endif
VOID DocFind_AddDefPages(DOCFIND *pdf);
IDocFindFileFilter * CreateDefaultDocFindFilter(void);
void DocFind_SizeControl(HWND hwndDlg, int id, int cx, BOOL fCombo);
void DocFind_ReportItemValueError(HWND hwndDlg, int idCtl,
        int iMsg, LPTSTR pszValue);
BOOL DocFind_SetupWildCardingOnFileSpec(LPTSTR pszSpecIn,
        LPTSTR * pszSpecOut);

int Docfind_SaveCriteriaItem(IStream * pstm, WORD wNum, LPTSTR psz, WORD fCharType);
WORD DocFind_GetTodaysDosDateMinusNDays(int nDays);
VOID DocFind_DosDateToSystemtime(WORD wFatDate, LPSYSTEMTIME pst);


// Define some psuedo property sheet messages...

// Sent to pages to tell the page if it should allow the user to make
// changes or not wParam is a BOOL (TRUE implies yes changes enabled)
#define DFM_ENABLECHANGES           (WM_USER+242)

// Sent to the "Name & Location" prop page to tell it to save the "look in"
// combo box value off when the dialog is being destroyed 
#define DFM_SAVELOOKIN              (WM_USER+243)


// functions in netfind.c

IDocFindFileFilter * CreateDefaultComputerFindFilter();

STDAPI_(BOOL) NextIDL(IEnumIDList *penum, LPITEMIDLIST *ppidl);

LPITEMIDLIST DocFind_ConvertPathToIDL(LPTSTR pszFullPath);


int DocFind_LocCBFindPidl(HWND hwnd, LPITEMIDLIST pidl);

int DocFind_LocCBAddItem(HWND hwnd, LPCITEMIDLIST pidlAbs,
                         int iIndex, int iIndent, int iImage, LPTSTR lpszText);

int DocFind_LocCBAddPidl(HWND hwnd, IShellFolder *psf,
       LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlAbs,
       BOOL fFullName, UINT iIndent);

BOOL DocFind_LocCBMeasureItem(HWND hwnd,
        MEASUREITEMSTRUCT FAR* lpMeasureItem);
BOOL DocFind_LocCBDrawItem(HWND hwnd,
        const DRAWITEMSTRUCT FAR* lpdi);

// Helper function such that I can get my hand on the thread handle.
HANDLE DocFind_GetSearchThreadHandle(HWND hwndDlg);


// Helper functions to use automation to open folders and select items...
HRESULT OpenContainingFolderAndGetShellFolderView(LPCITEMIDLIST pidlFolder, IShellFolderViewDual **ppsfv);
HRESULT SelectPidlInSFV(IShellFolderViewDual *psfv, LPCITEMIDLIST pidl, DWORD dwOpts);

// Helper function to get the IShellFolder for the folder.
// Note: It is safe to pass NULL for pdff and pidl if a valid pdffli is passed in.
IShellFolder *DocFind_GetObjectsIFolder(IDocFindFolder *pdff, DFFolderListItem *pdffli, LPCITEMIDLIST pidl);

#ifdef __cplusplus
}
#endif


//
// Used to implement a stack of sets of subdirectory names...
//
typedef struct _DIRBUF
{
    TCHAR FAR* ach;
    int ichPathEnd;
    UINT cb;
    UINT cbAlloc;
    UINT ichDirNext;
    LPTSTR psz;
    struct _DIRBUF FAR* pdbNext;
} DIRBUF;


//===========================================================================
// OLEDB search
//===========================================================================

EXTERN_C HRESULT DocFind_CreateOleDBEnum(
    IDocFindFileFilter * pdfff,
    IShellFolder *psf,
    LPWSTR *apwszPaths,
    UINT    *pcPaths,
    DWORD grfFlags,
    int iColSort,
    LPTSTR pszProgressText,
    IRowsetWatchNotify *prwn,
    IDFEnum **ppdfenum);


STDAPI SHFlushClipboard(void);              // In ole2def.c

#undef  INTERFACE
#define INTERFACE       IDocFindControllerNotify

// This interface is used to let the callback class talk to the class that is actually controlling
// the queries and the like.
DECLARE_INTERFACE_(IDocFindControllerNotify, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    
    // *** IDocFindControllerNotify methods ***
    STDMETHOD(DoSortOnColumn)(THIS_ UINT iCol, BOOL fSameCol) PURE;
    STDMETHOD(SaveSearch)(THIS) PURE;
    STDMETHOD(RestoreSearch)(THIS) PURE;
    STDMETHOD(StopSearch)(THIS) PURE;
    STDMETHOD(GetItemCount)(THIS_ UINT *pcItems) PURE;
    STDMETHOD(SetItemCount)(THIS_ UINT cItems) PURE;
    STDMETHOD(ViewDestroyed)(THIS) PURE;
};

#undef  INTERFACE
#define INTERFACE       IDocFindFolder

DECLARE_INTERFACE_(IDocFindFolder, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    
    // *** IDocFindFolder methods ***
    STDMETHOD(SetDocFindFilter)(THIS_ IDocFindFileFilter  *pdfff) PURE;
    STDMETHOD(GetDocFindFilter)(THIS_ IDocFindFileFilter  **pdfff) PURE;

    STDMETHOD(AddPidl)(THIS_ int i, LPITEMIDLIST pidl, DWORD dwItemID, ESFItem **ppItem) PURE;
    STDMETHOD(GetItem)(THIS_ int iItem, ESFItem **ppItem) PURE;
    STDMETHOD(DeleteItem)(THIS_ int iItem) PURE;
    STDMETHOD(GetItemCount)(THIS_ INT *pcItems) PURE;
    STDMETHOD(ValidateItems)(THIS_ int iItemFirst, int cItems, BOOL bSearchComplete) PURE;
    STDMETHOD(GetFolderListItemCount)(THIS_ INT *pcCount) PURE;
    STDMETHOD(GetFolderListItem)(THIS_ int iItem, DFFolderListItem **ppItem) PURE;
    STDMETHOD(SetItemsChangedSinceSort)(THIS) PURE;
    STDMETHOD(ClearItemList)(THIS) PURE;
    STDMETHOD(ClearFolderList)(THIS) PURE;
    STDMETHOD(AddFolderToFolderList)(THIS_ LPITEMIDLIST pidl, BOOL fCheckForDup, int * piFolder) PURE;
    STDMETHOD(SetAsyncEnum)(THIS_ IDFEnum *pdfEnumAsync) PURE;
    STDMETHOD(GetAsyncEnum)(THIS_ IDFEnum **ppdfEnumAsync) PURE;
    STDMETHOD(SetAsyncCount)(THIS_ DBCOUNTITEM cCount) PURE;
    STDMETHOD(CacheAllAsyncItems)(THIS) PURE;
    STDMETHOD_(BOOL,AllAsyncItemsCached)(THIS) PURE;
    STDMETHOD(ClearSaveStateList)(THIS) PURE;
    STDMETHOD(GetStateFromSaveStateList)(THIS_ DWORD dwItemID, DWORD *pdwState) PURE;
    STDMETHOD(MapFSPidlToDFPidl)(LPITEMIDLIST pidl, BOOL fMapToReal, LPITEMIDLIST *ppidl) PURE;
    STDMETHOD(GetParentsPIDL)(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlParent) PURE;
    STDMETHOD(RememberSelectedItems)(THIS) PURE;
    STDMETHOD(SetControllerNotifyObject)(IDocFindControllerNotify *pdfcn) PURE;
    STDMETHOD(GetControllerNotifyObject)(IDocFindControllerNotify **ppdfcn) PURE;
    STDMETHOD(SaveFolderList)(THIS_ IStream *pstm) PURE;
    STDMETHOD(RestoreFolderList)(THIS_ IStream *pstm) PURE;
    STDMETHOD(SaveItemList)(THIS_ IStream *pstm) PURE;
    STDMETHOD(RestoreItemList)(THIS_ IStream *pstm, int *pcItems) PURE;
    STDMETHOD(RestoreSearchFromSaveFile)(LPITEMIDLIST pidlSaveFile, IShellFolderView *psfv) PURE;
    STDMETHOD(SetEmptyText)(THIS_ LPCTSTR pszText) PURE;
    STDMETHOD_(BOOL,IsSlow)(THIS) PURE;
};

#ifdef __cplusplus
extern "C" {
#endif
IContextMenu* CDFFolderContextMenuItem_Create(HWND hwndOwner, IDocFindFolder *pdfFolder);
#ifdef __cplusplus
}
#endif

#endif   // !_INC_DOCFIND
