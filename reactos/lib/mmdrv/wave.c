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

#define NDEBUG
#include <debug.h>

/* ============================
 *  INTERNAL
 *  functions start here
 * ============================
 */

MMRESULT GetDeviceCapabilities(DWORD ID, UINT DeviceType,
                                      LPBYTE pCaps, DWORD Size)
{
    HANDLE DeviceHandle = NULL;
    MMRESULT Result = MMSYSERR_NOERROR;
    DWORD BytesReturned = 0;

    // Open the wave device

    Result = OpenDevice(DeviceType, ID, &DeviceHandle, GENERIC_READ);
    if (Result != MMSYSERR_NOERROR)
         return Result;

	if ((DeviceType == WaveOutDevice) || (DeviceType == WaveInDevice))
	{
		Result = DeviceIoControl(DeviceHandle, IOCTL_WAVE_GET_CAPABILITIES,
                            NULL, 0, (LPVOID)pCaps, Size,
                  &BytesReturned, NULL) ? MMSYSERR_NOERROR : TranslateStatus();
	}

	else if ((DeviceType == MidiInDevice) || (DeviceType == MidiOutDevice))
	{
	    Result = DeviceIoControl(DeviceHandle, IOCTL_MIDI_GET_CAPABILITIES,
                            NULL, 0, (LPVOID)pCaps, Size,
                  &BytesReturned, NULL) ? MMSYSERR_NOERROR : TranslateStatus();
	}

    else if (DeviceType == AuxDevice)
	{
	    Result = DeviceIoControl(DeviceHandle, IOCTL_AUX_GET_CAPABILITIES,
                            NULL, 0, (LPVOID)pCaps, Size,
                  &BytesReturned, NULL) ? MMSYSERR_NOERROR : TranslateStatus();
	}

	
		

    // Close the handle and return the result code
    CloseHandle(DeviceHandle);

    return Result;
}

static MMRESULT OpenWaveDevice(UINT  DeviceType,
								DWORD id,
								DWORD dwUser,
								DWORD dwParam1,
								DWORD dwParam2)
{
	// TODO: Implement
    //PWAVEALLOC     pClient;  
    //MMRESULT mResult;
    //BOOL Result;
    //DWORD BytesReturned;
    LPWAVEFORMATEX pFormats;

    pFormats = (LPWAVEFORMATEX)((LPWAVEOPENDESC)dwParam1)->lpFormat;


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
    PWAVEALLOC pTask = (PWAVEALLOC)dwUser;

    switch (dwMessage) {
        case WODM_GETNUMDEVS:
            DPRINT("WODM_GETNUMDEVS");
            return GetDeviceCount(WaveOutDevice);

        case WODM_GETDEVCAPS:
            DPRINT("WODM_GETDEVCAPS");
            return GetDeviceCapabilities(dwId, WaveOutDevice, (LPBYTE)dwParam1,
                                  (DWORD)dwParam2);

        case WODM_OPEN:
            DPRINT("WODM_OPEN");
            return OpenWaveDevice(WaveOutDevice, dwId, dwUser, dwParam1, dwParam2);

        case WODM_CLOSE:
			{
				MMRESULT Result;				
				DPRINT("WODM_CLOSE");

				// 1. Check if the task is ready to complete
                pTask->AuxFunction = WaveThreadClose;
                SetEvent(pTask->AuxEvent1);
                WaitForSingleObject(pTask->AuxEvent2, INFINITE);
	            			
				if ( pTask->AuxReturnCode != MMSYSERR_NOERROR) 
                {
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

				DPRINT("WODM_WRITE");

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
            

            DPRINT("WODM_PAUSE");
            pTask->AuxParam.State = WAVE_DD_STOP;
           
            pTask->AuxFunction = WaveThreadSetState;
            SetEvent(pTask->AuxEvent1);
            WaitForSingleObject(pTask->AuxEvent2, INFINITE);
	        return pTask->AuxReturnCode;

        case WODM_RESTART:
            DPRINT("WODM_RESTART");
            pTask->AuxParam.State = WAVE_DD_PLAY;

            pTask->AuxFunction = WaveThreadSetState;
            SetEvent(pTask->AuxEvent1);
            WaitForSingleObject(pTask->AuxEvent2, INFINITE);
	        return pTask->AuxReturnCode;            

        case WODM_RESET:
            DPRINT("WODM_RESET");
            pTask->AuxParam.State = WAVE_DD_RESET;

            pTask->AuxFunction = WaveThreadSetState;
            SetEvent(pTask->AuxEvent1);
            WaitForSingleObject(pTask->AuxEvent2, INFINITE);
	        return pTask->AuxReturnCode;
           
        case WODM_BREAKLOOP:
            DPRINT("WODM_BREAKLOOP");

            pTask->AuxFunction = WaveThreadBreakLoop;
            SetEvent(pTask->AuxEvent1);
            WaitForSingleObject(pTask->AuxEvent2, INFINITE);
	        return pTask->AuxReturnCode;

        case WODM_GETPOS:
            DPRINT("WODM_GETPOS");
            return GetPositionWaveDevice(pTask, (LPMMTIME)dwParam1, dwParam2);

        case WODM_SETPITCH:
            DPRINT("WODM_SETPITCH");
            pTask->AuxParam.GetSetData.pData = (PBYTE)&dwParam1;
            pTask->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            pTask->AuxParam.GetSetData.Function = IOCTL_WAVE_SET_PITCH;
            
            pTask->AuxFunction = WaveThreadSetData;
            SetEvent(pTask->AuxEvent1);
            WaitForSingleObject(pTask->AuxEvent2, INFINITE);
	        return pTask->AuxReturnCode;

        case WODM_SETVOLUME:
            DPRINT("WODM_SETVOLUME");
            {
                WAVE_DD_VOLUME Vol;
                Vol.Left = LOWORD(dwParam1) << 16;
                Vol.Right = HIWORD(dwParam1) << 16;

                return soundSetData(WaveOutDevice, dwId, sizeof(Vol),
                                  (PBYTE)&Vol, IOCTL_WAVE_SET_VOLUME);
            }

        case WODM_SETPLAYBACKRATE:
            DPRINT("WODM_SETPLAYBACKRATE");
            pTask->AuxParam.GetSetData.pData = (PBYTE)&dwParam1;
            pTask->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            pTask->AuxParam.GetSetData.Function = IOCTL_WAVE_SET_PLAYBACK_RATE;
            
            pTask->AuxFunction = WaveThreadSetData;
            SetEvent(pTask->AuxEvent1);
            WaitForSingleObject(pTask->AuxEvent2, INFINITE);
	        return pTask->AuxReturnCode;


        case WODM_GETPITCH:
            DPRINT("WODM_GETPITCH");
            pTask->AuxParam.GetSetData.pData = (PBYTE)dwParam1;
            pTask->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            pTask->AuxParam.GetSetData.Function = IOCTL_WAVE_GET_PITCH;
            
            pTask->AuxFunction = WaveThreadGetData;
            SetEvent(pTask->AuxEvent1);
            WaitForSingleObject(pTask->AuxEvent2, INFINITE);
	        return pTask->AuxReturnCode;

        case WODM_GETVOLUME:
            DPRINT("WODM_GETVOLUME");
            {
                WAVE_DD_VOLUME Vol = {};
                DWORD res;

                res = soundGetData(WaveOutDevice, dwId, sizeof(Vol),
                                (PBYTE)&Vol, IOCTL_WAVE_GET_VOLUME);

                if (res == MMSYSERR_NOERROR)
                    *(LPDWORD)dwParam1 = (DWORD)MAKELONG(HIWORD(Vol.Left), HIWORD(Vol.Right));
                
                return res;
            }

        case WODM_GETPLAYBACKRATE:
            DPRINT("WODM_GETPLAYBACKRATE");
            pTask->AuxParam.GetSetData.pData = (PBYTE)dwParam1;
            pTask->AuxParam.GetSetData.DataLen = sizeof(DWORD);
            pTask->AuxParam.GetSetData.Function = IOCTL_WAVE_GET_PLAYBACK_RATE;
                         
            pTask->AuxFunction = WaveThreadGetData;
            SetEvent(pTask->AuxEvent1);
            WaitForSingleObject(pTask->AuxEvent2, INFINITE);
	        return pTask->AuxReturnCode;

        default:
            return MMSYSERR_NOTSUPPORTED;
    }

	// This point of execution should never be reached
    return MMSYSERR_NOTSUPPORTED;
}


/*
 * @implemented
 */
APIENTRY DWORD widMessage(DWORD dwId, DWORD dwMessage, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
    DPRINT("widMessage\n");

    switch (dwMessage) 
    {
        case WIDM_GETNUMDEVS: 
            DPRINT("WIDM_GETNUMDEVS");
            return GetDeviceCount(WaveInDevice);

        case WIDM_GETDEVCAPS:
            DPRINT("WODM_GETDEVCAPS");
            return GetDeviceCapabilities(dwId, WaveInDevice, (LPBYTE)dwParam1, (DWORD)dwParam2);
             
        case WIDM_OPEN:
            DPRINT("WIDM_OPEN");
            return OpenWaveDevice(WaveInDevice, dwId, dwUser, dwParam1, dwParam2);

        case WIDM_CLOSE:
             return MMSYSERR_NOERROR;

        case WIDM_ADDBUFFER:
             return MMSYSERR_NOERROR;

        case WIDM_STOP:
             return MMSYSERR_NOERROR;

        case WIDM_START:
             return MMSYSERR_NOERROR;

        case WIDM_RESET:
             return MMSYSERR_NOERROR;

        case WIDM_GETPOS:
             return MMSYSERR_NOERROR;


        default :
             return MMSYSERR_NOTSUPPORTED;
    }
}

