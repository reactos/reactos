/*
 * Endpoint Mapper
 *
 * Copyright (C) 2007 Robert Shearman for CodeWeavers
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

#include "epm.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

struct registered_ept_entry
{
    struct list entry;
    GUID object;
    RPC_SYNTAX_IDENTIFIER iface;
    RPC_SYNTAX_IDENTIFIER syntax;
    char *protseq;
    char *endpoint;
    char *address;
    char annotation[ept_max_annotation_size];
};

static struct list registered_ept_entry_list = LIST_INIT(registered_ept_entry_list);

static CRITICAL_SECTION csEpm;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &csEpm,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": csEpm") }
};
static CRITICAL_SECTION csEpm = { &critsect_debug, -1, 0, 0, 0, 0 };

static const UUID nil_object;

/* must be called inside csEpm */
static void delete_registered_ept_entry(struct registered_ept_entry *entry)
{
    I_RpcFree(entry->protseq);
    I_RpcFree(entry->endpoint);
    I_RpcFree(entry->address);
    list_remove(&entry->entry);
    free(entry);
}

static struct registered_ept_entry *find_ept_entry(
    const RPC_SYNTAX_IDENTIFIER *iface, const RPC_SYNTAX_IDENTIFIER *syntax,
    const char *protseq, const char *endpoint, const char *address,
    const UUID *object)
{
    struct registered_ept_entry *entry;
    LIST_FOR_EACH_ENTRY(entry, &registered_ept_entry_list, struct registered_ept_entry, entry)
    {
        if (memcmp(&entry->iface, iface, sizeof(RPC_SYNTAX_IDENTIFIER))) continue;
        if (memcmp(&entry->syntax, syntax, sizeof(RPC_SYNTAX_IDENTIFIER))) continue;
        if (strcmp(entry->protseq, protseq)) continue;
        if (memcmp(&entry->object, object, sizeof(UUID))) continue;
	WINE_TRACE("found entry with iface %d.%d %s, syntax %d.%d %s, protseq %s, object %s\n",
                   entry->iface.SyntaxVersion.MajorVersion, entry->iface.SyntaxVersion.MinorVersion,
                   wine_dbgstr_guid(&entry->iface.SyntaxGUID),
                   entry->syntax.SyntaxVersion.MajorVersion, entry->syntax.SyntaxVersion.MinorVersion,
                   wine_dbgstr_guid(&entry->syntax.SyntaxGUID), protseq,
                   wine_dbgstr_guid(&entry->object));
        return entry;
    }
    WINE_TRACE("not found\n");
    return NULL;
}

void __RPC_USER ept_lookup_handle_t_rundown(ept_lookup_handle_t entry_handle)
{
    WINE_FIXME("%p\n", entry_handle);
}

void __cdecl ept_insert(handle_t h,
                        unsigned32 num_ents,
                        ept_entry_t entries[],
                        boolean32 replace,
                        error_status_t *status)
{
    unsigned32 i;
    RPC_STATUS rpc_status;

    WINE_TRACE("(%p, %lu, %p, %lu, %p)\n", h, num_ents, entries, replace, status);

    *status = RPC_S_OK;

    EnterCriticalSection(&csEpm);

    for (i = 0; i < num_ents; i++)
    {
        struct registered_ept_entry *entry = malloc(sizeof(*entry));
        if (!entry)
        {
            /* FIXME: cleanup code to delete added entries */
            *status = EPT_S_CANT_PERFORM_OP;
            break;
        }
        memcpy(entry->annotation, entries[i].annotation, sizeof(entries[i].annotation));
        rpc_status = TowerExplode(entries[i].tower, &entry->iface, &entry->syntax,
                                  &entry->protseq, &entry->endpoint,
                                  &entry->address);
        if (rpc_status != RPC_S_OK)
        {
            WINE_WARN("TowerExplode failed %lu\n", rpc_status);
            *status = rpc_status;
            free(entry);
            break; /* FIXME: more cleanup? */
        }

        entry->object = entries[i].object;

        if (replace)
        {
            /* FIXME: correct find algorithm */
            struct registered_ept_entry *old_entry = find_ept_entry(&entry->iface, &entry->syntax, entry->protseq, entry->endpoint, entry->address, &entry->object);
            if (old_entry) delete_registered_ept_entry(old_entry);
        }
        list_add_tail(&registered_ept_entry_list, &entry->entry);
    }

    LeaveCriticalSection(&csEpm);
}

void __cdecl ept_delete(handle_t h,
                        unsigned32 num_ents,
                        ept_entry_t entries[],
                        error_status_t *status)
{
    unsigned32 i;
    RPC_STATUS rpc_status;

    *status = RPC_S_OK;

    WINE_TRACE("(%p, %lu, %p, %p)\n", h, num_ents, entries, status);

    EnterCriticalSection(&csEpm);

    for (i = 0; i < num_ents; i++)
    {
        struct registered_ept_entry *entry;
        RPC_SYNTAX_IDENTIFIER iface, syntax;
        char *protseq;
        char *endpoint;
        char *address;
        rpc_status = TowerExplode(entries[i].tower, &iface, &syntax, &protseq,
                                  &endpoint, &address);
        if (rpc_status != RPC_S_OK)
            break;
        entry = find_ept_entry(&iface, &syntax, protseq, endpoint, address, &entries[i].object);

        I_RpcFree(protseq);
        I_RpcFree(endpoint);
        I_RpcFree(address);

        if (entry)
            delete_registered_ept_entry(entry);
        else
        {
            *status = EPT_S_NOT_REGISTERED;
            break;
        }
    }

    LeaveCriticalSection(&csEpm);
}

void __cdecl ept_lookup(handle_t h,
                        unsigned32 inquiry_type,
                        uuid_p_t object,
                        rpc_if_id_p_t interface_id,
                        unsigned32 vers_option,
                        ept_lookup_handle_t *entry_handle,
                        unsigned32 max_ents,
                        unsigned32 *num_ents,
                        ept_entry_t entries[],
                        error_status_t *status)
{
    WINE_FIXME("(%p, %p, %p): stub\n", h, entry_handle, status);

    *status = EPT_S_CANT_PERFORM_OP;
}

void __cdecl ept_map(handle_t h,
                     uuid_p_t object,
                     twr_p_t map_tower,
                     ept_lookup_handle_t *entry_handle,
                     unsigned32 max_towers,
                     unsigned32 *num_towers,
                     twr_p_t *towers,
                     error_status_t *status)
{
    RPC_STATUS rpc_status;
    RPC_SYNTAX_IDENTIFIER iface, syntax;
    char *protseq;
    struct registered_ept_entry *entry;

    *status = RPC_S_OK;
    *num_towers = 0;

    WINE_TRACE("(%p, %p, %p, %p, %lu, %p, %p, %p)\n", h, object, map_tower,
          entry_handle, max_towers, num_towers, towers, status);

    rpc_status = TowerExplode(map_tower, &iface, &syntax, &protseq,
                              NULL, NULL);
    if (rpc_status != RPC_S_OK)
    {
        *status = rpc_status;
        return;
    }

    EnterCriticalSection(&csEpm);

    LIST_FOR_EACH_ENTRY(entry, &registered_ept_entry_list, struct registered_ept_entry, entry)
    {
        if (IsEqualGUID(&entry->iface.SyntaxGUID, &iface.SyntaxGUID) &&
            (entry->iface.SyntaxVersion.MajorVersion == iface.SyntaxVersion.MajorVersion) &&
            (entry->iface.SyntaxVersion.MinorVersion >= iface.SyntaxVersion.MinorVersion) &&
            !memcmp(&entry->syntax, &syntax, sizeof(syntax)) &&
            !strcmp(entry->protseq, protseq) &&
            ((!object && IsEqualGUID(&entry->object, &nil_object)) || IsEqualGUID(object, &entry->object)))
        {
            if (*num_towers < max_towers)
            {
                rpc_status = TowerConstruct(&entry->iface, &entry->syntax,
                                            entry->protseq, entry->endpoint,
                                            entry->address,
                                            &towers[*num_towers]);
                if (rpc_status != RPC_S_OK)
                {
                    *status = rpc_status;
                    break; /* FIXME: more cleanup? */
                }
            }
            (*num_towers)++;
        }
    }

    LeaveCriticalSection(&csEpm);

    I_RpcFree(protseq);
}

void __cdecl ept_lookup_handle_free(handle_t h,
                                    ept_lookup_handle_t *entry_handle,
                                    error_status_t *status)
{
    WINE_FIXME("(%p, %p, %p): stub\n", h, entry_handle, status);

    *status = EPT_S_CANT_PERFORM_OP;
}

void __cdecl ept_inq_object(handle_t h,
                            GUID *ept_object,
                            error_status_t *status)
{
    WINE_FIXME("(%p, %p, %p): stub\n", h, ept_object, status);

    *status = EPT_S_CANT_PERFORM_OP;
}

void __cdecl ept_mgmt_delete(handle_t h,
                             boolean32 object_speced,
                             uuid_p_t object,
                             twr_p_t tower,
                             error_status_t *status)
{
    WINE_FIXME("(%p, %ld, %p, %p, %p): stub\n", h, object_speced, object, tower, status);

    *status = EPT_S_CANT_PERFORM_OP;
}
