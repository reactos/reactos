
/*
author: Boudewijn Dekker
original source : wine
todo: improve debug info
*/

#include <thread.h>


WINBASEAPI 
BOOL 
WINAPI 
SwitchToThread( 
    VOID 
    )
{
	return NtYieldExecution();
}

/* (WIN32) Thread Local Storage ******************************************** */

DWORD	WINAPI
TlsAlloc(VOID)
{
	DWORD 	dwTlsIndex = GetTeb()->dwTlsIndex;
	void	**TlsData = GetTeb()->TlsData;
	
	APISTR((LF_API, "TlsAlloc: (API)\n"));
	if (dwTlsIndex < sizeof(TlsData) / sizeof(TlsData[0]))
	{
		TlsData[dwTlsIndex] = NULL;
		return (dwTlsIndex++);
	}
	return (0xFFFFFFFFUL);
}

BOOL	WINAPI
TlsFree(DWORD dwTlsIndex)
{
	APISTR((LF_APISTUB, "TlsFree(DWORD=%ld)\n", dwTlsIndex));
	return (TRUE);
}

LPVOID	WINAPI
TlsGetValue(DWORD dwTlsIndex)
{
	
	DWORD 	dwTlsIndex = GetTeb()->dwTlsIndex;
	void	**TlsData = GetTeb()->TlsData;

	APISTR((LF_API, "TlsGetValue: (API) dwTlsIndex %ld\n", dwTlsIndex));
	if (dwTlsIndex < sizeof(TlsData) / sizeof(TlsData[0]))
	{
		LOGSTR((LF_LOG, "TlsGetValue: (LOG) [%ld] = %p\n",
			dwTlsIndex, TlsData[dwTlsIndex]));
		SetLastError(NO_ERROR);
		return (TlsData[dwTlsIndex]);
	}
	SetLastErrorEx(1, 0);
	return (NULL);
}

BOOL	WINAPI
TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
	
	DWORD 	dwTlsIndex = GetTeb()->dwTlsIndex;
	void	**TlsData = GetTeb()->TlsData;

	APISTR((LF_API, "TlsSetValue: (API) dwTlsIndex %ld lpTlsValue %p\n",
		dwTlsIndex, lpTlsValue));
	if (dwTlsIndex < sizeof(TlsData) / sizeof(TlsData[0]))
	{
		LOGSTR((LF_LOG, "TlsSetValue: (LOG) [%ld] = %p\n",
			dwTlsIndex, lpTlsValue));
		TlsData[dwTlsIndex] = lpTlsValue;
		return (TRUE);
	}
	return (FALSE);
}

/*************************************************************/