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
#include <time.h>

#include "nfs41_ops.h"
#include "from_kernel.h"
#include "upcall.h"
#include "util.h"
#include "daemon_debug.h"


/* windows volume queries want size in 'units', so we have to
 * convert the nfs space_* attributes from bytes to units */
#define SECTORS_PER_UNIT    8
#define BYTES_PER_SECTOR    512
#define BYTES_PER_UNIT      (SECTORS_PER_UNIT * BYTES_PER_SECTOR)

#define TO_UNITS(bytes) (bytes / BYTES_PER_UNIT)

#define VOLUME_CACHE_EXPIRATION 20


/* NFS41_VOLUME_QUERY */
static int parse_volume(unsigned char *buffer, uint32_t length, nfs41_upcall *upcall)
{
    int status;
    volume_upcall_args *args = &upcall->args.volume;

    status = safe_read(&buffer, &length, &args->query, sizeof(FS_INFORMATION_CLASS));
    if (status) goto out;

    dprintf(1, "parsing NFS41_VOLUME_QUERY: query=%d\n", args->query);
out:
    return status;
}

static int get_volume_size_info(
    IN nfs41_open_state *state,
    IN const char *query,
    OUT OPTIONAL PLONGLONG total_out,
    OUT OPTIONAL PLONGLONG user_out,
    OUT OPTIONAL PLONGLONG avail_out)
{
    nfs41_file_info info = { 0 };
    nfs41_superblock *superblock = state->file.fh.superblock;
    int status = ERROR_NOT_FOUND;

    AcquireSRWLockShared(&superblock->lock);
    /* check superblock for cached attributes */
    if (time(NULL) <= superblock->cache_expiration) {
        info.space_total = superblock->space_total;
        info.space_avail = superblock->space_avail;
        info.space_free = superblock->space_free;
        status = NO_ERROR;

        dprintf(2, "%s cached: %llu user, %llu free of %llu total\n",
            query, info.space_avail, info.space_free, info.space_total);
    }
    ReleaseSRWLockShared(&superblock->lock);

    if (status) {
        bitmap4 attr_request = { 2, { 0, FATTR4_WORD1_SPACE_AVAIL |
            FATTR4_WORD1_SPACE_FREE | FATTR4_WORD1_SPACE_TOTAL } };

        /* query the space_ attributes of the filesystem */
        status = nfs41_getattr(state->session, &state->file,
            &attr_request, &info);
        if (status) {
            eprintf("nfs41_getattr() failed with %s\n",
                nfs_error_string(status));
            status = nfs_to_windows_error(status, ERROR_BAD_NET_RESP);
            goto out;
        }

        AcquireSRWLockExclusive(&superblock->lock);
        superblock->space_total = info.space_total;
        superblock->space_avail = info.space_avail;
        superblock->space_free = info.space_free;
        superblock->cache_expiration = time(NULL) + VOLUME_CACHE_EXPIRATION;
        ReleaseSRWLockExclusive(&superblock->lock);

        dprintf(2, "%s: %llu user, %llu free of %llu total\n",
            query, info.space_avail, info.space_free, info.space_total);
    }

    if (total_out) *total_out = TO_UNITS(info.space_total);
    if (user_out) *user_out = TO_UNITS(info.space_avail);
    if (avail_out) *avail_out = TO_UNITS(info.space_free);
out:
    return status;
}

static int handle_volume(nfs41_upcall *upcall)
{
    volume_upcall_args *args = &upcall->args.volume;
    int status = NO_ERROR;

    switch (args->query) {
    case FileFsSizeInformation:
        args->len = sizeof(args->info.size);
        args->info.size.SectorsPerAllocationUnit = SECTORS_PER_UNIT;
        args->info.size.BytesPerSector = BYTES_PER_SECTOR;

        status = get_volume_size_info(upcall->state_ref,
            "FileFsSizeInformation",
            &args->info.size.TotalAllocationUnits.QuadPart,
            &args->info.size.AvailableAllocationUnits.QuadPart,
            NULL);
        break;

    case FileFsFullSizeInformation:
        args->len = sizeof(args->info.fullsize);
        args->info.fullsize.SectorsPerAllocationUnit = SECTORS_PER_UNIT;
        args->info.fullsize.BytesPerSector = BYTES_PER_SECTOR;

        status = get_volume_size_info(upcall->state_ref,
            "FileFsFullSizeInformation",
            &args->info.fullsize.TotalAllocationUnits.QuadPart,
            &args->info.fullsize.CallerAvailableAllocationUnits.QuadPart,
            &args->info.fullsize.ActualAvailableAllocationUnits.QuadPart);
        break;

    case FileFsAttributeInformation:
        args->len = sizeof(args->info.attribute);
        nfs41_superblock_fs_attributes(upcall->state_ref->file.fh.superblock,
            &args->info.attribute);
        break;

    default:
        eprintf("unhandled fs query class %d\n", args->query);
        status = ERROR_INVALID_PARAMETER;
        break;
    }
    return status;
}

static int marshall_volume(unsigned char *buffer, uint32_t *length, nfs41_upcall *upcall)
{
    int status;
    volume_upcall_args *args = &upcall->args.volume;

    status = safe_write(&buffer, length, &args->len, sizeof(args->len));
    if (status) goto out;
    status = safe_write(&buffer, length, &args->info, args->len);
out:
    return status;
}


const nfs41_upcall_op nfs41_op_volume = {
    parse_volume,
    handle_volume,
    marshall_volume
};
