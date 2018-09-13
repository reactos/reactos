//
// Defines IETHREADPARAM for shdocvw.dll and explorer.exe
//

#ifndef __IETHREAD_H__
#define __IETHREAD_H__

#include <desktopp.h>

typedef struct
{
    DWORD   dwSize;
    UINT    uFlags;
    int     nShow;
    HWND    hwndCaller;
    DWORD   dwHotKey;
    CLSID   clsid;
    CLSID   clsidInProc;
    UINT    oidl;               // Offset to pidl or 0
    UINT    oidlSelect;         // Offset to pidl or 0
    UINT    oidlRoot;           // Offset to pidl or 0
    UINT    opszPath;           // Offset to path or 0
} NEWFOLDERBLOCK, *PNEWFOLDERBLOCK;

typedef struct _WINVIEW
{
    BOOL UNUSED:1;      // unused
    BOOL bStdButtons:1; // Win95 called this bToolbar
    BOOL bStatusBar:1;  // Win95
    BOOL bLinks:1;      // IE3 called this bITBar
    BOOL bAddress:1;    // IE4
} WINVIEW;


// the size in characters of the name of the Event used to signal IEXPLORE
#define MAX_IEEVENTNAME (2+1+8+1+8+1)

#undef  INTERFACE
#define INTERFACE   IEFreeThreadedHandShake
DECLARE_INTERFACE_(IEFreeThreadedHandShake, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IIEFreeThreadedHandShake methods ***
    STDMETHOD_(void,   PutHevent) (THIS_ HANDLE hevent) PURE;
    STDMETHOD_(HANDLE, GetHevent) (THIS) PURE;
    STDMETHOD_(void,    PutHresult) (THIS_ HRESULT hres) PURE;
    STDMETHOD_(HRESULT, GetHresult) (THIS) PURE;
    STDMETHOD_(IStream*, GetStream) (THIS) PURE;
};


#ifdef NO_MARSHALLING

#undef  INTERFACE
#define INTERFACE IWindowStatus
DECLARE_INTERFACE_(IWindowStatus, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IWindowStatus methods ***
    STDMETHOD(IsWindowActivated)() PURE;
};

#endif


// NOTE: The IETHREADPARAM structure is used between shdocvw, shell32,
// and browseui, so it cannot be modified after we ship, only extended.
typedef struct
{
    LPCWSTR pszCmdLine;
    UINT    uFlags;             // COF_ bits
    int     nCmdShow;

    //  these will always be set together
    ITravelLog *ptl;
    DWORD dwBrowserIndex;

    IEFreeThreadedHandShake* piehs;   // caller (thread) owns it

    // these come from explorer's NEWFOLDERINFO
    LPITEMIDLIST pidl;

    WCHAR szDdeRegEvent[MAX_IEEVENTNAME];
    WCHAR szCloseEvent[MAX_IEEVENTNAME];
    
    IShellBrowser* psbCaller;
    HWND hwndCaller;
    ISplashScreen *pSplash;
    LPITEMIDLIST pidlSelect;    // Only used if COF_SELECT

    LPITEMIDLIST pidlRoot;      // Only used if COF_NEWROOT
                                // 99/04/07 #141049 vtan: Overload pidlRoot with
                                // HMONITOR information on Windows 2000. Check the
                                // uFlags for COF_HASHMONITOR before using this.

    CLSID clsid;                // Only used if COF_NEWROOT
    CLSID clsidInProc;          // Only used if COF_INPROC
    
    // these come from explorer.exe's cabview struct
    WINDOWPLACEMENT wp;
    FOLDERSETTINGS fs;
    UINT wHotkey;

    WINVIEW wv;

    SHELLVIEWID m_vidRestore;
    DWORD m_dwViewPriority;
    
    long dwRegister;            // The register that was gotten from RegisterPending
    IUnknown *punkRefProcess;

    BOOL fNoLocalFileWarning : 1;
    BOOL fDontUseHomePage : 1;
    BOOL fFullScreen : 1;
    BOOL fNoDragDrop : 1;
    BOOL fAutomation : 1;
    BOOL fCheckFirstOpen : 1;
    BOOL fDesktopChannel : 1;

#ifdef UNIX
    BOOL fShouldStart : 1;
#endif

#ifdef NO_MARSHALLING
    BOOL fOnIEThread : 1;
#endif //NO_MARSHALLING
} IETHREADPARAM;

#ifdef UNIX
#define COF_HELPMODE            0x00010000      // Special mode for help display
#endif

#endif // __IETHREAD_H__
