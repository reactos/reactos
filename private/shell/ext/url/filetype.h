//================================================================
//
//  (C) Copyright MICROSOFT Corp., 1994
//
//  TITLE:       FILETYPE.H
//  VERSION:     1.0
//  DATE:        5/10/94
//  AUTHOR:      Vince Roggero (vincentr)
//
//================================================================
//
//  CHANGE LOG:
//
//  DATE         REV DESCRIPTION
//  -----------  --- ---------------------------------------------
//               VMR Original version
//================================================================

//================================================================
//  File Types Control Panel Applet
//================================================================
#ifndef _INC_FILETYPE_
#define _INC_FILETYPE_

#ifdef FILETYPE_CPL
#define INC_OLE2
//#define _SHSEMIP_H_
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cpl.h>
#include <stdarg.h>
#include <shlobj.h>
#include "rcids.h"
#include "cstrings.h"
#include "shell.h"

#define IDI_FILETYPES				103
#define STR_DESKTOPCLASS "Progman"
#endif

//================================================================
// defines
//================================================================
#define FILETYPESHEETS          1
#define MAXTEXTLEN		32
#define NUMITEMS		7
#define NUMCOLUMNS		2
#define MAXCOLUMNHDG            64
#define MAXEXTSIZE              (PATH_CCH_EXT+2)
#define FTD_EDIT		1
#define FTD_EXT			2
#define FTD_COMMAND		3
#define FTD_DOCICON		4
#ifdef MIME
#define FTD_MIME        5
#endif   /* MIME */
#define WM_CTRL_SETFOCUS	WM_USER + 1
#define SHELLEXEICONINDEX   2
#define SHELLDEFICONINDEX   44


// File Type Attributes key's bitmap values (HKEY_CLASSES_ROOT\filetype,Attributes)
#define FTA_Exclude       0x00000001 //  1. used to exclude types like drvfile
#define FTA_Show          0x00000002 //  2. used to show types like folder that don't have associations
#define FTA_HasExtension  0x00000004 //  3. type has assoc extension
#define FTA_NoEdit        0x00000008 //  4. no editing of file type
#define FTA_NoRemove      0x00000010 //  5. no deling of the file type
#define FTA_NoNewVerb     0x00000020 //  6. no adding of verbs
#define FTA_NoEditVerb    0x00000040 //  7. no editing of predefined verbs
#define FTA_NoRemoveVerb  0x00000080 //  8. no deling of predefined verbs
#define FTA_NoEditDesc    0x00000100 //  9. no editing of file type description
#define FTA_NoEditIcon    0x00000200 // 10. no editing of doc icon
#define FTA_NoEditDflt    0x00000400 // 11. no changing of default verb
#define FTA_NoEditVerbCmd 0x00000800 // 12. no editing of the verbs command
#define FTA_NoEditVerbExe 0x00001000 // 13. no editing of the verbs exe
#define FTA_NoDDE         0x00002000 // 14. no editing of the DDE fields
#define FTA_ExtShellOpen  0x00004000 // 15. old style type: HKCR/.ext/shell/open/command
#ifdef MIME
#define FTA_NoEditMIME    0x00008000 // 16. no editing of the Content Type or Default Extension fields
#define FTA_OpenIsSafe    0x00010000 // 17. the file class's open verb may be safely invoked for downloaded files
#endif   /* MIME */

#define FTAV_UserDefVerb  0x00000001 // 1. identifies verb as being user defined (!predefined)

//================================================================
// typedef's
//================================================================

#ifdef MIME

typedef enum mimeflags
{
   /* The Content Type combo box drop down has been filled with MIME types. */

   MIME_FL_CONTENT_TYPES_ADDED   = 0x0001,

   /* flag combinations */

   ALL_MIME_FLAGS                = MIME_FL_CONTENT_TYPES_ADDED
}
MIMEFLAGS;

#endif

typedef struct tagFILETYPESINFO
{
	char szId[MAX_PATH];			// file type identifier		mmmfile
	char szDesc[MAX_PATH];			// file type description	Animation
	char szDefaultAction[MAX_PATH];		// default verb				mmmfile\shell = (szDefaultAction=open)
	DWORD dwAttributes;			// Optional attributes for type;	mmmfile\Attributes
	HICON hIconDoc;				// Large icon for doc
	HICON hIconOpen;			// Large icon for exe
	HKEY hkeyFT;				// HKEY for this file type
	HDPA hDPAExt;				// dynamic pointer array for list of extensions
#ifdef MIME
        DWORD dwMIMEFlags;                      // flags from MIMEFLAGS
	char szOriginalMIMEType[MAX_PATH];      // original MIME type
#endif   /* MIME */
} FILETYPESINFO, *PFILETYPESINFO;

typedef struct tagFILETYPESCOMMANDINFO
{
	char szId[MAX_PATH];			// file type identifier	mmmfile
	char szActionKey[MAX_PATH];		// action verb			mmmfile\shell\open (szAction=open)
	char szActionValue[MAX_PATH];		// action verb			mmmfile\shell\open=Open (szAction=open)
	char szCommand[MAX_PATH];		// app to execute		mmmfile\shell\open\command = szCommand (C:\FOO.EXE %1)
	char szDDEMsg[MAX_PATH];		// DDE message			mmmfile\shell\open\ddeexec = szDDEMsg
	char szDDEApp[MAX_PATH];		// DDE application		mmmfile\shell\open\ddeexec\application = szDDEApp
	char szDDEAppNot[MAX_PATH];		// DDE application !run	mmmfile\shell\open\ddeexec\ifexec = szDDEAppNot
	char szDDETopic[MAX_PATH];		// DDE topic			mmmfile\shell\open\ddeexec\topic = szDDETopic
	DWORD dwVerbAttributes;			// Optional attributes for verb;	mmmfile\shell\verb -> Attributes=dwVerbAttributes
} FILETYPESCOMMANDINFO, *PFILETYPESCOMMANDINFO;

typedef struct tagFILETYPESDIALOGINFO
{
	HWND hPropDialog;
	HWND hEditDialog;
	HWND hCmdDialog;
	HWND hwndDocIcon;
	HWND hwndOpenIcon;
	HWND hwndEditDocIcon;
	HWND hwndDocExt;
#ifdef MIME
        HWND hwndContentTypeComboBox;
        HWND hwndDefExtensionComboBox;
#endif   /* MIME */
	HWND hwndLVFT;
	HWND hwndLVFTEdit;
	HIMAGELIST himlFT;
	HIMAGELIST himlLarge;			// System image list large icons
	HIMAGELIST himlSmall;			// System image list small icons
	HANDLE hThread;
	DWORD dwCommand;				// Edit or New
	DWORD dwCommandEdit;			// Edit or New
	INT iItem;						// File Type ListView's item
	INT iEditItem;					// Edit ListView's item
	char szId[MAX_PATH];			// file type identifier	mmmfile
	char szIconPath[MAX_PATH];
	INT iIconIndex;
    HFONT hfReg;
    HFONT hfBold;
	PFILETYPESINFO pFTInfo;
	PFILETYPESCOMMANDINFO pFTCInfo;
} FILETYPESDIALOGINFO, *PFILETYPESDIALOGINFO;

//================================================================
// Prototypes
//================================================================
INT_PTR CALLBACK FT_DlgProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FTEdit_DlgProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FTCmd_DlgProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditExtSubClassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int CALLBACK ListViewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

VOID FileTypesProperties(HWND hwndOwner);
BOOL FT_InitListViewCols(HWND hwndLV);
BOOL FTEdit_InitListViewCols(HWND hwndLV);
BOOL FT_InitListView(PFILETYPESDIALOGINFO pFTDInfo);
int FTEdit_InitListView(PFILETYPESDIALOGINFO pFTDInfo);
BOOL FTEdit_AddInfoToLV(PFILETYPESDIALOGINFO pFTDInfo, LPSTR szActionKey, LPSTR szActionValue, LPSTR szId, HKEY hk);
BOOL FT_AddInfoToLV(PFILETYPESDIALOGINFO pFTDInfo, HKEY hkey, LPSTR szExt, LPSTR szDesc, LPSTR szId, DWORD dwAttributes);
VOID AddExtDot(LPSTR pszExt, UINT iExt);
VOID ExtToShellCommand(HKEY hkeyFT, LPSTR pszName, UINT uName);
VOID VerbToExe(HKEY hkeyFT, LPSTR pszVerb, LPSTR pszExe, DWORD *pdwExe);
BOOL FindDDEOptions(PFILETYPESDIALOGINFO pFTDInfo);
HICON GetDefaultIcon(HKEY *hkeyFT, LPSTR pszId, DWORD dwIconType);
BOOL DefaultAction(HKEY hkeyFT, LPSTR pszDefaultAction, DWORD *dwDefaultAction);
VOID FT_Cleanup(PFILETYPESDIALOGINFO pFTDInfo);
VOID FTEdit_Cleanup(PFILETYPESDIALOGINFO pFTDInfo);
DWORD GetFileTypeAttributes(HKEY hkeyFT);
DWORD SetVerbAttributes(HKEY hkeyFT, LPSTR pszVerb, DWORD dwAttibutes);
DWORD GetVerbAttributes(HKEY hkeyFT, LPSTR pszVerb);
VOID FT_MergeDuplicates(HWND hwndLV);
LONG SaveFileTypeData(DWORD dwName, PFILETYPESDIALOGINFO pFTDInfo);
VOID ResizeCommandDlg(HWND hDialog, BOOL bFlag);
LONG RemoveAction(PFILETYPESDIALOGINFO pFTDInfo, HKEY hk, LPCSTR szKey, LPSTR szAction);
LONG RemoveFileType(PFILETYPESDIALOGINFO pFTDInfo);
LPSTR StripRStr(LPSTR pszFirst, LPCSTR pszSrch);
BOOL ExtToTypeNameAndId(LPSTR pszExt, LPSTR pszDesc, DWORD *dwName, LPSTR pszId, DWORD *dwId);
VOID StrRemoveChar(LPSTR pszSrc, LPSTR pszDest, char ch);
BOOL SetDefaultAction(PFILETYPESDIALOGINFO pFTDInfo);
BOOL IsDefaultAction(PFILETYPESDIALOGINFO pFTDInfo, LPSTR pszAction);
HKEY GetHkeyFT(LPSTR pszId);
BOOL ActionExeIsValid(HWND hDialog, BOOL bFlag);
BOOL ActionIsEntered(HWND hDialog, BOOL bFlag);
BOOL ValidExtension(HWND hDialog, PFILETYPESDIALOGINFO pFTDInfo);
VOID DisplayDocObjects(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog);
VOID DisplayOpensWithObjects(PFILETYPESDIALOGINFO pFTDInfo, HWND hDialog);
VOID OkToClose_NoCancel(HWND hDialog);

int  ParseIconLocation(LPSTR pszIconFile);

#endif
