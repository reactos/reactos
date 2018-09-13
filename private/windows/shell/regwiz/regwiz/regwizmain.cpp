/*********************************************************************
Registration Wizard
10/12/94 - Tracy Ferrier
(c) 1994-95 Microsoft Corporation
**********************************************************************/

#include <Windows.h>
#include <RegPage.h>

#include <tapi.h>
#include "RegWizMain.h"
#include "Resource.h"
#include "Dialogs.h"
#include "regutil.h"
#include <process.h>
#include "sysinv.h"
#include <stdio.h>
#include "cntryinf.h"
#include "mcm.h"
#include "rw_common.h"
#include "rwpost.h"
#include "string.h"
#include <rpcdce.h>
#pragma comment(lib, "rpcrt4.lib")

// MSID Definitions
#define cchMaxSzMSID    32
#define cbMaxSzMSID     (cchMaxSzMSID + 1)
RECT gRect;

// Private functions
void __cdecl LaunchProductSearchThread(void*);
void __cdecl LaunchSystemInventoryThread(void* lParam);
void __cdecl LaunchOemCheckThread(void *);

typedef void (FAR PASCAL *LPFNPRODUCTSEARCH) (void (*pfnReceiveProductName) (LPTSTR,LPTSTR) );
void ReceiveProductName(LPTSTR szProductName,LPTSTR szProductPathName);
void ExitWithImproperBinary(HINSTANCE hInstance,HWND hParent);
void ExitWithInputParamError(HINSTANCE hInstance,HWND hParent);
void ExitWithTxferError(HINSTANCE hInstance,HWND hParent);
void ExitWithSuccessfulRegistration(HINSTANCE hInstance, LPTSTR szProductName);
void ExitWithModemError(HINSTANCE hInstance,HWND hParent);
void ExitWithModemCfgError(HINSTANCE hInstance,HWND hParent);
void ExitWithTcpCfgError(HINSTANCE hInstance,HWND hParent);
void ExitWithCompletedStatus(HINSTANCE hInstance,LPTSTR szProductName);
void ExitWithTryLater(HINSTANCE hInstance, HWND hParent);
void ExitWithConfigurationProblem(HINSTANCE hInstance, HWND hParent);
void ExitWithAnotherCopyRunning(HINSTANCE hInstance, HWND hParent);

BOOL ParseCmdLine(LPTSTR szCmdLine, LPTSTR szBuf, UINT cbBufMax,int *pSwitchType);
BOOL GetSignupLocation(HINSTANCE hInstance, LPTSTR szFileName,LPTSTR szDirectory);
void CopyCharToBuf ( _TCHAR** pszCur, _TCHAR** pszBuf, UINT* pcbBuf );

void RegWizStartupError(DWORD dwError, HINSTANCE hInstance, TCHAR *pszProductName=NULL);
int CheckOEMdll();
int OemPutDataInRegistry();

int CheckWin95OrNT();
int CheckIfProductIsRegistred(HINSTANCE hInstance ,
							  _TCHAR * szParamRegKey);
HBITMAP GetOemBmp();
HBITMAP BitmapFromDib (LPVOID pDIB, HPALETTE hpal,WORD wPalSize);

void GetWindowsDirectory(TCHAR *szParamRegKey,
						 TCHAR *czBuf);
/*****************************/
VOID	ReduceUUID(PSTR szUUID);
HRESULT HrTestHWID();
HRESULT GetNewGUID(PSTR pszGUID);
BOOL	CheckHWIDPresent();
void	MakeHWIDNotUsed(HINSTANCE hins);
/*****************************/
#ifdef _DEBUG
void DebugMessageBox(LPTSTR szMessage);
#else
#define DebugMessageBox(szMessage) 0
#endif

void CloseForTcpIcmp(); // TCP.CPP

#define chSpace 32
BOOL bOemDllLoaded = FALSE;
HINSTANCE hOemDll= NULL;
HANDLE hOemEvent =NULL;
HANDLE hProductEvent=NULL;
HANDLE hInventoryEvent=NULL;

_TCHAR szProductName[256];
_TCHAR szWindowsCaption[256];
_TCHAR szOEMIncenMsg[256];

static CRegWizard* vclRegWizard = NULL;
static LPTSTR lpszRegWizardClass = _T("RegWizard");

static HPALETTE gPal;

BOOL vDialogInitialized = FALSE;

//
//
//
#define SWITCH_WITH_UNKNOWNOPTION    0
#define SWITCH_WITH_I				 1
#define SWITCH_WITH_T				 2
#define SWITCH_WITH_R				 3



#define OEM_NO_ERROR		0
#define OEM_VALIDATE_FAILED 1
#define OEM_INTERNAL_ERROR  2

#define  WIN98_OS     1
#define  WINNT_OS     2
#define  UNKNOWN_OS   0

//
// returns 1 if Win95
//         2 if Win NT
//         0 if Error
int CheckWin95OrNT()
{
	int iRet = 0;
	OSVERSIONINFO  oi;
	oi.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
   	GetVersionEx(&oi);
	switch(oi.dwPlatformId) {
		case VER_PLATFORM_WIN32_NT:
			iRet = 2;
			break;
		case VER_PLATFORM_WIN32_WINDOWS:
			iRet = 1;
			break;
		default :
			break;
	}
	RW_DEBUG << "\n Check OS "  << iRet << flush;
	return iRet;
		


}


//
// Checks to make sure (8 RegWIz Binary is not used in NT
// and NT Binary is not being used in Windows 98
// returns :
//		NO_ERROR   if Binary is the one created for the OS it is running
//      RWZ_ERROR_INVALID_DLL : Dll is not intended for this OS
DWORD CheckForValidRegWizBinary()
{
	int iOsType;
	int iError;
	iError = NO_ERROR;
	iOsType = CheckWin95OrNT();
	#ifdef _WIN95
		if (iOsType != WIN98_OS )
		{
			// Error the OS is not Win 95
			iError = RWZ_ERROR_INVALID_DLL ;
			
		}
	#else
		if (iOsType != WINNT_OS )
		{
			// Error the OS is not Win NT
			iError = RWZ_ERROR_INVALID_DLL;
			
		}
	#endif
	return iError;


}

//int PASCAL _tWinMain( HANDLE hInstance, HANDLE hPrevInstance, LPTSTR lpszCmdParam, int nCmdShow)
// Returns 0 if invoked by /i
// Option : /t
// Returns	 0    if Not Registred
// Returns	 1    if Product is Registred
// Returns  -1  if errror in parameter /t option

int InvokeRegistration ( HINSTANCE hInstance  , LPCTSTR czPath)
/*********************************************************************
Main entry point for the Registration Wizard.
**********************************************************************/
{
	_TCHAR szParamRegKey[256];
	BOOL goodRegKey;
	HANDLE hMutexForInstanceCheck;
	TriState productSearchLibStatus;
	_TCHAR szTitle[64];
	_TCHAR szValue[256]; // used for MSID

	int iError;
	int iSwitchType;


	iSwitchType = SWITCH_WITH_UNKNOWNOPTION;
	hMutexForInstanceCheck = NULL;
	iError = NO_ERROR;

	//
	// Create a Mutex object to check for another copy of regWiz running
	//

	hMutexForInstanceCheck = CreateMutex(NULL,TRUE,_T("RegWizResource_1298345_ForRegistration"));
	if(hMutexForInstanceCheck != NULL ) {
		if( GetLastError() ==  ERROR_ALREADY_EXISTS){
			//
			// Already an instance of RegWiz is running
			iError = RWZ_ERROR_PREVIOUSCOPY_FOUND;
			goto StartupError_NOResourceCleanup;
		}
	}

	//
	//
	// Init variables for Font creation
	InitDlgNormalBoldFont();


	//
	// Check for Binary Validity
	if((iError=CheckForValidRegWizBinary()) ==
		RWZ_ERROR_INVALID_DLL)				{
			goto StartupError_NOResourceCleanup;
	}

	goodRegKey = ParseCmdLine((LPTSTR) czPath,szParamRegKey,256,
		&iSwitchType);

	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n After ParseCmdLine"  << flush;
	#endif
	if (!goodRegKey){
		// This is a special case for browser
		if(iSwitchType == SWITCH_WITH_T){
			return  -1;
		}
		iError = RWZ_ERROR_INVALID_PARAMETER;
		goto StartupError_NOResourceCleanup;
	}

	//
	//  This is a special case for Registration Wizard to be launched from Browsers.
	//  This is to check if the product is already registred
	//  This returns 1 if registred and 0 if it is not registred
	if(iSwitchType == SWITCH_WITH_T){
		return CheckIfProductIsRegistred(hInstance, szParamRegKey);
		
	}

	


	// 02/06/98 If REGWIZ.EXE is invoke with /r and the product info is not passed
	// then it has to regregister the OS
	// so if the product info is null it fills the proper OS reg details
	if(iSwitchType == SWITCH_WITH_R) {
		if(szParamRegKey[0] == NULL || szParamRegKey[0] == _T('\0')){
			// LOad the OS String
			switch(CheckWin95OrNT()) {
			case WIN98_OS:
				LoadString(hInstance,
					IDS_REREGISTER_OS1,
					szParamRegKey,
					256);
					
				break;
			case WINNT_OS :
				LoadString(hInstance,
					IDS_REREGISTER_OS2,
					szParamRegKey,
					256);
					//_tcscpy(szParamRegKey,_T("SOFTWARE\\Microsoft\\NT5.0"));
				break;
			default:
				break;
			}
		
		}				
		

	}

	RW_DEBUG << "\n Prod Key [" << szParamRegKey <<"]" <<  flush;
	vclRegWizard = new CRegWizard(hInstance,szParamRegKey);

	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n After new CRegWizard"  << flush;
	#endif

	if (vclRegWizard->GetInputParameterStatus() == FALSE){
		#ifdef _DEBUG
		DebugMessageBox(_T("One or more 'input parameter' registration keys are invalid."));
		#endif
		iError = RWZ_ERROR_INVALID_PARAMETER;
		goto StartupError_CloseRegWizard;
	}

	// Set the product name  globally so it can be accessed
	vclRegWizard->GetInputParameterString(IDS_INPUT_PRODUCTNAME,szProductName);
	SetProductBeingRegistred(szProductName);
	vclRegWizard->SetWindowCaption(szProductName);
	

	LoadString(hInstance,IDS_WINDOWS_CAPTION,szTitle,64);
	_tcscpy(szWindowsCaption,szProductName);
	_tcscat(szWindowsCaption,szTitle);

	// If registration has already been done for this product
	// (as specified in the input parameters), we can just
	// inform the user and then exit.
	if(iSwitchType != SWITCH_WITH_R){
	
		if (vclRegWizard->IsRegistered()){
			vclRegWizard->GetInputParameterString(IDS_INPUT_PRODUCTNAME,szProductName);
			iError = REGWIZ_ALREADY_CONFIGURED ;
			goto StartupError_CloseRegWizard;
		}
	}
	// If our CRegWizard object can't locate the product search
	// DLL (complinc.dll), we can't go on.
	productSearchLibStatus = vclRegWizard->GetProductSearchLibraryStatus();
	if (productSearchLibStatus == kTriStateFalse){
		#ifdef _DEBUG
			DebugMessageBox(_T("The Product Inventory DLL specified by the 'input parameter' registration key could")
			_T(" not be found, or has an invalid format."));
		#endif
		iError = RWZ_ERROR_INVALID_PARAMETER;
		goto StartupError_CloseRegWizard;
	}

	//
	//  Resource Allocation from now on has to be freed
	//  during Exit


	// Get Auto Dial Status
	GetAutoDialConfiguration();
	#ifdef _LOG_IN_FILE
		RW_DEBUG <<"\n After GetAutoDialConfiguration"  << flush;
	#endif
	InitializeInetThread(hInstance);

	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n After InitializeInetThread"  << flush;
	#endif

	//
	// Product Inventory Search in the background	
	hProductEvent = CreateEvent( NULL, TRUE, FALSE,NULL);
	hInventoryEvent = CreateEvent( NULL, TRUE, FALSE,NULL);
	if (productSearchLibStatus == kTriStateTrue){
		_beginthread(LaunchProductSearchThread,0,vclRegWizard);
	}
	else{
		vclRegWizard->SetProductSearchStatus(kTriStateTrue);
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\n Bypassed LaunchProductSearchThread"  << flush;
		#endif
		SetEvent(hProductEvent);
	}
	
	_beginthread(LaunchSystemInventoryThread,0,vclRegWizard);

	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n After LaunchSystemInventoryThread"  << flush;
	#endif
	// 03/11/99 Delete HWID Logic , Delete Existing HWID
	//if(CheckHWIDPresent() == FALSE)
	//	HrTestHWID();
	MakeHWIDNotUsed(hInstance);


	iError = (ULONG)DoRegistrationWizard(hInstance, vclRegWizard, szParamRegKey);

	#ifdef _LOG_IN_FILE
		RW_DEBUG <<"\n iError :  " << iError << flush;
	#endif

		

	if(hInventoryEvent)
	WaitForSingleObject(hInventoryEvent,INFINITE);
	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n System Inventory WAIT FINISHED" << flush;
	#endif
	if(hProductEvent)
	WaitForSingleObject(hProductEvent,INFINITE);
	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n Product Search WAIT FINISHED"<< flush;
	#endif
	
		

	if(hProductEvent)	CloseHandle(hProductEvent);
	if(hInventoryEvent) CloseHandle(hInventoryEvent);
	


	// Remove The MSID Entry
	RemoveMSIDEntry(hInstance);
	MakeHWIDNotUsed(hInstance);

	// Close any opened windows
	if(vclRegWizard)
	vclRegWizard->DestroyOpenedWindow();

	ResetAutoDialConfiguration();
	
	UnLoadInetCfgLib(); // Frees  INETCFG.DLL
	// comment the above line as it is creating a problem IE
	CloseForTcpIcmp(); //  Frees   ICMP.DLL and Closes Socket

	if(vclRegWizard) delete vclRegWizard;

	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n Exiting Regwiz....." << flush;
	#endif
	if(hMutexForInstanceCheck) {
		CloseHandle(hMutexForInstanceCheck);
	}

	goto CoolExit;

StartupError_CloseRegWizard:
	//
	// Close the
	RegWizStartupError(iError,hInstance,szProductName );
	if(hMutexForInstanceCheck) {
		CloseHandle(hMutexForInstanceCheck);
	}
	if(vclRegWizard) delete vclRegWizard;
	goto CoolExit;

StartupError_NOResourceCleanup:
	RegWizStartupError(iError,hInstance,szProductName );	
	if(hMutexForInstanceCheck) {
		CloseHandle(hMutexForInstanceCheck);
	}
		

CoolExit:
	//
	// Call function to destroy for the fonts that are created
	DeleteDlgNormalBoldFont();

	return iError;

}

void RegWizStartupError(DWORD dwError, HINSTANCE hInstance, TCHAR  *pszProductName)
{
	RW_DEBUG << "\n In RegWizStartupError " << dwError << flush;
	switch(dwError) {
		case  RWZ_ERROR_INVALID_DLL:
			ExitWithImproperBinary(hInstance, NULL);
			break;
		case  RWZ_ERROR_PREVIOUSCOPY_FOUND:
			ExitWithAnotherCopyRunning(hInstance, NULL);
			break;
		case  RWZ_ERROR_INVALID_PARAMETER:
			ExitWithInputParamError(hInstance,NULL);
			break;
		case  REGWIZ_ALREADY_CONFIGURED:
			ExitWithCompletedStatus(hInstance,pszProductName);
			break;

			

		default:
			break;
			//
			//
		break;

	}

}


/*   T R A N S M I T   R E G   W I Z   I N F O   */
/*-------------------------------------------------------------------------
    Owner: SteveBu

    REVIEW: add support fOEM, tricking RegWiz into posting both prodreg
    and OEM information.
-------------------------------------------------------------------------*/
void TransmitRegWizInfo(HINSTANCE hInstance,
						LPCTSTR szParams, BOOL fOEM)
	
{	
	
		
}







ModemStatus DetectModem(HINSTANCE hInstance)
/*********************************************************************
This function detects whether the user's machine has a modem connected
and properly configured.

Returns:
kNoneFound: No modem is connected.
kModemFound: A modem is connected and configured.
kConfigErr: A modem is connected, but it could not be configured
	correctly.
**********************************************************************/
{
	#ifdef _TAPI
		MODEMSTATUS msModemStatus = MSEnsureModemTAPI(hInstance,NULL);
		switch (msModemStatus)
		{
			case kMsModemOk:
				return kModemFound;
			case kMsModemNotFound:
				return kNoneFound;
			case kMsModemTooSlow:
				return kModemTooSlow;
			default:
				return kNoneFound;
		}
	#else
		if (vclRegWizard->GetInformationString(kInfoCountry,NULL) == NULL)
		{
			vclRegWizard->SetInformationString(kInfoCountry,_T("United States of America"));
		}
		return kModemFound;
	#endif
}



void __cdecl LaunchProductSearchThread(void* lParam)
/*********************************************************************
This function spawns a thread that builds an inventory list of the
products installed on the user's system.  The lParam parameter should
contain a pointer to our CRegWizard object.
**********************************************************************/
{
	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n LaunchProductSearchThread started" << flush;
	#endif
	
	CRegWizard* pclRegWizard = (CRegWizard*) lParam;
	pclRegWizard->SetProductSearchStatus(kTriStateFalse);

	LPFNPRODUCTSEARCH lpfnProdSearch;
	pclRegWizard->GetProductSearchProcAddress((FARPROC*) &lpfnProdSearch);
	if (lpfnProdSearch)
	{
		lpfnProdSearch(ReceiveProductName);
		pclRegWizard->SetProductSearchStatus(kTriStateTrue);
	}
	else
	{
		pclRegWizard->SetProductSearchStatus(kTriStateUndefined);
	}
	
	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n LaunchProductSearchThread finished" << flush;
	#endif

	SetEvent(hProductEvent);
	_endthread();
	
}

void __cdecl LaunchSystemInventoryThread(void* lParam)
/*********************************************************************
This function spawns a thread that builds an inventory list of
detected system information .  The lParam parameter should contain a
pointer to our CRegWizard object.
**********************************************************************/
{
	CRegWizard* pclRegWizard = (CRegWizard*) lParam;
	pclRegWizard->SetSystemInventoryStatus(FALSE);

	const int iInvBufSize = 256;
	_TCHAR szInventory[iInvBufSize];
	GetProcessorTypeString(pclRegWizard->GetInstance(),szInventory);
	pclRegWizard->SetInformationString(kInfoProcessor,szInventory);

	GetTotalMemoryString(pclRegWizard->GetInstance(),szInventory);
	pclRegWizard->SetInformationString(kInfoTotalRAM,szInventory);

	GetTotalHardDiskSpaceString(pclRegWizard->GetInstance(),szInventory);
	pclRegWizard->SetInformationString(kInfoTotalDiskSpace,szInventory);

	GetDisplayResolutionString(pclRegWizard->GetInstance(),szInventory);
	pclRegWizard->SetInformationString(kInfoDisplayResolution,szInventory);
	
	GetDisplayColorDepthString(pclRegWizard->GetInstance(),szInventory);
	pclRegWizard->SetInformationString(kInfoDisplayColorDepth,szInventory);

	GetWindowsVersionString(pclRegWizard->GetInstance(), szInventory);
	pclRegWizard->SetInformationString(kInfoOperatingSystem,szInventory);

	GetNetworkCardString(pclRegWizard->GetInstance(),szInventory);
	pclRegWizard->SetInformationString(kInfoNetwork,szInventory);

	GetModemString(pclRegWizard->GetInstance(),szInventory);
	pclRegWizard->SetInformationString(kInfoModem,szInventory);

	GetPointingDeviceString(pclRegWizard->GetInstance(),szInventory);
	pclRegWizard->SetInformationString(kInfoPointingDevice,szInventory);

	GetCDRomString(pclRegWizard->GetInstance(),szInventory);
	pclRegWizard->SetInformationString(kInfoCDROM,szInventory);

	GetSoundCardString(pclRegWizard->GetInstance(),szInventory);
	pclRegWizard->SetInformationString(kInfoSoundCard,szInventory);

	GetRemoveableMediaString(pclRegWizard->GetInstance(),szInventory,iInvBufSize);
	pclRegWizard->SetInformationString(kInfoRemoveableMedia,szInventory);

	GetOEMString(pclRegWizard->GetInstance(),szInventory);
	pclRegWizard->SetInformationString(kComputerManufacturer,szInventory);

	GetSCSIAdapterString(pclRegWizard->GetInstance(),szInventory);
	pclRegWizard->SetInformationString(kScsiAdapterInfo,szInventory);


	BOOL hasCoProcessor = IsCoProcessorAvailable(pclRegWizard->GetInstance());
	pclRegWizard->SetTriStateInformation(kInfoMathCoProcessor,
	hasCoProcessor == TRUE ? kTriStateTrue : kTriStateFalse);

	pclRegWizard->SetSystemInventoryStatus(TRUE);

	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n LaunchSystemInventoryThread finished" << flush;
	#endif
	SetEvent(hInventoryEvent);
	_endthread();
}



void ReceiveProductName(LPTSTR szProductName,LPTSTR szProductPathName)
/***************************************************************************
This function receives the given product name back from complinc.dll
****************************************************************************/
{
	vclRegWizard->AddProduct(szProductName,szProductPathName);
	RefreshInventoryList(vclRegWizard);
}


BOOL ParseCmdLine(LPTSTR szCmdLine, LPTSTR szBuf, UINT cbBufMax, int *pSwitchType)
/***************************************************************************
This function expects in the szCmdLine parameter a pointer to the
command line parameter string used when RegWiz was launched.  This string
will be parsed, and the bare command line argument (which should be a
RegDB key referencing our parameter block) will be returned in the
szBuf parameter.

If no valid command line parameter is located, an empty string will be
returned in szBuf, and FALSE will be returned as the function result.
****************************************************************************/
{

	TCHAR  cSwitch;
	*pSwitchType = SWITCH_WITH_UNKNOWNOPTION;
	if (szCmdLine == NULL || szCmdLine[0] == 0)
	{
		return FALSE;
	}
	else
	{
		LPTSTR szCurr = szCmdLine;
		while (*szCurr == chSpace)
		{
			szCurr = CharNext(szCurr);
		}
	

		if (*szCurr != _T('/') && *szCurr != _T('-')) return FALSE;
		szCurr = CharNext(szCurr);

		cSwitch = *szCurr;
		cSwitch = _totupper(cSwitch);

		if (cSwitch != _T('I') && cSwitch != _T('T') && cSwitch != _T('R') )return FALSE;

		if(cSwitch ==_T('T'))
		{
			*pSwitchType = SWITCH_WITH_T;
		}
		else
		if(cSwitch ==_T('I'))
		{
			*pSwitchType =SWITCH_WITH_I;
		}
		else
		if(cSwitch ==_T('R'))
		{
			*pSwitchType =SWITCH_WITH_R;
		}
		else
		{
			*pSwitchType =SWITCH_WITH_UNKNOWNOPTION;
		}

		szCurr = CharNext(szCurr);
		while (*szCurr == chSpace)
		{
			szCurr = CharNext(szCurr);
		}

		BOOL fInQuoted = FALSE;
		while (*szCurr != NULL && (fInQuoted == TRUE || fInQuoted == FALSE && *szCurr != chSpace))
		{
			if (*szCurr == _T('"'))
			{
				szCurr = CharNext(szCurr);
				fInQuoted = fInQuoted == TRUE ? FALSE : TRUE;
			}
			else
			{
				CopyCharToBuf(&szCurr,&szBuf,&cbBufMax);
			}
		}
		*szBuf = NULL;
		while (*szCurr == chSpace)
		{
			szCurr = CharNext(szCurr);
		}
		if (*szCurr != NULL) return FALSE;

		#ifdef _DEBUG
			if (fInQuoted)
			{
				DebugMessageBox(_T("Unmatched quotes in '/i' command line parameter."));
			}
		#endif
		return fInQuoted == TRUE ? FALSE : TRUE;
	}
}


void CopyCharToBuf ( LPTSTR* pszCur, LPTSTR* pszBuf, UINT* pcbBuf )
{
	LPTSTR szEnd = CharNext(*pszCur);
	if (UINT(szEnd - *pszCur) <= *pcbBuf)
		{
		while (*pszCur < szEnd)
			{
				//*(*pszBuf)++ = *(*pszCur)++;
				_tcscpy(*pszBuf,*pszCur);
				(*pszBuf) = _tcsinc((*pszBuf));
				(*pszCur) = _tcsinc((*pszCur));
				//(*pcbBuf) = _tcsdec((*pcbBuf));
				(*pcbBuf)--;
			}
		}
	else
		{
		*pszCur = szEnd;
		*pcbBuf = 0;
		}
}



void ExitWithInputParamError(HINSTANCE hInstance,HWND hParent)
/***************************************************************************
Displays a message informing the user of a problem with the input parameters
(either the command line argument regkey, or the parameter subkeys in the
Reg Database), and then terminates the RegWizard.
****************************************************************************/
{
	RegWizardMessage(hInstance,hParent,IDD_INPUTPARAM_ERR);
	#ifdef _REGWIZ_EXE
		exit(EXIT_FAILURE);
	#endif
}


void 	ExitWithTxferError(HINSTANCE hInstance,HWND hParent)
{
	RegWizardMessage(hInstance,hParent,IDD_POST_ERROR);
	#ifdef _REGWIZ_EXE
		exit(EXIT_FAILURE);
	#endif
}

void 	ExitWithTryLater(HINSTANCE hInstance,HWND hParent)
{
	RegWizardMessage(hInstance,hParent,IDD_POST_PROBLEM);
	#ifdef _REGWIZ_EXE
		exit(EXIT_FAILURE);
	#endif
}

void ExitWithConfigurationProblem(HINSTANCE hInstance, HWND hParent)
{
	RegWizardMessage(hInstance,hParent,IDD_CFG_PROBLEM);
	#ifdef _REGWIZ_EXE
		exit(EXIT_FAILURE);
	#endif

}

void ExitWithModemError(HINSTANCE hInstance,HWND hParent)
/***************************************************************************
Displays a message informing the user of that RegWizard cannot run without
a properly configured modem, and then terminates.
****************************************************************************/
{

	RegWizardMessageEx(hInstance,hParent,IDD_MODEM_ERR, GetProductBeingRegistred());
	#ifdef _REGWIZ_EXE
		exit(EXIT_SUCCESS);
	#endif
}

void ExitWithTcpCfgError(HINSTANCE hInstance,HWND hParent)
{
	RegWizardMessage(hInstance,hParent,IDD_NETWORK_CFG_ERROR);
	#ifdef _REGWIZ_EXE
		exit(EXIT_SUCCESS);
	#endif
}

void ExitWithImproperBinary(HINSTANCE hInstance,HWND hParent)
{
	RegWizardMessage(hInstance,hParent,IDD_ERROR_INVALIDBINARY);
	#ifdef _REGWIZ_EXE
		exit(EXIT_SUCCESS);
	#endif

}

void ExitWithModemCfgError(HINSTANCE hInstance,HWND hParent)
{
	RegWizardMessage(hInstance,hParent,IDD_MODEM_CFG_ERROR);
	#ifdef _REGWIZ_EXE
		exit(EXIT_SUCCESS);
	#endif
}

void ExitWithAnotherCopyRunning(HINSTANCE hInstance, HWND hParent)
{
	RegWizardMessage(hInstance,hParent,IDD_ANOTHERCOPY_ERROR);
	#ifdef _REGWIZ_EXE
		exit(EXIT_SUCCESS);
	#endif
}
void ExitWithSuccessfulRegistration(HINSTANCE hInstance, LPTSTR szProductName)
{
	RegWizardMessageEx(hInstance,NULL,IDD_SUCCESSFUL_REGISTRATION,szProductName);
	#ifdef _REGWIZ_EXE
		exit(EXIT_SUCCESS);
	#endif
}


void ExitWithCompletedStatus(HINSTANCE hInstance,LPTSTR szProductName)
/***************************************************************************
Displays a message informing the user of registration has already been
performed for this product, and then exits.  The szProductName parameter
should contain a pointer to the product name.
****************************************************************************/
{
	RegWizardMessageEx(hInstance,NULL,IDD_ALREADY_REGISTERED,szProductName);
	#ifdef _REGWIZ_EXE
		exit(EXIT_FAILURE);
	#endif
}



BOOL GetSignupLocation(HINSTANCE hInstance, LPTSTR szFileName,LPTSTR szDirectory)
/***************************************************************************
Returns the filename and full pathname to the directory of the SignUp
executable.  If either value cannot be determined (i.e. the pathname cannot
be found in the Registration Database), FALSE will be returned as the
function result.
****************************************************************************/
{
	HKEY hKey;
	_TCHAR szKeyName[256];
	BOOL retValue = FALSE;
	int resSize = LoadString(hInstance,IDS_SIGNUPLOC_KEY,szKeyName,255);
	LONG regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKeyName,0,KEY_READ,&hKey);
	if (regStatus == ERROR_SUCCESS)
	{
		_TCHAR szValueName[64];
		unsigned long infoSize = 255;
		resSize = LoadString(hInstance,IDS_SIGNUPLOC_VALUENAME,szValueName,64);
		regStatus = RegQueryValueEx(hKey,szValueName,NULL,0,(LPBYTE) szDirectory,&infoSize);
		if (regStatus == ERROR_SUCCESS)
		{
			retValue = TRUE;
			LoadString(hInstance,IDS_SIGNUPLOC_FILENAME,szFileName,255);
		}
		RegCloseKey(hKey);
	}
	return retValue;
}


int CheckOEMdll()
{
	return OEM_INTERNAL_ERROR;
}



#ifdef _DEBUG
void DebugMessageBox(LPTSTR szMessage)
/***************************************************************************
Displays the given message in a "Stop" message box (debug builds only).
****************************************************************************/
{
	MessageBox(NULL,szMessage,_T("Registration Wizard Error"),MB_ICONSTOP);
}
#endif



/*************************************
/*   R E D U C E   U   U   I   D   */
/*----------------------------------------------------------------------
    Owner: SteveBu
    Reduces a UUID to a string
----------------------------------------------------------------------*/
VOID ReduceUUID(PSTR szUUID)
{
        int i;
        for (i=9; i<13; i++) szUUID[i-1]=szUUID[i];
        for (i=14; i<18; i++) szUUID[i-2]=szUUID[i];
        for (i=19; i<23; i++) szUUID[i-3]=szUUID[i];
        for (i=24; i<36; i++) szUUID[i-4]=szUUID[i];
        szUUID[32]='\0';
}

/*   G E T   N E W   G   U   I   D   */
/*----------------------------------------------------------------------
    Owner: SteveBu
    Generates a new GUID.  Assumes passed in pszGUID is 32 characters in
    length.
	03/10/99:
	NUll String will be returned, GUID will no longer be created on 
	clients machine
-----------------------------------------------------------------------*/
HRESULT GetNewGUID(PSTR pszGUID)
{
        UUID    idNumber;
        PBYTE  uuidString;
        DWORD   dwErr;
		char szUidString[40];

		strcpy(pszGUID,"");
		return NO_ERROR;
		/******
        if ( (RPC_S_OK==(dwErr=UuidCreate(&idNumber))) &&
                 (RPC_S_OK==(dwErr=UuidToStringA(&idNumber,&uuidString))) )
        {
    		RW_DEBUG << "\n: Create New HWID :" << uuidString << flush;
                strcpy(szUidString, (PSTR)uuidString);
                RpcStringFreeA(&uuidString);
                ReduceUUID(szUidString);
				strcpy(pszGUID,szUidString);
                return NO_ERROR;
        }
        return HRESULT_FROM_WIN32(dwErr);
		**/
}

/**
BOOL CheckHWIDPresent()
{
	HKEY hKeyHWID;

	_TCHAR szKeyName[256];
	_TCHAR szValue[256];
	
	_tcscpy(szKeyName,_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"));
	
	LONG regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKeyName,0,KEY_READ,&hKeyHWID);
	if (regStatus == ERROR_SUCCESS)
	{
		_TCHAR szValueName[64];
		unsigned long infoSize = 255;
		LoadString(vclRegWizard->GetInstance(),IDS_HWID,szValueName,64);
		regStatus = RegQueryValueEx(hKeyHWID,szValueName,NULL,0,(LPBYTE) szValue,&infoSize);
		if (regStatus == ERROR_SUCCESS)
		{
			vclRegWizard->SetInformationString(kInfoHWID,szValue);
			RegCloseKey(hKeyHWID);
			return TRUE;
		}

	}
	RegCloseKey(hKeyHWID);
	return FALSE;
}

HRESULT HrTestHWID()
{
 DWORD	dwRet;
 _TCHAR szKeyName[256];
 HRESULT hr;
 HKEY	hIDKey;
 char szHWID[cbMaxSzMSID+64];
 TCHAR  *TP;

 _tcscpy(szKeyName,_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"));

 if (FAILED(hr = GetNewGUID(szHWID)))
	  szHWID[0] = '\0';
 else
 {
    RW_DEBUG << "\n:HWID :" << szHWID << flush;
  // Store HWID into
	dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKeyName,0,KEY_ALL_ACCESS,&hIDKey);
	if (dwRet == ERROR_SUCCESS)
	{
		TP = ConvertToUnicode(szHWID);
		dwRet = RegSetValueEx(hIDKey,_T("HWID"),NULL,REG_SZ,(CONST BYTE *)TP,
									_tcslen( TP) * sizeof(TCHAR) );
		vclRegWizard->SetInformationString(kInfoHWID,TP);
		RegCloseKey(hIDKey);
	}
 }

 return hr;
}
****/

void MakeHWIDNotUsed(HINSTANCE  hInstance)
{
	_TCHAR szKeyName[256];
	_TCHAR szValue[256];
	HRESULT hr;
	HKEY	hIDKey;
	DWORD dwRet;
	szValue[0] = _T('\0');
	LoadString(hInstance,IDS_NOTUSED,szValue,255);

	_tcscpy(szKeyName,_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"));
	dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKeyName,0,KEY_ALL_ACCESS,&hIDKey);
	if (dwRet == ERROR_SUCCESS) {
		//RegDeleteValue(hIDKey,_T("HWID") );
		RegSetValueEx(hIDKey,_T("HWID"),NULL,REG_SZ,(CONST BYTE *)szValue,
								_tcslen((LPCTSTR)szValue)* sizeof(_TCHAR) );
		RegCloseKey(hIDKey);
	}
	// Delete From User Informatipon
	_tcscpy(szKeyName,_T("SOFTWARE\\Microsoft\\User Information"));
	dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKeyName,0,KEY_ALL_ACCESS,&hIDKey);
	if (dwRet == ERROR_SUCCESS) {
		//RegDeleteValue(hIDKey,_T("HWID") );
		RegSetValueEx(hIDKey,_T("HWID"),NULL,REG_SZ,(CONST BYTE *)szValue,
								_tcslen((LPCTSTR)szValue) * sizeof(_TCHAR));
		RegCloseKey(hIDKey);
	}

}

//
// Return 0 : if not registred
//        1 : if Registred
//
int CheckIfProductIsRegistred(HINSTANCE hInstance ,
							  _TCHAR * szParamRegKey)
{
	int iReturn ;
	HKEY hKey;
	TCHAR szValueName[256] = _T("");
	TCHAR szRetVal[48];
	DWORD dwSize= 48;
	LONG regStatus ;
	int resSize = LoadString(hInstance,
				IDS_INPUT_ISREGISTERED,szValueName,128);

	iReturn  = 1;

	GetProductRoot(szParamRegKey , &hKey);
	if(!hKey) {
		return iReturn;
	}
	regStatus = RegQueryValueEx(hKey,
		szValueName,
		NULL,
		0,
		(LPBYTE) szRetVal,
		&dwSize);
	if (regStatus == ERROR_SUCCESS){
		// Verifty the Value
		//
		if(szRetVal[0] == _T('1')) {
			iReturn = 0; // Product Registred flag is set
		}
	}
	RegCloseKey(hKey);
	
	return iReturn;

}



HBITMAP GetOemBmp()
{
       HDC  hDC;
       BOOL bRet;

       // detect this display is 256 colors or not
       hDC = GetDC(NULL);
       bRet = (GetDeviceCaps(hDC, BITSPIXEL) != 8);
       ReleaseDC(NULL, hDC);
       if (bRet)
	   {
		   // the display is not 256 colors, let Windows handle it
          return LoadBitmap(vclRegWizard->GetInstance(),MAKEINTRESOURCE(IDB_BITMAP1));
       }


       LPBITMAPINFO lpBmpInfo;               // bitmap informaiton
       int i;
       HRSRC hRsrc;
	   HANDLE hDib;
	   HBITMAP hBMP;

       struct
	   {
			   WORD            palVersion;
		       WORD            palNumEntries;
			   PALETTEENTRY    PalEntry[256];
	   } MyPal;

       hRsrc = FindResource(vclRegWizard->GetInstance(), MAKEINTRESOURCE(IDB_BITMAP1),RT_BITMAP);
       if (!hRsrc)
         return NULL;

       hDib = LoadResource(vclRegWizard->GetInstance(), hRsrc);
       if (!hDib)
         return NULL;

       if (!(lpBmpInfo = (LPBITMAPINFO) LockResource(hDib)))
               return NULL;

       MyPal.palVersion = 0x300;
       MyPal.palNumEntries = 1 << lpBmpInfo->bmiHeader.biBitCount;

       for (i = 0; i < MyPal.palNumEntries; i++)
	   {
         MyPal.PalEntry[i].peRed   = lpBmpInfo->bmiColors[i].rgbRed;
         MyPal.PalEntry[i].peGreen = lpBmpInfo->bmiColors[i].rgbGreen;
         MyPal.PalEntry[i].peBlue  = lpBmpInfo->bmiColors[i].rgbBlue;
         MyPal.PalEntry[i].peFlags = 0;
       }
       gPal = CreatePalette((LPLOGPALETTE)&MyPal);

       if (gPal == NULL)
	   {        // create palette fail, let window handle the bitmap
          return LoadBitmap(vclRegWizard->GetInstance(),MAKEINTRESOURCE(IDB_BITMAP1));
       }

       hBMP = BitmapFromDib(hDib,gPal,MyPal.palNumEntries);
       UnlockResource(hDib);
	   if( hBMP == NULL ) {
		   DeleteObject(gPal);
		   gPal = NULL;
		   hBMP = LoadBitmap(vclRegWizard->GetInstance(),MAKEINTRESOURCE(IDB_BITMAP1));
       }
	   return hBMP;
}



/****************************************************************************
 *                                                                          *
 *  FUNCTION   : BitmapFromDib(LPVOID hdib, HPALETTE hpal, WORD palSize)                  *
 *                                                                          *
 *  PURPOSE    : Will create a DDB (Device Dependent Bitmap) given a global *
 *               handle to a memory block in CF_DIB format                  *
 *                                                                          *
 *  RETURNS    : A handle to the DDB.                                       *
 *                                                                          *
 ****************************************************************************/

HBITMAP BitmapFromDib (
    LPVOID         pDIB,
    HPALETTE   hpal, WORD wPalSize)
{
    LPBITMAPINFOHEADER  lpbi;
    HPALETTE            hpalT;
    HDC                 hdc;
    HBITMAP             hbm;



    if (!pDIB || wPalSize == 16 )
        return NULL;

    lpbi = (LPBITMAPINFOHEADER)pDIB; // lock resource


    hdc = GetDC(NULL);

    if (hpal){
        hpalT = SelectPalette(hdc,hpal,FALSE);
        RealizePalette(hdc);
    }

    hbm = CreateDIBitmap(hdc,
                (LPBITMAPINFOHEADER)lpbi,
                (LONG)CBM_INIT,
                (LPSTR)lpbi + lpbi->biSize + wPalSize*sizeof(PALETTEENTRY),
                (LPBITMAPINFO)lpbi,
                DIB_RGB_COLORS );

    if (hpal)
        SelectPalette(hdc,hpalT,FALSE);

    ReleaseDC(NULL,hdc);

    return hbm;
}
