/*
 * ReactOS Access Control List Editor
 * Copyright (C) 2004 ReactOS Team
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
/* $Id: aclui.c,v 1.5 2004/08/14 00:46:54 weiden Exp $
 *
 * PROJECT:         ReactOS Access Control List Editor
 * FILE:            lib/aclui/aclui.c
 * PURPOSE:         Access Control List Editor
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *
 * UPDATE HISTORY:
 *      08/10/2004  Created
 */
#define INITGUID
#include <windows.h>
#include <prsht.h>
#include <aclui.h>
#include <rosrtl/resstr.h>
#include "internal.h"
#include "resource.h"

HINSTANCE hDllInstance;



BOOL STDCALL
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpvReserved)
{
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
      hDllInstance = hinstDLL;
      break;
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}


UINT CALLBACK
SecurityPageCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
  switch(uMsg)
  {
    case PSPCB_CREATE:
      return TRUE;
    case PSPCB_RELEASE:
      return FALSE;
  }

  return FALSE;
}


INT_PTR CALLBACK
SecurityPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  return 0;
}


/*
 * CreateSecurityPage							EXPORTED
 *
 * @implemented
 */
HPROPSHEETPAGE
WINAPI
CreateSecurityPage(LPSECURITYINFO psi)
{
  PROPSHEETPAGE psp;
  SI_OBJECT_INFO ObjectInfo;
  HRESULT hRet;

  if(psi == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);

    DPRINT("No ISecurityInformation class passed!\n");
    return NULL;
  }

  /* get the object information from the client interface */
  hRet = psi->lpVtbl->GetObjectInformation(psi, &ObjectInfo);

  if(FAILED(hRet))
  {
    SetLastError(hRet);

    DPRINT("CreateSecurityPage() failed!\n");
    return NULL;
  }

  psp.dwSize = sizeof(PROPSHEETPAGE);
  psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK;
  psp.hInstance = hDllInstance;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_SECPAGE);
  psp.pfnDlgProc = SecurityPageProc;
  psp.lParam = (LPARAM)psi;
  psp.pfnCallback = SecurityPageCallback;

  if((ObjectInfo.dwFlags & SI_PAGE_TITLE) != 0 &&
     ObjectInfo.pszPageTitle != NULL && ObjectInfo.pszPageTitle[0] != L'\0')
  {
    /* Set the page title if the flag is present and the string isn't empty */
    psp.pszTitle = ObjectInfo.pszPageTitle;
    psp.dwFlags |= PSP_USETITLE;
  }

  return CreatePropertySheetPage(&psp);
}


/*
 * EditSecurity								EXPORTED
 *
 * @implemented
 */
BOOL
WINAPI
EditSecurity(HWND hwndOwner, LPSECURITYINFO psi)
{
  HRESULT hRet;
  SI_OBJECT_INFO ObjectInfo;
  PROPSHEETHEADER psh;
  HPROPSHEETPAGE hPages[1];
  LPWSTR lpCaption;
  BOOL Ret;
  
  if(psi == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    
    DPRINT("No ISecurityInformation class passed!\n");
    return FALSE;
  }
  
  /* get the object information from the client interface */
  hRet = psi->lpVtbl->GetObjectInformation(psi, &ObjectInfo);
  
  if(FAILED(hRet))
  {
    SetLastError(hRet);
    
    DPRINT("GetObjectInformation() failed!\n");
    return FALSE;
  }

  /* create the page */
  hPages[0] = CreateSecurityPage(psi);
  if(hPages[0] == NULL)
  {
    DPRINT("CreateSecurityPage(), couldn't create property sheet!\n");
    return FALSE;
  }
  
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags = PSH_DEFAULT;
  psh.hwndParent = hwndOwner;
  psh.hInstance = hDllInstance;
  if((ObjectInfo.dwFlags & SI_PAGE_TITLE) != 0 &&
     ObjectInfo.pszPageTitle != NULL && ObjectInfo.pszPageTitle[0] != L'\0')
  {
    /* Set the page title if the flag is present and the string isn't empty */
    psh.pszCaption = ObjectInfo.pszPageTitle;
    lpCaption = NULL;
  }
  else
  {
    /* Set the page title to the object name, make sure the format string
       has "%1" NOT "%s" because it uses FormatMessage() to automatically
       allocate the right amount of memory. */
    RosLoadAndFormatStr(hDllInstance, IDS_PSP_TITLE, &lpCaption, ObjectInfo.pszObjectName);
    psh.pszCaption = lpCaption;
  }
  psh.nPages = sizeof(hPages) / sizeof(HPROPSHEETPAGE);
  psh.nStartPage = 0;
  psh.phpage = hPages;
  
  Ret = (PropertySheet(&psh) != -1);
  
  if(lpCaption != NULL)
  {
    LocalFree((HLOCAL)lpCaption);
  }
  
  return Ret;
}

