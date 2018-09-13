/*

	File    : RegPage.h
	Date    : 12/31/97
	Author  : Suresh Krishnan
	Regsitration Wizard Page info using Wizard 97 control
	Modification History:
	4/29/98 : Removed Reseller screen constant
	4/28/98 : Added constants for Business user and Homer user screen



*/
#ifndef __REGWIZPAGE__
#define __REGWIZPAGE__

//
//
//
//
#include <Windows.h>
#include <windowsx.h>
#include <PRSHT.H>



class	 CRegWizard;
class    DialupHelperClass;
//
//  CONTROL ID of the Wizard 97 control
//  This is got using the SPY
//


#define  RWZ_WIZ97_STATIC_ID  3027
#define  RWZ_WIZ97_FINISH_ID  3025
#define  RWZ_WIZ97_NEXT_ID    3024
#define  RWZ_WIZ97_BACK_ID    3023
#define  RWZ_WIZ97_CANCEL_ID     2
#define  RWZ_WIZ97_HELP_ID       9


// for iLastKeyOperation
#define RWZ_UNRECOGNIZED_KEYPESS   0
#define RWZ_BACK_PRESSED	1
#define RWZ_NEXT_PRESSED	2
#define RWZ_CANCEL_PRESSED  3

//
//
//iCancelledByUser can have the following
#define  RWZ_SKIP_AND_GOTO_NEXT 3
#define  RWZ_ABORT_TOFINISH     2
#define  RWZ_CANCELLED_BY_USER  1
#define  RWZ_PAGE_OK            0
struct PageInfo
{

    UINT		CurrentPage;
    UINT		TotalPages;
	HFONT		hBigBoldFont;
	HFONT		hBoldFont;
	HINSTANCE 	hInstance;
	UINT        ErrorPage;  // Set By the Page exiting
	INT_PTR		iError;    // Error
	DWORD       dwConnectionType; // Via Network or Dialup
					// Set in the Welcome Screen and used in the Register Screen
	DWORD       dwMsgId;  // Msg COntext Id to be  displayed on the last page
	HPROPSHEETPAGE  *ahpsp ;  // Handle of Property sheet pages created
	CRegWizard* pclRegWizard;
	DialupHelperClass  *pDialupHelper; // This is Dialup helper class used in Dialup Screen
	int         iCancelledByUser;
	int         iLastKeyOperation;
      LPTSTR   pszProductPath;
};

//
//	Dialog Index of Registration Wizard
//

typedef enum
{	kWelcomeDialog,
	kInformDialog,
	kNameDialog,
	kAddressDialog,
	//kResellerDialog,
	kBusinessUserDialog,
	kHomeUserDialog,
	kSysInventoryDialog,
	kProductInventoryDialog,
	kRegisterDialog,
	kDialupDialog,
	kDialogExit
}RegWizScreenIndex;

//
// Used by System Inventory and Product Inventory
//
#define DO_NOT_SHOW_THIS_PAGE 1
#define DO_SHOW_THIS_PAGE     2

BOOL
Is256ColorSupported(
    VOID
    );



INT_PTR CALLBACK
WizardDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

VOID
SetControlFont(
    IN HFONT    hFont,
    IN HWND     hwnd,
    IN INT      nId
    );

VOID
SetupFonts(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN HFONT        *pBigBoldFont,
    IN HFONT        *pBoldFont
    );

VOID
DestroyFonts(
    IN HFONT        hBigBoldFont,
    IN HFONT        hBoldFont
    );

INT_PTR
DoRegistrationWizard(
                     HINSTANCE hInstance,
                     CRegWizard* clRegWizard,
                     LPTSTR szProductPath
                     );
#endif
