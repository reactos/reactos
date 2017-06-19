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

#ifndef __NFS41__
#define __NFS41__

#include "util.h"
#include "list.h"


struct __nfs41_session;
struct __nfs41_client;
struct __rpc_client;
struct __nfs41_root;

struct _FILE_GET_EA_INFORMATION;
struct _FILE_FULL_EA_INFORMATION;

typedef struct __nfs41_superblock {
    nfs41_fsid fsid;
    struct list_entry entry; /* position in nfs41_server.superblocks */

    bitmap4 supported_attrs;
    bitmap4 suppattr_exclcreat;
    bitmap4 default_getattr;

    nfstime4 time_delta;
    uint64_t maxread;
    uint64_t maxwrite;

    /* constant filesystem attributes */
    unsigned int layout_types : 3;
    unsigned int aclsupport : 3;
    unsigned int cansettime : 1;
    unsigned int link_support : 1;
    unsigned int symlink_support : 1;
    unsigned int ea_support : 1;
    unsigned int case_preserving : 1;
    unsigned int case_insensitive : 1;

    /* variable filesystem attributes */
    uint64_t space_avail;
    uint64_t space_free;
    uint64_t space_total;
    time_t cache_expiration; /* applies to space_ attributes */

    SRWLOCK lock;
} nfs41_superblock;

typedef struct __nfs41_superblock_list {
    struct list_entry head;
    SRWLOCK lock;
} nfs41_superblock_list;

struct server_addrs {
    multi_addr4 addrs; /* list of addrs we've used with this server */
    uint32_t next_index;
    SRWLOCK lock;
};

typedef struct __nfs41_server {
    char scope[NFS4_OPAQUE_LIMIT]; /* server_scope from exchangeid */
    char owner[NFS4_OPAQUE_LIMIT]; /* server_owner.major_id from exchangeid */
    struct server_addrs addrs;
    nfs41_superblock_list superblocks;
    struct nfs41_name_cache *name_cache;
    struct list_entry entry; /* position in global server list */
    LONG ref_count;
} nfs41_server;

enum delegation_status {
    DELEGATION_GRANTED,
    DELEGATION_RETURNING,
    DELEGATION_RETURNED,
};

typedef struct __nfs41_delegation_state {
    open_delegation4 state;
    nfs41_abs_path path;
    nfs41_path_fh parent;
    nfs41_path_fh file;
    struct list_entry client_entry; /* entry in nfs41_client.delegations */
    LONG ref_count;

    enum delegation_status status;
    SRWLOCK lock;
    CONDITION_VARIABLE cond;

    bool_t revoked; /* for recovery, accessed under client.state.lock */

    HANDLE srv_open; /* for rdbss cache invalidation */
} nfs41_delegation_state;

typedef struct __nfs41_lock_state {
    struct list_entry open_entry; /* entry in nfs41_open_state.locks */
    uint64_t offset;
    uint64_t length;
    uint32_t exclusive : 1;
    uint32_t delegated : 1; /* whether or not there is state on the server */
    uint32_t id : 30;
} nfs41_lock_state;

/* nfs41_open_state reference counting:
 * one reference is held implicitly by the driver (initialized to 1 on
 * OPEN and released on CLOSE). other references must be held during
 * upcalls to prevent a parallel CLOSE from freeing it prematurely. by
 * calling upcall_open_state_ref() when parsing the upcall, you are
 * guaranteed a matching dereference on upcall_cleanup() */
typedef struct __nfs41_open_state {
    nfs41_abs_path path;
    nfs41_path_fh parent;
    nfs41_path_fh file;
    nfs41_readdir_cookie cookie;
    struct __nfs41_session *session;
    uint32_t type;
    bool_t do_close;
    stateid4 stateid;
    state_owner4 owner;
    struct __pnfs_layout_state *layout;
    struct list_entry client_entry; /* entry in nfs41_client.opens */
    SRWLOCK lock;
    LONG ref_count;
    uint32_t share_access;
    uint32_t share_deny;
    uint64_t pnfs_last_offset; /* for layoutcommit */

    struct {
        nfs41_delegation_state *state;
        bool_t reclaim;
        CONDITION_VARIABLE cond;
    } delegation;

    struct { /* list of open lock state for recovery */
        stateid4 stateid;
        struct list_entry list;
        uint32_t counter;
        CRITICAL_SECTION lock;
    } locks;

    struct {
        struct _FILE_GET_EA_INFORMATION *list;
        uint32_t index;
        CRITICAL_SECTION lock;
    } ea;

    HANDLE srv_open; /* for data cache invalidation */
} nfs41_open_state;

typedef struct __nfs41_rpc_clnt {
    struct __rpc_client *rpc;
    SRWLOCK lock;
    HANDLE cond;
    struct __nfs41_client *client;
    multi_addr4 addrs;
    uint32_t addr_index; /* index of addr we're using */
    uint32_t wsize;
    uint32_t rsize;
    uint32_t version;
    uint32_t sec_flavor;
    uint32_t uid;
    uint32_t gid;
    char server_name[NI_MAXHOST];
    bool_t is_valid_session;
    bool_t in_recovery;
    bool_t needcb;
} nfs41_rpc_clnt;

struct client_state {
    struct list_entry opens; /* list of associated nfs41_open_state */
    struct list_entry delegations; /* list of associated delegations */
    CRITICAL_SECTION lock;
};

typedef struct __nfs41_client {
    nfs41_server *server;
    client_owner4 owner;
    uint64_t clnt_id;
    uint32_t seq_id;
    uint32_t roles;
    SRWLOCK exid_lock;
    struct __nfs41_session *session;
    SRWLOCK session_lock;
    nfs41_rpc_clnt *rpc;
    bool_t is_data;
    struct pnfs_layout_list *layouts;
    struct pnfs_file_device_list *devices;
    struct list_entry root_entry; /* position in nfs41_root.clients */
    struct __nfs41_root *root;

    struct {
        CONDITION_VARIABLE cond;
        CRITICAL_SECTION lock;
        bool_t in_recovery;
    } recovery;

    /* for state recovery on server reboot */
    struct client_state state;
} nfs41_client;

#define NFS41_MAX_NUM_SLOTS NFS41_MAX_RPC_REQS
typedef struct __nfs41_slot_table {
    uint32_t seq_nums[NFS41_MAX_NUM_SLOTS];
    uint32_t used_slots[NFS41_MAX_NUM_SLOTS];
    uint32_t max_slots;
    uint32_t highest_used;
    uint32_t num_used;
    ULONGLONG target_delay;
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE cond;
} nfs41_slot_table;

typedef struct __nfs41_channel_attrs {
    uint32_t                ca_headerpadsize;
    uint32_t                ca_maxrequestsize;
    uint32_t                ca_maxresponsesize;
    uint32_t                ca_maxresponsesize_cached;
    uint32_t                ca_maxoperations;
    uint32_t                ca_maxrequests;
    uint32_t                *ca_rdma_ird;
} nfs41_channel_attrs;

struct replay_cache {
    unsigned char buffer[NFS41_MAX_SERVER_CACHE];
    uint32_t length;
};

typedef struct __nfs41_cb_session {
    struct {
        struct replay_cache arg;
        struct replay_cache res;
    } replay;
    const unsigned char *cb_sessionid; /* -> nfs41_session.session_id */
    uint32_t cb_seqnum;
    uint32_t cb_slotid;
} nfs41_cb_session;

typedef struct __nfs41_session {
    nfs41_client *client;
    unsigned char session_id[NFS4_SESSIONID_SIZE];
    nfs41_channel_attrs fore_chan_attrs;
    nfs41_channel_attrs back_chan_attrs;
    uint32_t lease_time;
    nfs41_slot_table table;
    // array of slots
    HANDLE renew_thread;
    bool_t isValidState;
    uint32_t flags;
    nfs41_cb_session cb_session;
} nfs41_session;

/* nfs41_root reference counting:
 * similar to nfs41_open_state, the driver holds an implicit reference
 * between MOUNT and UNMOUNT. all other upcalls use upcall_root_ref() on
 * upcall_parse(), which prevents the root/clients from being freed and
 * guarantees a matching deref on upcall_cleanup() */
typedef struct __nfs41_root {
    client_owner4 client_owner;
    CRITICAL_SECTION lock;
    struct list_entry clients;
    uint32_t wsize;
    uint32_t rsize;
    LONG ref_count;
    uint32_t uid;
    uint32_t gid;
    DWORD sec_flavor;
} nfs41_root;


/* nfs41_namespace.c */
int nfs41_root_create(
    IN const char *name,
    IN uint32_t sec_flavor,
    IN uint32_t wsize,
    IN uint32_t rsize,
    OUT nfs41_root **root_out);

void nfs41_root_ref(
    IN nfs41_root *root);

void nfs41_root_deref(
    IN nfs41_root *root);

int nfs41_root_mount_addrs(
    IN nfs41_root *root,
    IN const multi_addr4 *addrs,
    IN bool_t is_data,
    IN OPTIONAL uint32_t lease_time,
    OUT nfs41_client **client_out);

int nfs41_root_mount_server(
    IN nfs41_root *root,
    IN nfs41_server *server,
    IN bool_t is_data,
    IN OPTIONAL uint32_t lease_time,
    OUT nfs41_client **client_out);

int nfs41_root_mount_referral(
    IN nfs41_root *root,
    IN const fs_locations4 *locations,
    OUT const fs_location4 **loc_out,
    OUT nfs41_client **client_out);

static __inline nfs41_session* nfs41_root_session(
    IN nfs41_root *root)
{
    nfs41_client *client;
    /* return a session for the server at the root of the namespace.
     * because we created it on mount, it's the first one in the list */
    EnterCriticalSection(&root->lock);
    client = list_container(root->clients.next, nfs41_client, root_entry);
    LeaveCriticalSection(&root->lock);
    return client->session;
}


/* nfs41_session.c */
int nfs41_session_create(
    IN nfs41_client *client,
    IN nfs41_session **session_out);

int nfs41_session_renew(
    IN nfs41_session *session);

int nfs41_session_set_lease(
    IN nfs41_session *session,
    IN uint32_t lease_time);

void nfs41_session_free(
    IN nfs41_session *session);

void nfs41_session_bump_seq(
    IN nfs41_session *session,
    IN uint32_t slotid,
    IN uint32_t target_highest_slotid);

void nfs41_session_free_slot(
    IN nfs41_session *session,
    IN uint32_t slotid);

void nfs41_session_get_slot(
    IN nfs41_session *session, 
    OUT uint32_t *slot, 
    OUT uint32_t *seq, 
    OUT uint32_t *highest);

int nfs41_session_recall_slot(
    IN nfs41_session *session,
    IN OUT uint32_t target_highest_slotid);

struct __nfs41_sequence_args;
void nfs41_session_sequence(
    struct __nfs41_sequence_args *args,
    nfs41_session *session,
    bool_t cachethis);

int nfs41_session_bad_slot(
    IN nfs41_session *session,
    IN OUT struct __nfs41_sequence_args *args);


/* nfs41_server.c */
void nfs41_server_list_init();

int nfs41_server_resolve(
    IN const char *hostname,
    IN unsigned short port,
    OUT multi_addr4 *addrs);

int nfs41_server_find_or_create(
    IN const char *server_owner_major_id,
    IN const char *server_scope,
    IN const netaddr4 *addr,
    OUT nfs41_server **server_out);

void nfs41_server_ref(
    IN nfs41_server *server);

void nfs41_server_deref(
    IN nfs41_server *server);

void nfs41_server_addrs(
    IN nfs41_server *server,
    OUT multi_addr4 *addrs);


/* nfs41_client.c */
int nfs41_client_owner(
    IN const char *name,
    IN uint32_t sec_flavor,
    OUT client_owner4 *owner);

uint32_t nfs41_exchange_id_flags(
    IN bool_t is_data);

struct __nfs41_exchange_id_res;

int nfs41_client_create(
    IN nfs41_rpc_clnt *rpc,
    IN const client_owner4 *owner,
    IN bool_t is_data,
    IN const struct __nfs41_exchange_id_res *exchangeid,
    OUT nfs41_client **client_out);

int nfs41_client_renew(
    IN nfs41_client *client);

void nfs41_client_free(
    IN nfs41_client *client);

static __inline nfs41_server* client_server(
    IN nfs41_client *client)
{
    /* the client's server could change during nfs41_client_renew(),
     * so access to client->server must be protected */
    nfs41_server *server;
    AcquireSRWLockShared(&client->exid_lock);
    server = client->server;
    ReleaseSRWLockShared(&client->exid_lock);
    return server;
}


/* nfs41_superblock.c */
int nfs41_superblock_for_fh(
    IN nfs41_session *session,
    IN const nfs41_fsid *fsid,
    IN const nfs41_fh *parent OPTIONAL,
    OUT nfs41_path_fh *file);

static __inline void nfs41_superblock_getattr_mask(
    IN const nfs41_superblock *superblock,
    OUT bitmap4 *attrs)
{
    memcpy(attrs, &superblock->default_getattr, sizeof(bitmap4));
}
static __inline void nfs41_superblock_supported_attrs(
    IN const nfs41_superblock *superblock,
    IN OUT bitmap4 *attrs)
{
    bitmap_intersect(attrs, &superblock->supported_attrs);
}
static __inline void nfs41_superblock_supported_attrs_exclcreat(
    IN const nfs41_superblock *superblock,
    IN OUT bitmap4 *attrs)
{
    bitmap_intersect(attrs, &superblock->suppattr_exclcreat);
}

struct _FILE_FS_ATTRIBUTE_INFORMATION;
void nfs41_superblock_fs_attributes(
    IN const nfs41_superblock *superblock,
    OUT struct _FILE_FS_ATTRIBUTE_INFORMATION *FsAttrs);

void nfs41_superblock_space_changed(
    IN nfs41_superblock *superblock);

void nfs41_superblock_list_init(
    IN nfs41_superblock_list *superblocks);

void nfs41_superblock_list_free(
    IN nfs41_superblock_list *superblocks);


/* nfs41_rpc.c */
int nfs41_rpc_clnt_create(
    IN const multi_addr4 *addrs,
    IN uint32_t wsize,
    IN uint32_t rsize,
    IN uint32_t uid,
    IN uint32_t gid,
    IN uint32_t sec_flavor,
    OUT nfs41_rpc_clnt **rpc_out);

void nfs41_rpc_clnt_free(
    IN nfs41_rpc_clnt *rpc);

int nfs41_send_compound(
    IN nfs41_rpc_clnt *rpc,
    IN char *inbuf,
    OUT char *outbuf);

static __inline netaddr4* nfs41_rpc_netaddr(
    IN nfs41_rpc_clnt *rpc)
{
    uint32_t id;
    AcquireSRWLockShared(&rpc->lock);
    /* only addr_index needs to be protected, as rpc->addrs is write-once */
    id = rpc->addr_index;
    ReleaseSRWLockShared(&rpc->lock);

    /* return the netaddr used to create the rpc client */
    return &rpc->addrs.arr[id];
}


/* open.c */
void nfs41_open_state_ref(
    IN nfs41_open_state *state);

void nfs41_open_state_deref(
    IN nfs41_open_state *state);

struct __stateid_arg;
void nfs41_open_stateid_arg(
    IN nfs41_open_state *state,
    OUT struct __stateid_arg *arg);


/* ea.c */
int nfs41_ea_set(
    IN nfs41_open_state *state,
    IN struct _FILE_FULL_EA_INFORMATION *ea);

#endif /* __NFS41__ */
