/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 services/dd/mpu401/mpu401.h
 * PURPOSE:              MPU-401 MIDI device driver header
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Sept 26, 2003: Created
 */

#ifndef __INCLUDES_MPU401_H__
#define __INCLUDES_MPU401_H__

#include <windows.h>
//#include <mmsystem.h>
//#include <mmddk.h>
//#include <winioctl.h>
#include "../../../lib/mmdrv/mmdef.h"

#define DEFAULT_PORT    0x330
#define DEFAULT_IRQ     9

#define DEVICE_SUBKEY   L"Devices"
#define PARMS_SUBKEY    L"Parameters"

#define REGISTRY_PORT   L"Port"

// At the moment, we just support a single device with fixed parameters:
#define MPU401_PORT     DEFAULT_PORT
#define MPU401_IRQ      DEFAULT_IRQ

#define MPU401_TIMEOUT  10000

/* OBSOLETE - see mmdef.h instead:
#define IOCTL_SOUND_BASE FILE_DEVICE_SOUND
// wave base 0
#define IOCTL_MIDI_BASE  0x0080

#define IOCTL_MIDI_GET_CAPABILITIES CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIDI_SET_STATE CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0002, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIDI_GET_STATE CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0003, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIDI_SET_VOLUME CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0004, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIDI_GET_VOLUME CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0005, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIDI_PLAY CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0006, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIDI_RECORD CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0007, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIDI_CACHE_PATCHES CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0008, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIDI_CACHE_DRUM_PATCHES CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0009, METHOD_BUFFERED, FILE_WRITE_ACCESS)
*/

// The MPU-401 has 2 ports, usually 0x330 and 0x331, which are known as
// "data" and "status/command", respectively. These macros deal with
// reading from and writing to these ports:

#define MPU401_WRITE_DATA(bp, x)    WRITE_PORT_UCHAR((PUCHAR) bp, x)
#define MPU401_READ_DATA(bp)        READ_PORT_UCHAR((PUCHAR) bp)
#define MPU401_WRITE_COMMAND(bp, x) WRITE_PORT_UCHAR((PUCHAR) bp+1, x)
#define MPU401_READ_STATUS(bp)      READ_PORT_UCHAR((PUCHAR) bp+1)


// Flow control

#define MPU401_READY_TO_SEND(bp) \
    MPU401_READ_STATUS(bp) & 0x80

#define MPU401_READY_TO_RECEIVE(bp) \
    MPU401_READ_STATUS(bp) & 0x40


#define MPU401_WRITE_BYTE(bp, x) \
    if (WaitToSend(bp)) MPU401_WRITE_DATA(bp, x)

#define MPU401_WRITE_MESSAGE(bp, status, da, db) \
    MPU401_WRITE(bp, status); \
    MPU401_WRITE(bp, da); \
    MPU401_WRITE(bp, db)

//#define MPU401_READ(bp)
//    if (WaitToRead(bp)) ... ???

/*
    DEVICE_EXTENSION contains the settings for each individual device
*/

typedef struct _DEVICE_EXTENSION
{
    PUNICODE_STRING RegistryPath;
    PDRIVER_OBJECT DriverObject;
    UINT Port;
    UINT IRQ;
//  KDPC Dpc;
//  KTIMER Timer;
//  KEVENT Event;
//  BOOLEAN BeepOn;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/*
    DEVICE_INSTANCE contains ???
*/

typedef struct _DEVICE_INSTANCE
{
    // pPrevGDI
    PDRIVER_OBJECT DriverObject;
} DEVICE_INSTANCE, *PDEVICE_INSTANCE;

/*
    CONFIG contains device parameters (port/IRQ)
    THIS STRUCTURE IS REDUNDANT
*/

//typedef struct _CONFIG
//{
//    UINT Port;
//    UINT IRQ;
//} CONFIG, *PCONFIG;

/*
    Some callback typedefs
*/

typedef NTSTATUS REGISTRY_CALLBACK_ROUTINE(PWSTR RegistryPath, PVOID Context);
typedef REGISTRY_CALLBACK_ROUTINE *PREGISTRY_CALLBACK_ROUTINE;


/*
    Prototypes for functions in portio.c :
*/

BOOLEAN WaitToSend(UINT BasePort);
BOOLEAN WaitToReceive(UINT BasePort);
BOOLEAN InitUARTMode(UINT BasePort);

/*
    Prototypes for functions in settings.c :
*/

NTSTATUS STDCALL EnumDeviceKeys(
    IN PUNICODE_STRING RegistryPath,
    IN PWSTR SubKey,
    IN PREGISTRY_CALLBACK_ROUTINE Callback,
    IN PVOID Context);

NTSTATUS STDCALL LoadSettings(
    IN  PWSTR ValueName,
    IN  ULONG ValueType,
    IN  PVOID ValueData,
    IN  ULONG ValueLength,
    IN  PVOID Context,
    IN  PVOID EntryContext);

#endif
