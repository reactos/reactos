/*	File: D:\WACKER\tdll\cnct.hh (Created: 10-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:36p $
 */

typedef struct stCnctPrivate *HHCNCT;

struct stCnctPrivate
	{
	HSESSION hSession;
	CRITICAL_SECTION csCnct;		// for snychronizing access
	HDRIVER hDriver;				// baby you can drive my connect
	HMODULE hModule;				// driver's lib module handle
	TCHAR achDllName[256];			// name of file containing driver

	time_t	tStartTime;				// Start time

	int (WINAPI *pfDestroy)(const HDRIVER hDriver);
	int (WINAPI *pfQueryStatus)(const HDRIVER hDriver);
	int (WINAPI *pfConnect)(const HDRIVER hDriver, const unsigned int uFlags);
	int (WINAPI *pfDisconnect)(const HDRIVER hDriver, const unsigned int uFlags);
	int (WINAPI *pfComEvent)(const HDRIVER hDriver, const enum COM_EVENTS event);
	int (WINAPI *pfInit)(const HDRIVER hDriver);
	int (WINAPI *pfLoad)(const HDRIVER hDriver);
	int (WINAPI *pfSave)(const HDRIVER hDriver);
	int (WINAPI *pfSetDestination)(const HDRIVER hDriver, TCHAR *const ach, const size_t cb);
	int (WINAPI *pfGetComSettingsString)(const HDRIVER hDriver, LPTSTR pachStr, const size_t cb);
	};

void cnctLock(const HHCNCT hhCnct);
void cnctUnlock(const HHCNCT hhCnct);
void cnctStubAll(const HHCNCT hhCnct);
