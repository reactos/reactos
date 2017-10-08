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

#ifndef __NFS41_DAEMON_NAME_CACHE_H__
#define __NFS41_DAEMON_NAME_CACHE_H__

#include "nfs41.h"


static __inline struct nfs41_name_cache* client_name_cache(
    IN nfs41_client *client)
{
    return client_server(client)->name_cache;
}

static __inline struct nfs41_name_cache* session_name_cache(
    IN nfs41_session *session)
{
    return client_name_cache(session->client);
}


/* attribute cache */
int nfs41_attr_cache_lookup(
    IN struct nfs41_name_cache *cache,
    IN uint64_t fileid,
    OUT nfs41_file_info *info_out);

int nfs41_attr_cache_update(
    IN struct nfs41_name_cache *cache,
    IN uint64_t fileid,
    IN const nfs41_file_info *info);


/* name cache */
int nfs41_name_cache_create(
    OUT struct nfs41_name_cache **cache_out);

int nfs41_name_cache_free(
    IN OUT struct nfs41_name_cache **cache_out);

int nfs41_name_cache_lookup(
    IN struct nfs41_name_cache *cache,
    IN const char *path,
    IN const char *path_end,
    OUT OPTIONAL const char **remaining_path_out,
    OUT OPTIONAL nfs41_fh *parent_out,
    OUT OPTIONAL nfs41_fh *target_out,
    OUT OPTIONAL nfs41_file_info *info_out,
    OUT OPTIONAL bool_t *is_negative);

int nfs41_name_cache_insert(
    IN struct nfs41_name_cache *cache,
    IN const char *path,
    IN const nfs41_component *name,
    IN OPTIONAL const nfs41_fh *fh,
    IN OPTIONAL const nfs41_file_info *info,
    IN OPTIONAL const change_info4 *cinfo,
    IN enum open_delegation_type4 delegation);

int nfs41_name_cache_delegreturn(
    IN struct nfs41_name_cache *cache,
    IN uint64_t fileid,
    IN const char *path,
    IN const nfs41_component *name);

int nfs41_name_cache_remove(
    IN struct nfs41_name_cache *cache,
    IN const char *path,
    IN const nfs41_component *name,
    IN uint64_t fileid,
    IN const change_info4 *cinfo);

int nfs41_name_cache_rename(
    IN struct nfs41_name_cache *cache,
    IN const char *src_path,
    IN const nfs41_component *src_name,
    IN const change_info4 *src_cinfo,
    IN const char *dst_path,
    IN const nfs41_component *dst_name,
    IN const change_info4 *dst_cinfo);

int nfs41_name_cache_remove_stale(
    IN struct nfs41_name_cache *cache,
    IN nfs41_session *session,
    IN nfs41_abs_path *path);

#endif /* !__NFS41_DAEMON_NAME_CACHE_H__ */
