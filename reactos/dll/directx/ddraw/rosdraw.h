#ifndef __DDRAW_PRIVATE
#define __DDRAW_PRIVATE

/********* Includes  *********/
#include <windows.h>
#include <stdio.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <d3dhal.h>
#include <ddrawgdi.h>

#include "Ddraw/ddraw.h"
#include "Surface/surface.h"
#include "Clipper/clipper.h"

#include "resource.h"

/* DirectDraw startup code only internal use  */
extern DDRAWI_DIRECTDRAW_GBL ddgbl;
extern DDRAWI_DDRAWSURFACE_GBL ddSurfGbl;
extern WCHAR classname[128];
extern WNDCLASSW wnd_class;
extern CRITICAL_SECTION ddcs;
extern IDirectDraw7Vtbl DirectDraw7_Vtable;
extern IDirectDraw4Vtbl DirectDraw4_Vtable;
extern IDirectDraw2Vtbl DirectDraw2_Vtable;
extern IDirectDrawVtbl DirectDraw_Vtable;


extern IDirectDrawSurface7Vtbl DirectDrawSurface7_Vtable;
extern IDirectDrawSurface4Vtbl DirectDrawSurface4_Vtable;
extern IDirectDrawSurface3Vtbl DirectDrawSurface3_Vtable;
extern IDirectDrawSurface2Vtbl DirectDrawSurface2_Vtable;
extern IDirectDrawSurfaceVtbl DirectDrawSurface_Vtable;
extern IDirectDrawPaletteVtbl DirectDrawPalette_Vtable;
extern IDirectDrawClipperVtbl DirectDrawClipper_Vtable;
extern IDirectDrawColorControlVtbl DirectDrawColorControl_Vtable;
extern IDirectDrawGammaControlVtbl DirectDrawGammaControl_Vtable;
extern IDirectDrawKernelVtbl        DirectDrawKernel_Vtable;
extern IDirectDrawSurfaceKernelVtbl DirectDrawSurfaceKernel_Vtable;

/* Start up direct hal or hel
 * iface = a pointer to the com object
 * pGUID = guid hardware acclations or software acclation this can  be NULL
 * reenable = FALSE if we whant create a new directdraw com
 *          = TRUE if we really whant rebuild the whole com interface (not in use)
 */

HRESULT WINAPI
StartDirectDraw(
                LPDIRECTDRAW iface,
                LPGUID pGUID,
                BOOL reenable);

/* iface = a pointer to the com object
 * reenable = FALSE / TRUE rebuld dx hal interface, this is need if we doing a mode change
 */

HRESULT WINAPI
StartDirectDrawHal(
                   LPDIRECTDRAW iface,
                   BOOL reenable);

/* iface = a pointer to the com object
 * reenable = FALSE / TRUE rebuld dx hel interface, this is need if we doing a mode change
 */

HRESULT WINAPI
StartDirectDrawHel(
                   LPDIRECTDRAW iface,
                   BOOL reenable);

HRESULT WINAPI
Create_DirectDraw (
                   LPGUID pGUID,
                   LPDIRECTDRAW* pIface,
                   REFIID id,
                   BOOL ex);

HRESULT WINAPI
ReCreateDirectDraw(LPDIRECTDRAW iface);
HRESULT
Internal_CreateSurface(
                       LPDDRAWI_DIRECTDRAW_INT pDDraw,
                       LPDDSURFACEDESC2 pDDSD,
                       LPDIRECTDRAWSURFACE7 *ppSurf,
                       IUnknown *pUnkOuter);

/* convert DDSURFACEDESC to DDSURFACEDESC2 */
void CopyDDSurfDescToDDSurfDesc2(LPDDSURFACEDESC2 dst_pDesc, LPDDSURFACEDESC src_pDesc);

/* DirectDraw Cleanup code only internal use */
VOID Cleanup(LPDDRAWI_DIRECTDRAW_INT iface);

/* own macro to alloc memmory */

/*
#define DxHeapMemAlloc(m)  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m)
#define DxHeapMemFree(p)   HeapFree(GetProcessHeap(), 0, p); \
                           p = NULL;
*/
#define DxHeapMemAlloc(p, m)  { \
                                p = malloc(m); \
                                if (p != NULL) \
                                { \
                                    ZeroMemory(p,m); \
                                } \
                              }
#define DxHeapMemFree(p)   { \
                             free(p); \
                             p = NULL; \
                           }

/******** Main Object ********/

/* Public interface */
VOID WINAPI AcquireDDThreadLock();
VOID WINAPI ReleaseDDThreadLock();

ULONG WINAPI  DirectDrawClipper_AddRef (LPDIRECTDRAWCLIPPER iface);
HRESULT WINAPI  DirectDrawClipper_Initialize( LPDIRECTDRAWCLIPPER iface, LPDIRECTDRAW lpDD, DWORD dwFlags);

typedef struct DDRAWI_DDCOLORCONTROL_INT
{
  LPVOID  lpVtbl;
  LPVOID  lpLcl;
  LPVOID  lpLink;
  DWORD  dwIntRefCnt;
} DDRAWI_DDCOLORCONTROL_INT, *LPDDRAWI_DDCOLORCONTROL_INT;

typedef struct _DDRAWI_DDGAMMACONTROL_INT
{
  LPVOID  lpVtbl;
  LPVOID  lpLcl;
  LPVOID  lpLink;
  DWORD  dwIntRefCnt;
} DDRAWI_DDGAMMACONTROL_INT, *LPDDRAWI_DDGAMMACONTROL_INT;

typedef struct _DDRAWI_DDKERNEL_INT
{
  LPVOID  lpVtbl;
  LPVOID  lpLcl;
  LPVOID  lpLink;
  DWORD  dwIntRefCnt;
} DDRAWI_KERNEL_INT, *LPDDRAWI_KERNEL_INT;

typedef struct _DDRAWI_DDKERNELSURFACE_INT
{
  LPVOID  lpVtbl;
  LPVOID  lpLcl;
  LPVOID  lpLink;
  DWORD  dwIntRefCnt;
} _DDRAWI_DDKERNELSURFACE_INT, *LPDDRAWI_DDKERNELSURFACE_INT;

/* now to real info that are for private use and are our own */








/********* Prototypes **********/
VOID Hal_DirectDraw_Release (LPDIRECTDRAW7);

/* Setting for HEL should be move to ros special reg key ? */

/* setup how much graphic memory should hel be limit, set it now to 64MB */
#define HEL_GRAPHIC_MEMORY_MAX 67108864

/*********** Macros ***********/

/*
   use this macro to close
   down the debuger text complete
   no debuging at all, it will
   crash ms debuger in VS
*/

//#define DX_WINDBG_trace()
//#define DX_STUB
//#define DX_STUB_DD_OK return DD_OK;
//#define DX_STUB_str(x)
//#define DX_WINDBG_trace_res


/*
   Use this macro if you want deboug in visual studio or
   if you have a program to look at the _INT struct from
   ReactOS ddraw.dll or ms ddraw.dll, so you can see what
   value ms are being setup.

   This macro will create allot warings and can not be help when you compile
*/


//#define DX_WINDBG_trace()
//#define DX_STUB
//#define DX_STUB_DD_OK return DD_OK;
//#define DX_STUB_str(x) printf("%s",x);
//#define DX_WINDBG_trace_res

/*
   use this if want doing a trace from a program
   like a game and ReactOS ddraw.dll in windows
   so you can figout what going wrong and what
   api are being call or if it hel or is it hal

   This marco does not create warings when you compile
*/

#define DX_STUB \
{ \
	static BOOL firstcall = TRUE; \
	if (firstcall) \
	{ \
		char buffer[1024]; \
		sprintf ( buffer, "Function %s is not implemented yet (%s:%d)\n", __FUNCTION__,__FILE__,__LINE__ ); \
		OutputDebugStringA(buffer); \
		firstcall = FALSE; \
	} \
} \
	return DDERR_UNSUPPORTED;



#define DX_STUB_DD_OK \
{ \
	static BOOL firstcall = TRUE; \
	if (firstcall) \
	{ \
		char buffer[1024]; \
		sprintf ( buffer, "Function %s is not implemented yet (%s:%d)\n", __FUNCTION__,__FILE__,__LINE__ ); \
		OutputDebugStringA(buffer); \
		firstcall = FALSE; \
	} \
} \
	return DD_OK;


#if 1
    #define DX_STUB_str(x) \
    { \
        char buffer[1024]; \
        sprintf ( buffer, "Function %s %s (%s:%d)\n", __FUNCTION__,x,__FILE__,__LINE__ ); \
        OutputDebugStringA(buffer); \
    }


    #define DX_WINDBG_trace() \
        static BOOL firstcallx = TRUE; \
        if (firstcallx) \
        { \
            char buffer[1024]; \
            sprintf ( buffer, "Enter Function %s (%s:%d)\n", __FUNCTION__,__FILE__,__LINE__ ); \
            OutputDebugStringA(buffer); \
            firstcallx = TRUE; \
        }


#define DX_WINDBG_trace_res(width,height,bpp, freq) \
    static BOOL firstcallxx = TRUE; \
    if (firstcallxx) \
    { \
        char buffer[1024]; \
        sprintf ( buffer, "Setmode have been req width=%d, height=%d bpp=%d freq = %d\n",width,height,bpp, freq); \
        OutputDebugStringA(buffer); \
        firstcallxx = TRUE; \
    }
#else
    #define DX_WINDBG_trace() //
    #define DX_WINDBG_trace_res(width,height,bpp, freq) \\

    #define DX_STUB_str(x) //

#endif

#endif /* __DDRAW_PRIVATE */
