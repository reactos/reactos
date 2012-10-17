/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/subsys/csrss/client.h
 * PURPOSE:         Public Definitions for CSR Clients
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _CSRCLIENT_H
#define _CSRCLIENT_H

#include "msg.h"

/*
BOOLEAN
NTAPI
CsrCaptureArguments(IN PCSR_THREAD CsrThread,
                    IN PCSR_API_MESSAGE ApiMessage);

VOID
NTAPI
CsrReleaseCapturedArguments(IN PCSR_API_MESSAGE ApiMessage);
*/

NTSTATUS
NTAPI
CsrClientConnectToServer(IN PWSTR ObjectDirectory,
                         IN ULONG ServerId,
                         IN PVOID ConnectionInfo,
                         IN OUT PULONG ConnectionInfoSize,
                         OUT PBOOLEAN ServerToServerCall);

NTSTATUS
NTAPI
CsrClientCallServer(IN OUT PCSR_API_MESSAGE Request,
                    IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer OPTIONAL,
                    IN ULONG ApiNumber,
                    IN ULONG RequestLength);

PVOID
NTAPI
CsrAllocateCaptureBuffer(IN ULONG ArgumentCount,
                         IN ULONG BufferSize);

VOID
NTAPI
CsrFreeCaptureBuffer(IN PCSR_CAPTURE_BUFFER CaptureBuffer);

ULONG
NTAPI
CsrAllocateMessagePointer(IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer,
                          IN ULONG MessageLength,
                          OUT PVOID* CaptureData);

VOID
NTAPI
CsrCaptureMessageBuffer(IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer,
                        IN PVOID MessageString,
                        IN ULONG StringLength,
                        OUT PVOID* CapturedData);

BOOLEAN
NTAPI
CsrValidateMessageBuffer(IN PCSR_API_MESSAGE ApiMessage,
                         IN PVOID* Buffer,
                         IN ULONG ArgumentSize,
                         IN ULONG ArgumentCount);

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

NTSTATUS
NTAPI
CsrIdentifyAlertableThread(VOID);

HANDLE
NTAPI
CsrGetProcessId(VOID);

NTSTATUS
NTAPI
CsrNewThread(VOID);

NTSTATUS
NTAPI
CsrSetPriorityClass(IN HANDLE Process,
                    IN OUT PULONG PriorityClass);

#endif // _CSRCLIENT_H

/* EOF */
