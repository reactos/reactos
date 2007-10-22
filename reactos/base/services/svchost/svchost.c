/*
 * PROJECT:			ReactOS SvcHost
 * LICENSE:			GPL - See COPYING in the top level directory
 * FILE:			/base/services/svchost/svchost.c
 * PURPOSE:			Provide dll service loader
 * PROGRAMMERS:		Gregor Brunmar (gregor.brunmar@home.se)
 */

/* INCLUDES ******************************************************************/

#include "svchost.h"
#include <debug.h>

#ifdef _MSC_VER
#undef DPRINT1
#define DPRINT1
#endif

/* DEFINES *******************************************************************/

static LPCTSTR SVCHOST_REG_KEY	= _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SvcHost");
static LPCTSTR SERVICE_KEY		= _T("SYSTEM\\CurrentControlSet\\Services\\");
static LPCTSTR PARAMETERS_KEY	= _T("\\Parameters");

#define SERVICE_KEY_LENGTH	_tcslen(SERVICE_KEY);
#define REG_MAX_DATA_SIZE	2048

static PSERVICE FirstService = NULL;

/* FUNCTIONS *****************************************************************/

BOOL PrepareService(LPCTSTR ServiceName)
{
	HKEY hServiceKey;
	TCHAR ServiceKeyBuffer[MAX_PATH + 1];
	DWORD LeftOfBuffer = sizeof(ServiceKeyBuffer) / sizeof(ServiceKeyBuffer[0]);
	DWORD KeyType;
	PTSTR Buffer = NULL;
	DWORD BufferSize = MAX_PATH + 1;
	LONG RetVal;
	HINSTANCE hServiceDll;
	TCHAR DllPath[MAX_PATH + 2]; /* See MSDN on ExpandEnvironmentStrings() for ANSI strings for more details on + 2 */
	LPSERVICE_MAIN_FUNCTION ServiceMainFunc;
	PSERVICE Service;

	/* Compose the registry path to the service's "Parameter" key */
	_tcsncpy(ServiceKeyBuffer, SERVICE_KEY, LeftOfBuffer);
	LeftOfBuffer -= _tcslen(SERVICE_KEY);
	_tcsncat(ServiceKeyBuffer, ServiceName, LeftOfBuffer);
	LeftOfBuffer -= _tcslen(ServiceName);
	_tcsncat(ServiceKeyBuffer, PARAMETERS_KEY, LeftOfBuffer);
	LeftOfBuffer -= _tcslen(PARAMETERS_KEY);

	if (LeftOfBuffer < 0)
	{
		DPRINT1("Buffer overflow for service name: '%s'\n", ServiceName);
		return FALSE;
	}

	/* Open the service registry key to find the dll name */
	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, ServiceKeyBuffer, 0, KEY_READ, &hServiceKey))
	{
		DPRINT1("Could not open service key (%s)\n", ServiceKeyBuffer);
		return FALSE;
	}

	do
	{
		if (Buffer)
			HeapFree(GetProcessHeap(), 0, Buffer);

		Buffer = HeapAlloc(GetProcessHeap(), 0, BufferSize);
		if (NULL == Buffer)
		{
			DPRINT1("Not enough memory for service: %s\n", ServiceName);
			return FALSE;
		}

		RetVal = RegQueryValueEx(hServiceKey, _T("ServiceDll"), NULL, &KeyType, (LPBYTE)Buffer, &BufferSize);

	} while (ERROR_MORE_DATA == RetVal);


	RegCloseKey(hServiceKey);

	if (ERROR_SUCCESS != RetVal || 0 == BufferSize)
	{
		DPRINT1("Could not read 'ServiceDll' value from service: %s, ErrorCode: 0x%x\n", ServiceName, RetVal);
		HeapFree(GetProcessHeap(), 0, Buffer);
		return FALSE;
	}

	/* Convert possible %SystemRoot% to a real path */
	BufferSize = ExpandEnvironmentStrings(Buffer, DllPath, sizeof(DllPath));
	if (0 == BufferSize)
	{
		DPRINT1("Invalid ServiceDll path: %s\n", Buffer);
		HeapFree(GetProcessHeap(), 0, Buffer);
		return FALSE;
	}

	HeapFree(GetProcessHeap(), 0, Buffer);

	/* Load the service dll */
	hServiceDll = LoadLibrary(DllPath);

	if (NULL == hServiceDll)
	{
		DPRINT1("Unable to load ServiceDll: %s\n", DllPath);
		return FALSE;
	}

	ServiceMainFunc = (LPSERVICE_MAIN_FUNCTION)GetProcAddress(hServiceDll, "ServiceMain");

	/* Allocate a service node in the linked list */
	Service = HeapAlloc(GetProcessHeap(), 0, sizeof(SERVICE));
	if (NULL == Service)
	{
		DPRINT1("Not enough memory for service: %s\n", ServiceName);
		return FALSE;
	}

	memset(Service, 0, sizeof(SERVICE));
	Service->Name = HeapAlloc(GetProcessHeap(), 0, _tcslen(ServiceName) + sizeof(TCHAR));
	if (NULL == Service->Name)
	{
		DPRINT1("Not enough memory for service: %s\n", ServiceName);
		HeapFree(GetProcessHeap(), 0, Service);
		return FALSE;
	}
	_tcscpy(Service->Name, ServiceName);

	Service->hServiceDll = hServiceDll;
	Service->ServiceMainFunc = ServiceMainFunc;

	Service->Next = FirstService;
	FirstService = Service;

	return TRUE;
}

VOID FreeServices()
{
	while (FirstService)
	{
		PSERVICE Service = FirstService;
		FirstService = Service->Next;

		FreeLibrary(Service->hServiceDll);

		HeapFree(GetProcessHeap(), 0, Service->Name);
		HeapFree(GetProcessHeap(), 0, Service);
	}
}

/*
 * Returns the number of services successfully loaded from the category
 */
DWORD LoadServiceCategory(LPCTSTR ServiceCategory)
{
	HKEY hServicesKey;
	DWORD KeyType;
	DWORD BufferSize = REG_MAX_DATA_SIZE;
	TCHAR Buffer[REG_MAX_DATA_SIZE];
	LPCTSTR ServiceName;
	DWORD BufferIndex = 0;
	DWORD NrOfServices = 0;

	/* Get all the services in this category */
	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, SVCHOST_REG_KEY, 0, KEY_READ, &hServicesKey))
	{
		DPRINT1("Could not open service category: %s\n", ServiceCategory);
		return 0;
	}

	if (ERROR_SUCCESS != RegQueryValueEx(hServicesKey, ServiceCategory, NULL, &KeyType, (LPBYTE)Buffer, &BufferSize))
	{
		DPRINT1("Could not open service category (2): %s\n", ServiceCategory);
		RegCloseKey(hServicesKey);
		return 0;
	}

	/* Clean up */
	RegCloseKey(hServicesKey);

	/* Load services in the category */
	ServiceName = Buffer;
	while (_T('\0') != ServiceName[0])
	{
		size_t Length;
		
		Length = _tcslen(ServiceName);
		if (0 == Length)
			break;

		if (TRUE == PrepareService(ServiceName))
			++NrOfServices;

		BufferIndex += (Length + 1) * sizeof(TCHAR);

		ServiceName = &Buffer[BufferIndex];
	}

	return NrOfServices;
}

int _tmain (int argc, LPTSTR argv [])
{
	DWORD NrOfServices;
	LPSERVICE_TABLE_ENTRY ServiceTable;
	DWORD i;

	if (argc < 3)
	{
		/* MS svchost.exe doesn't seem to print help, should we? */
		return 1;
	}

	if (_tcscmp(argv[1], _T("-k")) != 0)
	{
		/* For now, we only handle "-k" */
		return 1;
	}

	NrOfServices = LoadServiceCategory(argv[2]);

	if (0 == NrOfServices)
		return 1;

	ServiceTable = HeapAlloc(GetProcessHeap(), 0, sizeof(SERVICE_TABLE_ENTRY) * (NrOfServices + 1));

	if (NULL != ServiceTable)
	{
		PSERVICE Service = FirstService;

		for (i = 0; i < NrOfServices; ++i)
		{
			ServiceTable[i].lpServiceName = Service->Name;
			ServiceTable[i].lpServiceProc = Service->ServiceMainFunc;
			Service = Service->Next;
		}

		StartServiceCtrlDispatcher(ServiceTable);

		HeapFree(GetProcessHeap(), 0, ServiceTable);
	}
	else
	{
		DPRINT1("Not enough memory for the service table, trying to allocate %u bytes\n", sizeof(SERVICE_TABLE_ENTRY) * (NrOfServices + 1));
	}

	FreeServices();

    return 0;
}

/* EOF */
