// this file implements shell command files.
// .scf
// when executed, they run a shell internal command.
// they can have stream storage, or whatnot in them
//

// file format is *PURPOSELY* text so that folks can create and modify by hand

#include "stdafx.h"
#pragma hdrstop

//#include "cmdfile.h"
//#include "clsobj.h"
//#include "cmdfile.h"
//#include "desktop.h" // for DTM_BANDFILE message to send to desktop window
//#include <trayp.h>

#define SZDESKBAR TEXT("DeskBar")


LPBYTE GetProfileData(LPCTSTR pszSection, LPCTSTR pszSizeValue, LPCTSTR pszDataValue, LPCTSTR pszFile, UINT *puSize)
{
    LPBYTE p = NULL;
    
    if (!pszSizeValue)
        pszSizeValue = TEXT("size");
    
    UINT uSize = GetPrivateProfileInt(pszSection, pszSizeValue, 0, pszFile);
    if (uSize) {
        // use LocalAlloc so that we can strictly pass it to CreateStreamOnHLocal
        p = (LPBYTE)LocalAlloc(LPTR, uSize);
        
        if (!GetPrivateProfileStruct(pszSection, pszDataValue, p, uSize, pszFile)) {
            LocalFree(p);
            p = NULL;
        }
    }    
    
    if (puSize)
        *puSize = uSize;
    return p;
}

IStream* GetProfileStream(LPCTSTR pszSection, LPCTSTR pszSizeValue, LPCTSTR pszDataValue, LPCTSTR pszFile)
{   
    UINT uSize;
    LPBYTE p = GetProfileData(pszSection, pszSizeValue, pszDataValue, pszFile, &uSize);
    IStream* pstm = NULL;
    if (p) {

        pstm = SHCreateMemStream(p, uSize);
        LocalFree(p);
    }
    return pstm;
}

HWND GetInProcDesktop()
{
    HWND hwndDesktop = GetShellWindow();    // This is the "normal" desktop
    
    if (IsDesktopWindow(hwndDesktop))
    {
        DWORD dwProcID;
        
        GetWindowThreadProcessId(hwndDesktop, &dwProcID);
        if (GetCurrentProcessId() == dwProcID) {
            return hwndDesktop;
        }
    }
    
    return NULL;
}



IUnknown* CreateBandFromFile(LPCTSTR pszFile)
{
    TCHAR szClass[80];
    IUnknown* punk = NULL;
    if (!GetPrivateProfileString(SZDESKBAR, TEXT("CLSID"), TEXT(""), szClass, ARRAYSIZE(szClass), pszFile)) {
        
        // if not found, get the stream and do OleLoadFromStream
        IStream* pstm = GetProfileStream(SZDESKBAR, NULL, TEXT("Stream"), pszFile);
        
        if (pstm) {
            OleLoadFromStream(pstm, IID_IUnknown, (LPVOID*)&punk);
            pstm->Release();
        }
    } else {
        CLSID clsid;
        
        if (SHCLSIDFromString(szClass, &clsid))
        {
            if (SUCCEEDED(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&punk))) {
                IStream* pstm = GetProfileStream(SZDESKBAR, NULL, TEXT("Stream"), pszFile);
                if (pstm) {
                    IPersistStream *ppstm;
                    punk->QueryInterface(IID_IPersistStream, (LPVOID*)&ppstm);
                    if (ppstm) {
                        ppstm->Load(pstm);
                        ppstm->Release();
                    }
                    pstm->Release();
                }
            }
        }
        
    }
    
    return punk;
}

void SFC_IECommand(LPCTSTR pszFile)
{
    // ANSI!
    TCHAR szCommand[40];
    
    if (GetPrivateProfileString(TEXT("IE"), TEXT("Command"), TEXT(""), szCommand, ARRAYSIZE(szCommand), pszFile)) {
        if (!lstrcmpi(szCommand, TEXT("Channels"))
            && !SHRestricted2W(REST_NoChannelUI, NULL, 0))
        {
            Channel_QuickLaunch();
        }
    }
}
   

void SFC_BandFile(LPCTSTR pszFile)
{
    HWND hwndDesktop = GetInProcDesktop();
    
    if (!hwndDesktop) 
        return;
    
    LPTSTR psz = StrDup(pszFile);

    if (!PostMessage(hwndDesktop, DTM_BANDFILE, 0, (LPARAM)psz))
        LocalFree(psz);
}

#if 0
void SFC_DesktopCommand(LPCTSTR pszFile)
{
    UINT uID = GetPrivateProfileInt(TEXT("Desktop"), TEXT("Command"), 0, pszFile);

    if (uID) {
        HWND hwndDesktop = GetInProcDesktop();
        PostMessage(hwndDesktop, uID, 0, 0);
    }
}
#endif

extern HWND g_hwndTray; // desktop.cpp
void SFC_TrayCommand(LPCTSTR pszFile)
{
    // ANSI!
    char szFile[MAX_PATH];
    char szCommand[40];

    SHTCharToAnsi(pszFile, szFile, ARRAYSIZE(szFile));
    
    ASSERT(g_hwndTray);
    if (GetPrivateProfileStringA("Taskbar", "Command", "", szCommand, ARRAYSIZE(szCommand), szFile)) {
        LPSTR psz = StrDupA(szCommand);
        if (psz) {
            
            if (!PostMessage(g_hwndTray, TM_PRIVATECOMMAND, 0, (LPARAM)psz))
                LocalFree(psz);
        }
    }
}

enum {
    SCFIDM_NOCOMMAND      = 0,
    SCFIDM_BANDFILE       = 1,
    SCFIDM_TRAYCOMMAND    = 2,
    SCFIDM_IE             = 3,
};

typedef void (*SFCCOMMAND)(LPCTSTR lpszBuf);
typedef struct _EXECCOMMADNINFO
{
    UINT id;
    SFCCOMMAND pfn;
} EXECCOMMANDINFO;

EXECCOMMANDINFO const c_sCmdInfo[] = 
{
    
    { SCFIDM_BANDFILE, SFC_BandFile},
    { SCFIDM_TRAYCOMMAND, SFC_TrayCommand},
    { SCFIDM_IE, SFC_IECommand},
};

void ShellExecCommandFile(LPITEMIDLIST pidl)
{
    TCHAR szFile[MAX_PATH];

    if (SHGetPathFromIDList(pidl, szFile)) {
        UINT uID;
        
        uID = GetPrivateProfileInt(TEXT("Shell"), TEXT("Command"), 0, szFile);
        
        if (uID) {
            int i;
            
            for ( i = 0; i < ARRAYSIZE(c_sCmdInfo); i++) {
                if (uID == c_sCmdInfo[i].id) {
                    c_sCmdInfo[i].pfn(szFile);
                    break;
                }
            }
        }
    }
}


class CCmdFileIcon : public IExtractIconA, public IExtractIconW, public IPersistFile
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef) () ;
    STDMETHOD_(ULONG,Release) ();

    // *** IExtractIconA methods ***
    STDMETHOD(GetIconLocation)(
                         UINT   uFlags,
                         LPSTR  szIconFile,
                         UINT   cchMax,
                         int   * piIndex,
                         UINT  * pwFlags);

    STDMETHOD(Extract)(
                           LPCSTR pszFile,
                           UINT   nIconIndex,
                           HICON   *phiconLarge,
                           HICON   *phiconSmall,
                           UINT    nIconSize) {return S_FALSE;};

    // *** IExtractIconW methods ***
    STDMETHOD(GetIconLocation)(
                         UINT   uFlags,
                         LPWSTR  szIconFile,
                         UINT   cchMax,
                         int   * piIndex,
                         UINT  * pwFlags);

    STDMETHOD(Extract)(
                           LPCWSTR pszFile,
                           UINT   nIconIndex,
                           HICON   *phiconLarge,
                           HICON   *phiconSmall,
                           UINT    nIconSize) {return S_FALSE;};

    // IPersist methods

    STDMETHODIMP GetClassID(CLSID *pclsid) { *pclsid = CLSID_CmdFileIcon; return S_OK;};
    
    // IPersistFile methods

    STDMETHODIMP IsDirty(void) {return S_FALSE;};
    STDMETHODIMP Save(LPCOLESTR pcwszFileName, BOOL bRemember) {return S_OK;};
    STDMETHODIMP SaveCompleted(LPCOLESTR pcwszFileName){return S_OK;};
    STDMETHODIMP Load(LPCOLESTR pcwszFileName, DWORD dwMode);
    STDMETHODIMP GetCurFile(LPOLESTR *ppwszFileName);
    
    CCmdFileIcon() { _cRef = 1; DllAddRef(); };
    ~CCmdFileIcon() { DllRelease(); };
protected:

    UINT  _cRef;
    TCHAR _szFile[MAX_PATH];
};


ULONG CCmdFileIcon::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CCmdFileIcon::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


HRESULT CCmdFileIcon::GetIconLocation( UINT   uFlags,
                                    LPTSTR  szIconFile,
                                    UINT    cchMax,
                                    int   * piIndex,
                                    UINT  * pwFlags)
{
    TCHAR szData[MAX_PATH + 80];

    if (_szFile[0]) {
        *pwFlags = 0;
        *piIndex = 0;
        szIconFile[0] = 0;

        GetPrivateProfileString(TEXT("Shell"), TEXT("IconFile"), TEXT(""), szData, ARRAYSIZE(szData), _szFile);
        
        *piIndex = PathParseIconLocation(szData);
        lstrcpyn(szIconFile, szData, MAX_PATH);
        return S_OK;
    }
    
    return E_FAIL;
}

#ifdef UNICODE
HRESULT CCmdFileIcon::GetIconLocation( UINT   uFlags,
                                    LPSTR  szIconFile,
                                    UINT   cchMax,
                                    int   * piIndex,
                                    UINT  * pwFlags)
{
    WCHAR szAnsiIconPath[MAX_PATH];
    HRESULT hres;
    
    if (SUCCEEDED(hres = GetIconLocation(uFlags, szAnsiIconPath, MAX_PATH, piIndex, pwFlags)))
    {
        SHUnicodeToAnsi(szAnsiIconPath, szIconFile, cchMax);
    }
    
    return hres;
}
#else
HRESULT CCmdFileIcon::GetIconLocation( UINT   uFlags,
                                    LPWSTR  szIconFile,
                                    UINT   cchMax,
                                    int   * piIndex,
                                    UINT  * pwFlags)
{
    CHAR szAnsiIconPath[MAX_PATH];
    HRESULT hres;
    
    if (SUCCEEDED(hres = GetIconLocation(uFlags, szAnsiIconPath, MAX_PATH, piIndex, pwFlags)))
    {
        SHAnsiToUnicode(szAnsiIconPath, szIconFile, cchMax);
    }
    
    return hres;
}
#endif


// IPersistFile::Load handler for Intshcut 

STDMETHODIMP CCmdFileIcon::Load(LPCOLESTR pwszFile, DWORD dwMode)
{
    SHUnicodeToTChar(pwszFile, _szFile, ARRAYSIZE(_szFile));
    return S_OK;
}

STDMETHODIMP CCmdFileIcon::GetCurFile(LPOLESTR *ppwszFileName)
{
    SHTCharToUnicode(_szFile, *ppwszFileName, MAX_PATH);
    return S_OK;
}

HRESULT CCmdFileIcon::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IExtractIconA))
    {
        *ppvObj = SAFECAST(this, IExtractIconA*);
    } 
    else if (IsEqualIID(riid, IID_IExtractIconW))
    {
        *ppvObj = SAFECAST(this, IExtractIconW*);
    } 
    else if (IsEqualIID(riid, IID_IPersistFile) ||
            IsEqualIID(riid, IID_IPersist))
    {
        *ppvObj = SAFECAST(this, IPersistFile*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CCmdFileIcon_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppunk)
{
    HRESULT hres;
    IUnknown* pObj;

    pObj = (IExtractIcon*)new CCmdFileIcon();
    if (pObj)
    {
        hres = pObj->QueryInterface(riid, ppunk);
        pObj->Release();
    }
    else
    {
        *ppunk = NULL;
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

