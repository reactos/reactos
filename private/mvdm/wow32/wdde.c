/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WDDE.C
 *  WOW32 DDE worker routines.
 *
 *  History:
 *  WOW DDE support designed and developed by ChandanC
 *
--*/


#include "precomp.h"
#pragma hdrstop

LPDDENODE DDEInitiateList = NULL;
STATIC PHDDE phDDEFirst = NULL;       // pointer to first hDDE entry
STATIC PCPDATA pCPDataFirst = NULL;       // pointer to first CopyData entry

MODNAME(wdde.c);


// This routine maintains a list of client windows that are in
// Initiate mode. This is called from THUNKING of DDE_INITIATE
// message (from both the WMDISP32.C and WMSG16.C).
//

VOID WI32DDEAddInitiator (HAND16 Initiator)
{
    LPDDENODE Node;

    Node = (LPDDENODE) malloc_w(sizeof(DDENODE));

    if (Node) {

        //
        // Initialize Node with the Initiator's window handle
        //

        Node->Initiator = Initiator;

        //
        // Insert this Node into the linked list of DDE_Initiate message
        // in progress.
        //

        Node->Next = DDEInitiateList;
        DDEInitiateList = Node;

        LOGDEBUG(12, ("WOW::WI32DDEInitiator(): thunking -- adding an Initiator %04lX\n", Initiator));
    }
    else {

        //
        // We could not allocate memory.
        //

        LOGDEBUG(12, ("WOW::WI32DDEInitiator(): thunking -- Couldn't allocate memory\n"));
        WOW32ASSERT (FALSE);
    }
}


// This routine deletes the client window that was in Initiate mode. Because
// the initiate message is completed now. This is called from UNTHUNKING
// of DDE_INITIATE message (from both the WMDISP32.C and WMSG16.C).
//

VOID WI32DDEDeleteInitiator(HAND16 Initiator)
{
    LPDDENODE Node, Temp1;

    Node = DDEInitiateList;

    if (Node) {

        while (Node) {
            if (Node->Initiator == Initiator) {

                if (Node == DDEInitiateList) {

                    //
                    // first guy in the list
                    //

                    DDEInitiateList = Node->Next;
                }
                else {

                    //
                    // update the list
                    //

                    Temp1->Next = Node->Next;
                }

                LOGDEBUG(12, ("WOW::WI32DDEDeleteInitiator(): unthunking -- deleting an Initiator %08lX\n", Initiator));

                //
                // free up the memory
                //

                free_w(Node);
                Node = NULL;
            }
            else {

                //
                // traverse the list
                //

                Temp1 = Node;
                Node = Node->Next;
            }
        }

    }
    else {

        // This is an ERROR condition, which should never occur. If it does
        // talk to CHANDANC as soon as possible.
        //

        LOGDEBUG(0, ("WOW::WI32DDEDeletInitiator(): unthunking -- no Initiator\n"));
        WOW32ASSERT (FALSE);
    }
}


// This routine is used by DDE_ACK thunk to figure out how to thunk the
// DDE_ACK message, ie, whether lParam is a combination of 2 atoms or
// its a pointer to 32 bit packed structure.
//

BOOL WI32DDEInitiate(HAND16 Initiator)
{
    LPDDENODE Node;

    Node = DDEInitiateList;

    while (Node) {
        if (Node->Initiator == Initiator) {
            //
            // DDE_Initiate is in progress for this Window
            //

            LOGDEBUG(12, ("WOW::WI32DDEInitiate(): thunking -- found an Initiator %08lX\n", Initiator));
            return (TRUE);
        }
        else {
            Node = Node->Next;
        }
    }
    LOGDEBUG(12, ("WOW::WI32DDEInitiate(): thunking -- did not find an Initiator %08lX\n", Initiator));

    //
    // DDE_Initiate is not in progress for this Window
    //

    return (FALSE);
}


//
// This routine determines if the current dde operation is a poke to MSDRAW
// Pokes to MSDRAW for metafilepicts are special because the metafile pict
// is part of the POKE block.
//

BOOL DDEIsTargetMSDraw(HAND16 To_hwnd)
{
    BOOL   fStatus = FALSE;
    HANDLE hInst;
    HAND16 hModuleName;
    LPSTR  lpszModuleName16, lpszMsDraw = "MSDRAW.EXE";
    WORD   cchModuleName = MAX_PATH, cchMsDraw = 10;
    VPVOID vp;
    LPSTR  lpszNewMsDrawKey = "MSDRAW\\protocol\\StdFileEditing\\verb";
    HKEY   hKey = NULL;
    LONG   Status;

    //
    // To check if the target is msdraw, check the following.
    //
    //  - That the destination window hInst is that of a 16 bit task (this is
    //    checking to see if the LOWORD of the hInst is not 0.
    //  - That the module name is MSDRAW.
    //
    // NOTE: THERE ARE THREE CALLBACK16 ROUTINES IN THIS CALL, MAKING IT AN
    //       EXPENSIVE CALL.  HOWEVER THIS CALL IS RARELY MADE.
    //

    if (
        (hInst = (HANDLE)GetWindowLong((HWND)HWND32(To_hwnd),GWL_HINSTANCE))
        && (LOWORD(hInst) != 0 )
        && (vp = GlobalAllocLock16(GMEM_MOVEABLE, cchModuleName, &hModuleName))
       ) {

        //
        // Callback 16 to get the module name of the current hInst
        //

        if (cchModuleName = GetModuleFileName16( LOWORD(hInst), vp, cchModuleName )) {
            GETMISCPTR(vp, lpszModuleName16);
            fStatus = (cchModuleName >= cchMsDraw)
                      && !WOW32_stricmp( lpszModuleName16 + (cchModuleName - cchMsDraw), lpszMsDraw )
                      && (Status = RegOpenKeyEx( HKEY_CLASSES_ROOT, lpszNewMsDrawKey, 0, KEY_READ, &hKey)) != ERROR_SUCCESS;

            if (hKey) {
                RegCloseKey( hKey );
            }
            FREEMISCPTR(lpszModuleName16);
        }

        //
        // Cleanup
        //

        GlobalUnlockFree16(vp);
    }
    return ( fStatus );
}



// This routine converts a 32 bit DDE memory object into a 16 bit DDE
// memory object. It also, does the data conversion from 32 bit to 16 bit
// for the type of data.
//
// WARNING: The Copyh32Toh16() calls may cause 16-bit memory movement
//

HAND16 DDECopyhData16(HAND16 To_hwnd, HAND16 From_hwnd, HANDLE h32, PDDEINFO pDdeInfo)
{
    HAND16  h16 = 0;
    VPVOID  vp1, vp2;
    DDEDATA *lpMem32;
    DDEDATA16 *lpMem16;
    int cb;

    //
    // NULL handle ?
    //

    if (!h32) {
        LOGDEBUG(12, ("WOW::DDECopyhData16(): h32 is %08x\n", h32));
        return 0;
    }

    cb = GlobalSize(h32);
    lpMem32 = GlobalLock(h32);
    LOGDEBUG(12, ("WOW::DDECopyhData16(): CF_FORMAT is %04x\n", lpMem32->cfFormat));

    switch (lpMem32->cfFormat) {

        default:

        // This is intentional to let it thru to "case statements".
        // ChandanC 5/11/92.

        case CF_TEXT:
        case CF_DSPTEXT:
        case CF_SYLK:
        case CF_DIF:
        case CF_TIFF:
        case CF_OEMTEXT:
        case CF_PENDATA:
        case CF_RIFF:
        case CF_WAVE:
        case CF_OWNERDISPLAY:
            h16 = Copyh32Toh16 (cb, (LPBYTE) lpMem32);

            pDdeInfo->Format = lpMem32->cfFormat;
            break;

        case CF_BITMAP:
        case CF_DSPBITMAP:
            vp1 = GlobalAllocLock16(GMEM_DDESHARE, (sizeof(DDEDATA)-1+sizeof(HAND16)), &h16);
            if (vp1) {
                pDdeInfo->Format = lpMem32->cfFormat;
                GETMISCPTR(vp1, lpMem16);
                RtlCopyMemory(lpMem16, lpMem32, 4);
                STOREWORD(lpMem16->Value, GETHBITMAP16(*((HANDLE *)lpMem32->Value)));
                FLUSHVDMPTR(vp1, (sizeof(DDEDATA)-1+sizeof(HAND16)), lpMem16);
                FREEMISCPTR(lpMem16);
                GlobalUnlock16(h16);
            }
            break;

        case CF_PALETTE:
            vp1 = GlobalAllocLock16(GMEM_DDESHARE, (sizeof(DDEDATA)-1+sizeof(HAND16)), &h16);
            if (vp1) {
                pDdeInfo->Format = lpMem32->cfFormat;
                GETMISCPTR(vp1, lpMem16);
                RtlCopyMemory(lpMem16, lpMem32, 4);
                STOREWORD(lpMem16->Value, GETHPALETTE16(*((HANDLE *)lpMem32->Value)));
                FLUSHVDMPTR(vp1, (sizeof(DDEDATA)-1+sizeof(HAND16)), lpMem16);
                FREEMISCPTR(lpMem16);
                GlobalUnlock16(h16);
            }
            break;

        case CF_DIB:
        {
            LPBYTE lpMemDib32;
            HAND16 hDib16 = 0;
            HANDLE hDib32;

            vp1 = GlobalAllocLock16(GMEM_DDESHARE, (sizeof(DDEDATA)-1+sizeof(HAND16)), &h16);
            if (vp1) {

                GETMISCPTR(vp1, lpMem16);
                RtlCopyMemory(lpMem16, lpMem32, 4);
                FREEMISCPTR(lpMem16);

                hDib32 = (*((HANDLE *)lpMem32->Value));
                if (hDib32) {
                    lpMemDib32 = GlobalLock(hDib32);
                    cb = GlobalSize(hDib32);
                    hDib16 = Copyh32Toh16 (cb, (LPBYTE) lpMemDib32);
                    GlobalUnlock(hDib32);
                    pDdeInfo->Format = lpMem32->cfFormat;
                    pDdeInfo->Flags = 0;
                    pDdeInfo->h16 = 0;
                    DDEAddhandle(To_hwnd, From_hwnd, (HAND16) hDib16, hDib32, pDdeInfo);

                }

                GETMISCPTR(vp1, lpMem16);
                STOREWORD(lpMem16->Value, hDib16);
                GlobalUnlock16(h16);
                FLUSHVDMPTR(vp1, (sizeof(DDEDATA)-1+sizeof(HAND16)), lpMem16);
                FREEMISCPTR(lpMem16);
            }
        }
        break;

        case CF_METAFILEPICT:
        case CF_DSPMETAFILEPICT:
        {
            HANDLE hMeta32, hMF32 = NULL;
            HAND16 hMeta16 = 0, hMF16 = 0;
            LPMETAFILEPICT lpMemMeta32;
            LPMETAFILEPICT16 lpMemMeta16;
            BOOL IsMSDRAWPoke;

            //
            // We need to find out if the to_handle is MSDRAW, in which case
            // we should copy the METAFILEPICT data to the DDEPOKE instead
            // of a handle to the METAFILEPICT.

            if( IsMSDRAWPoke = ((pDdeInfo->Msg == WM_DDE_POKE) && DDEIsTargetMSDraw(To_hwnd)) ) {
                cb  = sizeof(DDEPOKE)-1+sizeof(METAFILEPICT16);
            }
            else {
                cb  = sizeof(DDEDATA)-1+sizeof(HAND16);
            }
            vp1 = GlobalAllocLock16(GMEM_DDESHARE, cb, &h16);


            if (vp1) {
                GETMISCPTR(vp1, lpMem16);
                RtlCopyMemory(lpMem16, lpMem32, 4);
                hMeta32 = (*((HANDLE *)lpMem32->Value));

                if ( IsMSDRAWPoke ) {

                    lpMemMeta16 = (LPMETAFILEPICT16)((PBYTE)lpMem16 + sizeof(DDEPOKE) - 1);
                    RtlZeroMemory( (PVOID)lpMemMeta16, sizeof (METAFILEPICT16) );
                    if (hMeta32) {
                        lpMemMeta32 = GlobalLock(hMeta32);
                        FixMetafile32To16 (lpMemMeta32, lpMemMeta16);
                        FREEMISCPTR(lpMem16);

                        hMF32 = lpMemMeta32->hMF;
                        if (hMF32) {
                            hMF16 = WinMetaFileFromHMF(hMF32, FALSE);
                            pDdeInfo->Format = lpMem32->cfFormat;
                            pDdeInfo->h16 = 0;
                            pDdeInfo->Flags = DDE_METAFILE;
                            DDEAddhandle(To_hwnd, From_hwnd, (HAND16) hMF16, hMF32, pDdeInfo);
                        }

                        GETMISCPTR(vp1, lpMem16);
                        lpMemMeta16 = (LPMETAFILEPICT16)((PBYTE)lpMem16 + sizeof(DDEPOKE) - 1);
                        STOREWORD(lpMemMeta16->hMF, hMF16);
                        GlobalUnlock(hMeta32);
                    }

                }
                else {
                    if (hMeta32) {
                        lpMemMeta32 = GlobalLock(hMeta32);
                        FREEMISCPTR(lpMem16);
                        vp2 = GlobalAllocLock16(GMEM_DDESHARE, sizeof(METAFILEPICT16), &hMeta16);
                        WOW32ASSERT(vp2);
                        if (vp2) {
                            GETMISCPTR(vp2, lpMemMeta16);
                            FixMetafile32To16 (lpMemMeta32, lpMemMeta16);
                            FREEMISCPTR(lpMemMeta16);

                            pDdeInfo->Format = lpMem32->cfFormat;
                            pDdeInfo->Flags = 0;
                            pDdeInfo->h16 = 0;
                            DDEAddhandle(To_hwnd, From_hwnd, (HAND16) hMeta16, hMeta32, pDdeInfo);
                            hMF32 = lpMemMeta32->hMF;
                            if (hMF32) {
                                hMF16 = WinMetaFileFromHMF(hMF32, FALSE);
                                pDdeInfo->Flags = DDE_METAFILE;
                                DDEAddhandle(To_hwnd, From_hwnd, (HAND16) hMF16, hMF32, pDdeInfo);
                            }

                            GETMISCPTR(vp2, lpMemMeta16);
                            STOREWORD(lpMemMeta16->hMF, hMF16);
                            GlobalUnlock16(hMeta16);
                            FLUSHVDMPTR(vp2, 8, lpMemMeta16);
                            FREEMISCPTR(lpMemMeta16);
                        }
                        GlobalUnlock(hMeta32);
                    }
                    GETMISCPTR(vp1, lpMem16);
                    STOREWORD(lpMem16->Value, hMeta16);
                }

                GlobalUnlock16(h16);
                FLUSHVDMPTR(vp1, cb, lpMem16);
                FREEMISCPTR(lpMem16);
            }
        }
        break;
    }

    GlobalUnlock(h32);

    return (h16);
}




// This routine converts a 16 bit DDE memory object into a 32 bit DDE
// memory object. It also, does the data conversion from 16 bit to 32 bit
// for the type of data.
//

HANDLE  DDECopyhData32(HAND16 To_hwnd, HAND16 From_hwnd, HAND16 h16, PDDEINFO pDdeInfo)
{
    HANDLE  h32 = NULL;
    INT     cb;
    VPVOID  vp;
    DDEDATA *lpMem16;
    DDEDATA32 *lpMem32;

    //
    // AmiPro passes a NULL handle.
    //

    if (!h16) {
        LOGDEBUG(12, ("WOW::DDECopyhData16(): h16 is %04x\n", h16));
        return (HANDLE) NULL;
    }

    vp = GlobalLock16(h16, &cb);
    GETMISCPTR(vp, lpMem16);
    LOGDEBUG(12, ("WOW::DDECopyhData32(): CF_FORMAT is %04x\n", lpMem16->cfFormat));

    switch(lpMem16->cfFormat) {

        default:

        // This is intentional to let it thru to the "case statements".
        // ChandanC 5/11/92.

        case CF_TEXT:
        case CF_DSPTEXT:
        case CF_SYLK:
        case CF_DIF:
        case CF_TIFF:
        case CF_OEMTEXT:
        case CF_PENDATA:
        case CF_RIFF:
        case CF_WAVE:
        case CF_OWNERDISPLAY:
            h32 = Copyh16Toh32 (cb, (LPBYTE) lpMem16);

            pDdeInfo->Format = lpMem16->cfFormat;
        break;

        case CF_BITMAP:
        case CF_DSPBITMAP:
            h32 = WOWGLOBALALLOC(GMEM_DDESHARE, (sizeof(DDEDATA)-1+sizeof(HANDLE)));
            if (h32) {
                pDdeInfo->Format = lpMem16->cfFormat;
                lpMem32 = GlobalLock(h32);
                RtlCopyMemory(lpMem32, lpMem16, 4);
                lpMem32->Value = HBITMAP32(FETCHWORD(*((WORD *)lpMem16->Value)));
                GlobalUnlock(h32);
            }
            break;

        case CF_PALETTE:
            h32 = WOWGLOBALALLOC(GMEM_DDESHARE, (sizeof(DDEDATA)-1+sizeof(HANDLE)));
            if (h32) {
                pDdeInfo->Format = lpMem16->cfFormat;
                lpMem32 = GlobalLock(h32);
                RtlCopyMemory(lpMem32, lpMem16, 4);
                lpMem32->Value = HPALETTE32(FETCHWORD(*((WORD *)lpMem16->Value)));
                GlobalUnlock(h32);
            }
            break;

        case CF_DIB:
        {
            LPBYTE lpMemDib16;
            HAND16 hDib16;
            HANDLE hDib32 = NULL;

            h32 = WOWGLOBALALLOC(GMEM_DDESHARE, (sizeof(DDEDATA)-1+sizeof(HANDLE)));
            if (h32) {
                lpMem32 = GlobalLock(h32);
                RtlCopyMemory(lpMem32, lpMem16, 4);

                hDib16 = FETCHWORD(*((WORD *)lpMem16->Value));
                if (hDib16) {
                    vp = GlobalLock16(hDib16, &cb);
                    GETMISCPTR(vp, lpMemDib16);
                    hDib32 = Copyh16Toh32 (cb, (LPBYTE) lpMemDib16);

                    pDdeInfo->Format = lpMem16->cfFormat;
                    pDdeInfo->Flags = 0;
                    pDdeInfo->h16 = 0;
                    DDEAddhandle(To_hwnd, From_hwnd, (HAND16) hDib16, hDib32, pDdeInfo);

                    GlobalUnlock16(hDib16);
                    FREEMISCPTR(lpMemDib16);
                }
                lpMem32->Value = hDib32;
                GlobalUnlock(h32);
            }
        }
        break;

        case CF_METAFILEPICT:
        case CF_DSPMETAFILEPICT:
        {
            HANDLE hMeta32 = NULL, hMF32 = NULL;
            HAND16 hMeta16, hMF16 = 0;
            LPMETAFILEPICT lpMemMeta32;
            LPMETAFILEPICT16 lpMemMeta16;

            h32 = WOWGLOBALALLOC(GMEM_DDESHARE, (sizeof(DDEDATA)-1+sizeof(HANDLE)));
            if (h32) {
                lpMem32 = GlobalLock(h32);
                RtlCopyMemory(lpMem32, lpMem16, 4);

                //
                // MSDRAW has the METAFILEPICT in the DDEPOKE block instead of
                // a handle to the METAFILEPICT.  So we need to find out if the
                // to handle belongs to MSDRAW.  Since MSDRAW is a 16 bit
                // server we needn't thunk the metafilepict at all, we will just
                // use NULL as the 32 bit handle to the metafilepict.
                //

                hMeta32 = NULL;
                if( !((pDdeInfo->Msg == WM_DDE_POKE) && DDEIsTargetMSDraw(To_hwnd)) ) {

                    hMeta16 = FETCHWORD(*((WORD *)lpMem16->Value));

                    //
                    // Make sure that a valid metafile pict handle has been
                    // passed in otherwise use NULL again as the hMeta32.
                    //

                    if (hMeta16 && (vp = GlobalLock16(hMeta16, &cb))) {
                        GETMISCPTR(vp, lpMemMeta16);
                        hMeta32 = WOWGLOBALALLOC(GMEM_DDESHARE, sizeof(METAFILEPICT));
                        WOW32ASSERT(hMeta32);
                        if (hMeta32) {
                            lpMemMeta32 = GlobalLock(hMeta32);
                            lpMemMeta32->mm = (LONG) FETCHSHORT(lpMemMeta16->mm);
                            lpMemMeta32->xExt = (LONG) FETCHSHORT(lpMemMeta16->xExt);
                            lpMemMeta32->yExt = (LONG) FETCHSHORT(lpMemMeta16->yExt);
                            pDdeInfo->Format = lpMem16->cfFormat;
                            pDdeInfo->Flags = 0;
                            pDdeInfo->h16 = 0;
                            DDEAddhandle(To_hwnd, From_hwnd, (HAND16) hMeta16, hMeta32, pDdeInfo);

                            hMF16 = FETCHWORD(lpMemMeta16->hMF);

                            if (hMF16) {
                                hMF32 = (HMETAFILE) HMFFromWinMetaFile(hMF16, FALSE);
                                pDdeInfo->Flags = DDE_METAFILE;
                                DDEAddhandle(To_hwnd, From_hwnd, (HAND16) hMF16, hMF32, pDdeInfo);
                            }

                            lpMemMeta32->hMF = (HMETAFILE) hMF32;
                            GlobalUnlock(hMeta32);
                        }
                        GlobalUnlock16(hMeta16);
                        FREEMISCPTR(lpMemMeta16);
                    }
                }
                lpMem32->Value = hMeta32;
                GlobalUnlock(h32);
            }
        }
        break;
    }

    GlobalUnlock16(h16);

    FREEMISCPTR(lpMem16);
    return (h32);
}


/****** These routines maintain a linked list of dde handles, which
******* are the h16 and h32 pairs.
******/



//  This routine adds the given h16-h32 pair to the linked list, and updates
//  the list.
//

BOOL DDEAddhandle(HAND16 To_hwnd, HAND16 From_hwnd, HAND16 hMem16, HANDLE hMem32, PDDEINFO pDdeInfo)
{
    PHDDE   phTemp;

    if (hMem16 && hMem32) {
        if (phTemp = malloc_w (sizeof(HDDE))) {
            phTemp->hMem16    = hMem16;
            phTemp->hMem32    = hMem32;
            phTemp->To_hwnd   = To_hwnd;
            phTemp->From_hwnd = From_hwnd;

            phTemp->DdeMsg    = pDdeInfo->Msg;
            phTemp->DdeFormat = pDdeInfo->Format;
            phTemp->DdeFlags  = pDdeInfo->Flags;

            phTemp->h16       = pDdeInfo->h16;

            phTemp->pDDENext  = phDDEFirst;     // insert at the top
            phDDEFirst        = phTemp;         // update list head

            // Mark the GAH_WOWDDEFREEHANDLE (ie GAH_PAHTOM) bit in the global
            // arena of this handle.

            W32MarkDDEHandle (hMem16);

            return (TRUE);
        }
        else {
            LOGDEBUG(2, ("WOW::DDEAddhandle(): *** memory allocation failed *** \n"));
            return (FALSE);
        }
    }

    LOGDEBUG(2,("WOW::DDEAddhandle(): *** ERROR *** one of the handles is NULL \n"));
    return (FALSE);
}


//  This routine deletes the given h16-h32 pair from the list and frees up
//   the memory.
//

BOOL DDEDeletehandle(HAND16 h16, HANDLE h32)
{
    PHDDE   phTemp1, phTemp2;

    phTemp1 = phDDEFirst;

    if ((phTemp1->hMem16 == h16) && (phTemp1->hMem32 == h32)) {  // first node
        phDDEFirst = phTemp1->pDDENext;
        free_w(phTemp1);
        return (TRUE);
    }
    else {                // rest of the list
        phTemp2 = phTemp1;
        phTemp1 = phTemp1->pDDENext;

        while (phTemp1) {
            if ((phTemp1->hMem16 == h16) && (phTemp1->hMem32 == h32)) {
                phTemp2->pDDENext = phTemp1->pDDENext;
                free_w(phTemp1);
                return (TRUE);
            }
            phTemp2 = phTemp1;
            phTemp1 = phTemp1->pDDENext;
        }

        LOGDEBUG (2, ("WOW::DDEDeleteHandle : Can't find a 16-32 memory pair\n"));
//        WOW32ASSERT (FALSE);

        return (FALSE);
    }
}


// This routine finds a hMem16 for a DDE conversation, if one exists.
//

HAND16 DDEFindPair16(HAND16 To_hwnd, HAND16 From_hwnd, HANDLE hMem32)
{
    PHDDE   phTemp;

    phTemp = phDDEFirst;

    while (phTemp) {
        if ((phTemp->To_hwnd == To_hwnd) &&
            (phTemp->From_hwnd == From_hwnd) &&
            (phTemp->hMem32 == hMem32)) {
                return (phTemp->hMem16);
        }
        else {
            phTemp = phTemp->pDDENext;
        }
    }
    return (HAND16) NULL;
}


// This routine finds a hMem32 for a DDE conversation, if one exists.
//

HANDLE DDEFindPair32(HAND16 To_hwnd, HAND16 From_hwnd, HAND16 hMem16)
{
    PHDDE   phTemp;

    phTemp = phDDEFirst;

    while (phTemp) {
        if ((phTemp->To_hwnd == To_hwnd) &&
            (phTemp->From_hwnd == From_hwnd) &&
            (phTemp->hMem16 == hMem16)) {
                return (phTemp->hMem32);
        }
        else {
            phTemp = phTemp->pDDENext;
        }
    }
    return (HANDLE) NULL;
}


// This routine find the DDE node that is doing DDE conversation
//

PHDDE DDEFindNode16 (HAND16 h16)
{
    PHDDE   phTemp;

    phTemp = phDDEFirst;

    while (phTemp) {
        if (phTemp->hMem16 == h16) {
            return (phTemp);
        }
        phTemp = phTemp->pDDENext;
    }

    return (NULL);
}


// This routine find the DDE node that is doing DDE conversation
//

PHDDE DDEFindNode32 (HANDLE h32)
{
    PHDDE   phTemp;

    phTemp = phDDEFirst;

    while (phTemp) {
        if (phTemp->hMem32 == h32) {
            return (phTemp);
        }
        phTemp = phTemp->pDDENext;
    }

    return (NULL);
}


// This routine returns a pointer to the DDE node, if the conversation exists,
// else it retunrs NULL

PHDDE DDEFindAckNode (HAND16 To_hwnd, HAND16 From_hwnd, HANDLE hMem32)
{
    PHDDE   phTemp;

    phTemp = phDDEFirst;

    while (phTemp) {
        if ((phTemp->To_hwnd == To_hwnd) &&
            (phTemp->From_hwnd == From_hwnd) &&
            (phTemp->hMem32 == hMem32)) {
                return (phTemp);
        }
        else {
            phTemp = phTemp->pDDENext;
        }
    }
    return (PHDDE) NULL;
}


//  This function marks GAH_WOWDDEFREEHANDLE bit in the global arena of the
//  hMem16.
//

VOID W32MarkDDEHandle (HAND16 hMem16)
{
    PARM16 Parm16;
    VPVOID vp = 0;

    Parm16.WndProc.wParam = hMem16;
    Parm16.WndProc.wMsg = 1;
    CallBack16(RET_WOWDDEFREEHANDLE, &Parm16, 0, &vp);
}

VOID W32UnMarkDDEHandle (HAND16 hMem16)
{
    PARM16 Parm16;
    VPVOID vp = 0;

    Parm16.WndProc.wParam = hMem16;
    Parm16.WndProc.wMsg = 0;
    CallBack16(RET_WOWDDEFREEHANDLE, &Parm16, 0, &vp);
}

// This function frees the 32 and 16 bit memory. It is called by 32 bit
// BASE by GlobalFree.
//

BOOL W32DDEFreeGlobalMem32 (HANDLE h32)
{
    HAND16 h16;
    PHDDE pDdeNode;
    BOOL fOkToFree = TRUE;

    if (h32) {
        if (pDdeNode = DDEFindNode32(h32)) {

            if (pDdeNode->DdeFlags & DDE_METAFILE) {
                LOGDEBUG (12, ("WOW32: W32DDEFreeGlobalMem32: Freeing MetaFile hMF32 %x\n", h32));
                DeleteMetaFile (h32);
                fOkToFree = FALSE;
            }

            while ((pDdeNode) && (h16 = pDdeNode->hMem16)) {
                W32UnMarkDDEHandle (h16);
                GlobalUnlockFree16(GlobalLock16(h16, NULL));
                DDEDeletehandle(h16, h32);
                pDdeNode = DDEFindNode32(h32);
            }
        }
        else {

            LOGDEBUG (2, ("WOW32: W32DDEFreeGlobalMem32: Can't find a 16-32 memory pair\n"));
        }
    }
    else {
        WOW32WARNMSG(FALSE, "WOW32: W32DDEFreeGlobalMem32: h32 is NULL to Win32 GlobalFree\n");
        /*
         * since in this case the Failure and Success return values from
         * GlobalFree are NULL, just return false so things are faster
         * in GlobalFree.
         */
        fOkToFree = FALSE;
    }

    return(fOkToFree);
}


// This function frees only the 32 bit memory because the 16 bit memory
// is being free'd by the 16 bit app. We are just getting the
// notification of this fact.  So free the corresponding 32 bit memory.
//

ULONG FASTCALL WK32WowDdeFreeHandle (PVDMFRAME pFrame)
{
    ULONG  ul;
    HAND16 h16;
    PWOWDDEFREEHANDLE16 parg16;

    GETARGPTR(pFrame, sizeof(WOWDDEFREEHANDLE16), parg16);

    h16 = (HAND16) parg16->h16;

    ul = W32DdeFreeHandle16 (h16);

    FREEARGPTR(parg16);
    RETURN (ul);
}


BOOL W32DdeFreeHandle16 (HAND16 h16)
{
    HANDLE h32;
    PHDDE pDdeNode;

    if (!(pDdeNode = DDEFindNode16(h16))) {
        LOGDEBUG (12, ("WOW::W32DdeFreeHandle16 : Not found h16 -> %04x\n", h16));

        // in this case look for a 16:32 pair in the list of hdrop handles
        // see file wshell.c for comments
        FindAndReleaseHDrop16(h16);

        return (TRUE);
    }

    LOGDEBUG (12, ("WOW::W32DdeFreeHandle16 : Entering... h16 -> %04x\n", h16));

    if (pDdeNode->DdeMsg == WM_DDE_EXECUTE) {
        LOGDEBUG (12, ("WOW::W32DdeFreeHandle16 : App TRYING  !!! to freeing EXECUTE h16 -> %04x\n", h16));
        pDdeNode->DdeFlags = pDdeNode->DdeFlags | DDE_EXECUTE_FREE_MEM;
        return (FALSE);
    }
    else {
        while ((pDdeNode) && (h32 = pDdeNode->hMem32)) {
            if (pDdeNode->DdeFlags & DDE_METAFILE) {
                DDEDeletehandle(h16, h32);
                DeleteMetaFile (h32);
            }
            else {
                /*
                 * REMOVE THE PAIR FIRST!!!
                 * Since GlobalFree will hook back to W32DDEFreeGlobalMem32
                 * we want to remove the handle from our tables before
                 * the call.
                 */
                DDEDeletehandle(h16, h32);
                WOWGLOBALFREE(h32);
            }

            pDdeNode = DDEFindNode16(h16);
        }
    }

    LOGDEBUG (12, ("WOW::W32DdeFreeHandle16 : Leaving ...\n"));
    return (TRUE);
}


//  This routine adds the given h16-h32 CopyData pair to the linked list,
//  and updates the list.
//

BOOL CopyDataAddNode (HAND16 To_hwnd, HAND16 From_hwnd, DWORD Mem16, DWORD Mem32, DWORD Flags)
{
    PCPDATA pTemp;

    if (Mem16 && Mem32) {
        if (pTemp = malloc_w (sizeof(CPDATA))) {
            pTemp->Mem16    = Mem16;
            pTemp->Mem32    = Mem32;
            pTemp->To_hwnd  = To_hwnd;
            pTemp->From_hwnd= From_hwnd;
            pTemp->Flags    = Flags;
            pTemp->Next     = pCPDataFirst;     // insert at the top
            pCPDataFirst    = pTemp;         // update list head

            return (TRUE);
        }
        else {
            LOGDEBUG(2, ("WOW::CopyDataAddNode: *** memory allocation failed *** \n"));
            return (FALSE);
        }
    }

    LOGDEBUG(2,("WOW::CopyDataAddNode: *** ERROR *** one of the memory pointers is NULL \n"));
    return (FALSE);
}


VPVOID CopyDataFindData16 (HWND16 To_hwnd, HWND16 From_hwnd, DWORD Mem)
{
    PCPDATA pTemp;

    pTemp = pCPDataFirst;

    while (pTemp) {
        if ((pTemp->To_hwnd == To_hwnd) &&
            (pTemp->From_hwnd == From_hwnd) &&
            (pTemp->Mem32 == Mem)) {
                return (pTemp->Mem16);
        }
        else {
            pTemp = pTemp->Next;
        }
    }
    return 0;
}


PCPDATA CopyDataFindData32 (HWND16 To_hwnd, HWND16 From_hwnd, DWORD Mem)
{
    PCPDATA pTemp;

    pTemp = pCPDataFirst;

    while (pTemp) {
        if ((pTemp->To_hwnd == To_hwnd) &&
            (pTemp->From_hwnd == From_hwnd) &&
            (pTemp->Mem16 == Mem)) {
                return (pTemp);
        }
        else {
            pTemp = pTemp->Next;
        }
    }
    return 0;
}


//  This routine deletes the given h16-h32 pair from the list.
//
//

BOOL CopyDataDeleteNode (HWND16 To_hwnd, HWND16 From_hwnd, DWORD Mem)
{
    PCPDATA pTemp1;
    PCPDATA pTemp2;

    pTemp1 = pCPDataFirst;

    if ((pTemp1->To_hwnd == To_hwnd) &&
        (pTemp1->From_hwnd == From_hwnd) &&
        (pTemp1->Mem32 == Mem)) {
        pCPDataFirst = pTemp1->Next;
        free_w (pTemp1);
        return (TRUE);
    }
    else {
        pTemp2 = pTemp1;
        pTemp1 = pTemp1->Next;

        while (pTemp1) {
            if ((pTemp1->To_hwnd == To_hwnd) &&
                (pTemp1->From_hwnd == From_hwnd) &&
                (pTemp1->Mem32 == Mem)) {
                    pTemp2->Next = pTemp1->Next;
                    free_w (pTemp1);
                    return (TRUE);
            }

            pTemp2 = pTemp1;
            pTemp1 = pTemp1->Next;
        }
        return (FALSE);
    }

}


// While allocating GMEM_DDESHARE memory object should we have GMEM_MOVEABLE
// flag or not ???????????????????
// ChandanC Sept 23rd 1993.
//
// WARNING: This function may cause 16-bit memory movement.
//

HAND16  Copyh32Toh16 (int cb, LPBYTE lpMem32)
{
    HAND16  h16 = 0;
    LPBYTE  lpMem16;
    VPVOID  vp;

    vp = GlobalAllocLock16(GMEM_DDESHARE | GMEM_MOVEABLE, cb, &h16);
    WOW32ASSERT(vp);
    if (vp) {
        GETMISCPTR(vp, lpMem16);
        RtlCopyMemory(lpMem16, lpMem32, cb);
        GlobalUnlock16(h16);
        FLUSHVDMPTR(vp, cb, lpMem16);
        FREEMISCPTR(lpMem16);
    }

    return (h16);
}


HANDLE  Copyh16Toh32 (int cb, LPBYTE lpMem16)
{
    HANDLE hMem32;
    LPBYTE  lpMem32;

    hMem32 = WOWGLOBALALLOC(GMEM_DDESHARE | GMEM_MOVEABLE, cb);
    WOW32ASSERT(hMem32);
    if (hMem32) {
        lpMem32 = GlobalLock(hMem32);
        RtlCopyMemory (lpMem32, lpMem16, cb);
        GlobalUnlock(hMem32);
    }

    return (hMem32);
}


VOID  FixMetafile32To16 (LPMETAFILEPICT lpMemMeta32, LPMETAFILEPICT16 lpMemMeta16)
{

    if (lpMemMeta32->mm == MM_ANISOTROPIC) {
        LONG xExt = lpMemMeta32->xExt;
        LONG yExt = lpMemMeta32->yExt;

        while (xExt < (LONG)(SHORT)MINSHORT
            || xExt > (LONG)(SHORT)MAXSHORT
            || yExt < (LONG)(SHORT)MINSHORT
            || yExt > (LONG)(SHORT)MAXSHORT) {
            xExt = xExt / 2;
            yExt = yExt / 2;
        }
        STORESHORT(lpMemMeta16->mm,   MM_ANISOTROPIC);
        STORESHORT(lpMemMeta16->xExt, xExt);
        STORESHORT(lpMemMeta16->yExt, yExt);
    }
    else {
        STORESHORT(lpMemMeta16->mm,   lpMemMeta32->mm);
        STORESHORT(lpMemMeta16->xExt, lpMemMeta32->xExt);
        STORESHORT(lpMemMeta16->yExt, lpMemMeta32->yExt);
    }
}

//
// CHEESE ALERT: This function is exported for the OLE DDE code
// to call so it can correctly free up metafile handle pairs in
// a VDM. This function is NOT found in any header files. If you
// change this, you need to find its use in the OLE project.
// Probably best to just leave it alone.
//
BOOL WINAPI WOWFreeMetafile( HANDLE h32 )
{
    return( W32DDEFreeGlobalMem32( h32 ) );
}
