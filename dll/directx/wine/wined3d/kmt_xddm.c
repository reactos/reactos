

#include "wined3d_private.h"
#include "wined3d_gl.h"

#include <d3dkmddi.h>

/* TODO: these can be fully implemented later, 
 * but they're > Vista for now so let's do this so we can test our code against vista.
 * https://github.com/wine-mirror/wine/commit/4a98b07c4bcc35a698448261478ba856c149cbea
 * this function is also primarily just used in wine so accurate vram data can be accessed. Nice!
*/
NTSTATUS
WINAPI
D3DKMTOpenAdapterFromLuid_wined3d(_Inout_ CONST D3DKMT_OPENADAPTERFROMLUID* unnamedParam1)
{
    DbgBreakPoint();
    return 0;
}

NTSTATUS
WINAPI
D3DKMTQueryVideoMemoryInfo_wined3d(
    D3DKMT_QUERYVIDEOMEMORYINFO *unnamedParam1)
{
    /* fallback here is perfectly fine! */
    return 1;
}