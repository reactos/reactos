#ifndef _FSTREEX_INC
#define _FSTREEX_INC

#include "idlcomm.h"
#include <filetype.h>
#include "pidl.h"       // IDFOLDER

typedef struct
{
#ifdef __cplusplus
    void *              iunk;
    void *              sf;
    void *              si;
    void *              sio;
    void *              pf;
#else // __cplusplus
    IUnknown            iunk;
    IShellFolder2       sf;
    IShellIcon          si;
    IShellIconOverlay   sio;
    IPersistFolder3     pf;
#endif // __cplusplus
    UINT                cRef;
    IUnknown            *punkOuter;

    LPITEMIDLIST        _pidl;                  // Absolute IDList (location in the name space)
    LPITEMIDLIST        _pidlTarget;            // Absolute IDList for folder target (location in namespace to enumerate)
                                                // WARNING: _csidlTrack overrides _pidlTarget
    LPTSTR              _pszPath;               // file system path (may be different from _pidl)
    LPTSTR              _pszNetProvider;        // network provider (for net calls we may need to make)

    int                 cHiddenFiles;
    ULONGLONG           cbSize;

    UINT                _csidl;                 // CSIDL_ value of this folder (if known)
    DWORD               _dwAttributes;          // attributes of this folder (if known)
    int                 _csidlTrack;            // CSIDL_ that we follow dynamically

    BOOL                fCachedCLSID : 1;       // clsidView is already cached
    BOOL                fHasCLSID    : 1;       // clsidView has a valid CLSID
    CLSID               _clsidView;             // CLSID for View object
    CLSID               _clsidBind;             // use CLSID_NULL for normal case
    HDSA                _hdsaColHandlers;       // cached list of columns and handlers
    DWORD               _dwColCount;            // count of unique columns
    int                 _iFolderIcon;           // icon for sub folders to inherit
    BOOL                _bUpdateExtendedCols;   // set to TRUE in response to SFVM_INSERTITEM callback, passed to IColumnProvider::GetItemData then cleared 
} CFSFolder;

// fstree.cpp
STDAPI CFSFolderCallback_Create(IShellFolder* psf, CFSFolder *pFSFolder, LONG lEvents, IShellFolderViewCB **ppsfvcb);

int FSSortIDToICol(int x);

STDAPI_(void) SHGetTypeName(LPCTSTR pszFile, HKEY hkey, BOOL fFolder, LPTSTR pszName, int cchNameMax);


STDAPI FS_CompareNamesCase(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2);
STDAPI FS_CompareFolderness(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2);
STDAPI FS_CompareModifiedDate(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2);
STDAPI FS_CompareAttribs(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2);

BOOL FS_HasType(LPCTSTR pszFileName);
void FS_GetTypeName(LPCIDFOLDER pidf, LPTSTR pszName, int cchNameMax);
STDAPI_(void) FS_GetSize(LPCITEMIDLIST pidlParent, LPCIDFOLDER pidf, ULONGLONG *pcbSize);
BOOL FS_ShowExtension(LPCIDFOLDER pidf);

STDAPI_(CFSFolder *) FS_GetFSFolderFromShellFolder(IShellFolder *psf);
STDAPI CFSFolder_GetPathForItem(CFSFolder *pfsf, LPCIDFOLDER pidf, LPTSTR pszPath);
STDAPI CFSFolder_GetPathForItemW(CFSFolder *pfsf, LPCIDFOLDER pidf, LPWSTR pszPath);
STDAPI_(DWORD) CFSFolder_Attributes(CFSFolder *pfsf);
STDAPI_(UINT) CFSFolder_GetCSIDL(CFSFolder *pfsf);
STDAPI_(BOOL) CFSFolder_IsCSIDL(CFSFolder *pfsf, UINT csidl);
STDAPI CFSFolder_GetCCHMax(CFSFolder *that, LPCIDFOLDER pidf, UINT *pcchMax);
STDAPI CFSFolder_CreateDataObject(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[], IDataObject **ppdtobj);
STDAPI CFSFolder_CreateIDList(CFSFolder *pfsf, const WIN32_FIND_DATA *pfd, LPITEMIDLIST *ppidl);

STDAPI GetIconOverlayManager(IShellIconOverlayManager **ppsiom);

typedef struct {
    BOOL fInitialized;
    POINT pt;
    POINT ptOrigin;
    UINT cxItem, cyItem;
    int xMul, yMul, xDiv, yDiv;
    POINT *pptOffset;
    UINT iItem;
} DROPHISTORY, *PDROPHISTORY;

STDAPI_(void) FS_PositionFileFromDrop(HWND hwnd, LPCTSTR pszFile, PDROPHISTORY pdh);
STDAPI_(BOOL) _GetFolderString(LPCTSTR pszFolder, LPCTSTR pszProvider, LPTSTR  pszProfile, int cchMax, LPCTSTR pszKey);


short _CompareFileTypes(IShellFolder *psf, LPCIDFOLDER pidf1, LPCIDFOLDER pidf2);

// Parameter to the "Drop" thread.

typedef struct {         // fsthp
    IDataObject     *pDataObj;      // null on entry to thread proc
    IStream         *pstmDataObj;   // marshalled data object
    DWORD           dwEffect;
    BOOL            fLinkOnly;
    POINT           ptDrop;
    BOOL            fSameHwnd;
    BOOL            fDragDrop;
    BOOL            fBkDropTarget;
    BOOL            bSyncCopy;
    UINT            idCmd;
    LPITEMIDLIST    pidl;
    DWORD           grfKeyState;
    HWND            hwndOwner;
    TCHAR           szPath[MAX_PATH];
} FSTHREADPARAM;

STDAPI FS_CreateFileFromClip(HWND hwnd, LPCTSTR pszPath, IDataObject *pdtobj, POINTL pt, DWORD *pdwEffect, BOOL fIsBkDropTarget);
STDAPI FS_AsyncCreateFileFromClip(HWND hwnd, LPCTSTR pszPath, IDataObject *pdtobj, POINTL pt, DWORD *pdwEffect, BOOL fIsBkDropTarget);

HRESULT CFSIDLData_GetData(IDataObject *pdtobj, LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
HRESULT CFSIDLData_QueryGetData(IDataObject *pdtobj, LPFORMATETC pformatetc);


//===========================================================================
// Class key related functions
//===========================================================================

STDAPI_(BOOL) SHGetClassKey(LPCITEMIDLIST pidl, HKEY *phkeyProgID, HKEY *phkeyBaseID);
STDAPI_(void) SHCloseClassKey(HKEY hkey);

//===========================================================================
// SHGetClassFlags
//===========================================================================
#define SHCF_ICON_INDEX             0x00000FFF
#define SHCF_ICON_PERINSTANCE       0x00001000
#define SHCF_ICON_DOCICON           0x00002000
#define SHCF_00004000               0x00004000
#define SHCF_00008000               0x00008000

#define SHCF_HAS_ICONHANDLER        0x00020000

#define SHCF_IS_DOCOBJECT           0x00100000

#define SHCF_IS_SHELLEXT            0x00400000
#define SHCF_00800000               0x00800000

#define SHCF_IS_LINK                0x01000000
#define SHCF_UNKNOWN                0x04000000
#define SHCF_ALWAYS_SHOW_EXT        0x08000000
#define SHCF_NEVER_SHOW_EXT         0x10000000
#define SHCF_20000000               0x20000000
#define SHCF_40000000               0x40000000
#define SHCF_80000000               0x80000000

STDAPI_(DWORD) SHGetClassFlags(LPCIDFOLDER pidf);

// from fstreex.c

#define SZ_REGKEY_MYCOMPUTER_NONENUM_POLICY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\NonEnum")

STDAPI CFSFolder_CreateFolder(IUnknown *punkOuter, LPCITEMIDLIST pidl, 
                              const PERSIST_FOLDER_TARGET_INFO *pf, REFIID riid, void **ppv);


STDAPI_(HKEY)  SHOpenShellFolderKey(const CLSID *pclsid);
STDAPI_(BOOL)  SHQueryShellFolderValue(const CLSID *pclsid, LPCTSTR pszValueName);
STDAPI_(DWORD) SHGetAttributesFromCLSID(const CLSID *pclsid, DWORD dwDefAttrs);

// dwRequested is the bits you are explicitly looking for. This is an optimization that prevents reg hits.
STDAPI_(DWORD) SHGetAttributesFromCLSID2(const CLSID *pclsid, DWORD dwDefAttrs, DWORD dwRequested);

STDAPI FS_CreateFSIDArray(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST * apidl, IDataObject *pdtInner, IDataObject **pdtobjOut);

STDAPI_(LRESULT) SHRenameFile(HWND hwnd, LPCTSTR pszDir, LPCTSTR pszOldName, LPCTSTR pszNewName, BOOL bRetainExtension);
STDAPI_(LRESULT) SHRenameFileEx(HWND hwnd, IUnknown * punkEnableModless, LPCTSTR pszDir, LPCTSTR pszOldName, LPCTSTR pszNewName, BOOL bRetainExtension);

STDAPI FS_CreateLinks(HWND hwnd, IShellFolder *psf, IDataObject *pdtobj, LPCTSTR pszDir, DWORD fMask);
STDAPI CreateLinkToPidl(LPCITEMIDLIST pidlAbs, LPCTSTR pszDir, LPITEMIDLIST* ppidl, UINT uFlags);

STDAPI CFSFolder_Properties(CFSFolder * thisptr, LPCITEMIDLIST pidlParent, IDataObject *pdtobj, LPCTSTR pStartPage);
STDAPI_(void) CFSFolder_Reset(CFSFolder *thisptr);

STDMETHODIMP_(ULONG) CFSFolderUnk_AddRef(IUnknown *punk);

STDMETHODIMP CFSFolder_QueryInterface(IShellFolder2 *psf, REFIID riid, void **ppvObj);
STDMETHODIMP_(ULONG) CFSFolder_AddRef(IShellFolder2 *psf);
STDMETHODIMP_(ULONG) CFSFolder_Release(IShellFolder2 *psf);

STDMETHODIMP CFSFolder_CreateViewObject(IShellFolder2 *psf, HWND hwnd, REFIID riid, void **ppvOut);
STDMETHODIMP CFSFolder_GetAttributesOf(IShellFolder2 *psf, UINT cidl, LPCITEMIDLIST * apidl, ULONG * prgfInOut);
STDMETHODIMP CFSFolder_ParseDisplayName(IShellFolder2 *psf, HWND hwndOwner, LPBC pbc, WCHAR *pwszDisplayName, ULONG *pchEaten, LPITEMIDLIST *ppidlOut, DWORD *pdwAttributes);
STDMETHODIMP CFSFolder_EnumObjects(IShellFolder2 *psf, HWND hwndOwner, DWORD grfFlags, IEnumIDList **ppenumUnknown);
STDMETHODIMP CFSFolder_BindToObject(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut);
STDMETHODIMP CFSFolder_BindToStorage(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut);
STDMETHODIMP CFSFolder_GetDisplayNameOf(IShellFolder2 *psf, LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET pStrRet);
STDMETHODIMP CFSFolder_SetNameOf(IShellFolder2 *psf, HWND hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR lpsName, DWORD uFlags, LPITEMIDLIST * ppidlOut);

STDMETHODIMP CFSFolder_GetUIObjectOf(IShellFolder2 *psf, HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, void **ppvOut);

STDMETHODIMP CFSFolder_EnumSearches(IShellFolder2 *psf2, LPENUMEXTRASEARCH *ppenum);


DWORD CFSIDLDropTarget_GetDefaultEffect(CIDLDropTarget *thisptr, DWORD grfKeyState, DWORD *pdwEffectInOut, UINT *pidMenu);
HRESULT CFSFolder_GetPath(CFSFolder *pfsf, LPTSTR pszPath);

STDAPI CFSDropTarget_CreateInstance(CFSFolder* pFSFolder, HWND hwnd, IDropTarget** ppdt);
STDAPI CFSFolder_CreateEnum(CFSFolder *pfolder, IUnknown *punk, HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum);

DWORD CALLBACK FileDropTargetThreadProc(void *pv);
STDAPI_(VOID) FreeFSThreadParam(FSTHREADPARAM* pfsthp);

STDAPI_(LPCIDFOLDER) FS_IsValidID(LPCITEMIDLIST pidl);
STDAPI_(LPTSTR)      FS_CopyName(LPCIDFOLDER pidf, LPTSTR pszName, UINT cchName);
STDAPI_(BOOL)        FS_IsCommonItem(LPCITEMIDLIST pidl);
STDAPI_(BOOL)        FS_MakeCommonItem(LPITEMIDLIST pidl);
STDAPI_(BOOL)        FS_IsFolder(LPCIDFOLDER pidf);
STDAPI_(BOOL)        FS_IsFileFolder(LPCIDFOLDER pidf);
STDAPI_(DWORD)       FS_GetUID(LPCIDFOLDER pidf);

#ifdef SYNC_BRIEFCASE

STDAPI CFSBrfFolder_CreateFromIDList(LPCITEMIDLIST pidl, REFIID riid, void **ppv);
STDAPI BrfStg_CreateInstance(LPCITEMIDLIST pidl, HWND hwnd, void **ppv);

#endif // SYNC_BRIEFCASE


// shared stuff for fs view callback (bitbuck shares this)

typedef struct {
    ULONGLONG cbBytes;      // total size of items selected
    int nItems;             // number of items selected

    int cFiles;             // # of files
    int cHiddenFiles;       // # of hiddenf iles
    ULONGLONG cbSize;       // total size of selected files

    int cNonFolders;        // how many non-folders we have

    int idDrive;            // drive info (if in explorer mode)
    ULONGLONG cbFree;       // drive free space
} FSSELCHANGEINFO;

void FSOnInsertDeleteItem(LPCITEMIDLIST pidlParent, FSSELCHANGEINFO *pfssci, LPCITEMIDLIST pidl, int iMul);
void FSOnSelChange(LPCITEMIDLIST pidlParent, SFVM_SELCHANGE_DATA* pdvsci, FSSELCHANGEINFO *pfssci);
HRESULT FSUpdateStatusBar(IUnknown *psite, FSSELCHANGEINFO *pfssci);



#endif
