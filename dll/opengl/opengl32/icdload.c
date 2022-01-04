/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/icdload.c
 * PURPOSE:              OpenGL32 lib, ICD dll loader
 */

#include "opengl32.h"

#include <winreg.h>

WINE_DEFAULT_DEBUG_CHANNEL(opengl32);

typedef struct
{
    DWORD Version;          /*!< Driver interface version */
    DWORD DriverVersion;    /*!< Driver version */
    WCHAR DriverName[256];  /*!< Driver name */
} Drv_Opengl_Info, *pDrv_Opengl_Info;

typedef enum
{
    OGL_CD_NOT_QUERIED,
    OGL_CD_NONE,
    OGL_CD_ROSSWI,
    OGL_CD_CUSTOM_ICD
} CUSTOM_DRIVER_STATE;

static CRITICAL_SECTION icdload_cs = {NULL, -1, 0, 0, 0, 0};
static struct ICD_Data* ICD_Data_List = NULL;
static const WCHAR OpenGLDrivers_Key[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers";
static const WCHAR CustomDrivers_Key[] = L"SOFTWARE\\ReactOS\\OpenGL";
static Drv_Opengl_Info CustomDrvInfo;
static CUSTOM_DRIVER_STATE CustomDriverState = OGL_CD_NOT_QUERIED;

static void APIENTRY wglSetCurrentValue(PVOID value)
{
    IntSetCurrentICDPrivate(value);
}

static PVOID APIENTRY wglGetCurrentValue()
{
    return IntGetCurrentICDPrivate();
}

static DHGLRC APIENTRY wglGetDHGLRC(struct wgl_context* context)
{
    return context->dhglrc;
}

/* GDI entry points (win32k) */
extern INT APIENTRY GdiDescribePixelFormat(HDC hdc, INT ipfd, UINT cjpfd, PPIXELFORMATDESCRIPTOR ppfd);
extern BOOL APIENTRY GdiSetPixelFormat(HDC hdc, INT ipfd);
extern BOOL APIENTRY GdiSwapBuffers(HDC hdc);

/* Retrieves the ICD data (driver version + relevant DLL entry points) for a device context */
struct ICD_Data* IntGetIcdData(HDC hdc)
{
    int ret;
    DWORD dwInput, dwValueType, Version, DriverVersion, Flags;
    Drv_Opengl_Info DrvInfo;
    pDrv_Opengl_Info pDrvInfo;
    struct ICD_Data* data;
    HKEY OglKey = NULL;
    HKEY DrvKey, CustomKey;
    WCHAR DllName[MAX_PATH];
    BOOL (WINAPI *DrvValidateVersion)(DWORD);
    void (WINAPI *DrvSetCallbackProcs)(int nProcs, PROC* pProcs);

    /* The following code is ReactOS specific and allows us to easily load an arbitrary ICD:
     * It checks HKCU\Software\ReactOS\OpenGL for a custom ICD and will always load it
     * no matter what driver the DC is associated with. It can also force using the
     * built-in Software Implementation*/
    if(CustomDriverState == OGL_CD_NOT_QUERIED)
    {
        /* Only do this once so there's not any significant performance penalty */
        CustomDriverState = OGL_CD_NONE;
        memset(&CustomDrvInfo, 0, sizeof(Drv_Opengl_Info));

        ret = RegOpenKeyExW(HKEY_CURRENT_USER, CustomDrivers_Key, 0, KEY_READ, &CustomKey);
        if(ret != ERROR_SUCCESS)
            goto custom_end;

        dwInput = sizeof(CustomDrvInfo.DriverName);
        ret = RegQueryValueExW(CustomKey, L"", 0, &dwValueType, (LPBYTE)CustomDrvInfo.DriverName, &dwInput);
        RegCloseKey(CustomKey);

        if((ret != ERROR_SUCCESS) || (dwValueType != REG_SZ) || !wcslen(CustomDrvInfo.DriverName))
            goto custom_end;

        if(!_wcsicmp(CustomDrvInfo.DriverName, L"ReactOS Software Implementation"))
        {
            /* Always announce the fact that we're forcing ROSSWI */
            ERR("Forcing ReactOS Software Implementation\n");
            CustomDriverState = OGL_CD_ROSSWI;
            return NULL;
        }

        ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, OpenGLDrivers_Key, 0, KEY_READ, &OglKey);
        if(ret != ERROR_SUCCESS)
            goto custom_end;

        ret = RegOpenKeyExW(OglKey, CustomDrvInfo.DriverName, 0, KEY_READ, &OglKey);
        if(ret != ERROR_SUCCESS)
            goto custom_end;

        dwInput = sizeof(CustomDrvInfo.Version);
        ret = RegQueryValueExW(OglKey, L"Version", 0, &dwValueType, (LPBYTE)&CustomDrvInfo.Version, &dwInput);
        if((ret != ERROR_SUCCESS) || (dwValueType != REG_DWORD))
            goto custom_end;

        dwInput = sizeof(DriverVersion);
        ret = RegQueryValueExW(OglKey, L"DriverVersion", 0, &dwValueType, (LPBYTE)&CustomDrvInfo.DriverVersion, &dwInput);
        CustomDriverState = OGL_CD_CUSTOM_ICD;

        /* Always announce the fact that we're overriding the default driver */
        ERR("Overriding the default OGL ICD with %S\n", CustomDrvInfo.DriverName);

custom_end:
        if(OglKey)
            RegCloseKey(OglKey);
        RegCloseKey(CustomKey);
    }

    /* If there's a custom ICD or ROSSWI was requested use it, otherwise proceed as usual */
    if(CustomDriverState == OGL_CD_CUSTOM_ICD)
    {
        pDrvInfo = &CustomDrvInfo;
    }
    else if(CustomDriverState == OGL_CD_ROSSWI)
    {
        return NULL;
    }
    else
    {
        /* First, see if the driver supports this */
        dwInput = OPENGL_GETINFO;
        ret = ExtEscape(hdc, QUERYESCSUPPORT, sizeof(DWORD), (LPCSTR)&dwInput, 0, NULL);

        /* Driver doesn't support opengl */
        if(ret <= 0)
            return NULL;

        /* Query for the ICD DLL name and version */
        dwInput = 0;
        ret = ExtEscape(hdc, OPENGL_GETINFO, sizeof(DWORD), (LPCSTR)&dwInput, sizeof(DrvInfo), (LPSTR)&DrvInfo);

        if(ret <= 0)
        {
            ERR("Driver claims to support OPENGL_GETINFO escape code, but doesn't.\n");
            return NULL;
        }

        pDrvInfo = &DrvInfo;
    }

    /* Protect the list while we are loading*/
    EnterCriticalSection(&icdload_cs);

    /* Search for it in the list of already loaded modules */
    data = ICD_Data_List;
    while(data)
    {
        if(!_wcsicmp(data->DriverName, pDrvInfo->DriverName))
        {
            /* Found it */
            TRACE("Found already loaded %p.\n", data);
            LeaveCriticalSection(&icdload_cs);
            return data;
        }
        data = data->next;
    }

    /* It was still not loaded, look for it in the registry */
    ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, OpenGLDrivers_Key, 0, KEY_READ, &OglKey);
    if(ret != ERROR_SUCCESS)
    {
        ERR("Failed to open the OpenGLDrivers key.\n");
        goto end;
    }
    ret = RegOpenKeyExW(OglKey, pDrvInfo->DriverName, 0, KEY_READ, &DrvKey);
    if(ret != ERROR_SUCCESS)
    {
        /* Some driver installer just provide the DLL name, like the Matrox G400 */
        TRACE("No driver subkey for %S, trying to get DLL name directly.\n", pDrvInfo->DriverName);
        dwInput = sizeof(DllName);
        ret = RegQueryValueExW(OglKey, pDrvInfo->DriverName, 0, &dwValueType, (LPBYTE)DllName, &dwInput);
        if((ret != ERROR_SUCCESS) || (dwValueType != REG_SZ))
        {
            ERR("Unable to get ICD DLL name!\n");
            RegCloseKey(OglKey);
            goto end;
        }
        Version = DriverVersion = Flags = 0;
        TRACE("DLL name is %S.\n", DllName);
    }
    else
    {
        /* The driver have a subkey for the ICD */
        TRACE("Querying details from registry for %S.\n", pDrvInfo->DriverName);
        dwInput = sizeof(DllName);
        ret = RegQueryValueExW(DrvKey, L"Dll", 0, &dwValueType, (LPBYTE)DllName, &dwInput);
        if((ret != ERROR_SUCCESS) || (dwValueType != REG_SZ))
        {
            ERR("Unable to get ICD DLL name!.\n");
            RegCloseKey(DrvKey);
            RegCloseKey(OglKey);
            goto end;
        }

        dwInput = sizeof(Version);
        ret = RegQueryValueExW(DrvKey, L"Version", 0, &dwValueType, (LPBYTE)&Version, &dwInput);
        if((ret != ERROR_SUCCESS) || (dwValueType != REG_DWORD))
        {
            WARN("No version in driver subkey\n");
        }
        else if(Version != pDrvInfo->Version)
        {
            ERR("Version mismatch between registry (%lu) and display driver (%lu).\n", Version, pDrvInfo->Version);
            RegCloseKey(DrvKey);
            RegCloseKey(OglKey);
            goto end;
        }

        dwInput = sizeof(DriverVersion);
        ret = RegQueryValueExW(DrvKey, L"DriverVersion", 0, &dwValueType, (LPBYTE)&DriverVersion, &dwInput);
        if((ret != ERROR_SUCCESS) || (dwValueType != REG_DWORD))
        {
            WARN("No driver version in driver subkey\n");
        }
        else if(DriverVersion != pDrvInfo->DriverVersion)
        {
            ERR("Driver version mismatch between registry (%lu) and display driver (%lu).\n", DriverVersion, pDrvInfo->DriverVersion);
            RegCloseKey(DrvKey);
            RegCloseKey(OglKey);
            goto end;
        }

        dwInput = sizeof(Flags);
        ret = RegQueryValueExW(DrvKey, L"Flags", 0, &dwValueType, (LPBYTE)&Flags, &dwInput);
        if((ret != ERROR_SUCCESS) || (dwValueType != REG_DWORD))
        {
            WARN("No driver version in driver subkey\n");
            Flags = 0;
        }

        /* We're done */
        RegCloseKey(DrvKey);
        TRACE("DLL name is %S, Version %lx, DriverVersion %lx, Flags %lx.\n", DllName, Version, DriverVersion, Flags);
    }
    /* No need for this anymore */
    RegCloseKey(OglKey);

    /* So far so good, allocate data */
    data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data));
    if(!data)
    {
        ERR("Unable to allocate ICD data!\n");
        goto end;
    }

    /* Load the library */
    data->hModule = LoadLibraryW(DllName);
    if(!data->hModule)
    {
        ERR("Could not load the ICD DLL: %S.\n", DllName);
        HeapFree(GetProcessHeap(), 0, data);
        data = NULL;
        goto end;
    }

    /*
     * Validate version, if needed.
     * Some drivers (at least VBOX), initialize stuff upon this call.
     */
    DrvValidateVersion = (void*)GetProcAddress(data->hModule, "DrvValidateVersion");
    if(DrvValidateVersion)
    {
        if(!DrvValidateVersion(pDrvInfo->DriverVersion))
        {
            ERR("DrvValidateVersion failed!.\n");
            goto fail;
        }
    }

    /* Pass the callbacks */
    DrvSetCallbackProcs = (void*)GetProcAddress(data->hModule, "DrvSetCallbackProcs");
    if(DrvSetCallbackProcs)
    {
        PROC callbacks[] = {
            (PROC)wglSetCurrentValue,
            (PROC)wglGetCurrentValue,
            (PROC)wglGetDHGLRC};
        DrvSetCallbackProcs(ARRAYSIZE(callbacks), callbacks);
    }

    /* Get the DLL exports */
#define DRV_LOAD(x) do                                  \
{                                                       \
    data->x = (void*)GetProcAddress(data->hModule, #x); \
    if(!data->x) {                                      \
        ERR("%S lacks " #x "!\n", DllName);             \
        goto fail;                                      \
    }                                                   \
} while(0)
    DRV_LOAD(DrvCopyContext);
    DRV_LOAD(DrvCreateContext);
    DRV_LOAD(DrvCreateLayerContext);
    DRV_LOAD(DrvDeleteContext);
    DRV_LOAD(DrvDescribeLayerPlane);
    DRV_LOAD(DrvDescribePixelFormat);
    DRV_LOAD(DrvGetLayerPaletteEntries);
    DRV_LOAD(DrvGetProcAddress);
    DRV_LOAD(DrvReleaseContext);
    DRV_LOAD(DrvRealizeLayerPalette);
    DRV_LOAD(DrvSetContext);
    DRV_LOAD(DrvSetLayerPaletteEntries);
    DRV_LOAD(DrvSetPixelFormat);
    DRV_LOAD(DrvShareLists);
    DRV_LOAD(DrvSwapBuffers);
    DRV_LOAD(DrvSwapLayerBuffers);
#undef DRV_LOAD

    /* Let's see if GDI should handle this instead of the ICD DLL */
    // FIXME: maybe there is a better way
    if (GdiDescribePixelFormat(hdc, 0, 0, NULL) != 0)
    {
        /* GDI knows what to do with that. Override */
        TRACE("Forwarding WGL calls to win32k!\n");
        data->DrvDescribePixelFormat = GdiDescribePixelFormat;
        data->DrvSetPixelFormat = GdiSetPixelFormat;
        data->DrvSwapBuffers = GdiSwapBuffers;
    }

    /* Copy the DriverName */
    wcscpy(data->DriverName, pDrvInfo->DriverName);

    /* Push the list */
    data->next = ICD_Data_List;
    ICD_Data_List = data;

    TRACE("Returning %p.\n", data);
    TRACE("ICD driver %S (%S) successfully loaded.\n", pDrvInfo->DriverName, DllName);

end:
    /* Unlock and return */
    LeaveCriticalSection(&icdload_cs);
    return data;

fail:
    LeaveCriticalSection(&icdload_cs);
    FreeLibrary(data->hModule);
    HeapFree(GetProcessHeap(), 0, data);
    return NULL;
}

void IntDeleteAllICDs(void)
{
    struct ICD_Data* data;

    EnterCriticalSection(&icdload_cs);

    while (ICD_Data_List != NULL)
    {
        data = ICD_Data_List;
        ICD_Data_List = data->next;

        FreeLibrary(data->hModule);
        HeapFree(GetProcessHeap(), 0, data);
    }
}
