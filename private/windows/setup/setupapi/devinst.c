/*++

Copyright (c) 1993-1998 Microsoft Corporation

Module Name:

    devinst.c

Abstract:

    Device Installer routines.

Author:

    Lonny McMichael (lonnym) 1-Aug-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Private prototypes.
//
DWORD
pSetupDiGetCoInstallerList(
    IN     HDEVINFO                 DeviceInfoSet,
    IN     PSP_DEVINFO_DATA         DeviceInfoData,
    IN     CONST GUID              *ClassGuid,
    IN OUT PDEVINSTALL_PARAM_BLOCK  InstallParamBlock
    );


//
// Private logging data
// these must be mirrored from setupapi.h
//
static LPCTSTR pSetupDiDifStrings[] = {
    NULL, // no DIF code
    TEXT("DIF_SELECTDEVICE"),
    TEXT("DIF_INSTALLDEVICE"),
    TEXT("DIF_ASSIGNRESOURCES"),
    TEXT("DIF_PROPERTIES"),
    TEXT("DIF_REMOVE"),
    TEXT("DIF_FIRSTTIMESETUP"),
    TEXT("DIF_FOUNDDEVICE"),
    TEXT("DIF_SELECTCLASSDRIVERS"),
    TEXT("DIF_VALIDATECLASSDRIVERS"),
    TEXT("DIF_INSTALLCLASSDRIVERS"),
    TEXT("DIF_CALCDISKSPACE"),
    TEXT("DIF_DESTROYPRIVATEDATA"),
    TEXT("DIF_VALIDATEDRIVER"),
    TEXT("DIF_MOVEDEVICE"),
    TEXT("DIF_DETECT"),
    TEXT("DIF_INSTALLWIZARD"),
    TEXT("DIF_DESTROYWIZARDDATA"),
    TEXT("DIF_PROPERTYCHANGE"),
    TEXT("DIF_ENABLECLASS"),
    TEXT("DIF_DETECTVERIFY"),
    TEXT("DIF_INSTALLDEVICEFILES"),
    TEXT("DIF_UNREMOVE"),
    TEXT("DIF_SELECTBESTCOMPATDRV"),
    TEXT("DIF_ALLOW_INSTALL"),
    TEXT("DIF_REGISTERDEVICE"),
    TEXT("DIF_NEWDEVICEWIZARD_PRESELECT"),
    TEXT("DIF_NEWDEVICEWIZARD_SELECT"),
    TEXT("DIF_NEWDEVICEWIZARD_PREANALYZE"),
    TEXT("DIF_NEWDEVICEWIZARD_POSTANALYZE"),
    TEXT("DIF_NEWDEVICEWIZARD_FINISHINSTALL"),
    NULL, // DIF_UNUSED1
    TEXT("DIF_INSTALLINTERFACES"),
    TEXT("DIF_DETECTCANCEL"),
    TEXT("DIF_REGISTER_COINSTALLERS"),
    TEXT("DIF_ADDPROPERTYPAGE_ADVANCED"),
    TEXT("DIF_ADDPROPERTYPAGE_BASIC"),
    TEXT("DIF_RESERVED1"),
    TEXT("DIF_TROUBLESHOOTER"),
    TEXT("DIF_POWERMESSAGEWAKE")
    //
    // append new DIF codes here
    //
};





#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiGetDeviceInstallParamsA(
    IN  HDEVINFO                DeviceInfoSet,
    IN  PSP_DEVINFO_DATA        DeviceInfoData,          OPTIONAL
    OUT PSP_DEVINSTALL_PARAMS_A DeviceInstallParams
    )
{
    SP_DEVINSTALL_PARAMS_W deviceInstallParams;
    DWORD rc;
    BOOL b;

    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
    b = SetupDiGetDeviceInstallParamsW(DeviceInfoSet,DeviceInfoData,&deviceInstallParams);
    rc = GetLastError();

    if(b) {
        rc = pSetupDiDevInstParamsUnicodeToAnsi(&deviceInstallParams,DeviceInstallParams);
        if(rc != NO_ERROR) {
            b = FALSE;
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
SetupDiGetDeviceInstallParamsW(
    IN  HDEVINFO                DeviceInfoSet,
    IN  PSP_DEVINFO_DATA        DeviceInfoData,          OPTIONAL
    OUT PSP_DEVINSTALL_PARAMS_W DeviceInstallParams
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    UNREFERENCED_PARAMETER(DeviceInstallParams);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiGetDeviceInstallParams(
    IN  HDEVINFO              DeviceInfoSet,
    IN  PSP_DEVINFO_DATA      DeviceInfoData,          OPTIONAL
    OUT PSP_DEVINSTALL_PARAMS DeviceInstallParams
    )
/*++

Routine Description:

    This routine retrieves installation parameters for a device information set
    (globally), or a particular device information element.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        installation parameters to be retrieved.

    DeviceInfoData - Optionally, supplies the address of a SP_DEVINFO_DATA
        structure containing installation parameters to be retrieved.  If this
        parameter is not specified, then the installation parameters retrieved
        will be associated with the device information set itself (for the global
        class driver list).

    DeviceInstallParams - Supplies the address of a SP_DEVINSTALL_PARAMS structure
        that will receive the installation parameters.  The cbSize field of this
        structure must be set to the size, in bytes, of a SP_DEVINSTALL_PARAMS
        structure before calling this API.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {

        if(DeviceInfoData) {
            //
            // Then we are to retrieve installation parameters for a particular
            // device.
            //
            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                Err = ERROR_INVALID_PARAMETER;
            } else {
                Err = GetDevInstallParams(pDeviceInfoSet,
                                          &(DevInfoElem->InstallParamBlock),
                                          DeviceInstallParams
                                         );
            }
        } else {
            //
            // Retrieve installation parameters for the global class driver list.
            //
            Err = GetDevInstallParams(pDeviceInfoSet,
                                      &(pDeviceInfoSet->InstallParamBlock),
                                      DeviceInstallParams
                                     );
        }

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
SetupDiGetClassInstallParamsA(
    IN  HDEVINFO                DeviceInfoSet,
    IN  PSP_DEVINFO_DATA        DeviceInfoData,         OPTIONAL
    OUT PSP_CLASSINSTALL_HEADER ClassInstallParams,     OPTIONAL
    IN  DWORD                   ClassInstallParamsSize,
    OUT PDWORD                  RequiredSize            OPTIONAL
    )
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    PDEVINSTALL_PARAM_BLOCK InstallParamBlock;
    DI_FUNCTION Function;
    SP_SELECTDEVICE_PARAMS_W SelectDeviceParams;
    SP_SELECTDEVICE_PARAMS_A SelectDeviceParamsA;
    DWORD requiredSize;
    DWORD Err;
    BOOL b;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {

        if(DeviceInfoData) {
            //
            // Then we are to retrieve installation parameters for a particular
            // device.
            //
            if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,DeviceInfoData,NULL)) {
                InstallParamBlock = &DevInfoElem->InstallParamBlock;
            } else {
                Err = ERROR_INVALID_PARAMETER;
            }
        } else {
            //
            // Retrieve installation parameters for the global class driver list.
            //
            InstallParamBlock = &pDeviceInfoSet->InstallParamBlock;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    if(Err == NO_ERROR) {
        if(InstallParamBlock->ClassInstallHeader) {
            Function = InstallParamBlock->ClassInstallHeader->InstallFunction;
        } else {
            Err = ERROR_NO_CLASSINSTALL_PARAMS;
        }
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    if(Err == NO_ERROR) {
        //
        // For DIF_SELECTDEVICE we need special processing since
        // the structure that goes with it is ansi/unicode specific.
        //
        if(Function == DIF_SELECTDEVICE) {

            b = SetupDiGetClassInstallParamsW(
                    DeviceInfoSet,
                    DeviceInfoData,
                    (PSP_CLASSINSTALL_HEADER)&SelectDeviceParams,
                    sizeof(SP_SELECTDEVICE_PARAMS_W),
                    &requiredSize
                    );

            if(b) {
                Err = pSetupDiSelDevParamsUnicodeToAnsi(&SelectDeviceParams,&SelectDeviceParamsA);
                if(Err == NO_ERROR) {

                    try {

                        if(ClassInstallParams) {

                            if((ClassInstallParamsSize < sizeof(SP_CLASSINSTALL_HEADER)) ||
                               (ClassInstallParams->cbSize != sizeof(SP_CLASSINSTALL_HEADER))) {

                                Err = ERROR_INVALID_USER_BUFFER;
                            }

                        } else if(ClassInstallParamsSize) {
                            Err = ERROR_INVALID_USER_BUFFER;
                        }

                        if(Err == NO_ERROR) {
                            //
                            // Store required size in output parameter (if requested).
                            //
                            if(RequiredSize) {
                                *RequiredSize = sizeof(SP_SELECTDEVICE_PARAMS_A);
                            }

                            //
                            // See if supplied buffer is large enough.
                            //
                            if(ClassInstallParamsSize < sizeof(SP_SELECTDEVICE_PARAMS_A)) {
                                Err = ERROR_INSUFFICIENT_BUFFER;
                            } else {
                                CopyMemory(
                                    ClassInstallParams,
                                    &SelectDeviceParamsA,
                                    sizeof(SP_SELECTDEVICE_PARAMS_A)
                                    );
                            }
                        }
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        Err = ERROR_INVALID_PARAMETER;
                    }
                }
            } else {
                Err = GetLastError();
            }
        } else {
            b = SetupDiGetClassInstallParamsW(
                    DeviceInfoSet,
                    DeviceInfoData,
                    ClassInstallParams,
                    ClassInstallParamsSize,
                    RequiredSize
                    );
            if(b) {
                Err = NO_ERROR;
            } else {
                Err = GetLastError();
            }
        }
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}
#else
//
// Unicode version
//
BOOL
WINAPI
SetupDiGetClassInstallParamsW(
    IN  HDEVINFO                DeviceInfoSet,
    IN  PSP_DEVINFO_DATA        DeviceInfoData,         OPTIONAL
    OUT PSP_CLASSINSTALL_HEADER ClassInstallParams,     OPTIONAL
    IN  DWORD                   ClassInstallParamsSize,
    OUT PDWORD                  RequiredSize            OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    UNREFERENCED_PARAMETER(ClassInstallParams);
    UNREFERENCED_PARAMETER(ClassInstallParamsSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
WINAPI
SetupDiGetClassInstallParams(
    IN  HDEVINFO                DeviceInfoSet,
    IN  PSP_DEVINFO_DATA        DeviceInfoData,         OPTIONAL
    OUT PSP_CLASSINSTALL_HEADER ClassInstallParams,     OPTIONAL
    IN  DWORD                   ClassInstallParamsSize,
    OUT PDWORD                  RequiredSize            OPTIONAL
    )
/*++

Routine Description:

    This routine retrieves class installer parameters for a device information set
    (globally), or a particular device information element.  These parameters are
    specific to a particular device installer function code (DI_FUNCTION) that will
    be stored in the ClassInstallHeader field located at the beginning of the
    parameter buffer.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        class installer parameters to be retrieved.

    DeviceInfoData - Optionally, supplies the address of a SP_DEVINFO_DATA
        structure containing class installer parameters to be retrieved.  If this
        parameter is not specified, then the class installer parameters retrieved
        will be associated with the device information set itself (for the global
        class driver list).

    ClassInstallParams - Optionally, supplies the address of a buffer containing a
        class install header structure.  This structure must have its cbSize field
        set to sizeof(SP_CLASSINSTALL_HEADER) on input, or the buffer is considered
        to be invalid.  On output, the InstallFunction field will be filled in with
        the DI_FUNCTION code for the class install parameters being retrieved, and
        if the buffer is large enough, it will receive the class installer parameters
        structure specific to that function code.

        If this parameter is not specified, then ClassInstallParamsSize must be zero.
        This would be done if the caller simply wants to determine how large a buffer
        is required.

    ClassInstallParamsSize - Supplies the size, in bytes, of the ClassInstallParams
        buffer, or zero, if ClassInstallParams is not supplied.  If the buffer is
        supplied, it must be _at least_ as large as sizeof(SP_CLASSINSTALL_HEADER).

    RequiredSize - Optionally, supplies the address of a variable that receives
        the number of bytes required to store the class installer parameters.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {

        if(DeviceInfoData) {
            //
            // Then we are to retrieve installation parameters for a particular
            // device.
            //
            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                Err = ERROR_INVALID_PARAMETER;
            } else {
                Err = GetClassInstallParams(&(DevInfoElem->InstallParamBlock),
                                            ClassInstallParams,
                                            ClassInstallParamsSize,
                                            RequiredSize
                                           );
            }
        } else {
            //
            // Retrieve installation parameters for the global class driver list.
            //
            Err = GetClassInstallParams(&(pDeviceInfoSet->InstallParamBlock),
                                        ClassInstallParams,
                                        ClassInstallParamsSize,
                                        RequiredSize
                                       );
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
WINAPI
_SetupDiSetDeviceInstallParams(
    IN HDEVINFO              DeviceInfoSet,
    IN PSP_DEVINFO_DATA      DeviceInfoData,     OPTIONAL
    IN PSP_DEVINSTALL_PARAMS DeviceInstallParams,
    IN BOOL                  MsgHandlerIsNativeCharWidth
    )
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {

        if(DeviceInfoData) {
            //
            // Then we are to set installation parameters for a particular
            // device.
            //
            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                Err = ERROR_INVALID_PARAMETER;
            } else {
                Err = SetDevInstallParams(pDeviceInfoSet,
                                          DeviceInstallParams,
                                          &(DevInfoElem->InstallParamBlock),
                                          MsgHandlerIsNativeCharWidth
                                         );
            }
        } else {
            //
            // Set installation parameters for the global class driver list.
            //
            Err = SetDevInstallParams(pDeviceInfoSet,
                                      DeviceInstallParams,
                                      &(pDeviceInfoSet->InstallParamBlock),
                                      MsgHandlerIsNativeCharWidth
                                     );
        }

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
SetupDiSetDeviceInstallParamsA(
    IN HDEVINFO                DeviceInfoSet,
    IN PSP_DEVINFO_DATA        DeviceInfoData,     OPTIONAL
    IN PSP_DEVINSTALL_PARAMS_A DeviceInstallParams
    )
{
    DWORD rc;
    SP_DEVINSTALL_PARAMS_W deviceInstallParams;

    rc = pSetupDiDevInstParamsAnsiToUnicode(DeviceInstallParams,&deviceInstallParams);
    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }

    return(_SetupDiSetDeviceInstallParams(DeviceInfoSet,DeviceInfoData,&deviceInstallParams,FALSE));
}
#else
//
// Unicode version
//
BOOL
WINAPI
SetupDiSetDeviceInstallParamsW(
    IN HDEVINFO                DeviceInfoSet,
    IN PSP_DEVINFO_DATA        DeviceInfoData,     OPTIONAL
    IN PSP_DEVINSTALL_PARAMS_W DeviceInstallParams
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    UNREFERENCED_PARAMETER(DeviceInstallParams);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
WINAPI
SetupDiSetDeviceInstallParams(
    IN HDEVINFO              DeviceInfoSet,
    IN PSP_DEVINFO_DATA      DeviceInfoData,     OPTIONAL
    IN PSP_DEVINSTALL_PARAMS DeviceInstallParams
    )
/*++

Routine Description:

    This routine sets installation parameters for a device information set
    (globally), or a particular device information element.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        installation parameters to be set.

    DeviceInfoData - Optionally, supplies the address of a SP_DEVINFO_DATA
        structure containing installation parameters to be set.  If this
        parameter is not specified, then the installation parameters set
        will be associated with the device information set itself (for the
        global class driver list).

    DeviceInstallParams - Supplies the address of a SP_DEVINSTALL_PARAMS structure
        containing the new values of the parameters.  The cbSize field of this
        structure must be set to the size, in bytes, of the structure before
        calling this API.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    All parameters will be validated before any changes are made, so a return
    status of FALSE indicates that no parameters were modified.

--*/

{
    return(_SetupDiSetDeviceInstallParams(DeviceInfoSet,DeviceInfoData,DeviceInstallParams,TRUE));
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiSetClassInstallParamsA(
    IN HDEVINFO                DeviceInfoSet,
    IN PSP_DEVINFO_DATA        DeviceInfoData,        OPTIONAL
    IN PSP_CLASSINSTALL_HEADER ClassInstallParams,    OPTIONAL
    IN DWORD                   ClassInstallParamsSize
    )
{
    DWORD Err;
    DI_FUNCTION Function;
    SP_SELECTDEVICE_PARAMS_W SelectParams;
    BOOL b;

    if(!ClassInstallParams) {
        //
        // Just pass it on to the unicode version since there's
        // no thunking to do. Note that the size must be 0.
        //
        if(ClassInstallParamsSize) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }
        return SetupDiSetClassInstallParamsW(
                    DeviceInfoSet,
                    DeviceInfoData,
                    ClassInstallParams,
                    ClassInstallParamsSize
                    );
    }

    Err = NO_ERROR;

    try {
        if(ClassInstallParams->cbSize == sizeof(SP_CLASSINSTALL_HEADER)) {
            Function = ClassInstallParams->InstallFunction;
        } else {
            //
            // Structure is invalid.
            //
            Err = ERROR_INVALID_PARAMETER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    if(Err != NO_ERROR) {
        SetLastError(Err);
        return(FALSE);
    }

    //
    // DIF_SELECTDEVICE is a special case since it has
    // an structure that needs to be translated from ansi to unicode.
    // Others can just be passed on to the unicode version with
    // no changes to the parameters.
    //
    if(Function == DIF_SELECTDEVICE) {

        b = FALSE;
        if(ClassInstallParamsSize >= sizeof(SP_SELECTDEVICE_PARAMS_A)) {

            Err = pSetupDiSelDevParamsAnsiToUnicode(
                    (PSP_SELECTDEVICE_PARAMS_A)ClassInstallParams,
                    &SelectParams
                    );

            if(Err == NO_ERROR) {

                b = SetupDiSetClassInstallParamsW(
                        DeviceInfoSet,
                        DeviceInfoData,
                        (PSP_CLASSINSTALL_HEADER)&SelectParams,
                        sizeof(SP_SELECTDEVICE_PARAMS_W)
                        );

                Err = GetLastError();
            }
        } else {
            Err = ERROR_INVALID_PARAMETER;
        }
    } else {
        b = SetupDiSetClassInstallParamsW(
                DeviceInfoSet,
                DeviceInfoData,
                ClassInstallParams,
                ClassInstallParamsSize
                );

        Err = GetLastError();
    }

    SetLastError(Err);
    return(b);
}
#else
//
// Unicode version
//
BOOL
WINAPI
SetupDiSetClassInstallParamsW(
    IN HDEVINFO                DeviceInfoSet,
    IN PSP_DEVINFO_DATA        DeviceInfoData,        OPTIONAL
    IN PSP_CLASSINSTALL_HEADER ClassInstallParams,    OPTIONAL
    IN DWORD                   ClassInstallParamsSize
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    UNREFERENCED_PARAMETER(ClassInstallParams);
    UNREFERENCED_PARAMETER(ClassInstallParamsSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif


BOOL
WINAPI
SetupDiSetClassInstallParams(
    IN HDEVINFO                DeviceInfoSet,
    IN PSP_DEVINFO_DATA        DeviceInfoData,        OPTIONAL
    IN PSP_CLASSINSTALL_HEADER ClassInstallParams,    OPTIONAL
    IN DWORD                   ClassInstallParamsSize
    )
/*++

Routine Description:

    This routine sets (or clears) class installer parameters for a device
    information set (globally), or a particular device information element.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        class installer parameters to be set.

    DeviceInfoData - Optionally, supplies the address of a SP_DEVINFO_DATA
        structure containing class installer parameters to be set.  If this
        parameter is not specified, then the class installer parameters to be
        set will be associated with the device information set itself (for the
        global class driver list).

    ClassInstallParams - Optionally, supplies the address of a buffer containing
        the class installer parameters to be used.    The SP_CLASSINSTALL_HEADER
        structure at the beginning of the buffer must have its cbSize field set to
        be sizeof(SP_CLASSINSTALL_HEADER), and the InstallFunction field must be
        set to the DI_FUNCTION code reflecting the type of parameters supplied in
        the rest of the buffer.

        If this parameter is not supplied, then the current class installer parameters
        (if any) will be cleared for the specified device information set or element.

    ClassInstallParamsSize - Supplies the size, in bytes, of the ClassInstallParams
        buffer.  If the buffer is not supplied (i.e., the class installer parameters
        are to be cleared), then this value must be zero.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    All parameters will be validated before any changes are made, so a return
    status of FALSE indicates that no parameters were modified.

    A side effect of setting class installer parameters is that the DI_CLASSINSTALLPARAMS
    flag is set.  If for some reason, it is desired to set the parameters, but disable
    their use, then this flag must be cleared via SetupDiSetDeviceInstallParams.

    If the class installer parameters are cleared, then the DI_CLASSINSTALLPARAMS flag
    is reset.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {

        if(DeviceInfoData) {
            //
            // Then we are to set class installer parameters for a particular device.
            //
            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                Err = ERROR_INVALID_PARAMETER;
            } else {
                Err = SetClassInstallParams(pDeviceInfoSet,
                                            ClassInstallParams,
                                            ClassInstallParamsSize,
                                            &(DevInfoElem->InstallParamBlock)
                                           );
            }
        } else {
            //
            // Set class installer parameters for the global class driver list.
            //
            Err = SetClassInstallParams(pDeviceInfoSet,
                                        ClassInstallParams,
                                        ClassInstallParamsSize,
                                        &(pDeviceInfoSet->InstallParamBlock)
                                       );
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiCallClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )
/*++

Routine Description:

    This routine calls the appropriate class installer with the specified
    installer function.

    Before calling the class installer, this routine will call any registered
    co-device installers (registration is either per-class or per-device;
    per-class installers are called first).  Any co-installer wishing to be
    called back once the class installer has finished installation may return
    ERROR_DI_POSTPROCESSING_REQUIRED.  Returning NO_ERROR will also allow
    installation to continue, but without a post-processing callback.  Returning
    any other error code will cause the install action to be aborted (any
    co-installers already called that have requested post-processing will be
    called back, with InstallResult indicating the cause of failure).

    After the class installer has performed the installation (or we've done the
    default if ERROR_DI_DO_DEFAULT is returned), then we'll call any co-installers
    who have requested postprocessing.  The list of co-installers is treated
    like a stack, so the co-installers called last 'on the way in' are called
    first 'on the way out'.

Arguments:

    InstallFunction - Class installer function to call.  This can be one
        of the following values, or any other (class-specific) value:

        DIF_SELECTDEVICE - Select a driver to be installed.
        DIF_INSTALLDEVICE - Install the driver for the device.  (DeviceInfoData
            must be specified.)
        DIF_ASSIGNRESOURCES - ** PRESENTLY UNUSED ON WINDOWS NT **
        DIF_PROPERTIES - Display a properties dialog for the device.
            (DeviceInfoData must be specified.)
        DIF_REMOVE - Remove the device.  (DeviceInfoData must be specified.)
        DIF_FIRSTTIMESETUP - Perform first time setup initialization.  This
            is used only for the global class information associated with
            the device information set (i.e., DeviceInfoData not specified).
        DIF_FOUNDDEVICE - ** UNUSED ON WINDOWS NT **
        DIF_SELECTCLASSDRIVERS - Select drivers for all devices of the class
            associated with the device information set or element.
        DIF_VALIDATECLASSDRIVERS - Ensure all devices of the class associated
            with the device information set or element are ready to be installed.
        DIF_INSTALLCLASSDRIVERS - Install drivers for all devices of the
            class associated with the device information set or element.
        DIF_CALCDISKSPACE - Compute the amount of disk space required by
            drivers.
        DIF_DESTROYPRIVATEDATA - Destroy any private date referenced by
            the ClassInstallReserved installation parameter for the specified
            device information set or element.
        DIF_VALIDATEDRIVER - ** UNUSED ON WINDOWS NT **
        DIF_MOVEDEVICE - The device is being moved to a new location in the
            Enum branch.  This means that the device instance name will change.
            (DeviceInfoData must be specified.)
        DIF_DETECT - Detect any devices of class associated with the device
            information set.
        DIF_INSTALLWIZARD - Add any pages necessary to the New Device Wizard
            for the class associated with the device information set or element.
            ** OBSOLETE--use DIF_NEWDEVICEWIZARD method instead **
        DIF_DESTROYWIZARDDATA - Destroy any private data allocated due to
            a DIF_INSTALLWIZARD message.
            ** OBSOLETE--not needed for DIF_NEWDEVICEWIZARD method **
        DIF_PROPERTYCHANGE - The device's properties are changing. The device
            is being enabled, disabled, or has had a resource change.
            (DeviceInfoData must be specified.)
        DIF_ENABLECLASS - ** UNUSED ON WINDOWS NT **
        DIF_DETECTVERIFY - The class installer should verify any devices it
            previously detected.  Non verified devices should be removed.
        DIF_INSTALLDEVICEFILES - The class installer should only install the
            driver files for the selected device.  (DeviceInfoData must be
            specified.)
        DIF_UNREMOVE - Unremoves a device from the system.  (DeviceInfoData must
            be specified.)
        DIF_SELECTBESTCOMPATDRV - Select the best driver from the device
            information element's compatible driver list.  (DeviceInfoData must
            be specified.)
        DIF_ALLOW_INSTALL - Determine whether or not the selected driver should
            be installed for the device.  (DeviceInfoData must be specified.)
        DIF_REGISTERDEVICE - The class installer should register the new,
            manually-installed, device information element (via
            SetupDiRegisterDeviceInfo) including, potentially, doing duplicate
            detection via the SPRDI_FIND_DUPS flag.  (DeviceInfoData must be
            specified.)
        DIF_NEWDEVICEWIZARD_PRESELECT - Allows class-/co-installers to supply
            wizard pages to be displayed before the Select Device page during
            "Add New Hardware" wizard.
        DIF_NEWDEVICEWIZARD_SELECT - Allows class-/co-installers to supply
            wizard pages to replace the default Select Device wizard page, as
            retrieved by SetupDiGetWizardPage(...SPWPT_SELECTDEVICE...)
        DIF_NEWDEVICEWIZARD_PREANALYZE - Allows class-/co-installers to supply
            wizard pages to be displayed before the analyze page.
        DIF_NEWDEVICEWIZARD_POSTANALYZE - Allows class-/co-installers to supply
            wizard pages to be displayed after the analyze page.
        DIF_NEWDEVICEWIZARD_FINISHINSTALL - Allows class-/co-installers (including
            device-specific co-installers) to supply wizard pages to be displayed
            after installation of the device has been performed (i.e., after
            DIF_INSTALLDEVICE has been processed), but prior to the wizard's finish
            page.  This message is sent not only for the "Add New Hardware" wizard,
            but also for the autodetection and "New Hardware Found" scenarios as
            well.
        DIF_UNUSED1 - ** PRESENTLY UNUSED ON WINDOWS NT **
        DIF_INSTALLINTERFACES - The class installer should create (and/or,
            potentially remove) interface devices for this device information
            element.
        DIF_DETECTCANCEL - After the detection is stopped, if the class
            installer was invoked for DIF_DETECT, then it is invoked for
            DIF_DETECTCANCEL. This gives the class installer a chance to clean
            up anything it did during DIF_DETECT such as drivers setup to do
            detection at reboot, and private data. It is passed the same
            HDEVINFO as it was for DIF_DETECT.
        DIF_REGISTER_COINSTALLERS - Register device-specific co-installers so
            that they can be involved in the rest of the installation.
            (DeviceInfoData must be specified.)
        DIF_ADDPROPERTYPAGE_ADVANCED - Allows class-/co-installers to supply
            advanced property pages for a device.
        DIF_ADDPROPERTYPAGE_BASIC - Allows class-/co-installers to supply
            basic property pages for a device.
        DIF_TROUBLESHOOTER - Allows class-/co-installers to launch a troubleshooter
            for this device or to return CHM and HTM troubleshooter files that will
            get launched with a call to the HtmlHelp() API. If the class-/co-installer
            launches its own troubleshooter then it should return NO_ERROR, it should
            return ERROR_DI_DO_DEFAULT regardless of if it sets the CHM and HTM values.
        DIF_POWERMESSAGEWAKE - Allows class-/co-installers to specify text that will be
            displayed on the power tab in device manager. The class-/co-installer
            should return NO_ERROR if it specifies any text and ERROR_DI_DO_DEFAULT
            otherwise.

        (Note: remember to add new DIF_xxxx to pSetupDiDifStrings at start of file)

    DeviceInfoSet - Supplies a handle to the device information set to
        perform installation for.

    DeviceInfoData - Optionally, specifies a particular device information
        element whose class installer is to be called.  If this parameter
        is not specified, then the class installer for the device information
        set itself will be called (if the set has an associated class).

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    This function will attempt to load and call the class installer for the
    class associated with the device information element or set specified.
    If there is no class installer, or the class installer returns
    ERR_DI_DO_DEFAULT, then this function will call a default procedure for
    the specified class installer function.

--*/

{
    return _SetupDiCallClassInstaller(InstallFunction,
                                      DeviceInfoSet,
                                      DeviceInfoData,
                                      CALLCI_LOAD_HELPERS | CALLCI_CALL_HELPERS
                                     );
}


BOOL
_SetupDiCallClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,      OPTIONAL
    IN DWORD            Flags
    )
/*++

Routine Description:

    Worker routine for SetupDiCallClassInstaller that allows the caller to
    control what actions are taken when handling this install request.  In
    addition to the first three parameters (refer to the documentation for
    SetupDiCallClassInstaller for details), the following flags may be
    specified in the Flags parameter:

    CALLCI_LOAD_HELPERS - If helper modules (class installer, co-installers)
        haven't been loaded, load them so they can participate in handling
        this install request.

    CALLCI_CALL_HELPERS - Call the class installer/co-installers to give them
        a chance to handle this install request.  If this flag is not specified,
        then only the default action will be taken.

    CALLCI_ALLOW_DRVSIGN_UI - If an unsigned class installer or co-installer is
        encountered, perform standard non-driver signing behavior.  (WHQL
        doesn't have a certification program for class-/co-installers!)

        BUGBUG (lonnym): We should probably employ driver signing policy
        (instead of non-driver signing policy) for class installers of
        WHQL-approved classes.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet;
    BOOL b, MustAbort;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    PDEVINSTALL_PARAM_BLOCK InstallParamBlock;
    HKEY hk;
    CONST GUID *ClassGuid;
    BOOL bRestoreMiniIconUsage = FALSE;
    BOOL MuteError = FALSE;
    PCOINSTALLER_INTERNAL_CONTEXT CoInstallerInternalContext;
    LONG i;
    HWND hwndParent;
    TCHAR DescBuffer[LINE_LEN];
    PTSTR DeviceDesc;
    PSETUP_LOG_CONTEXT LogContext = NULL;
    DWORD slot_dif_code = 0;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = ERROR_DI_DO_DEFAULT;
    CoInstallerInternalContext = NULL;
    i = 0;
    DevInfoElem = NULL;

    try {

        if(DeviceInfoData) {
            //
            // Then we are to call the class installer for a particular
            // device.
            //
            if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                       DeviceInfoData,
                                                       NULL)) {

                InstallParamBlock = &(DevInfoElem->InstallParamBlock);
                ClassGuid = &(DevInfoElem->ClassGuid);
            } else {
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }

        } else {
            InstallParamBlock = &(pDeviceInfoSet->InstallParamBlock);
            ClassGuid = pDeviceInfoSet->HasClassGuid ? &(pDeviceInfoSet->ClassGuid)
                                                     : NULL;
        }

        //
        // set the local log context before it gets used.
        //
        LogContext = InstallParamBlock->LogContext;

        if(Flags & CALLCI_LOAD_HELPERS) {
            //
            // Retrieve the parent window handle, as we may need it below if we
            // need to popup UI due to unsigned class-/co-installers.
            //
            if(hwndParent = InstallParamBlock->hwndParent) {
               if(!IsWindow(hwndParent)) {
                    hwndParent = NULL;
               }
            }

            //
            // Retrieve a device description to use in case we need to give a
            // driver signing warn/block popup.
            //
            if(GetBestDeviceDesc(DeviceInfoSet, DeviceInfoData, DescBuffer)) {
                DeviceDesc = DescBuffer;
            } else {
                DeviceDesc = NULL;
            }

            //
            // If the class installer has not been loaded, then load it and
            // get the function address for the ClassInstall function.
            //
            if(!InstallParamBlock->hinstClassInstaller) {

                if(ClassGuid &&
                   (hk = SetupDiOpenClassRegKey(ClassGuid, KEY_READ)) != INVALID_HANDLE_VALUE) {
                    DWORD slot = AllocLogInfoSlot(LogContext,FALSE);

                    WriteLogEntry(
                        LogContext,
                        slot,
                        MSG_LOG_CI_MODULE,
                        NULL,
                        DeviceDesc);

                    try {
                        Err = GetModuleEntryPoint(hk,
                                                  pszInstaller32,
                                                  pszCiDefaultProc,
                                                  &(InstallParamBlock->hinstClassInstaller),
                                                  &((FARPROC)InstallParamBlock->ClassInstallerEntryPoint),
                                                  &MustAbort,
                                                  LogContext,
                                                  hwndParent,
                                                  SetupapiVerifyClassInstProblem,
                                                  DeviceDesc,
                                                  DRIVERSIGN_NONE,
                                                  TRUE
                                                 );
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        Err = ERROR_INVALID_CLASS_INSTALLER;
                        if(InstallParamBlock->hinstClassInstaller) {
                            FreeLibrary(InstallParamBlock->hinstClassInstaller);
                            InstallParamBlock->hinstClassInstaller = NULL;
                        }
                        InstallParamBlock->ClassInstallerEntryPoint = NULL;
                    }

                    if (slot) {
                        ReleaseLogInfoSlot(LogContext,slot);
                    }


                    RegCloseKey(hk);

                    if((Err != NO_ERROR) && (Err != ERROR_DI_DO_DEFAULT)) {

                        if(!(InstallParamBlock->FlagsEx & DI_FLAGSEX_CI_FAILED)) {

                            TCHAR ClassName[MAX_GUID_STRING_LEN];
                            TCHAR Title[MAX_TITLE_LEN];

                            if(!SetupDiClassNameFromGuid(ClassGuid,
                                                         ClassName,
                                                         SIZECHARS(ClassName),
                                                         NULL)) {
                                //
                                // Use the ClassName buffer to hold the class
                                // GUID string (it's better than nothin')
                                //
                                pSetupStringFromGuid(ClassGuid,
                                                     ClassName,
                                                     SIZECHARS(ClassName)
                                                    );
                            }

                            //
                            // Write out an event log entry about this.
                            //
                            WriteLogEntry(
                                LogContext,
                                DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                                MSG_CI_LOADFAIL_ERROR,
                                NULL,
                                ClassName);
                            WriteLogError(
                                LogContext,
                                DRIVER_LOG_ERROR,
                                Err);
                            MuteError = TRUE;

                            if(!(GlobalSetupFlags & PSPGF_NONINTERACTIVE)) {

                                if(!LoadString(MyDllModuleHandle,
                                               IDS_DEVICEINSTALLER,
                                               Title,
                                               SIZECHARS(Title))) {
                                    *Title = TEXT('\0');
                                }
                                FormatMessageBox(MyDllModuleHandle,
                                                 InstallParamBlock->hwndParent,
                                                 MSG_CI_LOADFAIL_ERROR,
                                                 Title,
                                                 MB_OK,
                                                 ClassName
                                                );
                            }

                            InstallParamBlock->FlagsEx |= DI_FLAGSEX_CI_FAILED;
                        }

                        Err = ERROR_INVALID_CLASS_INSTALLER;
                        goto clean0;
                    }
                }
            }

            //
            // If we haven't retrieved a list of co-installers to call, retrieve the list
            // now.
            //
            if(InstallParamBlock->CoInstallerCount == -1) {

                DWORD slot = AllocLogInfoSlot(LogContext,FALSE);

                WriteLogEntry(
                    LogContext,
                    slot,
                    MSG_LOG_COINST_MODULE,
                    NULL,
                    DeviceDesc);

                Err = pSetupDiGetCoInstallerList(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 ClassGuid,
                                                 InstallParamBlock);

                if (slot) {
                    ReleaseLogInfoSlot(LogContext,slot);
                }

                if(Err != NO_ERROR) {
                    goto clean0;
                }

                MYASSERT(InstallParamBlock->CoInstallerCount >= 0);
            }
        }

        slot_dif_code = AllocLogInfoSlotOrLevel(LogContext,DRIVER_LOG_VERBOSE1,FALSE);
        if (slot_dif_code) {
            //
            // this is skipped if we know we would never log anything
            //
            // pass a string which we may log with an error
            // or will log at VERBOSE1 level
            //
            if ((InstallFunction >= (sizeof(pSetupDiDifStrings)/sizeof(pSetupDiDifStrings[0])))
                    || (pSetupDiDifStrings[InstallFunction]==NULL)) {
                //
                // bugbug!!! (jamiehun)
                // I would like to think this could be bad and should be logged at DRIVER_LOG_ERROR
                // however function header alludes to user defined DIF_xxxx codes
                //
                WriteLogEntry(
                    LogContext,
                    slot_dif_code,
                    MSG_LOG_DI_UNUSED_FUNC,
                    NULL,
                    InstallFunction);
            } else {
                //
                // use the string version of the DIF code
                //
                WriteLogEntry(
                    LogContext,
                    slot_dif_code,
                    MSG_LOG_DI_FUNC,
                    NULL,
                    pSetupDiDifStrings[InstallFunction]);
            }
        }

        if(Flags & CALLCI_CALL_HELPERS) {

            if(InstallParamBlock->CoInstallerCount > 0) {
                //
                // Allocate an array of co-installer context structures to be used when
                // calling (and potentially, re-calling) the entry points.
                //
                CoInstallerInternalContext = MyMalloc(sizeof(COINSTALLER_INTERNAL_CONTEXT) * InstallParamBlock->CoInstallerCount);
                if(!CoInstallerInternalContext) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean0;
                }

                ZeroMemory(CoInstallerInternalContext,
                           sizeof(COINSTALLER_INTERNAL_CONTEXT) * InstallParamBlock->CoInstallerCount
                          );

                //
                // Call each co-installer.  We must unlock the devinfo set first, to avoid deadlocks.
                //
                UnlockDeviceInfoSet(pDeviceInfoSet);
                pDeviceInfoSet = NULL;

                for(i = 0; i < InstallParamBlock->CoInstallerCount; i++) {
                    //
                    // Store entry point in our context array, because the class installer may destroy
                    // this devinfo element and we wouldn't know who to callback otherwise.
                    //
                    CoInstallerInternalContext[i].CoInstallerEntryPoint = InstallParamBlock->CoInstallerList[i].CoInstallerEntryPoint;

                    WriteLogEntry(
                              LogContext,
                              DRIVER_LOG_TIME,
                              MSG_LOG_COINST_START,
                              NULL,
                              i+1,InstallParamBlock->CoInstallerCount);
                    Err = CoInstallerInternalContext[i].CoInstallerEntryPoint(
                              InstallFunction,
                              DeviceInfoSet,
                              DeviceInfoData,
                              &(CoInstallerInternalContext[i].Context)
                             );

                    if(Err == ERROR_DI_POSTPROCESSING_REQUIRED) {
                        CoInstallerInternalContext[i].DoPostProcessing = TRUE;
                    } else if(Err != NO_ERROR) {
                        WriteLogEntry(
                                  LogContext,
                                  DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                                  MSG_LOG_COINST_END_ERROR,
                                  NULL,
                                  i+1,InstallParamBlock->CoInstallerCount);
                        WriteLogError(
                                  LogContext,
                                  DRIVER_LOG_ERROR,
                                  Err);
                        MuteError = TRUE; // already logged it
                        goto clean0;
                    }
                    WriteLogEntry(
                              LogContext,
                              DRIVER_LOG_VERBOSE1,
                              MSG_LOG_COINST_END,
                              NULL,
                              i+1,InstallParamBlock->CoInstallerCount);
                }
            }

            //
            // If there is a class installer entry point, then call it.
            //
            if(InstallParamBlock->ClassInstallerEntryPoint) {
                //
                // Make sure we don't have the HDEVINFO locked.
                //
                if(pDeviceInfoSet) {
                    UnlockDeviceInfoSet(pDeviceInfoSet);
                    pDeviceInfoSet = NULL;
                }

                WriteLogEntry(
                          LogContext,
                          DRIVER_LOG_TIME,
                          MSG_LOG_CI_START,
                          NULL);
                Err = InstallParamBlock->ClassInstallerEntryPoint(InstallFunction,
                                                                  DeviceInfoSet,
                                                                  DeviceInfoData
                                                                 );
            } else {
                Err = ERROR_DI_DO_DEFAULT;
            }
            if(Err != NO_ERROR && Err != ERROR_DI_DO_DEFAULT) {
                WriteLogEntry(
                          LogContext,
                          DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                          MSG_LOG_CI_END_ERROR,
                          NULL);
                WriteLogError(
                          LogContext,
                          DRIVER_LOG_ERROR,
                          Err);
            } else {
                WriteLogEntry(
                          LogContext,
                          DRIVER_LOG_VERBOSE1,
                          MSG_LOG_CI_END,
                          NULL);
            }

            if(Err != ERROR_DI_DO_DEFAULT) {
                MuteError = TRUE; // already logged it
                goto clean0;
            }
        }

        //
        // Now we need to retrieve the parameter block all over again (we don't
        // know what the class installer function might have done).
        //
        // First, re-acquire the lock on the HDEVINFO, if we released it above in order
        // to call the class installer.
        //
        if(!pDeviceInfoSet) {
            if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
                Err = ERROR_INVALID_HANDLE;
                goto clean0;
            }
        }

        if(DeviceInfoData) {
            if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                       DeviceInfoData,
                                                       NULL)) {

                InstallParamBlock = &(DevInfoElem->InstallParamBlock);
            } else {
                //
                // The device information element appears to have been
                // destroyed--treat this as if the DI_NODI_DEFAULTACTION
                // flag was set.
                //
                goto clean0;
            }
        } else {
            InstallParamBlock = &(pDeviceInfoSet->InstallParamBlock);
        }

        if(InstallParamBlock->Flags & DI_NODI_DEFAULTACTION) {
            //
            // We shouldn't provide a default action--just return the class installer result.
            //
            goto clean0;
        }

        Err = NO_ERROR;

        if((InstallFunction == DIF_SELECTDEVICE) && !(InstallParamBlock->Flags & DI_NOSELECTICONS)) {
            //
            // We don't want to display mini-icons in the default Select Device case.
            // Temporarily set the flag that prevents this.
            //
            InstallParamBlock->Flags |= DI_NOSELECTICONS;
            bRestoreMiniIconUsage = TRUE;
        }

        //
        // Now, release the HDEVINFO lock before calling the appropriate handler routine.
        //
        UnlockDeviceInfoSet(pDeviceInfoSet);
        pDeviceInfoSet = NULL;

        WriteLogEntry(
                  LogContext,
                  DRIVER_LOG_VERBOSE1,
                  MSG_LOG_CI_DEF_START,
                  NULL);

        switch(InstallFunction) {

            case DIF_SELECTDEVICE :

                b = SetupDiSelectDevice(DeviceInfoSet, DeviceInfoData);

                //
                // If we need to reset the DI_NOSELECTICONS flag we set above, then re-acquire
                // the lock and do that now.
                //
                if(bRestoreMiniIconUsage &&
                   (pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {

                    if(DeviceInfoData) {
                        if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                                   DeviceInfoData,
                                                                   NULL)) {

                            InstallParamBlock = &(DevInfoElem->InstallParamBlock);
                        } else {
                            InstallParamBlock = NULL;
                        }
                    } else {
                        InstallParamBlock = &(pDeviceInfoSet->InstallParamBlock);
                    }

                    if(InstallParamBlock) {
                        InstallParamBlock->Flags &= ~DI_NOSELECTICONS;
                    }
                }
                break;

            case DIF_SELECTBESTCOMPATDRV :

                b = SetupDiSelectBestCompatDrv(DeviceInfoSet, DeviceInfoData);
                break;

            case DIF_INSTALLDEVICE :

                b = SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData);
                break;

            case DIF_INSTALLDEVICEFILES :

                b = SetupDiInstallDriverFiles(DeviceInfoSet, DeviceInfoData);
                break;

            case DIF_INSTALLINTERFACES :

                b = SetupDiInstallDeviceInterfaces(DeviceInfoSet, DeviceInfoData);
                break;

            case DIF_REGISTER_COINSTALLERS :

                b = SetupDiRegisterCoDeviceInstallers(DeviceInfoSet, DeviceInfoData);
                break;

            case DIF_REMOVE :

                b = SetupDiRemoveDevice(DeviceInfoSet, DeviceInfoData);
                break;

            case DIF_UNREMOVE :

                b = SetupDiUnremoveDevice(DeviceInfoSet, DeviceInfoData);
                break;

            //
            // These are new messages for class installers such as the Network, where the
            // class installer will do all of the work.  If no action is taken, ie, the
            // class installer return ERROR_DI_DO_DEFAULT, then we return OK, since there
            // is no default action for these cases.
            //
            case DIF_SELECTCLASSDRIVERS:
            case DIF_VALIDATECLASSDRIVERS:
            case DIF_INSTALLCLASSDRIVERS:

                b = TRUE;
                Err = ERROR_DI_DO_DEFAULT;
                break;

            case DIF_MOVEDEVICE :

                b = SetupDiMoveDuplicateDevice(DeviceInfoSet, DeviceInfoData);
                break;

            case DIF_PROPERTYCHANGE :

                b = SetupDiChangeState(DeviceInfoSet, DeviceInfoData);
                break;

            case DIF_REGISTERDEVICE :

                b = SetupDiRegisterDeviceInfo(DeviceInfoSet,
                                              DeviceInfoData,
                                              0,
                                              NULL,
                                              NULL,
                                              NULL
                                             );
                break;

            //
            // If the DIF_ message is not one of the above, and it is not handled,
            // then let the caller handle it in a default manner.
            //
            default :
                b = TRUE;
                Err = ERROR_DI_DO_DEFAULT;
                break;
        }

        if(!b) {
            Err = GetLastError();
        }
        if(Err != NO_ERROR && Err != ERROR_DI_DO_DEFAULT) {
            WriteLogEntry(
                      LogContext,
                      DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                      MSG_LOG_CI_DEF_END_ERROR,
                      NULL);
            WriteLogError(
                      LogContext,
                      DRIVER_LOG_ERROR,
                      Err);
            MuteError = TRUE; // already logged it
        } else {
            WriteLogEntry(
                      LogContext,
                      DRIVER_LOG_VERBOSE1,
                      MSG_LOG_CI_DEF_END,
                      NULL);
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Access the following variables, so that the compiler will respect our statement
        // ordering w.r.t. assignment.
        //
        pDeviceInfoSet = pDeviceInfoSet;
        CoInstallerInternalContext = CoInstallerInternalContext;
        i = i;
        DevInfoElem = DevInfoElem;
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    //
    // Do a post-processing callback to any of the co-installers that requested one.
    //
    for(i--; i >= 0; i--) {

        if(CoInstallerInternalContext[i].DoPostProcessing) {

            CoInstallerInternalContext[i].Context.PostProcessing = TRUE;
            CoInstallerInternalContext[i].Context.InstallResult = Err;

            try {

                WriteLogEntry(
                          LogContext,
                          DRIVER_LOG_TIME,
                          MSG_LOG_COINST_POST_START,
                          NULL,
                          i);
                Err = CoInstallerInternalContext[i].CoInstallerEntryPoint(
                          InstallFunction,
                          DeviceInfoSet,
                          DevInfoElem ? DeviceInfoData : NULL,
                          &(CoInstallerInternalContext[i].Context)
                         );

                if(Err != NO_ERROR) {
                    WriteLogEntry(
                              LogContext,
                              DRIVER_LOG_VERBOSE1 | SETUP_LOG_BUFFER,
                              MSG_LOG_COINST_POST_END_ERROR,
                              NULL,
                              i+1);
                    WriteLogError(
                              LogContext,
                              DRIVER_LOG_VERBOSE1,
                              Err);
                } else {
                    WriteLogEntry(
                              LogContext,
                              DRIVER_LOG_VERBOSE1,
                              MSG_LOG_COINST_POST_END,
                              NULL,
                              i+1);
                }

            } except(EXCEPTION_EXECUTE_HANDLER) {
                ;   // ignore any co-installer that generates an exception during post-processing.
            }
        }
    }

    if(CoInstallerInternalContext) {
        MyFree(CoInstallerInternalContext);
    }

    //
    // If we just did a DIF_REGISTER_COINSTALLERS, then we invalidated our current list of
    // co-installers.  Clear our list, so it will be retrieved next time.
    // (NOTE:  Normally, the default action will be taken (i.e., SetupDiRegisterCoDeviceInstallers),
    // which will have already invalidated the list.  The class installer may have handled
    // this themselves, however, so we'll invalidate the list here as well just to be safe.)
    //
    if(InstallFunction == DIF_REGISTER_COINSTALLERS) {
        InvalidateHelperModules(DeviceInfoSet, DeviceInfoData, IHM_COINSTALLERS_ONLY);
    }

    if(!MuteError && Err != NO_ERROR && Err != ERROR_DI_DO_DEFAULT) {
        WriteLogEntry(
                  LogContext,
                  DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                  MSG_LOG_CCI_ERROR,
                  NULL,
                  i+1);
        WriteLogError(
                  LogContext,
                  DRIVER_LOG_ERROR,
                  Err);
    }

    if(slot_dif_code) {
        ReleaseLogInfoSlot(LogContext,slot_dif_code);
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
SetupDiInstallClassExA(
    IN HWND        hwndParent,         OPTIONAL
    IN PCSTR       InfFileName,        OPTIONAL
    IN DWORD       Flags,
    IN HSPFILEQ    FileQueue,          OPTIONAL
    IN CONST GUID *InterfaceClassGuid, OPTIONAL
    IN PVOID       Reserved1,
    IN PVOID       Reserved2
    )
{
    PCWSTR inf;
    DWORD rc;
    BOOL b;

    if(InfFileName) {
        rc = CaptureAndConvertAnsiArg(InfFileName,&inf);
    } else {
        rc = NO_ERROR;
        inf = NULL;
    }

    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }

    b = SetupDiInstallClassExW(hwndParent,inf,Flags,FileQueue,InterfaceClassGuid,Reserved1,Reserved2);
    rc = GetLastError();

    if(inf) {
        MyFree(inf);
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
SetupDiInstallClassExW(
    IN HWND        hwndParent,         OPTIONAL
    IN PCWSTR      InfFileName,        OPTIONAL
    IN DWORD       Flags,
    IN HSPFILEQ    FileQueue,          OPTIONAL
    IN CONST GUID *InterfaceClassGuid, OPTIONAL
    IN PVOID       Reserved1,
    IN PVOID       Reserved2
    )
{
    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(InfFileName);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(FileQueue);
    UNREFERENCED_PARAMETER(InterfaceClassGuid);
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Reserved2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiInstallClassEx(
    IN HWND        hwndParent,         OPTIONAL
    IN PCTSTR      InfFileName,        OPTIONAL
    IN DWORD       Flags,
    IN HSPFILEQ    FileQueue,          OPTIONAL
    IN CONST GUID *InterfaceClassGuid, OPTIONAL
    IN PVOID       Reserved1,
    IN PVOID       Reserved2
    )
/*++

Routine Description:

    This routine either:

        a) Installs a class installer by running the [ClassInstall32] section
           of the specified INF, or
        b) Installs an interface class specified in the InterfaceClassGuid
           parameter, running the install section for this class as listed in
           the [InterfaceInstall32] of the specified INF (if there is no entry,
           then installation simply involves creating the interface class subkey
           under the DeviceClasses key.

    If the InterfaceClassGuid parameter is specified, then we're installing an
    interface class (case b), otherwise, we're installing a class installer
    (case a).

Arguments:

    hwndParent - Optionally, supplies the handle of the parent window for any
        UI brought up as a result of installing this class.

    InfFileName - Optionally, supplies the name of the INF file containing a
        [ClassInstall32] section (if we're installing a class installer), or
        an [InterfaceInstall32] section with an entry for the specified interface
        class (if we're installing an interface class).  If installing a class
        installer, this parameter _must_ be supplied.

    Flags - Flags that control the installation.  May be a combination of the following:

        DI_NOVCP - This flag should be specified if HSPFILEQ is supplied.  This
            instructs SetupInstallFromInfSection to not create a queue of its
            own, and instead to use the caller-supplied one.  If this flag is
            specified, then no file copying will be done.

        DI_NOBROWSE - This flag should be specified if no file browsing should
            be allowed in the event a copy operation cannot find a specified
            file.  If the user supplies their own file queue, then this flag is
            ignored.

        DI_FORCECOPY - This flag should be specified if the files should always
            be copied, even if they're already present on the user's machine
            (i.e., don't ask the user if they want to keep their existing files).
            If the user supplies their own file queue, then this flag is ignored.

        DI_QUIETINSTALL - This flag should be specified if UI should be suppressed
            unless absolutely necessary (i.e., no progress dialog).  If the user
            supplies their own queue, then this flag is ignored.

            (NOTE:  During GUI-mode setup on Windows NT, quiet-install behavior
            is always employed in the absence of a user-supplied file queue.)

    FileQueue - If the DI_NOVCP flag is specified, then this parameter supplies a handle
        to a file queue where file operations are to be queued (but not committed).

    InterfaceClassGuid - Optionally, specifies the interface class to be installed.
        If this parameter is not specified, then we are installing a class installer
        whose class is the class of the INF specified by InfFileName.

    Reserved1, Reserved2 - Reserved for future use.  Must be NULL.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    This API is generally called by the Device Manager when it installs a device
    of a new (setup) class.

    Class installers may also use this API to install new interface classes.  Note
    that interface class installation can also happen automatically as a result of
    installing interface devices for a device instance (via SetupDiInstallInterfaceDevices).

--*/

{
    HINF hInf;
    DWORD Err, ScanQueueResult;
    TCHAR ClassInstallSectionName[MAX_SECT_NAME_LEN];
    GUID ClassGuid;
    HKEY hKey;
    PSP_FILE_CALLBACK MsgHandler;
    PVOID MsgHandlerContext;
    BOOL KeyNewlyCreated;
    PCTSTR GuidString, ClassName;
    BOOL CloseFileQueue;
    PTSTR SectionExtension;
    INFCONTEXT InterfaceClassInstallLine;
    PCTSTR UndecoratedInstallSection;
    DWORD InstallFlags;
    HMACHINE hMachine;
    REGMOD_CONTEXT RegContext;
    BOOL NoProgressUI;
    PSETUP_LOG_CONTEXT LogContext = NULL;

    //
    // Validate the flags.
    //
    if(Flags & ~(DI_NOVCP | DI_NOBROWSE | DI_FORCECOPY | DI_QUIETINSTALL)) {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    //
    // If the caller didn't specify an interface class GUID (i.e., we're installing a
    // class installer), then they'd better have supplied us with an INF filename.
    // Also, they have to pass NULL for the Reserved argument.
    //
    if((!InterfaceClassGuid && !InfFileName) || Reserved1 || Reserved2) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Make sure that the caller supplied us with a file queue, if necessary.
    //
    if((Flags & DI_NOVCP) && (!FileQueue || (FileQueue == INVALID_HANDLE_VALUE))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(hwndParent && !IsWindow(hwndParent)) {
        hwndParent = NULL;
    }

    //
    //bugbug. Take reserved field to be a hMachine. Since it is spec'd
    //to be NULL that will translate to the local machine, which is all we currently
    //support. Or do we want to make Reserved 1 a structure, and hMachine part of it?
    //This is for future expansion
    //
    hMachine = (HMACHINE) Reserved1;


    if(InfFileName) {

        if((hInf = SetupOpenInfFile(InfFileName,
                                    NULL,
                                    INF_STYLE_WIN4,
                                    NULL)) == INVALID_HANDLE_VALUE) {
            //
            // Last error is already set--just return failure.
            //
            return FALSE;
        }
        Err = InheritLogContext(((PLOADED_INF) hInf)->LogContext,&LogContext);
    } else {
        hInf = INVALID_HANDLE_VALUE;
        Err = CreateLogContext(NULL,&LogContext);
    }
    if (Err != NO_ERROR) {
        SetupCloseInfFile(hInf);
        SetLastError(Err);
        return FALSE;
    }
    //
    // we will look at ClassGuid at the end to give more error context
    // give it a zero-value so that we don't get random data
    // if we haven't initialized it before then
    //
    ZeroMemory(&ClassGuid,sizeof(ClassGuid));

    Err = NO_ERROR;
    MsgHandlerContext = NULL;
    hKey = NULL;
    KeyNewlyCreated = FALSE;
    CloseFileQueue = FALSE;

    try {

        if(InterfaceClassGuid) {
            TCHAR ClassGuidBuffer[GUID_STRING_LEN];
            //
            // Copy this GUID into our ClassGuid variable, which is used for
            // both installer and device interface classes.
            //
            CopyMemory(&ClassGuid, InterfaceClassGuid, sizeof(ClassGuid));

            //
            // Legacy (compatibility) class name is not needed for device
            // interface classes.
            //
            ClassName = NULL;

            if(pSetupStringFromGuid(&ClassGuid, ClassGuidBuffer, GUID_STRING_LEN) != NO_ERROR) {
                ClassGuidBuffer[0]=TEXT('*');
                ClassGuidBuffer[1]=TEXT('\0');
            }
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_INFO,
                MSG_LOG_DO_INTERFACE_CLASS_INSTALL,
                NULL,       // text message
                ClassGuidBuffer);

        } else {
            //
            // Retrieve the class GUID from the INF.  If it has no class GUID, then
            // we can't install from it (even if it specifies the class name).
            //
            // We utilize the fact that an INF handle is really a LOADED_INF pointer,
            // combined with the fact that no one else will ever access this handle
            // (hence no synchronization issues).  This permits us to retrieve this
            // version datum much more efficiently.
            //
            if(!(GuidString = pSetupGetVersionDatum(&((PLOADED_INF)hInf)->VersionBlock,
                                                    pszClassGuid))
               || (pSetupGuidFromString(GuidString, &ClassGuid) != NO_ERROR)) {

                Err = ERROR_INVALID_CLASS;
                goto clean0;
            }

            //
            // We'll need to get the class name out of the INF as well.
            //
            if(!(ClassName = pSetupGetVersionDatum(&((PLOADED_INF)hInf)->VersionBlock,
                                                   pszClass))) {
                Err = ERROR_INVALID_CLASS;
                goto clean0;
            }
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_INFO,
                MSG_LOG_DO_CLASS_INSTALL,
                NULL,       // text message
                (GuidString?GuidString:TEXT("*")),
                (ClassName?ClassName:TEXT("*")));
        }

        //
        // First, attempt to open the key (i.e., not create it).  If that fails,
        // then we'll try to create it.  That way, we can keep track of whether
        // clean-up is required if an error occurs.
        //
        if(CR_SUCCESS != CM_Open_Class_Key_Ex(&ClassGuid,
                                              ClassName,
                                              KEY_ALL_ACCESS,
                                              RegDisposition_OpenExisting,
                                              &hKey,
                                              InterfaceClassGuid ? CM_OPEN_CLASS_KEY_INTERFACE
                                                                 : CM_OPEN_CLASS_KEY_INSTALLER,
                                              hMachine))
        {
            //
            // The key doesn't already exist--we've got to create it.
            //
            if(CR_SUCCESS != CM_Open_Class_Key_Ex(&ClassGuid,
                                                  ClassName,
                                                  KEY_ALL_ACCESS,
                                                  RegDisposition_OpenAlways,
                                                  &hKey,
                                                  InterfaceClassGuid ? CM_OPEN_CLASS_KEY_INTERFACE
                                                                     : CM_OPEN_CLASS_KEY_INSTALLER,
                                                  hMachine))
            {
                hKey = NULL;    // make sure it's still NULL
                Err = ERROR_INVALID_DATA;
                goto clean0;
            }

            KeyNewlyCreated = TRUE;
        }

        if(hInf == INVALID_HANDLE_VALUE) {
            //
            // We've done all we need to do to install this interface device.
            //
            goto clean0;

        } else {
            //
            // Append the layout INF, if necessary.
            //
            SetupOpenAppendInfFile(NULL, hInf, NULL);
        }

        if(InterfaceClassGuid) {
            //
            // Look for an entry for this interface class in the [InterfaceInstall32] section
            // of the INF.
            //
            TCHAR GuidStringBuffer[GUID_STRING_LEN];

            pSetupStringFromGuid(InterfaceClassGuid, GuidStringBuffer, SIZECHARS(GuidStringBuffer));

            if(!SetupFindFirstLine(hInf,
                                   pszInterfaceInstall32,
                                   GuidStringBuffer,
                                   &InterfaceClassInstallLine)) {
                //
                // No install entry in this INF--we're done.
                //
                goto clean0;
            }

            //
            // Make sure the Flags field is zero.
            //
            if(SetupGetIntField(&InterfaceClassInstallLine, 2, (PINT)&InstallFlags) && InstallFlags) {
                Err = ERROR_BAD_INTERFACE_INSTALLSECT;
                goto clean0;
            }

            if((!(UndecoratedInstallSection = pSetupGetField(&InterfaceClassInstallLine, 1)))
               || !(*UndecoratedInstallSection))
            {
                //
                // No install section was given--we're done.
                //
                goto clean0;
            }

        } else {
            UndecoratedInstallSection = pszClassInstall32;

            ZeroMemory(&RegContext, sizeof(RegContext));
            RegContext.Flags |= INF_PFLAG_CLASSPROP;
            RegContext.ClassGuid = &ClassGuid;
            RegContext.hMachine = hMachine;
        }

        //
        // Get the 'real' (potentially OS/architecture-specific) class install
        // section name.
        //
        SetupDiGetActualSectionToInstall(hInf,
                                         UndecoratedInstallSection,
                                         ClassInstallSectionName,
                                         SIZECHARS(ClassInstallSectionName),
                                         NULL,
                                         &SectionExtension
                                        );
        //
        // Also say what section is about to be installed.
        //
        WriteLogEntry(LogContext,
            DRIVER_LOG_VERBOSE,
            MSG_LOG_CLASS_SECTION,
            NULL,
            ClassInstallSectionName);
        //
        // If this is the undecorated name, then make sure that the section actually exists.
        //
        if(!SectionExtension && (SetupGetLineCount(hInf, ClassInstallSectionName) == -1)) {
            Err = ERROR_SECTION_NOT_FOUND;
            WriteLogEntry(LogContext,
                DRIVER_LOG_ERROR,
                MSG_LOG_NOSECTION,
                NULL,
                ClassInstallSectionName);
            goto clean0;
        }

        if(!(Flags & DI_NOVCP)) {
            //
            // Since we may need to check the queued files to determine whether file copy
            // is necessary, we have to open our own queue, and commit it ourselves.
            //
            if((FileQueue = SetupOpenFileQueue()) != INVALID_HANDLE_VALUE) {
                CloseFileQueue = TRUE;
            } else {
                //
                // SetupOpenFileQueue sets actual error
                //
                Err = GetLastError();
                goto clean0;
            }

            NoProgressUI = (GuiSetupInProgress ||
                            (GlobalSetupFlags & PSPGF_NONINTERACTIVE) ||
                            (Flags & DI_QUIETINSTALL));

            if(!(MsgHandlerContext = SetupInitDefaultQueueCallbackEx(
                                         hwndParent,
                                         (NoProgressUI ? INVALID_HANDLE_VALUE : NULL),
                                         0,
                                         0,
                                         NULL))) {

                Err = ERROR_NOT_ENOUGH_MEMORY;
                SetupCloseFileQueue(FileQueue);
                CloseFileQueue = FALSE;
                goto clean0;
            }
            MsgHandler = SetupDefaultQueueCallback;
        }
        //
        // Replace the file queue's log context with current, if it's never been used
        //
        InheritLogContext(LogContext,&((PSP_FILE_QUEUE) FileQueue)->LogContext);

        Err = pSetupInstallFiles(hInf,
                                 NULL,
                                 ClassInstallSectionName,
                                 NULL,
                                 NULL,
                                 NULL,
                                 SP_COPY_NEWER_OR_SAME | SP_COPY_LANGUAGEAWARE |
                                     ((Flags & DI_NOBROWSE) ? SP_COPY_NOBROWSE : 0),
                                 NULL,
                                 FileQueue,
                                 //
                                 // This flag is ignored by pSetupInstallFiles
                                 // because we don't pass a callback here and we
                                 // pass a user-defined file queue. (In other words
                                 // we're not committing the queue so there's no
                                 // callback function to deal with, and the callback
                                 // would be the guy who would care about ansi vs unicode.)
                                 //
                                 TRUE
                                );

        if(CloseFileQueue) {

            if(Err == NO_ERROR) {
                //
                // We successfully queued up the file operations--now we need to commit
                // the queue.  First off, though, we should check to see if the files are
                // already there.  (If the 'force copy' flag is set, or if the INF is from
                // an OEM location, then we don't care if the files are already there--we
                // always need to copy them in that case.)
                //
                if((Flags & DI_FORCECOPY) || ((PLOADED_INF)hInf)->InfSourcePath) {
                    //
                    // always copy the files.
                    //
                    ScanQueueResult = 0;
                } else {
                    //
                    // Determine whether the queue actually needs to be committed.
                    //
                    // ScanQueueResult can have 1 of 3 values:
                    //
                    // 0: Some files were missing or invalid (i.e., digital
                    //    signatures weren't verified;
                    //    Must commit queue.
                    //
                    // 1: All files to be copied are already present/valid, and
                    //    the queue is empty;
                    //    Can skip committing queue.
                    //
                    // 2: All files to be copied are already present/valid, but
                    //    del/ren queues not empty.  Must commit queue. The
                    //    copy queue will have been emptied, so only del/ren
                    //    functions will be performed.
                    //
                    //
                    if(!SetupScanFileQueue(FileQueue,
                                           SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_PRUNE_COPY_QUEUE,
                                           hwndParent,
                                           NULL,
                                           NULL,
                                           &ScanQueueResult)) {
                        //
                        // SetupScanFileQueue should really never
                        // fail when you don't ask it to call a
                        // callback routine, but if it does, just
                        // go ahead and commit the queue.
                        //
                        ScanQueueResult = 0;
                    }
                }

                if(ScanQueueResult != 1) {
                    //
                    // Copy enqueued files. In this case the callback is
                    // SetupDefaultQueueCallback, so we know it's native char width.
                    //
                    if(!_SetupCommitFileQueue(hwndParent,
                                              FileQueue,
                                              MsgHandler,
                                              MsgHandlerContext,
                                              TRUE)) {
                        Err = GetLastError();
                    }
                }
            }

            //
            // Close our file queue handle.
            //
            SetupCloseFileQueue(FileQueue);
            CloseFileQueue = FALSE;
        }

        //
        // Terminate the default queue callback, if it was created.  (Do this before
        // checking the return status of the file copying.)
        //
        if(MsgHandlerContext) {
            SetupTermDefaultQueueCallback(MsgHandlerContext);
            MsgHandlerContext = NULL;
        }

        if(Err != NO_ERROR) {
            goto clean0;
        }

        //
        // If we get to here, then the file copying was successful--now we can perform
        // the rest of the installation. We don't pass a callback so we don't worry
        // about ansi vs unicode issues here.
        //
        if(!_SetupInstallFromInfSection(NULL,
                                        hInf,
                                        ClassInstallSectionName,
                                        SPINST_INIFILES
                                        | SPINST_REGISTRY
                                        | SPINST_INI2REG,
                                        hKey,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL,
                                        INVALID_HANDLE_VALUE,
                                        NULL,
                                        TRUE,
                                        (InterfaceClassGuid ? NULL : &RegContext)
                                       )) {
            Err = GetLastError();
            goto clean0;
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;

        if(MsgHandlerContext) {
            SetupTermDefaultQueueCallback(MsgHandlerContext);
        }
        if(CloseFileQueue) {
            SetupCloseFileQueue(FileQueue);
        }

        //
        // Reference the following variables so that the compiler will respect our statement
        // order w.r.t. assignment.
        //
        hKey = hKey;
        KeyNewlyCreated = KeyNewlyCreated;
    }

    if(hKey) {
        RegCloseKey(hKey);
        if((Err != NO_ERROR) && KeyNewlyCreated && !InterfaceClassGuid) {
            //
            // We hit an error, and the class installer key didn't previously exist,
            // so we want to remove it.
            //
            CM_Delete_Class_Key_Ex(&ClassGuid, CM_DELETE_CLASS_SUBKEYS,hMachine);
        }
    }

    if(hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }

    if (Err == NO_ERROR) {
        //
        // if we're >= DRIVER_LOG_INFO, give a +ve affirmation of install
        //
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_INFO,
            MSG_LOG_CLASS_INSTALLED,
            NULL,
            NULL);
    } else {
        //
        // indicate install failed, display error
        // we make an extra effort here to log the guid
        //
        TCHAR ClassGuidBuffer[GUID_STRING_LEN];

        if(pSetupStringFromGuid(&ClassGuid, ClassGuidBuffer, GUID_STRING_LEN) != NO_ERROR) {
            ClassGuidBuffer[0]=TEXT('*');
            ClassGuidBuffer[1]=TEXT('\0');
        }

        WriteLogEntry(
            LogContext,
            DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
            MSG_LOG_CLASS_ERROR_ENCOUNTERED,
            NULL,
            ClassGuidBuffer);
        WriteLogError(
            LogContext,
            DRIVER_LOG_ERROR,
            Err);
    }

    if (LogContext) {
        DeleteLogContext(LogContext);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiInstallClassA(
    IN HWND     hwndParent,  OPTIONAL
    IN PCSTR    InfFileName,
    IN DWORD    Flags,
    IN HSPFILEQ FileQueue    OPTIONAL
    )
{
    PCWSTR inf;
    DWORD rc;
    BOOL b;

    rc = CaptureAndConvertAnsiArg(InfFileName,&inf);
    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }

    b = SetupDiInstallClassExW(hwndParent,inf,Flags,FileQueue,NULL,NULL,NULL);
    rc = GetLastError();

    MyFree(inf);

    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupDiInstallClassW(
    IN HWND     hwndParent,  OPTIONAL
    IN PCWSTR   InfFileName,
    IN DWORD    Flags,
    IN HSPFILEQ FileQueue    OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(InfFileName);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(FileQueue);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiInstallClass(
    IN HWND     hwndParent,  OPTIONAL
    IN PCTSTR   InfFileName,
    IN DWORD    Flags,
    IN HSPFILEQ FileQueue    OPTIONAL
    )
/*++

Routine Description:

    This routine installs the [ClassInstall32] section of the specified INF.

Arguments:

    hwndParent - Optionally, supplies the handle of the parent window for any
        UI brought up as a result of installing this class.

    InfFileName - Supplies the name of the INF file containing a [ClassInstall32]
        section

    Flags - Flags that control the installation.  May be a combination of the following:

        DI_NOVCP - This flag should be specified if HSPFILEQ is supplied.  This
            instructs SetupInstallFromInfSection to not create a queue of its
            own, and instead to use the caller-supplied one.  If this flag is
            specified, then no file copying will be done.

        DI_NOBROWSE - This flag should be specified if no file browsing should
            be allowed in the event a copy operation cannot find a specified
            file.  If the user supplies their own file queue, then this flag is
            ignored.

        DI_FORCECOPY - This flag should be specified if the files should always
            be copied, even if they're already present on the user's machine
            (i.e., don't ask the user if they want to keep their existing files).
            If the user supplies their own file queue, then this flag is ignored.

        DI_QUIETINSTALL - This flag should be specified if UI should be suppressed
            unless absolutely necessary (i.e., no progress dialog).  If the user
            supplies their own queue, then this flag is ignored.

            (NOTE:  During GUI-mode setup on Windows NT, quiet-install behavior
            is always employed in the absence of a user-supplied file queue.)

    FileQueue - If the DI_NOVCP flag is specified, then this parameter supplies a handle
        to a file queue where file operations are to be queued (but not committed).

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    This API is generally called by the Device Manager when it installs a device
    of a new class.

--*/
{
    return SetupDiInstallClassEx(hwndParent,
                                 InfFileName,
                                 Flags,
                                 FileQueue,
                                 NULL,
                                 NULL,
                                 NULL
                                );
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiGetHwProfileFriendlyNameA(
    IN  DWORD  HwProfile,
    OUT PSTR   FriendlyName,
    IN  DWORD  FriendlyNameSize,
    OUT PDWORD RequiredSize      OPTIONAL
    )
{
    return SetupDiGetHwProfileFriendlyNameExA(HwProfile,
                                              FriendlyName,
                                              FriendlyNameSize,
                                              RequiredSize,
                                              NULL,
                                              NULL
                                             );
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupDiGetHwProfileFriendlyNameW(
    IN  DWORD  HwProfile,
    OUT PWSTR  FriendlyName,
    IN  DWORD  FriendlyNameSize,
    OUT PDWORD RequiredSize      OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(HwProfile);
    UNREFERENCED_PARAMETER(FriendlyName);
    UNREFERENCED_PARAMETER(FriendlyNameSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiGetHwProfileFriendlyName(
    IN  DWORD  HwProfile,
    OUT PTSTR  FriendlyName,
    IN  DWORD  FriendlyNameSize,
    OUT PDWORD RequiredSize      OPTIONAL
    )
/*++

Routine Description:

    See SetupDiGetHwProfileFriendlyNameEx for details.

--*/

{
    return SetupDiGetHwProfileFriendlyNameEx(HwProfile,
                                             FriendlyName,
                                             FriendlyNameSize,
                                             RequiredSize,
                                             NULL,
                                             NULL
                                            );
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiGetHwProfileFriendlyNameExA(
    IN  DWORD  HwProfile,
    OUT PSTR   FriendlyName,
    IN  DWORD  FriendlyNameSize,
    OUT PDWORD RequiredSize,     OPTIONAL
    IN  PCSTR  MachineName,      OPTIONAL
    IN  PVOID  Reserved
    )
{
    WCHAR UnicodeName[MAX_PROFILE_LEN];
    PSTR nameA;
    BOOL b;
    DWORD rc;
    DWORD requiredSize;
    PCWSTR UnicodeMachineName;

    if(MachineName) {
        rc = CaptureAndConvertAnsiArg(MachineName, &UnicodeMachineName);
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return FALSE;
        }
    } else {
        UnicodeMachineName = NULL;
    }

    b = SetupDiGetHwProfileFriendlyNameExW(HwProfile,
                                           UnicodeName,
                                           SIZECHARS(UnicodeName),
                                           &requiredSize,
                                           UnicodeMachineName,
                                           Reserved
                                          );
    rc = GetLastError();

    if(b) {

        if(nameA = UnicodeToAnsi(UnicodeName)) {

            requiredSize = lstrlenA(nameA) + 1;

            if(RequiredSize) {
                try {
                    *RequiredSize = requiredSize;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    rc = ERROR_INVALID_PARAMETER;
                    b = FALSE;
                }
            }

            if(b) {
                if(requiredSize > FriendlyNameSize) {
                    rc = ERROR_INSUFFICIENT_BUFFER;
                    b = FALSE;
                } else {
                    if(!lstrcpyA(FriendlyName,nameA)) {
                        //
                        // lstrcpy faulted, caller passed in bogus buffer.
                        //
                        rc = ERROR_INVALID_USER_BUFFER;
                        b = FALSE;
                    }
                }
            }

            MyFree(nameA);
        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            b = FALSE;
        }
    }

    if(UnicodeMachineName) {
        MyFree(UnicodeMachineName);
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
SetupDiGetHwProfileFriendlyNameExW(
    IN  DWORD  HwProfile,
    OUT PWSTR  FriendlyName,
    IN  DWORD  FriendlyNameSize,
    OUT PDWORD RequiredSize,     OPTIONAL
    IN  PCWSTR MachineName,      OPTIONAL
    IN  PVOID  Reserved
    )
{
    UNREFERENCED_PARAMETER(HwProfile);
    UNREFERENCED_PARAMETER(FriendlyName);
    UNREFERENCED_PARAMETER(FriendlyNameSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    UNREFERENCED_PARAMETER(MachineName);
    UNREFERENCED_PARAMETER(Reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiGetHwProfileFriendlyNameEx(
    IN  DWORD  HwProfile,
    OUT PTSTR  FriendlyName,
    IN  DWORD  FriendlyNameSize,
    OUT PDWORD RequiredSize,     OPTIONAL
    IN  PCTSTR MachineName,      OPTIONAL
    IN  PVOID  Reserved
    )
/*++

Routine Description:

    This routine retrieves the friendly name associated with a hardware profile ID.

Arguments:

    HwProfile - Supplies the hardware profile ID whose friendly name is to be
        retrieved.  If this parameter is 0, then the friendly name for the
        current hardware profile is retrieved.

    FriendlyName - Supplies the address of a character buffer that receives the
        friendly name of the hardware profile.

    FriendlyNameSize - Supplies the size, in characters, of the FriendlyName buffer.

    RequiredSize - Optionally, supplies the address of a variable that receives the
        number of characters required to store the friendly name (including
        terminating NULL).

    MachineName - Optionally, supplies the name of the remote machine containing the
        hardware profile whose friendly name is to be retrieved.  If this parameter
        is not specified, the local machine is used.

    Reserved - Reserved for future use--must be NULL.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    DWORD Err = ERROR_INVALID_HWPROFILE;
    HWPROFILEINFO HwProfInfo;
    ULONG i;
    CONFIGRET cr;
    DWORD NameLen;
    HMACHINE hMachine;

    //
    // Make sure the caller didn't pass us anything in the Reserved parameter.
    //
    if(Reserved) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // If the caller specified a remote machine name, connect to that machine
    //
    if(MachineName) {
        cr = CM_Connect_Machine(MachineName, &hMachine);
        if(cr != CR_SUCCESS) {
            SetLastError(MapCrToSpError(cr, ERROR_INVALID_DATA));
            return FALSE;
        }
    } else {
        hMachine = NULL;
    }

    //
    // If a hardware profile ID of 0 is specified, then retrieve information
    // about the current hardware profile, otherwise, enumerate the hardware
    // profiles, looking for the one specified.
    //
    if(HwProfile) {
        i = 0;
    } else {
        i = 0xFFFFFFFF;
    }

    do {

        if((cr = CM_Get_Hardware_Profile_Info_Ex(i, &HwProfInfo, 0, hMachine)) == CR_SUCCESS) {
            //
            // Hardware profile info retrieved--see if it's what we're looking for.
            //
            if(!HwProfile || (HwProfInfo.HWPI_ulHWProfile == HwProfile)) {

                try {

                    NameLen = lstrlen(HwProfInfo.HWPI_szFriendlyName) + 1;

                    if(RequiredSize) {
                        *RequiredSize = NameLen;
                    }

                    if(NameLen > FriendlyNameSize) {
                        Err = ERROR_INSUFFICIENT_BUFFER;
                    } else {
                        Err = NO_ERROR;
                        CopyMemory(FriendlyName,
                                   HwProfInfo.HWPI_szFriendlyName,
                                   NameLen * sizeof(TCHAR)
                                  );
                    }

                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Err = ERROR_INVALID_PARAMETER;
                }

                break;
            }
            //
            // This wasn't the profile we wanted--go on to the next one.
            //
            i++;

        } else if(!HwProfile || (cr != CR_NO_SUCH_VALUE)) {
            //
            // We should abort on any error other than CR_NO_SUCH_VALUE, otherwise
            // we might loop forever!
            //
            Err = ERROR_INVALID_DATA;
            break;
        }

    } while(cr != CR_NO_SUCH_VALUE);

    if(hMachine) {
        CM_Disconnect_Machine(hMachine);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiGetHwProfileList(
    OUT PDWORD HwProfileList,
    IN  DWORD  HwProfileListSize,
    OUT PDWORD RequiredSize,
    OUT PDWORD CurrentlyActiveIndex OPTIONAL
    )
/*++

Routine Description:

    This routine retrieves a list of all currently-defined hardware profile IDs.

Arguments:

    HwProfileList - Supplies the address of an array of DWORDs that will receive
        the list of currently defined hardware profile IDs.

    HwProfileListSize - Supplies the number of DWORDs in the HwProfileList array.

    RequiredSize - Supplies the address of a variable that receives the number
        of hardware profiles currently defined.  If this number is larger than
        HwProfileListSize, then the list will be truncated to fit the array size,
        and this value will indicate the array size that would be required to store
        the entire list (the function will fail, with GetLastError returning
        ERROR_INSUFFICIENT_BUFFER in that case).

    CurrentlyActiveIndex - Optionally, supplies the address of a variable that
        receives the index within the HwProfileList array of the currently active
        hardware profile.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    return SetupDiGetHwProfileListEx(HwProfileList,
                                     HwProfileListSize,
                                     RequiredSize,
                                     CurrentlyActiveIndex,
                                     NULL,
                                     NULL
                                    );
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiGetHwProfileListExA(
    OUT PDWORD HwProfileList,
    IN  DWORD  HwProfileListSize,
    OUT PDWORD RequiredSize,
    OUT PDWORD CurrentlyActiveIndex, OPTIONAL
    IN  PCSTR  MachineName,          OPTIONAL
    IN  PVOID  Reserved
    )
{
    PCWSTR UnicodeMachineName;
    DWORD rc;
    BOOL b;

    b = FALSE;

    if(MachineName) {
        rc = CaptureAndConvertAnsiArg(MachineName, &UnicodeMachineName);
    } else {
        UnicodeMachineName = NULL;
        rc = NO_ERROR;
    }

    if(rc == NO_ERROR) {

        b = SetupDiGetHwProfileListExW(HwProfileList,
                                       HwProfileListSize,
                                       RequiredSize,
                                       CurrentlyActiveIndex,
                                       UnicodeMachineName,
                                       Reserved
                                      );
        rc = GetLastError();
        if(UnicodeMachineName) {
            MyFree(UnicodeMachineName);
        }
    }

    SetLastError(rc);
    return b;
}
#else
//
// Unicode version
//
BOOL
WINAPI
SetupDiGetHwProfileListExW(
    OUT PDWORD HwProfileList,
    IN  DWORD  HwProfileListSize,
    OUT PDWORD RequiredSize,
    OUT PDWORD CurrentlyActiveIndex, OPTIONAL
    IN  PCWSTR MachineName,          OPTIONAL
    IN  PVOID  Reserved
    )
{
    UNREFERENCED_PARAMETER(HwProfileList);
    UNREFERENCED_PARAMETER(HwProfileListSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    UNREFERENCED_PARAMETER(CurrentlyActiveIndex);
    UNREFERENCED_PARAMETER(MachineName);
    UNREFERENCED_PARAMETER(Reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiGetHwProfileListEx(
    OUT PDWORD HwProfileList,
    IN  DWORD  HwProfileListSize,
    OUT PDWORD RequiredSize,
    OUT PDWORD CurrentlyActiveIndex, OPTIONAL
    IN  PCTSTR MachineName,          OPTIONAL
    IN  PVOID  Reserved
    )
/*++

Routine Description:

    This routine retrieves a list of all currently-defined hardware profile IDs.

Arguments:

    HwProfileList - Supplies the address of an array of DWORDs that will receive
        the list of currently defined hardware profile IDs.

    HwProfileListSize - Supplies the number of DWORDs in the HwProfileList array.

    RequiredSize - Supplies the address of a variable that receives the number
        of hardware profiles currently defined.  If this number is larger than
        HwProfileListSize, then the list will be truncated to fit the array size,
        and this value will indicate the array size that would be required to store
        the entire list (the function will fail, with GetLastError returning
        ERROR_INSUFFICIENT_BUFFER in that case).

    CurrentlyActiveIndex - Optionally, supplies the address of a variable that
        receives the index within the HwProfileList array of the currently active
        hardware profile.

    MachineName - Optionally, specifies the name of the remote machine to retrieve
        a list of hardware profiles for.

    Reserved - Reserved for future use--must be NULL.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    DWORD Err = NO_ERROR;
    DWORD CurHwProfile;
    HWPROFILEINFO HwProfInfo;
    ULONG i;
    CONFIGRET cr;
    HMACHINE hMachine;

    //
    // Make sure the caller didn't pass us anything in the Reserved parameter.
    //
    if(Reserved) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // If the caller specified a remote machine name, connect to that machine now.
    //
    if(MachineName) {
        cr = CM_Connect_Machine(MachineName, &hMachine);
        if(cr != CR_SUCCESS) {
            SetLastError(MapCrToSpError(cr, ERROR_INVALID_DATA));
            return FALSE;
        }
    } else {
        hMachine = NULL;
    }

    //
    // First retrieve the currently active hardware profile ID, so we'll know what
    // to look for when we're enumerating all profiles (only need to do this if the
    // user wants the index of the currently active hardware profile).
    //
    if(CurrentlyActiveIndex) {

        if(CM_Get_Hardware_Profile_Info_Ex(0xFFFFFFFF, &HwProfInfo, 0, hMachine) == CR_SUCCESS) {
            //
            // Store away the hardware profile ID.
            //
            CurHwProfile = HwProfInfo.HWPI_ulHWProfile;

        } else {
            Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
            goto clean0;
        }
    }

    try {
        //
        // Enumerate the hardware profiles, retrieving the ID for each.
        //
        i = 0;
        do {

            if((cr = CM_Get_Hardware_Profile_Info_Ex(i, &HwProfInfo, 0, hMachine)) == CR_SUCCESS) {
                if(i < HwProfileListSize) {
                    HwProfileList[i] = HwProfInfo.HWPI_ulHWProfile;
                }
                if(CurrentlyActiveIndex && (HwProfInfo.HWPI_ulHWProfile == CurHwProfile)) {
                    *CurrentlyActiveIndex = i;
                    //
                    // Clear the CurrentlyActiveIndex pointer, so we once we find the
                    // currently active profile, we won't have to keep comparing.
                    //
                    CurrentlyActiveIndex = NULL;
                }
                i++;
            }

        } while(cr == CR_SUCCESS);

        if(cr == CR_NO_MORE_HW_PROFILES) {
            //
            // Then we enumerated all hardware profiles.  Now see if we had enough
            // buffer to hold them all.
            //
            *RequiredSize = i;
            if(i > HwProfileListSize) {
                Err = ERROR_INSUFFICIENT_BUFFER;
            }
        } else {
            //
            // Something else happened (probably a key not present).
            //
            Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

clean0:

    if(hMachine) {
        CM_Disconnect_Machine(hMachine);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


DWORD
pSetupDiGetCoInstallerList(
    IN     HDEVINFO                 DeviceInfoSet,
    IN     PSP_DEVINFO_DATA         DeviceInfoData,   OPTIONAL
    IN     CONST GUID              *ClassGuid,
    IN OUT PDEVINSTALL_PARAM_BLOCK  InstallParamBlock
    )
/*++

Routine Description:

    This routine retrieves the list of co-installers (both class- and
    device-specific) and stores the entry points and module handles in
    the supplied install param block.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set to retrieve
        co-installers into.  If DeviceInfoSet is not specified, then the
        InstallParamBlock specified below will be that of the set itself.

    DeviceInfoData - Optionally, specifies the device information element
        for which a list of co-installers is to be retrieved.

    ClassGuid - Supplies the address of the (install) class GUID for which
        class-specific co-installers are to be retrieved.

    InstallParamBlock - Supplies the address of the install param block where
        the co-installer list is to be stored.  This will either be the param
        block of the set itself (if DeviceInfoData isn't specified), or of
        the specified device information element.

Return Value:

    If the function succeeds, the return value is NO_ERROR, otherwise it is
    a Win32 error code indicating the cause of failure.

--*/
{
    HKEY hk[2];
    DWORD Err, RegDataType, KeyIndex;
    LONG i;
    PTSTR CoInstallerBuffer;
    DWORD CoInstallerBufferSize;
    PTSTR CurEntry;
    PCOINSTALLER_NODE CoInstallerList, TempCoInstallerList;
    DWORD CoInstallerListSize;
    TCHAR GuidString[GUID_STRING_LEN];
    TCHAR DescBuffer[LINE_LEN];
    PTSTR DeviceDesc;
    HWND hwndParent;
    BOOL MustAbort;

    //
    // If there is already a list, then return success immediately.
    //
    if(InstallParamBlock->CoInstallerCount != -1) {
        return NO_ERROR;
    }

    //
    // Retrieve the parent window handle, as we may need it below if we need to
    // popup UI due to unsigned class-/co-installers.
    //
    if(hwndParent = InstallParamBlock->hwndParent) {
       if(!IsWindow(hwndParent)) {
            hwndParent = NULL;
       }
    }

    //
    // Retrieve a device description to use in case we need to give a driver
    // signing warn/block popup.
    //
    if(GetBestDeviceDesc(DeviceInfoSet, DeviceInfoData, DescBuffer)) {
        DeviceDesc = DescBuffer;
    } else {
        DeviceDesc = NULL;
    }

    //
    // Get the string form of the class GUID, because that will be the name of
    // the multi-sz value entry under HKLM\System\CCS\Control\CoDeviceInstallers
    // where class-specific co-installers will be registered
    //
    pSetupStringFromGuid(ClassGuid, GuidString, SIZECHARS(GuidString));

    //
    // Open the CoDeviceInstallers key, as well as the device's driver key (if a devinfo
    // element is specified).
    //
    Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszPathCoDeviceInstallers, 0, KEY_READ, &(hk[0]));

    if(Err != ERROR_SUCCESS) {
        hk[0] = INVALID_HANDLE_VALUE;
    }

    if(DeviceInfoData) {

        hk[1] = SetupDiOpenDevRegKey(DeviceInfoSet,
                                     DeviceInfoData,
                                     DICS_FLAG_GLOBAL,
                                     0,
                                     DIREG_DRV,
                                     KEY_READ
                                    );

    } else {
        hk[1] = INVALID_HANDLE_VALUE;
    }

    CoInstallerBuffer = NULL;
    CoInstallerBufferSize = 256 * sizeof(TCHAR);    // start out with 256-character buffer
    CoInstallerList = NULL;
    i = 0;

    try {

        for(KeyIndex = 0; KeyIndex < 2; KeyIndex++) {
            //
            // If we couldn't open a key for this location, move on to the next one.
            //
            if(hk[KeyIndex] == INVALID_HANDLE_VALUE) {
                continue;
            }

            //
            // Retrieve the multi-sz value containing the co-installer entries.
            //
            while(TRUE) {

                if(!CoInstallerBuffer) {
                    if(!(CoInstallerBuffer = MyMalloc(CoInstallerBufferSize))) {
                        Err = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }
                }

                Err = RegQueryValueEx(hk[KeyIndex],
                                      (KeyIndex ? pszCoInstallers32
                                                : GuidString),
                                      NULL,
                                      &RegDataType,
                                      (PBYTE)CoInstallerBuffer,
                                      &CoInstallerBufferSize
                                     );

                if(Err == ERROR_MORE_DATA) {
                    //
                    // Buffer wasn't large enough--free current one and try again with new size.
                    //
                    MyFree(CoInstallerBuffer);
                    CoInstallerBuffer = NULL;
                } else {
                    break;
                }
            }

            //
            // Only out-of-memory errors are treated as fatal here.
            //
            if(Err == ERROR_NOT_ENOUGH_MEMORY) {
                goto cleanClass0;
            } else if(Err == ERROR_SUCCESS) {
                //
                // Make sure the buffer we got back looks valid.
                //
                if((RegDataType != REG_MULTI_SZ) || (CoInstallerBufferSize < sizeof(TCHAR))) {
                    Err = ERROR_INVALID_COINSTALLER;
                    goto cleanClass0;
                }

                //
                // Count the number of entries in this multi-sz list.
                //
                for(CoInstallerListSize = 0, CurEntry = CoInstallerBuffer;
                    *CurEntry;
                    CoInstallerListSize++, CurEntry += (lstrlen(CurEntry) + 1)
                   );

                if(!CoInstallerListSize) {
                    //
                    // List is empty, move on to next one.
                    //
                    continue;
                }

                //
                // Allocate (or reallocate) an array large enough to hold this many co-installer entries.
                //
                if(CoInstallerList) {
                    TempCoInstallerList = MyRealloc(CoInstallerList,
                                                    (CoInstallerListSize + i) * sizeof(COINSTALLER_NODE)
                                                   );
                } else {
                    MYASSERT(i == 0);
                    TempCoInstallerList = MyMalloc(CoInstallerListSize * sizeof(COINSTALLER_NODE));
                }

                if(TempCoInstallerList) {
                    CoInstallerList = TempCoInstallerList;
                } else {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto cleanClass0;
                }

                //
                // Now loop through the list and get the co-installer for each entry.
                //
                for(CurEntry = CoInstallerBuffer; *CurEntry; CurEntry += (lstrlen(CurEntry) + 1)) {
                    //
                    // Initialize the hinstance to NULL, so we'll know whether or not we need to free
                    // the module if we hit an exception here.
                    //
                    CoInstallerList[i].hinstCoInstaller = NULL;

                    Err = GetModuleEntryPoint(INVALID_HANDLE_VALUE,
                                              CurEntry,
                                              pszCoInstallerDefaultProc,
                                              &(CoInstallerList[i].hinstCoInstaller),
                                              &((FARPROC)CoInstallerList[i].CoInstallerEntryPoint),
                                              &MustAbort,
                                              InstallParamBlock->LogContext,
                                              hwndParent,
                                              SetupapiVerifyCoInstProblem,
                                              DeviceDesc,
                                              DRIVERSIGN_NONE,
                                              TRUE
                                             );

                    if(Err == NO_ERROR) {
                        i++;
                    } else {
                        //
                        // If the error we encountered above causes us to abort
                        // (e.g., due to a driver signing problem), then get
                        // out now.  Otherwise, just skip this failed entry and
                        // move on to the next.
                        //
                        if(MustAbort) {
                            goto cleanClass0;
                        }
                    }
                }
            }
        }

        //
        // If we get to here then we've successfully retrieved the co-installer
        // list(s)
        //
        Err = NO_ERROR;

cleanClass0:
        ;       // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_COINSTALLER;
        for(; i >= 0; i--) {
            if(CoInstallerList[i].hinstCoInstaller) {
                FreeLibrary(CoInstallerList[i].hinstCoInstaller);
            }
        }
        //
        // Reference the following variables so the compiler will respect our
        // statement ordering w.r.t. assignment.
        //
        CoInstallerBuffer = CoInstallerBuffer;
        CoInstallerList = CoInstallerList;
    }

    if(CoInstallerBuffer) {
        MyFree(CoInstallerBuffer);
    }

    for(KeyIndex = 0; KeyIndex < 2; KeyIndex++) {
        if(hk[KeyIndex] != INVALID_HANDLE_VALUE) {
            RegCloseKey(hk[KeyIndex]);
        }
    }

    if(Err == NO_ERROR) {
        InstallParamBlock->CoInstallerList  = CoInstallerList;
        InstallParamBlock->CoInstallerCount = i;
    } else if(CoInstallerList) {
        MyFree(CoInstallerList);
    }

    return Err;
}

