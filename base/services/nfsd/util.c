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
#include <stdlib.h>
#include <wincrypt.h> /* for Crypt*() functions */

#include "daemon_debug.h"
#include "util.h"
#include "nfs41_ops.h"


int safe_read(unsigned char **pos, uint32_t *remaining, void *dest, uint32_t dest_len) 
{
    if (*remaining < dest_len)
        return ERROR_BUFFER_OVERFLOW;

    CopyMemory(dest, *pos, dest_len);
    *pos += dest_len;
    *remaining -= dest_len;
    return 0;
}

int safe_write(unsigned char **pos, uint32_t *remaining, void *src, uint32_t src_len)
{
    if (*remaining < src_len)
        return ERROR_BUFFER_OVERFLOW;

    CopyMemory(*pos, src, src_len);
    *pos += src_len;
    *remaining -= src_len;
    return 0;
}

int get_name(unsigned char **pos, uint32_t *remaining, const char **out_name)
{
    int status;
    USHORT len;
    
    status = safe_read(pos, remaining, &len, sizeof(USHORT));
    if (status) goto out;
    if (*remaining < len) {
        status = ERROR_BUFFER_OVERFLOW;
        goto out;
    }
    *out_name = (const char*)*pos;
    *pos += len;
    *remaining -= len;
out:
    return status;
}

const char* strip_path(
    IN const char *path,
    OUT uint32_t *len_out)
{
    const char *name = strrchr(path, '\\');
    name = name ? name + 1 : path;
    if (len_out)
        *len_out = (uint32_t)strlen(name);
    return name;
}

uint32_t max_read_size(
    IN const nfs41_session *session,
    IN const nfs41_fh *fh)
{
    const uint32_t maxresponse = session->fore_chan_attrs.ca_maxresponsesize;
    return (uint32_t)min(fh->superblock->maxread, maxresponse - READ_OVERHEAD);
}

uint32_t max_write_size(
    IN const nfs41_session *session,
    IN const nfs41_fh *fh)
{
    const uint32_t maxrequest = session->fore_chan_attrs.ca_maxrequestsize;
    return (uint32_t)min(fh->superblock->maxwrite, maxrequest - WRITE_OVERHEAD);
}

bool_t verify_write(
    IN nfs41_write_verf *verf,
    IN OUT enum stable_how4 *stable)
{
    if (verf->committed != UNSTABLE4) {
        *stable = verf->committed;
        dprintf(3, "verify_write: committed to stable storage\n");
        return 1;
    }

    if (*stable != UNSTABLE4) {
        memcpy(verf->expected, verf->verf, NFS4_VERIFIER_SIZE);
        *stable = UNSTABLE4;
        dprintf(3, "verify_write: first unstable write, saving verifier\n");
        return 1;
    }

    if (memcmp(verf->expected, verf->verf, NFS4_VERIFIER_SIZE) == 0) {
        dprintf(3, "verify_write: verifier matches expected\n");
        return 1;
    }

    dprintf(2, "verify_write: verifier changed; writes have been lost!\n");
    return 0;
}

bool_t verify_commit(
    IN nfs41_write_verf *verf)
{
    if (memcmp(verf->expected, verf->verf, NFS4_VERIFIER_SIZE) == 0) {
        dprintf(3, "verify_commit: verifier matches expected\n");
        return 1;
    }
    dprintf(2, "verify_commit: verifier changed; writes have been lost!\n");
    return 0;
}

ULONG nfs_file_info_to_attributes(
    IN const nfs41_file_info *info)
{
    ULONG attrs = 0;
    if (info->type == NF4DIR)
        attrs |= FILE_ATTRIBUTE_DIRECTORY;
    else if (info->type == NF4LNK) {
        attrs |= FILE_ATTRIBUTE_REPARSE_POINT;
        if (info->symlink_dir)
            attrs |= FILE_ATTRIBUTE_DIRECTORY;
    } else if (info->type != NF4REG)
        dprintf(1, "unhandled file type %d, defaulting to NF4REG\n",
            info->type);

    if (info->mode == 0444) /* XXX: 0444 for READONLY */
        attrs |= FILE_ATTRIBUTE_READONLY;

    if (info->hidden) attrs |= FILE_ATTRIBUTE_HIDDEN;
    if (info->system) attrs |= FILE_ATTRIBUTE_SYSTEM;
    if (info->archive) attrs |= FILE_ATTRIBUTE_ARCHIVE;

    // FILE_ATTRIBUTE_NORMAL attribute is only set if no other attributes are present.
    // all other override this value.
    return attrs ? attrs : FILE_ATTRIBUTE_NORMAL;
}

void nfs_to_basic_info(
    IN const nfs41_file_info *info,
    OUT PFILE_BASIC_INFO basic_out)
{
    nfs_time_to_file_time(&info->time_create, &basic_out->CreationTime);
    nfs_time_to_file_time(&info->time_access, &basic_out->LastAccessTime);
    nfs_time_to_file_time(&info->time_modify, &basic_out->LastWriteTime);
    /* XXX: was using 'change' attr, but that wasn't giving a time */
    nfs_time_to_file_time(&info->time_modify, &basic_out->ChangeTime);
    basic_out->FileAttributes = nfs_file_info_to_attributes(info);
}

void nfs_to_standard_info(
    IN const nfs41_file_info *info,
    OUT PFILE_STANDARD_INFO std_out)
{
    const ULONG FileAttributes = nfs_file_info_to_attributes(info);

    std_out->AllocationSize.QuadPart =
        std_out->EndOfFile.QuadPart = (LONGLONG)info->size;
    std_out->NumberOfLinks = info->numlinks;
    std_out->DeletePending = FALSE;
    std_out->Directory = FileAttributes & FILE_ATTRIBUTE_DIRECTORY ?
        TRUE : FALSE;
}

void nfs_to_network_openinfo(
    IN const nfs41_file_info *info,
    OUT PFILE_NETWORK_OPEN_INFORMATION net_out)
{
    
    nfs_time_to_file_time(&info->time_create, &net_out->CreationTime);
    nfs_time_to_file_time(&info->time_access, &net_out->LastAccessTime);
    nfs_time_to_file_time(&info->time_modify, &net_out->LastWriteTime);
    /* XXX: was using 'change' attr, but that wasn't giving a time */
    nfs_time_to_file_time(&info->time_modify, &net_out->ChangeTime);
    net_out->AllocationSize.QuadPart =
        net_out->EndOfFile.QuadPart = (LONGLONG)info->size;
    net_out->FileAttributes = nfs_file_info_to_attributes(info);
}

void get_file_time(
    OUT PLARGE_INTEGER file_time)
{
    GetSystemTimeAsFileTime((LPFILETIME)file_time);
}

void get_nfs_time(
    OUT nfstime4 *nfs_time)
{
    LARGE_INTEGER file_time;
    get_file_time(&file_time);
    file_time_to_nfs_time(&file_time, nfs_time);
}

bool_t multi_addr_find(
    IN const multi_addr4 *addrs,
    IN const netaddr4 *addr,
    OUT OPTIONAL uint32_t *index_out)
{
    uint32_t i;
    for (i = 0; i < addrs->count; i++) {
        const netaddr4 *saddr = &addrs->arr[i];
        if (!strncmp(saddr->netid, addr->netid, NFS41_NETWORK_ID_LEN) &&
            !strncmp(saddr->uaddr, addr->uaddr, NFS41_UNIVERSAL_ADDR_LEN)) {
            if (index_out) *index_out = i;
            return 1;
        }
    }
    return 0;
}

int nfs_to_windows_error(int status, int default_error)
{
    /* make sure this is actually an nfs error */
    if (status < 0 || (status > 70 && status < 10001) || status > 10087) {
        eprintf("nfs_to_windows_error called with non-nfs "
            "error code %d; returning the error as is\n", status);
        return status;
    }

    switch (status) {
    case NFS4_OK:               return NO_ERROR;
    case NFS4ERR_PERM:          return ERROR_ACCESS_DENIED;
    case NFS4ERR_NOENT:         return ERROR_FILE_NOT_FOUND;
    case NFS4ERR_IO:            return ERROR_NET_WRITE_FAULT;
    case NFS4ERR_ACCESS:        return ERROR_ACCESS_DENIED;
    case NFS4ERR_EXIST:         return ERROR_FILE_EXISTS;
    case NFS4ERR_XDEV:          return ERROR_NOT_SAME_DEVICE;
    case NFS4ERR_INVAL:         return ERROR_INVALID_PARAMETER;
    case NFS4ERR_FBIG:          return ERROR_FILE_TOO_LARGE;
    case NFS4ERR_NOSPC:         return ERROR_DISK_FULL;
    case NFS4ERR_ROFS:          return ERROR_NETWORK_ACCESS_DENIED;
    case NFS4ERR_MLINK:         return ERROR_TOO_MANY_LINKS;
    case NFS4ERR_NAMETOOLONG:   return ERROR_FILENAME_EXCED_RANGE;
    case NFS4ERR_STALE:         return ERROR_NETNAME_DELETED;
    case NFS4ERR_NOTEMPTY:      return ERROR_NOT_EMPTY;
    case NFS4ERR_DENIED:        return ERROR_LOCK_FAILED;
    case NFS4ERR_TOOSMALL:      return ERROR_BUFFER_OVERFLOW;
    case NFS4ERR_LOCKED:        return ERROR_LOCK_VIOLATION;
    case NFS4ERR_SHARE_DENIED:  return ERROR_SHARING_VIOLATION;
    case NFS4ERR_LOCK_RANGE:    return ERROR_NOT_LOCKED;
    case NFS4ERR_ATTRNOTSUPP:   return ERROR_NOT_SUPPORTED;
    case NFS4ERR_OPENMODE:      return ERROR_ACCESS_DENIED;
    case NFS4ERR_LOCK_NOTSUPP:  return ERROR_ATOMIC_LOCKS_NOT_SUPPORTED;

    case NFS4ERR_BADCHAR:
    case NFS4ERR_BADNAME:       return ERROR_INVALID_NAME;

    case NFS4ERR_NOTDIR:
    case NFS4ERR_ISDIR:
    case NFS4ERR_SYMLINK:
    case NFS4ERR_WRONG_TYPE:    return ERROR_INVALID_PARAMETER;

    case NFS4ERR_EXPIRED:
    case NFS4ERR_NOFILEHANDLE:
    case NFS4ERR_OLD_STATEID:
    case NFS4ERR_BAD_STATEID:
    case NFS4ERR_ADMIN_REVOKED: return ERROR_FILE_INVALID;

    case NFS4ERR_WRONGSEC:      return ERROR_ACCESS_DENIED;

    default:
        dprintf(1, "nfs error %s not mapped to windows error; "
            "returning default error %d\n",
            nfs_error_string(status), default_error);
        return default_error;
    }
}

int map_symlink_errors(int status)
{
    switch (status) {
    case NFS4ERR_BADCHAR:
    case NFS4ERR_BADNAME:       return ERROR_INVALID_REPARSE_DATA;
    case NFS4ERR_WRONG_TYPE:    return ERROR_NOT_A_REPARSE_POINT;
    case NFS4ERR_ACCESS:        return ERROR_ACCESS_DENIED;
    case NFS4ERR_NOTEMPTY:      return ERROR_NOT_EMPTY;
    default: return nfs_to_windows_error(status, ERROR_BAD_NET_RESP);
    }
}

bool_t next_component(
    IN const char *path,
    IN const char *path_end,
    OUT nfs41_component *component)
{
    const char *component_end;
    component->name = next_non_delimiter(path, path_end);
    component_end = next_delimiter(component->name, path_end);
    component->len = (unsigned short)(component_end - component->name);
    return component->len > 0;
}

bool_t last_component(
    IN const char *path,
    IN const char *path_end,
    OUT nfs41_component *component)
{
    const char *component_end = prev_delimiter(path_end, path);
    component->name = prev_non_delimiter(component_end, path);
    component->name = prev_delimiter(component->name, path);
    component->name = next_non_delimiter(component->name, component_end);
    component->len = (unsigned short)(component_end - component->name);
    return component->len > 0;
}

bool_t is_last_component(
    IN const char *path,
    IN const char *path_end)
{
    path = next_delimiter(path, path_end);
    return next_non_delimiter(path, path_end) == path_end;
}

void abs_path_copy(
    OUT nfs41_abs_path *dst,
    IN const nfs41_abs_path *src)
{
    dst->len = src->len;
    StringCchCopyNA(dst->path, NFS41_MAX_PATH_LEN, src->path, dst->len);
}

void path_fh_init(
    OUT nfs41_path_fh *file,
    IN nfs41_abs_path *path)
{
    file->path = path;
    last_component(path->path, path->path + path->len, &file->name);
}

void fh_copy(
    OUT nfs41_fh *dst,
    IN const nfs41_fh *src)
{
    dst->fileid = src->fileid;
    dst->superblock = src->superblock;
    dst->len = src->len;
    memcpy(dst->fh, src->fh, dst->len);
}

void path_fh_copy(
    OUT nfs41_path_fh *dst,
    IN const nfs41_path_fh *src)
{
    dst->path = src->path;
    if (dst->path) {
        const size_t name_start = src->name.name - src->path->path;
        dst->name.name = dst->path->path + name_start;
        dst->name.len = src->name.len;
    } else {
        dst->name.name = NULL;
        dst->name.len = 0;
    }
    fh_copy(&dst->fh, &src->fh);
}

int create_silly_rename(
    IN nfs41_abs_path *path,
    IN const nfs41_fh *fh,
    OUT nfs41_component *silly)
{
    HCRYPTPROV context;
    HCRYPTHASH hash;
    PBYTE buffer;
    DWORD length;
    const char *end = path->path + NFS41_MAX_PATH_LEN;
    const unsigned short extra_len = 2 + 16; //md5 is 16
    char name[NFS41_MAX_COMPONENT_LEN+1];
    unsigned char fhmd5[17] = { 0 };
    char *tmp;
    int status = NO_ERROR, i;

    if (path->len + extra_len >= NFS41_MAX_PATH_LEN) {
        status = ERROR_FILENAME_EXCED_RANGE;
        goto out;
    }

    /* set up the md5 hash generator */
    if (!CryptAcquireContext(&context, NULL, NULL,
        PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        status = GetLastError();
        eprintf("CryptAcquireContext() failed with %d\n", status);
        goto out;
    }
    if (!CryptCreateHash(context, CALG_MD5, 0, 0, &hash)) {
        status = GetLastError();
        eprintf("CryptCreateHash() failed with %d\n", status);
        goto out_context;
    }

    if (!CryptHashData(hash, (const BYTE*)fh->fh, (DWORD)fh->len, 0)) {
        status = GetLastError();
        eprintf("CryptHashData() failed with %d\n", status);
        goto out_hash;
    }

    /* extract the hash buffer */
    buffer = (PBYTE)fhmd5;
    length = 16;
    if (!CryptGetHashParam(hash, HP_HASHVAL, buffer, &length, 0)) {
        status = GetLastError();
        eprintf("CryptGetHashParam(val) failed with %d\n", status);
        goto out_hash;
    }    

    last_component(path->path, path->path + path->len, silly);
    StringCchCopyNA(name, NFS41_MAX_COMPONENT_LEN+1, silly->name, silly->len);

    tmp = (char*)silly->name;
    StringCchPrintf(tmp, end - tmp, ".%s.", name);
    tmp += silly->len + 2;

    for (i = 0; i < 16; i++, tmp++)
        StringCchPrintf(tmp, end - tmp, "%x", fhmd5[i]);

    path->len = path->len + extra_len;
    silly->len = silly->len + extra_len;

out_hash:
    CryptDestroyHash(hash);
out_context:
    CryptReleaseContext(context, 0);
out:
    return status;
}
