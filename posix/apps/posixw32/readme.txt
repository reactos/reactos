$Id: readme.txt,v 1.1 2002/06/09 08:37:07 ea Exp $

posixw32 - a Win32 client terminal emulator for the POSIX+ subsystem

SYNOPSYS

    posixw32 [program]

    program  program to be run in the terminal; if none is given,
             the shell for the current user (W32 session's) is
             used.

DESCRIPTION

    posixw32 emulates a DEC VT-100 terminal (on top of the CSRSS
    subsystem, hence the name) which is the controlling terminal
    for a process [program] running in the context of the PSX
    subsystem. posixw32 is a Win32 console application, not a PSX
    application. The process created by the PSX subsystem on behalf
    of posixw32 is not the child of the posixw32 instance that
    requested it. posixw32 simply performs terminal I/O in the CSRSS
    world (the W32 world!) for [program].

NOTES

    The role of posixw32 is creating a session in the PSX subsystem
    managing any I/O for it. This is how it works:

    1. posixw32 creates two well known named objects in the system
       name space that will allow the PSX subsystem server to build
       the I/O channel for the session. To let the PSX subsystem
       process recognize the objects, they contain a numeric suffix
       which is the process identifier (n) the system gives to each
       instance of posixw32:

           \POSIX+\Session\Pn      LPC port (IPC rendez-vous object)
           \POSIX+\Session\Dn      section (shared memory object)

       posixw32 also creates a new thread to manage the calls though
       the LPC port. Port Pn is used by the subsystem to control the
       terminal which posixw32 emulates.

    2. posixw32 connects to the PSX subsystem session port

           \POSIX+\SessionPort

       and asks the subsystem to create a new session.

    3. The PSX subsystem, if it decides to accept the request, creates
       a new session for that calling instance of posixw32 (n), and in
       turn connects back to the terminal control port

           \POSIX+\Session\Pn

    4. When posixw32 makes the PSX subsystem create the new session, it
       also tells the subsystem what program should be the session
       leader process. The PSX subsystem creates that process (the
       image file to start must be marked IMAGE_SUBSYSTEM_POSIX_GUI or
       IMAGE_SUBSYSTEM_POSIX_CUI).

    5. The requested process [program] runs in the context of the
       PSX subsystem and performs any terminal I/O via the channel
       posixw32 and the PSX susbstem created.

REVISIONS
    2001-05-05 created
    2002-03-03 simplified
    2002-06-08 renamed to avoid future name clash

AUTHOR

    Emanuele Aliberti <ea@iol.it>

CREDITS

    John L. Miller (johnmil@cs.cmu.edu, johnmil@jprc.com) code for
    a basic VT-100 emulator for Win32 consoles is used to process
    tc* calls output.

EOF
