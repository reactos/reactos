/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    devinfo.c

Abstract:

    Device Installer routines dealing with device information sets

Author:

    Lonny McMichael (lonnym) 10-May-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Define the context structure used by the default device comparison
// callback (used by SetupDiRegisterDeviceInfo).
//
typedef struct _DEFAULT_DEVCMP_CONTEXT {

    PCS_RESOURCE NewDevCsResource;
    PCS_RESOURCE CurDevCsResource;
    ULONG        CsResourceSize;    // applies to both buffers.

} DEFAULT_DEVCMP_CONTEXT, *PDEFAULT_DEVCMP_CONTEXT;


//
// Private routine prototypes.
//
DWORD
pSetupOpenAndAddNewDevInfoElem(
    IN  PDEVICE_INFO_SET DeviceInfoSet,
    IN  PCTSTR           DeviceInstanceId,
    IN  BOOL             AllowPhantom,
    IN  CONST GUID      *ClassGuid,              OPTIONAL
    IN  HWND             hwndParent,             OPTIONAL
    OUT PDEVINFO_ELEM   *DevInfoElem,
    IN  BOOL             CheckIfAlreadyThere,
    OUT PBOOL            AlreadyPresent,         OPTIONAL
    IN  BOOL             OpenExistingOnly,
    IN  ULONG            CmLocateFlags,
    IN  PDEVICE_INFO_SET ContainingDeviceInfoSet
    );

DWORD
pSetupAddNewDeviceInfoElement(
    IN  PDEVICE_INFO_SET DeviceInfoSet,
    IN  DEVINST          DevInst,
    IN  CONST GUID      *ClassGuid,
    IN  PCTSTR           Description,             OPTIONAL
    IN  HWND             hwndParent,              OPTIONAL
    IN  DWORD            DiElemFlags,
    IN  PDEVICE_INFO_SET ContainingDeviceInfoSet,
    OUT PDEVINFO_ELEM   *DeviceInfoElement
    );

DWORD
pSetupClassGuidFromDevInst(
    IN  DEVINST DevInst,
    IN  HMACHINE hMachine,
    OUT LPGUID  ClassGuid
    );

DWORD
pSetupDupDevCompare(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA NewDeviceData,
    IN PSP_DEVINFO_DATA ExistingDeviceData,
    IN PVOID            CompareContext
    );

DWORD
pSetupAddInterfaceDeviceToDevInfoElem(
    IN  PDEVICE_INFO_SET        DeviceInfoSet,
    IN  PDEVINFO_ELEM           DevInfoElem,
    IN  CONST GUID             *ClassGuid,
    IN  PTSTR                   InterfaceDeviceName,
    IN  BOOL                    IsActive,
    IN  BOOL                    StoreTruncateNode,
    IN  BOOL                    OpenExistingOnly,
    OUT PINTERFACE_DEVICE_NODE *InterfaceDeviceNode  OPTIONAL
    );

DWORD
_SetupDiOpenInterfaceDevice(
    IN  HDEVINFO                  DeviceInfoSet,
    IN  PTSTR                     DevicePath,
    IN  DWORD                     OpenFlags,
    OUT PSP_DEVICE_INTERFACE_DATA InterfaceDeviceData OPTIONAL
    );

DWORD
pSetupGetDevInstNameAndStatusForInterfaceDevice(
    IN  HKEY   hKeyInterfaceClass,
    IN  PCTSTR InterfaceDeviceName,
    OUT PTSTR  OwningDevInstName,     OPTIONAL
    IN  DWORD  OwningDevInstNameSize,
    OUT PBOOL  IsActive               OPTIONAL
    );

BOOL
pSetupDiGetOrSetDeviceInfoContext(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            InContext,
    OUT PDWORD           OutContext      OPTIONAL
    );


HDEVINFO
WINAPI
SetupDiCreateDeviceInfoList(
    IN CONST GUID *ClassGuid, OPTIONAL
    IN HWND        hwndParent OPTIONAL
    )
/*++

Routine Description:

    This API creates an empty device information set that will contain device
    device information member elements.  This set may be associated with an
    optionally-specified class GUID.

Arguments:

    ClassGuid - Optionally, supplies a pointer to the class GUID that is to be
        associated with this set.

    hwndParent - Optionally, supplies the window handle of the top-level window
        to use for any UI related to installation of a class driver contained
        in this set's global class driver list (if it has one).

Return Value:

    If the function succeeds, the return value is a handle to an empty device
    information set.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

--*/
{
    return SetupDiCreateDeviceInfoListEx(ClassGuid, hwndParent, NULL, NULL);
}


#ifdef UNICODE
//
// ANSI version
//
HDEVINFO
WINAPI
SetupDiCreateDeviceInfoListExA(
    IN CONST GUID *ClassGuid,   OPTIONAL
    IN HWND        hwndParent,  OPTIONAL
    IN PCSTR       MachineName, OPTIONAL
    IN PVOID       Reserved
    )
{
    PCWSTR UnicodeMachineName;
    DWORD rc;
    HDEVINFO hDevInfo;

    hDevInfo = INVALID_HANDLE_VALUE;

    if(MachineName) {
        rc = CaptureAndConvertAnsiArg(MachineName, &UnicodeMachineName);
    } else {
        UnicodeMachineName = NULL;
        rc = NO_ERROR;
    }

    if(rc == NO_ERROR) {

        hDevInfo = SetupDiCreateDeviceInfoListExW(ClassGuid,
                                                  hwndParent,
                                                  UnicodeMachineName,
                                                  Reserved
                                                 );
        rc = GetLastError();
        if(UnicodeMachineName) {
            MyFree(UnicodeMachineName);
        }
    }

    SetLastError(rc);
    return hDevInfo;
}
#else
//
// Unicode version
//
HDEVINFO
WINAPI
SetupDiCreateDeviceInfoListExW(
    IN CONST GUID *ClassGuid,   OPTIONAL
    IN HWND        hwndParent,  OPTIONAL
    IN PCWSTR      MachineName, OPTIONAL
    IN PVOID       Reserved
    )
{
    UNREFERENCED_PARAMETER(ClassGuid);
    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(MachineName);
    UNREFERENCED_PARAMETER(Reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

HDEVINFO
WINAPI
SetupDiCreateDeviceInfoListEx(
    IN CONST GUID *ClassGuid,   OPTIONAL
    IN HWND        hwndParent,  OPTIONAL
    IN PCTSTR      MachineName, OPTIONAL
    IN PVOID       Reserved
    )
/*++

Routine Description:

    This API creates an empty device information set that will contain device
    device information member elements.  This set may be associated with an
    optionally-specified class GUID.

Arguments:

    ClassGuid - Optionally, supplies a pointer to the class GUID that is to be
        associated with this set.

    hwndParent - Optionally, supplies the window handle of the top-level window
        to use for any UI related to installation of a class driver contained
        in this set's global class driver list (if it has one).

    MachineName - Optionally, supplies the name of the machine for which this
        device information set is to be related.  Only devices on that machine
        may be opened/created.  If this parameter is NULL, then the local machine
        is used.

    Reserved - Reserved for future use--must be NULL.

Return Value:

    If the function succeeds, the return value is a handle to an empty device
    information set.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

--*/
{
    PDEVICE_INFO_SET DeviceInfoSet;
    DWORD Err = NO_ERROR;
    CONFIGRET cr;

    //
    // Make sure the user didn't pass us anything in the Reserved parameter.
    //
    if(Reserved) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if(DeviceInfoSet = AllocateDeviceInfoSet()) {

        try {
            //
            // If the user specified the name of a remote machine, connect to
            // that machine now.
            //
            if(MachineName) {

                if(CR_SUCCESS != (cr = CM_Connect_Machine(MachineName, &(DeviceInfoSet->hMachine)))) {
                    //
                    // Make sure hMachine is still NULL, so we won't try to disconnect later.
                    //
                    DeviceInfoSet->hMachine = NULL;
                    Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                    goto clean0;
                }

                //
                // Store the machine name in the string table, so it can be
                // retrieved later via SetupDiGetDeviceInfoListDetail.
                //
                if(-1 == (DeviceInfoSet->MachineName = pStringTableAddString(DeviceInfoSet->StringTable,
                                                                             (PTSTR)MachineName,
                                                                             STRTAB_CASE_SENSITIVE,
                                                                             NULL,
                                                                             0))) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean0;
                }
            }

            if(ClassGuid) {
                //
                // If a class GUID was specified, then store it away in
                // the device information set.
                //
                CopyMemory(&(DeviceInfoSet->ClassGuid),
                           ClassGuid,
                           sizeof(GUID)
                          );
                DeviceInfoSet->HasClassGuid = TRUE;
            }

            DeviceInfoSet->InstallParamBlock.hwndParent = hwndParent;

clean0:     ;   // nothing to do.

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Err = ERROR_INVALID_PARAMETER;
            //
            // Reference the following variable so the compiler will respect statement ordering
            // w.r.t. assignment.
            //
            DeviceInfoSet->hMachine = DeviceInfoSet->hMachine;
        }

        if(Err != NO_ERROR) {
            DestroyDeviceInfoSet(NULL, DeviceInfoSet);
        }

    } else {
        Err = ERROR_NOT_ENOUGH_MEMORY;
    }

    SetLastError(Err);

    return (Err == NO_ERROR) ? (HDEVINFO)DeviceInfoSet
                             : (HDEVINFO)INVALID_HANDLE_VALUE;
}


BOOL
WINAPI
SetupDiGetDeviceInfoListClass(
    IN  HDEVINFO DeviceInfoSet,
    OUT LPGUID   ClassGuid
    )
/*++

Routine Description:

    This API retrieves the class GUID associated with a device information
    set (if it has an associated class).

Arguments:

    DeviceInfoSet - Supplies a handle to a device information set whose associated
        class is being queried.

    ClassGuid - Supplies a pointer to a variable that receives the GUID for the
        associated class.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.  If the set has no associated class, then
    GetLastError will return ERROR_NO_ASSOCIATED_CLASS.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {

        if(pDeviceInfoSet->HasClassGuid) {
            //
            // Copy the GUID to the user-supplied buffer.
            //
            CopyMemory(ClassGuid,
                       &(pDeviceInfoSet->ClassGuid),
                       sizeof(GUID)
                      );
        } else {
            Err = ERROR_NO_ASSOCIATED_CLASS;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return (Err == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiGetDeviceInfoListDetailA(
    IN  HDEVINFO                       DeviceInfoSet,
    OUT PSP_DEVINFO_LIST_DETAIL_DATA_A DeviceInfoSetDetailData
    )
{
    DWORD rc;
    BOOL b;
    SP_DEVINFO_LIST_DETAIL_DATA_W UnicodeDevInfoSetDetails;

    UnicodeDevInfoSetDetails.cbSize = sizeof(SP_DEVINFO_LIST_DETAIL_DATA_W);

    b = SetupDiGetDeviceInfoListDetailW(DeviceInfoSet, &UnicodeDevInfoSetDetails);
    rc = GetLastError();

    if(b) {
        rc = pSetupDiDevInfoSetDetailDataUnicodeToAnsi(&UnicodeDevInfoSetDetails, DeviceInfoSetDetailData);
        if(rc != NO_ERROR) {
            b = FALSE;
        }
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupDiGetDeviceInfoListDetailW(
    IN  HDEVINFO                       DeviceInfoSet,
    OUT PSP_DEVINFO_LIST_DETAIL_DATA_W DeviceInfoSetDetailData
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoSetDetailData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiGetDeviceInfoListDetail(
    IN  HDEVINFO                       DeviceInfoSet,
    OUT PSP_DEVINFO_LIST_DETAIL_DATA   DeviceInfoSetDetailData
    )
/*++

Routine Description:

    This routine retrieves information about the specified device information set,
    such as its associated class (if any), and the remote machine it was opened for
    (if this is a remoted HDEVINFO).  This API supercedes SetupDiGetDeviceInfoListClass.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set to retrieve
        detailed information for.

    DeviceInfoSetDetailData - Supplies the address of a structure that receives
        information about the specified device information set.  This structure is
        defined as follows:

            typedef struct _SP_DEVINFO_LIST_DETAIL_DATA {
                DWORD  cbSize;
                GUID   ClassGuid;
                HANDLE RemoteMachineHandle;
                TCHAR  RemoteMachineName[SP_MAX_MACHINENAME_LENGTH];
            } SP_DEVINFO_LIST_DETAIL_DATA, *PSP_DEVINFO_LIST_DETAIL_DATA;

        where:

            ClassGuid specifies the class associated with the device information
                set, or GUID_NULL if there is no associated class.

            RemoteMachineHandle is the ConfigMgr32 machine handle used to access
                the remote machine, if this is a remoted HDEVINFO (i.e., a
                MachineName was specified when the set was created via
                SetupDiCreateDeviceInfoListEx or SetupDiGetClassDevsEx).  All
                DevInst handles stored in SP_DEVINFO_DATA structures for elements
                of this set are relative to this handle, and must be used in
                combination with this handle when calling any CM_*_Ex APIs.

                If this is not a device information set for a remote machine, this
                field will be NULL.

                NOTE:  DO NOT destroy this handle via CM_Disconnect_Machine.  This
                handle will be cleaned up when the device information set is destroyed
                via SetupDiDestroyDeviceInfoList.

            RemoteMachineName specifies the name used to connect to the remote
                machine whose handle is stored in RemoteMachineHandle.  If this is
                not a device information set for a remote machine, this will be an
                empty string.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    The cbSize field of the output structure must be set to
    sizeof(SP_DEVINFO_LIST_DETAIL_DATA) or the  call will fail with
    ERROR_INVALID_USER_BUFFER.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PCTSTR MachineName;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {

        if(DeviceInfoSetDetailData->cbSize != sizeof(SP_DEVINFO_LIST_DETAIL_DATA)) {
            Err = ERROR_INVALID_USER_BUFFER;
            goto clean0;
        }

        //
        // Store the set's associated class GUID, or GUID_NULL if there isn't one.
        //
        if(pDeviceInfoSet->HasClassGuid) {
            CopyMemory(&(DeviceInfoSetDetailData->ClassGuid),
                       &(pDeviceInfoSet->ClassGuid),
                       sizeof(GUID)
                      );
        } else {
            CopyMemory(&(DeviceInfoSetDetailData->ClassGuid), &GUID_NULL, sizeof(GUID));
        }

        DeviceInfoSetDetailData->RemoteMachineHandle = pDeviceInfoSet->hMachine;

        //
        // If this is a remoted HDEVINFO, store the machine name in the caller's buffer,
        // otherwise store an empty string.
        //
        if(pDeviceInfoSet->hMachine) {
            MYASSERT(pDeviceInfoSet->MachineName != -1);
            MachineName = pStringTableStringFromId(pDeviceInfoSet->StringTable, pDeviceInfoSet->MachineName);
            lstrcpyn(DeviceInfoSetDetailData->RemoteMachineName,
                     MachineName,
                     SIZECHARS(DeviceInfoSetDetailData->RemoteMachineName)
                    );
        } else {
            MYASSERT(pDeviceInfoSet->MachineName == -1);
            *(DeviceInfoSetDetailData->RemoteMachineName) = TEXT('\0');
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return (Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiDestroyDeviceInfoList(
    IN  HDEVINFO DeviceInfoSet
    )
/*++

Routine Description:

    This API destroys a device information set, freeing all associated memory.

Arguments:

    DeviceInfoSet - Supplies a handle to a device information set to be destroyed.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    DWORD Err;
    PDEVICE_INFO_SET pDeviceInfoSet;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    try {
        Err = DestroyDeviceInfoSet(DeviceInfoSet, pDeviceInfoSet);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_HANDLE;
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiCreateDeviceInfoA(
    IN  HDEVINFO          DeviceInfoSet,
    IN  PCSTR             DeviceName,
    IN  CONST GUID       *ClassGuid,
    IN  PCSTR             DeviceDescription, OPTIONAL
    IN  HWND              hwndParent,        OPTIONAL
    IN  DWORD             CreationFlags,
    OUT PSP_DEVINFO_DATA  DeviceInfoData     OPTIONAL
    )
{
    PCWSTR deviceName,deviceDescription;
    DWORD rc;
    BOOL b;

    b = FALSE;
    rc = CaptureAndConvertAnsiArg(DeviceName,&deviceName);
    if(rc == NO_ERROR) {

        if(DeviceDescription) {
            rc = CaptureAndConvertAnsiArg(DeviceDescription,&deviceDescription);
        } else {
            deviceDescription = NULL;
        }

        if(rc == NO_ERROR) {

            b = SetupDiCreateDeviceInfoW(
                    DeviceInfoSet,
                    deviceName,
                    ClassGuid,
                    deviceDescription,
                    hwndParent,
                    CreationFlags,
                    DeviceInfoData
                    );

            rc = GetLastError();

            if(deviceDescription) {
                MyFree(deviceDescription);
            }
        }

        MyFree(deviceName);

    } else {
        //
        // The DeviceName parameter was bad--return the same error the unicode API does.
        //
        rc = ERROR_INVALID_DEVINST_NAME;
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode version
//
BOOL
WINAPI
SetupDiCreateDeviceInfoW(
    IN  HDEVINFO          DeviceInfoSet,
    IN  PCWSTR            DeviceName,
    IN  CONST GUID       *ClassGuid,
    IN  PCWSTR            DeviceDescription, OPTIONAL
    IN  HWND              hwndParent,        OPTIONAL
    IN  DWORD             CreationFlags,
    OUT PSP_DEVINFO_DATA  DeviceInfoData     OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceName);
    UNREFERENCED_PARAMETER(ClassGuid);
    UNREFERENCED_PARAMETER(DeviceDescription);
    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(CreationFlags);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiCreateDeviceInfo(
    IN  HDEVINFO          DeviceInfoSet,
    IN  PCTSTR            DeviceName,
    IN  CONST GUID       *ClassGuid,
    IN  PCTSTR            DeviceDescription, OPTIONAL
    IN  HWND              hwndParent,        OPTIONAL
    IN  DWORD             CreationFlags,
    OUT PSP_DEVINFO_DATA  DeviceInfoData     OPTIONAL
    )
/*++

Routine Description:

    This API creates a new device information element, and adds it as a new member
    to the specified set.

Arguments:

    DeviceInfoSet - Supplies a handle to a device information set to which this
        new device information element is to be added.

    DeviceName - Supplies either a full device instance ID (e.g., Root\*PNP0500\0000)
        or a Root-enumerated device ID, minus enumerator branch prefix and instance
        ID suffix (e.g., *PNP0500).  The latter may only be specified if the
        DICD_GENERATE_ID flag is specified in the CreationFlags parameter.

    ClassGuid - Supplies a pointer to the GUID for this device's class.  If the
        class is not yet known, this value should be GUID_NULL.

    DeviceDescription - Optionally, supplies a textual description of the device.

    hwndParent - Optionally, supplies the window handle of the top-level window
        to use for any UI related to installing the device.

    CreationFlags - Supplies flags controlling how the device information element
        is to be created.  May be a combination of the following values:

        DICD_GENERATE_ID -       If this flag is specified, then DeviceName contains only
                                 a Root-enumerated device ID, and needs to have a unique
                                 device instance key created for it.  This unique device
                                 instance key will be generated as:

                                     Enum\Root\<DeviceName>\<InstanceID>

                                 where <InstanceID> is a 4-digit, base-10 number that
                                 is unique among all subkeys under Enum\Root\<DeviceName>.
                                 The API, SetupDiGetDeviceInstanceId, may be called to
                                 find out what ID was generated for this device information
                                 element.

        DICD_INHERIT_CLASSDRVS - If this flag is specified, then the resulting device
                                 information element will inherit the class driver list (if any)
                                 associated with the device information set itself.  In addition,
                                 if there is a selected driver for the device information set,
                                 that same driver will be selected for the new device information
                                 element.

    DeviceInfoData - Optionaly, supplies a pointer to the variable that receives
        a context structure initialized for this new device information element.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    If this device instance is being added to a set that has an associated class,
    then the device class must be the same, or the call will fail, and GetLastError
    will return ERROR_CLASS_MISMATCH.

    If the specified device instance is the same as an existing device instance key in
    the registry, the call will fail with ERROR_DEVINST_ALREADY_EXISTS.  (This only
    applies if DICD_GENERATE_ID is not specified.)

    The specified class GUID will be written out to the ClassGUID device instance
    value entry.  If the class name can be retrieved (via SetupDiClassNameFromGuid),
    then it will be written to the Class value entry as well.

    If the new device information element was successfully created, but the
    user-supplied DeviceInfoData buffer is invalid, this API will return FALSE, with
    GetLastError returning ERROR_INVALID_USER_BUFFER.  The device information element
    _will_ have been added as a new member of the set, however.

    Note that since new device information elements are always added at the end
    of the existing list, the enumeration ordering is preserved, thus we don't
    need to invalidate our enumeration hint.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err, StringLen;
    PDEVINFO_ELEM DevInfoElem, PrevTailDevInfoElem;
    DEVINST DevInst, RootDevInst;
    CONFIGRET cr;
    ULONG CmFlags;
    TCHAR TempString[GUID_STRING_LEN];
    PDRIVER_LIST_OBJECT CurDrvListObject;

    //
    // We use the TempString buffer both for the string representation of
    // a Class GUID, and for the Class name.  The following assert ensures
    // that our assumptions about the relative lengths of these two strings
    // continues to be valid.
    //
    MYASSERT(GUID_STRING_LEN >= MAX_CLASS_NAME_LEN);

    if(CreationFlags & ~(DICD_GENERATE_ID | DICD_INHERIT_CLASSDRVS)) {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;
    DevInst = 0;
    DevInfoElem = NULL;

    try {
        //
        // Get a pointer to the current tail of the devinfo element list for this
        // set, so that we can easily lop off the new node if we encounter an error
        // after insertion.
        //
        PrevTailDevInfoElem = pDeviceInfoSet->DeviceInfoTail;

        //
        // Get a handle to the root device instance, to be used as the parent
        // for the phantom device instance we're about to create.
        //
        if(CM_Locate_DevInst_Ex(&RootDevInst, NULL, CM_LOCATE_DEVINST_NORMAL,
                                pDeviceInfoSet->hMachine) != CR_SUCCESS) {
            //
            // We're really hosed if we can't get a handle to the root device
            // instance!
            //
            Err = ERROR_INVALID_DATA;
            goto clean0;
        }

        //
        // Create a handle to a phantom device instance.
        //
        CmFlags = CM_CREATE_DEVINST_PHANTOM;

        if(CreationFlags & DICD_GENERATE_ID) {
            CmFlags |= CM_CREATE_DEVINST_GENERATE_ID;
        }

        if((cr = CM_Create_DevInst_Ex(&DevInst,
                                   (DEVINSTID)DeviceName,
                                   RootDevInst,
                                   CmFlags,
                                   pDeviceInfoSet->hMachine)) != CR_SUCCESS) {
            //
            // Make sure DevInst handle is still invalid, so we won't try to
            // delete it later.
            //
            DevInst = 0;
            Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
            goto clean0;
        }

        if(NO_ERROR != (Err = pSetupAddNewDeviceInfoElement(pDeviceInfoSet,
                                                            DevInst,
                                                            ClassGuid,
                                                            DeviceDescription,
                                                            hwndParent,
                                                            DIE_IS_PHANTOM,
                                                            pDeviceInfoSet,
                                                            &DevInfoElem))) {
            //
            // Make sure DevInfoElem is still NULL, so we won't try to free it later.
            //
            DevInfoElem = NULL;
            goto clean0;
        }

        //
        // Now, set the Class and ClassGUID properties for the new device instance.
        //
        pSetupStringFromGuid(ClassGuid, TempString, SIZECHARS(TempString));
        CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                         CM_DRP_CLASSGUID,
                                         (PVOID)TempString,
                                         GUID_STRING_LEN * sizeof(TCHAR),
                                         0,pDeviceInfoSet->hMachine);


        if(!IsEqualGUID(ClassGuid, &GUID_NULL) &&
           SetupDiClassNameFromGuid(ClassGuid,
                                    TempString,
                                    SIZECHARS(TempString),
                                    &StringLen)) {

            CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                             CM_DRP_CLASS,
                                             (PVOID)TempString,
                                             StringLen * sizeof(TCHAR),
                                             0,pDeviceInfoSet->hMachine);
        }

        //
        // If the caller wants the newly-created devinfo element to inherit the global
        // class driver list, do that now.
        //
        if((CreationFlags & DICD_INHERIT_CLASSDRVS) && (pDeviceInfoSet->ClassDriverHead)) {
            //
            // Find the global class driver list in the devinfo set's list of driver lists.
            //
            CurDrvListObject = GetAssociatedDriverListObject(pDeviceInfoSet->ClassDrvListObjectList,
                                                             pDeviceInfoSet->ClassDriverHead,
                                                             NULL
                                                            );
            MYASSERT(CurDrvListObject && (CurDrvListObject->RefCount > 0));

            //
            // We found the driver list object, now do the inheritance, and increment the refcount.
            //
            DevInfoElem->ClassDriverCount = pDeviceInfoSet->ClassDriverCount;
            DevInfoElem->ClassDriverHead  = pDeviceInfoSet->ClassDriverHead;
            DevInfoElem->ClassDriverTail  = pDeviceInfoSet->ClassDriverTail;

            if(DevInfoElem->SelectedDriver = pDeviceInfoSet->SelectedClassDriver) {
                DevInfoElem->SelectedDriverType = SPDIT_CLASSDRIVER;
            }

            DevInfoElem->InstallParamBlock.Flags   |= CurDrvListObject->ListCreationFlags;
            DevInfoElem->InstallParamBlock.FlagsEx |= CurDrvListObject->ListCreationFlagsEx;
            DevInfoElem->InstallParamBlock.DriverPath = CurDrvListObject->ListCreationDriverPath;

            CurDrvListObject->RefCount++;
        }

clean0:
        ; // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Reference the following variables so the compiler will respect our statement ordering
        // w.r.t. assignment.
        //
        DevInst = DevInst;
        DevInfoElem = DevInfoElem;
        PrevTailDevInfoElem = PrevTailDevInfoElem;
    }

    if(Err == NO_ERROR) {

        if(DeviceInfoData) {
            //
            // The user supplied a buffer to receive a SP_DEVINFO_DATA
            // structure, so fill that in now.
            //
            try {

                if(!(DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                                      DevInfoElem,
                                                      DeviceInfoData))) {
                    Err = ERROR_INVALID_USER_BUFFER;
                }

            } except(EXCEPTION_EXECUTE_HANDLER) {
                Err = ERROR_INVALID_USER_BUFFER;
            }
        }

    } else if(DevInst) {

        //
        // BUGBUG (jamiehun 21/1/99)
        // is there anything we want to do if this fails?
        //
        cr = CM_Uninstall_DevInst(DevInst, 0);

        if(DevInfoElem) {
            //
            // An error occurred after we created the device information element--clean it up now.
            //
            try {

                MYASSERT(!DevInfoElem->Next);
                if(PrevTailDevInfoElem) {
                    MYASSERT(PrevTailDevInfoElem->Next == DevInfoElem);
                    PrevTailDevInfoElem->Next = NULL;
                    pDeviceInfoSet->DeviceInfoTail = PrevTailDevInfoElem;
                } else {
                    pDeviceInfoSet->DeviceInfoHead = pDeviceInfoSet->DeviceInfoTail = NULL;
                }

                MYASSERT(pDeviceInfoSet->DeviceInfoCount > 0);
                pDeviceInfoSet->DeviceInfoCount--;

                MyFree(DevInfoElem);

            } except(EXCEPTION_EXECUTE_HANDLER) {
                ;   // nothing to do.
            }
        }
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiOpenDeviceInfoA(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PCSTR            DeviceInstanceId,
    IN  HWND             hwndParent,        OPTIONAL
    IN  DWORD            OpenFlags,
    OUT PSP_DEVINFO_DATA DeviceInfoData     OPTIONAL
    )
{
    PCWSTR deviceInstanceId;
    DWORD rc;
    BOOL b;

    rc = CaptureAndConvertAnsiArg(DeviceInstanceId,&deviceInstanceId);
    if(rc == NO_ERROR) {

        b = SetupDiOpenDeviceInfoW(
                DeviceInfoSet,
                deviceInstanceId,
                hwndParent,
                OpenFlags,
                DeviceInfoData
                );

        rc = GetLastError();

        MyFree(deviceInstanceId);

    } else {
        b = FALSE;
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode version
//
BOOL
WINAPI
SetupDiOpenDeviceInfoW(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PCWSTR           DeviceInstanceId,
    IN  HWND             hwndParent,        OPTIONAL
    IN  DWORD            OpenFlags,
    OUT PSP_DEVINFO_DATA DeviceInfoData     OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInstanceId);
    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(OpenFlags);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiOpenDeviceInfo(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PCTSTR           DeviceInstanceId,
    IN  HWND             hwndParent,       OPTIONAL
    IN  DWORD            OpenFlags,
    OUT PSP_DEVINFO_DATA DeviceInfoData    OPTIONAL
    )
/*++

Routine Description:

    This API retrieves information about an existing device instance, and adds
    it to the specified device information set.  If a device information element
    already exists for this device instance, the existing element is returned.

Arguments:

    DeviceInfoSet - Supplies a handle to a device information set to which the
        opened device information element is to be added.

    DeviceInstanceId - Supplies the ID of the device instance.  This is the
        registry path (relative to the Enum branch) of the device instance key.
        (E.g., Root\*PNP0500\0000)

    hwndParent - Optionally, supplies the window handle of the top-level window
        to use for any UI related to installing the device.

    OpenFlags - Supplies flags controlling how the device information element
        is to be opened.  May be a combination of the following values:

        DIOD_INHERIT_CLASSDRVS - If this flag is specified, then the resulting device
                                 information element will inherit the class driver
                                 list (if any) associated with the device information
                                 set itself.  In addition, if there is a selected
                                 driver for the device information set, that same
                                 driver will be selected for the new device information
                                 element.

                                 If the device information element was already present,
                                 its class driver list (if any) will be replaced with
                                 this new, inherited, list.

        DIOD_CANCEL_REMOVE     - If this flag is set, a device that was marked for removal
                                 will be have its pending removal cancelled.

    DeviceInfoData - Optionally, supplies a pointer to the variable that receives
        a context structure initialized for the opened device information element.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    If this device instance is being added to a set that has an associated class,
    then the device class must be the same, or the call will fail, and GetLastError
    will return ERROR_CLASS_MISMATCH.

    If the new device information element was successfully opened, but the
    user-supplied DeviceInfoData buffer is invalid, this API will return FALSE,
    with GetLastError returning ERROR_INVALID_USER_BUFFER.  The device information
    element _will_ have been added as a new member of the set, however.

    Note that since new device information elements are always added at the end
    of the existing list, the enumeration ordering is preserved, thus we don't
    need to invalidate our enumeration hint.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    PDRIVER_LIST_OBJECT CurDrvListObject;
    BOOL AlreadyPresent;

    if(OpenFlags & ~(DIOD_INHERIT_CLASSDRVS | DIOD_CANCEL_REMOVE)) {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {

        Err = pSetupOpenAndAddNewDevInfoElem(pDeviceInfoSet,
                                             DeviceInstanceId,
                                             TRUE,
                                             NULL,
                                             hwndParent,
                                             &DevInfoElem,
                                             TRUE,
                                             &AlreadyPresent,
                                             FALSE,
                                             ((OpenFlags & DIOD_CANCEL_REMOVE)
                                                 ? CM_LOCATE_DEVNODE_CANCELREMOVE : 0),
                                             pDeviceInfoSet
                                            );

        if(Err != NO_ERROR) {
            goto clean0;
        }

        //
        // If the caller wants the newly-opened devinfo element to inherit the global
        // class driver list, do that now.
        //
        if(OpenFlags & DIOD_INHERIT_CLASSDRVS) {
            //
            // If this devinfo element already existed, then it may already have a class
            // driver list.  Destroy that list before inheriting from the global class
            // driver list.
            //
            if(AlreadyPresent) {
                //
                // If the selected driver is a class driver, then reset the selection.
                //
                if(DevInfoElem->SelectedDriverType == SPDIT_CLASSDRIVER) {
                    DevInfoElem->SelectedDriverType = SPDIT_NODRIVER;
                    DevInfoElem->SelectedDriver = NULL;
                }

                //
                // Destroy the existing class driver list for this device.
                //
                DereferenceClassDriverList(pDeviceInfoSet, DevInfoElem->ClassDriverHead);
                DevInfoElem->ClassDriverCount = 0;
                DevInfoElem->ClassDriverHead = DevInfoElem->ClassDriverTail = NULL;
                DevInfoElem->InstallParamBlock.Flags   &= ~(DI_DIDCLASS | DI_MULTMFGS);
                DevInfoElem->InstallParamBlock.FlagsEx &= ~DI_FLAGSEX_DIDINFOLIST;
            }

            if(pDeviceInfoSet->ClassDriverHead) {
                //
                // Find the global class driver list in the devinfo set's list of driver lists.
                //
                CurDrvListObject = GetAssociatedDriverListObject(pDeviceInfoSet->ClassDrvListObjectList,
                                                                 pDeviceInfoSet->ClassDriverHead,
                                                                 NULL
                                                                );
                MYASSERT(CurDrvListObject && (CurDrvListObject->RefCount > 0));

                //
                // We found the driver list object, now increment its refcount, and do the
                // inheritance.
                //
                CurDrvListObject->RefCount++;

                DevInfoElem->ClassDriverCount = pDeviceInfoSet->ClassDriverCount;
                DevInfoElem->ClassDriverHead  = pDeviceInfoSet->ClassDriverHead;
                DevInfoElem->ClassDriverTail  = pDeviceInfoSet->ClassDriverTail;

                if(pDeviceInfoSet->SelectedClassDriver) {
                    DevInfoElem->SelectedDriver = pDeviceInfoSet->SelectedClassDriver;
                    DevInfoElem->SelectedDriverType = SPDIT_CLASSDRIVER;
                }

                DevInfoElem->InstallParamBlock.Flags   |= CurDrvListObject->ListCreationFlags;
                DevInfoElem->InstallParamBlock.FlagsEx |= CurDrvListObject->ListCreationFlagsEx;
                DevInfoElem->InstallParamBlock.DriverPath = CurDrvListObject->ListCreationDriverPath;
            }
        }

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    if((Err == NO_ERROR) && DeviceInfoData) {
        //
        // The user supplied a buffer to receive a SP_DEVINFO_DATA
        // structure, so fill that in now.
        //
        try {

            if(!(DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                                  DevInfoElem,
                                                  DeviceInfoData))) {
                Err = ERROR_INVALID_USER_BUFFER;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Err = ERROR_INVALID_USER_BUFFER;
        }
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
HDEVINFO
WINAPI
SetupDiGetClassDevsA(
    IN CONST GUID *ClassGuid,  OPTIONAL
    IN PCSTR       Enumerator, OPTIONAL
    IN HWND        hwndParent, OPTIONAL
    IN DWORD       Flags
    )
{
    PCWSTR enumerator;
    DWORD rc;
    HDEVINFO h;

    if(Enumerator) {
        rc = CaptureAndConvertAnsiArg(Enumerator,&enumerator);
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return INVALID_HANDLE_VALUE;
        }
    } else {
        enumerator = NULL;
    }

    h = SetupDiGetClassDevsExW(ClassGuid,
                               enumerator,
                               hwndParent,
                               Flags,
                               NULL,
                               NULL,
                               NULL
                              );
    rc = GetLastError();

    if(enumerator) {
        MyFree(enumerator);
    }

    SetLastError(rc);
    return h;
}
#else
//
// Unicode version
//
HDEVINFO
WINAPI
SetupDiGetClassDevsW(
    IN CONST GUID *ClassGuid,  OPTIONAL
    IN PCWSTR      Enumerator, OPTIONAL
    IN HWND        hwndParent, OPTIONAL
    IN DWORD       Flags
    )
{
    UNREFERENCED_PARAMETER(ClassGuid);
    UNREFERENCED_PARAMETER(Enumerator);
    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(Flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(INVALID_HANDLE_VALUE);
}
#endif

HDEVINFO
WINAPI
SetupDiGetClassDevs(
    IN CONST GUID *ClassGuid,  OPTIONAL
    IN PCTSTR      Enumerator, OPTIONAL
    IN HWND        hwndParent, OPTIONAL
    IN DWORD       Flags
    )
/*++

Routine Description:

    See SetupDiGetClassDevsEx for details.

--*/
{
    return SetupDiGetClassDevsEx(ClassGuid,
                                 Enumerator,
                                 hwndParent,
                                 Flags,
                                 NULL,
                                 NULL,
                                 NULL
                                );
}


#ifdef UNICODE
//
// ANSI version
//
HDEVINFO
WINAPI
SetupDiGetClassDevsExA(
    IN CONST GUID *ClassGuid,     OPTIONAL
    IN PCSTR       Enumerator,    OPTIONAL
    IN HWND        hwndParent,    OPTIONAL
    IN DWORD       Flags,
    IN HDEVINFO    DeviceInfoSet, OPTIONAL
    IN PCSTR       MachineName,   OPTIONAL
    IN PVOID       Reserved
    )
{
    PCWSTR UnicodeEnumerator, UnicodeMachineName;
    DWORD rc;
    HDEVINFO h;

    h = INVALID_HANDLE_VALUE;

    if(Enumerator) {
        rc = CaptureAndConvertAnsiArg(Enumerator, &UnicodeEnumerator);
        if(rc != NO_ERROR) {
            goto clean0;
        }
    } else {
        UnicodeEnumerator = NULL;
    }

    if(MachineName) {
        rc = CaptureAndConvertAnsiArg(MachineName,&UnicodeMachineName);
        if(rc != NO_ERROR) {
            goto clean1;
        }
    } else {
        UnicodeMachineName = NULL;
    }

    h = SetupDiGetClassDevsExW(ClassGuid,
                               UnicodeEnumerator,
                               hwndParent,
                               Flags,
                               DeviceInfoSet,
                               UnicodeMachineName,
                               Reserved
                              );
    rc = GetLastError();

    if(UnicodeMachineName) {
        MyFree(UnicodeMachineName);
    }

clean1:
    if(UnicodeEnumerator) {
        MyFree(UnicodeEnumerator);
    }

clean0:
    SetLastError(rc);
    return h;
}
#else
//
// Unicode version
//
HDEVINFO
WINAPI
SetupDiGetClassDevsExW(
    IN CONST GUID *ClassGuid,     OPTIONAL
    IN PCWSTR      Enumerator,    OPTIONAL
    IN HWND        hwndParent,    OPTIONAL
    IN DWORD       Flags,
    IN HDEVINFO    DeviceInfoSet, OPTIONAL
    IN PCWSTR      MachineName,   OPTIONAL
    IN PVOID       Reserved
    )
{
    UNREFERENCED_PARAMETER(ClassGuid);
    UNREFERENCED_PARAMETER(Enumerator);
    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(MachineName);
    UNREFERENCED_PARAMETER(Reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(INVALID_HANDLE_VALUE);
}
#endif

HDEVINFO
WINAPI
SetupDiGetClassDevsEx(
    IN CONST GUID *ClassGuid,     OPTIONAL
    IN PCTSTR      Enumerator,    OPTIONAL
    IN HWND        hwndParent,    OPTIONAL
    IN DWORD       Flags,
    IN HDEVINFO    DeviceInfoSet, OPTIONAL
    IN PCTSTR      MachineName,   OPTIONAL
    IN PVOID       Reserved
    )
/*++

Routine Description:

    This routine returns a device information set containing all installed
    devices of the specified class.

Arguments:

    ClassGuid - Optionally, supplies the address of the class GUID to use
        when creating the list of devices.  If the DIGCF_ALLCLASSES flag is
        set, then this parameter is ignored, and the resulting list will
        contain all classes of devices (i.e., every installed device).

        If the DIGCF_DEVICEINTERFACE flag _is not_ set, then this class GUID
        represents a setup class.

        If the DIGCF_DEVICEINTERFACE flag _is_ set, then this class GUID
        represents an interface class.

    Enumerator - Optional parameter that filters the members of the returned
        device information set based on their enumerator (i.e., provider).

        If the DIGCF_DEVICEINTERFACE flag _is not_ set in the Flags parameter,
        then this string represents the name of the key under the Enum branch
        containing devices instances for which information is to be retrieved.
        If this parameter is not specified, then device information will be
        retrieved for all device instances in the entire Enum tree.

        If the DIGCF_DEVICEINTERFACE flag _is_ set, then this string represents
        the PnP name of a particular device for which interfaces are to be
        retrieved.  In this case, the resulting device information set will
        consist of a single device information element--the device whose name
        was specified as the enumerator.  The interface devices provided by this
        PnP device can then be enumerated via SetupDiEnumInterfaceDevice.

    hwndParent - Optionally, supplies the handle of the top-level window to be
        used for any UI relating to the members of this set.

    Flags - Supplies control options used in building the device information set.
        May be a combination of the following values:

        DIGCF_PRESENT         - Return only devices that are currently present.
        DIGCF_ALLCLASSES      - Return a list of installed devices for all classes.
                                If set, this flag will cause ClassGuid to be ignored.
        DIGCF_PROFILE         - Return only devices that are a part of the current
                                hardware profile.
        DIGCF_DEVICEINTERFACE - Return a list of all devices that expose interfaces
                                of the class specified by ClassGUID (NOTE: in this
                                context, ClassGuid is an interface class, _not_ a
                                setup class).  The interface devices exposed by the
                                members of the resulting set may be enumerated via
                                SetupDiEnumInterfaceDevice.
        DIGCF_DEFAULT         - When used with DIGCF_DEVICEINTERFACE, this flag
                                results in a list that contains only one device
                                information element.  Enumerating that device will
                                return exactly one interface device--the one that has
                                been marked as the system default interface device for
                                that particular interface class.  If there is no default
                                interface device for the specified class, the API will
                                fail, and GetLastError will return
                                ERROR_NO_DEFAULT_DEVICE_INTERFACE.

                                BUGBUG (lonnym): NOT IMPLEMENTED YET!

    DeviceInfoSet - Optionally, supplies the handle of an existing device
        information set into which these new device information elements (and,
        if DIGCF_DEVICEINTERFACE is specified, device interfaces) will be added.
        If this parameter is specified, then this same HDEVINFO will be returned
        upon success, with the retrieved device information/device interface
        elements added.  If this parameter is not specified, then a new device
        information set will be created, and its handle returned.

        NOTE: if this parameter is specified, then the associated class of this
        device information set (if any) must match the ClassGuid specified, if
        that class GUID is a setup class (i.e., the DIGCF_DEVICEINTERFACE flag
        isn't set).  If the DIGCF_DEVICEINTERFACE flag is set, then the device
        interfaces retrieved will be filtered based on whether or not their
        corresponding device's setup class matches that of the device
        information set.  This trick can be used, for example, to retrieve a
        list of device interfaces of a particular interface class, but only if
        those interfaces are exposed by devices of a particular setup class.
        E.g.,

            1.  Create a device information set (via SetupDiCreateDeviceInfoList)
                whose associated setup class is "Volume".
            2.  Call SetupDiGetClassDevsEx to retrieve a list of all device
                interfaces of interface class "mounted device", passing in the
                HDEVINFO retrieved in step 1.

        The result of the above steps would be a device information set
        containing all device interfaces of (interface) class "mounted device"
        that are exposed by devnodes of (setup) class "Volume".

        Note that retrieval of new device information elements into an existing
        HDEVINFO set doesn't invalidate our devinfo enumeration hint, since new
        devinfo elements are always added onto the end of the list.

    MachineName - Optionally, supplies the name of a remote machine for which a
        device information set is to be retrieved.  If this parameter is NULL,
        then the local machine is used.

    Reserved - Reserved for future use--must be NULL.

Return Value:

    If the function succeeds, the return value is a handle to a device
    information set containing all installed devices matching the specified
    parameters.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

--*/
{
    HDEVINFO hDevInfo;
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    DWORD Err;
    CONFIGRET cr;
    PTCHAR DevIdBuffer;
    ULONG DevIdBufferLen, CSConfigFlags;
    PTSTR CurDevId, DeviceInstanceToOpen;
    HKEY hKeyDevClassRoot, hKeyCurDevClass;
    TCHAR InterfaceGuidString[GUID_STRING_LEN];
    BOOL GetInterfaceList, GetNextInterfaceClass;
    DWORD InterfaceClassKeyIndex;
    FILETIME LastWriteTime;
    GUID GuidBuffer;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    DWORD RegDataType, DataBufferSize;
    BOOL DevInfoAlreadyPresent, IsActive;
    SP_DEVINFO_DATA DeviceInfoData;
    CONST GUID * ExistingClassGuid;

    //
    // Make sure the user didn't pass us anything in the Reserved parameter.
    //
    if(Reserved) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    //
    // Unless the caller wants a list of all classes, they'd better supply a class GUID.
    //
    if(!(Flags & DIGCF_ALLCLASSES) && !ClassGuid) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    //
    // DIGCF_DEFAULT can only be used in conjunction with DIGCF_DEVICEINTERFACE.
    //
    if((Flags & (DIGCF_DEFAULT | DIGCF_DEVICEINTERFACE)) == DIGCF_DEFAULT) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if(!DeviceInfoSet || (DeviceInfoSet == INVALID_HANDLE_VALUE)) {
        //
        // The caller didn't supply us with a device information set in which
        // to add our newly-retrieved elements, so we need to create our own.
        //
        if((hDevInfo = SetupDiCreateDeviceInfoListEx((Flags & (DIGCF_ALLCLASSES | DIGCF_DEVICEINTERFACE))
                                                          ? NULL
                                                          : ClassGuid,
                                                     hwndParent,
                                                     MachineName,
                                                     NULL)) == INVALID_HANDLE_VALUE) {
            //
            // Last error already set.
            //
            return INVALID_HANDLE_VALUE;
        }

        pDeviceInfoSet = AccessDeviceInfoSet(hDevInfo);

    } else {
        //
        // The caller wants us to use an existing device information set.  Make
        // a copy of it, and work with that one, so that if something fails, we
        // haven't screwed up the original one.
        //
        // NOTE:  DO NOT do anything with the DeviceInfoSet after this point,
        // as doing so will get the original out-of-sync with the current copy
        // we're working with.
        //
        hDevInfo = NULL;
        pDeviceInfoSet = CloneDeviceInfoSet(DeviceInfoSet);
        if(!pDeviceInfoSet) {
            //
            // Last error already set.
            //
            return INVALID_HANDLE_VALUE;
        }
    }

    Err = NO_ERROR;
    DevIdBuffer = NULL;
    hKeyDevClassRoot = hKeyCurDevClass = INVALID_HANDLE_VALUE;

    try {
        //
        // If the caller supplied us with a previously-existing devinfo set in
        // which to add new elements, we need to make sure that the setup class
        // GUID associated with this devinfo set (if any) matches the setup
        // class GUID that the caller supplied.
        //
        if(hDevInfo) {
            //
            // We always want the ExistingClassGuid pointer to be NULL when we
            // haven't been passed in a previously-existing device information
            // set.
            //
            ExistingClassGuid = NULL;

        } else {

            if(pDeviceInfoSet->HasClassGuid) {
                //
                // Remember the devinfo set's associated setup class GUID, to
                // be used later in filtering device interfaces based on the
                // setup class of their underlying devnode.
                //
                ExistingClassGuid = &(pDeviceInfoSet->ClassGuid);

                if(ClassGuid && !(Flags & (DIGCF_ALLCLASSES | DIGCF_DEVICEINTERFACE))) {

                    if(!IsEqualGUID(ExistingClassGuid, ClassGuid)) {
                        Err = ERROR_CLASS_MISMATCH;
                        goto clean0;
                    }
                }

            } else {
                //
                // The caller-supplied devinfo set had no associated setup
                // class.  Remember that fact, so that we won't try to filter
                // device interfaces based on the underlying devices' setup
                // class.
                //
                ExistingClassGuid = NULL;
            }
        }

        if(GetInterfaceList = (Flags & DIGCF_DEVICEINTERFACE)) {  // yes, we want an assignment here.
            //
            // Open the root of the DeviceClasses registry branch
            //
            hKeyDevClassRoot = SetupDiOpenClassRegKeyEx(NULL,
                                                        KEY_READ,
                                                        DIOCR_INTERFACE,
                                                        NULL,
                                                        NULL
                                                       );

            if(hKeyDevClassRoot == INVALID_HANDLE_VALUE) {
                Err = GetLastError();
                goto clean0;
            }

            if(Flags & DIGCF_ALLCLASSES) {
                InterfaceClassKeyIndex = 0;
                ClassGuid = &GuidBuffer;
            }

            if(Flags & DIGCF_PRESENT) {
                //
                // Since we're only going to be retrieving a list of device
                // interfaces that are currently 'active', we can set the
                // 'IsActive' flag to always be TRUE.
                //
                IsActive = TRUE;
            }
        }

        //
        // As an optimization, start out with a 16K (character) buffer, in the hopes of avoiding
        // two scans through the hardware tree (once to get the size, and again to get the data).
        //
        DevIdBufferLen = 16384;

        do {

            if(GetInterfaceList) {

                if(Flags & DIGCF_ALLCLASSES) {
                    //
                    // We have to enumerate through all interface device classes, and retrieve
                    // a list of device interfaces for each one.
                    //
                    DataBufferSize = SIZECHARS(InterfaceGuidString);

                    switch(RegEnumKeyEx(hKeyDevClassRoot,
                                        InterfaceClassKeyIndex,
                                        InterfaceGuidString,
                                        &DataBufferSize,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &LastWriteTime)) {

                        case ERROR_SUCCESS :
                            GetNextInterfaceClass = TRUE;
                            InterfaceClassKeyIndex++;
                            break;

                        case ERROR_NO_MORE_ITEMS :
                            //
                            // We've processed all of the interface class GUIDs--we're done.
                            //
                            GetNextInterfaceClass = FALSE;
                            continue;

                        default :
                            //
                            // Some other error occurred.  Skip this subkey, and continue
                            // with the next one.
                            //
                            GetNextInterfaceClass = TRUE;
                            InterfaceClassKeyIndex++;
                            continue;
                    }

                    //
                    // Convert the GUID string retrieved above into its binary form, for use
                    // below.
                    //
                    if(pSetupGuidFromString(InterfaceGuidString, &GuidBuffer) != NO_ERROR) {
                        //
                        // The subkey we enumerated is not a valid GUID string--skip this
                        // subkey, and continue on with the next one.
                        //
                        continue;
                    }

                } else {
                    //
                    // We're just retrieving devices for a single interface class (which the
                    // caller specified).  All we need to do is initialize the GUID string
                    // buffer with the textual form of the GUID.
                    //
                    pSetupStringFromGuid(ClassGuid,
                                         InterfaceGuidString,
                                         SIZECHARS(InterfaceGuidString)
                                        );
                    //
                    // We only need to go through this list once.
                    //
                    GetNextInterfaceClass = FALSE;
                }

                //
                // We'll be using the same character buffer to store each device instance ID
                // we're opening below.
                //
                DeviceInstanceToOpen = DeviceInstanceId;

            } else {
                //
                // We're not retrieving a list of interface devices, so we'll never go through
                // this loop more than once.
                //
                GetNextInterfaceClass = FALSE;
            }

            //
            // Retrieve a list of device names.
            //
            while(TRUE) {

                if(!DevIdBuffer) {

                    if(!(DevIdBuffer = MyMalloc(DevIdBufferLen * sizeof(TCHAR)))) {
                        Err = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean0;
                    }
                }

                if(GetInterfaceList) {
                    cr = CM_Get_Device_Interface_List_Ex((LPGUID)ClassGuid,
                                                         (DEVINSTID)Enumerator,
                                                         DevIdBuffer,
                                                         DevIdBufferLen,
                                                         (Flags & DIGCF_PRESENT)
                                                             ? CM_GET_DEVICE_INTERFACE_LIST_PRESENT
                                                             : CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES,
                                                         pDeviceInfoSet->hMachine
                                                        );
                } else {
                    cr = CM_Get_Device_ID_List_Ex(Enumerator,
                                                  DevIdBuffer,
                                                  DevIdBufferLen,
                                                  Enumerator ? CM_GETIDLIST_FILTER_ENUMERATOR
                                                             : CM_GETIDLIST_FILTER_NONE,
                                                  pDeviceInfoSet->hMachine
                                                 );
                }

                if(cr == CR_SUCCESS) {
                    //
                    // Device list successfully retrieved!
                    //
                    break;

                } else {
                    //
                    // Free the current buffer before determining what error occurred.
                    //
                    MyFree(DevIdBuffer);
                    DevIdBuffer = NULL;

                    if(cr == CR_BUFFER_SMALL) {
                        //
                        // OK, so our buffer wasn't big enough--just how big
                        // does it need to be?
                        //
                        if(GetInterfaceList) {

                            if(CM_Get_Device_Interface_List_Size_Ex(&DevIdBufferLen,
                                                                    (LPGUID)ClassGuid,
                                                                    (DEVINSTID)Enumerator,
                                                                    (Flags & DIGCF_PRESENT)
                                                                        ? CM_GET_DEVICE_INTERFACE_LIST_PRESENT
                                                                        : CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES,
                                                                    pDeviceInfoSet->hMachine) != CR_SUCCESS) {
                                //
                                // Couldn't retrieve the list size--this should
                                // never happen.
                                //
                                Err = ERROR_INVALID_DATA;
                                goto clean0;
                            }

                        } else {

                            if(CM_Get_Device_ID_List_Size_Ex(&DevIdBufferLen,
                                                          Enumerator,
                                                          Enumerator ? CM_GETIDLIST_FILTER_ENUMERATOR
                                                                     : CM_GETIDLIST_FILTER_NONE,
                                                          pDeviceInfoSet->hMachine) != CR_SUCCESS) {
                                //
                                // Couldn't retrieve the list size--this should
                                // never happen.
                                //
                                Err = ERROR_INVALID_DATA;
                                goto clean0;
                            }
                        }

                    } else {
                        //
                        // An error occurred, and it wasn't because we supplied
                        // too small a buffer.
                        //
                        Err = ERROR_INVALID_DATA;
                        goto clean0;
                    }
                }
            }

            //
            // We have now retrieved a list of all the specified devices.  If
            // these are device interfaces, we need to open the key for this
            // interface class underneath the DeviceClasses key.
            //
            if(GetInterfaceList) {

                if(RegOpenKeyEx(hKeyDevClassRoot,
                                InterfaceGuidString,
                                0,
                                KEY_READ,
                                &hKeyCurDevClass) != ERROR_SUCCESS) {
                    //
                    // Make sure hKeyCurDevClass is still set to
                    // INVALID_HANDLE_VALUE, so that we'll know not to close it.
                    //
                    hKeyCurDevClass = INVALID_HANDLE_VALUE;

                    //
                    // Skip this interface class.
                    //
                    continue;
                }
            }

            //
            // Now create device information elements from the members of this
            // list.
            //
            for(CurDevId = DevIdBuffer;
                *CurDevId;
                CurDevId += lstrlen(CurDevId) + 1) {

                //
                // If this is a device interface, we must retrieve the
                // associated device instance name.
                //
                if(GetInterfaceList) {

                    if(NO_ERROR != pSetupGetDevInstNameAndStatusForInterfaceDevice(
                                       hKeyCurDevClass,
                                       CurDevId,
                                       DeviceInstanceId,
                                       SIZECHARS(DeviceInstanceId),
                                       (Flags & DIGCF_PRESENT) ? NULL : &IsActive)) {
                        //
                        // Couldn't retrieve the name of the owning device
                        // instance--skip this device interface.
                        //
                        continue;
                    }

                } else {
                    DeviceInstanceToOpen = CurDevId;
                }

                if(Flags & DIGCF_PROFILE) {
                    //
                    // Verify that this device instance is part of the current
                    // hardware profile.
                    //
                    if(CM_Get_HW_Prof_Flags_Ex(DeviceInstanceToOpen,
                                               0,
                                               &CSConfigFlags,
                                               0,
                                               pDeviceInfoSet->hMachine) == CR_SUCCESS) {

                        if(CSConfigFlags & CSCONFIGFLAG_DO_NOT_CREATE) {
                            continue;
                        }
                    }
                }

                //
                // Note the last parameter in the following call to
                // pSetupOpenAndAddNewDevInfoElem--in the case where we're
                // adding to an existing caller-supplied HDEVINFO set (i.e.,
                // hDevInfo is NULL), we cast the HDEVINFO to a DEVICE_INFO_SET
                // pointer, since that's what ends up getting stored in the
                // ContainingDeviceInfoSet field of a devinfo element structure.
                // That field is used for quick validation that a caller-
                // supplied device information element is valid.
                //
                // If we ever decide to change the internal implementation of
                // how an HDEVINFO translates into its underlying
                // DEVICE_INFO_SET, then we'll need to update the code below
                // accordingly.  (See also the comments under
                // AccessDeviceInfoSet, CloneDeviceInfoSet, and
                // RollbackDeviceInfoSet.)
                //
                Err = pSetupOpenAndAddNewDevInfoElem(pDeviceInfoSet,
                                                     DeviceInstanceToOpen,
                                                     !(Flags & DIGCF_PRESENT),
                                                     ((Flags & (DIGCF_ALLCLASSES | DIGCF_DEVICEINTERFACE))
                                                        ? ExistingClassGuid
                                                        : ClassGuid),
                                                     hwndParent,
                                                     &DevInfoElem,
                                                     (GetInterfaceList || !hDevInfo),
                                                     &DevInfoAlreadyPresent,
                                                     FALSE,
                                                     0,
                                                     (hDevInfo ? pDeviceInfoSet : (PDEVICE_INFO_SET)DeviceInfoSet)
                                                    );

                if(Err != NO_ERROR) {

                    if(Err == ERROR_NOT_ENOUGH_MEMORY) {
                        goto clean0;
                    }

                    Err = NO_ERROR;
                    continue;
                }

                if(GetInterfaceList) {
                    //
                    // Now that we've successfully opened up the device instance that 'owns'
                    // this device interface, add a new interface device node onto this
                    // devinfo element's list.
                    //
                    if(NO_ERROR != (Err = pSetupAddInterfaceDeviceToDevInfoElem(pDeviceInfoSet,
                                                                                DevInfoElem,
                                                                                ClassGuid,
                                                                                CurDevId,
                                                                                IsActive,
                                                                                !hDevInfo,
                                                                                FALSE,
                                                                                NULL))) {
                        //
                        // The only error we should be getting back from this routine is
                        // out-of-memory, which is always a fatal error.
                        //
                        goto clean0;
                    }
                }
            }

            //
            // If we're working with interface devices, we need to close the interface
            // class key we opened above.
            //
            if(GetInterfaceList) {
                RegCloseKey(hKeyCurDevClass);
                hKeyCurDevClass = INVALID_HANDLE_VALUE;
            }

        } while(GetNextInterfaceClass);

clean0:
        ; // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;

        if(hKeyCurDevClass != INVALID_HANDLE_VALUE) {
            RegCloseKey(hKeyCurDevClass);
        }

        //
        // Access the following variables, so the compiler will respect
        // the statement ordering in the try clause.
        //
        DevIdBuffer = DevIdBuffer;
        hKeyDevClassRoot = hKeyDevClassRoot;
    }

    if(DevIdBuffer) {
        MyFree(DevIdBuffer);
    }

    if(hKeyDevClassRoot != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKeyDevClassRoot);
    }

    if(Err != NO_ERROR) {
        if(hDevInfo) {
            DestroyDeviceInfoSet(hDevInfo, pDeviceInfoSet);
        } else {
            pDeviceInfoSet = RollbackDeviceInfoSet(DeviceInfoSet, pDeviceInfoSet);
            MYASSERT(pDeviceInfoSet);
            UnlockDeviceInfoSet(pDeviceInfoSet);
        }
        SetLastError(Err);
        hDevInfo = INVALID_HANDLE_VALUE;
    } else {
        if(!hDevInfo) {
            //
            // We retrieved additional elements into an existing device
            // information set.  Replace the existing device information set
            // with the new one (i.e., into the same handle), and return the
            // same HDEVINFO handle that the caller passed in as the
            // DeviceInfoSet parameter.
            //
            pDeviceInfoSet = CommitDeviceInfoSet(DeviceInfoSet, pDeviceInfoSet);
            MYASSERT(pDeviceInfoSet);

            //
            // Set hDevInfo to be the same as the DeviceInfoSet handle we were
            // passed in, so that we can return it to the caller.
            //
            hDevInfo = DeviceInfoSet;
        }
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    return hDevInfo;
}


DWORD
pSetupAddNewDeviceInfoElement(
    IN  PDEVICE_INFO_SET pDeviceInfoSet,
    IN  DEVINST          DevInst,
    IN  CONST GUID      *ClassGuid,
    IN  PCTSTR           Description,             OPTIONAL
    IN  HWND             hwndParent,              OPTIONAL
    IN  DWORD            DiElemFlags,
    IN  PDEVICE_INFO_SET ContainingDeviceInfoSet,
    OUT PDEVINFO_ELEM   *DeviceInfoElement
    )
/*++

Routine Description:

    This routine creates a new device information element based on the
    supplied information, and adds it to the specified device information set.
    ASSUMES THAT THE CALLING ROUTINE HAS ALREADY ACQUIRED THE LOCK!

Arguments:

    pDeviceInfoSet - Device information set to add this new element to.

    DevInst - Supplies the device instance handle of the element to be added.

    ClassGuid - Class GUID of the element to be added.

    Description - Optionally, supplies the description of the element to
        be added.

    hwndParent - Optionally, supplies the handle to the top level window for
        UI relating to this element.

    DiElemFlags - Specifies flags pertaining to the device information element
        being created.

    ContainingDeviceInfoSet - Supplies a pointer to the device information set
        structure with which this element is to be associated.  This may be
        different from the pDeviceInfoSet parameter if we're working against a
        cloned devinfo set (i.e., to facilitate rollback).

    DeviceInfoElement - Supplies the address of the variable that receives a
        pointer to the newly-allocated device information element.

Return Value:

    If the function succeeds, the return value is NO_ERROR, otherwise the
    ERROR_* code is returned.

Remarks:

    Since the new element is added onto the end of the existing list, our
    enumeration hint isn't invalidated.

--*/
{
    DWORD Err = NO_ERROR;
    TCHAR TempString[LINE_LEN];

    *DeviceInfoElement = NULL;


    try {
        //
        // If there is a class associated with this device information set,
        // verify that it is the same as that of the new element.
        //
        if(pDeviceInfoSet->HasClassGuid &&
           !IsEqualGUID(&(pDeviceInfoSet->ClassGuid), ClassGuid)) {

            Err = ERROR_CLASS_MISMATCH;
            goto clean0;

        }

        //
        // Allocate storage for the element.
        //
        if(!(*DeviceInfoElement = MyMalloc(sizeof(DEVINFO_ELEM)))) {

            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        ZeroMemory(*DeviceInfoElement, sizeof(DEVINFO_ELEM));

        //
        // Store the address of the containing devinfo set in the structure
        // for this element.  This is used for efficient validation of a
        // caller-supplied SP_DEVINFO_DATA.
        //
        (*DeviceInfoElement)->ContainingDeviceInfoSet = ContainingDeviceInfoSet;

        //
        // Initialize the element with the specified information
        //
        CopyMemory(&((*DeviceInfoElement)->ClassGuid),
                   ClassGuid,
                   sizeof(GUID)
                  );
        (*DeviceInfoElement)->InstallParamBlock.hwndParent = hwndParent;

        if(Description) {
            //
            // Set the device instance's DeviceDesc property to the specified
            // description.
            //
            CM_Set_DevInst_Registry_Property_Ex(DevInst,
                                             CM_DRP_DEVICEDESC,
                                             Description,
                                             (lstrlen(Description) + 1) * sizeof(TCHAR),
                                             0,
                                             pDeviceInfoSet->hMachine);

            //
            // Store two versions of the description--one case-sensitive (for display)
            // and the other case-insensitive (for fast lookup).
            //
            lstrcpyn(TempString, Description, SIZECHARS(TempString));

            if((((*DeviceInfoElement)->DeviceDescriptionDisplayName =
                      pStringTableAddString(pDeviceInfoSet->StringTable,
                                            TempString,
                                            STRTAB_CASE_SENSITIVE,
                                            NULL,0)) == -1) ||
               (((*DeviceInfoElement)->DeviceDescription =
                      pStringTableAddString(pDeviceInfoSet->StringTable,
                                            TempString,
                                            STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                            NULL,0)) == -1)) {

                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }

        } else {
            (*DeviceInfoElement)->DeviceDescription =
                (*DeviceInfoElement)->DeviceDescriptionDisplayName = -1;
        }

        (*DeviceInfoElement)->DevInst = DevInst;
        (*DeviceInfoElement)->DiElemFlags = DiElemFlags;
        (*DeviceInfoElement)->InstallParamBlock.DriverPath = -1;
        (*DeviceInfoElement)->InstallParamBlock.CoInstallerCount = -1;

        //
        // If we're in GUI-mode setup on Windows NT, we'll automatically set 
        // the DI_FLAGSEX_IN_SYSTEM_SETUP flag in the devinstall parameter 
        // block for this devinfo element.
        //
        if(GuiSetupInProgress) {
            (*DeviceInfoElement)->InstallParamBlock.FlagsEx |= DI_FLAGSEX_IN_SYSTEM_SETUP;
        }

        //
        // If we're in non-interactive mode, set the "be quiet" bits.
        //
        if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
            (*DeviceInfoElement)->InstallParamBlock.Flags   |= DI_QUIETINSTALL;
            (*DeviceInfoElement)->InstallParamBlock.FlagsEx |= DI_FLAGSEX_NOUIONQUERYREMOVE;
        }

        //
        // Initialize our enumeration 'hints'
        //
        (*DeviceInfoElement)->ClassDriverEnumHintIndex = INVALID_ENUM_INDEX;
        (*DeviceInfoElement)->CompatDriverEnumHintIndex = INVALID_ENUM_INDEX;

        //
        // Create a log context separate from the parent.
        //
        if(CreateLogContext(NULL, &(*DeviceInfoElement)->InstallParamBlock.LogContext) != NO_ERROR) {
            //
            // if it failed, we will inheret the log context, since it's better than nothing
            // in theory, this should never happen, or if it does, other things will fail too
            //
            (*DeviceInfoElement)->InstallParamBlock.LogContext = NULL;

            Err = InheritLogContext(pDeviceInfoSet->InstallParamBlock.LogContext, &(*DeviceInfoElement)->InstallParamBlock.LogContext);
            if (Err != NO_ERROR) {
                goto clean0;
            }
        }

        //
        // Now, insert the new element at the end of the device
        // information set's list of elements.
        //
        if(pDeviceInfoSet->DeviceInfoHead) {
            pDeviceInfoSet->DeviceInfoTail->Next = *DeviceInfoElement;
            pDeviceInfoSet->DeviceInfoTail = *DeviceInfoElement;
        } else {
            pDeviceInfoSet->DeviceInfoHead =
            pDeviceInfoSet->DeviceInfoTail = *DeviceInfoElement;
        }
        pDeviceInfoSet->DeviceInfoCount++;

clean0:
        ; // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    if((Err != NO_ERROR) && *DeviceInfoElement) {

        MyFree(*DeviceInfoElement);
        *DeviceInfoElement = NULL;
    }

    return Err;
}


DWORD
pSetupClassGuidFromDevInst(
    IN  DEVINST DevInst,
    IN  HMACHINE hMachine,
    OUT LPGUID  ClassGuid
    )
/*++

Routine Description:

    This routine attempts to retrieve the class GUID for the specified device
    instance from its device registry key.  If it cannot retrieve one, it
    returns GUID_NULL.

Arguments:

    DevInst - Supplies the handle of the device instance whose class GUID is
        to be retrieved.

        hMachine - Machine context to operate in

    ClassGuid - Supplies the address of the variable that receives the class
        GUID, or GUID_NULL if no class GUID can be retrieved.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.  (Presently, the only
    failure condition returned is ERROR_NOT_ENOUGH_MEMORY.)

--*/
{
    DWORD NumGuids;
    TCHAR TempString[GUID_STRING_LEN];
    DWORD StringSize;

    StringSize = sizeof(TempString);
    if(CM_Get_DevInst_Registry_Property_Ex(DevInst,
                                        CM_DRP_CLASSGUID,
                                        NULL,
                                        TempString,
                                        &StringSize,
                                        0,
                                        hMachine) == CR_SUCCESS) {
        //
        // We retrieved the class GUID (in string form) for this device
        // instance--now, convert it into its binary representation.
        //
        return pSetupGuidFromString(TempString, ClassGuid);
    }

    //
    // We couldn't retrieve a ClassGUID--let's see if there's a Class name we can
    // work with.
    //
    StringSize = sizeof(TempString);
    if(CM_Get_DevInst_Registry_Property_Ex(DevInst,
                                        CM_DRP_CLASS,
                                        NULL,
                                        TempString,
                                        &StringSize,
                                        0,
                                        hMachine) == CR_SUCCESS) {
        //
        // OK, we found out the class name.  Now see if we can find a
        // single class GUID to match it.
        //
        if(SetupDiClassGuidsFromName(TempString, ClassGuid, 1, &NumGuids) && NumGuids) {
            //
            // We found exactly one, so we're happy.
            //
            return NO_ERROR;
        }
    }

    //
    // We have no idea what class of device this is, so use GUID_NULL.
    //
    CopyMemory(ClassGuid, &GUID_NULL, sizeof(GUID));

    return NO_ERROR;
}


BOOL
WINAPI
SetupDiDeleteDeviceInfo(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine deletes a member from the specified device information set.
    THIS DOES NOT DELETE ACTUAL DEVICES!

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device information element to be deleted.

    DeviceInfoData - Supplies a pointer to the SP_DEVINFO_DATA structure for
        the device information element to be deleted.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    If the specified device information element is explicitly in use by a wizard
    page, then the call will fail, and GetLastError will return
    ERROR_DEVINFO_DATA_LOCKED.  This will happen if a handle to a wizard page was
    retrieved via SetupDiGetWizardPage, and this element was specified, along with
    the DIWP_FLAG_USE_DEVINFO_DATA flag.  In order to be able to delete this element,
    the wizard HPROPSHEETPAGE handle must be closed (either explicitly, or after a
    call to PropertySheet() completes).

    Since we don't track where this devinfo element lives in relation to our
    current enumeration hint, we just invalidate the hint, so that next
    enumeration must scan from the beginning of the list.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM ElemToDelete, PrevElem, NextElem;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {
        //
        // Get a pointer to the element we are to delete.
        //
        ElemToDelete = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                 DeviceInfoData,
                                                 &PrevElem
                                                );
        if(ElemToDelete) {
            //
            // Make sure that this element isn't currently locked by
            // a wizard page.
            //
            if(ElemToDelete->DiElemFlags & DIE_IS_LOCKED) {
                Err = ERROR_DEVINFO_DATA_LOCKED;
                goto clean0;
            }

            NextElem = ElemToDelete->Next;

            //
            // Destroy the devinfo element.  We need to do this before
            // altering the list, because we will be calling the class
            // installer with DIF_DESTROYPRIVATEDATA, and it needs to
            // be able to access this element (obviously).
            //
            DestroyDeviceInfoElement(DeviceInfoSet, pDeviceInfoSet, ElemToDelete);

            //
            // Now remove the element from the list.
            //
            if(PrevElem) {
                PrevElem->Next = NextElem;
            } else {
                pDeviceInfoSet->DeviceInfoHead = NextElem;
            }

            if(!NextElem) {
                pDeviceInfoSet->DeviceInfoTail = PrevElem;
            }

            MYASSERT(pDeviceInfoSet->DeviceInfoCount > 0);
            pDeviceInfoSet->DeviceInfoCount--;

            //
            // If this element was the currently selected device for this
            // set, then reset the device selection.
            //
            if(pDeviceInfoSet->SelectedDevInfoElem == ElemToDelete) {
                pDeviceInfoSet->SelectedDevInfoElem = NULL;
            }

            //
            // Invalidate our enumeration hint for this devinfo element list.
            //
            pDeviceInfoSet->DeviceInfoEnumHint = NULL;
            pDeviceInfoSet->DeviceInfoEnumHintIndex = INVALID_ENUM_INDEX;

        } else {
            Err = ERROR_INVALID_PARAMETER;
        }

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiEnumDeviceInfo(
    IN  HDEVINFO         DeviceInfoSet,
    IN  DWORD            MemberIndex,
    OUT PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This API enumerates the members of the specified device information set.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set whose members
        are to be enumerated.

    MemberIndex - Supplies the zero-based index of the device information member
        to be retreived.

    DeviceInfoData - Supplies a pointer to a SP_DEVINFO_DATA structure that will
        receive information about this member.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    To enumerate device information members, an application should initially call
    the SetupDiEnumDeviceInfo function with the MemberIndex parameter set to zero.
    The application should then increment MemberIndex and call the
    SetupDiEnumDeviceInfo function until there are no more values (i.e., the
    function fails, and GetLastError returns ERROR_NO_MORE_ITEMS).

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err, i;
    PDEVINFO_ELEM DevInfoElem;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {

        if(MemberIndex >= pDeviceInfoSet->DeviceInfoCount) {
            Err = ERROR_NO_MORE_ITEMS;
            goto clean0;
        }

        //
        // Find the element corresponding to the specified index (using our
        // enumeration hint optimization, if possible)
        //
        if(pDeviceInfoSet->DeviceInfoEnumHintIndex <= MemberIndex) {
            MYASSERT(pDeviceInfoSet->DeviceInfoEnumHint);
            DevInfoElem = pDeviceInfoSet->DeviceInfoEnumHint;
            i = pDeviceInfoSet->DeviceInfoEnumHintIndex;
        } else {
            DevInfoElem = pDeviceInfoSet->DeviceInfoHead;
            i = 0;
        }
        for(; i < MemberIndex; i++) {
            DevInfoElem = DevInfoElem->Next;
        }

        if(!(DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                              DevInfoElem,
                                              DeviceInfoData))) {
            Err = ERROR_INVALID_USER_BUFFER;
        }

        //
        // Remember this element as our new enumeration hint.
        //
        pDeviceInfoSet->DeviceInfoEnumHintIndex = MemberIndex;
        pDeviceInfoSet->DeviceInfoEnumHint = DevInfoElem;

clean0:
        ; // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiRegisterDeviceInfo(
    IN     HDEVINFO           DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA   DeviceInfoData,
    IN     DWORD              Flags,
    IN     PSP_DETSIG_CMPPROC CompareProc,      OPTIONAL
    IN     PVOID              CompareContext,   OPTIONAL
    OUT    PSP_DEVINFO_DATA   DupDeviceInfoData OPTIONAL
    )
/*++

Routine Description:

    This API registers a device instance with the Plug & Play Manager.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set that contains
        the device information element for this device instance.

    DeviceInfoData - Supplies a pointer to the SP_DEVINFO_DATA structure for the
        device instance being registered.  This is an IN OUT parameter, since the
        DevInst field of the structure may be updated with a new handle value upon
        return.

    Flags - Controls how the device is to be registered.  May be a combination of
        the following values:

            SPRDI_FIND_DUPS - Search for a previously-existing device instance
                              corresponding to this device information.  If this
                              flag is not specified, the device instance will be
                              registered, regardless of whether a device instance
                              already exists for it.

    CompareProc - Optionally, supplies a comparison callback function to be used in
        duplicate detection.  If specified, the function will be called for each
        device instance that is of the same class as the device instance being
        registered.  The prototype of the callback function is as follows:

            typedef DWORD (CALLBACK* PSP_DETSIG_CMPPROC)(
                IN HDEVINFO         DeviceInfoSet,
                IN PSP_DEVINFO_DATA NewDeviceData,
                IN PSP_DEVINFO_DATA ExistingDeviceData,
                IN PVOID            CompareContext      OPTIONAL
                );

        The compare function must return ERROR_DUPLICATE_FOUND if it finds the two
        devices to be duplicates of each other, and NO_ERROR otherwise.  If some
        other error (e.g., out-of-memory) is encountered, the callback should return
        the appropriate ERROR_* code indicating the failure that occurred.

        If a CompareProc is not supplied, and duplicate detection is requested, then a
        default comparison behavior will be used.  (See pSetupDupDevCompare for details.)

    CompareContext - Optionally, supplies the address of a caller-supplied context
        buffer that will be passed into the compare callback routine.  This parameter
        is ignored if CompareProc is not supplied.

    DupDeviceInfoData - Optionally, supplies a pointer to a device information
        element that will be initialized for the duplicate device instance, if any,
        discovered as a result of attempting to register this device.  This will
        be filled in if the function returns FALSE, and GetLastError returns
        ERROR_DUPLICATE_FOUND.  This device information element will be added as
        a member of the specified DeviceInfoSet (if it wasn't already a member).
        If DupDeviceInfoData is not supplied, then the duplicate WILL NOT be added
        to the device information set.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    After registering a device information element, the caller should refresh any
    stored copies of the devinst handle associated with this device, as the handle
    value may have changed during registration.  The caller need not re-retrieve
    the SP_DEVINFO_DATA structure, because the devinst field of the DeviceInfoData
    structure will be updated to reflect the current handle value.

    This API may invalidate our devinfo element enumeration hint.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem, CurDevInfoElem;
    CONFIGRET cr;
    ULONG DevIdBufferLen, ulStatus, ulProblem;
    PTCHAR DevIdBuffer = NULL;
    PTSTR CurDevId;
    DEVINST ParentDevInst;
    BOOL AlreadyPresent;
    SP_DEVINFO_DATA CurDevInfoData;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    DEFAULT_DEVCMP_CONTEXT DevCmpContext;
    LOG_CONF NewDevLogConfig;
    RES_DES NewDevResDes;

    if(Flags & ~SPRDI_FIND_DUPS) {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    //
    // Initialize the following variables so we'll know whether we need to free any of their
    // associated resources.
    //
    ZeroMemory(&DevCmpContext, sizeof(DevCmpContext));
    NewDevLogConfig = (LOG_CONF)NULL;
    NewDevResDes = (RES_DES)NULL;

    try {

        DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                DeviceInfoData,
                                                NULL
                                               );
        if(!DevInfoElem) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        } else if(DevInfoElem->DiElemFlags & DIE_IS_REGISTERED) {
            //
            // Nothing to do--it's already been registered.
            //
            goto clean0;
        }

        //
        // If the caller requested duplicate detection then retrieve
        // all device instances of this class, and compare each one
        // with the device instance being registered.
        //
        if(Flags & SPRDI_FIND_DUPS) {

            do {

                if(CM_Get_Device_ID_List_Size_Ex(&DevIdBufferLen, NULL, CM_GETIDLIST_FILTER_NONE,
                                                 pDeviceInfoSet->hMachine) != CR_SUCCESS) {
                    Err = ERROR_INVALID_DATA;
                    goto clean0;
                } else if(!DevIdBufferLen) {
                    break;
                }

                if(!(DevIdBuffer = MyMalloc(DevIdBufferLen * sizeof(TCHAR)))) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean0;
                }

                cr = CM_Get_Device_ID_List_Ex(NULL,
                                           DevIdBuffer,
                                           DevIdBufferLen,
                                           CM_GETIDLIST_FILTER_NONE,
                                           pDeviceInfoSet->hMachine);
                if(cr == CR_BUFFER_SMALL) {
                    //
                    // This will only happen if a device instance was added between
                    // the time that we calculated the size, and when we attempted
                    // to retrieve the list.  In this case, we'll simply retrieve
                    // the size again, and re-attempt to retrieve the list.
                    //
                    MyFree(DevIdBuffer);
                    DevIdBuffer = NULL;
                } else if(cr != CR_SUCCESS) {
                    Err = ERROR_INVALID_DATA;
                    goto clean0;
                }

            } while(cr == CR_BUFFER_SMALL);

            if(!DevIdBufferLen) {
                goto NoDups;
            }

            //
            // Initialize the structure to be used during duplicate comparison callback.
            //
            CurDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

            //
            // We have retrieved a list of every device instance in the system--now
            // do the comparison for each one that matches the class of the device
            // being registered.
            //

            if(!CompareProc) {
                //
                // We are supposed to do the comparisons, so set up to do our default comparison.
                //
                if((cr = CM_Get_First_Log_Conf_Ex(&NewDevLogConfig,
                                               DevInfoElem->DevInst,
                                               BOOT_LOG_CONF,
                                               pDeviceInfoSet->hMachine)) != CR_SUCCESS) {
                    //
                    // Ensure that our NewDevLogConfig handle is still NULL, so we won't try
                    // to free it.
                    //
                    NewDevLogConfig = (LOG_CONF)NULL;

                    if(cr == CR_INVALID_DEVINST) {
                        Err = ERROR_INVALID_PARAMETER;
                        goto clean0;
                    } else {
                        //
                        // The only value we should get here is CR_NO_MORE_LOG_CONF.
                        // In this case, there is no comparison data, so we assume there is
                        // no possibility of duplication.
                        //
                        goto NoDups;
                    }
                }

                if(CM_Get_Next_Res_Des_Ex(&NewDevResDes,
                                       NewDevLogConfig,
                                       ResType_ClassSpecific,
                                       NULL,
                                       0,
                                       pDeviceInfoSet->hMachine) != CR_SUCCESS) {
                    //
                    // Ensure that our NewDevResDes is still NULL, so we won't try to free it.
                    //
                    NewDevResDes = (RES_DES)NULL;

                    //
                    // Since we can't retrieve the ResDes handle, assume there are no duplicates.
                    //
                    goto NoDups;
                }

                //
                // Now retrieve the actual data for the ResDes.
                //
                do {

                    if((CM_Get_Res_Des_Data_Size_Ex(&DevCmpContext.CsResourceSize,
                                                 NewDevResDes,
                                                 0,
                                                 pDeviceInfoSet->hMachine) != CR_SUCCESS) ||
                       !DevCmpContext.CsResourceSize) {
                        //
                        // Can't find out the size of the data, or there is none--assume no dups.
                        //
                        goto NoDups;
                    }

                    if(DevCmpContext.NewDevCsResource = MyMalloc(DevCmpContext.CsResourceSize)) {

                        if((cr = CM_Get_Res_Des_Data_Ex(NewDevResDes,
                                                     DevCmpContext.NewDevCsResource,
                                                     DevCmpContext.CsResourceSize,
                                                     0,
                                                     pDeviceInfoSet->hMachine)) != CR_SUCCESS) {

                            if(cr == CR_BUFFER_SMALL) {
                                //
                                // Then someone increased the size of the resource data before we
                                // got a chance to read it.  Free our buffer and try again.
                                //
                                MyFree(DevCmpContext.NewDevCsResource);
                                DevCmpContext.NewDevCsResource = NULL;
                            } else {
                                //
                                // Some other error occurred (highly unlikely).  Assume no dups.
                                //
                                goto NoDups;
                            }
                        }

                    } else {
                        //
                        // not enough memory--this is bad enough for us to abort.
                        //
                        Err = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean0;
                    }

                } while(cr != CR_SUCCESS);

                                //Bugbug check the mapping of Res_Des_Data to Res_Des_Data_Ex to verify this is
                                //still true
                //
                // We have successfully retrieved the class-specific resource data for the new
                // device's boot LogConfig.  Now allocate a buffer of the same size to store the
                // corresponding resource data for each device instance we're comparing against.
                // We don't have to worry about devices whose resource data is larger, because
                // CM_Get_Res_Des_Data_Ex will do a partial fill to a buffer that's not large enough
                // to contain the entire structure.  Since our default comparison only compares
                // the PnP detect signature (i.e., it ignores the legacy data at the very end of
                // the buffer, we're guaranteed that we have enough data to make the determination.
                //
                if(!(DevCmpContext.CurDevCsResource = MyMalloc(DevCmpContext.CsResourceSize))) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean0;
                }

                CompareProc = pSetupDupDevCompare;
                CompareContext = &DevCmpContext;
            }

            for(CurDevId = DevIdBuffer;
                *CurDevId;
                CurDevId += lstrlen(CurDevId) + 1) {

                Err = pSetupOpenAndAddNewDevInfoElem(pDeviceInfoSet,
                                                     CurDevId,
                                                     TRUE,
                                                     &(DevInfoElem->ClassGuid),
                                                     pDeviceInfoSet->InstallParamBlock.hwndParent,
                                                     &CurDevInfoElem,
                                                     TRUE,
                                                     &AlreadyPresent,
                                                     FALSE,
                                                     0,
                                                     pDeviceInfoSet
                                                    );

                if(Err == ERROR_NOT_ENOUGH_MEMORY) {
                    //
                    // Out-of-memory error is the only one bad enough to get us to abort.
                    //
                    goto clean0;
                } else if(Err != NO_ERROR) {
                    //
                    // Just ignore this device instance, and move on to the next.
                    //
                    Err = NO_ERROR;
                    continue;
                }

                DevInfoDataFromDeviceInfoElement(pDeviceInfoSet, CurDevInfoElem, &CurDevInfoData);

                //
                // We now have the possible duplicate in our set.  Call the comparison callback
                // routine.
                //
                Err = CompareProc(DeviceInfoSet, DeviceInfoData, &CurDevInfoData, CompareContext);

                //
                // If the device instance was created temporarily for the comparison, then it
                // may need to be destroyed.  It should be destroyed if it wasn't a duplicate,
                // or if the duplicate output parameter wasn't supplied.
                //
                if(!AlreadyPresent) {
                    if((Err != ERROR_DUPLICATE_FOUND) || !DupDeviceInfoData) {
                        SetupDiDeleteDeviceInfo(DeviceInfoSet, &CurDevInfoData);
                    }
                }

                if(Err != NO_ERROR) {
                    goto clean0;
                }
            }
        }

NoDups:

        //
        // To turn this phantom device instance into a 'live' device instance, we simply call
        // CM_Create_DevInst_Ex, which does the right thing (without reenumerating the whole
        // hardware tree!).
        //
        CM_Get_Device_ID_Ex(DevInfoElem->DevInst,
                         DeviceInstanceId,
                         SIZECHARS(DeviceInstanceId),
                         0,
                         pDeviceInfoSet->hMachine);

        CM_Get_Parent_Ex(&ParentDevInst, DevInfoElem->DevInst, 0,pDeviceInfoSet->hMachine);

        if(CM_Create_DevInst_Ex(&(DevInfoElem->DevInst),
                             DeviceInstanceId,
                             ParentDevInst,
                             CM_CREATE_DEVINST_NORMAL |
                             CM_CREATE_DEVINST_DO_NOT_INSTALL,
                             pDeviceInfoSet->hMachine) == CR_SUCCESS) {
            //
            // Device is no longer a phantom!
            //
            DevInfoElem->DiElemFlags &= ~DIE_IS_PHANTOM;
        } else {
            //
            // This should never happen!
            //
            Err = ERROR_NO_SUCH_DEVINST;
            goto clean0;
        }

        DevInfoElem->DiElemFlags |= DIE_IS_REGISTERED;

clean0:
        ; // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Access the following variables so the compiler will respect our statement
        // ordering in the try clause.
        //
        DevIdBuffer = DevIdBuffer;
        DevCmpContext.NewDevCsResource = DevCmpContext.NewDevCsResource;
        DevCmpContext.CurDevCsResource = DevCmpContext.CurDevCsResource;
        NewDevLogConfig = NewDevLogConfig;
        NewDevResDes = NewDevResDes;
    }

    if(DevIdBuffer) {
        MyFree(DevIdBuffer);
    }

    if(DevCmpContext.NewDevCsResource) {
        MyFree(DevCmpContext.NewDevCsResource);
    }

    if(DevCmpContext.CurDevCsResource) {
        MyFree(DevCmpContext.CurDevCsResource);
    }

    if(NewDevResDes) {
        CM_Free_Res_Des_Handle(NewDevResDes);
    }

    if(NewDevLogConfig) {
        CM_Free_Log_Conf_Handle(NewDevLogConfig);
    }

    if((Err == ERROR_DUPLICATE_FOUND) && DupDeviceInfoData) {
        //
        // The user supplied a buffer to receive the SP_DEVINFO_DATA
        // structure for the duplicate.
        //
        try {

            if(!(DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                                  CurDevInfoElem,
                                                  DupDeviceInfoData))) {
                Err = ERROR_INVALID_USER_BUFFER;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Err = ERROR_INVALID_USER_BUFFER;
        }
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


DWORD
pSetupOpenAndAddNewDevInfoElem(
    IN  PDEVICE_INFO_SET pDeviceInfoSet,
    IN  PCTSTR           DeviceInstanceId,
    IN  BOOL             AllowPhantom,
    IN  CONST GUID      *ClassGuid,              OPTIONAL
    IN  HWND             hwndParent,             OPTIONAL
    OUT PDEVINFO_ELEM   *DevInfoElem,
    IN  BOOL             CheckIfAlreadyPresent,
    OUT PBOOL            AlreadyPresent,         OPTIONAL
    IN  BOOL             OpenExistingOnly,
    IN  ULONG            CmLocateFlags,
    IN  PDEVICE_INFO_SET ContainingDeviceInfoSet
    )
/*++

Routine Description:

    This routine opens a DEVINST handle to an existing device instance, and
    creates a new device information element for it.  This element is added
    to the specified device information set.
    ASSUMES THAT THE CALLING ROUTINE HAS ALREADY ACQUIRED THE LOCK!

Arguments:

    DeviceInfoSet - Device information set to add the new element to.

    DeviceInstanceId - Supplies the name of the device instance to be opened.

    AllowPhantom - Specifies whether or not phantom device instances should be
        allowed.  If this flag is not set, and the specified device instance is
        not currently active, then the routine will fail with ERROR_NO_SUCH_DEVINST.

    ClassGuid - Optionally, supplies the class that the specified device instance
        must be in order to be added to the set.  If the device instance is found
        to be of some class other than the one specified, then the call will fail with
        ERROR_CLASS_MISMATCH.  If this parameter is not specified, then the only check
        that will be done on the device's class is to make sure that it matches the
        class of the set (if the set has an associated class).

    hwndParent - Optionally, supplies the handle to the top level window for
        UI relating to this element.

    DevInfoElem - Optionally, supplies the address of the variable that
        receives a pointer to the newly-allocated device information element.

    CheckIfAlreadyPresent - Specifies whether this routine should check to see whether
        the device instance is already in the specified devinfo set.

    AlreadyPresent - Optionally, supplies the address of a boolean variable
        that is set to indicate whether or not the specified device instance
        was already in the device information set.  If CheckIfAlreadyThere is FALSE,
        then this parameter is ignored.

    OpenExistingOnly - If this flag is non-zero, then only succeed if the device
        information element is already in the set.  If this flag is TRUE, then
        the CheckIfAlreadyPresent flag must also be TRUE.

    CmLocateFlags - Supplies additional flags to be passed to CM_Locate_DevInst.

    ContainingDeviceInfoSet - Supplies a pointer to the device information set
        structure with which this element is to be associated.  This may be
        different from the pDeviceInfoSet parameter if we're working against a
        cloned devinfo set (i.e., to facilitate rollback).

Return Value:

    If the function succeeds, the return value is NO_ERROR, otherwise the
    ERROR_* code is returned.

Remarks:

    Note that since new device information elements are always added at the end
    of the existing list, the enumeration ordering is preserved, thus we don't
    need to invalidate our enumeration hint.

--*/
{
    CONFIGRET cr;
    DEVINST DevInst;
    DWORD Err, DiElemFlags;
    GUID GuidBuffer;

    if((cr = CM_Locate_DevInst_Ex(&DevInst,
                                 (DEVINSTID)DeviceInstanceId,
                                 CM_LOCATE_DEVINST_NORMAL | CmLocateFlags,
                                 pDeviceInfoSet->hMachine)) == CR_SUCCESS) {

        DiElemFlags = DIE_IS_REGISTERED;

    } else {

        if(cr == CR_INVALID_DEVICE_ID) {
            return ERROR_INVALID_DEVINST_NAME;
        } else if(!AllowPhantom) {
            return ERROR_NO_SUCH_DEVINST;
        }

        //
        // It could be that the device instance is present in the registry, but
        // not currently 'live'.  If this is the case, we'll be able to get a
        // handle to it by locating it as a phantom device instance.
        //
        if(CM_Locate_DevInst_Ex(&DevInst,
                                (DEVINSTID)DeviceInstanceId,
                                CM_LOCATE_DEVINST_PHANTOM | CmLocateFlags,
                                pDeviceInfoSet->hMachine) != CR_SUCCESS) {

            return ERROR_NO_SUCH_DEVINST;
        }

        DiElemFlags = DIE_IS_REGISTERED | DIE_IS_PHANTOM;
    }

    //
    // If requested, search through the current list of device information elements
    // to see if this element already exists.
    //
    if(CheckIfAlreadyPresent) {

        if(*DevInfoElem = FindDevInfoByDevInst(pDeviceInfoSet, DevInst, NULL)) {
            //
            // Make sure that this device instance is of the proper class, if a class GUID
            // filter was supplied.
            //
            if(ClassGuid && !IsEqualGUID(ClassGuid, &((*DevInfoElem)->ClassGuid))) {
                return ERROR_CLASS_MISMATCH;
            }

            if(AlreadyPresent) {
                *AlreadyPresent = TRUE;
            }
            return NO_ERROR;

        } else if(AlreadyPresent) {
            *AlreadyPresent = FALSE;
            if(OpenExistingOnly) {
                //
                // The requested device information element isn't in the set,
                // so we must fail the call.
                //
                return ERROR_NO_SUCH_DEVICE_INTERFACE;
            }
        }
    }

    //
    // Retrieve the class GUID for this device instance.
    //
    if((Err = pSetupClassGuidFromDevInst(DevInst, pDeviceInfoSet->hMachine,&GuidBuffer)) != NO_ERROR) {
        return Err;
    }

    //
    // If a class GUID filter was specified, then make sure that it matches the
    // class GUID for this device instance.
    //
    if(ClassGuid && !IsEqualGUID(ClassGuid, &GuidBuffer)) {
        return ERROR_CLASS_MISMATCH;
    }

    return pSetupAddNewDeviceInfoElement(pDeviceInfoSet,
                                         DevInst,
                                         &GuidBuffer,
                                         NULL,
                                         hwndParent,
                                         DiElemFlags,
                                         ContainingDeviceInfoSet,
                                         DevInfoElem
                                        );
}


DWORD
pSetupDupDevCompare(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA NewDeviceData,
    IN PSP_DEVINFO_DATA ExistingDeviceData,
    IN PVOID            CompareContext
    )
/*++

Routine Description:

    This routine is the default comparison routine for SetupDiRegisterDeviceInfo.
    It is used to determine whether the new device (i.e., the one being registered) is
    a duplicate of an existing device.

    The current algorithm for duplicate detection is as follows:

        Compare the BOOT_LOG_CONF logical configurations for the two devices.  Two
        resource types are used in this comparison--ResType_IO and ResType_ClassSpecific.
        The IO ranges, if any, for the two devices will be compared to see if they're
        identical.  Also, if the devices have a class-specific resource, then the
        CSD_ClassGuid, and the Plug&Play detect signature in CSD_Signature will be
        binary-compared.

        BUGBUG (lonnym): presently, the LogConfig only supports the class-specific resource,
        so I/O resource comparison is not done.

Arguments:

    DeviceInfoSet - Supplies the handle of the device information set containing both devices
        being compared.

    NewDeviceData - Supplies the address of the SP_DEVINFO_DATA for the device being registered.

    ExistingDeviceData - Supplies the address of the SP_DEVINFO_DATA for the existing device with
        which the new device is being compared.

    CompareContext - Supplies the address of a context buffer used during the comparison.  This
        buffer is actually a DEFAULT_DEVCMP_CONTEXT structure, defined as follows:

            typedef struct _DEFAULT_DEVCMP_CONTEXT {

                PCS_RESOURCE NewDevCsResource;
                PCS_RESOURCE CurDevCsResource;
                ULONG        CsResourceSize;

            } DEFAULT_DEVCMP_CONTEXT, *PDEFAULT_DEVCMP_CONTEXT;

        NewDevCsResource points to the class-specific resource buffer for the new device.
        CurDevCsResource points to a working buffer that should be used to retrieve the
            class-specific resource for the existing device.
        CsResourceSize supplies the size in bytes of these two buffers (they're both the
            same size).

Return Value:

    If the two devices are not duplicates of each other, the return value is NO_ERROR.
    If the two devices are duplicates of each other, the return value is ERROR_DUPLICATE_FOUND.

--*/
{
    LOG_CONF ExistingDeviceLogConfig;
    RES_DES ExistingDeviceResDes;
    CONFIGRET cr;
    PDEFAULT_DEVCMP_CONTEXT DevCmpContext;
    PCS_DES NewCsDes, ExistingCsDes;
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    HMACHINE hMachine;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    hMachine = pDeviceInfoSet->hMachine;

    UnlockDeviceInfoSet(pDeviceInfoSet);


    //
    // First, retrieve the boot LogConfig for the existing device.
    //
    if(CM_Get_First_Log_Conf_Ex(&ExistingDeviceLogConfig,
                             ExistingDeviceData->DevInst,
                             BOOT_LOG_CONF,
                             hMachine) != CR_SUCCESS) {
        //
        // Couldn't get the boot LogConfig--assume this device isn't a duplicate.
        //
        return NO_ERROR;
    }

    //
    // Assume there are no duplicates.
    //
    Err = NO_ERROR;

    //
    // Now, retrieve the the ResDes handle for the class-specific resource.
    //
    if(CM_Get_Next_Res_Des_Ex(&ExistingDeviceResDes,
                           ExistingDeviceLogConfig,
                           ResType_ClassSpecific,
                           NULL,
                           0,
                           hMachine) != CR_SUCCESS) {
        //
        // Couldn't get the class-specific ResDes handle--assume this device isn't a duplicate
        //
        goto clean0;
    }

    //
    // Now, retrieve the actual data associated with this ResDes.  Note that we don't care if
    // we get a CR_BUFFER_SMALL error, because we are guaranteed that we got back at least the
    // amount of data that we have for the new device.  That's all we need to do our comparison.
    //
    DevCmpContext = (PDEFAULT_DEVCMP_CONTEXT)CompareContext;

    cr = CM_Get_Res_Des_Data_Ex(ExistingDeviceResDes,
                             DevCmpContext->CurDevCsResource,
                             DevCmpContext->CsResourceSize,
                             0,
                             hMachine);

    if((cr == CR_SUCCESS) || (cr == CR_BUFFER_SMALL)) {
        //
        // We got _at least_ enough of the buffer to do the comparison.
        //
        NewCsDes = &(DevCmpContext->NewDevCsResource->CS_Header);
        ExistingCsDes = &(DevCmpContext->CurDevCsResource->CS_Header);

        //
        //  First, see if the Plug&Play detect signatures are both the same size.
        //
        if(NewCsDes->CSD_SignatureLength == ExistingCsDes->CSD_SignatureLength) {
            //
            // See if the class GUIDs are the same.
            //
            if(IsEqualGUID(&(NewCsDes->CSD_ClassGuid), &(ExistingCsDes->CSD_ClassGuid))) {
                //
                // Finally, see if the PnP detect signatures are identical
                //
                if(!memcmp(NewCsDes->CSD_Signature,
                           ExistingCsDes->CSD_Signature,
                           NewCsDes->CSD_SignatureLength)) {
                    //
                    // We have ourselves a duplicate!
                    //
                    Err = ERROR_DUPLICATE_FOUND;
                }
            }
        }
    }

    CM_Free_Res_Des_Handle(ExistingDeviceResDes);

clean0:
    CM_Free_Log_Conf_Handle(ExistingDeviceLogConfig);

    return Err;
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiGetDeviceInstanceIdA(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    OUT PSTR             DeviceInstanceId,
    IN  DWORD            DeviceInstanceIdSize,
    OUT PDWORD           RequiredSize          OPTIONAL
    )
{
    WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN];
    PSTR deviceInstanceIdA;
    DWORD AnsiLength;
    BOOL b;
    DWORD rc;
    DWORD requiredSize;

    b = SetupDiGetDeviceInstanceIdW(
            DeviceInfoSet,
            DeviceInfoData,
            deviceInstanceId,
            MAX_DEVICE_ID_LEN,
            &requiredSize
            );

    if(!b) {
        return(FALSE);
    }

    rc = GetLastError();

    if(deviceInstanceIdA = UnicodeToAnsi(deviceInstanceId)) {

        AnsiLength = lstrlenA(deviceInstanceIdA) + 1;

        if(RequiredSize) {
            try {
                *RequiredSize = AnsiLength;
            } except(EXCEPTION_EXECUTE_HANDLER) {
                rc = ERROR_INVALID_PARAMETER;
                b = FALSE;
            }
        }

        if(DeviceInstanceIdSize >= AnsiLength) {

            if(!lstrcpyA(DeviceInstanceId,deviceInstanceIdA)) {
                //
                // lstrcpy faulted; assume caller's pointer invalid
                //
                rc = ERROR_INVALID_USER_BUFFER;
                b = FALSE;
            }
        } else {
            rc = ERROR_INSUFFICIENT_BUFFER;
            b = FALSE;
        }

        MyFree(deviceInstanceIdA);

    } else {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        b = FALSE;
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode version
//
BOOL
WINAPI
SetupDiGetDeviceInstanceIdW(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    OUT PWSTR            DeviceInstanceId,
    IN  DWORD            DeviceInstanceIdSize,
    OUT PDWORD           RequiredSize          OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    UNREFERENCED_PARAMETER(DeviceInstanceId);
    UNREFERENCED_PARAMETER(DeviceInstanceIdSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiGetDeviceInstanceId(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    OUT PTSTR            DeviceInstanceId,
    IN  DWORD            DeviceInstanceIdSize,
    OUT PDWORD           RequiredSize          OPTIONAL
    )
/*++

Routine Description:

    This routine retrieves the device instance ID associated with a device
    information element.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device information element whose ID is to be retrieved.

    DeviceInfoData - Supplies a pointer to the SP_DEVINFO_DATA structure for
        the device information element whose ID is to be retrieved.

    DeviceInstanceId - Supplies the address of a character buffer that will
        receive the ID for the specified device information element.

    DeviceInstanceIdSize - Supplies the size, in characters, of the DeviceInstanceId
        buffer.

    RequiredSize - Optionally, supplies the address of a variable that receives the
        number of characters required to store the device instance ID.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    CONFIGRET cr;
    ULONG ulLen;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }


    Err = NO_ERROR;

    try {
        //
        // Get a pointer to the element whose ID we are to retrieve.
        //
        if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // Find out how large the buffer needs to be.  We always have to
        // make this call first, because CM_Get_Device_ID_Ex doesn't return
        // a CR_BUFFER_SMALL error if there isn't room for the terminating
        // NULL.
        //
        if((cr = CM_Get_Device_ID_Size_Ex(&ulLen,
                                       DevInfoElem->DevInst,
                                       0,
                                       pDeviceInfoSet->hMachine)) == CR_SUCCESS) {
            //
            // The size returned from CM_Get_Device_ID_Size doesn't include
            // the terminating NULL.
            //
            ulLen++;

        } else {

            Err = (cr == CR_INVALID_DEVINST) ? ERROR_NO_SUCH_DEVINST
                                             : ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        if(RequiredSize) {
            *RequiredSize = ulLen;
        }

        if(DeviceInstanceIdSize < ulLen) {
            Err = ERROR_INSUFFICIENT_BUFFER;
            goto clean0;
        }

        //
        // Now retrieve the ID.
        //
        if((cr = CM_Get_Device_ID_Ex(DevInfoElem->DevInst,
                                  DeviceInstanceId,
                                  DeviceInstanceIdSize,
                                  0,
                                  pDeviceInfoSet->hMachine)) != CR_SUCCESS) {
            switch(cr) {

                case CR_INVALID_POINTER :
                    Err = ERROR_INVALID_USER_BUFFER;
                    break;

                default :
                    //
                    // Should never hit this!
                    //
                    Err = ERROR_INVALID_DATA;
            }
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


DWORD
pSetupAddInterfaceDeviceToDevInfoElem(
    IN  PDEVICE_INFO_SET        DeviceInfoSet,
    IN  PDEVINFO_ELEM           DevInfoElem,
    IN  CONST GUID             *InterfaceClassGuid,
    IN  PTSTR                   InterfaceDeviceName,
    IN  BOOL                    IsActive,
    IN  BOOL                    StoreTruncateNode,
    IN  BOOL                    OpenExistingOnly,
    OUT PINTERFACE_DEVICE_NODE *InterfaceDeviceNode  OPTIONAL
    )
/*++

Routine Description:

    This routine adds the specified interface device onto a device information
    element's list of interface devices.

Arguments:

    DeviceInfoSet - Supplies a pointer to the device information set
        containing the specified element.

    DevInfoElem - Supplies a pointer to the DEVINFO_ELEM structure whose
        interface device list is being added to.

    InterfaceClassGuid - Supplies a pointer to a GUID representing the class
        that this interface device is a member of.

    InterfaceDeviceName - Supplies the symbolic link name of the interface device
        being added.

    IsActive - Specifies whether or not the interface device is presently active.

    StoreTruncateNode - If non-zero, then store the address of this device
        interface node (if newly-added) when this is the first such node added
        to the device information elements device interface node list (i.e.,
        the interface class list's InterfaceDeviceTruncateNode field is NULL).

    OpenExistingOnly - If non-zero, then only succeed if the requested device
        interface is already in the device information set.

    InterfaceDeviceNode - Optionally, supplies the address of an interface device
        node pointer to be filled in with the node created for this interface device.

Return Value:

    If success, the return value is NO_ERROR.
    If failure, the return value is ERROR_NOT_ENOUGH_MEMORY.

--*/
{
    LONG GuidIndex;
    PINTERFACE_CLASS_LIST InterfaceClassList;
    PINTERFACE_DEVICE_NODE NewInterfaceDeviceNode, CurInterfaceDevice, PrevInterfaceDevice;
    LONG SymLinkNameId;

    //
    // First, get a reference (i.e., pointer) to this interface class guid (create one
    // if it's not already present for this set).
    //
    GuidIndex = AddOrGetGuidTableIndex(DeviceInfoSet, InterfaceClassGuid, TRUE);

    if(GuidIndex == -1) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Now, get the interface class list for this class from the relevant
    // devinfo element (again, we will create a new (empty) list if it doesn't
    // already exist).
    //
    if(!(InterfaceClassList = AddOrGetInterfaceClassList(DeviceInfoSet,
                                                         DevInfoElem,
                                                         GuidIndex,
                                                         TRUE))) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Now we will add a new device interface node to this list (making sure
    // that the node isn't already there).
    //
    SymLinkNameId = pStringTableAddString(DeviceInfoSet->StringTable,
                                          InterfaceDeviceName,
                                          STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                          NULL,
                                          0
                                         );

    if(SymLinkNameId == -1) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for(CurInterfaceDevice = InterfaceClassList->InterfaceDeviceNode, PrevInterfaceDevice = NULL;
        CurInterfaceDevice;
        PrevInterfaceDevice = CurInterfaceDevice, CurInterfaceDevice = CurInterfaceDevice->Next) {

        if(CurInterfaceDevice->SymLinkName == SymLinkNameId) {
            //
            // The node is already in our list, we don't want to add it again.
            // Update the flags for this interface device to reflect whether
            // the device is presently active.
            //
            CurInterfaceDevice->Flags = (CurInterfaceDevice->Flags & ~SPINT_ACTIVE) | (IsActive ? SPINT_ACTIVE : 0);
            //
            // Return this node to the caller.
            //
            if(InterfaceDeviceNode) {
                *InterfaceDeviceNode = CurInterfaceDevice;
            }
            return NO_ERROR;
        }
    }

    //
    // The device interface node wasn't already in our list--add it (unless
    // we've been told not to)
    //
    if(OpenExistingOnly) {
        return ERROR_NO_SUCH_DEVICE_INTERFACE;
    }

    if(!(NewInterfaceDeviceNode = MyMalloc(sizeof(INTERFACE_DEVICE_NODE)))) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ZeroMemory(NewInterfaceDeviceNode, sizeof(INTERFACE_DEVICE_NODE));

    NewInterfaceDeviceNode->SymLinkName = SymLinkNameId;

    if(PrevInterfaceDevice) {
        PrevInterfaceDevice->Next = NewInterfaceDeviceNode;
    } else {
        InterfaceClassList->InterfaceDeviceNode = NewInterfaceDeviceNode;
    }
    InterfaceClassList->InterfaceDeviceCount++;

    //
    // If this is the first device interface node added to this list, then
    // remember it so we can truncate the list at this point if we later find
    // that we need to rollback (because we encountered some error).
    //
    if(StoreTruncateNode && !InterfaceClassList->InterfaceDeviceTruncateNode) {
        InterfaceClassList->InterfaceDeviceTruncateNode = NewInterfaceDeviceNode;
    }

    //
    // Store the interface class GUID index in the node, so that we can easily
    // determine the class of the node later.
    //
    NewInterfaceDeviceNode->GuidIndex = GuidIndex;

    //
    // Setup the flags for this interface device (these are the same flags that
    // the caller sees in the SP_INTERFACE_DEVICE_DATA structure).
    //
    // BUGBUG (lonnym): we currently aren't handling the SPINT_DEFAULT flag!
    //
    NewInterfaceDeviceNode->Flags = IsActive ? SPINT_ACTIVE : 0;

    //
    // Store a back-pointer in the device interface node, so that we can get
    // back to the devinfo element that owns it (there are circumstances when
    // we will be given a device interface data buffer outside of the context
    // of any devinfo element).
    //
    NewInterfaceDeviceNode->OwningDevInfoElem = DevInfoElem;

    if(InterfaceDeviceNode) {
        *InterfaceDeviceNode = NewInterfaceDeviceNode;
    }

    return NO_ERROR;
}


BOOL
WINAPI
SetupDiEnumDeviceInterfaces(
    IN  HDEVINFO                   DeviceInfoSet,
    IN  PSP_DEVINFO_DATA           DeviceInfoData,     OPTIONAL
    IN  CONST GUID                *InterfaceClassGuid,
    IN  DWORD                      MemberIndex,
    OUT PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData
    )
/*++

Routine Description:

    This API enumerates device interfaces of the specified class that are
    contained in the devinfo set (optionally, filtered based on DeviceInfoData).

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        device interfaces to be enumerated.

    DeviceInfoData - Optionally, supplies a pointer to a device information
        element for whom device interfaces are to be enumerated.

    InterfaceClassGuid - Supplies a pointer to the interface class GUID whose
        members are to be enumerated.

    MemberIndex - Supplies the zero-based index of the device interface to be
        retrieved.  If DeviceInfoData is specified, then this is relative to
        all device interfaces of the specified class owned by that device
        information element.  If DeviceInfoData is not specified, then this
        index is relative to all device interfaces contained in the device
        information set.

    InterfaceDeviceData - Supplies a pointer to a device interface data buffer
        that receives information about the specified device interface.  The
        cbSize field of this structure must be filled in with
        sizeof(SP_DEVICE_INTERFACE_DATA), or the buffer is considered invalid.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    To enumerate device interface members, an application should initially call
    the SetupDiEnumDeviceInterfaces function with the MemberIndex parameter set
    to zero.  The application should then increment MemberIndex and call the
    SetupDiEnumDeviceInterfaces function until there are no more values (i.e.,
    the function fails, and GetLastError returns ERROR_NO_MORE_ITEMS).

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err, i;
    PDEVINFO_ELEM DevInfoElem;
    LONG InterfaceClassGuidIndex;
    PINTERFACE_CLASS_LIST InterfaceClassList;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {
        //
        // Retrieve the index of this interface class GUID.
        //
        if((InterfaceClassGuidIndex = AddOrGetGuidTableIndex(pDeviceInfoSet,
                                                             InterfaceClassGuid,
                                                             FALSE)) == -1) {
            Err = ERROR_NO_MORE_ITEMS;
            goto clean0;
        }

        //
        // Find the requested interface device.
        //
        if(DeviceInfoData) {
            //
            // Then we're enumerating only those interface devices that are owned
            // by a particular devinfo element.
            //
            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }

            if(!(InterfaceClassList = AddOrGetInterfaceClassList(pDeviceInfoSet,
                                                                 DevInfoElem,
                                                                 InterfaceClassGuidIndex,
                                                                 FALSE))
               || (MemberIndex >= InterfaceClassList->InterfaceDeviceCount))
            {
                Err = ERROR_NO_MORE_ITEMS;
                goto clean0;
            }

        } else {
            //
            // We're enumerating across all devinfo elements. Find the appropriate devinfo
            // element, and adjust the member index accordingly.
            //
            for(DevInfoElem = pDeviceInfoSet->DeviceInfoHead;
                DevInfoElem;
                DevInfoElem = DevInfoElem->Next) {

                if(InterfaceClassList = AddOrGetInterfaceClassList(pDeviceInfoSet,
                                                                   DevInfoElem,
                                                                   InterfaceClassGuidIndex,
                                                                   FALSE)) {

                    if(MemberIndex < InterfaceClassList->InterfaceDeviceCount) {
                        //
                        // We've found the devinfo element containing the interface device
                        // we're looking for.
                        //
                        break;

                    } else {
                        //
                        // The interface device we're looking for isn't associated with
                        // this devinfo element.  Adjust our index to eliminate the interface
                        // devices for this element, and continue searching.
                        //
                        MemberIndex -= InterfaceClassList->InterfaceDeviceCount;
                    }
                }
            }

            if(!DevInfoElem) {
                //
                // Then the specified index was higher than the count of interface devices
                // in this devinfo set.
                //
                Err = ERROR_NO_MORE_ITEMS;
                goto clean0;
            }
        }

        //
        // If we reach this point, we've found the devinfo element that contains the requested
        // interface device, and we have a pointer to the relevant interface class list.  Now
        // all we need to do is retrieve the correct member of this list, and fill in the caller's
        // interface device data buffer with the appropriate information.
        //
        InterfaceDeviceNode = InterfaceClassList->InterfaceDeviceNode;

        for(i = 0; i < MemberIndex; i++) {
            InterfaceDeviceNode = InterfaceDeviceNode->Next;
        }

        if(!InterfaceDeviceDataFromNode(InterfaceDeviceNode, InterfaceClassGuid, DeviceInterfaceData)) {
            Err = ERROR_INVALID_USER_BUFFER;
        }

clean0:
        ; // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiGetDeviceInterfaceDetailA(
    IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,     OPTIONAL
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL
    )
{
    //
    // Since the maximum length for both the symbolic link and refstring components
    // of the interface device name is 255 characters (excluding NULL), the maximum
    // length of the entire interface device name is 512 characters
    // (255 + 255 + 1 backslash + 1 NULL character).
    //
    // Thus, we will retrieve the unicode form of this information using a maximally-
    // sized buffer, then convert it to ANSI, and store it in the caller's buffer, if
    // the caller's buffer is large enough.
    //
    BYTE UnicodeBuffer[offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath) + (512 * sizeof(WCHAR))];
    PCHAR AnsiBuffer;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W UnicodeDetailData;
    DWORD rc, UnicodeRequiredSize, ReturnBufferRequiredSize;
    int AnsiStringSize;

    //
    // Check parameters.
    //
    rc = NO_ERROR;
    try {
        if(DeviceInterfaceDetailData) {
            //
            // Check signature and make sure buffer is large enough
            // to hold fixed part and at least a valid empty string.
            //
            if((DeviceInterfaceDetailData->cbSize != sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A))
            || (DeviceInterfaceDetailDataSize < (offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_A,DevicePath)+sizeof(CHAR)))) {

                rc = ERROR_INVALID_USER_BUFFER;
            }
        } else {
            //
            // Doesn't want data, size has to be 0.
            //
            if(DeviceInterfaceDetailDataSize) {
                rc = ERROR_INVALID_USER_BUFFER;
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_USER_BUFFER;
    }

    if(rc != NO_ERROR) {
        SetLastError(rc);
        return FALSE;
    }

    UnicodeDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)UnicodeBuffer;
    UnicodeDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

    if(!SetupDiGetDeviceInterfaceDetailW(DeviceInfoSet,
                                         DeviceInterfaceData,
                                         UnicodeDetailData,
                                         sizeof(UnicodeBuffer),
                                         &UnicodeRequiredSize,
                                         DeviceInfoData)) {
        return FALSE;
    }

    //
    // We successfully retrieved the (unicode) device interface details.  Now convert it
    // to ANSI, and store it in the caller's buffer.
    //
    UnicodeRequiredSize -= offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath);
    UnicodeRequiredSize /= sizeof(WCHAR);

    //
    // Allocate an ANSI buffer to be used during the conversion.  The maximum size the buffer
    // would need to be would be 2 * NumUnicodeChars.
    //
    if(!(AnsiBuffer = MyMalloc(UnicodeRequiredSize * 2))) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    try {

        AnsiStringSize = WideCharToMultiByte(CP_ACP,
                                             0,
                                             UnicodeDetailData->DevicePath,
                                             UnicodeRequiredSize,
                                             AnsiBuffer,
                                             UnicodeRequiredSize * 2,
                                             NULL,
                                             NULL
                                             );

        if(!AnsiStringSize) {
            //
            // This should never happen!
            //
            rc = GetLastError();
            goto clean0;
        }

        ReturnBufferRequiredSize = AnsiStringSize + offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath);

        if(RequiredSize) {
            *RequiredSize = ReturnBufferRequiredSize;
        }

        if(ReturnBufferRequiredSize > DeviceInterfaceDetailDataSize) {
            rc = ERROR_INSUFFICIENT_BUFFER;
            goto clean0;
        }

        //
        // OK, so we've determined that the caller's buffer is big enough.  Now, copy the
        // ANSI data into their buffer.
        //
        CopyMemory(DeviceInterfaceDetailData->DevicePath,
                   AnsiBuffer,
                   AnsiStringSize
                  );

clean0:
        ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_USER_BUFFER;
    }

    MyFree(AnsiBuffer);

    SetLastError(rc);
    return (rc == NO_ERROR);
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupDiGetDeviceInterfaceDetailW(
    IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,     OPTIONAL
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInterfaceData);
    UNREFERENCED_PARAMETER(DeviceInterfaceDetailData);
    UNREFERENCED_PARAMETER(DeviceInterfaceDetailDataSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiGetDeviceInterfaceDetail(
    IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA   DeviceInterfaceDetailData,     OPTIONAL
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL
    )
/*++

Routine Description:

    This routine retrieves details about a particular device interface (i.e., what
    it's "name" is that you can do a CreateFile on).

Arguments:

    DeviceInfoSet - Supplies a handle to a device information set containing
        a device interface to retrieve details about.

    DeviceInterfaceData - Supplies a device interface information structure
        for which details are to be retrieved.

    DeviceInterfaceDetailData - Optionally, supplies the address of a device
        interface detail data structure that will receive additional information
        about the specified device interface.  If this parameter is not specified,
        then DeviceInterfaceDetailDataSize must be zero (this would be done if the
        caller was only interested in finding out how large of a buffer is required).
        If this parameter is specified, the cbSize field of this structure must
        be set to the size of the structure before calling this API. NOTE:
        The 'size of the structure' on input means sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA).
        Note that this is essentially just a signature and is entirely separate
        from DeviceInterfaceDetailDataSize.  See below.

    DeviceInterfaceDetailDataSize - Supplies the size, in bytes, of the
        DeviceInterfaceDetailData buffer. To be valid this buffer must be at least
        offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA, DevicePath) + sizeof(TCHAR) bytes,
        which allows storage of the fixed part of the structure and a single nul to
        terminate an empty multi_sz. (Depending on structure alignment,
        character width, and the data to be returned, this may actually be
        smaller than sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA).

    RequiredSize - Optionally, supplies the address of a variable that receives
        the number of bytes required to store the detailed device interface
        information.  This value includes both the size of the structure itself,
        and the additional number of bytes required for the variable-length
        character buffer at the end of it that holds the device path.

    DeviceInfoData - Optionally, supplies a pointer to a SP_DEVINFO_DATA structure
        that will receive information about the device information element that
        owns this device interface.  Callers that only want to retrieve this parameter
        may pass NULL for DeviceInterfaceDetailData, and pass 0 for
        DeviceInterfaceDetailDataSize.  Assuming the specified device interface is
        valid, the API will fail, with GetLastError returning ERROR_INSUFFICIENT_BUFFER.
        However, DeviceInfoData will have been correctly filled in with the
        associated device information element.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode;
    PCTSTR DevicePath;
    DWORD DevicePathLength, BufferLengthNeeded;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {
        //
        // First, find the devinfo element that owns this interface device.  This
        // is used as a form of validation, and also may be needed later on, if the
        // user supplied us with a DeviceInfoData buffer to be filled in.
        //
        if(!(DevInfoElem = FindDevInfoElemForInterfaceDevice(pDeviceInfoSet, DeviceInterfaceData))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // The Reserved field contains a pointer to the underlying interface device node.
        //
        InterfaceDeviceNode = (PINTERFACE_DEVICE_NODE)(DeviceInterfaceData->Reserved);

        DevicePath = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                              InterfaceDeviceNode->SymLinkName
                                             );

        DevicePathLength = (lstrlen(DevicePath) + 1) * sizeof(TCHAR);

        //
        // Before attempting to store the device path in the caller's buffer, check to see
        // whether they requested that the associated devinfo element be returned.  If so,
        // do that first.
        //
        if(DeviceInfoData) {

            if(!(DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                                  DevInfoElem,
                                                  DeviceInfoData))) {
                Err = ERROR_INVALID_USER_BUFFER;
                goto clean0;
            }
        }

        //
        // Validate the caller's buffer.
        //
        if(DeviceInterfaceDetailData) {

            if((DeviceInterfaceDetailDataSize <
                (offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA, DevicePath) + sizeof(TCHAR))) ||
               (DeviceInterfaceDetailData->cbSize != sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA))) {

                Err = ERROR_INVALID_USER_BUFFER;
                goto clean0;
            }

        } else if(DeviceInterfaceDetailDataSize) {
            Err = ERROR_INVALID_USER_BUFFER;
            goto clean0;
        }

        //
        // Compute the buffer size required.
        //
        BufferLengthNeeded = offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA, DevicePath) + DevicePathLength;

        if(RequiredSize) {
            *RequiredSize = BufferLengthNeeded;
        }

        if(BufferLengthNeeded > DeviceInterfaceDetailDataSize) {
            Err = ERROR_INSUFFICIENT_BUFFER;
            goto clean0;
        }

        CopyMemory(DeviceInterfaceDetailData->DevicePath, DevicePath, DevicePathLength);

clean0: ;   // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiOpenDeviceInterfaceA(
    IN  HDEVINFO                  DeviceInfoSet,
    IN  PCSTR                     DevicePath,
    IN  DWORD                     OpenFlags,
    OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData OPTIONAL
    )
{
    PCWSTR UnicodeDevicePath;
    DWORD rc;

    rc = CaptureAndConvertAnsiArg(DevicePath, &UnicodeDevicePath);
    if(rc == NO_ERROR) {

        rc = _SetupDiOpenInterfaceDevice(DeviceInfoSet,
                                         (PWSTR)UnicodeDevicePath,
                                         OpenFlags,
                                         DeviceInterfaceData
                                        );

        MyFree(UnicodeDevicePath);

    }

    SetLastError(rc);
    return(rc == NO_ERROR);
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupDiOpenDeviceInterfaceW(
    IN  HDEVINFO                  DeviceInfoSet,
    IN  PCWSTR                    DevicePath,
    IN  DWORD                     OpenFlags,
    OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DevicePath);
    UNREFERENCED_PARAMETER(OpenFlags);
    UNREFERENCED_PARAMETER(DeviceInterfaceData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiOpenDeviceInterface(
    IN  HDEVINFO                  DeviceInfoSet,
    IN  PCTSTR                    DevicePath,
    IN  DWORD                     OpenFlags,
    OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData OPTIONAL
    )
/*++

Routine Description:

    This routine opens up the device information element that exposes the
    specified device interface (if it's not already in the device information
    set), and then adds this device interface to the set.

Arguments:

    DeviceInfoSet - Supplies a handle to a device information set into which this
        new device interface element is to be opened.

        NOTE:  The class of the underlying device instance must match the class
        of the set (or the set should have no associated class).  If this is not
        the case, the call will fail, and GetLastError will return ERROR_CLASS_MISMATCH.

    DevicePath - Supplies the name of the device interface to be opened.  This name
        is a Win32 device path of the form "\\?\<InterfaceDeviceName>[\<RefString>]",
        and is returned via a previous enumeration of device interface (i.e., via
        SetupDiGetClassDevs(...DIGCF_INTERFACEDEVICE) or by notification via
        RegisterDeviceNotification).

    OpenFlags - Supplies flags controlling how the device interface element is
        to be opened.  May be a combination of the following values:

        DIODI_NO_ADD - Only succeed the call (and optionally return the device
                       interface data) if the device interface already exists
                       in the device information set.  This flag may be used to
                       get a device interface data context buffer back given a
                       device interface name, without causing that interface to
                       be opened if it's not already in the set.

                       This is useful, for example, when an app receives a
                       device interface removal notification.  Such an app will
                       want to remove the corresponding device interface data
                       from the device information they're using as a container,
                       but they wouldn't want to open up a device interface
                       element not already in the set just so they can close it.

    DeviceInterfaceData - Optionally, supplies a pointer to a device interface data
        buffer that receives information about the specified device interface.  The
        cbSize field of this structure must be filled in with sizeof(SP_DEVICE_INTERFACE_DATA)
        or the buffer is considered invalid.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    If the new device interface was successfully opened, but the user-supplied
    DeviceInterfaceData buffer is invalid, this API will return FALSE, with
    GetLastError returning ERROR_INVALID_USER_BUFFER.  The device interface
    element _will_ have been added as a new member of the set, however.

    If the device interface already exists in the set, the flags will be updated
    to reflect the current state of the device.  Thus, for example, if a device
    was not active when originally opened into the set, but has since become
    active, this API may be used to 'refresh' the flags on that device interface
    element, so that the SPINT_ACTIVE bit is once again in sync with reality.

    Note that since new device information elements are always added at the end
    of the existing list, the enumeration ordering is preserved, thus we don't
    need to invalidate our enumeration hint.

--*/
{
    PCTSTR WritableDevicePath;
    DWORD rc;

    rc = CaptureStringArg(DevicePath, &WritableDevicePath);
    if(rc == NO_ERROR) {

        rc = _SetupDiOpenInterfaceDevice(DeviceInfoSet,
                                         (PTSTR)WritableDevicePath,
                                         OpenFlags,
                                         DeviceInterfaceData
                                        );

        MyFree(WritableDevicePath);
    }

    SetLastError(rc);
    return(rc == NO_ERROR);
}


DWORD
_SetupDiOpenInterfaceDevice(
    IN  HDEVINFO                  DeviceInfoSet,
    IN  PTSTR                     DevicePath,
    IN  DWORD                     OpenFlags,
    OUT PSP_DEVICE_INTERFACE_DATA InterfaceDeviceData OPTIONAL
    )
/*++

Routine Description:

    Worker routine for SetupDiOpenInterfaceDevice(A|W).  This is a separate routine
    so that both A and W versions can capture their DevicePath argument into a
    writable buffer, because we need this for adding the case-insensitive form to
    the string table.

Arguments:

    See SetupDiOpenInterfaceDevice for details.

Return Value:

    If the function succeeds, the return value is NO_ERROR.  Otherwise, it is a
    Win32 error code.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err, DevicePathLen;
    PCTSTR p;
    TCHAR InterfaceGuidString[GUID_STRING_LEN];
    GUID InterfaceGuid;
    HKEY hKey;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    BOOL DevInfoAlreadyPresent, IsActive;
    PDEVINFO_ELEM DevInfoElem;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode;

    if(OpenFlags & ~DIODI_NO_ADD) {
        return ERROR_INVALID_FLAGS;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        return ERROR_INVALID_HANDLE;
    }

    Err = NO_ERROR;
    hKey = INVALID_HANDLE_VALUE;
    DevInfoElem = NULL;

    try {
        //
        // Retrieve the interface class of this device.  Since the device path is of
        // the form "\\?\MungedDevInstName#{InterfaceClassGuid}[\RefString]", we can
        // retrieve the GUID from the name.
        //
        // NOTE: The algorithm about how this name is generated must be kept in sync
        // with the kernel-mode implementation of IoRegisterDeviceClassAssocation, et. al.
        //
        DevicePathLen = lstrlen(DevicePath);

        //
        // Move past "\\?\" prefix (also allow "\\.\" until Memphis fixes their code)
        //
        if((DevicePathLen < 4) ||
           (DevicePath[0] != TEXT('\\')) ||
           (DevicePath[1] != TEXT('\\')) ||
           ((DevicePath[2] != TEXT('?')) && (DevicePath[2] != TEXT('.'))) ||
           (DevicePath[3] != TEXT('\\')))
        {
            Err = ERROR_BAD_PATHNAME;
            goto clean0;
        }

        p = _tcschr(&(DevicePath[4]), TEXT('\\'));

        if(!p) {
            //
            // This name has no refstring--set the pointer to the end of the string
            //
            p = DevicePath + DevicePathLen;
        }

        //
        // Make sure there are enough characters preceding the current position for a
        // GUID to fit.
        //
        if(p < (DevicePath + 3 + GUID_STRING_LEN)) {
            Err = ERROR_BAD_PATHNAME;
            goto clean0;
        }

        lstrcpyn(InterfaceGuidString, p - (GUID_STRING_LEN - 1), SIZECHARS(InterfaceGuidString));

        if(pSetupGuidFromString(InterfaceGuidString, &InterfaceGuid) != NO_ERROR) {
            Err = ERROR_BAD_PATHNAME;
            goto clean0;
        }

        //
        // OK, now we know that we retrieved a valid GUID from an (apparently) valid device path.
        // Go open up this interface device key under the DeviceClasses registry branch.
        //
        hKey = SetupDiOpenClassRegKeyEx(&InterfaceGuid,
                                        KEY_READ,
                                        DIOCR_INTERFACE,
                                        NULL,
                                        NULL
                                       );

        if(hKey == INVALID_HANDLE_VALUE) {
            Err = GetLastError();
            goto clean0;
        }

        if(NO_ERROR != (Err = pSetupGetDevInstNameAndStatusForInterfaceDevice(
                                  hKey,
                                  DevicePath,
                                  DeviceInstanceId,
                                  SIZECHARS(DeviceInstanceId),
                                  &IsActive)))
        {
            goto clean0;
        }

        if(NO_ERROR != (Err = pSetupOpenAndAddNewDevInfoElem(pDeviceInfoSet,
                                                             DeviceInstanceId,
                                                             TRUE,
                                                             NULL,
                                                             NULL,
                                                             &DevInfoElem,
                                                             TRUE,
                                                             &DevInfoAlreadyPresent,
                                                             (OpenFlags & DIODI_NO_ADD),
                                                             0,
                                                             pDeviceInfoSet)))
        {
            //
            // Make sure DevInfoElem is still NULL, so we won't try to delete it.
            //
            DevInfoElem = NULL;

            goto clean0;
        }

        //
        // Now that we've successfully opened up the device instance that 'owns'
        // this interface device, add a new interface device node onto this
        // devinfo element's list.
        //
        if((NO_ERROR == (Err = pSetupAddInterfaceDeviceToDevInfoElem(pDeviceInfoSet,
                                                                     DevInfoElem,
                                                                     &InterfaceGuid,
                                                                     DevicePath,
                                                                     IsActive,
                                                                     FALSE,
                                                                     (OpenFlags & DIODI_NO_ADD),
                                                                     &InterfaceDeviceNode)))
           || DevInfoAlreadyPresent)
        {
            //
            // Either we successfully added the interface device or the owning devinfo element
            // was already in the set.  In either case, we want to reset the DevInfoElem pointer
            // to NULL so we won't try to delete it from the set.
            //
            DevInfoElem = NULL;
        }

clean0: ; // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Reference the following variables so the compiler will respect statement ordering
        // w.r.t. assignment.
        //
        DevInfoElem = DevInfoElem;
        hKey = hKey;
    }

    if(hKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKey);
    }

    if(Err != NO_ERROR) {

        if(DevInfoElem) {

            SP_DEVINFO_DATA DeviceInfoData;

            DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            DevInfoDataFromDeviceInfoElement(pDeviceInfoSet, DevInfoElem, &DeviceInfoData);
            SetupDiDeleteDeviceInfo(DeviceInfoSet, &DeviceInfoData);
        }

    } else if(InterfaceDeviceData) {

        try {

            if(!InterfaceDeviceDataFromNode(InterfaceDeviceNode, &InterfaceGuid, InterfaceDeviceData)) {
                Err = ERROR_INVALID_USER_BUFFER;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Err = ERROR_INVALID_USER_BUFFER;
        }
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    return Err;
}


BOOL
WINAPI
SetupDiGetDeviceInterfaceAlias(
    IN  HDEVINFO                   DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData,
    IN  CONST GUID                *AliasInterfaceClassGuid,
    OUT PSP_DEVICE_INTERFACE_DATA  AliasDeviceInterfaceData
    )
/*++

Routine Description:

    This routine retrieves the device interface of a particular class that 'aliases'
    the specified device interface.  Two device interfaces are considered aliases of
    each other if the following to criteria are met:

        1.  Both device interfaces are exposed by the same device instance.
        2.  Both device interfaces share the same RefString.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing the
        device interface for which an alias is to be retrieved.

    DeviceInterfaceData - Specifies the device interface whose alias is to be
        retrieved.

    AliasInterfaceClassGuid - Supplies a pointer to the GUID representing the interface
        class for which the alias is to be retrieved.

    AliasDeviceInterfaceData - Supplies a pointer to a device interface data buffer
        that receives information about the alias device interface.  The cbSize field
        of this structure must be filled in with sizeof(SP_DEVICE_INTERFACE_DATA) or
        the buffer is considered invalid.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

    Remarks:

    If the alias device interface was successfully opened, but the user-supplied
    AliasDeviceInterfaceData buffer is invalid, this API will return FALSE, with
    GetLastError returning ERROR_INVALID_USER_BUFFER.  The alias device interface
    element _will_ have been added as a new member of the set, however.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem, DevInfoElem2;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode;
    PCTSTR DevicePath;
    PTSTR AliasPath;
    ULONG AliasPathLength;
    CONFIGRET cr;
    SP_DEVICE_INTERFACE_DATA TempInterfaceDevData;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;
    AliasPath = NULL;

    try {
        //
        // First, find the devinfo element that owns this interface device (for validation).
        //
        if(!(DevInfoElem = FindDevInfoElemForInterfaceDevice(pDeviceInfoSet, DeviceInterfaceData))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // The Reserved field contains a pointer to the underlying interface device node.
        //
        InterfaceDeviceNode = (PINTERFACE_DEVICE_NODE)(DeviceInterfaceData->Reserved);

        //
        // Get the device path for this interface device.
        //
        DevicePath = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                              InterfaceDeviceNode->SymLinkName
                                             );

        //
        // Choose a buffer size that should always be large enough (we know this is the
        // case today, but since there is no defined maximum length on this path, we leave
        // the capability for it to grow in the future).
        //
        AliasPathLength = 512;

        while(TRUE) {

            if(!(AliasPath = MyMalloc(AliasPathLength * sizeof(TCHAR)))) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }

            //
            // Now retrieve the name of this interface device's alias in the specified class.
            //
            cr = CM_Get_Device_Interface_Alias_Ex(DevicePath,
                                               (LPGUID)AliasInterfaceClassGuid,
                                               AliasPath,
                                               &AliasPathLength,
                                               0,
                                               pDeviceInfoSet->hMachine);

            if(cr == CR_SUCCESS) {
                break;
            } else {
                //
                // If our buffer was too small, then free it, and try again with a larger buffer.
                //
                if(cr == CR_BUFFER_SMALL) {
                    MyFree(AliasPath);
                    AliasPath = NULL;
                } else {
                    Err = MapCrToSpError(cr, ERROR_NO_SUCH_DEVICE_INTERFACE);
                    goto clean0;
                }
            }
        }

        //
        // If we get to here then we've successfully retrieved the alias name.  Now open this
        // interface device in our device information set.
        //
        TempInterfaceDevData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        if(!SetupDiOpenDeviceInterface(DeviceInfoSet,
                                       AliasPath,
                                       0,
                                       &TempInterfaceDevData)) {
            //
            // This should never happen.
            //
            Err = GetLastError();
            goto clean0;
        }

        //
        // Retrieve the device information element for this alias interface device (this has to succeed).
        //
        DevInfoElem2 = FindDevInfoElemForInterfaceDevice(pDeviceInfoSet, DeviceInterfaceData);

        //
        // Since these two interface devices are aliases of each other, they'd better be owned by
        // the same devinfo element!
        //
        MYASSERT(DevInfoElem == DevInfoElem2);

        InterfaceDeviceNode = (PINTERFACE_DEVICE_NODE)(TempInterfaceDevData.Reserved);

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Reference the following variable so the compiler will respect our statement ordering
        // w.r.t. assignment.
        //
        AliasPath = AliasPath;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    if(AliasPath) {
        MyFree(AliasPath);
    }

    if(Err == NO_ERROR) {

        try {

            if(!InterfaceDeviceDataFromNode(InterfaceDeviceNode,
                                            AliasInterfaceClassGuid,
                                            AliasDeviceInterfaceData)) {

                Err = ERROR_INVALID_USER_BUFFER;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Err = ERROR_INVALID_USER_BUFFER;
        }
    }

    SetLastError(Err);
    return(Err == NO_ERROR);

}


DWORD
pSetupGetDevInstNameAndStatusForInterfaceDevice(
    IN  HKEY   hKeyInterfaceClass,
    IN  PCTSTR InterfaceDeviceName,
    OUT PTSTR  OwningDevInstName,     OPTIONAL
    IN  DWORD  OwningDevInstNameSize,
    OUT PBOOL  IsActive               OPTIONAL
    )
/*++

Routine Description:

    This routine retrieves the name of the device instance that exposes the specified
    interface device and whether or not that interface device is currently active.

Arguments:

    hKeyInterfaceClass - Supplies a handle to the registry key for the interface class
        of which this interface device is a member.  E.g.,

        HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{InterfaceClassGuid}

    InterfaceDeviceName - Supplies the name of the interface device.

    OwningDevInstName - Optionally, supplies the address of a character buffer that
        receives the name of the device instance that exposes this interface device.
        This buffer should be at least MAX_DEVICE_ID_LEN characters long.

    OwningDevInstNameSize - Supplies the size, in characters, of the OwningDevInstName
        buffer.

    IsActive - Optionally, supplies the address of a boolean variable that is set upon
        return to indicate whether this interface is presently exposed.

Return Value:

    If the function succeeds, the return value is NO_ERROR.  Otherwise, it is a Win32
    error code.

--*/
{
    DWORD Err, DataBufferSize, RegDataType;
    HKEY hKeyInterfaceDevice, hKeyControl;

    hKeyInterfaceDevice = hKeyControl = INVALID_HANDLE_VALUE;

    try {

        DataBufferSize = OwningDevInstNameSize * sizeof(TCHAR);

        Err = OpenDeviceInterfaceSubKey(hKeyInterfaceClass,
                                        InterfaceDeviceName,
                                        KEY_READ,
                                        &hKeyInterfaceDevice,
                                        OwningDevInstName,
                                        &DataBufferSize
                                       );

        if(Err != ERROR_SUCCESS) {
            //
            // Make sure the key handle is still invalid, so we'll know not to
            // close it.
            //
            hKeyInterfaceDevice = INVALID_HANDLE_VALUE;
            goto clean0;
        }

        if(IsActive) {
            //
            // The user wants to find out whether this interface device is currently active.
            // Check the 'Linked' value entry under the volatile 'Control' subkey to find
            // this out.
            //
            *IsActive = FALSE;

            if(ERROR_SUCCESS == RegOpenKeyEx(hKeyInterfaceDevice,
                                             pszControl,
                                             0,
                                             KEY_READ,
                                             &hKeyControl)) {

                DataBufferSize = sizeof(*IsActive);
                if(ERROR_SUCCESS != RegQueryValueEx(hKeyControl,
                                                    pszLinked,
                                                    NULL,
                                                    NULL,
                                                    (PBYTE)IsActive,
                                                    &DataBufferSize)) {
                    *IsActive = FALSE;
                }
            }
        }

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Reference the following variables so the compiler will respect statement
        // ordering w.r.t. assignment.
        //
        hKeyInterfaceDevice = hKeyInterfaceDevice;
        hKeyControl = hKeyControl;
    }

    if(hKeyControl != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKeyControl);
    }

    if(hKeyInterfaceDevice != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKeyInterfaceDevice);
    }

    return Err;
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiCreateDeviceInterfaceA(
    IN  HDEVINFO                   DeviceInfoSet,
    IN  PSP_DEVINFO_DATA           DeviceInfoData,
    IN  CONST GUID                *InterfaceClassGuid,
    IN  PCSTR                      ReferenceString,    OPTIONAL
    IN  DWORD                      CreationFlags,
    OUT PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData OPTIONAL
    )
{
    PCWSTR UnicodeRefString;
    DWORD rc;
    BOOL b;

    b = FALSE;

    if(ReferenceString) {
        rc = CaptureAndConvertAnsiArg(ReferenceString, &UnicodeRefString);
    } else {
        UnicodeRefString = NULL;
        rc = NO_ERROR;
    }

    if(rc == NO_ERROR) {

        b = SetupDiCreateDeviceInterfaceW(DeviceInfoSet,
                                          DeviceInfoData,
                                          InterfaceClassGuid,
                                          UnicodeRefString,
                                          CreationFlags,
                                          DeviceInterfaceData
                                         );
        rc = GetLastError();

        if(UnicodeRefString) {
            MyFree(UnicodeRefString);
        }
    }

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode version
//
BOOL
WINAPI
SetupDiCreateDeviceInterfaceW(
    IN  HDEVINFO                   DeviceInfoSet,
    IN  PSP_DEVINFO_DATA           DeviceInfoData,
    IN  CONST GUID                *InterfaceClassGuid,
    IN  PCWSTR                     ReferenceString,    OPTIONAL
    IN  DWORD                      CreationFlags,
    OUT PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    UNREFERENCED_PARAMETER(InterfaceClassGuid);
    UNREFERENCED_PARAMETER(ReferenceString);
    UNREFERENCED_PARAMETER(CreationFlags);
    UNREFERENCED_PARAMETER(DeviceInterfaceData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiCreateDeviceInterface(
    IN  HDEVINFO                   DeviceInfoSet,
    IN  PSP_DEVINFO_DATA           DeviceInfoData,
    IN  CONST GUID                *InterfaceClassGuid,
    IN  PCTSTR                     ReferenceString,    OPTIONAL
    IN  DWORD                      CreationFlags,
    OUT PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData OPTIONAL
    )
/*++

Routine Description:

    This API creates (registers) a device interface for the specified device
    information element, and adds this device interface to the device information
    set.

Arguments:

    DeviceInfoSet - Supplies a handle to a device information set containing the
        device information element for which a new device interface is being added.

    DeviceInfoData - Supplies the device information element for whom a device
        interface is being added.

    InterfaceClassGuid - Supplies the address of a GUID containing the class
        for this new device interface.

    ReferenceString - Optionally, supplies the reference string to be passed to the
        driver when opening this device interface.  This string becomes part of the
        device interface's name (as an additional path component).

    CreationFlags - Reserved for future use, must be set to 0.

    DeviceInterfaceData - Optionally, supplies a pointer to a device interface data
        buffer that receives information about the newly-created device interface.
        The cbSize field of this structure must be filled in with sizeof(SP_DEVICE_INTERFACE_DATA)
        or the buffer is considered invalid.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    If the new device interface was successfully created, but the user-supplied
    DeviceInterfaceData buffer is invalid, this API will return FALSE, with
    GetLastError returning ERROR_INVALID_USER_BUFFER.  The device interface
    element _will_ have been added as a new member of the set, however.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    TCHAR InterfaceDeviceName[(2 * MAX_PATH) + 1];  // 2 max-sized regkey names + terminating NULL.
    ULONG InterfaceDeviceNameSize;
    CONFIGRET cr;
    BOOL IsActive;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode;
    HKEY hKey;

    if(CreationFlags) {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;
    hKey = INVALID_HANDLE_VALUE;

    try {
        //
        // Get a pointer to the device information element we're registering an
        // interface device for.
        //
        if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // Register the interface device.
        //
        InterfaceDeviceNameSize = SIZECHARS(InterfaceDeviceName);
        cr = CM_Register_Device_Interface_Ex(DevInfoElem->DevInst,
                                          (LPGUID)InterfaceClassGuid,
                                          ReferenceString,
                                          InterfaceDeviceName,
                                          &InterfaceDeviceNameSize,
                                          0,
                                          pDeviceInfoSet->hMachine);

        if(cr != CR_SUCCESS) {
            Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
            goto clean0;
        }

        //
        // This interface device might have already been registered, in which case it
        // could already be active.  We must check the 'Linked' registry value to see
        // whether this device is active.
        //
        hKey = SetupDiOpenClassRegKeyEx(InterfaceClassGuid,
                                        KEY_READ,
                                        DIOCR_INTERFACE,
                                        NULL,
                                        NULL
                                       );

        if(hKey != INVALID_HANDLE_VALUE) {

            if(NO_ERROR != pSetupGetDevInstNameAndStatusForInterfaceDevice(
                               hKey,
                               InterfaceDeviceName,
                               NULL,
                               0,
                               &IsActive))
            {
                //
                // This shouldn't fail, but if it does, then just assume that the
                // interface device's status is non-active.
                //
                IsActive = FALSE;
            }

        } else {
            //
            // This should never happen--if it does, assume that the interface device
            // isn't active.
            //
            IsActive = FALSE;
        }

        //
        // The interface device was successfully registered, now add it to the list of
        // interface devices associated with this device information element.
        //
        Err = pSetupAddInterfaceDeviceToDevInfoElem(pDeviceInfoSet,
                                                    DevInfoElem,
                                                    InterfaceClassGuid,
                                                    InterfaceDeviceName,
                                                    IsActive,
                                                    FALSE,
                                                    FALSE,
                                                    &InterfaceDeviceNode
                                                   );

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Reference the following variable so the compiler will respect statement
        // ordering w.r.t. assignment.
        //
        hKey = hKey;
    }

    if(hKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKey);
    }

    if((Err == NO_ERROR) && DeviceInterfaceData) {

        try {
            if(!InterfaceDeviceDataFromNode(InterfaceDeviceNode, InterfaceClassGuid, DeviceInterfaceData)) {
                Err = ERROR_INVALID_USER_BUFFER;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Err = ERROR_INVALID_USER_BUFFER;
        }
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return (Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiDeleteDeviceInterfaceData(
    IN HDEVINFO                  DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
    )
/*++

Routine Description:

    This API deletes the specified device interface element from the device
    information set.  It _does not_ remove (unregister) the device interface
    from the system (to do that, use SetupDiRemoveDeviceInterface).

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        device interface to be deleted.

    DeviceInterfaceData - Specifies the device interface to be deleted.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    After a device interface is deleted, the device interface enumeration index
    is invalid, and enumeration should be re-started at index 0.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode, CurInterfaceDeviceNode, PrevInterfaceDeviceNode;
    PINTERFACE_CLASS_LIST InterfaceClassList;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {
        //
        // First, find the devinfo element that owns this interface device.
        //
        if(!(DevInfoElem = FindDevInfoElemForInterfaceDevice(pDeviceInfoSet, DeviceInterfaceData))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // The Reserved field contains a pointer to the underlying interface device node.
        //
        InterfaceDeviceNode = (PINTERFACE_DEVICE_NODE)(DeviceInterfaceData->Reserved);

        //
        // Find this devinfo element's interface device list for this class.
        //
        if(!(InterfaceClassList = AddOrGetInterfaceClassList(pDeviceInfoSet,
                                                             DevInfoElem,
                                                             InterfaceDeviceNode->GuidIndex,
                                                             FALSE)))
        {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // Find this interface device node in the list of interface devices for this device
        // information element.
        //
        for(CurInterfaceDeviceNode = InterfaceClassList->InterfaceDeviceNode, PrevInterfaceDeviceNode = NULL;
            CurInterfaceDeviceNode;
            PrevInterfaceDeviceNode = CurInterfaceDeviceNode, CurInterfaceDeviceNode = CurInterfaceDeviceNode->Next)
        {
            if(CurInterfaceDeviceNode == InterfaceDeviceNode) {
                break;
            }
        }

        if(!CurInterfaceDeviceNode) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        MYASSERT(InterfaceClassList->InterfaceDeviceCount);

        if(PrevInterfaceDeviceNode) {
            PrevInterfaceDeviceNode->Next = CurInterfaceDeviceNode->Next;
        } else {
            InterfaceClassList->InterfaceDeviceNode = CurInterfaceDeviceNode->Next;
        }

        MyFree(InterfaceDeviceNode);
        InterfaceClassList->InterfaceDeviceCount--;

clean0: ;   // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiRemoveDeviceInterface(
    IN     HDEVINFO                  DeviceInfoSet,
    IN OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
    )
/*++

Routine Description:

    This API removes (unregisters) the specified device interface.  It _does not_
    delete the device interface element from the device information set (thus
    enumeration is not affected).  Instead, it marks the device interface element
    as invalid, so that it cannot be used in any subsequent API calls except
    SetupDiDeleteDeviceInterfaceData.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        device interface to be removed.

    DeviceInterfaceData - Specifies the device interface to be removed.  All
        traces of this device will be removed from the registry.

        Upon return, the Flags field of this structure will be updated to reflect
        the new state of this device interface.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    There is no way to unregister a device interface while it is active.  Thus,
    this API will fail with ERROR_DEVICE_INTERFACE_ACTIVE in this case.  If this
    happens, you can do one of the following things in an attempt to remove the
    device interface:

        1.  If there is some defined mechanism of communication to the device
            interface/underlying device instance (e.g., an IOCTL) that causes the
            driver to un-expose the device interface, then this method may be used,
            and _then_ SetupDiRemoveDeviceInterface may be called.

        2.  If there is no mechanism as described in method (1), then the owning
            device instance must be stopped (e.g., via SetupDiChangeState), which
            will cause all device interfaces owned by that device instance to go
            inactive.  After that is done, then SetupDiRemoveDeviceInterface may
            be called.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode;
    PCTSTR DevicePath;
    CONFIGRET cr;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {
        //
        // Get a pointer to the device information element for the specified
        // interface device.
        //
        if(!(DevInfoElem = FindDevInfoElemForInterfaceDevice(pDeviceInfoSet, DeviceInterfaceData))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // The Reserved field contains a pointer to the underlying interface device node.
        //
        InterfaceDeviceNode = (PINTERFACE_DEVICE_NODE)(DeviceInterfaceData->Reserved);

        //
        // OK, now open the interface device's root storage key.
        //
        DevicePath = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                              InterfaceDeviceNode->SymLinkName
                                             );

        cr = CM_Unregister_Device_Interface_Ex(DevicePath, 0,pDeviceInfoSet->hMachine);

        if(cr != CR_SUCCESS) {

            switch(cr) {

                case CR_NO_SUCH_DEVICE_INTERFACE :
                    //
                    // The device interface was deleted after it was enumerated/opened
                    // by this client.  In this case, we'll go ahead and succeed this
                    // call.
                    //
                    break;

                case CR_DEVICE_INTERFACE_ACTIVE :
                    Err = ERROR_DEVICE_INTERFACE_ACTIVE;
                    //
                    // If our SPINT_ACTIVE flag isn't set, then that means that the device
                    // wasn't active the last time we looked.  Update our flag to indicate
                    // the device's new state.
                    //
                    InterfaceDeviceNode->Flags |= SPINT_ACTIVE;
                    goto clean1;

                default :
                    Err = ERROR_INVALID_DATA;
                    goto clean0;
            }
        }

        //
        // The interface device was successfully removed.  Now, mark the interface device
        // node to reflect that it's now invalid.
        //
        InterfaceDeviceNode->Flags |= SPINT_REMOVED;

        //
        // Also, clear the SPINT_ACTIVE flag, in case it's set.  It's possible that we thought
        // the device was active, even though it was deactivated since the last time we looked.
        //
        InterfaceDeviceNode->Flags &= ~SPINT_ACTIVE;

        //
        // BUGBUG (lonnym): If we removed the default device, we need to reset the default!!!
        //

clean1:
        //
        // Finally, updated the flags in the caller-supplied buffer to indicate the new status
        // of this interface device.
        //
        DeviceInterfaceData->Flags = InterfaceDeviceNode->Flags;

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
pSetupDiSetDeviceInfoContext(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Context
    )
/*++

Routine Description:

    This API stores a context value into the specified device information element
    for later retrieval via pSetupDiGetDeviceInfoContext.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device information element with which the context data is to be
        associated.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure indicating
        which element the context data should be associated with.

    Context - Specifies the data value to be stored for this device information element.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    return pSetupDiGetOrSetDeviceInfoContext(DeviceInfoSet,
                                             DeviceInfoData,
                                             Context,
                                             NULL
                                            );
}


BOOL
pSetupDiGetDeviceInfoContext(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    OUT PDWORD           Context
    )
/*++

Routine Description:

    This API retrieves a context value from the specified device information element
    (stored there via pSetupDiSetDeviceInfoContext).

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device information element with which the context data is associated.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure indicating
        which element the context data is associated with.

    Context - Supplies the address of a variable that receives the context value
        stored for the device information element in a prior call to
        pSetupDiSetDeviceInfoContext.  If no context data has previously been stored
        for this element, this variable will be filled in with zero upon return.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    //
    // If we let a NULL context pointer go through to the worker routine, it will
    // think this is a 'set' instead of a 'get'.  Make sure that doesn't happen.
    //
    if(!Context) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pSetupDiGetOrSetDeviceInfoContext(DeviceInfoSet,
                                             DeviceInfoData,
                                             0,               // ignored
                                             Context
                                            );
}


BOOL
pSetupDiGetOrSetDeviceInfoContext(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            InContext,
    OUT PDWORD           OutContext      OPTIONAL
    )
/*++

Routine Description:

    This API retrieves or sets a context value from the specified device information
    element (stored there via pSetupDiSetDeviceInfoContext).

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device information element with which the context data is associated.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure indicating
        which element the context data is associated with.

    InContext - Specifies the data value to be stored for this device information element.
        If OutContext is specified, then this is a 'get' instead of a 'set', and
        this parameter is ignored.

    OutContext - Optionally, supplies the address of a variable that receives the
        context value stored for the device information element in a prior call to
        pSetupDiSetDeviceInfoContext.  If no context data has previously been stored
        for this element, this variable will be filled in with zero upon return.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    DWORD Err;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR; // assume success.

    try {

        DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                DeviceInfoData,
                                                NULL
                                               );
        if(!DevInfoElem) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        if(OutContext) {
            //
            // Store the context in the caller-supplied buffer.
            //
            *OutContext = DevInfoElem->Context;
        } else {
            //
            // Set the context to the caller-supplied value.
            //
            DevInfoElem->Context = InContext;
        }

clean0:
        ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}

