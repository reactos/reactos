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

#ifndef __PNFS_H__
#define __PNFS_H__

#include "nfs41_types.h"
#include "list.h"


/* preprocessor options */
#ifndef PNFS_DISABLE

# ifndef PNFS_DISABLE_READ
#  define PNFS_ENABLE_READ
# endif
# ifndef PNFS_DISABLE_WRITE
#  define PNFS_ENABLE_WRITE
# endif

# define PNFS_THREADING

/* XXX: the thread-by-server model breaks down when using dense layouts,
 * because multiple stripes could be mapped to a single data server, and
 * the per-data-server thread would have to send a COMMIT for each stripe */
//# define PNFS_THREAD_BY_SERVER

#endif


/* forward declarations from nfs41.h */
struct __nfs41_client;
struct __nfs41_session;
struct __nfs41_open_state;
struct __nfs41_root;
struct __stateid_arg;


/* pnfs error values, in order of increasing severity */
enum pnfs_status {
    PNFS_SUCCESS            = 0,
    PNFS_PENDING,
    PNFS_READ_EOF,
    PNFSERR_NOT_SUPPORTED,
    PNFSERR_NOT_CONNECTED,
    PNFSERR_IO,
    PNFSERR_NO_DEVICE,
    PNFSERR_NO_LAYOUT,
    PNFSERR_INVALID_FH_LIST,
    PNFSERR_INVALID_DS_INDEX,
    PNFSERR_RESOURCES,
    PNFSERR_LAYOUT_RECALLED,
    PNFSERR_LAYOUT_CHANGED,
};

enum pnfs_layout_type {
    PNFS_LAYOUTTYPE_FILE    = 1,
    PNFS_LAYOUTTYPE_OBJECT  = 2,
    PNFS_LAYOUTTYPE_BLOCK   = 3
};

enum pnfs_iomode {
    PNFS_IOMODE_READ        = 0x1,
    PNFS_IOMODE_RW          = 0x2,
    PNFS_IOMODE_ANY         = PNFS_IOMODE_READ | PNFS_IOMODE_RW
};

enum pnfs_layout_status {
    /* a LAYOUTGET error indicated that this layout will never be granted */
    PNFS_LAYOUT_UNAVAILABLE = 0x10,
    /* LAYOUTGET returned BADIOMODE, so a RW layout will never be granted */
    PNFS_LAYOUT_NOT_RW      = 0x20,
};

enum pnfs_device_status {
    /* GETDEVICEINFO was successful */
    PNFS_DEVICE_GRANTED     = 0x1,
    /* a bulk recall or lease expiration led to device invalidation */
    PNFS_DEVICE_REVOKED     = 0x2,
};

enum pnfs_return_type {
    PNFS_RETURN_FILE        = 1,
    PNFS_RETURN_FSID        = 2,
    PNFS_RETURN_ALL         = 3
};

#define NFL4_UFLG_MASK                  0x0000003F
#define NFL4_UFLG_DENSE                 0x00000001
#define NFL4_UFLG_COMMIT_THRU_MDS       0x00000002
#define NFL4_UFLG_STRIPE_UNIT_SIZE_MASK 0xFFFFFFC0

#define PNFS_DEVICEID_SIZE              16


/* device */
typedef struct __pnfs_device {
    unsigned char           deviceid[PNFS_DEVICEID_SIZE];
    enum pnfs_layout_type   type;
    enum pnfs_device_status status;
    uint32_t                layout_count; /* layouts using this device */
    CRITICAL_SECTION        lock;
} pnfs_device;

typedef struct __pnfs_stripe_indices {
    uint32_t                count;
    uint32_t                *arr;
} pnfs_stripe_indices;

typedef struct __pnfs_data_server {
    struct __nfs41_client   *client;
    multi_addr4             addrs;
    SRWLOCK                 lock;
} pnfs_data_server;

typedef struct __pnfs_data_server_list {
    uint32_t                count;
    pnfs_data_server        *arr;
} pnfs_data_server_list;

typedef struct __pnfs_file_device {
    pnfs_device             device;
    pnfs_stripe_indices     stripes;
    pnfs_data_server_list   servers;
    struct pnfs_file_device_list *devices; /* -> nfs41_client.devices */
    struct list_entry       entry; /* position in devices */
} pnfs_file_device;


/* layout */
typedef struct __pnfs_layout_state {
    nfs41_fh                meta_fh;
    stateid4                stateid;
    struct list_entry       entry; /* position in nfs41_client.layouts */
    struct list_entry       layouts; /* list of pnfs_file_layouts */
    struct list_entry       recalls; /* list of pnfs_layouts */
    enum pnfs_layout_status status;
    bool_t                  return_on_close;
    LONG                    open_count; /* for return on last close */
    uint32_t                io_count; /* number of pending io operations */
    bool_t                  pending; /* pending LAYOUTGET/LAYOUTRETURN */
    SRWLOCK                 lock;
    CONDITION_VARIABLE      cond;
} pnfs_layout_state;

typedef struct __pnfs_layout {
    struct list_entry       entry;
    uint64_t                offset;
    uint64_t                length;
    enum pnfs_iomode        iomode;
    enum pnfs_layout_type   type;
} pnfs_layout;

typedef struct __pnfs_file_layout_handles {
    uint32_t                count;
    nfs41_path_fh           *arr;
} pnfs_file_layout_handles;

typedef struct __pnfs_file_layout {
    pnfs_layout             layout;
    pnfs_file_layout_handles filehandles;
    unsigned char           deviceid[PNFS_DEVICEID_SIZE];
    pnfs_file_device        *device;
    uint64_t                pattern_offset;
    uint32_t                first_index;
    uint32_t                util;
} pnfs_file_layout;


/* pnfs_layout.c */
struct pnfs_layout_list;
struct cb_layoutrecall_args;

enum pnfs_status pnfs_layout_list_create(
    OUT struct pnfs_layout_list **layouts_out);

void pnfs_layout_list_free(
    IN struct pnfs_layout_list *layouts);

enum pnfs_status pnfs_layout_state_open(
    IN struct __nfs41_open_state *state,
    OUT pnfs_layout_state **layout_out);

/* expects caller to hold an exclusive lock on pnfs_layout_state */
enum pnfs_status pnfs_layout_state_prepare(
    IN pnfs_layout_state *state,
    IN struct __nfs41_session *session,
    IN nfs41_path_fh *meta_file,
    IN struct __stateid_arg *stateid,
    IN enum pnfs_iomode iomode,
    IN uint64_t offset,
    IN uint64_t length);

void pnfs_layout_state_close(
    IN struct __nfs41_session *session,
    IN struct __nfs41_open_state *state,
    IN bool_t remove);

enum pnfs_status pnfs_file_layout_recall(
    IN struct __nfs41_client *client,
    IN const struct cb_layoutrecall_args *recall);

/* expects caller to hold a shared lock on pnfs_layout_state */
enum pnfs_status pnfs_layout_recall_status(
    IN const pnfs_layout_state *state,
    IN const pnfs_layout *layout);

void pnfs_layout_recall_fenced(
    IN pnfs_layout_state *state,
    IN const pnfs_layout *layout);

/* expects caller to hold an exclusive lock on pnfs_layout_state */
void pnfs_layout_io_start(
    IN pnfs_layout_state *state);

void pnfs_layout_io_finished(
    IN pnfs_layout_state *state);


/* pnfs_device.c */
struct pnfs_file_device_list;

enum pnfs_status pnfs_file_device_list_create(
    OUT struct pnfs_file_device_list **devices_out);

void pnfs_file_device_list_free(
    IN struct pnfs_file_device_list *devices);

void pnfs_file_device_list_invalidate(
    IN struct pnfs_file_device_list *devices);

enum pnfs_status pnfs_file_device_get(
    IN struct __nfs41_session *session,
    IN struct pnfs_file_device_list *devices,
    IN unsigned char *deviceid,
    OUT pnfs_file_device **device_out);

void pnfs_file_device_put(
    IN pnfs_file_device *device);

struct notify_deviceid4; /* from nfs41_callback.h */
enum notify_deviceid_type4;
enum pnfs_status pnfs_file_device_notify(
    IN struct pnfs_file_device_list *devices,
    IN const struct notify_deviceid4 *change);

enum pnfs_status pnfs_data_server_client(
    IN struct __nfs41_root *root,
    IN pnfs_data_server *server,
    IN uint32_t default_lease,
    OUT struct __nfs41_client **client_out);


/* pnfs_io.c */
enum pnfs_status pnfs_read(
    IN struct __nfs41_root *root,
    IN struct __nfs41_open_state *state,
    IN struct __stateid_arg *stateid,
    IN pnfs_layout_state *layout,
    IN uint64_t offset,
    IN uint64_t length,
    OUT unsigned char *buffer_out,
    OUT ULONG *len_out);

enum pnfs_status pnfs_write(
    IN struct __nfs41_root *root,
    IN struct __nfs41_open_state *state,
    IN struct __stateid_arg *stateid,
    IN pnfs_layout_state *layout,
    IN uint64_t offset,
    IN uint64_t length,
    IN unsigned char *buffer,
    OUT ULONG *len_out,
    OUT nfs41_file_info *cinfo);


/* helper functions */
#ifndef __REACTOS__
__inline int is_dense(
#else
FORCEINLINE int is_dense(
#endif
    IN const pnfs_file_layout *layout)
{
    return (layout->util & NFL4_UFLG_DENSE) != 0;
}
#ifndef __REACTOS__
__inline int should_commit_to_mds(
#else
FORCEINLINE int should_commit_to_mds(
#endif
    IN const pnfs_file_layout *layout)
{
    return (layout->util & NFL4_UFLG_COMMIT_THRU_MDS) != 0;
}
#ifndef __REACTOS__
__inline uint32_t layout_unit_size(
#else
FORCEINLINE uint32_t layout_unit_size(
#endif
    IN const pnfs_file_layout *layout)
{
    return layout->util & NFL4_UFLG_STRIPE_UNIT_SIZE_MASK;
}
#ifndef __REACTOS__
__inline uint64_t stripe_unit_number(
#else
FORCEINLINE uint64_t stripe_unit_number(
#endif
    IN const pnfs_file_layout *layout,
    IN uint64_t offset,
    IN uint32_t unit_size)
{
    const uint64_t relative_offset = offset - layout->pattern_offset;
    return relative_offset / unit_size;
}
#ifndef __REACTOS__
__inline uint64_t stripe_unit_offset(
#else
FORCEINLINE uint64_t stripe_unit_offset(
#endif
    IN const pnfs_file_layout *layout,
    IN uint64_t sui,
    IN uint32_t unit_size)
{
    return layout->pattern_offset + unit_size * sui;
}
#ifndef __REACTOS__
__inline uint32_t stripe_index(
#else
FORCEINLINE uint32_t stripe_index(
#endif
    IN const pnfs_file_layout *layout,
    IN uint64_t sui,
    IN uint32_t stripe_count)
{
    return (uint32_t)((sui + layout->first_index) % stripe_count);
}
#ifndef __REACTOS__
__inline uint32_t data_server_index(
#else
FORCEINLINE uint32_t data_server_index(
#endif
    IN const pnfs_file_device *device,
    IN uint32_t stripeid)
{
    return device->stripes.arr[stripeid];
}

#endif /* !__PNFS_H__ */
