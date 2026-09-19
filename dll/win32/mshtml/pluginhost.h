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

struct plugin_prop
{
    DISPID id;
    WCHAR name[1];
};

struct HTMLPluginContainer {
    HTMLElement element;

    PluginHost *plugin_host;

    struct plugin_prop **props;
    DWORD props_size;
    DWORD props_len;
};

DEFINE_GUID(IID_HTMLPluginContainer, 0xbd7a6050,0xb373,0x4f6f,0xa4,0x93,0xdd,0x40,0xc5,0x23,0xa8,0x6a);

extern const IID IID_HTMLPluginContainer;

HRESULT create_plugin_host(HTMLDocumentNode*,HTMLPluginContainer*);
void update_plugin_window(PluginHost*,HWND,const RECT*);
void detach_plugin_host(PluginHost*);

HRESULT get_plugin_disp(HTMLPluginContainer*,IDispatch**);
void notif_container_change(HTMLPluginContainer*,DISPID);
void bind_activex_event(HTMLDocumentNode*,HTMLPluginContainer*,WCHAR*,IDispatch*);

void HTMLPluginContainer_destructor(DispatchEx *dispex);
HRESULT HTMLPluginContainer_get_dispid(DispatchEx *dispex, const WCHAR *name, DWORD grfdex, DISPID *dispid);
HRESULT HTMLPluginContainer_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
                                   VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller);
HRESULT HTMLPluginContainer_get_prop_desc(DispatchEx *dispex, DISPID id, struct property_info *desc);
