/* 
 * Direct3D NT driver interface
 */

#ifndef __DDK_D3DNTHAL_H
#define __DDK_D3DNTHAL_H

#include <ddk/ddrawint.h>
#include <d3dtypes.h>
#include <d3dcaps.h>

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID(GUID_D3DCallbacks,                   0x7BF06990, 0x8794, 0x11D0, 0x91, 0x39, 0x08, 0x00, 0x36, 0xD2, 0xEF, 0x02);
DEFINE_GUID(GUID_D3DCallbacks3,                  0xDDF41230, 0xEC0A, 0x11D0, 0xA9, 0xB6, 0x00, 0xAA, 0x00, 0xC0, 0x99, 0x3E);
DEFINE_GUID(GUID_D3DExtendedCaps,                0x7DE41F80, 0x9D93, 0x11D0, 0x89, 0xAB, 0x00, 0xA0, 0xC9, 0x05, 0x41, 0x29);
DEFINE_GUID(GUID_D3DParseUnknownCommandCallback, 0x2E04FFA0, 0x98E4, 0x11D1, 0x8C, 0xE1, 0x00, 0xA0, 0xC9, 0x06, 0x29, 0xA8);
DEFINE_GUID(GUID_ZPixelFormats,                  0x93869880, 0x36CF, 0x11D1, 0x9B, 0x1B, 0x00, 0xAA, 0x00, 0xBB, 0xB8, 0xAE);
DEFINE_GUID(GUID_DDStereoMode,                   0xF828169C, 0xA8E8, 0x11D2, 0xA1, 0xF2, 0x00, 0xA0, 0xC9, 0x83, 0xEA, 0xF6);

typedef struct _D3DNTHAL_CONTEXTCREATEDATA *PD3DNTHAL_CONTEXTCREATEDATA;
typedef struct _D3DNTHAL_CONTEXTDESTROYDATA *PD3DNTHAL_CONTEXTDESTROYDATA;
typedef struct _D3DNTHAL_DRAWPRIMITIVES2DATA *PD3DNTHAL_DRAWPRIMITIVES2DATA;
typedef struct _D3DNTHAL_VALIDATETEXTURESTAGESTATEDATA *PD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA;

/* DIRECT3D object callbacks */
typedef DWORD (STDCALL *PD3DNTHAL_CONTEXTCREATECB)(PD3DNTHAL_CONTEXTCREATEDATA);
typedef DWORD (STDCALL *PD3DNTHAL_CONTEXTDESTROYCB)(PD3DNTHAL_CONTEXTDESTROYDATA);

typedef struct _D3DNTHAL_CALLBACKS {
	DWORD                      dwSize;
	PD3DNTHAL_CONTEXTCREATECB  ContextCreate;
	PD3DNTHAL_CONTEXTDESTROYCB ContextDestroy;
	PVOID                      Reserved[32];
} D3DNTHAL_CALLBACKS;
typedef D3DNTHAL_CALLBACKS *PD3DNTHAL_CALLBACKS;

/* Structures to report driver capabilities */

typedef struct _D3DNTHAL_DEVICEDESC {
	DWORD            dwSize;
	DWORD            dwFlags;
	D3DCOLORMODEL    dcmColorModel;
	DWORD            dwDevCaps;
	D3DTRANSFORMCAPS dtcTransformCaps;
	BOOL             bClipping;
	D3DLIGHTINGCAPS  dlcLightingCaps;
	D3DPRIMCAPS      dpcLineCaps;
	D3DPRIMCAPS      dpcTriCaps;
	DWORD            dwDeviceRenderBitDepth;
	DWORD            dwDeviceZBufferBitDepth;
	DWORD            dwMaxBufferSize;
	DWORD            dwMaxVertexCount;
} D3DNTHAL_DEVICEDESC, *PD3DNT_HALDEVICEDESC;

typedef struct _D3DNTHAL_GLOBALDRIVERDATA {
	DWORD               dwSize;
	D3DNTHAL_DEVICEDESC hwCaps;
	DWORD               dwNumVertices;
	DWORD               dwNumClipVertices;
	DWORD               dwNumTextureFormats;
	LPDDSURFACEDESC     lpTextureFormats;
} D3DNTHAL_GLOBALDRIVERDATA, *PD3DNTHAL_GLOBALDRIVERDATA;


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __DDK_D3DNTHAL_H */
