/*
 * SHFileOperation
 *
 * Copyright 2000 Juergen Schmied
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2002 Dietrich Teickner (from Odin)
 * Copyright 2002 Rolf Kalbermatter
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

#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shellapi.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "shresdef.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "shell32_main.h"
#include "undocshell.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define IsAttribFile(x) (!(x == -1) && !(x & FILE_ATTRIBUTE_DIRECTORY))
#define IsAttribDir(x)  (!(x == -1) && (x & FILE_ATTRIBUTE_DIRECTORY))

#define IsDotDir(x)     ((x[0] == '.') && ((x[1] == 0) || ((x[1] == '.') && (x[2] == 0))))

#define FO_MASK         0xF

CHAR aWildcardFile[] = {'*','.','*',0};
WCHAR wWildcardFile[] = {'*','.','*',0};
WCHAR wWildcardChars[] = {'*','?',0};
WCHAR wBackslash[] = {'\\',0};

static DWORD SHNotifyCreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES sec);
static DWORD SHNotifyCreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec);
static DWORD SHNotifyRemoveDirectoryA(LPCSTR path);
static DWORD SHNotifyRemoveDirectoryW(LPCWSTR path);
static DWORD SHNotifyDeleteFileA(LPCSTR path);
static DWORD SHNotifyDeleteFileW(LPCWSTR path);
static DWORD SHNotifyMoveFileW(LPCWSTR src, LPCWSTR dest, BOOL bRenameIfExists);
static DWORD SHNotifyCopyFileW(LPCWSTR src, LPCWSTR dest, BOOL bRenameIfExists);

typedef struct
{
	UINT caption_resource_id, text_resource_id;
} SHELL_ConfirmIDstruc;

static BOOL SHELL_ConfirmIDs(int nKindOfDialog, SHELL_ConfirmIDstruc *ids)
{
	switch (nKindOfDialog) {
	  case ASK_DELETE_FILE:
	    ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
	    ids->text_resource_id  = IDS_DELETEITEM_TEXT;
	    return TRUE;
	  case ASK_DELETE_FOLDER:
	    ids->caption_resource_id  = IDS_DELETEFOLDER_CAPTION;
	    ids->text_resource_id  = IDS_DELETEITEM_TEXT;
	    return TRUE;
	  case ASK_DELETE_MULTIPLE_ITEM:
	    ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
	    ids->text_resource_id  = IDS_DELETEMULTIPLE_TEXT;
	    return TRUE;
	  case ASK_OVERWRITE_FILE:
	    ids->caption_resource_id  = IDS_OVERWRITEFILE_CAPTION;
	    ids->text_resource_id  = IDS_OVERWRITEFILE_TEXT;
	    return TRUE;
	  default:
	    FIXME(" Unhandled nKindOfDialog %d stub\n", nKindOfDialog);
	}
	return FALSE;
}

BOOL SHELL_ConfirmDialog(int nKindOfDialog, LPCSTR szDir)
{
	CHAR szCaption[255], szText[255], szBuffer[MAX_PATH + 256];
	SHELL_ConfirmIDstruc ids;

	if (!SHELL_ConfirmIDs(nKindOfDialog, &ids))
	  return FALSE;

	LoadStringA(shell32_hInstance, ids.caption_resource_id, szCaption, sizeof(szCaption));
	LoadStringA(shell32_hInstance, ids.text_resource_id, szText, sizeof(szText));

	FormatMessageA(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
	               szText, 0, 0, szBuffer, sizeof(szBuffer), (va_list*)&szDir);

	return (IDOK == MessageBoxA(GetActiveWindow(), szBuffer, szCaption, MB_OKCANCEL | MB_ICONEXCLAMATION));
}

BOOL SHELL_ConfirmDialogW(int nKindOfDialog, LPCWSTR szDir)
{
	WCHAR szCaption[255], szText[255], szBuffer[MAX_PATH + 256];
	SHELL_ConfirmIDstruc ids;

	if (!SHELL_ConfirmIDs(nKindOfDialog, &ids))
	  return FALSE;

	LoadStringW(shell32_hInstance, ids.caption_resource_id, szCaption, sizeof(szCaption));
	LoadStringW(shell32_hInstance, ids.text_resource_id, szText, sizeof(szText));

	FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
	               szText, 0, 0, szBuffer, sizeof(szBuffer), (va_list*)&szDir);

	return (IDOK == MessageBoxW(GetActiveWindow(), szBuffer, szCaption, MB_OKCANCEL | MB_ICONEXCLAMATION));
}

/**************************************************************************
 * SHELL_DeleteDirectoryA()  [internal]
 *
 * like rm -r
 */
BOOL SHELL_DeleteDirectoryA(LPCSTR pszDir, BOOL bShowUI)
{
	BOOL    ret = TRUE;
	HANDLE  hFind;
	WIN32_FIND_DATAA wfd;
	char    szTemp[MAX_PATH];

	/* Make sure the directory exists before eventually prompting the user */
	PathCombineA(szTemp, pszDir, aWildcardFile);
	hFind = FindFirstFileA(szTemp, &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
	  return FALSE;

	if (!bShowUI || SHELL_ConfirmDialog(ASK_DELETE_FOLDER, pszDir))
	{
	  do
	  {
	    LPSTR lp = wfd.cAlternateFileName;
	    if (!lp[0])
	      lp = wfd.cFileName;
	    if (IsDotDir(lp))
	      continue;
	    PathCombineA(szTemp, pszDir, lp);
	    if (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
	      ret = SHELL_DeleteDirectoryA(szTemp, FALSE);
	    else
	      ret = (SHNotifyDeleteFileA(szTemp) == ERROR_SUCCESS);
	  } while (ret && FindNextFileA(hFind, &wfd));
	}
	FindClose(hFind);
	if (ret)
	  ret = (SHNotifyRemoveDirectoryA(pszDir) == ERROR_SUCCESS);
	return ret;
}

BOOL SHELL_DeleteDirectoryW(LPCWSTR pszDir, BOOL bShowUI)
{
	BOOL    ret = TRUE;
	HANDLE  hFind;
	WIN32_FIND_DATAW wfd;
	WCHAR   szTemp[MAX_PATH];

	/* Make sure the directory exists before eventually prompting the user */
	PathCombineW(szTemp, pszDir, wWildcardFile);
	hFind = FindFirstFileW(szTemp, &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
	  return FALSE;

	if (!bShowUI || SHELL_ConfirmDialogW(ASK_DELETE_FOLDER, pszDir))
	{
	  do
	  {
	    LPWSTR lp = wfd.cAlternateFileName;
	    if (!lp[0])
	      lp = wfd.cFileName;
	    if (IsDotDir(lp))
	      continue;
	    PathCombineW(szTemp, pszDir, lp);
	    if (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
	      ret = SHELL_DeleteDirectoryW(szTemp, FALSE);
	    else
	      ret = (SHNotifyDeleteFileW(szTemp) == ERROR_SUCCESS);
	  } while (ret && FindNextFileW(hFind, &wfd));
	}
	FindClose(hFind);
	if (ret)
	  ret = (SHNotifyRemoveDirectoryW(pszDir) == ERROR_SUCCESS);
	return ret;
}

/**************************************************************************
 *  SHELL_DeleteFileA()      [internal]
 */
BOOL SHELL_DeleteFileA(LPCSTR pszFile, BOOL bShowUI)
{
	if (bShowUI && !SHELL_ConfirmDialog(ASK_DELETE_FILE, pszFile))
	  return FALSE;

	return (SHNotifyDeleteFileA(pszFile) == ERROR_SUCCESS);
}

BOOL SHELL_DeleteFileW(LPCWSTR pszFile, BOOL bShowUI)
{
	if (bShowUI && !SHELL_ConfirmDialogW(ASK_DELETE_FILE, pszFile))
	  return FALSE;

	return (SHNotifyDeleteFileW(pszFile) == ERROR_SUCCESS);
}

/**************************************************************************
 * Win32CreateDirectory      [SHELL32.93]
 *
 * Creates a directory. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  path       [I]   path to directory to create
 *
 * RETURNS
 *  TRUE if successful, FALSE otherwise
 *
 * NOTES:
 *  Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be ANSI.
 *  This is Unicode on NT/2000
 */
static DWORD SHNotifyCreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES sec)
{
	WCHAR wPath[MAX_PATH];
	TRACE("(%s, %p)\n", debugstr_a(path), sec);

	MultiByteToWideChar(CP_ACP, 0, path, -1, wPath, MAX_PATH);
	return SHNotifyCreateDirectoryW(wPath, sec);
}

static DWORD SHNotifyCreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec)
{
	TRACE("(%s, %p)\n", debugstr_w(path), sec);

	if (StrPBrkW(path, wWildcardChars))
	{
	  /* FIXME: This test is currently necessary since our CreateDirectory
	     implementation does create directories with wildcard characters
	     without objection!! Once this is fixed, this here can go away. */
	  SetLastError(ERROR_INVALID_NAME);
#ifdef W98_FO_FUNCTION /* W98 */
	  return ERROR_FILE_NOT_FOUND;
#else
	  return ERROR_INVALID_NAME;
#endif
	}

	if (CreateDirectoryW(path, sec))
	{
	  SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW, path, NULL);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

BOOL WINAPI Win32CreateDirectoryAW(LPCVOID path, LPSECURITY_ATTRIBUTES sec)
{
	if (SHELL_OsIsUnicode())
	  return (SHNotifyCreateDirectoryW(path, sec) == ERROR_SUCCESS);
	return (SHNotifyCreateDirectoryA(path, sec) == ERROR_SUCCESS);
}

/************************************************************************
 * Win32RemoveDirectory      [SHELL32.94]
 *
 * Deletes a directory. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  path       [I]   path to directory to delete
 *
 * RETURNS
 *  TRUE if successful, FALSE otherwise
 *
 * NOTES:
 *  Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be ANSI.
 *  This is Unicode on NT/2000
 */

static DWORD SHNotifyRemoveDirectoryA(LPCSTR path)
{
	WCHAR wPath[MAX_PATH];
	TRACE("(%s)\n", debugstr_a(path));

	MultiByteToWideChar(CP_ACP, 0, path, -1, wPath, MAX_PATH);
	return SHNotifyRemoveDirectoryW(wPath);
}

/***********************************************************************/

static DWORD SHNotifyRemoveDirectoryW(LPCWSTR path)
{
	BOOL ret;
	TRACE("(%s)\n", debugstr_w(path));

	ret = RemoveDirectoryW(path);
	if (!ret)
	{
	  /* Directory may be write protected */
	  DWORD dwAttr = GetFileAttributesW(path);
	  if (dwAttr != -1 && dwAttr & FILE_ATTRIBUTE_READONLY)
	    if (SetFileAttributesW(path, dwAttr & ~FILE_ATTRIBUTE_READONLY))
	      ret = RemoveDirectoryW(path);
	}
	if (ret)
	{
	  SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW, path, NULL);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/***********************************************************************/

BOOL WINAPI Win32RemoveDirectoryAW(LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return (SHNotifyRemoveDirectoryW(path) == ERROR_SUCCESS);
	return (SHNotifyRemoveDirectoryA(path) == ERROR_SUCCESS);
}

/************************************************************************
 * Win32DeleteFile           [SHELL32.164]
 *
 * Deletes a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  path       [I]   path to file to delete
 *
 * RETURNS
 *  TRUE if successful, FALSE otherwise
 *
 * NOTES:
 *  Verified on Win98 / IE 5 (SHELL32 4.72, March 1999 build) to be ANSI.
 *  This is Unicode on NT/2000
 */

static DWORD SHNotifyDeleteFileA(LPCSTR path)
{
	WCHAR wPath[MAX_PATH];
	TRACE("(%s)\n", debugstr_a(path));

	MultiByteToWideChar(CP_ACP, 0, path, -1, wPath, MAX_PATH);
	return SHNotifyDeleteFileW(wPath);
}

/***********************************************************************/

static DWORD SHNotifyDeleteFileW(LPCWSTR path)
{
	BOOL ret;

	TRACE("(%s)\n", debugstr_w(path));

	ret = DeleteFileW(path);
	if (!ret)
	{
	  /* File may be write protected or a system file */
	  DWORD dwAttr = GetFileAttributesW(path);
	  if ((dwAttr != -1) && (dwAttr & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	    if (SetFileAttributesW(path, dwAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	      ret = DeleteFileW(path);
	}
	if (ret)
	{
	  SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, path, NULL);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/***********************************************************************/

DWORD WINAPI Win32DeleteFileAW(LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return (SHNotifyDeleteFileW(path) == ERROR_SUCCESS);
	return (SHNotifyDeleteFileA(path) == ERROR_SUCCESS);
}

/************************************************************************
 * SHNotifyMoveFile          [internal]
 *
 * Moves a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  src        [I]   path to source file to move
 *  dest       [I]   path to target file to move to
 *  bRename    [I]   if TRUE, the target file will be renamed if a
 *                   file with this name already exists
 *
 * RETURNS
 *  ERORR_SUCCESS if successful
 */
static DWORD SHNotifyMoveFileW(LPCWSTR src, LPCWSTR dest, BOOL bRename)
{
	BOOL ret;

	TRACE("(%s %s %s)\n", debugstr_w(src), debugstr_w(dest), bRename ? "renameIfExists" : "");

	if (StrPBrkW(dest, wWildcardChars))
	{
	  /* FIXME: This test is currently necessary since our MoveFile
	     implementation does create files with wildcard characters
	     without objection!! Once this is fixed, this here can go away. */
	  SetLastError(ERROR_INVALID_NAME);
#ifdef W98_FO_FUNCTION /* W98 */
	  return ERROR_FILE_NOT_FOUND;
#else
	  return ERROR_INVALID_NAME;
#endif
	}

	ret = MoveFileW(src, dest);
	if (!ret)
	{
	  /* Source file may be write protected or a system file */
	  DWORD dwAttr = GetFileAttributesW(src);
	  if ((dwAttr != -1) && (dwAttr & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	    if (SetFileAttributesW(src, dwAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	      ret = MoveFileW(src, dest);

	  if (!ret && bRename)
	  {
	    /* Destination file probably exists */
	    dwAttr = GetFileAttributesW(dest);
	    if (dwAttr != -1)
	    {
	      FIXME("Rename on move to existing file not implemented!\n");
	    }
	  }
	}
	if (ret)
	{
	  SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_PATHW, src, dest);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/************************************************************************
 * SHNotifyCopyFile          [internal]
 *
 * Copies a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  src        [I]   path to source file to move
 *  dest       [I]   path to target file to move to
 *  bRename    [I]   if TRUE, the target file will be renamed if a
 *                   file with this name already exists
 *
 * RETURNS
 *  ERROR_SUCCESS if successful
 */
static DWORD SHNotifyCopyFileW(LPCWSTR src, LPCWSTR dest, BOOL bRename)
{
	BOOL ret;

	TRACE("(%s %s %s)\n", debugstr_w(src), debugstr_w(dest), bRename ? "renameIfExists" : "");

	ret = CopyFileW(src, dest, TRUE);
	if (!ret && bRename)
	{
	  /* Destination file probably exists */
	  DWORD dwAttr = GetFileAttributesW(dest);
	  if (dwAttr != -1)
	  {
	    FIXME("Rename on copy to existing file not implemented!\n");
	  }
	}
	if (ret)
	{
	  SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, dest, NULL);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/*************************************************************************
 * SHCreateDirectory         [SHELL32.165]
 *
 * Create a directory at the specified location
 *
 * PARAMS
 *  hWnd       [I]
 *  path       [I]   path of directory to create
 *
 * RETURNS
 *  ERRROR_SUCCESS or one of the following values:
 *  ERROR_BAD_PATHNAME if the path is relative
 *  ERROR_FILE_EXISTS when a file with that name exists
 *  ERROR_ALREADY_EXISTS when the directory already exists
 *  ERROR_FILENAME_EXCED_RANGE if the filename was to long to process
 *
 * NOTES
 *  exported by ordinal
 *  Win9x exports ANSI
 *  WinNT/2000 exports Unicode
 */
DWORD WINAPI SHCreateDirectory(HWND hWnd, LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return SHCreateDirectoryExW(hWnd, path, NULL);
	return SHCreateDirectoryExA(hWnd, path, NULL);
}

/*************************************************************************
 * SHCreateDirectoryExA      [SHELL32.@]
 *
 * Create a directory at the specified location
 *
 * PARAMS
 *  hWnd       [I]   
 *  path       [I]   path of directory to create 
 *  sec        [I]   security attributes to use or NULL
 *
 * RETURNS
 *  ERRROR_SUCCESS or one of the following values:
 *  ERROR_BAD_PATHNAME if the path is relative
 *  ERORO_INVALID_NAME if the path contains invalid chars
 *  ERROR_FILE_EXISTS when a file with that name exists
 *  ERROR_ALREADY_EXISTS when the directory already exists
 *  ERROR_FILENAME_EXCED_RANGE if the filename was to long to process
 */
DWORD WINAPI SHCreateDirectoryExA(HWND hWnd, LPCSTR path, LPSECURITY_ATTRIBUTES sec)
{
	WCHAR wPath[MAX_PATH];
	TRACE("(%p, %s, %p)\n",hWnd, debugstr_a(path), sec);

	MultiByteToWideChar(CP_ACP, 0, path, -1, wPath, MAX_PATH);
	return SHCreateDirectoryExW(hWnd, wPath, sec);
}

/*************************************************************************
 * SHCreateDirectoryExW      [SHELL32.@]
 */
DWORD WINAPI SHCreateDirectoryExW(HWND hWnd, LPCWSTR path, LPSECURITY_ATTRIBUTES sec)
{
	DWORD ret = ERROR_BAD_PATHNAME;
	TRACE("(%p, %s, %p)\n",hWnd, debugstr_w(path), sec);

	if (PathIsRelativeW(path))
	{
	  SetLastError(ret);
	}
	else
	{
	  ret = SHNotifyCreateDirectoryW(path, sec);
	  if (ret != ERROR_FILE_EXISTS &&
	      ret != ERROR_ALREADY_EXISTS &&
	      ret != ERROR_FILENAME_EXCED_RANGE)
	  {
	  /* handling network file names?
	    lstrcpynW(pathName, path, MAX_PATH);
	    lpStr = PathAddBackslashW(pathName);*/
	    FIXME("Semi-stub, non zero hWnd should be used somehow?\n");
	  }
	}
	return ret;
}

/*************************************************************************
 *
 * SHFileStrICmp HelperFunction for SHFileOperationW
 *
 */
BOOL SHFileStrICmpW(LPWSTR p1, LPWSTR p2, LPWSTR p1End, LPWSTR p2End)
{
	WCHAR C1 = '\0';
	WCHAR C2 = '\0';
	int i_Temp = -1;
	int i_len1 = lstrlenW(p1);
	int i_len2 = lstrlenW(p2);

	if (p1End && (&p1[i_len1] >= p1End) && ('\\' == p1End[0]))
	{
	  C1 = p1End[0];
	  p1End[0] = '\0';
	  i_len1 = lstrlenW(p1);
	}
	if (p2End)
	{
	  if ((&p2[i_len2] >= p2End) && ('\\' == p2End[0]))
	  {
	    C2 = p2End[0];
	    if (C2)
	      p2End[0] = '\0';
	  }
	}
	else
	{
	  if ((i_len1 <= i_len2) && ('\\' == p2[i_len1]))
	  {
	    C2 = p2[i_len1];
	    if (C2)
	      p2[i_len1] = '\0';
	  }
	}
	i_len2 = lstrlenW(p2);
	if (i_len1 == i_len2)
	  i_Temp = lstrcmpiW(p1,p2);
	if (C1)
	  p1[i_len1] = C1;
	if (C2)
	  p2[i_len2] = C2;
	return !(i_Temp);
}

/*************************************************************************
 *
 * SHFileStrCpyCat HelperFunction for SHFileOperationW
 *
 */
LPWSTR SHFileStrCpyCatW(LPWSTR pTo, LPCWSTR pFrom, LPCWSTR pCatStr)
{
	LPWSTR pToFile = NULL;
	int  i_len;
	if (pTo)
	{
	  if (pFrom)
	    lstrcpyW(pTo, pFrom);
	  if (pCatStr)
	  {
	    i_len = lstrlenW(pTo);
	    if ((i_len) && (pTo[--i_len] != '\\'))
	      i_len++;
	    pTo[i_len] = '\\';
	    if (pCatStr[0] == '\\')
	      pCatStr++; \
	    lstrcpyW(&pTo[i_len+1], pCatStr);
	  }
	  pToFile = StrRChrW(pTo,NULL,'\\');
	  /* termination of the new string-group */
	  pTo[(lstrlenW(pTo)) + 1] = '\0';
	}
	return pToFile;
}

/**************************************************************************
 *	SHELL_FileNamesMatch()
 *
 * Accepts two \0 delimited lists of the file names. Checks whether number of
 * files in both lists is the same, and checks also if source-name exists.
 */
BOOL SHELL_FileNamesMatch(LPCWSTR pszFiles1, LPCWSTR pszFiles2, BOOL bOnlySrc)
{
	while ((pszFiles1[0] != '\0') &&
	       (bOnlySrc || (pszFiles2[0] != '\0')))
	{
	  if (NULL == StrPBrkW(pszFiles1, wWildcardChars))
	  {
	    if (-1 == GetFileAttributesW(pszFiles1))
	      return FALSE;
	  }
	  pszFiles1 += lstrlenW(pszFiles1) + 1;
	  if (!bOnlySrc)
	    pszFiles2 += lstrlenW(pszFiles2) + 1;
	}
	return ((pszFiles1[0] == '\0') && (bOnlySrc || (pszFiles2[0] == '\0')));
}

/*************************************************************************
 *
 * SHNameTranslate HelperFunction for SHFileOperationA
 *
 * Translates a list of 0 terminated ASCII strings into Unicode. If *wString
 * is NULL, only the necessary size of the string is determined and returned,
 * otherwise the ASCII strings are copied into it and the buffer is increased
 * to point to the location after the final 0 termination char.
 */
DWORD SHNameTranslate(LPWSTR* wString, LPCWSTR* pWToFrom, BOOL more)
{
	DWORD size = 0, aSize = 0;
	LPCSTR aString = (LPCSTR)*pWToFrom;

	if (aString)
	{
	  do
	  {
	    size = lstrlenA(aString) + 1;
	    aSize += size;
	    aString += size;
	  } while ((size != 1) && more);
	  /* The two sizes might be different in the case of multibyte chars */
	  size = MultiByteToWideChar(CP_ACP, 0, aString, aSize, *wString, 0);
	  if (*wString) /* only in the second loop */
	  {
	    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)*pWToFrom, aSize, *wString, size);
	    *pWToFrom = *wString;
	    *wString += size;
	  }
	}
	return size;
}
/*************************************************************************
 * SHFileOperationA          [SHELL32.@]
 *
 * Function to copy, move, delete and create one or more files with optional
 * user prompts.
 *
 * PARAMS
 *  lpFileOp   [I/O] pointer to a structure containing all the necessary information
 *
 * NOTES
 *  exported by name
 */
int WINAPI SHFileOperationA(LPSHFILEOPSTRUCTA lpFileOp)
{
	SHFILEOPSTRUCTW nFileOp = *((LPSHFILEOPSTRUCTW)lpFileOp);
	int retCode = 0;
	DWORD size;
	LPWSTR ForFree = NULL, /* we change wString in SHNameTranslate and can't use it for freeing */
	       wString = NULL; /* we change this in SHNameTranslate */

	TRACE("\n");
	if (FO_DELETE == (nFileOp.wFunc & FO_MASK))
	  nFileOp.pTo = NULL; /* we need a NULL or a valid pointer for translation */
	if (!(nFileOp.fFlags & FOF_SIMPLEPROGRESS))
	  nFileOp.lpszProgressTitle = NULL; /* we need a NULL or a valid pointer for translation */
	while (1) /* every loop calculate size, second translate also, if we have storage for this */
	{
	  size = SHNameTranslate(&wString, &nFileOp.lpszProgressTitle, FALSE); /* no loop */
	  size += SHNameTranslate(&wString, &nFileOp.pFrom, TRUE); /* internal loop */
	  size += SHNameTranslate(&wString, &nFileOp.pTo, TRUE); /* internal loop */

	  if (ForFree)
	  {
	    retCode = SHFileOperationW(&nFileOp);
	    HeapFree(GetProcessHeap(), 0, ForFree); /* we can not use wString, it was changed */
	    break;
	  }
	  else
	  {
	    wString = ForFree = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
	    if (ForFree) continue;
	    retCode = ERROR_OUTOFMEMORY;
	    nFileOp.fAnyOperationsAborted = TRUE;
	    SetLastError(retCode);
	    return retCode;
	  }
	}

	lpFileOp->hNameMappings = nFileOp.hNameMappings;
	lpFileOp->fAnyOperationsAborted = nFileOp.fAnyOperationsAborted;
	return retCode;
}

static const char * debug_shfileops_flags( DWORD fFlags )
{
    return wine_dbg_sprintf( "%s%s%s%s%s%s%s%s%s%s%s%s%s",
	fFlags & FOF_MULTIDESTFILES ? "FOF_MULTIDESTFILES " : "",
	fFlags & FOF_CONFIRMMOUSE ? "FOF_CONFIRMMOUSE " : "",
	fFlags & FOF_SILENT ? "FOF_SILENT " : "",
	fFlags & FOF_RENAMEONCOLLISION ? "FOF_RENAMEONCOLLISION " : "",
	fFlags & FOF_NOCONFIRMATION ? "FOF_NOCONFIRMATION " : "",
	fFlags & FOF_WANTMAPPINGHANDLE ? "FOF_WANTMAPPINGHANDLE " : "",
	fFlags & FOF_ALLOWUNDO ? "FOF_ALLOWUNDO " : "",
	fFlags & FOF_FILESONLY ? "FOF_FILESONLY " : "",
	fFlags & FOF_SIMPLEPROGRESS ? "FOF_SIMPLEPROGRESS " : "",
	fFlags & FOF_NOCONFIRMMKDIR ? "FOF_NOCONFIRMMKDIR " : "",
	fFlags & FOF_NOERRORUI ? "FOF_NOERRORUI " : "",
	fFlags & FOF_NOCOPYSECURITYATTRIBS ? "FOF_NOCOPYSECURITYATTRIBS" : "",
	fFlags & 0xf000 ? "MORE-UNKNOWN-Flags" : "");
}

static const char * debug_shfileops_action( DWORD op )
{
    LPCSTR cFO_Name [] = {"FO_????","FO_MOVE","FO_COPY","FO_DELETE","FO_RENAME"};
    return wine_dbg_sprintf("%s", cFO_Name[ op ]);
}

/*************************************************************************
 * SHFileOperationW          [SHELL32.@]
 *
 * See SHFileOperationA
 */
int WINAPI SHFileOperationW(LPSHFILEOPSTRUCTW lpFileOp)
{
	SHFILEOPSTRUCTW nFileOp = *(lpFileOp);

	LPCWSTR pNextFrom = nFileOp.pFrom;
	LPCWSTR pNextTo = nFileOp.pTo;
	LPCWSTR pFrom = pNextFrom;
	LPCWSTR pTo = NULL;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATAW wfd;
	LPWSTR pTempFrom = NULL;
	LPWSTR pTempTo = NULL;
	LPWSTR pFromFile;
	LPWSTR pToFile = NULL;
	LPWSTR lpFileName;
	int retCode = 0;
	DWORD ToAttr;
	DWORD ToPathAttr;
	DWORD FromPathAttr;
	FILEOP_FLAGS OFl = ((FILEOP_FLAGS)lpFileOp->fFlags & 0xfff);

	BOOL b_Multi = (nFileOp.fFlags & FOF_MULTIDESTFILES);

	BOOL b_MultiTo = (FO_DELETE != (lpFileOp->wFunc & FO_MASK));
	BOOL b_MultiPaired = (!b_MultiTo);
	BOOL b_MultiFrom = FALSE;
	BOOL not_overwrite;
	BOOL ask_overwrite;
	BOOL b_SameRoot;
	BOOL b_SameTailName;
	BOOL b_ToInvalidTail = FALSE;
	BOOL b_ToValid; /* for W98-Bug for FO_MOVE with source and target in same rootdrive */
	BOOL b_Mask;
	BOOL b_ToTailSlash = FALSE;

	long FuncSwitch = (nFileOp.wFunc & FO_MASK);
	long level= nFileOp.wFunc>>4;

	/*  default no error */
	nFileOp.fAnyOperationsAborted = FALSE;

	if ((FuncSwitch < FO_MOVE) || (FuncSwitch > FO_RENAME))
	  goto shfileop_normal; /* no valid FunctionCode */

	if (level == 0)
            TRACE("%s: flags (0x%04x) : %s\n",
                debug_shfileops_action(FuncSwitch), nFileOp.fFlags,
                debug_shfileops_flags(nFileOp.fFlags) );

        /* establish when pTo is interpreted as the name of the destination file
         * or the directory where the Fromfile should be copied to.
         * This depends on:
         * (1) pTo points to the name of an existing directory;
         * (2) the flag FOF_MULTIDESTFILES is present;
         * (3) whether pFrom point to multiple filenames.
         *
         * Some experiments:
         *
         * destisdir               1 1 1 1 0 0 0 0
         * FOF_MULTIDESTFILES      1 1 0 0 1 1 0 0
         * multiple from filenames 1 0 1 0 1 0 1 0
         *                         ---------------
         * copy files to dir       1 0 1 1 0 0 1 0
         * create dir              0 0 0 0 0 0 1 0
         */
/*
 * FOF_MULTIDESTFILES, FOF_NOCONFIRMATION, FOF_FILESONLY  are implemented
 * FOF_CONFIRMMOUSE, FOF_SILENT, FOF_NOCONFIRMMKDIR,
 *       FOF_SIMPLEPROGRESS, FOF_NOCOPYSECURITYATTRIBS    are not implemented and ignored
 * FOF_RENAMEONCOLLISION                                  are implemented partially and breaks if file exist
 * FOF_ALLOWUNDO, FOF_WANTMAPPINGHANDLE                   are not implemented and breaks
 * if any other flag set, an error occurs
 */
        TRACE("%s level=%ld nFileOp.fFlags=0x%x\n", 
                debug_shfileops_action(FuncSwitch), level, lpFileOp->fFlags);

/*    OFl &= (-1 - (FOF_MULTIDESTFILES | FOF_FILESONLY)); */
/*    OFl ^= (FOF_SILENT | FOF_NOCONFIRMATION | FOF_SIMPLEPROGRESS | FOF_NOCONFIRMMKDIR); */
        OFl &= (~(FOF_MULTIDESTFILES | FOF_NOCONFIRMATION | FOF_FILESONLY));  /* implemented */
        OFl ^= (FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_NOCOPYSECURITYATTRIBS); /* ignored, if one */
        OFl &= (~FOF_SIMPLEPROGRESS);                      /* ignored, only with FOF_SILENT */
        if (OFl)
        {
	    if (OFl & (~(FOF_CONFIRMMOUSE | FOF_SILENT | FOF_RENAMEONCOLLISION |
	                 FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_NOCOPYSECURITYATTRIBS)))
	    {
                TRACE("%s level=%ld lpFileOp->fFlags=0x%x not implemented, Aborted=TRUE, stub\n",
                      debug_shfileops_action(FuncSwitch), level, OFl);
                retCode = 0x403; /* 1027, we need an extension to shlfileop */
                goto shfileop_error;
	    }
	    else
	    {
                TRACE("%s level=%ld lpFileOp->fFlags=0x%x not fully implemented, stub\n", 
                      debug_shfileops_action(FuncSwitch), level, OFl);
	    } 
        } 

        if ((pNextFrom) && (!(b_MultiTo) || (pNextTo)))
        {
	    nFileOp.pFrom = pTempFrom = HeapAlloc(GetProcessHeap(), 0, ((1 + 2 * (b_MultiTo)) * MAX_PATH + 6) * sizeof(WCHAR));
	    if (!pTempFrom)
	    {
                retCode = ERROR_OUTOFMEMORY;
                SetLastError(retCode);
                goto shfileop_error;
	    }
	    if (b_MultiTo)
                pTempTo = &pTempFrom[MAX_PATH + 4];
	    nFileOp.pTo = pTempTo;
	    ask_overwrite = (!(nFileOp.fFlags & FOF_NOCONFIRMATION) && !(nFileOp.fFlags & FOF_RENAMEONCOLLISION));
	    not_overwrite = (!(nFileOp.fFlags & FOF_NOCONFIRMATION) ||  (nFileOp.fFlags & FOF_RENAMEONCOLLISION));
        }
        else
        {
	    retCode = 0x402;      /* 1026 */
	    goto shfileop_error;
        }
        /* need break at error before change sourcepointer */
        while(!nFileOp.fAnyOperationsAborted && (pNextFrom[0]))
        {
	    nFileOp.wFunc =  ((level + 1) << 4) + FuncSwitch;
	    nFileOp.fFlags = lpFileOp->fFlags;

	    if (b_MultiTo)
	    {
                pTo = pNextTo;
                pNextTo = &pNextTo[lstrlenW(pTo)+1];
                b_MultiTo = (b_Multi && pNextTo[0]);
	    }

	    pFrom = pNextFrom;
	    pNextFrom = &pNextFrom[lstrlenW(pNextFrom)+1];
	    if (!b_MultiFrom && !b_MultiTo)
                b_MultiFrom = (pNextFrom[0]);

	    pFromFile = SHFileStrCpyCatW(pTempFrom, pFrom, NULL);

	    if (pTo)
	    {
                pToFile = SHFileStrCpyCatW(pTempTo, pTo, NULL);
	    }
	    if (!b_MultiPaired)
	    {
                b_MultiPaired =
                    SHELL_FileNamesMatch(lpFileOp->pFrom, lpFileOp->pTo, (!b_Multi || b_MultiFrom));
	    }
	    if (!(b_MultiPaired) || !(pFromFile) || !(pFromFile[1]) || ((pTo) && !(pToFile)))
	    {
                retCode = 0x402;      /* 1026 */
                goto shfileop_error;
	    }
	    if (pTo)
	    {
                b_ToTailSlash = (!pToFile[1]);
                if (b_ToTailSlash)
                {
                    pToFile[0] = '\0';
                    if (StrChrW(pTempTo,'\\'))
                    {
                        pToFile = SHFileStrCpyCatW(pTempTo, NULL, NULL);
                    }
                }
                b_ToInvalidTail = (NULL != StrPBrkW(&pToFile[1], wWildcardChars));
	    }

	    /* for all */
	    b_Mask = (NULL != StrPBrkW(&pFromFile[1], wWildcardChars));
	    if (FO_RENAME == FuncSwitch)
	    {
                /* temporary only for FO_RENAME */
                if (b_MultiTo || b_MultiFrom || (b_Mask && !b_ToInvalidTail))
                {
#ifndef W98_FO_FUNCTION
                    retCode = ERROR_GEN_FAILURE;  /* W2K ERROR_GEN_FAILURE, W98 returns no error */
#endif
                    goto shfileop_error;
                }
	    }

	    hFind = FindFirstFileW(pFrom, &wfd);
	    if (INVALID_HANDLE_VALUE == hFind)
	    {
                if ((FO_DELETE == FuncSwitch) && (b_Mask))
                {
                    pFromFile[0] = '\0';
                    FromPathAttr = GetFileAttributesW(pTempFrom);
                    pFromFile[0] = '\\';
                    if (IsAttribDir(FromPathAttr))
                    {
                        /* FO_DELETE with mask and without found is valid */
                        goto shfileop_normal;
                    }
                }
                /* root (without mask) is also not allowed as source, tested in W98 */
                retCode = 0x402;   /* 1026 */
                goto shfileop_error;
	    }

/* for all */
#define HIGH_ADR (LPWSTR)0xffffffff

/* ???	    b_Mask = (!SHFileStrICmpA(&pFromFile[1], &wfd.cFileName[0], HIGH_ADR, HIGH_ADR)); */
	    if (!pTo) /* FO_DELETE */
	    {
                do
                {
                    lpFileName = wfd.cAlternateFileName;
                    if (!lpFileName[0])
                        lpFileName = wfd.cFileName;
                    if (IsDotDir(lpFileName) ||
                        ((b_Mask) && IsAttribDir(wfd.dwFileAttributes) && (nFileOp.fFlags & FOF_FILESONLY)))
                        continue;
                    SHFileStrCpyCatW(&pFromFile[1], lpFileName, NULL);
                    /* TODO: Check the SHELL_DeleteFileOrDirectoryW() function in shell32.dll */
                    if (IsAttribFile(wfd.dwFileAttributes))
                    {
                        nFileOp.fAnyOperationsAborted = (SHNotifyDeleteFileW(pTempFrom) != ERROR_SUCCESS);
                        retCode = 0x78; /* value unknown */
                    }
                    else
                    {
                        nFileOp.fAnyOperationsAborted = (!SHELL_DeleteDirectoryW(pTempFrom, (!(nFileOp.fFlags & FOF_NOCONFIRMATION))));
                        retCode = 0x79; /* value unknown */
                    }
                } while (!nFileOp.fAnyOperationsAborted && FindNextFileW(hFind, &wfd));
                FindClose(hFind);
                hFind = INVALID_HANDLE_VALUE;
                if (nFileOp.fAnyOperationsAborted)
                {
                    goto shfileop_error;
                }
                continue;
	    } /* FO_DELETE ends, pTo must be always valid from here */

	    b_SameRoot = (toupperW(pTempFrom[0]) == toupperW(pTempTo[0]));
	    b_SameTailName = SHFileStrICmpW(pToFile, pFromFile, NULL, NULL);

	    ToPathAttr = ToAttr = GetFileAttributesW(pTempTo);
	    if (!b_Mask && (ToAttr == -1) && (pToFile))
	    {
                pToFile[0] = '\0';
                ToPathAttr = GetFileAttributesW(pTempTo);
                pToFile[0] = '\\';
	    }

	    if (FO_RENAME == FuncSwitch)
	    {
                if (!b_SameRoot || b_Mask /* FO_RENAME works not with Mask */
                    || !SHFileStrICmpW(pTempFrom, pTempTo, pFromFile, NULL)
                    || (SHFileStrICmpW(pTempFrom, pTempTo, pFromFile, HIGH_ADR) && !b_ToTailSlash))
                {
                    retCode = 0x73;
                    goto shfileop_error;
                }
                if (b_ToInvalidTail)
                {
                    retCode=0x2;
                    goto shfileop_error;
                }
                if (-1 == ToPathAttr)
                {
                    retCode = 0x75;
                    goto shfileop_error;
                }
                if (IsAttribDir(wfd.dwFileAttributes) && IsAttribDir(ToAttr))
                {
                    retCode = (b_ToTailSlash) ? 0xb7 : 0x7b;
                    goto shfileop_error;
                }
                /* we use SHNotifyMoveFile() instead MoveFileW */
                if (SHNotifyMoveFileW(pTempFrom, pTempTo, nFileOp.fFlags & FOF_RENAMEONCOLLISION) != ERROR_SUCCESS)
                {
                    /* we need still the value for the returncode, we use the mostly assumed */
                    retCode = 0xb7;
                    goto shfileop_error;
                }
                goto shfileop_normal;
	    }

	    /* W98 Bug with FO_MOVE different to FO_COPY, better the same as FO_COPY */
	    b_ToValid = ((b_SameTailName &&  b_SameRoot && (FO_COPY == FuncSwitch)) ||
                         (b_SameTailName && !b_SameRoot) || (b_ToInvalidTail));

	    /* handle mask in source */
	    if (b_Mask)
	    {
                if (!IsAttribDir(ToAttr))
                {
                    retCode = (b_ToInvalidTail &&/* b_SameTailName &&*/ (FO_MOVE == FuncSwitch)) \
                        ? 0x2 : 0x75;
                    goto shfileop_error;
                }
                pToFile = SHFileStrCpyCatW(pTempTo, NULL, wBackslash);
                nFileOp.fFlags = (nFileOp.fFlags | FOF_MULTIDESTFILES);
                do
                {
                    lpFileName = wfd.cAlternateFileName;
                    if (!lpFileName[0])
                        lpFileName = wfd.cFileName;
                    if (IsDotDir(lpFileName) ||
                        (IsAttribDir(wfd.dwFileAttributes) && (nFileOp.fFlags & FOF_FILESONLY)))
                        continue; /* next name in pTempFrom(dir) */
                    SHFileStrCpyCatW(&pToFile[1], lpFileName, NULL);
                    SHFileStrCpyCatW(&pFromFile[1], lpFileName, NULL);
                    retCode = SHFileOperationW (&nFileOp);
                } while(!nFileOp.fAnyOperationsAborted && FindNextFileW(hFind, &wfd));
	    }
	    FindClose(hFind);
	    hFind = INVALID_HANDLE_VALUE;
	    /* FO_COPY/FO_MOVE with mask, FO_DELETE and FO_RENAME are solved */
	    if (b_Mask)
                continue;

	    /* only FO_COPY/FO_MOVE without mask, all others are (must be) solved */
	    if (IsAttribDir(wfd.dwFileAttributes) && (ToAttr == -1))
	    {
                if (pToFile)
                {
                    pToFile[0] = '\0';
                    ToPathAttr = GetFileAttributesW(pTempTo);
                    if ((ToPathAttr == -1) && b_ToValid)
                    {
                        /* create dir must be here, sample target D:\y\ *.* create with RC=10003 */
                        if (SHCreateDirectoryExW(NULL, pTempTo, NULL))
                        {
                            retCode = 0x73;/* value unknown */
                            goto shfileop_error;
                        }
                        ToPathAttr = GetFileAttributesW(pTempTo);
                    }
                    pToFile[0] = '\\';
                    if (b_ToInvalidTail)
                    {
                        retCode = 0x10003;
                        goto shfileop_error;
                    }
                }
	    }

	    /* trailing BackSlash is ever removed and pToFile points to BackSlash before */
	    if (!b_MultiTo && (b_MultiFrom || (!(b_Multi) && IsAttribDir(ToAttr))))
	    {
                if ((FO_MOVE == FuncSwitch) && IsAttribDir(ToAttr) && IsAttribDir(wfd.dwFileAttributes))
                {
                    if (b_Multi)
                    {
                        retCode = 0x73; /* !b_Multi = 0x8 ?? */
                        goto shfileop_error;
                    }
                }
                pToFile = SHFileStrCpyCatW(pTempTo, NULL, wfd.cFileName);
                ToAttr = GetFileAttributesW(pTempTo);
	    }

	    if (IsAttribDir(ToAttr))
	    {
                if (IsAttribFile(wfd.dwFileAttributes))
                {
                    retCode = (FO_COPY == FuncSwitch) ? 0x75 : 0xb7;
                    goto shfileop_error;
                }
	    }
	    else
	    {
                pToFile[0] = '\0';
                ToPathAttr = GetFileAttributesW(pTempTo);
                pToFile[0] = '\\';
                if (IsAttribFile(ToPathAttr))
                {
                    /* error, is this tested ? */
                    retCode = 0x777402;
                    goto shfileop_error;
                }
	    }

	    /* singlesource + no mask */
	    if (-1 == (ToAttr & ToPathAttr))
	    {
                /* Target-dir does not exist, and cannot be created */
                retCode=0x75;
                goto shfileop_error;
	    }

	    switch(FuncSwitch)
	    {
	    case FO_MOVE:
                pToFile = NULL;
                if ((ToAttr == -1) && SHFileStrICmpW(pTempFrom, pTempTo, pFromFile, NULL))
                {
                    nFileOp.wFunc =  ((level+1)<<4) + FO_RENAME;
                }
                else
                {
                    if (b_SameRoot && IsAttribDir(ToAttr) && IsAttribDir(wfd.dwFileAttributes))
                    {
                        /* we need pToFile for FO_DELETE after FO_MOVE contence */
                        pToFile = SHFileStrCpyCatW(pTempFrom, NULL, wWildcardFile);
                    }
                    else
                    {
                        nFileOp.wFunc =  ((level+1)<<4) + FO_COPY;
                    }
                }
                retCode = SHFileOperationW(&nFileOp);
                if (pToFile)
                    ((DWORD*)pToFile)[0] = '\0';
                if (!nFileOp.fAnyOperationsAborted && (FO_RENAME != (nFileOp.wFunc & 0xf)))
                {
                    nFileOp.wFunc =  ((level+1)<<4) + FO_DELETE;
                    retCode = SHFileOperationW(&nFileOp);
                }
                continue;
	    case FO_COPY:
                if (SHFileStrICmpW(pTempFrom, pTempTo, NULL, NULL))
                { /* target is the same as source ? */
                    /* we still need the value for the returncode, we assume 0x71 */
                    retCode = 0x71;
                    goto shfileop_error;
                }
                if (IsAttribDir((ToAttr & wfd.dwFileAttributes)))
                {
                    if (IsAttribDir(ToAttr) || !SHCreateDirectoryExW(NULL,pTempTo, NULL))
                    {
/* ???	          nFileOp.fFlags = (nFileOp.fFlags | FOF_MULTIDESTFILES); */
                        SHFileStrCpyCatW(pTempFrom, NULL, wWildcardFile);
                        retCode = SHFileOperationW(&nFileOp);
                    }
                    else
                    {
                        retCode = 0x750;/* value unknown */
                        goto shfileop_error;
                    }
                }
                else
                {
                    if (!(ask_overwrite && SHELL_ConfirmDialogW(ASK_OVERWRITE_FILE, pTempTo))
                        && (not_overwrite))
                    {
                        /* we still need the value for the returncode, we use the mostly assumed */
                        retCode = 0x73;
                        goto shfileop_error;
                    }
                    if (SHNotifyCopyFileW(pTempFrom, pTempTo, nFileOp.fFlags & FOF_RENAMEONCOLLISION) != ERROR_SUCCESS)
                    {
                        retCode = 0x77; /* value unknown */
                        goto shfileop_error;
                    }
                }
	    }
        }

shfileop_normal:
	if (!(nFileOp.fAnyOperationsAborted))
	  retCode = 0;
shfileop_error:
	if (hFind != INVALID_HANDLE_VALUE)
	  FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;
	if (pTempFrom)
	  HeapFree(GetProcessHeap(), 0, pTempFrom);
	if (retCode)
	{
	  nFileOp.fAnyOperationsAborted = TRUE;
	}
	TRACE("%s level=%ld AnyOpsAborted=%s ret=0x%x, with %s %s%s\n",
              debug_shfileops_action(FuncSwitch), level,
	      nFileOp.fAnyOperationsAborted ? "TRUE":"FALSE",
	      retCode, debugstr_w(pFrom), pTo ? "-> ":"", debugstr_w(pTo));

	lpFileOp->fAnyOperationsAborted = nFileOp.fAnyOperationsAborted;
	return retCode;
}

/*************************************************************************
 * SHFileOperation        [SHELL32.@]
 *
 */
DWORD WINAPI SHFileOperationAW(LPVOID lpFileOp)
{
	if (SHELL_OsIsUnicode())
	  return SHFileOperationW(lpFileOp);
	return SHFileOperationA(lpFileOp);
}

/*************************************************************************
 * SheGetDirW [SHELL32.281]
 *
 */
HRESULT WINAPI SheGetDirW(LPWSTR u, LPWSTR v)
{	FIXME("%p %p stub\n",u,v);
	return 0;
}

/*************************************************************************
 * SheChangeDirW [SHELL32.274]
 *
 */
HRESULT WINAPI SheChangeDirW(LPWSTR u)
{	FIXME("(%s),stub\n",debugstr_w(u));
	return 0;
}

/*************************************************************************
 * IsNetDrive			[SHELL32.66]
 */
BOOL WINAPI IsNetDrive(DWORD drive)
{
	char root[4];
	strcpy(root, "A:\\");
	root[0] += (char)drive;
	return (GetDriveTypeA(root) == DRIVE_REMOTE);
}
