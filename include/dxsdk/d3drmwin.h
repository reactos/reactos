
#ifndef __D3DRMWIN_H__
#define __D3DRMWIN_H__

#ifndef WIN32
#define WIN32
#endif

#include "d3drm.h"
#include "ddraw.h"
#include "d3d.h"

#undef INTERFACE
#define INTERFACE IDirect3DRMWinDevice

DECLARE_INTERFACE_(IDirect3DRMWinDevice, IDirect3DRMObject)
{
  IUNKNOWN_METHODS(PURE);
  IDIRECT3DRMOBJECT_METHODS(PURE);
  STDMETHOD(HandlePaint) (THIS_ HDC hdc) PURE;
  STDMETHOD(HandleActivate) (THIS_ WORD wparam) PURE;
};

DEFINE_GUID(IID_IDirect3DRMWinDevice, 0xC5016CC0, 0xD273, 0x11CE, 0xAC, 0x48, 0x0, 0x0, 0xC0, 0x38, 0x25, 0xA1);
WIN_TYPES(IDirect3DRMWinDevice, DIRECT3DRMWINDEVICE);

#endif

