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

#ifndef IDMAP_H
#define IDMAP_H

#include "nfs41_types.h"


/* idmap.c */
typedef struct idmap_context nfs41_idmapper;

int nfs41_idmap_create(
    nfs41_idmapper **context_out);

void nfs41_idmap_free(
    nfs41_idmapper *context);


int nfs41_idmap_name_to_ids(
    nfs41_idmapper *context,
    const char *username,
    uid_t *uid_out,
    gid_t *gid_out);

int nfs41_idmap_uid_to_name(
    nfs41_idmapper *context,
    uid_t uid,
    char *name_out,
    size_t len);

int nfs41_idmap_principal_to_ids(
    nfs41_idmapper *context,
    const char *principal,
    uid_t *uid_out,
    gid_t *gid_out);

int nfs41_idmap_group_to_gid(
    nfs41_idmapper *context,
    const char *name,
    gid_t *gid_out);

int nfs41_idmap_gid_to_group(
    nfs41_idmapper *context,
    gid_t gid,
    char *name_out,
    size_t len);

#endif /* !IDMAP_H */
