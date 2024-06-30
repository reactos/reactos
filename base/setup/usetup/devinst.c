/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * PURPOSE:         Device installation
 * PROGRAMMER:      Hervé Poussineau (hpoussin@reactos.org)
 *                  Hermes Belusca-Maito
 */

#include <usetup.h>

#define NDEBUG
#include <debug.h>

#define INITGUID
#include <guiddef.h>
#include <libs/umpnpmgr/sysguid.h>

/* LOCALS ********************************************************************/

static HANDLE hEnumKey = NULL;
static HANDLE hServicesKey = NULL;

static HANDLE hNoPendingInstalls = NULL;

static HANDLE hPnpThread = NULL;
static HANDLE hDeviceInstallThread = NULL;

/* Device-install event list */
static HANDLE hDeviceInstallListMutex = NULL;
static LIST_ENTRY DeviceInstallListHead;
static HANDLE hDeviceInstallListNotEmpty = NULL;

typedef struct
{
    LIST_ENTRY ListEntry;
    WCHAR DeviceIds[ANYSIZE_ARRAY];
} DeviceInstallParams;

/* FUNCTIONS *****************************************************************/

static BOOLEAN
AreDriversLoaded(
    IN PCWSTR DeviceId)
{
    PLUGPLAY_CONTROL_STATUS_DATA PlugPlayData;
    NTSTATUS Status;

    RtlInitUnicodeString(&PlugPlayData.DeviceInstance, DeviceId);
    PlugPlayData.Operation = PNP_GET_DEVICE_STATUS;

    Status = NtPlugPlayControl(PlugPlayControlDeviceStatus, &PlugPlayData, sizeof(PlugPlayData));
    if (NT_SUCCESS(Status))
    {
        return (_Bool)((PlugPlayData.DeviceStatus & DN_DRIVER_LOADED) &&
                       !(PlugPlayData.DeviceStatus & DN_HAS_PROBLEM));
    }
    else
    {
        return FALSE;
    }
}

static BOOLEAN
InstallDriver(
    IN HINF hInf,
    IN HANDLE hServices,
    IN HANDLE hDeviceKey,
    IN LPCWSTR DeviceId,
    IN LPCWSTR HardwareId)
{
    UNICODE_STRING ServiceU = RTL_CONSTANT_STRING(L"Service");
    UNICODE_STRING ErrorControlU = RTL_CONSTANT_STRING(L"ErrorControl");
    UNICODE_STRING StartU = RTL_CONSTANT_STRING(L"Start");
    UNICODE_STRING TypeU = RTL_CONSTANT_STRING(L"Type");
    UNICODE_STRING UpperFiltersU = RTL_CONSTANT_STRING(L"UpperFilters");
    PWSTR keyboardClass = L"kbdclass\0";
    PWSTR partMgr = L"partmgr\0";

    UNICODE_STRING StringU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hService;
    INFCONTEXT Context;
    PCWSTR Driver, ClassGuid, ImagePath;
    ULONG dwValue;
    ULONG Disposition;
    NTSTATUS Status;
    BOOLEAN deviceInstalled = FALSE;

    /* First check if the driver needs any action at all */
    if (AreDriversLoaded(DeviceId))
        return TRUE;

    /* Check if we know the hardware */
    if (!SpInfFindFirstLine(hInf, L"HardwareIdsDatabase", HardwareId, &Context))
        return FALSE;
    if (!INF_GetDataField(&Context, 1, &Driver))
        return FALSE;

    /* Get associated class GUID (if any) */
    if (!INF_GetDataField(&Context, 2, &ClassGuid))
        ClassGuid = NULL;

    /* Find associated driver name */
    /* FIXME: check in other sections too! */
    if (!SpInfFindFirstLine(hInf, L"BootBusExtenders.Load", Driver, &Context)
     && !SpInfFindFirstLine(hInf, L"BusExtenders.Load", Driver, &Context)
     && !SpInfFindFirstLine(hInf, L"SCSI.Load", Driver, &Context)
     && !SpInfFindFirstLine(hInf, L"InputDevicesSupport.Load", Driver, &Context)
     && !SpInfFindFirstLine(hInf, L"Keyboard.Load", Driver, &Context))
    {
        INF_FreeData(ClassGuid);
        INF_FreeData(Driver);
        return FALSE;
    }

    if (!INF_GetDataField(&Context, 1, &ImagePath))
    {
        INF_FreeData(ClassGuid);
        INF_FreeData(Driver);
        return FALSE;
    }

    DPRINT1("Using driver '%S' for device '%S'\n", ImagePath, DeviceId);

    /* Create service key */
    RtlInitUnicodeString(&StringU, Driver);
    InitializeObjectAttributes(&ObjectAttributes, &StringU, OBJ_CASE_INSENSITIVE, hServices, NULL);
    Status = NtCreateKey(&hService, KEY_SET_VALUE, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, &Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey('%wZ') failed with status 0x%08x\n", &StringU, Status);
        INF_FreeData(ImagePath);
        INF_FreeData(ClassGuid);
        INF_FreeData(Driver);
        return FALSE;
    }

    /* Fill service key */
    if (Disposition == REG_CREATED_NEW_KEY)
    {
        dwValue = 0;
        NtSetValueKey(hService,
                      &ErrorControlU,
                      0,
                      REG_DWORD,
                      &dwValue,
                      sizeof(dwValue));

        dwValue = 0;
        NtSetValueKey(hService,
                      &StartU,
                      0,
                      REG_DWORD,
                      &dwValue,
                      sizeof(dwValue));

        dwValue = SERVICE_KERNEL_DRIVER;
        NtSetValueKey(hService,
                      &TypeU,
                      0,
                      REG_DWORD,
                      &dwValue,
                      sizeof(dwValue));
    }

    INF_FreeData(ImagePath);
    NtClose(hService);

    /* Add kbdclass and partmgr upper filters */
    if (ClassGuid &&_wcsicmp(ClassGuid, L"{4D36E96B-E325-11CE-BFC1-08002BE10318}") == 0)
    {
        DPRINT1("Installing keyboard class driver for '%S'\n", DeviceId);
        NtSetValueKey(hDeviceKey,
                      &UpperFiltersU,
                      0,
                      REG_MULTI_SZ,
                      keyboardClass,
                      (wcslen(keyboardClass) + 2) * sizeof(WCHAR));
    }
    else if (ClassGuid && _wcsicmp(ClassGuid, L"{4D36E967-E325-11CE-BFC1-08002BE10318}") == 0)
    {
        DPRINT1("Installing partition manager driver for '%S'\n", DeviceId);
        NtSetValueKey(hDeviceKey,
                      &UpperFiltersU,
                      0,
                      REG_MULTI_SZ,
                      partMgr,
                      (wcslen(partMgr) + 2) * sizeof(WCHAR));
    }

    INF_FreeData(ClassGuid);

    /* Associate device with the service we just filled */
    Status = NtSetValueKey(hDeviceKey,
                           &ServiceU,
                           0,
                           REG_SZ,
                           (PVOID)Driver,
                           (wcslen(Driver) + 1) * sizeof(WCHAR));
    if (NT_SUCCESS(Status))
    {
        /* We've registered the driver, time to start a device */
        PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA ControlData;
        RtlInitUnicodeString(&ControlData.DeviceInstance, DeviceId);

        Status = NtPlugPlayControl(PlugPlayControlStartDevice, &ControlData, sizeof(ControlData));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtPlugPlayControl() failed with status 0x%08x\n", Status);
        }

        deviceInstalled = NT_SUCCESS(Status);
    }

    INF_FreeData(Driver);

    return deviceInstalled;
}

static VOID
InstallDevice(
    IN HINF hInf,
    IN HANDLE hEnum,
    IN HANDLE hServices,
    IN LPCWSTR DeviceId)
{
    UNICODE_STRING HardwareIDU = RTL_CONSTANT_STRING(L"HardwareID");
    UNICODE_STRING CompatibleIDsU = RTL_CONSTANT_STRING(L"CompatibleIDs");

    UNICODE_STRING DeviceIdU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LPCWSTR HardwareID;
    PKEY_VALUE_PARTIAL_INFORMATION pPartialInformation = NULL;
    HANDLE hDeviceKey;
    ULONG ulRequired;
    BOOLEAN bDriverInstalled = FALSE;
    NTSTATUS Status;

    RtlInitUnicodeString(&DeviceIdU, DeviceId);
    InitializeObjectAttributes(&ObjectAttributes, &DeviceIdU, OBJ_CASE_INSENSITIVE, hEnum, NULL);
    Status = NtOpenKey(&hDeviceKey, KEY_QUERY_VALUE | KEY_SET_VALUE, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Unable to open subkey '%S'\n", DeviceId);
        return;
    }

    Status = NtQueryValueKey(
        hDeviceKey,
        &HardwareIDU,
        KeyValuePartialInformation,
        NULL,
        0,
        &ulRequired);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        pPartialInformation = (PKEY_VALUE_PARTIAL_INFORMATION)RtlAllocateHeap(ProcessHeap, 0, ulRequired);
        if (!pPartialInformation)
        {
            DPRINT1("RtlAllocateHeap() failed\n");
            NtClose(hDeviceKey);
            return;
        }
        Status = NtQueryValueKey(
            hDeviceKey,
            &HardwareIDU,
            KeyValuePartialInformation,
            pPartialInformation,
            ulRequired,
            &ulRequired);
    }
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        /* Nothing to do */
    }
    else if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryValueKey() failed with status 0x%08x\n", Status);
        if (pPartialInformation)
            RtlFreeHeap(ProcessHeap, 0, pPartialInformation);
        NtClose(hDeviceKey);
        return;
    }
    else if (pPartialInformation)
    {
        for (HardwareID = (LPCWSTR)pPartialInformation->Data;
             (PUCHAR)HardwareID < pPartialInformation->Data + pPartialInformation->DataLength
                 && *HardwareID
                 && !bDriverInstalled;
            HardwareID += wcslen(HardwareID) + 1)
        {
            bDriverInstalled = InstallDriver(hInf, hServices,hDeviceKey, DeviceId, HardwareID);
        }
    }

    if (!bDriverInstalled)
    {
        if (pPartialInformation)
        {
            RtlFreeHeap(ProcessHeap, 0, pPartialInformation);
            pPartialInformation = NULL;
        }
        Status = NtQueryValueKey(
            hDeviceKey,
            &CompatibleIDsU,
            KeyValuePartialInformation,
            NULL,
            0,
            &ulRequired);
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            pPartialInformation = (PKEY_VALUE_PARTIAL_INFORMATION)RtlAllocateHeap(ProcessHeap, 0, ulRequired);
            if (!pPartialInformation)
            {
                DPRINT("RtlAllocateHeap() failed\n");
                NtClose(hDeviceKey);
                return;
            }
            Status = NtQueryValueKey(
                hDeviceKey,
                &CompatibleIDsU,
                KeyValuePartialInformation,
                pPartialInformation,
                ulRequired,
                &ulRequired);
        }
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            /* Nothing to do */
        }
        else if (!NT_SUCCESS(Status))
        {
            if (pPartialInformation)
                RtlFreeHeap(ProcessHeap, 0, pPartialInformation);
            NtClose(hDeviceKey);
            DPRINT1("NtQueryValueKey() failed with status 0x%08x\n", Status);
            return;
        }
        else if (pPartialInformation)
        {
            for (HardwareID = (LPCWSTR)pPartialInformation->Data;
                 (PUCHAR)HardwareID < pPartialInformation->Data + pPartialInformation->DataLength
                     && *HardwareID
                     && !bDriverInstalled;
                HardwareID += wcslen(HardwareID) + 1)
            {
                bDriverInstalled = InstallDriver(hInf, hServices,hDeviceKey, DeviceId, HardwareID);
            }
        }
    }
    if (!bDriverInstalled)
        DPRINT("No driver available for %S\n", DeviceId);

    RtlFreeHeap(ProcessHeap, 0, pPartialInformation);
    NtClose(hDeviceKey);
}

/* Loop to install all queued devices installations */
static ULONG NTAPI
DeviceInstallThread(IN PVOID Parameter)
{
    HINF hSetupInf = *(HINF*)Parameter;
    PLIST_ENTRY ListEntry;
    DeviceInstallParams* Params;
    LARGE_INTEGER Timeout;

    for (;;)
    {
        /* Dequeue the next oldest device-install event */
        NtWaitForSingleObject(hDeviceInstallListMutex, FALSE, NULL);
        ListEntry = (IsListEmpty(&DeviceInstallListHead)
                        ? NULL : RemoveHeadList(&DeviceInstallListHead));
        NtReleaseMutant(hDeviceInstallListMutex, NULL);

        if (ListEntry == NULL)
        {
            /*
             * The list is now empty, but there may be a new enumerated device
             * that is going to be added to the list soon. In order to avoid
             * setting the hNoPendingInstalls event to release it soon after,
             * we wait for maximum 1 second for no PnP enumeration event being
             * received before declaring that no pending installations are
             * taking place and setting the corresponding event.
             */
            Timeout.QuadPart = -10000000LL; /* Wait for 1 second */
            if (NtWaitForSingleObject(hDeviceInstallListNotEmpty, FALSE, &Timeout) == STATUS_TIMEOUT)
            {
                /* We timed out: set the event and do the actual wait */
                NtSetEvent(hNoPendingInstalls, NULL);
                NtWaitForSingleObject(hDeviceInstallListNotEmpty, FALSE, NULL);
            }
        }
        else
        {
            NtResetEvent(hNoPendingInstalls, NULL);
            Params = CONTAINING_RECORD(ListEntry, DeviceInstallParams, ListEntry);
            InstallDevice(hSetupInf, hEnumKey, hServicesKey, Params->DeviceIds);
            RtlFreeHeap(ProcessHeap, 0, Params);
        }
    }

    return 0;
}

static ULONG NTAPI
PnpEventThread(IN PVOID Parameter)
{
    NTSTATUS Status;
    PLUGPLAY_CONTROL_USER_RESPONSE_DATA ResponseData = {0, 0, 0, 0};
    PPLUGPLAY_EVENT_BLOCK PnpEvent, NewPnpEvent;
    ULONG PnpEventSize;

    UNREFERENCED_PARAMETER(Parameter);

    PnpEventSize = 0x1000;
    PnpEvent = RtlAllocateHeap(ProcessHeap, 0, PnpEventSize);
    if (PnpEvent == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Quit;
    }

    for (;;)
    {
        DPRINT("Calling NtGetPlugPlayEvent()\n");

        /* Wait for the next PnP event */
        Status = NtGetPlugPlayEvent(0, 0, PnpEvent, PnpEventSize);

        /* Resize the buffer for the PnP event if it's too small */
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            PnpEventSize += 0x400;
            NewPnpEvent = RtlReAllocateHeap(ProcessHeap, 0, PnpEvent, PnpEventSize);
            if (NewPnpEvent == NULL)
            {
                Status = STATUS_NO_MEMORY;
                goto Quit;
            }
            PnpEvent = NewPnpEvent;
            continue;
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtGetPlugPlayEvent() failed (Status 0x%08lx)\n", Status);
            goto Quit;
        }

        /* Process the PnP event */
        DPRINT("Received PnP Event\n");
        if (IsEqualGUID(&PnpEvent->EventGuid, &GUID_DEVICE_ENUMERATED))
        {
            DeviceInstallParams* Params;
            ULONG len;
            ULONG DeviceIdLength;

            DPRINT("Device enumerated: %S\n", PnpEvent->TargetDevice.DeviceIds);

            DeviceIdLength = wcslen(PnpEvent->TargetDevice.DeviceIds);
            if (DeviceIdLength)
            {
                /* Allocate a new device-install event */
                len = FIELD_OFFSET(DeviceInstallParams, DeviceIds) + (DeviceIdLength + 1) * sizeof(WCHAR);
                Params = RtlAllocateHeap(ProcessHeap, 0, len);
                if (Params)
                {
                    wcscpy(Params->DeviceIds, PnpEvent->TargetDevice.DeviceIds);

                    /* Queue the event (will be dequeued by DeviceInstallThread) */
                    NtWaitForSingleObject(hDeviceInstallListMutex, FALSE, NULL);
                    InsertTailList(&DeviceInstallListHead, &Params->ListEntry);
                    NtReleaseMutant(hDeviceInstallListMutex, NULL);

                    NtSetEvent(hDeviceInstallListNotEmpty, NULL);
                }
                else
                {
                    DPRINT1("Not enough memory (size %lu)\n", len);
                }
            }
        }
        else
        {
            DPRINT("Unknown event, GUID {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
                PnpEvent->EventGuid.Data1, PnpEvent->EventGuid.Data2, PnpEvent->EventGuid.Data3,
                PnpEvent->EventGuid.Data4[0], PnpEvent->EventGuid.Data4[1], PnpEvent->EventGuid.Data4[2],
                PnpEvent->EventGuid.Data4[3], PnpEvent->EventGuid.Data4[4], PnpEvent->EventGuid.Data4[5],
                PnpEvent->EventGuid.Data4[6], PnpEvent->EventGuid.Data4[7]);
        }

        /* Dequeue the current PnP event and signal the next one */
        Status = NtPlugPlayControl(PlugPlayControlUserResponse,
                                   &ResponseData,
                                   sizeof(ResponseData));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtPlugPlayControl(PlugPlayControlUserResponse) failed (Status 0x%08lx)\n", Status);
            goto Quit;
        }
    }

    Status = STATUS_SUCCESS;

Quit:
    if (PnpEvent)
        RtlFreeHeap(ProcessHeap, 0, PnpEvent);

    NtTerminateThread(NtCurrentThread(), Status);
    return Status;
}

NTSTATUS
WaitNoPendingInstallEvents(
    IN PLARGE_INTEGER Timeout OPTIONAL)
{
    return NtWaitForSingleObject(hNoPendingInstalls, FALSE, Timeout);
}

BOOLEAN
EnableUserModePnpManager(VOID)
{
    LARGE_INTEGER Timeout;

    /* Start the PnP thread */
    if (hPnpThread != NULL)
        NtResumeThread(hPnpThread, NULL);

    /*
     * Wait a little bit so that we get a chance to have some events being
     * queued by the time the device-installation thread becomes resumed.
     */
    Timeout.QuadPart = -10000000LL; /* Wait for 1 second */
    NtWaitForSingleObject(hDeviceInstallListNotEmpty, FALSE, &Timeout);

    /* Start the device installation thread */
    if (hDeviceInstallThread != NULL)
        NtResumeThread(hDeviceInstallThread, NULL);

    return TRUE;
}

BOOLEAN
DisableUserModePnpManager(VOID)
{
    /* Wait until all pending installations are done, then freeze the threads */
    if (WaitNoPendingInstallEvents(NULL) != STATUS_WAIT_0)
        DPRINT1("WaitNoPendingInstallEvents() failed to wait!\n");

    // TODO: use signalling events

    NtSuspendThread(hPnpThread, NULL);
    NtSuspendThread(hDeviceInstallThread, NULL);

    return TRUE;
}

NTSTATUS
InitializeUserModePnpManager(
    IN HINF* phSetupInf)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;

    UNICODE_STRING EnumU = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Enum");
    UNICODE_STRING ServicesU = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services");

    Status = NtCreateEvent(&hNoPendingInstalls,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not create the Pending-Install Event! (Status 0x%08lx)\n", Status);
        goto Failure;
    }

    /*
     * Initialize the device-install event list
     */

    Status = NtCreateEvent(&hDeviceInstallListNotEmpty,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not create the List Event! (Status 0x%08lx)\n", Status);
        goto Failure;
    }

    Status = NtCreateMutant(&hDeviceInstallListMutex,
                            MUTANT_ALL_ACCESS,
                            NULL, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not create the List Mutex! (Status 0x%08lx)\n", Status);
        goto Failure;
    }
    InitializeListHead(&DeviceInstallListHead);

    InitializeObjectAttributes(&ObjectAttributes, &EnumU, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenKey(&hEnumKey, KEY_QUERY_VALUE, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenKey('%wZ') failed (Status 0x%08lx)\n", &EnumU, Status);
        goto Failure;
    }

    InitializeObjectAttributes(&ObjectAttributes, &ServicesU, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtCreateKey(&hServicesKey, KEY_ALL_ACCESS, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey('%wZ') failed (Status 0x%08lx)\n", &ServicesU, Status);
        goto Failure;
    }

    /* Create the PnP event thread in suspended state */
    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 TRUE,
                                 0,
                                 0,
                                 0,
                                 PnpEventThread,
                                 NULL,
                                 &hPnpThread,
                                 NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create the PnP event thread (Status 0x%08lx)\n", Status);
        hPnpThread = NULL;
        goto Failure;
    }

    /* Create the device installation thread in suspended state */
    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 TRUE,
                                 0,
                                 0,
                                 0,
                                 DeviceInstallThread,
                                 phSetupInf,
                                 &hDeviceInstallThread,
                                 NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create the device installation thread (Status 0x%08lx)\n", Status);
        hDeviceInstallThread = NULL;
        goto Failure;
    }

    return STATUS_SUCCESS;

Failure:
    if (hPnpThread)
    {
        NtTerminateThread(hPnpThread, STATUS_SUCCESS);
        NtClose(hPnpThread);
    }
    hPnpThread = NULL;

    if (hServicesKey)
        NtClose(hServicesKey);
    hServicesKey = NULL;

    if (hEnumKey)
        NtClose(hEnumKey);
    hEnumKey = NULL;

    if (hDeviceInstallListMutex)
        NtClose(hDeviceInstallListMutex);
    hDeviceInstallListMutex = NULL;

    if (hDeviceInstallListNotEmpty)
        NtClose(hDeviceInstallListNotEmpty);
    hDeviceInstallListNotEmpty = NULL;

    if (hNoPendingInstalls)
        NtClose(hNoPendingInstalls);
    hNoPendingInstalls = NULL;

    return Status;
}

VOID
TerminateUserModePnpManager(VOID)
{
    DisableUserModePnpManager();

    // TODO: use signalling events

    /* Kill the PnP thread as it blocks inside the NtGetPlugPlayEvent() call */
    if (hPnpThread)
    {
        NtTerminateThread(hPnpThread, STATUS_SUCCESS);
        NtClose(hPnpThread);
    }
    hPnpThread = NULL;

    /* Kill the device installation thread */
    if (hDeviceInstallThread)
    {
        NtTerminateThread(hDeviceInstallThread, STATUS_SUCCESS);
        NtClose(hDeviceInstallThread);
    }
    hDeviceInstallThread = NULL;

    /* Close the opened handles */

    if (hServicesKey)
        NtClose(hServicesKey);
    hServicesKey = NULL;

    if (hEnumKey)
        NtClose(hEnumKey);
    hEnumKey = NULL;

    if (hNoPendingInstalls)
        NtClose(hNoPendingInstalls);
    hNoPendingInstalls = NULL;

    if (hDeviceInstallListNotEmpty)
        NtClose(hDeviceInstallListNotEmpty);
    hDeviceInstallListNotEmpty = NULL;
}


/* PnP Configuration Manager ENUMERATION API *********************************/

#define _CFGMGR32_
#include <cfgmgr32.h>

static CONFIGRET
NtStatusToCrError(
    _In_ NTSTATUS Status)
{
    switch (Status)
    {
        case STATUS_NOT_IMPLEMENTED:
            return CR_CALL_NOT_IMPLEMENTED;

        case STATUS_INVALID_PARAMETER:
            return CR_INVALID_DATA;

        case STATUS_NO_SUCH_DEVICE:
            return CR_NO_SUCH_DEVINST;

        case STATUS_ACCESS_DENIED:
            return CR_ACCESS_DENIED;

        case STATUS_BUFFER_TOO_SMALL:
            return CR_BUFFER_SMALL;

        case STATUS_OBJECT_NAME_NOT_FOUND:
            return CR_NO_SUCH_VALUE;

        default:
            return CR_FAILURE;
    }
}

// CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_SizeW(
    _Out_ PULONG pulLen,
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ DEVINSTID_W pDeviceID,
    _In_ ULONG ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;

    if (!pulLen)
        return CR_INVALID_POINTER;

    if (ulFlags & ~CM_GET_DEVICE_INTERFACE_LIST_BITS)
        return CR_INVALID_FLAG;

    *pulLen = 0;

    // PNP_GetInterfaceDeviceListSize()
    do
    {
        NTSTATUS Status;
        PLUGPLAY_CONTROL_INTERFACE_DEVICE_LIST_DATA PlugPlayData;

#if 0
        if (!IsValidDeviceInstanceID(pDeviceID))
        {
            ret = CR_INVALID_DEVINST;
            break;
        }
#endif

        RtlInitUnicodeString(&PlugPlayData.DeviceInstance, pDeviceID);

        PlugPlayData.FilterGuid = InterfaceClassGuid;
        PlugPlayData.Flags = ulFlags;
        PlugPlayData.Buffer = NULL;
        PlugPlayData.BufferSize = 0;

        Status = NtPlugPlayControl(PlugPlayControlGetInterfaceDeviceList,
                                   &PlugPlayData,
                                   sizeof(PlugPlayData));
        if (NT_SUCCESS(Status))
            *pulLen = PlugPlayData.BufferSize;
        else
            ret = NtStatusToCrError(Status);
    } while (0);

    return ret;
}

// CMAPI
CONFIGRET
WINAPI
CM_Get_Device_Interface_ListW(
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ DEVINSTID_W pDeviceID,
    _Out_writes_(BufferLen) PWCHAR Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags)
{
    CONFIGRET ret = CR_SUCCESS;

    if (!Buffer || BufferLen == 0)
        return CR_INVALID_POINTER;

    if (ulFlags & ~CM_GET_DEVICE_INTERFACE_LIST_BITS)
        return CR_INVALID_FLAG;

    *Buffer = UNICODE_NULL;

    // PNP_GetInterfaceDeviceList()
    do
    {
        NTSTATUS Status;
        PLUGPLAY_CONTROL_INTERFACE_DEVICE_LIST_DATA PlugPlayData;

#if 0
        if (!IsValidDeviceInstanceID(pDeviceID))
        {
            ret = CR_INVALID_DEVINST;
            break;
        }
#endif

        RtlInitUnicodeString(&PlugPlayData.DeviceInstance, pDeviceID);

        PlugPlayData.FilterGuid = InterfaceClassGuid;
        PlugPlayData.Flags = ulFlags;
        PlugPlayData.Buffer = Buffer;
        PlugPlayData.BufferSize = BufferLen;

        Status = NtPlugPlayControl(PlugPlayControlGetInterfaceDeviceList,
                                   &PlugPlayData,
                                   sizeof(PlugPlayData));
        if (NT_SUCCESS(Status))
            BufferLen = PlugPlayData.BufferSize;
        else
            ret = NtStatusToCrError(Status);
    } while (0);

    return ret;
}

/* EOF */
