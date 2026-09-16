/*
 * Copyright 2019 Jacek Caban for CodeWeavers
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

#include <stdarg.h>
#include <math.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "mshtmdid.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct SVGElement {
    HTMLElement element;
    ISVGElement ISVGElement_iface;
};

static inline SVGElement *impl_from_ISVGElement(ISVGElement *iface)
{
    return CONTAINING_RECORD(iface, SVGElement, ISVGElement_iface);
}

DISPEX_IDISPATCH_IMPL(SVGElement, ISVGElement,
                      impl_from_ISVGElement(iface)->element.node.event_target.dispex)

static HRESULT WINAPI SVGElement_put_xmlbase(ISVGElement *iface, BSTR v)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_get_xmlbase(ISVGElement *iface, BSTR *p)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_putref_ownerSVGElement(ISVGElement *iface, ISVGSVGElement *v)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_get_ownerSVGElement(ISVGElement *iface, ISVGSVGElement **p)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_putref_viewportElement(ISVGElement *iface, ISVGElement *v)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_get_viewportElement(ISVGElement *iface, ISVGElement **p)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_putref_focusable(ISVGElement *iface, ISVGAnimatedEnumeration *v)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGElement_get_focusable(ISVGElement *iface, ISVGAnimatedEnumeration **p)
{
    SVGElement *This = impl_from_ISVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const ISVGElementVtbl SVGElementVtbl = {
    SVGElement_QueryInterface,
    SVGElement_AddRef,
    SVGElement_Release,
    SVGElement_GetTypeInfoCount,
    SVGElement_GetTypeInfo,
    SVGElement_GetIDsOfNames,
    SVGElement_Invoke,
    SVGElement_put_xmlbase,
    SVGElement_get_xmlbase,
    SVGElement_putref_ownerSVGElement,
    SVGElement_get_ownerSVGElement,
    SVGElement_putref_viewportElement,
    SVGElement_get_viewportElement,
    SVGElement_putref_focusable,
    SVGElement_get_focusable
};

static inline SVGElement *SVGElement_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, SVGElement, element.node.event_target.dispex);
}

static void *SVGElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    SVGElement *This = SVGElement_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_ISVGElement, riid))
        return &This->ISVGElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static const NodeImplVtbl SVGElementImplVtbl = {
    .clsid                 = &CLSID_SVGElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
};

static const event_target_vtbl_t SVGElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= SVGElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLElement_traverse,
        .unlink         = HTMLElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

dispex_static_data_t SVGElement_dispex = {
    .id           = PROT_SVGElement,
    .prototype_id = PROT_Element,
    .vtbl         = &SVGElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLUnknownElement_tid,
    .init_info    = HTMLElement_init_dispex_info,
};

static void init_svg_element(SVGElement *svg_element, HTMLDocumentNode *doc, nsIDOMSVGElement *nselem, dispex_static_data_t *dispex_data)
{
    svg_element->ISVGElement_iface.lpVtbl = &SVGElementVtbl;
    HTMLElement_Init(&svg_element->element, doc, (nsIDOMElement*)nselem, dispex_data);
}

struct SVGSVGElement {
    SVGElement svg_element;
    ISVGSVGElement ISVGSVGElement_iface;
};

static inline SVGSVGElement *impl_from_ISVGSVGElement(ISVGSVGElement *iface)
{
    return CONTAINING_RECORD(iface, SVGSVGElement, ISVGSVGElement_iface);
}

DISPEX_IDISPATCH_IMPL(SVGSVGElement, ISVGSVGElement,
                      impl_from_ISVGSVGElement(iface)->svg_element.element.node.event_target.dispex)

static HRESULT WINAPI SVGSVGElement_putref_x(ISVGSVGElement *iface, ISVGAnimatedLength *v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_x(ISVGSVGElement *iface, ISVGAnimatedLength **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_putref_y(ISVGSVGElement *iface, ISVGAnimatedLength *v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_y(ISVGSVGElement *iface, ISVGAnimatedLength **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_putref_width(ISVGSVGElement *iface, ISVGAnimatedLength *v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_width(ISVGSVGElement *iface, ISVGAnimatedLength **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_putref_height(ISVGSVGElement *iface, ISVGAnimatedLength *v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_height(ISVGSVGElement *iface, ISVGAnimatedLength **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_put_contentScriptType(ISVGSVGElement *iface, BSTR v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_contentScriptType(ISVGSVGElement *iface, BSTR *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_put_contentStyleType(ISVGSVGElement *iface, BSTR v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_contentStyleType(ISVGSVGElement *iface, BSTR *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_putref_viewport(ISVGSVGElement *iface, ISVGRect *v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_viewport(ISVGSVGElement *iface, ISVGRect **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_put_pixelUnitToMillimeterX(ISVGSVGElement *iface, float v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%f)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_pixelUnitToMillimeterX(ISVGSVGElement *iface, float *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_put_pixelUnitToMillimeterY(ISVGSVGElement *iface, float v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%f)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_pixelUnitToMillimeterY(ISVGSVGElement *iface, float *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_put_screenPixelToMillimeterX(ISVGSVGElement *iface, float v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%f)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_screenPixelToMillimeterX(ISVGSVGElement *iface, float *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_put_screenPixelToMillimeterY(ISVGSVGElement *iface, float v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%f)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_screenPixelToMillimeterY(ISVGSVGElement *iface, float *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_put_useCurrentView(ISVGSVGElement *iface, VARIANT_BOOL v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_useCurrentView(ISVGSVGElement *iface, VARIANT_BOOL *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_putref_currentView(ISVGSVGElement *iface, ISVGViewSpec *v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_currentView(ISVGSVGElement *iface, ISVGViewSpec **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_put_currentScale(ISVGSVGElement *iface, float v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%f)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_currentScale(ISVGSVGElement *iface, float *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_putref_currentTranslate(ISVGSVGElement *iface, ISVGPoint *v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_get_currentTranslate(ISVGSVGElement *iface, ISVGPoint **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_suspendRedraw(ISVGSVGElement *iface, ULONG max_wait, ULONG *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%lu %p)\n", This, max_wait, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_unsuspendRedraw(ISVGSVGElement *iface, ULONG id)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%lu)\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_unsuspendRedrawAll(ISVGSVGElement *iface)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_forceRedraw(ISVGSVGElement *iface)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_pauseAnimations(ISVGSVGElement *iface)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_unpauseAnimations(ISVGSVGElement *iface)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_animationsPaused(ISVGSVGElement *iface, VARIANT_BOOL *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_getCurrentTime(ISVGSVGElement *iface, float *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_setCurrentTime(ISVGSVGElement *iface, float v)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%f)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_getIntersectionList(ISVGSVGElement *iface, ISVGRect *rect,
        ISVGElement *reference_element, VARIANT *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p %p %p)\n", This, rect, reference_element, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_getEnclosureList(ISVGSVGElement *iface, ISVGRect *rect,
        ISVGElement *reference_element, VARIANT *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_checkIntersection(ISVGSVGElement *iface, ISVGElement *element,
        ISVGRect *rect, VARIANT_BOOL *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p %p %p)\n", This, element, rect, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_checkEnclosure(ISVGSVGElement *iface, ISVGElement *element,
        ISVGRect *rect, VARIANT_BOOL *p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p %p %p)\n", This, element, rect, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_deselectAll(ISVGSVGElement *iface)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_createSVGNumber(ISVGSVGElement *iface, ISVGNumber **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_createSVGLength(ISVGSVGElement *iface, ISVGLength **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_createSVGAngle(ISVGSVGElement *iface, ISVGAngle **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_createSVGPoint(ISVGSVGElement *iface, ISVGPoint **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_createSVGMatrix(ISVGSVGElement *iface, ISVGMatrix **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_createSVGRect(ISVGSVGElement *iface, ISVGRect **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_createSVGTransform(ISVGSVGElement *iface, ISVGTransform **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_createSVGTransformFromMatrix(ISVGSVGElement *iface,
        ISVGMatrix *matrix, ISVGTransform **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%p %p)\n", This, matrix, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGSVGElement_getElementById(ISVGSVGElement *iface, BSTR id, IHTMLElement **p)
{
    SVGSVGElement *This = impl_from_ISVGSVGElement(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(id), p);
    return E_NOTIMPL;
}

static const ISVGSVGElementVtbl SVGSVGElementVtbl = {
    SVGSVGElement_QueryInterface,
    SVGSVGElement_AddRef,
    SVGSVGElement_Release,
    SVGSVGElement_GetTypeInfoCount,
    SVGSVGElement_GetTypeInfo,
    SVGSVGElement_GetIDsOfNames,
    SVGSVGElement_Invoke,
    SVGSVGElement_putref_x,
    SVGSVGElement_get_x,
    SVGSVGElement_putref_y,
    SVGSVGElement_get_y,
    SVGSVGElement_putref_width,
    SVGSVGElement_get_width,
    SVGSVGElement_putref_height,
    SVGSVGElement_get_height,
    SVGSVGElement_put_contentScriptType,
    SVGSVGElement_get_contentScriptType,
    SVGSVGElement_put_contentStyleType,
    SVGSVGElement_get_contentStyleType,
    SVGSVGElement_putref_viewport,
    SVGSVGElement_get_viewport,
    SVGSVGElement_put_pixelUnitToMillimeterX,
    SVGSVGElement_get_pixelUnitToMillimeterX,
    SVGSVGElement_put_pixelUnitToMillimeterY,
    SVGSVGElement_get_pixelUnitToMillimeterY,
    SVGSVGElement_put_screenPixelToMillimeterX,
    SVGSVGElement_get_screenPixelToMillimeterX,
    SVGSVGElement_put_screenPixelToMillimeterY,
    SVGSVGElement_get_screenPixelToMillimeterY,
    SVGSVGElement_put_useCurrentView,
    SVGSVGElement_get_useCurrentView,
    SVGSVGElement_putref_currentView,
    SVGSVGElement_get_currentView,
    SVGSVGElement_put_currentScale,
    SVGSVGElement_get_currentScale,
    SVGSVGElement_putref_currentTranslate,
    SVGSVGElement_get_currentTranslate,
    SVGSVGElement_suspendRedraw,
    SVGSVGElement_unsuspendRedraw,
    SVGSVGElement_unsuspendRedrawAll,
    SVGSVGElement_forceRedraw,
    SVGSVGElement_pauseAnimations,
    SVGSVGElement_unpauseAnimations,
    SVGSVGElement_animationsPaused,
    SVGSVGElement_getCurrentTime,
    SVGSVGElement_setCurrentTime,
    SVGSVGElement_getIntersectionList,
    SVGSVGElement_getEnclosureList,
    SVGSVGElement_checkIntersection,
    SVGSVGElement_checkEnclosure,
    SVGSVGElement_deselectAll,
    SVGSVGElement_createSVGNumber,
    SVGSVGElement_createSVGLength,
    SVGSVGElement_createSVGAngle,
    SVGSVGElement_createSVGPoint,
    SVGSVGElement_createSVGMatrix,
    SVGSVGElement_createSVGRect,
    SVGSVGElement_createSVGTransform,
    SVGSVGElement_createSVGTransformFromMatrix,
    SVGSVGElement_getElementById
};

static inline SVGSVGElement *SVGSVGElement_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, SVGSVGElement, svg_element.element.node.event_target.dispex);
}

static void *SVGSVGElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    SVGSVGElement *This = SVGSVGElement_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_ISVGSVGElement, riid))
        return &This->ISVGSVGElement_iface;

    return SVGElement_query_interface(&This->svg_element.element.node.event_target.dispex, riid);
}

static const NodeImplVtbl SVGSVGElementImplVtbl = {
    .clsid                 = &CLSID_SVGSVGElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
};

static const event_target_vtbl_t SVGSVGElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= SVGSVGElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLElement_traverse,
        .unlink         = HTMLElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

dispex_static_data_t SVGSVGElement_dispex = {
    .id           = PROT_SVGSVGElement,
    .prototype_id = PROT_SVGElement,
    .vtbl         = &SVGSVGElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLUnknownElement_tid,
    .init_info    = HTMLElement_init_dispex_info,
};

static HRESULT create_viewport_element(HTMLDocumentNode *doc, nsIDOMSVGElement *nselem, HTMLElement **elem)
{
    SVGSVGElement *ret;

    ret = calloc(1, sizeof(SVGSVGElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->ISVGSVGElement_iface.lpVtbl = &SVGSVGElementVtbl;
    ret->svg_element.element.node.vtbl = &SVGSVGElementImplVtbl;

    init_svg_element(&ret->svg_element, doc, nselem, &SVGSVGElement_dispex);

    *elem = &ret->svg_element.element;
    return S_OK;
}

struct SVGCircleElement {
    SVGElement svg_element;
    ISVGCircleElement ISVGCircleElement_iface;
};

static inline SVGCircleElement *impl_from_ISVGCircleElement(ISVGCircleElement *iface)
{
    return CONTAINING_RECORD(iface, SVGCircleElement, ISVGCircleElement_iface);
}

DISPEX_IDISPATCH_IMPL(SVGCircleElement, ISVGCircleElement,
                      impl_from_ISVGCircleElement(iface)->svg_element.element.node.event_target.dispex)

static HRESULT WINAPI SVGCircleElement_putref_cx(ISVGCircleElement *iface, ISVGAnimatedLength *v)
{
    SVGCircleElement *This = impl_from_ISVGCircleElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGCircleElement_get_cx(ISVGCircleElement *iface, ISVGAnimatedLength **p)
{
    SVGCircleElement *This = impl_from_ISVGCircleElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGCircleElement_putref_cy(ISVGCircleElement *iface, ISVGAnimatedLength *v)
{
    SVGCircleElement *This = impl_from_ISVGCircleElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGCircleElement_get_cy(ISVGCircleElement *iface, ISVGAnimatedLength **p)
{
    SVGCircleElement *This = impl_from_ISVGCircleElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGCircleElement_putref_r(ISVGCircleElement *iface, ISVGAnimatedLength *v)
{
    SVGCircleElement *This = impl_from_ISVGCircleElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGCircleElement_get_r(ISVGCircleElement *iface, ISVGAnimatedLength **p)
{
    SVGCircleElement *This = impl_from_ISVGCircleElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const ISVGCircleElementVtbl SVGCircleElementVtbl = {
    SVGCircleElement_QueryInterface,
    SVGCircleElement_AddRef,
    SVGCircleElement_Release,
    SVGCircleElement_GetTypeInfoCount,
    SVGCircleElement_GetTypeInfo,
    SVGCircleElement_GetIDsOfNames,
    SVGCircleElement_Invoke,
    SVGCircleElement_putref_cx,
    SVGCircleElement_get_cx,
    SVGCircleElement_putref_cy,
    SVGCircleElement_get_cy,
    SVGCircleElement_putref_r,
    SVGCircleElement_get_r
};

static inline SVGCircleElement *SVGCircleElement_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, SVGCircleElement, svg_element.element.node.event_target.dispex);
}

static void *SVGCircleElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    SVGCircleElement *This = SVGCircleElement_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_ISVGCircleElement, riid))
        return &This->ISVGCircleElement_iface;

    return SVGElement_query_interface(&This->svg_element.element.node.event_target.dispex, riid);
}

static const NodeImplVtbl SVGCircleElementImplVtbl = {
    .clsid                 = &CLSID_SVGCircleElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
};

static const event_target_vtbl_t SVGCircleElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= SVGCircleElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLElement_traverse,
        .unlink         = HTMLElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

dispex_static_data_t SVGCircleElement_dispex = {
    .id           = PROT_SVGCircleElement,
    .prototype_id = PROT_SVGElement,
    .vtbl         = &SVGCircleElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLUnknownElement_tid,
    .init_info    = HTMLElement_init_dispex_info,
};

static HRESULT create_circle_element(HTMLDocumentNode *doc, nsIDOMSVGElement *nselem, HTMLElement **elem)
{
    SVGCircleElement *ret;

    ret = calloc(1, sizeof(SVGCircleElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->ISVGCircleElement_iface.lpVtbl = &SVGCircleElementVtbl;
    ret->svg_element.element.node.vtbl = &SVGCircleElementImplVtbl;

    init_svg_element(&ret->svg_element, doc, nselem, &SVGCircleElement_dispex);

    *elem = &ret->svg_element.element;
    return S_OK;
}

typedef struct {
    ISVGTextContentElement ISVGTextContentElement_iface;
    SVGElement *svg_element;
} SVGTextContentElement;

static inline SVGTextContentElement *impl_from_ISVGTextContentElement(ISVGTextContentElement *iface)
{
    return CONTAINING_RECORD(iface, SVGTextContentElement, ISVGTextContentElement_iface);
}

DISPEX_IDISPATCH_IMPL(SVGTextContentElement, ISVGTextContentElement,
                      impl_from_ISVGTextContentElement(iface)->svg_element->element.node.event_target.dispex)

static HRESULT WINAPI SVGTextContentElement_putref_textLength(ISVGTextContentElement *iface, ISVGAnimatedLength *v)
{
    SVGTextContentElement *This = impl_from_ISVGTextContentElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGTextContentElement_get_textLength(ISVGTextContentElement *iface, ISVGAnimatedLength **p)
{
    SVGTextContentElement *This = impl_from_ISVGTextContentElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGTextContentElement_putref_lengthAdjust(ISVGTextContentElement *iface, ISVGAnimatedEnumeration *v)
{
    SVGTextContentElement *This = impl_from_ISVGTextContentElement(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGTextContentElement_get_lengthAdjust(ISVGTextContentElement *iface, ISVGAnimatedEnumeration **p)
{
    SVGTextContentElement *This = impl_from_ISVGTextContentElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGTextContentElement_getNumberOfChars(ISVGTextContentElement *iface, LONG *p)
{
    SVGTextContentElement *This = impl_from_ISVGTextContentElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGTextContentElement_getComputedTextLength(ISVGTextContentElement *iface, float *p)
{
    SVGTextContentElement *This = impl_from_ISVGTextContentElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGTextContentElement_getSubStringLength(ISVGTextContentElement *iface,
                                                               LONG charnum, LONG nchars, float *p)
{
    SVGTextContentElement *This = impl_from_ISVGTextContentElement(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, charnum, nchars, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGTextContentElement_getStartPositionOfChar(ISVGTextContentElement *iface,
                                                                   LONG charnum, ISVGPoint **p)
{
    SVGTextContentElement *This = impl_from_ISVGTextContentElement(iface);
    FIXME("(%p)->(%ld %p)\n", This, charnum, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGTextContentElement_getEndPositionOfChar(ISVGTextContentElement *iface,
                                                                 LONG charnum, ISVGPoint **p)
{
    SVGTextContentElement *This = impl_from_ISVGTextContentElement(iface);
    FIXME("(%p)->(%ld %p)\n", This, charnum, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGTextContentElement_getExtentOfChar(ISVGTextContentElement *iface,
                                                            LONG charnum, ISVGRect **p)
{
    SVGTextContentElement *This = impl_from_ISVGTextContentElement(iface);
    FIXME("(%p)->(%ld %p)\n", This, charnum, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGTextContentElement_getRotationOfChar(ISVGTextContentElement *iface,
                                                              LONG charnum, float *p)
{
    SVGTextContentElement *This = impl_from_ISVGTextContentElement(iface);
    FIXME("(%p)->(%ld %p)\n", This, charnum, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGTextContentElement_getCharNumAtPosition(ISVGTextContentElement *iface,
                                                                 ISVGPoint *point, LONG *p)
{
    SVGTextContentElement *This = impl_from_ISVGTextContentElement(iface);
    FIXME("(%p)->(%p %p)\n", This, point, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI SVGTextContentElement_selectSubString(ISVGTextContentElement *iface,
                                                            LONG charnum, LONG nchars)
{
    SVGTextContentElement *This = impl_from_ISVGTextContentElement(iface);
    FIXME("(%p)->(%ld %ld)\n", This, charnum, nchars);
    return E_NOTIMPL;
}

static const ISVGTextContentElementVtbl SVGTextContentElementVtbl = {
    SVGTextContentElement_QueryInterface,
    SVGTextContentElement_AddRef,
    SVGTextContentElement_Release,
    SVGTextContentElement_GetTypeInfoCount,
    SVGTextContentElement_GetTypeInfo,
    SVGTextContentElement_GetIDsOfNames,
    SVGTextContentElement_Invoke,
    SVGTextContentElement_putref_textLength,
    SVGTextContentElement_get_textLength,
    SVGTextContentElement_putref_lengthAdjust,
    SVGTextContentElement_get_lengthAdjust,
    SVGTextContentElement_getNumberOfChars,
    SVGTextContentElement_getComputedTextLength,
    SVGTextContentElement_getSubStringLength,
    SVGTextContentElement_getStartPositionOfChar,
    SVGTextContentElement_getEndPositionOfChar,
    SVGTextContentElement_getExtentOfChar,
    SVGTextContentElement_getRotationOfChar,
    SVGTextContentElement_getCharNumAtPosition,
    SVGTextContentElement_selectSubString
};

static void init_text_content_element(SVGTextContentElement *text_content, SVGElement *svg_element)
{
    text_content->ISVGTextContentElement_iface.lpVtbl = &SVGTextContentElementVtbl;
    text_content->svg_element = svg_element;
}

struct SVGTSpanElement {
    SVGElement svg_element;
    SVGTextContentElement text_content;
};

static inline SVGTSpanElement *SVGTSpanElement_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, SVGTSpanElement, svg_element.element.node.event_target.dispex);
}

static void *SVGTSpanElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    SVGTSpanElement *This = SVGTSpanElement_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_ISVGTSpanElement, riid))
        return &This->svg_element.ISVGElement_iface; /* no additional methods */
    if(IsEqualGUID(&IID_ISVGTextContentElement, riid))
        return &This->text_content.ISVGTextContentElement_iface;

    return SVGElement_query_interface(&This->svg_element.element.node.event_target.dispex, riid);
}

dispex_static_data_t SVGTextContentElement_dispex = {
    .id           = PROT_SVGTextContentElement,
    .prototype_id = PROT_SVGElement,
};

dispex_static_data_t SVGTextPositioningElement_dispex = {
    .id           = PROT_SVGTextPositioningElement,
    .prototype_id = PROT_SVGTextContentElement,
};

static const NodeImplVtbl SVGTSpanElementImplVtbl = {
    .clsid                 = &CLSID_SVGTSpanElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
};

static const event_target_vtbl_t SVGTSpanElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= SVGTSpanElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLElement_traverse,
        .unlink         = HTMLElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

dispex_static_data_t SVGTSpanElement_dispex = {
    .id           = PROT_SVGTSpanElement,
    .prototype_id = PROT_SVGTextPositioningElement,
    .vtbl         = &SVGTSpanElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLUnknownElement_tid,
    .init_info    = HTMLElement_init_dispex_info,
};

static HRESULT create_tspan_element(HTMLDocumentNode *doc, nsIDOMSVGElement *nselem, HTMLElement **elem)
{
    SVGTSpanElement *ret;

    ret = calloc(1, sizeof(SVGTSpanElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->svg_element.element.node.vtbl = &SVGTSpanElementImplVtbl;
    init_text_content_element(&ret->text_content, &ret->svg_element);
    init_svg_element(&ret->svg_element, doc, nselem, &SVGTSpanElement_dispex);

    *elem = &ret->svg_element.element;
    return S_OK;
}

HRESULT create_svg_element(HTMLDocumentNode *doc, nsIDOMSVGElement *dom_element, const WCHAR *tag_name, HTMLElement **elem)
{
    SVGElement *svg_element;

    TRACE("%s\n", debugstr_w(tag_name));

    if(!wcscmp(tag_name, L"svg"))
        return create_viewport_element(doc, dom_element, elem);
    if(!wcscmp(tag_name, L"circle"))
        return create_circle_element(doc, dom_element, elem);
    if(!wcscmp(tag_name, L"tspan"))
        return create_tspan_element(doc, dom_element, elem);

    svg_element = calloc(1, sizeof(*svg_element));
    if(!svg_element)
        return E_OUTOFMEMORY;

    svg_element->element.node.vtbl = &SVGElementImplVtbl;
    init_svg_element(svg_element, doc, dom_element, &SVGElement_dispex);
    *elem = &svg_element->element;
    return S_OK;
}
