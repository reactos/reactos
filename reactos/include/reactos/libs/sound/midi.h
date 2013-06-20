/*
    ReactOS Sound System
    MIDI constants

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        26 May 2008 - Created
*/

#ifndef ROS_MIDI
#define ROS_MIDI

/* Channel-based MIDI status bytes */
#define MIDI_NOTE_OFF                   0x80
#define MIDI_NOTE_ON                    0x90
#define MIDI_KEY_PRESSURE               0xA0
#define MIDI_CONTROL_CHANGE             0xB0
#define MIDI_PROGRAM_CHANGE             0xC0
#define MIDI_CHANNEL_PRESSURE           0xD0
#define MIDI_PITCH_BEND                 0xE0

/* System MIDI status bytes */
#define MIDI_SYSEX_START                0xF0
#define MIDI_QUARTER_FRAME              0xF1
#define MIDI_SONG_POSITION              0xF2
#define MIDI_SONG_SELECT                0xF3
#define MIDI_TUNE_REQUEST               0xF6
#define MIDI_SYSEX_END                  0xF7
#define MIDI_CLOCK                      0xF8
#define MIDI_TICK                       0xF9
#define MIDI_START                      0xFA
#define MIDI_CONTINUE                   0xFB
#define MIDI_STOP                       0xFC
#define MIDI_ACTIVE_SENSE               0xFE
#define MIDI_RESET                      0xFF

#endif
