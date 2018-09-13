/****************************** Module Header ******************************\
* Module Name: notify.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Handles notification of key value changes in the registry that affect
* the Program Manager.
*
* History:
* 04-16-92 JohanneC       Created.
\***************************************************************************/
#include "progman.h"


BOOL InHandleProgramGroupsEvent = FALSE;
BOOL bHandleProgramGroupsEvent = TRUE;

//
// 2 watch events: common groups key & personal groups key
//
HANDLE gahEvents[2];
HANDLE hEventCommonGroups;
HANDLE hEventPersGroups;
HANDLE hEventGroupValueSet;

HANDLE hChangeNotifyThread;

/***************************************************************************\
*
*
*
* History:
* 04-16-91 Johannec       Created
\***************************************************************************/
BOOL APIENTRY InitializeGroupKeyNotification()
{

    hEventCommonGroups = CreateEvent(NULL, FALSE, FALSE, NULL);
    hEventPersGroups = CreateEvent(NULL, FALSE, FALSE, NULL);
    hEventGroupValueSet = CreateEvent(NULL, FALSE, FALSE,
                                      TEXT(" Progman.GroupValueSet"));

    gahEvents[0] = hEventPersGroups;
    gahEvents[1] = hEventCommonGroups;

    if (hEventPersGroups && hkeyProgramGroups) {
        RegNotifyChangeKeyValue(hkeyProgramGroups, TRUE, REG_NOTIFY_CHANGE_NAME,
                                 hEventPersGroups, TRUE);
    }
    if (hEventCommonGroups && hkeyCommonGroups) {
        RegNotifyChangeKeyValue(hkeyCommonGroups, TRUE, REG_NOTIFY_CHANGE_NAME,
                                 hEventCommonGroups, TRUE);
    }


    if (!hEventCommonGroups && !hEventPersGroups) {
        return(FALSE);
    }

    return(TRUE);
}


VOID APIENTRY ResetProgramGroupsEvent(BOOL bCommonGroup)
{
   if (bCommonGroup) {
       if (hEventCommonGroups && hkeyCommonGroups) {
           ResetEvent(hEventCommonGroups);
           RegNotifyChangeKeyValue(hkeyCommonGroups, TRUE, REG_NOTIFY_CHANGE_NAME,
               hEventCommonGroups, TRUE);
       }
   }
   else {
      if (hEventPersGroups && hkeyProgramGroups) {
          ResetEvent(hEventPersGroups);
          RegNotifyChangeKeyValue(hkeyProgramGroups, TRUE, REG_NOTIFY_CHANGE_NAME,
              hEventPersGroups, TRUE);
      }
   }
}

/***************************************************************************\
*
*
*
*
* History:
* 04-16-91 Johannec       Created
\***************************************************************************/
VOID HandleGroupKeyChange(BOOL bCommonGroup)
{
    int i = 0;
    DWORD cbGroupKey = MAXKEYLEN;
    TCHAR szGroupKey[MAXKEYLEN];
    FILETIME ft;
    HWND hwndT;
    PGROUP pGroup;
    PGROUP *ppGroup;
    PITEM pItem;
    HKEY hkeyGroups;
    HKEY hkey;
    TCHAR szT[10];

    if (InHandleProgramGroupsEvent || !bHandleProgramGroupsEvent) {
        goto RegNotify;
    }

    InHandleProgramGroupsEvent = TRUE;

    if (bCommonGroup) {
        hkeyGroups = hkeyCommonGroups;
    }
    else {
        hkeyGroups = hkeyProgramGroups;
    }

    while (!RegEnumKeyEx(hkeyGroups, i, szGroupKey, &cbGroupKey, 0, 0, 0, &ft)) {
        if (cbGroupKey) {
          /* Search for the group... if it already exists, activate it. */
          hwndT = GetWindow(hwndMDIClient, GW_CHILD);
          while (hwndT) {
              /* Skip icon titles. */
              if (!GetWindow(hwndT, GW_OWNER)) {
                  /* Compare the group title with the request. */
                  pGroup = (PGROUP)GetWindowLongPtr(hwndT,GWLP_PGROUP);
                  if (!lstrcmpi(szGroupKey, pGroup->lpKey)) {
                      if (pGroup->fCommon && bCommonGroup ||
                           !pGroup->fCommon && !bCommonGroup)
                          break;
                  }
              }
              hwndT = GetWindow(hwndT, GW_HWNDNEXT);
          }

          /* If we didn't find it, load it. */
          if (!hwndT) {
              //
              // Wait until the value is set before loading the group.
              //
              WaitForSingleObject(hEventGroupValueSet, 300);
              ResetEvent(hEventGroupValueSet);

              LoadGroupWindow(szGroupKey, 0, bCommonGroup);
          }
        }

        i++;
        cbGroupKey = MAXKEYLEN;
    }

    //
    // Test if any groups were deleted through regedt32.exe.
    //

    for (hwndT = GetWindow(hwndMDIClient, GW_CHILD);
                   hwndT;
                   hwndT = GetWindow(hwndT, GW_HWNDNEXT)) {

        /* Skip icon titles. */
        if (GetWindow(hwndT, GW_OWNER))
            continue;

        /* Compare the group title with the request. */
         pGroup = (PGROUP)GetWindowLongPtr(hwndT, GWLP_PGROUP);
         if ( (pGroup->fCommon && !bCommonGroup) ||
              (!pGroup->fCommon && bCommonGroup) ) {
             continue;
         }

         if (RegOpenKey(hkeyGroups, pGroup->lpKey, &hkey) == ERROR_SUCCESS) {
             RegCloseKey(hkey);
         }
         else {
             //
             // Couldn't find the group in the registry so delete it
             // in progman.
             //
             /* Destroy the window, the global memory block, and the file. */
             SendMessage(hwndMDIClient, WM_MDIDESTROY, (WPARAM)hwndT, 0L);
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
    }

RegNotify:

    ResetProgramGroupsEvent(bCommonGroup);

    InHandleProgramGroupsEvent = FALSE;
}
