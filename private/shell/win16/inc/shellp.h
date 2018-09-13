#ifndef _SHELLP_H_
#define _SHELLP_H_

//
// Define API decoration for direct importing of DLL references.
//
#ifndef WINSHELLAPI
#if !defined(_SHELL32_)
#define WINSHELLAPI DECLSPEC_IMPORT
#else
#define WINSHELLAPI
#endif
#endif // WINSHELLAPI

//
// shell private header
//

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//===========================================================================
#ifndef _SHSEMIP_H_
// Handle to property sheet extension array
DECLARE_HANDLE( HPSXA );
#endif // _SHSEMIP_H_

//===========================================================================
// Shell restrictions. (Parameter for SHRestriction)
typedef enum
{
	REST_NONE			= 0x00000000,
	REST_NORUN 			= 0x00000001,
	REST_NOCLOSE			= 0x00000002,
	REST_NOSAVESET			= 0x00000004,
	REST_NOFILEMENU 		= 0x00000008,
	REST_NOSETFOLDERS		= 0x00000010,
	REST_NOSETTASKBAR		= 0x00000020,
	REST_NODESKTOP			= 0x00000040,
	REST_NOFIND			= 0x00000080,
	REST_NODRIVES			= 0x00000100,
	REST_NODRIVEAUTORUN		= 0x00000200,
	REST_NODRIVETYPEAUTORUN		= 0x00000400,
	REST_NONETHOOD			= 0x00000800,
	REST_STARTBANNER		= 0x00001000,
	REST_RESTRICTRUN		= 0x00002000,
	REST_NOPRINTERTABS		= 0x00004000,
	REST_NOPRINTERDELETE		= 0x00008000,
	REST_NOPRINTERADD		= 0x00010000,
	REST_NOSTARTMENUSUBFOLDERS	= 0x00020000,
        REST_MYDOCSONNET                = 0x00040000,
        REST_NOEXITTODOS                = 0x00080000,
} RESTRICTIONS;

WINSHELLAPI HRESULT WINAPI CIDLData_CreateFromIDArray(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[], LPDATAOBJECT * ppdtobj);
WINSHELLAPI BOOL WINAPI SHIsBadInterfacePtr(LPCVOID pv, UINT cbVtbl);
//
// Stream API
//
WINSHELLAPI LPSTREAM WINAPI OpenRegStream(HKEY hkey, LPCSTR pszSubkey, LPCSTR pszValue, DWORD grfMode);
WINSHELLAPI LPSTREAM WINAPI OpenFileStream(LPCSTR szFile, DWORD grfMode);
//
// OLE ripoffs of Drag and Drop related API
//
WINSHELLAPI HRESULT WINAPI SHRegisterDragDrop(HWND hwnd, LPDROPTARGET pdtgt);
WINSHELLAPI HRESULT WINAPI SHRevokeDragDrop(HWND hwnd);
WINSHELLAPI HRESULT WINAPI SHDoDragDrop(HWND hwndOwner, LPDATAOBJECT pdata, LPDROPSOURCE pdsrc, DWORD dwEffect, LPDWORD pdwEffect);
//
// Special folder
//
WINSHELLAPI LPITEMIDLIST WINAPI SHCloneSpecialIDList(HWND hwndOwner, int nFolder, BOOL fCreate);
WINSHELLAPI BOOL WINAPI SHGetSpecialFolderPath(HWND hwndOwner, LPSTR lpszPath, int nFolder, BOOL fCreate);
// DiskFull
WINSHELLAPI void WINAPI SHHandleDiskFull(HWND hwnd, int idDrive);

//
// File Search APIS
//
WINSHELLAPI BOOL WINAPI SHFindFiles(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlSaveFile);
WINSHELLAPI BOOL WINAPI SHFindComputer(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlSaveFile);
//
//
WINSHELLAPI void WINAPI PathGetShortPath(LPSTR pszLongPath);
WINSHELLAPI BOOL WINAPI PathFindOnPath(LPSTR szFile, LPCSTR FAR * ppszOtherDirs);
WINSHELLAPI BOOL WINAPI PathYetAnotherMakeUniqueName(LPSTR  pszUniqueName, LPCSTR pszPath, LPCSTR pszShort, LPCSTR pszFileSpec);
//
WINSHELLAPI BOOL WINAPI Win32CreateDirectory(LPCSTR lpszPath, LPSECURITY_ATTRIBUTES lpsa);
WINSHELLAPI BOOL WINAPI Win32RemoveDirectory(LPCSTR lpszPath);
WINSHELLAPI BOOL WINAPI Win32DeleteFile(LPCSTR lpszPath);

// Convert an IDList into a logical IDList so that desktop folders	
// appear at the right spot in the tree					
WINSHELLAPI LPITEMIDLIST WINAPI SHLogILFromFSIL(LPCITEMIDLIST pidlFS);		

// Convert an ole string.
WINSHELLAPI BOOL WINAPI StrRetToStrN(LPSTR szOut, UINT uszOut, LPSTRRET pStrRet, LPCITEMIDLIST pidl);

WINSHELLAPI DWORD WINAPI SHWaitForFileToOpen(LPCITEMIDLIST pidl,
                               UINT uOptions, DWORD dwtimeout);
WINSHELLAPI HRESULT WINAPI SHGetRealIDL(LPSHELLFOLDER psf, LPCITEMIDLIST pidlSimple, LPITEMIDLIST FAR* ppidlReal);

WINSHELLAPI void WINAPI SetAppStartingCursor(HWND hwnd, BOOL fSet);

#define DECLAREWAITCURSOR  HCURSOR hcursor_wait_cursor_save
#define SetWaitCursor()   hcursor_wait_cursor_save = SetCursor(LoadCursor(NULL, IDC_WAIT))
#define ResetWaitCursor() SetCursor(hcursor_wait_cursor_save)

WINSHELLAPI DWORD WINAPI SHRestricted(RESTRICTIONS rest);
WINSHELLAPI LPVOID WINAPI SHGetHandlerEntry(LPCSTR szHandler, LPCSTR szProcName, HINSTANCE *lpModule);

WINSHELLAPI STDAPI SHCoCreateInstance(LPCSTR pszCLSID, const CLSID FAR * lpclsid,
	LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppv);
WINSHELLAPI BOOL  WINAPI SignalFileOpen(LPCITEMIDLIST pidl);
WINSHELLAPI LPITEMIDLIST WINAPI SHSimpleIDListFromPath(LPCSTR pszPath);
WINSHELLAPI int WINAPI SHCreateDirectory(HWND hwnd, LPCSTR pszPath);

WINSHELLAPI HPSXA SHCreatePropSheetExtArray( HKEY hKey, PCSTR pszSubKey, UINT max_iface );
WINSHELLAPI void SHDestroyPropSheetExtArray( HPSXA hpsxa );
WINSHELLAPI UINT SHAddFromPropSheetExtArray( HPSXA hpsxa, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam );
WINSHELLAPI UINT SHReplaceFromPropSheetExtArray( HPSXA hpsxa, UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam );
WINSHELLAPI DWORD SHNetConnectionDialog(HWND hwnd, LPSTR pszRemoteName, DWORD dwType) ;
WINSHELLAPI STDAPI SHLoadOLE(LPARAM lParam);
WINSHELLAPI void WINAPI Desktop_UpdateBriefcaseOnEvent(HWND hwnd, UINT uEvent);

WINSHELLAPI HRESULT WINAPI SHCreateStdEnumFmtEtc(UINT cfmt, const FORMATETC afmt[], LPENUMFORMATETC * ppenumFormatEtc);

// Shell create link API
#define SHCL_USETEMPLATE	0x0001
#define SHCL_USEDESKTOP		0x0002
#define SHCL_CONFIRM		0x0004

WINSHELLAPI HRESULT WINAPI SHCreateLinks(HWND hwnd, LPCSTR pszDir, IDataObject *pDataObj, UINT fFlags, LPITEMIDLIST* ppidl);

//
// Interface pointer validation
//
#define IsBadInterfacePtr(pitf, ITF)  SHIsBadInterfacePtr(pitf, sizeof(ITF##Vtbl))

//===========================================================================
// Image dragging API (definitely private)
//===========================================================================

// stuff for doing auto scrolling
#define NUM_POINTS	3
typedef struct {	// asd
    int iNextSample;
    DWORD dwLastScroll;
    BOOL bFull;
    POINT pts[NUM_POINTS];
    DWORD dwTimes[NUM_POINTS];
} AUTO_SCROLL_DATA;

#define DAD_InitScrollData(pad) (pad)->bFull = FALSE, (pad)->iNextSample = 0, (pad)->dwLastScroll = 0

WINSHELLAPI BOOL WINAPI DAD_SetDragImage(HIMAGELIST him, POINT FAR* pptOffset);
WINSHELLAPI BOOL WINAPI DAD_DragEnter(HWND hwndTarget);
WINSHELLAPI BOOL WINAPI DAD_DragEnterEx(HWND hwndTarget, const POINT ptStart);
WINSHELLAPI BOOL WINAPI DAD_ShowDragImage(BOOL fShow);
WINSHELLAPI BOOL WINAPI DAD_DragMove(POINT pt);
WINSHELLAPI BOOL WINAPI DAD_DragLeave(void);
WINSHELLAPI BOOL WINAPI DAD_AutoScroll(HWND hwnd, AUTO_SCROLL_DATA *pad, const POINT *pptNow);
WINSHELLAPI BOOL WINAPI DAD_SetDragImageFromListView(HWND hwndLV, POINT ptOffset);

//===========================================================================
// Another block of private API
//===========================================================================

// indexes into the shell image lists (Shell_GetImageList) for default images
#define II_DOCNOASSOC   0         // document (blank page) (not associated)
#define II_DOCUMENT     1         // document (with stuff on the page)
#define II_APPLICATION  2         // application (exe, com, bat)
#define II_FOLDER       3         // folder (plain)
#define II_FOLDEROPEN   4         // folder (open)
// Startmenu images.							
#define II_STPROGS    	19						
#define II_STDOCS	20						
#define II_STSETNGS    	21						
#define II_STFIND    	22						
#define II_STHELP    	23						
#define II_STRUN  	24						
#define II_STSUSPD  	25						
#define II_STEJECT  	26						
#define II_STSHUTD    	27						
// More startmenu image.
#define II_STCPANEL	35
#define II_STSPROGS	36
#define II_STPRNTRS	37
#define II_STFONTS	38
#define II_STTASKBR	39
									
WINSHELLAPI BOOL  WINAPI Shell_GetImageLists(HIMAGELIST FAR *phiml, HIMAGELIST FAR *phimlSmall);
WINSHELLAPI void  WINAPI Shell_SysColorChange(void);
WINSHELLAPI int   WINAPI Shell_GetCachedImageIndex(LPCSTR pszIconPath, int iIconIndex, UINT uIconFlags);

WINSHELLAPI LRESULT WINAPI SHShellFolderView_Message(HWND hwndMain, UINT uMsg, LPARAM lParam);

// A usefull function in Defview for mapping idlist into index into system
// image list.  Optionally it can also look up the index of the selected
// icon.
WINSHELLAPI int WINAPI SHMapPIDLToSystemImageListIndex(LPSHELLFOLDER pshf, LPCITEMIDLIST pidl, int *piIndexSel);
//
// OLE string
//
WINSHELLAPI int WINAPI OleStrToStrN(LPSTR, int, LPCOLESTR, int);
WINSHELLAPI int WINAPI StrToOleStrN(LPOLESTR, int, LPCSTR, int);
WINSHELLAPI int WINAPI OleStrToStr(LPSTR, LPCOLESTR);
WINSHELLAPI int WINAPI StrToOleStr(LPOLESTR, LPCSTR);

//===========================================================================
// Useful macros
//===========================================================================
#define ResultFromShort(i)  ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(i)))
#define ShortFromResult(r)  (short)SCODE_CODE(GetScode(r))


// Tray CopyData Messages
#define TCDM_APPBAR     0x00000000
#define TCDM_NOTIFY     0x00000001
#define TCDM_LOADINPROC 0x00000002


//===========================================================================
// IShellFolder::UIObject helper
//===========================================================================

STDAPI SHCreateDefExtIconKey(HKEY hkey, LPCSTR pszModule, int iIcon, int iIconOpen, UINT uFlags, LPEXTRACTICON FAR* pxiconOut);
STDAPI SHCreateDefExtIcon(LPCSTR pszModule, int iIcon, int iIconOpen, UINT uFlags, LPEXTRACTICON FAR* pxiconOut);

//
// BUGBUG: lParam for DFM_INVOKECOMMAND needs to be changed
// now it contains "args" but it should just contain the LPCMINVOKECOMMANDINFO
//                                  uMsg       wParam       lParam
#define DFM_MERGECONTEXTMENU         1      // uFlags       LPQCMINFO
#define DFM_INVOKECOMMAND            2      // idCmd        pszArgs
#define DFM_ADDREF                   3      // 0            0
#define DFM_RELEASE                  4      // 0            0
#define DFM_GETHELPTEXT              5      // idCmd,cchMax pszText
#define DFM_WM_MEASUREITEM           6      // ---from the message---
#define DFM_WM_DRAWITEM              7      // ---from the message---
#define DFM_WM_INITMENUPOPUP         8      // ---from the message---
#define DFM_VALIDATECMD              9      // idCmd        0
#define DFM_MERGECONTEXTMENU_TOP     10     // uFlags       LPQCMINFO

// Commands from DFM_INVOKECOMMAND when strings are passed in
#define DFM_CMD_DELETE		((WPARAM)-1)
#define DFM_CMD_MOVE		((WPARAM)-2)
#define DFM_CMD_COPY		((WPARAM)-3)
#define DFM_CMD_LINK		((WPARAM)-4)
#define DFM_CMD_PROPERTIES	((WPARAM)-5)
#define DFM_CMD_NEWFOLDER	((WPARAM)-6)
#define DFM_CMD_PASTE		((WPARAM)-7)
#define DFM_CMD_VIEWLIST	((WPARAM)-8)
#define DFM_CMD_VIEWDETAILS	((WPARAM)-9)
#define DFM_CMD_PASTELINK       ((WPARAM)-10)
#define DFM_CMD_PASTESPECIAL	((WPARAM)-11)
#define DFM_CMD_MODALPROP       ((WPARAM)-12)

typedef struct _QCMINFO	// qcm
{
    HMENU	hmenu;		// in
    UINT	indexMenu;	// in
    UINT	idCmdFirst;	// in/out
    UINT	idCmdLast;	// in
} QCMINFO, FAR* LPQCMINFO;

typedef HRESULT (CALLBACK FAR* LPFNDFMCALLBACK)(LPSHELLFOLDER psf,
						HWND hwndOwner,
						LPDATAOBJECT pdtobj,
						UINT uMsg,
						WPARAM wParam,
						LPARAM lParam);

STDAPI CDefFolderMenu_Create(LPCITEMIDLIST pidlFolder,
			     HWND hwndOwner,
			     UINT cidl, LPCITEMIDLIST FAR* apidl,
			     LPSHELLFOLDER psf,
			     LPFNDFMCALLBACK lpfn,
			     HKEY hkeyProgID, HKEY hkeyBaseProgID,
			     LPCONTEXTMENU FAR* ppcm);

void PASCAL CDefFolderMenu_MergeMenu(HINSTANCE hinst, UINT idMainMerge, UINT idPopupMerge,
	LPQCMINFO pqcm);
void PASCAL Def_InitFileCommands(ULONG dwAttr, HMENU hmInit, UINT idCmdFirst,
	BOOL bContext);
void PASCAL Def_InitEditCommands(ULONG dwAttr, HMENU hmInit, UINT idCmdFirst,
	LPDROPTARGET pdtgt, UINT fContext);
void NEAR PASCAL _SHPrettyMenu(HMENU hm);

//===========================================================================
// Default IShellView for IShellFolder
//===========================================================================

WINSHELLAPI HRESULT WINAPI SHCreateShellFolderView(LPSHELLFOLDER pshf, LPCITEMIDLIST pidl, LONG lEvent, LPSHELLVIEW FAR* ppsv);

// Menu ID's
#ifdef BUG_23171_FIXED
#define SFVIDM_FIRST			(FCIDM_SHVIEWLAST-0x0fff)
#else
// MENUEX currently cannot handle subtraction in the ID's, so we need
// to subtract for it.
#if (FCIDM_SHVIEWLAST != 0x7fff)
#error FCIDM_SHVIEWLAST has changed, so shellp.h needs to also
#endif
#define SFVIDM_FIRST			(0x7000)
#endif
#define SFVIDM_LAST			(FCIDM_SHVIEWLAST)

// Popup menu ID's used in merging menus
#define SFVIDM_MENU_ARRANGE	(SFVIDM_FIRST + 0x0001)
#define SFVIDM_MENU_VIEW	(SFVIDM_FIRST + 0x0002)
#define SFVIDM_MENU_SELECT	(SFVIDM_FIRST + 0x0003)

// TBINFO flags
#define TBIF_APPEND     0
#define TBIF_PREPEND    1
#define TBIF_REPLACE    2

typedef struct _TBINFO
{
    UINT        cbuttons;       // out
    UINT        uFlags;         // out (one of TBIF_ flags)
} TBINFO, FAR * LPTBINFO;

typedef struct _COPYHOOKINFO
{
    HWND hwnd;
    DWORD wFunc;
    DWORD wFlags;
    LPCSTR pszSrcFile;
    DWORD dwSrcAttribs;
    LPCSTR pszDestFile;
    DWORD dwDestAttribs;
} COPYHOOKINFO, *LPCOPYHOOKINFO;

typedef struct _DETAILSINFO
{
    LPCITEMIDLIST pidl;     // pidl to get details of
    // Note: do not change the order of these fields until IShellDetails
    //       has gone away!
    int fmt;                // LVCFMT_* value (header only)
    int cxChar;             // Number of "average" characters (header only)
    STRRET str;             // String information
} DETAILSINFO, *PDETAILSINFO;

//                               uMsg    wParam         lParam
#define DVM_MERGEMENU            1    // uFlags             LPQCMINFO
#define DVM_INVOKECOMMAND        2    // idCmd              0
#define DVM_GETHELPTEXT          3    // idCmd,cchMax       pszText
#define DVM_GETTOOLTIPTEXT       4    // idCmd,cchMax       pszText
#define DVM_GETBUTTONINFO        5    // 0                  LPTBINFO
#define DVM_GETBUTTONS           6    // idCmdFirst,cbtnMax LPTBBUTTON
#define DVM_INITMENUPOPUP        7    // idCmdFirst,nIndex  hmenu
#define DVM_SELCHANGE            8    // idCmdFirst,nItem   PDVSELCHANGEINFO
#define DVM_DRAWITEM             9    // idCmdFirst         pdis
#define DVM_MEASUREITEM         10    // idCmdFirst         pmis
#define DVM_EXITMENULOOP        11    // -                  -
#define DVM_RELEASE             12    // -                  lSelChangeInfo (ShellFolder private)
#define DVM_GETCCHMAX           13    // pidlItem           pcchMax
#define DVM_FSNOTIFY            14    // LPITEMIDLIST*      lEvent
#define DVM_WINDOWCREATED       15    // hwnd               PDVSELCHANGEINFO
#define DVM_WINDOWDESTROY       16    // hwnd               PDVSELCHANGEINFO
#define DVM_REFRESH             17    // -                  lSelChangeInfo
#define DVM_SETFOCUS            18    // -                  lSelChangeInfo
#define DVM_KILLFOCUS           19    // -                  -
#define DVM_QUERYCOPYHOOK	20    // -                  -
#define DVM_NOTIFYCOPYHOOK      21    // -                  LPCOPYHOOKINFO
#define DVM_NOTIFY		22    // idFrom		    LPNOTIFY
#define DVM_GETDETAILSOF        23    // iColumn            PDETAILSINFO
#define DVM_COLUMNCLICK         24    // iColumn            -
#define DVM_QUERYFSNOTIFY       25    // -                  FSNotifyEntry *
#define DVM_DEFITEMCOUNT        26    // -                  PINT
#define DVM_DEFVIEWMODE         27    // -                  PFOLDERVIEWMODE
#define DVM_UNMERGEMENU         28    // uFlags
#define DVM_INSERTITEM          29    // pidl               PDVSELCHANGEINFO
#define DVM_DELETEITEM          30    // pidl               PDVSELCHANGEINFO
#define DVM_UPDATESTATUSBAR     31    // -                  lSelChangeInfo
#define DVM_BACKGROUNDENUM      32
#define DVM_GETWORKINGDIR       33
#define DVM_GETCOLSAVESTREAM    34    // flags              IStream **
#define DVM_SELECTALL           35    //                    lSelChangeInfo
#define DVM_DIDDRAGDROP         36    // dwEffect           IDataObject *

typedef struct _DVSELCHANGEINFO {
    UINT uOldState;
    UINT uNewState;
    LPARAM lParamItem;
    LPARAM* plParam;
} DVSELCHANGEINFO, *PDVSELCHANGEINFO;

typedef HRESULT (CALLBACK FAR* LPFNVIEWCALLBACK)(LPSHELLVIEW psvOuter,
						LPSHELLFOLDER psf,
                                                HWND hwndMain,
                                                UINT uMsg,
                                                WPARAM wParam,
                                                LPARAM lParam);

// SHCreateShellFolderViewEx struct
typedef struct _CSFV
{
    UINT            cbSize;
    LPSHELLFOLDER   pshf;
    LPSHELLVIEW     psvOuter;
    LPCITEMIDLIST   pidl;
    LONG            lEvents;
    LPFNVIEWCALLBACK pfnCallback;       // No callback if NULL
    FOLDERVIEWMODE  fvm;
} CSFV, FAR * LPCSFV;

// Tell the FolderView to rearrange.  The lParam will be passed to
// IShellFolder::CompareIDs
#define SFVM_REARRANGE		0x00000001
#define ShellFolderView_ReArrange(_hwnd, _lparam) \
	(BOOL)SHShellFolderView_Message(_hwnd, SFVM_REARRANGE, _lparam)

// Get the last sorting parameter given to FolderView
#define SFVM_GETARRANGEPARAM	0x00000002
#define ShellFolderView_GetArrangeParam(_hwnd) \
	(LPARAM)SHShellFolderView_Message(_hwnd, SFVM_GETARRANGEPARAM, 0L)

// Add an OBJECT into the view (May need to add insert also)
#define SFVM_ADDOBJECT         0x00000003
#define ShellFolderView_AddObject(_hwnd, _pidl) \
	(LPARAM)SHShellFolderView_Message(_hwnd, SFVM_ADDOBJECT, (LPARAM)_pidl)

// Gets the count of objects in the view
#define SFVM_GETOBJECTCOUNT         0x00000004
#define ShellFolderView_GetObjectCount(_hwnd) \
	(LPARAM)SHShellFolderView_Message(_hwnd, SFVM_GETOBJECTCOUNT, (LPARAM)0)

// Returns a pointer to the Idlist associated with the specified index
// Returns NULL if at end of list.
#define SFVM_GETOBJECT         0x00000005
#define ShellFolderView_GetObject(_hwnd, _iObject) \
	(LPARAM)SHShellFolderView_Message(_hwnd, SFVM_GETOBJECT, _iObject)

// Remove an OBJECT into the view (This works by pidl, may need index also);
#define SFVM_REMOVEOBJECT         0x00000006
#define ShellFolderView_RemoveObject(_hwnd, _pidl) \
	(LPARAM)SHShellFolderView_Message(_hwnd, SFVM_REMOVEOBJECT, (LPARAM)_pidl)

// updates an object by passing in pointer to two PIDLS, the first
// is the old pidl, the second one is the one with update information.
#define SFVM_UPDATEOBJECT         0x00000007
#define ShellFolderView_UpdateObject(_hwnd, _ppidl) \
	(LPARAM)SHShellFolderView_Message(_hwnd, SFVM_UPDATEOBJECT, (LPARAM)_ppidl)

// Sets the redraw mode for the window that is displaying the information
#define SFVM_SETREDRAW           0x00000008
#define ShellFolderView_SetRedraw(_hwnd, fRedraw) \
	(LPARAM)SHShellFolderView_Message(_hwnd, SFVM_SETREDRAW, (LPARAM)fRedraw)

// Returns an array of the selected IDS to the caller.
//     lparam is a pointer to receive the idlists into
//     return value is the count of items in the array.
#define SFVM_GETSELECTEDOBJECTS 0x00000009
#define ShellFolderView_GetSelectedObjects(_hwnd, ppidl) \
	(LPARAM)SHShellFolderView_Message(_hwnd, SFVM_GETSELECTEDOBJECTS, (LPARAM)ppidl)

// Checks if the current drop is on the view window
//     lparam is unused
//     return value is TRUE if the current drop is upon the background of the
//         view window, FALSE otherwise
#define SFVM_ISDROPONSOURCE	0x0000000a
#define ShellFolderView_IsDropOnSource(_hwnd, _pdtgt) \
	(BOOL)SHShellFolderView_Message(_hwnd, SFVM_ISDROPONSOURCE, (LPARAM)_pdtgt)

// Moves the selected icons in the listview
//     lparam is a pointer to a drop target
//     return value is unused
#define SFVM_MOVEICONS		0x0000000b
#define ShellFolderView_MoveIcons(_hwnd, _pdt) \
	(void)SHShellFolderView_Message(_hwnd, SFVM_MOVEICONS, (LPARAM)(LPDROPTARGET)_pdt)

// Gets the start point of a drag-drop
//     lparam is a pointer to a point
//     return value is unused
#define SFVM_GETDRAGPOINT	0x0000000c
#define ShellFolderView_GetDragPoint(_hwnd, _ppt) \
	(BOOL)SHShellFolderView_Message(_hwnd, SFVM_GETDRAGPOINT, (LPARAM)(LPPOINT)_ppt)

// Gets the end point of a drag-drop
//     lparam is a pointer to a point
//     return value is unused
#define SFVM_GETDROPPOINT	0x0000000d
#define ShellFolderView_GetDropPoint(_hwnd, _ppt) \
	SHShellFolderView_Message(_hwnd, SFVM_GETDROPPOINT, (LPARAM)(LPPOINT)_ppt)

#define ShellFolderView_GetAnchorPoint(_hwnd, _fStart, _ppt) \
	(BOOL)((_fStart) ? ShellFolderView_GetDragPoint(_hwnd, _ppt) : ShellFolderView_GetDropPoint(_hwnd, _ppt))

typedef struct _SFV_SETITEMPOS
{
	LPCITEMIDLIST pidl;
	POINT pt;
} SFV_SETITEMPOS, FAR *LPSFV_SETITEMPOS;

// Sets the position of an item in the viewer
//     lparam is a pointer to a SVF_SETITEMPOS
//     return value is unused
#define SFVM_SETITEMPOS		0x0000000e
#define ShellFolderView_SetItemPos(_hwnd, _pidl, _x, _y) \
{	SFV_SETITEMPOS _sip = {_pidl, {_x, _y}}; \
	SHShellFolderView_Message(_hwnd, SFVM_SETITEMPOS, (LPARAM)(LPSFV_SETITEMPOS)&_sip);}

// Determines if a given drop target interface is the one being used for
// the background of the ShellFolderView (as opposed to an object in the
// view)
//     lparam is a pointer to a drop target interface
//     return value is TRUE if it is the background drop target, FALSE otherwise
#define SFVM_ISBKDROPTARGET	0x0000000f
#define ShellFolderView_IsBkDropTarget(_hwnd, _pdptgt) \
        (BOOL)SHShellFolderView_Message(_hwnd, SFVM_ISBKDROPTARGET, (LPARAM)(LPDROPTARGET)_pdptgt)

//  Notifies a ShellView when one of its objects get put on the clipboard
//  as a result of a menu command.
//
//  called by defcm.c when it does a copy/cut
//
//     lparam is the dwEffect (DROPEFFECT_MOVE, DROPEFFECT_COPY)
//     return value is void.
#define SFVM_SETCLIPBOARD       0x00000010
#define ShellFolderView_SetClipboard(_hwnd, _dwEffect) \
        (void)SHShellFolderView_Message(_hwnd, SFVM_SETCLIPBOARD, (LPARAM)(DWORD)(_dwEffect))


// sets auto arrange
#define SFVM_AUTOARRANGE        0x00000011
#define ShellFolderView_AutoArrange(_hwnd) \
        (void)SHShellFolderView_Message(_hwnd, SFVM_AUTOARRANGE, 0)

// sets snap to grid
#define SFVM_ARRANGEGRID        0x00000012
#define ShellFolderView_ArrangeGrid(_hwnd) \
        (void)SHShellFolderView_Message(_hwnd, SFVM_ARRANGEGRID, 0)

#define SFVM_GETAUTOARRANGE     0x00000013
#define ShellFolderView_GetAutoArrange(_hwnd) \
        (BOOL)SHShellFolderView_Message(_hwnd, SFVM_GETAUTOARRANGE, 0)

#define SFVM_GETSELECTEDCOUNT     0x00000014
#define ShellFolderView_GetSelectedCount(_hwnd) \
        (BOOL)SHShellFolderView_Message(_hwnd, SFVM_GETSELECTEDCOUNT, 0)

typedef struct {
    int cxSmall;
    int cySmall;
    int cxLarge;
    int cyLarge;
} ITEMSPACING, *LPITEMSPACING;

#define SFVM_GETITEMSPACING     0x00000015
#define ShellFolderView_GetItemSpacing(_hwnd, lpis) \
        (BOOL)SHShellFolderView_Message(_hwnd, SFVM_GETITEMSPACING, (LPARAM)lpis)

// Causes an object to be repainted
#define SFVM_REFRESHOBJECT      0x00000016
#define ShellFolderView_RefreshObject(_hwnd, _ppidl) \
        (LPARAM)SHShellFolderView_Message(_hwnd, SFVM_REFRESHOBJECT, (LPARAM)_ppidl)


#define SFVM_SETPOINTS           0x00000017
#define ShellFolderView_SetPoints(_hwnd, _pdtobj) \
        (void)SHShellFolderView_Message(_hwnd, SFVM_SETPOINTS, (LPARAM)_pdtobj)

// SVM_SELECTANDPOSITIONITEM lParam
typedef struct
{
	LPCITEMIDLIST pidl;	// relative pidl to the view
	UINT  uSelectFlags;	// select flags
        BOOL fMove; // if true, we should also move it to point pt
        POINT pt;
} SFM_SAP;

// shell view messages
#define SVM_SELECTITEM       		(WM_USER + 1)
#define SVM_MOVESELECTEDITEMS           (WM_USER + 2)
#define SVM_GETANCHORPOINT              (WM_USER + 3)
#define SVM_GETITEMPOSITION             (WM_USER + 4)
#define SVM_SELECTANDPOSITIONITEM       (WM_USER + 5)

// Heap tracking stuff.
#ifdef MEMMON
#ifndef INC_MEMMON
#define INC_MEMMON
#define LocalAlloc	SHLocalAlloc
#define LocalFree	SHLocalFree
#define LocalReAlloc	SHLocalReAlloc

WINSHELLAPI HLOCAL WINAPI SHLocalAlloc(UINT uFlags, UINT cb);
WINSHELLAPI HLOCAL WINAPI SHLocalReAlloc(HLOCAL hOld, UINT cbNew, UINT uFlags);
WINSHELLAPI HLOCAL WINAPI SHLocalFree(HLOCAL h);
#endif
#endif

//===========================================================================
// CDefShellFolder members (for easy subclassing)
//===========================================================================

// Single instance members
STDMETHODIMP_(ULONG) CSIShellFolder_AddRef(LPSHELLFOLDER psf) ;
STDMETHODIMP_(ULONG) CSIShellFolder_Release(LPSHELLFOLDER psf);

// Default implementation (no dependencies to the instance data)
STDMETHODIMP CDefShellFolder_QueryInterface(LPSHELLFOLDER psf, REFIID riid, LPVOID FAR* ppvObj);
STDMETHODIMP CDefShellFolder_BindToStorage(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, LPBC pbc,
    			 REFIID riid, LPVOID FAR* ppvOut);
STDMETHODIMP CDefShellFolder_BindToObject(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, LPVOID FAR* ppvOut);
STDMETHODIMP CDefShellFolder_GetAttributesOf(LPSHELLFOLDER psf, UINT cidl, LPCITEMIDLIST FAR* apidl, ULONG FAR* rgfOut);
STDMETHODIMP CDefShellFolder_SetNameOf(LPSHELLFOLDER psf, HWND hwndOwner,
	LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD dwReserved, LPITEMIDLIST FAR* ppidlOut);

// File Search APIS
WINSHELLAPI LPCONTEXTMENU WINAPI SHFind_InitMenuPopup(HMENU hmenu, HWND hwndOwner, UINT idCmdFirst, UINT idCmdLast);

WINSHELLAPI void WINAPI Control_RunDLL(HWND hwndStub, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow);
// to add 16 bit pages to 32bit things.  hGlobal can be NULL
WINSHELLAPI UINT WINAPI SHAddPages16(HGLOBAL hGlobal, LPCSTR pszDllEntry, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);

WINSHELLAPI HRESULT WINAPI SHCreateShellFolderViewEx(LPCSFV pcsfv, LPSHELLVIEW FAR* ppsv);

//===========================================================================
// Defview related API and interface
//
//  Notes: At this point, we have no plan to publish this mechanism.
//===========================================================================

typedef struct _SHELLDETAILS
{
	int	fmt;		// LVCFMT_* value (header only)
	int	cxChar;		// Number of "average" characters (header only)
	STRRET	str;		// String information
} SHELLDETAILS, FAR *LPSHELLDETAILS;

#undef  INTERFACE
#define INTERFACE   IShellDetails

DECLARE_INTERFACE_(IShellDetails, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellDetails methods ***
    STDMETHOD(GetDetailsOf)(THIS_ LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetails) PURE;
    STDMETHOD(ColumnClick)(THIS_ UINT iColumn) PURE;
};

//
// Private QueryContextMenuFlag passed from DefView
//
#define CMF_DVFILE	 0x00010000	// "File" pulldown

//
// Functions to help the cabinets sync to each other
//  uOptions parameter to SHWaitForFileOpen
//
#define WFFO_WAITTIME 10000L

#define WFFO_ADD        0x0001
#define WFFO_REMOVE     0x0002
#define WFFO_WAIT       0x0004


// Common strings
#define STR_DESKTOPCLASS	"Progman"

//===========================================================================
// Helper functions for pidl allocation using the task allocator.
//
WINSHELLAPI HRESULT WINAPI SHILClone(LPCITEMIDLIST pidl, LPITEMIDLIST * ppidlOut);
WINSHELLAPI HRESULT WINAPI SHILCombine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, LPITEMIDLIST * ppidlOut);
#define SHILFree(pidl)	SHFree(pidl)

WINSHELLAPI HRESULT WINAPI SHDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv);


#include <fsmenu.h>

//===========================================================================

//----------------------------------------------------------------------------
#define IsLFNDriveORD		119
WINSHELLAPI BOOL WINAPI IsLFNDrive(LPCSTR pszPath);
WINSHELLAPI int WINAPI SHOutOfMemoryMessageBox(HWND hwndOwner, LPSTR pszTitle, UINT fuStyle);
WINSHELLAPI BOOL WINAPI SHWinHelp(HWND hwndMain, LPCSTR lpszHelp, UINT usCommand, DWORD ulData);

WINSHELLAPI BOOL WINAPI RLBuildListOfPaths(void);

#define SHValidateUNCORD        173

#define VALIDATEUNC_NOUI        0x0002      // dont bring up stinking UI!
#define VALIDATEUNC_CONNECT     0x0001      // connect a drive letter
#define VALIDATEUNC_PRINT       0x0004      // validate as print share instead of disk share
#define VALIDATEUNC_VALID       0x0007      // valid flags


WINSHELLAPI BOOL WINAPI SHValidateUNC(HWND hwndOwner, LPSTR pszFile, UINT fConnect);

//----------------------------------------------------------------------------
#define OleStrToStrNORD			        78
#define SHCloneSpecialIDListORD		      	89
#define SHDllGetClassObjectORD		       128
#define SHLogILFromFSILORD			95
#define SHMapPIDLToSystemImageListIndexORD	77
#define SHShellFolderView_MessageORD		73
#define Shell_GetImageListsORD			71
#define SHGetSpecialFolderPathORD	       175
#define StrToOleStrNORD				79

#define ILCloneORD				18
#define ILCloneFirstORD				19
#define ILCombineORD				25
#define ILCreateFromPathORD			157
#define ILFindChildORD				24
#define ILFreeORD				155
#define ILGetNextORD				153
#define ILGetSizeORD				152
#define ILIsEqualORD				21
#define ILRemoveLastIDORD			17
#define PathAddBackslashORD			32
#define PathCombineORD				37
#define PathIsExeORD				43
#define PathMatchSpecORD			46
#define SHGetSetSettingsORD			68
#define SHILCreateFromPathORD			28

#define SHFreeORD				195
#define MemMon_FreeORD				123

//
// Storage name of a scrap/bookmark item
//
#define WSTR_SCRAPITEM L"\003ITEM000"

//
//  PifMgr Thunked APIs (in SHELL.DLL)
//
extern int  WINAPI PifMgr_OpenProperties(LPCSTR lpszApp, LPCSTR lpszPIF, int hInf, int flOpt);
extern int  WINAPI PifMgr_GetProperties(int hProps, LPCSTR lpszGroup, LPVOID lpProps, int cbProps, int flOpt);
extern int  WINAPI PifMgr_SetProperties(int hProps, LPCSTR lpszGroup, const VOID FAR *lpProps, int cbProps, int flOpt);
extern int  WINAPI PifMgr_CloseProperties(int hProps, int flOpt);

//
// exported from SHSCRAP.DLL
//
#define SCRAP_CREATEFROMDATAOBJECT "Scrap_CreateFromDataObject"
typedef HRESULT (WINAPI FAR * LPFNSCRAPCREATEFROMDATAOBJECT)(LPCSTR pszPath, LPDATAOBJECT pDataObj, BOOL fLink, LPSTR pszNewFile);
extern HRESULT WINAPI Scrap_CreateFromDataObject(LPCSTR pszPath, LPDATAOBJECT pDataObj, BOOL fLink, LPSTR pszNewFile);

WINSHELLAPI void WINAPI SHSetInstanceExplorer(IUnknown *punk);

// Always use TerminateThreadEx.
BOOL APIENTRY TerminateThreadEx(HANDLE hThread, DWORD dwExitCode, BOOL bCleanupFlag);
#define TerminateThread(hThread, dwExitCode) TerminateThreadEx(hThread, dwExitCode, TRUE)

//===========================================================================
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#endif // _SHELLP_H_
