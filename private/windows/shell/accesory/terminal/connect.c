/*===========================================================================*/
/*          Copyright (c) 1987 - 1990, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define  NOGDICAPMASKS     TRUE
#define  NOVIRTUALKEYCODES TRUE
#define  NOICONS	         TRUE
#define  NOKEYSTATES       TRUE
#define  NOSYSCOMMANDS     TRUE
#define  NOATOM	         TRUE
#define  NOCLIPBOARD       TRUE
#define  NODRAWTEXT	      TRUE
#define  NOMB	            TRUE
#define  NOMINMAX	         TRUE
#define  NOSCROLL	         TRUE
#define  NOHELP            TRUE
#define  NOPROFILER	      TRUE
#define  NODEFERWINDOWPOS  TRUE
#define  NOPEN             TRUE
#define  NO_TASK_DEFINES   TRUE
#define  NOLSTRING         TRUE
#define  WIN31
#define  USECOMM

#include <windows.h>
#include <port1632.h>
#include "dcrc.h"
#include "dynacomm.h"
#include "network.h"
#include "video.h"  
#include "connect.h"


/**************************************************************************/
/*                                                                        */
/* initConnectors                                                         */
/*                                                                        */
/**************************************************************************/

BOOL initConnectors(BOOL bInit)
{
   LPCONNECTORS   lpConnectors;              /* slc nova 031 */

   if(bInit)                                 /* we are initializing stuff */
   {
      ghConnectors = GlobalAlloc(GHND | GMEM_ZEROINIT, (DWORD) sizeof(CONNECTORS));
      if(ghConnectors == NULL)
         return(FALSE);

                                             /* slc nova 031 */
      ghCCB = GlobalAlloc(GHND | GMEM_ZEROINIT, (DWORD)sizeof(CONNECTOR_CONTROL_BLOCK));

      /* the lpConnectors struct not really used by DynaComm at this time */
      /* let's go ahead and allocate room for one CCB */
      lpConnectors = (LPCONNECTORS) GlobalLock(ghConnectors);
      lpConnectors->hCCBArray = ghCCB;

      if(lpConnectors->hCCBArray == NULL)
      {
         GlobalUnlock(ghConnectors);
         GlobalFree(ghConnectors);
         return(FALSE);
      }

      lpConnectors->lpCCB[0] = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(lpConnectors->hCCBArray);

      GlobalUnlock(ghConnectors);
   }
   else
   {  /* we are unloading this stuff */
      GlobalFree(ghCCB);                     /* slc nova 031 */
      GlobalFree(ghConnectors);              /* slc nova 031 */
   }

   return(TRUE);
}


/**************************************************************************/
/*                                                                        */
/* addConnectorList                                                       */
/*                                                                        */
/**************************************************************************/

VOID addConnectorList(HWND hDlg, WORD wId)   /* slc nova 031 */
{
   HANDLE   theHandle;
   OFSTRUCT ofDummy;

   if (MOpenFile((LPSTR)"LANMAN.DLL", (LPOFSTRUCT)&ofDummy, OF_EXIST) != -1)
      SendDlgItemMessage(hDlg, wId, LB_INSERTSTRING, -1, (LONG) (LPSTR)"LANMAN");
}


/**************************************************************************/
/*                                                                        */
/* DLL_ConnectConnector                                      seh nova 005 */
/*                                                                        */
/**************************************************************************/

WORD DLL_ConnectConnector(HANDLE hCCB, BOOL bShow) /* slc nova 031 seh nova 005 */
{
   FARPROC                   lpfnGetType;
   LPCONNECTOR_CONTROL_BLOCK lpCCB;
   WORD                      wResult = 0;

   if(hCCB == NULL)                          /* slc nova 028 */
      return(0);

   if((lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(hCCB)) == NULL)
      return(0);

   if((lpfnGetType = GetProcAddress(lpCCB->hConnectorInst,
                            MAKEINTRESOURCE(ORD_CONNECTCONNECTOR))) != NULL)
   {
      wResult = ((WORD)(*lpfnGetType)(dlgGetFocus(), lpCCB, (BOOL)bShow));
   }

   GlobalUnlock(hCCB);
   return(wResult);
}


/**************************************************************************/
/*                                                                        */
/* DLL_DisconnectConnector                                  seh nova 005  */
/*                                                                        */
/**************************************************************************/

WORD DLL_DisconnectConnector(HANDLE hCCB)
{
   FARPROC                   lpfnGetType;
   LPCONNECTOR_CONTROL_BLOCK lpCCB;
   WORD                      wResult = 0;

   if(hCCB == NULL)
      return(0);

   if((lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(hCCB)) == NULL)
      return(0);

   if((lpfnGetType = GetProcAddress(lpCCB->hConnectorInst,
                           MAKEINTRESOURCE(ORD_DISCONNECTCONNECTOR))) != NULL)
   {
/*      wResult = ((WORD)(*lpfnGetType)(dlgGetFocus(), lpCCB)); */
   }

   GlobalUnlock(hCCB);
   return(wResult);
}


/**************************************************************************/
/*                                                                        */
/* DLL_modemSendBreak                                                     */
/*                                                                        */
/**************************************************************************/

WORD DLL_modemSendBreak(HANDLE hCCB, INT nTimes)
{
   LPCONNECTOR_CONTROL_BLOCK  lpCCB;
   FARPROC                    lpfnCommandConnector;
   WORD                       wResult = 0;

   if(hCCB == NULL)
      return(0);

   if((lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(hCCB)) == NULL)
      return(0);

   if((lpfnCommandConnector = GetProcAddress(lpCCB->hConnectorInst,
                              MAKEINTRESOURCE(ORD_COMMANDCONNECTOR))) != NULL)
   {
   }

   GlobalUnlock(hCCB);
   return(wResult);
}


/**************************************************************************/
/*                                                                        */
/* DLL_ReadConnector                                                      */
/*                                                                        */
/**************************************************************************/

WORD DLL_ReadConnector(HANDLE hCCB)          /* slc nova 031 */
{
   LPCONNECTOR_CONTROL_BLOCK  lpCCB;
   FARPROC                    lpfnRead;
   WORD                       wResult = 0;

   if(hCCB == NULL)                          /* slc nova 028 */
      return(0);

   if((lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(hCCB)) == NULL)
      return(0);

   if((lpfnRead = GetProcAddress(lpCCB->hConnectorInst,
                                 MAKEINTRESOURCE(ORD_READCONNECTOR))) != NULL)
   {
      wResult = (WORD)(*lpfnRead)(lpCCB);
   }

   GlobalUnlock(hCCB);
   return(wResult);
}


/**************************************************************************/
/*                                                                        */
/* DLL_ConnectBytes                                                       */
/*                                                                        */
/**************************************************************************/

WORD DLL_ConnectBytes(HANDLE hCCB)           /* slc nova 031 */
{
   if(hCCB == NULL)
   {
      serCount = 0;
      return(FALSE);
   }

   return(DLL_ReadConnector(hCCB));
}


/**************************************************************************/
/*                                                                        */
/* DLL_WriteConnector                                                     */
/*                                                                        */
/**************************************************************************/

WORD DLL_WriteConnector(HANDLE hCCB)         /* slc nova 031 */
{
   LPCONNECTOR_CONTROL_BLOCK  lpCCB;
   WORD                       wResult = FALSE;
   FARPROC                    lpfnWrite;

   if(hCCB == NULL)                          /* slc nova 028 */
   {
      serCount = 0;
      return(wResult);
   }

   if((lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(hCCB)) == NULL)
      return(0);

   if((lpfnWrite = GetProcAddress(lpCCB->hConnectorInst,
                              MAKEINTRESOURCE(ORD_WRITECONNECTOR))) != NULL)
   {
      wResult = (WORD)(*lpfnWrite)(lpCCB);
   }

   GlobalUnlock(hCCB);
   return(wResult);
}


/**************************************************************************/
/*                                                                        */
/* DLL_ExitConnector                                                      */
/*                                                                        */
/**************************************************************************/

WORD DLL_ExitConnector(HANDLE hCCB, recTrmParams *pTrmParams)
{
   LPCONNECTOR_CONTROL_BLOCK  lpCCB;
   FARPROC                    lpfnGetType;

   if(hCCB == NULL)                          /* slc nova 028 */
      return(0);

   if((lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(hCCB)) == NULL)
      return(0);

   if((lpfnGetType = GetProcAddress(lpCCB->hConnectorInst,
                                 MAKEINTRESOURCE(ORD_EXITCONNECTOR))) != NULL)
   {
   }

   if(lpCCB->hConnectorInst != 0)            /* slc nova 031 */
      FreeLibrary(lpCCB->hConnectorInst);
   lpCCB->hConnectorInst = 0;

   GlobalUnlock(hCCB);

   return(0);
}


/**************************************************************************/
/*                                                                        */
/* DLL_ResetConnector                                                     */
/*                                                                        */
/**************************************************************************/

WORD DLL_ResetConnector(HANDLE hCCB, BOOL bShow)   /* slc nova 031 */
{
   LPCONNECTOR_CONTROL_BLOCK lpCCB;
   FARPROC                   lpfnGetType;
   WORD                      wResult = 0;

   if((lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(hCCB)) == NULL)
      return(0);

   if((lpfnGetType = GetProcAddress(lpCCB->hConnectorInst,
                              MAKEINTRESOURCE(ORD_RESETCONNECTOR))) != NULL)
   {
      wResult = (WORD)(*lpfnGetType)(dlgGetFocus(), lpCCB, (BOOL)bShow);
   }

   GlobalUnlock(hCCB);
   return(wResult);
}


/**************************************************************************/
/*                                                                        */
/* DLL_SetupConnector                                                     */
/*                                                                        */
/**************************************************************************/

WORD DLL_SetupConnector(HANDLE hCCB, BOOL bShow)   /* slc nova 031 */
{
   LPCONNECTOR_CONTROL_BLOCK lpCCB;
   FARPROC                   lpfnProcAddr;
   WORD                      wResult = 0;

   if((lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(hCCB)) == NULL)
      return(0);

   if((lpfnProcAddr = GetProcAddress(lpCCB->hConnectorInst,
                              MAKEINTRESOURCE(ORD_SETUPCONNECTOR))) != NULL)
   {
      ccbFromTrmParams(lpCCB, &trmParams);
      wResult = (WORD)(*lpfnProcAddr)(dlgGetFocus(), lpCCB, (BOOL)bShow);
      ccbToTrmParams(&trmParams, lpCCB);
   }

   GlobalUnlock(hCCB);
   return(wResult);
}


/**************************************************************************/
/*                                                                        */
/* loadConnector                                                          */
/*                                                                        */
/**************************************************************************/

HANDLE loadConnector(HWND hTopWnd, HANDLE hCCB, LPSTR lpszConnector, BOOL bShow)
{
   LPCONNECTOR_CONTROL_BLOCK  lpCCB;
   FARPROC                    lpfnGetType;
   CHAR                       szWork[PATHLEN];
   HANDLE                     hResult = NULL;

   if((lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(hCCB)) == NULL)
      return(0);

   if(lpCCB->hConnectorInst != 0)
      FreeLibrary(lpCCB->hConnectorInst);
   lpCCB->hConnectorInst = 0;

   if(*lpszConnector)
   {
      if(!lstrcmp(lpszConnector, (LPSTR)"LANMAN"))
      {
         lstrcpy((LPSTR)szWork, lpszConnector);
         strcat(szWork, ".DLL");
      }
      lpCCB->hConnectorInst = LoadLibrary((LPSTR)szWork);   /* slc nova 028 */
   }

/* in win30 <32 rc is an error, but in win32 rc is NULL in case of an error -sdj*/

#ifdef ORGCODE
   if(lpCCB->hConnectorInst < 32)
      lpCCB->hConnectorInst = 0;
#else
   if(lpCCB->hConnectorInst == NULL)
      lpCCB->hConnectorInst = (HANDLE)0;
#endif

   if(lpCCB->hConnectorInst != 0)
   {  /* check to see if we have a DC_CONNECTOR type dll */
      if((lpfnGetType = GetProcAddress(lpCCB->hConnectorInst,
                                    MAKEINTRESOURCE(ORD_GETDLLTYPE))) == NULL)
      {
         FreeLibrary(lpCCB->hConnectorInst);
         lpCCB->hConnectorInst = 0;
      }
      else
      {
         if((WORD)(*lpfnGetType)((HWND)hTopWnd, (BOOL)bShow) != DC_CONNECTOR)
         {
            FreeLibrary(lpCCB->hConnectorInst);
            lpCCB->hConnectorInst = 0;
         }
      }
   }
   hResult = lpCCB->hConnectorInst;
   GlobalUnlock(hCCB);

   if(hResult)                               /* slc nova 031 */
      DLL_ResetConnector(hCCB, FALSE);

   return(hResult);
}


/**************************************************************************/
/*                                                                        */
/* DLL_HasSetupBox                                          seh nova 006  */
/*                                                                        */
/**************************************************************************/

BOOL DLL_HasSetupBox(HANDLE hCCB)
{
   LPCONNECTOR_CONTROL_BLOCK  lpCCB;
   FARPROC                    lpProc;
   BOOL		                  bHasSetup;

   if(hCCB == NULL)                          /* slc nova 028 */
      return(0);
                                   
   lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(hCCB);  /* slc nova 031 */

   if((lpProc = GetProcAddress(lpCCB->hConnectorInst,
                              MAKEINTRESOURCE(ORD_GETEXTENDEDINFO))) == NULL)
   {
      bHasSetup = FALSE;
   }
   else
   {
      bHasSetup = (WORD)(*lpProc)((WORD)GI_IDENTIFY, (WORD)GI_SETUPBOX, (LPSTR)NULL);
   }

   return(bHasSetup);
}


/**************************************************************************/
/*                                                                        */
/* getConnectorCaps                                                       */
/*                                                                        */
/**************************************************************************/

WORD getConnectorCaps(LPCONNECTOR_CONTROL_BLOCK lpCCB)   /* slc nova 031 */
{
   FARPROC  lpProc;
   WORD     wReturn;

   if((lpProc = GetProcAddress(lpCCB->hConnectorInst,
                              MAKEINTRESOURCE(ORD_GETCONNECTCAPS))) != NULL)
   {
      wReturn = (WORD)(*lpProc)((WORD)SET_PARAMETERS);
   }

   return(wReturn);
}


/**************************************************************************/
/*                                                                        */
/* getConnectorSettings                                                   */
/*                                                                        */
/**************************************************************************/

WORD getConnectorSettings(LPCONNECTOR_CONTROL_BLOCK lpCCB, BOOL bShow) /* slc nova 031 */
{
   FARPROC  lpProc;
   WORD     wReturn;
   WORD     wPassVal = SP_GETDEFAULT;

   if(bShow)
      wPassVal |= SP_SHOW;

   if((lpProc = GetProcAddress(lpCCB->hConnectorInst,
                                 MAKEINTRESOURCE(ORD_SETPARAMETERS))) != NULL)
   {
      wReturn = (WORD)(*lpProc)((WORD)wPassVal, (LPCONNECTOR_CONTROL_BLOCK)lpCCB);
   }

   return(wReturn);
}


/**************************************************************************/
/*                                                                        */
/* setConnectorSettings                                                   */
/*                                                                        */
/**************************************************************************/

WORD setConnectorSettings(HWND hDlg, HANDLE hCCB, BOOL bDefaults) /* slc nova 031 */
{
   LPCONNECTOR_CONTROL_BLOCK  lpCCB;         /* slc nova 031 */
   LPCONNECTORS               lpConnectors;  /* slc nova 031 */
   FARPROC                    lpProc;
   WORD                       wSettings;
   WORD                       wWorkSettings, wWork;

   if(hCCB == NULL)                          /* slc nova 028 */
      return(0);
                                   
   lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(hCCB);  /* slc nova 031 */
   if(lpCCB->hConnectorInst == 0)            /* slc nova 046 */
   {
      GlobalUnlock(hCCB);
      return(0);
   }

   if((lpProc = GetProcAddress(lpCCB->hConnectorInst,
                              MAKEINTRESOURCE(ORD_GETEXTENDEDINFO))) != NULL)
   {
      wSettings = (WORD)(*lpProc)((WORD)GI_IDENTIFY, (WORD)GI_SETUPBOX, (LPSTR)NULL);
   }

   if(wSettings)
   {
      EnableWindow(GetDlgItem(hDlg, ITMSETUP), TRUE);    /* seh nova 005 */
      ShowWindow(GetDlgItem(hDlg, ITMSETUP), SW_SHOW);   /* seh nova 005 */
   }
   else
   {
      EnableWindow(GetDlgItem(hDlg, ITMSETUP),  FALSE);  /* seh nova 005 */
      ShowWindow(GetDlgItem(hDlg, ITMSETUP), SW_HIDE);   /* seh nova 005 */
   }

   wSettings = getConnectorCaps(lpCCB);      /* slc nova 031 */
   getConnectorSettings(lpCCB, FALSE);       /* slc nova 031 */
   if(bDefaults)                             /* slc nova 031 */
   {
      ccbFromTrmParams(lpCCB, &trmParams);
      ccbToTrmParams(&trmParams, lpCCB);
   }
   
   /* enable baud rates */
   for(wWork = ITMBD110; wWork <= ITMBD192; wWork++)
      EnableWindow(GetDlgItem(hDlg, wWork), FALSE);

   if((wSettings & SP_BAUD) == SP_BAUD)
   {
      wWorkSettings = lpCCB->wBaudFlags;

      if((wWorkSettings & BAUD_110) == BAUD_110)
         EnableWindow(GetDlgItem(hDlg, ITMBD110), TRUE);

      if((wWorkSettings & BAUD_300) == BAUD_300)
         EnableWindow(GetDlgItem(hDlg, ITMBD300), TRUE);

      if((wWorkSettings & BAUD_600) == BAUD_600)
         EnableWindow(GetDlgItem(hDlg, ITMBD600), TRUE);

      if((wWorkSettings & BAUD_120) == BAUD_120)
         EnableWindow(GetDlgItem(hDlg, ITMBD120), TRUE);

      if((wWorkSettings & BAUD_240) == BAUD_240)
         EnableWindow(GetDlgItem(hDlg, ITMBD240), TRUE);

      if((wWorkSettings & BAUD_480) == BAUD_480)
         EnableWindow(GetDlgItem(hDlg, ITMBD480), TRUE);

      if((wWorkSettings & BAUD_960) == BAUD_960)
         EnableWindow(GetDlgItem(hDlg, ITMBD960), TRUE);

      if((wWorkSettings & BAUD_192) == BAUD_192)
         EnableWindow(GetDlgItem(hDlg, ITMBD192), TRUE);

      CheckRadioButton(hDlg, ITMBD110, ITMBD192, putCCB_BAUDITM(lpCCB->wBaudSet));
   }

   /*  enable data bits */
   for(wWork = ITMDATA5; wWork <= ITMDATA8; wWork++)
      EnableWindow(GetDlgItem(hDlg, wWork), FALSE);

   if((wSettings & SP_DATABITS) == SP_DATABITS)
   {
      wWorkSettings = lpCCB->wDataBitFlags;

      if((wWorkSettings & DATABITS_5) == DATABITS_5)
         EnableWindow(GetDlgItem(hDlg, ITMDATA5), TRUE);

      if((wWorkSettings & DATABITS_6) == DATABITS_6)
         EnableWindow(GetDlgItem(hDlg, ITMDATA6), TRUE);

      if((wWorkSettings & DATABITS_7) == DATABITS_7)
         EnableWindow(GetDlgItem(hDlg, ITMDATA7), TRUE);

      if((wWorkSettings & DATABITS_8) == DATABITS_8)
         EnableWindow(GetDlgItem(hDlg, ITMDATA8), TRUE);

      CheckRadioButton(hDlg, ITMDATA5, ITMDATA8, putCCB_DATABITS(lpCCB->wDataBitSet));
   }

   /* enable stop bits */
   for(wWork = ITMSTOP1; wWork <= ITMSTOP2; wWork++)
      EnableWindow(GetDlgItem(hDlg, wWork), FALSE);

   if((wSettings & SP_STOPBITS) == SP_STOPBITS)
   {
      wWorkSettings = lpCCB->wStopBitFlags;

      if((wWorkSettings & STOPBITS_10) == STOPBITS_10)
         EnableWindow(GetDlgItem(hDlg, ITMSTOP1), TRUE);

      if((wWorkSettings & STOPBITS_15) == STOPBITS_15)
         EnableWindow(GetDlgItem(hDlg, ITMSTOP5), TRUE);

      if((wWorkSettings & STOPBITS_20) == STOPBITS_20)
         EnableWindow(GetDlgItem(hDlg,ITMSTOP2), TRUE);

      CheckRadioButton(hDlg, ITMSTOP1, ITMSTOP2, putCCB_STOPBITS(lpCCB->wStopBitSet));
   }

   /* enable parity options */
   for(wWork = ITMNOPARITY; wWork <= ITMSPACEPARITY; wWork++)
      EnableWindow(GetDlgItem(hDlg, wWork), FALSE);

   if((wSettings & SP_PARITY) == SP_PARITY)
   {
      wWorkSettings = lpCCB->wParityFlags;

      if((wWorkSettings & PARITY_NONE) == PARITY_NONE)
         EnableWindow(GetDlgItem(hDlg, ITMNOPARITY), TRUE);

      if((wWorkSettings & PARITY_ODD) == PARITY_ODD)
         EnableWindow(GetDlgItem(hDlg, ITMODDPARITY), TRUE);

      if((wWorkSettings & PARITY_EVEN) == PARITY_EVEN)
         EnableWindow(GetDlgItem(hDlg, ITMEVENPARITY), TRUE);

      if((wWorkSettings & PARITY_MARK) == PARITY_MARK)
         EnableWindow(GetDlgItem(hDlg, ITMMARKPARITY), TRUE);

      if((wWorkSettings & PARITY_SPACE) == PARITY_SPACE)
         EnableWindow(GetDlgItem(hDlg, ITMSPACEPARITY), TRUE);

      CheckRadioButton(hDlg, ITMNOPARITY, ITMSPACEPARITY, putCCB_PARITY(lpCCB->wParitySet));
   }

   /* enable handshake options */
   for(wWork = ITMXONFLOW; wWork <= ITMNOFLOW; wWork++)
      EnableWindow(GetDlgItem(hDlg, wWork), FALSE);

   if((wSettings & SP_HANDSHAKING) == SP_HANDSHAKING)
   {
      wWorkSettings = lpCCB->wHandshakeFlags;

      if((wWorkSettings & HANDSHAKE_XONXOFF) == HANDSHAKE_XONXOFF)
         EnableWindow(GetDlgItem(hDlg, ITMXONFLOW), TRUE);

      if((wWorkSettings & HANDSHAKE_HARDWARE) == HANDSHAKE_HARDWARE)
         EnableWindow(GetDlgItem(hDlg, ITMHARDFLOW), TRUE);

      if((wWorkSettings & HANDSHAKE_NONE) == HANDSHAKE_NONE)
         EnableWindow(GetDlgItem(hDlg, ITMNOFLOW), TRUE);

      CheckRadioButton(hDlg, ITMXONFLOW, ITMNOFLOW, putCCB_FLOWCTRL(lpCCB->wHandshakeSet));
   }

   /* enable carrier detect button */
   //if((wSettings & SP_RLSD) == SP_RLSD)		  BUGBUG, needs to be changed to SP_CARRIER_DETECT
   //{
   //	EnableWindow(GetDlgItem(hDlg, ITMCARRIER), TRUE);
   //}
   //else
   //{
      EnableWindow(GetDlgItem(hDlg, ITMCARRIER), FALSE);
   //}

   /* enable parity check button */
   if((wSettings & SP_PARITY_CHECK) == SP_PARITY_CHECK)
   {
      EnableWindow(GetDlgItem(hDlg, ITMPARITY), TRUE);
   }
   else
   {
      EnableWindow(GetDlgItem(hDlg, ITMPARITY), FALSE);
   }

   GlobalUnlock(hCCB);                       /* slc nova 031 */
   return(wSettings);
}

/**************************************************************************/

INT GetDynaCommProfileString(LPSTR lpsz1, LPSTR lpsz2, LPSTR lpsz3,
                             LPSTR lpsz4, INT nSize, LPSTR lpsz5)
{
   return GetPrivateProfileString(lpsz1, lpsz2, lpsz3, lpsz4, nSize, lpsz5);
}


/**************************************************************************/
/* getConnectType                                          seh nova 005   */
/**************************************************************************/

WORD getConnectType(HANDLE hConnector, HANDLE hCCB)
{
   LPCONNECTOR_CONTROL_BLOCK lpCCB;
   WORD                      wResult;

   lpCCB = (LPCONNECTOR_CONTROL_BLOCK)GlobalLock(hCCB);
   wResult = (WORD)lpCCB->wType;
   GlobalUnlock(hCCB);

   return(wResult);
}


/**************************************************************************/
/* ccbFromTrmParams                                         slc nova 028  */
/**************************************************************************/

/**************************************************************************/
/* getCCB_BAUD                                             seh nova 005   */
/**************************************************************************/

WORD getCCB_BAUD(WORD wSpeed)
{
   WORD wReturn;

   switch(wSpeed)                            /* from DC to CCB */
   {
   case 75:
      wReturn = BAUD_075;
      break;
   case 110:
      wReturn = BAUD_110;
      break;
   case 300:
      wReturn = BAUD_300;
      break;
   case 600:
      wReturn = BAUD_600;
      break;
   case 1200:
      wReturn = BAUD_120;
      break;
   case 2400:
      wReturn = BAUD_240;
      break;
   case 4800:
      wReturn = BAUD_480;
      break;
   case 9600:
      wReturn = BAUD_960;
      break;
   case 19200:
      wReturn = BAUD_192;
      break;
   default:
      wReturn = BAUD_USER;
      break;
   }

   return(wReturn);
}

/**************************************************************************/
/* getCCB_DATABITS                                         slc nova 028   */
/**************************************************************************/

WORD getCCB_DATABITS(WORD wInput)
{
   WORD wReturn;

   switch(wInput)
   {
   case ITMDATA5:
      wReturn = DATABITS_5;
      break;
   case ITMDATA6:
      wReturn = DATABITS_6;
      break;
   case ITMDATA7:
      wReturn = DATABITS_7;
      break;
   case ITMDATA8:
      wReturn = DATABITS_8;
      break;
   default:
      wReturn = DATABITS_8;
      break;
   }

   return(wReturn);
}

/**************************************************************************/
/* getCCB_PARITY                                           slc nova 028   */
/**************************************************************************/

WORD getCCB_PARITY(WORD wInput)
{
   WORD wReturn;

   switch(wInput)
   {
   case ITMNOPARITY:
      wReturn = PARITY_NONE;
      break;
   case ITMODDPARITY:
      wReturn = PARITY_ODD;
      break;
   case ITMEVENPARITY:
      wReturn = PARITY_EVEN;
      break;
   case ITMMARKPARITY:
      wReturn = PARITY_MARK;
      break;
   case ITMSPACEPARITY:
      wReturn = PARITY_SPACE;
      break;
   default:
      wReturn = PARITY_NONE;
      break;
   }

   return(wReturn);
}

/**************************************************************************/
/* getCCB_STOPBITS                                         slc nova 028   */
/**************************************************************************/

WORD getCCB_STOPBITS(WORD wInput)
{
   WORD wReturn;

   switch(wInput)
   {
   case ITMSTOP1:
      wReturn = STOPBITS_10;
      break;
   case ITMSTOP5:
      wReturn = STOPBITS_15;
      break;
   case ITMSTOP2:
      wReturn = STOPBITS_20;
      break;
   default:
      wReturn = STOPBITS_10;
      break;
   }

   return(wReturn);
}

/**************************************************************************/
/* getCCB_FLOWCTRL                                         slc nova 028   */
/**************************************************************************/

WORD getCCB_FLOWCTRL(WORD wInput)
{
   WORD wReturn;

   switch(wInput)
   {
   case ITMXONFLOW:
      wReturn = HANDSHAKE_XONXOFF;
      break;
   case ITMHARDFLOW:
      wReturn = HANDSHAKE_HARDWARE;
      break;
   case ITMNOFLOW:
      wReturn = HANDSHAKE_NONE;
      break;
   case ITMETXFLOW:
      wReturn = HANDSHAKE_ETXFLOW;
      break;
   default:
      wReturn = HANDSHAKE_XONXOFF;
      break;
   }

   return(wReturn);
}

/**************************************************************************/
/* getCCB_MISCSET                                          slc nova 028   */
/**************************************************************************/

WORD getCCB_MISCSET(WORD wInput1, WORD wInput2)
{
   WORD wReturn = 0;

   if(wInput1)
      wReturn = MISC_CARRIER_DETECT;

   if(wInput2)
      wReturn |= MISC_PARITY_CHECK;

   return(wReturn);
}

/**************************************************************************/

VOID ccbFromTrmParams(LPCONNECTOR_CONTROL_BLOCK lpCCB, recTrmParams *pTrmParams)
{

// -sdj, dec'91 If the trmparams structure  is packed with 1 byte alignment
// -sdj, MIPS compiler breaks while compiling this funciton, I think the line
// -sdj, is: lpCCB->byPadChar = (pTrmParams->commFlags & DCS_CF_NETNAMEPADDING) ? 0x20 : 0x00;
// -sdj, the Fix for this cc bug wont make it into the pdk2, so lets bypass
// -sdj, this function code for the time being
// #ifdef BUGBYPASS
// #else

   lstrcpy(lpCCB->szDLLName, (LPSTR)pTrmParams->szConnectorName); /* slc nova 106 */

   lpCCB->wSpeed         = pTrmParams->speed;             /* seh nova 005 */
   lpCCB->wBaudSet       = getCCB_BAUD(lpCCB->wSpeed);   /* seh nova 005 */
   lpCCB->wDataBitSet    = getCCB_DATABITS(pTrmParams->dataBits);
   lpCCB->wParitySet     = getCCB_PARITY(pTrmParams->parity);
   lpCCB->wStopBitSet    = getCCB_STOPBITS(pTrmParams->stopBits);
   lpCCB->wHandshakeSet  = getCCB_FLOWCTRL(pTrmParams->flowControl);
   lpCCB->wMiscSet       = getCCB_MISCSET(pTrmParams->fCarrier,
                                          pTrmParams->fParity);

   lstrcpy(lpCCB->szPhoneNumber, (LPSTR)pTrmParams->phone); /* slc nova 106 */
   lstrcpy(lpCCB->szClient, (LPSTR)pTrmParams->localName);  /* slc nova 106 */
   lstrcpy(lpCCB->szServer, (LPSTR)pTrmParams->remoteName); /* slc nova 106 */
   lpCCB->byPadChar = (pTrmParams->commFlags & DCS_CF_NETNAMEPADDING) ? 0x20 : 0x00;

   lmovmem((LPSTR)pTrmParams->connectorConfigData, lpCCB->configBuffer, 32); /* slc nova 106 */

// #endif

}

/**************************************************************************/
/* ccbToTrmParams                                           slc nova 028  */
/**************************************************************************/

/**************************************************************************/
/* putCCB_BAUDITM                                          seh nova 005   */
/**************************************************************************/

WORD putCCB_BAUDITM(WORD wID)    /* called by setConnectorSettings() */
{
   WORD wReturn;

   switch(wID)
   {
   case BAUD_075:
      wReturn = ITMBD110;        /* dc does not support yet */
      break;
   case BAUD_110:
      wReturn = ITMBD110;
      break;
   case BAUD_300:
      wReturn = ITMBD300;
      break;
   case BAUD_600:
      wReturn = ITMBD600;
      break;
   case BAUD_120:
      wReturn = ITMBD120;
      break;
   case BAUD_240:
      wReturn = ITMBD240;
      break;
   case BAUD_480:
      wReturn = ITMBD480;
      break;
   case BAUD_960:
      wReturn = ITMBD960;
      break;
   case BAUD_192:
      wReturn = ITMBD192;
      break;
   default:
      wReturn = ITMBD120;
      break;
   }

   return(wReturn);
}

/**************************************************************************/
/* putCCB_BAUD                                             seh nova 005   */
/**************************************************************************/

WORD putCCB_BAUD(WORD wID)
{
   WORD wReturn;

   switch(wID)
   {
   case BAUD_075:
      wReturn = 75;
      break;
   case BAUD_110:
      wReturn = 110;
      break;
   case BAUD_300:
      wReturn = 300;
      break;
   case BAUD_600:
      wReturn = 600;
      break;
   case BAUD_120:
      wReturn = 1200;
      break;
   case BAUD_240:
      wReturn = 2400;
      break;
   case BAUD_480:
      wReturn = 4800;
      break;
   case BAUD_960:
      wReturn = 9600;
      break;
   case BAUD_192:
      wReturn = 19200;
      break;
   default:
      wReturn = 1200;
      break;
   }

   return(wReturn);
}

/**************************************************************************/
/* putCCB_DATABITS                                         slc nova 028   */
/**************************************************************************/

WORD putCCB_DATABITS(WORD wInput)
{
   WORD wReturn;

   switch(wInput)
   {
   case DATABITS_5:
      wReturn = ITMDATA5;
      break;
   case DATABITS_6:
      wReturn = ITMDATA6;
      break;
   case DATABITS_7:
      wReturn = ITMDATA7;
      break;
   case DATABITS_8:
      wReturn = ITMDATA8;
      break;
   default:
      wReturn = ITMDATA8;
      break;
   }

   return(wReturn);
}

/**************************************************************************/
/* putCCB_PARITY                                           slc nova 028   */
/**************************************************************************/

WORD putCCB_PARITY(WORD wInput)
{
   WORD wReturn;

   switch(wInput)
   {
   case PARITY_NONE:
      wReturn = ITMNOPARITY;
      break;
   case PARITY_ODD:
      wReturn = ITMODDPARITY;
      break;
   case PARITY_EVEN:
      wReturn = ITMEVENPARITY;
      break;
   case PARITY_MARK:
      wReturn = ITMMARKPARITY;
      break;
   case PARITY_SPACE:
      wReturn = ITMSPACEPARITY;
      break;
   default:
      wReturn = ITMNOPARITY;
      break;
   }

   return(wReturn);
}

/**************************************************************************/
/* putCCB_STOPBITS                                         slc nova 028   */
/**************************************************************************/

WORD putCCB_STOPBITS(WORD wInput)
{
   WORD wReturn;

   switch(wInput)
   {
   case STOPBITS_10:
      wReturn = ITMSTOP1;
      break;
   case STOPBITS_15:
      wReturn = ITMSTOP5;
      break;
   case STOPBITS_20:
      wReturn = ITMSTOP2;
      break;
   default:
      wReturn = ITMSTOP1;
      break;
   }

   return(wReturn);
}

/**************************************************************************/
/* putCCB_FLOWCTRL                                         slc nova 028   */
/**************************************************************************/

WORD putCCB_FLOWCTRL(WORD wInput)
{
   WORD wReturn;

   switch(wInput)
   {
   case HANDSHAKE_XONXOFF:
      wReturn = ITMXONFLOW;
      break;
   case HANDSHAKE_HARDWARE:
      wReturn = ITMHARDFLOW;
      break;
   case HANDSHAKE_NONE:
      wReturn = ITMNOFLOW;
      break;
   case HANDSHAKE_ETXFLOW:
      wReturn = ITMETXFLOW;
      break;
   default:
      wReturn = ITMXONFLOW;
      break;
   }

   return(wReturn);
}

/**************************************************************************/
/* putCCB_MISCSET                                          slc nova 028   */
/**************************************************************************/

WORD putCCB_MISCSET(WORD wInput1, WORD wInput2)
{
   WORD wReturn = 0;

   if(wInput1)
      wReturn = MISC_CARRIER_DETECT;

   if(wInput2)
      wReturn |= MISC_PARITY_CHECK;

   return(wReturn);
}

/**************************************************************************/

VOID ccbToTrmParams(recTrmParams *pTrmParams, LPCONNECTOR_CONTROL_BLOCK lpCCB)
{
// -sdj, dec'91 If the trmparams structure  is packed with 1 byte alignment
// -sdj, MIPS compiler breaks while compiling this funciton,
// -sdj, the Fix for this cc bug wont make it into the pdk2, so lets bypass
// -sdj, this function code for the time being
// #ifdef BUGBYPASS
// #else

   lstrcpy((LPSTR)pTrmParams->szConnectorName, lpCCB->szDLLName); /* slc nova 106 */

   pTrmParams->speed       = putCCB_BAUD(lpCCB->wBaudSet);
   pTrmParams->dataBits    = putCCB_DATABITS(lpCCB->wDataBitSet);
   pTrmParams->parity      = putCCB_PARITY(lpCCB->wParitySet);
   pTrmParams->stopBits    = putCCB_STOPBITS(lpCCB->wStopBitSet);
   pTrmParams->flowControl = putCCB_FLOWCTRL(lpCCB->wHandshakeSet);
   pTrmParams->fCarrier    = (lpCCB->wMiscSet & MISC_CARRIER_DETECT) ? 1 : 0;
   pTrmParams->fParity     = (lpCCB->wMiscSet & MISC_PARITY_CHECK) ? 1 : 0;

   lstrcpy((LPSTR)pTrmParams->phone, lpCCB->szPhoneNumber); /* slc nova 106 */
   lstrcpy((LPSTR)pTrmParams->localName, lpCCB->szClient);  /* slc nova 106 */
   lstrcpy((LPSTR)pTrmParams->remoteName, lpCCB->szServer); /* slc nova 106 */
   if(lpCCB->byPadChar == 0x20)
   {
      pTrmParams->commFlags |= DCS_CF_NETNAMEPADDING;
      NETNAMEPADDING = ' ';
   }
   else
   {
      pTrmParams->commFlags &= ~DCS_CF_NETNAMEPADDING;
      NETNAMEPADDING = '\0';
   }

   lmovmem(lpCCB->configBuffer, (LPSTR)pTrmParams->connectorConfigData, 32); /* slc nova 106 */

// #endif
}

/* the end */
