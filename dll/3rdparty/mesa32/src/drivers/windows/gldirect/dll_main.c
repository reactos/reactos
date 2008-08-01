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
* Description:  Win32 DllMain functions.
*
****************************************************************************/

// INITGUID must only be defined once.
// Don't put it in a shared header file!
// GLD3 uses dxguid.lib, so INITGUID must *not* be used!
#ifndef _USE_GLD3_WGL
#define INITGUID
#endif // _USE_GLD3_WGL

#include "dllmain.h"

//#include "snap/graphics.h"
//#include "drvlib/os/os.h"

#ifdef _USE_GLD3_WGL
typedef void (APIENTRY *LPDGLSPLASHSCREEN)(int, int, char*);
#include "gld_driver.h"
#endif

// ***********************************************************************

BOOL bInitialized = FALSE;              // callback driver initialized?
BOOL bExited = FALSE;                   // callback driver exited this instance?
HINSTANCE hInstanceDll = NULL;          // DLL instance handle

static BOOL bDriverValidated = FALSE;	// prior validation status
static BOOL	bSplashScreen = TRUE;	    // Splash Screen ?
static BOOL bValidINIFound = FALSE;     // Have we found a valid INI file?

HHOOK 	hKeyHook = NULL;				// global keyboard handler hook

// Multi-threaded support needs to be reflected in Mesa code. (DaveM)
int _gld_bMultiThreaded = FALSE;

// ***********************************************************************

DWORD dwLogging = 0; 					// Logging flag
DWORD dwDebugLevel = 0;                 // Log debug level

char szLogPath[_MAX_PATH] = {"\0"};		// Log file path
char szSNAPPath[_MAX_PATH] = {"\0"};	// SNAP driver path

#ifndef _USE_GLD3_WGL
DGL_wglFuncs wglFuncs = {
	sizeof(DGL_wglFuncs),
	DGL_ChoosePixelFormat,
	DGL_CopyContext,
	DGL_CreateContext,
	DGL_CreateLayerContext,
	DGL_DeleteContext,
	DGL_DescribeLayerPlane,
	DGL_DescribePixelFormat,
	DGL_GetCurrentContext,
	DGL_GetCurrentDC,
	DGL_GetDefaultProcAddress,
	DGL_GetLayerPaletteEntries,
	DGL_GetPixelFormat,
	DGL_GetProcAddress,
	DGL_MakeCurrent,
	DGL_RealizeLayerPalette,
	DGL_SetLayerPaletteEntries,
	DGL_SetPixelFormat,
	DGL_ShareLists,
	DGL_SwapBuffers,
	DGL_SwapLayerBuffers,
	DGL_UseFontBitmapsA,
	DGL_UseFontBitmapsW,
	DGL_UseFontOutlinesA,
	DGL_UseFontOutlinesW,
};

DGL_mesaFuncs mesaFuncs = {
	sizeof(DGL_mesaFuncs),
};
#endif // _USE_GLD3_WGL

// ***********************************************************************

typedef struct {
	DWORD	dwDriver;			// 0=SciTech SW, 1=Direct3D SW, 2=Direct3D HW
	BOOL	bMipmapping;		// 0=off, 1=on
	BOOL	bMultitexture;		// 0=off, 1=on
	BOOL	bWaitForRetrace;	// 0=off, 1=on
	BOOL	bFullscreenBlit;	// 0=off, 1=on
	BOOL	bFastFPU;			// 0=off, 1=on
	BOOL	bDirectDrawPersistant;// 0=off, 1=on
	BOOL	bPersistantBuffers; // 0=off, 1=on
	DWORD	dwLogging;			// 0=off, 1=normal, 2=crash-proof
	DWORD	dwLoggingSeverity;	// 0=all, 1=warnings+errors, 2=errors only
	BOOL	bMessageBoxWarnings;// 0=off, 1=on
	BOOL	bMultiThreaded;		// 0=off, 1=on
	BOOL	bAppCustomizations;	// 0=off, 1=on
	BOOL	bHotKeySupport;		// 0=off, 1=on
	BOOL	bSplashScreen;		// 0=off, 1=on

#ifdef _USE_GLD3_WGL
	//
	// New for GLDirect 3.0
	//
	DWORD	dwAdapter;			// DX8 adpater ordinal
	DWORD	dwTnL;				// Transform & Lighting type
	DWORD	dwMultisample;		// DX8 multisample type
#endif // _USE_GLD3_WGL
} INI_settings;

static INI_settings ini;

// ***********************************************************************

BOOL APIENTRY DGL_initDriver(
#ifdef _USE_GLD3_WGL
	void)
{
#else
	DGL_wglFuncs *lpWglFuncs,
	DGL_mesaFuncs *lpMesaFuncs)
{
	// Check for valid pointers
	if ((lpWglFuncs == NULL) || (lpMesaFuncs == NULL))
		return FALSE;

	// Check for valid structs
	if (lpWglFuncs->dwSize != sizeof(DGL_wglFuncs)) {
		return FALSE;
	}

	// Check for valid structs
	if (lpMesaFuncs->dwSize != sizeof(DGL_mesaFuncs)) {
		return FALSE;
	}

	// Copy the Mesa functions
	memcpy(&mesaFuncs, lpMesaFuncs, sizeof(DGL_mesaFuncs));

	// Pass back the wgl functions
	memcpy(lpWglFuncs, &wglFuncs, sizeof(DGL_wglFuncs));
#endif // _USE_GLD3_WGL

    // Finally initialize the callback driver
    if (!dglInitDriver())
        return FALSE;

	return TRUE;
};

// ***********************************************************************

BOOL ReadINIFile(
	HINSTANCE hInstance)
{
	char		szModuleFilename[MAX_PATH];
	char		szSystemDirectory[MAX_PATH];
	const char	szSectionName[] = "Config";
	char		szINIFile[MAX_PATH];
	int			pos;

	// Now using the DLL module handle. KeithH, 24/May/2000.
	// Addendum: GetModuleFileName(NULL, ...    returns process filename,
	//           GetModuleFileName(hModule, ... returns DLL filename,

	// Get the dll path and filename.
	GetModuleFileName(hInstance, &szModuleFilename[0], MAX_PATH); // NULL for current process
	// Get the System directory.
	GetSystemDirectory(&szSystemDirectory[0], MAX_PATH);

	// Test to see if DLL is in system directory.
	if (strnicmp(szModuleFilename, szSystemDirectory, strlen(szSystemDirectory))==0) {
		// DLL *is* in system directory.
		// Return FALSE to indicate that registry keys should be read.
		return FALSE;
	}

	// Compose filename of INI file
	strcpy(szINIFile, szModuleFilename);
	pos = strlen(szINIFile);
	while (szINIFile[pos] != '\\') {
		pos--;
	}
	szINIFile[pos+1] = '\0';
    // Use run-time DLL path for log file too
    strcpy(szLogPath, szINIFile);
    szLogPath[pos] = '\0';
    // Complete full INI file path
	strcat(szINIFile, "gldirect.ini");

	// Read settings from private INI file.
	// Note that defaults are contained in the calls.
	ini.dwDriver = GetPrivateProfileInt(szSectionName, "dwDriver", 2, szINIFile);
	ini.bMipmapping = GetPrivateProfileInt(szSectionName, "bMipmapping", 1, szINIFile);
	ini.bMultitexture = GetPrivateProfileInt(szSectionName, "bMultitexture", 1, szINIFile);
	ini.bWaitForRetrace = GetPrivateProfileInt(szSectionName, "bWaitForRetrace", 0, szINIFile);
	ini.bFullscreenBlit = GetPrivateProfileInt(szSectionName, "bFullscreenBlit", 0, szINIFile);
	ini.bFastFPU = GetPrivateProfileInt(szSectionName, "bFastFPU", 1, szINIFile);
	ini.bDirectDrawPersistant = GetPrivateProfileInt(szSectionName, "bPersistantDisplay", 0, szINIFile);
	ini.bPersistantBuffers = GetPrivateProfileInt(szSectionName, "bPersistantResources", 0, szINIFile);
	ini.dwLogging = GetPrivateProfileInt(szSectionName, "dwLogging", 0, szINIFile);
	ini.dwLoggingSeverity = GetPrivateProfileInt(szSectionName, "dwLoggingSeverity", 0, szINIFile);
	ini.bMessageBoxWarnings = GetPrivateProfileInt(szSectionName, "bMessageBoxWarnings", 0, szINIFile);
	ini.bMultiThreaded = GetPrivateProfileInt(szSectionName, "bMultiThreaded", 0, szINIFile);
	ini.bAppCustomizations = GetPrivateProfileInt(szSectionName, "bAppCustomizations", 1, szINIFile);
	ini.bHotKeySupport = GetPrivateProfileInt(szSectionName, "bHotKeySupport", 0, szINIFile);
	ini.bSplashScreen = GetPrivateProfileInt(szSectionName, "bSplashScreen", 1, szINIFile);

#ifdef _USE_GLD3_WGL
	// New for GLDirect 3.x
	ini.dwAdapter		= GetPrivateProfileInt(szSectionName, "dwAdapter", 0, szINIFile);
	// dwTnL now defaults to zero (chooses TnL at runtime). KeithH
	ini.dwTnL			= GetPrivateProfileInt(szSectionName, "dwTnL", 0, szINIFile);
	ini.dwMultisample	= GetPrivateProfileInt(szSectionName, "dwMultisample", 0, szINIFile);
#endif

	return TRUE;
}

// ***********************************************************************

BOOL dllReadRegistry(
	HINSTANCE hInstance)
{
	// Read settings from INI file, if available
    bValidINIFound = FALSE;
	if (ReadINIFile(hInstance)) {
		const char *szRendering[3] = {
			"SciTech Software Renderer",
			"Direct3D MMX Software Renderer",
			"Direct3D Hardware Renderer"
		};
		// Set globals
		glb.bPrimary = 1;
		glb.bHardware = (ini.dwDriver == 2) ? 1 : 0;
#ifndef _USE_GLD3_WGL
		memset(&glb.ddGuid, 0, sizeof(glb.ddGuid));
		glb.d3dGuid = (ini.dwDriver == 2) ? IID_IDirect3DHALDevice : IID_IDirect3DRGBDevice;
#endif // _USE_GLD3_WGL
		strcpy(glb.szDDName, "Primary");
		strcpy(glb.szD3DName, szRendering[ini.dwDriver]);
		glb.dwRendering = ini.dwDriver;
		glb.bUseMipmaps = ini.bMipmapping;
		glb.bMultitexture = ini.bMultitexture;
		glb.bWaitForRetrace = ini.bWaitForRetrace;
		glb.bFullscreenBlit = ini.bFullscreenBlit;
		glb.bFastFPU = ini.bFastFPU;
		glb.bDirectDrawPersistant = ini.bDirectDrawPersistant;
		glb.bPersistantBuffers = ini.bPersistantBuffers;
		dwLogging = ini.dwLogging;
		dwDebugLevel = ini.dwLoggingSeverity;
		glb.bMessageBoxWarnings = ini.bMessageBoxWarnings;
		glb.bMultiThreaded = ini.bMultiThreaded;
		glb.bAppCustomizations = ini.bAppCustomizations;
        glb.bHotKeySupport = ini.bHotKeySupport;
		bSplashScreen = ini.bSplashScreen;
#ifdef _USE_GLD3_WGL
		// New for GLDirect 3.x
		glb.dwAdapter		= ini.dwAdapter;
		glb.dwDriver		= ini.dwDriver;
		glb.dwTnL			= ini.dwTnL;
		glb.dwMultisample	= ini.dwMultisample;
#endif
        bValidINIFound = TRUE;
		return TRUE;
	}
	// Read settings from registry
	else {
	HKEY	hReg;
	DWORD	cbValSize;
	DWORD	dwType = REG_SZ; // Registry data type for strings
	BOOL	bRegistryError;
	BOOL	bSuccess;

#define REG_READ_DWORD(a, b)							\
	cbValSize = sizeof(b);								\
	if (ERROR_SUCCESS != RegQueryValueEx( hReg, (a),	\
		NULL, NULL, (LPBYTE)&(b), &cbValSize ))			\
		bRegistryError = TRUE;

#define REG_READ_DEVICEID(a, b)									\
	cbValSize = MAX_DDDEVICEID_STRING;							\
	if(ERROR_SUCCESS != RegQueryValueEx(hReg, (a), 0, &dwType,	\
					(LPBYTE)&(b), &cbValSize))					\
		bRegistryError = TRUE;

#define REG_READ_STRING(a, b)									\
	cbValSize = sizeof((b));									\
	if(ERROR_SUCCESS != RegQueryValueEx(hReg, (a), 0, &dwType,	\
					(LPBYTE)&(b), &cbValSize))					\
		bRegistryError = TRUE;

	// Read settings from the registry.

	// Open the registry key for the current user if it exists.
	bSuccess = (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
									  DIRECTGL_REG_SETTINGS_KEY,
									  0,
									  KEY_READ,
									  &hReg));
    // Otherwise open the registry key for the local machine.
    if (!bSuccess)
        bSuccess = (ERROR_SUCCESS == RegOpenKeyEx(DIRECTGL_REG_KEY_ROOT,
									  DIRECTGL_REG_SETTINGS_KEY,
									  0,
									  KEY_READ,
									  &hReg));
    if (!bSuccess)
        return FALSE;

	bRegistryError = FALSE;

	REG_READ_DWORD(DIRECTGL_REG_SETTING_PRIMARY, glb.bPrimary);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_D3D_HW, glb.bHardware);
#ifndef _USE_GLD3_WGL
	REG_READ_DWORD(DIRECTGL_REG_SETTING_DD_GUID, glb.ddGuid);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_D3D_GUID, glb.d3dGuid);
#endif // _USE_GLD3_WGL
	REG_READ_DWORD(DIRECTGL_REG_SETTING_LOGGING, dwLogging);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_DEBUGLEVEL, dwDebugLevel);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_RENDERING, glb.dwRendering);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_MULTITEXTURE, glb.bMultitexture);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_WAITFORRETRACE, glb.bWaitForRetrace);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_FULLSCREENBLIT, glb.bFullscreenBlit);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_USEMIPMAPS, glb.bUseMipmaps);

	REG_READ_DEVICEID(DIRECTGL_REG_SETTING_DD_NAME, glb.szDDName);
	REG_READ_DEVICEID(DIRECTGL_REG_SETTING_D3D_NAME, glb.szD3DName);

	REG_READ_DWORD(DIRECTGL_REG_SETTING_MSGBOXWARNINGS, glb.bMessageBoxWarnings);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_PERSISTDISPLAY, glb.bDirectDrawPersistant);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_PERSISTBUFFERS, glb.bPersistantBuffers);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_FASTFPU, glb.bFastFPU);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_HOTKEYS, glb.bHotKeySupport);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_MULTITHREAD, glb.bMultiThreaded);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_APPCUSTOM, glb.bAppCustomizations);
    REG_READ_DWORD(DIRECTGL_REG_SETTING_SPLASHSCREEN, bSplashScreen);

#ifdef _USE_GLD3_WGL
	// New for GLDirect 3.x
	glb.dwDriver = glb.dwRendering;
	REG_READ_DWORD(DIRECTGL_REG_SETTING_ADAPTER, glb.dwAdapter);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_TNL, glb.dwTnL);
	REG_READ_DWORD(DIRECTGL_REG_SETTING_MULTISAMPLE, glb.dwMultisample);
#endif

	RegCloseKey(hReg);

	// Open the global registry key for GLDirect
	bSuccess = (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
									  DIRECTGL_REG_SETTINGS_KEY,
									  0,
									  KEY_READ,
									  &hReg));
    if (bSuccess) {
	    // Read the installation path for GLDirect
	    REG_READ_STRING("InstallLocation",szLogPath);
	    RegCloseKey(hReg);
        }

	if (bRegistryError || !bSuccess)
		return FALSE;
	else
		
		return TRUE;

#undef REG_READ_DWORD
#undef REG_READ_DEVICEID
#undef REG_READ_STRING
	}
}

// ***********************************************************************

BOOL dllWriteRegistry(
	void )
{
	HKEY 	hReg;
	DWORD 	dwCreateDisposition, cbValSize;
	BOOL 	bRegistryError = FALSE;

#define REG_WRITE_DWORD(a, b)							\
	cbValSize = sizeof(b);								\
	if (ERROR_SUCCESS != RegSetValueEx( hReg, (a),		\
		0, REG_DWORD, (LPBYTE)&(b), cbValSize ))		\
		bRegistryError = TRUE;

	if (ERROR_SUCCESS == RegCreateKeyEx( DIRECTGL_REG_KEY_ROOT, DIRECTGL_REG_SETTINGS_KEY,
										0, NULL, 0, KEY_WRITE, NULL, &hReg,
										&dwCreateDisposition )) {
		RegFlushKey(hReg); // Make sure keys are written to disk
		RegCloseKey(hReg);
		hReg = NULL;
		}

	if (bRegistryError)
		return FALSE;
	else
		return TRUE;

#undef REG_WRITE_DWORD
}

// ***********************************************************************

void dglInitHotKeys(HINSTANCE hInstance)
{
	// Hot-Key support at all?
	if (!glb.bHotKeySupport)
		return;

	// Install global keyboard interceptor
	hKeyHook = SetWindowsHookEx(WH_KEYBOARD, dglKeyProc, hInstance, 0);
}

// ***********************************************************************

void dglExitHotKeys(void)
{
	// Hot-Key support at all?
	if (!glb.bHotKeySupport)
		return;

	// Remove global keyboard interceptor
	if (hKeyHook)
		UnhookWindowsHookEx(hKeyHook);
	hKeyHook = NULL;
}

// ***********************************************************************

// Note: This app-customization step must be performed in both the main
// OpenGL32 driver and the callback driver DLLs for multithreading option.
void dglSetAppCustomizations(void)
{
	char		szModuleFileName[MAX_PATH];
	int			iSize = MAX_PATH;

	// Get the currently loaded EXE filename.
	GetModuleFileName(NULL, &szModuleFileName[0], MAX_PATH); // NULL for current process
	strupr(szModuleFileName);
	iSize = strlen(szModuleFileName);

	// Check for specific EXEs and adjust global settings accordingly

	// NOTE: In GLD3.x "bDirectDrawPersistant" corresponds to IDirect3D8 and
	//       "bPersistantBuffers" corresponds to IDirect3DDevice8. KeithH

	// Case 1: 3DStudio must be multi-threaded
	// Added: Discreet GMAX (3DStudio MAX 4 for gamers. KeithH)
	if (strstr(szModuleFileName, "3DSMAX.EXE")
		|| strstr(szModuleFileName, "3DSVIZ.EXE")
		|| strstr(szModuleFileName, "GMAX.EXE")) {
		glb.bMultiThreaded = TRUE;
		glb.bDirectDrawPersistant = FALSE;
		glb.bPersistantBuffers = FALSE;
		return;
	}

	// Case 2: Solid Edge must use pre-allocated resources for all GLRCs
	if (strstr(szModuleFileName, "PART.EXE")
		|| strstr(szModuleFileName, "ASSEMBL.EXE")
		|| strstr(szModuleFileName, "DRAFT.EXE")
		|| strstr(szModuleFileName, "SMARTVW.EXE")
		|| strstr(szModuleFileName, "SMETAL.EXE")) {
		glb.bMultiThreaded = FALSE;
		glb.bDirectDrawPersistant = TRUE;
		glb.bPersistantBuffers = FALSE;
		return;
	}

	// Case 3: Sudden Depth creates and destroys GLRCs on paint commands
	if (strstr(szModuleFileName, "SUDDEPTH.EXE")
		|| strstr(szModuleFileName, "SUDDEMO.EXE")) {
		glb.bMultiThreaded = FALSE;
		glb.bDirectDrawPersistant = TRUE;
		glb.bPersistantBuffers = TRUE;
		glb.bFullscreenBlit = TRUE;
		return;
	}

	// Case 4: StereoGraphics test apps create and destroy GLRCs on paint commands
	if (strstr(szModuleFileName, "REDBLUE.EXE")
		|| strstr(szModuleFileName, "DIAGNOSE.EXE")) {
		glb.bMultiThreaded = FALSE;
		glb.bDirectDrawPersistant = TRUE;
		glb.bPersistantBuffers = TRUE;
		return;
	}

	// Case 5: Pipes screen savers share multiple GLRCs for same window
	if (strstr(szModuleFileName, "PIPES.SCR")
		|| (strstr(szModuleFileName, "PIPES") && strstr(szModuleFileName, ".SCR"))) {
		glb.bMultiThreaded = FALSE;
		glb.bDirectDrawPersistant = TRUE;
		glb.bPersistantBuffers = TRUE;
		return;
	}

	// Case 6: AutoVue uses sub-viewport ops which are temporarily broken in stereo window
	if (strstr(szModuleFileName, "AVWIN.EXE")) {
		glb.bMultiThreaded = FALSE;
		glb.bDirectDrawPersistant = TRUE;
		glb.bPersistantBuffers = TRUE;
		return;
	}
	// Case 7: Quake3 is waiting for DDraw objects to be released at exit
	if (strstr(szModuleFileName, "QUAKE")) {
		glb.bMultiThreaded = FALSE;
		glb.bDirectDrawPersistant = FALSE;
		glb.bPersistantBuffers = FALSE;
        glb.bFullscreenBlit = FALSE;
		return;
	}
	// Case 8: Reflection GLX server is unable to switch contexts at run-time
	if (strstr(szModuleFileName, "RX.EXE")) {
		glb.bMultiThreaded = FALSE;
        glb.bMessageBoxWarnings = FALSE;
		return;
	}
	// Case 9: Original AutoCAD 2000 must share DDraw objects across GLRCs
	if (strstr(szModuleFileName, "ACAD.EXE")) {
		glb.bFastFPU = FALSE;
        if (GetModuleHandle("wopengl6.hdi") != NULL) {
		glb.bMultiThreaded = FALSE;
		glb.bDirectDrawPersistant = TRUE;
		glb.bPersistantBuffers = FALSE;
		}
		return;
	}
}

// ***********************************************************************

BOOL dglInitDriver(void)
{
	UCHAR szExeName[MAX_PATH];
	const char *szRendering[] = {
		"Mesa Software",
		"Direct3D RGB SW",
		"Direct3D HW",
	};
    static BOOL bWarnOnce = FALSE;

    // Already initialized?
    if (bInitialized)
        return TRUE;

    // Moved from DllMain DLL_PROCESS_ATTACH:

		// (Re-)Init defaults
		dglInitGlobals();

		// Read registry or INI file settings
		if (!dllReadRegistry(hInstanceDll)) {
            if (!bWarnOnce)
			    MessageBox( NULL, "GLDirect has not been configured.\n\n"
							  "Please run the configuration program\n"
                              "before using GLDirect with applications.\n",
							  "GLDirect", MB_OK | MB_ICONWARNING);
            bWarnOnce = TRUE;
            return FALSE;
		}

#ifdef _USE_GLD3_WGL
		// Must do this as early as possible.
		// Need to read regkeys/ini-file first though.
		gldInitDriverPointers(glb.dwDriver);

		// Create private driver globals
		_gldDriver.CreatePrivateGlobals();
#endif
		// Overide settings with application customizations
		if (glb.bAppCustomizations)
			dglSetAppCustomizations();

//#ifndef _USE_GLD3_WGL
		// Set the global memory type to either sysmem or vidmem
		glb.dwMemoryType = glb.bHardware ? DDSCAPS_VIDEOMEMORY : DDSCAPS_SYSTEMMEMORY;
//#endif

		// Multi-threaded support overides persistant display support
		if (glb.bMultiThreaded)
			glb.bDirectDrawPersistant = glb.bPersistantBuffers = FALSE;

        // Multi-threaded support needs to be reflected in Mesa code. (DaveM)
        _gld_bMultiThreaded = glb.bMultiThreaded;

		// Start logging
        ddlogPathOption(szLogPath);
		ddlogWarnOption(glb.bMessageBoxWarnings);
		ddlogOpen((DDLOG_loggingMethodType)dwLogging,
				  (DDLOG_severityType)dwDebugLevel);

		// Obtain the name of the calling app
		ddlogMessage(DDLOG_SYSTEM, "Driver           : SciTech GLDirect 4.0\n");
		GetModuleFileName(NULL, szExeName, sizeof(szExeName));
		ddlogPrintf(DDLOG_SYSTEM, "Executable       : %s", szExeName);

		ddlogPrintf(DDLOG_SYSTEM, "DirectDraw device: %s", glb.szDDName);
		ddlogPrintf(DDLOG_SYSTEM, "Direct3D driver  : %s", glb.szD3DName);

		ddlogPrintf(DDLOG_SYSTEM, "Rendering type   : %s", szRendering[glb.dwRendering]);

		ddlogPrintf(DDLOG_SYSTEM, "Multithreaded    : %s", glb.bMultiThreaded ? "Enabled" : "Disabled");
		ddlogPrintf(DDLOG_SYSTEM, "Display resources: %s", glb.bDirectDrawPersistant ? "Persistant" : "Instanced");
		ddlogPrintf(DDLOG_SYSTEM, "Buffer resources : %s", glb.bPersistantBuffers ? "Persistant" : "Instanced");

		dglInitContextState();
		dglBuildPixelFormatList();
		//dglBuildTextureFormatList();

    // D3D callback driver is now successfully initialized
    bInitialized = TRUE;
    // D3D callback driver is now ready to be exited
    bExited = FALSE;

    return TRUE;
}

// ***********************************************************************

void dglExitDriver(void)
{

	// Only need to clean up once per instance:
	// May be called implicitly from DLL_PROCESS_DETACH,
	// or explicitly from DGL_exitDriver().
	if (bExited)
		return;
	bExited = TRUE;

    // DDraw objects may be invalid when DLL unloads.
__try {

	// Clean-up sequence (moved from DLL_PROCESS_DETACH)
#ifndef _USE_GLD3_WGL
	dglReleaseTextureFormatList();
#endif
	dglReleasePixelFormatList();
	dglDeleteContextState();

#ifdef _USE_GLD3_WGL
	_gldDriver.DestroyPrivateGlobals();
#endif

}
__except(EXCEPTION_EXECUTE_HANDLER) {
	    ddlogPrintf(DDLOG_WARN, "Exception raised in dglExitDriver.");
}

	// Close the log file
	ddlogClose();
}

// ***********************************************************************

int WINAPI DllMain(
	HINSTANCE hInstance,
	DWORD fdwReason,
	PVOID pvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
        // Cache DLL instance handle
        hInstanceDll = hInstance;

        // Flag that callback driver has yet to be initialized
        bInitialized = bExited = FALSE;

#ifndef _USE_GLD3_WGL
        // Init internal Mesa function pointers
		memset(&mesaFuncs, 0, sizeof(DGL_mesaFuncs));
#endif // _USE_GLD3_WGL

		// Init defaults
		dglInitGlobals();

        // Defer rest of DLL initialization to 1st WGL function call
		break;

	case DLL_PROCESS_DETACH:
		// Call exit clean-up sequence
		dglExitDriver();
		break;
	}

	return TRUE;
}

// ***********************************************************************

void APIENTRY DGL_exitDriver(void)
{
	// Call exit clean-up sequence
	dglExitDriver();
}

// ***********************************************************************

void APIENTRY DGL_reinitDriver(void)
{
	// Force init sequence again
    bInitialized = bExited = FALSE;
	dglInitDriver();
}

// ***********************************************************************

int WINAPI DllInitialize(
	HINSTANCE hInstance,
	DWORD fdwReason,
	PVOID pvReserved)
{
	// Some Watcom compiled executables require this.
	return DllMain(hInstance, fdwReason, pvReserved);
}

// ***********************************************************************

void DGL_LoadSplashScreen(int piReg, char* pszUser)
{
	HINSTANCE			hSplashDll = NULL;
	LPDGLSPLASHSCREEN 	dglSplashScreen = NULL;
	static BOOL 		bOnce = FALSE;
    static int          iReg = 0;
    static char         szUser[255] = {"\0"};

    // Display splash screen at all?
    if (!bSplashScreen)
        return;

	// Only display splash screen once
	if (bOnce)
		return;
	bOnce = TRUE;

    // Make local copy of string for passing to DLL
    if (pszUser)
        strcpy(szUser, pszUser);
    iReg = piReg;

	// Load Splash Screen DLL
	// (If it fails to load for any reason, we don't care...)
	hSplashDll = LoadLibrary("gldsplash.dll");
	if (hSplashDll) {
		// Execute the Splash Screen function
		dglSplashScreen = (LPDGLSPLASHSCREEN)GetProcAddress(hSplashDll, "GLDSplashScreen");
		if (dglSplashScreen)
			(*dglSplashScreen)(1, iReg, szUser);
		// Don't unload the DLL since splash screen dialog is modeless now
		}
}

// ***********************************************************************

BOOL dglValidate()
{
	char *szCaption = "SciTech GLDirect Driver";
	UINT uType = MB_OK | MB_ICONEXCLAMATION;

#ifdef _USE_GLD3_WGL
	// (Re)build pixelformat list
	if (glb.bPixelformatsDirty)
		_gldDriver.BuildPixelformatList();
#endif

	// Check to see if we have already validated
	if (bDriverValidated && bInitialized)
		return TRUE;

    // Since all (most) the WGL functions must be validated at this point,
    // this also insure that the callback driver is completely initialized.
    if (!bInitialized)
        if (!dglInitDriver()) {
			MessageBox(NULL,
				"The GLDirect driver could not initialize.\n\n"
				"Please run the configuration program to\n"
				"properly configure the driver, or else\n"
                "re-run the installation program.", szCaption, uType);
			_exit(1); // Bail
        }

    return TRUE;
}

// ***********************************************************************

