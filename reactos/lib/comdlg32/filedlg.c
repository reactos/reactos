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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
 * FIXME: lpstrCustomFilter not handled
 *
 * FIXME: if the size of lpstrFile (nMaxFile) is too small the first
 * two bytes of lpstrFile should contain the needed size
 *
 * FIXME: algorithm for selecting the initial directory is too simple
 *
 * FIXME: add to recent docs
 *
 * FIXME: flags not implemented: OFN_CREATEPROMPT, OFN_DONTADDTORECENT,
 * OFN_ENABLEINCLUDENOTIFY, OFN_ENABLESIZING,
 * OFN_NODEREFERENCELINKS, OFN_NOREADONLYRETURN,
 * OFN_NOTESTFILECREATE, OFN_USEMONIKERS
 *
 * FIXME: lCustData for lpfnHook (WM_INITDIALOG)
 *
 *
 */

#include "config.h"
#include "wine/port.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "dlgs.h"
#include "cdlg.h"
#include "wine/debug.h"
#include "cderr.h"
#include "shellapi.h"
#include "shlguid.h"
#include "shlobj.h"
#include "filedlgbrowser.h"
#include "shlwapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#define UNIMPLEMENTED_FLAGS \
(OFN_CREATEPROMPT | OFN_DONTADDTORECENT |\
OFN_ENABLEINCLUDENOTIFY | OFN_ENABLESIZING |\
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
#define ICONWIDTH 18
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
  SendMessageA(hwnd,CB_ADDSTRING,0,(LPARAM)str);
#define CBAddStringW(hwnd,str) \
  SendMessageW(hwnd,CB_ADDSTRING,0,(LPARAM)str);

#define CBInsertString(hwnd,str,pos) \
  SendMessageA(hwnd,CB_INSERTSTRING,(WPARAM)pos,(LPARAM)str);

#define CBDeleteString(hwnd,pos) \
  SendMessageA(hwnd,CB_DELETESTRING,(WPARAM)pos,0);

#define CBSetItemDataPtr(hwnd,iItemId,dataPtr) \
  SendMessageA(hwnd,CB_SETITEMDATA,(WPARAM)iItemId,(LPARAM)dataPtr);

#define CBGetItemDataPtr(hwnd,iItemId) \
  SendMessageA(hwnd,CB_GETITEMDATA,(WPARAM)iItemId,0)

#define CBGetLBText(hwnd,iItemId,str) \
  SendMessageA(hwnd,CB_GETLBTEXT,(WPARAM)iItemId,(LPARAM)str);

#define CBGetCurSel(hwnd) \
  SendMessageA(hwnd,CB_GETCURSEL,0,0);

#define CBSetCurSel(hwnd,pos) \
  SendMessageA(hwnd,CB_SETCURSEL,(WPARAM)pos,0);

#define CBGetCount(hwnd) \
    SendMessageA(hwnd,CB_GETCOUNT,0,0);
#define CBShowDropDown(hwnd,show) \
  SendMessageA(hwnd,CB_SHOWDROPDOWN,(WPARAM)show,0);
#define CBSetItemHeight(hwnd,index,height) \
  SendMessageA(hwnd,CB_SETITEMHEIGHT,(WPARAM)index,(LPARAM)height);

#define CBSetExtendedUI(hwnd,flag) \
  SendMessageA(hwnd,CB_SETEXTENDEDUI,(WPARAM)(flag),0)

const char *FileOpenDlgInfosStr = "FileOpenDlgInfos"; /* windows property description string */
const char *LookInInfosStr = "LookInInfos"; /* LOOKIN combo box property */

/***********************************************************************
 * Prototypes
 */

/* Internal functions used by the dialog */
static LRESULT FILEDLG95_FillControls(HWND hwnd, WPARAM wParam, LPARAM lParam);
static LRESULT FILEDLG95_OnWMCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
static LRESULT FILEDLG95_OnWMGetIShellBrowser(HWND hwnd);
       BOOL    FILEDLG95_OnOpen(HWND hwnd);
static LRESULT FILEDLG95_InitControls(HWND hwnd);
static void    FILEDLG95_Clean(HWND hwnd);

/* Functions used by the shell navigation */
static LRESULT FILEDLG95_SHELL_Init(HWND hwnd);
static BOOL    FILEDLG95_SHELL_UpFolder(HWND hwnd);
static BOOL    FILEDLG95_SHELL_ExecuteCommand(HWND hwnd, LPCSTR lpVerb);
static void    FILEDLG95_SHELL_Clean(HWND hwnd);
static BOOL    FILEDLG95_SHELL_BrowseToDesktop(HWND hwnd);

/* Functions used by the filetype combo box */
static HRESULT FILEDLG95_FILETYPE_Init(HWND hwnd);
static BOOL    FILEDLG95_FILETYPE_OnCommand(HWND hwnd, WORD wNotifyCode);
static int     FILEDLG95_FILETYPE_SearchExt(HWND hwnd,LPCWSTR lpstrExt);
static void    FILEDLG95_FILETYPE_Clean(HWND hwnd);

/* Functions used by the Look In combo box */
static HRESULT FILEDLG95_LOOKIN_Init(HWND hwndCombo);
static LRESULT FILEDLG95_LOOKIN_DrawItem(LPDRAWITEMSTRUCT pDIStruct);
static BOOL    FILEDLG95_LOOKIN_OnCommand(HWND hwnd, WORD wNotifyCode);
static int     FILEDLG95_LOOKIN_AddItem(HWND hwnd,LPITEMIDLIST pidl, int iInsertId);
static int     FILEDLG95_LOOKIN_SearchItem(HWND hwnd,WPARAM searchArg,int iSearchMethod);
static int     FILEDLG95_LOOKIN_InsertItemAfterParent(HWND hwnd,LPITEMIDLIST pidl);
static int     FILEDLG95_LOOKIN_RemoveMostExpandedItem(HWND hwnd);
       int     FILEDLG95_LOOKIN_SelectItem(HWND hwnd,LPITEMIDLIST pidl);
static void    FILEDLG95_LOOKIN_Clean(HWND hwnd);

/* Miscellaneous tool functions */
HRESULT       GetName(LPSHELLFOLDER lpsf, LPITEMIDLIST pidl,DWORD dwFlags,LPSTR lpstrFileName);
HRESULT       GetFileName(HWND hwnd, LPITEMIDLIST pidl, LPSTR lpstrFileName);
IShellFolder* GetShellFolderFromPidl(LPITEMIDLIST pidlAbs);
LPITEMIDLIST  GetParentPidl(LPITEMIDLIST pidl);
LPITEMIDLIST  GetPidlFromName(IShellFolder *psf,LPWSTR lpcstrFileName);

/* Shell memory allocation */
static void *MemAlloc(UINT size);
static void MemFree(void *mem);

BOOL WINAPI GetFileName95(FileOpenDlgInfos *fodInfos);
INT_PTR CALLBACK FileOpenDlgProc95(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT SendCustomDlgNotificationMessage(HWND hwndParentDlg, UINT uCode);
HRESULT FILEDLG95_HandleCustomDialogMessages(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL FILEDLG95_OnOpenMultipleFiles(HWND hwnd, LPWSTR lpstrFileList, UINT nFileCount, UINT sizeUsed);
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
BOOL WINAPI GetFileName95(FileOpenDlgInfos *fodInfos)
{

    LRESULT lRes;
    LPCVOID template;
    HRSRC hRes;
    HANDLE hDlgTmpl = 0;

    /* test for missing functionality */
    if (fodInfos->ofnInfos->Flags & UNIMPLEMENTED_FLAGS)
    {
      FIXME("Flags 0x%08lx not yet implemented\n",
         fodInfos->ofnInfos->Flags & UNIMPLEMENTED_FLAGS);
    }

    /* Create the dialog from a template */

    if(!(hRes = FindResourceA(COMDLG32_hInstance,MAKEINTRESOURCEA(NEWFILEOPENORD),(LPSTR)RT_DIALOG)))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
        return FALSE;
    }
    if (!(hDlgTmpl = LoadResource(COMDLG32_hInstance, hRes )) ||
        !(template = LockResource( hDlgTmpl )))
    {
        COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
        return FALSE;
    }

    /* old style hook messages */
    if (IsHooked(fodInfos))
    {
      fodInfos->HookMsg.fileokstring = RegisterWindowMessageA(FILEOKSTRINGA);
      fodInfos->HookMsg.lbselchstring = RegisterWindowMessageA(LBSELCHSTRINGA);
      fodInfos->HookMsg.helpmsgstring = RegisterWindowMessageA(HELPMSGSTRINGA);
      fodInfos->HookMsg.sharevistring = RegisterWindowMessageA(SHAREVISTRINGA);
    }

    lRes = DialogBoxIndirectParamA(COMDLG32_hInstance,
                                  (LPDLGTEMPLATEA) template,
                                  fodInfos->ofnInfos->hwndOwner,
                                  FileOpenDlgProc95,
                                  (LPARAM) fodInfos);

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
BOOL  WINAPI GetFileDialog95A(LPOPENFILENAMEA ofn,UINT iDlgType)
{
  BOOL ret;
  FileOpenDlgInfos fodInfos;
  LPSTR lpstrSavDir = NULL;
  LPWSTR title = NULL;
  LPWSTR defext = NULL;
  LPWSTR filter = NULL;
  LPWSTR customfilter = NULL;

  /* Initialize FileOpenDlgInfos structure */
  ZeroMemory(&fodInfos, sizeof(FileOpenDlgInfos));

  /* Pass in the original ofn */
  fodInfos.ofnInfos = ofn;

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

    /* filter is a list...  title\0ext\0......\0\0 */
    s = ofn->lpstrCustomFilter;
    while (*s) s = s+strlen(s)+1;
    s++;
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
      ret = 0;
  }

  if (lpstrSavDir)
  {
      SetCurrentDirectoryA(lpstrSavDir);
      MemFree(lpstrSavDir);
  }

  if(title)
    MemFree(title);
  if(defext)
    MemFree(defext);
  if(filter)
    MemFree(filter);
  if(customfilter)
    MemFree(customfilter);
  if(fodInfos.initdir)
    MemFree(fodInfos.initdir);

  if(fodInfos.filename)
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
 * FIXME: lpstrCustomFilter has to be converted back
 *
 */
BOOL  WINAPI GetFileDialog95W(LPOPENFILENAMEW ofn,UINT iDlgType)
{
  BOOL ret;
  FileOpenDlgInfos fodInfos;
  LPSTR lpstrSavDir = NULL;

  /* Initialize FileOpenDlgInfos structure */
  ZeroMemory(&fodInfos, sizeof(FileOpenDlgInfos));

  /*  Pass in the original ofn */
  fodInfos.ofnInfos = (LPOPENFILENAMEA) ofn;

  fodInfos.title = ofn->lpstrTitle;
  fodInfos.defext = ofn->lpstrDefExt;
  fodInfos.filter = ofn->lpstrFilter;
  fodInfos.customfilter = ofn->lpstrCustomFilter;

  /* convert string arguments, save others */
  if(ofn->lpstrFile)
  {
    fodInfos.filename = MemAlloc(ofn->nMaxFile*sizeof(WCHAR));
    strncpyW(fodInfos.filename,ofn->lpstrFile,ofn->nMaxFile);
  }
  else
    fodInfos.filename = NULL;

  if(ofn->lpstrInitialDir)
  {
    DWORD len = strlenW(ofn->lpstrInitialDir);
    fodInfos.initdir = MemAlloc((len+1)*sizeof(WCHAR));
    strcpyW(fodInfos.initdir,ofn->lpstrInitialDir);
  }
  else
    fodInfos.initdir = NULL;

  /* save current directory */
  if (ofn->Flags & OFN_NOCHANGEDIR)
  {
     lpstrSavDir = MemAlloc(MAX_PATH);
     GetCurrentDirectoryA(MAX_PATH, lpstrSavDir);
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
      ret = 0;
  }

  if (lpstrSavDir)
  {
      SetCurrentDirectoryA(lpstrSavDir);
      MemFree(lpstrSavDir);
  }

  /* restore saved IN arguments and convert OUT arguments back */
  MemFree(fodInfos.filename);
  MemFree(fodInfos.initdir);
  return ret;
}

/***********************************************************************
 *      ArrangeCtrlPositions [internal]
 *
 * NOTE: Do not change anything here without a lot of testing.
 */
static void ArrangeCtrlPositions(HWND hwndChildDlg, HWND hwndParentDlg, BOOL hide_help)
{
    HWND hwndChild, hwndStc32;
    RECT rectParent, rectChild, rectStc32;
    INT help_fixup = 0;

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
            if (rectChild.top > rectStc32.bottom)
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

    if (hwndStc32)
    {
        if (rectParent.right > rectChild.right)
        {
            rectParent.right += rectChild.right;
            rectParent.right -= rectStc32.right - rectStc32.left;
        }
        else
        {
            rectParent.right = rectChild.right;
        }

        if (rectParent.bottom > rectChild.bottom)
        {
            rectParent.bottom += rectChild.bottom;
            rectParent.bottom -= rectStc32.bottom - rectStc32.top;
        }
        else
        {
            rectParent.bottom = rectChild.bottom;
        }
    }
    else
    {
        rectParent.bottom += rectChild.bottom;
    }

    /* finally use fixed parent size */
    rectParent.bottom -= help_fixup;

    /* save the size of the parent's client area */
    rectChild.right = rectParent.right;
    rectChild.bottom = rectParent.bottom;

    /* set the size of the parent dialog */
    AdjustWindowRectEx(&rectParent, GetWindowLongW(hwndParentDlg, GWL_STYLE),
                       FALSE, GetWindowLongW(hwndParentDlg, GWL_EXSTYLE));
    SetWindowPos(hwndParentDlg, 0,
                 0, 0,
                 rectParent.right - rectParent.left,
                 rectParent.bottom - rectParent.top,
                 SWP_NOMOVE | SWP_NOZORDER);

    /* set the size of the child dialog */
    SetWindowPos(hwndChildDlg, HWND_BOTTOM,
                 0, 0, rectChild.right, rectChild.bottom, SWP_NOACTIVATE);
}

INT_PTR CALLBACK FileOpenDlgProcUserTemplate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg) {
    case WM_INITDIALOG:
        return TRUE;
    }
    return FALSE;
}

HWND CreateTemplateDialog(FileOpenDlgInfos *fodInfos, HWND hwnd)
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
        hinst = 0;
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
            LPOPENFILENAMEW ofn = (LPOPENFILENAMEW) fodInfos->ofnInfos;
            hRes = FindResourceW( hinst, ofn->lpTemplateName, (LPWSTR)RT_DIALOG);
        }
        else
        {
            LPOPENFILENAMEA ofn = fodInfos->ofnInfos;
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
      hChildDlg = CreateDialogIndirectParamA(COMDLG32_hInstance, template, hwnd,
                                             IsHooked(fodInfos) ? (DLGPROC)fodInfos->ofnInfos->lpfnHook : FileOpenDlgProcUserTemplate,
                                             (LPARAM)fodInfos->ofnInfos);
      if(hChildDlg)
      {
        ShowWindow(hChildDlg,SW_SHOW);
        return hChildDlg;
      }
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

HRESULT SendCustomDlgNotificationMessage(HWND hwndParentDlg, UINT uCode)
{
    FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwndParentDlg,FileOpenDlgInfosStr);

    TRACE("%p 0x%04x\n",hwndParentDlg, uCode);

    if(!fodInfos) return 0;

    if(fodInfos->unicode)
      FIXME("sending OPENFILENAMEA structure. Hook is expecting OPENFILENAMEW!\n");

    if(fodInfos->DlgInfos.hwndCustomDlg)
    {
        OFNOTIFYA ofnNotify;
	HRESULT ret;
        ofnNotify.hdr.hwndFrom=hwndParentDlg;
        ofnNotify.hdr.idFrom=0;
        ofnNotify.hdr.code = uCode;
        ofnNotify.lpOFN = fodInfos->ofnInfos;
        ofnNotify.pszFile = NULL;
	TRACE("CALL NOTIFY for %x\n", uCode);
	ret = SendMessageA(fodInfos->DlgInfos.hwndCustomDlg,WM_NOTIFY,0,(LPARAM)&ofnNotify);
	TRACE("RET NOTIFY\n");
	return ret;
    }
    return TRUE;
}

HRESULT FILEDLG95_Handle_GetFilePath(HWND hwnd, DWORD size, LPSTR buffer)
{
    UINT sizeUsed = 0, n, total;
    LPWSTR lpstrFileList = NULL;
    WCHAR lpstrCurrentDir[MAX_PATH];
    FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

    TRACE("CDM_GETFILEPATH:\n");

    if ( ! (fodInfos->ofnInfos->Flags & OFN_EXPLORER ) )
        return -1;

    /* get path and filenames */
    SHGetPathFromIDListW(fodInfos->ShellInfos.pidlAbsCurrent,lpstrCurrentDir);
    n = FILEDLG95_FILENAME_GetFileNames(hwnd, &lpstrFileList, &sizeUsed);

    TRACE("path >%s< filespec >%s< %d files\n",
         debugstr_w(lpstrCurrentDir),debugstr_w(lpstrFileList),n);

    total = WideCharToMultiByte(CP_ACP, 0, lpstrCurrentDir, -1, 
                                NULL, 0, NULL, NULL);
    total += WideCharToMultiByte(CP_ACP, 0, lpstrFileList, sizeUsed, 
                                NULL, 0, NULL, NULL);

    /* Prepend the current path */
    n = WideCharToMultiByte(CP_ACP, 0, lpstrCurrentDir, -1, 
                            buffer, size, NULL, NULL);

    if(n<size)
    {
        /* 'n' includes trailing \0 */
        buffer[n-1] = '\\';
        WideCharToMultiByte(CP_ACP, 0, lpstrFileList, sizeUsed, 
                            &buffer[n], size-n, NULL, NULL);
    }
    MemFree(lpstrFileList);

    TRACE("returned -> %s\n",debugstr_a(buffer));

    return total;
}

HRESULT FILEDLG95_Handle_GetFileSpec(HWND hwnd, DWORD size, LPSTR buffer)
{
    UINT sizeUsed = 0;
    LPWSTR lpstrFileList = NULL;

    TRACE("CDM_GETSPEC:\n");

    FILEDLG95_FILENAME_GetFileNames(hwnd, &lpstrFileList, &sizeUsed);
    WideCharToMultiByte(CP_ACP, 0, lpstrFileList, sizeUsed, buffer, size, NULL, NULL);
    MemFree(lpstrFileList);

    return sizeUsed;
}

/***********************************************************************
*         FILEDLG95_HandleCustomDialogMessages
*
* Handle Custom Dialog Messages (CDM_FIRST -- CDM_LAST) messages
*/
HRESULT FILEDLG95_HandleCustomDialogMessages(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    char lpstrPath[MAX_PATH];
    FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);
    if(!fodInfos) return -1;

    switch(uMsg)
    {
        case CDM_GETFILEPATH:
            return FILEDLG95_Handle_GetFilePath(hwnd, (UINT)wParam, (LPSTR)lParam);

        case CDM_GETFOLDERPATH:
            TRACE("CDM_GETFOLDERPATH:\n");
	    SHGetPathFromIDListA(fodInfos->ShellInfos.pidlAbsCurrent,lpstrPath);
            if ((LPSTR)lParam!=NULL)
                lstrcpynA((LPSTR)lParam,lpstrPath,(int)wParam);
            return strlen(lpstrPath);

        case CDM_GETSPEC:
            return FILEDLG95_Handle_GetFileSpec(hwnd, (UINT)wParam, (LPSTR)lParam);

        case CDM_SETCONTROLTEXT:
            TRACE("CDM_SETCONTROLTEXT:\n");
	    if ( 0 != lParam )
	        SetDlgItemTextA( hwnd, (UINT) wParam, (LPSTR) lParam );
	    return TRUE;

        case CDM_HIDECONTROL:
        case CDM_SETDEFEXT:
            FIXME("CDM_HIDECONTROL,CDM_SETCONTROLTEXT,CDM_SETDEFEXT not implemented\n");
            return -1;
    }
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
  TRACE("0x%04x 0x%04x\n", hwnd, uMsg);
#endif

  switch(uMsg)
  {
    case WM_INITDIALOG:
      {
         FileOpenDlgInfos * fodInfos = (FileOpenDlgInfos *)lParam;

	 /* Adds the FileOpenDlgInfos in the property list of the dialog
            so it will be easily accessible through a GetPropA(...) */
      	 SetPropA(hwnd, FileOpenDlgInfosStr, (HANDLE) fodInfos);

      	 fodInfos->DlgInfos.hwndCustomDlg =
     	   CreateTemplateDialog((FileOpenDlgInfos *)lParam, hwnd);

         FILEDLG95_InitControls(hwnd);

         if (fodInfos->DlgInfos.hwndCustomDlg)
             ArrangeCtrlPositions(fodInfos->DlgInfos.hwndCustomDlg, hwnd,
                 (fodInfos->ofnInfos->Flags & (OFN_HIDEREADONLY | OFN_SHOWHELP)) == OFN_HIDEREADONLY);

      	 FILEDLG95_FillControls(hwnd, wParam, lParam);

         SendCustomDlgNotificationMessage(hwnd,CDN_INITDONE);
         SendCustomDlgNotificationMessage(hwnd,CDN_FOLDERCHANGE);
         SendCustomDlgNotificationMessage(hwnd,CDN_SELCHANGE);
         return 0;
       }
    case WM_COMMAND:
      return FILEDLG95_OnWMCommand(hwnd, wParam, lParam);
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
      RemovePropA(hwnd, FileOpenDlgInfosStr);
      return FALSE;

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
	    lpdi->lpszText =  (LPSTR) stringId;
	}
        return FALSE;
    }
    default :
      if(uMsg >= CDM_FIRST && uMsg <= CDM_LAST)
        return FILEDLG95_HandleCustomDialogMessages(hwnd, uMsg, wParam, lParam);
      return FALSE;
  }
}

/***********************************************************************
 *      FILEDLG95_InitControls
 *
 * WM_INITDIALOG message handler (before hook notification)
 */
static LRESULT FILEDLG95_InitControls(HWND hwnd)
{
  int win2000plus = 0;
  int win98plus   = 0;
  int handledPath = FALSE;
  OSVERSIONINFOA osVi;
  const WCHAR szwSlash[] = { '\\', 0 };
  const WCHAR szwStar[] = { '*',0 };

  TBBUTTON tbb[] =
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
  TBADDBITMAP tba[2];
  RECT rectTB;
  RECT rectlook;
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  tba[0].hInst = HINST_COMMCTRL;
  tba[0].nID   = IDB_VIEW_SMALL_COLOR;
  tba[1].hInst = COMDLG32_hInstance;
  tba[1].nID   = 800;

  TRACE("%p\n", fodInfos);

  /* Get windows version emulating */
  osVi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
  GetVersionExA(&osVi);
  if (osVi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
    win98plus   = ((osVi.dwMajorVersion > 4) || ((osVi.dwMajorVersion == 4) && (osVi.dwMinorVersion > 0)));
  } else if (osVi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
    win2000plus = (osVi.dwMajorVersion > 4);
    if (win2000plus) win98plus = TRUE;
  }
  TRACE("Running on 2000+ %d, 98+ %d\n", win2000plus, win98plus);

  /* Get the hwnd of the controls */
  fodInfos->DlgInfos.hwndFileName = GetDlgItem(hwnd,IDC_FILENAME);
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

  fodInfos->DlgInfos.hwndTB = CreateWindowExA(0, TOOLBARCLASSNAMEA, NULL,
        WS_CHILD | WS_GROUP | WS_VISIBLE | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS | CCS_NODIVIDER | CCS_NORESIZE,
        rectTB.left, rectTB.top,
        rectTB.right - rectTB.left, rectTB.bottom - rectTB.top,
        hwnd, (HMENU)IDC_TOOLBAR, COMDLG32_hInstance, NULL);

  SendMessageA(fodInfos->DlgInfos.hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);

/* FIXME: use TB_LOADIMAGES when implemented */
/*  SendMessageA(fodInfos->DlgInfos.hwndTB, TB_LOADIMAGES, (WPARAM) IDB_VIEW_SMALL_COLOR, HINST_COMMCTRL);*/
  SendMessageA(fodInfos->DlgInfos.hwndTB, TB_ADDBITMAP, (WPARAM) 12, (LPARAM) &tba[0]);
  SendMessageA(fodInfos->DlgInfos.hwndTB, TB_ADDBITMAP, (WPARAM) 1, (LPARAM) &tba[1]);

  SendMessageA(fodInfos->DlgInfos.hwndTB, TB_ADDBUTTONSA, (WPARAM) 9,(LPARAM) &tbb);
  SendMessageA(fodInfos->DlgInfos.hwndTB, TB_AUTOSIZE, 0, 0);

  /* Set the window text with the text specified in the OPENFILENAME structure */
  if(fodInfos->title)
  {
      SetWindowTextW(hwnd,fodInfos->title);
  }
  else if (fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG)
  {
      SetWindowTextA(hwnd,"Save");
  }

  /* Initialise the file name edit control */
  handledPath = FALSE;
  TRACE("Before manipilation, file = %s, dir = %s\n", debugstr_w(fodInfos->filename), debugstr_w(fodInfos->initdir));

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
            strcpyW(fodInfos->filename,nameBit);

            *nameBit = 0x00;
            if (fodInfos->initdir == NULL)
                MemFree(fodInfos->initdir);
            fodInfos->initdir = MemAlloc((strlenW(tmpBuf) + 1)*sizeof(WCHAR));
            strcpyW(fodInfos->initdir, tmpBuf);
            handledPath = TRUE;
            TRACE("Value in Filename includes path, overriding InitialDir: %s, %s\n",
                    debugstr_w(fodInfos->filename), debugstr_w(fodInfos->initdir));
         }
         SetDlgItemTextW(hwnd, IDC_FILENAME, fodInfos->filename);

      } else {
         SetDlgItemTextW(hwnd, IDC_FILENAME, fodInfos->filename);
      }
  }

  /* 2. (All platforms) If initdir is not null, then use it */
  if ((handledPath == FALSE) && (fodInfos->initdir!=NULL) &&
                                (*fodInfos->initdir!=0x00))
  {
      /* Work out the proper path as supplied one might be relative          */
      /* (Here because supplying '.' as dir browses to My Computer)          */
      if (handledPath==FALSE) {
          WCHAR tmpBuf[MAX_PATH];
          WCHAR tmpBuf2[MAX_PATH];
          WCHAR *nameBit;
          DWORD result;

          strcpyW(tmpBuf, fodInfos->initdir);
          if( PathFileExistsW(tmpBuf) ) {
              /* initdir does not have to be a directory. If a file is
               * specified, the dir part is taken */
              if( PathIsDirectoryW(tmpBuf)) {
                  if (tmpBuf[strlenW(tmpBuf)-1] != '\\') {
                     strcatW(tmpBuf, szwSlash);
                  }
                  strcatW(tmpBuf, szwStar);
              }
              result = GetFullPathNameW(tmpBuf, MAX_PATH, tmpBuf2, &nameBit);
              if (result) {
                 *nameBit = 0x00;
                 if (fodInfos->initdir)
                    MemFree(fodInfos->initdir);
                 fodInfos->initdir = MemAlloc((strlenW(tmpBuf2) + 1)*sizeof(WCHAR));
                 strcpyW(fodInfos->initdir, tmpBuf2);
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
  }

  if ((handledPath == FALSE) && ((fodInfos->initdir==NULL) ||
                                 (*fodInfos->initdir==0x00)))
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
            strcpyW(fodInfos->filename, nameBit);
            *nameBit = 0x00;

            len = strlenW(tmpBuf);
            if(fodInfos->initdir)
               MemFree(fodInfos->initdir);
            fodInfos->initdir = MemAlloc((len+1)*sizeof(WCHAR));
            strcpyW(fodInfos->initdir, tmpBuf);

            handledPath = TRUE;
            TRACE("Value in Filename includes path, overriding initdir: %s, %s\n",
                 debugstr_w(fodInfos->filename), debugstr_w(fodInfos->initdir));
         }
         SetDlgItemTextW(hwnd, IDC_FILENAME, fodInfos->filename);
      }

      /* 4. win98+ and win2000+ if any files of specified filter types in
            current directory, use it                                      */
      if ( win98plus && handledPath == FALSE &&
           fodInfos->filter && *fodInfos->filter) {

         BOOL   searchMore = TRUE;
         LPCWSTR lpstrPos = fodInfos->filter;
         WIN32_FIND_DATAW FindFileData;
         HANDLE hFind;

         while (searchMore)
         {
           /* filter is a list...  title\0ext\0......\0\0 */

           /* Skip the title */
           if(! *lpstrPos) break;	/* end */
           lpstrPos += strlenW(lpstrPos) + 1;

           /* See if any files exist in the current dir with this extension */
           if(! *lpstrPos) break;	/* end */

           hFind = FindFirstFileW(lpstrPos, &FindFileData);

           if (hFind == INVALID_HANDLE_VALUE) {
               /* None found - continue search */
               lpstrPos += strlenW(lpstrPos) + 1;

           } else {
               searchMore = FALSE;

               if(fodInfos->initdir)
                  MemFree(fodInfos->initdir);
               fodInfos->initdir = MemAlloc(MAX_PATH*sizeof(WCHAR));
               GetCurrentDirectoryW(MAX_PATH, fodInfos->initdir);

               handledPath = TRUE;
               TRACE("No initial dir specified, but files of type %s found in current, so using it\n",
                 debugstr_w(lpstrPos));
               break;
           }
         }
      }

      /* 5. Win2000+: FIXME: Next, Recently used? Not sure how windows does this */

      /* 6. Win98+ and 2000+: Use personal files dir, others use current dir */
      if (handledPath == FALSE && (win2000plus || win98plus)) {
          fodInfos->initdir = MemAlloc(MAX_PATH*sizeof(WCHAR));

          if(FAILED(COMDLG32_SHGetFolderPathW(hwnd, CSIDL_PERSONAL, 0, 0, fodInfos->initdir)))
          {
            if(FAILED(COMDLG32_SHGetFolderPathW(hwnd, CSIDL_DESKTOPDIRECTORY|CSIDL_FLAG_CREATE, 0, 0, fodInfos->initdir)))
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
      } else if (handledPath==FALSE) {
          fodInfos->initdir = MemAlloc(MAX_PATH*sizeof(WCHAR));
          GetCurrentDirectoryW(MAX_PATH, fodInfos->initdir);
          handledPath = TRUE;
          TRACE("No initial dir specified, using current dir of %s\n", debugstr_w(fodInfos->initdir));
      }
  }
  SetFocus(GetDlgItem(hwnd, IDC_FILENAME));
  TRACE("After manipulation, file = %s, dir = %s\n", debugstr_w(fodInfos->filename), debugstr_w(fodInfos->initdir));

  /* Must the open as read only check box be checked ?*/
  if(fodInfos->ofnInfos->Flags & OFN_READONLY)
  {
    SendDlgItemMessageA(hwnd,IDC_OPENREADONLY,BM_SETCHECK,(WPARAM)TRUE,0);
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

  /* Resize the height, if open as read only checkbox ad help button
     are hidden and we are not using a custom template nor a customDialog
     */
  if ( (fodInfos->ofnInfos->Flags & OFN_HIDEREADONLY) &&
       (!(fodInfos->ofnInfos->Flags &
         (OFN_SHOWHELP|OFN_ENABLETEMPLATE|OFN_ENABLETEMPLATEHANDLE))) && 
       (!fodInfos->DlgInfos.hwndCustomDlg ))
  {
    RECT rectDlg, rectHelp, rectCancel;
    GetWindowRect(hwnd, &rectDlg);
    GetWindowRect(GetDlgItem(hwnd, pshHelp), &rectHelp);
    GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rectCancel);
    /* subtract the height of the help button plus the space between
       the help button and the cancel button to the height of the dialog */
    SetWindowPos(hwnd, 0, 0, 0, rectDlg.right-rectDlg.left,
                 (rectDlg.bottom-rectDlg.top) - (rectHelp.bottom - rectCancel.bottom),
                 SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
  }
  /* change Open to Save FIXME: use resources */
  if (fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG)
  {
      SetDlgItemTextA(hwnd,IDOK,"&Save");
      SetDlgItemTextA(hwnd,IDC_LOOKINSTATIC,"Save &in");
  }
  return 0;
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

  /* Initialize the filter combo box */
  FILEDLG95_FILETYPE_Init(hwnd);

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
static LRESULT FILEDLG95_OnWMCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  WORD wNotifyCode = HIWORD(wParam); /* notification code */
  WORD wID = LOWORD(wParam);         /* item, control, or accelerator identifier */
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

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

  case IDC_FILENAME:
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

  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  SetWindowLongA(hwnd,DWL_MSGRESULT,(LONG)fodInfos->Shell.FOIShellBrowser);

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
        TRACE("---\n");
        /* First send CDN_FILEOK as MSDN doc says */
        SendCustomDlgNotificationMessage(hwnd,CDN_FILEOK);
        if (GetWindowLongW(fodInfos->DlgInfos.hwndCustomDlg, DWL_MSGRESULT))
        {
            TRACE("canceled\n");
            return FALSE;
        }

        /* fodInfos->ofnInfos points to an ASCII or UNICODE structure as appropriate */
        SendMessageW(fodInfos->DlgInfos.hwndCustomDlg,
                     fodInfos->HookMsg.fileokstring, 0, (LPARAM)fodInfos->ofnInfos);
        if (GetWindowLongW(fodInfos->DlgInfos.hwndCustomDlg, DWL_MSGRESULT))
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
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  if(fodInfos->unicode)
  {
     LPOPENFILENAMEW ofn = (LPOPENFILENAMEW) fodInfos->ofnInfos;
     ofn->lpstrFile[0] = '\0';
  }
  else
  {
     LPOPENFILENAMEA ofn = fodInfos->ofnInfos;
     ofn->lpstrFile[0] = '\0';
  }

  SHGetPathFromIDListW( fodInfos->ShellInfos.pidlAbsCurrent, lpstrPathSpec );

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
        WCHAR nl[] = {'\n',0};

        LoadStringW(COMDLG32_hInstance, IDS_FILENOTFOUND, lpstrNotFound, 100);
        LoadStringW(COMDLG32_hInstance, IDS_VERIFYFILE, lpstrMsg, 100);

        strcpyW(tmp, lpstrTemp);
        strcatW(tmp, nl);
        strcatW(tmp, lpstrNotFound);
        strcatW(tmp, nl);
        strcatW(tmp, lpstrMsg);

        MessageBoxW(hwnd, tmp, fodInfos->title, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
      }

      /* move to the next file in the list of files */
      lpstrTemp += strlenW(lpstrTemp) + 1;
      COMDLG32_SHFree(pidl);
    }
  }

  nSizePath = strlenW(lpstrPathSpec) + 1;
  if ( !(fodInfos->ofnInfos->Flags & OFN_EXPLORER) )
  {
    /* For "oldstyle" dialog the components have to
       be separated by blanks (not '\0'!) and short
       filenames have to be used! */
    FIXME("Components have to be separated by blanks\n");
  }
  if(fodInfos->unicode)
  {
    LPOPENFILENAMEW ofn = (LPOPENFILENAMEW) fodInfos->ofnInfos;
    strcpyW( ofn->lpstrFile, lpstrPathSpec);
    memcpy( ofn->lpstrFile + nSizePath, lpstrFileList, sizeUsed*sizeof(WCHAR) );
  }
  else
  {
    LPOPENFILENAMEA ofn = fodInfos->ofnInfos;

    if (ofn->lpstrFile != NULL)
    {
      WideCharToMultiByte(CP_ACP, 0, lpstrPathSpec, -1,
			  ofn->lpstrFile, ofn->nMaxFile, NULL, NULL);
      if (ofn->nMaxFile > nSizePath)
      {
	WideCharToMultiByte(CP_ACP, 0, lpstrFileList, sizeUsed,
			    ofn->lpstrFile + nSizePath,
			    ofn->nMaxFile - nSizePath, NULL, NULL);
      }
    }
  }

  fodInfos->ofnInfos->nFileOffset = nSizePath + 1;
  fodInfos->ofnInfos->nFileExtension = 0;

  if ( !FILEDLG95_SendFileOK(hwnd, fodInfos) )
    return FALSE;

  /* clean and exit */
  FILEDLG95_Clean(hwnd);
  return EndDialog(hwnd,TRUE);
}

/***********************************************************************
 *      FILEDLG95_OnOpen
 *
 * Ok button WM_COMMAND message handler
 *
 * If the function succeeds, the return value is nonzero.
 */
#define ONOPEN_BROWSE 1
#define ONOPEN_OPEN   2
#define ONOPEN_SEARCH 3
static void FILEDLG95_OnOpenMessage(HWND hwnd, int idCaption, int idText)
{
  char strMsgTitle[MAX_PATH];
  char strMsgText [MAX_PATH];
  if (idCaption)
    LoadStringA(COMDLG32_hInstance, idCaption, strMsgTitle, sizeof(strMsgTitle));
  else
    strMsgTitle[0] = '\0';
  LoadStringA(COMDLG32_hInstance, idText, strMsgText, sizeof(strMsgText));
  MessageBoxA(hwnd,strMsgText, strMsgTitle, MB_OK | MB_ICONHAND);
}

BOOL FILEDLG95_OnOpen(HWND hwnd)
{
  LPWSTR lpstrFileList;
  UINT nFileCount = 0;
  UINT sizeUsed = 0;
  BOOL ret = TRUE;
  WCHAR lpstrPathAndFile[MAX_PATH];
  WCHAR lpstrTemp[MAX_PATH];
  LPSHELLFOLDER lpsf = NULL;
  int nOpenAction;
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("hwnd=%p\n", hwnd);

  /* get the files from the edit control */
  nFileCount = FILEDLG95_FILENAME_GetFileNames(hwnd, &lpstrFileList, &sizeUsed);

  /* try if the user selected a folder in the shellview */
  if(nFileCount == 0)
  {
      BrowseSelectedFolder(hwnd);
      return FALSE;
  }

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

  /* Get the current directory name */
  if (!SHGetPathFromIDListW(fodInfos->ShellInfos.pidlAbsCurrent, lpstrPathAndFile))
  {
    /* we are in a special folder, default to desktop */
    if(FAILED(COMDLG32_SHGetFolderPathW(hwnd, CSIDL_DESKTOPDIRECTORY|CSIDL_FLAG_CREATE, 0, 0, lpstrPathAndFile)))
    {
      /* last fallback */
      GetCurrentDirectoryW(MAX_PATH, lpstrPathAndFile);
    }
  }
  PathAddBackslashW(lpstrPathAndFile);

  TRACE("current directory=%s\n", debugstr_w(lpstrPathAndFile));

  /* if the user specifyed a fully qualified path use it */
  if(PathIsRelativeW(lpstrFileList))
  {
    strcatW(lpstrPathAndFile, lpstrFileList);
  }
  else
  {
    /* does the path have a drive letter? */
    if (PathGetDriveNumberW(lpstrFileList) == -1)
      strcpyW(lpstrPathAndFile+2, lpstrFileList);
    else
      strcpyW(lpstrPathAndFile, lpstrFileList);
  }

  /* resolve "." and ".." */
  PathCanonicalizeW(lpstrTemp, lpstrPathAndFile );
  strcpyW(lpstrPathAndFile, lpstrTemp);
  TRACE("canon=%s\n", debugstr_w(lpstrPathAndFile));

  MemFree(lpstrFileList);

/*
  Step 2: here we have a cleaned up path

  We have to parse the path step by step to see if we have to browse
  to a folder if the path points to a directory or the last
  valid element is a directory.

  valid variables:
    lpstrPathAndFile: cleaned up path
 */

  nOpenAction = ONOPEN_BROWSE;

  /* don't apply any checks with OFN_NOVALIDATE */
  {
    LPWSTR lpszTemp, lpszTemp1;
    LPITEMIDLIST pidl = NULL;
    WCHAR szwInvalid[] = { '/',':','<','>','|', 0};

    /* check for invalid chars */
    if((strpbrkW(lpstrPathAndFile+3, szwInvalid) != NULL) && !(fodInfos->ofnInfos->Flags & OFN_NOVALIDATE))
    {
      FILEDLG95_OnOpenMessage(hwnd, IDS_INVALID_FILENAME_TITLE, IDS_INVALID_FILENAME);
      ret = FALSE;
      goto ret;
    }

    if (FAILED (SHGetDesktopFolder(&lpsf))) return FALSE;

    lpszTemp1 = lpszTemp = lpstrPathAndFile;
    while (lpszTemp1)
    {
      LPSHELLFOLDER lpsfChild;
      WCHAR lpwstrTemp[MAX_PATH];
      DWORD dwEaten, dwAttributes;
      LPWSTR p;

      strcpyW(lpwstrTemp, lpszTemp);
      p = PathFindNextComponentW(lpwstrTemp);

      if (!p) break; /* end of path */

      *p = 0;
      lpszTemp = lpszTemp + strlenW(lpwstrTemp);

      if(*lpszTemp==0)
      {
        WCHAR wszWild[] = { '*', '?', 0 };
	/* if the last element is a wildcard do a search */
        if(strpbrkW(lpszTemp1, wszWild) != NULL)
        {
	  nOpenAction = ONOPEN_SEARCH;
	  break;
	}
      }
      lpszTemp1 = lpszTemp;

      TRACE("parse now=%s next=%s sf=%p\n",debugstr_w(lpwstrTemp), debugstr_w(lpszTemp), lpsf);

      if(lstrlenW(lpwstrTemp)==2) PathAddBackslashW(lpwstrTemp);

      dwAttributes = SFGAO_FOLDER;
      if(SUCCEEDED(IShellFolder_ParseDisplayName(lpsf, hwnd, NULL, lpwstrTemp, &dwEaten, &pidl, &dwAttributes)))
      {
        /* the path component is valid, we have a pidl of the next path component */
        TRACE("parse OK attr=0x%08lx pidl=%p\n", dwAttributes, pidl);
        if(dwAttributes & SFGAO_FOLDER)
        {
          if(FAILED(IShellFolder_BindToObject(lpsf, pidl, 0, &IID_IShellFolder, (LPVOID*)&lpsfChild)))
          {
            ERR("bind to failed\n"); /* should not fail */
            break;
          }
          IShellFolder_Release(lpsf);
          lpsf = lpsfChild;
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
      else if (!(fodInfos->ofnInfos->Flags & OFN_NOVALIDATE))
      {
	if(*lpszTemp)	/* points to trailing null for last path element */
        {
	  if(fodInfos->ofnInfos->Flags & OFN_PATHMUSTEXIST)
	  {
            FILEDLG95_OnOpenMessage(hwnd, 0, IDS_PATHNOTEXISTING);
	    break;
	  }
	}
        else
	{
          if( (fodInfos->ofnInfos->Flags & OFN_FILEMUSTEXIST) &&
             !( fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG ) )
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
  }

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
        if(fodInfos->ShellInfos.lpstrCurrentFilter)
	  MemFree((LPVOID)fodInfos->ShellInfos.lpstrCurrentFilter);
        len = strlenW(lpszTemp)+1;
        fodInfos->ShellInfos.lpstrCurrentFilter = MemAlloc(len * sizeof(WCHAR));
        strcpyW( fodInfos->ShellInfos.lpstrCurrentFilter, lpszTemp);

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
	    IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser, pidlCurrent, SBSP_ABSOLUTE);
	  }
	  else if( nOpenAction == ONOPEN_SEARCH )
	  {
            IShellView_Refresh(fodInfos->Shell.FOIShellView);
	  }
          COMDLG32_SHFree(pidlCurrent);
        }
      }
      ret = FALSE;
      break;
    case ONOPEN_OPEN:   /* fill in the return struct and close the dialog */
      TRACE("ONOPEN_OPEN %s\n", debugstr_w(lpstrPathAndFile));
      {
	/* add default extension */
	if (fodInfos->defext)
	{
	  WCHAR *ext = PathFindExtensionW(lpstrPathAndFile);
	  
	  if (! *ext)
	  {
	    /* only add "." in case a default extension does exist */
	    if (*fodInfos->defext != '\0')
	    {
                const WCHAR szwDot[] = {'.',0};
		int PathLength = strlenW(lpstrPathAndFile);

	        strcatW(lpstrPathAndFile, szwDot);
	        strcatW(lpstrPathAndFile, fodInfos->defext);

		/* In Open dialog: if file does not exist try without extension */
		if (!(fodInfos->DlgInfos.dwDlgProp & FODPROP_SAVEDLG)
		    && !PathFileExistsW(lpstrPathAndFile))
		  lpstrPathAndFile[PathLength] = '\0';
	    }
	  }

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
	  if (answer == IDNO)
	  {
	    ret = FALSE;
	    goto ret;
	  }
	}

        /* Check that the size of the file does not exceed buffer size.
             (Allow for extra \0 if OFN_MULTISELECT is set.) */
        if(strlenW(lpstrPathAndFile) < fodInfos->ofnInfos->nMaxFile -
            ((fodInfos->ofnInfos->Flags & OFN_ALLOWMULTISELECT) ? 1 : 0))
        {
          LPWSTR lpszTemp;

          /* fill destination buffer */
          if (fodInfos->ofnInfos->lpstrFile)
          {
             if(fodInfos->unicode)
             {
               LPOPENFILENAMEW ofn = (LPOPENFILENAMEW) fodInfos->ofnInfos;

               strncpyW(ofn->lpstrFile, lpstrPathAndFile, ofn->nMaxFile);
               if (ofn->Flags & OFN_ALLOWMULTISELECT)
                 ofn->lpstrFile[lstrlenW(ofn->lpstrFile) + 1] = '\0';
             }
             else
             {
               LPOPENFILENAMEA ofn = fodInfos->ofnInfos;

               WideCharToMultiByte(CP_ACP, 0, lpstrPathAndFile, -1,
                                   ofn->lpstrFile, ofn->nMaxFile, NULL, NULL);
               if (ofn->Flags & OFN_ALLOWMULTISELECT)
                 ofn->lpstrFile[lstrlenA(ofn->lpstrFile) + 1] = '\0';
             }
          }

          /* set filename offset */
          lpszTemp = PathFindFileNameW(lpstrPathAndFile);
          fodInfos->ofnInfos->nFileOffset = (lpszTemp - lpstrPathAndFile);

          /* set extension offset */
          lpszTemp = PathFindExtensionW(lpstrPathAndFile);
          fodInfos->ofnInfos->nFileExtension = (*lpszTemp) ? (lpszTemp - lpstrPathAndFile) + 1 : 0;

          /* set the lpstrFileTitle */
          if(fodInfos->ofnInfos->lpstrFileTitle)
	  {
            LPWSTR lpstrFileTitle = PathFindFileNameW(lpstrPathAndFile);
            if(fodInfos->unicode)
            {
              LPOPENFILENAMEW ofn = (LPOPENFILENAMEW) fodInfos->ofnInfos;
	      strncpyW(ofn->lpstrFileTitle, lpstrFileTitle, ofn->nMaxFileTitle);
            }
            else
            {
              LPOPENFILENAMEA ofn = fodInfos->ofnInfos;
              WideCharToMultiByte(CP_ACP, 0, lpstrFileTitle, -1,
                    ofn->lpstrFileTitle, ofn->nMaxFileTitle, NULL, NULL);
            }
	  }

          if ( !FILEDLG95_SendFileOK(hwnd, fodInfos) )
	      goto ret;

          TRACE("close\n");
	  FILEDLG95_Clean(hwnd);
          ret = EndDialog(hwnd, TRUE);
	}
	else
        {
          /* FIXME set error FNERR_BUFFERTOSMALL */
          FILEDLG95_Clean(hwnd);
          ret = EndDialog(hwnd, FALSE);
        }
        goto ret;
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
static HRESULT FILEDLG95_SHELL_Init(HWND hwnd)
{
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

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
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

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
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  if(SUCCEEDED(IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser,
                                          NULL,
                                          SBSP_PARENT)))
  {
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
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);
  LPITEMIDLIST pidl;
  HRESULT hres;

  TRACE("\n");

  SHGetSpecialFolderLocation(0,CSIDL_DESKTOP,&pidl);
  hres = IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser, pidl, SBSP_ABSOLUTE);
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
    FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

    TRACE("\n");

    COMDLG32_SHFree(fodInfos->ShellInfos.pidlAbsCurrent);

    /* clean Shell interfaces */
    IShellView_DestroyViewWindow(fodInfos->Shell.FOIShellView);
    IShellView_Release(fodInfos->Shell.FOIShellView);
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
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("\n");

  if(fodInfos->filter)
  {
    int nFilters = 0;	/* number of filters */
    LPWSTR lpstrFilter;
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
      lpstrPos += strlenW(lpstrPos) + 1;

      /* Copy the extensions */
      if (! *lpstrPos) return E_FAIL;	/* malformed filter */
      if (!(lpstrExt = MemAlloc((strlenW(lpstrPos)+1)*sizeof(WCHAR)))) return E_FAIL;
      strcpyW(lpstrExt,lpstrPos);
      lpstrPos += strlenW(lpstrPos) + 1;

      /* Add the item at the end of the combo */
      CBAddStringW(fodInfos->DlgInfos.hwndFileTypeCB, lpstrDisplay);
      CBSetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB, nFilters, lpstrExt);
      nFilters++;
    }
    /*
     * Set the current filter to the one specified
     * in the initialisation structure
     * FIXME: lpstrCustomFilter not handled at all
     */

    /* set default filter index */
    if(fodInfos->ofnInfos->nFilterIndex == 0 && fodInfos->customfilter == NULL)
      fodInfos->ofnInfos->nFilterIndex = 1;

    /* First, check to make sure our index isn't out of bounds. */
    if ( fodInfos->ofnInfos->nFilterIndex > nFilters )
      fodInfos->ofnInfos->nFilterIndex = nFilters;

    /* Set the current index selection. */
    CBSetCurSel(fodInfos->DlgInfos.hwndFileTypeCB, fodInfos->ofnInfos->nFilterIndex-1);

    /* Get the corresponding text string from the combo box. */
    lpstrFilter = (LPWSTR) CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,
                                             fodInfos->ofnInfos->nFilterIndex-1);

    if ((INT)lpstrFilter == CB_ERR)  /* control is empty */
      lpstrFilter = NULL;

    if(lpstrFilter)
    {
      DWORD len;
      CharLowerW(lpstrFilter); /* lowercase */
      len = strlenW(lpstrFilter)+1;
      fodInfos->ShellInfos.lpstrCurrentFilter = MemAlloc( len * sizeof(WCHAR) );
      strcpyW(fodInfos->ShellInfos.lpstrCurrentFilter,lpstrFilter);
    }
  }
  return NOERROR;
}

/***********************************************************************
 *      FILEDLG95_FILETYPE_OnCommand
 *
 * WM_COMMAND of the file type combo box
 * If the function succeeds, the return value is nonzero.
 */
static BOOL FILEDLG95_FILETYPE_OnCommand(HWND hwnd, WORD wNotifyCode)
{
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  switch(wNotifyCode)
  {
    case CBN_SELENDOK:
    {
      LPWSTR lpstrFilter;

      /* Get the current item of the filetype combo box */
      int iItem = CBGetCurSel(fodInfos->DlgInfos.hwndFileTypeCB);

      /* set the current filter index - indexed from 1 */
      fodInfos->ofnInfos->nFilterIndex = iItem + 1;

      /* Set the current filter with the current selection */
      if(fodInfos->ShellInfos.lpstrCurrentFilter)
         MemFree((LPVOID)fodInfos->ShellInfos.lpstrCurrentFilter);

      lpstrFilter = (LPWSTR) CBGetItemDataPtr(fodInfos->DlgInfos.hwndFileTypeCB,
                                             iItem);
      if((int)lpstrFilter != CB_ERR)
      {
          DWORD len;
          CharLowerW(lpstrFilter); /* lowercase */
          len = strlenW(lpstrFilter)+1;
          fodInfos->ShellInfos.lpstrCurrentFilter = MemAlloc( len * sizeof(WCHAR) );
          strcpyW(fodInfos->ShellInfos.lpstrCurrentFilter,lpstrFilter);
          SendCustomDlgNotificationMessage(hwnd,CDN_TYPECHANGE);
      }

      /* Refresh the actual view to display the included items*/
      IShellView_Refresh(fodInfos->Shell.FOIShellView);
    }
  }
  return FALSE;
}
/***********************************************************************
 *      FILEDLG95_FILETYPE_SearchExt
 *
 * searches for a extension in the filetype box
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
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);
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
  if(fodInfos->ShellInfos.lpstrCurrentFilter)
     MemFree(fodInfos->ShellInfos.lpstrCurrentFilter);

}

/***********************************************************************
 *      FILEDLG95_LOOKIN_Init
 *
 * Initialisation of the look in combo box
 */
static HRESULT FILEDLG95_LOOKIN_Init(HWND hwndCombo)
{
  IShellFolder	*psfRoot, *psfDrives;
  IEnumIDList	*lpeRoot, *lpeDrives;
  LPITEMIDLIST	pidlDrives, pidlTmp, pidlTmp1, pidlAbsTmp;

  LookInInfos *liInfos = MemAlloc(sizeof(LookInInfos));

  TRACE("\n");

  liInfos->iMaxIndentation = 0;

  SetPropA(hwndCombo, LookInInfosStr, (HANDLE) liInfos);

  /* set item height for both text field and listbox */
  CBSetItemHeight(hwndCombo,-1,GetSystemMetrics(SM_CYSMICON));
  CBSetItemHeight(hwndCombo,0,GetSystemMetrics(SM_CYSMICON));
   
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
        COMDLG32_SHFree(pidlTmp);
      }
      IEnumIDList_Release(lpeRoot);
    }
    IShellFolder_Release(psfRoot);
  }

  COMDLG32_SHFree(pidlDrives);
  return NOERROR;
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
  SHFILEINFOA sfi;
  HIMAGELIST ilItemImage;
  int iIndentation;
  TEXTMETRICA tm;
  LPSFOLDER tmpFolder;


  LookInInfos *liInfos = (LookInInfos *)GetPropA(pDIStruct->hwndItem,LookInInfosStr);

  TRACE("\n");

  if(pDIStruct->itemID == -1)
    return 0;

  if(!(tmpFolder = (LPSFOLDER) CBGetItemDataPtr(pDIStruct->hwndItem,
                            pDIStruct->itemID)))
    return 0;


  if(pDIStruct->itemID == liInfos->uSelectedItem)
  {
    ilItemImage = (HIMAGELIST) SHGetFileInfoA ((LPCSTR) tmpFolder->pidlItem,
                                               0,
                                               &sfi,
                                               sizeof (SHFILEINFOA),
                                               SHGFI_PIDL | SHGFI_SMALLICON |
                                               SHGFI_OPENICON | SHGFI_SYSICONINDEX    |
                                               SHGFI_DISPLAYNAME );
  }
  else
  {
    ilItemImage = (HIMAGELIST) SHGetFileInfoA ((LPCSTR) tmpFolder->pidlItem,
                                                  0,
                                                  &sfi,
                                                  sizeof (SHFILEINFOA),
                                                  SHGFI_PIDL | SHGFI_SMALLICON |
                                                  SHGFI_SYSICONINDEX |
                                                  SHGFI_DISPLAYNAME);
  }

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
  {
    iIndentation = 0;
    ilItemImage = (HIMAGELIST) SHGetFileInfoA ((LPCSTR) tmpFolder->pidlItem,
                                                0,
                                                &sfi,
                                                sizeof (SHFILEINFOA),
                                                SHGFI_PIDL | SHGFI_SMALLICON | SHGFI_OPENICON
                                                | SHGFI_SYSICONINDEX | SHGFI_DISPLAYNAME  );

  }
  else
  {
    iIndentation = tmpFolder->m_iIndent;
  }
  /* Draw text and icon */

  /* Initialise the icon display area */
  rectIcon.left = pDIStruct->rcItem.left + ICONWIDTH/2 * iIndentation;
  rectIcon.top = pDIStruct->rcItem.top;
  rectIcon.right = rectIcon.left + ICONWIDTH;
  rectIcon.bottom = pDIStruct->rcItem.bottom;

  /* Initialise the text display area */
  GetTextMetricsA(pDIStruct->hDC, &tm);
  rectText.left = rectIcon.right;
  rectText.top =
	  (pDIStruct->rcItem.top + pDIStruct->rcItem.bottom - tm.tmHeight) / 2;
  rectText.right = pDIStruct->rcItem.right + XTEXTOFFSET;
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
  if(sfi.szDisplayName)
    TextOutA(pDIStruct->hDC,rectText.left,rectText.top,sfi.szDisplayName,strlen(sfi.szDisplayName));


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
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

  TRACE("%p\n", fodInfos);

  switch(wNotifyCode)
  {
    case CBN_SELENDOK:
    {
      LPSFOLDER tmpFolder;
      int iItem;

      iItem = CBGetCurSel(fodInfos->DlgInfos.hwndLookInCB);

      if(!(tmpFolder = (LPSFOLDER) CBGetItemDataPtr(fodInfos->DlgInfos.hwndLookInCB,
                                               iItem)))
	return FALSE;


      if(SUCCEEDED(IShellBrowser_BrowseObject(fodInfos->Shell.FOIShellBrowser,
                                              tmpFolder->pidlItem,
                                              SBSP_ABSOLUTE)))
      {
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
  SHFILEINFOA sfi;
  SFOLDER *tmpFolder;
  LookInInfos *liInfos;

  TRACE("%08x\n", iInsertId);

  if(!pidl)
    return -1;

  if(!(liInfos = (LookInInfos *)GetPropA(hwnd,LookInInfosStr)))
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
  SHGetFileInfoA((LPSTR)pidl,
                  0,
                  &sfi,
                  sizeof(sfi),
                  SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX
                  | SHGFI_PIDL | SHGFI_SMALLICON | SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED);

  TRACE("-- Add %s attr=%08lx\n", sfi.szDisplayName, sfi.dwAttributes);

  if((sfi.dwAttributes & SFGAO_FILESYSANCESTOR) || (sfi.dwAttributes & SFGAO_FILESYSTEM))
  {
    int iItemID;

    TRACE("-- Add %s at %u\n", sfi.szDisplayName, tmpFolder->m_iIndent);

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

  iParentPos = FILEDLG95_LOOKIN_SearchItem(hwnd,(WPARAM)pidlParent,SEARCH_PIDL);

  if(iParentPos < 0)
  {
    iParentPos = FILEDLG95_LOOKIN_InsertItemAfterParent(hwnd,pidlParent);
  }

  /* Free pidlParent memory */
  COMDLG32_SHFree((LPVOID)pidlParent);

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

  liInfos = (LookInInfos *)GetPropA(hwnd,LookInInfosStr);

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

  LookInInfos *liInfos = (LookInInfos *)GetPropA(hwnd,LookInInfosStr);

  TRACE("\n");

  if(liInfos->iMaxIndentation <= 2)
    return -1;

  if((iItemPos = FILEDLG95_LOOKIN_SearchItem(hwnd,(WPARAM)liInfos->iMaxIndentation,SEARCH_EXP)) >=0)
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

  TRACE("0x%08x 0x%x\n",searchArg, iSearchMethod);

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
    FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);
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
    char              lpstrTemp[MAX_PATH];
    LPSTR             lpstrAllFile = NULL, lpstrCurrFile = NULL;

    TRACE("\n");
    fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

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
            nLength += strlen( lpstrTemp ) + 3;
            nFiles++;
	  }
          COMDLG32_SHFree( pidl );
	}
      }
    }

    /* allocate the buffer */
    if (nFiles <= 1) nLength = MAX_PATH;
    lpstrAllFile = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nLength);
    lpstrAllFile[0] = '\0';

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
              strcpy( lpstrCurrFile, lpstrTemp );
              lpstrCurrFile += strlen( lpstrTemp );
              strcpy( lpstrCurrFile, "\" " );
              lpstrCurrFile += 2;
	    }
	    else
	    {
              strcpy( lpstrAllFile, lpstrTemp );
	    }
          }
          COMDLG32_SHFree( (LPVOID) pidl );
	}
      }
      SetWindowTextA( fodInfos->DlgInfos.hwndFileName, lpstrAllFile );
       
      /* Select the file name like Windows does */ 
      SendMessageA(fodInfos->DlgInfos.hwndFileName, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
    }
    HeapFree(GetProcessHeap(),0, lpstrAllFile );
}


/* copied from shell32 to avoid linking to it */
static HRESULT COMDLG32_StrRetToStrNA (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl)
{
	switch (src->uType)
	{
	  case STRRET_WSTR:
	    WideCharToMultiByte(CP_ACP, 0, src->u.pOleStr, -1, (LPSTR)dest, len, NULL, NULL);
	    COMDLG32_SHFree(src->u.pOleStr);
	    break;

	  case STRRET_CSTR:
	    lstrcpynA((LPSTR)dest, src->u.cStr, len);
	    break;

	  case STRRET_OFFSET:
	    lstrcpynA((LPSTR)dest, ((LPCSTR)&pidl->mkid)+src->u.uOffset, len);
	    break;

	  default:
	    FIXME("unknown type!\n");
	    if (len)
	    {
	      *(LPSTR)dest = '\0';
	    }
	    return(FALSE);
	}
	return S_OK;
}

/***********************************************************************
 * FILEDLG95_FILENAME_GetFileNames
 *
 * copies the filenames to a 0-delimited string list (A\0B\0C\0\0)
 */
int FILEDLG95_FILENAME_GetFileNames (HWND hwnd, LPWSTR * lpstrFileList, UINT * sizeUsed)
{
	FileOpenDlgInfos *fodInfos  = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);
	UINT nStrCharCount = 0;	/* index in src buffer */
	UINT nFileIndex = 0;	/* index in dest buffer */
	UINT nFileCount = 0;	/* number of files */
	UINT nStrLen = 0;	/* length of string in edit control */
	LPWSTR lpstrEdit;	/* buffer for string from edit control */

	TRACE("\n");

	/* get the filenames from the edit control */
	nStrLen = SendMessageW(fodInfos->DlgInfos.hwndFileName, WM_GETTEXTLENGTH, 0, 0);
	lpstrEdit = MemAlloc( (nStrLen+1)*sizeof(WCHAR) );
	GetDlgItemTextW(hwnd, IDC_FILENAME, lpstrEdit, nStrLen+1);

	TRACE("nStrLen=%u str=%s\n", nStrLen, debugstr_w(lpstrEdit));

	/* we might get single filename without any '"',
	 * so we need nStrLen + terminating \0 + end-of-list \0 */
	*lpstrFileList = MemAlloc( (nStrLen+2)*sizeof(WCHAR) );
	*sizeUsed = 0;

	/* build 0-delimited file list from filenames */
	while ( nStrCharCount <= nStrLen )
	{
	  if ( lpstrEdit[nStrCharCount]=='"' )
	  {
	    nStrCharCount++;
	    while ((lpstrEdit[nStrCharCount]!='"') && (nStrCharCount <= nStrLen))
	    {
	      (*lpstrFileList)[nFileIndex++] = lpstrEdit[nStrCharCount];
	      (*sizeUsed)++;
	      nStrCharCount++;
	    }
	    (*lpstrFileList)[nFileIndex++] = '\0';
	    (*sizeUsed)++;
	    nFileCount++;
	  }
	  nStrCharCount++;
	}

	/* single, unquoted string */
	if ((nStrLen > 0) && (*sizeUsed == 0) )
	{
	  strcpyW(*lpstrFileList, lpstrEdit);
	  nFileIndex = strlenW(lpstrEdit) + 1;
	  (*sizeUsed) = nFileIndex;
	  nFileCount = 1;
	}

	/* trailing \0 */
	(*lpstrFileList)[nFileIndex] = '\0';
	(*sizeUsed)++;

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

    /* Set the FORMATETC structure*/
    SETDefFormatEtc(formatetc, RegisterClipboardFormatA(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);

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
UINT GetNumSelected( IDataObject *doSelected )
{
    UINT retVal = 0;
    STGMEDIUM medium;
    FORMATETC formatetc;

    TRACE("sv=%p\n", doSelected);

    if (!doSelected) return 0;

    /* Set the FORMATETC structure*/
    SETDefFormatEtc(formatetc, RegisterClipboardFormatA(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);

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

HRESULT GetName(LPSHELLFOLDER lpsf, LPITEMIDLIST pidl,DWORD dwFlags,LPSTR lpstrFileName)
{
  STRRET str;
  HRESULT hRes;

  TRACE("sf=%p pidl=%p\n", lpsf, pidl);

  if(!lpsf)
  {
    HRESULT hRes;
    SHGetDesktopFolder(&lpsf);
    hRes = GetName(lpsf,pidl,dwFlags,lpstrFileName);
    IShellFolder_Release(lpsf);
    return hRes;
  }

  /* Get the display name of the pidl relative to the folder */
  if (SUCCEEDED(hRes = IShellFolder_GetDisplayNameOf(lpsf, pidl, dwFlags, &str)))
  {
      return COMDLG32_StrRetToStrNA(lpstrFileName, MAX_PATH, &str, pidl);
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
LPITEMIDLIST GetPidlFromName(IShellFolder *lpsf,LPWSTR lpcstrFileName)
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
BOOL IsPidlFolder (LPSHELLFOLDER psf, LPCITEMIDLIST pidl)
{
	ULONG uAttr  = SFGAO_FOLDER | SFGAO_HASSUBFOLDER;
	HRESULT ret;

	TRACE("%p, %p\n", psf, pidl);

  	ret = IShellFolder_GetAttributesOf( psf, 1, &pidl, &uAttr );

	TRACE("-- 0x%08lx 0x%08lx\n", uAttr, ret);
	/* see documentation shell 4.1*/
        return uAttr & (SFGAO_FOLDER | SFGAO_HASSUBFOLDER);
}

/***********************************************************************
 *      BrowseSelectedFolder
 */
static BOOL BrowseSelectedFolder(HWND hwnd)
{
  BOOL bBrowseSelFolder = FALSE;
  FileOpenDlgInfos *fodInfos = (FileOpenDlgInfos *) GetPropA(hwnd,FileOpenDlgInfosStr);

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
               WCHAR notexist[] = {'P','a','t','h',' ','d','o','e','s',
                                   ' ','n','o','t',' ','e','x','i','s','t',0};
               MessageBoxW( hwnd, notexist, fodInfos->title, MB_OK | MB_ICONEXCLAMATION );
          }

         bBrowseSelFolder = TRUE;
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
    if(mem)
    {
        HeapFree(GetProcessHeap(),0,mem);
    }
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
	return GetFileDialog95W(ofn, SAVE_DIALOG);
}
