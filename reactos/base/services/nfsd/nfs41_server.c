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
#include <stdio.h>

#include "wintirpc.h"
#include "rpc/rpc.h"

#include "name_cache.h"
#include "daemon_debug.h"
#include "nfs41.h"
#include "util.h"


#define SRVLVL 2 /* dprintf level for server logging */


/* nfs41_server_list */
struct server_list {
    struct list_entry       head;
    CRITICAL_SECTION        lock;
};
static struct server_list g_server_list;

#define server_entry(pos) list_container(pos, nfs41_server, entry)


void nfs41_server_list_init()
{
    list_init(&g_server_list.head);
    InitializeCriticalSection(&g_server_list.lock);
}

/* http://tools.ietf.org/html/rfc5661#section-1.6
 * 1.6. General Definitions: Server Owner:
 * "When the client has two connections each to a peer with the same major
 * identifier, the client assumes that both peers are the same server (the
 * server namespace is the same via each connection)" */

/* http://tools.ietf.org/html/rfc5661#section-2.10.4
 * 2.10.4. Server Scope
 * "When the server scope values are the same, server owner value may be
 * validly compared.  In cases where the server scope values are different,
 * server owner values are treated as different even if they contain all
 * identical bytes." */

/* given these definitions, we require that both the server_owner.major_id
 * and server_scope are identical when matching instances of nfs41_server */

struct server_info {
    const char *scope;
    const char *owner;
};

static int server_compare(
    const struct list_entry *entry,
    const void *value)
{
    const nfs41_server *server = server_entry(entry);
    const struct server_info *info = (const struct server_info*)value;
    const int diff = strncmp(server->scope, info->scope, NFS4_OPAQUE_LIMIT);
    return diff ? diff : strncmp(server->owner, info->owner, NFS4_OPAQUE_LIMIT);
}

static int server_entry_find(
    IN struct server_list *servers,
    IN const struct server_info *info,
    OUT struct list_entry **entry_out)
{
    *entry_out = list_search(&servers->head, info, server_compare);
    return *entry_out ? NO_ERROR : ERROR_FILE_NOT_FOUND;
}

static int server_create(
    IN const struct server_info *info,
    OUT nfs41_server **server_out)
{
    int status = NO_ERROR;
    nfs41_server *server;

    server = calloc(1, sizeof(nfs41_server));
    if (server == NULL) {
        status = GetLastError();
        eprintf("failed to allocate server %s\n", info->owner);
        goto out;
    }

    StringCchCopyA(server->scope, NFS4_OPAQUE_LIMIT, info->scope);
    StringCchCopyA(server->owner, NFS4_OPAQUE_LIMIT, info->owner);
    InitializeSRWLock(&server->addrs.lock);
    nfs41_superblock_list_init(&server->superblocks);

    status = nfs41_name_cache_create(&server->name_cache);
    if (status) {
        eprintf("nfs41_name_cache_create() failed with %d\n", status);
        goto out_free;
    }
out:
    *server_out = server;
    return status;

out_free:
    free(server);
    server = NULL;
    goto out;
}

static void server_free(
    IN nfs41_server *server)
{
    dprintf(SRVLVL, "server_free(%s)\n", server->owner);
    nfs41_superblock_list_free(&server->superblocks);
    nfs41_name_cache_free(&server->name_cache);
    free(server);
}

static __inline void server_ref_locked(
    IN nfs41_server *server)
{
    server->ref_count++;
    dprintf(SRVLVL, "nfs41_server_ref(%s) count %d\n",
        server->owner, server->ref_count);
}

void nfs41_server_ref(
    IN nfs41_server *server)
{
    EnterCriticalSection(&g_server_list.lock);

    server_ref_locked(server);

    LeaveCriticalSection(&g_server_list.lock);
}

void nfs41_server_deref(
    IN nfs41_server *server)
{
    EnterCriticalSection(&g_server_list.lock);

    server->ref_count--;
    dprintf(SRVLVL, "nfs41_server_deref(%s) count %d\n",
        server->owner, server->ref_count);
    if (server->ref_count == 0) {
        list_remove(&server->entry);
        server_free(server);
    }

    LeaveCriticalSection(&g_server_list.lock);
}

static void server_addrs_add(
    IN OUT struct server_addrs *addrs,
    IN const netaddr4 *addr)
{
    /* we keep a list of addrs used to connect to each server. once it gets
     * bigger than NFS41_ADDRS_PER_SERVER, overwrite the oldest addrs. use
     * server_addrs.next_index to implement a circular array */

    AcquireSRWLockExclusive(&addrs->lock);

    if (multi_addr_find(&addrs->addrs, addr, NULL)) {
        dprintf(SRVLVL, "server_addrs_add() found existing addr '%s'.\n",
            addr->uaddr);
    } else {
        /* overwrite the address at 'next_index' */
        StringCchCopyA(addrs->addrs.arr[addrs->next_index].netid,
            NFS41_NETWORK_ID_LEN+1, addr->netid);
        StringCchCopyA(addrs->addrs.arr[addrs->next_index].uaddr,
            NFS41_UNIVERSAL_ADDR_LEN+1, addr->uaddr);

        /* increment/wrap next_index */
        addrs->next_index = (addrs->next_index + 1) % NFS41_ADDRS_PER_SERVER;
        /* update addrs.count if necessary */
        if (addrs->addrs.count < addrs->next_index)
            addrs->addrs.count = addrs->next_index;

        dprintf(SRVLVL, "server_addrs_add() added new addr '%s'.\n",
            addr->uaddr);
    }
    ReleaseSRWLockExclusive(&addrs->lock);
}

void nfs41_server_addrs(
    IN nfs41_server *server,
    OUT multi_addr4 *addrs)
{
    struct server_addrs *saddrs = &server->addrs;
    uint32_t i, j;

    /* make a copy of the server's addrs, with most recent first */
    AcquireSRWLockShared(&saddrs->lock);
    j = saddrs->next_index;
    for (i = 0; i < saddrs->addrs.count; i++) {
        /* decrement/wrap j */
        j = (NFS41_ADDRS_PER_SERVER + j - 1) % NFS41_ADDRS_PER_SERVER;
        memcpy(&addrs->arr[i], &saddrs->addrs.arr[j], sizeof(netaddr4));
    }
    ReleaseSRWLockShared(&saddrs->lock);
}

int nfs41_server_find_or_create(
    IN const char *server_owner_major_id,
    IN const char *server_scope,
    IN const netaddr4 *addr,
    OUT nfs41_server **server_out)
{
    struct server_info info;
    struct list_entry *entry;
    nfs41_server *server;
    int status;

    info.owner = server_owner_major_id;
    info.scope = server_scope;

    dprintf(SRVLVL, "--> nfs41_server_find_or_create(%s)\n", info.owner);

    EnterCriticalSection(&g_server_list.lock);

    /* search for an existing server */
    entry = list_search(&g_server_list.head, &info, server_compare);
    if (entry == NULL) {
        /* create a new server */
        status = server_create(&info, &server);
        if (status == NO_ERROR) {
            /* add it to the list */
            list_add_tail(&g_server_list.head, &server->entry);
            *server_out = server;

            dprintf(SRVLVL, "<-- nfs41_server_find_or_create() "
                "returning new server %p\n", server);
        } else {
            dprintf(SRVLVL, "<-- nfs41_server_find_or_create() "
                "returning %d\n", status);
        }
    } else {
        server = server_entry(entry);
        status = NO_ERROR;

        dprintf(SRVLVL, "<-- nfs41_server_find_or_create() "
            "returning existing server %p\n", server);
    }

    if (server) {
        /* register the address used to connect */
        server_addrs_add(&server->addrs, addr);

        server_ref_locked(server);
    }

    *server_out = server;
    LeaveCriticalSection(&g_server_list.lock);
    return status;
}

int nfs41_server_resolve(
    IN const char *hostname,
    IN unsigned short port,
    OUT multi_addr4 *addrs)
{
    int status = ERROR_BAD_NET_NAME;
    char service[16];
    struct addrinfo hints = { 0 }, *res, *info;
    struct netconfig *nconf;
    struct netbuf addr;
    char *netid, *uaddr;

    dprintf(SRVLVL, "--> nfs41_server_resolve(%s:%u)\n",
        hostname, port);

    addrs->count = 0;

    StringCchPrintfA(service, 16, "%u", port);

    /* request a list of tcp addrs for the given hostname,port */
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, service, &hints, &res) != 0)
        goto out;

    for (info = res; info != NULL; info = info->ai_next) {
        /* find the appropriate entry in /etc/netconfig */
        switch (info->ai_family) {
        case AF_INET:  netid = "tcp";  break;
        case AF_INET6: netid = "tcp6"; break;
        default: continue;
        }

        nconf = getnetconfigent(netid);
        if (nconf == NULL)
            continue;

        /* convert to a transport-independent universal address */
        addr.buf = info->ai_addr;
        addr.maxlen = addr.len = (unsigned int)info->ai_addrlen;

        uaddr = taddr2uaddr(nconf, &addr);
        freenetconfigent(nconf);

        if (uaddr == NULL)
            continue;

        StringCchCopyA(addrs->arr[addrs->count].netid,
            NFS41_NETWORK_ID_LEN+1, netid);
        StringCchCopyA(addrs->arr[addrs->count].uaddr,
            NFS41_UNIVERSAL_ADDR_LEN+1, uaddr);
        freeuaddr(uaddr);

        status = NO_ERROR;
        if (++addrs->count >= NFS41_ADDRS_PER_SERVER)
            break;
    }
    freeaddrinfo(res);
out:
    if (status)
        dprintf(SRVLVL, "<-- nfs41_server_resolve(%s:%u) returning "
            "error %d\n", hostname, port, status);
    else
        dprintf(SRVLVL, "<-- nfs41_server_resolve(%s:%u) returning "
            "%s\n", hostname, port, addrs->arr[0].uaddr);
    return status;
}
