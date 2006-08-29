
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command 
 * FILE:            
 * PURPOSE:         
 *
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org) 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

void help();
int unimplement();


INT cmdHelp(INT argc, CHAR **argv);

INT cmdStart(INT argc, CHAR **argv );
INT start_service(CHAR *service);

INT cmdStop(INT argc, CHAR **argv );
INT stop_service(CHAR *service);

/* Control and start rpcclient */
BOOL myCreateProcessStartGetSzie(CHAR *cmdline, LONG *size);
BOOL myCreateProcessStart(CHAR *cmdline, CHAR *srvlst, LONG size);
BOOL myCreateProcess(HANDLE hChildStdoutWr, HANDLE hChildStdinRd, CHAR *cmdline);
LONG ReadPipe(HANDLE hStdoutWr, HANDLE hStdoutRd, CHAR *srvlst, LONG size);
LONG ReadPipeSize(HANDLE hStdoutWr, HANDLE hStdoutRd); 
INT row_scanner_service(CHAR *buffer, LONG* pos, LONG size, CHAR *name,CHAR *save);

