#ifndef SNDBLST_H
#define SNDBLST_H

#include <debug.h>
#include <ntddk.h>

#define SB_WAVE_IN_DEVICE_NAME  L"\\Device\\SBWaveIn"
#define SB_WAVE_OUT_DEVICE_NAME L"\\Device\\SBWaveOut"
/* TODO: MIDI */
#define SB_AUX_DEVICE_NAME      L"\\Device\\SBAux"
#define SB_MIXER_DEVICE_NAME    L"\\Device\\SBMixer"

#define DEFAULT_PORT        0x220
#define DEFAULT_IRQ         5
#define DEFAULT_DMA         1
#define DEFAULT_BUFFER_SIZE 65535

#define SB_TIMEOUT          1000000

#define SB_DSP_READY        0xaa

enum
{
    NotDetected,
    SoundBlaster,
    SoundBlasterPro,
    SoundBlaster2,
    SoundBlasterPro2,
    SoundBlasterProMCV,
    SoundBlaster16
};

enum
{
    SB_RESET_PORT           = 0x06,
    SB_READ_DATA_PORT       = 0x0a,
    SB_WRITE_DATA_PORT      = 0x0c,
    SB_WRITE_STATUS_PORT    = 0x0c,
    SB_READ_STATUS_PORT     = 0x0e
};

enum
{
    SbAutoInitDmaOutput     = 0x1c,
    SbAutoInitDmaInput      = 0x2c,
    SbSetOutputRate         = 0x41, /* DSP v4.xx */
    SbSetInputRate          = 0x42, /* DSP v4.xx */
    SbSetBlockSize          = 0x48, /* DSP v2.00 + */
    SbPauseDac              = 0x80,
    SbPauseDmaOutput        = 0xd0,
    SbEnableSpeaker         = 0xd1,
    SbDisableSpeaker        = 0xd3,
    SbGetSpeakerStatus      = 0xd8, /* DSP v2.00 + */
    SbGetDspVersion         = 0xe1
};

typedef struct _SOUND_BLASTER_PARAMETERS
{
    PDRIVER_OBJECT driver;
    PWSTR registry_path;
    PKINTERRUPT interrupt;
    ULONG port;
    ULONG irq;
    ULONG dma;
    ULONG buffer_size;
    USHORT dsp_version;
} SOUND_BLASTER_PARAMETERS, *PSOUND_BLASTER_PARAMETERS;


typedef STDCALL NTSTATUS REGISTRY_CALLBACK_ROUTINE(PDRIVER_OBJECT DriverObject, PWSTR RegistryPath);
typedef REGISTRY_CALLBACK_ROUTINE *PREGISTRY_CALLBACK_ROUTINE;


/*
    Port I/O
*/

#define SbWrite(sbdevice, subport, data) \
    WRITE_PORT_UCHAR((PUCHAR) sbdevice->port + subport, data)

#define SbRead(sbdevice, subport) \
    READ_PORT_UCHAR((PUCHAR) sbdevice->port + subport)

#define SbWriteReset(sbdevice, data) \
    SbWrite(sbdevice, SB_RESET_PORT, data)

#define SbWriteDataWithoutWait(sbdevice, data) \
    SbWrite(sbdevice, SB_WRITE_DATA_PORT, data)

#define SbReadDataWithoutWait(sbdevice) \
    SbRead(sbdevice, SB_READ_DATA_PORT)


#define SbGetWriteStatus(sbdevice) \
    SbRead(sbdevice, SB_WRITE_STATUS_PORT)

#define SbGetReadStatus(sbdevice) \
    SbRead(sbdevice, SB_READ_STATUS_PORT)



BOOLEAN
WaitForReady(
    PSOUND_BLASTER_PARAMETERS SBDevice,
    UCHAR Port);

#define WaitToWrite(sbdevice) \
    WaitForReady(sbdevice, SB_WRITE_STATUS_PORT)

#define WaitToRead(sbdevice) \
    WaitForReady(sbdevice, SB_READ_STATUS_PORT)

BOOLEAN
ResetSoundBlaster(
    PSOUND_BLASTER_PARAMETERS SBDevice);

ULONG
GetSoundBlasterModel(
    PSOUND_BLASTER_PARAMETERS SBDevice);

BOOLEAN
IsSampleRateCompatible(
    PSOUND_BLASTER_PARAMETERS SBDevice,
    ULONG SampleRate);

BOOLEAN
SetOutputSampleRate(
    PSOUND_BLASTER_PARAMETERS SBDevice,
    ULONG SampleRate);

BOOLEAN
EnableSpeaker(
    PSOUND_BLASTER_PARAMETERS SBDevice);

BOOLEAN
DisableSpeaker(
    PSOUND_BLASTER_PARAMETERS SBDevice);

BOOLEAN
StartSoundOutput(
    PSOUND_BLASTER_PARAMETERS SBDevice,
    ULONG BitDepth,
    ULONG Channels,
    ULONG BlockSize);


/*
    interrupt.c
*/

NTSTATUS
EnableIrq(
    PDEVICE_OBJECT DeviceObject);

#endif
