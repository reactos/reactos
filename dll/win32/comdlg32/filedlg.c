/*
 * COMMDLG - File Open Dialogs Win95 look and feel
 *
 * Copyright 1999 Francois Boisvert
 * Copyright 1999, 2000 Juergen Schmied
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * FIXME: The whole concept of handling unicode is badly broken.
 *	many hook-messages expect a pointer to a
 *	OPENFILENAMEA or W structure. With the current architecture
 *	we would have to convert the beast at every call to a hook.
 *	we have to find a better solution but it would likely cause
 *	a complete rewrite after which we should handle the
 *	OPENFILENAME structure without any converting (jsch).
 *
 * FIXME: any hook gets a OPENFILENAMEA structure
 *
 * FIXME: CDN_FILEOK is wrong implemented, other CDN_ messages likely too
 *
 * FIXME: old style hook messages are not implemented (except FILEOKSTRING)
 *
 * FIXME: algorithm for selecting the initial directory is too simple
 *
 * FIXME: add to recent docs
 *
 * FIXME: flags not implemented: OFN_DONTADDTORECENT,
 * OFN_NODEREFERENCELINKS, OFN_NOREADONLYRETURN,
 * OFN_NOTESTFILECREATE, OFN_USEMONIKERS
 *
 * FIXME: lCustData for lpfnHook (WM_INITDIALOG)
 *
 *
 */

#include "cdlg.h"

#include <shellapi.h>

#define UNIMPLEMENTED_FLAGS \
(OFN_DONTADDTORECENT |\
OFN_NODEREFERENCELINKS | OFN_NOREADONLYRETURN |\
OFN_NOTESTFILECREATE /*| OFN_USEMONIKERS*/)

#define IsHooked(fodInfos) \
	((fodInfos->ofnInfos->Flags & OFN_ENABLEHOOK) && fodInfos->ofnInfos->lpfnHook)
/***********************************************************************
 * Data structure and global variables
 */
typedef struct SFolder
{
  int m_iImageIndex;    /* Index of picture in image list */
  HIMAGELIST hImgList;
  int m_iIndent;      /* Indentation index */
  LPITEMIDLIST pidlItem;  /* absolute pidl of the item */

} SFOLDER,*LPSFOLDER;

typedef struct tagLookInInfo
{
  int iMaxIndentation;
  UINT uSelectedItem;
} LookInInfos;


/***********************************************************************
 * Defines and global variables
 */

/* Draw item constant */
#define XTEXTOFFSET 3

/* AddItem flags*/
#define LISTEND -1

/* SearchItem methods */
#define SEARCH_PIDL 1
#define SEARCH_EXP  2
#define ITEM_NOTFOUND -1

/* Undefined windows message sent by CreateViewObject*/
#define WM_GETISHELLBROWSER  WM_USER+7

/* NOTE
 * Those macros exist in windowsx.h. However, you can't really use them since
 * they rely on the UNICODE defines and can't be used inside Wine itself.
 */

/* Combo box macros */
#define CBAddString(hwnd,str) \
    SendMessageW(hwnd, CB_ADDSTRING, 0, (LPARAM)(str));

#define CBInsertString(hwnd,str,pos) \
    SendMessageW(hwnd, CB_INSERTSTRING, (WPARAM)(pos), (LPARAM)(str));

#define CBDeleteString(hwnd,pos) \
    SendMessageW(hwnd, CB_DELETESTRING, (WPARAM)(pos), 0);

#define CBSetItemDataPtr(hwnd,iItemId,dataPtr) \
    SendMessageW(hwnd, CB_SETITEMDATA, (WPARAM)(iItemId), (LPARAM)(dataPtr));

#define CBGetItemDataPtr(hwnd,iItemId) \
    SendMessageW(hwnd, CB_GETITEMDATA, (WPARAM)(iItemId), 0)

#define CBGetLBText(hwnd,iItemId,str) \
    SendMessageW(hwnd, CB_GETLBTEXT, (WPARAM)(iItemId), (LPARAM)(str));

#define CBGetCurSel(hwnd) \
    SendMessageW(hwnd, CB_GETCURSEL, 0, 0);

#define CBSetCurSel(hwnd,pos) \
    SendMessageW(hwnd, CB_SETCURSEL, (WPARAM)(pos), 0);

#define CBGetCount(hwnd) \
    SendMessageW(hwnd, CB_GETCOUNT, 0, 0);
#define CBShowDropDown(hwnd,show) \
    SendMessageW(hwnd, CB_SHOWDROPDOWN, (WPARAM)(show), 0);
#define CBSetItemHeight(hwnd,index,height) \
    SendMessageW(hwnd, CB_SETITEMHEIGHT, (WPARAM)(index), (LPARAM)(height));

#define CBSetExtendedUI(hwnd,flag) \
    SendMessageW(hwnd, CB_SETEXTENDEDUI, (WPARAM)(flag), 0)

const char FileOpenDlgInfosStr[] = "FileOpenDlgInfos"; /* windows property description string */
static const char LookInInfosStr[] = "LookInInfos"; /* LOOKIN combo box property */
static SIZE MemDialogSize = { 0, 0}; /* keep size of the (resizable) dialog */

static const WCHAR LastVisitedMRUW[] =
    {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'E','x','p','l','o','r','e','r','\\','C','o','m','D','l','g','3','2','\\',
        'L','a','s','t','V','i','s','i','t','e','d','M','R','U',0};
static const WCHAR MRUListW[] = {'M','R','U','L','i','s','t',0};

/***********************************************************************
 * Prototypes
 */

/* Internal functions used by the dialog */
static LRESULT FILEDLG95_ResizeControls(HWND hwnd, WPARAM wParam, LPARAM lParam);
static LRESULT FILEDLG95_FillControls(HWND hwnd, WPARAM wParam, LPARAM lParam);
static LRESULT FILEDLG95_OnWMCommand(HWND hwnd, WPARAM wParam);
static LRESULT FILEDLG95_OnWMGetIShellBrowser(HWND hwnd);
static BOOL    FILEDLG95_OnOpen(HWND hwnd);
static LRESULT FILEDLG95_InitControls(HWND hwnd);
static void    FILEDLG95_Clean(HWND hwnd);

/* Functions used by the shell navigation */
static LRESULT FILEDLG95_SHELL_Init(HWND hwnd);
static BOOL    FILEDLG95_SHELL_UpFolder(HWND hwnd);
static BOOL    FILEDLG95_SHELL_ExecuteCommand(HWND hwnd, LPCSTR lpVerb);
static void    FILEDLG95_SHELL_Clean(HWND hwnd);
static BOOL    FILEDLG95_SHELL_BrowseToDesktop(HWND hwnd);

/* Functions used by the EDIT box */
static int FILEDLG95_FILENAME_GetFileNames (HWND hwnd, LPWSTR * lpstrFileList, UINT * sizeUsed);

/* Functions used by the filetype combo box */
static HRESULT FILEDLG95_FILETYPE_Init(HWND hwnd);
static BOOL    FILEDLG95_FILETYPE_OnCommand(HWND hwnd, WORD wNotifyCode);
static int     FILEDLG95_FILETYPE_SearchExt(HWND hwnd,LPCWSTR lpstrExt);
static void    FILEDLG95_FILETYPE_Clean(HWND hwnd);

/* Functions used by the Look In combo box */
static void    FILEDLG95_LOOKIN_Init(HWND hwndCombo);
static LRESULT FILEDLG95_LOOKIN_DrawItem(LPDRAWITEMSTRUCT pDIStruct);
static BOOL    FILEDLG95_LOOKIN_OnCommand(HWND hwnd, WORD wNotifyCode);
static int     FILEDLG95_LOOKIN_AddItem(HWND hwnd,LPITEMIDLIST pidl, int iInsertId);
static int     FILEDLG95_LOOKIN_SearchItem(HWND hwnd,WPARAM searchArg,int iSearchMethod);
static int     FILEDLG95_LOOKIN_InsertItemAfterParent(HWND hwnd,LPITEMIDLIST pidl);
static int     FILEDLG95_LOOKIN_RemoveMostExpandedItem(HWND hwnd);
       int     FILEDLG95_LOOKIN_SelectItem(HWND hwnd,LPITEMIDLIST pidl);
static void    FILEDLG95_LOOKIN_Clean(HWND hwnd);

/* Functions for dealing with the most-recently-used registry keys */
static void FILEDLG95_MRU_load_filename(LPWSTR stored_path);
static WCHAR FILEDLG95_MRU_get_slot(LPCWSTR module_name, LPWSTR stored_path, PHKEY hkey_ret);
static void FILEDLG95_MRU_save_filename(LPCWSTR filename);

/* Miscellaneous tool functions */
static HRESULT GetName(LPSHELLFOLDER lpsf, LPITEMIDLIST pidl,DWORD dwFlags,LPWSTR lpstrFileName);
IShellFolder* GetShellFolderFromPidl(LPITEMIDLIST pidlAbs);
LPITEMIDLIST  GetParentPidl(LPITEMIDLIST pidl);
static LPITEMIDLIST GetPidlFromName(IShellFolder *psf,LPWSTR lpcstrFileName);
static BOOL IsPidlFolder (LPSHELLFOLDER psf, LPCITEMIDLIST pidl);
static UINT GetNumSelected( IDataObject *doSelected );

/* Shell memory allocation */
static void *MemAlloc(UINT size);
static void MemFree(void *mem);

static INT_PTR CALLBACK FileOpenDlgProc95(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static INT_PTR FILEDLG95_HandleCustomDialogMessages(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL FILEDLG95_OnOpenMultipleFiles(HWND hwnd, LPWSTR lpstrFileList, UINT nFileCount, UINT sizeUsed);
static BOOL BrowseSelectedFolder(HWND hwnd);

/***********************************************************************
 *      GetFileName95
 *
 * Creates an Open common dialog box that lets the user select
 * the drive, directory, and the name of a file or set of files to open.
 *
 * IN  : The FileOpenDlgInfos structure associated with the dialog
 * OUT : TRUE on success
 *       FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 */
static BOOL GetFileName95(FileOpenDlgInfos *fodInfos)
{

    LRESULT lRes;
    LPCVOID origTemplate;
    DWORD dwSize;
    LPDLGTEMPLATEW template;
    HRSRC hRes;
    HANDLE hDlgTmpl = 0;
    HRESULT hr;

    /* test for missing functionality */
    if (fodInfos->ofnInfos->Flags & UNIMPLEMENTED_FLAGS)
    {
      FIXME("Flags 0x%08x not yet implemented\n",
         fodInfos->ofnInfos->Flags & UNIMPLEMENTED_FLAGS);
    }

    /* Create the dialog from a template */

    if(!(hRes = FindResourceW(COMDLG32_hInstance,MAKEINTRESOURCEW(NEWFILEOPENORD),(LPCWSTR)RT_DIALOG)))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
        return FALSE;
    }
    if (!(dwSize = SizeofResource(COMDLG32_hInstance, hRes)) ||
        !(hDlgTmpl = LoadResource(COMDLG32_hInstance, hRes)) ||
        !(origTemplate = LockResource(hDlgTmpl)))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
        return FALSE;
    }
    if (!(template = HeapAlloc(GetProcessHeap(), 0, dwSize)))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_MEMALLOCFAILURE);
        return FALSE;
    }
    memcpy(template, origTemplate, dwSize);

    /* msdn: explorer style dialogs permit sizing by default.
     * The OFN_ENABLESIZING flag is only needed when a hook or
     * custom tmeplate is provided */
    if( (fodInfos->ofnInfos->Flags & OFN_EXPLORER) &&
            !(fodInfos->ofnInfos->Flags & ( OFN_ENABLEHOOK | OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE)))
        fodInfos->ofnInfos->Flags |= OFN_ENABLESIZING;

    if (fodInfos->ofnInfos->Flags & OFN_ENABLESIZING)
    {
        template->style |= WS_SIZEBOX;
        fodInfos->sizedlg.cx = fodInfos->sizedlg.cy = 0;
        fodInfos->initial_size.x = fodInfos->initial_size.y = 0;
    }
    else
        template->style &= ~WS_SIZEBOX;


    /* old style hook messages */
    if (IsHooked(fodInfos))
    {
      fodInfos->HookMsg.fileokstring = RegisterWindowMessageW(FILEOKSTRINGW);
      fodInfos->HookMsg.lbselchstring = RegisterWindowMessageW(LBSELCHSTRINGW);
      fodInfos->HookMsg.helpmsgstring = RegisterWindowMessageW(HELPMSGSTRINGW);
      fodInfos->HookMsg.sharevistring = RegisterWindowMessageW(SHAREVISTRINGW);
    }

    /* Some shell namespace extensions depend on COM being initialized. */
    hr = OleInitialize(NULL);

    if (fodInfos->unicode)
      lRes = DialogBoxIndirectParamW(COMDLG32_hInstance,
                                     template,
                                     fodInfos->ofnInfos->hwndOwner,
                                     FileOpenDlgProc95,
                                     (LPARAM) fodInfos);
    else
      lRes = DialogBoxIndirectParamA(COMDLG32_hInstance,
                                     template,
                                     fodInfos->ofnInfos->hwndOwner,
                                     FileOpenDlgProc95,
                                     (LPARAM) fodInfos);
    if (SUCCEEDED(hr)) 
        OleUninitialize();

    HeapFree(GetProcessHeap(), 0, template);

    /* Unable to create the dialog */
    if( lRes == -1)
        return FALSE;

    return lRes;
}

/***********************************************************************
 *      GetFileDialog95A
 *
 * Call GetFileName95 with this structure and clean the memory.
 *
 * IN  : The OPENFILENAMEA initialisation structure passed to
 *       GetOpenFileNameA win api function (see filedlg.c)
 */
static BOOL GetFileDialog95A(LPOPENFILENAMEA ofn,UINT iDlgType)
{
  BOOL ret;
  FileOpenDlgInfos fodInfos;
  LPSTR lpstrSavDir = NULL;
  LPWSTR title = NULL;
  LPWSTR defext = NULL;
  LPWSTR filter = NULL;
  LPWSTR customfilter = NULL;
  INITCOMMONCONTROLSEX icc;

  /* Initialize ComboBoxEx32 */
  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_USEREX_CLASSES;
  InitCommonControlsEx(&icc);

  /* Initialize CommDlgExtendedError() */
  COMDLG32_SetCommDlgExtendedError(0);

  /* Initialize FileOpenDlgInfos structure */
  ZeroMemory(&fodInfos, sizeof(FileOpenDlgInfos));

  /* Pass in the original ofn */
  fodInfos.ofnInfos = (LPOPENFILENAMEW)ofn;

  /* save current directory */
  if (ofn->Flags & OFN_NOCHANGEDIR)
  {
     lpstrSavDir = MemAlloc(MAX_PATH);
     GetCurrentDirectoryA(MAX_PATH, lpstrSavDir);
  }

  fodInfos.unicode = FALSE;

  /* convert all the input strings to unicode */
  if(ofn->lpstrInitialDir)
  {
    DWORD len = MultiByteToWideChar( CP_ACP, 0, ofn->lpstrInitialDir, -1, NULL, 0 );
    fodInfos.initdir = MemAlloc((len+1)*sizeof(WCHAR));
    MultiByteToWideChar( CP_ACP, 0, ofn->lpstrInitialDir, -1, fodInfos.initdir, len);
  }
  else
    fodInfos.initdir = NULL;

  if(ofn->lpstrFile)
  {
    fodInfos.filename = MemAlloc(ofn->nMaxFile*sizeof(WCHAR));
    MultiByteToWideChar( CP_ACP, 0, ofn->lpstrFile, -1, fodInfos.filename, ofn->nMaxFile);
  }
  else
    fodInfos.filename = NULL;

  if(ofn->lpstrDefExt)
  {
    DWORD len = MultiByteToWideChar( CP_ACP, 0, ofn->lpstrDefExt, -1, NULL, 0 );
    defext = MemAlloc((len+1)*sizeof(WCHAR));
    MultiByteToWideChar( CP_ACP, 0, ofn->lpstrDefExt, -1, defext, len);
  }
  fodInfos.defext = defext;

  if(ofn->lpstrTitle)
  {
    DWORD len = MultiByteToWideChar( CP_ACP, 0, ofn->lpstrTitle, -1, NULL, 0 );
    title = MemAlloc((len+1)*sizeof(WCHAR));
    MultiByteToWideChar( CP_ACP, 0, ofn->lpstrTitle, -1, title, len);
  }
  fodInfos.title = title;

  if (ofn->lpstrFilter)
  {
    LPCSTR s;
    int n, len;

    /* filter is a list...  title\0ext\0......\0\0 */
    s = ofn->lpstrFilter;
    while (*s) s = s+strlen(s)+1;
    s++;
    n = s - ofn->lpstrFilter;
    len = MultiByteToWideChar( CP_ACP, 0, ofn->lpstrFilter, n, NULL, 0 );
    filter = MemAlloc(len*sizeof(WCHAR));
    MultiByteToWideChar( CP_ACP, 0, ofn->lpstrFilter, n, filter, len );
  }
  fodInfos.filter = filter;

  /* convert lpstrCustomFilter */
  if (ofn->lpstrCustomFilter)
  {
    LPCSTR s;
    int n, len;

    /* customfilter contains a pair of strings...  title\0ext\0 */
    s = ofn->lpstrCustomFilter;
    if (*s) s = s+strlen(s)+1;
    if (*s) s = s+strlen(s)+1;
    n = s - ofn->lpstrCustomFilter;
    len = MultiByteToWideChar( CP_ACP, 0, ofn->lpstrCustomFilter, n, NULL, 0 );
    customfilter = MemAlloc(len*sizeof(WCHAR));
    MultiByteToWideChar( CP_ACP, 0, ofn->lpstrCustomFilter, n, customfilter, len );
  }
  fodInfos.customfilter = customfilter;

  /* Initialize the dialog property */
  fodInfos.DlgInfos.dwDlgProp = 0;
  fodInfos.DlgInfos.hwndCustomDlg = NULL;

  switch(iDlgType)
  {
    case OPEN_DIALOG :
      ret = GetFileName95(&fodInfos);
      break;
    case SAVE_DIALOG :
      fodInfos.DlgInfos.dwDlgProp |= FODPROP_SAVEDLG;
      ret = GetFileName95(&fodInfos);
      break;
    default :
      ret = FALSE;
  }

  if (lpstrSavDir)
  {
      SetCurrentDirectoryA(lpstrSavDir);
      MemFree(lpstrSavDir);
  }

  MemFree(title);
  MemFree(defext);
  MemFree(filter);
  MemFree(customfilter);
  MemFree(fodInfos.initdir);
  MemFree(fodInfos.filename);

  TRACE("selected file: %s\n",ofn->lpstrFile);

  return ret;
}

/***********************************************************************
 *      GetFileDialog95W
 *
 * Copy the OPENFILENAMEW structure in a FileOpenDlgInfos structure.
 * Call GetFileName95 with this structure and clean the memory.
 *
 */
static BOOL GetFileDialog95W(LPOPENFILENAMEW ofn,UINT iDlgType)
{
  BOOL ret;
  FileOpenDlgInfos fodInfos;
  LPWSTR lpstrSavDir = NULL;
  INITCOMMONCONTROLSEX icc;

  /* Initialize ComboBoxEx32 */
  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_USEREX_CLASSES;
  InitCommonControlsEx(&icc);

  /* Initialize CommDlgExtendedError() */
  COMDLG32_SetCommDlgExtendedError(0);

  /* Initialize FileOpenDlgInfos structure */
  ZeroMemory(&fodInfos, sizeof(FileOpenDlgInfos));

  /*  Pass in the original ofn */
  fodInfos.ofnInfos = ofn;

  fodInfos.title = ofn->lpstrTitle;
  fodInfos.defext = ofn->lpstrDefExt;
  fodInfos.filter = ofn->lpstrFilter;
  fodInfos.customfilter = ofn->lpstrCustomFilter;

  /* convert string arguments, save others */
  if(ofn->lpstrFile)
  {
    fodInfos.filename = MemAlloc(ofn->nMaxFile*sizeof(WCHAR));
    lstrcpynW(fodInfos.filename,ofn->lpstrFile,ofn->nMaxFile);
  }
  else
    fodInfos.filename = NULL;

  if(ofn->lpstrInitialDir)
  {
    /* fodInfos.initdir = strdupW(ofn->lpstrInitialDir); */
    DWORD len = lstrlenW(ofn->lpstrInitialDir)+1;
    fodInfos.initdir = MemAlloc(len*sizeof(WCHAR));
    memcpy(fodInfos.initdir,ofn->lpstrInitialDir,len*sizeof(WCHAR));
  }
  else
    fodInfos.initdir = NULL;

  /* save current directory */
  if (ofn->Flags & OFN_NOCHANGEDIR)
  {
     lpstrSavDir = MemAlloc(MAX_PATH*sizeof(WCHAR));
     GetCurrentDirectoryW(MAX_PATH, lpstrSavDir);
  }

  fodInfos.unicode = TRUE;

  switch(iDlgType)
  {
  case OPEN_DIALOG :
      ret = GetFileName95(&fodInfos);
      break;
  case SAVE_DIALOG :
      fodInfos.DlgInfos.dwDlgProp |= FODPROP_SAVEDLG;
      ret = GetFileName95(&fodInfos);
      break;
  default :
      ret = FALSE;
  }

  if (lpstrSavDir)
  {
      SetCurrentDirectoryW(lpstrSavDir);
      MemFree(lpstrSavDir);
  }

  /* restore saved IN arguments and convert OUT arguments back */
  MemFree(fodInfos.filename);
  MemFree(fodInfos.initdir);
  return ret;
}

/******************************************************************************
 * COMDLG32_GetDisplayNameOf [internal]
 *
 * Helper function to get the display name for a pidl.
 */
static BOOL COMDLG32_GetDisplayNameOf(LPCITEMIDLIST pidl, LPWSTR pwszPath) {
    LPSHELLFOLDER psfDesktop;
    STRRET strret;
        
    if (FAILED(SHGetDesktopFolder(&psfDesktop)))
        return FALSE;

    if (FAILED(IShellFolder_GetDisplayNameOf(psfDesktop, pidl, SHGDN_FORPARSING, &strret))) {
        IShellFolder_Release(psfDesktop);
        return FALSE;
    }

    IShellFolder_Release(psfDesktop);
    return SUCCEEDED(StrRetToBufW(&strret, pidl, pwszPath, MAX_PATH));
}

/******************************************************************************
 * COMDLG32_GetCanonicalPath [internal]
 *
 * Helper function to get the canonical path.
 */
void COMDLG32_GetCanonicalPath(PCIDLIST_ABSOLUTE pidlAbsCurrent,
                               LPWSTR lpstrFile, LPWSTR lpstrPathAndFile)
{
  WCHAR lpstrTemp[MAX_PATH];

  /* Get the current directory name */
  if (!COMDLG32_GetDisplayNameOf(pidlAbsCurrent, lpstrPathAndFile))
  {
    /* last fallback */
    GetCurrentDirectoryW(MAX_PATH, lpstrPathAndFile);
  }
  PathAddBackslashW(lpstrPathAndFile);

  TRACE("current directory=%s\n", debugstr_w(lpstrPathAndFile));

  /* if the user specified a fully qualified path use it */
  if(PathIsRelativeW(lpstrFile))
  {
    lstrcatW(lpstrPathAndFile, lpstrFile);
  }
  else
  {
    /* does the path have a drive letter? */
    if (PathGetDriveNumberW(lpstrFile) == -1)
      lstrcpyW(lpstrPathAndFile+2, lpstrFile);
    else
      lstrcpyW(lpstrPathAndFile, lpstrFile);
  }

  /* resolve "." and ".." */
  PathCanonicalizeW(lpstrTemp, lpstrPathAndFile );
  lstrcpyW(lpstrPathAndFile, lpstrTemp);
  TRACE("canon=%s\n", debugstr_w(lpstrPathAndFile));
}

/***********************************************************************
 *      COMDLG32_SplitFileNames [internal]
 *
 * Creates a delimited list of filenames.
 */
int COMDLG32_SplitFileNames(LPWSTR lpstrEdit, UINT nStrLen, LPWSTR *lpstrFileList, UINT *sizeUsed)
{
	UINT nStrCharCount = 0;	/* index in src buffer */
	UINT nFileIndex = 0;	/* index in dest buffer */
	UINT nFileCount = 0;	/* number of files */

	/* we might get single filename without any '"',
	 * so we need nStrLen + terminating \0 + end-of-list \0 */
	*lpstrFileList = MemAlloc( (nStrLen+2)*sizeof(WCHAR) );
	*sizeUsed = 0;

	/* build delimited file list from filenames */
	while ( nStrCharCount <= nStrLen )
	{
	  if ( lpstrEdit[nStrCharCount]=='"' )
	  {
	    nStrCharCount++;
	    while ((lpstrEdit[nStrCharCount]!='"') && (nStrCharCount <= nStrLen))
	    {
	      (*lpstrFileList)[nFileIndex++] = lpstrEdit[nStrCharCount];
	      nStrCharCount++;
	    }
	    (*lpstrFileList)[nFileIndex++] = 0;
	    nFileCount++;
	  }
	  nStrCharCount++;
	}

	/* single, unquoted string */
	if ((nStrLen > 0) && (nFileIndex == 0) )
	{
	  lstrcpyW(*lpstrFileList, lpstrEdit);
	  nFileIndex = lstrlenW(lpstrEdit) + 1;
	  nFileCount = 1;
	}

        /* trailing \0 */
        (*lpstrFileList)[nFileIndex++] = '\0';

        *sizeUsed = nFileIndex;
	return nFileCount;
}

/***********************************************************************
 *      ArrangeCtrlPositions [internal]
 *
 * NOTE: Make sure to add testcases for any changes made here.
 */
static void ArrangeCtrlPositions(HWND hwndChildDlg, HWND hwndParentDlg, BOOL hide_help)
{
    HWND hwndChild, hwndStc32;
    RECT rectParent, rectChild, rectStc32;
    INT help_fixup = 0;
    int chgx, chgy;

    /* Take into account if open as read only checkbox and help button
     * are hidden
     */
     if (hide_help)
     {
         RECT rectHelp, rectCancel;
         GetWindowRect(GetDlgItem(hwndParentDlg, pshHelp), &rectHelp);
         GetWindowRect(GetDlgItem(hwndParentDlg, IDCANCEL), &rectCancel);
         /* subtract the height of the help button plus the space between
          * the help button and the cancel button to the height of the dialog
          */
          help_fixup = rectHelp.bottom - rectCancel.bottom;
    }

    /*
      There are two possibilities to add components to the default file dialog box.

      By default, all the new components are added below the standard dialog box (the else case).

      However, if there is a static text component with the stc32 id, a special case happens.
      The x and y coordinates of stc32 indicate the top left corner where to place the standard file dialog box
      in the window and the cx and cy indicate how to size the window.
      Moreover, if the new component's coordinates are on the left of the stc32 , it is placed on the left 
      of the standard file dialog box. If they are above the stc32 component, it is placed above and so on....
      
     */

    GetClientRect(hwndParentDlg, &rectParent);

    /* when arranging controls we have to use fixed parent size */
    rectParent.bottom -= help_fixup;

    hwndStc32 = GetDlgItem(hwndChildDlg, stc32);
    if (hwndStc32)
    {
        GetWindowRect(hwndStc32, &rectStc32);
        MapWindowPoints(0, hwndChildDlg, (LPPOINT)&rectStc32, 2);

        /* set the size of the stc32 control according to the size of
         * client area of the parent dialog
         */
        SetWindowPos(hwndStc32, 0,
                     0, 0,
                     rectParent.right, rectParent.bottom,
                     SWP_NOMOVE | SWP_NOZORDER);
    }
    else
        SetRectEmpty(&rectStc32);

    /* this part moves controls of the child dialog */
    hwndChild = GetWindow(hwndChildDlg, GW_CHILD);
    while (hwndChild)
    {
        if (hwndChild != hwndStc32)
        {
            GetWindowRect(hwndChild, &rectChild);
            MapWindowPoints(0, hwndChildDlg, (LPPOINT)&rectChild, 2);

            /* move only if stc32 exist */
            if (hwndStc32 && rectChild.left > rectStc32.right)
            {
                /* move to the right of visible controls of the parent dialog */
                rectChild.left += rectParent.right;
                rectChild.left -= rectStc32.right;
            }
            /* move even if stc32 doesn't exist */
            if (rectChild.top >= rectStc32.bottom)
            {
                /* move below visible controls of the parent dialog */
                rectChild.top += rectParent.bottom;
                rectChild.top -= rectStc32.bottom - rectStc32.top;
            }

            SetWindowPos(hwndChild, 0, rectChild.left, rectChild.top,
                         0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        hwndChild = GetWindow(hwndChild, GW_HWNDNEXT);
    }

    /* this part moves controls of the parent dialog */
    hwndChild = GetWindow(hwndParentDlg, GW_CHILD);
    while (hwndChild)
    {
        if (hwndChild != hwndChildDlg)
        {
            GetWindowRect(hwndChild, &rectChild);
            MapWindowPoints(0, hwndParentDlg, (LPPOINT)&rectChild, 2);

            /* left,top of stc32 marks the position of controls
             * from the parent dialog
             */
            rectChild.left += rectStc32.left;
            rectChild.top += rectStc32.top;

            SetWindowPos(hwndChild, 0, rectChild.left, rectChild.top,
                         0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        hwndChild = GetWindow(hwndChild, GW_HWNDNEXT);
    }

    /* calculate the size of the resulting dialog */

    /* here we have to use original parent size */
    GetClientRect(hwndParentDlg, &rectParent);
    GetClientRect(hwndChildDlg, &rectChild);
    TRACE( "parent %s child %s stc32 %s\n", wine_dbgstr_rect( &rectParent),
            wine_dbgstr_rect( &rectChild), wine_dbgstr_rect( &rectStc32));

    if (hwndStc32)
    {
        /* width */
        if (rectParent.right > rectStc32.right - rectStc32.left)
            chgx = rectChild.right - ( rectStc32.right - rectStc32.left);
        else
            chgx = rectChild.right - rectParent.right;
        /* height */
        if (rectParent.bottom > rectStc32.bottom - rectStc32.top)
            chgy = rectChild.bottom - ( rectStc32.bottom - rectStc32.top) - help_fixup;
        else
            /* Unconditionally set new dialog
             * height to that of the child
             */
            chgy = rectChild.bottom - rectParent.bottom;
    }
    else
    {
        chgx = 0;
        chgy = rectChild.bottom - help_fixup;
    }
    /* set the size of the parent dialog */
    GetWindowRect(hwndParentDlg, &rectParent);
    SetWindowPos(hwndParentDlg, 0,
                 0, 0,
                 rectParent.right - rectParent.left + chgx,
                 rectParent.bottom - rectParent.top + chgy,
                 SWP_NOMOVE | SWP_NOZORDER);
}

static INT_PTR CALLBACK FileOpenDlgProcUserTemplate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg) {
    case WM_INITDIALOG:
        return TRUE;
    }
    return FALSE;
}

static HWND CreateTemplateDialog(FileOpenDlgInfos *fodInfos, HWND hwnd)
{
    LPCVOID template;
    HRSRC hRes;
    HANDLE hDlgTmpl = 0;
    HWND hChildDlg = 0;

    TRACE("\n");

    /*
     * If OFN_ENABLETEMPLATEHANDLE is specified, the OPENFILENAME
     * structure's hInstance parameter is not a HINSTANCE, but
     * instead a pointer to a template resource to use.
     */
    if (fodInfos->ofnInfos->Flags & (OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE))
    {
      HINSTANCE hinst;
      if (fodInfos->ofnInfos->Flags  & OFN_ENABLETEMPLATEHANDLE)
      {
        hinst = COMDLG32_hInstance;
        if( !(template = LockResource( fodInfos->ofnInfos->hInstance)))
        {
          COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
          return NULL;
        }
      }
      else
      {
        hinst = fodInfos->ofnInfos->hInstance;
        if(fodInfos->unicode)
        {
            LPOPENFILENAMEW ofn = fodInfos->ofnInfos;
            hRes = FindResourceW( hinst, ofn->lpTemplateName, (LPWSTR)RT_DIALOG);
        }
        else
        {
            LPOPENFILENAMEA ofn = (LPOPENFILENAMEA)fodInfos->ofnInfos;
            hRes = FindResourceA( hinst, ofn->lpTemplateName, (LPSTR)RT_DIALOG);
        }
        if (!hRes)
        {
          COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
          return NULL;
        }
        if (!(hDlgTmpl = LoadResource( hinst, hRes )) ||
            !(template = LockResource( hDlgTmpl )))
        {
          COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
          return NULL;
    	}
      }
      if (fodInfos->unicode)
          hChildDlg = CreateDialogIndirectParamW(hinst, template, hwnd,
              IsHooked(fodInfos) ? (DLGPROC)fodInfos->ofnInfos->lpfnHook : FileOpenDlgProcUserTemplate,
              (LPARAM)fodInfos->ofnInfos);
      else
          hChildDlg = CreateDialogIndirectParamA(hinst, template, hwnd,
              IsHooked(fodInfos) ? (DLGPROC)fodInfos->ofnInfos->lpfnHook : FileOpenDlgProcUserTemplate,
              (LPARAM)fodInfos->ofnInfos);
      return hChildDlg;
    }
    else if( IsHooked(fodInfos))
    {
      RECT rectHwnd;
      struct  {
         DLGTEMPLATE tmplate;
         WORD menu,class,title;
         } temp;
      GetClientRect(hwnd,&rectHwnd);
      temp.tmplate.style = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | DS_CONTROL | DS_3DLOOK;
      temp.tmplate.dwExtendedStyle = 0;
      temp.tmplate.cdit = 0;
      temp.tmplate.x = 0;
      temp.tmplate.y = 0;
      temp.tmplate.cx = 0;
      temp.tmplate.cy = 0;
      temp.menu = temp.class = temp.title = 0;

      hChildDlg = CreateDialogIndirectParamA(COMDLG32_hInstance, &temp.tmplate,
                  hwnd, (DLGPROC)fodInfos->ofnInfos->lpfnHook, (LPARAM)fodInfos->ofnInfos);

      return hChildDlg;
    }
    return NULL;
}

/***********************************************************************
*          SendCustomDlgNotificationMessage
*
* Send CustomDialogNotification (CDN_FIRST -- CDN_LAST) message to the custom template dialog
*/

LRESULT SendCustomDlgNotificationMessage(HWND hwndParentDlg, UINT uCode)
{
    LRESULT hook_result = 0;
    FileOpenDlgInfos *fodInfos = GetPropA(hwndParentDlg,FileOpenDlgInfosStr);

    TRACE("%p 0x%04x\n",hwndParentDlg, uCode);

    if(!fodInfos) return 0;

    if(fodInfos->DlgInfos.hwndCustomDlg)
    {
	TRACE("CALL NOTIFY for %x\n", uCode);
        if(fodInfos->unicode)
        {
            OFNOTIFYW ofnNotify;
            ofnNotify.hdr.hwndFrom=hwndParentDlg;
            ofnNotify.hdr.idFrom=0;
            ofnNotify.hdr.code = uCode;
            ofnNotify.lpOFN = fodInfos->ofnInfos;
            ofnNotify.pszFile = NULL;
            hook_result = SendMessageW(fodInfos->DlgInfos.hwndCustomDlg,WM_NOTIFY,0,(LPARAM)&ofnNotify);
        }
        else
        {
            OFNOTIFYA ofnNotify;
            ofnNotify.hdr.hwndFrom=hwndParentDlg;
            ofnNotify.hdr.idFrom=0;
            ofnNotify.hdr.code = uCode;
            ofnNotify.lpOFN = (LPOPENFILENAMEA)fodInfos->ofnInfos;
            ofnNotify.pszFile = NULL;
            hook_result = SendMessageA(fodInfos->DlgInfos.hwndCustomDlg,WM_NOTIFY,0,(LPARAM)&ofnNotify);
        }
	TRACE("RET NOTIFY\n");
    }
    TRACE("Retval: 0x%08lx\n", hook_result);
    return hook_result;
}

static INT_PTR FILEDLG95_Handle_GetFilePath(HWND hwnd, DWORD size, LPVOID result)
{
    UINT len, total;
    WCHAR *p, *buffer;
    FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);

    TRACE("CDM_GETFILEPATH:\n");

    if ( ! (fodInfos->ofnInfos->Flags & OFN_EXPLORER ) )
        return -1;

    /* get path and filenames */
    len = SendMessageW( fodInfos->DlgInfos.hwndFileName, WM_GETTEXTLENGTH, 0, 0 );
    buffer = HeapAlloc( GetProcessHeap(), 0, (len + 2 + MAX_PATH) * sizeof(WCHAR) );
    COMDLG32_GetDisplayNameOf( fodInfos->ShellInfos.pidlAbsCurrent, buffer );
    if (len)
    {
        p = buffer + strlenW(buffer);
        *p++ = '\\';
        SendMessageW( fodInfos->DlgInfos.hwndFileName, WM_GETTEXT, len + 1, (LPARAM)p );
    }
    if (fodInfos->unicode)
    {
        total = strlenW( buffer) + 1;
        if (result) lstrcpynW( result, buffer, size );
        TRACE( "CDM_GETFILEPATH: returning %u %s\n", total, debugstr_w(result));
    }
    else
    {
        total = WideCharToMultiByte( CP_ACP, 0, buffer, -1, NULL, 0, NULL, NULL );
        if (total <= size) WideCharToMultiByte( CP_ACP, 0, buffer, -1, result, size, NULL, NULL );
        TRACE( "CDM_GETFILEPATH: returning %u %s\n", total, debugstr_a(result));
    }
    HeapFree( GetProcessHeap(), 0, buffer );
    return total;
}

/***********************************************************************
*         FILEDLG95_HandleCustomDialogMessages
*
* Handle Custom Dialog Messages (CDM_FIRST -- CDM_LAST) messages
*/
static INT_PTR FILEDLG95_HandleCustomDialogMessages(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);
    WCHAR lpstrPath[MAX_PATH];
    INT_PTR retval;

    if(!fodInfos) return FALSE;

    switch(uMsg)
    {
        case CDM_GETFILEPATH:
            retval = FILEDLG95_Handle_GetFilePath(hwnd, (UINT)wParam, (LPVOID)lParam);
            break;

        case CDM_GETFOLDERPATH:
            TRACE("CDM_GETFOLDERPATH:\n");
            COMDLG32_GetDisplayNameOf(fodInfos->ShellInfos.pidlAbsCurrent, lpstrPath);
            if (lParam) 
            {
                if (fodInfos->unicode)
                    lstrcpynW((LPWSTR)lParam, lpstrPath, (int)wParam);
                else
                    WideCharToMultiByte(CP_ACP, 0, lpstrPath, -1, 
                                        (LPSTR)lParam, (int)wParam, NULL, NULL);
            }        
            retval = lstrlenW(lpstrPath) + 1;
            break;

        case CDM_GETFOLDERIDLIST:
            retval = COMDLG32_PIDL_ILGetSize(fodInfos->ShellInfos.pidlAbsCurrent);
            if (retval <= wParam)
                memcpy((void*)lParam, fodInfos->ShellInfos.pidlAbsCurrent, retval);
            break;

        case CDM_GETSPEC:
            TRACE("CDM_GETSPEC:\n");
            retval = SendMessageW(fodInfos->DlgInfos.hwndFileName, WM_GETTEXTLENGTH, 0, 0) + 1;
            if (lParam)
            {
                if (fodInfos->unicode)
                    SendMessageW(fodInfos->DlgInfos.hwndFileName, WM_GETTEXT, wParam, lParam);
                else
                    SendMessageA(fodInfos->DlgInfos.hwndFileName, WM_GETTEXT, wParam, lParam);
            }
            break;

        case CDM_SETCONTROLTEXT:
            TRACE("CDM_SETCONTROLTEXT:\n");
	    if ( lParam )
            {
                if( fodInfos->unicode )
	            SetDlgItemTextW( hwnd, (UINT) wParam, (LPWSTR) lParam );
                else
	            SetDlgItemTextA( hwnd, (UINT) wParam, (LPSTR) lParam );
            }
            retval = TRUE;
            break;

        case CDM_HIDECONTROL:
            /* MSDN states that it should fail for not OFN_EXPLORER case */
            if (fodInfos->ofnInfos->Flags & OFN_EXPLORER)
            {
                HWND control = GetDlgItem( hwnd, wParam );
                if (control) ShowWindow( control, SW_HIDE );
                retval = TRUE;
            }
            else retval = FALSE;
            break;

        default:
            if (uMsg >= CDM_FIRST && uMsg <= CDM_LAST)
                FIXME("message CDM_FIRST+%04x not implemented\n", uMsg - CDM_FIRST);
            return FALSE;
    }
    SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, retval);
    return TRUE;
}

/***********************************************************************
 *          FILEDLG95_OnWMGetMMI
 *
 * WM_GETMINMAXINFO message handler for resizable dialogs
 */
static LRESULT FILEDLG95_OnWMGetMMI( HWND hwnd, LPMINMAXINFO mmiptr)
{
    FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);
    if( !(fodInfos->ofnInfos->Flags & OFN_ENABLESIZING)) return FALSE;
    if( fodInfos->initial_size.x || fodInfos->initial_size.y)
    {
        mmiptr->ptMinTrackSize = fodInfos->initial_size;
    }
    return TRUE;
}

/***********************************************************************
 *          FILEDLG95_OnWMSize
 *
 * WM_SIZE message handler, resize the dialog. Re-arrange controls.
 *
 * FIXME: this could be made more elaborate. Now use a simple scheme
 * where the file view is enlarged and the controls are either moved
 * vertically or horizontally to get out of the way. Only the "grip"
 * is moved in both directions to stay in the corner.
 */
static LRESULT FILEDLG95_OnWMSize(HWND hwnd, WPARAM wParam)
{
    RECT rc, rcview;
    int chgx, chgy;
    HWND ctrl;
    HDWP hdwp;
    FileOpenDlgInfos *fodInfos;

    if( wParam != SIZE_RESTORED) return FALSE;
    fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);
    if( !(fodInfos->ofnInfos->Flags & OFN_ENABLESIZING)) return FALSE;
    /* get the new dialog rectangle */
    GetWindowRect( hwnd, &rc);
    TRACE("Size from %d,%d to %d,%d\n", fodInfos->sizedlg.cx, fodInfos->sizedlg.cy,
            rc.right -rc.left, rc.bottom -rc.top);
    /* not initialized yet */
    if( (fodInfos->sizedlg.cx == 0 && fodInfos->sizedlg.cy == 0) ||
        ((fodInfos->sizedlg.cx == rc.right -rc.left) && /* no change */
             (fodInfos->sizedlg.cy == rc.bottom -rc.top)))
        return FALSE;
    chgx = rc.right - rc.left - fodInfos->sizedlg.cx;
    chgy = rc.bottom - rc.top - fodInfos->sizedlg.cy;
    fodInfos->sizedlg.cx = rc.right - rc.left;
    fodInfos->sizedlg.cy = rc.bottom - rc.top;
    /* change the size of the view window */
    GetWindowRect( fodInfos->ShellInfos.hwndView, &rcview);
    MapWindowPoints( NULL, hwnd, (LPPOINT) &rcview, 2);
    hdwp = BeginDeferWindowPos( 10);
    DeferWindowPos( hdwp, fodInfos->ShellInfos.hwndView, NULL, 0, 0,
            rcview.right - rcview.left + chgx,
            rcview.bottom - rcview.top + chgy,
            SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    /* change position and sizes of the controls */
    for( ctrl = GetWindow( hwnd, GW_CHILD); ctrl ; ctrl = GetWindow( ctrl, GW_HWNDNEXT))
    {
        int ctrlid = GetDlgCtrlID( ctrl);
        GetWindowRect( ctrl, &rc);
        MapWindowPoints( NULL, hwnd, (LPPOINT) &rc, 2);
        if( ctrl == fodInfos->DlgInfos.hwndGrip)
        {
            DeferWindowPos( hdwp, ctrl, NULL, rc.left + chgx, rc.top + chgy,
                    0, 0,
                    SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
        }
        else if( rc.top > rcview.bottom)
        {
            /* if it was below the shell view
             * move to bottom */
            switch( ctrlid)
            {
                /* file name (edit or comboboxex) and file types combo change also width */
                case edt1:
                case cmb13:
                case cmb1:
                    DeferWindowPos( hdwp, ctrl, NULL, rc.left, rc.top + chgy,
                            rc.right - rc.left + chgx, rc.bottom - rc.top,
                            SWP_NOACTIVATE | SWP_NOZORDER);
                    break;
                    /* then these buttons must move out of the way */
                case IDOK:
                case IDCANCEL:
                case pshHelp:
                    DeferWindowPos( hdwp, ctrl, NULL, rc.left + chgx, rc.top + chgy,
                            0, 0,
                            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
                    break;
                default:
                DeferWindowPos( hdwp, ctrl, NULL, rc.left, rc.top + chgy,
                        0, 0,
                        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            }
        }
        else if( rc.left > rcview.right)
        {
            /* if it was to the right of the shell view
             * move to right */
            DeferWindowPos( hdwp, ctrl, NULL, rc.left + chgx, rc.top,
                    0, 0,
                    SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
        }
        else
            /* special cases */
        {
            switch( ctrlid)
            {
#if 0 /* this is Win2k, Win XP. Vista and Higher don't move/size these controls */
                case IDC_LOOKIN:
                    DeferWindowPos( hdwp, ctrl, NULL, 0, 0,
                            rc.right - rc.left + chgx, rc.bottom - rc.top,
                            SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
                    break;
                case IDC_TOOLBARSTATIC:
                case IDC_TOOLBAR:
                    DeferWindowPos( hdwp, ctrl, NULL, rc.left + chgx, rc.top,
                            0, 0,
                            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
                    break;
#endif
                /* not resized in windows. Since wine uses this invisible control
                 * to size the browser view it needs to be resized */
                case IDC_SHELLSTATIC:
                    DeferWindowPos( hdwp, ctrl, NULL, 0, 0,
                            rc.right - rc.left + chgx,
                            rc.bottom - rc.top + chgy,
                            SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
                    break;
            }
        }
    }
    if(fodInfos->DlgInfos.hwndCustomDlg &&
        (fodInfos->ofnInfos->Flags & (OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE)))
    {
        for( ctrl = GetWindow( fodInfos->DlgInfos.hwndCustomDlg, GW_CHILD);
                ctrl ; ctrl = GetWindow( ctrl, GW_HWNDNEXT))
        {
            GetWindowRect( ctrl, &rc);
            MapWindowPoints( NULL, hwnd, (LPPOINT) &rc, 2);
            if( rc.top > rcview.bottom)
            {
                /* if it was below the shell view
                 * move to bottom */
                DeferWindowPos( hdwp, ctrl, NULL, rc.left, rc.top + chgy,
                        rc.right - rc.left, rc.bottom - rc.top,
                        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            }
            else if( rc.left > rcview.right)
            {
                /* if it was to the right of the shell view
                 * move to right */
                DeferWindowPos( hdwp, ctrl, NULL, rc.left + chgx, rc.top,
                        rc.right - rc.left, rc.bottom - rc.top,
                        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            }
        }
        /* size the custom dialog at the end: some applications do some
         * control re-arranging at this point */
        GetClientRect(hwnd, &rc);
        DeferWindowPos( hdwp,fodInfos->DlgInfos.hwndCustomDlg, NULL,
            0, 0, rc.right, rc.bottom, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    }
    EndDeferWindowPos( hdwp);
    /* should not be needed */
    RedrawWindow( hwnd, NULL, 0, RDW_ALLCHILDREN | RDW_INVALIDATE );
    return TRUE;
}

/***********************************************************************
 *          FileOpenDlgProc95
 *
 * File open dialog procedure
 */
INT_PTR CALLBACK FileOpenDlgProc95(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
  TRACE("%p 0x%04x\n", hwnd, uMsg);
#endif

  switch(uMsg)
  {
    case WM_INITDIALOG:
      {
         FileOpenDlgInfos * fodInfos = (FileOpenDlgInfos *)lParam;
         RECT rc, rcstc;
         int gripx = GetSystemMetrics( SM_CYHSCROLL);
         int gripy = GetSystemMetrics( SM_CYVSCROLL);

	 /* Adds the FileOpenDlgInfos in the property list of the dialog
            so it will be easily accessible through a GetPropA(...) */
         SetPropA(hwnd, FileOpenDlgInfosStr, fodInfos);

         FILEDLG95_InitControls(hwnd);

         if (fodInfos->ofnInfos->Flags & OFN_ENABLESIZING)
         {
             GetWindowRect( hwnd, &rc);
             fodInfos->DlgInfos.hwndGrip =
                 CreateWindowExA( 0, "SCROLLBAR", NULL,
                     WS_CHILD | WS_GROUP | WS_VISIBLE | WS_CLIPSIBLINGS |
                     SBS_SIZEGRIP | SBS_SIZEBOXBOTTOMRIGHTALIGN,
                     rc.right - gripx, rc.bottom - gripy,
                     gripx, gripy, hwnd, (HMENU) -1, COMDLG32_hInstance, NULL);
         }

      	 fodInfos->DlgInfos.hwndCustomDlg =
     	   CreateTemplateDialog((FileOpenDlgInfos *)lParam, hwnd);

         FILEDLG95_ResizeControls(hwnd, wParam, lParam);
      	 FILEDLG95_FillControls(hwnd, wParam, lParam);

         if( fodInfos->DlgInfos.hwndCustomDlg)
             ShowWindow( fodInfos->DlgInfos.hwndCustomDlg, SW_SHOW);

         if(fodInfos->ofnInfos->Flags & OFN_EXPLORER) {
             SendCustomDlgNotificationMessage(hwnd,CDN_INITDONE);
             SendCustomDlgNotificationMessage(hwnd,CDN_FOLDERCHANGE);
         }

         /* if the app has changed the position of the invisible listbox,
          * change that of the listview (browser) as well */
         GetWindowRect( fodInfos->ShellInfos.hwndView, &rc);
         GetWindowRect( GetDlgItem( hwnd, IDC_SHELLSTATIC ), &rcstc);
         if( !EqualRect( &rc, &rcstc))
         {
             MapWindowPoints( NULL, hwnd, (LPPOINT) &rcstc, 2);
             SetWindowPos( fodInfos->ShellInfos.hwndView, NULL,
                     rcstc.left, rcstc.top, rcstc.right - rcstc.left, rcstc.bottom - rcstc.top,
                     SWP_NOACTIVATE | SWP_NOZORDER);
         }

         if (fodInfos->ofnInfos->Flags & OFN_ENABLESIZING)
         {
             GetWindowRect( hwnd, &rc);
             fodInfos->sizedlg.cx = rc.right - rc.left;
             fodInfos->sizedlg.cy = rc.bottom - rc.top;
             fodInfos->initial_size.x = fodInfos->sizedlg.cx;
             fodInfos->initial_size.y = fodInfos->sizedlg.cy;
             GetClientRect( hwnd, &rc);
             SetWindowPos( fodInfos->DlgInfos.hwndGrip, NULL,
                     rc.right - gripx, rc.bottom - gripy,
                     0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
             /* resize the dialog to the previous invocation */
             if( MemDialogSize.cx && MemDialogSize.cy)
                 SetWindowPos( hwnd, NULL,
                         0, 0, MemDialogSize.cx, MemDialogSize.cy,
                         SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
         }

         if(fodInfos->ofnInfos->Flags & OFN_EXPLORER)
             SendCustomDlgNotificationMessage(hwnd,CDN_SELCHANGE);

         return 0;
       }
    case WM_SIZE:
      return FILEDLG95_OnWMSize(hwnd, wParam);
    case WM_GETMINMAXINFO:
      return FILEDLG95_OnWMGetMMI( hwnd, (LPMINMAXINFO)lParam);
    case WM_COMMAND:
      return FILEDLG95_OnWMCommand(hwnd, wParam);
    case WM_DRAWITEM:
      {
        switch(((LPDRAWITEMSTRUCT)lParam)->CtlID)
        {
        case IDC_LOOKIN:
          FILEDLG95_LOOKIN_DrawItem((LPDRAWITEMSTRUCT) lParam);
          return TRUE;
        }
      }
      return FALSE;

    case WM_GETISHELLBROWSER:
      return FILEDLG95_OnWMGetIShellBrowser(hwnd);

    case WM_DESTROY:
      {
          FileOpenDlgInfos * fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);
          if (fodInfos && fodInfos->ofnInfos->Flags & OFN_ENABLESIZING)
              MemDialogSize = fodInfos->sizedlg;
          RemovePropA(hwnd, FileOpenDlgInfosStr);
          return FALSE;
      }
    case WM_NOTIFY:
    {
	LPNMHDR lpnmh = (LPNMHDR)lParam;
	UINT stringId = -1;

	/* set up the button tooltips strings */
	if(TTN_GETDISPINFOA == lpnmh->code )
	{
	    LPNMTTDISPINFOA lpdi = (LPNMTTDISPINFOA)lParam;
	    switch(lpnmh->idFrom )
	    {
		/* Up folder button */
		case FCIDM_TB_UPFOLDER:
		    stringId = IDS_UPFOLDER;
		    break;
		/* New folder button */
		case FCIDM_TB_NEWFOLDER:
		    stringId = IDS_NEWFOLDER;
		    break;
		/* List option button */
		case FCIDM_TB_SMALLICON:
		    stringId = IDS_LISTVIEW;
		    break;
		/* Details option button */
		case FCIDM_TB_REPORTVIEW:
		    stringId = IDS_REPORTVIEW;
		    break;
		/* Desktop button */
		case FCIDM_TB_DESKTOP:
		    stringId = IDS_TODESKTOP;
		    break;
		default:
		    stringId = 0;
	    }
	    lpdi->hinst = COMDLG32_hInstance;
	    lpdi->lpszText =  MAKEINTRESOURCEA(stringId);
	}
        return FALSE;
    }
    default :
      if(uMsg >= CDM_FIRST && uMsg <= CDM_LAST)
        return FILEDLG95_HandleCustomDialogMessages(hwnd, uMsg, wParam, lParam);
      return FALSE;
  }
}

static inline BOOL filename_is_edit( const FileOpenDlgInfos *info )
{
    return (info->ofnInfos->lStructSize == OPENFILENAME_SIZE_VERSION_400W) &&
        (info->ofnInfos->Flags & (OFN_ENABLEHOOK | OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE));
}

/***********************************************************************
 *      FILEDLG95_InitControls
 *
 * WM_INITDIALOG message handler (before hook notification)
 */
static LRESULT FILEDLG95_InitControls(HWND hwnd)
{
  BOOL win2000plus = FALSE;
  BOOL win98plus   = FALSE;
  BOOL handledPath = FALSE;
  OSVERSIONINFOW osVi;
  static const WCHAR szwSlash[] = { '\\', 0 };
  static const WCHAR szwStar[] = { '*',0 };

  static const TBBUTTON tbb[] =
  {
   {0,                 0,                   TBSTATE_ENABLED, BTNS_SEP, {0, 0}, 0, 0 },
   {VIEW_PARENTFOLDER, FCIDM_TB_UPFOLDER,   TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0 },
   {0,                 0,                   TBSTATE_ENABLED, BTNS_SEP, {0, 0}, 0, 0 },
   {VIEW_NEWFOLDER+1,  FCIDM_TB_DESKTOP,    TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0 },
   {0,                 0,                   TBSTATE_ENABLED, BTNS_SEP, {0, 0}, 0, 0 },
   {VIEW_NEWFOLDER,    FCIDM_TB_NEWFOLDER,  TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0 },
   {0,                 0,                   TBSTATE_ENABLED, BTNS_SEP, {0, 0}, 0, 0 },
   {VIEW_LIST,         FCIDM_TB_SMALLICON,  TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0 },
   {VIEW_DETAILS,      FCIDM_TB_REPORTVIEW, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0 },
  };
  static const TBADDBITMAP tba = {HINST_COMMCTRL, IDB_VIEW_SMALL_COLOR};

  RECT rectTB;
  RECT rectlook;

  HIMAGELIST toolbarImageList;
  SHFILEINFOA shFileInfo;
  ITEMIDLIST *desktopPidl;

  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("%p\n", fodInfos);

  /* Get windows version emulating */
  osVi.dwOSVersionInfoSize = sizeof(osVi);
  GetVersionExW(&osVi);
  if (osVi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
    win98plus   = ((osVi.dwMajorVersion > 4) || ((osVi.dwMajorVersion == 4) && (osVi.dwMinorVersion > 0)));
  } else if (osVi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
    win2000plus = (osVi.dwMajorVersion > 4);
    if (win2000plus) win98plus = TRUE;
  }
  TRACE("Running on 2000+ %d, 98+ %d\n", win2000plus, win98plus);


  /* Use either the edit or the comboboxex for the filename control */
  if (filename_is_edit( fodInfos ))
  {
      DestroyWindow( GetDlgItem( hwnd, cmb13 ) );
      fodInfos->DlgInfos.hwndFileName = GetDlgItem( hwnd, edt1 );
  }
  else
  {
      DestroyWindow( GetDlgItem( hwnd, edt1 ) );
      fodInfos->DlgInfos.hwndFileName = GetDlgItem( hwnd, cmb13 );
  }

  /* Get the hwnd of the controls */
  fodInfos->DlgInfos.hwndFileTypeCB = GetDlgItem(hwnd,IDC_FILETYPE);
  fodInfos->DlgInfos.hwndLookInCB = GetDlgItem(hwnd,IDC_LOOKIN);

  GetWindowRect( fodInfos->DlgInfos.hwndLookInCB,&rectlook);
  MapWindowPoints( 0, hwnd,(LPPOINT)&rectlook,2);

  /* construct the toolbar */
  GetWindowRect(GetDlgItem(hwnd,IDC_TOOLBARSTATIC),&rectTB);
  MapWindowPoints( 0, hwnd,(LPPOINT)&rectTB,2);

  rectTB.right = rectlook.right + rectTB.right - rectTB.left;
  rectTB.bottom = rectlook.top - 1 + rectTB.bottom - rectTB.top;
  rectTB.left = rectlook.right;
  rectTB.top = rectlook.top-1;

  if (fodInfos->unicode)
      fodInfos->DlgInfos.hwndTB = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL,
          WS_CHILD | WS_GROUP | WS_VISIBLE | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS | CCS_NODIVIDER | CCS_NORESIZE,
          rectTB.left, rectTB.top,
          rectTB.right - rectTB.left, rectTB.bottom - rectTB.top,
          hwnd, (HMENU)IDC_TOOLBAR, COMDLG32_hInstance, NULL);
  else
      fodInfos->DlgInfos.hwndTB = CreateWindowExA(0, TOOLBARCLASSNAMEA, NULL,
          WS_CHILD | WS_GROUP | WS_VISIBLE | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS | CCS_NODIVIDER | CCS_NORESIZE,
          rectTB.left, rectTB.top,
          rectTB.right - rectTB.left, rectTB.bottom - rectTB.top,
          hwnd, (HMENU)IDC_TOOLBAR, COMDLG32_hInstance, NULL);

  SendMessageW(fodInfos->DlgInfos.hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

/* FIXME: use TB_LOADIMAGES when implemented */
/*  SendMessageW(fodInfos->DlgInfos.hwndTB, TB_LOADIMAGES, IDB_VIEW_SMALL_COLOR, HINST_COMMCTRL);*/
  SendMessageW(fodInfos->DlgInfos.hwndTB, TB_SETMAXTEXTROWS, 0, 0);
  SendMessageW(fodInfos->DlgInfos.hwndTB, TB_ADDBITMAP, 12, (LPARAM) &tba);

  /* Retrieve and add desktop icon to the toolbar */
  toolbarImageList = (HIMAGELIST)SendMessageW(fodInfos->DlgInfos.hwndTB, TB_GETIMAGELIST, 0, 0L);
  SHGetSpecialFolderLocation(hwnd, CSIDL_DESKTOP, &desktopPidl);
  SHGetFileInfoA((LPCSTR)desktopPidl, 0, &shFileInfo, sizeof(shFileInfo),
    SHGFI_PIDL | SHGFI_ICON | SHGFI_SMALLICON);
  ImageList_AddIcon(toolbarImageList, shFileInfo.hIcon);

  DestroyIcon(shFileInfo.hIcon);
  CoTaskMemFree(desktopPidl);

  /* Finish Toolbar Construction */
  SendMessageW(fodInfos->DlgInfos.hwndTB, TB_ADDBUTTONSW, 9, (LPARAM) tbb);
  SendMessageW(fodInfos->DlgInfos.hwndTB, TB_AUTOSIZE, 0, 0);

  /* Set the window text with the text specified in the OPENFILENAME structure */
  if(fodInfos->title)
  {
      SetWindowTextW(hwnd,fodInfos->title);
  }
  else if (fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG)
  {
      WCHAR buf[64];
      LoadStringW(COMDLG32_hInstance, IDS_SAVE_AS, buf, sizeof(buf)/sizeof(WCHAR));
      SetWindowTextW(hwnd, buf);
  }

  /* Initialise the file name edit control */
  handledPath = FALSE;
  TRACE("Before manipulation, file = %s, dir = %s\n", debugstr_w(fodInfos->filename), debugstr_w(fodInfos->initdir));

  if(fodInfos->filename)
  {
      /* 1. If win2000 or higher and filename contains a path, use it
         in preference over the lpstrInitialDir                       */
      if (win2000plus && *fodInfos->filename && strpbrkW(fodInfos->filename, szwSlash)) {
         WCHAR tmpBuf[MAX_PATH];
         WCHAR *nameBit;
         DWORD result;

         result = GetFullPathNameW(fodInfos->filename, MAX_PATH, tmpBuf, &nameBit);
         if (result) {

            /* nameBit is always shorter than the original filename */
            lstrcpyW(fodInfos->filename,nameBit);

            *nameBit = 0x00;
            MemFree(fodInfos->initdir);
            fodInfos->initdir = MemAlloc((lstrlenW(tmpBuf) + 1)*sizeof(WCHAR));
            lstrcpyW(fodInfos->initdir, tmpBuf);
            handledPath = TRUE;
            TRACE("Value in Filename includes path, overriding InitialDir: %s, %s\n",
                    debugstr_w(fodInfos->filename), debugstr_w(fodInfos->initdir));
         }
         SetWindowTextW( fodInfos->DlgInfos.hwndFileName, fodInfos->filename );

      } else {
         SetWindowTextW( fodInfos->DlgInfos.hwndFileName, fodInfos->filename );
      }
  }

  /* 2. (All platforms) If initdir is not null, then use it */
  if (!handledPath && fodInfos->initdir && *fodInfos->initdir)
  {
        /* Work out the proper path as supplied one might be relative          */
        /* (Here because supplying '.' as dir browses to My Computer)          */
        WCHAR tmpBuf[MAX_PATH];
        WCHAR tmpBuf2[MAX_PATH];
        WCHAR *nameBit;
        DWORD result;

        lstrcpyW(tmpBuf, fodInfos->initdir);
        if (PathFileExistsW(tmpBuf)) {
            /* initdir does not have to be a directory. If a file is
             * specified, the dir part is taken */
            if (PathIsDirectoryW(tmpBuf)) {
                PathAddBackslashW(tmpBuf);
                lstrcatW(tmpBuf, szwStar);
            }
            result = GetFullPathNameW(tmpBuf, MAX_PATH, tmpBuf2, &nameBit);
            if (result) {
                *nameBit = 0x00;
                MemFree(fodInfos->initdir);
                fodInfos->initdir = MemAlloc((lstrlenW(tmpBuf2) + 1) * sizeof(WCHAR));
                lstrcpyW(fodInfos->initdir, tmpBuf2);
                handledPath = TRUE;
                TRACE("Value in InitDir changed to %s\n", debugstr_w(fodInfos->initdir));
            }
        }
        else if (fodInfos->initdir)
        {
            MemFree(fodInfos->initdir);
            fodInfos->initdir = NULL;
            TRACE("Value in InitDir is not an existing path, changed to (nil)\n");
        }
  }

  if (!handledPath && (!fodInfos->initdir || !*fodInfos->initdir))
  {
      /* 3. All except w2k+: if filename contains a path use it */
      if (!win2000plus && fodInfos->filename &&
          *fodInfos->filename &&
          strpbrkW(fodInfos->filename, szwSlash)) {
         WCHAR tmpBuf[MAX_PATH];
         WCHAR *nameBit;
         DWORD result;

         result = GetFullPathNameW(fodInfos->filename, MAX_PATH,
                                  tmpBuf, &nameBit);
         if (result) {
            int len;

            /* nameBit is always shorter than the original filename */
            lstrcpyW(fodInfos->filename, nameBit);
            *nameBit = 0x00;

            len = lstrlenW(tmpBuf);
            MemFree(fodInfos->initdir);
            fodInfos->initdir = MemAlloc((len+1)*sizeof(WCHAR));
            lstrcpyW(fodInfos->initdir, tmpBuf);

            handledPath = TRUE;
            TRACE("Value in Filename includes path, overriding initdir: %s, %s\n",
                 debugstr_w(fodInfos->filename), debugstr_w(fodInfos->initdir));
         }
         SetWindowTextW( fodInfos->DlgInfos.hwndFileName, fodInfos->filename );
      }

      /* 4. Win2000+: Recently used */
      if (!handledPath && win2000plus) {
          fodInfos->initdir = MemAlloc(MAX_PATH * sizeof(WCHAR));
          fodInfos->initdir[0] = '\0';

          FILEDLG95_MRU_load_filename(fodInfos->initdir);

          if (fodInfos->initdir[0] && PathFileExistsW(fodInfos->initdir)){
             handledPath = TRUE;
          }else{
             MemFree(fodInfos->initdir);
             fodInfos->initdir = NULL;
          }
      }

      /* 5. win98+ and win2000+ if any files of specified filter types in
            current directory, use it                                      */
      if (win98plus && !handledPath && fodInfos->filter && *fodInfos->filter) {

         LPCWSTR lpstrPos = fodInfos->filter;
         WIN32_FIND_DATAW FindFileData;
         HANDLE hFind;

         while (1)
         {
           /* filter is a list...  title\0ext\0......\0\0 */

           /* Skip the title */
           if(! *lpstrPos) break;	/* end */
           lpstrPos += lstrlenW(lpstrPos) + 1;

           /* See if any files exist in the current dir with this extension */
           if(! *lpstrPos) break;	/* end */

           hFind = FindFirstFileW(lpstrPos, &FindFileData);

           if (hFind == INVALID_HANDLE_VALUE) {
               /* None found - continue search */
               lpstrPos += lstrlenW(lpstrPos) + 1;

           } else {

               MemFree(fodInfos->initdir);
               fodInfos->initdir = MemAlloc(MAX_PATH*sizeof(WCHAR));
               GetCurrentDirectoryW(MAX_PATH, fodInfos->initdir);

               handledPath = TRUE;
               TRACE("No initial dir specified, but files of type %s found in current, so using it\n",
                 debugstr_w(lpstrPos));
               FindClose(hFind);
               break;
           }
         }
      }

      /* 6. Win98+ and 2000+: Use personal files dir, others use current dir */
      if (!handledPath && (win2000plus || win98plus)) {
          fodInfos->initdir = MemAlloc(MAX_PATH*sizeof(WCHAR));

          if(!COMDLG32_SHGetFolderPathW(hwnd, CSIDL_PERSONAL, 0, 0, fodInfos->initdir))
          {
            if(!COMDLG32_SHGetFolderPathW(hwnd, CSIDL_DESKTOPDIRECTORY|CSIDL_FLAG_CREATE, 0, 0, fodInfos->initdir))
            {
                /* last fallback */
                GetCurrentDirectoryW(MAX_PATH, fodInfos->initdir);
                TRACE("No personal or desktop dir, using cwd as failsafe: %s\n", debugstr_w(fodInfos->initdir));
            } else {
                TRACE("No personal dir, using desktop instead: %s\n", debugstr_w(fodInfos->initdir));
            }
          } else {
            TRACE("No initial dir specified, using personal files dir of %s\n", debugstr_w(fodInfos->initdir));
          }
          handledPath = TRUE;
      } else if (!handledPath) {
          fodInfos->initdir = MemAlloc(MAX_PATH*sizeof(WCHAR));
          GetCurrentDirectoryW(MAX_PATH, fodInfos->initdir);
          handledPath = TRUE;
          TRACE("No initial dir specified, using current dir of %s\n", debugstr_w(fodInfos->initdir));
      }
  }
  SetFocus( fodInfos->DlgInfos.hwndFileName );
  TRACE("After manipulation, file = %s, dir = %s\n", debugstr_w(fodInfos->filename), debugstr_w(fodInfos->initdir));

  /* Must the open as read only check box be checked ?*/
  if(fodInfos->ofnInfos->Flags & OFN_READONLY)
  {
    SendDlgItemMessageW(hwnd,IDC_OPENREADONLY,BM_SETCHECK,TRUE,0);
  }

  /* Must the open as read only check box be hidden? */
  if(fodInfos->ofnInfos->Flags & OFN_HIDEREADONLY)
  {
    ShowWindow(GetDlgItem(hwnd,IDC_OPENREADONLY),SW_HIDE);
    EnableWindow(GetDlgItem(hwnd, IDC_OPENREADONLY), FALSE);
  }

  /* Must the help button be hidden? */
  if (!(fodInfos->ofnInfos->Flags & OFN_SHOWHELP))
  {
    ShowWindow(GetDlgItem(hwnd, pshHelp), SW_HIDE);
    EnableWindow(GetDlgItem(hwnd, pshHelp), FALSE);
  }

  /* change Open to Save */
  if (fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG)
  {
      WCHAR buf[16];
      LoadStringW(COMDLG32_hInstance, IDS_SAVE_BUTTON, buf, sizeof(buf)/sizeof(WCHAR));
      SetDlgItemTextW(hwnd, IDOK, buf);
      LoadStringW(COMDLG32_hInstance, IDS_SAVE_IN, buf, sizeof(buf)/sizeof(WCHAR));
      SetDlgItemTextW(hwnd, IDC_LOOKINSTATIC, buf);
  }

  /* Initialize the filter combo box */
  FILEDLG95_FILETYPE_Init(hwnd);

  return 0;
}

/***********************************************************************
 *      FILEDLG95_ResizeControls
 *
 * WM_INITDIALOG message handler (after hook notification)
 */
static LRESULT FILEDLG95_ResizeControls(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) lParam;

  if (fodInfos->DlgInfos.hwndCustomDlg)
  {
    RECT rc;
    UINT flags = SWP_NOACTIVATE;

    ArrangeCtrlPositions(fodInfos->DlgInfos.hwndCustomDlg, hwnd,
        (fodInfos->ofnInfos->Flags & (OFN_HIDEREADONLY | OFN_SHOWHELP)) == OFN_HIDEREADONLY);

    /* resize the custom dialog to the parent size */
    if (fodInfos->ofnInfos->Flags & (OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE))
      GetClientRect(hwnd, &rc);
    else
    {
      /* our own fake template is zero sized and doesn't have children, so
       * there is no need to resize it. Picasa depends on it.
       */
      flags |= SWP_NOSIZE;
      SetRectEmpty(&rc);
    }
    SetWindowPos(fodInfos->DlgInfos.hwndCustomDlg, HWND_BOTTOM,
                 0, 0, rc.right, rc.bottom, flags);
  }
  else
  {
    /* Resize the height; if opened as read-only, checkbox and help button are
     * hidden and we are not using a custom template nor a customDialog
     */
    if ( (fodInfos->ofnInfos->Flags & OFN_HIDEREADONLY) &&
                (!(fodInfos->ofnInfos->Flags &
                   (OFN_SHOWHELP|OFN_ENABLETEMPLATE|OFN_ENABLETEMPLATEHANDLE))))
    {
      RECT rectDlg, rectHelp, rectCancel;
      GetWindowRect(hwnd, &rectDlg);
      GetWindowRect(GetDlgItem(hwnd, pshHelp), &rectHelp);
      GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rectCancel);
      /* subtract the height of the help button plus the space between the help
       * button and the cancel button to the height of the dialog
       */
      SetWindowPos(hwnd, 0, 0, 0, rectDlg.right-rectDlg.left,
          (rectDlg.bottom-rectDlg.top) - (rectHelp.bottom - rectCancel.bottom),
          SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
    }
  }
  return TRUE;
}

/***********************************************************************
 *      FILEDLG95_FillControls
 *
 * WM_INITDIALOG message handler (after hook notification)
 */
static LRESULT FILEDLG95_FillControls(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  LPITEMIDLIST pidlItemId = NULL;

  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) lParam;

  TRACE("dir=%s file=%s\n",
  debugstr_w(fodInfos->initdir), debugstr_w(fodInfos->filename));

  /* Get the initial directory pidl */

  if(!(pidlItemId = GetPidlFromName(fodInfos->Shell.FOIShellFolder,fodInfos->initdir)))
  {
    WCHAR path[MAX_PATH];

    GetCurrentDirectoryW(MAX_PATH,path);
    pidlItemId = GetPidlFromName(fodInfos->Shell.FOIShellFolder, path);
  }

  /* Initialise shell objects */
  FILEDLG95_SHELL_Init(hwnd);

  /* Initialize the Look In combo box */
  FILEDLG95_LOOKIN_Init(fodInfos->DlgInfos.hwndLookInCB);

  /* Browse to the initial directory */
  IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser,pidlItemId, SBSP_ABSOLUTE);

  /* Free pidlItem memory */
  COMDLG32_SHFree(pidlItemId);

  return TRUE;
}
/***********************************************************************
 *      FILEDLG95_Clean
 *
 * Regroups all the cleaning functions of the filedlg
 */
void FILEDLG95_Clean(HWND hwnd)
{
      FILEDLG95_FILETYPE_Clean(hwnd);
      FILEDLG95_LOOKIN_Clean(hwnd);
      FILEDLG95_SHELL_Clean(hwnd);
}
/***********************************************************************
 *      FILEDLG95_OnWMCommand
 *
 * WM_COMMAND message handler
 */
static LRESULT FILEDLG95_OnWMCommand(HWND hwnd, WPARAM wParam)
{
  WORD wNotifyCode = HIWORD(wParam); /* notification code */
  WORD wID = LOWORD(wParam);         /* item, control, or accelerator identifier */
  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);

  switch(wID)
  {
    /* OK button */
  case IDOK:
    FILEDLG95_OnOpen(hwnd);
    break;
    /* Cancel button */
  case IDCANCEL:
    FILEDLG95_Clean(hwnd);
    EndDialog(hwnd, FALSE);
    break;
    /* Filetype combo box */
  case IDC_FILETYPE:
    FILEDLG95_FILETYPE_OnCommand(hwnd,wNotifyCode);
    break;
    /* LookIn combo box */
  case IDC_LOOKIN:
    FILEDLG95_LOOKIN_OnCommand(hwnd,wNotifyCode);
    break;

  /* --- toolbar --- */
    /* Up folder button */
  case FCIDM_TB_UPFOLDER:
    FILEDLG95_SHELL_UpFolder(hwnd);
    break;
    /* New folder button */
  case FCIDM_TB_NEWFOLDER:
    FILEDLG95_SHELL_ExecuteCommand(hwnd,CMDSTR_NEWFOLDERA);
    break;
    /* List option button */
  case FCIDM_TB_SMALLICON:
    FILEDLG95_SHELL_ExecuteCommand(hwnd,CMDSTR_VIEWLISTA);
    break;
    /* Details option button */
  case FCIDM_TB_REPORTVIEW:
    FILEDLG95_SHELL_ExecuteCommand(hwnd,CMDSTR_VIEWDETAILSA);
    break;
    /* Details option button */
  case FCIDM_TB_DESKTOP:
    FILEDLG95_SHELL_BrowseToDesktop(hwnd);
    break;

  case edt1:
  case cmb13:
    break;

  }
  /* Do not use the listview selection anymore */
  fodInfos->DlgInfos.dwDlgProp &= ~FODPROP_USEVIEW;
  return 0;
}

/***********************************************************************
 *      FILEDLG95_OnWMGetIShellBrowser
 *
 * WM_GETISHELLBROWSER message handler
 */
static LRESULT FILEDLG95_OnWMGetIShellBrowser(HWND hwnd)
{
  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  SetWindowLongPtrW(hwnd,DWLP_MSGRESULT,(LONG_PTR)fodInfos->Shell.FOIShellBrowser);

  return TRUE;
}


/***********************************************************************
 *      FILEDLG95_SendFileOK
 *
 * Sends the CDN_FILEOK notification if required
 *
 * RETURNS
 *  TRUE if the dialog should close
 *  FALSE if the dialog should not be closed
 */
static BOOL FILEDLG95_SendFileOK( HWND hwnd, FileOpenDlgInfos *fodInfos )
{
    /* ask the hook if we can close */
    if(IsHooked(fodInfos))
    {
        LRESULT retval = 0;

        TRACE("---\n");
        /* First send CDN_FILEOK as MSDN doc says */
        if(fodInfos->ofnInfos->Flags & OFN_EXPLORER)
            retval = SendCustomDlgNotificationMessage(hwnd,CDN_FILEOK);
        if( retval)
        {
            TRACE("canceled\n");
            return FALSE;
        }

        /* fodInfos->ofnInfos points to an ASCII or UNICODE structure as appropriate */
        retval = SendMessageW(fodInfos->DlgInfos.hwndCustomDlg,
                              fodInfos->HookMsg.fileokstring, 0, (LPARAM)fodInfos->ofnInfos);
        if( retval)
        {
            TRACE("canceled\n");
            return FALSE;
        }
    }
    return TRUE;
}

/***********************************************************************
 *      FILEDLG95_OnOpenMultipleFiles
 *
 * Handles the opening of multiple files.
 *
 * FIXME
 *  check destination buffer size
 */
BOOL FILEDLG95_OnOpenMultipleFiles(HWND hwnd, LPWSTR lpstrFileList, UINT nFileCount, UINT sizeUsed)
{
  WCHAR   lpstrPathSpec[MAX_PATH] = {0};
  UINT   nCount, nSizePath;
  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  if(fodInfos->unicode)
  {
     LPOPENFILENAMEW ofn = fodInfos->ofnInfos;
     ofn->lpstrFile[0] = '\0';
  }
  else
  {
     LPOPENFILENAMEA ofn = (LPOPENFILENAMEA) fodInfos->ofnInfos;
     ofn->lpstrFile[0] = '\0';
  }

  COMDLG32_GetDisplayNameOf( fodInfos->ShellInfos.pidlAbsCurrent, lpstrPathSpec );

  if ( !(fodInfos->ofnInfos->Flags & OFN_NOVALIDATE) &&
      ( fodInfos->ofnInfos->Flags & OFN_FILEMUSTEXIST) &&
       ! ( fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG ) )
  {
    LPWSTR lpstrTemp = lpstrFileList;

    for ( nCount = 0; nCount < nFileCount; nCount++ )
    {
      LPITEMIDLIST pidl;

      pidl = GetPidlFromName(fodInfos->Shell.FOIShellFolder, lpstrTemp);
      if (!pidl)
      {
        WCHAR lpstrNotFound[100];
        WCHAR lpstrMsg[100];
        WCHAR tmp[400];
        static const WCHAR nl[] = {'\n',0};

        LoadStringW(COMDLG32_hInstance, IDS_FILENOTFOUND, lpstrNotFound, 100);
        LoadStringW(COMDLG32_hInstance, IDS_VERIFYFILE, lpstrMsg, 100);

        lstrcpyW(tmp, lpstrTemp);
        lstrcatW(tmp, nl);
        lstrcatW(tmp, lpstrNotFound);
        lstrcatW(tmp, nl);
        lstrcatW(tmp, lpstrMsg);

        MessageBoxW(hwnd, tmp, fodInfos->title, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
      }

      /* move to the next file in the list of files */
      lpstrTemp += lstrlenW(lpstrTemp) + 1;
      COMDLG32_SHFree(pidl);
    }
  }

  nSizePath = lstrlenW(lpstrPathSpec) + 1;
  if ( !(fodInfos->ofnInfos->Flags & OFN_EXPLORER) )
  {
    /* For "oldstyle" dialog the components have to
       be separated by blanks (not '\0'!) and short
       filenames have to be used! */
    FIXME("Components have to be separated by blanks\n");
  }
  if(fodInfos->unicode)
  {
    LPOPENFILENAMEW ofn = fodInfos->ofnInfos;
    lstrcpyW( ofn->lpstrFile, lpstrPathSpec);
    memcpy( ofn->lpstrFile + nSizePath, lpstrFileList, sizeUsed*sizeof(WCHAR) );
  }
  else
  {
    LPOPENFILENAMEA ofn = (LPOPENFILENAMEA)fodInfos->ofnInfos;

    if (ofn->lpstrFile != NULL)
    {
      nSizePath = WideCharToMultiByte(CP_ACP, 0, lpstrPathSpec, -1,
			  ofn->lpstrFile, ofn->nMaxFile, NULL, NULL);
      if (ofn->nMaxFile > nSizePath)
      {
	WideCharToMultiByte(CP_ACP, 0, lpstrFileList, sizeUsed,
			    ofn->lpstrFile + nSizePath,
			    ofn->nMaxFile - nSizePath, NULL, NULL);
      }
    }
  }

  fodInfos->ofnInfos->nFileOffset = nSizePath;
  fodInfos->ofnInfos->nFileExtension = 0;

  if ( !FILEDLG95_SendFileOK(hwnd, fodInfos) )
    return FALSE;

  /* clean and exit */
  FILEDLG95_Clean(hwnd);
  return EndDialog(hwnd,TRUE);
}

/* Returns the 'slot name' of the given module_name in the registry's
 * most-recently-used list.  This will be an ASCII value in the
 * range ['a','z'). Returns zero on error.
 *
 * The slot's value in the registry has the form:
 *   module_name\0mru_path\0
 *
 * If stored_path is given, then stored_path will contain the path name
 * stored in the registry's MRU list for the given module_name.
 *
 * If hkey_ret is given, then hkey_ret will be a handle to the registry's
 * MRU list key for the given module_name.
 */
static WCHAR FILEDLG95_MRU_get_slot(LPCWSTR module_name, LPWSTR stored_path, PHKEY hkey_ret)
{
    WCHAR mru_list[32], *cur_mru_slot;
    BOOL taken[25] = {0};
    DWORD mru_list_size = sizeof(mru_list), key_type = -1, i;
    HKEY hkey_tmp, *hkey;
    LONG ret;

    if(hkey_ret)
        hkey = hkey_ret;
    else
        hkey = &hkey_tmp;

    if(stored_path)
        *stored_path = '\0';

    ret = RegCreateKeyW(HKEY_CURRENT_USER, LastVisitedMRUW, hkey);
    if(ret){
        WARN("Unable to create MRU key: %d\n", ret);
        return 0;
    }

    ret = RegGetValueW(*hkey, NULL, MRUListW, RRF_RT_REG_SZ, &key_type,
            (LPBYTE)mru_list, &mru_list_size);
    if(ret || key_type != REG_SZ){
        if(ret == ERROR_FILE_NOT_FOUND)
            return 'a';

        WARN("Error getting MRUList data: type: %d, ret: %d\n", key_type, ret);
        RegCloseKey(*hkey);
        return 0;
    }

    for(cur_mru_slot = mru_list; *cur_mru_slot; ++cur_mru_slot){
        WCHAR value_data[MAX_PATH], value_name[2] = {0};
        DWORD value_data_size = sizeof(value_data);

        *value_name = *cur_mru_slot;

        ret = RegGetValueW(*hkey, NULL, value_name, RRF_RT_REG_BINARY,
                &key_type, (LPBYTE)value_data, &value_data_size);
        if(ret || key_type != REG_BINARY){
            WARN("Error getting MRU slot data: type: %d, ret: %d\n", key_type, ret);
            continue;
        }

        if(!strcmpiW(module_name, value_data)){
            if(!hkey_ret)
                RegCloseKey(*hkey);
            if(stored_path)
                lstrcpyW(stored_path, value_data + lstrlenW(value_data) + 1);
            return *value_name;
        }
    }

    if(!hkey_ret)
        RegCloseKey(*hkey);

    /* the module name isn't in the registry, so find the next open slot */
    for(cur_mru_slot = mru_list; *cur_mru_slot; ++cur_mru_slot)
        taken[*cur_mru_slot - 'a'] = TRUE;
    for(i = 0; i < 25; ++i){
        if(!taken[i])
            return i + 'a';
    }

    /* all slots are taken, so return the last one in MRUList */
    --cur_mru_slot;
    return *cur_mru_slot;
}

/* save the given filename as most-recently-used path for this module */
static void FILEDLG95_MRU_save_filename(LPCWSTR filename)
{
    WCHAR module_path[MAX_PATH], *module_name, slot, slot_name[2] = {0};
    LONG ret;
    HKEY hkey;

    /* get the current executable's name */
    if(!GetModuleFileNameW(GetModuleHandleW(NULL), module_path, sizeof(module_path)/sizeof(module_path[0]))) {
        WARN("GotModuleFileName failed: %d\n", GetLastError());
        return;
    }
    module_name = strrchrW(module_path, '\\');
    if(!module_name)
        module_name = module_path;
    else
        module_name += 1;

    slot = FILEDLG95_MRU_get_slot(module_name, NULL, &hkey);
    if(!slot)
        return;
    *slot_name = slot;

    { /* update the slot's info */
        WCHAR *path_ends, *final;
        DWORD path_len, final_len;

        /* use only the path segment of `filename' */
        path_ends = strrchrW(filename, '\\');
        path_len = path_ends - filename;

        final_len = path_len + lstrlenW(module_name) + 2;

        final = MemAlloc(final_len * sizeof(WCHAR));
        if(!final)
            return;
        lstrcpyW(final, module_name);
        memcpy(final + lstrlenW(final) + 1, filename, path_len * sizeof(WCHAR));
        final[final_len-1] = '\0';

        ret = RegSetValueExW(hkey, slot_name, 0, REG_BINARY, (LPBYTE)final,
                final_len * sizeof(WCHAR));
        if(ret){
            WARN("Error saving MRU data to slot %s: %d\n", wine_dbgstr_w(slot_name), ret);
            MemFree(final);
            RegCloseKey(hkey);
            return;
        }

        MemFree(final);
    }

    { /* update MRUList value */
        WCHAR old_mru_list[32], new_mru_list[32];
        WCHAR *old_mru_slot, *new_mru_slot = new_mru_list;
        DWORD mru_list_size = sizeof(old_mru_list), key_type;

        ret = RegGetValueW(hkey, NULL, MRUListW, RRF_RT_ANY, &key_type,
                (LPBYTE)old_mru_list, &mru_list_size);
        if(ret || key_type != REG_SZ){
            if(ret == ERROR_FILE_NOT_FOUND){
                new_mru_list[0] = slot;
                new_mru_list[1] = '\0';
            }else{
                WARN("Error getting MRUList data: type: %d, ret: %d\n", key_type, ret);
                RegCloseKey(hkey);
                return;
            }
        }else{
            /* copy old list data over so that the new slot is at the start
             * of the list */
            *new_mru_slot++ = slot;
            for(old_mru_slot = old_mru_list; *old_mru_slot; ++old_mru_slot){
                if(*old_mru_slot != slot)
                    *new_mru_slot++ = *old_mru_slot;
            }
            *new_mru_slot = '\0';
        }

        ret = RegSetValueExW(hkey, MRUListW, 0, REG_SZ, (LPBYTE)new_mru_list,
                (lstrlenW(new_mru_list) + 1) * sizeof(WCHAR));
        if(ret){
            WARN("Error saving MRUList data: %d\n", ret);
            RegCloseKey(hkey);
            return;
        }
    }
}

/* load the most-recently-used path for this module */
static void FILEDLG95_MRU_load_filename(LPWSTR stored_path)
{
    WCHAR module_path[MAX_PATH], *module_name;

    /* get the current executable's name */
    if(!GetModuleFileNameW(GetModuleHandleW(NULL), module_path, sizeof(module_path)/sizeof(module_path[0]))) {
        WARN("GotModuleFileName failed: %d\n", GetLastError());
        return;
    }
    module_name = strrchrW(module_path, '\\');
    if(!module_name)
        module_name = module_path;
    else
        module_name += 1;

    FILEDLG95_MRU_get_slot(module_name, stored_path, NULL);
    TRACE("got MRU path: %s\n", wine_dbgstr_w(stored_path));
}

void FILEDLG95_OnOpenMessage(HWND hwnd, int idCaption, int idText)
{
  WCHAR strMsgTitle[MAX_PATH];
  WCHAR strMsgText [MAX_PATH];
  if (idCaption)
    LoadStringW(COMDLG32_hInstance, idCaption, strMsgTitle, sizeof(strMsgTitle)/sizeof(WCHAR));
  else
    strMsgTitle[0] = '\0';
  LoadStringW(COMDLG32_hInstance, idText, strMsgText, sizeof(strMsgText)/sizeof(WCHAR));
  MessageBoxW(hwnd,strMsgText, strMsgTitle, MB_OK | MB_ICONHAND);
}

int FILEDLG95_ValidatePathAction(LPWSTR lpstrPathAndFile, IShellFolder **ppsf,
                                 HWND hwnd, DWORD flags, BOOL isSaveDlg, int defAction)
{
    int nOpenAction = defAction;
    LPWSTR lpszTemp, lpszTemp1;
    LPITEMIDLIST pidl = NULL;
    static const WCHAR szwInvalid[] = { '/',':','<','>','|', 0};

    /* check for invalid chars */
    if((strpbrkW(lpstrPathAndFile+3, szwInvalid) != NULL) && !(flags & OFN_NOVALIDATE))
    {
        FILEDLG95_OnOpenMessage(hwnd, IDS_INVALID_FILENAME_TITLE, IDS_INVALID_FILENAME);
        return FALSE;
    }

    if (FAILED (SHGetDesktopFolder(ppsf))) return FALSE;

    lpszTemp1 = lpszTemp = lpstrPathAndFile;
    while (lpszTemp1)
    {
        LPSHELLFOLDER lpsfChild;
        WCHAR lpwstrTemp[MAX_PATH];
        DWORD dwEaten, dwAttributes;
        LPWSTR p;

        lstrcpyW(lpwstrTemp, lpszTemp);
        p = PathFindNextComponentW(lpwstrTemp);

        if (!p) break; /* end of path */

        *p = 0;
        lpszTemp = lpszTemp + lstrlenW(lpwstrTemp);

        /* There are no wildcards when OFN_NOVALIDATE is set */
        if(*lpszTemp==0 && !(flags & OFN_NOVALIDATE))
        {
            static const WCHAR wszWild[] = { '*', '?', 0 };
            /* if the last element is a wildcard do a search */
            if(strpbrkW(lpszTemp1, wszWild) != NULL)
            {
                nOpenAction = ONOPEN_SEARCH;
                break;
            }
        }
        lpszTemp1 = lpszTemp;

        TRACE("parse now=%s next=%s sf=%p\n",debugstr_w(lpwstrTemp), debugstr_w(lpszTemp), *ppsf);

        /* append a backslash to drive letters */
        if(lstrlenW(lpwstrTemp)==2 && lpwstrTemp[1] == ':' &&
           ((lpwstrTemp[0] >= 'a' && lpwstrTemp[0] <= 'z') ||
            (lpwstrTemp[0] >= 'A' && lpwstrTemp[0] <= 'Z')))
        {
            PathAddBackslashW(lpwstrTemp);
        }

        dwAttributes = SFGAO_FOLDER;
        if(SUCCEEDED(IShellFolder_ParseDisplayName(*ppsf, hwnd, NULL, lpwstrTemp, &dwEaten, &pidl, &dwAttributes)))
        {
            /* the path component is valid, we have a pidl of the next path component */
            TRACE("parse OK attr=0x%08x pidl=%p\n", dwAttributes, pidl);
            if(dwAttributes & SFGAO_FOLDER)
            {
                if(FAILED(IShellFolder_BindToObject(*ppsf, pidl, 0, &IID_IShellFolder, (LPVOID*)&lpsfChild)))
                {
                    ERR("bind to failed\n"); /* should not fail */
                    break;
                }
                IShellFolder_Release(*ppsf);
                *ppsf = lpsfChild;
                lpsfChild = NULL;
            }
            else
            {
                TRACE("value\n");

                /* end dialog, return value */
                nOpenAction = ONOPEN_OPEN;
                break;
            }
            COMDLG32_SHFree(pidl);
            pidl = NULL;
        }
        else if (!(flags & OFN_NOVALIDATE))
        {
            if(*lpszTemp ||	/* points to trailing null for last path element */
               (lpwstrTemp[strlenW(lpwstrTemp)-1] == '\\')) /* or if last element ends in '\' */
            {
                if(flags & OFN_PATHMUSTEXIST)
                {
                    FILEDLG95_OnOpenMessage(hwnd, 0, IDS_PATHNOTEXISTING);
                    break;
                }
            }
            else
            {
                if( (flags & OFN_FILEMUSTEXIST) && !isSaveDlg )
                {
                    FILEDLG95_OnOpenMessage(hwnd, 0, IDS_FILENOTEXISTING);
                    break;
                }
            }
            /* change to the current folder */
            nOpenAction = ONOPEN_OPEN;
            break;
        }
        else
        {
            nOpenAction = ONOPEN_OPEN;
            break;
        }
    }
    if(pidl) COMDLG32_SHFree(pidl);

    return nOpenAction;
}

/***********************************************************************
 *      FILEDLG95_OnOpen
 *
 * Ok button WM_COMMAND message handler
 *
 * If the function succeeds, the return value is nonzero.
 */
BOOL FILEDLG95_OnOpen(HWND hwnd)
{
  LPWSTR lpstrFileList;
  UINT nFileCount = 0;
  UINT sizeUsed = 0;
  BOOL ret = TRUE;
  WCHAR lpstrPathAndFile[MAX_PATH];
  LPSHELLFOLDER lpsf = NULL;
  int nOpenAction;
  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("hwnd=%p\n", hwnd);

  /* try to browse the selected item */
  if(BrowseSelectedFolder(hwnd))
      return FALSE;

  /* get the files from the edit control */
  nFileCount = FILEDLG95_FILENAME_GetFileNames(hwnd, &lpstrFileList, &sizeUsed);

  if(nFileCount == 0)
      return FALSE;

  if(nFileCount > 1)
  {
      ret = FILEDLG95_OnOpenMultipleFiles(hwnd, lpstrFileList, nFileCount, sizeUsed);
      goto ret;
  }

  TRACE("count=%u len=%u file=%s\n", nFileCount, sizeUsed, debugstr_w(lpstrFileList));

/*
  Step 1:  Build a complete path name from the current folder and
  the filename or path in the edit box.
  Special cases:
  - the path in the edit box is a root path
    (with or without drive letter)
  - the edit box contains ".." (or a path with ".." in it)
*/

  COMDLG32_GetCanonicalPath(fodInfos->ShellInfos.pidlAbsCurrent, lpstrFileList, lpstrPathAndFile);
  MemFree(lpstrFileList);

/*
  Step 2: here we have a cleaned up path

  We have to parse the path step by step to see if we have to browse
  to a folder if the path points to a directory or the last
  valid element is a directory.

  valid variables:
    lpstrPathAndFile: cleaned up path
 */

  if (nFileCount &&
      (fodInfos->ofnInfos->Flags & OFN_NOVALIDATE) &&
      !(fodInfos->ofnInfos->Flags & OFN_FILEMUSTEXIST))
    nOpenAction = ONOPEN_OPEN;
  else
    nOpenAction = ONOPEN_BROWSE;

  nOpenAction = FILEDLG95_ValidatePathAction(lpstrPathAndFile, &lpsf, hwnd,
                                             fodInfos->ofnInfos->Flags,
                                             fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG,
                                             nOpenAction);
  if(!nOpenAction)
      goto ret;

/*
  Step 3: here we have a cleaned up and validated path

  valid variables:
   lpsf:             ShellFolder bound to the rightmost valid path component
   lpstrPathAndFile: cleaned up path
   nOpenAction:      action to do
*/
  TRACE("end validate sf=%p\n", lpsf);

  switch(nOpenAction)
  {
    case ONOPEN_SEARCH:   /* set the current filter to the file mask and refresh */
      TRACE("ONOPEN_SEARCH %s\n", debugstr_w(lpstrPathAndFile));
      {
        int iPos;
        LPWSTR lpszTemp = PathFindFileNameW(lpstrPathAndFile);
        DWORD len;

        /* replace the current filter */
        MemFree(fodInfos->ShellInfos.lpstrCurrentFilter);
        len = lstrlenW(lpszTemp)+1;
        fodInfos->ShellInfos.lpstrCurrentFilter = MemAlloc(len * sizeof(WCHAR));
        lstrcpyW( fodInfos->ShellInfos.lpstrCurrentFilter, lpszTemp);

        /* set the filter cb to the extension when possible */
        if(-1 < (iPos = FILEDLG95_FILETYPE_SearchExt(fodInfos->DlgInfos.hwndFileTypeCB, lpszTemp)))
        CBSetCurSel(fodInfos->DlgInfos.hwndFileTypeCB, iPos);
      }
      /* fall through */
    case ONOPEN_BROWSE:   /* browse to the highest folder we could bind to */
      TRACE("ONOPEN_BROWSE\n");
      {
	IPersistFolder2 * ppf2;
        if(SUCCEEDED(IShellFolder_QueryInterface( lpsf, &IID_IPersistFolder2, (LPVOID*)&ppf2)))
        {
          LPITEMIDLIST pidlCurrent;
          IPersistFolder2_GetCurFolder(ppf2, &pidlCurrent);
          IPersistFolder2_Release(ppf2);
	  if( ! COMDLG32_PIDL_ILIsEqual(pidlCurrent, fodInfos->ShellInfos.pidlAbsCurrent))
	  {
            if (SUCCEEDED(IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser, pidlCurrent, SBSP_ABSOLUTE))
                && fodInfos->ofnInfos->Flags & OFN_EXPLORER)
            {
              SendCustomDlgNotificationMessage(hwnd, CDN_FOLDERCHANGE);
            }
	  }
	  else if( nOpenAction == ONOPEN_SEARCH )
	  {
            if (fodInfos->Shell.FOIShellView)
              IShellView_Refresh(fodInfos->Shell.FOIShellView);
	  }
          COMDLG32_SHFree(pidlCurrent);
          if (filename_is_edit( fodInfos ))
              SendMessageW(fodInfos->DlgInfos.hwndFileName, EM_SETSEL, 0, -1);
        }
      }
      ret = FALSE;
      break;
    case ONOPEN_OPEN:   /* fill in the return struct and close the dialog */
      TRACE("ONOPEN_OPEN %s\n", debugstr_w(lpstrPathAndFile));
      {
        WCHAR *ext = NULL;

        /* update READONLY check box flag */
	if ((SendMessageW(GetDlgItem(hwnd,IDC_OPENREADONLY),BM_GETCHECK,0,0) & 0x03) == BST_CHECKED)
	  fodInfos->ofnInfos->Flags |= OFN_READONLY;
	else
	  fodInfos->ofnInfos->Flags &= ~OFN_READONLY;

        /* Attach the file extension with file name*/
        ext = PathFindExtensionW(lpstrPathAndFile);
        if (! *ext && fodInfos->defext)
        {
            /* if no extension is specified with file name, then */
            /* attach the extension from file filter or default one */
            
            WCHAR *filterExt = NULL;
            LPWSTR lpstrFilter = NULL;
            static const WCHAR szwDot[] = {'.',0};
            int PathLength = lstrlenW(lpstrPathAndFile);

            /*Get the file extension from file type filter*/
            lpstrFilter = (LPWSTR) CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,
                                             fodInfos->ofnInfos->nFilterIndex-1);

            if (lpstrFilter != (LPWSTR)CB_ERR)  /* control is not empty */
            {
                WCHAR* filterSearchIndex;
                filterExt = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(lpstrFilter) + 1) * sizeof(WCHAR));
                strcpyW(filterExt, lpstrFilter);

                /* if a semicolon-separated list of file extensions was given, do not include the
                   semicolon or anything after it in the extension.
                   example: if filterExt was "*.abc;*.def", it will become "*.abc" */
                filterSearchIndex = strchrW(filterExt, ';');
                if (filterSearchIndex)
                {
                    filterSearchIndex[0] = '\0';
                }

                /* strip the * or anything else from the extension, "*.abc" becomes "abc" */
                /* if the extension is invalid or contains a glob, ignore it */
                filterSearchIndex = PathFindExtensionW(filterExt);
                if (*filterSearchIndex++ && !strchrW(filterSearchIndex, '*') && !strchrW(filterSearchIndex, '?'))
                {
                    strcpyW(filterExt, filterSearchIndex);
                }
                else
                {
                    HeapFree(GetProcessHeap(), 0, filterExt);
                    filterExt = NULL;
                }
            }

            if (!filterExt)
            {
                /* use the default file extension */
                filterExt = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(fodInfos->defext) + 1) * sizeof(WCHAR));
                strcpyW(filterExt, fodInfos->defext);
            }

            if (*filterExt) /* ignore filterExt="" */
            {
                /* Attach the dot*/
                lstrcatW(lpstrPathAndFile, szwDot);
                /* Attach the extension */
                lstrcatW(lpstrPathAndFile, filterExt);
            }

            HeapFree(GetProcessHeap(), 0, filterExt);

            /* In Open dialog: if file does not exist try without extension */
            if (!(fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG) && !PathFileExistsW(lpstrPathAndFile))
                  lpstrPathAndFile[PathLength] = '\0';

            /* Set/clear the output OFN_EXTENSIONDIFFERENT flag */
            if (*ext)
                ext++;
            if (!lstrcmpiW(fodInfos->defext, ext))
                fodInfos->ofnInfos->Flags &= ~OFN_EXTENSIONDIFFERENT;
            else
                fodInfos->ofnInfos->Flags |= OFN_EXTENSIONDIFFERENT;
	}

	/* In Save dialog: check if the file already exists */
	if (fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG
	    && fodInfos->ofnInfos->Flags & OFN_OVERWRITEPROMPT
	    && PathFileExistsW(lpstrPathAndFile))
	{
	  WCHAR lpstrOverwrite[100];
	  int answer;

	  LoadStringW(COMDLG32_hInstance, IDS_OVERWRITEFILE, lpstrOverwrite, 100);
	  answer = MessageBoxW(hwnd, lpstrOverwrite, fodInfos->title,
			       MB_YESNO | MB_ICONEXCLAMATION);
	  if (answer == IDNO || answer == IDCANCEL)
	  {
	    ret = FALSE;
	    goto ret;
	  }
	}

        /* In Open dialog: check if it should be created if it doesn't exist */
        if (!(fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG)
            && fodInfos->ofnInfos->Flags & OFN_CREATEPROMPT
            && !PathFileExistsW(lpstrPathAndFile))
        {
          WCHAR lpstrCreate[100];
          int answer;

          LoadStringW(COMDLG32_hInstance, IDS_CREATEFILE, lpstrCreate, 100);
          answer = MessageBoxW(hwnd, lpstrCreate, fodInfos->title,
                               MB_YESNO | MB_ICONEXCLAMATION);
          if (answer == IDNO || answer == IDCANCEL)
          {
            ret = FALSE;
            goto ret;
          }
        }

        /* Check that the size of the file does not exceed buffer size.
             (Allow for extra \0 if OFN_MULTISELECT is set.) */
        if(lstrlenW(lpstrPathAndFile) < fodInfos->ofnInfos->nMaxFile -
            ((fodInfos->ofnInfos->Flags & OFN_ALLOWMULTISELECT) ? 1 : 0))
        {

          /* fill destination buffer */
          if (fodInfos->ofnInfos->lpstrFile)
          {
             if(fodInfos->unicode)
             {
               LPOPENFILENAMEW ofn = fodInfos->ofnInfos;

               lstrcpynW(ofn->lpstrFile, lpstrPathAndFile, ofn->nMaxFile);
               if (ofn->Flags & OFN_ALLOWMULTISELECT)
                 ofn->lpstrFile[lstrlenW(ofn->lpstrFile) + 1] = '\0';
             }
             else
             {
               LPOPENFILENAMEA ofn = (LPOPENFILENAMEA)fodInfos->ofnInfos;

               WideCharToMultiByte(CP_ACP, 0, lpstrPathAndFile, -1,
                                   ofn->lpstrFile, ofn->nMaxFile, NULL, NULL);
               if (ofn->Flags & OFN_ALLOWMULTISELECT)
                 ofn->lpstrFile[lstrlenA(ofn->lpstrFile) + 1] = '\0';
             }
          }

          if(fodInfos->unicode)
          {
              LPWSTR lpszTemp;

              /* set filename offset */
              lpszTemp = PathFindFileNameW(lpstrPathAndFile);
              fodInfos->ofnInfos->nFileOffset = (lpszTemp - lpstrPathAndFile);

              /* set extension offset */
              lpszTemp = PathFindExtensionW(lpstrPathAndFile);
              fodInfos->ofnInfos->nFileExtension = (*lpszTemp) ? (lpszTemp - lpstrPathAndFile) + 1 : 0;
          }
          else
          {
              LPSTR lpszTemp;
              CHAR tempFileA[MAX_PATH];

              /* avoid using fodInfos->ofnInfos->lpstrFile since it can be NULL */
              WideCharToMultiByte(CP_ACP, 0, lpstrPathAndFile, -1,
                                  tempFileA, sizeof(tempFileA), NULL, NULL);

              /* set filename offset */
              lpszTemp = PathFindFileNameA(tempFileA);
              fodInfos->ofnInfos->nFileOffset = (lpszTemp - tempFileA);

              /* set extension offset */
              lpszTemp = PathFindExtensionA(tempFileA);
              fodInfos->ofnInfos->nFileExtension = (*lpszTemp) ? (lpszTemp - tempFileA) + 1 : 0;
          }

          /* set the lpstrFileTitle */
          if(fodInfos->ofnInfos->lpstrFileTitle)
	  {
            LPWSTR lpstrFileTitle = PathFindFileNameW(lpstrPathAndFile);
            if(fodInfos->unicode)
            {
              LPOPENFILENAMEW ofn = fodInfos->ofnInfos;
	      lstrcpynW(ofn->lpstrFileTitle, lpstrFileTitle, ofn->nMaxFileTitle);
            }
            else
            {
              LPOPENFILENAMEA ofn = (LPOPENFILENAMEA)fodInfos->ofnInfos;
              WideCharToMultiByte(CP_ACP, 0, lpstrFileTitle, -1,
                    ofn->lpstrFileTitle, ofn->nMaxFileTitle, NULL, NULL);
            }
	  }

          /* copy currently selected filter to lpstrCustomFilter */
          if (fodInfos->ofnInfos->lpstrCustomFilter)
          {
            LPOPENFILENAMEA ofn = (LPOPENFILENAMEA)fodInfos->ofnInfos;
            int len = WideCharToMultiByte(CP_ACP, 0, fodInfos->ShellInfos.lpstrCurrentFilter, -1,
                                          NULL, 0, NULL, NULL);
            if (len + strlen(ofn->lpstrCustomFilter) + 1 <= ofn->nMaxCustFilter)
            {
              LPSTR s = ofn->lpstrCustomFilter;
              s += strlen(ofn->lpstrCustomFilter)+1;
              WideCharToMultiByte(CP_ACP, 0, fodInfos->ShellInfos.lpstrCurrentFilter, -1,
                                  s, len, NULL, NULL);
            }
          }


          if ( !FILEDLG95_SendFileOK(hwnd, fodInfos) )
	      goto ret;

          FILEDLG95_MRU_save_filename(lpstrPathAndFile);

          TRACE("close\n");
	  FILEDLG95_Clean(hwnd);
          ret = EndDialog(hwnd, TRUE);
	}
	else
        {
          WORD size;

          size = lstrlenW(lpstrPathAndFile) + 1;
          if (fodInfos->ofnInfos->Flags & OFN_ALLOWMULTISELECT)
             size += 1;
          /* return needed size in first two bytes of lpstrFile */
          if(fodInfos->ofnInfos->lpstrFile)
              *(WORD *)fodInfos->ofnInfos->lpstrFile = size;
          FILEDLG95_Clean(hwnd);
          ret = EndDialog(hwnd, FALSE);
          COMDLG32_SetCommDlgExtendedError(FNERR_BUFFERTOOSMALL);
        }
      }
      break;
  }

ret:
  if(lpsf) IShellFolder_Release(lpsf);
  return ret;
}

/***********************************************************************
 *      FILEDLG95_SHELL_Init
 *
 * Initialisation of the shell objects
 */
static LRESULT FILEDLG95_SHELL_Init(HWND hwnd)
{
  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  /*
   * Initialisation of the FileOpenDialogInfos structure
   */

  /* Shell */

  /*ShellInfos */
  fodInfos->ShellInfos.hwndOwner = hwnd;

  /* Disable multi-select if flag not set */
  if (!(fodInfos->ofnInfos->Flags & OFN_ALLOWMULTISELECT))
  {
     fodInfos->ShellInfos.folderSettings.fFlags |= FWF_SINGLESEL;
  }
  fodInfos->ShellInfos.folderSettings.fFlags |= FWF_AUTOARRANGE | FWF_ALIGNLEFT;
  fodInfos->ShellInfos.folderSettings.ViewMode = FVM_LIST;

  /* Construct the IShellBrowser interface */
  fodInfos->Shell.FOIShellBrowser = IShellBrowserImpl_Construct(hwnd);

  return NOERROR;
}

/***********************************************************************
 *      FILEDLG95_SHELL_ExecuteCommand
 *
 * Change the folder option and refresh the view
 * If the function succeeds, the return value is nonzero.
 */
static BOOL FILEDLG95_SHELL_ExecuteCommand(HWND hwnd, LPCSTR lpVerb)
{
  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);
  IContextMenu * pcm;

  TRACE("(%p,%p)\n", hwnd, lpVerb);

  if(SUCCEEDED(IShellView_GetItemObject(fodInfos->Shell.FOIShellView,
					SVGIO_BACKGROUND,
					&IID_IContextMenu,
					(LPVOID*)&pcm)))
  {
    CMINVOKECOMMANDINFO ci;
    ZeroMemory(&ci, sizeof(CMINVOKECOMMANDINFO));
    ci.cbSize = sizeof(CMINVOKECOMMANDINFO);
    ci.lpVerb = lpVerb;
    ci.hwnd = hwnd;

    IContextMenu_InvokeCommand(pcm, &ci);
    IContextMenu_Release(pcm);
  }

  return FALSE;
}

/***********************************************************************
 *      FILEDLG95_SHELL_UpFolder
 *
 * Browse to the specified object
 * If the function succeeds, the return value is nonzero.
 */
static BOOL FILEDLG95_SHELL_UpFolder(HWND hwnd)
{
  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  if(SUCCEEDED(IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser,
                                          NULL,
                                          SBSP_PARENT)))
  {
    if(fodInfos->ofnInfos->Flags & OFN_EXPLORER)
        SendCustomDlgNotificationMessage(hwnd, CDN_FOLDERCHANGE);
    return TRUE;
  }
  return FALSE;
}

/***********************************************************************
 *      FILEDLG95_SHELL_BrowseToDesktop
 *
 * Browse to the Desktop
 * If the function succeeds, the return value is nonzero.
 */
static BOOL FILEDLG95_SHELL_BrowseToDesktop(HWND hwnd)
{
  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);
  LPITEMIDLIST pidl;
  HRESULT hres;

  TRACE("\n");

  SHGetSpecialFolderLocation(0,CSIDL_DESKTOP,&pidl);
  hres = IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser, pidl, SBSP_ABSOLUTE);
  if(fodInfos->ofnInfos->Flags & OFN_EXPLORER)
      SendCustomDlgNotificationMessage(hwnd, CDN_FOLDERCHANGE);
  COMDLG32_SHFree(pidl);
  return SUCCEEDED(hres);
}
/***********************************************************************
 *      FILEDLG95_SHELL_Clean
 *
 * Cleans the memory used by shell objects
 */
static void FILEDLG95_SHELL_Clean(HWND hwnd)
{
    FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);

    TRACE("\n");

    COMDLG32_SHFree(fodInfos->ShellInfos.pidlAbsCurrent);

    /* clean Shell interfaces */
    if (fodInfos->Shell.FOIShellView)
    {
      IShellView_DestroyViewWindow(fodInfos->Shell.FOIShellView);
      IShellView_Release(fodInfos->Shell.FOIShellView);
    }
    IShellFolder_Release(fodInfos->Shell.FOIShellFolder);
    IShellBrowser_Release(fodInfos->Shell.FOIShellBrowser);
    if (fodInfos->Shell.FOIDataObject)
      IDataObject_Release(fodInfos->Shell.FOIDataObject);
}

/***********************************************************************
 *      FILEDLG95_FILETYPE_Init
 *
 * Initialisation of the file type combo box
 */
static HRESULT FILEDLG95_FILETYPE_Init(HWND hwnd)
{
  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);
  int nFilters = 0;  /* number of filters */
  int nFilterIndexCB;

  TRACE("\n");

  if(fodInfos->customfilter)
  {
      /* customfilter has one entry...  title\0ext\0
       * Set first entry of combo box item with customfilter
       */
      LPWSTR  lpstrExt;
      LPCWSTR lpstrPos = fodInfos->customfilter;

      /* Get the title */
      lpstrPos += lstrlenW(fodInfos->customfilter) + 1;

      /* Copy the extensions */
      if (! *lpstrPos) return E_FAIL;	/* malformed filter */
      if (!(lpstrExt = MemAlloc((lstrlenW(lpstrPos)+1)*sizeof(WCHAR)))) return E_FAIL;
      lstrcpyW(lpstrExt,lpstrPos);

      /* Add the item at the end of the combo */
      CBAddString(fodInfos->DlgInfos.hwndFileTypeCB, fodInfos->customfilter);
      CBSetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB, nFilters, lpstrExt);
      nFilters++;
  }
  if(fodInfos->filter)
  {
    LPCWSTR lpstrPos = fodInfos->filter;

    for(;;)
    {
      /* filter is a list...  title\0ext\0......\0\0
       * Set the combo item text to the title and the item data
       *  to the ext
       */
      LPCWSTR lpstrDisplay;
      LPWSTR lpstrExt;

      /* Get the title */
      if(! *lpstrPos) break;	/* end */
      lpstrDisplay = lpstrPos;
      lpstrPos += lstrlenW(lpstrPos) + 1;

      CBAddString(fodInfos->DlgInfos.hwndFileTypeCB, lpstrDisplay);

      nFilters++;

      /* Copy the extensions */
      if (!(lpstrExt = MemAlloc((lstrlenW(lpstrPos)+1)*sizeof(WCHAR)))) return E_FAIL;
      lstrcpyW(lpstrExt,lpstrPos);
      lpstrPos += lstrlenW(lpstrPos) + 1;

      /* Add the item at the end of the combo */
      CBSetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB, nFilters-1, lpstrExt);

      /* malformed filters are added anyway... */
      if (!*lpstrExt) break;
    }
  }

  /*
   * Set the current filter to the one specified
   * in the initialisation structure
   */
  if (fodInfos->filter || fodInfos->customfilter)
  {
    LPWSTR lpstrFilter;

    /* Check to make sure our index isn't out of bounds. */
    if ( fodInfos->ofnInfos->nFilterIndex >
         nFilters - (fodInfos->customfilter == NULL ? 0 : 1) )
      fodInfos->ofnInfos->nFilterIndex = (fodInfos->customfilter == NULL ? 1 : 0);

    /* set default filter index */
    if(fodInfos->ofnInfos->nFilterIndex == 0 && fodInfos->customfilter == NULL)
      fodInfos->ofnInfos->nFilterIndex = 1;

    /* calculate index of Combo Box item */
    nFilterIndexCB = fodInfos->ofnInfos->nFilterIndex;
    if (fodInfos->customfilter == NULL)
      nFilterIndexCB--;

    /* Set the current index selection. */
    CBSetCurSel(fodInfos->DlgInfos.hwndFileTypeCB, nFilterIndexCB);

    /* Get the corresponding text string from the combo box. */
    lpstrFilter = (LPWSTR) CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,
                                             nFilterIndexCB);

    if ((INT_PTR)lpstrFilter == CB_ERR)  /* control is empty */
      lpstrFilter = NULL;

    if(lpstrFilter)
    {
      DWORD len;
      CharLowerW(lpstrFilter); /* lowercase */
      len = lstrlenW(lpstrFilter)+1;
      fodInfos->ShellInfos.lpstrCurrentFilter = MemAlloc( len * sizeof(WCHAR) );
      lstrcpyW(fodInfos->ShellInfos.lpstrCurrentFilter,lpstrFilter);
    }
  } else
      fodInfos->ofnInfos->nFilterIndex = 0;
  return S_OK;
}

/***********************************************************************
 *      FILEDLG95_FILETYPE_OnCommand
 *
 * WM_COMMAND of the file type combo box
 * If the function succeeds, the return value is nonzero.
 */
static BOOL FILEDLG95_FILETYPE_OnCommand(HWND hwnd, WORD wNotifyCode)
{
  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);

  switch(wNotifyCode)
  {
    case CBN_SELENDOK:
    {
      LPWSTR lpstrFilter;

      /* Get the current item of the filetype combo box */
      int iItem = CBGetCurSel(fodInfos->DlgInfos.hwndFileTypeCB);

      /* set the current filter index */
      fodInfos->ofnInfos->nFilterIndex = iItem +
        (fodInfos->customfilter == NULL ? 1 : 0);

      /* Set the current filter with the current selection */
      MemFree(fodInfos->ShellInfos.lpstrCurrentFilter);

      lpstrFilter = (LPWSTR) CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,
                                             iItem);
      if((INT_PTR)lpstrFilter != CB_ERR)
      {
          DWORD len;
          CharLowerW(lpstrFilter); /* lowercase */
          len = lstrlenW(lpstrFilter)+1;
          fodInfos->ShellInfos.lpstrCurrentFilter = MemAlloc( len * sizeof(WCHAR) );
          lstrcpyW(fodInfos->ShellInfos.lpstrCurrentFilter,lpstrFilter);
          if(fodInfos->ofnInfos->Flags & OFN_EXPLORER)
              SendCustomDlgNotificationMessage(hwnd,CDN_TYPECHANGE);
      }

      /* Refresh the actual view to display the included items*/
      if (fodInfos->Shell.FOIShellView)
        IShellView_Refresh(fodInfos->Shell.FOIShellView);
    }
  }
  return FALSE;
}
/***********************************************************************
 *      FILEDLG95_FILETYPE_SearchExt
 *
 * searches for an extension in the filetype box
 */
static int FILEDLG95_FILETYPE_SearchExt(HWND hwnd,LPCWSTR lpstrExt)
{
  int i, iCount = CBGetCount(hwnd);

  TRACE("%s\n", debugstr_w(lpstrExt));

  if(iCount != CB_ERR)
  {
    for(i=0;i<iCount;i++)
    {
      if(!lstrcmpiW(lpstrExt,(LPWSTR)CBGetItemDataPtr(hwnd,i)))
          return i;
    }
  }
  return -1;
}

/***********************************************************************
 *      FILEDLG95_FILETYPE_Clean
 *
 * Clean the memory used by the filetype combo box
 */
static void FILEDLG95_FILETYPE_Clean(HWND hwnd)
{
  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);
  int iPos;
  int iCount = CBGetCount(fodInfos->DlgInfos.hwndFileTypeCB);

  TRACE("\n");

  /* Delete each string of the combo and their associated data */
  if(iCount != CB_ERR)
  {
    for(iPos = iCount-1;iPos>=0;iPos--)
    {
      MemFree((LPSTR) CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,iPos));
      CBDeleteString(fodInfos->DlgInfos.hwndFileTypeCB,iPos);
    }
  }
  /* Current filter */
  MemFree(fodInfos->ShellInfos.lpstrCurrentFilter);

}

/***********************************************************************
 *      FILEDLG95_LOOKIN_Init
 *
 * Initialisation of the look in combo box
 */

/* Small helper function, to determine if the unixfs shell extension is rooted 
 * at the desktop. Copied from dlls/shell32/shfldr_unixfs.c. 
 */
static inline BOOL FILEDLG95_unixfs_is_rooted_at_desktop(void) {
    HKEY hKey;
    static const WCHAR wszRootedAtDesktop[] = { 'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'E','x','p','l','o','r','e','r','\\','D','e','s','k','t','o','p','\\',
        'N','a','m','e','S','p','a','c','e','\\','{','9','D','2','0','A','A','E','8',
        '-','0','6','2','5','-','4','4','B','0','-','9','C','A','7','-',
        '7','1','8','8','9','C','2','2','5','4','D','9','}',0 };
    
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszRootedAtDesktop, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return FALSE;
        
    RegCloseKey(hKey);
    return TRUE;
}

static void FILEDLG95_LOOKIN_Init(HWND hwndCombo)
{
  IShellFolder	*psfRoot, *psfDrives;
  IEnumIDList	*lpeRoot, *lpeDrives;
  LPITEMIDLIST	pidlDrives, pidlTmp, pidlTmp1, pidlAbsTmp;
  HDC hdc;
  TEXTMETRICW tm;
  LookInInfos *liInfos = MemAlloc(sizeof(LookInInfos));

  TRACE("\n");

  liInfos->iMaxIndentation = 0;

  SetPropA(hwndCombo, LookInInfosStr, liInfos);

  hdc = GetDC( hwndCombo );
  SelectObject( hdc, (HFONT)SendMessageW( hwndCombo, WM_GETFONT, 0, 0 ));
  GetTextMetricsW( hdc, &tm );
  ReleaseDC( hwndCombo, hdc );

  /* set item height for both text field and listbox */
  CBSetItemHeight( hwndCombo, -1, max( tm.tmHeight, GetSystemMetrics(SM_CYSMICON) ));
  CBSetItemHeight( hwndCombo, 0, max( tm.tmHeight, GetSystemMetrics(SM_CYSMICON) ));

  /* Turn on the extended UI for the combo box like Windows does */
  CBSetExtendedUI(hwndCombo, TRUE);

  /* Initialise data of Desktop folder */
  SHGetSpecialFolderLocation(0,CSIDL_DESKTOP,&pidlTmp);
  FILEDLG95_LOOKIN_AddItem(hwndCombo, pidlTmp,LISTEND);
  COMDLG32_SHFree(pidlTmp);

  SHGetSpecialFolderLocation(0,CSIDL_DRIVES,&pidlDrives);

  SHGetDesktopFolder(&psfRoot);

  if (psfRoot)
  {
    /* enumerate the contents of the desktop */
    if(SUCCEEDED(IShellFolder_EnumObjects(psfRoot, hwndCombo, SHCONTF_FOLDERS, &lpeRoot)))
    {
      while (S_OK == IEnumIDList_Next(lpeRoot, 1, &pidlTmp, NULL))
      {
	FILEDLG95_LOOKIN_AddItem(hwndCombo, pidlTmp,LISTEND);

	/* If the unixfs extension is rooted, we don't expand the drives by default */
	if (!FILEDLG95_unixfs_is_rooted_at_desktop()) 
	{
	  /* special handling for CSIDL_DRIVES */
	  if (COMDLG32_PIDL_ILIsEqual(pidlTmp, pidlDrives))
	  {
	    if(SUCCEEDED(IShellFolder_BindToObject(psfRoot, pidlTmp, NULL, &IID_IShellFolder, (LPVOID*)&psfDrives)))
	    {
	      /* enumerate the drives */
	      if(SUCCEEDED(IShellFolder_EnumObjects(psfDrives, hwndCombo,SHCONTF_FOLDERS, &lpeDrives)))
	      {
	        while (S_OK == IEnumIDList_Next(lpeDrives, 1, &pidlTmp1, NULL))
	        {
	          pidlAbsTmp = COMDLG32_PIDL_ILCombine(pidlTmp, pidlTmp1);
	          FILEDLG95_LOOKIN_AddItem(hwndCombo, pidlAbsTmp,LISTEND);
	          COMDLG32_SHFree(pidlAbsTmp);
	          COMDLG32_SHFree(pidlTmp1);
	        }
	        IEnumIDList_Release(lpeDrives);
	      }
	      IShellFolder_Release(psfDrives);
	    }
	  }
	}

        COMDLG32_SHFree(pidlTmp);
      }
      IEnumIDList_Release(lpeRoot);
    }
    IShellFolder_Release(psfRoot);
  }

  COMDLG32_SHFree(pidlDrives);
}

/***********************************************************************
 *      FILEDLG95_LOOKIN_DrawItem
 *
 * WM_DRAWITEM message handler
 */
static LRESULT FILEDLG95_LOOKIN_DrawItem(LPDRAWITEMSTRUCT pDIStruct)
{
  COLORREF crWin = GetSysColor(COLOR_WINDOW);
  COLORREF crHighLight = GetSysColor(COLOR_HIGHLIGHT);
  COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);
  RECT rectText;
  RECT rectIcon;
  SHFILEINFOW sfi;
  HIMAGELIST ilItemImage;
  int iIndentation;
  TEXTMETRICW tm;
  LPSFOLDER tmpFolder;
  UINT shgfi_flags = SHGFI_PIDL | SHGFI_OPENICON | SHGFI_SYSICONINDEX | SHGFI_DISPLAYNAME;
  UINT icon_width, icon_height;

  TRACE("\n");

  if(pDIStruct->itemID == -1)
    return 0;

  if(!(tmpFolder = (LPSFOLDER) CBGetItemDataPtr(pDIStruct->hwndItem,
                            pDIStruct->itemID)))
    return 0;


  icon_width = GetSystemMetrics(SM_CXICON);
  icon_height = GetSystemMetrics(SM_CYICON);
  if (pDIStruct->rcItem.bottom - pDIStruct->rcItem.top < icon_height)
  {
      icon_width = GetSystemMetrics(SM_CXSMICON);
      icon_height = GetSystemMetrics(SM_CYSMICON);
      shgfi_flags |= SHGFI_SMALLICON;
  }

  ilItemImage = (HIMAGELIST) SHGetFileInfoW ((LPCWSTR) tmpFolder->pidlItem,
                                             0, &sfi, sizeof (sfi), shgfi_flags );

  /* Is this item selected ? */
  if(pDIStruct->itemState & ODS_SELECTED)
  {
    SetTextColor(pDIStruct->hDC,(0x00FFFFFF & ~(crText)));
    SetBkColor(pDIStruct->hDC,crHighLight);
    FillRect(pDIStruct->hDC,&pDIStruct->rcItem,GetSysColorBrush(COLOR_HIGHLIGHT));
  }
  else
  {
    SetTextColor(pDIStruct->hDC,crText);
    SetBkColor(pDIStruct->hDC,crWin);
    FillRect(pDIStruct->hDC,&pDIStruct->rcItem,GetSysColorBrush(COLOR_WINDOW));
  }

  /* Do not indent item if drawing in the edit of the combo */
  if(pDIStruct->itemState & ODS_COMBOBOXEDIT)
    iIndentation = 0;
  else
    iIndentation = tmpFolder->m_iIndent;

  /* Draw text and icon */

  /* Initialise the icon display area */
  rectIcon.left = pDIStruct->rcItem.left + 1 + icon_width/2 * iIndentation;
  rectIcon.top = (pDIStruct->rcItem.top + pDIStruct->rcItem.bottom - icon_height) / 2;
  rectIcon.right = rectIcon.left + icon_width + XTEXTOFFSET;
  rectIcon.bottom = (pDIStruct->rcItem.top + pDIStruct->rcItem.bottom + icon_height) / 2;

  /* Initialise the text display area */
  GetTextMetricsW(pDIStruct->hDC, &tm);
  rectText.left = rectIcon.right;
  rectText.top =
	  (pDIStruct->rcItem.top + pDIStruct->rcItem.bottom - tm.tmHeight) / 2;
  rectText.right = pDIStruct->rcItem.right;
  rectText.bottom =
	  (pDIStruct->rcItem.top + pDIStruct->rcItem.bottom + tm.tmHeight) / 2;

  /* Draw the icon from the image list */
  ImageList_Draw(ilItemImage,
                 sfi.iIcon,
                 pDIStruct->hDC,
                 rectIcon.left,
                 rectIcon.top,
                 ILD_TRANSPARENT );

  /* Draw the associated text */
  TextOutW(pDIStruct->hDC,rectText.left,rectText.top,sfi.szDisplayName,lstrlenW(sfi.szDisplayName));
  return NOERROR;
}

/***********************************************************************
 *      FILEDLG95_LOOKIN_OnCommand
 *
 * LookIn combo box WM_COMMAND message handler
 * If the function succeeds, the return value is nonzero.
 */
static BOOL FILEDLG95_LOOKIN_OnCommand(HWND hwnd, WORD wNotifyCode)
{
  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("%p\n", fodInfos);

  switch(wNotifyCode)
  {
    case CBN_SELENDOK:
    {
      LPSFOLDER tmpFolder;
      int iItem;

      iItem = CBGetCurSel(fodInfos->DlgInfos.hwndLookInCB);

      if( iItem == CB_ERR) return FALSE;

      if(!(tmpFolder = (LPSFOLDER) CBGetItemDataPtr(fodInfos->DlgInfos.hwndLookInCB,
                                               iItem)))
	return FALSE;


      if(SUCCEEDED(IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser,
                                              tmpFolder->pidlItem,
                                              SBSP_ABSOLUTE)))
      {
        if(fodInfos->ofnInfos->Flags & OFN_EXPLORER)
            SendCustomDlgNotificationMessage(hwnd, CDN_FOLDERCHANGE);
        return TRUE;
      }
      break;
    }

  }
  return FALSE;
}

/***********************************************************************
 *      FILEDLG95_LOOKIN_AddItem
 *
 * Adds an absolute pidl item to the lookin combo box
 * returns the index of the inserted item
 */
static int FILEDLG95_LOOKIN_AddItem(HWND hwnd,LPITEMIDLIST pidl, int iInsertId)
{
  LPITEMIDLIST pidlNext;
  SHFILEINFOW sfi;
  SFOLDER *tmpFolder;
  LookInInfos *liInfos;

  TRACE("%08x\n", iInsertId);

  if(!pidl)
    return -1;

  if(!(liInfos = GetPropA(hwnd,LookInInfosStr)))
    return -1;

  tmpFolder = MemAlloc(sizeof(SFOLDER));
  tmpFolder->m_iIndent = 0;

  /* Calculate the indentation of the item in the lookin*/
  pidlNext = pidl;
  while( (pidlNext=COMDLG32_PIDL_ILGetNext(pidlNext)) )
  {
    tmpFolder->m_iIndent++;
  }

  tmpFolder->pidlItem = COMDLG32_PIDL_ILClone(pidl);

  if(tmpFolder->m_iIndent > liInfos->iMaxIndentation)
    liInfos->iMaxIndentation = tmpFolder->m_iIndent;

  sfi.dwAttributes = SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM;
  SHGetFileInfoW((LPCWSTR)pidl,
                  0,
                  &sfi,
                  sizeof(sfi),
                  SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX
                  | SHGFI_PIDL | SHGFI_SMALLICON | SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED);

  TRACE("-- Add %s attr=%08x\n", debugstr_w(sfi.szDisplayName), sfi.dwAttributes);

  if((sfi.dwAttributes & SFGAO_FILESYSANCESTOR) || (sfi.dwAttributes & SFGAO_FILESYSTEM))
  {
    int iItemID;

    TRACE("-- Add %s at %u\n", debugstr_w(sfi.szDisplayName), tmpFolder->m_iIndent);

    /* Add the item at the end of the list */
    if(iInsertId < 0)
    {
      iItemID = CBAddString(hwnd,sfi.szDisplayName);
    }
    /* Insert the item at the iInsertId position*/
    else
    {
      iItemID = CBInsertString(hwnd,sfi.szDisplayName,iInsertId);
    }

    CBSetItemDataPtr(hwnd,iItemID,tmpFolder);
    return iItemID;
  }

  COMDLG32_SHFree( tmpFolder->pidlItem );
  MemFree( tmpFolder );
  return -1;

}

/***********************************************************************
 *      FILEDLG95_LOOKIN_InsertItemAfterParent
 *
 * Insert an item below its parent
 */
static int FILEDLG95_LOOKIN_InsertItemAfterParent(HWND hwnd,LPITEMIDLIST pidl)
{

  LPITEMIDLIST pidlParent = GetParentPidl(pidl);
  int iParentPos;

  TRACE("\n");

  if (pidl == pidlParent)
    return -1;

  iParentPos = FILEDLG95_LOOKIN_SearchItem(hwnd,(WPARAM)pidlParent,SEARCH_PIDL);

  if(iParentPos < 0)
  {
    iParentPos = FILEDLG95_LOOKIN_InsertItemAfterParent(hwnd,pidlParent);
  }

  /* Free pidlParent memory */
  COMDLG32_SHFree(pidlParent);

  return FILEDLG95_LOOKIN_AddItem(hwnd,pidl,iParentPos + 1);
}

/***********************************************************************
 *      FILEDLG95_LOOKIN_SelectItem
 *
 * Adds an absolute pidl item to the lookin combo box
 * returns the index of the inserted item
 */
int FILEDLG95_LOOKIN_SelectItem(HWND hwnd,LPITEMIDLIST pidl)
{
  int iItemPos;
  LookInInfos *liInfos;

  TRACE("\n");

  iItemPos = FILEDLG95_LOOKIN_SearchItem(hwnd,(WPARAM)pidl,SEARCH_PIDL);

  liInfos = GetPropA(hwnd,LookInInfosStr);

  if(iItemPos < 0)
  {
    while(FILEDLG95_LOOKIN_RemoveMostExpandedItem(hwnd) > -1);
    iItemPos = FILEDLG95_LOOKIN_InsertItemAfterParent(hwnd,pidl);
  }

  else
  {
    SFOLDER *tmpFolder = (LPSFOLDER) CBGetItemDataPtr(hwnd,iItemPos);
    while(liInfos->iMaxIndentation > tmpFolder->m_iIndent)
    {
      int iRemovedItem;

      if(-1 == (iRemovedItem = FILEDLG95_LOOKIN_RemoveMostExpandedItem(hwnd)))
        break;
      if(iRemovedItem < iItemPos)
        iItemPos--;
    }
  }

  CBSetCurSel(hwnd,iItemPos);
  liInfos->uSelectedItem = iItemPos;

  return 0;

}

/***********************************************************************
 *      FILEDLG95_LOOKIN_RemoveMostExpandedItem
 *
 * Remove the item with an expansion level over iExpansionLevel
 */
static int FILEDLG95_LOOKIN_RemoveMostExpandedItem(HWND hwnd)
{
  int iItemPos;
  LookInInfos *liInfos = GetPropA(hwnd,LookInInfosStr);

  TRACE("\n");

  if(liInfos->iMaxIndentation <= 2)
    return -1;

  if((iItemPos = FILEDLG95_LOOKIN_SearchItem(hwnd,liInfos->iMaxIndentation,SEARCH_EXP)) >=0)
  {
    SFOLDER *tmpFolder = (LPSFOLDER) CBGetItemDataPtr(hwnd,iItemPos);
    COMDLG32_SHFree(tmpFolder->pidlItem);
    MemFree(tmpFolder);
    CBDeleteString(hwnd,iItemPos);
    liInfos->iMaxIndentation--;

    return iItemPos;
  }

  return -1;
}

/***********************************************************************
 *      FILEDLG95_LOOKIN_SearchItem
 *
 * Search for pidl in the lookin combo box
 * returns the index of the found item
 */
static int FILEDLG95_LOOKIN_SearchItem(HWND hwnd,WPARAM searchArg,int iSearchMethod)
{
  int i = 0;
  int iCount = CBGetCount(hwnd);

  TRACE("0x%08lx 0x%x\n",searchArg, iSearchMethod);

  if (iCount != CB_ERR)
  {
    for(;i<iCount;i++)
    {
      LPSFOLDER tmpFolder = (LPSFOLDER) CBGetItemDataPtr(hwnd,i);

      if(iSearchMethod == SEARCH_PIDL && COMDLG32_PIDL_ILIsEqual((LPITEMIDLIST)searchArg,tmpFolder->pidlItem))
        return i;
      if(iSearchMethod == SEARCH_EXP && tmpFolder->m_iIndent == (int)searchArg)
        return i;
    }
  }

  return -1;
}

/***********************************************************************
 *      FILEDLG95_LOOKIN_Clean
 *
 * Clean the memory used by the lookin combo box
 */
static void FILEDLG95_LOOKIN_Clean(HWND hwnd)
{
    FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);
    LookInInfos *liInfos = GetPropA(fodInfos->DlgInfos.hwndLookInCB,LookInInfosStr);
    int iPos;
    int iCount = CBGetCount(fodInfos->DlgInfos.hwndLookInCB);

    TRACE("\n");

    /* Delete each string of the combo and their associated data */
    if (iCount != CB_ERR)
    {
      for(iPos = iCount-1;iPos>=0;iPos--)
      {
        SFOLDER *tmpFolder = (LPSFOLDER) CBGetItemDataPtr(fodInfos->DlgInfos.hwndLookInCB,iPos);
        COMDLG32_SHFree(tmpFolder->pidlItem);
        MemFree(tmpFolder);
        CBDeleteString(fodInfos->DlgInfos.hwndLookInCB,iPos);
      }
    }

    /* LookInInfos structure */
    MemFree(liInfos);
    RemovePropA(fodInfos->DlgInfos.hwndLookInCB,LookInInfosStr);
}

/***********************************************************************
 * FILEDLG95_FILENAME_FillFromSelection
 *
 * fills the edit box from the cached DataObject
 */
void FILEDLG95_FILENAME_FillFromSelection (HWND hwnd)
{
    FileOpenDlgInfos *fodInfos;
    LPITEMIDLIST      pidl;
    UINT              nFiles = 0, nFileToOpen, nFileSelected, nLength = 0;
    WCHAR             lpstrTemp[MAX_PATH];
    LPWSTR            lpstrAllFile, lpstrCurrFile;

    TRACE("\n");
    fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);

    /* Count how many files we have */
    nFileSelected = GetNumSelected( fodInfos->Shell.FOIDataObject );

    /* calculate the string length, count files */
    if (nFileSelected >= 1)
    {
      nLength += 3;	/* first and last quotes, trailing \0 */
      for ( nFileToOpen = 0; nFileToOpen < nFileSelected; nFileToOpen++ )
      {
        pidl = GetPidlFromDataObject( fodInfos->Shell.FOIDataObject, nFileToOpen+1 );

        if (pidl)
	{
          /* get the total length of the selected file names */
          lpstrTemp[0] = '\0';
          GetName( fodInfos->Shell.FOIShellFolder, pidl, SHGDN_INFOLDER|SHGDN_FORPARSING, lpstrTemp );

          if ( ! IsPidlFolder(fodInfos->Shell.FOIShellFolder, pidl) ) /* Ignore folders */
	  {
            nLength += lstrlenW( lpstrTemp ) + 3;
            nFiles++;
	  }
          COMDLG32_SHFree( pidl );
	}
      }
    }

    /* allocate the buffer */
    if (nFiles <= 1) nLength = MAX_PATH;
    lpstrAllFile = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nLength * sizeof(WCHAR));

    /* Generate the string for the edit control */
    if(nFiles >= 1)
    {
      lpstrCurrFile = lpstrAllFile;
      for ( nFileToOpen = 0; nFileToOpen < nFileSelected; nFileToOpen++ )
      {
        pidl = GetPidlFromDataObject( fodInfos->Shell.FOIDataObject, nFileToOpen+1 );

        if (pidl)
	{
	  /* get the file name */
          lpstrTemp[0] = '\0';
          GetName( fodInfos->Shell.FOIShellFolder, pidl, SHGDN_INFOLDER|SHGDN_FORPARSING, lpstrTemp );

          if (! IsPidlFolder(fodInfos->Shell.FOIShellFolder, pidl)) /* Ignore folders */
	  {
            if ( nFiles > 1)
	    {
              *lpstrCurrFile++ =  '\"';
              lstrcpyW( lpstrCurrFile, lpstrTemp );
              lpstrCurrFile += lstrlenW( lpstrTemp );
              *lpstrCurrFile++ = '\"';
              *lpstrCurrFile++ = ' ';
              *lpstrCurrFile = 0;
	    }
	    else
	    {
              lstrcpyW( lpstrAllFile, lpstrTemp );
	    }
          }
          COMDLG32_SHFree( pidl );
	}
      }
      SetWindowTextW( fodInfos->DlgInfos.hwndFileName, lpstrAllFile );
       
      /* Select the file name like Windows does */
      if (filename_is_edit( fodInfos ))
          SendMessageW(fodInfos->DlgInfos.hwndFileName, EM_SETSEL, 0, -1);
    }
    HeapFree(GetProcessHeap(),0, lpstrAllFile );
}


/* copied from shell32 to avoid linking to it
 * Although shell32 is already linked the behaviour of exported StrRetToStrN
 * is dependent on whether emulated OS is unicode or not.
 */
static HRESULT COMDLG32_StrRetToStrNW (LPWSTR dest, DWORD len, LPSTRRET src, const ITEMIDLIST *pidl)
{
	switch (src->uType)
	{
	  case STRRET_WSTR:
	    lstrcpynW(dest, src->u.pOleStr, len);
	    COMDLG32_SHFree(src->u.pOleStr);
	    break;

	  case STRRET_CSTR:
            if (!MultiByteToWideChar( CP_ACP, 0, src->u.cStr, -1, dest, len ) && len)
                  dest[len-1] = 0;
	    break;

	  case STRRET_OFFSET:
            if (!MultiByteToWideChar( CP_ACP, 0, ((LPCSTR)&pidl->mkid)+src->u.uOffset, -1, dest, len ) && len)
                  dest[len-1] = 0;
	    break;

	  default:
	    FIXME("unknown type %x!\n", src->uType);
	    if (len) *dest = '\0';
	    return E_FAIL;
	}
	return S_OK;
}

/***********************************************************************
 * FILEDLG95_FILENAME_GetFileNames
 *
 * Copies the filenames to a delimited string list.
 */
static int FILEDLG95_FILENAME_GetFileNames (HWND hwnd, LPWSTR * lpstrFileList, UINT * sizeUsed)
{
	FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);
	UINT nFileCount = 0;	/* number of files */
	UINT nStrLen = 0;	/* length of string in edit control */
	LPWSTR lpstrEdit;	/* buffer for string from edit control */

	TRACE("\n");

	/* get the filenames from the filename control */
	nStrLen = GetWindowTextLengthW( fodInfos->DlgInfos.hwndFileName );
	lpstrEdit = MemAlloc( (nStrLen+1)*sizeof(WCHAR) );
	GetWindowTextW( fodInfos->DlgInfos.hwndFileName, lpstrEdit, nStrLen+1);

	TRACE("nStrLen=%u str=%s\n", nStrLen, debugstr_w(lpstrEdit));

	nFileCount = COMDLG32_SplitFileNames(lpstrEdit, nStrLen, lpstrFileList, sizeUsed);
	MemFree(lpstrEdit);
	return nFileCount;
}

#define SETDefFormatEtc(fe,cf,med) \
{ \
    (fe).cfFormat = cf;\
    (fe).dwAspect = DVASPECT_CONTENT; \
    (fe).ptd =NULL;\
    (fe).tymed = med;\
    (fe).lindex = -1;\
};

/*
 * DATAOBJECT Helper functions
 */

/***********************************************************************
 * COMCTL32_ReleaseStgMedium
 *
 * like ReleaseStgMedium from ole32
 */
static void COMCTL32_ReleaseStgMedium (STGMEDIUM medium)
{
      if(medium.pUnkForRelease)
      {
        IUnknown_Release(medium.pUnkForRelease);
      }
      else
      {
        GlobalUnlock(medium.u.hGlobal);
        GlobalFree(medium.u.hGlobal);
      }
}

/***********************************************************************
 *          GetPidlFromDataObject
 *
 * Return pidl(s) by number from the cached DataObject
 *
 * nPidlIndex=0 gets the fully qualified root path
 */
LPITEMIDLIST GetPidlFromDataObject ( IDataObject *doSelected, UINT nPidlIndex)
{

    STGMEDIUM medium;
    FORMATETC formatetc;
    LPITEMIDLIST pidl = NULL;

    TRACE("sv=%p index=%u\n", doSelected, nPidlIndex);

    if (!doSelected)
        return NULL;
	
    /* Set the FORMATETC structure*/
    SETDefFormatEtc(formatetc, RegisterClipboardFormatA(CFSTR_SHELLIDLISTA), TYMED_HGLOBAL);

    /* Get the pidls from IDataObject */
    if(SUCCEEDED(IDataObject_GetData(doSelected,&formatetc,&medium)))
    {
      LPIDA cida = GlobalLock(medium.u.hGlobal);
      if(nPidlIndex <= cida->cidl)
      {
        pidl = COMDLG32_PIDL_ILClone((LPITEMIDLIST)(&((LPBYTE)cida)[cida->aoffset[nPidlIndex]]));
      }
      COMCTL32_ReleaseStgMedium(medium);
    }
    return pidl;
}

/***********************************************************************
 *          GetNumSelected
 *
 * Return the number of selected items in the DataObject.
 *
*/
static UINT GetNumSelected( IDataObject *doSelected )
{
    UINT retVal = 0;
    STGMEDIUM medium;
    FORMATETC formatetc;

    TRACE("sv=%p\n", doSelected);

    if (!doSelected) return 0;

    /* Set the FORMATETC structure*/
    SETDefFormatEtc(formatetc, RegisterClipboardFormatA(CFSTR_SHELLIDLISTA), TYMED_HGLOBAL);

    /* Get the pidls from IDataObject */
    if(SUCCEEDED(IDataObject_GetData(doSelected,&formatetc,&medium)))
    {
      LPIDA cida = GlobalLock(medium.u.hGlobal);
      retVal = cida->cidl;
      COMCTL32_ReleaseStgMedium(medium);
      return retVal;
    }
    return 0;
}

/*
 * TOOLS
 */

/***********************************************************************
 *      GetName
 *
 * Get the pidl's display name (relative to folder) and
 * put it in lpstrFileName.
 *
 * Return NOERROR on success,
 * E_FAIL otherwise
 */

static HRESULT GetName(LPSHELLFOLDER lpsf, LPITEMIDLIST pidl,DWORD dwFlags,LPWSTR lpstrFileName)
{
  STRRET str;
  HRESULT hRes;

  TRACE("sf=%p pidl=%p\n", lpsf, pidl);

  if(!lpsf)
  {
    SHGetDesktopFolder(&lpsf);
    hRes = GetName(lpsf,pidl,dwFlags,lpstrFileName);
    IShellFolder_Release(lpsf);
    return hRes;
  }

  /* Get the display name of the pidl relative to the folder */
  if (SUCCEEDED(hRes = IShellFolder_GetDisplayNameOf(lpsf, pidl, dwFlags, &str)))
  {
      return COMDLG32_StrRetToStrNW(lpstrFileName, MAX_PATH, &str, pidl);
  }
  return E_FAIL;
}

/***********************************************************************
 *      GetShellFolderFromPidl
 *
 * pidlRel is the item pidl relative
 * Return the IShellFolder of the absolute pidl
 */
IShellFolder *GetShellFolderFromPidl(LPITEMIDLIST pidlAbs)
{
  IShellFolder *psf = NULL,*psfParent;

  TRACE("%p\n", pidlAbs);

  if(SUCCEEDED(SHGetDesktopFolder(&psfParent)))
  {
    psf = psfParent;
    if(pidlAbs && pidlAbs->mkid.cb)
    {
      if(SUCCEEDED(IShellFolder_BindToObject(psfParent, pidlAbs, NULL, &IID_IShellFolder, (LPVOID*)&psf)))
      {
	IShellFolder_Release(psfParent);
        return psf;
      }
    }
    /* return the desktop */
    return psfParent;
  }
  return NULL;
}

/***********************************************************************
 *      GetParentPidl
 *
 * Return the LPITEMIDLIST to the parent of the pidl in the list
 */
LPITEMIDLIST GetParentPidl(LPITEMIDLIST pidl)
{
  LPITEMIDLIST pidlParent;

  TRACE("%p\n", pidl);

  pidlParent = COMDLG32_PIDL_ILClone(pidl);
  COMDLG32_PIDL_ILRemoveLastID(pidlParent);

  return pidlParent;
}

/***********************************************************************
 *      GetPidlFromName
 *
 * returns the pidl of the file name relative to folder
 * NULL if an error occurred
 */
static LPITEMIDLIST GetPidlFromName(IShellFolder *lpsf,LPWSTR lpcstrFileName)
{
  LPITEMIDLIST pidl = NULL;
  ULONG ulEaten;

  TRACE("sf=%p file=%s\n", lpsf, debugstr_w(lpcstrFileName));

  if(!lpcstrFileName) return NULL;
  if(!*lpcstrFileName) return NULL;

  if(!lpsf)
  {
    if (SUCCEEDED(SHGetDesktopFolder(&lpsf))) {
        IShellFolder_ParseDisplayName(lpsf, 0, NULL, lpcstrFileName, &ulEaten, &pidl, NULL);
        IShellFolder_Release(lpsf);
    }
  }
  else
  {
    IShellFolder_ParseDisplayName(lpsf, 0, NULL, lpcstrFileName, &ulEaten, &pidl, NULL);
  }
  return pidl;
}

/*
*/
static BOOL IsPidlFolder (LPSHELLFOLDER psf, LPCITEMIDLIST pidl)
{
	ULONG uAttr  = SFGAO_FOLDER | SFGAO_HASSUBFOLDER;
	HRESULT ret;

	TRACE("%p, %p\n", psf, pidl);

  	ret = IShellFolder_GetAttributesOf( psf, 1, &pidl, &uAttr );

	TRACE("-- 0x%08x 0x%08x\n", uAttr, ret);
	/* see documentation shell 4.1*/
        return uAttr & (SFGAO_FOLDER | SFGAO_HASSUBFOLDER);
}

/***********************************************************************
 *      BrowseSelectedFolder
 */
static BOOL BrowseSelectedFolder(HWND hwnd)
{
  BOOL bBrowseSelFolder = FALSE;
  FileOpenDlgInfos *fodInfos = GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  if (GetNumSelected(fodInfos->Shell.FOIDataObject) == 1)
  {
      LPITEMIDLIST pidlSelection;

      /* get the file selected */
      pidlSelection  = GetPidlFromDataObject( fodInfos->Shell.FOIDataObject, 1);
      if (IsPidlFolder (fodInfos->Shell.FOIShellFolder, pidlSelection))
      {
          if ( FAILED( IShellBrowser_BrowseObject( fodInfos->Shell.FOIShellBrowser,
                         pidlSelection, SBSP_RELATIVE ) ) )
          {
               static const WCHAR notexist[] = {'P','a','t','h',' ','d','o','e','s',
                                   ' ','n','o','t',' ','e','x','i','s','t',0};
               MessageBoxW( hwnd, notexist, fodInfos->title, MB_OK | MB_ICONEXCLAMATION );
          }
          bBrowseSelFolder = TRUE;
          if(fodInfos->ofnInfos->Flags & OFN_EXPLORER)
              SendCustomDlgNotificationMessage(hwnd,CDN_FOLDERCHANGE);
      }
      COMDLG32_SHFree( pidlSelection );
  }

  return bBrowseSelFolder;
}

/*
 * Memory allocation methods */
static void *MemAlloc(UINT size)
{
    return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size);
}

static void MemFree(void *mem)
{
    HeapFree(GetProcessHeap(),0,mem);
}

static inline BOOL valid_struct_size( DWORD size )
{
    return (size == OPENFILENAME_SIZE_VERSION_400W) ||
        (size == sizeof( OPENFILENAMEW ));
}

static inline BOOL is_win16_looks(DWORD flags)
{
    return (flags & (OFN_ALLOWMULTISELECT|OFN_ENABLEHOOK|OFN_ENABLETEMPLATE) &&
            !(flags & OFN_EXPLORER));
}

/* ------------------ APIs ---------------------- */

/***********************************************************************
 *            GetOpenFileNameA  (COMDLG32.@)
 *
 * Creates a dialog box for the user to select a file to open.
 *
 * RETURNS
 *    TRUE on success: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 */
BOOL WINAPI GetOpenFileNameA(
	LPOPENFILENAMEA ofn) /* [in/out] address of init structure */
{
    TRACE("flags %08x\n", ofn->Flags);

    if (!valid_struct_size( ofn->lStructSize ))
    {
        COMDLG32_SetCommDlgExtendedError( CDERR_STRUCTSIZE );
        return FALSE;
    }

    /* OFN_FILEMUSTEXIST implies OFN_PATHMUSTEXIST */
    if (ofn->Flags & OFN_FILEMUSTEXIST)
        ofn->Flags |= OFN_PATHMUSTEXIST;

    if (is_win16_looks(ofn->Flags))
        return GetFileName31A(ofn, OPEN_DIALOG);
    else
        return GetFileDialog95A(ofn, OPEN_DIALOG);
}

/***********************************************************************
 *            GetOpenFileNameW (COMDLG32.@)
 *
 * Creates a dialog box for the user to select a file to open.
 *
 * RETURNS
 *    TRUE on success: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 */
BOOL WINAPI GetOpenFileNameW(
	LPOPENFILENAMEW ofn) /* [in/out] address of init structure */
{
    TRACE("flags %08x\n", ofn->Flags);

    if (!valid_struct_size( ofn->lStructSize ))
    {
        COMDLG32_SetCommDlgExtendedError( CDERR_STRUCTSIZE );
        return FALSE;
    }

    /* OFN_FILEMUSTEXIST implies OFN_PATHMUSTEXIST */
    if (ofn->Flags & OFN_FILEMUSTEXIST)
        ofn->Flags |= OFN_PATHMUSTEXIST;

    if (is_win16_looks(ofn->Flags))
        return GetFileName31W(ofn, OPEN_DIALOG);
    else
        return GetFileDialog95W(ofn, OPEN_DIALOG);
}


/***********************************************************************
 *            GetSaveFileNameA  (COMDLG32.@)
 *
 * Creates a dialog box for the user to select a file to save.
 *
 * RETURNS
 *    TRUE on success: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 */
BOOL WINAPI GetSaveFileNameA(
	LPOPENFILENAMEA ofn) /* [in/out] address of init structure */
{
    if (!valid_struct_size( ofn->lStructSize ))
    {
        COMDLG32_SetCommDlgExtendedError( CDERR_STRUCTSIZE );
        return FALSE;
    }

    if (is_win16_looks(ofn->Flags))
        return GetFileName31A(ofn, SAVE_DIALOG);
    else
        return GetFileDialog95A(ofn, SAVE_DIALOG);
}

/***********************************************************************
 *            GetSaveFileNameW  (COMDLG32.@)
 *
 * Creates a dialog box for the user to select a file to save.
 *
 * RETURNS
 *    TRUE on success: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 */
BOOL WINAPI GetSaveFileNameW(
	LPOPENFILENAMEW ofn) /* [in/out] address of init structure */
{
    if (!valid_struct_size( ofn->lStructSize ))
    {
        COMDLG32_SetCommDlgExtendedError( CDERR_STRUCTSIZE );
        return FALSE;
    }

    if (is_win16_looks(ofn->Flags))
        return GetFileName31W(ofn, SAVE_DIALOG);
    else
        return GetFileDialog95W(ofn, SAVE_DIALOG);
}

/***********************************************************************
 *	GetFileTitleA		(COMDLG32.@)
 *
 * See GetFileTitleW.
 */
short WINAPI GetFileTitleA(LPCSTR lpFile, LPSTR lpTitle, WORD cbBuf)
{
    int ret;
    UNICODE_STRING strWFile;
    LPWSTR lpWTitle;

    RtlCreateUnicodeStringFromAsciiz(&strWFile, lpFile);
    lpWTitle = RtlAllocateHeap( GetProcessHeap(), 0, cbBuf*sizeof(WCHAR));
    ret = GetFileTitleW(strWFile.Buffer, lpWTitle, cbBuf);
    if (!ret) WideCharToMultiByte( CP_ACP, 0, lpWTitle, -1, lpTitle, cbBuf, NULL, NULL );
    RtlFreeUnicodeString( &strWFile );
    RtlFreeHeap( GetProcessHeap(), 0, lpWTitle );
    return ret;
}


/***********************************************************************
 *	GetFileTitleW		(COMDLG32.@)
 *
 * Get the name of a file.
 *
 * PARAMS
 *  lpFile  [I] name and location of file
 *  lpTitle [O] returned file name
 *  cbBuf   [I] buffer size of lpTitle
 *
 * RETURNS
 *  Success: zero
 *  Failure: negative number.
 */
short WINAPI GetFileTitleW(LPCWSTR lpFile, LPWSTR lpTitle, WORD cbBuf)
{
	int i, len;
        static const WCHAR brkpoint[] = {'*','[',']',0};
	TRACE("(%p %p %d);\n", lpFile, lpTitle, cbBuf);

	if(lpFile == NULL || lpTitle == NULL)
		return -1;

	len = lstrlenW(lpFile);

	if (len == 0)
		return -1;

	if(strpbrkW(lpFile, brkpoint))
		return -1;

	len--;

	if(lpFile[len] == '/' || lpFile[len] == '\\' || lpFile[len] == ':')
		return -1;

	for(i = len; i >= 0; i--)
	{
		if (lpFile[i] == '/' ||  lpFile[i] == '\\' ||  lpFile[i] == ':')
		{
			i++;
			break;
		}
	}

	if(i == -1)
		i++;

	TRACE("---> %s\n", debugstr_w(&lpFile[i]));

	len = lstrlenW(lpFile+i)+1;
	if(cbBuf < len)
		return len;

	lstrcpyW(lpTitle, &lpFile[i]);
	return 0;
}
