/*
 *	IEnumIDList
 *
 *	Copyright 1998	Juergen Schmied <juergen.schmied@metronet.de>
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "undocshell.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"
#include <cpl.h>

#include "pidl.h"
#include "shlguid.h"
#include "shell32_main.h"
#include "control.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct tagENUMLIST
{
	struct tagENUMLIST	*pNext;
	LPITEMIDLIST		pidl;

} ENUMLIST, *LPENUMLIST;

typedef struct
{
	ICOM_VFIELD(IEnumIDList);
	DWORD				ref;
	LPENUMLIST			mpFirst;
	LPENUMLIST			mpLast;
	LPENUMLIST			mpCurrent;

} IEnumIDListImpl;

static struct ICOM_VTABLE(IEnumIDList) eidlvt;

/**************************************************************************
 *  AddToEnumList()
 */
static BOOL AddToEnumList(
	IEnumIDList * iface,
	LPITEMIDLIST pidl)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	LPENUMLIST  pNew;

	TRACE("(%p)->(pidl=%p)\n",This,pidl);
	pNew = (LPENUMLIST)SHAlloc(sizeof(ENUMLIST));
	if(pNew)
	{
	  /*set the next pointer */
	  pNew->pNext = NULL;
	  pNew->pidl = pidl;

	  /*is This the first item in the list? */
	  if(!This->mpFirst)
	  {
	    This->mpFirst = pNew;
	    This->mpCurrent = pNew;
	  }

	  if(This->mpLast)
	  {
	    /*add the new item to the end of the list */
	    This->mpLast->pNext = pNew;
	  }

	  /*update the last item pointer */
	  This->mpLast = pNew;
	  TRACE("-- (%p)->(first=%p, last=%p)\n",This,This->mpFirst,This->mpLast);
	  return TRUE;
	}
	return FALSE;
}

/**************************************************************************
 *  CreateFolderEnumList()
 */
static BOOL CreateFolderEnumList(
	IEnumIDList * iface,
	LPCSTR lpszPath,
	DWORD dwFlags)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	LPITEMIDLIST	pidl=NULL;
	WIN32_FIND_DATAA stffile;
	HANDLE hFile;
	CHAR  szPath[MAX_PATH];

	TRACE("(%p)->(path=%s flags=0x%08lx) \n",This,debugstr_a(lpszPath),dwFlags);

	if(!lpszPath || !lpszPath[0]) return FALSE;

	strcpy(szPath, lpszPath);
	PathAddBackslashA(szPath);
	strcat(szPath,"*.*");

	/*enumerate the folders*/
	if(dwFlags & SHCONTF_FOLDERS)
	{
	  TRACE("-- (%p)-> enumerate SHCONTF_FOLDERS of %s\n",This,debugstr_a(szPath));
	  hFile = FindFirstFileA(szPath,&stffile);
	  if( hFile != INVALID_HANDLE_VALUE )
	  {
	    do
	    {
	      if( !(dwFlags & SHCONTF_INCLUDEHIDDEN) && (stffile.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ) continue;
	      if( (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strcmp (stffile.cFileName, ".") && strcmp (stffile.cFileName, ".."))
	      {
		pidl = _ILCreateFolder (&stffile);
		if(pidl && AddToEnumList((IEnumIDList*)This, pidl))
		{
		  continue;
		}
		return FALSE;
	      }
	    } while( FindNextFileA(hFile,&stffile));
	    FindClose (hFile);
	  }
	}

	/*enumerate the non-folder items (values) */
	if(dwFlags & SHCONTF_NONFOLDERS)
	{
	  TRACE("-- (%p)-> enumerate SHCONTF_NONFOLDERS of %s\n",This,debugstr_a(szPath));
	  hFile = FindFirstFileA(szPath,&stffile);
	  if( hFile != INVALID_HANDLE_VALUE )
	  {
	    do
	    {
	      if( !(dwFlags & SHCONTF_INCLUDEHIDDEN) && (stffile.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ) continue;
	      if(! (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
	      {
		pidl = _ILCreateValue(&stffile);
		if(pidl && AddToEnumList((IEnumIDList*)This, pidl))
		{
		  continue;
		}
		return FALSE;
	      }
	    } while( FindNextFileA(hFile,&stffile));
	    FindClose (hFile);
	  }
	}
	return TRUE;
}

BOOL SHELL_RegisterCPanelApp(IEnumIDList* list, LPCSTR path)
{
    LPITEMIDLIST pidl;
    CPlApplet* applet;
    CPanel panel;
    CPLINFO info;
    unsigned i;
    int iconIdx;

    char displayName[MAX_PATH];
    char comment[MAX_PATH];

    WCHAR wpath[MAX_PATH];

    MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

    panel.first = NULL;
    applet = Control_LoadApplet(0, wpath, &panel);

    if (applet) {
	for(i=0; i<applet->count; ++i) {
	    WideCharToMultiByte(CP_ACP, 0, applet->info[i].szName, -1, displayName, MAX_PATH, 0, 0);
	    WideCharToMultiByte(CP_ACP, 0, applet->info[i].szInfo, -1, comment, MAX_PATH, 0, 0);

	    applet->proc(0, CPL_INQUIRE, i, (LPARAM)&info);

	    if (info.idIcon > 0)
		iconIdx = -info.idIcon;	/* negative icon index instead of icon number */
	    else
		iconIdx = 0;

	    pidl = _ILCreateCPanel(path, displayName, comment, iconIdx);

	    if (pidl)
		AddToEnumList(list, pidl);
	}

	Control_UnloadApplet(applet);
    }

    return TRUE;
}

int SHELL_RegisterRegistryCPanelApps(IEnumIDList* list, HKEY hkey_root, LPCSTR szRepPath)
{
  char name[MAX_PATH];
  char value[MAX_PATH];
  HKEY hkey;

  int cnt = 0;

  if (RegOpenKeyA(hkey_root, szRepPath, &hkey) == ERROR_SUCCESS)
  {
    int idx = 0;
    for(;; ++idx)
    {
      DWORD nameLen = MAX_PATH;
      DWORD valueLen = MAX_PATH;

      if (RegEnumValueA(hkey, idx, name, &nameLen, NULL, NULL, (LPBYTE)&value, &valueLen) != ERROR_SUCCESS)
	break;

      if (SHELL_RegisterCPanelApp(list, value))
        ++cnt;
    }

    RegCloseKey(hkey);
  }

  return cnt;
}

int SHELL_RegisterCPanelFolders(IEnumIDList* list, HKEY hkey_root, LPCSTR szRepPath)
{
  char name[MAX_PATH];
  HKEY hkey;

  int cnt = 0;

  if (RegOpenKeyA(hkey_root, szRepPath, &hkey) == ERROR_SUCCESS)
  {
    int idx = 0;
    for(;; ++idx)
    {
      if (RegEnumKeyA(hkey, idx, name, MAX_PATH) != ERROR_SUCCESS)
	break;

      if (*name == '{') {
	LPITEMIDLIST pidl = _ILCreateSpecial(name);

	if (pidl && AddToEnumList(list, pidl))
	    ++cnt;
      }
    }

    RegCloseKey(hkey);
  }

  return cnt;
}

/**************************************************************************
 *  CreateCPanelEnumList()
 */
static BOOL CreateCPanelEnumList(
	IEnumIDList * iface,
	DWORD dwFlags)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	CHAR szPath[MAX_PATH];
	WIN32_FIND_DATAA wfd;
	HANDLE hFile;

	TRACE("(%p)->(flags=0x%08lx) \n",This,dwFlags);

	/* enumerate control panel folders folders */
	if (dwFlags & SHCONTF_FOLDERS)
	  SHELL_RegisterCPanelFolders((IEnumIDList*)This, HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\NameSpace");

	/* enumerate the control panel applets */
	if (dwFlags & SHCONTF_NONFOLDERS)
	{
	  LPSTR p;

	  GetSystemDirectoryA(szPath, MAX_PATH);
	  p = PathAddBackslashA(szPath);
	  strcpy(p, "*.cpl");

	  TRACE("-- (%p)-> enumerate SHCONTF_NONFOLDERS of %s\n",This,debugstr_a(szPath));
	  hFile = FindFirstFileA(szPath, &wfd);

	  if (hFile != INVALID_HANDLE_VALUE)
	  {
	    do
	    {
	      if (!(dwFlags & SHCONTF_INCLUDEHIDDEN) && (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
		continue;

	      if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		strcpy(p, wfd.cFileName);
	      	SHELL_RegisterCPanelApp((IEnumIDList*)This, szPath);
	      }
	    } while(FindNextFileA(hFile, &wfd));

	    FindClose(hFile);
	  }

	  SHELL_RegisterRegistryCPanelApps((IEnumIDList*)This, HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls");
	  SHELL_RegisterRegistryCPanelApps((IEnumIDList*)This, HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls");
	}

	return TRUE;
}

/**************************************************************************
 *  CreateDesktopEnumList()
 */
static BOOL CreateDesktopEnumList(
	IEnumIDList * iface,
	DWORD dwFlags)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	LPITEMIDLIST	pidl=NULL;
	HKEY	hkey;
	char	szPath[MAX_PATH];

	TRACE("(%p)->(flags=0x%08lx) \n",This,dwFlags);

	/*enumerate the root folders */
	if(dwFlags & SHCONTF_FOLDERS)
	{
	  /*create the pidl for This item */
	  pidl = _ILCreateMyComputer();
	  if(pidl)
	  {
	    if(!AddToEnumList((IEnumIDList*)This, pidl))
	      return FALSE;
	  }

	  if(! RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\explorer\\desktop\\NameSpace", 0, KEY_READ, &hkey))
	  {
	    char iid[50];
	    int i=0;

	    while (1)
	    {
	      DWORD size = sizeof (iid);

	      if(ERROR_SUCCESS!=RegEnumKeyExA(hkey, i, iid, &size, 0, NULL, NULL, NULL))
	        break;

	      pidl = _ILCreateSpecial(iid);

	      if(pidl)
	        AddToEnumList((IEnumIDList*)This, pidl);

	      i++;
	    }
	    RegCloseKey(hkey);
	  }
	}

	/*enumerate the elements in %windir%\desktop */
	SHGetSpecialFolderPathA(0, szPath, CSIDL_DESKTOPDIRECTORY, FALSE);

    /*FIXME: hide icons for classes in SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\explorer\\HideDesktopIcons\\ClassicStartMenu
	-> alter attributes in IDLDATA.folder.uFileAttribs ?
    */
	CreateFolderEnumList( (IEnumIDList*)This, szPath, dwFlags);

	return TRUE;
}

/**************************************************************************
 *  CreateMyCompEnumList()
 */
static BOOL CreateMyCompEnumList(
	IEnumIDList * iface,
	DWORD dwFlags)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	LPITEMIDLIST	pidl=NULL;
	DWORD		dwDrivemap;
	CHAR		szDriveName[4];
	HKEY		hkey;

	TRACE("(%p)->(flags=0x%08lx) \n",This,dwFlags);

	/*enumerate the folders*/
	if(dwFlags & SHCONTF_FOLDERS)
	{
	  dwDrivemap = GetLogicalDrives();
	  strcpy (szDriveName,"A:\\");
	  while (szDriveName[0]<='Z')
	  {
	    if(dwDrivemap & 0x00000001L)
	    {
	      pidl = _ILCreateDrive(szDriveName);
	      if(pidl)
	      {
		if(!AddToEnumList((IEnumIDList*)This, pidl))
	          return FALSE;
	      }
	    }
	    szDriveName[0]++;
	    dwDrivemap = dwDrivemap >> 1;
	  }

	  TRACE("-- (%p)-> enumerate (mycomputer shell extensions)\n",This);
	  if(! RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\explorer\\mycomputer\\NameSpace", 0, KEY_READ, &hkey))
	  {
	    char iid[50];
	    int i=0;

	    while (1)
	    {
	      DWORD size = sizeof (iid);

	      if(ERROR_SUCCESS!=RegEnumKeyExA(hkey, i, iid, &size, 0, NULL, NULL, NULL))
	        break;

	      pidl = _ILCreateSpecial(iid);

	      if(pidl)
	        AddToEnumList((IEnumIDList*)This, pidl);

	      i++;
	    }
	    RegCloseKey(hkey);
	  }
	}
	return TRUE;
}

/**************************************************************************
*   DeleteList()
*/
static BOOL DeleteList(
	IEnumIDList * iface)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	LPENUMLIST  pDelete;

	TRACE("(%p)->()\n",This);

	while(This->mpFirst)
	{ pDelete = This->mpFirst;
	  This->mpFirst = pDelete->pNext;
	  SHFree(pDelete->pidl);
	  SHFree(pDelete);
	}
	This->mpFirst = This->mpLast = This->mpCurrent = NULL;
	return TRUE;
}

/**************************************************************************
 *  IEnumIDList_Folder_Constructor
 *
 */

IEnumIDList * IEnumIDList_Constructor(
	LPCSTR lpszPath,
	DWORD dwFlags,
	DWORD dwKind)
{
	IEnumIDListImpl*	lpeidl;
	BOOL			ret = FALSE;

	TRACE("()->(%s flags=0x%08lx kind=0x%08lx)\n",debugstr_a(lpszPath),dwFlags, dwKind);

	lpeidl = (IEnumIDListImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IEnumIDListImpl));

	if(lpeidl)
	{
	  lpeidl->ref = 1;
	  lpeidl->lpVtbl = &eidlvt;

	  switch (dwKind)
	  {
	    case EIDL_DESK:
	      ret = CreateDesktopEnumList((IEnumIDList*)lpeidl, dwFlags);
	      break;

	    case EIDL_MYCOMP:
	      ret = CreateMyCompEnumList((IEnumIDList*)lpeidl, dwFlags);
	      break;

	    case EIDL_FILE:
	      ret = CreateFolderEnumList((IEnumIDList*)lpeidl, lpszPath, dwFlags);
	      break;

	    case EIDL_CPANEL:
	      ret = CreateCPanelEnumList((IEnumIDList*)lpeidl, dwFlags);
	      break;
	  }

	    if(!ret) {
	        HeapFree(GetProcessHeap(),0,lpeidl);
	        lpeidl = NULL;
	    }
	}

	TRACE("-- (%p)->()\n",lpeidl);

	return (IEnumIDList*)lpeidl;
}

/**************************************************************************
 *  EnumIDList_QueryInterface
 */
static HRESULT WINAPI IEnumIDList_fnQueryInterface(
	IEnumIDList * iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This;
	}
	else if(IsEqualIID(riid, &IID_IEnumIDList))  /*IEnumIDList*/
	{    *ppvObj = (IEnumIDList*)This;
	}

	if(*ppvObj)
	{ IEnumIDList_AddRef((IEnumIDList*)*ppvObj);
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}

	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/******************************************************************************
 * IEnumIDList_fnAddRef
 */
static ULONG WINAPI IEnumIDList_fnAddRef(
	IEnumIDList * iface)
{
	ICOM_THIS(IEnumIDListImpl,iface);
	TRACE("(%p)->(%lu)\n",This,This->ref);
	return ++(This->ref);
}
/******************************************************************************
 * IEnumIDList_fnRelease
 */
static ULONG WINAPI IEnumIDList_fnRelease(
	IEnumIDList * iface)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	TRACE("(%p)->(%lu)\n",This,This->ref);

	if(!--(This->ref)) {
	  TRACE(" destroying IEnumIDList(%p)\n",This);
	  DeleteList((IEnumIDList*)This);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

/**************************************************************************
 *  IEnumIDList_fnNext
 */

static HRESULT WINAPI IEnumIDList_fnNext(
	IEnumIDList * iface,
	ULONG celt,
	LPITEMIDLIST * rgelt,
	ULONG *pceltFetched)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	ULONG    i;
	HRESULT  hr = S_OK;
	LPITEMIDLIST  temp;

	TRACE("(%p)->(%ld,%p, %p)\n",This,celt,rgelt,pceltFetched);

/* It is valid to leave pceltFetched NULL when celt is 1. Some of explorer's
 * subsystems actually use it (and so may a third party browser)
 */
	if(pceltFetched)
	  *pceltFetched = 0;

	*rgelt=0;

	if(celt > 1 && !pceltFetched)
	{ return E_INVALIDARG;
	}

	if(celt > 0 && !This->mpCurrent)
	{ return S_FALSE;
	}

	for(i = 0; i < celt; i++)
	{ if(!(This->mpCurrent))
	    break;

	  temp = ILClone(This->mpCurrent->pidl);
	  rgelt[i] = temp;
	  This->mpCurrent = This->mpCurrent->pNext;
	}
	if(pceltFetched)
	{  *pceltFetched = i;
	}

	return hr;
}

/**************************************************************************
*  IEnumIDList_fnSkip
*/
static HRESULT WINAPI IEnumIDList_fnSkip(
	IEnumIDList * iface,ULONG celt)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	DWORD    dwIndex;
	HRESULT  hr = S_OK;

	TRACE("(%p)->(%lu)\n",This,celt);

	for(dwIndex = 0; dwIndex < celt; dwIndex++)
	{ if(!This->mpCurrent)
	  { hr = S_FALSE;
	    break;
	  }
	  This->mpCurrent = This->mpCurrent->pNext;
	}
	return hr;
}
/**************************************************************************
*  IEnumIDList_fnReset
*/
static HRESULT WINAPI IEnumIDList_fnReset(
	IEnumIDList * iface)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	TRACE("(%p)\n",This);
	This->mpCurrent = This->mpFirst;
	return S_OK;
}
/**************************************************************************
*  IEnumIDList_fnClone
*/
static HRESULT WINAPI IEnumIDList_fnClone(
	IEnumIDList * iface,LPENUMIDLIST * ppenum)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	TRACE("(%p)->() to (%p)->() E_NOTIMPL\n",This,ppenum);
	return E_NOTIMPL;
}

/**************************************************************************
 *  IEnumIDList_fnVTable
 */
static ICOM_VTABLE (IEnumIDList) eidlvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IEnumIDList_fnQueryInterface,
	IEnumIDList_fnAddRef,
	IEnumIDList_fnRelease,
	IEnumIDList_fnNext,
	IEnumIDList_fnSkip,
	IEnumIDList_fnReset,
	IEnumIDList_fnClone,
};
