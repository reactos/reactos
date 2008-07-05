/*
    ReactOS Sound System
    Sound Blaster / SB Pro / SB 16 driver
    NT4 driver model

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        25 May 2008 - Created

    Notes:
        ** This driver is currently solely being used to test the sound
        library, which actually implements the main body of the driver. This
        will eventually be replaced with a proper driver. It assumes a
        configuration of base port 0x220, IRQ 5, DMA channel 1 **
*/

#include <ntddk.h>
#include <ntddsnd.h>
#include <debug.h>

#include <hardware.h>
#include <sbdsp.h>
#include <devname.h>

#define SB_DEFAULT_TIMEOUT  1000


typedef struct _SOUND_BLASTER_EXTENSION
{
    ULONG NothingHereYet;
} SOUND_BLASTER_EXTENSION, *PSOUND_BLASTER_EXTENSION;


NTSTATUS STDCALL
CreateSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
CloseSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
CleanupSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
ControlSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;}

NTSTATUS STDCALL
WriteSoundBlaster(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;}


/*
    ============ HACKY TESTING CODE FOLLOWS ==============
*/

PDEVICE_OBJECT g_device;
PKINTERRUPT g_interrupt = 0;

STDCALL
BOOLEAN
InterruptService(
    IN  struct _KINTERRUPT *Interrupt,
    IN  PVOID ServiceContext)
{
    DbgPrint("ISR called\n");
    return TRUE;
}



NTSTATUS
PrepareSoundBlaster(
    IN  PDRIVER_OBJECT DriverObject)
{
    NTSTATUS result;

    DbgPrint("Creating sound device\n");
    result = CreateSoundDeviceWithDefaultName(DriverObject, WAVE_OUT_DEVICE_TYPE, 0, 0, &g_device);
    DbgPrint("Request returned status 0x%08x\n",
             result);

    DbgPrint("Attaching interrupt\n");
    /* Interrupt */
    result = LegacyAttachInterrupt(g_device, 5, InterruptService, &g_interrupt);
    DbgPrint("Request %s\n",
             result == STATUS_SUCCESS ? "succeeded" : "failed");

    return result;
}


VOID STDCALL
UnloadSoundBlaster(
    IN  PDRIVER_OBJECT DriverObject)
{
    NTSTATUS Status;
    DbgPrint("Sound Blaster driver being unloaded\n");

    if ( g_interrupt )
    {
        DbgPrint("Disconnecting interrupt\n");
        IoDisconnectInterrupt(g_interrupt);
    }

    DbgPrint("Destroying devices\n");
    Status = DestroySoundDeviceWithDefaultName(g_device, 0, 69);
    DbgPrint("Status 0x%08x\n", Status);

    /*INFO_(IHVAUDIO, "Sound Blaster driver being unloaded");*/
}

/* This is to be moved into the sound library later */
#define SOUND_PARAMETERS_KEYNAME_W      L"Parameters"
#define SOUND_DEVICES_KEYNAME_W         L"Devices"
#define SOUND_DEVICE_KEYNAME_PREFIX_W   L"Device"

/* NT4 */
#if 0
ULONG
GetSoundDeviceCount(
    IN  PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE KeyHandle;
    ULONG DeviceCount = 0;
    PCWSTR RegistryPathBuffer;
    UNICODE_STRING FullRegistryPath;
    ULONG PathLength;

    //PathLength = RegistryPath.Length +;

    /* TODO */
    /*RegistryPathBuffer = ExAllocatePoolWithTag(PAGED_POOL,*/

    InitializeObjectAttributes(&Attributes,
                               RegistryPath,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&KeyHandle,
                       KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                       &Attributes);

    if ( ! NT_SUCCESS(Status) )
    {
        return 0;
    }

    //ZwEnumerateKey(Key
}
#endif


/*
    This is purely for testing the sound library at present. Eventually this
    should be re-formed to consider that some (admittedly slightly crazy)
    users might have more than one sound blaster card, or perhaps might have
    a different configuration.

    ie, this is pretty much as non-PnP as you can get!
*/

STDCALL
NTSTATUS
TestSoundBlasterLibrary()
{
    NTSTATUS result;
    BOOLEAN speaker_state = TRUE, agc_enabled = FALSE;
    UCHAR major = 0x69, minor = 0x96;
    UCHAR level = 0;

    /* 0x220 is the port we expect to work */
    DbgPrint("Resetting Sound Blaster DSP at 0x220\n");
    result = SbDspReset((PUCHAR)0x220, SB_DEFAULT_TIMEOUT);
    DbgPrint("Reset was %s\n",
             result == STATUS_SUCCESS ? "successful" : "unsuccessful");

    /* 0x240, we don't expect to work */
    DbgPrint("Resetting Sound Blaster DSP at 0x240\n");
    result = SbDspReset((PUCHAR)0x240, SB_DEFAULT_TIMEOUT);
    DbgPrint("Reset was %s\n",
             result == STATUS_SUCCESS ? "successful" : "unsuccessful");

    /* Try getting version */
    DbgPrint("Retrieving Sound Blaster version...\n");
    result = SbDspGetVersion((PUCHAR)0x220, &major, &minor, SB_DEFAULT_TIMEOUT);

    DbgPrint("Version retrival was %s\n",
             result == STATUS_SUCCESS ? "successful" : "unsuccessful");
    DbgPrint("Sound Blaster DSP version is %d.%02d\n", major, minor);

    /* Speaker tests */

    result = SbDspIsSpeakerEnabled((PUCHAR)0x220, &speaker_state, SB_DEFAULT_TIMEOUT);
    DbgPrint("Speaker state retrieval %s\n",
             result == STATUS_SUCCESS ? "succeeded" : "failed");

    DbgPrint("Speaker state is presently %s\n",
             speaker_state ? "ENABLED" : "DISABLED");

    result = SbDspEnableSpeaker((PUCHAR)0x220, SB_DEFAULT_TIMEOUT);
    DbgPrint("Speaker enable request %s\n",
             result == STATUS_SUCCESS ? "succeeded" : "failed");

    result = SbDspIsSpeakerEnabled((PUCHAR)0x220, &speaker_state, SB_DEFAULT_TIMEOUT);
    DbgPrint("Speaker state retrieval %s\n",
             result == STATUS_SUCCESS ? "succeeded" : "failed");
    DbgPrint("Speaker state is now %s\n",
             speaker_state ? "ENABLED" : "DISABLED");

    result = SbDspDisableSpeaker((PUCHAR)0x220, SB_DEFAULT_TIMEOUT);
    DbgPrint("Speaker disable request %s\n",
             result == STATUS_SUCCESS ? "succeeded" : "failed");

    result = SbDspIsSpeakerEnabled((PUCHAR)0x220, &speaker_state, SB_DEFAULT_TIMEOUT);
    DbgPrint("Speaker state retrieval %s\n",
             result == STATUS_SUCCESS ? "succeeded" : "failed");
    DbgPrint("Speaker state is now %s\n",
             speaker_state ? "ENABLED" : "DISABLED");


    /* Mixer tests */

    DbgPrint("Resetting SB Mixer\n");
    SbMixerReset((PUCHAR)0x220);

    DbgPrint("Setting master level to 0F\n");
    result = SbMixerSetLevel((PUCHAR)0x220, SB_MIX_MASTER_LEVEL, 0x0f);
    DbgPrint("Request %s\n",
             result == STATUS_SUCCESS ? "succeeded" : "failed");

    DbgPrint("Getting master level\n");
    result = SbMixerGetLevel((PUCHAR)0x220, SB_MIX_MASTER_LEVEL, &level);
    DbgPrint("Request %s\n",
             result == STATUS_SUCCESS ? "succeeded" : "failed");
    DbgPrint("Level is 0x%x\n", level);


    /* AGC testing */

    agc_enabled = SbMixerIsAGCEnabled((PUCHAR)0x220);
    DbgPrint("AGC enabled? %d\n", agc_enabled);

    SbMixerEnableAGC((PUCHAR)0x220);
    agc_enabled = SbMixerIsAGCEnabled((PUCHAR)0x220);
    DbgPrint("AGC enabled? %d\n", agc_enabled);

    SbMixerDisableAGC((PUCHAR)0x220);
    agc_enabled = SbMixerIsAGCEnabled((PUCHAR)0x220);
    DbgPrint("AGC enabled? %d\n", agc_enabled);

    return STATUS_SUCCESS;
}




NTSTATUS STDCALL
DriverEntry(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;

    DbgPrint("Sound Blaster driver (NT4 model) by Silver Blade\n");
    /*INFO_(IHVAUDIO, "Sound Blaster driver (NT4 model) by Silver Blade");*/

    DriverObject->Flags = 0;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = CleanupSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ControlSoundBlaster;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = WriteSoundBlaster;
    DriverObject->DriverUnload = UnloadSoundBlaster;

    /* Hax */
    TestSoundBlasterLibrary();
    return PrepareSoundBlaster(DriverObject);
}
