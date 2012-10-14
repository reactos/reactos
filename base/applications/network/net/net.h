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
#include <winsvc.h>

VOID help(VOID);
INT unimplemented(INT argc, WCHAR **argv);


INT cmdHelp(INT argc, WCHAR **argv);
INT cmdStart(INT argc, WCHAR **argv);
INT cmdStop(INT argc, WCHAR **argv);
