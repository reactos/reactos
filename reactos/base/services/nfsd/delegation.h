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

#ifndef DELEGATION_H
#define DELEGATION_H

#include "nfs41.h"


/* option to avoid conflicts by returning the delegation */
#define DELEGATION_RETURN_ON_CONFLICT


/* reference counting and cleanup */
void nfs41_delegation_ref(
    IN nfs41_delegation_state *state);

void nfs41_delegation_deref(
    IN nfs41_delegation_state *state);

void nfs41_client_delegation_free(
    IN nfs41_client *client);


/* open delegation */
int nfs41_delegation_granted(
    IN nfs41_session *session,
    IN nfs41_path_fh *parent,
    IN nfs41_path_fh *file,
    IN open_delegation4 *delegation,
    IN bool_t try_recovery,
    OUT nfs41_delegation_state **deleg_out);

int nfs41_delegate_open(
    IN nfs41_open_state *state,
    IN uint32_t create,
    IN OPTIONAL nfs41_file_info *createattrs,
    OUT nfs41_file_info *info);

int nfs41_delegation_to_open(
    IN nfs41_open_state *open,
    IN bool_t try_recovery);

void nfs41_delegation_remove_srvopen(
    IN nfs41_session *session,
    IN nfs41_path_fh *file);

/* synchronous delegation return */
#ifdef DELEGATION_RETURN_ON_CONFLICT
int nfs41_delegation_return(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
#ifndef __REACTOS__
    IN enum open_delegation_type4 access,
#else
    IN int access,
#endif
    IN bool_t truncate);
#else
static int nfs41_delegation_return(
    IN nfs41_session *session,
    IN nfs41_path_fh *file,
    IN enum open_delegation_type4 access,
    IN bool_t truncate)
{
    return NFS4_OK;
}
#endif


/* asynchronous delegation recall */
int nfs41_delegation_recall(
    IN nfs41_client *client,
    IN nfs41_fh *fh,
    IN const stateid4 *stateid,
    IN bool_t truncate);

int nfs41_delegation_getattr(
    IN nfs41_client *client,
    IN const nfs41_fh *fh,
    IN const bitmap4 *attr_request,
    OUT nfs41_file_info *info);


/* after client state recovery, return any 'recalled' delegations;
 * must be called under the client's state lock */
int nfs41_client_delegation_recovery(
    IN nfs41_client *client);

/* attempt to return the least recently used delegation;
 * fails with NFS4ERR_BADHANDLE if all delegations are in use */
int nfs41_client_delegation_return_lru(
    IN nfs41_client *client);

#endif /* DELEGATION_H */
