/*
    Sound card operations

    http://www.cae.wisc.edu/~brodskye/sb16doc/sb16doc.html
*/

#include <ddk/ntddk.h>

#include <debug.h>
#include "sndblst.h"

VOID SetOutputSampleRate(UINT BasePort, UINT SampleRate)
{
    // This only works for DSP v4.xx ONLY - need a workaround!
    DbgPrint("Setting output sample rate\n");

    // WAIT
//    if (! WaitToSend(BasePort))
//        return;

    SB_WRITE_BYTE(BasePort, SB_SET_OUTPUT_RATE);
    SB_WRITE_BYTE(BasePort, SampleRate / 256);  // high
    SB_WRITE_BYTE(BasePort, SampleRate % 256);  // low
}


VOID EnableSpeaker(UINT BasePort, BOOLEAN SpeakerOn)
{
    DbgPrint("Setting speaker status %d\n", SpeakerOn);

//    if (! WaitForWrite(BasePort))
//        return;

    SB_WRITE_BYTE(BasePort, SpeakerOn ? SB_ENABLE_SPEAKER : SB_DISABLE_SPEAKER);
}


BOOLEAN IsSpeakerEnabled(UINT BasePort)
{
    DbgPrint("Obtaining speaker status\n");

//    if (! WaitToSend(BasePort))
//        return FALSE;

    SB_WRITE_BYTE(BasePort, SB_GET_SPEAKER_STATUS);
    if (! WaitToReceive(BasePort))
        return FALSE;

    return SB_READ_DATA(BasePort) == 0xff;
}


VOID BeginPlayback(UINT BasePort, UINT BitDepth, UINT Channels, UINT BlockSize)
{
    CHAR Command;

    DbgPrint("BeginPlayback(%d, %d, %d, %d)\n", BasePort, BitDepth, Channels, BlockSize);

//    switch(BitDepth)
//    {
//        case 8 :    Command = 0xc0; break;
//        case 16 :   Command = 0xb0; break;  // Make sure we support it
//        default :   Command = 0xc0;
//    }

    DbgPrint("Initiating playback\n");

    // TEMPORARY:
    SB_WRITE_BYTE(BasePort, 0xc6);
    SB_WRITE_BYTE(BasePort, 0); // mode - TEMPORARY
    SB_WRITE_BYTE(BasePort, BlockSize % 256);
    SB_WRITE_BYTE(BasePort, BlockSize / 256);
}
