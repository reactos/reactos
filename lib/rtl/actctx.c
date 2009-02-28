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


/* INTERNAL FUNCTIONS *******************************************************/

static NTSTATUS open_nt_file( HANDLE *handle, UNICODE_STRING *name )
{
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    return NtOpenFile( handle, GENERIC_READ, &attr, &io, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT );
}


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
    /*
    if (NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame)
    {
        *Context = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame->ActivationContext;
        RtlAddRefActivationContext(*Context);
    }
    else
        *Context = 0;
    */
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
    PACTCTX_SECTION_KEYED_DATA Data = ReturnedData;

    UNIMPLEMENTED;

    if (!Data || Data->cbSize < offsetof(ACTCTX_SECTION_KEYED_DATA, ulAssemblyRosterIndex) ||
        !SectionName || !SectionName->Buffer)
    {
        return STATUS_INVALID_PARAMETER;
    }

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

    if (Context == ACTCTX_FAKE_HANDLE)
        return STATUS_SUCCESS;

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
    /*
    PRTL_ACTIVATION_CONTEXT_STACK_FRAME Frame;

    Frame = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*Frame) );
    if (!Frame)
        return STATUS_NO_MEMORY;

    Frame->Previous = NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame;
    Frame->ActivationContext = Handle;
    Frame->Flags = 0;
    NtCurrentTeb()->ActivationContextStackPointer->ActiveFrame = Frame;
    RtlAddRefActivationContext(Handle);

    *Cookie = (ULONG_PTR)Frame;
    DPRINT( "%p Cookie=%lx\n", Handle, *Cookie );
    */

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
    UNICODE_STRING NameW;
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    UNIMPLEMENTED;

    if (!pActCtx || (pActCtx->cbSize < sizeof(*pActCtx)) ||
        (pActCtx->dwFlags & ~ACTCTX_FLAGS_ALL))
        return STATUS_INVALID_PARAMETER;
  
    NameW.Buffer = NULL;
    if (pActCtx->lpSource)
    {
        if (!RtlDosPathNameToNtPathName_U(pActCtx->lpSource, &NameW, NULL, NULL))
        {
            Status = STATUS_NO_SUCH_FILE;
            goto Error;
        }
        Status = open_nt_file( &hFile, &NameW );
        if (!NT_SUCCESS(Status))
        {
            RtlFreeUnicodeString( &NameW );
            goto Error;
        }
    }

    if (pActCtx->dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID)
    {
        /* if we have a resource it's a PE file */
        if (pActCtx->dwFlags & ACTCTX_FLAG_HMODULE_VALID)
        {
        }
        else if (pActCtx->lpSource)
        {
        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
        }
	}
	else
	{
        /* manifest file */
    }

    if (hFile)
        NtClose( hFile );
    if (NameW.Buffer)
        RtlFreeUnicodeString( &NameW );

    *Handle = ACTCTX_FAKE_HANDLE;
    return Status;

Error:
    if (hFile) NtClose( hFile );
    return Status;

}
