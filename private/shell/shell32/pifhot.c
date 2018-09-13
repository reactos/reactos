/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1991
 *  All Rights Reserved.
 *
 *
 *  PIFHOT.C
 *  User interface routines for hot-keys
 *
 *  History:
 *  Created 21-Dec-1992 5:30pm by Jeff Parsons (based on old PIFEDIT code)
 *  Rewritten 25-Dec-1993 by Raymond Chen
 */

#include "shellprv.h"
#pragma hdrstop

/*
 * Values for the second argument to MapVirtualKey, mysteriously
 * missing from windows.h.
 */
#define MVK_OEMFROMVK 0
#define MVK_VKFROMOEM 1

/*
 * BEWARE!  Converting a VK to a scan code and back again does not
 * necessarily get you back where you started!  The main culprit
 * is the numeric keypad, since (for example) VK_LEFT and VK_NUMPAD4
 * both map to scan code 0x4B.  We catch the numeric keypad specially
 * for exactly this purpose.
 *
 * The following table converts VK codes VK_NUMPAD0 through VK_NUMPAD9
 * to the corresponding VK_ code if NumLock is off.
 *
 * Note that this table and the loop that accesses it assume that the
 * scan codes VK_NUMPAD0 through VK_NUMPAD9 are consecutive.
 */

WORD mpvkvk[10] = {
    VK_INSERT,                  /* VK_NUMPAD0 */
    VK_END,                     /* VK_NUMPAD1 */
    VK_DOWN,                    /* VK_NUMPAD2 */
    VK_NEXT,                    /* VK_NUMPAD3 */
    VK_LEFT,                    /* VK_NUMPAD4 */
    VK_CLEAR,                   /* VK_NUMPAD5 */
    VK_RIGHT,                   /* VK_NUMPAD6 */
    VK_HOME,                    /* VK_NUMPAD7 */
    VK_UP,                      /* VK_NUMPAD8 */
    VK_PRIOR,                   /* VK_NUMPAD9 */
};


/** HotKeyWindowsFromOem - Convert OEM hotkey into Windows hotkey
 *
 * INPUT
 *  lppifkey -> PIFKEY describing OEM Hotkey
 *
 * OUTPUT
 *  Windows hotkey value corresponding to lpwHotkey.
 */

WORD HotKeyWindowsFromOem(LPCPIFKEY lppifkey)
{
    WORD wHotKey = 0;

    if (lppifkey->Scan) {
        wHotKey = (WORD) MapVirtualKey(lppifkey->Scan, MVK_VKFROMOEM);

        if (lppifkey->Val & 2) {
            WORD vk;
            for (vk = VK_NUMPAD0; vk <= VK_NUMPAD9; vk++) {
                if (wHotKey == mpvkvk[vk - VK_NUMPAD0]) {
                    wHotKey = vk; break;
                }
            }
            ASSERTTRUE(vk <= VK_NUMPAD9); /* Buggy PIF; do what we can */
        }

        if (lppifkey->Val & 1) wHotKey |= (HOTKEYF_EXT << 8);

        if (lppifkey->ShVal & (fPIFSh_RShf | fPIFSh_LShf))
            wHotKey |= (HOTKEYF_SHIFT << 8);

        if (lppifkey->ShVal & (fPIFSh_LCtrl|fPIFSh_RCtrl|fPIFSh_Ctrl))
            wHotKey |= (HOTKEYF_CONTROL << 8);

        if (lppifkey->ShVal & (fPIFSh_LAlt|fPIFSh_RAlt|fPIFSh_Alt))
            wHotKey |= (HOTKEYF_ALT << 8);
    }
    return wHotKey;
}


/** HotKeyOemFromWindows - Convert Windows hotkey into OEM hotkey
 *
 * INPUT
 *  lppifkey -> struct PIF_Key to receive OEM hotkey
 *  wHotKey  =  Windows hotkey
 *
 * OUTPUT
 *  lppifkey filled with hotkey info
 */

void HotKeyOemFromWindows(LPPIFKEY lppifkey, WORD wHotKey)
{
    lppifkey->Scan = 0;
    lppifkey->ShVal = 0;
    lppifkey->ShMsk = 0;
    lppifkey->Val = 0;

    if (wHotKey) {
        lppifkey->Scan = (WORD) MapVirtualKey(LOBYTE(wHotKey), MVK_OEMFROMVK);
        lppifkey->ShMsk = fPIFSh_RShf | fPIFSh_LShf | fPIFSh_Ctrl | fPIFSh_Alt;

        if (wHotKey & (HOTKEYF_EXT << 8)) lppifkey->Val |= 1;

        /* Assumes that VK_NUMPAD0 through VK_NUMPAD9 are consecutive */
        if ((wHotKey - VK_NUMPAD0) < 10) lppifkey->Val |= 2;

        if (wHotKey & (HOTKEYF_SHIFT << 8))
            lppifkey->ShVal |= fPIFSh_RShf | fPIFSh_LShf;

        if (wHotKey & (HOTKEYF_CONTROL << 8))
            lppifkey->ShVal |= fPIFSh_Ctrl;

        if (wHotKey & (HOTKEYF_ALT << 8))
            lppifkey->ShVal |= fPIFSh_Alt;
    }
}
