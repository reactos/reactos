/*
 * Dwmapi
 *
 * Copyright 2007 Andras Kovacs
 * Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * ReactOS/VerOS DwmIsCompositionEnabled stays FALSE: guest
 * output/ntdll-apps/serial-App-20261116T205323Z.log reports
 * VERPORT PsGetVersion 10.0.19045, so RtlGetVersion made composition
 * look enabled while DwmExtendFrameIntoClientArea was a no-op S_OK stub.
 * Public: https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmiscompositionenabled
 * ExtendFrame: https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmextendframeintoclientarea
 * In-tree DwmGetTransportAttributes already returns DWM_E_COMPOSITIONDISABLED.
 * Guest after composition-off: serial-App-20261116T210232Z.log still
 * FIXME DwmExtendFrameIntoClientArea; Ryan could not scroll, minimize,
 * or click Start through the Firefox window.
 */

#include <stdarg.h>

#ifdef __REACTOS__
#include <rtlfuncs.h>
#else
#include "winternl.h"
#endif

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "dwmapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwmapi);


/**********************************************************************
 *           DwmIsCompositionEnabled         (DWMAPI.@)
 *
 * Guest: output/ntdll-apps/serial-App-20261116T205323Z.log VERPORT
 * PsGetVersion 10.0.19045. With composition reported TRUE, Firefox
 * extends into a glass frame that VerOS does not composite, so the
 * title bar shows the desktop and the content client stays solid grey.
 */
HRESULT WINAPI DwmIsCompositionEnabled(BOOL *enabled)
{
#ifndef __REACTOS__
    RTL_OSVERSIONINFOEXW version;
#endif

    TRACE("%p\n", enabled);

    if (!enabled)
        return E_INVALIDARG;

    *enabled = FALSE;
#ifndef __REACTOS__
    /*
     * Wine may advertise composition from the host OS version. VerOS has
     * no DWM compositor (DwmExtendFrameIntoClientArea is a stub; transport
     * attributes already return DWM_E_COMPOSITIONDISABLED), so leave FALSE.
     */
    version.dwOSVersionInfoSize = sizeof(version);
    if (!RtlGetVersion(&version))
        *enabled = (version.dwMajorVersion > 6 || (version.dwMajorVersion == 6 && version.dwMinorVersion >= 3));
#endif

    return S_OK;
}

/**********************************************************************
 *           DwmEnableComposition         (DWMAPI.102)
 */
HRESULT WINAPI DwmEnableComposition(UINT uCompositionAction)
{
    FIXME("(%d) stub\n", uCompositionAction);

    return S_OK;
}

/**********************************************************************
 *           DwmExtendFrameIntoClientArea    (DWMAPI.@)
 *
 * Guest: output/ntdll-apps/serial-App-20261116T210232Z.log still
 * FIXME-stubbed this after DwmIsCompositionEnabled stayed FALSE.
 * Returning S_OK let Firefox keep glass margins with no DWM, so
 * clicks missed Start/desktop and about:support would not scroll.
 * MSDN: composition disabled -> DWM_E_COMPOSITIONDISABLED.
 */
HRESULT WINAPI DwmExtendFrameIntoClientArea(HWND hwnd, const MARGINS* margins)
{
    TRACE("(%p, %p)\n", hwnd, margins);

    return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmGetColorizationColor      (DWMAPI.@)
 */
HRESULT WINAPI DwmGetColorizationColor(DWORD *colorization, BOOL *opaque_blend)
{
    FIXME("(%p, %p) stub\n", colorization, opaque_blend);

    return E_NOTIMPL;
}

/**********************************************************************
 *                  DwmFlush              (DWMAPI.@)
 */
HRESULT WINAPI DwmFlush(void)
{
    static BOOL once;

    if (!once++) FIXME("() stub\n");

    return S_OK;
}

/**********************************************************************
 *        DwmInvalidateIconicBitmaps      (DWMAPI.@)
 */
HRESULT WINAPI DwmInvalidateIconicBitmaps(HWND hwnd)
{
    static BOOL once;

    if (!once++) FIXME("(%p) stub\n", hwnd);

    return E_NOTIMPL;
}

/**********************************************************************
 *           DwmSetWindowAttribute         (DWMAPI.@)
 */
HRESULT WINAPI DwmSetWindowAttribute(HWND hwnd, DWORD attributenum, LPCVOID attribute, DWORD size)
{
    static BOOL once;

    if (!once++) FIXME("(%p, %lx, %p, %lx) stub\n", hwnd, attributenum, attribute, size);

    return S_OK;
}

/**********************************************************************
 *           DwmGetGraphicsStreamClient         (DWMAPI.@)
 */
HRESULT WINAPI DwmGetGraphicsStreamClient(UINT uIndex, UUID *pClientUuid)
{
    FIXME("(%d, %p) stub\n", uIndex, pClientUuid);

    return E_NOTIMPL;
}

/**********************************************************************
 *           DwmGetTransportAttributes         (DWMAPI.@)
 */
HRESULT WINAPI DwmGetTransportAttributes(BOOL *pfIsRemoting, BOOL *pfIsConnected, DWORD *pDwGeneration)
{
    FIXME("(%p, %p, %p) stub\n", pfIsRemoting, pfIsConnected, pDwGeneration);

    return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmUnregisterThumbnail         (DWMAPI.@)
 */
HRESULT WINAPI DwmUnregisterThumbnail(HTHUMBNAIL thumbnail)
{
    FIXME("(%p) stub\n", thumbnail);

    return E_NOTIMPL;
}

/**********************************************************************
 *           DwmEnableMMCSS         (DWMAPI.@)
 */
HRESULT WINAPI DwmEnableMMCSS(BOOL enableMMCSS)
{
    FIXME("(%d) stub\n", enableMMCSS);

    return S_OK;
}

/**********************************************************************
 *           DwmGetGraphicsStreamTransformHint         (DWMAPI.@)
 */
HRESULT WINAPI DwmGetGraphicsStreamTransformHint(UINT uIndex, MilMatrix3x2D *pTransform)
{
    FIXME("(%d, %p) stub\n", uIndex, pTransform);

    return E_NOTIMPL;
}

/**********************************************************************
 *           DwmEnableBlurBehindWindow         (DWMAPI.@)
 */
HRESULT WINAPI DwmEnableBlurBehindWindow(HWND hWnd, const DWM_BLURBEHIND *pBlurBuf)
{
    FIXME("%p %p\n", hWnd, pBlurBuf);

    return E_NOTIMPL;
}

/**********************************************************************
 *           DwmDefWindowProc         (DWMAPI.@)
 */
BOOL WINAPI DwmDefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    static int i;

    if (!i++) FIXME("stub\n");

    return FALSE;
}

/**********************************************************************
 *           DwmGetWindowAttribute         (DWMAPI.@)
 */
HRESULT WINAPI DwmGetWindowAttribute(HWND hwnd, DWORD attribute, PVOID pv_attribute, DWORD size)
{
    FIXME("(%p %ld %p %ld) stub\n", hwnd, attribute, pv_attribute, size);

    return E_NOTIMPL;
}

/**********************************************************************
 *           DwmRegisterThumbnail         (DWMAPI.@)
 */
HRESULT WINAPI DwmRegisterThumbnail(HWND dest, HWND src, PHTHUMBNAIL thumbnail_id)
{
    FIXME("(%p %p %p) stub\n", dest, src, thumbnail_id);

    return E_NOTIMPL;
}

static int get_display_frequency(void)
{
    DEVMODEW mode;
    BOOL ret;

    memset(&mode, 0, sizeof(mode));
    mode.dmSize = sizeof(mode);
    ret = EnumDisplaySettingsExW(NULL, ENUM_CURRENT_SETTINGS, &mode, 0);
    if (ret && mode.dmFields & DM_DISPLAYFREQUENCY && mode.dmDisplayFrequency)
    {
        return mode.dmDisplayFrequency;
    }
    else
    {
        WARN("Failed to query display frequency, returning a fallback value.\n");
        return 60;
    }
}

/**********************************************************************
 *           DwmGetCompositionTimingInfo         (DWMAPI.@)
 */
HRESULT WINAPI DwmGetCompositionTimingInfo(HWND hwnd, DWM_TIMING_INFO *info)
{
    LARGE_INTEGER performance_frequency, qpc;
    static int i, display_frequency;

    if (!info)
        return E_INVALIDARG;

    if (info->cbSize != sizeof(DWM_TIMING_INFO))
        return MILERR_MISMATCHED_SIZE;

    if(!i++) FIXME("(%p %p)\n", hwnd, info);

    memset(info, 0, info->cbSize);
    info->cbSize = sizeof(DWM_TIMING_INFO);

    display_frequency = get_display_frequency();
    info->rateRefresh.uiNumerator = display_frequency;
    info->rateRefresh.uiDenominator = 1;
    info->rateCompose.uiNumerator = display_frequency;
    info->rateCompose.uiDenominator = 1;

    QueryPerformanceFrequency(&performance_frequency);
    info->qpcRefreshPeriod = performance_frequency.QuadPart / display_frequency;

    QueryPerformanceCounter(&qpc);
    info->qpcVBlank = (qpc.QuadPart / info->qpcRefreshPeriod) * info->qpcRefreshPeriod;

    return S_OK;
}

/**********************************************************************
 *           DwmAttachMilContent         (DWMAPI.@)
 */
HRESULT WINAPI DwmAttachMilContent(HWND hwnd)
{
    FIXME("(%p) stub\n", hwnd);
    return E_NOTIMPL;
}

/**********************************************************************
 *           DwmDetachMilContent         (DWMAPI.@)
 */
HRESULT WINAPI DwmDetachMilContent(HWND hwnd)
{
    FIXME("(%p) stub\n", hwnd);
    return E_NOTIMPL;
}

/**********************************************************************
 *           DwmUpdateThumbnailProperties         (DWMAPI.@)
 */
HRESULT WINAPI DwmUpdateThumbnailProperties(HTHUMBNAIL thumbnail, const DWM_THUMBNAIL_PROPERTIES *props)
{
    FIXME("(%p, %p) stub\n", thumbnail, props);
    return E_NOTIMPL;
}

/**********************************************************************
 *           DwmSetPresentParameters         (DWMAPI.@)
 */
HRESULT WINAPI DwmSetPresentParameters(HWND hwnd, DWM_PRESENT_PARAMETERS *params)
{
    FIXME("(%p %p) stub\n", hwnd, params);
    return S_OK;
};

/**********************************************************************
 *           DwmSetIconicLivePreviewBitmap         (DWMAPI.@)
 */
HRESULT WINAPI DwmSetIconicLivePreviewBitmap(HWND hwnd, HBITMAP hbmp, POINT *pos, DWORD flags)
{
    FIXME("(%p %p %p %lx) stub\n", hwnd, hbmp, pos, flags);
    return S_OK;
};

/**********************************************************************
 *           DwmSetIconicThumbnail         (DWMAPI.@)
 */
HRESULT WINAPI DwmSetIconicThumbnail(HWND hwnd, HBITMAP hbmp, DWORD flags)
{
    FIXME("(%p %p %lx) stub\n", hwnd, hbmp, flags);
    return S_OK;
};

/**********************************************************************
 *           DwmpGetColorizationParameters         (DWMAPI.@)
 */
HRESULT WINAPI DwmpGetColorizationParameters(void *params)
{
    FIXME("(%p) stub\n", params);
    return E_NOTIMPL;
}
