/*	File: D:\WACKER\tdll\cnctstub.c (Created: 18-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:39p $
 */

#include <windows.h>
#pragma hdrstop

#include <time.h>

#include "stdtyp.h"
#include "session.h"
#include "cnct.h"
#include "cnct.hh"
#include "tchar.h"

static int WINAPI cnctstub(const HDRIVER hDriver);
static int WINAPI cnctstubQueryStatus(const HDRIVER hDriver);
static int WINAPI cnctstubConnect(const HDRIVER hDriver, const unsigned int uCnctFlags);
static int WINAPI cnctstubGetComSettingsString(const HDRIVER hDriver, LPTSTR pachStr, const size_t cb);
static int WINAPI cnctstubComEvent(const HDRIVER hDriver, const enum COM_EVENTS event);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	cnctStubAll
 *
 * DESCRIPTION:
 *	Stubs all function pointers in a connection handle to a stub
 *	procedure so that the connection handle can function without
 *	a driver.
 *
 * ARGUMENTS:
 *	hhCnct	- private connection handle
 *
 * RETURNS:
 *	void
 *
 */
void cnctStubAll(const HHCNCT hhCnct)
	{
	hhCnct->pfDestroy = cnctstub;
	hhCnct->pfQueryStatus = cnctstubQueryStatus;
	hhCnct->pfConnect = cnctstubConnect;
	hhCnct->pfDisconnect = cnctstubConnect;
	hhCnct->pfComEvent = cnctstubComEvent;
	hhCnct->pfInit = cnctstub;
	hhCnct->pfLoad = cnctstub;
	hhCnct->pfSave = cnctstub;
	hhCnct->pfGetComSettingsString = cnctstubGetComSettingsString;
	return;
	}

/* --- Stub Functions --- */

static int WINAPI cnctstub(const HDRIVER hDriver)
	{
	return CNCT_NOT_SUPPORTED;
	}

static int WINAPI cnctstubQueryStatus(const HDRIVER hDriver)
	{
	return CNCT_NOT_SUPPORTED;
	}

static int WINAPI cnctstubConnect(const HDRIVER hDriver,
		const unsigned int uCnctFlags)
	{
	return CNCT_NOT_SUPPORTED;
	}

static int WINAPI cnctstubGetComSettingsString(const HDRIVER hDriver,
		LPTSTR pachStr, const size_t cb)
	{
	return CNCT_NOT_SUPPORTED;
	}

static int WINAPI cnctstubComEvent(const HDRIVER hDriver,
        const enum COM_EVENTS event)
    {
    return CNCT_NOT_SUPPORTED;
    }
