/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Runtime Library
 * PURPOSE:         Activation Context Support
 * FILE:            lib/rtl/actctx.c
 * PROGRAMERS:      Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

#define QUERY_ACTCTX_FLAG_ACTIVE (0x00000001)

/* FUNCTIONS ***************************************************************/

VOID
NTAPI
RtlAddRefActivationContext(PVOID Context)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
RtlActivateActivationContextUnsafeFast(IN PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED Frame,
                                       IN PVOID Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlAllocateActivationContextStack(IN PVOID *Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlGetActiveActivationContext(IN PVOID *Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
RtlReleaseActivationContext(IN PVOID *Context)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
RtlFindActivationContextSectionString(IN ULONG dwFlags,
                                      IN const GUID *lpExtensionGuid,
                                      IN ULONG SectionType,
                                      IN PUNICODE_STRING SectionName,
                                      IN PVOID ReturnedData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlQueryInformationActivationContext(DWORD dwFlags,
                                     PVOID Context,
                                     PVOID pvSubInstance,
                                     ULONG ulInfoClass,
                                     PVOID pvBuffer,
                                     SIZE_T cbBuffer OPTIONAL,
                                     SIZE_T *pcbWrittenOrRequired OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlQueryInformationActiveActivationContext(ULONG ulInfoClass,
                                           PVOID pvBuffer,
                                           SIZE_T cbBuffer OPTIONAL,
                                           SIZE_T *pcbWrittenOrRequired OPTIONAL)
{
    return RtlQueryInformationActivationContext(QUERY_ACTCTX_FLAG_ACTIVE,
                                                NULL,
                                                NULL,
                                                ulInfoClass,
                                                pvBuffer,
                                                cbBuffer,
                                                pcbWrittenOrRequired);
}

NTSTATUS
NTAPI
RtlZombifyActivationContext(PVOID Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlDeactivateActivationContext(DWORD dwFlags,
                               ULONG_PTR ulCookie)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlDeactivateActivationContextUnsafeFast(IN PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED Frame)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
