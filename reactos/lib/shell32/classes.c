/*
 *	file type mapping
 *	(HKEY_CLASSES_ROOT - Stuff)
 *
 * Copyright 1998, 1999, 2000 Juergen Schmied
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "wine/debug.h"
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

#include "shlobj.h"
#include "shell32_main.h"
#include "shlguid.h"
#include "shresdef.h"
#include "shlwapi.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define MAX_EXTENSION_LENGTH 20

BOOL HCR_MapTypeToValueW(LPCWSTR szExtension, LPWSTR szFileType, DWORD len, BOOL bPrependDot)
{	
	HKEY	hkey;
	WCHAR	szTemp[MAX_EXTENSION_LENGTH + 2];

	TRACE("%s %p\n", debugstr_w(szExtension), debugstr_w(szFileType));

    /* added because we do not want to have double dots */
    if (szExtension[0] == '.')
        bPrependDot = 0;

	if (bPrependDot)
	  szTemp[0] = '.';

	lstrcpynW(szTemp + (bPrependDot?1:0), szExtension, MAX_EXTENSION_LENGTH);

	if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szTemp, 0, 0x02000000, &hkey))
	{ 
	  return FALSE;
	}

	if (RegQueryValueW(hkey, NULL, szFileType, &len))
	{ 
	  RegCloseKey(hkey);
	  return FALSE;
	}

	RegCloseKey(hkey);

	TRACE("--UE;\n} %s\n", debugstr_w(szFileType));

	return TRUE;
}

BOOL HCR_MapTypeToValueA(LPCSTR szExtension, LPSTR szFileType, DWORD len, BOOL bPrependDot)
{
	HKEY	hkey;
	char	szTemp[MAX_EXTENSION_LENGTH + 2];

	TRACE("%s %p\n", szExtension, szFileType);

    /* added because we do not want to have double dots */
    if (szExtension[0] == '.')
        bPrependDot = 0;

	if (bPrependDot)
	  szTemp[0] = '.';

	lstrcpynA(szTemp + (bPrependDot?1:0), szExtension, MAX_EXTENSION_LENGTH);

	if (RegOpenKeyExA(HKEY_CLASSES_ROOT, szTemp, 0, 0x02000000, &hkey))
	{ 
	  return FALSE;
	}

	if (RegQueryValueA(hkey, NULL, szFileType, &len))
	{ 
	  RegCloseKey(hkey);
	  return FALSE;
	}

	RegCloseKey(hkey);

	TRACE("--UE;\n} %s\n", szFileType);

	return TRUE;
}


BOOL HCR_GetExecuteCommandW( HKEY hkeyClass, LPCWSTR szClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len )
{
        static const WCHAR swShell[] = {'\\','s','h','e','l','l','\\',0};
        static const WCHAR swCommand[] = {'\\','c','o','m','m','a','n','d',0};
	BOOL	ret = FALSE;

	TRACE("%p %s %s %p\n", hkeyClass, debugstr_w(szClass), debugstr_w(szVerb), szDest);

	if (szClass)
            RegOpenKeyExW(HKEY_CLASSES_ROOT, szClass, 0, 0x02000000, &hkeyClass);

        if (hkeyClass)
	{
	    WCHAR sTemp[MAX_PATH];
	    lstrcpyW(sTemp, swShell);
	    lstrcatW(sTemp, szVerb);
	    lstrcatW(sTemp, swCommand);

	    ret = (ERROR_SUCCESS == SHGetValueW(hkeyClass, sTemp, NULL, NULL, szDest, &len));

	    if (szClass)
	       RegCloseKey(hkeyClass);
	}

	TRACE("-- %s\n", debugstr_w(szDest) );
	return ret;
}

/***************************************************************************************
*	HCR_GetDefaultIcon	[internal]
*
* Gets the icon for a filetype
*/
static BOOL HCR_RegOpenClassIDKey(REFIID riid, HKEY *hkey)
{
	char	xriid[50];
    sprintf( xriid, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 riid->Data1, riid->Data2, riid->Data3,
                 riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
                 riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7] );

 	TRACE("%s\n",xriid );

	return !RegOpenKeyExA(HKEY_CLASSES_ROOT, xriid, 0, KEY_READ, hkey);
}

static BOOL HCR_RegGetDefaultIconW(HKEY hkey, LPWSTR szDest, DWORD len, LPDWORD dwNr)
{
	DWORD dwType;
	WCHAR sTemp[MAX_PATH];
	WCHAR sNum[5];

	if (!RegQueryValueExW(hkey, NULL, 0, &dwType, (LPBYTE)szDest, &len))
	{
      if (dwType == REG_EXPAND_SZ)
	  {
	    ExpandEnvironmentStringsW(szDest, sTemp, MAX_PATH);
	    lstrcpynW(szDest, sTemp, len);
	  }
	  if (ParseFieldW (szDest, 2, sNum, 5))
             *dwNr = atoiW(sNum);
          else
             *dwNr=0; /* sometimes the icon number is missing */
	  ParseFieldW (szDest, 1, szDest, len);
	  return TRUE;
	}
	return FALSE;
}

static BOOL HCR_RegGetDefaultIconA(HKEY hkey, LPSTR szDest, DWORD len, LPDWORD dwNr)
{
	DWORD dwType;
	char sTemp[MAX_PATH];
	char  sNum[5];

	if (!RegQueryValueExA(hkey, NULL, 0, &dwType, szDest, &len))
	{
      if (dwType == REG_EXPAND_SZ)
	  {
	    ExpandEnvironmentStringsA(szDest, sTemp, MAX_PATH);
	    lstrcpynA(szDest, sTemp, len);
	  }
	  if (ParseFieldA (szDest, 2, sNum, 5))
             *dwNr=atoi(sNum);
          else
             *dwNr=0; /* sometimes the icon number is missing */
	  ParseFieldA (szDest, 1, szDest, len);
	  return TRUE;
	}
	return FALSE;
}

BOOL HCR_GetDefaultIconW(LPCWSTR szClass, LPWSTR szDest, DWORD len, LPDWORD dwNr)
{
        static const WCHAR swDefaultIcon[] = {'\\','D','e','f','a','u','l','t','I','c','o','n',0};
	HKEY	hkey;
	WCHAR	sTemp[MAX_PATH];
	BOOL	ret = FALSE;

	TRACE("%s\n",debugstr_w(szClass) );

	lstrcpynW(sTemp, szClass, MAX_PATH);
	lstrcatW(sTemp, swDefaultIcon);

	if (!RegOpenKeyExW(HKEY_CLASSES_ROOT, sTemp, 0, 0x02000000, &hkey))
	{
	  ret = HCR_RegGetDefaultIconW(hkey, szDest, len, dwNr);
	  RegCloseKey(hkey);
	}
	TRACE("-- %s %li\n", debugstr_w(szDest), *dwNr );
	return ret;
}

BOOL HCR_GetDefaultIconA(LPCSTR szClass, LPSTR szDest, DWORD len, LPDWORD dwNr)
{
	HKEY	hkey;
	char	sTemp[MAX_PATH];
	BOOL	ret = FALSE;

	TRACE("%s\n",szClass );

	sprintf(sTemp, "%s\\DefaultIcon",szClass);

	if (!RegOpenKeyExA(HKEY_CLASSES_ROOT, sTemp, 0, 0x02000000, &hkey))
	{
	  ret = HCR_RegGetDefaultIconA(hkey, szDest, len, dwNr);
	  RegCloseKey(hkey);
	}
	TRACE("-- %s %li\n", szDest, *dwNr );
	return ret;
}

BOOL HCR_GetDefaultIconFromGUIDW(REFIID riid, LPWSTR szDest, DWORD len, LPDWORD dwNr)
{
	HKEY	hkey;
	BOOL	ret = FALSE;

	if (HCR_RegOpenClassIDKey(riid, &hkey))
	{
	  ret = HCR_RegGetDefaultIconW(hkey, szDest, len, dwNr);
	  RegCloseKey(hkey);
	}
	TRACE("-- %s %li\n", debugstr_w(szDest), *dwNr );
	return ret;
}

/***************************************************************************************
*	HCR_GetClassName	[internal]
*
* Gets the name of a registred class
*/
static WCHAR swEmpty[] = {0};

BOOL HCR_GetClassNameW(REFIID riid, LPWSTR szDest, DWORD len)
{	
	HKEY	hkey;
	BOOL ret = FALSE;
	DWORD buflen = len;

 	szDest[0] = 0;
	if (HCR_RegOpenClassIDKey(riid, &hkey))
	{
	  if (!RegQueryValueExW(hkey, swEmpty, 0, NULL, (LPBYTE)szDest, &len))
	  {
	    ret = TRUE;
	  }
	  RegCloseKey(hkey);
	}

	if (!ret || !szDest[0])
	{
	  if(IsEqualIID(riid, &CLSID_ShellDesktop))
	  {
	    if (LoadStringW(shell32_hInstance, IDS_DESKTOP, szDest, buflen))
	      ret = TRUE;
	  }
	  else if (IsEqualIID(riid, &CLSID_MyComputer))
	  {
	    if(LoadStringW(shell32_hInstance, IDS_MYCOMPUTER, szDest, buflen))
	      ret = TRUE;
	  }
	}
	TRACE("-- %s\n", debugstr_w(szDest));
	return ret;
}

BOOL HCR_GetClassNameA(REFIID riid, LPSTR szDest, DWORD len)
{	HKEY	hkey;
	BOOL ret = FALSE;
	DWORD buflen = len;

	szDest[0] = 0;
	if (HCR_RegOpenClassIDKey(riid, &hkey))
	{
	  if (!RegQueryValueExA(hkey,"",0,NULL,szDest,&len))
	  {
	    ret = TRUE;
	  }
	  RegCloseKey(hkey);
	}

	if (!ret || !szDest[0])
	{
	  if(IsEqualIID(riid, &CLSID_ShellDesktop))
	  {
	    if (LoadStringA(shell32_hInstance, IDS_DESKTOP, szDest, buflen))
	      ret = TRUE;
	  }
	  else if (IsEqualIID(riid, &CLSID_MyComputer))
	  {
	    if(LoadStringA(shell32_hInstance, IDS_MYCOMPUTER, szDest, buflen))
	      ret = TRUE;
	  }
	}

	TRACE("-- %s\n", szDest);

	return ret;
}

/***************************************************************************************
*	HCR_GetFolderAttributes	[internal]
*
* gets the folder attributes of a class
*
* FIXME
*	verify the defaultvalue for *szDest
*/
BOOL HCR_GetFolderAttributes (REFIID riid, LPDWORD szDest)
{	HKEY	hkey;
	char	xriid[60];
	DWORD	attributes;
	DWORD	len = 4;

        sprintf( xriid, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 riid->Data1, riid->Data2, riid->Data3,
                 riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
                 riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7] );
	TRACE("%s\n",xriid );

	if (!szDest) return FALSE;
	*szDest = SFGAO_FOLDER|SFGAO_FILESYSTEM;

	strcat (xriid, "\\ShellFolder");

	if (RegOpenKeyExA(HKEY_CLASSES_ROOT,xriid,0,KEY_READ,&hkey))
	{
	  return FALSE;
	}

	if (RegQueryValueExA(hkey,"Attributes",0,NULL,(LPBYTE)&attributes,&len))
	{
	  RegCloseKey(hkey);
	  return FALSE;
	}

	RegCloseKey(hkey);

	TRACE("-- 0x%08lx\n", attributes);

	*szDest = attributes;

	return TRUE;
}
