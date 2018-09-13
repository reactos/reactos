/*********************************************************************
Registration Wizard
Class: CRegWizard

--- This class is responsible for accumulating information gathered
from the user by the Registration Wizard, and then writing it to the
Registration Database in preparation for transmission via modem.

11/3/94 - Tracy Ferrier
(c) 1994-95 Microsoft Corporation
**********************************************************************/
#include <Windows.h>
#include <stdio.h>
#include "cregwiz.h"
#include "resource.h"
#include "version.h"
#include "regutil.h"
#include "cntryinf.h"
#include "rwwin95.h"
#include "cntryinf.h"
#include "wininet.h"
#include "rw_common.h"

#define kRegBufferSize	260

CRegWizard::CRegWizard(HINSTANCE hInstance, LPTSTR szParamRegKey)
/*********************************************************************
Constructor for our Registration Wizard class.  The szParamRegKey
parameter should be a Registration Database key pointing to a block
of Reg Wizard input parameters.
**********************************************************************/
{

	m_hInstance = hInstance;
	_tcscpy(m_szParamRegKey,szParamRegKey);
	m_szInfoParentKey[0] = NULL;
	m_productNameCount = 0;
	m_searchCompleted = kTriStateFalse;
	m_systemInventoryCompleted = FALSE;
	m_lpfnProductSearch = NULL;
	m_countryCode= 0;  // CXZ   5/8/97   from NULL to 0
	m_dialogActive = FALSE;
	m_hwndStartDialog = NULL;
	m_hwndCurrDialog = NULL;
	m_wDialogExitButton = 0;
	m_szLogFilePath[0] = NULL;
	m_hLogFile = INVALID_HANDLE_VALUE;
	m_hwndDialogToHide = NULL;
	m_hwndPrevDialog = NULL;
	m_ccpLibrary	 = NULL;

	for (short index = 0;index < kInfoLastIndex;index++)
	{
		m_rgszInfoArray[index] = NULL;
		m_writeEnable[index] = TRUE;
	}

	// Since we want to perform a new product search each time, we'll delete any
	// existing product name keys.
	index = 0;
	_TCHAR szParentKey[255];
	_TCHAR szProductBase[64];
	LONG regStatus;
	HKEY hKey;
	GetInfoRegistrationParentKey(szParentKey);
	int resSize = LoadString(m_hInstance,IDS_PRODUCTBASEKEY,szProductBase,64);
	
	regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szParentKey,NULL,KEY_ALL_ACCESS,&hKey);

	if (regStatus == ERROR_SUCCESS)
	{
		for (int x = 1;x <= kMaxProductCount;x++)
		{
			_TCHAR szProductValueName[256];
			_stprintf(szProductValueName,_T("%s %i"),szProductBase,x);
			regStatus = RegSetValueEx(hKey,szProductValueName,NULL,REG_SZ,(CONST BYTE*) _T(""),1);
		}
	}

	// Read any default information from the registry (if RegWizard
	// hasn't been run before, there will be no existing default info).
	ReadInfoFromRegistry();
	ResolveCurrentCountryCode();	

	// These four information strings we know implicitly,
	// so we'll set them now.
	_TCHAR szInfo[256];
	GetRegWizardVersionString(hInstance,szInfo);
	SetInformationString(kInfoVersion,szInfo);

	LoadString(hInstance,IDS_MAKERCOMPANY,szInfo,64);
	SetInformationString(kInfoMakerCompany,szInfo);

	SetInformationString(kInfoResultPath,m_szParamRegKey);

	GetDateFormat(LOCALE_SYSTEM_DEFAULT,0,NULL,_T("MM'/'dd'/'yyyy"),szInfo,256);
	SetInformationString(kInfoDate,szInfo);

	LANGID langID;
	GetSystemLanguageInfo(szInfo,256,&langID);
	wsprintf(szInfo,_T("%i"),langID);
	SetInformationString(kInfoLanguage,szInfo);

	// Build our tables specifying country-specific parameters
	// for all our edit fields.
	BuildAddrSpecTables();
    m_hAccel=LoadAccelerators(m_hInstance,MAKEINTRESOURCE(IDR_ACCELERATOR));

}

CRegWizard::~CRegWizard()
/*********************************************************************
Destructor for our Registration Wizard class
**********************************************************************/
{

	if(m_addrJumpTable != NULL)
		GlobalFree( m_addrJumpTable );
	
	if(m_addrSpecTable != NULL)
		GlobalFree( m_addrSpecTable );
		
	for (short index = 0;index < kInfoLastIndex;index++)
	{

		if(m_rgszInfoArray[index] != NULL)
		{
			LocalFree(m_rgszInfoArray[index]);
			m_rgszInfoArray[index] = NULL;
		}
	}

	if (m_hLogFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hLogFile);
	}

	FreeLibrary(m_ccpLibrary);
}


void CRegWizard::StartRegWizardDialog(int wDlgResID, DLGPROC lpDialogProc)
/*********************************************************************
Given a dialog template resource ID (wDlgResID) and a pointer to a
DialogProc callback function, StartRegWizardDialog creates and
displays a dialog window.

Note: Creating our dialogs as modeless lets us keep the current dialog
displayed while the next dialog is initializing (which can take a
while for some RegWizard dialogs), and then immediately flip to the
next dialog.
**********************************************************************/
{
	if (m_hwndStartDialog == NULL)
	{
		m_hwndStartDialog = CreateDialogParam(m_hInstance,MAKEINTRESOURCE(wDlgResID),
			NULL,lpDialogProc, (LPARAM) this);
		m_dialogActive = TRUE;
		ActivateRegWizardDialog();
	}
}


void CRegWizard::ActivateRegWizardDialog( void )
/*********************************************************************
ActivateRegWizardDialog should be called after creating a RegWizard
dialog window.  This function will display the window, and will
then destroy the current dialog window (if any).
**********************************************************************/
{
	if (m_hwndStartDialog)
	{	
		HWND hW;
		RECT r;
		
		
		hW = m_hwndDialogToHide? m_hwndDialogToHide : m_hwndCurrDialog;
		if( hW )
		{
			GetWindowRect(hW, &r);
			SetWindowPos( m_hwndStartDialog, NULL, r.left, r.top,0,0,
												SWP_NOSIZE|SWP_NOZORDER);
		
		}
		
		
		ShowWindow(m_hwndStartDialog,SW_SHOW);
		
		if(m_hwndDialogToHide != NULL)
		{
			ShowWindow(m_hwndDialogToHide,SW_HIDE);
			m_hwndDialogToHide = NULL;
		}
		else
		{
			if(m_hwndCurrDialog != NULL)
				DestroyWindow(m_hwndCurrDialog);
		}
		m_hwndCurrDialog = m_hwndStartDialog;
		m_hwndStartDialog = NULL;
		HCURSOR hCursor = LoadCursor(NULL,IDC_ARROW);
		SetCursor(hCursor);
	}
}


void CRegWizard::SetPrevDialog(void)
{
	m_hwndPrevDialog = m_hwndCurrDialog;
}

INT_PTR CRegWizard::ProcessRegWizardDialog( void )
/*********************************************************************
After calling StartRegWizardDialog, ProcessRegWizardDialog should
next be called.  This function will retain control until the user
dismisses the current dialog.  The ID of the control used to
terminate the dialog will be returned as the function result.
**********************************************************************/
{
	if (m_hwndCurrDialog)
	{
		while (IsDialogActive())
		{
			MSG msg;
			GetMessage(&msg,NULL,0,0);
	        if (!TranslateAccelerator(m_hwndCurrDialog, m_hAccel, &msg))
		    {
				if (!IsDialogMessage(m_hwndCurrDialog,&msg))
				{
					TranslateMessage(&msg);
			        DispatchMessage(&msg);
				}
			}
		}
	}
	return GetDialogExitButton();
}



void CRegWizard::EndRegWizardDialog(INT_PTR wExitID)
/*********************************************************************
Should be called from within the DialogProc when the user wants to
dismiss the dialog.  The ID of the control used to terminate the
dialog should be passed as the wExitID parameter.
**********************************************************************/
{
	HCURSOR hCursor = LoadCursor(NULL,IDC_WAIT);
	SetCursor(hCursor);
	EnableWindow(GetDlgItem(m_hwndCurrDialog,IDB_EXIT),FALSE);
	if(wExitID == IDB_REG_LATER)
		EnableWindow(GetDlgItem(m_hwndCurrDialog,IDB_REG_LATER),FALSE);
	else
	if(wExitID == IDB_BACK)
		EnableWindow(GetDlgItem(m_hwndCurrDialog,IDB_BACK),FALSE);
	else
	{
		if(GetDlgItem(m_hwndCurrDialog,IDB_BACK) == NULL)
			EnableWindow(GetDlgItem(m_hwndCurrDialog,IDB_REG_LATER),FALSE);
		else
			EnableWindow(GetDlgItem(m_hwndCurrDialog,IDB_BACK),FALSE);
	}

	EnableWindow(GetDlgItem(m_hwndCurrDialog,IDB_NEXT),FALSE);
	m_wDialogExitButton = wExitID;
	m_dialogActive = FALSE;
}


BOOL CRegWizard::IsDialogActive( void )
/*********************************************************************
Returns TRUE if a RegWizard dialog is currently active (i.e. the
EndRegWizardDialog function has not been called by the active dialog's
DialogProc.
**********************************************************************/
{
	return m_dialogActive;
}


INT_PTR CRegWizard::GetDialogExitButton( void )
/*********************************************************************
Returns the ID of the control used to dismiss the current dialog.
**********************************************************************/
{
	return m_wDialogExitButton;
}

void CRegWizard::SetDialogExitButton( int nButton )
/*********************************************************************
Returns the ID of the control used to dismiss the current dialog.
**********************************************************************/
{
	m_wDialogExitButton = nButton;
}

TriState CRegWizard::GetProductSearchLibraryStatus( void )
/*********************************************************************
Returns:
- kTriStateTrue: the ProductSearch library is available and can be
	successfully loaded.
- kTriStateFalse: the ProductSearch library couldn't be found.
- kTriStateUndefined: product searching does not need to be performed.
**********************************************************************/
{
	FARPROC lpfnProductSearch;
	BOOL status = GetProductSearchProcAddress(&lpfnProductSearch);
	if (status == TRUE)
	{
		return lpfnProductSearch == NULL ? kTriStateUndefined : kTriStateTrue;
	}
	else
	{
		return kTriStateFalse;
	}
}


BOOL CRegWizard::GetProductSearchProcAddress(FARPROC* lpfnProductSearch)
/*********************************************************************
This function attempts to load the ProductSearch (CCP) library, and
if successful, returns the ProcAddress of the RegProductSearch
function.

Returns:
-- TRUE if the ProcAddress passed in lpfnProductSearch is valid,
OR if the input parameters to RegWizard specify that no product
searching is to be performed (in which case NULL will be returned in
the lpfnProductSearch parameter).

-- FALSE if the ProductSearch library could not be located, AND the
input parameters to the RegWizard specified that product searching is
to be performed.

Note: RegWizard determines whether product searching is to be
performed by looking at the InventoryPath "input parameter" registry
key.  If it is not present, or contains a blank value, then no
product searching is to be performed.
**********************************************************************/
{
	BOOL returnVal = FALSE;
	if (m_lpfnProductSearch)
	{
		*lpfnProductSearch = m_lpfnProductSearch;
		returnVal = TRUE;
	}
	else
	{
		*lpfnProductSearch = NULL;
		_TCHAR szLibName[kRegBufferSize];
		BOOL goodParam = GetInputParameterString(IDS_INPUT_INVENTORYPATH,szLibName);
		if (goodParam == FALSE || (goodParam == TRUE && szLibName[0] == NULL))
		{
			*lpfnProductSearch = NULL;
			returnVal = TRUE;
		}
		else
		{
			m_ccpLibrary = LoadLibrary(szLibName);
			if (m_ccpLibrary)
			{
				m_lpfnProductSearch = GetProcAddress(m_ccpLibrary,"RegProductSearch");
				if (m_lpfnProductSearch)
				{
					*lpfnProductSearch = m_lpfnProductSearch;
					returnVal = TRUE;
				}
			}
		}
	}
	return returnVal;
}



BOOL CRegWizard::GetInputParameterStatus( void )
/***************************************************************************
Returns TRUE only if the input parameter registrion key passed to the
CRegWizard constructor points to a valid key that contains a proper block of
input parameter subkeys.
****************************************************************************/
{
	BOOL returnVal = FALSE;
	_TCHAR szParam[kRegBufferSize];
	if (GetInputParameterString(IDS_INPUT_PRODUCTNAME,szParam))
	{
		if (GetInputParameterString(IDS_INPUT_PRODUCTID,szParam))
		{
			returnVal = TRUE;

			// 12/13/94: we will no longer consider a missing inventory
			// path key to be an error (it now means "don't do product
			// inventory").
			//if (GetInputParameterString(IDS_INPUT_INVENTORYPATH,szParam))
			//{
			//	returnVal = TRUE;
			//}
		}
	}
	return returnVal;
}



BOOL CRegWizard::GetInputParameterString(short paramID, LPTSTR szParam)
/***************************************************************************
This function retrieves an input parameter string.  The paramID parameter
must be the resource ID of the Registration Database key whose contents are
to be returned in the szParam parameter.

Allowable values for paramID:
- IDS_INPUT_PRODUCTNAME
- IDS_INPUT_PRODUCTID
- IDS_INPUT_INVENTORYPATH
- IDS_INPUT_ISREGISTERED

Returns:
- FALSE if the specified key cannot be found in the Registry.
****************************************************************************/
{
	BOOL returnVal = FALSE;
	_TCHAR szFullParamRegKey[300];
	_TCHAR szParamSubKey[64];
	LPTSTR szValueName;
	szParam[0] = NULL;
	_tcscpy(szFullParamRegKey,m_szParamRegKey);
	int resSize = LoadString(m_hInstance,paramID,szParamSubKey,63);
	#ifdef USE_INPUT_SUBKEYS
	{
		_tcscat(szFullParamRegKey,_T"\\");
		_tcscat(szFullParamRegKey,szParamSubKey);
		szValueName = NULL;
	}
	#else
	{
		szValueName = szParamSubKey;
	}
	#endif

	HKEY hKey;
	LONG regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szFullParamRegKey,0,KEY_READ,&hKey);
	if (regStatus == ERROR_SUCCESS)
	{
		unsigned long infoSize;
		infoSize = kRegBufferSize;
		regStatus = RegQueryValueEx(hKey,szValueName,NULL,0,(LPBYTE) szParam,&infoSize);
		if (regStatus == ERROR_SUCCESS)
		{
			returnVal = TRUE;
		}
		RegCloseKey(hKey);
	}
	return returnVal;
}


BOOL CRegWizard::IsRegistered( void )
/***************************************************************************
This function returns TRUE if registration for the product specified by
the input parameters (via the Registration Database) has already been
performed.
****************************************************************************/
{
	_TCHAR szIsRegistered[kRegBufferSize];
	BOOL goodParam = GetInputParameterString(IDS_INPUT_ISREGISTERED,szIsRegistered);
	return goodParam == TRUE && szIsRegistered[0] == _T('1') ? TRUE : FALSE;

}


int CRegWizard::AddProduct(LPTSTR szProductName,LPTSTR szProductPath)
/***************************************************************************
This function adds the product whose name is given by the szProductName
parameter to the inventory of our user's installed products.

Returns: the new count of products on the list.
****************************************************************************/
{
	short strLenName = (_tcslen(szProductName)+1) * sizeof(_TCHAR);
	short strLenPath = (_tcslen(szProductPath)+1) * sizeof(_TCHAR);
	if (m_productNameCount < kMaxProductCount)
	{
		m_rgszProductName[m_productNameCount] = (LPTSTR) LocalAlloc(0,strLenName );
		_tcscpy(m_rgszProductName[m_productNameCount],szProductName);

		m_rgszProductPath[m_productNameCount] = (LPTSTR) LocalAlloc(0,strLenPath);
		_tcscpy(m_rgszProductPath[m_productNameCount],szProductPath);

		m_rghProductIcon[m_productNameCount] = NULL;
		m_productNameCount++;
	}
	return m_productNameCount;
}


void CRegWizard::GetProductName(LPTSTR szProductName,INT_PTR index)
/***************************************************************************
This function returns the product name whose index is given by the 'index'
parameter.  If index is greater than the number of products on the list, an
empty string will be returned.
****************************************************************************/
{
	szProductName[0] = NULL;
	if (index < m_productNameCount)
	{
		_tcscpy(szProductName, m_rgszProductName[index]);
	}
}

HICON CRegWizard::GetProductIcon(INT_PTR index)
/***************************************************************************
This function returns an icon for the product whose index is given by the
'index' parameter. If index is greater than the number of products on the
list, NULL will be returned.
****************************************************************************/
{
	if (index < m_productNameCount)
	{
		if (m_rghProductIcon[index] == NULL)
		{
			m_rghProductIcon[index] = ExtractIcon(m_hInstance,m_rgszProductPath[index],0);
			//m_rghProductIcon[index] = LoadImage(m_hInstance,m_rgszProductPath[index],IMAGE_ICON,32,32,LR_LOADFROMFILE);
			DWORD lastErr = GetLastError();
			if (m_rghProductIcon[index] == NULL)
			{
				m_rghProductIcon[index] = LoadIcon(m_hInstance,MAKEINTRESOURCE(IDI_REGWIZ));
			}
		}
		return m_rghProductIcon[index];
	}
	else
	{
		return NULL;
	}
}



short CRegWizard::GetProductCount( void )
/***************************************************************************
This function returns a count of the current number of products occupying
the inventory list.
****************************************************************************/
{
	return m_productNameCount;
}



void CRegWizard::SetInformationString(InfoIndex index, LPTSTR szInfo)
/***************************************************************************
This function saves the given string interally, associating it with info
attribute whose ID is given by the 'index' parameter.
****************************************************************************/
{
	short strLen ;
	if (index < kInfoLastIndex)
	{
		if (m_rgszInfoArray[index] != NULL)
		{
			LocalFree(m_rgszInfoArray[index]);
			m_rgszInfoArray[index] = NULL;
		}
		if(szInfo == NULL ) return;

		strLen = (_tcslen(szInfo)+1) * sizeof(_TCHAR);
		m_rgszInfoArray[index] = (LPTSTR) LocalAlloc(0, strLen);
		_tcscpy(m_rgszInfoArray[index],szInfo);
	}
}


BOOL CRegWizard::GetInformationString(InfoIndex index, LPTSTR szInfo)
/***************************************************************************
This function retrieves the information string whose ID is given by the
'index' parameter.  If the requested string has not been set yet, FALSE
will be returned as the function result, and an empty string will be
returned in szInfo.

Note: if you are interested only in determining whether the value is set
for a particular information string, you can pass NULL for szInfo.
****************************************************************************/
{
	if (index < kInfoLastIndex && m_rgszInfoArray[index] && m_rgszInfoArray[index][0]!=_T('\0') )
	{
		if (szInfo) _tcscpy(szInfo,m_rgszInfoArray[index]);
		return TRUE;
	}
	else
	{

		//if (szInfo) szInfo[0] = NULL;  by Suresh 06/6/97
		szInfo[0] = _T('\0');
		return FALSE;
	}
}


void CRegWizard::SetTriStateInformation(InfoIndex index, TriState infoValue)
/***************************************************************************
This function saves the given TriState value interally, associating it with
the info attribute whose ID is given by the 'index' parameter.

Note: if infoValue is kTriStateTrue, the value will be saved as "1"; if
kTriStateFalse or kTriStateUndefined, it will be saved as "0".
****************************************************************************/
{
	_TCHAR szInfo[4];
	_stprintf(szInfo,_T("%i"),infoValue == kTriStateTrue ? 1 : 0);
	SetInformationString(index,szInfo);
}


TriState CRegWizard::GetTriStateInformation(InfoIndex index)
/***************************************************************************
This function retrieves the TriState value whose ID is given by the 'index'
parameter.

Returns:
- kTriStateTrue
- kTriStateFalse
- kTriStateUndefined: value has not been set yet.
****************************************************************************/
{
	_TCHAR szInfo[kRegBufferSize];
	BOOL goodString = GetInformationString(index,szInfo);
	if (goodString == FALSE)
	{
		return kTriStateUndefined;
	}
	else
	{
		return szInfo[0] == _T('0') ? kTriStateFalse : kTriStateTrue;
	}
}


void CRegWizard::WriteEnableInformation(InfoIndex index, BOOL shouldWrite)
/***************************************************************************
If shouldWrite is TRUE, the information associated with the given index
will be enabled for writing to the Registration Database; otherwise, the
value for this index will be written as a NULL string.  By default, all
information members are write-enabled.
****************************************************************************/
{
	m_writeEnable[index] = shouldWrite;
}


BOOL CRegWizard::IsInformationWriteEnabled(InfoIndex index)
/***************************************************************************
Returns TRUE if the information associated with the given index is enabled
for writing to the Registration Database.
****************************************************************************/
{
	return m_writeEnable[index];
}


void CRegWizard::SetProductSearchStatus(TriState searchCompleted)
/***************************************************************************
This function needs to be called with a searchCompleted value of
kTriStateTrue when the product searching thread completes, or
kTriStateUndefined if an error prevents the search from being completed.
****************************************************************************/
{
	m_searchCompleted = searchCompleted;
}


TriState CRegWizard::GetProductSearchStatus( void )
/***************************************************************************
This function returns:
- kTriStateTrue if the product searching thread has completed.
- kTriStateFalse if the search is still in progress.
- kTriStateUndefined if an error prevented the search from being completed.
****************************************************************************/
{
	return m_searchCompleted;
}


void CRegWizard::SetSystemInventoryStatus(BOOL invCompleted)
/***************************************************************************
This function needs to be called with a invCompleted value of TRUE when
the system inventory compilation thread completes.
****************************************************************************/
{
	m_systemInventoryCompleted = invCompleted;
}


BOOL CRegWizard::GetSystemInventoryStatus( void )
/***************************************************************************
This function returns TRUE if the system inventory compilation thread has
completed.
****************************************************************************/
{
	return m_systemInventoryCompleted;
}


void CRegWizard::WriteInfoToRegistry( void )
/***************************************************************************
Writes all the information gathered by the RegWizard to the appropriate keys
in the Registration Database.
****************************************************************************/
{

	short index = kInfoFirstName;
	_TCHAR szRegKey[kRegBufferSize];
	_TCHAR szLogBuffer[kRegBufferSize];
	HKEY hKey;
	DWORD dwReserved=0;
	DWORD dwDisposition;
	TBYTE* lpbData;
	DWORD  dwStrSz;

	#ifndef WRITE_COUNTRY_AS_STRING
	wsprintf(szLogBuffer,_T("%li"),GetCountryCode());
	SetInformationString(kInfoCountry,szLogBuffer);
	#endif
	
	_tcscpy(szLogBuffer,_T("=== Microsoft Registration Wizard ==="));
	WriteToLogFile(szLogBuffer);

	GetInfoRegistrationParentKey(szRegKey);	

	LONG regStatus = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
					szRegKey,
					dwReserved,
					NULL,
					REG_OPTION_NON_VOLATILE,
					KEY_ALL_ACCESS,
					NULL,
					&hKey,
					&dwDisposition);

	if (regStatus != ERROR_SUCCESS) return;

	while (index != kInfoLastIndex)
	{
		_TCHAR szInfo[kRegBufferSize];
		_TCHAR szValueName[kRegBufferSize];
		GetInfoRegValueName((InfoIndex) index,szValueName);

		// The value of any information entry that has been "write-disabled"
		// will be blanked out (i.e. the key will be written, but with an
		// empty string as the value).
		if (m_writeEnable[index] == FALSE)
		{
			szInfo[0] = NULL;
		}
		else
		{
			GetInformationString((InfoIndex) index,szInfo);
		}
		
		// First, write to our log file
		wsprintf(szLogBuffer,_T("%s = %s"),szValueName,szInfo);
		WriteToLogFile(szLogBuffer);
		if(szInfo[0] == _T('\0')) {
			lpbData = (TBYTE*) _T("\0");
			dwStrSz = _tcslen((const TCHAR *) lpbData)* sizeof(_TCHAR);
			regStatus = RegSetValueEx(hKey,szValueName,NULL,REG_SZ,(CONST BYTE*) lpbData,sizeof(TCHAR));
			

		}else{
		 lpbData = (TBYTE*) szInfo;
		 dwStrSz = _tcslen(szInfo)* sizeof(_TCHAR);
		 	regStatus = RegSetValueEx(hKey,szValueName,NULL,REG_SZ,(CONST BYTE *)lpbData, dwStrSz);
		}
		
		index++;
	}

	// If the kInfoIncludeProducts flag is FALSE, we want to write the ProductX keys,
	// but blank out the values.
	BOOL shouldIncludeProducts = GetTriStateInformation(kInfoIncludeProducts);
	index = 0;
	_TCHAR szProductBase[64];
	int resSize = LoadString(m_hInstance,IDS_PRODUCTBASEKEY,szProductBase,64);
	while (index < kMaxProductCount)
	{
		_TCHAR szProductValueName[kRegBufferSize];
		_TCHAR szProductName[kRegBufferSize];
		_stprintf(szProductValueName,_T("%s %i"),szProductBase,index + 1);
		if (shouldIncludeProducts == kTriStateTrue)
		{
			GetProductName(szProductName,index);
		}
		else
		{
			szProductName[0] = NULL;
		}
		wsprintf(szLogBuffer,_T("%s = %s"),szProductValueName,szProductName);
		WriteToLogFile(szLogBuffer);
		regStatus = RegSetValueEx(hKey,szProductValueName,NULL,REG_SZ,(CONST BYTE*) szProductName,_tcslen(szProductName)* sizeof(_TCHAR));
		index++;
	}

	RegCloseKey(hKey);
	CloseLogFile();
}
									

void CRegWizard::ReadInfoFromRegistry( void )
/***************************************************************************
Reads any information written to the Registration Database by previous runs
of RegWizard, and populates all information strings accordingly.
****************************************************************************/
{
	short index = kInfoFirstName;
	_TCHAR szRegKey[kRegBufferSize];
	HKEY hKey;

	GetInfoRegistrationParentKey(szRegKey);	
#ifdef SURESH
	LONG regStatus = RegOpenKeyEx(HKEY_CURRENT_USER,szRegKey,0,KEY_READ,&hKey);
#endif
	LONG regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szRegKey,0,KEY_READ,&hKey);

	if (regStatus == ERROR_SUCCESS)
	{
		while (index != kInfoLastIndex)
		{
			_TCHAR szInfo[kRegBufferSize];
			_TCHAR szValueName[kRegBufferSize];
			unsigned long infoSize;
			BOOL refresh = GetInfoRegValueName((InfoIndex) index,szValueName);
			if (refresh)
			{
				infoSize = kRegBufferSize;
				regStatus = RegQueryValueEx(hKey,szValueName,NULL,0,(LPBYTE) szInfo,&infoSize);
				if (regStatus == ERROR_SUCCESS)
				{
					SetInformationString((InfoIndex) index,szInfo);
				}
			}
			index++;
		}
		RegCloseKey(hKey);
	}

	// Two of the information fields are fed to RegWizard as input parameters;
	// we'll read these from our input parameter block.
	_TCHAR szParam[kRegBufferSize];
	GetInputParameterString(IDS_INPUT_PRODUCTNAME,szParam);
	SetInformationString(kInfoApplicationName,szParam);
	GetInputParameterString(IDS_INPUT_PRODUCTID,szParam);
	SetInformationString(kInfoProductID,szParam);
}


BOOL CRegWizard::GetInfoRegValueName(InfoIndex infoIndex,LPTSTR szValueName)
/***************************************************************************
Returns in buffer pointed to by szValueName the value name associated with
the information item specified by infoIndex.

Returns:
If this key represents a value that should be used to refresh the value in
memory when RegWizard starts up,TRUE will be returned as the function result.
****************************************************************************/
{
	_TCHAR szOrigValueName[kRegBufferSize];
	LPTSTR szValueNamePtr;
	BOOL shouldRefresh = FALSE;
	short infoResIndex ;
	int resSize;

	if(infoIndex < kDivisionName ){
		infoResIndex = IDS_INFOKEY1 + infoIndex;
		resSize = LoadString(m_hInstance,infoResIndex,szOrigValueName,255);
	}else {
		infoResIndex = IDS_DIVISIONNAME_KEY + (infoIndex-kDivisionName);
		resSize = LoadString(m_hInstance,infoResIndex,szOrigValueName,255);

	}
	// After 03/03/98 in corporationg extra fields for Division and UserID, assuming
	// more fields can be added in the future a set of 15 more ID is reserver in the resource
	// so for Division and USEID, this pool of resource ID  will be used



	// SubKeys are flagged as 'refresh' keys by a leading underscore
	szValueNamePtr = szOrigValueName;
	if (szOrigValueName[0] == _T('_'))
	{
		shouldRefresh = TRUE;
		szValueNamePtr = _tcsinc(szValueNamePtr);
	}
	_tcscpy(szValueName,szValueNamePtr);
	return shouldRefresh;
}


void CRegWizard::GetInfoRegistrationParentKey(LPTSTR szRegKey)
/***************************************************************************
Returns the Registration Database key that specifies the parent of all our
information keys.
****************************************************************************/
{
	if (m_szInfoParentKey[0] == NULL)
	{
		_TCHAR szPartialKey[kRegBufferSize];
		int resSize = LoadString(m_hInstance,IDS_KEY2,szRegKey,255);
		_tcscat(szRegKey,_T("\\"));
		resSize = LoadString(m_hInstance,IDS_KEY3,szPartialKey,255);
		_tcscat(szRegKey,szPartialKey);
		_tcscat(szRegKey,_T("\\"));
		resSize = LoadString(m_hInstance,IDS_KEY4,szPartialKey,255);
		_tcscat(szRegKey,szPartialKey);
		#ifdef USE_INFO_SUBKEYS
		_tcscat(szRegKey,_T("\\"));
		#endif
		_tcscpy(m_szInfoParentKey,szRegKey);
	}
	else
	{
		_tcscpy(szRegKey,m_szInfoParentKey);
	}
}


void CRegWizard::GetRegKey(LPTSTR szRegKey)
/**************************************************************************
Returns the country code selected by the user in the Address dialog.
****************************************************************************/
{
	_tcscpy(szRegKey,m_szParamRegKey);
}


HINSTANCE CRegWizard::GetInstance( void )
/***************************************************************************
Returns the instance handle of the application that created this
CRegWiz object.
****************************************************************************/
{
	return m_hInstance;
}


void CRegWizard::SetCountryCode(DWORD countryCode)
/***************************************************************************
Saves off the country code selected by the user in the Address dialog.
****************************************************************************/
{
	m_countryCode = countryCode;
}


DWORD CRegWizard::GetCountryCode( void )
/**************************************************************************
Returns the country code selected by the user in the Address dialog.
****************************************************************************/
{
	return m_countryCode;
}


void CRegWizard::GetCountryAddrSpec(LONG lCountryID,ADDRSPEC* addrSpec )
/**************************************************************************
Returns a ADDRSPEC structure based on the specified countryID value.
****************************************************************************/
{
	if (lCountryID >= 0 && m_addrJumpTable != NULL)
	{
		JTE* addrJumpTable = (JTE*) GlobalLock(m_addrJumpTable);
		JTE jumpIndex = addrJumpTable[lCountryID];
		if (addrSpec)
		{
			ADDRSPEC* addrSpecTable = (ADDRSPEC*) GlobalLock(m_addrSpecTable);
			*addrSpec = addrSpecTable[jumpIndex];
			GlobalUnlock(m_addrSpecTable);
		}
		GlobalUnlock(m_addrJumpTable);
	}
}


void CRegWizard::GetAddrSpecProperties(DWORD dwCountryCode, ADDRSPEC_FIELD addrSpecField, MAXLEN* maxLen,BOOL* isRequired)
/*********************************************************************
Returns the maximum char length and "required?" properties associated
with the editable field specified by addrSpecField, and the country
code specified in the dwCountryCode parameter.
**********************************************************************/
{
	BOOL lclIsRequired = TRUE;
	ADDRSPEC addrSpec;
	GetCountryAddrSpec(dwCountryCode,&addrSpec);
	if (maxLen)
	{
		*maxLen = addrSpec.maxLen[addrSpecField];
		if (*maxLen < 0)
		{
			*maxLen = abs(*maxLen);
			lclIsRequired = FALSE;
		}
	}
	if((dwCountryCode == 0)&&(addrSpecField == kAddrSpecState))
	{
			*maxLen = (MAXLEN)2;
	}
	if((dwCountryCode == 0)&&(addrSpecField == kAddrSpecResellerState))
	{
			*maxLen = (MAXLEN)2;
	}
	
	RW_DEBUG << "COUNTRY CODE:" << dwCountryCode << "SPEC FIELD" << addrSpecField<<"LENGTH:" << (int) (*maxLen) << endl;

	if (isRequired) *isRequired = lclIsRequired;
}


BOOL CRegWizard::IsEditTextFieldValid(HWND hwndDlg,int editID)
/*********************************************************************
Returns TRUE if the edit text field specified by the editID parameter
contains at least one character, OR has been marked as 'not required'
by a call to the ConfigureEditTextField function.

Note: if the edit text field has been disabled, it will considered to
be "not required", regardless of the state set by ConfigureEditText-
Field.
**********************************************************************/
{
	HWND hwndEdit = GetDlgItem(hwndDlg,editID);
	BOOL isEnabled = IsWindowEnabled(hwndEdit);
	BOOL isRequired = isEnabled == FALSE ? FALSE : (BOOL) HIWORD(GetWindowLongPtr(hwndEdit,GWLP_USERDATA));
	LRESULT editTextLen = SendMessage(hwndEdit,WM_GETTEXTLENGTH,0,0L);
	return isRequired == FALSE || editTextLen > 0 ? TRUE : FALSE;
}



void CRegWizard::GetEditTextFieldAttachedString(HWND hwndDlg,int editID,LPTSTR szAttached,int cbBufferSize)
/*********************************************************************
Returns the string whose resource ID was attached to the specified
edit control by a call to ConfigureEditTextField
**********************************************************************/
{
	_TCHAR szBuffer[kRegBufferSize];
	HWND hwndEdit = GetDlgItem(hwndDlg,editID);
	WORD wLabelID = LOWORD(GetWindowLongPtr(hwndEdit,GWLP_USERDATA));
	HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwndDlg,GWLP_HINSTANCE);
	GetDlgItemText(hwndDlg,wLabelID,szAttached,cbBufferSize);
	StripCharFromString(szAttached, szBuffer, _T('&'));
	StripCharFromString(szBuffer, szAttached, _T(':'));
}



void CRegWizard::ConfigureEditTextField(HWND hwndDlg,int editFieldID,ADDRSPEC_FIELD addrSpecField,int iAttachedStrID)
/*********************************************************************
Sets the maximum character limit and "required?" status for the edit
field whose ID is given by the editFieldID parameter.  The
addrSpecField parameter specifies which field within the AddrSpec
(as defined by the currently selected country) to use to determine
the char limit and "required?" values.

If the text currently residing in the edit control is longer than
the maximum allowed for that field, ConfigureEditTextField will
truncate it to the allowable value.

Note: the current country code MUST be set via a call to
SetCountryCode before calling ConfigureEditTextField.
**********************************************************************/
{
	MAXLEN maxLen;
	BOOL isRequired;
	DWORD dwTapiCntryId;
	maxLen = 0;
	isRequired = FALSE;
	dwTapiCntryId = gTapiCountryTable.GetTapiIDForTheCountryIndex(m_countryCode);
	//GetAddrSpecProperties(m_countryCode,addrSpecField,&maxLen,&isRequired);
	GetAddrSpecProperties(dwTapiCntryId,addrSpecField,&maxLen,&isRequired);
	SendDlgItemMessage(hwndDlg,editFieldID,EM_LIMITTEXT,maxLen,0L);
	
	LRESULT dwTextLen = SendDlgItemMessage(hwndDlg,editFieldID,WM_GETTEXTLENGTH,0,0L);
	if (dwTextLen > (LRESULT) maxLen)
	{
		_TCHAR szText[256];
		SendDlgItemMessage(hwndDlg,editFieldID,WM_GETTEXT,255,(LPARAM) szText);
		szText[maxLen] = NULL;
		SendDlgItemMessage(hwndDlg,editFieldID,WM_SETTEXT,0,(LPARAM) szText);
	}

	HWND hwndEdit = GetDlgItem(hwndDlg,editFieldID);
	LONG_PTR lWindowLong = (isRequired << 16) | iAttachedStrID;
	SetWindowLongPtr(hwndEdit,GWLP_USERDATA,(LONG_PTR)lWindowLong);
}



void CRegWizard::BuildAddrSpecTables( void )
/*********************************************************************
Builds country code-dependent name/address specification table (by
reading and parsing the specification strings for each country from
string resources.
**********************************************************************/
{
	// Build country-code dependent address spec tables
	LONG lMaxCntryCode = GetResNumber(m_hInstance,IDS_CNTRY_MAXCODE);
	m_addrJumpTable = GlobalAlloc(GHND, sizeof(JTE) * (lMaxCntryCode + 1));
	JTE* addrJumpTable = (JTE*) GlobalLock(m_addrJumpTable);

	LONG lUniqueCount = GetResNumber(m_hInstance,IDS_CNTRY_UNIQUECOUNT);
	m_addrSpecTable = GlobalAlloc(GHND, sizeof(ADDRSPEC) * (lUniqueCount + 1));
	ADDRSPEC* addrSpecTable = (ADDRSPEC*) GlobalLock(m_addrSpecTable);

	LONG lSpecTableIndex = 0;
	int iStrResID = IDS_CNTRY_DEFAULT;
	while (iStrResID < IDS_CNTRY_END && lSpecTableIndex < kMaxAddrSpecTableSize)
	{
		_TCHAR szSpecString[64];
		int iStrLen = LoadString(m_hInstance,iStrResID,szSpecString,64);
		// Gaps are allowable in the string resource ID numbering, so watch out.
		if (iStrLen > 0)
		{						
			LPTSTR szSpecPtr = szSpecString;
			LPTSTR szNext;						
			long lCntryCode = _tcstol(szSpecPtr,&szNext,10);
			if (lCntryCode < lMaxCntryCode && lCntryCode >= 0 && szNext[0] == _T(':'))
			{
				//szSpecPtr += _tcsclen(szNext + 1);
				szSpecPtr = szNext + 1;
				LONG lInterTableReference = _tcstol(szSpecPtr,&szNext,10);
				if (szNext[0] == NULL || szNext[0] == _T(' '))
				{
					addrJumpTable[lCntryCode] = addrJumpTable[lInterTableReference];
				}
				else
				{
					addrJumpTable[lCntryCode] = (JTE) lSpecTableIndex;
					WORD wSpecIndex = 0;
					do
					{
						LONG lMaxLen = _tcstol(szSpecPtr,&szNext,10);

						// A missing 'y' or 'n' specifier is considered a
						// syntax error, and we'll stop processing that line.
						if (szNext[0] == _T('y') || szNext[0] == _T('n'))
						{
							if (lMaxLen > kMaxLenSize) lMaxLen = kMaxLenSize;
							if (szNext[0] == _T('n')) lMaxLen *= -1;
							addrSpecTable[lSpecTableIndex].maxLen[wSpecIndex++] = (MAXLEN) lMaxLen;
							szSpecPtr = szNext + 1;
						}
						else
						{
							szSpecPtr = NULL;
						}
					}while (szSpecPtr && szSpecPtr[0] && wSpecIndex < kAddrSpecCount);
					lSpecTableIndex++;
				}
			}
		}
		iStrResID++;	
	}
	GlobalUnlock(m_addrJumpTable);
	GlobalUnlock(m_addrSpecTable);
}


void CRegWizard::ResolveCurrentCountryCode( void )
/*********************************************************************
If the user has run RegWizard previously, we'll use the last selection
as the current country code.  Otherwise, we'll ask Tapi for the
current system country code.
**********************************************************************/
{
	_TCHAR szCountry[kRegBufferSize];
	DWORD dwCountryCode=0;
		
	if( GetInformationString(kInfoCountry,szCountry) )
	{
		dwCountryCode = _ttol(szCountry);
	}
	else
	{
		// Get TAPI Country Code
		if (GetTapiCurrentCountry(m_hInstance,&dwCountryCode)){
			RW_DEBUG << "\n TAPI Country Code :[" << dwCountryCode << flush;
				

			dwCountryCode= gTapiCountryTable.GetCountryCode(dwCountryCode);
			RW_DEBUG << "]=Mapping Index : " << dwCountryCode << flush;
		}else {
			dwCountryCode = 0; // Fore it to USA

		}
	}
	SetCountryCode(dwCountryCode);
}


void CRegWizard::SetLogFileName(LPTSTR lpszLogFilePath)
/*********************************************************************
If this function is called with a valid full pathname to a file,
CRegWizard will write all registration information, as text, to this
file, in addition to writing it to the Registration Database.
**********************************************************************/
{
	_tcscpy(m_szLogFilePath,lpszLogFilePath);
}


void CRegWizard::CreateLogFile( void )
/**********************************************************************
This function attempts to create a logfile into which all pertinent
registration information will be dumped, if that file has not been
created already (by a previous call to CreateLogFile).

Note: CreateLogFile is called automatically by WriteToLogFile if the
log file does not exist, so you don't need to call CreateLogFile first.
***********************************************************************/
{
	if (m_szLogFilePath[0] && m_hLogFile == INVALID_HANDLE_VALUE)
	{
		m_hLogFile = CreateFile(m_szLogFilePath,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,NULL);
	}
}


void CRegWizard::WriteToLogFile(LPTSTR lpszLine)
/**********************************************************************
This function first checks to see if registration logging has been
enabled (via a call to SetLogFileName), and if so, writes the given
line to the designated log file.

Note: WriteToLogFile automatically appends CR/LF to the given string.

Note: If the log file does not yet exist, WriteToLogFile will create
it automatically.
***********************************************************************/
{
	#define chEol	_T('\n')
	#define chCR	_T('\r')

	if (m_hLogFile == INVALID_HANDLE_VALUE)
	{
		CreateLogFile();
	}
	
	if (m_hLogFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwBytesWritten;
		DWORD wLen = _tcslen(lpszLine);
		lpszLine[wLen]   = chCR;
		lpszLine[wLen + 1] = chEol;
		lpszLine[wLen + 2] = NULL;
		WriteFile(m_hLogFile,lpszLine, _tcslen(lpszLine)* sizeof(_TCHAR),&dwBytesWritten,NULL);
	}		
}


void CRegWizard::CloseLogFile( void )
/**********************************************************************
If the RegWizard log file is open, this function closes it.
***********************************************************************/
{
	if (m_hLogFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hLogFile);
	}
}


HWND CRegWizard::GetCurrDialog(void )
{
	return m_hwndCurrDialog;
}

void CRegWizard::SetDialogHide(HWND hDialogToHide)
{
	m_hwndDialogToHide = hDialogToHide;
}

BOOL CRegWizard::ShowPrevDialog(void)
{
	if(m_hwndPrevDialog != NULL)
	{
		RECT r;
		HWND hwndTmpDialog;
		GetWindowRect(m_hwndDialogToHide, &r);
		SetWindowPos(m_hwndPrevDialog, NULL, r.left, r.top,0,0,SWP_NOSIZE|SWP_NOZORDER);
		ShowWindow(m_hwndDialogToHide,SW_HIDE);
		m_hwndDialogToHide = NULL;
		hwndTmpDialog = m_hwndCurrDialog;
		m_hwndCurrDialog = m_hwndPrevDialog;
		m_hwndPrevDialog = hwndTmpDialog;
		
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void CRegWizard::SetWindowCaption(LPTSTR lpszWindowsCaption)
{
	_TCHAR szTitle[64];
	LoadString(m_hInstance,IDS_WINDOWS_CAPTION,szTitle,64);
	_tcscpy(m_szWindowsCaption,lpszWindowsCaption);
	_tcscat(m_szWindowsCaption,szTitle);
}


LPTSTR CRegWizard::GetWindowCaption(void)
{
	return m_szWindowsCaption;
}

