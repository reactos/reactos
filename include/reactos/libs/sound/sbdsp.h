/*
    ReactOS Sound System
    Sound Blaster DSP support

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        26 May 2008 - Created

    Notes:
        Where timeouts are concerned, a value of 0 is interpreted as "forever".
*/

#ifndef ROS_SOUND_SBDSP_H
#define ROS_SOUND_SBDSP_H

/*
    Product versions
    DSP 1.0, 1.5, 2.0, 2.01 correspond with respective Sound Blaster versions.
    DSP 3.xx is Sound Blaster Pro
    DSP 4.xx is Sound Blaster 16
*/

#define SOUND_BLASTER_1_0       L"Sound Blaster 1.0"
#define SOUND_BLASTER_1_5       L"Sound Blaster 1.5"
#define SOUND_BLASTER_2_0       L"Sound Blaster 2.0"
#define SOUND_BLASTER_PRO       L"Sound Blaster Pro"
#define SOUND_BLASTER_16        L"Sound Blaster 16"


/*
    Sound Blaster ports I/O
*/
#define READ_SB_FM1_STATUS(bp)          READ_PORT_UCHAR((PUCHAR) bp)
#define WRITE_SB_FM1_REGISTER(bp, x)    WRITE_PORT_UCHAR((PUCHAR) bp, x)
#define WRITE_SB_FM1_DATA(bp, x)        WRITE_PORT_UCHAR((PUCHAR) bp+0x01, x)

#define READ_SB_AFM_STATUS(bp)          READ_PORT_UCHAR((PUCHAR) bp+0x02)
#define WRITE_SB_AFM_REGISTER(bp, x)    WRITE_PORT_UCHAR((PUCHAR) bp+0x02, x)
#define WRITE_SB_AFM_DATA(bp, x)        WRITE_PORT_UCHAR((PUCHAR) bp+0x03, x)

#define WRITE_SB_MIXER_REGISTER(bp, x)  WRITE_PORT_UCHAR((PUCHAR) bp+0x04, x)
#define READ_SB_MIXER_DATA(bp)          READ_PORT_UCHAR((PUCHAR) bp+0x05)
#define WRITE_SB_MIXER_DATA(bp, x)      WRITE_PORT_UCHAR((PUCHAR) bp+0x05, x)

#define WRITE_SB_DSP_RESET(bp, x)       WRITE_PORT_UCHAR((PUCHAR) bp+0x06, x)

#define READ_SB_FM2_STATUS(bp)          READ_PORT_UCHAR((PUCHAR) bp+0x08)
#define WRITE_SB_FM2_REGISTER(bp, x)    WRITE_PORT_UCHAR((PUCHAR) bp+0x08, x)
#define WRITE_SB_FM2_DATA(bp, x)        WRITE_PORT_UCHAR((PUCHAR) bp+0x09, x)

#define READ_SB_DSP_DATA(bp)            READ_PORT_UCHAR((PUCHAR) bp+0x0A)
#define WRITE_SB_DSP_DATA(bp, x)        WRITE_PORT_UCHAR((PUCHAR) bp+0x0C, x)
#define WRITE_SB_DSP_COMMAND(bp, x)     WRITE_PORT_UCHAR((PUCHAR) bp+0x0C, x)

/* Clear to send */
#define SB_DSP_CLEAR_TO_SEND(bp) \
    ( ! (READ_PORT_UCHAR((PUCHAR) bp+0x0C) & 0x80 ) )

/* Data available for reading */
#define SB_DSP_DATA_AVAILABLE(bp) \
    ( READ_PORT_UCHAR((PUCHAR) bp+0x0E) & 0x80 )


#define SB_DSP_READY            0xAA

/*
    Sound Blaster DSP commands
    (partial list)
*/
#define SB_DSP_OUTPUT_RATE      0x41
#define SB_DSP_INPUT_RATE       0x42
#define SB_DSP_BLOCK_SIZE       0x48
#define SB_DSP_SPEAKER_ON       0xD1
#define SB_DSP_SPEAKER_OFF      0xD3
#define SB_DSP_SPEAKER_STATUS   0xD8
#define SB_DSP_VERSION          0xE1

/*
    Mixer lines (legacy)
*/
#define SB_MIX_VOC_LEVEL        0x04
#define SB_MIX_LEGACY_MIC_LEVEL 0x0A
#define SB_MIX_MASTER_LEVEL     0x22
#define SB_MIX_FM_LEVEL         0x26
#define SB_MIX_CD_LEVEL         0x28
#define SB_MIX_LINE_LEVEL       0x2E

/*
    Mixer lines
*/
#define SB_MIX_RESET                0x00
#define SB_MIX_MASTER_LEFT_LEVEL    0x30
#define SB_MIX_MASTER_RIGHT_LEVEL   0x31
#define SB_MIX_VOC_LEFT_LEVEL       0x32
#define SB_MIX_VOC_RIGHT_LEVEL      0x33
#define SB_MIX_MIDI_LEFT_LEVEL      0x34
#define SB_MIX_MIDI_RIGHT_LEVEL     0x35
#define SB_MIX_CD_LEFT_LEVEL        0x36
#define SB_MIX_CD_RIGHT_LEVEL       0x37
#define SB_MIX_LINE_LEFT_LEVEL      0x38
#define SB_MIX_LINE_RIGHT_LEVEL     0x39
#define SB_MIX_MIC_LEVEL            0x3A
#define SB_MIX_PC_SPEAKER_LEVEL     0x3B
#define SB_MIX_OUTPUT_SWITCHES      0x3C
#define SB_MIX_INPUT_LEFT_SWITCHES  0x3D
#define SB_MIX_INPUT_RIGHT_SWITCHES 0x3E
#define SB_MIX_INPUT_LEFT_GAIN      0x3F
#define SB_MIX_INPUT_RIGHT_GAIN     0x40
#define SB_MIX_OUTPUT_LEFT_GAIN     0x41
#define SB_MIX_OUTPUT_RIGHT_GAIN    0x42
#define SB_MIX_AGC                  0x43
#define SB_MIX_TREBLE_LEFT_LEVEL    0x44
#define SB_MIX_TREBLE_RIGHT_LEVEL   0x45
#define SB_MIX_BASS_LEFT_LEVEL      0x46
#define SB_MIX_BASS_RIGHT_LEVEL     0x47

/*
    Mixer switches
    (are these correct?)
*/
#define SB_MIX_MIDI_LEFT_SWITCH     0x01
#define SB_MIX_MIDI_RIGHT_SWITCH    0x02
#define SB_MIX_LINE_LEFT_SWITCH     0x04
#define SB_MIX_LINE_RIGHT_SWITCH    0x08
#define SB_MIX_CD_LEFT_SWITCH       0x10
#define SB_MIX_CD_RIGHT_SWITCH      0x20
#define SB_MIX_MIC_SWITCH           0x40


/*
    Reset the Sound Blaster DSP.
*/
NTSTATUS
SbDspReset(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout);

/*
    Wait for the Sound Blaster DSP to be ready for data to be written to it.
*/
NTSTATUS
SbDspWaitToWrite(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout);

/*
    Wait for data to be ready for reading from the Sound Blaster DSP.
*/
NTSTATUS
SbDspWaitToRead(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout);

/*
    Wait for the Sound Blaster DSP to be ready for data to be written to it,
    then (providing it becomes ready within the timeout period), write the
    data to it.
*/
NTSTATUS
SbDspWrite(
    IN  PUCHAR BasePort,
    IN  UCHAR DataByte,
    IN  ULONG Timeout);

/*
    Wait for the Sound Blaster DSP to be ready for data to be read from it,
    then read the data from it into the pointer supplied as DataByte. If
    the timeout is exceeded, DataByte will not be modified.
*/
NTSTATUS
SbDspRead(
    IN  PUCHAR BasePort,
    OUT PUCHAR DataByte,
    IN  ULONG Timeout);

/*
    This can only be called immediately after a reset has been issued. The
    major version and minor version are returned in MajorVersion and
    MinorVersion, respectively.

    The timeout applies to each DSP read/write performed. Note that the
    data pointed to by MajorVersion may still fail if the retrieval of
    MinorVersion times out.
*/
NTSTATUS
SbDspGetVersion(
    IN  PUCHAR BasePort,
    OUT PUCHAR MajorVersion,
    OUT PUCHAR MinorVersion,
    IN  ULONG Timeout);

/*
    Turn the speaker on.
*/
NTSTATUS
SbDspEnableSpeaker(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout);

/*
    Turn the speaker off.
*/
NTSTATUS
SbDspDisableSpeaker(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout);

/*
    Obtains the speaker status, storing the result in IsEnabled. This will be
    TRUE if the speaker is enabled, otherwise FALSE.
*/
NTSTATUS
SbDspIsSpeakerEnabled(
    IN  PUCHAR BasePort,
    OUT PBOOLEAN IsEnabled,
    IN  ULONG Timeout);

/*
    Validate the input sample rate. The major and minor versions are required
    to determine the capabilities of the card.
*/
BOOLEAN
SbDspIsValidInputRate(
    IN  UCHAR MajorVersion,
    IN  UCHAR MinorVersion,
    IN  USHORT Rate,
    IN  BOOLEAN Stereo);

/*
    Validate the output sample rate. The major and minor versions are required
    to determine the capabilities of the card.
*/
BOOLEAN
SbDspIsValidOutputRate(
    IN  UCHAR MajorVersion,
    IN  UCHAR MinorVersion,
    IN  USHORT Rate,
    IN  BOOLEAN Stereo);

/*
    Set the output/playback rate
    * DSP 4.xx only
*/
NTSTATUS
SbDsp4SetOutputRate(
    IN  PUCHAR BasePort,
    IN  USHORT Rate,
    IN  ULONG Timeout);

/*
    Set the input/record rate
    * DSP 4.xx only
*/
NTSTATUS
SbDsp4SetInputRate(
    IN  PUCHAR BasePort,
    IN  USHORT Rate,
    IN  ULONG Timeout);


/*
    Reset the mixer
*/
VOID
SbMixerReset(IN PUCHAR BasePort);

/*
    Pack mixer level data
*/
NTSTATUS
SbMixerPackLevelData(
    IN  UCHAR Line,
    IN  UCHAR Level,
    OUT PUCHAR PackedLevel);

/*
    Unpack mixer level data
*/
NTSTATUS
SbMixerUnpackLevelData(
    IN  UCHAR Line,
    IN  UCHAR PackedLevel,
    OUT PUCHAR Level);

/*
    Set a mixer line level
*/
NTSTATUS
SbMixerSetLevel(
    IN  PUCHAR BasePort,
    IN  UCHAR Line,
    IN  UCHAR Level);

/*
    Get a mixer line level
*/
NTSTATUS
SbMixerGetLevel(
    IN  PUCHAR BasePort,
    IN  UCHAR Line,
    OUT PUCHAR Level);

/*
    Enable automatic gain control
*/
VOID
SbMixerEnableAGC(IN PUCHAR BasePort);

/*
    Disable automatic gain control
*/
VOID
SbMixerDisableAGC(IN PUCHAR BasePort);

/*
    Retrieve the current state of the automatic gain control
*/
BOOLEAN
SbMixerIsAGCEnabled(IN PUCHAR BasePort);


#endif
