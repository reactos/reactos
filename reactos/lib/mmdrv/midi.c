/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/mmdrv/midi.c
 * PURPOSE:              Multimedia User Mode Driver
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Jan 30, 2004: Imported into ReactOS tree
 */

#include "mmdrv.h"


// MIDI device instance information
//

typedef struct tag_MIDIALLOC {
    struct tag_MIDIALLOC *Next;         // Chain of devices
    UINT                DeviceNumber;   // Number of device
    UINT                DeviceType;     // MidiInput or MidiOutput
    DWORD               dwCallback;     // client's callback
    DWORD               dwInstance;     // client's instance data
    HMIDI               hMidi;          // handle for stream
    HANDLE              DeviceHandle;   // Midi device handle
    LPMIDIHDR           lpMIQueue;      // Buffers sent to device
                                        // This is only required so that
                                        // CLOSE knows when things have
                                        // really finished.
                                        // notify.  This is only accessed
                                        // on the device thread and its
                                        // apcs so does not need any
                                        // synchronized access.
    HANDLE              Event;          // Event for driver syncrhonization
                                        // and notification of auxiliary
                                        // task operation completion.
//    MIDITHREADFUNCTION  AuxFunction;    // Function for thread to perform
    union {
        LPMIDIHDR       pHdr;           // Buffer to pass in aux task
        ULONG           State;          // State to set
        struct {
            ULONG       Function;       // IOCTL to use
            PBYTE       pData;          // Data to set or get
            ULONG       DataLen;        // Length of data
        } GetSetData;

    } AuxParam;
                                        // 0 means terminate task.
    HANDLE              ThreadHandle;   // Handle for termination ONLY
    HANDLE              AuxEvent1;      // Aux thread waits on this
    HANDLE              AuxEvent2;      // Aux thread caller waits on this
    DWORD               AuxReturnCode;  // Return code from Aux task
    DWORD               dwFlags;        // Open flags
//    PLOCALMIDIDATA      Mid;            // Extra midi input structures
    int                 l;              // Helper global for modMidiLength

} MIDIALLOC, *PMIDIALLOC;

PMIDIALLOC MidiHandleList;              // Our chain of wave handles



static DWORD OpenMidiDevice(UINT DeviceType, DWORD ID, DWORD User, DWORD Param1, DWORD Param2)
{
    PMIDIALLOC pClient = NULL;
    MMRESULT Result = MMSYSERR_NOERROR;
    
    // Check ID?
    printf("OpenMidiDevice()\n");
    
    switch(DeviceType)
    {
        case MidiOutDevice :
            pClient = (PMIDIALLOC) HeapAlloc(Heap, 0, sizeof(MIDIALLOC));
            if ( pClient ) memset(pClient, 0, sizeof(MIDIALLOC));
            break;
        
        case MidiInDevice :
            // TODO
            break;
    };
    
    if ( !pClient )
        return MMSYSERR_NOMEM;
        
    pClient->DeviceType = DeviceType;
    pClient->dwCallback = ((LPMIDIOPENDESC)Param1)->dwCallback;
    pClient->dwInstance = ((LPMIDIOPENDESC)Param1)->dwInstance;
    pClient->hMidi = ((LPMIDIOPENDESC)Param1)->hMidi;
    pClient->dwFlags = Param2;
    
    Result = OpenDevice(DeviceType, ID, &pClient->DeviceHandle, (GENERIC_READ | GENERIC_WRITE));
    
    if ( Result != MMSYSERR_NOERROR )
    {
        // cleanup
        return Result;
    }
    
    pClient->Event = CreateEvent(NULL, FALSE, FALSE, NULL);
    
    if ( !pClient->Event )
    {
        // cleanup
        return MMSYSERR_NOMEM;
    }

    PMIDIALLOC *pUserHandle;
    pUserHandle = (PMIDIALLOC*) User;
    *pUserHandle = pClient;
    
        // callback    
    return MMSYSERR_NOERROR;
}



static DWORD WriteMidi(PBYTE pData, ULONG Length, PMIDIALLOC pClient)
{
    DWORD BytesReturned;
    
    if ( !DeviceIoControl(pClient->DeviceHandle, IOCTL_MIDI_PLAY, (PVOID)pData,
                          Length, NULL, 0, &BytesReturned, NULL))
        return TranslateStatus();

    return MMSYSERR_NOERROR;
}


static int GetMidiLength(PMIDIALLOC pClient, BYTE b)
{
    if (b >= 0xF8)
    {
        // Realtime message - leave running status
        return 1; // Write one byte
    }

    switch (b)
    {
        case 0xF0: case 0xF4: case 0xF5: case 0xF6: case 0xF7:
            pClient->l = 1;
            return pClient->l;

        case 0xF1: case 0xF3:
            pClient->l = 2;
            return pClient->l;

        case 0xF2:
            pClient->l = 3;
            return pClient->l;
    }

    switch (b & 0xF0)
    {
        case 0x80: case 0x90: case 0xA0: case 0xB0: case 0xE0:
            pClient->l = 3;
            return pClient->l;

        case 0xC0: case 0xD0:
            pClient->l = 2;
            return pClient->l;
    }

    return (pClient->l - 1); // uses previous value if data byte (running status)
}



/* ----------------------------------------------------------------------------
    Exported functions
----------------------------------------------------------------------------- */

APIENTRY DWORD midMessage(DWORD ID, DWORD Message, DWORD User, DWORD Param1, DWORD Param2)
{
    printf("midMessage\n");
    return MMSYSERR_NOERROR;
}

APIENTRY DWORD modMessage(DWORD ID, DWORD Message, DWORD User, DWORD Param1, DWORD Param2)
{
    printf("modMessage\n");
    
    switch(Message)
    {
        case MODM_GETNUMDEVS:
            printf("MODM_GETNUMDEVS == %d\n", (int)GetDeviceCount(MidiOutDevice));
            return GetDeviceCount(MidiOutDevice);
        
        case MODM_GETDEVCAPS:
            printf("MODM_GETDEVCAPS");
            return MMSYSERR_NOTSUPPORTED;
            
        case MODM_OPEN :
            return OpenMidiDevice(MidiOutDevice, ID, User, Param1, Param2);

        case MODM_CLOSE:
            printf("MODM_CLOSE");
            return MMSYSERR_NOTSUPPORTED;

        case MODM_DATA:
            printf("MODM_DATA");

            int i;
            BYTE b[4];
            for (i = 0; i < 4; i ++) {
                b[i] = (BYTE)(Param1 % 256);
                Param1 /= 256;
            }
            return WriteMidi(b, GetMidiLength((PMIDIALLOC)User, b[0]),
                                (PMIDIALLOC)User);

        case MODM_LONGDATA:
            printf("MODM_LONGDATA");
            return MMSYSERR_NOTSUPPORTED;

        case MODM_RESET:
            printf("MODM_RESET");
            return MMSYSERR_NOTSUPPORTED;

        case MODM_SETVOLUME:
            printf("MODM_SETVOLUME");
            return MMSYSERR_NOTSUPPORTED;

        case MODM_GETVOLUME:
            printf("MODM_GETVOLUME");
            return MMSYSERR_NOTSUPPORTED;

        case MODM_CACHEPATCHES:
            printf("MODM_CACHEPATCHES");
            return MMSYSERR_NOTSUPPORTED;

        case MODM_CACHEDRUMPATCHES:
            printf("MODM_CACHEDRUMPATCHES");
            return MMSYSERR_NOTSUPPORTED;
            
    };

    return MMSYSERR_NOTSUPPORTED;
}
