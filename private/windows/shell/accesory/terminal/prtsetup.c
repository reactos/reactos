/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"


/*---------------------------------------------------------------------------*/

#define PRTLISTSIZE              2048


/*---------------------------------------------------------------------------*/
/* Print Setup Utilities                                               [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR PrtGetList(HANDLE   *hList, LPSTR    *lpList)
{
   BYTE  szDevices[MINRESSTR];

   if((*hList = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (DWORD) PRTLISTSIZE)) == NULL)
      return(FALSE);

   if((*lpList = GlobalLock(*hList)) == NULL)
   {
      *hList = GlobalFree(*hList);
      return(FALSE);
   }

   LoadString(hInst, STR_INI_DEVICES, (LPSTR) szDevices, sizeof(szDevices));
   return(GetProfileString((LPSTR) szDevices, NULL, NULL_STR, *lpList, PRTLISTSIZE));  /* mbbx 2.01.97 ... */
/* return(GetProfileString((LPSTR) szDevices, NULL, NULL, *lpList, PRTLISTSIZE)); */
}


/*---------------------------------------------------------------------------*/

VOID NEAR PrtFreeList(HANDLE   hList)
{
   GlobalUnlock(hList);
   GlobalFree(hList);
}


/*---------------------------------------------------------------------------*/

WORD NEAR PrtGetDevice(BYTE  *pDevice, BYTE  *pDriver, BYTE  *pPort)
{
   WORD  PrtGetDevice;
   BYTE  szDevice[MINRESSTR], work[STR255];
   BYTE  *pch;

   LoadString(hInst, STR_INI_DEVICES, (LPSTR) szDevice, sizeof(szDevice));
   GetProfileString((LPSTR) szDevice, (LPSTR) pDevice, NULL_STR, (LPSTR) work, sizeof(work));

   *pDriver = 0;
   *pPort = 0;
   sscanf(work, "%[^,]%c%s", pDriver, szDevice, pPort);

   for(pch = pDriver + strlen(pDriver); (pch > pDriver) && (*(pch-1) == 0x20); pch -= 1);
   *pch = 0;

   if(PrtGetDevice = (*pPort != 0))
   {
      for(pch = pPort; *pch != 0; pch += 1)
         if(*pch == ',')
         {
            *pch = 0;
            PrtGetDevice += 1;
         }

      *(pch+1) = 0;
   }

   return(PrtGetDevice);
}


/*---------------------------------------------------------------------------*/

BOOL NEAR PrtTestDevice(BYTE  *pDevice, BYTE  *pDriver, BYTE  *pPort, BYTE  *pResult)
{
   BYTE  szWindows[MINRESSTR], szDevice[MINRESSTR];
   BYTE  work[STR255];

   sprintf(pResult, "%s,%s,%s", pDevice, pDriver, pPort);

   LoadString(hInst, STR_INI_WINDOWS, (LPSTR) szWindows, sizeof(szWindows));
   LoadString(hInst, STR_INI_DEVICE, (LPSTR) szDevice, sizeof(szDevice));
   GetProfileString((LPSTR) szWindows, (LPSTR) szDevice, NULL_STR, (LPSTR) work, sizeof(work));
   /* changed from strcmpi -sdj */
   return(lstrcmpi(pResult, work) == 0);
}


/*---------------------------------------------------------------------------*/
/* PrtInitList() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

VOID NEAR PrtInitList(HWND  hDlg)
{
   HANDLE   hList;
   LPSTR    lpList;
   INT      nSelect, nCount, ndx, ndx2;
   BYTE     work[STR255], szDriver[80], szPort[128], szDevice[128];

   SetCursor(LoadCursor(NULL, IDC_WAIT));

   if(PrtGetList(&hList, &lpList))
   {
      nSelect = -1;
      nCount = 0;

      for(ndx = 0; (lpList[ndx] != 0) && (ndx < PRTLISTSIZE); 
          ndx += (lstrlen(lpList + ndx) + 1))
      {
         lstrcpy((LPSTR) work, lpList + ndx);
         if(PrtGetDevice(work, szDriver, szPort))
         {
            for(ndx2 = 0; szPort[ndx2] != 0; 
                ndx2 += (strlen(szPort + ndx2) + 1))
            {
               if(PrtTestDevice(work, szDriver, szPort + ndx2, szDevice))
                  nSelect = nCount;

               strcpy(szDevice, work);
               LoadString(hInst, STR_INI_ON, (LPSTR) szDevice + strlen(szDevice), MINRESSTR);
               strcpy(szDevice + strlen(szDevice), szPort + ndx2);
               SendDlgItemMessage(hDlg, IDPRINTNAME, LB_INSERTSTRING, -1, (LONG) ((LPSTR) szDevice));

               nCount += 1;
            }
         }
      }

      if(nSelect != -1)
         SendDlgItemMessage(hDlg, IDPRINTNAME, LB_SETCURSEL, nSelect, 0L);
      else
      {
         EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
         EnableWindow(GetDlgItem(hDlg, IDPRTSETUP), FALSE);
      }

      PrtFreeList(hList);
   }

   SetCursor(LoadCursor(NULL, IDC_ARROW));
}


/*---------------------------------------------------------------------------*/
/* PrtDoCommand() -                                                    [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR PrtDoCommand(HWND  hDlg, BOOL  bSetup)
{
   HANDLE   hList, hDriver;
   LPSTR    lpList;
   INT      nSelect, nPorts, ndx;
   BYTE     work[STR255], szDriver[80], szPort[128], szDevice[128];
   FARPROC  lpDriver;

   if((nSelect = SendDlgItemMessage(hDlg, IDPRINTNAME, LB_GETCURSEL, 0, 0L)) == -1)
      return(TRUE);

   SetCursor(LoadCursor(NULL, IDC_WAIT));

   if(PrtGetList(&hList, &lpList))
   {
      for(ndx = 0; nSelect >= 0; ndx += (lstrlen(lpList + ndx) + 1))
      {
         lstrcpy((LPSTR) work, lpList + ndx);
         if((nPorts = PrtGetDevice(work, szDriver, szPort)) > nSelect)
         {
            for(ndx = 0; nSelect > 0; ndx += (strlen(szPort + ndx) + 1))
            {
               if(szPort[ndx] == 0)
                  break;

               nSelect -= 1;
            }
            break;
         }

         nSelect -= nPorts;
      }

      PrtFreeList(hList);

      if(nSelect == 0)
      {
         if(bSetup)
         {
            strcpy(szDriver + strlen(szDriver), DRIVER_FILE_TYPE+2);
/* jtf 3.14            if((hDriver = fileLoadLibrary(szDriver)) != NULL) */

         /*if((hDriver = MLoadLibrary((LPSTR) szDriver)) >= 32)
            LoadLibrary >= 32  changed to LoadLibrary != NULL -sdj*/
#ifdef ORGCODE
         if((hDriver = LoadLibrary((LPSTR) szDriver)) >= 32) /* jtf 3.14 */
#else
         if((hDriver = LoadLibrary((LPSTR) szDriver)) != NULL) /* jtf 3.14 */
#endif
            {
               LoadString(hInst, STR_INI_DEVICEMODE, (LPSTR) szDevice, sizeof(szDevice));
               if(lpDriver = GetProcAddress(hDriver, (LPSTR) szDevice))
                  (*lpDriver) (hDlg, hDriver, (LPSTR) work, (LPSTR) szPort);

               FreeLibrary(hDriver);
            }
         }
         else if(!PrtTestDevice(work, szDriver, szPort + ndx, szDevice))
         {
            LoadString(hInst, STR_INI_WINDOWS, (LPSTR) work, sizeof(work));
            LoadString(hInst, STR_INI_DEVICE, (LPSTR) szDriver, sizeof(szDriver));
            WriteProfileString((LPSTR) work, (LPSTR) szDriver, (LPSTR) szDevice);
         }
      }
   }

   SetCursor(LoadCursor(NULL, IDC_ARROW));

   return(bSetup);
}


/*---------------------------------------------------------------------------*/
/* dbPrtSetup() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL  APIENTRY dbPrtSetup(HWND  hDlg, WORD  message, WPARAM wParam, LONG  lParam)
{
   BOOL  result;

   switch(message)
   {
   case WM_INITDIALOG:
      initDlgPos(hDlg);

      PrtInitList(hDlg);                     /* mbbx 2.01.149 ... */
      return(TRUE);

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
      case IDCANCEL:
      case IDPRTSETUP:
         result = (GET_WM_COMMAND_ID(wParam, lParam) != IDCANCEL);
         break;

      case IDPRINTNAME:
         switch(GET_WM_COMMAND_CMD(wParam, lParam))
         {
         case LBN_SELCHANGE:
            result = (SendDlgItemMessage(hDlg, IDPRINTNAME, LB_GETCURSEL, 0, 0L) != -1);
            EnableWindow(GetDlgItem(hDlg, IDOK), result);
            EnableWindow(GetDlgItem(hDlg, IDPRTSETUP), result);
            return(TRUE);

         case LBN_DBLCLK:
            result = TRUE;
            break;

         default:
            return(TRUE);
         }
         break;

      default:
         return(FALSE);
      }
      break;

   default:
      return(FALSE);
   }

   if(result)                                /* mbbx 2.01.149 ... */
   {
      if(PrtDoCommand(hDlg, GET_WM_COMMAND_ID(wParam, lParam) == IDPRTSETUP))
         return(TRUE);
   }

   EndDialog(hDlg, 0);
   return(TRUE);
}
