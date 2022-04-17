/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 lib/mmdrv/utils.c
 * PURPOSE:              Multimedia User Mode Driver (utility functions)
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Jan 30, 2004: Imported into ReactOS tree
 */

#include "mmdrv.h"

#define NDEBUG
#include <debug.h>

typedef struct _DEVICE_LIST
{
    struct _DEVICE_LIST *Next;
    DWORD   DeviceType;
    ULONG   CardIndex;
    PVOID   DeviceInstanceData;
    ULONG   DeviceInstanceDataSize;
    WCHAR   Name[1];
} DEVICE_LIST, *PDEVICE_LIST;

PDEVICE_LIST DeviceList;


DWORD TranslateStatus(void)
{
    switch(GetLastError())
    {
        case NO_ERROR :
        case ERROR_IO_PENDING :
            return MMSYSERR_NOERROR;

        case ERROR_BUSY :
            return MMSYSERR_ALLOCATED;

        case ERROR_NOT_SUPPORTED :
        case ERROR_INVALID_FUNCTION :
            return MMSYSERR_NOTSUPPORTED;

        case ERROR_NOT_ENOUGH_MEMORY :
            return MMSYSERR_NOMEM;

        case ERROR_ACCESS_DENIED :
            return MMSYSERR_BADDEVICEID;

        case ERROR_INSUFFICIENT_BUFFER :
            return MMSYSERR_INVALPARAM;

        default :
            return MMSYSERR_ERROR;
    };
}



MMRESULT OpenDevice(UINT DeviceType, DWORD ID, PHANDLE pDeviceHandle,
                    DWORD Access)
{
    DPRINT("OpenDevice()\n");
    WCHAR DeviceName[SOUND_MAX_DEVICE_NAME];
    *pDeviceHandle = INVALID_HANDLE_VALUE;

    if (ID > SOUND_MAX_DEVICES)
        return MMSYSERR_BADDEVICEID;

    switch(DeviceType)
    {
        case WaveOutDevice :
            wsprintf(DeviceName, L"\\\\.%ls%d", WAVE_OUT_DEVICE_NAME_U + strlen("\\Device"), ID);
            break;
        case WaveInDevice :
            wsprintf(DeviceName, L"\\\\.%ls%d", WAVE_IN_DEVICE_NAME_U + strlen("\\Device"), ID);
            break;
        case MidiOutDevice :
            wsprintf(DeviceName, L"\\\\.%ls%d", MIDI_OUT_DEVICE_NAME_U + strlen("\\Device"), ID);
            break;
        case MidiInDevice :
            wsprintf(DeviceName, L"\\\\.%ls%d", MIDI_IN_DEVICE_NAME_U + strlen("\\Device"), ID);
            break;
		case AuxDevice :
			 wsprintf(DeviceName, L"\\\\.%ls%d", AUX_DEVICE_NAME_U + strlen("\\Device"), ID);
			 break;
        default :
            DPRINT("No Auido Device Found");
            return MMSYSERR_BADDEVICEID; /* Maybe we should change error code */
    };

    DPRINT("Attempting to open %S\n", DeviceName);

    *pDeviceHandle = CreateFile(DeviceName, Access, FILE_SHARE_WRITE, NULL,
                                OPEN_EXISTING, Access != GENERIC_READ ? FILE_FLAG_OVERLAPPED : 0,
                                NULL);

    DPRINT("DeviceHandle == 0x%x\n", (int)*pDeviceHandle);

    if (pDeviceHandle == INVALID_HANDLE_VALUE)
        return TranslateStatus();

    return MMSYSERR_NOERROR;
}


// DEVICE LIST MANAGEMENT


BOOL AddDeviceToList(PDEVICE_LIST* pList, DWORD DeviceType, DWORD CardIndex,
                     LPWSTR Name)
{
    PDEVICE_LIST pNewDevice;

    DPRINT("AddDeviceToList()\n");

    pNewDevice = (PDEVICE_LIST) HeapAlloc(Heap, 0,
        sizeof(DEVICE_LIST) + lstrlen(Name) * sizeof(WCHAR));

    if ( !pNewDevice )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pNewDevice->DeviceType = DeviceType;
    pNewDevice->CardIndex = CardIndex;
    lstrcpy(pNewDevice->Name, Name);
    pNewDevice->DeviceInstanceData = NULL;
    pNewDevice->Next = *pList;
    *pList = pNewDevice;

    DPRINT("Success!\n");

    return TRUE;
}


VOID FreeDeviceList()
{
    PDEVICE_LIST pDevice;

    DPRINT("FreeDeviceList()\n");

    while (DeviceList)
    {
        pDevice = DeviceList;
        DeviceList = pDevice->Next;

        if (pDevice->DeviceInstanceData)
            HeapFree(Heap, 0, (LPVOID)pDevice->DeviceInstanceData);

        HeapFree(Heap, 0, (LPSTR)pDevice);
    }
}


MMRESULT FindDevices()
{
//    DWORD Index;
//    HKEY DriverKey;

    DPRINT("Finding devices\n");

//    DriverKey = OpenParametersKey();
//  see drvutil.c of MS DDK for how this SHOULD be done...


    SHORT i;
    HANDLE h;
    WCHAR DeviceName[SOUND_MAX_DEVICE_NAME];

    for (i=0; OpenDevice(WaveOutDevice, i, &h, GENERIC_READ) == MMSYSERR_NOERROR; i++)
    {
        wsprintf(DeviceName, L"WaveOut%d\0", i);
        CloseHandle(h);
        AddDeviceToList(&DeviceList, WaveOutDevice, 0, DeviceName);
    }

    for (i=0; OpenDevice(WaveInDevice, i, &h, GENERIC_READ) == MMSYSERR_NOERROR; i++)
    {
        wsprintf(DeviceName, L"WaveIn%d\0", i);
        CloseHandle(h);
        AddDeviceToList(&DeviceList, WaveInDevice, 0, DeviceName);
    }

    for (i=0; OpenDevice(MidiOutDevice, i, &h, GENERIC_READ) == MMSYSERR_NOERROR; i++)
    {
        wsprintf(DeviceName, L"MidiOut%d\0", i);
        CloseHandle(h);
        AddDeviceToList(&DeviceList, MidiOutDevice, 0, DeviceName);
    }

    for (i=0; OpenDevice(MidiInDevice, i, &h, GENERIC_READ) == MMSYSERR_NOERROR; i++)
    {
        wsprintf(DeviceName, L"MidiIn%d\0", i);
        CloseHandle(h);
        AddDeviceToList(&DeviceList, MidiInDevice, 0, DeviceName);
    }

    for (i=0; OpenDevice(AuxDevice, i, &h, GENERIC_READ) == MMSYSERR_NOERROR; i++)
    {
        wsprintf(DeviceName, L"Aux%d\0", i);
        CloseHandle(h);
        AddDeviceToList(&DeviceList, AuxDevice, 0, DeviceName);
    }


    // MIDI Out 0: MPU-401 UART
    // AddDeviceToList(&DeviceList, MidiOutDevice, 0, L"MidiOut0");
    // Wave Out 0: Sound Blaster 16 (ok?)
    // AddDeviceToList(&DeviceList, WaveOutDevice, 0, L"WaveOut0");

    return MMSYSERR_NOERROR; // ok?
}



DWORD GetDeviceCount(UINT DeviceType)
{
    int i;
    PDEVICE_LIST List;

    for (List = DeviceList, i = 0; List != NULL; List = List->Next)
        if (List->DeviceType == DeviceType)
            i ++;

    return i;
}
