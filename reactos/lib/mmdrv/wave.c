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

PWAVEALLOC WaveLists;   

/* ============================
 *  INTERNAL
 *  functions start here
 * ============================
 */
static DWORD waveThread(LPVOID lpParameter)
{
  return MMSYSERR_NOERROR;
}

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
    PWAVEALLOC     pClient = (PWAVEALLOC)dwUser;  
    MMRESULT mResult;
    BOOL Result;
    DWORD BytesReturned;
    LPWAVEFORMATEX pFormats;
    PWAVEALLOC *pUserHandle;
    HANDLE hDevice;

    pFormats = (LPWAVEFORMATEX)((LPWAVEOPENDESC)dwParam1)->lpFormat;

    if (dwParam2 & WAVE_FORMAT_QUERY) 
    {        
        mResult = OpenDevice(DeviceType, id, &hDevice, GENERIC_READ);
        if (mResult != MMSYSERR_NOERROR) 
            return mResult;
        
        Result = DeviceIoControl(hDevice, IOCTL_WAVE_QUERY_FORMAT, (PVOID)pFormats,
                                 pFormats->wFormatTag == WAVE_FORMAT_PCM ?
                                 sizeof(PCMWAVEFORMAT) : sizeof(WAVEFORMATEX) + pFormats->cbSize,                        
                                 NULL, 0, &BytesReturned, NULL);
        CloseHandle(hDevice);
        return Result ? MMSYSERR_NOERROR : GetLastError() == ERROR_NOT_SUPPORTED ? WAVERR_BADFORMAT : TranslateStatus();
    }

    EnterCriticalSection(&CS);

    for (pClient = WaveLists; pClient != NULL; pClient = pClient->Next) 
    {
        if (pClient->DeviceNumber == id && pClient->DeviceType == DeviceType) 
        {        
            if (pClient->hDev != INVALID_HANDLE_VALUE) 
            {
                LeaveCriticalSection(&CS);
                return MMSYSERR_ALLOCATED;
            }
            break;
        }
    }

    if (pClient == NULL) 
    {
        pClient = (PWAVEALLOC)HeapAlloc(Heap, HEAP_ZERO_MEMORY, sizeof(WAVEALLOC));
        if (pClient == NULL) 
        {
            LeaveCriticalSection(&CS);
            return MMSYSERR_NOMEM;
        }
        
        pClient->DeviceNumber = id;
        pClient->Next = WaveLists;
        pClient->DeviceType = DeviceType;
        
        WaveLists = pClient;
    }

    pClient->hWave       = ((LPWAVEOPENDESC)dwParam1)->hWave;
    pClient->dwInstance  = ((LPWAVEOPENDESC)dwParam1)->dwInstance;
    pClient->dwFlags     = dwParam2;
    pClient->dwCallback  = ((LPWAVEOPENDESC)dwParam1)->dwCallback;
    pClient->hDev = INVALID_HANDLE_VALUE;
    pClient->NextBuffer  = NULL;
    pClient->DeviceQueue = NULL;
    pClient->LoopHead    = NULL;
    pClient->LoopCount   = 0;
    pClient->BytesOutstanding = 0;
    pClient->BufferPosition = 0;
    
    
    

    mResult = OpenDevice(DeviceType, id, &pClient->hDev, (GENERIC_READ | GENERIC_WRITE));

    if (mResult != MMSYSERR_NOERROR) 
    {    
        LeaveCriticalSection(&CS);
        return mResult;
    }

     if (!DeviceIoControl(pClient->hDev,IOCTL_WAVE_SET_FORMAT, (PVOID)pFormats,
                             pFormats->wFormatTag == WAVE_FORMAT_PCM ?
                             sizeof(PCMWAVEFORMAT) : sizeof(WAVEFORMATEX) + pFormats->cbSize,
                             NULL, 0, &BytesReturned, NULL))
    {
        CloseHandle(pClient->hDev);
        pClient->hDev = INVALID_HANDLE_VALUE;
        LeaveCriticalSection(&CS);
        return GetLastError() == ERROR_NOT_SUPPORTED ? WAVERR_BADFORMAT : TranslateStatus();
    }

    LeaveCriticalSection(&CS);

    if (!pClient->Event) 
    {
        pClient->Event = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pClient->Event == NULL) 
        {
            // Cleanup
            return MMSYSERR_NOMEM;
        }

        pClient->AuxEvent1 = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!pClient->AuxEvent1) 
        {
           // Cleanup
            return MMSYSERR_NOMEM;
        }

        pClient->AuxEvent2 = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!pClient->AuxEvent2) 
        {
            // Cleanup
            return MMSYSERR_NOMEM;
        }


        mResult = mmTaskCreate((LPTASKCALLBACK)waveThread, &pClient->ThreadHandle, (DWORD)pClient);
        if ( mResult != MMSYSERR_NOERROR) 
        {
            // Cleanup
            return MMSYSERR_NOMEM;
        }

        WaitForSingleObject(pClient->AuxEvent2, INFINITE);
     }
     
     *pUserHandle = pClient;
      pUserHandle = (PWAVEALLOC *)dwUser;
   

     if (pClient->dwCallback)
     {
        DriverCallback(pClient->dwCallback, HIWORD(pClient->dwFlags),
                       (HDRVR)pClient->hWave,  DeviceType == WaveOutDevice ? WOM_OPEN : WIM_OPEN, 
                       pClient->dwInstance, 0L, 0L);                 
     }

	return MMSYSERR_NOERROR;
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
				DPRINT("WODM_CLOSE");

				// 1. Check if the task is ready to complete
                pTask->AuxFunction = WaveThreadClose;
                SetEvent(pTask->AuxEvent1);
                WaitForSingleObject(pTask->AuxEvent2, INFINITE);
	            			
				if ( pTask->AuxReturnCode != MMSYSERR_NOERROR) 
                {
				    return pTask->AuxReturnCode;
				}
				else
					
                {
                    if (pTask->dwCallback)
                    {
                        DriverCallback(pTask->dwCallback, HIWORD(pTask->dwFlags), (HDRVR)pTask->hWave, 
                                       WOM_CLOSE, pTask->dwInstance, 0L, 0L);                   
                    }
                }

				
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

