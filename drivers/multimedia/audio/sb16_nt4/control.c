#include <sndblst.h>

/*
    TODO: MmMapIoSpace()
*/

/*
    This checks the read or write status port of the device.
*/

BOOLEAN
WaitForReady(
    PSOUND_BLASTER_PARAMETERS SBDevice,
    UCHAR Port)
{
    ULONG timeout = SB_TIMEOUT;
    BOOL ready = FALSE;

    while ( ( ! ready ) && ( timeout > 0 ) )
    {
        if ( SbRead(SBDevice, Port) & 0x80 )
            return TRUE;

        timeout --;
    }

    return FALSE;
}

BOOLEAN
SbWriteData(
    PSOUND_BLASTER_PARAMETERS SBDevice,
    UCHAR Data)
{
    if ( ! WaitToWrite(SBDevice) )
        return FALSE;

    DPRINT("Writing 0x%x to Sound Blaster card (data)\n", Data);
    SbWrite(SBDevice, SB_WRITE_DATA_PORT, Data);

    return TRUE;
}

BOOLEAN
SbReadData(
    PSOUND_BLASTER_PARAMETERS SBDevice,
    PUCHAR Data)
{
    if ( ! WaitToWrite(SBDevice) )
        return FALSE;

    *Data = SbRead(SBDevice, SB_READ_DATA_PORT);
    DPRINT("Read 0x%x from Sound Blaster card (data)\n", *Data);

    return TRUE;
}

BOOLEAN
ResetSoundBlaster(
    PSOUND_BLASTER_PARAMETERS SBDevice)
{
    BOOLEAN acked = FALSE;
    ULONG timeout;

    SbWriteReset(SBDevice, 0x01);
    for (timeout = 0; timeout < 30000; timeout ++ );
    SbWriteReset(SBDevice, 0x00);

    DPRINT("Waiting for SB to acknowledge our reset request\n");

    if ( ! WaitToRead(SBDevice) )
    {
        DPRINT("Didn't get an ACK :(\n");
        return FALSE;
    }

    timeout = 0;

    while ( ( ! acked ) && ( timeout < SB_TIMEOUT ) )
    {
        acked = ( SbReadDataWithoutWait(SBDevice) == SB_DSP_READY );
        timeout ++;
    }

    if ( ! acked )
    {
        DPRINT("Didn't get an ACK :(\n");
        return FALSE;
    }

    return TRUE;
}

ULONG
GetSoundBlasterModel(
    PSOUND_BLASTER_PARAMETERS SBDevice)
{
    UCHAR MajorVer, MinorVer;

    DPRINT("Querying DSP version\n");

    if ( ! SbWriteData(SBDevice, SbGetDspVersion) )
        return NotDetected;

    if ( ! WaitToRead(SBDevice) )
        return NotDetected;

    if ( SbReadData(SBDevice, &MajorVer) )
    {
        if ( SbReadData(SBDevice, &MinorVer) )
        {
            DPRINT("Version %d.%d\n", MajorVer, MinorVer);

            SBDevice->dsp_version = (MajorVer * 256) + MinorVer;

            if ( SBDevice->dsp_version < 0x0200 )
                return SoundBlaster;
            else if ( ( SBDevice->dsp_version & 0xFF00 ) == 0x0200 )
                return SoundBlaster2;
            else if ( ( SBDevice->dsp_version & 0xFF00 ) == 0x0300 )
                return SoundBlasterPro;
            else if ( SBDevice->dsp_version >= 0x0400 )
                return SoundBlaster16;

            return NotDetected;
        }
    }

    return NotDetected;
}


BOOLEAN
IsSampleRateCompatible(
    PSOUND_BLASTER_PARAMETERS SBDevice,
    ULONG SampleRate)
{
    /* TODO */
    return TRUE;
}

BOOLEAN
SetOutputSampleRate(
    PSOUND_BLASTER_PARAMETERS SBDevice,
    ULONG SampleRate)
{
    /* Only works for DSP v4.xx */
    DPRINT("Setting sample rate\n");

    SbWriteData(SBDevice, SbSetOutputRate);
    SbWriteData(SBDevice, SampleRate / 256);
    SbWriteData(SBDevice, SampleRate % 256);

    return TRUE;
}

BOOLEAN
EnableSpeaker(
    PSOUND_BLASTER_PARAMETERS SBDevice)
{
    DPRINT("Enabling speaker\n");

    return SbWriteData(SBDevice, SbEnableSpeaker);
}

BOOLEAN
DisableSpeaker(
    PSOUND_BLASTER_PARAMETERS SBDevice)
{
    DPRINT("Disabling speaker\n");

    return SbWriteData(SBDevice, SbDisableSpeaker);
}

BOOLEAN
StartSoundOutput(
    PSOUND_BLASTER_PARAMETERS SBDevice,
    ULONG BitDepth,
    ULONG Channels,
    ULONG BlockSize)
{
    DPRINT("Initializing output with %d channels at %d bits/sample\n", Channels, BitDepth);

    UCHAR command = 0xc6, mode = 0x00;

    if ( ( Channels < 1 ) || ( Channels > 2 ) )
        return FALSE;

    if ( ( BitDepth != 8 ) && ( BitDepth != 16 ) )
        return FALSE;

    switch ( BitDepth )
    {
        case 8 :    command = 0xc6; break;
        case 16 :   command = 0xb6; break;
    };

    switch ( Channels )
    {
        case 1 :    mode = 0x00; break;
        case 2 :    mode = 0x20; break;
    }
#if 0
    first_byte = (BitDepth == 16) ? 0xb6 : 0xc6;
    second_byte = (Channels == 1) ? 0x20 : 0x00;
#endif

    if ( SBDevice->dsp_version < 0x0400 )
    {
        /* TODO: Additional programming required */
    }

    /* Send freq */
    SbWriteData(SBDevice, command);
    SbWriteData(SBDevice, mode);
    SbWriteData(SBDevice, BlockSize % 256);
    SbWriteData(SBDevice, BlockSize / 256);

    DPRINT("Finished programming the DSP\n");

    return TRUE;
}
