/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/mmdrv/wave.c
 * PURPOSE:              Multimedia User Mode Driver
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Jan 30, 2004: Imported into ReactOS tree
 */

#include "mmdrv.h"


static MMRESULT GetDeviceCapabilities(DWORD ID, UINT DeviceType,
                                      LPBYTE pCaps, DWORD Size)
{
    // FIXME: Implement :)
//    return sndGetData(DeviceType, id, dwSize, lpCaps,
//                      IOCTL_WAVE_GET_CAPABILITIES);

    HANDLE DeviceHandle = NULL;
    MMRESULT Result = MMSYSERR_NOERROR;
    DWORD BytesReturned = 0;

    // Open the wave device

    Result = OpenDevice(DeviceType, ID, &DeviceHandle, GENERIC_READ);
    if (Result != MMSYSERR_NOERROR)
         return Result;

    //
    // Set our data.
    //
    // Setting the overlapped parameter (last) to null means we
    // wait until the operation completes.
    //

    Result = DeviceIoControl(DeviceHandle, IOCTL_WAVE_GET_CAPABILITIES,
                            NULL, 0, (LPVOID)pCaps, Size,
                  &BytesReturned, NULL) ? MMSYSERR_NOERROR : TranslateStatus();


    // Close the handle and return the result code
//    CloseHandle(DeviceHandle);

    return Result;
}


APIENTRY DWORD wodMessage(DWORD ID, DWORD Message, DWORD User, DWORD Param1, DWORD Param2)
{
//    PWAVEALLOC pOutClient;
    MMRESULT Result;

    switch (Message) {
        case WODM_GETNUMDEVS:
            printf(("WODM_GETNUMDEVS"));
            return GetDeviceCount(WaveOutDevice);

        case WODM_GETDEVCAPS:
            printf(("WODM_GETDEVCAPS"));
            return GetDeviceCapabilities(ID, WaveOutDevice, (LPBYTE)Param1,
                                  (DWORD)Param2);

        case WODM_OPEN:
            printf(("WODM_OPEN"));
//            return waveOpen(WaveOutDevice, id, dwUser, dwParam1, dwParam2);

        case WODM_CLOSE:
            printf(("WODM_CLOSE"));
//            pOutClient = (PWAVEALLOC)dwUser;

            //
            // Call our task to see if it's ready to complete
            //
//            Result = waveThreadCall(WaveThreadClose, pOutClient);
            if (Result != MMSYSERR_NOERROR) {
                return Result;
            }

//            waveCallback(pOutClient, WOM_CLOSE, 0L);

            //
            // Close our device
            //
//            if (pOutClient->hDev != INVALID_HANDLE_VALUE) {
//                CloseHandle(pOutClient->hDev);

                EnterCriticalSection(&CS);
//                pOutClient->hDev = INVALID_HANDLE_VALUE;
                LeaveCriticalSection(&CS);
//            }

            return MMSYSERR_NOERROR;

        case WODM_WRITE:
            printf(("WODM_WRITE"));
/*            WinAssert(dwParam1 != 0);
            WinAssert(!(((LPWAVEHDR)dwParam1)->dwFlags &
                     ~(WHDR_INQUEUE|WHDR_DONE|WHDR_PREPARED|
                       WHDR_BEGINLOOP|WHDR_ENDLOOP)));

            ((LPWAVEHDR)dwParam1)->dwFlags &=
                (WHDR_INQUEUE|WHDR_DONE|WHDR_PREPARED|
                 WHDR_BEGINLOOP|WHDR_ENDLOOP);

            WinAssert(((LPWAVEHDR)dwParam1)->dwFlags & WHDR_PREPARED);

            // check if it's been prepared
            if (!(((LPWAVEHDR)dwParam1)->dwFlags & WHDR_PREPARED))
                return WAVERR_UNPREPARED;

            WinAssert(!(((LPWAVEHDR)dwParam1)->dwFlags & WHDR_INQUEUE));

            // if it is already in our Q, then we cannot do this
            if ( ((LPWAVEHDR)dwParam1)->dwFlags & WHDR_INQUEUE )
                return ( WAVERR_STILLPLAYING );

            // store the pointer to my WAVEALLOC structure in the wavehdr
            pOutClient = (PWAVEALLOC)dwUser;
            ((LPWAVEHDR)dwParam1)->reserved = (DWORD)(LPSTR)pOutClient;

            return waveWrite((LPWAVEHDR)dwParam1, pOutClient);
*/

        case WODM_PAUSE:
            printf(("WODM_PAUSE"));
/*
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.State = WAVE_DD_STOP;
            return waveThreadCall(WaveThreadSetState, pOutClient);
*/

        case WODM_RESTART:
            printf(("WODM_RESTART"));
/*
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.State = WAVE_DD_PLAY;
            return waveThreadCall(WaveThreadSetState, pOutClient);
*/

        case WODM_RESET:
            printf(("WODM_RESET"));

/*
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.State = WAVE_DD_RESET;
            return waveThreadCall(WaveThreadSetState, pOutClient);
*/

        case WODM_BREAKLOOP:
/*
            pOutClient = (PWAVEALLOC)dwUser;
            printf(("WODM_BREAKLOOP"));
            return waveThreadCall(WaveThreadBreakLoop, pOutClient);
*/


        case WODM_GETPOS:
            printf(("WODM_GETPOS"));
            /*
            pOutClient = (PWAVEALLOC)dwUser;
            return waveGetPos(pOutClient, (LPMMTIME)dwParam1, dwParam2);
            */

        case WODM_SETPITCH:
            printf(("WODM_SETPITCH"));
            /*
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.GetSetData.pData = (PBYTE)&dwParam1;
            pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            pOutClient->AuxParam.GetSetData.Function = IOCTL_WAVE_SET_PITCH;
            return waveThreadCall(WaveThreadSetData, pOutClient);
            */

        case WODM_SETVOLUME:
            printf(("WODM_SETVOLUME"));// i didnt do this - AG
            //pOutClient = (PWAVEALLOC)dwUser;
            //pOutClient->AuxParam.GetSetData.pData = *(PBYTE *)&dwParam1;
            //pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            //pOutClient->AuxParam.GetSetData.Function = IOCTL_WAVE_SET_VOLUME;
            //return waveThreadCall(WaveThreadSetData, pOutClient);

            {
                //
                // Translate to device volume structure
                //

//                WAVE_DD_VOLUME Volume;
//                Volume.Left = LOWORD(dwParam1) << 16;
//                Volume.Right = HIWORD(dwParam1) << 16;

//                return sndSetData(WaveOutDevice, id, sizeof(Volume),
//                                  (PBYTE)&Volume, IOCTL_WAVE_SET_VOLUME);
            }


        case WODM_SETPLAYBACKRATE:
        /*
            printf(("WODM_SETPLAYBACKRATE"));
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.GetSetData.pData = (PBYTE)&dwParam1;
            pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            pOutClient->AuxParam.GetSetData.Function =
                IOCTL_WAVE_SET_PLAYBACK_RATE;
            return waveThreadCall(WaveThreadSetData, pOutClient);
        */

        case WODM_GETPITCH:
        /*
            printf(("WODM_GETPITCH"));
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.GetSetData.pData = (PBYTE)dwParam1;
            pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            pOutClient->AuxParam.GetSetData.Function = IOCTL_WAVE_GET_PITCH;
            return waveThreadCall(WaveThreadGetData, pOutClient);
        */

        case WODM_GETVOLUME:
            printf(("WODM_GETVOLUME"));
            //pOutClient = (PWAVEALLOC)dwUser;
            //pOutClient->AuxParam.GetSetData.pData = *(PBYTE *)&dwParam1;
            //pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            //pOutClient->AuxParam.GetSetData.Function = IOCTL_WAVE_GET_VOLUME;
            //return waveThreadCall(WaveThreadGetData, pOutClient);

            {
                //
                // Translate to device volume structure
                //
/*
                WAVE_DD_VOLUME Volume;
                DWORD rc;

                rc = sndGetData(WaveOutDevice, id, sizeof(Volume),
                                (PBYTE)&Volume, IOCTL_WAVE_GET_VOLUME);

                if (rc == MMSYSERR_NOERROR) {
                    *(LPDWORD)dwParam1 =
                        (DWORD)MAKELONG(HIWORD(Volume.Left),
                                        HIWORD(Volume.Right));
                }

                return rc;
                */
            }

        case WODM_GETPLAYBACKRATE:
            printf(("WODM_GETPLAYBACKRATE"));
            /*
            pOutClient = (PWAVEALLOC)dwUser;
            pOutClient->AuxParam.GetSetData.pData = (PBYTE)dwParam1;
            pOutClient->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            pOutClient->AuxParam.GetSetData.Function =
                IOCTL_WAVE_GET_PLAYBACK_RATE;
            return waveThreadCall(WaveThreadGetData, pOutClient);
            */

        default:
            return MMSYSERR_NOTSUPPORTED;
    }

    //
    // Should not get here
    //

    return MMSYSERR_NOTSUPPORTED;
}


APIENTRY DWORD widMessage(DWORD id, DWORD msg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
    printf("widMessage\n");

    switch (msg) {
        case WIDM_GETNUMDEVS:
            return GetDeviceCount(WaveInDevice);
        default :
            return MMSYSERR_NOERROR;
    }
}

