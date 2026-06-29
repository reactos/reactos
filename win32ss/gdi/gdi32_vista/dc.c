#include <gdi32_vista.h>

/***********************************************************************
 *           D3DKMTCreateDCFromMemory    (GDI32.@)
 */
NTSTATUS APIENTRY D3DKMTCreateDCFromMemory(_Inout_ D3DKMT_CREATEDCFROMMEMORY* desc)
{
    return NtGdiDdDDICreateDCFromMemory( desc );
}

NTSTATUS APIENTRY D3DKMTDestroyDCFromMemory(_In_ CONST D3DKMT_DESTROYDCFROMMEMORY* desc)
{
    return NtGdiDdDDIDestroyDCFromMemory( desc );
}
