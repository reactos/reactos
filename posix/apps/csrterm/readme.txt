$Id: readme.txt,v 1.1 2002/03/17 22:15:39 ea Exp $

csrterm - a CSR client terminal emulator for the POSIX+ subsystem

SYNOPSYS

    csrterm [program]

    program  program to be run in the terminal; if none is given,
             the shell for the current user (W32 session's) is
             used.

DESCRIPTION

    csrterm emulates a DEC VT-100 terminal (on top of the CSRSS
    subsystem, hence the name) which is the controlling terminal
    for a process [program] running in the context of the PSX
    subsystem. csrterm is a Win32 console application, not a PSX
    application. The process created by the PSX subsystem on behalf
    of csrterm is not the child of the csrterm instance that
    requested it. csrterm simply performs terminal I/O in the CSRSS
    world (the W32 world!) for [program].

NOTES

    The role of csrterm is creating a session in the PSX subsystem
    managing any I/O for it. This is how it works:

    1. csrterm creates two well known named objects in the system
       name space that will allow the PSX subsystem server to build
       the I/O channel for the session. To let the PSX subsystem
       process recognize the objects, they contain a numeric suffix
       which is the process identifier (n) the system gives to each
       instance of csrterm:

           \POSIX+\Session\Pn      LPC port (IPC rendez-vous object)
           \POSIX+\Session\Dn      section (shared memory object)

       csrterm also creates a new thread to manage the calls though
       the LPC port. Port Pn is used by the subsystem to control the
       terminal which csrterm emulates.

    2. csrterm connects to the PSX subsystem session port

           \POSIX+\SessionPort

       and asks the subsystem to create a new session.

    3. The PSX subsystem, if it decides to accept the request, creates
       a new session for that calling instance of csrterm (n), and in
       turn connects back to the terminal control port

           \POSIX+\Session\Pn

    4. When csrterm makes the PSX subsystem create the new session, it
       also tells the subsystem what program should be the session
       leader process. The PSX subsystem creates that process (the
       image file to start must be marked IMAGE_SUBSYSTEM_POSIX_GUI or
       IMAGE_SUBSYSTEM_POSIX_CUI).

    5. The requested process [program] runs in the context of the
       PSX subsystem and performs any terminal I/O via the channel
       csrterm and the PSX susbstem created.

REVISIONS
    2001-05-05 created
    2002-03-03 simplified

AUTHOR

    Emanuele Aliberti <ea@iol.it>

CREDITS

    John L. Miller (johnmil@cs.cmu.edu, johnmil@jprc.com) code for
    a basic VT-100 emulator for Win32 consoles is used to process
    tc* calls output.

EOF
