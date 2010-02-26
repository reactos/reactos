/*
    ReactOS Sound System
    Sound Blaster DSP support
    Mixer routines

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        2 July 2008 - Created

    Notes:
        Functions documented in sbdsp.h

        Currently, input/output switches and PC speaker volume
        level are not supported.

        The I/O switches are used for muting/unmuting mic, etc.
*/

#include <ntddk.h>
#include <debug.h>

#include <sbdsp.h>

VOID
SbMixerReset(IN PUCHAR BasePort)
{
    WRITE_SB_MIXER_REGISTER(BasePort, SB_MIX_RESET);
    /* Are we meant to send anything else? */
}

NTSTATUS
SbMixerPackLevelData(
    IN  UCHAR Line,
    IN  UCHAR Level,
    OUT PUCHAR PackedLevel)
{
    if ( ! PackedLevel )
        return STATUS_INVALID_PARAMETER_3;

    switch ( Line )
    {
        case SB_MIX_MASTER_LEFT_LEVEL :
        case SB_MIX_MASTER_RIGHT_LEVEL :
        case SB_MIX_VOC_LEFT_LEVEL :
        case SB_MIX_VOC_RIGHT_LEVEL :
        case SB_MIX_MIDI_LEFT_LEVEL :
        case SB_MIX_MIDI_RIGHT_LEVEL :
        case SB_MIX_CD_LEFT_LEVEL :
        case SB_MIX_CD_RIGHT_LEVEL :
        case SB_MIX_LINE_LEFT_LEVEL :
        case SB_MIX_LINE_RIGHT_LEVEL :
        case SB_MIX_MIC_LEVEL :
        case SB_MIX_LEGACY_MIC_LEVEL :  /* is this correct? */
        {
            if ( Level >= 0x20 )
                return STATUS_INVALID_PARAMETER_2;

            *PackedLevel = Level << 3;
            return STATUS_SUCCESS;
        }

        case SB_MIX_INPUT_LEFT_GAIN :
        case SB_MIX_INPUT_RIGHT_GAIN :
        case SB_MIX_OUTPUT_LEFT_GAIN :
        case SB_MIX_OUTPUT_RIGHT_GAIN :
        {
            if ( Level >= 0x04 )
                return STATUS_INVALID_PARAMETER_2;

            *PackedLevel = Level << 6;
            return STATUS_SUCCESS;
        }

        case SB_MIX_VOC_LEVEL :         /* legacy */
        case SB_MIX_MASTER_LEVEL :
        case SB_MIX_FM_LEVEL :
        case SB_MIX_CD_LEVEL :
        case SB_MIX_LINE_LEVEL :
        case SB_MIX_TREBLE_LEFT_LEVEL : /* bass/treble */
        case SB_MIX_TREBLE_RIGHT_LEVEL :
        case SB_MIX_BASS_LEFT_LEVEL :
        case SB_MIX_BASS_RIGHT_LEVEL :
        {
            if ( Level >= 0x10 )
                return STATUS_INVALID_PARAMETER_2;

            *PackedLevel = Level << 4;
            return STATUS_SUCCESS;
        }

        default :
            return STATUS_INVALID_PARAMETER_1;
    };
}

NTSTATUS
SbMixerUnpackLevelData(
    IN  UCHAR Line,
    IN  UCHAR PackedLevel,
    OUT PUCHAR Level)
{
    if ( ! Level )
        return STATUS_INVALID_PARAMETER_3;

    switch ( Line )
    {
        case SB_MIX_MASTER_LEFT_LEVEL :
        case SB_MIX_MASTER_RIGHT_LEVEL :
        case SB_MIX_VOC_LEFT_LEVEL :
        case SB_MIX_VOC_RIGHT_LEVEL :
        case SB_MIX_MIDI_LEFT_LEVEL :
        case SB_MIX_MIDI_RIGHT_LEVEL :
        case SB_MIX_CD_LEFT_LEVEL :
        case SB_MIX_CD_RIGHT_LEVEL :
        case SB_MIX_LINE_LEFT_LEVEL :
        case SB_MIX_LINE_RIGHT_LEVEL :
        case SB_MIX_MIC_LEVEL :
        {
            *Level = PackedLevel >> 3;
            return STATUS_SUCCESS;
        }

        case SB_MIX_INPUT_LEFT_GAIN :
        case SB_MIX_INPUT_RIGHT_GAIN :
        case SB_MIX_OUTPUT_LEFT_GAIN :
        case SB_MIX_OUTPUT_RIGHT_GAIN :
        {
            *Level = PackedLevel >> 6;
            return STATUS_SUCCESS;
        }

        case SB_MIX_VOC_LEVEL :         /* legacy */
        case SB_MIX_MASTER_LEVEL :
        case SB_MIX_FM_LEVEL :
        case SB_MIX_CD_LEVEL :
        case SB_MIX_LINE_LEVEL :
        case SB_MIX_TREBLE_LEFT_LEVEL : /* bass/treble */
        case SB_MIX_TREBLE_RIGHT_LEVEL :
        case SB_MIX_BASS_LEFT_LEVEL :
        case SB_MIX_BASS_RIGHT_LEVEL :
        {
            *Level = PackedLevel >> 4;
            return STATUS_SUCCESS;
        }

        default :
            return STATUS_INVALID_PARAMETER_1;
    };
}

NTSTATUS
SbMixerSetLevel(
    IN  PUCHAR BasePort,
    IN  UCHAR Line,
    IN  UCHAR Level)
{
    UCHAR PackedLevel = 0;
    NTSTATUS Status;

    Status = SbMixerPackLevelData(Line, Level, &PackedLevel);

    switch ( Status )
    {
        case STATUS_SUCCESS :
            break;

        case STATUS_INVALID_PARAMETER_1 :
            return STATUS_INVALID_PARAMETER_2;

        case STATUS_INVALID_PARAMETER_2 :
            return STATUS_INVALID_PARAMETER_3;

        default :
            return Status;
    };

    DbgPrint("SbMixerSetLevel: Line 0x%x, raw level 0x%x, packed 0x%x\n", Line, Level, PackedLevel);

    WRITE_SB_MIXER_REGISTER(BasePort, Line);
    WRITE_SB_MIXER_DATA(BasePort, PackedLevel);

    return STATUS_SUCCESS;
}

NTSTATUS
SbMixerGetLevel(
    IN  PUCHAR BasePort,
    IN  UCHAR Line,
    OUT PUCHAR Level)
{
    UCHAR PackedLevel = 0;
    NTSTATUS Status;

    if ( ! Level )
        return STATUS_INVALID_PARAMETER_3;

    WRITE_SB_MIXER_REGISTER(BasePort, Line);
    PackedLevel = READ_SB_MIXER_DATA(BasePort);

    Status = SbMixerUnpackLevelData(Line, PackedLevel, Level);

    switch ( Status )
    {
        case STATUS_SUCCESS :
            break;

        case STATUS_INVALID_PARAMETER_1 :
            return STATUS_INVALID_PARAMETER_2;

        case STATUS_INVALID_PARAMETER_2 :
            return STATUS_INVALID_PARAMETER_3;

        default :
            return Status;
    };

    DbgPrint("SbMixerGetLevel: Line 0x%x, raw level 0x%x, packed 0x%x\n", Line, Level, PackedLevel);

    return STATUS_SUCCESS;
}

VOID
SbMixerEnableAGC(IN PUCHAR BasePort)
{
    /* Untested... */
    WRITE_SB_MIXER_REGISTER(BasePort, SB_MIX_AGC);
    WRITE_SB_MIXER_DATA(BasePort, 1);
}

VOID
SbMixerDisableAGC(IN PUCHAR BasePort)
{
    /* Untested... */
    WRITE_SB_MIXER_REGISTER(BasePort, SB_MIX_AGC);
    WRITE_SB_MIXER_DATA(BasePort, 0);
}

BOOLEAN
SbMixerIsAGCEnabled(IN PUCHAR BasePort)
{
    /* Untested... */
    WRITE_SB_MIXER_REGISTER(BasePort, SB_MIX_AGC);
    return (READ_SB_MIXER_DATA(BasePort) != 0);
}
