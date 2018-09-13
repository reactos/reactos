/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991,1992                    **/
/***************************************************************************/
// ts=4

// DDE.C - ClipBook viewer DDEML callback function
// 4-92 clausgi created


#include "windows.h"
#include "clipbook.h"
#include "common.h"

// internal forwards

static HWND GetConvHwnd ( HCONV hConv );

// ddeml callback routine

HDDEDATA EXPENTRY DdeCallback(
WORD wType,
WORD wFmt,
HCONV hConv,
HSZ hszTopic,
HSZ hszItem,
HDDEDATA hData,
DWORD lData1,
DWORD lData2)
{
HWND hwndTmp;
CONVINFO ConvInfo;
PDATAREQ pDataReq;

switch (wType)
   {
case XTYP_ADVDATA:
   if ( hwndTmp = GetConvHwnd ( hConv ) )
      {
      InitListBox ( hwndTmp, hData );
      }
   return FALSE;
   break;

case XTYP_DISCONNECT:
   ConvInfo.cb = sizeof(CONVINFO);
   if ( DdeQueryConvInfo ( hConv, (DWORD)QID_SYNC, &ConvInfo ) == 0 )
      {
      PERROR(TEXT("DdeQueryConvInfo for %lx failed: %x\n\r"),
         (DWORD)hConv, DdeGetLastError(idInst));
      break;
      }
   if ( pDataReq = (PDATAREQ)ConvInfo.hUser )
      {
      PINFO(TEXT("Freeing data req on %lx at disconnect time\n\r"), hConv );
      ProcessDataReq ( 0, pDataReq );
      DeleteDataReq ( pDataReq );
      DdeSetUserHandle ( hConv, (DWORD)QID_SYNC, 0L );
      }
   else
      {
      PINFO(TEXT("Disconnect received on %lx - no datareq\n\r"), hConv );
      }
   break;

case XTYP_XACT_COMPLETE:
   ConvInfo.cb = sizeof(CONVINFO);
   if ( DdeQueryConvInfo ( hConv, (DWORD)QID_SYNC, &ConvInfo ) == 0 )
      {
      PERROR(TEXT("DdeQueryConvInfo for %lx failed: %x\n\r"),
         (DWORD)hConv, DdeGetLastError(idInst));
      break;
      }

   PINFO(TEXT("dde callback: got %lx data from conv handle %lx\n\r"),
      ConvInfo.hUser, (DWORD)hConv );

   pDataReq = (PDATAREQ)ConvInfo.hUser;

   ProcessDataReq ( hData,  pDataReq );
   DeleteDataReq ( pDataReq );
   if ( pDataReq->fDisconnect )
      {
      DdeDisconnect ( hConv );
      }
   DdeSetUserHandle ( hConv, (DWORD)QID_SYNC, 0L );
   break;

case XTYP_REGISTER:
case XTYP_UNREGISTER:
case XTYP_ADVREQ:
case XTYP_REQUEST:
case XTYP_ADVSTART:
case XTYP_CONNECT_CONFIRM:
case XTYP_CONNECT:
default:
   break;
 }
return 0;
}

// this function retrieves the window handle associated with
// a conversation handle - the hande is put there by
// using DdeSetUserHandle at DdeConnect time

static HWND GetConvHwnd ( HCONV hConv )
{
   CONVINFO ConvInfo;
   PDATAREQ pDataReq;

   ConvInfo.cb = sizeof(CONVINFO);
   if ( DdeQueryConvInfo ( hConv, (DWORD)QID_SYNC, &ConvInfo ) == 0 )
      {
      PERROR(TEXT("DdeQueryConvInfo for %lx failed: %x\n\r"),
         (DWORD)hConv, DdeGetLastError(idInst));
      }

   pDataReq = (PDATAREQ)ConvInfo.hUser;

   PINFO(TEXT("GetConvHwnd: got %p as conv handle\r\n"), pDataReq);

   if ( !IsWindow ( pDataReq->hwndMDI ) )
      {
      PERROR(TEXT("Invalid window %lx in conv Uhandle: %lx!\n\r"),
         (DWORD)pDataReq->hwndMDI, (DWORD)hConv );
      return NULL;
      }
   return pDataReq->hwndMDI;
}

//
// Purpose: Get the version of Clipsrv connected to the given MDI
//    child.
//
// Parameters:
//    hwndChild - The child window.
//
// Returns:
//    A version number with the Clipsrv OS version in the hiword, and
//    the Clipsrv version in the loword.
//
//    Hiword values:
//       0 - Win 3.x
//       1 - NT 1.x
//
//    Loword values:
//       0 - WFW 1.0 Clipsrv
//       1 - NT  1.0 Clipsrv, adds [version] and [security] executes
//
/////////////////////////////////////////////////////////////////////
DWORD GetClipsrvVersion(
HWND hwndChild)
{
MDIINFO *pMDI;
HDDEDATA hdde;
DWORD    dwRet;
char    *lpszDDE;

dwRet = 0;
pMDI = GETMDIINFO(hwndChild);

if (pMDI && !(pMDI->flags & F_CLPBRD))
   {
   hdde = MySyncXact(SZCMD_VERSION, lstrlen(SZCMD_VERSION) + 1,
         pMDI->hExeConv, 0L, CF_TEXT, XTYP_EXECUTE,
         SHORT_SYNC_TIMEOUT, NULL);

   if (hdde)
      {
      lpszDDE = (char *)DdeAccessData(hdde, &dwRet);

      if (lpszDDE)
         {
         dwRet = MAKELONG(lpszDDE[0] - '0', lpszDDE[2] - '0');
         }
      else
         {
         dwRet = 0L;
         }

      DdeUnaccessData(hdde);
      DdeFreeDataHandle(hdde);
      }
   else
      {
      PINFO(TEXT("Clipsrv didn't like version execute\r\n"));
      }
   }
else
   {
   PERROR(TEXT("No Clipsrv for clipboard!\r\n"));
   }

return dwRet;
}
