// Credit.c
#define STRICT

#include <windows.h>
#include "Init_End.h"
#include "resource.h"

#include "credits.h"


/***************************************************/
/* Startup procedure for modal dialog box */
/***************************************************/

INT_PTR CreditsDlgFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
    INT_PTR  ReturnValue;
    TCHAR    str[256]=TEXT("");
    TCHAR    title[256]=TEXT("");

    ReturnValue = DialogBox(hInst,  MAKEINTRESOURCE(IDD_CREDITS), 
                            hWnd, CreditsDlgProc);

    if (ReturnValue==-1)
    {
        LoadString(hInst, IDS_CANNOTCREATEDLG, &str[0], 256);
        LoadString(hInst, IDS_CREDITSBOX, &title[0], 256);
        MessageBox(hWnd, str, title, MB_OK|MB_ICONHAND);
    }
    return ReturnValue;
}


/*****************************************************************************/
/* Modal dialog box procedure */
/*****************************************************************************/

INT_PTR CALLBACK CreditsDlgProc(HWND hDlg, UINT message, 
                                WPARAM wParam, LPARAM lParam)
{	
    switch(message)
    {
        case WM_INITDIALOG:
            return CreditsDlgDefault(hDlg,message,wParam,lParam);
            break;


        case WM_COMMAND:
            switch(wParam)
            {
                case IDOK:
                    if (!CreditsDlgDefault(hDlg,message,wParam,lParam))
                    { 	
                        EndDialog(hDlg,IDOK);
				    }
					  
                    break;

				case IDCANCEL:
						EndDialog(hDlg,IDCANCEL);
					 break; 


				default:
					  return CreditsDlgDefault(hDlg,message,wParam,lParam);
					  break;
					 }
		  default:
		  return CreditsDlgDefault(hDlg,message,wParam,lParam);
		  break;
		}
	 return TRUE;/* Did process the message */
}


/***************************************************************/

BOOL CreditsDlgDefault(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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

