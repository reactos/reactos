
/*****************************************************************************

                C L I P B O O K   V I E W E R   C O M M A N D S

    Name:       cvcomman.c
    Date:       21-Jan-1994
    Creator:    Unknown

    Description:
        This module handles all WM_COMMAND's.

    History:
        19-Apr-1994 John Fu     Add defines for FOCUSDLG_ to allow wider
                                browse in remote connect dialog.

        30-Jun-1994 John Fu     Add better cleanup in OnIDMDelete and OnIDMKeep
                                Now will delete the trust share and if problem
                                creating a page will ask server to delete it
                                as well.

        09-Aug-1994 John Fu     Changed IDM_SAVEAS, IDM_OPEN to save/open file
                                directly without the clipbook server.
                                Changed IDM_COPY to assign szSaveFileName to
                                "" so when WM_RENDERFORMAT will render through
                                DDE from clipbook server.

        13-Mar-1995 John Fu     Add hXacting event for timing related problems
                                Add capability to Paste to Page
                                Add IDM_UPDATE_PAGEVIEW

        03-Nov-1997 DrewM       Add support for context sensitive help
                                Add support for 15 character computer names
                                    (bug 2767)
                                Add call to UpdateListBox to remove the hand
                                    icon when a page is unshared. (bug 41147)

*****************************************************************************/





#define    NOAUTOUPDATE 1
#define    MAX_FILENAME_LENGTH 255

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <nddeapi.h>
#include <shellapi.h>
#include <assert.h>

#include "common.h"
#include "clipbook.h"
#include "clipbrd.h"
#include "clipdsp.h"
#include "dialogs.h"
#include "clpbkdlg.h"
#include "cvcomman.h"
#include "cvinit.h"
#include "cvutil.h"
#include "helpids.h"
#include "debugout.h"
#include "initmenu.h"
#include "shares.h"
#include "clipfile.h"

#include <htmlhelp.h>


// Typedef for the ShellAbout function
typedef void (WINAPI *LPFNSHELLABOUT)(HWND, LPTSTR, LPTSTR, HICON);




// Flags and typedef for the NT LanMan computer browser dialog.
// The actual function is I_SystemFocusDialog, in NTLANMAN.DLL.
#define FOCUSDLG_DOMAINS_ONLY        (1)
#define FOCUSDLG_SERVERS_ONLY        (2)
#define FOCUSDLG_SERVERS_AND_DOMAINS (3)

#define FOCUSDLG_BROWSE_LOGON_DOMAIN         0x00010000
#define FOCUSDLG_BROWSE_WKSTA_DOMAIN         0x00020000
#define FOCUSDLG_BROWSE_OTHER_DOMAINS        0x00040000
#define FOCUSDLG_BROWSE_TRUSTING_DOMAINS     0x00080000
#define FOCUSDLG_BROWSE_WORKGROUP_DOMAINS    0x00100000

typedef UINT (APIENTRY *LPFNSYSFOCUS)(HWND, UINT, LPWSTR, UINT, PBOOL, LPWSTR, DWORD);


static TCHAR szDirName[256] = {'\0',};









///////////////////////////////////////////////////////////////////////
//
// Purpose: Delete the selected share.
//
///////////////////////////////////////////////////////////////////////
LRESULT OnIDMDelete(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wparam,
    LPARAM  lparam)
{
int         tmp;
LPLISTENTRY lpLE;
LISTENTRY   LE;
LRESULT     ret = FALSE;
TCHAR       PageName[MAX_PAGENAME_LENGTH+1];


    if (!pActiveMDI)
        return 0L;



    // Doing a "delete" on the clipboard window clears clipboard
    if (pActiveMDI->flags & F_CLPBRD)
        {
        if ( ClearClipboard(hwndApp) == IDOK )
            {
            EmptyClipboard();
            InitializeMenu ( GetMenu(hwnd) );

            // Force redraw of clipboard window
            if (hwndClpbrd)
                {
                InvalidateRect(hwndClpbrd, NULL, TRUE);
                ret = TRUE;
                }
            }

        return ret;
        }





    tmp = (int)SendMessage (pActiveMDI->hWndListbox, LB_GETCURSEL, 0, 0L);

    if (tmp == LB_ERR)
        {
        PERROR("Could not figure out which item was selected!\r\n");
        return ret;
        }



    SendMessage ( pActiveMDI->hWndListbox, LB_GETTEXT, tmp, (LPARAM)(LPCSTR)&lpLE);
    memcpy(&LE, lpLE, sizeof(LE));

    wsprintf(szBuf, szDeleteConfirmFmt, (LPTSTR)((lpLE->name)+1) );
    MessageBeep ( MB_ICONEXCLAMATION );

    lstrcpy (PageName, lpLE->name);

    if (MessageBox ( hwndApp, szBuf, szDelete, MB_ICONEXCLAMATION|MB_OKCANCEL ) != IDCANCEL)
        {
        AssertConnection ( hwndActiveChild );

        if ( hwndActiveChild == hwndClpOwner )
            {
            ForceRenderAll( hwnd, NULL );
            }


        // Perform an execute to the server to let it know that
        // we're not sharing anymore.

        wsprintf(szBuf,TEXT("%s%s"), SZCMD_DELETE, lpLE->name);

        if (MySyncXact (szBuf,
                        lstrlen(szBuf) +1,
                        pActiveMDI->hExeConv,
                        0L,
                        CF_TEXT,
                        XTYP_EXECUTE,
                        SHORT_SYNC_TIMEOUT,
                        NULL)
            )
            {
            TCHAR   ComputerName[MAX_COMPUTERNAME_LENGTH+3] = TEXT("\\\\");
            DWORD   CNLen = sizeof (ComputerName);


            // Need to delete the trust
            GetComputerName (ComputerName+2, &CNLen);
            #ifdef USETWOSHARESPERPAGE
                if (fSharePreference)
                    PageName[0] = SHR_CHAR;
                else
                    PageName[0] = UNSHR_CHAR;
            #else
                PageName[0] = SHR_CHAR;
            #endif
            NDdeSetTrustedShare (ComputerName, PageName, NDDE_TRUST_SHARE_DEL);



            if ( pActiveMDI->DisplayMode == DSP_PAGE )
                {
                PINFO(TEXT("forcing back to list mode\n\r"));
                SendMessage (hwndApp, WM_COMMAND,
                             pActiveMDI->OldDisplayMode == DSP_PREV ?
                               IDM_PREVIEWS : IDM_LISTVIEW,
                             0L );
                }

            UpdateListBox (hwndActiveChild, pActiveMDI->hExeConv);
            InitializeMenu (GetMenu(hwndApp));
            }
        else
            {
            XactMessageBox (hInst, hwndApp, IDS_APPNAME, MB_OK | MB_ICONHAND);
            }
        }



    return ret;

}








/*
 *      OnIDMKeep
 *
 *
 *  Purpose: Create a Clipbook page.
 */

LRESULT OnIDMKeep (
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam,
    BOOL    bNewPage)
{
PMDIINFO        pMDI;
int             tmp;
DWORD           ret;
HANDLE          hData;
PNDDESHAREINFO  lpDdeInfo = NULL;
TCHAR           atchItem[256];
#ifdef NOOLEITEMSPERMIT
    unsigned    i;
#endif
LPTSTR          lpEnd; // Pointer to the end of the current data block
TCHAR           rgtchCName[MAX_COMPUTERNAME_LENGTH + 3];
DWORD           dwLen;
HCURSOR         hCursor;
BOOL            bShareSave = fSharePreference;
KEEPASDLG_PARAM KeepAs;






    if (!CountClipboardFormats())
        {
        PERROR (TEXT("Paste entered with no data on the clipboard!\r\n"));
        goto done;
        }



    if (!hwndLocal || !IsWindow(hwndLocal))
        {
        MessageBoxID (hInst, hwnd, IDS_NOCLPBOOK, IDS_APPNAME, MB_OK | MB_ICONSTOP);
        goto done;
        }



    if (!pActiveMDI)
        goto done;




    pMDI = GETMDIINFO(hwndLocal);







    if (bNewPage)
        {
        tmp = (int)SendMessage (pActiveMDI->hWndListbox, LB_GETCOUNT, 0, 0L );

        if (tmp >= MAX_ALLOWED_PAGES)
            {
            MessageBoxID (hInst, hwnd, IDS_MAXPAGESERROR, IDS_PASTEDLGTITLE, MB_OK|MB_ICONEXCLAMATION);
            goto done;
            }


        // Do the dialog and get KeepAs request

        KeepAs.ShareName[0]   = TEXT('\0');
        KeepAs.bAlreadyExist  = FALSE;
        KeepAs.bAlreadyShared = FALSE;

        dwCurrentHelpId = 0;            //  F1 will be context sensitive


        ret = (DWORD)DialogBoxParam (hInst,
                                     MAKEINTRESOURCE(IDD_KEEPASDLG),
                                     hwnd,
                                     KeepAsDlgProc ,
                                     (LPARAM)&KeepAs);

        PINFO (TEXT("DialogBox returning %d\n\r"), ret );
        dwCurrentHelpId = 0L;


        // refresh main window
        UpdateWindow (hwndApp);

        if (!ret || !KeepAs.ShareName[0])
            goto done;

        bShareSave = fSharePreference;
        }



    if (!bNewPage || KeepAs.bAlreadyExist)
        {
        PLISTENTRY lpLE;

        if (!bNewPage)
            tmp = (int)SendMessage (pMDI->hWndListbox, LB_GETCURSEL, 0, 0);
        else
            tmp = (int)SendMessage (pMDI->hWndListbox,
                                    LB_FINDSTRING,
                                    (WPARAM)-1,
                                    (LPARAM)(LPCSTR)KeepAs.ShareName);


        if (LB_ERR == tmp)
            goto done;

        SendMessage (pMDI->hWndListbox,
                     LB_GETTEXT,
                     tmp,
                     (LPARAM)&lpLE);

        strcpy (KeepAs.ShareName, lpLE->name);

        KeepAs.bAlreadyShared = IsShared (lpLE);
        KeepAs.bAlreadyExist  = TRUE;

        fSharePreference = bNewPage? fSharePreference: KeepAs.bAlreadyShared;
        }









    // Set up NetDDE share for the page
    lpDdeInfo = GlobalAllocPtr (GHND, 2048 * sizeof(TCHAR));
    if (!lpDdeInfo)
       {
       MessageBoxID (hInst, hwnd, IDS_INTERNALERR, IDS_APPNAME, MB_OK|MB_ICONSTOP);
       goto done;
       }



    hCursor = SetCursor (LoadCursor (NULL, IDC_WAIT));




    // Set up computer name with \\ in front

    rgtchCName[1] = rgtchCName[0] = TEXT('\\');
    dwLen = MAX_COMPUTERNAME_LENGTH+1;
    GetComputerName (rgtchCName + 2, &dwLen);



    lpEnd = (LPTSTR)lpDdeInfo + sizeof(NDDESHAREINFO);



    // Set up the constant members of the struct

    if (KeepAs.bAlreadyExist && KeepAs.bAlreadyShared)
        {
        DWORD   dwAddItem = 0;
        DWORD   dwTotal;

        ret = NDdeShareGetInfo (rgtchCName,
                                KeepAs.ShareName,
                                2,
                                (PUCHAR)lpDdeInfo,
                                2048 * sizeof (TCHAR),
                                &dwTotal,
                                (PUSHORT)&dwAddItem);
        }
    else
        {
        lpDdeInfo->lRevision        = 1L;
        lpDdeInfo->fSharedFlag      = 0;
        lpDdeInfo->fService         = 1; //0;
        lpDdeInfo->fStartAppFlag    = 0;
        lpDdeInfo->qModifyId[0]     = 0;
        lpDdeInfo->qModifyId[1]     = 0;
        lpDdeInfo->nCmdShow         = SW_SHOWMINNOACTIVE;
        lpDdeInfo->lShareType       = SHARE_TYPE_STATIC;
        }



    // Enter the share name... must be == $<PAGENAME>.
    // (WFW used the dollar sign, we use the dollar sign. I
    //  hate backwards compatibility.)
    lpDdeInfo->lpszShareName = lpEnd;

    #ifdef USETWOSHARESPERPAGE
        if (fSharePreference || KeepAs.bAlreadyShared)
            {
            *lpEnd = SHR_CHAR;
            }
        else
            {
            *lpEnd = UNSHR_CHAR;
            }
    #else
        *lpEnd = SHR_CHAR;
    #endif


    lstrcpy(lpDdeInfo->lpszShareName + 1, KeepAs.ShareName + 1);
    lpEnd += lstrlen(lpDdeInfo->lpszShareName) + 1;


    // Start work on the app|topic list
    lpDdeInfo->lpszAppTopicList = lpEnd;

    // By default, there are no items.
    atchItem[0] = TEXT('\0');

    // Set up old-style and OLE name if cf_objectlink is
    // available, else set '\0'.
    if (OpenClipboard(hwnd))
        {
        unsigned cb;
        LPTSTR lpData;

        if ((hData = VGetClipboardData(NULL, cf_link)) &&
            (lpData = GlobalLock(hData)))
            {
            PINFO(TEXT("Link found\r\n"));
            lstrcpy(lpEnd, lpData);
            lpEnd += cb = lstrlen(lpEnd);
            *lpEnd++ = TEXT('|');
            lstrcpy(lpEnd, lpData + cb + 1);
            cb += lstrlen(lpEnd) + 2;
            lpEnd += lstrlen(lpEnd) + 1;
            lstrcpy(atchItem, lpData + cb);
            GlobalUnlock(lpData);
            lpDdeInfo->lShareType |= SHARE_TYPE_OLD;
            }
        else
            {
            *lpEnd++ = TEXT('\0');
            }

        if ((hData = VGetClipboardData(NULL, cf_objectlink)) &&
            (lpData = GlobalLock(hData)))
            {
            PINFO(TEXT("ObjectLink found\r\n"));
            lstrcpy(lpEnd, lpData);
            lpEnd += cb = lstrlen(lpEnd);
            *lpEnd++ = TEXT('|');
            lstrcpy(lpEnd, lpData + cb + 1);
            cb += lstrlen(lpEnd) + 2;
            lpEnd += lstrlen(lpEnd) + 1;
            lstrcpy(atchItem, lpData + cb);
            GlobalUnlock(lpData);
            lpDdeInfo->lShareType |= SHARE_TYPE_NEW;
            }
        else
            {
            *lpEnd++ = TEXT('\0');
            }

        CloseClipboard();
        }
    else // We couldn't open, we can't get objectlink.
       {
       *lpEnd++ = TEXT('\0');
       *lpEnd++ = TEXT('\0');
       }




    // Set up "CLIPSRV|*<pagename>" for a static app/topic
    // We use the *<pagename> form because when the page
    // is first created, it's ALWAYS unshared, and the server's
    // expecting us to be on the "unshared" topic name.
    // Unless the page already exists and is already shared.

    lstrcpy (lpEnd, SZ_SRV_NAME);
    lstrcat (lpEnd, TEXT(BAR_CHAR));
    lpEnd += lstrlen(lpEnd);

    if (KeepAs.bAlreadyShared)
        *lpEnd = SHR_CHAR;
    else
        *lpEnd = UNSHR_CHAR;



    lstrcpy(lpEnd + 1, KeepAs.ShareName + 1);
    lpEnd += lstrlen(lpEnd) + 1;
    // NetDDE requires a fourth NULL at the end of the app/topic
    // list - dumb, but easier to do this than fix that.
    *lpEnd++ = TEXT('\0');

    lpDdeInfo->lpszItemList = lpEnd;
    // If there's an item listed, we need to set the item.
    // Otherwise, set no items-- this is an OLE link to the entire
    // document. ANY item, but there's nothing but the static
    // share anyway.
    if (lstrlen(atchItem))
        {
        lstrcpy(lpEnd, atchItem);
        lpEnd += lstrlen(lpEnd) + 1;
        lpDdeInfo->cNumItems = 1;
        #ifdef NOOLEITEMSPERMIT
            for (i = 0; i < NOLEITEMS; i++)
                {
                lstrcpy(lpEnd, OleShareItems[i]);
                lpEnd += lstrlen(lpEnd) + 1;
                }
            lpDdeInfo->cNumItems = NOLEITEMS + 1;
        #endif
        }
    else
        {
        lpDdeInfo->cNumItems = 0;
        *lpEnd++ = TEXT('\0');
        }


    // Finish off item list with an extra null.
    *lpEnd++ = TEXT('\0');

    // Get an SD -- if this fails, we'll get a NULL pSD back,
    // and we'll end up creating the share with a default SD.
    // This is a tolerable condition.
    // pSD = CurrentUserOnlySD();




    // Create the share

    if (!KeepAs.bAlreadyExist)
        {
        DumpDdeInfo(lpDdeInfo, rgtchCName);
        ret = NDdeShareAdd (rgtchCName, 2, NULL, (LPBYTE)lpDdeInfo, sizeof(NDDESHAREINFO) );

        // We have to set security in a separate step, because
        // we set up a "default" DACL, and if we pass it in to
        // NDdeShareAdd(), it'll get overwritten by the "inherited"
        // DACL
        // The security people are scum. Use default
        // ([CreatorAll WorldRL]) share security.
        // if (pSD && NDDE_NO_ERROR == ret)
        //    {
        //    PrintSD(pSD);
        //
        //    ret = NDdeSetShareSecurity(rgtchCName,
        //                lpDdeInfo->lpszShareName,
        //                DACL_SECURITY_INFORMATION, pSD);
        //    }
        //
        PINFO(TEXT("NDdeShareAdd ret %ld\r\n"), ret);
        // if (pSD)
        //    {
        //    GlobalFree(pSD);
        //    }



        if (ret != NDDE_NO_ERROR && ret != NDDE_SHARE_ALREADY_EXIST)
            {
            if (NDDE_ACCESS_DENIED == ret)
                {
                MessageBoxID (hInst, hwnd, IDS_PRIVILEGEERROR, IDS_APPNAME, MB_OK|MB_ICONSTOP);
                }
            else
                {
                PERROR(TEXT("NDDE Error %d\r\n"), ret);
                NDdeMessageBox (hInst, hwnd, ret, IDS_APPNAME, MB_OK|MB_ICONSTOP);
                }
            goto done;
            }



        // Need to trust the share so that we can init through it!
        ret = NDdeSetTrustedShare (rgtchCName,
                                   lpDdeInfo->lpszShareName,
                                   NDDE_TRUST_SHARE_INIT);

        if (ret != NDDE_NO_ERROR)
            NDdeMessageBox (hInst, hwnd, ret, IDS_APPNAME, MB_OK|MB_ICONSTOP);

        }
    else
        {
        ret = NDdeShareSetInfo (rgtchCName,
                                lpDdeInfo->lpszShareName,
                                2,
                                (LPBYTE)lpDdeInfo,
                                sizeof(NDDESHAREINFO),
                                0);

        if (NDDE_NO_ERROR != ret)
            {
            NDdeMessageBox (hInst, hwnd, ret, IDS_APPNAME, MB_OK|MB_ICONSTOP);
            goto done;
            }
        }







    // Send DEExecute to tell clipsrv that we've created this page,
    // and will it please make an actual file for it?
    // NOTE must force all formats rendered to prevent deadlock
    // on the clipboard.
    ForceRenderAll (hwnd, NULL);

    lstrcat (lstrcpy (szBuf, SZCMD_PASTE), KeepAs.ShareName);


    AssertConnection (hwndLocal);



    if (!MySyncXact ((LPBYTE)szBuf,
                     lstrlen(szBuf) +1,
                     pMDI->hExeConv,
                     0L,
                     CF_TEXT,
                     XTYP_EXECUTE,
                     LONG_SYNC_TIMEOUT,
                     NULL))
        {
        XactMessageBox (hInst, hwnd, IDS_APPNAME, MB_OK|MB_ICONSTOP);


        if (!KeepAs.bAlreadyExist)
            {
            // Problem creating the page so ask the server to delete it
            wsprintf (szBuf, TEXT("%s%s"), SZCMD_DELETE, KeepAs.ShareName);
            MySyncXact (szBuf,
                        lstrlen (szBuf) +1,
                        pMDI->hExeConv,
                        0L,
                        CF_TEXT,
                        XTYP_EXECUTE,
                        SHORT_SYNC_TIMEOUT,
                        NULL);


            // and we'll delete the rest.
            NDdeSetTrustedShare (rgtchCName,
                                 lpDdeInfo->lpszShareName,
                                 NDDE_TRUST_SHARE_DEL);

            NDdeShareDel (rgtchCName,
                          lpDdeInfo->lpszShareName,
                          0);
            goto done;
            }
        }






    // Turn off redraw and add the new page to list.  Adding the new item
    // to list is necessary because the Properties() call below.  Turning
    // off the redraw is necessary because we sometimes get into a re-entrancy
    // problem.  When the list box is update, it is redrawn and if we're in
    // the preview mode, we get into the async xaction in the middle of some
    // sync xact.

    SendMessage (pMDI->hWndListbox, WM_SETREDRAW, FALSE, 0);

    if (!KeepAs.bAlreadyExist)
        {
        PLISTENTRY lpLE;

        // below code is copied from InitListBox()
        if (lpLE = (PLISTENTRY)GlobalAllocPtr (GHND, sizeof(LISTENTRY)))
            {
            lpLE->fDelete = TRUE;
            lpLE->fTriedGettingPreview = FALSE;
            lstrcpy (lpLE->name, KeepAs.ShareName);
            SendMessage (pMDI->hWndListbox, LB_ADDSTRING, 0, (LPARAM)lpLE);
            }
        }




    if (fSharePreference != KeepAs.bAlreadyShared)
        {
        // get the item number
        tmp = (int)SendMessage (pMDI->hWndListbox,
                                LB_FINDSTRING,
                                (WPARAM)-1,
                                (LPARAM)(LPCSTR)KeepAs.ShareName);


        if (LB_ERR != tmp)
            {
            if (fSharePreference)
                {
                PLISTENTRY lpLE;

                SendMessage (pMDI->hWndListbox,
                             LB_GETTEXT,
                             tmp,
                             (LPARAM)&lpLE);
                Properties (hwnd, lpLE);
                }
            else
                OnIdmUnshare (tmp);
            }
        }




    // Now, turn on redraw.

    SendMessage (pMDI->hWndListbox, WM_SETREDRAW, TRUE, 0);



    // update the list box in all cases, the function
    // is smart enough to figure out which item has
    // changed and update only it.

    UpdateListBox (hwndLocal, pMDI->hExeConv);
    InvalidateRect (pMDI->hWndListbox, NULL, FALSE);




done:
    if (lpDdeInfo)
        GlobalFreePtr (lpDdeInfo);


    InitializeMenu (GetMenu (hwndApp));
    hCursor = SetCursor (hCursor);


    fSharePreference = bShareSave;


    return 0L;

}








/*
 *      OnIDMCopy
 *
 *  Handles IDM_COPY to copy a page to clipbrd.
 */


LRESULT OnIDMCopy (
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam)
{
LPLISTENTRY lpLE;
PMDIINFO    pMDIc;
PDATAREQ    pDataReq;
TCHAR       tchTmp;
INT         tmp;
BOOL        fLocked;




    if (WAIT_TIMEOUT == WaitForSingleObject (hXacting, 0))
        return 0L;



    fLocked = LockApp (TRUE, NULL);





    // make a copy to ensure that the global is not
    // changed from under us in case proc is reentered

    if (!(pMDIc = GETMDIINFO(hwndActiveChild)))
        goto done;

    tmp = (int)SendMessage ( pMDIc->hWndListbox, LB_GETCURSEL, 0, 0L );

    if (tmp == LB_ERR)
        goto done;




    if (SendMessage (pMDIc->hWndListbox, LB_GETTEXT, tmp, (LPARAM)(LPCSTR)&lpLE)
        == LB_ERR )
        {
        PERROR(TEXT("IDM_COPY: bad listbox index: %d\n\r"), tmp );
        goto done;
        }

    if (!(pDataReq = CreateNewDataReq()))
        {
        PERROR(TEXT("error from CreateNewDataReq\n\r"));
        goto done;
        }



    if (pMDIc->hszClpTopic)
        {
        DdeFreeStringHandle (idInst, pMDIc->hszClpTopic);
        }



    tchTmp = lpLE->name[0];
    lpLE->name[0] = SHR_CHAR;
    pMDIc->hszClpTopic = DdeCreateStringHandle(idInst, lpLE->name, 0);


    // If we're local, trust the share so we can copy through it
    if (hwndActiveChild == hwndLocal)
       {
       DWORD adwTrust[3];

       NDdeGetTrustedShare(NULL, lpLE->name, adwTrust, adwTrust + 1,
            adwTrust + 2);
       adwTrust[0] |= NDDE_TRUST_SHARE_INIT;

       NDdeSetTrustedShare(NULL, lpLE->name, adwTrust[0]);
       }

    lpLE->name[0] = tchTmp;


    if ( !pMDIc->hszClpTopic )
       {
       MessageBoxID (hInst,
                     hwndActiveChild,
                     IDS_DATAUNAVAIL,
                     IDS_APPNAME,
                     MB_OK | MB_ICONEXCLAMATION );
       goto done;
       }



    if (pMDIc->hClpConv)
       {
       DdeDisconnect (pMDIc->hClpConv);
       pMDIc->hClpConv = NULL;
       }

    pMDIc->hClpConv = DdeConnect (idInst,
                                  pMDIc->hszConvPartner,
                                  pMDIc->hszClpTopic,
                                  NULL);



    if (!pMDIc->hClpConv)
       {
       PERROR(TEXT("DdeConnect to (%s) failed %d\n\r"),
              (LPSTR)(lpLE->name), DdeGetLastError(idInst) );
       MessageBoxID (hInst,
                     hwndActiveChild,
                     IDS_DATAUNAVAIL,
                     IDS_APPNAME,
                     MB_OK | MB_ICONEXCLAMATION);
       goto done;
       }


    pDataReq->rqType      = RQ_COPY;
    pDataReq->hwndList    = pMDIc->hWndListbox;
    pDataReq->iListbox    = tmp;
    pDataReq->hwndMDI     = hwndActiveChild;
    pDataReq->fDisconnect = FALSE;
    pDataReq->wFmt        = CF_TEXT;

    DdeSetUserHandle (pMDIc->hClpConv, (DWORD)QID_SYNC, (DWORD_PTR)pDataReq);
    DdeKeepStringHandle (idInst, hszFormatList);



    if (!DdeClientTransaction (NULL,
                               0L,
                               pMDIc->hClpConv,
                               hszFormatList,
                               CF_TEXT,
                               XTYP_REQUEST,
                               (DWORD)TIMEOUT_ASYNC,
                               NULL))
        DdeMessageBox (hInst,
                       hwndApp,
                       DdeGetLastError(idInst),
                       IDS_APPNAME,
                       MB_OK|MB_ICONEXCLAMATION);



    // now a copy request packed will arrive at the call-back... or
    // not. BUGBUG report an error if no callback?



done:

    if (fLocked)
        LockApp (FALSE, NULL);


    SetEvent (hXacting);

    return 0L;

}










/*
 *      CreateClipboardWindow
 *
 *  Purpose: Create and activate a window showing the contents of the
 *  clipboard.
 */

static void CreateClipboardWindow (void)
{
WINDOWPLACEMENT wpl;
HMENU           hSysMenu;
PMDIINFO        pMDI;


    // create Clipboard Window
    hwndClpbrd = NewWindow();
    if (NULL == hwndClpbrd)
        {
        return;
        }


    pMDI              = GETMDIINFO(hwndClpbrd);
    pMDI->flags       = F_CLPBRD;
    pMDI->DisplayMode = DSP_PAGE;

    AdjustControlSizes ( hwndClpbrd );
    ShowHideControls ( hwndClpbrd );

    lstrcpy (pMDI->szBaseName, szSysClpBrd);
    lstrcpy (pMDI->szComputerName, TEXT(""));

    SetWindowText ( hwndClpbrd, szSysClpBrd );

    // Grey out close item on sys menu
    hSysMenu = GetSystemMenu ( hwndClpbrd, FALSE );
    EnableMenuItem (hSysMenu, SC_CLOSE, MF_GRAYED | MF_BYCOMMAND);

    // Tell MDI where the Window menu is -- must do this BEFORE placing
    // the clipboard window. (If the clipboard window's maximized, its
    // System menu is the first menu-- not the app's File menu.)
    hSysMenu = GetSubMenu(GetMenu(hwndApp), WINDOW_MENU_INDEX);
    SendMessage(hwndMDIClient, WM_MDISETMENU, 0, (LPARAM)hSysMenu);

    if ( ReadWindowPlacement ( szSysClpBrd, &wpl ))
        {
        wpl.length = sizeof(WINDOWPLACEMENT);
        wpl.flags = WPF_SETMINPOSITION;
        SetWindowPlacement ( hwndClpbrd, &wpl );
        PINFO(TEXT("sizing %s from .ini\n\r"), (LPSTR)szSysClpBrd);
        UpdateWindow ( hwndClpbrd );
        }
    else
        {
        PINFO(TEXT("showing %s in default size/posiiton\n\r"),
            (LPSTR)szSysClpBrd );
        ShowWindow ( hwndClpbrd, SW_MINIMIZE );
        }

    SendMessage ( hwndMDIClient, WM_MDIACTIVATE, (WPARAM)hwndClpbrd, 0L );

}









/*
 *      CreateLocalWindow
 *
 *  Purpose: Create the "Local Clipbook" window.
 *
 *  Parameters: None.
 *
 *  Returns: Void.
 *
 */

static void CreateLocalWindow (void)
{
WINDOWPLACEMENT wpl;
HMENU           hSysMenu;
PMDIINFO        pMDI;



    hwndLocal = NewWindow();
    if (NULL == hwndLocal)
        {
        return;
        }


    pMDI = GETMDIINFO(hwndLocal);
    ShowHideControls (hwndLocal);

    pMDI->hszConvPartner   =
    pMDI->hszConvPartnerNP = hszDataSrv;
    pMDI->hExeConv         = InitSysConv (hwndLocal,
                                          pMDI->hszConvPartner,
                                          hszSystem,
                                          TRUE);

    if (!pMDI->hExeConv )
        goto error;



    pMDI->flags = F_LOCAL;

    if (!UpdateListBox ( hwndLocal, pMDI->hExeConv ))
        goto error;



    SetWindowText ( hwndLocal, szLocalClpBk );


    lstrcpy (pMDI->szBaseName, szLocalClpBk);
    lstrcpy (pMDI->szComputerName, TEXT(""));



    hSysMenu = GetSystemMenu ( hwndLocal, FALSE );
    EnableMenuItem ( hSysMenu, SC_CLOSE, MF_GRAYED );


    if ( ReadWindowPlacement ( szLocalClpBk, &wpl ))
        {
        wpl.length = sizeof(WINDOWPLACEMENT);
        wpl.flags = WPF_SETMINPOSITION;
        SetWindowPlacement ( hwndLocal, &wpl );
        PINFO(TEXT("sizing Local Clipbook from .ini\n\r"));
        UpdateWindow ( hwndLocal );
        }
    else
        {
        if ( !IsIconic(hwndApp))
            {
            RECT MDIrect;

            PINFO(TEXT("calculating size for Local Clipbook window\n\r"));
            GetClientRect ( hwndMDIClient, &MDIrect );
            MoveWindow ( hwndLocal,
                MDIrect.left, MDIrect.top, MDIrect.right - MDIrect.left,
                ( MDIrect.bottom - MDIrect.top )
                - GetSystemMetrics(SM_CYICONSPACING), FALSE );
            }
        else
            {
            fNeedToTileWindows = TRUE;
            }
        ShowWindow ( hwndLocal, SW_SHOWNORMAL );
        }

    SendMessage (hwndMDIClient, WM_MDIACTIVATE, (WPARAM)hwndLocal, 0L);
    SendMessage (hwndMDIClient, WM_MDIREFRESHMENU, 0, 0L);

    if (NULL != hkeyRoot)
        {
        DWORD dwDefView = IDM_LISTVIEW;
        DWORD dwSize = sizeof(dwDefView);

        if (ERROR_SUCCESS != RegQueryValueEx(hkeyRoot,
              (LPTSTR)szDefView, NULL, NULL, (LPBYTE)&dwDefView, &dwSize));
            {
            PINFO(TEXT("Couldn't get DefView value\r\n"));
            }

        SendMessage ( hwndApp, WM_COMMAND, dwDefView, 0L );
        }


    return;


error:
    #if DEBUG
        MessageBox (hwndApp,
                    TEXT("No Local Server"),
                    TEXT("ClipBook Initialization"),
                    MB_OK | MB_ICONEXCLAMATION );
    #endif

    fShareEnabled = FALSE;

    //SendMessage ( hwndLocal, WM_CLOSE, 0, 0L );
    SendMessage (hwndLocal, WM_MDIDESTROY, 0, 0L);

    hwndLocal = NULL;

    return;

}











/*
 *      UnsharePage
 *
 *
 *  Purpose: Unshare the selected page in the active window.
 *
 *  Parameters: None.
 *
 *  Returns: Void. All error handling is provided within the function.
 *
 */

void UnsharePage (void)
{
DWORD           adwTrust[3];
int             tmp;
LPLISTENTRY     lpLE;
DWORD           ret;
WORD            wAddlItems;
PNDDESHAREINFO  lpDdeI;
DWORD           dwRet = 2048 * sizeof(TCHAR);



    assert(pActiveMDI);

    if (!pActiveMDI);
        return;



    tmp = (int)SendMessage (pActiveMDI->hWndListbox, LB_GETCURSEL, 0, 0L);


    if ( tmp == LB_ERR )
        return;


    if (!(lpDdeI = LocalAlloc(LPTR, 2048 * sizeof(TCHAR))))
       {
       MessageBoxID (hInst, hwndApp, IDS_INTERNALERR, IDS_APPNAME, MB_OK | MB_ICONHAND);
       return;
       }


   SendMessage (  pActiveMDI->hWndListbox, LB_GETTEXT, tmp,
       (LPARAM)(LPCSTR)&lpLE);

   AssertConnection(hwndActiveChild);

   PINFO(TEXT("for share [%s]"), lpLE->name);
   wAddlItems = 0;
   ret = NDdeShareGetInfo ( NULL, lpLE->name, 2,
       (LPBYTE)lpDdeI, 2048 * sizeof(TCHAR), &dwRet, &wAddlItems );


    if (NDDE_ACCESS_DENIED == ret)
        {
        MessageBoxID (hInst, hwndApp, IDS_PRIVILEGEERROR, IDS_APPNAME,
                      MB_OK | MB_ICONHAND);
        }
    else if (NDDE_NO_ERROR != ret)
        {
        PERROR(TEXT("Error from NDdeShareSetInfo %d\n\r"), ret );
        NDdeMessageBox (hInst, hwndApp, ret,
                        IDS_SHAREDLGTITLE, MB_ICONHAND | MB_OK );
        }
    else
        {
        register LPTSTR lpOog;

        lpOog = lpDdeI->lpszAppTopicList;

        // Jump over the first two NULL chars you find-- these
        // are the old- and new-style app/topic pairs, we don't
        // mess with them. Then jump over the next BAR_CHAR you find.
        // The first character after that is the first char of the
        // static topic-- change that to a UNSHR_CHAR.

        while (*lpOog++) ;
        while (*lpOog++) ;


        // BUGBUG: TEXT('|') should == BAR_CHAR. If not, this needs to
        // be adjusted.

        while (*lpOog++ != TEXT('|')) ;



        *lpOog = UNSHR_CHAR;
        lpDdeI->fSharedFlag = 0L;

        DumpDdeInfo(lpDdeI, NULL);

        // We want to get trusted info BEFORE we start changing
        // the share.
        NDdeGetTrustedShare(NULL, lpLE->name, adwTrust,
           adwTrust + 1, adwTrust + 2);

        ret = NDdeShareSetInfo ( NULL, lpLE->name, 2,
            (LPBYTE)lpDdeI, 2048 * sizeof(TCHAR), 0 );
        if (NDDE_NO_ERROR == ret)
            {
            #if 0
            PSECURITY_DESCRIPTOR pSD;
            DWORD dwSize;
            TCHAR atch[2048];
            BOOL  fDacl, fDefault;
            PACL  pacl;

            pSD = atch;
            if (NDDE_NO_ERROR == NDdeGetShareSecurity (NULL,
                                                       lpLE->name,
                                                       DACL_SECURITY_INFORMATION,
                                                       pSD,
                                                       2048 * sizeof(TCHAR),
                                                       &dwSize))
                {
                if (GetSecurityDescriptorDacl(pSD, &fDacl,
                      &pacl, &fDefault))
                    {
                    if (fDefault || !fDacl)
                        {
                        if (pSD = CurrentUserOnlySD())
                            {
                            NDdeSetShareSecurity (NULL,
                                                  lpLE->name,
                                                  DACL_SECURITY_INFORMATION,
                                                  pSD);
                            LocalFree(pSD);
                            }
                        }
                    }
                }
            #endif

            // We've finished mucking with the share, now set
            // trust info
            PINFO(TEXT("Setting trust info to 0x%lx\r\n"),
               adwTrust[0]);
            NDdeSetTrustedShare(NULL, lpLE->name,
               adwTrust[0]);

            ///////////////////////////////////////////////
            // do the execute to change the server state
            lstrcat(lstrcpy(szBuf,SZCMD_UNSHARE),lpLE->name);
            PINFO(TEXT("sending cmd [%s]\n\r"), szBuf);

            if (MySyncXact ((LPBYTE)szBuf,
                            lstrlen(szBuf) +1,
                            GETMDIINFO(hwndLocal)->hExeConv,
                            0L,
                            CF_TEXT,
                            XTYP_EXECUTE,
                            SHORT_SYNC_TIMEOUT,
                            NULL))
                {
                SetShared(lpLE, FALSE);
                InitializeMenu(GetMenu(hwndApp));
                }
            else
                {
                XactMessageBox (hInst, hwndApp, IDS_APPNAME, MB_OK | MB_ICONSTOP);
                }
            }
        }


}









/*
 *      OnIdmUnshare
 *
 *
 *  Purpose: Set the currently selected page in the active MDI window
 *      to 'unshared'.
 *
 *  dwItem is the item number to unshare.  If == LB_ERR then the current
 *      selected item will be unshared.
 *
 *  Parameters: None.
 *
 *  Returns: 0L always, function handles its own errors.
 *
 */

LRESULT OnIdmUnshare (DWORD dwItem)
{
PNDDESHAREINFO lpDdeI;
PLISTENTRY     lpLE;
DWORD          adwTrust[3];
WORD           wAddlItems;
DWORD          ret;
DWORD          dwRet = 2048 * sizeof(TCHAR);
LPTSTR         lpOog;

#if 0
    PSECURITY_DESCRIPTOR    pSD;
    DWORD                   dwSize;
    BOOL                    fDacl, fDefault;
    PACL                    pacl;
#endif




    if (!pActiveMDI)
        return 0L;


    if (LB_ERR == dwItem)
        dwItem = (int)SendMessage (pActiveMDI->hWndListbox, LB_GETCURSEL, 0, 0L );

    if (LB_ERR == dwItem)
        {
        PERROR(TEXT("IDM_UNSHARE w/no page selected\r\n"));
        return 0L;
        }




    if (!(lpDdeI = LocalAlloc(LPTR, 2048 * sizeof(TCHAR))))
        {
        MessageBoxID (hInst, hwndApp, IDS_INTERNALERR, IDS_APPNAME,
                      MB_OK | MB_ICONHAND);
        return 0L;
        }



    SendMessage (pActiveMDI->hWndListbox,
                 LB_GETTEXT,
                 dwItem,
                 (LPARAM)(LPCSTR)&lpLE);


    AssertConnection(hwndActiveChild);





    PINFO(TEXT("for share [%s]"), lpLE->name);
    wAddlItems = 0;
    ret = NDdeShareGetInfo (NULL, lpLE->name,
                            2,
                            (LPBYTE)lpDdeI,
                            2048 * sizeof(TCHAR),
                            &dwRet,
                            &wAddlItems );


    if (NDDE_ACCESS_DENIED == ret)
        {
        MessageBoxID (hInst, hwndApp, IDS_PRIVILEGEERROR, IDS_APPNAME,
                      MB_OK | MB_ICONHAND);
        return 0L;
        }
    else if (ret != NDDE_NO_ERROR)
        {
        PERROR(TEXT("Error from NDdeShareSetInfo %d\n\r"), ret );
        NDdeMessageBox (hInst, hwndApp, ret,
                        IDS_SHAREDLGTITLE, MB_ICONHAND | MB_OK );
        return 0L;
        }



    lpOog = lpDdeI->lpszAppTopicList;


    // Jump over the first two NULL chars you find-- these
    // are the old- and new-style app/topic pairs, we don't
    // mess with them. Then jump over the next BAR_CHAR you find.
    // The first character after that is the first char of the
    // static topic-- change that to a SHR_CHAR.

    while (*lpOog++) ;
    while (*lpOog++) ;


    // BUGBUG: TEXT('|') should == BAR_CHAR. If not, this needs to
    // be adjusted.

    while (*lpOog++ != TEXT('|')) ;


    *lpOog = UNSHR_CHAR;
    lpDdeI->fSharedFlag = 1L;



    // Have to get trusted share settings before we modify
    // the share, 'cuz they'll be invalid.

    NDdeGetTrustedShare (NULL,
                         lpDdeI->lpszShareName,
                         adwTrust,
                         adwTrust + 1,
                         adwTrust + 2);


    DumpDdeInfo (lpDdeI, NULL);
    ret = NDdeShareSetInfo (NULL,
                            lpDdeI->lpszShareName,
                            2,
                            (LPBYTE)lpDdeI,
                            2048 * sizeof(TCHAR),
                            0);


    if (NDDE_ACCESS_DENIED == ret)
        {
        MessageBoxID (hInst, hwndApp, IDS_PRIVILEGEERROR, IDS_APPNAME,
                      MB_OK | MB_ICONHAND);
        return 0L;
        }
    else if (NDDE_NO_ERROR != ret)
        {
        NDdeMessageBox (hInst, hwndApp, ret, IDS_APPNAME,
                        MB_OK | MB_ICONHAND);
        PERROR(TEXT("Couldn't set share info\r\n"));
        return 0L;
        }




    #if 0
        pSD = LocalAlloc (LPTR, 30);
        ret = NDdeGetShareSecurity (NULL,
                                    lpDdeI->lpszShareName,
                                    DACL_SECURITY_INFORMATION,
                                    pSD,
                                    30,
                                    &dwSize);

        if (NDDE_BUF_TOO_SMALL == ret && dwSize < 65535L)
            {
            LocalFree(pSD);
            pSD = LocalAlloc(LPTR, dwSize);

            ret =  NDdeGetShareSecurity (NULL,
                                         lpDdeI->lpszShareName,
                                         DACL_SECURITY_INFORMATION,
                                         pSD,
                                         30,
                                         &dwSize);
            }


        if (NDDE_NO_ERROR == ret)
            {
            if (GetSecurityDescriptorDacl(pSD, &fDacl, &pacl, &fDefault))
                {
                LocalFree (pSD);

                if (fDefault || !fDacl)
                    {
                    if (pSD = CurrentUserOnlySD())
                        {
                        NDdeSetShareSecurity (NULL,
                                              lpDdeI->lpszShareName,
                                              DACL_SECURITY_INFORMATION,
                                              pSD);
                        }
                    else
                        {
                        PERROR(TEXT("Couldn't make CUOnlySD"));
                        }
                    }
                else
                    {
                    PINFO(TEXT("Non-default DACL"));
                    }
                }
            else
                {
                PINFO(TEXT("No DACL"));
                }
            }
        else
            {
            PERROR(TEXT("Couldn't get security for share"));
            }
        PERROR(TEXT("\r\n"));

        if (pSD)
            {
            LocalFree(pSD);
            }
    #endif



    // Setting trusted share info needs to be the last
    // operation we do on the share.
    if (NDDE_NO_ERROR != NDdeSetTrustedShare (NULL, lpDdeI->lpszShareName, adwTrust[0]))
        {
        PERROR(TEXT("Couldn't set trust status\r\n"));
        }

    ///////////////////////////////////////////////
    // do the execute to change the server state
    lstrcat(lstrcpy(szBuf,SZCMD_UNSHARE),lpLE->name);
    PINFO(TEXT("sending cmd [%s]\n\r"), szBuf);

    if (MySyncXact ((LPBYTE)szBuf,
                    lstrlen(szBuf) +1,
                    GETMDIINFO(hwndLocal)->hExeConv,
                    0L,
                    CF_TEXT,
                    XTYP_EXECUTE,
                    SHORT_SYNC_TIMEOUT,
                    NULL))
        {
        SetShared(lpLE, FALSE);
        InitializeMenu(GetMenu(hwndApp));
        }
    else
        {
        XactMessageBox (hInst, hwndApp, IDS_APPNAME, MB_OK | MB_ICONSTOP );
        }

    return(0L);

}









/*
 *      ClipBookCommand
 *
 * Purpose: Process menu commands for the Clipbook Viewer.
 *
 * Parameters: As wndproc.
 *
 * Returns: 0L, or DefWindowProc() if wParam isn't a WM_COMMAND id I
 *    know about.
 *
 */

LRESULT ClipBookCommand (
     HWND   hwnd,
     UINT   msg,
     WPARAM wParam,
     LPARAM lParam)
{
int             tmp;
UINT            wNewFormat;
UINT            wOldFormat;
LPLISTENTRY     lpLE;
BOOL            bRet;
DWORD           dwErr;


    switch (LOWORD(wParam))
        {
        case IDM_AUDITING:
            return(EditPermissions(TRUE));
            // return EditAuditing();
            break;

        case IDM_OWNER:
            return EditOwner();
            break;

        case IDM_PERMISSIONS:
            {
            PLISTENTRY pLE;
            RECT       Rect;
            INT        i;

            i = (INT)EditPermissions(FALSE);


            // Permissions may have changed.  Get old data, they need
            //  to be updated.

            SendMessage (pActiveMDI->hWndListbox, LB_GETTEXT,     i, (LPARAM)&pLE);
            SendMessage (pActiveMDI->hWndListbox, LB_GETITEMRECT, i, (LPARAM)&Rect);


            // Delete the old bitmap.  If we are allowed to see it we'll
            //  get it when the list item is redrawn.

            DeleteObject (pLE->hbmp);
            pLE->fTriedGettingPreview = FALSE;
            pLE->hbmp = NULL;


            // Make it redraw.

            InvalidateRect (pActiveMDI->hWndListbox, &Rect, FALSE);
            }
            break;

        case IDC_TOOLBAR:
            MenuHelp( WM_COMMAND, wParam, lParam, GetMenu(hwnd), hInst,
                  hwndStatus, nIDs );
            break;

        case IDM_EXIT:
            SendMessage (hwnd, WM_CLOSE, 0, 0L);
            break;

        case IDM_TILEVERT:
        case IDM_TILEHORZ:
            SendMessage(hwndMDIClient, WM_MDITILE,
                wParam == IDM_TILEHORZ ? MDITILE_HORIZONTAL : MDITILE_VERTICAL,
                0L);
            break;

        case IDM_CASCADE:
            SendMessage (hwndMDIClient, WM_MDICASCADE, 0, 0L);
            break;

        case IDM_ARRANGEICONS:
            SendMessage (hwndMDIClient, WM_MDIICONARRANGE, 0, 0L);
            break;

        case IDM_COPY:
            szSaveFileName[0] = '\0';
            OnIDMCopy (hwnd, msg, wParam, lParam);
            break;

        case IDM_TOOLBAR:
            if ( fToolBar )
                {
                fToolBar = FALSE;
                ShowWindow ( hwndToolbar, SW_HIDE );
                AdjustMDIClientSize();
                }
            else
                {
                fToolBar = TRUE;
                AdjustMDIClientSize();
                ShowWindow ( hwndToolbar, SW_SHOW );
                }
            break;

        case IDM_STATUSBAR:

            if ( fStatus )
                {
                fStatus = FALSE;
                ShowWindow ( hwndStatus, SW_HIDE );
                AdjustMDIClientSize();
                }
            else
                {
                fStatus = TRUE;
                AdjustMDIClientSize();
                ShowWindow ( hwndStatus, SW_SHOW );
                }
            break;

        case ID_PAGEUP:
        case ID_PAGEDOWN:
            {
            HWND hwndc;
            PMDIINFO pMDIc;
            UINT iLstbox, iLstboxOld;

            // copy to make sure this value doesn't change when we yield
            hwndc = hwndActiveChild;

            if (!(pMDIc = GETMDIINFO(hwndc)))
                break;

            SetFocus ( hwndc );

            // make sure this is not clipboard window...
            if ( pMDIc->flags & F_CLPBRD )
                break;

            // must be in page view
            if ( pMDIc->DisplayMode != DSP_PAGE )
                break;

            iLstbox = (int)SendMessage ( pMDIc->hWndListbox,
                LB_GETCURSEL, 0, 0L );
            if ( iLstbox == LB_ERR )
                break;

            // page up on first entry?
            if ( iLstbox == 0 && wParam == ID_PAGEUP )
                {
                MessageBeep(0);
                break;
                }

            // page down on last entry?
            if ( (int)iLstbox == (int)SendMessage(pMDIc->hWndListbox,
                LB_GETCOUNT,0,0L) - 1 && wParam == (WPARAM)ID_PAGEDOWN )
                {
                MessageBeep(0);
                break;
                }

            // move selection up/down as appropriate
            iLstboxOld;
            if ( wParam == ID_PAGEDOWN )
                iLstbox++;
            else
                iLstbox--;

            SetListboxEntryToPageWindow ( hwndc, pMDIc, iLstbox );
            }
            break;

        case IDM_LISTVIEW:
        case IDM_PREVIEWS:
            {
            HWND    hwndtmp;
            int     OldSel;
            int     OldDisplayMode;
            TCHAR   szBuf[80];

            SetFocus (hwndActiveChild);

            if (!pActiveMDI)
                break;


            // make sure this is not clipboard window...
            if (pActiveMDI->flags & F_CLPBRD)
                break;

            // NOP?
            if (pActiveMDI->DisplayMode == DSP_PREV && wParam == IDM_PREVIEWS ||
                pActiveMDI->DisplayMode == DSP_LIST && wParam == IDM_LISTVIEW)
                break;

            OldDisplayMode = pActiveMDI->DisplayMode;

            // nuke vclipboard if there is one
            if ( pActiveMDI->pVClpbrd )
                {
                DestroyVClipboard( pActiveMDI->pVClpbrd );
                pActiveMDI->pVClpbrd = NULL;
                }


            // Save selection... (extra code to avoid strange lb div-by-zero)
            OldSel = (int)SendMessage( pActiveMDI->hWndListbox, LB_GETCURSEL, 0, 0L);
            SendMessage (pActiveMDI->hWndListbox, LB_SETCURSEL, (WPARAM)-1, 0L);
            UpdateNofMStatus (hwndActiveChild);
            SendMessage (pActiveMDI->hWndListbox, WM_SETREDRAW, 0, 0L);


            // set new display mode so listbox will get created right
            pActiveMDI->DisplayMode = (wParam == IDM_PREVIEWS)? DSP_PREV :DSP_LIST;


            // save handle to old listbox
            hwndtmp =  pActiveMDI->hWndListbox;


            // hide the old listbox - will soon destroy
            ShowWindow ( hwndtmp, SW_HIDE );


            // make new listbox and save handle in extra window data
            pActiveMDI->hWndListbox = CreateNewListBox (hwndActiveChild,
                                                        (pActiveMDI->DisplayMode == DSP_PREV)?
                                                         LBS_PREVIEW:
                                                         LBS_LISTVIEW);

            // loop, extracting items from one box and into other
            while (SendMessage (hwndtmp, LB_GETTEXT, 0, (LPARAM)(LPCSTR)&lpLE ) != LB_ERR)
                {
                // mark this item not to be deleted in WM_DELETEITEM
                lpLE->fDelete = FALSE;

                // remove from listbox
                SendMessage (hwndtmp, LB_DELETESTRING, 0, 0L);

                // reset fDelete flag
                lpLE->fDelete = TRUE;

                // add to new listbox
                SendMessage (pActiveMDI->hWndListbox, LB_ADDSTRING, 0, (LPARAM)(LPCSTR)lpLE);
                }



            // kill old (empty) listbox
            DestroyWindow ( hwndtmp );


            if ( pActiveMDI->flags & F_LOCAL )
                {
                SetWindowText ( hwndLocal, szLocalClpBk );
                lstrcpy(szBuf, szDefView);
                }
            else
                {
                wsprintf(szBuf, szClipBookOnFmt,(LPSTR)(pActiveMDI->szBaseName));
                SetWindowText ( hwndActiveChild, szBuf );
                lstrcpy(szBuf, pActiveMDI->szBaseName);
                lstrcat(szBuf, szConn);
                }

            if (NULL != hkeyRoot)
                {
                DWORD dwValue;

                dwValue = pActiveMDI->DisplayMode == DSP_LIST ? IDM_LISTVIEW :
                          pActiveMDI->DisplayMode == DSP_PREV ? IDM_PREVIEWS :
                          IDM_PAGEVIEW;

                RegSetValueEx (hkeyRoot, (LPTSTR)szBuf, 0L, REG_DWORD,
                               (LPBYTE)&dwValue, sizeof(DWORD));
                }


            // adjust size and show
            AdjustControlSizes( hwndActiveChild );
            ShowHideControls ( hwndActiveChild );

            // restore selection
            SendMessage( pActiveMDI->hWndListbox, LB_SETCURSEL, OldSel, 0L );
            UpdateNofMStatus ( hwndActiveChild );

            InitializeMenu ( GetMenu(hwndApp) );
            SetFocus ( pActiveMDI->hWndListbox );
            break;
            }

        case IDM_UPDATE_PAGEVIEW:
        case IDM_PAGEVIEW:
            {
            HWND hwndc;
            PMDIINFO pMDIc;

            // copy to make sure this value doesn't change when we yield
            hwndc = hwndActiveChild;

            if (!(pMDIc = GETMDIINFO(hwndc)))
                break;

            SetFocus (hwndc);


            // make sure this is not clipboard window...

            if (pMDIc->flags & F_CLPBRD)
                break;


            // if switch to page view

            if (IDM_PAGEVIEW == LOWORD(wParam))
                {
                // already in page view?
                if (pMDIc->DisplayMode == DSP_PAGE)
                    break;
                }
            else
                {
                // make sure we're not in an sync xaction, if so
                //  post a message and try again later.
                if (WAIT_TIMEOUT == WaitForSingleObject (hXacting, 0))
                    {
                    PostMessage (hwndApp, WM_COMMAND, IDM_UPDATE_PAGEVIEW, 0L);
                    break;
                    }

                // hXacting is now reset, set it so it can be used again
                SetEvent (hXacting);
                }


            tmp = (int)SendMessage (pMDIc->hWndListbox, LB_GETCURSEL, 0, 0L);
            if (tmp == LB_ERR)
                break;



            SetListboxEntryToPageWindow (hwndc, pMDIc, tmp);
            break;
            }

        case IDM_SHARE:
            if (!pActiveMDI)
                break;

            tmp = (int) SendMessage (pActiveMDI->hWndListbox,LB_GETCURSEL, 0, 0L);

            if ( tmp != LB_ERR )
                {
                SendMessage (pActiveMDI->hWndListbox, LB_GETTEXT, tmp, (LPARAM)&lpLE);

                // We create the NetDDE share when we create the page, not when we
                // share it. Thus, we're always 'editing the properties' of an existing
                // share, even if the user thinks that he's sharing the page NOW.
                Properties(hwnd, lpLE);

                // Redraw the listbox.
                if (pActiveMDI->DisplayMode == DSP_PREV)
                    {
                    InvalidateRect(pActiveMDI->hWndListbox, NULL, FALSE);
                    }
                else
                    {
                    SendMessage(pActiveMDI->hWndListbox,LB_SETCURSEL, tmp, 0L);
                    UpdateNofMStatus(hwndActiveChild);
                    }
                }
            break;

        case IDM_CLPWND:
            CreateClipboardWindow();
            break;

        case IDM_LOCAL:
            CreateLocalWindow();
            break;

        case IDM_UNSHARE:
            bRet = (BOOL)OnIdmUnshare(LB_ERR);
            UpdateListBox (hwndActiveChild, pActiveMDI->hExeConv);
            return bRet;
            break;

        case IDM_DELETE:
            bRet = (BOOL)OnIDMDelete(hwnd, msg, wParam, lParam);
            return bRet;
            break;


        case IDM_PASTE_PAGE:
        case IDM_KEEP:
            bRet = (BOOL)OnIDMKeep (hwnd,
                                    msg,
                                    wParam,
                                    lParam,
                                    IDM_KEEP == LOWORD(wParam) /* a new page? */);
            return bRet;
            break;

        case IDM_SAVEAS:
            {
            OPENFILENAME ofn;
            CHAR         szFile[MAX_FILENAME_LENGTH + 1];

            if (CountClipboardFormats())
                {
                szFile[0] = '\0';
                // Initialize the OPENFILENAME members
                ofn.lStructSize       = sizeof(OPENFILENAME);
                ofn.hwndOwner         = hwnd;
                ofn.lpstrFilter       = szFilter;
                ofn.lpstrCustomFilter = (LPTSTR) NULL;
                ofn.nMaxCustFilter    = 0L;
                ofn.nFilterIndex      = 1;
                ofn.lpstrFile         = (LPTSTR)szFile;
                ofn.nMaxFile          = sizeof(szFile);
                ofn.lpstrFileTitle    = NULL;
                ofn.nMaxFileTitle     = 0L;
                ofn.lpstrInitialDir   = szDirName;
                ofn.lpstrTitle        = (LPTSTR) NULL;
                ofn.lpstrDefExt       = "CLP";
                ofn.Flags             = OFN_HIDEREADONLY |
                                        OFN_NOREADONLYRETURN |
                                        OFN_OVERWRITEPROMPT;

                if (GetSaveFileName (&ofn) && szFile[0])
                    {
                    // NOTE must force all formats rendered!
                    ForceRenderAll (hwnd, NULL);

                    AssertConnection (hwndLocal);

                    // If user picked first filter ("NT Clipboard"), use save as..
                    // other filters would use save as old.
                    wsprintfA (szBuf, "%s%s",
                               (ofn.nFilterIndex == 1) ?
                                (LPSTR)SZCMD_SAVEAS :
                                (LPSTR)SZCMD_SAVEASOLD,
                               (LPSTR)szFile );



                    // these two lines replaces the follow if() else so we can save
                    // files directly without going to the server.

                    dwErr = SaveClipboardToFile (hwndApp, NULL, szFile, FALSE);
                    SysMessageBox (hInst, hwnd, dwErr, IDS_APPNAME, MB_OK|MB_ICONHAND);


                    /****** replaced with above two lines, JohnFu
                    if (!(pMDIc = GETMDIINFO(hwndLocal)))
                        {
                        dwErr = SaveClipboardToFile (hwndApp, NULL, szFile, FALSE);
                        SysMessageBox (hInst, hwnd, dwErr, IDS_APPNAME, MB_OK|MB_ICONHAND);
                        }
                    else
                        {
                        bRet = MySyncXact ((LPBYTE)szBuf,
                                            lstrlen(szBuf) +1,
                                            pMDIc->hExeConv,
                                            0L,
                                            CF_TEXT,
                                            XTYP_EXECUTE,
                                            LONG_SYNC_TIMEOUT,
                                            NULL);

                        if (!bRet)
                            XactMessageBox (hInst, hwnd, IDS_APPNAME, MB_OK|MB_ICONHAND);
                        }
                    ******/
                    }
                }
            break;
            }
        case IDM_OPEN:
            {
            OPENFILENAME ofn;
            TCHAR        szFile[MAX_PATH+1] = TEXT("*.clp");

            // Initialize the OPENFILENAME members
            ofn.lStructSize       = sizeof(OPENFILENAME);
            ofn.hwndOwner         = hwnd;
            ofn.lpstrFilter       = szFilter;
            ofn.lpstrCustomFilter = (LPTSTR) NULL;
            ofn.nMaxCustFilter    = 0L;
            ofn.nFilterIndex      = 1;
            ofn.lpstrFile         = (LPTSTR)szFile;
            ofn.nMaxFile          = sizeof(szFile);
            ofn.lpstrFileTitle    = NULL;
            ofn.nMaxFileTitle     = 0L;
            ofn.lpstrInitialDir   = szDirName;
            ofn.lpstrTitle        = (LPTSTR) NULL;
            ofn.Flags             = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
            ofn.lpstrDefExt       = TEXT("CLP");

            if (GetOpenFileName (&ofn) && szFile[0])
                 {
                 // prompt for clearing clipboard
                 if (ClearClipboard(hwnd))
                    {
                    AssertConnection ( hwndLocal );

                    wsprintf(szBuf,TEXT("%s%s"), (LPTSTR)SZCMD_OPEN, (LPTSTR)szFile);

                    // these two lines replaces the follow if() else so we can open
                    // files directly without going to the server.

                    dwErr = OpenClipboardFile (hwndApp, szFile);
                    SysMessageBox (hInst, hwnd, dwErr, IDS_APPNAME, MB_OK|MB_ICONHAND);


                    /****** replaced with above two lines, JohnFu
                    if (!(pMDIc = GETMDIINFO(hwndLocal)))
                        {
                        dwErr = OpenClipboardFile (hwndApp, szFile);
                        SysMessageBox (hInst, hwnd, dwErr, IDS_APPNAME, MB_OK|MB_ICONHAND);
                        }
                    else
                        {
                        bRet = MySyncXact ((LPBYTE)szBuf,
                                           lstrlen(szBuf) +1,
                                           pMDIc->hExeConv,
                                           0L,
                                           CF_TEXT,
                                           XTYP_EXECUTE,
                                           SHORT_SYNC_TIMEOUT,
                                           NULL);

                        if (!bRet)
                            XactMessageBox (hInst, hwnd, IDS_APPNAME, MB_OK | MB_ICONHAND);
                        }
                    ******/

                    InitializeMenu (GetMenu(hwnd));
                    }
                 }
            break;
            }

        case IDM_DISCONNECT:

            if (!pActiveMDI)
                break;

            // don't allow close of local or clipboard window
            if (pActiveMDI->flags & (F_LOCAL | F_CLPBRD))
                break;
            SendMessage ( hwndActiveChild, WM_CLOSE, 0, 0L );
            break;

        case IDM_CONNECT:
            {
            WCHAR rgwch[MAX_COMPUTERNAME_LENGTH + 3];
            BOOL  bOK = FALSE;
            BOOL  fFoundLMDlg = FALSE;
            HMODULE hMod;
            LPFNSYSFOCUS lpfn;
            #ifndef UNICODE
              WCHAR rgwchHelp[64];
            #endif

            *szConvPartner = '\0';
            rgwch[0] = L'\0';

            if (hMod = LoadLibraryW(L"NTLANMAN.DLL"))
               {
               if (lpfn = (LPFNSYSFOCUS)GetProcAddress(hMod, "I_SystemFocusDialog"))
                  {
                  #ifndef UNICODE
                    MultiByteToWideChar(CP_ACP, 0, szHelpFile, -1, rgwchHelp, 64);
                  #endif

                  fFoundLMDlg = TRUE;
                  (*lpfn)(hwnd,
                          FOCUSDLG_BROWSE_LOGON_DOMAIN |
                          FOCUSDLG_BROWSE_WKSTA_DOMAIN |
                          FOCUSDLG_BROWSE_OTHER_DOMAINS |
                          FOCUSDLG_BROWSE_TRUSTING_DOMAINS |
                          FOCUSDLG_BROWSE_WORKGROUP_DOMAINS |
                          FOCUSDLG_SERVERS_ONLY,
                          rgwch,
                          MAX_COMPUTERNAME_LENGTH + 3,
                          &bOK,
                          #ifndef UNICODE
                            rgwchHelp,
                          #else
                            szHelpFile,
                          #endif
                          IDH_SELECT_COMPUTER);

                  if (IDOK == bOK)
                     {
                     #ifndef UNICODE
                     WideCharToMultiByte(CP_ACP,
                         WC_COMPOSITECHECK | WC_DISCARDNS, rgwch,
                         -1, szConvPartner, MAX_COMPUTERNAME_LENGTH + 2, NULL, &bOK);
                     #else
                     lstrcpy(szConvPartner, rgwch);
                     #endif
                     }
                  else
                     {
                     szConvPartner[0] = TEXT('\0');
                     }
                  }
               else
                  {
                  PERROR(TEXT("Couldn't find connect proc!\r\n"));
                  }
               FreeLibrary(hMod);
               }
            else
               {
               PERROR(TEXT("Couldn't find NTLANMAN.DLL\r\n"));
               }

            // If we didn't find the fancy LanMan dialog, we still can get
            // by with our own cheesy version-- 'course, ours comes up faster, too.
            if (!fFoundLMDlg)
               {
               bOK = (BOOL)DialogBox(hInst, MAKEINTRESOURCE(IDD_CONNECT), hwnd,
                                     ConnectDlgProc);
               }

            if ( *szConvPartner )
               {
               CreateNewRemoteWindow ( szConvPartner, TRUE );
               }
            else
               {
                MessageBoxID (hInst,
                              hwnd,
                              IDS_NOCONNECTION,
                              IDS_APPNAME,
                              MB_OK | MB_ICONHAND);
               }
            UpdateWindow ( hwnd );
            break;
            }

        case IDM_REFRESH:

            if (!pActiveMDI)
                break;

            #if DEBUG
                {
                DWORD cbDBL = sizeof(DebugLevel);

                RegQueryValueEx(hkeyRoot, szDebug, NULL, NULL,
                    (LPBYTE)&DebugLevel, &cbDBL);
                }
            #endif
            if (pActiveMDI->flags & F_CLPBRD)
                break;

            AssertConnection ( hwndActiveChild );
            UpdateListBox ( hwndActiveChild, pActiveMDI->hExeConv );
            break;

        case IDM_CONTENTS:
            HtmlHelp(GetDesktopWindow(), szChmHelpFile, HH_DISPLAY_TOPIC, 0L);
            break;

        case IDM_ABOUT:
           {
           HMODULE hMod;
           LPFNSHELLABOUT lpfn;

           if (hMod = LoadLibrary(TEXT("SHELL32")))
              {
              if (lpfn = (LPFNSHELLABOUT)GetProcAddress(hMod,
                 #ifdef UNICODE
                   "ShellAboutW"
                 #else
                   "ShellAboutA"
                 #endif
                 ))
                 {
                 (*lpfn)(hwnd, szAppName, szNull,
                      LoadIcon(hInst, MAKEINTRESOURCE(IDFRAMEICON)));
                 }
              FreeLibrary(hMod);
              }
           else
              {
              PERROR(TEXT("Couldn't get SHELL32.DLL\r\n"));
              }
           }
           break;

        case CBM_AUTO:
        case CF_PALETTE:
        case CF_TEXT:
        case CF_BITMAP:
        case CF_METAFILEPICT:
        case CF_SYLK:
        case CF_DIF:
        case CF_TIFF:
        case CF_OEMTEXT:
        case CF_DIB:
        case CF_OWNERDISPLAY:
        case CF_DSPTEXT:
        case CF_DSPBITMAP:
        case CF_DSPMETAFILEPICT:
        case CF_PENDATA:
        case CF_RIFF:
        case CF_WAVE:
        case CF_ENHMETAFILE:
        case CF_UNICODETEXT:
        case CF_DSPENHMETAFILE:
        case CF_LOCALE:

            if (!pActiveMDI)
               break;

            if ( pActiveMDI->CurSelFormat != wParam)
                {
                CheckMenuItem (hDispMenu, pActiveMDI->CurSelFormat, MF_BYCOMMAND | MF_UNCHECKED);
                CheckMenuItem (hDispMenu, (UINT)wParam,                   MF_BYCOMMAND | MF_CHECKED);

                DrawMenuBar(hwnd);

                wOldFormat = GetBestFormat( hwndActiveChild, pActiveMDI->CurSelFormat);
                wNewFormat = GetBestFormat( hwndActiveChild, (UINT)wParam);

                if (wOldFormat == wNewFormat)
                    {
                    /* An equivalent format is selected; No change */
                    pActiveMDI->CurSelFormat = (UINT)wParam;
                    }
                else
                    {
                    /* A different format is selected; So, refresh... */

                    /* Change the character sizes based on new format. */
                    ChangeCharDimensions (hwndActiveChild, wOldFormat, wNewFormat);

                    pActiveMDI->fDisplayFormatChanged = TRUE;
                    pActiveMDI->CurSelFormat = (UINT)wParam;

                    // NOTE OwnerDisplay stuff applies only to the "real" clipboard!

                    if (wOldFormat == CF_OWNERDISPLAY)
                        {
                        /* Save the owner Display Scroll info */
                        SaveOwnerScrollInfo(hwndClpbrd);
                        ShowScrollBar ( hwndClpbrd, SB_BOTH, FALSE );
                        ShowHideControls(hwndClpbrd);
                        ResetScrollInfo( hwndActiveChild );
                        InvalidateRect ( hwndActiveChild, NULL, TRUE );
                        break;
                        }

                    if (wNewFormat == CF_OWNERDISPLAY)
                        {
                        /* Restore the owner display scroll info */
                        ShowHideControls(hwndClpbrd);
                        ShowWindow ( pActiveMDI->hwndSizeBox, SW_HIDE );
                        RestoreOwnerScrollInfo(hwndClpbrd);
                        InvalidateRect ( hwndActiveChild, NULL, TRUE );
                        break;
                        }

                    InvalidateRect  (hwndActiveChild, NULL, TRUE);
                    ResetScrollInfo (hwndActiveChild );
                    }
                }
            break;

        default:
            return DefFrameProc ( hwnd,hwndMDIClient,msg,wParam,lParam);
        }


    // return DefFrameProc ( hwnd,hwndMDIClient,msg,wParam,lParam);
    return 0;

}









/*
 *      SetListboxEntryToPageWindow
 */

BOOL SetListboxEntryToPageWindow(
    HWND        hwndc,
    PMDIINFO    pMDIc,
    int         lbindex)
{
HCONV       hConv;
LPLISTENTRY lpLE;
PVCLPBRD    pVClp;
PDATAREQ    pDataReq;
BOOL        fOK = FALSE;
TCHAR       tchTmp;
BOOL        fLocked;

// get listbox entry data



    if (WAIT_TIMEOUT == WaitForSingleObject (hXacting, 0))
        return fOK;



    fLocked = LockApp (TRUE, NULL);



    if (LB_ERR == SendMessage (pMDIc->hWndListbox, LB_GETTEXT, lbindex, (LPARAM)(LPCSTR)&lpLE)
        || !lpLE
        || !(pDataReq = CreateNewDataReq()))
        {
        PERROR(TEXT("error from CreateNewDataReq\n\r"));
        goto done;
        }



    // make new clipboard
    if (!(pVClp = CreateVClipboard(hwndc)))
        {
        PERROR(TEXT("Failed to create Vclipboard\n\r"));
        goto done;
        }



    // nuke previous vclipboard if any
    if ( pMDIc->pVClpbrd )
        DestroyVClipboard( pMDIc->pVClpbrd );


    pMDIc->pVClpbrd = pVClp;


    // Set up $<page name> for topic
    if (pMDIc->hszClpTopic)
        DdeFreeStringHandle ( idInst, pMDIc->hszClpTopic );



    tchTmp = lpLE->name[0];
    lpLE->name[0] = SHR_CHAR;
    pMDIc->hszVClpTopic = DdeCreateStringHandle ( idInst, lpLE->name, 0 );
    lpLE->name[0] = tchTmp;

    if (!pMDIc->hszVClpTopic)
       {
       PERROR(TEXT("Couldn't make string handle for %s\r\n"), lpLE->name);
       goto done;
       }





    if (pMDIc->hVClpConv)
       {
       DdeDisconnect (pMDIc->hVClpConv);
       pMDIc->hVClpConv = NULL;
       }


    hConv = DdeConnect (idInst, pMDIc->hszConvPartner, pMDIc->hszVClpTopic, NULL);
    if (!hConv)
       {
       PERROR(TEXT("DdeConnect for Vclip failed: %x\n\r"), DdeGetLastError(idInst) );
       goto done;
       }


    pMDIc->hVClpConv = hConv;

    DdeKeepStringHandle (idInst, hszFormatList);

    pDataReq->rqType      = RQ_SETPAGE;
    pDataReq->hwndList    = pMDIc->hWndListbox;
    pDataReq->iListbox    = lbindex;
    pDataReq->hwndMDI     = hwndc;
    pDataReq->fDisconnect = FALSE;
    pDataReq->wFmt        = CF_TEXT;

    DdeSetUserHandle (hConv, (DWORD)QID_SYNC, (DWORD_PTR)pDataReq);

    if (!DdeClientTransaction (NULL,
                               0L,
                               hConv,
                               hszFormatList,
                               CF_TEXT,
                               XTYP_REQUEST,
                               (DWORD)TIMEOUT_ASYNC,
                               NULL ))
        DdeMessageBox (hInst,
                       pDataReq->hwndMDI,
                       DdeGetLastError (idInst),
                       IDS_APPNAME,
                       MB_OK|MB_ICONEXCLAMATION);

    fOK = TRUE;




done:

    if (!fOK)
        MessageBoxID ( hInst, hwndc, IDS_INTERNALERR, IDS_APPNAME, MB_OK | MB_ICONSTOP );

    if (fLocked)
        LockApp (FALSE, NULL);



    SetEvent (hXacting);

    return(fOK);

}
