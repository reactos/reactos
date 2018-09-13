// about.c
#define STRICT

#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>

#include "Init_End.h"
#include "resource.h"

#include "scan.h"
#include "credits.h"
#include "dgupgrad.h"
#include "about.h"
#include "door.h"
#include "kbmain.h"

extern DWORD GetDesktop();
extern BOOL g_fShowWarningAgain;

extern KBPREFINFO  *kbPref;

/**************************************************************************/

/* Startup procedure for modal dialog box */

INT_PTR AboutDlgFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ReturnValue;
	TCHAR   str[256];
	TCHAR   title[256];

    ReturnValue = DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUT), NULL,
                            AboutDlgProc);

	 if (ReturnValue==-1)
	 {	
        LoadString(hInst, IDS_CANNOTCREATEDLG, &str[0], 256);
        LoadString(hInst, IDS_ABOUTBOX, &title[0], 256);
		MessageBox(hWnd, str, title, MB_OK|MB_ICONHAND);
	 }
	 return ReturnValue;
}


/*****************************************************************************/

/* Modal dialog box procedure */

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, 
                              WPARAM wParam, LPARAM lParam) 
{	
    switch(message)
    {
        case WM_INITDIALOG:
        {
			RelocateDialog(hDlg);

            KillScanTimer(TRUE);  //kill scanning

        }
        return AboutDlgDefault(hDlg,message,wParam,lParam);
        break;

		case WM_PAINT:
			{
				HDC hDC, hDC1;
				HBITMAP hBitMap, hOld;
				RECT rc;
				POINT pt;
				PAINTSTRUCT ps;
				// hDC = (HDC) wParam;
				hDC = BeginPaint(hDlg, &ps);

				GetWindowRect(GetDlgItem(hDlg, IDC_LOGO), &rc);
				pt.x = rc.left;
				pt.y = rc.top;
				ScreenToClient(hDlg, &pt);

				hDC1 = CreateCompatibleDC(hDC);

				hBitMap = (HBITMAP) LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP4), IMAGE_BITMAP, 0, 0, 
					    LR_LOADMAP3DCOLORS);

				hOld = SelectObject(hDC1, hBitMap);

				BitBlt(hDC, pt.x, pt.y, 112, 32, hDC1, 0,0, SRCCOPY);

				DeleteObject(hBitMap);
				SelectObject(hDC1, hOld);
				DeleteDC(hDC1);

				EndPaint(hDlg, &ps);
				
			}
			break;
		
		case WM_NOTIFY:
		{
			// Web address linked: Anil
			INT idCtl		= (INT)wParam;
			LPNMHDR pnmh	= (LPNMHDR)lParam;
			switch ( pnmh->code)
			{
				case NM_RETURN:
				case NM_CLICK:
				if ( (idCtl == IDC_ENABLEWEB2) && (GetDesktop() == DESKTOP_DEFAULT))
				{
					TCHAR webAddr[256];
					LoadString(hInst, IDS_ENABLEWEB, webAddr, 256);
					ShellExecute(hDlg, TEXT("open"), TEXT("iexplore.exe"), webAddr, NULL, SW_SHOW); 
				}
				break;
			}
		}
		break;

        case WM_COMMAND:
            switch(wParam)
            {
                case IDOK:
                    if (!AboutDlgDefault(hDlg,message,wParam,lParam))
                    {
                         EndDialog(hDlg,IDOK);
                    }
					  
                break;

				case IDCANCEL:
						EndDialog(hDlg,IDCANCEL);
					 break; 

				// v-mjgran: The upgrade button has been removed
                case BUT_UPGRADE:
                    EndDialog(hDlg,IDCANCEL);   
                    UpgradeDlgFunc(hDlg, message, wParam, lParam);
                break;


				default:
					  return AboutDlgDefault(hDlg,message,wParam,lParam);
					  break;
					 }
		  default:
		  return AboutDlgDefault(hDlg,message,wParam,lParam);
		  break;
		}
	 return TRUE;/* Did process the message */
}


/***************************************************************/

BOOL AboutDlgDefault(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{	
		
	 switch(message)
		  {
		  case WM_INITDIALOG:

		  return TRUE;       /* TRUE means Windows will process WM_INITDIALOG */
		  break;

		  case WM_COMMAND:
				switch(wParam)
					 {

				default:
					  return FALSE; /* Didn't process the message */
					  break;
					 }
				break;

		  case WM_DRAWITEM:      /* Draw graphic button(s)     */
				{
				LPDRAWITEMSTRUCT lpDrawItem;

				lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
				if (lpDrawItem->CtlType!=ODT_BUTTON)
					 return FALSE;
				if (lpDrawItem->itemAction!=ODA_DRAWENTIRE)
					 return FALSE;
				switch(lpDrawItem->CtlID)
					 {

				default:
					  return FALSE; /* Didn't process the message */
					  break;
					 }
				}
				break;

		  default:
				return FALSE; /* Didn't process the message */
				break;
		  }
	 return TRUE;
}


/*****************************************************************************/


/**************************************************************************/
// Initial Warning Message Management
/**************************************************************************/

/* Startup procedure for modal dialog box */

INT_PTR WarningMsgDlgFunc(HWND hWnd)
{
	INT_PTR ReturnValue;
	TCHAR   str[256];
	TCHAR   title[256];

    ReturnValue = DialogBox(hInst, MAKEINTRESOURCE(IDD_WARNING_MSG), NULL,
                            WarningMsgDlgProc);

	 if (ReturnValue==-1)
	 {	
        LoadString(hInst, IDS_CANNOTCREATEDLG, &str[0], 256);
        LoadString(hInst, IDS_WARNING_MSG, &title[0], 256);
		MessageBox(hWnd, str, title, MB_OK|MB_ICONHAND);
	 }

//	 hWarningIcon = LoadIcon(NULL, IDI_WARNING);
	 return ReturnValue;
}


/*****************************************************************************/

/* Modal dialog box procedure */

INT_PTR CALLBACK WarningMsgDlgProc(HWND hDlg, UINT message, 
                              WPARAM wParam, LPARAM lParam) 
{	
    switch(message)
    {
        case WM_INITDIALOG:
        {
			RelocateDialog(hDlg);

            KillScanTimer(TRUE);  //kill scanning

        }
        return WarningMsgDlgDefault(hDlg,message,wParam,lParam);
        break;

		case WM_NOTIFY:
		{
			INT idCtl		= (INT)wParam;
			LPNMHDR pnmh	= (LPNMHDR)lParam;
			switch ( pnmh->code)
			{
				case NM_RETURN:
				case NM_CLICK:
				if ( (idCtl == IDC_ENABLEWEB) && (GetDesktop() == DESKTOP_DEFAULT))
				{
					TCHAR webAddr[256];
					LoadString(hInst, IDS_ENABLEWEB, webAddr, 256);
					ShellExecute(hDlg, TEXT("open"), TEXT("iexplore.exe"), webAddr, NULL, SW_SHOW); 
				}
				break;
			}
		}
		break;

		case WM_CLOSE:
			EndDialog(hDlg,IDOK);
			break;

        case WM_COMMAND:
            switch(wParam)
            {
                case IDOK:
                    if (!WarningMsgDlgDefault(hDlg,message,wParam,lParam))
                    {
                         EndDialog(hDlg,IDOK);
                    }
					  
                break;
				default:
					  return WarningMsgDlgDefault(hDlg,message,wParam,lParam);
					  break;
					 }
		  default:
		  return WarningMsgDlgDefault(hDlg,message,wParam,lParam);
		  break;
		}
	 return TRUE;/* Did process the message */
}


/***************************************************************/
BOOL WarningMsgDlgDefault(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{	
		
	 switch(message)
		  {
		  case WM_INITDIALOG:

		  return TRUE;       /* TRUE means Windows will process WM_INITDIALOG */
		  break;

		  case WM_COMMAND:
				switch(wParam)
				{
					case IDOK:
					{
						LRESULT lRes = SendMessage(GetDlgItem(hDlg, IDC_SHOW_AGAIN), BM_GETCHECK, 0, 0);

						if (lRes == BST_CHECKED)
						{
							g_fShowWarningAgain = FALSE;
						}
						else
						{
							g_fShowWarningAgain = TRUE;
						}

						kbPref->fShowWarningAgain = g_fShowWarningAgain;

						return FALSE;
					}
					break;
				
					case IDCANCEL:
						EndDialog(hDlg,IDCANCEL);
					 break; 

					default:
					  return FALSE; /* Didn't process the message */
					  break;
				}
				break;

		  default:
				return FALSE; /* Didn't process the message */
				break;
		  }
	 return TRUE;
}


/*****************************************************************************/


