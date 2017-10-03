/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

#include <windows.h>
#include <strsafe.h>

#include "nfs41_ops.h"
#include "util.h"
#include "daemon_debug.h"


#define NSLVL 2 /* dprintf level for namespace logging */


#define client_entry(pos) list_container(pos, nfs41_client, root_entry)


/* nfs41_root */
int nfs41_root_create(
    IN const char *name,
    IN uint32_t sec_flavor,
    IN uint32_t wsize,
    IN uint32_t rsize,
    OUT nfs41_root **root_out)
{
    int status = NO_ERROR;
    nfs41_root *root;

    dprintf(NSLVL, "--> nfs41_root_create()\n");

    root = calloc(1, sizeof(nfs41_root));
    if (root == NULL) {
        status = GetLastError();
        goto out;
    }

    list_init(&root->clients);
    root->wsize = wsize;
    root->rsize = rsize;
    InitializeCriticalSection(&root->lock);
    root->ref_count = 1;
    root->sec_flavor = sec_flavor;

    /* generate a unique client_owner */
    status = nfs41_client_owner(name, sec_flavor, &root->client_owner);
    if (status) {
        eprintf("nfs41_client_owner() failed with %d\n", status);
        free(root);
        goto out;
    }

    *root_out = root;
out:
    dprintf(NSLVL, "<-- nfs41_root_create() returning %d\n", status);
    return status;
}

static void root_free(
    IN nfs41_root *root)
{
    struct list_entry *entry, *tmp;

    dprintf(NSLVL, "--> nfs41_root_free()\n");

    /* free clients */
    list_for_each_tmp(entry, tmp, &root->clients)
        nfs41_client_free(client_entry(entry));
    DeleteCriticalSection(&root->lock);
    free(root);

    dprintf(NSLVL, "<-- nfs41_root_free()\n");
}

void nfs41_root_ref(
    IN nfs41_root *root)
{
    const LONG count = InterlockedIncrement(&root->ref_count);

    dprintf(NSLVL, "nfs41_root_ref() count %d\n", count);
}

void nfs41_root_deref(
    IN nfs41_root *root)
{
    const LONG count = InterlockedDecrement(&root->ref_count);

    dprintf(NSLVL, "nfs41_root_deref() count %d\n", count);
    if (count == 0)
        root_free(root);
}


/* root_client_find_addrs() */
struct cl_addr_info {
    const multi_addr4       *addrs;
    uint32_t                roles;
};

static int cl_addr_compare(
    IN const struct list_entry *entry,
    IN const void *value)
{
    nfs41_client *client = client_entry(entry);
    const struct cl_addr_info *info = (const struct cl_addr_info*)value;
    uint32_t i, roles;

    /* match any of the desired roles */
    AcquireSRWLockShared(&client->exid_lock);
    roles = info->roles & client->roles;
    ReleaseSRWLockShared(&client->exid_lock);

    if (roles == 0)
        return ERROR_FILE_NOT_FOUND;

    /* match any address in 'addrs' with any address in client->rpc->addrs */
    for (i = 0; i < info->addrs->count; i++)
        if (multi_addr_find(&client->rpc->addrs, &info->addrs->arr[i], NULL))
            return NO_ERROR;

    return ERROR_FILE_NOT_FOUND;
}

static int root_client_find_addrs(
    IN nfs41_root *root,
    IN const multi_addr4 *addrs,
    IN bool_t is_data,
    OUT nfs41_client **client_out)
{
    struct cl_addr_info info;
    struct list_entry *entry;
    int status;

    dprintf(NSLVL, "--> root_client_find_addrs()\n");

    info.addrs = addrs;
    info.roles = nfs41_exchange_id_flags(is_data) & EXCHGID4_FLAG_MASK_PNFS;

    entry = list_search(&root->clients, &info, cl_addr_compare);
    if (entry) {
        *client_out = client_entry(entry);
        status = NO_ERROR;
        dprintf(NSLVL, "<-- root_client_find_addrs() returning 0x%p\n",
            *client_out);
    } else {
        status = ERROR_FILE_NOT_FOUND;
        dprintf(NSLVL, "<-- root_client_find_addrs() failed with %d\n",
            status);
    }
    return status;
}

/* root_client_find() */
struct cl_exid_info {
    const nfs41_exchange_id_res *exchangeid;
    uint32_t                roles;
};

static int cl_exid_compare(
    IN const struct list_entry *entry,
    IN const void *value)
{
    nfs41_client *client = client_entry(entry);
    const struct cl_exid_info *info = (const struct cl_exid_info*)value;
    int status = ERROR_FILE_NOT_FOUND;

    AcquireSRWLockShared(&client->exid_lock);

    /* match any of the desired roles */
    if ((info->roles & client->roles) == 0)
        goto out;
    /* match server_owner.major_id */
    if (strncmp(info->exchangeid->server_owner.so_major_id,
        client->server->owner, NFS4_OPAQUE_LIMIT) != 0)
        goto out;
    /* match server_scope */
    if (strncmp(info->exchangeid->server_scope,
        client->server->scope, NFS4_OPAQUE_LIMIT) != 0)
        goto out;
    /* match clientid */
    if (info->exchangeid->clientid != client->clnt_id)
        goto out;

    status = NO_ERROR;
out:
    ReleaseSRWLockShared(&client->exid_lock);
    return status;
}

static int root_client_find(
    IN nfs41_root *root,
    IN const nfs41_exchange_id_res *exchangeid,
    IN bool_t is_data,
    OUT nfs41_client **client_out)
{
    struct cl_exid_info info;
    struct list_entry *entry;
    int status;

    dprintf(NSLVL, "--> root_client_find()\n");

    info.exchangeid = exchangeid;
    info.roles = nfs41_exchange_id_flags(is_data) & EXCHGID4_FLAG_MASK_PNFS;

    entry = list_search(&root->clients, &info, cl_exid_compare);
    if (entry) {
        *client_out = client_entry(entry);
        status = NO_ERROR;
        dprintf(NSLVL, "<-- root_client_find() returning 0x%p\n",
            *client_out);
    } else {
        status = ERROR_FILE_NOT_FOUND;
        dprintf(NSLVL, "<-- root_client_find() failed with %d\n",
            status);
    }
    return status;
}

static int session_get_lease(
    IN nfs41_session *session,
    IN OPTIONAL uint32_t lease_time)
{
    bool_t use_mds_lease;
    int status;

    /* http://tools.ietf.org/html/rfc5661#section-13.1.1
     * 13.1.1. Sessions Considerations for Data Servers:
     * If the reply to EXCHANGE_ID has just the EXCHGID4_FLAG_USE_PNFS_DS role
     * set, then (as noted in Section 13.6) the client will not be able to
     * determine the data server's lease_time attribute because GETATTR will
     * not be permitted.  Instead, the rule is that any time a client
     * receives a layout referring it to a data server that returns just the
     * EXCHGID4_FLAG_USE_PNFS_DS role, the client MAY assume that the
     * lease_time attribute from the metadata server that returned the
     * layout applies to the data server. */
    AcquireSRWLockShared(&session->client->exid_lock);
    use_mds_lease = session->client->roles == EXCHGID4_FLAG_USE_PNFS_DS;
    ReleaseSRWLockShared(&session->client->exid_lock);

    if (!use_mds_lease) {
        /* the client is allowed to GETATTR, so query the lease_time */
        nfs41_file_info info = { 0 };
        bitmap4 attr_request = { 1, { FATTR4_WORD0_LEASE_TIME, 0, 0 } };

        status = nfs41_getattr(session, NULL, &attr_request, &info);
        if (status) {
            eprintf("nfs41_getattr() failed with %s\n",
                nfs_error_string(status));
            status = nfs_to_windows_error(status, ERROR_BAD_NET_RESP);
            goto out;
        }
        lease_time = info.lease_time;
    }

    status = nfs41_session_set_lease(session, lease_time);
    if (status) {
        eprintf("nfs41_session_set_lease() failed %d\n", status);
        goto out;
    }
out:
    return status;
}

static int root_client_create(
    IN nfs41_root *root,
    IN nfs41_rpc_clnt *rpc,
    IN bool_t is_data,
    IN OPTIONAL uint32_t lease_time,
    IN const nfs41_exchange_id_res *exchangeid,
    OUT nfs41_client **client_out)
{
    nfs41_client *client;
    nfs41_session *session;
    int status;

    /* create client (transfers ownership of rpc to client) */
    status = nfs41_client_create(rpc, &root->client_owner,
        is_data, exchangeid, &client);
    if (status) {
        eprintf("nfs41_client_create() failed with %d\n", status);
        goto out;
    }
    client->root = root;
    rpc->client = client;

    /* create session (and client takes ownership) */
    status = nfs41_session_create(client, &session);
    if (status) {
        eprintf("nfs41_session_create failed %d\n", status);
        goto out_err;
    }

    if (!is_data) {
        /* send RECLAIM_COMPLETE, but don't fail on ERR_NOTSUPP */
        status = nfs41_reclaim_complete(session);
        if (status && status != NFS4ERR_NOTSUPP) {
            eprintf("nfs41_reclaim_complete() failed with %s\n",
                nfs_error_string(status));
            status = ERROR_BAD_NETPATH;
            goto out_err;
        }
    }

    /* get least time and start session renewal thread */
    status = session_get_lease(session, lease_time);
    if (status)
        goto out_err;

    *client_out = client;
out:
    return status;

out_err:
    nfs41_client_free(client);
    goto out;
}

int nfs41_root_mount_addrs(
    IN nfs41_root *root,
    IN const multi_addr4 *addrs,
    IN bool_t is_data,
    IN OPTIONAL uint32_t lease_time,
    OUT nfs41_client **client_out)
{
    nfs41_exchange_id_res exchangeid = { 0 };
    nfs41_rpc_clnt *rpc;
    nfs41_client *client, *existing;
    int status;

    dprintf(NSLVL, "--> nfs41_root_mount_addrs()\n");

    /* look for an existing client that matches the address and role */
    EnterCriticalSection(&root->lock);
    status = root_client_find_addrs(root, addrs, is_data, &client);
    LeaveCriticalSection(&root->lock);

    if (status == NO_ERROR)
        goto out;

    /* create an rpc client */
    status = nfs41_rpc_clnt_create(addrs, root->wsize, root->rsize,
        root->uid, root->gid, root->sec_flavor, &rpc);
    if (status) {
        eprintf("nfs41_rpc_clnt_create() failed %d\n", status);
        goto out;
    }

    /* get a clientid with exchangeid */
    status = nfs41_exchange_id(rpc, &root->client_owner,
        nfs41_exchange_id_flags(is_data), &exchangeid);
    if (status) {
        eprintf("nfs41_exchange_id() failed %s\n", nfs_error_string(status));
        status = ERROR_BAD_NET_RESP;
        goto out_free_rpc;
    }

    /* attempt to match existing clients by the exchangeid response */
    EnterCriticalSection(&root->lock);
    status = root_client_find(root, &exchangeid, is_data, &client);
    LeaveCriticalSection(&root->lock);

    if (status == NO_ERROR)
        goto out_free_rpc;

    /* create a client for this clientid */
    status = root_client_create(root, rpc, is_data,
        lease_time, &exchangeid, &client);
    if (status) {
        eprintf("nfs41_client_create() failed %d\n", status);
        /* root_client_create takes care of cleaning up 
         * thus don't go to out_free_rpc */
        goto out;
    }

    /* because we don't hold the root's lock over session creation,
     * we could end up creating multiple clients with the same
     * server and roles */
    EnterCriticalSection(&root->lock);
    status = root_client_find(root, &exchangeid, is_data, &existing);

    if (status) {
        dprintf(NSLVL, "caching new client 0x%p\n", client);

        /* the client is not a duplicate, so add it to the list */
        list_add_tail(&root->clients, &client->root_entry);
        status = NO_ERROR;
    } else {
        dprintf(NSLVL, "created a duplicate client 0x%p! using "
            "existing client 0x%p instead\n", client, existing);

        /* a matching client has been created in parallel, so free
         * the one we created and use the existing client instead */
        nfs41_client_free(client);
        client = existing;
    }
    LeaveCriticalSection(&root->lock);

out:
    if (status == NO_ERROR)
        *client_out = client;
    dprintf(NSLVL, "<-- nfs41_root_mount_addrs() returning %d\n", status);
    return status;

out_free_rpc:
    nfs41_rpc_clnt_free(rpc);
    goto out;
}


/* http://tools.ietf.org/html/rfc5661#section-11.9
 * 11.9. The Attribute fs_locations
 * An entry in the server array is a UTF-8 string and represents one of a
 * traditional DNS host name, IPv4 address, IPv6 address, or a zero-length
 * string.  An IPv4 or IPv6 address is represented as a universal address
 * (see Section 3.3.9 and [15]), minus the netid, and either with or without
 * the trailing ".p1.p2" suffix that represents the port number.  If the
 * suffix is omitted, then the default port, 2049, SHOULD be assumed.  A
 * zero-length string SHOULD be used to indicate the current address being
 * used for the RPC call. */
static int referral_mount_location(
    IN nfs41_root *root,
    IN const fs_location4 *loc,
    OUT nfs41_client **client_out)
{
    multi_addr4 addrs;
    int status = ERROR_BAD_NET_NAME;
    uint32_t i;

    /* create a client and session for the first available server */
    for (i = 0; i < loc->server_count; i++) {
        /* XXX: only deals with 'address' as a hostname with default port */
        status = nfs41_server_resolve(loc->servers[i].address, 2049, &addrs);
        if (status) continue;

        status = nfs41_root_mount_addrs(root, &addrs, 0, 0, client_out);
        if (status == NO_ERROR)
            break;
    }
    return status;
}

int nfs41_root_mount_referral(
    IN nfs41_root *root,
    IN const fs_locations4 *locations,
    OUT const fs_location4 **loc_out,
    OUT nfs41_client **client_out)
{
    int status = ERROR_BAD_NET_NAME;
    uint32_t i;

    /* establish a mount to the first available location */
    for (i = 0; i < locations->location_count; i++) {
        status = referral_mount_location(root,
            &locations->locations[i], client_out);
        if (status == NO_ERROR) {
            *loc_out = &locations->locations[i];
            break;
        }
    }
    return status;
}
