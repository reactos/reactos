$Id$

This is SMDLL: a helper library to talk to the ReactOS session manager (SM).

It should be linked in the following components:

a) the SM itself, because it registers for managing native processes
   IMAGE_SUBSYSTEM_NATIVE;

b) environment subsystem servers, because each one should register in
   the SM its own subsystem (willing to manage those processes);

c) terminal emulators for optional subsystems, like posixw32 and os2w32,
   to ask the SM to start the optional subsystem server they need connect to;

d) system and development utilites to debug/query the SM.

2004-02-15 ea


How a subsystem uses these APIs
===============================

Thread #0							Thread #1
- create your own directory (\EXAMPLE)
- create an event E0
- create your call back API port (\EXAMPLE\SbApiPort)
  and serving thread T1
								- wait connection requests on call
								  back port (\EXAMPLE\SbApiPort)
- SmConnectApiPort(
	\EXAMPLE\SbApiPort,
	hSbApiPort,
	SUBSYSTEM_ID,
	& hSmApiPort)
- wait for E0
								- as SM calls back, signal event E0
- create your API port (\EXAMPLE\ApiPort) and
  initialize the subsystem
- call SmCompleteSession (hSmApiPort,
			  hSbApiPort,
			  hApiPort)
- manage processes etc.