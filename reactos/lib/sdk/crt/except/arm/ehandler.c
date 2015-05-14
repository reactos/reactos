/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            lib/sdk/crt/except/ehandler.c
* PURPOSE:         Low level exception handler functions
* PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
*/

/* INCLUDES *****************************************************************/

#include <precomp.h>

_CRTIMP
EXCEPTION_DISPOSITION
__cdecl
__C_specific_handler(
    struct _EXCEPTION_RECORD *_ExceptionRecord,
    void *_EstablisherFrame,
    struct _CONTEXT *_ContextRecord,
    struct _DISPATCHER_CONTEXT *_DispatcherContext)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0;
}

DWORD
__CxxFrameHandler3(
    PEXCEPTION_RECORD rec,
    EXCEPTION_REGISTRATION_RECORD* ExceptionRegistrationFrame,
    PCONTEXT context,
    EXCEPTION_REGISTRATION_RECORD** _ExceptionRecord)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0;
}

