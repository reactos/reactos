/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Runtime Library
 * PURPOSE:         User-Mode Exception Support
 * FILE:            lib/rtl/exception.c
 * PROGRAMERS:      Alex Ionescu (alex@relsoft.net)
 *                  David Welch <welch@cwcom.net>
 *                  Skywing <skywing@valhallalegends.com>
 *                  KJK::Hyperion <noog@libero.it>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#define NDEBUG
#include <debug.h>

NTSYSAPI
VOID
NTAPI
RtlCaptureContext
(OUT PCONTEXT ContextRecord)
{
    // XXX arty fixme
}

NTSYSAPI
BOOLEAN
NTAPI
RtlDispatchException
(IN PEXCEPTION_RECORD ExceptionRecord,
 IN PCONTEXT Context)
{
    // XXX arty fixme
    return TRUE;
}

NTSYSAPI
PVOID
RtlpGetExceptionAddress()
{
    // XXX arty fixme
    return NULL;
}

VOID
NTAPI
RtlUnwind(IN PVOID TargetFrame OPTIONAL,
          IN PVOID TargetIp OPTIONAL,
          IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
          IN PVOID ReturnValue)
{
    // XXX arty fixme
}

NTSYSAPI
VOID
NTAPI
RtlGetCallersAddress(
    OUT PVOID  *CallersAddress,
    OUT PVOID  *CallersCaller)
{
}
