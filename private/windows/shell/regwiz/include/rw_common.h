#ifndef __RW_COMMON__
#define __RW_COMMON__

#include <windows.h>
#include  <stdio.h>
//#include <iostream.h>
//#include <fstream.h>
#include <tchar.h>
#include <wininet.h>

#ifdef __cplusplus
extern "C" 
{
#endif

int  InvokeRegistration ( HINSTANCE hInstance  , LPCTSTR  czPath);
void TransmitRegWizInfo ( HINSTANCE hInstance , LPCTSTR szParams,BOOL fOEM);
void DisplayInterNetConnectingMsg(HINSTANCE hIns);
void CloseDisplayInetrnetConnecting();
DWORD CheckWithDisplayInterNetConnectingMsg(HINSTANCE hIns);

void SetMSID(HINSTANCE hInstance);
BOOL GetMSIDfromRegistry(HINSTANCE hInstance,LPTSTR szValue);
BOOL GetMSIDfromCookie(HINSTANCE hInstance,LPTSTR szMSID);

DWORD_PTR GetProxyAuthenticationInfo(HINSTANCE hIns,TCHAR *czProxy,
								 TCHAR *czUserName,TCHAR *czPswd);
void  RemoveMSIDEntry(HINSTANCE hIns);

#ifdef __cplusplus
}
#endif

TCHAR * GetModemDeviceInformation(HINSTANCE hIns, int iModemIndex);
//
// Status returend while finding the removable media and cdrom
//
#define     REGFIND_ERROR      1
#define     REGFIND_RECURSE    2
#define     REGFIND_FINISH     3

//
// Status returend while Cheching for InternetConnection
//
#define     DIALUP_NOT_REQUIRED      1
#define     DIALUP_REQUIRED          2
//#define     CONNECTION_CANNOT_BE_ESTABLISHED  3

//
// HTTP Post related status messages
//
#define  RWZ_NOERROR  0
#define  RWZ_NO_INFO_AVAILABLE			1
#define  RWZ_INVALID_INFORMATION		2 
#define  RWZ_BUFFER_SIZE_INSUFFICIENT	3
#define  RWZ_INTERNAL_ERROR				4 // Internal Programming Error 
#define  RWZ_POST_SUCCESS               5
#define  RWZ_POST_FAILURE               6    
#define  RWZ_POST_WITH_SSL_FAILURE      7    
#define  RWZ_POST_MSN_SITE_BUSY         8   
#define  RWZ_ERROR_NOTCPIP              9
//
//  Error Values returned by the Signup Dialogue
//

#define  RWZ_ERROR_LOCATING_MSN_FILES       10
#define  RWZ_ERROR_LOCATING_DUN_FILES       11 
#define  RWZ_ERROR_MODEM_IN_USE             12  
#define  RWZ_ERROR_MODEM_CFG_ERROR			13 
#define  RWZ_ERROR_TXFER_CANCELLED_BY_USER	14  
#define  RWZ_ERROR_CANCELLED_BY_USER		14 
#define  RWZ_ERROR_SYSTEMERROR				15
#define  RWZ_ERROR_NODIALTONE				16
// Environment Not proper 
#define  RWZ_ERROR_MODEM_NOT_FOUND		   17
#define  RWZ_ERROR_NO_ANSWER               18    // no response engaged tone
#define  RWZ_ERROR_RASDLL_NOTFOUND         19

#define     CONNECTION_CANNOT_BE_ESTABLISHED  20 // Mdem cfg error
// Error in Invoking 
#define  RWZ_ERROR_INVALID_PARAMETER	    30
#define  RWZ_ERROR_INVALID_DLL              31
#define  REGWIZ_ALREADY_CONFIGURED			32 
#define  RWZ_ERROR_PREVIOUSCOPY_FOUND       33 

#define  RWZ_ERROR_REGISTERLATER            34   


// Status of Ping

#define   RWZ_PINGSTATUS_NOTCPIP    40
#define   RWZ_PINGSTATUS_SUCCESS    41
#define   RWZ_PINGSTATUS_FAIL       42





//
//  The below defines  is for creating a Log File 
//
#define _LOG_IN_FILE                 //	 uses a file 

class RWDebug {
public:
	RWDebug() {
		m_iError = 0;
		fp       = NULL;
	};
	~RWDebug(){};
	void     UseStandardOutput();
	void     CreateLogFile(char *czFile);
	inline  RWDebug& operator<<(RWDebug& (__cdecl * _f)(RWDebug&));
	RWDebug& operator <<( int  iv) ;
	RWDebug& operator <<( unsigned int  iv) ;
	RWDebug& operator <<( short sv) ;
	RWDebug& operator <<( unsigned short usv) ;
	RWDebug& operator <<( unsigned short *usv) ;
	RWDebug& operator <<( void *  pVoid) ;
	RWDebug& operator <<( long  lv) ;
	RWDebug& operator <<( unsigned long ulv) ;
	RWDebug& operator <<( float  fv) ;
	RWDebug& operator <<( char   cv) ;
	//RWDebug& operator <<( bool  bv) ;
	RWDebug& operator <<( char *  sv) ;
	RWDebug& operator <<( unsigned char *  sv) ;
	RWDebug& operator <<( const char *  sv) ;
	RWDebug& flush() {return *this;};
	RWDebug& Write (char *czT); 
private :
	FILE *fp;
	char czTemp[48];
	int m_iError;
	

};
inline RWDebug& RWDebug::operator<<(RWDebug& (__cdecl * _f)(RWDebug&)) { (*_f)(*this); return *this; }
inline RWDebug& __cdecl flush(RWDebug& _outs) { return _outs.flush(); }
inline RWDebug& __cdecl endl(RWDebug& _outs) { return _outs << '\n' << flush; }

//ostream &GetDebugLogStream();
RWDebug &GetDebugLogStream();

REGSAM RW_GetSecDes() ;
int  GetProductRoot (TCHAR * pPath , PHKEY  phKey);

#define RW_DEBUG  GetDebugLogStream()



/* 
 Function name	: RegFindValueInAllSubKey
 Description	: It searches the subkey for the presence of the ValueName "Type" which has 
				  value given by szValueToFind.It returns the value of "Identifier" 
				  ValueName present along with the type in szIdentifier.
 Return Value	: TRUE if Successful else FALSE
*/
int RegFindValueInAllSubKey(HINSTANCE hInstance,HKEY key ,LPCTSTR szSubKeyNameToFind,LPCTSTR szValueToFind,LPTSTR szIdentifier,int nType );

/* 
 Function name	: RegFindTheSubKey
 Description	: Finds the key specified within the subkey. 
 Return Value	: TRUE if Successful else FALSE 
*/
BOOL RegFindTheSubKey(HKEY key,LPCTSTR szSubKeyName,LPCTSTR szSubKeyNameToFind,LPTSTR szData);
/*
 Function name	: RegGetPointingDevice
 Description	: Get the pointing device. 
 Return Value	: TRUE if Successful else FALSE 
*/
BOOL RegGetPointingDevice(HKEY hKey,LPCTSTR szSubKeyName,LPTSTR szData);


LPCTSTR BstrToSz(BSTR pszW);

#ifdef _UNICODE
	TCHAR* ConvertToUnicode(char FAR* szA);
#else
	TCHAR * ConvertToUnicode(TCHAR * szW) ;
#endif

char * ConvertToANSIString (LPCTSTR  szW);

int IsDialupConnectionActive();

HRESULT GetNewGUID(PSTR pszGUID);
//
//
//  Internet connection settings related  function
//  define ATK_INET.CPP  
int  DisableAutoDial(); // Disables Auto Dial
int  ResetAutoDialConfiguration();
void GetAutoDialConfiguration();
void UnLoadInetCfgLib();
//
//
//  product Info related function
void SetProductBeingRegistred(TCHAR *szProduct);
TCHAR *GetProductBeingRegistred();

//
// OEM DLL Validation 
#define OEM_NO_ERROR		0
#define OEM_VALIDATE_FAILED 1
#define OEM_INTERNAL_ERROR  2
int CheckOEMdll(); // This checks if Registration is with OEM
int GetOemManufacturer (TCHAR *szProductregKey, TCHAR *szBuf );
#endif
