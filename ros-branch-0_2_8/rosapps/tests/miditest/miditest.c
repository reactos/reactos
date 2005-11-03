#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>

// WINE's mmsystem.h doesn't seem to define these properly:

#define MIDIOUTCAPS MIDIOUTCAPSA
#define MIDIINCAPS MIDIINCAPSA
#undef midiOutGetDevCaps
#define midiOutGetDevCaps midiOutGetDevCapsA
#undef midiInGetDevCaps
#define midiInGetDevCaps midiInGetDevCapsA


int main()
{
    UINT outs = midiOutGetNumDevs();
//    UINT ins = midiInGetNumDevs();

    MIDIOUTCAPS outcaps;
//    MIDIINCAPS incaps;

    int c;

    printf("MIDI output devices: %d\n", outs);

    for (c = 0; c < outs; c ++)
    {
        if (midiOutGetDevCaps(c, &outcaps, sizeof(MIDIOUTCAPS)) == MMSYSERR_NOERROR)
            printf("Device #%d: %s\n", c, outcaps.szPname);
    }

    printf("Opening MIDI output #0\n");

    HMIDIOUT Handle = NULL;
    UINT Result = midiOutOpen(&Handle, 0, 0, 0, CALLBACK_NULL);
    printf("Result == %d Handle == %d\n", Result, (int)Handle);

    // play something:
    midiOutShortMsg(Handle, 0x007f3090);

/*
    printf("\nMIDI input devices: %d\n", ins);

    for (c = 0; c < ins; c ++)
    {
        midiInGetDevCaps(c, &incaps, sizeof(incaps));
        printf("Device #%d: %s\n", c, incaps.szPname);
    }
*/
    return 0;
}
