#include <gdi32_vista.h>

/***********************************************************************
 *           D3DKMTCreateDCFromMemory    (GDI32.@)
 */
DWORD WINAPI D3DKMTCreateDCFromMemory( D3DKMT_CREATEDCFROMMEMORY *desc )
{
    return NtGdiDdDDICreateDCFromMemory( desc );
}

DWORD WINAPI D3DKMTDestroyDCFromMemory( const D3DKMT_DESTROYDCFROMMEMORY *desc )
{
    return NtGdiDdDDIDestroyDCFromMemory( desc );
}
