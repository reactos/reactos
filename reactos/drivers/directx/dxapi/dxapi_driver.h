


/* DDK/NDK/SDK Headers */
/* DDK/NDK/SDK Headers */
#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include <ddk/ntifs.h>
#include <ddk/tvout.h>
#include <ndk/ntndk.h>



#include <stdarg.h>
#include <windef.h>
#include <winerror.h>
#include <wingdi.h>
#include <winddi.h>
#include <winuser.h>
#include <prntfont.h>
#include <dde.h>
#include <wincon.h>

#include <ddk/ddkmapi.h>

/* Prototypes */
VOID DxGetVersionNumber(PVOID lpvInBuffer, LPDDGETVERSIONNUMBER lpvOutBuffer);
VOID DxCloseHandle(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxOpenDirectDraw(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxOpenSurface(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxOpenVideoPort(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxGetKernelCaps(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxGetFieldNumber(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxSetFieldNumber(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxSetSkipPattern(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxGetSurfaceState(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxSetSurfaceState(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxLock(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxFlipOverlay(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxFlipVideoPort(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxGetCurrentAutoflip(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxGetPreviousAutoflip(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxRegisterEvent(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxUnregisterEvent(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxGetPolarity(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxOpenVpCatureDevice(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxAddVpCaptureBuffer(PVOID lpvInBuffer, PVOID lpvOutBuffer);
VOID DxFlushVpCaptureBuffs(PVOID lpvInBuffer, PVOID lpvOutBuffer);

/* Internal driver table being use as looking up table for correct size of structs */
DWORD tblCheckInBuffer [] =
{
    /* DD_DXAPI_GETVERSIONNUMBER */
    0,
    /* DD_DXAPI_CLOSEHANDLE */
    sizeof(DDCLOSEHANDLE),
    0, // DD_DXAPI_OPENDIRECTDRAW
    0, // DD_DXAPI_OPENSURFACE
    0, // DD_DXAPI_OPENVIDEOPORT
    /* DD_DXAPI_GETKERNELCAPS */
    sizeof(HANDLE),
    /* DD_DXAPI_GET_VP_FIELD_NUMBER */
    sizeof(DDGETFIELDNUMIN),
    0, // DD_DXAPI_SET_VP_FIELD_NUMBER
    0, // DD_DXAPI_SET_VP_SKIP_FIELD
    0, // DD_DXAPI_GET_SURFACE_STATE
    0, // DD_DXAPI_SET_SURFACE_STATE
    0, // DD_DXAPI_LOCK
    /* DD_DXAPI_FLIP_OVERLAY */
    sizeof(DDFLIPOVERLAY),
    /* DD_DXAPI_FLIP_VP */
    sizeof(DDFLIPVIDEOPORT),
    /* DD_DXAPI_GET_CURRENT_VP_AUTOFLIP_SURFACE */
    sizeof(DDGETAUTOFLIPIN),
    /* DD_DXAPI_GET_LAST_VP_AUTOFLIP_SURFACE */
    sizeof(DDGETAUTOFLIPIN),
    /* DD_DXAPI_REGISTER_CALLBACK */
    sizeof(DDREGISTERCALLBACK),
    /* DD_DXAPI_UNREGISTER_CALLBACK */
    sizeof(DDREGISTERCALLBACK),
    /* DD_DXAPI_GET_POLARITY */
    sizeof(DDGETPOLARITYIN),
    /* DD_DXAPI_OPENVPCAPTUREDEVICE */
    sizeof(DDOPENVPCAPTUREDEVICEIN),
    /* DD_DXAPI_ADDVPCAPTUREBUFFER */
    sizeof(DDADDVPCAPTUREBUFF),
    /* DD_DXAPI_FLUSHVPCAPTUREBUFFERS */
    sizeof(HANDLE)
};

DWORD tblCheckOutBuffer [] =
{
    /* DD_DXAPI_GETVERSIONNUMBER */
    sizeof(DDGETVERSIONNUMBER),
    /* DD_DXAPI_CLOSEHANDLE */
    sizeof(DWORD),
    0, // DD_DXAPI_OPENDIRECTDRAW
    0, // DD_DXAPI_OPENSURFACE
    0, // DD_DXAPI_OPENVIDEOPORT
    /* DD_DXAPI_GETKERNELCAPS */
    sizeof(DDGETKERNELCAPSOUT),
    /* DD_DXAPI_GET_VP_FIELD_NUMBER */
    sizeof(DDGETFIELDNUMOUT),
    0, // DD_DXAPI_SET_VP_FIELD_NUMBER
    0, // DD_DXAPI_SET_VP_SKIP_FIELD
    0, // DD_DXAPI_GET_SURFACE_STATE
    0, // DD_DXAPI_SET_SURFACE_STATE
    0, // DD_DXAPI_LOCK
    /* DD_DXAPI_FLIP_OVERLAY */
    sizeof(DWORD),
    /* DD_DXAPI_FLIP_VP */
    sizeof(DWORD),
    /* DD_DXAPI_GET_CURRENT_VP_AUTOFLIP_SURFACE */
    sizeof(DDGETAUTOFLIPOUT),
    /* DD_DXAPI_GET_LAST_VP_AUTOFLIP_SURFACE */
    sizeof(DDGETAUTOFLIPOUT),
    /* DD_DXAPI_REGISTER_CALLBACK */
    sizeof(DWORD),
    /* DD_DXAPI_UNREGISTER_CALLBACK */
    sizeof(DWORD),
    /* DD_DXAPI_GET_POLARITY */
    sizeof(DDGETPOLARITYOUT),
    /* DD_DXAPI_OPENVPCAPTUREDEVICE */
    sizeof(DDOPENVPCAPTUREDEVICEOUT),
    /* DD_DXAPI_ADDVPCAPTUREBUFFER */
    sizeof(DWORD),
    /* DD_DXAPI_FLUSHVPCAPTUREBUFFERS */
    sizeof(DWORD)
};


/* Internal driver function */
DRVFN gDxApiEntryPoint [] = 
{
    {DD_DXAPI_GETVERSIONNUMBER - DD_FIRST_DXAPI, (PFN) DxGetVersionNumber},
    {DD_DXAPI_CLOSEHANDLE - DD_FIRST_DXAPI, (PFN) DxCloseHandle},
    {DD_DXAPI_OPENDIRECTDRAW - DD_FIRST_DXAPI, (PFN) DxOpenDirectDraw},
    {DD_DXAPI_OPENSURFACE - DD_FIRST_DXAPI, (PFN) DxOpenSurface},
    {DD_DXAPI_OPENVIDEOPORT - DD_FIRST_DXAPI, (PFN) DxOpenVideoPort},
    {DD_DXAPI_GETKERNELCAPS - DD_FIRST_DXAPI, (PFN) DxGetKernelCaps},
    {DD_DXAPI_GET_VP_FIELD_NUMBER - DD_FIRST_DXAPI, (PFN) DxGetFieldNumber},
    {DD_DXAPI_SET_VP_FIELD_NUMBER - DD_FIRST_DXAPI, (PFN) DxSetFieldNumber},
    {DD_DXAPI_SET_VP_SKIP_FIELD - DD_FIRST_DXAPI, (PFN) DxSetSkipPattern},
    {DD_DXAPI_GET_SURFACE_STATE - DD_FIRST_DXAPI, (PFN) DxGetSurfaceState},
    {DD_DXAPI_SET_SURFACE_STATE - DD_FIRST_DXAPI, (PFN) DxSetSurfaceState},
    {DD_DXAPI_LOCK - DD_FIRST_DXAPI, (PFN) DxLock},
    {DD_DXAPI_FLIP_OVERLAY - DD_FIRST_DXAPI, (PFN) DxFlipOverlay},
    {DD_DXAPI_FLIP_VP - DD_FIRST_DXAPI, (PFN) DxFlipVideoPort},
    {DD_DXAPI_GET_CURRENT_VP_AUTOFLIP_SURFACE - DD_FIRST_DXAPI, (PFN) DxGetCurrentAutoflip},
    {DD_DXAPI_GET_LAST_VP_AUTOFLIP_SURFACE - DD_FIRST_DXAPI, (PFN) DxGetPreviousAutoflip},
    {DD_DXAPI_REGISTER_CALLBACK - DD_FIRST_DXAPI, (PFN) DxRegisterEvent},
    {DD_DXAPI_UNREGISTER_CALLBACK - DD_FIRST_DXAPI, (PFN) DxUnregisterEvent},
    {DD_DXAPI_GET_POLARITY - DD_FIRST_DXAPI, (PFN) DxGetPolarity},
    {DD_DXAPI_OPENVPCAPTUREDEVICE - DD_FIRST_DXAPI, (PFN) DxOpenVpCatureDevice},
    {DD_DXAPI_ADDVPCAPTUREBUFFER - DD_FIRST_DXAPI, (PFN) DxAddVpCaptureBuffer},
    {DD_DXAPI_FLUSHVPCAPTUREBUFFERS - DD_FIRST_DXAPI, (PFN) DxFlushVpCaptureBuffs}
};



