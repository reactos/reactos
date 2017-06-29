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

#include <winsock2.h>
#include <strsafe.h>
#include "pnfs.h"
#include "daemon_debug.h"


const char* pnfs_error_string(enum pnfs_status status)
{
    switch (status) {
    case PNFS_SUCCESS:          return "PNFS_SUCCESS";
    case PNFS_PENDING:          return "PNFS_PENDING";
    case PNFS_READ_EOF:         return "PNFS_READ_EOF";
    case PNFSERR_NOT_SUPPORTED: return "PNFSERR_NOT_SUPPORTED";
    case PNFSERR_NOT_CONNECTED: return "PNFSERR_NOT_CONNECTED";
    case PNFSERR_IO:            return "PNFSERR_IO";
    case PNFSERR_NO_DEVICE:     return "PNFSERR_NO_DEVICE";
    case PNFSERR_NO_LAYOUT:     return "PNFSERR_NO_LAYOUT";
    case PNFSERR_INVALID_FH_LIST: return "PNFSERR_INVALID_FH_LIST";
    case PNFSERR_INVALID_DS_INDEX: return "PNFSERR_INVALID_DS_INDEX";
    case PNFSERR_RESOURCES:     return "PNFSERR_RESOURCES";
    case PNFSERR_LAYOUT_RECALLED: return "PNFSERR_LAYOUT_RECALLED";
    case PNFSERR_LAYOUT_CHANGED: return "PNFSERR_LAYOUT_CHANGED";
    default:                    return "Invalid pnfs status";
    }
}

const char* pnfs_layout_type_string(enum pnfs_layout_type type)
{
    switch (type) {
    case PNFS_LAYOUTTYPE_FILE:  return "PNFS_LAYOUTTYPE_FILE";
    case PNFS_LAYOUTTYPE_OBJECT: return "PNFS_LAYOUTTYPE_OBJECT";
    case PNFS_LAYOUTTYPE_BLOCK: return "PNFS_LAYOUTTYPE_BLOCK";
    default:                    return "Invalid layout type";
    }
}

const char* pnfs_iomode_string(enum pnfs_iomode iomode)
{
    switch (iomode) {
    case PNFS_IOMODE_READ:      return "PNFS_IOMODE_READ";
    case PNFS_IOMODE_RW:        return "PNFS_IOMODE_RW";
    case PNFS_IOMODE_ANY:       return "PNFS_IOMODE_ANY";
    default:                    return "Invalid io mode";
    }
}

void dprint_deviceid(
    IN int level,
    IN const char *title,
    IN const unsigned char *deviceid)
{
    /* deviceid is 16 bytes, so print it as 4 uints */
    uint32_t *p = (uint32_t*)deviceid;
    dprintf(level, "%s%08X.%08X.%08X.%08X\n",
        title, htonl(p[0]), htonl(p[1]), htonl(p[2]), htonl(p[3]));
}

void dprint_layout(
    IN int level,
    IN const pnfs_file_layout *layout)
{
    dprintf(level, "  type:             %s\n", pnfs_layout_type_string(layout->layout.type));
    dprintf(level, "  iomode:           %s\n", pnfs_iomode_string(layout->layout.iomode));
    dprint_deviceid(level, "  deviceid:         ", layout->deviceid);
    dprintf(level, "  offset:           %llu\n", layout->layout.offset);
    dprintf(level, "  length:           %llu\n", layout->layout.length);
    dprintf(level, "  pattern_offset:   %llu\n", layout->pattern_offset);
    dprintf(level, "  first_index:      %u\n", layout->first_index);
    dprintf(level, "  dense:            %u\n", is_dense(layout));
    dprintf(level, "  commit_to_mds:    %u\n", should_commit_to_mds(layout));
    dprintf(level, "  stripe_unit_size: %u\n", layout_unit_size(layout));
    dprintf(level, "  file handles:     %u\n", layout->filehandles.count);
}

#define MULTI_ADDR_BUFFER_LEN \
    (NFS41_ADDRS_PER_SERVER*(NFS41_UNIVERSAL_ADDR_LEN+1)+1)

static void dprint_multi_addr(
    IN int level,
    IN uint32_t index,
    IN const multi_addr4 *addrs)
{
    char buffer[MULTI_ADDR_BUFFER_LEN] = "";
    uint32_t i;
    for (i = 0; i < addrs->count; i++) {
        StringCchCatA(buffer, MULTI_ADDR_BUFFER_LEN, addrs->arr[i].uaddr);
        StringCchCatA(buffer, MULTI_ADDR_BUFFER_LEN, " ");
    }
    dprintf(level, "  servers[%d]:       [ %s]\n", index, buffer);
}

void dprint_device(
    IN int level,
    IN const pnfs_file_device *device)
{
    uint32_t i;
    dprint_deviceid(level, "  deviceid:         ", device->device.deviceid);
    dprintf(level, "  type:             %s\n", pnfs_layout_type_string(device->device.type));
    dprintf(level, "  stripes:          %u\n", device->stripes.count);
    for (i = 0; i < device->servers.count; i++)
        dprint_multi_addr(level, i, &device->servers.arr[i].addrs);
}
