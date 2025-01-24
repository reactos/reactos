Intel 8042 port driver

This directory contains a driver for Intels 8042 and compatible controllers.
It is based on the information in the DDK documentation on MSDN. It is intended
to be compatible with keyboard and mouse drivers written for Windows. It is
not based on the i8042prt example driver that's included with the DDK.

The directory contains these files:

createclose.c: open/close devices functionnality

i8042prt.c: Main controller functionality, things shared by keyboards and mice

keyboard.c: keyboard functionality: detection, interrupt handling

misc.c: misc things, mostly related to Irp passing

mouse.c: mouse functionality: detection, interrupt handling, packet parsing for
         standard ps2 and microsoft mice

pnp.c: Plug&Play functionnality

ps2pp.c: logitech ps2++ mouse packat parsing (basic)

readwrite.c: read/write to the i8042 controller

registry.c: registry reading

setup.c: add keyboard support during the 1st stage setup

i8042prt.rc: obvious


Some parts of the driver make little sense. This is because it implements
an interface that has evolved over a long time, and because the ps/2
'standard' is really awful.

Things to add:

- Better AT (before ps2) keyboard handling
- SiS keyboard controller detection
- Mouse identification
- General robustness: reset mouse if things go wrong
- Handling all registry settings
- ACPI

Things not to add:

- Other mouse protocols, touchpad handling etc. : Write a filter driver instead
- Keyboard lights handling: Should be in win32k
- Keyboard scancode translation: Should be in win32k

Things requiring work elsewhere:

- Debugger interface (TAB + key):
  Currently this interface wants translated keycodes, which are not
  implemented by this driver. As it just uses a giant switch with
  hardcoded cases, this should not be hard to fix.

- Class drivers:
  The class drivers should be able to handle reads for more than one packet
  at a time (kbdclass should, mouclass does not). Win32k should send such
  requests.


I put a lot of work in making it work like Microsofts driver does, so third party drivers can work. Please keep it that way.


Links:

Here's a link describing most of the registry settings:

http://www.microsoft.com/resources/documentation/Windows/2000/server/reskit/en-us/Default.asp?url=/resources/documentation/Windows/2000/server/reskit/en-us/regentry/31493.asp (DEAD_LINK)

PS/2 protocol documentation:

http://www.win.tue.nl/~aeb/linux/kbd/scancodes.html

It also contains a link to a description of the ps2++ protocol, which has
since disappeared. Archive.org still has it.
