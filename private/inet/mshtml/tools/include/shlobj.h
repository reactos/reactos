//===========================================================================
//
// Copyright (c) Microsoft Corporation 1991-1996
//
// File: shlobj.h
//
//===========================================================================

#ifndef _SHLOBJ_H_
#define _SHLOBJ_H_

#ifndef SNDMSG
#ifdef __cplusplus
#define SNDMSG ::SendMessage
#else
#define SNDMSG SendMessage
#endif
#endif // ifndef SNDMSG

//
// Define API decoration for direct importing of DLL references.
//
#ifndef WINSHELLAPI
#if !defined(_SHELL32_)
#define WINSHELLAPI       DECLSPEC_IMPORT
#define SHSTDAPI          EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define SHSTDAPI_(type)   EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#else
#define WINSHELLAPI
#define SHSTDAPI          STDAPI
#define SHSTDAPI_(type)   STDAPI_(type)
#endif
#endif // WINSHELLAPI


#include <ole2.h>
#ifndef _PRSHT_H_
#include <prsht.h>
#endif
#ifndef _INC_COMMCTRL
#include <commctrl.h>   // for LPTBBUTTON
#endif

#ifndef INITGUID
#include <shlguid.h>
#endif /* !INITGUID */

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif /* __cplusplus */

//===========================================================================
//
// Object identifiers in the explorer's name space (ItemID and IDList)
//
//  All the items that the user can browse with the explorer (such as files,
// directories, servers, work-groups, etc.) has an identifier which is unique
// among items within the parent folder. Those identifiers are called item
// IDs (SHITEMID). Since all its parent folders have their own item IDs,
// any items can be uniquely identified by a list of item IDs, which is called
// an ID list (ITEMIDLIST).
//
//  ID lists are almost always allocated by the task allocator (see some
// description below as well as OLE 2.0 SDK) and may be passed across
// some of shell interfaces (such as IShellFolder). Each item ID in an ID list
// is only meaningful to its parent folder (which has generated it), and all
// the clients must treat it as an opaque binary data except the first two
// bytes, which indicates the size of the item ID.
//
//  When a shell extension -- which implements the IShellFolder interace --
// generates an item ID, it may put any information in it, not only the data
// with that it needs to identifies the item, but also some additional
// information, which would help implementing some other functions efficiently.
// For example, the shell's IShellFolder implementation of file system items
// stores the primary (long) name of a file or a directory as the item
// identifier, but it also stores its alternative (short) name, size and date
// etc.
//
//  When an ID list is passed to one of shell APIs (such as SHGetPathFromIDList),
// it is always an absolute path -- relative from the root of the name space,
// which is the desktop folder. When an ID list is passed to one of IShellFolder
// member function, it is always a relative path from the folder (unless it
// is explicitly specified).
//
//===========================================================================

//
// SHITEMID -- Item ID
//
typedef struct _SHITEMID        // mkid
{
    USHORT      cb;             // Size of the ID (including cb itself)
    BYTE        abID[1];        // The item ID (variable length)
} SHITEMID;
typedef UNALIGNED SHITEMID *LPSHITEMID;
typedef const UNALIGNED SHITEMID *LPCSHITEMID;

//
// ITEMIDLIST -- List if item IDs (combined with 0-terminator)
//
typedef struct _ITEMIDLIST      // idl
{
    SHITEMID    mkid;
} ITEMIDLIST;
typedef UNALIGNED ITEMIDLIST * LPITEMIDLIST;
typedef const UNALIGNED ITEMIDLIST * LPCITEMIDLIST;


//===========================================================================
//
// Task allocator API
//
//  All the shell extensions MUST use the task allocator (see OLE 2.0
// programming guild for its definition) when they allocate or free
// memory objects (mostly ITEMIDLIST) that are returned across any
// shell interfaces. There are two ways to access the task allocator
// from a shell extension depending on whether or not it is linked with
// OLE32.DLL or not (purely for efficiency).
//
// (1) A shell extension which calls any OLE API (i.e., linked with
//  OLE32.DLL) should call OLE's task allocator (by retrieving
//  the task allocator by calling CoGetMalloc API).
//
// (2) A shell extension which does not call any OLE API (i.e., not linked
//  with OLE32.DLL) should call the shell task allocator API (defined
//  below), so that the shell can quickly loads it when OLE32.DLL is not
//  loaded by any application at that point.
//
// Notes:
//  In next version of Windowso release, SHGetMalloc will be replaced by
// the following macro.
//
// #define SHGetMalloc(ppmem)   CoGetMalloc(MEMCTX_TASK, ppmem)
//
//===========================================================================

WINSHELLAPI HRESULT WINAPI SHGetMalloc(LPMALLOC * ppMalloc);



//===========================================================================
//
// IContextMenu interface
//
// [OverView]
//
//  The shell uses the IContextMenu interface in following three cases.
//
// case-1: The shell is loading context menu extensions.
//
//   When the user clicks the right mouse button on an item within the shell's
//  name space (i.g., file, directory, server, work-group, etc.), it creates
//  the default context menu for its type, then loads context menu extensions
//  that are registered for that type (and its base type) so that they can
//  add extra menu items. Those context menu extensions are registered at
//  HKCR\{ProgID}\shellex\ContextMenuHandlers.
//
// case-2: The shell is retrieving a context menu of sub-folders in extended
//   name-space.
//
//   When the explorer's name space is extended by name space extensions,
//  the shell calls their IShellFolder::GetUIObjectOf to get the IContextMenu
//  objects when it creates context menus for folders under those extended
//  name spaces.
//
// case-3: The shell is loading non-default drag and drop handler for directories.
//
//   When the user performed a non-default drag and drop onto one of file
//  system folders (i.e., directories), it loads shell extensions that are
//  registered at HKCR\{ProgID}\DragDropHandlers.
//
//
// [Member functions]
//
//
// IContextMenu::QueryContextMenu
//
//   This member function may insert one or more menuitems to the specified
//  menu (hmenu) at the specified location (indexMenu which is never be -1).
//  The IDs of those menuitem must be in the specified range (idCmdFirst and
//  idCmdLast). It returns the maximum menuitem ID offset (ushort) in the
//  'code' field (low word) of the scode.
//
//   The uFlags specify the context. It may have one or more of following
//  flags.
//
//  CMF_DEFAULTONLY: This flag is passed if the user is invoking the default
//   action (typically by double-clicking, case 1 and 2 only). Context menu
//   extensions (case 1) should not add any menu items, and returns NOERROR.
//
//  CMF_VERBSONLY: The explorer passes this flag if it is constructing
//   a context menu for a short-cut object (case 1 and case 2 only). If this
//   flag is passed, it should not add any menu-items that is not appropriate
//   from a short-cut.
//    A good example is the "Delete" menuitem, which confuses the user
//   because it is not clear whether it deletes the link source item or the
//   link itself.
//
//  CMF_EXPLORER: The explorer passes this flag if it has the left-side pane
//   (case 1 and 2 only). Context menu extensions should ignore this flag.
//
//   High word (16-bit) are reserved for context specific communications
//  and the rest of flags (13-bit) are reserved by the system.
//
//
// IContextMenu::InvokeCommand
//
//   This member is called when the user has selected one of menuitems that
//  are inserted by previous QueryContextMenu member. In this case, the
//  LOWORD(lpici->lpVerb) contains the menuitem ID offset (menuitem ID -
//  idCmdFirst).
//
//   This member function may also be called programmatically. In such a case,
//  lpici->lpVerb specifies the canonical name of the command to be invoked,
//  which is typically retrieved by GetCommandString member previously.
//
//  Parameters in lpci:
//    cbSize -- Specifies the size of this structure (sizeof(*lpci))
//    hwnd   -- Specifies the owner window for any message/dialog box.
//    fMask  -- Specifies whether or not dwHotkey/hIcon paramter is valid.
//    lpVerb -- Specifies the command to be invoked.
//    lpParameters -- Parameters (optional)
//    lpDirectory  -- Working directory (optional)
//    nShow -- Specifies the flag to be passed to ShowWindow (SW_*).
//    dwHotKey -- Hot key to be assigned to the app after invoked (optional).
//    hIcon -- Specifies the icon (optional).
//
//
// IContextMenu::GetCommandString
//
//   This member function is called by the explorer either to get the
//  canonical (language independent) command name (uFlags == GCS_VERB) or
//  the help text ((uFlags & GCS_HELPTEXT) != 0) for the specified command.
//  The retrieved canonical string may be passed to its InvokeCommand
//  member function to invoke a command programmatically. The explorer
//  displays the help texts in its status bar; therefore, the length of
//  the help text should be reasonably short (<40 characters).
//
//  Parameters:
//   idCmd -- Specifies menuitem ID offset (from idCmdFirst)
//   uFlags -- Either GCS_VERB or GCS_HELPTEXT
//   pwReserved -- Reserved (must pass NULL when calling, must ignore when called)
//   pszName -- Specifies the string buffer.
//   cchMax -- Specifies the size of the string buffer.
//
//===========================================================================

// QueryContextMenu uFlags
#define CMF_NORMAL              0x00000000
#define CMF_DEFAULTONLY         0x00000001
#define CMF_VERBSONLY           0x00000002
#define CMF_EXPLORE             0x00000004
#define CMF_NOVERBS             0x00000008
#define CMF_CANRENAME           0x00000010
#define CMF_NODEFAULT           0x00000020
#define CMF_INCLUDESTATIC       0x00000040
#define CMF_RESERVED            0xffff0000      // View specific

// GetCommandString uFlags
#define GCS_VERBA        0x00000000     // canonical verb
#define GCS_HELPTEXTA    0x00000001     // help text (for status bar)
#define GCS_VALIDATEA    0x00000002     // validate command exists
#define GCS_VERBW        0x00000004     // canonical verb (unicode)
#define GCS_HELPTEXTW    0x00000005     // help text (unicode version)
#define GCS_VALIDATEW    0x00000006     // validate command exists (unicode)
#define GCS_UNICODE      0x00000004     // for bit testing - Unicode string

#ifdef UNICODE
#define GCS_VERB        GCS_VERBW
#define GCS_HELPTEXT    GCS_HELPTEXTW
#define GCS_VALIDATE    GCS_VALIDATEW
#else
#define GCS_VERB        GCS_VERBA
#define GCS_HELPTEXT    GCS_HELPTEXTA
#define GCS_VALIDATE    GCS_VALIDATEA
#endif

#define CMDSTR_NEWFOLDERA   "NewFolder"
#define CMDSTR_VIEWLISTA    "ViewList"
#define CMDSTR_VIEWDETAILSA "ViewDetails"
#define CMDSTR_NEWFOLDERW   L"NewFolder"
#define CMDSTR_VIEWLISTW    L"ViewList"
#define CMDSTR_VIEWDETAILSW L"ViewDetails"

#ifdef UNICODE
#define CMDSTR_NEWFOLDER    CMDSTR_NEWFOLDERW
#define CMDSTR_VIEWLIST     CMDSTR_VIEWLISTW
#define CMDSTR_VIEWDETAILS  CMDSTR_VIEWDETAILSW
#else
#define CMDSTR_NEWFOLDER    CMDSTR_NEWFOLDERA
#define CMDSTR_VIEWLIST     CMDSTR_VIEWLISTA
#define CMDSTR_VIEWDETAILS  CMDSTR_VIEWDETAILSA
#endif

#define CMIC_MASK_HOTKEY        SEE_MASK_HOTKEY
#define CMIC_MASK_ICON          SEE_MASK_ICON
#define CMIC_MASK_FLAG_NO_UI    SEE_MASK_FLAG_NO_UI
#define CMIC_MASK_UNICODE       SEE_MASK_UNICODE
#define CMIC_MASK_NO_CONSOLE    SEE_MASK_NO_CONSOLE
#define CMIC_MASK_HASLINKNAME   SEE_MASK_HASLINKNAME
#define CMIC_MASK_FLAG_SEP_VDM  SEE_MASK_FLAG_SEPVDM
#define CMIC_MASK_HASTITLE      SEE_MASK_HASTITLE
#define CMIC_MASK_ASYNCOK       SEE_MASK_ASYNCOK


typedef struct _CMINVOKECOMMANDINFO {
    DWORD cbSize;        // sizeof(CMINVOKECOMMANDINFO)
    DWORD fMask;         // any combination of CMIC_MASK_*
    HWND hwnd;           // might be NULL (indicating no owner window)
    LPCSTR lpVerb;       // either a string or MAKEINTRESOURCE(idOffset)
    LPCSTR lpParameters; // might be NULL (indicating no parameter)
    LPCSTR lpDirectory;  // might be NULL (indicating no specific directory)
    int nShow;           // one of SW_ values for ShowWindow() API

    DWORD dwHotKey;
    HANDLE hIcon;
} CMINVOKECOMMANDINFO,  *LPCMINVOKECOMMANDINFO;

typedef struct _CMInvokeCommandInfoEx {
    DWORD cbSize;        // must be sizeof(CMINVOKECOMMANDINFOEX)
    DWORD fMask;         // any combination of CMIC_MASK_*
    HWND hwnd;           // might be NULL (indicating no owner window)
    LPCSTR lpVerb;       // either a string or MAKEINTRESOURCE(idOffset)
    LPCSTR lpParameters; // might be NULL (indicating no parameter)
    LPCSTR lpDirectory;  // might be NULL (indicating no specific directory)
    int nShow;           // one of SW_ values for ShowWindow() API

    DWORD dwHotKey;
    HANDLE hIcon;
    LPCSTR lpTitle;      // For CreateProcess-StartupInfo.lpTitle
    LPCWSTR lpVerbW;        // Unicode verb (for those who can use it)
    LPCWSTR lpParametersW;  // Unicode parameters (for those who can use it)
    LPCWSTR lpDirectoryW;   // Unicode directory (for those who can use it)
    LPCWSTR lpTitleW;       // Unicode title (for those who can use it)
} CMINVOKECOMMANDINFOEX,  *LPCMINVOKECOMMANDINFOEX;

#undef  INTERFACE
#define INTERFACE   IContextMenu

DECLARE_INTERFACE_(IContextMenu, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(QueryContextMenu)(THIS_
                                HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags) PURE;

    STDMETHOD(InvokeCommand)(THIS_
                             LPCMINVOKECOMMANDINFO lpici) PURE;

    STDMETHOD(GetCommandString)(THIS_
                                UINT        idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax) PURE;
};

typedef IContextMenu *  LPCONTEXTMENU;

//
// IContextMenu2 (IContextMenu with one new member)
//
// IContextMenu2::HandleMenuMsg
//
//  This function is called, if the client of IContextMenu is aware of
// IContextMenu2 interface and receives one of following messages while
// it is calling TrackPopupMenu (in the window proc of hwndOwner):
//      WM_INITPOPUP, WM_DRAWITEM and WM_MEASUREITEM
//  The callee may handle these messages to draw owner draw menuitems.
//

#undef  INTERFACE
#define INTERFACE   IContextMenu2

DECLARE_INTERFACE_(IContextMenu2, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(QueryContextMenu)(THIS_
                                HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags) PURE;

    STDMETHOD(InvokeCommand)(THIS_
                             LPCMINVOKECOMMANDINFO lpici) PURE;

    STDMETHOD(GetCommandString)(THIS_
                                UINT        idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax) PURE;
    STDMETHOD(HandleMenuMsg)(THIS_
                             UINT uMsg,
                             WPARAM wParam,
                             LPARAM lParam) PURE;
};

typedef IContextMenu2 * LPCONTEXTMENU2;




//===========================================================================
//
// Interface: IShellExtInit
//
//  The IShellExtInit interface is used by the explorer to initialize shell
// extension objects. The explorer (1) calls CoCreateInstance (or equivalent)
// with the registered CLSID and IID_IShellExtInit, (2) calls its Initialize
// member, then (3) calls its QueryInterface to a particular interface (such
// as IContextMenu or IPropSheetExt and (4) performs the rest of operation.
//
//
// [Member functions]
//
// IShellExtInit::Initialize
//
//  This member function is called when the explorer is initializing either
// context menu extension, property sheet extension or non-default drag-drop
// extension.
//
//  Parameters: (context menu or property sheet extension)
//   pidlFolder -- Specifies the parent folder
//   lpdobj -- Spefifies the set of items selected in that folder.
//   hkeyProgID -- Specifies the type of the focused item in the selection.
//
//  Parameters: (non-default drag-and-drop extension)
//   pidlFolder -- Specifies the target (destination) folder
//   lpdobj -- Specifies the items that are dropped (see the description
//    about shell's clipboard below for clipboard formats).
//   hkeyProgID -- Specifies the folder type.
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IShellExtInit

DECLARE_INTERFACE_(IShellExtInit, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellExtInit methods ***
    STDMETHOD(Initialize)(THIS_ LPCITEMIDLIST pidlFolder,
                          LPDATAOBJECT lpdobj, HKEY hkeyProgID) PURE;
};

typedef IShellExtInit * LPSHELLEXTINIT;


//===========================================================================
//
// Interface: IShellPropSheetExt
//
//  The explorer uses the IShellPropSheetExt to allow property sheet
// extensions or control panel extensions to add additional property
// sheet pages.
//
//
// [Member functions]
//
// IShellPropSheetExt::AddPages
//
//  The explorer calls this member function when it finds a registered
// property sheet extension for a particular type of object. For each
// additional page, the extension creates a page object by calling
// CreatePropertySheetPage API and calls lpfnAddPage.
//
//  Parameters:
//   lpfnAddPage -- Specifies the callback function.
//   lParam -- Specifies the opaque handle to be passed to the callback function.
//
//
// IShellPropSheetExt::ReplacePage
//
//  The explorer never calls this member of property sheet extensions. The
// explorer calls this member of control panel extensions, so that they
// can replace some of default control panel pages (such as a page of
// mouse control panel).
//
//  Parameters:
//   uPageID -- Specifies the page to be replaced.
//   lpfnReplace Specifies the callback function.
//   lParam -- Specifies the opaque handle to be passed to the callback function.
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IShellPropSheetExt

DECLARE_INTERFACE_(IShellPropSheetExt, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellPropSheetExt methods ***
    STDMETHOD(AddPages)(THIS_ LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam) PURE;
    STDMETHOD(ReplacePage)(THIS_ UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam) PURE;
};

typedef IShellPropSheetExt * LPSHELLPROPSHEETEXT;


//===========================================================================
//
// IPersistFolder Interface
//
//  The IPersistFolder interface is used by the file system implementation of
// IShellFolder::BindToObject when it is initializing a shell folder object.
//
//
// [Member functions]
//
// IPersistFolder::Initialize
//
//  This member function is called when the explorer is initializing a
// shell folder object.
//
//  Parameters:
//   pidl -- Specifies the absolute location of the folder.
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IPersistFolder

DECLARE_INTERFACE_(IPersistFolder, IPersist)    // fld
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(THIS_ LPCLSID lpClassID) PURE;

    // *** IPersistFolder methods ***
    STDMETHOD(Initialize)(THIS_ LPCITEMIDLIST pidl) PURE;
};

typedef IPersistFolder *LPPERSISTFOLDER;


//===========================================================================
//
// IExtractIcon interface
//
//  This interface is used in two different places in the shell.
//
// Case-1: Icons of sub-folders for the scope-pane of the explorer.
//
//  It is used by the explorer to get the "icon location" of
// sub-folders from each shell folders. When the user expands a folder
// in the scope pane of the explorer, the explorer does following:
//  (1) binds to the folder (gets IShellFolder),
//  (2) enumerates its sub-folders by calling its EnumObjects member,
//  (3) calls its GetUIObjectOf member to get IExtractIcon interface
//     for each sub-folders.
//  In this case, the explorer uses only IExtractIcon::GetIconLocation
// member to get the location of the appropriate icon. An icon location
// always consists of a file name (typically DLL or EXE) and either an icon
// resource or an icon index.
//
//
// Case-2: Extracting an icon image from a file
//
//  It is used by the shell when it extracts an icon image
// from a file. When the shell is extracting an icon from a file,
// it does following:
//  (1) creates the icon extraction handler object (by getting its CLSID
//     under the {ProgID}\shell\ExtractIconHanler key and calling
//     CoCreateInstance requesting for IExtractIcon interface).
//  (2) Calls IExtractIcon::GetIconLocation.
//  (3) Then, calls IExtractIcon::ExtractIcon with the location/index pair.
//  (4) If (3) returns NOERROR, it uses the returned icon.
//  (5) Otherwise, it recursively calls this logic with new location
//     assuming that the location string contains a fully qualified path name.
//
//  From extension programmer's point of view, there are only two cases
// where they provide implementations of IExtractIcon:
//  Case-1) providing explorer extensions (i.e., IShellFolder).
//  Case-2) providing per-instance icons for some types of files.
//
// Because Case-1 is described above, we'll explain only Case-2 here.
//
// When the shell is about display an icon for a file, it does following:
//  (1) Finds its ProgID and ClassID.
//  (2) If the file has a ClassID, it gets the icon location string from the
//    "DefaultIcon" key under it. The string indicates either per-class
//    icon (e.g., "FOOBAR.DLL,2") or per-instance icon (e.g., "%1,1").
//  (3) If a per-instance icon is specified, the shell creates an icon
//    extraction handler object for it, and extracts the icon from it
//    (which is described above).
//
//  It is important to note that the shell calls IExtractIcon::GetIconLocation
// first, then calls IExtractIcon::Extract. Most application programs
// that support per-instance icons will probably store an icon location
// (DLL/EXE name and index/id) rather than an icon image in each file.
// In those cases, a programmer needs to implement only the GetIconLocation
// member and it Extract member simply returns S_FALSE. They need to
// implement Extract member only if they decided to store the icon images
// within files themselved or some other database (which is very rare).
//
//
//
// [Member functions]
//
//
// IExtractIcon::GetIconLocation
//
//  This function returns an icon location.
//
//  Parameters:
//   uFlags     [in]  -- Specifies if it is opened or not (GIL_OPENICON or 0)
//   szIconFile [out] -- Specifies the string buffer buffer for a location name.
//   cchMax     [in]  -- Specifies the size of szIconFile (almost always MAX_PATH)
//   piIndex    [out] -- Sepcifies the address of UINT for the index.
//   pwFlags    [out] -- Returns GIL_* flags
//  Returns:
//   NOERROR, if it returns a valid location; S_FALSE, if the shell use a
//   default icon.
//
//  Notes: The location may or may not be a path to a file. The caller can
//   not assume anything unless the subsequent Extract member call returns
//   S_FALSE.
//
//   if the returned location is not a path to a file, GIL_NOTFILENAME should
//   be set in the returned flags.
//
// IExtractIcon::Extract
//
//  This function extracts an icon image from a specified file.
//
//  Parameters:
//   pszFile [in] -- Specifies the icon location (typically a path to a file).
//   nIconIndex [in] -- Specifies the icon index.
//   phiconLarge [out] -- Specifies the HICON variable for large icon.
//   phiconSmall [out] -- Specifies the HICON variable for small icon.
//   nIconSize [in] -- Specifies the size icon required (size of large icon)
//                     LOWORD is the requested large icon size
//                     HIWORD is the requested small icon size
//  Returns:
//   NOERROR, if it extracted the from the file.
//   S_FALSE, if the caller should extract from the file specified in the
//           location.
//
//===========================================================================

// GetIconLocation() input flags

#define GIL_OPENICON     0x0001      // allows containers to specify an "open" look
#define GIL_FORSHELL     0x0002      // icon is to be displayed in a ShellFolder
#define GIL_ASYNC        0x0020      // this is an async extract, return E_ASYNC

// GetIconLocation() return flags

#define GIL_SIMULATEDOC  0x0001      // simulate this document icon for this
#define GIL_PERINSTANCE  0x0002      // icons from this class are per instance (each file has its own)
#define GIL_PERCLASS     0x0004      // icons from this class per class (shared for all files of this type)
#define GIL_NOTFILENAME  0x0008      // location is not a filename, must call ::ExtractIcon
#define GIL_DONTCACHE    0x0010      // this icon should not be cached

#undef  INTERFACE
#define INTERFACE   IExtractIconA

DECLARE_INTERFACE_(IExtractIconA, IUnknown)     // exic
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExtractIcon methods ***
    STDMETHOD(GetIconLocation)(THIS_
                         UINT   uFlags,
                         LPSTR  szIconFile,
                         UINT   cchMax,
                         int   * piIndex,
                         UINT  * pwFlags) PURE;

    STDMETHOD(Extract)(THIS_
                           LPCSTR pszFile,
                           UINT   nIconIndex,
                           HICON   *phiconLarge,
                           HICON   *phiconSmall,
                           UINT    nIconSize) PURE;
};

typedef IExtractIconA * LPEXTRACTICONA;

#undef  INTERFACE
#define INTERFACE   IExtractIconW

DECLARE_INTERFACE_(IExtractIconW, IUnknown)     // exic
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExtractIcon methods ***
    STDMETHOD(GetIconLocation)(THIS_
                         UINT   uFlags,
                         LPWSTR szIconFile,
                         UINT   cchMax,
                         int   * piIndex,
                         UINT  * pwFlags) PURE;

    STDMETHOD(Extract)(THIS_
                           LPCWSTR pszFile,
                           UINT   nIconIndex,
                           HICON   *phiconLarge,
                           HICON   *phiconSmall,
                           UINT    nIconSize) PURE;
};

typedef IExtractIconW * LPEXTRACTICONW;

#ifdef UNICODE
#define IExtractIcon        IExtractIconW
#define IExtractIconVtbl    IExtractIconWVtbl
#define LPEXTRACTICON       LPEXTRACTICONW
#else
#define IExtractIcon        IExtractIconA
#define IExtractIconVtbl    IExtractIconAVtbl
#define LPEXTRACTICON       LPEXTRACTICONA
#endif

//===========================================================================
//
// IShellIcon Interface
//
// used to get a icon index for a IShellFolder object.
//
// this interface can be implemented by a IShellFolder, as a quick way to
// return the icon for a object in the folder.
//
// a instance of this interface is only created once for the folder, unlike
// IExtractIcon witch is created once for each object.
//
// if a ShellFolder does not implement this interface, the standard
// GetUIObject(....IExtractIcon) method will be used to get a icon
// for all objects.
//
// the following standard imagelist indexs can be returned:
//
//      0   document (blank page) (not associated)
//      1   document (with stuff on the page)
//      2   application (exe, com, bat)
//      3   folder (plain)
//      4   folder (open)
//
// IShellIcon:GetIconOf(pidl, flags, lpIconIndex)
//
//      pidl            object to get icon for.
//      flags           GIL_* input flags (GIL_OPEN, ...)
//      lpIconIndex     place to return icon index.
//
//  returns:
//      NOERROR, if lpIconIndex contains the correct system imagelist index.
//      S_FALSE, if unable to get icon for this object, go through
//               GetUIObject, IExtractIcon, methods.
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IShellIcon

DECLARE_INTERFACE_(IShellIcon, IUnknown)      // shi
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellIcon methods ***
    STDMETHOD(GetIconOf)(THIS_ LPCITEMIDLIST pidl, UINT flags,
                    LPINT lpIconIndex) PURE;
};

typedef IShellIcon *LPSHELLICON;

//===========================================================================
//
// IShellLink Interface
//
//===========================================================================

#ifdef UNICODE
#define IShellLink      IShellLinkW
#define IShellLinkVtbl  IShellLinkWVtbl
#else
#define IShellLink      IShellLinkA
#define IShellLinkVtbl  IShellLinkAVtbl
#endif

// IShellLink::Resolve fFlags
typedef enum {
    SLR_NO_UI           = 0x0001,
    SLR_ANY_MATCH       = 0x0002,
    SLR_UPDATE          = 0x0004,
} SLR_FLAGS;

// IShellLink::GetPath fFlags
typedef enum {
    SLGP_SHORTPATH      = 0x0001,
    SLGP_UNCPRIORITY    = 0x0002,
} SLGP_FLAGS;

#undef  INTERFACE
#define INTERFACE   IShellLinkA

DECLARE_INTERFACE_(IShellLinkA, IUnknown)       // sl
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(GetPath)(THIS_ LPSTR pszFile, int cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags) PURE;

    STDMETHOD(GetIDList)(THIS_ LPITEMIDLIST * ppidl) PURE;
    STDMETHOD(SetIDList)(THIS_ LPCITEMIDLIST pidl) PURE;

    STDMETHOD(GetDescription)(THIS_ LPSTR pszName, int cchMaxName) PURE;
    STDMETHOD(SetDescription)(THIS_ LPCSTR pszName) PURE;

    STDMETHOD(GetWorkingDirectory)(THIS_ LPSTR pszDir, int cchMaxPath) PURE;
    STDMETHOD(SetWorkingDirectory)(THIS_ LPCSTR pszDir) PURE;

    STDMETHOD(GetArguments)(THIS_ LPSTR pszArgs, int cchMaxPath) PURE;
    STDMETHOD(SetArguments)(THIS_ LPCSTR pszArgs) PURE;

    STDMETHOD(GetHotkey)(THIS_ WORD *pwHotkey) PURE;
    STDMETHOD(SetHotkey)(THIS_ WORD wHotkey) PURE;

    STDMETHOD(GetShowCmd)(THIS_ int *piShowCmd) PURE;
    STDMETHOD(SetShowCmd)(THIS_ int iShowCmd) PURE;

    STDMETHOD(GetIconLocation)(THIS_ LPSTR pszIconPath, int cchIconPath, int *piIcon) PURE;
    STDMETHOD(SetIconLocation)(THIS_ LPCSTR pszIconPath, int iIcon) PURE;

    STDMETHOD(SetRelativePath)(THIS_ LPCSTR pszPathRel, DWORD dwReserved) PURE;

    STDMETHOD(Resolve)(THIS_ HWND hwnd, DWORD fFlags) PURE;

    STDMETHOD(SetPath)(THIS_ LPCSTR pszFile) PURE;
};

#undef  INTERFACE
#define INTERFACE   IShellLinkW

DECLARE_INTERFACE_(IShellLinkW, IUnknown)       // sl
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(GetPath)(THIS_ LPWSTR pszFile, int cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags) PURE;

    STDMETHOD(GetIDList)(THIS_ LPITEMIDLIST * ppidl) PURE;
    STDMETHOD(SetIDList)(THIS_ LPCITEMIDLIST pidl) PURE;

    STDMETHOD(GetDescription)(THIS_ LPWSTR pszName, int cchMaxName) PURE;
    STDMETHOD(SetDescription)(THIS_ LPCWSTR pszName) PURE;

    STDMETHOD(GetWorkingDirectory)(THIS_ LPWSTR pszDir, int cchMaxPath) PURE;
    STDMETHOD(SetWorkingDirectory)(THIS_ LPCWSTR pszDir) PURE;

    STDMETHOD(GetArguments)(THIS_ LPWSTR pszArgs, int cchMaxPath) PURE;
    STDMETHOD(SetArguments)(THIS_ LPCWSTR pszArgs) PURE;

    STDMETHOD(GetHotkey)(THIS_ WORD *pwHotkey) PURE;
    STDMETHOD(SetHotkey)(THIS_ WORD wHotkey) PURE;

    STDMETHOD(GetShowCmd)(THIS_ int *piShowCmd) PURE;
    STDMETHOD(SetShowCmd)(THIS_ int iShowCmd) PURE;

    STDMETHOD(GetIconLocation)(THIS_ LPWSTR pszIconPath, int cchIconPath, int *piIcon) PURE;
    STDMETHOD(SetIconLocation)(THIS_ LPCWSTR pszIconPath, int iIcon) PURE;

    STDMETHOD(SetRelativePath)(THIS_ LPCWSTR pszPathRel, DWORD dwReserved) PURE;

    STDMETHOD(Resolve)(THIS_ HWND hwnd, DWORD fFlags) PURE;

    STDMETHOD(SetPath)(THIS_ LPCWSTR pszFile) PURE;
};

#ifdef _INC_SHELLAPI    /* for LPSHELLEXECUTEINFO */
//===========================================================================
//
// IShellExecuteHook Interface
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IShellExecuteHookA

DECLARE_INTERFACE_(IShellExecuteHookA, IUnknown) // shexhk
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // *** IShellExecuteHookA methods ***
    STDMETHOD(Execute)(THIS_ LPSHELLEXECUTEINFOA pei) PURE;
};

#undef  INTERFACE
#define INTERFACE   IShellExecuteHookW

DECLARE_INTERFACE_(IShellExecuteHookW, IUnknown) // shexhk
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // *** IShellExecuteHookW methods ***
    STDMETHOD(Execute)(THIS_ LPSHELLEXECUTEINFOW pei) PURE;
};

#ifdef UNICODE
#define IShellExecuteHook       IShellExecuteHookW
#define IShellExecuteHookVtbl   IShellExecuteHookWVtbl
#else
#define IShellExecuteHook       IShellExecuteHookA
#define IShellExecuteHookVtbl   IShellExecuteHookAVtbl
#endif
#endif

//===========================================================================
//
// IURLSearchHook Interface
//
// IURLSearchHook Interface is called whenever a non standard URL is passed in
// the API -- IURLQualify. It can be used to redirect the user's request to a search
// engine or a specific web site.
// Non standard URL: URLS that does not have a protocal prefix -- "www.microsoft.com" and
// the protocal cannot be easily guessed by ApplyURLPrefix -- "home.microsoft.com"
//
// The parameters of Translate(..)
//    lpwszSearchURL -- (IN/OUT) Wide char buffer that contains the request "URL" user
//                      typed in as input and the tranlated URL as output.
//    cchBufferSize  -- (IN) size of lpwszSearchURL
//
// Return Values:
// S_OK         Search handled completely, pszResult has the full URL to browse to.
//              Stop running any further IURLSearchHooks and pass this URL back to
//              the browser for browsing.
//
// S_FALSE      Query has been preprocessed, pszResult has the result of the preprocess,
//              further search still needed. Go on executing the rest of the IURLSearchHooks
//              The preprocessing steps can be: 1. replaced certain characters
//                                              2. added more hints
//
// E_ABORT      Search handled completely, stop running any further IURLSearchHooks,
//              but NO BROWSING NEEDED as a result, pszResult is a copy of pcszQuery.
//              BUGBUG: This is not fully implemented, yet, making IURLQualify return this
//              involves too much change.
//
// BUGBUG:: E_ABORT is currently treated as E_FAIL. It requires too much change.
//
// E_FAIL       This Hook was unsuccessful. Search not handled at all, pcszQueryURL has the
//              query string. Please go on running other IURLSearchHooks.
//
// BUGBUG:  There is a potential danger in this Interface, that is people can write
//          very bad SearchHooks, but we(Microsoft) take the blaim.
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IURLSearchHook

DECLARE_INTERFACE_(IURLSearchHook, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // *** IURLSearchHook methods ***
    STDMETHOD(Translate)(LPWSTR lpwszSearchURL, DWORD cchBufferSize) PURE;
};

//===========================================================================
//
// INewShortcutHook Interface
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   INewShortcutHookA

DECLARE_INTERFACE_(INewShortcutHookA, IUnknown) // nshhk
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // *** INewShortcutHook methods ***
    STDMETHOD(SetReferent)(THIS_ LPCSTR pcszReferent, HWND hwnd) PURE;
    STDMETHOD(GetReferent)(THIS_ LPSTR pszReferent, int cchReferent) PURE;
    STDMETHOD(SetFolder)(THIS_ LPCSTR pcszFolder) PURE;
    STDMETHOD(GetFolder)(THIS_ LPSTR pszFolder, int cchFolder) PURE;
    STDMETHOD(GetName)(THIS_ LPSTR pszName, int cchName) PURE;
    STDMETHOD(GetExtension)(THIS_ LPSTR pszExtension, int cchExtension) PURE;
};

#undef  INTERFACE
#define INTERFACE   INewShortcutHookW

DECLARE_INTERFACE_(INewShortcutHookW, IUnknown) // nshhk
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // *** INewShortcutHook methods ***
    STDMETHOD(SetReferent)(THIS_ LPCWSTR pcszReferent, HWND hwnd) PURE;
    STDMETHOD(GetReferent)(THIS_ LPWSTR pszReferent, int cchReferent) PURE;
    STDMETHOD(SetFolder)(THIS_ LPCWSTR pcszFolder) PURE;
    STDMETHOD(GetFolder)(THIS_ LPWSTR pszFolder, int cchFolder) PURE;
    STDMETHOD(GetName)(THIS_ LPWSTR pszName, int cchName) PURE;
    STDMETHOD(GetExtension)(THIS_ LPWSTR pszExtension, int cchExtension) PURE;
};

#ifdef UNICODE
#define INewShortcutHook        INewShortcutHookW
#define INewShortcutHookVtbl    INewShortcutHookWVtbl
#else
#define INewShortcutHook        INewShortcutHookA
#define INewShortcutHookVtbl    INewShortcutHookAVtbl
#endif

//===========================================================================
//
// ICopyHook Interface
//
//  The copy hook is called whenever file system directories are
//  copy/moved/deleted/renamed via the shell.  It is also called by the shell
//  on changes of status of printers.
//
//  Clients register their id under STRREG_SHEX_COPYHOOK for file system hooks
//  and STRREG_SHEx_PRNCOPYHOOK for printer hooks.
//  the CopyCallback is called prior to the action, so the hook has the chance
//  to allow, deny or cancel the operation by returning the falues:
//     IDYES  -  means allow the operation
//     IDNO   -  means disallow the operation on this file, but continue with
//              any other operations (eg. batch copy)
//     IDCANCEL - means disallow the current operation and cancel any pending
//              operations
//
//   arguments to the CopyCallback
//      hwnd - window to use for any UI
//      wFunc - what operation is being done
//      wFlags - and flags (FOF_*) set in the initial call to the file operation
//      pszSrcFile - name of the source file
//      dwSrcAttribs - file attributes of the source file
//      pszDestFile - name of the destiation file (for move and renames)
//      dwDestAttribs - file attributes of the destination file
//
//
//===========================================================================

#ifndef FO_MOVE //these need to be kept in sync with the ones in shellapi.h

// file operations

#define FO_MOVE           0x0001
#define FO_COPY           0x0002
#define FO_DELETE         0x0003
#define FO_RENAME         0x0004

#define FOF_MULTIDESTFILES         0x0001
#define FOF_CONFIRMMOUSE           0x0002
#define FOF_SILENT                 0x0004  // don't create progress/report
#define FOF_RENAMEONCOLLISION      0x0008
#define FOF_NOCONFIRMATION         0x0010  // Don't prompt the user.
#define FOF_WANTMAPPINGHANDLE      0x0020  // Fill in SHFILEOPSTRUCT.hNameMappings
                                      // Must be freed using SHFreeNameMappings
#define FOF_ALLOWUNDO              0x0040
#define FOF_FILESONLY              0x0080  // on *.*, do only files
#define FOF_SIMPLEPROGRESS         0x0100  // means don't show names of files
#define FOF_NOCONFIRMMKDIR         0x0200  // don't confirm making any needed dirs
#define FOF_NOERRORUI              0x0400  // don't put up error UI

typedef UINT FILEOP_FLAGS;

// printer operations

#define PO_DELETE       0x0013  // printer is being deleted
#define PO_RENAME       0x0014  // printer is being renamed
#define PO_PORTCHANGE   0x0020  // port this printer connected to is being changed
                                // if this id is set, the strings received by
                                // the copyhook are a doubly-null terminated
                                // list of strings.  The first is the printer
                                // name and the second is the printer port.
#define PO_REN_PORT     0x0034  // PO_RENAME and PO_PORTCHANGE at same time.

// no POF_ flags currently defined

typedef UINT PRINTEROP_FLAGS;

#endif // FO_MOVE

#undef  INTERFACE
#define INTERFACE   ICopyHookA

DECLARE_INTERFACE_(ICopyHookA, IUnknown)        // sl
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD_(UINT,CopyCallback) (THIS_ HWND hwnd, UINT wFunc, UINT wFlags, LPCSTR pszSrcFile, DWORD dwSrcAttribs,
                                   LPCSTR pszDestFile, DWORD dwDestAttribs) PURE;
};

typedef ICopyHookA *    LPCOPYHOOKA;

#undef  INTERFACE
#define INTERFACE   ICopyHookW

DECLARE_INTERFACE_(ICopyHookW, IUnknown)        // sl
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD_(UINT,CopyCallback) (THIS_ HWND hwnd, UINT wFunc, UINT wFlags, LPCWSTR pszSrcFile, DWORD dwSrcAttribs,
                                   LPCWSTR pszDestFile, DWORD dwDestAttribs) PURE;
};

typedef ICopyHookW *    LPCOPYHOOKW;

#ifdef UNICODE
#define ICopyHook       ICopyHookW
#define ICopyHookVtbl   ICopyHookWVtbl
#define LPCOPYHOOK      LPCOPYHOOKW
#else
#define ICopyHook       ICopyHookA
#define ICopyHookVtbl   ICopyHookAVtbl
#define LPCOPYHOOK      LPCOPYHOOKA
#endif

//===========================================================================
//
// IFileViewerSite Interface
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IFileViewerSite

DECLARE_INTERFACE(IFileViewerSite)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(SetPinnedWindow) (THIS_ HWND hwnd) PURE;
    STDMETHOD(GetPinnedWindow) (THIS_ HWND *phwnd) PURE;
};

typedef IFileViewerSite * LPFILEVIEWERSITE;


//===========================================================================
//
// IFileViewer Interface
//
// Implemented in a FileViewer component object.  Used to tell a
// FileViewer to PrintTo or to view, the latter happening though
// ShowInitialize and Show.  The filename is always given to the
// viewer through IPersistFile.
//
//===========================================================================

typedef struct
{
    // Stuff passed into viewer (in)
    DWORD cbSize;           // Size of structure for future expansion...
    HWND hwndOwner;         // who is the owner window.
    int iShow;              // The show command

    // Passed in and updated  (in/Out)
    DWORD dwFlags;          // flags
    RECT rect;              // Where to create the window may have defaults
    LPUNKNOWN punkRel;      // Relese this interface when window is visible

    // Stuff that might be returned from viewer (out)
    OLECHAR strNewFile[MAX_PATH];   // New File to view.

} FVSHOWINFO, *LPFVSHOWINFO;

    // Define File View Show Info Flags.
#define FVSIF_RECT      0x00000001      // The rect variable has valid data.
#define FVSIF_PINNED    0x00000002      // We should Initialize pinned

#define FVSIF_NEWFAILED 0x08000000      // The new file passed back failed
                                        // to be viewed.

#define FVSIF_NEWFILE   0x80000000      // A new file to view has been returned
#define FVSIF_CANVIEWIT 0x40000000      // The viewer can view it.

#undef  INTERFACE
#define INTERFACE   IFileViewerA

DECLARE_INTERFACE(IFileViewerA)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(ShowInitialize) (THIS_ LPFILEVIEWERSITE lpfsi) PURE;
    STDMETHOD(Show) (THIS_ LPFVSHOWINFO pvsi) PURE;
    STDMETHOD(PrintTo) (THIS_ LPSTR pszDriver, BOOL fSuppressUI) PURE;
};

typedef IFileViewerA * LPFILEVIEWERA;

#undef  INTERFACE
#define INTERFACE   IFileViewerW

DECLARE_INTERFACE(IFileViewerW)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(ShowInitialize) (THIS_ LPFILEVIEWERSITE lpfsi) PURE;
    STDMETHOD(Show) (THIS_ LPFVSHOWINFO pvsi) PURE;
    STDMETHOD(PrintTo) (THIS_ LPWSTR pszDriver, BOOL fSuppressUI) PURE;
};

typedef IFileViewerW * LPFILEVIEWERW;

#ifdef UNICODE
#define IFileViewer IFileViewerW
#define LPFILEVIEWER LPFILEVIEWERW
#else
#define IFileViewer IFileViewerA
#define LPFILEVIEWER LPFILEVIEWERA
#endif



// CGID_ShellDocView Command Target IDs. for shell doc view wedge
enum {
    SHDVID_FINALTITLEAVAIL,     // variantIn bstr - sent after final OLECMDID_SETTITLE is sent
    SHDVID_MIMECSETMENUOPEN,    // mimecharset menu open commands
    SHDVID_PRINTFRAME,          // print HTML frame
    SHDVID_PUTOFFLINE,          // The Offline property has been changed
    SHDVID_PUTSILENT,           // The frame's Silent property has been changed
    SHDVID_GOBACK,              // Navigate Back
    SHDVID_GOFORWARD,           // Navigate Forward
    SHDVID_CANGOBACK,           // Is Back Navigation Possible?
    SHDVID_CANGOFORWARD,        // Is Forward Navigation Possible?
    SHDVID_CANACTIVATENOW,      // (down) (PICS) OK to navigate to this view now?
    SHDVID_ACTIVATEMENOW,       // (up) (PICS) Rating checks out, navigate now
    SHDVID_CANSUPPORTPICS,      // (down) variantIn I4: IOleCommandTarget to reply to
    SHDVID_PICSLABELFOUND,      // (up) variantIn bstr: PICS label
    SHDVID_NOMOREPICSLABELS,    // (up) End of document, no more PICS labels coming
};



//==========================================================================
//
// IShellBrowser/IShellView/IShellFolder interface
//
//  These three interfaces are used when the shell communicates with
// name space extensions. The shell (explorer) provides IShellBrowser
// interface, and extensions implements IShellFolder and IShellView
// interfaces.
//
//==========================================================================


//--------------------------------------------------------------------------
//
// Command/menuitem IDs
//
//  The explorer dispatches WM_COMMAND messages based on the range of
// command/menuitem IDs. All the IDs of menuitems that the view (right
// pane) inserts must be in FCIDM_SHVIEWFIRST/LAST (otherwise, the explorer
// won't dispatch them). The view should not deal with any menuitems
// in FCIDM_BROWSERFIRST/LAST (otherwise, it won't work with the future
// version of the shell).
//
//  FCIDM_SHVIEWFIRST/LAST      for the right pane (IShellView)
//  FCIDM_BROWSERFIRST/LAST     for the explorer frame (IShellBrowser)
//  FCIDM_GLOBAL/LAST           for the explorer's submenu IDs
//
//--------------------------------------------------------------------------

#define FCIDM_SHVIEWFIRST           0x0000
#define FCIDM_SHVIEWLAST            0x7fff
#define FCIDM_BROWSERFIRST          0xa000
#define FCIDM_BROWSERLAST           0xbf00
#define FCIDM_GLOBALFIRST           0x8000
#define FCIDM_GLOBALLAST            0x9fff

//
// Global submenu IDs and separator IDs
//
#define FCIDM_MENU_FILE             (FCIDM_GLOBALFIRST+0x0000)
#define FCIDM_MENU_EDIT             (FCIDM_GLOBALFIRST+0x0040)
#define FCIDM_MENU_VIEW             (FCIDM_GLOBALFIRST+0x0080)
#define FCIDM_MENU_VIEW_SEP_OPTIONS (FCIDM_GLOBALFIRST+0x0081)
#define FCIDM_MENU_TOOLS            (FCIDM_GLOBALFIRST+0x00c0)
#define FCIDM_MENU_TOOLS_SEP_GOTO   (FCIDM_GLOBALFIRST+0x00c1)
#define FCIDM_MENU_HELP             (FCIDM_GLOBALFIRST+0x0100)
#define FCIDM_MENU_FIND             (FCIDM_GLOBALFIRST+0x0140)
#define FCIDM_MENU_EXPLORE          (FCIDM_GLOBALFIRST+0x0150)
#define FCIDM_MENU_FAVORITES        (FCIDM_GLOBALFIRST+0x0170)

//--------------------------------------------------------------------------
// control IDs known to the view
//--------------------------------------------------------------------------

#define FCIDM_TOOLBAR      (FCIDM_BROWSERFIRST + 0)
#define FCIDM_STATUS       (FCIDM_BROWSERFIRST + 1)


//--------------------------------------------------------------------------
//
// FOLDERSETTINGS
//
//  FOLDERSETTINGS is a data structure that explorer passes from one folder
// view to another, when the user is browsing. It calls ISV::GetCurrentInfo
// member to get the current settings and pass it to ISV::CreateViewWindow
// to allow the next folder view "inherit" it. These settings assumes a
// particular UI (which the shell's folder view has), and shell extensions
// may or may not use those settings.
//
//--------------------------------------------------------------------------

typedef LPBYTE LPVIEWSETTINGS;

// NB Bitfields.
// FWF_DESKTOP implies FWF_TRANSPARENT/NOCLIENTEDGE/NOSCROLL
typedef enum
    {
    FWF_AUTOARRANGE =       0x0001,
    FWF_ABBREVIATEDNAMES =  0x0002,
    FWF_SNAPTOGRID =        0x0004,
    FWF_OWNERDATA =         0x0008,
    FWF_BESTFITWINDOW =     0x0010,
    FWF_DESKTOP =           0x0020,
    FWF_SINGLESEL =         0x0040,
    FWF_NOSUBFOLDERS =      0x0080,
    FWF_TRANSPARENT  =      0x0100,
    FWF_NOCLIENTEDGE =      0x0200,
    FWF_NOSCROLL     =      0x0400,
    FWF_ALIGNLEFT    =      0x0800,
    FWF_SINGLECLICKACTIVATE=0x8000  // TEMPORARY -- NO UI FOR THIS
    } FOLDERFLAGS;

typedef enum
    {
    FVM_ICON =              1,
    FVM_SMALLICON =         2,
    FVM_LIST =              3,
    FVM_DETAILS =           4,
    } FOLDERVIEWMODE;

typedef struct
    {
    UINT ViewMode;       // View mode (FOLDERVIEWMODE values)
    UINT fFlags;         // View options (FOLDERFLAGS bits)
    } FOLDERSETTINGS, *LPFOLDERSETTINGS;

typedef const FOLDERSETTINGS * LPCFOLDERSETTINGS;


//--------------------------------------------------------------------------
//
// Interface:   IShellBrowser
//
//  IShellBrowser interface is the interface that is provided by the shell
// explorer/folder frame window. When it creates the "contents pane" of
// a shell folder (which provides IShellFolder interface), it calls its
// CreateViewObject member function to create an IShellView object. Then,
// it calls its CreateViewWindow member to create the "contents pane"
// window. The pointer to the IShellBrowser interface is passed to
// the IShellView object as a parameter to this CreateViewWindow member
// function call.
//
//    +--------------------------+  <-- Explorer window
//    | [] Explorer              |
//    |--------------------------+       IShellBrowser
//    | File Edit View ..        |
//    |--------------------------|
//    |        |                 |
//    |        |              <-------- Content pane
//    |        |                 |
//    |        |                 |       IShellView
//    |        |                 |
//    |        |                 |
//    +--------------------------+
//
//
//
// [Member functions]
//
//
// IShellBrowser::GetWindow(phwnd)
//
//   Inherited from IOleWindow::GetWindow.
//
//
// IShellBrowser::ContextSensitiveHelp(fEnterMode)
//
//   Inherited from IOleWindow::ContextSensitiveHelp.
//
//
// IShellBrowser::InsertMenusSB(hmenuShared, lpMenuWidths)
//
//   Similar to the IOleInPlaceFrame::InsertMenus. The explorer will put
//  "File" and "Edit" pulldown in the File menu group, "View" and "Tools"
//  in the Container menu group and "Help" in the Window menu group. Each
//  pulldown menu will have a uniqu ID, FCIDM_MENU_FILE/EDIT/VIEW/TOOLS/HELP.
//  The view is allowed to insert menuitems into those sub-menus by those
//  IDs must be between FCIDM_SHVIEWFIRST and FCIDM_SHVIEWLAST.
//
//
// IShellBrowser::SetMenuSB(hmenuShared, holemenu, hwndActiveObject)
//
//   Similar to the IOleInPlaceFrame::SetMenu. The explorer ignores the
//  holemenu parameter (reserved for future enhancement)  and performs
//  menu-dispatch based on the menuitem IDs (see the description above).
//  It is important to note that the explorer will add different
//  set of menuitems depending on whether the view has a focus or not.
//  Therefore, it is very important to call ISB::OnViewWindowActivate
//  whenever the view window (or its children) gets the focus.
//
//
// IShellBrowser::RemoveMenusSB(hmenuShared)
//
//   Same as the IOleInPlaceFrame::RemoveMenus.
//
//
// IShellBrowser::SetStatusTextSB(lpszStatusText)
//
//   Same as the IOleInPlaceFrame::SetStatusText. It is also possible to
//  send messages directly to the status window via SendControlMsg.
//
//
// IShellBrowser::EnableModelessSB(fEnable)
//
//   Same as the IOleInPlaceFrame::EnableModeless.
//
//
// IShellBrowser::TranslateAcceleratorSB(lpmsg, wID)
//
//   Same as the IOleInPlaceFrame::TranslateAccelerator, but will be
//  never called because we don't support EXEs (i.e., the explorer has
//  the message loop). This member function is defined here for possible
//  future enhancement.
//
//
// IShellBrowser::BrowseObject(pidl, wFlags)
//
//   The view calls this member to let shell explorer browse to another
//  folder. The pidl and wFlags specifies the folder to be browsed.
//
//  Following three flags specifies whether it creates another window or not.
//   SBSP_SAMEBROWSER  -- Browse to another folder with the same window.
//   SBSP_NEWBROWSER   -- Creates another window for the specified folder.
//   SBSP_DEFBROWSER   -- Default behavior (respects the view option).
//
//  Following three flags specifies open, explore, or default mode. These   .
//  are ignored if SBSP_SAMEBROWSER or (SBSP_DEFBROWSER && (single window   .
//  browser || explorer)).                                                  .
//   SBSP_OPENMODE     -- Use a normal folder window
//   SBSP_EXPLOREMODE  -- Use an explorer window
//   SBSP_DEFMODE      -- Use the same as the current window
//
//  Following three flags specifies the pidl.
//   SBSP_ABSOLUTE -- pidl is an absolute pidl (relative from desktop)
//   SBSP_RELATIVE -- pidl is relative from the current folder.
//   SBSP_PARENT   -- Browse the parent folder (ignores the pidl)
//   SBSP_NAVIGATEBACK    -- Navigate back (ignores the pidl)
//   SBSP_NAVIGATEFORWARD -- Navigate forward (ignores the pidl)
//
//
// IShellBrowser::GetViewStateStream(grfMode, ppstm)
//
//   The browser returns an IStream interface as the storage for view
//  specific state information.
//
//   grfMode -- Specifies the read/write access (STGM_READ/WRITE/READWRITE)
//   ppstm   -- Specifies the LPSTREAM variable to be filled.
//
//
// IShellBrowser::GetControlWindow(id, phwnd)
//
//   The shell view may call this member function to get the window handle
//  of Explorer controls (toolbar or status winodw -- FCW_TOOLBAR or
//  FCW_STATUS).
//
//
// IShellBrowser::SendControlMsg(id, uMsg, wParam, lParam, pret)
//
//   The shell view calls this member function to send control messages to
//  one of Explorer controls (toolbar or status window -- FCW_TOOLBAR or
//  FCW_STATUS).
//
//
// IShellBrowser::QueryActiveShellView(IShellView * ppshv)
//
//   This member returns currently activated (displayed) shellview object.
//  A shellview never need to call this member function.
//
//
// IShellBrowser::OnViewWindowActive(pshv)
//
//   The shell view window calls this member function when the view window
//  (or one of its children) got the focus. It MUST call this member before
//  calling IShellBrowser::InsertMenus, because it will insert different
//  set of menu items depending on whether the view has the focus or not.
//
//
// IShellBrowser::SetToolbarItems(lpButtons, nButtons, uFlags)
//
//   The view calls this function to add toolbar items to the exporer's
//  toolbar. "lpButtons" and "nButtons" specifies the array of toolbar
//  items. "uFlags" must be one of FCT_MERGE, FCT_CONFIGABLE, FCT_ADDTOEND.
//
//-------------------------------------------------------------------------

#undef  INTERFACE
#define INTERFACE   IShellBrowser

//
// Values for wFlags parameter of ISB::BrowseObject() member.
//
#define SBSP_DEFBROWSER         0x0000
#define SBSP_SAMEBROWSER        0x0001
#define SBSP_NEWBROWSER         0x0002

#define SBSP_DEFMODE            0x0000
#define SBSP_OPENMODE           0x0010
#define SBSP_EXPLOREMODE        0x0020

#define SBSP_ABSOLUTE           0x0000
#define SBSP_RELATIVE           0x1000
#define SBSP_PARENT             0x2000
#define SBSP_NAVIGATEBACK       0x4000
#define SBSP_NAVIGATEFORWARD    0x8000

#define SBSP_INITIATEDBYHLINKFRAME        0x80000000
#define SBSP_REDIRECT                     0x40000000

//
// Values for id parameter of ISB::GetWindow/SendControlMsg members.
//
// WARNING:
//  Any shell extensions which sends messages to those control windows
// might not work in the future version of windows. If you really need
// to send messages to them, (1) don't assume that those control window
// always exist (i.e. GetControlWindow may fail) and (2) verify the window
// class of the window before sending any messages.
//
#define FCW_STATUS      0x0001
#define FCW_TOOLBAR     0x0002
#define FCW_TREE        0x0003
#define FCW_INTERNETBAR 0x0006

//
// Values for uFlags paremeter of ISB::SetToolbarItems member.
//
#define FCT_MERGE       0x0001
#define FCT_CONFIGABLE  0x0002
#define FCT_ADDTOEND    0x0004


DECLARE_INTERFACE_(IShellBrowser, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IShellBrowser methods *** (same as IOleInPlaceFrame)
    STDMETHOD(InsertMenusSB) (THIS_ HMENU hmenuShared,
                                LPOLEMENUGROUPWIDTHS lpMenuWidths) PURE;
    STDMETHOD(SetMenuSB) (THIS_ HMENU hmenuShared, HOLEMENU holemenuReserved,
                HWND hwndActiveObject) PURE;
    STDMETHOD(RemoveMenusSB) (THIS_ HMENU hmenuShared) PURE;
    STDMETHOD(SetStatusTextSB) (THIS_ LPCOLESTR lpszStatusText) PURE;
    STDMETHOD(EnableModelessSB) (THIS_ BOOL fEnable) PURE;
    STDMETHOD(TranslateAcceleratorSB) (THIS_ LPMSG lpmsg, WORD wID) PURE;

    // *** IShellBrowser methods ***
    STDMETHOD(BrowseObject)(THIS_ LPCITEMIDLIST pidl, UINT wFlags) PURE;
    STDMETHOD(GetViewStateStream)(THIS_ DWORD grfMode,
                LPSTREAM  *ppStrm) PURE;
    STDMETHOD(GetControlWindow)(THIS_ UINT id, HWND * lphwnd) PURE;
    STDMETHOD(SendControlMsg)(THIS_ UINT id, UINT uMsg, WPARAM wParam,
                LPARAM lParam, LRESULT * pret) PURE;
    STDMETHOD(QueryActiveShellView)(THIS_ struct IShellView ** ppshv) PURE;
    STDMETHOD(OnViewWindowActive)(THIS_ struct IShellView * ppshv) PURE;
    STDMETHOD(SetToolbarItems)(THIS_ LPTBBUTTON lpButtons, UINT nButtons,
                UINT uFlags) PURE;
};

typedef IShellBrowser * LPSHELLBROWSER;

enum {
    SBSC_HIDE = 0,
    SBSC_SHOW = 1,
    SBSC_TOGGLE = 2,
    SBSC_QUERY =  3
};

// CGID_Explorer Command Target IDs
enum {
    SBCMDID_ENABLESHOWTREE          = 0,
    SBCMDID_SHOWCONTROL             = 1,        // variant vt_i4 = loword = FCW_* hiword = SBSC_*
    SBCMDID_CANCELNAVIGATION        = 2,        // cancel last navigation
    SBCMDID_MAYSAVECHANGES          = 3,        // about to close and may save changes
    SBCMDID_SETHLINKFRAME           = 4,        // variant vt_i4 = phlinkframe
    SBCMDID_ENABLESTOP              = 5,        // variant vt_bool = fEnable
    SBCMDID_OPTIONS                 = 6,        // the view.options page
    SBCMDID_EXPLORER                = 7,        // are you explorer.exe?
    SBCMDID_ADDTOFAVORITES          = 8,
    SBCMDID_ACTIVEOBJECTMENUS       = 9,
    SBCMDID_MAYSAVEVIEWSTATE        = 10,       // Should we save view stream
    SBCMDID_DOFAVORITESMENU         = 11,       // popup the favorites menu
    SBCMDID_DOMAILMENU              = 12,       // popup the mail menu
    SBCMDID_GETADDRESSBARTEXT       = 13,       // get user-typed text
    SBCMDID_ASYNCNAVIGATION         = 14,       // do an async navigation
    SBCMDID_SETSEARCHINFO           = 15,       // BUGBUG: temporary tonyci 12nov96
    SBCMDID_GETSEARCHINFO           = 16,       // BUGBUG: temporary tonyci 12nov96
};


//-------------------------------------------------------------------------
// ICommDlgBrowser interface
//
//  ICommDlgBrowser interface is the interface that is provided by the new
// common dialog window to hook and modify the behavior of IShellView.  When
// a default view is created, it queries its parent IShellBrowser for the
// ICommDlgBrowser interface.  If supported, it calls out to that interface
// in several cases that need to behave differently in a dialog.
//
// Member functions:
//
//  ICommDlgBrowser::OnDefaultCommand()
//    Called when the user double-clicks in the view or presses Enter.  The
//   browser should return S_OK if it processed the action itself, S_FALSE
//   to let the view perform the default action.
//
//  ICommDlgBrowser::OnStateChange(ULONG uChange)
//    Called when some states in the view change.  'uChange' is one of the
//   CDBOSC_* values.  This call is made after the state (selection, focus,
//   etc) has changed.  There is no return value.
//
//  ICommDlgBrowser::IncludeObject(LPCITEMIDLIST pidl)
//    Called when the view is enumerating objects.  'pidl' is a relative
//   IDLIST.  The browser should return S_OK to include the object in the
//   view, S_FALSE to hide it
//
//-------------------------------------------------------------------------

#undef  INTERFACE
#define INTERFACE   ICommDlgBrowser

#define CDBOSC_SETFOCUS     0x00000000
#define CDBOSC_KILLFOCUS    0x00000001
#define CDBOSC_SELCHANGE    0x00000002
#define CDBOSC_RENAME       0x00000003

DECLARE_INTERFACE_(ICommDlgBrowser, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** ICommDlgBrowser methods ***
    STDMETHOD(OnDefaultCommand) (THIS_ struct IShellView * ppshv) PURE;
    STDMETHOD(OnStateChange) (THIS_ struct IShellView * ppshv,
                ULONG uChange) PURE;
    STDMETHOD(IncludeObject) (THIS_ struct IShellView * ppshv,
                LPCITEMIDLIST pidl) PURE;
};

typedef ICommDlgBrowser * LPCOMMDLGBROWSER;


//==========================================================================
//
// Interface:   IShellView
//
// IShellView::GetWindow(phwnd)
//
//   Inherited from IOleWindow::GetWindow.
//
//
// IShellView::ContextSensitiveHelp(fEnterMode)
//
//   Inherited from IOleWindow::ContextSensitiveHelp.
//
//
// IShellView::TranslateAccelerator(lpmsg)
//
//   Similar to IOleInPlaceActiveObject::TranlateAccelerator. The explorer
//  calls this function BEFORE any other translation. Returning S_OK
//  indicates that the message was translated (eaten) and should not be
//  translated or dispatched by the explorer.
//
//
// IShellView::EnableModeless(fEnable)
//   Similar to IOleInPlaceActiveObject::EnableModeless.
//
//
// IShellView::UIActivate(uState)
//
//   The explorer calls this member function whenever the activation
//  state of the view window is changed by a certain event that is
//  NOT caused by the shell view itself.
//
//   SVUIA_DEACTIVATE will be passed when the explorer is about to
//  destroy the shell view window; the shell view is supposed to remove
//  all the extended UIs (typically merged menu and modeless popup windows).
//
//   SVUIA_ACTIVATE_NOFOCUS will be passsed when the shell view is losing
//  the input focus or the shell view has been just created without the
//  input focus; the shell view is supposed to set menuitems appropriate
//  for non-focused state (no selection specific items should be added).
//
//   SVUIA_ACTIVATE_FOCUS will be passed when the explorer has just
//  created the view window with the input focus; the shell view is
//  supposed to set menuitems appropriate for focused state.
//
//   SVUIA_INPLACEACTIVATE(new) will be passed when the shell view is opened
//  within an ActiveX control, which is not a UI active. In this case,
//  the shell view should not merge menus or put toolbas. To be compatible
//  with Win95 client, we don't pass this value unless the view supports
//  IShellView2.
//
//   The shell view should not change focus within this member function.
//  The shell view should not hook the WM_KILLFOCUS message to remerge
//  menuitems. However, the shell view typically hook the WM_SETFOCUS
//  message, and re-merge the menu after calling IShellBrowser::
//  OnViewWindowActivated.
//
//
// IShellView::Refresh()
//
//   The explorer calls this member when the view needs to refresh its
//  contents (such as when the user hits F5 key).
//
//
// IShellView::CreateViewWindow
//
//   This member creates the view window (right-pane of the explorer or the
//  client window of the folder window).
//
//
// IShellView::DestroyViewWindow
//
//   This member destroys the view window.
//
//
// IShellView::GetCurrentInfo
//
//   This member returns the folder settings.
//
//
// IShellView::AddPropertySHeetPages
//
//   The explorer calls this member when it is opening the option property
//  sheet. This allows the view to add additional pages to it.
//
//
// IShellView::SaveViewState()
//
//   The explorer calls this member when the shell view is supposed to
//  store its view settings. The shell view is supposed to get a view
//  stream by calling IShellBrowser::GetViewStateStream and store the
//  current view state into that stream.
//
//
// IShellView::SelectItem(pidlItem, uFlags)
//
//   The explorer calls this member to change the selection state of
//  item(s) within the shell view window.  If pidlItem is NULL and uFlags
//  is SVSI_DESELECTOTHERS, all items should be deselected.
//
//-------------------------------------------------------------------------

#undef  INTERFACE
#define INTERFACE   IShellView

//
// shellview select item flags
//
#define SVSI_DESELECT   0x0000
#define SVSI_SELECT     0x0001
#define SVSI_EDIT       0x0003  // includes select
#define SVSI_DESELECTOTHERS 0x0004
#define SVSI_ENSUREVISIBLE  0x0008
#define SVSI_FOCUSED        0x0010

//
// shellview get item object flags
//
#define SVGIO_BACKGROUND    0x00000000
#define SVGIO_SELECTION     0x00000001
#define SVGIO_ALLVIEW       0x00000002

//
// uState values for IShellView::UIActivate
//
typedef enum {
    SVUIA_DEACTIVATE       = 0,
    SVUIA_ACTIVATE_NOFOCUS = 1,
    SVUIA_ACTIVATE_FOCUS   = 2,
    SVUIA_INPLACEACTIVATE  = 3          // new flag for IShellView2
} SVUIA_STATUS;

DECLARE_INTERFACE_(IShellView, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IShellView methods ***
    STDMETHOD(TranslateAccelerator) (THIS_ LPMSG lpmsg) PURE;
#ifdef _FIX_ENABLEMODELESS_CONFLICT
    STDMETHOD(EnableModelessSV) (THIS_ BOOL fEnable) PURE;
#else
    STDMETHOD(EnableModeless) (THIS_ BOOL fEnable) PURE;
#endif
    STDMETHOD(UIActivate) (THIS_ UINT uState) PURE;
    STDMETHOD(Refresh) (THIS) PURE;

    STDMETHOD(CreateViewWindow)(THIS_ IShellView  *lpPrevView,
                    LPCFOLDERSETTINGS lpfs, IShellBrowser  * psb,
                    RECT * prcView, HWND  *phWnd) PURE;
    STDMETHOD(DestroyViewWindow)(THIS) PURE;
    STDMETHOD(GetCurrentInfo)(THIS_ LPFOLDERSETTINGS lpfs) PURE;
    STDMETHOD(AddPropertySheetPages)(THIS_ DWORD dwReserved,
                    LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam) PURE;
    STDMETHOD(SaveViewState)(THIS) PURE;
    STDMETHOD(SelectItem)(THIS_ LPCITEMIDLIST pidlItem, UINT uFlags) PURE;
    STDMETHOD(GetItemObject)(THIS_ UINT uItem, REFIID riid,
                    LPVOID *ppv) PURE;
};

typedef IShellView *    LPSHELLVIEW;

typedef GUID SHELLVIEWID;

#define SV2GV_CURRENTVIEW ((UINT)-1)
#define SV2GV_DEFAULTVIEW ((UINT)-2)

typedef struct _SV2CVW2_PARAMS
{
    DWORD cbSize;

    IShellView *psvPrev;
    FOLDERSETTINGS const *pfs;
    IShellBrowser *psbOwner;
    RECT *prcView;
    SHELLVIEWID const *pvid;

    HWND hwndView;
} SV2CVW2_PARAMS;
typedef SV2CVW2_PARAMS *LPSV2CVW2_PARAMS;

#undef  INTERFACE
#define INTERFACE   IShellView2

DECLARE_INTERFACE_(IShellView2, IShellView)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IShellView methods ***
    STDMETHOD(TranslateAccelerator) (THIS_ LPMSG lpmsg) PURE;
#ifdef _FIX_ENABLEMODELESS_CONFLICT
    STDMETHOD(EnableModelessSV) (THIS_ BOOL fEnable) PURE;
#else
    STDMETHOD(EnableModeless) (THIS_ BOOL fEnable) PURE;
#endif
    STDMETHOD(UIActivate) (THIS_ UINT uState) PURE;
    STDMETHOD(Refresh) (THIS) PURE;

    STDMETHOD(CreateViewWindow)(THIS_ IShellView  *lpPrevView,
                    LPCFOLDERSETTINGS lpfs, IShellBrowser  * psb,
                    RECT * prcView, HWND  *phWnd) PURE;
    STDMETHOD(DestroyViewWindow)(THIS) PURE;
    STDMETHOD(GetCurrentInfo)(THIS_ LPFOLDERSETTINGS lpfs) PURE;
    STDMETHOD(AddPropertySheetPages)(THIS_ DWORD dwReserved,
                    LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam) PURE;
    STDMETHOD(SaveViewState)(THIS) PURE;
    STDMETHOD(SelectItem)(THIS_ LPCITEMIDLIST pidlItem, UINT uFlags) PURE;
    STDMETHOD(GetItemObject)(THIS_ UINT uItem, REFIID riid,
                    LPVOID *ppv) PURE;

    // *** IShellView2 methods ***
    STDMETHOD(GetView)(THIS_ SHELLVIEWID* pvid, ULONG uView) PURE;
    STDMETHOD(CreateViewWindow2)(THIS_ LPSV2CVW2_PARAMS lpParams) PURE;
    STDMETHOD(HandleRename)(THIS_ LPCITEMIDLIST pidlNew) PURE;
};

//-------------------------------------------------------------------------
//
// struct STRRET
//
// structure for returning strings from IShellFolder member functions
//
//-------------------------------------------------------------------------
#define STRRET_WSTR     0x0000          // Use STRRET.pOleStr
#define STRRET_OFFSET   0x0001          // Use STRRET.uOffset to Ansi
#define STRRET_CSTR     0x0002          // Use STRRET.cStr


typedef struct _STRRET
{
    UINT uType; // One of the STRRET_* values
    union
    {
        LPWSTR          pOleStr;        // OLESTR that will be freed
        LPSTR           pStr;           // ANSI string that will be freed (needed?)
        UINT            uOffset;        // Offset into SHITEMID
        char            cStr[MAX_PATH]; // Buffer to fill in (ANSI)
    } DUMMYUNIONNAME;
} STRRET, *LPSTRRET;


//-------------------------------------------------------------------------
//
// SHGetPathFromIDList
//
//  This function assumes the size of the buffer (MAX_PATH). The pidl
// should point to a file system object.
//
//-------------------------------------------------------------------------

WINSHELLAPI BOOL WINAPI SHGetPathFromIDListA(LPCITEMIDLIST pidl, LPSTR pszPath);
WINSHELLAPI BOOL WINAPI SHGetPathFromIDListW(LPCITEMIDLIST pidl, LPWSTR pszPath);

#ifdef UNICODE
#define SHGetPathFromIDList SHGetPathFromIDListW
#else
#define SHGetPathFromIDList SHGetPathFromIDListA
#endif


//-------------------------------------------------------------------------
//
// SHGetSpecialFolderLocation
//
//  Caller should call SHFree to free the returned pidl.
//
//-------------------------------------------------------------------------
//
// registry entries for special paths are kept in :
#define REGSTR_PATH_SPECIAL_FOLDERS    REGSTR_PATH_EXPLORER TEXT("\\Shell Folders")



#define CSIDL_DESKTOP                   0x0000
#define CSIDL_INTERNET                  0x0001
#define CSIDL_PROGRAMS                  0x0002
#define CSIDL_CONTROLS                  0x0003
#define CSIDL_PRINTERS                  0x0004
#define CSIDL_PERSONAL                  0x0005
#define CSIDL_FAVORITES                 0x0006
#define CSIDL_STARTUP                   0x0007
#define CSIDL_RECENT                    0x0008
#define CSIDL_SENDTO                    0x0009
#define CSIDL_BITBUCKET                 0x000a
#define CSIDL_STARTMENU                 0x000b
#define CSIDL_DESKTOPDIRECTORY          0x0010
#define CSIDL_DRIVES                    0x0011
#define CSIDL_NETWORK                   0x0012
#define CSIDL_NETHOOD                   0x0013
#define CSIDL_FONTS                     0x0014
#define CSIDL_TEMPLATES                 0x0015
#define CSIDL_COMMON_STARTMENU          0x0016
#define CSIDL_COMMON_PROGRAMS           0X0017
#define CSIDL_COMMON_STARTUP            0x0018
#define CSIDL_COMMON_DESKTOPDIRECTORY   0x0019
#define CSIDL_APPDATA                   0x001a
#define CSIDL_PRINTHOOD                 0x001b

WINSHELLAPI HRESULT WINAPI SHGetSpecialFolderLocation(HWND hwndOwner, int nFolder, LPITEMIDLIST * ppidl);

//-------------------------------------------------------------------------
//
// SHBrowseForFolder API
//
//-------------------------------------------------------------------------

typedef int (CALLBACK* BFFCALLBACK)(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

typedef struct _browseinfoA {
    HWND        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPSTR        pszDisplayName;// Return display name of item selected.
    LPCSTR       lpszTitle;      // text to go in the banner over the tree.
    UINT         ulFlags;       // Flags that control the return stuff
    BFFCALLBACK  lpfn;
    LPARAM      lParam;         // extra info that's passed back in callbacks

    int          iImage;      // output var: where to return the Image index.
} BROWSEINFOA, *PBROWSEINFOA, *LPBROWSEINFOA;

typedef struct _browseinfoW {
    HWND        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPWSTR       pszDisplayName;// Return display name of item selected.
    LPCWSTR      lpszTitle;      // text to go in the banner over the tree.
    UINT         ulFlags;       // Flags that control the return stuff
    BFFCALLBACK  lpfn;
    LPARAM      lParam;         // extra info that's passed back in callbacks

    int          iImage;      // output var: where to return the Image index.
} BROWSEINFOW, *PBROWSEINFOW, *LPBROWSEINFOW;

#ifdef UNICODE
#define BROWSEINFO      BROWSEINFOW
#define PBROWSEINFO     PBROWSEINFOW
#define LPBROWSEINFO    LPBROWSEINFOW
#else
#define BROWSEINFO      BROWSEINFOA
#define PBROWSEINFO     PBROWSEINFOA
#define LPBROWSEINFO    LPBROWSEINFOA
#endif

// Browsing for directory.
#define BIF_RETURNONLYFSDIRS   0x0001  // For finding a folder to start document searching
#define BIF_DONTGOBELOWDOMAIN  0x0002  // For starting the Find Computer
#define BIF_STATUSTEXT         0x0004
#define BIF_RETURNFSANCESTORS  0x0008

#define BIF_BROWSEFORCOMPUTER  0x1000  // Browsing for Computers.
#define BIF_BROWSEFORPRINTER   0x2000  // Browsing for Printers
#define BIF_BROWSEINCLUDEFILES 0x4000  // Browsing for Everything

// message from browser
#define BFFM_INITIALIZED        1
#define BFFM_SELCHANGED         2

// messages to browser
#define BFFM_SETSTATUSTEXTA     (WM_USER + 100)
#define BFFM_ENABLEOK           (WM_USER + 101)
#define BFFM_SETSELECTIONA      (WM_USER + 102)
#define BFFM_SETSELECTIONW      (WM_USER + 103)
#define BFFM_SETSTATUSTEXTW     (WM_USER + 104)

WINSHELLAPI LPITEMIDLIST WINAPI SHBrowseForFolderA(LPBROWSEINFOA lpbi);
WINSHELLAPI LPITEMIDLIST WINAPI SHBrowseForFolderW(LPBROWSEINFOW lpbi);

#ifdef UNICODE
#define SHBrowseForFolder   SHBrowseForFolderW
#define BFFM_SETSTATUSTEXT  BFFM_SETSTATUSTEXTW
#define BFFM_SETSELECTION   BFFM_SETSELECTIONW
#else
#define SHBrowseForFolder   SHBrowseForFolderA
#define BFFM_SETSTATUSTEXT  BFFM_SETSTATUSTEXTA
#define BFFM_SETSELECTION   BFFM_SETSELECTIONA
#endif

//-------------------------------------------------------------------------
//
// SHLoadInProc
//
//   When this function is called, the shell calls CoCreateInstance
//  (or equivalent) with CLSCTX_INPROC_SERVER and the specified CLSID
//  from within the shell's process and release it immediately.
//
//-------------------------------------------------------------------------

WINSHELLAPI HRESULT WINAPI SHLoadInProc(REFCLSID rclsid);


//-------------------------------------------------------------------------
//
// IEnumIDList interface
//
//  IShellFolder::EnumObjects member returns an IEnumIDList object.
//
//-------------------------------------------------------------------------

typedef struct IEnumIDList      *LPENUMIDLIST;

#undef  INTERFACE
#define INTERFACE       IEnumIDList

DECLARE_INTERFACE_(IEnumIDList, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IEnumIDList methods ***
    STDMETHOD(Next)  (THIS_ ULONG celt,
                      LPITEMIDLIST *rgelt,
                      ULONG *pceltFetched) PURE;
    STDMETHOD(Skip)  (THIS_ ULONG celt) PURE;
    STDMETHOD(Reset) (THIS) PURE;
    STDMETHOD(Clone) (THIS_ IEnumIDList **ppenum) PURE;
};


//-------------------------------------------------------------------------
//
// IShellFolder interface
//
//
// [Member functions]
//
// IShellFolder::BindToObject(pidl, pbc, riid, ppvOut)
//   This function returns an instance of a sub-folder which is specified
//  by the IDList (pidl).
//
// IShellFolder::BindToStorage(pidl, pbc, riid, ppvObj)
//   This function returns a storage instance of a sub-folder which is
//  specified by the IDList (pidl). The shell never calls this member
//  function in the first release of Win95.
//
// IShellFolder::CompareIDs(lParam, pidl1, pidl2)
//   This function compares two IDLists and returns the result. The shell
//  explorer always passes 0 as lParam, which indicates "sort by name".
//  It should return 0 (as CODE of the scode), if two id indicates the
//  same object; negative value if pidl1 should be placed before pidl2;
//  positive value if pidl2 should be placed before pidl1.
//
// IShellFolder::CreateViewObject(hwndOwner, riid, ppvOut)
//   This function creates a view object of the folder itself. The view
//  object is a difference instance from the shell folder object.
//   "hwndOwner" can be used  as the owner window of its dialog box or
//  menu during the lifetime of the view object.
//  instance which has only one reference count. The explorer may create
//  more than one instances of view object from one shell folder object
//  and treat them as separate instances.
//
// IShellFolder::GetAttributesOf(cidl, apidl, prgfInOut)
//   This function returns the attributes of specified objects in that
//  folder. "cidl" and "apidl" specifies objects. "apidl" contains only
//  simple IDLists. The explorer initializes *prgfInOut with a set of
//  flags to be evaluated. The shell folder may optimize the operation
//  by not returning unspecified flags.
//
// IShellFolder::GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut)
//   This function creates a UI object to be used for specified objects.
//  The shell explorer passes either IID_IDataObject (for transfer operation)
//  or IID_IContextMenu (for context menu operation) as riid.
//
// IShellFolder::GetDisplayNameOf
//   This function returns the display name of the specified object.
//  If the ID contains the display name (in the locale character set),
//  it returns the offset to the name. Otherwise, it returns a pointer
//  to the display name string (UNICODE), which is allocated by the
//  task allocator, or fills in a buffer.
//
// IShellFolder::SetNameOf
//   This function sets the display name of the specified object.
//  If it changes the ID as well, it returns the new ID which is
//  alocated by the task allocator.
//
//-------------------------------------------------------------------------

#undef  INTERFACE
#define INTERFACE       IShellFolder

// IShellFolder::GetDisplayNameOf/SetNameOf uFlags
typedef enum tagSHGDN
{
    SHGDN_NORMAL            = 0,        // default (display purpose)
    SHGDN_INFOLDER          = 1,        // displayed under a folder (relative)
    SHGDN_FORADDRESSBAR     = 0x4000,   // for displaying in the address (drives dropdown) bar
    SHGDN_FORPARSING        = 0x8000,   // for ParseDisplayName or path
} SHGNO;

// IShellFolder::EnumObjects
typedef enum tagSHCONTF
{
    SHCONTF_FOLDERS         = 32,       // for shell browser
    SHCONTF_NONFOLDERS      = 64,       // for default view
    SHCONTF_INCLUDEHIDDEN   = 128,      // for hidden/system objects
} SHCONTF;

// IShellFolder::GetAttributesOf flags
#define SFGAO_CANCOPY           DROPEFFECT_COPY // Objects can be copied
#define SFGAO_CANMOVE           DROPEFFECT_MOVE // Objects can be moved
#define SFGAO_CANLINK           DROPEFFECT_LINK // Objects can be linked
#define SFGAO_CANRENAME         0x00000010L     // Objects can be renamed
#define SFGAO_CANDELETE         0x00000020L     // Objects can be deleted
#define SFGAO_HASPROPSHEET      0x00000040L     // Objects have property sheets
#define SFGAO_DROPTARGET        0x00000100L     // Objects are drop target
#define SFGAO_CAPABILITYMASK    0x00000177L
#define SFGAO_LINK              0x00010000L     // Shortcut (link)
#define SFGAO_SHARE             0x00020000L     // shared
#define SFGAO_READONLY          0x00040000L     // read-only
#define SFGAO_GHOSTED           0x00080000L     // ghosted icon
#define SFGAO_HIDDEN            0x00080000L     // hidden object
#define SFGAO_DISPLAYATTRMASK   0x000F0000L
#define SFGAO_FILESYSANCESTOR   0x10000000L     // It contains file system folder
#define SFGAO_FOLDER            0x20000000L     // It's a folder.
#define SFGAO_FILESYSTEM        0x40000000L     // is a file system thing (file/folder/root)
#define SFGAO_HASSUBFOLDER      0x80000000L     // Expandable in the map pane
#define SFGAO_CONTENTSMASK      0x80000000L
#define SFGAO_VALIDATE          0x01000000L     // invalidate cached information
#define SFGAO_REMOVABLE         0x02000000L     // is this removeable media?
#define SFGAO_COMPRESSED        0x04000000L     // Object is compressed (use alt color)
#define SFGAO_BROWSABLE         0x08000000L     // is in-place browsable
#define SFGAO_NONENUMERATED     0x00100000L     // is a non-enumerated object
#define SFGAO_NEWCONTENT        0x00200000L     // should show bold in explorer tree

DECLARE_INTERFACE_(IShellFolder, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellFolder methods ***
    STDMETHOD(ParseDisplayName) (THIS_ HWND hwndOwner,
        LPBC pbcReserved, LPOLESTR lpszDisplayName,
        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes) PURE;

    STDMETHOD(EnumObjects) ( THIS_ HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppenumIDList) PURE;

    STDMETHOD(BindToObject)     (THIS_ LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, LPVOID * ppvOut) PURE;
    STDMETHOD(BindToStorage)    (THIS_ LPCITEMIDLIST pidl, LPBC pbcReserved,
                                 REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD(CompareIDs)       (THIS_ LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) PURE;
    STDMETHOD(CreateViewObject) (THIS_ HWND hwndOwner, REFIID riid, LPVOID * ppvOut) PURE;
    STDMETHOD(GetAttributesOf)  (THIS_ UINT cidl, LPCITEMIDLIST * apidl,
                                    ULONG * rgfInOut) PURE;
    STDMETHOD(GetUIObjectOf)    (THIS_ HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                                 REFIID riid, UINT * prgfInOut, LPVOID * ppvOut) PURE;
    STDMETHOD(GetDisplayNameOf) (THIS_ LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName) PURE;
    STDMETHOD(SetNameOf)        (THIS_ HWND hwndOwner, LPCITEMIDLIST pidl,
                                 LPCOLESTR lpszName, DWORD uFlags,
                                 LPITEMIDLIST * ppidlOut) PURE;
};

typedef IShellFolder * LPSHELLFOLDER;

//
//  Helper function which returns a IShellFolder interface to the desktop
// folder. This is equivalent to call CoCreateInstance with CLSID_ShellDesktop.
//
//  CoCreateInstance(CLSID_Desktop, NULL,
//                   CLSCTX_INPROC, IID_IShellFolder, &pshf);
//
WINSHELLAPI HRESULT WINAPI SHGetDesktopFolder(LPSHELLFOLDER *ppshf);


//-------------------------------------------------------------------------
//
// DELEGATEITEMID structure
//
//
// A Delegate Item ID is a standard SHITEMID with some additional
// fields defined.
//
//-------------------------------------------------------------------------

typedef struct tagDELEGATEITEMID {
    WORD cbSize;                // Size of entire item ID
    WORD wOuter;                // Private data owned by the outer folder
    WORD cbInner;               // Size of delegate's data
    BYTE rgb[1];                // Inner folder's data,
                                //   followed by outer folder's data.
} DELEGATEITEMID, *PDELEGATEITEMID;

//-------------------------------------------------------------------------
//
// IDelegateFolder interface
//
//
// [Member functions]
//
// IDelegateFolder::SetItemAlloc(pm)
//   This function gives the object an IMalloc interface that it
//   should use to alloc and free item IDs.  These IDs are in the
//   form of DELEGATEITEMIDs.
//
//-------------------------------------------------------------------------

#undef INTERFACE
#define INTERFACE IDelegateFolder
DECLARE_INTERFACE_(IDelegateFolder, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IDelegateFolder methods ***
    STDMETHOD(SetItemAlloc)(THIS_ IMalloc *pm) PURE;
};



//==========================================================================
// IShellToolbarSite/IShellToolbar/IShellToolbarFrame interfaces
//
//  These interfaces allows us (or ISVs) to install/update external Internet
// Toolbar for IE and the shell. The frame will simply get the CLSID from
// registry (to be defined) and CoCreateInstance it.
//
//==========================================================================

#undef  INTERFACE
#define INTERFACE   IShellToolbarSite

DECLARE_INTERFACE_(IShellToolbarSite, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IShellToolbarSite methods ***
    STDMETHOD(GetBorderST) (THIS_ IUnknown* punkSrc, LPRECT prcBorder) PURE;
    STDMETHOD(RequestBorderSpaceST) (THIS_ IUnknown* punkSrc, LPCBORDERWIDTHS pbw) PURE;
    STDMETHOD(SetBorderSpaceST) (THIS_ IUnknown* punkSrc, LPCBORDERWIDTHS pbw) PURE;
    STDMETHOD(OnFocusChangeST) (THIS_ IUnknown* punkSrc, BOOL fSetFocus) PURE;
};

#undef  INTERFACE
#define INTERFACE   IShellToolbarFrame

#define STFRF_NORMAL            0x0000
#define STFRF_DELETECONFIGDATA  0x0001

DECLARE_INTERFACE_(IShellToolbarFrame, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IShellToolbarFrame methods ***
    STDMETHOD(AddToolbar) (THIS_ IUnknown* punkSrc, LPCWSTR pwszItem, DWORD dwReserved) PURE;
    STDMETHOD(RemoveToolbar) (THIS_ IUnknown* punkSrc, DWORD dwRemoveFlags) PURE;
    STDMETHOD(FindToolbar) (THIS_ LPCWSTR pwszItem, REFIID riid, LPVOID* ppvObj) PURE;
};

#undef  INTERFACE
#define INTERFACE   IShellToolbar

DECLARE_INTERFACE_(IShellToolbar, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IShellToolbar methods ***
    STDMETHOD(SetToolbarSite) (THIS_ IUnknown* punkSite) PURE;
    STDMETHOD(ShowST)         (THIS_ BOOL fShow) PURE;
    STDMETHOD(CloseST)        (THIS_ DWORD dwReserved) PURE;
    STDMETHOD(ResizeBorderST) (THIS_ LPCRECT   prcBorder,
                                     IUnknown* punkToolbarSite,
                                     BOOL      fReserved) PURE;
    STDMETHOD(TranslateAcceleratorST) (THIS_ LPMSG lpmsg) PURE;
    STDMETHOD(HasFocus)       (THIS) PURE;
};


#undef  INTERFACE
#define INTERFACE   IShellToolband

DECLARE_INTERFACE_(IShellToolband, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IShellToolbar methods ***
    STDMETHOD(SetToolbarSite) (THIS_ IUnknown* punkSite) PURE;
    STDMETHOD(ShowST)         (THIS_ BOOL fShow) PURE;
    STDMETHOD(CloseST)        (THIS_ DWORD dwReserved) PURE;
    STDMETHOD(ResizeBorderST) (THIS_ LPCRECT   prcBorder,
                                     IUnknown* punkToolbarSite,
                                     BOOL      fReserved) PURE;
    STDMETHOD(TranslateAcceleratorST) (THIS_ LPMSG lpmsg) PURE;
    STDMETHOD(HasFocus)       (THIS) PURE;

    // *** IShellToolband methos ***
    STDMETHOD(GetBandInfo)    (THIS_ DWORD fViewMode, LPPOINT pptMinSize, 
                              LPWSTR pszTitle, int cchTitle, LPDWORD dwSizeMode) PURE;
};

#define STBBIF_VIEWMODE_NORMAL   0
#define STBBIF_VIEWMODE_VERTICAL 1

#define STBBIF_SIZEMODE_NORMAL  0x0
#define STBBIF_SIZEMODE_FIXED   0x1
#define STBBIF_SIZEMODE_TITLE   0x2
#define STBBIF_SIZEMODE_FIXEDBMP 0x4   // a fixed background bitmap (if supported)




//==========================================================================
// Clipboard format which may be supported by IDataObject from system
// defined shell folders (such as directories, network, ...).
//==========================================================================

#define CFSTR_SHELLIDLIST       TEXT("Shell IDList Array")      // CF_IDLIST
#define CFSTR_SHELLIDLISTOFFSET TEXT("Shell Object Offsets")    // CF_OBJECTPOSITIONS
#define CFSTR_NETRESOURCES      TEXT("Net Resource")            // CF_NETRESOURCE
#define CFSTR_FILEDESCRIPTORA   TEXT("FileGroupDescriptor")     // CF_FILEGROUPDESCRIPTORA
#define CFSTR_FILEDESCRIPTORW   TEXT("FileGroupDescriptorW")    // CF_FILEGROUPDESCRIPTORW
#define CFSTR_FILECONTENTS      TEXT("FileContents")            // CF_FILECONTENTS
#define CFSTR_FILENAMEA         TEXT("FileName")                // CF_FILENAMEA
#define CFSTR_FILENAMEW         TEXT("FileNameW")               // CF_FILENAMEW
#define CFSTR_PRINTERGROUP      TEXT("PrinterFriendlyName")     // CF_PRINTERS
#define CFSTR_FILENAMEMAPA      TEXT("FileNameMap")             // CF_FILENAMEMAPA
#define CFSTR_FILENAMEMAPW      TEXT("FileNameMapW")            // CF_FILENAMEMAPW
#define CFSTR_SHELLURL          TEXT("UniformResourceLocator")
#define CFSTR_PREFERREDDROPEFFECT TEXT("Preferred DropEffect")

#ifdef UNICODE
#define CFSTR_FILEDESCRIPTOR    CFSTR_FILEDESCRIPTORW
#define CFSTR_FILENAME          CFSTR_FILENAMEW
#define CFSTR_FILENAMEMAP       CFSTR_FILENAMEMAPW
#else
#define CFSTR_FILEDESCRIPTOR    CFSTR_FILEDESCRIPTORA
#define CFSTR_FILENAME          CFSTR_FILENAMEA
#define CFSTR_FILENAMEMAP       CFSTR_FILENAMEMAPA
#endif

//
// CF_OBJECTPOSITIONS
//
//



#define DVASPECT_SHORTNAME      2 // use for CF_HDROP to get short name version
//
// format of CF_NETRESOURCE
//
typedef struct _NRESARRAY {     // anr
    UINT cItems;
    NETRESOURCE nr[1];
} NRESARRAY, * LPNRESARRAY;

//
// format of CF_IDLIST
//
typedef struct _IDA {
    UINT cidl;          // number of relative IDList
    UINT aoffset[1];    // [0]: folder IDList, [1]-[cidl]: item IDList
} CIDA, * LPIDA;

//
// FILEDESCRIPTOR.dwFlags field indicate which fields are to be used
//
typedef enum {
    FD_CLSID            = 0x0001,
    FD_SIZEPOINT        = 0x0002,
    FD_ATTRIBUTES       = 0x0004,
    FD_CREATETIME       = 0x0008,
    FD_ACCESSTIME       = 0x0010,
    FD_WRITESTIME       = 0x0020,
    FD_FILESIZE         = 0x0040,
    FD_LINKUI           = 0x8000,       // 'link' UI is prefered
} FD_FLAGS;

typedef struct _FILEDESCRIPTORA { // fod
    DWORD dwFlags;

    CLSID clsid;
    SIZEL sizel;
    POINTL pointl;

    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    CHAR   cFileName[ MAX_PATH ];
} FILEDESCRIPTORA, *LPFILEDESCRIPTORA;

typedef struct _FILEDESCRIPTORW { // fod
    DWORD dwFlags;

    CLSID clsid;
    SIZEL sizel;
    POINTL pointl;

    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    WCHAR  cFileName[ MAX_PATH ];
} FILEDESCRIPTORW, *LPFILEDESCRIPTORW;

#ifdef UNICODE
#define FILEDESCRIPTOR      FILEDESCRIPTORW
#define LPFILEDESCRIPTOR    LPFILEDESCRIPTORW
#else
#define FILEDESCRIPTOR      FILEDESCRIPTORA
#define LPFILEDESCRIPTOR    LPFILEDESCRIPTORA
#endif

//
// format of CF_FILEGROUPDESCRIPTOR
//
typedef struct _FILEGROUPDESCRIPTORA { // fgd
     UINT cItems;
     FILEDESCRIPTORA fgd[1];
} FILEGROUPDESCRIPTORA, * LPFILEGROUPDESCRIPTORA;

typedef struct _FILEGROUPDESCRIPTORW { // fgd
     UINT cItems;
     FILEDESCRIPTORW fgd[1];
} FILEGROUPDESCRIPTORW, * LPFILEGROUPDESCRIPTORW;

#ifdef UNICODE
#define FILEGROUPDESCRIPTOR     FILEGROUPDESCRIPTORW
#define LPFILEGROUPDESCRIPTOR   LPFILEGROUPDESCRIPTORW
#else
#define FILEGROUPDESCRIPTOR     FILEGROUPDESCRIPTORA
#define LPFILEGROUPDESCRIPTOR   LPFILEGROUPDESCRIPTORA
#endif

//
// format of CF_HDROP and CF_PRINTERS, in the HDROP case the data that follows
// is a double null terinated list of file names, for printers they are printer
// friendly names
//
typedef struct _DROPFILES {
   DWORD pFiles;                       // offset of file list
   POINT pt;                           // drop point (client coords)
   BOOL fNC;                           // is it on NonClient area
                                       // and pt is in screen coords
   BOOL fWide;                         // WIDE character switch
} DROPFILES, FAR * LPDROPFILES;


//====== File System Notification APIs ===============================
//



//
//  File System Notification flags
//



#define SHCNE_RENAMEITEM          0x00000001L
#define SHCNE_CREATE              0x00000002L
#define SHCNE_DELETE              0x00000004L
#define SHCNE_MKDIR               0x00000008L
#define SHCNE_RMDIR               0x00000010L
#define SHCNE_MEDIAINSERTED       0x00000020L
#define SHCNE_MEDIAREMOVED        0x00000040L
#define SHCNE_DRIVEREMOVED        0x00000080L
#define SHCNE_DRIVEADD            0x00000100L
#define SHCNE_NETSHARE            0x00000200L
#define SHCNE_NETUNSHARE          0x00000400L
#define SHCNE_ATTRIBUTES          0x00000800L
#define SHCNE_UPDATEDIR           0x00001000L
#define SHCNE_UPDATEITEM          0x00002000L
#define SHCNE_SERVERDISCONNECT    0x00004000L
#define SHCNE_UPDATEIMAGE         0x00008000L
#define SHCNE_DRIVEADDGUI         0x00010000L
#define SHCNE_RENAMEFOLDER        0x00020000L
#define SHCNE_FREESPACE           0x00040000L
#define SHCNE_EXTENDED_EVENT      0x00080000L // Extended Event.

#define SHCNE_ASSOCCHANGED        0x08000000L

#define SHCNE_DISKEVENTS          0x0002381FL
#define SHCNE_GLOBALEVENTS        0x0C0D81E0L // Events that dont match pidls first
#define SHCNE_ALLEVENTS           0x7FFFFFFFL
#define SHCNE_INTERRUPT           0x80000000L // The presence of this flag indicates
                                            // that the event was generated by an
                                            // interrupt.  It is stripped out before
                                            // the clients of SHCNNotify_ see it.

#define SHCNEE_THEMECHANGED       0x00000001L  // The theme changed


// Flags
// uFlags & SHCNF_TYPE is an ID which indicates what dwItem1 and dwItem2 mean
#define SHCNF_IDLIST      0x0000        // LPITEMIDLIST
#define SHCNF_PATHA       0x0001        // path name
#define SHCNF_PRINTERA    0x0002        // printer friendly name
#define SHCNF_DWORD       0x0003        // DWORD
#define SHCNF_PATHW       0x0005        // path name
#define SHCNF_PRINTERW    0x0006        // printer friendly name
#define SHCNF_TYPE        0x00FF
#define SHCNF_FLUSH       0x1000
#define SHCNF_FLUSHNOWAIT 0x2000

#ifdef UNICODE
#define SHCNF_PATH      SHCNF_PATHW
#define SHCNF_PRINTER   SHCNF_PRINTERW
#else
#define SHCNF_PATH      SHCNF_PATHA
#define SHCNF_PRINTER   SHCNF_PRINTERA
#endif



//
//  APIs
//
WINSHELLAPI void WINAPI SHChangeNotify(LONG wEventId, UINT uFlags,
                                LPCVOID dwItem1, LPCVOID dwItem2);

//
// SHAddToRecentDocs
//
#define SHARD_PIDL      0x00000001L
#define SHARD_PATHA     0x00000002L
#define SHARD_PATHW     0x00000003L

#ifdef UNICODE
#define SHARD_PATH  SHARD_PATHW
#else
#define SHARD_PATH  SHARD_PATHA
#endif

WINSHELLAPI void WINAPI SHAddToRecentDocs(UINT uFlags, LPCVOID pv);





WINSHELLAPI HRESULT WINAPI SHGetInstanceExplorer(IUnknown **ppunk);

//
// SHGetDataFromIDListA/W
//
#define SHGDFIL_FINDDATA        1
#define SHGDFIL_NETRESOURCE     2
#define SHGDFIL_DESCRIPTIONID   3

#define SHDID_ROOT_REGITEM          1
#define SHDID_FS_FILE               2
#define SHDID_FS_DIRECTORY          3
#define SHDID_FS_OTHER              4
#define SHDID_COMPUTER_DRIVE35      5
#define SHDID_COMPUTER_DRIVE525     6
#define SHDID_COMPUTER_REMOVABLE    7
#define SHDID_COMPUTER_FIXED        8
#define SHDID_COMPUTER_NETDRIVE     9
#define SHDID_COMPUTER_CDROM        10
#define SHDID_COMPUTER_RAMDISK      11
#define SHDID_COMPUTER_OTHER        12
#define SHDID_NET_DOMAIN            13
#define SHDID_NET_SERVER            14
#define SHDID_NET_SHARE             15
#define SHDID_NET_RESTOFNET         16
#define SHDID_NET_OTHER             17

typedef struct _SHDESCRIPTIONID {
    DWORD   dwDescriptionId;
    CLSID   clsid;
} SHDESCRIPTIONID, *LPSHDESCRIPTIONID;

WINSHELLAPI HRESULT WINAPI SHGetDataFromIDListA(LPSHELLFOLDER psf, LPCITEMIDLIST pidl,
        int nFormat, PVOID pv, int cb);
WINSHELLAPI HRESULT WINAPI SHGetDataFromIDListW(LPSHELLFOLDER psf, LPCITEMIDLIST pidl,
        int nFormat, PVOID pv, int cb);

#ifdef UNICODE
#define SHGetDataFromIDList SHGetDataFromIDListW
#else
#define SHGetDataFromIDList SHGetDataFromIDListA
#endif



//===========================================================================



#ifdef __cplusplus
}

#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#endif // _SHLOBJ_H_
