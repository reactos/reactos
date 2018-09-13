#ifndef _UTIL_H_
#define _UTIL_H_

#include <shldispp.h>

int IsVK_TABCycler(MSG *pMsg);
BOOL IsVK_CtlTABCycler(MSG *pMsg);

#ifdef __cplusplus
//=--------------------------------------------------------------------------=
// allocates a temporary buffer that will disappear when it goes out of scope
// NOTE: be careful of that -- make sure you use the string in the same or
// nested scope in which you created this buffer. people should not use this
// class directly.  use the macro(s) below.
//
class TempBuffer {
#ifdef DEBUG
    const DWORD* _pdwSigniture;
    const static DWORD s_dummy;
#endif
  public:
    TempBuffer(ULONG cBytes) {
#ifdef DEBUG
        _pdwSigniture = &s_dummy;
#endif
        m_pBuf = (cBytes <= 120) ? &m_szTmpBuf : LocalAlloc(LMEM_FIXED, cBytes);
        m_fHeapAlloc = (cBytes > 120);
    }
    ~TempBuffer() {
        if (m_pBuf && m_fHeapAlloc) LocalFree(m_pBuf);
    }
    void *GetBuffer() {
        return m_pBuf;
    }

  private:
    void *m_pBuf;
    // we'll use this temp buffer for small cases.
    //
    char  m_szTmpBuf[120];
    unsigned m_fHeapAlloc:1;
};

#endif  // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

//=--------------------------------------------------------------------------=
// string helpers.
//
// given and ANSI String, copy it into a wide buffer.
// be careful about scoping when using this macro!
//
// how to use the below two macros:
//
//  ...
//  LPSTR pszA;
//  pszA = MyGetAnsiStringRoutine();
//  MAKE_WIDEPTR_FROMANSI(pwsz, pszA);
//  MyUseWideStringRoutine(pwsz);
//  ...
//
// similarily for MAKE_ANSIPTR_FROMWIDE.  note that the first param does not
// have to be declared, and no clean up must be done.
//
#define MAKE_WIDEPTR_FROMANSI(ptrname, ansistr) \
    long __l##ptrname = (lstrlen(ansistr) + 1) * sizeof(WCHAR); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    MultiByteToWideChar(CP_ACP, 0, ansistr, -1, (LPWSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname); \
    LPWSTR ptrname = (LPWSTR)__TempBuffer##ptrname.GetBuffer()

#define MAKE_ANSIPTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (lstrlenW(widestr) + 1) * 2 * sizeof(char); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    WideCharToMultiByte(CP_ACP, 0, widestr, -1, (LPSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname, NULL, NULL); \
    LPSTR ptrname = (LPSTR)__TempBuffer##ptrname.GetBuffer()

BOOL __cdecl _FormatMessage(LPCWSTR szTemplate, LPWSTR szBuf, UINT cchBuf, ...);

HRESULT IUnknown_FileSysChange(IUnknown* punk, DWORD dwEvent, LPCITEMIDLIST* ppidl);
HRESULT QueryService_SID_IBandProxy(IUnknown * punkParent, REFIID riid, IBandProxy ** ppbp, void **ppvObj);
HRESULT CreateIBandProxyAndSetSite(IUnknown * punkParent, REFIID riid, IBandProxy ** ppbp, void **ppvObj);
DWORD GetPreferedDropEffect(IDataObject *pdtobj);
HRESULT _SetPreferedDropEffect(IDataObject *pdtobj, DWORD dwEffect);
#ifdef DEBUG
int SearchDWP(DWORD_PTR *pdwBuf, int cbBuf, DWORD_PTR dwVal);
#endif


#define REGVALUE_STREAMSA           "Streams"
#define REGVALUE_STREAMS            TEXT(REGVALUE_STREAMSA)

#define SZ_REGKEY_TYPEDCMDMRU       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU")
#define SZ_REGKEY_TYPEDURLMRU       TEXT("Software\\Microsoft\\Internet Explorer\\TypedURLs")
#define SZ_REGVAL_MRUENTRY          TEXT("url%lu")

#define SZ_REGKEY_INETCPL_POLICIES   TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Control Panel")
#define SZ_REGVALUE_RESETWEBSETTINGS TEXT("ResetWebSettings")

#define SZ_REGKEY_IE_POLICIES       TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Main")
#define SZ_REGVALUE_IEREPAIR        TEXT("Repair IE Option")
#define SZ_REGKEY_ACTIVE_SETUP      TEXT("Software\\Microsoft\\Active Setup")
#define SZ_REGVALUE_DISABLE_REPAIR  TEXT("DisableRepair")

#if 0
BOOL    IsIERepairOn();
#endif

BOOL IsResetWebSettingsEnabled(void);

HRESULT GetMRUEntry(HKEY hKey, DWORD dwMRUIndex, LPTSTR pszMRUEntry, DWORD cchMRUEntry, LPITEMIDLIST * ppidl);

extern UINT g_cfURL;
extern UINT g_cfHIDA;
extern UINT g_cfFileDescA;
extern UINT g_cfFileDescW;
extern UINT g_cfFileContents;

extern const CLSID g_clsidNull; // for those that want a NULL clsid.

void InitClipboardFormats();

// raymondc's futile attempt to reduce confusion
//
// EICH_KBLAH = a registry key named blah
// EICH_SBLAH = a win.ini section named blah

#define EICH_UNKNOWN        0xFFFFFFFF
#define EICH_KINET          0x00000002
#define EICH_KINETMAIN      0x00000004
#define EICH_KWIN           0x00000008
#define EICH_KWINPOLICY     0x00000010
#define EICH_KWINEXPLORER   0x00000020
#define EICH_SSAVETASKBAR   0x00000040
#define EICH_SWINDOWMETRICS 0x00000080
#define EICH_SPOLICY        0x00000100
#define EICH_SSHELLMENU     0x00000200
#define EICH_KWINEXPLSMICO  0x00000400
#define EICH_SWINDOWS       0x00000800

DWORD SHIsExplorerIniChange(WPARAM wParam, LPARAM lParam);


STDAPI_(HKEY) SHGetExplorerHkey();

#define GEN_DEBUGSTRW(str)  ((str) ? (str) : L"<Null Str>")
#define GEN_DEBUGSTRA(str)  ((str) ? (str) : "<Null Str>")

#ifdef UNICODE
#define GEN_DEBUGSTR  GEN_DEBUGSTRW
#else // UNICODE
#define GEN_DEBUGSTR  GEN_DEBUGSTRA
#endif // UNICODE


void _InitAppGlobals();
BOOL _InitComCtl32();

void ReleaseStgMediumHGLOBAL(STGMEDIUM *pstg);
void* DataObj_GetDataOfType(IDataObject* pdtobj, UINT cfType, STGMEDIUM *pstg);
HRESULT RootCreateFromPath(LPCTSTR pszPath, LPITEMIDLIST * ppidl);

extern DEFFOLDERSETTINGS g_dfs;

STDAPI_(void) SaveDefaultFolderSettings(UINT flags);
STDAPI_(void) GetCabState(CABINETSTATE *pcs);

//-------------------------------------------------------------------------

//***   Reg_GetStrs -- read registry strings into struct fields
struct regstrs
{
    LPTSTR  name;   // registry name
    int     off;    // struct offset
};

BOOL    ViewIDFromViewMode(UINT uViewMode, SHELLVIEWID *pvid);
void Reg_GetStrs(HKEY hkey, const struct regstrs *tab, TCHAR *szBuf, int cchBuf, void *pv);
void IEPlaySound(LPCTSTR pszSound, BOOL fSysSound);

HRESULT DropOnMailRecipient(IDataObject *pdtobj, DWORD grfKeyState);
HRESULT SendDocToMailRecipient(LPCITEMIDLIST pidl, UINT uiCodePage, DWORD grfKeyState, IUnknown *pUnkSite);

#ifdef DEBUG
void    Dbg_DumpMenu(LPCTSTR psz, HMENU hmenu);
#else
#define Dbg_DumpMenu(psz, hmenu)
#endif

extern const LARGE_INTEGER c_li0;
extern BOOL g_fNewNotify;
// BUGBUG:: Need to handle two different implementations of SHChangeRegister...
typedef ULONG (* PFNSHCHANGENOTIFYREGISTER)(HWND hwnd, int fSources, LONG fEvents, UINT wMsg, int cEntries, SHChangeNotifyEntry *pshcne);
typedef BOOL  (* PFNSHCHANGENOTIFYDEREGISTER)(unsigned long ulID);

ULONG RegisterNotify(HWND hwnd, UINT nMsg, LPCITEMIDLIST pidl, DWORD dwEvents, UINT uFlags, BOOL fRecursive);

STDAPI_(void) DetectRegisterNotify(void);
int PropBag_ReadInt4(IPropertyBag* pPropBag, LPWSTR pszKey, int iDefault);
HINSTANCE HinstShdocvw();
HINSTANCE HinstShell32();

HRESULT CreateFromRegKey(LPCTSTR pszKey, LPCTSTR pszValue, REFIID riid, void **ppv);

extern const VARIANT c_vaEmpty;
//
// BUGBUG: Remove this ugly const to non-const casting if we can
//  figure out how to put const in IDL files.
//
#define PVAREMPTY ((VARIANT*)&c_vaEmpty)

BOOL ILIsBrowsable(LPCITEMIDLIST pidl, BOOL *pfISFolder);

STDAPI_(LPITEMIDLIST) IEILCreate(UINT cbSize);

BOOL GetInfoTipEx(IShellFolder* psf, DWORD dwFlags, LPCITEMIDLIST pidl, LPTSTR pszText, int cchTextMax);
BOOL IsBrowsableShellExt(LPCITEMIDLIST pidl);
void OpenFolderPidl(LPCITEMIDLIST pidl);
void OpenFolderPath(LPCTSTR pszPath);
int WINAPI _SHHandleUpdateImage( LPCITEMIDLIST pidlExtra );

extern BOOL g_fICWCheckComplete;
BOOL    CheckSoftwareUpdateUI( HWND hwndOwner, IShellBrowser *pisb );
BOOL    CheckRunICW(LPCTSTR);

#ifdef DEBUG
LPTSTR Dbg_PidlStr(LPCITEMIDLIST pidl, LPTSTR pszBuffer, DWORD cchBufferSize);
#else // DEBUG
#define Dbg_PidlStr(pidl, pszBuffer, cchBufferSize)   ((LPTSTR)NULL)
#endif // DEBUG

HRESULT SavePidlAsLink(IUnknown* punkSite, IStream *pstm, LPCITEMIDLIST pidl);
HRESULT LoadPidlAsLink(IUnknown* punkSite, IStream *pstm, LPITEMIDLIST *ppidl);

#define ADJUST_TO_WCHAR_POS     0
#define ADJUST_TO_TCHAR_POS     1
int AdjustECPosition(char *psz, int iPos, int iType);
HRESULT ContextMenu_GetCommandStringVerb(LPCONTEXTMENU pcm, UINT idCmd, LPTSTR pszVerb, int cchVerb);
BOOL ExecItemFromFolder(HWND hwnd, LPCSTR pszVerb, IShellFolder* psf, LPCITEMIDLIST pidlItem);

// See if a give URL is actually present as an installed entry
STDAPI_(BOOL) CallCoInternetQueryInfo(LPCTSTR pszURL, QUERYOPTION QueryOption);
#define UrlIsInstalledEntry(pszURL) CallCoInternetQueryInfo(pszURL, QUERY_IS_INSTALLEDENTRY)

BOOL IsSubscribableA(LPCSTR pszUrl);
BOOL IsSubscribableW(LPCWSTR pwzUrl);

HRESULT IURLQualifyW(IN LPCWSTR pcwzURL, DWORD dwFlags, OUT LPWSTR pwzTranslatedURL, LPBOOL pbWasSearchURL, LPBOOL pbWasCorrected);

#ifdef UNICODE
#define IsSubscribable IsSubscribableW
#define IURLQualifyT   IURLQualifyW
#else // UNICODE
#define IsSubscribable IsSubscribableA
#define IURLQualifyT   IURLQualifyA
#endif // UNICODE

#define IURLQualifyA   IURLQualify


HDPA    GetSortedIDList(LPITEMIDLIST pidl);
void    FreeSortedIDList(HDPA hdpa);

//#define StopWatch       StopWatchT

int GetColorComponent(LPSTR *ppsz);
COLORREF RegGetColorRefString( HKEY hkey, LPTSTR RegValue, COLORREF Value);
LRESULT SetHyperlinkCursor(IShellFolder* pShellFolder, LPCITEMIDLIST pidl);

HRESULT StrCmpIWithRoot(LPCTSTR szDispNameIn, BOOL fTotalStrCmp, LPTSTR * ppszCachedRoot);
STDAPI UpdateSubscriptions();

enum TRI_STATE
{
    TRI_UNKNOWN = 2,
    TRI_TRUE = TRUE,
    TRI_FALSE = FALSE
};
LONG OpenRegUSKey(LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
HWND GetTrayWindow();

#define STREAMSIZE_UNKNOWN  0xFFFFFFFF
HRESULT SaveStreamHeader(IStream *pstm, DWORD dwSignature, DWORD dwVersion, DWORD dwSize);
HRESULT LoadStreamHeader(IStream *pstm, DWORD dwSignature, DWORD dwStartVersion, DWORD dwEndVersion, DWORD * pdwSize, DWORD * pdwVersionOut);

// _FrameTrack flags
#define TRACKHOT        0x0001
#define TRACKEXPAND     0x0002
#define TRACKNOCHILD    0x0004
void FrameTrack(HDC hdc, LPRECT prc, UINT uFlags);

#ifdef __cplusplus
//+-------------------------------------------------------------------------
// This function scans the document for the given HTML tag and returns the
// result in a collection.
//--------------------------------------------------------------------------
interface IHTMLDocument2;
interface IHTMLElementCollection;
HRESULT GetDocumentTags(IHTMLDocument2* pHTMLDocument, LPOLESTR pszTagName, IHTMLElementCollection** ppTagsCollection);


// CMenuList:  a small class that tracks whether a given hmenu belongs
//             to the frame or the object, so the messages can be
//             dispatched correctly.
class CMenuList
{
public:
    CMenuList(void);
    ~CMenuList(void);

    void Set(HMENU hmenuShared, HMENU hmenuFrame);
    void AddMenu(HMENU hmenu);
    void RemoveMenu(HMENU hmenu);
    BOOL IsObjectMenu(HMENU hmenu);

#ifdef DEBUG
    void Dump(LPCTSTR pszMsg);
#endif

private:
    HDSA    _hdsa;
};


};
#endif


#ifdef __cplusplus
class CAssociationList
{
public:
    //
    // WARNING: We don't want a destructor on this because then it can't
    // be a global object without bringing in the CRT main.  So
    // we can't free the DSA in a destructor.  The DSA memory will be
    // freed by the OS when the process detaches.  If this
    // class is ever dynamically allocated (ie not a static) then
    // we will need to free the DSA.
    //
//    ~CAssociationList();
    BOOL    Add(DWORD dwKey, LPVOID lpData);
    void    Delete(DWORD dwKey);
    HRESULT Find(DWORD dwKey, LPVOID* ppData);

protected:
    int     FindEntry(DWORD dwKey);

    struct ASSOCDATA
    {
            DWORD dwKey;
            LPVOID lpData;
    };

    HDSA    _hdsa;
};

#endif

STDAPI_(void) DrawMenuItem(DRAWITEMSTRUCT* pdi, LPCTSTR lpszMenuText, UINT iIcon);
STDAPI_(LRESULT) MeasureMenuItem(MEASUREITEMSTRUCT *lpmi, LPCTSTR lpszMenuText);

void FireEventSz(LPCTSTR szEvent);
#ifndef UNICODE
void FireEventSzW(LPCWSTR szEvent);
#else
#define FireEventSzW FireEventSz
#endif

// comctl32.dll doesn't really implement Str_SetPtrW.
STDAPI_(BOOL) Str_SetPtrPrivateW(WCHAR * UNALIGNED * ppwzCurrent, LPCWSTR pwzNew);

// This function is similar to Str_SetPtrPrivateW but it is compatible with API's
// that use LocalAlloc for string memory
STDAPI_(BOOL) SetStr(WCHAR * UNALIGNED * ppwzCurrent, LPCWSTR pwzNew);

// Review chrisny:  this can be moved into an object easily to handle generic droptarget, dropcursor
// , autoscrool, etc. . .
STDAPI_(void) _DragEnter(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtObject);
STDAPI_(void) _DragMove(HWND hwndTarget, const POINTL ptStart);

#define Str_SetPtrW         Str_SetPtrPrivateW

STDAPI_(BOOL) _MenuCharMatch(LPCTSTR lpsz, TCHAR ch, BOOL fIgnoreAmpersand);

STDAPI GetNavigateTarget(IShellFolder *psf, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl, DWORD *pdwAttribs);

STDAPI_(BOOL) DoDragDropWithInternetShortcut(IOleCommandTarget *pcmdt, LPITEMIDLIST pidl, HWND hwnd);

STDAPI_(BSTR) SysAllocStringA(LPCSTR);

#ifdef UNICODE
#define SysAllocStringT(psz)    SysAllocString(psz)
#else
#define SysAllocStringT(psz)    SysAllocStringA(psz)
#endif

void EnableOKButton(HWND hDlg, int id, LPTSTR pszText);
BOOL IsExplorerWindow(HWND hwnd);
BOOL IsFolderWindow(HWND hwnd);

BOOL WasOpenedAsBrowser(IUnknown *punkSite);

STDAPI_(LPCITEMIDLIST) VariantToConstIDList(const VARIANT *pv);
STDAPI_(LPITEMIDLIST) VariantToIDList(const VARIANT *pv);
STDAPI VariantToBuffer(const VARIANT* pvar, void *pv, UINT cb);
STDAPI VariantToGUID(VARIANT *pvar, LPGUID pguid);
STDAPI InitVariantFromBuffer(VARIANT *pvar, const void *pv, UINT cb);
STDAPI_(BOOL) InitVariantFromIDList(VARIANT* pvar, LPCITEMIDLIST pidl);
STDAPI_(BOOL) InitVariantFromGUID(VARIANT *pvar, GUID *pguid);
STDAPI_(void) InitVariantFromIDListInProc(VARIANT *pvar, LPCITEMIDLIST pidl);

STDAPI_(HWND) GetTopLevelAncestor(HWND hWnd);

STDAPI SHNavigateToFavorite(IShellFolder* psf, LPCITEMIDLIST pidl, IUnknown* punkSite, DWORD dwFlags);
STDAPI SHGetTopBrowserWindow(IUnknown* punk, HWND* phwnd);
STDAPI_(void) UpdateButtonArray(TBBUTTON *ptbDst, const TBBUTTON *ptbSrc, int ctb, LONG_PTR lStrOffset);
STDAPI_(BOOL) DoesAppWantUrl(LPCTSTR pszCmdLine);

STDAPI SHCreateThreadRef(LONG *pcRef, IUnknown **ppunk);
BOOL ILIsFolder(LPCITEMIDLIST pidl);
HRESULT URLToCacheFile(LPCWSTR pszUrl, LPWSTR pszFile, int cchFile);

#ifdef DEBUG
void DebugDumpPidl(DWORD dwDumpFlag, LPTSTR pszOutputString, LPCITEMIDLIST pidl);
#else
#define DebugDumpPidl(p, q, w)
#endif

STDAPI_(LPITEMIDLIST) SafeILClone(LPCITEMIDLIST pidl);
#define ILClone         SafeILClone   

#endif // _UTIL_H_
