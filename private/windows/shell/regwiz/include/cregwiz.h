/*********************************************************************
Registration Wizard
CRegWiz.h

11/3/94 - Tracy Ferrier
(c) 1994-95 Microsoft Corporation
**********************************************************************/
#ifndef __CRegWizard__
#define __CRegWizard__

#include <tchar.h>

const kMaxProductCount = 12;
const kSystemInventoryItemCount = 13;
const kMaxCountryCount = 250;

typedef enum
{
	kInfoFirstName = 0,
	kInfoLastName,
	kInfoCompany,
	kInfoMailingAddress,
	kInfoAdditionalAddress,
	kInfoCity,
	kInfoState,
	kInfoZip,
	kInfoCountry,
	kInfoPhoneNumber,
	kInfoDeclinesNonMSProducts,
	kInfoProductID,
	kInfoProcessor,
	kInfoMathCoProcessor,
	kInfoTotalRAM,
	kInfoTotalDiskSpace,
	kInfoRemoveableMedia,
	kInfoDisplayResolution,
	kInfoDisplayColorDepth,
	kInfoPointingDevice,
	kInfoNetwork,
	kInfoModem,
	kInfoSoundCard,
	kInfoCDROM,
	kInfoOperatingSystem,
	kInfoIncludeSystem,
	kInfoIncludeProducts,
	kInfoApplicationName,
	kInfoOEM,
	kInfoVersion,
	kInfoMakerCompany,
	kInfoResultPath,
	kInfoDate,
	kInfoLanguage,
	kInfoEmailName,
	kInfoResellerName,
	kInfoResellerCity,
	kInfoResellerState,
	kInfoHWID,
	kInfoMSID,
	kInfoPhoneExt,
	kDivisionName, // Division name and User Id is added for FE 3/3/98
	kUserId, 
	kAreaCode,
	kHomeSwKnow,
	kHomeExcited,
	kHomePcSwKnow,
	kBusinessRole,
	kScsiAdapterInfo, // System Inventory
	kComputerManufacturer, // Sysinv Dlg , entry
	kMiddleName,
	kInfoLastIndex
}InfoIndex;

#define kFirstSystemIndex	kInfoProcessor
#define kLastSystemIndex	kInfoOperatingSystem

typedef enum
{
	kTriStateFalse,
	kTriStateTrue,
	kTriStateUndefined
}TriState;

// Typedefs for JumpTableElement;  kMaxAddrSpecTableSize can be
// no larger than the largest value representable by JTE
typedef _TUCHAR JTE;
#define kMaxAddrSpecTableSize 255
#define kMaxLenSize 127
#define kAddrSpecCount 19
//typedef _TSCHAR MAXLEN;
typedef char MAXLEN;
typedef struct
{
	MAXLEN maxLen[kAddrSpecCount];
}ADDRSPEC,*PADDRSPEC;

typedef enum
{
	kAddrSpecFirstName = 0,
	kAddrSpecLastName,
	kAddrSpecCompanyName,
	kAddrSpecAddress,
	kAddrSpecAddress2,
	kAddrSpecCity,
	kAddrSpecState,
	kAddrSpecPostalCode,
	kAddrSpecPhone,
	kAddrSpecEmailName,
	kAddrSpecResellerName,
	kAddrSpecResellerCity,
	kAddrSpecResellerState,
	kAddrSpecExtension,
	kAddrSpecDivision,
	kAddrSpecAreaCode,
	kAddrSpecUserId,
	kSIComputerManufacturer,
	kAddrMiddleName
}ADDRSPEC_FIELD;

class CRegWizard
{
public:
	CRegWizard(HINSTANCE hInstance, LPTSTR szParamRegKey);
	virtual ~CRegWizard();

	void StartRegWizardDialog(int wDlgResID, DLGPROC lpDialogProc);
	void ActivateRegWizardDialog( void );
	INT_PTR ProcessRegWizardDialog( void );
	void EndRegWizardDialog(INT_PTR wExitID);
	BOOL IsDialogActive( void );
	
	void SetWindowCaption(LPTSTR lpszWindowsCaption);
	LPTSTR GetWindowCaption();
	
	INT_PTR GetDialogExitButton( void );
	
	BOOL GetInputParameterStatus( void );
	BOOL GetInputParameterString(short paramID, LPTSTR szParam);

	BOOL IsRegistered( void );
	int AddProduct(LPTSTR szProductName,LPTSTR szProductPath);

	void SetInformationString(InfoIndex index, LPTSTR szInfo);
	BOOL GetInformationString(InfoIndex index, LPTSTR szInfo);
	void SetTriStateInformation(InfoIndex index, TriState infoValue);
	TriState GetTriStateInformation(InfoIndex index);
	void WriteEnableInformation(InfoIndex index, BOOL shouldWrite);
	BOOL IsInformationWriteEnabled(InfoIndex index);

	TriState GetProductSearchLibraryStatus( void );
	BOOL GetProductSearchProcAddress(FARPROC* lpfnProductSearch);
	void SetProductSearchStatus(TriState searchCompleted);
	TriState GetProductSearchStatus( void );
	void SetSystemInventoryStatus(BOOL invCompleted);
	BOOL GetSystemInventoryStatus( void );

	void GetProductName(LPTSTR szProductName,INT_PTR index);
	HICON GetProductIcon(INT_PTR index);
	short GetProductCount( void );
	void WriteInfoToRegistry( void );
	BOOL GetInfoRegValueName(InfoIndex infoIndex,LPTSTR szValueName);
	void GetInfoRegistrationParentKey(LPTSTR szRegKey);
	HINSTANCE GetInstance( void );
	void SetCountryCode(DWORD countryCode);
	DWORD GetCountryCode( void );
	void GetCountryAddrSpec(LONG lCountryID,ADDRSPEC* addrSpec );
	void GetAddrSpecProperties(DWORD dwCountryCode, ADDRSPEC_FIELD addrSpecField, MAXLEN* maxLen,BOOL* isRequired);
	void ConfigureEditTextField(HWND hwndDlg,int editFieldID,ADDRSPEC_FIELD addrSpecField,int iAttachedStrID);
	void SetLogFileName(LPTSTR lpszLogFilePath);
    void DestroyOpenedWindow()
	{
		if (m_hwndCurrDialog)
		{
			DestroyWindow(m_hwndCurrDialog);
			m_hwndCurrDialog = NULL;
		}

	}
	void GetRegKey(LPTSTR szRegKey);
	HWND GetCurrDialog(void );
	void SetDialogHide(HWND hDialogToHide);
	BOOL ShowPrevDialog(void);
	void SetPrevDialog(void);
	void SetDialogExitButton( int nButton );
	static BOOL IsEditTextFieldValid(HWND hwndDlg,int editID);
	static void GetEditTextFieldAttachedString(HWND hwndDlg,int editID,LPTSTR szAttached,int cbBufferSize);
	void ResolveCurrentCountryCode( void );
	BOOL GetMSIDfromCookie(LPTSTR);

	_TCHAR 		m_szParamRegKey[256];

private:
	void BuildAddrSpecTables( void );
	void ReadInfoFromRegistry( void );
	void CreateLogFile( void );
	void WriteToLogFile(LPTSTR lpszLine);
	void CloseLogFile( void );

	HINSTANCE 	m_hInstance;
	HACCEL      m_hAccel;
	LPTSTR 		m_rgszProductName[kMaxProductCount];
	LPTSTR 		m_rgszProductPath[kMaxProductCount];
	HICON		m_rghProductIcon[kMaxProductCount];
	LPTSTR 		m_rgszInfoArray[kInfoLastIndex];
	BOOL 		m_writeEnable[kInfoLastIndex];
	short 		m_productNameCount;
	TriState 	m_searchCompleted;
	BOOL 		m_systemInventoryCompleted;
	_TCHAR 		m_szWindowsCaption[256];
	_TCHAR 		m_szInfoParentKey[256];
	_TCHAR		m_szLogFilePath[_MAX_PATH];
	FARPROC 	m_lpfnProductSearch;
	DWORD 		m_countryCode;
	HANDLE		m_addrJumpTable;
	HANDLE		m_addrSpecTable;
	HANDLE 		m_hLogFile;

	BOOL		m_dialogActive;
	HWND		m_hwndStartDialog;
	HWND		m_hwndCurrDialog;
	INT_PTR		m_wDialogExitButton;
	HWND		m_hwndDialogToHide;
	HWND		m_hwndPrevDialog;
	HINSTANCE	m_ccpLibrary; 
};
	
#endif
