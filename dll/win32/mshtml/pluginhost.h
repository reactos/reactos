/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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

#pragma once

typedef struct HTMLPluginContainer HTMLPluginContainer;
typedef struct PHEventSink PHEventSink;

typedef struct {
    IOleClientSite       IOleClientSite_iface;
    IAdviseSinkEx        IAdviseSinkEx_iface;
    IPropertyNotifySink  IPropertyNotifySink_iface;
    IDispatch            IDispatch_iface;
    IOleInPlaceSiteEx    IOleInPlaceSiteEx_iface;
    IOleControlSite      IOleControlSite_iface;
    IBindHost            IBindHost_iface;
    IServiceProvider     IServiceProvider_iface;

    LONG ref;

    IUnknown *plugin_unk;
    IOleInPlaceObject *ip_object;
    CLSID clsid;

    IDispatch *disp;

    HWND hwnd;
    RECT rect;
    BOOL ui_active;

    HTMLDocumentNode *doc;
    struct list entry;

    PHEventSink *sink;
    HTMLPluginContainer *element;
} PluginHost;

struct HTMLPluginContainer {
    HTMLElement element;

    PluginHost *plugin_host;

    DISPID *props;
    DWORD props_size;
    DWORD props_len;
};

DEFINE_GUID(IID_HTMLPluginContainer, 0xbd7a6050,0xb373,0x4f6f,0xa4,0x93,0xdd,0x40,0xc5,0x23,0xa8,0x6a);

extern const IID IID_HTMLPluginContainer DECLSPEC_HIDDEN;

HRESULT create_plugin_host(HTMLDocumentNode*,HTMLPluginContainer*) DECLSPEC_HIDDEN;
void update_plugin_window(PluginHost*,HWND,const RECT*) DECLSPEC_HIDDEN;
void detach_plugin_host(PluginHost*) DECLSPEC_HIDDEN;

HRESULT create_param_prop_bag(nsIDOMHTMLElement*,IPropertyBag**) DECLSPEC_HIDDEN;

HRESULT create_ip_window(IOleInPlaceUIWindow**) DECLSPEC_HIDDEN;
HRESULT create_ip_frame(IOleInPlaceFrame**) DECLSPEC_HIDDEN;

HRESULT get_plugin_disp(HTMLPluginContainer*,IDispatch**) DECLSPEC_HIDDEN;
HRESULT get_plugin_dispid(HTMLPluginContainer*,WCHAR*,DISPID*) DECLSPEC_HIDDEN;
HRESULT invoke_plugin_prop(HTMLPluginContainer*,DISPID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*) DECLSPEC_HIDDEN;
void notif_container_change(HTMLPluginContainer*,DISPID) DECLSPEC_HIDDEN;
void bind_activex_event(HTMLDocumentNode*,HTMLPluginContainer*,WCHAR*,IDispatch*) DECLSPEC_HIDDEN;
