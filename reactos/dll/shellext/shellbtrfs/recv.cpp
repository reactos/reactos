/* Copyright (c) Mark Harmstone 2017
 *
 * This file is part of WinBtrfs.
 *
 * WinBtrfs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public Licence as published by
 * the Free Software Foundation, either version 3 of the Licence, or
 * (at your option) any later version.
 *
 * WinBtrfs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public Licence for more details.
 *
 * You should have received a copy of the GNU Lesser General Public Licence
 * along with WinBtrfs.  If not, see <http://www.gnu.org/licenses/>. */

#include "shellext.h"
#include <windows.h>
#include <strsafe.h>
#include <stddef.h>
#include <sys/stat.h>
#include <string>
#include "recv.h"
#include "resource.h"

#ifndef __REACTOS__
#ifndef _MSC_VER
#include <cpuid.h>
#else
#include <intrin.h>
#endif

#include <smmintrin.h>
#endif

#define EA_NTACL "security.NTACL"
#define EA_DOSATTRIB "user.DOSATTRIB"
#define EA_REPARSE "user.reparse"
#define EA_EA "user.EA"
#define XATTR_USER "user."

#ifndef __REACTOS__
BOOL have_sse42 = FALSE;
#endif

static const UINT32 crctable[] = {
    0x00000000, 0xf26b8303, 0xe13b70f7, 0x1350f3f4, 0xc79a971f, 0x35f1141c, 0x26a1e7e8, 0xd4ca64eb,
    0x8ad958cf, 0x78b2dbcc, 0x6be22838, 0x9989ab3b, 0x4d43cfd0, 0xbf284cd3, 0xac78bf27, 0x5e133c24,
    0x105ec76f, 0xe235446c, 0xf165b798, 0x030e349b, 0xd7c45070, 0x25afd373, 0x36ff2087, 0xc494a384,
    0x9a879fa0, 0x68ec1ca3, 0x7bbcef57, 0x89d76c54, 0x5d1d08bf, 0xaf768bbc, 0xbc267848, 0x4e4dfb4b,
    0x20bd8ede, 0xd2d60ddd, 0xc186fe29, 0x33ed7d2a, 0xe72719c1, 0x154c9ac2, 0x061c6936, 0xf477ea35,
    0xaa64d611, 0x580f5512, 0x4b5fa6e6, 0xb93425e5, 0x6dfe410e, 0x9f95c20d, 0x8cc531f9, 0x7eaeb2fa,
    0x30e349b1, 0xc288cab2, 0xd1d83946, 0x23b3ba45, 0xf779deae, 0x05125dad, 0x1642ae59, 0xe4292d5a,
    0xba3a117e, 0x4851927d, 0x5b016189, 0xa96ae28a, 0x7da08661, 0x8fcb0562, 0x9c9bf696, 0x6ef07595,
    0x417b1dbc, 0xb3109ebf, 0xa0406d4b, 0x522bee48, 0x86e18aa3, 0x748a09a0, 0x67dafa54, 0x95b17957,
    0xcba24573, 0x39c9c670, 0x2a993584, 0xd8f2b687, 0x0c38d26c, 0xfe53516f, 0xed03a29b, 0x1f682198,
    0x5125dad3, 0xa34e59d0, 0xb01eaa24, 0x42752927, 0x96bf4dcc, 0x64d4cecf, 0x77843d3b, 0x85efbe38,
    0xdbfc821c, 0x2997011f, 0x3ac7f2eb, 0xc8ac71e8, 0x1c661503, 0xee0d9600, 0xfd5d65f4, 0x0f36e6f7,
    0x61c69362, 0x93ad1061, 0x80fde395, 0x72966096, 0xa65c047d, 0x5437877e, 0x4767748a, 0xb50cf789,
    0xeb1fcbad, 0x197448ae, 0x0a24bb5a, 0xf84f3859, 0x2c855cb2, 0xdeeedfb1, 0xcdbe2c45, 0x3fd5af46,
    0x7198540d, 0x83f3d70e, 0x90a324fa, 0x62c8a7f9, 0xb602c312, 0x44694011, 0x5739b3e5, 0xa55230e6,
    0xfb410cc2, 0x092a8fc1, 0x1a7a7c35, 0xe811ff36, 0x3cdb9bdd, 0xceb018de, 0xdde0eb2a, 0x2f8b6829,
    0x82f63b78, 0x709db87b, 0x63cd4b8f, 0x91a6c88c, 0x456cac67, 0xb7072f64, 0xa457dc90, 0x563c5f93,
    0x082f63b7, 0xfa44e0b4, 0xe9141340, 0x1b7f9043, 0xcfb5f4a8, 0x3dde77ab, 0x2e8e845f, 0xdce5075c,
    0x92a8fc17, 0x60c37f14, 0x73938ce0, 0x81f80fe3, 0x55326b08, 0xa759e80b, 0xb4091bff, 0x466298fc,
    0x1871a4d8, 0xea1a27db, 0xf94ad42f, 0x0b21572c, 0xdfeb33c7, 0x2d80b0c4, 0x3ed04330, 0xccbbc033,
    0xa24bb5a6, 0x502036a5, 0x4370c551, 0xb11b4652, 0x65d122b9, 0x97baa1ba, 0x84ea524e, 0x7681d14d,
    0x2892ed69, 0xdaf96e6a, 0xc9a99d9e, 0x3bc21e9d, 0xef087a76, 0x1d63f975, 0x0e330a81, 0xfc588982,
    0xb21572c9, 0x407ef1ca, 0x532e023e, 0xa145813d, 0x758fe5d6, 0x87e466d5, 0x94b49521, 0x66df1622,
    0x38cc2a06, 0xcaa7a905, 0xd9f75af1, 0x2b9cd9f2, 0xff56bd19, 0x0d3d3e1a, 0x1e6dcdee, 0xec064eed,
    0xc38d26c4, 0x31e6a5c7, 0x22b65633, 0xd0ddd530, 0x0417b1db, 0xf67c32d8, 0xe52cc12c, 0x1747422f,
    0x49547e0b, 0xbb3ffd08, 0xa86f0efc, 0x5a048dff, 0x8ecee914, 0x7ca56a17, 0x6ff599e3, 0x9d9e1ae0,
    0xd3d3e1ab, 0x21b862a8, 0x32e8915c, 0xc083125f, 0x144976b4, 0xe622f5b7, 0xf5720643, 0x07198540,
    0x590ab964, 0xab613a67, 0xb831c993, 0x4a5a4a90, 0x9e902e7b, 0x6cfbad78, 0x7fab5e8c, 0x8dc0dd8f,
    0xe330a81a, 0x115b2b19, 0x020bd8ed, 0xf0605bee, 0x24aa3f05, 0xd6c1bc06, 0xc5914ff2, 0x37faccf1,
    0x69e9f0d5, 0x9b8273d6, 0x88d28022, 0x7ab90321, 0xae7367ca, 0x5c18e4c9, 0x4f48173d, 0xbd23943e,
    0xf36e6f75, 0x0105ec76, 0x12551f82, 0xe03e9c81, 0x34f4f86a, 0xc69f7b69, 0xd5cf889d, 0x27a40b9e,
    0x79b737ba, 0x8bdcb4b9, 0x988c474d, 0x6ae7c44e, 0xbe2da0a5, 0x4c4623a6, 0x5f16d052, 0xad7d5351,
};

// HW code taken from https://github.com/rurban/smhasher/blob/master/crc32_hw.c
#define ALIGN_SIZE      0x08UL
#define ALIGN_MASK      (ALIGN_SIZE - 1)
#define CALC_CRC(op, crc, type, buf, len)                               \
do {                                                                  \
    for (; (len) >= sizeof (type); (len) -= sizeof(type), buf += sizeof (type)) { \
        (crc) = op((crc), *(type *) (buf));                               \
    }                                                                   \
} while(0)

#ifndef __REACTOS__
static UINT32 crc32c_hw(const void *input, ULONG len, UINT32 crc) {
    const char* buf = (const char*)input;

    // Annoyingly, the CRC32 intrinsics don't work properly in modern versions of MSVC -
    // it compiles _mm_crc32_u8 as if it was _mm_crc32_u32. And because we're apparently
    // not allowed to use inline asm on amd64, there's no easy way to fix this!

    for (; (len > 0) && ((size_t)buf & ALIGN_MASK); len--, buf++) {
#ifdef _MSC_VER
        crc = crctable[(crc ^ *buf) & 0xff] ^ (crc >> 8);
#else
        crc = _mm_crc32_u8(crc, *buf);
#endif
    }

#ifdef _AMD64_
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244) // _mm_crc32_u64 wants to return UINT64(!)
#pragma warning(disable:4242)
#endif
    CALC_CRC(_mm_crc32_u64, crc, UINT64, buf, len);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
    CALC_CRC(_mm_crc32_u32, crc, UINT32, buf, len);

#ifdef _MSC_VER
    for (; len > 0; len--, buf++) {
        crc = crctable[(crc ^ *buf) & 0xff] ^ (crc >> 8);
    }
#else
    CALC_CRC(_mm_crc32_u16, crc, UINT16, buf, len);
    CALC_CRC(_mm_crc32_u8, crc, UINT8, buf, len);
#endif

    return crc;
}
#endif

static UINT32 calc_crc32c(UINT32 seed, UINT8* msg, ULONG msglen) {
#ifndef __REACTOS__
    if (have_sse42)
        return crc32c_hw(msg, msglen, seed);
    else {
#endif
        UINT32 rem;
        ULONG i;

        rem = seed;

        for (i = 0; i < msglen; i++) {
            rem = crctable[(rem ^ msg[i]) & 0xff] ^ (rem >> 8);
        }

        return rem;
#ifndef __REACTOS__
    }
#endif
}

BOOL BtrfsRecv::find_tlv(UINT8* data, ULONG datalen, UINT16 type, void** value, ULONG* len) {
    ULONG off = 0;

    while (off < datalen) {
        btrfs_send_tlv* tlv = (btrfs_send_tlv*)(data + off);
        UINT8* payload = data + off + sizeof(btrfs_send_tlv);

        if (off + sizeof(btrfs_send_tlv) + tlv->length > datalen) // file is truncated
            return FALSE;

        if (tlv->type == type) {
            *value = payload;
            *len = tlv->length;
            return TRUE;
        }

        off += sizeof(btrfs_send_tlv) + tlv->length;
    }

    return FALSE;
}

BOOL BtrfsRecv::utf8_to_utf16(HWND hwnd, char* utf8, ULONG utf8len, std::wstring* utf16) {
    NTSTATUS Status;
    ULONG utf16len;
    WCHAR* buf;

    Status = RtlUTF8ToUnicodeN(NULL, 0, &utf16len, utf8, utf8len);
    if (!NT_SUCCESS(Status)) {
        ShowRecvError(IDS_RECV_RTLUTF8TOUNICODEN_FAILED, Status, format_ntstatus(Status).c_str());
        return FALSE;
    }

    buf = (WCHAR*)malloc(utf16len + sizeof(WCHAR));

    if (!buf) {
        ShowRecvError(IDS_OUT_OF_MEMORY);
        return FALSE;
    }

    Status = RtlUTF8ToUnicodeN(buf, utf16len, &utf16len, utf8, utf8len);
    if (!NT_SUCCESS(Status)) {
        ShowRecvError(IDS_RECV_RTLUTF8TOUNICODEN_FAILED, Status, format_ntstatus(Status).c_str());
        free(buf);
        return FALSE;
    }

    buf[utf16len / sizeof(WCHAR)] = 0;

    *utf16 = buf;

    free(buf);

    return TRUE;
}

BOOL BtrfsRecv::cmd_subvol(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    char* name;
    BTRFS_UUID* uuid;
    UINT64* gen;
    ULONG namelen, uuidlen, genlen, bcslen;
    btrfs_create_subvol* bcs;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    std::wstring nameu;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&name, &namelen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_UUID, (void**)&uuid, &uuidlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"uuid");
        return FALSE;
    }

    if (uuidlen < sizeof(BTRFS_UUID)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"uuid", uuidlen, sizeof(BTRFS_UUID));
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_TRANSID, (void**)&gen, &genlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"transid");
        return FALSE;
    }

    if (genlen < sizeof(UINT64)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"transid", genlen, sizeof(UINT64));
        return FALSE;
    }

    this->subvol_uuid = *uuid;
    this->stransid = *gen;

    if (!utf8_to_utf16(hwnd, name, namelen, &nameu))
        return FALSE;

    bcslen = offsetof(btrfs_create_subvol, name[0]) + (nameu.length() * sizeof(WCHAR));
    bcs = (btrfs_create_subvol*)malloc(bcslen);

    bcs->readonly = TRUE;
    bcs->posix = TRUE;
    bcs->namelen = nameu.length() * sizeof(WCHAR);
    memcpy(bcs->name, nameu.c_str(), bcs->namelen);

    Status = NtFsControlFile(parent, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_CREATE_SUBVOL, bcs, bcslen, NULL, 0);
    if (!NT_SUCCESS(Status)) {
        ShowRecvError(IDS_RECV_CREATE_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());
        return FALSE;
    }

    subvolpath = dirpath;
    subvolpath += L"\\";
    subvolpath += nameu;

    if (dir != INVALID_HANDLE_VALUE)
        CloseHandle(dir);

    if (master != INVALID_HANDLE_VALUE)
        CloseHandle(master);

    master = CreateFileW(subvolpath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, NULL);
    if (master == INVALID_HANDLE_VALUE) {
        ShowRecvError(IDS_RECV_CANT_OPEN_PATH, subvolpath.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    Status = NtFsControlFile(master, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_RESERVE_SUBVOL, bcs, bcslen, NULL, 0);
    if (!NT_SUCCESS(Status)) {
        ShowRecvError(IDS_RECV_RESERVE_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());
        return FALSE;
    }

    dir = CreateFileW(subvolpath.c_str(), FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, NULL);
    if (dir == INVALID_HANDLE_VALUE) {
        ShowRecvError(IDS_RECV_CANT_OPEN_PATH, subvolpath.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    subvolpath += L"\\";

    add_cache_entry(&this->subvol_uuid, this->stransid, subvolpath);

    num_received++;

    return TRUE;
}

void BtrfsRecv::add_cache_entry(BTRFS_UUID* uuid, UINT64 transid, std::wstring path) {
    subvol_cache sc;

    sc.uuid = *uuid;
    sc.transid = transid;
    sc.path = path;

    cache.push_back(sc);
}

BOOL BtrfsRecv::cmd_snapshot(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    char* name;
    BTRFS_UUID *uuid, *parent_uuid;
    UINT64 *gen, *parent_transid;
    ULONG namelen, uuidlen, genlen, paruuidlen, partransidlen, bcslen;
    btrfs_create_snapshot* bcs;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    std::wstring nameu, parpath;
    btrfs_find_subvol bfs;
    WCHAR parpathw[MAX_PATH], volpathw[MAX_PATH];
    HANDLE subvol;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&name, &namelen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_UUID, (void**)&uuid, &uuidlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"uuid");
        return FALSE;
    }

    if (uuidlen < sizeof(BTRFS_UUID)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"uuid", uuidlen, sizeof(BTRFS_UUID));
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_TRANSID, (void**)&gen, &genlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"transid");
        return FALSE;
    }

    if (genlen < sizeof(UINT64)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"transid", genlen, sizeof(UINT64));
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_UUID, (void**)&parent_uuid, &paruuidlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"clone_uuid");
        return FALSE;
    }

    if (paruuidlen < sizeof(BTRFS_UUID)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"clone_uuid", paruuidlen, sizeof(BTRFS_UUID));
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_CTRANSID, (void**)&parent_transid, &partransidlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"clone_ctransid");
        return FALSE;
    }

    if (partransidlen < sizeof(UINT64)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"clone_ctransid", partransidlen, sizeof(UINT64));
        return FALSE;
    }

    this->subvol_uuid = *uuid;
    this->stransid = *gen;

    if (!utf8_to_utf16(hwnd, name, namelen, &nameu))
        return FALSE;

    bfs.uuid = *parent_uuid;
    bfs.ctransid = *parent_transid;

    Status = NtFsControlFile(parent, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_FIND_SUBVOL, &bfs, sizeof(btrfs_find_subvol),
                             parpathw, sizeof(parpathw));
    if (Status == STATUS_NOT_FOUND) {
        ShowRecvError(IDS_RECV_CANT_FIND_PARENT_SUBVOL);
        return FALSE;
    } else if (!NT_SUCCESS(Status)) {
        ShowRecvError(IDS_RECV_FIND_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());
        return FALSE;
    }

    if (!GetVolumePathNameW(dirpath.c_str(), volpathw, (sizeof(volpathw) / sizeof(WCHAR)) - 1)) {
        ShowRecvError(IDS_RECV_GETVOLUMEPATHNAME_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    parpath = volpathw;
    if (parpath.substr(parpath.length() - 1) == L"\\")
        parpath = parpath.substr(0, parpath.length() - 1);

    parpath += parpathw;

    subvol = CreateFileW(parpath.c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (subvol == INVALID_HANDLE_VALUE) {
        ShowRecvError(IDS_RECV_CANT_OPEN_PATH, parpath.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    bcslen = offsetof(btrfs_create_snapshot, name[0]) + (nameu.length() * sizeof(WCHAR));
    bcs = (btrfs_create_snapshot*)malloc(bcslen);

    bcs->readonly = TRUE;
    bcs->posix = TRUE;
    bcs->subvol = subvol;
    bcs->namelen = nameu.length() * sizeof(WCHAR);
    memcpy(bcs->name, nameu.c_str(), bcs->namelen);

    Status = NtFsControlFile(parent, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_CREATE_SNAPSHOT, bcs, bcslen, NULL, 0);
    if (!NT_SUCCESS(Status)) {
        ShowRecvError(IDS_RECV_CREATE_SNAPSHOT_FAILED, Status, format_ntstatus(Status).c_str());
        return FALSE;
    }

    subvolpath = dirpath;
    subvolpath += L"\\";
    subvolpath += nameu;

    if (dir != INVALID_HANDLE_VALUE)
        CloseHandle(dir);

    if (master != INVALID_HANDLE_VALUE)
        CloseHandle(master);

    master = CreateFileW(subvolpath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, NULL);
    if (master == INVALID_HANDLE_VALUE) {
        ShowRecvError(IDS_RECV_CANT_OPEN_PATH, subvolpath.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    Status = NtFsControlFile(master, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_RESERVE_SUBVOL, bcs, bcslen, NULL, 0);
    if (!NT_SUCCESS(Status)) {
        ShowRecvError(IDS_RECV_RESERVE_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());
        return FALSE;
    }

    dir = CreateFileW(subvolpath.c_str(), FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, NULL);
    if (dir == INVALID_HANDLE_VALUE) {
        ShowRecvError(IDS_RECV_CANT_OPEN_PATH, subvolpath.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    subvolpath += L"\\";

    add_cache_entry(&this->subvol_uuid, this->stransid, subvolpath);

    num_received++;

    return TRUE;
}

BOOL BtrfsRecv::cmd_mkfile(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    char *name, *pathlink;
    UINT64 *inode, *rdev = NULL, *mode = NULL;
    ULONG namelen, inodelen, bmnsize;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_mknod* bmn;
    std::wstring nameu, pathlinku;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&name, &namelen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_INODE, (void**)&inode, &inodelen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"inode");
        return FALSE;
    }

    if (inodelen < sizeof(UINT64)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"inode", inodelen, sizeof(UINT64));
        return FALSE;
    }

    if (cmd->cmd == BTRFS_SEND_CMD_MKNOD || cmd->cmd == BTRFS_SEND_CMD_MKFIFO || cmd->cmd == BTRFS_SEND_CMD_MKSOCK) {
        ULONG rdevlen, modelen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_RDEV, (void**)&rdev, &rdevlen)) {
            ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"rdev");
            return FALSE;
        }

        if (rdevlen < sizeof(UINT64)) {
            ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"rdev", rdev, sizeof(UINT64));
            return FALSE;
        }

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_MODE, (void**)&mode, &modelen)) {
            ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"mode");
            return FALSE;
        }

        if (modelen < sizeof(UINT64)) {
            ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"mode", modelen, sizeof(UINT64));
            return FALSE;
        }
    } else if (cmd->cmd == BTRFS_SEND_CMD_SYMLINK) {
        ULONG pathlinklen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH_LINK, (void**)&pathlink, &pathlinklen)) {
            ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path_link");
            return FALSE;
        }

        if (!utf8_to_utf16(hwnd, pathlink, pathlinklen, &pathlinku))
            return FALSE;
    }

    if (!utf8_to_utf16(hwnd, name, namelen, &nameu))
        return FALSE;

    bmnsize = sizeof(btrfs_mknod) - sizeof(WCHAR) + (nameu.length() * sizeof(WCHAR));
    bmn = (btrfs_mknod*)malloc(bmnsize);

    bmn->inode = *inode;

    if (cmd->cmd == BTRFS_SEND_CMD_MKDIR)
        bmn->type = BTRFS_TYPE_DIRECTORY;
    else if (cmd->cmd == BTRFS_SEND_CMD_MKNOD)
        bmn->type = *mode & S_IFCHR ? BTRFS_TYPE_CHARDEV : BTRFS_TYPE_BLOCKDEV;
    else if (cmd->cmd == BTRFS_SEND_CMD_MKFIFO)
        bmn->type = BTRFS_TYPE_FIFO;
    else if (cmd->cmd == BTRFS_SEND_CMD_MKSOCK)
        bmn->type = BTRFS_TYPE_SOCKET;
    else
        bmn->type = BTRFS_TYPE_FILE;

    bmn->st_rdev = rdev ? *rdev : 0;
    bmn->namelen = nameu.length() * sizeof(WCHAR);
    memcpy(bmn->name, nameu.c_str(), bmn->namelen);

    Status = NtFsControlFile(dir, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_MKNOD, bmn, bmnsize, NULL, 0);
    if (!NT_SUCCESS(Status)) {
        ShowRecvError(IDS_RECV_MKNOD_FAILED, Status, format_ntstatus(Status).c_str());
        free(bmn);
        return FALSE;
    }

    free(bmn);

    if (cmd->cmd == BTRFS_SEND_CMD_SYMLINK) {
        HANDLE h;
        REPARSE_DATA_BUFFER* rdb;
        ULONG rdblen;
        btrfs_set_inode_info bsii;

        rdblen = offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer[0]) + (2 * pathlinku.length() * sizeof(WCHAR));

        rdb = (REPARSE_DATA_BUFFER*)malloc(rdblen);

        rdb->ReparseTag = IO_REPARSE_TAG_SYMLINK;
        rdb->ReparseDataLength = rdblen - offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer);
        rdb->Reserved = 0;
        rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
        rdb->SymbolicLinkReparseBuffer.SubstituteNameLength = pathlinku.length() * sizeof(WCHAR);
        rdb->SymbolicLinkReparseBuffer.PrintNameOffset = pathlinku.length() * sizeof(WCHAR);
        rdb->SymbolicLinkReparseBuffer.PrintNameLength = pathlinku.length() * sizeof(WCHAR);
        rdb->SymbolicLinkReparseBuffer.Flags = SYMLINK_FLAG_RELATIVE;

        memcpy(rdb->SymbolicLinkReparseBuffer.PathBuffer, pathlinku.c_str(), rdb->SymbolicLinkReparseBuffer.SubstituteNameLength);
        memcpy(rdb->SymbolicLinkReparseBuffer.PathBuffer + (rdb->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(WCHAR)),
                pathlinku.c_str(), rdb->SymbolicLinkReparseBuffer.PrintNameLength);

        h = CreateFileW((subvolpath + nameu).c_str(), GENERIC_WRITE | WRITE_DAC, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            ShowRecvError(IDS_RECV_CANT_OPEN_FILE, funcname, nameu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
            free(rdb);
            return FALSE;
        }

        Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_SET_REPARSE_POINT, rdb, rdblen, NULL, 0);
        if (!NT_SUCCESS(Status)) {
            ShowRecvError(IDS_RECV_SET_REPARSE_POINT_FAILED, Status, format_ntstatus(Status).c_str());
            free(rdb);
            CloseHandle(h);
            return FALSE;
        }

        free(rdb);

        memset(&bsii, 0, sizeof(btrfs_set_inode_info));

        bsii.mode_changed = TRUE;
        bsii.st_mode = 0777;

        Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), NULL, 0);
        if (!NT_SUCCESS(Status)) {
            ShowRecvError(IDS_RECV_SETINODEINFO_FAILED, Status, format_ntstatus(Status).c_str());
            CloseHandle(h);
            return FALSE;
        }

        CloseHandle(h);
    } else if (cmd->cmd == BTRFS_SEND_CMD_MKNOD || cmd->cmd == BTRFS_SEND_CMD_MKFIFO || cmd->cmd == BTRFS_SEND_CMD_MKSOCK) {
        UINT64* mode;
        ULONG modelen;

        if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_MODE, (void**)&mode, &modelen)) {
            HANDLE h;
            btrfs_set_inode_info bsii;

            if (modelen < sizeof(UINT64)) {
                ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"mode", modelen, sizeof(UINT64));
                return FALSE;
            }

            h = CreateFileW((subvolpath + nameu).c_str(), WRITE_DAC, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, NULL);
            if (h == INVALID_HANDLE_VALUE) {
                ShowRecvError(IDS_RECV_CANT_OPEN_FILE, funcname, nameu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
                return FALSE;
            }

            memset(&bsii, 0, sizeof(btrfs_set_inode_info));

            bsii.mode_changed = TRUE;
            bsii.st_mode = *mode;

            Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), NULL, 0);
            if (!NT_SUCCESS(Status)) {
                ShowRecvError(IDS_RECV_SETINODEINFO_FAILED, Status, format_ntstatus(Status).c_str());
                CloseHandle(h);
                return FALSE;
            }

            CloseHandle(h);
        }
    }

    return TRUE;
}

BOOL BtrfsRecv::cmd_rename(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    char *path, *path_to;
    ULONG path_len, path_to_len;
    std::wstring pathu, path_tou;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &path_len)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH_TO, (void**)&path_to, &path_to_len)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path_to");
        return FALSE;
    }

    if (!utf8_to_utf16(hwnd, path, path_len, &pathu))
        return FALSE;

    if (!utf8_to_utf16(hwnd, path_to, path_to_len, &path_tou))
        return FALSE;

    if (!MoveFileW((subvolpath + pathu).c_str(), (subvolpath + path_tou).c_str())) {
        ShowRecvError(IDS_RECV_MOVEFILE_FAILED, pathu.c_str(), path_tou.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    return TRUE;
}

BOOL BtrfsRecv::cmd_link(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    char *path, *path_link;
    ULONG path_len, path_link_len;
    std::wstring pathu, path_linku;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &path_len)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH_LINK, (void**)&path_link, &path_link_len)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path_link");
        return FALSE;
    }

    if (!utf8_to_utf16(hwnd, path, path_len, &pathu))
        return FALSE;

    if (!utf8_to_utf16(hwnd, path_link, path_link_len, &path_linku))
        return FALSE;

    if (!CreateHardLinkW((subvolpath + pathu).c_str(), (subvolpath + path_linku).c_str(), NULL)) {
        ShowRecvError(IDS_RECV_CREATEHARDLINK_FAILED, pathu.c_str(), path_linku.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    return TRUE;
}

BOOL BtrfsRecv::cmd_unlink(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    char* path;
    ULONG pathlen;
    std::wstring pathu;
    ULONG att;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!utf8_to_utf16(hwnd, path, pathlen, &pathu))
        return FALSE;

    att = GetFileAttributesW((subvolpath + pathu).c_str());
    if (att == INVALID_FILE_ATTRIBUTES) {
        ShowRecvError(IDS_RECV_GETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    if (att & FILE_ATTRIBUTE_READONLY) {
        if (!SetFileAttributesW((subvolpath + pathu).c_str(), att & ~FILE_ATTRIBUTE_READONLY)) {
            ShowRecvError(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }
    }

    if (!DeleteFileW((subvolpath + pathu).c_str())) {
        ShowRecvError(IDS_RECV_DELETEFILE_FAILED, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    return TRUE;
}

BOOL BtrfsRecv::cmd_rmdir(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    char* path;
    ULONG pathlen;
    std::wstring pathu;
    ULONG att;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!utf8_to_utf16(hwnd, path, pathlen, &pathu))
        return FALSE;

    att = GetFileAttributesW((subvolpath + pathu).c_str());
    if (att == INVALID_FILE_ATTRIBUTES) {
        ShowRecvError(IDS_RECV_GETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    if (att & FILE_ATTRIBUTE_READONLY) {
        if (!SetFileAttributesW((subvolpath + pathu).c_str(), att & ~FILE_ATTRIBUTE_READONLY)) {
            ShowRecvError(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }
    }

    if (!RemoveDirectoryW((subvolpath + pathu).c_str())) {
        ShowRecvError(IDS_RECV_REMOVEDIRECTORY_FAILED, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    return TRUE;
}

BOOL BtrfsRecv::cmd_setxattr(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    char *path, *xattrname;
    UINT8* xattrdata;
    ULONG pathlen, xattrnamelen, xattrdatalen;
    std::wstring pathu;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_XATTR_NAME, (void**)&xattrname, &xattrnamelen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"xattr_name");
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_XATTR_DATA, (void**)&xattrdata, &xattrdatalen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"xattr_data");
        return FALSE;
    }

    if (!utf8_to_utf16(hwnd, path, pathlen, &pathu))
        return FALSE;

    if (xattrnamelen > strlen(XATTR_USER) && !memcmp(xattrname, XATTR_USER, strlen(XATTR_USER)) &&
        (xattrnamelen != strlen(EA_DOSATTRIB) || memcmp(xattrname, EA_DOSATTRIB, xattrnamelen)) &&
        (xattrnamelen != strlen(EA_EA) || memcmp(xattrname, EA_EA, xattrnamelen)) &&
        (xattrnamelen != strlen(EA_REPARSE) || memcmp(xattrname, EA_REPARSE, xattrnamelen))) {
        HANDLE h;
        std::wstring streamname;
        ULONG att;

        if (!utf8_to_utf16(hwnd, xattrname, xattrnamelen, &streamname))
            return FALSE;

        att = GetFileAttributesW((subvolpath + pathu).c_str());
        if (att == INVALID_FILE_ATTRIBUTES) {
            ShowRecvError(IDS_RECV_GETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }

        if (att & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributesW((subvolpath + pathu).c_str(), att & ~FILE_ATTRIBUTE_READONLY)) {
                ShowRecvError(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                return FALSE;
            }
        }

        streamname = streamname.substr(strlen(XATTR_USER));

        h = CreateFileW((subvolpath + pathu + L":" + streamname).c_str(), GENERIC_WRITE, 0,
                        NULL, CREATE_ALWAYS, FILE_FLAG_POSIX_SEMANTICS, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            ShowRecvError(IDS_RECV_CANT_CREATE_FILE, (pathu + L":" + streamname).c_str(), GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }

        if (xattrdatalen > 0) {
            if (!WriteFile(h, xattrdata, xattrdatalen, NULL, NULL)) {
                ShowRecvError(IDS_RECV_WRITEFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                CloseHandle(h);
                return FALSE;
            }
        }

        CloseHandle(h);

        if (att & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributesW((subvolpath + pathu).c_str(), att)) {
                ShowRecvError(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                return FALSE;
            }
        }
    } else {
        HANDLE h;
        IO_STATUS_BLOCK iosb;
        NTSTATUS Status;
        ULONG bsxalen, perms = FILE_WRITE_ATTRIBUTES;
        btrfs_set_xattr* bsxa;

        if (xattrnamelen == strlen(EA_NTACL) && !memcmp(xattrname, EA_NTACL, xattrnamelen))
            perms |= WRITE_DAC | WRITE_OWNER;
        else if (xattrnamelen == strlen(EA_EA) && !memcmp(xattrname, EA_EA, xattrnamelen))
            perms |= FILE_WRITE_EA;

        h = CreateFileW((subvolpath + pathu).c_str(), perms, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS | FILE_OPEN_REPARSE_POINT, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            ShowRecvError(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }

        bsxalen = offsetof(btrfs_set_xattr, data[0]) + xattrnamelen + xattrdatalen;
        bsxa = (btrfs_set_xattr*)malloc(bsxalen);
        if (!bsxa) {
            ShowRecvError(IDS_OUT_OF_MEMORY);
            CloseHandle(h);
            return FALSE;
        }

        bsxa->namelen = xattrnamelen;
        bsxa->valuelen = xattrdatalen;
        memcpy(bsxa->data, xattrname, xattrnamelen);
        memcpy(&bsxa->data[xattrnamelen], xattrdata, xattrdatalen);

        Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SET_XATTR, bsxa, bsxalen, NULL, 0);
        if (!NT_SUCCESS(Status)) {
            ShowRecvError(IDS_RECV_SETXATTR_FAILED, Status, format_ntstatus(Status).c_str());
            free(bsxa);
            CloseHandle(h);
            return FALSE;
        }

        free(bsxa);
        CloseHandle(h);
    }

    return TRUE;
}

BOOL BtrfsRecv::cmd_removexattr(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    char *path, *xattrname;
    ULONG pathlen, xattrnamelen;
    std::wstring pathu;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_XATTR_NAME, (void**)&xattrname, &xattrnamelen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"xattr_name");
        return FALSE;
    }

    if (!utf8_to_utf16(hwnd, path, pathlen, &pathu))
        return FALSE;

    if (xattrnamelen > strlen(XATTR_USER) && !memcmp(xattrname, XATTR_USER, strlen(XATTR_USER)) &&
        (xattrnamelen != strlen(EA_DOSATTRIB) || memcmp(xattrname, EA_DOSATTRIB, xattrnamelen)) &&
        (xattrnamelen != strlen(EA_EA) || memcmp(xattrname, EA_EA, xattrnamelen))) { // deleting stream
        ULONG att;
        std::wstring streamname;

        if (!utf8_to_utf16(hwnd, xattrname, xattrnamelen, &streamname))
            return FALSE;

        streamname = streamname.substr(strlen(XATTR_USER));

        att = GetFileAttributesW((subvolpath + pathu).c_str());
        if (att == INVALID_FILE_ATTRIBUTES) {
            ShowRecvError(IDS_RECV_GETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }

        if (att & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributesW((subvolpath + pathu).c_str(), att & ~FILE_ATTRIBUTE_READONLY)) {
                ShowRecvError(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                return FALSE;
            }
        }

        if (!DeleteFileW((subvolpath + pathu + L":" + streamname).c_str())) {
            ShowRecvError(IDS_RECV_DELETEFILE_FAILED, (pathu + L":" + streamname).c_str(), GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }

        if (att & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributesW((subvolpath + pathu).c_str(), att)) {
                ShowRecvError(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                return FALSE;
            }
        }
    } else {
        HANDLE h;
        IO_STATUS_BLOCK iosb;
        NTSTATUS Status;
        ULONG bsxalen, perms = FILE_WRITE_ATTRIBUTES;
        btrfs_set_xattr* bsxa;

        if (xattrnamelen == strlen(EA_NTACL) && !memcmp(xattrname, EA_NTACL, xattrnamelen))
            perms |= WRITE_DAC | WRITE_OWNER;
        else if (xattrnamelen == strlen(EA_EA) && !memcmp(xattrname, EA_EA, xattrnamelen))
            perms |= FILE_WRITE_EA;

        h = CreateFileW((subvolpath + pathu).c_str(), perms, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS | FILE_OPEN_REPARSE_POINT, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            ShowRecvError(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }

        bsxalen = offsetof(btrfs_set_xattr, data[0]) + xattrnamelen;
        bsxa = (btrfs_set_xattr*)malloc(bsxalen);
        if (!bsxa) {
            ShowRecvError(IDS_OUT_OF_MEMORY);
            CloseHandle(h);
            return FALSE;
        }

        bsxa->namelen = xattrnamelen;
        bsxa->valuelen = 0;
        memcpy(bsxa->data, xattrname, xattrnamelen);

        Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SET_XATTR, bsxa, bsxalen, NULL, 0);
        if (!NT_SUCCESS(Status)) {
            ShowRecvError(IDS_RECV_SETXATTR_FAILED, Status, format_ntstatus(Status).c_str());
            free(bsxa);
            CloseHandle(h);
            return FALSE;
        }

        free(bsxa);
        CloseHandle(h);
    }

    return TRUE;
}

BOOL BtrfsRecv::cmd_write(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    char* path;
    UINT64* offset;
    UINT8* writedata;
    ULONG pathlen, offsetlen, datalen;
    std::wstring pathu;
    HANDLE h;
    LARGE_INTEGER offli;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_OFFSET, (void**)&offset, &offsetlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"offset");
        return FALSE;
    }

    if (offsetlen < sizeof(UINT64)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"offset", offsetlen, sizeof(UINT64));
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_DATA, (void**)&writedata, &datalen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"data");
        return FALSE;
    }

    if (!utf8_to_utf16(hwnd, path, pathlen, &pathu))
        return FALSE;

    if (lastwritepath != pathu) {
        FILE_BASIC_INFO fbi;

        if (lastwriteatt & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributesW((subvolpath + lastwritepath).c_str(), lastwriteatt)) {
                ShowRecvError(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                return FALSE;
            }
        }

        CloseHandle(lastwritefile);

        lastwriteatt = GetFileAttributesW((subvolpath + pathu).c_str());
        if (lastwriteatt == INVALID_FILE_ATTRIBUTES) {
            ShowRecvError(IDS_RECV_GETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }

        if (lastwriteatt & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributesW((subvolpath + pathu).c_str(), lastwriteatt & ~FILE_ATTRIBUTE_READONLY)) {
                ShowRecvError(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                return FALSE;
            }
        }

        h = CreateFileW((subvolpath + pathu).c_str(), FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            ShowRecvError(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }

        lastwritepath = pathu;
        lastwritefile = h;

        memset(&fbi, 0, sizeof(FILE_BASIC_INFO));

        fbi.LastWriteTime.QuadPart = -1;

        if (!SetFileInformationByHandle(h, FileBasicInfo, &fbi, sizeof(FILE_BASIC_INFO))) {
            ShowRecvError(IDS_RECV_SETFILEINFO_FAILED, GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }
    } else
        h = lastwritefile;

    offli.QuadPart = *offset;

    if (SetFilePointer(h, offli.LowPart, &offli.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        ShowRecvError(IDS_RECV_SETFILEPOINTER_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    if (!WriteFile(h, writedata, datalen, NULL, NULL)) {
        ShowRecvError(IDS_RECV_WRITEFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    return TRUE;
}

BOOL BtrfsRecv::cmd_clone(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    char *path, *clonepath;
    UINT64 *offset, *cloneoffset, *clonetransid, *clonelen;
    BTRFS_UUID* cloneuuid;
    ULONG i, offsetlen, pathlen, clonepathlen, cloneoffsetlen, cloneuuidlen, clonetransidlen, clonelenlen;
    std::wstring pathu, clonepathu, clonepar;
    btrfs_find_subvol bfs;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    WCHAR cloneparw[MAX_PATH];
    HANDLE src, dest;
    DUPLICATE_EXTENTS_DATA ded;
    LARGE_INTEGER filesize;
    BOOL found = FALSE;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_OFFSET, (void**)&offset, &offsetlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"offset");
        return FALSE;
    }

    if (offsetlen < sizeof(UINT64)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"offset", offsetlen, sizeof(UINT64));
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_LENGTH, (void**)&clonelen, &clonelenlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"clone_len");
        return FALSE;
    }

    if (clonelenlen < sizeof(UINT64)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"clone_len", clonelenlen, sizeof(UINT64));
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!utf8_to_utf16(hwnd, path, pathlen, &pathu))
        return FALSE;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_UUID, (void**)&cloneuuid, &cloneuuidlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"clone_uuid");
        return FALSE;
    }

    if (cloneuuidlen < sizeof(BTRFS_UUID)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"clone_uuid", cloneuuidlen, sizeof(BTRFS_UUID));
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_CTRANSID, (void**)&clonetransid, &clonetransidlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"clone_ctransid");
        return FALSE;
    }

    if (clonetransidlen < sizeof(UINT64)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"clone_ctransid", clonetransidlen, sizeof(UINT64));
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_PATH, (void**)&clonepath, &clonepathlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"clone_path");
        return FALSE;
    }

    if (!utf8_to_utf16(hwnd, clonepath, clonepathlen, &clonepathu))
        return FALSE;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_OFFSET, (void**)&cloneoffset, &cloneoffsetlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"clone_offset");
        return FALSE;
    }

    if (cloneoffsetlen < sizeof(UINT64)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"clone_offset", cloneoffsetlen, sizeof(UINT64));
        return FALSE;
    }

    for (i = 0; i < cache.size(); i++) {
        if (!memcmp(cloneuuid, &cache[i].uuid, sizeof(BTRFS_UUID)) && *clonetransid == cache[i].transid) {
            clonepar = cache[i].path;
            found = TRUE;
            break;
        }
    }

    if (!found) {
        WCHAR volpathw[MAX_PATH];

        bfs.uuid = *cloneuuid;
        bfs.ctransid = *clonetransid;

        Status = NtFsControlFile(dir, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_FIND_SUBVOL, &bfs, sizeof(btrfs_find_subvol),
                                 cloneparw, sizeof(cloneparw));
        if (Status == STATUS_NOT_FOUND) {
            ShowRecvError(IDS_RECV_CANT_FIND_CLONE_SUBVOL);
            return FALSE;
        } else if (!NT_SUCCESS(Status)) {
            ShowRecvError(IDS_RECV_FIND_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());
            return FALSE;
        }

        if (!GetVolumePathNameW(dirpath.c_str(), volpathw, (sizeof(volpathw) / sizeof(WCHAR)) - 1)) {
            ShowRecvError(IDS_RECV_GETVOLUMEPATHNAME_FAILED, GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }

        clonepar = volpathw;
        if (clonepar.substr(clonepar.length() - 1) == L"\\")
            clonepar = clonepar.substr(0, clonepar.length() - 1);

        clonepar += cloneparw;
        clonepar += L"\\";

        add_cache_entry(cloneuuid, *clonetransid, clonepar);
    }

    src = CreateFileW((clonepar + clonepathu).c_str(), FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS, NULL);
    if (src == INVALID_HANDLE_VALUE) {
        ShowRecvError(IDS_RECV_CANT_OPEN_FILE, funcname, (clonepar + clonepathu).c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    dest = CreateFileW((subvolpath + pathu).c_str(), FILE_WRITE_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS, NULL);
    if (dest == INVALID_HANDLE_VALUE) {
        ShowRecvError(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        CloseHandle(src);
        return FALSE;
    }

    if (!GetFileSizeEx(dest, &filesize)) {
        ShowRecvError(IDS_RECV_GETFILESIZEEX_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        CloseHandle(dest);
        CloseHandle(src);
        return FALSE;
    }

    if ((UINT64)filesize.QuadPart < *offset + *clonelen) {
        LARGE_INTEGER sizeli;

        sizeli.QuadPart = *offset + *clonelen;

        if (SetFilePointer(dest, sizeli.LowPart, &sizeli.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
            ShowRecvError(IDS_RECV_SETFILEPOINTER_FAILED, GetLastError(), format_message(GetLastError()).c_str());
            CloseHandle(dest);
            CloseHandle(src);
            return FALSE;
        }

        if (!SetEndOfFile(dest)) {
            ShowRecvError(IDS_RECV_SETENDOFFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
            CloseHandle(dest);
            CloseHandle(src);
            return FALSE;
        }
    }

    ded.FileHandle = src;
    ded.SourceFileOffset.QuadPart = *cloneoffset;
    ded.TargetFileOffset.QuadPart = *offset;
    ded.ByteCount.QuadPart = *clonelen;

    Status = NtFsControlFile(dest, NULL, NULL, NULL, &iosb, FSCTL_DUPLICATE_EXTENTS_TO_FILE, &ded, sizeof(DUPLICATE_EXTENTS_DATA),
                             NULL, 0);
    if (!NT_SUCCESS(Status)) {
        ShowRecvError(IDS_RECV_DUPLICATE_EXTENTS_FAILED, Status, format_ntstatus(Status).c_str());
        CloseHandle(dest);
        CloseHandle(src);
        return FALSE;
    }

    CloseHandle(dest);
    CloseHandle(src);

    return TRUE;
}

BOOL BtrfsRecv::cmd_truncate(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    char* path;
    UINT64* size;
    ULONG pathlen, sizelen;
    std::wstring pathu;
    HANDLE h;
    LARGE_INTEGER sizeli;
    DWORD att;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_SIZE, (void**)&size, &sizelen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"size");
        return FALSE;
    }

    if (sizelen < sizeof(UINT64)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"size", sizelen, sizeof(UINT64));
        return FALSE;
    }

    if (!utf8_to_utf16(hwnd, path, pathlen, &pathu))
        return FALSE;

    att = GetFileAttributesW((subvolpath + pathu).c_str());
    if (att == INVALID_FILE_ATTRIBUTES) {
        ShowRecvError(IDS_RECV_GETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    if (att & FILE_ATTRIBUTE_READONLY) {
        if (!SetFileAttributesW((subvolpath + pathu).c_str(), att & ~FILE_ATTRIBUTE_READONLY)) {
            ShowRecvError(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }
    }

    h = CreateFileW((subvolpath + pathu).c_str(), FILE_WRITE_DATA, 0, NULL, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        ShowRecvError(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    sizeli.QuadPart = *size;

    if (SetFilePointer(h, sizeli.LowPart, &sizeli.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        ShowRecvError(IDS_RECV_SETFILEPOINTER_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        CloseHandle(h);
        return FALSE;
    }

    if (!SetEndOfFile(h)) {
        ShowRecvError(IDS_RECV_SETENDOFFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        CloseHandle(h);
        return FALSE;
    }

    CloseHandle(h);

    if (att & FILE_ATTRIBUTE_READONLY) {
        if (!SetFileAttributesW((subvolpath + pathu).c_str(), att)) {
            ShowRecvError(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
            return FALSE;
        }
    }

    return TRUE;
}

BOOL BtrfsRecv::cmd_chmod(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    HANDLE h;
    char* path;
    UINT32* mode;
    ULONG pathlen, modelen;
    std::wstring pathu;
    btrfs_set_inode_info bsii;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_MODE, (void**)&mode, &modelen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"mode");
        return FALSE;
    }

    if (modelen < sizeof(UINT32)) {
        ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"mode", modelen, sizeof(UINT32));
        return FALSE;
    }

    if (!utf8_to_utf16(hwnd, path, pathlen, &pathu))
        return FALSE;

    h = CreateFileW((subvolpath + pathu).c_str(), WRITE_DAC, 0, NULL, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        ShowRecvError(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    memset(&bsii, 0, sizeof(btrfs_set_inode_info));

    bsii.mode_changed = TRUE;
    bsii.st_mode = *mode;

    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), NULL, 0);
    if (!NT_SUCCESS(Status)) {
        ShowRecvError(IDS_RECV_SETINODEINFO_FAILED, Status, format_ntstatus(Status).c_str());
        CloseHandle(h);
        return FALSE;
    }

    CloseHandle(h);

    return TRUE;
}

BOOL BtrfsRecv::cmd_chown(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    HANDLE h;
    char* path;
    UINT32 *uid, *gid;
    ULONG pathlen, uidlen, gidlen;
    std::wstring pathu;
    btrfs_set_inode_info bsii;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!utf8_to_utf16(hwnd, path, pathlen, &pathu))
        return FALSE;

    h = CreateFileW((subvolpath + pathu).c_str(), FILE_WRITE_ATTRIBUTES | WRITE_OWNER | WRITE_DAC, 0, NULL, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        ShowRecvError(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    memset(&bsii, 0, sizeof(btrfs_set_inode_info));

    if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_UID, (void**)&uid, &uidlen)) {
        if (uidlen < sizeof(UINT32)) {
            ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"uid", uidlen, sizeof(UINT32));
            CloseHandle(h);
            return FALSE;
        }

        bsii.uid_changed = TRUE;
        bsii.st_uid = *uid;
    }

    if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_GID, (void**)&gid, &gidlen)) {
        if (gidlen < sizeof(UINT32)) {
            ShowRecvError(IDS_RECV_SHORT_PARAM, funcname, L"gid", gidlen, sizeof(UINT32));
            CloseHandle(h);
            return FALSE;
        }

        bsii.gid_changed = TRUE;
        bsii.st_gid = *gid;
    }

    if (bsii.uid_changed || bsii.gid_changed) {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;

        Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), NULL, 0);
        if (!NT_SUCCESS(Status)) {
            ShowRecvError(IDS_RECV_SETINODEINFO_FAILED, Status, format_ntstatus(Status).c_str());
            CloseHandle(h);
            return FALSE;
        }
    }

    CloseHandle(h);

    return TRUE;
}

static __inline UINT64 unix_time_to_win(BTRFS_TIME* t) {
    return (t->seconds * 10000000) + (t->nanoseconds / 100) + 116444736000000000;
}

BOOL BtrfsRecv::cmd_utimes(HWND hwnd, btrfs_send_command* cmd, UINT8* data) {
    char* path;
    ULONG pathlen;
    std::wstring pathu;
    HANDLE h;
    FILE_BASIC_INFO fbi;
    BTRFS_TIME* time;
    ULONG timelen;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen)) {
        ShowRecvError(IDS_RECV_MISSING_PARAM, funcname, L"path");
        return FALSE;
    }

    if (!utf8_to_utf16(hwnd, path, pathlen, &pathu))
        return FALSE;

    h = CreateFileW((subvolpath + pathu).c_str(), FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        ShowRecvError(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    memset(&fbi, 0, sizeof(FILE_BASIC_INFO));

    if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_OTIME, (void**)&time, &timelen) && timelen >= sizeof(BTRFS_TIME))
        fbi.CreationTime.QuadPart = unix_time_to_win(time);

    if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_ATIME, (void**)&time, &timelen) && timelen >= sizeof(BTRFS_TIME))
        fbi.LastAccessTime.QuadPart = unix_time_to_win(time);

    if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_MTIME, (void**)&time, &timelen) && timelen >= sizeof(BTRFS_TIME))
        fbi.LastWriteTime.QuadPart = unix_time_to_win(time);

    if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_CTIME, (void**)&time, &timelen) && timelen >= sizeof(BTRFS_TIME))
        fbi.ChangeTime.QuadPart = unix_time_to_win(time);

    if (!SetFileInformationByHandle(h, FileBasicInfo, &fbi, sizeof(FILE_BASIC_INFO))) {
        ShowRecvError(IDS_RECV_SETFILEINFO_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        CloseHandle(h);
        return FALSE;
    }

    CloseHandle(h);

    return TRUE;
}

void BtrfsRecv::ShowRecvError(int resid, ...) {
    WCHAR s[1024], t[1024];
    va_list ap;

    if (!hwnd)
        return;

    if (!LoadStringW(module, resid, s, sizeof(s) / sizeof(WCHAR))) {
        ShowError(hwnd, GetLastError());
        return;
    }

    va_start(ap, resid);
#ifndef __REACTOS__
    vswprintf(t, sizeof(t) / sizeof(WCHAR), s, ap);
#else
    vsnwprintf(t, sizeof(t) / sizeof(WCHAR), s, ap);
#endif

    SetDlgItemTextW(hwnd, IDC_RECV_MSG, t);

    va_end(ap);

    SendMessageW(GetDlgItem(hwnd, IDC_RECV_PROGRESS), PBM_SETSTATE, PBST_ERROR, 0);
}

static void delete_directory(std::wstring dir) {
    HANDLE h;
    WIN32_FIND_DATAW fff;

    h = FindFirstFileW((dir + L"*").c_str(), &fff);

    if (h == INVALID_HANDLE_VALUE)
        return;

    do {
        std::wstring file;

        file = fff.cFileName;

        if (file != L"." && file != L"..") {
            if (fff.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                SetFileAttributesW((dir + file).c_str(), fff.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);

            if (fff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (!(fff.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
                    delete_directory(dir + file + L"\\");
                else
                    RemoveDirectoryW((dir + file).c_str());
            } else
                DeleteFileW((dir + file).c_str());
        }
    } while (FindNextFileW(h, &fff));

    FindClose(h);

    RemoveDirectoryW(dir.c_str());
}

static BOOL check_csum(btrfs_send_command* cmd, UINT8* data) {
    UINT32 crc32 = cmd->csum, calc;

    cmd->csum = 0;

    calc = calc_crc32c(0, (UINT8*)cmd, sizeof(btrfs_send_command));

    if (cmd->length > 0)
        calc = calc_crc32c(calc, data, cmd->length);

    return calc == crc32 ? TRUE : FALSE;
}

BOOL BtrfsRecv::do_recv(HANDLE f, UINT64* pos, UINT64 size) {
    btrfs_send_header header;
    BOOL b = TRUE, ended = FALSE;

    if (!ReadFile(f, &header, sizeof(btrfs_send_header), NULL, NULL)) {
        ShowRecvError(IDS_RECV_READFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        return FALSE;
    }

    *pos += sizeof(btrfs_send_header);

    if (memcmp(header.magic, BTRFS_SEND_MAGIC, sizeof(header.magic))) {
        ShowRecvError(IDS_RECV_NOT_A_SEND_STREAM);
        return FALSE;
    }

    if (header.version > 1) {
        ShowRecvError(IDS_RECV_UNSUPPORTED_VERSION, header.version);
        return FALSE;
    }

    SendMessageW(GetDlgItem(hwnd, IDC_RECV_PROGRESS), PBM_SETRANGE32, 0, (LPARAM)65536);

    lastwritefile = INVALID_HANDLE_VALUE;
    lastwritepath = L"";
    lastwriteatt = 0;

    while (TRUE) {
        btrfs_send_command cmd;
        UINT8* data;
        ULONG progress;

        if (cancelling) {
            b = FALSE;
            break;
        }

        progress = (ULONG)((float)*pos * 65536.0f / (float)size);
        SendMessageW(GetDlgItem(hwnd, IDC_RECV_PROGRESS), PBM_SETPOS, progress, 0);

        if (!ReadFile(f, &cmd, sizeof(btrfs_send_command), NULL, NULL)) {
            if (GetLastError() != ERROR_HANDLE_EOF) {
                ShowRecvError(IDS_RECV_READFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                break;
            }

            break;
        }

        *pos += sizeof(btrfs_send_command);

        if (cmd.length > 0) {
            if (*pos + cmd.length > size) {
                ShowRecvError(IDS_RECV_FILE_TRUNCATED);
                b = FALSE;
                break;
            }

            data = (UINT8*)malloc(cmd.length);
            if (!data) {
                ShowRecvError(IDS_OUT_OF_MEMORY);
                b = FALSE;
                break;
            }

            if (!ReadFile(f, data, cmd.length, NULL, NULL)) {
                ShowRecvError(IDS_RECV_READFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                b = FALSE;
                break;
            }

            *pos += cmd.length;
        } else
            data = NULL;

        if (!check_csum(&cmd, data)) {
            ShowRecvError(IDS_RECV_CSUM_ERROR);
            if (data) free(data);
            b = FALSE;
            break;
        }

        if (cmd.cmd == BTRFS_SEND_CMD_END) {
            if (data) free(data);
            ended = TRUE;
            break;
        }

        if (lastwritefile != INVALID_HANDLE_VALUE && cmd.cmd != BTRFS_SEND_CMD_WRITE) {
            if (lastwriteatt & FILE_ATTRIBUTE_READONLY) {
                if (!SetFileAttributesW((subvolpath + lastwritepath).c_str(), lastwriteatt)) {
                    ShowRecvError(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                    return FALSE;
                }
            }

            CloseHandle(lastwritefile);

            lastwritefile = INVALID_HANDLE_VALUE;
            lastwritepath = L"";
            lastwriteatt = 0;
        }

        switch (cmd.cmd) {
            case BTRFS_SEND_CMD_SUBVOL:
                b = cmd_subvol(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_SNAPSHOT:
                b = cmd_snapshot(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_MKFILE:
            case BTRFS_SEND_CMD_MKDIR:
            case BTRFS_SEND_CMD_MKNOD:
            case BTRFS_SEND_CMD_MKFIFO:
            case BTRFS_SEND_CMD_MKSOCK:
            case BTRFS_SEND_CMD_SYMLINK:
                b = cmd_mkfile(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_RENAME:
                b = cmd_rename(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_LINK:
                b = cmd_link(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_UNLINK:
                b = cmd_unlink(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_RMDIR:
                b = cmd_rmdir(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_SET_XATTR:
                b = cmd_setxattr(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_REMOVE_XATTR:
                b = cmd_removexattr(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_WRITE:
                b = cmd_write(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_CLONE:
                b = cmd_clone(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_TRUNCATE:
                b = cmd_truncate(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_CHMOD:
                b = cmd_chmod(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_CHOWN:
                b = cmd_chown(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_UTIMES:
                b = cmd_utimes(hwnd, &cmd, data);
            break;

            case BTRFS_SEND_CMD_UPDATE_EXTENT:
                // does nothing
            break;

            default:
                ShowRecvError(IDS_RECV_UNKNOWN_COMMAND, cmd.cmd);
                b = FALSE;
            break;
        }

        if (data) free(data);

        if (!b)
            break;
    }

    if (lastwritefile != INVALID_HANDLE_VALUE) {
        if (lastwriteatt & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributesW((subvolpath + lastwritepath).c_str(), lastwriteatt)) {
                ShowRecvError(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                return FALSE;
            }
        }

        CloseHandle(lastwritefile);
    }

    if (b && !ended) {
        ShowRecvError(IDS_RECV_FILE_TRUNCATED);
        b = FALSE;
    }

    if (b) {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;
        btrfs_received_subvol brs;

        brs.generation = stransid;
        brs.uuid = subvol_uuid;

        Status = NtFsControlFile(dir, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_RECEIVED_SUBVOL, &brs, sizeof(btrfs_received_subvol),
                                 NULL, 0);
        if (!NT_SUCCESS(Status)) {
            ShowRecvError(IDS_RECV_RECEIVED_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());
            b = FALSE;
        }
    }

    CloseHandle(dir);

    if (master != INVALID_HANDLE_VALUE)
        CloseHandle(master);

    if (!b && subvolpath != L"") {
        ULONG attrib;

        attrib = GetFileAttributesW(subvolpath.c_str());
        attrib &= ~FILE_ATTRIBUTE_READONLY;

        if (!SetFileAttributesW(subvolpath.c_str(), attrib))
            ShowRecvError(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        else
            delete_directory(subvolpath);
    }

    return b;
}

DWORD BtrfsRecv::recv_thread() {
    HANDLE f;
    LARGE_INTEGER size;
    UINT64 pos = 0;
    BOOL b;

    running = TRUE;

    f = CreateFileW(streamfile.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) {
        ShowRecvError(IDS_RECV_CANT_OPEN_FILE, funcname, streamfile.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        goto end;
    }

    if (!GetFileSizeEx(f, &size)) {
        ShowRecvError(IDS_RECV_GETFILESIZEEX_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        goto end;
    }

    parent = CreateFileW(dirpath.c_str(), FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, NULL);
    if (parent == INVALID_HANDLE_VALUE) {
        ShowRecvError(IDS_RECV_CANT_OPEN_PATH, dirpath.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        CloseHandle(f);
        goto end;
    }

    do {
        b = do_recv(f, &pos, size.QuadPart);
    } while (b && pos < (UINT64)size.QuadPart);

    CloseHandle(parent);
    CloseHandle(f);

    if (b && hwnd) {
        WCHAR s[255];

        SendMessageW(GetDlgItem(hwnd, IDC_RECV_PROGRESS), PBM_SETPOS, 65536, 0);

        if (num_received == 1) {
            if (!LoadStringW(module, IDS_RECV_SUCCESS, s, sizeof(s) / sizeof(WCHAR)))
                ShowError(hwnd, GetLastError());
            else
                SetDlgItemTextW(hwnd, IDC_RECV_MSG, s);
        } else {
            if (!LoadStringW(module, IDS_RECV_SUCCESS_PLURAL, s, sizeof(s) / sizeof(WCHAR)))
                ShowError(hwnd, GetLastError());
            else {
                WCHAR t[255];

                if (StringCchPrintfW(t, sizeof(t) / sizeof(WCHAR), s, num_received) == STRSAFE_E_INSUFFICIENT_BUFFER)
                    ShowError(hwnd, ERROR_INSUFFICIENT_BUFFER);
                else
                    SetDlgItemTextW(hwnd, IDC_RECV_MSG, t);
            }
        }

        if (!LoadStringW(module, IDS_RECV_BUTTON_OK, s, sizeof(s) / sizeof(WCHAR)))
            ShowError(hwnd, GetLastError());
        else
            SetDlgItemTextW(hwnd, IDCANCEL, s);
    }

end:
    thread = NULL;
    running = FALSE;

    return 0;
}

static DWORD WINAPI global_recv_thread(LPVOID lpParameter) {
    BtrfsRecv* br = (BtrfsRecv*)lpParameter;

    return br->recv_thread();
}

INT_PTR CALLBACK BtrfsRecv::RecvProgressDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            this->hwnd = hwndDlg;
            thread = CreateThread(NULL, 0, global_recv_thread, this, 0, NULL);

            if (!thread)
                ShowRecvError(IDS_RECV_CREATETHREAD_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        break;

        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    switch (LOWORD(wParam)) {
                        case IDOK:
                        case IDCANCEL:
                            if (running) {
                                WCHAR s[255];

                                cancelling = TRUE;

                                if (!LoadStringW(module, IDS_RECV_CANCELLED, s, sizeof(s) / sizeof(WCHAR))) {
                                    ShowError(hwndDlg, GetLastError());
                                    return FALSE;
                                }

                                SetDlgItemTextW(hwnd, IDC_RECV_MSG, s);
                                SendMessageW(GetDlgItem(hwnd, IDC_RECV_PROGRESS), PBM_SETPOS, 0, 0);

                                if (!LoadStringW(module, IDS_RECV_BUTTON_OK, s, sizeof(s) / sizeof(WCHAR))) {
                                    ShowError(hwndDlg, GetLastError());
                                    return FALSE;
                                }

                                SetDlgItemTextW(hwnd, IDCANCEL, s);
                            } else
                                EndDialog(hwndDlg, 1);

                            return TRUE;
                    }
                break;
            }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK stub_RecvProgressDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsRecv* br;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        br = (BtrfsRecv*)lParam;
    } else {
        br = (BtrfsRecv*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    }

    if (br)
        return br->RecvProgressDlgProc(hwndDlg, uMsg, wParam, lParam);
    else
        return FALSE;
}

void BtrfsRecv::Open(HWND hwnd, WCHAR* file, WCHAR* path, BOOL quiet) {
#ifndef __REACTOS__
    UINT32 cpuInfo[4];
#endif

    streamfile = file;
    dirpath = path;
    subvolpath = L"";

#ifndef __REACTOS__
#ifndef _MSC_VER
    __get_cpuid(1, &cpuInfo[0], &cpuInfo[1], &cpuInfo[2], &cpuInfo[3]);
    have_sse42 = cpuInfo[2] & bit_SSE4_2;
#else
    __cpuid((int*)cpuInfo, 1);
    have_sse42 = cpuInfo[2] & (1 << 20);
#endif
#endif

    if (quiet)
        recv_thread();
    else {
        if (DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_RECV_PROGRESS), hwnd, stub_RecvProgressDlgProc, (LPARAM)this) <= 0)
            ShowError(hwnd, GetLastError());
    }
}

void CALLBACK RecvSubvolGUIW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    OPENFILENAMEW ofn;
    WCHAR file[MAX_PATH];
    BtrfsRecv* recv;
    HANDLE token;
    TOKEN_PRIVILEGES* tp;
    LUID luid;
    ULONG tplen;

    set_dpi_aware();

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        ShowError(hwnd, GetLastError());
        return;
    }

    tplen = offsetof(TOKEN_PRIVILEGES, Privileges[0]) + (3 * sizeof(LUID_AND_ATTRIBUTES));
    tp = (TOKEN_PRIVILEGES*)malloc(tplen);
    if (!tp) {
        ShowStringError(hwnd, IDS_OUT_OF_MEMORY);
        CloseHandle(token);
        return;
    }

    tp->PrivilegeCount = 3;

    if (!LookupPrivilegeValueW(NULL, L"SeManageVolumePrivilege", &luid)) {
        ShowError(hwnd, GetLastError());
        free(tp);
        CloseHandle(token);
        return;
    }

    tp->Privileges[0].Luid = luid;
    tp->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!LookupPrivilegeValueW(NULL, L"SeSecurityPrivilege", &luid)) {
        ShowError(hwnd, GetLastError());
        free(tp);
        CloseHandle(token);
        return;
    }

    tp->Privileges[1].Luid = luid;
    tp->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    if (!LookupPrivilegeValueW(NULL, L"SeRestorePrivilege", &luid)) {
        ShowError(hwnd, GetLastError());
        free(tp);
        CloseHandle(token);
        return;
    }

    tp->Privileges[2].Luid = luid;
    tp->Privileges[2].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(token, FALSE, tp, tplen, NULL, NULL)) {
        ShowError(hwnd, GetLastError());
        free(tp);
        CloseHandle(token);
        return;
    }

    file[0] = 0;

    memset(&ofn, 0, sizeof(OPENFILENAMEW));
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = module;
    ofn.lpstrFile = file;
    ofn.nMaxFile = sizeof(file) / sizeof(WCHAR);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameW(&ofn)) {
        recv = new BtrfsRecv;

        recv->Open(hwnd, file, lpszCmdLine, FALSE);

        delete recv;
    }

    free(tp);
    CloseHandle(token);
}

void CALLBACK RecvSubvolW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    LPWSTR* args;
    int num_args;

    args = CommandLineToArgvW(lpszCmdLine, &num_args);

    if (!args)
        return;

    if (num_args >= 2) {
        BtrfsRecv* br;
        HANDLE token;
        TOKEN_PRIVILEGES* tp;
        ULONG tplen;
        LUID luid;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            goto end;

        tplen = offsetof(TOKEN_PRIVILEGES, Privileges[0]) + (3 * sizeof(LUID_AND_ATTRIBUTES));
        tp = (TOKEN_PRIVILEGES*)malloc(tplen);
        if (!tp) {
            CloseHandle(token);
            goto end;
        }

        tp->PrivilegeCount = 3;

        if (!LookupPrivilegeValueW(NULL, L"SeManageVolumePrivilege", &luid)) {
            free(tp);
            CloseHandle(token);
            goto end;
        }

        tp->Privileges[0].Luid = luid;
        tp->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!LookupPrivilegeValueW(NULL, L"SeSecurityPrivilege", &luid)) {
            free(tp);
            CloseHandle(token);
            goto end;
        }

        tp->Privileges[1].Luid = luid;
        tp->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

        if (!LookupPrivilegeValueW(NULL, L"SeRestorePrivilege", &luid)) {
            free(tp);
            CloseHandle(token);
            goto end;
        }

        tp->Privileges[2].Luid = luid;
        tp->Privileges[2].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(token, FALSE, tp, tplen, NULL, NULL)) {
            free(tp);
            CloseHandle(token);
            goto end;
        }

        free(tp);
        CloseHandle(token);

        br = new BtrfsRecv;
        br->Open(NULL, args[0], args[1], TRUE);

        delete br;
    }

end:
    LocalFree(args);
}
