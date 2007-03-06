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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>

#define INITGUID
#include "wine/test.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "winuser.h"
#include "shlguid.h"
#include "wine/shobjidl.h"

/* Function ptrs for ordinal calls */
static HMODULE hShlwapi = 0;
static BOOL (WINAPI *pSHLWAPI_269)(LPCSTR, CLSID *) = 0;
static DWORD (WINAPI *pSHLWAPI_23)(REFGUID, LPSTR, INT) = 0;

/* GUIDs to test */
const GUID * TEST_guids[] = {
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
  &IID_IDelayedRelease,
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

DEFINE_GUID(IID_Endianess, 0x01020304, 0x0506, 0x0708, 0x09, 0x0A, 0x0B,
            0x0C, 0x0D, 0x0E, 0x0F, 0x0A);

static void test_ClassIDs(void)
{
  const GUID **guids = TEST_guids;
  char szBuff[256];
  GUID guid;
  DWORD dwLen;
  BOOL bRet;
  int i = 0;

  if (!pSHLWAPI_269 || !pSHLWAPI_23)
    return;

  while (*guids)
  {
    dwLen = pSHLWAPI_23(*guids, szBuff, 256);
    ok(dwLen == 39, "wrong size for id %d\n", i);

    bRet = pSHLWAPI_269(szBuff, &guid);
    ok(bRet != FALSE, "created invalid string '%s'\n", szBuff);

    if (bRet)
      ok(IsEqualGUID(*guids, &guid), "GUID created wrong %d\n", i);

    guids++;
    i++;
  }

  /* Test endianess */
  dwLen = pSHLWAPI_23(&IID_Endianess, szBuff, 256);
  ok(dwLen == 39, "wrong size for IID_Endianess\n");

  ok(!strcmp(szBuff, "{01020304-0506-0708-090A-0B0C0D0E0F0A}"),
     "Endianess Broken, got '%s'\n", szBuff);

  /* test lengths */
  szBuff[0] = ':';
  dwLen = pSHLWAPI_23(&IID_Endianess, szBuff, 0);
  ok(dwLen == 0, "accepted bad length\n");
  ok(szBuff[0] == ':', "wrote to buffer with no length\n");

  szBuff[0] = ':';
  dwLen = pSHLWAPI_23(&IID_Endianess, szBuff, 38);
  ok(dwLen == 0, "accepted bad length\n");
  ok(szBuff[0] == ':', "wrote to buffer with no length\n");

  szBuff[0] = ':';
  dwLen = pSHLWAPI_23(&IID_Endianess, szBuff, 39);
  ok(dwLen == 39, "rejected ok length\n");
  ok(szBuff[0] == '{', "Didn't write to buffer with ok length\n");

  /* Test string */
  strcpy(szBuff, "{xxx-");
  bRet = pSHLWAPI_269(szBuff, &guid);
  ok(bRet == FALSE, "accepted invalid string\n");

  dwLen = pSHLWAPI_23(&IID_Endianess, szBuff, 39);
  ok(dwLen == 39, "rejected ok length\n");
  ok(szBuff[0] == '{', "Didn't write to buffer with ok length\n");
}


START_TEST(clsid)
{
  hShlwapi = LoadLibraryA("shlwapi.dll");
  ok(hShlwapi != 0, "LoadLibraryA failed\n");
  if (hShlwapi)
  {
    pSHLWAPI_269 = (void*)GetProcAddress(hShlwapi, (LPSTR)269);
    pSHLWAPI_23 = (void*)GetProcAddress(hShlwapi, (LPSTR)23);
  }

  test_ClassIDs();

  if (hShlwapi)
    FreeLibrary(hShlwapi);
}
