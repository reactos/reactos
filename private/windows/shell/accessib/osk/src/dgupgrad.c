// DgUpgrade.c
#define STRICT

#include <windows.h>
#include <shellapi.h>

#include "Init_End.h"
#include "resource.h"

#include "dgupgrad.h"
#include "kbmain.h"


extern DWORD GetDesktop();

/***************************************************/
INT_PTR UpgradeDlgFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ReturnValue;
	TCHAR str[256];
	TCHAR title[256];

	 ReturnValue = DialogBox(hInst,  MAKEINTRESOURCE(DLG_UPGRADE), NULL, 
                             (DLGPROC)UpgradeDlgProc);

	 if (ReturnValue==-1)
	 {	
        LoadString(hInst, IDS_CANNOTCREATEDLG, &str[0], 256);
        LoadString(hInst, IDS_TITLE1, &title[0], 256);
		MessageBox(hWnd, str, title, MB_OK|MB_ICONHAND);
	 }
	 return ReturnValue;
}

/*****************************************************************************/
//
/*****************************************************************************/
INT_PTR CALLBACK 
UpgradeDlgProc(HWND hDlg, unsigned message, WPARAM wParam, LPARAM lParam) 
{	
	TCHAR str[256]=TEXT("");

	 switch(message)
		  {
		  case WM_INITDIALOG:

			RelocateDialog(hDlg);


		  return UpgradeDlgDefault(hDlg,message,wParam,lParam);
		  break;

		case WM_PAINT:
			{
				HDC hDC, hDC1;
				HBITMAP hBitMap, hOld;
				RECT rc;
				PAINTSTRUCT ps;
				// hDC = (HDC) wParam;
				hDC = BeginPaint(hDlg, &ps);

				GetWindowRect(GetDlgItem(hDlg, IDC_LOGO1), &rc);
				ScreenToClient(hDlg, (LPPOINT)&rc);

				hDC1 = CreateCompatibleDC(hDC);

				hBitMap = (HBITMAP) LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP4), IMAGE_BITMAP, 0, 0, 
					    LR_LOADMAP3DCOLORS);

				hOld = SelectObject(hDC1, hBitMap);

				BitBlt(hDC, rc.left, rc.top, 112, 32, hDC1, 0,0, SRCCOPY);
				
				DeleteObject(hBitMap);
				SelectObject(hDC1, hOld);
				DeleteDC(hDC1);

				EndPaint(hDlg, &ps);
				
			}
			break;

		  case WM_COMMAND:
				switch(wParam)
					 {
				case IDOK:
					EndDialog(hDlg,IDOK);
				break;

				case IDCANCEL:
					EndDialog(hDlg,IDCANCEL);
				break; 

				case BUT_WEBSITE:	
					//load our web URL
					if ( GetDesktop() == DESKTOP_DEFAULT )
					{

						LoadString(hInst, IDS_WEDSITE, &str[0], 256);	
					
						ShellExecute(NULL, TEXT("open"), str, NULL, NULL, 
                                 SW_SHOWNORMAL);

						EndDialog(hDlg,IDOK);
					}
				break;

				case BUT_MAIL:

					LoadString(hInst, IDS_MAIL, &str[0], 256);	

					ShellExecute(NULL, TEXT("open"), str, NULL, NULL, 
                                 SW_SHOWNORMAL);

					EndDialog(hDlg,IDOK);
				break;


				default:
					  return UpgradeDlgDefault(hDlg,message,wParam,lParam);
					  break;
					 }
		  default:
		  return UpgradeDlgDefault(hDlg,message,wParam,lParam);
		  break;
		}
	 return TRUE;/* Did process the message */
}

/***************************************************************/
//
/***************************************************************/
BOOL UpgradeDlgDefault(HWND hDlg, unsigned message, WPARAM wParam,LPARAM lParam)
{	
		
	 switch(message)
		  {
		  case WM_INITDIALOG:

		  return TRUE;     /* TRUE means Windows will process WM_INITDIALOG */
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

