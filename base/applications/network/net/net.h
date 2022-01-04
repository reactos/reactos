/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org)
 */

#ifndef _NET_PCH_
#define _NET_PCH_

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <wincon.h>
#include <winsvc.h>
#include <winnetwk.h>
#include <lm.h>
#include <ndk/rtlfuncs.h>

#include <conutils.h>

#include <net_msg.h>

extern HMODULE hModuleNetMsg;

VOID
PrintPaddedResourceString(
    UINT uID,
    INT nPaddedLength);

VOID
PrintPadding(
    WCHAR chr,
    INT nPaddedLength);

VOID
PrintMessageString(
    DWORD dwMessage);

VOID
PrintMessageStringV(
    DWORD dwMessage,
    ...);

VOID
PrintPaddedMessageString(
    DWORD dwMessage,
    INT nPaddedLength);

VOID
PrintErrorMessage(
    DWORD dwError);

VOID
PrintNetMessage(
    DWORD dwMessage);

VOID
ReadFromConsole(
    LPWSTR lpInput,
    DWORD dwLength,
    BOOL bEcho);

VOID help(VOID);
INT unimplemented(INT argc, WCHAR **argv);

INT cmdAccounts(INT argc, WCHAR **argv);
INT cmdComputer(INT argc, WCHAR **argv);
INT cmdConfig(INT argc, WCHAR **argv);
INT cmdContinue(INT argc, WCHAR **argv);
INT cmdGroup(INT argc, WCHAR **argv);
INT cmdHelp(INT argc, WCHAR **argv);
INT cmdHelpMsg(INT argc, WCHAR **argv);
INT cmdLocalGroup(INT argc, WCHAR **argv);
INT cmdPause(INT argc, WCHAR **argv);
INT cmdShare(INT argc, WCHAR **argv);
INT cmdStart(INT argc, WCHAR **argv);
INT cmdStatistics(INT argc, WCHAR **argv);
INT cmdStop(INT argc, WCHAR **argv);
INT cmdSyntax(INT argc, WCHAR **argv);
INT cmdUse(INT argc, WCHAR **argv);
INT cmdUser(INT argc, WCHAR **argv);

#endif /* _NET_PCH_ */
