/*
 *	OLEDLG library
 *
 *	Copyright 2003	Ulrich Czekalla for CodeWeavers
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
 */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"

#include "oledlg.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledlg);

typedef struct
{
    HWND hwndSelf;
    BOOL bObjListInit; /* Object list has been initialized */
    LPOLEUIINSERTOBJECTA lpOleUIInsertObject;

    HWND hwndObjTypeLBL;
    HWND hwndObjTypeLB;
    HWND hwndFileLBL;
    HWND hwndFileTB;
    HWND hwndCreateCtrlCB;
    HWND hwndCreateNewCB;
    HWND hwndCreateFromFileCB;
    HWND hwndDisplayIconCB;
    HWND hwndAddCtrlBTN;
    HWND hwndBrowseBTN;
    HWND hwndResultDesc;

} InsertObjectDlgInfo;

static INT_PTR CALLBACK UIInsertObjectDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT UIINSOBJDLG_OnWMCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
static void UIINSERTOBJECTDLG_InitDialog(InsertObjectDlgInfo* pdlgInfo);
static void UIINSERTOBJECTDLG_SelectCreateCtrl(InsertObjectDlgInfo* pdlgInfo);
static void UIINSERTOBJECTDLG_SelectCreateFromFile(InsertObjectDlgInfo* pdlgInfo);
static void UIINSERTOBJECTDLG_SelectCreateNew(InsertObjectDlgInfo* pdlgInfo);
static BOOL UIINSERTOBJECTDLG_PopulateObjectTypes(InsertObjectDlgInfo* pdlgInfo);
static void UIINSERTOBJECTDLG_FreeObjectTypes(InsertObjectDlgInfo* pdlgInfo);
static void UIINSERTOBJECTDLG_SelChange(InsertObjectDlgInfo* pdlgInfo);
static BOOL UIINSERTOBJECTDLG_OnOpen(InsertObjectDlgInfo* pdlgInfo);
static void UIINSERTOBJECTDLG_BrowseFile(InsertObjectDlgInfo* pdlgInfo);
static void UIINSERTOBJECTDLG_AddControl(InsertObjectDlgInfo* pdlgInfo);

typedef HRESULT (*DLLREGISTER)          (void);

extern HINSTANCE OLEDLG_hInstance;
static const WCHAR OleUIInsertObjectInfoStr[] = {'O','l','e','U','I','I','n','s','e','r','t','O','b','j','e','c','t',
    'I','n','f','o','S','t','r',0};

/***********************************************************************
 *           OleUIInsertObjectA (OLEDLG.3)
 */
UINT WINAPI OleUIInsertObjectA(LPOLEUIINSERTOBJECTA lpOleUIInsertObject)
{
  LRESULT lRes;
  LPCVOID template;
  HRSRC hRes;
  InsertObjectDlgInfo dlgInfo;
  HANDLE hDlgTmpl = 0;

  if (lpOleUIInsertObject->lpszTemplate || lpOleUIInsertObject->hResource)
     FIXME("Customized template not supported\n");

  /* Create the dialog from a template */
  if(!(hRes = FindResourceA(OLEDLG_hInstance,MAKEINTRESOURCEA(UIINSERTOBJECT),
      (LPSTR)RT_DIALOG)))
  {
      return OLEUI_ERR_FINDTEMPLATEFAILURE;
  }

  if (!(hDlgTmpl = LoadResource(OLEDLG_hInstance, hRes )) ||
      !(template = LockResource( hDlgTmpl )))
  {
      return OLEUI_ERR_LOADTEMPLATEFAILURE;
  }

  /* Initialize InsertObjectDlgInfo structure */
  dlgInfo.lpOleUIInsertObject = lpOleUIInsertObject;
  dlgInfo.bObjListInit = FALSE;

  lRes = DialogBoxIndirectParamA(OLEDLG_hInstance, (const DLGTEMPLATE*) template,
      lpOleUIInsertObject->hWndOwner, UIInsertObjectDlgProc,
      (LPARAM) &dlgInfo);

    /* Unable to create the dialog */
    if( lRes == -1)
        return OLEUI_ERR_DIALOGFAILURE;

    return lRes;
}


/***********************************************************************
 *          UIInsertObjectDlgProc
 *
 * OLE UI Insert Object dialog procedure
 */
INT_PTR CALLBACK UIInsertObjectDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  InsertObjectDlgInfo* pdlgInfo = GetPropW(hwnd, OleUIInsertObjectInfoStr);

  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
        InsertObjectDlgInfo* pdlgInfo = (InsertObjectDlgInfo*)lParam;

        pdlgInfo->hwndSelf = hwnd;

        SetPropW(hwnd, OleUIInsertObjectInfoStr, pdlgInfo);

        UIINSERTOBJECTDLG_InitDialog(pdlgInfo);

        return 0;
    }

    case WM_COMMAND:
      return UIINSOBJDLG_OnWMCommand(hwnd, wParam, lParam);

    case WM_DESTROY:
      if (pdlgInfo)
          UIINSERTOBJECTDLG_FreeObjectTypes(pdlgInfo);
      RemovePropW(hwnd, OleUIInsertObjectInfoStr);
      return FALSE;

    default :
      return FALSE;
  }
}


/***********************************************************************
 *      UIINSOBJDLG_OnWMCommand
 *
 * WM_COMMAND message handler
 */
static LRESULT UIINSOBJDLG_OnWMCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  InsertObjectDlgInfo* pdlgInfo = GetPropW(hwnd, OleUIInsertObjectInfoStr);
  WORD wNotifyCode = HIWORD(wParam);
  WORD wID = LOWORD(wParam);

  switch(wID)
  {
    case IDOK:
      EndDialog(hwnd, UIINSERTOBJECTDLG_OnOpen(pdlgInfo));
      break;

    case IDCANCEL:
      EndDialog(hwnd, FALSE);
      break;

    case IDC_CREATECONTROL:
      UIINSERTOBJECTDLG_SelectCreateCtrl(pdlgInfo);
      break;

    case IDC_CREATENEW:
      UIINSERTOBJECTDLG_SelectCreateNew(pdlgInfo);
      break;

    case IDC_CREATEFROMFILE:
      UIINSERTOBJECTDLG_SelectCreateFromFile(pdlgInfo);
      break;

    case IDC_BROWSE:
      UIINSERTOBJECTDLG_BrowseFile(pdlgInfo);
      break;

    case IDC_ADDCONTROL:
      UIINSERTOBJECTDLG_AddControl(pdlgInfo);
      break;

    case IDC_OBJTYPELIST:
      if (wNotifyCode == LBN_SELCHANGE)
        UIINSERTOBJECTDLG_SelChange(pdlgInfo);
      break;
  }
  return 0;
}


/***********************************************************************
 *      UIINSERTOBJECTDLG_InitDialog
 *
 * Initialize dialog display
 */
static void UIINSERTOBJECTDLG_InitDialog(InsertObjectDlgInfo* pdlgInfo)
{
  /* Initialize InsertObjectDlgInfo data structure */
  pdlgInfo->hwndObjTypeLB = GetDlgItem(pdlgInfo->hwndSelf, IDC_OBJTYPELIST);
  pdlgInfo->hwndObjTypeLBL = GetDlgItem(pdlgInfo->hwndSelf, IDC_OBJTYPELBL);
  pdlgInfo->hwndFileLBL = GetDlgItem(pdlgInfo->hwndSelf, IDC_FILELBL);
  pdlgInfo->hwndFileTB = GetDlgItem(pdlgInfo->hwndSelf, IDC_FILE);
  pdlgInfo->hwndCreateCtrlCB = GetDlgItem(pdlgInfo->hwndSelf, IDC_CREATECONTROL);
  pdlgInfo->hwndCreateNewCB = GetDlgItem(pdlgInfo->hwndSelf, IDC_CREATENEW);
  pdlgInfo->hwndCreateFromFileCB = GetDlgItem(pdlgInfo->hwndSelf, IDC_CREATEFROMFILE);
  pdlgInfo->hwndDisplayIconCB = GetDlgItem(pdlgInfo->hwndSelf, IDC_ASICON);
  pdlgInfo->hwndAddCtrlBTN = GetDlgItem(pdlgInfo->hwndSelf, IDC_ADDCONTROL);
  pdlgInfo->hwndBrowseBTN = GetDlgItem(pdlgInfo->hwndSelf, IDC_BROWSE);
  pdlgInfo->hwndResultDesc = GetDlgItem(pdlgInfo->hwndSelf, IDC_RESULTDESC);

  /* Setup dialog controls based on flags */
  if (pdlgInfo->lpOleUIInsertObject->lpszCaption)
     SetWindowTextA(pdlgInfo->hwndSelf, pdlgInfo->lpOleUIInsertObject->lpszCaption);

  ShowWindow(pdlgInfo->hwndCreateCtrlCB, (pdlgInfo->lpOleUIInsertObject->dwFlags &
    IOF_SHOWINSERTCONTROL) ? SW_SHOW : SW_HIDE);
  ShowWindow(pdlgInfo->hwndDisplayIconCB, (pdlgInfo->lpOleUIInsertObject->dwFlags &
    IOF_CHECKDISPLAYASICON) ? SW_SHOW : SW_HIDE);
  EnableWindow(pdlgInfo->hwndDisplayIconCB, !(pdlgInfo->lpOleUIInsertObject->dwFlags &
    IOF_DISABLEDISPLAYASICON));

  if (pdlgInfo->lpOleUIInsertObject->dwFlags & IOF_SELECTCREATECONTROL)
    UIINSERTOBJECTDLG_SelectCreateCtrl(pdlgInfo);
  else if (pdlgInfo->lpOleUIInsertObject->dwFlags & IOF_SELECTCREATEFROMFILE)
    UIINSERTOBJECTDLG_SelectCreateFromFile(pdlgInfo);
  else /* (pdlgInfo->lpOleUIInsertObject->dwFlags & IOF_SELECTCREATENEW) */
    UIINSERTOBJECTDLG_SelectCreateNew(pdlgInfo);
}


/***********************************************************************
 *      UIINSERTOBJECTDLG_SelectCreateCtrl
 *
 * Select Create Control Radio Button
 */
static void UIINSERTOBJECTDLG_SelectCreateCtrl(InsertObjectDlgInfo* pdlgInfo)
{
  ShowWindow(pdlgInfo->hwndDisplayIconCB, SW_HIDE);
  ShowWindow(pdlgInfo->hwndFileLBL, SW_HIDE);
  ShowWindow(pdlgInfo->hwndFileTB, SW_HIDE);
  ShowWindow(pdlgInfo->hwndBrowseBTN, SW_HIDE);

  ShowWindow(pdlgInfo->hwndObjTypeLBL, SW_SHOW);
  ShowWindow(pdlgInfo->hwndObjTypeLB, SW_SHOW);
  ShowWindow(pdlgInfo->hwndAddCtrlBTN, SW_SHOW);

  SendMessageA(pdlgInfo->hwndCreateCtrlCB, BM_SETCHECK, BST_CHECKED, 0);

  /* Populate object type listbox */
  if (!pdlgInfo->bObjListInit)
     UIINSERTOBJECTDLG_PopulateObjectTypes(pdlgInfo);
}


/***********************************************************************
 *      UIINSERTOBJECTDLG_SelectCreateNew
 *
 * Select Create New Radio Button
 */
static void UIINSERTOBJECTDLG_SelectCreateNew(InsertObjectDlgInfo* pdlgInfo)
{
  ShowWindow(pdlgInfo->hwndFileLBL, SW_HIDE);
  ShowWindow(pdlgInfo->hwndFileTB, SW_HIDE);
  ShowWindow(pdlgInfo->hwndAddCtrlBTN, SW_HIDE);
  ShowWindow(pdlgInfo->hwndBrowseBTN, SW_HIDE);

  if (pdlgInfo->lpOleUIInsertObject->dwFlags & IOF_CHECKDISPLAYASICON)
    ShowWindow(pdlgInfo->hwndDisplayIconCB, SW_SHOW);

  ShowWindow(pdlgInfo->hwndObjTypeLBL, SW_SHOW);
  ShowWindow(pdlgInfo->hwndObjTypeLB, SW_SHOW);

  SendMessageA(pdlgInfo->hwndCreateNewCB, BM_SETCHECK, BST_CHECKED, 0);

  if (!pdlgInfo->bObjListInit)
     UIINSERTOBJECTDLG_PopulateObjectTypes(pdlgInfo);

  UIINSERTOBJECTDLG_SelChange(pdlgInfo);
}


/***********************************************************************
 *      UIINSERTOBJECTDLG_SelectCreateFromFile
 *
 * Select Create From File Radio Button
 */
static void UIINSERTOBJECTDLG_SelectCreateFromFile(InsertObjectDlgInfo* pdlgInfo)
{
  WCHAR resstr[MAX_PATH];

  ShowWindow(pdlgInfo->hwndAddCtrlBTN, SW_HIDE);
  ShowWindow(pdlgInfo->hwndObjTypeLBL, SW_HIDE);
  ShowWindow(pdlgInfo->hwndObjTypeLB, SW_HIDE);

  if (pdlgInfo->lpOleUIInsertObject->dwFlags & IOF_CHECKDISPLAYASICON)
    ShowWindow(pdlgInfo->hwndDisplayIconCB, SW_SHOW);

  ShowWindow(pdlgInfo->hwndFileLBL, SW_SHOW);
  ShowWindow(pdlgInfo->hwndFileTB, SW_SHOW);
  ShowWindow(pdlgInfo->hwndBrowseBTN, SW_SHOW);

  SendMessageW(pdlgInfo->hwndCreateFromFileCB, BM_SETCHECK, BST_CHECKED, 0);

  if (LoadStringW(OLEDLG_hInstance, IDS_RESULTFILEOBJDESC, resstr, MAX_PATH))
     SendMessageW(pdlgInfo->hwndResultDesc, WM_SETTEXT, 0, (LPARAM)resstr);
}


/***********************************************************************
 *      UIINSERTOBJECTDLG_PopulateObjectTypes
 *
 * Populate Object Type listbox
 */
static BOOL UIINSERTOBJECTDLG_PopulateObjectTypes(InsertObjectDlgInfo* pdlgInfo)
{
  static const WCHAR szClsid[] = {'C','L','S','I','D',0};
  static const WCHAR szInsertable[] = {'I','n','s','e','r','t','a','b','l','e',0};
  static const WCHAR szNotInsertable[] = {'N','o','t','I','n','s','e','r','t','a','b','l','e',0};
  DWORD i;
  LONG len;
  HKEY hkclsids;
  HKEY hkey;
  CLSID clsid;
  LSTATUS ret;
  WCHAR keydesc[MAX_PATH];
  WCHAR keyname[MAX_PATH];
  WCHAR szclsid[128];
  DWORD index = 0;

  UIINSERTOBJECTDLG_FreeObjectTypes(pdlgInfo);

  RegOpenKeyExW(HKEY_CLASSES_ROOT, szClsid, 0, KEY_READ, &hkclsids);

  while (ERROR_SUCCESS == (ret = RegEnumKeyW(hkclsids, index, szclsid, ARRAY_SIZE(szclsid))))
  {
    index++;

    RegOpenKeyExW(hkclsids, szclsid, 0, KEY_READ, &hkey);

    len = sizeof(keyname);
    if (ERROR_SUCCESS != RegQueryValueW(hkey, szInsertable, keyname, &len))
        continue;

    len = sizeof(keyname);
    if (ERROR_SUCCESS == RegQueryValueW(hkey, szNotInsertable, keyname, &len))
        continue;

    CLSIDFromString(szclsid, &clsid);

    for (i = 0; i < pdlgInfo->lpOleUIInsertObject->cClsidExclude; i++)
      if (IsEqualGUID(&pdlgInfo->lpOleUIInsertObject->lpClsidExclude[i], &clsid))
        break;

    if (i < pdlgInfo->lpOleUIInsertObject->cClsidExclude)
      continue;

    len = sizeof(keydesc);
    if (ERROR_SUCCESS == RegQueryValueW(hkey, NULL, keydesc, &len))
    {
       CLSID* lpclsid = HeapAlloc(GetProcessHeap(), 0, sizeof(CLSID));
       *lpclsid = clsid;

       len = SendMessageW(pdlgInfo->hwndObjTypeLB, LB_ADDSTRING, 0, (LPARAM)keydesc);
       SendMessageW(pdlgInfo->hwndObjTypeLB, LB_SETITEMDATA, len, (LPARAM)lpclsid);
    }
  }

  pdlgInfo->bObjListInit = (ret == ERROR_NO_MORE_ITEMS);

  return pdlgInfo->bObjListInit;
}


/***********************************************************************
 *      UIINSERTOBJECTDLG_FreeObjectTypes
 *
 * Free Object Types listbox
 */
static void UIINSERTOBJECTDLG_FreeObjectTypes(InsertObjectDlgInfo* pdlgInfo)
{
  UINT i, count;

  count = SendMessageA(pdlgInfo->hwndObjTypeLB, LB_GETCOUNT, 0, 0);

  for (i = 0; i < count; i++)
  {
      CLSID* lpclsid = (CLSID*) SendMessageA(pdlgInfo->hwndObjTypeLB, 
         LB_GETITEMDATA, i, 0);
      HeapFree(GetProcessHeap(), 0, lpclsid);
  }
}


/***********************************************************************
 *      UIINSERTOBJECTDLG_SelChange
 *
 * Handle object type selection change
 */
static void UIINSERTOBJECTDLG_SelChange(InsertObjectDlgInfo* pdlgInfo)
{
  INT index;
  WCHAR objname[MAX_PATH];
  WCHAR objdesc[MAX_PATH];
  WCHAR resstr[MAX_PATH];

  TRACE("\n");

  if (LoadStringW(OLEDLG_hInstance, IDS_RESULTOBJDESC, resstr, MAX_PATH) &&
     ((index = SendMessageW(pdlgInfo->hwndObjTypeLB, LB_GETCURSEL, 0, 0)) >= 0) &&
     SendMessageW(pdlgInfo->hwndObjTypeLB, LB_GETTEXT, index, (LPARAM)objname))
       wsprintfW(objdesc, resstr, objname);
  else
    objdesc[0] = 0;

  SendMessageW(pdlgInfo->hwndResultDesc, WM_SETTEXT, 0, (LPARAM)objdesc);
}


/***********************************************************************
 *      UIINSERTOBJECTDLG_SelChange
 *
 * Handle OK Button
 */
static BOOL UIINSERTOBJECTDLG_OnOpen(InsertObjectDlgInfo* pdlgInfo)
{
  BOOL bret = FALSE;

  if (BST_CHECKED == SendMessageA(pdlgInfo->hwndCreateCtrlCB, BM_GETCHECK, 0, 0) ||
      BST_CHECKED == SendMessageA(pdlgInfo->hwndCreateNewCB, BM_GETCHECK, 0, 0))
  {
    INT index = SendMessageA(pdlgInfo->hwndObjTypeLB, LB_GETCURSEL, 0, 0);

    if (index >= 0)
    {
       CLSID* clsid = (CLSID*) SendMessageA(pdlgInfo->hwndObjTypeLB, 
          LB_GETITEMDATA, index, 0);
       pdlgInfo->lpOleUIInsertObject->clsid = *clsid;

       if (pdlgInfo->lpOleUIInsertObject->dwFlags & IOF_CREATENEWOBJECT)
       {
          pdlgInfo->lpOleUIInsertObject->sc= OleCreate(
             &pdlgInfo->lpOleUIInsertObject->clsid,
             &pdlgInfo->lpOleUIInsertObject->iid,
             pdlgInfo->lpOleUIInsertObject->oleRender,
             pdlgInfo->lpOleUIInsertObject->lpFormatEtc,
             pdlgInfo->lpOleUIInsertObject->lpIOleClientSite,
             pdlgInfo->lpOleUIInsertObject->lpIStorage,
             pdlgInfo->lpOleUIInsertObject->ppvObj);
       }

       bret = TRUE;
    }
  }
  else if (BST_CHECKED == SendMessageA(pdlgInfo->hwndCreateFromFileCB, BM_GETCHECK, 0, 0))
  {
    if (pdlgInfo->lpOleUIInsertObject->lpszFile)
    {
      HRESULT hres;
      WCHAR wcsFile[MAX_PATH];

      if (SendMessageW(pdlgInfo->hwndFileTB, WM_GETTEXT, MAX_PATH, (LPARAM)wcsFile))
          WideCharToMultiByte(CP_ACP, 0, wcsFile, -1,
              pdlgInfo->lpOleUIInsertObject->lpszFile, pdlgInfo->lpOleUIInsertObject->cchFile, NULL, NULL);

      if (SUCCEEDED(hres = GetClassFile(wcsFile, &pdlgInfo->lpOleUIInsertObject->clsid)))
      {
         if (pdlgInfo->lpOleUIInsertObject->dwFlags & IOF_CREATEFILEOBJECT)
         {
           hres = OleCreateFromFile(
             &pdlgInfo->lpOleUIInsertObject->clsid,
             wcsFile,
             &pdlgInfo->lpOleUIInsertObject->iid,
             pdlgInfo->lpOleUIInsertObject->oleRender,
             pdlgInfo->lpOleUIInsertObject->lpFormatEtc,
             pdlgInfo->lpOleUIInsertObject->lpIOleClientSite,
             pdlgInfo->lpOleUIInsertObject->lpIStorage,
             pdlgInfo->lpOleUIInsertObject->ppvObj);
         }

         bret = TRUE;
      }
      pdlgInfo->lpOleUIInsertObject->sc = hres;
    }
  }

  return bret;
}


/***********************************************************************
 *      UIINSERTOBJECTDLG_BrowseFile
 *
 * Browse for the file
 */
static void UIINSERTOBJECTDLG_BrowseFile(InsertObjectDlgInfo* pdlgInfo)
{
   OPENFILENAMEA fn;
   char fname[MAX_PATH];
   char title[32];

   fn.lStructSize = sizeof(OPENFILENAMEA);
   fn.hwndOwner = pdlgInfo->hwndSelf;
   fn.hInstance = 0;
   fn.lpstrFilter = "All Files\0*.*\0\0";
   fn.lpstrCustomFilter = NULL;
   fn.nMaxCustFilter = 0;
   fn.nFilterIndex = 0;

   SendMessageA(pdlgInfo->hwndFileTB, WM_GETTEXT, MAX_PATH, (LPARAM)fname);
   fn.lpstrFile = fname;
   fn.nMaxFile = MAX_PATH;

   fn.lpstrFileTitle = NULL;
   fn.nMaxFileTitle = 0;
   fn.lpstrInitialDir = NULL;

   LoadStringA(OLEDLG_hInstance, IDS_BROWSE, title, 32);
   fn.lpstrTitle = title;

   fn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST |
      OFN_HIDEREADONLY | OFN_LONGNAMES;
   fn.nFileOffset = 0;
   fn.nFileExtension = 0;
   fn.lpstrDefExt = NULL;
   fn.lCustData = 0;
   fn.lpfnHook = NULL;
   fn.lpTemplateName = NULL;

   if (GetOpenFileNameA(&fn))
      SendMessageA(pdlgInfo->hwndFileTB, WM_SETTEXT, 0, (LPARAM)fn.lpstrFile);
}


/***********************************************************************
 *      UIINSERTOBJECTDLG_AddControl
 *
 * Add control to Object Type
 */
static void UIINSERTOBJECTDLG_AddControl(InsertObjectDlgInfo* pdlgInfo)
{
   OPENFILENAMEA fn;
   char fname[MAX_PATH];
   char title[32];

   fn.lStructSize = sizeof(OPENFILENAMEA);
   fn.hwndOwner = pdlgInfo->hwndSelf;
   fn.hInstance = 0;
   fn.lpstrFilter = "OLE Controls\0*.ocx\0Libraries\0*.dll\0All Files\0*.*\0\0";
   fn.lpstrCustomFilter = NULL;
   fn.nMaxCustFilter = 0;
   fn.nFilterIndex = 0;

   fname[0] = 0;
   fn.lpstrFile = fname;
   fn.nMaxFile = MAX_PATH;

   fn.lpstrFileTitle = NULL;
   fn.nMaxFileTitle = 0;
   fn.lpstrInitialDir = NULL;

   LoadStringA(OLEDLG_hInstance, IDS_BROWSE, title, 32);
   fn.lpstrTitle = title;

   fn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST |
      OFN_HIDEREADONLY | OFN_LONGNAMES;
   fn.nFileOffset = 0;
   fn.nFileExtension = 0;
   fn.lpstrDefExt = NULL;
   fn.lCustData = 0;
   fn.lpfnHook = NULL;
   fn.lpTemplateName = NULL;

   if (GetOpenFileNameA(&fn))
   {
      HMODULE hMod;
      BOOL bValid = FALSE;

      hMod = LoadLibraryA(fn.lpstrFile);

      if (hMod)
      {
          DLLREGISTER regproc;

          regproc = (DLLREGISTER) GetProcAddress(hMod, "DllRegisterServer");
          if (regproc)
          {
             if (S_OK == regproc())
             {
                UIINSERTOBJECTDLG_PopulateObjectTypes(pdlgInfo);
                bValid = TRUE;
             }
          }

          FreeLibrary(hMod);
      }

      if (!bValid)
      {
          WCHAR title[32];
          WCHAR msg[256];

          LoadStringW(OLEDLG_hInstance, IDS_NOTOLEMODCAPTION, title, 32);
          LoadStringW(OLEDLG_hInstance, IDS_NOTOLEMOD, msg, 256);

          MessageBoxW(pdlgInfo->hwndSelf, msg, title, MB_ICONEXCLAMATION);
      }
   }
}
