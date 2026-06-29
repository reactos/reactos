/* Unit test suite for SHLWAPI Class ID functions
 *
 * Copyright 2003 Jon Griffiths
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

#include <stdio.h>

#include "wine/test.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "winuser.h"
#include "initguid.h"
#include "shlguid.h"
#include "shobjidl.h"
#include "olectl.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

/* This GUID has been removed from the PSDK */
DEFINE_OLEGUID(WINE_IID_IDelayedRelease,     0x000214EDL, 0, 0);

/* Function ptrs for ordinal calls */
static HMODULE hShlwapi = 0;
static BOOL (WINAPI *pGUIDFromStringA)(LPCSTR, CLSID *) = 0;
static DWORD (WINAPI *pSHStringFromGUIDA)(REFGUID, LPSTR, INT) = 0;

/* GUIDs to test */
static const GUID * TEST_guids[] = {
  &CLSID_ShellDesktop,
  &CLSID_ShellLink,
  &CATID_BrowsableShellExt,
  &CATID_BrowseInPlace,
  &CATID_DeskBand,
  &CATID_InfoBand,
  &CATID_CommBand,
  &FMTID_Intshcut,
  &FMTID_InternetSite,
  &CGID_Explorer,
  &CGID_ShellDocView,
  &CGID_ShellServiceObject,
  &CGID_ExplorerBarDoc,
  &IID_INewShortcutHookA,
  &IID_IShellIcon,
  &IID_IShellFolder,
  &IID_IShellExtInit,
  &IID_IShellPropSheetExt,
  &IID_IPersistFolder,
  &IID_IExtractIconA,
  &IID_IShellDetails,
  &WINE_IID_IDelayedRelease,
  &IID_IShellLinkA,
  &IID_IShellCopyHookA,
  &IID_IFileViewerA,
  &IID_ICommDlgBrowser,
  &IID_IEnumIDList,
  &IID_IFileViewerSite,
  &IID_IContextMenu2,
  &IID_IShellExecuteHookA,
  &IID_IPropSheetPage,
  &IID_INewShortcutHookW,
  &IID_IFileViewerW,
  &IID_IShellLinkW,
  &IID_IExtractIconW,
  &IID_IShellExecuteHookW,
  &IID_IShellCopyHookW,
  &IID_IRemoteComputer,
  &IID_IQueryInfo,
  &IID_IDockingWindow,
  &IID_IDockingWindowSite,
  &CLSID_NetworkPlaces,
  &CLSID_NetworkDomain,
  &CLSID_NetworkServer,
  &CLSID_NetworkShare,
  &CLSID_MyComputer,
  &CLSID_Internet,
  &CLSID_ShellFSFolder,
  &CLSID_RecycleBin,
  &CLSID_ControlPanel,
  &CLSID_Printers,
  &CLSID_MyDocuments,
  NULL
};

DEFINE_GUID(IID_Endianness, 0x01020304, 0x0506, 0x0708, 0x09, 0x0A, 0x0B,
            0x0C, 0x0D, 0x0E, 0x0F, 0x0A);

static void test_ClassIDs(void)
{
  const GUID **guids = TEST_guids;
  char szBuff[256];
  GUID guid;
  DWORD dwLen;
  BOOL bRet;
  int i = 0;
  BOOL is_vista = FALSE;

  if (!pGUIDFromStringA || !pSHStringFromGUIDA)
    return;

  while (*guids)
  {
    dwLen = pSHStringFromGUIDA(*guids, szBuff, 256);
    if (!i && dwLen == S_OK) is_vista = TRUE;  /* seems to return an HRESULT on vista */
    ok(dwLen == (is_vista ? S_OK : 39), "wrong size %lu for id %d\n", dwLen, i);

    bRet = pGUIDFromStringA(szBuff, &guid);
    ok(bRet != FALSE, "created invalid string '%s'\n", szBuff);

    if (bRet)
      ok(IsEqualGUID(*guids, &guid), "GUID created wrong %d\n", i);

    guids++;
    i++;
  }

  /* Test endianness */
  dwLen = pSHStringFromGUIDA(&IID_Endianness, szBuff, 256);
  ok(dwLen == (is_vista ? S_OK : 39), "wrong size %lu for IID_Endianness\n", dwLen);

  ok(!strcmp(szBuff, "{01020304-0506-0708-090A-0B0C0D0E0F0A}"),
     "Endianness Broken, got '%s'\n", szBuff);

  /* test lengths */
  szBuff[0] = ':';
  dwLen = pSHStringFromGUIDA(&IID_Endianness, szBuff, 0);
  ok(dwLen == (is_vista ? E_FAIL : 0), "accepted bad length\n");
  ok(szBuff[0] == ':', "wrote to buffer with no length\n");

  szBuff[0] = ':';
  dwLen = pSHStringFromGUIDA(&IID_Endianness, szBuff, 38);
  ok(dwLen == (is_vista ? E_FAIL : 0), "accepted bad length\n");
  ok(szBuff[0] == ':', "wrote to buffer with no length\n");

  szBuff[0] = ':';
  dwLen = pSHStringFromGUIDA(&IID_Endianness, szBuff, 39);
  ok(dwLen == (is_vista ? S_OK : 39), "rejected ok length\n");
  ok(szBuff[0] == '{', "Didn't write to buffer with ok length\n");

  strcpy(szBuff, "{xxx-");
  bRet = pGUIDFromStringA(szBuff, &guid);
  ok(bRet == FALSE, "accepted invalid string\n");

  dwLen = pSHStringFromGUIDA(&IID_Endianness, szBuff, 39);
  ok(dwLen == (is_vista ? S_OK : 39), "rejected ok length\n");
  ok(szBuff[0] == '{', "Didn't write to buffer with ok length\n");
}

static void test_CLSIDFromProgIDWrap(void)
{
    HRESULT (WINAPI *pCLSIDFromProgIDWrap)(LPCOLESTR,LPCLSID);
    CLSID clsid = IID_NULL;
    HRESULT hres;

    static const WCHAR wszStdPicture[] = {'S','t','d','P','i','c','t','u','r','e',0};

    pCLSIDFromProgIDWrap = (void*)GetProcAddress(hShlwapi,(char*)435);

    hres = pCLSIDFromProgIDWrap(wszStdPicture, &clsid);
    ok(hres == S_OK, "CLSIDFromProgIDWrap failed: %08lx\n", hres);
    ok(IsEqualGUID(&CLSID_StdPicture, &clsid), "wrong clsid\n");

    hres = pCLSIDFromProgIDWrap(NULL, &clsid);
    ok(hres == E_INVALIDARG, "CLSIDFromProgIDWrap failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = pCLSIDFromProgIDWrap(wszStdPicture, NULL);
    ok(hres == E_INVALIDARG, "CLSIDFromProgIDWrap failed: %08lx, expected E_INVALIDARG\n", hres);
}

START_TEST(clsid)
{
  hShlwapi = GetModuleHandleA("shlwapi.dll");

  /* SHCreateStreamOnFileEx was introduced in shlwapi v6.0 */
  if(!GetProcAddress(hShlwapi, "SHCreateStreamOnFileEx")){
      win_skip("Too old shlwapi version\n");
      return;
  }

  pGUIDFromStringA = (void*)GetProcAddress(hShlwapi, (LPSTR)269);
  pSHStringFromGUIDA = (void*)GetProcAddress(hShlwapi, (LPSTR)23);

  test_ClassIDs();
  test_CLSIDFromProgIDWrap();
}
