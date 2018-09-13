#define STRICT

#include <windows.h>
#include <shellapi.h>

#include "Init_End.h"
#include "resource.h"

#include "thanks.h"


HWND hPreWindow=NULL;


/***************************************************/

/***************************************************/
/* Startup procedure for modal dialog box          */
/***************************************************/

INT_PTR ThanksDlgFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ReturnValue;
	TCHAR   str[256];
	TCHAR   title[256];
	
    hPreWindow = GetActiveWindow();


    ReturnValue = DialogBox(hInst, MAKEINTRESOURCE(IDD_THANK_YOU), hWnd, 
                            ThanksDlgProc);

    if (ReturnValue==-1)
	{	
        LoadString(hInst, IDS_CANNOTCREATEDLG, &str[0], 256);
        LoadString(hInst, IDS_THANKYOU, &title[0], 256);
        MessageBox(hWnd, str, title, MB_OK|MB_ICONHAND);
    }
	 return ReturnValue;
}


/******************************************************************************/
/* Modal dialog box procedure */
/******************************************************************************/

INT_PTR CALLBACK ThanksDlgProc(HWND hDlg, unsigned message, 
                               WPARAM wParam, LPARAM lParam)
{

    switch(message)
    {
        case WM_INITDIALOG:
            return ThanksDlgDefault(hDlg,message,wParam,lParam);
            break;


		  case WM_COMMAND:
				switch(wParam)
					 {
				case IDOK:
					  if (!ThanksDlgDefault(hDlg,message,wParam,lParam))
					  { 	EndDialog(hDlg,IDOK);

							SetActiveWindow(hPreWindow);
					  }
					  
					  break;

				default:
					  return ThanksDlgDefault(hDlg,message,wParam,lParam);
					  break;
					 }
		  default:
		  return ThanksDlgDefault(hDlg,message,wParam,lParam);
		  break;
		}
	 return TRUE;/* Did process the message */
}

/***************************************************************/

BOOL ThanksDlgDefault(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{	 
    TCHAR str[256]=TEXT("");
	

    switch(message)
        {
        case WM_INITDIALOG:
		    return TRUE;    /* TRUE means Windows will process WM_INITDIALOG */
            break;

        case WM_COMMAND:
            switch(wParam)
            {
				case TXT_WEBSITE:	
					
					EndDialog(hDlg,IDOK);
					
					//load our web URL
					LoadString(hInst, IDS_WEDSITE, &str[0], 256);	
					
					ShellExecute(NULL, TEXT("open"), str, NULL, NULL, 
                                 SW_SHOWNORMAL);
				break;

				case BUT_MAIL:
					
					EndDialog(hDlg,IDOK);

					LoadString(hInst, IDS_MAIL, &str[0], 256);	
					ShellExecute(NULL, TEXT("open"), str, NULL, NULL, 
                                 SW_SHOWNORMAL);
					
				break;

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

