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


APIENTRY DWORD auxMessage(UINT uDevice,
                  UINT uMessage,
                  DWORD dwUser,
                  DWORD dwParam1,
                  DWORD dwParam2)

{
    printf("auxMessage\n");

	// the following cases are documented by DDK
	switch (uMessage)
	{
	case AUXDM_GETDEVCAPS:
		return 0;
	case AUXDM_GETNUMDEVS:
		return 0;
	case AUXDM_GETVOLUME:
		return 0;
	case AUXDM_SETVOLUME:
		return 0;
	}

    return MMSYSERR_NOERROR;


// WARNING: MS Code -- delete as soon as possible !
/*    MMRESULT mRet;
    AUX_DD_VOLUME Volume;

    switch (uMsg) {
    case AUXDM_GETDEVCAPS:
        dprintf2(("AUXDM_GETDEVCAPS"));
        return sndGetData(AuxDevice, uDevice, dwParam2, (LPBYTE)dwParam1,
                          IOCTL_AUX_GET_CAPABILITIES);

    case AUXDM_GETNUMDEVS:
        dprintf2(("AUXDM_GETNUMDEVS"));
        return sndGetNumDevs(AuxDevice);

    case AUXDM_GETVOLUME:
        dprintf2(("AUXDM_GETVOLUME"));

        mRet = sndGetData(AuxDevice, uDevice, sizeof(Volume),
                          (PBYTE)&Volume, IOCTL_AUX_GET_VOLUME);

        if (mRet == MMSYSERR_NOERROR) {
            *(LPDWORD)dwParam1 =
                (DWORD)MAKELONG(HIWORD(Volume.Left),
                                HIWORD(Volume.Right));
        }

        return mRet;

    case AUXDM_SETVOLUME:
        dprintf2(("AUXDM_SETVOLUME"));
        Volume.Left = LOWORD(dwParam1) << 16;
        Volume.Right = HIWORD(dwParam1) << 16;

        return sndSetData(AuxDevice, uDevice, sizeof(Volume),
                          (PBYTE)&Volume, IOCTL_AUX_SET_VOLUME);
    }
*/
}
