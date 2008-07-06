/*
    ReactOS Sound System
    Sound Blaster MME Driver

    Purpose:
        MME driver entry-point

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        6 July 2008 - Created
*/

#include <windows.h>
#include <ntddsnd.h>
#include <mmddk.h>
#include <mmebuddy.h>
#include <debug.h>


BOOLEAN FoundDevice(
    UCHAR DeviceType,
    PWSTR DevicePath,
    HANDLE Handle)
{
    /* Nothing particularly special required... */
    return ( AddSoundDevice(DeviceType, DevicePath) == MMSYSERR_NOERROR );
}


APIENTRY LONG
DriverProc(
    DWORD driver_id,
    HANDLE driver_handle,
    UINT message,
    LONG parameter1,
    LONG parameter2)
{
    switch ( message )
    {
        case DRV_LOAD :
            SOUND_DEBUG(L"DRV_LOAD");

            EnumerateNt4ServiceSoundDevices(L"sndblst",
                                            WAVE_OUT_DEVICE_TYPE,
                                            FoundDevice);

            return 1L;

        case DRV_FREE :
            SOUND_DEBUG(L"DRV_FREE");
            return 1L;

        default :
            return DefaultDriverProc(driver_id,
                                     driver_handle,
                                     message,
                                     parameter1,
                                     parameter2);
    }
}

int APIENTRY wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine,
    int nCmdShow)
{
    DriverProc(0, 0, DRV_LOAD, 0, 0);

    SOUND_DEBUG_HEX(wodMessage(0, WODM_GETNUMDEVS, 0, 0, 0));

    return 0;
}
