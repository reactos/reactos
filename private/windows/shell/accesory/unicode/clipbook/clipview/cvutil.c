
/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991,1992                    **/
/***************************************************************************/
// ts=4

// CVUTIL.C - support functions for ClipBook viewer
// 11-91 clausgi created

#define WIN31
#define STRICT

#include "windows.h"
#include "winuserp.h"
#include <windowsx.h>
#include <assert.h>
#include <memory.h>
#include "clipbook.h"
#include "clipdsp.h"
#include "commctrl.h"
#include "..\common\common.h"
#include "uniconv.h"


extern BOOL fAuditEnabled;

// Windows 3.1-type structures - Win31 packed on byte boundaries.
#pragma pack(1)
typedef struct
   {
   WORD FormatID;
   DWORD DataLen;
   DWORD DataOffset;
   CHAR Name[CCHFMTNAMEMAX];
   } OLDFORMATHEADER;

// Windows 3.1 BITMAP struct - used to save Win 3.1 .CLP files
typedef struct {
   WORD bmType;
   WORD bmWidth;
   WORD bmHeight;
   WORD bmWidthBytes;
   BYTE bmPlanes;
   BYTE bmBitsPixel;
   LPVOID bmBits;
   } WIN31BITMAP;

// Windows 3.1 METAFILEPICT struct
typedef struct {
   WORD mm;
   WORD xExt;
   WORD yExt;
   WORD hMF;
   } WIN31METAFILEPICT;

#pragma pack()

extern HICON hicLock;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#if DEBUG
void DumpDataReq(
PDATAREQ pdr)
{
PINFO(TEXT("Datareq: type %d, to window %lx, \r\nat index %u, fDisc=%u, format=%u\r\n"),
      pdr->rqType, pdr->hwndMDI, pdr->iListbox, pdr->fDisconnect, pdr->wFmt);
}
#else
#define DumpDataReq(x)
#endif

// AdjustEditWindows //////////////////
//
// This function sizes the listbox windows associated
// with an MDI child window when its size changes

VOID AdjustControlSizes ( HWND hwnd )
{
RECT rc1, rc2;
PMDIINFO pMDI;
int cx = GetSystemMetrics ( SM_CXVSCROLL );
int cy = GetSystemMetrics ( SM_CYHSCROLL );

pMDI = GETMDIINFO(hwnd);

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
   MoveWindow ( pMDI->hwndHscroll,  rc1.left - 1, rc2.bottom,
      ( rc2.right - rc2.left ) + 2, cy, TRUE );

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

VOID ShowHideControls ( HWND hwnd )
{
   PMDIINFO pMDI = GETMDIINFO(hwnd);
   int nShowScroll;
   int nShowList;

   switch ( pMDI->DisplayMode ) {
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
   ShowWindow ( pMDI->hwndPgUp,
      (pMDI->flags & F_CLPBRD)? SW_HIDE: nShowScroll );
   ShowWindow ( pMDI->hwndPgDown,
      (pMDI->flags & F_CLPBRD)? SW_HIDE: nShowScroll );
}

// AssertConnection /////////////////

BOOL AssertConnection ( HWND hwnd )
{
if (IsWindow(hwnd))
   {
   if ( GETMDIINFO(hwnd)->hExeConv ||
        (GETMDIINFO(hwnd)->hExeConv = InitSysConv( hwnd,
            GETMDIINFO(hwnd)->hszConvPartner, hszClpBookShare, FALSE))
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
HWND hwnd,
HSZ hszApp,
HSZ hszTopic,
BOOL fLocal )
{
HCONV hConv = 0L;
PDATAREQ pDataReq;
#if DEBUG
TCHAR atchApp[256];
TCHAR atchTopic[256];

if (DdeQueryString(idInst, hszApp, atchApp,
         CharSizeOf(atchApp), CP_WINANSI) &&
    DdeQueryString(idInst, hszTopic, atchTopic,
         CharSizeOf(atchTopic), CP_WINANSI))
   {
   PINFO(TEXT("InitSysConv: [%s | %s]\r\n"), atchApp, atchTopic);
   }
else
   {
   PERROR(TEXT("I don't know my app/topic pair!\r\n"));
   }
#endif

if (LockApp(TRUE, szEstablishingConn))
   {
   hConv = DdeConnect ( idInst, hszApp, hszTopic, NULL );
   if (!hConv)
      {
      unsigned uiErr;

      DdeGetLastError(idInst);
      PINFO(TEXT("Failed first try at CLIPSRV, #%x\r\n"),uiErr);

      MessageBoxID(hInst, hwnd, IDS_NOCLPBOOK, IDS_APPNAME,
            MB_OK | MB_ICONHAND);
      }
   else
      {
      PINFO(TEXT("Making datareq."));

      if ( pDataReq = CreateNewDataReq() )
         {
         pDataReq->rqType  = RQ_EXECONV;
         pDataReq->hwndMDI = hwnd;
         pDataReq->wFmt    = CF_TEXT;
         DdeSetUserHandle ( hConv, (DWORD)QID_SYNC, (DWORD)pDataReq );

         Sleep(3000);
         PINFO(TEXT("Entering AdvStart transaction "));

         if ( MySyncXact ( NULL, 0L, hConv, hszTopics,
                  CF_TEXT, XTYP_ADVSTART, LONG_SYNC_TIMEOUT, NULL ) == FALSE )
            {
            unsigned uiErr;

            uiErr = DdeGetLastError(idInst);
            PERROR(TEXT("AdvStart failed: %x\n\r"), uiErr);
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
BOOL UpdateListBox ( HWND hwnd, HCONV hConv )
{
HDDEDATA hData;
BOOL fOK = TRUE;

if ( hConv == 0L || !IsWindow( hwnd ))
   {
   PERROR(TEXT("UpdateListBox called with garbage\n\r"));
   fOK = FALSE;
   }
else
   {
   if (GETMDIINFO(hwnd)->flags & F_LOCAL)
      {
      PINFO(TEXT("Getting all topics\r\n"));
      }
   else
      {
      PINFO(TEXT("Getting shared topics\r\n"));
      }

   hData = MySyncXact ( NULL, 0L, hConv, hszTopics,
      CF_TEXT, XTYP_REQUEST, SHORT_SYNC_TIMEOUT, NULL );

   if ( !hData )
      {
      unsigned uiErr;

      uiErr = DdeGetLastError(idInst);
      PERROR(TEXT("XACT for topiclist failed: %x\n\r"), uiErr);

      MessageBoxID ( hInst, hwnd, IDS_DATAUNAVAIL, IDS_APPNAME,
         MB_OK | MB_ICONEXCLAMATION );
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
VOID GetPreviewBitmap (
HWND hwnd,
LPTSTR szName,
UINT index )
{
HSZ      hszTopic, hszItem;
HCONV    hConv;
HDDEDATA hRet;
PDATAREQ pDataReq;
BOOL     fAlreadyLocked;
TCHAR    tchTmp;

fAlreadyLocked = !LockApp(TRUE, szNull);

tchTmp = szName[0];
szName[0] = SHR_CHAR;
if (( hszTopic = DdeCreateStringHandle ( idInst, szName, 0 )))
   {
   if (( hszItem = DdeCreateStringHandle ( idInst, SZPREVNAME, 0 )))
      {
      if ( ( pDataReq = CreateNewDataReq()) )
         {
         #if DEBUG
         TCHAR atch[64];

         DdeQueryString(idInst, GETMDIINFO(hwnd)->hszConvPartnerNP,
               atch, 64, CP_WINANSI);
         PINFO(TEXT("GetPrevBmp: Connecting [%s | %s ! %s]\r\n"),
               atch, szName, SZPREVNAME);
         #endif

         if ( hConv = DdeConnect (idInst,
              GETMDIINFO(hwnd)->hszConvPartnerNP, hszTopic, NULL ))
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

               NDdeSetTrustedShare(NULL, szName,
                     adwTrust[0] | NDDE_TRUST_SHARE_INIT);
               }

            hRet = DdeClientTransaction ( NULL, 0L, hConv,
               hszItem, cf_preview, XTYP_REQUEST, (DWORD)TIMEOUT_ASYNC, NULL );

            if ( !hRet )
               {
               unsigned uiErr;

               uiErr = DdeGetLastError(idInst);
               PERROR(TEXT("GetPreviewBitmap: Async Transaction for (%s) failed:%x\n\r"),
                  szName, uiErr);
               }

            pDataReq->rqType = RQ_PREVBITMAP;
            pDataReq->hwndList = GETMDIINFO(hwnd)->hWndListbox;
            pDataReq->iListbox = index;
            pDataReq->hwndMDI = hwnd;
            pDataReq->fDisconnect = TRUE;
            pDataReq->wFmt        = cf_preview;

            DdeSetUserHandle ( hConv, (DWORD)QID_SYNC, (DWORD)pDataReq );
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
               szBuf, szName, uiErr);
            }
         #endif

         if (!fAlreadyLocked)
            {
            LockApp(FALSE,NULL);
            }
         }
      else
         {
         PERROR(TEXT("GetPreviewBitmap: no pdatareq\n\r"));
         }
      DdeFreeStringHandle ( idInst, hszItem );
      }
   else
      {
      PERROR(TEXT("GetPreviewBitmap: no item handle\n\r"));
      }
   DdeFreeStringHandle(idInst, hszTopic);
   }
else
   {
   PERROR(TEXT("GetPreviewBitmap: no topic handle\n\r"));
   }
szName[0] = tchTmp;
}

VOID SetBitmapToListboxEntry (
HDDEDATA hbmp,
HWND hwndList,
UINT index )
{
LPLISTENTRY lpLE;
RECT rc;
HBITMAP hBitmap;
LPBYTE lpBitData;
DWORD cbDataLen;
unsigned uiErr;

#if DEBUG
uiErr = DdeGetLastError(idInst);
if (uiErr)
   {
   PINFO(TEXT("SBmp2LBEntr: %d\r\n"), uiErr);
   }
#endif


if ( !IsWindow( hwndList )                     ||
   SendMessage ( hwndList, LB_GETTEXT,
      index, (LPARAM)(LPCSTR)&lpLE ) == LB_ERR ||
   SendMessage ( hwndList, LB_GETITEMRECT,
      index, (LPARAM)(LPRECT)&rc ) == LB_ERR )
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

#define BOGUS_CHAR TEXT('?')


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
HWND hwnd,
HDDEDATA hData )
{
LPTSTR lpszList, q, p;
DWORD cbDataLen;
TCHAR c;
PLISTENTRY pLE;
HWND hwndlist;
int OldCount, OldSel;
unsigned i;
BOOL fDel;
TCHAR ach[MAX_PAGENAME_LENGTH + 1];
unsigned j;

if ( hData == 0L || !IsWindow ( hwnd ) )
   {
   PERROR(TEXT("InitListBox called with garbage\n\r"));
   return FALSE;
   }

// Get a copy of the data in the handle
lpszList = (LPTSTR)DdeAccessData ( hData, &cbDataLen );
DdeUnaccessData(hData);
lpszList = GlobalAllocPtr(GHND, cbDataLen);
DdeGetData(hData, (LPBYTE) lpszList, cbDataLen, 0L);

// Sometimes, the data will be longer than the string. This
// would make the 'put tabs back' code below fail if we didn't
// do this.
cbDataLen = lstrlen(lpszList);

PINFO(TEXT("InitLB: %s \r\n"), lpszList);

if ( !lpszList )
   {
   PERROR(TEXT("error accessing data in InitListBox\n\r"));
   return FALSE;
   }

if ( !(hwndlist = GETMDIINFO(hwnd)->hWndListbox) )
   return FALSE;

SendMessage ( hwndlist, WM_SETREDRAW, 0, 0L );
OldCount = (int)SendMessage ( hwndlist, LB_GETCOUNT, 0, 0L );
OldSel = (int)SendMessage ( hwndlist, LB_GETCURSEL, 0, 0L );
// SendMessage ( hwndlist, (UINT)LB_RESETCONTENT, 0, 0L );

// Delete items in list that don't exist anymore
for (i = 0; i < OldCount; i++)
   {
   SendMessage(hwndlist, LB_GETTEXT, i, (LPARAM)&pLE);
   fDel = TRUE;

   if (pLE)
      {
      for (q = _tcstok(lpszList, TEXT("\t")); q; q = _tcstok(NULL, TEXT("\t")))
         {
         PINFO(TEXT("<%hs>"), q);

         if (0 == lstrcmp(pLE->name, q))
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
         if (TEXT('\0') == *q)
            {
            *q = TEXT('\t');
            }
         }
      *q = TEXT('\0');
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
for (q = _tcstok(lpszList, TEXT("\t")); q; q = _tcstok(NULL, TEXT("\t")))
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
         SendMessage(hwndlist, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)pLE);
         }
      }
   }

GlobalFreePtr(lpszList);

// Select the item at the same position we were at
if ((OldCount = SendMessage ( hwndlist, LB_GETCOUNT, 0, 0L)) > 0 )
   {
   SendMessage ( hwndlist, LB_SETCURSEL,
         (OldSel >= OldCount) ? OldCount - 1 : OldSel, 0L );
   }
SendMessage ( hwndlist, WM_SETREDRAW, 1, 0L );
UpdateNofMStatus( hwnd );
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

UINT MyGetFormat (
LPTSTR szFmt,
int mode )
{
UINT uiPrivates[] = {
      CF_BITMAP,
      CF_METAFILEPICT,
      CF_PALETTE,
      CF_ENHMETAFILE,
      CF_DIB};
TCHAR szBuf[40];
unsigned i;

PINFO(TEXT("\nMyGetFormat [%s] %d:"), szFmt, mode);

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

VOID HandleOwnerDraw (
HWND hwnd,
WPARAM wParam,
LPARAM lParam )
{
   LPDRAWITEMSTRUCT lpds;
   RECT tmprc;
   COLORREF OldTextColor;
   COLORREF OldBkColor;
   COLORREF BackColor;
   COLORREF TextColor;
   HBRUSH hBkBrush;
   DWORD cbData = 0L;
   LPLISTENTRY lpLE;
   BOOL fSel = FALSE;

   lpds = (LPDRAWITEMSTRUCT) lParam;

   // this section handles listbox drawing

   switch ( lpds->CtlID )
      {
   case ID_LISTBOX:
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
            TextOut ( lpds->hDC, lpds->rcItem.left + 2 * LSTBTDX,
               lpds->rcItem.top+1,
               &(lpLE->name[1]), lstrlen((lpLE->name)) - 1 );
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

            tmprc.top = lpds->rcItem.top + PREVBRD;
            tmprc.bottom = lpds->rcItem.top + PREVBRD + PREVBMPSIZ;
            tmprc.left = lpds->rcItem.left + 5 * PREVBRD;
            tmprc.right = lpds->rcItem.right - 5 * PREVBRD;

            Rectangle ( lpds->hDC, tmprc.left, tmprc.top,
               tmprc.right, tmprc.bottom );

            // draw preview bitmap if available
            if ( lpLE->hbmp == NULL )
               {
               if ( !lpLE->fTriedGettingPreview )
                  {
                  GetPreviewBitmap ( hwnd, lpLE->name, lpds->itemID );
                  lpLE->fTriedGettingPreview = TRUE;
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

            if ( IsShared ( lpLE ) && fShareEnabled ) {
               DrawIcon ( lpds->hDC,
                  tmprc.left - 10,
                  tmprc.top + PREVBMPSIZ - 24,
                  LoadIcon ( hInst, (LPTSTR) MAKEINTRESOURCE(IDSHAREICON))
               );
            }

         }

         if ( lpds->itemAction & ( ODA_SELECT | ODA_DRAWENTIRE | ODA_FOCUS )) {

            tmprc = lpds->rcItem;
            tmprc.left += PREVBRD;
            tmprc.right -= PREVBRD;
            tmprc.top += PREVBMPSIZ + 2 * PREVBRD;
            tmprc.bottom--;

            if ( ( lpds->itemState & ODS_SELECTED ) &&
               ( lpds->itemState & ODS_FOCUS ) ) {
               TextColor = GetSysColor ( COLOR_HIGHLIGHTTEXT );
               BackColor = GetSysColor ( COLOR_HIGHLIGHT );
            }
            else {
               TextColor = GetSysColor ( COLOR_WINDOWTEXT );
               BackColor = GetSysColor ( COLOR_WINDOW );
            }

            OldTextColor = SetTextColor ( lpds->hDC, TextColor );
            OldBkColor = SetBkColor ( lpds->hDC, BackColor );
            hOldFont = SelectObject ( lpds->hDC, hFontPreview );

            if ( hBkBrush = CreateSolidBrush ( BackColor )) {
               FillRect ( lpds->hDC, &tmprc, hBkBrush );
               DeleteObject ( hBkBrush );
            }

#ifdef JAPAN    /* 10-Aug-93 v-katsuy */
            DrawText ( lpds->hDC, &(lpLE->name[1]), lstrlen(lpLE->name) -1,
               &tmprc, DT_CENTER | DT_WORDBREAK | DT_NOJWORDBRK | DT_NOPREFIX);
#else
            DrawText ( lpds->hDC, &(lpLE->name[1]), lstrlen(lpLE->name) -1,
               &tmprc, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX );
#endif

            SetTextColor ( lpds->hDC, OldTextColor );
            SetBkColor ( lpds->hDC, OldBkColor );
            SelectObject ( lpds->hDC, hOldFont );
         }
      }
      break;

   case ID_PAGEUP:
   case ID_PAGEDOWN:

      if ( lpds->itemAction & ( ODA_SELECT | ODA_DRAWENTIRE )) {
         if ( lpds->itemState & ODS_SELECTED )
            hOldBitmap = SelectObject ( hBtnDC, (lpds->CtlID==ID_PAGEUP)?
               hPgUpDBmp : hPgDnDBmp );
         else
            hOldBitmap = SelectObject ( hBtnDC, (lpds->CtlID==ID_PAGEUP)?
               hPgUpBmp : hPgDnBmp );
         StretchBlt ( lpds->hDC, lpds->rcItem.top, lpds->rcItem.left,
            GetSystemMetrics ( SM_CXVSCROLL ),
            GetSystemMetrics ( SM_CYHSCROLL ),
            hBtnDC,
            0,
            0,
            17,   // x and y of resource bitmaps
            17,
            SRCCOPY );
         SelectObject ( hBtnDC, hOldBitmap );
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

HWND CreateNewListBox (
HWND hwnd,
DWORD style )
{
HWND hLB;

hLB = CreateWindow(TEXT("listbox"),
        szNull,
        WS_CHILD | LBS_STANDARD | LBS_NOINTEGRALHEIGHT | style,
        0, 0, 100, 100,
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

BOOL SetClipboardFormatFromDDE (
HWND hwnd,
UINT uiFmt,
HDDEDATA hDDE )
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


PINFO(TEXT("SetClpFmtDDE: format %d, handle %ld | "), uiFmt, hDDE);

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
   }
else
   {
   PINFO(TEXT("%d bytes of data. "), cbData);

   if (hData = GlobalAlloc(GHND, cbData))
      {
      if (lpData = GlobalLock(hData))
         {
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

         default:
            fOK = TRUE;
            }

         if (!hData)
            {
            PERROR(TEXT("SetClipboardFormatFromDDE returning FALSE\n\r"));
            }

         if (fOK)
            {
            PINFO(TEXT("SCFFDDE: Setting VClpD\r\n"));
            VSetClipboardData( GETMDIINFO(hwnd)->pVClpbrd, uiFmt, hData);
            }
         else
            {
            if (!(GETMDIINFO(hwnd)->flags & F_CLPBRD))
               {
               VSetClipboardData( GETMDIINFO(hwnd)->pVClpbrd, uiFmt,
                     INVALID_HANDLE_VALUE);
               }
            }
         }
      else
         {
         PERROR(TEXT("GlobalLock failed\n\r"));
         }
      // No GlobalFree() call here, 'cause we've put hData on the clp
      }
   else
      {
      PERROR(TEXT("GlobalAlloc failed\n\r"));
      }
   DdeUnaccessData(hDDE);
   }
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
HWND  PASCAL NewWindow( VOID )
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
              (LONG)(LPMDICREATESTRUCT)&mcs);

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

if ( IsIconic(hwndApp) )
   return;

//   wpl.length = sizeof(WINDOWPLACEMENT);
//   GetWindowPlacement ( hwndApp, &wpl );

//   if ( wpl.showCmd != SW_SHOWMAXIMIZED )
//      rcApp = wpl.rcNormalPosition;
//   else
   GetClientRect ( hwndApp, &rcApp );

rcMDI.top = 0;
rcMDI.bottom =  rcApp.bottom - rcApp.top;
//      - ( GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU)))
//      - ( 2 * GetSystemMetrics(SM_CYFRAME ));
rcMDI.left = 0;
rcMDI.right = rcApp.right - rcApp.left;
//      - ( 2 * GetSystemMetrics(SM_CXFRAME));

MoveWindow ( hwndMDIClient,
   rcMDI.left - 1,
   rcMDI.top + (fToolBar?dyButtonBar:0),
   ( rcMDI.right - rcMDI.left ) + 2,
   (( rcMDI.bottom - rcMDI.top ) - (fStatus?dyStatus:0) )
      - (fToolBar?dyButtonBar:0),
   TRUE );

if ( fNeedToTileWindows ) {
   SendMessage ( hwndMDIClient, WM_MDITILE, 0, 0 );
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

HDDEDATA GetConvDataItem (
HWND hwnd,
LPTSTR szTopic,
LPTSTR szItem,
UINT uiFmt )
{
HCONV hConv;
HSZ hszTopic;
HSZ hszItem;
HDDEDATA hRet = 0;

// assert(fAppLockedState);

PINFO(TEXT("GConvDI: %s ! %s, %x\r\n"), szTopic, szItem, uiFmt);

if (!( hszTopic = DdeCreateStringHandle ( idInst, szTopic, 0 )))
   {
   PERROR(TEXT("GetConvDataItem: DdeCreateStringHandle failed\n\r"));
   return 0;
   }

if (!( hszItem = DdeCreateStringHandle ( idInst, szItem, 0 )))
   {
   DdeFreeStringHandle ( idInst, hszTopic );
   PERROR(TEXT("GetConvDataItem: DdeCreateStringHandle failed\n\r"));
   return 0;
   }

if ( hConv = DdeConnect (idInst,
   ( uiFmt == cf_preview ) && !(GETMDIINFO(hwnd)->flags & F_LOCAL ) ?
   GETMDIINFO(hwnd)->hszConvPartnerNP : GETMDIINFO(hwnd)->hszConvPartner,
                                    hszTopic,NULL ))
   {
   hRet = MySyncXact ( NULL, 0L, hConv,
      hszItem, uiFmt, XTYP_REQUEST, SHORT_SYNC_TIMEOUT, NULL );
   if ( !hRet )
      {
      PERROR(TEXT("Transaction for (%s):(%s) failed: %x\n\r"),
         szTopic, szItem, DdeGetLastError ( idInst ));
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
int nCode,
WPARAM wParam,
LPARAM lParam)
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

      if ( !TranslateMDISysAccel (hwndMDIClient, (LPMSG)lParam )
           && (hAccel ?  !TranslateAccelerator(hwndApp, hAccel, (LPMSG)lParam)
               : 1)
         )
         {
         TranslateMessage ((LPMSG)lParam );
         DispatchMessage ((LPMSG)lParam );
         }
      return(1);
      }
   return(0);
}

#if DEBUG
// DebOut ////////////////////////////////
//
// this function prints the varargs supplied on the
// debug terminal. It should be called through the
// macros PERROR or PINFO(TEXT so that the call will be conditional
// on the value of DebugLevel, and will not generate any code if
// DEBUG is not defined.

VOID DebOut ( LPTSTR format, ... )
{
static TCHAR buf[256];

wvsprintf( buf, format, (LPBYTE)&format + sizeof(format));
OutputDebugString(buf);
}
#endif


// MySyncXact ///////////////////////////////
//
// this function is a wrapper to DdeClientTransaction which
// performs some checks related to the Locked state of the app

HDDEDATA MySyncXact (
LPBYTE lpbData,
DWORD cbDataLen,
HCONV hConv,
HSZ hszItem,
UINT wFmt,
UINT wType,
DWORD dwTimeout,
LPDWORD lpdwResult )
{
HDDEDATA hDDE;
BOOL fAlreadyLocked;

#if DEBUG
if (dwTimeout != TIMEOUT_ASYNC)
   {
   dwTimeout +=10000;
   }
#endif

// Do some checks...
fAlreadyLocked = !LockApp(TRUE, NULL);

hDDE = DdeClientTransaction ( lpbData, cbDataLen, hConv, hszItem, wFmt,
    wType, dwTimeout, lpdwResult );

if (!hDDE)
   {
   unsigned uiErr;
   TCHAR    lpbItem[64];

   uiErr = DdeGetLastError(idInst);
   #if DEBUG
   PERROR(TEXT("MySyncXact fail err %d.\r\n"), uiErr);

   DdeQueryString(idInst, hszItem, lpbItem, 64, CP_WINANSI);
   PINFO(TEXT("Parameters: data at %lx (%s), len %ld, HCONV %lx\r\n"),
         lpbData, (CF_TEXT == wFmt && lpbData) ? lpbData : TEXT("Not text"),
         cbDataLen, hConv);

   PINFO(TEXT("item %lx (%s), fmt %d, type %d, timeout %ld\r\n"),
         hszItem, lpbItem, wFmt, wType, dwTimeout);
   #endif
   }

if (!fAlreadyLocked)
   {
   LockApp(FALSE, NULL);
   }

return hDDE;
}

// ResetScrollInfo ///////////////////////////
//
// this function resets the scroll information of the
// MDI child window designated by the supplied handle
VOID ResetScrollInfo ( HWND hwnd )
{
   PMDIINFO pMDI = GETMDIINFO(hwnd);

   // Invalidate object info; reset scroll position to 0.
   // cyScrollLast = cxScrollLast = -1;
   pMDI->cyScrollLast = -1L;
   pMDI->cyScrollNow = 0L;
   pMDI->cxScrollLast = -1;
   pMDI->cxScrollNow = 0;

   // Range is set in case CF_OWNERDISPLAY owner changed it.
   PINFO(TEXT("SETSCROLLRANGE for window '%s'\n\r"),
         (LPTSTR)(pMDI->szBaseName) );
   SetScrollRange(pMDI->hwndVscroll, SB_CTL, 0, VPOSLAST, FALSE);
   SetScrollPos(pMDI->hwndVscroll, SB_CTL, (int)(pMDI->cyScrollNow), TRUE);
   SetScrollRange(pMDI->hwndHscroll, SB_CTL, 0, HPOSLAST, FALSE);
   SetScrollPos(pMDI->hwndHscroll, SB_CTL, pMDI->cxScrollNow, TRUE);
}

// IsShared ///////////////////////////////////
//
// this function sets the shared state of the ownerdraw
// listbox entry denoted by the supplied pointer. Shared/nonshared
// status is expressed as a 1 character prefix to the description string
//
// return TRUE if shared, false otherwise

BOOL IsShared ( LPLISTENTRY lpLE )
{
   if ( lpLE->name[0] == SHR_CHAR )
      return TRUE;
#if DEBUG
   if ( lpLE->name[0] != UNSHR_CHAR )
      PERROR(TEXT("bad prefix char in share name: %s\n\r"),
            lpLE->name );
#endif
   return FALSE;
}

// SetShared //////////////////////////////////////////
//
// sets shared state to fShared, returns previous state

BOOL SetShared ( LPLISTENTRY lpLE, BOOL fShared )
{
   BOOL fSave = lpLE->name[0] == SHR_CHAR ? TRUE : FALSE;
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


BOOL LockApp (
BOOL fLock,
LPTSTR lpszComment )
{
   static HCURSOR hOldCursor;
   BOOL fOK = FALSE;

   if ( lpszComment )
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
         // SetCapture ( hwndDummy );
         EnableWindow ( hwndApp, FALSE );
         fAppLockedState = TRUE;
         fOK = TRUE;
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
         SetCursor ( hOldCursor );
         EnableWindow ( hwndApp, TRUE );
         ReleaseCapture ();
         fAppLockedState = FALSE;
         fOK = TRUE;

         // take care of any deferred clipboard update requests
         if ( fClipboardNeedsPainting )
            {
            PostMessage ( hwndApp, WM_DRAWCLIPBOARD, 0, 0L );
            }
         }
      }
   return fOK;
}

// ForceRenderAll ///////////////////////////////////
//
// this function forces a complete rendering of any delayed
// render clipboard formats
BOOL ForceRenderAll ( HWND hwnd, PVCLPBRD pVclp )
{
HANDLE h;
UINT uiFmt;

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
}

BOOL UpdateNofMStatus(HWND hwnd)
{
   HWND hwndlistbox;
   int total = 0;
   int sel = LB_ERR;

   if ( hwnd == NULL ) {
      SendMessage ( hwndStatus, SB_SETTEXT, 0, (LPARAM)NULL );
      return TRUE;
   }

   if ( GETMDIINFO(hwnd)->flags & F_CLPBRD ) {
         SendMessage ( hwndStatus, SB_SETTEXT, 0,
            (LPARAM)(LPTSTR) szSysClpBrd );
         return TRUE;
   }

   if ( IsWindow( hwndlistbox = GETMDIINFO(hwnd)->hWndListbox ) ) {
      total = (int)SendMessage ( hwndlistbox, LB_GETCOUNT, (WPARAM)0, 0L );
      sel = (int)SendMessage ( hwndlistbox, LB_GETCURSEL, 0, 0L);
   }

   if ( sel == (int)LB_ERR ) {
      if ( total == 1 )
         SendMessage (hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPCSTR)szPageFmt);
      else {
         wsprintf( szBuf, szPageFmtPl, total );
         SendMessage (hwndStatus, SB_SETTEXT, 0, (LPARAM)(LPCSTR)szBuf );
      }
   }
   else {
      wsprintf(szBuf, szPageOfPageFmt, sel+1, total );
      SendMessage ( hwndStatus, SB_SETTEXT, 0,
         (LPARAM)(LPCSTR)szBuf );
   }
   return TRUE;
}

BOOL RestoreAllSavedConnections ( VOID )
{
TCHAR szName[80];
BOOL ret = TRUE;
unsigned i;

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

BOOL CreateNewRemoteWindow (
LPTSTR szMachineName,
BOOL fReconnect )
{
HWND hwndc;
PMDIINFO pMDIc;
WINDOWPLACEMENT wpl;

// make new window active
hwndc = NewWindow();
if (NULL == hwndc)
   {
   return FALSE;
   }

pMDIc = GETMDIINFO(hwndc);


// save base name for window
lstrcpy ( pMDIc->szBaseName, szMachineName );

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

pMDIc->hExeConv = InitSysConv( hwndc, pMDIc->hszConvPartner,
      hszClpBookShare, FALSE);

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

         PINFO(TEXT("saving connection: '%s'\n\r"), szBuf );
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

int MessageBoxID(
HANDLE hInstance,
HWND hwndParent,
UINT TextID,
UINT TitleID,
UINT fuStyle)
{
LoadString ( hInstance, TextID, szBuf, SZBUFSIZ );
LoadString ( hInstance, TitleID, szBuf2, SZBUFSIZ );

MessageBeep ( fuStyle & MB_SNDMASK );
return MessageBox ( hwndParent, szBuf, szBuf2, fuStyle );
}

PDATAREQ CreateNewDataReq ( VOID )
{
return (PDATAREQ) GlobalAlloc ( GPTR, sizeof(DATAREQ) );
}


BOOL DeleteDataReq ( PDATAREQ pDataReq )
{
return ( (HGLOBAL)pDataReq == GlobalFree ( pDataReq ) );
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
HDDEDATA hData,
PDATAREQ pDataReq )
{
LPTSTR lpszList;
LPTSTR q, p;
DWORD cbDataLen;
UINT tmp;
PMDIINFO pMDI;
LPLISTENTRY lpLE;

PINFO(TEXT("PDR:"));

if ( !pDataReq || !IsWindow(pDataReq->hwndMDI) )
   {
   PERROR(TEXT("ProcessDataReq: bogus DATAREQ\n\r"));
   return FALSE;
   }

if (!hData)
   {
   PERROR(TEXT("ProcessDataReq: Woe, woe, we have gotten null data!\r\n"));
   DumpDataReq(pDataReq);
   if (RQ_COPY == pDataReq->rqType)
      {
      MessageBoxID(hInst, hwndApp, IDS_DATAUNAVAIL, IDS_APPNAME,
            MB_OK | MB_ICONHAND);
      }
   return FALSE;
   }

pMDI = GETMDIINFO(pDataReq->hwndMDI);

switch ( pDataReq->rqType )
   {
case RQ_PREVBITMAP:
   PINFO(TEXT("Got bitmap for item %d in %x\r\n"),pDataReq->iListbox,
         pDataReq->hwndList);
   SetBitmapToListboxEntry( hData, pDataReq->hwndList, pDataReq->iListbox);
   return TRUE;

case RQ_EXECONV:
   // must be from disconnect
   GETMDIINFO(pDataReq->hwndMDI)->hExeConv = 0L;
   PINFO(TEXT("setting hExeConv NULL!\n\r"));
   break;

case RQ_COPY:
   PINFO(TEXT("RQ_COPY:"));
   if ( hData == FALSE )
      {
      PERROR(TEXT("REQUEST for format list failed: %x\n\r"),
         DdeGetLastError(idInst));
      MessageBoxID ( hInst, pDataReq->hwndMDI, IDS_DATAUNAVAIL,
         IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION );
      break;
      }

   lpszList = (LPTSTR) DdeAccessData ( hData, &cbDataLen );

   if ( !lpszList )
      {
      PERROR(TEXT("DdeAccessData failed: %lx\n\r"), (DWORD)lpszList );
      MessageBoxID ( hInst, pDataReq->hwndMDI, IDS_DATAUNAVAIL,
         IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION );
      break;
      }

   PINFO(TEXT("formatlist:>%s<\n\r"), lpszList );
   // this client now becomes the clipboard owner!!!

   if ( SyncOpenClipboard ( hwndApp ) == TRUE )
      {
      // Need to lock app while we fill the clipboard with formats,
      // else hwndClpbrd will try to frantically try to redraw while
      // we're doing it. Since hwndClpbrd needs to openclipboard() to
      // do that, we don't want it to.
      LockApp(TRUE, szNull);

      // reset clipboard view format to auto
      pMDI->CurSelFormat = CBM_AUTO;

      EmptyClipboard();

      hwndClpOwner = pDataReq->hwndMDI;
      PINFO(TEXT("Formats:"));

      if (pDataReq->wFmt != CF_TEXT)
         {
         PERROR(TEXT("Format %d, expected CF_TEXT!\r\n"), pDataReq->wFmt);
         }

      for (q = _tcstok(lpszList, TEXT("\t")); q; q = _tcstok(NULL, TEXT("\t")))
         {
         PINFO(TEXT("[%s] "),q);
         tmp = MyGetFormat(q, GETFORMAT_DONTLIE);
         if (0 == tmp)
            {
            PERROR(TEXT("MyGetFormat failure!\r\n"));
            }
         else
            {
            tmp = (UINT)SetClipboardData(tmp, NULL);
            PINFO(TEXT("SetClData rtn %d.\r\n"), tmp);
            }
         }

      PINFO(TEXT("\r\n"));

      SyncCloseClipboard();

      LockApp(FALSE, szNull);

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
   return TRUE;

case RQ_SETPAGE:
   PINFO(TEXT("RQ_SETPAGE:"));

   if ( hData == FALSE )
      {
      PERROR(TEXT("vclip: REQUEST for format list failed: %x\n\r"),
         DdeGetLastError(idInst));
      MessageBoxID ( hInst, pDataReq->hwndMDI, IDS_DATAUNAVAIL,
         IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION );
      return FALSE;
      }

   if ( SendMessage ( pMDI->hWndListbox,
         LB_GETTEXT, pDataReq->iListbox,
         (LPARAM)(LPCSTR)&lpLE) == LB_ERR )
      {
      PERROR(TEXT("IDM_COPY: bad listbox index: %d\n\r"), pDataReq->iListbox );
      break;
      }

   lpszList = (LPTSTR) DdeAccessData ( hData, &cbDataLen );

   if ( !lpszList )
      {
      MessageBoxID ( hInst, pDataReq->hwndMDI, IDS_DATAUNAVAIL,
         IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION );
      return FALSE;
      }

   if ( VOpenClipboard ( pMDI->pVClpbrd, pDataReq->hwndMDI ) == TRUE )
      {

      VEmptyClipboard( pMDI->pVClpbrd );

      // PINFO(TEXT("Formats:"));

      for (q = _tcstok(lpszList, TEXT("\t")); q; q = _tcstok(NULL, TEXT("\t")))
         {
         tmp = MyGetFormat(q, GETFORMAT_DONTLIE);
         VSetClipboardData(pMDI->pVClpbrd, tmp, NULL);
         }

      // PINFO(TEXT("\r\n"));

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
   return TRUE;

default:
   PERROR (TEXT("unknown type %d in ProcessDataReq\n\r"),
         pDataReq->rqType );
   return FALSE;
   }
}

