/************************************************************
 *    IContextMenu
 *
 * Undocumented:
 * word95 gets a IContextMenu Interface and calls HandleMenuMsg()
 * whitch should only a member of IContextMenu2. 
 */

#ifndef __WINE_WINE_OBJ_CONTEXTMENU_H
#define __WINE_WINE_OBJ_CONTEXTMENU_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

typedef struct IContextMenu IContextMenu, *LPCONTEXTMENU;

/* QueryContextMenu uFlags */
#define CMF_NORMAL              0x00000000
#define CMF_DEFAULTONLY         0x00000001
#define CMF_VERBSONLY           0x00000002
#define CMF_EXPLORE             0x00000004
#define CMF_NOVERBS             0x00000008
#define CMF_CANRENAME           0x00000010
#define CMF_NODEFAULT           0x00000020
#define CMF_INCLUDESTATIC       0x00000040
#define CMF_RESERVED            0xffff0000      /* View specific */

/* GetCommandString uFlags */
#define GCS_VERBA        0x00000000     /* canonical verb */
#define GCS_HELPTEXTA    0x00000001     /* help text (for status bar) */
#define GCS_VALIDATEA    0x00000002     /* validate command exists */
#define GCS_VERBW        0x00000004     /* canonical verb (unicode) */
#define GCS_HELPTEXTW    0x00000005     /* help text (unicode version) */
#define GCS_VALIDATEW    0x00000006     /* validate command exists (unicode) */
#define GCS_UNICODE      0x00000004     /* for bit testing - Unicode string */

#define GCS_VERB        GCS_VERBA
#define GCS_HELPTEXT    GCS_HELPTEXTA
#define GCS_VALIDATE    GCS_VALIDATEA

#define CMDSTR_NEWFOLDERA   "NewFolder"
#define CMDSTR_VIEWLISTA    "ViewList"
#define CMDSTR_VIEWDETAILSA "ViewDetails"
static const WCHAR CMDSTR_NEWFOLDERW[] = {'N','e','w','F','o','l','d','e','r',0};
static const WCHAR CMDSTR_VIEWLISTW [] = {'V','i','e','w','L','i','s','t',0};
static const WCHAR CMDSTR_VIEWDETAILSW[] = {'V','i','e','w','D','e','t','a','i','l','s',0};

#define CMDSTR_NEWFOLDER    CMDSTR_NEWFOLDERA
#define CMDSTR_VIEWLIST     CMDSTR_VIEWLISTA
#define CMDSTR_VIEWDETAILS  CMDSTR_VIEWDETAILSA

#define CMIC_MASK_HOTKEY        SEE_MASK_HOTKEY
#define CMIC_MASK_ICON          SEE_MASK_ICON
#define CMIC_MASK_FLAG_NO_UI    SEE_MASK_FLAG_NO_UI
#define CMIC_MASK_UNICODE       SEE_MASK_UNICODE
#define CMIC_MASK_NO_CONSOLE    SEE_MASK_NO_CONSOLE
#define CMIC_MASK_HASLINKNAME   SEE_MASK_HASLINKNAME
#define CMIC_MASK_FLAG_SEP_VDM  SEE_MASK_FLAG_SEPVDM
#define CMIC_MASK_HASTITLE      SEE_MASK_HASTITLE
#define CMIC_MASK_ASYNCOK       SEE_MASK_ASYNCOK

#define CMIC_MASK_PTINVOKE      0x20000000

/*NOTE: When SEE_MASK_HMONITOR is set, hIcon is treated as hMonitor */
typedef struct tagCMINVOKECOMMANDINFO 
{	DWORD cbSize;        /* sizeof(CMINVOKECOMMANDINFO) */
	DWORD fMask;         /* any combination of CMIC_MASK_* */
	HWND hwnd;         /* might be NULL (indicating no owner window) */
	LPCSTR lpVerb;       /* either a string or MAKEINTRESOURCE(idOffset) */
	LPCSTR lpParameters; /* might be NULL (indicating no parameter) */
	LPCSTR lpDirectory;  /* might be NULL (indicating no specific directory) */
	INT nShow;           /* one of SW_ values for ShowWindow() API */

	DWORD dwHotKey;
	HANDLE hIcon;
} CMINVOKECOMMANDINFO,  *LPCMINVOKECOMMANDINFO;

typedef struct tagCMInvokeCommandInfoEx 
{	DWORD cbSize;        /* must be sizeof(CMINVOKECOMMANDINFOEX) */
	DWORD fMask;         /* any combination of CMIC_MASK_* */
	HWND hwnd;         /* might be NULL (indicating no owner window) */
	LPCSTR lpVerb;       /* either a string or MAKEINTRESOURCE(idOffset) */
	LPCSTR lpParameters; /* might be NULL (indicating no parameter) */
	LPCSTR lpDirectory;  /* might be NULL (indicating no specific directory) */
	INT nShow;           /* one of SW_ values for ShowWindow() API */

	DWORD dwHotKey;

	HANDLE hIcon;
	LPCSTR lpTitle;        /* For CreateProcess-StartupInfo.lpTitle */
	LPCWSTR lpVerbW;       /* Unicode verb (for those who can use it) */
	LPCWSTR lpParametersW; /* Unicode parameters (for those who can use it) */
	LPCWSTR lpDirectoryW;  /* Unicode directory (for those who can use it) */
	LPCWSTR lpTitleW;      /* Unicode title (for those who can use it) */
	POINT ptInvoke;      /* Point where it's invoked */

} CMINVOKECOMMANDINFOEX,  *LPCMINVOKECOMMANDINFOEX;

#define ICOM_INTERFACE IContextMenu
#define IContextMenu_METHODS \
	ICOM_METHOD5(HRESULT, QueryContextMenu, HMENU, hmenu, UINT, indexMenu, UINT, idCmdFirst, UINT, idCmdLast, UINT, uFlags) \
	ICOM_METHOD1(HRESULT, InvokeCommand, LPCMINVOKECOMMANDINFO, lpici) \
	ICOM_METHOD5(HRESULT, GetCommandString, UINT, idCmd, UINT, uType, UINT*, pwReserved, LPSTR, pszName, UINT, cchMax) \
	ICOM_METHOD3(HRESULT, HandleMenuMsg, UINT, uMsg, WPARAM, wParam, LPARAM, lParam) \
	void * guard;   /*possibly another nasty entry from ContextMenu3 ?*/
#define IContextMenu_IMETHODS \
	IUnknown_IMETHODS \
	IContextMenu_METHODS
ICOM_DEFINE(IContextMenu,IUnknown)
#undef ICOM_INTERFACE

#define IContextMenu_QueryInterface(p,a,b)		ICOM_CALL2(QueryInterface,p,a,b)
#define IContextMenu_AddRef(p)				ICOM_CALL(AddRef,p)
#define IContextMenu_Release(p)				ICOM_CALL(Release,p)
#define IContextMenu_QueryContextMenu(p,a,b,c,d,e)	ICOM_CALL5(QueryContextMenu,p,a,b,c,d,e)
#define IContextMenu_InvokeCommand(p,a)			ICOM_CALL1(InvokeCommand,p,a)
#define IContextMenu_GetCommandString(p,a,b,c,d,e)	ICOM_CALL5(GetCommandString,p,a,b,c,d,e)
#define IContextMenu_HandleMenuMsg(p,a,b,c)		ICOM_CALL3(HandleMenuMsg,p,a,b,c)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_CONTEXTMENU_H */
