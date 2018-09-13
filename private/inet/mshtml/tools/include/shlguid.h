//
// For shell-reserved GUID
//
//  The Win95 Shell has been allocated a block of 256 GUIDs,
// which follow the general format:
//
//  000214xx-0000-0000-C000-000000000046
//
//
#define DEFINE_SHLGUID(name, l, w1, w2) DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

//
// Class IDs        xx=00-8F
//
DEFINE_SHLGUID(CLSID_ShellDesktop,      0x00021400L, 0, 0);
DEFINE_SHLGUID(CLSID_ShellLink,         0x00021401L, 0, 0);

//
// CatInformation IDs   xx=90-9F
//
DEFINE_SHLGUID(CATID_BrowsableShellExt, 0x00021490L, 0, 0);
DEFINE_SHLGUID(CATID_BrowseInPlace,     0x00021491L, 0, 0);

// Format IDs       xx=A0-CF
DEFINE_SHLGUID(FMTID_Intshcut,          0x000214A0L, 0, 0);
DEFINE_SHLGUID(FMTID_InternetSite,      0x000214A1L, 0, 0);

// command group ids xx=D0-DF
DEFINE_SHLGUID(CGID_Explorer,           0x000214D0L, 0, 0);
DEFINE_SHLGUID(CGID_ShellDocView,       0x000214D1L, 0, 0);
DEFINE_SHLGUID(CGID_ShellServiceObject, 0x000214D2L, 0, 0);

//
// Interface IDs    xx=E0-FF
//
DEFINE_SHLGUID(IID_IShellToolbar,       0x000214E0L, 0, 0); 
DEFINE_SHLGUID(IID_INewShortcutHookA,   0x000214E1L, 0, 0);
DEFINE_SHLGUID(IID_IShellBrowser,       0x000214E2L, 0, 0);
DEFINE_SHLGUID(IID_IShellView,          0x000214E3L, 0, 0);
DEFINE_SHLGUID(IID_IContextMenu,        0x000214E4L, 0, 0);
DEFINE_SHLGUID(IID_IShellIcon,          0x000214E5L, 0, 0);
DEFINE_SHLGUID(IID_IShellFolder,        0x000214E6L, 0, 0);
DEFINE_SHLGUID(IID_IShellExtInit,       0x000214E8L, 0, 0);
DEFINE_SHLGUID(IID_IShellPropSheetExt,  0x000214E9L, 0, 0);
DEFINE_SHLGUID(IID_IPersistFolder,      0x000214EAL, 0, 0);
DEFINE_SHLGUID(IID_IExtractIconA,       0x000214EBL, 0, 0);
DEFINE_SHLGUID(IID_IShellLinkA,         0x000214EEL, 0, 0);
DEFINE_SHLGUID(IID_IShellCopyHookA,     0x000214EFL, 0, 0);
DEFINE_SHLGUID(IID_IFileViewerA,        0x000214F0L, 0, 0);
DEFINE_SHLGUID(IID_ICommDlgBrowser,     0x000214F1L, 0, 0);
DEFINE_SHLGUID(IID_IEnumIDList,         0x000214F2L, 0, 0);
DEFINE_SHLGUID(IID_IFileViewerSite,     0x000214F3L, 0, 0);
DEFINE_SHLGUID(IID_IContextMenu2,       0x000214F4L, 0, 0);
DEFINE_SHLGUID(IID_IShellExecuteHookA,  0x000214F5L, 0, 0);
DEFINE_SHLGUID(IID_IPropSheetPage,      0x000214F6L, 0, 0);
DEFINE_SHLGUID(IID_INewShortcutHookW,   0x000214F7L, 0, 0);
DEFINE_SHLGUID(IID_IFileViewerW,        0x000214F8L, 0, 0);
DEFINE_SHLGUID(IID_IShellLinkW,         0x000214F9L, 0, 0);
DEFINE_SHLGUID(IID_IExtractIconW,       0x000214FAL, 0, 0);
DEFINE_SHLGUID(IID_IShellExecuteHookW,  0x000214FBL, 0, 0);
DEFINE_SHLGUID(IID_IShellCopyHookW,     0x000214FCL, 0, 0);
DEFINE_SHLGUID(IID_IShellToolbarSite,   0x000214FFL, 0, 0); 

// AC60F6A0-0FD9-11D0-99CB-00C04FD64497
DEFINE_GUID(IID_IURLSearchHook, 0xAC60F6A0L, 0x0FD9, 0x11D0, 0x99, 0xCB, 0x00, 0xC0, 0x4F, 0xD6, 0x44, 0x97);

DEFINE_GUID(IID_IShellView2, 0x88E39E80L, 0x3578, 0x11CF, 0xAE, 0x69, 0x08, 0x00, 0x2B, 0x2E, 0x12, 0x62);

DEFINE_GUID(IID_IDelegateFolder, 0xADD8BA80L, 0x002B, 0x11D0, 0x8F, 0x0F, 0x00, 0xC0, 0x4F, 0xD7, 0xD0, 0x62);

DEFINE_GUID(IID_IShellToolbarFrame, 0xEB0FE170L, 0x1A3A, 0x11D0, 0x89, 0xB3, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xAC);
DEFINE_GUID(IID_IShellToolband, 0xEB0FE171L, 0x1A3A, 0x11D0, 0x89, 0xB3, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xAC);



#define SID_SShellBrowser IID_IShellBrowser
#define SID_SShellDesktop CLSID_ShellDesktop

#ifdef UNICODE
#define IID_IFileViewer         IID_IFileViewerW
#define IID_IShellLink          IID_IShellLinkW
#define IID_IExtractIcon        IID_IExtractIconW
#define IID_IShellCopyHook      IID_IShellCopyHookW
#define IID_IShellExecuteHook   IID_IShellExecuteHookW
#define IID_INewShortcutHook    IID_INewShortcutHookW
#else
#define IID_IFileViewer         IID_IFileViewerA
#define IID_IShellLink          IID_IShellLinkA
#define IID_IExtractIcon        IID_IExtractIconA
#define IID_IShellCopyHook      IID_IShellCopyHookA
#define IID_IShellExecuteHook   IID_IShellExecuteHookA
#define IID_INewShortcutHook    IID_INewShortcutHookA
#endif


#ifndef NO_INTSHCUT_GUIDS
// Include internet shortcut GUIDs
#include <isguids.h>
#endif

#include <exdisp.h>

#ifndef NO_SHDOCVW_GUIDS
#define DIID_DInternetExplorerEvents DIID_DWebBrowserEvents
#define DInternetExplorerEvents DWebBrowserEvents
#define IID_IInternetExplorer IID_IWebBrowserApp

// UrlHistory Guids
DEFINE_GUID(CLSID_CUrlHistory, 0x3C374A40L, 0xBAE4, 0x11CF, 0xBF, 0x7D, 0x00, 0xAA, 0x00, 0x69, 0x46, 0xEE);
DEFINE_GUID(IID_IUrlHistoryStg, 0x3C374A41L, 0xBAE4, 0x11CF, 0xBF, 0x7D, 0x00, 0xAA, 0x00, 0x69, 0x46, 0xEE);
DEFINE_GUID(IID_IEnumSTATURL, 0x3C374A42L, 0xBAE4, 0x11CF, 0xBF, 0x7D, 0x00, 0xAA, 0x00, 0x69, 0x46, 0xEE);
#define SID_SUrlHistory         CLSID_CUrlHistory


//UrlSearchHook Guids
//{CFBFAE00-17A6-11D0-99CB-00C04FD64497}
DEFINE_GUID(CLSID_CURLSearchHook, 0xCFBFAE00L, 0x17A6, 0x11D0, 0x99, 0xCB, 0x00, 0xC0, 0x4F, 0xD6, 0x44, 0x97);

// Temp to keep old guids working during the transition
DEFINE_GUID(DIID_DHTMLDocEvents,0x626FC521,0xA41E,0x11CF,0xA7,0x31,0x00,0xA0,0xC9,0x08,0x26,0x37);
#define SID_SInternetExplorer IID_IWebBrowserApp
#define SID_SWebBrowserApp    IID_IWebBrowserApp

//
// Top-most browser implementation in the heirarchy. use IServiceProvider::QueryService()
// to get to interfaces (IID_IShellBrowser, IID_IBrowserService, etc.)
//
// 4C96BE40-915C-11CF-99D3-00AA004AE837
DEFINE_GUID(SID_STopLevelBrowser, 0x4C96BE40L, 0x915C, 0x11CF, 0x99, 0xD3, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37);

#define IID_IShellExplorer        IID_IWebBrowser
#define DIID_DShellExplorerEvents DIID_DWebBrowserEvents
#define CLSID_ShellExplorer       CLSID_WebBrowser
#define IID_DIExplorer            IID_IWebBrowserApp
#define DIID_DExplorerEvents      DIID_DInternetExplorer
#endif


