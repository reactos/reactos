/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 drivers/dd/sndblst/sndblst.h
 * PURPOSE:              Sound Blaster driver header
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Sept 28, 2003: Created
 */

#ifndef __INCLUDES_SNDBLST_H__
#define __INCLUDES_SNDBLST_H__

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#define DEFAULT_PORT    0x220
#define DEFAULT_IRQ     5
#define DEFAULT_DMA     1
#define DEFAULT_BUFSIZE 0x4000
#define DEFAULT_SAMPLERATE  11025
#define DEFAULT_BITDEPTH    8
#define DEFAULT_CHANNELS    1

#define VALID_IRQS      {5}

#define MIN_BUFSIZE     0x1000
#define MAX_BUFSIZE     0x4000

#define DEVICE_SUBKEY   L"Devices"
#define PARMS_SUBKEY    L"Parameters"

#define REGISTRY_PORT   L"Port"

// At the moment, we just support a single device with fixed parameters:
#define SB_PORT         DEFAULT_PORT
#define SB_IRQ          DEFAULT_IRQ
#define SB_DMA          DEFAULT_DMA
#define SB_BUFSIZE      DEFAULT_BUFSIZE

#define SB_TIMEOUT      1000000

#define IOCTL_SOUND_BASE FILE_DEVICE_SOUND
#define IOCTL_WAVE_BASE  0x0000 // CORRECT?

/* #define IOCTL_MIDI_PLAY CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0006, \
 *                                METHOD_BUFFERED, FILE_WRITE_ACCESS)
 */

// Some constants

#define SB_DSP_READY        0xaa

// Commands (only the ones we use)

#define SB_SET_OUTPUT_RATE  0x41        // DSP v4.xx only
#define SB_SET_INPUT_RATE   0x42        // DSP v4.xx only
#define SB_SET_BLOCK_SIZE   0x48        // DSP v2.00 +
#define SB_ENABLE_SPEAKER   0xd1
#define SB_DISABLE_SPEAKER  0xd3
#define SB_GET_SPEAKER_STATUS 0xd8      // DSP v2.00 +
#define SB_GET_DSP_VERSION  0xe1


// Hmm... These are a weenie bit trickier than MPU401...

#define SB_WRITE_RESET(bp, x)       WRITE_PORT_UCHAR((PUCHAR) bp+0x6, x)
#define SB_READ_DATA(bp)            READ_PORT_UCHAR((PUCHAR) bp+0xa)
#define SB_WRITE_DATA(bp, x)        WRITE_PORT_UCHAR((PUCHAR) bp+0xc, x)
#define SB_READ_WRITESTATUS(bp)     READ_PORT_UCHAR((PUCHAR) bp+0xc)
#define SB_READ_READSTATUS(bp)      READ_PORT_UCHAR((PUCHAR) bp+0xe)

// Flow control

#define SB_READY_TO_SEND(bp) \
    SB_READ_WRITESTATUS(bp) & 0x80

#define SB_READY_TO_RECEIVE(bp) \
    SB_READ_READSTATUS(bp) & 0x80


#define SB_WRITE_BYTE(bp, x) \
    if (WaitToSend(bp)) SB_WRITE_DATA(bp, x)

//#define MPU401_READ(bp)
//    if (WaitToRead(bp)) ... ???

/*
    DEVICE_EXTENSION contains the settings for each individual device
*/

typedef struct _DEVICE_EXTENSION
{
    PWSTR RegistryPath;
    PDRIVER_OBJECT DriverObject;
    UINT Port;
    UINT IRQ;
    UINT DMA;
    ULONG BufferSize;
    PADAPTER_OBJECT Adapter;
    PMDL Mdl;
    PCHAR VirtualBuffer;
    PHYSICAL_ADDRESS Buffer;
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
WORD InitSoundCard(UINT BasePort);

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




BOOLEAN CreateDMA(PDEVICE_OBJECT DeviceObject);



VOID SetOutputSampleRate(UINT BasePort, UINT SampleRate);
VOID EnableSpeaker(UINT BasePort, BOOLEAN SpeakerOn);
BOOLEAN IsSpeakerEnabled(UINT BasePort);
VOID BeginPlayback(UINT BasePort, UINT BitDepth, UINT Channels, UINT BlockSize);

#endif
