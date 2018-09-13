// Copyright (c) 1997-1999 Microsoft Corporation
// KBMAIN.C 
// Additions, Bug Fixes 1999 Anil Kumar
//  

#define STRICT


#include <windows.h>
#include <malloc.h>

#include "kbmain.h"
#include "Init_End.h"
#include "door.h"
#include "resource.h"


//*****************************************************************************
//    Functions prototype
//*****************************************************************************
#include "sdgutil.h"
#include "dgadvsca.h"
#include "Init_End.h"

#define MAX_KEY_TEXT		8

extern DWORD GetDesktop();

DWORD	g_rgHelpIds[] ={
		IDOK,	        70525,
		IDCANCEL,	    70530,
		CHK_KEY,        70545,
        COMBO_KB_KEY,   70545,
        CHK_PORT,       70540
    };

/*****************************************************************************/

INT_PTR AdvScanDlgFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ReturnValue;

    ReturnValue = DialogBox(hInst,  MAKEINTRESOURCE(IDD_ADVANCE_SCANNING), 
                            hWnd, (DLGPROC)AdvScanDlgProc);

    if (ReturnValue==-1)
	{	
        SendErrorMessage(IDS_CANNOTCREATEDLG);
	}
	return ReturnValue;
}

/*****************************************************************************/
INT_PTR CALLBACK AdvScanDlgProc(HWND hDlg, UINT message, 
                                WPARAM wParam, LPARAM lParam) 
{	
	HWND	hComboBox;
	int		nSel;
	static  BOOL bKBKey;
	static  UINT uKBKey;
	static  BOOL bPort;
    // F1 key is always for help and F10 for menu , So donot 
    // use these for scanning. a-anilk
/*	TCHAR	sKBKey[11][6]={ TEXT("Space"), TEXT("Enter"), 
                            TEXT("F2"),  TEXT("F3"), TEXT("F4"),
                            TEXT("F5"),    TEXT("F6"),  TEXT("F7"), TEXT("F8"),
                            TEXT("F9"),  TEXT("F12") };
*/
	LPTSTR sKBKey[11];
	UINT    ary_KBKey[11]={VK_SPACE, VK_RETURN, 
                           VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, 
		                   VK_F7, VK_F8, VK_F9, VK_F12};
	int i;
	
	int nCopiedChars, nTextSpace, nOldCopied;

	BOOL bRetValue = TRUE;


	//v-mjgran: Init sKBKey;
	for (i=0; i<11; i++)
	{
		nTextSpace = MAX_KEY_TEXT;

		sKBKey[i] = (LPTSTR) malloc (nTextSpace*sizeof(TCHAR));

		nCopiedChars = LoadString(hInst, IDS_SPACE_KEY+i, sKBKey[i], nTextSpace);
		nOldCopied = 0;
		while (nCopiedChars == (nTextSpace-1) && nOldCopied != nCopiedChars)
		{
			// To allow more space in diferent languages
			free(sKBKey[i]);
			nTextSpace = nTextSpace << 1;		//duplicate the available space
			sKBKey[i] = (LPTSTR) malloc (nTextSpace*sizeof(TCHAR));
			nOldCopied = nCopiedChars;
			nCopiedChars = LoadString(hInst, IDS_SPACE_KEY+i, sKBKey[i], nTextSpace);
		}
	}


	switch(message)
		{
		case WM_INITDIALOG:

			CheckDlgButton(hDlg, CHK_PORT,
							((bPort = kbPref->bPort) ? BST_CHECKED : \
													   BST_UNCHECKED));
			
			CheckDlgButton(hDlg, CHK_KEY,
                           ((bKBKey = kbPref->bKBKey) ? BST_CHECKED : \
                                                        BST_UNCHECKED));

			EnableWindow(GetDlgItem(hDlg, COMBO_KB_KEY), bKBKey ? TRUE : FALSE);
			
			//Set the Switch key from the setting
			uKBKey = kbPref->uKBKey;

			//Combo box
			hComboBox = GetDlgItem(hDlg, COMBO_KB_KEY);

            // Number of choices = 11 :a-anilk
			for(i=0; i < 11; i++)
            {
				SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)sKBKey[i]);
            }
			
			for(i=0; i < 11; i++)
            {
				if(uKBKey == ary_KBKey[i])
				{	
                    //set which choice be in the combo box at starting
                    SendMessage(hComboBox, CB_SETCURSEL, i, 0L);     
					break;
				}
            }

			//return TRUE;
			bRetValue = TRUE;
		break;

		case WM_COMMAND:
				switch(LOWORD(wParam))
					{
				case IDOK:
					
					//Switch Key
					if((bKBKey != kbPref->bKBKey) || (uKBKey != kbPref->uKBKey))
					{	
						//Save it to the setting record
						kbPref->bKBKey = bKBKey;
						kbPref->uKBKey = uKBKey;
						
						if(bKBKey)   //Config the scan key
							ConfigSwitchKey(kbPref->uKBKey, TRUE);
						else         //disable the scan key
							ConfigSwitchKey(0, FALSE);
					}

					//Switch Port
					if(bPort != kbPref->bPort)
					{
						kbPref->bPort = bPort;
						
						//Config the port (On or OFF)
						ConfigPort(bPort);
					}


					EndDialog(hDlg,IDOK);
				break;

				case IDCANCEL:
					EndDialog(hDlg,IDCANCEL);
				break;

				case CHK_PORT:
					bPort = !bPort;
				break;

				case CHK_KEY:
					bKBKey = !bKBKey;

					EnableWindow(GetDlgItem(hDlg, COMBO_KB_KEY), 
                                 (bKBKey ? TRUE : FALSE));
	
				break;

				case COMBO_KB_KEY:
					nSel= (int)SendMessage(GetDlgItem(hDlg, COMBO_KB_KEY), 
                                           CB_GETCURSEL, 0, 0L);
					
					uKBKey = ary_KBKey[nSel];

				break;

				default:
					//return FALSE;
					bRetValue = FALSE;
				break;
					}
		break;
		
        case WM_HELP:
			if ( GetDesktop() != DESKTOP_DEFAULT )
				return FALSE;

            WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, __TEXT("osk.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_rgHelpIds);
            //return(TRUE);
			bRetValue = TRUE;
        
        case WM_CONTEXTMENU:  // right mouse click
			if ( GetDesktop() != DESKTOP_DEFAULT )
				return FALSE;

            WinHelp((HWND) wParam, __TEXT("osk.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_rgHelpIds);
            break;

		default:
			//return FALSE;
			bRetValue = FALSE;
		break;
		}		


	for (i=0; i<11; i++)
		free(sKBKey[i]);

	//return TRUE;
	return bRetValue;
}

