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

#define REG_MAX_DATA_SIZE   2048

static PSERVICE FirstService = NULL;

/* FUNCTIONS *****************************************************************/

BOOL PrepareService(PCWSTR ServiceName)
{
    HKEY hServiceKey;
    WCHAR ServiceKeyBuffer[512];
    HRESULT Result;
    DWORD ValueType;
    PWSTR Buffer;
    DWORD BufferSize;
    LONG RetVal;
    HINSTANCE hServiceDll;
    PWSTR DllPath;
    DWORD DllPathSize;
    LPSERVICE_MAIN_FUNCTION ServiceMainFunc;
    PSERVICE Service;
    DWORD ServiceNameSize;

    /* Compose the registry path to the service's "Parameter" key */
    Result = StringCbCopy(ServiceKeyBuffer, sizeof(ServiceKeyBuffer), SERVICE_KEY);
    if (FAILED(Result))
    {
        DPRINT1("Buffer overflow for service name: '%ls'\n", ServiceName);
        return FALSE;
    }
    Result = StringCbCat(ServiceKeyBuffer, sizeof(ServiceKeyBuffer), ServiceName);
    if (FAILED(Result))
    {
        DPRINT1("Buffer overflow for service name: '%ls'\n", ServiceName);
        return FALSE;
    }
    Result = StringCbCat(ServiceKeyBuffer, sizeof(ServiceKeyBuffer), PARAMETERS_KEY);
    if (FAILED(Result))
    {
        DPRINT1("Buffer overflow for service name: '%ls'\n", ServiceName);
        return FALSE;
    }

    /* Open the service registry key to find the dll name */
    RetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ServiceKeyBuffer, 0, KEY_READ, &hServiceKey);
    if (RetVal != ERROR_SUCCESS)
    {
        DPRINT1("Could not open service key (%ls)\n", ServiceKeyBuffer);
        return FALSE;
    }

    BufferSize = 0;
    RetVal = RegQueryValueEx(hServiceKey, L"ServiceDll", NULL, &ValueType, NULL, &BufferSize);
    if (RetVal != ERROR_SUCCESS)
    {
        RegCloseKey(hServiceKey);
        return FALSE;
    }

    Buffer = HeapAlloc(GetProcessHeap(), 0, BufferSize);
    if (Buffer == NULL)
    {
        DPRINT1("Not enough memory for service: %ls\n", ServiceName);
        RegCloseKey(hServiceKey);
        return FALSE;
    }

    RetVal = RegQueryValueEx(hServiceKey, L"ServiceDll", NULL, &ValueType, (LPBYTE)Buffer, &BufferSize);
    if (RetVal != ERROR_SUCCESS || BufferSize == 0)
    {
        DPRINT1("Could not read 'ServiceDll' value from service: %ls, ErrorCode: 0x%lx\n", ServiceName, RetVal);
        RegCloseKey(hServiceKey);
        HeapFree(GetProcessHeap(), 0, Buffer);
        return FALSE;
    }
    RegCloseKey(hServiceKey);

    if (ValueType == REG_EXPAND_SZ)
    {
        /* Convert possible %SystemRoot% to a real path */
        DllPathSize = ExpandEnvironmentStrings(Buffer, NULL, 0);
        if (DllPathSize == 0)
        {
            DPRINT1("Invalid ServiceDll path: %ls, ErrorCode: %lu\n", Buffer, GetLastError());
            HeapFree(GetProcessHeap(), 0, Buffer);
            return FALSE;
        }
        DllPath = HeapAlloc(GetProcessHeap(), 0, DllPathSize * sizeof(WCHAR));
        if (DllPath == NULL)
        {
            DPRINT1("Not enough memory for service: %ls\n", ServiceName);
            HeapFree(GetProcessHeap(), 0, Buffer);
            return FALSE;
        }
        if (ExpandEnvironmentStrings(Buffer, DllPath, DllPathSize) != DllPathSize)
        {
            DPRINT1("Invalid ServiceDll path: %ls, ErrorCode: %lu\n", Buffer, GetLastError());
            HeapFree(GetProcessHeap(), 0, DllPath);
            HeapFree(GetProcessHeap(), 0, Buffer);
            return FALSE;
        }

        HeapFree(GetProcessHeap(), 0, Buffer);
    }
    else if (ValueType == REG_SZ)
    {
        DllPath = Buffer;
    }
    else
    {
        DPRINT1("Invalid ServiceDll value type %lu for service: %ls, ErrorCode: %lu\n", ValueType, ServiceName, GetLastError());
        HeapFree(GetProcessHeap(), 0, Buffer);
        return FALSE;
    }

    /* Load the service dll */
    DPRINT("Trying to load dll\n");
    hServiceDll = LoadLibrary(DllPath);
    if (hServiceDll == NULL)
    {
        DPRINT1("Unable to load ServiceDll: %ls, ErrorCode: %lu\n", DllPath, GetLastError());
        HeapFree(GetProcessHeap(), 0, DllPath);
        return FALSE;
    }
    HeapFree(GetProcessHeap(), 0, DllPath);

    ServiceMainFunc = (LPSERVICE_MAIN_FUNCTION)GetProcAddress(hServiceDll, "ServiceMain");

    /* Allocate a service node in the linked list */
    Service = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SERVICE));
    if (Service == NULL)
    {
        DPRINT1("Not enough memory for service: %ls\n", ServiceName);
        return FALSE;
    }

    ServiceNameSize = (wcslen(ServiceName) + 1) * sizeof(WCHAR);
    Service->Name = HeapAlloc(GetProcessHeap(), 0, ServiceNameSize);
    if (Service->Name == NULL)
    {
        DPRINT1("Not enough memory for service: %ls\n", ServiceName);
        HeapFree(GetProcessHeap(), 0, Service);
        return FALSE;
    }
    Result = StringCbCopy(Service->Name, ServiceNameSize, ServiceName);
    ASSERT(SUCCEEDED(Result));
    (VOID)Result;

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
    DWORD ValueType;
    WCHAR Buffer[REG_MAX_DATA_SIZE];
    DWORD BufferSize = sizeof(Buffer);
    PCWSTR ServiceName;
    DWORD BufferIndex = 0;
    DWORD NrOfServices = 0;
    LONG RetVal;

    /* Get all the services in this category */
    RetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SVCHOST_REG_KEY, 0, KEY_READ, &hServicesKey);
    if (RetVal != ERROR_SUCCESS)
    {
        DPRINT1("Could not open service category: %ls\n", ServiceCategory);
        return 0;
    }

    RetVal = RegQueryValueEx(hServicesKey, ServiceCategory, NULL, &ValueType, (LPBYTE)Buffer, &BufferSize);
    if (RetVal != ERROR_SUCCESS)
    {
        DPRINT1("Could not open service category (2): %ls\n", ServiceCategory);
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

        if (PrepareService(ServiceName))
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

    if (wcscmp(argv[1], L"-k"))
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
            DPRINT("Loading service: %ls\n", Service->Name);
            ServiceTable[i].lpServiceName = Service->Name;
            ServiceTable[i].lpServiceProc = Service->ServiceMainFunc;
            Service = Service->Next;
        }
        ASSERT(Service == NULL);

        /* Set a NULL entry to end the service table */
        ServiceTable[i].lpServiceName = NULL;
        ServiceTable[i].lpServiceProc = NULL;

        if (!StartServiceCtrlDispatcher(ServiceTable))
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
