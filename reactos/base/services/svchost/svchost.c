/*
 * PROJECT:         ReactOS SvcHost
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            /base/services/svchost/svchost.c
 * PURPOSE:         Provide dll service loader
 * PROGRAMMERS:     Gregor Brunmar (gregor.brunmar@home.se)
 */

/* INCLUDES ******************************************************************/

#include "svchost.h"

#define NDEBUG
#include <debug.h>

/* DEFINES *******************************************************************/

static PCWSTR SVCHOST_REG_KEY   = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SvcHost";
static PCWSTR SERVICE_KEY       = L"SYSTEM\\CurrentControlSet\\Services\\";
static PCWSTR PARAMETERS_KEY    = L"\\Parameters";

#define SERVICE_KEY_LENGTH  wcslen(SERVICE_KEY);
#define REG_MAX_DATA_SIZE   2048

static PSERVICE FirstService = NULL;

/* FUNCTIONS *****************************************************************/

BOOL PrepareService(PCWSTR ServiceName)
{
    HKEY hServiceKey;
    WCHAR ServiceKeyBuffer[MAX_PATH + 1];
    DWORD LeftOfBuffer = sizeof(ServiceKeyBuffer) / sizeof(ServiceKeyBuffer[0]);
    DWORD KeyType;
    PWSTR Buffer = NULL;
    DWORD BufferSize = MAX_PATH + 1;
    LONG RetVal;
    HINSTANCE hServiceDll;
    WCHAR DllPath[MAX_PATH + 2]; /* See MSDN on ExpandEnvironmentStrings() for ANSI strings for more details on + 2 */
    LPSERVICE_MAIN_FUNCTION ServiceMainFunc;
    PSERVICE Service;

    /* Compose the registry path to the service's "Parameter" key */
    wcsncpy(ServiceKeyBuffer, SERVICE_KEY, LeftOfBuffer);
    LeftOfBuffer -= wcslen(SERVICE_KEY);
    wcsncat(ServiceKeyBuffer, ServiceName, LeftOfBuffer);
    LeftOfBuffer -= wcslen(ServiceName);
    wcsncat(ServiceKeyBuffer, PARAMETERS_KEY, LeftOfBuffer);
    LeftOfBuffer -= wcslen(PARAMETERS_KEY);

    if (LeftOfBuffer < 0)
    {
        DPRINT1("Buffer overflow for service name: '%s'\n", ServiceName);
        return FALSE;
    }

    /* Open the service registry key to find the dll name */
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, ServiceKeyBuffer, 0, KEY_READ, &hServiceKey) != ERROR_SUCCESS)
    {
        DPRINT1("Could not open service key (%s)\n", ServiceKeyBuffer);
        return FALSE;
    }

    do
    {
        if (Buffer)
            HeapFree(GetProcessHeap(), 0, Buffer);

        Buffer = HeapAlloc(GetProcessHeap(), 0, BufferSize);
        if (Buffer == NULL)
        {
            DPRINT1("Not enough memory for service: %s\n", ServiceName);
            return FALSE;
        }

        RetVal = RegQueryValueEx(hServiceKey, L"ServiceDll", NULL, &KeyType, (LPBYTE)Buffer, &BufferSize);

    } while (RetVal == ERROR_MORE_DATA);


    RegCloseKey(hServiceKey);

    if (RetVal != ERROR_SUCCESS || BufferSize == 0)
    {
        DPRINT1("Could not read 'ServiceDll' value from service: %s, ErrorCode: 0x%x\n", ServiceName, RetVal);
        HeapFree(GetProcessHeap(), 0, Buffer);
        return FALSE;
    }

    /* Convert possible %SystemRoot% to a real path */
    BufferSize = ExpandEnvironmentStrings(Buffer, DllPath, _countof(DllPath));
    if (BufferSize == 0)
    {
        DPRINT1("Invalid ServiceDll path: %s\n", Buffer);
        HeapFree(GetProcessHeap(), 0, Buffer);
        return FALSE;
    }

    HeapFree(GetProcessHeap(), 0, Buffer);

    /* Load the service dll */
    DPRINT("Trying to load dll\n");
    hServiceDll = LoadLibrary(DllPath);

    if (hServiceDll == NULL)
    {
        DPRINT1("Unable to load ServiceDll: %s, ErrorCode: %u\n", DllPath, GetLastError());
        return FALSE;
    }

    ServiceMainFunc = (LPSERVICE_MAIN_FUNCTION)GetProcAddress(hServiceDll, "ServiceMain");

    /* Allocate a service node in the linked list */
    Service = HeapAlloc(GetProcessHeap(), 0, sizeof(SERVICE));
    if (Service == NULL)
    {
        DPRINT1("Not enough memory for service: %s\n", ServiceName);
        return FALSE;
    }

    memset(Service, 0, sizeof(SERVICE));
    Service->Name = HeapAlloc(GetProcessHeap(), 0, (wcslen(ServiceName)+1) * sizeof(WCHAR));
    if (Service->Name == NULL)
    {
        DPRINT1("Not enough memory for service: %s\n", ServiceName);
        HeapFree(GetProcessHeap(), 0, Service);
        return FALSE;
    }
    wcscpy(Service->Name, ServiceName);

    Service->hServiceDll = hServiceDll;
    Service->ServiceMainFunc = ServiceMainFunc;

    Service->Next = FirstService;
    FirstService = Service;

    return TRUE;
}

VOID FreeServices(VOID)
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
DWORD LoadServiceCategory(PCWSTR ServiceCategory)
{
    HKEY hServicesKey;
    DWORD KeyType;
    DWORD BufferSize = REG_MAX_DATA_SIZE;
    WCHAR Buffer[REG_MAX_DATA_SIZE];
    PCWSTR ServiceName;
    DWORD BufferIndex = 0;
    DWORD NrOfServices = 0;

    /* Get all the services in this category */
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SVCHOST_REG_KEY, 0, KEY_READ, &hServicesKey) != ERROR_SUCCESS)
    {
        DPRINT1("Could not open service category: %s\n", ServiceCategory);
        return 0;
    }

    if (RegQueryValueEx(hServicesKey, ServiceCategory, NULL, &KeyType, (LPBYTE)Buffer, &BufferSize) != ERROR_SUCCESS)
    {
        DPRINT1("Could not open service category (2): %s\n", ServiceCategory);
        RegCloseKey(hServicesKey);
        return 0;
    }

    /* Clean up */
    RegCloseKey(hServicesKey);

    /* Load services in the category */
    ServiceName = Buffer;
    while (ServiceName[0] != UNICODE_NULL)
    {
        size_t Length;

        Length = wcslen(ServiceName);
        if (Length == 0)
            break;

        if (PrepareService(ServiceName) == TRUE)
            ++NrOfServices;

        BufferIndex += Length + 1;

        ServiceName = &Buffer[BufferIndex];
    }

    return NrOfServices;
}

int wmain(int argc, wchar_t **argv)
{
    DWORD NrOfServices;
    LPSERVICE_TABLE_ENTRY ServiceTable;

    if (argc < 3)
    {
        /* MS svchost.exe doesn't seem to print help, should we? */
        return 0;
    }

    if (wcscmp(argv[1], L"-k") != 0)
    {
        /* For now, we only handle "-k" */
        return 0;
    }

    NrOfServices = LoadServiceCategory(argv[2]);

    DPRINT("NrOfServices: %lu\n", NrOfServices);
    if (NrOfServices == 0)
        return 0;

    ServiceTable = HeapAlloc(GetProcessHeap(), 0, sizeof(SERVICE_TABLE_ENTRY) * (NrOfServices + 1));

    if (ServiceTable != NULL)
    {
        DWORD i;
        PSERVICE Service = FirstService;

        /* Fill the service table */
        for (i = 0; i < NrOfServices; i++)
        {
            DPRINT("Loading service: %s\n", Service->Name);
            ServiceTable[i].lpServiceName = Service->Name;
            ServiceTable[i].lpServiceProc = Service->ServiceMainFunc;
            Service = Service->Next;
        }

        /* Set a NULL entry to end the service table */
        ServiceTable[i].lpServiceName = NULL;
        ServiceTable[i].lpServiceProc = NULL;

        if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
            DPRINT1("Failed to start service control dispatcher, ErrorCode: %lu\n", GetLastError());

        HeapFree(GetProcessHeap(), 0, ServiceTable);
    }
    else
    {
        DPRINT1("Not enough memory for the service table, trying to allocate %u bytes\n", sizeof(SERVICE_TABLE_ENTRY) * (NrOfServices + 1));
    }

    DPRINT("Freeing services...\n");
    FreeServices();

    return 0;
}

/* EOF */
