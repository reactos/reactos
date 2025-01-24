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

#ifndef __NFS41_DAEMON_UTIL_H__
#define __NFS41_DAEMON_UTIL_H__

#include "nfs41_types.h"
#include "from_kernel.h"

extern DWORD NFS41D_VERSION;
struct __nfs41_session;
struct __nfs41_write_verf;
enum stable_how4;

int safe_read(unsigned char **pos, uint32_t *remaining, void *dest, uint32_t dest_len);
int safe_write(unsigned char **pos, uint32_t *remaining, void *dest, uint32_t dest_len);
int get_name(unsigned char **pos, uint32_t *remaining, const char **out_name);

const char* strip_path(
    IN const char *path,
    OUT uint32_t *len_out OPTIONAL);

uint32_t max_read_size(
    IN const struct __nfs41_session *session,
    IN const nfs41_fh *fh);
uint32_t max_write_size(
    IN const struct __nfs41_session *session,
    IN const nfs41_fh *fh);

bool_t verify_write(
    IN nfs41_write_verf *verf,
    IN OUT enum stable_how4 *stable);
bool_t verify_commit(
    IN nfs41_write_verf *verf);

/* bitmap4 */
static __inline bool_t bitmap_isset(
    IN const bitmap4 *mask,
    IN uint32_t word,
    IN uint32_t flag)
{
    return mask->count > word && mask->arr[word] & flag;
}
static __inline void bitmap_set(
    IN bitmap4 *mask,
    IN uint32_t word,
    IN uint32_t flag)
{
    if (mask->count > word)
        mask->arr[word] |= flag;
    else {
        mask->count = word + 1;
        mask->arr[word] = flag;
    }
}
static __inline void bitmap_unset(
    IN bitmap4 *mask,
    IN uint32_t word,
    IN uint32_t flag)
{
    if (mask->count > word) {
        mask->arr[word] &= ~flag;
        while (mask->count && mask->arr[mask->count-1] == 0)
            mask->count--;
    }
}
static __inline void bitmap_intersect(
    IN bitmap4 *dst,
    IN const bitmap4 *src)
{
    uint32_t i, count = 0;
    for (i = 0; i < 3; i++) {
        dst->arr[i] &= src->arr[i];
        if (dst->arr[i])
            count = i+1;
    }
    dst->count = min(dst->count, count);
}

ULONG nfs_file_info_to_attributes(
    IN const nfs41_file_info *info);
void nfs_to_basic_info(
    IN const nfs41_file_info *info,
    OUT PFILE_BASIC_INFO basic_out);
void nfs_to_standard_info(
    IN const nfs41_file_info *info,
    OUT PFILE_STANDARD_INFO std_out);
void nfs_to_network_openinfo(
    IN const nfs41_file_info *info,
    OUT PFILE_NETWORK_OPEN_INFORMATION std_out);

/* https://learn.microsoft.com/en-us/windows/win32/sysinfo/file-times
 * A file time is a 64-bit value that represents the number of
 * 100-nanosecond intervals that have elapsed since 12:00 A.M.
 * January 1, 1601 Coordinated Universal Time (UTC). */
#define FILETIME_EPOCH 116444736000000000LL

static __inline void file_time_to_nfs_time(
    IN const PLARGE_INTEGER file_time,
    OUT nfstime4 *nfs_time)
{
    LONGLONG diff = file_time->QuadPart - FILETIME_EPOCH;
    nfs_time->seconds = diff / 10000000;
    nfs_time->nseconds = (uint32_t)((diff % 10000000)*100);
}

static __inline void nfs_time_to_file_time(
    IN const nfstime4 *nfs_time,
    OUT PLARGE_INTEGER file_time)
{
    file_time->QuadPart = FILETIME_EPOCH +
        nfs_time->seconds * 10000000 +
        nfs_time->nseconds / 100;
}

void get_file_time(
    OUT PLARGE_INTEGER file_time);
void get_nfs_time(
    OUT nfstime4 *nfs_time);

static __inline void nfstime_normalize(
    IN OUT nfstime4 *nfstime)
{
    /* return time in normalized form (0 <= nsec < 1s) */
    while ((int32_t)nfstime->nseconds < 0) {
        nfstime->nseconds += 1000000000;
        nfstime->seconds--;
    }
}
static __inline void nfstime_diff(
    IN const nfstime4 *lhs,
    IN const nfstime4 *rhs,
    OUT nfstime4 *result)
{
    /* result = lhs - rhs */
    result->seconds = lhs->seconds - rhs->seconds;
    result->nseconds = lhs->nseconds - rhs->nseconds;
    nfstime_normalize(result);
}
static __inline void nfstime_abs(
    IN const nfstime4 *nt,
    OUT nfstime4 *result)
{
    if (nt->seconds < 0) {
        const nfstime4 zero = { 0, 0 };
        nfstime_diff(&zero, nt, result); /* result = 0 - nt */
    } else if (result != nt)
        memcpy(result, nt, sizeof(nfstime4));
}


int create_silly_rename(
    IN nfs41_abs_path *path,
    IN const nfs41_fh *fh,
    OUT nfs41_component *silly);

bool_t multi_addr_find(
    IN const multi_addr4 *addrs,
    IN const netaddr4 *addr,
    OUT OPTIONAL uint32_t *index_out);

/* nfs_to_windows_error
 *   Returns a windows ERROR_ code corresponding to the given NFS4ERR_ status.
 * If the status is outside the range of valid NFS4ERR_ values, it is returned
 * unchanged.  Otherwise, if the status does not match a value in the mapping,
 * a debug warning is generated and the default_error value is returned.
 */
int nfs_to_windows_error(int status, int default_error);

int map_symlink_errors(int status);

#ifndef __REACTOS__
__inline uint32_t align8(uint32_t offset) {
#else
FORCEINLINE uint32_t align8(uint32_t offset) {
#endif
    return 8 + ((offset - 1) & ~7);
}
#ifndef __REACTOS__
__inline uint32_t align4(uint32_t offset) {
#else
FORCEINLINE uint32_t align4(uint32_t offset) {
#endif
    return 4 + ((offset - 1) & ~3);
}

/* path parsing */
#ifndef __REACTOS__
__inline int is_delimiter(char c) {
#else
FORCEINLINE int is_delimiter(char c) {
#endif
    return c == '\\' || c == '/' || c == '\0';
}
#ifndef __REACTOS__
__inline const char* next_delimiter(const char *pos, const char *end) {
#else
FORCEINLINE const char* next_delimiter(const char *pos, const char *end) {
#endif
    while (pos < end && !is_delimiter(*pos))
        pos++;
    return pos;
}
#ifndef __REACTOS__
__inline const char* prev_delimiter(const char *pos, const char *start) {
#else
FORCEINLINE const char* prev_delimiter(const char *pos, const char *start) {
#endif
    while (pos > start && !is_delimiter(*pos))
        pos--;
    return pos;
}
#ifndef __REACTOS__
__inline const char* next_non_delimiter(const char *pos, const char *end) {
#else
FORCEINLINE const char* next_non_delimiter(const char *pos, const char *end) {
#endif
    while (pos < end && is_delimiter(*pos))
        pos++;
    return pos;
}
#ifndef __REACTOS__
__inline const char* prev_non_delimiter(const char *pos, const char *start) {
#else
FORCEINLINE const char* prev_non_delimiter(const char *pos, const char *start) {
#endif
    while (pos > start && is_delimiter(*pos))
        pos--;
    return pos;
}

bool_t next_component(
    IN const char *path,
    IN const char *path_end,
    OUT nfs41_component *component);

bool_t last_component(
    IN const char *path,
    IN const char *path_end,
    OUT nfs41_component *component);

bool_t is_last_component(
    IN const char *path,
    IN const char *path_end);

void abs_path_copy(
    OUT nfs41_abs_path *dst,
    IN const nfs41_abs_path *src);

void path_fh_init(
    OUT nfs41_path_fh *file,
    IN nfs41_abs_path *path);

void fh_copy(
    OUT nfs41_fh *dst,
    IN const nfs41_fh *src);

void path_fh_copy(
    OUT nfs41_path_fh *dst,
    IN const nfs41_path_fh *src);

#ifndef __REACTOS__
__inline int valid_handle(HANDLE handle) {
#else
FORCEINLINE int valid_handle(HANDLE handle) {
#endif
    return handle != INVALID_HANDLE_VALUE && handle != 0;
}

#endif /* !__NFS41_DAEMON_UTIL_H__ */
