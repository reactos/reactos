/*
    ReactOS Sound System
    Device type IDs and macros

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        14 Feb 2009 - Split from ntddsnd.h

    These are enhancements to the original NT4 DDK audio device header
    files.
*/

#ifndef SNDTYPES_H
#define SNDTYPES_H

/*
    Device types

    Based on the values stored into the registry by the NT4 sndblst
    driver.
*/

typedef enum
{
    // The sound device types
    WAVE_IN_DEVICE_TYPE     = 1,
    WAVE_OUT_DEVICE_TYPE    = 2,
    MIDI_IN_DEVICE_TYPE     = 3,
    MIDI_OUT_DEVICE_TYPE    = 4,
    AUX_DEVICE_TYPE         = 5,
    MIXER_DEVICE_TYPE       = 6,

    // Range of valid device type IDs
    MIN_SOUND_DEVICE_TYPE   = 1,
    MAX_SOUND_DEVICE_TYPE   = 6,

    // Number of sound device types
    SOUND_DEVICE_TYPES      = 6
} SOUND_DEVICE_TYPE;

#define IS_VALID_SOUND_DEVICE_TYPE(x) \
    ( ( x >= MIN_SOUND_DEVICE_TYPE ) && ( x <= MAX_SOUND_DEVICE_TYPE ) )

#define IS_WAVE_DEVICE_TYPE(x) \
    ( ( x == WAVE_IN_DEVICE_TYPE ) || ( x == WAVE_OUT_DEVICE_TYPE ) )

#define IS_MIDI_DEVICE_TYPE(x) \
    ( ( x == MIDI_IN_DEVICE_TYPE ) || ( x == MIDI_OUT_DEVICE_TYPE ) )

#define IS_AUX_DEVICE_TYPE(x) \
    ( x == AUX_DEVICE_TYPE )

#define IS_MIXER_DEVICE_TYPE(x) \
    ( x == MIXER_DEVICE_TYPE )


#endif
