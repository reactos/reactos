$Id$

This is SMDLL: a helper library to talk to the ReactOS session manager (SM).

It should be linked in the following components:

a) the SM itself, because iy registers for managing native processes
   IMAGE_SUBSYSTEM_NATIVE;

b) environment subsystem servers, because each one should register in
   the SM its own subsystem (willing to manageg those processes);

c) terminal emulators for optional subsystems, like posixw32 and os2w32,
   to ask the SM to start the optional subsystem server they need connect to;

d) system and development utilites to debug/query the SM.

2004-02-15 ea