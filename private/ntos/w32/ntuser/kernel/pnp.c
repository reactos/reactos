/****************************** Module Header ******************************\
* Module Name: pnp.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module tracks device interface changes so we can keep track of know how many mice and
* keyboards and mouse
* and mouse reports.
*
* History:
* 97-10-16   IanJa   Interpreted from a dream that Ken Ray had.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

DEVICE_TEMPLATE aDeviceTemplate[DEVICE_TYPE_MAX + 1] = {
    // DEVICE_TYPE_MOUSE
    {
        sizeof(GENERIC_DEVICE_INFO)+sizeof(MOUSE_DEVICE_INFO),    // cbDeviceInfo
        &GUID_CLASS_MOUSE,                                        // pClassGUID
        PMAP_MOUCLASS_PARAMS,                                     // uiRegistrySection
        L"mouclass",                                              // pwszClassName
        DD_MOUSE_DEVICE_NAME_U L"0",                              // pwszDefDevName
        DD_MOUSE_DEVICE_NAME_U L"Legacy0",                        // pwszLegacyDevName
        IOCTL_MOUSE_QUERY_ATTRIBUTES,                             // IOCTL_Attr
        FIELD_OFFSET(DEVICEINFO, mouse.Attr),                     // offAttr
        sizeof((PDEVICEINFO)NULL)->mouse.Attr,                    // cbAttr
        FIELD_OFFSET(DEVICEINFO, mouse.Data),                     // offData
        sizeof((PDEVICEINFO)NULL)->mouse.Data,                    // cbData
        ProcessMouseInput,                                        // Reader routine
        NULL                                                      // pkeHidChange
    },
    // DEVICE_TYPE_KEYBOARD
    {
        sizeof(GENERIC_DEVICE_INFO)+sizeof(KEYBOARD_DEVICE_INFO), // cbDeviceInfo
        &GUID_CLASS_KEYBOARD,                                     // pClassGUID
        PMAP_KBDCLASS_PARAMS,                                     // uiRegistrySection
        L"kbdclass",                                              // pwszClassName
        DD_KEYBOARD_DEVICE_NAME_U L"0",                           // pwszDefDevName
        DD_KEYBOARD_DEVICE_NAME_U L"Legacy0",                     // pwszLegacyDevName
        IOCTL_KEYBOARD_QUERY_ATTRIBUTES,                          // IOCTL_Attr
        FIELD_OFFSET(DEVICEINFO, keyboard.Attr),                  // offAttr
        sizeof((PDEVICEINFO)NULL)->keyboard.Attr,                 // cbAttr
        FIELD_OFFSET(DEVICEINFO, keyboard.Data),                  // offData
        sizeof((PDEVICEINFO)NULL)->keyboard.Data,                 // cbData
        ProcessKeyboardInput,                                     // Reader routine
        NULL                                                      // pkeHidChange
    },
    // Add new input device type template here
};

#ifdef DIAGNOSE_IO
NTSTATUS gKbdIoctlLEDSStatus = -1;   // last IOCTL_KEYBOARD_QUERY_INDICATORS
#endif

typedef struct _CDROM_NOTIFY {
    LIST_ENTRY                   Entry;
    ULONG                        Size;
    PVOID                        RegistrationHandle;
    ULONG                        Event;
    // Must be last field
    MOUNTMGR_DRIVE_LETTER_TARGET DeviceName;
} CDROM_NOTIFY, *PCDROM_NOTIFY;

LIST_ENTRY gMediaChangeList;
PFAST_MUTEX gMediaChangeMutex;
HANDLE gpEventMediaChange     = NULL;
UCHAR DriveLetterChange[26];
#define EVENT_CDROM_MEDIA_ARRIVAL 1
#define EVENT_CDROM_MEDIA_REMOVAL 2

/***************************************************************************\
* Win32kPnPDriverEntry
*
* This is the callback function when we call IoCreateDriver to create a
* PnP Driver Object.  In this function, we need to remember the DriverObject.
*
* Parameters:
*   DriverObject - Pointer to the driver object created by the system.
*   RegistryPath - is NULL.
*
* Return Value: STATUS_SUCCESS
*
* History:
* 10-20-97  IanJa   Taken from ntos\io\pnpinit.c
\***************************************************************************/

NTSTATUS
Win32kPnPDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING pustrRegistryPath
    )

{
    TAGMSG2(DBGTAG_PNP,
            "Win32kPnPDriverEntry(DriverObject = %lx, pustrRegistryPath = %#p)",
            DriverObject, pustrRegistryPath);

    //
    // File the pointer to our driver object away
    //
    gpWin32kDriverObject = DriverObject;
    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pustrRegistryPath);
}


/***************************************************************************\
* Initialize the global event used in notifying CSR that media has changed.
*
* Execution Context:
*
* History:
\***************************************************************************/

VOID
InitializeMediaChange(HANDLE hMediaRequestEvent)
{
    if (!gbRemoteSession) {

        InitializeListHead(&gMediaChangeList);

        ObReferenceObjectByHandle(hMediaRequestEvent,
                                  EVENT_ALL_ACCESS,
                                  *ExEventObjectType,
                                  KernelMode,
                                  &gpEventMediaChange,
                                  NULL);

        gMediaChangeMutex = UserAllocPoolNonPaged(sizeof(FAST_MUTEX), TAG_PNP);

        if (gMediaChangeMutex) {
            ExInitializeFastMutex(gMediaChangeMutex);
        }
    }

    return;
}

__inline VOID EnterMediaCrit() {
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(gMediaChangeMutex);
}

__inline VOID LeaveMediaCrit() {
    ExReleaseFastMutexUnsafe(gMediaChangeMutex);
    KeLeaveCriticalRegion();
}



/***************************************************************************\
* Routines to support CDROM driver letters.
*
* Execution Context:
*
* History:
\***************************************************************************/

ULONG GetDeviceChangeInfo()
{
    UNICODE_STRING                      name;
    PFILE_OBJECT                        FileObject;
    PDEVICE_OBJECT                      DeviceObject;
    KEVENT                              event;
    PIRP                                irp;
    MOUNTMGR_DRIVE_LETTER_INFORMATION   output;
    IO_STATUS_BLOCK                     ioStatus;
    NTSTATUS                            status;
    PCDROM_NOTIFY                       pContext = 0;

    ULONG retval = 0;

    if (!(ISCSRSS())) {
        return 0;
    }

    EnterMediaCrit();
    if (!IsListEmpty(&gMediaChangeList)) {
        pContext = (PCDROM_NOTIFY) RemoveTailList(&gMediaChangeList);
    }
    LeaveMediaCrit();

    if (pContext == NULL) {
        return 0;
    }

    RtlInitUnicodeString(&name, MOUNTMGR_DEVICE_NAME);
    status = IoGetDeviceObjectPointer(&name,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);

    if (NT_SUCCESS(status)) {

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER,
                                            DeviceObject,
                                            &pContext->DeviceName,
                                            sizeof(MOUNTMGR_DRIVE_LETTER_TARGET) +
                                                pContext->DeviceName.DeviceNameLength,
                                            &output,
                                            sizeof(output),
                                            FALSE,
                                            &event,
                                            &ioStatus);
        if (irp) {

            status = IoCallDriver(DeviceObject, irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                status = ioStatus.Status;
            }
            if ((status == STATUS_SUCCESS) &&
                (output.CurrentDriveLetter)) {

                UserAssert((output.CurrentDriveLetter - 'A') < 30);

                retval = 1 << (output.CurrentDriveLetter - 'A');

                if (pContext->Event & EVENT_CDROM_MEDIA_ARRIVAL) {
                    retval |= 0x80000000;
                }
            }
        }

        ObDereferenceObject(FileObject);
    }

    //
    // Allways free the request
    //

    UserFreePool(pContext);

    return retval;
}

/***************************************************************************\
* Handle device notifications such as MediaChanged
*
* Execution Context:
*
* History:
\***************************************************************************/
NTSTATUS DeviceCDROMNotify(
    IN PTARGET_DEVICE_CUSTOM_NOTIFICATION Notification,
    IN PCDROM_NOTIFY pContext)
{
    PCDROM_NOTIFY pNew;

    CheckCritOut();

    UserAssert(!gbRemoteSession);
    UserAssert(pContext);

    if (IsEqualGUID(&Notification->Event, &GUID_IO_MEDIA_ARRIVAL))
    {
        pContext->Event = EVENT_CDROM_MEDIA_ARRIVAL;
    }
    else if (IsEqualGUID(&Notification->Event, &GUID_IO_MEDIA_REMOVAL))
    {
        pContext->Event = EVENT_CDROM_MEDIA_REMOVAL;
    }
    else if (IsEqualGUID(&Notification->Event, &GUID_TARGET_DEVICE_REMOVE_COMPLETE))
    {
        IoUnregisterPlugPlayNotification(pContext->RegistrationHandle);
        UserFreePool(pContext);
        return STATUS_SUCCESS;
    }
    else
    {
        return STATUS_SUCCESS;
    }

    //
    // Process the arrival or removal
    //
    // We must queue this otherwise we end up bugchecking on Terminal Server
    // This is due to opening a handle from within the system process which
    // requires us to do an attach process.
    //

    pNew = UserAllocPoolNonPaged(pContext->Size, TAG_PNP);
    if (pNew)
    {
        RtlCopyMemory(pNew, pContext, pContext->Size);

        EnterMediaCrit();
        InsertHeadList(&gMediaChangeList, &pNew->Entry);
        LeaveMediaCrit();

        KeSetEvent(gpEventMediaChange, EVENT_INCREMENT, FALSE);
    }

    return STATUS_SUCCESS;
}



/***************************************************************************\
* DeviceClassCDROMNotify
*
* This gets called when CDROM appears or disappears
*
\***************************************************************************/
NTSTATUS
DeviceClassCDROMNotify (
    IN PDEVICE_INTERFACE_CHANGE_NOTIFICATION classChange,
    IN PVOID Unused
    )
{
    NTSTATUS       Status = STATUS_SUCCESS;
    PFILE_OBJECT   FileObject;
    PDEVICE_OBJECT DeviceObject;
    PCDROM_NOTIFY  pContext;
    ULONG          Size;

    UNREFERENCED_PARAMETER(Unused);

    CheckCritOut();

    /*
     * Sanity check the DeviceType, and that it matches the InterfaceClassGuid
     */
    UserAssert(IsEqualGUID(&classChange->InterfaceClassGuid, &CdRomClassGuid));

    if (IsEqualGUID(&classChange->Event, &GUID_DEVICE_INTERFACE_ARRIVAL)) {

        Status = IoGetDeviceObjectPointer(classChange->SymbolicLinkName,
                                          FILE_READ_ATTRIBUTES,
                                          &FileObject,
                                          &DeviceObject);

        if (NT_SUCCESS(Status)) {

            Size = sizeof(CDROM_NOTIFY) + classChange->SymbolicLinkName->Length;

            pContext = (PCDROM_NOTIFY) UserAllocPool(Size, TAG_PNP);

            //
            // Register For MediaChangeNotifications on all the CDROMs.
            //

            if (pContext) {

                pContext->Size = Size;
                pContext->DeviceName.DeviceNameLength = classChange->SymbolicLinkName->Length;
                RtlCopyMemory(pContext->DeviceName.DeviceName,
                              classChange->SymbolicLinkName->Buffer,
                              pContext->DeviceName.DeviceNameLength);

                IoRegisterPlugPlayNotification (
                        EventCategoryTargetDeviceChange,
                        0,
                        FileObject,
                        gpWin32kDriverObject,
                        DeviceCDROMNotify,
                        pContext,
                        &(pContext->RegistrationHandle));
            }

            ObDereferenceObject(FileObject);
        }

    } else if (IsEqualGUID(&classChange->Event, &GUID_DEVICE_INTERFACE_REMOVAL)) {

        //
        // Do nothing - we already remove the registration.
        //

    } else {
        RIPMSG0(RIP_ERROR, "unrecognized Event GUID");
    }


    return STATUS_SUCCESS;
}


/***************************************************************************\
* CreateDeviceInfo
*
* This creates an instance of an input device for USER.  To do this it:
*  - Allocates a DEVICEINFO struct
*  - Adds it to USER's list of input devices
*  - Initializes some of the fields
*  - Signals the input servicing thread to open and read the new device.
*
* Type - the device type (DEVICE_TYPE_MOUSE, DEVICE_TYPE_KEYBOARD)
* Name - the device name.
*        When trying to open a HYDRA client's mouse, Name is NULL.
* bFlags - some initial flags to set (eg: GDIF_NOTPNP)
*
* THIS FUNCTION IS CALLED IN THE CONTEXT OF THE KERNEL PROCESS
* so we mustn't open the mouse here, else the handle we get will not belong
* to the Win32k process.
*
* History:
* 11-26-90 DavidPe      Created.
* 01-07-98 IanJa        Plug & Play
\***************************************************************************/

PDEVICEINFO CreateDeviceInfo(DWORD DeviceType, PUNICODE_STRING pustrName, BYTE bFlags)
{
    PDEVICEINFO pDeviceInfo = NULL;

    CheckCritIn();
    BEGINATOMICCHECK();

    UserAssert(pustrName != NULL);

    TAGMSG3(DBGTAG_PNP, "CreateDeviceInfo(%d, %S, %x)", DeviceType, pustrName->Buffer, bFlags);

    if (DeviceType > DEVICE_TYPE_MAX) {
        RIPMSG1(RIP_ERROR, "Unknown DeviceType %lx", DeviceType);
    }

    pDeviceInfo = UserAllocPoolZInit(aDeviceTemplate[DeviceType].cbDeviceInfo, TAG_PNP);
    if (pDeviceInfo == NULL) {
        RIPMSG0(RIP_WARNING, "CreateDeviceInfo() out of memory allocating DEVICEINFO");
        EXITATOMICCHECK();
        return NULL;
    }

    if (pustrName->Buffer != NULL) {
        pDeviceInfo->ustrName.Buffer = UserAllocPool(pustrName->Length, TAG_PNP);

        if (pDeviceInfo->ustrName.Buffer == NULL) {
            RIPMSG1(RIP_WARNING, "CreateDeviceInfo: Can't duplicate string %ws",
                    pustrName->Buffer);
            goto CreateFailed;
        }

        pDeviceInfo->ustrName.MaximumLength = pustrName->Length;
        RtlCopyUnicodeString(&pDeviceInfo->ustrName, pustrName);
    }

    pDeviceInfo->type = (BYTE)DeviceType;

    /*
     * Create this device's HidChangeCompletion event. When the RIT completes
     * a synchronous ProcessDeviceChanges() it signals the HidChangeCompletion
     * event to wake the requesting RequestDeviceChange() which is blocking on
     * the event.
     * Each device has it's own HidChangeCompletion event,
     * since multiple PnP notification may arrive  for several different
     * devices simultaneously.  (see #331320 IanJa)
     */
    pDeviceInfo->pkeHidChangeCompleted = CreateKernelEvent(SynchronizationEvent, FALSE);
    if (pDeviceInfo->pkeHidChangeCompleted == NULL) {
        RIPMSG0(RIP_WARNING,
                "CreateDeviceInfo: failed to create pkeHidChangeCompleted");
        goto CreateFailed;
    }

    /*
     * Link it in
     */
    EnterDeviceInfoListCrit();
    pDeviceInfo->pNext = gpDeviceInfoList;
    gpDeviceInfoList = pDeviceInfo;
    pDeviceInfo->bFlags |= bFlags;

    /*
     * Tell the RIT there is a new device so that it can open it and start
     * reading from it.  This is non-blocking (no GDIAF_PNPWAITING bit set)
     */
    RequestDeviceChange(pDeviceInfo, GDIAF_ARRIVED, TRUE);
    LeaveDeviceInfoListCrit();

    EXITATOMICCHECK();
    return pDeviceInfo;

CreateFailed:

    if (pDeviceInfo) {
        if (pDeviceInfo->ustrName.Buffer) {
            UserFreePool(pDeviceInfo->ustrName.Buffer);
        }
        UserFreePool(pDeviceInfo);
    }

    ENDATOMICCHECK();
    return NULL;
}


/***************************************************************************\
* DeviceClassNotify
*
* This gets called when an input device is attached or detached.
* If this happens during initialization (for mice already connected) we
* come here by in the context of the RIT.  If hot-(un)plugging a mouse,
* then we are called on a thread from the Kernel process.
*
* History:
* 10-20-97  IanJa   Taken from some old code of KenRay's
\***************************************************************************/
NTSTATUS
DeviceClassNotify (
    IN PDEVICE_INTERFACE_CHANGE_NOTIFICATION classChange,
    IN PVOID DeviceType // (context)
    )
{
    DWORD dwDeviceType;

    CheckCritOut();
    dwDeviceType = PtrToUlong( DeviceType );
    TAGMSG2(DBGTAG_PNP, "enter DeviceClassNotify(%lx, %lx)", classChange, dwDeviceType);

    /*
     * Sanity check the DeviceType, and that it matches the InterfaceClassGuid
     */
    UserAssert((dwDeviceType == DEVICE_TYPE_MOUSE) || (dwDeviceType == DEVICE_TYPE_KEYBOARD));
    UserAssert(IsEqualGUID(&classChange->InterfaceClassGuid, aDeviceTemplate[dwDeviceType].pClassGUID));


    TAGMSG3(DBGTAG_PNP | RIP_THERESMORE, " Event GUID %lx, %x, %x",
            classChange->Event.Data1,
            classChange->Event.Data2,
            classChange->Event.Data3);
    TAGMSG8(DBGTAG_PNP | RIP_THERESMORE, " %2x%2x%2x%2x%2x%2x%2x%2x",
            classChange->Event.Data4[0], classChange->Event.Data4[1],
            classChange->Event.Data4[2], classChange->Event.Data4[3],
            classChange->Event.Data4[4], classChange->Event.Data4[5],
            classChange->Event.Data4[6], classChange->Event.Data4[7]);
    TAGMSG4(DBGTAG_PNP | RIP_THERESMORE, " InterfaceClassGuid %lx, %lx, %lx, %lx",
            ((DWORD *)&(classChange->InterfaceClassGuid))[0],
            ((DWORD *)&(classChange->InterfaceClassGuid))[1],
            ((DWORD *)&(classChange->InterfaceClassGuid))[2],
            ((DWORD *)&(classChange->InterfaceClassGuid))[3]);
    TAGMSG1(DBGTAG_PNP | RIP_THERESMORE, " SymbolicLinkName %ws", classChange->SymbolicLinkName->Buffer);

    if (IsEqualGUID(&classChange->Event, &GUID_DEVICE_INTERFACE_ARRIVAL)) {

        // A new hid device class association has arrived
        EnterCrit();
        CreateDeviceInfo(dwDeviceType, classChange->SymbolicLinkName, 0);
        LeaveCrit();
        TAGMSG0(DBGTAG_PNP, "=== CREATED ===");
    }

    return STATUS_SUCCESS;
}

/****************************************************************************\
* If a device class "all-for-one" setting (ConnectMultiplePorts) is on,
* then we just open the device the old (non-PnP) way and return TRUE.  (As a
* safety feature we also do this if gpWin32kDriverObject is NULL, because this
* driver object is needed to register for PnP device class notifications)
* Otherwise, return FALSE so we can continue and register for Arrival/Departure
* notifications.
*
* This code was originally intended to be temporary until ConnectMultiplePorts
* was finally turned off.
* But now I think we have to keep it for backward compatibility with
* drivers that filter Pointer/KeyboardClass0 and/or those that replace
* Pointer/KeyboardClass0 by putting a different name in the registry under
* System\CurrentControlSet\Services\RIT\mouclass (or kbbclass)
\****************************************************************************/
BOOL
OpenMultiplePortDevice(DWORD DeviceType)
{
    WCHAR awchDeviceName[MAX_PATH];
    UNICODE_STRING DeviceName;
    PDEVICE_TEMPLATE pDevTpl;
    PDEVICEINFO pDeviceInfo;
    PWCHAR pwchNameIndex;

    UINT uiConnectMultiplePorts = 0;

    CheckCritIn();

    if (DeviceType <= DEVICE_TYPE_MAX) {
        pDevTpl = &aDeviceTemplate[DeviceType];
    } else {
        RIPMSG1(RIP_ERROR, "OpenMultiplePortDevice(%d) - unknown type", DeviceType);
        return FALSE;
    }

    /*
     * Note that we don't need to FastOpenUserProfileMapping() here since
     * uiRegistrySection (PMAP_MOUCLASS_PARAMS/PMAP_KBDCLASS_PARAMS) is a
     * machine setiing, not a user setting.
     */
    uiConnectMultiplePorts = FastGetProfileDwordW(NULL,
            pDevTpl->uiRegistrySection, L"ConnectMultiplePorts", 0);

    /*
     * Open the device for read access.
     */
    if (uiConnectMultiplePorts || (gpWin32kDriverObject == NULL)) {
        /*
         * Find out if there is a name substitution in the registry.
         * Note that we don't need to FastOpenUserProfileMapping() here since
         * PMAP_INPUT is a machine setting, not a user setting.
         */
        FastGetProfileStringW(NULL,
                PMAP_INPUT,
                pDevTpl->pwszClassName,
                pDevTpl->pwszDefDevName, // if no substitution, use this default
                awchDeviceName,
                sizeof(awchDeviceName)/sizeof(WCHAR));

        RtlInitUnicodeString(&DeviceName, awchDeviceName);

        pDeviceInfo = CreateDeviceInfo(DeviceType, &DeviceName, GDIF_NOTPNP);
        if (pDeviceInfo) {
            return TRUE;
        }
    } else {
        DeviceName.Length = 0;
        DeviceName.MaximumLength = sizeof(awchDeviceName);
        DeviceName.Buffer = awchDeviceName;

        RtlAppendUnicodeToString(&DeviceName, pDevTpl->pwszLegacyDevName);
        pwchNameIndex = &DeviceName.Buffer[(DeviceName.Length / sizeof(WCHAR)) - 1];
        for (*pwchNameIndex = L'0'; *pwchNameIndex <= L'9'; (*pwchNameIndex)++) {
            CreateDeviceInfo(DeviceType, &DeviceName, GDIF_NOTPNP);
        }
    }

    return FALSE;
}


/***************************************************************************\
* RegisterForDeviceClassNotifications
*   Get ready to receive notifications that a mouse or keyboard is plugged in
*   or removed, then request notifications by registering for them.
*
* History:
* 10-20-97  IanJa   Taken from ntos\io\pnpinit.c
\***************************************************************************/
NTSTATUS
xxxRegisterForDeviceClassNotifications()
{
    IO_NOTIFICATION_EVENT_CATEGORY eventCategory;
    ULONG                          eventFlags;
    PVOID                          RegistrationEntry;
    NTSTATUS                       Status;
    UNICODE_STRING                 ustrDriverName;
    DWORD                          DeviceType;



    CheckCritIn();

    TAGMSG0(DBGTAG_PNP, "enter xxxRegisterForDeviceClassNotifications()");

    /*
     * Remote hydra session indicates CreateDeviceInfo in xxxRemoteReconnect
     */
    UserAssert(!gbRemoteSession);

    /*
     * This must be done before devices are registered for device notifications
     * which will occur as a result of CreateDeviceInfo()...
     */
    RtlInitUnicodeString(&ustrDriverName, L"\\Driver\\Win32k");
    Status = IoCreateDriver(&ustrDriverName, Win32kPnPDriverEntry);

    TAGMSG1(DBGTAG_PNP | RIP_THERESMORE, "IoCreateDriver returned status = %lx", Status);
    TAGMSG1(DBGTAG_PNP, "gpWin32kDriverObject = %lx", gpWin32kDriverObject);

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_ERROR, "IoCreateDriver failed, status %lx", Status);
        Status = STATUS_SUCCESS;
    }

    UserAssert(gpWin32kDriverObject);

    //
    // We are only interested in DeviceClasses changing.
    //
    eventCategory = EventCategoryDeviceInterfaceChange;

    //
    // We want to be notified for all devices that are in the system.
    // those that are know now, and those that will arive later.
    // This allows us to have one code path for adding devices, and eliminates
    // the nasty race condition.  If we were only interested in the devices
    // that exist at this one moment in time, and not future devices, we
    // would call IoGetDeviceClassAssociations.
    //
    eventFlags = PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES;

    /*
     * For all input device types:
     *  If they are Multiple Port Devices (ie: not PnP) just open them
     *  Else Register them for PnP notifications (they will be opened when the
     *       arrival notification arrives.
     * If devices are already attached, we will received immediate notification
     * during the call to IoRegisterPlugPlayNotification, so we must LeaveCrit
     * because the callback routine DeviceClassNotify expects it.
     */
    for (DeviceType = 0; DeviceType <= DEVICE_TYPE_MAX; DeviceType++) {
        if (!OpenMultiplePortDevice(DeviceType)) {
            /*
             * Make the registration
             */

            TAGMSG1(DBGTAG_PNP, "Registering device type %d", DeviceType);
            LeaveCrit(); // for DeviceClassNotify
            Status = IoRegisterPlugPlayNotification (
                         eventCategory,
                         eventFlags,
                         (PVOID)aDeviceTemplate[DeviceType].pClassGUID,
                         gpWin32kDriverObject,
                         (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)DeviceClassNotify,
                         LongToPtr( DeviceType ),
                         &RegistrationEntry);
            EnterCrit();
            TAGMSG1(DBGTAG_PNP, "Registration returned status %lx", Status);
            if (!NT_SUCCESS(Status)) {
                RIPMSG2(RIP_ERROR, "IoRegisterPlugPlayNotification(%d) failed, status %lx",
                        DeviceType, Status);
            }
        }
    }

    // Now Register for CD_ROM notifications
    LeaveCrit(); // for DeviceClassNotify

    Status = IoRegisterPlugPlayNotification (
                 eventCategory,
                 eventFlags,
                 (PVOID) &CdRomClassGuid,
                 gpWin32kDriverObject,
                 (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)DeviceClassCDROMNotify,
                 NULL,
                 &RegistrationEntry);
    EnterCrit();



    return Status;
}



/***************************************************************************\
* QueryDeviceInfo
*
* Query the device information.  This function is an async function,
* so be sure any buffers it uses aren't allocated on the stack!
*
* If this is an asynchronous IOCTL, perhaps we should be waiting on
* the file handle or on an event for it to succeed?
*
* This function must called by the RIT, not directly by PnP notification
* (else the handle we issue the IOCTL on will be invalid)
*
* History:
* 01-20-99 IanJa        Created.
\***************************************************************************/
NTSTATUS
QueryDeviceInfo(
    PDEVICEINFO pDeviceInfo)
{
    NTSTATUS Status;
    PDEVICE_TEMPLATE pDevTpl = &aDeviceTemplate[pDeviceInfo->type];

#ifdef DIAGNOSE_IO
    pDeviceInfo->AttrStatus =
#endif
    Status = ZwDeviceIoControlFile(pDeviceInfo->handle, NULL, NULL, NULL,
                 &pDeviceInfo->iosb,
                 pDevTpl->IOCTL_Attr,
                 NULL, 0,
                 (PVOID)((PBYTE)pDeviceInfo + pDevTpl->offAttr),
                 pDevTpl->cbAttr);

    if (!NT_SUCCESS(Status)) {
        RIPMSG2(RIP_WARNING, "QueryDeviceInfo(%p): IOCTL failed - Status %lx",
                pDeviceInfo, Status);
    }
    TAGMSG1(DBGTAG_PNP, "IOCTL_*_QUERY_ATTRIBUTES returns Status %lx", Status);
    return Status;
}


/***************************************************************************\
* OpenDevice
*
* This function opens an input device for USER, mouse or keyboard.
*
*
* Return value
*   BOOL did the operation succeed?
*
* When trying to open a HYDRA client's mouse (or kbd?), pDeviceInfo->ustrName
* is NULL.
*
* This function must called by the RIT, not directly by PnP
* notification (that way the handle we are about to create will be in the right
* our process)
*
* History:
* 11-26-90 DavidPe      Created.
* 01-07-98 IanJa        Plug & Play
* 04-17-98 IanJa        Only open mice in RIT context.
\***************************************************************************/
BOOL OpenDevice(PDEVICEINFO pDeviceInfo)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    CheckCritIn();
    UserAssert((PtiCurrentShared() == gptiRit) || (PtiCurrentShared() == gTermIO.ptiDesktop));

    TAGMSG3(DBGTAG_PNP, "OpenDevice(): Opening type %d (%lx %ws)",
            pDeviceInfo->type, pDeviceInfo->handle, pDeviceInfo->ustrName.Buffer);

#ifdef DIAGNOSE_IO
    pDeviceInfo->OpenerProcess = PsGetCurrentThread()->Cid.UniqueProcess;
#endif

    if (gbRemoteSession) {

        /*
         * For other than the console, the mouse handle is
         * set before createwinstation.
         */
        UserAssert(pDeviceInfo->ustrName.Buffer == NULL);

        pDeviceInfo->bFlags |= GDIF_NOTPNP;

        switch (pDeviceInfo->type) {
        case DEVICE_TYPE_MOUSE:
            pDeviceInfo->handle = ghRemoteMouseChannel;
            if (ghRemoteMouseChannel == NULL) {
               return FALSE;
            }
            break;
        case DEVICE_TYPE_KEYBOARD:
            pDeviceInfo->handle = ghRemoteKeyboardChannel;
            if (ghRemoteKeyboardChannel == NULL) {
               return FALSE;
            }
            break;
        default:
            RIPMSG2(RIP_ERROR, "Unknown device type %d DeviceInfo %#p",
                    pDeviceInfo->type, pDeviceInfo);
            return FALSE;
        }
    } else {
        InitializeObjectAttributes(&ObjectAttributes, &(pDeviceInfo->ustrName), 0, NULL, NULL);

#ifdef DIAGNOSE_IO
        pDeviceInfo->OpenStatus =
#endif
        Status = ZwCreateFile(&pDeviceInfo->handle, FILE_READ_DATA | SYNCHRONIZE,
                &ObjectAttributes, &pDeviceInfo->iosb, NULL, 0, FILE_SHARE_WRITE, FILE_OPEN_IF, 0, NULL, 0);

        TAGMSG2(DBGTAG_PNP, "ZwCreateFile returns handle %lx, Status %lx",
                pDeviceInfo->handle, Status);

        if (!NT_SUCCESS(Status)) {
            if ((pDeviceInfo->bFlags & GDIF_NOTPNP) == 0) {
                /*
                 * Don't warn about PS/2 mice: the PointerClassLegacy0 -9 and
                 * KeyboardClassLegacy0 - 9 will usually fail to be created
                 */
                RIPMSG1(RIP_WARNING, "OpenDevice: ZwCreateFile failed with Status %lx", Status);
            }
            /*
             * Don't FreeDeviceInfo here because that alters gpDeviceInfoList
             * which our caller, ProcessDeviceChanges, is traversing.
             * Instead, let ProcessDeviceChanges do it.
             */
            return FALSE;
        }
    }

    Status = QueryDeviceInfo(pDeviceInfo);


    return NT_SUCCESS(Status);
}

VOID CloseDevice(PDEVICEINFO pDeviceInfo)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    CheckCritIn();


    TAGMSG3(DBGTAG_PNP, "CloseDevice(): closing type %d (%lx %ws)",
            pDeviceInfo->type, pDeviceInfo->handle, pDeviceInfo->ustrName.Buffer);
    if (pDeviceInfo->handle) {
        UserAssert(pDeviceInfo->OpenerProcess == PsGetCurrentThread()->Cid.UniqueProcess);
        ZwCancelIoFile(pDeviceInfo->handle, &IoStatusBlock);
        UserAssertMsg2(NT_SUCCESS(IoStatusBlock.Status), "NtCancelIoFile handle %x failed status %#x",
                 pDeviceInfo->handle, IoStatusBlock.Status);
        Status = ZwClose(pDeviceInfo->handle);
        UserAssertMsg2(NT_SUCCESS(Status), "ZwClose handle %x failed status %#x",
                pDeviceInfo->handle, Status);
        pDeviceInfo->handle = 0;
    } else {
        /*
         * Assert the IO was cancelled or we tried to read the device
         * after the first close (which set the handle to 0 - an invalid handle)
         */
        UserAssert((pDeviceInfo->iosb.Status == STATUS_CANCELLED) ||
                   (pDeviceInfo->ReadStatus == STATUS_INVALID_HANDLE));
    }
}

/*****************************************************************************\
* RegisterForDeviceChangeNotifications()
*
* Device Notifications such as QueryRemove, RemoveCancelled, RemoveComplete
* tell us what is going on with the mouse.
* To register for device notifications:
* (1) Obtain a pointer to the device object (pFileObject)
* (2) Register for target device change notifications, saving the
*     notification handle (which we will need in order to deregister)
*
* It doesn't matter too much if this fails: we just won't be able to eject the
* hardware via the UI very successfully. (We can still just yank it though).
* This will also fail if the ConnectMultiplePorts was set for this device.
*
* 1998-10-05 IanJa    Created
\*****************************************************************************/
BOOL RegisterForDeviceChangeNotifications(PDEVICEINFO pDeviceInfo)
{
    PFILE_OBJECT pFileObject;
    NTSTATUS Status;

    /*
     * In or Out of User critical section:
     * In when called from RIT ProcessDeviceChanges();
     * Out when called from the DeviceNotify callback
     */
    CheckCritIn();
    UserAssert((PtiCurrentShared() == gptiRit) || (PtiCurrentShared() == gTermIO.ptiDesktop));
    UserAssert(pDeviceInfo->handle);
    UserAssert(pDeviceInfo->OpenerProcess == PsGetCurrentThread()->Cid.UniqueProcess);

    if (pDeviceInfo->bFlags & GDIF_NOTPNP) {
        return TRUE;
    }
    Status = ObReferenceObjectByHandle(pDeviceInfo->handle,
                                       0,
                                       NULL,
                                       KernelMode,
                                       (PVOID)&pFileObject,
                                       NULL);
    if (NT_SUCCESS(Status)) {
        Status = IoRegisterPlugPlayNotification (
                EventCategoryTargetDeviceChange,  // EventCategory
                0,                                // EventCategoryFlags
                (PVOID)pFileObject,               // EventCategoryData
                gpWin32kDriverObject,             // DriverObject
                // (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)
                DeviceNotify,
                (PVOID)pDeviceInfo,                       // Context
                &pDeviceInfo->NotificationEntry);
        ObDereferenceObject(pFileObject);
        if (!NT_SUCCESS(Status)) {
            // This is only OK if ConnectMultiplePorts is on (ie: not a PnP device)
            RIPMSG3(RIP_ERROR,
                    "IoRegisterPlugPlayNotification failed on device %.*ws, status %lx, email DoronH : #333453",
                    pDeviceInfo->ustrName.Length / sizeof(WCHAR),
                    pDeviceInfo->ustrName.Buffer, Status);
        }
    } else {
        // non-catastrophic error (won't be able to remove device)
        RIPMSG2(RIP_ERROR, "Can't get pFileObject from handle %lx, status %lx",
                pDeviceInfo->handle, Status);
    }

    return NT_SUCCESS(Status);
}


BOOL UnregisterForDeviceChangeNotifications(PDEVICEINFO pDeviceInfo)
{
    NTSTATUS Status;

    CheckCritIn();
    UserAssert((PtiCurrentShared() == gptiRit) || (PtiCurrentShared() == gTermIO.ptiDesktop));
    UserAssert(pDeviceInfo->NotificationEntry);
    UserAssert(pDeviceInfo->OpenerProcess == PsGetCurrentThread()->Cid.UniqueProcess);

    if (pDeviceInfo->NotificationEntry == NULL) {
        /*
         * This happens for non-PnP devices or if the earlier
         * IoRegisterPlugPlayNotification() failed.  Return now since
         * IoUnregisterPlugPlayNotification(NULL) will bluescreen.
         */
        return TRUE;
    }

    // non-PnP devices should not have any NotificationEntry:
    UserAssert((pDeviceInfo->bFlags & GDIF_NOTPNP) == 0);

    TAGMSG3(DBGTAG_PNP, "UnregisterForDeviceChangeNotifications(): type %d (%lx %ws)",
            pDeviceInfo->type, pDeviceInfo, pDeviceInfo->ustrName.Buffer);
    Status = IoUnregisterPlugPlayNotification(pDeviceInfo->NotificationEntry);
    if (!NT_SUCCESS(Status)) {
        RIPMSG2(RIP_ERROR,
                "IoUnregisterPlugPlayNotification failed Status = %lx, DEVICEINFO %lx",
                Status, pDeviceInfo);
        return FALSE;
    }
    pDeviceInfo->NotificationEntry = 0;
    return TRUE;
}


/***************************************************************************\
* Handle device notifications such as QueryRemove, CancelRemove etc.
*
* Execution Context:
*    when yanked:  a non-WIN32 thread.
*    via UI:       ??? (won't see this except from laptop being undocked?)
*
* History:
\***************************************************************************/
NTSTATUS DeviceNotify(
    IN PPLUGPLAY_NOTIFY_HDR pNotification,
    IN PDEVICEINFO pDeviceInfo)  // should the context be a kernel address?
{
    USHORT usAction;

#if DBG
    {
        PDEVICEINFO pDeviceInfoDbg;
        for (pDeviceInfoDbg = gpDeviceInfoList; pDeviceInfoDbg; pDeviceInfoDbg = pDeviceInfoDbg->pNext) {
            if (pDeviceInfoDbg == pDeviceInfo) {
                break;
            }
        }
        UserAssertMsg1(pDeviceInfoDbg != NULL, "Notification for unlisted DEVICEINFO %lx", pDeviceInfo);
    }
#endif

    CheckCritOut();

    UserAssert(!gbRemoteSession);

    TAGMSG1(DBGTAG_PNP | RIP_THERESMORE, "DeviceNotify >>> %lx", pDeviceInfo);

    UserAssert(pDeviceInfo->OpenerProcess != PsGetCurrentThread()->Cid.UniqueProcess);
    UserAssert(pDeviceInfo->usActions == 0);

    if (IsEqualGUID(&pNotification->Event, &GUID_TARGET_DEVICE_QUERY_REMOVE)) {
        TAGMSG0(DBGTAG_PNP | RIP_NONAME, "QueryRemove");
        usAction = GDIAF_QUERYREMOVE;

    } else if (IsEqualGUID(&pNotification->Event, &GUID_TARGET_DEVICE_REMOVE_CANCELLED)) {
        TAGMSG0(DBGTAG_PNP | RIP_NONAME, "RemoveCancelled");
        usAction = GDIAF_REMOVECANCELLED;

    } else if (IsEqualGUID(&pNotification->Event, &GUID_TARGET_DEVICE_REMOVE_COMPLETE)) {
        TAGMSG1(DBGTAG_PNP | RIP_NONAME, "RemoveComplete (process %#x)", PsGetCurrentThread()->Cid.UniqueProcess);
        usAction = GDIAF_DEPARTED;

    } else {
        TAGMSG4(DBGTAG_PNP | RIP_NONAME, "GUID Unknown: %lx:%lx:%lx:%x...",
                pNotification->Event.Data1, pNotification->Event.Data2,
                pNotification->Event.Data3, pNotification->Event.Data4[0]);
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * Signal the RIT to ProcessDeviceChanges()
     * Wait for completion according to the GDIAF_PNPWAITING bit
     */
    CheckCritOut();
    CheckDeviceInfoListCritOut();
    RequestDeviceChange(pDeviceInfo, (USHORT)(usAction | GDIAF_PNPWAITING), FALSE);

    return STATUS_SUCCESS;
}


/***************************************************************************\
* StartDeviceRead
*
* This function makes an asynchronous read request to the input device driver,
* unless the device has been marked for destruction (GDIAF_FREEME)
*
* Returns:
*   The next DeviceInfo on the list if this device was freed: If the caller
*   was not already in the DeviceInfoList critical section, the this must be
*   ignored as it is not safe.
*   NULL if the read succeeded.
*
* History:
* 11-26-90 DavidPe      Created.
* 10-20-98 IanJa        Generalized for PnP input devices
\***************************************************************************/
PDEVICEINFO StartDeviceRead(
    PDEVICEINFO pDeviceInfo)
{
    PDEVICE_TEMPLATE pDevTpl;

    pDeviceInfo->bFlags |= GDIF_READING;

    /*
     * If this device needs freeing, abandon reading now and request the free.
     */
    if (pDeviceInfo->usActions & GDIAF_FREEME) {
        BOOL fPreviouslyAcquired = ExIsResourceAcquiredExclusiveLite(gpresDeviceInfoList);
        if (!fPreviouslyAcquired) {
            EnterDeviceInfoListCrit();
        }
        pDeviceInfo->bFlags &= ~GDIF_READING;
        pDeviceInfo = FreeDeviceInfo(pDeviceInfo);
        if (!fPreviouslyAcquired) {
            LeaveDeviceInfoListCrit();
        }
        return pDeviceInfo;
    }

    /*
     * Initialize in case read fails
     */
    pDeviceInfo->iosb.Status = STATUS_UNSUCCESSFUL; // catch concurrent writes?
    pDeviceInfo->iosb.Information = 0;

    pDevTpl = &aDeviceTemplate[pDeviceInfo->type];

    UserAssert(pDeviceInfo->OpenerProcess == PsGetCurrentThread()->Cid.UniqueProcess);
    LOGTIME(pDeviceInfo->timeStartRead);
#ifdef DIAGNOSE_IO
    pDeviceInfo->nReadsOutstanding++;
#endif
    pDeviceInfo->ReadStatus = ZwReadFile(
            pDeviceInfo->handle,
            NULL,                // hReadEvent
            InputApc,            // InputApc()
            pDeviceInfo,         // ApcContext
            &pDeviceInfo->iosb,
            (PVOID)((PBYTE)pDeviceInfo + pDevTpl->offData),
            pDevTpl->cbData,
            PZERO(LARGE_INTEGER), NULL);
    LOGTIME(pDeviceInfo->timeEndRead);

#if DBG
    if (pDeviceInfo->bFlags & GDIF_DBGREAD) {
        TAGMSG2(DBGTAG_PNP, "ZwReadFile of Device handle %lx returned status %lx",
                pDeviceInfo->handle, pDeviceInfo->ReadStatus);
    }
#endif

    if (!NT_SUCCESS(pDeviceInfo->ReadStatus)) {
        BOOL fPreviouslyAcquired = ExIsResourceAcquiredExclusiveLite(gpresDeviceInfoList);
        if (!fPreviouslyAcquired) {
            EnterDeviceInfoListCrit();
        }

        /*
         * If insufficient resources, retry the read the next time the RIT
         * wakes up for the ID_TIMER event by incrementing gnRetryReadInput
         * (Cheaper than setting our own timer),
         * Else just abandon reading.
         */
        if (pDeviceInfo->ReadStatus == STATUS_INSUFFICIENT_RESOURCES) {
            if (pDeviceInfo->nRetryRead++ < MAXIMUM_READ_RETRIES) {
                pDeviceInfo->usActions |= GDIAF_RETRYREAD;
                gnRetryReadInput++;
            }
        } else {
            pDeviceInfo->bFlags &= ~GDIF_READING;
        }

#ifdef DIAGNOSE_IO
        pDeviceInfo->nReadsOutstanding--;
#endif
        if (!fPreviouslyAcquired) {
            LeaveDeviceInfoListCrit();
        }
    } else {
        pDeviceInfo->nRetryRead = 0;
    }

    if (!ISTS() && !NT_SUCCESS(pDeviceInfo->ReadStatus))
        RIPMSG2(RIP_WARNING, "StartDeviceRead %#p failed Status %#x",
                pDeviceInfo, pDeviceInfo->ReadStatus);

    return NULL;
}

 VOID ProcessDeviceChanges(DWORD DeviceType)
{
    PDEVICEINFO pDeviceInfo;
    USHORT usOriginalActions;
#if DBG
    int nChanges = 0;
    ULONG timeStartReadPrev;
#endif

    /*
     * Reset summary information for all Mice and Keyboards
     */
    DWORD nMice = 0;
    DWORD nWheels = 0;
    DWORD nMaxButtons = 0;
    int   nKeyboards = 0;

    CheckCritIn();
    BEGINATOMICCHECK();
    UserAssert((PtiCurrentShared() == gptiRit) || (PtiCurrentShared() == gTermIO.ptiDesktop));

    EnterDeviceInfoListCrit();
    BEGINATOMICDEVICEINFOLISTCHECK();

    /*
     * Look for devices to Create (those which have newly arrived)
     * and for devices to Terminate (these which have just departed)
     * and for device change notifications.
     * Make sure the actions are processed in the right order in case we
     * are being asked for more than one action per device: for example,
     * we sometimes get QueryRemove followed quickly by RemoveCancelled
     * and both actions arrive here together: we should do them in the
     * correct order.
     */
    pDeviceInfo = gpDeviceInfoList;
    while (pDeviceInfo) {
        if (pDeviceInfo->type != DeviceType) {
            pDeviceInfo = pDeviceInfo->pNext;
            continue;
        }

        usOriginalActions = pDeviceInfo->usActions;
        UserAssert((usOriginalActions == 0) || (usOriginalActions & ~GDIAF_PNPWAITING));

        /*
         * Refresh Mouse:
         * We read a MOUSE_ATTRIBUTES_CHANGED flag when a PS/2 mouse
         * is plugged back in. Find out the attributes of the device.
         */
        if (pDeviceInfo->usActions & GDIAF_REFRESH_MOUSE) {
            pDeviceInfo->usActions &= ~GDIAF_REFRESH_MOUSE;

            UserAssert(pDeviceInfo->type == DEVICE_TYPE_MOUSE);
#if DBG
            nChanges++;
#endif
            TAGMSG1(DBGTAG_PNP, "QueryDeviceInfo: %lx", pDeviceInfo);
            QueryDeviceInfo(pDeviceInfo);
        }

        /*
         * QueryRemove:
         * Close the file object, but retain the DEVICEINFO struct and the
         * registration in case we later get a RemoveCancelled.
         */
        if (pDeviceInfo->usActions & GDIAF_QUERYREMOVE) {
            pDeviceInfo->usActions &= ~GDIAF_QUERYREMOVE;
#if DBG
            nChanges++;
#endif
            TAGMSG1(DBGTAG_PNP, "QueryRemove: %lx", pDeviceInfo);
            CloseDevice(pDeviceInfo);
        }

        /*
         * New device arrived or RemoveCancelled:
         * If new device, Open it, register for notifications and start reading
         * If RemoveCancelled, unregister the old notfications first
         */
        if (pDeviceInfo->usActions & (GDIAF_ARRIVED | GDIAF_REMOVECANCELLED)) {
            // Reopen the file object, (this is a new file object, of course),
            // Unregister for the old file, register with this new one.
            if (pDeviceInfo->usActions & GDIAF_REMOVECANCELLED) {
                pDeviceInfo->usActions &= ~GDIAF_REMOVECANCELLED;
#if DBG
                nChanges++;
#endif
                TAGMSG1(DBGTAG_PNP, "RemoveCancelled: %lx", pDeviceInfo);
                UnregisterForDeviceChangeNotifications(pDeviceInfo);
            }

#if DBG
            if (pDeviceInfo->usActions & GDIAF_ARRIVED) {
                nChanges++;
            }
#endif

            pDeviceInfo->usActions &= ~GDIAF_ARRIVED;
            if (OpenDevice(pDeviceInfo)) {
                PDEVICEINFO pDeviceInfoNext;
                RegisterForDeviceChangeNotifications(pDeviceInfo);
                
                if (!(gbRemoteSession && (pDeviceInfo->usActions & GDIAF_RECONNECT))) {
                    
                    pDeviceInfoNext = StartDeviceRead(pDeviceInfo);
                    if (pDeviceInfoNext) {
                        /*
                         * pDeviceInfo wasa freed, move onto the next
                         */
                        pDeviceInfo = pDeviceInfoNext;
                        continue;
                    }
                }
                pDeviceInfo->usActions &= ~GDIAF_RECONNECT;
                
            } else {
                /*
                 * If the Open failed, we free the device here, and move on to
                 * the next device.
                 * Assert to catch re-open failure upon RemoveCancelled.
                 */
#if DBG
                if ((usOriginalActions & GDIAF_ARRIVED) == 0) {
                    RIPMSG2(RIP_WARNING, "Re-Open %#p failed status %x during RemoveCancelled",
                            pDeviceInfo, pDeviceInfo->OpenStatus);
                }
#endif
                pDeviceInfo = FreeDeviceInfo(pDeviceInfo);
                continue;
            }
        }

        /*
         * RemoveComplete:
         * Close the file object, if you have not already done so, Unregister.
         * FreeDeviceInfo here (which will actually request a free from the
         * reader or the PnP requestor thread), and move on to the next device.
         */
        if (pDeviceInfo->usActions & GDIAF_DEPARTED) {
            pDeviceInfo->usActions &= ~GDIAF_DEPARTED;
#if DBG
            nChanges++;
#endif
            TAGMSG1(DBGTAG_PNP, "RemoveComplete: %lx (process %#x)", pDeviceInfo);
            CloseDevice(pDeviceInfo);
            UnregisterForDeviceChangeNotifications(pDeviceInfo);
            pDeviceInfo = FreeDeviceInfo(pDeviceInfo);
            continue;
        }

        if (pDeviceInfo->usActions & GDIAF_IME_STATUS) {
            pDeviceInfo->usActions &= ~GDIAF_IME_STATUS;
#if DBG
            nChanges++;
#endif
            if ((pDeviceInfo->type == DEVICE_TYPE_KEYBOARD) && (pDeviceInfo->handle)) {
                if (FUJITSU_KBD_CONSOLE(pDeviceInfo->keyboard.Attr.KeyboardIdentifier) ||
                    (gbRemoteSession &&
                     FUJITSU_KBD_REMOTE(gRemoteClientKeyboardType))
                   ) {
                    /*
                     * Fill up the KEYBOARD_IME_STATUS structure.
                     */
                    ZwDeviceIoControlFile(pDeviceInfo->handle, NULL, NULL, NULL,
                            &giosbKbdControl, IOCTL_KEYBOARD_SET_IME_STATUS,
                            (PVOID)&gKbdImeStatus, sizeof(gKbdImeStatus), NULL, 0);
                }
            }
        }

        if (pDeviceInfo->usActions & GDIAF_RETRYREAD) {
            PDEVICEINFO pDeviceInfoNext;
            pDeviceInfo->usActions &= ~GDIAF_RETRYREAD;
            UserAssert(pDeviceInfo->ReadStatus == STATUS_INSUFFICIENT_RESOURCES);
#if DBG
            timeStartReadPrev = pDeviceInfo->timeStartRead;
#endif
            TAGMSG2(DBGTAG_PNP, "Retry Read %#p after %lx ticks",
                    pDeviceInfo, pDeviceInfo->timeStartRead - timeStartReadPrev);
            pDeviceInfoNext = StartDeviceRead(pDeviceInfo);
            if (pDeviceInfoNext) {
                /*
                 * pDeviceInfo wasa freed, move onto the next
                 */
                pDeviceInfo = pDeviceInfoNext;
                continue;
            }
        }

        /*
         * Gather summary information on open devices
         */
        if (pDeviceInfo->handle) {
            switch (pDeviceInfo->type) {
            case DEVICE_TYPE_MOUSE:
                UserAssert(PtiCurrentShared() == gTermIO.ptiDesktop);
                if (pDeviceInfo->usActions & GDIAF_REFRESH_MOUSE) {
                    pDeviceInfo->usActions &= ~GDIAF_REFRESH_MOUSE;
#if DBG
                    nChanges++;
#endif
                }
                nMice++;
                nMaxButtons = max(nMaxButtons, pDeviceInfo->mouse.Attr.NumberOfButtons);
                switch(pDeviceInfo->mouse.Attr.MouseIdentifier) {
                case WHEELMOUSE_I8042_HARDWARE:
                case WHEELMOUSE_SERIAL_HARDWARE:
                case WHEELMOUSE_HID_HARDWARE:
                    nWheels++;
                }
                break;

            case DEVICE_TYPE_KEYBOARD:
                UserAssert(PtiCurrentShared() == gptiRit);
                {
                    NTSTATUS Status;
                    // BUG BUG: why query each keyboard for indicators? They should all
                    // be the same.
#ifdef DIAGNOSE_IO
                    gKbdIoctlLEDSStatus =
#endif
                    Status = ZwDeviceIoControlFile(pDeviceInfo->handle, NULL, NULL, NULL,
                            &giosbKbdControl, IOCTL_KEYBOARD_QUERY_INDICATORS,
                            NULL, 0,
                            (PVOID)&gklpBootTime, sizeof(gklpBootTime));
                    UserAssertMsg2(NT_SUCCESS(Status),
                            "IOCTL_KEYBOARD_QUERY_INDICATORS failed: DeviceInfo %#x, Status %#x",
                             pDeviceInfo, Status);
                    // BUG BUG, Don't handle multiple keyboards properly: the last
                    // keyboard on the list is the one whose attributes we report!
                    gKeyboardInfo = pDeviceInfo->keyboard.Attr;
                }
                nKeyboards++;
                break;

            default:
                // Add code for a new type of input device here
                RIPMSG2(RIP_ERROR, "pDeviceInfo %#p has strange type %d",
                        pDeviceInfo, pDeviceInfo->type);
                break;
            }
        }

        /*
         * Notify the PnP thread that a change has been completed
         */
        if (usOriginalActions & GDIAF_PNPWAITING) {
            KeSetEvent(pDeviceInfo->pkeHidChangeCompleted, EVENT_INCREMENT, FALSE);
        }

        pDeviceInfo = pDeviceInfo->pNext;
    }

    ENDATOMICDEVICEINFOLISTCHECK();
    LeaveDeviceInfoListCrit();


    switch (DeviceType) {
    case DEVICE_TYPE_MOUSE:
        /*
         * Apply summary information for Mice
         */
        if (nMice) {
            if (gnMice == 0) {
                /*
                 * We had no mouse before but we have one now: add a cursor
                 */
                SET_GTERMF(GTERMF_MOUSE);
                SYSMET(MOUSEPRESENT) = TRUE;
                SetGlobalCursorLevel(0);
                UserAssert(PpiFromProcess(gpepCSRSS)->ptiList->iCursorLevel == 0);
                UserAssert(PpiFromProcess(gpepCSRSS)->ptiList->pq->iCursorLevel == 0);
                GreMovePointer(gpDispInfo->hDev, gpsi->ptCursor.x, gpsi->ptCursor.y);
            }
        } else {
            if (gnMice != 0) {
                /*
                 * We had a mouse before but we don't now: remove the cursor
                 */
                CLEAR_GTERMF(GTERMF_MOUSE);
                SYSMET(MOUSEPRESENT) = FALSE;
                SetGlobalCursorLevel(-1);
                /*
                 * Don't leave mouse buttons stuck down, clear the global button
                 * state here, otherwise weird stuff might happen.
                 * Also do this in Alt-Tab processing and zzzCancelJournalling.
                 */
#if DBG
                if (gwMouseOwnerButton)
                    RIPMSG1(RIP_WARNING,
                            "gwMouseOwnerButton=%x, being cleared forcibly\n",
                            gwMouseOwnerButton);
#endif
                gwMouseOwnerButton = 0;
            }
        }
        /*
         * Mouse button count represents the number of buttons on the mouse with
         * the most buttons.
         */
        SYSMET(CMOUSEBUTTONS) = nMaxButtons;
        SYSMET(MOUSEWHEELPRESENT) = (nWheels > 0);
        gnMice = nMice;
        break;

    case DEVICE_TYPE_KEYBOARD:
        /*
         * Apply summary information for Keyboards
         */

        if (nKeyboards > gnKeyboards) {
            /*
             * We have more keyboards, let set their LEDs properly
             */
            UpdateKeyLights(FALSE);
        }

        if ((nKeyboards != 0) && (gnKeyboards == 0)) {
            /*
             * We had no keyboard but we have one now: set the system hotkeys.
             */
            SetDebugHotKeys();
        }
        gnKeyboards = nKeyboards;
        break;

    default:
        break;
    }

    ENDATOMICCHECK();
}

/***************************************************************************\
* RequestDeviceChange()
*
* Flag the Device for the specified actions, then set its pkeHidChange to
* trigger the RIT to perform the actions.
* The current thread may not be able to do this if it is a PnP notification
* from another process.
*
* History:
* 01-20-99 IanJa        Created.
\***************************************************************************/
VOID RequestDeviceChange(
    PDEVICEINFO pDeviceInfo,
    USHORT usAction,
    BOOL fInDeviceInfoListCrit)
{
    PDEVICE_TEMPLATE pDevTpl = &aDeviceTemplate[pDeviceInfo->type];
    UserAssert(pDevTpl->pkeHidChange != NULL);
    UserAssert((usAction & GDIAF_FREEME) == 0);
    UserAssert((pDeviceInfo->usActions & GDIAF_PNPWAITING) == 0);

#if DBG
    if (pDeviceInfo->usActions != 0) {
        TAGMSG3(DBGTAG_PNP, "RequestDeviceChange(%#p, %x), but action %x pending",
                pDeviceInfo, usAction, pDeviceInfo->usActions);
    }

    /*
     * We can't ask for synchronized actions to be performed on the Device List
     * if we are holding the Device List lock or the User Critical Section:
     * ProcessDeviceChanges() requires both of these itself.
     */
    if (usAction & GDIAF_PNPWAITING) {
        CheckDeviceInfoListCritOut();
        CheckCritOut();
    }
#endif

    TAGMSG2(DBGTAG_PNP, "RequestDeviceChange(%p, %x)", pDeviceInfo, usAction);

    /*
     * Grab the DeviceInfoList critical section if we don't already have it
     */
    UserAssert(!fInDeviceInfoListCrit == !ExIsResourceAcquiredExclusiveLite(gpresDeviceInfoList));
    if (fInDeviceInfoListCrit) {
        CheckDeviceInfoListCritIn();
        pDeviceInfo->usActions |= usAction;
    } else {
        EnterDeviceInfoListCrit();
        pDeviceInfo->usActions |= usAction;
        LeaveDeviceInfoListCrit();
    }

    if (usAction & GDIAF_PNPWAITING) {
        CheckDeviceInfoListCritOut();
        KeSetEvent(pDevTpl->pkeHidChange, EVENT_INCREMENT, FALSE);
        KeWaitForSingleObject(pDeviceInfo->pkeHidChangeCompleted, WrUserRequest, KernelMode, FALSE, NULL);

        EnterDeviceInfoListCrit();
        /*
         * Assert that nothing else cleared GDIAF_PNPWAITING - only do it here.
         * Check that the action we were waiting for actually occurred.
         */
        UserAssert(pDeviceInfo->usActions & GDIAF_PNPWAITING);
        pDeviceInfo->usActions &= ~GDIAF_PNPWAITING;
        UserAssert((pDeviceInfo->usActions & usAction) == 0);
        if (pDeviceInfo->usActions & GDIAF_FREEME) {
            FreeDeviceInfo(pDeviceInfo);
        }
        LeaveDeviceInfoListCrit();
    } else {
        KeSetEvent(pDevTpl->pkeHidChange, EVENT_INCREMENT, FALSE);
    }
}
