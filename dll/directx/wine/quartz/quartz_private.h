/*              DirectShow private interfaces (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
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
 */

#ifndef __QUARTZ_PRIVATE_INCLUDED__
#define __QUARTZ_PRIVATE_INCLUDED__

#include <stdarg.h>
#include <stdbool.h>
#include <wchar.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"
#include "dvdmedia.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/strmbase.h"
#include "wine/list.h"

#define DECLARE_CRITICAL_SECTION(cs)                                    \
    static CRITICAL_SECTION cs;                                         \
    static CRITICAL_SECTION_DEBUG cs##_debug =                          \
    { 0, 0, &cs, { &cs##_debug.ProcessLocksList, &cs##_debug.ProcessLocksList }, \
      0, 0, { (DWORD_PTR)(__FILE__ ": " # cs) }};                       \
    static CRITICAL_SECTION cs = { &cs##_debug, -1, 0, 0, 0, 0 };

/* see IAsyncReader::Request on MSDN for the explanation of this */
#define MEDIATIME_FROM_BYTES(x) ((LONGLONG)(x) * 10000000)
#define BYTES_FROM_MEDIATIME(time) ((time) / 10000000)

bool array_reserve(void **elements, size_t *capacity, size_t count, size_t size);

HRESULT acm_wrapper_create(IUnknown *outer, IUnknown **out);
HRESULT async_reader_create(IUnknown *outer, IUnknown **out);
HRESULT avi_dec_create(IUnknown *outer, IUnknown **out);
HRESULT avi_splitter_create(IUnknown *outer, IUnknown **out);
HRESULT dsound_render_create(IUnknown *outer, IUnknown **out);
HRESULT filter_graph_create(IUnknown *outer, IUnknown **out);
HRESULT filter_graph_no_thread_create(IUnknown *outer, IUnknown **out);
HRESULT filter_mapper_create(IUnknown *outer, IUnknown **out);
HRESULT mem_allocator_create(IUnknown *outer, IUnknown **out);
HRESULT mpeg_audio_codec_create(IUnknown *outer, IUnknown **out);
HRESULT mpeg_layer3_decoder_create(IUnknown *outer, IUnknown **out);
HRESULT mpeg_video_codec_create(IUnknown *outer, IUnknown **out);
HRESULT mpeg1_splitter_create(IUnknown *outer, IUnknown **out);
HRESULT system_clock_create(IUnknown *outer, IUnknown **out);
HRESULT seeking_passthrough_create(IUnknown *outer, IUnknown **out);
HRESULT video_renderer_create(IUnknown *outer, IUnknown **out);
HRESULT video_renderer_default_create(IUnknown *outer, IUnknown **out);
HRESULT vmr7_presenter_create(IUnknown *outer, IUnknown **out);
HRESULT vmr7_create(IUnknown *outer, IUnknown **out);
HRESULT vmr9_create(IUnknown *outer, IUnknown **out);
HRESULT wave_parser_create(IUnknown *outer, IUnknown **out);

extern const char * qzdebugstr_guid(const GUID * id);
extern void video_unregister_windowclass(void);

BOOL get_media_type(const WCHAR *filename, GUID *majortype, GUID *subtype, GUID *source_clsid);

struct video_window
{
    IVideoWindow IVideoWindow_iface;
    IBasicVideo IBasicVideo_iface;

    RECT src, dst;
    BOOL default_dst;

    BOOL AutoShow;
    HWND hwnd;
    HWND hwndDrain;
    HWND hwndOwner;
    struct strmbase_filter *pFilter;
    struct strmbase_pin *pPin;
    const struct video_window_ops *ops;
};

struct video_window_ops
{
    void (*get_default_rect)(struct video_window *window, RECT *rect);
    HRESULT (*get_current_image)(struct video_window *window, LONG *size, LONG *image);
};

void video_window_cleanup(struct video_window *window);
HRESULT video_window_create_window(struct video_window *window);
void video_window_init(struct video_window *window, const IVideoWindowVtbl *vtbl,
        struct strmbase_filter *filter, struct strmbase_pin *pin, const struct video_window_ops *ops);
void video_window_unregister_class(void);

HRESULT WINAPI BaseControlWindowImpl_QueryInterface(IVideoWindow *iface, REFIID iid, void **out);
ULONG WINAPI BaseControlWindowImpl_AddRef(IVideoWindow *iface);
ULONG WINAPI BaseControlWindowImpl_Release(IVideoWindow *iface);
HRESULT WINAPI BaseControlWindowImpl_GetTypeInfoCount(IVideoWindow *iface, UINT *pctinfo);
HRESULT WINAPI BaseControlWindowImpl_GetTypeInfo(IVideoWindow *iface, UINT iTInfo, LCID lcid, ITypeInfo**ppTInfo);
HRESULT WINAPI BaseControlWindowImpl_GetIDsOfNames(IVideoWindow *iface, REFIID riid, LPOLESTR*rgszNames, UINT cNames, LCID lcid, DISPID*rgDispId);
HRESULT WINAPI BaseControlWindowImpl_Invoke(IVideoWindow *iface, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS*pDispParams, VARIANT*pVarResult, EXCEPINFO*pExepInfo, UINT*puArgErr);
HRESULT WINAPI BaseControlWindowImpl_put_Caption(IVideoWindow *iface, BSTR strCaption);
HRESULT WINAPI BaseControlWindowImpl_get_Caption(IVideoWindow *iface, BSTR *strCaption);
HRESULT WINAPI BaseControlWindowImpl_put_WindowStyle(IVideoWindow *iface, LONG WindowStyle);
HRESULT WINAPI BaseControlWindowImpl_get_WindowStyle(IVideoWindow *iface, LONG *WindowStyle);
HRESULT WINAPI BaseControlWindowImpl_put_WindowStyleEx(IVideoWindow *iface, LONG WindowStyleEx);
HRESULT WINAPI BaseControlWindowImpl_get_WindowStyleEx(IVideoWindow *iface, LONG *WindowStyleEx);
HRESULT WINAPI BaseControlWindowImpl_put_AutoShow(IVideoWindow *iface, LONG AutoShow);
HRESULT WINAPI BaseControlWindowImpl_get_AutoShow(IVideoWindow *iface, LONG *AutoShow);
HRESULT WINAPI BaseControlWindowImpl_put_WindowState(IVideoWindow *iface, LONG WindowState);
HRESULT WINAPI BaseControlWindowImpl_get_WindowState(IVideoWindow *iface, LONG *WindowState);
HRESULT WINAPI BaseControlWindowImpl_put_BackgroundPalette(IVideoWindow *iface, LONG BackgroundPalette);
HRESULT WINAPI BaseControlWindowImpl_get_BackgroundPalette(IVideoWindow *iface, LONG *pBackgroundPalette);
HRESULT WINAPI BaseControlWindowImpl_put_Visible(IVideoWindow *iface, LONG Visible);
HRESULT WINAPI BaseControlWindowImpl_get_Visible(IVideoWindow *iface, LONG *pVisible);
HRESULT WINAPI BaseControlWindowImpl_put_Left(IVideoWindow *iface, LONG Left);
HRESULT WINAPI BaseControlWindowImpl_get_Left(IVideoWindow *iface, LONG *pLeft);
HRESULT WINAPI BaseControlWindowImpl_put_Width(IVideoWindow *iface, LONG Width);
HRESULT WINAPI BaseControlWindowImpl_get_Width(IVideoWindow *iface, LONG *pWidth);
HRESULT WINAPI BaseControlWindowImpl_put_Top(IVideoWindow *iface, LONG Top);
HRESULT WINAPI BaseControlWindowImpl_get_Top(IVideoWindow *iface, LONG *pTop);
HRESULT WINAPI BaseControlWindowImpl_put_Height(IVideoWindow *iface, LONG Height);
HRESULT WINAPI BaseControlWindowImpl_get_Height(IVideoWindow *iface, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_put_Owner(IVideoWindow *iface, OAHWND Owner);
HRESULT WINAPI BaseControlWindowImpl_get_Owner(IVideoWindow *iface, OAHWND *Owner);
HRESULT WINAPI BaseControlWindowImpl_put_MessageDrain(IVideoWindow *iface, OAHWND Drain);
HRESULT WINAPI BaseControlWindowImpl_get_MessageDrain(IVideoWindow *iface, OAHWND *Drain);
HRESULT WINAPI BaseControlWindowImpl_get_BorderColor(IVideoWindow *iface, LONG *Color);
HRESULT WINAPI BaseControlWindowImpl_put_BorderColor(IVideoWindow *iface, LONG Color);
HRESULT WINAPI BaseControlWindowImpl_get_FullScreenMode(IVideoWindow *iface, LONG *FullScreenMode);
HRESULT WINAPI BaseControlWindowImpl_put_FullScreenMode(IVideoWindow *iface, LONG FullScreenMode);
HRESULT WINAPI BaseControlWindowImpl_SetWindowForeground(IVideoWindow *iface, LONG Focus);
HRESULT WINAPI BaseControlWindowImpl_SetWindowPosition(IVideoWindow *iface, LONG Left, LONG Top, LONG Width, LONG Height);
HRESULT WINAPI BaseControlWindowImpl_GetWindowPosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_NotifyOwnerMessage(IVideoWindow *iface, OAHWND hwnd, LONG uMsg, LONG_PTR wParam, LONG_PTR lParam);
HRESULT WINAPI BaseControlWindowImpl_GetMinIdealImageSize(IVideoWindow *iface, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_GetMaxIdealImageSize(IVideoWindow *iface, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_GetRestorePosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_HideCursor(IVideoWindow *iface, LONG HideCursor);
HRESULT WINAPI BaseControlWindowImpl_IsCursorHidden(IVideoWindow *iface, LONG *CursorHidden);

#endif /* __QUARTZ_PRIVATE_INCLUDED__ */
