MME BUDDY

This library currently is capable of maintaining lists of devices for all of
the MME types, it will provide the appropriate entrypoints for each device
type, and code using this library simply needs to inform the MME Buddy
library of the devices that exist, and provide callback routines to be used
when opening/closing/playing, etc.

Code using this library needs to provide its own DriverProc entrypoint (this
may be refactored in future so that simply an init/cleanup routine need be
provided.)


WAVE OUTPUT
===========
Supported MME messages:
* WODM_GETNUMDEVS (Get number of devices)
* WODM_GETDEVCAPS (Get device capabilities)
* WODM_OPEN (Open a device, query supported formats)
* WODM_CLOSE (Close a device)
* WODM_PREPARE (Prepare a wave header)
* WODM_UNPREPARE (Unprepare a wave header)
* WODM_WRITE (Submit a prepared header to be played)

Unsupported MME messages:
* Any not mentioned above

Notes/Bugs:
* WHDR_BEGINLOOP and WHDR_ENDLOOP are ignored
* Not possible to pause/restart playback


WAVE INPUT
==========
Supported MME messages:
* WIDM_GETNUMDEVS (Get number of devices)

Unsupported MME messages:
* Any not mentioned above

Notes/Bugs:
* Mostly unimplemented


MIDI OUTPUT
===========
Supported MME messages:
* MODM_GETNUMDEVS (Get number of devices)

Unsupported MME messages:
* Any not mentioned above

Notes/Bugs:
* Mostly unimplemented


MIDI INPUT
==========
Supported MME messages:
* MIDM_GETNUMDEVS (Get number of devices)

Unsupported MME messages:
* Any not mentioned above

Notes/Bugs:
* Mostly unimplemented


AUXILIARY
=========
Supported MME messages:
* AUXM_GETNUMDEVS (Get number of devices)

Unsupported MME messages:
* Any not mentioned above

Notes/Bugs:
* Mostly unimplemented


MIXER
=====
Supported MME messages:
* MXDM_GETNUMDEVS (Get number of devices)

Unsupported MME messages:
* Any not mentioned above

Notes/Bugs:
* Mostly unimplemented
