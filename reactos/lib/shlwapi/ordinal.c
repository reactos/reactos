/*
 * SHLWAPI ordinal functions
 *
 * Copyright 1997 Marcus Meissner
 *           1998 Jürgen Schmied
 *           2001-2003 Jon Griffiths
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

#define COM_NO_WINDOWS_H
#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "docobj.h"
#include "exdisp.h"
#include "shlguid.h"
#include "wingdi.h"
#include "shlobj.h"
#include "olectl.h"
#include "shellapi.h"
#include "commdlg.h"
#include "wine/unicode.h"
#include "servprov.h"
#include "winreg.h"
#include "wine/debug.h"
#include "shlwapi.h"


WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* Get a function pointer from a DLL handle */
#define GET_FUNC(func, module, name, fail) \
  do { \
    if (!func) { \
      if (!SHLWAPI_h##module && !(SHLWAPI_h##module = LoadLibraryA(#module ".dll"))) return fail; \
      func = (fn##func)GetProcAddress(SHLWAPI_h##module, name); \
      if (!func) return fail; \
    } \
  } while (0)

/* DLL handles for late bound calls */
extern HINSTANCE shlwapi_hInstance;
extern HMODULE SHLWAPI_hshell32;
extern HMODULE SHLWAPI_hwinmm;
extern HMODULE SHLWAPI_hcomdlg32;
extern HMODULE SHLWAPI_hcomctl32;
extern HMODULE SHLWAPI_hmpr;
extern HMODULE SHLWAPI_hurlmon;
extern HMODULE SHLWAPI_hversion;

extern DWORD SHLWAPI_ThreadRef_index;

typedef HANDLE HSHARED; /* Shared memory */

/* following is GUID for IObjectWithSite::SetSite  -- see _174           */
static DWORD id1[4] = {0xfc4801a3, 0x11cf2ba9, 0xaa0029a2, 0x52733d00};
/* following is GUID for IPersistMoniker::GetClassID  -- see _174        */
static DWORD id2[4] = {0x79eac9ee, 0x11cebaf9, 0xaa00828c, 0x0ba94b00};

/* Function pointers for GET_FUNC macro; these need to be global because of gcc bug */
typedef LPITEMIDLIST (WINAPI *fnpSHBrowseForFolderW)(LPBROWSEINFOW);
static  fnpSHBrowseForFolderW pSHBrowseForFolderW;
typedef BOOL    (WINAPI *fnpPlaySoundW)(LPCWSTR, HMODULE, DWORD);
static  fnpPlaySoundW pPlaySoundW;
typedef DWORD   (WINAPI *fnpSHGetFileInfoW)(LPCWSTR,DWORD,SHFILEINFOW*,UINT,UINT);
static  fnpSHGetFileInfoW pSHGetFileInfoW;
typedef UINT    (WINAPI *fnpDragQueryFileW)(HDROP, UINT, LPWSTR, UINT);
static  fnpDragQueryFileW pDragQueryFileW;
typedef BOOL    (WINAPI *fnpSHGetPathFromIDListW)(LPCITEMIDLIST, LPWSTR);
static  fnpSHGetPathFromIDListW pSHGetPathFromIDListW;
typedef BOOL    (WINAPI *fnpShellExecuteExW)(LPSHELLEXECUTEINFOW);
static  fnpShellExecuteExW pShellExecuteExW;
typedef HICON   (WINAPI *fnpSHFileOperationW)(LPSHFILEOPSTRUCTW);
static  fnpSHFileOperationW pSHFileOperationW;
typedef UINT    (WINAPI *fnpExtractIconExW)(LPCWSTR, INT,HICON *,HICON *, UINT);
static  fnpExtractIconExW pExtractIconExW;
typedef BOOL    (WINAPI *fnpSHGetNewLinkInfoW)(LPCWSTR, LPCWSTR, LPCWSTR, BOOL*, UINT);
static  fnpSHGetNewLinkInfoW pSHGetNewLinkInfoW;
typedef HRESULT (WINAPI *fnpSHDefExtractIconW)(LPCWSTR, int, UINT, HICON*, HICON*, UINT);
static  fnpSHDefExtractIconW pSHDefExtractIconW;
typedef HICON   (WINAPI *fnpExtractIconW)(HINSTANCE, LPCWSTR, UINT);
static  fnpExtractIconW pExtractIconW;
typedef BOOL    (WINAPI *fnpGetSaveFileNameW)(LPOPENFILENAMEW);
static  fnpGetSaveFileNameW pGetSaveFileNameW;
typedef DWORD   (WINAPI *fnpWNetRestoreConnectionW)(HWND, LPWSTR);
static  fnpWNetRestoreConnectionW pWNetRestoreConnectionW;
typedef DWORD   (WINAPI *fnpWNetGetLastErrorW)(LPDWORD, LPWSTR, DWORD, LPWSTR, DWORD);
static  fnpWNetGetLastErrorW pWNetGetLastErrorW;
typedef BOOL    (WINAPI *fnpPageSetupDlgW)(LPPAGESETUPDLGW);
static  fnpPageSetupDlgW pPageSetupDlgW;
typedef BOOL    (WINAPI *fnpPrintDlgW)(LPPRINTDLGW);
static  fnpPrintDlgW pPrintDlgW;
typedef BOOL    (WINAPI *fnpGetOpenFileNameW)(LPOPENFILENAMEW);
static  fnpGetOpenFileNameW pGetOpenFileNameW;
typedef DWORD   (WINAPI *fnpGetFileVersionInfoSizeW)(LPCWSTR,LPDWORD);
static  fnpGetFileVersionInfoSizeW pGetFileVersionInfoSizeW;
typedef BOOL    (WINAPI *fnpGetFileVersionInfoW)(LPCWSTR,DWORD,DWORD,LPVOID);
static  fnpGetFileVersionInfoW pGetFileVersionInfoW;
typedef WORD    (WINAPI *fnpVerQueryValueW)(LPVOID,LPCWSTR,LPVOID*,UINT*);
static  fnpVerQueryValueW pVerQueryValueW;
typedef BOOL    (WINAPI *fnpCOMCTL32_417)(HDC,INT,INT,UINT,const RECT*,LPCWSTR,UINT,const INT*);
static  fnpCOMCTL32_417 pCOMCTL32_417;
typedef HRESULT (WINAPI *fnpDllGetVersion)(DLLVERSIONINFO*);
static  fnpDllGetVersion pDllGetVersion;
typedef HRESULT (WINAPI *fnpCreateFormatEnumerator)(UINT,FORMATETC*,IEnumFORMATETC**);
static fnpCreateFormatEnumerator pCreateFormatEnumerator;
typedef HRESULT (WINAPI *fnpRegisterFormatEnumerator)(LPBC,IEnumFORMATETC*,DWORD);
static fnpRegisterFormatEnumerator pRegisterFormatEnumerator;

HRESULT WINAPI IUnknown_QueryService(IUnknown*,REFGUID,REFIID,LPVOID*);
HRESULT WINAPI SHInvokeCommand(HWND,IShellFolder*,LPCITEMIDLIST,BOOL);
HRESULT WINAPI CLSIDFromStringWrap(LPCWSTR,CLSID*);
BOOL    WINAPI SHAboutInfoW(LPWSTR,DWORD);

/*
 NOTES: Most functions exported by ordinal seem to be superflous.
 The reason for these functions to be there is to provide a wrapper
 for unicode functions to provide these functions on systems without
 unicode functions eg. win95/win98. Since we have such functions we just
 call these. If running Wine with native DLL's, some late bound calls may
 fail. However, its better to implement the functions in the forward DLL
 and recommend the builtin rather than reimplementing the calls here!
*/

/*************************************************************************
 * SHLWAPI_DupSharedHandle
 *
 * Internal implemetation of SHLWAPI_11.
 */
static
HSHARED WINAPI SHLWAPI_DupSharedHandle(HSHARED hShared, DWORD dwDstProcId,
                                       DWORD dwSrcProcId, DWORD dwAccess,
                                       DWORD dwOptions)
{
  HANDLE hDst, hSrc;
  DWORD dwMyProcId = GetCurrentProcessId();
  HSHARED hRet = (HSHARED)NULL;

  TRACE("(%p,%ld,%ld,%08lx,%08lx)\n", (PVOID)hShared, dwDstProcId, dwSrcProcId,
        dwAccess, dwOptions);

  /* Get dest process handle */
  if (dwDstProcId == dwMyProcId)
    hDst = GetCurrentProcess();
  else
    hDst = OpenProcess(PROCESS_DUP_HANDLE, 0, dwDstProcId);

  if (hDst)
  {
    /* Get src process handle */
    if (dwSrcProcId == dwMyProcId)
      hSrc = GetCurrentProcess();
    else
      hSrc = OpenProcess(PROCESS_DUP_HANDLE, 0, dwSrcProcId);

    if (hSrc)
    {
      /* Make handle available to dest process */
      if (!DuplicateHandle(hDst, (HANDLE)hShared, hSrc, &hRet,
                           dwAccess, 0, dwOptions | DUPLICATE_SAME_ACCESS))
        hRet = (HSHARED)NULL;

      if (dwSrcProcId != dwMyProcId)
        CloseHandle(hSrc);
    }

    if (dwDstProcId != dwMyProcId)
      CloseHandle(hDst);
  }

  TRACE("Returning handle %p\n", (PVOID)hRet);
  return hRet;
}

/*************************************************************************
 * @  [SHLWAPI.7]
 *
 * Create a block of sharable memory and initialise it with data.
 *
 * PARAMS
 * dwProcId [I] ID of process owning data
 * lpvData  [I] Pointer to data to write
 * dwSize   [I] Size of data
 *
 * RETURNS
 * Success: A shared memory handle
 * Failure: NULL
 *
 * NOTES
 * Ordinals 7-11 provide a set of calls to create shared memory between a
 * group of processes. The shared memory is treated opaquely in that its size
 * is not exposed to clients who map it. This is accomplished by storing
 * the size of the map as the first DWORD of mapped data, and then offsetting
 * the view pointer returned by this size.
 *
 */
HSHARED WINAPI SHAllocShared(DWORD dwProcId, DWORD dwSize, LPCVOID lpvData)
{
  HANDLE hMap;
  LPVOID pMapped;
  HSHARED hRet = (HSHARED)NULL;

  TRACE("(%ld,%p,%ld)\n", dwProcId, lpvData, dwSize);

  /* Create file mapping of the correct length */
  hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, FILE_MAP_READ, 0,
                            dwSize + sizeof(dwSize), NULL);
  if (!hMap)
    return hRet;

  /* Get a view in our process address space */
  pMapped = MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

  if (pMapped)
  {
    /* Write size of data, followed by the data, to the view */
    *((DWORD*)pMapped) = dwSize;
    if (dwSize)
      memcpy((char *) pMapped + sizeof(dwSize), lpvData, dwSize);

    /* Release view. All further views mapped will be opaque */
    UnmapViewOfFile(pMapped);
    hRet = SHLWAPI_DupSharedHandle((HSHARED)hMap, dwProcId,
                                   GetCurrentProcessId(), FILE_MAP_ALL_ACCESS,
                                   DUPLICATE_SAME_ACCESS);
  }

  CloseHandle(hMap);
  return hRet;
}

/*************************************************************************
 * @ [SHLWAPI.8]
 *
 * Get a pointer to a block of shared memory from a shared memory handle.
 *
 * PARAMS
 * hShared  [I] Shared memory handle
 * dwProcId [I] ID of process owning hShared
 *
 * RETURNS
 * Success: A pointer to the shared memory
 * Failure: NULL
 *
 */
PVOID WINAPI SHLockShared(HSHARED hShared, DWORD dwProcId)
{
  HSHARED hDup;
  LPVOID pMapped;

  TRACE("(%p %ld)\n", (PVOID)hShared, dwProcId);

  /* Get handle to shared memory for current process */
  hDup = SHLWAPI_DupSharedHandle(hShared, dwProcId, GetCurrentProcessId(),
                                 FILE_MAP_ALL_ACCESS, 0);
  /* Get View */
  pMapped = MapViewOfFile((HANDLE)hDup, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
  CloseHandle(hDup);

  if (pMapped)
    return (char *) pMapped + sizeof(DWORD); /* Hide size */
  return NULL;
}

/*************************************************************************
 * @ [SHLWAPI.9]
 *
 * Release a pointer to a block of shared memory.
 *
 * PARAMS
 * lpView [I] Shared memory pointer
 *
 * RETURNS
 * Success: TRUE
 * Failure: FALSE
 *
 */
BOOL WINAPI SHUnlockShared(LPVOID lpView)
{
  TRACE("(%p)\n", lpView);
  return UnmapViewOfFile((char *) lpView - sizeof(DWORD)); /* Include size */
}

/*************************************************************************
 * @ [SHLWAPI.10]
 *
 * Destroy a block of sharable memory.
 *
 * PARAMS
 * hShared  [I] Shared memory handle
 * dwProcId [I] ID of process owning hShared
 *
 * RETURNS
 * Success: TRUE
 * Failure: FALSE
 *
 */
BOOL WINAPI SHFreeShared(HSHARED hShared, DWORD dwProcId)
{
  HSHARED hClose;

  TRACE("(%p %ld)\n", (PVOID)hShared, dwProcId);

  /* Get a copy of the handle for our process, closing the source handle */
  hClose = SHLWAPI_DupSharedHandle(hShared, dwProcId, GetCurrentProcessId(),
                                   FILE_MAP_ALL_ACCESS,DUPLICATE_CLOSE_SOURCE);
  /* Close local copy */
  return CloseHandle((HANDLE)hClose);
}

/*************************************************************************
 * @   [SHLWAPI.11]
 *
 * Copy a sharable memory handle from one process to another.
 *
 * PARAMS
 * hShared     [I] Shared memory handle to duplicate
 * dwDstProcId [I] ID of the process wanting the duplicated handle
 * dwSrcProcId [I] ID of the process owning hShared
 * dwAccess    [I] Desired DuplicateHandle() access
 * dwOptions   [I] Desired DuplicateHandle() options
 *
 * RETURNS
 * Success: A handle suitable for use by the dwDstProcId process.
 * Failure: A NULL handle.
 *
 */
HSHARED WINAPI SHMapHandle(HSHARED hShared, DWORD dwDstProcId, DWORD dwSrcProcId,
                          DWORD dwAccess, DWORD dwOptions)
{
  HSHARED hRet;

  hRet = SHLWAPI_DupSharedHandle(hShared, dwDstProcId, dwSrcProcId,
                                 dwAccess, dwOptions);
  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.13]
 *
 * Create and register a clipboard enumerator for a web browser.
 *
 * PARAMS
 *  lpBC      [I] Binding context
 *  lpUnknown [I] An object exposing the IWebBrowserApp interface
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code.
 *
 * NOTES
 *  The enumerator is stored as a property of the web browser. If it does not
 *  yet exist, it is created and set before being registered.
 */
HRESULT WINAPI RegisterDefaultAcceptHeaders(LPBC lpBC, IUnknown *lpUnknown)
{
  static const WCHAR szProperty[] = { '{','D','0','F','C','A','4','2','0',
      '-','D','3','F','5','-','1','1','C','F', '-','B','2','1','1','-','0',
      '0','A','A','0','0','4','A','E','8','3','7','}','\0' };
  IEnumFORMATETC* pIEnumFormatEtc = NULL;
  VARIANTARG var;
  HRESULT hRet;
  IWebBrowserApp* pBrowser = NULL;

  TRACE("(%p, %p)\n", lpBC, lpUnknown);

  /* Get An IWebBrowserApp interface from  lpUnknown */
  hRet = IUnknown_QueryService(lpUnknown, &IID_IWebBrowserApp, &IID_IWebBrowserApp, (PVOID)&pBrowser);
  if (FAILED(hRet) || !pBrowser)
    return E_NOINTERFACE;

  V_VT(&var) = VT_EMPTY;

  /* The property we get is the browsers clipboard enumerator */
  hRet = IWebBrowserApp_GetProperty(pBrowser, (BSTR)szProperty, &var);
  if (FAILED(hRet))
    return hRet;

  if (V_VT(&var) == VT_EMPTY)
  {
    /* Iterate through accepted documents and RegisterClipBoardFormatA() them */
    char szKeyBuff[128], szValueBuff[128];
    DWORD dwKeySize, dwValueSize, dwRet = 0, dwCount = 0, dwNumValues, dwType;
    FORMATETC* formatList, *format;
    HKEY hDocs;

    TRACE("Registering formats and creating IEnumFORMATETC instance\n");

    if (!RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\Current"
                     "Version\\Internet Settings\\Accepted Documents", &hDocs))
      return E_FAIL;

    /* Get count of values in key */
    while (!dwRet)
    {
      dwKeySize = sizeof(szKeyBuff);
      dwRet = RegEnumValueA(hDocs,dwCount,szKeyBuff,&dwKeySize,0,&dwType,0,0);
      dwCount++;
    }

    dwNumValues = dwCount;

    /* Note: dwCount = number of items + 1; The extra item is the end node */
    format = formatList = HeapAlloc(GetProcessHeap(), 0, dwCount * sizeof(FORMATETC));
    if (!formatList)
      return E_OUTOFMEMORY;

    if (dwNumValues > 1)
    {
      dwRet = 0;
      dwCount = 0;

      dwNumValues--;

      /* Register clipboard formats for the values and populate format list */
      while(!dwRet && dwCount < dwNumValues)
      {
        dwKeySize = sizeof(szKeyBuff);
        dwValueSize = sizeof(szValueBuff);
        dwRet = RegEnumValueA(hDocs, dwCount, szKeyBuff, &dwKeySize, 0, &dwType,
                              (PBYTE)szValueBuff, &dwValueSize);
        if (!dwRet)
          return E_FAIL;

        format->cfFormat = RegisterClipboardFormatA(szValueBuff);
        format->ptd = NULL;
        format->dwAspect = 1;
        format->lindex = 4;
        format->tymed = -1;

        format++;
        dwCount++;
      }
    }

    /* Terminate the (maybe empty) list, last entry has a cfFormat of 0 */
    format->cfFormat = 0;
    format->ptd = NULL;
    format->dwAspect = 1;
    format->lindex = 4;
    format->tymed = -1;

    /* Create a clipboard enumerator */
    GET_FUNC(pCreateFormatEnumerator, urlmon, "CreateFormatEnumerator", E_FAIL);
    hRet = pCreateFormatEnumerator(dwNumValues, formatList, &pIEnumFormatEtc);

    if (FAILED(hRet) || !pIEnumFormatEtc)
      return hRet;

    /* Set our enumerator as the browsers property */
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown*)pIEnumFormatEtc;

    hRet = IWebBrowserApp_PutProperty(pBrowser, (BSTR)szProperty, var);
    if (FAILED(hRet))
    {
       IEnumFORMATETC_Release(pIEnumFormatEtc);
       goto RegisterDefaultAcceptHeaders_Exit;
    }
  }

  if (V_VT(&var) == VT_UNKNOWN)
  {
    /* Our variant is holding the clipboard enumerator */
    IUnknown* pIUnknown = V_UNKNOWN(&var);
    IEnumFORMATETC* pClone = NULL;

    TRACE("Retrieved IEnumFORMATETC property\n");

    /* Get an IEnumFormatEtc interface from the variants value */
    pIEnumFormatEtc = NULL;
    hRet = IUnknown_QueryInterface(pIUnknown, &IID_IEnumFORMATETC,
                                   (PVOID)&pIEnumFormatEtc);
    if (!hRet && pIEnumFormatEtc)
    {
      /* Clone and register the enumerator */
      hRet = IEnumFORMATETC_Clone(pIEnumFormatEtc, &pClone);
      if (!hRet && pClone)
      {
        GET_FUNC(pRegisterFormatEnumerator, urlmon, "RegisterFormatEnumerator", E_FAIL);
        pRegisterFormatEnumerator(lpBC, pClone, 0);

        IEnumFORMATETC_Release(pClone);
      }

      /* Release the IEnumFormatEtc interface */
      IEnumFORMATETC_Release(pIUnknown);
    }
    IUnknown_Release(V_UNKNOWN(&var));
  }

RegisterDefaultAcceptHeaders_Exit:
  IWebBrowserApp_Release(pBrowser);
  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.14]
 *
 * Get Explorers "AcceptLanguage" setting.
 *
 * PARAMS
 *  langbuf [O] Destination for language string
 *  buflen  [I] Length of langbuf
 *
 * RETURNS
 *  Success: S_OK.   langbuf is set to the language string found.
 *  Failure: E_FAIL, If any arguments are invalid, error occurred, or Explorer
 *           does not contain the setting.
 */
HRESULT WINAPI GetAcceptLanguagesA(
	LPSTR langbuf,
	LPDWORD buflen)
{
	CHAR *mystr;
	DWORD mystrlen, mytype;
	HKEY mykey;
	LCID mylcid;

	mystrlen = (*buflen > 6) ? *buflen : 6;
	mystr = (CHAR*)HeapAlloc(GetProcessHeap(),
				 HEAP_ZERO_MEMORY, mystrlen);
	RegOpenKeyA(HKEY_CURRENT_USER,
		    "Software\\Microsoft\\Internet Explorer\\International",
		    &mykey);
	if (RegQueryValueExA(mykey, "AcceptLanguage",
			      0, &mytype, (PBYTE)mystr, &mystrlen)) {
	    /* Did not find value */
	    mylcid = GetUserDefaultLCID();
	    /* somehow the mylcid translates into "en-us"
	     *  this is similar to "LOCALE_SABBREVLANGNAME"
	     *  which could be gotten via GetLocaleInfo.
	     *  The only problem is LOCALE_SABBREVLANGUAGE" is
	     *  a 3 char string (first 2 are country code and third is
	     *  letter for "sublanguage", which does not come close to
	     *  "en-us"
	     */
	    lstrcpyA(mystr, "en-us");
	    mystrlen = lstrlenA(mystr);
	}
	else {
	    /* handle returned string */
	    FIXME("missing code\n");
	}
	if (mystrlen > *buflen)
	    lstrcpynA(langbuf, mystr, *buflen);
	else {
	    lstrcpyA(langbuf, mystr);
	    *buflen = lstrlenA(langbuf);
	}
	RegCloseKey(mykey);
	HeapFree(GetProcessHeap(), 0, mystr);
	TRACE("language is %s\n", debugstr_a(langbuf));
	return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.15]
 *
 * Unicode version of GetAcceptLanguagesA.
 */
HRESULT WINAPI GetAcceptLanguagesW(
	LPWSTR langbuf,
	LPDWORD buflen)
{
	CHAR *mystr;
	DWORD mystrlen, mytype;
	HKEY mykey;
	LCID mylcid;

	mystrlen = (*buflen > 6) ? *buflen : 6;
	mystr = (CHAR*)HeapAlloc(GetProcessHeap(),
				 HEAP_ZERO_MEMORY, mystrlen);
	RegOpenKeyA(HKEY_CURRENT_USER,
		    "Software\\Microsoft\\Internet Explorer\\International",
		    &mykey);
	if (RegQueryValueExA(mykey, "AcceptLanguage",
			      0, &mytype, (PBYTE)mystr, &mystrlen)) {
	    /* Did not find value */
	    mylcid = GetUserDefaultLCID();
	    /* somehow the mylcid translates into "en-us"
	     *  this is similar to "LOCALE_SABBREVLANGNAME"
	     *  which could be gotten via GetLocaleInfo.
	     *  The only problem is LOCALE_SABBREVLANGUAGE" is
	     *  a 3 char string (first 2 are country code and third is
	     *  letter for "sublanguage", which does not come close to
	     *  "en-us"
	     */
	    lstrcpyA(mystr, "en-us");
	    mystrlen = lstrlenA(mystr);
	}
	else {
	    /* handle returned string */
	    FIXME("missing code\n");
	}
	RegCloseKey(mykey);
	*buflen = MultiByteToWideChar(0, 0, mystr, -1, langbuf, (*buflen)-1);
	HeapFree(GetProcessHeap(), 0, mystr);
	TRACE("language is %s\n", debugstr_w(langbuf));
	return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.23]
 *
 * Convert a GUID to a string.
 *
 * PARAMS
 *  guid [I] GUID to convert
 *  str  [O] Destination for string
 *  cmax [I] Length of output buffer
 *
 * RETURNS
 *  The length of the string created.
 */
INT WINAPI SHStringFromGUIDA(REFGUID guid, LPSTR lpszDest, INT cchMax)
{
  char xguid[40];
  INT iLen;

  TRACE("(%s,%p,%d)\n", debugstr_guid(guid), lpszDest, cchMax);

  sprintf(xguid, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
          guid->Data1, guid->Data2, guid->Data3,
          guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
          guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);

  iLen = strlen(xguid) + 1;

  if (iLen > cchMax)
    return 0;
  memcpy(lpszDest, xguid, iLen);
  return iLen;
}

/*************************************************************************
 *      @	[SHLWAPI.24]
 *
 * Unicode version of SHStringFromGUIDA.
 */
INT WINAPI SHStringFromGUIDW(REFGUID guid, LPWSTR lpszDest, INT cchMax)
{
  char xguid[40];
  INT iLen = SHStringFromGUIDA(guid, xguid, cchMax);

  if (iLen)
    MultiByteToWideChar(CP_ACP, 0, xguid, -1, lpszDest, cchMax);
  return iLen;
}

/*************************************************************************
 *      @	[SHLWAPI.25]
 *
 * Determine if a Unicode character is alphabetic.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is alphabetic,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharAlphaWrapW(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_ALPHA) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.26]
 *
 * Determine if a Unicode character is upper-case.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is upper-case,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharUpperWrapW(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_UPPER) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.27]
 *
 * Determine if a Unicode character is lower-case.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is lower-case,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharLowerWrapW(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_LOWER) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.28]
 *
 * Determine if a Unicode character is alphabetic or a digit.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is alphabetic or a digit,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharAlphaNumericWrapW(WCHAR wc)
{
    return (get_char_typeW(wc) & (C1_ALPHA|C1_DIGIT)) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.29]
 *
 * Determine if a Unicode character is a space.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is a space,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharSpaceW(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_SPACE) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.30]
 *
 * Determine if a Unicode character is a blank.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is a blank,
 *  FALSE otherwise.
 *
 */
BOOL WINAPI IsCharBlankW(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_BLANK) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.31]
 *
 * Determine if a Unicode character is punctuation.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is punctuation,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharPunctW(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_PUNCT) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.32]
 *
 * Determine if a Unicode character is a control character.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is a control character,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharCntrlW(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_CNTRL) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.33]
 *
 * Determine if a Unicode character is a digit.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is a digit,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharDigitW(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_DIGIT) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.34]
 *
 * Determine if a Unicode character is a hex digit.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is a hex digit,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharXDigitW(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_XDIGIT) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.35]
 *
 */
BOOL WINAPI GetStringType3ExW(LPWSTR lpszStr, DWORD dwLen, LPVOID p3)
{
    FIXME("(%s,0x%08lx,%p): stub\n", debugstr_w(lpszStr), dwLen, p3);
    return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.36]
 *
 * Insert a bitmap menu item at the bottom of a menu.
 *
 * PARAMS
 *  hMenu [I] Menu to insert into
 *  flags [I] Flags for insertion
 *  id    [I] Menu ID of the item
 *  str   [I] Menu text for the item
 *
 * RETURNS
 *  Success: TRUE,  the item is inserted into the menu
 *  Failure: FALSE, if any parameter is invalid
 */
BOOL WINAPI AppendMenuWrapW(HMENU hMenu, UINT flags, UINT id, LPCWSTR str)
{
    TRACE("(%p,0x%08x,0x%08x,%s)\n",hMenu, flags, id, debugstr_w(str));
    return InsertMenuW(hMenu, -1, flags | MF_BITMAP, id, str);
}

/*************************************************************************
 *      @	[SHLWAPI.74]
 *
 * Get the text from a given dialog item.
 *
 * PARAMS
 *  hWnd     [I] Handle of dialog
 *  nItem    [I] Index of item
 *  lpsDest  [O] Buffer for receiving window text
 *  nDestLen [I] Length of buffer.
 *
 * RETURNS
 *  Success: The length of the returned text.
 *  Failure: 0.
 */
INT WINAPI GetDlgItemTextWrapW(HWND hWnd, INT nItem, LPWSTR lpsDest,INT nDestLen)
{
  HWND hItem = GetDlgItem(hWnd, nItem);

  if (hItem)
    return GetWindowTextW(hItem, lpsDest, nDestLen);
  if (nDestLen)
    *lpsDest = (WCHAR)'\0';
  return 0;
}

/*************************************************************************
 *      @   [SHLWAPI.138]
 *
 * Set the text of a given dialog item.
 *
 * PARAMS
 *  hWnd     [I] Handle of dialog
 *  iItem    [I] Index of item
 *  lpszText [O] Text to set
 *
 * RETURNS
 *  Success: TRUE.  The text of the dialog is set to lpszText.
 *  Failure: FALSE, Otherwise.
 */
BOOL WINAPI SetDlgItemTextWrapW(HWND hWnd, INT iItem, LPCWSTR lpszText)
{
    HWND hWndItem = GetDlgItem(hWnd, iItem);
    if (hWndItem)
        return SetWindowTextW(hWndItem, lpszText);
    return FALSE;
}

/*************************************************************************
 *      @	[SHLWAPI.151]
 *
 * Compare two Ascii strings up to a given length.
 *
 * PARAMS
 *  lpszSrc [I] Source string
 *  lpszCmp [I] String to compare to lpszSrc
 *  len     [I] Maximum length
 *
 * RETURNS
 *  A number greater than, less than or equal to 0 depending on whether
 *  lpszSrc is greater than, less than or equal to lpszCmp.
 */
DWORD WINAPI StrCmpNCA(LPCSTR lpszSrc, LPCSTR lpszCmp, INT len)
{
    return strncmp(lpszSrc, lpszCmp, len);
}

/*************************************************************************
 *      @	[SHLWAPI.152]
 *
 * Unicode version of StrCmpNCA.
 */
DWORD WINAPI StrCmpNCW(LPCWSTR lpszSrc, LPCWSTR lpszCmp, INT len)
{
    return strncmpW(lpszSrc, lpszCmp, len);
}

/*************************************************************************
 *      @	[SHLWAPI.153]
 *
 * Compare two Ascii strings up to a given length, ignoring case.
 *
 * PARAMS
 *  lpszSrc [I] Source string
 *  lpszCmp [I] String to compare to lpszSrc
 *  len     [I] Maximum length
 *
 * RETURNS
 *  A number greater than, less than or equal to 0 depending on whether
 *  lpszSrc is greater than, less than or equal to lpszCmp.
 */
DWORD WINAPI StrCmpNICA(LPCSTR lpszSrc, LPCSTR lpszCmp, DWORD len)
{
    return strncasecmp(lpszSrc, lpszCmp, len);
}

/*************************************************************************
 *      @	[SHLWAPI.154]
 *
 * Unicode version of StrCmpNICA.
 */
DWORD WINAPI StrCmpNICW(LPCWSTR lpszSrc, LPCWSTR lpszCmp, DWORD len)
{
    return strncmpiW(lpszSrc, lpszCmp, len);
}

/*************************************************************************
 *      @	[SHLWAPI.155]
 *
 * Compare two Ascii strings.
 *
 * PARAMS
 *  lpszSrc [I] Source string
 *  lpszCmp [I] String to compare to lpszSrc
 *
 * RETURNS
 *  A number greater than, less than or equal to 0 depending on whether
 *  lpszSrc is greater than, less than or equal to lpszCmp.
 */
DWORD WINAPI StrCmpCA(LPCSTR lpszSrc, LPCSTR lpszCmp)
{
    return strcmp(lpszSrc, lpszCmp);
}

/*************************************************************************
 *      @	[SHLWAPI.156]
 *
 * Unicode version of StrCmpCA.
 */
DWORD WINAPI StrCmpCW(LPCWSTR lpszSrc, LPCWSTR lpszCmp)
{
    return strcmpW(lpszSrc, lpszCmp);
}

/*************************************************************************
 *      @	[SHLWAPI.157]
 *
 * Compare two Ascii strings, ignoring case.
 *
 * PARAMS
 *  lpszSrc [I] Source string
 *  lpszCmp [I] String to compare to lpszSrc
 *
 * RETURNS
 *  A number greater than, less than or equal to 0 depending on whether
 *  lpszSrc is greater than, less than or equal to lpszCmp.
 */
DWORD WINAPI StrCmpICA(LPCSTR lpszSrc, LPCSTR lpszCmp)
{
    return strcasecmp(lpszSrc, lpszCmp);
}

/*************************************************************************
 *      @	[SHLWAPI.158]
 *
 * Unicode version of StrCmpICA.
 */
DWORD WINAPI StrCmpICW(LPCWSTR lpszSrc, LPCWSTR lpszCmp)
{
    return strcmpiW(lpszSrc, lpszCmp);
}

/*************************************************************************
 *      @	[SHLWAPI.160]
 *
 * Get an identification string for the OS and explorer.
 *
 * PARAMS
 *  lpszDest  [O] Destination for Id string
 *  dwDestLen [I] Length of lpszDest
 *
 * RETURNS
 *  TRUE,  If the string was created successfully
 *  FALSE, Otherwise
 */
BOOL WINAPI SHAboutInfoA(LPSTR lpszDest, DWORD dwDestLen)
{
  WCHAR buff[2084];

  TRACE("(%p,%ld)\n", lpszDest, dwDestLen);

  if (lpszDest && SHAboutInfoW(buff, dwDestLen))
  {
    WideCharToMultiByte(CP_ACP, 0, buff, -1, lpszDest, dwDestLen, NULL, NULL);
    return TRUE;
  }
  return FALSE;
}

/*************************************************************************
 *      @	[SHLWAPI.161]
 *
 * Unicode version of SHAboutInfoA.
 */
BOOL WINAPI SHAboutInfoW(LPWSTR lpszDest, DWORD dwDestLen)
{
  static const WCHAR szIEKey[] = { 'S','O','F','T','W','A','R','E','\\',
    'M','i','c','r','o','s','o','f','t','\\','I','n','t','e','r','n','e','t',
    ' ','E','x','p','l','o','r','e','r','\0' };
  static const WCHAR szWinNtKey[] = { 'S','O','F','T','W','A','R','E','\\',
    'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s',' ',
    'N','T','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\0' };
  static const WCHAR szWinKey[] = { 'S','O','F','T','W','A','R','E','\\',
    'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\0' };
  static const WCHAR szRegKey[] = { 'S','O','F','T','W','A','R','E','\\',
    'M','i','c','r','o','s','o','f','t','\\','I','n','t','e','r','n','e','t',
    ' ','E','x','p','l','o','r','e','r','\\',
    'R','e','g','i','s','t','r','a','t','i','o','n','\0' };
  static const WCHAR szVersion[] = { 'V','e','r','s','i','o','n','\0' };
  static const WCHAR szCustomized[] = { 'C','u','s','t','o','m','i','z','e','d',
    'V','e','r','s','i','o','n','\0' };
  static const WCHAR szOwner[] = { 'R','e','g','i','s','t','e','r','e','d',
    'O','w','n','e','r','\0' };
  static const WCHAR szOrg[] = { 'R','e','g','i','s','t','e','r','e','d',
    'O','r','g','a','n','i','z','a','t','i','o','n','\0' };
  static const WCHAR szProduct[] = { 'P','r','o','d','u','c','t','I','d','\0' };
  static const WCHAR szUpdate[] = { 'I','E','A','K',
    'U','p','d','a','t','e','U','r','l','\0' };
  static const WCHAR szHelp[] = { 'I','E','A','K',
    'H','e','l','p','S','t','r','i','n','g','\0' };
  WCHAR buff[2084];
  HKEY hReg;
  DWORD dwType, dwLen;

  TRACE("(%p,%ld)\n", lpszDest, dwDestLen);

  if (!lpszDest)
    return FALSE;

  *lpszDest = '\0';

  /* Try the NT key first, followed by 95/98 key */
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szWinNtKey, 0, KEY_READ, &hReg) &&
      RegOpenKeyExW(HKEY_LOCAL_MACHINE, szWinKey, 0, KEY_READ, &hReg))
    return FALSE;

  /* OS Version */
  buff[0] = '\0';
  dwLen = 30;
  if (!SHGetValueW(HKEY_LOCAL_MACHINE, szIEKey, szVersion, &dwType, buff, &dwLen))
  {
    DWORD dwStrLen = strlenW(buff);
    dwLen = 30 - dwStrLen;
    SHGetValueW(HKEY_LOCAL_MACHINE, szIEKey,
                szCustomized, &dwType, buff+dwStrLen, &dwLen);
  }
  StrCatBuffW(lpszDest, buff, dwDestLen);

  /* ~Registered Owner */
  buff[0] = '~';
  dwLen = 256;
  if (SHGetValueW(hReg, szOwner, 0, &dwType, buff+1, &dwLen))
    buff[1] = '\0';
  StrCatBuffW(lpszDest, buff, dwDestLen);

  /* ~Registered Organization */
  dwLen = 256;
  if (SHGetValueW(hReg, szOrg, 0, &dwType, buff+1, &dwLen))
    buff[1] = '\0';
  StrCatBuffW(lpszDest, buff, dwDestLen);

  /* FIXME: Not sure where this number comes from  */
  buff[0] = '~';
  buff[1] = '0';
  buff[2] = '\0';
  StrCatBuffW(lpszDest, buff, dwDestLen);

  /* ~Product Id */
  dwLen = 256;
  if (SHGetValueW(HKEY_LOCAL_MACHINE, szRegKey, szProduct, &dwType, buff+1, &dwLen))
    buff[1] = '\0';
  StrCatBuffW(lpszDest, buff, dwDestLen);

  /* ~IE Update Url */
  dwLen = 2048;
  if(SHGetValueW(HKEY_LOCAL_MACHINE, szWinKey, szUpdate, &dwType, buff+1, &dwLen))
    buff[1] = '\0';
  StrCatBuffW(lpszDest, buff, dwDestLen);

  /* ~IE Help String */
  dwLen = 256;
  if(SHGetValueW(hReg, szHelp, 0, &dwType, buff+1, &dwLen))
    buff[1] = '\0';
  StrCatBuffW(lpszDest, buff, dwDestLen);

  RegCloseKey(hReg);
  return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.163]
 *
 * Call IOleCommandTarget_QueryStatus() on an object.
 *
 * PARAMS
 *  lpUnknown     [I] Object supporting the IOleCommandTarget interface
 *  pguidCmdGroup [I] GUID for the command group
 *  cCmds         [I]
 *  prgCmds       [O] Commands
 *  pCmdText      [O] Command text
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_FAIL, if lpUnknown is NULL.
 *           E_NOINTERFACE, if lpUnknown does not support IOleCommandTarget.
 *           Otherwise, an error code from IOleCommandTarget_QueryStatus().
 */
HRESULT WINAPI IUnknown_QueryStatus(IUnknown* lpUnknown, REFGUID pguidCmdGroup,
                           ULONG cCmds, OLECMD *prgCmds, OLECMDTEXT* pCmdText)
{
  HRESULT hRet = E_FAIL;

  TRACE("(%p,%p,%ld,%p,%p)\n",lpUnknown, pguidCmdGroup, cCmds, prgCmds, pCmdText);

  if (lpUnknown)
  {
    IOleCommandTarget* lpOle;

    hRet = IUnknown_QueryInterface(lpUnknown, &IID_IOleCommandTarget,
                                   (void**)&lpOle);

    if (SUCCEEDED(hRet) && lpOle)
    {
      hRet = IOleCommandTarget_QueryStatus(lpOle, pguidCmdGroup, cCmds,
                                           prgCmds, pCmdText);
      IOleCommandTarget_Release(lpOle);
    }
  }
  return hRet;
}

/*************************************************************************
 *      @		[SHLWAPI.164]
 *
 * Call IOleCommandTarget_Exec() on an object.
 *
 * PARAMS
 *  lpUnknown     [I] Object supporting the IOleCommandTarget interface
 *  pguidCmdGroup [I] GUID for the command group
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_FAIL, if lpUnknown is NULL.
 *           E_NOINTERFACE, if lpUnknown does not support IOleCommandTarget.
 *           Otherwise, an error code from IOleCommandTarget_Exec().
 */
HRESULT WINAPI IUnknown_Exec(IUnknown* lpUnknown, REFGUID pguidCmdGroup,
                           DWORD nCmdID, DWORD nCmdexecopt, VARIANT* pvaIn,
                           VARIANT* pvaOut)
{
  HRESULT hRet = E_FAIL;

  TRACE("(%p,%p,%ld,%ld,%p,%p)\n",lpUnknown, pguidCmdGroup, nCmdID,
        nCmdexecopt, pvaIn, pvaOut);

  if (lpUnknown)
  {
    IOleCommandTarget* lpOle;

    hRet = IUnknown_QueryInterface(lpUnknown, &IID_IOleCommandTarget,
                                   (void**)&lpOle);
    if (SUCCEEDED(hRet) && lpOle)
    {
      hRet = IOleCommandTarget_Exec(lpOle, pguidCmdGroup, nCmdID,
                                    nCmdexecopt, pvaIn, pvaOut);
      IOleCommandTarget_Release(lpOle);
    }
  }
  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.165]
 *
 * Retrieve, modify, and re-set a value from a window.
 *
 * PARAMS
 *  hWnd   [I] Window to get value from
 *  offset [I] Offset of value
 *  wMask  [I] Mask for uiFlags
 *  wFlags [I] Bits to set in window value
 *
 * RETURNS
 *  The new value as it was set, or 0 if any parameter is invalid.
 *
 * NOTES
 *  Any bits set in uiMask are cleared from the value, then any bits set in
 *  uiFlags are set in the value.
 */
LONG WINAPI SHSetWindowBits(HWND hwnd, INT offset, UINT wMask, UINT wFlags)
{
  LONG ret = GetWindowLongA(hwnd, offset);
  LONG newFlags = (wFlags & wMask) | (ret & ~wFlags);

  if (newFlags != ret)
    ret = SetWindowLongA(hwnd, offset, newFlags);
  return ret;
}

/*************************************************************************
 *      @	[SHLWAPI.167]
 *
 * Change a window's parent.
 *
 * PARAMS
 *  hWnd       [I] Window to change parent of
 *  hWndParent [I] New parent window
 *
 * RETURNS
 *  The old parent of hWnd.
 *
 * NOTES
 *  If hWndParent is NULL (desktop), the window style is changed to WS_POPUP.
 *  If hWndParent is NOT NULL then we set the WS_CHILD style.
 */
HWND WINAPI SHSetParentHwnd(HWND hWnd, HWND hWndParent)
{
  TRACE("%p, %p\n", hWnd, hWndParent);

  if(GetParent(hWnd) == hWndParent)
    return 0;

  if(hWndParent)
    SHSetWindowBits(hWnd, GWL_STYLE, WS_CHILD, WS_CHILD);
  else
    SHSetWindowBits(hWnd, GWL_STYLE, WS_POPUP, WS_POPUP);

  return SetParent(hWnd, hWndParent);
}

/*************************************************************************
 *      @       [SHLWAPI.168]
 *
 * Locate and advise a connection point in an IConnectionPointContainer object.
 *
 * PARAMS
 *  lpUnkSink   [I] Sink for the connection point advise call
 *  riid        [I] REFIID of connection point to advise
 *  bAdviseOnly [I] TRUE = Advise only, FALSE = Unadvise first
 *  lpUnknown   [I] Object supporting the IConnectionPointContainer interface
 *  lpCookie    [O] Pointer to connection point cookie
 *  lppCP       [O] Destination for the IConnectionPoint found
 *
 * RETURNS
 *  Success: S_OK. If lppCP is non-NULL, it is filled with the IConnectionPoint
 *           that was advised. The caller is responsable for releasing it.
 *  Failure: E_FAIL, if any arguments are invalid.
 *           E_NOINTERFACE, if lpUnknown isn't an IConnectionPointContainer,
 *           Or an HRESULT error code if any call fails.
 */
HRESULT WINAPI ConnectToConnectionPoint(IUnknown* lpUnkSink, REFIID riid, BOOL bAdviseOnly,
                           IUnknown* lpUnknown, LPDWORD lpCookie,
                           IConnectionPoint **lppCP)
{
  HRESULT hRet;
  IConnectionPointContainer* lpContainer;
  IConnectionPoint *lpCP;

  if(!lpUnknown || (bAdviseOnly && !lpUnkSink))
    return E_FAIL;

  if(lppCP)
    *lppCP = NULL;

  hRet = IUnknown_QueryInterface(lpUnknown, &IID_IConnectionPointContainer,
                                 (void**)&lpContainer);
  if (SUCCEEDED(hRet))
  {
    hRet = IConnectionPointContainer_FindConnectionPoint(lpContainer, riid, &lpCP);

    if (SUCCEEDED(hRet))
    {
      if(!bAdviseOnly)
        hRet = IConnectionPoint_Unadvise(lpCP, *lpCookie);
      hRet = IConnectionPoint_Advise(lpCP, lpUnkSink, lpCookie);

      if (FAILED(hRet))
        *lpCookie = 0;

      if (lppCP && SUCCEEDED(hRet))
        *lppCP = lpCP; /* Caller keeps the interface */
      else
        IConnectionPoint_Release(lpCP); /* Release it */
    }

    IUnknown_Release(lpContainer);
  }
  return hRet;
}

/*************************************************************************
 *	@	[SHLWAPI.169]
 *
 * Release an interface.
 *
 * PARAMS
 *  lpUnknown [I] Object to release
 *
 * RETURNS
 *  Nothing.
 */
DWORD WINAPI IUnknown_AtomicRelease(IUnknown ** lpUnknown)
{
    IUnknown *temp;

    TRACE("(%p)\n",lpUnknown);

    if(!lpUnknown || !*((LPDWORD)lpUnknown)) return 0;
    temp = *lpUnknown;
    *lpUnknown = NULL;

    TRACE("doing Release\n");
    
    return IUnknown_Release(temp);
}

/*************************************************************************
 *      @	[SHLWAPI.170]
 *
 * Skip '//' if present in a string.
 *
 * PARAMS
 *  lpszSrc [I] String to check for '//'
 *
 * RETURNS
 *  Success: The next character after the '//' or the string if not present
 *  Failure: NULL, if lpszStr is NULL.
 */
LPCSTR WINAPI PathSkipLeadingSlashesA(LPCSTR lpszSrc)
{
  if (lpszSrc && lpszSrc[0] == '/' && lpszSrc[1] == '/')
    lpszSrc += 2;
  return lpszSrc;
}

/*************************************************************************
 *      @		[SHLWAPI.171]
 *
 * Check if two interfaces come from the same object.
 *
 * PARAMS
 *   lpInt1 [I] Interface to check against lpInt2.
 *   lpInt2 [I] Interface to check against lpInt1.
 *
 * RETURNS
 *   TRUE, If the interfaces come from the same object.
 *   FALSE Otherwise.
 */
BOOL WINAPI SHIsSameObject(IUnknown* lpInt1, IUnknown* lpInt2)
{
  LPVOID lpUnknown1, lpUnknown2;

  TRACE("%p %p\n", lpInt1, lpInt2);

  if (!lpInt1 || !lpInt2)
    return FALSE;

  if (lpInt1 == lpInt2)
    return TRUE;

  if (!SUCCEEDED(IUnknown_QueryInterface(lpInt1, &IID_IUnknown,
                                       (LPVOID *)&lpUnknown1)))
    return FALSE;

  if (!SUCCEEDED(IUnknown_QueryInterface(lpInt2, &IID_IUnknown,
                                       (LPVOID *)&lpUnknown2)))
    return FALSE;

  if (lpUnknown1 == lpUnknown2)
    return TRUE;
  
  return FALSE;
}

/*************************************************************************
 *      @	[SHLWAPI.172]
 *
 * Get the window handle of an object.
 *
 * PARAMS
 *  lpUnknown [I] Object to get the window handle of
 *  lphWnd    [O] Destination for window handle
 *
 * RETURNS
 *  Success: S_OK. lphWnd contains the objects window handle.
 *  Failure: An HRESULT error code.
 *
 * NOTES
 *  lpUnknown is expected to support one of the following interfaces:
 *  IOleWindow(), IInternetSecurityMgrSite(), or IShellView().
 */
HRESULT WINAPI IUnknown_GetWindow(IUnknown *lpUnknown, HWND *lphWnd)
{
  /* FIXME: Wine has no header for this object */
  static const GUID IID_IInternetSecurityMgrSite = { 0x79eac9ed,
    0xbaf9, 0x11ce, { 0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b }};
  IUnknown *lpOle;
  HRESULT hRet = E_FAIL;

  TRACE("(%p,%p)\n", lpUnknown, lphWnd);

  if (!lpUnknown)
    return hRet;

  hRet = IUnknown_QueryInterface(lpUnknown, &IID_IOleWindow, (void**)&lpOle);

  if (FAILED(hRet))
  {
    hRet = IUnknown_QueryInterface(lpUnknown,&IID_IShellView, (void**)&lpOle);

    if (FAILED(hRet))
    {
      hRet = IUnknown_QueryInterface(lpUnknown, &IID_IInternetSecurityMgrSite,
                                      (void**)&lpOle);
    }
  }

  if (SUCCEEDED(hRet))
  {
    /* Lazyness here - Since GetWindow() is the first method for the above 3
     * interfaces, we use the same call for them all.
     */
    hRet = IOleWindow_GetWindow((IOleWindow*)lpOle, lphWnd);
    IUnknown_Release(lpOle);
    if (lphWnd)
      TRACE("Returning HWND=%p\n", *lphWnd);
  }

  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.173]
 *
 * Call a method on as as yet unidentified object.
 *
 * PARAMS
 *  pUnk [I] Object supporting the unidentified interface,
 *  arg  [I] Argument for the call on the object.
 *
 * RETURNS
 *  S_OK.
 */
HRESULT WINAPI IUnknown_SetOwner(IUnknown *pUnk, ULONG arg)
{
  static const GUID guid_173 = {
    0x5836fb00, 0x8187, 0x11cf, { 0xa1,0x2b,0x00,0xaa,0x00,0x4a,0xe8,0x37 }
  };
  IMalloc *pUnk2;

  TRACE("(%p,%ld)\n", pUnk, arg);

  /* Note: arg may not be a ULONG and pUnk2 is for sure not an IMalloc -
   *       We use this interface as its vtable entry is compatible with the
   *       object in question.
   * FIXME: Find out what this object is and where it should be defined.
   */
  if (pUnk &&
      SUCCEEDED(IUnknown_QueryInterface(pUnk, &guid_173, (void**)&pUnk2)))
  {
    IMalloc_Alloc(pUnk2, arg); /* Faked call!! */
    IMalloc_Release(pUnk2);
  }
  return S_OK;
}

/*************************************************************************
 *      @	[SHLWAPI.174]
 *
 * Call either IObjectWithSite_SetSite() or IPersistMoniker_GetClassID() on
 * an interface.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_FAIL, if p1 is NULL.
 *           E_NOINTERFACE If p1 does not support the IPersist interface,
 *           Or an HRESULT error code.
 */
DWORD WINAPI IUnknown_SetSite(
        IUnknown *p1,     /* [in]   OLE object                          */
        LPVOID *p2)       /* [out]  ptr for call results */
{
    DWORD ret, aa;

    if (!p1) return E_FAIL;

    /* see if SetSite interface exists for IObjectWithSite object */
    ret = IUnknown_QueryInterface((IUnknown *)p1, (REFIID)id1, (LPVOID *)&p1);
    TRACE("first IU_QI ret=%08lx, p1=%p\n", ret, p1);
    if (ret) {

	/* see if GetClassId interface exists for IPersistMoniker object */
	ret = IUnknown_QueryInterface((IUnknown *)p1, (REFIID)id2, (LPVOID *)&aa);
	TRACE("second IU_QI ret=%08lx, aa=%08lx\n", ret, aa);
	if (ret) return ret;

	/* fake a GetClassId call */
	ret = IOleWindow_GetWindow((IOleWindow *)aa, (HWND*)p2);
	TRACE("second IU_QI doing 0x0c ret=%08lx, *p2=%08lx\n", ret,
	      *(LPDWORD)p2);
	IUnknown_Release((IUnknown *)aa);
    }
    else {
	/* fake a SetSite call */
	ret = IOleWindow_GetWindow((IOleWindow *)p1, (HWND*)p2);
	TRACE("first IU_QI doing 0x0c ret=%08lx, *p2=%08lx\n", ret,
	      *(LPDWORD)p2);
	IUnknown_Release((IUnknown *)p1);
    }
    return ret;
}

/*************************************************************************
 *      @	[SHLWAPI.175]
 *
 * Call IPersist_GetClassID() on an object.
 *
 * PARAMS
 *  lpUnknown [I] Object supporting the IPersist interface
 *  lpClassId [O] Destination for Class Id
 *
 * RETURNS
 *  Success: S_OK. lpClassId contains the Class Id requested.
 *  Failure: E_FAIL, If lpUnknown is NULL,
 *           E_NOINTERFACE If lpUnknown does not support IPersist,
 *           Or an HRESULT error code.
 */
HRESULT WINAPI IUnknown_GetClassID(IUnknown *lpUnknown, CLSID* lpClassId)
{
  IPersist* lpPersist;
  HRESULT hRet = E_FAIL;

  TRACE("(%p,%p)\n", lpUnknown, debugstr_guid(lpClassId));

  if (lpUnknown)
  {
    hRet = IUnknown_QueryInterface(lpUnknown,&IID_IPersist,(void**)&lpPersist);
    if (SUCCEEDED(hRet))
    {
      IPersist_GetClassID(lpPersist, lpClassId);
      IPersist_Release(lpPersist);
    }
  }
  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.176]
 *
 * Retrieve a Service Interface from an object.
 *
 * PARAMS
 *  lpUnknown [I] Object to get an IServiceProvider interface from
 *  sid       [I] Service ID for IServiceProvider_QueryService() call
 *  riid      [I] Function requested for QueryService call
 *  lppOut    [O] Destination for the service interface pointer
 *
 * RETURNS
 *  Success: S_OK. lppOut contains an object providing the requested service
 *  Failure: An HRESULT error code
 *
 * NOTES
 *  lpUnknown is expected to support the IServiceProvider interface.
 */
HRESULT WINAPI IUnknown_QueryService(IUnknown* lpUnknown, REFGUID sid, REFIID riid,
                           LPVOID *lppOut)
{
  IServiceProvider* pService = NULL;
  HRESULT hRet;

  if (!lppOut)
    return E_FAIL;

  *lppOut = NULL;

  if (!lpUnknown)
    return E_FAIL;

  /* Get an IServiceProvider interface from the object */
  hRet = IUnknown_QueryInterface(lpUnknown, &IID_IServiceProvider,
                                 (LPVOID*)&pService);

  if (!hRet && pService)
  {
    TRACE("QueryInterface returned (IServiceProvider*)%p\n", pService);

    /* Get a Service interface from the object */
    hRet = IServiceProvider_QueryService(pService, sid, riid, lppOut);

    TRACE("(IServiceProvider*)%p returned (IUnknown*)%p\n", pService, *lppOut);

    /* Release the IServiceProvider interface */
    IUnknown_Release(pService);
  }
  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.177]
 *
 * Loads a popup menu.
 *
 * PARAMS
 *  hInst  [I] Instance handle
 *  szName [I] Menu name
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI SHLoadMenuPopup(HINSTANCE hInst, LPCWSTR szName)
{
  HMENU hMenu, hSubMenu;

  if ((hMenu = LoadMenuW(hInst, szName)))
  {
    if ((hSubMenu = GetSubMenu(hMenu, 0)))
      RemoveMenu(hMenu, 0, MF_BYPOSITION);

    DestroyMenu(hMenu);
    return TRUE;
  }
  return FALSE;
}

typedef struct _enumWndData
{
  UINT   uiMsgId;
  WPARAM wParam;
  LPARAM lParam;
  LRESULT (WINAPI *pfnPost)(HWND,UINT,WPARAM,LPARAM);
} enumWndData;

/* Callback for SHLWAPI_178 */
static BOOL CALLBACK SHLWAPI_EnumChildProc(HWND hWnd, LPARAM lParam)
{
  enumWndData *data = (enumWndData *)lParam;

  TRACE("(%p,%p)\n", hWnd, data);
  data->pfnPost(hWnd, data->uiMsgId, data->wParam, data->lParam);
  return TRUE;
}

/*************************************************************************
 * @  [SHLWAPI.178]
 *
 * Send or post a message to every child of a window.
 *
 * PARAMS
 *  hWnd    [I] Window whose children will get the messages
 *  uiMsgId [I] Message Id
 *  wParam  [I] WPARAM of message
 *  lParam  [I] LPARAM of message
 *  bSend   [I] TRUE = Use SendMessageA(), FALSE = Use PostMessageA()
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  The appropriate ASCII or Unicode function is called for the window.
 */
void WINAPI SHPropagateMessage(HWND hWnd, UINT uiMsgId, WPARAM wParam, LPARAM lParam, BOOL bSend)
{
  enumWndData data;

  TRACE("(%p,%u,%d,%ld,%d)\n", hWnd, uiMsgId, wParam, lParam, bSend);

  if(hWnd)
  {
    data.uiMsgId = uiMsgId;
    data.wParam  = wParam;
    data.lParam  = lParam;

    if (bSend)
      data.pfnPost = IsWindowUnicode(hWnd) ? (void*)SendMessageW : (void*)SendMessageA;
    else
      data.pfnPost = IsWindowUnicode(hWnd) ? (void*)PostMessageW : (void*)PostMessageA;

    EnumChildWindows(hWnd, SHLWAPI_EnumChildProc, (LPARAM)&data);
  }
}

/*************************************************************************
 *      @	[SHLWAPI.180]
 *
 * Remove all sub-menus from a menu.
 *
 * PARAMS
 *  hMenu [I] Menu to remove sub-menus from
 *
 * RETURNS
 *  Success: 0.  All sub-menus under hMenu are removed
 *  Failure: -1, if any parameter is invalid
 */
DWORD WINAPI SHRemoveAllSubMenus(HMENU hMenu)
{
  int iItemCount = GetMenuItemCount(hMenu) - 1;
  while (iItemCount >= 0)
  {
    HMENU hSubMenu = GetSubMenu(hMenu, iItemCount);
    if (hSubMenu)
      RemoveMenu(hMenu, iItemCount, MF_BYPOSITION);
    iItemCount--;
  }
  return iItemCount;
}

/*************************************************************************
 *      @	[SHLWAPI.181]
 *
 * Enable or disable a menu item.
 *
 * PARAMS
 *  hMenu   [I] Menu holding menu item
 *  uID     [I] ID of menu item to enable/disable
 *  bEnable [I] Whether to enable (TRUE) or disable (FALSE) the item.
 *
 * RETURNS
 *  The return code from EnableMenuItem.
 */
UINT WINAPI SHEnableMenuItem(HMENU hMenu, UINT wItemID, BOOL bEnable)
{
  return EnableMenuItem(hMenu, wItemID, bEnable ? MF_ENABLED : MF_GRAYED);
}

/*************************************************************************
 * @	[SHLWAPI.182]
 *
 * Check or uncheck a menu item.
 *
 * PARAMS
 *  hMenu  [I] Menu holding menu item
 *  uID    [I] ID of menu item to check/uncheck
 *  bCheck [I] Whether to check (TRUE) or uncheck (FALSE) the item.
 *
 * RETURNS
 *  The return code from CheckMenuItem.
 */
DWORD WINAPI SHCheckMenuItem(HMENU hMenu, UINT uID, BOOL bCheck)
{
  return CheckMenuItem(hMenu, uID, bCheck ? MF_CHECKED : MF_UNCHECKED);
}

/*************************************************************************
 *      @	[SHLWAPI.183]
 *
 * Register a window class if it isn't already.
 *
 * PARAMS
 *  lpWndClass [I] Window class to register
 *
 * RETURNS
 *  The result of the RegisterClassA call.
 */
DWORD WINAPI SHRegisterClassA(WNDCLASSA *wndclass)
{
  WNDCLASSA wca;
  if (GetClassInfoA(wndclass->hInstance, wndclass->lpszClassName, &wca))
    return TRUE;
  return (DWORD)RegisterClassA(wndclass);
}

/*************************************************************************
 *      @	[SHLWAPI.187]
 *
 * Call IPersistPropertyBag_Load() on an object.
 *
 * PARAMS
 *  lpUnknown [I] Object supporting the IPersistPropertyBag interface
 *  lpPropBag [O] Destination for loaded IPropertyBag
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code, or E_FAIL if lpUnknown is NULL.
 */
DWORD WINAPI SHLoadFromPropertyBag(IUnknown *lpUnknown, IPropertyBag* lpPropBag)
{
  IPersistPropertyBag* lpPPBag;
  HRESULT hRet = E_FAIL;

  TRACE("(%p,%p)\n", lpUnknown, lpPropBag);

  if (lpUnknown)
  {
    hRet = IUnknown_QueryInterface(lpUnknown, &IID_IPersistPropertyBag,
                                   (void**)&lpPPBag);
    if (SUCCEEDED(hRet) && lpPPBag)
    {
      hRet = IPersistPropertyBag_Load(lpPPBag, lpPropBag, NULL);
      IPersistPropertyBag_Release(lpPPBag);
    }
  }
  return hRet;
}

/*************************************************************************
 * @  [SHLWAPI.189]
 *
 * Call IOleControlSite_GetExtendedControl() on an object.
 *
 * PARAMS
 *  lpUnknown [I] Object supporting the IOleControlSite interface
 *  lppDisp   [O] Destination for resulting IDispatch.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code, or E_FAIL if lpUnknown is NULL.
 */
DWORD WINAPI IUnknown_OnFocusOCS(IUnknown *lpUnknown, IDispatch** lppDisp)
{
  IOleControlSite* lpCSite;
  HRESULT hRet = E_FAIL;

  TRACE("(%p,%p)\n", lpUnknown, lppDisp);
  if (lpUnknown)
  {
    hRet = IUnknown_QueryInterface(lpUnknown, &IID_IOleControlSite,
                                   (void**)&lpCSite);
    if (SUCCEEDED(hRet) && lpCSite)
    {
      hRet = IOleControlSite_GetExtendedControl(lpCSite, lppDisp);
      IOleControlSite_Release(lpCSite);
    }
  }
  return hRet;
}

/*************************************************************************
 * @    [SHLWAPI.192]
 *
 * Get a sub-menu from a menu item.
 *
 * PARAMS
 *  hMenu [I] Menu to get sub-menu from
 *  uID   [I] ID of menu item containing sub-menu
 *
 * RETURNS
 *  The sub-menu of the item, or a NULL handle if any parameters are invalid.
 */
HMENU WINAPI SHGetMenuFromID(HMENU hMenu, UINT uID)
{
  MENUITEMINFOA mi;

  TRACE("(%p,%uld)\n", hMenu, uID);

  mi.cbSize = sizeof(MENUITEMINFOA);
  mi.fMask = MIIM_SUBMENU;

  if (!GetMenuItemInfoA(hMenu, uID, 0, &mi))
    return (HMENU)NULL;

  return mi.hSubMenu;
}

/*************************************************************************
 *      @	[SHLWAPI.193]
 *
 * Get the color depth of the primary display.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The color depth of the primary display.
 */
DWORD WINAPI SHGetCurColorRes()
{
    HDC hdc;
    DWORD ret;

    TRACE("()\n");

    hdc = GetDC(0);
    ret = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
    ReleaseDC(0, hdc);
    return ret;
}

/*************************************************************************
 *      @	[SHLWAPI.194]
 *
 * Wait for a message to arrive, with a timeout.
 *
 * PARAMS
 *  hand      [I] Handle to query
 *  dwTimeout [I] Timeout in ticks or INFINITE to never timeout
 *
 * RETURNS
 *  STATUS_TIMEOUT if no message is received before dwTimeout ticks passes.
 *  Otherwise returns the value from MsgWaitForMultipleObjectsEx when a
 *  message is available.
 */
DWORD WINAPI SHWaitForSendMessageThread(HANDLE hand, DWORD dwTimeout)
{
  DWORD dwEndTicks = GetTickCount() + dwTimeout;
  DWORD dwRet;

  while ((dwRet = MsgWaitForMultipleObjectsEx(1, &hand, dwTimeout, QS_SENDMESSAGE, 0)) == 1)
  {
    MSG msg;

    PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);

    if (dwTimeout != INFINITE)
    {
        if ((int)(dwTimeout = dwEndTicks - GetTickCount()) <= 0)
            return WAIT_TIMEOUT;
    }
  }

  return dwRet;
}

/*************************************************************************
 *      @       [SHLWAPI.197]
 *
 * Blank out a region of text by drawing the background only.
 *
 * PARAMS
 *  hDC   [I] Device context to draw in
 *  pRect [I] Area to draw in
 *  cRef  [I] Color to draw in
 *
 * RETURNS
 *  Nothing.
 */
DWORD WINAPI SHFillRectClr(HDC hDC, LPCRECT pRect, COLORREF cRef)
{
    COLORREF cOldColor = SetBkColor(hDC, cRef);
    ExtTextOutA(hDC, 0, 0, ETO_OPAQUE, pRect, 0, 0, 0);
    SetBkColor(hDC, cOldColor);
    return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.198]
 *
 * Return the value asociated with a key in a map.
 *
 * PARAMS
 *  lpKeys   [I] A list of keys of length iLen
 *  lpValues [I] A list of values associated with lpKeys, of length iLen
 *  iLen     [I] Length of both lpKeys and lpValues
 *  iKey     [I] The key value to look up in lpKeys
 *
 * RETURNS
 *  The value in lpValues associated with iKey, or -1 if iKey is not
 *  found in lpKeys.
 *
 * NOTES
 *  - If two elements in the map share the same key, this function returns
 *    the value closest to the start of the map
 *  - The native version of this function crashes if lpKeys or lpValues is NULL.
 */
int WINAPI SHSearchMapInt(const int *lpKeys, const int *lpValues, int iLen, int iKey)
{
  if (lpKeys && lpValues)
  {
    int i = 0;

    while (i < iLen)
    {
      if (lpKeys[i] == iKey)
        return lpValues[i]; /* Found */
      i++;
    }
  }
  return -1; /* Not found */
}


/*************************************************************************
 *      @	[SHLWAPI.199]
 *
 * Copy an interface pointer
 *
 * PARAMS
 *   lppDest   [O] Destination for copy
 *   lpUnknown [I] Source for copy
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI IUnknown_Set(IUnknown **lppDest, IUnknown *lpUnknown)
{
  TRACE("(%p,%p)\n", lppDest, lpUnknown);

  if (lppDest)
    IUnknown_AtomicRelease(lppDest); /* Release existing interface */

  if (lpUnknown)
  {
    /* Copy */
    IUnknown_AddRef(lpUnknown);
    *lppDest = lpUnknown;
  }
}

/*************************************************************************
 *      @	[SHLWAPI.201]
 *
 */
HRESULT WINAPI MayExecForward(IUnknown* lpUnknown, INT iUnk, REFGUID pguidCmdGroup,
                           DWORD nCmdID, DWORD nCmdexecopt, VARIANT* pvaIn,
                           VARIANT* pvaOut)
{
  FIXME("(%p,%d,%p,%ld,%ld,%p,%p) - stub!\n", lpUnknown, iUnk, pguidCmdGroup,
        nCmdID, nCmdexecopt, pvaIn, pvaOut);
  return DRAGDROP_E_NOTREGISTERED;
}

/*************************************************************************
 *      @	[SHLWAPI.202]
 *
 */
HRESULT WINAPI IsQSForward(REFGUID pguidCmdGroup,ULONG cCmds, OLECMD *prgCmds)
{
  FIXME("(%p,%ld,%p) - stub!\n", pguidCmdGroup, cCmds, prgCmds);
  return DRAGDROP_E_NOTREGISTERED;
}

/*************************************************************************
 * @	[SHLWAPI.204]
 *
 * Determine if a window is not a child of another window.
 *
 * PARAMS
 * hParent [I] Suspected parent window
 * hChild  [I] Suspected child window
 *
 * RETURNS
 * TRUE:  If hChild is a child window of hParent
 * FALSE: If hChild is not a child window of hParent, or they are equal
 */
BOOL WINAPI SHIsChildOrSelf(HWND hParent, HWND hChild)
{
  TRACE("(%p,%p)\n", hParent, hChild);

  if (!hParent || !hChild)
    return TRUE;
  else if(hParent == hChild)
    return FALSE;
  return !IsChild(hParent, hChild);
}

/*************************************************************************
 *      @	[SHLWAPI.208]
 *
 * Some sort of memory management process.
 */
DWORD WINAPI FDSA_Initialize(
	DWORD    a,
	DWORD    b,
	LPVOID   c,
	LPVOID   d,
	DWORD    e)
{
    FIXME("(0x%08lx 0x%08lx %p %p 0x%08lx) stub\n",
	  a, b, c, d, e);
    return 1;
}

/*************************************************************************
 *      @	[SHLWAPI.209]
 *
 * Some sort of memory management process.
 */
DWORD WINAPI FDSA_Destroy(
	LPVOID   a)
{
    FIXME("(%p) stub\n",
	  a);
    return 1;
}

/*************************************************************************
 *      @	[SHLWAPI.210]
 *
 * Some sort of memory management process.
 */
DWORD WINAPI FDSA_InsertItem(
	LPVOID   a,
	DWORD    b,
	LPVOID   c)
{
    FIXME("(%p 0x%08lx %p) stub\n",
	  a, b, c);
    return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.211]
 */
DWORD WINAPI FDSA_DeleteItem(
	LPVOID   a,
	DWORD    b)
{
    FIXME("(%p 0x%08lx) stub\n",
	  a, b);
    return 1;
}

typedef struct {
    REFIID   refid;
    DWORD    indx;
} IFACE_INDEX_TBL;

/*************************************************************************
 *      @	[SHLWAPI.219]
 *
 * Call IUnknown_QueryInterface() on a table of objects.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_POINTER or E_NOINTERFACE.
 */
HRESULT WINAPI QISearch(
	LPVOID w,           /* [in]   Table of interfaces */
	IFACE_INDEX_TBL *x, /* [in]   Array of REFIIDs and indexes into the table */
	REFIID riid,        /* [in]   REFIID to get interface for */
	LPVOID *ppv)          /* [out]  Destination for interface pointer */
{
	HRESULT ret;
	IUnknown *a_vtbl;
	IFACE_INDEX_TBL *xmove;

	TRACE("(%p %p %s %p)\n", w,x,debugstr_guid(riid),ppv);
	if (ppv) {
	    xmove = x;
	    while (xmove->refid) {
		TRACE("trying (indx %ld) %s\n", xmove->indx, debugstr_guid(xmove->refid));
		if (IsEqualIID(riid, xmove->refid)) {
		    a_vtbl = (IUnknown*)(xmove->indx + (LPBYTE)w);
		    TRACE("matched, returning (%p)\n", a_vtbl);
		    *ppv = (LPVOID)a_vtbl;
		    IUnknown_AddRef(a_vtbl);
		    return S_OK;
		}
		xmove++;
	    }

	    if (IsEqualIID(riid, &IID_IUnknown)) {
		a_vtbl = (IUnknown*)(x->indx + (LPBYTE)w);
		TRACE("returning first for IUnknown (%p)\n", a_vtbl);
		*ppv = (LPVOID)a_vtbl;
		IUnknown_AddRef(a_vtbl);
		return S_OK;
	    }
	    *ppv = 0;
	    ret = E_NOINTERFACE;
	} else
	    ret = E_POINTER;

	TRACE("-- 0x%08lx\n", ret);
	return ret;
}

/*************************************************************************
 *      @	[SHLWAPI.221]
 *
 * Remove the "PropDlgFont" property from a window.
 *
 * PARAMS
 *  hWnd [I] Window to remove the property from
 *
 * RETURNS
 *  A handle to the removed property, or NULL if it did not exist.
 */
HANDLE WINAPI SHRemoveDefaultDialogFont(HWND hWnd)
{
  HANDLE hProp;

  TRACE("(%p)\n", hWnd);

  hProp = GetPropA(hWnd, "PropDlgFont");

  if(hProp)
  {
    DeleteObject(hProp);
    hProp = RemovePropA(hWnd, "PropDlgFont");
  }
  return hProp;
}

/*************************************************************************
 *      @	[SHLWAPI.236]
 *
 * Load the in-process server of a given GUID.
 *
 * PARAMS
 *  refiid [I] GUID of the server to load.
 *
 * RETURNS
 *  Success: A handle to the loaded server dll.
 *  Failure: A NULL handle.
 */
HMODULE WINAPI SHPinDllOfCLSID(REFIID refiid)
{
    HKEY newkey;
    DWORD type, count;
    CHAR value[MAX_PATH], string[MAX_PATH];

    strcpy(string, "CLSID\\");
    SHStringFromGUIDA(refiid, string + 6, sizeof(string)/sizeof(char) - 6);
    strcat(string, "\\InProcServer32");

    count = MAX_PATH;
    RegOpenKeyExA(HKEY_CLASSES_ROOT, string, 0, 1, &newkey);
    RegQueryValueExA(newkey, 0, 0, &type, (PBYTE)value, &count);
    RegCloseKey(newkey);
    return LoadLibraryExA(value, 0, 0);
}

/*************************************************************************
 *      @	[SHLWAPI.237]
 *
 * Unicode version of SHLWAPI_183.
 */
DWORD WINAPI SHRegisterClassW(WNDCLASSW * lpWndClass)
{
	WNDCLASSW WndClass;

	TRACE("(%p %s)\n",lpWndClass->hInstance, debugstr_w(lpWndClass->lpszClassName));

	if (GetClassInfoW(lpWndClass->hInstance, lpWndClass->lpszClassName, &WndClass))
		return TRUE;
	return RegisterClassW(lpWndClass);
}

/*************************************************************************
 *      @	[SHLWAPI.238]
 *
 * Unregister a list of classes.
 *
 * PARAMS
 *  hInst      [I] Application instance that registered the classes
 *  lppClasses [I] List of class names
 *  iCount     [I] Number of names in lppClasses
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI SHUnregisterClassesA(HINSTANCE hInst, LPCSTR *lppClasses, INT iCount)
{
  WNDCLASSA WndClass;

  TRACE("(%p,%p,%d)\n", hInst, lppClasses, iCount);

  while (iCount > 0)
  {
    if (GetClassInfoA(hInst, *lppClasses, &WndClass))
      UnregisterClassA(*lppClasses, hInst);
    lppClasses++;
    iCount--;
  }
}

/*************************************************************************
 *      @	[SHLWAPI.239]
 *
 * Unicode version of SHUnregisterClassesA.
 */
void WINAPI SHUnregisterClassesW(HINSTANCE hInst, LPCWSTR *lppClasses, INT iCount)
{
  WNDCLASSW WndClass;

  TRACE("(%p,%p,%d)\n", hInst, lppClasses, iCount);

  while (iCount > 0)
  {
    if (GetClassInfoW(hInst, *lppClasses, &WndClass))
      UnregisterClassW(*lppClasses, hInst);
    lppClasses++;
    iCount--;
  }
}

/*************************************************************************
 *      @	[SHLWAPI.240]
 *
 * Call The correct (Ascii/Unicode) default window procedure for a window.
 *
 * PARAMS
 *  hWnd     [I] Window to call the default procedure for
 *  uMessage [I] Message ID
 *  wParam   [I] WPARAM of message
 *  lParam   [I] LPARAM of message
 *
 * RETURNS
 *  The result of calling DefWindowProcA() or DefWindowProcW().
 */
LRESULT CALLBACK SHDefWindowProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	if (IsWindowUnicode(hWnd))
		return DefWindowProcW(hWnd, uMessage, wParam, lParam);
	return DefWindowProcA(hWnd, uMessage, wParam, lParam);
}

/*************************************************************************
 *      @	[SHLWAPI.257]
 *
 * Create a worker window using CreateWindowExA().
 *
 * PARAMS
 *  wndProc    [I] Window procedure
 *  hWndParent [I] Parent window
 *  dwExStyle  [I] Extra style flags
 *  dwStyle    [I] Style flags
 *  hMenu      [I] Window menu
 *  z          [I] Unknown
 *
 * RETURNS
 *  Success: The window handle of the newly created window.
 *  Failure: 0.
 */
HWND WINAPI SHCreateWorkerWindowA(LONG wndProc, HWND hWndParent, DWORD dwExStyle,
                        DWORD dwStyle, HMENU hMenu, LONG z)
{
  static const char* szClass = "WorkerA";
  WNDCLASSA wc;
  HWND hWnd;

  TRACE("(0x%08lx,%p,0x%08lx,0x%08lx,%p,0x%08lx)\n",
         wndProc, hWndParent, dwExStyle, dwStyle, hMenu, z);

  /* Create Window class */
  wc.style         = 0;
  wc.lpfnWndProc   = DefWindowProcA;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 4;
  wc.hInstance     = shlwapi_hInstance;
  wc.hIcon         = (HICON)0;
  wc.hCursor       = LoadCursorA((HINSTANCE)0, (LPSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)COLOR_BTNSHADOW;
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = szClass;

  SHRegisterClassA(&wc); /* Register class */

  /* FIXME: Set extra bits in dwExStyle */

  hWnd = CreateWindowExA(dwExStyle, szClass, 0, dwStyle, 0, 0, 0, 0,
                         hWndParent, hMenu, shlwapi_hInstance, 0);
  if (hWnd)
  {
    SetWindowLongA(hWnd, DWL_MSGRESULT, z);

    if (wndProc)
      SetWindowLongA(hWnd, GWL_WNDPROC, wndProc);
  }
  return hWnd;
}

typedef struct tagPOLICYDATA
{
  DWORD policy;        /* flags value passed to SHRestricted */
  LPCWSTR appstr;      /* application str such as "Explorer" */
  LPCWSTR keystr;      /* name of the actual registry key / policy */
} POLICYDATA, *LPPOLICYDATA;

#define SHELL_NO_POLICY 0xffffffff

/* default shell policy registry key */
static const WCHAR strRegistryPolicyW[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o',
                                      's','o','f','t','\\','W','i','n','d','o','w','s','\\',
                                      'C','u','r','r','e','n','t','V','e','r','s','i','o','n',
                                      '\\','P','o','l','i','c','i','e','s',0};

/*************************************************************************
 * @                          [SHLWAPI.271]
 *
 * Retrieve a policy value from the registry.
 *
 * PARAMS
 *  lpSubKey   [I]   registry key name
 *  lpSubName  [I]   subname of registry key
 *  lpValue    [I]   value name of registry value
 *
 * RETURNS
 *  the value associated with the registry key or 0 if not found
 */
DWORD WINAPI SHGetRestriction(LPCWSTR lpSubKey, LPCWSTR lpSubName, LPCWSTR lpValue)
{
	DWORD retval, datsize = sizeof(retval);
	HKEY hKey;

	if (!lpSubKey)
	  lpSubKey = strRegistryPolicyW;

	retval = RegOpenKeyW(HKEY_LOCAL_MACHINE, lpSubKey, &hKey);
    if (retval != ERROR_SUCCESS)
	  retval = RegOpenKeyW(HKEY_CURRENT_USER, lpSubKey, &hKey);
	if (retval != ERROR_SUCCESS)
	  return 0;

	SHGetValueW(hKey, lpSubName, lpValue, NULL, (LPBYTE)&retval, &datsize);
	RegCloseKey(hKey);
	return retval;
}

/*************************************************************************
 * @                         [SHLWAPI.266]
 *
 * Helper function to retrieve the possibly cached value for a specific policy
 *
 * PARAMS
 *  policy     [I]   The policy to look for
 *  initial    [I]   Main registry key to open, if NULL use default
 *  polTable   [I]   Table of known policies, 0 terminated
 *  polArr     [I]   Cache array of policy values
 *
 * RETURNS
 *  The retrieved policy value or 0 if not successful
 *
 * NOTES
 *  This function is used by the native SHRestricted function to search for the
 *  policy and cache it once retrieved. The current Wine implementation uses a
 *  different POLICYDATA structure and implements a similar algorithme adapted to
 *  that structure.
 */
DWORD WINAPI SHRestrictionLookup(
	DWORD policy,
	LPCWSTR initial,
	LPPOLICYDATA polTable,
	LPDWORD polArr)
{
	TRACE("(0x%08lx %s %p %p)\n", policy, debugstr_w(initial), polTable, polArr);

	if (!polTable || !polArr)
	  return 0;

	for (;polTable->policy; polTable++, polArr++)
	{
	  if (policy == polTable->policy)
	  {
	    /* we have a known policy */

	    /* check if this policy has been cached */
		if (*polArr == SHELL_NO_POLICY)
	      *polArr = SHGetRestriction(initial, polTable->appstr, polTable->keystr);
	    return *polArr;
	  }
	}
	/* we don't know this policy, return 0 */
	TRACE("unknown policy: (%08lx)\n", policy);
	return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.267]
 *
 * Get an interface from an object.
 *
 * RETURNS
 *  Success: S_OK. ppv contains the requested interface.
 *  Failure: An HRESULT error code.
 *
 * NOTES
 *   This QueryInterface asks the inner object for a interface. In case
 *   of aggregation this request would be forwarded by the inner to the
 *   outer object. This function asks the inner object directly for the
 *   interface circumventing the forwarding to the outer object.
 */
HRESULT WINAPI SHWeakQueryInterface(
	IUnknown * pUnk,   /* [in] Outer object */
	IUnknown * pInner, /* [in] Inner object */
	IID * riid, /* [in] Interface GUID to query for */
	LPVOID* ppv) /* [out] Destination for queried interface */
{
	HRESULT hret = E_NOINTERFACE;
	TRACE("(pUnk=%p pInner=%p\n\tIID:  %s %p)\n",pUnk,pInner,debugstr_guid(riid), ppv);

	*ppv = NULL;
	if(pUnk && pInner) {
	    hret = IUnknown_QueryInterface(pInner, riid, (LPVOID*)ppv);
	    if (SUCCEEDED(hret)) IUnknown_Release(pUnk);
	}
	TRACE("-- 0x%08lx\n", hret);
	return hret;
}

/*************************************************************************
 *      @	[SHLWAPI.268]
 *
 * Move a reference from one interface to another.
 *
 * PARAMS
 *   lpDest     [O] Destination to receive the reference
 *   lppUnknown [O] Source to give up the reference to lpDest
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI SHWeakReleaseInterface(IUnknown *lpDest, IUnknown **lppUnknown)
{
  TRACE("(%p,%p)\n", lpDest, lppUnknown);

  if (*lppUnknown)
  {
    /* Copy Reference*/
    IUnknown_AddRef(lpDest);
    IUnknown_AtomicRelease(lppUnknown); /* Release existing interface */
  }
}

/*************************************************************************
 *      @	[SHLWAPI.269]
 *
 * Convert an ASCII string of a CLSID into a CLSID.
 *
 * PARAMS
 *  idstr [I] String representing a CLSID in registry format
 *  id    [O] Destination for the converted CLSID
 *
 * RETURNS
 *  Success: TRUE. id contains the converted CLSID.
 *  Failure: FALSE.
 */
BOOL WINAPI GUIDFromStringA(LPCSTR idstr, CLSID *id)
{
  WCHAR wClsid[40];
  MultiByteToWideChar(CP_ACP, 0, idstr, -1, wClsid, sizeof(wClsid)/sizeof(WCHAR));
  return SUCCEEDED(CLSIDFromStringWrap(wClsid, id));
}

/*************************************************************************
 *      @	[SHLWAPI.270]
 *
 * Unicode version of GUIDFromStringA.
 */
BOOL WINAPI GUIDFromStringW(LPCWSTR idstr, CLSID *id)
{
  return SUCCEEDED(CLSIDFromStringWrap(idstr, id));
}

/*************************************************************************
 *      @	[SHLWAPI.276]
 *
 * Determine if the browser is integrated into the shell, and set a registry
 * key accordingly.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  1, If the browser is not integrated.
 *  2, If the browser is integrated.
 *
 * NOTES
 *  The key "HKLM\Software\Microsoft\Internet Explorer\IntegratedBrowser" is
 *  either set to TRUE, or removed depending on whether the browser is deemed
 *  to be integrated.
 */
DWORD WINAPI WhichPlatform()
{
  static LPCSTR szIntegratedBrowser = "IntegratedBrowser";
  static DWORD dwState = 0;
  HKEY hKey;
  DWORD dwRet, dwData, dwSize;

  if (dwState)
    return dwState;

  /* If shell32 exports DllGetVersion(), the browser is integrated */
  GET_FUNC(pDllGetVersion, shell32, "DllGetVersion", 1);
  dwState = pDllGetVersion ? 2 : 1;

  /* Set or delete the key accordingly */
  dwRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                        "Software\\Microsoft\\Internet Explorer", 0,
                         KEY_ALL_ACCESS, &hKey);
  if (!dwRet)
  {
    dwRet = RegQueryValueExA(hKey, szIntegratedBrowser, 0, 0,
                             (LPBYTE)&dwData, &dwSize);

    if (!dwRet && dwState == 1)
    {
      /* Value exists but browser is not integrated */
      RegDeleteValueA(hKey, szIntegratedBrowser);
    }
    else if (dwRet && dwState == 2)
    {
      /* Browser is integrated but value does not exist */
      dwData = TRUE;
      RegSetValueExA(hKey, szIntegratedBrowser, 0, REG_DWORD,
                     (LPBYTE)&dwData, sizeof(dwData));
    }
    RegCloseKey(hKey);
  }
  return dwState;
}

/*************************************************************************
 *      @	[SHLWAPI.278]
 *
 * Unicode version of SHCreateWorkerWindowA.
 */
HWND WINAPI SHCreateWorkerWindowW(LONG wndProc, HWND hWndParent, DWORD dwExStyle,
                        DWORD dwStyle, HMENU hMenu, LONG z)
{
  static const WCHAR szClass[] = { 'W', 'o', 'r', 'k', 'e', 'r', 'W', '\0' };
  WNDCLASSW wc;
  HWND hWnd;

  TRACE("(0x%08lx,%p,0x%08lx,0x%08lx,%p,0x%08lx)\n",
         wndProc, hWndParent, dwExStyle, dwStyle, hMenu, z);

  /* If our OS is natively ASCII, use the ASCII version */
  if (!(GetVersion() & 0x80000000))  /* NT */
    return SHCreateWorkerWindowA(wndProc, hWndParent, dwExStyle, dwStyle, hMenu, z);

  /* Create Window class */
  wc.style         = 0;
  wc.lpfnWndProc   = DefWindowProcW;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 4;
  wc.hInstance     = shlwapi_hInstance;
  wc.hIcon         = (HICON)0;
  wc.hCursor       = LoadCursorA((HINSTANCE)0, (LPSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)COLOR_BTNSHADOW;
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = szClass;

  SHRegisterClassW(&wc); /* Register class */

  /* FIXME: Set extra bits in dwExStyle */

  hWnd = CreateWindowExW(dwExStyle, szClass, 0, dwStyle, 0, 0, 0, 0,
                         hWndParent, hMenu, shlwapi_hInstance, 0);
  if (hWnd)
  {
    SetWindowLongA(hWnd, DWL_MSGRESULT, z);

    if (wndProc)
      SetWindowLongA(hWnd, GWL_WNDPROC, wndProc);
  }
  return hWnd;
}

/*************************************************************************
 *      @	[SHLWAPI.279]
 *
 * Get and show a context menu from a shell folder.
 *
 * PARAMS
 *  hWnd           [I] Window displaying the shell folder
 *  lpFolder       [I] IShellFolder interface
 *  lpApidl        [I] Id for the particular folder desired
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI SHInvokeDefaultCommand(HWND hWnd, IShellFolder* lpFolder, LPCITEMIDLIST lpApidl)
{
  return SHInvokeCommand(hWnd, lpFolder, lpApidl, FALSE);
}

/*************************************************************************
 *      @	[SHLWAPI.281]
 *
 * _SHPackDispParamsV
 */
HRESULT WINAPI SHPackDispParamsV(LPVOID w, LPVOID x, LPVOID y, LPVOID z)
{
	FIXME("%p %p %p %p\n",w,x,y,z);
	return E_FAIL;
}

/*************************************************************************
 *      @       [SHLWAPI.282]
 *
 * This function seems to be a forward to SHPackDispParamsV (whatever THAT
 * function does...).
 */
HRESULT WINAPI SHPackDispParams(LPVOID w, LPVOID x, LPVOID y, LPVOID z)
{
  FIXME("%p %p %p %p\n", w, x, y, z);
  return E_FAIL;
}

/*************************************************************************
 *      @	[SHLWAPI.284]
 *
 * _IConnectionPoint_SimpleInvoke
 */
DWORD WINAPI IConnectionPoint_SimpleInvoke(
	LPVOID x,
	LPVOID y,
	LPVOID z)
{
        FIXME("(%p %p %p) stub\n",x,y,z);
	return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.285]
 *
 * Notify an IConnectionPoint object of changes.
 *
 * PARAMS
 *  lpCP   [I] Object to notify
 *  dispID [I]
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_NOINTERFACE, if lpCP is NULL or does not support the
 *           IConnectionPoint interface.
 */
HRESULT WINAPI IConnectionPoint_OnChanged(IConnectionPoint* lpCP, DISPID dispID)
{
  IEnumConnections *lpEnum;
  HRESULT hRet = E_NOINTERFACE;

  TRACE("(%p,0x%8lX)\n", lpCP, dispID);

  /* Get an enumerator for the connections */
  if (lpCP)
    hRet = IConnectionPoint_EnumConnections(lpCP, &lpEnum);

  if (SUCCEEDED(hRet))
  {
    IPropertyNotifySink *lpSink;
    CONNECTDATA connData;
    ULONG ulFetched;

    /* Call OnChanged() for every notify sink in the connection point */
    while (IEnumConnections_Next(lpEnum, 1, &connData, &ulFetched) == S_OK)
    {
      if (SUCCEEDED(IUnknown_QueryInterface(connData.pUnk, &IID_IPropertyNotifySink, (void**)&lpSink)) &&
          lpSink)
      {
        IPropertyNotifySink_OnChanged(lpSink, dispID);
        IPropertyNotifySink_Release(lpSink);
      }
      IUnknown_Release(connData.pUnk);
    }

    IEnumConnections_Release(lpEnum);
  }
  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.287]
 *
 * Notify an IConnectionPointContainer object of changes.
 *
 * PARAMS
 *  lpUnknown [I] Object to notify
 *  dispID    [I]
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_NOINTERFACE, if lpUnknown is NULL or does not support the
 *           IConnectionPointContainer interface.
 */
HRESULT WINAPI IUnknown_CPContainerOnChanged(IUnknown *lpUnknown, DISPID dispID)
{
  IConnectionPointContainer* lpCPC = NULL;
  HRESULT hRet = E_NOINTERFACE;

  TRACE("(%p,0x%8lX)\n", lpUnknown, dispID);

  if (lpUnknown)
    hRet = IUnknown_QueryInterface(lpUnknown, &IID_IConnectionPointContainer, (void**)&lpCPC);

  if (SUCCEEDED(hRet))
  {
    IConnectionPoint* lpCP;

    hRet = IConnectionPointContainer_FindConnectionPoint(lpCPC, &IID_IPropertyNotifySink, &lpCP);
    IConnectionPointContainer_Release(lpCPC);

    hRet = IConnectionPoint_OnChanged(lpCP, dispID);
    IConnectionPoint_Release(lpCP);
  }
  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.289]
 *
 * See PlaySoundW.
 */
BOOL WINAPI PlaySoundWrapW(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound)
{
  GET_FUNC(pPlaySoundW, winmm, "PlaySoundW", FALSE);
  return pPlaySoundW(pszSound, hmod, fdwSound);
}

/*************************************************************************
 *      @	[SHLWAPI.294]
 */
BOOL WINAPI SHGetIniStringW(LPSTR str1, LPSTR str2, LPSTR pStr, DWORD some_len,  LPCSTR lpStr2)
{
    /*
     * str1:		"I"	"I"	pushl esp+0x20
     * str2:		"U"	"I"	pushl 0x77c93810
     * (is "I" and "U" "integer" and "unsigned" ??)
     *
     * pStr:		""	""	pushl eax
     * some_len:	0x824	0x104	pushl 0x824
     * lpStr2:		"%l"	"%l"	pushl esp+0xc
     *
     * shlwapi. StrCpyNW(lpStr2, irrelevant_var, 0x104);
     * LocalAlloc(0x00, some_len) -> irrelevant_var
     * LocalAlloc(0x40, irrelevant_len) -> pStr
     * shlwapi.294(str1, str2, pStr, some_len, lpStr2);
     * shlwapi.PathRemoveBlanksW(pStr);
     */
    FIXME("('%s', '%s', '%s', %08lx, '%s'): stub!\n", str1, str2, pStr, some_len, lpStr2);
    return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.295]
 *
 * Called by ICQ2000b install via SHDOCVW:
 * str1: "InternetShortcut"
 * x: some unknown pointer
 * str2: "http://free.aol.com/tryaolfree/index.adp?139269"
 * str3: "C:\\WINDOWS\\Desktop.new2\\Free AOL & Unlimited Internet.url"
 *
 * In short: this one maybe creates a desktop link :-)
 */
BOOL WINAPI SHSetIniStringW(LPWSTR str1, LPVOID x, LPWSTR str2, LPWSTR str3)
{
    FIXME("('%s', %p, '%s', '%s'), stub.\n", debugstr_w(str1), x, debugstr_w(str2), debugstr_w(str3));
    return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.299]
 *
 * See COMCTL32_417.
 */
BOOL WINAPI ExtTextOutWrapW(HDC hdc, INT x, INT y, UINT flags, const RECT *lprect,
                         LPCWSTR str, UINT count, const INT *lpDx)
{
    GET_FUNC(pCOMCTL32_417, comctl32, (LPCSTR)417, FALSE);
    return pCOMCTL32_417(hdc, x, y, flags, lprect, str, count, lpDx);
}

/*************************************************************************
 *      @	[SHLWAPI.313]
 *
 * See SHGetFileInfoW.
 */
DWORD WINAPI SHGetFileInfoWrapW(LPCWSTR path, DWORD dwFileAttributes,
                         SHFILEINFOW *psfi, UINT sizeofpsfi, UINT flags)
{
  GET_FUNC(pSHGetFileInfoW, shell32, "SHGetFileInfoW", 0);
  return pSHGetFileInfoW(path, dwFileAttributes, psfi, sizeofpsfi, flags);
}

/*************************************************************************
 *      @	[SHLWAPI.318]
 *
 * See DragQueryFileW.
 */
UINT WINAPI DragQueryFileWrapW(HDROP hDrop, UINT lFile, LPWSTR lpszFile, UINT lLength)
{
  GET_FUNC(pDragQueryFileW, shell32, "DragQueryFileW", 0);
  return pDragQueryFileW(hDrop, lFile, lpszFile, lLength);
}

/*************************************************************************
 *      @	[SHLWAPI.333]
 *
 * See SHBrowseForFolderW.
 */
LPITEMIDLIST WINAPI SHBrowseForFolderWrapW(LPBROWSEINFOW lpBi)
{
  GET_FUNC(pSHBrowseForFolderW, shell32, "SHBrowseForFolderW", NULL);
  return pSHBrowseForFolderW(lpBi);
}

/*************************************************************************
 *      @	[SHLWAPI.334]
 *
 * See SHGetPathFromIDListW.
 */
BOOL WINAPI SHGetPathFromIDListWrapW(LPCITEMIDLIST pidl,LPWSTR pszPath)
{
  GET_FUNC(pSHGetPathFromIDListW, shell32, "SHGetPathFromIDListW", 0);
  return pSHGetPathFromIDListW(pidl, pszPath);
}

/*************************************************************************
 *      @	[SHLWAPI.335]
 *
 * See ShellExecuteExW.
 */
BOOL WINAPI ShellExecuteExWrapW(LPSHELLEXECUTEINFOW lpExecInfo)
{
  GET_FUNC(pShellExecuteExW, shell32, "ShellExecuteExW", FALSE);
  return pShellExecuteExW(lpExecInfo);
}

/*************************************************************************
 *      @	[SHLWAPI.336]
 *
 * See SHFileOperationW.
 */
HICON WINAPI SHFileOperationWrapW(LPSHFILEOPSTRUCTW lpFileOp)
{
  GET_FUNC(pSHFileOperationW, shell32, "SHFileOperationW", 0);
  return pSHFileOperationW(lpFileOp);
}

/*************************************************************************
 *      @	[SHLWAPI.337]
 *
 * See ExtractIconExW.
 */
UINT WINAPI ExtractIconExWrapW(LPCWSTR lpszFile, INT nIconIndex, HICON *phiconLarge,
                         HICON *phiconSmall, UINT nIcons)
{
  GET_FUNC(pExtractIconExW, shell32, "ExtractIconExW", 0);
  return pExtractIconExW(lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
}

/*************************************************************************
 *      @	[SHLWAPI.342]
 *
 */
LONG WINAPI SHInterlockedCompareExchange( PLONG dest, LONG xchg, LONG compare)
{
        return InterlockedCompareExchange(dest, xchg, compare);
}

/*************************************************************************
 *      @	[SHLWAPI.346]
 */
DWORD WINAPI SHUnicodeToUnicode(
	LPCWSTR src,
	LPWSTR dest,
	int len)
{
	FIXME("(%s %p 0x%08x)stub\n",debugstr_w(src),dest,len);
	lstrcpynW(dest, src, len);
	return lstrlenW(dest)+1;
}

/*************************************************************************
 *      @	[SHLWAPI.350]
 *
 * See GetFileVersionInfoSizeW.
 */
DWORD WINAPI GetFileVersionInfoSizeWrapW(
	LPWSTR x,
	LPVOID y)
{
        DWORD ret;

	GET_FUNC(pGetFileVersionInfoSizeW, version, "GetFileVersionInfoSizeW", 0);
	ret = pGetFileVersionInfoSizeW(x, y);
	return 0x208 + ret;
}

/*************************************************************************
 *      @	[SHLWAPI.351]
 *
 * See GetFileVersionInfoW.
 */
BOOL  WINAPI GetFileVersionInfoWrapW(
	LPWSTR w,   /* [in] path to dll */
	DWORD  x,   /* [in] parm 2 to GetFileVersionInfoA */
	DWORD  y,   /* [in] return value from SHLWAPI_350() - assume length */
	LPVOID z)   /* [in/out] buffer (+0x208 sent to GetFileVersionInfoA()) */
{
    GET_FUNC(pGetFileVersionInfoW, version, "GetFileVersionInfoW", 0);
    return pGetFileVersionInfoW(w, x, y-0x208, (char*)z+0x208);
}

/*************************************************************************
 *      @	[SHLWAPI.352]
 *
 * See VerQueryValueW.
 */
WORD WINAPI VerQueryValueWrapW(
	LPVOID w,   /* [in] Buffer from SHLWAPI_351() */
	LPWSTR x,   /* [in]   Value to retrieve - converted and passed to VerQueryValueA() as #2 */
	LPVOID y,   /* [out]  Ver buffer - passed to VerQueryValueA as #3 */
	UINT*  z)   /* [in]   Ver length - passed to VerQueryValueA as #4 */
{
    GET_FUNC(pVerQueryValueW, version, "VerQueryValueW", 0);
    return pVerQueryValueW((char*)w+0x208, x, y, z);
}

#define IsIface(type) SUCCEEDED((hRet = IUnknown_QueryInterface(lpUnknown, &IID_##type, (void**)&lpObj)))
#define IShellBrowser_EnableModeless IShellBrowser_EnableModelessSB
#define EnableModeless(type) type##_EnableModeless((type*)lpObj, bModeless)

/*************************************************************************
 *      @	[SHLWAPI.355]
 *
 * Change the modality of a shell object.
 *
 * PARAMS
 *  lpUnknown [I] Object to make modeless
 *  bModeless [I] TRUE=Make modeless, FALSE=Make modal
 *
 * RETURNS
 *  Success: S_OK. The modality lpUnknown is changed.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  lpUnknown must support the IOleInPlaceFrame interface, the
 *  IInternetSecurityMgrSite interface, the IShellBrowser interface
 *  or the IDocHostUIHandler interface, or this call fails.
 */
HRESULT WINAPI IUnknown_EnableModeless(IUnknown *lpUnknown, BOOL bModeless)
{
  IUnknown *lpObj;
  HRESULT hRet;

  TRACE("(%p,%d)\n", lpUnknown, bModeless);

  if (!lpUnknown)
    return E_FAIL;

  if (IsIface(IOleInPlaceFrame))
    EnableModeless(IOleInPlaceFrame);
  else if (IsIface(IShellBrowser))
    EnableModeless(IShellBrowser);
#if 0
  /* FIXME: Wine has no headers for these objects yet */
  else if (IsIface(IInternetSecurityMgrSite))
    EnableModeless(IInternetSecurityMgrSite);
  else if (IsIface(IDocHostUIHandler))
    EnableModeless(IDocHostUIHandler);
#endif
  else
    return hRet;

  IUnknown_Release(lpObj);
  return S_OK;
}

/*************************************************************************
 *      @	[SHLWAPI.357]
 *
 * See SHGetNewLinkInfoW.
 */
BOOL WINAPI SHGetNewLinkInfoWrapW(LPCWSTR pszLinkTo, LPCWSTR pszDir, LPWSTR pszName,
                        BOOL *pfMustCopy, UINT uFlags)
{
  GET_FUNC(pSHGetNewLinkInfoW, shell32, "SHGetNewLinkInfoW", FALSE);
  return pSHGetNewLinkInfoW(pszLinkTo, pszDir, pszName, pfMustCopy, uFlags);
}

/*************************************************************************
 *      @	[SHLWAPI.358]
 *
 * See SHDefExtractIconW.
 */
UINT WINAPI SHDefExtractIconWrapW(LPCWSTR pszIconFile, int iIndex, UINT uFlags, HICON* phiconLarge,
                         HICON* phiconSmall, UINT nIconSize)
{
  GET_FUNC(pSHDefExtractIconW, shell32, "SHDefExtractIconW", 0);
  return pSHDefExtractIconW(pszIconFile, iIndex, uFlags, phiconLarge, phiconSmall, nIconSize);
}

/*************************************************************************
 *      @	[SHLWAPI.363]
 *
 * Get and show a context menu from a shell folder.
 *
 * PARAMS
 *  hWnd           [I] Window displaying the shell folder
 *  lpFolder       [I] IShellFolder interface
 *  lpApidl        [I] Id for the particular folder desired
 *  bInvokeDefault [I] Whether to invoke the default menu item
 *
 * RETURNS
 *  Success: S_OK. If bInvokeDefault is TRUE, the default menu action was
 *           executed.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI SHInvokeCommand(HWND hWnd, IShellFolder* lpFolder, LPCITEMIDLIST lpApidl, BOOL bInvokeDefault)
{
  IContextMenu *iContext;
  HRESULT hRet = E_FAIL;

  TRACE("(%p,%p,%p,%d)\n", hWnd, lpFolder, lpApidl, bInvokeDefault);

  if (!lpFolder)
    return hRet;

  /* Get the context menu from the shell folder */
  hRet = IShellFolder_GetUIObjectOf(lpFolder, hWnd, 1, &lpApidl,
                                    &IID_IContextMenu, 0, (void**)&iContext);
  if (SUCCEEDED(hRet))
  {
    HMENU hMenu;
    if ((hMenu = CreatePopupMenu()))
    {
      HRESULT hQuery;
      DWORD dwDefaultId = 0;

      /* Add the context menu entries to the popup */
      hQuery = IContextMenu_QueryContextMenu(iContext, hMenu, 0, 1, 0x7FFF,
                                             bInvokeDefault ? CMF_NORMAL : CMF_DEFAULTONLY);

      if (SUCCEEDED(hQuery))
      {
        if (bInvokeDefault &&
            (dwDefaultId = GetMenuDefaultItem(hMenu, 0, 0)) != 0xFFFFFFFF)
        {
          CMINVOKECOMMANDINFO cmIci;
          /* Invoke the default item */
          memset(&cmIci,0,sizeof(cmIci));
          cmIci.cbSize = sizeof(cmIci);
          cmIci.fMask = CMIC_MASK_ASYNCOK;
          cmIci.hwnd = hWnd;
          cmIci.lpVerb = MAKEINTRESOURCEA(dwDefaultId);
          cmIci.nShow = SW_SCROLLCHILDREN;

          hRet = IContextMenu_InvokeCommand(iContext, &cmIci);
        }
      }
      DestroyMenu(hMenu);
    }
    IContextMenu_Release(iContext);
  }
  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.370]
 *
 * See ExtractIconW.
 */
HICON WINAPI ExtractIconWrapW(HINSTANCE hInstance, LPCWSTR lpszExeFileName,
                         UINT nIconIndex)
{
  GET_FUNC(pExtractIconW, shell32, "ExtractIconW", NULL);
  return pExtractIconW(hInstance, lpszExeFileName, nIconIndex);
}

/*************************************************************************
 *      @	[SHLWAPI.376]
 */
LANGID WINAPI MLGetUILanguage()
{
    FIXME("() stub\n");
    /* FIXME: This should be a forward in the .spec file to the win2k function
     * kernel32.GetUserDefaultUILanguage, however that function isn't there yet.
     */
    return GetUserDefaultLangID();
}

/*************************************************************************
 *      @	[SHLWAPI.377]
 *
 * Load a library from the directory of a particular process.
 *
 * PARAMS
 *  new_mod   [I] Library name
 *  inst_hwnd [I] Module whose directory is to be used
 *  dwFlags   [I] Flags controlling the load
 *
 * RETURNS
 *  Success: A handle to the loaded module
 *  Failure: A NULL handle.
 */
HMODULE WINAPI MLLoadLibraryA(LPCSTR new_mod, HMODULE inst_hwnd, DWORD dwFlags)
{
  /* FIXME: Native appears to do DPA_Create and a DPA_InsertPtr for
   *        each call here.
   * FIXME: Native shows calls to:
   *  SHRegGetUSValue for "Software\Microsoft\Internet Explorer\International"
   *                      CheckVersion
   *  RegOpenKeyExA for "HKLM\Software\Microsoft\Internet Explorer"
   *  RegQueryValueExA for "LPKInstalled"
   *  RegCloseKey
   *  RegOpenKeyExA for "HKCU\Software\Microsoft\Internet Explorer\International"
   *  RegQueryValueExA for "ResourceLocale"
   *  RegCloseKey
   *  RegOpenKeyExA for "HKLM\Software\Microsoft\Active Setup\Installed Components\{guid}"
   *  RegQueryValueExA for "Locale"
   *  RegCloseKey
   *  and then tests the Locale ("en" for me).
   *     code below
   *  after the code then a DPA_Create (first time) and DPA_InsertPtr are done.
   */
    CHAR mod_path[2*MAX_PATH];
    LPSTR ptr;
    DWORD len;

    FIXME("(%s,%p,0x%08lx) semi-stub!\n", debugstr_a(new_mod), inst_hwnd, dwFlags);
    len = GetModuleFileNameA(inst_hwnd, mod_path, sizeof(mod_path));
    if (!len || len >= sizeof(mod_path)) return NULL;

    ptr = strrchr(mod_path, '\\');
    if (ptr) {
	strcpy(ptr+1, new_mod);
	TRACE("loading %s\n", debugstr_a(mod_path));
	return LoadLibraryA(mod_path);
    }
    return NULL;
}

/*************************************************************************
 *      @	[SHLWAPI.378]
 *
 * Unicode version of MLLoadLibraryA.
 */
HMODULE WINAPI MLLoadLibraryW(LPCWSTR new_mod, HMODULE inst_hwnd, DWORD dwFlags)
{
    WCHAR mod_path[2*MAX_PATH];
    LPWSTR ptr;
    DWORD len;

    FIXME("(%s,%p,0x%08lx) semi-stub!\n", debugstr_w(new_mod), inst_hwnd, dwFlags);
    len = GetModuleFileNameW(inst_hwnd, mod_path, sizeof(mod_path) / sizeof(WCHAR));
    if (!len || len >= sizeof(mod_path) / sizeof(WCHAR)) return NULL;

    ptr = strrchrW(mod_path, '\\');
    if (ptr) {
	strcpyW(ptr+1, new_mod);
	TRACE("loading %s\n", debugstr_w(mod_path));
	return LoadLibraryW(mod_path);
    }
    return NULL;
}

/*************************************************************************
 * ColorAdjustLuma      [SHLWAPI.@]
 *
 * Adjust the luminosity of a color
 *
 * PARAMS
 *  cRGB         [I] RGB value to convert
 *  dwLuma       [I] Luma adjustment
 *  bUnknown     [I] Unknown
 *
 * RETURNS
 *  The adjusted RGB color.
 */
COLORREF WINAPI ColorAdjustLuma(COLORREF cRGB, int dwLuma, BOOL bUnknown)
{
  TRACE("(0x%8lx,%d,%d)\n", cRGB, dwLuma, bUnknown);

  if (dwLuma)
  {
    WORD wH, wL, wS;

    ColorRGBToHLS(cRGB, &wH, &wL, &wS);

    FIXME("Ignoring luma adjustment\n");

    /* FIXME: The ajdustment is not linear */

    cRGB = ColorHLSToRGB(wH, wL, wS);
  }
  return cRGB;
}

/*************************************************************************
 *      @	[SHLWAPI.389]
 *
 * See GetSaveFileNameW.
 */
BOOL WINAPI GetSaveFileNameWrapW(LPOPENFILENAMEW ofn)
{
  GET_FUNC(pGetSaveFileNameW, comdlg32, "GetSaveFileNameW", FALSE);
  return pGetSaveFileNameW(ofn);
}

/*************************************************************************
 *      @	[SHLWAPI.390]
 *
 * See WNetRestoreConnectionW.
 */
DWORD WINAPI WNetRestoreConnectionWrapW(HWND hwndOwner, LPWSTR lpszDevice)
{
  GET_FUNC(pWNetRestoreConnectionW, mpr, "WNetRestoreConnectionW", 0);
  return pWNetRestoreConnectionW(hwndOwner, lpszDevice);
}

/*************************************************************************
 *      @	[SHLWAPI.391]
 *
 * See WNetGetLastErrorW.
 */
DWORD WINAPI WNetGetLastErrorWrapW(LPDWORD lpError, LPWSTR lpErrorBuf, DWORD nErrorBufSize,
                         LPWSTR lpNameBuf, DWORD nNameBufSize)
{
  GET_FUNC(pWNetGetLastErrorW, mpr, "WNetGetLastErrorW", 0);
  return pWNetGetLastErrorW(lpError, lpErrorBuf, nErrorBufSize, lpNameBuf, nNameBufSize);
}

/*************************************************************************
 *      @	[SHLWAPI.401]
 *
 * See PageSetupDlgW.
 */
BOOL WINAPI PageSetupDlgWrapW(LPPAGESETUPDLGW pagedlg)
{
  GET_FUNC(pPageSetupDlgW, comdlg32, "PageSetupDlgW", FALSE);
  return pPageSetupDlgW(pagedlg);
}

/*************************************************************************
 *      @	[SHLWAPI.402]
 *
 * See PrintDlgW.
 */
BOOL WINAPI PrintDlgWrapW(LPPRINTDLGW printdlg)
{
  GET_FUNC(pPrintDlgW, comdlg32, "PrintDlgW", FALSE);
  return pPrintDlgW(printdlg);
}

/*************************************************************************
 *      @	[SHLWAPI.403]
 *
 * See GetOpenFileNameW.
 */
BOOL WINAPI GetOpenFileNameWrapW(LPOPENFILENAMEW ofn)
{
  GET_FUNC(pGetOpenFileNameW, comdlg32, "GetOpenFileNameW", FALSE);
  return pGetOpenFileNameW(ofn);
}

/* INTERNAL: Map from HLS color space to RGB */
static WORD WINAPI ConvertHue(int wHue, WORD wMid1, WORD wMid2)
{
  wHue = wHue > 240 ? wHue - 240 : wHue < 0 ? wHue + 240 : wHue;

  if (wHue > 160)
    return wMid1;
  else if (wHue > 120)
    wHue = 160 - wHue;
  else if (wHue > 40)
    return wMid2;

  return ((wHue * (wMid2 - wMid1) + 20) / 40) + wMid1;
}

/* Convert to RGB and scale into RGB range (0..255) */
#define GET_RGB(h) (ConvertHue(h, wMid1, wMid2) * 255 + 120) / 240

/*************************************************************************
 *      ColorHLSToRGB	[SHLWAPI.@]
 *
 * Convert from hls color space into an rgb COLORREF.
 *
 * PARAMS
 *  wHue        [I] Hue amount
 *  wLuminosity [I] Luminosity amount
 *  wSaturation [I] Saturation amount
 *
 * RETURNS
 *  A COLORREF representing the converted color.
 *
 * NOTES
 *  Input hls values are constrained to the range (0..240).
 */
COLORREF WINAPI ColorHLSToRGB(WORD wHue, WORD wLuminosity, WORD wSaturation)
{
  WORD wRed;

  if (wSaturation)
  {
    WORD wGreen, wBlue, wMid1, wMid2;

    if (wLuminosity > 120)
      wMid2 = wSaturation + wLuminosity - (wSaturation * wLuminosity + 120) / 240;
    else
      wMid2 = ((wSaturation + 240) * wLuminosity + 120) / 240;

    wMid1 = wLuminosity * 2 - wMid2;

    wRed   = GET_RGB(wHue + 80);
    wGreen = GET_RGB(wHue);
    wBlue  = GET_RGB(wHue - 80);

    return RGB(wRed, wGreen, wBlue);
  }

  wRed = wLuminosity * 255 / 240;
  return RGB(wRed, wRed, wRed);
}

/*************************************************************************
 *      @	[SHLWAPI.413]
 *
 * Get the current docking status of the system.
 *
 * PARAMS
 *  dwFlags [I] DOCKINFO_ flags from "winbase.h", unused
 *
 * RETURNS
 *  One of DOCKINFO_UNDOCKED, DOCKINFO_UNDOCKED, or 0 if the system is not
 *  a notebook.
 */
DWORD WINAPI SHGetMachineInfo(DWORD dwFlags)
{
  HW_PROFILE_INFOA hwInfo;

  TRACE("(0x%08lx)\n", dwFlags);

  GetCurrentHwProfileA(&hwInfo);
  switch (hwInfo.dwDockInfo & (DOCKINFO_DOCKED|DOCKINFO_UNDOCKED))
  {
  case DOCKINFO_DOCKED:
  case DOCKINFO_UNDOCKED:
    return hwInfo.dwDockInfo & (DOCKINFO_DOCKED|DOCKINFO_UNDOCKED);
  default:
    return 0;
  }
}

/*************************************************************************
 *      @	[SHLWAPI.418]
 *
 * Function seems to do FreeLibrary plus other things.
 *
 * FIXME native shows the following calls:
 *   RtlEnterCriticalSection
 *   LocalFree
 *   GetProcAddress(Comctl32??, 150L)
 *   DPA_DeletePtr
 *   RtlLeaveCriticalSection
 *  followed by the FreeLibrary.
 *  The above code may be related to .377 above.
 */
BOOL WINAPI MLFreeLibrary(HMODULE hModule)
{
	FIXME("(%p) semi-stub\n", hModule);
	return FreeLibrary(hModule);
}

/*************************************************************************
 *      @	[SHLWAPI.419]
 */
BOOL WINAPI SHFlushSFCacheWrap(void) {
  FIXME(": stub\n");
  return TRUE;
}

/*************************************************************************
 *      @      [SHLWAPI.429]
 * FIXME I have no idea what this function does or what its arguments are.
 */
BOOL WINAPI MLIsMLHInstance(HINSTANCE hInst)
{
       FIXME("(%p) stub\n", hInst);
       return FALSE;
}


/*************************************************************************
 *      @	[SHLWAPI.430]
 */
DWORD WINAPI MLSetMLHInstance(HINSTANCE hInst, HANDLE hHeap)
{
	FIXME("(%p,%p) stub\n", hInst, hHeap);
	return E_FAIL;   /* This is what is used if shlwapi not loaded */
}

/*************************************************************************
 *      @	[SHLWAPI.431]
 */
DWORD WINAPI MLClearMLHInstance(DWORD x)
{
	FIXME("(0x%08lx)stub\n", x);
	return 0xabba1247;
}

/*************************************************************************
 *      @	[SHLWAPI.436]
 *
 * Convert an Unicode string CLSID into a CLSID.
 *
 * PARAMS
 *  idstr      [I]   string containing a CLSID in text form
 *  id         [O]   CLSID extracted from the string
 *
 * RETURNS
 *  S_OK on success or E_INVALIDARG on failure
 *
 * NOTES
 *  This is really CLSIDFromString() which is exported by ole32.dll,
 *  however the native shlwapi.dll does *not* import ole32. Nor does
 *  ole32.dll import this ordinal from shlwapi. Therefore we must conclude
 *  that MS duplicated the code for CLSIDFromString(), and yes they did, only
 *  it returns an E_INVALIDARG error code on failure.
 *  This is a duplicate (with changes for Unicode) of CLSIDFromString16()
 *  in "dlls/ole32/compobj.c".
 */
HRESULT WINAPI CLSIDFromStringWrap(LPCWSTR idstr, CLSID *id)
{
	LPCWSTR s = idstr;
	BYTE *p;
	INT i;
	WCHAR table[256];

	if (!s) {
	  memset(id, 0, sizeof(CLSID));
	  return S_OK;
	}
	else {  /* validate the CLSID string */

	  if (strlenW(s) != 38)
	    return E_INVALIDARG;

	  if ((s[0]!=L'{') || (s[9]!=L'-') || (s[14]!=L'-') || (s[19]!=L'-') || (s[24]!=L'-') || (s[37]!=L'}'))
	    return E_INVALIDARG;

	  for (i=1; i<37; i++)
	  {
	    if ((i == 9)||(i == 14)||(i == 19)||(i == 24))
	      continue;
	    if (!(((s[i] >= L'0') && (s[i] <= L'9'))  ||
	        ((s[i] >= L'a') && (s[i] <= L'f'))  ||
	        ((s[i] >= L'A') && (s[i] <= L'F')))
	       )
	      return E_INVALIDARG;
	  }
	}

    TRACE("%s -> %p\n", debugstr_w(s), id);

  /* quick lookup table */
    memset(table, 0, 256*sizeof(WCHAR));

    for (i = 0; i < 10; i++) {
	table['0' + i] = i;
    }
    for (i = 0; i < 6; i++) {
	table['A' + i] = i+10;
	table['a' + i] = i+10;
    }

    /* in form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} */

    p = (BYTE *) id;

    s++;	/* skip leading brace  */
    for (i = 0; i < 4; i++) {
	p[3 - i] = table[*s]<<4 | table[*(s+1)];
	s += 2;
    }
    p += 4;
    s++;	/* skip - */

    for (i = 0; i < 2; i++) {
	p[1-i] = table[*s]<<4 | table[*(s+1)];
	s += 2;
    }
    p += 2;
    s++;	/* skip - */

    for (i = 0; i < 2; i++) {
	p[1-i] = table[*s]<<4 | table[*(s+1)];
	s += 2;
    }
    p += 2;
    s++;	/* skip - */

    /* these are just sequential bytes */
    for (i = 0; i < 2; i++) {
	*p++ = table[*s]<<4 | table[*(s+1)];
	s += 2;
    }
    s++;	/* skip - */

    for (i = 0; i < 6; i++) {
	*p++ = table[*s]<<4 | table[*(s+1)];
	s += 2;
    }

    return S_OK;
}

/*************************************************************************
 *      @	[SHLWAPI.437]
 *
 * Determine if the OS supports a given feature.
 *
 * PARAMS
 *  dwFeature [I] Feature requested (undocumented)
 *
 * RETURNS
 *  TRUE  If the feature is available.
 *  FALSE If the feature is not available.
 */
DWORD WINAPI IsOS(DWORD feature)
{
  FIXME("(0x%08lx) stub\n", feature);
  if (feature == 4)
    return TRUE;
  return FALSE;
}

/*************************************************************************
 *      ColorRGBToHLS	[SHLWAPI.@]
 *
 * Convert an rgb COLORREF into the hls color space.
 *
 * PARAMS
 *  cRGB         [I] Source rgb value
 *  pwHue        [O] Destination for converted hue
 *  pwLuminance  [O] Destination for converted luminance
 *  pwSaturation [O] Destination for converted saturation
 *
 * RETURNS
 *  Nothing. pwHue, pwLuminance and pwSaturation are set to the converted
 *  values.
 *
 * NOTES
 *  Output HLS values are constrained to the range (0..240).
 *  For Achromatic conversions, Hue is set to 160.
 */
VOID WINAPI ColorRGBToHLS(COLORREF cRGB, LPWORD pwHue,
			  LPWORD pwLuminance, LPWORD pwSaturation)
{
  int wR, wG, wB, wMax, wMin, wHue, wLuminosity, wSaturation;

  TRACE("(%08lx,%p,%p,%p)\n", cRGB, pwHue, pwLuminance, pwSaturation);

  wR = GetRValue(cRGB);
  wG = GetGValue(cRGB);
  wB = GetBValue(cRGB);

  wMax = max(wR, max(wG, wB));
  wMin = min(wR, min(wG, wB));

  /* Luminosity */
  wLuminosity = ((wMax + wMin) * 240 + 255) / 510;

  if (wMax == wMin)
  {
    /* Achromatic case */
    wSaturation = 0;
    /* Hue is now unrepresentable, but this is what native returns... */
    wHue = 160;
  }
  else
  {
    /* Chromatic case */
    int wDelta = wMax - wMin, wRNorm, wGNorm, wBNorm;

    /* Saturation */
    if (wLuminosity <= 120)
      wSaturation = ((wMax + wMin)/2 + wDelta * 240) / (wMax + wMin);
    else
      wSaturation = ((510 - wMax - wMin)/2 + wDelta * 240) / (510 - wMax - wMin);

    /* Hue */
    wRNorm = (wDelta/2 + wMax * 40 - wR * 40) / wDelta;
    wGNorm = (wDelta/2 + wMax * 40 - wG * 40) / wDelta;
    wBNorm = (wDelta/2 + wMax * 40 - wB * 40) / wDelta;

    if (wR == wMax)
      wHue = wBNorm - wGNorm;
    else if (wG == wMax)
      wHue = 80 + wRNorm - wBNorm;
    else
      wHue = 160 + wGNorm - wRNorm;
    if (wHue < 0)
      wHue += 240;
    else if (wHue > 240)
      wHue -= 240;
  }
  if (pwHue)
    *pwHue = wHue;
  if (pwLuminance)
    *pwLuminance = wLuminosity;
  if (pwSaturation)
    *pwSaturation = wSaturation;
}

/*************************************************************************
 *      SHCreateShellPalette	[SHLWAPI.@]
 */
HPALETTE WINAPI SHCreateShellPalette(HDC hdc)
{
	FIXME("stub\n");
	return CreateHalftonePalette(hdc);
}

/*************************************************************************
 *	SHGetInverseCMAP (SHLWAPI.@)
 *
 * Get an inverse color map table.
 *
 * PARAMS
 *  lpCmap  [O] Destination for color map
 *  dwSize  [I] Size of memory pointed to by lpCmap
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_POINTER,    If lpCmap is invalid.
 *           E_INVALIDARG, If dwFlags is invalid
 *           E_OUTOFMEMORY, If there is no memory available
 *
 * NOTES
 *  dwSize may only be CMAP_PTR_SIZE (4) or CMAP_SIZE (8192).
 *  If dwSize = CMAP_PTR_SIZE, *lpCmap is set to the address of this DLL's
 *  internal CMap.
 *  If dwSize = CMAP_SIZE, lpCmap is filled with a copy of the data from
 *  this DLL's internal CMap.
 */
HRESULT WINAPI SHGetInverseCMAP(LPDWORD dest, DWORD dwSize)
{
    if (dwSize == 4) {
	FIXME(" - returning bogus address for SHGetInverseCMAP\n");
	*dest = (DWORD)0xabba1249;
	return 0;
    }
    FIXME("(%p, %#lx) stub\n", dest, dwSize);
    return 0;
}

/*************************************************************************
 *      SHIsLowMemoryMachine	[SHLWAPI.@]
 *
 * Determine if the current computer has low memory.
 *
 * PARAMS
 *  x [I] FIXME
 *
 * RETURNS
 *  TRUE if the users machine has 16 Megabytes of memory or less,
 *  FALSE otherwise.
 */
BOOL WINAPI SHIsLowMemoryMachine (DWORD x)
{
  FIXME("(0x%08lx) stub\n", x);
  return FALSE;
}

/*************************************************************************
 *      GetMenuPosFromID	[SHLWAPI.@]
 *
 * Return the position of a menu item from its Id.
 *
 * PARAMS
 *   hMenu [I] Menu containing the item
 *   wID   [I] Id of the menu item
 *
 * RETURNS
 *  Success: The index of the menu item in hMenu.
 *  Failure: -1, If the item is not found.
 */
INT WINAPI GetMenuPosFromID(HMENU hMenu, UINT wID)
{
 MENUITEMINFOA mi;
 INT nCount = GetMenuItemCount(hMenu), nIter = 0;

 while (nIter < nCount)
 {
   mi.wID = 0;
   if (!GetMenuItemInfoA(hMenu, nIter, TRUE, &mi) && mi.wID == wID)
     return nIter;
   nIter++;
 }
 return -1;
}

/*************************************************************************
 *      @	[SHLWAPI.179]
 *
 * Same as SHLWAPI.GetMenuPosFromID
 */
DWORD WINAPI SHMenuIndexFromID(HMENU hMenu, UINT uID)
{
    return GetMenuPosFromID(hMenu, uID);
}


/*************************************************************************
 *      @	[SHLWAPI.448]
 */
VOID WINAPI FixSlashesAndColonW(LPWSTR lpwstr)
{
    while (*lpwstr)
    {
        if (*lpwstr == '/')
            *lpwstr = '\\';
        lpwstr++;
    }
}


/*************************************************************************
 *      @	[SHLWAPI.461]
 */
DWORD WINAPI SHGetAppCompatFlags()
{
  FIXME("stub\n");
  return 0;
}


/*************************************************************************
 *      @	[SHLWAPI.549]
 */
HRESULT WINAPI SHCoCreateInstanceAC(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
                                    DWORD dwClsContext, REFIID iid, LPVOID *ppv)
{
    return CoCreateInstance(rclsid, pUnkOuter, dwClsContext, iid, ppv);
}

/*************************************************************************
 * SHSkipJunction	[SHLWAPI.@]
 *
 * Determine if a bind context can be bound to an object
 *
 * PARAMS
 *  pbc    [I] Bind context to check
 *  pclsid [I] CLSID of object to be bound to
 *
 * RETURNS
 *  TRUE: If it is safe to bind
 *  FALSE: If pbc is invalid or binding would not be safe
 *
 */
BOOL WINAPI SHSkipJunction(IBindCtx *pbc, const CLSID *pclsid)
{
  static const WCHAR szSkipBinding[] = { 'S','k','i','p',' ',
    'B','i','n','d','i','n','g',' ','C','L','S','I','D','\0' };
  BOOL bRet = FALSE;

  if (pbc)
  {
    IUnknown* lpUnk;

    if (SUCCEEDED(IBindCtx_GetObjectParam(pbc, (LPOLESTR)szSkipBinding, &lpUnk)))
    {
      CLSID clsid;

      if (SUCCEEDED(IUnknown_GetClassID(lpUnk, &clsid)) &&
          IsEqualGUID(pclsid, &clsid))
        bRet = TRUE;

      IUnknown_Release(lpUnk);
    }
  }
  return bRet;
}

/***********************************************************************
 *		SHGetShellKey (SHLWAPI.@)
 */
DWORD WINAPI SHGetShellKey(DWORD a, DWORD b, DWORD c)
{
    FIXME("(%lx, %lx, %lx): stub\n", a, b, c);
    return 0x50;
}

/***********************************************************************
 *		SHQueueUserWorkItem (SHLWAPI.@)
 */
HRESULT WINAPI SHQueueUserWorkItem(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g)
{
    FIXME("(%lx, %lx, %lx, %lx, %lx, %lx, %lx): stub\n", a, b, c, d, e, f, g);
    return E_FAIL;
}

/***********************************************************************
 *		IUnknown_OnFocusChangeIS (SHLWAPI.@)
 */
DWORD WINAPI IUnknown_OnFocusChangeIS(IUnknown * pUnk, IUnknown * pFocusObject, BOOL bChange)
{
    FIXME("(%p, %p, %s)\n", pUnk, pFocusObject, bChange ? "TRUE" : "FALSE");

/*
    IInputObjectSite * pIOS = NULL;
    if (SUCCEEDED(IUnknown_QueryInterface(pUnk, &IID_IInputObjectSite, (void **)&pIOS))
        IInputObjectSite_OnFocusChangeIS(pIOS, pFocusObject, bChange);
*/

    return 0;
}

/***********************************************************************
 *		SHGetValueW (SHLWAPI.@)
 */
HRESULT WINAPI SKGetValueW(DWORD a, LPWSTR b, LPWSTR c, DWORD d, DWORD e, DWORD f)
{
    FIXME("(%lx, %s, %s, %lx, %lx, %lx): stub\n", a, debugstr_w(b), debugstr_w(c), d, e, f);
    return E_FAIL;
}
