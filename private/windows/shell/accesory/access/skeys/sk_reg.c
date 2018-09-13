/*--------------------------------------------------------------
 *
 * FILE:			SK_Reg.c
 *
 * PURPOSE:	   	These functions process data to and from the registry
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *--- Includes  --------------------------------------------*/
#include	<windows.h>

#include	"debug.h"
#include	"sk_defs.h"
#include	"sk_comm.h"
#include	"sk_reg.h"

// Private Functions -------------------------------------------

static DWORD OpenRegistry(int User);
static void CloseRegistry();
static void SetRegistryValues();
static void GetRegistryValues();

// Variables  --------------------------------------------

HKEY	hKeyApp;

/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL GetUserValues()
 *
 *	TYPE		Local
 *
 * PURPOSE		Read the registery an collect the data for the current
 *				user.  This Information is then setup in the comm routines.
 *				This is called when someone logs into NT.
 *	
 * INPUTS		User Type Default or Current User
 *
 * RETURNS		TRUE - User wants Serial Keys Enabled
 *				FALSE- User wants Serial Keys Disabled
 *
 *---------------------------------------------------------------*/
BOOL GetUserValues(int User)
{
	DWORD Status;

	DBG_OUT("GetUserValues()");

	if (!(Status = OpenRegistry(User)))	// Did Open Registry Succed?
		return(FALSE);					// No - Fail

	switch (Status)						// What is status?
	{
		// This case should only be true the frist time
		// the registry is opened for the current user.
		case REG_CREATED_NEW_KEY:		// Is this an empty Registry?
			SetRegistryValues(); 		// Yes - Set Default Values
			break;

		case REG_OPENED_EXISTING_KEY:	// Is this an existing Registry?
			GetRegistryValues();  		// Yes - Get Values
			break;
	}
		
	CloseRegistry();
	return(TRUE);
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
BOOL SetUserValues()
{
	DWORD Status;

	DBG_OUT("SetUserValues()");

	if (!(Status = OpenRegistry(REG_USER)))		// Did Open Registry Succed?
		return(FALSE);					// No - Fail

	SetRegistryValues();  				// Set New Values
	CloseRegistry();					// Close Registry
	return(TRUE);
}

/*---------------------------------------------------------------
 *
 *	Local Functions - 
 *
/*---------------------------------------------------------------
 *
 * FUNCTION	DWORD OpenRegistry()
 *
 *	TYPE		Global
 *
 * PURPOSE		Opens the Registry for reading or writing
 *	
 * INPUTS		User Type Default or Current User
 *
 * RETURNS		0  = Failed
 *				>0 = REG_CREATED_NEW_KEY or REG_OPENED_EXISTING_KEY
 *
 *---------------------------------------------------------------*/
static DWORD OpenRegistry(int User)
{
	LONG	ret;
	DWORD	Disposition;

	DBG_OUT(" OpenRegistry()");

	switch (User)
	{
		case REG_USER:				// Current User
			ret =RegCreateKeyEx
				(
					HKEY_CURRENT_USER,
                    TEXT("Control Panel\\Accessibility\\SerialKeys"),
					0,NULL,
					REG_OPTION_NON_VOLATILE,
					KEY_ALL_ACCESS,
					NULL,
					&hKeyApp,
					&Disposition
				);
			break;

		case REG_DEF:				// Default 
			ret =RegCreateKeyEx
				(
					HKEY_USERS,
                    TEXT(".DEFAULT\\Control Panel\\Accessibility\\SerialKeys"),
					0,NULL,
					REG_OPTION_NON_VOLATILE,
					KEY_ALL_ACCESS,
					NULL,
					&hKeyApp,
					&Disposition
				);
			break;

		default:
			ret = FALSE;
			break;
	}

	if (ret != ERROR_SUCCESS)		// Did open succede?
		return(FALSE);				// No -

	return (Disposition);
	
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void CloseRegistry()
 *
 *	TYPE		Global
 *
 * PURPOSE		Closes the Registry for reading or writing
 *	
 * INPUTS		None
 *
 * RETURNS		0  = Failed
 *				>0 = REG_CREATED_NEW_KEY or REG_OPENED_EXISTING_KEY
 *
 *---------------------------------------------------------------*/
static void CloseRegistry()
{
	DBG_OUT(" CloseRegistry()");
	RegCloseKey(hKeyApp);
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
static void SetRegistryValues()
{
	long ret;
	DWORD dwFlags;

	DBG_OUT(" SetRegistryValues()");

	dwFlags = skNewKey.dwFlags | SERKF_AVAILABLE;
	ret = RegSetValueEx(				// Write dwFlags
			hKeyApp,
			REG_FLAGS,
			0,REG_DWORD,
			(CONST LPBYTE) &dwFlags,
			sizeof(DWORD));
				
	if (ret != ERROR_SUCCESS)		// Did open succede?
	{
		DBG_ERR("Unable to Set Registry Value");
		return;						// No -
	}


	if (NULL == skNewKey.lpszActivePort)
	{
		ret = RegSetValueEx(			// Write Active Port
				hKeyApp,
				REG_ACTIVEPORT,
				0,
				REG_SZ,
				(CONST LPBYTE) TEXT(""),
				1 * sizeof(*skNewKey.lpszActivePort)); // size of one char, the term null
	}
	else
	{
		ret = RegSetValueEx(			// Write Active Port
				hKeyApp,
				REG_ACTIVEPORT,
				0,
				REG_SZ,
				(CONST LPBYTE) skNewKey.lpszActivePort,
				(lstrlen(skNewKey.lpszActivePort) + 1) * 
					sizeof(*skNewKey.lpszActivePort));
	}			
	if (ret != ERROR_SUCCESS)		// Did open succede?
		return;						// No -

	if (NULL == skNewKey.lpszPort)
	{
		ret = RegSetValueEx(			// Write Active Port
				hKeyApp,
				REG_PORT,
				0,
				REG_SZ,
				(CONST LPBYTE) TEXT(""),
				1 * sizeof(*skNewKey.lpszPort)); // size of one char, the term null
	}
	else
	{
		ret = RegSetValueEx(			// Write Active Port
				hKeyApp,
				REG_PORT,
				0,
				REG_SZ,
				(CONST LPBYTE)skNewKey.lpszPort,
				(lstrlen(skNewKey.lpszPort) + 1) * sizeof(*skNewKey.lpszPort));
	}
					
	if (ret != ERROR_SUCCESS)		// Did open succede?
		return;						// No -

	ret = RegSetValueEx				// Write Active Port
		(
			hKeyApp,
			REG_BAUD,
			0,REG_DWORD,
			(CONST LPBYTE) &skNewKey.iBaudRate,
			sizeof(skNewKey.iBaudRate)
		);
				
	if (ret != ERROR_SUCCESS)		// Did open succede?
		return;						// No -
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void GetRegistryValues()
 *
 *	TYPE		Global
 *
 * PURPOSE		Reads the values in the SerialKeys structure to
 *				the Registry.
 *	
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void GetRegistryValues()
{
	long lRet;
	DWORD dwType;
	DWORD cbData;
	
	DBG_OUT(" GetRegistryValues()");

	skNewKey.dwFlags = 0;
	cbData = sizeof(skNewKey.dwFlags);
	lRet = RegQueryValueEx(
			hKeyApp,
			REG_FLAGS,
			0,&dwType,
			(LPBYTE)&skNewKey.dwFlags,
			&cbData);
				
	skNewKey.dwFlags |= SERKF_AVAILABLE;
    
	if (NULL != skNewKey.lpszActivePort)
	{
		cbData = MAX_PATH * sizeof(*skNewKey.lpszActivePort);
		lRet = RegQueryValueEx(
				hKeyApp,
				REG_ACTIVEPORT,
				0,&dwType,
				(LPBYTE)skNewKey.lpszActivePort,
				&cbData);
					
		if (lRet != ERROR_SUCCESS)
		{
			lstrcpy(skNewKey.lpszActivePort, TEXT("COM1"));
		}
	}

	if (NULL != skNewKey.lpszPort)
	{
		cbData = MAX_PATH * sizeof(*skNewKey.lpszPort);
		lRet = RegQueryValueEx(				// Write Active Port
				hKeyApp,
				REG_PORT,
				0,&dwType,
				(LPBYTE)skNewKey.lpszPort,
				&cbData);
					
		if (lRet != ERROR_SUCCESS)
		{
			lstrcpy(skNewKey.lpszPort, TEXT("COM1"));
		}
	}

	cbData = sizeof(skNewKey.iBaudRate);
	lRet = RegQueryValueEx(			// Write Active Port
			hKeyApp,
			REG_BAUD,
			0,&dwType,
			(LPBYTE)&skNewKey.iBaudRate,
			&cbData);
				
	if (ERROR_SUCCESS != lRet)
	{
		skNewKey.iBaudRate = 300;
	}

}
