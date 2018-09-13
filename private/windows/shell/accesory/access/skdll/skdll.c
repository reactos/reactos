/*--------------------------------------------------------------
 *
 * FILE:			SKEYDLL.C
 *
 * PURPOSE:		    The file contains the SerialKeys DLL Functions
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *------------------------------------------------------------*/

#include	"windows.h"
#include	"..\skeys\sk_dllif.h"
#include	"..\skeys\sk_dll.h"
#include	"..\skeys\sk_reg.h"
#include	"..\access\skeys.h"

#define ARRAY_SIZE(a)      (sizeof(a) / sizeof((a)[0]))

#define RUNNINGEVENT TEXT("SkeysRunning")

static BOOL SerialKeysInstall(void);
static BOOL IsSerialKeysRunning();
static BOOL IsServiceStartAllowed();
static BOOL WaitForServiceRunning();


/*---------------------------------------------------------------
 *
 * FUNCTION	int APIENTRY LibMain
 *
 *	TYPE		Global
 *
 * PURPOSE		LibMain is called by Windows when
 *				the DLL is initialized, Thread Attached, and other times.
 *				Refer to SDK documentation, as to the different ways this
 *				may be called.
 *
 *				The LibMain function should perform additional initialization
 *				tasks required by the DLL.  In this example, no initialization
 *				tasks are required.  LibMain should return a value of 1 if
 *				the initialization is successful.
 *
 * INPUTS	 
 *
 * RETURNS		TRUE - Transfer Ok
 *				FALSE- Transfer Failed
 *
 *---------------------------------------------------------------*/
INT  APIENTRY LibMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	return 1;

	UNREFERENCED_PARAMETER(hInst);
	UNREFERENCED_PARAMETER(ul_reason_being_called);
	UNREFERENCED_PARAMETER(lpReserved);
}


/*---------------------------------------------------------------
 *
 * FUNCTION	void SkeyGetRegistryValues()
 *
 *	TYPE		Global
 *
 * PURPOSE		Reads the values from the registry into the 
 *              SerialKeys structure
 *	
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static BOOL SkeyGetRegistryValues(HKEY hkey, LPSERIALKEYS psk)
{
	LONG lErr;
	DWORD dwType;
	DWORD cbData;
	
	psk->iPortState = 0;
	psk->iActive = 0;

	psk->dwFlags = 0;
    cbData = sizeof(psk->dwFlags);

	lErr = RegQueryValueEx(
			hkey,
			REG_FLAGS,
			0,
			&dwType,
			(LPBYTE)&psk->dwFlags,
			&cbData);
	
	psk->dwFlags |= SERKF_AVAILABLE;

	if (NULL != psk->lpszActivePort)
	{
		cbData = MAX_PATH * sizeof(*psk->lpszActivePort);
		lErr = RegQueryValueEx(
				hkey,
				REG_ACTIVEPORT,
				0,
				&dwType,
				(LPBYTE)psk->lpszActivePort,
				&cbData);

		if (ERROR_SUCCESS != lErr)
		{
			lstrcpy(psk->lpszActivePort, TEXT("COM1"));
		}
	}

	if (NULL != psk->lpszPort)
	{
        cbData = MAX_PATH * sizeof(*psk->lpszPort);
		lErr = RegQueryValueEx(
				hkey,
				REG_PORT,
				0,
				&dwType,
				(LPBYTE)psk->lpszPort,
				&cbData);

		if (ERROR_SUCCESS != lErr)
		{
			lstrcpy(psk->lpszPort, TEXT("COM1"));
		}
    }				

    cbData = sizeof(psk->iBaudRate);
	lErr = RegQueryValueEx(
			hkey,
			REG_BAUD,
			0,&dwType,
			(LPBYTE)&psk->iBaudRate,
			&cbData);

	if (ERROR_SUCCESS != lErr)
	{
		psk->iBaudRate = 300;
	}
				
	return TRUE;
}


/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL SkeyGetUserValues()
 *
 *	TYPE		Local
 *
 * PURPOSE		Read the registery an collect the data for the current
 *				user.
 *	
 * RETURNS		TRUE - User wants Serial Keys Enabled
 *				FALSE- User wants Serial Keys Disabled
 *
 *---------------------------------------------------------------*/
BOOL SkeyGetUserValues(LPSERIALKEYS psk)
{
	BOOL fOk = FALSE;
    HKEY  hkey;
	DWORD dwRet;
    DWORD dwDisposition;

     dwRet = RegCreateKeyEx( HKEY_CURRENT_USER,
            TEXT("Control Panel\\Accessibility\\SerialKeys"),
            0,
            NULL,                // CLASS NAME??
            0,                   // by default is non-volatile
            KEY_READ,
            NULL,                // default security descriptor
            &hkey,
            &dwDisposition);    // yes we throw this away

    if (ERROR_SUCCESS == dwRet)
	{
		fOk = SkeyGetRegistryValues(hkey, psk);
		RegCloseKey(hkey);
	}
	if (fOk)
	{
		// Not available unless the service is running or this
		// user can start it
		if (IsSerialKeysRunning() || IsServiceStartAllowed())
		{
			psk->dwFlags |= SERKF_AVAILABLE;
		}
		else
		{
			psk->dwFlags &= ~SERKF_AVAILABLE;
		}
	}
	return(fOk);
}


/*---------------------------------------------------------------
 *
 * FUNCTION	void SetRegistryValues()
 *
 *	TYPE		Global
 *
 * PURPOSE		Writes the values in the SerialKeys structure to
 *				the Registry.
 *	
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static BOOL SkeySetRegistryValues(HKEY hkey, LPSERIALKEYS psk)
{
	LONG lErr;
	BOOL fOk;
	DWORD dwFlags;

	dwFlags = psk->dwFlags | SERKF_AVAILABLE;
	lErr = RegSetValueEx(			// Write dwFlags
			hkey,
			REG_FLAGS,
			0,REG_DWORD,
			(CONST LPBYTE)&dwFlags,
			sizeof(DWORD));

    fOk = (ERROR_SUCCESS == lErr);

	if (fOk)
	{
		lErr = RegSetValueEx(			// Write Active Port
				hkey,
				REG_ACTIVEPORT,
				0,REG_SZ,
				(CONST LPBYTE) psk->lpszActivePort,
				(NULL == psk->lpszActivePort) ? 0 : 
					(lstrlen(psk->lpszActivePort) + 1) * 
						sizeof(*psk->lpszActivePort));
					
        fOk = (ERROR_SUCCESS == lErr);
    }				

	if (fOk)
	{
		lErr = RegSetValueEx(			// Write Active Port
				hkey,
				REG_PORT,
				0,REG_SZ,
				(CONST LPBYTE)psk->lpszPort,
				(NULL == psk->lpszPort) ? 0 : 
					(lstrlen(psk->lpszPort) + 1) * sizeof(*psk->lpszPort));
					
        fOk = (ERROR_SUCCESS == lErr);
    }				

	if (fOk)
	{
		lErr = RegSetValueEx(			// Write Active Port
				hkey,
				REG_BAUD,
				0,REG_DWORD,
				(CONST LPBYTE) &psk->iBaudRate,
				sizeof(psk->iBaudRate));

        fOk = (ERROR_SUCCESS == lErr);
    }				

	return fOk;
}


/*---------------------------------------------------------------
 *
 * FUNCTION	void SetUserValues()
 *
 *	TYPE		Global
 *
 * PURPOSE		This function writes out information to the
 *				registry.
 *	
 * INPUTS		None
 *
 * RETURNS		TRUE - Write Successful
 *				FALSE- Write Failed
 *
 *---------------------------------------------------------------*/
BOOL SkeySetUserValues(LPSERIALKEYS psk)
{
	BOOL fOk = FALSE;
    HKEY  hkey;
	DWORD dwRet;
    DWORD dwDisposition;

    dwRet = RegCreateKeyEx(
			HKEY_CURRENT_USER,
	        TEXT("Control Panel\\Accessibility\\SerialKeys"),
            0,
            NULL,                // class name
            REG_OPTION_NON_VOLATILE,
            KEY_READ | KEY_WRITE,
            NULL,                // default security descriptor
            &hkey,
            &dwDisposition);    // yes we throw this away

    if (ERROR_SUCCESS == dwRet)
	{
	    fOk = SkeySetRegistryValues(hkey, psk);
		RegCloseKey(hkey);
	}
	return(fOk);
}

#if 0 // This old code is no longer needed ////////////////////////////////////
/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL IsSerialKeysInstalled();
 *
 *	TYPE		Local
 *
 * PURPOSE		This function passes the information from the 
 *				Serial Keys application to the Server
 *
 * INPUTS	 	None
 *
 * RETURNS		TRUE - SerialKeys is Installed
 *				FALSE- SerialKeys Not Installed
 *
 *---------------------------------------------------------------*/
static BOOL IsSerialKeysInstalled()
{

	SC_HANDLE   schService = NULL;
	SC_HANDLE   schSCManager = NULL;

	BOOL fOk = FALSE;
	//
	// Check if the Serial Keys Service is installed
	schSCManager = OpenSCManager
	(
		NULL,                   // machine (NULL == local)
		NULL,                   // database (NULL == default)
		SC_MANAGER_ALL_ACCESS   // access required
	);

	if (NULL != schSCManager)
	{
		schService = OpenService(schSCManager, "SerialKeys", SERVICE_ALL_ACCESS);
			
		if (NULL != schService)
		{
			CloseServiceHandle(schService);
			fOk = TRUE;
        }

		CloseServiceHandle(schSCManager);
    }

	return fOk;
}
#endif ////////////////////////////////////////////////////////////////////////

BOOL IsSerialKeysRunning()
{
	BOOL fRunning = FALSE;
    HANDLE hEventSkeysServiceRunning;

	hEventSkeysServiceRunning = OpenEvent(SYNCHRONIZE, FALSE, RUNNINGEVENT);
    if (NULL != hEventSkeysServiceRunning)
	{
		DWORD dwWait;

		dwWait = WaitForSingleObject(hEventSkeysServiceRunning, 0);

		fRunning = (WAIT_OBJECT_0 == dwWait);
        CloseHandle(hEventSkeysServiceRunning);
    }
	return fRunning;
}


BOOL IsServiceStartAllowed()
{
	BOOL fServiceStartAllowed = FALSE;

	SC_HANDLE   schSCManager = NULL;

	schSCManager = OpenSCManager(   // Open Service Manager
		NULL,                       // machine (NULL == local)
	    NULL,                       // database (NULL == default)
		SC_MANAGER_CREATE_SERVICE);     // access required

	if (NULL != schSCManager)  // Did Open Service succeed?
	{
        CloseServiceHandle(schSCManager);
		fServiceStartAllowed = TRUE;
	}

	return fServiceStartAllowed;
}


BOOL SkeyServiceRequest(UINT uAction, LPSERIALKEYS psk, BOOL fWinIni)
{
	BOOL fOk = FALSE;
    SKEYDLL SKeyDLL;
	DWORD bytesRead;

	if (IsSerialKeysRunning())
	{
		memset(&SKeyDLL, 0, sizeof(SKeyDLL));
		SKeyDLL.Message = uAction;
		if (psk->lpszActivePort != NULL)
		{
			strcpy(SKeyDLL.szActivePort,psk->lpszActivePort);
		}

		if (psk->lpszPort != NULL)
		{
			strcpy(SKeyDLL.szPort,psk->lpszPort);
		}

		SKeyDLL.dwFlags		= psk->dwFlags | SERKF_AVAILABLE;
		SKeyDLL.iBaudRate	= psk->iBaudRate;
		SKeyDLL.iPortState	= psk->iPortState;
		SKeyDLL.iSave 		= fWinIni;

		fOk = CallNamedPipe(
				SKEY_NAME, 						// Pipe name
				&SKeyDLL, 
				sizeof(SKeyDLL),
				&SKeyDLL, 
				sizeof(SKeyDLL),
				&bytesRead, 
				NMPWAIT_USE_DEFAULT_WAIT);

		if (fOk)
		{
			if (psk->lpszActivePort != NULL)
			{
				strcpy(psk->lpszActivePort,SKeyDLL.szActivePort);
			}

			if (psk->lpszPort != NULL)
			{
				strcpy(psk->lpszPort,SKeyDLL.szPort);
			}

			psk->dwFlags 		= SKeyDLL.dwFlags | SERKF_AVAILABLE;	  
			psk->iBaudRate 		= SKeyDLL.iBaudRate; 
			psk->iPortState 	= SKeyDLL.iPortState;
		}
	}
    return fOk;
}


BOOL SkeyInitUser()
{
    BOOL fOk;
	SERIALKEYS sk;
	TCHAR szActivePort[256];
	TCHAR szPort[256];


	memset(&sk, 0, sizeof(sk));
	sk.cbSize = sizeof(sk);
	sk.lpszActivePort = szActivePort;
	sk.lpszPort = szPort;

	fOk = SkeyGetUserValues(&sk);
	if (fOk)
	{
		fOk = SkeyServiceRequest(SPI_SETSERIALKEYS, &sk, FALSE);
	}
	return fOk;
}


/*---------------------------------------------------------------
 *
 * FUNCTION	int APIENTRY SKEY_SystemParameterInfo
 *
 *	TYPE		Global
 *
 * PURPOSE		This function passes the information from the 
 *				Serial Keys application to the Server
 *
 * INPUTS	 
 *
 * RETURNS		TRUE - Transfer Ok
 *				FALSE- Transfer Failed
 *
 *---------------------------------------------------------------*/
BOOL APIENTRY SKEY_SystemParametersInfo(
		UINT uAction, 
		UINT uParam, 
		LPSERIALKEYS psk, 
		BOOL fWinIni)
{
	BOOL fOk;
	BOOL fStarted;

	fOk = ((uAction ==  SK_SPI_INITUSER) || 
		   (NULL != psk && (0 != psk->cbSize)));
    
    if (fOk)
	{
		switch (uAction)			
		{
		case SPI_SETSERIALKEYS:
			fOk = SkeySetUserValues(psk);
            
			if (fOk && (psk->dwFlags & SERKF_SERIALKEYSON) && IsServiceStartAllowed())
			{
				fOk = SerialKeysInstall();
			}

			if (fOk && IsSerialKeysRunning())
			{
	            fOk = SkeyInitUser();
			}
			
			break;

		case SPI_GETSERIALKEYS:	
			fOk = SkeyGetUserValues(psk);
			if (fOk && (psk->dwFlags & SERKF_SERIALKEYSON) && 
				!IsSerialKeysRunning() && IsServiceStartAllowed())
			{
				fOk = SerialKeysInstall();
			}

			if (fOk && IsSerialKeysRunning())
			{
	            fOk = SkeyInitUser();
			}

			break;

        case SK_SPI_INITUSER:
			// give the service a chance to start
			fStarted = WaitForServiceRunning();
			
			if (!fStarted)
			{
				// service does not seem to be running
				// let's try to start it
				fOk = SkeyGetUserValues(psk);

				if (fOk && (psk->dwFlags & SERKF_SERIALKEYSON) && 
					!IsSerialKeysRunning() && IsServiceStartAllowed())
				{
					SerialKeysInstall();
				}

				if (IsSerialKeysRunning())
				{
					fOk = SkeyInitUser();
				}
			}
			break;

		default:
			fOk = FALSE;			// No - Fail
		}
	}
    return fOk;
}

BOOL WaitForServiceRunning()
{
	BOOL fOk = FALSE;
    HANDLE hEventSkeysServiceRunning;
    BYTE abSd[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)abSd;
    SECURITY_ATTRIBUTES sa;

    hEventSkeysServiceRunning = NULL;

	if (InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION))
	{
	    if (SetSecurityDescriptorDacl(pSD,TRUE,NULL,FALSE))
	    {

			sa.nLength = sizeof(sa);
			sa.bInheritHandle = TRUE;
			sa.lpSecurityDescriptor = pSD;

			hEventSkeysServiceRunning = CreateEvent(
				&sa,	// Security
				TRUE,	// Manual reset?
				FALSE,  // initial state - not signaled
				RUNNINGEVENT);  // name
		}
	}
	
    if (NULL != hEventSkeysServiceRunning)
	{
		DWORD dwWait;

		dwWait = WaitForSingleObject(hEventSkeysServiceRunning, 60 * 1000);

		fOk = (WAIT_OBJECT_0 == dwWait);
        CloseHandle(hEventSkeysServiceRunning);
    }
	
	return fOk;

}



/****************************************************************************/

BOOL SerialKeysInstall(void)
{
    BOOL fStarted = FALSE;
	SERVICE_STATUS  ssStatus;
	DWORD   dwOldCheckPoint;
	TCHAR   szFileName[255];

	SC_HANDLE   schService = NULL;
	SC_HANDLE   schSCManager = NULL;

	schSCManager = OpenSCManager(   // Open Service Manager
		NULL,                       // machine (NULL == local)
	    NULL,                       // database (NULL == default)
		SC_MANAGER_ALL_ACCESS);     // access required

	if (NULL != schSCManager)  // Did Open Service succeed?
	{
	    schService = OpenService(
			    schSCManager ,
			    __TEXT("SerialKeys"),
			    SERVICE_ALL_ACCESS);

	    if (NULL != schService)
		{			
			// insure the serivce is auto-start

			ChangeServiceConfig(
				schService,
				SERVICE_WIN32_OWN_PROCESS,
				SERVICE_AUTO_START,		// when to start service 
				SERVICE_NO_CHANGE,		// severity if service fails to start 
				NULL,					// pointer to service binary file name 
				NULL,					// pointer to load ordering group name 
				NULL,					// pointer to variable to get tag identifier 
				NULL,					// pointer to array of dependency names 
				NULL,					// pointer to account name of service 
				NULL,					// pointer to password for service account  
				__TEXT("SerialKeys"));	// name to display 
		}
		else
		{
			GetWindowsDirectory(szFileName, ARRAY_SIZE(szFileName));
			lstrcat(szFileName, __TEXT("\\system32\\skeys.exe"));

			// Is Service File installed?
			if (0xFFFFFFFF != GetFileAttributes(szFileName)) 
			{       
				schService = CreateService(
					schSCManager,               // SCManager database
					__TEXT("SerialKeys"),       // name of service
					__TEXT("SerialKeys"),       // name to display 
					SERVICE_ALL_ACCESS,         // desired access
					SERVICE_WIN32_OWN_PROCESS,  // service type
					SERVICE_AUTO_START,         // start type
					SERVICE_ERROR_NORMAL,       // error control type
					szFileName,                 // service's binary
					NULL,                       // no load ordering group
					NULL,                       // no tag identifier
					NULL,                       // no dependencies
					NULL,                       // LocalSystem account
					NULL);                      // no password
			}
		}
	    if (NULL != schService)
        {
            BOOL fOk = QueryServiceStatus(schService,&ssStatus);
			if (fOk && ssStatus.dwCurrentState != SERVICE_RUNNING)
			{
				static PTSTR pszArg = TEXT("F\0"); // force service to start
				PTSTR apszArg[] = {pszArg, NULL};
				
				if (StartService(schService, 1, apszArg))
				{   
					while(fOk && ssStatus.dwCurrentState != SERVICE_RUNNING)
					{
						dwOldCheckPoint = ssStatus.dwCheckPoint;
						Sleep(max(ssStatus.dwWaitHint, 1000));
						fOk = QueryServiceStatus(schService,&ssStatus);
						fOk = (fOk && (dwOldCheckPoint >= ssStatus.dwCheckPoint));
					}
				}
			}
			fStarted = fOk && (ssStatus.dwCurrentState == SERVICE_RUNNING);
			CloseServiceHandle(schService);
			
			if (fStarted)
			{
				fStarted = WaitForServiceRunning();
			}
        }
        CloseServiceHandle(schSCManager);
    }
    return fStarted;
}
