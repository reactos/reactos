/*
 * ishcut.h - Internet Shortcut class implementation description.
 */

#ifndef _INTSHCUT_HPP_
#define _INTSHCUT_HPP_

#include "urlprop.h"
#include "subsmgr.h"
#include "cowsite.h"
//
// Define this to enable the integrated history database
//
#define USE_NEW_HISTORYDATA

#ifdef __cplusplus

/* Types
 ********/

// Intshcut flags

#define ISF_DEFAULT             0x00000000
#define ISF_DIRTY               0x00000001      // URL is dirty
#define ISF_DESKTOP             0x00000002      // Located on the desktop
#define ISF_FAVORITES           0x00000004      // Located in favorites folder
#define ISF_WATCH               0x00000008      // Scratch flag for context menu
#define ISF_SPECIALICON         0x00000010      // Icon is munged for splat
#define ISF_CODEPAGE            0x00000020      // Code page is set
#define ISF_ALL                 0x0000003F


// Intshcut Shell extension

class Intshcut : public IDataObject,
                 public IContextMenu2,
                 public IExtractIconA,
                 public IExtractIconW,
                 public IPersistFile,
                 public IPersistStream,
                 public IShellExtInit,
                 public IShellLinkA,
                 public IShellLinkW,
                 public IShellPropSheetExt,
                 public IPropertySetStorage,
                 public INewShortcutHookA,
                 public INewShortcutHookW,
                 public IUniformResourceLocatorA,
                 public IUniformResourceLocatorW,
                 public IQueryInfo,
                 public IQueryCodePage,
                 public CObjectWithSite, 
                 public INamedPropertyBag,
                 public IOleCommandTarget
{


private:

    LONG        m_cRef;
    DWORD       m_dwFlags;              // ISF_* flags
    LPTSTR      m_pszFile;              // Name of internet shortcut
    LPTSTR      m_pszFileToLoad ;        // Name of Internet Shortcut that was 
    IntshcutProp *m_pprop;              // Internal properties
    IntsiteProp  *m_psiteprop;          // Internet Site properties
    LPTSTR      m_pszFolder;            // Used by INewShortcutHook
    UINT        m_uiCodePage;           // Used by IQueryCodePage -- sendmail.dll for send current document
    BOOL        m_bCheckForDelete;      // Used to see if we need to delete a subscription if the
                                        // shortcut is deleted.
    BOOL        m_fMustLoadSync;        // Set to TRUE if any interface other than IPersistFile or
                                        // IExtractIconW/A are given out
    BOOL        m_fProbablyDefCM;       // this shortcut was most likely init'd by defcm
    
    IDataObject *m_pInitDataObject;
    LPTSTR      m_pszTempFileName;      // temporary file to be deleted when ishcut goes away
    LPTSTR      m_pszDescription;
    IUnknown   *_punkLink;                   //  for file: URLs

    STDMETHODIMP InitProp(void);
    STDMETHODIMP InitSiteProp(void);
    STDMETHODIMP OnReadOffline(void);
    STDMETHODIMP OnWatch(void);
    STDMETHODIMP MirrorProperties(void);

    // data transfer methods

    STDMETHODIMP_(DWORD) GetFileContentsAndSize(LPSTR *ppsz);
    STDMETHODIMP TransferUniformResourceLocator(FORMATETC *pfmtetc, STGMEDIUM *pstgmed);
    STDMETHODIMP TransferText(FORMATETC *pfmtetc, STGMEDIUM *pstgmed);
    STDMETHODIMP TransferFileGroupDescriptorA(FORMATETC *pfmtetc, STGMEDIUM *pstgmed);
    STDMETHODIMP TransferFileGroupDescriptorW(FORMATETC *pfmtetc, STGMEDIUM *pstgmed);
    STDMETHODIMP TransferFileContents(FORMATETC *pfmtetc, STGMEDIUM *pstgmed);
    STDMETHODIMP GetDocumentStream(IStream **ppstm);
    STDMETHODIMP GetDocumentName(LPTSTR pszName);

    // protocol registration methods

    STDMETHODIMP RegisterProtocolHandler(HWND hwndParent, LPTSTR pszAppBuf, UINT ucAppBufLen);

    HRESULT _Extract(LPCTSTR pszIconFile, UINT iIcon, HICON * phiconLarge, HICON * phiconSmall, UINT ucIconSize);
    HRESULT _GetIconLocation(UINT uFlags, LPWSTR pszIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags);

    ~Intshcut(void);    // Prevent this class from being allocated on the stack or it will fault.

public:
    Intshcut(void);

    // IDataObject methods

    STDMETHODIMP GetData(FORMATETC *pfmtetcIn, STGMEDIUM *pstgmed);
    STDMETHODIMP GetDataHere(FORMATETC *pfmtetc, STGMEDIUM *pstgpmed);
    STDMETHODIMP QueryGetData(FORMATETC *pfmtetc);
    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pfmtetcIn, FORMATETC *pfmtetcOut);
    STDMETHODIMP SetData(FORMATETC *pfmtetc, STGMEDIUM *pstgmed, BOOL bRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppienumFormatEtc);
    STDMETHODIMP DAdvise(FORMATETC *pfmtetc, DWORD dwAdviseFlags, IAdviseSink * piadvsink, PDWORD pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppienumStatData);

    // IExtractIconA methods

    STDMETHODIMP GetIconLocation(UINT uFlags, LPSTR pszIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags);
    STDMETHODIMP Extract(LPCSTR pcszFile, UINT uIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT ucIconSize);

    // IExtractIconW methods

    STDMETHODIMP GetIconLocation(UINT uFlags, LPWSTR pszIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags);
    STDMETHODIMP Extract(LPCWSTR pcszFile, UINT uIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT ucIconSize);

    // INewShortcutHookA methods

    STDMETHODIMP SetReferent(LPCSTR pcszReferent, HWND hwndParent);
    STDMETHODIMP GetReferent(LPSTR pszReferent, int ncReferentBufLen);
    STDMETHODIMP SetFolder(LPCSTR pcszFolder);
    STDMETHODIMP GetFolder(LPSTR pszFolder, int ncFolderBufLen);
    STDMETHODIMP GetName(LPSTR pszName, int ncNameBufLen);
    STDMETHODIMP GetExtension(LPSTR pszExtension, int ncExtensionBufLen);

    // INewShortcutHookW methods

    STDMETHODIMP SetReferent(LPCWSTR pcszReferent, HWND hwndParent);
    STDMETHODIMP GetReferent(LPWSTR pszReferent, int ncReferentBufLen);
    STDMETHODIMP SetFolder(LPCWSTR pcszFolder);
    STDMETHODIMP GetFolder(LPWSTR pszFolder, int ncFolderBufLen);
    STDMETHODIMP GetName(LPWSTR pszName, int ncNameBufLen);
    STDMETHODIMP GetExtension(LPWSTR pszExtension, int ncExtensionBufLen);

    // IPersist methods

    STDMETHODIMP GetClassID(CLSID *pclsid);

    // IPersistFile methods

    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Save(LPCOLESTR pcwszFileName, BOOL bRemember);
    STDMETHODIMP SaveCompleted(LPCOLESTR pcwszFileName);
    STDMETHODIMP Load(LPCOLESTR pcwszFileName, DWORD dwMode);
    STDMETHODIMP GetCurFile(LPOLESTR *ppwszFileName);

    // IPersistStream methods

    STDMETHODIMP Save(IStream * pistr, BOOL bClearDirty);
    STDMETHODIMP Load(IStream * pistr);
    STDMETHODIMP GetSizeMax(PULARGE_INTEGER pcbSize);

    // IShellExtInit methods

    STDMETHODIMP Initialize(LPCITEMIDLIST pcidlFolder, IDataObject * pidobj, HKEY hkeyProgID);

    // IShellLink methods

    STDMETHODIMP SetPath(LPCSTR pcszPath);
    STDMETHODIMP GetPath(LPSTR pszFile, int ncFileBufLen, PWIN32_FIND_DATAA pwfd, DWORD dwFlags);
    STDMETHODIMP SetRelativePath(LPCSTR pcszRelativePath, DWORD dwReserved);
    STDMETHODIMP SetIDList(LPCITEMIDLIST pcidl);
    STDMETHODIMP GetIDList(LPITEMIDLIST *ppidl);
    STDMETHODIMP SetDescription(LPCSTR pcszDescription);
    STDMETHODIMP GetDescription(LPSTR pszDescription, int ncDesciptionBufLen);
    STDMETHODIMP SetArguments(LPCSTR pcszArgs);
    STDMETHODIMP GetArguments(LPSTR pszArgs, int ncArgsBufLen);
    STDMETHODIMP SetWorkingDirectory(LPCSTR pcszWorkingDirectory);
    STDMETHODIMP GetWorkingDirectory(LPSTR pszWorkingDirectory, int ncbLen);
    STDMETHODIMP SetHotkey(WORD wHotkey);
    STDMETHODIMP GetHotkey(PWORD pwHotkey);
    STDMETHODIMP SetShowCmd(int nShowCmd);
    STDMETHODIMP GetShowCmd(PINT pnShowCmd);
    STDMETHODIMP SetIconLocation(LPCSTR pcszIconFile, int niIcon);
    STDMETHODIMP GetIconLocation(LPSTR pszIconFile, int ncbLen, PINT pniIcon);
    STDMETHODIMP Resolve(HWND hwnd, DWORD dwFlags);

    // IShellLinkW functions that change from the A functions...
    STDMETHODIMP SetPath(LPCWSTR pcszPath);
    STDMETHODIMP GetPath(LPWSTR pszFile, int ncFileBufLen, PWIN32_FIND_DATAW pwfd, DWORD dwFlags);
    STDMETHODIMP SetRelativePath(LPCWSTR pcszRelativePath, DWORD dwReserved);
    STDMETHODIMP SetDescription(LPCWSTR pcszDescription);
    STDMETHODIMP GetDescription(LPWSTR pszDescription, int ncDesciptionBufLen);
    STDMETHODIMP SetArguments(LPCWSTR pcszArgs);
    STDMETHODIMP GetArguments(LPWSTR pszArgs, int ncArgsBufLen);
    STDMETHODIMP SetWorkingDirectory(LPCWSTR pcszWorkingDirectory);
    STDMETHODIMP GetWorkingDirectory(LPWSTR pszWorkingDirectory, int ncbLen);
    STDMETHODIMP SetIconLocation(LPCWSTR pcszIconFile, int niIcon);
    STDMETHODIMP GetIconLocation(LPWSTR pszIconFile, int ncbLen, PINT pniIcon);

    // IShellPropSheetExt methods

    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
    STDMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam);

    // IContextMenu methods

    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(IN LPCMINVOKECOMMANDINFO pici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT * puReserved, LPSTR pszName, UINT cchMax);
    STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IUniformResourceLocatorA methods

    STDMETHODIMP SetURL(LPCSTR pcszURL, DWORD dwFlags);
    STDMETHODIMP GetURL(LPSTR *ppszURL);
    STDMETHODIMP InvokeCommand(PURLINVOKECOMMANDINFOA purlici);
    
    // IUniformResourceLocatorW methods

    STDMETHODIMP SetURL(LPCWSTR pcszURL, DWORD dwFlags);
    STDMETHODIMP GetURL(LPWSTR *ppszURL);
    STDMETHODIMP InvokeCommand(PURLINVOKECOMMANDINFOW purlici);
    
    // IPropertySetStorage methods

    STDMETHODIMP Create(REFFMTID fmtid, const CLSID * pclsid, DWORD grfFlags, DWORD grfMode, IPropertyStorage** ppPropStg);
    STDMETHODIMP Open(REFFMTID fmtid, DWORD grfMode, IPropertyStorage** ppPropStg);
    STDMETHODIMP Delete(REFFMTID fmtid);
    STDMETHODIMP Enum(IEnumSTATPROPSETSTG** ppenum);

    // IQueryInfo methods

    STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip);
    STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);

    // IOleCommandTarget
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup,
                                ULONG cCmds, MSOCMD rgCmds[], MSOCMDTEXT *pcmdtext);
    STDMETHODIMP Exec(const GUID *pguidCmdGroup,
                        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
                        
    // IQueryCodePage methods 
    // Purpose: This is a hack to use the URL to store the codepage for
    // send currenct document and pass it to sendmail.dll
    STDMETHODIMP GetCodePage(UINT * puiCodePage);
    STDMETHODIMP SetCodePage(UINT uiCodePage);

    // *** IObjectWithSite methods from CObjectWithSite***
    /*
    virtual STDMETHODIMP SetSite(IUnknown *pUnkSite);        
    virtual STDMETHODIMP GetSite(REFIID riid, void **ppvSite);
    */

    // INamedPropertyBag Methods
    STDMETHODIMP ReadPropertyNPB(/* [in] */ LPCOLESTR pszSectionname, 
                                       /* [in] */ LPCOLESTR pszPropName, 
                                       /* [out] */ PROPVARIANT *pVar);
                            
    STDMETHODIMP WritePropertyNPB(/* [in] */ LPCOLESTR pszSectionname, 
                                        /* [in] */ LPCOLESTR pszPropName, 
                                        /* [in] */ PROPVARIANT  *pVar);


    STDMETHODIMP RemovePropertyNPB (/* [in] */ LPCOLESTR pszBagname,
                                    /* [in] */ LPCOLESTR pszPropName);
    
    // IUnknown methods
    STDMETHODIMP  QueryInterface(REFIID riid, PVOID *ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // other methods
    
    STDMETHODIMP SaveToFile(LPCTSTR pcszFile, BOOL bRemember);
    STDMETHODIMP LoadFromFile(LPCTSTR pcszFile);
    STDMETHODIMP LoadFromAsyncFileNow();
    STDMETHODIMP GetCurFile(LPTSTR pszFile, UINT ucbLen);
    STDMETHODIMP Dirty(BOOL bDirty);
    STDMETHODIMP GetURLIconLocation(UINT uInFlags, LPTSTR pszBuf, UINT cchBuf, int * pniIcon, BOOL fRecentlyChanged, OUT PUINT  puOutFlags);
    
    STDMETHODIMP GetIconLocationFromFlags(UINT uInFlags, LPTSTR pszIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags, DWORD dwPropFlags);
    STDMETHODIMP_(void) ChangeNotify(LONG wEventId, UINT uFlags);
    STDMETHODIMP GetIDListInternal(LPITEMIDLIST *ppidl);
    STDMETHODIMP GetURLW(WCHAR **ppszURL);
    BOOL ExtractIconFromWininetCache(IN  LPCTSTR pszIconString, 
                                     IN  UINT iIcon, 
                                     OUT HICON * phiconLarge, 
                                     OUT HICON * phiconSmall, 
                                     IN  UINT ucIconSize,
                                     BOOL *pfFoundUrl,
                                     DWORD dwPropFlags);
    STDMETHODIMP _GetIconLocationWithURLHelper(IN  LPTSTR pszBuf,
                                               IN  int    cchBuf,
                                               OUT PINT   pniIcon,
                                               IN  LPTSTR pszActualUrl,
                                               UINT cchUrlBufSize,
                                               BOOL fRecentlyChanged);

    STDMETHODIMP _DoIconDownload();
    STDMETHODIMP _SaveOffPersistentDataFromSite();
    STDMETHODIMP _CreateTemporaryBackingFile();
    STDMETHODIMP _SetTempFileName(TCHAR *pszTempFileName);
    STDMETHODIMP _ComputeDescription();
    STDMETHODIMP_(BOOL) _IsInFavoritesFolder();
    IDataObject *GetInitDataObject() { ASSERT(m_pInitDataObject); return m_pInitDataObject; }
    STDMETHODIMP_(BOOL)_TryLink(REFIID riid, void **ppvOut);
    STDMETHODIMP _CreateShellLink(LPCTSTR pszPath, IUnknown **ppunk);


    // Query methods

    STDMETHODIMP_(DWORD) GetScheme(void);

#ifdef DEBUG
    STDMETHODIMP_(void) Dump(void);
    friend BOOL IsValidPCIntshcut(const Intshcut *pcintshcut);
#endif
};

typedef Intshcut * PIntshcut;
typedef const Intshcut CIntshcut;
typedef const Intshcut * PCIntshcut;



/* Prototypes
 *************/

// isbase.cpp

HRESULT ValidateURL(LPCTSTR pcszURL);

HRESULT IsProtocolRegistered(LPCTSTR pcszProtocol);

BOOL    AnyMeatW(LPCWSTR pcsz);
BOOL    AnyMeatA(LPCSTR pcsz);
#ifdef UNICODE
#define AnyMeat     AnyMeatW
#else
#define AnyMeat     AnyMeatA
#endif

BOOL    IsWebsite(Intshcut * pintshcut);

#define ISHCUT_INISTRING_SECTION      TEXT("InternetShortcut")
#define ISHCUT_INISTRING_SECTIONW         L"InternetShortcut"
#define ISHCUT_INISTRING_URL          TEXT("URL")
#define ISHCUT_INISTRING_WORKINGDIR   TEXT("WorkingDirectory")
#define ISHCUT_INISTRING_WHATSNEW     TEXT("WhatsNew")
#define ISHCUT_INISTRING_AUTHOR       TEXT("Author")
#define ISHCUT_INISTRING_DESC         TEXT("Desc")
#define ISHCUT_INISTRING_COMMENT      TEXT("Comment")
#define ISHCUT_INISTRING_MODIFIED     TEXT("Modified")
#define ISHCUT_INISTRING_ICONINDEX    TEXT("IconIndex")
#define ISHCUT_INISTRING_ICONINDEXW       L"IconIndex"
#define ISHCUT_INISTRING_ICONFILE     TEXT("IconFile")
#define ISHCUT_INISTRING_ICONFILEW         L"IconFile"

#define ISHCUT_DEFAULT_FAVICONW            L"favicon.ico";
#define ISHCUT_DEFAULT_FAVICONATROOTW      L"/favicon.ico";


HRESULT 
GetGenericURLIcon(
    LPTSTR pszIconFile,
    UINT cchIconFile, 
    PINT pniIcon);


struct IS_SUBS_DEL_DATA
{
    TCHAR m_szFile[MAX_PATH];
    LPWSTR m_pwszURL;

    ~IS_SUBS_DEL_DATA()
    {
        if (m_pwszURL)
        {
            SHFree(m_pwszURL);
        }
    }
};

#endif  // __cplusplus


//
// Prototypes for all modules
//

STDAPI  CopyURLProtocol(LPCTSTR pcszURL, LPTSTR * ppszProtocol, PARSEDURL * ppu);

#endif  // _INTSHCUT_HPP_
