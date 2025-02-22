----------------------------------
REACTOS MPU-401 MIDI DEVICE DRIVER
by Andrew Greenwood
----------------------------------

This driver initializes the MPU-401 MIDI/joystick port found on
most sound cards, and allows the sending of simple messages.

It's far from complete, and at present will only support 1 device.

In Bochs, the MIDI output will be played using whatever device is
set up in Windows as your MIDI output.

For real hardware, the output will be played to whatever device is
attached to your MIDI/joystick port, or, in some cases, the wave-table
or other synth on-board your card (note: this is NOT an FM synth
driver!)


Thanks to Vizzini and all the other great ReactOS developers for
helping me code this driver and also for giving me encouragement.

I'd also like to thank Jeff Glatt, whose MIDI and MPU-401
documentation has been a valuable resource to me over the past few
years, and who provided me with almost all of my knowledge of MIDI
and MPU-401. His site is at: www.borg.com/~jglatt/

- Andrew "Silver Blade" Greenwood
