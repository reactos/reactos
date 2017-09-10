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

#ifndef __NFS41_DAEMON_COMPOUND_H__
#define __NFS41_DAEMON_COMPOUND_H__

#include "nfs41.h"


/* COMPOUND */
typedef struct __nfs_argop4 {
    uint32_t                op;
    void                    *arg;
} nfs_argop4;

typedef struct __nfs41_compound_args {
    uint32_t                tag_len;
    unsigned char           tag[NFS4_OPAQUE_LIMIT];
    uint32_t                minorversion;
    uint32_t                argarray_count;
    nfs_argop4              *argarray; /* <> */
} nfs41_compound_args;

typedef struct __nfs_resop4 {
    uint32_t                op;
    void                    *res;
} nfs_resop4;

typedef struct __nfs41_compound_res {
    uint32_t                status;
    uint32_t                tag_len;
    unsigned char           tag[NFS4_OPAQUE_LIMIT];
    uint32_t                resarray_count;
    nfs_resop4              *resarray; /* <> */
} nfs41_compound_res;

typedef struct __nfs41_compound {
    nfs41_compound_args     args;
    nfs41_compound_res      res;
} nfs41_compound;


int compound_error(int status);

void compound_init(
    nfs41_compound *compound,
    nfs_argop4 *argops,
    nfs_resop4 *resops,
    const char *tag);

void compound_add_op(
    nfs41_compound *compound,
    uint32_t opnum,
    void *arg,
    void *res);

int compound_encode_send_decode(
    nfs41_session *session,
    nfs41_compound *compound,
    bool_t try_recovery);

#endif /* __NFS41_DAEMON_COMPOUND_H__ */
