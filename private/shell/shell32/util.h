//---------------------------------------------------------------------------
// This is a desperate attempt to try and track dependancies.

#ifndef _UTIL_H
#define _UTIL_H

#include "unicpp\utils.h"


STDAPI Stream_WriteString(IStream *pstm, LPCTSTR psz, BOOL bWideInStream);
STDAPI Stream_ReadString(IStream *pstm, LPTSTR pwsz, UINT cchBuf, BOOL bWideInStream);
STDAPI Str_SetFromStream(IStream *pstm, LPTSTR *ppsz, BOOL bWideInStream);
STDAPI CopyStreamUI(IStream *pstmSrc, IStream *pstmDest, IProgressDialog *pdlg);


#define HIDWORD(_qw)    (DWORD)((_qw)>>32)
#define LODWORD(_qw)    (DWORD)(_qw)

// Sizes of various stringized numbers
#define MAX_INT64_SIZE  30              // 2^64 is less than 30 chars long
#define MAX_COMMA_NUMBER_SIZE   (MAX_INT64_SIZE + 10)
#define MAX_COMMA_AS_K_SIZE     (MAX_COMMA_NUMBER_SIZE + 10)


STDAPI_(void)   SHPlaySound(LPCTSTR pszSound);

STDAPI_(BOOL)   TouchFile(LPCTSTR pszFile);
STDAPI_(BOOL)   IsNullTime(const FILETIME *pft);
STDAPI_(LPTSTR) AddCommas(DWORD dw, LPTSTR lpBuff);
STDAPI_(LPTSTR) AddCommas64(_int64 n, LPTSTR lpBuff);
STDAPI_(LPWSTR) AddCommasW(DWORD dw, LPWSTR lpBuff);    // BUGBUG_BOBDAY Temporary until UNICODE
STDAPI_(LPTSTR) ShortSizeFormat(DWORD dw, LPTSTR szBuf);
STDAPI_(LPTSTR) ShortSizeFormat64(__int64 qwSize, LPTSTR szBuf);

STDAPI_(void) DosTimeToDateTimeString(WORD wDate, WORD wTime, LPTSTR pszText, UINT cchText, int fmt);
STDAPI_(int)  GetDateString(WORD wDate, LPTSTR szStr);
STDAPI_(WORD) ParseDateString(LPCWSTR pszStr);
STDAPI_(int)  GetTimeString(WORD wTime, LPTSTR szStr);
STDAPI_(HWND) GetTopLevelAncestor(HWND hWnd);
STDAPI_(BOOL) ParseField(LPCTSTR szData, int n, LPTSTR szBuf, int iBufLen);
STDAPI_(UINT) Shell_MergeMenus(HMENU hmDst, HMENU hmSrc, UINT uInsert, UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags);
STDAPI_(void) SetICIKeyModifiers(DWORD* pfMask);
STDAPI_(void) GetMsgPos(POINT *ppt);

//For use with CreateDesktopComponents
#define DESKCOMP_IMAGE  0x00000001
#define DESKCOMP_URL    0x00000002
#define DESKCOMP_MULTI  0x00000004
#define DESKCOMP_CDF    0x00000008

STDAPI IsDeskCompHDrop(IDataObject * pido);
STDAPI CreateDesktopComponents(LPCSTR pszUrl, IDataObject * pido, HWND hwnd, DWORD fFlags, int x, int y);
STDAPI ExecuteDeskCompHDrop(LPTSTR pszMultipleUrls, HWND hwnd, int x, int y);

STDAPI_(LONG) RegSetString(HKEY hk, LPCTSTR pszSubKey, LPCTSTR pszValue);
STDAPI_(BOOL) RegSetValueString(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPCTSTR psz);
STDAPI_(BOOL) RegGetValueString(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPTSTR psz, DWORD cb);

STDAPI_(BOOL) GetShellClassInfo(LPCTSTR pszPath, LPTSTR pszKey, LPTSTR pszBuffer, DWORD cchBuffer);
STDAPI_(BOOL) GetShellClassInfoInfoTip(LPCTSTR pszPath, LPTSTR pszBuffer, DWORD cchBuffer);
STDAPI_(BOOL) GetShellClassInfoHTMLInfoTipFile(LPCTSTR pszPath, LPTSTR pszBuffer, DWORD cchBuffer);

#define RGS_IGNORECLEANBOOT 0x00000001

#define TrimWhiteSpaceW(psz)        StrTrimW(psz, L" \t")
#define TrimWhiteSpaceA(psz)        StrTrimA(psz, " \t")

#ifdef UNICODE
#define TrimWhiteSpace      TrimWhiteSpaceW
#else
#define TrimWhiteSpace      TrimWhiteSpaceA
#endif

STDAPI_(LPCTSTR) SkipLeadingSlashes(LPCTSTR pszURL);

STDAPI_(BSTR) SysAllocStringA(LPCSTR);

#ifdef UNICODE
#define SysAllocStringT(psz)        SysAllocString(psz)
#else // UNICODE
#define SysAllocStringT(psz)        SysAllocStringA(psz)
#endif // UNICODE

STDAPI Shell32GetTypeInfo(LCID lcid, UUID uuid, ITypeInfo **ppITypeInfo);


// BUGBUG: no reason to use lpcText (make this UINT id) since we
// only want to load resources by ID
STDAPI_(LPSTR) ResourceCStrToStrA(HINSTANCE hAppInst, LPCSTR lpcText);
STDAPI_(LPWSTR) ResourceCStrToStrW(HINSTANCE hAppInst, LPCWSTR lpcText);

#ifdef UNICODE
#define ResourceCStrToStr   ResourceCStrToStrW
#else
#define ResourceCStrToStr   ResourceCStrToStrA
#endif


STDAPI_(void) SHRegCloseKeys(HKEY ahkeys[], UINT ckeys);
STDAPI_(void) HWNDWSPrintf(HWND hwnd, LPCTSTR psz, BOOL fCompactPath);

#define ustrcmp(psz1, psz2) _ustrcmp(psz1, psz2, FALSE)
#define ustrcmpi(psz1, psz2) _ustrcmp(psz1, psz2, TRUE)
int _ustrcmp(LPCTSTR psz1, LPCTSTR psz2, BOOL fCaseInsensitive);

// On Win95, RegDeleteKey deletes the key and all subkeys.  On NT, RegDeleteKey
// fails if there are any subkeys.  On NT, we'll make shell code that assumes
// the Win95 behavior work by mapping SHRegDeleteKey to a helper function that
// does the recursive delete.
#ifdef WINNT
LONG SHRegDeleteKeyW(HKEY hKey, LPCTSTR lpSubKey);
 #ifdef UNICODE
  #define SHRegDeleteKey SHRegDeleteKeyW
 #else  // ANSI WINNT, a case that never really gets shipped.  Avoid making a
       // needless SHRegDeleteKeyA by defining this function to the
       // (non-recursive) RegDeleteKey
  #define SHRegDeleteKey RegDeleteKey
 #endif // UNICODE vs !UNICODE
#else  // !WINNT
 #define SHRegDeleteKey RegDeleteKey
#endif // WINNT vs !WINNT

STDAPI StringToStrRet(LPCTSTR pszName, STRRET *pStrRet);
STDAPI ResToStrRet(UINT id, STRRET *pStrRet);
STDAPI_(LPCTSTR) SkipServerSlashes(LPCTSTR pszName);

STDAPI_(LPITEMIDLIST) ILCombineParentAndFirst(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlNext);
STDAPI_(LPITEMIDLIST) ILCloneUpTo(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlUpTo);

STDAPI_(int)    lstrcmpiNoDBCS(LPCTSTR lpsz1, LPCTSTR lpsz2);

typedef struct {
    LPITEMIDLIST pidlParent;
    LPDATAOBJECT pdtobj;
    LPCTSTR pStartPage;
    IShellFolder* psf;

    // keep this last
    LPTHREAD_START_ROUTINE lpStartAddress;
}  PROPSTUFF;

// BUGBUG (reinerf)
// the fucking alpha cpp compiler seems to fuck up the goddam type "LPITEMIDLIST", so to work
// around the fucking peice of shit compiler we pass the last param as an LPVOID instead of a LPITEMIDLIST
STDAPI_(void) SHLaunchPropSheet(LPTHREAD_START_ROUTINE lpStartAddress, LPDATAOBJECT pdtobj, LPCTSTR pStartPage, IShellFolder* psf, LPVOID pidlParent);


// these don't do anything since shell32 does not support unload, but use this
// for code consistancy with dlls that do support this

#define DllAddRef()
#define DllRelease()

//
//  these are functions that moved from shlexec.c.
//  most of them have something to do with locating and identifying applications
//
HWND GetTopParentWindow(HWND hwnd);
DWORD SHProcessMessagesUntilEvent(HWND hwnd, HANDLE hEvent, DWORD dwTimeout);



// map the PropVariantClear function to our internal wrapper to save loading OleAut32.dll
#define PropVariantClear PropVariantClearLazy
STDAPI PropVariantClearLazy(PROPVARIANT * pvar);

#ifndef UNICODE
int CountChar(LPCTSTR pcsz);
#endif

STDAPI_(BOOL) CenterWindow(HWND hwndChild, HWND hwndParent);

STDAPI GetCurFolderImpl(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl);
STDAPI SHGetIDListFromUnk(IUnknown *punk, LPITEMIDLIST *ppidl);
STDAPI SHGetTargetFolderPath(LPCITEMIDLIST pidl, LPTSTR pszPath, UINT cchBuf);
STDAPI GetPathFromLinkFile(LPCTSTR pszLinkPath, LPTSTR pszTargetPath, int cchTargetPath);

STDAPI_(void)    DrawMenuItem(DRAWITEMSTRUCT* pdi, LPCTSTR lpszMenuText, UINT iIcon);
STDAPI_(LRESULT) MeasureMenuItem(MEASUREITEMSTRUCT *lpmi, LPCTSTR lpszMenuText);

STDAPI_(BOOL) GetFileDescription(LPCTSTR pszPath, LPTSTR pszDesc, UINT *pcchDesc);
STDAPI_(BOOL) IsPathInOpenWithKillList(LPCTSTR pszPath);
STDAPI PathFromDataObject(IDataObject *pdtobj, LPTSTR pszPath, UINT cchPath);
STDAPI PidlFromDataObject(IDataObject *pdtobj, LPITEMIDLIST * ppidlTarget);

// calls ShellMessageBox if SHRestricted fails the restriction
STDAPI_(BOOL) SHIsRestricted(HWND hwnd, RESTRICTIONS rest);
STDAPI_(BOOL) SafePathListAppend(LPTSTR pszDestPath, DWORD cchDestSize, LPCTSTR pszPathToAdd);

STDAPI_(BOOL) ILGetDisplayNameExA(IShellFolder *psfRoot, LPCITEMIDLIST pidl, LPSTR pszName, DWORD cchSize, int fType);
STDAPI_(BOOL) ILGetDisplayNameExW(IShellFolder *psfRoot, LPCITEMIDLIST pidl, LPWSTR pszName, DWORD cchSize, int fType);

STDAPI_(BOOL) Priv_Str_SetPtrW(WCHAR *UNALIGNED *ppwzCurrent, LPCWSTR pwzNew);

#define SEARCHNAMESPACEID_FILE_PATH             1   // Go parse it.
#define SEARCHNAMESPACEID_DOCUMENTFOLDERS       2
#define SEARCHNAMESPACEID_LOCALHARDDRIVES       3
#define SEARCHNAMESPACEID_MYNETWORKPLACES       4

STDAPI_(LPTSTR) DumpPidl(LPCITEMIDLIST pidl);

STDAPI_(BOOL) SHTrackPopupMenu(HMENU hmenu, UINT wFlags, int x, int y, int wReserved, HWND hwnd, LPCRECT lprc);
STDAPI_(HMENU) SHLoadPopupMenu(HINSTANCE hinst, UINT id);

STDAPI_(void) PathToAppPathKey(LPCTSTR pszPath, LPTSTR pszKey, int cchKey);
STDAPI_(BOOL) PathToAppPath(LPCTSTR pszPath, LPTSTR pszResult);
STDAPI_(BOOL) PathIsRegisteredProgram(LPCTSTR pszPath);

STDAPI SHGetUIObjectOf(HWND hwnd, LPTSTR pszPath, REFIID riid, void **ppvOut);

STDAPI_(HANDLE) SHGetCachedGlobalCounter(HANDLE *phCache, const GUID *pguid);
STDAPI_(void) SHDestroyCachedGlobalCounter(HANDLE *phCache);

#define GPFIDL_DEFAULT      0x0000      // normal Win32 file name
#define GPFIDL_ALTNAME      0x0001      // short file name
#define GPFIDL_NONFSNAME    0x0002      // non file system name

STDAPI_(BOOL) SHGetPathFromIDListEx(LPCITEMIDLIST pidl, LPTSTR pszPath, UINT uOpts);

// Review chrisny:  this can be moved into an object easily to handle generic droptarget, dropcursor
// , autoscrool, etc. . .
void _DragEnter(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtObject);
void _DragMove(HWND hwndTarget, const POINTL ptStart);

STDAPI FindFileOrFolders_GetDefaultSearchGUID(IShellFolder2 *psf, LPGUID pGuid);

STDAPI SavePersistHistory(IUnknown* punk, IStream* pstm);

STDAPI SEI2ICIX(LPSHELLEXECUTEINFO pei, LPCMINVOKECOMMANDINFOEX pici, LPVOID *ppvFree);
STDAPI ICIX2SEI(LPCMINVOKECOMMANDINFOEX pici, LPSHELLEXECUTEINFO pei);
STDAPI ICI2ICIX(LPCMINVOKECOMMANDINFO piciIn, LPCMINVOKECOMMANDINFOEX piciOut, LPVOID *ppvFree);
STDAPI_(BOOL) PathIsEqualOrSubFolder(LPCTSTR pszFolder, LPCTSTR pszSubFolder);
STDAPI_(BOOL) PathIsEqualOrSubFolderOf(const UINT rgFolders[], LPCTSTR pszSubFolder);
STDAPI_(LPTSTR) PathBuildSimpleRoot(int iDrive, LPTSTR pszDrive);

IProgressDialog * CProgressDialog_CreateInstance(UINT idTitle, UINT idAnimation, HINSTANCE hAnimationInst);

STDAPI_(BOOL) IsWindowInProcess(HWND hwnd);

STDAPI_(DWORD) BindCtx_GetMode(IBindCtx *pbc, DWORD grfModeDefault);

STDAPI SHCreateFileSysBindCtx(const WIN32_FIND_DATA *pfd, IBindCtx **ppbc);
STDAPI SHIsFileSysBindCtx(IBindCtx *pbc, WIN32_FIND_DATA *pfd);
STDAPI SHSimpleIDListFromFindData(LPCTSTR pszPath, const WIN32_FIND_DATA *pfd, LPITEMIDLIST *ppidl);
STDAPI SHSimpleIDListFromFindData2(IShellFolder *psf, const WIN32_FIND_DATA *pfd, LPITEMIDLIST *ppidl);
STDAPI SHCreateFSIDList(LPCTSTR pszFolder, const WIN32_FIND_DATA *pfd, LPITEMIDLIST *ppidl);

STDAPI InvokeVerbOnItems(HWND hwnd, LPCTSTR pszVerb, UINT uFlags, IShellFolder *psf, UINT cidl, LPCITEMIDLIST *apidl);
STDAPI InvokeVerbOnDataObj(HWND hwnd, LPCTSTR pszVerb, UINT uFlags, IDataObject *pdtobj);
STDAPI DeleteFilesInDataObject(HWND hwnd, UINT uFlags, IDataObject *pdtobj);
STDAPI DeleteFilesInDataObjectEx(HWND hwnd, UINT uFlags, IDataObject *pdtobj, UINT fOptions);


STDAPI GetCLSIDFromIDList(LPCITEMIDLIST pidl, CLSID *pclsid);
STDAPI GetItemCLSID(IShellFolder2 *psf, LPCITEMIDLIST pidl, CLSID *pclsid);
STDAPI_(BOOL) IsIDListInNameSpace(LPCITEMIDLIST pidl, const CLSID *pclsid);

STDAPI_(void) CleanupFileSystem();
SHSTDAPI_(HICON) SHGetFileIcon(HINSTANCE hinst, LPCTSTR pszPath, DWORD dwFileAttribute, UINT uFlags);
STDAPI GetIconLocationFromExt(IN LPTSTR pszExt, OUT LPTSTR pszIconPath, UINT cchIconPath, OUT LPINT piIconIndex);

STDAPI_(BOOL) IsMainShellProcess(); // is this the process that owns the desktop hwnd (eg the main explorer process)
STDAPI_(BOOL) IsProcessAnExplorer();
__inline BOOL IsSecondaryExplorerProcess()
{
    return (IsProcessAnExplorer() && !IsMainShellProcess());
}

STDAPI SHILAppend(LPITEMIDLIST pidlToAppend, LPITEMIDLIST *ppidl);
STDAPI SHILPrepend(LPITEMIDLIST pidlToPrepend, LPITEMIDLIST *ppidl);

//
// IDList macros and other stuff needed by the COFSFolder project
//
typedef enum {
    ILCFP_FLAG_NORMAL           = 0x0000,
    ILCFP_FLAG_SKIPJUNCTIONS    = 0x0001,
} ILCFP_FLAGS;

STDAPI ILCreateFromPathEx(LPCTSTR pszPath, IUnknown *punkToSkip, ILCFP_FLAGS dwFlags, LPITEMIDLIST *ppidl, DWORD *rgfInOut);
STDAPI_(BOOL) ILIsParent(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL fImmediate);

STDAPI_(BOOL) SHSkipJunctionBinding(IBindCtx *pbc, const CLSID *pclsidSkip);
STDAPI SHCreateSkipBindCtx(IUnknown *punkToSkip, IBindCtx **ppbc);

STDAPI_(void) SetUnknownOnSuccess(HRESULT hres, IUnknown *punk, IUnknown **ppunkToSet);
STDAPI SHCacheTrackingFolder(LPCITEMIDLIST pidlRoot, int csidlTarget, IShellFolder2 **ppsfCache);
#define MAKEINTIDLIST(csidl)    (LPCITEMIDLIST)MAKEINTRESOURCE(csidl)

STDAPI_(BOOL) PathIsShortcut(LPCTSTR psz);

typedef struct _ICONMAP
{
    UINT uType;                  // SHID_ type
    UINT indexResource;          // Resource index (of SHELL232.DLL)
} ICONMAP, *LPICONMAP;

STDAPI_(UINT) SILGetIconIndex(LPCITEMIDLIST pidl, const ICONMAP aicmp[], UINT cmax);

STDAPI_(LPITEMIDLIST) VariantToIDList(const VARIANT *pv);
STDAPI VariantToBuffer(const VARIANT *pvar, void *pv, UINT cb);
STDAPI VariantToGUID(const VARIANT *pvar, GUID *pguid);
STDAPI VariantToStrRet(const VARIANT *pv, STRRET *pstret);
STDAPI_(LPTSTR) VariantToStr(const VARIANT *pvar, LPTSTR pszBuf, int cchBuf);
STDAPI_(LPCWSTR) VariantToStrW(const VARIANT *pvar);

STDAPI InitVariantFromBuffer(VARIANT *pvar, const void *pv, UINT cb);
STDAPI InitVariantFromIDList(VARIANT* pvar, LPCITEMIDLIST pidl);
STDAPI InitVariantFromGUID(VARIANT *pvar, GUID *pguid);
STDAPI InitVariantFromStr(VARIANT *pvar, LPCTSTR pstr);
STDAPI InitVariantFromStrRet(STRRET *pstrret, LPCITEMIDLIST pidl, VARIANT *pv);

BOOL IsSelf(UINT cidl, LPCITEMIDLIST *apidl);

//
// Context menu helper functions
//
STDAPI_(UINT) GetMenuIndexForCanonicalVerb(HMENU hMenu, IContextMenu* pcm, UINT idCmdFirst, LPCWSTR pwszVerb);

BOOL AllowedToEncrypt();

#ifdef __cplusplus
#define IsEqualSCID(a, b)   (((a).pid == (b).pid) && IsEqualIID((a).fmtid, (b).fmtid) )
#else
#define IsEqualSCID(a, b)   (((a).pid == (b).pid) && IsEqualIID(&((a).fmtid),&((b).fmtid)))
#endif
//
//  Helper function for defview callbacks.
//
STDAPI_(LPCITEMIDLIST) GetSelectedObjectFromSite(IUnknown *psite);

STDAPI SHFindFirstFile(LPCTSTR pszPath, WIN32_FIND_DATA *pfd, HANDLE *phfind);
STDAPI SHFindFirstFileRetry(HWND hwnd, IUnknown *punkEnableModless, LPCTSTR pszPath, WIN32_FIND_DATA *pfd, HANDLE *phfind, DWORD dwFlags);
STDAPI_(UINT) SHEnumErrorMessageBox(HWND hwnd, UINT idTemplate, DWORD err, LPCTSTR pszParam, BOOL fNet, UINT dwFlags);

LPSTR _ConstructMessageStringA(HINSTANCE hInst, LPCSTR pszMsg, va_list *ArgList);
LPWSTR _ConstructMessageStringW(HINSTANCE hInst, LPCWSTR pszMsg, va_list *ArgList);
#ifdef UNICODE
#define _ConstructMessageString _ConstructMessageStringW
#else
#define _ConstructMessageString _ConstructMessageStringA
#endif


// TransferDelete() fOptions flags
#define SD_USERCONFIRMATION      0x0001
#define SD_SILENT                0x0002
#define SD_NOUNDO                0x0004
#define SD_WARNONNUKE            0x0008 // we pass this for drag-drop on recycle bin in case something is really going to be deleted

STDAPI_(void) TransferDelete(HWND hwnd, HDROP hDrop, UINT fOptions);

STDAPI LoadFromFile(const CLSID *pclsid, LPCTSTR pszFile, REFIID riid, void **ppv);

STDAPI_(BOOL) App_IsLFNAware(LPCTSTR pszFile);

//
// SH(Get/Set)IniStringUTF7
//
// These are just like Get/WriteProfileString except that if the KeyName
// begins with SZ_CANBEUNICODE, we will use SHGetIniString instead of
// the profile functions.  (The SZ_CANBEUNICODE will be stripped off
// before calling SHGetIniString.)  This allows us to stash unicode
// strings into INI files (which are ASCII) by encoding them as UTF7.
//
// In other words, SHGetIniStringUTF7("Settings", SZ_CANBEUNICODE "Name", ...)
// will read from section "Settings", key name "Name", but will also
// look at the UTF7-encoded version stashed in the "Settings.W" section.
//
#ifdef UNICODE
#define SZ_CANBEUNICODE     TEXT("@")
#define CH_CANBEUNICODE     TEXT('@')
STDAPI_(DWORD) SHGetIniStringUTF7(LPCWSTR lpSection, LPCWSTR lpKey, LPWSTR lpBuf, DWORD nSize, LPCWSTR lpFile);
STDAPI_(BOOL) SHSetIniStringUTF7(LPCWSTR lpSection, LPCWSTR lpKey, LPCWSTR lpString, LPCWSTR lpFile);
#else
#define SZ_CANBEUNICODE     TEXT("")
#define SHGetIniStringUTF7(lpSection, lpKey, lpBuf, nSize, lpFile) \
  GetPrivateProfileStringA(lpSection, lpKey, "", lpBuf, nSize, lpFile)
#define SHSetIniStringUTF7 WritePrivateProfileStringA
#endif

STDAPI_(BOOL) ShowSuperHidden();
STDAPI_(void) ReplaceDlgIcon(HWND hDlg, UINT id, HICON hIcon);
STDAPI_(LONG) GetOfflineShareStatus(LPCTSTR pcszPath);

HRESULT SHGetSetFolderSetting(LPCTSTR pszIniFile, DWORD dwReadWrite, LPCTSTR pszSection,
        LPCTSTR pszKey, LPTSTR pszValue, DWORD cchValueSize);
HRESULT SHGetSetFolderSettingPath(LPCTSTR pszIniFile, DWORD dwReadWrite, LPCTSTR pszSection,
        LPCTSTR pszKey, LPTSTR pszValue, DWORD cchValueSize);
void ExpandOtherVariables(LPTSTR pszFile, int cch);
void SubstituteWebDir(LPTSTR pszFile, int cch);

#endif // _UTIL_H
