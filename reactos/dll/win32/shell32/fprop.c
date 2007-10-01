/*
 *                 Shell Library Functions
 *
 * Copyright 2005 Johannes Anderwald
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
#define YDEBUG
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "wine/debug.h"

#include "shellapi.h"
#include <shlwapi.h>
#include "shlobj.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "undocshell.h"
#include <prsht.h>

typedef struct _LANGANDCODEPAGE_ 
  {
    WORD lang;
    WORD code;
} LANGANDCODEPAGE, *LPLANGANDCODEPAGE;

/*************************************************************************
 *
 * SH_FileGeneralFileType [Internal]
 *
 * retrieves file extension description from registry and sets it in dialog
 *
 * TODO: retrieve file extension default icon and load it
 *       find executable name from registry, retrieve description from executable
 */

BOOL
SH_FileGeneralSetFileType(HWND hwndDlg, WCHAR * filext)
{
     WCHAR name[MAX_PATH];
     WCHAR value[MAX_PATH];
     DWORD lname = MAX_PATH;
     DWORD lvalue = MAX_PATH;
 
     HKEY hKey;
     LONG result;
     HWND hDlgCtrl;

     TRACE("fileext %s\n", debugstr_w(filext));

     if (filext == NULL)
		 return FALSE;

     hDlgCtrl = GetDlgItem(hwndDlg, 14005);

     if (hDlgCtrl == NULL)
         return FALSE;

     if (RegOpenKeyW(HKEY_CLASSES_ROOT, filext, &hKey) != ERROR_SUCCESS)
     {
         /* the fileextension is unknown, so default to string "FileExtension File" */
         SendMessageW(hDlgCtrl, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)value);
         sprintfW(name, value, &filext[1]);
         SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)name);
         return TRUE;
     }
     result = RegEnumValueW(hKey, 0, name, &lname, NULL, NULL, (LPBYTE)value, &lvalue);
     RegCloseKey(hKey);

    if (result != ERROR_SUCCESS)
        return FALSE;
    if (RegOpenKeyW(HKEY_CLASSES_ROOT, value, &hKey) == ERROR_SUCCESS)
    {
        lvalue = lname = MAX_PATH;
        result = RegEnumValueW(hKey,0, name, &lname, NULL, NULL, (LPBYTE)value, &lvalue);
        RegCloseKey(hKey);
    }

    /* file extension type */
    SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)value);
    return TRUE;
}
/*************************************************************************
 *
 * SHFileGeneralGetFileTimeString [Internal]
 *
 * formats a given LPFILETIME struct into readable user format 
 */

BOOL 
SHFileGeneralGetFileTimeString(LPFILETIME lpFileTime, WCHAR * lpResult)
{
    FILETIME ft;
    SYSTEMTIME dt;
    WORD wYear;
    static const WCHAR wFormat[] = {'%','0','2','d','/','%','0','2','d','/','%','0','4','d',' ',' ','%','0','2','d',':','%','0','2','u',0};
  
    if (lpFileTime == NULL || lpResult == NULL)
        return FALSE;

    if (!FileTimeToLocalFileTime(lpFileTime, &ft))
        return FALSE;

    FileTimeToSystemTime(&ft, &dt);

    wYear = dt.wYear;
    /* ddmmyy */
    sprintfW (lpResult, wFormat, dt.wDay, dt.wMonth, wYear, dt.wHour, dt.wMinute);
    
    TRACE("result %s\n",debugstr_w(lpResult));
    return TRUE;
}

/*************************************************************************
 *
 * SH_FileGeneralSetText [Internal]
 *
 * sets file path string and filename string
 *
 */

BOOL 
SH_FileGeneralSetText(HWND hwndDlg, WCHAR * lpstr)
{
    int flength;
    int plength;
    WCHAR * lpdir;
    WCHAR buff[MAX_PATH];
    HWND hDlgCtrl;
  
    if (lpstr == NULL)
        return FALSE;

    lpdir = strrchrW(lpstr, '\\'); /* find the last occurence of '\\' */

    plength = strlenW(lpstr);
    flength = strlenW(lpdir);
  
    if (lpdir)
    {
        /* location text field */
        strncpyW(buff, lpstr, plength - flength);
        buff[plength - flength] = UNICODE_NULL;
        hDlgCtrl = GetDlgItem(hwndDlg, 14009);
        SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)buff);
    }

    if(flength > 1)
    {
         /* text filename field */
         strncpyW(buff, &lpdir[1], flength);
         hDlgCtrl = GetDlgItem(hwndDlg, 14001);
         SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)buff);
    }

    return TRUE;
}

/*************************************************************************
 *
 * SH_FileGeneralSetFileSizeTime [Internal]
 *
 * retrieves file information from file and sets in dialog
 *
 */

BOOL
SH_FileGeneralSetFileSizeTime(HWND hwndDlg, WCHAR * lpfilename, PULARGE_INTEGER lpfilesize)
{
    BOOL result;
    HANDLE hFile; 
    FILETIME create_time;
    FILETIME accessed_time;
    FILETIME write_time;
    WCHAR resultstr[MAX_PATH];
    HWND hDlgCtrl;
    LARGE_INTEGER file_size;
    WCHAR szFormat[] = { '%','u',' ','B','y','t','e','s',0 };

    if (lpfilename == NULL)
        return FALSE;
  
    hFile = CreateFileW(lpfilename,
                        GENERIC_READ,
                        FILE_SHARE_READ,NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        WARN("failed to open file %s\n", debugstr_w(lpfilename));
        return FALSE;
    }
    
    result = GetFileTime(hFile, &create_time, &accessed_time, &write_time);
  
    if (!result)
    {
        WARN("GetFileTime failed\n");
        return FALSE;
    }
    if (SHFileGeneralGetFileTimeString(&create_time,resultstr))
    {
        hDlgCtrl = GetDlgItem(hwndDlg, 14015);
        SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)resultstr);
    }

    if (SHFileGeneralGetFileTimeString(&accessed_time, resultstr))
    {
        hDlgCtrl = GetDlgItem(hwndDlg, 14017);
        SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)resultstr);
    }

    if (SHFileGeneralGetFileTimeString(&write_time, resultstr))
    {
        hDlgCtrl = GetDlgItem(hwndDlg, 14019);
        SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)resultstr);
    }
  
    if (!GetFileSizeEx(hFile, &file_size))
    {
        WARN("GetFileSize failed\n");
        CloseHandle(hFile);
        return FALSE;
    }
    CloseHandle(hFile);
#if 0
    if (!StrFormatByteSizeW(file_size.QuadPart, resultstr, sizeof(resultstr)))
       return FALSE;
#else
    sprintfW(resultstr, szFormat, file_size.QuadPart);
#endif

   hDlgCtrl = GetDlgItem(hwndDlg, 14011);
   TRACE("result size %u resultstr %s\n", file_size.QuadPart, debugstr_w(resultstr));
   SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)resultstr);
 
   if (lpfilesize)
       lpfilesize->QuadPart = (ULONGLONG)file_size.QuadPart;

   return TRUE;
}

/*************************************************************************
 *
 * SH_SetFileVersionText [Internal]
 *
 *
 */

BOOL
SH_FileVersionQuerySetText(HWND hwndDlg, DWORD dlgId, LPVOID pInfo, WCHAR * text, WCHAR ** resptr)
{
  UINT reslen;
  HWND hDlgCtrl;

  if(hwndDlg == NULL || resptr == NULL || text == NULL)
   return FALSE;
  
  if(VerQueryValueW(pInfo, text, (LPVOID *)resptr, &reslen))
  {
    /* file description property */
   hDlgCtrl = GetDlgItem(hwndDlg, dlgId);
   TRACE("%s :: %s\n",debugstr_w(text), debugstr_w(*resptr));
   SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)0, (LPARAM)*resptr);
   return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 *
 * SH_FileVersionQuerySetListText [Internal]
 *
 * retrieves a version string and adds it to listbox
 *
 */


BOOL 
SH_FileVersionQuerySetListText(HWND hwndDlg, LPVOID pInfo, const WCHAR * text, WCHAR **resptr, WORD lang, WORD code)
{
  UINT reslen;
  HWND hDlgCtrl;
  UINT index;
  static const WCHAR wFormat[] = { '\\','S','t','r','i','n','g','F','i','l','e','I','n',
	  'f','o','\\','%','0','4','x','%','0','4','x','\\','%','s',0 };
  WCHAR buff[256];

  TRACE("text %s, resptr %p hwndDlg %p\n",debugstr_w(text), resptr, hwndDlg);

  if(hwndDlg == NULL || resptr == NULL || text == NULL)
   return FALSE;

  sprintfW(buff, wFormat, lang, code, text);
  if(VerQueryValueW(pInfo, buff, (LPVOID *)resptr, &reslen))
  {
    /* listbox name property */
   hDlgCtrl = GetDlgItem(hwndDlg, 14009);
   TRACE("%s :: %s\n",debugstr_w(text), debugstr_w(*resptr));
   index = SendMessageW(hDlgCtrl, LB_ADDSTRING, (WPARAM)-1, (LPARAM)text);
   SendMessageW(hDlgCtrl, LB_SETITEMDATA, (WPARAM)index, (LPARAM)(WCHAR*)*resptr);
   return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 *
 * SH_FileVersionInitialize [Internal]
 *
 * sets all file version properties in dialog
 */
BOOL
SH_FileVersionInitialize(HWND hwndDlg, WCHAR * lpfilename)
{
  LPVOID pBuf;
  DWORD versize;
  DWORD handle;
  LPVOID info = NULL;
  UINT infolen;
  WCHAR buff[256];
  HWND hDlgCtrl;
  WORD lang = 0;
  WORD code = 0;
  LPLANGANDCODEPAGE lplangcode;
  WCHAR * str;
  static const WCHAR wVersionFormat[] = { '%','d','.','%','d','.','%','d','.','%','d',0 };
  static const WCHAR wFileDescriptionFormat[] = { '\\','S','t','r','i','n','g','F','i','l','e','I','n','f','o',
       '\\','%','0','4','x','%','0','4','x','\\','F','i','l','e','D','e','s','c','r','i','p','t','i','o','n',0 };
  static const WCHAR wLegalCopyrightFormat[] = { '\\','S','t','r','i','n','g','F','i','l','e','I','n','f','o',
       '\\','%','0','4','x','%','0','4','x','\\','L','e','g','a','l','C','o','p','y','r','i','g','h','t',0 };  
  static const WCHAR wTranslation[] = { 'V','a','r','F','i','l','e','I','n','f','o','\\','T','r','a','n','s','l','a','t','i','o','n',0 };
  static const WCHAR wCompanyName[] = { 'C','o','m','p','a','n','y','N','a','m','e',0 };  
  static const WCHAR wFileVersion[] = { 'F','i','l','e','V','e','r','s','i','o','n',0 };
  static const WCHAR wInternalName[] = { 'I','n','t','e','r','n','a','l','N','a','m','e',0 };
  static const WCHAR wOriginalFilename[] = { 'O','r','i','g','i','n','a','l','F','i','l','e','n','a','m','e',0 };
  static const WCHAR wProductName[] = { 'P','r','o','d','u','c','t','N','a','m','e',0 };
  static const WCHAR wProductVersion[] = { 'P','r','o','d','u','c','t','V','e','r','s','i','o','n',0 };
  static const WCHAR wSlash[] = { '\\',0 };


  if(lpfilename == 0)
    return FALSE;

  if(!(versize = GetFileVersionInfoSizeW(lpfilename, &handle)))
  {
	WARN("GetFileVersionInfoSize failed\n");
    return FALSE;
  }
  
  if(!(pBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, versize)))
  {
	WARN("HeapAlloc failed bytes %x\n",versize);
    return FALSE;
  }
  
  if(!GetFileVersionInfoW(lpfilename, handle, versize, pBuf))
  {
	HeapFree(GetProcessHeap(), 0, pBuf);
    return FALSE;
  }
  if(VerQueryValueW(pBuf, wSlash, &info, &infolen))
  {
    VS_FIXEDFILEINFO * inf = (VS_FIXEDFILEINFO *)info;
	sprintfW(buff, wVersionFormat,inf->dwFileVersionMS & 0xFFFF0000,
		                                  inf->dwFileVersionMS & 0x0000FFFF,
										  inf->dwFileVersionLS & 0xFFFF0000,
										  inf->dwFileVersionLS & 0x0000FFFF);

   hDlgCtrl = GetDlgItem(hwndDlg, 14001);
   TRACE("MS %x LS %x res %s \n",inf->dwFileVersionMS, inf->dwFileVersionLS, debugstr_w(buff));
   SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)buff);
  }
  if(VerQueryValueW(pBuf, wTranslation, (LPVOID *)&lplangcode, &infolen))
  {
    /* FIXME find language from current locale / if not available,
	 * default to english
	 * for now default to first available language
     */
    lang = lplangcode->lang;
    code = lplangcode->code;
  }
  
  sprintfW(buff, wFileDescriptionFormat, lang, code);
  SH_FileVersionQuerySetText(hwndDlg, 14003, pBuf, buff, &str);
    
  sprintfW(buff, wLegalCopyrightFormat, lang, code);
  SH_FileVersionQuerySetText(hwndDlg, 14005, pBuf, buff, &str);

  /* listbox properties */
  SH_FileVersionQuerySetListText(hwndDlg, pBuf, wCompanyName, &str, lang, code);
  SH_FileVersionQuerySetListText(hwndDlg, pBuf, wFileVersion, &str, lang, code);
  SH_FileVersionQuerySetListText(hwndDlg, pBuf, wInternalName, &str, lang, code);

  /* FIXME insert language identifier */

  SH_FileVersionQuerySetListText(hwndDlg, pBuf, wOriginalFilename, &str, lang, code);
  SH_FileVersionQuerySetListText(hwndDlg, pBuf, wProductName, &str, lang, code);
  SH_FileVersionQuerySetListText(hwndDlg, pBuf, wProductVersion, &str, lang, code);
  SetWindowLong(hwndDlg, DWL_USER, (LONG)pBuf);
  
  /* select first item */
  hDlgCtrl = GetDlgItem(hwndDlg, 14009);
  SendMessageW(hDlgCtrl, LB_SETCURSEL, 0, 0);
  str = (WCHAR *)SendMessageW(hDlgCtrl, LB_GETITEMDATA, (WPARAM)0, (LPARAM)NULL);
  hDlgCtrl = GetDlgItem(hwndDlg, 14010);
  SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)str);
  return TRUE;
}

/*************************************************************************
 *
 * SH_FileVersionDlgProc
 *
 * wnd proc of 'Version' property sheet page
 */
INT_PTR 
CALLBACK 
SH_FileVersionDlgProc(   
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
  LPPROPSHEETPAGE ppsp;
  WCHAR * lpstr;
  LPVOID * buf;
  switch(uMsg)
  {
  case WM_INITDIALOG:
	 ppsp = (LPPROPSHEETPAGE)lParam;
	 if(ppsp == NULL)
	   break;

	 TRACE("WM_INITDIALOG hwnd %p lParam %p ppsplParam %x\n",hwndDlg, lParam, ppsp->lParam);

	 lpstr = (WCHAR *)ppsp->lParam;

	 if(lpstr == NULL)
	   break;

	 return SH_FileVersionInitialize(hwndDlg, lpstr);


  case WM_COMMAND:
     if(LOWORD(wParam) == 14009 && HIWORD(wParam) == LBN_DBLCLK)
	 {
       HWND hDlgCtrl;
	   LRESULT lresult;
	   WCHAR * str;
	   
	   hDlgCtrl = GetDlgItem(hwndDlg, 14009);
	   lresult = SendMessageW(hDlgCtrl, LB_GETCURSEL, (WPARAM)NULL, (LPARAM)NULL);
       if(lresult == LB_ERR)
	   {
	     break;
	   }
       str = (WCHAR *)SendMessageW(hDlgCtrl, LB_GETITEMDATA, (WPARAM)lresult, (LPARAM)NULL);

	   if(str == NULL)
	   {
		 break;
	   }
	   hDlgCtrl = GetDlgItem(hwndDlg, 14010);
       TRACE("hDlgCtrl %x string %s \n",hDlgCtrl, debugstr_w(str));
	   SendMessageW(hDlgCtrl, WM_SETTEXT, (WPARAM)NULL, (LPARAM)str);
       return TRUE;
	 }
  case WM_DESTROY:
       buf = (LPVOID)GetWindowLong(hwndDlg, DWL_USER);
	   HeapFree(GetProcessHeap(), 0, buf);
  default:
	  break;
  }
  return FALSE;
}

/*************************************************************************
 *
 * SH_FileGeneralDlgProc
 *
 * wnd proc of 'General' property sheet page
 *
 */

INT_PTR 
CALLBACK 
SH_FileGeneralDlgProc(   
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    LPPROPSHEETPAGEW ppsp;
    WCHAR * lpstr;
    switch(uMsg)
    {
    case WM_INITDIALOG:
        ppsp = (LPPROPSHEETPAGEW)lParam;
        if (ppsp == NULL)
            break;
        TRACE("WM_INITDIALOG hwnd %p lParam %p ppsplParam %S\n",hwndDlg, lParam, ppsp->lParam);

        lpstr = (WCHAR *)ppsp->lParam;

        if ( lpstr == NULL)
		{
            ERR("no filename\n");
            break;
        }
        /* set general text properties filename filelocation and icon */
        SH_FileGeneralSetText(hwndDlg, lpstr);
        /* enumerate file extension from registry and application which opens it*/
        SH_FileGeneralSetFileType(hwndDlg, strrchrW(lpstr, '.'));
        /* set file time create/modfied/accessed */
        SH_FileGeneralSetFileSizeTime(hwndDlg, lpstr, NULL);
        return TRUE;
  default:
      break;
  }
  return FALSE;
}


/*************************************************************************
 *
 * SH_ShowPropertiesDialog
 *
 * called from ShellExecuteExW32
 * 
 * lpf contains (quoted) path of folder/file
 *
 * TODO: provide button change application type if file has registered type
 *        make filename field editable and apply changes to filename on close
 */
	
BOOL
SH_ShowPropertiesDialog(WCHAR * lpf)
{
    PROPSHEETHEADERW pinfo;
    HPROPSHEETPAGE hppages[MAX_PROPERTY_SHEET_PAGE];
    HPROPSHEETPAGE hpage;
    WCHAR wFileName[MAX_PATH];
	UINT num_pages = 0;
	DWORD dwHandle = 0;

    TRACE("SH_ShowPropertiesDialog entered\n");

    if (lpf== NULL)
        return FALSE;
  
    if ( !strlenW(lpf) )
        return FALSE;

    memset(hppages, 0x0, sizeof(HPROPSHEETPAGE) * MAX_PROPERTY_SHEET_PAGE);
    if (lpf[0] == '"')
    {
        /* remove quotes from lpf */
        LPWSTR src = lpf + 1;
        LPWSTR dst = wFileName;
      
        while(*src && *src!='"')
            *dst++ = *src++;

        *dst = '\0';
    }
	else
	{
	    strcpyW(wFileName, lpf);
	}

    if (PathIsDirectoryW(wFileName) || strlenW(wFileName) == 3)
    {
        FIXME("directory / drive resources are missing\n");
        return FALSE;
    }
    hpage = SH_CreatePropertySheetPage("SHELL_FILE_GENERAL_DLG", SH_FileGeneralDlgProc, (LPARAM)wFileName);

    if (hpage == NULL)
        return FALSE;

    hppages[num_pages] = hpage;
    num_pages++;
    if ( GetFileVersionInfoSizeW(lpf, &dwHandle) )
	{
         if ( (hpage = SH_CreatePropertySheetPage("SHELL_FILE_VERSION_DLG",SH_FileVersionDlgProc, (LPARAM)lpf))!= NULL)
         {
              hppages[num_pages] = hpage;
		      num_pages++;
         }
	}
    memset(&pinfo, 0x0, sizeof(PROPSHEETHEADERW));
    pinfo.dwSize = sizeof(PROPSHEETHEADERW);
    pinfo.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE;
    pinfo.nPages = num_pages;
    pinfo.phpage = hppages;
    pinfo.pszCaption = wFileName;
    return (PropertySheetW(&pinfo) != -1);
}
/*EOF */
