/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 lib/mmdrv/wave.c
 * PURPOSE:              Multimedia User Mode Driver
 * PROGRAMMER:           Andrew Greenwood
 *                       Aleksey Bragin (aleksey at studiocerebral.com)
 * UPDATE HISTORY:
 *                       Jan 30, 2004: Imported into ReactOS tree (Greenwood)
 *                       Mar 16, 2004: Implemented some funcs (Bragin)
 */

#include "mmdrv.h"
#include "wave.h"

/* ============================
 *  INTERNAL
 *  functions start here
 * ============================
 */

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

static MMRESULT OpenWaveDevice(UINT  DeviceType,
								DWORD id,
								DWORD dwUser,
								DWORD dwParam1,
								DWORD dwParam2)
{
	// TODO: Implement
	return MMSYSERR_NOERROR;
}

//FIXME: Params are MS-specific
static MMRESULT ThreadCallWaveDevice(WAVETHREADFUNCTION Function, PWAVEALLOC pClient)
{
	return MMSYSERR_NOERROR;
}

//FIXME: Params are MS-specific
static void CallbackWaveDevice(PWAVEALLOC pWave, DWORD msg, DWORD dw1)
{
}

//FIXME: Params are MS-specific
static MMRESULT WriteWaveDevice(LPWAVEHDR pHdr, PWAVEALLOC pClient)
{
	return MMSYSERR_NOERROR;
}

//FIXME: MS-specific code, except for name of the func!
MMRESULT GetPositionWaveDevice(PWAVEALLOC pClient, LPMMTIME lpmmt, DWORD dwSize)
{
	/*
    WAVE_DD_POSITION PositionData;
    MMRESULT mErr;

    if (dwSize < sizeof(MMTIME))
        return MMSYSERR_ERROR;

    //
    // Get the current position from the driver
    //
    mErr = sndGetHandleData(pClient->hDev,
                            sizeof(PositionData),
                            &PositionData,
                            IOCTL_WAVE_GET_POSITION,
                            pClient->Event);

    if (mErr == MMSYSERR_NOERROR) {
        if (lpmmt->wType == TIME_BYTES) {
            lpmmt->u.cb = PositionData.ByteCount;
        }

        // default is samples.
        else {
            lpmmt->wType = TIME_SAMPLES;
            lpmmt->u.sample = PositionData.SampleCount;
        }
    }

    return mErr;*/ return MMSYSERR_NOERROR;
}

//FIXME: Params are MS-specific
MMRESULT soundSetData(UINT DeviceType, UINT DeviceId, UINT Length, PBYTE Data,
                     ULONG Ioctl)
{
	return MMSYSERR_NOERROR;
}

//FIXME: Params are MS-specific
MMRESULT soundGetData(UINT DeviceType, UINT DeviceId, UINT Length, PBYTE Data,
                     ULONG Ioctl)
{
	return MMSYSERR_NOERROR;
}


/* ============================
 *  EXPORT
 *  functions start here
 * ============================
 */

/*
 * @implemented
 */
APIENTRY DWORD wodMessage(DWORD dwId, DWORD dwMessage, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
    switch (dwMessage) {
        case WODM_GETNUMDEVS:
            printf("WODM_GETNUMDEVS");
            return GetDeviceCount(WaveOutDevice);

        case WODM_GETDEVCAPS:
            printf("WODM_GETDEVCAPS");
            return GetDeviceCapabilities(dwId, WaveOutDevice, (LPBYTE)dwParam1,
                                  (DWORD)dwParam2);

        case WODM_OPEN:
            printf("WODM_OPEN");
            return OpenWaveDevice(WaveOutDevice, dwId, dwUser, dwParam1, dwParam2);

        case WODM_CLOSE:
			{
				MMRESULT Result;
				PWAVEALLOC pTask = (PWAVEALLOC)dwUser;
				printf("WODM_CLOSE");

				// 1. Check if the task is ready to complete
				Result = ThreadCallWaveDevice(WaveThreadClose, pTask);
				if (Result != MMSYSERR_NOERROR) {
				    return Result;
				}
				else
					CallbackWaveDevice(pTask, WOM_CLOSE, 0L);
				
				// 2. Close the device
				if (pTask->hDev != INVALID_HANDLE_VALUE) {
					CloseHandle(pTask->hDev);

					EnterCriticalSection(&CS);
					pTask->hDev = INVALID_HANDLE_VALUE;
					LeaveCriticalSection(&CS);
				}

				return MMSYSERR_NOERROR;
			};

        case WODM_WRITE:
			{
				LPWAVEHDR pWaveHdr = (LPWAVEHDR)dwParam1;

				printf("WODM_WRITE");

				if (dwParam1 != 0)
					return MMSYSERR_INVALPARAM;

				if ((pWaveHdr->dwFlags & ~(WHDR_INQUEUE|WHDR_DONE|WHDR_PREPARED|WHDR_BEGINLOOP|WHDR_ENDLOOP)))
					return MMSYSERR_INVALPARAM;

				pWaveHdr->dwFlags &= (WHDR_INQUEUE|WHDR_DONE|WHDR_PREPARED|WHDR_BEGINLOOP|WHDR_ENDLOOP);

				if ((pWaveHdr->dwFlags & WHDR_PREPARED) == 0)
					return MMSYSERR_INVALPARAM;

				// Check, if the wave header is already prepared
				if (!(pWaveHdr->dwFlags & WHDR_PREPARED))
					return WAVERR_UNPREPARED;

				// If it's already located in the queue, this op is impossible
				if (pWaveHdr->dwFlags & WHDR_INQUEUE )
					return ( WAVERR_STILLPLAYING );

				// save WAVEALLOC pointer in the WaveHeader
				pWaveHdr->reserved = dwUser;

				return WriteWaveDevice(pWaveHdr, (PWAVEALLOC)dwUser);
			}


        case WODM_PAUSE:
            printf("WODM_PAUSE");
            ((PWAVEALLOC)dwUser)->AuxParam.State = WAVE_DD_STOP;
            return ThreadCallWaveDevice(WaveThreadSetState, (PWAVEALLOC)dwUser);

        case WODM_RESTART:
            printf("WODM_RESTART");
            ((PWAVEALLOC)dwUser)->AuxParam.State = WAVE_DD_PLAY;
            return ThreadCallWaveDevice(WaveThreadSetState, (PWAVEALLOC)dwUser);

        case WODM_RESET:
            printf("WODM_RESET");
            ((PWAVEALLOC)dwUser)->AuxParam.State = WAVE_DD_RESET;
            return ThreadCallWaveDevice(WaveThreadSetState, (PWAVEALLOC)dwUser);

        case WODM_BREAKLOOP:
            printf("WODM_BREAKLOOP");
            return ThreadCallWaveDevice(WaveThreadBreakLoop, (PWAVEALLOC)dwUser);

        case WODM_GETPOS:
            printf("WODM_GETPOS");
            return GetPositionWaveDevice(((PWAVEALLOC)dwUser), (LPMMTIME)dwParam1, dwParam2);

        case WODM_SETPITCH:
            printf("WODM_SETPITCH");
            ((PWAVEALLOC)dwUser)->AuxParam.GetSetData.pData = (PBYTE)&dwParam1;
            ((PWAVEALLOC)dwUser)->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            ((PWAVEALLOC)dwUser)->AuxParam.GetSetData.Function = IOCTL_WAVE_SET_PITCH;
            return ThreadCallWaveDevice(WaveThreadSetData, ((PWAVEALLOC)dwUser));

        case WODM_SETVOLUME:
            printf("WODM_SETVOLUME");
            {
                WAVE_DD_VOLUME Vol;
                Vol.Left = LOWORD(dwParam1) << 16;
                Vol.Right = HIWORD(dwParam1) << 16;

                return soundSetData(WaveOutDevice, dwId, sizeof(Vol),
                                  (PBYTE)&Vol, IOCTL_WAVE_SET_VOLUME);
            }

        case WODM_SETPLAYBACKRATE:
            printf("WODM_SETPLAYBACKRATE");
            ((PWAVEALLOC)dwUser)->AuxParam.GetSetData.pData = (PBYTE)&dwParam1;
            ((PWAVEALLOC)dwUser)->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            ((PWAVEALLOC)dwUser)->AuxParam.GetSetData.Function = IOCTL_WAVE_SET_PLAYBACK_RATE;
            return ThreadCallWaveDevice(WaveThreadSetData, (PWAVEALLOC)dwUser);

        case WODM_GETPITCH:
            printf("WODM_GETPITCH");
            ((PWAVEALLOC)dwUser)->AuxParam.GetSetData.pData = (PBYTE)dwParam1;
            ((PWAVEALLOC)dwUser)->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            ((PWAVEALLOC)dwUser)->AuxParam.GetSetData.Function = IOCTL_WAVE_GET_PITCH;
            return ThreadCallWaveDevice(WaveThreadGetData, (PWAVEALLOC)dwUser);

        case WODM_GETVOLUME:
            printf("WODM_GETVOLUME");
            {
                WAVE_DD_VOLUME Vol;
                DWORD res;

                res = soundGetData(WaveOutDevice, dwId, sizeof(Vol),
                                (PBYTE)&Vol, IOCTL_WAVE_GET_VOLUME);

                if (res == MMSYSERR_NOERROR)
                    *(LPDWORD)dwParam1 = (DWORD)MAKELONG(HIWORD(Vol.Left), HIWORD(Vol.Right));
                
                return res;
            }

        case WODM_GETPLAYBACKRATE:
            printf("WODM_GETPLAYBACKRATE");
            ((PWAVEALLOC)dwUser)->AuxParam.GetSetData.pData = (PBYTE)dwParam1;
            ((PWAVEALLOC)dwUser)->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            ((PWAVEALLOC)dwUser)->AuxParam.GetSetData.Function = IOCTL_WAVE_GET_PLAYBACK_RATE;
            return ThreadCallWaveDevice(WaveThreadGetData, (PWAVEALLOC)dwUser);

        default:
            return MMSYSERR_NOTSUPPORTED;
    }

	// This point of execution should never be reached
    return MMSYSERR_NOTSUPPORTED;
}


/*
 * @unimplemented
 */
APIENTRY DWORD widMessage(DWORD dwId, DWORD dwMessage, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
    printf("widMessage\n");

    switch (dwMessage) {
        case WIDM_GETNUMDEVS:
            return GetDeviceCount(WaveInDevice);
        default :
            return MMSYSERR_NOERROR;
    }
}

