#include <windows.h>
#include <stdio.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#define NO_SHLWAPI_STRFCNS
#define NO_SHLWAPI_PATH
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_UALSTR
#define NO_SHLWAPI_HTTP
#define NO_SHLWAPI_INTERNAL
#include <shlwapip.h>

#define FE_SB 1

#include <shlobjp.h>

#ifndef PLINKINFO
#define PLINKINFO LPVOID
#endif

#define EXP_SZ_LINK_SIG         0xA0000001
//#define NT_CONSOLE_PROPS_SIG    0xA0000002 // moved to shlobj.w
#define EXP_TRACKER_SIG         0xA0000003
//#define NT_FE_CONSOLE_PROPS_SIG 0xA0000004 // moved to shlobj.w
#define EXP_DARWIN_ID_SIG       0xA0000006

#define EXP_HEADER DATABLOCK_HEADER
#define LPEXP_HEADER LPDATABLOCK_HEADER

// most expansion data structures go here
// those shared with other components (NT40 Console stuff)
// are in shlobj.w (private)
//

typedef struct {
    IShellLink          sl;
    IPersistStream      ps;
    IPersistFile        pf;
    IShellExtInit       si;
    IContextMenu2       cm;
    IDropTarget         dt;
#ifdef USE_DATA_OBJ
    IDataObj            dobj;
#endif
////IExtractIcon        xi;
#ifdef UNICODE
    IShellLinkA         slA;            // To support ANSI callers
#endif
#ifdef ENABLE_TRACK
    IShellLinkTracker   slt;        // Interface to CTracker object.
#endif
#ifdef WINNT
    IShellLinkDataList  sldl;
#endif

    UINT                cRef;

    BOOL                bDirty;         // something has changed
    LPTSTR              pszCurFile;     // current file from IPersistFile
    LPTSTR              pszRelSource;   // overrides pszCurFile in relative tracking

    IContextMenu        *pcmTarget;     // stuff for IContextMenu
    UINT                indexMenuSave;
    UINT                idCmdFirstSave;
    UINT                idCmdLastSave;
    UINT                uFlagsSave;

    BOOL                fDataAlreadyResolved;   // for data object

    // IDropTarget specific
    IDropTarget*        pdtSrc;         // IDropTarget of link source (unresolved)
    DWORD               grfKeyStateLast;

    // persistant data

    LPITEMIDLIST        pidl;           // may be NULL
    PLINKINFO           pli;            // may be NULL

    LPTSTR              pszName;        // title on short volumes
    LPTSTR              pszRelPath;
    LPTSTR              pszWorkingDir;
    LPTSTR              pszArgs;
    LPTSTR              pszIconLocation;

    LPDBLIST            pExtraData;     // extra data to preserve for future compatibility

#ifdef ENABLE_TRACK
    struct CTracker *   ptracker;
#endif

    SHELL_LINK_DATA     sld;
} CShellLink;

