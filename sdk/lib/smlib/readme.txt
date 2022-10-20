
SMLIB: Client Library to talk to the ReactOS NT-Compatible Session Manager (SM).

It should be linked in the following components:

a) the SM itself, because it registers for managing native processes
   IMAGE_SUBSYSTEM_NATIVE;

b) environment subsystem servers, because each one should register in
   the SM its own subsystem (willing to manage those processes);

c) application launchers (e.g. terminal emulators) for optional subsystems,
   to ask the SM to start the optional subsystem server they need to connect;

d) system and development utilities to debug/query the SM.

2004-02-15 ea
2022-11-03 hbelusca


How a subsystem uses these APIs
===============================

Thread #0                                   Thread #1

- Creates its own directory (\EXAMPLE)
- Initializes the main parts of the subsystem.
- Creates its main API port (\EXAMPLE\ApiPort),
  and servicing thread for it. Programs running
  under this subsystem will communicate with
  this API port.

- Creates its SM callback API port (\EXAMPLE\SbApiPort)
  and servicing thread T1.

                                            - Waits connection requests on the
                                            callback port (\EXAMPLE\SbApiPort)

- Registers to the SM by calling
  SmConnectToSm(
    "\EXAMPLE\SbApiPort",
    hSbApiPort,
    SUBSYSTEM_ID,
    &hSmApiPort);
                                            - As the SM calls back, validates
                                            and accepts connection.

- The subsystem is now ready to
  manage processes, etc.

-----

Thread #N                                   Thread #1

                                            - The SM calls back the subsystem
                                            SbCreateSession() API, so that it
                                            can initialize a new environment
                                            session (with associated SessionId).

- When no more processes to manage exist,
  terminate the subsystem session by calling
  SmSessionComplete(
    hSmApiPort,
    SessionId,
    ExitStatus);
                                            - The SM calls back the subsystem
                                            SbTerminateSession() API.
