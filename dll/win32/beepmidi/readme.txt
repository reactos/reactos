BEEPMIDI :: BEEP.SYS MIDI DRIVER
(c) Andrew Greenwood, 2007.

http://www.silverblade.co.uk

Released as open-source software. You may copy, re-distribute and modify
this software, provided this copyright notice remains intact.

WHAT'S THIS ?
    BeepMidi is a MME MIDI driver for NT-compatible operating systems,
    which uses BEEP.SYS (the kernel-mode PC speaker driver) to play
    MIDI data. It installs as a standard MIDI output device and can even
    be selected as your default MIDI output device. The fundamental
    code for interacting with BEEP.SYS was taken from ReactOS' kernel32
    module.

WHY WAS THIS WRITTEN ?
    Primarily for educational reasons - in the process, I've learned more
    about the driver side of the MME API and how to interact with kernel
    device drivers. It aids as a good starting point from which to
    move on to bigger and better things :)

HOW TO INSTALL :
    Copy the file to C:\WINDOWS\SYSTEM32\BEEPMIDI.DLL

    Go into HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\
    Drivers32 in RegEdit and look for the "midi" entries on the right hand
    side. Find the highest numbered one (eg: midi1) and create a new STRING
    value. Give it another midi name, but one above the current highest
    entry present (eg: midi2.)

    You'll now see a "PC Speaker" entry in Sound & Audio Devices.

TWEAKING:
    See the comments toward the top of beepmidi.c for tweakable driver
    parameters. These can only be adjusted in the source code at present.

FEATURES :
    * Supports note-on and note-off messages on channels 1-9 and 11-16
      (channel 10 is rhythm, which is not supported.)
    * Fake polyphony (actually just arpeggiates playing notes!)
    * Threaded design for continuous playback (optional.)

ROOM FOR IMPROVEMENT :
    * Pitch bend is not supported
    * Velocity could determine timeslice
    * Should wait for timeslice to complete before adding/removing notes
    * Would be nice to allow configuration of polyphony etc. via Control Panel

BUGS :
    * Crashes when used with Windows Media Player (mplayer2 is fine though)
