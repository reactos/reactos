// CRegWizCtrl.cpp : Implementation of CRegWizCtrl
#include "stdafx.h"
#include "RegwizCtrl.h"
#include "CRwCtrl.h"
#include "rw_common.h"
#include "regutil.h"

int TstFn1 ( HANDLE hInstance  );

_TCHAR szProductPath[256];

/////////////////////////////////////////////////////////////////////////////
// CRegWizCtrl

STDMETHODIMP CRegWizCtrl::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IRegWizCtrl,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CRegWizCtrl::InvokeRegWizard(BSTR ProductPath)
{
	LPCTSTR  pPath;
	int      iRet;
	iRet = 0; // No Error
	// TODO: Add your implementation code here
	//TstFn1 ( _Module.GetModuleInstance(), m_ctlButton.m_hWnd);
	#ifndef _UNICODE
	pPath	= BstrToSz(ProductPath);
	#else
	pPath   = ProductPath;
	#endif
	if(pPath) {
		//MessageBox("TransferRegWizInformation",lpInfo);
		iRet = InvokeRegistration ( _Module.GetModuleInstance()  ,pPath );
#ifndef _UNICODE
		CoTaskMemFree((void *) pPath);
#endif

	}
	//return (HRESULT) iRet;
	//return iRet == NO_ERROR ? S_OK:E_FAIL;
	return S_OK;
}

STDMETHODIMP CRegWizCtrl::TransferRegWizInformation(BSTR InformationPath)
{
	// TODO: Add your implementation code here
	LPCTSTR lpInfo;
	//::MessageBox(NULL,"TransferRegWizInformation","SSS ",MB_OK);

#ifndef _UNICODE
	lpInfo	= BstrToSz(InformationPath);
#else
	lpInfo = InformationPath;
#endif

	if(lpInfo) {
	//	MessageBox("TransferRegWizInformation",lpInfo);
		TransmitRegWizInfo(_Module.GetModuleInstance(), lpInfo,FALSE);
#ifndef _UNICODE
		CoTaskMemFree((void *) lpInfo);
#endif

	}


	return S_OK;
}

STDMETHODIMP CRegWizCtrl::put_IsRegistered(BSTR InformationPath)
{
	LPCTSTR lpInfo;

	#ifndef _UNICODE
		lpInfo	= BstrToSz(InformationPath);
	#else
		lpInfo = InformationPath;
	#endif

	_tcscpy(szProductPath,lpInfo);
	return S_OK;
}

STDMETHODIMP CRegWizCtrl::get_IsRegistered(VARIANT_BOOL * pbStatus)
{
	int iReturn ;
	HKEY hKey;
	TCHAR szValueName[256] = _T("");
	TCHAR szRetVal[48];
	DWORD dwSize= 48;
	LONG regStatus ;
	
	*pbStatus = FALSE;

	HINSTANCE hInstance = _Module.GetModuleInstance();

	int resSize = LoadString(hInstance,
				IDS_INPUT_ISREGISTERED,szValueName,128);

	iReturn  = 1;

    regStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
	szProductPath,0,KEY_READ |KEY_WRITE,&hKey);
	if (regStatus != ERROR_SUCCESS) 
	{
		return E_FAIL; // error
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
			*pbStatus = TRUE; // Product Registred flag is set
		}
	}
	RegCloseKey(hKey);
	return S_OK;
}

/*   B S T R   C R E A T E   S Z   */
/*-------------------------------------------------------------------------
    Owner: SteveBu
 
    Creates a BSTR from a string
-------------------------------------------------------------------------*/
#ifndef _UNICODE
BSTR BstrCreateSz(LPCSTR lpa)
{
    BSTR bsz;
    UINT cch;

    cch = lstrlen(lpa);

    bsz = SysAllocStringLen(NULL, cch);
    
    if (bsz == NULL)
        return NULL;

    if (cch > 0)
	    MultiByteToWideChar(CP_ACP, 0, lpa, cch, bsz, cch);

	bsz[cch] = NULL;
	return bsz;
}
#endif 


/*   G E T _   M   S   I   D   */
/*-------------------------------------------------------------------------
    Owner: SteveBu
 
    Returns a user's MSID from the Registry.  If it does not exist, this 
    code issues a MSID using GetNewGUID.
-------------------------------------------------------------------------*/
STDMETHODIMP CRegWizCtrl::get_MSID(BSTR *pbstrMSID)
{
	HRESULT hr = S_OK;
	_TCHAR szValue[256];
	char   czNewGuid[256];
	BOOL fPutMSID = FALSE;

	szValue[0] = '\0';

	*pbstrMSID = NULL;

	if(!GetMSIDfromRegistry(_Module.GetModuleInstance(), szValue))
		{
		GetNewGUID(czNewGuid);
		_tcscpy(szValue,ConvertToUnicode(czNewGuid));

		fPutMSID = TRUE;
		}

	// TODO: Add your implementation code here

#ifndef _UNICODE
	if ((*pbstrMSID = BstrCreateSz(szValue)) == NULL)
#else
	if ((*pbstrMSID = SysAllocString(szValue)) == NULL)
#endif // _UNICODE
		{
		hr = E_OUTOFMEMORY;
		}


	if (SUCCEEDED(hr) && fPutMSID && *pbstrMSID)
		return put_MSID(*pbstrMSID);
	
	return hr;
}

/*   P U T _   M   S   I   D   */
/*-------------------------------------------------------------------------
    Owner: SteveBu
 
    Adds a MSID to the registry
-------------------------------------------------------------------------*/
STDMETHODIMP CRegWizCtrl::put_MSID(BSTR bstrMSID)
{
	DWORD	dwRet;
	_TCHAR szKeyName[256];
	HKEY	hIDKey;
	TCHAR  szMSID[256];
	HINSTANCE hInstance = _Module.GetModuleInstance();

	_tcscpy(szKeyName,_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"));

#ifndef _UNICODE
	WideCharToMultiByte(CP_ACP, 0, bstrMSID, -1, szMSID, 256, NULL, NULL);
#else
	_tcscpy(szMSID, bstrMSID);
#endif // _UNICODE

	dwRet = RegOpenKeyEx(HKEY_CURRENT_USER,szKeyName,0,KEY_ALL_ACCESS,&hIDKey);
	if (dwRet == ERROR_SUCCESS) 
		{
		szMSID[32]=_T('\0');
		// Store MSID into
		dwRet = RegSetValueEx(hIDKey,_T("MSID"),NULL,REG_SZ,(CONST BYTE *)szMSID,
								_tcslen((LPCTSTR)szMSID));
		RegCloseKey(hIDKey);
		}

	_TCHAR szPartialKey[256];
	int resSize = LoadString(hInstance,IDS_KEY2,szKeyName,255);
	_tcscat(szKeyName,_T("\\"));
	resSize = LoadString(hInstance,IDS_KEY3,szPartialKey,255);
	_tcscat(szKeyName,szPartialKey);
	_tcscat(szKeyName,_T("\\"));
	resSize = LoadString(hInstance,IDS_KEY4,szPartialKey,255);
	_tcscat(szKeyName,szPartialKey);

	dwRet = RegOpenKeyEx(HKEY_CURRENT_USER,szKeyName,0,KEY_ALL_ACCESS,&hIDKey);
	if (dwRet == ERROR_SUCCESS) 
		{
		// Store MSID into
		dwRet = RegSetValueEx(hIDKey,_T("MSID"),NULL,REG_SZ,(CONST BYTE *)szMSID, _tcslen((LPCTSTR)szMSID));
		RegCloseKey(hIDKey);
		}

	return S_OK;
}

/*   G E T _   H   W   I   D   */
/*-------------------------------------------------------------------------
    Owner: SteveBu
 
    Returns the machine HWID or creates it.
-------------------------------------------------------------------------*/
STDMETHODIMP CRegWizCtrl::get_HWID(BSTR * pbstrHWID)
{
	HKEY hKeyHWID;
	HRESULT hr = S_OK;
	BOOL fPutHWID = FALSE;

	_TCHAR szKeyName[256];
	_TCHAR szValue[256];
	char   czNewGuid[256];

	*pbstrHWID = NULL;
	szValue[0] = '\0';

	
	_tcscpy(szKeyName,_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"));
	
	LONG regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKeyName, 0, KEY_READ, &hKeyHWID);
	
	if (regStatus == ERROR_SUCCESS)
		{
		_TCHAR szValueName[64];
		unsigned long infoSize = 255;
		
		LoadString(_Module.GetModuleInstance(),IDS_HWID, szValueName,64);
		regStatus = RegQueryValueEx(hKeyHWID,szValueName,NULL,0,(LPBYTE) szValue,&infoSize);
		if (regStatus != ERROR_SUCCESS)
			{
			GetNewGUID(czNewGuid);
			_tcscpy(szValue,ConvertToUnicode(czNewGuid));
			fPutHWID = TRUE;
			}
			
		RegCloseKey(hKeyHWID);
		}

#ifndef _UNICODE
	if ((*pbstrHWID = BstrCreateSz(szValue)) == NULL)
#else
	if ((*pbstrHWID = SysAllocString(szValue)) == NULL)
#endif // _UNICODE
		{
		hr = E_OUTOFMEMORY;
		}

	if (SUCCEEDED(hr) && fPutHWID && *pbstrHWID)
		return put_HWID(*pbstrHWID);
	
	return hr;
}

/*   P U T _   H   W   I   D   */
/*-------------------------------------------------------------------------
    Owner: SteveBu
 
    Adds the passed in HWID to the registry
-------------------------------------------------------------------------*/
STDMETHODIMP CRegWizCtrl::put_HWID(BSTR bstrHWID)
{
	DWORD	dwRet;
	_TCHAR szKeyName[256];
	HKEY	hIDKey;
 
	_tcscpy(szKeyName,_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"));

	// Store HWID into Registry
	dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKeyName,0,KEY_ALL_ACCESS,&hIDKey);
	
	if (dwRet == ERROR_SUCCESS) 
		{
		_TCHAR szHWID[256];
		
#ifndef _UNICODE
		WideCharToMultiByte(CP_ACP, 0, bstrHWID, -1, szHWID, 256, NULL, NULL);
#else
		_tcscpy(szHWID, bstrHWID);
#endif // _UNICODE		

		dwRet = RegSetValueEx(hIDKey,_T("HWID"),NULL, REG_SZ,(CONST BYTE *)szHWID, _tcslen((LPCTSTR)szHWID));
		RegCloseKey(hIDKey);
		}

	return S_OK;
}

/*   T R A N S F E R   O   E   M   R E G   W I Z   I N F O R M A T I O N   */
/*-------------------------------------------------------------------------
    Owner: SteveBu
 
    Transfer product registration and OEM information.
-------------------------------------------------------------------------*/
STDMETHODIMP CRegWizCtrl::TransferOEMRegWizInformation(BSTR InformationPath)
{
	LPCTSTR lpInfo;
	//::MessageBox(NULL,"TransferOEMRegWizInformation","SSS ",MB_OK);

#ifndef _UNICODE
	lpInfo	= BstrToSz(InformationPath);
#else
	lpInfo = InformationPath;
#endif

	if(lpInfo) {
	//	MessageBox("TransferRegWizInformation",lpInfo);
		TransmitRegWizInfo(_Module.GetModuleInstance(), lpInfo, TRUE);
#ifndef _UNICODE
		CoTaskMemFree((void *) lpInfo);
#endif
	}


	return S_OK;
}

/*   G E T _   V E R S I O N   */
/*-------------------------------------------------------------------------
    Owner: SteveBu
 
    Returns the RegWiz Version Number
-------------------------------------------------------------------------*/
STDMETHODIMP CRegWizCtrl::get_Version(BSTR * pbstr)
{
	_TCHAR szValue[256];
	
	GetRegWizardVersionString(_Module.GetModuleInstance(),szValue);

#ifndef _UNICODE
	if ((*pbstr = BstrCreateSz(szValue)) == NULL)
#else
	if ((*pbstr = SysAllocString(szValue)) == NULL)
#endif // _UNICODE
		{
		return E_OUTOFMEMORY;
		}

	return S_OK;
}
