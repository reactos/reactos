/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command 
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org)
 */

#ifndef _NET_PCH_
#define _NET_PCH_

#define WIN32_NO_STATUS

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <wincon.h>
#include <winuser.h>
#include <winsvc.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <lm.h>
#include <ndk/rtlfuncs.h>

#include "resource.h"

VOID
PrintResourceString(
    INT resID,
    ...);

VOID
PrintPaddedResourceString(
    INT resID,
    INT nPaddedLength);

VOID
PrintPadding(
    WCHAR chr,
    INT nPaddedLength);

VOID
PrintToConsole(
    LPWSTR lpFormat,
    ...);

VOID
WriteToConsole(
    LPWSTR lpString);

VOID
ReadFromConsole(
    LPWSTR lpInput,
    DWORD dwLength,
    BOOL bEcho);

VOID help(VOID);
INT unimplemented(INT argc, WCHAR **argv);

INT cmdAccounts(INT argc, WCHAR **argv);
INT cmdContinue(INT argc, WCHAR **argv);
INT cmdHelp(INT argc, WCHAR **argv);
INT cmdHelpMsg(INT argc, WCHAR **argv);
INT cmdLocalGroup(INT argc, WCHAR **argv);
INT cmdPause(INT argc, WCHAR **argv);
INT cmdStart(INT argc, WCHAR **argv);
INT cmdStop(INT argc, WCHAR **argv);
INT cmdUser(INT argc, WCHAR **argv);

#endif /* _NET_PCH_ */
