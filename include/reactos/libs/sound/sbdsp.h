/*
    ReactOS Sound System
    Sound Blaster DSP support

    Author:
        Andrew Greenwood (andrew.greenwood@silverblade.co.uk)

    History:
        26 May 2008 - Created

    Notes:
        ...
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

#define READ_SB_DSP_WRITE_BUFFER_STATUS(bp) \
    ( READ_PORT_UCHAR((PUCHAR) bp+0x0C) & 0x01 )

#define READ_SB_DSP_READ_BUFFER_STATUS(bp) \
    ( READ_PORT_UCHAR((PUCHAR) bp+0x0E) & 0x01 )


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
    Routines
*/
BOOLEAN
ResetSoundBlasterDSP(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout);

BOOLEAN
WaitForSoundBlasterDSPReady(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout);

NTSTATUS
GetSoundBlasterDSPVersion(
    IN  PUCHAR BasePort,
    OUT PUCHAR MajorVersion,
    OUT PUCHAR MinorVersion,
    IN  ULONG Timeout);

#endif
