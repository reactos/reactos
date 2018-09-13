
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ci.c

Abstract:

    Battery Class Installer

Author:

    Scott Brenden

Environment:

Notes:


Revision History:

--*/




#include "proj.h"

#include <initguid.h>
#include <devguid.h>


BOOL APIENTRY LibMain(
    HANDLE hDll, 
    DWORD dwReason,  
    LPVOID lpReserved)
{
    
    switch( dwReason ) {
    case DLL_PROCESS_ATTACH:
        
        TRACE_MSG (TF_FUNC, "Battery Class Installer Loaded\n");
        DisableThreadLibraryCalls(hDll);

        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }


    
    return TRUE;
} 



DWORD
APIENTRY
BatteryClassInstall(
    IN DI_FUNCTION      DiFunction,
    IN HDEVINFO         DevInfoHandle,
    IN PSP_DEVINFO_DATA DevInfoData     OPTIONAL
    )       
/*++

Routine Description:

    This function is the class installer entry-point.

Arguments:

    DiFunction      - Requested installation function

    DevInfoHandle   - Handle to a device information set

    DevInfoData     - Pointer to device information about device to install

Return Value:

    

--*/
{
    DWORD                   status;
    SP_DEVINSTALL_PARAMS    devParams;

    
    //
    // Get the DeviceInstallParams, because some of the InstallFunction
    // handlers may find some of its fields useful.  Keep in mind not
    // to set the DeviceInstallParams using this same structure at the
    // end.  The handlers may have called functions which would change the
    // DeviceInstallParams, and simply calling SetupDiSetDeviceInstallParams
    // with this blanket structure would destroy those settings.
    //

    devParams.cbSize = sizeof(devParams);
    if (!SetupDiGetDeviceInstallParams(DevInfoHandle, DevInfoData, &devParams))
    {
        status = GetLastError();

    } else {
        TRACE_MSG (TF_GENERAL, "DiFunction = %x\n", DiFunction);

        //
        // Dispatch the InstallFunction
        //

        switch (DiFunction) {
            case DIF_INSTALLDEVICE:
                status = InstallCompositeBattery (DevInfoHandle, DevInfoData, &devParams);
                if (status == ERROR_SUCCESS) {
                    // 
                    // Let the default device installer actually install the battery. 
                    //
                    
                    status = ERROR_DI_DO_DEFAULT;
                }
                break;


            default:
                status = ERROR_DI_DO_DEFAULT;
                break;
        }
    }


    return status;
}





DWORD
PRIVATE
InstallCompositeBattery (
    IN     HDEVINFO                DevInfoHandle,
    IN     PSP_DEVINFO_DATA        DevInfoData,         OPTIONAL
    IN OUT PSP_DEVINSTALL_PARAMS   DevInstallParams
    )
/*++

Routine Description:

    This function installs the composite battery if it hasn't already been
    installed.

Arguments:

    DevInfoHandle       - Handle to a device information set

    DevInfoData         - Pointer to device information about device to install

    DevInstallParams    - Device install parameters associated with device 

Return Value:

    

--*/
{
    DWORD                   status;
    PSP_DEVINFO_DATA        newDevInfoData;
    HDEVINFO                newDevInfoHandle;
    SP_DRVINFO_DATA         driverInfoData;
    UCHAR                   tmpBuffer[100];
    DWORD                   bufferLen;
    
    
    // DebugBreak();

    //
    // Allocate local memory for a new device info structure
    //

    if(!(newDevInfoData = LocalAlloc(LPTR, sizeof(SP_DEVINFO_DATA)))) {
        status = GetLastError();
        TRACE_MSG (TF_ERROR, "Couldn't allocate composite battery device info- %x\n", status);
        goto clean0;
    }

    
    //
    // Create a new device info list.  Since we are "manufacturing" a completely new 
    // device with the Composite Battery, we can't use any of the information from 
    // the battery device list.
    //

    newDevInfoHandle = SetupDiCreateDeviceInfoList ((LPGUID)&GUID_DEVCLASS_SYSTEM, DevInstallParams->hwndParent);
    if (newDevInfoHandle == INVALID_HANDLE_VALUE) {
        status = GetLastError();
        TRACE_MSG (TF_ERROR, "Can't create DevInfoList - %x\n", status);
        goto clean1;
    }
    
    
    //
    // Attempt to manufacture a new device information element for the root enumerated
    // composite battery.
    //
    
    newDevInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
    if(!SetupDiCreateDeviceInfo(newDevInfoHandle,
                              TEXT("Root\\COMPOSITE_BATTERY\\0000"),
                              (LPGUID)&GUID_DEVCLASS_SYSTEM,
                              NULL,
                              DevInstallParams->hwndParent,  // same parent window as enumerated device
                              0,
                              newDevInfoData)) {

        status = GetLastError();

        if (status == ERROR_DEVINST_ALREADY_EXISTS) {
            //
            // The composite battery is already installed.  Our work is done.
            //

            TRACE_MSG (TF_GENERAL, "Composite Battery Already Installed\n");
            status = ERROR_SUCCESS;
            goto clean2;
        
        } else {

            TRACE_MSG (TF_ERROR, "Error creating composite battery devinfo - %x\n", status);
            goto clean2;
        }
    }


    //
    // Register the device so it is not a phantom anymore
    //

    if (!SetupDiRegisterDeviceInfo(newDevInfoHandle, newDevInfoData, 0, NULL, NULL, NULL)) {
        status = GetLastError();
        TRACE_MSG (TF_ERROR, "Couldn't register device - %x\n", status);
        goto clean3;
    }


    //
    // Set the hardware ID.  For the composite battery it will be COMPOSITE_BATTERY
    //
    
    memset (tmpBuffer, 0, sizeof(tmpBuffer));
    lstrcpy (tmpBuffer, TEXT("COMPOSITE_BATTERY"));

    bufferLen = lstrlen(tmpBuffer) + (2 * sizeof(TCHAR));
    TRACE_MSG (TF_GENERAL, "tmpBuffer - %s\n with strlen = %x\n", tmpBuffer, bufferLen);

    status = SetupDiSetDeviceRegistryProperty (
	                    newDevInfoHandle,
                        newDevInfoData,
                        SPDRP_HARDWAREID,
	                    tmpBuffer,
	                    bufferLen
	                    );

    if (!status) {
        status = GetLastError();
        TRACE_MSG(TF_ERROR, "Couldn't set the HardwareID - %x\n", status);
        goto clean3;
    }


    //
    // Build a compatible driver list for this new device...
    //
    
    if(!SetupDiBuildDriverInfoList(newDevInfoHandle, newDevInfoData, SPDIT_COMPATDRIVER)) {
        status = GetLastError();
        TRACE_MSG(TF_ERROR, "Couldn't build class driver list - %x\n", status);
        goto clean3;
    }


    //
    // Select the first driver in the list as this will be the most compatible
    //

    driverInfoData.cbSize = sizeof (SP_DRVINFO_DATA);
    if (!SetupDiEnumDriverInfo(newDevInfoHandle, newDevInfoData, SPDIT_COMPATDRIVER, 0, &driverInfoData)) {
        status = GetLastError();
        TRACE_MSG(TF_ERROR, "Couldn't get driver list - %x\n", status);
        goto clean3;

    } else {
        TRACE_MSG(TF_GENERAL, "Driver info - \n"
                              "------------- DriverType     %x\n"
                              "------------- Description    %s\n"
                              "------------- MfgName        %s\n"
                              "------------- ProviderName   %s\n\n",
                              driverInfoData.DriverType,
                              driverInfoData.Description,
                              driverInfoData.MfgName,
                              driverInfoData.ProviderName);
	    if (!SetupDiSetSelectedDriver(newDevInfoHandle, newDevInfoData, &driverInfoData)) {
            status = GetLastError();
            TRACE_MSG (TF_ERROR, "Couldn't select driver - %x\n", status);
            goto clean4;
        } 
    }

    
    //
    // Install the device
    //

    if (!SetupDiInstallDevice (newDevInfoHandle, newDevInfoData)) {
        status = GetLastError();
        TRACE_MSG (TF_ERROR, "Couldn't install device - %x\n", status);
        goto clean4;
    }

    
    //
    // If we got here we were successful
    //

    status = ERROR_SUCCESS;
    SetLastError (status);
    goto clean1;


clean4:
    SetupDiDestroyDriverInfoList (newDevInfoHandle, newDevInfoData, SPDIT_COMPATDRIVER);

clean3:
    SetupDiDeleteDeviceInfo (newDevInfoHandle, newDevInfoData);

clean2:
    SetupDiDestroyDeviceInfoList (newDevInfoHandle);

clean1:
    LocalFree (newDevInfoData);

clean0:
   return status;
}

