/****************************** Module Header ******************************\
* Module Name: whotkeys.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the core functions of 3.1 window hotkey processing.
*
* History:
* 16-Apr-1992 JimA      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* HotKeyToWindow
*
* Scans the hotkey table and returns the pwnd corresponding to the
* given hot key.  Returns NULL if no such hot key in the list.  Looks at the
* current key state array.
*
* History:
* 04-15-92 JimA         Ported from Win3.1 sources.
\***************************************************************************/

PWND HotKeyToWindow(
    DWORD key)
{
    PHOTKEYSTRUCT phk;
    int ckeys;

    ckeys = gcHotKey;

    if (ckeys == 0)
        return 0;

    phk = gpHotKeyList;

    while (ckeys) {
        if (phk->key == key)
            return TestWF(phk->spwnd, WFVISIBLE) ? phk->spwnd : NULL;
        phk++;
        ckeys--;
    }

    return 0;
}


/***************************************************************************\
* HotKeyHelper
*
* Scans the hot key list and returns a pointer to the entry for the
* window.
*
* History:
* 04-15-92 JimA         Ported from Win3.1 sources.
\***************************************************************************/

PHOTKEYSTRUCT HotKeyHelper(
    PWND pwnd)
{
    PHOTKEYSTRUCT phk;
    int count;

    count = gcHotKey;

    if (gpHotKeyList == NULL)
        return 0;

    phk = gpHotKeyList;

    while (count) {
        if (phk->spwnd == pwnd)
            return phk;
        phk++;
        count--;
    }

    return 0;
}


/***************************************************************************\
* DWP_SetHotKey
*
* Set the hot key for this window.  Replace existing hot key, or if new
* key is NULL, delete the entry.  Return 2 if key already existed and
* was replaced, 1 if key did not exist and was set, 0 for
* failure, and -1 for invalid hot key.
*
* History:
* 15-Apr-1992 JimA      Ported from Win3.1 sources.
\***************************************************************************/

UINT DWP_SetHotKey(
    PWND  pwnd,
    DWORD dwKey)
{
    PHOTKEYSTRUCT phk;
    BOOL          fKeyExists = FALSE;
    PWND          pwndTemp;

    /*
     * Filter out invalid hotkeys
     */
    if (LOBYTE(dwKey) == VK_ESCAPE ||
        LOBYTE(dwKey) == VK_SPACE ||
        LOBYTE(dwKey) == VK_TAB ||
        LOBYTE(dwKey) == VK_PACKET) {

        return (UINT)-1;
    }

    /*
     * Don't allow hotkeys for children
     */
    if (TestWF(pwnd, WFCHILD))
        return 0;

    /*
     * Check if the hot key exists and is assigned to a different pwnd
     */
    if (dwKey != 0) {

        pwndTemp = HotKeyToWindow(dwKey);

        if ((pwndTemp != NULL) && (pwndTemp != pwnd))
            fKeyExists = TRUE;
    }

    /*
     * Get the hotkey assigned to the window, if any
     */
    if ((phk = HotKeyHelper(pwnd)) == NULL) {

        /*
         * Window doesn't exist in the hotkey list and key is being set
         * to zero, so just return.
         */
        if (dwKey == 0)
            return 1;

        /*
         * Allocate and point to a spot for the new hotkey
         */
        if (gcHotKey >= gcHotKeyAlloc) {

            if (gcHotKeyAlloc) {

                phk = (PHOTKEYSTRUCT)UserReAllocPool(
                        (HANDLE)gpHotKeyList,
                        gcHotKeyAlloc * sizeof(HOTKEYSTRUCT),
                        (gcHotKey + 1) * sizeof(HOTKEYSTRUCT), TAG_HOTKEY);

                if (phk != NULL) {

                    gpHotKeyList = phk;
                    phk = &gpHotKeyList[gcHotKey++];
                    gcHotKeyAlloc = gcHotKey;

                } else {

                    return 0;
                }

            } else {

                UserAssert(gpHotKeyList == NULL);
                UserAssert(gcHotKey == 0);

                phk = (PHOTKEYSTRUCT)UserAllocPool(sizeof(HOTKEYSTRUCT),
                                                   TAG_HOTKEY);

                if (phk != NULL) {

                    gpHotKeyList = phk;
                    gcHotKey = 1;
                    gcHotKeyAlloc = 1;

                } else {

                    return 0;
                }
            }

        } else {
            phk = &gpHotKeyList[gcHotKey++];
        }
    }

    if (dwKey == 0) {

        /*
         * The hotkey for this window is being deleted. Copy the last item
         * on the list on top of the one being deleted.
         */
        if (--gcHotKey) {

            Lock(&phk->spwnd, gpHotKeyList[gcHotKey].spwnd);
            Unlock(&gpHotKeyList[gcHotKey].spwnd);

            phk->key = gpHotKeyList[gcHotKey].key;
            phk = (PHOTKEYSTRUCT)UserReAllocPool((HANDLE)gpHotKeyList,
                gcHotKeyAlloc * sizeof(HOTKEYSTRUCT),
                gcHotKey * sizeof(HOTKEYSTRUCT), TAG_HOTKEY);

            if (phk != NULL) {
                gpHotKeyList = phk;
                gcHotKeyAlloc = gcHotKey;
            }

        } else {

            Unlock(&gpHotKeyList[gcHotKey].spwnd);
            UserFreePool((HANDLE)gpHotKeyList);
            gpHotKeyList = NULL;
            gcHotKeyAlloc = 0;
        }

    } else {

        /*
         * Add the window and key to the list
         */
        phk->spwnd = NULL;
        Lock(&phk->spwnd, pwnd);
        phk->key = dwKey;
    }

    return fKeyExists ? 2 : 1;
}

/***************************************************************************\
* DWP_GetHotKey
*
*
* History:
* 15-Apr-1992 JimA      Created.
\***************************************************************************/

UINT DWP_GetHotKey(
    PWND pwnd)
{
    PHOTKEYSTRUCT phk;

    if ((phk = HotKeyHelper(pwnd)) == NULL)
        return 0;

    return phk->key;
}
