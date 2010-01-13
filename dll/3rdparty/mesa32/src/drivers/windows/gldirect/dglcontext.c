/****************************************************************************
*
*                        Mesa 3-D graphics library
*                        Direct3D Driver Interface
*
*  ========================================================================
*
*   Copyright (C) 1991-2004 SciTech Software, Inc. All rights reserved.
*
*   Permission is hereby granted, free of charge, to any person obtaining a
*   copy of this software and associated documentation files (the "Software"),
*   to deal in the Software without restriction, including without limitation
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,
*   and/or sell copies of the Software, and to permit persons to whom the
*   Software is furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included
*   in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
*   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
*   SCITECH SOFTWARE INC BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
*   OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
*  ======================================================================
*
* Language:     ANSI C
* Environment:  Windows 9x (Win32)
*
* Description:  Context handling.
*
****************************************************************************/

#include "dglcontext.h"

// Get compile errors without this. KeithH
//#include "scitech.h"	// ibool, etc.

#ifdef _USE_GLD3_WGL
#include "gld_driver.h"

extern void _gld_mesa_warning(GLcontext *, char *);
extern void _gld_mesa_fatal(GLcontext *, char *);
#endif // _USE_GLD3_WGL

// TODO: Clean out old DX6-specific code from GLD 2.x CAD driver
// if it is no longer being built as part of GLDirect. (DaveM)

// ***********************************************************************

#define GLDERR_NONE     0
#define GLDERR_MEM      1
#define GLDERR_DDRAW    2
#define GLDERR_D3D      3
#define GLDERR_BPP      4

char szResourceWarning[] =
"GLDirect does not have enough video memory resources\n"
"to support the requested OpenGL rendering context.\n\n"
"You may have to reduce the current display resolution\n"
"to obtain satisfactory OpenGL performance.\n";

char szDDrawWarning[] =
"GLDirect is unable to initialize DirectDraw for the\n"
"requested OpenGL rendering context.\n\n"
"You will have to check the DirectX control panel\n"
"for further information.\n";

char szD3DWarning[] =
"GLDirect is unable to initialize Direct3D for the\n"
"requested OpenGL rendering context.\n\n"
"You may have to change the display mode resolution\n"
"color depth or check the DirectX control panel for\n"
"further information.\n";

char szBPPWarning[] =
"GLDirect is unable to use the selected color depth for\n"
"the requested OpenGL rendering context.\n\n"
"You will have to change the display mode resolution\n"
"color depth with the Display Settings control panel.\n";

int nContextError = GLDERR_NONE;

// ***********************************************************************

#define VENDORID_ATI 0x1002

static DWORD devATIRagePro[] = {
	0x4742, // 3D RAGE PRO BGA AGP 1X/2X
	0x4744, // 3D RAGE PRO BGA AGP 1X only
	0x4749, // 3D RAGE PRO BGA PCI 33 MHz
	0x4750, // 3D RAGE PRO PQFP PCI 33 MHz
	0x4751, // 3D RAGE PRO PQFP PCI 33 MHz limited 3D
	0x4C42, // 3D RAGE LT PRO BGA-312 AGP 133 MHz
	0x4C44, // 3D RAGE LT PRO BGA-312 AGP 66 MHz
	0x4C49, // 3D RAGE LT PRO BGA-312 PCI 33 MHz
	0x4C50, // 3D RAGE LT PRO BGA-256 PCI 33 MHz
	0x4C51, // 3D RAGE LT PRO BGA-256 PCI 33 MHz limited 3D
};

static DWORD devATIRageIIplus[] = {
	0x4755, // 3D RAGE II+
	0x4756, // 3D RAGE IIC PQFP PCI
	0x4757, // 3D RAGE IIC BGA AGP
	0x475A, // 3D RAGE IIC PQFP AGP
	0x4C47, // 3D RAGE LT-G
};

// ***********************************************************************

#ifndef _USE_GLD3_WGL
extern DGL_mesaFuncs mesaFuncs;
#endif

extern DWORD dwLogging;

#ifdef GLD_THREADS
#pragma message("compiling DGLCONTEXT.C vars for multi-threaded support")
CRITICAL_SECTION CriticalSection;		// for serialized access
DWORD		dwTLSCurrentContext = 0xFFFFFFFF;	// TLS index for current context
DWORD		dwTLSPixelFormat = 0xFFFFFFFF;		// TLS index for current pixel format
#endif
HGLRC		iCurrentContext = 0;		// Index of current context (static)
BOOL		bContextReady = FALSE;		// Context state ready ?

DGL_ctx		ctxlist[DGL_MAX_CONTEXTS];	// Context list

// ***********************************************************************

static BOOL bHaveWin95 = FALSE;
static BOOL bHaveWinNT = FALSE;
static BOOL bHaveWin2K = FALSE;

/****************************************************************************
REMARKS:
Detect the installed OS type.
****************************************************************************/
static void DetectOS(void)
{
    OSVERSIONINFO VersionInformation;
    LPOSVERSIONINFO lpVersionInformation = &VersionInformation;

    VersionInformation.dwOSVersionInfoSize = sizeof(VersionInformation);

	GetVersionEx(lpVersionInformation);

    switch (VersionInformation.dwPlatformId) {
    	case VER_PLATFORM_WIN32_WINDOWS:
			bHaveWin95 = TRUE;
			bHaveWinNT = FALSE;
			bHaveWin2K = FALSE;
            break;
    	case VER_PLATFORM_WIN32_NT:
			bHaveWin95 = FALSE;
			if (VersionInformation.dwMajorVersion <= 4) {
				bHaveWinNT = TRUE;
				bHaveWin2K = FALSE;
                }
            else {
				bHaveWinNT = FALSE;
				bHaveWin2K = TRUE;
                }
			break;
		case VER_PLATFORM_WIN32s:
			bHaveWin95 = FALSE;
			bHaveWinNT = FALSE;
			bHaveWin2K = FALSE;
			break;
        }
}

// ***********************************************************************

HWND hWndEvent = NULL;					// event monitor window
HWND hWndLastActive = NULL;				// last active client window
LONG __stdcall GLD_EventWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

// ***********************************************************************

// Checks if the HGLRC is valid in range of context list.
BOOL dglIsValidContext(
	HGLRC a)
{
	return ((int)a > 0 && (int)a <= DGL_MAX_CONTEXTS);
}

// ***********************************************************************

// Convert a HGLRC to a pointer into the context list.
DGL_ctx* dglGetContextAddress(
	const HGLRC a)
{
	if (dglIsValidContext(a))
		return &ctxlist[(int)a-1];
	return NULL;
}

// ***********************************************************************

// Return the current HGLRC (however it may be stored for multi-threading).
HGLRC dglGetCurrentContext(void)
{
#ifdef GLD_THREADS
	HGLRC hGLRC;
	// load from thread-specific instance
	if (glb.bMultiThreaded) {
		// protect against calls from arbitrary threads
		__try {
			hGLRC = (HGLRC)TlsGetValue(dwTLSCurrentContext);
		}
		__except(EXCEPTION_EXECUTE_HANDLER) {
			hGLRC = iCurrentContext;
		}
	}
	// load from global static var
	else {
		hGLRC = iCurrentContext;
	}
	return hGLRC;
#else
	return iCurrentContext;
#endif
}

// ***********************************************************************

// Set the current HGLRC (however it may be stored for multi-threading).
void dglSetCurrentContext(HGLRC hGLRC)
{
#ifdef GLD_THREADS
	// store in thread-specific instance
	if (glb.bMultiThreaded) {
		// protect against calls from arbitrary threads
		__try {
			TlsSetValue(dwTLSCurrentContext, (LPVOID)hGLRC);
		}
		__except(EXCEPTION_EXECUTE_HANDLER) {
			iCurrentContext = hGLRC;
		}
	}
	// store in global static var
	else {
		iCurrentContext = hGLRC;
	}
#else
	iCurrentContext = hGLRC;
#endif
}

// ***********************************************************************

// Return the current HDC only for a currently active HGLRC.
HDC dglGetCurrentDC(void)
{
	HGLRC hGLRC;
	DGL_ctx* lpCtx;

	hGLRC = dglGetCurrentContext();
	if (hGLRC) {
		lpCtx = dglGetContextAddress(hGLRC);
		return lpCtx->hDC;
	}
	return 0;
}

// ***********************************************************************

void dglInitContextState()
{
	int i;
	WNDCLASS wc;

#ifdef GLD_THREADS
	// Allocate thread local storage indexes for current context and pixel format
	dwTLSCurrentContext = TlsAlloc();
	dwTLSPixelFormat = TlsAlloc();
#endif

	dglSetCurrentContext(NULL); // No current rendering context

	 // Clear all context data
	ZeroMemory(ctxlist, sizeof(ctxlist[0]) * DGL_MAX_CONTEXTS);

	for (i=0; i<DGL_MAX_CONTEXTS; i++)
		ctxlist[i].bAllocated = FALSE; // Flag context as unused

	// This section of code crashes the dll in circumstances where the app
	// creates and destroys contexts.
/*
	// Register the class for our event monitor window
	wc.style = 0;
	wc.lpfnWndProc = GLD_EventWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hIcon = LoadIcon(GetModuleHandle(NULL), IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "GLDIRECT";
	RegisterClass(&wc);

	// Create the non-visible window to monitor all broadcast messages
	hWndEvent = CreateWindowEx(
		WS_EX_TOOLWINDOW,"GLDIRECT","GLDIRECT",WS_POPUP,
		0,0,0,0,
		NULL,NULL,GetModuleHandle(NULL),NULL);
*/

#ifdef GLD_THREADS
	// Create a critical section object for serializing access to
	// DirectDraw and DDStereo create/destroy functions in multiple threads
	if (glb.bMultiThreaded)
		InitializeCriticalSection(&CriticalSection);
#endif

	// Context state is now initialized and ready
	bContextReady = TRUE;
}

// ***********************************************************************

void dglDeleteContextState()
{
	int i;
	static BOOL bOnceIsEnough = FALSE;

	// Only call once, from either DGL_exitDriver(), or DLL_PROCESS_DETACH
	if (bOnceIsEnough)
		return;
	bOnceIsEnough = TRUE;

	for (i=0; i<DGL_MAX_CONTEXTS; i++) {
		if (ctxlist[i].bAllocated == TRUE) {
			ddlogPrintf(DDLOG_WARN, "** Context %i not deleted - cleaning up.", (i+1));
			dglDeleteContext((HGLRC)(i+1));
		}
	}

	// Context state is no longer ready
	bContextReady = FALSE;

    // If executed when DLL unloads, DDraw objects may be invalid.
    // So catch any page faults with this exception handler.
__try {

	// Release final DirectDraw interfaces
	if (glb.bDirectDrawPersistant) {
//		RELEASE(glb.lpGlobalPalette);
//		RELEASE(glb.lpDepth4);
//		RELEASE(glb.lpBack4);
//		RELEASE(glb.lpPrimary4);
//	    RELEASE(glb.lpDD4);
    }
}
__except(EXCEPTION_EXECUTE_HANDLER) {
    ddlogPrintf(DDLOG_WARN, "Exception raised in dglDeleteContextState.");
}

	// Destroy our event monitor window
	if (hWndEvent) {
		DestroyWindow(hWndEvent);
		hWndEvent = hWndLastActive = NULL;
	}

#ifdef GLD_THREADS
	// Destroy the critical section object
	if (glb.bMultiThreaded)
		DeleteCriticalSection(&CriticalSection);

	// Release thread local storage indexes for current HGLRC and pixel format
	TlsFree(dwTLSPixelFormat);
	TlsFree(dwTLSCurrentContext);
#endif
}

// ***********************************************************************

// Application Window message handler interception
static LONG __stdcall dglWndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	DGL_ctx* 	lpCtx = NULL;
	LONG 		lpfnWndProc = 0L;
	int  		i;
	HGLRC 		hGLRC;
	RECT 		rect;
	PAINTSTRUCT	ps;
    BOOL        bQuit = FALSE;
    BOOL        bMain = FALSE;
    LONG        rc;

    // Get the window's message handler *before* it is unhooked in WM_DESTROY

    // Is this the main window?
    if (hwnd == glb.hWndActive) {
        bMain = TRUE;
        lpfnWndProc = glb.lpfnWndProc;
    }
    // Search for DGL context matching window handle
    for (i=0; i<DGL_MAX_CONTEXTS; i++) {
	    if (ctxlist[i].hWnd == hwnd) {
	        lpCtx = &ctxlist[i];
	        lpfnWndProc = lpCtx->lpfnWndProc;
		    break;
        }
    }
	// Not one of ours...
	if (!lpfnWndProc)
	    return DefWindowProc(hwnd, msg, wParam, lParam);

    // Intercept messages amd process *before* passing on to window
	switch (msg) {
#ifdef _USE_GLD3_WGL
	case WM_DISPLAYCHANGE:
		glb.bPixelformatsDirty = TRUE;
		break;
#endif
	case WM_ACTIVATEAPP:
		glb.bAppActive = (BOOL)wParam;
		ddlogPrintf(DDLOG_INFO, "Calling app has been %s", glb.bAppActive ? "activated" : "de-activated");
		break;
	case WM_ERASEBKGND:
		// Eat the GDI erase event for the GL window
        if (!lpCtx || !lpCtx->bHasBeenCurrent)
            break;
		lpCtx->bGDIEraseBkgnd = TRUE;
		return TRUE;
	case WM_PAINT:
		// Eat the invalidated update region if render scene is in progress
        if (!lpCtx || !lpCtx->bHasBeenCurrent)
            break;
		if (lpCtx->bFrameStarted) {
			if (GetUpdateRect(hwnd, &rect, FALSE)) {
				BeginPaint(hwnd, &ps);
				EndPaint(hwnd, &ps);
				ValidateRect(hwnd, &rect);
				return TRUE;
				}
			}
		break;
	}
	// Call the appropriate window message handler
	rc = CallWindowProc((WNDPROC)lpfnWndProc, hwnd, msg, wParam, lParam);

    // Intercept messages and process *after* passing on to window
	switch (msg) {
    case WM_QUIT:
	case WM_DESTROY:
        bQuit = TRUE;
		if (lpCtx && lpCtx->bAllocated) {
			ddlogPrintf(DDLOG_WARN, "WM_DESTROY detected for HWND=%X, HDC=%X, HGLRC=%d", hwnd, lpCtx->hDC, i+1);
			dglDeleteContext((HGLRC)(i+1));
		}
		break;
#if 0
	case WM_SIZE:
		// Resize surfaces to fit window but not viewport (in case app did not bother)
        if (!lpCtx || !lpCtx->bHasBeenCurrent)
            break;
		w = LOWORD(lParam);
		h = HIWORD(lParam);
		if (lpCtx->dwWidth < w || lpCtx->dwHeight < h) {
			if (!dglWglResizeBuffers(lpCtx->glCtx, TRUE))
                 dglWglResizeBuffers(lpCtx->glCtx, FALSE);
        }
		break;
#endif
    }

    // If the main window is quitting, then so should we...
    if (bMain && bQuit) {
		ddlogPrintf(DDLOG_SYSTEM, "shutting down after WM_DESTROY detected for main HWND=%X", hwnd);
        dglDeleteContextState();
        dglExitDriver();
    }

    return rc;
}

// ***********************************************************************

// Driver Window message handler
static LONG __stdcall GLD_EventWndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (msg) {
        // May be sent by splash screen dialog on exit
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_ACTIVE && glb.hWndActive) {
                SetForegroundWindow(glb.hWndActive);
                return 0;
                }
            break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ***********************************************************************

// Intercepted Keyboard handler for detecting hot keys.
LRESULT CALLBACK dglKeyProc(
	int code,
	WPARAM wParam,
	LPARAM lParam)
{
	HWND hWnd, hWndFrame;
	HGLRC hGLRC = NULL;
	DGL_ctx* lpCtx = NULL;
	int cmd = 0, dx1 = 0, dx2 = 0, i;
	static BOOL bAltPressed = FALSE;
	static BOOL bCtrlPressed = FALSE;
	static BOOL bShiftPressed = FALSE;
    RECT r, rf, rc;
    POINT pt;
    BOOL bForceReshape = FALSE;

	return CallNextHookEx(hKeyHook, code, wParam, lParam);
}

// ***********************************************************************

HWND hWndMatch;

// Window handle enumeration procedure.
BOOL CALLBACK dglEnumChildProc(
    HWND hWnd,
    LPARAM lParam)
{
    RECT rect;

    // Find window handle with matching client rect.
    GetClientRect(hWnd, &rect);
    if (EqualRect(&rect, (RECT*)lParam)) {
        hWndMatch = hWnd;
        return FALSE;
        }
    // Continue with next child window.
    return TRUE;
}

// ***********************************************************************

// Find window handle with matching client rect.
HWND dglFindWindowRect(
    RECT* pRect)
{
    hWndMatch = NULL;
    EnumChildWindows(GetForegroundWindow(), dglEnumChildProc, (LPARAM)pRect);
    return hWndMatch;
}

// ***********************************************************************
#ifndef _USE_GLD3_WGL
void dglChooseDisplayMode(
	DGL_ctx *lpCtx)
{
	// Note: Choose an exact match if possible.

	int				i;
	DWORD			area;
	DWORD			bestarea;
	DDSURFACEDESC2	*lpDDSD		= NULL;	// Mode list pointer
	DDSURFACEDESC2	*lpBestDDSD = NULL;	// Pointer to best

	lpDDSD = glb.lpDisplayModes;
	for (i=0; i<glb.nDisplayModeCount; i++, lpDDSD++) {
		if ((lpDDSD->dwWidth == lpCtx->dwWidth) &&
			(lpDDSD->dwHeight == lpCtx->dwHeight))
			goto matched; // Mode has been exactly matched
		// Choose modes that are larger in both dimensions than
		// the window, but smaller in area than the current best.
		if ( (lpDDSD->dwWidth >= lpCtx->dwWidth) &&
			 (lpDDSD->dwHeight >= lpCtx->dwHeight))
		{
			if (lpBestDDSD == NULL) {
				lpBestDDSD = lpDDSD;
				bestarea = lpDDSD->dwWidth * lpDDSD->dwHeight;
				continue;
			}
			area = lpDDSD->dwWidth * lpDDSD->dwHeight;
			if (area < bestarea) {
				lpBestDDSD = lpDDSD;
				bestarea = area;
			}
		}
	}

	// Safety check
	if (lpBestDDSD == NULL) {
		ddlogMessage(DDLOG_CRITICAL, "dglChooseDisplayMode");
		return;
	}

	lpCtx->dwModeWidth = lpBestDDSD->dwWidth;
	lpCtx->dwModeHeight = lpBestDDSD->dwHeight;
matched:
	ddlogPrintf(DDLOG_INFO, "Matched (%ldx%ld) to (%ldx%ld)",
		lpCtx->dwWidth, lpCtx->dwHeight, lpCtx->dwModeWidth, lpCtx->dwModeHeight);
}
#endif // _USE_GLD3_WGL
// ***********************************************************************

static BOOL IsDevice(
	DWORD *lpDeviceIdList,
	DWORD dwDeviceId,
	int count)
{
	int i;

	for (i=0; i<count; i++)
		if (dwDeviceId == lpDeviceIdList[i])
			return TRUE;

	return FALSE;
}

// ***********************************************************************

void dglTestForBrokenCards(
	DGL_ctx *lpCtx)
{
#ifndef _GLD3
	DDDEVICEIDENTIFIER	dddi; // DX6 device identifier

	// Sanity check.
	if (lpCtx == NULL) {
		// Testing for broken cards is sensitive area, so we don't want
		// anything saying "broken cards" in the error message. ;)
		ddlogMessage(DDLOG_ERROR, "Null context passed to TFBC\n");
		return;
	}

	if (lpCtx->lpDD4 == NULL) {
		// Testing for broken cards is sensitive area, so we don't want
		// anything saying "broken cards" in the error message. ;)
		ddlogMessage(DDLOG_ERROR, "Null DD4 passed to TFBC\n");
		return;
	}

	// Microsoft really fucked up with the GetDeviceIdentifier function
	// on Windows 2000, since it locks up on stock driers on the CD. Updated
	// drivers from vendors appear to work, but we can't identify the drivers
	// without this function!!! For now we skip these tests on Windows 2000.
	if ((GetVersion() & 0x80000000UL) == 0)
		return;

	// Obtain device info
	if (FAILED(IDirectDraw4_GetDeviceIdentifier(lpCtx->lpDD4, &dddi, 0)))
		return;

	// Useful info. Log it.
	ddlogPrintf(DDLOG_INFO, "DirectDraw: VendorId=0x%x, DeviceId=0x%x", dddi.dwVendorId, dddi.dwDeviceId);

	// Vendor 1: ATI
	if (dddi.dwVendorId == VENDORID_ATI) {
		// Test A: ATI Rage PRO
		if (IsDevice(devATIRagePro, dddi.dwDeviceId, sizeof(devATIRagePro)))
			glb.bUseMipmaps = FALSE;
		// Test B: ATI Rage II+
		if (IsDevice(devATIRageIIplus, dddi.dwDeviceId, sizeof(devATIRageIIplus)))
			glb.bEmulateAlphaTest = TRUE;
	}

	// Vendor 2: Matrox
	if (dddi.dwVendorId == 0x102B) {
		// Test: Matrox G400 stencil buffer support does not work for AutoCAD
		if (dddi.dwDeviceId == 0x0525) {
			lpCtx->lpPF->pfd.cStencilBits = 0;
			if (lpCtx->lpPF->iZBufferPF != -1) {
				glb.lpZBufferPF[lpCtx->lpPF->iZBufferPF].dwStencilBitDepth = 0;
				glb.lpZBufferPF[lpCtx->lpPF->iZBufferPF].dwStencilBitMask = 0;
				glb.lpZBufferPF[lpCtx->lpPF->iZBufferPF].dwFlags &= ~DDPF_STENCILBUFFER;
			}
		}
	}
#endif // _GLD3
}

// ***********************************************************************

BOOL dglCreateContextBuffers(
	HDC a,
	DGL_ctx *lpCtx,
	BOOL bFallback)
{
	HRESULT				hResult;

	int					i;
//	HGLRC				hGLRC;
//	DGL_ctx*			lpCtx;

#ifndef _USE_GLD3_WGL
	DWORD				dwFlags;
	DDSURFACEDESC2		ddsd2;
	DDSCAPS2			ddscaps2;
	LPDIRECTDRAWCLIPPER	lpddClipper;
	D3DDEVICEDESC		D3DHWDevDesc;	// Direct3D Hardware description
	D3DDEVICEDESC		D3DHELDevDesc;	// Direct3D Hardware Emulation Layer
#endif // _USE_GLD3_WGL

	float				inv_aspect;

	GLenum				bDoubleBuffer;	// TRUE if double buffer required
	GLenum				bDepthBuffer;	// TRUE if depth buffer required

	const PIXELFORMATDESCRIPTOR	*lpPFD = &lpCtx->lpPF->pfd;

	// Vars for Mesa visual
	DWORD				dwDepthBits		= 0;
	DWORD				dwStencilBits	= 0;
	DWORD				dwAlphaBits		= 0;
	DWORD				bAlphaSW		= GL_FALSE;
	DWORD				bDouble			= GL_FALSE;

	DDSURFACEDESC2 		ddsd2DisplayMode;
	BOOL				bFullScrnWin	= FALSE;	// fullscreen-size window ?
	DDBLTFX 			ddbltfx;
	DWORD				dwMemoryType 	= (bFallback) ? DDSCAPS_SYSTEMMEMORY : glb.dwMemoryType;
	BOOL				bBogusWindow	= FALSE;	// non-drawable window ?
	DWORD               dwColorRef      = 0;        // GDI background color
	RECT				rcDst;						// GDI window rect
	POINT				pt;							// GDI window point

	// Palette used for creating default global palette
	PALETTEENTRY	ppe[256];

#ifndef _USE_GLD3_WGL
	// Vertex buffer description. Used for creation of vertex buffers
	D3DVERTEXBUFFERDESC vbufdesc;
#endif // _USE_GLD3_WGL

#define DDLOG_CRITICAL_OR_WARN	(bFallback ? DDLOG_CRITICAL : DDLOG_WARN)

	ddlogPrintf(DDLOG_SYSTEM, "dglCreateContextBuffers for HDC=%X", a);
    nContextError = GLDERR_NONE;

#ifdef GLD_THREADS
	// Serialize access to DirectDraw object creation or DDS start
	if (glb.bMultiThreaded)
		EnterCriticalSection(&CriticalSection);
#endif

	// Check for back buffer
	bDoubleBuffer = GL_TRUE; //(lpPFD->dwFlags & PFD_DOUBLEBUFFER) ? GL_TRUE : GL_FALSE;
	// Since we always do back buffering, check if we emulate front buffering
	lpCtx->EmulateSingle =
		(lpPFD->dwFlags & PFD_DOUBLEBUFFER) ? FALSE : TRUE;
#if 0	// Don't have to mimic MS OpenGL behavior for front-buffering (DaveM)
	lpCtx->EmulateSingle |=
		(lpPFD->dwFlags & PFD_SUPPORT_GDI) ? TRUE : FALSE;
#endif

	// Check for depth buffer
	bDepthBuffer = (lpPFD->cDepthBits) ? GL_TRUE : GL_FALSE;

	lpCtx->bDoubleBuffer = bDoubleBuffer;
	lpCtx->bDepthBuffer = bDepthBuffer;

	// Set the Fullscreen flag for the context.
//	lpCtx->bFullscreen = glb.bFullscreen;

	// Obtain the dimensions of the rendering window
	lpCtx->hDC = a; // Cache DC
	lpCtx->hWnd = WindowFromDC(lpCtx->hDC);
	// Check for non-window DC = memory DC ?
	if (lpCtx->hWnd == NULL) {
        // bitmap memory contexts are always single-buffered
        lpCtx->EmulateSingle = TRUE;
		bBogusWindow = TRUE;
		ddlogPrintf(DDLOG_INFO, "Non-Window Memory Device Context");
		if (GetClipBox(lpCtx->hDC, &lpCtx->rcScreenRect) == ERROR) {
			ddlogMessage(DDLOG_WARN, "GetClipBox failed in dglCreateContext\n");
			SetRect(&lpCtx->rcScreenRect, 0, 0, 0, 0);
		}
	}
	else if (!GetClientRect(lpCtx->hWnd, &lpCtx->rcScreenRect)) {
		bBogusWindow = TRUE;
		ddlogMessage(DDLOG_WARN, "GetClientRect failed in dglCreateContext\n");
		SetRect(&lpCtx->rcScreenRect, 0, 0, 0, 0);
	}
	lpCtx->dwWidth = lpCtx->rcScreenRect.right - lpCtx->rcScreenRect.left;
	lpCtx->dwHeight = lpCtx->rcScreenRect.bottom - lpCtx->rcScreenRect.top;

	ddlogPrintf(DDLOG_INFO, "Input window %X: w=%i, h=%i",
							lpCtx->hWnd, lpCtx->dwWidth, lpCtx->dwHeight);

	// What if app only zeroes one dimension instead of both? (DaveM)
	if ( (lpCtx->dwWidth == 0) || (lpCtx->dwHeight == 0) ) {
		// Make the buffer size something sensible
		lpCtx->dwWidth = 8;
		lpCtx->dwHeight = 8;
	}

	// Set defaults
	lpCtx->dwModeWidth = lpCtx->dwWidth;
	lpCtx->dwModeHeight = lpCtx->dwHeight;
/*
	// Find best display mode for fullscreen
	if (glb.bFullscreen || !glb.bPrimary) {
		dglChooseDisplayMode(lpCtx);
	}
*/
	// Misc initialisation
	lpCtx->bCanRender = FALSE; // No rendering allowed yet
	lpCtx->bSceneStarted = FALSE;
	lpCtx->bFrameStarted = FALSE;

	// Detect OS (specifically 'Windows 2000' or 'Windows XP')
	DetectOS();

	// NOTE: WinNT not supported
	ddlogPrintf(DDLOG_INFO, "OS: %s", bHaveWin95 ? "Win9x" : (bHaveWin2K ? "Win2000/XP" : "Unsupported") );

	// Test for Fullscreen
	if (bHaveWin95) { // Problems with fullscreen on Win2K/XP
		if ((GetSystemMetrics(SM_CXSCREEN) == lpCtx->dwWidth) && 
			(GetSystemMetrics(SM_CYSCREEN) == lpCtx->dwHeight))
		{
			// Workaround for some apps that crash when going fullscreen.
			//lpCtx->bFullscreen = TRUE;
		}
		
	}

#ifdef _USE_GLD3_WGL
	_gldDriver.CreateDrawable(lpCtx, glb.bDirectDrawPersistant, glb.bPersistantBuffers);
#else
	// Check if DirectDraw has already been created by original GLRC (DaveM)
	if (glb.bDirectDrawPersistant && glb.bDirectDraw) {
		lpCtx->lpDD4 = glb.lpDD4;
		IDirectDraw4_AddRef(lpCtx->lpDD4);
		goto SkipDirectDrawCreate;
	}

	// Create DirectDraw object
	if (glb.bPrimary)
		hResult = DirectDrawCreate(NULL, &lpCtx->lpDD1, NULL);
	else {
		// A non-primary device is to be used.
		// Force context to be Fullscreen, secondary adaptors can not
		// be used in a window.
		hResult = DirectDrawCreate(&glb.ddGuid, &lpCtx->lpDD1, NULL);
		lpCtx->bFullscreen = TRUE;
	}
	if (FAILED(hResult)) {
		MessageBox(NULL, "Unable to initialize DirectDraw", "GLDirect", MB_OK);
		ddlogError(DDLOG_CRITICAL_OR_WARN, "Unable to create DirectDraw interface", hResult);
        nContextError = GLDERR_DDRAW;
		goto return_with_error;
	}

	// Query for DX6 IDirectDraw4.
	hResult = IDirectDraw_QueryInterface(lpCtx->lpDD1,
										 &IID_IDirectDraw4,
										 (void**)&lpCtx->lpDD4);
	if (FAILED(hResult)) {
		MessageBox(NULL, "GLDirect requires DirectX 6.0 or above", "GLDirect", MB_OK);
		ddlogError(DDLOG_CRITICAL_OR_WARN, "Unable to create DirectDraw4 interface", hResult);
        nContextError = GLDERR_DDRAW;
		goto return_with_error;
	}

	// Cache DirectDraw interface for subsequent GLRCs
	if (glb.bDirectDrawPersistant && !glb.bDirectDraw) {
		glb.lpDD4 = lpCtx->lpDD4;
		IDirectDraw4_AddRef(glb.lpDD4);
		glb.bDirectDraw = TRUE;
	}
SkipDirectDrawCreate:

	// Now we have a DD4 interface we can check for broken cards
	dglTestForBrokenCards(lpCtx);

	// Test if primary device can use flipping instead of blitting
	ZeroMemory(&ddsd2DisplayMode, sizeof(ddsd2DisplayMode));
	ddsd2DisplayMode.dwSize = sizeof(ddsd2DisplayMode);
	hResult = IDirectDraw4_GetDisplayMode(
					lpCtx->lpDD4,
					&ddsd2DisplayMode);
	if (SUCCEEDED(hResult)) {
		if ( (lpCtx->dwWidth == ddsd2DisplayMode.dwWidth) &&
				 (lpCtx->dwHeight == ddsd2DisplayMode.dwHeight) ) {
			// We have a fullscreen-size window
			bFullScrnWin = TRUE;
			// OK to use DirectDraw fullscreen mode ?
			if (glb.bPrimary && !glb.bFullscreenBlit && !lpCtx->EmulateSingle && !glb.bDirectDrawPersistant) {
				lpCtx->bFullscreen = TRUE;
				ddlogMessage(DDLOG_INFO, "Primary upgraded to page flipping.\n");
			}
		}
		// Cache the display mode dimensions
		lpCtx->dwModeWidth = ddsd2DisplayMode.dwWidth;
		lpCtx->dwModeHeight = ddsd2DisplayMode.dwHeight;
	}

	// Clamp the effective window dimensions to primary surface.
	// We need to do this for D3D viewport dimensions even if wide
	// surfaces are supported. This also is a good idea for handling
	// whacked-out window dimensions passed for non-drawable windows
	// like Solid Edge. (DaveM)
	if (lpCtx->dwWidth > ddsd2DisplayMode.dwWidth)
		lpCtx->dwWidth = ddsd2DisplayMode.dwWidth;
	if (lpCtx->dwHeight > ddsd2DisplayMode.dwHeight)
		lpCtx->dwHeight = ddsd2DisplayMode.dwHeight;

	// Check for non-RGB desktop resolution
	if (!lpCtx->bFullscreen && ddsd2DisplayMode.ddpfPixelFormat.dwRGBBitCount <= 8) {
		ddlogPrintf(DDLOG_CRITICAL_OR_WARN, "Desktop color depth %d bpp not supported",
			ddsd2DisplayMode.ddpfPixelFormat.dwRGBBitCount);
        nContextError = GLDERR_BPP;
		goto return_with_error;
	}
#endif // _USE_GLD3_WGL

	ddlogPrintf(DDLOG_INFO, "Window: w=%i, h=%i (%s)",
							lpCtx->dwWidth,
							lpCtx->dwHeight,
							lpCtx->bFullscreen ? "fullscreen" : "windowed");

#ifndef _USE_GLD3_WGL
	// Obtain ddraw caps
    ZeroMemory(&lpCtx->ddCaps, sizeof(DDCAPS));
	lpCtx->ddCaps.dwSize = sizeof(DDCAPS);
	if (glb.bHardware) {
		// Get HAL caps
		IDirectDraw4_GetCaps(lpCtx->lpDD4, &lpCtx->ddCaps, NULL);
	} else {
		// Get HEL caps
		IDirectDraw4_GetCaps(lpCtx->lpDD4, NULL, &lpCtx->ddCaps);
	}

	// If this flag is present then we can't default to Mesa
	// SW rendering between BeginScene() and EndScene().
	if (lpCtx->ddCaps.dwCaps2 & DDCAPS2_NO2DDURING3DSCENE) {
		ddlogMessage(DDLOG_INFO,
			"Warning          : No 2D allowed during 3D scene.\n");
	}

	// Query for DX6 Direct3D3 interface
	hResult = IDirectDraw4_QueryInterface(lpCtx->lpDD4,
										  &IID_IDirect3D3,
										  (void**)&lpCtx->lpD3D3);
	if (FAILED(hResult)) {
		MessageBox(NULL, "Unable to initialize Direct3D", "GLDirect", MB_OK);
		ddlogError(DDLOG_CRITICAL_OR_WARN, "Unable to create Direct3D interface", hResult);
        nContextError = GLDERR_D3D;
		goto return_with_error;
	}

	// Context creation
	if (lpCtx->bFullscreen) {
		// FULLSCREEN

        // Disable warning popups when in fullscreen mode
        ddlogWarnOption(FALSE);

		// Have to release persistant primary surface if fullscreen mode
		if (glb.bDirectDrawPersistant && glb.bDirectDrawPrimary) {
			RELEASE(glb.lpPrimary4);
			glb.bDirectDrawPrimary = FALSE;
		}

		dwFlags = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT;
		if (glb.bFastFPU)
			dwFlags |= DDSCL_FPUSETUP;	// fast FPU setup optional (DaveM)
		hResult = IDirectDraw4_SetCooperativeLevel(lpCtx->lpDD4, lpCtx->hWnd, dwFlags);
		if (FAILED(hResult)) {
			ddlogError(DDLOG_CRITICAL_OR_WARN, "Unable to set Exclusive Fullscreen mode", hResult);
			goto return_with_error;
		}

		hResult = IDirectDraw4_SetDisplayMode(lpCtx->lpDD4,
											  lpCtx->dwModeWidth,
											  lpCtx->dwModeHeight,
											  lpPFD->cColorBits,
											  0,
											  0);
		if (FAILED(hResult)) {
			ddlogError(DDLOG_CRITICAL_OR_WARN, "SetDisplayMode failed", hResult);
			goto return_with_error;
		}

		// ** The display mode has changed, so dont use MessageBox! **

		ZeroMemory(&ddsd2, sizeof(ddsd2));
		ddsd2.dwSize = sizeof(ddsd2);

		if (bDoubleBuffer) {
			// Double buffered
			// Primary surface
			ddsd2.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
			ddsd2.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
								   DDSCAPS_FLIP |
								   DDSCAPS_COMPLEX |
								   DDSCAPS_3DDEVICE |
								   dwMemoryType;
			ddsd2.dwBackBufferCount = 1;

			hResult = IDirectDraw4_CreateSurface(lpCtx->lpDD4, &ddsd2, &lpCtx->lpFront4, NULL);
			if (FAILED(hResult)) {
				ddlogError(DDLOG_CRITICAL_OR_WARN, "CreateSurface (primary) failed", hResult);
                nContextError = GLDERR_MEM;
				goto return_with_error;
			}

			// Render target surface
			ZeroMemory(&ddscaps2, sizeof(ddscaps2)); // Clear the entire struct.
			ddscaps2.dwCaps = DDSCAPS_BACKBUFFER;
			hResult = IDirectDrawSurface4_GetAttachedSurface(lpCtx->lpFront4, &ddscaps2, &lpCtx->lpBack4);
			if (FAILED(hResult)) {
				ddlogError(DDLOG_CRITICAL_OR_WARN, "GetAttachedSurface failed", hResult);
                nContextError = GLDERR_MEM;
				goto return_with_error;
			}
		} else {
			// Single buffered
			// Primary surface
			ddsd2.dwFlags = DDSD_CAPS;
			ddsd2.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
								   //DDSCAPS_3DDEVICE |
								   dwMemoryType;

			hResult = IDirectDraw4_CreateSurface(lpCtx->lpDD4, &ddsd2, &lpCtx->lpFront4, NULL);
			if (FAILED(hResult)) {
				ddlogError(DDLOG_CRITICAL_OR_WARN, "CreateSurface (primary) failed", hResult);
                nContextError = GLDERR_MEM;
				goto return_with_error;
			}

			lpCtx->lpBack4 = NULL;
		}
	} else {
		// WINDOWED

        // OK to enable warning popups in windowed mode
        ddlogWarnOption(glb.bMessageBoxWarnings);

		dwFlags = DDSCL_NORMAL;
		if (glb.bMultiThreaded)
			dwFlags |= DDSCL_MULTITHREADED;
		if (glb.bFastFPU)
			dwFlags |= DDSCL_FPUSETUP;	// fast FPU setup optional (DaveM)
		hResult = IDirectDraw4_SetCooperativeLevel(lpCtx->lpDD4,
												  lpCtx->hWnd,
												  dwFlags);
		if (FAILED(hResult)) {
			ddlogError(DDLOG_CRITICAL_OR_WARN, "Unable to set Normal coop level", hResult);
			goto return_with_error;
		}
		// Has Primary surface already been created for original GLRC ?
		// Note this can only be applicable for windowed modes
		if (glb.bDirectDrawPersistant && glb.bDirectDrawPrimary) {
			lpCtx->lpFront4 = glb.lpPrimary4;
			IDirectDrawSurface4_AddRef(lpCtx->lpFront4);
			// Update the window on the default clipper
			IDirectDrawSurface4_GetClipper(lpCtx->lpFront4, &lpddClipper);
			IDirectDrawClipper_SetHWnd(lpddClipper, 0, lpCtx->hWnd);
			IDirectDrawClipper_Release(lpddClipper);
			goto SkipPrimaryCreate;
		}

		// Primary surface
		ZeroMemory(&ddsd2, sizeof(ddsd2));
		ddsd2.dwSize = sizeof(ddsd2);
		ddsd2.dwFlags = DDSD_CAPS;
		ddsd2.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		hResult = IDirectDraw4_CreateSurface(lpCtx->lpDD4, &ddsd2, &lpCtx->lpFront4, NULL);
		if (FAILED(hResult)) {
			ddlogError(DDLOG_CRITICAL_OR_WARN, "CreateSurface (primary) failed", hResult);
            nContextError = GLDERR_MEM;
			goto return_with_error;
		}

		// Cache Primary surface for subsequent GLRCs
		// Note this can only be applicable to subsequent windowed modes
		if (glb.bDirectDrawPersistant && !glb.bDirectDrawPrimary) {
			glb.lpPrimary4 = lpCtx->lpFront4;
			IDirectDrawSurface4_AddRef(glb.lpPrimary4);
			glb.bDirectDrawPrimary = TRUE;
		}

		// Clipper object
		hResult = DirectDrawCreateClipper(0, &lpddClipper, NULL);
		if (FAILED(hResult)) {
			ddlogError(DDLOG_CRITICAL_OR_WARN, "CreateClipper failed", hResult);
			goto return_with_error;
		}
		hResult = IDirectDrawClipper_SetHWnd(lpddClipper, 0, lpCtx->hWnd);
		if (FAILED(hResult)) {
			RELEASE(lpddClipper);
			ddlogError(DDLOG_CRITICAL_OR_WARN, "SetHWnd failed", hResult);
			goto return_with_error;
		}
		hResult = IDirectDrawSurface4_SetClipper(lpCtx->lpFront4, lpddClipper);
		RELEASE(lpddClipper); // We have finished with it.
		if (FAILED(hResult)) {
			ddlogError(DDLOG_CRITICAL_OR_WARN, "SetClipper failed", hResult);
			goto return_with_error;
		}
SkipPrimaryCreate:

		if (bDoubleBuffer) {
			// Render target surface
			ZeroMemory(&ddsd2, sizeof(ddsd2));
			ddsd2.dwSize = sizeof(ddsd2);
			ddsd2.dwFlags        = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
			ddsd2.dwWidth        = lpCtx->dwWidth;
			ddsd2.dwHeight       = lpCtx->dwHeight;
			ddsd2.ddsCaps.dwCaps = DDSCAPS_3DDEVICE |
								   DDSCAPS_OFFSCREENPLAIN |
								   dwMemoryType;

			// Reserve the entire desktop size for persistant buffers option
			if (glb.bDirectDrawPersistant && glb.bPersistantBuffers) {
				ddsd2.dwWidth = ddsd2DisplayMode.dwWidth;
				ddsd2.dwHeight = ddsd2DisplayMode.dwHeight;
			}
			// Re-use original back buffer if persistant buffers exist
			if (glb.bDirectDrawPersistant && glb.bPersistantBuffers && glb.lpBack4)
				hResult = IDirectDrawSurface4_AddRef(lpCtx->lpBack4 = glb.lpBack4);
			else
				hResult = IDirectDraw4_CreateSurface(lpCtx->lpDD4, &ddsd2, &lpCtx->lpBack4, NULL);
			if (FAILED(hResult)) {
				ddlogError(DDLOG_CRITICAL_OR_WARN, "Create Backbuffer failed", hResult);
                nContextError = GLDERR_MEM;
				goto return_with_error;
			}
			if (glb.bDirectDrawPersistant && glb.bPersistantBuffers && !glb.lpBack4)
				IDirectDrawSurface4_AddRef(glb.lpBack4 = lpCtx->lpBack4);
		} else {
			lpCtx->lpBack4 = NULL;
		}
	}

	//
	// Now create the Z-buffer
	//
	lpCtx->bStencil = FALSE; // Default to no stencil buffer
	if (bDepthBuffer && (lpCtx->lpPF->iZBufferPF != -1)) {
		// Get z-buffer dimensions from the render target
		// Setup the surface desc for the z-buffer.
		ZeroMemory(&ddsd2, sizeof(ddsd2));
		ddsd2.dwSize = sizeof(ddsd2);
		ddsd2.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
		ddsd2.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | dwMemoryType;
		ddsd2.dwWidth = lpCtx->dwWidth;
		ddsd2.dwHeight = lpCtx->dwHeight;
		memcpy(&ddsd2.ddpfPixelFormat,
			&glb.lpZBufferPF[lpCtx->lpPF->iZBufferPF],
			sizeof(DDPIXELFORMAT) );

		// Reserve the entire desktop size for persistant buffers option
		if (glb.bDirectDrawPersistant && glb.bPersistantBuffers) {
			ddsd2.dwWidth = ddsd2DisplayMode.dwWidth;
			ddsd2.dwHeight = ddsd2DisplayMode.dwHeight;
		}

		// Create a z-buffer
		if (glb.bDirectDrawPersistant && glb.bPersistantBuffers && glb.lpDepth4)
			hResult = IDirectDrawSurface4_AddRef(lpCtx->lpDepth4 = glb.lpDepth4);
		else
			hResult = IDirectDraw4_CreateSurface(lpCtx->lpDD4, &ddsd2, &lpCtx->lpDepth4, NULL);
		if (FAILED(hResult)) {
			ddlogError(DDLOG_CRITICAL_OR_WARN, "CreateSurface (ZBuffer) failed", hResult);
            nContextError = GLDERR_MEM;
			goto return_with_error;
		}
		if (glb.bDirectDrawPersistant && glb.bPersistantBuffers && !glb.lpDepth4)
			IDirectDrawSurface4_AddRef(glb.lpDepth4 = lpCtx->lpDepth4);
		else if (glb.bDirectDrawPersistant && glb.bPersistantBuffers && glb.lpDepth4 && glb.lpBack4)
			IDirectDrawSurface4_DeleteAttachedSurface(glb.lpBack4, 0, glb.lpDepth4);

		// Attach Zbuffer to render target
		TRY(IDirectDrawSurface4_AddAttachedSurface(
			bDoubleBuffer ? lpCtx->lpBack4 : lpCtx->lpFront4,
			lpCtx->lpDepth4),
			"dglCreateContext: Attach Zbuffer");
		if (glb.lpZBufferPF[lpCtx->lpPF->iZBufferPF].dwFlags & DDPF_STENCILBUFFER) {
			lpCtx->bStencil = TRUE;
			ddlogMessage(DDLOG_INFO, "Depth buffer has stencil\n");
		}
	}

	// Clear all back buffers and Z-buffers in case of memory recycling.
	ZeroMemory(&ddbltfx, sizeof(ddbltfx));
	ddbltfx.dwSize = sizeof(ddbltfx);
	IDirectDrawSurface4_Blt(lpCtx->lpBack4, NULL, NULL, NULL,
		DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
	if (lpCtx->lpDepth4)
		IDirectDrawSurface4_Blt(lpCtx->lpDepth4, NULL, NULL, NULL,
			DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);

	// Now that we have a Z-buffer we can create the 3D device
	hResult = IDirect3D3_CreateDevice(lpCtx->lpD3D3,
									  &glb.d3dGuid,
									  bDoubleBuffer ? lpCtx->lpBack4 : lpCtx->lpFront4,
									  &lpCtx->lpDev3,
									  NULL);
	if (FAILED(hResult)) {
		ddlogError(DDLOG_CRITICAL_OR_WARN, "Unable to create Direct3D device", hResult);
        nContextError = GLDERR_D3D;
		goto return_with_error;
	}

	// We must do this as soon as the device is created
	dglInitStateCaches(lpCtx);

	// Obtain the D3D Device Description
	D3DHWDevDesc.dwSize = D3DHELDevDesc.dwSize = sizeof(D3DDEVICEDESC);
	TRY(IDirect3DDevice3_GetCaps(lpCtx->lpDev3,
								 &D3DHWDevDesc,
								 &D3DHELDevDesc),
								 "dglCreateContext: GetCaps failed");

	// Choose the relevant description and cache it in the context.
	// We will use this description later for caps checking
	memcpy(	&lpCtx->D3DDevDesc,
			glb.bHardware ? &D3DHWDevDesc : &D3DHELDevDesc,
			sizeof(D3DDEVICEDESC));

	// Now we can examine the texture formats
	if (!dglBuildTextureFormatList(lpCtx->lpDev3)) {
		ddlogMessage(DDLOG_CRITICAL_OR_WARN, "dglBuildTextureFormatList failed\n");
		goto return_with_error;
	}

	// Get the pixel format of the back buffer
	lpCtx->ddpfRender.dwSize = sizeof(lpCtx->ddpfRender);
	if (bDoubleBuffer)
		hResult = IDirectDrawSurface4_GetPixelFormat(
					lpCtx->lpBack4,
					&lpCtx->ddpfRender);
	else
		hResult = IDirectDrawSurface4_GetPixelFormat(
					lpCtx->lpFront4,
					&lpCtx->ddpfRender);

	if (FAILED(hResult)) {
		ddlogError(DDLOG_CRITICAL_OR_WARN, "GetPixelFormat failed", hResult);
		goto return_with_error;
	}
	// Find a pixel packing function suitable for this surface
	pxClassifyPixelFormat(&lpCtx->ddpfRender,
						  &lpCtx->fnPackFunc,
						  &lpCtx->fnUnpackFunc,
						  &lpCtx->fnPackSpanFunc);

	// Viewport
	hResult = IDirect3D3_CreateViewport(lpCtx->lpD3D3, &lpCtx->lpViewport3, NULL);
	if (FAILED(hResult)) {
		ddlogError(DDLOG_CRITICAL_OR_WARN, "CreateViewport failed", hResult);
		goto return_with_error;
	}

	hResult = IDirect3DDevice3_AddViewport(lpCtx->lpDev3, lpCtx->lpViewport3);
	if (FAILED(hResult)) {
		ddlogError(DDLOG_CRITICAL_OR_WARN, "AddViewport failed", hResult);
		goto return_with_error;
	}

	// Initialise the viewport
	// Note screen coordinates are used for viewport clipping since D3D
	// transform operations are not used in the GLD CAD driver. (DaveM)
	inv_aspect = (float)lpCtx->dwHeight/(float)lpCtx->dwWidth;

	lpCtx->d3dViewport.dwSize = sizeof(lpCtx->d3dViewport);
	lpCtx->d3dViewport.dwX = 0;
	lpCtx->d3dViewport.dwY = 0;
	lpCtx->d3dViewport.dwWidth = lpCtx->dwWidth;
	lpCtx->d3dViewport.dwHeight = lpCtx->dwHeight;
	lpCtx->d3dViewport.dvClipX = 0; // -1.0f;
	lpCtx->d3dViewport.dvClipY = 0; // inv_aspect;
	lpCtx->d3dViewport.dvClipWidth = lpCtx->dwWidth; // 2.0f;
	lpCtx->d3dViewport.dvClipHeight = lpCtx->dwHeight; // 2.0f * inv_aspect;
	lpCtx->d3dViewport.dvMinZ = 0.0f;
	lpCtx->d3dViewport.dvMaxZ = 1.0f;
	TRY(IDirect3DViewport3_SetViewport2(lpCtx->lpViewport3, &lpCtx->d3dViewport), "dglCreateContext: SetViewport2");

	hResult = IDirect3DDevice3_SetCurrentViewport(lpCtx->lpDev3, lpCtx->lpViewport3);
	if (FAILED(hResult)) {
		ddlogError(DDLOG_CRITICAL_OR_WARN, "SetCurrentViewport failed", hResult);
		goto return_with_error;
	}

	lpCtx->dwBPP = lpPFD->cColorBits;
	lpCtx->iZBufferPF = lpCtx->lpPF->iZBufferPF;

	// Set last texture to NULL
	for (i=0; i<MAX_TEXTURE_UNITS; i++) {
		lpCtx->ColorOp[i] = D3DTOP_DISABLE;
		lpCtx->AlphaOp[i] = D3DTOP_DISABLE;
		lpCtx->tObj[i] = NULL;
	}

	// Default to perspective correct texture mapping
	dglSetRenderState(lpCtx, D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE, "TexturePersp");

	// Set the default culling mode
	lpCtx->cullmode = D3DCULL_NONE;
	dglSetRenderState(lpCtx, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE, "CullMode");

	// Disable specular
	dglSetRenderState(lpCtx, D3DRENDERSTATE_SPECULARENABLE, FALSE, "SpecularEnable");
	// Disable subpixel correction
//	dglSetRenderState(lpCtx, D3DRENDERSTATE_SUBPIXEL, FALSE, "SubpixelEnable");
	// Disable dithering
	dglSetRenderState(lpCtx, D3DRENDERSTATE_DITHERENABLE, FALSE, "DitherEnable");

	// Initialise the primitive caches
//	lpCtx->dwNextLineVert	= 0;
//	lpCtx->dwNextTriVert	= 0;

	// Init the global texture palette
	lpCtx->lpGlobalPalette = NULL;

	// Init the HW/SW usage counters
//	lpCtx->dwHWUsageCount = lpCtx->dwSWUsageCount = 0L;

	//
	// Create two D3D vertex buffers.
	// One will hold the pre-transformed data with the other one
	// being used to hold the post-transformed & clipped verts.
	//
#if 0  // never used (DaveM)
	vbufdesc.dwSize = sizeof(D3DVERTEXBUFFERDESC);
	vbufdesc.dwCaps = D3DVBCAPS_WRITEONLY;
	if (glb.bHardware == FALSE)
		vbufdesc.dwCaps = D3DVBCAPS_SYSTEMMEMORY;
	vbufdesc.dwNumVertices = 32768; // For the time being

	// Source vertex buffer
	vbufdesc.dwFVF = DGL_LVERTEX;
	hResult = IDirect3D3_CreateVertexBuffer(lpCtx->lpD3D3, &vbufdesc, &lpCtx->m_vbuf, 0, NULL);
	if (FAILED(hResult)) {
		ddlogError(DDLOG_CRITICAL_OR_WARN, "CreateVertexBuffer(src) failed", hResult);
		goto return_with_error;
	}

	// Destination vertex buffer
	vbufdesc.dwFVF = (glb.bMultitexture == FALSE) ? D3DFVF_TLVERTEX : (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2);
	hResult = IDirect3D3_CreateVertexBuffer(lpCtx->lpD3D3, &vbufdesc, &lpCtx->m_pvbuf, 0, NULL);
	if(FAILED(hResult)) {
		ddlogError(DDLOG_CRITICAL_OR_WARN, "CreateVertexBuffer(dst) failed", hResult);
		goto return_with_error;
	}
#endif

#endif _USE_GLD3_WGL

	//
	//	Now create the Mesa context
	//

	// Create the Mesa visual
	if (lpPFD->cDepthBits)
		dwDepthBits = 16;
	if (lpPFD->cStencilBits)
		dwStencilBits = 8;
	if (lpPFD->cAlphaBits) {
		dwAlphaBits = 8;
		bAlphaSW = GL_TRUE;
	}
	if (lpPFD->dwFlags & PFD_DOUBLEBUFFER)
		bDouble = GL_TRUE;
//	lpCtx->EmulateSingle =
//		(lpPFD->dwFlags & PFD_DOUBLEBUFFER) ? FALSE : TRUE;

#ifdef _USE_GLD3_WGL
	lpCtx->glVis = _mesa_create_visual(
		GL_TRUE,		// RGB mode
		bDouble,    /* double buffer */
		GL_FALSE,			// stereo
		lpPFD->cRedBits,
		lpPFD->cGreenBits,
		lpPFD->cBlueBits,
		dwAlphaBits,
		0,				// index bits
		dwDepthBits,
		dwStencilBits,
		lpPFD->cAccumRedBits,	// accum bits
		lpPFD->cAccumGreenBits,	// accum bits
		lpPFD->cAccumBlueBits,	// accum bits
		lpPFD->cAccumAlphaBits,	// accum alpha bits
		1				// num samples
		);
#else // _USE_GLD3_WGL
	lpCtx->glVis = (*mesaFuncs.gl_create_visual)(
		GL_TRUE,			// RGB mode
		bAlphaSW,			// Is an alpha buffer required?
		bDouble,			// Is an double-buffering required?
		GL_FALSE,			// stereo
		dwDepthBits,		// depth_size
		dwStencilBits,		// stencil_size
		lpPFD->cAccumBits,	// accum_size
		0,					// colour-index bits
		lpPFD->cRedBits,	// Red bit count
		lpPFD->cGreenBits,	// Green bit count
		lpPFD->cBlueBits,	// Blue bit count
		dwAlphaBits			// Alpha bit count
		);
#endif // _USE_GLD3_WGL

	if (lpCtx->glVis == NULL) {
		ddlogMessage(DDLOG_CRITICAL_OR_WARN, "gl_create_visual failed\n");
		goto return_with_error;
	}

#ifdef _USE_GLD3_WGL
	lpCtx->glCtx = _mesa_create_context(lpCtx->glVis, NULL, (void *)lpCtx, GL_TRUE);
#else
	// Create the Mesa context
	lpCtx->glCtx = (*mesaFuncs.gl_create_context)(
					lpCtx->glVis,	// Mesa visual
					NULL,			// share list context
					(void *)lpCtx,	// Pointer to our driver context
					GL_TRUE			// Direct context flag
				   );
#endif // _USE_GLD3_WGL

	if (lpCtx->glCtx == NULL) {
		ddlogMessage(DDLOG_CRITICAL_OR_WARN, "gl_create_context failed\n");
		goto return_with_error;
	}

	// Create the Mesa framebuffer
#ifdef _USE_GLD3_WGL
	lpCtx->glBuffer = _mesa_create_framebuffer(
		lpCtx->glVis,
		lpCtx->glVis->depthBits > 0,
		lpCtx->glVis->stencilBits > 0,
		lpCtx->glVis->accumRedBits > 0,
		GL_FALSE //swalpha
		);
#else
	lpCtx->glBuffer = (*mesaFuncs.gl_create_framebuffer)(lpCtx->glVis);
#endif // _USE_GLD3_WGL

	if (lpCtx->glBuffer == NULL) {
		ddlogMessage(DDLOG_CRITICAL_OR_WARN, "gl_create_framebuffer failed\n");
		goto return_with_error;
	}

#ifdef _USE_GLD3_WGL
	// Init Mesa internals
	_swrast_CreateContext( lpCtx->glCtx );
	_vbo_CreateContext( lpCtx->glCtx );
	_tnl_CreateContext( lpCtx->glCtx );
	_swsetup_CreateContext( lpCtx->glCtx );

	_gldDriver.InitialiseMesa(lpCtx);
	
	lpCtx->glCtx->imports.warning	= _gld_mesa_warning;
	lpCtx->glCtx->imports.fatal		= _gld_mesa_fatal;

#else
	// Tell Mesa how many texture stages we have
	glb.wMaxSimultaneousTextures = lpCtx->D3DDevDesc.wMaxSimultaneousTextures;
	// Only use as many Units as the spec requires
	if (glb.wMaxSimultaneousTextures > MAX_TEXTURE_UNITS)
		glb.wMaxSimultaneousTextures = MAX_TEXTURE_UNITS;
	lpCtx->glCtx->Const.MaxTextureUnits = glb.wMaxSimultaneousTextures;
	ddlogPrintf(DDLOG_INFO, "Texture stages   : %d", glb.wMaxSimultaneousTextures);

	// Set the max texture size.
	// NOTE: clamped to a max of 1024 for extra performance!
	lpCtx->dwMaxTextureSize = (lpCtx->D3DDevDesc.dwMaxTextureWidth <= 1024) ? lpCtx->D3DDevDesc.dwMaxTextureWidth : 1024;

// Texture resize takes place elsewhere. KH
// NOTE: This was added to workaround an issue with the Intel app.
#if 0
	lpCtx->glCtx->Const.MaxTextureSize = lpCtx->dwMaxTextureSize;
#else
	lpCtx->glCtx->Const.MaxTextureSize = 1024;
#endif

	// Setup the Display Driver pointers
	dglSetupDDPointers(lpCtx->glCtx);

	// Initialise all the Direct3D renderstates
	dglInitStateD3D(lpCtx->glCtx);

#if 0
	// Signal a reload of texture state on next glBegin
	lpCtx->m_texHandleValid = FALSE;
	lpCtx->m_mtex = FALSE;
	lpCtx->m_texturing = FALSE;
#else
	// Set default texture unit state
//	dglSetTexture(lpCtx, 0, NULL);
//	dglSetTexture(lpCtx, 1, NULL);
#endif

	//
	// Set the global texture palette to default values.
	//

	// Clear the entire palette
	ZeroMemory(ppe, sizeof(PALETTEENTRY) * 256);

	// Fill the palette with a default colour.
	// A garish colour is used to catch bugs. Here Magenta is used.
	for (i=0; i < 256; i++) {
		ppe[i].peRed	= 255;
		ppe[i].peGreen	= 0;
		ppe[i].peBlue	= 255;
	}

	RELEASE(lpCtx->lpGlobalPalette);

	if (glb.bDirectDrawPersistant && glb.bPersistantBuffers && glb.lpGlobalPalette)
		hResult = IDirectDrawPalette_AddRef(lpCtx->lpGlobalPalette = glb.lpGlobalPalette);
	else
		hResult = IDirectDraw4_CreatePalette(
				lpCtx->lpDD4,
				DDPCAPS_INITIALIZE | DDPCAPS_8BIT | DDPCAPS_ALLOW256,
				ppe,
				&(lpCtx->lpGlobalPalette),
				NULL);
	if (FAILED(hResult)) {
		ddlogError(DDLOG_ERROR, "Default CreatePalette failed\n", hResult);
		lpCtx->lpGlobalPalette = NULL;
		goto return_with_error;
	}
	if (glb.bDirectDrawPersistant && glb.bPersistantBuffers && !glb.lpGlobalPalette)
		IDirectDrawPalette_AddRef(glb.lpGlobalPalette = lpCtx->lpGlobalPalette);

#endif // _USE_GLD3_WGL

	// ** If we have made it to here then we can enable rendering **
	lpCtx->bCanRender = TRUE;

//	ddlogMessage(DDLOG_SYSTEM, "dglCreateContextBuffers succeded\n");

#ifdef GLD_THREADS
	// Release serialized access
	if (glb.bMultiThreaded)
		LeaveCriticalSection(&CriticalSection);
#endif

	return TRUE;

return_with_error:
	// Clean up before returning.
	// This is critical for secondary devices.

	lpCtx->bCanRender = FALSE;

#ifdef _USE_GLD3_WGL
	// Destroy the Mesa context
	if (lpCtx->glBuffer)
		_mesa_destroy_framebuffer(lpCtx->glBuffer);
	if (lpCtx->glCtx)
		_mesa_destroy_context(lpCtx->glCtx);
	if (lpCtx->glVis)
		_mesa_destroy_visual(lpCtx->glVis);

	// Destroy driver data
	_gldDriver.DestroyDrawable(lpCtx);
#else
	// Destroy the Mesa context
	if (lpCtx->glBuffer)
		(*mesaFuncs.gl_destroy_framebuffer)(lpCtx->glBuffer);
	if (lpCtx->glCtx)
		(*mesaFuncs.gl_destroy_context)(lpCtx->glCtx);
	if (lpCtx->glVis)
		(*mesaFuncs.gl_destroy_visual)(lpCtx->glVis);

	RELEASE(lpCtx->m_pvbuf); // Release D3D vertex buffer
	RELEASE(lpCtx->m_vbuf); // Release D3D vertex buffer

	if (lpCtx->lpViewport3) {
		if (lpCtx->lpDev3) IDirect3DDevice3_DeleteViewport(lpCtx->lpDev3, lpCtx->lpViewport3);
		RELEASE(lpCtx->lpViewport3);
		lpCtx->lpViewport3 = NULL;
	}

	RELEASE(lpCtx->lpDev3);
	if (lpCtx->lpDepth4) {
		if (lpCtx->lpBack4)
			IDirectDrawSurface4_DeleteAttachedSurface(lpCtx->lpBack4, 0L, lpCtx->lpDepth4);
		else
			IDirectDrawSurface4_DeleteAttachedSurface(lpCtx->lpFront4, 0L, lpCtx->lpDepth4);
		RELEASE(lpCtx->lpDepth4);
		lpCtx->lpDepth4 = NULL;
	}
	RELEASE(lpCtx->lpBack4);
	RELEASE(lpCtx->lpFront4);
	else
	if (lpCtx->bFullscreen) {
		IDirectDraw4_RestoreDisplayMode(lpCtx->lpDD4);
		IDirectDraw4_SetCooperativeLevel(lpCtx->lpDD4, NULL, DDSCL_NORMAL);
	}
	RELEASE(lpCtx->lpD3D3);
	RELEASE(lpCtx->lpDD4);
	RELEASE(lpCtx->lpDD1);
#endif // _USE_GLD3_WGL

	lpCtx->bAllocated = FALSE;

#ifdef GLD_THREADS
	// Release serialized access
	if (glb.bMultiThreaded)
		LeaveCriticalSection(&CriticalSection);
#endif

	return FALSE;

#undef DDLOG_CRITICAL_OR_WARN
}

// ***********************************************************************

HGLRC dglCreateContext(
	HDC a,
	const DGL_pixelFormat *lpPF)
{
	int i;
	HGLRC				hGLRC;
	DGL_ctx*			lpCtx;
	static BOOL			bWarnOnce = TRUE;
	DWORD				dwThreadId = GetCurrentThreadId();
    char                szMsg[256];
    HWND                hWnd;
    LONG                lpfnWndProc;

	// Validate license
	if (!dglValidate())
		return NULL;

	// Is context state ready ?
	if (!bContextReady)
		return NULL;

	ddlogPrintf(DDLOG_SYSTEM, "dglCreateContext for HDC=%X, ThreadId=%X", a, dwThreadId);

	// Find next free context.
	// Also ensure that only one Fullscreen context is created at any one time.
	hGLRC = 0; // Default to Not Found
	for (i=0; i<DGL_MAX_CONTEXTS; i++) {
		if (ctxlist[i].bAllocated) {
			if (/*glb.bFullscreen && */ctxlist[i].bFullscreen)
				break;
		} else {
			hGLRC = (HGLRC)(i+1);
			break;
		}
	}

	// Bail if no GLRC was found
	if (!hGLRC)
		return NULL;

	// Set the context pointer
	lpCtx = dglGetContextAddress(hGLRC);
	// Make sure that context is zeroed before we do anything.
	// MFC and C++ apps call wglCreateContext() and wglDeleteContext() multiple times,
	// even though only one context is ever used by the app, so keep it clean. (DaveM)
	ZeroMemory(lpCtx, sizeof(DGL_ctx));
	lpCtx->bAllocated = TRUE;
	// Flag that buffers need creating on next wglMakeCurrent call.
	lpCtx->bHasBeenCurrent = FALSE;
	lpCtx->lpPF = (DGL_pixelFormat *)lpPF;	// cache pixel format
	lpCtx->bCanRender = FALSE;

	// Create all the internal resources here, not in dglMakeCurrent().
	// We do a re-size check in dglMakeCurrent in case of re-allocations. (DaveM)
	// We now try context allocations twice, first with video memory,
	// then again with system memory. This is similar to technique
	// used for dglWglResizeBuffers(). (DaveM)
	if (lpCtx->bHasBeenCurrent == FALSE) {
		if (!dglCreateContextBuffers(a, lpCtx, FALSE)) {
			if (glb.bMessageBoxWarnings && bWarnOnce && dwLogging) {
				bWarnOnce = FALSE;
                switch (nContextError) {
                   case GLDERR_DDRAW: strcpy(szMsg, szDDrawWarning); break;
                   case GLDERR_D3D: strcpy(szMsg, szD3DWarning); break;
                   case GLDERR_MEM: strcpy(szMsg, szResourceWarning); break;
                   case GLDERR_BPP: strcpy(szMsg, szBPPWarning); break;
                   default: strcpy(szMsg, "");
                }
                if (strlen(szMsg))
                    MessageBox(NULL, szMsg, "GLDirect", MB_OK | MB_ICONWARNING);
			}
            // Only need to try again if memory error
            if (nContextError == GLDERR_MEM) {
			    ddlogPrintf(DDLOG_WARN, "dglCreateContext failed 1st time with video memory");
            }
            else {
			    ddlogPrintf(DDLOG_ERROR, "dglCreateContext failed");
                return NULL;
            }
		}
	}

	// Now that we have a hWnd, we can intercept the WindowProc.
    hWnd = lpCtx->hWnd;
    if (hWnd) {
		// Only hook individual window handler once if not hooked before.
		lpfnWndProc = GetWindowLong(hWnd, GWL_WNDPROC);
		if (lpfnWndProc != (LONG)dglWndProc) {
			lpCtx->lpfnWndProc = lpfnWndProc;
			SetWindowLong(hWnd, GWL_WNDPROC, (LONG)dglWndProc);
			}
        // Find the parent window of the app too.
        if (glb.hWndActive == NULL) {
            while (hWnd != NULL) {
                glb.hWndActive = hWnd;
                hWnd = GetParent(hWnd);
            }
            // Hook the parent window too.
            lpfnWndProc = GetWindowLong(glb.hWndActive, GWL_WNDPROC);
            if (glb.hWndActive == lpCtx->hWnd)
                glb.lpfnWndProc = lpCtx->lpfnWndProc;
            else if (lpfnWndProc != (LONG)dglWndProc)
                glb.lpfnWndProc = lpfnWndProc;
            if (glb.lpfnWndProc)
                SetWindowLong(glb.hWndActive, GWL_WNDPROC, (LONG)dglWndProc);
        }
    }

	ddlogPrintf(DDLOG_SYSTEM, "dglCreateContext succeeded for HGLRC=%d", (int)hGLRC);

	return hGLRC;
}

// ***********************************************************************
// Make a DirectGL context current
// Used by wgl functions and dgl functions
BOOL dglMakeCurrent(
	HDC a,
	HGLRC b)
{
	int context;
	DGL_ctx* lpCtx;
	HWND hWnd;
	BOOL bNeedResize = FALSE;
	BOOL bWindowChanged, bContextChanged;
	LPDIRECTDRAWCLIPPER	lpddClipper;
	DWORD dwThreadId = GetCurrentThreadId();
	LONG lpfnWndProc;

	// Validate license
	if (!dglValidate())
		return FALSE;

	// Is context state ready ?
	if (!bContextReady)
		return FALSE;

	context = (int)b; // This is as a result of STRICT!
	ddlogPrintf(DDLOG_SYSTEM, "dglMakeCurrent: HDC=%X, HGLRC=%d, ThreadId=%X", a, context, dwThreadId);

	// If the HGLRC is NULL then make no context current;
	// Ditto if the HDC is NULL either. (DaveM)
	if (context == 0 || a == 0) {
		// Corresponding Mesa operation
#ifdef _USE_GLD3_WGL
		_mesa_make_current(NULL, NULL);
#else
		(*mesaFuncs.gl_make_current)(NULL, NULL);
#endif
		dglSetCurrentContext(0);
		return TRUE;
	}

	// Make sure the HGLRC is in range
	if ((context > DGL_MAX_CONTEXTS) || (context < 0)) {
		ddlogMessage(DDLOG_ERROR, "dglMakeCurrent: HGLRC out of range\n");
		return FALSE;
	}

	// Find address of context and make sure that it has been allocated
	lpCtx = dglGetContextAddress(b);
	if (!lpCtx->bAllocated) {
		ddlogMessage(DDLOG_ERROR, "dglMakeCurrent: Context not allocated\n");
//		return FALSE;
		return TRUE; // HACK: Shuts up "WebLab Viewer Pro". KeithH
	}

#ifdef GLD_THREADS
	// Serialize access to DirectDraw or DDS operations
	if (glb.bMultiThreaded)
		EnterCriticalSection(&CriticalSection);
#endif

	// Check if window has changed
	hWnd = (a != lpCtx->hDC) ? WindowFromDC(a) : lpCtx->hWnd;
	bWindowChanged = (hWnd != lpCtx->hWnd) ? TRUE : FALSE;
	bContextChanged = (b != dglGetCurrentContext()) ? TRUE : FALSE;

	// If the window has changed, make sure the clipper is updated. (DaveM)
	if (glb.bDirectDrawPersistant && !lpCtx->bFullscreen && (bWindowChanged || bContextChanged)) {
		lpCtx->hWnd = hWnd;
#ifndef _USE_GLD3_WGL
		IDirectDrawSurface4_GetClipper(lpCtx->lpFront4, &lpddClipper);
		IDirectDrawClipper_SetHWnd(lpddClipper, 0, lpCtx->hWnd);
		IDirectDrawClipper_Release(lpddClipper);
#endif // _USE_GLD3_WGL
	}

	// Make sure hDC and hWnd is current. (DaveM)
	// Obtain the dimensions of the rendering window
	lpCtx->hDC = a; // Cache DC
	lpCtx->hWnd = hWnd;
	hWndLastActive = hWnd;

	// Check for non-window DC = memory DC ?
	if (hWnd == NULL) {
		if (GetClipBox(a, &lpCtx->rcScreenRect) == ERROR) {
			ddlogMessage(DDLOG_WARN, "GetClipBox failed in dglMakeCurrent\n");
			SetRect(&lpCtx->rcScreenRect, 0, 0, 0, 0);
		}
	}
	else if (!GetClientRect(lpCtx->hWnd, &lpCtx->rcScreenRect)) {
		ddlogMessage(DDLOG_WARN, "GetClientRect failed in dglMakeCurrent\n");
		SetRect(&lpCtx->rcScreenRect, 0, 0, 0, 0);
	}
	// Check if buffers need to be re-sized;
	// If so, wait until Mesa GL stuff is setup before re-sizing;
	if (lpCtx->dwWidth != lpCtx->rcScreenRect.right - lpCtx->rcScreenRect.left ||
		lpCtx->dwHeight != lpCtx->rcScreenRect.bottom - lpCtx->rcScreenRect.top)
		bNeedResize = TRUE;

	// Now we can update our globals
	dglSetCurrentContext(b);

	// Corresponding Mesa operation
#ifdef _USE_GLD3_WGL
	_mesa_make_current(lpCtx->glCtx, lpCtx->glBuffer);
	lpCtx->glCtx->Driver.UpdateState(lpCtx->glCtx, _NEW_ALL);
	if (bNeedResize) {
		// Resize buffers (Note Mesa GL needs to be setup beforehand);
		// Resize Mesa internal buffer too via glViewport() command,
		// which subsequently calls dglWglResizeBuffers() too.
		lpCtx->glCtx->Driver.Viewport(lpCtx->glCtx, 0, 0, lpCtx->dwWidth, lpCtx->dwHeight);
		lpCtx->bHasBeenCurrent = TRUE;
	}
#else
	(*mesaFuncs.gl_make_current)(lpCtx->glCtx, lpCtx->glBuffer);

	dglSetupDDPointers(lpCtx->glCtx);

	// Insure DirectDraw surfaces fit current window DC
	if (bNeedResize) {
		// Resize buffers (Note Mesa GL needs to be setup beforehand);
		// Resize Mesa internal buffer too via glViewport() command,
		// which subsequently calls dglWglResizeBuffers() too.
		(*mesaFuncs.gl_Viewport)(lpCtx->glCtx, 0, 0, lpCtx->dwWidth, lpCtx->dwHeight);
		lpCtx->bHasBeenCurrent = TRUE;
	}
#endif // _USE_GLD3_WGL
	ddlogPrintf(DDLOG_SYSTEM, "dglMakeCurrent: width = %d, height = %d", lpCtx->dwWidth, lpCtx->dwHeight);

	// We have to clear D3D back buffer and render state if emulated front buffering
	// for different window (but not context) like in Solid Edge.
	if (glb.bDirectDrawPersistant && glb.bPersistantBuffers
		&& (bWindowChanged /* || bContextChanged */) && lpCtx->EmulateSingle) {
#ifdef _USE_GLD3_WGL
//		IDirect3DDevice8_EndScene(lpCtx->pDev);
//		lpCtx->bSceneStarted = FALSE;
		lpCtx->glCtx->Driver.Clear(lpCtx->glCtx, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
			GL_TRUE, 0, 0, lpCtx->dwWidth, lpCtx->dwHeight);
#else
		IDirect3DDevice3_EndScene(lpCtx->lpDev3);
		lpCtx->bSceneStarted = FALSE;
		dglClearD3D(lpCtx->glCtx, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
			GL_TRUE, 0, 0, lpCtx->dwWidth, lpCtx->dwHeight);
#endif // _USE_GLD3_WGL
	}

	// The first time we call MakeCurrent we set the initial viewport size
	if (lpCtx->bHasBeenCurrent == FALSE)
#ifdef _USE_GLD3_WGL
		lpCtx->glCtx->Driver.Viewport(lpCtx->glCtx, 0, 0, lpCtx->dwWidth, lpCtx->dwHeight);
#else
		(*mesaFuncs.gl_Viewport)(lpCtx->glCtx, 0, 0, lpCtx->dwWidth, lpCtx->dwHeight);
#endif // _USE_GLD3_WGL
	lpCtx->bHasBeenCurrent = TRUE;

#ifdef GLD_THREADS
	// Release serialized access
	if (glb.bMultiThreaded)
		LeaveCriticalSection(&CriticalSection);
#endif

	return TRUE;
}

// ***********************************************************************

BOOL dglDeleteContext(
	HGLRC a)
{
	DGL_ctx* lpCtx;
	DWORD dwThreadId = GetCurrentThreadId();
    char argstr[256];

#if 0	// We have enough trouble throwing exceptions as it is... (DaveM)
	// Validate license
	if (!dglValidate())
		return FALSE;
#endif

	// Is context state ready ?
	if (!bContextReady)
		return FALSE;

	ddlogPrintf(DDLOG_SYSTEM, "dglDeleteContext: Deleting context HGLRC=%d, ThreadId=%X", (int)a, dwThreadId);

	// Make sure the HGLRC is in range
	if (((int) a> DGL_MAX_CONTEXTS) || ((int)a < 0)) {
		ddlogMessage(DDLOG_ERROR, "dglDeleteCurrent: HGLRC out of range\n");
		return FALSE;
	}

	// Make sure context is valid
	lpCtx = dglGetContextAddress(a);
	if (!lpCtx->bAllocated) {
		ddlogPrintf(DDLOG_WARN, "Tried to delete unallocated context HGLRC=%d", (int)a);
//		return FALSE;
		return TRUE; // HACK: Shuts up "WebLab Viewer Pro". KeithH
	}

	// Make sure context is de-activated
	if (a == dglGetCurrentContext()) {
		ddlogPrintf(DDLOG_WARN, "dglDeleteContext: context HGLRC=%d still active", (int)a);
		dglMakeCurrent(NULL, NULL);
	}

#ifdef GLD_THREADS
	// Serialize access to DirectDraw or DDS operations
	if (glb.bMultiThreaded)
		EnterCriticalSection(&CriticalSection);
#endif

	// We are about to destroy all Direct3D objects.
	// Therefore we must disable rendering
	lpCtx->bCanRender = FALSE;

	// This exception handler was installed to catch some
	// particularly nasty apps. Console apps that call exit()
	// fall into this catagory (i.e. Win32 Glut).

    // VC cannot successfully implement multiple exception handlers
    // if more than one exception occurs. Therefore reverting back to
    // single exception handler as Keith originally had it. (DaveM)

#define WARN_MESSAGE(p) strcpy(argstr, (#p));
#define SAFE_RELEASE(p) WARN_MESSAGE(p); RELEASE(p);

__try {
#ifdef _USE_GLD3_WGL
    WARN_MESSAGE(gl_destroy_framebuffer);
	if (lpCtx->glBuffer)
		_mesa_destroy_framebuffer(lpCtx->glBuffer);
    WARN_MESSAGE(gl_destroy_context);
	if (lpCtx->glCtx)
		_mesa_destroy_context(lpCtx->glCtx);
    WARN_MESSAGE(gl_destroy_visual);
	if (lpCtx->glVis)
		_mesa_destroy_visual(lpCtx->glVis);

	_gldDriver.DestroyDrawable(lpCtx);
#else
	// Destroy the Mesa context
    WARN_MESSAGE(gl_destroy_framebuffer);
	if (lpCtx->glBuffer)
		(*mesaFuncs.gl_destroy_framebuffer)(lpCtx->glBuffer);
    WARN_MESSAGE(gl_destroy_context);
	if (lpCtx->glCtx)
		(*mesaFuncs.gl_destroy_context)(lpCtx->glCtx);
    WARN_MESSAGE(gl_destroy_visual);
	if (lpCtx->glVis)
		(*mesaFuncs.gl_destroy_visual)(lpCtx->glVis);

	SAFE_RELEASE(lpCtx->m_pvbuf); // release D3D vertex buffer
	SAFE_RELEASE(lpCtx->m_vbuf); // release D3D vertex buffer

	// Delete the global palette
	SAFE_RELEASE(lpCtx->lpGlobalPalette);

	// Clean up.
	if (lpCtx->lpViewport3) {
		if (lpCtx->lpDev3) IDirect3DDevice3_DeleteViewport(lpCtx->lpDev3, lpCtx->lpViewport3);
		SAFE_RELEASE(lpCtx->lpViewport3);
		lpCtx->lpViewport3 = NULL;
	}

	SAFE_RELEASE(lpCtx->lpDev3);
	if (lpCtx->lpDepth4) {
		if (lpCtx->lpBack4)
			IDirectDrawSurface4_DeleteAttachedSurface(lpCtx->lpBack4, 0L, lpCtx->lpDepth4);
		else
			IDirectDrawSurface4_DeleteAttachedSurface(lpCtx->lpFront4, 0L, lpCtx->lpDepth4);
		SAFE_RELEASE(lpCtx->lpDepth4);
		lpCtx->lpDepth4 = NULL;
	}
	SAFE_RELEASE(lpCtx->lpBack4);
	SAFE_RELEASE(lpCtx->lpFront4);
	if (lpCtx->bFullscreen) {
		IDirectDraw4_RestoreDisplayMode(lpCtx->lpDD4);
		IDirectDraw4_SetCooperativeLevel(lpCtx->lpDD4, NULL, DDSCL_NORMAL);
	}
	SAFE_RELEASE(lpCtx->lpD3D3);
	SAFE_RELEASE(lpCtx->lpDD4);
	SAFE_RELEASE(lpCtx->lpDD1);
#endif // _ULSE_GLD3_WGL

}
__except(EXCEPTION_EXECUTE_HANDLER) {
    ddlogPrintf(DDLOG_WARN, "Exception raised in dglDeleteContext: %s", argstr);
}

	// Restore the window message handler because this context may be used
	// again by another window with a *different* message handler. (DaveM)
	if (lpCtx->lpfnWndProc) {
		SetWindowLong(lpCtx->hWnd, GWL_WNDPROC, (LONG)lpCtx->lpfnWndProc);
		lpCtx->lpfnWndProc = (LONG)NULL;
		}

	lpCtx->bAllocated = FALSE; // This context is now free for use

#ifdef GLD_THREADS
	// Release serialized access
	if (glb.bMultiThreaded)
		LeaveCriticalSection(&CriticalSection);
#endif

	return TRUE;
}

// ***********************************************************************

BOOL dglSwapBuffers(
	HDC hDC)
{
	RECT		rSrcRect;	// Source rectangle
	RECT		rDstRect;	// Destination rectangle
	POINT		pt;
	HRESULT		hResult;

	DDBLTFX		bltFX;
	DWORD		dwBlitFlags;
	DDBLTFX		*lpBltFX;

//	DWORD		dwThreadId = GetCurrentThreadId();
	HGLRC		hGLRC = dglGetCurrentContext();
	DGL_ctx		*lpCtx = dglGetContextAddress(hGLRC);
	HWND		hWnd;

	HDC 		hDCAux;		// for memory DC
	int 		x,y,w,h;	// for memory DC BitBlt

#if 0	// Perhaps not a good idea. Called too often. KH
	// Validate license
	if (!dglValidate())
		return FALSE;
#endif

	if (!lpCtx) {
		return TRUE; //FALSE; // No current context
	}

	if (!lpCtx->bCanRender) {
		// Don't return false else some apps will bail.
		return TRUE;
	}

	hWnd = lpCtx->hWnd;
	if (hDC != lpCtx->hDC) {
		ddlogPrintf(DDLOG_WARN, "dglSwapBuffers: HDC=%X does not match HDC=%X for HGLRC=%d", hDC, lpCtx->hDC, hGLRC);
		hWnd = WindowFromDC(hDC);
	}

#ifndef _USE_GLD3_WGL
	// Ensure that the surfaces exist before we tell
	// the device to render to them.
	IDirectDraw4_RestoreAllSurfaces(lpCtx->lpDD4);

	// Make sure that the vertex caches have been emptied
//	dglStateChange(lpCtx);

	// Some OpenGL programs don't issue a glFinish - check for it here.
	if (lpCtx->bSceneStarted) {
		IDirect3DDevice3_EndScene(lpCtx->lpDev3);
		lpCtx->bSceneStarted = FALSE;
	}
#endif

#if 0
	// If the calling app is not active then we don't need to Blit/Flip.
	// We can therefore simply return TRUE.
	if (!glb.bAppActive)
		return TRUE;
	// Addendum: This is WRONG! We should bail if the app is *minimized*,
	//           not merely if the app is just plain 'not active'.
	//           KeithH, 27/May/2000.
#endif

	// Check for non-window DC = memory DC ?
	if (hWnd == NULL) {
		if (GetClipBox(hDC, &rSrcRect) == ERROR)
			return TRUE;
		// Use GDI BitBlt instead from compatible DirectDraw DC
		x = rSrcRect.left;
		y = rSrcRect.top;
		w = rSrcRect.right - rSrcRect.left;
		h = rSrcRect.bottom - rSrcRect.top;

		// Ack. DX8 does not have a GetDC() function...
                // TODO: Defer to DX7 or DX9 drivers... (DaveM)
		return TRUE;
	}

	// Bail if window client region is not drawable, like in Solid Edge
	if (!IsWindow(hWnd) /* || !IsWindowVisible(hWnd) */ || !GetClientRect(hWnd, &rSrcRect))
		return TRUE;

#ifdef GLD_THREADS
	// Serialize access to DirectDraw or DDS operations
	if (glb.bMultiThreaded)
		EnterCriticalSection(&CriticalSection);
#endif

#ifdef _USE_GLD3_WGL
	// Notify Mesa of impending swap, so Mesa can flush internal buffers.
	_mesa_notifySwapBuffers(lpCtx->glCtx);
	// Now perform driver buffer swap
	_gldDriver.SwapBuffers(lpCtx, hDC, hWnd);
#else
	if (lpCtx->bFullscreen) {
		// Sync with retrace if required
		if (glb.bWaitForRetrace) {
			IDirectDraw4_WaitForVerticalBlank(
				lpCtx->lpDD4,
				DDWAITVB_BLOCKBEGIN,
				0);
		}

		// Perform the fullscreen flip
		TRY(IDirectDrawSurface4_Flip(
			lpCtx->lpFront4,
			NULL,
			DDFLIP_WAIT),
			"dglSwapBuffers: Flip");
	} else {
		// Calculate current window position and size
		pt.x = pt.y = 0;
		ClientToScreen(hWnd, &pt);
		GetClientRect(hWnd, &rDstRect);
		if (rDstRect.right > lpCtx->dwModeWidth)
			rDstRect.right = lpCtx->dwModeWidth;
		if (rDstRect.bottom > lpCtx->dwModeHeight)
			rDstRect.bottom = lpCtx->dwModeHeight;
		OffsetRect(&rDstRect, pt.x, pt.y);
		rSrcRect.left = rSrcRect.top = 0;
		rSrcRect.right = lpCtx->dwWidth;
		rSrcRect.bottom = lpCtx->dwHeight;
		if (rSrcRect.right > lpCtx->dwModeWidth)
			rSrcRect.right = lpCtx->dwModeWidth;
		if (rSrcRect.bottom > lpCtx->dwModeHeight)
			rSrcRect.bottom = lpCtx->dwModeHeight;

		if (glb.bWaitForRetrace) {
			// Sync the blit to the vertical retrace
			ZeroMemory(&bltFX, sizeof(bltFX));
			bltFX.dwSize = sizeof(bltFX);
			bltFX.dwDDFX = DDBLTFX_NOTEARING;
			dwBlitFlags = DDBLT_WAIT | DDBLT_DDFX;
			lpBltFX = &bltFX;
		} else {
			dwBlitFlags = DDBLT_WAIT;
			lpBltFX = NULL;
		}

		// Perform the actual blit
		TRY(IDirectDrawSurface4_Blt(
			lpCtx->lpFront4,
			&rDstRect,
			lpCtx->lpBack4, // Blit source
			&rSrcRect,
			dwBlitFlags,
			lpBltFX),
			"dglSwapBuffers: Blt");
	}
#endif // _USE_GLD3_WGL

#ifdef GLD_THREADS
	// Release serialized access
	if (glb.bMultiThreaded)
		LeaveCriticalSection(&CriticalSection);
#endif

    // TODO: Re-instate rendering bitmap snapshot feature??? (DaveM)

	// Render frame is completed
	ValidateRect(hWnd, NULL);
	lpCtx->bFrameStarted = FALSE;

	return TRUE;
}

// ***********************************************************************
