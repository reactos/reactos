/*
 * The parameters of many functions changes between different OS versions
 * (NT uses Unicode strings, 95 uses ASCII strings)
 *
 * Copyright 1997 Marcus Meissner
 *           1998 Jürgen Schmied
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

#include <wine/config.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <wine/winternl.h>
#include <shlobj.h>
#include <undocshell.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <commoncontrols.h>
#include <recyclebin.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "pidl.h"
#include "shell32_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);
WINE_DECLARE_DEBUG_CHANNEL(pidl);

/* FIXME: !!! move CREATEMRULIST and flags to header file !!! */
/*        !!! it is in both here and comctl32undoc.c      !!! */
typedef struct tagCREATEMRULIST
{
    DWORD  cbSize;        /* size of struct */
    DWORD  nMaxItems;     /* max no. of items in list */
    DWORD  dwFlags;       /* see below */
    HKEY   hKey;          /* root reg. key under which list is saved */
    LPCSTR lpszSubKey;    /* reg. subkey */
    int (CALLBACK *lpfnCompare)(LPCVOID, LPCVOID, DWORD); /* item compare proc */
} CREATEMRULISTA, *LPCREATEMRULISTA;

/* dwFlags */
#define MRUF_STRING_LIST  0 /* list will contain strings */
#define MRUF_BINARY_LIST  1 /* list will contain binary data */
#define MRUF_DELAYED_SAVE 2 /* only save list order to reg. is FreeMRUList */

extern HANDLE WINAPI CreateMRUListA(LPCREATEMRULISTA lpcml);
extern VOID WINAPI FreeMRUList(HANDLE hMRUList);
extern INT    WINAPI AddMRUData(HANDLE hList, LPCVOID lpData, DWORD cbData);
extern INT    WINAPI FindMRUData(HANDLE hList, LPCVOID lpData, DWORD cbData, LPINT lpRegNum);
extern INT    WINAPI EnumMRUListA(HANDLE hList, INT nItemPos, LPVOID lpBuffer, DWORD nBufferSize);


/*************************************************************************
 * ParseFieldA					[internal]
 *
 * copies a field from a ',' delimited string
 *
 * first field is nField = 1
 */
DWORD WINAPI ParseFieldA(
	LPCSTR src,
	DWORD nField,
	LPSTR dst,
	DWORD len)
{
	WARN("(%s,0x%08x,%p,%d) semi-stub.\n",debugstr_a(src),nField,dst,len);

	if (!src || !src[0] || !dst || !len)
	  return 0;

	/* skip n fields delimited by ',' */
	while (nField > 1)
	{
	  if (*src=='\0') return FALSE;
	  if (*(src++)==',') nField--;
	}

	/* copy part till the next ',' to dst */
	while ( *src!='\0' && *src!=',' && (len--)>0 ) *(dst++)=*(src++);

	/* finalize the string */
	*dst=0x0;

	return TRUE;
}

/*************************************************************************
 * ParseFieldW			[internal]
 *
 * copies a field from a ',' delimited string
 *
 * first field is nField = 1
 */
DWORD WINAPI ParseFieldW(LPCWSTR src, DWORD nField, LPWSTR dst, DWORD len)
{
	WARN("(%s,0x%08x,%p,%d) semi-stub.\n", debugstr_w(src), nField, dst, len);

	if (!src || !src[0] || !dst || !len)
	  return 0;

	/* skip n fields delimited by ',' */
	while (nField > 1)
	{
	  if (*src == 0x0) return FALSE;
	  if (*src++ == ',') nField--;
	}

	/* copy part till the next ',' to dst */
	while ( *src != 0x0 && *src != ',' && (len--)>0 ) *(dst++) = *(src++);

	/* finalize the string */
	*dst = 0x0;

	return TRUE;
}

/*************************************************************************
 * ParseField			[SHELL32.58]
 */
DWORD WINAPI ParseFieldAW(LPCVOID src, DWORD nField, LPVOID dst, DWORD len)
{
	if (SHELL_OsIsUnicode())
	  return ParseFieldW(src, nField, dst, len);
	return ParseFieldA(src, nField, dst, len);
}

/*************************************************************************
 * GetFileNameFromBrowse            [SHELL32.63]
 *
 */
BOOL WINAPI GetFileNameFromBrowse(
    HWND hwndOwner,
    LPWSTR lpstrFile,
    UINT nMaxFile,
    LPCWSTR lpstrInitialDir,
    LPCWSTR lpstrDefExt,
    LPCWSTR lpstrFilter,
    LPCWSTR lpstrTitle)
{
typedef BOOL (WINAPI *GetOpenFileNameProc)(OPENFILENAMEW *ofn);
    HMODULE hmodule;
    GetOpenFileNameProc pGetOpenFileNameW;
    OPENFILENAMEW ofn;
    BOOL ret;

    TRACE("%p, %s, %d, %s, %s, %s, %s)\n",
      hwndOwner, debugstr_w(lpstrFile), nMaxFile, lpstrInitialDir, lpstrDefExt,
      lpstrFilter, lpstrTitle);

    hmodule = LoadLibraryW(L"comdlg32.dll");
    if(!hmodule) return FALSE;
    pGetOpenFileNameW = (GetOpenFileNameProc)GetProcAddress(hmodule, "GetOpenFileNameW");
    if(!pGetOpenFileNameW)
    {
    FreeLibrary(hmodule);
    return FALSE;
    }

    memset(&ofn, 0, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.lpstrFilter = lpstrFilter;
    ofn.lpstrFile = lpstrFile;
    ofn.nMaxFile = nMaxFile;
    ofn.lpstrInitialDir = lpstrInitialDir;
    ofn.lpstrTitle = lpstrTitle;
    ofn.lpstrDefExt = lpstrDefExt;
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    ret = pGetOpenFileNameW(&ofn);

    FreeLibrary(hmodule);
    return ret;
}

/*************************************************************************
 * SHGetSetSettings				[SHELL32.68]
 */
VOID WINAPI SHGetSetSettings(LPSHELLSTATE lpss, DWORD dwMask, BOOL bSet)
{
  if(bSet)
  {
    FIXME("%p 0x%08x TRUE\n", lpss, dwMask);
  }
  else
  {
    SHGetSettings((LPSHELLFLAGSTATE)lpss,dwMask);
  }
}

/*************************************************************************
 * SHGetSettings				[SHELL32.@]
 *
 * NOTES
 *  the registry path are for win98 (tested)
 *  and possibly are the same in nt40
 *
 */
VOID WINAPI SHGetSettings(LPSHELLFLAGSTATE lpsfs, DWORD dwMask)
{
	HKEY	hKey;
	DWORD	dwData;
	DWORD	dwDataSize = sizeof (DWORD);

	TRACE("(%p 0x%08x)\n",lpsfs,dwMask);

	if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
				 0, 0, 0, KEY_ALL_ACCESS, 0, &hKey, 0))
	  return;

	if ( (SSF_SHOWEXTENSIONS & dwMask) && !RegQueryValueExA(hKey, "HideFileExt", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fShowExtensions  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_SHOWINFOTIP & dwMask) && !RegQueryValueExA(hKey, "ShowInfoTip", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fShowInfoTip  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_DONTPRETTYPATH & dwMask) && !RegQueryValueExA(hKey, "DontPrettyPath", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fDontPrettyPath  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_HIDEICONS & dwMask) && !RegQueryValueExA(hKey, "HideIcons", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fHideIcons  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_MAPNETDRVBUTTON & dwMask) && !RegQueryValueExA(hKey, "MapNetDrvBtn", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fMapNetDrvBtn  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_SHOWATTRIBCOL & dwMask) && !RegQueryValueExA(hKey, "ShowAttribCol", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fShowAttribCol  = ((dwData == 0) ?  0 : 1);

	if (((SSF_SHOWALLOBJECTS | SSF_SHOWSYSFILES) & dwMask) && !RegQueryValueExA(hKey, "Hidden", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	{ if (dwData == 0)
	  { if (SSF_SHOWALLOBJECTS & dwMask)	lpsfs->fShowAllObjects  = 0;
	    if (SSF_SHOWSYSFILES & dwMask)	lpsfs->fShowSysFiles  = 0;
	  }
	  else if (dwData == 1)
	  { if (SSF_SHOWALLOBJECTS & dwMask)	lpsfs->fShowAllObjects  = 1;
	    if (SSF_SHOWSYSFILES & dwMask)	lpsfs->fShowSysFiles  = 0;
	  }
	  else if (dwData == 2)
	  { if (SSF_SHOWALLOBJECTS & dwMask)	lpsfs->fShowAllObjects  = 0;
	    if (SSF_SHOWSYSFILES & dwMask)	lpsfs->fShowSysFiles  = 1;
	  }
	}
	RegCloseKey (hKey);

	TRACE("-- 0x%04x\n", *(WORD*)lpsfs);
}

/*************************************************************************
 * SHShellFolderView_Message			[SHELL32.73]
 *
 * Send a message to an explorer cabinet window.
 *
 * PARAMS
 *  hwndCabinet [I] The window containing the shellview to communicate with
 *  dwMessage   [I] The SFVM message to send
 *  dwParam     [I] Message parameter
 *
 * RETURNS
 *  fixme.
 *
 * NOTES
 *  Message SFVM_REARRANGE = 1
 *
 *    This message gets sent when a column gets clicked to instruct the
 *    shell view to re-sort the item list. dwParam identifies the column
 *    that was clicked.
 */
LRESULT WINAPI SHShellFolderView_Message(
	HWND hwndCabinet,
	UINT uMessage,
	LPARAM lParam)
{
	FIXME("%p %08x %08lx stub\n",hwndCabinet, uMessage, lParam);
	return 0;
}

/*************************************************************************
 * RegisterShellHook				[SHELL32.181]
 *
 * Register a shell hook.
 *
 * PARAMS
 *      hwnd   [I]  Window handle
 *      dwType [I]  Type of hook.
 *
 * NOTES
 *     Exported by ordinal
 */
BOOL WINAPI RegisterShellHook(
	HWND hWnd,
	DWORD dwType)
{
    if (dwType == 3)
    {
        SetTaskmanWindow(hWnd);
        return RegisterShellHookWindow(hWnd);
    }
    else if (dwType == 0)
    {
        return DeregisterShellHookWindow(hWnd);
    }

    ERR("Unsupported argument");
    return FALSE;
}

/*************************************************************************
 * ShellMessageBoxW				[SHELL32.182]
 *
 * See ShellMessageBoxA.
 *
 * NOTE:
 * shlwapi.ShellMessageBoxWrapW is a duplicate of shell32.ShellMessageBoxW
 * because we can't forward to it in the .spec file since it's exported by
 * ordinal. If you change the implementation here please update the code in
 * shlwapi as well.
 */
int ShellMessageBoxW(
	HINSTANCE hInstance,
	HWND hWnd,
	LPCWSTR lpText,
	LPCWSTR lpCaption,
	UINT uType,
	...)
{
	WCHAR	szText[100],szTitle[100];
	LPCWSTR pszText = szText, pszTitle = szTitle;
	LPWSTR  pszTemp;
	__ms_va_list args;
	int	ret;

	__ms_va_start(args, uType);
	/* wvsprintfA(buf,fmt, args); */

	TRACE("(%p,%p,%p,%p,%08x)\n",
	    hInstance,hWnd,lpText,lpCaption,uType);

	if (IS_INTRESOURCE(lpCaption))
	  LoadStringW(hInstance, LOWORD(lpCaption), szTitle, sizeof(szTitle)/sizeof(szTitle[0]));
	else
	  pszTitle = lpCaption;

	if (IS_INTRESOURCE(lpText))
	  LoadStringW(hInstance, LOWORD(lpText), szText, sizeof(szText)/sizeof(szText[0]));
	else
	  pszText = lpText;

	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
		       pszText, 0, 0, (LPWSTR)&pszTemp, 0, &args);

	__ms_va_end(args);

	ret = MessageBoxW(hWnd,pszTemp,pszTitle,uType);
        LocalFree(pszTemp);
	return ret;
}

/*************************************************************************
 * ShellMessageBoxA				[SHELL32.183]
 *
 * Format and output an error message.
 *
 * PARAMS
 *  hInstance [I] Instance handle of message creator
 *  hWnd      [I] Window handle of message creator
 *  lpText    [I] Resource Id of title or LPSTR
 *  lpCaption [I] Resource Id of title or LPSTR
 *  uType     [I] Type of error message
 *
 * RETURNS
 *  A return value from MessageBoxA().
 *
 * NOTES
 *     Exported by ordinal
 */
int ShellMessageBoxA(
	HINSTANCE hInstance,
	HWND hWnd,
	LPCSTR lpText,
	LPCSTR lpCaption,
	UINT uType,
	...)
{
	char	szText[100],szTitle[100];
	LPCSTR  pszText = szText, pszTitle = szTitle;
	LPSTR   pszTemp;
	__ms_va_list args;
	int	ret;

	__ms_va_start(args, uType);
	/* wvsprintfA(buf,fmt, args); */

	TRACE("(%p,%p,%p,%p,%08x)\n",
	    hInstance,hWnd,lpText,lpCaption,uType);

	if (IS_INTRESOURCE(lpCaption))
	  LoadStringA(hInstance, LOWORD(lpCaption), szTitle, sizeof(szTitle));
	else
	  pszTitle = lpCaption;

	if (IS_INTRESOURCE(lpText))
	  LoadStringA(hInstance, LOWORD(lpText), szText, sizeof(szText));
	else
	  pszText = lpText;

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
		       pszText, 0, 0, (LPSTR)&pszTemp, 0, &args);

	__ms_va_end(args);

	ret = MessageBoxA(hWnd,pszTemp,pszTitle,uType);
        LocalFree(pszTemp);
	return ret;
}

/*************************************************************************
 * SHRegisterDragDrop				[SHELL32.86]
 *
 * Probably equivalent to RegisterDragDrop but under Windows 95 it could use the
 * shell32 built-in "mini-COM" without the need to load ole32.dll - see SHLoadOLE
 * for details. Under Windows 98 this function initializes the true OLE when called
 * the first time, on XP always returns E_OUTOFMEMORY and it got removed from Vista.
 *
 * We follow Windows 98 behaviour.
 *
 * NOTES
 *     exported by ordinal
 *
 * SEE ALSO
 *     RegisterDragDrop, SHLoadOLE
 */
HRESULT WINAPI SHRegisterDragDrop(
	HWND hWnd,
	LPDROPTARGET pDropTarget)
{
        static BOOL ole_initialized = FALSE;
        HRESULT hr;

        TRACE("(%p,%p)\n", hWnd, pDropTarget);

        if (!ole_initialized)
        {
            hr = OleInitialize(NULL);
            if (FAILED(hr))
                return hr;
            ole_initialized = TRUE;
        }
	return RegisterDragDrop(hWnd, pDropTarget);
}

/*************************************************************************
 * SHRevokeDragDrop				[SHELL32.87]
 *
 * Probably equivalent to RevokeDragDrop but under Windows 95 it could use the
 * shell32 built-in "mini-COM" without the need to load ole32.dll - see SHLoadOLE
 * for details. Function removed from Windows Vista.
 *
 * We call ole32 RevokeDragDrop which seems to work even if OleInitialize was
 * not called.
 *
 * NOTES
 *     exported by ordinal
 *
 * SEE ALSO
 *     RevokeDragDrop, SHLoadOLE
 */
HRESULT WINAPI SHRevokeDragDrop(HWND hWnd)
{
    TRACE("(%p)\n", hWnd);
    return RevokeDragDrop(hWnd);
}

/*************************************************************************
 * SHDoDragDrop					[SHELL32.88]
 *
 * Probably equivalent to DoDragDrop but under Windows 9x it could use the
 * shell32 built-in "mini-COM" without the need to load ole32.dll - see SHLoadOLE
 * for details
 *
 * NOTES
 *     exported by ordinal
 *
 * SEE ALSO
 *     DoDragDrop, SHLoadOLE
 */
HRESULT WINAPI SHDoDragDrop(
	HWND hWnd,
	LPDATAOBJECT lpDataObject,
	LPDROPSOURCE lpDropSource,
	DWORD dwOKEffect,
	LPDWORD pdwEffect)
{
    FIXME("(%p %p %p 0x%08x %p):stub.\n",
    hWnd, lpDataObject, lpDropSource, dwOKEffect, pdwEffect);
	return DoDragDrop(lpDataObject, lpDropSource, dwOKEffect, pdwEffect);
}

/*************************************************************************
 * ArrangeWindows				[SHELL32.184]
 *
 */
WORD WINAPI ArrangeWindows(HWND hwndParent, DWORD dwReserved, const RECT *lpRect,
        WORD cKids, const HWND *lpKids)
{
    /* Unimplemented in WinXP SP3 */
    TRACE("(%p 0x%08x %p 0x%04x %p):stub.\n",
	   hwndParent, dwReserved, lpRect, cKids, lpKids);
    return 0;
}

/*************************************************************************
 * SignalFileOpen				[SHELL32.103]
 *
 * NOTES
 *     exported by ordinal
 */
BOOL WINAPI
SignalFileOpen (PCIDLIST_ABSOLUTE pidl)
{
    FIXME("(%p):stub.\n", pidl);

    return FALSE;
}

/*************************************************************************
 * SHADD_get_policy - helper function for SHAddToRecentDocs
 *
 * PARAMETERS
 *   policy    [IN]  policy name (null termed string) to find
 *   type      [OUT] ptr to DWORD to receive type
 *   buffer    [OUT] ptr to area to hold data retrieved
 *   len       [IN/OUT] ptr to DWORD holding size of buffer and getting
 *                      length filled
 *
 * RETURNS
 *   result of the SHQueryValueEx call
 */
static INT SHADD_get_policy(LPCSTR policy, LPDWORD type, LPVOID buffer, LPDWORD len)
{
    HKEY Policy_basekey;
    INT ret;

    /* Get the key for the policies location in the registry
     */
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
		      "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
		      0, KEY_READ, &Policy_basekey)) {

	if (RegOpenKeyExA(HKEY_CURRENT_USER,
			  "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
			  0, KEY_READ, &Policy_basekey)) {
	    TRACE("No Explorer Policies location exists. Policy wanted=%s\n",
		  policy);
	    *len = 0;
	    return ERROR_FILE_NOT_FOUND;
	}
    }

    /* Retrieve the data if it exists
     */
    ret = SHQueryValueExA(Policy_basekey, policy, 0, type, buffer, len);
    RegCloseKey(Policy_basekey);
    return ret;
}


/*************************************************************************
 * SHADD_compare_mru - helper function for SHAddToRecentDocs
 *
 * PARAMETERS
 *   data1     [IN] data being looked for
 *   data2     [IN] data in MRU
 *   cbdata    [IN] length from FindMRUData call (not used)
 *
 * RETURNS
 *   position within MRU list that data was added.
 */
static INT CALLBACK SHADD_compare_mru(LPCVOID data1, LPCVOID data2, DWORD cbData)
{
    return lstrcmpiA(data1, data2);
}

/*************************************************************************
 * SHADD_create_add_mru_data - helper function for SHAddToRecentDocs
 *
 * PARAMETERS
 *   mruhandle    [IN] handle for created MRU list
 *   doc_name     [IN] null termed pure doc name
 *   new_lnk_name [IN] null termed path and file name for .lnk file
 *   buffer       [IN/OUT] 2048 byte area to construct MRU data
 *   len          [OUT] ptr to int to receive space used in buffer
 *
 * RETURNS
 *   position within MRU list that data was added.
 */
static INT SHADD_create_add_mru_data(HANDLE mruhandle, LPCSTR doc_name, LPCSTR new_lnk_name,
                                     LPSTR buffer, INT *len)
{
    LPSTR ptr;
    INT wlen;

    /*FIXME: Document:
     *  RecentDocs MRU data structure seems to be:
     *    +0h   document file name w/ terminating 0h
     *    +nh   short int w/ size of remaining
     *    +n+2h 02h 30h, or 01h 30h, or 00h 30h  -  unknown
     *    +n+4h 10 bytes zeros  -   unknown
     *    +n+eh shortcut file name w/ terminating 0h
     *    +n+e+nh 3 zero bytes  -  unknown
     */

    /* Create the MRU data structure for "RecentDocs"
	 */
    ptr = buffer;
    lstrcpyA(ptr, doc_name);
    ptr += (lstrlenA(buffer) + 1);
    wlen= lstrlenA(new_lnk_name) + 1 + 12;
    *((short int*)ptr) = wlen;
    ptr += 2;   /* step past the length */
    *(ptr++) = 0x30;  /* unknown reason */
    *(ptr++) = 0;     /* unknown, but can be 0x00, 0x01, 0x02 */
    memset(ptr, 0, 10);
    ptr += 10;
    lstrcpyA(ptr, new_lnk_name);
    ptr += (lstrlenA(new_lnk_name) + 1);
    memset(ptr, 0, 3);
    ptr += 3;
    *len = ptr - buffer;

    /* Add the new entry into the MRU list
     */
    return AddMRUData(mruhandle, buffer, *len);
}

/*************************************************************************
 * SHAddToRecentDocs				[SHELL32.@]
 *
 * Modify (add/clear) Shell's list of recently used documents.
 *
 * PARAMETERS
 *   uFlags  [IN] SHARD_PATHA, SHARD_PATHW or SHARD_PIDL
 *   pv      [IN] string or pidl, NULL clears the list
 *
 * NOTES
 *     exported by name
 *
 * FIXME
 *  convert to unicode
 */
void WINAPI SHAddToRecentDocs (UINT uFlags,LPCVOID pv)
{
/* If list is a string list lpfnCompare has the following prototype
 * int CALLBACK MRUCompareString(LPCSTR s1, LPCSTR s2)
 * for binary lists the prototype is
 * int CALLBACK MRUCompareBinary(LPCVOID data1, LPCVOID data2, DWORD cbData)
 * where cbData is the no. of bytes to compare.
 * Need to check what return value means identical - 0?
 */


    UINT olderrormode;
    HKEY HCUbasekey;
    CHAR doc_name[MAX_PATH];
    CHAR link_dir[MAX_PATH];
    CHAR new_lnk_filepath[MAX_PATH];
    CHAR new_lnk_name[MAX_PATH];
    CHAR * ext;
    IMalloc *ppM;
    LPITEMIDLIST pidl;
    HWND hwnd = 0;       /* FIXME:  get real window handle */
    INT ret;
    DWORD data[64], datalen, type;

    TRACE("%04x %p\n", uFlags, pv);

    /*FIXME: Document:
     *  RecentDocs MRU data structure seems to be:
     *    +0h   document file name w/ terminating 0h
     *    +nh   short int w/ size of remaining
     *    +n+2h 02h 30h, or 01h 30h, or 00h 30h  -  unknown
     *    +n+4h 10 bytes zeros  -   unknown
     *    +n+eh shortcut file name w/ terminating 0h
     *    +n+e+nh 3 zero bytes  -  unknown
     */

    /* See if we need to do anything.
     */
    datalen = 64;
    ret=SHADD_get_policy( "NoRecentDocsHistory", &type, data, &datalen);
    if ((ret > 0) && (ret != ERROR_FILE_NOT_FOUND)) {
	ERR("Error %d getting policy \"NoRecentDocsHistory\"\n", ret);
	return;
    }
    if (ret == ERROR_SUCCESS) {
	if (!( (type == REG_DWORD) ||
	       ((type == REG_BINARY) && (datalen == 4)) )) {
	    ERR("Error policy data for \"NoRecentDocsHistory\" not formatted correctly, type=%d, len=%d\n",
		type, datalen);
	    return;
	}

	TRACE("policy value for NoRecentDocsHistory = %08x\n", data[0]);
	/* now test the actual policy value */
	if ( data[0] != 0)
	    return;
    }

    /* Open key to where the necessary info is
     */
    /* FIXME: This should be done during DLL PROCESS_ATTACH (or THREAD_ATTACH)
     *        and the close should be done during the _DETACH. The resulting
     *        key is stored in the DLL global data.
     */
    if (RegCreateKeyExA(HKEY_CURRENT_USER,
			"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
			0, 0, 0, KEY_READ, 0, &HCUbasekey, 0)) {
	ERR("Failed to create 'Software\\Microsoft\\Windows\\CurrentVersion\\Explorer'\n");
	return;
    }

    /* Get path to user's "Recent" directory
     */
    if(SUCCEEDED(SHGetMalloc(&ppM))) {
	if (SUCCEEDED(SHGetSpecialFolderLocation(hwnd, CSIDL_RECENT,
						 &pidl))) {
	    SHGetPathFromIDListA(pidl, link_dir);
	    IMalloc_Free(ppM, pidl);
	}
	else {
	    /* serious issues */
	    link_dir[0] = 0;
	    ERR("serious issues 1\n");
	}
	IMalloc_Release(ppM);
    }
    else {
	/* serious issues */
	link_dir[0] = 0;
	ERR("serious issues 2\n");
    }
    TRACE("Users Recent dir %s\n", link_dir);

    /* If no input, then go clear the lists */
    if (!pv) {
	/* clear user's Recent dir
	 */

	/* FIXME: delete all files in "link_dir"
	 *
	 * while( more files ) {
	 *    lstrcpyA(old_lnk_name, link_dir);
	 *    PathAppendA(old_lnk_name, filenam);
	 *    DeleteFileA(old_lnk_name);
	 * }
	 */
	FIXME("should delete all files in %s\\\n", link_dir);

	/* clear MRU list
	 */
	/* MS Bug ?? v4.72.3612.1700 of shell32 does the delete against
	 *  HKEY_LOCAL_MACHINE version of ...CurrentVersion\Explorer
	 *  and naturally it fails w/ rc=2. It should do it against
	 *  HKEY_CURRENT_USER which is where it is stored, and where
	 *  the MRU routines expect it!!!!
	 */
	RegDeleteKeyA(HCUbasekey, "RecentDocs");
	RegCloseKey(HCUbasekey);
	return;
    }

    /* Have data to add, the jobs to be done:
     *   1. Add document to MRU list in registry "HKCU\Software\
     *      Microsoft\Windows\CurrentVersion\Explorer\RecentDocs".
     *   2. Add shortcut to document in the user's Recent directory
     *      (CSIDL_RECENT).
     *   3. Add shortcut to Start menu's Documents submenu.
     */

    /* Get the pure document name from the input
     */
    switch (uFlags)
    {
    case SHARD_PIDL:
        SHGetPathFromIDListA(pv, doc_name);
        break;

    case SHARD_PATHA:
        lstrcpynA(doc_name, pv, MAX_PATH);
        break;

    case SHARD_PATHW:
        WideCharToMultiByte(CP_ACP, 0, pv, -1, doc_name, MAX_PATH, NULL, NULL);
        break;

    default:
        FIXME("Unsupported flags: %u\n", uFlags);
        return;
    }

    TRACE("full document name %s\n", debugstr_a(doc_name));

#ifdef __REACTOS__
    /* check if file is a shortcut */
    ext = strrchr(doc_name, '.');
    if (!lstrcmpiA(ext, ".lnk"))
    {
        IShellLinkA* ShellLink;
        IShellLink_ConstructFromFile(NULL, &IID_IShellLinkA, (LPCITEMIDLIST)SHSimpleIDListFromPathA(doc_name), (LPVOID*)&ShellLink);
        IShellLinkA_GetPath(ShellLink, doc_name, MAX_PATH, NULL, 0);
        IShellLinkA_Release(ShellLink);
    }

    ext = strrchr(doc_name, '.');
    if (!lstrcmpiA(ext, ".exe"))
    {
        /* executables are not added */
        return;
    }
#endif

    PathStripPathA(doc_name);
    TRACE("stripped document name %s\n", debugstr_a(doc_name));


    /* ***  JOB 1: Update registry for ...\Explorer\RecentDocs list  *** */

    {  /* on input needs:
	*      doc_name    -  pure file-spec, no path
	*      link_dir    -  path to the user's Recent directory
	*      HCUbasekey  -  key of ...Windows\CurrentVersion\Explorer" node
	* creates:
	*      new_lnk_name-  pure file-spec, no path for new .lnk file
	*      new_lnk_filepath
	*                  -  path and file name of new .lnk file
	*/
	CREATEMRULISTA mymru;
	HANDLE mruhandle;
	INT len, pos, bufused, err;
	INT i;
	DWORD attr;
	CHAR buffer[2048];
	CHAR *ptr;
	CHAR old_lnk_name[MAX_PATH];
	short int slen;

	mymru.cbSize = sizeof(CREATEMRULISTA);
	mymru.nMaxItems = 15;
	mymru.dwFlags = MRUF_BINARY_LIST | MRUF_DELAYED_SAVE;
	mymru.hKey = HCUbasekey;
	mymru.lpszSubKey = "RecentDocs";
        mymru.lpfnCompare = SHADD_compare_mru;
	mruhandle = CreateMRUListA(&mymru);
	if (!mruhandle) {
	    /* MRU failed */
	    ERR("MRU processing failed, handle zero\n");
	    RegCloseKey(HCUbasekey);
	    return;
	}
	len = lstrlenA(doc_name);
	pos = FindMRUData(mruhandle, doc_name, len, 0);

	/* Now get the MRU entry that will be replaced
	 * and delete the .lnk file for it
	 */
	if ((bufused = EnumMRUListA(mruhandle, (pos == -1) ? 14 : pos,
                                    buffer, 2048)) != -1) {
	    ptr = buffer;
	    ptr += (lstrlenA(buffer) + 1);
	    slen = *((short int*)ptr);
	    ptr += 2;  /* skip the length area */
	    if (bufused >= slen + (ptr-buffer)) {
		/* buffer size looks good */
		ptr += 12; /* get to string */
		len = bufused - (ptr-buffer);  /* get length of buf remaining */
		if ((lstrlenA(ptr) > 0) && (lstrlenA(ptr) <= len-1)) {
		    /* appears to be good string */
		    lstrcpyA(old_lnk_name, link_dir);
		    PathAppendA(old_lnk_name, ptr);
		    if (!DeleteFileA(old_lnk_name)) {
			if ((attr = GetFileAttributesA(old_lnk_name)) == INVALID_FILE_ATTRIBUTES) {
			    if ((err = GetLastError()) != ERROR_FILE_NOT_FOUND) {
				ERR("Delete for %s failed, err=%d, attr=%08x\n",
				    old_lnk_name, err, attr);
			    }
			    else {
				TRACE("old .lnk file %s did not exist\n",
				      old_lnk_name);
			    }
			}
			else {
			    ERR("Delete for %s failed, attr=%08x\n",
				old_lnk_name, attr);
			}
		    }
		    else {
			TRACE("deleted old .lnk file %s\n", old_lnk_name);
		    }
		}
	    }
	}

	/* Create usable .lnk file name for the "Recent" directory
	 */
	wsprintfA(new_lnk_name, "%s.lnk", doc_name);
	lstrcpyA(new_lnk_filepath, link_dir);
	PathAppendA(new_lnk_filepath, new_lnk_name);
	i = 1;
	olderrormode = SetErrorMode(SEM_FAILCRITICALERRORS);
	while (GetFileAttributesA(new_lnk_filepath) != INVALID_FILE_ATTRIBUTES) {
	    i++;
	    wsprintfA(new_lnk_name, "%s (%u).lnk", doc_name, i);
	    lstrcpyA(new_lnk_filepath, link_dir);
	    PathAppendA(new_lnk_filepath, new_lnk_name);
	}
	SetErrorMode(olderrormode);
	TRACE("new shortcut will be %s\n", new_lnk_filepath);

	/* Now add the new MRU entry and data
	 */
	pos = SHADD_create_add_mru_data(mruhandle, doc_name, new_lnk_name,
					buffer, &len);
	FreeMRUList(mruhandle);
	TRACE("Updated MRU list, new doc is position %d\n", pos);
    }

    /* ***  JOB 2: Create shortcut in user's "Recent" directory  *** */

    {  /* on input needs:
	*      doc_name    -  pure file-spec, no path
	*      new_lnk_filepath
	*                  -  path and file name of new .lnk file
 	*      uFlags[in]  -  flags on call to SHAddToRecentDocs
	*      pv[in]      -  document path/pidl on call to SHAddToRecentDocs
	*/
	IShellLinkA *psl = NULL;
	IPersistFile *pPf = NULL;
	HRESULT hres;
	CHAR desc[MAX_PATH];
	WCHAR widelink[MAX_PATH];

	CoInitialize(0);

	hres = CoCreateInstance( &CLSID_ShellLink,
				 NULL,
				 CLSCTX_INPROC_SERVER,
				 &IID_IShellLinkA,
				 (LPVOID )&psl);
	if(SUCCEEDED(hres)) {

	    hres = IShellLinkA_QueryInterface(psl, &IID_IPersistFile,
					     (LPVOID *)&pPf);
	    if(FAILED(hres)) {
		/* bombed */
		ERR("failed QueryInterface for IPersistFile %08x\n", hres);
		goto fail;
	    }

	    /* Set the document path or pidl */
	    if (uFlags == SHARD_PIDL) {
                hres = IShellLinkA_SetIDList(psl, pv);
	    } else {
                hres = IShellLinkA_SetPath(psl, pv);
	    }
	    if(FAILED(hres)) {
		/* bombed */
		ERR("failed Set{IDList|Path} %08x\n", hres);
		goto fail;
	    }

	    lstrcpyA(desc, "Shortcut to ");
	    lstrcatA(desc, doc_name);
	    hres = IShellLinkA_SetDescription(psl, desc);
	    if(FAILED(hres)) {
		/* bombed */
		ERR("failed SetDescription %08x\n", hres);
		goto fail;
	    }

	    MultiByteToWideChar(CP_ACP, 0, new_lnk_filepath, -1,
				widelink, MAX_PATH);
	    /* create the short cut */
	    hres = IPersistFile_Save(pPf, widelink, TRUE);
	    if(FAILED(hres)) {
		/* bombed */
		ERR("failed IPersistFile::Save %08x\n", hres);
		IPersistFile_Release(pPf);
		IShellLinkA_Release(psl);
		goto fail;
	    }
	    hres = IPersistFile_SaveCompleted(pPf, widelink);
	    IPersistFile_Release(pPf);
	    IShellLinkA_Release(psl);
	    TRACE("shortcut %s has been created, result=%08x\n",
		  new_lnk_filepath, hres);
	}
	else {
	    ERR("CoCreateInstance failed, hres=%08x\n", hres);
	}
    }

 fail:
    CoUninitialize();

    /* all done */
    RegCloseKey(HCUbasekey);
    return;
}

/*************************************************************************
 * SHCreateShellFolderViewEx			[SHELL32.174]
 *
 * Create a new instance of the default Shell folder view object.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: error value
 *
 * NOTES
 *  see IShellFolder::CreateViewObject
 */
HRESULT WINAPI SHCreateShellFolderViewEx(
	LPCSFV psvcbi,    /* [in] shelltemplate struct */
	IShellView **ppv) /* [out] IShellView pointer */
{
	IShellView * psf;
	HRESULT hRes;

	TRACE("sf=%p pidl=%p cb=%p mode=0x%08x parm=%p\n",
	  psvcbi->pshf, psvcbi->pidl, psvcbi->pfnCallback,
	  psvcbi->fvm, psvcbi->psvOuter);

	*ppv = NULL;
    hRes = IShellView_Constructor(psvcbi->pshf, &psf);

    if (FAILED(hRes))
        return hRes;

	hRes = IShellView_QueryInterface(psf, &IID_IShellView, (LPVOID *)ppv);
	IShellView_Release(psf);

	return hRes;
}
/*************************************************************************
 *  SHWinHelp					[SHELL32.127]
 *
 */
HRESULT WINAPI SHWinHelp (DWORD v, DWORD w, DWORD x, DWORD z)
{	FIXME("0x%08x 0x%08x 0x%08x 0x%08x stub\n",v,w,x,z);
	return 0;
}
/*************************************************************************
 *  SHRunControlPanel [SHELL32.161]
 *
 */
BOOL WINAPI SHRunControlPanel (LPCWSTR commandLine, HWND parent)
{
	FIXME("(%s, %p): stub\n", debugstr_w(commandLine), parent);
	return FALSE;
}

static LPUNKNOWN SHELL32_IExplorerInterface=0;
/*************************************************************************
 * SHSetInstanceExplorer			[SHELL32.176]
 *
 * NOTES
 *  Sets the interface
 */
VOID WINAPI SHSetInstanceExplorer (LPUNKNOWN lpUnknown)
{	TRACE("%p\n", lpUnknown);
	SHELL32_IExplorerInterface = lpUnknown;
}
/*************************************************************************
 * SHGetInstanceExplorer			[SHELL32.@]
 *
 * NOTES
 *  gets the interface pointer of the explorer and a reference
 */
HRESULT WINAPI SHGetInstanceExplorer (IUnknown **lpUnknown)
{	TRACE("%p\n", lpUnknown);

	*lpUnknown = SHELL32_IExplorerInterface;

	if (!SHELL32_IExplorerInterface)
	  return E_FAIL;

	IUnknown_AddRef(SHELL32_IExplorerInterface);
	return S_OK;
}
/*************************************************************************
 * SHFreeUnusedLibraries			[SHELL32.123]
 *
 * Probably equivalent to CoFreeUnusedLibraries but under Windows 9x it could use
 * the shell32 built-in "mini-COM" without the need to load ole32.dll - see SHLoadOLE
 * for details
 *
 * NOTES
 *     exported by ordinal
 *
 * SEE ALSO
 *     CoFreeUnusedLibraries, SHLoadOLE
 */
void WINAPI SHFreeUnusedLibraries (void)
{
	FIXME("stub\n");
	CoFreeUnusedLibraries();
}
/*************************************************************************
 * DAD_AutoScroll				[SHELL32.129]
 *
 */
BOOL WINAPI DAD_AutoScroll(HWND hwnd, AUTO_SCROLL_DATA *samples, const POINT * pt)
{
    FIXME("hwnd = %p %p %p\n",hwnd,samples,pt);
    return FALSE;
}
/*************************************************************************
 * DAD_DragEnter				[SHELL32.130]
 *
 */
BOOL WINAPI DAD_DragEnter(HWND hwnd)
{
    FIXME("hwnd = %p\n",hwnd);
    return FALSE;
}
/*************************************************************************
 * DAD_DragEnterEx				[SHELL32.131]
 *
 */
BOOL WINAPI DAD_DragEnterEx(HWND hwnd, POINT p)
{
    FIXME("hwnd = %p (%d,%d)\n",hwnd,p.x,p.y);
    return FALSE;
}
/*************************************************************************
 * DAD_DragMove				[SHELL32.134]
 *
 */
BOOL WINAPI DAD_DragMove(POINT p)
{
    FIXME("(%d,%d)\n",p.x,p.y);
    return FALSE;
}
/*************************************************************************
 * DAD_DragLeave				[SHELL32.132]
 *
 */
BOOL WINAPI DAD_DragLeave(VOID)
{
    FIXME("\n");
    return FALSE;
}
/*************************************************************************
 * DAD_SetDragImage				[SHELL32.136]
 *
 * NOTES
 *  exported by name
 */
BOOL WINAPI DAD_SetDragImage(
	HIMAGELIST himlTrack,
	LPPOINT lppt)
{
    FIXME("%p %p stub\n",himlTrack, lppt);
    return FALSE;
}
/*************************************************************************
 * DAD_ShowDragImage				[SHELL32.137]
 *
 * NOTES
 *  exported by name
 */
BOOL WINAPI DAD_ShowDragImage(BOOL bShow)
{
    FIXME("0x%08x stub\n",bShow);
    return FALSE;
}

static const WCHAR szwCabLocation[] = {
  'S','o','f','t','w','a','r','e','\\',
  'M','i','c','r','o','s','o','f','t','\\',
  'W','i','n','d','o','w','s','\\',
  'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
  'E','x','p','l','o','r','e','r','\\',
  'C','a','b','i','n','e','t','S','t','a','t','e',0
};

static const WCHAR szwSettings[] = { 'S','e','t','t','i','n','g','s',0 };

/*************************************************************************
 * ReadCabinetState				[SHELL32.651] NT 4.0
 *
 */
BOOL WINAPI ReadCabinetState(CABINETSTATE *cs, int length)
{
	HKEY hkey = 0;
	DWORD type, r;

	TRACE("%p %d\n", cs, length);

	if( (cs == NULL) || (length < (int)sizeof(*cs))  )
		return FALSE;

	r = RegOpenKeyW( HKEY_CURRENT_USER, szwCabLocation, &hkey );
	if( r == ERROR_SUCCESS )
	{
		type = REG_BINARY;
		r = RegQueryValueExW( hkey, szwSettings,
			NULL, &type, (LPBYTE)cs, (LPDWORD)&length );
		RegCloseKey( hkey );

	}

	/* if we can't read from the registry, create default values */
	if ( (r != ERROR_SUCCESS) || (cs->cLength < sizeof(*cs)) ||
		(cs->cLength != length) )
	{
		ERR("Initializing shell cabinet settings\n");
		memset(cs, 0, sizeof(*cs));
		cs->cLength          = sizeof(*cs);
		cs->nVersion         = 2;
		cs->fFullPathTitle   = FALSE;
		cs->fSaveLocalView   = TRUE;
		cs->fNotShell        = FALSE;
		cs->fSimpleDefault   = TRUE;
		cs->fDontShowDescBar = FALSE;
		cs->fNewWindowMode   = FALSE;
		cs->fShowCompColor   = FALSE;
		cs->fDontPrettyNames = FALSE;
		cs->fAdminsCreateCommonGroups = TRUE;
		cs->fMenuEnumFilter  = 96;
	}

	return TRUE;
}

/*************************************************************************
 * WriteCabinetState				[SHELL32.652] NT 4.0
 *
 */
BOOL WINAPI WriteCabinetState(CABINETSTATE *cs)
{
	DWORD r;
	HKEY hkey = 0;

	TRACE("%p\n",cs);

	if( cs == NULL )
		return FALSE;

	r = RegCreateKeyExW( HKEY_CURRENT_USER, szwCabLocation, 0,
		 NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL);
	if( r == ERROR_SUCCESS )
	{
		r = RegSetValueExW( hkey, szwSettings, 0,
			REG_BINARY, (LPBYTE) cs, cs->cLength);

		RegCloseKey( hkey );
	}

	return (r==ERROR_SUCCESS);
}

/*************************************************************************
 * FileIconInit 				[SHELL32.660]
 *
 */
BOOL WINAPI FileIconInit(BOOL bFullInit)
{	FIXME("(%s)\n", bFullInit ? "true" : "false");
	return FALSE;
}

/*************************************************************************
 * IsUserAnAdmin    [SHELL32.680] NT 4.0
 *
 * Checks whether the current user is a member of the Administrators group.
 *
 * PARAMS
 *     None
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI IsUserAnAdmin(VOID)
{
    SID_IDENTIFIER_AUTHORITY Authority = {SECURITY_NT_AUTHORITY};
    HANDLE hToken;
    DWORD dwSize;
    PTOKEN_GROUPS lpGroups;
    PSID lpSid;
    DWORD i;
    BOOL bResult = FALSE;

    TRACE("\n");
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        return FALSE;
    }

    if (!GetTokenInformation(hToken, TokenGroups, NULL, 0, &dwSize))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            CloseHandle(hToken);
            return FALSE;
        }
    }

    lpGroups = HeapAlloc(GetProcessHeap(), 0, dwSize);
    if (lpGroups == NULL)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    if (!GetTokenInformation(hToken, TokenGroups, lpGroups, dwSize, &dwSize))
    {
        HeapFree(GetProcessHeap(), 0, lpGroups);
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);
    if (!AllocateAndInitializeSid(&Authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                  &lpSid))
    {
        HeapFree(GetProcessHeap(), 0, lpGroups);
        return FALSE;
    }

    for (i = 0; i < lpGroups->GroupCount; i++)
    {
        if (EqualSid(lpSid, lpGroups->Groups[i].Sid))
        {
            bResult = TRUE;
            break;
        }
    }

    FreeSid(lpSid);
    HeapFree(GetProcessHeap(), 0, lpGroups);
    return bResult;
}

/*************************************************************************
 * SetAppStartingCursor				[SHELL32.99]
 */
HRESULT WINAPI SetAppStartingCursor(HWND u, DWORD v)
{	FIXME("hwnd=%p 0x%04x stub\n",u,v );
	return 0;
}

/*************************************************************************
 * SHLoadOLE					[SHELL32.151]
 *
 * To reduce the memory usage of Windows 95, its shell32 contained an
 * internal implementation of a part of COM (see e.g. SHGetMalloc, SHCoCreateInstance,
 * SHRegisterDragDrop etc.) that allowed to use in-process STA objects without
 * the need to load OLE32.DLL. If OLE32.DLL was already loaded, the SH* function
 * would just call the Co* functions.
 *
 * The SHLoadOLE was called when OLE32.DLL was being loaded to transfer all the
 * information from the shell32 "mini-COM" to ole32.dll.
 *
 * See http://blogs.msdn.com/oldnewthing/archive/2004/07/05/173226.aspx for a
 * detailed description.
 *
 * Under wine ole32.dll is always loaded as it is imported by shlwapi.dll which is
 * imported by shell32 and no "mini-COM" is used (except for the "LoadWithoutCOM"
 * hack in SHCoCreateInstance)
 */
HRESULT WINAPI SHLoadOLE(LPARAM lParam)
{	FIXME("0x%08lx stub\n",lParam);
	return S_OK;
}
/*************************************************************************
 * DriveType					[SHELL32.64]
 *
 */
int WINAPI DriveType(int DriveType)
{
    WCHAR root[] = L"A:\\";
    root[0] = L'A' + DriveType;
    return GetDriveTypeW(root);
}
/*************************************************************************
 * InvalidateDriveType            [SHELL32.65]
 * Unimplemented in XP SP3
 */
int WINAPI InvalidateDriveType(int u)
{
    TRACE("0x%08x stub\n",u);
    return 0;
}
/*************************************************************************
 * SHAbortInvokeCommand				[SHELL32.198]
 *
 */
HRESULT WINAPI SHAbortInvokeCommand(void)
{	FIXME("stub\n");
	return 1;
}
/*************************************************************************
 * SHOutOfMemoryMessageBox			[SHELL32.126]
 *
 */
int WINAPI SHOutOfMemoryMessageBox(
	HWND hwndOwner,
	LPCSTR lpCaption,
	UINT uType)
{
	FIXME("%p %s 0x%08x stub\n",hwndOwner, lpCaption, uType);
	return 0;
}
/*************************************************************************
 * SHFlushClipboard				[SHELL32.121]
 *
 */
HRESULT WINAPI SHFlushClipboard(void)
{
    return OleFlushClipboard();
}

/*************************************************************************
 * SHWaitForFileToOpen				[SHELL32.97]
 *
 */
BOOL WINAPI SHWaitForFileToOpen(
	LPCITEMIDLIST pidl,
	DWORD dwFlags,
	DWORD dwTimeout)
{
	FIXME("%p 0x%08x 0x%08x stub\n", pidl, dwFlags, dwTimeout);
	return FALSE;
}

/************************************************************************
 *	RLBuildListOfPaths			[SHELL32.146]
 *
 * NOTES
 *   builds a DPA
 */
DWORD WINAPI RLBuildListOfPaths (void)
{	FIXME("stub\n");
	return 0;
}
/************************************************************************
 *	SHValidateUNC				[SHELL32.173]
 *
 */
BOOL WINAPI SHValidateUNC (HWND hwndOwner, PWSTR pszFile, UINT fConnect)
{
	FIXME("(%p, %s, 0x%08x): stub\n", hwndOwner, debugstr_w(pszFile), fConnect);
	return FALSE;
}

/************************************************************************
 * DoEnvironmentSubstA [SHELL32.@]
 *
 * See DoEnvironmentSubstW.
 */
DWORD WINAPI DoEnvironmentSubstA(LPSTR pszString, UINT cchString)
{
    LPSTR dst;
    BOOL res = FALSE;
    DWORD len = cchString;

    TRACE("(%s, %d)\n", debugstr_a(pszString), cchString);
    if (pszString == NULL) /* Really return 0? */
        return 0;
    if ((dst = (LPSTR)HeapAlloc(GetProcessHeap(), 0, cchString * sizeof(CHAR))))
    {
        len = ExpandEnvironmentStringsA(pszString, dst, cchString);
        /* len includes the terminating 0 */
        if (len && len < cchString)
        {
            res = TRUE;
            memcpy(pszString, dst, len);
        }
        else
            len = cchString;

        HeapFree(GetProcessHeap(), 0, dst);
    }
    return MAKELONG(len, res);
}

/************************************************************************
 * DoEnvironmentSubstW [SHELL32.@]
 *
 * Replace all %KEYWORD% in the string with the value of the named
 * environment variable. If the buffer is too small, the string is not modified.
 *
 * PARAMS
 *  pszString  [I] '\0' terminated string with %keyword%.
 *             [O] '\0' terminated string with %keyword% substituted.
 *  cchString  [I] size of str.
 *
 * RETURNS
 *  Success:  The string in the buffer is updated
 *            HIWORD: TRUE
 *            LOWORD: characters used in the buffer, including space for the terminating 0
 *  Failure:  buffer too small. The string is not modified.
 *            HIWORD: FALSE
 *            LOWORD: provided size of the buffer in characters
 */
DWORD WINAPI DoEnvironmentSubstW(LPWSTR pszString, UINT cchString)
{
    LPWSTR dst;
    BOOL res = FALSE;
    DWORD len = cchString;

    TRACE("(%s, %d)\n", debugstr_w(pszString), cchString);

    if ((cchString < MAXLONG) && (dst = HeapAlloc(GetProcessHeap(), 0, cchString * sizeof(WCHAR))))
    {
        len = ExpandEnvironmentStringsW(pszString, dst, cchString);
        /* len includes the terminating 0 */
        if (len && len <= cchString)
        {
            res = TRUE;
            memcpy(pszString, dst, len * sizeof(WCHAR));
        }
        else
            len = cchString;

        HeapFree(GetProcessHeap(), 0, dst);
    }
    return MAKELONG(len, res);
}

/************************************************************************
 *	DoEnvironmentSubst			[SHELL32.53]
 *
 * See DoEnvironmentSubstA.
 */
DWORD WINAPI DoEnvironmentSubstAW(LPVOID x, UINT y)
{
    if (SHELL_OsIsUnicode())
        return DoEnvironmentSubstW(x, y);
    return DoEnvironmentSubstA(x, y);
}

/*************************************************************************
 *      GUIDFromStringA   [SHELL32.703]
 */
BOOL WINAPI GUIDFromStringA(LPCSTR str, LPGUID guid)
{
    TRACE("GUIDFromStringA() stub\n");
    return FALSE;
}

/*************************************************************************
 *      GUIDFromStringW   [SHELL32.704]
 */
BOOL WINAPI GUIDFromStringW(LPCWSTR str, LPGUID guid)
{
    UNICODE_STRING guid_str;

    RtlInitUnicodeString(&guid_str, str);
    return !RtlGUIDFromString(&guid_str, guid);
}

/*************************************************************************
 *      PathIsTemporaryA    [SHELL32.713]
 */
BOOL WINAPI PathIsTemporaryA(LPSTR Str)
{
    FIXME("(%s)stub\n", debugstr_a(Str));
    return FALSE;
}

/*************************************************************************
 *      PathIsTemporaryW    [SHELL32.714]
 */
BOOL WINAPI PathIsTemporaryW(LPWSTR Str)
{
    FIXME("(%s)stub\n", debugstr_w(Str));
    return FALSE;
}

typedef struct _PSXA
{
    UINT uiCount;
    UINT uiAllocated;
    IShellPropSheetExt *pspsx[1];
} PSXA, *PPSXA;

typedef struct _PSXA_CALL
{
    LPFNADDPROPSHEETPAGE lpfnAddReplaceWith;
    LPARAM lParam;
    BOOL bCalled;
    BOOL bMultiple;
    UINT uiCount;
} PSXA_CALL, *PPSXA_CALL;

static BOOL CALLBACK PsxaCall(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PPSXA_CALL Call = (PPSXA_CALL)lParam;

    if (Call != NULL)
    {
        if ((Call->bMultiple || !Call->bCalled) &&
            Call->lpfnAddReplaceWith(hpage, Call->lParam))
        {
            Call->bCalled = TRUE;
            Call->uiCount++;
            return TRUE;
        }
    }

    return FALSE;
}

/*************************************************************************
 *      SHAddFromPropSheetExtArray	[SHELL32.167]
 */
UINT WINAPI SHAddFromPropSheetExtArray(HPSXA hpsxa, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    PSXA_CALL Call;
    UINT i;
    PPSXA psxa = (PPSXA)hpsxa;

    TRACE("(%p,%p,%08lx)\n", hpsxa, lpfnAddPage, lParam);

    if (psxa)
    {
        ZeroMemory(&Call, sizeof(Call));
        Call.lpfnAddReplaceWith = lpfnAddPage;
        Call.lParam = lParam;
        Call.bMultiple = TRUE;

        /* Call the AddPage method of all registered IShellPropSheetExt interfaces */
        for (i = 0; i != psxa->uiCount; i++)
        {
            psxa->pspsx[i]->lpVtbl->AddPages(psxa->pspsx[i], PsxaCall, (LPARAM)&Call);
        }

        return Call.uiCount;
    }

    return 0;
}

/*************************************************************************
 *      SHCreatePropSheetExtArray	[SHELL32.168]
 */
HPSXA WINAPI SHCreatePropSheetExtArray(HKEY hKey, LPCWSTR pszSubKey, UINT max_iface)
{
    return SHCreatePropSheetExtArrayEx(hKey, pszSubKey, max_iface, NULL);
}

/*************************************************************************
 *      SHCreatePropSheetExtArrayEx	[SHELL32.194]
 */
HPSXA WINAPI SHCreatePropSheetExtArrayEx(HKEY hKey, LPCWSTR pszSubKey, UINT max_iface, LPDATAOBJECT pDataObj)
{
    static const WCHAR szPropSheetSubKey[] = {'s','h','e','l','l','e','x','\\','P','r','o','p','e','r','t','y','S','h','e','e','t','H','a','n','d','l','e','r','s',0};
    WCHAR szHandler[64];
    DWORD dwHandlerLen;
    WCHAR szClsidHandler[39];
    DWORD dwClsidSize;
    CLSID clsid;
    LONG lRet;
    DWORD dwIndex;
    IShellExtInit *psxi;
    IShellPropSheetExt *pspsx;
    HKEY hkBase, hkPropSheetHandlers;
    PPSXA psxa = NULL;

    TRACE("(%p,%s,%u)\n", hKey, debugstr_w(pszSubKey), max_iface);

    if (max_iface == 0)
        return NULL;

    /* Open the registry key */
    lRet = RegOpenKeyW(hKey, pszSubKey, &hkBase);
    if (lRet != ERROR_SUCCESS)
        return NULL;

    lRet = RegOpenKeyExW(hkBase, szPropSheetSubKey, 0, KEY_ENUMERATE_SUB_KEYS, &hkPropSheetHandlers);
    RegCloseKey(hkBase);
    if (lRet == ERROR_SUCCESS)
    {
        /* Create and initialize the Property Sheet Extensions Array */
        psxa = LocalAlloc(LMEM_FIXED, FIELD_OFFSET(PSXA, pspsx[max_iface]));
        if (psxa)
        {
            ZeroMemory(psxa, FIELD_OFFSET(PSXA, pspsx[max_iface]));
            psxa->uiAllocated = max_iface;

            /* Enumerate all subkeys and attempt to load the shell extensions */
            dwIndex = 0;
            do
            {
                dwHandlerLen = sizeof(szHandler) / sizeof(szHandler[0]);
                lRet = RegEnumKeyExW(hkPropSheetHandlers, dwIndex++, szHandler, &dwHandlerLen, NULL, NULL, NULL, NULL);
                if (lRet != ERROR_SUCCESS)
                {
                    if (lRet == ERROR_MORE_DATA)
                        continue;

                    if (lRet == ERROR_NO_MORE_ITEMS)
                        lRet = ERROR_SUCCESS;
                    break;
                }

                /* The CLSID is stored either in the key itself or in its default value. */
                if (FAILED(lRet = SHCLSIDFromStringW(szHandler, &clsid)))
                {
                    dwClsidSize = sizeof(szClsidHandler);
                    if (SHGetValueW(hkPropSheetHandlers, szHandler, NULL, NULL, szClsidHandler, &dwClsidSize) == ERROR_SUCCESS)
                    {
                        /* Force a NULL-termination and convert the string */
                        szClsidHandler[(sizeof(szClsidHandler) / sizeof(szClsidHandler[0])) - 1] = 0;
                        lRet = SHCLSIDFromStringW(szClsidHandler, &clsid);
                    }
                }

                if (SUCCEEDED(lRet))
                {
                    /* Attempt to get an IShellPropSheetExt and an IShellExtInit instance.
                       Only if both interfaces are supported it's a real shell extension.
                       Then call IShellExtInit's Initialize method. */
                    if (SUCCEEDED(CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER/* | CLSCTX_NO_CODE_DOWNLOAD */, &IID_IShellPropSheetExt, (LPVOID *)&pspsx)))
                    {
                        if (SUCCEEDED(pspsx->lpVtbl->QueryInterface(pspsx, &IID_IShellExtInit, (PVOID *)&psxi)))
                        {
                            if (SUCCEEDED(psxi->lpVtbl->Initialize(psxi, NULL, pDataObj, hKey)))
                            {
                                /* Add the IShellPropSheetExt instance to the array */
                                psxa->pspsx[psxa->uiCount++] = pspsx;
                            }
                            else
                            {
                                psxi->lpVtbl->Release(psxi);
                                pspsx->lpVtbl->Release(pspsx);
                            }
                        }
                        else
                            pspsx->lpVtbl->Release(pspsx);
                    }
                }

            } while (psxa->uiCount != psxa->uiAllocated);
        }
        else
            lRet = ERROR_NOT_ENOUGH_MEMORY;

        RegCloseKey(hkPropSheetHandlers);
    }

    if (lRet != ERROR_SUCCESS && psxa)
    {
        SHDestroyPropSheetExtArray((HPSXA)psxa);
        psxa = NULL;
    }

    return (HPSXA)psxa;
}

/*************************************************************************
 *      SHReplaceFromPropSheetExtArray	[SHELL32.170]
 */
UINT WINAPI SHReplaceFromPropSheetExtArray(HPSXA hpsxa, UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    PSXA_CALL Call;
    UINT i;
    PPSXA psxa = (PPSXA)hpsxa;

    TRACE("(%p,%u,%p,%08lx)\n", hpsxa, uPageID, lpfnReplaceWith, lParam);

    if (psxa)
    {
        ZeroMemory(&Call, sizeof(Call));
        Call.lpfnAddReplaceWith = lpfnReplaceWith;
        Call.lParam = lParam;

        /* Call the ReplacePage method of all registered IShellPropSheetExt interfaces.
           Each shell extension is only allowed to call the callback once during the callback. */
        for (i = 0; i != psxa->uiCount; i++)
        {
            Call.bCalled = FALSE;
            psxa->pspsx[i]->lpVtbl->ReplacePage(psxa->pspsx[i], uPageID, PsxaCall, (LPARAM)&Call);
        }

        return Call.uiCount;
    }

    return 0;
}

/*************************************************************************
 *      SHDestroyPropSheetExtArray	[SHELL32.169]
 */
void WINAPI SHDestroyPropSheetExtArray(HPSXA hpsxa)
{
    UINT i;
    PPSXA psxa = (PPSXA)hpsxa;

    TRACE("(%p)\n", hpsxa);

    if (psxa)
    {
        for (i = 0; i != psxa->uiCount; i++)
        {
            psxa->pspsx[i]->lpVtbl->Release(psxa->pspsx[i]);
        }

        LocalFree(psxa);
    }
}

/*************************************************************************
 *      CIDLData_CreateFromIDArray	[SHELL32.83]
 *
 *  Create IDataObject from PIDLs??
 */
HRESULT WINAPI CIDLData_CreateFromIDArray(
	LPCITEMIDLIST pidlFolder,
    UINT cpidlFiles,
	LPCITEMIDLIST *lppidlFiles,
	LPDATAOBJECT *ppdataObject)
{
    UINT i;
    HWND hwnd = 0;   /*FIXME: who should be hwnd of owner? set to desktop */
    HRESULT hResult;

    TRACE("(%p, %d, %p, %p)\n", pidlFolder, cpidlFiles, lppidlFiles, ppdataObject);
    if (TRACE_ON(pidl))
    {
	pdump (pidlFolder);
	for (i=0; i<cpidlFiles; i++) pdump (lppidlFiles[i]);
    }
    hResult = IDataObject_Constructor(hwnd, pidlFolder, lppidlFiles, cpidlFiles, ppdataObject);
    return hResult;
}

/*************************************************************************
 * SHCreateStdEnumFmtEtc			[SHELL32.74]
 *
 * NOTES
 *
 */
HRESULT WINAPI SHCreateStdEnumFmtEtc(
    UINT cFormats,
	const FORMATETC *lpFormats,
	LPENUMFORMATETC *ppenumFormatetc)
{
	IEnumFORMATETC *pef;
	HRESULT hRes;
	TRACE("cf=%d fe=%p pef=%p\n", cFormats, lpFormats, ppenumFormatetc);

    hRes = IEnumFORMATETC_Constructor(cFormats, lpFormats, &pef);
    if (FAILED(hRes))
        return hRes;

	IEnumFORMATETC_AddRef(pef);
	hRes = IEnumFORMATETC_QueryInterface(pef, &IID_IEnumFORMATETC, (LPVOID*)ppenumFormatetc);
	IEnumFORMATETC_Release(pef);

	return hRes;
}

/*************************************************************************
 *		SHFindFiles (SHELL32.90)
 */
BOOL WINAPI SHFindFiles( LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlSaveFile )
{
    FIXME("%p %p\n", pidlFolder, pidlSaveFile );
    return FALSE;
}

/*************************************************************************
 *		SHUpdateImageW (SHELL32.192)
 *
 * Notifies the shell that an icon in the system image list has been changed.
 *
 * PARAMS
 *  pszHashItem [I] Path to file that contains the icon.
 *  iIndex      [I] Zero-based index of the icon in the file.
 *  uFlags      [I] Flags determining the icon attributes. See notes.
 *  iImageIndex [I] Index of the icon in the system image list.
 *
 * RETURNS
 *  Nothing
 *
 * NOTES
 *  uFlags can be one or more of the following flags:
 *  GIL_NOTFILENAME - pszHashItem is not a file name.
 *  GIL_SIMULATEDOC - Create a document icon using the specified icon.
 */
void WINAPI SHUpdateImageW(LPCWSTR pszHashItem, int iIndex, UINT uFlags, int iImageIndex)
{
    FIXME("%s, %d, 0x%x, %d - stub\n", debugstr_w(pszHashItem), iIndex, uFlags, iImageIndex);
}

/*************************************************************************
 *		SHUpdateImageA (SHELL32.191)
 *
 * See SHUpdateImageW.
 */
VOID WINAPI SHUpdateImageA(LPCSTR pszHashItem, INT iIndex, UINT uFlags, INT iImageIndex)
{
    FIXME("%s, %d, 0x%x, %d - stub\n", debugstr_a(pszHashItem), iIndex, uFlags, iImageIndex);
}

INT WINAPI SHHandleUpdateImage(LPCITEMIDLIST pidlExtra)
{
    FIXME("%p - stub\n", pidlExtra);

    return -1;
}

BOOL WINAPI SHObjectProperties(HWND hwnd, DWORD dwType, LPCWSTR szObject, LPCWSTR szPage)
{
    FIXME("%p, 0x%08x, %s, %s - stub\n", hwnd, dwType, debugstr_w(szObject), debugstr_w(szPage));

    return TRUE;
}

BOOL WINAPI SHGetNewLinkInfoA(LPCSTR pszLinkTo, LPCSTR pszDir, LPSTR pszName, BOOL *pfMustCopy,
                              UINT uFlags)
{
    WCHAR wszLinkTo[MAX_PATH];
    WCHAR wszDir[MAX_PATH];
    WCHAR wszName[MAX_PATH];
    BOOL res;

    MultiByteToWideChar(CP_ACP, 0, pszLinkTo, -1, wszLinkTo, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, pszDir, -1, wszDir, MAX_PATH);

    res = SHGetNewLinkInfoW(wszLinkTo, wszDir, wszName, pfMustCopy, uFlags);

    if (res)
        WideCharToMultiByte(CP_ACP, 0, wszName, -1, pszName, MAX_PATH, NULL, NULL);

    return res;
}

BOOL WINAPI SHGetNewLinkInfoW(LPCWSTR pszLinkTo, LPCWSTR pszDir, LPWSTR pszName, BOOL *pfMustCopy,
                              UINT uFlags)
{
    const WCHAR *basename;
    WCHAR *dst_basename;
    int i=2;
    static const WCHAR lnkformat[] = {'%','s','.','l','n','k',0};
    static const WCHAR lnkformatnum[] = {'%','s',' ','(','%','d',')','.','l','n','k',0};

    TRACE("(%s, %s, %p, %p, 0x%08x)\n", debugstr_w(pszLinkTo), debugstr_w(pszDir),
          pszName, pfMustCopy, uFlags);

    *pfMustCopy = FALSE;

    if (uFlags & SHGNLI_PIDL)
    {
        FIXME("SHGNLI_PIDL flag unsupported\n");
        return FALSE;
    }

    if (uFlags)
        FIXME("ignoring flags: 0x%08x\n", uFlags);

    /* FIXME: should test if the file is a shortcut or DOS program */
    if (GetFileAttributesW(pszLinkTo) == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    basename = strrchrW(pszLinkTo, '\\');
    if (basename)
        basename = basename+1;
    else
        basename = pszLinkTo;

    lstrcpynW(pszName, pszDir, MAX_PATH);
    if (!PathAddBackslashW(pszName))
        return FALSE;

    dst_basename = pszName + strlenW(pszName);

    snprintfW(dst_basename, pszName + MAX_PATH - dst_basename, lnkformat, basename);

    while (GetFileAttributesW(pszName) != INVALID_FILE_ATTRIBUTES)
    {
        snprintfW(dst_basename, pszName + MAX_PATH - dst_basename, lnkformatnum, basename, i);
        i++;
    }

    return TRUE;
}

HRESULT WINAPI SHStartNetConnectionDialog(HWND hwnd, LPCSTR pszRemoteName, DWORD dwType)
{
    FIXME("%p, %s, 0x%08x - stub\n", hwnd, debugstr_a(pszRemoteName), dwType);

    return S_OK;
}
/*************************************************************************
 *              SHSetLocalizedName (SHELL32.@)
 */
HRESULT WINAPI SHSetLocalizedName(LPCWSTR pszPath, LPCWSTR pszResModule, int idsRes)
{
    FIXME("%p, %s, %d - stub\n", pszPath, debugstr_w(pszResModule), idsRes);

    return S_OK;
}

/*************************************************************************
 *              LinkWindow_RegisterClass (SHELL32.258)
 */
BOOL WINAPI LinkWindow_RegisterClass(void)
{
    FIXME("()\n");
    return TRUE;
}

/*************************************************************************
 *              LinkWindow_UnregisterClass (SHELL32.259)
 */
BOOL WINAPI LinkWindow_UnregisterClass(DWORD dwUnused)
{
    FIXME("()\n");
    return TRUE;
}

/*************************************************************************
 *              SHFlushSFCache (SHELL32.526)
 *
 * Notifies the shell that a user-specified special folder location has changed.
 *
 * NOTES
 *   In Wine, the shell folder registry values are not cached, so this function
 *   has no effect.
 */
void WINAPI SHFlushSFCache(void)
{
}

/*************************************************************************
 *              SHGetImageList (SHELL32.727)
 *
 * Returns a copy of a shell image list.
 *
 * NOTES
 *   Windows XP features 4 sizes of image list, and Vista 5. Wine currently
 *   only supports the traditional small and large image lists, so requests
 *   for the others will currently fail.
 */
HRESULT WINAPI SHGetImageList(int iImageList, REFIID riid, void **ppv)
{
    HIMAGELIST hLarge, hSmall;
    HIMAGELIST hNew;
    HRESULT ret = E_FAIL;

    /* Wine currently only maintains large and small image lists */
    if ((iImageList != SHIL_LARGE) && (iImageList != SHIL_SMALL) && (iImageList != SHIL_SYSSMALL))
    {
        FIXME("Unsupported image list %i requested\n", iImageList);
        return E_FAIL;
    }

    Shell_GetImageLists(&hLarge, &hSmall);
#ifndef __REACTOS__
    hNew = ImageList_Duplicate(iImageList == SHIL_LARGE ? hLarge : hSmall);

    /* Get the interface for the new image list */
    if (hNew)
    {
        ret = HIMAGELIST_QueryInterface(hNew, riid, ppv);
        ImageList_Destroy(hNew);
    }
#else
    /* Duplicating the imagelist causes the start menu items not to draw on
     * the first show. Was the Duplicate necessary for some reason? I believe
     * Windows returns the raw pointer here. */
    hNew = (iImageList == SHIL_LARGE ? hLarge : hSmall);
    ret = IImageList2_QueryInterface((IImageList2 *) hNew, riid, ppv);
#endif

    return ret;
}

/*************************************************************************
 * SHCreateShellFolderView			[SHELL32.256]
 *
 * Create a new instance of the default Shell folder view object.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: error value
 *
 * NOTES
 *  see IShellFolder::CreateViewObject
 */
HRESULT WINAPI SHCreateShellFolderView(const SFV_CREATE *pcsfv,
                        IShellView **ppsv)
{
	IShellView * psf;
	HRESULT hRes;

	*ppsv = NULL;
	if (!pcsfv || pcsfv->cbSize != sizeof(*pcsfv))
	  return E_INVALIDARG;

	TRACE("sf=%p outer=%p callback=%p\n",
	  pcsfv->pshf, pcsfv->psvOuter, pcsfv->psfvcb);

    hRes = IShellView_Constructor(pcsfv->pshf, &psf);
    if (FAILED(hRes))
        return hRes;

	hRes = IShellView_QueryInterface(psf, &IID_IShellView, (LPVOID *)ppsv);
	IShellView_Release(psf);

	return hRes;
}
