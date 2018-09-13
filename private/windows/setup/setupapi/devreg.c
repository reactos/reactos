/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    devreg.c

Abstract:

    Device Installer routines for registry storage/retrieval.

Author:

    Lonny McMichael (lonnym) 1-July-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Private function prototypes
//
DWORD
pSetupOpenOrCreateDevRegKey(
    IN  PDEVICE_INFO_SET DeviceInfoSet,
    IN  PDEVINFO_ELEM    DevInfoElem,
    IN  DWORD            Scope,
    IN  DWORD            HwProfile,
    IN  DWORD            KeyType,
    IN  BOOL             Create,
    IN  REGSAM           samDesired,
    OUT PHKEY            hDevRegKey
    );

BOOL
pSetupFindUniqueKey(
    IN HKEY   hkRoot,
    IN LPTSTR SubKey,
    IN ULONG  SubKeyLength
    );

DWORD
pSetupOpenOrCreateInterfaceDeviceRegKey(
    IN  HKEY                      hInterfaceClassKey,
    IN  PDEVICE_INFO_SET          DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA InterfaceDeviceData,
    IN  BOOL                      Create,
    IN  REGSAM                    samDesired,
    OUT PHKEY                     hInterfaceDeviceKey
    );

DWORD
pSetupDeleteInterfaceDeviceKey(
    IN HKEY                      hInterfaceClassKey,
    IN PDEVICE_INFO_SET          DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA InterfaceDeviceData
    );


HKEY
WINAPI
SetupDiOpenClassRegKey(
    IN CONST GUID *ClassGuid, OPTIONAL
    IN REGSAM      samDesired
    )
/*++

Routine Description:

    This API opens the installer class registry key or a specific class
    installer's subkey.

Arguments:

    ClassGuid - Optionally, supplies a pointer to the GUID of the class whose
        key is to be opened.  If this parameter is NULL, then the root of the
        class tree will be opened.

    samDesired - Specifies the access you require for this key.

Return Value:

    If the function succeeds, the return value is a handle to an opened registry
    key.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

Remarks:

    This API _will not_ create a registry key if it doesn't already exist.

    The handle returned from this API must be closed by calling RegCloseKey.

    To get at the interface class (DeviceClasses) branch, or to access the
    registry on a remote machine, use SetupDiOpenClassRegKeyEx.

--*/
{
    return SetupDiOpenClassRegKeyEx(ClassGuid, samDesired, DIOCR_INSTALLER, NULL, NULL);
}


#ifdef UNICODE
//
// ANSI version
//
HKEY
WINAPI
SetupDiOpenClassRegKeyExA(
    IN CONST GUID *ClassGuid,   OPTIONAL
    IN REGSAM      samDesired,
    IN DWORD       Flags,
    IN PCSTR       MachineName, OPTIONAL
    IN PVOID       Reserved
    )
{
    PCWSTR UnicodeMachineName;
    DWORD rc;
    HKEY hk;

    hk = INVALID_HANDLE_VALUE;

    if(MachineName) {
        rc = CaptureAndConvertAnsiArg(MachineName, &UnicodeMachineName);
    } else {
        UnicodeMachineName = NULL;
        rc = NO_ERROR;
    }

    if(rc == NO_ERROR) {

        hk = SetupDiOpenClassRegKeyExW(ClassGuid,
                                       samDesired,
                                       Flags,
                                       UnicodeMachineName,
                                       Reserved
                                      );
        rc = GetLastError();

        if(UnicodeMachineName) {
            MyFree(UnicodeMachineName);
        }
    }

    SetLastError(rc);
    return hk;
}
#else
//
// Unicode version
//
HKEY
WINAPI
SetupDiOpenClassRegKeyExW(
    IN CONST GUID *ClassGuid,   OPTIONAL
    IN REGSAM      samDesired,
    IN DWORD       Flags,
    IN PCWSTR      MachineName, OPTIONAL
    IN PVOID       Reserved
    )
{
    UNREFERENCED_PARAMETER(ClassGuid);
    UNREFERENCED_PARAMETER(samDesired);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(MachineName);
    UNREFERENCED_PARAMETER(Reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


HKEY
WINAPI
SetupDiOpenClassRegKeyEx(
    IN CONST GUID *ClassGuid,   OPTIONAL
    IN REGSAM      samDesired,
    IN DWORD       Flags,
    IN PCTSTR      MachineName, OPTIONAL
    IN PVOID       Reserved
    )
/*++

Routine Description:

    This API opens the root of either the installer or the interface class registry
    branch, or a specified class subkey under one of these branches.

    If the root key is requested, it will be created if not already present (i.e.,
    you're always guaranteed to get a handle to the root unless a registry error
    occurs).

    If a particular class subkey is requested, it will be returned if present.
    Otherwise, this API will return ERROR_INVALID_CLASS.

Arguments:

    ClassGuid - Optionally, supplies a pointer to the GUID of the class whose
        key is to be opened.  If this parameter is NULL, then the root of the
        class tree will be opened.  This GUID is either an installer class or
        an interface class depending on the Flags argument.

    samDesired - Specifies the access you require for this key.

    Flags - Specifies which registry branch the key is to be opened for.  May
        be one of the following values:

        DIOCR_INSTALLER - Open the class installer (Class) branch.
        DIOCR_INTERFACE - Open the interface class (DeviceClasses) branch.

    MachineName - If specified, this value indicates the remote machine where
        the key is to be opened.

    Reserved - Reserved for future use--must be NULL.

Return Value:

    If the function succeeds, the return value is a handle to an opened registry
    key.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

Remarks:

    The handle returned from this API must be closed by calling RegCloseKey.

--*/
{
    HKEY hk;
    CONFIGRET cr;
    DWORD Err = NO_ERROR;
    HMACHINE hMachine = NULL;

    //
    // Make sure the user didn't pass us anything in the Reserved parameter.
    //
    if(Reserved) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    //
    // Validate the flags (really, just an enum for now, but treated as
    // flags for future extensibility).
    //
    if((Flags & ~(DIOCR_INSTALLER | DIOCR_INTERFACE)) ||
       ((Flags != DIOCR_INSTALLER) && (Flags != DIOCR_INTERFACE))) {

        SetLastError(ERROR_INVALID_FLAGS);
        return INVALID_HANDLE_VALUE;
    }

    try {

        if(MachineName) {

            if(CR_SUCCESS != (cr = CM_Connect_Machine(MachineName, &hMachine))) {
                //
                // Make sure machine handle is still invalid, so we won't
                // try to disconnect later.
                //
                hMachine = NULL;
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                goto clean0;
            }
        }

        if((cr = CM_Open_Class_Key_Ex((LPGUID)ClassGuid,
                                      NULL,
                                      samDesired,
                                      ClassGuid ? RegDisposition_OpenExisting
                                                : RegDisposition_OpenAlways,
                                      &hk,
                                      (Flags & DIOCR_INSTALLER) ? CM_OPEN_CLASS_KEY_INSTALLER
                                                                : CM_OPEN_CLASS_KEY_INTERFACE,
                                      hMachine)) != CR_SUCCESS)
        {
            if(cr == CR_NO_SUCH_REGISTRY_KEY) {
                Err = ERROR_INVALID_CLASS;
            } else {
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
            }
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Reference the following variable(s) so the compiler will respect our statement
        // ordering w.r.t. assignment.
        //
        hMachine = hMachine;
    }

    if(hMachine) {
        CM_Disconnect_Machine(hMachine);
    }

    SetLastError(Err);
    return (Err == NO_ERROR) ? hk : INVALID_HANDLE_VALUE;
}


#ifdef UNICODE
//
// ANSI version
//
HKEY
WINAPI
SetupDiCreateDevRegKeyA(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Scope,
    IN DWORD            HwProfile,
    IN DWORD            KeyType,
    IN HINF             InfHandle,      OPTIONAL
    IN PCSTR            InfSectionName  OPTIONAL
    )
{
    DWORD rc;
    PWSTR name;
    HKEY h;

    if(InfSectionName) {
        rc = CaptureAndConvertAnsiArg(InfSectionName,&name);
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return(INVALID_HANDLE_VALUE);
        }
    } else {
        name = NULL;
    }

    h = SetupDiCreateDevRegKeyW(
            DeviceInfoSet,
            DeviceInfoData,
            Scope,
            HwProfile,
            KeyType,
            InfHandle,
            name
            );

    rc = GetLastError();

    if(name) {
        MyFree(name);
    }
    SetLastError(rc);
    return(h);
}
#else
//
// Unicode stub
//
HKEY
WINAPI
SetupDiCreateDevRegKeyW(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Scope,
    IN DWORD            HwProfile,
    IN DWORD            KeyType,
    IN HINF             InfHandle,      OPTIONAL
    IN PCWSTR           InfSectionName  OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    UNREFERENCED_PARAMETER(Scope);
    UNREFERENCED_PARAMETER(HwProfile);
    UNREFERENCED_PARAMETER(KeyType);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(InfSectionName);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(INVALID_HANDLE_VALUE);
}
#endif

HKEY
WINAPI
SetupDiCreateDevRegKey(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Scope,
    IN DWORD            HwProfile,
    IN DWORD            KeyType,
    IN HINF             InfHandle,      OPTIONAL
    IN PCTSTR           InfSectionName  OPTIONAL
    )
/*++

Routine Description:

    This routine creates a registry storage key for device-specific configuration
    information, and returns a handle to the key.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        information about the device instance whose registry configuration storage
        key is to be created.

    DeviceInfoData - Supplies a pointer to a SP_DEVINFO_DATA structure indicating
        the device instance to create the registry key for.

    Scope - Specifies the scope of the registry key to be created.  This determines
        where the information is actually stored--the key created may be one that is
        global (i.e., constant regardless of current hardware profile) or hardware
        profile-specific.  May be one of the following values:

        DICS_FLAG_GLOBAL - Create a key to store global configuration information.

        DICS_FLAG_CONFIGSPECIFIC - Create a key to store hardware profile-specific
                                   information.

    HwProfile - Specifies the hardware profile to create a key for, if the Scope parameter
        is set to DICS_FLAG_CONFIGSPECIFIC.  If this parameter is 0, then the key
        for the current hardware profile should be created (i.e., in the Class branch
        under HKEY_CURRENT_CONFIG).  If Scope is DICS_FLAG_GLOBAL, then this parameter
        is ignored.

    KeyType - Specifies the type of registry storage key to be created.  May be one of
        the following values:

        DIREG_DEV - Create a hardware registry key for the device.  This is the key for
            storage of driver-independent configuration information.  (This key is in
            the device instance key in the Enum branch.

        DIREG_DRV - Create a software, or driver, registry key for the device.  (This key
            is located in the class branch.)

    InfHandle - Optionally, supplies the handle of an opened INF file containing an
        install section to be executed for the newly-created key.  If this parameter is
        specified, then InfSectionName must be specified as well.

    InfSectionName - Optionally, supplies the name of an install section in the INF
        file specified by InfHandle.  This section will be executed for the newly
        created key. If this parameter is specified, then InfHandle must be specified
        as well.

Return Value:

    If the function succeeds, the return value is a handle to a newly-created 
    registry key where private configuration data pertaining to this device 
    instance may be stored/retrieved.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

Remarks:

    The handle returned from this routine must be closed by calling RegCloseKey.

    The specified device instance must have been previously registered (i.e., 
    if it was created via SetupDiCreateDeviceInfo, then SetupDiRegisterDeviceInfo 
    must have been subsequently called.)
    
    During GUI-mode setup on Windows NT, quiet-install behavior is always 
    employed in the absence of a user-supplied file queue, regardless of 
    whether the device information element has the DI_QUIETINSTALL flag set.

--*/

{
    HKEY hk = INVALID_HANDLE_VALUE;
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    PSP_FILE_CALLBACK MsgHandler;
    PVOID MsgHandlerContext;
    BOOL MsgHandlerIsNativeCharWidth;
    BOOL NoProgressUI;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }

    Err = NO_ERROR;

    try {
        //
        // Get a pointer to the element for the specified device
        // instance.
        //
        if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // Create the requested registry storage key.
        //
        if((Err = pSetupOpenOrCreateDevRegKey(pDeviceInfoSet,
                                              DevInfoElem,
                                              Scope,
                                              HwProfile,
                                              KeyType,
                                              TRUE,
                                              KEY_ALL_ACCESS,
                                              &hk)) != NO_ERROR) {
            goto clean0;
        }

        //
        // We successfully created the storage key, now run an INF install
        // section against it (if specified).
        //
        if(InfHandle && (InfHandle != INVALID_HANDLE_VALUE) && InfSectionName) {
            //
            // If a copy msg handler and context haven't been specified, then use
            // the default one.
            //
            if(DevInfoElem->InstallParamBlock.InstallMsgHandler) {
                MsgHandler        = DevInfoElem->InstallParamBlock.InstallMsgHandler;
                MsgHandlerContext = DevInfoElem->InstallParamBlock.InstallMsgHandlerContext;
                MsgHandlerIsNativeCharWidth = DevInfoElem->InstallParamBlock.InstallMsgHandlerIsNativeCharWidth;
            } else {

                NoProgressUI = (GuiSetupInProgress || (DevInfoElem->InstallParamBlock.Flags & DI_QUIETINSTALL));

                if(!(MsgHandlerContext = SetupInitDefaultQueueCallbackEx( 
                                             DevInfoElem->InstallParamBlock.hwndParent, 
                                             (NoProgressUI ? INVALID_HANDLE_VALUE : NULL),
                                             0,
                                             0,
                                             NULL))) {

                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean0;
                }
                MsgHandler = SetupDefaultQueueCallback;
                MsgHandlerIsNativeCharWidth = TRUE;
            }

            if(!_SetupInstallFromInfSection(DevInfoElem->InstallParamBlock.hwndParent,
                                            InfHandle,
                                            InfSectionName,
                                            SPINST_ALL,
                                            hk,
                                            NULL,
                                            0,
                                            MsgHandler,
                                            MsgHandlerContext,
                                            ((KeyType == DIREG_DEV) ? DeviceInfoSet
                                                                    : INVALID_HANDLE_VALUE),
                                            ((KeyType == DIREG_DEV) ? DeviceInfoData
                                                                    : NULL),
                                            MsgHandlerIsNativeCharWidth,
                                            NULL
                                           )) {
                Err = GetLastError();
            }

            //
            // If we used the default msg handler, release the default context now.
            //
            if(!DevInfoElem->InstallParamBlock.InstallMsgHandler) {
                SetupTermDefaultQueueCallback(MsgHandlerContext);
            }
        }

clean0:
        ; // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    if(Err == NO_ERROR) {
        return hk;
    } else {
        if(hk != INVALID_HANDLE_VALUE) {
            RegCloseKey(hk);
        }
        SetLastError(Err);
        return INVALID_HANDLE_VALUE;
    }
}


HKEY
WINAPI
SetupDiOpenDevRegKey(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Scope,
    IN DWORD            HwProfile,
    IN DWORD            KeyType,
    IN REGSAM           samDesired
    )
/*++

Routine Description:

    This routine opens a registry storage key for device-specific configuration
    information, and returns a handle to the key.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        information about the device instance whose registry configuration storage
        key is to be opened.

    DeviceInfoData - Supplies a pointer to a SP_DEVINFO_DATA structure indicating
        the device instance to open the registry key for.

    Scope - Specifies the scope of the registry key to be opened.  This determines
        where the information is actually stored--the key opened may be one that is
        global (i.e., constant regardless of current hardware profile) or hardware
        profile-specific.  May be one of the following values:

        DICS_FLAG_GLOBAL - Open a key to store global configuration information.

        DICS_FLAG_CONFIGSPECIFIC - Open a key to store hardware profile-specific
                                   information.

    HwProfile - Specifies the hardware profile to open a key for, if the Scope parameter
        is set to DICS_FLAG_CONFIGSPECIFIC.  If this parameter is 0, then the key
        for the current hardware profile should be opened (i.e., in the Class branch
        under HKEY_CURRENT_CONFIG).  If Scope is SPDICS_FLAG_GLOBAL, then this parameter
        is ignored.

    KeyType - Specifies the type of registry storage key to be opened.  May be one of
        the following values:

        DIREG_DEV - Open a hardware registry key for the device.  This is the key for
            storage of driver-independent configuration information.  (This key is in
            the device instance key in the Enum branch.

        DIREG_DRV - Open a software (i.e., driver) registry key for the device.  (This key
            is located in the class branch.)

    samDesired - Specifies the access you require for this key.

Return Value:

    If the function succeeds, the return value is a handle to an opened registry
    key where private configuration data pertaining to this device instance may be
    stored/retrieved.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

Remarks:

    The handle returned from this routine must be closed by calling RegCloseKey.

    The specified device instance must have been previously registered (i.e., if it
    was created via SetupDiCreateDeviceInfo, then SetupDiRegisterDeviceInfo must have
    been subsequently called.)

--*/

{
    HKEY hk;
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }

    Err = NO_ERROR;

    try {
        //
        // Get a pointer to the element for the specified device
        // instance.
        //
        if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                   DeviceInfoData,
                                                   NULL)) {
            //
            // Open the requested registry storage key.
            //
            Err = pSetupOpenOrCreateDevRegKey(pDeviceInfoSet,
                                              DevInfoElem,
                                              Scope,
                                              HwProfile,
                                              KeyType,
                                              FALSE,
                                              samDesired,
                                              &hk
                                             );
        } else {
            Err = ERROR_INVALID_PARAMETER;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return (Err == NO_ERROR) ? hk : INVALID_HANDLE_VALUE;
}


DWORD
pSetupOpenOrCreateDevRegKey(
    IN  PDEVICE_INFO_SET pDeviceInfoSet,
    IN  PDEVINFO_ELEM    DevInfoElem,
    IN  DWORD            Scope,
    IN  DWORD            HwProfile,
    IN  DWORD            KeyType,
    IN  BOOL             Create,
    IN  REGSAM           samDesired,
    OUT PHKEY            hDevRegKey
    )
/*++

Routine Description:

    This routine creates or opens a registry storage key for the specified
    device information element, and returns a handle to the opened key.

Arguments:

    DeviceInfoSet - Supplies a pointer to the device information set containing
        the element for which a registry storage key is to be created/opened.

    DevInfoElem - Supplies a pointer to the device information element for
        which a registry storage key is to be created/opened.

    Scope - Specifies the scope of the registry key to be created/opened.  This determines
        where the information is actually stored--the key created may be one that is
        global (i.e., constant regardless of current hardware profile) or hardware
        profile-specific.  May be one of the following values:

        DICS_FLAG_GLOBAL - Create/open a key to store global configuration information.

        DICS_FLAG_CONFIGSPECIFIC - Create/open a key to store hardware profile-specific
                                   information.

    HwProfile - Specifies the hardware profile to create/open a key for, if the Scope parameter
        is set to DICS_FLAG_CONFIGSPECIFIC.  If this parameter is 0, then the key
        for the current hardware profile should be created/opened (i.e., in the Class branch
        under HKEY_CURRENT_CONFIG).  If Scope is SPDICS_FLAG_GLOBAL, then this parameter
        is ignored.

    KeyType - Specifies the type of registry storage key to be created/opened.  May be one of
        the following values:

        DIREG_DEV - Create/open a hardware registry key for the device.  This is the key for
            storage of driver-independent configuration information.  (This key is in
            the device instance key in the Enum branch.

        DIREG_DRV - Create/open a software, or driver, registry key for the device.  (This key
            is located in the class branch.)

    Create - Specifies whether the key should be created if doesn't already exist.

    samDesired - Specifies the access you require for this key.

    hDevRegKey - Supplies the address of a variable that receives a handle to the
        requested registry key.  (This variable will only be written to if the
        handle is successfully opened.)

Return Value:

    If the function is successful, the return value is NO_ERROR, otherwise, it is
    the ERROR_* code indicating the error that occurred.

Remarks:

    If a software key is requested (DIREG_DRV), and there isn't already a 'Driver'
    value entry, then one will be created.  This entry is of the form:

        <ClassGUID>\<instance>

    where <instance> is a base-10, 4-digit number that is unique within that class.

--*/

{
    ULONG RegistryBranch;
    CONFIGRET cr;
    DWORD Err, Disposition;
    HKEY hk, hkClass;
    TCHAR DriverKey[GUID_STRING_LEN + 5];   // Eg, {4d36e978-e325-11ce-bfc1-08002be10318}\0000
    ULONG DriverKeyLength;
    TCHAR EmptyString = TEXT('\0');

    //
    // Under Win95, the class key uses the class name instead of its GUID.  The maximum
    // length of a class name is less than the length of a GUID string, but put a check
    // here just to make sure that this assumption remains valid.
    //
#if MAX_CLASS_NAME_LEN > MAX_GUID_STRING_LEN
#error MAX_CLASS_NAME_LEN is larger than MAX_GUID_STRING_LEN--fix DriverKey!
#endif

    //
    // Figure out what flags to pass to CM_Open_DevInst_Key
    //
    switch(KeyType) {

        case DIREG_DEV :
            RegistryBranch = CM_REGISTRY_HARDWARE;
            break;

        case DIREG_DRV :
            //
            // This key may only be opened if the device instance has been registered.
            //
            if(!(DevInfoElem->DiElemFlags & DIE_IS_REGISTERED)) {
                return ERROR_DEVINFO_NOT_REGISTERED;
            }

            //
            // Retrieve the 'Driver' registry property which indicates where the
            // storage key is located in the class branch.
            //
            DriverKeyLength = sizeof(DriverKey);
            if((cr = CM_Get_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                      CM_DRP_DRIVER,
                                                      NULL,
                                                      DriverKey,
                                                      &DriverKeyLength,
                                                      0,
                                                      pDeviceInfoSet->hMachine)) != CR_SUCCESS) {

                if(cr != CR_NO_SUCH_VALUE) {
                    return (cr == CR_INVALID_DEVINST) ? ERROR_NO_SUCH_DEVINST
                                                      : ERROR_INVALID_DATA;
                } else if(!Create) {
                    return ERROR_KEY_DOES_NOT_EXIST;
                }

                //
                // The Driver entry doesn't exist, and we should create it.
                //
                hk = INVALID_HANDLE_VALUE;
                if(CM_Open_Class_Key_Ex(NULL,
                                     NULL,
                                     KEY_ALL_ACCESS,
                                     RegDisposition_OpenAlways,
                                     &hkClass,
                                     0,
                                     pDeviceInfoSet->hMachine) != CR_SUCCESS) {
                    //
                    // This shouldn't fail.
                    //
                    return ERROR_INVALID_DATA;
                }

                try {
                    //
                    // Find a unique key name under this class key.
                    //
                    DriverKeyLength = SIZECHARS(DriverKey);
                    if(CM_Get_Class_Key_Name_Ex(&(DevInfoElem->ClassGuid),
                                             DriverKey,
                                             &DriverKeyLength,
                                             0,
                                             pDeviceInfoSet->hMachine) != CR_SUCCESS) {

                        Err = ERROR_INVALID_CLASS;
                        goto clean0;
                    }
                    DriverKeyLength--;  // don't want to include terminating NULL.

                    while(pSetupFindUniqueKey(hkClass, DriverKey, DriverKeyLength)) {

                        if((Err = RegCreateKeyEx(hkClass,
                                                 DriverKey,
                                                 0,
                                                 &EmptyString,
                                                 REG_OPTION_NON_VOLATILE,
                                                 KEY_ALL_ACCESS,
                                                 NULL,
                                                 &hk,
                                                 &Disposition)) == ERROR_SUCCESS) {
                            //
                            // Everything's great, unless the Disposition indicates
                            // that the key already existed.  That means that someone
                            // else claimed the key before we got a chance to.  In
                            // that case, we close this key, and try again.
                            //
                            if(Disposition == REG_OPENED_EXISTING_KEY) {
                                RegCloseKey(hk);
                                hk = INVALID_HANDLE_VALUE;
                                //
                                // Truncate off the class instance part, to be replaced
                                // with a new instance number the next go-around.
                                //
                                DriverKey[GUID_STRING_LEN - 1] = TEXT('\0');
                            } else {
                                break;
                            }
                        } else {
                            hk = INVALID_HANDLE_VALUE;
                            break;
                        }
                    }

                    if(Err != NO_ERROR) {   // NO_ERROR == ERROR_SUCCESS
                        goto clean0;
                    }

                    //
                    // Set the device instance's 'Driver' registry property to reflect the
                    // new software registry storage location.
                    //
                    CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                     CM_DRP_DRIVER,
                                                     DriverKey,
                                                     sizeof(DriverKey),
                                                     0,
                                                     pDeviceInfoSet->hMachine);


clean0:             ;   // nothing to do

                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Err = ERROR_INVALID_PARAMETER;
                    //
                    // Access the hk variable so that the compiler will respect
                    // the statement ordering in the try clause.
                    //
                    hk = hk;
                }

                if(hk != INVALID_HANDLE_VALUE) {
                    RegCloseKey(hk);
                }

                RegCloseKey(hkClass);

                if(Err != NO_ERROR) {
                    return Err;
                }
            }

            RegistryBranch = CM_REGISTRY_SOFTWARE;
            break;

        default :
            return ERROR_INVALID_FLAGS;
    }

    if(Scope == DICS_FLAG_CONFIGSPECIFIC) {
        RegistryBranch |= CM_REGISTRY_CONFIG;
    } else if(Scope != DICS_FLAG_GLOBAL) {
        return ERROR_INVALID_FLAGS;
    }

    cr = CM_Open_DevInst_Key_Ex(DevInfoElem->DevInst,
                             samDesired,
                             HwProfile,
                             (Create ? RegDisposition_OpenAlways : RegDisposition_OpenExisting),
                             &hk,
                             RegistryBranch,
                             pDeviceInfoSet->hMachine);
    if(cr == CR_SUCCESS) {
        *hDevRegKey = hk;
        Err = NO_ERROR;
    } else {

        switch(cr) {

            case CR_INVALID_DEVINST :
                Err = ERROR_NO_SUCH_DEVINST;
                break;

            case CR_NO_SUCH_REGISTRY_KEY :
                Err = ERROR_KEY_DOES_NOT_EXIST;
                break;

            default :
                Err = ERROR_INVALID_DATA;
        }
    }

    return Err;
}


BOOL
WINAPI
_SetupDiGetDeviceRegistryProperty(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize         OPTIONAL
#ifdef UNICODE
    IN ,BOOL             Ansi
#endif
    )
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    CONFIGRET cr;
    ULONG CmRegProperty, PropLength;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {
        //
        // Get a pointer to the element for the specified device
        // instance.
        //
        if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        if(Property < SPDRP_MAXIMUM_PROPERTY) {
            CmRegProperty = SPDRP_TO_CMDRP(Property);
        } else {
            Err = ERROR_INVALID_REG_PROPERTY;
            goto clean0;
        }

        PropLength = PropertyBufferSize;
#ifdef UNICODE
        if(Ansi) {
            cr = CM_Get_DevInst_Registry_Property_ExA(
                    DevInfoElem->DevInst,
                    CmRegProperty,
                    PropertyRegDataType,
                    PropertyBuffer,
                    &PropLength,
                    0,
                    pDeviceInfoSet->hMachine);
        } else
#endif
        cr = CM_Get_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                              CmRegProperty,
                                              PropertyRegDataType,
                                              PropertyBuffer,
                                              &PropLength,
                                              0,
                                              pDeviceInfoSet->hMachine);

        if((cr == CR_SUCCESS) || (cr == CR_BUFFER_SMALL)) {

            if(RequiredSize) {
                *RequiredSize = PropLength;
            }
        }

        if(cr != CR_SUCCESS) {

            switch(cr) {

                case CR_INVALID_DEVINST :
                    Err = ERROR_NO_SUCH_DEVINST;
                    break;

                case CR_INVALID_PROPERTY :
                    Err = ERROR_INVALID_REG_PROPERTY;
                    break;

                case CR_BUFFER_SMALL :
                    Err = ERROR_INSUFFICIENT_BUFFER;
                    break;

                default :
                    Err = ERROR_INVALID_DATA;
            }
        }

clean0:
        ; // Nothing to do.

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
SetupDiGetDeviceRegistryPropertyA(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize         OPTIONAL
    )
{
    BOOL b;

    b = _SetupDiGetDeviceRegistryProperty(
            DeviceInfoSet,
            DeviceInfoData,
            Property,
            PropertyRegDataType,
            PropertyBuffer,
            PropertyBufferSize,
            RequiredSize,
            TRUE
            );

    return(b);
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupDiGetDeviceRegistryPropertyW(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize         OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    UNREFERENCED_PARAMETER(Property);
    UNREFERENCED_PARAMETER(PropertyRegDataType);
    UNREFERENCED_PARAMETER(PropertyBuffer);
    UNREFERENCED_PARAMETER(PropertyBufferSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiGetDeviceRegistryProperty(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize         OPTIONAL
    )
/*++

Routine Description:

    This routine retrieves the specified property from the Plug & Play device
    storage location in the registry.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        information about the device instance to retrieve a Plug & Play registry
        property for.

    DeviceInfoData - Supplies a pointer to a SP_DEVINFO_DATA structure indicating
        the device instance to retrieve the Plug & Play registry property for.

    Property - Supplies an ordinal specifying the property to be retrieved.  Refer
        to sdk\inc\setupapi.h for a complete list of properties that may be retrieved.

    PropertyRegDataType - Optionally, supplies the address of a variable that
        will receive the data type of the property being retrieved.  This will
        be one of the standard registry data types (REG_SZ, REG_BINARY, etc.)

    PropertyBuffer - Supplies the address of a buffer that receives the property
        data.

    PropertyBufferSize - Supplies the length, in bytes, of PropertyBuffer.

    RequiredSize - Optionally, supplies the address of a variable that receives
        the number of bytes required to store the requested property in the buffer.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.  If the supplied buffer was not large enough
    to hold the requested property, the error will be ERROR_INSUFFICIENT_BUFFER,
    and RequiredSize will specify how large the buffer needs to be.

--*/

{
    BOOL b;

    b = _SetupDiGetDeviceRegistryProperty(
            DeviceInfoSet,
            DeviceInfoData,
            Property,
            PropertyRegDataType,
            PropertyBuffer,
            PropertyBufferSize,
            RequiredSize
#ifdef UNICODE
           ,FALSE
#endif
            );

    return(b);
}



BOOL
WINAPI
_SetupDiSetDeviceRegistryProperty(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData,
    IN     DWORD            Property,
    IN     CONST BYTE*      PropertyBuffer,    OPTIONAL
    IN     DWORD            PropertyBufferSize
#ifdef UNICODE
    IN    ,BOOL             Ansi
#endif
    )
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    CONFIGRET cr;
    ULONG CmRegProperty;
    GUID ClassGuid;
    BOOL ClassGuidSpecified;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    DWORD ClassNameLength;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {
        //
        // Get a pointer to the element for the specified device
        // instance.
        //
        if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // Make sure the property code is in-range, and is not SPDRP_CLASS
        // (the Class property is not settable directly, and is automatically
        // updated when the ClassGUID property changes).
        //
        if((Property < SPDRP_MAXIMUM_PROPERTY) && (Property != SPDRP_CLASS)) {
            CmRegProperty = SPDRP_TO_CMDRP(Property);
        } else {
            Err = ERROR_INVALID_REG_PROPERTY;
            goto clean0;
        }

        //
        // If the property we're setting is ClassGUID, then we need to check to
        // see whether the new GUID is different from the current one.  If there's
        // no change, then we're done.
        //
        if(CmRegProperty == CM_DRP_CLASSGUID) {

            if(!PropertyBuffer) {
                //
                // Then the intent is to reset the device's class GUID.  Make
                // sure they passed us a buffer length of zero.
                //
                if(PropertyBufferSize) {
                    Err = ERROR_INVALID_PARAMETER;
                    goto clean0;
                }

                ClassGuidSpecified = FALSE;

            } else {

#ifdef UNICODE
                //
                // If we're being called from the ANSI API then we need
                // to convert the ANSI string representation of the GUID
                // to Unicode before we convert the string to an actual GUID.
                //
                PCWSTR UnicodeGuidString;

                if(Ansi) {
                    UnicodeGuidString = AnsiToUnicode((PCSTR)PropertyBuffer);
                    if(!UnicodeGuidString) {
                        Err = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean0;
                    }
                } else {
                    UnicodeGuidString = (PCWSTR)PropertyBuffer;
                }
                Err = pSetupGuidFromString(UnicodeGuidString,&ClassGuid);
                if(UnicodeGuidString != (PCWSTR)PropertyBuffer) {
                    MyFree(UnicodeGuidString);
                }
                if(Err != NO_ERROR) {
                    goto clean0;
                }
#else
                if((Err = pSetupGuidFromString((PCTSTR)PropertyBuffer, &ClassGuid)) != NO_ERROR) {
                    goto clean0;
                }
#endif
                ClassGuidSpecified = TRUE;
            }

            if(IsEqualGUID(&(DevInfoElem->ClassGuid),
                           (ClassGuidSpecified ? &ClassGuid
                                               : &GUID_NULL))) {
                //
                // No change--nothing to do.
                //
                goto clean0;
            }

            //
            // We're changing the class of this device.  First, make sure that the
            // set containing this device doesn't have an associated class (otherwise,
            // we'll suddenly have a device whose class doesn't match the set's class).
            //
            if(pDeviceInfoSet->HasClassGuid) {
                Err = ERROR_CLASS_MISMATCH;
            } else {
                Err = InvalidateHelperModules(DeviceInfoSet, DeviceInfoData, 0);
            }

            if(Err != NO_ERROR) {
                goto clean0;
            }

            //
            // Everything seems to be in order.  Before going any further, we need to
            // delete any software keys associated with this device, so we don't leave
            // orphans in the registry when we change the device's class.
            //
            pSetupDeleteDevRegKeys(DevInfoElem->DevInst,
                                   DICS_FLAG_GLOBAL | DICS_FLAG_CONFIGSPECIFIC,
                                   (DWORD)-1,
                                   DIREG_DRV,
                                   TRUE
                                  );
            //
            // Now delete the Driver property for this device...
            //
            CM_Set_DevInst_Registry_Property(DevInfoElem->DevInst,
                                             CM_DRP_DRIVER,
                                             NULL,
                                             0,
                                             0
                                            );
        }

#ifdef UNICODE
        if(Ansi) {
            cr = CM_Set_DevInst_Registry_PropertyA(
                    DevInfoElem->DevInst,
                    CmRegProperty,
                    (PVOID)PropertyBuffer,
                    PropertyBufferSize,
                    0
                    );
        } else
#endif
        cr = CM_Set_DevInst_Registry_Property(DevInfoElem->DevInst,
                                              CmRegProperty,
                                              (PVOID)PropertyBuffer,
                                              PropertyBufferSize,
                                              0
                                             );
        if(cr == CR_SUCCESS) {
            //
            // If we were setting the device's ClassGUID property, then we need to
            // update its Class name property as well.
            //
            if(CmRegProperty == CM_DRP_CLASSGUID) {

                if(ClassGuidSpecified) {

                    if(!SetupDiClassNameFromGuid(&ClassGuid,
                                                 ClassName,
                                                 SIZECHARS(ClassName),
                                                 &ClassNameLength)) {
                        //
                        // We couldn't retrieve the corresponding class name.
                        // Set ClassNameLength to zero so that we reset class
                        // name below.
                        //
                        ClassNameLength = 0;
                    }

                } else {
                    //
                    // Resetting ClassGUID--we want to reset class name also.
                    //
                    ClassNameLength = 0;
                }

                CM_Set_DevInst_Registry_Property(DevInfoElem->DevInst,
                                                 CM_DRP_CLASS,
                                                 ClassNameLength ? (PVOID)ClassName : NULL,
                                                 ClassNameLength * sizeof(TCHAR),
                                                 0
                                                );

                //
                // Finally, update the device's class GUID, and also update the
                // caller-supplied SP_DEVINFO_DATA structure to reflect the device's
                // new class.
                //
                CopyMemory(&(DevInfoElem->ClassGuid),
                           (ClassGuidSpecified ? &ClassGuid : &GUID_NULL),
                           sizeof(GUID)
                          );

                CopyMemory(&(DeviceInfoData->ClassGuid),
                           (ClassGuidSpecified ? &ClassGuid : &GUID_NULL),
                           sizeof(GUID)
                          );
            }

        } else {

            switch(cr) {

                case CR_INVALID_DEVINST :
                    Err = ERROR_NO_SUCH_DEVINST;
                    break;

                case CR_INVALID_PROPERTY :
                    Err = ERROR_INVALID_REG_PROPERTY;
                    break;

                case CR_INVALID_DATA :
                    Err = ERROR_INVALID_PARAMETER;
                    break;

                default :
                    Err = ERROR_INVALID_DATA;
            }
        }

clean0:
        ; // Nothing to do.

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
SetupDiSetDeviceRegistryPropertyA(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData,
    IN     DWORD            Property,
    IN     CONST BYTE*      PropertyBuffer,    OPTIONAL
    IN     DWORD            PropertyBufferSize
    )
{
    BOOL b;

    b = _SetupDiSetDeviceRegistryProperty(
            DeviceInfoSet,
            DeviceInfoData,
            Property,
            PropertyBuffer,
            PropertyBufferSize,
            TRUE
            );

    return(b);
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupDiSetDeviceRegistryPropertyW(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData,
    IN     DWORD            Property,
    IN     CONST BYTE*      PropertyBuffer,    OPTIONAL
    IN     DWORD            PropertyBufferSize
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    UNREFERENCED_PARAMETER(Property);
    UNREFERENCED_PARAMETER(PropertyBuffer);
    UNREFERENCED_PARAMETER(PropertyBufferSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiSetDeviceRegistryProperty(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData,
    IN     DWORD            Property,
    IN     CONST BYTE*      PropertyBuffer,    OPTIONAL
    IN     DWORD            PropertyBufferSize
    )

/*++

Routine Description:

    This routine sets the specified Plug & Play device registry property.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        information about the device instance to set a Plug & Play registry
        property for.

    DeviceInfoData - Supplies a pointer to a SP_DEVINFO_DATA structure indicating
        the device instance to set the Plug & Play registry property for.  If the
        ClassGUID property is being set, then this structure will be updated upon
        return to reflect the device's new class.

    Property - Supplies an ordinal specifying the property to be set.  Refer to
        sdk\inc\setupapi.h for a complete listing of values that may be set
        (these values are denoted with 'R/W' in their descriptive comment).

    PropertyBuffer - Supplies the address of a buffer containing the new data
        for the property.  If the property is being cleared, then this pointer
        should be NULL, and PropertyBufferSize must be zero.

    PropertyBufferSize - Supplies the length, in bytes, of PropertyBuffer.  If
        PropertyBuffer isn't specified (i.e., the property is to be cleared),
        then this value must be zero.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    Note that the Class property cannot be set.  This is because it is based on
    the corresponding ClassGUID, and is automatically updated when that property
    changes.

    Also, note that when the ClassGUID property changes, this routine automatically
    cleans up any software keys associated with the device.  Otherwise, we would 
    be left with orphaned registry keys.

--*/

{
    BOOL b;

    b = _SetupDiSetDeviceRegistryProperty(
            DeviceInfoSet,
            DeviceInfoData,
            Property,
            PropertyBuffer,
            PropertyBufferSize
#ifdef UNICODE
           ,FALSE
#endif
            );

    return(b);
}

DWORD
_SetupDiGetClassRegistryProperty(
    IN  LPGUID           ClassGuid,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize,        OPTIONAL
    IN  PCTSTR           MachineName,         OPTIONAL
    IN  BOOL             Ansi
    )
/*++

    See SetupDiGetClassRegistryProperty
    
--*/
{
    DWORD Err;
    CONFIGRET cr;
    ULONG CmRegProperty, PropLength;
    HMACHINE hMachine = NULL;
    Err = NO_ERROR;

#ifndef UNICODE
    UNREFERENCED_PARAMETER(Ansi);
#endif

    try {
        //
        // if we want to set register for another machine, find that machine
        //
        if(MachineName) {

            if(CR_SUCCESS != (cr = CM_Connect_Machine(MachineName, &hMachine))) {
                //
                // Make sure machine handle is still invalid, so we won't
                // try to disconnect later.
                //
                hMachine = NULL;
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                leave;
            }
        }

        if(Property < SPCRP_MAXIMUM_PROPERTY) {
            CmRegProperty = SPCRP_TO_CMCRP(Property);
        } else {
            Err = ERROR_INVALID_REG_PROPERTY;
            leave;
        }
    
        PropLength = PropertyBufferSize;
    #ifdef UNICODE
        if(Ansi) {
            cr = CM_Get_Class_Registry_PropertyA(
                    ClassGuid,
                    CmRegProperty,
                    PropertyRegDataType,
                    PropertyBuffer,
                    &PropLength,
                    0,
                    hMachine);
         } else {
             cr = CM_Get_Class_Registry_PropertyW(
                     ClassGuid,
                     CmRegProperty,
                     PropertyRegDataType,
                     PropertyBuffer,
                     &PropLength,
                     0,
                     hMachine);
         }
    #else
        //
        // on ANSI version
        //
        cr = CM_Get_Class_Registry_Property(
                ClassGuid,
                CmRegProperty,
                PropertyRegDataType,
                PropertyBuffer,
                &PropLength,
                0,
                hMachine);
    #endif

        if((cr == CR_SUCCESS) || (cr == CR_BUFFER_SMALL)) {
    
            if(RequiredSize) {
                *RequiredSize = PropLength;
            }
        }
    
        if(cr != CR_SUCCESS) {
    
            switch(cr) {
    
                case CR_INVALID_DEVINST :
                    Err = ERROR_NO_SUCH_DEVINST;
                    break;
    
                case CR_INVALID_PROPERTY :
                    Err = ERROR_INVALID_REG_PROPERTY;
                    break;
    
                case CR_BUFFER_SMALL :
                    Err = ERROR_INSUFFICIENT_BUFFER;
                    break;
    
                default :
                    Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
            }
        }
    
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }
    
    if (hMachine != NULL) {
        CM_Disconnect_Machine(hMachine);        
    }

    return Err;
}

#ifdef UNICODE
//
// ANSI version
//
WINSETUPAPI
BOOL
WINAPI
SetupDiGetClassRegistryPropertyA(
    IN  LPGUID           ClassGuid,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize,        OPTIONAL
    IN  PCSTR            MachineName,         OPTIONAL
    IN  PVOID            Reserved
    )
/*++

    See SetupDiGetClassRegistryProperty
    
--*/
{
    PCWSTR MachineString = NULL;
    DWORD Err = NO_ERROR;

    if (Reserved != NULL) {
        //
        // make sure caller doesn't pass a value here
        // so we know we can use this at a later date
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    try {
        //
        // convert machine-name to local
        //
    
        if (MachineName != NULL) {
            MachineString = AnsiToUnicode(MachineName);
            if(!MachineString) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                leave;
            }
        }
        Err = _SetupDiGetClassRegistryProperty(ClassGuid,
                                                Property,
                                                PropertyRegDataType,
                                                PropertyBuffer,
                                                PropertyBufferSize,
                                                RequiredSize,
                                                MachineString,
                                                TRUE);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = NO_ERROR;
    }
    if (MachineString != NULL) {
        MyFree(MachineString);
    }
    SetLastError(Err);
    return (BOOL)(Err == NO_ERROR);
}

#else
//
// UNICODE stub
//
WINSETUPAPI
BOOL
WINAPI
SetupDiGetClassRegistryPropertyW(
    IN  LPGUID           ClassGuid,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize,        OPTIONAL
    IN  PCWSTR           MachineName,         OPTIONAL
    IN  PVOID            Reserved
    )
/*++

    See SetupDiGetClassRegistryProperty
    
--*/
{
    UNREFERENCED_PARAMETER(ClassGuid);
    UNREFERENCED_PARAMETER(Property);
    UNREFERENCED_PARAMETER(PropertyRegDataType);
    UNREFERENCED_PARAMETER(PropertyBuffer);
    UNREFERENCED_PARAMETER(PropertyBufferSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    UNREFERENCED_PARAMETER(MachineName);
    UNREFERENCED_PARAMETER(Reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

#endif // UNICODE

WINSETUPAPI
BOOL
WINAPI
SetupDiGetClassRegistryProperty(
    IN  LPGUID           ClassGuid,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize,        OPTIONAL
    IN  PCTSTR           MachineName,         OPTIONAL
    IN  PVOID            Reserved
    )
/*++

Routine Description:

    This routine gets the specified Plug & Play device class registry property.
    This is just a wrapper around the Config Mgr API
    Typically the properties here can be overridden on a per-device basis,
    however this routine returns the class properties only.

Arguments:

    ClassGuid - Supplies the class Guid that the property is to be got from

    Property - Supplies an ordinal specifying the property to be set.  Refer to
        sdk\inc\setupapi.h for a complete listing of values that may be set
        (these values are denoted with 'R/W' in their descriptive comment).

    PropertyBuffer - Supplies the address of a buffer containing the new data
        for the property.  If the property is being cleared, then this pointer
        should be NULL, and PropertyBufferSize must be zero.

    PropertyBufferSize - Supplies the length, in bytes, of PropertyBuffer.  If
        PropertyBuffer isn't specified (i.e., the property is to be cleared),
        then this value must be zero.

    MachineName - Allows properties to be set on a remote machine (if Non-NULL)
    
    Reserved - should be nULL

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    DWORD Err = NO_ERROR;

    if (Reserved != NULL) {
        //
        // make sure caller doesn't pass a value here
        // so we know we can use this at a later date
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Err = _SetupDiGetClassRegistryProperty(ClassGuid,
                                            Property,
                                            PropertyRegDataType,
                                            PropertyBuffer,
                                            PropertyBufferSize,
                                            RequiredSize,
                                            MachineName,
                                            FALSE);

    SetLastError(Err);
    return (BOOL)(Err == NO_ERROR);
}


DWORD
_SetupDiSetClassRegistryProperty(
    IN  LPGUID           ClassGuid,
    IN  DWORD            Property,
    IN  CONST BYTE*      PropertyBuffer,      OPTIONAL
    IN  DWORD            PropertyBufferSize,
    IN  PCTSTR           MachineName,         OPTIONAL
    IN  BOOL             Ansi
    )
/*++

    See SetupDiGetClassRegistryProperty
    
--*/
{
    DWORD Err;
    CONFIGRET cr;
    ULONG CmRegProperty, PropLength;
    HMACHINE hMachine = NULL;

    Err = NO_ERROR;

    try {
        //
        // if we want to set register for another machine, find that machine
        //
        if(MachineName) {

            if(CR_SUCCESS != (cr = CM_Connect_Machine(MachineName, &hMachine))) {
                //
                // Make sure machine handle is still invalid, so we won't
                // try to disconnect later.
                //
                hMachine = NULL;
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                leave;
            }
        }

        if(Property < SPCRP_MAXIMUM_PROPERTY) {
            CmRegProperty = SPCRP_TO_CMCRP(Property);
        } else {
            Err = ERROR_INVALID_REG_PROPERTY;
            leave;
        }
    
        PropLength = PropertyBufferSize;

    #ifdef UNICODE
        if(Ansi) {
            cr = CM_Set_Class_Registry_PropertyA(
                    ClassGuid,
                    CmRegProperty,
                    PropertyBuffer,
                    PropLength,
                    0,
                    hMachine);
         } else {
             cr = CM_Set_Class_Registry_PropertyW(
                     ClassGuid,
                     CmRegProperty,
                     PropertyBuffer,
                     PropLength,
                     0,
                     hMachine);
         }
    #else
        //
        // on ANSI version
        //
        cr = CM_Set_Class_Registry_Property(
                ClassGuid,
                CmRegProperty,
                PropertyBuffer,
                PropLength,
                0,
                hMachine);
    #endif

        if(cr != CR_SUCCESS) {
    
            switch(cr) {
    
                case CR_INVALID_PROPERTY :
                    Err = ERROR_INVALID_REG_PROPERTY;
                    break;
    
                default :
                    Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
            }
        }
    
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }
    
    if (hMachine != NULL) {
        CM_Disconnect_Machine(hMachine);        
    }

    return Err;
}

#ifdef UNICODE
//
// ANSI version
//
WINSETUPAPI
BOOL
WINAPI
SetupDiSetClassRegistryPropertyA(
    IN     LPGUID           ClassGuid,
    IN     DWORD            Property,
    IN     CONST BYTE*      PropertyBuffer,    OPTIONAL
    IN     DWORD            PropertyBufferSize,
    IN     PCSTR            MachineName,       OPTIONAL
    IN     PVOID            Reserved
    )
/*++

    See SetupDiSetClassRegistryProperty
    
--*/
{
    PCWSTR MachineString = NULL;
    DWORD Err = NO_ERROR;

    if (Reserved != NULL) {
        //
        // make sure caller doesn't pass a value here
        // so we know we can use this at a later date
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    try {
        //
        // convert machine-name to local
        //
    
        if (MachineName != NULL) {
            MachineString = AnsiToUnicode(MachineName);
            if(!MachineString) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                leave;
            }
        }
        Err = _SetupDiSetClassRegistryProperty(ClassGuid,
                                                Property,
                                                PropertyBuffer,
                                                PropertyBufferSize,
                                                MachineString,
                                                TRUE);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = NO_ERROR;
    }
    if (MachineString != NULL) {
        MyFree(MachineString);
    }
    SetLastError(Err);
    return (BOOL)(Err == NO_ERROR);
}

#else
//
// UNICODE stub
//
WINSETUPAPI
BOOL
WINAPI
SetupDiSetClassRegistryPropertyW(
    IN     LPGUID           ClassGuid,
    IN     DWORD            Property,
    IN     CONST BYTE*      PropertyBuffer,    OPTIONAL
    IN     DWORD            PropertyBufferSize,
    IN     PCWSTR           MachineName,       OPTIONAL
    IN     PVOID            Reserved
    )
/*++

    See SetupDiSetClassRegistryProperty
    
--*/
{
    UNREFERENCED_PARAMETER(ClassGuid);
    UNREFERENCED_PARAMETER(Property);
    UNREFERENCED_PARAMETER(PropertyBuffer);
    UNREFERENCED_PARAMETER(PropertyBufferSize);
    UNREFERENCED_PARAMETER(MachineName);
    UNREFERENCED_PARAMETER(Reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

#endif // UNICODE


WINSETUPAPI
BOOL
WINAPI
SetupDiSetClassRegistryProperty(
    IN     LPGUID           ClassGuid,
    IN     DWORD            Property,
    IN     CONST BYTE*      PropertyBuffer,    OPTIONAL
    IN     DWORD            PropertyBufferSize,
    IN     PCTSTR           MachineName,       OPTIONAL
    IN     PVOID            Reserved
    )
/*++

Routine Description:

    This routine sets the specified Plug & Play device class registry property.
    This is just a wrapper around the Config Mgr API
    Typically the properties here can be overridden on a per-device basis

Arguments:

    ClassGuid - Supplies the class Guid for the P&P device, that the property is to
        be set for

    Property - Supplies an ordinal specifying the property to be retrieved.  Refer
        to sdk\inc\setupapi.h for a complete list of properties that may be retrieved.

    PropertyRegDataType - Optionally, supplies the address of a variable that
        will receive the data type of the property being retrieved.  This will
        be one of the standard registry data types (REG_SZ, REG_BINARY, etc.)

    PropertyBuffer - Supplies the address of a buffer that receives the property
        data.

    PropertyBufferSize - Supplies the length, in bytes, of PropertyBuffer.

    RequiredSize - Optionally, supplies the address of a variable that receives
        the number of bytes required to store the requested property in the buffer.

    MachineName - Allows properties to be got from a remote machine (if Non-NULL)
    
    Reserved - should be nULL

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    DWORD Err;

    Err = _SetupDiSetClassRegistryProperty(ClassGuid,
                                            Property,
                                            PropertyBuffer,
                                            PropertyBufferSize,
                                            MachineName,
                                            FALSE);
    SetLastError(Err);
    return (BOOL)(Err == NO_ERROR);
}




BOOL
pSetupFindUniqueKey(
    IN HKEY   hkRoot,
    IN LPTSTR SubKey,
    IN ULONG  SubKeyLength
    )
/*++

Routine Description:

    This routine finds a unique key under the specified subkey.  This key is
    of the form <SubKey>\xxxx, where xxxx is a base-10, 4-digit number.

Arguments:

    hkRoot - Root key under which the specified SubKey is located.

    SubKey - Name of the subkey, under which a unique key is to be generated.

    SubKeyLength - Supplies the length of the SubKey string, not including
        terminating NULL.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.

--*/
{
    INT  i;
    HKEY hk;

    for(i = 0; i <= 9999; i++) {
        wsprintf(&(SubKey[SubKeyLength]), pszUniqueSubKey, i);
        if(RegOpenKeyEx(hkRoot, SubKey, 0, KEY_READ, &hk) != ERROR_SUCCESS) {
            return TRUE;
        }
        RegCloseKey(hk);
    }
    return FALSE;
}


BOOL
WINAPI
SetupDiDeleteDevRegKey(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Scope,
    IN DWORD            HwProfile,
    IN DWORD            KeyType
    )
/*++

Routine Description:

    This routine deletes the specified registry key(s) associated with a device
    information element.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device instance to delete key(s) for.

    DeviceInfoData - Supplies a pointer to a SP_DEVINFO_DATA structure indicating
        the device instance to delete key(s) for.

    Scope - Specifies the scope of the registry key to be deleted.  This determines
        where the key to be deleted is located--the key may be one that is global
        (i.e., constant regardless of current hardware profile) or hardware
        profile-specific.  May be a combination of the following values:

        DICS_FLAG_GLOBAL - Delete the key that stores global configuration information.

        DICS_FLAG_CONFIGSPECIFIC - Delete the key that stores hardware profile-specific
                                   information.

    HwProfile - Specifies the hardware profile to delete a key for, if the Scope parameter
        includes the DICS_FLAG_CONFIGSPECIFIC flag.  If this parameter is 0, then the key
        for the current hardware profile should be deleted (i.e., in the Class branch
        under HKEY_CURRENT_CONFIG).  If this parameter is 0xFFFFFFFF, then the key for
        _all_ hardware profiles should be deleted.

    KeyType - Specifies the type of registry storage key to be deleted.  May be one of
        the following values:

        DIREG_DEV - Delete the hardware registry key for the device.  This is the key for
            storage of driver-independent configuration information.  (This key is in
            the device instance key in the Enum branch.

        DIREG_DRV - Delete the software (i.e., driver) registry key for the device.  (This key
            is located in the class branch.)

        DIREG_BOTH - Delete both the hardware and software keys for the device.

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

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    try {
        //
        // Get a pointer to the element for the specified device
        // instance.
        //
        if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                   DeviceInfoData,
                                                   NULL)) {

            Err = pSetupDeleteDevRegKeys(DevInfoElem->DevInst, Scope, HwProfile, KeyType, FALSE);

        } else {
            Err = ERROR_INVALID_PARAMETER;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


DWORD
pSetupDeleteDevRegKeys(
    IN DEVINST DevInst,
    IN DWORD   Scope,
    IN DWORD   HwProfile,
    IN DWORD   KeyType,
    IN BOOL    DeleteUserKeys
    )
/*++

Routine Description:

    This is the worker routine for SetupDiDeleteDevRegKey.  See the discussion of
    that API for details.

Return Value:

    If successful, the return value is NO_ERROR;

    If failure, the return value is a Win32 error code indicating the cause of failure.

Remarks:

    Even if one of the operations in this routine fails, all operations will be attempted.
    Thus, as many keys as possible will be deleted.  The error returned will be the first
    error that was encountered in this case.

--*/
{
    CONFIGRET cr, crTemp;
    DWORD Err;

    cr = CR_SUCCESS;

    if(Scope & DICS_FLAG_GLOBAL) {

        if((KeyType == DIREG_DEV) || (KeyType == DIREG_BOTH)) {
            crTemp = CM_Delete_DevInst_Key(DevInst, 0, CM_REGISTRY_HARDWARE);
            if((cr == CR_SUCCESS) && (crTemp != CR_SUCCESS) && (crTemp != CR_NO_SUCH_REGISTRY_KEY)) {
                cr = crTemp;
            }
        }

        if((KeyType == DIREG_DRV) || (KeyType == DIREG_BOTH)) {
            crTemp = CM_Delete_DevInst_Key(DevInst, 0, CM_REGISTRY_SOFTWARE);
            if((cr == CR_SUCCESS) && (crTemp != CR_SUCCESS) && (crTemp != CR_NO_SUCH_REGISTRY_KEY)) {
                cr = crTemp;
            }
        }
    }

    if(Scope & DICS_FLAG_CONFIGSPECIFIC) {

        if((KeyType == DIREG_DEV) || (KeyType == DIREG_BOTH)) {
            crTemp = CM_Delete_DevInst_Key(DevInst, HwProfile, CM_REGISTRY_HARDWARE | CM_REGISTRY_CONFIG);
            if((cr == CR_SUCCESS) && (crTemp != CR_SUCCESS) && (crTemp != CR_NO_SUCH_REGISTRY_KEY)) {
                cr = crTemp;
            }
        }

        if((KeyType == DIREG_DRV) || (KeyType == DIREG_BOTH)) {
            crTemp = CM_Delete_DevInst_Key(DevInst, HwProfile, CM_REGISTRY_SOFTWARE | CM_REGISTRY_CONFIG);
            if((cr == CR_SUCCESS) && (crTemp != CR_SUCCESS) && (crTemp != CR_NO_SUCH_REGISTRY_KEY)) {
                cr = crTemp;
            }
        }
    }

    if(DeleteUserKeys) {

        if((KeyType == DIREG_DEV) || (KeyType == DIREG_BOTH)) {
            crTemp = CM_Delete_DevInst_Key(DevInst, 0, CM_REGISTRY_HARDWARE | CM_REGISTRY_USER);
            if((cr == CR_SUCCESS) && (crTemp != CR_SUCCESS) && (crTemp != CR_NO_SUCH_REGISTRY_KEY)) {
                cr = crTemp;
            }
        }

        if((KeyType == DIREG_DRV) || (KeyType == DIREG_BOTH)) {
            crTemp = CM_Delete_DevInst_Key(DevInst, 0, CM_REGISTRY_SOFTWARE | CM_REGISTRY_USER);
            if((cr == CR_SUCCESS) && (crTemp != CR_SUCCESS) && (crTemp != CR_NO_SUCH_REGISTRY_KEY)) {
                cr = crTemp;
            }
        }
    }

    //
    // Now translate the ConfigMgr return code into a Win32 one.
    //
    switch(cr) {

        case CR_SUCCESS :
            Err = NO_ERROR;
            break;

        case CR_REGISTRY_ERROR :
            Err = ERROR_ACCESS_DENIED;
            break;

        case CR_INVALID_DEVINST :
            Err = ERROR_NO_SUCH_DEVINST;
            break;

        default :
            Err = ERROR_INVALID_DATA;
    }

    return Err;
}


#ifdef UNICODE
//
// ANSI version
//
HKEY
WINAPI
SetupDiCreateDeviceInterfaceRegKeyA(
    IN HDEVINFO                  DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN DWORD                     Reserved,
    IN REGSAM                    samDesired,
    IN HINF                      InfHandle,           OPTIONAL
    IN PCSTR                     InfSectionName       OPTIONAL
    )
{
    DWORD rc;
    PWSTR name;
    HKEY h;

    if(InfSectionName) {
        rc = CaptureAndConvertAnsiArg(InfSectionName, &name);
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return(INVALID_HANDLE_VALUE);
        }
    } else {
        name = NULL;
    }

    h = SetupDiCreateDeviceInterfaceRegKeyW(DeviceInfoSet,
                                            DeviceInterfaceData,
                                            Reserved,
                                            samDesired,
                                            InfHandle,
                                            name
                                           );

    rc = GetLastError();

    if(name) {
        MyFree(name);
    }
    SetLastError(rc);
    return(h);
}
#else
//
// Unicode stub
//
HKEY
WINAPI
SetupDiCreateDeviceInterfaceRegKeyW(
    IN HDEVINFO                  DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN DWORD                     Reserved,
    IN REGSAM                    samDesired,
    IN HINF                      InfHandle,           OPTIONAL
    IN PCWSTR                    InfSectionName       OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInterfaceData);
    UNREFERENCED_PARAMETER(Reserved);
    UNREFERENCED_PARAMETER(samDesired);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(InfSectionName);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(INVALID_HANDLE_VALUE);
}
#endif

HKEY
WINAPI
SetupDiCreateDeviceInterfaceRegKey(
    IN HDEVINFO                  DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN DWORD                     Reserved,
    IN REGSAM                    samDesired,
    IN HINF                      InfHandle,           OPTIONAL
    IN PCTSTR                    InfSectionName       OPTIONAL
    )
/*++

Routine Description:

    This routine creates a registry storage key for a particular device interface,
    and returns a handle to the key.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device interface for whom a registry key is to be created.

    DeviceInterfaceData - Supplies a pointer to a device interface data structure
        indicating which device interface a key is to be created for.

    Reserved - Reserved for future use, must be set to 0.

    samDesired - Specifies the registry access desired for the resulting key handle.

    InfHandle - Optionally, supplies the handle of an opened INF file containing an
        install section to be executed for the newly-created key.  If this parameter is
        specified, then InfSectionName must be specified as well.

    InfSectionName - Optionally, supplies the name of an install section in the INF
        file specified by InfHandle.  This section will be executed for the newly
        created key. If this parameter is specified, then InfHandle must be specified
        as well.

Return Value:

    If the function succeeds, the return value is a handle to a newly-created 
    registry key where private configuration data pertaining to this device 
    interface may be stored/retrieved.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

Remarks:

    The handle returned from this routine must be closed by calling RegCloseKey.

    During GUI-mode setup on Windows NT, quiet-install behavior is always 
    employed in the absence of a user-supplied file queue, regardless of 
    whether the device information element has the DI_QUIETINSTALL flag set.

--*/

{
    HKEY hk, hSubKey;
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    PSP_FILE_CALLBACK MsgHandler;
    PVOID MsgHandlerContext;
    BOOL MsgHandlerIsNativeCharWidth;
    BOOL NoProgressUI;

    if(Reserved != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }

    Err = NO_ERROR;
    hk = hSubKey = INVALID_HANDLE_VALUE;

    try {
        //
        // Get a pointer to the device information element for the specified
        // interface device.
        //
        if(!(DevInfoElem = FindDevInfoElemForInterfaceDevice(pDeviceInfoSet, DeviceInterfaceData))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        hk = SetupDiOpenClassRegKeyEx(&(DeviceInterfaceData->InterfaceClassGuid),
                                      KEY_READ,
                                      DIOCR_INTERFACE,
                                      NULL,
                                      NULL
                                     );

        if(hk == INVALID_HANDLE_VALUE) {
            //
            // We couldn't open the interface class subkey--this should never happen.
            //
            Err = GetLastError();
            goto clean0;
        }

        //
        // Now, create the client-accessible registry storage key for this interface device.
        //
        Err = pSetupOpenOrCreateInterfaceDeviceRegKey(hk,
                                                      pDeviceInfoSet,
                                                      DeviceInterfaceData,
                                                      TRUE,
                                                      samDesired,
                                                      &hSubKey
                                                     );

        RegCloseKey(hk);
        hk = INVALID_HANDLE_VALUE;

        if(Err != NO_ERROR) {
            goto clean0;
        }

        //
        // We successfully created the storage key, now run an INF install
        // section against it (if specified).
        //
        if(InfHandle && (InfHandle != INVALID_HANDLE_VALUE) && InfSectionName) {
            //
            // If a copy msg handler and context haven't been specified, then 
            // use the default one.
            //
            if(DevInfoElem->InstallParamBlock.InstallMsgHandler) {
                MsgHandler        = DevInfoElem->InstallParamBlock.InstallMsgHandler;
                MsgHandlerContext = DevInfoElem->InstallParamBlock.InstallMsgHandlerContext;
                MsgHandlerIsNativeCharWidth = DevInfoElem->InstallParamBlock.InstallMsgHandlerIsNativeCharWidth;
            } else {

                NoProgressUI = (GuiSetupInProgress || (DevInfoElem->InstallParamBlock.Flags & DI_QUIETINSTALL));

                if(!(MsgHandlerContext = SetupInitDefaultQueueCallbackEx( 
                                             DevInfoElem->InstallParamBlock.hwndParent, 
                                             (NoProgressUI ? INVALID_HANDLE_VALUE : NULL),
                                             0,
                                             0,
                                             NULL))) {

                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean0;
                }
                MsgHandler = SetupDefaultQueueCallback;
                MsgHandlerIsNativeCharWidth = TRUE;
            }

            if(!_SetupInstallFromInfSection(DevInfoElem->InstallParamBlock.hwndParent,
                                            InfHandle,
                                            InfSectionName,
                                            SPINST_ALL ^ SPINST_LOGCONFIG,
                                            hSubKey,
                                            NULL,
                                            0,
                                            MsgHandler,
                                            MsgHandlerContext,
                                            INVALID_HANDLE_VALUE,
                                            NULL,
                                            MsgHandlerIsNativeCharWidth,
                                            NULL
                                            )) {
                Err = GetLastError();
            }

            //
            // If we used the default msg handler, release the default context 
            // now.
            //
            if(!DevInfoElem->InstallParamBlock.InstallMsgHandler) {
                SetupTermDefaultQueueCallback(MsgHandlerContext);
            }
        }

clean0:
        ; // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;

        if(hk != INVALID_HANDLE_VALUE) {
            RegCloseKey(hk);
        }

        //
        // Access the following registry handle so that the compiler will 
        // respect statement ordering w.r.t. its assignment.
        //
        hSubKey = hSubKey;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    if(Err == NO_ERROR) {
        return hSubKey;
    } else {
        if(hSubKey != INVALID_HANDLE_VALUE) {
            RegCloseKey(hSubKey);
        }
        SetLastError(Err);
        return INVALID_HANDLE_VALUE;
    }
}


HKEY
WINAPI
SetupDiOpenDeviceInterfaceRegKey(
    IN HDEVINFO                  DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN DWORD                     Reserved,
    IN REGSAM                    samDesired
    )
/*++

Routine Description:

    This routine opens a registry storage key for a particular device interface,
    and returns a handle to the key.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device interface for whom a registry key is to be opened.

    InterfaceDeviceData - Supplies a pointer to a device interface data structure
        indicating which device interface a key is to be opened for.

    Reserved - Reserved for future use, must be set to 0.

    samDesired - Specifies the access you require for this key.

Return Value:

    If the function succeeds, the return value is a handle to an opened registry
    key where private configuration data pertaining to this device interface may be
    stored/retrieved.

    If the function fails, the return value is INVALID_HANDLE_VALUE.  To get
    extended error information, call GetLastError.

Remarks:

    The handle returned from this routine must be closed by calling RegCloseKey.

--*/

{
    HKEY hk, hSubKey;
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;

    if(Reserved != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }

    Err = NO_ERROR;
    hk = INVALID_HANDLE_VALUE;

    try {
        //
        // Get a pointer to the device information element for the specified
        // interface device.
        //
        if(!(DevInfoElem = FindDevInfoElemForInterfaceDevice(pDeviceInfoSet, DeviceInterfaceData))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        hk = SetupDiOpenClassRegKeyEx(&(DeviceInterfaceData->InterfaceClassGuid),
                                      KEY_READ,
                                      DIOCR_INTERFACE,
                                      NULL,
                                      NULL
                                     );

        if(hk == INVALID_HANDLE_VALUE) {
            //
            // We couldn't open the interface class subkey--this should never happen.
            //
            Err = GetLastError();
            goto clean0;
        }

        //
        // Now, open up the client-accessible registry storage key for this interface device.
        //
        Err = pSetupOpenOrCreateInterfaceDeviceRegKey(hk,
                                                      pDeviceInfoSet,
                                                      DeviceInterfaceData,
                                                      FALSE,
                                                      samDesired,
                                                      &hSubKey
                                                     );

clean0:
        ; // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Access the following registry handle so that the compiler will respect
        // statement ordering w.r.t. its assignment.
        //
        hk = hk;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    if(hk != INVALID_HANDLE_VALUE) {
        RegCloseKey(hk);
    }

    SetLastError(Err);
    return (Err == NO_ERROR) ? hSubKey : INVALID_HANDLE_VALUE;
}


BOOL
WINAPI
SetupDiDeleteDeviceInterfaceRegKey(
    IN HDEVINFO                  DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN DWORD                     Reserved
    )
/*++

Routine Description:

    This routine deletes the registry key associated with a device interface.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device interface whose registry key is to be deleted.

    DeviceInterfaceData - Supplies a pointer to a device interface data structure
        indicating which device interface is to have its registry key deleted.

    Reserved - Reserved for future use, must be set to 0.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    HKEY hk;
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;

    if(Reserved != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    hk = INVALID_HANDLE_VALUE;

    try {
        //
        // Get a pointer to the device information element for the specified
        // interface device.
        //
        if(!(DevInfoElem = FindDevInfoElemForInterfaceDevice(pDeviceInfoSet, DeviceInterfaceData))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        hk = SetupDiOpenClassRegKeyEx(&(DeviceInterfaceData->InterfaceClassGuid),
                                      KEY_READ,
                                      DIOCR_INTERFACE,
                                      NULL,
                                      NULL
                                     );

        if(hk == INVALID_HANDLE_VALUE) {
            //
            // We couldn't open the interface class subkey--this should never happen.
            //
            Err = GetLastError();
            goto clean0;
        }

        //
        // Now delete the interface device key.
        //
        Err = pSetupDeleteInterfaceDeviceKey(hk, pDeviceInfoSet, DeviceInterfaceData);

clean0:
        ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Access the following registry handle so the compiler will respect statement
        // ordering w.r.t. its assignment.
        //
        hk = hk;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    if(hk != INVALID_HANDLE_VALUE) {
        RegCloseKey(hk);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}


DWORD
pSetupOpenOrCreateInterfaceDeviceRegKey(
    IN  HKEY                      hInterfaceClassKey,
    IN  PDEVICE_INFO_SET          DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA InterfaceDeviceData,
    IN  BOOL                      Create,
    IN  REGSAM                    samDesired,
    OUT PHKEY                     hInterfaceDeviceKey
    )
/*++

Routine Description:

    This routine creates or opens a registry storage key for the specified
    interface device, and returns a handle to the opened key.

Arguments:

    hInterfaceClassKey - Supplies a handle to the opened driver key, underneath which
        resides the interface device key to be deleted.

    DeviceInfoSet - Supplies a pointer to the device information set containing
        the interface device for which a registry storage key is to be created/opened.

    InterfaceDeviceData - Supplies a pointer to an interface device data structure
        indicating which interface device a key is to be opened/created for.

    Create - Specifies whether the key should be created if doesn't already exist.

    samDesired - Specifies the access you require for this key.

    hInterfaceDeviceKey - Supplies the address of a variable that receives a handle
        to the requested registry key.  (This variable will only be written to if the
        handle is successfully opened.)

Return Value:

    If the function is successful, the return value is NO_ERROR, otherwise, it is
    the ERROR_* code indicating the error that occurred.

Remarks:

    The algorithm used to form the storage keys for an interface device must be kept
    in sync with the kernel mode implementation of IoOpenDeviceClassRegistryKey.

--*/
{
    DWORD Err;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode;
    LPGUID ClassGuid;
    HKEY hInterfaceDeviceRootKey, hSubKey;
    DWORD Disposition;
    PCTSTR DevicePath;

    Err = NO_ERROR;
    hInterfaceDeviceRootKey = INVALID_HANDLE_VALUE;
    try {
        //
        // Get the interface device node, and verify that its class matches what the
        // caller passed us.
        //
        InterfaceDeviceNode = (PINTERFACE_DEVICE_NODE)(InterfaceDeviceData->Reserved);
        ClassGuid = &(DeviceInfoSet->GuidTable[InterfaceDeviceNode->GuidIndex]);

        if(!IsEqualGUID(ClassGuid, &(InterfaceDeviceData->InterfaceClassGuid))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // Verify that this interface device hasn't been removed.
        //
        if(InterfaceDeviceNode->Flags & SPINT_REMOVED) {
            Err = ERROR_DEVICE_INTERFACE_REMOVED;
            goto clean0;
        }

        //
        // OK, now open the interface device's root storage key.
        //
        DevicePath = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                              InterfaceDeviceNode->SymLinkName
                                             );

        if(ERROR_SUCCESS != OpenDeviceInterfaceSubKey(hInterfaceClassKey,
                                                      DevicePath,
                                                      KEY_READ,
                                                      &hInterfaceDeviceRootKey,
                                                      NULL,
                                                      NULL)) {
            //
            // Make sure hInterfaceDeviceRootKey is still INVALID_HANDLE_VALUE, so we
            // won't try to close it later.
            //
            hInterfaceDeviceRootKey = INVALID_HANDLE_VALUE;
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        if(Create) {

            Err = RegCreateKeyEx(hInterfaceDeviceRootKey,
                                 pszDeviceParameters,
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 samDesired,
                                 NULL,
                                 &hSubKey,
                                 &Disposition
                                );
        } else {

            Err = RegOpenKeyEx(hInterfaceDeviceRootKey,
                               pszDeviceParameters,
                               0,
                               samDesired,
                               &hSubKey
                              );
        }

clean0:
        ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Access the following variable so the compiler will respect statement ordering
        // w.r.t. assignment.
        //
        hInterfaceDeviceRootKey = hInterfaceDeviceRootKey;
    }

    if(hInterfaceDeviceRootKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hInterfaceDeviceRootKey);
    }

    if(Err == NO_ERROR) {
        *hInterfaceDeviceKey = hSubKey;
    }

    return Err;
}


DWORD
pSetupDeleteInterfaceDeviceKey(
    IN HKEY                      hInterfaceClassKey,
    IN PDEVICE_INFO_SET          DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA InterfaceDeviceData
    )
/*++

Routine Description:

    This routine deletes an interface device registry key (recursively deleting
    any subkeys as well).

Arguments:

    hInterfaceClassKey - Supplies the handle to the registry key underneath which the 2-level
        interface class hierarchy exists.

    DeviceInfoSet - Supplies a pointer to the device information set containing
        the interface device whose registry key is to be deleted.

    InterfaceDeviceData - Supplies a pointer to an interface device data structure
        indicating which interface device is to have its registry key deleted.

Return Value:

    If successful, the return value is NO_ERROR;

    If failure, the return value is a Win32 error code indicating the cause of failure.

--*/
{
    DWORD Err;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode;
    LPGUID ClassGuid;
    HKEY hInterfaceDeviceRootKey;
    PCTSTR DevicePath;

    Err = NO_ERROR;
    hInterfaceDeviceRootKey = INVALID_HANDLE_VALUE;

    try {
        //
        // Get the interface device node, and verify that its class matches what the
        // caller passed us.
        //
        InterfaceDeviceNode = (PINTERFACE_DEVICE_NODE)(InterfaceDeviceData->Reserved);
        ClassGuid = &(DeviceInfoSet->GuidTable[InterfaceDeviceNode->GuidIndex]);

        if(!IsEqualGUID(ClassGuid, &(InterfaceDeviceData->InterfaceClassGuid))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // Verify that this interface device hasn't been removed.
        //
        if(InterfaceDeviceNode->Flags & SPINT_REMOVED) {
            Err = ERROR_DEVICE_INTERFACE_REMOVED;
            goto clean0;
        }

        //
        // OK, now open the interface device's root storage key.
        //
        DevicePath = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                              InterfaceDeviceNode->SymLinkName
                                             );

        if(ERROR_SUCCESS != OpenDeviceInterfaceSubKey(hInterfaceClassKey,
                                                      DevicePath,
                                                      KEY_READ,
                                                      &hInterfaceDeviceRootKey,
                                                      NULL,
                                                      NULL)) {
            //
            // Make sure hInterfaceDeviceRootKey is still INVALID_HANDLE_VALUE, so we
            // won't try to close it later.
            //
            hInterfaceDeviceRootKey = INVALID_HANDLE_VALUE;
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        Err = RegistryDelnode(hInterfaceDeviceRootKey, pszDeviceParameters);

clean0:
        ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Access the following variable so the compiler will respect statement ordering
        // w.r.t. assignment.
        //
        hInterfaceDeviceRootKey = hInterfaceDeviceRootKey;
    }

    if(hInterfaceDeviceRootKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hInterfaceDeviceRootKey);
    }

    return Err;
}

