/****************************************************************************

    PROGRAM: grptoreg

    PURPOSE: Move the Program Manager's group files (*.GRP) to the
             registry. Also updates the Program Manager's settings that are
             in progman.ini.

             This program needs to be run in the directory where the .grp
             and progman.ini files reside. It uses progman.ini to update the
             registry, only the group files named in progman.ini will
             be entered into the registry.

             Once this program is run, all that needs to be done is:
             Logoff, then logon again.

    FUNCTIONS:

    COMMENTS:
             Most of the functions were extracted from the Program Manager.
             If the group fromat changes in the Progam Manager, the same
             changes must be done here (and in grptoreg.h). Same thing
             applies where in the registry this information is to be
             stored/read.


    MODIFICATION HISTORY
        Created:  4/10/92       JohanneC

****************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "grptoreg.h"

BOOL SaveGroupInRegistry(PGROUP pGroup, BOOL bCommonGrp, BOOL bOverwriteGroup);

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  MyDwordAlign() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT MyDwordAlign(INT wStrLen)
{
    return ((wStrLen + 3) & ~3);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SizeofGroup_U() - for unicode groups                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/
DWORD PASCAL SizeofGroup_U(LPGROUPDEF_U lpgd)
{
    LPPMTAG lptag;
    DWORD cbSeg;
    DWORD cb;

    cbSeg = GlobalSize(lpgd);

    lptag = (LPPMTAG)((LPSTR)lpgd+lpgd->cbGroup);

    if ((DWORD)((PCHAR)lptag - (PCHAR)lpgd +MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb))+4) <= cbSeg
        && lptag->wID == ID_MAGIC
        && lptag->wItem == (int)0xFFFF
        && lptag->cb == (WORD)(MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)) + 4)
        && *(PLONG)lptag->rgb == TAG_MAGIC)
      {
        while ((cb = (DWORD)((PCHAR)lptag - (PCHAR)lpgd + MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)))) <= cbSeg)
          {
            if (lptag->wID == ID_LASTTAG)
                return cb;
            (LPSTR)lptag += lptag->cb;
          }
      }
    return lpgd->cbGroup;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SizeofGroup() - for ansi groups                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/
DWORD SizeofGroup(LPGROUPDEF lpgd)
{
    LPPMTAG lptag;
    WORD cbSeg;
    WORD cb;

    cbSeg = (WORD)GlobalSize(lpgd);

    lptag = (LPPMTAG)((LPSTR)lpgd+lpgd->cbGroup);

    if ((WORD)((PCHAR)lptag - (PCHAR)lpgd +MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb))+4) <= cbSeg
            && lptag->wID == ID_MAGIC
            && lptag->wItem == (int)0xFFFF
            && lptag->cb == (WORD)(MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)) + 4)
            && *(PLONG)lptag->rgb == TAG_MAGIC) {

        while ((cb = (WORD)((PCHAR)lptag - (PCHAR)lpgd + MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)))) <= cbSeg) {
            if (lptag->wID == ID_LASTTAG)
                return (DWORD)cb;
            (LPSTR)lptag += lptag->cb;
        }
    }
    return lpgd->cbGroup;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  IsGroup() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/
BOOL PASCAL IsGroup(PSTR p)
{
    if (_strnicmp(p, "GROUP", CCHGROUP) != 0) {;
        return FALSE;
    }

    /*
     * Can't have 0 for first digit
     */
    if (p[5] == '0') {
        return FALSE;
    }

    /*
     * Everything else must be a number
     */
    for (p += CCHGROUP; *p; p++) {
        if (*p < '0' || *p > '9') {
            return FALSE;
        }
    }

    return TRUE;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RemoveString() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/
VOID APIENTRY RemoveString(PSTR pString)
{

    PSTR pT = pString + lstrlen(pString) + 1;

    while (*pT)
      {
        while (*pString++ = *pT++)
            ;
      }
    *pString = 0;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  StringToEnd() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/
VOID APIENTRY StringToEnd(PSTR pString)
{
    char *pT,*pTT;

    for (pT = pString; *pT; )
        while (*pT++)
            ;
    for (pTT = pString; *pT++ = *pTT++;)
        ;
    *pT = 0;

    RemoveString(pString);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetGroupList() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/
VOID PASCAL GetGroupList(PSTR szList)
{
    char szT[20];
    PSTR pT, pTT, pS;
    INT cGroups;         // The number of Groups= lines.

    GetPrivateProfileString(szGroups, NULL, szNULL, szList, (CGROUPSMAX+1)*8, szINIFile);

    cGroups = 0;

    /*
     * Filter out anything that isn't group#.
     */
    for (pT = szList; *pT; ) {
        AnsiUpper(pT);

        if (IsGroup(pT)) {
            pT += lstrlen(pT) + 1;
            cGroups++;
        } else {
            RemoveString(pT);
        }
    }

    /*
     * Sort the groups
     */
    lstrcpy(szT, "Group");
    for (pT = szGroupsOrder; *pT; ) {
        while (*pT == ' ') {
            pT++;
        }

        if (*pT < '0' || *pT > '9') {
            break;
        }

        pTT = szT + CCHGROUP;
        while (*pT >= '0' && *pT <= '9') {
            *pTT++ = *pT++;
        }
        *pTT=0;

        for (pS = szList; *pS; pS += lstrlen(pS) + 1) {
            if (!lstrcmpi(pS,szT)) {
                StringToEnd(pS);
                cGroups--;
                break;
            }
        }
    }

    /*
     * Move any remaining groups to the end of the list so that they load
     * last and appear on top of everything else - keeps DOS based install
     * programs happy.
     */
    while (cGroups>0) {
        StringToEnd(szList);
        cGroups--;
    }

}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CreateGroupHandle() -                                                   */
/*                                                                          */
/* Creates a discarded handle for use as a group handle... on the first     */
/* LockGroup() the file will be loaded.                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

HANDLE PASCAL CreateGroupHandle(void)
{
  HANDLE hGroup;

  if (hGroup = GlobalAlloc(GMEM_MOVEABLE | GMEM_DISCARDABLE, 1L))
      GlobalDiscard(hGroup);

  return(hGroup);
}

/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  LockGroup() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/
LPGROUPDEF PASCAL LockGroup(PGROUP pGroup)

{
  LPGROUPDEF lpgd;
  int        cbGroup;
  int        fh;
  PSTR       psz;

  psz = pGroup->lpKey;

  /* Find and open the group file. */
  fh = _open(pGroup->lpKey, O_RDONLY | O_BINARY);
  if (fh == -1) {
      goto LGError1;
  }

  /* Find the size of the file by seeking to the end. */
  cbGroup = (WORD)_lseek(fh, 0L, SEEK_END);
  if (cbGroup < sizeof(GROUPDEF)) {
      goto LGError2;
  }

  _lseek(fh, 0L, SEEK_SET);

  /* Allocate some memory for the thing. */
  if (!(pGroup->hGroup = GlobalReAlloc(pGroup->hGroup, (DWORD)cbGroup, GMEM_MOVEABLE))) {
      psz = NULL;
      goto LGError2;
  }

  lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);

  /* Read the whole group file into memory. */
  if (_read(fh, (PSTR)lpgd, cbGroup) != cbGroup)
      goto LGError3;

  if (lpgd->dwMagic == GROUP_MAGIC) {
      /* Validate the group file by checking the magic bytes and the checksum. */
      if (lpgd->cbGroup > (WORD)cbGroup)
          goto LGError3;
  }
  else if (lpgd->dwMagic == GROUP_UNICODE) {
      LPGROUPDEF_U lpgd_U;

      lpgd_U = (LPGROUPDEF_U)lpgd;
      if (lpgd_U->cbGroup > (DWORD)cbGroup) {
          goto LGError3;
      }
  }
  else {
      goto LGError3;
  }

  /* Now return the pointer. */
  _close(fh);
  return(lpgd);

LGError3:
  GlobalUnlock(pGroup->hGroup);
  GlobalDiscard(pGroup->hGroup);

LGError2:
  _close(fh);

LGError1:
  return(NULL);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  UnlockGroup() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/
void PASCAL UnlockGroup(PGROUP pGroup)
{
    GlobalUnlock(pGroup->hGroup);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FindFreeIndex() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/
WORD FindFreeIndex(BOOL bCommonGrp)
{
    WORD wIndex;
    LPSTR pszT;
    WORD i;
    static BOOL bFirstTime = TRUE;
    DWORD cbData;

    if (!*szGroupsOrder) {
        cbData = sizeof(szGroupsOrder);
        if (RegQueryValueEx(hkeyPMSettings, szOrder, 0, 0, szGroupsOrder, &cbData))
            if (!GetPrivateProfileString(szSettings, szOrder, szNULL, szGroupsOrder, sizeof(szGroupsOrder), szINIFile)) {
                pGroupIndexes[1] = 1;
                return(1);
            }
    }

    if (bFirstTime) {
        bFirstTime = FALSE;

        /* Initialize the array to zero since we are 1 based */
        for (i = 1; i <= CGROUPSMAX; i++) {
             pGroupIndexes[i] = 0;
        }

        pszT = szGroupsOrder;
        while(*pszT) {

            /* Skip blanks */
            while (*pszT == ' ')
                pszT++;

            /* Check for NULL */
            if (!(*pszT))
                break;

            if ( (bCommonGrp && *pszT == 'C') ||
                 (!bCommonGrp && *pszT != 'C') ) {

                if (bCommonGrp)
                    pszT++;

                wIndex = 0;

                for (; *pszT && *pszT!=' '; pszT++) {
                    wIndex *= 10;
                    wIndex += *pszT - '0';
                }
                pGroupIndexes[wIndex] = wIndex;

            } else {
                /* Skip to next entry */
                while (*pszT && *pszT != ' ')
                    pszT++;
            }
        }
    }
    for (i = 1; i <= CGROUPSMAX; i++) {
        if (pGroupIndexes[i] != i) {
            pGroupIndexes[i] = i;
            return(i);
        }
    }

    if (i > CGROUPSMAX) { // too many groups
        return((WORD)-1);
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RemoveBackslashFromKeyName() -                                          */
/*                                                                          */
/*  replace the invalid characters for a key name by some valid charater.   */
/*  the same characters that are invalid for a file name are invalid for a  */
/*  key name.                                                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void RemoveBackslashFromKeyName(LPSTR lpKeyName)
{
    LPSTR lpt;

    for (lpt = lpKeyName; *lpt; lpt++) {
        if ((*lpt == '\\') || (*lpt == ':') || (*lpt == '>') || (*lpt == '<') ||
             (*lpt == '*') || (*lpt == '?') ){
            *lpt = '.';
        }
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  LoadGroup() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PGROUP PASCAL LoadGroup(PSTR pGroupFile, WORD wIndex, BOOL bCommonGrp)
{
    PGROUP pGroup;
    LPGROUPDEF lpgd;
    char szFmt[] = " %d";
    char szCFmt[] = " C%d";
    char szIndex[5];
    BOOL bNewOrder = FALSE;

    if (!wIndex) {
        wIndex = FindFreeIndex(bCommonGrp);
        bNewOrder = TRUE;
    }

    pGroup = (PGROUP)LocalAlloc(LPTR,sizeof(GROUP));
    if (!pGroup) {
        return NULL;
    }

    pGroup->hGroup = CreateGroupHandle();
    pGroup->pItems = NULL;
    pGroup->hbm    = NULL;
    pGroup->wIndex = wIndex;

    pGroup->lpKey = (PSTR)LocalAlloc(LPTR, lstrlen(pGroupFile) + 1);
    pGroup->fLoaded = FALSE;

    if (!pGroup->lpKey) {
        GlobalFree(pGroup->hGroup);
        LocalFree((HANDLE)pGroup);
        return NULL;
    }

    lstrcpy(pGroup->lpKey, pGroupFile);

    /*
     * Note that we're about to load a group for the first time.
     * NB Stting this tells LockGroup that the caller can handle the errors.
     */
    lpgd = LockGroup(pGroup);

    /*
     * The group has been loaded or at least we tried.
     */
    if (!lpgd) {
LoadFail:
        LocalFree((HANDLE)pGroup->lpKey);
        GlobalFree(pGroup->hGroup);
        LocalFree((HANDLE)pGroup);
        return NULL;
    }

    /*
     * test if it is a Windows 3.1 group file format. If so it is not
     * valid in WIN32. In Windows 3.1 RECT and POINT are WORD instead of LONG.
     */

    if ( lpgd->dwMagic == GROUP_MAGIC &&
         ((lpgd->rcNormal.left != (INT)(SHORT)lpgd->rcNormal.left) ||
         (lpgd->rcNormal.right != (INT)(SHORT)lpgd->rcNormal.right) ||
         (lpgd->rcNormal.top != (INT)(SHORT)lpgd->rcNormal.top) ||
         (lpgd->rcNormal.bottom != (INT)(SHORT)lpgd->rcNormal.bottom) )){
        /* The group file is invalid. */
        UnlockGroup(pGroup);
        goto LoadFail;
    }

    /*
     * In the registry, the group's key is it's title.
     */
    if (lpgd->dwMagic == GROUP_MAGIC) {
        LocalFree(pGroup->lpKey);
        if (pGroup->lpKey = (PSTR)LocalAlloc(LPTR, lstrlen(PTR(lpgd, lpgd->pName)) + 1))
            lstrcpy(pGroup->lpKey, PTR(lpgd, lpgd->pName));
        RemoveBackslashFromKeyName(pGroup->lpKey);
    }
    else if (lpgd->dwMagic == GROUP_UNICODE){
        LPGROUPDEF_U lpgd_U;
        ANSI_STRING AnsiString;
        UNICODE_STRING UniString;

        lpgd_U = (LPGROUPDEF_U)lpgd;

        RtlInitUnicodeString(&UniString, (LPWSTR)(PTR(lpgd_U, lpgd_U->pName)));
        RtlUnicodeStringToAnsiString(&AnsiString, &UniString, TRUE);

        LocalFree(pGroup->lpKey);
        if (pGroup->lpKey = (PSTR)LocalAlloc(LPTR, lstrlen(AnsiString.Buffer) + 1))
            lstrcpy(pGroup->lpKey, AnsiString.Buffer);
        RemoveBackslashFromKeyName(pGroup->lpKey);
        RtlFreeAnsiString(&AnsiString);
    }

    UnlockGroup(pGroup);

    if (bNewOrder) {
        if (bCommonGrp)
            wsprintf(szIndex, szCFmt, wIndex);
        else
            wsprintf(szIndex, szFmt, wIndex);
        lstrcat(szGroupsOrder, szIndex);
        RegSetValueEx(hkeyPMSettings, szOrder, 0, REG_SZ, szGroupsOrder,
                                lstrlen(szGroupsOrder)+1);
    }
    return(pGroup);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  UnloadGroup() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void PASCAL UnloadGroup(PGROUP pGroup)
{
    PITEM pItem, pItemNext;

    // Free the group segment.
    GlobalFree(pGroup->hGroup);

    // Free the local stuff.
    LocalFree((HANDLE)pGroup->lpKey);

    // The item data.
    for (pItem = pGroup->pItems; pItem; pItem = pItemNext) {
        pItemNext = pItem->pNext;
        LocalFree((HANDLE) pItem);
    }

    // Lastly, free the group structure itself.
    LocalFree((HANDLE)pGroup);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  LoadAllGroups() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/
VOID PASCAL LoadAllGroups()
{
    PSTR pT, pszT;
    char szGroupList[(CGROUPSMAX+1)*8];
    WORD wIndex;
    char szPath[120];
    PGROUP pGroup;

    pT = szGroupList;
    for (GetGroupList(pT); *pT; pT += (lstrlen(pT) + 1)) {
        if (!GetPrivateProfileString(szGroups, pT, szNULL, szPath,
                sizeof(szPath), szINIFile)) {
            continue;
        }

        wIndex = 0;
        for (pszT = pT + CCHGROUP; *pszT; pszT++) {
            wIndex *= 10;
            wIndex += *pszT - '0';
        }

        pGroup = LoadGroup(szPath, wIndex, FALSE);
        if (pGroup) {
            SaveGroupInRegistry(pGroup, FALSE, FALSE);
            UnloadGroup(pGroup);
        }
        else {
            bNoError = FALSE;
            wsprintf(szMessage, "An error has occurred reading group file %s, no registry update.\n",
                     szPath);
            printf(szMessage);
        }
    }
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SaveGroupInRegistry() -                                                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL SaveGroupInRegistry(PGROUP pGroup, BOOL bCommonGrp, BOOL bOverwriteGroup)
{
    LPGROUPDEF lpgd;
    int cb;
    int err;
    char szT[66];
    static char szFmt[] = "Group%d";
    HKEY hkey;
    HKEY hkeyGroups;
    PSECURITY_ATTRIBUTES pSecAttr;
    DWORD dwDisposition;
    HANDLE hEvent;


    lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);
    if (!lpgd)
        return FALSE;

    if (bCommonGrp) {
        hkeyGroups = hkeyCommonGroups;
        pSecAttr = pCGrpSecAttr;
    }
    else {
        hkeyGroups = hkeyProgramGroups;
        pSecAttr = pPGrpSecAttr;
    }

    while(1) {
        if (err = RegCreateKeyEx(hkeyGroups, pGroup->lpKey, 0, 0, 0,
                                  DELETE | KEY_READ | KEY_WRITE, pSecAttr,
                                  &hkey, &dwDisposition)) {
            goto Exit1;
        }
        if (dwDisposition == REG_CREATED_NEW_KEY) {
            break;
        }
        if (dwDisposition == REG_OPENED_EXISTING_KEY) {
            if (!bOverwriteGroup) {
                RegCloseKey(hkey);
                wsprintf(szMessage, "Could not save the group %s in the registry. The group already exists.", pGroup->lpKey);
                printf(szMessage);
                return(FALSE);
            }
            else {
                RegCloseKey(hkey);
                if (RegDeleteKey(hkeyGroups, pGroup->lpKey) != ERROR_SUCCESS) {
                    wsprintf(szMessage, "Could not save the group %s in the registry. The group already exists.", pGroup->lpKey);
                    printf(szMessage);
                    return(FALSE);
                }
            }
        }
    }


    /* update groups section. */
    wsprintf(szT, szFmt, pGroup->wIndex);
    RegSetValueEx(hkeyPMGroups, szT, 0, REG_SZ, (LPBYTE)pGroup->lpKey, lstrlen(pGroup->lpKey)+1);

    if (lpgd->dwMagic == GROUP_MAGIC) {
        cb = SizeofGroup(lpgd);
    }
    else {
        cb = SizeofGroup_U((LPGROUPDEF_U)lpgd);
    }

    err = RegSetValueEx(hkey, NULL, 0, REG_BINARY, (LPTSTR)lpgd, cb);

    hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, "Progman.GroupValueSet");
    if (hEvent) {
        SetEvent(hEvent);
    }

    RegFlushKey(hkey);
    RegCloseKey(hkey);

Exit1:
    GlobalUnlock(pGroup->hGroup);

    if (err) {
        bNoError = FALSE;
        wsprintf(szMessage, "Could not save the group %s in the registry.", pGroup->lpKey);
        printf(szMessage);
    }
    GlobalDiscard(pGroup->hGroup);
    return(!err);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ReadSettings() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/
BOOL ReadSettings()
{

  OFSTRUCT of;
  CHAR szT[MAX_PATH];

  lstrcpy(szT, ".\\");
  lstrcat(szT, szINIFile);
  if (OpenFile(szT, &of, OF_EXIST) == -1) {
      // set the current directory to the windows directory
      GetWindowsDirectory(szT, MAX_PATH);
      SetCurrentDirectory(szT);
  }
  if (OpenFile(szINIFile, &of, OF_EXIST) == -1) {
      return(FALSE);
  }
  bMinOnRun = GetPrivateProfileInt(szSettings, szMinOnRun, bMinOnRun, szINIFile);
  bAutoArrange = GetPrivateProfileInt(szSettings, szAutoArrange, bAutoArrange, szINIFile);
  bSaveSettings = GetPrivateProfileInt(szSettings, szSaveSettings, bSaveSettings, szINIFile);
  GetPrivateProfileString(szSettings, szWindow, szNULL, szPMWindowSetting, sizeof(szPMWindowSetting), szINIFile);
  GetPrivateProfileString(szSettings, szStartup, szNULL, szStartupGroup, sizeof(szStartupGroup), szINIFile);

  fNoRun = GetPrivateProfileInt(szRestrict, szNoRun, FALSE, szINIFile);
  fNoClose = GetPrivateProfileInt(szRestrict, szNoClose, FALSE, szINIFile);
  fNoSave = GetPrivateProfileInt(szRestrict, szNoSave, FALSE, szINIFile);
  fNoFileMenu = GetPrivateProfileInt(szRestrict, szNoFileMenu, FALSE, szINIFile);
  dwEditLevel = GetPrivateProfileInt(szRestrict, szEditLevel, (WORD)0, szINIFile);

  GetPrivateProfileString(szSettings, szOrder, szNULL, szGroupsOrder, sizeof(szGroupsOrder), szINIFile);
  return(TRUE);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SaveSettingsToRegistry() -                                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/
BOOL SaveSettingsToRegistry()
{
    int err;

    if (hkeyPMSettings) {
        err = RegSetValueEx(hkeyPMSettings, szWindow, 0, REG_SZ, szPMWindowSetting,
                                lstrlen(szPMWindowSetting)+1);
        err |= RegSetValueEx(hkeyPMSettings, szOrder, 0, REG_SZ, szGroupsOrder,
                                lstrlen(szGroupsOrder)+1);
        if (*szStartupGroup)
            err |= RegSetValueEx(hkeyPMSettings, szStartup, 0, REG_SZ, szStartupGroup,
                                lstrlen(szStartupGroup)+1);
        err |= RegSetValueEx(hkeyPMSettings, szMinOnRun, 0, REG_DWORD,
                               (LPBYTE)&bMinOnRun, sizeof(bMinOnRun));
        err |= RegSetValueEx(hkeyPMSettings, szAutoArrange, 0, REG_DWORD,
                               (LPBYTE)&bAutoArrange, sizeof(bAutoArrange));
        err |= RegSetValueEx(hkeyPMSettings, szSaveSettings, 0, REG_DWORD,
                               (LPBYTE)&bSaveSettings, sizeof(bSaveSettings));
    }
    if (hkeyPMRestrict) {
        err = RegSetValueEx(hkeyPMRestrict, szNoRun, 0, REG_DWORD,
                               (LPBYTE)&fNoRun, sizeof(fNoRun));
        err |= RegSetValueEx(hkeyPMRestrict, szNoClose, 0, REG_DWORD,
                               (LPBYTE)&fNoClose, sizeof(fNoClose));
        err |= RegSetValueEx(hkeyPMRestrict, szNoSave, 0, REG_DWORD,
                               (LPBYTE)&fNoSave, sizeof(fNoSave));
        err |= RegSetValueEx(hkeyPMRestrict, szNoFileMenu, 0, REG_DWORD,
                               (LPBYTE)&fNoFileMenu, sizeof(fNoFileMenu));
        err |= RegSetValueEx(hkeyPMRestrict, szEditLevel, 0, REG_DWORD,
                               (LPBYTE)&dwEditLevel, sizeof(dwEditLevel));
    }
    return(!err);
}

BOOL AccessToPersonalGroups()
{
    //
    // Initialize security attributes for Personal Groups.
    //
    pPGrpSecAttr = &PGrpSecAttr;
    if (!InitializeSecurityAttributes(pPGrpSecAttr, FALSE))
        pPGrpSecAttr = NULL;

    /*
     * Create/Open the registry keys corresponding to progman.ini sections.
     */
    if (RegCreateKeyEx(HKEY_CURRENT_USER, szProgramManager, 0, szProgramManager, 0,
                         KEY_READ | KEY_WRITE,
                         pPGrpSecAttr, &hkeyProgramManager, NULL)) {
        return(FALSE);
    }

    RegCreateKeyEx(hkeyProgramManager, szSettings, 0, szProgramManager, 0,
                         KEY_READ | KEY_WRITE,
                         pPGrpSecAttr, &hkeyPMSettings, NULL);

    RegCreateKeyEx(hkeyProgramManager, szRestrict, 0, szProgramManager, 0,
                         KEY_READ,
                         pPGrpSecAttr, &hkeyPMRestrict, NULL);

    RegCreateKeyEx(hkeyProgramManager, szGroups, 0, szProgramManager, 0,
                         KEY_READ | KEY_WRITE,
                         pPGrpSecAttr, &hkeyPMGroups, NULL);

    if (RegCreateKeyEx(HKEY_CURRENT_USER, szProgramGroups, 0, szProgramGroups, 0,
                         KEY_READ | KEY_WRITE,
                         pPGrpSecAttr, &hkeyProgramGroups, NULL) ){
        return(FALSE);
    }
    return(TRUE);
}

BOOL AccessToCommonGroups()
{
    //
    // Initialize security attributes for Common Groups.
    //
    pCGrpSecAttr = &CGrpSecAttr;
    if (!InitializeSecurityAttributes(pCGrpSecAttr, TRUE)) {
        pCGrpSecAttr = NULL;
        return(FALSE);
    }

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szCommonGroups, 0, szProgramGroups, 0,
                         KEY_READ | KEY_WRITE | DELETE,
                         pCGrpSecAttr, &hkeyCommonGroups, NULL) ){
        return(FALSE);
    }

    return(TRUE);
}

void Usage()
{
                printf("\nConverts a Windows NT compatible .GRP file into the Registry for use by\n\
Windows NT.  Note that the .GRP file must have been created using REGTOGRP.\n\
GRPTOREG will not accept MS-DOS Windows .GRP files.\n\n\
GRPTOREG [/o] [/c] groupfiles \n\n\
groupfiles   Path to a .GRP file created by REGTOGRP. If more than one file,\n\
             separate by spaces.\n\
/o           If a Program Manager group already exists with the same name as\n\
             groupfile, overwrite it.\n\
/c           Will create a Common group from the groupfile (must have \n\
             administrative privileges), otherwise a personal group is created.\n");

}

int __cdecl
main( argc, argv )
int argc;
char *argv[];
{
    INT i;
    CHAR *cp;
    LPSTR pGroupFile;
    PGROUP pGroup;
    BOOL bCommonGrp = FALSE;
    BOOL bOverwriteGroup = FALSE;

    if (argc > 1) {  // just update specified groups.

        for(i = 1; i < argc && (argv[i][0] == '/' || argv[i][0] == '-'); ++i) {
            for(cp = &argv[i][1]; *cp != '\0'; ++cp) {
            switch(*cp) {
            case 'c':
                //
                // The user wants common groups.
                // First check if the user has permission to create common groups.
                //
                if (AccessToCommonGroups()) {
                    bCommonGrp = TRUE;
                }
                else {
                    wsprintf(szMessage, "You do not have write access to Common Groups. No Groups will be created.");
                    printf(szMessage);
                    goto Exit;
                }
                break;
            case 'o':
                bOverwriteGroup = TRUE;
                break;
            case '?':
            default:
                Usage();
                goto Exit;

            }
            }
       }

       if (!bCommonGrp) {
            //
            // The user wants personal groups.
            // First check if the user has access to the registry to
            // create personal groups in Progman.
            //
            if (!AccessToPersonalGroups()) {
                wsprintf(szMessage, "You do not have write access to the registry. No updates will take place.");
                printf(szMessage);
                goto Exit;
            }

        }

        for (; i < argc; i++) {
            pGroupFile = argv[i];
            pGroup = LoadGroup(pGroupFile, 0, bCommonGrp);
            if (pGroup) {
                SaveGroupInRegistry(pGroup, bCommonGrp, bOverwriteGroup);
                UnloadGroup(pGroup);
            }
            else {
                bNoError = FALSE;
                wsprintf(szMessage, "An error has occurred reading group file %s, no registry update.",
                         pGroupFile);
                printf(szMessage);
            }
        }
    }
    else
        { // update all groups thru progman.ini
        if (hkeyProgramManager) {
            if (!ReadSettings()) {
                bNoError = FALSE;
                wsprintf(szMessage, "The file PROGMAN.INI is not in this directory.");
                printf(szMessage);
            }
            if (bNoError && !SaveSettingsToRegistry()) {
//                wsprintf(szMessage, "An error occured while saving settings to the registry.");
            }
        }
        if (hkeyProgramGroups) {
            bNoError = TRUE;
            LoadAllGroups();
            if (bNoError) {
                wsprintf(szMessage, "Groups were updated successfully to the registry.");
                printf(szMessage);
            }
        }
    }

Exit:

    if (hkeyProgramManager) {
        RegFlushKey(hkeyProgramManager);
        if (hkeyPMSettings)
            RegCloseKey(hkeyPMSettings);
        if (hkeyPMRestrict)
            RegCloseKey(hkeyPMRestrict);
        if (hkeyPMGroups)
            RegCloseKey(hkeyPMGroups);
        if (hkeyProgramManager)
            RegCloseKey(hkeyProgramManager);
    }

    if (hkeyProgramGroups) {
        RegFlushKey(hkeyProgramGroups);
        RegCloseKey(hkeyProgramGroups);
    }

    if (hkeyCommonGroups) {
        RegFlushKey(hkeyCommonGroups);
        RegCloseKey(hkeyCommonGroups);
    }


    //
    // Free up the security descriptor
    //

    if (pPGrpSecAttr) {
        DeleteSecurityDescriptor(pPGrpSecAttr->lpSecurityDescriptor);
    }

    if (pCGrpSecAttr) {
        DeleteSecurityDescriptor(pCGrpSecAttr->lpSecurityDescriptor);
    }
    return(TRUE);
}
