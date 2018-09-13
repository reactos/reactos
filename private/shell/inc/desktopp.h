#ifndef _desktop_h
#define _desktop_h

#include <desktray.h>

// REVIEW: does not seem to be used...
// #define DTM_SIZEDESKTOP             (WM_USER + 75)
// #define DTM_EXITWINDOWS             (WM_USER + 78)

#define DTM_THREADEXIT              (WM_USER + 76)
#define DTM_SAVESTATE               (WM_USER + 77)
#define DTM_SHELLSERVICEOBJECTS     (WM_USER + 79)
#define DTM_STARTWAIT               (WM_USER + 80)
#define DTM_ENDWAIT                 (WM_USER + 81)

#define DTM_RELEASEPROXYDESKTOP     (WM_USER + 82)

#define DTM_RAISE                       (WM_USER + 83)
#define DTRF_RAISE      0
#define DTRF_LOWER      1
#define DTRF_QUERY      2    // to avoid sending a message to a hung desktop, query passes hwndTray in wParam, and desktop send TRAY a TM_DESKTOPSTATE message

#define DTM_ADDREFPROXYDESKTOP      (WM_USER + 84)
#define DTM_CREATESAVEDWINDOWS      (WM_USER + 85)
#define DTM_ENUMBANDS               (WM_USER + 86)

#ifdef DEBUG
#define DTM_NEXTCTL                 (WM_USER + 87)
#endif
#define DTM_UIACTIVATEIO            (WM_USER + 88)
#define DTM_ONFOCUSCHANGEIS         (WM_USER + 89)

#define DTM_SETUPAPPRAN             (WM_USER + 90)  // NT 5 USER posts this message to us

// END OF IE 4.00 / 4.01 MESSAGES

// BEGINNING OF IE 5.00 MESSAGES

#define DTM_GETVIEWAREAS            (WM_USER + 91)  // View area is WorkArea minus toolbar areas.
#define DTM_DESKTOPCONTEXTMENU      (WM_USER + 92)
#define DTM_UPDATENOW               (WM_USER + 93)

#define DTM_QUERYHKCRCHANGED        (WM_USER + 94)  // ask the desktop if HKCR has changed

#define DTM_MAKEHTMLCHANGES         (WM_USER + 95)  // Make changes to desktop html using dynamic HTML


#define COF_NORMAL              0x00000000
#define COF_CREATENEWWINDOW     0x00000001      // "/N"
#define COF_USEOPENSETTINGS     0x00000002      // "/A"
#define COF_WAITFORPENDING      0x00000004      // Should wait for Pending
#define COF_EXPLORE             0x00000008      // "/E"
#define COF_NEWROOT             0x00000010      // "/ROOT"
#define COF_ROOTCLASS           0x00000020      // "/ROOT,<GUID>"
#define COF_SELECT              0x00000040      // "/SELECT"
#define COF_AUTOMATION          0x00000080      // The user is trying to use automation
#define COF_OPENMASK            0x000000FF
#define COF_NOTUSERDRIVEN       0x00000100      // Not user driven
#define COF_NOTRANSLATE         0x00000200      // Don't ILCombine(pidlRoot, pidl)
#define COF_INPROC              0x00000400      // not used
#define COF_CHANGEROOTOK        0x00000800      // Try Desktop root if not in our root
#define COF_NOUI                0x00001000      // Start background desktop only (no folder/explorer)
#define COF_SHDOCVWFORMAT       0x00002000      // indicates this struct has been converted to abide by shdocvw format. 
                                                // this flag is temporary until we rip out all the 
#define COF_NOFINDWINDOW        0x00004000      // Don't try to find the window
#define COF_HASHMONITOR         0x00008000      // pidlRoot in IETHREADPARAM struct contains an HMONITOR
#define COF_SHELLFOLDERWINDOW   0x01000000      // This is a folder window, don't append - Microsoft Internet... when no pidl...
#define COF_PARSEPATHW          0x02000000      // the NFI.pszPath needs to be parsed but it is UNICODE
#define COF_FIREEVENTONDDEREG   0x20000000      // Fire an event when DDE server is registered
#define COF_FIREEVENTONCLOSE    0x40000000      // Fire an event when browser window closes
#define COF_IEXPLORE            0x80000000

//  this is used by DTM_QUERYHKCRCHANGED and the OpenAs Dialog
//  because the OpenAs Dialog is always in a separate process,
//  and it needs to cache a cookie in the desktop for the DTM
//  the QHKCRID is passed as the wParam in the message.
typedef enum 
{
    QHKCRID_NONE = 0,
    QHKCRID_MIN = 1, 
    QHKCRID_OPENAS = QHKCRID_MIN,
    QHKCRID_VIEWMENUPOPUP,
    QHKCRID_MAX
} QHKCRID;

//  didnt add PARSEPATHA because only browseui adds it, and it is UNICODE
//  but might need it later...
#define COF_PARSEPATH      COF_PARSEPATHW

typedef struct
{
    LPSTR pszPath;
    LPITEMIDLIST pidl;

    UINT uFlags;                // COF_ bits, (shared with IETHREADPARAM.uFlags
    int nShow;
    HWND hwndCaller;
    DWORD dwHotKey;
    LPITEMIDLIST pidlSelect;    // Only used if COF_SELECT

    LPSTR pszRoot;              // Only used for Parse_CmdLine
    LPITEMIDLIST pidlRoot;      // Only used if COF_NEWROOT
    CLSID clsid;                // Only used if COF_NEWROOT

    CLSID clsidInProc;          // Only used if COF_INPROC
} NEWFOLDERINFO, *PNEWFOLDERINFO;

STDAPI_(HANDLE) SHCreateDesktop(IDeskTray* pdtray);
STDAPI_(BOOL) CreateFromDesktop(PNEWFOLDERINFO pfi);
STDAPI_(BOOL) SHCreateFromDesktop(PNEWFOLDERINFO pfi);
STDAPI_(BOOL) SHDesktopMessageLoop(HANDLE hDesktop);
STDAPI_(BOOL) SHExplorerParseCmdLine(PNEWFOLDERINFO pfi);

// for the desktop to handle DDE
#define IDT_DDETIMEOUT      1
STDAPI_(LRESULT) DDEHandleMsgs(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
STDAPI_(void) DDEHandleTimeout(HWND hwnd);


#endif

