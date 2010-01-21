
#include <ntdll.h>
#define NDEBUG
#include <debug.h>

#include "wine/unicode.h"


/***********************************************************************
 *           create_module_activation_context
 */
NTSTATUS create_module_activation_context( LDR_DATA_TABLE_ENTRY *module )
{
    NTSTATUS status;
    LDR_RESOURCE_INFO info;
    IMAGE_RESOURCE_DATA_ENTRY *entry;

    info.Type = (ULONG)RT_MANIFEST;
    info.Name = (ULONG)ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
    info.Language = 0;
    if (!(status = LdrFindResource_U( module->DllBase, &info, 3, &entry )))
    {
        ACTCTXW ctx;
        ctx.cbSize   = sizeof(ctx);
        ctx.lpSource = NULL;
        ctx.dwFlags  = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
        ctx.hModule  = module->DllBase;
        ctx.lpResourceName = (LPCWSTR)ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
        status = RtlCreateActivationContext( &module->EntryPointActivationContext, &ctx );
    }
    return status;
}

NTSTATUS find_actctx_dll( LPCWSTR libname, WCHAR *fullname )
{
    static const WCHAR winsxsW[] = {'\\','w','i','n','s','x','s','\\',0};
    /* FIXME: Handle modules that have a private manifest file
    static const WCHAR dotManifestW[] = {'.','m','a','n','i','f','e','s','t',0}; */

    ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION *info;
    ACTCTX_SECTION_KEYED_DATA data;
    UNICODE_STRING nameW;
    NTSTATUS status;
    SIZE_T needed, size = 1024;

    RtlInitUnicodeString( &nameW, libname );
    data.cbSize = sizeof(data);
    status = RtlFindActivationContextSectionString( FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                                                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                                    &nameW, &data );
    if (status != STATUS_SUCCESS) return status;

    for (;;)
    {
        if (!(info = RtlAllocateHeap( RtlGetProcessHeap(), 0, size )))
        {
            status = STATUS_NO_MEMORY;
            goto done;
        }
        status = RtlQueryInformationActivationContext( 0, data.hActCtx, &data.ulAssemblyRosterIndex,
                                                       AssemblyDetailedInformationInActivationContext,
                                                       info, size, &needed );
        if (status == STATUS_SUCCESS) break;
        if (status != STATUS_BUFFER_TOO_SMALL) goto done;
        RtlFreeHeap( RtlGetProcessHeap(), 0, info );
        size = needed;
    }

    DPRINT1("manafestpath === %S\n", info->lpAssemblyManifestPath);
    DPRINT1("DirectoryName === %S\n", info->lpAssemblyDirectoryName);
    if (!info->lpAssemblyManifestPath || !info->lpAssemblyDirectoryName)
    {
        status = STATUS_SXS_KEY_NOT_FOUND;
        goto done;
    }

    DPRINT("%S. %S\n", info->lpAssemblyManifestPath, info->lpAssemblyDirectoryName);
    wcscpy(fullname , SharedUserData->NtSystemRoot);
    wcscat(fullname, winsxsW);
    wcscat(fullname, info->lpAssemblyDirectoryName);
    wcscat(fullname, L"\\");
    wcscat(fullname, libname);
    DPRINT("Successfully found a side by side %S\n", fullname);
    status = STATUS_SUCCESS;

done:
    RtlFreeHeap( RtlGetProcessHeap(), 0, info );
    RtlReleaseActivationContext( data.hActCtx );
    DPRINT("%S\n", fullname);
    return status;
}
