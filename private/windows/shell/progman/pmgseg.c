/****************************************************************************/
/*                                                                          */
/*  PMGSEG.C -                                                              */
/*                                                                          */
/*      Program Manager Group Handling Routines                             */
/*                                                                          */
/****************************************************************************/

#include "progman.h"
#include "dde.h"
#include "convgrp.h"

#define WORD_MIN -32767
#define WORD_MAX  32767

#ifndef ORGCODE
#include "fcntl.h"
#include "io.h"
#include "stdio.h"
#include <tchar.h>
#define S_IREAD     0000400         /* read permission, owner */
#define S_IWRITE    0000200         /* write permission, owner */
#endif

BOOL fFirstLoad = FALSE;
extern BOOL bHandleProgramGroupsEvent;

#if 0
// DOS apps are no longer set to fullscreen by default in progman
//  5-3-93 johannec (bug 8343)
#ifdef i386
BOOL IsDOSApplication(LPTSTR lpPath);
BOOL SetDOSApplicationToFullScreen(LPTSTR lpTitle);
#endif
#endif

void NEAR PASCAL RemoveItemFromList(PGROUP pGroup, PITEM pItem)
    // Removes a PITEM from the list.
{
    PITEM *ppItem;

    /* Cause it to be repainted later. */
    InvalidateIcon(pGroup, pItem);

    if (pItem == pGroup->pItems) {
        /*
         * first one in list, must invalidate next one so it paints an active
         * title bar.
         */
        InvalidateIcon(pGroup,pItem->pNext);
    }

    /* Remove it from the list. */
    for (ppItem = &pGroup->pItems;*ppItem != pItem;
        ppItem = &((*ppItem)->pNext));

    *ppItem = pItem->pNext;

    /* Lastly free up the memory. */
    LocalFree((HANDLE)pItem);
}

#ifdef DEBUG
void NEAR PASCAL CheckBeforeReAlloc(HANDLE h)
{
        TCHAR buf[100];

        if ((BYTE)GlobalFlags(h)) {
                wsprintf(buf, TEXT("LockCount before realloc %d\r\n"), (BYTE)GlobalFlags(h));
                OutputDebugString(buf);
                DbgBreakPoint();
        }
}
#else
#define CheckBeforeReAlloc(h)
#endif

#ifdef PARANOID
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CheckRange() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void PASCAL CheckRange(
    LPGROUPDEF lpgd,
    LPTSTR lp1,
    WORD *lpw1,
    WORD cb1,
    LPTSTR lp2,
    WORD w2,
    WORD cb2,
    LPTSTR lpThing)
{
    WORD w1 = *lpw1;
    WORD e1, e2;

    if (!w1 || (w1 == w2)) {
        return;
    }

    if (!cb1) {
        cb1 = (WORD)lstrlen((LPTSTR) PTR(lpgd, *lpw1));
    }

    e1 = w1 + cb1;
    e2 = w2 + cb2;

    if ((w1 < e2) && (w2 < e1)) {
        KdPrint(("ERROR: %s overlaps %s in %s!!!!\r\n",lp2,lp1,lpThing));
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  CheckPointer() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

void PASCAL CheckPointer(
    LPGROUPDEF lpgd,
    LPTSTR lp,
    WORD *lpw,
    WORD cb,
    WORD limit)
{
    LPITEMDEF lpid;
    int i;

    if (lpw == NULL || !*lpw) {
        KdPrint(("Warning: %s is NULL\r\n", lp));
        DebugBreak();
    }

    if (!cb) {
        cb = lstrlen((LPTSTR) PTR(lpgd, *lpw));
    }

    if (*lpw + cb > limit) {
        KdPrint(("ERROR: %s runs off end of group\r\n", lp));
        return;
    }

}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  VerifyGroup() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

void PASCAL VerifyGroup(
    LPGROUPDEF lpgd)
{
    int       i;
    LPITEMDEF lpid;
    DWORD     limit = lpgd->cbGroup;

    KdPrint(("\r\nChecking Group %s\r\n",(LPTSTR) PTR(lpgd, lpgd->pName)));
    CheckPointer(lpgd, TEXT("Group Name"), &lpgd->pName, 0, limit);

    for (i = 0; i < (int)lpgd->cItems; i++) {
        if (!lpgd->rgiItems[i]) {
            continue;
        }

        lpid = ITEM(lpgd, i);
        KdPrint(("Checking item %d at %4.4X (%s):\r\n", i, lpgd->rgiItems[i],
                (LPTSTR) PTR(lpgd, lpid->pName)));
        CheckPointer(lpgd, TEXT("Itemdef"), lpgd->rgiItems + i, sizeof(ITEMDEF), limit);
        CheckPointer(lpgd, TEXT("Item name"), &lpid->pName, 0, limit);
        CheckPointer(lpgd, TEXT("item command"), &lpid->pCommand, 0, limit);
        CheckPointer(lpgd, TEXT("item icon path"), &lpid->pIconPath, 0, limit);
    }
}
#endif


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  IsGroupReadOnly() -                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL FAR PASCAL IsGroupReadOnly(LPTSTR szGroupKey, BOOL bCommonGroup)
{
    HKEY hkey;
    HKEY hkeyGroups;

    if (bCommonGroup)
       hkeyGroups = hkeyCommonGroups;
    else if (bUseANSIGroups)
        hkeyGroups = hkeyAnsiProgramGroups;
    else
        hkeyGroups = hkeyProgramGroups;

    if (!hkeyGroups)
        return(FALSE);

    if (!RegOpenKeyEx(hkeyGroups, szGroupKey, 0, DELETE | KEY_READ | KEY_WRITE, &hkey)){
        RegCloseKey(hkey);
        return(FALSE);
    }
    return(TRUE);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GroupCheck() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL FAR PASCAL GroupCheck(PGROUP pGroup)
{
    if (!fExiting && IsGroupReadOnly(pGroup->lpKey, pGroup->fCommon)) {
        pGroup->fRO = TRUE;
        return FALSE;
    }
    pGroup->fRO = FALSE;
    return TRUE;
}


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
/*  SizeofGroup() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/
DWORD PASCAL SizeofGroup(LPGROUPDEF lpgd)
{
    LPPMTAG lptag;
    DWORD cbSeg;
    DWORD cb;

    cbSeg = (DWORD)GlobalSize(lpgd);

    lptag = (LPPMTAG)((LPSTR)lpgd+lpgd->cbGroup);

    if ((DWORD)((PCHAR)lptag - (PCHAR)lpgd +MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb))+4) <= cbSeg
        && lptag->wID == ID_MAGIC
        && lptag->wItem == (int)0xFFFF
        && lptag->cb == (WORD)(MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)) + 4)
        && *(PLONG)lptag->rgb == PMTAG_MAGIC)
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
/*                                                                            */
/*  LockGroup() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

/* Given the handle to the group's window, lock the group segment and return
 * a pointer thereto.  Reloads the group segment if it is not in memory.
 */

LPGROUPDEF FAR PASCAL LockGroup(HWND hwndGroup)

{
  PGROUP     pGroup;
  LPGROUPDEF lpgd;
  WORD       status;
  LPTSTR      lpszKey;
  HKEY       hKey = NULL;
  LONG       err;
  DWORD      cbMaxValueLen = 0;
  FILETIME   ft;
  TCHAR       szClass[64];
  DWORD      dummy = 64;
  DWORD      cbSecDesc;
  HKEY       hkeyGroups;
  BOOL       bCommonGroup;

  wLockError = 0;   // No errors.

  /* Find the handle and try to lock it. */
  pGroup = (PGROUP)GetWindowLong(hwndGroup, GWLP_PGROUP);
  lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);

  /* If we got a non-NULL selector, return the pointer. */
  if (pGroup->fLoaded)
      return(lpgd);

  if (lpgd) {
      GlobalUnlock(pGroup->hGroup);
  }

  NukeIconBitmap(pGroup);        // invalidate the bitmap

  /* The group has been discarded, must reread the file... */
  lpszKey = pGroup->lpKey;

  pGroup->fRO = FALSE;

  bCommonGroup = pGroup->fCommon;
  if (bCommonGroup)
      hkeyGroups = hkeyCommonGroups;
  else if (bUseANSIGroups)
      hkeyGroups = hkeyAnsiProgramGroups;
  else
      hkeyGroups = hkeyProgramGroups;

  if (!hkeyGroups)
      goto RegError;

  /* Try to open the group key. */
  if (err = RegOpenKeyEx(hkeyGroups, lpszKey, 0,
                         DELETE | KEY_READ | KEY_WRITE,
                         &hKey)) {
      /* Try read-only access */
      if (err = RegOpenKeyEx(hkeyGroups, lpszKey, 0,
                         KEY_READ, &hKey) || !hKey) {
          status = IDS_NOGRPFILE;
          goto LGError1;
      }
      if (!bUseANSIGroups) {
          pGroup->fRO = TRUE;
      }
  }

  if (!(err = RegQueryInfoKey(hKey,
                              szClass,
                              &dummy,   // cbClass
                              NULL,     // Title index
                              &dummy,   // cbSubKeys
                              &dummy,   // cb Max subkey length
                              &dummy,   // max class len
                              &dummy,   // values count
                              &dummy,   // max value name length
                              &cbMaxValueLen,
                              &cbSecDesc,   // cb Security Descriptor
                              &ft))) {
      if (!pGroup->ftLastWriteTime.dwLowDateTime &&
                   !pGroup->ftLastWriteTime.dwHighDateTime)
          pGroup->ftLastWriteTime = ft;
      else if (pGroup->ftLastWriteTime.dwLowDateTime != ft.dwLowDateTime ||
               pGroup->ftLastWriteTime.dwHighDateTime != ft.dwHighDateTime ) {
          wLockError = LOCK_FILECHANGED;
          status = IDS_GRPHASCHANGED;
          if (!fExiting)     // Don't reload changed groups on exit.
              PostMessage(hwndProgman,WM_RELOADGROUP,(WPARAM)pGroup,0L);
          goto LGError2;
      }
  }

  /* Find the size of the file by seeking to the end. */
  if (cbMaxValueLen < sizeof(GROUPDEF)) {
      status = IDS_BADFILE;
      goto LGError2;
  }

  /* Allocate some memory for the thing. */
  CheckBeforeReAlloc(pGroup->hGroup);
  if (!GlobalReAlloc(pGroup->hGroup, (DWORD)cbMaxValueLen, GMEM_MOVEABLE)) {
      wLockError = LOCK_LOWMEM;
      status = IDS_LOWMEM;
      lpszKey = NULL;
      goto LGError2;
  }

  pGroup->fLoaded = TRUE;
  lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);

  /* Read the whole group data into memory. */
  status = IDS_BADFILE;
  if (err = RegQueryValueEx(hKey, NULL, 0, 0, (LPBYTE)lpgd, &cbMaxValueLen)) {
      goto LGError3;
  }
  //
  // If we start out from the ANSI groups, we need the security description
  // to copy the entire information to the UNICODE groups
  //
  if (bUseANSIGroups) {
      pGroup->pSecDesc = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, cbSecDesc);
      RegGetKeySecurity(hKey, DACL_SECURITY_INFORMATION, pGroup->pSecDesc, &cbSecDesc);
  }
  else {
      pGroup->pSecDesc = NULL;
  }

  //
  // If we loaded an old format ANSI group, then convert it to the
  // UNICODE format and save it back in the registry.
  //
  if (lpgd->dwMagic == GROUP_MAGIC) {
      HANDLE hUNIGroup;

      if (cbMaxValueLen = ConvertToUnicodeGroup((LPGROUPDEF_A)lpgd, &hUNIGroup)) {
          UnlockGroup(hwndGroup);
          /* Free the ANSI group. */
          GlobalFree(pGroup->hGroup);
          pGroup->hGroup = hUNIGroup;
          lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);
      }
      else {
          goto LGError3;
      }
  }

  if (lpgd->dwMagic != GROUP_UNICODE)
      goto LGError3;

  if (lpgd->cbGroup > cbMaxValueLen)
      goto LGError3;

  /* Now return the pointer. */
  RegCloseKey(hKey);

  return(lpgd);

LGError3:
  GlobalUnlock(pGroup->hGroup);
  GlobalDiscard(pGroup->hGroup);
  pGroup->fLoaded = FALSE;

LGError2:
  RegCloseKey(hKey);

LGError1:
  if (status != IDS_LOWMEM && status != IDS_GRPHASCHANGED && status != IDS_NOGRPFILE) {
      MyMessageBox(hwndProgman, IDS_GROUPFILEERR, status, pGroup->lpKey,
                   MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
      if (status == IDS_BADFILE) {
          //
          // stop handling of Program Groups key changes.
          //
          bHandleProgramGroupsEvent = FALSE;

          RegDeleteKey(hkeyGroups, lpszKey);

          //
          // reset handling of Program Groups key changes.
          //
          ResetProgramGroupsEvent(bCommonGroup);
          bHandleProgramGroupsEvent = TRUE;
      }
      return(NULL);
  }

  /*
   * Special case the group not being found so we can delete it's entry...
   */
  if (status == IDS_NOGRPFILE) {
      /*
       * If no restrictions then we can fixup progman.ini...
       */
      if (!fNoSave && dwEditLevel < 1) {
          TCHAR szGroup[10];

          if (MyMessageBox(hwndProgman,IDS_GROUPFILEERR,IDS_NOGRPFILE2,lpszKey, MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON1 | MB_SYSTEMMODAL) == IDNO) {
              wsprintf(szGroup,TEXT("Group%d"),pGroup->wIndex);
              //
              // stop handling of Program Groups key changes.
              //
              bHandleProgramGroupsEvent = FALSE;
              RegDeleteKey(hkeyProgramGroups, lpszKey);

              //
              // reset handling of Program Groups key changes.
              //
              ResetProgramGroupsEvent(bCommonGroup);
              bHandleProgramGroupsEvent = TRUE;
              RegDeleteValue(hkeyPMGroups, szGroup);

              if (!fFirstLoad)
                  PostMessage(hwndProgman,WM_UNLOADGROUP,(WPARAM)hwndGroup,0L);
          }
      }
      else {
RegError:
          /*
           * Restrictions mean that the user can only OK this error...
           */
          MyMessageBox(hwndProgman, IDS_GROUPFILEERR, IDS_NOGRPFILE, lpszKey,
                           MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
      }
  }

  ShowWindow(hwndGroup, SW_SHOWMINNOACTIVE);

  return(NULL);
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  UnlockGroup() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

void FAR PASCAL UnlockGroup(register HWND hwndGroup)

{
  GlobalUnlock(((PGROUP)GetWindowLongPtr(hwndGroup,GWLP_PGROUP))->hGroup);
}

/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  LockItem() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

LPITEMDEF FAR PASCAL LockItem(PGROUP pGroup, PITEM pItem)
{
  LPGROUPDEF        lpgd;

  lpgd = LockGroup(pGroup->hwnd);

  if (!lpgd)
      return((LPITEMDEF)NULL);

  return ITEM(lpgd,pItem->iItem);
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  KeepGroupAround() -                                                     */
/*                                                                            */
/*--------------------------------------------------------------------------*/

/*
 * Sets or unsets the discardable flag for the given group file.  If setting
 * to non-discard, forces the group to be in memory.
 */

HANDLE PASCAL KeepGroupAround(HWND hwndGroup, BOOL fKeep)
{
    PGROUP pGroup;

    UNREFERENCED_PARAMETER(fKeep);
    pGroup = (PGROUP)GetWindowLongPtr(hwndGroup, GWLP_PGROUP);
    return pGroup->hGroup;

#ifdef ORGCODE
    PGROUP pGroup;
    WORD flag;

    pGroup = (PGROUP)GetWindowLongPtr(hwndGroup, GWLP_PGROUP);

    if (fKeep) {
        if (LockGroup(hwndGroup)) {
            UnlockGroup(hwndGroup);  // it is still in memory
        } else {
            return NULL; // failure
        }

        flag = GMEM_MODIFY | GMEM_MOVEABLE;  // make non discardable
    } else {
        flag = GMEM_MODIFY | GMEM_MOVEABLE | GMEM_DISCARDABLE;  // discardable
    }

    return GlobalReAlloc(pGroup->hGroup, 0, flag);
#endif
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SaveGroup() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*
 * Writes out a group file.  It must already be in memory or the operation
 * is meaningless.
 */
BOOL APIENTRY SaveGroup(
    HWND hwndGroup, BOOL bDiscard
    )
{
    LPGROUPDEF lpgd;
    HKEY       hKey;
    PGROUP     pGroup;
    WORD       status = 0;
    DWORD      cb;
    LONG       err;
    HKEY       hkeyGroups;
    BOOL       bCommonGroup;
    DWORD      dwDisposition;

    pGroup = (PGROUP)GetWindowLongPtr(hwndGroup, GWLP_PGROUP);

    bCommonGroup = pGroup->fCommon;

    if (!bUseANSIGroups && IsGroupReadOnly(pGroup->lpKey, bCommonGroup)) {
        // Don't produce an error message for RO groups.
        return FALSE;
    }

    lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);
    if (!lpgd) {
        return FALSE;
    }

    if (bCommonGroup)
        hkeyGroups = hkeyCommonGroups;
    else
        hkeyGroups = hkeyProgramGroups;

    if (!hkeyGroups) {
        goto Exit1;
    }

    // it may already exist

    if (err = RegCreateKeyEx(hkeyGroups, pGroup->lpKey, 0, 0, 0,
                     DELETE | KEY_READ | KEY_WRITE | WRITE_DAC,
                     pSecurityAttributes, &hKey, &dwDisposition)) {
    //if (err = RegOpenKeyEx(hkeyGroups, pGroup->lpKey, 0,
    //                              KEY_SET_VALUE, &hKey)) {
        /*
         * We can't open output group key.
         */
        if (err = RegOpenKeyEx(hkeyGroups, pGroup->lpKey, 0,
                                  KEY_READ, &hKey)) {
            status = IDS_NOGRPFILE;
        } else {
            // status = IDS_GRPISRO;
            RegCloseKey(hKey);
        }
        goto Exit1;
    }
    else {
        if (dwDisposition == REG_CREATED_NEW_KEY && bUseANSIGroups) {
            RegSetKeySecurity(hKey, DACL_SECURITY_INFORMATION, pGroup->pSecDesc);
            LocalFree(pGroup->pSecDesc);
            pGroup->pSecDesc = NULL;
        }

    }

    //
    // stop handling Program Groups key changes for a SAveGroup.
    //
    bHandleProgramGroupsEvent = FALSE;

    cb = SizeofGroup(lpgd);
    if (err = RegSetValueEx(hKey, NULL, 0, REG_BINARY, (LPBYTE)lpgd, cb)) {
        status = IDS_CANTWRITEGRP;
    }

    RegFlushKey(hKey);
    RegCloseKey(hKey);

    pGroup->ftLastWriteTime.dwLowDateTime = 0;   // update file time stamp if we need to reload
    pGroup->ftLastWriteTime.dwHighDateTime = 0;

Exit1:
    GlobalUnlock(pGroup->hGroup);

    if (status && !fExiting) {
        MyMessageBox(hwndProgman, IDS_GROUPFILEERR, status, pGroup->lpKey,
                MB_OK | MB_ICONEXCLAMATION);

        /*
         * Force the group to be reset.
         */
        if (bDiscard) {
            GlobalDiscard(pGroup->hGroup);
            pGroup->fLoaded = FALSE;
            InvalidateRect(pGroup->hwnd, NULL, TRUE);
        }
    }

    //
    // reset handling of Program Groups key changes.
    //
    ResetProgramGroupsEvent(bCommonGroup);
    bHandleProgramGroupsEvent = TRUE;
    return (status == 0);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AdjustPointers() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*
 * Adjusts pointers in the segment after a section is moved up or down.
 */
void PASCAL AdjustPointers(LPGROUPDEF lpgd, DWORD iFirst, DWORD di)
{
    WORD i;
    LPITEMDEF lpid;

    if (lpgd->pName >= iFirst) {
        lpgd->pName += di;
    }

    for (i = 0; i < lpgd->cItems; i++) {
        if (!lpgd->rgiItems[i]) {
            continue;
        }

        if (lpgd->rgiItems[i] >= iFirst) {
            lpgd->rgiItems[i] += di;
        }

        lpid = ITEM(lpgd, i);

        if (lpid->pIconRes >= iFirst)
            lpid->pIconRes += di;
        if (lpid->pName >= iFirst)
            lpid->pName += di;
        if (lpid->pCommand >= iFirst)
            lpid->pCommand += di;
        if (lpid->pIconPath >= iFirst)
            lpid->pIconPath += di;
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FindFreeItemIndex() -                                                   */
/*                                                                          */
/* Returns the index of a free slot in the item offset array.  If necessary,*/
/* moves stuff around.                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

WORD PASCAL FindFreeItemIndex(HWND hwndGroup)
{
    LPGROUPDEF lpgd;
    PGROUP     pGroup;
    WORD       i;
    LPTSTR      lp1;
    LPTSTR      lp2;
    DWORD      cb;

    lpgd = LockGroup(hwndGroup);
    if (!lpgd) {
        return(0xFFFF);
    }

    for (i = 0; i < lpgd->cItems; i++) {
        if (!lpgd->rgiItems[i]) {
            UnlockGroup(hwndGroup);
            return(i);
        }
    }

    /*
     * Didn't find an empty slot... make some new ones.
     */
    pGroup = (PGROUP)GetWindowLongPtr(hwndGroup, GWLP_PGROUP);

    // Current groups+tags size.
    cb = SizeofGroup(lpgd);

    // Increase space reserved item info.
    lpgd->cbGroup += NSLOTS*sizeof(DWORD);

    // Increase size of whole group.
    cb += NSLOTS*sizeof(DWORD);

    UnlockGroup(hwndGroup);

    CheckBeforeReAlloc(pGroup->hGroup);
    if (!GlobalReAlloc(pGroup->hGroup, cb, GMEM_MOVEABLE)) {
        return 0xFFFF;
    }

    lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);

    /*
     * Copy tags junk (which starts at the end of the rgiItems array)
     * up a bit to make room for the bigger array..
     */
    lp1 = (LPTSTR)&(lpgd->rgiItems[lpgd->cItems]);
    lp2 = (LPTSTR)&(lpgd->rgiItems[lpgd->cItems + NSLOTS]);

    /*
     * Copy everything down in the segment.
     */
    RtlMoveMemory(lp2, lp1, (WORD)(cb - (DWORD)((LPSTR)lp2 - (LPSTR)lpgd)));

    /*
     * Zero out the new offsets.
     */
    for (i = (WORD)lpgd->cItems; i < (WORD)(lpgd->cItems + NSLOTS); i++) {
        lpgd->rgiItems[i] = 0;
    }

    i = lpgd->cItems;

    /* Record that we now have more slots */
    lpgd->cItems += NSLOTS;

    /*
     * Fix up all the offsets in the segment.  Since the rgiItems array is
     * part of the group header, all the pointers will change.
     */
    AdjustPointers(lpgd, (WORD)1, NSLOTS * sizeof(DWORD));

    GlobalUnlock(pGroup->hGroup);

    return i;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DeleteThing() -                                                         */
/*                                                                          */
/*                                                                          */
/* Removes a part of the group segment.  Updates everything in the segment  */
/* but does not realloc.                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void NEAR PASCAL DeleteThing(LPGROUPDEF lpgd, LPDWORD lpiThing, WORD cbThing)
{
  DWORD       dwThingOffset;
  LPTSTR      lp1;
  LPTSTR      lp2;
  INT        cb;
  WORD       cbThingSize;

  if (cbThing == 0xFFFF) {
      return;
  }

  dwThingOffset = *lpiThing;

  if (!dwThingOffset)
      return;

  *lpiThing = 0;

  lp1 = (LPTSTR) PTR(lpgd, dwThingOffset);

  /* If its a string we're removing, the caller can pass 0 as the length
   * and have it calculated!!!
   */
  if (!cbThing) {
      cbThing = (WORD)sizeof(TCHAR)*(1 + lstrlen(lp1));
  }

  cbThingSize = (WORD)MyDwordAlign((int)cbThing);

  lp2 = (LPTSTR)((LPBYTE)lp1 + cbThingSize);

  cb = (int)SizeofGroup(lpgd);

  RtlMoveMemory(lp1, lp2, (cb - (DWORD)((LPSTR)lp2 - (LPSTR)lpgd)));

  lpgd->cbGroup -= cbThingSize;

  AdjustPointers(lpgd, dwThingOffset, -cbThingSize);

}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddThing() -                                                            */
/*                                                                          */
/* in:                                                                      */
/*	hGroup	group handle, must not be discardable                           */
/*	lpStuff	pointer to data or NULL to init data to zero                    */
/*	cbStuff	count of item (may be 0) if lpStuff is a string                 */
/*                                                                          */
/* Adds an object to the group segment and returns its offset.	Will        */
/* reallocate the segment if necessary.                                     */
/*                                                                          */
/* Handle passed in must not be discardable                                 */
/*                                                                          */
/* returns:                                                                 */
/*	0	failure                                                             */
/*	> 0	offset to thing in the segment                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

DWORD PASCAL AddThing(HANDLE hGroup, LPTSTR lpStuff, DWORD cbStuff)
{
    DWORD        cb;
    LPGROUPDEF  lpgd;
    DWORD        offset;
    LPTSTR       lpT;
    DWORD        cbStuffSize;
    DWORD        cbGroupSize;
    DWORD        myOffset;

    if (cbStuff == 0xFFFFFFFF) {
        return 0xFFFFFFFF;
    }

    if (!cbStuff) {
        cbStuff = sizeof(TCHAR)*(DWORD)(1 + lstrlen(lpStuff));
    }

    cbStuffSize = MyDwordAlign((int)cbStuff);

    lpgd = (LPGROUPDEF)GlobalLock(hGroup);
    cb = SizeofGroup(lpgd);
    cbGroupSize = MyDwordAlign((int)cb);

    offset = lpgd->cbGroup;
    myOffset = (DWORD)MyDwordAlign((int)offset);

    GlobalUnlock(hGroup);

    CheckBeforeReAlloc(hGroup);
    if (!GlobalReAlloc(hGroup,(DWORD)(cbGroupSize + cbStuffSize), GMEM_MOVEABLE))
        return 0;

    lpgd = (LPGROUPDEF)GlobalLock(hGroup);

    /*
     * Slide the tags up
     */
    RtlMoveMemory((LPSTR)lpgd + myOffset + cbStuffSize, (LPSTR)lpgd + myOffset,
                            (cbGroupSize - myOffset));
    lpgd->cbGroup += cbStuffSize;

    lpT = (LPTSTR)((LPSTR)lpgd + myOffset);
    if (lpStuff) {
        RtlMoveMemory(lpT, lpStuff, cbStuff);

    } else {
        /*
         * Zero it
         */
        while (cbStuffSize--) {
            *((LPSTR)lpT)++ = 0;
        }
    }


    GlobalUnlock(hGroup);

    return myOffset;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FindTag() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LPPMTAG NEAR PASCAL FindTag(LPGROUPDEF lpgd, int item, WORD id)
{
    LPPMTAG lptag;
    int cbSeg;
    int cb;

    cbSeg = (DWORD)GlobalSize(lpgd);

    lptag = (LPPMTAG)((LPSTR)lpgd+lpgd->cbGroup);

    if ((PCHAR)lptag - (PCHAR)lpgd + MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)) + 4 <= cbSeg
        && lptag->wID == ID_MAGIC
        && lptag->wItem == (int)0xFFFF
        && lptag->cb == (WORD)(MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)) +4)
        && *(LONG FAR *)lptag->rgb == PMTAG_MAGIC) {

        while ((cb = (int)((PCHAR)lptag - (PCHAR)lpgd + MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)))) <= cbSeg)
        {
            if ((item == lptag->wItem)
                && (id == 0 || id == lptag->wID)) {
                return lptag;
            }

            if (lptag->wID == ID_LASTTAG)
                return NULL;

            (LPSTR)lptag += lptag->cb;
        }
    }
    return NULL;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CopyTag() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT FAR PASCAL CopyTag(LPGROUPDEF lpgd, int item, WORD id, LPTSTR lpbuf, int cb)
{
    LPTSTR lpt;
    LPPMTAG lptag;
    WORD cbT;

    lptag = FindTag(lpgd,item,id);

    if (lptag == NULL)
        return 0;

    if (cb > (int)lptag->cb)
        cb = lptag->cb;

    cbT = (WORD)cb;

    lpt = (LPTSTR) lptag->rgb;

    while (*lpt && cbT) {
       *lpbuf++=*lpt++;
       cbT--;
    }

    if (!(*lpt) && cbT) {
        *lpbuf = TEXT('\0');
    }

    return cb;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DeleteTag() -                                                           */
/*                                                                          */
/* in:                                                                      */
/*	hGroup	group handle, can be discardable (alwayws shrink object)        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID FAR PASCAL DeleteTag(HANDLE hGroup, int item, WORD id)
{
    LPPMTAG lptag;
    LPTSTR lp1, lp2;
    LPTSTR lpend;
    LPGROUPDEF lpgd;

    lpgd = (LPGROUPDEF) GlobalLock(hGroup);

    lptag = FindTag(lpgd,item,id);

    if (lptag == NULL) {
        GlobalUnlock(hGroup);
        return;
    }

    lp1 = (LPTSTR)lptag;

    lp2 = (LPTSTR)((LPSTR)lptag + lptag->cb);

    lpend = (LPTSTR)((LPSTR)lpgd + SizeofGroup(lpgd));

    while (lp2 < lpend) {
        *lp1++ = *lp2++;
    }

    /* always reallocing smaller
     */
    GlobalUnlock(hGroup);
    CheckBeforeReAlloc(hGroup);
    GlobalReAlloc(hGroup, (DWORD)((LPSTR)lp1 - (LPSTR)lpgd), 0);

    return;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddTag() -                                                              */
/*                                                                          */
/* in:                                                                      */
/*	h	group handle, must not be discardable!                              */
/*                                                                          */
/* returns:                                                                 */
/*  0	failure                                                             */
/*	1	success                                                             */
/*--------------------------------------------------------------------------*/
INT PASCAL AddTag(HANDLE h, int item, WORD id, LPTSTR lpbuf, int cb)
{
    LPPMTAG lptag;
    WORD fAddFirst;
    LPGROUPDEF lpgd;
    int cbNew;
    int cbMyLen;
    LPGROUPDEF lpgdOld;


    if (!cb && lpbuf) {
        cb = sizeof(TCHAR)*(lstrlen(lpbuf) + 1);
    }
    cbMyLen = MyDwordAlign(cb);

    if (!lpbuf) {
        cb = 0;
        cbMyLen = 0;
    }

    /*
     * Remove the old version of the tag, if any.
     */
    DeleteTag(h, item, id);

    lpgd = (LPGROUPDEF)GlobalLock(h);

    lptag = FindTag(lpgd, (int)0xFFFF, (WORD)ID_LASTTAG);

    if (!lptag) {
        /*
         * In this case, there are no tags at all, and we have to add
         * the first tag, the interesting tag, and the last tag
         */
        cbNew = 3 * (MyDwordAlign(sizeof(PMTAG)) - MyDwordAlign(sizeof(lptag->rgb))) + 4 + cbMyLen;
        fAddFirst = TRUE;
        lptag = (LPPMTAG)((LPSTR)lpgd + lpgd->cbGroup);

    } else {
        /*
         * In this case, only the interesting tag needs to be added
         * but we count in the last because the delta is from lptag
         */
        cbNew = 2 * (MyDwordAlign(sizeof(PMTAG)) - MyDwordAlign(sizeof(lptag->rgb))) + cbMyLen;
        fAddFirst = FALSE;
    }

    /*
     * check for 64K limit
     */
    if ((DWORD_PTR)lptag + cbNew < (DWORD_PTR)lptag) {
        return 0;
    }

    cbNew += (int)((PCHAR)lptag -(PCHAR)lpgd);
    lpgdOld = lpgd;
    GlobalUnlock(h);
    CheckBeforeReAlloc(h);
    if (!GlobalReAlloc(h, (DWORD)cbNew, GMEM_MOVEABLE)) {
        return 0;
    }

    lpgd = (LPGROUPDEF)GlobalLock(h);
    lptag = (LPPMTAG)((LPSTR)lpgd + ((LPSTR)lptag - (LPSTR)lpgdOld));
    if (fAddFirst) {
        /*
         * Add the first tag
         */
        lptag->wID = ID_MAGIC;
        lptag->wItem = (int)0xFFFF;
        *(LONG FAR *)lptag->rgb = PMTAG_MAGIC;
        lptag->cb = (WORD)(MyDwordAlign(sizeof(PMTAG)) - MyDwordAlign(sizeof(lptag->rgb)) + 4);
        (LPSTR)lptag += lptag->cb;
    }

    /*
     * Add the tag
     */
    lptag->wID = id;
    lptag->wItem = item;
    lptag->cb = (WORD)(MyDwordAlign(sizeof(PMTAG)) - MyDwordAlign(sizeof(lptag->rgb)) + cbMyLen);
    if (lpbuf) {
        RtlMoveMemory(lptag->rgb, lpbuf, (WORD)cb);
    }
    (LPSTR)lptag += lptag->cb;

    /*
     * Add the end tag
     */
    lptag->wID = ID_LASTTAG;
    lptag->wItem = (int)0xFFFF;
    lptag->cb = 0;

    GlobalUnlock(h);

    return 1;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  NukeIconBitmap -- Deletes the icon bitmap if one exists for the group   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void PASCAL NukeIconBitmap(PGROUP pGroup)
{
    if (pGroup->hbm) {
        DeleteObject(pGroup->hbm);
        pGroup->hbm = NULL;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GroupFlag() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

WORD PASCAL GroupFlag(PGROUP pGroup, PITEM pItem, WORD wFlag)
{
    LPGROUPDEF lpgd;
    LPPMTAG lptag;
    WORD wT = 0;
    int wItem;

    if (pItem) {
        wItem = pItem->iItem;
    } else {
        wItem = (int)0xFFFF;
    }

    lpgd = LockGroup(pGroup->hwnd);
    if (!lpgd)
        return 0;

    lptag = FindTag(lpgd, wItem, wFlag);

    if (!lptag)
        wT = 0;
    else if (lptag->cb == (WORD)(MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb))))
        wT = 1;
    else if (lptag->cb == (WORD)(MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)) + 1))
        wT = lptag->rgb[0];
    else if (lptag->cb == (WORD)(MyDwordAlign(sizeof(PMTAG))-MyDwordAlign(sizeof(lptag->rgb)) + 4))
        wT = *(LPWORD)lptag->rgb;
    UnlockGroup(pGroup->hwnd);

    return wT;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetGroupTag() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

WORD PASCAL GetGroupTag(
    PGROUP pGroup,
    PITEM pItem,
    WORD id,
    LPTSTR lpT,
    WORD cb)
{
    WORD wT;
    LPGROUPDEF lpgd;
    int wItem;

    if (pItem)
        wItem = pItem->iItem;
    else
        wItem = (int)0xFFFF;

    lpgd = LockGroup(pGroup->hwnd);
    if (!lpgd)
        return 0;

    wT = (WORD)CopyTag(lpgd, wItem, id, lpT, cb);

    UnlockGroup(pGroup->hwnd);
    return wT;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ChangeTagID() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void PASCAL ChangeTagID(
    LPGROUPDEF lpgd,
    int iOld,
    int iNew)
{
    LPPMTAG lptag;

    while (lptag = FindTag(lpgd,iOld,0)) {
        lptag->wItem = iNew;
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  LoadItem() -                                                            */
/*                                                                          */
/* Creates an item window (iconic) within a group window.  Assumes that the */
/* group segment is up to date.                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PITEM PASCAL LoadItem(HWND hwndGroup, WORD iItem, BOOL bActivate)
{
    LPGROUPDEF lpgd;
    LPITEMDEF  lpid;
    PGROUP     pGroup;
    PITEM      pItem;
    PITEM      *ppItem;

    lpgd = LockGroup(hwndGroup);
    if (!lpgd)
        return NULL;

    pItem = (PITEM)LocalAlloc(LPTR, sizeof(ITEM));
    if (!pItem) {
        UnlockGroup(hwndGroup);
        return NULL;
    }

    pGroup = (PGROUP)GetWindowLongPtr(hwndGroup, GWLP_PGROUP);

    NukeIconBitmap(pGroup);

    if (bActivate) {
        InvalidateIcon(pGroup, pGroup->pItems);
        pItem->pNext = pGroup->pItems;
        pGroup->pItems = pItem;
    } else {
        ppItem = &pGroup->pItems;
        while (*ppItem) {
            ppItem = &((*ppItem)->pNext);
        }
        pItem->pNext = NULL;
        *ppItem = pItem;
    }

    lpid = ITEM(lpgd, iItem);

    pItem->iItem = iItem;
    pItem->dwDDEId = 0;
    SetRectEmpty(&pItem->rcTitle);
    SetRectEmpty(&pItem->rcIcon);

    ComputeIconPosition(pGroup, lpid->pt, &pItem->rcIcon, &pItem->rcTitle,
            (LPTSTR) PTR(lpgd, lpid->pName));

    UnlockGroup(hwndGroup);

    InvalidateIcon(pGroup, pItem);

    return pItem;
}


PITEM FindItemName(LPGROUPDEF lpgd, register PITEM pItem, LPTSTR lpTitle)
{
  LPITEMDEF  lpid;

  while (pItem) {
      lpid = ITEM(lpgd, pItem->iItem);

      if (!lstrcmp(lpTitle, (LPTSTR) PTR(lpgd, lpid->pName)))
        return pItem;

      pItem = pItem->pNext;
  }

  return NULL;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CreateNewItem() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*
 * Creates a new item in the file, and adds a window for it.
 */
PITEM PASCAL CreateNewItem(
    HWND    hwndGroup,
    LPTSTR   lpTitle,
    LPTSTR   lpCommand,
    LPTSTR   lpIconPath,
    LPTSTR   lpDefDir,
    WORD    wHotKey,
    BOOL    fMinimize,
    WORD    wIconId,
    WORD    wIconIndex,
    HICON   hIcon,
    LPPOINT lppt,
    DWORD   dwFlags)
{
    LPGROUPDEF lpgd;
    LPITEMDEF  lpid;
    WORD       id;
    DWORD      offset;
    PGROUP     pGroup;
    LPTSTR     lpIconRes;
    WORD       cbIconRes;
    //DWORD    dwVer;
    WORD       wVer;
    WORD       idError = IDS_LOWMEM;
    PITEM      pItem;
    DWORD      cb;
    TCHAR      szCommand[3*MAX_PATH];
    TCHAR      szExeDir[MAXITEMPATHLEN + 1];
    TCHAR      szIconExe[MAX_PATH];
    TCHAR      szTemp[MAXITEMPATHLEN+1];
    LPTSTR     lp1, lp2, lp3;
    HANDLE     hIconRes;
    HANDLE     hModule;
    BOOL       fWin32App = FALSE;
    BOOL       fUseDefaultIcon = FALSE;
    BOOL       bNoIconPath = TRUE;
    TCHAR      cSeparator;

    /*
     * Before we do anything, whack the command line and exedir
     */
    lp1 = lpCommand;
    if (*lpCommand == TEXT('"') && wcschr(lpCommand + 1, TEXT('"'))) {
        cSeparator = TEXT('"');
        //lp1++;
    }
    else {
        cSeparator = TEXT(' ');
    }
    for (lp2=lp3=szExeDir; *lp1 && *lp1 != cSeparator; lp1 = CharNext(lp1))
    {
        *lp2++ = *lp1;

        /*
         * We know we're looking at the first byte
         */
        if ((*lp1 == TEXT(':')) || (*lp1 == TEXT('\\'))) {
            lp3 = lp2;
        }
    }

    *lp2 = 0;

    /*
     * If the default dir pointer is NULL then we use the directory
     * component of the command line.  Otherwise we do the normal
     * path whacking stuff to get everything into 3.0 format.
     */
    if (lpDefDir) {
        LPTSTR lpT;

        lpT = lpDefDir;
        // We have a valid pointer.
    	lstrcpy(szCommand,lpDefDir);
#if 0
/* spaces are allowed in LFN.
 */
        RemoveLeadingSpaces(szCommand);
#endif

        // If a default dir was supplied then go ahead and whack it
        // into 3.0 format otherwise leave it blank.
        if (*lpDefDir)
        {
            LPTSTR lpNextChar;

            // locate the character before the NULL
            while ( *(lpNextChar = CharNext(lpDefDir)) )
                lpDefDir = lpNextChar;

            // If there is no '\' seperator, add one.
            if (lpDefDir[0] != TEXT('\\')) {
                lstrcat(szCommand,TEXT("\\"));
            }
        }

        /*
         * Now add the filename itself.  this puts the command in the
         * 3.0 format: defdir\exename
         */
        lstrcat(szCommand, lp3);

        /*
         * Append the arguments
         */
        lstrcat(szCommand, lp1);
        lpDefDir = lpT;

    } else {
        /*
         * Use the same command line (note def dir is assigned exe dir
         */
        lstrcpy(szCommand, lpCommand);
    }

    /*
     * Now truncate exedir so that it does not include the command filename
     */
    *lp3 = 0;

    pGroup = (PGROUP)GetWindowLongPtr(hwndGroup, GWLP_PGROUP);
    lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);

    if (!GroupCheck(pGroup)) {
    	MyMessageBox(hwndProgman, IDS_GROUPFILEERR, IDS_GROUPRO,
                     (LPTSTR) PTR(lpgd, lpgd->pName),
                     MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
	GlobalUnlock(pGroup->hGroup);
        return NULL;
    }
    GlobalUnlock(pGroup->hGroup);

    if (*lpIconPath) {
	    bNoIconPath = FALSE;
    }

    lstrcpy(szIconExe, lpIconPath);
    if (bNoIconPath && !(dwFlags & CI_NO_ASSOCIATION)) {
        lstrcpy(szIconExe, lpCommand);
    }
    DoEnvironmentSubst(szIconExe, (WORD)CharSizeOf(szIconExe));
    StripArgs(szIconExe);
    if (!bNoIconPath) {
        TagExtension(szIconExe, sizeof(szIconExe));
    }

    if (*szIconExe == TEXT('"') && *(szIconExe + lstrlen(szIconExe)-1) == TEXT('"')) {
        SheRemoveQuotes(szIconExe);
    }

    if (bNoIconPath) {
        //
        // if it's a relative path, extractassociatedicon and LoadLibrary don't
        // handle that so find the executable first
        //
        SetCurrentDirectory(szOriginalDirectory);
        FindExecutable(szIconExe, lpDefDir, szTemp);
        if (*szTemp) {
            lstrcpy(szIconExe, szTemp);
            TagExtension(szIconExe, sizeof(szIconExe));
            if (*szIconExe == TEXT('"') && *(szIconExe + lstrlen(szIconExe)-1) == TEXT('"')) {
		        SheRemoveQuotes(szIconExe);
	        }
        }
        else {
            *szIconExe = 0;    // Use a dummy value so no icons will be found
                               // and progman's item icon will be used instead
                               // This is to make moricons.dll item icon be the
                               // right one.  -johannec 6/4/93
        }
        //
        // reset the current directory to progman's working directory i.e. Windows directory
        //
        SetCurrentDirectory(szWindowsDirectory);

        wIconId = 0;
        wIconIndex = 0;
    }


NoIcon:

    if (!wIconId) {
        TCHAR szOldIconExe[MAX_PATH];

        lstrcpy(szOldIconExe, szIconExe);
        hIcon = ExtractAssociatedIconEx(hAppInstance, szIconExe, &wIconIndex, &wIconId);
        if (lstrcmp(szOldIconExe, szIconExe)) {
            /* using default icon from Progman.exe */
            fUseDefaultIcon = TRUE;
        }
        if (hIcon)
            DestroyIcon(hIcon);
    }

    lpIconRes = NULL;
    hIconRes = NULL;
    if (hModule = LoadLibrary(szIconExe)) {
        fWin32App = TRUE;
        hIconRes = FindResource(hModule, (LPTSTR) MAKEINTRESOURCE(wIconId), (LPTSTR) MAKEINTRESOURCE(RT_ICON));
        if (hIconRes) {
            //dwVer = 0x00030000;  // resource version is windows 3.x
            wVer = 3;  // resource version is windows 3.x
            cbIconRes = (WORD)SizeofResource(hModule, hIconRes);
            hIconRes = LoadResource(hModule, hIconRes);
            lpIconRes = LockResource(hIconRes);
        }
        if (fUseDefaultIcon) {
            wIconId = 0;
        }
    }
    else { // Win 3.1 app

        if (wVer = ExtractIconResInfo(hAppInstance, szIconExe, wIconIndex, &cbIconRes, &hIconRes)){
            lpIconRes = GlobalLock(hIconRes);
        }
    }

    if (!lpIconRes) {
       wIconId = 0;
       wIconIndex = 0;

       // ToddB: I see no harm in always setting the current directory to the WinDir
       //   before jumping back to NoIcon.  Seems to be required to fix a Japanese bug.
       //   The WinDir is the default directory of Progman anyhow, I really don't see
       //   where it's possible for us to not already be in this directory.
       SetCurrentDirectory(szWindowsDirectory);

       goto NoIcon;
    }

    if (!KeepGroupAround(hwndGroup, TRUE)) {
        goto FreeIcon;
    }

    id = FindFreeItemIndex(hwndGroup);
    if (id == 0xFFFF) {
        goto FreeIcon;
    }

    if (id >= CITEMSMAX) {                      // check group size limit
        idError = IDS_TOOMANYITEMS;
        goto FreeIcon;
    }

    offset = AddThing(pGroup->hGroup, (TCHAR)0, (WORD)sizeof(ITEMDEF));
    lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);

    if (!offset) {
        goto QuitThis;
    }

    lpgd->rgiItems[id] = offset;
    lpid = ITEM(lpgd, id);

    GlobalUnlock(pGroup->hGroup);
    offset = AddThing(pGroup->hGroup, lpTitle, (WORD)0);
    lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);
    lpid = ITEM(lpgd, id);
    if (!offset) {
        goto PuntCreation;
    }

    lpid->pName = offset;

    GlobalUnlock(pGroup->hGroup);
    offset = AddThing(pGroup->hGroup, szCommand,(WORD) 0);
    lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);
    lpid = ITEM(lpgd, id);
    if (!offset) {
        goto PuntCreation;
    }
    lpid->pCommand = offset;

    GlobalUnlock(pGroup->hGroup);
    CheckEscapes(szIconExe, CharSizeOf(szIconExe));
    offset = AddThing(pGroup->hGroup, szIconExe,(WORD) 0);
    lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);
    lpid = ITEM(lpgd, id);
    if (!offset)
        goto PuntCreation;
    lpid->pIconPath = offset;

    GlobalUnlock(pGroup->hGroup);
    offset = AddThing(pGroup->hGroup, lpIconRes, cbIconRes);
    lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);
    lpid = ITEM(lpgd, id);
    if (!offset)
        goto PuntCreation;
    lpid->pIconRes = offset;

    if (lppt) {
        lpid->pt = *lppt;
    } else {
        lpid->pt.x = lpid->pt.y = -1;
    }

    lpid->iIcon = wIconId;
    lpid->wIconIndex = wIconIndex;
    //lpid->dwIconVer = dwVer;
    lpid->wIconVer = wVer;
    lpid->cbIconRes = cbIconRes;

    if (cbIconRes != 0xFFFF)
    if (fWin32App) {
        UnlockResource(hIconRes);
        FreeResource(hIconRes);
        FreeLibrary(hModule);
    }
    else {
        GlobalUnlock(hIconRes);
        GlobalFree(hIconRes);
    }
    GlobalUnlock(pGroup->hGroup);

    if (wHotKey) {
        AddTag(pGroup->hGroup, (int)id, (WORD)ID_HOTKEY, (LPTSTR)&wHotKey, sizeof(wHotKey));
    }
    if (fMinimize) {
        AddTag(pGroup->hGroup, (int)id, (WORD)ID_MINIMIZE, NULL, 0);
    }
    if (dwFlags & CI_SEPARATE_VDM) {
        AddTag(pGroup->hGroup, (int)id, (WORD)ID_NEWVDM, NULL, 0);
    }
    if (*szExeDir) {
        AddTag(pGroup->hGroup, (int)id, (WORD)ID_APPLICATIONDIR, szExeDir, 0);
    }

    pItem = LoadItem(hwndGroup, id, dwFlags & CI_ACTIVATE);

    lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);
    lpid = ITEM(lpgd, id);

    if (!pItem) {
        goto PuntCreation;
    }

    lpid->pt.x = pItem->rcIcon.left;
    lpid->pt.y = pItem->rcIcon.top;

    GlobalUnlock(pGroup->hGroup);

#if 0
// DOS apps are no longer set to fullscreen by default in progman
//  5-3-93 johannec (bug 8343)
#ifdef i386
    //
    // If this is a new DOS application, set the default to full screen.
    // This is only done for x86, since mips doesn't have full screen.
    //
    lstrcpy(szCommand, lpCommand);
    DoEnvironmentSubst(szCommand, (WORD)lstrlen(szCommand));
    StripArgs(szCommand);
    TagExtension(szCommand, sizeof(szCommand));
    *szTemp = 0;
    FindExecutable(szCommand, lpDefDir, szTemp);

    if ((dwFlags & CI_SET_DOS_FULLSCRN) && *szTemp && IsDOSApplication(szTemp)) {
        SetDOSApplicationToFullScreen(lpTitle);
    }
#endif
#endif

    KeepGroupAround(hwndGroup, FALSE);

    // We need to save the current group to disk now
    // in case a setup program is doing DDE with us,
    // and they reboot the system when finished.

    if (!SaveGroup (hwndGroup, FALSE)) {
        idError = 0;
        DeleteItem(pGroup, pItem);
        goto FreeIcon;
    }

    return pItem;

PuntCreation:
    /*
     * Note, must set lpid after each because it may move
     */
    DeleteThing(lpgd, (LPDWORD)&lpid->pName, 0);
    DeleteThing(lpgd, (LPDWORD)&lpid->pCommand, 0);
    DeleteThing(lpgd, (LPDWORD)&lpid->pIconPath, 0);
    DeleteThing(lpgd, (LPDWORD)&lpid->pIconRes, lpid->cbIconRes);
    DeleteThing(lpgd, (LPDWORD)&lpgd->rgiItems[id], sizeof(ITEMDEF));

QuitThis:
    cb = SizeofGroup(lpgd);
    UnlockGroup(pGroup->hwnd);

    CheckBeforeReAlloc(pGroup->hGroup);
    GlobalReAlloc(pGroup->hGroup, cb, GMEM_MOVEABLE);

    KeepGroupAround(hwndGroup, FALSE);

FreeIcon:
    if (cbIconRes != 0xFFFF)
    if (fWin32App) {
        UnlockResource(hIconRes);
        FreeResource(hIconRes);
        FreeLibrary(hModule);
    }
    else {
        GlobalUnlock(hIconRes);
        GlobalFree(hIconRes);
    }

    if (idError != 0)
        MyMessageBox(hwndProgman, IDS_GROUPFILEERR, idError, NULL,
                        MB_OK | MB_ICONEXCLAMATION);
    // Force re-read of group.
    //GlobalDiscard(pGroup->hGroup);
    //pGroup->fLoaded = FALSE ;
    //LockGroup(pGroup->hwnd);
    //UnlockGroup(pGroup->hwnd);

    return NULL;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DeleteItem() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID FAR PASCAL DeleteItem(PGROUP pGroup, PITEM pItem)
{
  LPGROUPDEF lpgd;
  LPITEMDEF  lpid;
  DWORD      cb;
  LPPMTAG    lptag;

  lpgd = LockGroup(pGroup->hwnd);
  if (!lpgd)
      return;

  if (!GroupCheck(pGroup)) {
      MyMessageBox(hwndProgman, IDS_GROUPFILEERR, IDS_GROUPRO,
                   (LPTSTR) PTR(lpgd, lpgd->pName),
                   MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
      InvalidateIcon(pGroup, pItem);
      return;
  }

  NukeIconBitmap(pGroup);

  lpid = ITEM(lpgd,pItem->iItem);

  if ( (lpgd->cbGroup != (DWORD)MyDwordAlign((int)lpgd->cbGroup)) ||
       (lpid->pName != (DWORD)MyDwordAlign((int)lpid->pName)) ||
       (lpid->pCommand != (DWORD)MyDwordAlign((int)lpid->pCommand)) ||
       (lpid->pIconPath != (DWORD)MyDwordAlign((int)lpid->pIconPath)) ||
       (lpgd->rgiItems[pItem->iItem] != (DWORD)MyDwordAlign((int)lpgd->rgiItems[pItem->iItem])) ) {

      MyMessageBox(hwndProgman, IDS_GROUPFILEERR, IDS_BADFILE,
                   (LPTSTR) PTR(lpgd, lpgd->pName),
                   MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
      return;
  }


  /* note, must set lpid after each because it may move
   */
  DeleteThing(lpgd, (LPDWORD)&lpid->pName, 0);
  DeleteThing(lpgd, (LPDWORD)&lpid->pCommand, 0);
  DeleteThing(lpgd, (LPDWORD)&lpid->pIconPath, 0);
  DeleteThing(lpgd, (LPDWORD)&lpid->pIconRes, lpid->cbIconRes);
  DeleteThing(lpgd, (LPDWORD)&lpgd->rgiItems[pItem->iItem], sizeof(ITEMDEF));

  while (lptag = FindTag(lpgd,pItem->iItem,0)) {
      /* delete all tags associated with this item
       */
      UnlockGroup(pGroup->hwnd);
      DeleteTag(pGroup->hGroup, lptag->wItem, lptag->wID);
      lpgd = LockGroup(pGroup->hwnd);
  }

  /* Don't need Item anymore so delete it. */
  RemoveItemFromList(pGroup, pItem);

  cb = SizeofGroup(lpgd);

  UnlockGroup(pGroup->hwnd);

  CheckBeforeReAlloc(pGroup->hGroup);
  GlobalReAlloc(pGroup->hGroup, cb, GMEM_MOVEABLE);

  if (bAutoArrange && !bAutoArranging)
      ArrangeItems(pGroup->hwnd);
  else if (!bAutoArranging)
      CalcGroupScrolls(pGroup->hwnd);

}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CreateItemIcons() -                                                     */
/*                                                                          */
/* Creates all the item windows...                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID PASCAL CreateItemIcons(HWND hwndGroup)
{
    LPGROUPDEF lpgd;
    int        i;

    lpgd = LockGroup(hwndGroup);

    if (!lpgd) {
        return;
    }

    /*
     * Create the items in reverse Z-Order.
     */
    for (i = lpgd->cItems - 1; i >= 0; i--) {
        if (lpgd->rgiItems[i]) {
            LoadItem(hwndGroup, (WORD)i, TRUE);
        }
    }

    UnlockGroup(hwndGroup);

  // REVIEW This may be not be needed because LoadGroupWindow does a
  // SetInternalWindowPos which MIGHT already be generating the messages
  // to do this.
  if (bAutoArrange && !bAutoArranging)
      ArrangeItems(hwndGroup);
  else if (!bAutoArranging)
      CalcGroupScrolls(hwndGroup);
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CheckIconResolution() -                                                 */
/*                                                                          */
/* Makes sure we have the right icons loaded... reextracts and saves        */
/* the file if not.                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL CheckIconResolution(HWND hwndGroup)

{
  LPGROUPDEF        lpgd;
  LPITEMDEF         lpid;
  register PGROUP   pGroup;
  HANDLE            hGroup;
  BOOL              fGottaDoIt;
  register HDC      hdc;
  int               i;
  HICON             hIcon;
  WORD              cbIconRes;
  DWORD             pIconRes;
  LPTSTR            lpIconRes;
  WORD              wFormat;
  TCHAR             szTemp[MAXITEMPATHLEN];
  HANDLE            hModule;
  BOOL              fWin32App;
  //DWORD             dwVer;
  WORD              wVer;

  lpgd = LockGroup(hwndGroup);
  if (!lpgd)
      return;

  hdc = GetDC(hwndGroup);

  wFormat = (WORD)GetDeviceCaps(hdc, BITSPIXEL) |
        (WORD)GetDeviceCaps(hdc, PLANES) * (WORD)256;

  ReleaseDC(hwndGroup,hdc);

  fGottaDoIt = lpgd->wIconFormat != wFormat ||
               lpgd->cxIcon != (WORD)GetSystemMetrics(SM_CXICON) ||
               lpgd->cyIcon != (WORD)GetSystemMetrics(SM_CYICON);

  if (!fGottaDoIt) {
      goto CleanUpAndLeave;
  }

  pGroup = (PGROUP)GetWindowLongPtr(hwndGroup,GWLP_PGROUP);

  NukeIconBitmap(pGroup);

  /* Save the new resolution parameters in the group file. */
  lpgd->wIconFormat = wFormat;
  lpgd->cxIcon = (WORD)GetSystemMetrics(SM_CXICON);
  lpgd->cyIcon = (WORD)GetSystemMetrics(SM_CYICON);

  hGroup = pGroup->hGroup;

  for (i = 0; i < (int)lpgd->cItems; ++i) {
      if (!lpgd->rgiItems[i])
          continue;

      lpid = ITEM(lpgd, i);
      DeleteThing(lpgd, (LPDWORD)&lpid->pIconRes, lpid->cbIconRes);
      lpid = ITEM(lpgd, i);

      lstrcpy(szTemp, (LPTSTR) PTR(lpgd, lpid->pIconPath));
      if (!*szTemp) {
          /* Get default icon path */
          lstrcpy(szTemp, (LPTSTR) PTR(lpgd, lpid->pCommand));
          DoEnvironmentSubst(szTemp, (WORD)(MAXITEMPATHLEN+1));
          StripArgs(szTemp);
      }
      SheRemoveQuotes(szTemp);
    cbIconRes = 0xFFFF;
    lpIconRes = NULL;
    hIcon = NULL;

    if (hModule = LoadLibrary(szTemp)) {
        // if WIN32 app
        fWin32App = TRUE;
        hIcon = (HICON)FindResource(hModule, (LPTSTR) MAKEINTRESOURCE(lpid->iIcon), (LPTSTR) MAKEINTRESOURCE(RT_ICON));
        if (hIcon) {
            //dwVer = 0x00030000;
            wVer = 3;
            cbIconRes = (WORD)SizeofResource(hModule, (HRSRC)hIcon);
            hIcon = (HICON)LoadResource(hModule, (HRSRC)hIcon);
            lpIconRes = LockResource(hIcon);
        }
    }
    else { // Win 3.1 app
        fWin32App = FALSE;
        if (wVer = ExtractIconResInfo(hAppInstance, szTemp, lpid->iIcon, &cbIconRes, (LPHANDLE)&hIcon)){
            lpIconRes = GlobalLock(hIcon);
        }
    }


      UnlockGroup(hwndGroup);

      pIconRes = AddThing(pGroup->hGroup, lpIconRes, cbIconRes);
      lpgd = LockGroup(hwndGroup);
      if (!lpgd)
          continue;

      /* In case the segment got moved... */
      lpid = ITEM(lpgd, i);

      if (hIcon)
      if (fWin32App) {
          UnlockResource(hIcon);
          FreeResource(hIcon);
          FreeLibrary(hModule);
      }
      else {
          GlobalUnlock(hIcon);
          GlobalFree(hIcon);
      }

      lpid->pIconRes = pIconRes;
      //lpid->dwIconVer = dwVer;
      lpid->wIconVer = wVer;
      lpid->cbIconRes = cbIconRes;
    }

#ifdef ORGCODE
      // Check everythings OK.
      if (!pHdr || !pAND || !pXOR)
        {
          // FU - delete icon stuff for this item..
          // REVIEW UNDONE - warn user about memory problem  ?

#ifdef DEBUG
          KdPrint(("PM.CIR: Corrupted icon %s \n\r", (LPTSTR) PTR(lpid->pName)));
#endif
          lpid = ITEM(lpgd, i);
          DeleteThing(lpgd, (LPDWORD)&lpid->pIconRes, lpid->cbIconRes);

          // Mark item as being effed - the header is checked by
          // GetItemIcon.
          lpid = ITEM(lpgd, i);
          lpid->pIconRes = NULL;
          lpid->wIconVer = 0;           // This is the important one.
          lpid->cbIconRes = 0;

          // Warn user when we're through, not right in the middle.
          fErrorOnExtract = TRUE;
        }
    }
#endif

  if (!GroupCheck(pGroup))
      MyMessageBox(hwndProgman, IDS_GROUPFILEERR, IDS_EEGROUPRO,
                   (LPTSTR) PTR(lpgd, lpgd->pName),
                   MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);


CleanUpAndLeave:
  UnlockGroup(hwndGroup);

//REVIEW See above.
  KeepGroupAround(hwndGroup, FALSE);
  return;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CreateGroupHandle() -                                                   */
/*                                                                          */
/* Creates a discarded handle for use as a group handle... on the first     */
/* LockGroup() the file will be loaded.                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

HANDLE NEAR PASCAL CreateGroupHandle(void)
{
  register HANDLE   hGroup;

  if (hGroup = GlobalAlloc(GMEM_MOVEABLE | GMEM_DISCARDABLE, 1L))
      GlobalDiscard(hGroup);

  return(hGroup);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  StartupGroup() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID FAR PASCAL StartupGroup(HWND hwnd)
{
  PITEM pItemCur, pItemExec;
  LPGROUPDEF lpgd;
  INT xLast, yLast;    // Coord of icon to exec.
  INT xBest, yBest;    // Coord of next topmost-leftmost icon.
  MSG msg;             // Peek a message.
  PGROUP pGroup;       // The group.

  pGroup = (PGROUP)GetWindowLongPtr(hwnd, GWLP_PGROUP);

  /*
   * Handle Startup group in icon position order - not Z-order.
   */
  lpgd = LockGroup(hwnd);
  if (!lpgd)
      return;

  /*
   * Starts with the top left and works from left to right then
   * top to bottom.
   * This is really naff in terms of speed, but groups are usually
   * small and the time cost is still small compared to that of execing.
   */
  yLast = WORD_MIN;
  xLast = WORD_MIN;
  for (;;) {
      /*
       * Init
       */
      xBest = WORD_MAX;
      yBest = WORD_MAX;
      pItemExec = NULL;
      /*
       * Find next icon to the right of this one.
       */
      for (pItemCur = pGroup->pItems; pItemCur; pItemCur = pItemCur->pNext) {
          /*
           * Look for Icon to the right of this one.
           * REVIEW This will ignore icons stacked on top of each other.
           */
          if (pItemCur->rcIcon.top >= yLast
            && pItemCur->rcIcon.top <= yLast + (cyArrange/2)
            && pItemCur->rcIcon.left < xBest
            && pItemCur->rcIcon.left > xLast) {
              pItemExec = pItemCur;
              xBest = pItemCur->rcIcon.left;
          }
          /*
           * Check if it'll be suitable for the next row.
           */
          else if (pItemCur->rcIcon.top > yLast + (cyArrange/2)
            && pItemCur->rcIcon.top < yBest) {
              yBest = pItemCur->rcIcon.top;
          }

      }

      if (pItemExec) {
          /*
           * Found one on the current row.
           */

          xLast = xBest;
          /*
           * Move this item to the top of the z-order so that any searches
           * done during DDE will find the last execed item first.
           * REVIEW This messes with the z-odrder of the startup group.
           */
          BringItemToTop(pGroup, pItemExec, TRUE);
          /* Start it up. */
          ExecItem(pGroup,pItemExec,FALSE, TRUE);
          /*
           * Handle any DDE before doing anything else to stop
           * the message queue from over-flowing.
           */
          while(PeekMessage(&msg, hwndProgman, 0, 0, PM_REMOVE|PM_NOYIELD)) {
              TranslateMessage(&msg);
              DispatchMessage(&msg);
          }


      }
      else if (yBest != WORD_MAX) {
          /*
           * Nothing left on the current row but there is another row.
           */
          yLast = yBest;
          xLast = WORD_MIN;

      }
      else {
          /*
           * Nothing left.
           */
          goto Quit;
      }
  }

Quit:
  UnlockGroup(pGroup->hwnd);
}


/*---------------------------------------------------------------------------
 * Check for null item pointers and item pointers that go out of the group.
 * REVIEW UNDONE this doesn't check that the pointers point to things
 * after the end of the items array.
 */
BOOL NEAR PASCAL ValidItems(LPGROUPDEF lpgd)
{
    INT i;
    LPITEMDEF lpid;
    DWORD cbGroup;

    if (!lpgd)
        return FALSE;


    cbGroup = lpgd->cbGroup;

    for (i = 0; (WORD)i < lpgd->cItems; i++) {
        if (!lpgd->rgiItems[i])
	        continue;

        lpid = ITEM(lpgd,i);
        if (!lpid)
            return FALSE;

        if (lpid->pName > cbGroup)
            return FALSE;
        if (lpid->pCommand > cbGroup)
            return FALSE;
        if (lpid->pIconPath > cbGroup)
            return FALSE;
        if ((lpid->cbIconRes != (WORD)-1) && (lpid->pIconRes > cbGroup))
            return FALSE;

    }

    return(TRUE);
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  IsGroupAlreadyLoaded() -                                                */
/*                                                                          */
/* Determines if the user is trying to load a currently loaded group.       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

HWND NEAR PASCAL IsGroupAlreadyLoaded(LPTSTR lpGroupKey, BOOL bCommonGroup)
{
  HWND     hwndT;
  PGROUP   pGroup;

  for (hwndT=GetWindow(hwndMDIClient, GW_CHILD); hwndT; hwndT=GetWindow(hwndT, GW_HWNDNEXT)) {
      if (GetWindow(hwndT, GW_OWNER))
          continue;

      pGroup = (PGROUP)GetWindowLongPtr(hwndT, GWLP_PGROUP);
      if (!lstrcmpi(lpGroupKey, pGroup->lpKey)) {

          if (bCommonGroup) {

              if (pGroup->fCommon)
                  return(hwndT);

          } else {

              if (!pGroup->fCommon)
                  return(hwndT);

          }
      }
  }
  return(NULL);
}


BOOL NEAR PASCAL IndexUsed(WORD wIndex, BOOL bCommonGroup)
{
    PGROUP pGroup;

    for (pGroup = pFirstGroup; pGroup; pGroup = pGroup->pNext)
        if ((pGroup->wIndex == wIndex) && (pGroup->fCommon == bCommonGroup))
            return TRUE;

    return FALSE;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  LoadGroupWindow() -                                                     */
/*                                                                          */
/* Creates a group window by sending an MDI create message to the MDI client. */
/* The MDICREATESTRUCT contains a parameter pointer to the name of the group  */
/* key.                                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/
// GroupFile must be ANSI.

HWND PASCAL LoadGroupWindow(LPTSTR lpKey, WORD wIndex, BOOL bCommonGroup)
{
    MDICREATESTRUCT mdics;
    PGROUP          pGroup;
    TCHAR           szGroupClass[64];
    LPGROUPDEF      lpgd;
    HWND            hwnd;
    TCHAR           szCommonGroupSuffix[MAXKEYLEN];
    TCHAR           szCommonGroupTitle[2*MAXKEYLEN];
    WINDOWPLACEMENT wp;

    //
    // Check if the group is already loaded. This will prevent duplicate groups.
    //
    if (hwnd = IsGroupAlreadyLoaded(lpKey, bCommonGroup)) {
        return(hwnd);
    }

    if (!wIndex) {
        while (IndexUsed(++wIndex, bCommonGroup))
                ;
    }

    if (!LoadString(hAppInstance, IDS_GROUPCLASS, szGroupClass,
            CharSizeOf(szGroupClass))) {
        return NULL;
    }

    pGroup = (PGROUP)LocalAlloc(LPTR,sizeof(GROUP));
    if (!pGroup) {
        return NULL;
    }

    pGroup->hGroup = CreateGroupHandle();
    pGroup->pItems = NULL;
    pGroup->hbm    = NULL;
    pGroup->wIndex = wIndex;
    pGroup->fCommon = bCommonGroup;
    pGroup->ftLastWriteTime.dwLowDateTime = 0;
    pGroup->ftLastWriteTime.dwHighDateTime = 0;

    pGroup->lpKey = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(lpKey) + 1));
    pGroup->fLoaded = FALSE;

    if (!pGroup->lpKey) {
GoAway:
        GlobalFree(pGroup->hGroup);
        LocalFree((HANDLE)pGroup);
        if (!fLowMemErrYet) {
            MyMessageBox(hwndProgman, IDS_APPTITLE, IDS_LOWMEMONINIT,
                    lpKey, MB_OK|MB_ICONEXCLAMATION);
            fLowMemErrYet = TRUE;
        }
        return NULL;
    }

    lstrcpy(pGroup->lpKey, lpKey);

    mdics.szTitle = TEXT("");
    mdics.hOwner = hAppInstance;
    mdics.szClass = szGroupClass;
    mdics.style = WS_VSCROLL|WS_HSCROLL;
    mdics.x = mdics.y = mdics.cx = mdics.cy = CW_USEDEFAULT;
    mdics.lParam = (LPARAM)pGroup;

    /*
     * REVIEW HACK - Set the auto arranging flag to stop the group being
     * loaded by ArrangingIcons doing a LockGroup  and then producing
     * an error if something goes wrong. We're going to do a lock
     * later on anyway and we don't want two error messages.
     */
    bAutoArranging = TRUE;
    pGroup->hwnd = (HWND)SendMessage(hwndMDIClient, WM_MDICREATE, 0, (LPARAM)(LPTSTR)&mdics);
    bAutoArranging = FALSE;

    if (!pGroup->hwnd) {
        LocalFree((HANDLE)pGroup->lpKey);
        goto GoAway;
    }

    /*
     * Note that we're about to load a group for the first time.
     * NB Stting this tells LockGroup that the caller can handle the errors.
     */
    fFirstLoad = TRUE;
    lpgd = LockGroup(pGroup->hwnd);
    /*
     * The group has been loaded or at least we tried.
     */
    fFirstLoad = FALSE;
    if (!lpgd) {
LoadFail:
        /* Loading the group failed somehow... */
        SendMessage(hwndMDIClient, WM_MDIDESTROY, (WPARAM)pGroup->hwnd, 0L);
        //
        // stop handling of Program Groups key changes.
        //
        bHandleProgramGroupsEvent = FALSE;
        RegDeleteKey(hkeyProgramGroups, pGroup->lpKey);

        //
        // reset handling of Program Groups key changes.
        //
        ResetProgramGroupsEvent(bCommonGroup);
        bHandleProgramGroupsEvent = TRUE;

        LocalFree((HANDLE)pGroup->lpKey);
        GlobalFree(pGroup->hGroup);
        LocalFree((HANDLE)pGroup);
        return NULL;
    }

    /*
     * test if it is a Windows 3.1 group file format. If so it is not
     * valid in WIN32. In Windows 3.1 RECT and POINT are WORD instead of LONG.
     */

    if ( (lpgd->rcNormal.left != (INT)(SHORT)lpgd->rcNormal.left) ||
         (lpgd->rcNormal.right != (INT)(SHORT)lpgd->rcNormal.right) ||
         (lpgd->rcNormal.top != (INT)(SHORT)lpgd->rcNormal.top) ||
         (lpgd->rcNormal.bottom != (INT)(SHORT)lpgd->rcNormal.bottom) ){
        /* The group is invalid. */
        MyMessageBox(hwndProgman, IDS_GROUPFILEERR, IDS_BADFILE,
                                (LPTSTR) PTR(lpgd, lpgd->pName),
                                MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
        UnlockGroup(pGroup->hwnd);
        goto LoadFail;
    }

    if (!ValidItems(lpgd)) {
        MyMessageBox(hwndProgman, IDS_GROUPFILEERR, IDS_BADFILE,
                                (LPTSTR) PTR(lpgd, lpgd->pName),
                                MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
        UnlockGroup(pGroup->hwnd);
        goto LoadFail;
    }


    if (lpgd->nCmdShow) {
        SetInternalWindowPos(pGroup->hwnd, (UINT)lpgd->nCmdShow, &lpgd->rcNormal,
                &lpgd->ptMin);
    }

    if (pGroup->fCommon) {

        //
        // Add the common group suffix to the name of the group e.g. (Common)
        // Only do this if the group window is not minimized.
        //

        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(pGroup->hwnd, &wp);
        if ((wp.showCmd == SW_MINIMIZE) ||
            (wp.showCmd == SW_SHOWMINIMIZED) ||
            (wp.showCmd == SW_SHOWMINNOACTIVE) ) {
            SetWindowText(pGroup->hwnd, (LPTSTR) PTR(lpgd, lpgd->pName));
        }
        else {
            lstrcpy(szCommonGroupTitle, (LPTSTR) PTR(lpgd, lpgd->pName));
            if (LoadString(hAppInstance, IDS_COMMONGRPSUFFIX, szCommonGroupSuffix,
                           CharSizeOf(szCommonGroupSuffix))) {
                lstrcat(szCommonGroupTitle, szCommonGroupSuffix);
            }
            SetWindowText(pGroup->hwnd, szCommonGroupTitle);
            if (!UserIsAdmin) {
                pGroup->fRO = TRUE;
            }
        }
    }
    else {
        SetWindowText(pGroup->hwnd, (LPTSTR) PTR(lpgd, lpgd->pName));
    }


    UnlockGroup(pGroup->hwnd);

    //CheckIconResolution(pGroup->hwnd);

    CreateItemIcons(pGroup->hwnd);

    /*
     * Link the group.
     */
    pGroup->pNext = NULL;
    *pLastGroup = pGroup;
    pLastGroup = &pGroup->pNext;
    pCurrentGroup = pGroup;

#ifdef NOTINUSER
    CalcChildScroll(pGroup->hwnd, SB_BOTH);
#endif

    GroupCheck(pGroup);

    return(pGroup->hwnd);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  UnloadGroupWindow() -                                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void FAR PASCAL UnloadGroupWindow(HWND hwnd)
{
    PGROUP pGroup, *ppGroup;
    PITEM pItem, pItemNext;

    pGroup = (PGROUP)GetWindowLongPtr(hwnd,GWLP_PGROUP);

    /* Destroy the window. */
    SendMessage(hwndMDIClient,WM_MDIDESTROY,(WPARAM)hwnd,0L);

    /* Free the group segment. */
    GlobalFree(pGroup->hGroup);

    /* Free the local stuff. */
    LocalFree((HANDLE)pGroup->lpKey);

    /* The cached bitmap if there is one. */
    if (pGroup->hbm) {
        DeleteObject(pGroup->hbm);
        pGroup->hbm = NULL;
    }

    /* The item data. */
    for (pItem = pGroup->pItems; pItem; pItem = pItemNext) {
        pItemNext = pItem->pNext;
        LocalFree((HANDLE) pItem);
    }

    /* Remove the group from the linked list. */
    for (ppGroup = &pFirstGroup; *ppGroup; ppGroup = &((*ppGroup)->pNext)) {
        if (*ppGroup == pGroup) {
            *ppGroup = pGroup->pNext;
            break;
        }
    }
    if (pLastGroup == &pGroup->pNext)
	pLastGroup = ppGroup;

    /* Lastly, free the group structure itself. */
    LocalFree((HANDLE)pGroup);
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

void RemoveBackslashFromKeyName(LPTSTR lpKeyName)
{
    LPTSTR lpt;

    for (lpt = lpKeyName; *lpt; lpt++) {
        if ((*lpt == TEXT('\\')) || (*lpt == TEXT(':')) || (*lpt == TEXT('>')) || (*lpt == TEXT('<')) ||
             (*lpt == TEXT('*')) || (*lpt == TEXT('?')) ){
            *lpt = TEXT('.');
        }
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CreateNewGroup() -                                                      */
/*                                                                          */
/*  This function creates a new, empty group.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

HWND PASCAL CreateNewGroup(LPTSTR pGroupName, BOOL bCommonGroup)
{
    HANDLE      hT;
    LPGROUPDEF  lpgd;
    PGROUP      pGroup;
    HDC         hdc;
    int         i;
    int         cb;
    TCHAR       szKeyName[MAXKEYLEN+1];
    HWND        hwnd;
    DWORD       status = 0;
    WORD        cGroups;
    INT         wGroupNameLen;   //length of pGroupName DWORD aligned.
    HKEY        hkeyGroups;
    HKEY        hKey;
    HWND        hwndT;
    PSECURITY_ATTRIBUTES pSecAttr;

    /*
     * Check we're not trying to create too many groups.
     * Count the current number of groups.
     */
    cGroups = 0;

    for (hwnd = GetWindow(hwndMDIClient, GW_CHILD); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
        if (GetWindow(hwnd, GW_OWNER))
            continue;
        //
        // count the common groups seperately from the personal group.
        // Both have a maximum of CGROUPSMAX groups.
        //
        pGroup = (PGROUP)GetWindowLongPtr(hwnd, GWLP_PGROUP);
        if (bCommonGroup && pGroup->fCommon ||
                             !bCommonGroup && !pGroup->fCommon) {
            cGroups++;
        }
    }

    // Compare with limit.
    if (cGroups >= CGROUPSMAX) {
        status = bCommonGroup ? IDS_TOOMANYCOMMONGROUPS : IDS_TOOMANYGROUPS;
        goto Exit;
    }

    if (bCommonGroup) {

        hkeyGroups = hkeyCommonGroups;
        pSecAttr = pAdminSecAttr;
        if (!hkeyGroups) {
            if (MyMessageBox(hwndProgman,
                             IDS_COMMONGROUPERR,
                             IDS_NOCOMMONGRPS,
                             pGroupName,
                             MB_OKCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL)
                       == IDOK) {

                hkeyGroups = hkeyProgramGroups;
                pSecAttr = pSecurityAttributes;
                bCommonGroup = FALSE;

            } else {
                return(NULL);
            }
        }

    } else {

        hkeyGroups = hkeyProgramGroups;
        pSecAttr = pSecurityAttributes;

    }

    if (!hkeyGroups) {
        status = IDS_NOGRPFILE;
        goto Exit;
    }

    //
    // Replace backslash in the group name because the registry does not
    // allow key names with backslash, bckslash is used to separate keys.
    //
    lstrcpy(szKeyName, pGroupName);
    RemoveBackslashFromKeyName(szKeyName);

    //
    // Test for existing key.
    //
    while (!RegOpenKeyEx(hkeyGroups, szKeyName, 0, KEY_READ, &hKey)) {
        /* a group with this name already exists */
        if (hwndT = IsGroupAlreadyLoaded(szKeyName, bCommonGroup)) {
            if (lstrlen(szKeyName) < MAXKEYLEN) {
                lstrcat(szKeyName, TEXT("."));
                GlobalUnlock(pGroup->hGroup);
                RegCloseKey(hKey);
                continue;
            }
        }
        RegCloseKey(hKey);
        goto LoadGroupFile;
    }

    wGroupNameLen = MyDwordAlign(sizeof(TCHAR)*(lstrlen(pGroupName) + 1));
    cb = sizeof(GROUPDEF) + (NSLOTS * sizeof(DWORD)) +  wGroupNameLen;

    /*
     * In CreateNewGroup before GlobalAlloc.
     */
    hT = GlobalAlloc(GHND, (DWORD)cb);
    if (!hT) {
        status = IDS_LOWMEM;
        goto Exit;
    }

    lpgd = (LPGROUPDEF)GlobalLock(hT);

    lpgd->dwMagic = GROUP_UNICODE;
    lpgd->cbGroup = (DWORD)cb;
    lpgd->nCmdShow = 0;            /* use MDI defaults  */
    lpgd->pName = sizeof(GROUPDEF) + NSLOTS * sizeof(DWORD);
    hdc = GetDC(NULL);
    lpgd->wIconFormat = (WORD)GetDeviceCaps(hdc, BITSPIXEL) + (WORD)256 *
            (WORD)GetDeviceCaps(hdc, PLANES);
    ReleaseDC(NULL, hdc);
    lpgd->cxIcon = (WORD)GetSystemMetrics(SM_CXICON);
    lpgd->cyIcon = (WORD)GetSystemMetrics(SM_CYICON);
    lpgd->Reserved1 = (WORD)-1;
    lpgd->Reserved2 = (DWORD)-1;

    lpgd->cItems = NSLOTS;

    for (i = 0; i < NSLOTS; i++) {
        lpgd->rgiItems[i] = 0;
    }

    lstrcpy((LPTSTR)((LPSTR)lpgd + sizeof(GROUPDEF) + NSLOTS * sizeof(DWORD)),
            pGroupName);

    /*
     * In CreateNewGroup before SizeofGroup.
     */
    cb = (int)SizeofGroup(lpgd);


    //
    // stop handling of Program Groups key changes when creating groups.
    //
    bHandleProgramGroupsEvent = FALSE;

    //
    // BUGBUG pSecurityAttributes might change for Common groups.
    //

    if (!RegCreateKeyEx(hkeyGroups, szKeyName, 0, 0, 0,
                     DELETE | KEY_READ | KEY_WRITE,
                     pSecAttr, &hKey, NULL)) {
        if (RegSetValueEx(hKey, NULL, 0, REG_BINARY, (LPBYTE)lpgd, cb))
            status = IDS_CANTWRITEGRP;
        RegCloseKey(hKey);
    }
    else
        status = IDS_NOGRPFILE;

    GlobalUnlock(hT);
    GlobalFree(hT);

Exit:
    if (status) {
        MyMessageBox(hwndProgman, IDS_GROUPFILEERR, (WORD)status, pGroupName,
                MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
        ResetProgramGroupsEvent(bCommonGroup);
	bHandleProgramGroupsEvent = TRUE;
        return NULL;
    }

LoadGroupFile:
    /*
     * The group file now exists on the disk... load it in!
     */
    fLowMemErrYet = FALSE;
    fErrorOnExtract = FALSE;
    hwnd = LoadGroupWindow(szKeyName, 0, bCommonGroup);

    if (fErrorOnExtract) {
        // On observed problem with icon extraction has been to do
        // with a low memory.
        MyMessageBox(hwndProgman, IDS_OOMEXITTITLE, IDS_LOWMEMONEXTRACT,
            NULL, MB_OK|MB_ICONHAND|MB_SYSTEMMODAL);
    }

    // Save the group section even if SaveSettings is off to stop
    // stupid users from hosing themselves.
    if (!bCommonGroup) {
        WriteGroupsSection();
    }
    ResetProgramGroupsEvent(bCommonGroup);
    bHandleProgramGroupsEvent = TRUE;
    return hwnd;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DeleteGroup() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID FAR PASCAL DeleteGroup(HWND hwndGroup)
{
  PGROUP pGroup = (PGROUP)GetWindowLongPtr(hwndGroup, GWLP_PGROUP);
  PGROUP *ppGroup;
  PITEM  pItem;
  TCHAR   szT[10];
  BOOL   bCommonGroup;
  HKEY   hkeyGroups;

  //
  // stop handling of Program Groups key changes when deleting groups.
  //
  bHandleProgramGroupsEvent = FALSE;

  bCommonGroup = pGroup->fCommon;
  if (bCommonGroup)
      hkeyGroups = hkeyCommonGroups;
  else
      hkeyGroups = hkeyProgramGroups;

  if (pGroup->fRO || RegDeleteKey(hkeyGroups, pGroup->lpKey) != ERROR_SUCCESS) {
      MyMessageBox(hwndProgman, IDS_GROUPFILEERR, IDS_ERRORDELETEGROUP,
                                                      pGroup->lpKey, MB_OK);
      //
      // reset handling of Program Groups key changes.
      //
      ResetProgramGroupsEvent(bCommonGroup);
      bHandleProgramGroupsEvent = TRUE;
      return;   // cannot delete the group
  }

  //
  // reset handling of Program Groups key changes.
  //
  ResetProgramGroupsEvent(bCommonGroup);
  bHandleProgramGroupsEvent = TRUE;

  /* Destroy the window, the global memory block, and the file. */
  SendMessage(hwndMDIClient, WM_MDIDESTROY, (WPARAM)hwndGroup, 0L);
  NukeIconBitmap(pGroup);
  GlobalFree(pGroup->hGroup);

  if (!bCommonGroup) {

      //
      // Remove the program manager's settings for that personal group.
      //

      wsprintf(szT,TEXT("Group%d"),pGroup->wIndex);
      RegDeleteValue(hkeyPMGroups, szT);
  }

  /* Unlink the group structure. */
  for (ppGroup=&pFirstGroup; *ppGroup && *ppGroup != pGroup; ppGroup = &(*ppGroup)->pNext)
      ;

  if (*ppGroup)
      *ppGroup = pGroup->pNext;

  if (pLastGroup == &pGroup->pNext)
      pLastGroup = ppGroup;

  /* Destroying the window should activate another one, but if it is the
   * last one, nothing will get activated, so to make sure punt the
   * current group pointer...
   */
  if (pCurrentGroup == pGroup)
      pCurrentGroup = NULL;

  /* Lastly, toss out the group and item structures. */
  while (pGroup->pItems) {
      pItem = pGroup->pItems;
      pGroup->pItems = pItem->pNext;
      LocalFree((HANDLE)pItem);
  }
  LocalFree((HANDLE)pGroup->lpKey);
  LocalFree((HANDLE)pGroup);

  if (!bCommonGroup) {

      //
      // Change the program manager's settings for that personal group.
      //

      WriteGroupsSection();
  }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ChangeGroupTitle() -                                                    */
/*                                                                          */
/*  Modifies the name of a program group, on the screen and in the file.    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID FAR PASCAL ChangeGroupTitle(HWND hwndGroup, LPTSTR lpName, BOOL bCommonGroup)
{
  LPGROUPDEF lpgd;
  PGROUP pGroup;
  DWORD pName;
  TCHAR szCommonGroupSuffix[MAXKEYLEN];
  TCHAR szCommonGroupTitle[2*MAXKEYLEN];
  WINDOWPLACEMENT wp;

  if (!hwndGroup)
      return;

  //
  // Change the title of the window.
  //

  if (bCommonGroup) {

      //
      // Add the common group suffix to the name of the group e.g. (Common),
      // do not append the common suffix if the group window is minimized.
      //
      wp.length = sizeof(WINDOWPLACEMENT);
      GetWindowPlacement(hwndGroup, &wp);
      if (wp.showCmd == SW_MINIMIZE || wp.showCmd == SW_SHOWMINIMIZED ||
          wp.showCmd == SW_SHOWMINNOACTIVE) {
          SetWindowText(hwndGroup, lpName);
      }
      else {

          lstrcpy(szCommonGroupTitle, lpName);
          if (LoadString(hAppInstance, IDS_COMMONGRPSUFFIX, szCommonGroupSuffix,
                         CharSizeOf(szCommonGroupSuffix))) {
              lstrcat(szCommonGroupTitle, szCommonGroupSuffix);
          }
          SetWindowText(hwndGroup, szCommonGroupTitle);
      }
  }
  else {
      SetWindowText(hwndGroup, lpName);
  }

  //
  // Remove the old name.
  //

  lpgd = LockGroup(hwndGroup);
  if (!lpgd)
      return;

  DeleteThing(lpgd, (LPDWORD)&lpgd->pName, 0);
  UnlockGroup(hwndGroup);

  //
  // Insert the new one.
  //

  pGroup = (PGROUP)GetWindowLongPtr(hwndGroup,GWLP_PGROUP);
  pName = AddThing(pGroup->hGroup, lpName ,(WORD)0);

  //
  // Set the new offset...
  //

  if (lpgd = LockGroup(hwndGroup)) {
      lpgd->pName = pName;
      UnlockGroup(hwndGroup);
  }

}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SetGroupDimensions() -                                                  */
/*                                                                          */
/*  Saves the size and position of the group file in the group segment, as  */
/*  as well as the positions of all the item icons                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID NEAR PASCAL SetGroupDimensions(HWND hwndGroup)
{
  LPGROUPDEF        lpgd;
  LPITEMDEF        lpid;
  PGROUP pGroup;
  PITEM pItem;
  WORD i;

  lpgd = LockGroup(hwndGroup);
  if (!lpgd)
      return;

  lpgd->nCmdShow = (WORD)GetInternalWindowPos(hwndGroup, &lpgd->rcNormal,
            &lpgd->ptMin);

  pGroup = (PGROUP)GetWindowLongPtr(hwndGroup,GWLP_PGROUP);
  NukeIconBitmap(pGroup);	// invalidate the bitmap

  for (pItem=pGroup->pItems; pItem; pItem=pItem->pNext) {
      lpid = ITEM(lpgd,pItem->iItem);
      lpid->pt.x = pItem->rcIcon.left;
      lpid->pt.y = pItem->rcIcon.top;

      /* save offset of ITEMDEF for each item
       */
      ChangeTagID(lpgd,pItem->iItem,(int)lpgd->rgiItems[pItem->iItem]);
      pItem->iItem = (int)lpgd->rgiItems[pItem->iItem];
  }

  for (i=0, pItem=pGroup->pItems; pItem; pItem=pItem->pNext, i++) {
      /* write offsets back out in Z order and update the index
       */
      ChangeTagID(lpgd,pItem->iItem,(int)i);
      lpgd->rgiItems[i] = (DWORD)pItem->iItem;
      pItem->iItem = (int)i;
  }

  /* Clear out remaining pointers to prevent duped item wierdness. */
  while (i < lpgd->cItems)
      lpgd->rgiItems[i++] = 0;

  UnlockGroup(hwndGroup);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  WriteGroupsSection() -                                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void PASCAL WriteGroupsSection(VOID)
{
    PGROUP      pGroup;
    HCURSOR     hCursor;
    HWND        hwndGroup;
    LPGROUPDEF  lpgd;
    TCHAR        szT[66];
    TCHAR        szOrd[CGROUPSMAX*8+7];
    TCHAR szFmt[] = TEXT("Group%d");
    TCHAR szFmtCommonGrp[] = TEXT("GroupC%d");
    INT cGroups;
    TCHAR szGroupKey[MAXKEYLEN];
    INT i;
    RECT rc;
    POINT ptMin;
    TCHAR szCGrpInfo[MAXKEYLEN];
    INT cbValueName;

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ShowCursor(TRUE);

    if (!(hwndGroup = GetWindow(hwndMDIClient, GW_CHILD))) {
        goto WPIExit;
    }

    hwndGroup = GetWindow(hwndGroup, GW_HWNDLAST);

    szOrd[0] = 0;
    cGroups = 0;

    //
    // Clear user's previous positioning of common groups.
    //
    if (hkeyPMCommonGroups) {
        cbValueName = CharSizeOf(szGroupKey);
        while (!RegEnumValue(hkeyPMCommonGroups, 0, szGroupKey, &cbValueName, 0, 0,
                             0, 0)) {
            RegDeleteValue(hkeyPMCommonGroups, szGroupKey);
            cbValueName = CharSizeOf(szGroupKey);
        }
    }

    for (; hwndGroup; hwndGroup = GetWindow(hwndGroup, GW_HWNDPREV)) {
        /*
         * Check to make sure we're not out of room for the order string.
         */
        if (cGroups > CGROUPSMAX) {
            MessageBeep(0);
            break;
        }

        if (GetWindow(hwndGroup, GW_OWNER)) {
            continue;
        }

        pGroup = (PGROUP)GetWindowLongPtr(hwndGroup, GWLP_PGROUP);

        if (!pGroup->lpKey || !*pGroup->lpKey) {
            if (pGroup->lpKey) {
                LocalFree((HANDLE)pGroup->lpKey);
            }

            lpgd = LockGroup(hwndGroup);
            if (!lpgd) {
                continue;
            }

            lstrcpy(szGroupKey, (LPTSTR) PTR(lpgd, lpgd->pName));
            pGroup->lpKey = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(szGroupKey) + 1));
            lstrcpy(pGroup->lpKey, szGroupKey);
            UnlockGroup(hwndGroup);
        }

        if (pGroup->fCommon) {
            wsprintf(szT, szFmtCommonGrp, pGroup->wIndex);
            lstrcat(szOrd, TEXT(" C"));
            lstrcat(szOrd, szT + CCHCOMMONGROUP);
            if (hkeyPMCommonGroups) {
                i = GetInternalWindowPos(hwndGroup, &rc, &ptMin);

                if (i==SW_SHOWMINNOACTIVE)
                    i = SW_SHOWNORMAL;

                wsprintf(szCGrpInfo, TEXT("%d %d %d %d %d %d %d "),
                          rc.left, rc.top, rc.right, rc.bottom,
                          ptMin.x, ptMin.y, i);
                lstrcat(szCGrpInfo, pGroup->lpKey);
                RegSetValueEx(hkeyPMCommonGroups, szT, 0, REG_SZ, (LPBYTE)szCGrpInfo, sizeof(TCHAR)*(lstrlen(szCGrpInfo)+1));
            }
        }
        else {
            wsprintf(szT, szFmt, pGroup->wIndex);
            lstrcat(szOrd, TEXT(" "));
            lstrcat(szOrd, szT + CCHGROUP);
            if (hkeyPMGroups) {
                RegSetValueEx(hkeyPMGroups, szT, 0, REG_SZ, (LPBYTE)pGroup->lpKey, sizeof(TCHAR)*(lstrlen(pGroup->lpKey)+1));
            }
        }

        cGroups++;
    }

    if (hkeyPMSettings) {
        RegSetValueEx(hkeyPMSettings, szOrder, 0, REG_SZ, (LPBYTE)szOrd, sizeof(TCHAR)*(lstrlen(szOrd) + 1));
    }

WPIExit:
    ShowCursor(FALSE);
    SetCursor(hCursor);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SaveGroupsContent() -
/*
/*  Save the contents of the all the groups, doesn't save changes in
/*  size or position if bSaveGroupSettings is FALSE, only the group items.
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL SaveGroupsContent(BOOL bSaveGroupSettings)
{
    HWND hwndGroup;

    hwndGroup = GetWindow(hwndMDIClient, GW_CHILD);
    if (!hwndGroup)
        return FALSE;

    for (hwndGroup=GetWindow(hwndGroup, GW_HWNDLAST); hwndGroup; hwndGroup=GetWindow(hwndGroup, GW_HWNDPREV)) {
        if (GetWindow(hwndGroup, GW_OWNER))
            continue;

        /* Save the latest sizes and positions. */
        if (bSaveGroupSettings) {
            SetGroupDimensions(hwndGroup);
            if (wLockError == LOCK_LOWMEM && !fLowMemErrYet) {
                // No more error messages.
                fLowMemErrYet = TRUE;
                wLockError = 0;
                // Warn user that some settings couldn't be saved.
                MyMessageBox(hwndProgman, IDS_OOMEXITTITLE, IDS_LOWMEMONEXIT, NULL,
                     MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
            }
        }

        SaveGroup(hwndGroup, TRUE);
    }

    RegFlushKey(HKEY_CURRENT_USER);

    return(TRUE);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  WriteINIFile() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void FAR PASCAL WriteINIFile()
{
    register int  i;
    RECT          rc;
    TCHAR          szT[40];
    HANDLE        hCursor;

    //
    // Don't save if restricted. But force save if we've just converted the
    // ansi groups to unicode so we can work from unicode the next time around.
    //
    if (fNoSave && !bUseANSIGroups)
        return;

    fLowMemErrYet = FALSE;

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ShowCursor(TRUE);

    i = GetInternalWindowPos(hwndProgman, &rc, NULL);

    if (i==SW_SHOWMINNOACTIVE)
        i = SW_SHOWNORMAL;

    wsprintf(szT, TEXT("%d %d %d %d %d"), rc.left, rc.top, rc.right, rc.bottom, i);
    if (hkeyPMSettings) {
        RegSetValueEx(hkeyPMSettings, szWindow, 0, REG_SZ, (LPBYTE)szT, sizeof(TCHAR)*(lstrlen(szT)+ 1));
    }

    if (!bLoadEvil)
        WriteGroupsSection();
    //
    // Save all groups content, TRUE means we want the size and position saved
    // as well.
    //
    SaveGroupsContent(TRUE);

    ShowCursor(FALSE);
    SetCursor(hCursor);
}




/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetItemCommand() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void FAR PASCAL GetItemCommand(
    PGROUP pGroup,
    PITEM pItem,
    LPTSTR lpCommand,
    LPTSTR lpDir)
{
    BYTE b=0;
    LPTSTR lp1, lp2, lp3;
    LPGROUPDEF lpgd;
    LPITEMDEF lpid;
    BOOL bNoFirstQuote = TRUE;

    if (!GetGroupTag(pGroup,pItem,(WORD)ID_APPLICATIONDIR,lpCommand,MAXITEMPATHLEN)) {
        // the application directory is not defined
        *lpCommand = 0;
    }

    lpgd = LockGroup(pGroup->hwnd);
    if (!lpgd) {
        *lpCommand = 0;
        *lpDir = 0;
        return;
    }
    lpid = ITEM(lpgd,pItem->iItem);

    // init working directory
    lp3 = lpDir;
    *lp3 = 0;

    // item command
    lp1 = (LPTSTR) PTR(lpgd, lpid->pCommand);
    if (*lp1 == TEXT('"')) {
        lp2 = lp1;
        while (lp2 && (lp2 = wcschr(lp2+1, TEXT('"'))) && *(lp2+1) != TEXT('\\')) { //go to next quote
             ;
        }
        if (!lp2) {
            //
            // The directory is not in quotes and since the command path starts
            // with a quote, there's no working directory.
            //
            lp2 = lpDir;
	    *lp2 = 0;
        }
        else {
            if (*(lp2+1) == TEXT('\\')) {
                //
                // the working directory is in quotes
                //
                *lp3++ = *lp1++; //write first quote

                for (; *lp1 && lp1 != lp2; lp1 = CharNext(lp1)) {
                    *lp3++ = *lp1;
                }
                if (*lp1 == TEXT('"')) {
                   *lp3++ = *lp1++; //write last quote
                   lp1++;
                   *lp3 = 0;
                }
            }
            lp2 = lp3 + lstrlen(lp3);
	}
    }
    else {
        //
        // if there's a working directory, it is not in quotes
        //

        for (lp2 = lp3 = lpDir; *lp1 && *lp1 != TEXT(' ') && *lp1 != TEXT('"'); // the command line might be in quotes
                                               lp1 = CharNext(lp1)) {

            *lp3++ = *lp1;

            if (*lp1 == TEXT(':') || *lp1 == TEXT('\\'))
                lp2 = lp3;
        }
        *lp3 = 0;
    }

    /* we are assuming the exe dir contains the necessary separator
     * add the filename to the command line
     */
    lstrcat(lpCommand,lp2);
    /* add the arguments to the command line
     */
    lstrcat(lpCommand,lp1);


    /* truncate the command name from the exe path.  note this implies
     * that if there is no path, lpDir will be empty
     */
    *lp2 = 0;
    lp2 = CharPrev(lpDir,lp2);
    if (*lp2 == TEXT('\\') && *CharPrev(lpDir,lp2) != TEXT(':'))
        *lp2 = 0;

    UnlockGroup(pGroup->hwnd);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DuplicateItem() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PITEM PASCAL DuplicateItem(
    PGROUP pGroup,
    PITEM pItem,
    PGROUP pGNew,
    LPPOINT lppt)
{
    WORD       wIconId;
    WORD       wIconIndex;
    LPITEMDEF  lpid;
    LPGROUPDEF lpgd;
    TCHAR       szCommand[MAXITEMPATHLEN + 1];
    TCHAR       szDefDir[2 * (MAXITEMPATHLEN + 1)];
    TCHAR       szIconPath[MAXITEMPATHLEN + 1];
    TCHAR       szName[64];
    TCHAR       szExpPath[MAXITEMPATHLEN+1];
    TCHAR       szExpDir[MAXITEMPATHLEN+1];
    DWORD       dwFlags = CI_ACTIVATE;

    lpid = LockItem(pGroup, pItem);
    if (lpid == 0L) {
        UnlockGroup(pGroup->hwnd);
        return NULL;
    }

    lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup);

    lstrcpy(szName, (LPTSTR) PTR(lpgd, lpid->pName));
    lstrcpy(szIconPath, (LPTSTR) PTR(lpgd, lpid->pIconPath));
    wIconId = lpid->iIcon;
    wIconIndex = lpid->wIconIndex;
    GlobalUnlock(pGroup->hGroup);
    UnlockGroup(pGroup->hwnd);

    GetItemCommand(pGroup, pItem, szCommand, szDefDir);

    //
    // I f there's no icon path, check if we have an executable associated
    // with the command path.
    //
    if (!*szIconPath) {
        lstrcpy(szExpPath, szCommand);
        DoEnvironmentSubst(szExpPath, CharSizeOf(szExpPath));
        StripArgs(szExpPath);
        lstrcpy(szExpDir, szDefDir);
        DoEnvironmentSubst(szExpDir, CharSizeOf(szExpDir));
        FindExecutable(szExpPath, szExpDir, szIconPath);
        if (!*szIconPath) {
            dwFlags |= CI_NO_ASSOCIATION;
        }
        else
            *szIconPath = 0;
    }
    if (GroupFlag(pGroup, pItem, (WORD)ID_NEWVDM)) {
        dwFlags |= CI_SEPARATE_VDM;
    }

    return CreateNewItem(pGNew->hwnd,
                         szName,
                         szCommand,
                         szIconPath,
                         szDefDir,
                         GroupFlag(pGroup, pItem, (WORD)ID_HOTKEY),
                         GroupFlag(pGroup, pItem, (WORD)ID_MINIMIZE),
                         wIconId,
                         wIconIndex,
                         NULL,
                         lppt,
                         dwFlags);
}
