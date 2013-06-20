/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command 
 * FILE:            
 * PURPOSE:         
 *
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org)
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>
#include <stdio.h>

VOID help(VOID);
INT unimplemented(INT argc, WCHAR **argv);

INT cmdContinue(INT argc, WCHAR **argv);
INT cmdHelp(INT argc, WCHAR **argv);
INT cmdHelpMsg(INT argc, WCHAR **argv);
INT cmdPause(INT argc, WCHAR **argv);
INT cmdStart(INT argc, WCHAR **argv);
INT cmdStop(INT argc, WCHAR **argv);
