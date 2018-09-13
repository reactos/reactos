//-----------------------------------------------------------------------------
// RNAAPI class
// 
// This class provides a series of cover function for the RNAPH/RASAPI32 dlls
//
// Created 1-29-96	ChrisK

// ############################################################################
// INCLUDES
#include "pch.hpp"
//#include "ras.h"
#include <ras.h>
#pragma pack (4)
//#if !defined(WIN16)
//#include <rnaph.h>
//#endif
#pragma pack ()
#include "rnaapi.h"
#include "debug.h"

// ############################################################################
// RNAAPI class 
CRNAAPI::CRNAAPI()
{
	m_hInst = LoadLibrary("RASAPI32.DLL");
	m_hInst2 = LoadLibrary("RNAPH.DLL");

	m_fnRasEnumDeviecs = NULL;
	m_fnRasValidateEntryName = NULL;
	m_fnRasSetEntryProperties = NULL;
	m_fnRasGetEntryProperties = NULL;
}

// ############################################################################
CRNAAPI::~CRNAAPI()
{
	// Clean up
	if (m_hInst) FreeLibrary(m_hInst);
	if (m_hInst2) FreeLibrary(m_hInst2);
}

// ############################################################################
DWORD CRNAAPI::RasEnumDevices(LPRASDEVINFO lpRasDevInfo, LPDWORD lpcb,
							 LPDWORD lpcDevices)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi("RasEnumDevices",(FARPROC*)&m_fnRasEnumDeviecs);

	if (m_fnRasEnumDeviecs)
		dwRet = (*m_fnRasEnumDeviecs) (lpRasDevInfo, lpcb, lpcDevices);

	return dwRet;
}

// ############################################################################
BOOL CRNAAPI::LoadApi(LPSTR pszFName, FARPROC* pfnProc)
{
	if (*pfnProc == NULL)
	{
		// Look for the entry point in the first DLL
		if (m_hInst)
			*pfnProc = GetProcAddress(m_hInst,pszFName);
		
		// if that fails, look for the entry point in the second DLL
		if (m_hInst2 && !(*pfnProc))
			*pfnProc = GetProcAddress(m_hInst2,pszFName);
	}

	return (pfnProc != NULL);
}

// ############################################################################
DWORD CRNAAPI::RasValidateEntryName(LPSTR lpszPhonebook,LPSTR lpszEntry)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi("RasValidateEntryName",(FARPROC*)&m_fnRasValidateEntryName);

	if (m_fnRasValidateEntryName)
		dwRet = (*m_fnRasValidateEntryName) (lpszPhonebook, lpszEntry);

	return dwRet;
}

// ############################################################################
DWORD CRNAAPI::RasSetEntryProperties(LPSTR lpszPhonebook, LPSTR lpszEntry,
									LPBYTE lpbEntryInfo, DWORD dwEntryInfoSize,
									LPBYTE lpbDeviceInfo, DWORD dwDeviceInfoSize)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi("RasSetEntryProperties",(FARPROC*)&m_fnRasSetEntryProperties);

	if (m_fnRasSetEntryProperties)
		dwRet = (*m_fnRasSetEntryProperties) (lpszPhonebook, lpszEntry,
									lpbEntryInfo, dwEntryInfoSize,
									lpbDeviceInfo, dwDeviceInfoSize);

	return dwRet;
}

// ############################################################################
DWORD CRNAAPI::RasGetEntryProperties(LPSTR lpszPhonebook, LPSTR lpszEntry,
									LPBYTE lpbEntryInfo, LPDWORD lpdwEntryInfoSize,
									LPBYTE lpbDeviceInfo, LPDWORD lpdwDeviceInfoSize)
{
	DWORD dwRet = ERROR_DLL_NOT_FOUND;

	// Look for the API if we haven't already found it
	LoadApi("RasGetEntryProperties",(FARPROC*)&m_fnRasGetEntryProperties);

	if (m_fnRasGetEntryProperties)
		dwRet = (*m_fnRasGetEntryProperties) (lpszPhonebook, lpszEntry,
									lpbEntryInfo, lpdwEntryInfoSize,
									lpbDeviceInfo, lpdwDeviceInfoSize);

	return dwRet;
}
