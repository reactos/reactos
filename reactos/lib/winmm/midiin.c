/*
 *  WinMM (midiin.c) : MIDI input related functions
 *
 *  [8-18-2003] AG: Started adding stubs and implemented a few functions
*/

#include <windows.h>
typedef UINT *LPUINT;
#include <mmsystem.h>

#define NDEBUG
#include <debug.h>


#define IsValidMidiInHandle(hmi) \
    (((LPMidiInHandleInfo*)hmi < mi_HandleInfo) || \
     ((LPMidiInHandleInfo*)hmi >= mi_HandleInfo + (mi_HandleCount * sizeof(MidiInHandleInfo))))


typedef struct MidiInDeviceInfo
{
    BOOL IsOpen;    // Correct?
} MidiInDeviceInfo, *LPMidiInDeviceInfo;

LPMidiInDeviceInfo *mi_DeviceInfo = NULL;
UINT mi_DeviceCount = 0;


typedef struct MidiInHandleInfo
{
    UINT DeviceID;  // Needs to be first
    BOOL IsOpen;
} MidiInHandleInfo, *LPMidiInHandleInfo;


// Array of MidiInHandleInfo structures
LPMidiInHandleInfo *mi_HandleInfo = NULL;
UINT mi_HandleCount = 0;


/* ------------------------------------------------------------------------- */

MMRESULT WINAPI midiInOpen(
    LPHMIDIIN lphMidiIn,
    UINT uDeviceID,
    DWORD dwCallback,
    DWORD dwCallbackInstance,
    DWORD dwFlags)
{
    // TODO: Add device open checking and return MMSYSERR_ALLOCATED, but what
    // happens for multi-client drivers?

    MidiInHandleInfo *Info = NULL;
    int i;
    
    if (! lphMidiIn)
        return MMSYSERR_INVALPARAM;

    if ((uDeviceID >= mi_DeviceCount) && (uDeviceID != MIDI_MAPPER))
        return MMSYSERR_BADDEVICEID;

    // Make sure we have a callback address if a callback is desired
    if ((! dwCallback) && (dwFlags != CALLBACK_NULL))
        return MMSYSERR_INVALPARAM;


    // Check existing handles to see if one is free
    for (i = 0; i < mi_HandleCount; i ++)
        if (! mi_HandleInfo[i]->IsOpen)
        {
            Info = mi_HandleInfo[i];
            break;
        }

    // Allocate a new handle info structure
    if (! Info)
    {
        mi_HandleCount ++;
        
        LPMidiInHandleInfo *Old = mi_HandleInfo;

        // File mapping stuff to replace this needed:
//        if (! mi_HandleInfo)
//            mi_HandleInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(MidiInHandleInfo) * mi_HandleCount);
//        else
//            mi_HandleInfo = HeapReAlloc(GetProcessHeap(), 0, mi_HandleInfo, sizeof(MidiInHandleInfo) * mi_HandleCount);
            
        if (! mi_HandleInfo)
        {
            mi_HandleCount --;
            mi_HandleInfo = Old;
            return MMSYSERR_NOMEM;  // Correct?
        }
        
        Info = mi_HandleInfo[mi_HandleCount - 1];
    }

    Info->DeviceID = uDeviceID;

    // Pretend we opened OK (really need to query device driver)
    Info->IsOpen = TRUE;
    
    if (Info->IsOpen)
    {
        LPMIDICALLBACK mi_Proc = (LPMIDICALLBACK) dwCallback;

        switch(dwFlags)
        {
            case CALLBACK_FUNCTION :
                mi_Proc((HMIDIIN) Info, MM_MOM_OPEN, dwCallbackInstance, 0, 0);
                break;

            case CALLBACK_EVENT :
                // Do something
                break;

            case CALLBACK_THREAD :
                // Do something
                break;

            case CALLBACK_WINDOW :
                // Do something
                break;
        }
    }
    
    else
        return MMSYSERR_ERROR;  // Correct if can't be opened?

    // Copy the handle (really a pointer to Info):
    *lphMidiIn = (HMIDIIN) Info;

    return MMSYSERR_NOERROR;
}


MMRESULT WINAPI midiInClose(HMIDIIN hMidiIn)
{
    LPMidiInHandleInfo Info = NULL;

    if (IsValidMidiInHandle(hMidiIn))
        return MMSYSERR_INVALHANDLE;
    
    // Check if buffers still queued and return MIDIERR_STILLPLAYING if so...
    // TODO
    
    Info = (LPMidiInHandleInfo) hMidiIn;
    Info->IsOpen = FALSE;
    
    return MMSYSERR_NOERROR;
}


/* ------------------------------------------------------------------------- */

void mi_Init()
{
    // Internal routine for initializing MIDI-Out related stuff

    // Just set up a fake device for now
    // FILE MAPPING!
//    mi_DeviceCount ++;
//    HeapAlloc(GetProcessHeap(), 0, sizeof(MidiInDeviceInfo) * mi_DeviceCount);
}
