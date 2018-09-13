#include "pch.hxx"
#pragma hdrstop

#include "pgbase.h"
#include "resource.h"
#include "DlgFonts.h"

// Initialization of static members
CWizardPageOrder WizardPage::sm_WizPageOrder;

WizardPage::WizardPage(
					   LPPROPSHEETPAGE ppsp,
					   int nIdTitle,
					   int nIdSubTitle
					   ) : m_hwnd(NULL), m_dwPageId(0)
{
	_ASSERTE(NULL != ppsp);
	
	// If we have a subtitle, we must have a title
	_ASSERTE(nIdSubTitle?nIdTitle:TRUE);
	
	//
	// Many of the members are 0 or NULL.
	//
	ZeroMemory(ppsp, sizeof(PROPSHEETPAGE));
	
	ppsp->dwSize	= sizeof(PROPSHEETPAGE);
	
	ppsp->dwFlags	= PSP_DEFAULT;
	ppsp->hInstance = g_hInstDll;
	
	// If we are using a title/subtitle, include the flags
	// Otherwise, hide the header
	if(nIdTitle)
	{
		ppsp->dwFlags |= PSP_USEHEADERTITLE | (nIdSubTitle?PSP_USEHEADERSUBTITLE:0);
		ppsp->pszHeaderTitle = MAKEINTRESOURCE(nIdTitle);
		ppsp->pszHeaderSubTitle = MAKEINTRESOURCE(nIdSubTitle);
		
	}
	else
		ppsp->dwFlags |= PSP_HIDEHEADER;
	
	
	//
	// Callback is a base class function.  The derived page
	// classes need to implement OnPropSheetPageCreate() and 
	// OnPropSheetPageRelease() if they want to handle this callback.
	// By WizardPage::OnPropSheetPageCreate() returns 1.
	//
	ppsp->pfnCallback = WizardPage::PropSheetPageCallback;
	ppsp->dwFlags	 |= (PSP_USECALLBACK /*| PSP_USEREFPARENT*/); // JMC: TODO: Do we want PSP_USEREFPARENT
	
	//
	// Store "this" in the page struct so we can call member functions
	// from the page's message proc.
	//
	_ASSERTE(NULL != this);
	ppsp->lParam = (LPARAM)this;
	
	//
	// All dialog messages first go through the base class' message proc.
	// Virtual functions are called for some messages.	If not processed
	// using a message-specific virtual function, the message is passed 
	// to the derived class instance through the virtual funcion HandleMsg.
	//
	ppsp->pfnDlgProc = WizardPage::DlgProc;
}


WizardPage::~WizardPage(
						VOID
						)
{
}


UINT
WizardPage::PropSheetPageCallback(
								  HWND hwnd,
								  UINT uMsg,
								  LPPROPSHEETPAGE ppsp
								  )
{
	UINT uResult = 0;
	WizardPage *pThis = (WizardPage *)ppsp->lParam;
	_ASSERTE(NULL != pThis);
	
	switch(uMsg)
	{
	case PSPCB_CREATE:
		uResult = pThis->OnPropSheetPageCreate(hwnd, ppsp);
		break;
		
	case PSPCB_RELEASE:
		uResult = pThis->OnPropSheetPageRelease(hwnd, ppsp);
		//
		// IMPORTANT:
		// This is where we delete each property sheet page.
		// HERE and ONLY HERE.
		//
		//			  delete pThis; // We won't do this since we'll keep our own list
		// The reason it won't work is because if you never get to a page, you
		// will never get this message
		break;
	}
	return uResult;
}



//
// This is a static method.
//
BOOL 
WizardPage::DlgProc(
					HWND hwnd, 
					UINT uMsg, 
					WPARAM wParam, 
					LPARAM lParam
					)
{
	BOOL bResult		= FALSE;
	PROPSHEETPAGE *ppsp = NULL;
	
	if (WM_INITDIALOG == uMsg)
		ppsp = (PROPSHEETPAGE *)lParam;
	else
		ppsp = (PROPSHEETPAGE *)GetWindowLong(hwnd, GWL_USERDATA);
	
	if (NULL != ppsp)
	{
		WizardPage *pThis = (WizardPage *)ppsp->lParam;
		_ASSERTE(NULL != pThis);
		
		switch(uMsg)
		{
		case WM_INITDIALOG:
			{
				// The following will set fonts for 'known' controls,
				DialogFonts_InitWizardPage(hwnd);
				
				//
				// Store address of PROPSHEETPAGE struct for this page 
				// in window's user data.
				//
				SetWindowLong(hwnd, GWL_USERDATA, (LONG)lParam);
				pThis->m_hwnd = hwnd;
				bResult = pThis->OnInitDialog(hwnd, wParam, lParam);
			}
			break;
			
		case WM_NOTIFY:
			bResult = pThis->OnNotify(hwnd, wParam, lParam);
			break;
			
		case PSM_QUERYSIBLINGS:
			bResult = pThis->OnPSM_QuerySiblings(hwnd, wParam, lParam);
			break;
			
		case WM_COMMAND:
			bResult = pThis->OnCommand(hwnd, wParam, lParam);
			
		default:
			//
			// Let derived class instance handle any other messages
			// as needed.
			//
			bResult = pThis->HandleMsg(hwnd, uMsg, wParam, lParam);
			break;
		}
	}
	
	return bResult;
}


LRESULT
WizardPage::OnNotify(
					 HWND hwnd,
					 WPARAM wParam,
					 LPARAM lParam
					 )
{
	INT idCtl		= (INT)wParam;
	LPNMHDR pnmh	= (LPNMHDR)lParam;	  
	LRESULT lResult = 0;
	
	switch(pnmh->code)
	{
	case PSN_APPLY:
		lResult = OnPSN_Apply(hwnd, idCtl, (LPPSHNOTIFY)pnmh);
		break;
	case PSN_HELP:
		lResult = OnPSN_Help(hwnd, idCtl, (LPPSHNOTIFY)pnmh);
		break;
	case PSN_KILLACTIVE:
		lResult = OnPSN_KillActive(hwnd, idCtl, (LPPSHNOTIFY)pnmh);
		break;
	case PSN_QUERYCANCEL:
		lResult = OnPSN_QueryCancel(hwnd, idCtl, (LPPSHNOTIFY)pnmh);
		break;
	case PSN_RESET:
		lResult = OnPSN_Reset(hwnd, idCtl, (LPPSHNOTIFY)pnmh);
		break;
	case PSN_SETACTIVE:
		lResult = OnPSN_SetActive(hwnd, idCtl, (LPPSHNOTIFY)pnmh);
		break;
	case PSN_WIZBACK:
		lResult = OnPSN_WizBack(hwnd, idCtl, (LPPSHNOTIFY)pnmh);
		break;
	case PSN_WIZNEXT:
		lResult = OnPSN_WizNext(hwnd, idCtl, (LPPSHNOTIFY)pnmh);
		break;
	case PSN_WIZFINISH:
		lResult = OnPSN_WizFinish(hwnd, idCtl, (LPPSHNOTIFY)pnmh);
		break;
	default:
		break;
	}
	return lResult;
}


LRESULT
WizardPage::OnPSN_SetActive(
							HWND hwnd, 
							INT idCtl, 
							LPPSHNOTIFY pnmh
							)
{
	// JMC: TODO: Maybe put this in the OnNotify Code so the overrided class does
	// not have to call this

	//
	// By default, each wizard page has a BACK and NEXT button.
	//
	DWORD dwFlags = 0;
	if(sm_WizPageOrder.GetPrevPage(m_dwPageId))
		dwFlags |= PSWIZB_BACK;
	
	if(sm_WizPageOrder.GetNextPage(m_dwPageId))
		dwFlags |= PSWIZB_NEXT;
	else
		dwFlags |= PSWIZB_FINISH;
	
	PropSheet_SetWizButtons(GetParent(hwnd), dwFlags);
	
	// Tell the wizard that it's ok to go to this page
	SetWindowLong(hwnd, DWL_MSGRESULT, 0);
	return TRUE;
}


LRESULT
WizardPage::OnPSM_QuerySiblings(
								HWND hwnd, 
								WPARAM wParam, 
								LPARAM lParam
								)
{
	return 0;
}

LRESULT
WizardPage::OnPSN_QueryCancel(
							   HWND hwnd, 
							   INT idCtl, 
							   LPPSHNOTIFY pnmh
							   )
{
	switch(MessageBox(hwnd, __TEXT("Do you want keep the changes you have made so far?\r\n\r\nChoose Yes to keep the changes.\r\nChoose No to undo any changes.\r\nChoose Cancel to return to the wizard."), __TEXT("Do you want keep the changes you have made so far?"), MB_YESNOCANCEL))
	{
	case IDYES:
		SetWindowLong(hwnd, DWL_MSGRESULT, 0);
		break;
	case IDNO:
		// Restore all settings to the original settings
		g_Options.ApplyOriginal();
		SetWindowLong(hwnd, DWL_MSGRESULT, 0);
		break;
	case IDCANCEL:
		SetWindowLong(hwnd, DWL_MSGRESULT, 1);
		break;;

	}
	return TRUE;
}
