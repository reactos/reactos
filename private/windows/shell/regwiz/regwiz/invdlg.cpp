/*********************************************************************
Registration Wizard

InventoryDialog.cpp
10/21/94 - Tracy Ferrier
02/12/98 - Suresh Krishnan
07/20/98 -  Modified as System Inventory items to be displayed using ListView control
		    and added an Edit control for Computer Manufacturer and Model entry
			EnabelOrDisableSIItems() function sets the flag if the SI info need
			to be sent to the backend

(c) 1994-95 Microsoft Corporation
**********************************************************************/

#include <Windows.h>
#include <stdio.h>
#include "RegPage.h"
#include "regwizmain.h"
#include "resource.h"
#include "dialogs.h"
#include "regutil.h"
#include <sysinv.h>
#include <rw_common.h>

//#include <windowsx.h>
#include <commctrl.h>

// ListVive control related supporting functions
HWND CreateListView(HINSTANCE hInstance,
					HWND hwndParent,
					RECT *pRect);

void AddIconToListView(HINSTANCE hInstance, HWND hwndListView);
BOOL InitListViewHeaders(HINSTANCE hInstance, HWND hwndListView);
BOOL AddSI_ItemsToListView(HWND hwndSIListView, CRegWizard* pRegWizard);
void ConfigureSIEditFields(CRegWizard* pclRegWizard,HWND hwndDlg);

void ConfigureSIEditFields(CRegWizard* pclRegWizard,HWND hwndDlg);
BOOL ValidateSIDialog(CRegWizard* pclRegWizard,HWND hwndDlg);
int ValidateSIEditFields(CRegWizard* pclRegWizard,HWND hwndDlg);

//
void EnabelOrDisableSIItems( BOOL bAction, CRegWizard* pclRegWizard);

void DestroyListViewResources();



#define NO_OF_SI_ITEMS          13
#define NUMBER_OF_SI_ICONS		5
#define NO_SI_ICONS_FORFUTURE   1
#define  MAX_NO_SI_COLUMNS                       2 // Number of columns
#define  MAX_COLUMNITEM_DESCRIPTION_LEN          25 // Length of Description item on header

//
// Global and Static variables

static HIMAGELIST  himlSmall = NULL;
static HIMAGELIST  himlLarge =NULL;

//  This structure that maps the Device Name in the resource with the ICON and corrosponding
//  index in RegWiz to get the Device Description
//
//
typedef struct  SIItemMapping{
	int iResIndexForDevice;
	int iIconIndex;
	InfoIndex  iIndexToGetDeviceDescription;
} _SIItemMapping ;

static _SIItemMapping  SITable[NO_OF_SI_ITEMS]= {
	{IDS_INFOKEY13,0,kInfoProcessor},// Processor
	{IDS_INFOKEY15,1,kInfoTotalRAM}, // Total RAM
	{IDS_INFOKEY16,2,kInfoTotalDiskSpace}, // Total HardDisk Space
	{IDS_INFOKEY17,3,kInfoRemoveableMedia}, // Removable Media
	{IDS_INFOKEY18,4,kInfoDisplayResolution}, // Display Resolution
	{IDS_INFOKEY20,5,kInfoPointingDevice}, // Pointing Device
	{IDS_INFOKEY21,6,kInfoNetwork}, // Network
	{IDS_SCSI_ADAPTER,7,kScsiAdapterInfo}, // SCSI
	{IDS_INFOKEY22,8,kInfoModem}, // Modem
	{IDS_INFOKEY23,9,kInfoSoundCard}, // SoundCard
	{IDS_INFOKEY24,10,kInfoCDROM}, // CD ROM
	{IDS_INFOKEY25,11,kInfoOperatingSystem},  // Operating System
	{IDS_COMPUTER_MODEL,12,kComputerManufacturer}  // Operating System
};

INT_PTR
CALLBACK
SystemInventoryDialogProc(
                          HWND hwndDlg,
                          UINT uMsg,
                          WPARAM wParam,
                          LPARAM lParam
                          )
/*********************************************************************
Dialog Proc for the Registration Wizard dialog that displays system
inventory information, such as processor type, RAM, display type,
network type, etc.
**********************************************************************/
{

      _TCHAR  szInventory[256];
	CRegWizard* pclRegWizard;
	INT_PTR iRet;
	_TCHAR szInfo[256];
    INT_PTR bStatus;
	static HBITMAP fHBitmap = NULL;
	static int iMaxLabelWidth = 0;
	static BOOL fMaxWidthCalcDone = FALSE;
	static int iShowThisPage= DO_SHOW_THIS_PAGE;
	TriState shouldInclude;

	HWND hSI;
	RECT SIRect;
	RECT SICRect,DlgRect,CliDlgRect;
	HWND	hwndSIListView;

	pclRegWizard = NULL;
	bStatus = TRUE;

	PageInfo *pi = (PageInfo *)GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
	if(pi) {
		pclRegWizard = pi->pclRegWizard;
	}
	

    switch (uMsg)
    {
		case WM_DESTROY:
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, NULL );
			if(fHBitmap) {
				DeleteObject(fHBitmap);
			}
			fHBitmap = NULL;
			break;
		case WM_CLOSE:
			 break;			
        case WM_INITDIALOG:
		{
			pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
			pclRegWizard = pi->pclRegWizard;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, (LONG_PTR)pi );
			SetControlFont( pi->hBigBoldFont, hwndDlg, IDT_TEXT1);
			//
			// Check if System Inv DLL is present
			if( CheckSysInvDllPresent() != SYSINV_DLL_PRESENT) {
				iShowThisPage= DO_NOT_SHOW_THIS_PAGE;
			}


			if(iShowThisPage== DO_SHOW_THIS_PAGE) {
				 GetModemString(pclRegWizard->GetInstance(),szInventory);
				 pclRegWizard->SetInformationString(kInfoModem,szInventory);
				 fHBitmap = LoadBitmap(pclRegWizard->GetInstance(),MAKEINTRESOURCE(IDB_SYSINV_ICONS));
				 NormalizeDlgItemFont(hwndDlg,IDC_TITLE,RWZ_MAKE_BOLD);
				 NormalizeDlgItemFont(hwndDlg,IDC_SUBTITLE);
				 NormalizeDlgItemFont(hwndDlg,IDT_TEXT1);
				 NormalizeDlgItemFont(hwndDlg,IDT_TEXT2);
				 NormalizeDlgItemFont(hwndDlg,IDT_TEXT3);
				 NormalizeDlgItemFont(hwndDlg,IDC_RADIO1);
				 NormalizeDlgItemFont(hwndDlg,IDC_RADIO2);
				 SetWindowText(hwndDlg,pclRegWizard->GetWindowCaption());
				 hwndSIListView = GetDlgItem(hwndDlg,IDC_LIST1);
				 AddIconToListView(pclRegWizard->GetInstance(), hwndSIListView);
				if(!hwndSIListView) {
						// Error in creating List view control  so skip to the next page
						// This a system error which should occure
					iShowThisPage= DO_NOT_SHOW_THIS_PAGE;
				}else {
					// Add SI entries
					InitListViewHeaders(pclRegWizard->GetInstance(), hwndSIListView);
					AddSI_ItemsToListView(hwndSIListView,
							pclRegWizard);
					//ConfigureSIEditFields(pclRegWizard,hwndDlg);
				}
				
			}
			

   	        vDialogInitialized = TRUE;
            return TRUE;
		} // WM_INIT
		break;
		
		break;
		case WM_NOTIFY:
        {   LPNMHDR pnmh = (LPNMHDR)lParam;
            switch( pnmh->code ){
            case PSN_SETACTIVE:
				if( iShowThisPage== DO_NOT_SHOW_THIS_PAGE ) {
					pclRegWizard->SetTriStateInformation(kInfoIncludeSystem,kTriStateFalse);
					pi->iCancelledByUser = RWZ_SKIP_AND_GOTO_NEXT;
					if( pi->iLastKeyOperation == RWZ_BACK_PRESSED){
						PropSheet_PressButton (GetParent( hwndDlg ),PSBTN_BACK);
					}else {
						PropSheet_PressButton (GetParent( hwndDlg ),PSBTN_NEXT);
					}

				}
				else {
					pi->iCancelledByUser = RWZ_PAGE_OK;
					pi->iLastKeyOperation = RWZ_UNRECOGNIZED_KEYPESS;
					PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK);

					shouldInclude = pclRegWizard->GetTriStateInformation(kInfoIncludeSystem);
					RW_DEBUG << "INV DLG  ; ACTIVE  " << shouldInclude << flush;
					if (shouldInclude == kTriStateTrue ){
						CheckRadioButton(hwndDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO1);
						PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
					}
					else if (shouldInclude == kTriStateFalse){
						CheckRadioButton(hwndDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO2);
						PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
					}
					else if (shouldInclude == kTriStateUndefined){
						PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK);
			
					}
					//
					// Enable for previpously entred value in screen
					if(IsDlgButtonChecked(hwndDlg,IDC_RADIO1)){
						PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
					}
					if(IsDlgButtonChecked(hwndDlg,IDC_RADIO2)){
						PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
					}
					/*
					//
					// Init Computer Model Field
					if (pclRegWizard->GetInformationString(kComputerManufacturer,szInfo)){
						SendDlgItemMessage(hwndDlg,IDC_EDIT1,WM_SETTEXT,0,(LPARAM) szInfo);
					}*/

				}

                break;

            case PSN_WIZNEXT:
					switch(pi->iCancelledByUser) {
					case  RWZ_CANCELLED_BY_USER :
					pi->CurrentPage=pi->TotalPages-1;
					PropSheet_SetCurSel(GetParent(hwndDlg),NULL,pi->TotalPages-1);
					break;
					case RWZ_PAGE_OK:
						iRet=0;
					/*	if( ValidateInvDialog(hwndDlg,IDS_BAD_SYSINV) &&
							ValidateSIDialog(pclRegWizard,hwndDlg))*/
						if( ValidateInvDialog(hwndDlg,IDS_BAD_SYSINV))
						{
							BOOL yesChecked = IsDlgButtonChecked(hwndDlg,IDC_RADIO1);
							BOOL noChecked = IsDlgButtonChecked(hwndDlg,IDC_RADIO2);
							if (yesChecked){
								//pclRegWizard->WriteEnableSystemInventory(TRUE);
								// send info to backend
								EnabelOrDisableSIItems(TRUE,pclRegWizard);
								pclRegWizard->SetTriStateInformation(kInfoIncludeSystem,kTriStateTrue);
							}else if (noChecked){
								//pclRegWizard->WriteEnableSystemInventory(FALSE);

								// Do not send to the back end
								EnabelOrDisableSIItems(FALSE,pclRegWizard);
								pclRegWizard->SetTriStateInformation(kInfoIncludeSystem,kTriStateFalse);
							}
						/*
							// Get Computer Model Information
							SendDlgItemMessage(hwndDlg,IDC_EDIT1,WM_GETTEXT,255,(LPARAM) szInfo);
							pclRegWizard->SetInformationString(kComputerManufacturer,szInfo);
							RW_DEBUG << "\n Computer Model " << szInfo << flush;
*/
							pi->CurrentPage++;
							pi->iLastKeyOperation = RWZ_NEXT_PRESSED;
						// Set as Next Key Button Pressed
						}else {
						// Force it it be in this screen
							iRet=-1;
						}
						SetWindowLongPtr( hwndDlg ,DWLP_MSGRESULT, (INT_PTR) iRet);
					break;
					case RWZ_SKIP_AND_GOTO_NEXT:
					default:
						// Do not Validate the page and just go to the next page
						pi->CurrentPage++;
						pi->iLastKeyOperation = RWZ_NEXT_PRESSED;

					break;
				} // end of switch pi->iCancelledByUser
				break;
            case PSN_WIZBACK:
                pi->CurrentPage--;
				pi->iLastKeyOperation = RWZ_BACK_PRESSED;
				break;
			case PSN_QUERYCANCEL :
				if (CancelRegWizard(pclRegWizard->GetInstance(),hwndDlg)) {
					//pclRegWizard->EndRegWizardDialog(IDB_EXIT) ;
					iRet = 1;
					pi->ErrorPage  = kSysInventoryDialog;
					pi->iError     = RWZ_ERROR_CANCELLED_BY_USER;
					SetWindowLongPtr( hwndDlg,DWLP_MSGRESULT, (INT_PTR) iRet);
					pi->iCancelledByUser = RWZ_CANCELLED_BY_USER;
					pi->iLastKeyOperation = RWZ_CANCEL_PRESSED;
					PropSheet_PressButton (GetParent( hwndDlg ),PSBTN_NEXT);

				}else {
					//
					// Prevent Cancell Operation as User does not want to Cancel
					iRet = 1;
				}
				SetWindowLongPtr( hwndDlg,DWLP_MSGRESULT, (INT_PTR) iRet); 				
				break;
				default:
                //bStatus = FALSE;
                break;
            }
        } // WM_Notify
		break;
        case WM_COMMAND:
		{
			switch (wParam)
            {
              case IDC_RADIO2:
			  case IDC_RADIO1:
					if (vDialogInitialized){
						// If the 'No' button is checked, the user is declining
						// the "Non-Microsoft product" offers
						if(IsDlgButtonChecked(hwndDlg,IDC_RADIO1)){
							PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
						}
						if(IsDlgButtonChecked(hwndDlg,IDC_RADIO2)){
							PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
						}
					}
				break;
			  default:
				  break;
            }
		}// End of WM_COMMAND
        break;
        default:
		bStatus = FALSE;
        break;
    }
    return bStatus;

}



BOOL ValidateInvDialog(HWND hwndDlg,int iStrID)
/*********************************************************************
This function checks the two radio buttons in the System Inventory or
Product Inventory dialog boxes.  If neither button is selected,
ValidateInvDialog will put up a validation error dialog, and return
FALSE as the function result; otherwise, TRUE will be returned.

If an error dialog is presented, the string whose resource ID is
passed in the iStrID parameter will displayed in the dialog text
field.
**********************************************************************/
{
	BOOL isYesChecked = IsDlgButtonChecked(hwndDlg,IDC_RADIO1) == 1 ? TRUE : FALSE;
	BOOL isNoChecked = IsDlgButtonChecked(hwndDlg,IDC_RADIO2) == 1 ? TRUE : FALSE;

	if (isYesChecked == TRUE || isNoChecked == TRUE)
	{
		return TRUE;
	}
	else
	{
		_TCHAR szMessage[256];
		HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwndDlg,GWLP_HINSTANCE);
		LoadString(hInstance,iStrID,szMessage,256);
		RegWizardMessageEx(hInstance,hwndDlg,IDD_INVALID_DLG,szMessage);
		return FALSE;
	}
	


}







//
//
//
//
HWND CreateListView(HINSTANCE hInstance,
					HWND hwndParent,
					RECT *pRect)
{
	DWORD       dwStyle;
	HWND        hwndListView;
	HIMAGELIST  himlSmall;
	HIMAGELIST  himlLarge;
	BOOL        bSuccess = TRUE;
	dwStyle =   WS_TABSTOP  |
				WS_CHILD |
				WS_BORDER |
				LVS_REPORT |//LVS_LIST |
                LVS_SHAREIMAGELISTS | // LVS_NOCOLUMNHEADER |
				WS_VISIBLE;

	hwndListView = CreateWindowEx( WS_EX_CLIENTEDGE | WS_EX_TOPMOST,          // ex style
                                   WC_LISTVIEW,               // class name - defined in commctrl.h
                                   NULL,                      // window text
                                   dwStyle,                   // style
                                   pRect->left,                         // x position
                                   pRect->top,
								   pRect->right,
								   pRect->bottom,
								  //pRect->right - pRect->left, // width
								  //pRect->bottom - pRect->top,//  height
                                   hwndParent,                // parent
                                   (HMENU) IDC_LIST1,       // ID
                                   hInstance,                // instance
                                 NULL);                     // no extra data

   if(!hwndListView)
   return NULL;
	
   AddIconToListView(hInstance, hwndListView);
   return hwndListView;

}


void AddIconToListView( HINSTANCE hInstance, HWND hwndListView )
{
	HICON hIconItem;

	himlSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON),TRUE,NUMBER_OF_SI_ICONS,
		NO_SI_ICONS_FORFUTURE);

	himlLarge = ImageList_Create(GetSystemMetrics(SM_CXICON),
		GetSystemMetrics(SM_CYICON),TRUE,NUMBER_OF_SI_ICONS,
		NO_SI_ICONS_FORFUTURE);


	// Add Icon to image List
	for (int i=0; i <NO_OF_SI_ITEMS ;i++ ) {
		hIconItem =  LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SIICON1+i));
		ImageList_AddIcon(himlSmall,hIconItem);
		ImageList_AddIcon(himlLarge,hIconItem);
		DeleteObject(hIconItem );
	}
/***
	hIconItem =  LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BADREGWIZ));
	ImageList_AddIcon(himlSmall,hIconItem);
	ImageList_AddIcon(himlLarge,hIconItem);
	DeleteObject(hIconItem );

	hIconItem =  LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WORLD));
	ImageList_AddIcon(himlSmall,hIconItem);
	ImageList_AddIcon(himlLarge,hIconItem);
	DeleteObject(hIconItem );

	hIconItem =  LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ENVELOPE));
	ImageList_AddIcon(himlSmall,hIconItem);
	ImageList_AddIcon(himlLarge,hIconItem);
	DeleteObject(hIconItem );

	hIconItem =  LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REGWIZ));
	ImageList_AddIcon(himlSmall,hIconItem);
	ImageList_AddIcon(himlLarge,hIconItem);
	DeleteObject(hIconItem );
	**/
	
	// Assign  the image lists  to the list view control
	ListView_SetImageList(hwndListView, himlSmall, LVSIL_SMALL);
	ListView_SetImageList(hwndListView, himlLarge, LVSIL_NORMAL);

}

BOOL InitListViewHeaders(HINSTANCE hInstance, HWND hwndListView)
{
	LV_COLUMN   lvColumn;
	RECT         CliRect;
	int         i = 0;
	TCHAR       szString[MAX_NO_SI_COLUMNS][MAX_COLUMNITEM_DESCRIPTION_LEN] =
	{	 TEXT("Device"),
		 TEXT("Description")
	};

	LoadString(hInstance,IDS_SI_DEVICENAME, szString[0],MAX_COLUMNITEM_DESCRIPTION_LEN);
	LoadString(hInstance,IDS_SI_DEVICEDESCRIPTION, szString[1],MAX_COLUMNITEM_DESCRIPTION_LEN);
	//initialize the columns
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	//lvColumn.cx = 100;
	GetClientRect(hwndListView,&CliRect);

	lvColumn.cx = 133;
	lvColumn.pszText = szString[0];
	SendMessage(hwndListView, LVM_INSERTCOLUMN, (WPARAM)i, (LPARAM)&lvColumn);
	
	i = 1;
	lvColumn.cx = CliRect.right - 149;
	lvColumn.pszText = szString[1];
	SendMessage(hwndListView, LVM_INSERTCOLUMN, (WPARAM)i, (LPARAM)&lvColumn);

   return TRUE;

}



/********************************************************************************

   AddSI_ItemsToLIstVIew
   This function adds the SI items in the list view

******************************************************************************/

BOOL AddSI_ItemsToListView( HWND hwndListView, CRegWizard* pclRegWizard)
{
	LV_ITEM     lvItem;
	int         i,nImageCount;
	TCHAR       szTempDevice[MAX_PATH];
				

	HIMAGELIST  himl;
	IMAGEINFO   ii;
	HINSTANCE hInstance;
	BOOL bOemInfoPresent = TRUE;

	 hInstance = pclRegWizard->GetInstance();

	// SendMessage(hwndListView, WM_SETREDRAW, FALSE, 0);
	//empty the list
	SendMessage(hwndListView, LVM_DELETEALLITEMS, 0, 0);
	//get the number of icons in the image list
	himl = (HIMAGELIST)SendMessage(hwndListView, LVM_GETIMAGELIST, (WPARAM)LVSIL_SMALL, 0);
	nImageCount = ImageList_GetImageCount(himl);
	for(i = 0; i < NO_OF_SI_ITEMS ; i++) {
		// Get Device Name
		LoadString(hInstance,SITable[i].iResIndexForDevice, szTempDevice, 256);
		//fill in the LV_ITEM structure for the first item
		lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
		lvItem.pszText = szTempDevice;
		lvItem.iImage = SITable[i].iIconIndex;
		lvItem.iItem = (INT)SendMessage(hwndListView, LVM_GETITEMCOUNT, 0, 0);
		lvItem.iSubItem = 0;
	   //add the item - get the index in case the ListView is sorted
		SendMessage(hwndListView, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);

	   //GetDevice Description
		if(!pclRegWizard->GetInformationString(SITable[i].iIndexToGetDeviceDescription,
			szTempDevice))
		{
			if(i == 12)
			{
				bOemInfoPresent = FALSE;
			}

			LoadString(hInstance,IDS_SYSINV_NOTFOUND,szTempDevice, 256);
		}
		lvItem.iSubItem = 1;
		SendMessage(hwndListView, LVM_SETITEM, 0, (LPARAM)&lvItem);
		
   	}
	
	if(!bOemInfoPresent)
	{
		ListView_DeleteItem(hwndListView,12);
	}

	SendMessage(hwndListView, WM_SETREDRAW, TRUE, 0);
	UpdateWindow(hwndListView);
	//EnableWindow(hwndListView, FALSE);
	return TRUE;
}


//
//  This function should be called before exiting RegWiz this will release the resources allocated
//  for list view control (image list)
//
//
void DestroyListViewResources()
{
	if(himlSmall) {
		ImageList_Destroy(himlSmall);
		himlSmall = NULL;
	}
	if(himlLarge) {
		ImageList_Destroy(himlLarge);
		himlLarge = NULL;
	}

}

//
//    bAction : TRUE  ( Enabel send info to  backend)
//              FALSE ( Do not send info to backend )
//
//

void EnabelOrDisableSIItems( BOOL bAction, CRegWizard* pclRegWizard)
{

	int iIndex;
	// Itens in the List View Control
	for(iIndex = 0; iIndex < NO_OF_SI_ITEMS ; iIndex++) {
		pclRegWizard->WriteEnableInformation(SITable[iIndex].iIndexToGetDeviceDescription,
			bAction);

	}
	// Computer Model
//	pclRegWizard->WriteEnableInformation(kComputerManufacturer, bAction);

}

//BOOL ValidateSIDialog(CRegWizard* pclRegWizard,HWND hwndDlg)
/*********************************************************************
Returns TRUE if all required user input is valid in the Address
dialog.  If any required edit field input is empty, ValidateAddrDialog
will put up a message box informing the user of the problem, and set
the focus to the offending control.
**********************************************************************/
/*{
	int iInvalidEditField = ValidateSIEditFields(pclRegWizard,hwndDlg);
	if (iInvalidEditField == NULL)
	{
		return TRUE;
	}
	else
	{
		_TCHAR szLabel[128];
		_TCHAR szMessage[256];
		CRegWizard::GetEditTextFieldAttachedString(hwndDlg,iInvalidEditField,szLabel,128);
		HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwndDlg,GWLP_HINSTANCE);
		LoadAndCombineString(hInstance,szLabel,IDS_BAD_PREFIX,szMessage);
		RegWizardMessageEx(hInstance,hwndDlg,IDD_INVALID_DLG,szMessage);
		HWND hwndInvField = GetDlgItem(hwndDlg,iInvalidEditField);
		SetFocus(hwndInvField);
		return FALSE;
	}
}*/


//int ValidateSIEditFields(CRegWizard* pclRegWizard,HWND hwndDlg)
/*********************************************************************
ValidateFEAddrEditFields validates all edit fields in the Address
dialog.  If any required field is empty, the ID of the first empty
edit field control will be returned as the function result.  If all
fields are OK, NULL will be returned.
**********************************************************************/
/*{

	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT1)) return IDC_EDIT1;
	return NULL;
}


void ConfigureSIEditFields(CRegWizard* pclRegWizard,HWND hwndDlg)
{
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT1,kSIComputerManufacturer,IDC_COMPUTER_MODEL);
}*/