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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "winver.h"
#include "winnetwk.h"
#include "mmsystem.h"
#include "objbase.h"
#include "exdisp.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "shellapi.h"
#include "commdlg.h"
#include "mshtmhst.h"
#include "wine/unicode.h"
#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* DLL handles for late bound calls */
extern HINSTANCE shlwapi_hInstance;
extern DWORD SHLWAPI_ThreadRef_index;

HRESULT WINAPI IUnknown_QueryService(IUnknown*,REFGUID,REFIID,LPVOID*);
HRESULT WINAPI SHInvokeCommand(HWND,IShellFolder*,LPCITEMIDLIST,BOOL);
BOOL    WINAPI SHAboutInfoW(LPWSTR,DWORD);

/*
 NOTES: Most functions exported by ordinal seem to be superfluous.
 The reason for these functions to be there is to provide a wrapper
 for unicode functions to provide these functions on systems without
 unicode functions eg. win95/win98. Since we have such functions we just
 call these. If running Wine with native DLLs, some late bound calls may
 fail. However, it is better to implement the functions in the forward DLL
 and recommend the builtin rather than reimplementing the calls here!
*/

/*************************************************************************
 * SHLWAPI_DupSharedHandle
 *
 * Internal implemetation of SHLWAPI_11.
 */
static HANDLE SHLWAPI_DupSharedHandle(HANDLE hShared, DWORD dwDstProcId,
                                      DWORD dwSrcProcId, DWORD dwAccess,
                                      DWORD dwOptions)
{
  HANDLE hDst, hSrc;
  DWORD dwMyProcId = GetCurrentProcessId();
  HANDLE hRet = NULL;

  TRACE("(%p,%d,%d,%08x,%08x)\n", hShared, dwDstProcId, dwSrcProcId,
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
      if (!DuplicateHandle(hDst, hShared, hSrc, &hRet,
                           dwAccess, 0, dwOptions | DUPLICATE_SAME_ACCESS))
        hRet = NULL;

      if (dwSrcProcId != dwMyProcId)
        CloseHandle(hSrc);
    }

    if (dwDstProcId != dwMyProcId)
      CloseHandle(hDst);
  }

  TRACE("Returning handle %p\n", hRet);
  return hRet;
}

/*************************************************************************
 * @  [SHLWAPI.7]
 *
 * Create a block of sharable memory and initialise it with data.
 *
 * PARAMS
 * lpvData  [I] Pointer to data to write
 * dwSize   [I] Size of data
 * dwProcId [I] ID of process owning data
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
HANDLE WINAPI SHAllocShared(LPCVOID lpvData, DWORD dwSize, DWORD dwProcId)
{
  HANDLE hMap;
  LPVOID pMapped;
  HANDLE hRet = NULL;

  TRACE("(%p,%d,%d)\n", lpvData, dwSize, dwProcId);

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
    if (lpvData)
      memcpy((char *) pMapped + sizeof(dwSize), lpvData, dwSize);

    /* Release view. All further views mapped will be opaque */
    UnmapViewOfFile(pMapped);
    hRet = SHLWAPI_DupSharedHandle(hMap, dwProcId,
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
PVOID WINAPI SHLockShared(HANDLE hShared, DWORD dwProcId)
{
  HANDLE hDup;
  LPVOID pMapped;

  TRACE("(%p %d)\n", hShared, dwProcId);

  /* Get handle to shared memory for current process */
  hDup = SHLWAPI_DupSharedHandle(hShared, dwProcId, GetCurrentProcessId(),
                                 FILE_MAP_ALL_ACCESS, 0);
  /* Get View */
  pMapped = MapViewOfFile(hDup, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
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
BOOL WINAPI SHFreeShared(HANDLE hShared, DWORD dwProcId)
{
  HANDLE hClose;

  TRACE("(%p %d)\n", hShared, dwProcId);

  /* Get a copy of the handle for our process, closing the source handle */
  hClose = SHLWAPI_DupSharedHandle(hShared, dwProcId, GetCurrentProcessId(),
                                   FILE_MAP_ALL_ACCESS,DUPLICATE_CLOSE_SOURCE);
  /* Close local copy */
  return CloseHandle(hClose);
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
HANDLE WINAPI SHMapHandle(HANDLE hShared, DWORD dwDstProcId, DWORD dwSrcProcId,
                          DWORD dwAccess, DWORD dwOptions)
{
  HANDLE hRet;

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
  BSTR property;
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
  property = SysAllocString(szProperty);
  hRet = IWebBrowserApp_GetProperty(pBrowser, property, &var);
  SysFreeString(property);
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
    hRet = CreateFormatEnumerator(dwNumValues, formatList, &pIEnumFormatEtc);

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
    if (hRet == S_OK && pIEnumFormatEtc)
    {
      /* Clone and register the enumerator */
      hRet = IEnumFORMATETC_Clone(pIEnumFormatEtc, &pClone);
      if (hRet == S_OK && pClone)
      {
        RegisterFormatEnumerator(lpBC, pClone, 0);

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
 *      @	[SHLWAPI.15]
 *
 * Get Explorers "AcceptLanguage" setting.
 *
 * PARAMS
 *  langbuf [O] Destination for language string
 *  buflen  [I] Length of langbuf
 *          [0] Success: used length of langbuf
 *
 * RETURNS
 *  Success: S_OK.   langbuf is set to the language string found.
 *  Failure: E_FAIL, If any arguments are invalid, error occurred, or Explorer
 *           does not contain the setting.
 *           E_INVALIDARG, If the buffer is not big enough
 */
HRESULT WINAPI GetAcceptLanguagesW( LPWSTR langbuf, LPDWORD buflen)
{
    static const WCHAR szkeyW[] = {
	'S','o','f','t','w','a','r','e','\\',
	'M','i','c','r','o','s','o','f','t','\\',
	'I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r','\\',
	'I','n','t','e','r','n','a','t','i','o','n','a','l',0};
    static const WCHAR valueW[] = {
	'A','c','c','e','p','t','L','a','n','g','u','a','g','e',0};
    static const WCHAR enusW[] = {'e','n','-','u','s',0};
    DWORD mystrlen, mytype;
    HKEY mykey;
    HRESULT retval;
    LCID mylcid;
    WCHAR *mystr;

    if(!langbuf || !buflen || !*buflen)
	return E_FAIL;

    mystrlen = (*buflen > 20) ? *buflen : 20 ;
    mystr = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * mystrlen);
    RegOpenKeyW(HKEY_CURRENT_USER, szkeyW, &mykey);
    if(RegQueryValueExW(mykey, valueW, 0, &mytype, (PBYTE)mystr, &mystrlen)) {
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
        lstrcpyW(mystr, enusW);
        mystrlen = lstrlenW(mystr);
    } else {
        /* handle returned string */
        FIXME("missing code\n");
    }
    memcpy( langbuf, mystr, min(*buflen,strlenW(mystr)+1)*sizeof(WCHAR) );

    if(*buflen > strlenW(mystr)) {
	*buflen = strlenW(mystr);
	retval = S_OK;
    } else {
	*buflen = 0;
	retval = E_INVALIDARG;
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }
    RegCloseKey(mykey);
    HeapFree(GetProcessHeap(), 0, mystr);
    return retval;
}

/*************************************************************************
 *      @	[SHLWAPI.14]
 *
 * Ascii version of GetAcceptLanguagesW.
 */
HRESULT WINAPI GetAcceptLanguagesA( LPSTR langbuf, LPDWORD buflen)
{
    WCHAR *langbufW;
    DWORD buflenW, convlen;
    HRESULT retval;

    if(!langbuf || !buflen || !*buflen) return E_FAIL;

    buflenW = *buflen;
    langbufW = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * buflenW);
    retval = GetAcceptLanguagesW(langbufW, &buflenW);

    if (retval == S_OK)
    {
        convlen = WideCharToMultiByte(CP_ACP, 0, langbufW, -1, langbuf, *buflen, NULL, NULL);
    }
    else  /* copy partial string anyway */
    {
        convlen = WideCharToMultiByte(CP_ACP, 0, langbufW, *buflen, langbuf, *buflen, NULL, NULL);
        if (convlen < *buflen) langbuf[convlen] = 0;
    }
    *buflen = buflenW ? convlen : 0;

    HeapFree(GetProcessHeap(), 0, langbufW);
    return retval;
}

/*************************************************************************
 *      @	[SHLWAPI.23]
 *
 * Convert a GUID to a string.
 *
 * PARAMS
 *  guid     [I] GUID to convert
 *  lpszDest [O] Destination for string
 *  cchMax   [I] Length of output buffer
 *
 * RETURNS
 *  The length of the string created.
 */
INT WINAPI SHStringFromGUIDA(REFGUID guid, LPSTR lpszDest, INT cchMax)
{
  char xguid[40];
  INT iLen;

  TRACE("(%s,%p,%d)\n", debugstr_guid(guid), lpszDest, cchMax);

  sprintf(xguid, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
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
INT WINAPI SHStringFromGUIDW(REFGUID guid, LPWSTR lpszDest, INT cchMax)
{
  WCHAR xguid[40];
  INT iLen;
  static const WCHAR wszFormat[] = {'{','%','0','8','l','X','-','%','0','4','X','-','%','0','4','X','-',
      '%','0','2','X','%','0','2','X','-','%','0','2','X','%','0','2','X','%','0','2','X','%','0','2',
      'X','%','0','2','X','%','0','2','X','}',0};

  TRACE("(%s,%p,%d)\n", debugstr_guid(guid), lpszDest, cchMax);

  sprintfW(xguid, wszFormat, guid->Data1, guid->Data2, guid->Data3,
          guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
          guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);

  iLen = strlenW(xguid) + 1;

  if (iLen > cchMax)
    return 0;
  memcpy(lpszDest, xguid, iLen*sizeof(WCHAR));
  return iLen;
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
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_SPACE);
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
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_BLANK);
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
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_PUNCT);
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
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_CNTRL);
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
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_DIGIT);
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
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_XDIGIT);
}

/*************************************************************************
 *      @	[SHLWAPI.35]
 *
 */
BOOL WINAPI GetStringType3ExW(LPWSTR src, INT count, LPWORD type)
{
    return GetStringTypeW(CT_CTYPE3, src, count, type);
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
    return StrCmpNA(lpszSrc, lpszCmp, len);
}

/*************************************************************************
 *      @	[SHLWAPI.152]
 *
 * Unicode version of StrCmpNCA.
 */
DWORD WINAPI StrCmpNCW(LPCWSTR lpszSrc, LPCWSTR lpszCmp, INT len)
{
    return StrCmpNW(lpszSrc, lpszCmp, len);
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
    return StrCmpNIA(lpszSrc, lpszCmp, len);
}

/*************************************************************************
 *      @	[SHLWAPI.154]
 *
 * Unicode version of StrCmpNICA.
 */
DWORD WINAPI StrCmpNICW(LPCWSTR lpszSrc, LPCWSTR lpszCmp, DWORD len)
{
    return StrCmpNIW(lpszSrc, lpszCmp, len);
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
    return lstrcmpA(lpszSrc, lpszCmp);
}

/*************************************************************************
 *      @	[SHLWAPI.156]
 *
 * Unicode version of StrCmpCA.
 */
DWORD WINAPI StrCmpCW(LPCWSTR lpszSrc, LPCWSTR lpszCmp)
{
    return lstrcmpW(lpszSrc, lpszCmp);
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
    return lstrcmpiA(lpszSrc, lpszCmp);
}

/*************************************************************************
 *      @	[SHLWAPI.158]
 *
 * Unicode version of StrCmpICA.
 */
DWORD WINAPI StrCmpICW(LPCWSTR lpszSrc, LPCWSTR lpszCmp)
{
    return lstrcmpiW(lpszSrc, lpszCmp);
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

  TRACE("(%p,%d)\n", lpszDest, dwDestLen);

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

  TRACE("(%p,%d)\n", lpszDest, dwDestLen);

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

  TRACE("(%p,%p,%d,%p,%p)\n",lpUnknown, pguidCmdGroup, cCmds, prgCmds, pCmdText);

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

  TRACE("(%p,%p,%d,%d,%p,%p)\n",lpUnknown, pguidCmdGroup, nCmdID,
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
 *           that was advised. The caller is responsible for releasing it.
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

  if (FAILED(IUnknown_QueryInterface(lpInt1, &IID_IUnknown,
                                       (LPVOID *)&lpUnknown1)))
    return FALSE;

  if (FAILED(IUnknown_QueryInterface(lpInt2, &IID_IUnknown,
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

  TRACE("(%p,%d)\n", pUnk, arg);

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
 * Call either IObjectWithSite_SetSite() or IInternetSecurityManager_SetSecuritySite() on
 * an object.
 *
 */
HRESULT WINAPI IUnknown_SetSite(
        IUnknown *obj,        /* [in]   OLE object     */
        IUnknown *site)       /* [in]   Site interface */
{
    HRESULT hr;
    IObjectWithSite *iobjwithsite;
    IInternetSecurityManager *isecmgr;

    if (!obj) return E_FAIL;

    hr = IUnknown_QueryInterface(obj, &IID_IObjectWithSite, (LPVOID *)&iobjwithsite);
    TRACE("IID_IObjectWithSite QI ret=%08x, %p\n", hr, iobjwithsite);
    if (SUCCEEDED(hr))
    {
	hr = IObjectWithSite_SetSite(iobjwithsite, site);
	TRACE("done IObjectWithSite_SetSite ret=%08x\n", hr);
	IUnknown_Release(iobjwithsite);
    }
    else
    {
	hr = IUnknown_QueryInterface(obj, &IID_IInternetSecurityManager, (LPVOID *)&isecmgr);
	TRACE("IID_IInternetSecurityManager QI ret=%08x, %p\n", hr, isecmgr);
	if (FAILED(hr)) return hr;

	hr = IInternetSecurityManager_SetSecuritySite(isecmgr, (IInternetSecurityMgrSite *)site);
	TRACE("done IInternetSecurityManager_SetSecuritySite ret=%08x\n", hr);
	IUnknown_Release(isecmgr);
    }
    return hr;
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

  if (hRet == S_OK && pService)
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
  HMENU hMenu;

  if ((hMenu = LoadMenuW(hInst, szName)))
  {
    if (GetSubMenu(hMenu, 0))
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

  TRACE("(%p,%u,%ld,%ld,%d)\n", hWnd, uiMsgId, wParam, lParam, bSend);

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
 *      @	[SHLWAPI.186]
 */
BOOL WINAPI SHSimulateDrop(IDropTarget *pDrop, IDataObject *pDataObj,
                           DWORD grfKeyState, PPOINTL lpPt, DWORD* pdwEffect)
{
  DWORD dwEffect = DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY;
  POINTL pt = { 0, 0 };

  if (!lpPt)
    lpPt = &pt;

  if (!pdwEffect)
    pdwEffect = &dwEffect;

  IDropTarget_DragEnter(pDrop, pDataObj, grfKeyState, *lpPt, pdwEffect);

  if (*pdwEffect)
    return IDropTarget_Drop(pDrop, pDataObj, grfKeyState, *lpPt, pdwEffect);

  IDropTarget_DragLeave(pDrop);
  return TRUE;
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
 * @  [SHLWAPI.188]
 *
 * Call IOleControlSite_TranslateAccelerator()  on an object.
 *
 * PARAMS
 *  lpUnknown   [I] Object supporting the IOleControlSite interface.
 *  lpMsg       [I] Key message to be processed.
 *  dwModifiers [I] Flags containing the state of the modifier keys.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code, or E_INVALIDARG if lpUnknown is NULL.
 */
HRESULT WINAPI IUnknown_TranslateAcceleratorOCS(IUnknown *lpUnknown, LPMSG lpMsg, DWORD dwModifiers)
{
  IOleControlSite* lpCSite = NULL;
  HRESULT hRet = E_INVALIDARG;

  TRACE("(%p,%p,0x%08x)\n", lpUnknown, lpMsg, dwModifiers);
  if (lpUnknown)
  {
    hRet = IUnknown_QueryInterface(lpUnknown, &IID_IOleControlSite,
                                   (void**)&lpCSite);
    if (SUCCEEDED(hRet) && lpCSite)
    {
      hRet = IOleControlSite_TranslateAccelerator(lpCSite, lpMsg, dwModifiers);
      IOleControlSite_Release(lpCSite);
    }
  }
  return hRet;
}


/*************************************************************************
 * @  [SHLWAPI.189]
 *
 * Call IOleControlSite_OnFocus() on an object.
 *
 * PARAMS
 *  lpUnknown [I] Object supporting the IOleControlSite interface.
 *  fGotFocus [I] Whether focus was gained (TRUE) or lost (FALSE).
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code, or E_FAIL if lpUnknown is NULL.
 */
HRESULT WINAPI IUnknown_OnFocusOCS(IUnknown *lpUnknown, BOOL fGotFocus)
{
  IOleControlSite* lpCSite = NULL;
  HRESULT hRet = E_FAIL;

  TRACE("(%p,%s)\n", lpUnknown, fGotFocus ? "TRUE" : "FALSE");
  if (lpUnknown)
  {
    hRet = IUnknown_QueryInterface(lpUnknown, &IID_IOleControlSite,
                                   (void**)&lpCSite);
    if (SUCCEEDED(hRet) && lpCSite)
    {
      hRet = IOleControlSite_OnFocus(lpCSite, fGotFocus);
      IOleControlSite_Release(lpCSite);
    }
  }
  return hRet;
}

/*************************************************************************
 * @    [SHLWAPI.190]
 */
HRESULT WINAPI IUnknown_HandleIRestrict(LPUNKNOWN lpUnknown, PVOID lpArg1,
                                        PVOID lpArg2, PVOID lpArg3, PVOID lpArg4)
{
  /* FIXME: {D12F26B2-D90A-11D0-830D-00AA005B4383} - What object does this represent? */
  static const DWORD service_id[] = { 0xd12f26b2, 0x11d0d90a, 0xaa000d83, 0x83435b00 };
  /* FIXME: {D12F26B1-D90A-11D0-830D-00AA005B4383} - Also Unknown/undocumented */
  static const DWORD function_id[] = { 0xd12f26b1, 0x11d0d90a, 0xaa000d83, 0x83435b00 };
  HRESULT hRet = E_INVALIDARG;
  LPUNKNOWN lpUnkInner = NULL; /* FIXME: Real type is unknown */

  TRACE("(%p,%p,%p,%p,%p)\n", lpUnknown, lpArg1, lpArg2, lpArg3, lpArg4);

  if (lpUnknown && lpArg4)
  {
     hRet = IUnknown_QueryService(lpUnknown, (REFGUID)service_id,
                                  (REFGUID)function_id, (void**)&lpUnkInner);

     if (SUCCEEDED(hRet) && lpUnkInner)
     {
       /* FIXME: The type of service object requested is unknown, however
	* testing shows that its first method is called with 4 parameters.
	* Fake this by using IParseDisplayName_ParseDisplayName since the
	* signature and position in the vtable matches our unknown object type.
	*/
       hRet = IParseDisplayName_ParseDisplayName((LPPARSEDISPLAYNAME)lpUnkInner,
                                                 lpArg1, lpArg2, lpArg3, lpArg4);
       IUnknown_Release(lpUnkInner);
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
  MENUITEMINFOW mi;

  TRACE("(%p,%u)\n", hMenu, uID);

  mi.cbSize = sizeof(mi);
  mi.fMask = MIIM_SUBMENU;

  if (!GetMenuItemInfoW(hMenu, uID, FALSE, &mi))
    return NULL;

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
DWORD WINAPI SHGetCurColorRes(void)
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
 *      @       [SHLWAPI.195]
 *
 * Determine if a shell folder can be expanded.
 *
 * PARAMS
 *  lpFolder [I] Parent folder containing the object to test.
 *  pidl     [I] Id of the object to test.
 *
 * RETURNS
 *  Success: S_OK, if the object is expandable, S_FALSE otherwise.
 *  Failure: E_INVALIDARG, if any argument is invalid.
 *
 * NOTES
 *  If the object to be tested does not expose the IQueryInfo() interface it
 *  will not be identified as an expandable folder.
 */
HRESULT WINAPI SHIsExpandableFolder(LPSHELLFOLDER lpFolder, LPCITEMIDLIST pidl)
{
  HRESULT hRet = E_INVALIDARG;
  IQueryInfo *lpInfo;

  if (lpFolder && pidl)
  {
    hRet = IShellFolder_GetUIObjectOf(lpFolder, NULL, 1, &pidl, &IID_IQueryInfo,
                                      NULL, (void**)&lpInfo);
    if (FAILED(hRet))
      hRet = S_FALSE; /* Doesn't expose IQueryInfo */
    else
    {
      DWORD dwFlags = 0;

      /* MSDN states of IQueryInfo_GetInfoFlags() that "This method is not
       * currently used". Really? You wouldn't be holding out on me would you?
       */
      hRet = IQueryInfo_GetInfoFlags(lpInfo, &dwFlags);

      if (SUCCEEDED(hRet))
      {
        /* 0x2 is an undocumented flag apparently indicating expandability */
        hRet = dwFlags & 0x2 ? S_OK : S_FALSE;
      }

      IQueryInfo_Release(lpInfo);
    }
  }
  return hRet;
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
 * Return the value associated with a key in a map.
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
 *      @	[SHLWAPI.200]
 *
 */
HRESULT WINAPI MayQSForward(IUnknown* lpUnknown, PVOID lpReserved,
                            REFGUID riidCmdGrp, ULONG cCmds,
                            OLECMD *prgCmds, OLECMDTEXT* pCmdText)
{
  FIXME("(%p,%p,%p,%d,%p,%p) - stub\n",
        lpUnknown, lpReserved, riidCmdGrp, cCmds, prgCmds, pCmdText);

  /* FIXME: Calls IsQSForward & IUnknown_QueryStatus */
  return DRAGDROP_E_NOTREGISTERED;
}

/*************************************************************************
 *      @	[SHLWAPI.201]
 *
 */
HRESULT WINAPI MayExecForward(IUnknown* lpUnknown, INT iUnk, REFGUID pguidCmdGroup,
                           DWORD nCmdID, DWORD nCmdexecopt, VARIANT* pvaIn,
                           VARIANT* pvaOut)
{
  FIXME("(%p,%d,%p,%d,%d,%p,%p) - stub!\n", lpUnknown, iUnk, pguidCmdGroup,
        nCmdID, nCmdexecopt, pvaIn, pvaOut);
  return DRAGDROP_E_NOTREGISTERED;
}

/*************************************************************************
 *      @	[SHLWAPI.202]
 *
 */
HRESULT WINAPI IsQSForward(REFGUID pguidCmdGroup,ULONG cCmds, OLECMD *prgCmds)
{
  FIXME("(%p,%d,%p) - stub!\n", pguidCmdGroup, cCmds, prgCmds);
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
 *    FDSA functions.  Manage a dynamic array of fixed size memory blocks.
 */

typedef struct
{
    DWORD num_items;       /* Number of elements inserted */
    void *mem;             /* Ptr to array */
    DWORD blocks_alloced;  /* Number of elements allocated */
    BYTE inc;              /* Number of elements to grow by when we need to expand */
    BYTE block_size;       /* Size in bytes of an element */
    BYTE flags;            /* Flags */
} FDSA_info;

#define FDSA_FLAG_INTERNAL_ALLOC 0x01 /* When set we have allocated mem internally */

/*************************************************************************
 *      @	[SHLWAPI.208]
 *
 * Initialize an FDSA array.
 */
BOOL WINAPI FDSA_Initialize(DWORD block_size, DWORD inc, FDSA_info *info, void *mem,
                            DWORD init_blocks)
{
    TRACE("(0x%08x 0x%08x %p %p 0x%08x)\n", block_size, inc, info, mem, init_blocks);

    if(inc == 0)
        inc = 1;

    if(mem)
        memset(mem, 0, block_size * init_blocks);
    
    info->num_items = 0;
    info->inc = inc;
    info->mem = mem;
    info->blocks_alloced = init_blocks;
    info->block_size = block_size;
    info->flags = 0;

    return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.209]
 *
 * Destroy an FDSA array
 */
BOOL WINAPI FDSA_Destroy(FDSA_info *info)
{
    TRACE("(%p)\n", info);

    if(info->flags & FDSA_FLAG_INTERNAL_ALLOC)
    {
        HeapFree(GetProcessHeap(), 0, info->mem);
        return FALSE;
    }

    return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.210]
 *
 * Insert element into an FDSA array
 */
DWORD WINAPI FDSA_InsertItem(FDSA_info *info, DWORD where, const void *block)
{
    TRACE("(%p 0x%08x %p)\n", info, where, block);
    if(where > info->num_items)
        where = info->num_items;

    if(info->num_items >= info->blocks_alloced)
    {
        DWORD size = (info->blocks_alloced + info->inc) * info->block_size;
        if(info->flags & 0x1)
            info->mem = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, info->mem, size);
        else
        {
            void *old_mem = info->mem;
            info->mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
            memcpy(info->mem, old_mem, info->blocks_alloced * info->block_size);
        }
        info->blocks_alloced += info->inc;
        info->flags |= 0x1;
    }

    if(where < info->num_items)
    {
        memmove((char*)info->mem + (where + 1) * info->block_size,
                (char*)info->mem + where * info->block_size,
                (info->num_items - where) * info->block_size);
    }
    memcpy((char*)info->mem + where * info->block_size, block, info->block_size);

    info->num_items++;
    return where;
}

/*************************************************************************
 *      @	[SHLWAPI.211]
 *
 * Delete an element from an FDSA array.
 */
BOOL WINAPI FDSA_DeleteItem(FDSA_info *info, DWORD where)
{
    TRACE("(%p 0x%08x)\n", info, where);

    if(where >= info->num_items)
        return FALSE;

    if(where < info->num_items - 1)
    {
        memmove((char*)info->mem + where * info->block_size,
                (char*)info->mem + (where + 1) * info->block_size,
                (info->num_items - where - 1) * info->block_size);
    }
    memset((char*)info->mem + (info->num_items - 1) * info->block_size,
           0, info->block_size);
    info->num_items--;
    return TRUE;
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
		TRACE("trying (indx %d) %s\n", xmove->indx, debugstr_guid(xmove->refid));
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

	TRACE("-- 0x%08x\n", ret);
	return ret;
}

/*************************************************************************
 * @ [SHLWAPI.220]
 *
 * Set the Font for a window and the "PropDlgFont" property of the parent window.
 *
 * PARAMS
 *  hWnd [I] Parent Window to set the property
 *  id   [I] Index of child Window to set the Font
 *
 * RETURNS
 *  Success: S_OK
 *
 */
HRESULT WINAPI SHSetDefaultDialogFont(HWND hWnd, INT id)
{
    FIXME("(%p, %d) stub\n", hWnd, id);
    return S_OK;
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
 *      @       [SHLWAPI.256]
 */
HRESULT WINAPI IUnknown_GetSite(LPUNKNOWN lpUnknown, REFIID iid, PVOID *lppSite)
{
  HRESULT hRet = E_INVALIDARG;
  LPOBJECTWITHSITE lpSite = NULL;

  TRACE("(%p,%s,%p)\n", lpUnknown, debugstr_guid(iid), lppSite);

  if (lpUnknown && iid && lppSite)
  {
    hRet = IUnknown_QueryInterface(lpUnknown, &IID_IObjectWithSite,
                                   (void**)&lpSite);
    if (SUCCEEDED(hRet) && lpSite)
    {
      hRet = IObjectWithSite_GetSite(lpSite, iid, lppSite);
      IObjectWithSite_Release(lpSite);
    }
  }
  return hRet;
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
  static const char szClass[] = "WorkerA";
  WNDCLASSA wc;
  HWND hWnd;

  TRACE("(0x%08x,%p,0x%08x,0x%08x,%p,0x%08x)\n",
         wndProc, hWndParent, dwExStyle, dwStyle, hMenu, z);

  /* Create Window class */
  wc.style         = 0;
  wc.lpfnWndProc   = DefWindowProcA;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 4;
  wc.hInstance     = shlwapi_hInstance;
  wc.hIcon         = NULL;
  wc.hCursor       = LoadCursorA(NULL, (LPSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = szClass;

  SHRegisterClassA(&wc); /* Register class */

  /* FIXME: Set extra bits in dwExStyle */

  hWnd = CreateWindowExA(dwExStyle, szClass, 0, dwStyle, 0, 0, 0, 0,
                         hWndParent, hMenu, shlwapi_hInstance, 0);
  if (hWnd)
  {
    SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, z);

    if (wndProc)
      SetWindowLongPtrA(hWnd, GWLP_WNDPROC, wndProc);
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
 *  different POLICYDATA structure and implements a similar algorithm adapted to
 *  that structure.
 */
DWORD WINAPI SHRestrictionLookup(
	DWORD policy,
	LPCWSTR initial,
	LPPOLICYDATA polTable,
	LPDWORD polArr)
{
	TRACE("(0x%08x %s %p %p)\n", policy, debugstr_w(initial), polTable, polArr);

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
	TRACE("unknown policy: (%08x)\n", policy);
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
 *   This QueryInterface asks the inner object for an interface. In case
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
	TRACE("-- 0x%08x\n", hret);
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
  return SUCCEEDED(CLSIDFromString(wClsid, id));
}

/*************************************************************************
 *      @	[SHLWAPI.270]
 *
 * Unicode version of GUIDFromStringA.
 */
BOOL WINAPI GUIDFromStringW(LPCWSTR idstr, CLSID *id)
{
    return SUCCEEDED(CLSIDFromString((LPOLESTR)idstr, id));
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
DWORD WINAPI WhichPlatform(void)
{
  static const char szIntegratedBrowser[] = "IntegratedBrowser";
  static DWORD dwState = 0;
  HKEY hKey;
  DWORD dwRet, dwData, dwSize;
  HMODULE hshell32;

  if (dwState)
    return dwState;

  /* If shell32 exports DllGetVersion(), the browser is integrated */
  dwState = 1;
  hshell32 = LoadLibraryA("shell32.dll");
  if (hshell32)
  {
    FARPROC pDllGetVersion;
    pDllGetVersion = GetProcAddress(hshell32, "DllGetVersion");
    dwState = pDllGetVersion ? 2 : 1;
    FreeLibrary(hshell32);
  }

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

  TRACE("(0x%08x,%p,0x%08x,0x%08x,%p,0x%08x)\n",
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
  wc.hIcon         = NULL;
  wc.hCursor       = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = szClass;

  SHRegisterClassW(&wc); /* Register class */

  /* FIXME: Set extra bits in dwExStyle */

  hWnd = CreateWindowExW(dwExStyle, szClass, 0, dwStyle, 0, 0, 0, 0,
                         hWndParent, hMenu, shlwapi_hInstance, 0);
  if (hWnd)
  {
    SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, z);

    if (wndProc)
      SetWindowLongPtrW(hWnd, GWLP_WNDPROC, wndProc);
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
HRESULT WINAPI SHPackDispParamsV(DISPPARAMS *params, VARIANTARG *args, UINT cnt, __ms_va_list valist)
{
  VARIANTARG *iter;

  TRACE("(%p %p %u ...)\n", params, args, cnt);

  params->rgvarg = args;
  params->rgdispidNamedArgs = NULL;
  params->cArgs = cnt;
  params->cNamedArgs = 0;

  iter = args+cnt;

  while(iter-- > args) {
    V_VT(iter) = va_arg(valist, enum VARENUM);

    TRACE("vt=%d\n", V_VT(iter));

    if(V_VT(iter) & VT_BYREF) {
      V_BYREF(iter) = va_arg(valist, LPVOID);
    } else {
      switch(V_VT(iter)) {
      case VT_I4:
        V_I4(iter) = va_arg(valist, LONG);
        break;
      case VT_BSTR:
        V_BSTR(iter) = va_arg(valist, BSTR);
        break;
      case VT_DISPATCH:
        V_DISPATCH(iter) = va_arg(valist, IDispatch*);
        break;
      case VT_BOOL:
        V_BOOL(iter) = va_arg(valist, int);
        break;
      case VT_UNKNOWN:
        V_UNKNOWN(iter) = va_arg(valist, IUnknown*);
        break;
      default:
        V_VT(iter) = VT_I4;
        V_I4(iter) = va_arg(valist, LONG);
      }
    }
  }

  return S_OK;
}

/*************************************************************************
 *      @       [SHLWAPI.282]
 *
 * SHPackDispParams
 */
HRESULT WINAPIV SHPackDispParams(DISPPARAMS *params, VARIANTARG *args, UINT cnt, ...)
{
  __ms_va_list valist;
  HRESULT hres;

  __ms_va_start(valist, cnt);
  hres = SHPackDispParamsV(params, args, cnt, valist);
  __ms_va_end(valist);
  return hres;
}

/*************************************************************************
 *      SHLWAPI_InvokeByIID
 *
 *   This helper function calls IDispatch::Invoke for each sink
 * which implements given iid or IDispatch.
 *
 */
static HRESULT SHLWAPI_InvokeByIID(
        IConnectionPoint* iCP,
        REFIID iid,
        DISPID dispId,
        DISPPARAMS* dispParams)
{
  IEnumConnections *enumerator;
  CONNECTDATA rgcd;

  HRESULT result = IConnectionPoint_EnumConnections(iCP, &enumerator);
  if (FAILED(result))
    return result;

  while(IEnumConnections_Next(enumerator, 1, &rgcd, NULL)==S_OK)
  {
    IDispatch *dispIface;
    if (SUCCEEDED(IUnknown_QueryInterface(rgcd.pUnk, iid, (LPVOID*)&dispIface)) ||
        SUCCEEDED(IUnknown_QueryInterface(rgcd.pUnk, &IID_IDispatch, (LPVOID*)&dispIface)))
    {
      IDispatch_Invoke(dispIface, dispId, &IID_NULL, 0, DISPATCH_METHOD, dispParams, NULL, NULL, NULL);
      IDispatch_Release(dispIface);
    }
  }

  IEnumConnections_Release(enumerator);

  return S_OK;
}

/*************************************************************************
 *      @	[SHLWAPI.284]
 *
 *  IConnectionPoint_SimpleInvoke
 */
HRESULT WINAPI IConnectionPoint_SimpleInvoke(
        IConnectionPoint* iCP,
        DISPID dispId,
        DISPPARAMS* dispParams)
{
  IID iid;
  HRESULT result;

  TRACE("(%p)->(0x%x %p)\n",iCP,dispId,dispParams);

  result = IConnectionPoint_GetConnectionInterface(iCP, &iid);
  if (SUCCEEDED(result))
    result = SHLWAPI_InvokeByIID(iCP, &iid, dispId, dispParams);

  return result;
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

  TRACE("(%p,0x%8X)\n", lpCP, dispID);

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
 *      @	[SHLWAPI.286]
 *
 *  IUnknown_CPContainerInvokeParam
 */
HRESULT WINAPIV IUnknown_CPContainerInvokeParam(
        IUnknown *container,
        REFIID riid,
        DISPID dispId,
        VARIANTARG* buffer,
        DWORD cParams, ...)
{
  HRESULT result;
  IConnectionPoint *iCP;
  IConnectionPointContainer *iCPC;
  DISPPARAMS dispParams = {buffer, NULL, cParams, 0};
  __ms_va_list valist;

  if (!container)
    return E_NOINTERFACE;

  result = IUnknown_QueryInterface(container, &IID_IConnectionPointContainer,(LPVOID*) &iCPC);
  if (FAILED(result))
      return result;

  result = IConnectionPointContainer_FindConnectionPoint(iCPC, riid, &iCP);
  IConnectionPointContainer_Release(iCPC);
  if(FAILED(result))
      return result;

  __ms_va_start(valist, cParams);
  SHPackDispParamsV(&dispParams, buffer, cParams, valist);
  __ms_va_end(valist);

  result = SHLWAPI_InvokeByIID(iCP, riid, dispId, &dispParams);
  IConnectionPoint_Release(iCP);

  return result;
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

  TRACE("(%p,0x%8X)\n", lpUnknown, dispID);

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
    return PlaySoundW(pszSound, hmod, fdwSound);
}

/*************************************************************************
 *      @	[SHLWAPI.294]
 */
BOOL WINAPI SHGetIniStringW(LPCWSTR str1, LPCWSTR str2, LPWSTR pStr, DWORD some_len, LPCWSTR lpStr2)
{
    FIXME("(%s,%s,%p,%08x,%s): stub!\n", debugstr_w(str1), debugstr_w(str2),
        pStr, some_len, debugstr_w(lpStr2));
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
    FIXME("(%s, %p, %s, %s), stub.\n", debugstr_w(str1), x, debugstr_w(str2), debugstr_w(str3));
    return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.313]
 *
 * See SHGetFileInfoW.
 */
DWORD WINAPI SHGetFileInfoWrapW(LPCWSTR path, DWORD dwFileAttributes,
                         SHFILEINFOW *psfi, UINT sizeofpsfi, UINT flags)
{
    return SHGetFileInfoW(path, dwFileAttributes, psfi, sizeofpsfi, flags);
}

/*************************************************************************
 *      @	[SHLWAPI.318]
 *
 * See DragQueryFileW.
 */
UINT WINAPI DragQueryFileWrapW(HDROP hDrop, UINT lFile, LPWSTR lpszFile, UINT lLength)
{
    return DragQueryFileW(hDrop, lFile, lpszFile, lLength);
}

/*************************************************************************
 *      @	[SHLWAPI.333]
 *
 * See SHBrowseForFolderW.
 */
LPITEMIDLIST WINAPI SHBrowseForFolderWrapW(LPBROWSEINFOW lpBi)
{
    return SHBrowseForFolderW(lpBi);
}

/*************************************************************************
 *      @	[SHLWAPI.334]
 *
 * See SHGetPathFromIDListW.
 */
BOOL WINAPI SHGetPathFromIDListWrapW(LPCITEMIDLIST pidl,LPWSTR pszPath)
{
    return SHGetPathFromIDListW(pidl, pszPath);
}

/*************************************************************************
 *      @	[SHLWAPI.335]
 *
 * See ShellExecuteExW.
 */
BOOL WINAPI ShellExecuteExWrapW(LPSHELLEXECUTEINFOW lpExecInfo)
{
    return ShellExecuteExW(lpExecInfo);
}

/*************************************************************************
 *      @	[SHLWAPI.336]
 *
 * See SHFileOperationW.
 */
INT WINAPI SHFileOperationWrapW(LPSHFILEOPSTRUCTW lpFileOp)
{
    return SHFileOperationW(lpFileOp);
}

/*************************************************************************
 *      @	[SHLWAPI.342]
 *
 */
PVOID WINAPI SHInterlockedCompareExchange( PVOID *dest, PVOID xchg, PVOID compare )
{
    return InterlockedCompareExchangePointer( dest, xchg, compare );
}

/*************************************************************************
 *      @	[SHLWAPI.350]
 *
 * See GetFileVersionInfoSizeW.
 */
DWORD WINAPI GetFileVersionInfoSizeWrapW( LPCWSTR filename, LPDWORD handle )
{
    return GetFileVersionInfoSizeW( filename, handle );
}

/*************************************************************************
 *      @	[SHLWAPI.351]
 *
 * See GetFileVersionInfoW.
 */
BOOL  WINAPI GetFileVersionInfoWrapW( LPCWSTR filename, DWORD handle,
                                      DWORD datasize, LPVOID data )
{
    return GetFileVersionInfoW( filename, handle, datasize, data );
}

/*************************************************************************
 *      @	[SHLWAPI.352]
 *
 * See VerQueryValueW.
 */
WORD WINAPI VerQueryValueWrapW( LPVOID pBlock, LPCWSTR lpSubBlock,
                                LPVOID *lplpBuffer, UINT *puLen )
{
    return VerQueryValueW( pBlock, lpSubBlock, lplpBuffer, puLen );
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
 *  the IDocHostUIHandler interface, or the IOleInPlaceActiveObject interface,
 *  or this call will fail.
 */
HRESULT WINAPI IUnknown_EnableModeless(IUnknown *lpUnknown, BOOL bModeless)
{
  IUnknown *lpObj;
  HRESULT hRet;

  TRACE("(%p,%d)\n", lpUnknown, bModeless);

  if (!lpUnknown)
    return E_FAIL;

  if (IsIface(IOleInPlaceActiveObject))
    EnableModeless(IOleInPlaceActiveObject);
  else if (IsIface(IOleInPlaceFrame))
    EnableModeless(IOleInPlaceFrame);
  else if (IsIface(IShellBrowser))
    EnableModeless(IShellBrowser);
  else if (IsIface(IInternetSecurityMgrSite))
    EnableModeless(IInternetSecurityMgrSite);
  else if (IsIface(IDocHostUIHandler))
    EnableModeless(IDocHostUIHandler);
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
    return SHGetNewLinkInfoW(pszLinkTo, pszDir, pszName, pfMustCopy, uFlags);
}

/*************************************************************************
 *      @	[SHLWAPI.358]
 *
 * See SHDefExtractIconW.
 */
UINT WINAPI SHDefExtractIconWrapW(LPCWSTR pszIconFile, int iIndex, UINT uFlags, HICON* phiconLarge,
                         HICON* phiconSmall, UINT nIconSize)
{
    return SHDefExtractIconW(pszIconFile, iIndex, uFlags, phiconLarge, phiconSmall, nIconSize);
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
    return ExtractIconW(hInstance, lpszExeFileName, nIconIndex);
}

/*************************************************************************
 *      @	[SHLWAPI.377]
 *
 * Load a library from the directory of a particular process.
 *
 * PARAMS
 *  new_mod        [I] Library name
 *  inst_hwnd      [I] Module whose directory is to be used
 *  dwCrossCodePage [I] Should be FALSE (currently ignored)
 *
 * RETURNS
 *  Success: A handle to the loaded module
 *  Failure: A NULL handle.
 */
HMODULE WINAPI MLLoadLibraryA(LPCSTR new_mod, HMODULE inst_hwnd, DWORD dwCrossCodePage)
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

    FIXME("(%s,%p,%d) semi-stub!\n", debugstr_a(new_mod), inst_hwnd, dwCrossCodePage);
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
HMODULE WINAPI MLLoadLibraryW(LPCWSTR new_mod, HMODULE inst_hwnd, DWORD dwCrossCodePage)
{
    WCHAR mod_path[2*MAX_PATH];
    LPWSTR ptr;
    DWORD len;

    FIXME("(%s,%p,%d) semi-stub!\n", debugstr_w(new_mod), inst_hwnd, dwCrossCodePage);
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
  TRACE("(0x%8x,%d,%d)\n", cRGB, dwLuma, bUnknown);

  if (dwLuma)
  {
    WORD wH, wL, wS;

    ColorRGBToHLS(cRGB, &wH, &wL, &wS);

    FIXME("Ignoring luma adjustment\n");

    /* FIXME: The adjustment is not linear */

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
    return GetSaveFileNameW(ofn);
}

/*************************************************************************
 *      @	[SHLWAPI.390]
 *
 * See WNetRestoreConnectionW.
 */
DWORD WINAPI WNetRestoreConnectionWrapW(HWND hwndOwner, LPWSTR lpszDevice)
{
    return WNetRestoreConnectionW(hwndOwner, lpszDevice);
}

/*************************************************************************
 *      @	[SHLWAPI.391]
 *
 * See WNetGetLastErrorW.
 */
DWORD WINAPI WNetGetLastErrorWrapW(LPDWORD lpError, LPWSTR lpErrorBuf, DWORD nErrorBufSize,
                         LPWSTR lpNameBuf, DWORD nNameBufSize)
{
    return WNetGetLastErrorW(lpError, lpErrorBuf, nErrorBufSize, lpNameBuf, nNameBufSize);
}

/*************************************************************************
 *      @	[SHLWAPI.401]
 *
 * See PageSetupDlgW.
 */
BOOL WINAPI PageSetupDlgWrapW(LPPAGESETUPDLGW pagedlg)
{
    return PageSetupDlgW(pagedlg);
}

/*************************************************************************
 *      @	[SHLWAPI.402]
 *
 * See PrintDlgW.
 */
BOOL WINAPI PrintDlgWrapW(LPPRINTDLGW printdlg)
{
    return PrintDlgW(printdlg);
}

/*************************************************************************
 *      @	[SHLWAPI.403]
 *
 * See GetOpenFileNameW.
 */
BOOL WINAPI GetOpenFileNameWrapW(LPOPENFILENAMEW ofn)
{
    return GetOpenFileNameW(ofn);
}

/*************************************************************************
 *      @	[SHLWAPI.404]
 */
HRESULT WINAPI SHIShellFolder_EnumObjects(LPSHELLFOLDER lpFolder, HWND hwnd, SHCONTF flags, IEnumIDList **ppenum)
{
    IPersist *persist;
    HRESULT hr;

    hr = IShellFolder_QueryInterface(lpFolder, &IID_IPersist, (LPVOID)&persist);
    if(SUCCEEDED(hr))
    {
        CLSID clsid;
        hr = IPersist_GetClassID(persist, &clsid);
        if(SUCCEEDED(hr))
        {
            if(IsEqualCLSID(&clsid, &CLSID_ShellFSFolder))
                hr = IShellFolder_EnumObjects(lpFolder, hwnd, flags, ppenum);
            else
                hr = E_FAIL;
        }
        IPersist_Release(persist);
    }
    return hr;
}

/* INTERNAL: Map from HLS color space to RGB */
static WORD ConvertHue(int wHue, WORD wMid1, WORD wMid2)
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

  TRACE("(0x%08x)\n", dwFlags);

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
	FIXME("(0x%08x)stub\n", x);
	return 0xabba1247;
}

/*************************************************************************
 * @ [SHLWAPI.432]
 *
 * See SHSendMessageBroadcastW
 *
 */
DWORD WINAPI SHSendMessageBroadcastA(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return SendMessageTimeoutA(HWND_BROADCAST, uMsg, wParam, lParam,
                               SMTO_ABORTIFHUNG, 2000, NULL);
}

/*************************************************************************
 * @ [SHLWAPI.433]
 *
 * A wrapper for sending Broadcast Messages to all top level Windows
 *
 */
DWORD WINAPI SHSendMessageBroadcastW(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return SendMessageTimeoutW(HWND_BROADCAST, uMsg, wParam, lParam,
                               SMTO_ABORTIFHUNG, 2000, NULL);
}

/*************************************************************************
 *      @	[SHLWAPI.436]
 *
 * Convert a Unicode string CLSID into a CLSID.
 *
 * PARAMS
 *  idstr      [I]   string containing a CLSID in text form
 *  id         [O]   CLSID extracted from the string
 *
 * RETURNS
 *  S_OK on success or E_INVALIDARG on failure
 */
HRESULT WINAPI CLSIDFromStringWrap(LPCWSTR idstr, CLSID *id)
{
    return CLSIDFromString((LPOLESTR)idstr, id);
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
BOOL WINAPI IsOS(DWORD feature)
{
    OSVERSIONINFOA osvi;
    DWORD platform, majorv, minorv;

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    if(!GetVersionExA(&osvi))  {
        ERR("GetVersionEx failed\n");
        return FALSE;
    }

    majorv = osvi.dwMajorVersion;
    minorv = osvi.dwMinorVersion;
    platform = osvi.dwPlatformId;

#define ISOS_RETURN(x) \
    TRACE("(0x%x) ret=%d\n",feature,(x)); \
    return (x);

    switch(feature)  {
    case OS_WIN32SORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32s
                 || platform == VER_PLATFORM_WIN32_WINDOWS)
    case OS_NT:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT)
    case OS_WIN95ORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_WINDOWS)
    case OS_NT4ORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && majorv >= 4)
    case OS_WIN2000ORGREATER_ALT:
    case OS_WIN2000ORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && majorv >= 5)
    case OS_WIN98ORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_WINDOWS && minorv >= 10)
    case OS_WIN98_GOLD:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_WINDOWS && minorv == 10)
    case OS_WIN2000PRO:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && majorv >= 5)
    case OS_WIN2000SERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && (minorv == 0 || minorv == 1))
    case OS_WIN2000ADVSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && (minorv == 0 || minorv == 1))
    case OS_WIN2000DATACENTER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && (minorv == 0 || minorv == 1))
    case OS_WIN2000TERMINAL:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && (minorv == 0 || minorv == 1))
    case OS_EMBEDDED:
        FIXME("(OS_EMBEDDED) What should we return here?\n");
        return FALSE;
    case OS_TERMINALCLIENT:
        FIXME("(OS_TERMINALCLIENT) What should we return here?\n");
        return FALSE;
    case OS_TERMINALREMOTEADMIN:
        FIXME("(OS_TERMINALREMOTEADMIN) What should we return here?\n");
        return FALSE;
    case OS_WIN95_GOLD:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_WINDOWS && minorv == 0)
    case OS_MEORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_WINDOWS && minorv >= 90)
    case OS_XPORGREATER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && majorv >= 5 && minorv >= 1)
    case OS_HOME:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && majorv >= 5 && minorv >= 1)
    case OS_PROFESSIONAL:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT)
    case OS_DATACENTER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT)
    case OS_ADVSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && majorv >= 5)
    case OS_SERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT)
    case OS_TERMINALSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT)
    case OS_PERSONALTERMINALSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT && minorv >= 1 && majorv >= 5)
    case OS_FASTUSERSWITCHING:
        FIXME("(OS_FASTUSERSWITCHING) What should we return here?\n");
        return TRUE;
    case OS_WELCOMELOGONUI:
        FIXME("(OS_WELCOMELOGONUI) What should we return here?\n");
        return FALSE;
    case OS_DOMAINMEMBER:
        FIXME("(OS_DOMAINMEMBER) What should we return here?\n");
        return TRUE;
    case OS_ANYSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT)
    case OS_WOW6432:
        FIXME("(OS_WOW6432) Should we check this?\n");
        return FALSE;
    case OS_WEBSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT)
    case OS_SMALLBUSINESSSERVER:
        ISOS_RETURN(platform == VER_PLATFORM_WIN32_NT)
    case OS_TABLETPC:
        FIXME("(OS_TABLEPC) What should we return here?\n");
        return FALSE;
    case OS_SERVERADMINUI:
        FIXME("(OS_SERVERADMINUI) What should we return here?\n");
        return FALSE;
    case OS_MEDIACENTER:
        FIXME("(OS_MEDIACENTER) What should we return here?\n");
        return FALSE;
    case OS_APPLIANCE:
        FIXME("(OS_APPLIANCE) What should we return here?\n");
        return FALSE;
    }

#undef ISOS_RETURN

    WARN("(0x%x) unknown parameter\n",feature);

    return FALSE;
}

/*************************************************************************
 * @  [SHLWAPI.439]
 */
HRESULT WINAPI SHLoadRegUIStringW(HKEY hkey, LPCWSTR value, LPWSTR buf, DWORD size)
{
    DWORD type, sz = size;

    if(RegQueryValueExW(hkey, value, NULL, &type, (LPBYTE)buf, &sz) != ERROR_SUCCESS)
        return E_FAIL;

    return SHLoadIndirectString(buf, buf, size, NULL);
}

/*************************************************************************
 * @  [SHLWAPI.478]
 *
 * Call IInputObject_TranslateAcceleratorIO() on an object.
 *
 * PARAMS
 *  lpUnknown [I] Object supporting the IInputObject interface.
 *  lpMsg     [I] Key message to be processed.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code, or E_INVALIDARG if lpUnknown is NULL.
 */
HRESULT WINAPI IUnknown_TranslateAcceleratorIO(IUnknown *lpUnknown, LPMSG lpMsg)
{
  IInputObject* lpInput = NULL;
  HRESULT hRet = E_INVALIDARG;

  TRACE("(%p,%p)\n", lpUnknown, lpMsg);
  if (lpUnknown)
  {
    hRet = IUnknown_QueryInterface(lpUnknown, &IID_IInputObject,
                                   (void**)&lpInput);
    if (SUCCEEDED(hRet) && lpInput)
    {
      hRet = IInputObject_TranslateAcceleratorIO(lpInput, lpMsg);
      IInputObject_Release(lpInput);
    }
  }
  return hRet;
}

/*************************************************************************
 * @  [SHLWAPI.481]
 *
 * Call IInputObject_HasFocusIO() on an object.
 *
 * PARAMS
 *  lpUnknown [I] Object supporting the IInputObject interface.
 *
 * RETURNS
 *  Success: S_OK, if lpUnknown is an IInputObject object and has the focus,
 *           or S_FALSE otherwise.
 *  Failure: An HRESULT error code, or E_INVALIDARG if lpUnknown is NULL.
 */
HRESULT WINAPI IUnknown_HasFocusIO(IUnknown *lpUnknown)
{
  IInputObject* lpInput = NULL;
  HRESULT hRet = E_INVALIDARG;

  TRACE("(%p)\n", lpUnknown);
  if (lpUnknown)
  {
    hRet = IUnknown_QueryInterface(lpUnknown, &IID_IInputObject,
                                   (void**)&lpInput);
    if (SUCCEEDED(hRet) && lpInput)
    {
      hRet = IInputObject_HasFocusIO(lpInput);
      IInputObject_Release(lpInput);
    }
  }
  return hRet;
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

  TRACE("(%08x,%p,%p,%p)\n", cRGB, pwHue, pwLuminance, pwSaturation);

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
    FIXME("(%p, %#x) stub\n", dest, dwSize);
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
  FIXME("(0x%08x) stub\n", x);
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
 MENUITEMINFOW mi;
 INT nCount = GetMenuItemCount(hMenu), nIter = 0;

 while (nIter < nCount)
 {
   mi.cbSize = sizeof(mi);
   mi.fMask = MIIM_ID;
   if (GetMenuItemInfoW(hMenu, nIter, TRUE, &mi) && mi.wID == wID)
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
DWORD WINAPI SHGetAppCompatFlags(DWORD dwUnknown)
{
  FIXME("(0x%08x) stub\n", dwUnknown);
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
  static WCHAR szSkipBinding[] = { 'S','k','i','p',' ',
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
    FIXME("(%x, %x, %x): stub\n", a, b, c);
    return 0x50;
}

/***********************************************************************
 *		SHQueueUserWorkItem (SHLWAPI.@)
 */
BOOL WINAPI SHQueueUserWorkItem(LPTHREAD_START_ROUTINE pfnCallback, 
        LPVOID pContext, LONG lPriority, DWORD_PTR dwTag,
        DWORD_PTR *pdwId, LPCSTR pszModule, DWORD dwFlags)
{
    TRACE("(%p, %p, %d, %lx, %p, %s, %08x)\n", pfnCallback, pContext,
          lPriority, dwTag, pdwId, debugstr_a(pszModule), dwFlags);

    if(lPriority || dwTag || pdwId || pszModule || dwFlags)
        FIXME("Unsupported arguments\n");

    return QueueUserWorkItem(pfnCallback, pContext, 0);
}

/***********************************************************************
 *		SHSetTimerQueueTimer (SHLWAPI.263)
 */
HANDLE WINAPI SHSetTimerQueueTimer(HANDLE hQueue,
        WAITORTIMERCALLBACK pfnCallback, LPVOID pContext, DWORD dwDueTime,
        DWORD dwPeriod, LPCSTR lpszLibrary, DWORD dwFlags)
{
    HANDLE hNewTimer;

    /* SHSetTimerQueueTimer flags -> CreateTimerQueueTimer flags */
    if (dwFlags & TPS_LONGEXECTIME) {
        dwFlags &= ~TPS_LONGEXECTIME;
        dwFlags |= WT_EXECUTELONGFUNCTION;
    }
    if (dwFlags & TPS_EXECUTEIO) {
        dwFlags &= ~TPS_EXECUTEIO;
        dwFlags |= WT_EXECUTEINIOTHREAD;
    }

    if (!CreateTimerQueueTimer(&hNewTimer, hQueue, pfnCallback, pContext,
                               dwDueTime, dwPeriod, dwFlags))
        return NULL;

    return hNewTimer;
}

/***********************************************************************
 *		IUnknown_OnFocusChangeIS (SHLWAPI.@)
 */
HRESULT WINAPI IUnknown_OnFocusChangeIS(LPUNKNOWN lpUnknown, LPUNKNOWN pFocusObject, BOOL bFocus)
{
    IInputObjectSite *pIOS = NULL;
    HRESULT hRet = E_INVALIDARG;

    TRACE("(%p, %p, %s)\n", lpUnknown, pFocusObject, bFocus ? "TRUE" : "FALSE");

    if (lpUnknown)
    {
        hRet = IUnknown_QueryInterface(lpUnknown, &IID_IInputObjectSite,
                                       (void **)&pIOS);
        if (SUCCEEDED(hRet) && pIOS)
        {
            hRet = IInputObjectSite_OnFocusChangeIS(pIOS, pFocusObject, bFocus);
            IInputObjectSite_Release(pIOS);
        }
    }
    return hRet;
}

/***********************************************************************
 *		SHGetValueW (SHLWAPI.@)
 */
HRESULT WINAPI SKGetValueW(DWORD a, LPWSTR b, LPWSTR c, DWORD d, DWORD e, DWORD f)
{
    FIXME("(%x, %s, %s, %x, %x, %x): stub\n", a, debugstr_w(b), debugstr_w(c), d, e, f);
    return E_FAIL;
}

typedef HRESULT (WINAPI *DllGetVersion_func)(DLLVERSIONINFO *);

/***********************************************************************
 *              GetUIVersion (SHLWAPI.452)
 */
DWORD WINAPI GetUIVersion(void)
{
    static DWORD version;

    if (!version)
    {
        DllGetVersion_func pDllGetVersion;
        HMODULE dll = LoadLibraryA("shell32.dll");
        if (!dll) return 0;

        pDllGetVersion = (DllGetVersion_func)GetProcAddress(dll, "DllGetVersion");
        if (pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            dvi.cbSize = sizeof(DLLVERSIONINFO);
            if (pDllGetVersion(&dvi) == S_OK) version = dvi.dwMajorVersion;
        }
        FreeLibrary( dll );
        if (!version) version = 3;  /* old shell dlls don't have DllGetVersion */
    }
    return version;
}

/***********************************************************************
 *              ShellMessageBoxWrapW [SHLWAPI.388]
 *
 * See shell32.ShellMessageBoxW
 *
 * NOTE:
 * shlwapi.ShellMessageBoxWrapW is a duplicate of shell32.ShellMessageBoxW
 * because we can't forward to it in the .spec file since it's exported by
 * ordinal. If you change the implementation here please update the code in
 * shell32 as well.
 */
INT WINAPIV ShellMessageBoxWrapW(HINSTANCE hInstance, HWND hWnd, LPCWSTR lpText,
                                 LPCWSTR lpCaption, UINT uType, ...)
{
    WCHAR szText[100], szTitle[100];
    LPCWSTR pszText = szText, pszTitle = szTitle;
    LPWSTR pszTemp;
    __ms_va_list args;
    int ret;

    __ms_va_start(args, uType);

    TRACE("(%p,%p,%p,%p,%08x)\n", hInstance, hWnd, lpText, lpCaption, uType);

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

    ret = MessageBoxW(hWnd, pszTemp, pszTitle, uType);
    LocalFree(pszTemp);
    return ret;
}

HRESULT WINAPI IUnknown_QueryServiceExec(IUnknown *unk, REFIID service, REFIID clsid,
                                         DWORD x1, DWORD x2, DWORD x3, void **ppvOut)
{
    FIXME("%p %s %s %08x %08x %08x %p\n", unk,
          debugstr_guid(service), debugstr_guid(clsid), x1, x2, x3, ppvOut);
    return E_NOTIMPL;
}

HRESULT WINAPI IUnknown_ProfferService(IUnknown *unk, void *x0, void *x1, void *x2)
{
    FIXME("%p %p %p %p\n", unk, x0, x1, x2);
    return E_NOTIMPL;
}

/***********************************************************************
 *              ZoneComputePaneSize [SHLWAPI.382]
 */
UINT WINAPI ZoneComputePaneSize(HWND hwnd)
{
    FIXME("\n");
    return 0x95;
}

/***********************************************************************
 *              SHChangeNotifyWrap [SHLWAPI.394]
 */
void WINAPI SHChangeNotifyWrap(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2)
{
    SHChangeNotify(wEventId, uFlags, dwItem1, dwItem2);
}

typedef struct SHELL_USER_SID {   /* according to MSDN this should be in shlobj.h... */
    SID_IDENTIFIER_AUTHORITY sidAuthority;
    DWORD                    dwUserGroupID;
    DWORD                    dwUserID;
} SHELL_USER_SID, *PSHELL_USER_SID;

typedef struct SHELL_USER_PERMISSION { /* ...and this should be in shlwapi.h */
    SHELL_USER_SID susID;
    DWORD          dwAccessType;
    BOOL           fInherit;
    DWORD          dwAccessMask;
    DWORD          dwInheritMask;
    DWORD          dwInheritAccessMask;
} SHELL_USER_PERMISSION, *PSHELL_USER_PERMISSION;

/***********************************************************************
 *             GetShellSecurityDescriptor [SHLWAPI.475]
 *
 * prepares SECURITY_DESCRIPTOR from a set of ACEs
 *
 * PARAMS
 *  apUserPerm [I] array of pointers to SHELL_USER_PERMISSION structures,
 *                 each of which describes permissions to apply
 *  cUserPerm  [I] number of entries in apUserPerm array
 *
 * RETURNS
 *  success: pointer to SECURITY_DESCRIPTOR
 *  failure: NULL
 *
 * NOTES
 *  Call should free returned descriptor with LocalFree
 */
PSECURITY_DESCRIPTOR WINAPI GetShellSecurityDescriptor(PSHELL_USER_PERMISSION *apUserPerm, int cUserPerm)
{
    PSID *sidlist;
    PSID  cur_user = NULL;
    BYTE  tuUser[2000];
    DWORD acl_size;
    int   sid_count, i;
    PSECURITY_DESCRIPTOR psd = NULL;

    TRACE("%p %d\n", apUserPerm, cUserPerm);

    if (apUserPerm == NULL || cUserPerm <= 0)
        return NULL;

    sidlist = HeapAlloc(GetProcessHeap(), 0, cUserPerm * sizeof(PSID));
    if (!sidlist)
        return NULL;

    acl_size = sizeof(ACL);

    for(sid_count = 0; sid_count < cUserPerm; sid_count++)
    {
        static SHELL_USER_SID null_sid = {{SECURITY_NULL_SID_AUTHORITY}, 0, 0};
        PSHELL_USER_PERMISSION perm = apUserPerm[sid_count];
        PSHELL_USER_SID sid = &perm->susID;
        PSID pSid;
        BOOL ret = TRUE;

        if (!memcmp((void*)sid, (void*)&null_sid, sizeof(SHELL_USER_SID)))
        {  /* current user's SID */ 
            if (!cur_user)
            {
                HANDLE Token;
                DWORD bufsize = sizeof(tuUser);

                ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &Token);
                if (ret)
                {
                    ret = GetTokenInformation(Token, TokenUser, (void*)tuUser, bufsize, &bufsize );
                    if (ret)
                        cur_user = ((PTOKEN_USER)tuUser)->User.Sid;
                    CloseHandle(Token);
                }
            }
            pSid = cur_user;
        } else if (sid->dwUserID==0) /* one sub-authority */
            ret = AllocateAndInitializeSid(&sid->sidAuthority, 1, sid->dwUserGroupID, 0,
                    0, 0, 0, 0, 0, 0, &pSid);
        else
            ret = AllocateAndInitializeSid(&sid->sidAuthority, 2, sid->dwUserGroupID, sid->dwUserID,
                    0, 0, 0, 0, 0, 0, &pSid);
        if (!ret)
            goto free_sids;

        sidlist[sid_count] = pSid;
        /* increment acl_size (1 ACE for non-inheritable and 2 ACEs for inheritable records */
        acl_size += (sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD) + GetLengthSid(pSid)) * (perm->fInherit ? 2 : 1);
    }

    psd = LocalAlloc(0, sizeof(SECURITY_DESCRIPTOR) + acl_size);

    if (psd != NULL)
    {
        PACL pAcl = (PACL)(((BYTE*)psd)+sizeof(SECURITY_DESCRIPTOR));

        if (!InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION))
            goto error;

        if (!InitializeAcl(pAcl, acl_size, ACL_REVISION))
            goto error;

        for(i = 0; i < sid_count; i++)
        {
            PSHELL_USER_PERMISSION sup = apUserPerm[i];
            PSID sid = sidlist[i];

            switch(sup->dwAccessType)
            {
                case ACCESS_ALLOWED_ACE_TYPE:
                    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, sup->dwAccessMask, sid))
                        goto error;
                    if (sup->fInherit && !AddAccessAllowedAceEx(pAcl, ACL_REVISION, 
                                (BYTE)sup->dwInheritMask, sup->dwInheritAccessMask, sid))
                        goto error;
                    break;
                case ACCESS_DENIED_ACE_TYPE:
                    if (!AddAccessDeniedAce(pAcl, ACL_REVISION, sup->dwAccessMask, sid))
                        goto error;
                    if (sup->fInherit && !AddAccessDeniedAceEx(pAcl, ACL_REVISION, 
                                (BYTE)sup->dwInheritMask, sup->dwInheritAccessMask, sid))
                        goto error;
                    break;
                default:
                    goto error;
            }
        }

        if (!SetSecurityDescriptorDacl(psd, TRUE, pAcl, FALSE))
            goto error;
    }
    goto free_sids;

error:
    LocalFree(psd);
    psd = NULL;
free_sids:
    for(i = 0; i < sid_count; i++)
    {
        if (!cur_user || sidlist[i] != cur_user)
            FreeSid(sidlist[i]);
    }
    HeapFree(GetProcessHeap(), 0, sidlist);

    return psd;
}
