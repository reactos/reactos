/*
 *  WinMM (midiout.c) : MIDI output related functions
 *
 *  [8-18-2003] AG: Started adding stubs and implemented a few functions
*/

#include <windows.h>
typedef UINT *LPUINT;
#include <mmsystem.h>
#include "winmm.h"

#define NDEBUG
#include <debug.h>


#define IsValidMidiOutHandle(hmo) \
    (((LPMidiOutHandleInfo*)hmo < mo_HandleInfo) || \
     ((LPMidiOutHandleInfo*)hmo >= mo_HandleInfo + (mo_HandleCount * sizeof(MidiOutHandleInfo))))


LPMidiOutDeviceInfo *mo_DeviceInfo = NULL;
UINT mo_DeviceCount = 0;
LPMidiOutHandleInfo *mo_HandleInfo = NULL;
UINT mo_HandleCount = 0;


/* ------------------------------------------------------------------------- */

MMRESULT WINAPI midiOutOpen(
    LPHMIDIOUT lphmo,
    UINT uDeviceID,
    DWORD dwCallback,
    DWORD dwCallbackInstance,
    DWORD dwFlags)
{
    // TODO: Add device open checking and return MMSYSERR_ALLOCATED, but what
    // happens for multi-client drivers?
    // Also, MIDI_MAPPER needs to be implemented somehow...

    MidiOutHandleInfo *Info = NULL;
    int i;
    
    if (! lphmo)
        return MMSYSERR_INVALPARAM;

    if ((uDeviceID >= mo_DeviceCount) && (uDeviceID != MIDI_MAPPER))
        return MMSYSERR_BADDEVICEID;

    // Make sure we have a callback address if a callback is desired
    if ((! dwCallback) && (dwFlags != CALLBACK_NULL))
        return MMSYSERR_INVALPARAM;


    // Check existing handles to see if one is free
    for (i = 0; i < mo_HandleCount; i ++)
        if (! mo_HandleInfo[i]->IsOpen)
        {
            Info = mo_HandleInfo[i];
            break;
        }

    // Allocate a new handle info structure
    if (! Info)
    {
        mo_HandleCount ++;
        
        LPMidiOutHandleInfo *Old = mo_HandleInfo;

        // This was before I added file mapping stuff
//        if (! mo_HandleInfo)
//            mo_HandleInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(MidiOutHandleInfo) * mo_HandleCount);
//        else
//            mo_HandleInfo = HeapReAlloc(GetProcessHeap(), 0, mo_HandleInfo, sizeof(MidiOutHandleInfo) * mo_HandleCount);
            
        if (! mo_HandleInfo)
        {
            mo_HandleCount --;
            mo_HandleInfo = Old;
            return MMSYSERR_NOMEM;  // Correct?
        }
        
        Info = mo_HandleInfo[mo_HandleCount - 1];
    }

    Info->DeviceID = uDeviceID;

    // Pretend we opened OK (really need to query device driver)
    Info->IsOpen = TRUE;
    
    if (Info->IsOpen)
    {
        LPMIDICALLBACK mo_Proc = (LPMIDICALLBACK) dwCallback;

        switch(dwFlags)
        {
            case CALLBACK_FUNCTION :
                mo_Proc((HMIDIOUT) Info, MM_MOM_OPEN, dwCallbackInstance, 0, 0);
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
    *lphmo = (HMIDIOUT) Info;

    return MMSYSERR_NOERROR;
}


MMRESULT WINAPI midiOutClose(HMIDIOUT hmo)
{
    LPMidiOutHandleInfo Info = NULL;

    if (IsValidMidiOutHandle(hmo))
        return MMSYSERR_INVALHANDLE;
    
    // Check if buffers still queued and return MIDIERR_STILLPLAYING if so...
    // TODO
    
    Info = (LPMidiOutHandleInfo) hmo;
    Info->IsOpen = FALSE;
    
    return MMSYSERR_NOERROR;
}


MMRESULT WINAPI midiOutGetDevCaps(
    UINT uDeviceID,
    LPMIDIOUTCAPS lpMidiOutCaps,
    UINT cbMidiOutCaps)
{
    UNIMPLEMENTED;
    return MMSYSERR_NOERROR;
}


MMRESULT WINAPI midiOutGetErrorText(
    MMRESULT mmrError,
    LPSTR lpText,
    UINT cchText)
{
    if (! cchText)
        return MMSYSERR_NOERROR;

    if (! lpText)
        return MMSYSERR_INVALPARAM;

    if (((mmrError >= MMSYSERR_BASE) && (mmrError <= MMSYSERR_LASTERROR)) ||
        ((mmrError >= MIDIERR_BASE) && (mmrError <= MIDIERR_LASTERROR)))
    {
//        LoadString(GetModuleHandle(NULL), mmrError, lpText, cchText);  // bytes/chars?
        return MMSYSERR_NOERROR;
    }
    else
        return MMSYSERR_BADERRNUM;
}


MMRESULT WINAPI midiOutGetID(
    HMIDIOUT hmo,
    LPUINT puDeviceID)
{
    // What triggers MMSYSERR_NODRIVER and MMSYSERR_NOMEM error codes?

    LPMidiOutHandleInfo Info = NULL;
    
    if (! puDeviceID)
        return MMSYSERR_INVALPARAM;

    if (IsValidMidiOutHandle(hmo))
        return MMSYSERR_INVALHANDLE;
    
    Info = (LPMidiOutHandleInfo) hmo;
    *puDeviceID = Info->DeviceID;

    return MMSYSERR_NOERROR;
}


UINT WINAPI midiOutGetNumDevs(VOID)
{
    // +1 for MIDI_MAPPER :
    return mo_DeviceCount ? mo_DeviceCount + 1 : 0;
}


MMRESULT WINAPI midiOutSetVolume(
    HMIDIOUT hmo,
    DWORD dwVolume)
{
    if (IsValidMidiOutHandle(hmo))
        return MMSYSERR_INVALHANDLE;

    UNIMPLEMENTED;
    return MMSYSERR_NOERROR;
}


MMRESULT WINAPI midiOutGetVolume(
    HMIDIOUT hmo,
    LPDWORD lpdwVolume)
{
    if (IsValidMidiOutHandle(hmo))
        return MMSYSERR_INVALHANDLE;

    UNIMPLEMENTED;
    return MMSYSERR_NOERROR;
}


MMRESULT WINAPI midiOutLongMsg(
    HMIDIOUT hmo,
    LPMIDIHDR lpMidiOutHdr,
    UINT cbMidiOutHdr)
{
    if (IsValidMidiOutHandle(hmo))
        return MMSYSERR_INVALHANDLE;

    UNIMPLEMENTED;
    return MMSYSERR_NOERROR;
}


// This is *supposed* to return a DWORD:
MMRESULT WINAPI midiOutMessage(
    HMIDIOUT hmo,
    UINT msg,
    DWORD dw1,
    DWORD dw2)
{
    if (IsValidMidiOutHandle(hmo))
        return MMSYSERR_INVALHANDLE;

    UNIMPLEMENTED;
    return 0;
}


MMRESULT WINAPI midiOutPrepareHeader(
    HMIDIOUT hmo,
    LPMIDIHDR lpMidiOutHdr,
    UINT cbMidiOutHdr)
{
    if (IsValidMidiOutHandle(hmo))
        return MMSYSERR_INVALHANDLE;

    if (! lpMidiOutHdr)
        return MMSYSERR_INVALPARAM;
        
    if (! lpMidiOutHdr->lpData)
        return MMSYSERR_INVALPARAM;

    // Yup, we're prepared! dwFlags must be 0 to begin with, so no | needed:
    lpMidiOutHdr->dwFlags = MHDR_PREPARED;

    return MMSYSERR_NOERROR;
}


MMRESULT WINAPI midiOutUnprepareHeader(
    HMIDIOUT hmo,
    LPMIDIHDR lpMidiOutHdr,
    UINT cbMidiOutHdr)
{
    if (IsValidMidiOutHandle(hmo))
        return MMSYSERR_INVALHANDLE;

    if (! lpMidiOutHdr)
        return MMSYSERR_INVALPARAM;
        
    if (! lpMidiOutHdr->lpData)
        return MMSYSERR_INVALPARAM;

    // We're unprepared! Clear the MHDR_PREPARED flag:
    lpMidiOutHdr->dwFlags &= ! MHDR_PREPARED;

    return MMSYSERR_NOERROR;
}


MMRESULT WINAPI midiOutReset(HMIDIOUT hmo)
{
    if (IsValidMidiOutHandle(hmo))
        return MMSYSERR_INVALHANDLE;

    UNIMPLEMENTED;
    return MMSYSERR_NOERROR;
}


MMRESULT WINAPI midiOutShortMsg(
    HMIDIOUT hmo,
    DWORD dwMsg)
{
    if (IsValidMidiOutHandle(hmo))
        return MMSYSERR_INVALHANDLE;

    UNIMPLEMENTED;
    return MMSYSERR_NOERROR;
}


MMRESULT WINAPI midiOutCacheDrumPatches(
    HMIDIOUT hmo,
    UINT wPatch,
    WORD* lpKeyArray,
    UINT wFlags)
{
    UNIMPLEMENTED;
    return MMSYSERR_NOERROR;
}


MMRESULT WINAPI midiOutCachePatches(
    HMIDIOUT hmo,
    UINT wBank,
    WORD* lpPatchArray,
    UINT wFlags)
{
    UNIMPLEMENTED;
    return MMSYSERR_NOERROR;
}


/* ------------------------------------------------------------------------- */

void mo_Init()
{
    // Internal routine for initializing MIDI-Out related stuff

    BOOL First;
    HANDLE modi_fm, mohi_fm;
    
//    modi_fm = CreateFileMapping((HANDLE)0xFFFFFFFF, (LPSECURITY_ATTRIBUTES)NULL,
//                            PAGE_READWRITE, 0, 64 * 1024, FM_MIDI_OUT_DEV_INFO);

    First = ! GetLastError();

//    mo_DeviceInfo = MapViewOfFile(modi_fm, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SomeStruct)
    
    if (First)
    {
        mo_DeviceInfo = MapViewOfFile(modi_fm, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MidiOutDeviceInfo));

        // Just set up a fake device for now
        mo_DeviceCount ++;  // need mapping for this
//        mo_DeviceInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(MidiOutDeviceInfo) * mo_DeviceCount);
        // Set info now
        
        UnmapViewOfFile(mo_DeviceInfo);
    }

//    mohi_fm = CreateFileMapping((HANDLE)0xFFFFFFFF, (LPSECURITY_ATTRIBUTES)NULL,
//                           PAGE_READWRITE, 0, 64 * 1024, FM_MIDI_OUT_HANDLE_INFO);
}
