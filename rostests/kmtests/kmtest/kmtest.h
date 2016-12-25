/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Loader Application
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#ifndef _KMTESTS_H_
#define _KMTESTS_H_

extern PCSTR ErrorFileAndLine;

#ifndef KMT_STRINGIZE
#define KMT_STRINGIZE(x) #x
#endif /* !defined KMT_STRINGIZE */

#define location(file, line)                    do { ErrorFileAndLine = file ":" KMT_STRINGIZE(line); } while (0)
#define error_value(Error, value)               do { location(__FILE__, __LINE__); Error = value; } while (0)
#define error(Error)                            error_value(Error, GetLastError())
#define error_goto(Error, label)                do { error(Error); goto label; } while (0)
#define error_value_goto(Error, value, label)   do { error_value(Error, value); goto label; } while (0)

/* service management functions */
DWORD
KmtServiceInit(VOID);

DWORD
KmtServiceCleanup(
    BOOLEAN IgnoreErrors);

DWORD
KmtCreateService(
    IN PCWSTR ServiceName,
    IN PCWSTR ServicePath,
    IN PCWSTR DisplayName OPTIONAL,
    OUT SC_HANDLE *ServiceHandle);

DWORD
KmtStartService(
    IN PCWSTR ServiceName OPTIONAL,
    IN OUT SC_HANDLE *ServiceHandle);

DWORD
KmtCreateAndStartService(
    IN PCWSTR ServiceName,
    IN PCWSTR ServicePath,
    IN PCWSTR DisplayName OPTIONAL,
    OUT SC_HANDLE *ServiceHandle,
    IN BOOLEAN RestartIfRunning);

DWORD
KmtStopService(
    IN PCWSTR ServiceName OPTIONAL,
    IN OUT SC_HANDLE *ServiceHandle);

DWORD
KmtDeleteService(
    IN PCWSTR ServiceName OPTIONAL,
    IN OUT SC_HANDLE *ServiceHandle);

DWORD KmtCloseService(
    IN OUT SC_HANDLE *ServiceHandle);

#endif /* !defined _KMTESTS_H_ */
