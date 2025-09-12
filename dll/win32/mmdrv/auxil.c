/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 lib/mmdrv/auxil.c
 * PURPOSE:              Multimedia User Mode Driver
 * PROGRAMMER:           Andrew Greenwood
 *                       Aleksey Bragin
 * UPDATE HISTORY:
 *                       Mar 16, 2004: Created skeleton implementation
 */

#include "mmdrv.h"

#define NDEBUG
#include <debug.h>

#include "wave.h"

APIENTRY DWORD auxMessage(UINT dwId,
                  UINT uMessage,
                  DWORD dwUser,
                  DWORD dwParam1,
                  DWORD dwParam2)

{
    MMRESULT Result;
    AUX_DD_VOLUME Volume;

    DPRINT("auxMessage\n");


	// the following cases are documented by DDK
	switch (uMessage)
	{
	case AUXDM_GETDEVCAPS:
		DPRINT("AUXDM_GETDEVCAPS");
		return GetDeviceCapabilities(dwId, AuxDevice, (LPBYTE)dwParam1, (DWORD)dwParam2);

	case AUXDM_GETNUMDEVS:
		DPRINT("AUXDM_GETNUMDEVS");
		return GetDeviceCount(AuxDevice);

	case AUXDM_GETVOLUME:
         DPRINT("AUXDM_GETVOLUME");
         Result = AuxGetAudio(dwId, (PBYTE) &Volume, sizeof(Volume));

         if (Result == MMSYSERR_NOERROR)
         {
            *(LPDWORD)dwParam1 = (DWORD)MAKELONG(HIWORD(Volume.Left), HIWORD(Volume.Right));
         }
         return Result;


	case AUXDM_SETVOLUME:
        DPRINT("AUXDM_SETVOLUME");

        Volume.Right = HIWORD(dwParam1) << 16;
        Volume.Left = LOWORD(dwParam1) << 16;

        return AuxSetAudio(dwId, (PBYTE)&Volume, sizeof(Volume));

	}

    return MMSYSERR_NOERROR;
}


DWORD AuxGetAudio(DWORD dwID, PBYTE pVolume, DWORD sizeVolume)
{
    HANDLE DeviceHandle;
    MMRESULT Result;
    DWORD BytesReturned;

    Result = OpenDevice(AuxDevice, dwID, &DeviceHandle, GENERIC_READ);
    if (Result != MMSYSERR_NOERROR)
         return Result;


    Result = DeviceIoControl(DeviceHandle, IOCTL_AUX_GET_VOLUME, NULL, 0, (LPVOID)pVolume, sizeVolume,
                           &BytesReturned, NULL) ? MMSYSERR_NOERROR : TranslateStatus();


    CloseHandle(DeviceHandle);

    return Result;
 }

DWORD AuxSetAudio(DWORD dwID, PBYTE pVolume, DWORD sizeVolume)
{
    HANDLE DeviceHandle;
    MMRESULT Result;
    DWORD BytesReturned;

    Result = OpenDevice(AuxDevice, dwID, &DeviceHandle, GENERIC_READ);
    if (Result != MMSYSERR_NOERROR)
         return Result;

    Result = DeviceIoControl(DeviceHandle, IOCTL_AUX_SET_VOLUME, (LPVOID)pVolume, sizeVolume, NULL, 0,
                           &BytesReturned, NULL) ? MMSYSERR_NOERROR : TranslateStatus();


    CloseHandle(DeviceHandle);

    return Result;
 }

