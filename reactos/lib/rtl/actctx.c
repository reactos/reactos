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

#define ACTCTX_FLAGS_ALL (\
 ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID |\
 ACTCTX_FLAG_LANGID_VALID |\
 ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID |\
 ACTCTX_FLAG_RESOURCE_NAME_VALID |\
 ACTCTX_FLAG_SET_PROCESS_DEFAULT |\
 ACTCTX_FLAG_APPLICATION_NAME_VALID |\
 ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF |\
 ACTCTX_FLAG_HMODULE_VALID )

#define ACTCTX_FAKE_HANDLE ((HANDLE) 0xf00baa)
#define ACTCTX_FAKE_COOKIE ((ULONG_PTR) 0xf00bad)

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

    *Context = ACTCTX_FAKE_HANDLE;

    return STATUS_SUCCESS;
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
                                      IN const GUID *ExtensionGuid,
                                      IN ULONG SectionType,
                                      IN PUNICODE_STRING SectionName,
                                      IN OUT PVOID ReturnedData)
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

    if (ulCookie == ACTCTX_FAKE_COOKIE)
        return STATUS_SUCCESS;

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlDeactivateActivationContextUnsafeFast(IN PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED Frame)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlActivateActivationContext(IN ULONG Unknown, IN HANDLE Handle, OUT PULONG_PTR Cookie)
{
    UNIMPLEMENTED;

    if (Cookie)
        *Cookie = ACTCTX_FAKE_COOKIE;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlCreateActivationContext(OUT PHANDLE Handle, IN OUT PVOID ReturnedData)
{
    PCACTCTXW pActCtx = (PCACTCTXW) ReturnedData;

    UNIMPLEMENTED;

    if (!pActCtx)
        *Handle = INVALID_HANDLE_VALUE;
    if (pActCtx->cbSize != sizeof *pActCtx)
        *Handle = INVALID_HANDLE_VALUE;
    if (pActCtx->dwFlags & ~ACTCTX_FLAGS_ALL)
        *Handle = INVALID_HANDLE_VALUE;
    *Handle = ACTCTX_FAKE_HANDLE;

    return STATUS_SUCCESS;
}
