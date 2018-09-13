/*
	File 	: RegPage.CPP
	Date 	: 02/11/98
	Author 	: Suresh Krishnan
	
	Registration Wizard Page Control using Wizard 97 control
	
	This file exposes
	DoRegistrationWizard () which creates the necessary screen pages for RegWIz
	Modification History:
	4/29/98 : Removed Reseller screen as per Microsoft Request
	4/28/98 : Added Business user and Homer user screen

*/
#include <tchar.h>
#include <Windows.h>
#include <Resource.h>
#include <rw_common.h>
#include <fe_util.h>
#include "RegWizMain.h"
#include "RegPage.h"

BOOL bPostSuccessful = FALSE;

UINT CALLBACK DefaultPropSheetPageProc(HWND hwnd,UINT uMsg, LPPROPSHEETPAGE ppsp);
UINT CALLBACK AddressPropSheetPageProc(HWND hwnd,UINT uMsg, LPPROPSHEETPAGE ppsp);
UINT CALLBACK AddressFEPropSheetPageProc(HWND hwnd,UINT uMsg, LPPROPSHEETPAGE ppsp);
UINT CALLBACK ResellerPropSheetPageProc(HWND hwnd,UINT uMsg, LPPROPSHEETPAGE ppsp);
UINT CALLBACK RegisterPropSheetPageProc(HWND hwnd,UINT uMsg, LPPROPSHEETPAGE ppsp);

INT_PTR CALLBACK WizardDlgProc(IN HWND     hwnd,IN UINT     uMsg,IN WPARAM   wParam,IN LPARAM   lParam);
extern INT_PTR  CALLBACK WelcomeDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR  CALLBACK InformDialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
extern INT_PTR  CALLBACK NameDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR  CALLBACK NameFEDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR  CALLBACK AddressDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR  CALLBACK AddressFEDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR  CALLBACK ResellerDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR  CALLBACK SystemInventoryDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR  CALLBACK ProdInventoryDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR  CALLBACK RegisterDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR  CALLBACK DialupScreenProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR  CALLBACK FinalScreenDialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam);
//extern INT_PTR  CALLBACK FinalFailedScreenDialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam);
extern INT_PTR  CALLBACK BusinessUserDialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam);
extern INT_PTR  CALLBACK HomeUserDialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam);

typedef struct RegWizPageInfo{
	DLGPROC		pfnCallback;
	LPFNPSPCALLBACK  pfnPropSheetProc;
	WORD            wDlgId;
	WORD			wTitle;
	WORD			wSubTitle;
}_RegWizPageInfo;

RegWizPageInfo RwPgInf[] = {
{WelcomeDialogProc,DefaultPropSheetPageProc, IDD_WELCOME_FOR98,IDS_WELCOME_SCR_TITLE,IDS_WELCOME_SCR_STITLE}, // Welcome Screen
{InformDialogProc,DefaultPropSheetPageProc,IDD_INFORM, IDS_INFORM_SCR_TITLE,IDS_INFORM_SCR_STITLE},  // Inform Screen
{NameDialogProc,DefaultPropSheetPageProc,IDD_NAME,  IDS_NAME_SCR_TITLE,IDS_NAME_SCR_STITLE},  // Name Screen
{AddressDialogProc,AddressPropSheetPageProc ,IDD_ADDRESS,		IDS_ADDRESS_SCR_TITLE,IDS_ADDRESS_SCR_STITLE},  // Address Screen
//{ResellerDialogProc,ResellerPropSheetPageProc,IDD_RESELLER ,  IDS_RESELLER_SCR_TITLE,IDS_RESELLER_SCR_STITLE},  // Reseller Screen
{BusinessUserDialogProc,DefaultPropSheetPageProc,IDD_BUSINESS_QUESTIONS ,  IDS_BUSINESSUSER_SCR_TITLE,IDS_BUSINESSUSER_SCR_STITLE},  // Business User Screen
{HomeUserDialogProc,DefaultPropSheetPageProc,IDD_HOME_QUESTIONS ,  IDS_HOMEUSER_SCR_TITLE,IDS_HOMEUSER_SCR_STITLE},  // Home user  Screen
{SystemInventoryDialogProc,DefaultPropSheetPageProc,IDD_INVENTORY,		IDS_SYSINV_SCR_TITLE,IDS_SYSINV_SCR_STITLE},  // Sys Inv Screen
{ProdInventoryDialogProc,DefaultPropSheetPageProc,IDD_PRODINVENTORY, IDS_PRODINV_SCR_TITLE,IDS_PRODINV_SCR_STITLE},  // Product Inventory Screen
{RegisterDialogProc,RegisterPropSheetPageProc,IDD_REGISTER, IDS_REGISTER_SCR_TITLE,IDS_REGISTER_SCR_STITLE},  // Register Screen
{DialupScreenProc,DefaultPropSheetPageProc,IDD_DIAL, IDS_DIALUP_SCR_TITLE,IDS_DIALUP_SCR_STITLE}  // Dialup Screen

};

RegWizPageInfo RwFEPgInf[] = {
{NameFEDialogProc,DefaultPropSheetPageProc,IDD_NAME_FE,  IDS_NAME_SCR_TITLE,IDS_NAME_SCR_STITLE},  // Name Screen
{AddressFEDialogProc,AddressFEPropSheetPageProc ,IDD_ADDRESS_FE,		IDS_ADDRESS_SCR_TITLE,IDS_ADDRESS_SCR_STITLE},  // Address Screen

};


//
//  Default Property Sheet Procedure called during Property Sheet creatinion
//  and deletion
//
//
//
UINT CALLBACK DefaultPropSheetPageProc(HWND hwnd,
								UINT uMsg,
								LPPROPSHEETPAGE ppsp
								)
{
	
	switch(uMsg) {
	case PSPCB_CREATE :
	default:
		break;

	}
	return 1;

}

INT_PTR DoRegistrationWizard(HINSTANCE hInstance,
						  CRegWizard* pclRegWizard,
						  LPTSTR szProductPath)

{
    UINT				iPage;
	UINT			kNumPages           = kDialogExit+1;
	INT_PTR			iError;

	UINT            i                   = 0;
    BOOL            bStatus             = TRUE;
    PageInfo        PageInfo            = {0};
    PROPSHEETPAGE   psp                 = {0};
    HPROPSHEETPAGE  *ahpsp;
    PROPSHEETHEADER psh                 = {0};

	iError = RWZ_ERROR_CANCELLED_BY_USER;

	#ifdef _LOG_IN_FILE
		 RW_DEBUG  << "\n INFORM DIALOG HEADER"<< flush;
	#endif

	// Allocate and initialize
    ahpsp = (HPROPSHEETPAGE *) GlobalAlloc( GMEM_FIXED, sizeof( HPROPSHEETPAGE) *
										kNumPages);

	for(iPage=0;iPage < kNumPages;iPage++) {
		ahpsp[iPage] = 0;			
	}
	iPage = 0;
	// Create Welcome Page
	psp.pfnCallback         = DefaultPropSheetPageProc;
	psp.dwSize              = sizeof( psp );
    psp.dwFlags             = PSP_DEFAULT;
    psp.hInstance           = hInstance;
    psp.lParam              = (LPARAM)&PageInfo;
	psp.pfnDlgProc          = WelcomeDialogProc;
	psp.dwFlags             = PSP_DEFAULT | PSP_HIDEHEADER| PSP_USECALLBACK ;;
    psp.pszTemplate         = MAKEINTRESOURCE( IDD_WELCOME);
	ahpsp[iPage++]          = CreatePropertySheetPage( &psp );

	psp.dwFlags             = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE |PSP_USECALLBACK ;
	

	for(;iPage < kNumPages-1 ;){
		psp.pfnDlgProc          = RwPgInf[iPage].pfnCallback;
			//(int (__stdcall *)(void))NameDialogProc;
		psp.pfnCallback         =  RwPgInf[iPage].pfnPropSheetProc;
		//psp.pfnDlgProc          = WizardDlgProc;
		psp.pszHeaderTitle      = MAKEINTRESOURCE(RwPgInf[iPage].wTitle);
		psp.pszHeaderSubTitle   = MAKEINTRESOURCE(RwPgInf[iPage].wSubTitle);
		
		if(iPage == kInformDialog)
		{
			_TCHAR szheader[256],szheaderInfo[256],szInfo[256];
			LoadString(hInstance,IDS_INFORM_SCR_STITLE,szheaderInfo,256);
			pclRegWizard->GetInputParameterString(IDS_INPUT_PRODUCTNAME,szInfo);
			_stprintf(szheader,szheaderInfo,szInfo);
			psp.pszHeaderSubTitle   = szheader;
		}
		else
		if(iPage == kDialupDialog)
		{
			_TCHAR szheader[256],szheaderInfo[256],szInfo[256];
			LoadString(hInstance,IDS_DIALUP_SCR_STITLE,szheaderInfo,256);
			pclRegWizard->GetInputParameterString(IDS_INPUT_PRODUCTNAME,szInfo);
			_stprintf(szheader,szheaderInfo,szInfo);
			psp.pszHeaderSubTitle   = szheader;
		}

		psp.pszTemplate         = MAKEINTRESOURCE(RwPgInf[iPage].wDlgId  );

		// Change the
		if(IsFarEastCountry(hInstance) ==  kFarEastCountry ) {
			switch( iPage) {
			case kNameDialog   :
			case kAddressDialog:
				psp.pfnDlgProc          = RwFEPgInf[iPage-kNameDialog].pfnCallback;
				psp.pfnCallback         = RwFEPgInf[iPage-kNameDialog].pfnPropSheetProc;
				psp.pszHeaderTitle      = MAKEINTRESOURCE(RwFEPgInf[iPage-kNameDialog].wTitle);
				psp.pszHeaderSubTitle   = MAKEINTRESOURCE(RwFEPgInf[iPage-kNameDialog].wSubTitle);
				psp.pszTemplate         = MAKEINTRESOURCE(RwFEPgInf[iPage-kNameDialog].wDlgId);
				break;
			default :
			break;
			}
		}
		ahpsp[iPage++]          = CreatePropertySheetPage( &psp );
	}

	// Create Last Page
	//psp.dwFlags             = PSP_DEFAULT ;
	psp.pszTemplate         = MAKEINTRESOURCE(IDD_FAILURE_REGISTRATION);	
	psp.pfnDlgProc          = FinalScreenDialogProc;

	psp.dwFlags             = PSP_DEFAULT | PSP_HIDEHEADER;

    ahpsp[iPage++]          = CreatePropertySheetPage( &psp );

	psh.dwFlags             = PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
	psh.dwFlags             &= ~(PSH_HASHELP);
	psh.pszbmWatermark      = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader         = MAKEINTRESOURCE(IDB_BANNER);

    psh.dwSize              = sizeof( psh );
    psh.hInstance           = hInstance;
    psh.hwndParent          = NULL;
    psh.pszCaption          = _T("Registration Wizard for Windows NT ");//MAKEINTRESOURCE( IDS_TITLE );
    psh.phpage              = ahpsp;
    psh.nStartPage          = 0;
    psh.nPages              = iPage;
    PageInfo.TotalPages     = iPage;
	PageInfo.pclRegWizard   = pclRegWizard;
	PageInfo.hInstance		= hInstance;
	PageInfo.pDialupHelper  = NULL;
	PageInfo.ErrorPage      = -1;
	PageInfo.dwConnectionType   =1;
    PageInfo.pszProductPath = szProductPath;

    //
    // Create the bold fonts.
    //
    SetupFonts( hInstance, NULL, &PageInfo.hBigBoldFont, &PageInfo.hBoldFont );

    //
    // Validate all the pages.
    //
    for( i = 0; i < kNumPages; i++ )
    {
        if( ahpsp[i] == 0 )
        {
            bStatus = FALSE;
        }
    }

	INT_PTR iRetVal=0;
    //
    // Display the wizard.
    //
    if( bStatus )
    {
		iRetVal = PropertySheet( &psh );
        if(iRetVal  == -1 )
        {
            bStatus = FALSE;
        }
    }
	if( iRetVal == 0 ) {
		// Cancelled By the User
		if( PageInfo.ErrorPage  == kWelcomeDialog) {
				
				iError = PageInfo.iError;
				return iError;
			;
		}
	}
    if( !bStatus )
    {
        //
        // Manually destroy the pages if something failed.
        //
        for( i = 0; i < psh.nPages; i++)
        {
            if( ahpsp[i] )
            {
                DestroyPropertySheetPage( ahpsp[i] );
            }
        }
    }

    //
    // Destroy the fonts that were created.
    //
    DestroyFonts( PageInfo.hBigBoldFont, PageInfo.hBoldFont );

    return iError;


}



//
// Wizard dialog proc.
//
INT_PTR CALLBACK
WizardDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{
    BOOL bStatus = TRUE;
    PageInfo *pi = (PageInfo *)GetWindowLongPtr( hwnd, GWLP_USERDATA );

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pi );
        if( ( pi->CurrentPage == 0 ) || ( pi->CurrentPage == pi->TotalPages - 1 ) )
        {
        	SetControlFont( pi->hBigBoldFont, hwnd, IDT_TEXT1);
	        //SetControlFont( pi->hBoldFont,    hwnd, IDC_BOLDTITLE);
        }
        break;

    case WM_DESTROY:
        SetWindowLongPtr( hwnd, GWLP_USERDATA, NULL );
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code )
            {
            case PSN_SETACTIVE:
                if( pi->CurrentPage == 0 )
                    PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT );
                else if( pi->CurrentPage == pi->TotalPages - 1 ) {
                    PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK | PSWIZB_FINISH );
					// PropSheet_SetFinishText(GetParent( hwnd ),"Register");
				}
                else
                    PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK );
                break;

            case PSN_WIZNEXT:
                pi->CurrentPage++;
                break;

            case PSN_WIZBACK:
                pi->CurrentPage--;
                break;
			case PSN_QUERYCANCEL :
				
				SetWindowLongPtr( GetParent( hwnd ),DWLP_MSGRESULT, (LONG_PTR) 5);
				break;

            default:
                bStatus = FALSE;
                break;
            }
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}


VOID
SetControlFont(
    IN HFONT    hFont,
    IN HWND     hwnd,
    IN INT      nId
    )
{
	if( hFont )
    {
    	HWND hwndControl = GetDlgItem(hwnd, nId);

    	if( hwndControl )
        {
        	SetWindowFont(hwndControl, hFont, TRUE);

        }
    }
}

VOID
SetupFonts(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN HFONT        *pBigBoldFont,
    IN HFONT        *pBoldFont
    )
{
    //
	// Create the fonts we need based on the dialog font
    //
	NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

	LOGFONT BigBoldLogFont  = ncm.lfMessageFont;
	LOGFONT BoldLogFont     = ncm.lfMessageFont;

    //
	// Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;
	BoldLogFont.lfWeight      = FW_BOLD;

    TCHAR FontSizeString[MAX_PATH];
    INT FontSize;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
    if(!LoadString(hInstance,IDS_LARGEFONTNAME,BigBoldLogFont.lfFaceName,LF_FACESIZE))
    {
        lstrcpy(BigBoldLogFont.lfFaceName,TEXT("MS Shell Dlg"));
    }

    if(LoadString(hInstance,IDS_LARGEFONTSIZE,FontSizeString,sizeof(FontSizeString)/sizeof(TCHAR)))
    {
        FontSize = _tcstoul( FontSizeString, NULL, 10 );
    }
    else
    {
        FontSize = 12;
    }

	HDC hdc = GetDC( hwnd );

    if( hdc )
    {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);

        *pBigBoldFont = CreateFontIndirect(&BigBoldLogFont);
		*pBoldFont    = CreateFontIndirect(&BoldLogFont);

        ReleaseDC(hwnd,hdc);
    }
}

VOID
DestroyFonts(
    IN HFONT        hBigBoldFont,
    IN HFONT        hBoldFont
    )
{
    if( hBigBoldFont )
    {
        DeleteObject( hBigBoldFont );
    }

    if( hBoldFont )
    {
        DeleteObject( hBoldFont );
    }
}
