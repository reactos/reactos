/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver logging function declarations
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#ifndef _KMTEST_LOG_H_
#define _KMTEST_LOG_H_

#include <ntddk.h>

NTSTATUS LogInit(VOID);
VOID LogFree(VOID);

VOID LogPrint(IN PCSTR Message);
VOID LogPrintF(IN PCSTR Format, ...);
VOID LogVPrintF(IN PCSTR Format, va_list Arguments);
SIZE_T LogRead(OUT PVOID Buffer, IN SIZE_T BufferSize);

#endif /* !defined _KMTEST_LOG_H_ */
