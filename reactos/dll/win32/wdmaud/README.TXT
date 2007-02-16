User-mode Multimedia Driver for WDM Audio
-----------------------------------------

USAGE
-----
This is a "drop-in" replacement for the Windows XP wdmaud.drv component. To
make use of it, you'll need to disable system file protection somehow (easy
way is to rename all *.cab files in the sub-folders of C:\WINDOWS\DRIVER CACHE
to something else, then delete or rename WDMAUD.DRV in both C:\WINDOWS\SYSTEM32
and C:\WINDOWS\SYSTEM32\DLLCACHE.)

Now put the ReactOS wdmaud.drv in C:\WINDOWS\SYSTEM32. At some point, you'll
be asked to insert the Windows CD as some files are missing - cancel this and
choose "yes" when asked if you're sure. This is due to the system file
protection not being able to have its own way.

That should be all there is to it.


IMPLEMENTATION/DEVELOPMENT NOTES
--------------------------------
The style of driver used here dates back to the days of Windows 3.1. Not much
has changed - the NT 4 Sound Blaster driver exported the same functions and
supported the same message codes as the traditional Windows 3.1 user-mode
drivers did.

But in XP, something strange happens with WDMAUD (which is why you can't just
put this next to the existing WDMAUD under a different name!)

It appears that WINMM.DLL treats WDMAUD.DRV differently to other drivers.

Here's a summary of how things work differently:

1)	It seems DRV_ENABLE and DRV_DISABLE are the only important messages
	processed by DriverProc. These open and close the kernel-mode
	driver.

2)	Each message handling function (aside from DriverProc) receives a
	DRVM_INIT message after the driver has been opened. The second
	parameter of this is a pointer to a string containing a device
	path (\\?\... format.) Returning zero seems to be the accepted
	thing to do, in any case.

	The purpose of this function (in our case) is to allow WDMAUD.DRV
	to let WDMAUD.SYS know which device we want to play with.

	Presumably, this is called when new devices are added to the system,
	as well. I don't know if this is called once per hardware device...

3)	xxxx_GETNUMDEVS now has extra data passed to it! The first
	parameter is a device path string - which will have been passed
	to DRVM_INIT previously.

4)	xxxx_GETDEVCAPS is a bit hazardous. The old set of parameters were:
	1 - Pointer to a capabilities structure (eg: WAVEOUTCAPS)
	2 - Size of the above structure

	But now, the parameters are:
	1 - Pointer to a MDEVCAPSEX struct (which points to a regular
	    capabilities structure)
	2 - Device path string

	So anything expecting the second parameter to be a size (for copying
	memory maybe) is in for a bit of a surprise there!

The reason for the above changes is Plug and Play. It seems that the extra
functionality was added in Windows 98 (possibly 95 as well) to make it
possible to hot-swap winmm-supported devices without requiring a restart of
whatever applications are using the devices.

That's the theory, at least.


TODO
----
Our WINMM.DLL will need hacking to make sure it can take into account the
preferential treatment given to WDMAUD.DRV

I'm not sure if it'll work with it yet as there seems to be some accomodation
for the PnP DRVM_INIT and DRVM_EXIT messages, but whether or not winmm will
function correctly with this WDMAUD.DRV replacement is something that remains
to be seen.


THANKS
------
Thanks to everyone who has encouraged me to continue developing the audio
system for ReactOS.

In particular, I'd like to thank Alex Ionescu for the many hours of assistance
he has given me in figuring out how things are done.


-

Andrew Greenwood
andrew.greenwood AT silverblade DOT co DOT uk
