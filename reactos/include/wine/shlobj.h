/*
 * Copyright (C) the Wine project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_SHLOBJ_H
#define __WINE_SHLOBJ_H

#include <ole2.h>
#include <commctrl.h>
#include <prsht.h>
#include <shlguid.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include <pshpack1.h>

#include <shtypes.h>
#include <shobjidl.h>


LPVOID       WINAPI SHAlloc(ULONG);
HRESULT      WINAPI SHCoCreateInstance(LPCWSTR,const CLSID*,IUnknown*,REFIID,LPVOID*);
HRESULT      WINAPI SHCreateStdEnumFmtEtc(DWORD,const FORMATETC *,IEnumFORMATETC**);
BOOL         WINAPI SHFindFiles(LPCITEMIDLIST,LPCITEMIDLIST);
void         WINAPI SHFree(LPVOID);
BOOL         WINAPI SHGetPathFromIDListA(LPCITEMIDLIST pidl,LPSTR pszPath);
BOOL         WINAPI SHGetPathFromIDListW(LPCITEMIDLIST pidl,LPWSTR pszPath);
#define             SHGetPathFromIDList WINELIB_NAME_AW(SHGetPathFromIDList)
HRESULT      WINAPI SHILCreateFromPath(LPCWSTR,LPITEMIDLIST*,DWORD*);
LPITEMIDLIST WINAPI SHSimpleIDListFromPath(LPCWSTR);
int          WINAPI SHMapPIDLToSystemImageListIndex(IShellFolder*,LPCITEMIDLIST,int*);

LPITEMIDLIST WINAPI ILAppendID(LPITEMIDLIST,LPCSHITEMID,BOOL);
LPITEMIDLIST WINAPI ILClone(LPCITEMIDLIST);
LPITEMIDLIST WINAPI ILCloneFirst(LPCITEMIDLIST);
LPITEMIDLIST WINAPI ILCreateFromPathA(LPCSTR);
LPITEMIDLIST WINAPI ILCreateFromPathW(LPCWSTR);
#define             ILCreateFromPath WINELIB_NAME_AW(ILCreateFromPath)
LPITEMIDLIST WINAPI ILCombine(LPCITEMIDLIST,LPCITEMIDLIST);
LPITEMIDLIST WINAPI ILFindChild(LPCITEMIDLIST,LPCITEMIDLIST);
LPITEMIDLIST WINAPI ILFindLastID(LPCITEMIDLIST);
void         WINAPI ILFree(LPITEMIDLIST);
LPITEMIDLIST WINAPI ILGetNext(LPCITEMIDLIST);
UINT         WINAPI ILGetSize(LPCITEMIDLIST);
BOOL         WINAPI ILIsEqual(LPCITEMIDLIST,LPCITEMIDLIST);
BOOL         WINAPI ILIsParent(LPCITEMIDLIST,LPCITEMIDLIST,BOOL);
BOOL         WINAPI ILRemoveLastID(LPCITEMIDLIST);

HRESULT WINAPI SHCoCreateInstance(LPCWSTR, const CLSID*, LPUNKNOWN, REFIID, LPVOID*);

BOOL WINAPI SHGetPathFromIDListA (LPCITEMIDLIST pidl,LPSTR pszPath);
BOOL WINAPI SHGetPathFromIDListW (LPCITEMIDLIST pidl,LPWSTR pszPath);
#define     SHGetPathFromIDList WINELIB_NAME_AW(SHGetPathFromIDList)

/*****************************************************************************
 * Predeclare interfaces
 */
typedef struct IShellIcon IShellIcon, *LPSHELLICON;


/*****************************************************************************
 * IContextMenu interface
 */


/* DATAOBJECT_InitShellIDList*/
#define CFSTR_SHELLIDLIST       "Shell IDList Array"      /* CF_IDLIST */

extern UINT cfShellIDList;

typedef struct
{	UINT cidl;
	UINT aoffset[1];
} CIDA, *LPIDA;

#define CFSTR_SHELLIDLISTOFFSET "Shell Object Offsets"    /* CF_OBJECTPOSITIONS */
#define CFSTR_NETRESOURCES      "Net Resource"            /* CF_NETRESOURCE */

/* DATAOBJECT_InitFileGroupDesc */
#define CFSTR_FILEDESCRIPTORA   "FileGroupDescriptor"     /* CF_FILEGROUPDESCRIPTORA */
extern UINT cfFileGroupDesc;

#define CFSTR_FILEDESCRIPTORW   "FileGroupDescriptorW"    /* CF_FILEGROUPDESCRIPTORW */

/* DATAOBJECT_InitFileContents*/
#define CFSTR_FILECONTENTS      "FileContents"            /* CF_FILECONTENTS */
extern UINT cfFileContents;

#define CFSTR_FILENAMEA         "FileName"                /* CF_FILENAMEA */
#define CFSTR_FILENAMEW         "FileNameW"               /* CF_FILENAMEW */
#define CFSTR_PRINTERGROUP      "PrinterFriendlyName"     /* CF_PRINTERS */
#define CFSTR_FILENAMEMAPA      "FileNameMap"             /* CF_FILENAMEMAPA */
#define CFSTR_FILENAMEMAPW      "FileNameMapW"            /* CF_FILENAMEMAPW */
#define CFSTR_SHELLURL          "UniformResourceLocator"
#define CFSTR_PREFERREDDROPEFFECT "Preferred DropEffect"
#define CFSTR_PERFORMEDDROPEFFECT "Performed DropEffect"
#define CFSTR_PASTESUCCEEDED    "Paste Succeeded"
#define CFSTR_INDRAGLOOP        "InShellDragLoop"

#define CFSTR_FILENAME WINELIB_NAME_AW(CFSTR_FILENAME)


/************************************************************************
* IShellView interface
*/

typedef GUID SHELLVIEWID;
#define SV_CLASS_NAME   ("SHELLDLL_DefView")

#define FCIDM_SHVIEWFIRST       0x0000
/* undocumented */
#define FCIDM_SHVIEW_ARRANGE    0x7001
#define FCIDM_SHVIEW_DELETE     0x7011
#define FCIDM_SHVIEW_PROPERTIES 0x7013
#define FCIDM_SHVIEW_CUT        0x7018
#define FCIDM_SHVIEW_COPY       0x7019
#define FCIDM_SHVIEW_INSERT     0x701A
#define FCIDM_SHVIEW_UNDO       0x701B
#define FCIDM_SHVIEW_INSERTLINK 0x701C
#define FCIDM_SHVIEW_SELECTALL  0x7021
#define FCIDM_SHVIEW_INVERTSELECTION 0x7022

#define FCIDM_SHVIEW_BIGICON    0x7029
#define FCIDM_SHVIEW_SMALLICON  0x702A
#define FCIDM_SHVIEW_LISTVIEW   0x702B
#define FCIDM_SHVIEW_REPORTVIEW 0x702C
/* 0x7030-0x703f are used by the shellbrowser */
#define FCIDM_SHVIEW_AUTOARRANGE 0x7031
#define FCIDM_SHVIEW_SNAPTOGRID 0x7032

#define FCIDM_SHVIEW_HELP       0x7041
#define FCIDM_SHVIEW_RENAME     0x7050
#define FCIDM_SHVIEW_CREATELINK 0x7051
#define FCIDM_SHVIEW_NEWLINK    0x7052
#define FCIDM_SHVIEW_NEWFOLDER  0x7053

#define FCIDM_SHVIEW_REFRESH    0x7100 /* FIXME */
#define FCIDM_SHVIEW_EXPLORE    0x7101 /* FIXME */
#define FCIDM_SHVIEW_OPEN       0x7102 /* FIXME */

#define FCIDM_SHVIEWLAST        0x7fff
#define FCIDM_BROWSERFIRST      0xA000
/* undocumented toolbar items from stddlg's*/
#define FCIDM_TB_UPFOLDER       0xA001
#define FCIDM_TB_NEWFOLDER      0xA002
#define FCIDM_TB_SMALLICON      0xA003
#define FCIDM_TB_REPORTVIEW     0xA004
#define FCIDM_TB_DESKTOP        0xA005  /* FIXME */

#define FCIDM_BROWSERLAST       0xbf00
#define FCIDM_GLOBALFIRST       0x8000
#define FCIDM_GLOBALLAST        0x9fff

/*
* Global submenu IDs and separator IDs
*/
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

/* control IDs known to the view */
#define FCIDM_TOOLBAR      (FCIDM_BROWSERFIRST + 0)
#define FCIDM_STATUS       (FCIDM_BROWSERFIRST + 1)


/****************************************************************************
 * IShellIcon interface
 */

#define INTERFACE IShellIcon
#define IShellIcon_METHODS \
    IUnknown_METHODS \
    STDMETHOD(GetIconOf)(THIS_ LPCITEMIDLIST pidl, UINT flags, LPINT lpIconIndex) PURE;
ICOM_DEFINE(IShellIcon, IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IShellIcon_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellIcon_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IShellIcon_Release(p)                 (p)->lpVtbl->Release(p)
/*** IShellIcon methods ***/
#define IShellIcon_GetIconOf(p,a,b,c)         (p)->lpVtbl->GetIconOf(p,a,b,c)
#endif

/****************************************************************************
* SHAddToRecentDocs API
*/
#define SHARD_PIDL      0x00000001L
#define SHARD_PATHA     0x00000002L
#define SHARD_PATHW     0x00000003L
#define SHARD_PATH WINELIB_NAME_AW(SHARD_PATH)

DWORD WINAPI SHAddToRecentDocs(UINT uFlags, LPCVOID pv);

/****************************************************************************
 * SHBrowseForFolder API
 */
typedef INT (CALLBACK *BFFCALLBACK)(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

typedef struct tagBROWSEINFOA {
    HWND        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPSTR         pszDisplayName;
    LPCSTR        lpszTitle;
    UINT        ulFlags;
    BFFCALLBACK   lpfn;
    LPARAM        lParam;
    INT         iImage;
} BROWSEINFOA, *PBROWSEINFOA, *LPBROWSEINFOA;

typedef struct tagBROWSEINFOW {
    HWND        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPWSTR        pszDisplayName;
    LPCWSTR       lpszTitle;
    UINT        ulFlags;
    BFFCALLBACK   lpfn;
    LPARAM        lParam;
    INT         iImage;
} BROWSEINFOW, *PBROWSEINFOW, *LPBROWSEINFOW;

#define BROWSEINFO   WINELIB_NAME_AW(BROWSEINFO)
#define PBROWSEINFO  WINELIB_NAME_AW(PBROWSEINFO)
#define LPBROWSEINFO WINELIB_NAME_AW(LPBROWSEINFO)

/* Browsing for directory. */
#define BIF_RETURNONLYFSDIRS   0x0001
#define BIF_DONTGOBELOWDOMAIN  0x0002
#define BIF_STATUSTEXT         0x0004
#define BIF_RETURNFSANCESTORS  0x0008
#define BIF_EDITBOX            0x0010
#define BIF_VALIDATE           0x0020
#define BIF_NEWDIALOGSTYLE     0x0040   

#define BIF_BROWSEFORCOMPUTER  0x1000
#define BIF_BROWSEFORPRINTER   0x2000
#define BIF_BROWSEINCLUDEFILES 0x4000

/* message from browser */
#define BFFM_INITIALIZED        1
#define BFFM_SELCHANGED         2
#define BFFM_VALIDATEFAILEDA    3   /* lParam:szPath ret:1(cont),0(EndDialog) */
#define BFFM_VALIDATEFAILEDW    4   /* lParam:wzPath ret:1(cont),0(EndDialog) */

/* messages to browser */
#define BFFM_SETSTATUSTEXTA     (WM_USER+100)
#define BFFM_ENABLEOK           (WM_USER+101)
#define BFFM_SETSELECTIONA      (WM_USER+102)
#define BFFM_SETSELECTIONW      (WM_USER+103)
#define BFFM_SETSTATUSTEXTW     (WM_USER+104)
#define BFFM_SETOKTEXT          (WM_USER+105)
#define BFFM_SETEXPANDED        (WM_USER+106)

LPITEMIDLIST WINAPI SHBrowseForFolderA(LPBROWSEINFOA lpbi);
LPITEMIDLIST WINAPI SHBrowseForFolderW(LPBROWSEINFOW lpbi);
#define SHBrowseForFolder	 WINELIB_NAME_AW(SHBrowseForFolder)
#define BFFM_SETSTATUSTEXT  WINELIB_NAME_AW(BFFM_SETSTATUSTEXT)
#define BFFM_SETSELECTION   WINELIB_NAME_AW(BFFM_SETSELECTION)
#define BFFM_VALIDATEFAILED WINELIB_NAME_AW(BFFM_VALIDATEFAILED)

/****************************************************************************
*	SHGetDataFromIDList API
*/
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
#define SHDID_COMPUTER_IMAGING      18
#define SHDID_COMPUTER_AUDIO        19
#define SHDID_COMPUTER_SHAREDDOCS   20

typedef struct _SHDESCRIPTIONID
{   DWORD   dwDescriptionId;
    CLSID   clsid;
} SHDESCRIPTIONID, *LPSHDESCRIPTIONID;

HRESULT WINAPI SHGetDataFromIDListA(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID pv, int cb);
HRESULT WINAPI SHGetDataFromIDListW(LPSHELLFOLDER psf, LPCITEMIDLIST pidl, int nFormat, LPVOID pv, int cb);
#define  SHGetDataFromIDList WINELIB_NAME_AW(SHGetDataFromIDList)

BOOL WINAPI SHGetSpecialFolderPathA (HWND hwndOwner, LPSTR szPath, int nFolder, BOOL bCreate);
BOOL WINAPI SHGetSpecialFolderPathW (HWND hwndOwner, LPWSTR szPath, int nFolder, BOOL bCreate);
#define  SHGetSpecialFolderPath WINELIB_NAME_AW(SHGetSpecialFolderPath)

/****************************************************************************
* shlview structures
*/

/*
* IShellFolderViewCallback Callback
*  This "callback" is called by the shells default IShellView implementation (that
*  we got using SHCreateShellViewEx()), to notify us of the various things that
*  are happening to the shellview (and ask for things too).
*
*  You don't have to support anything here - anything you don't want to
*  handle, the shell will do itself if you just return E_NOTIMPL. This parameters
*  that the shell passes to this function are entirely undocumented.
*
*  HOWEVER, as the cabview sample as originally written used this callback, the
*  writers implemented the callback mechanism on top of their own IShellView.
*  Look there for some clues on what to do here.
*/

typedef HRESULT (CALLBACK *SHELLVIEWPROC)(DWORD dwUserParam,LPSHELLFOLDER psf,
                         HWND hwnd,UINT uMsg,UINT wParam,LPARAM lParam);

/* NF valid values for the "viewmode" item of the SHELLTEMPLATE*/
#define NF_INHERITVIEW    0x0000
#define NF_LOCALVIEW        0x0001

typedef struct _SHELLVIEWDATA   /* idl */
{ DWORD           dwSize;
  LPSHELLFOLDER   pShellFolder;
  DWORD           dwUserParam;
  LPCITEMIDLIST   pidl;
  DWORD           v3;        /* always 0 */
  SHELLVIEWPROC   pCallBack;
  DWORD           viewmode;  /* NF_* enum */
} SHELLVIEWDATA, * LPSHELLVIEWDATA;

HRESULT WINAPI SHGetMalloc(LPMALLOC *lpmal) ;

/**********************************************************************
 * SHGetSetSettings ()
 */

typedef struct
{
    BOOL fShowAllObjects : 1;
    BOOL fShowExtensions : 1;
    BOOL fNoConfirmRecycle : 1;

    BOOL fShowSysFiles : 1;
    BOOL fShowCompColor : 1;
    BOOL fDoubleClickInWebView : 1;
    BOOL fDesktopHTML : 1;
    BOOL fWin95Classic : 1;
    BOOL fDontPrettyPath : 1;
    BOOL fShowAttribCol : 1;
    BOOL fMapNetDrvBtn : 1;
    BOOL fShowInfoTip : 1;
    BOOL fHideIcons : 1;
    BOOL fWebView : 1;
    BOOL fFilter : 1;
    BOOL fShowSuperHidden : 1;
    BOOL fNoNetCrawling : 1;

    DWORD dwWin95Unused;
    UINT  uWin95Unused;
    LONG   lParamSort;
    int    iSortDirection;
    UINT   version;
    UINT uNotUsed;
    BOOL fSepProcess: 1;
    BOOL fStartPanelOn: 1;
    BOOL fShowStartPage: 1;
    UINT fSpareFlags : 13;
} SHELLSTATE, *LPSHELLSTATE;

/**********************************************************************
 * SHGetSettings ()
 */
typedef struct
{	BOOL fShowAllObjects : 1;
	BOOL fShowExtensions : 1;
	BOOL fNoConfirmRecycle : 1;
	BOOL fShowSysFiles : 1;

	BOOL fShowCompColor : 1;
	BOOL fDoubleClickInWebView : 1;
	BOOL fDesktopHTML : 1;
	BOOL fWin95Classic : 1;

	BOOL fDontPrettyPath : 1;
	BOOL fShowAttribCol : 1;
	BOOL fMapNetDrvBtn : 1;
	BOOL fShowInfoTip : 1;

	BOOL fHideIcons : 1;
	UINT fRestFlags : 3;
} SHELLFLAGSTATE, * LPSHELLFLAGSTATE;

VOID WINAPI SHGetSettings(LPSHELLFLAGSTATE lpsfs, DWORD dwMask);

#define SSF_SHOWALLOBJECTS		0x0001
#define SSF_SHOWEXTENSIONS		0x0002
#define SSF_SHOWCOMPCOLOR		0x0008
#define SSF_SHOWSYSFILES		0x0020
#define SSF_DOUBLECLICKINWEBVIEW	0x0080
#define SSF_SHOWATTRIBCOL		0x0100
#define SSF_DESKTOPHTML			0x0200
#define SSF_WIN95CLASSIC		0x0400
#define SSF_DONTPRETTYPATH		0x0800
#define SSF_SHOWINFOTIP			0x2000
#define SSF_MAPNETDRVBUTTON		0x1000
#define SSF_NOCONFIRMRECYCLE		0x8000
#define SSF_HIDEICONS			0x4000

/****************************************************************************
* SHRestricted API
*/
typedef enum RESTRICTIONS
{
	REST_NONE			= 0x00000000,
	REST_NORUN			= 0x00000001,
	REST_NOCLOSE			= 0x00000002,
	REST_NOSAVESET			= 0x00000004,
	REST_NOFILEMENU			= 0x00000008,
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
	REST_MYDOCSONNET		= 0x00040000,
	REST_NOEXITTODOS		= 0x00080000,
	REST_ENFORCESHELLEXTSECURITY	= 0x00100000,
	REST_LINKRESOLVEIGNORELINKINFO	= 0x00200000,
	REST_NOCOMMONGROUPS		= 0x00400000,
	REST_SEPARATEDESKTOPPROCESS	= 0x00800000,
	REST_NOWEB			= 0x01000000,
	REST_NOTRAYCONTEXTMENU		= 0x02000000,
	REST_NOVIEWCONTEXTMENU		= 0x04000000,
	REST_NONETCONNECTDISCONNECT	= 0x08000000,
	REST_STARTMENULOGOFF		= 0x10000000,
	REST_NOSETTINGSASSIST		= 0x20000000,
	REST_NOINTERNETICON		= 0x40000001,
	REST_NORECENTDOCSHISTORY,
	REST_NORECENTDOCSMENU,
	REST_NOACTIVEDESKTOP,
	REST_NOACTIVEDESKTOPCHANGES,
	REST_NOFAVORITESMENU,
	REST_CLEARRECENTDOCSONEXIT,
	REST_CLASSICSHELL,
	REST_NOCUSTOMIZEWEBVIEW,

	REST_NOHTMLWALLPAPER		= 0x40000010,
	REST_NOCHANGINGWALLPAPER,
	REST_NODESKCOMP,
	REST_NOADDDESKCOMP,
	REST_NODELDESKCOMP,
	REST_NOCLOSEDESKCOMP,
	REST_NOCLOSE_DRAGDROPBAND,
	REST_NOMOVINGBAND,
	REST_NOEDITDESKCOMP,
	REST_NORESOLVESEARCH,
	REST_NORESOLVETRACK,
	REST_FORCECOPYACLWITHFILE,
	REST_NOLOGO3CHANNELNOTIFY,
	REST_NOFORGETSOFTWAREUPDATE,
	REST_NOSETACTIVEDESKTOP,
	REST_NOUPDATEWINDOWS,
	REST_NOCHANGESTARMENU,		/* 0x40000020 */
	REST_NOFOLDEROPTIONS,
	REST_HASFINDCOMPUTERS,
	REST_INTELLIMENUS,
	REST_RUNDLGMEMCHECKBOX,
	REST_ARP_ShowPostSetup,
	REST_NOCSC,
	REST_NOCONTROLPANEL,
	REST_ENUMWORKGROUP,
	REST_ARP_NOARP,
	REST_ARP_NOREMOVEPAGE,
	REST_ARP_NOADDPAGE,
	REST_ARP_NOWINSETUPPAGE,
	REST_GREYMSIADS,
	REST_NOCHANGEMAPPEDDRIVELABEL,
	REST_NOCHANGEMAPPEDDRIVECOMMENT,
	REST_MaxRecentDocs,		/* 0x40000030 */
	REST_NONETWORKCONNECTIONS,
	REST_FORCESTARTMENULOGOFF,
	REST_NOWEBVIEW,
	REST_NOCUSTOMIZETHISFOLDER,
	REST_NOENCRYPTION,

	REST_ALLOWFRENCHENCRYPTION,	/* not documented */

	REST_DONTSHOWSUPERHIDDEN,
	REST_NOSHELLSEARCHBUTTON,
	REST_NOHARDWARETAB,
	REST_NORUNASINSTALLPROMPT,
	REST_PROMPTRUNASINSTALLNETPATH,
	REST_NOMANAGEMYCOMPUTERVERB,
	REST_NORECENTDOCSNETHOOD,
	REST_DISALLOWRUN,
	REST_NOWELCOMESCREEN,
	REST_RESTRICTCPL,		/* 0x40000040 */
	REST_DISALLOWCPL,
	REST_NOSMBALLOONTIP,
	REST_NOSMHELP,
	REST_NOWINKEYS,
	REST_NOENCRYPTONMOVE,
	REST_NOLOCALMACHINERUN,
	REST_NOCURRENTUSERRUN,
	REST_NOLOCALMACHINERUNONCE,
	REST_NOCURRENTUSERRUNONCE,
	REST_FORCEACTIVEDESKTOPON,
	REST_NOCOMPUTERSNEARME,
	REST_NOVIEWONDRIVE,
	REST_NONETCRAWL,
	REST_NOSHAREDDOCUMENTS,
	REST_NOSMMYDOCS,
	REST_NOSMMYPICS,		/* 0x40000050 */
	REST_ALLOWBITBUCKDRIVES,
	REST_NONLEGACYSHELLMODE,
	REST_NOCONTROLPANELBARRICADE,
	REST_NOSTARTPAGE,
	REST_NOAUTOTRAYNOTIFY,
	REST_NOTASKGROUPING,
	REST_NOCDBURNING,
	REST_MYCOMPNOPROP,
	REST_MYDOCSNOPROP,
	REST_NOSTARTPANEL,
	REST_NODISPLAYAPPEARANCEPAGE,
	REST_NOTHEMESTAB,
	REST_NOVISUALSTYLECHOICE,
	REST_NOSIZECHOICE,
	REST_NOCOLORCHOICE,
	REST_SETVISUALSTYLE,		/* 0x40000060 */
	REST_STARTRUNNOHOMEPATH,
	REST_NOUSERNAMEINSTARTPANEL,
	REST_NOMYCOMPUTERICON,
	REST_NOSMNETWORKPLACES,
	REST_NOSMPINNEDLIST,
	REST_NOSMMYMUSIC,
	REST_NOSMEJECTPC,
	REST_NOSMMOREPROGRAMS,
	REST_NOSMMFUPROGRAMS,
	REST_NOTRAYITEMSDISPLAY,
	REST_NOTOOLBARSONTASKBAR,
	/* 0x4000006C
	   0x4000006D
	   0x4000006E */
	REST_NOSMCONFIGUREPROGRAMS	= 0x4000006F,
	REST_HIDECLOCK,			/* 0x40000070 */
	REST_NOLOWDISKSPACECHECKS,
	REST_NOENTIRENETWORK,
	REST_NODESKTOPCLEANUP,
	REST_BITBUCKNUKEONDELETE,
	REST_BITBUCKCONFIRMDELETE,
	REST_BITBUCKNOPROP,
	REST_NODISPBACKGROUND,
	REST_NODISPSCREENSAVEPG,
	REST_NODISPSETTINGSPG,
	REST_NODISPSCREENSAVEPREVIEW,
	REST_NODISPLAYCPL,
	REST_HIDERUNASVERB,
	REST_NOTHUMBNAILCACHE,
	REST_NOSTRCMPLOGICAL,
	REST_NOPUBLISHWIZARD,
	REST_NOONLINEPRINTSWIZARD,	/* 0x40000080 */
	REST_NOWEBSERVICES,
	REST_ALLOWUNHASHEDWEBVIEW,
	REST_ALLOWLEGACYWEBVIEW,
	REST_REVERTWEBVIEWSECURITY,
	
	REST_INHERITCONSOLEHANDLES	= 0x40000086,

	REST_NODISCONNECT		= 0x41000001,
	REST_NOSECURITY,
	REST_NOFILEASSOCIATE,		/* 0x41000003 */
} RESTRICTIONS;

DWORD WINAPI SHRestricted(RESTRICTIONS rest);

/****************************************************************************
* SHChangeNotify API
*/
typedef struct _SHChangeNotifyEntry
{
    LPCITEMIDLIST pidl;
    BOOL   fRecursive;
} SHChangeNotifyEntry;

#define SHCNE_RENAMEITEM	0x00000001
#define SHCNE_CREATE		0x00000002
#define SHCNE_DELETE		0x00000004
#define SHCNE_MKDIR		0x00000008
#define SHCNE_RMDIR		0x00000010
#define SHCNE_MEDIAINSERTED	0x00000020
#define SHCNE_MEDIAREMOVED	0x00000040
#define SHCNE_DRIVEREMOVED	0x00000080
#define SHCNE_DRIVEADD		0x00000100
#define SHCNE_NETSHARE		0x00000200
#define SHCNE_NETUNSHARE	0x00000400
#define SHCNE_ATTRIBUTES	0x00000800
#define SHCNE_UPDATEDIR		0x00001000
#define SHCNE_UPDATEITEM	0x00002000
#define SHCNE_SERVERDISCONNECT	0x00004000
#define SHCNE_UPDATEIMAGE	0x00008000
#define SHCNE_DRIVEADDGUI	0x00010000
#define SHCNE_RENAMEFOLDER	0x00020000
#define SHCNE_FREESPACE		0x00040000

#define SHCNE_EXTENDED_EVENT	0x04000000
#define SHCNE_ASSOCCHANGED	0x08000000
#define SHCNE_DISKEVENTS	0x0002381F
#define SHCNE_GLOBALEVENTS	0x0C0581E0
#define SHCNE_ALLEVENTS		0x7FFFFFFF
#define SHCNE_INTERRUPT		0x80000000

#define SHCNEE_ORDERCHANGED	0x0002L
#define SHCNEE_MSI_CHANGE	0x0004L
#define SHCNEE_MSI_UNINSTALL	0x0005L

#define SHCNF_IDLIST		0x0000
#define SHCNF_PATHA		0x0001
#define SHCNF_PRINTERA		0x0002
#define SHCNF_DWORD		0x0003
#define SHCNF_PATHW		0x0005
#define SHCNF_PRINTERW		0x0006
#define SHCNF_TYPE		0x00FF
#define SHCNF_FLUSH		0x1000
#define SHCNF_FLUSHNOWAIT	0x2000

#define SHCNF_PATH              WINELIB_NAME_AW(SHCNF_PATH)
#define SHCNF_PRINTER           WINELIB_NAME_AW(SHCNF_PRINTER)

void WINAPI SHChangeNotify(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2);

/*
 * IShellChangeNotify
 */
typedef struct IShellChangeNotify IShellChangeNotify, *LPSHELLCHANGENOTIFY;

#define INTERFACE IShellChangeNotify
#define IShellChangeNotify_METHODS \
    IUnknown_METHODS \
    STDMETHOD(OnChange)(THIS_ LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) PURE;

ICOM_DEFINE(IShellChangeNotify, IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IShellChangeNotify_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IShellChangeNotify_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IShellChangeNotify_Release(p)                 (p)->lpVtbl->Release(p)
/*** IShellChangeNotify methods ***/
#define IShellChangeNotify_OnChange(p,a,b,c)         (p)->lpVtbl->OnChange(p,a,b,c)
#endif

typedef struct _SHChangeDWORDAsIDList {
    USHORT   cb;
    DWORD    dwItem1;
    DWORD    dwItem2;
    USHORT   cbZero;
} SHChangeDWORDAsIDList, *LPSHChangeDWORDAsIDList;

typedef struct _SHChangeProductKeyAsIDList {
    USHORT cb;
    WCHAR wszProductKey[39];
    USHORT cbZero;
} SHChangeProductKeyAsIDList, *LPSHChangeProductKeyAsIDList;

ULONG WINAPI SHChangeNotifyRegister(HWND hwnd, int fSources, LONG fEvents, UINT wMsg,
                                    int cEntries, SHChangeNotifyEntry *pshcne);
BOOL WINAPI SHChangeNotifyDeregister(ULONG ulID);
HANDLE WINAPI SHChangeNotification_Lock(HANDLE hChangeNotification, DWORD dwProcessId,
                                        LPITEMIDLIST **pppidl, LONG *plEvent);
BOOL WINAPI SHChangeNotification_Unlock(HANDLE hLock);

HRESULT WINAPI SHGetRealIDL(IShellFolder *psf, LPCITEMIDLIST pidlSimple, LPITEMIDLIST * ppidlReal);

/****************************************************************************
* SHCreateDirectory API
*/
DWORD WINAPI SHCreateDirectory(HWND, LPCVOID);
DWORD WINAPI SHCreateDirectoryExA(HWND, LPCSTR, LPSECURITY_ATTRIBUTES);
DWORD WINAPI SHCreateDirectoryExW(HWND, LPCWSTR, LPSECURITY_ATTRIBUTES);

/****************************************************************************
* SHGetSpecialFolderLocation API
*/
HRESULT WINAPI SHGetSpecialFolderLocation(HWND hwndOwner, int nFolder, LPITEMIDLIST * ppidl);
HRESULT WINAPI SHGetFolderLocation(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwReserved, LPITEMIDLIST *ppidl);

#define CSIDL_DESKTOP		0x0000
#define CSIDL_INTERNET		0x0001
#define CSIDL_PROGRAMS		0x0002
#define CSIDL_CONTROLS		0x0003
#define CSIDL_PRINTERS		0x0004
#define CSIDL_PERSONAL		0x0005
#define CSIDL_FAVORITES		0x0006
#define CSIDL_STARTUP		0x0007
#define CSIDL_RECENT		0x0008
#define CSIDL_SENDTO		0x0009
#define CSIDL_BITBUCKET		0x000a
#define CSIDL_STARTMENU		0x000b
#define CSIDL_MYDOCUMENTS	0x000c
#define CSIDL_MYMUSIC		0x000d
#define CSIDL_MYVIDEO		0x000e
#define CSIDL_DESKTOPDIRECTORY	0x0010
#define CSIDL_DRIVES		0x0011
#define CSIDL_NETWORK		0x0012
#define CSIDL_NETHOOD		0x0013
#define CSIDL_FONTS		0x0014
#define CSIDL_TEMPLATES		0x0015
#define CSIDL_COMMON_STARTMENU	0x0016
#define CSIDL_COMMON_PROGRAMS	0X0017
#define CSIDL_COMMON_STARTUP	0x0018
#define CSIDL_COMMON_DESKTOPDIRECTORY	0x0019
#define CSIDL_APPDATA		0x001a
#define CSIDL_PRINTHOOD		0x001b
#define CSIDL_LOCAL_APPDATA	0x001c
#define CSIDL_ALTSTARTUP	0x001d
#define CSIDL_COMMON_ALTSTARTUP	0x001e
#define CSIDL_COMMON_FAVORITES  0x001f
#define CSIDL_INTERNET_CACHE	0x0020
#define CSIDL_COOKIES		0x0021
#define CSIDL_HISTORY		0x0022
#define CSIDL_COMMON_APPDATA	0x0023
#define CSIDL_WINDOWS		0x0024
#define CSIDL_SYSTEM		0x0025
#define CSIDL_PROGRAM_FILES	0x0026
#define CSIDL_MYPICTURES	0x0027
#define CSIDL_PROFILE		0x0028
#define CSIDL_SYSTEMX86		0x0029
#define CSIDL_PROGRAM_FILESX86	0x002a
#define CSIDL_PROGRAM_FILES_COMMON	0x002b
#define CSIDL_PROGRAM_FILES_COMMONX86	0x002c
#define CSIDL_COMMON_TEMPLATES	0x002d
#define CSIDL_COMMON_DOCUMENTS	0x002e
#define CSIDL_COMMON_ADMINTOOLS	0x002f
#define CSIDL_ADMINTOOLS	0x0030
#define CSIDL_CONNECTIONS	0x0031
#define CSIDL_COMMON_MUSIC	0x0035
#define CSIDL_COMMON_PICTURES	0x0036
#define CSIDL_COMMON_VIDEO	0x0037
#define CSIDL_RESOURCES		0x0038
#define CSIDL_RESOURCES_LOCALIZED 0x0039
#define CSIDL_COMMON_OEM_LINKS	0x003a
#define CSIDL_CDBURN_AREA	0x003b
#define CSIDL_COMPUTERSNEARME	0x003d
#define CSIDL_PROFILES		0x003e
#define CSIDL_FOLDER_MASK	0x00ff
#define CSIDL_FLAG_PER_USER_INIT 0x0800
#define CSIDL_FLAG_NO_ALIAS	0x1000
#define CSIDL_FLAG_DONT_VERIFY	0x4000
#define CSIDL_FLAG_CREATE	0x8000

#define CSIDL_FLAG_MASK		0xff00

/****************************************************************************
 * SHGetDesktopFolder API
 */
DWORD WINAPI SHGetDesktopFolder(IShellFolder * *);

/****************************************************************************
 * SHBindToParent API
 */
HRESULT WINAPI SHBindToParent(LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppv, LPCITEMIDLIST *ppidlLast);

/****************************************************************************
* SHDefExtractIcon API
*/
HRESULT WINAPI SHDefExtractIconA(LPCSTR pszIconFile, int iIndex, UINT uFlags,
                                 HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize);
HRESULT WINAPI SHDefExtractIconW(LPCWSTR pszIconFile, int iIndex, UINT uFlags,
                                 HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize);
#define        SHDefExtractIcon WINELIB_NAME_AW(SHDefExtractIcon)

/*
 * DROPFILES for CF_HDROP and CF_PRINTERS
 */
typedef struct _DROPFILES
{
  DWORD pFiles;
  POINT pt;
  BOOL  fNC;
  BOOL  fWide;
} DROPFILES, *LPDROPFILES;

#include <poppack.h> 

/*****************************************************************************
 * IFileSystemBindData interface
 */
#ifndef __IFileSystemBindData_FWD_DEFINED__
#define __IFileSystemBindData_FWD_DEFINED__
typedef struct IFileSystemBindData IFileSystemBindData;
#endif

typedef IFileSystemBindData *LPFILESYSTEMBINDDATA;

#ifndef __IFileSystemBindData_INTERFACE_DEFINED__
#define __IFileSystemBindData_INTERFACE_DEFINED__

DEFINE_GUID(IID_IFileSystemBindData, 0x01e18d10, 0x4d8b, 0x11d2, 0x85,0x5d, 0x00,0x60,0x08,0x05,0x93,0x67);
#if defined(__cplusplus) && !defined(CINTERFACE)
struct IFileSystemBindData : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetFindData(
        WIN32_FIND_DATAW* pfd) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetFindData(
        const WIN32_FIND_DATAW* pfd) = 0;

};
#else
typedef struct IFileSystemBindDataVtbl IFileSystemBindDataVtbl;
struct IFileSystemBindData {
    const IFileSystemBindDataVtbl* lpVtbl;
};
struct IFileSystemBindDataVtbl {
    ICOM_MSVTABLE_COMPAT_FIELDS

    /*** IUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IFileSystemBindData* This,
        REFIID riid,
        void** ppvObject);

    ULONG (STDMETHODCALLTYPE *AddRef)(
        IFileSystemBindData* This);

    ULONG (STDMETHODCALLTYPE *Release)(
        IFileSystemBindData* This);

    /*** IFileSystemBindData methods ***/
    HRESULT (STDMETHODCALLTYPE *GetFindData)(
        IFileSystemBindData* This,
        WIN32_FIND_DATAW* pfd);

    HRESULT (STDMETHODCALLTYPE *SetFindData)(
        IFileSystemBindData* This,
        const WIN32_FIND_DATAW* pfd);

};

/*** IUnknown methods ***/
#define IFileSystemBindData_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IFileSystemBindData_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IFileSystemBindData_Release(p) (p)->lpVtbl->Release(p)
/*** IFileSystemBindData methods ***/
#define IFileSystemBindData_GetFindData(p,a) (p)->lpVtbl->GetFindData(p,a)
#define IFileSystemBindData_SetFindData(p,a) (p)->lpVtbl->SetFindData(p,a)

#endif

#define IFileSystemBindData_METHODS \
    ICOM_MSVTABLE_COMPAT_FIELDS \
    /*** IUnknown methods ***/ \
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE; \
    STDMETHOD_(ULONG,AddRef)(THIS) PURE; \
    STDMETHOD_(ULONG,Release)(THIS) PURE; \
    /*** IFileSystemBindData methods ***/ \
    STDMETHOD_(HRESULT,GetFindData)(THIS_ WIN32_FIND_DATAW* pfd) PURE; \
    STDMETHOD_(HRESULT,SetFindData)(THIS_ const WIN32_FIND_DATAW* pfd) PURE;

#endif  /* __IFileSystemBindData_INTERFACE_DEFINED__ */


/****************************************************************************
 * Drag And Drop Routines
 */

/* DAD_AutoScroll sample structure */
#define NUM_POINTS 3
typedef struct
{
    int   iNextSample;
    DWORD dwLastScroll;
    BOOL  bFull;
    POINT pts[NUM_POINTS];
    DWORD dwTimes[NUM_POINTS];
} AUTO_SCROLL_DATA;

BOOL         WINAPI DAD_SetDragImage(HIMAGELIST,LPPOINT);
BOOL         WINAPI DAD_DragEnterEx(HWND,POINT);
BOOL         WINAPI DAD_DragEnterEx2(HWND,POINT,IDataObject*);
BOOL         WINAPI DAD_DragMove(POINT);
BOOL         WINAPI DAD_DragLeave(void);
BOOL         WINAPI DAD_AutoScroll(HWND,AUTO_SCROLL_DATA*,LPPOINT);
HRESULT      WINAPI SHDoDragDrop(HWND,IDataObject*,IDropSource*,DWORD,LPDWORD);


/****************************************************************************
 * Cabinet functions
 */

typedef struct {
    WORD cLength;
    WORD nVersion;
    BOOL fFullPathTitle:1;
    BOOL fSaveLocalView:1;
    BOOL fNotShell:1;
    BOOL fSimpleDefault:1;
    BOOL fDontShowDescBar:1;
    BOOL fNewWindowMode:1;
    BOOL fShowCompColor:1;
    BOOL fDontPrettyNames:1;
    BOOL fAdminsCreateCommonGroups:1;
    UINT fUnusedFlags:7;
    UINT fMenuEnumFilter;
} CABINETSTATE, *LPCABINETSTATE;

#define CABINETSTATE_VERSION 2

BOOL WINAPI ReadCabinetState(CABINETSTATE *, int);
BOOL WINAPI WriteCabinetState(CABINETSTATE *);


#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_SHLOBJ_H */
