/*
 * Copyright 2016 Michael Müller
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

#define COBJMACROS



#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "rpcproxy.h"
#include "inseng.h"

#include "inseng_private.h"

#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(inseng);

#define DEFAULT_INSTALLER_DESC "Active Setup Installation"

struct cifgroup
{
    ICifGroup ICifGroup_iface;

    struct list entry;

    ICifFile *parent;

    char *id;
    char *description;
    DWORD priority;
};

struct ciffenum_components
{
    IEnumCifComponents IEnumCifComponents_iface;
    LONG ref;

    ICifFile *file;
    struct list *start;
    struct list *position;

    char *group_id;
};

struct ciffenum_groups
{
    IEnumCifGroups IEnumCifGroups_iface;
    LONG ref;

    ICifFile *file;
    struct list *start;
    struct list *position;
};

struct url_info
{
    struct list entry;
    INT index;
    char *url;
    DWORD flags;
};

struct dependency_info
{
    struct list entry;
    char *id;
    char *type;
};

struct cifcomponent
{
    ICifComponent ICifComponent_iface;

    struct list entry;

    ICifFile *parent;

    char *id;
    char *guid;
    char *description;
    char *details;
    char *group;


    DWORD version;
    DWORD build;
    char *patchid;

    char *locale;
    char *key_uninstall;

    DWORD size_win;
    DWORD size_app;
    DWORD size_download;
    DWORD size_extracted;

    char *key_success;
    char *key_progress;
    char *key_cancel;

    DWORD as_aware;
    DWORD reboot;
    DWORD admin;
    DWORD visibleui;

    DWORD priority;
    DWORD platform;

    struct list dependencies;
    struct list urls;

    /* mode */
    /* det version */
    /* one component */
    /* custom data */

    /* in memory state */
    DWORD queue_state;
    DWORD current_priority;
    DWORD size_actual_download;
    BOOL downloaded;
    BOOL installed;
};

struct ciffile
{
    ICifFile ICifFile_iface;
    LONG ref;

    struct list components;
    struct list groups;

    char *name;
};

static inline struct ciffile *impl_from_ICiffile(ICifFile *iface)
{
    return CONTAINING_RECORD(iface, struct ciffile, ICifFile_iface);
}

static inline struct cifcomponent *impl_from_ICifComponent(ICifComponent *iface)
{
    return CONTAINING_RECORD(iface, struct cifcomponent, ICifComponent_iface);
}

static inline struct cifgroup *impl_from_ICifGroup(ICifGroup *iface)
{
    return CONTAINING_RECORD(iface, struct cifgroup, ICifGroup_iface);
}

static inline struct ciffenum_components *impl_from_IEnumCifComponents(IEnumCifComponents *iface)
{
    return CONTAINING_RECORD(iface, struct ciffenum_components, IEnumCifComponents_iface);
}

static inline struct ciffenum_groups *impl_from_IEnumCifGroups(IEnumCifGroups *iface)
{
    return CONTAINING_RECORD(iface, struct ciffenum_groups, IEnumCifGroups_iface);
}

static HRESULT enum_components_create(ICifFile *file, struct list *start, char *group_id, IEnumCifComponents **iface);

static HRESULT copy_substring_null(char *dest, int max_len, char *src)
{
    if (!src)
        return E_FAIL;

    if (max_len <= 0)
        return S_OK;

    if (!dest)
        return E_FAIL;

    while (*src && max_len-- > 1)
        *dest++ = *src++;
    *dest = 0;

    return S_OK;
}

static void url_entry_free(struct url_info *url)
{
    heap_free(url->url);
    heap_free(url);
}

static void dependency_entry_free(struct dependency_info *dependency)
{
    heap_free(dependency->id);
    heap_free(dependency);
}

static void component_free(struct cifcomponent *comp)
{
    struct dependency_info *dependency, *dependency_next;
    struct url_info *url, *url_next;

    heap_free(comp->id);
    heap_free(comp->guid);
    heap_free(comp->description);
    heap_free(comp->details);
    heap_free(comp->group);

    heap_free(comp->patchid);

    heap_free(comp->locale);
    heap_free(comp->key_uninstall);

    heap_free(comp->key_success);
    heap_free(comp->key_progress);
    heap_free(comp->key_cancel);

    LIST_FOR_EACH_ENTRY_SAFE(dependency, dependency_next, &comp->dependencies, struct dependency_info, entry)
    {
        list_remove(&dependency->entry);
        dependency_entry_free(dependency);
    }

    LIST_FOR_EACH_ENTRY_SAFE(url, url_next, &comp->urls, struct url_info, entry)
    {
        list_remove(&url->entry);
        url_entry_free(url);
    }

    heap_free(comp);
}

static void group_free(struct cifgroup *group)
{
    heap_free(group->id);
    heap_free(group->description);
    heap_free(group);
}

static HRESULT WINAPI group_GetID(ICifGroup *iface, char *id, DWORD size)
{
    struct cifgroup *This = impl_from_ICifGroup(iface);

    TRACE("(%p)->(%p, %u)\n", This, id, size);

    return copy_substring_null(id, size, This->id);
}

static HRESULT WINAPI group_GetDescription(ICifGroup *iface, char *desc, DWORD size)
{
    struct cifgroup *This = impl_from_ICifGroup(iface);

    TRACE("(%p)->(%p, %u)\n", This, desc, size);

    return copy_substring_null(desc, size, This->description);
}

static DWORD WINAPI group_GetPriority(ICifGroup *iface)
{
    struct cifgroup *This = impl_from_ICifGroup(iface);

    TRACE("(%p)\n", This);

    return This->priority;
}

static HRESULT WINAPI group_EnumComponents(ICifGroup *iface, IEnumCifComponents **enum_components, DWORD filter, LPVOID pv)
{
    struct cifgroup *This = impl_from_ICifGroup(iface);
    struct ciffile *file;

    TRACE("(%p)->(%p, %u, %p)\n", This, enum_components, filter, pv);

    if (filter)
        FIXME("filter (%x) not supported\n", filter);
    if (pv)
        FIXME("how to handle pv (%p)?\n", pv);

    file = impl_from_ICiffile(This->parent);
    return enum_components_create(This->parent, &file->components, This->id, enum_components);
}

static DWORD WINAPI group_GetCurrentPriority(ICifGroup *iface)
{
    struct cifgroup *This = impl_from_ICifGroup(iface);

    FIXME("(%p): stub\n", This);

    return 0;
}

static const ICifGroupVtbl cifgroupVtbl =
{
    group_GetID,
    group_GetDescription,
    group_GetPriority,
    group_EnumComponents,
    group_GetCurrentPriority,
};

void component_set_actual_download_size(ICifComponent *iface, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    This->size_actual_download = size;
}

void component_set_downloaded(ICifComponent *iface, BOOL value)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    This->downloaded = value;
}

void component_set_installed(ICifComponent *iface, BOOL value)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    This->installed = value;
}

char *component_get_id(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    return This->id;
}

static HRESULT WINAPI component_GetID(ICifComponent *iface, char *id, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)->(%p, %u)\n", This, id, size);

    return copy_substring_null(id, size, This->id);
}

static HRESULT WINAPI component_GetGUID(ICifComponent *iface, char *guid, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)->(%p, %u)\n", This, guid, size);

    return copy_substring_null(guid, size, This->guid);
}

static HRESULT WINAPI component_GetDescription(ICifComponent *iface, char *desc, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)->(%p, %u)\n", This, desc, size);

    return copy_substring_null(desc, size, This->description);
}

static HRESULT WINAPI component_GetDetails(ICifComponent *iface, char *details, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)->(%p, %u)\n", This, details, size);

    return copy_substring_null(details, size, This->details);
}

static HRESULT WINAPI component_GetUrl(ICifComponent *iface, UINT index, char *url, DWORD size, DWORD *flags)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);
    struct url_info *entry;

    TRACE("(%p)->(%u, %p, %u, %p)\n", This, index, url, size, flags);

    /* FIXME: check how functions behaves for url == NULL */

    if (!flags)
        return E_FAIL;

    LIST_FOR_EACH_ENTRY(entry, &This->urls, struct url_info, entry)
    {
        if (entry->index != index)
            continue;

        *flags = entry->flags;
        return copy_substring_null(url, size, entry->url);
    }

    return E_FAIL;
}

static HRESULT WINAPI component_GetFileExtractList(ICifComponent *iface, UINT index, char *list, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    FIXME("(%p)->(%u, %p, %u): stub\n", This, index, list, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI component_GetUrlCheckRange(ICifComponent *iface, UINT index, DWORD *min, DWORD *max)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    FIXME("(%p)->(%u, %p, %p): stub\n", This, index, min, max);

    return E_NOTIMPL;
}

static HRESULT WINAPI component_GetCommand(ICifComponent *iface, UINT index, char *cmd, DWORD cmd_size, char *switches, DWORD switch_size, DWORD *type)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    FIXME("(%p)->(%u, %p, %u, %p, %u, %p): stub\n", This, index, cmd, cmd_size, switches, switch_size, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI component_GetVersion(ICifComponent *iface, DWORD *version, DWORD *build)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)->(%p, %p)\n", This, version, build);

    if (!version || !build)
        return E_FAIL;

    *version = This->version;
    *build = This->build;

    return S_OK;
}

static HRESULT WINAPI component_GetLocale(ICifComponent *iface, char *locale, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)->(%p, %u)\n", This, locale, size);

    return copy_substring_null(locale, size, This->locale);
}

static HRESULT WINAPI component_GetUninstallKey(ICifComponent *iface, char *key, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)->(%p, %u)\n", This, key, size);

    return copy_substring_null(key, size, This->key_uninstall);
}

static HRESULT WINAPI component_GetInstalledSize(ICifComponent *iface, DWORD *win, DWORD *app)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)->(%p, %p)\n", This, win, app);

    if (!win || !app)
        return E_FAIL;

    *win = This->size_win;
    *app = This->size_app;

    return S_OK;
}

static DWORD WINAPI component_GetDownloadSize(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)\n", This);

    return This->size_download;
}

static DWORD WINAPI component_GetExtractSize(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)\n", This);

    return This->size_extracted;
}

static HRESULT WINAPI component_GetSuccessKey(ICifComponent *iface, char *key, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)->(%p, %u)\n", This, key, size);

    return copy_substring_null(key, size, This->key_success);
}

static HRESULT WINAPI component_GetProgressKeys(ICifComponent *iface, char *progress, DWORD progress_size,
                                                char *cancel, DWORD cancel_size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);
    HRESULT hr;

    TRACE("(%p)->(%p, %u, %p, %u): semi-stub\n", This, progress, progress_size, cancel, cancel_size);

    hr = copy_substring_null(progress, progress_size, This->key_progress);
    if (hr != S_OK) return hr;

    if (cancel_size > 0 && cancel)
        *cancel = 0;

    return S_OK;
}

static HRESULT WINAPI component_IsActiveSetupAware(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)\n", This);

    return This->as_aware ? S_OK : S_FALSE;
}

static HRESULT WINAPI component_IsRebootRequired(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)\n", This);

    return This->reboot ? S_OK : S_FALSE;
}

static HRESULT WINAPI component_RequiresAdminRights(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)\n", This);

    return This->admin ? S_OK : S_FALSE;
}

static DWORD WINAPI component_GetPriority(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)\n", This);

    return This->priority;
}

static HRESULT WINAPI component_GetDependency(ICifComponent *iface, UINT index, char *id, DWORD id_size, char *type, DWORD *ver, DWORD *build)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);
    struct dependency_info *entry;
    ICifComponent *dependency;
    int pos = 0;

    TRACE("(%p)->(%u, %p, %u, %p, %p, %p)\n", This, index, id, id_size, type, ver, build);

    if (!id || !ver || !build)
        return E_FAIL;

    LIST_FOR_EACH_ENTRY(entry, &This->dependencies, struct dependency_info, entry)
    {
        if (pos++ < index)
            continue;

        if (ICifFile_FindComponent(This->parent, entry->id, &dependency) == S_OK)
        {
            ICifComponent_GetVersion(dependency, ver, build);
        }
        else
        {
            *ver = -1;
            *build = -1;
        }

        if (entry->type)
            *type = *entry->type;
        else
            *type = 'I';

        return copy_substring_null(id, id_size, entry->id);
    }

    return E_FAIL;
}

static DWORD WINAPI component_GetPlatform(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)\n", This);

    return This->platform;
}

static HRESULT WINAPI component_GetMode(ICifComponent *iface, UINT index, char *mode, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    FIXME("(%p)->(%u, %p, %u): stub\n", This, index, mode, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI component_GetGroup(ICifComponent *iface, char *id, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)->(%p, %u)\n", This, id, size);

    return copy_substring_null(id, size, This->group);
}

static HRESULT WINAPI component_IsUIVisible(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)\n", This);

    return This->visibleui ? S_OK : S_FALSE;
}

static HRESULT WINAPI component_GetPatchID(ICifComponent *iface, char *id, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)->(%p, %u)\n", This, id, size);

    return copy_substring_null(id, size, This->patchid);
}

static HRESULT WINAPI component_GetDetVersion(ICifComponent *iface, char *dll, DWORD dll_size, char *entry, DWORD entry_size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    FIXME("(%p)->(%p, %u, %p, %u): stub\n", This, dll, dll_size, entry, entry_size);

    return E_NOTIMPL;
}

static HRESULT WINAPI component_GetTreatAsOneComponents(ICifComponent *iface, UINT index, char *id, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    FIXME("(%p)->(%u, %p, %u): stub\n", This, index, id, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI component_GetCustomData(ICifComponent *iface, char *key, char *data, DWORD size)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    FIXME("(%p)->(%s, %p, %u): stub\n", This, debugstr_a(key), data, size);

    return E_NOTIMPL;
}

static DWORD WINAPI component_IsComponentInstalled(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)\n", This);

    return This->installed;
}

static HRESULT WINAPI component_IsComponentDownloaded(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)\n", This);

    return This->downloaded ? S_OK : S_FALSE;
}

static DWORD WINAPI component_IsThisVersionInstalled(ICifComponent *iface, DWORD version, DWORD build, DWORD *ret_version, DWORD *ret_build)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    FIXME("(%p)->(%u, %u, %p, %p): stub\n", This, version, build, ret_version, ret_build);

    return 0;
}

static DWORD WINAPI component_GetInstallQueueState(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)\n", This);

    return This->queue_state;
}

static HRESULT WINAPI component_SetInstallQueueState(ICifComponent *iface, DWORD state)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)->(%u)\n", This, state);

    This->queue_state = state;
    return S_OK;
}

static DWORD WINAPI component_GetActualDownloadSize(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)\n", This);

    return This->size_download;
}

static DWORD WINAPI component_GetCurrentPriority(ICifComponent *iface)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)\n", This);

    return This->current_priority;
}


static HRESULT WINAPI component_SetCurrentPriority(ICifComponent *iface, DWORD priority)
{
    struct cifcomponent *This = impl_from_ICifComponent(iface);

    TRACE("(%p)->(%u)\n", This, priority);

    This->current_priority = priority;
    return S_OK;
}

static const ICifComponentVtbl cifcomponentVtbl =
{
    component_GetID,
    component_GetGUID,
    component_GetDescription,
    component_GetDetails,
    component_GetUrl,
    component_GetFileExtractList,
    component_GetUrlCheckRange,
    component_GetCommand,
    component_GetVersion,
    component_GetLocale,
    component_GetUninstallKey,
    component_GetInstalledSize,
    component_GetDownloadSize,
    component_GetExtractSize,
    component_GetSuccessKey,
    component_GetProgressKeys,
    component_IsActiveSetupAware,
    component_IsRebootRequired,
    component_RequiresAdminRights,
    component_GetPriority,
    component_GetDependency,
    component_GetPlatform,
    component_GetMode,
    component_GetGroup,
    component_IsUIVisible,
    component_GetPatchID,
    component_GetDetVersion,
    component_GetTreatAsOneComponents,
    component_GetCustomData,
    component_IsComponentInstalled,
    component_IsComponentDownloaded,
    component_IsThisVersionInstalled,
    component_GetInstallQueueState,
    component_SetInstallQueueState,
    component_GetActualDownloadSize,
    component_GetCurrentPriority,
    component_SetCurrentPriority,
};

static HRESULT WINAPI enum_components_QueryInterface(IEnumCifComponents *iface, REFIID riid, void **ppv)
{
    struct ciffenum_components *This = impl_from_IEnumCifComponents(iface);

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IEnumCifComponents_iface;
    }
    /*
    else if (IsEqualGUID(&IID_IEnumCifComponents, riid))
    {
        TRACE("(%p)->(IID_ICifFile %p)\n", This, ppv);
        *ppv = &This->IEnumCifComponents_iface;
    }
    */
    else
    {
        FIXME("(%p)->(%s %p) not found\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI enum_components_AddRef(IEnumCifComponents *iface)
{
    struct ciffenum_components *This = impl_from_IEnumCifComponents(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI enum_components_Release(IEnumCifComponents *iface)
{
    struct ciffenum_components *This = impl_from_IEnumCifComponents(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
    {
        ICifFile_Release(This->file);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI enum_components_Next(IEnumCifComponents *iface, ICifComponent **component)
{
    struct ciffenum_components *This = impl_from_IEnumCifComponents(iface);
    struct cifcomponent *comp;

    TRACE("(%p)->(%p)\n", This, component);

    if (!component)
        return E_FAIL;

    if (!This->position)
    {
        *component = NULL;
        return E_FAIL;
    }

    do
    {
        This->position = list_next(This->start, This->position);
        if (!This->position)
        {
            *component = NULL;
            return E_FAIL;
        }

        comp = CONTAINING_RECORD(This->position, struct cifcomponent, entry);
    } while (This->group_id && (!comp->group || strcmp(This->group_id, comp->group)));

    *component = &comp->ICifComponent_iface;
    return S_OK;
}

static HRESULT WINAPI enum_components_Reset(IEnumCifComponents *iface)
{
    struct ciffenum_components *This = impl_from_IEnumCifComponents(iface);

    TRACE("(%p)\n", This);

    This->position = This->start;
    return S_OK;
}

static const IEnumCifComponentsVtbl enum_componentsVtbl =
{
    enum_components_QueryInterface,
    enum_components_AddRef,
    enum_components_Release,
    enum_components_Next,
    enum_components_Reset,
};

static HRESULT enum_components_create(ICifFile *file, struct list *start, char *group_id, IEnumCifComponents **iface)
{
    struct ciffenum_components *enumerator;

    enumerator = heap_alloc_zero(sizeof(*enumerator));
    if (!enumerator) return E_OUTOFMEMORY;

    enumerator->IEnumCifComponents_iface.lpVtbl = &enum_componentsVtbl;
    enumerator->ref      = 1;
    enumerator->file     = file;
    enumerator->start    = start;
    enumerator->position = start;
    enumerator->group_id = group_id;

    ICifFile_AddRef(file);

    *iface = &enumerator->IEnumCifComponents_iface;
    return S_OK;
}

static HRESULT WINAPI enum_groups_QueryInterface(IEnumCifGroups *iface, REFIID riid, void **ppv)
{
    struct ciffenum_groups *This = impl_from_IEnumCifGroups(iface);

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IEnumCifGroups_iface;
    }
    /*
    else if (IsEqualGUID(&IID_IEnumCifGroups, riid))
    {
        TRACE("(%p)->(IID_ICifFile %p)\n", This, ppv);
        *ppv = &This->IEnumCifGroups_iface;
    }
    */
    else
    {
        FIXME("(%p)->(%s %p) not found\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI enum_groups_AddRef(IEnumCifGroups *iface)
{
    struct ciffenum_groups *This = impl_from_IEnumCifGroups(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI enum_groups_Release(IEnumCifGroups *iface)
{
    struct ciffenum_groups *This = impl_from_IEnumCifGroups(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
    {
        ICifFile_Release(This->file);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI enum_groups_Next(IEnumCifGroups *iface, ICifGroup **group)
{
    struct ciffenum_groups *This = impl_from_IEnumCifGroups(iface);
    struct cifgroup *gp;

    TRACE("(%p)->(%p)\n", This, group);

    if (!This->position || !group)
        return E_FAIL;

    This->position = list_next(This->start, This->position);

    if (!This->position)
        return E_FAIL;

    gp = CONTAINING_RECORD(This->position, struct cifgroup, entry);
    *group = &gp->ICifGroup_iface;
    return S_OK;
}

static HRESULT WINAPI enum_groups_Reset(IEnumCifGroups *iface)
{
    struct ciffenum_groups *This = impl_from_IEnumCifGroups(iface);

    TRACE("(%p)\n", This);

    This->position = This->start;
    return S_OK;
}

static const IEnumCifGroupsVtbl enum_groupsVtbl =
{
    enum_groups_QueryInterface,
    enum_groups_AddRef,
    enum_groups_Release,
    enum_groups_Next,
    enum_groups_Reset,
};

static HRESULT enum_groups_create(ICifFile *file, struct list *start, IEnumCifGroups **iface)
{
    struct ciffenum_groups *enumerator;

    enumerator = heap_alloc_zero(sizeof(*enumerator));
    if (!enumerator) return E_OUTOFMEMORY;

    enumerator->IEnumCifGroups_iface.lpVtbl = &enum_groupsVtbl;
    enumerator->ref      = 1;
    enumerator->file     = file;
    enumerator->start    = start;
    enumerator->position = start;

    ICifFile_AddRef(file);

    *iface = &enumerator->IEnumCifGroups_iface;
    return S_OK;
}

static HRESULT WINAPI ciffile_QueryInterface(ICifFile *iface, REFIID riid, void **ppv)
{
    struct ciffile *This = impl_from_ICiffile(iface);

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->ICifFile_iface;
    }
    else if (IsEqualGUID(&IID_ICifFile, riid))
    {
        TRACE("(%p)->(IID_ICifFile %p)\n", This, ppv);
        *ppv = &This->ICifFile_iface;
    }
    else
    {
        FIXME("(%p)->(%s %p) not found\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI ciffile_AddRef(ICifFile *iface)
{
    struct ciffile *This = impl_from_ICiffile(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI ciffile_Release(ICifFile *iface)
{
    struct ciffile *This = impl_from_ICiffile(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
    {
        struct cifcomponent *comp, *comp_next;
        struct cifgroup *group, *group_next;

        heap_free(This->name);

        LIST_FOR_EACH_ENTRY_SAFE(comp, comp_next, &This->components, struct cifcomponent, entry)
        {
            list_remove(&comp->entry);
            component_free(comp);
        }

        LIST_FOR_EACH_ENTRY_SAFE(group, group_next, &This->groups, struct cifgroup, entry)
        {
            list_remove(&group->entry);
            group_free(group);
        }

        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI ciffile_EnumComponents(ICifFile *iface, IEnumCifComponents **enum_components, DWORD filter, void *pv)
{
    struct ciffile *This = impl_from_ICiffile(iface);

    TRACE("(%p)->(%p, %u, %p)\n", This, enum_components, filter, pv);

    if (filter)
        FIXME("filter (%x) not supported\n", filter);
    if (pv)
        FIXME("how to handle pv (%p)?\n", pv);

    return enum_components_create(iface, &This->components, NULL, enum_components);
}

static HRESULT WINAPI ciffile_FindComponent(ICifFile *iface, const char *id, ICifComponent **component)
{
    struct ciffile *This = impl_from_ICiffile(iface);
    struct cifcomponent *comp;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_a(id), component);

    LIST_FOR_EACH_ENTRY(comp, &This->components, struct cifcomponent, entry)
    {
        if (strcmp(comp->id, id) != 0)
            continue;

        *component = &comp->ICifComponent_iface;
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI ciffile_EnumGroups(ICifFile *iface, IEnumCifGroups **enum_groups, DWORD filter, void *pv)
{
    struct ciffile *This = impl_from_ICiffile(iface);

    TRACE("(%p)->(%p, %u, %p)\n", This, enum_groups, filter, pv);

    if (filter)
        FIXME("filter (%x) not supported\n", filter);
    if (pv)
        FIXME("how to handle pv (%p)?\n", pv);

    return enum_groups_create(iface, &This->groups, enum_groups);
}

static HRESULT WINAPI ciffile_FindGroup(ICifFile *iface, const char *id, ICifGroup **group)
{
    struct ciffile *This = impl_from_ICiffile(iface);
    struct cifgroup *gp;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_a(id), group);

    LIST_FOR_EACH_ENTRY(gp, &This->groups, struct cifgroup, entry)
    {
        if (strcmp(gp->id, id) != 0)
            continue;

        *group = &gp->ICifGroup_iface;
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI ciffile_EnumModes(ICifFile *iface, IEnumCifModes **cuf_modes, DWORD filter, void *pv)
{
    struct ciffile *This = impl_from_ICiffile(iface);

    FIXME("(%p)->(%p, %u, %p): stub\n", This, cuf_modes, filter, pv);

    return E_NOTIMPL;
}

static HRESULT WINAPI ciffile_FindMode(ICifFile *iface, const char *id, ICifMode **mode)
{
    struct ciffile *This = impl_from_ICiffile(iface);

    FIXME("(%p)->(%s, %p): stub\n", This, debugstr_a(id), mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI ciffile_GetDescription(ICifFile *iface, char *desc, DWORD size)
{
    struct ciffile *This = impl_from_ICiffile(iface);

    TRACE("(%p)->(%p, %u)\n", This, desc, size);

    return copy_substring_null(desc, size, This->name);
}

static HRESULT WINAPI ciffile_GetDetDlls(ICifFile *iface, char *dlls, DWORD size)
{
    struct ciffile *This = impl_from_ICiffile(iface);

    FIXME("(%p)->(%p, %u): stub\n", This, dlls, size);

    return E_NOTIMPL;
}

static const ICifFileVtbl ciffileVtbl =
{
    ciffile_QueryInterface,
    ciffile_AddRef,
    ciffile_Release,
    ciffile_EnumComponents,
    ciffile_FindComponent,
    ciffile_EnumGroups,
    ciffile_FindGroup,
    ciffile_EnumModes,
    ciffile_FindMode,
    ciffile_GetDescription,
    ciffile_GetDetDlls,
};

static BOOL copy_string(char **dest, const char *source)
{
    if (!source)
    {
        *dest = NULL;
        return TRUE;
    }

    *dest = strdupA(source);
    if (!dest) return FALSE;
    return TRUE;
}

static BOOL section_get_str(struct inf_section *inf_sec, const char *key, char **value, const char *def)
{
    struct inf_value *inf_val;

    inf_val = inf_get_value(inf_sec, key);
    if (!inf_val) return copy_string(value, def);

    *value = inf_value_get_value(inf_val);
    if (!*value) return FALSE;

    return TRUE;
}

static char *next_part(char **str, BOOL strip_quotes)
{
    char *start = *str;
    char *next = *str;

    while (*next && *next != ',')
        next++;

    if (!*next)
    {
        *str = trim(start, NULL, strip_quotes);
        return NULL;
    }

    *next = 0;
    *str = trim(start, NULL, strip_quotes);
    return ++next;
}

static BOOL value_get_str_field(struct inf_value *inf_val, int field, char **value, const char *def)
{
    char *line, *str, *next;
    int i = 0;

    line = inf_value_get_value(inf_val);
    if (!line) return FALSE;

    str = line;
    do
    {
        i++;
        next = next_part(&str, TRUE);

        if (field == i)
        {
            BOOL ret = copy_string(value, str);
            heap_free(line);
            return ret;
        }

        str = next;
    } while (str);

    return copy_string(value, def);
}

/*
static BOOL section_get_str_field(struct inf_section *inf_sec, const char *key, int field, char **value, const char *def)
{
    struct inf_value *inf_val;

    inf_val = inf_get_value(inf_sec, key);
    if (!inf_val) return copy_string(value, def);

    return value_get_str_field(inf_val, field, value, def);
}
*/

static BOOL section_get_dword(struct inf_section *inf_sec, const char *key, DWORD *value, DWORD def)
{
    struct inf_value *inf_val;
    char *str;

    inf_val = inf_get_value(inf_sec, key);
    if (!inf_val)
    {
        *value = def;
        return TRUE;
    }

    str = inf_value_get_value(inf_val);
    if (!str) return FALSE;

    *value = atoi(str);
    heap_free(str);

    return TRUE;
}

static BOOL value_get_dword_field(struct inf_value *inf_val, int field, DWORD *value, DWORD def)
{
    char *value_str;
    BOOL ret;

    ret = value_get_str_field(inf_val, field, &value_str, NULL);
    if (!ret) return FALSE;
    if (!value_str)
    {
        *value = def;
        return TRUE;
    }

    *value = atoi(value_str);
    heap_free(value_str);

    return TRUE;
}

static BOOL section_get_dword_field(struct inf_section *inf_sec, const char *key, int field, DWORD *value, DWORD def)
{
    struct inf_value *inf_val;

    inf_val = inf_get_value(inf_sec, key);
    if (!inf_val)
    {
        *value = def;
        return TRUE;
    }

    return value_get_dword_field(inf_val, field, value, def);
}

static HRESULT process_version(struct ciffile *file, struct inf_section *section)
{
    if (!section_get_str(section, "DisplayName", &file->name, DEFAULT_INSTALLER_DESC))
        return E_OUTOFMEMORY;

    return S_OK;
}

static BOOL read_version_entry(struct inf_section *section, DWORD *ret_ver, DWORD *ret_build)
{
    DWORD version = 0;
    DWORD build = 0;
    char *line, *str, *next;

    if (!section_get_str(section, "Version", &line, NULL))
        return FALSE;
    if (!line) goto done;

    str = line;

    next = next_part(&str, TRUE);
    version |= atoi(str) << 16;
    if (!next) goto done;
    str = next;

    next = next_part(&str, TRUE);
    version |= atoi(str) & 0xffff;
    if (!next) goto done;
    str = next;

    next = next_part(&str, TRUE);
    build |= atoi(str) << 16;
    if (!next) goto done;
    str = next;

    next_part(&str, TRUE);
    build |= atoi(str) & 0xffff;

done:
    heap_free(line);
    *ret_ver = version;
    *ret_build = build;
    return TRUE;
}

static BOOL read_platform_entry(struct inf_section *section, DWORD *ret_platform)
{
    DWORD platform = PLATFORM_ALL;
    char *line, *str, *next;

    if (!section_get_str(section, "Platform", &line, NULL))
        return FALSE;
    if (!line) goto done;

    platform = 0;
    str = line;
    do
    {
        next = next_part(&str, TRUE);

        if (strcasecmp(str, "Win95") == 0)
            platform |= PLATFORM_WIN98;
        else if (strcasecmp(str, "Win98") == 0)
            platform |= PLATFORM_WIN98;
        else if (strcasecmp(str, "NT4") == 0)
            platform |= PLATFORM_NT4;
        else if (strcasecmp(str, "NT5") == 0)
            platform |= PLATFORM_NT5;
        else if (strcasecmp(str, "NT4Alpha") == 0)
            platform |= PLATFORM_NT4;
        else if (strcasecmp(str, "NT5Alpha") == 0)
            platform |= PLATFORM_NT5;
        else if (strcasecmp(str, "Millen") == 0)
            platform |= PLATFORM_MILLEN;
        else
            FIXME("Unknown platform: %s\n", debugstr_a(str));

        str = next;
    } while (str);

done:
    heap_free(line);
    *ret_platform = platform;
    return TRUE;
}

static BOOL read_dependencies(struct cifcomponent *component, struct inf_section *section)
{
    struct dependency_info *dependency;
    char *line, *str, *next;
    BOOL ret = TRUE;

    if (!section_get_str(section, "Dependencies", &line, NULL))
        return E_OUTOFMEMORY;
    if (!line) goto done;

    ret = FALSE;
    str = line;
    do
    {
        next = next_part(&str, TRUE);

        dependency = heap_alloc_zero(sizeof(*dependency));
        if (!dependency) goto done;

        dependency->id = strdupA(str);
        if (!dependency->id)
        {
            heap_free(dependency);
            goto done;
        }

        dependency->type = strstr(dependency->id, ":");
        if (dependency->type) *dependency->type++ = 0;

        list_add_tail(&component->dependencies, &dependency->entry);

        str = next;
    } while (str);

    ret = TRUE;

done:
    heap_free(line);
    return ret;
}

static BOOL read_urls(struct cifcomponent *component, struct inf_section *section)
{
    struct inf_value *inf_value = NULL;
    struct url_info *url_entry;
    char *str, *next;
    int index;

    while (inf_section_next_value(section, &inf_value))
    {
        str = inf_value_get_key(inf_value);
        if (!str) return E_OUTOFMEMORY;

        if (strncasecmp(str, "URL", 3))
            goto next;

        if (!str[3])
            goto next;

        index = strtol(str+3, &next, 10);
        if (next == str+3 || *next != 0 || index < 1)
            goto next;
        index--;

        url_entry = heap_alloc_zero(sizeof(*url_entry));
        if (!url_entry) goto error;

        url_entry->index = index;

        if (!value_get_str_field(inf_value, 1, &url_entry->url, NULL))
            goto error;
        if (!url_entry->url || !*url_entry->url)
        {
            url_entry_free(url_entry);
            goto next;
        }

        if (!value_get_dword_field(inf_value, 2, &url_entry->flags, 0))
            goto error;

        list_add_tail(&component->urls, &url_entry->entry);

    next:
        heap_free(str);
    }

    return TRUE;

error:
    heap_free(str);
    url_entry_free(url_entry);
    return FALSE;
};

void add_component_by_priority(struct ciffile *file, struct cifcomponent *component)
{
    struct cifcomponent *entry;

    LIST_FOR_EACH_ENTRY(entry, &file->components, struct cifcomponent, entry)
    {
        if (entry->priority > component->priority)
            continue;

        list_add_before(&entry->entry, &component->entry);
        return;
    }

    list_add_tail(&file->components, &component->entry);
}

static HRESULT process_component(struct ciffile *file, struct inf_section *section, const char *section_name)
{
    struct cifcomponent *component;
    HRESULT hr = E_OUTOFMEMORY;

    component = heap_alloc_zero(sizeof(*component));
    if (!component) return E_OUTOFMEMORY;

    component->ICifComponent_iface.lpVtbl = &cifcomponentVtbl;
    component->parent = &file->ICifFile_iface;

    list_init(&component->urls);
    list_init(&component->dependencies);

    component->queue_state = ActionNone;

    component->id = strdupA(section_name);
    if (!component->id) goto error;

    if (!section_get_str(section, "DisplayName", &component->description, NULL))
        goto error;
    if (!section_get_str(section, "GUID", &component->guid, NULL))
        goto error;
    if (!section_get_str(section, "Details", &component->details, NULL))
        goto error;
    if (!section_get_str(section, "Group", &component->group, NULL))
        goto error;
    if (!section_get_str(section, "Locale", &component->locale, "en"))
        goto error;
    if (!section_get_str(section, "PatchID", &component->patchid, NULL))
        goto error;

    if (!section_get_dword_field(section, "Size", 1, &component->size_download, 0))
        goto error;
    if (!section_get_dword_field(section, "Size", 2, &component->size_extracted, 0))
        goto error;
    if (!section_get_dword_field(section, "InstalledSize", 1, &component->size_app, 0))
        goto error;
    if (!section_get_dword_field(section, "InstalledSize", 2, &component->size_win, 0))
        goto error;

    if (!section_get_str(section, "SuccessKey", &component->key_success, NULL))
        goto error;
    if (!section_get_str(section, "CancelKey", &component->key_cancel, NULL))
        goto error;
    if (!section_get_str(section, "ProgressKey", &component->key_progress, NULL))
        goto error;
    if (!section_get_str(section, "UninstallKey", &component->key_uninstall, NULL))
        goto error;
    if (!section_get_dword(section, "Reboot", &component->reboot, 0))
        goto error;
    if (!section_get_dword(section, "AdminCheck", &component->admin, 0))
        goto error;
    if (!section_get_dword(section, "UIVisible", &component->visibleui, 1))
        goto error;
    if (!section_get_dword(section, "ActiveSetupAware", &component->as_aware, 0))
        goto error;
    if (!section_get_dword(section, "Priority", &component->priority, 0))
        goto error;

    if (!read_version_entry(section, &component->version, &component->build))
        goto error;
    if (!read_platform_entry(section, &component->platform))
        goto error;
    if (!read_urls(component, section))
        goto error;
    if (!read_dependencies(component, section))
        goto error;

    component->current_priority = component->priority;

    add_component_by_priority(file, component);
    return S_OK;

error:
    component_free(component);
    return hr;
}

static HRESULT process_group(struct ciffile *file, struct inf_section *section, const char *section_name)
{
    struct cifgroup *group;
    HRESULT hr = E_OUTOFMEMORY;

    group = heap_alloc_zero(sizeof(*group));
    if (!group) return E_OUTOFMEMORY;

    group->ICifGroup_iface.lpVtbl = &cifgroupVtbl;
    group->parent = &file->ICifFile_iface;

    group->id = strdupA(section_name);
    if (!group->id) goto error;

    if (!section_get_str(section, "DisplayName", &group->description, NULL))
        goto error;
    if (!section_get_dword(section, "Priority", &group->priority, 0))
        goto error;

    list_add_head(&file->groups, &group->entry);
    return S_OK;

error:
    group_free(group);
    return hr;
}

static HRESULT process_section(struct ciffile *file, struct inf_section *section, const char *section_name)
{
    HRESULT hr;
    char *type;

    if (!section_get_str(section, "SectionType", &type, "Component"))
        return E_OUTOFMEMORY;

    if (!strcasecmp(type, "Component"))
        hr = process_component(file, section, section_name);
    else if (strcasecmp(type, "Group") == 0)
        hr = process_group(file, section, section_name);
    else
        FIXME("Don't know how to process %s\n", debugstr_a(type));

    heap_free(type);
    return hr;
}

static HRESULT process_inf(struct ciffile *file, struct inf_file *inf)
{
    struct inf_section *section = NULL;
    char *section_name;
    HRESULT hr = S_OK;

    while (SUCCEEDED(hr) && inf_next_section(inf, &section))
    {
        section_name = inf_section_get_name(section);
        if (!section_name) return E_OUTOFMEMORY;

        TRACE("start processing section %s\n", debugstr_a(section_name));

        if (!strcasecmp(section_name, "Strings") ||
            !strncasecmp(section_name, "Strings.", strlen("Strings.")))
        {
            /* Ignore string sections */
        }
        else if (strcasecmp(section_name, "Version") == 0)
            hr = process_version(file, section);
        else
            hr = process_section(file, section, section_name);

        TRACE("finished processing section %s (%x)\n", debugstr_a(section_name), hr);
        heap_free(section_name);
    }

    /* In case there was no version section, set the default installer description */
    if (SUCCEEDED(hr) && !file->name)
    {
        file->name = strdupA(DEFAULT_INSTALLER_DESC);
        if (!file->name) hr = E_OUTOFMEMORY;
    }

    return hr;
}

static HRESULT load_ciffile(const char *path, ICifFile **icif)
{
    struct inf_file *inf = NULL;
    struct ciffile *file;
    HRESULT hr = E_FAIL;

    file = heap_alloc_zero(sizeof(*file));
    if(!file) return E_OUTOFMEMORY;

    file->ICifFile_iface.lpVtbl = &ciffileVtbl;
    file->ref = 1;

    list_init(&file->components);
    list_init(&file->groups);

    hr = inf_load(path, &inf);
    if (FAILED(hr)) goto error;

    hr = process_inf(file, inf);
    if (FAILED(hr)) goto error;

    *icif = &file->ICifFile_iface;
    return S_OK;

error:
    if (inf) inf_free(inf);
    ICifFile_Release(&file->ICifFile_iface);
    return hr;
}

HRESULT WINAPI GetICifFileFromFile(ICifFile **icif, const char *path)
{
    TRACE("(%p, %s)\n", icif, debugstr_a(path));

    return load_ciffile(path, icif);
}


HRESULT WINAPI GetICifRWFileFromFile(ICifRWFile **icif, const char *path)
{
    FIXME("(%p, %s): stub\n", icif, debugstr_a(path));

    return E_NOTIMPL;
}
