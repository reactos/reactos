#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <ndk/rtlfuncs.h>
#include <wingdi.h>
#include <winddi.h>
#include <prntfont.h>
#include <ntgdityp.h>
#include <ntgdi.h>

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
