/* $Id: exit.c,v 1.2 2002/02/20 09:17:58 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/stdlib/exit.c
 * PURPOSE:     Terminate a process
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              27/12/2001: Created
 */

#include <ddk/ntddk.h>
#include <stdlib.h>
#include <psx/debug.h>

void exit(int status)
{
 TODO("call all functions registered with atexit()");

 TODO("flush all output streams, close all open streams");
 TODO("remove all files created by tmpfile()");

 TODO("close all of the file descriptors, directory streams, conversion \
descriptors and message catalogue descriptors");
 TODO("send SIGCHILD to the parent process");
 TODO("set parent pid of children to pid of psxss");
 TODO("detach each attached shared-memory segment");
 TODO("for each semaphore for which the calling process has set a semadj \
value(), add the value to the semval of the semaphore.");
 TODO("if the process is a controlling process, send SIGHUP to each process \
in the foreground process group...");
 TODO("... and disassociate the terminal from the session");
 TODO("if the exit causes a process group to become orphaned, and if any \
member of the newly-orphaned process group is stopped, send SIGHUP and \
SIGCONT to each process in the newly-orphaned process group");
 TODO("all open named semaphores in the calling process are closed");
 TODO("remove any memory locks");
 TODO("destroy memory mappings");
 TODO("close all open message queue descriptors");

#if 0
 ExitProcess(status);
#endif

 NtTerminateProcess(NtCurrentProcess(), status);

}

/* EOF */

