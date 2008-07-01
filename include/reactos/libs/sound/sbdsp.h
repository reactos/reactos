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

#include <sound/time.h>

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
    Validate the sample rate
    * DSP 4.xx only
*/
BOOLEAN
SbDspIsValidRate(
    IN  USHORT Rate);

/*
    Set the output/playback rate
    * DSP 4.xx only
*/
NTSTATUS
SbDspSetOutputRate(
    IN  PUCHAR BasePort,
    IN  USHORT Rate,
    IN  ULONG Timeout);

/*
    Set the input/record rate
    * DSP 4.xx only
*/
NTSTATUS
SbDspSetInputRate(
    IN  PUCHAR BasePort,
    IN  USHORT Rate,
    IN  ULONG Timeout);


#endif
