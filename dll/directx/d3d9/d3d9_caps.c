#include <d3d9.h>
#include <ddraw.h>
#include <d3dnthal.h>
#include <d3dhal.h>
#include <ddrawi.h>
#include <ddrawgdi.h>
#include <dll/directx/d3d8thk.h>
#include <debug.h>
#include <limits.h>
#include "d3d9_helpers.h"
#include "d3d9_caps.h"
#include "adapter.h"
#include "d3d9_callbacks.h"

static INT g_NumDevices = 0;

void CreateDisplayModeList(LPCSTR lpszDeviceName, D3DDISPLAYMODE* pDisplayModes, DWORD* pNumDisplayModes, D3DFORMAT Default16BitFormat, D3D9_Unknown6BC* pUnknown6BC)
{
    DEVMODEA DevMode;
    DWORD ModeIndex = 0;
    DWORD ValidModes = 0;

    while (TRUE == EnumDisplaySettingsA(lpszDeviceName, ModeIndex, &DevMode))
    {
        D3DFORMAT DefaultFormat;

        if (DevMode.dmBitsPerPel != 15 &&
            DevMode.dmBitsPerPel != 16 &&
            DevMode.dmBitsPerPel != 32)
        {
            ++ModeIndex;
            continue;
        }

        ++ValidModes;

        if (DevMode.dmBitsPerPel == 15 || DevMode.dmBitsPerPel == 16)
        {
            if (NULL == pUnknown6BC)
            {
                ++ModeIndex;
                continue;
            }

            DefaultFormat = Default16BitFormat;
        }
        else
        {
            DefaultFormat = D3DFMT_X8R8G8B8;
        }

        if (NULL != pDisplayModes)
        {
            if (ValidModes == *pNumDisplayModes)
                break;

            pDisplayModes->Width = DevMode.dmPelsWidth;
            pDisplayModes->Height = DevMode.dmPelsHeight;
            pDisplayModes->RefreshRate = DevMode.dmDisplayFrequency;
            pDisplayModes->Format = DefaultFormat;
            ++pDisplayModes;
        }

        ++ModeIndex;
    }

    *pNumDisplayModes = ValidModes;
}

static void CreateInternalDeviceData(HDC hDC, LPCSTR lpszDeviceName, D3D9_Unknown6BC** ppUnknown, D3DDEVTYPE DeviceType, HMODULE* hD3DRefDll)
{
    D3D9_Unknown6BC* pUnknown6BC;
    DWORD ValueSize;

    if (ppUnknown) *ppUnknown = NULL;
    if (hD3DRefDll) *hD3DRefDll = NULL;

    if (DeviceType != D3DDEVTYPE_HAL)
    {
        /* TODO: Implement D3DDEVTYPE_REF and D3DDEVTYPE_SW */
        UNIMPLEMENTED;
        return;
    }

    pUnknown6BC = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(D3D9_Unknown6BC));
    if (NULL == pUnknown6BC)
    {
        DPRINT1("Out of memory");
        return;
    }

    pUnknown6BC->hDirectDrawLocal = OsThunkDdCreateDirectDrawObject(hDC);
    if (0 == pUnknown6BC->hDirectDrawLocal)
    {
        HeapFree(GetProcessHeap(), 0, pUnknown6BC);
        return;
    }


    SafeCopyString(pUnknown6BC->szDeviceName, CCHDEVICENAME, lpszDeviceName);
    //pUnknown6BC->DeviceUniq = DdQueryDisplaySettingsUniqueness();
    pUnknown6BC->DeviceType = DeviceType;


    ValueSize = sizeof(DWORD);
    ReadRegistryValue(REG_DWORD, "ForceDriverFlagsOn", (LPBYTE)&pUnknown6BC->bForceDriverFlagsOn, &ValueSize);

    ValueSize = sizeof(DWORD);
    ReadRegistryValue(REG_DWORD, "ForceDriverFlagsOff", (LPBYTE)&pUnknown6BC->bForceDriverFlagsOff, &ValueSize);

    ++g_NumDevices;

    *ppUnknown = pUnknown6BC;
}

static void ReleaseInternalDeviceData(LPD3D9_DEVICEDATA pDeviceData)
{
    OsThunkDdDeleteDirectDrawObject(pDeviceData->pUnknown6BC->hDirectDrawLocal);

    HeapFree(GetProcessHeap(), 0, pDeviceData->pUnknown6BC);
    pDeviceData->pUnknown6BC = NULL;

    --g_NumDevices;
}

BOOL GetDeviceData(LPD3D9_DEVICEDATA pDeviceData)
{
    BOOL bRet;
    D3DHAL_GLOBALDRIVERDATA GlobalDriverData;
    D3DHAL_D3DEXTENDEDCAPS D3dExtendedCaps;
    LPDDSURFACEDESC puD3dTextureFormats;
    DDPIXELFORMAT* pD3dZStencilFormatList;
    D3DDISPLAYMODE* pD3dDisplayModeList;
    D3DQUERYTYPE* pD3dQueryList;
    DWORD NumTextureFormats = 0;
    DWORD NumStencilFormats = 0;
    DWORD NumExtendedFormats = 0;
    DWORD NumQueries = 0;

    if (NULL == pDeviceData->pUnknown6BC)
    {
        CreateInternalDeviceData(
            pDeviceData->hDC,
            pDeviceData->szDeviceName,
            &pDeviceData->pUnknown6BC,
            pDeviceData->DeviceType,
            &pDeviceData->hD3DRefDll
            );

        if (NULL == pDeviceData->pUnknown6BC)
        {
            DPRINT1("Failed to create DirectDrawObject for Direct3D9");
            return FALSE;
        }
    }
    else
    {
        D3D9_DRIVERCAPS DriverCaps;
        D3D9_CALLBACKS D3D9Callbacks;

        if (FALSE == CanReenableDirectDrawObject(pDeviceData->pUnknown6BC))
        {
            DPRINT1("Failed to re-enable DirectDrawObject");
            return FALSE;
        }

        bRet = GetD3D9DriverInfo(
            pDeviceData->pUnknown6BC,
            &DriverCaps,
            &D3D9Callbacks,
            pDeviceData->szDeviceName,
            pDeviceData->hD3DRefDll,
            &GlobalDriverData,
            &D3dExtendedCaps,
            NULL,
            NULL,
            NULL,
            NULL,
            &NumTextureFormats,
            &NumStencilFormats,
            &NumExtendedFormats,
            &NumQueries
            );

        if (TRUE == bRet)
        {
            pDeviceData->DriverCaps.dwDisplayWidth = DriverCaps.dwDisplayWidth;
            pDeviceData->DriverCaps.dwDisplayHeight = DriverCaps.dwDisplayHeight;
            pDeviceData->DriverCaps.RawDisplayFormat = DriverCaps.RawDisplayFormat;
            pDeviceData->DriverCaps.DisplayFormat = DriverCaps.DisplayFormat;
            pDeviceData->DriverCaps.dwRefreshRate = DriverCaps.dwRefreshRate;
        }

        return bRet;
    }

    /* Cleanup of old stuff */
    if (pDeviceData->DriverCaps.pSupportedFormatOps)
    {
        HeapFree(GetProcessHeap(), 0, pDeviceData->DriverCaps.pSupportedFormatOps);
        pDeviceData->DriverCaps.pSupportedFormatOps = NULL;
    }
    if (pDeviceData->DriverCaps.pSupportedExtendedModes)
    {
        HeapFree(GetProcessHeap(), 0, pDeviceData->DriverCaps.pSupportedExtendedModes);
        pDeviceData->DriverCaps.pSupportedExtendedModes = NULL;
    }
    if (pDeviceData->DriverCaps.pSupportedQueriesList)
    {
        HeapFree(GetProcessHeap(), 0, pDeviceData->DriverCaps.pSupportedQueriesList);
        pDeviceData->DriverCaps.pSupportedQueriesList = NULL;
    }

    if (FALSE == CanReenableDirectDrawObject(pDeviceData->pUnknown6BC))
    {
        DPRINT1("Failed to re-enable DirectDrawObject");
        ReleaseInternalDeviceData(pDeviceData);
        return FALSE;
    }

    bRet = GetD3D9DriverInfo(
        pDeviceData->pUnknown6BC,
        &pDeviceData->DriverCaps,
        &pDeviceData->D3D9Callbacks,
        pDeviceData->szDeviceName,
        pDeviceData->hD3DRefDll,
        &GlobalDriverData,
        &D3dExtendedCaps,
        NULL,
        NULL,
        NULL,
        NULL,
        &NumTextureFormats,
        &NumStencilFormats,
        &NumExtendedFormats,
        &NumQueries
        );

    if (FALSE == bRet)
    {
        DPRINT1("Could not query DirectDrawObject, aborting");
        ReleaseInternalDeviceData(pDeviceData);
        return FALSE;
    }

    puD3dTextureFormats = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, max(NumTextureFormats, 1) * sizeof(DDSURFACEDESC));
    pD3dZStencilFormatList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, max(NumStencilFormats, 1) * sizeof(DDPIXELFORMAT));
    pD3dDisplayModeList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, max(NumExtendedFormats, 1) * sizeof(D3DDISPLAYMODE));
    pD3dQueryList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, max(NumQueries, 1) * sizeof(D3DQUERYTYPE));

    bRet = GetD3D9DriverInfo(
        pDeviceData->pUnknown6BC,
        &pDeviceData->DriverCaps,
        &pDeviceData->D3D9Callbacks,
        pDeviceData->szDeviceName,
        pDeviceData->hD3DRefDll,
        &GlobalDriverData,
        &D3dExtendedCaps,
        puD3dTextureFormats,
        pD3dZStencilFormatList,
        pD3dDisplayModeList,
        pD3dQueryList,
        &NumTextureFormats,
        &NumStencilFormats,
        &NumExtendedFormats,
        &NumQueries
        );

    if (FALSE == bRet)
    {
        DPRINT1("Could not query DirectDrawObject, aborting");
        HeapFree(GetProcessHeap(), 0, puD3dTextureFormats);
        HeapFree(GetProcessHeap(), 0, pD3dZStencilFormatList);
        HeapFree(GetProcessHeap(), 0, pD3dDisplayModeList);
        HeapFree(GetProcessHeap(), 0, pD3dQueryList);
        ReleaseInternalDeviceData(pDeviceData);
        return FALSE;
    }

    pDeviceData->DriverCaps.NumSupportedFormatOps = NumTextureFormats;
    if (NumTextureFormats > 0)
        pDeviceData->DriverCaps.pSupportedFormatOps = puD3dTextureFormats;

    pDeviceData->DriverCaps.NumSupportedExtendedModes = NumExtendedFormats;
    if (NumExtendedFormats > 0)
        pDeviceData->DriverCaps.pSupportedExtendedModes = pD3dDisplayModeList;

    pDeviceData->DriverCaps.NumSupportedQueries = NumQueries;
    if (NumQueries > 0)
        pDeviceData->DriverCaps.pSupportedQueriesList = pD3dQueryList;

    HeapFree(GetProcessHeap(), 0, pD3dZStencilFormatList);

    return TRUE;
}



BOOL CanReenableDirectDrawObject(D3D9_Unknown6BC* pUnknown)
{
    BOOL bDisplayModeWasChanged;

    /* Try the real way first */
    if (TRUE == OsThunkDdReenableDirectDrawObject(pUnknown->hDirectDrawLocal, &bDisplayModeWasChanged))
        return TRUE;

    /* Ref types and software types can always be reenabled after a mode switch */
    if (pUnknown->DeviceType == D3DDEVTYPE_REF || pUnknown->DeviceType == D3DDEVTYPE_SW)
        return TRUE;

    return FALSE;
}



static void PrepareDriverInfoData(DD_GETDRIVERINFODATA* DrvInfo, LPVOID pData, DWORD dwExpectedSize)
{
    memset(DrvInfo, 0, sizeof(DD_GETDRIVERINFODATA));
    DrvInfo->dwSize = sizeof(DD_GETDRIVERINFODATA);
    DrvInfo->guidInfo = GUID_GetDriverInfo2;
    DrvInfo->dwExpectedSize = dwExpectedSize;
    DrvInfo->lpvData = pData;
    DrvInfo->ddRVal = E_FAIL;
}

static void ResetGetDriverInfo2Data(DD_GETDRIVERINFO2DATA* DrvInfo2, DWORD dwType, DWORD dwExpectedSize)
{
    memset(DrvInfo2, 0, dwExpectedSize);
    DrvInfo2->dwMagic = D3DGDI2_MAGIC;
    DrvInfo2->dwType = dwType;
    DrvInfo2->dwExpectedSize = dwExpectedSize;
}

BOOL GetD3D9DriverInfo( D3D9_Unknown6BC* pUnknown6BC,
                        LPD3D9_DRIVERCAPS pDriverCaps,
                        D3D9_CALLBACKS* pD3D9Callbacks,
                        LPCSTR lpszDeviceName,
                        HMODULE hD3dRefDll,
                        D3DHAL_GLOBALDRIVERDATA* pGblDriverData,
                        D3DHAL_D3DEXTENDEDCAPS* pD3dExtendedCaps,
                        LPDDSURFACEDESC puD3dTextureFormats,
                        DDPIXELFORMAT* pD3dZStencilFormatList,
                        D3DDISPLAYMODE* pD3dDisplayModeList,
                        D3DQUERYTYPE* pD3dQueryList,
                        LPDWORD pNumTextureFormats,
                        LPDWORD pNumZStencilFormats,
                        LPDWORD pNumExtendedFormats,
                        LPDWORD pNumQueries)
{
    BOOL bRet;
    DWORD ValueSize;
    DWORD dwDXVersion;

    DD_HALINFO HalInfo;
    DWORD CallBackFlags[3];
    D3DNTHAL_CALLBACKS D3dCallbacks;
    D3DNTHAL_GLOBALDRIVERDATA D3dDriverData;
    DD_D3DBUFCALLBACKS D3dBufferCallbacks;
    DWORD NumHeaps = 0;
    DWORD NumFourCC = 0;

    BOOL bDX8Mode = FALSE;

    DD_GETDRIVERINFODATA DrvInfo;
    DD_GETDDIVERSIONDATA DdiVersion;
    DD_GETFORMATCOUNTDATA FormatCountData;
    DD_GETEXTENDEDMODECOUNTDATA ExModeCountData;
    DD_GETD3DQUERYCOUNTDATA D3dQueryCountData;

    /* Init */
    *pNumTextureFormats = 0;
    *pNumZStencilFormats = 0;
    *pNumExtendedFormats = 0;
    *pNumQueries = 0;
    memset(pD3dExtendedCaps, 0, sizeof(D3DHAL_D3DEXTENDEDCAPS));
    memset(pGblDriverData, 0, sizeof(D3DHAL_GLOBALDRIVERDATA));
    memset(pDriverCaps, 0, sizeof(D3D9_DRIVERCAPS));

    /* Set runtime version */
    ValueSize = sizeof(dwDXVersion);
    if (FALSE == ReadRegistryValue(REG_DWORD, "DD_RUNTIME_VERSION", (LPBYTE)&dwDXVersion, &ValueSize))
        dwDXVersion = DD_RUNTIME_VERSION;


    bRet = OsThunkDdQueryDirectDrawObject(
        pUnknown6BC->hDirectDrawLocal,
        &HalInfo,
        CallBackFlags,
        &D3dCallbacks,
        &D3dDriverData,
        &D3dBufferCallbacks,
        NULL,
        &NumHeaps,
        NULL,
        &NumFourCC,
        NULL
        );

    if (bRet == FALSE)
    {
        /* TODO: Handle error */
        return FALSE;
    }

    if ((HalInfo.ddCaps.dwSVBCaps2 & DDCAPS2_AUTOFLIPOVERLAY) == 0 &&
        puD3dTextureFormats != NULL)
    {
        bRet = OsThunkDdQueryDirectDrawObject(
            pUnknown6BC->hDirectDrawLocal,
            &HalInfo,
            CallBackFlags,
            &D3dCallbacks,
            &D3dDriverData,
            &D3dBufferCallbacks,
            puD3dTextureFormats,
            &NumHeaps,
            NULL,
            &NumFourCC,
            NULL
            );

        if (FALSE == bRet)
            return FALSE;
    }

    if (NULL == pUnknown6BC->swDDICreateDirectDrawObject)
    {
        *pNumTextureFormats = D3dDriverData.dwNumTextureFormats;
    }

    pDriverCaps->DriverCaps9.Caps = HalInfo.ddCaps.dwCaps;
    pDriverCaps->DriverCaps9.Caps2 = HalInfo.ddCaps.dwCaps2;
    pDriverCaps->DriverCaps9.Caps3 = HalInfo.ddCaps.dwSVCaps;
    pDriverCaps->dwSVBCaps = HalInfo.ddCaps.dwSVBCaps;
    pDriverCaps->dwVSBCaps = HalInfo.ddCaps.dwVSBCaps;
    pDriverCaps->dwSVBCaps2 = HalInfo.ddCaps.dwSVBCaps2;
    pUnknown6BC->lDisplayPitch = HalInfo.vmiData.lDisplayPitch;

    if (HalInfo.dwFlags & DDHALINFO_GETDRIVERINFO2)
    {
        /* GUID_GetDriverInfo2 - Inform driver of DX version */
        {
            DD_DXVERSION DxVersion;

            ResetGetDriverInfo2Data(&DxVersion.gdi2, D3DGDI2_TYPE_DXVERSION, sizeof(DD_DXVERSION));
            DxVersion.dwDXVersion = dwDXVersion;

            PrepareDriverInfoData(&DrvInfo, &DxVersion, sizeof(DxVersion));
            OsThunkDdGetDriverInfo(pUnknown6BC->hDirectDrawLocal, &DrvInfo);
        }


        /* GUID_GetDriverInfo2 - Get DDI version */
        {
            ResetGetDriverInfo2Data(&DdiVersion.gdi2, D3DGDI2_TYPE_GETDDIVERSION, sizeof(DD_GETDDIVERSIONDATA));
            PrepareDriverInfoData(&DrvInfo, &DdiVersion, sizeof(DdiVersion));
            bRet = OsThunkDdGetDriverInfo(pUnknown6BC->hDirectDrawLocal, &DrvInfo);

            if (DdiVersion.dwDDIVersion != DX9_DDI_VERSION)
            {
                DWORD ForceDDIOn;

                ValueSize = sizeof(ForceDDIOn);
                if (TRUE == ReadRegistryValue(REG_DWORD, "ForceOldDDIOn", (LPBYTE)&ForceDDIOn, &ValueSize) &&
                    0 != ForceDDIOn)
                {
                    DdiVersion.dwDDIVersion = DX9_DDI_VERSION;
                }
            }
        }


        /* Check for errors to fallback to DX8 mode  */
        if (DdiVersion.dwDDIVersion < DX9_DDI_VERSION)
        {
            bDX8Mode = TRUE;

            if (DdiVersion.dwDDIVersion == 0)
            {
                DPRINT1("Driver claims to be DX9 driver, but didn't report DX9 DDI version - reverting to DX8 mode");
            }
            else
            {
                DPRINT1("Driver claims to be DX9 driver, but was built with an old DDI version - reverting to DX8 mode");
            }

            /* GUID_GetDriverInfo2 - Get D3DCAPS8 */
            {
                D3DCAPS8 DriverCaps8;

                ResetGetDriverInfo2Data((DD_GETDRIVERINFO2DATA*)&DriverCaps8, D3DGDI2_TYPE_GETD3DCAPS8, sizeof(D3DCAPS8));
                PrepareDriverInfoData(&DrvInfo, &DriverCaps8, sizeof(D3DCAPS8));

                if (FALSE == OsThunkDdGetDriverInfo(pUnknown6BC->hDirectDrawLocal, &DrvInfo) ||
                    S_OK != DrvInfo.ddRVal ||
                    DrvInfo.dwActualSize != sizeof(D3DCAPS8))
                {
                    DPRINT1("Driver returned an invalid D3DCAPS8 structure - aborting");
                    return FALSE;
                }

                memcpy(&pDriverCaps->DriverCaps9, &DriverCaps8, sizeof(D3DCAPS8));
                pDriverCaps->DriverCaps9.Caps = HalInfo.ddCaps.dwCaps;
                pDriverCaps->dwDriverCaps |= D3D9_INT_D3DCAPS8_VALID;
            }
        }


        /* GUID_GetDriverInfo2 - Get D3DCAPS9 */
        if (FALSE == bDX8Mode)
        {
            D3DCAPS9 DriverCaps9;

            ResetGetDriverInfo2Data((DD_GETDRIVERINFO2DATA*)&DriverCaps9, D3DGDI2_TYPE_GETD3DCAPS9, sizeof(D3DCAPS9));
            PrepareDriverInfoData(&DrvInfo, &DriverCaps9, sizeof(D3DCAPS9));

            if (FALSE == OsThunkDdGetDriverInfo(pUnknown6BC->hDirectDrawLocal, &DrvInfo) ||
                S_OK != DrvInfo.ddRVal ||
                DrvInfo.dwActualSize != sizeof(D3DCAPS9))
            {
                DPRINT1("Driver returned an invalid D3DCAPS9 structure - aborting");
                return FALSE;
            }

            pDriverCaps->DriverCaps9 = DriverCaps9;
            pDriverCaps->DriverCaps9.Caps = HalInfo.ddCaps.dwCaps;
            pDriverCaps->dwDriverCaps |= D3D9_INT_D3DCAPS9_VALID;
        }


        /* GUID_GetDriverInfo2 - Get format count data */
        {
            ResetGetDriverInfo2Data(&FormatCountData.gdi2, D3DGDI2_TYPE_GETFORMATCOUNT, sizeof(DD_GETFORMATCOUNTDATA));
            PrepareDriverInfoData(&DrvInfo, &FormatCountData, sizeof(DD_GETFORMATCOUNTDATA));
            FormatCountData.dwFormatCount = UINT_MAX;
            FormatCountData.dwReserved = dwDXVersion;

            if (TRUE == OsThunkDdGetDriverInfo(pUnknown6BC->hDirectDrawLocal, &DrvInfo))
            {
                if (DrvInfo.ddRVal != S_OK)
                {
                    DPRINT1("Driver claimed to be DX9 driver, but didn't support D3DGDI_TYPE_GETFORMATCOUNT in GetDriverInfo call");
                    return FALSE;
                }
                else if (DrvInfo.dwActualSize != sizeof(DD_GETFORMATCOUNTDATA))
                {
                    DPRINT1("Driver returned an invalid DD_GETFORMATCOUNTDATA structure - aborting");
                    return FALSE;
                }
                else if (FormatCountData.dwFormatCount == UINT_MAX)
                {
                    DPRINT1("Driver didn't set DD_GETFORMATCOUNTDATA.dwFormatCount - aborting");
                    return FALSE;
                }

                *pNumTextureFormats = FormatCountData.dwFormatCount;
            }
        }

        /* GUID_GetDriverInfo2 - Get format data */
        if (puD3dTextureFormats != NULL)
        {
            DWORD FormatIndex;
            DD_GETFORMATDATA FormatData;

            for (FormatIndex = 0; FormatIndex < FormatCountData.dwFormatCount; FormatIndex++)
            {
                ResetGetDriverInfo2Data(&FormatData.gdi2, D3DGDI2_TYPE_GETFORMAT, sizeof(DD_GETFORMATDATA));
                PrepareDriverInfoData(&DrvInfo, &FormatData, sizeof(DD_GETFORMATDATA));
                FormatData.dwFormatIndex = FormatIndex;

                if (TRUE == OsThunkDdGetDriverInfo(pUnknown6BC->hDirectDrawLocal, &DrvInfo))
                {
                    if (DrvInfo.ddRVal != S_OK)
                    {
                        DPRINT1("Driver claimed to be DX9 driver, but didn't support D3DGDI_TYPE_GETFORMAT in GetDriverInfo call");
                        return FALSE;
                    }
                    else if (DrvInfo.dwActualSize != sizeof(DD_GETFORMATDATA))
                    {
                        DPRINT1("Driver returned an invalid DD_GETFORMATDATA structure - aborting");
                        return FALSE;
                    }
                    else if (FormatData.format.dwSize != sizeof(DDPIXELFORMAT))
                    {
                        DPRINT1("Driver didn't set DD_GETFORMATDATA.format - aborting");
                        return FALSE;
                    }

                    /* Copy format data to puD3dTextureFormats */
                    memset(puD3dTextureFormats, 0, sizeof(DDSURFACEDESC));
                    puD3dTextureFormats->dwSize = sizeof(DDSURFACEDESC);
                    puD3dTextureFormats->dwFlags = DDSD_PIXELFORMAT;
                    memcpy(&puD3dTextureFormats->ddpfPixelFormat, &FormatData.format, sizeof(DDPIXELFORMAT));

                    if ((FormatData.format.dwOperations & D3DFORMAT_OP_PIXELSIZE) != 0 &&
                        FormatData.format.dwPrivateFormatBitCount > 0)
                    {
                        /* TODO: Register driver's own pixelformat */
                    }

                    ++puD3dTextureFormats;
                }
            }
        }

        /* GUID_GetDriverInfo2 - Get extended mode count data */
        {
            ResetGetDriverInfo2Data(&ExModeCountData.gdi2, D3DGDI2_TYPE_GETEXTENDEDMODECOUNT, sizeof(DD_GETEXTENDEDMODECOUNTDATA));
            PrepareDriverInfoData(&DrvInfo, &ExModeCountData, sizeof(DD_GETEXTENDEDMODECOUNTDATA));
            ExModeCountData.dwModeCount = UINT_MAX;
            ExModeCountData.dwReserved = dwDXVersion;

            if (TRUE == OsThunkDdGetDriverInfo(pUnknown6BC->hDirectDrawLocal, &DrvInfo))
            {
                if (DrvInfo.ddRVal == S_OK)
                {
                    if (DrvInfo.dwActualSize != sizeof(DD_GETEXTENDEDMODECOUNTDATA))
                    {
                        DPRINT1("Driver returned an invalid DD_GETEXTENDEDFORMATCOUNTDATA structure - aborting");
                        return FALSE;
                    }
                    else if (ExModeCountData.dwModeCount == UINT_MAX)
                    {
                        DPRINT1("Driver didn't set DD_GETEXTENDEDMODECOUNTDATA.dwModeCount - aborting");
                        return FALSE;
                    }

                    *pNumExtendedFormats = ExModeCountData.dwModeCount;
                }
                else
                {
                    ExModeCountData.dwModeCount = 0;
                }
            }
        }

        /* GUID_GetDriverInfo2 - Get extended mode data */
        if (pD3dDisplayModeList != NULL)
        {
            DWORD ModeIndex;
            DD_GETEXTENDEDMODEDATA ExModeData;

            for (ModeIndex = 0; ModeIndex < ExModeCountData.dwModeCount; ModeIndex++)
            {
                ResetGetDriverInfo2Data(&ExModeData.gdi2, D3DGDI2_TYPE_GETEXTENDEDMODE, sizeof(DD_GETEXTENDEDMODEDATA));
                PrepareDriverInfoData(&DrvInfo, &ExModeData, sizeof(DD_GETEXTENDEDMODEDATA));
                ExModeData.dwModeIndex = ModeIndex;
                ExModeData.mode.Width = UINT_MAX;

                if (TRUE == OsThunkDdGetDriverInfo(pUnknown6BC->hDirectDrawLocal, &DrvInfo))
                {
                    if (DrvInfo.ddRVal != S_OK)
                    {
                        DPRINT1("Driver claimed to be DX9 driver, but didn't support D3DGDI2_TYPE_GETEXTENDEDMODE in GetDriverInfo call");
                        return FALSE;
                    }
                    else if (DrvInfo.dwActualSize != sizeof(DD_GETEXTENDEDMODEDATA))
                    {
                        DPRINT1("Driver returned an invalid DD_GETEXTENDEDMODEDATA structure - aborting");
                        return FALSE;
                    }
                    else if (ExModeData.mode.Width != UINT_MAX)
                    {
                        DPRINT1("Driver didn't set DD_GETEXTENDEDMODEDATA.mode - aborting");
                        return FALSE;
                    }

                    memcpy(pD3dDisplayModeList, &ExModeData.mode, sizeof(D3DDISPLAYMODE));
                    ++pD3dDisplayModeList;
                }
            }
        }

        /* GUID_GetDriverInfo2 - Get adapter group */
        {
            DD_GETADAPTERGROUPDATA AdapterGroupData;
            ResetGetDriverInfo2Data(&AdapterGroupData.gdi2, D3DGDI2_TYPE_GETADAPTERGROUP, sizeof(DD_GETADAPTERGROUPDATA));
            PrepareDriverInfoData(&DrvInfo, &AdapterGroupData, sizeof(DD_GETADAPTERGROUPDATA));
            AdapterGroupData.ulUniqueAdapterGroupId = UINT_MAX;

            if (TRUE == OsThunkDdGetDriverInfo(pUnknown6BC->hDirectDrawLocal, &DrvInfo))
            {
                if (DrvInfo.ddRVal != S_OK)
                {
                    DPRINT1("Driver claimed to be DX9 driver, but didn't support D3DGDI2_TYPE_GETADAPTERGROUP in GetDriverInfo call");
                    return FALSE;
                }
                else if (DrvInfo.dwActualSize != sizeof(DD_GETADAPTERGROUPDATA))
                {
                    DPRINT1("Driver returned an invalid DD_GETADAPTERGROUPDATA structure - aborting");
                    return FALSE;
                }
                else if (AdapterGroupData.ulUniqueAdapterGroupId == UINT_MAX)
                {
                    DPRINT1("Driver didn't set DD_GETADAPTERGROUPDATA.ulUniqueAdapterGroupId - aborting");
                    return FALSE;
                }

                pDriverCaps->ulUniqueAdapterGroupId = (ULONG)AdapterGroupData.ulUniqueAdapterGroupId;
            }
        }

        /* GUID_GetDriverInfo2 - Query count data */
        {
            ResetGetDriverInfo2Data(&D3dQueryCountData.gdi2, D3DGDI2_TYPE_GETD3DQUERYCOUNT, sizeof(DD_GETD3DQUERYCOUNTDATA));
            PrepareDriverInfoData(&DrvInfo, &D3dQueryCountData, sizeof(DD_GETD3DQUERYCOUNTDATA));
            D3dQueryCountData.dwNumQueries = UINT_MAX;

            if (TRUE == OsThunkDdGetDriverInfo(pUnknown6BC->hDirectDrawLocal, &DrvInfo))
            {
                if (DrvInfo.ddRVal != S_OK)
                {
                    DPRINT1("Driver claimed to be DX9 driver, but didn't support D3DGDI2_TYPE_GETD3DQUERYCOUNT in GetDriverInfo call");
                    return FALSE;
                }
                else if (DrvInfo.dwActualSize != sizeof(DD_GETD3DQUERYCOUNTDATA))
                {
                    DPRINT1("Driver returned an invalid DD_GETD3DQUERYCOUNTDATA structure - aborting");
                    return FALSE;
                }
                else if (D3dQueryCountData.dwNumQueries == UINT_MAX)
                {
                    DPRINT1("Driver didn't set DD_GETD3DQUERYCOUNTDATA.dwNumQueries - aborting");
                    return FALSE;
                }

                *pNumQueries = D3dQueryCountData.dwNumQueries;
            }
        }

        /* GUID_GetDriverInfo2 - Query data */
        if (pD3dQueryList != NULL)
        {
            DWORD QueryIndex;
            DD_GETD3DQUERYDATA D3dQueryData;

            for (QueryIndex = 0; QueryIndex < D3dQueryCountData.dwNumQueries; QueryIndex++)
            {
                ResetGetDriverInfo2Data(&D3dQueryData.gdi2, D3DGDI2_TYPE_GETD3DQUERY, sizeof(DD_GETD3DQUERYDATA));
                PrepareDriverInfoData(&DrvInfo, &D3dQueryData, sizeof(DD_GETD3DQUERYDATA));
                D3dQueryData.dwQueryIndex = QueryIndex;

                if (TRUE == OsThunkDdGetDriverInfo(pUnknown6BC->hDirectDrawLocal, &DrvInfo))
                {
                    if (DrvInfo.ddRVal != S_OK)
                    {
                        DPRINT1("Driver claimed to be DX9 driver, but didn't support D3DGDI2_TYPE_GETD3DQUERY in GetDriverInfo call");
                        return FALSE;
                    }
                    else if (DrvInfo.dwActualSize != sizeof(DD_GETD3DQUERYDATA))
                    {
                        DPRINT1("Driver returned an invalid DD_GETD3DQUERYDATA structure - aborting");
                        return FALSE;
                    }

                    *pD3dQueryList = D3dQueryData.QueryType;
                    ++pD3dQueryList;
                }
            }
        }
    }

    /* D3dDriverData -> pGblDriverData */
    memcpy(&pGblDriverData->hwCaps, &D3dDriverData.hwCaps, sizeof(D3DNTHALDEVICEDESC_V1));
    pGblDriverData->dwNumVertices = D3dDriverData.dwNumVertices;
    pGblDriverData->dwNumClipVertices = D3dDriverData.dwNumClipVertices;

    /* GUID_D3DExtendedCaps */
    {
        DrvInfo.dwSize = sizeof(DD_GETDRIVERINFODATA);
        DrvInfo.guidInfo = GUID_D3DExtendedCaps;
        DrvInfo.dwExpectedSize = sizeof(D3DHAL_D3DEXTENDEDCAPS);
        DrvInfo.lpvData = pD3dExtendedCaps;
        bRet = OsThunkDdGetDriverInfo(pUnknown6BC->hDirectDrawLocal, &DrvInfo);

        if (TRUE != bRet || DrvInfo.ddRVal != S_OK)
        {
            DPRINT1("Driver failed call to GetDriverInfo() with: GUID_D3DExtendedCaps");
            return FALSE;
        }
    }

    /* GUID_ZPixelFormats */
    {
        DDPIXELFORMAT *pZPixelFormats = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FormatCountData.dwFormatCount * sizeof(DDPIXELFORMAT));

        DrvInfo.dwSize = sizeof(DD_GETDRIVERINFODATA);
        DrvInfo.guidInfo = GUID_ZPixelFormats;
        DrvInfo.dwExpectedSize = FormatCountData.dwFormatCount * sizeof(DDPIXELFORMAT);
        DrvInfo.lpvData = pZPixelFormats;
        bRet = OsThunkDdGetDriverInfo(pUnknown6BC->hDirectDrawLocal, &DrvInfo);

        if (TRUE != bRet || DrvInfo.ddRVal != S_OK)
        {
            DPRINT1("Driver failed call to GetDriverInfo() with: GUID_ZPixelFormats");
            HeapFree(GetProcessHeap(), 0, pZPixelFormats);
            return FALSE;
        }

        *pNumZStencilFormats = FormatCountData.dwFormatCount;

        if (pD3dZStencilFormatList != NULL)
            memcpy(pD3dZStencilFormatList, pZPixelFormats, FormatCountData.dwFormatCount * sizeof(DDPIXELFORMAT));

        HeapFree(GetProcessHeap(), 0, pZPixelFormats);
    }

    /* Get current display format */
    {
        D3DDISPLAYMODE CurrentDisplayMode;
        GetAdapterMode(lpszDeviceName, &CurrentDisplayMode);
        pUnknown6BC->RawDisplayFormat = CurrentDisplayMode.Format;
        pUnknown6BC->DisplayFormat = CurrentDisplayMode.Format;

        if ((HalInfo.vmiData.ddpfDisplay.dwFlags & DDPF_ALPHAPIXELS) != 0)
        {
            if (CurrentDisplayMode.Format == D3DFMT_X8R8G8B8)
            {
                pUnknown6BC->DisplayFormat = D3DFMT_A8R8G8B8;
            }
            else if (CurrentDisplayMode.Format == D3DFMT_X1R5G5B5)
            {
                pUnknown6BC->DisplayFormat = D3DFMT_A1R5G5B5;
            }
        }

        pDriverCaps->dwDisplayWidth = CurrentDisplayMode.Width;
        pDriverCaps->dwDisplayHeight = CurrentDisplayMode.Height;
        pDriverCaps->RawDisplayFormat = CurrentDisplayMode.Format;
        pDriverCaps->DisplayFormat = pUnknown6BC->DisplayFormat;
        pDriverCaps->dwRefreshRate = CurrentDisplayMode.RefreshRate;
    }

    /* TODO: Set all internal function pointers to create surface, etc. */
    pD3D9Callbacks->DdGetAvailDriverMemory = &D3d9GetAvailDriverMemory;

    /* Set device rect */
    {
        HMONITOR hMonitor;
        MONITORINFO MonitorInfo;

        memset(&MonitorInfo, 0, sizeof(MONITORINFO));
        MonitorInfo.cbSize = sizeof(MONITORINFO);

        hMonitor = GetAdapterMonitor(lpszDeviceName);
        if (TRUE == GetMonitorInfoA(hMonitor, &MonitorInfo))
        {
            pUnknown6BC->DeviceRect = MonitorInfo.rcMonitor;
        }
        else
        {
            DPRINT1("Could not get monitor information");
        }
    }

    pUnknown6BC->dwCaps = pDriverCaps->DriverCaps9.Caps;
    pUnknown6BC->dwSVBCaps = pDriverCaps->dwSVBCaps;

    if (FALSE == bDX8Mode)
    {
        pUnknown6BC->MajorDxVersion = 9;

        if (0 != (pDriverCaps->DriverCaps9.VertexProcessingCaps & D3DVTXPCAPS_NO_VSDT_UBYTE4))
        {
            DPRINT1("Driver claimed to be DX9 driver, but used depricated D3DCAPS9.VertexProcessingCaps: D3DVTXPCAPS_NO_VSDT_UBYTE4 instead of not setting D3DCAPS9.DeclTypes: D3DDTCAPS_UBYTE4.");
            return FALSE;
        }
    }
    else
    {
        pUnknown6BC->MajorDxVersion = 8;

        if (0 == (pDriverCaps->DriverCaps9.VertexProcessingCaps & D3DVTXPCAPS_NO_VSDT_UBYTE4))
        {
            pDriverCaps->DriverCaps9.DeclTypes |= D3DDTCAPS_UBYTE4;
            pDriverCaps->DriverCaps9.VertexProcessingCaps &= ~D3DVTXPCAPS_NO_VSDT_UBYTE4;
        }
    }

    return TRUE;
}
