
/*****************************************************************************

                    C L I P B O O K   U T I L I T I E S

    Name:       cvutil.c
    Date:       21-Jan-1994
    Creator:    Unknown

    Description:
        Utility functions for clipbook viewer.

    History:
        03-Feb-1994 John Fu     Add switch in ProcessDataReq's !hData path
                                to allow the lock to be displayed.
        19-Apr-1994 John Fu     Add DDE_DIB2BITMAP and veriouse fixes for
                                DIB to BITMAP conversion.
        30-Jun-1994 John Fu     Improved GetPreviewBitmap to allow multiple
                                connect tries and disable EDIT while waiting
                                to complete async transaction (there's still
                                holes).
        13-Mar-1995 John Fu     Add hXacting event to fix timing problems
                                Add Paste to Page stuff
                                Add UpdatePage
                                Fix MyMsgFilterProc

*****************************************************************************/





#define WIN31
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <assert.h>
#include <memory.h>
#include <stdio.h>

#include "common.h"
#include "clipbook.h"
#include "clipbrd.h"
#include "clipdsp.h"
#include "cvinit.h"
#include "cvutil.h"
#include "dib.h"
#include "strtok.h"
#include "initmenu.h"
#include "debugout.h"



DWORD   gXERR_Type      = 0;
DWORD   gXERR_Err       = 0;
HSZ     hszErrorRequest = 0;





#if DEBUG
static void DumpDataReq (PDATAREQ pdr);


static void DumpDataReq(
    PDATAREQ    pdr)
{
    PINFO(TEXT("Datareq: type %d, to window %lx, \r\nat index %u, fDisc=%u, format=%u\r\n"),
               pdr->rqType,
               pdr->hwndMDI,
               pdr->iListbox,
               pdr->fDisconnect,
               pdr->wFmt);
}
#else
#define DumpDataReq(x)
#endif







// AdjustControlSizes //////////////////
//
// This function sizes the listbox windows associated
// with an MDI child window when its size changes

VOID AdjustControlSizes (
    HWND    hwnd)
{
RECT        rc1, rc2;
PMDIINFO    pMDI;
int         cx = GetSystemMetrics ( SM_CXVSCROLL );
int         cy = GetSystemMetrics ( SM_CYHSCROLL );



    if (!(pMDI = GETMDIINFO(hwnd)))
        return;


    GetClientRect ( hwnd, &rc1 );
    rc2 = rc1;
    rc2.right -= cx - 1;
    rc2.bottom -= cy - 1;

    switch ( pMDI->DisplayMode )
        {
        case DSP_LIST:
        case DSP_PREV:
            MoveWindow ( pMDI->hWndListbox, rc1.left - 1, rc1.top - 1,
               rc1.right - rc1.left + 2, ( rc1.bottom - rc1.top ) + 2, TRUE );
            break;

        case DSP_PAGE:
            MoveWindow (pMDI->hwndHscroll,
                        rc1.left - 1,
                        rc2.bottom,
                        (rc2.right - rc2.left) +2,
                        cy,
                        TRUE );

            if ( pMDI->flags & F_CLPBRD ) {
               MoveWindow ( pMDI->hwndVscroll, rc2.right, rc1.top - 1,
                  cx, ( rc2.bottom - rc2.top ) + 2, TRUE );
               }
            else
               {
               MoveWindow ( pMDI->hwndVscroll, rc2.right, rc1.top - 1,
                  cx, ( rc2.bottom - rc2.top ) + 2 - 2*cy, TRUE );
               }
            MoveWindow ( pMDI->hwndSizeBox,  rc2.right, rc2.bottom, cx, cy, TRUE );

            if ( ! ( pMDI->flags & F_CLPBRD ) )
               {
               MoveWindow ( pMDI->hwndPgUp, rc2.right,
                  rc2.bottom + 1 - 2*cy, cx, cy, TRUE );
               MoveWindow ( pMDI->hwndPgDown, rc2.right,
                  rc2.bottom + 1 - cy, cx, cy, TRUE );
               }

            // adjust display window
            pMDI->rcWindow = rc2;
            break;
        }

}






VOID ShowHideControls (
    HWND    hwnd)
{
PMDIINFO    pMDI;
int         nShowScroll;
int         nShowList;


    if (!(pMDI = GETMDIINFO(hwnd)))
        return;


    switch ( pMDI->DisplayMode )
        {
        case DSP_PREV:
        case DSP_LIST:
           nShowScroll = SW_HIDE;
           nShowList = SW_SHOW;
           break;

        case DSP_PAGE:
           if ( GetBestFormat( hwnd, pMDI->CurSelFormat) != CF_OWNERDISPLAY )
              nShowScroll = SW_SHOW;
           else
              {
              nShowScroll = SW_HIDE;
              ShowScrollBar ( hwnd, SB_BOTH, TRUE );
              }
           nShowList = SW_HIDE;
           break;
        }


   ShowWindow ( pMDI->hWndListbox, nShowList );
   ShowWindow ( pMDI->hwndVscroll, nShowScroll );
   ShowWindow ( pMDI->hwndHscroll, nShowScroll );
   ShowWindow ( pMDI->hwndSizeBox, nShowScroll );
   ShowWindow ( pMDI->hwndPgUp,    (pMDI->flags & F_CLPBRD)? SW_HIDE: nShowScroll );
   ShowWindow ( pMDI->hwndPgDown,  (pMDI->flags & F_CLPBRD)? SW_HIDE: nShowScroll );

}








// AssertConnection /////////////////

BOOL AssertConnection (
    HWND    hwnd)
{
PMDIINFO    pMDI;

    if (!(pMDI = GETMDIINFO(hwnd)))
        return FALSE;


    if (IsWindow(hwnd))
       {
       if (pMDI->hExeConv ||
           (pMDI->hExeConv = InitSysConv (hwnd,
                                          pMDI->hszConvPartner,
                                          hszClpBookShare,
                                          FALSE))
          )
          {
          return TRUE;
          }
       }
    return FALSE;
}










// InitSysConv ////////////////////////
//
// Purpose: Establishes a conversation with the given app and topic.
//
// Parameters:
//    hwnd      - MDI child window to own this conversation
//    hszApp    - App name to connect to
//    hszTopic  - Topic to connect to
//    fLocal    - Ignored.
//
// Returns: Handle to the conversation (0L if no conv. could be established).
//

HCONV InitSysConv (
    HWND    hwnd,
    HSZ     hszApp,
    HSZ     hszTopic,
    BOOL    fLocal )
{
HCONV       hConv = 0L;
PDATAREQ    pDataReq;
DWORD       dwErr;


#if DEBUG
TCHAR       atchApp[256];
TCHAR       atchTopic[256];

    if (DdeQueryString(idInst, hszApp, atchApp,
             sizeof(atchApp), CP_WINANSI) &&
        DdeQueryString(idInst, hszTopic, atchTopic,
             sizeof(atchTopic), CP_WINANSI))
       {
       PINFO(TEXT("InitSysConv: [%s | %s]\r\n"), atchApp, atchTopic);
       }
    else
       {
       PERROR(TEXT("I don't know my app/topic pair!\r\n"));
       }
#endif




    if (LockApp (TRUE, szEstablishingConn))
        {
        hConv = DdeConnect ( idInst, hszApp, hszTopic, NULL );
        if (!hConv)
            {
            dwErr = DdeGetLastError(idInst);
            PINFO(TEXT("Failed first try at CLIPSRV, #%x\r\n"), dwErr);

            if (GetSystemMetrics(SM_REMOTESESSION) )
                {
                MessageBoxID (hInst, hwnd, IDS_TSNOTSUPPORTED, IDS_APPNAME, MB_OK | MB_ICONHAND);
                }
            else
                {
                MessageBoxID (hInst, hwnd, IDS_NOCLPBOOK, IDS_APPNAME, MB_OK | MB_ICONHAND);
                }
            }
        else
            {
            PINFO(TEXT("Making datareq."));

            if ( pDataReq = CreateNewDataReq() )
                {
                pDataReq->rqType  = RQ_EXECONV;
                pDataReq->hwndMDI = hwnd;
                pDataReq->wFmt    = CF_TEXT;
                DdeSetUserHandle ( hConv, (DWORD)QID_SYNC, (DWORD_PTR)pDataReq );

                Sleep(3000);
                PINFO(TEXT("Entering AdvStart transaction "));

                if (!MySyncXact ( NULL, 0L, hConv, hszTopics,
                         CF_TEXT, XTYP_ADVSTART, LONG_SYNC_TIMEOUT, NULL ))
                    {
                    XactMessageBox (hInst, hwnd, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
                    }
                }
            else
                {
                PERROR(TEXT("InitSysConv:Could not create data req\r\n"));
                }
            }
        LockApp ( FALSE, szNull );
        }
    else
        {
        PERROR(TEXT("app locked in initsysconv\n\r"));
        }

    return hConv;

}







// UpdateListBox ////////////////////////////
//
// This function updates the contents of a listbox
// given the window handle of the MDI child window
// and the conversation over which the data is to be
// obtained

BOOL UpdateListBox(
    HWND    hwnd,
    HCONV   hConv)
{
HDDEDATA    hData;
BOOL        fOK = TRUE;



    if ( hConv == 0L || !IsWindow( hwnd ))
        {
        PERROR(TEXT("UpdateListBox called with garbage\n\r"));
        fOK = FALSE;
        }
    else
        {
        if (GETMDIINFO(hwnd) && GETMDIINFO(hwnd)->flags & F_LOCAL)
            {
            PINFO(TEXT("Getting all topics\r\n"));
            }
        else
            {
            PINFO(TEXT("Getting shared topics\r\n"));
            }



        // ask clipsrv to initialize shares

        MySyncXact (SZCMD_INITSHARE,
                    sizeof (SZCMD_INITSHARE),
                    hConv,
                    0L,
                    CF_TEXT,
                    XTYP_EXECUTE,
                    SHORT_SYNC_TIMEOUT,
                    NULL);



        //get the data

        hData = MySyncXact (NULL,
                            0L,
                            hConv,
                            hszTopics,
                            CF_TEXT,
                            XTYP_REQUEST,
                            SHORT_SYNC_TIMEOUT,
                            NULL );

        if ( !hData )
            {
            XactMessageBox (hInst,
                            hwnd,
                            IDS_APPNAME,
                            MB_OK | MB_ICONEXCLAMATION);
            fOK = FALSE;
            }
        else
            {
            fOK =  InitListBox ( hwnd, hData );
            }
        }

    return fOK;

}







// GetPreviewBitmap //////////////////////////////
// Informs CLIPSRV via DDE that we need a preview bitmap
// for the given page.
//
// Parameters:
//    hwnd -   Clipbook window which wants the bitmap
//    szName - Name of the clipbook page.
//    index  - Page's index within the listbox in hwnd
//
// Returns:
//    void.
//

BOOL GetPreviewBitmap (
    HWND    hwnd,
    LPTSTR  szName,
    UINT    index)
{
HSZ         hszTopic, hszItem = 0L;
HCONV       hConv;
HDDEDATA    hRet;
PDATAREQ    pDataReq;
BOOL        fLocked;
TCHAR       tchTmp;



    if (WAIT_TIMEOUT == WaitForSingleObject (hXacting, 0))
        return FALSE;



    fLocked = LockApp (TRUE, NULL);



    tchTmp = szName[0];
    szName[0] = SHR_CHAR;

    if (0 == (hszTopic = DdeCreateStringHandle (idInst, szName, 0)))
        {
        PERROR(TEXT("GetPreviewBitmap: no topic handle\n\r"));
        goto done;
        }


    if (0 == (hszItem = DdeCreateStringHandle (idInst, SZPREVNAME, 0)))
        {
        PERROR(TEXT("GetPreviewBitmap: no item handle\n\r"));
        goto done;
        }


    if (!GETMDIINFO(hwnd))
        {
        PERROR(TEXT("GETMDIINFO(hwnd) -> NULL\n\r"));
        goto done;
        }


    if (NULL == (pDataReq = CreateNewDataReq()))
        {
        PERROR(TEXT("GetPreviewBitmap: no pdatareq\n\r"));
        goto done;
        }


    #if DEBUG
    {
    TCHAR atch[64];

    DdeQueryString(idInst, GETMDIINFO(hwnd)->hszConvPartnerNP,
          atch, 64, CP_WINANSI);
    PINFO(TEXT("GetPrevBmp: Connecting [%s | %s ! %s]\r\n"),
          atch, szName, SZPREVNAME);
    }
    #endif





    //
    // Let's try to connect up to ten times.  Sometimes when updating
    // the thumbnails if the user changes a page, the server will be
    // busy doing that and we can't connect here.  So, at least try
    // a few times.
    //
    {
    INT trycnt = 0;
    hConv = 0L;

    while (trycnt < 10 && !hConv)
        {
        hConv = DdeConnect (idInst, GETMDIINFO(hwnd)->hszConvPartnerNP, hszTopic, NULL);
        trycnt++;
        if (hConv) continue;

        PINFO (TEXT("GetPreviewBitmap: trying to connect again\r\n"));
        Sleep (200);
        }
    }



    if (hConv)
        {
        DWORD adwTrust[3];
        BOOL  fLocal = FALSE;

        if (GETMDIINFO(hwnd)->flags & F_LOCAL)
            {
            fLocal = TRUE;

            if (NDDE_NO_ERROR !=  NDdeGetTrustedShare(NULL, szName,
                  adwTrust, adwTrust + 1, adwTrust + 2))
                {
                adwTrust[0] = 0L;
                }

            NDdeSetTrustedShare (NULL,
                                 szName,
                                 adwTrust[0] | NDDE_TRUST_SHARE_INIT);
            }

        pDataReq->rqType      = RQ_PREVBITMAP;
        pDataReq->hwndList    = GETMDIINFO(hwnd)->hWndListbox;
        pDataReq->iListbox    = index;
        pDataReq->hwndMDI     = hwnd;
        pDataReq->fDisconnect = TRUE;
        pDataReq->wFmt        = (WORD)cf_preview;
        pDataReq->wRetryCnt   = 3;




        {
        /****   disable all edit function   ****/
        /**** will enable in after callback ****/

        // If the user does a paste or make some changes to the pages while
        // clipbrd is waiting for the xaction to complete, sometimes we get
        // a popup says there's a problem with connection (or something similar)
        // It seems there's some dirty code is causing this.  Below is a temp
        // fix which works well on fast machines.  On slower machines it may
        // still fail at times.  A better fix may be not to use async at all.
        // But there's not enough time to do this right now.
        //
        // NOTE: If there's multiple requests, one may complete while we're still
        // waitng for another.  This will cause the EDIT functions to be enabled
        // while we are still waiting.                          6/29/94 JyF.


        HANDLE hmenu;

        hmenu = GetMenu (hwndApp);

        EnableMenuItem (hmenu, IDM_COPY,       MF_GRAYED | MF_BYCOMMAND);
        EnableMenuItem (hmenu, IDM_KEEP,       MF_GRAYED | MF_BYCOMMAND);
        EnableMenuItem (hmenu, IDM_PASTE_PAGE, MF_GRAYED | MF_BYCOMMAND);
        EnableMenuItem (hmenu, IDM_DELETE,     MF_GRAYED | MF_BYCOMMAND);

        SendMessage (hwndToolbar, TB_ENABLEBUTTON, IDM_COPY,   FALSE);
        SendMessage (hwndToolbar, TB_ENABLEBUTTON, IDM_KEEP,   FALSE);
        SendMessage (hwndToolbar, TB_ENABLEBUTTON, IDM_DELETE, FALSE);
        }


        hRet = DdeClientTransaction (NULL,
                                     0L,
                                     hConv,
                                     hszItem,
                                     cf_preview,
                                     XTYP_REQUEST,
                                     (DWORD)TIMEOUT_ASYNC,
                                     NULL);

        if ( !hRet )
            {
            unsigned uiErr;

            uiErr = DdeGetLastError (idInst);
            PERROR(TEXT("GetPreviewBitmap: Async Transaction for (%s) failed:%x\n\r"),
               szName, uiErr);
            }


        DdeSetUserHandle ( hConv, (DWORD)QID_SYNC, (DWORD_PTR)pDataReq );
        }
    #if DEBUG
    else
        {
        unsigned uiErr;

        uiErr = DdeGetLastError(idInst);
        DdeQueryString(idInst, GETMDIINFO(hwnd)->hszConvPartner,
           szBuf, 128, CP_WINANSI );
        PERROR(TEXT("GetPreviewBitmap: connect to %lx|%lx (%s|%s) failed: %d\n\r"),
           GETMDIINFO(hwnd)->hszConvPartner, hszTopic,
           (LPTSTR)szBuf, (LPTSTR)szName, uiErr);
        }
    #endif





done:

    if (!hszTopic)
        DdeFreeStringHandle (idInst, hszTopic);

    if (!hszItem)
        DdeFreeStringHandle ( idInst, hszItem );

    if (fLocked)
        LockApp (FALSE, NULL);

    szName[0] = tchTmp;



    SetEvent (hXacting);

    return TRUE;

}









VOID SetBitmapToListboxEntry (
    HDDEDATA    hbmp,
    HWND        hwndList,
    UINT        index)
{
LPLISTENTRY lpLE;
RECT        rc;
HBITMAP     hBitmap;
LPBYTE      lpBitData;
DWORD       cbDataLen;
unsigned    uiErr;


#if DEBUG
uiErr = DdeGetLastError(idInst);
if (uiErr)
    {
    PINFO(TEXT("SBmp2LBEntr: %d\r\n"), uiErr);
    }
#endif




    if (!IsWindow (hwndList)
        || SendMessage (hwndList, LB_GETTEXT,     index, (LPARAM)(LPCSTR)&lpLE) == LB_ERR
        || SendMessage (hwndList, LB_GETITEMRECT, index, (LPARAM)(LPRECT)&rc)   == LB_ERR)
        {
        DdeFreeDataHandle(hbmp);
        PERROR(TEXT("SetBitmapToListboxEntry: bad window: %x\n\r"), hwndList);
        }
    else
        {
        if (hbmp)
            {
            if ( lpBitData = DdeAccessData ( hbmp, &cbDataLen ))
                {
                // create the preview bitmap
                hBitmap  = CreateBitmap (PREVBMPSIZ,PREVBMPSIZ,1,1, lpBitData);
                DdeUnaccessData ( hbmp );
                }
            else
                {
                PERROR(TEXT("SB2LB: Couldn't access data!\r\n"));
                hBitmap = NULL;
                }

            DdeFreeDataHandle ( hbmp );
            lpLE->hbmp = hBitmap;

            PINFO(TEXT("Successfully set bmp.\r\n"));
            }

        PINFO(TEXT("Invalidating (%d,%d)-(%d,%d)\r\n"),rc.left, rc.top,
              rc.right, rc.bottom);
        InvalidateRect ( hwndList, &rc, TRUE );
        }


    uiErr = DdeGetLastError(idInst);
    if (uiErr)
        {
        PINFO (TEXT("SBmp2LBEntr: exit err %d\r\n"), uiErr);
        }

}








/*
 *      UpdatePage
 *
 *  When user paste into an existing page, the first item
 *  in szList is the share name of the page pasted. Since
 *  the name did not change, we need to do some special
 *  processing to update the display.
 *
 */

BOOL    UpdatePage (HWND hwnd, LPTSTR szList)
{
PMDIINFO    pMDI;
PLISTENTRY  pLE;
TCHAR       szPageBuf[MAX_NDDESHARENAME+1];
LPTSTR      szPage = szPageBuf;
RECT        Rect;
INT         i;



    *szPage = TEXT('\0');


    // does the first item in szList spcifies
    // an updated page?

    if (BOGUS_CHAR != *szList)
        return FALSE;



    // get the share name

    szList++;

    while (*szList && TEXT('\t') != *szList)
        *szPage++ = *szList++;

    *szPage = TEXT('\0');






    // Find the page, notice the name comparison below does not
    // compare the first char.  This is because the updated page's
    // share state may have changed so the first char won't match.

    pMDI = GETMDIINFO(hwnd);

    for (i=0;
         LB_ERR != SendMessage(pMDI->hWndListbox, LB_GETTEXT, i, (LPARAM)&pLE);
         i++)
        {
        if (pLE)
            if (!lstrcmpiA(pLE->name+1, szPageBuf+1))
                {
                goto update;
                }
        }


    return FALSE;




update:

    // invalidate the preview bitmap

    SendMessage (pMDI->hWndListbox, LB_GETITEMRECT, i, (LPARAM)&Rect);

    if (pLE->hbmp)
        DeleteObject (pLE->hbmp);

    pLE->fTriedGettingPreview = FALSE;
    pLE->hbmp = NULL;

    InvalidateRect (pMDI->hWndListbox, &Rect, FALSE);




    // if in page view and the page is the one currently
    // selected then update the page view

    if (DSP_PAGE == pMDI->DisplayMode)
        if (SendMessage (pMDI->hWndListbox, LB_GETCURSEL, 0, 0) == i)
            PostMessage (hwndApp, WM_COMMAND, IDM_UPDATE_PAGEVIEW, 0L);




    return TRUE;

}







// InitListBox //////////////////////////////////
//
// this function initializes the entries of a listbox
// given the handle of the MDI child window that owns
// the list box and a ddeml data handle that contains the
// tab-separated list of items that are to appear in the
// listbox
// BUGBUG: Right now, this deletes all entries in the list and
//    then recreates them. It would be more efficient to add or
//    delete only those items that have changed. This would save
//    CONSIDERABLE time in thumbnail mode-- now, we have to
//    establish a new DDE conversation with the server for each
//    page, just to get the thumbnail bitmap.


BOOL InitListBox (
    HWND        hwnd,
    HDDEDATA    hData )
{
PMDIINFO    pMDI;
PLISTENTRY  pLE;
LPTSTR      lpszList, q;
DWORD       cbDataLen;
HWND        hwndlist;
int         OldCount;
int         NewCount;
int         OldSel;
LPTSTR      OldSelString;
int         i;
BOOL        fDel;




    if ( hData == 0L || !IsWindow ( hwnd ) )
       {
       PERROR(TEXT("InitListBox called with garbage\n\r"));
       return FALSE;
       }



    // Get a copy of the data in the handle
    lpszList = (LPTSTR)DdeAccessData ( hData, &cbDataLen );
    DdeUnaccessData(hData);
    lpszList = GlobalAllocPtr(GHND, cbDataLen);
    DdeGetData(hData, lpszList, cbDataLen, 0L);

    // Sometimes, the data will be longer than the string. This
    // would make the 'put tabs back' code below fail if we didn't
    // do this.
    cbDataLen = lstrlen(lpszList);

    PINFO(TEXT("InitLB: %s \r\n"), lpszList);

    if (!lpszList)
        {
        PERROR(TEXT("error accessing data in InitListBox\n\r"));
        return FALSE;
        }


    if (!(pMDI = GETMDIINFO(hwnd)))
        return FALSE;



    if (!(hwndlist = GETMDIINFO(hwnd)->hWndListbox))
        return FALSE;





    SendMessage ( hwndlist, WM_SETREDRAW, 0, 0L );





    // let's update the page that was pasted into
    // an existing page.

    UpdatePage (hwnd, lpszList);






    OldCount = (int)SendMessage ( hwndlist, LB_GETCOUNT, 0, 0L );
    OldSel = (int)SendMessage ( hwndlist, LB_GETCURSEL, 0, 0L );
    OldSelString = (LPTSTR)SendMessage (hwndlist, LB_GETITEMDATA, OldSel, 0);
    // SendMessage ( hwndlist, (UINT)LB_RESETCONTENT, 0, 0L );





    // Delete items in list that don't exist anymore
    for (i = 0; i < OldCount; i++)
        {
        SendMessage (hwndlist, LB_GETTEXT, i, (LPARAM)&pLE);
        fDel = TRUE;

        if (pLE)
            {
            for (q = strtokA(lpszList, "\t"); q; q = strtokA(NULL, "\t"))
                {
                PINFO(TEXT("<%hs>"), q);

                if (0 == lstrcmpA(pLE->name, q))
                   {
                   fDel = FALSE;
                   *q = BOGUS_CHAR;
                   break;
                   }
                }
            PINFO(TEXT("\r\n"));

            // Put back the tab chars that strtok ripped out
            for (q = lpszList;q < lpszList + cbDataLen;q++)
                {
                if ('\0' == *q)
                   {
                   *q = '\t';
                   }
                }
            *q = '\0';
            PINFO(TEXT("Restored %hs\r\n"), lpszList);

            if (fDel)
                {
                PINFO(TEXT("Deleting item %s at pos %d\r\n"), pLE->name, i);
                pLE->fDelete = TRUE;
                SendMessage(hwndlist, LB_DELETESTRING, i, 0L);
                i--;
                if (OldCount)
                   {
                   OldCount--;
                   }
                }
            }
        else
            {
            PERROR(TEXT("Got NULL pLE!\r\n"));
            }
        }




    // Add new items to list
    for (q = strtokA(lpszList, "\t"); q; q = strtokA(NULL, "\t"))
       {
       // only add shared items if remote, never re-add existing items
       if (BOGUS_CHAR != *q &&
           (( GETMDIINFO(hwnd)->flags & F_LOCAL ) || *q == SHR_CHAR ))
          {
          // allocate a new list entry...
          if ( ( pLE = (PLISTENTRY)GlobalAllocPtr ( GHND,
                sizeof ( LISTENTRY ))) != NULL )
             {
             // mark this item to be deleted in WM_DELETEITEM
             pLE->fDelete = TRUE;
             pLE->fTriedGettingPreview = FALSE;

             lstrcpy(pLE->name, q);
             PINFO(TEXT("Adding item %s\r\n"), pLE->name);
             SendMessage(hwndlist, LB_ADDSTRING, 0, (LPARAM)(LPCSTR)pLE);
             }
          }
       }







    // Select the item at the same position we were at

    NewCount = (int)SendMessage (hwndlist, LB_GETCOUNT, 0, 0L);

    if (NewCount)
        if (OldCount == NewCount)
            {
            SendMessage (hwndlist,
                         LB_SETCURSEL,
                         OldSel,
                         0L);
            }
        else if (LB_ERR != (LRESULT)OldSelString)
            {
            SendMessage (hwndlist,
                         LB_SELECTSTRING,
                         OldSel-1,  // we can do this 'cause listbox is sorted
                         (LPARAM)OldSelString);
            }




    SendMessage ( hwndlist, WM_SETREDRAW, 1, 0L );
    UpdateNofMStatus( hwnd );


    if (lpszList)
        GlobalFreePtr(lpszList);

    return TRUE;

}








// MyGetFormat ////////////////////////////
//
// this function returns the UINT ID of the
// format matchine the supplied string. This
// is the reverse of the "getclipboardformatname" function.
//
// Note that the formats &Bitmap, &Picture and Pal&ette exist
// both as predefined windows clipboard formats and as privately
// registered formats. The integer switch passed to this function
// determines whether the instrinsic format or the privately registered
// format ID is returned
//
// GETFORMAT_DONTLIE   return instrinsic format i.e. CF_BITMAP
// GETFORMAT_LIE      return registered format i.e. cf_bitmap

UINT MyGetFormat(
    LPTSTR  szFmt,
    int     mode)
{
TCHAR       szBuf[40];
unsigned    i;
UINT        uiPrivates[] = {CF_BITMAP,
                           CF_METAFILEPICT,
                           CF_PALETTE,
                           CF_ENHMETAFILE,
                           CF_DIB};



    PINFO("\nMyGetFormat [%s] %d:", szFmt, mode);

    for (i = 0;i <= CF_ENHMETAFILE ;i++)
        {
        LoadString(hInst, i, szBuf, 40);
        if (!lstrcmp( szFmt, szBuf))
            {
            if (GETFORMAT_DONTLIE == mode)
                {
                PINFO(TEXT("No-lie fmt %d\r\n"), i);
                }
            else
                {
                unsigned j;

                for (j = 0;j <sizeof(uiPrivates)/sizeof(uiPrivates[0]);j++)
                    {
                    if (i == uiPrivates[j])
                        {
                        i = RegisterClipboardFormat(szBuf);
                        break;
                        }
                    }
                }
            PINFO(TEXT("Format result %d\r\n"), i);
            return(i);
            }
        }

    for (i = CF_OWNERDISPLAY;i <= CF_DSPENHMETAFILE ;i++ )
        {
        LoadString(hInst, i, szBuf, 40);
        if (!lstrcmp( szFmt, szBuf))
            {
            if (GETFORMAT_DONTLIE != mode)
                {
                i = RegisterClipboardFormat(szBuf);
                }
            return(i);
            }
        }

    PINFO(TEXT("Registering format %s\n\r"), szFmt );

    return RegisterClipboardFormat ( szFmt );


}











// HandleOwnerDraw ////////////////////////////////
//
// This function handles drawing of owner draw buttons
// and listboxes in this app.

VOID HandleOwnerDraw(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam)
{
LPDRAWITEMSTRUCT    lpds;
RECT                tmprc;
COLORREF            OldTextColor;
COLORREF            OldBkColor;
COLORREF            BackColor;
COLORREF            TextColor;
HBRUSH              hBkBrush;
DWORD               cbData = 0L;
LPLISTENTRY         lpLE;
BOOL                fSel = FALSE;



   lpds = (LPDRAWITEMSTRUCT) lParam;

   // this section handles listbox drawing

    switch ( lpds->CtlID )
        {
        case ID_LISTBOX:
            if (!GETMDIINFO(hwnd))
                 break;

            if ( GETMDIINFO(hwnd)->DisplayMode == DSP_LIST )
                {
                if ( lpds->itemAction & (ODA_DRAWENTIRE|ODA_SELECT|ODA_FOCUS))
                    {
                    if ( SendMessage ( GETMDIINFO(hwnd)->hWndListbox, LB_GETTEXT,
                          lpds->itemID, (LPARAM)(LPCSTR)&lpLE ) == LB_ERR )
                        {
                        return;
                        }

                    hOldBitmap = SelectObject ( hBtnDC, hbmStatus );

                    tmprc = lpds->rcItem;

                    if ( lpds->itemState & ODS_SELECTED &&
                                lpds->itemState & ODS_FOCUS )
                        {
                        TextColor = GetSysColor ( COLOR_HIGHLIGHTTEXT );
                        BackColor = GetSysColor ( COLOR_HIGHLIGHT );
                        }
                    else
                        {
                        TextColor = GetSysColor ( COLOR_WINDOWTEXT );
                        BackColor = GetSysColor ( COLOR_WINDOW );
                        }

                    OldTextColor = SetTextColor ( lpds->hDC, TextColor );
                    OldBkColor = SetBkColor ( lpds->hDC, BackColor );

                    hBkBrush = CreateSolidBrush ( BackColor );
                    if ( hBkBrush )
                       FillRect ( lpds->hDC, &tmprc, hBkBrush );
                    DeleteObject ( hBkBrush );

                    hOldFont = SelectObject ( lpds->hDC, hFontPreview );



                    TextOut (lpds->hDC,
                             lpds->rcItem.left + 2 * LSTBTDX,
                             lpds->rcItem.top+1,
                             &(lpLE->name[1]),
                             lstrlen((lpLE->name)) - 1);

                    SelectObject ( lpds->hDC, hOldFont );

                    if ( IsShared( lpLE ) && fShareEnabled )
                        {
                        BitBlt ( lpds->hDC, lpds->rcItem.left + ( LSTBTDX / 2 ),
                           lpds->rcItem.top, LSTBTDX, LSTBTDY,
                           hBtnDC,
                           SHR_PICT_X,
                           SHR_PICT_Y +
                           (( lpds->itemState & ODS_SELECTED ) &&
                            ( lpds->itemState & ODS_FOCUS ) ? 0 : LSTBTDY ),
                           SRCCOPY );
                        }
                    else
                        {
                        BitBlt ( lpds->hDC, lpds->rcItem.left + ( LSTBTDX / 2 ),
                           lpds->rcItem.top, LSTBTDX, LSTBTDY,
                           hBtnDC,
                           SAV_PICT_X,
                           SAV_PICT_Y +
                           (( lpds->itemState & ODS_SELECTED ) &&
                            ( lpds->itemState & ODS_FOCUS ) ? 0 : LSTBTDY ),
                           SRCCOPY );
                        }

                    SelectObject ( hBtnDC, hOldBitmap );
                    SetTextColor ( lpds->hDC, OldTextColor );
                    SetBkColor ( lpds->hDC, OldBkColor );

                    if ( lpds->itemAction & ODA_FOCUS &&
                       lpds->itemState & ODS_FOCUS )
                        {
                        DrawFocusRect ( lpds->hDC, &(lpds->rcItem) );
                        }
                    }
                }
            else if ( GETMDIINFO(hwnd)->DisplayMode == DSP_PREV )
                {
                if ( lpds->itemAction & ODA_FOCUS )
                    {
                    DrawFocusRect ( lpds->hDC, &(lpds->rcItem) );
                    }

                if ( SendMessage ( GETMDIINFO(hwnd)->hWndListbox, LB_GETTEXT,
                      lpds->itemID, (LPARAM)(LPCSTR)&lpLE ) == LB_ERR )
                    {
                    //PERROR(TEXT("weird, WM_DRAWITEM for empty listbox\n\r");
                    // not weird, for empty focus rect
                    return;
                    }

                if ( lpds->itemAction & ODA_DRAWENTIRE )
                    {

                    // erase any bogus leftover focusrect
                    // due to what I consider ownerdraw bugs
                    if ( hBkBrush = CreateSolidBrush ( GetSysColor(COLOR_WINDOW)))
                        {
                        FillRect ( lpds->hDC, &(lpds->rcItem), hBkBrush );
                        DeleteObject ( hBkBrush );
                        }

                    tmprc.top    = lpds->rcItem.top + PREVBRD;
                    tmprc.bottom = lpds->rcItem.top + PREVBRD + PREVBMPSIZ;
                    tmprc.left   = lpds->rcItem.left + 5 * PREVBRD;
                    tmprc.right  = lpds->rcItem.right - 5 * PREVBRD;

                    Rectangle (lpds->hDC,
                               tmprc.left,
                               tmprc.top,
                               tmprc.right,
                               tmprc.bottom );

                    // draw preview bitmap if available
                    if (lpLE->hbmp == NULL)
                        {
                        if (!lpLE->fTriedGettingPreview)
                            {
                            if (!GetPreviewBitmap (hwnd,
                                                   lpLE->name,
                                                   lpds->itemID))
                                {
                                lpLE->fTriedGettingPreview = FALSE;

                                InvalidateRect (lpds->hwndItem,
                                                &(lpds->rcItem),
                                                FALSE);
                                break;
                                }
                            else
                                {
                                lpLE->fTriedGettingPreview = TRUE;
                                }
                            }
                        else
                            {
                            DrawIcon ( lpds->hDC,
                               // the magic '19' below is a function of the icon
                                  tmprc.left + PREVBMPSIZ - 19,
                                  tmprc.top,
                                  hicLock);
                            }
                        }
                    else
                        {
                        hOldBitmap = SelectObject ( hBtnDC, lpLE->hbmp );
                        BitBlt ( lpds->hDC, tmprc.left+1, tmprc.top+1,
                              ( tmprc.right - tmprc.left ) - 2,
                              ( tmprc.bottom - tmprc.top ) - 2,
                              hBtnDC, 0, 0, SRCCOPY );
                        SelectObject ( hBtnDC, hOldBitmap );
                        }

                    // draw share icon in corner...

                    if ( IsShared ( lpLE ) && fShareEnabled )
                        {
                        DrawIcon (lpds->hDC,
                                  tmprc.left - 10,
                                  tmprc.top + PREVBMPSIZ - 24,
                                  LoadIcon ( hInst, MAKEINTRESOURCE(IDSHAREICON)));
                       }
                    }

                if ( lpds->itemAction & ( ODA_SELECT | ODA_DRAWENTIRE | ODA_FOCUS ))
                    {
                    tmprc = lpds->rcItem;
                    tmprc.left += PREVBRD;
                    tmprc.right -= PREVBRD;
                    tmprc.top += PREVBMPSIZ + 2 * PREVBRD;
                    tmprc.bottom--;

                    if ((lpds->itemState & ODS_SELECTED) &&
                        (lpds->itemState & ODS_FOCUS))
                        {
                        TextColor = GetSysColor ( COLOR_HIGHLIGHTTEXT );
                        BackColor = GetSysColor ( COLOR_HIGHLIGHT );
                        }
                    else
                        {
                        TextColor = GetSysColor ( COLOR_WINDOWTEXT );
                        BackColor = GetSysColor ( COLOR_WINDOW );
                        }

                    OldTextColor = SetTextColor ( lpds->hDC, TextColor );
                    OldBkColor = SetBkColor ( lpds->hDC, BackColor );
                    hOldFont = SelectObject ( lpds->hDC, hFontPreview );

                    if ( hBkBrush = CreateSolidBrush ( BackColor ))
                        {
                        FillRect ( lpds->hDC, &tmprc, hBkBrush );
                        DeleteObject ( hBkBrush );
                        }


                    DrawText (lpds->hDC,
                              &(lpLE->name[1]),
                              lstrlen(lpLE->name) -1,
                              &tmprc,
                              DT_CENTER | DT_WORDBREAK | DT_NOPREFIX );

                    SetTextColor ( lpds->hDC, OldTextColor );
                    SetBkColor ( lpds->hDC, OldBkColor );
                    SelectObject ( lpds->hDC, hOldFont );
                    }
                }
            break;

        case ID_PAGEUP:
        case ID_PAGEDOWN:

            if (lpds->itemAction & (ODA_SELECT | ODA_DRAWENTIRE))
                {
                if (lpds->itemState & ODS_SELECTED)
                    hOldBitmap = SelectObject (hBtnDC,
                                               (lpds->CtlID==ID_PAGEUP)? hPgUpDBmp: hPgDnDBmp);
                else
                    hOldBitmap = SelectObject (hBtnDC,
                                               (lpds->CtlID==ID_PAGEUP)? hPgUpBmp: hPgDnBmp);

                StretchBlt (lpds->hDC,
                            lpds->rcItem.top,
                            lpds->rcItem.left,
                            GetSystemMetrics (SM_CXVSCROLL),
                            GetSystemMetrics (SM_CYHSCROLL),
                            hBtnDC,
                            0,
                            0,
                            17,     // x and y of resource bitmaps
                            17,
                            SRCCOPY);

                SelectObject (hBtnDC, hOldBitmap);
                }
            break;

        default:
            PERROR(TEXT("spurious WM_DRAWITEM ctlID %x\n\r"), lpds->CtlID );
            break;
        }

}










// CreateNewListBox ///////////////////////////////
//
// this function creates a new ownerdraw listbox in one of
// two styles suitable for this app: multicolumn for the
// preview bitmap display, and single column for the description
// display preceeded by the little clipboard entry icons

HWND CreateNewListBox(
    HWND    hwnd,
    DWORD   style)
{
HWND hLB;


    hLB = CreateWindow (TEXT("listbox"),
                        szNull,
                        WS_CHILD | LBS_STANDARD | LBS_NOINTEGRALHEIGHT | style,
                        0,
                        0,
                        100,
                        100,
                        hwnd,
                        (HMENU)ID_LISTBOX,
                        hInst,
                        0L );

    if ( style & LBS_MULTICOLUMN )
       SendMessage ( hLB, LB_SETCOLUMNWIDTH, PREVBMPSIZ + 10*PREVBRD, 0L );

    return hLB;

}





// SetClipboardFormatFromDDE ///////////////////////////
//
// This function accepts a ddeml data handle and uses the
// data contained in it to set the clipboard data in the specified
// format to the virtual clipboard associated with the supplied MDI
// child window handle. This could be the real clipboard if the MDI
// child window handle refers to the clipboard child window.

BOOL SetClipboardFormatFromDDE(
    HWND     hwnd,
    UINT     uiFmt,
    HDDEDATA hDDE)
{
HANDLE         hBitmap;
HANDLE         hData;
LPBYTE         lpData;
LPBYTE         lpSrc;
BITMAP         bitmap;
HPALETTE       hPalette;
LPLOGPALETTE   lpLogPalette;
DWORD          cbData;
int            err;
BOOL           fOK = FALSE;


    PINFO("SetClpFmtDDE: format %d, handle %ld | ", uiFmt, hDDE);


    // Check for existing errors, clear the error flag
    err = DdeGetLastError(idInst);



    if (err != DMLERR_NO_ERROR)
        {
        PERROR(TEXT("Existing err %x\r\n"), err);
        }


    // get size of data
    if (NULL == (lpSrc = DdeAccessData ( hDDE, &cbData )))
        {
        #if DEBUG
        unsigned i;

        i = DdeGetLastError(idInst);
        PERROR(TEXT("DdeAccessData fail %d on handle %ld\r\n"), i, hDDE);
        #endif
        goto done;
        }




    PINFO(TEXT("%d bytes of data. "), cbData);

    if (!(hData = GlobalAlloc(GHND, cbData)))
        {
        PERROR(TEXT("GlobalAlloc failed\n\r"));
        goto done2;
        }



    if (!(lpData = GlobalLock(hData)))
       {
       PERROR(TEXT("GlobalLock failed\n\r"));
       goto done2;
       }



    memcpy(lpData, lpSrc, cbData);
    GlobalUnlock(hData);



    // As when we write these we have to special case a few of
    // these guys.  This code and the write code should match in terms
    // of the sizes and positions of data blocks being written out.
    switch ( uiFmt )
        {
        case CF_METAFILEPICT:
           {
           HANDLE      hMF;
           HANDLE      hMFP;
           HANDLE      hDataOut =  NULL;
           LPMETAFILEPICT   lpMFP;

           // Create the METAFILE with the bits we read in. */
           lpData = GlobalLock(hData);
           if (hMF = SetMetaFileBitsEx(cbData - sizeof(WIN31METAFILEPICT),
                    lpData + sizeof(WIN31METAFILEPICT)))
              {
              // Alloc a METAFILEPICT header.
              if (hMFP = GlobalAlloc(GHND, (DWORD)sizeof(METAFILEPICT)))
                 {
                 if (!(lpMFP = (LPMETAFILEPICT)GlobalLock(hMFP)))
                    {
                    PERROR(TEXT("Set...FromDDE: GlobalLock failed\n\r"));
                    GlobalFree(hMFP);
                    }
                 else
                    {
                    // Have to set this struct memberwise because it's packed
                    // as a WIN31METAFILEPICT in the data we get via DDE
                    lpMFP->hMF = hMF;      /* Update the METAFILE handle  */
                    lpMFP->xExt =((WIN31METAFILEPICT *)lpData)->xExt;
                    lpMFP->yExt =((WIN31METAFILEPICT *)lpData)->yExt;
                    lpMFP->mm   =((WIN31METAFILEPICT *)lpData)->mm;

                    GlobalUnlock(hMFP);      /* Unlock the header      */
                    hDataOut = hMFP;       /* Stuff this in the clipboard */
                    fOK = TRUE;
                    }
                 }
              else
                 {
                 PERROR(TEXT("SCFDDE: GlobalAlloc fail in MFP, %ld\r\n"),
                       GetLastError());
                 }
              }
           else
              {
              PERROR(TEXT("SClipFDDE: SetMFBitsEx fail %ld\r\n"), GetLastError());
              }
           GlobalUnlock(hData);

           // GlobalFree(hData);

           hData = hDataOut;
           break;
           }

        case CF_ENHMETAFILE:
           // We get a block of memory containing enhmetafile bits in this case.
           if (lpData = GlobalLock(hData))
              {
              HENHMETAFILE henh;

              henh = SetEnhMetaFileBits(cbData, lpData);

              if (NULL == henh)
                 {
                 PERROR(TEXT("SetEnhMFBits fail %d\r\n"), GetLastError());
                 }
              else
                 {
                 fOK = TRUE;
                 }

              GlobalUnlock(hData);
              GlobalFree(hData);

              hData = henh;
              }
           else
              {
              GlobalFree(hData);
              hData = NULL;
              }
           break;

        case CF_BITMAP:
           if (!(lpData = GlobalLock(hData)))
              {
              GlobalFree(hData);
              }
           else
              {
              bitmap.bmType = ((WIN31BITMAP *)lpData)->bmType;
              bitmap.bmWidth = ((WIN31BITMAP *)lpData)->bmWidth;
              bitmap.bmHeight = ((WIN31BITMAP *)lpData)->bmHeight;
              bitmap.bmWidthBytes = ((WIN31BITMAP *)lpData)->bmWidthBytes;
              bitmap.bmPlanes = ((WIN31BITMAP *)lpData)->bmPlanes;
              bitmap.bmBitsPixel = ((WIN31BITMAP *)lpData)->bmBitsPixel;
              bitmap.bmBits = lpData + sizeof(WIN31BITMAP);

              // If this fails we should avoid doing the SetClipboardData()
              // below with the hData check.
              hBitmap = CreateBitmapIndirect(&bitmap);

              GlobalUnlock(hData);
              GlobalFree(hData);
              hData = hBitmap;      // Stuff this in the clipboard

              if (hBitmap)
                 {
                 fOK = TRUE;
                 }
              }
           break;

        case CF_PALETTE:
           if (!(lpLogPalette = (LPLOGPALETTE)GlobalLock(hData)))
              {
              GlobalFree(hData);
              DdeUnaccessData( hDDE );
              DdeFreeDataHandle ( hDDE );
              fOK = FALSE;
              }
           else
              {
              // Create a logical palette.
              if (!(hPalette = CreatePalette(lpLogPalette)))
                 {
                 GlobalUnlock(hData);
                 GlobalFree(hData);
                 }
              else
                 {
                 GlobalUnlock(hData);
                 GlobalFree(hData);

                 hData = hPalette;      // Stuff this into clipboard
                 fOK = TRUE;
                 }
              }
           break;


        case DDE_DIB2BITMAP:

            // convert dib to bitmap
            {
            HBITMAP hBmp;

            hBmp = BitmapFromDib (hData,
                                  VGetClipboardData (GETMDIINFO(hwnd)->pVClpbrd, CF_PALETTE));

            GlobalFree (hData);
            hData = hBmp;

            uiFmt = CF_BITMAP;

            fOK = TRUE;
            break;
            }


        default:
           fOK = TRUE;
        }



    if (!hData)
        {
        PERROR(TEXT("SetClipboardFormatFromDDE returning FALSE\n\r"));
        }


    if (GETMDIINFO(hwnd))
        if (fOK)
            {
            PINFO(TEXT("SCFFDDE: Setting VClpD\r\n"));
            VSetClipboardData( GETMDIINFO(hwnd)->pVClpbrd, uiFmt, hData);
            }
        else if (!(GETMDIINFO(hwnd)->flags & F_CLPBRD))
            {
                VSetClipboardData (GETMDIINFO(hwnd)->pVClpbrd, uiFmt,
                                   INVALID_HANDLE_VALUE);
            }


    // No GlobalFree() call here, 'cause we've put hData on the clp


done2:
    DdeUnaccessData(hDDE);

done:
    DdeFreeDataHandle(hDDE);

    return fOK;

}









// NewWindow /////////////////////////////////////////////
//
// this function creates a new MDI child window. special
// case code detects if the window created is the special case
// clipboard MDI child window or the special case local clipbook
// window, this information is used to size the initial 2 windows
// to be tiled side-by-side


HWND  NewWindow(VOID)
{
HWND hwnd;
MDICREATESTRUCT mcs;

    mcs.szTitle = TEXT("");
    mcs.szClass = szChild;
    mcs.hOwner   = hInst;

    /* Use the default size for the window */

    if ( !hwndClpbrd )
       {
       mcs.style = WS_MINIMIZE;
       }
    else
       {
       mcs.style = 0;
       }
    mcs.x = mcs.cx = CW_USEDEFAULT;
    mcs.y = mcs.cy = CW_USEDEFAULT;

    /* Set the style DWORD of the window to default */

    // note not visible!
    mcs.style |= ( WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_CAPTION |
       WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX |
       WS_SYSMENU );

    /* tell the MDI Client to create the child */
    hwnd = (HWND)SendMessage (hwndMDIClient,
               WM_MDICREATE,
               0,
               (LPARAM)(LPMDICREATESTRUCT)&mcs);

    return hwnd;
}








// AdjustMDIClientSize //////////////////////////////
//
// this function adjusts the size of the MDI client window
// when the application is resized according to whether the
// toolbar/status bar is visible, etc.

VOID AdjustMDIClientSize(VOID)
{
RECT rcApp;
RECT rcMDI;


    //   WINDOWPLACEMENT wpl;

    if (IsIconic(hwndApp))
        return;

    //wpl.length = sizeof(WINDOWPLACEMENT);
    //GetWindowPlacement ( hwndApp, &wpl );

    //if ( wpl.showCmd != SW_SHOWMAXIMIZED )
    //   rcApp = wpl.rcNormalPosition;
    //else
    //   GetClientRect ( hwndApp, &rcApp );


    GetClientRect (hwndApp, &rcApp);



    rcMDI.top    = 0;
    rcMDI.bottom = rcApp.bottom - rcApp.top;
    //      - ( GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU)))
    //      - ( 2 * GetSystemMetrics(SM_CYFRAME));

    rcMDI.left = 0;
    rcMDI.right = rcApp.right - rcApp.left;
    //      - ( 2 * GetSystemMetrics(SM_CXFRAME));

    MoveWindow (hwndMDIClient,
                rcMDI.left - 1,
                rcMDI.top + (fToolBar? (dyButtonBar +1): 0),
                (rcMDI.right - rcMDI.left) + 2,
                ((rcMDI.bottom - rcMDI.top) - (fStatus?dyStatus:0)) -(fToolBar?(dyButtonBar +1):0),
                TRUE);

    if (fNeedToTileWindows )
        {
        SendMessage (hwndMDIClient, WM_MDITILE, 0, 0);
        fNeedToTileWindows = FALSE;
        }
}









// GetConvDataItem ///////////////////////////////////
//
// this function retrieves the data item associated with the
// supplied topic and item from whatever local or remote host
// the MDI child window specified by the supplied handle is
// communicating with. It is used to get the preview bitmaps and
// to get individual format data.
//
// NOTE: caller should LockApp before calling this!

HDDEDATA GetConvDataItem(
    HWND    hwnd,
    LPTSTR  szTopic,
    LPTSTR  szItem,
    UINT    uiFmt)
{
HCONV       hConv;
HSZ         hszTopic;
HSZ         hszItem;
HDDEDATA    hRet = 0;
PMDIINFO    pMDI;



    // assert(fAppLockedState);

    PINFO(TEXT("GConvDI: %s ! %s, %x\r\n"), szTopic, szItem, uiFmt);

    if (!( hszTopic = DdeCreateStringHandle ( idInst, szTopic, 0 )))
        {
        PERROR(TEXT("GetConvDataItem: DdeCreateStringHandle failed\n\r"));
        return 0;
        }

    if (!(hszItem = DdeCreateStringHandle ( idInst, szItem, 0 )))
        {
        DdeFreeStringHandle ( idInst, hszTopic );
        PERROR(TEXT("GetConvDataItem: DdeCreateStringHandle failed\n\r"));
        return 0;
        }


    if (!(pMDI = GETMDIINFO(hwnd)))
        return 0;

    if ( hConv = DdeConnect (idInst,
                             (uiFmt == cf_preview && !(pMDI->flags & F_LOCAL))?
                              pMDI->hszConvPartnerNP:
                              pMDI->hszConvPartner,
                             hszTopic,NULL ))
        {
        hRet = MySyncXact (NULL, 0L, hConv,
                           hszItem, uiFmt, XTYP_REQUEST, SHORT_SYNC_TIMEOUT, NULL );
        if ( !hRet )
           {
           PERROR(TEXT("Transaction for (%s):(%s) failed: %x\n\r"),
              szTopic, szItem, DdeGetLastError(idInst));
           }
        }
    #if DEBUG
    else
        {
        DdeQueryString ( idInst, GETMDIINFO(hwnd)->hszConvPartner,
           szBuf, 128, CP_WINANSI );
        PERROR(TEXT("GetConvDataItem: connect to %s|%s failed: %d\n\r"),
                    (LPTSTR)szBuf,
                    (LPTSTR)szTopic, DdeGetLastError(idInst) );
        }
    #endif


    DdeDisconnect ( hConv );
    DdeFreeStringHandle ( idInst, hszTopic );


    return hRet;

}








//***************************************************************************
//  FUNCTION   : MyMsgFilterProc
//
//  PURPOSE   : This filter proc gets called for each message we handle.
//            This allows our application to properly dispatch messages
//            that we might not otherwise see because of DDEMLs modal
//            loop that is used while processing synchronous transactions.
//
//            Generally, applications that only do synchronous transactions
//            in response to user input (as this app does) does not need
//            to install such a filter proc because it would be very rare
//            that a user could command the app fast enough to cause
//            problems.  However, this is included as an example.

LRESULT  PASCAL MyMsgFilterProc(
    int     nCode,
    WPARAM  wParam,
    LPARAM  lParam)
{



    if (( nCode == MSGF_DIALOGBOX || nCode == MSGF_MENU ) &&
          ((LPMSG)lParam)->message == WM_KEYDOWN &&
          ((LPMSG)lParam)->wParam == VK_F1 )
       {
       PostMessage ( hwndApp, WM_F1DOWN, nCode, 0L );
       }
    else if (nCode == MSGF_DDEMGR)
       {
       /* If a keyboard message is for the MDI , let the MDI client
        * take care of it.  Otherwise, check to see if it's a normal
        * accelerator key.  Otherwise, just handle the message as usual.
        */

       //if (!TranslateMDISysAccel (hwndMDIClient, (LPMSG)lParam) &&
       //    (hAccel? !TranslateAccelerator(hwndApp, hAccel, (LPMSG)lParam): 1))
       //   {
       //   TranslateMessage ((LPMSG)lParam );
       //   DispatchMessage ((LPMSG)lParam );
       //   }
       //return(1);
       }

    return(0);
}










// MySyncXact ///////////////////////////////
//
// this function is a wrapper to DdeClientTransaction which
// performs some checks related to the Locked state of the app

HDDEDATA MySyncXact(
    LPBYTE  lpbData,
    DWORD   cbDataLen,
    HCONV   hConv,
    HSZ     hszItem,
    UINT    wFmt,
    UINT    wType,
    DWORD   dwTimeout,
    LPDWORD lpdwResult)
{
HDDEDATA    hDDE;
BOOL        fAlreadyLocked;
UINT        uiErr;
UINT        DdeErr = 0;
DWORD       dwTmp  = 0;

#if DEBUG
if (dwTimeout != TIMEOUT_ASYNC)
    {
    dwTimeout +=10000;
    }
#endif





    // are we already in transaction?

    if (WAIT_TIMEOUT == WaitForSingleObject (hXacting, 0))
        {
        Sleep (2000);
        ClearInput (hwndApp);
        return 0L;
        }



    fAlreadyLocked = !LockApp(TRUE, NULL);



    gXERR_Type = 0;
    gXERR_Err  = 0;





    hDDE = DdeClientTransaction (lpbData,
                                 cbDataLen,
                                 hConv,
                                 hszItem,
                                 wFmt,
                                 wType,
                                 dwTimeout,
                                 lpdwResult );


    if (!hDDE)
        {
        DWORD   size;
        LPBYTE  lpByte;


        DdeErr = DdeGetLastError(idInst);

        #if DEBUG
        PERROR("MySyncXact fail err %d.\r\n", uiErr);

        DdeQueryString (idInst, hszItem, lpbItem, 64, CP_WINANSI);
        PINFO(TEXT("Parameters: data at %lx (%s), len %ld, HCONV %lx\r\n"),
              lpbData, (CF_TEXT == wFmt && lpbData) ? lpbData : TEXT("Not text"),
              cbDataLen, hConv);

        PINFO(TEXT("item %lx (%s), fmt %d, type %d, timeout %ld\r\n"),
              hszItem, lpbItem, wFmt, wType, dwTimeout);
        #endif


        //
        // There was an error in the transaction, let's ask
        // the server what was it.
        //

        hDDE = DdeClientTransaction (NULL,
                                     0L,
                                     hConv,
                                     hszErrorRequest,
                                     CF_TEXT,
                                     XTYP_REQUEST,
                                     SHORT_SYNC_TIMEOUT,
                                     NULL);

        uiErr = DdeGetLastError (idInst);

        if (lpByte = DdeAccessData (hDDE, &size))
            sscanf (lpByte, XERR_FORMAT, &gXERR_Type, &gXERR_Err);

        DdeUnaccessData (hDDE);
        DdeFreeDataHandle (hDDE);

        hDDE = 0;
        }



    if (!gXERR_Type && DdeErr)
        {
        gXERR_Type = XERRT_DDE;
        gXERR_Err  = DdeErr;
        }



    if (!fAlreadyLocked)
        {
        LockApp(FALSE, NULL);
        }



    SetEvent (hXacting);


    return hDDE;


}









/*
 *      RequestXactError
 *
 *  Ask the server for error code.
 */

void    RequestXactError(
    HCONV   hConv)
{
HDDEDATA    hDDE;
BOOL        fAlreadyLocked;
UINT        uiErr;
UINT        DdeErr = 0;
DWORD       size;
LPBYTE      lpByte;



    // Are we already in transaction?

    if (WAIT_TIMEOUT == WaitForSingleObject (hXacting, 0))
        {
        Sleep (2000);
        ClearInput (hwndApp);
        return;
        }



    fAlreadyLocked = !LockApp(TRUE, NULL);



    gXERR_Type = 0;
    gXERR_Err  = 0;



    DdeErr = DdeGetLastError(idInst);




    hDDE = DdeClientTransaction (NULL,
                                 0L,
                                 hConv,
                                 hszErrorRequest,
                                 CF_TEXT,
                                 XTYP_REQUEST,
                                 SHORT_SYNC_TIMEOUT,
                                 NULL);

    uiErr = DdeGetLastError (idInst);


    if (lpByte = DdeAccessData (hDDE, &size))
        sscanf (lpByte, XERR_FORMAT, &gXERR_Type, &gXERR_Err);

    DdeUnaccessData (hDDE);
    DdeFreeDataHandle (hDDE);




    if (!gXERR_Type && DdeErr)
        {
        gXERR_Type = XERRT_DDE;
        gXERR_Err  = DdeErr;
        }



    if (!fAlreadyLocked)
        {
        LockApp(FALSE, NULL);
        }


    SetEvent (hXacting);


}










// ResetScrollInfo ///////////////////////////
//
// this function resets the scroll information of the
// MDI child window designated by the supplied handle

VOID ResetScrollInfo(
    HWND    hwnd)
{
PMDIINFO pMDI = GETMDIINFO(hwnd);

    if (!pMDI)
        return;

    // Invalidate object info; reset scroll position to 0.
    // cyScrollLast = cxScrollLast = -1;
    pMDI->cyScrollLast = -1L;
    pMDI->cyScrollNow = 0L;
    pMDI->cxScrollLast = -1;
    pMDI->cxScrollNow = 0;

    // Range is set in case CF_OWNERDISPLAY owner changed it.
    PINFO(TEXT("SETSCROLLRANGE for window '%s'\n\r"),
          (LPTSTR)(pMDI->szBaseName) );

    SetScrollRange (pMDI->hwndVscroll, SB_CTL, 0, VPOSLAST, FALSE);
    SetScrollRange (pMDI->hwndHscroll, SB_CTL, 0, HPOSLAST, FALSE);
    SetScrollPos   (pMDI->hwndVscroll, SB_CTL, (int)(pMDI->cyScrollNow), TRUE);
    SetScrollPos   (pMDI->hwndHscroll, SB_CTL, pMDI->cxScrollNow,        TRUE);
}









// IsShared ///////////////////////////////////
//
// this function sets the shared state of the ownerdraw
// listbox entry denoted by the supplied pointer. Shared/nonshared
// status is expressed as a 1 character prefix to the description string
//
// return TRUE if shared, false otherwise

BOOL IsShared(
    LPLISTENTRY lpLE)
{
    if (!lpLE)
        return FALSE;

    if ( lpLE->name[0] == SHR_CHAR )
       return TRUE;

    #if DEBUG
        if ( lpLE->name[0] != UNSHR_CHAR )
            PERROR(TEXT("bad prefix char in share name: %s\n\r"),
                   (LPTSTR)lpLE->name );
    #endif

    return FALSE;
}








// SetShared //////////////////////////////////////////
//
// sets shared state to fShared, returns previous state

BOOL SetShared(
    LPLISTENTRY lpLE,
    BOOL        fShared)
{
BOOL fSave;

    fSave = lpLE->name[0] == SHR_CHAR ? TRUE : FALSE;
    lpLE->name[0] = ( fShared ? SHR_CHAR : UNSHR_CHAR );

    return fSave;
}









// LockApp ////////////////////////////////////////////
//
// this function effectively disables the windows UI during
// synchronous ddeml transactions to prevent the user from initiating
// another transaction or causing the window procedure of this app
// or another application to be re-entered in a way that could cause
// failures... A primary example is that sometimes we are forced to
// go into a ddeml transaction with the clipboard open...  this app
// and other apps must not be caused to access the clipboard during that
// time, so this mechanism emulates the hourglass...
//
// NOTE: do not call LockApp in a section of code where the
// cursor is already captured, such as in response to a scroll
// message, or the releasecapture during unlock will cause strange and
// bad things to happen.


BOOL LockApp(
    BOOL    fLock,
    LPTSTR  lpszComment)
{
static HCURSOR  hOldCursor;
BOOL            fOK = FALSE;


    if (lpszComment)
        {
        SetStatusBarText( lpszComment );
        }

    if ( fLock == TRUE )
        {
        if ( fAppLockedState )
            {
            PERROR(TEXT("LockApp(TRUE): already locked\n\r"));
            }
        else
            {
            hOldCursor = SetCursor ( LoadCursor ( NULL, IDC_WAIT ));

            SetCapture ( hwndDummy );
            EnableWindow ( hwndApp, FALSE );

            fOK = TRUE;
            fAppLockedState = TRUE;
            }
        }
    else
        {
        if ( !fAppLockedState )
            {
            PERROR(TEXT("LockApp(FALSE): not locked\n\r"));
            }
        else
            {

            ClearInput (hwndApp);

            EnableWindow ( hwndApp, TRUE );
            ReleaseCapture ();

            SetCursor ( hOldCursor );

            fOK = TRUE;

            // take care of any deferred clipboard update requests
            if ( fClipboardNeedsPainting )
                {
                PostMessage ( hwndApp, WM_DRAWCLIPBOARD, 0, 0L );
                }

            fAppLockedState = FALSE;
            }
        }

    return fOK;

}








// ForceRenderAll ///////////////////////////////////
//
// this function forces a complete rendering of any delayed
// render clipboard formats
BOOL ForceRenderAll(
    HWND        hwnd,
    PVCLPBRD    pVclp)
{
HANDLE  h;
UINT    uiFmt;

    if ( !VOpenClipboard ( pVclp, hwnd ))
        {
        PERROR(TEXT("Can't open clipboard in ForceRenderAll\n\r"));
        return FALSE;
        }


    for ( uiFmt = VEnumClipboardFormats( pVclp, 0); uiFmt;
          uiFmt = VEnumClipboardFormats( pVclp, uiFmt))
        {
        PINFO(TEXT("ForceRenderAll: force rendering %x\n\r"), uiFmt );
        h = VGetClipboardData ( pVclp, uiFmt );
        }

    VCloseClipboard ( pVclp );
    return TRUE;
}










BOOL UpdateNofMStatus(
    HWND    hwnd)
{
HWND    hwndlistbox;
int     total = 0;
int     sel = LB_ERR;



    if (hwnd == NULL)
        {
        SendMessage ( hwndStatus, SB_SETTEXT, 0, (LPARAM)NULL );
        return TRUE;
        }


    if (!GETMDIINFO(hwnd))
        return FALSE;


    if (GETMDIINFO(hwnd)->flags & F_CLPBRD)
        {
        SendMessage ( hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPTSTR) szSysClpBrd );
        return TRUE;
        }

    if ( IsWindow( hwndlistbox = GETMDIINFO(hwnd)->hWndListbox ) )
        {
        total = (int)SendMessage ( hwndlistbox, LB_GETCOUNT, (WPARAM)0, 0L );
        sel = (int)SendMessage ( hwndlistbox, LB_GETCURSEL, 0, 0L);
        }

    if ( sel == (int)LB_ERR )
        {
        if ( total == 1 )
            SendMessage (hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPCSTR)szPageFmt);
        else
            {
            wsprintf( szBuf, szPageFmtPl, total );
            SendMessage (hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPCSTR)szBuf );
            }
        }
    else
        {
        wsprintf(szBuf, szPageOfPageFmt, sel+1, total );
        SendMessage ( hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPCSTR)szBuf );
        }


    return TRUE;

}








BOOL RestoreAllSavedConnections(void)
{
TCHAR       szName[80];
BOOL        ret = TRUE;
unsigned    i;

    i = lstrlen(szConn);

    if (NULL != hkeyRoot)
        {
        DWORD dwSize = 80;
        DWORD iSubkey = 0;

        while (ERROR_SUCCESS == RegEnumKeyEx(hkeyRoot, iSubkey,
                    szName, &dwSize, NULL, NULL, NULL, NULL) )
            {
            if (0 == memcmp(szName, szConn, i))
                {
                PINFO(TEXT("Restoring connection to '%s'\n\r"), szName + i);

                if ( !CreateNewRemoteWindow ( szName + i, FALSE ) )
                    {
                    TCHAR szWindowName[80];

                    // remove re-connect entry
                    RegDeleteKey(hkeyRoot, szName);

                    lstrcat(lstrcpy(szWindowName, szWindows),
                          szName + i);
                    RegDeleteKey(hkeyRoot, szWindowName);
                    ret = 0;
                    }
                }

            dwSize = 80;
            iSubkey++;
            }
        }

    return ret;

}








BOOL CreateNewRemoteWindow(
    LPTSTR  szMachineName,
    BOOL    fReconnect)
{
WINDOWPLACEMENT wpl;
HWND            hwndc;
PMDIINFO        pMDIc;




    // make new window active
    hwndc = NewWindow();
    if (NULL == hwndc)
       {
       return FALSE;
       }


    if (!(pMDIc = GETMDIINFO(hwndc)))
        return FALSE;


    // save base name for window
    lstrcpy (pMDIc->szBaseName, szMachineName);
    lstrcpy (pMDIc->szComputerName, szMachineName);

    wsprintf ( szBuf, TEXT("%s\\%s"), (LPTSTR)szMachineName, (LPTSTR)szNDDEcode);

    pMDIc->hszConvPartner = DdeCreateStringHandle ( idInst, szBuf, 0 );

    PINFO(TEXT("Trying to talk to %s\r\n"),szBuf);

    wsprintf ( szBuf, TEXT("%s\\%s"), (LPTSTR)szMachineName, (LPTSTR)szNDDEcode1 );
    pMDIc->hszConvPartnerNP = DdeCreateStringHandle ( idInst, szBuf, 0 );

    PINFO(TEXT("NP = %s\r\n"),szBuf);

    #if DEBUG
    DdeQueryString(idInst, hszSystem, szBuf, 128, CP_WINANSI);
    PINFO(TEXT("Topic = %s\r\n"), szBuf);

    PINFO(TEXT("Existing err = %lx\r\n"), DdeGetLastError(idInst));
    #endif

    pMDIc->hExeConv = InitSysConv (hwndc, pMDIc->hszConvPartner, hszClpBookShare, FALSE);



    if ( pMDIc->hExeConv )
       {
       if ( UpdateListBox ( hwndc, pMDIc->hExeConv ))
          {
          wsprintf(szBuf, szClipBookOnFmt, (LPTSTR)(pMDIc->szBaseName) );
          SetWindowText ( hwndc, szBuf );

          if ( ReadWindowPlacement ( pMDIc->szBaseName, &wpl ))
             {
             wpl.length = sizeof(WINDOWPLACEMENT);
             wpl.flags = WPF_SETMINPOSITION;
             SetWindowPlacement ( hwndc, &wpl );
             UpdateWindow ( hwndc );
             }
          else
             {
             ShowWindow ( hwndc, SW_SHOWNORMAL );
             }

          ShowWindow ( pMDIc->hWndListbox, SW_SHOW );
          SendMessage ( hwndMDIClient, WM_MDIACTIVATE, (WPARAM)hwndc, 0L );
          SendMessage ( hwndMDIClient, WM_MDISETMENU, (WPARAM) TRUE, 0L );

          hwndActiveChild = hwndc;
          pActiveMDI = GETMDIINFO(hwndc);

          if ( fReconnect )
             {
             TCHAR szName[80];
             DWORD dwData;

             lstrcat(lstrcpy(szName, szConn), szBuf);

             dwData = pMDIc->DisplayMode == DSP_LIST ? 1 : 2;

             RegSetValueEx(hkeyRoot, szName, 0L, REG_DWORD,
                   (LPBYTE)&dwData, sizeof(dwData));

             PINFO(TEXT("saving connection: '%s'\n\r"), (LPTSTR)szBuf );
             }
          else
             {
             TCHAR szName[80];
             DWORD dwData;
             DWORD dwDataSize = sizeof(dwData);

             lstrcat(lstrcpy(szName, szConn), pMDIc->szBaseName);

             RegQueryValueEx(hkeyRoot, szName, NULL, NULL,
                   (LPBYTE)&dwData, &dwDataSize);

             if (2 == dwData)
                {
                SendMessage ( hwndApp, WM_COMMAND, IDM_PREVIEWS, 0L );
                }
             }

          return TRUE;
          }
       else
          {
          PERROR(TEXT("UpdateListBox failed\n\r"));
          return FALSE;
          }
       }
    else
       {
       unsigned uiErr;

       #if DEBUG
       DdeQueryString(idInst, pMDIc->hszConvPartner, szBuf, 128, CP_WINANSI);
       #endif

       uiErr = DdeGetLastError(idInst);
       PERROR(TEXT("Can't find %s|System. Error #%x\n\r"),(LPTSTR)szBuf, uiErr );
       }


    return FALSE;
}







#define MB_SNDMASK (MB_ICONHAND|MB_ICONQUESTION|MB_ICONASTERISK|MB_ICONEXCLAMATION)

/*
 *      MessageBoxID
 *
 *  Display a message box with strings specified by
 *  TextID and TitleID.
 */

int MessageBoxID(
    HANDLE  hInstance,
    HWND    hwndParent,
    UINT    TextID,
    UINT    TitleID,
    UINT    fuStyle)
{
    LoadString (hInstance, TextID,  szBuf,  SZBUFSIZ);
    LoadString (hInstance, TitleID, szBuf2, SZBUFSIZ);

    MessageBeep (fuStyle & MB_SNDMASK);
    return MessageBox (hwndParent, szBuf, szBuf2, fuStyle);
}






/*
 *      NDdeMessageBox
 *
 *  Display a message box with NDde error
 *  string specified by errCode and title
 *  string specified by TitleID.
 */

int NDdeMessageBox(
    HANDLE  hInstance,
    HWND    hwnd,
    UINT    errCode,
    UINT    TitleID,
    UINT    fuStyle)
{
    if (!errCode)
        return IDOK;

    NDdeGetErrorString (errCode, szBuf, SZBUFSIZ);
    LoadString (hInstance, TitleID, szBuf2, SZBUFSIZ);

    MessageBeep (fuStyle & MB_SNDMASK);
    return MessageBox (hwnd, szBuf, szBuf2, fuStyle);

}






/*
 *      SysMessageBox
 *
 *  Display a messag box for system message
 *  strings specified by dwErr and titl string
 *  specified by TitleID.
 */

int SysMessageBox(
    HANDLE  hInstance,
    HWND    hwnd,
    DWORD   dwErr,
    UINT    TitleID,
    UINT    fuStyle)
{
DWORD   dwR;
LPTSTR  lpBuffer;
DWORD   dwSize = 20;

    if (dwErr == NO_ERROR)
        return IDOK;

    dwR = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER|
                         FORMAT_MESSAGE_FROM_SYSTEM,
                         NULL,
                         dwErr,
                         0,
                         (LPTSTR)&lpBuffer, // dual use param. does not match prototype
                         dwSize,
                         NULL);

    LoadString (hInstance, TitleID, szBuf2, SZBUFSIZ);

    MessageBeep (fuStyle & MB_SNDMASK);
    dwR = MessageBox (hwnd, lpBuffer, szBuf2, fuStyle);

    LocalFree (lpBuffer);

    return dwR;
}






/*
 *      XactMessageBox
 *
 *  Display a message box for error
 *  occured in an transaction.  MySyncXact
 *  must be called to do the transaction
 *  before calling this function.
 */

int XactMessageBox(
    HANDLE  hInstance,
    HWND    hwnd,
    UINT    TitleID,
    UINT    fuStyle)
{

    switch (gXERR_Type)
        {
        case XERRT_NDDE:
            return NDdeMessageBox (hInstance, hwnd, gXERR_Err, TitleID, fuStyle);
        case XERRT_DDE:
            return DdeMessageBox (hInstance, hwnd, gXERR_Err, TitleID, fuStyle);
        case XERRT_SYS:
            return SysMessageBox (hInstance, hwnd, gXERR_Err, TitleID, fuStyle);
        default:
            return IDOK;
        }
}






/*
 *      DdeNessageBox
 *
 *  Displays a message box for DDE
 *  error strings specified by errCode
 *  and title string spcified by TitleID.
 */

int DdeMessageBox(
    HANDLE  hInstance,
    HWND    hwnd,
    UINT    errCode,
    UINT    TitleID,
    UINT    fuStyle)
{
TCHAR szErr[1024];


    switch (errCode)
        {
        case DMLERR_ADVACKTIMEOUT:
        case DMLERR_DATAACKTIMEOUT:
        case DMLERR_EXECACKTIMEOUT:
        case DMLERR_POKEACKTIMEOUT:
        case DMLERR_UNADVACKTIMEOUT:
        case DMLERR_NO_CONV_ESTABLISHED:
            if (hwnd == hwndLocal)
                LoadString (hInstance, IDS_NOCLPBOOK, szBuf, SZBUFSIZ);
            else
                LoadString (hInstance, IDS_DATAUNAVAIL, szBuf, SZBUFSIZ);
            break;

        case DMLERR_NOTPROCESSED:
        case DMLERR_BUSY:
        case DMLERR_DLL_NOT_INITIALIZED:
        case DMLERR_DLL_USAGE:
        case DMLERR_INVALIDPARAMETER:
        case DMLERR_LOW_MEMORY:
        case DMLERR_MEMORY_ERROR:
        case DMLERR_POSTMSG_FAILED:
        case DMLERR_REENTRANCY:
        case DMLERR_SERVER_DIED:
        case DMLERR_SYS_ERROR:
        case DMLERR_UNFOUND_QUEUE_ID:
            LoadString (hInstance, IDS_INTERNALERR, szBuf, SZBUFSIZ);
            break;
        default:
            return IDOK;
        }

    LoadString (hInstance, TitleID, szBuf2, SZBUFSIZ);

    wsprintf (szErr, "%s (%#x)", szBuf, errCode);

    MessageBeep (fuStyle & MB_SNDMASK);
    return MessageBox (hwnd, szErr, szBuf2, fuStyle);

}







/*
 *      ClearInput
 *
 *  Removes all keyboard and mouse messages
 *  from message queue
 */

void    ClearInput (HWND    hWnd)
{
MSG Msg;

    while (PeekMessage (&Msg, hWnd, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE));
    while (PeekMessage (&Msg, hWnd, WM_KEYFIRST,   WM_KEYLAST,   PM_REMOVE));
}






PDATAREQ CreateNewDataReq (void)
{
    return (PDATAREQ) GlobalAlloc (GPTR, sizeof(DATAREQ));
}






BOOL DeleteDataReq(
    PDATAREQ    pDataReq)
{
    return ((HGLOBAL)pDataReq == GlobalFree (pDataReq));
}






//
// Purpose: Handle data returned from CLIPSRV via DDE.
//
// Parameters:
//    hData - The data handle the XTYP_XACT_COMPLETE message gave us,
//            or 0L if we got XTYP_DISCONNECT instead.
//
//    pDataReq - Pointer to a DATAREQ struct containing info about what
//               we wanted the data for. This is gotten via DdeGetUserHandle.
//
// Returns:
//    TRUE on success, FALSE on failure.
//
//////////////////////////////////////////////////////////////////////////

BOOL ProcessDataReq(
    HDDEDATA    hData,
    PDATAREQ    pDataReq)
{
LPLISTENTRY lpLE;
LPSTR       lpwszList;
LPSTR       q;
HCURSOR     hSaveCursor;
DWORD       cbDataLen;
UINT        tmp;
PMDIINFO    pMDI;
UINT        uiErr;
BOOL        bRet = FALSE;



    hSaveCursor = SetCursor (LoadCursor (NULL, IDC_WAIT));



    PINFO("PDR:");

    if ( !pDataReq || !IsWindow(pDataReq->hwndMDI) )
        {
        PERROR(TEXT("ProcessDataReq: bogus DATAREQ\n\r"));
        goto done;
        }


    if (!hData)
        {
        PERROR("ProcessDataReq: Woe, woe, we have gotten null data!\r\n");
        DumpDataReq(pDataReq);


        switch (pDataReq->rqType)
            {
            case RQ_COPY:
                MessageBoxID (hInst, hwndApp, IDS_DATAUNAVAIL, IDS_APPNAME,
                              MB_OK | MB_ICONHAND);
                break;
            case RQ_PREVBITMAP:
                // We still have to display the lock icon.
                SetBitmapToListboxEntry (hData, pDataReq->hwndList, pDataReq->iListbox);
                break;
            }

        goto done;;
        }




    if (!(pMDI = GETMDIINFO(pDataReq->hwndMDI)))
        goto done;






    switch ( pDataReq->rqType )
        {
        case RQ_PREVBITMAP:
           PINFO("Got bitmap for item %d in %x\r\n",pDataReq->iListbox,
                 pDataReq->hwndList);
           SetBitmapToListboxEntry( hData, pDataReq->hwndList, pDataReq->iListbox);
           InitializeMenu (GetMenu (hwndApp));
           bRet = TRUE;
           break;

        case RQ_EXECONV:
           // must be from disconnect
           GETMDIINFO(pDataReq->hwndMDI)->hExeConv = 0L;
           PINFO(TEXT("setting hExeConv NULL!\n\r"));
           break;

        case RQ_COPY:
           PINFO("RQ_COPY:");
           if ( hData == FALSE )
              {
              uiErr = DdeGetLastError (idInst);
              PERROR(TEXT("REQUEST for format list failed: %x\n\r"), uiErr);
              DdeMessageBox (hInst, pDataReq->hwndMDI, uiErr, IDS_APPNAME, MB_OK|MB_ICONEXCLAMATION);
              break;
              }

           lpwszList = DdeAccessData ( hData, &cbDataLen );

           if ( !lpwszList )
              {
              uiErr = DdeGetLastError (idInst);
              DdeMessageBox (hInst, pDataReq->hwndMDI, uiErr, IDS_APPNAME, MB_OK|MB_ICONEXCLAMATION);
              break;
              }

           PINFO(TEXT("formatlist:>%ws<\n\r"), lpwszList );
           // this client now becomes the clipboard owner!!!

           if (SyncOpenClipboard (hwndApp) == TRUE)
              {
              BOOL  bHasBitmap = FALSE;
              BOOL  bLocked;

              // Need to lock app while we fill the clipboard with formats,
              // else hwndClpbrd will try to frantically try to redraw while
              // we're doing it. Since hwndClpbrd needs to openclipboard() to
              // do that, we don't want it to.
              bLocked = LockApp(TRUE, szNull);

              // reset clipboard view format to auto
              pMDI->CurSelFormat = CBM_AUTO;

              EmptyClipboard();

              hwndClpOwner = pDataReq->hwndMDI;
              PINFO(TEXT("Formats:"));

              if (pDataReq->wFmt != CF_TEXT)
                 {
                 PERROR(TEXT("Format %d, expected CF_TEXT!\r\n"), pDataReq->wFmt);
                 }


              for (q = strtokA(lpwszList, "\t");q;q = strtokA(NULL, "\t"))
                 {
                 PINFO(TEXT("[%s] "),q);
                 tmp = MyGetFormat(q, GETFORMAT_DONTLIE);
                 if (0 == tmp)
                    {
                    PERROR(TEXT("MyGetFormat failure!\r\n"));
                    }
                 else
                    {
                    switch (tmp)
                        {
                        case CF_DIB:
                            // DDBitmap can be converted from Dib.
                            SetClipboardData (CF_BITMAP, NULL);
                        default:
                            SetClipboardData (tmp, NULL);
                        }
                    }
                 }

              PINFO("\r\n");

              SyncCloseClipboard();

              if (bLocked)
                 LockApp (FALSE, szNull);


              // Redraw clipboard window.
              if (hwndClpbrd)
                 {
                 InvalidateRect(hwndClpbrd, NULL, TRUE);
                 }
              }
           else
              {
              PERROR(TEXT("ProcessDataReq: unable to open clipboard\n\r"));
              }

           DdeUnaccessData ( hData );
           DdeFreeDataHandle ( hData );
           bRet = TRUE;
           break;

        case RQ_SETPAGE:
           PINFO(TEXT("RQ_SETPAGE:"));

           if ( hData == FALSE )
              {
              uiErr = DdeGetLastError (idInst);
              PERROR(TEXT("vclip: REQUEST for format list failed: %x\n\r"), idInst);
              DdeMessageBox (hInst, pDataReq->hwndMDI, idInst, IDS_APPNAME, MB_OK|MB_ICONEXCLAMATION);
              break;
              }

           if ( SendMessage ( pMDI->hWndListbox,
                 LB_GETTEXT, pDataReq->iListbox,
                 (LPARAM)(LPCSTR)&lpLE) == LB_ERR )
              {
              PERROR(TEXT("IDM_COPY: bad listbox index: %d\n\r"), pDataReq->iListbox );
              break;
              }

           lpwszList = DdeAccessData ( hData, &cbDataLen );

           if ( !lpwszList )
              {
              uiErr = DdeGetLastError (idInst);
              DdeMessageBox (hInst, pDataReq->hwndMDI, uiErr, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION );
              break;
              }

           if ( VOpenClipboard ( pMDI->pVClpbrd, pDataReq->hwndMDI ) == TRUE )
              {
              BOOL  bHasBitmap = FALSE;

              VEmptyClipboard( pMDI->pVClpbrd );

              for (q = strtokA(lpwszList, "\t");q;q = strtokA(NULL,"\t"))
                 {
                 tmp = MyGetFormat(q, GETFORMAT_DONTLIE);

                 switch (tmp)
                     {
                     case CF_DIB:
                         // DDBitmap can be converted from Dib.
                         VSetClipboardData (pMDI->pVClpbrd, CF_BITMAP, NULL);
                     default:
                         VSetClipboardData (pMDI->pVClpbrd, tmp, NULL);
                     }
                 }

              VCloseClipboard( pMDI->pVClpbrd );
              }
           else
              {
              PERROR(TEXT("ProcessDataReq: unable to open Vclipboard\n\r"));
              }

           DdeUnaccessData ( hData );
           DdeFreeDataHandle ( hData );

           // set proper window text
           if ( pMDI->flags & F_LOCAL )
              {
              wsprintf( szBuf, TEXT("%s - %s"), szLocalClpBk, &(lpLE->name[1]) );
              }
           else
              {
              wsprintf( szBuf, TEXT("%s - %s"),
                    (pMDI->szBaseName), &(lpLE->name[1]) );
              }
           SetWindowText ( pDataReq->hwndMDI, szBuf );

           SetFocus ( pDataReq->hwndMDI );
           pMDI->CurSelFormat = CBM_AUTO;
           pMDI->fDisplayFormatChanged = TRUE;
           ResetScrollInfo ( pDataReq->hwndMDI );

           // means data is for going into page mode
           if ( pMDI->DisplayMode != DSP_PAGE )
              {
              pMDI->OldDisplayMode = pMDI->DisplayMode;
              pMDI->DisplayMode = DSP_PAGE;
              AdjustControlSizes ( pDataReq->hwndMDI );
              ShowHideControls ( pDataReq->hwndMDI );
              InitializeMenu ( GetMenu(hwndApp) );
              }
           else // data is for scrolling up or down one page
              {
              SendMessage ( pMDI->hWndListbox, LB_SETCURSEL,
                 pDataReq->iListbox, 0L );
              }

           UpdateNofMStatus ( pDataReq->hwndMDI );
           InvalidateRect ( pDataReq->hwndMDI, NULL, TRUE );

           // refresh preview bitmap?
           if ( !lpLE->hbmp )
              {
              GetPreviewBitmap ( pDataReq->hwndMDI, lpLE->name,
                 pDataReq->iListbox );
              }

           // PINFO("\r\n");
           bRet = TRUE;
           break;

        default:
           PERROR (TEXT("unknown type %d in ProcessDataReq\n\r"),
                 pDataReq->rqType );
           break;
        }



done:

    SetCursor (hSaveCursor);

    return bRet;

}
