
#ifndef __AMAUDIO__
#define __AMAUDIO__

#ifdef __cplusplus
extern "C" {
#endif

#include <mmsystem.h>
#include <dsound.h>

#undef INTERFACE
#define INTERFACE IAMDirectSound

DECLARE_INTERFACE_(IAMDirectSound,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(GetDirectSoundInterface)(THIS_ LPDIRECTSOUND *lplpds) PURE;
  STDMETHOD(GetPrimaryBufferInterface)(THIS_ LPDIRECTSOUNDBUFFER *lplpdsb) PURE;
  STDMETHOD(GetSecondaryBufferInterface)(THIS_ LPDIRECTSOUNDBUFFER *lplpdsb) PURE;
  STDMETHOD(ReleaseDirectSoundInterface)(THIS_ LPDIRECTSOUND lpds) PURE;
  STDMETHOD(ReleasePrimaryBufferInterface)(THIS_ LPDIRECTSOUNDBUFFER lpdsb) PURE;
  STDMETHOD(ReleaseSecondaryBufferInterface)(THIS_ LPDIRECTSOUNDBUFFER lpdsb) PURE;
  STDMETHOD(SetFocusWindow)(THIS_ HWND, BOOL) PURE ;
  STDMETHOD(GetFocusWindow)(THIS_ HWND *, BOOL*) PURE ;
};
#undef INTERFACE

#ifdef __cplusplus
}
#endif
#endif
