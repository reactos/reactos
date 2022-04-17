/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Client/Server Runtime SubSystem
 * FILE:            include/reactos/subsys/csr/csr.h
 * PURPOSE:         Public definitions for CSR Clients
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _CSR_H
#define _CSR_H

#include "csrmsg.h"

NTSTATUS
NTAPI
CsrClientConnectToServer(IN PWSTR ObjectDirectory,
                         IN ULONG ServerId,
                         IN PVOID ConnectionInfo,
                         IN OUT PULONG ConnectionInfoSize,
                         OUT PBOOLEAN ServerToServerCall);

NTSTATUS
NTAPI
CsrClientCallServer(IN OUT PCSR_API_MESSAGE ApiMessage,
                    IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer OPTIONAL,
                    IN CSR_API_NUMBER ApiNumber,
                    IN ULONG DataLength);

PCSR_CAPTURE_BUFFER
NTAPI
CsrAllocateCaptureBuffer(IN ULONG ArgumentCount,
                         IN ULONG BufferSize);

ULONG
NTAPI
CsrAllocateMessagePointer(IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer,
                          IN ULONG MessageLength,
                          OUT PVOID* CapturedData);

VOID
NTAPI
CsrCaptureMessageBuffer(IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer,
                        IN PVOID MessageBuffer OPTIONAL,
                        IN ULONG MessageLength,
                        OUT PVOID* CapturedData);

VOID
NTAPI
CsrFreeCaptureBuffer(IN PCSR_CAPTURE_BUFFER CaptureBuffer);

VOID
NTAPI
CsrCaptureMessageString(IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer,
                        IN PCSTR String OPTIONAL,
                        IN ULONG StringLength,
                        IN ULONG MaximumLength,
                        OUT PSTRING CapturedString);

NTSTATUS
NTAPI
CsrCaptureMessageMultiUnicodeStringsInPlace(OUT PCSR_CAPTURE_BUFFER* CaptureBuffer,
                                            IN ULONG StringsCount,
                                            IN PUNICODE_STRING* MessageStrings);

PLARGE_INTEGER
NTAPI
CsrCaptureTimeout(IN ULONG Milliseconds,
                  OUT PLARGE_INTEGER Timeout);

VOID
NTAPI
CsrProbeForRead(IN PVOID Address,
                IN ULONG Length,
                IN ULONG Alignment);

VOID
NTAPI
CsrProbeForWrite(IN PVOID Address,
                 IN ULONG Length,
                 IN ULONG Alignment);

HANDLE
NTAPI
CsrGetProcessId(VOID);

NTSTATUS
NTAPI
CsrNewThread(VOID);

NTSTATUS
NTAPI
CsrIdentifyAlertableThread(VOID);

NTSTATUS
NTAPI
CsrSetPriorityClass(IN HANDLE Process,
                    IN OUT PULONG PriorityClass);

#endif // _CSR_H

/* EOF */
