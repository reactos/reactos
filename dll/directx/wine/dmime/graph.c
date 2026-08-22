/* IDirectMusicGraph
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dmime_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

struct tool_entry
{
    struct list entry;
    IDirectMusicTool *tool;
    DWORD delivery;
};

struct graph
{
    IDirectMusicGraph IDirectMusicGraph_iface;
    struct dmobject dmobj;
    LONG ref;

    struct list tools;
};

static inline struct graph *impl_from_IDirectMusicGraph(IDirectMusicGraph *iface)
{
    return CONTAINING_RECORD(iface, struct graph, IDirectMusicGraph_iface);
}

static inline struct graph *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct graph, dmobj.IPersistStream_iface);
}

static HRESULT WINAPI graph_QueryInterface(IDirectMusicGraph *iface, REFIID riid, void **ret_iface)
{
    struct graph *This = impl_from_IDirectMusicGraph(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectMusicGraph))
    {
        *ret_iface = &This->IDirectMusicGraph_iface;
    }
    else if (IsEqualIID(riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;

    if (*ret_iface)
    {
        IDirectMusicGraph_AddRef(iface);
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI graph_AddRef(IDirectMusicGraph *iface)
{
    struct graph *This = impl_from_IDirectMusicGraph(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): %ld\n", This, ref);

    return ref;
}

static ULONG WINAPI graph_Release(IDirectMusicGraph *iface)
{
    struct graph *This = impl_from_IDirectMusicGraph(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): %ld\n", This, ref);

    if (!ref)
    {
        struct tool_entry *entry, *next;

        LIST_FOR_EACH_ENTRY_SAFE(entry, next, &This->tools, struct tool_entry, entry)
        {
            list_remove(&entry->entry);
            IDirectMusicTool_Release(entry->tool);
            free(entry);
        }

        free(This);
    }

    return ref;
}

static HRESULT WINAPI graph_StampPMsg(IDirectMusicGraph *iface, DMUS_PMSG *msg)
{
    const DWORD delivery_flags = DMUS_PMSGF_TOOL_IMMEDIATE | DMUS_PMSGF_TOOL_QUEUE | DMUS_PMSGF_TOOL_ATTIME;
    struct graph *This = impl_from_IDirectMusicGraph(iface);
    struct tool_entry *entry, *next, *first;

    TRACE("(%p, %p)\n", This, msg);

    if (!msg) return E_POINTER;

    first = LIST_ENTRY(This->tools.next, struct tool_entry, entry);
    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &This->tools, struct tool_entry, entry)
        if (entry->tool == msg->pTool) break;
    if (&entry->entry == &This->tools) next = first;

    if (msg->pTool)
    {
        IDirectMusicTool_Release(msg->pTool);
        msg->pTool = NULL;
    }

    if (&next->entry == &This->tools) return DMUS_S_LAST_TOOL;

    if (!msg->pGraph)
    {
        msg->pGraph = iface;
        IDirectMusicGraph_AddRef(msg->pGraph);
    }

    msg->pTool = next->tool;
    IDirectMusicTool_AddRef(msg->pTool);

    msg->dwFlags &= ~delivery_flags;
    msg->dwFlags |= next->delivery;

    return S_OK;
}

static HRESULT WINAPI graph_InsertTool(IDirectMusicGraph *iface, IDirectMusicTool *tool,
        DWORD *channels, DWORD channel_count, LONG index)
{
    struct graph *This = impl_from_IDirectMusicGraph(iface);
    struct tool_entry *entry, *next;
    HRESULT hr;

    TRACE("(%p, %p, %p, %ld, %li)\n", This, tool, channels, channel_count, index);

    if (!tool) return E_POINTER;

    LIST_FOR_EACH_ENTRY(next, &This->tools, struct tool_entry, entry)
    {
        if (next->tool == tool) return DMUS_E_ALREADY_EXISTS;
        if (index-- <= 0) break;
    }

    if (!(entry = calloc(1, sizeof(*entry)))) return E_OUTOFMEMORY;
    entry->tool = tool;
    IDirectMusicTool_AddRef(tool);
    IDirectMusicTool_Init(tool, iface);
    if (FAILED(hr = IDirectMusicTool_GetMsgDeliveryType(tool, &entry->delivery)))
    {
        WARN("Failed to get delivery type from tool %p, hr %#lx\n", tool, hr);
        entry->delivery = DMUS_PMSGF_TOOL_IMMEDIATE;
    }
    list_add_before(&next->entry, &entry->entry);

    return S_OK;
}

static HRESULT WINAPI graph_GetTool(IDirectMusicGraph *iface, DWORD index, IDirectMusicTool **ret_tool)
{
    struct graph *This = impl_from_IDirectMusicGraph(iface);
    struct tool_entry *entry;

    TRACE("(%p, %ld, %p)\n", This, index, ret_tool);

    if (!ret_tool) return E_POINTER;

    LIST_FOR_EACH_ENTRY(entry, &This->tools, struct tool_entry, entry)
    {
        if (!index--)
        {
            *ret_tool = entry->tool;
            IDirectMusicTool_AddRef(entry->tool);
            return S_OK;
        }
    }

    return DMUS_E_NOT_FOUND;
}

static HRESULT WINAPI graph_RemoveTool(IDirectMusicGraph *iface, IDirectMusicTool *tool)
{
    struct graph *This = impl_from_IDirectMusicGraph(iface);
    struct tool_entry *entry;

    TRACE("(%p, %p)\n", This, tool);

    if (!tool) return E_POINTER;

    LIST_FOR_EACH_ENTRY(entry, &This->tools, struct tool_entry, entry)
    {
        if (entry->tool == tool)
        {
            list_remove(&entry->entry);
            IDirectMusicTool_Release(entry->tool);
            free(entry);
            return S_OK;
        }
    }

    return DMUS_E_NOT_FOUND;
}

static const IDirectMusicGraphVtbl graph_vtbl =
{
    graph_QueryInterface,
    graph_AddRef,
    graph_Release,
    graph_StampPMsg,
    graph_InsertTool,
    graph_GetTool,
    graph_RemoveTool,
};

static HRESULT WINAPI graph_IDirectMusicObject_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream)
        return E_POINTER;
    if (!desc || desc->dwSize != sizeof(*desc))
        return E_INVALIDARG;

    if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || riff.type != DMUS_FOURCC_TOOLGRAPH_FORM) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_CHUNKNOTFOUND;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc,
            DMUS_OBJ_OBJECT|DMUS_OBJ_NAME|DMUS_OBJ_CATEGORY|DMUS_OBJ_VERSION);
    if (FAILED(hr))
        return hr;

    desc->guidClass = CLSID_DirectMusicGraph;
    desc->dwValidData |= DMUS_OBJ_CLASS;

    dump_DMUS_OBJECTDESC(desc);
    return S_OK;
}

static const IDirectMusicObjectVtbl dmobject_vtbl = {
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    graph_IDirectMusicObject_ParseDescriptor
};

static HRESULT WINAPI graph_IPersistStream_Load(IPersistStream *iface, IStream *stream)
{
    struct graph *This = impl_from_IPersistStream(iface);

    FIXME("(%p, %p): Loading not implemented yet\n", This, stream);

    return IDirectMusicObject_ParseDescriptor(&This->dmobj.IDirectMusicObject_iface, stream,
            &This->dmobj.desc);
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    dmobj_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    graph_IPersistStream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT create_dmgraph(REFIID riid, void **ret_iface)
{
    struct graph *obj;
    HRESULT hr;

    *ret_iface = NULL;
    if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
    obj->IDirectMusicGraph_iface.lpVtbl = &graph_vtbl;
    obj->ref = 1;
    dmobject_init(&obj->dmobj, &CLSID_DirectMusicGraph, (IUnknown *)&obj->IDirectMusicGraph_iface);
    obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
    obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;
    list_init(&obj->tools);

    hr = IDirectMusicGraph_QueryInterface(&obj->IDirectMusicGraph_iface, riid, ret_iface);
    IDirectMusicGraph_Release(&obj->IDirectMusicGraph_iface);

    return hr;
}
