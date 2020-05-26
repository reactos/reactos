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
#include <iostream>
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

const string EA_NTACL = "security.NTACL";
const string EA_DOSATTRIB = "user.DOSATTRIB";
const string EA_REPARSE = "user.reparse";
const string EA_EA = "user.EA";
const string XATTR_USER = "user.";

#ifndef __REACTOS__
bool have_sse42 = false;
#endif

static const uint32_t crctable[] = {
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
    for (; (len) >= sizeof (type); (len) -= (ULONG)sizeof(type), buf += sizeof (type)) { \
        (crc) = (uint32_t)op((crc), *(type *) (buf));                               \
    }                                                                   \
} while(0)

#ifndef __REACTOS__
static uint32_t crc32c_hw(const void *input, ULONG len, uint32_t crc) {
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
#pragma warning(disable:4244) // _mm_crc32_u64 wants to return uint64_t(!)
#pragma warning(disable:4242)
#endif
    CALC_CRC(_mm_crc32_u64, crc, uint64_t, buf, len);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
    CALC_CRC(_mm_crc32_u32, crc, uint32_t, buf, len);

#ifdef _MSC_VER
    for (; len > 0; len--, buf++) {
        crc = crctable[(crc ^ *buf) & 0xff] ^ (crc >> 8);
    }
#else
    CALC_CRC(_mm_crc32_u16, crc, uint16_t, buf, len);
    CALC_CRC(_mm_crc32_u8, crc, uint8_t, buf, len);
#endif

    return crc;
}
#endif

static uint32_t calc_crc32c(uint32_t seed, uint8_t* msg, ULONG msglen) {
#ifndef __REACTOS__
    if (have_sse42)
        return crc32c_hw(msg, msglen, seed);
    else {
#endif
        uint32_t rem;
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

bool BtrfsRecv::find_tlv(uint8_t* data, ULONG datalen, uint16_t type, void** value, ULONG* len) {
    size_t off = 0;

    while (off < datalen) {
        btrfs_send_tlv* tlv = (btrfs_send_tlv*)(data + off);
        uint8_t* payload = data + off + sizeof(btrfs_send_tlv);

        if (off + sizeof(btrfs_send_tlv) + tlv->length > datalen) // file is truncated
            return false;

        if (tlv->type == type) {
            *value = payload;
            *len = tlv->length;
            return true;
        }

        off += sizeof(btrfs_send_tlv) + tlv->length;
    }

    return false;
}

void BtrfsRecv::cmd_subvol(HWND hwnd, btrfs_send_command* cmd, uint8_t* data, const win_handle& parent) {
    string name;
    BTRFS_UUID* uuid;
    uint64_t* gen;
    ULONG uuidlen, genlen;
    btrfs_create_subvol* bcs;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;

    {
        char* namebuf;
        ULONG namelen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&namebuf, &namelen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        name = string(namebuf, namelen);
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_UUID, (void**)&uuid, &uuidlen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"uuid");

    if (uuidlen < sizeof(BTRFS_UUID))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"uuid", uuidlen, sizeof(BTRFS_UUID));

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_TRANSID, (void**)&gen, &genlen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"transid");

    if (genlen < sizeof(uint64_t))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"transid", genlen, sizeof(uint64_t));

    this->subvol_uuid = *uuid;
    this->stransid = *gen;

    auto nameu = utf8_to_utf16(name);

    size_t bcslen = offsetof(btrfs_create_subvol, name[0]) + (nameu.length() * sizeof(WCHAR));
    bcs = (btrfs_create_subvol*)malloc(bcslen);

    bcs->readonly = true;
    bcs->posix = true;
    bcs->namelen = (uint16_t)(nameu.length() * sizeof(WCHAR));
    memcpy(bcs->name, nameu.c_str(), bcs->namelen);

    Status = NtFsControlFile(parent, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_CREATE_SUBVOL, bcs, (ULONG)bcslen, nullptr, 0);
    if (!NT_SUCCESS(Status))
        throw string_error(IDS_RECV_CREATE_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());

    subvolpath = dirpath;
    subvolpath += L"\\";
    subvolpath += nameu;

    if (dir != INVALID_HANDLE_VALUE)
        CloseHandle(dir);

    if (master != INVALID_HANDLE_VALUE)
        CloseHandle(master);

    master = CreateFileW(subvolpath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, nullptr);
    if (master == INVALID_HANDLE_VALUE)
        throw string_error(IDS_RECV_CANT_OPEN_PATH, subvolpath.c_str(), GetLastError(), format_message(GetLastError()).c_str());

    Status = NtFsControlFile(master, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_RESERVE_SUBVOL, bcs, (ULONG)bcslen, nullptr, 0);
    if (!NT_SUCCESS(Status))
        throw string_error(IDS_RECV_RESERVE_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());

    dir = CreateFileW(subvolpath.c_str(), FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, nullptr);
    if (dir == INVALID_HANDLE_VALUE)
        throw string_error(IDS_RECV_CANT_OPEN_PATH, subvolpath.c_str(), GetLastError(), format_message(GetLastError()).c_str());

    subvolpath += L"\\";

    add_cache_entry(&this->subvol_uuid, this->stransid, subvolpath);

    num_received++;
}

void BtrfsRecv::add_cache_entry(BTRFS_UUID* uuid, uint64_t transid, const wstring& path) {
    subvol_cache sc;

    sc.uuid = *uuid;
    sc.transid = transid;
    sc.path = path;

    cache.push_back(sc);
}

void BtrfsRecv::cmd_snapshot(HWND hwnd, btrfs_send_command* cmd, uint8_t* data, const win_handle& parent) {
    string name;
    BTRFS_UUID *uuid, *parent_uuid;
    uint64_t *gen, *parent_transid;
    ULONG uuidlen, genlen, paruuidlen, partransidlen;
    btrfs_create_snapshot* bcs;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    wstring parpath;
    btrfs_find_subvol bfs;
    WCHAR parpathw[MAX_PATH], volpathw[MAX_PATH];
    size_t bcslen;

    {
        char* namebuf;
        ULONG namelen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&namebuf, &namelen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        name = string(namebuf, namelen);
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_UUID, (void**)&uuid, &uuidlen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"uuid");

    if (uuidlen < sizeof(BTRFS_UUID))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"uuid", uuidlen, sizeof(BTRFS_UUID));

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_TRANSID, (void**)&gen, &genlen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"transid");

    if (genlen < sizeof(uint64_t))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"transid", genlen, sizeof(uint64_t));

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_UUID, (void**)&parent_uuid, &paruuidlen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"clone_uuid");

    if (paruuidlen < sizeof(BTRFS_UUID))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"clone_uuid", paruuidlen, sizeof(BTRFS_UUID));

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_CTRANSID, (void**)&parent_transid, &partransidlen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"clone_ctransid");

    if (partransidlen < sizeof(uint64_t))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"clone_ctransid", partransidlen, sizeof(uint64_t));

    this->subvol_uuid = *uuid;
    this->stransid = *gen;

    auto nameu = utf8_to_utf16(name);

    bfs.uuid = *parent_uuid;
    bfs.ctransid = *parent_transid;

    Status = NtFsControlFile(parent, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_FIND_SUBVOL, &bfs, sizeof(btrfs_find_subvol),
                             parpathw, sizeof(parpathw));
    if (Status == STATUS_NOT_FOUND)
        throw string_error(IDS_RECV_CANT_FIND_PARENT_SUBVOL);
    else if (!NT_SUCCESS(Status))
        throw string_error(IDS_RECV_FIND_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());

    if (!GetVolumePathNameW(dirpath.c_str(), volpathw, (sizeof(volpathw) / sizeof(WCHAR)) - 1))
        throw string_error(IDS_RECV_GETVOLUMEPATHNAME_FAILED, GetLastError(), format_message(GetLastError()).c_str());

    parpath = volpathw;
    if (parpath.substr(parpath.length() - 1) == L"\\")
        parpath = parpath.substr(0, parpath.length() - 1);

    parpath += parpathw;

    {
        win_handle subvol = CreateFileW(parpath.c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (subvol == INVALID_HANDLE_VALUE)
            throw string_error(IDS_RECV_CANT_OPEN_PATH, parpath.c_str(), GetLastError(), format_message(GetLastError()).c_str());

        bcslen = offsetof(btrfs_create_snapshot, name[0]) + (nameu.length() * sizeof(WCHAR));
        bcs = (btrfs_create_snapshot*)malloc(bcslen);

        bcs->readonly = true;
        bcs->posix = true;
        bcs->subvol = subvol;
        bcs->namelen = (uint16_t)(nameu.length() * sizeof(WCHAR));
        memcpy(bcs->name, nameu.c_str(), bcs->namelen);

        Status = NtFsControlFile(parent, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_CREATE_SNAPSHOT, bcs, (ULONG)bcslen, nullptr, 0);
        if (!NT_SUCCESS(Status))
            throw string_error(IDS_RECV_CREATE_SNAPSHOT_FAILED, Status, format_ntstatus(Status).c_str());
    }

    subvolpath = dirpath;
    subvolpath += L"\\";
    subvolpath += nameu;

    if (dir != INVALID_HANDLE_VALUE)
        CloseHandle(dir);

    if (master != INVALID_HANDLE_VALUE)
        CloseHandle(master);

    master = CreateFileW(subvolpath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, nullptr);
    if (master == INVALID_HANDLE_VALUE)
        throw string_error(IDS_RECV_CANT_OPEN_PATH, subvolpath.c_str(), GetLastError(), format_message(GetLastError()).c_str());

    Status = NtFsControlFile(master, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_RESERVE_SUBVOL, bcs, (ULONG)bcslen, nullptr, 0);
    if (!NT_SUCCESS(Status))
        throw string_error(IDS_RECV_RESERVE_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());

    dir = CreateFileW(subvolpath.c_str(), FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, nullptr);
    if (dir == INVALID_HANDLE_VALUE)
        throw string_error(IDS_RECV_CANT_OPEN_PATH, subvolpath.c_str(), GetLastError(), format_message(GetLastError()).c_str());

    subvolpath += L"\\";

    add_cache_entry(&this->subvol_uuid, this->stransid, subvolpath);

    num_received++;
}

void BtrfsRecv::cmd_mkfile(HWND hwnd, btrfs_send_command* cmd, uint8_t* data) {
    uint64_t *inode, *rdev = nullptr, *mode = nullptr;
    ULONG inodelen;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_mknod* bmn;
    wstring nameu, pathlinku;

    {
        char* name;
        ULONG namelen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&name, &namelen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        nameu = utf8_to_utf16(string(name, namelen));
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_INODE, (void**)&inode, &inodelen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"inode");

    if (inodelen < sizeof(uint64_t))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"inode", inodelen, sizeof(uint64_t));

    if (cmd->cmd == BTRFS_SEND_CMD_MKNOD || cmd->cmd == BTRFS_SEND_CMD_MKFIFO || cmd->cmd == BTRFS_SEND_CMD_MKSOCK) {
        ULONG rdevlen, modelen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_RDEV, (void**)&rdev, &rdevlen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"rdev");

        if (rdevlen < sizeof(uint64_t))
            throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"rdev", rdev, sizeof(uint64_t));

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_MODE, (void**)&mode, &modelen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"mode");

        if (modelen < sizeof(uint64_t))
            throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"mode", modelen, sizeof(uint64_t));
    } else if (cmd->cmd == BTRFS_SEND_CMD_SYMLINK) {
        char* pathlink;
        ULONG pathlinklen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH_LINK, (void**)&pathlink, &pathlinklen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path_link");

        pathlinku = utf8_to_utf16(string(pathlink, pathlinklen));
    }

    size_t bmnsize = sizeof(btrfs_mknod) - sizeof(WCHAR) + (nameu.length() * sizeof(WCHAR));
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
    bmn->namelen = (uint16_t)(nameu.length() * sizeof(WCHAR));
    memcpy(bmn->name, nameu.c_str(), bmn->namelen);

    Status = NtFsControlFile(dir, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_MKNOD, bmn, (ULONG)bmnsize, nullptr, 0);
    if (!NT_SUCCESS(Status)) {
        free(bmn);
        throw string_error(IDS_RECV_MKNOD_FAILED, Status, format_ntstatus(Status).c_str());
    }

    free(bmn);

    if (cmd->cmd == BTRFS_SEND_CMD_SYMLINK) {
        REPARSE_DATA_BUFFER* rdb;
        btrfs_set_inode_info bsii;

        size_t rdblen = offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer[0]) + (2 * pathlinku.length() * sizeof(WCHAR));

        if (rdblen >= 0x10000)
            throw string_error(IDS_RECV_PATH_TOO_LONG, funcname);

        rdb = (REPARSE_DATA_BUFFER*)malloc(rdblen);

        rdb->ReparseTag = IO_REPARSE_TAG_SYMLINK;
        rdb->ReparseDataLength = (uint16_t)(rdblen - offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer));
        rdb->Reserved = 0;
        rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
        rdb->SymbolicLinkReparseBuffer.SubstituteNameLength = (uint16_t)(pathlinku.length() * sizeof(WCHAR));
        rdb->SymbolicLinkReparseBuffer.PrintNameOffset = (uint16_t)(pathlinku.length() * sizeof(WCHAR));
        rdb->SymbolicLinkReparseBuffer.PrintNameLength = (uint16_t)(pathlinku.length() * sizeof(WCHAR));
        rdb->SymbolicLinkReparseBuffer.Flags = SYMLINK_FLAG_RELATIVE;

        memcpy(rdb->SymbolicLinkReparseBuffer.PathBuffer, pathlinku.c_str(), rdb->SymbolicLinkReparseBuffer.SubstituteNameLength);
        memcpy(rdb->SymbolicLinkReparseBuffer.PathBuffer + (rdb->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(WCHAR)),
                pathlinku.c_str(), rdb->SymbolicLinkReparseBuffer.PrintNameLength);

        win_handle h = CreateFileW((subvolpath + nameu).c_str(), GENERIC_WRITE | WRITE_DAC, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            free(rdb);
            throw string_error(IDS_RECV_CANT_OPEN_FILE, funcname, nameu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        }

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_SET_REPARSE_POINT, rdb, (ULONG)rdblen, nullptr, 0);
        if (!NT_SUCCESS(Status)) {
            free(rdb);
            throw string_error(IDS_RECV_SET_REPARSE_POINT_FAILED, Status, format_ntstatus(Status).c_str());
        }

        free(rdb);

        memset(&bsii, 0, sizeof(btrfs_set_inode_info));

        bsii.mode_changed = true;
        bsii.st_mode = 0777;

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), nullptr, 0);
        if (!NT_SUCCESS(Status))
            throw string_error(IDS_RECV_SETINODEINFO_FAILED, Status, format_ntstatus(Status).c_str());
    } else if (cmd->cmd == BTRFS_SEND_CMD_MKNOD || cmd->cmd == BTRFS_SEND_CMD_MKFIFO || cmd->cmd == BTRFS_SEND_CMD_MKSOCK) {
        uint64_t* mode;
        ULONG modelen;

        if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_MODE, (void**)&mode, &modelen)) {
            btrfs_set_inode_info bsii;

            if (modelen < sizeof(uint64_t))
                throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"mode", modelen, sizeof(uint64_t));

            win_handle h = CreateFileW((subvolpath + nameu).c_str(), WRITE_DAC, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                       nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, nullptr);
            if (h == INVALID_HANDLE_VALUE)
                throw string_error(IDS_RECV_CANT_OPEN_FILE, funcname, nameu.c_str(), GetLastError(), format_message(GetLastError()).c_str());

            memset(&bsii, 0, sizeof(btrfs_set_inode_info));

            bsii.mode_changed = true;
            bsii.st_mode = (uint32_t)*mode;

            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), nullptr, 0);
            if (!NT_SUCCESS(Status))
                throw string_error(IDS_RECV_SETINODEINFO_FAILED, Status, format_ntstatus(Status).c_str());
        }
    }
}

void BtrfsRecv::cmd_rename(HWND hwnd, btrfs_send_command* cmd, uint8_t* data) {
    wstring pathu, path_tou;

    {
        char* path;
        ULONG path_len;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &path_len))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        pathu = utf8_to_utf16(string(path, path_len));
    }

    {
        char* path_to;
        ULONG path_to_len;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH_TO, (void**)&path_to, &path_to_len))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path_to");

        path_tou = utf8_to_utf16(string(path_to, path_to_len));
    }

    if (!MoveFileW((subvolpath + pathu).c_str(), (subvolpath + path_tou).c_str()))
        throw string_error(IDS_RECV_MOVEFILE_FAILED, pathu.c_str(), path_tou.c_str(), GetLastError(), format_message(GetLastError()).c_str());
}

void BtrfsRecv::cmd_link(HWND hwnd, btrfs_send_command* cmd, uint8_t* data) {
    wstring pathu, path_linku;

    {
        char* path;
        ULONG path_len;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &path_len))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        pathu = utf8_to_utf16(string(path, path_len));
    }

    {
        char* path_link;
        ULONG path_link_len;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH_LINK, (void**)&path_link, &path_link_len))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path_link");

        path_linku = utf8_to_utf16(string(path_link, path_link_len));
    }

    if (!CreateHardLinkW((subvolpath + pathu).c_str(), (subvolpath + path_linku).c_str(), nullptr))
        throw string_error(IDS_RECV_CREATEHARDLINK_FAILED, pathu.c_str(), path_linku.c_str(), GetLastError(), format_message(GetLastError()).c_str());
}

void BtrfsRecv::cmd_unlink(HWND hwnd, btrfs_send_command* cmd, uint8_t* data) {
    wstring pathu;
    ULONG att;

    {
        char* path;
        ULONG pathlen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        pathu = utf8_to_utf16(string(path, pathlen));
    }

    att = GetFileAttributesW((subvolpath + pathu).c_str());
    if (att == INVALID_FILE_ATTRIBUTES)
        throw string_error(IDS_RECV_GETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());

    if (att & FILE_ATTRIBUTE_READONLY) {
        if (!SetFileAttributesW((subvolpath + pathu).c_str(), att & ~FILE_ATTRIBUTE_READONLY))
            throw string_error(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
    }

    if (!DeleteFileW((subvolpath + pathu).c_str()))
        throw string_error(IDS_RECV_DELETEFILE_FAILED, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
}

void BtrfsRecv::cmd_rmdir(HWND hwnd, btrfs_send_command* cmd, uint8_t* data) {
    wstring pathu;
    ULONG att;

    {
        char* path;
        ULONG pathlen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        pathu = utf8_to_utf16(string(path, pathlen));
    }

    att = GetFileAttributesW((subvolpath + pathu).c_str());
    if (att == INVALID_FILE_ATTRIBUTES)
        throw string_error(IDS_RECV_GETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());

    if (att & FILE_ATTRIBUTE_READONLY) {
        if (!SetFileAttributesW((subvolpath + pathu).c_str(), att & ~FILE_ATTRIBUTE_READONLY))
            throw string_error(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
    }

    if (!RemoveDirectoryW((subvolpath + pathu).c_str()))
        throw string_error(IDS_RECV_REMOVEDIRECTORY_FAILED, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());
}

void BtrfsRecv::cmd_setxattr(HWND hwnd, btrfs_send_command* cmd, uint8_t* data) {
    string xattrname;
    uint8_t* xattrdata;
    ULONG xattrdatalen;
    wstring pathu;

    {
        char* path;
        ULONG pathlen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        pathu = utf8_to_utf16(string(path, pathlen));
    }

    {
        char* xattrnamebuf;
        ULONG xattrnamelen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_XATTR_NAME, (void**)&xattrnamebuf, &xattrnamelen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"xattr_name");

        xattrname = string(xattrnamebuf, xattrnamelen);
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_XATTR_DATA, (void**)&xattrdata, &xattrdatalen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"xattr_data");

    if (xattrname.length() > XATTR_USER.length() && xattrname.substr(0, XATTR_USER.length()) == XATTR_USER &&
        xattrname != EA_DOSATTRIB && xattrname != EA_EA && xattrname != EA_REPARSE) {
        ULONG att;

        auto streamname = utf8_to_utf16(xattrname);

        att = GetFileAttributesW((subvolpath + pathu).c_str());
        if (att == INVALID_FILE_ATTRIBUTES)
            throw string_error(IDS_RECV_GETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());

        if (att & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributesW((subvolpath + pathu).c_str(), att & ~FILE_ATTRIBUTE_READONLY))
                throw string_error(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        }

        streamname = streamname.substr(XATTR_USER.length());

        win_handle h = CreateFileW((subvolpath + pathu + L":" + streamname).c_str(), GENERIC_WRITE, 0,
                                   nullptr, CREATE_ALWAYS, FILE_FLAG_POSIX_SEMANTICS, nullptr);
        if (h == INVALID_HANDLE_VALUE)
            throw string_error(IDS_RECV_CANT_CREATE_FILE, (pathu + L":" + streamname).c_str(), GetLastError(), format_message(GetLastError()).c_str());

        if (xattrdatalen > 0) {
            if (!WriteFile(h, xattrdata, xattrdatalen, nullptr, nullptr))
                throw string_error(IDS_RECV_WRITEFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        }

        if (att & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributesW((subvolpath + pathu).c_str(), att))
                throw string_error(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        }
    } else {
        IO_STATUS_BLOCK iosb;
        NTSTATUS Status;
        ULONG perms = FILE_WRITE_ATTRIBUTES;
        btrfs_set_xattr* bsxa;

        if (xattrname == EA_NTACL)
            perms |= WRITE_DAC | WRITE_OWNER;
        else if (xattrname == EA_EA)
            perms |= FILE_WRITE_EA;

        win_handle h = CreateFileW((subvolpath + pathu).c_str(), perms, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS | FILE_OPEN_REPARSE_POINT, nullptr);
        if (h == INVALID_HANDLE_VALUE)
            throw string_error(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());

        size_t bsxalen = offsetof(btrfs_set_xattr, data[0]) + xattrname.length() + xattrdatalen;
        bsxa = (btrfs_set_xattr*)malloc(bsxalen);
        if (!bsxa)
            throw string_error(IDS_OUT_OF_MEMORY);

        bsxa->namelen = (uint16_t)xattrname.length();
        bsxa->valuelen = (uint16_t)xattrdatalen;
        memcpy(bsxa->data, xattrname.c_str(), xattrname.length());
        memcpy(&bsxa->data[xattrname.length()], xattrdata, xattrdatalen);

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_SET_XATTR, bsxa, (ULONG)bsxalen, nullptr, 0);
        if (!NT_SUCCESS(Status)) {
            free(bsxa);
            throw string_error(IDS_RECV_SETXATTR_FAILED, Status, format_ntstatus(Status).c_str());
        }

        free(bsxa);
    }
}

void BtrfsRecv::cmd_removexattr(HWND hwnd, btrfs_send_command* cmd, uint8_t* data) {
    wstring pathu;
    string xattrname;

    {
        char* path;
        ULONG pathlen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        pathu = utf8_to_utf16(string(path, pathlen));
    }

    {
        char* xattrnamebuf;
        ULONG xattrnamelen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_XATTR_NAME, (void**)&xattrnamebuf, &xattrnamelen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"xattr_name");

        xattrname = string(xattrnamebuf, xattrnamelen);
    }

    if (xattrname.length() > XATTR_USER.length() && xattrname.substr(0, XATTR_USER.length()) == XATTR_USER && xattrname != EA_DOSATTRIB && xattrname != EA_EA) { // deleting stream
        ULONG att;

        auto streamname = utf8_to_utf16(xattrname);

        streamname = streamname.substr(XATTR_USER.length());

        att = GetFileAttributesW((subvolpath + pathu).c_str());
        if (att == INVALID_FILE_ATTRIBUTES)
            throw string_error(IDS_RECV_GETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());

        if (att & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributesW((subvolpath + pathu).c_str(), att & ~FILE_ATTRIBUTE_READONLY))
                throw string_error(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        }

        if (!DeleteFileW((subvolpath + pathu + L":" + streamname).c_str()))
            throw string_error(IDS_RECV_DELETEFILE_FAILED, (pathu + L":" + streamname).c_str(), GetLastError(), format_message(GetLastError()).c_str());

        if (att & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributesW((subvolpath + pathu).c_str(), att))
                throw string_error(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        }
    } else {
        IO_STATUS_BLOCK iosb;
        NTSTATUS Status;
        ULONG perms = FILE_WRITE_ATTRIBUTES;
        btrfs_set_xattr* bsxa;

        if (xattrname == EA_NTACL)
            perms |= WRITE_DAC | WRITE_OWNER;
        else if (xattrname == EA_EA)
            perms |= FILE_WRITE_EA;

        win_handle h = CreateFileW((subvolpath + pathu).c_str(), perms, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS | FILE_OPEN_REPARSE_POINT, nullptr);
        if (h == INVALID_HANDLE_VALUE)
            throw string_error(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());

        size_t bsxalen = offsetof(btrfs_set_xattr, data[0]) + xattrname.length();
        bsxa = (btrfs_set_xattr*)malloc(bsxalen);
        if (!bsxa)
            throw string_error(IDS_OUT_OF_MEMORY);

        bsxa->namelen = (uint16_t)(xattrname.length());
        bsxa->valuelen = 0;
        memcpy(bsxa->data, xattrname.c_str(), xattrname.length());

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_SET_XATTR, bsxa, (ULONG)bsxalen, nullptr, 0);
        if (!NT_SUCCESS(Status)) {
            free(bsxa);
            throw string_error(IDS_RECV_SETXATTR_FAILED, Status, format_ntstatus(Status).c_str());
        }

        free(bsxa);
    }
}

void BtrfsRecv::cmd_write(HWND hwnd, btrfs_send_command* cmd, uint8_t* data) {
    uint64_t* offset;
    uint8_t* writedata;
    ULONG offsetlen, datalen;
    wstring pathu;
    HANDLE h;
    LARGE_INTEGER offli;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;

    {
        char* path;
        ULONG pathlen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        pathu = utf8_to_utf16(string(path, pathlen));
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_OFFSET, (void**)&offset, &offsetlen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"offset");

    if (offsetlen < sizeof(uint64_t))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"offset", offsetlen, sizeof(uint64_t));

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_DATA, (void**)&writedata, &datalen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"data");

    if (lastwritepath != pathu) {
        FILE_BASIC_INFO fbi;

        if (lastwriteatt & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributesW((subvolpath + lastwritepath).c_str(), lastwriteatt))
                throw string_error(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        }

        CloseHandle(lastwritefile);

        lastwriteatt = GetFileAttributesW((subvolpath + pathu).c_str());
        if (lastwriteatt == INVALID_FILE_ATTRIBUTES)
            throw string_error(IDS_RECV_GETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());

        if (lastwriteatt & FILE_ATTRIBUTE_READONLY) {
            if (!SetFileAttributesW((subvolpath + pathu).c_str(), lastwriteatt & ~FILE_ATTRIBUTE_READONLY))
                throw string_error(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        }

        h = CreateFileW((subvolpath + pathu).c_str(), FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES, 0, nullptr, OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, nullptr);
        if (h == INVALID_HANDLE_VALUE)
            throw string_error(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());

        lastwritepath = pathu;
        lastwritefile = h;

        memset(&fbi, 0, sizeof(FILE_BASIC_INFO));

        fbi.LastWriteTime.QuadPart = -1;

        Status = NtSetInformationFile(h, &iosb, &fbi, sizeof(FILE_BASIC_INFO), FileBasicInformation);
        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);
    } else
        h = lastwritefile;

    offli.QuadPart = *offset;

    if (SetFilePointer(h, offli.LowPart, &offli.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        throw string_error(IDS_RECV_SETFILEPOINTER_FAILED, GetLastError(), format_message(GetLastError()).c_str());

    if (!WriteFile(h, writedata, datalen, nullptr, nullptr))
        throw string_error(IDS_RECV_WRITEFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
}

void BtrfsRecv::cmd_clone(HWND hwnd, btrfs_send_command* cmd, uint8_t* data) {
    uint64_t *offset, *cloneoffset, *clonetransid, *clonelen;
    BTRFS_UUID* cloneuuid;
    ULONG i, offsetlen, cloneoffsetlen, cloneuuidlen, clonetransidlen, clonelenlen;
    wstring pathu, clonepathu, clonepar;
    btrfs_find_subvol bfs;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    WCHAR cloneparw[MAX_PATH];
    DUPLICATE_EXTENTS_DATA ded;
    LARGE_INTEGER filesize;
    bool found = false;

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_OFFSET, (void**)&offset, &offsetlen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"offset");

    if (offsetlen < sizeof(uint64_t))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"offset", offsetlen, sizeof(uint64_t));

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_LENGTH, (void**)&clonelen, &clonelenlen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"clone_len");

    if (clonelenlen < sizeof(uint64_t))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"clone_len", clonelenlen, sizeof(uint64_t));

    {
        char* path;
        ULONG pathlen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        pathu = utf8_to_utf16(string(path, pathlen));
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_UUID, (void**)&cloneuuid, &cloneuuidlen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"clone_uuid");

    if (cloneuuidlen < sizeof(BTRFS_UUID))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"clone_uuid", cloneuuidlen, sizeof(BTRFS_UUID));

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_CTRANSID, (void**)&clonetransid, &clonetransidlen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"clone_ctransid");

    if (clonetransidlen < sizeof(uint64_t))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"clone_ctransid", clonetransidlen, sizeof(uint64_t));

    {
        char* clonepath;
        ULONG clonepathlen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_PATH, (void**)&clonepath, &clonepathlen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"clone_path");

        clonepathu = utf8_to_utf16(string(clonepath, clonepathlen));
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_CLONE_OFFSET, (void**)&cloneoffset, &cloneoffsetlen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"clone_offset");

    if (cloneoffsetlen < sizeof(uint64_t))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"clone_offset", cloneoffsetlen, sizeof(uint64_t));

    for (i = 0; i < cache.size(); i++) {
        if (!memcmp(cloneuuid, &cache[i].uuid, sizeof(BTRFS_UUID)) && *clonetransid == cache[i].transid) {
            clonepar = cache[i].path;
            found = true;
            break;
        }
    }

    if (!found) {
        WCHAR volpathw[MAX_PATH];

        bfs.uuid = *cloneuuid;
        bfs.ctransid = *clonetransid;

        Status = NtFsControlFile(dir, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_FIND_SUBVOL, &bfs, sizeof(btrfs_find_subvol),
                                 cloneparw, sizeof(cloneparw));
        if (Status == STATUS_NOT_FOUND)
            throw string_error(IDS_RECV_CANT_FIND_CLONE_SUBVOL);
        else if (!NT_SUCCESS(Status))
            throw string_error(IDS_RECV_FIND_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());

        if (!GetVolumePathNameW(dirpath.c_str(), volpathw, (sizeof(volpathw) / sizeof(WCHAR)) - 1))
            throw string_error(IDS_RECV_GETVOLUMEPATHNAME_FAILED, GetLastError(), format_message(GetLastError()).c_str());

        clonepar = volpathw;
        if (clonepar.substr(clonepar.length() - 1) == L"\\")
            clonepar = clonepar.substr(0, clonepar.length() - 1);

        clonepar += cloneparw;
        clonepar += L"\\";

        add_cache_entry(cloneuuid, *clonetransid, clonepar);
    }

    {
        win_handle src = CreateFileW((clonepar + clonepathu).c_str(), FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS, nullptr);
        if (src == INVALID_HANDLE_VALUE)
            throw string_error(IDS_RECV_CANT_OPEN_FILE, funcname, (clonepar + clonepathu).c_str(), GetLastError(), format_message(GetLastError()).c_str());

        win_handle dest = CreateFileW((subvolpath + pathu).c_str(), FILE_WRITE_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS, nullptr);
        if (dest == INVALID_HANDLE_VALUE)
            throw string_error(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());

        if (!GetFileSizeEx(dest, &filesize))
            throw string_error(IDS_RECV_GETFILESIZEEX_FAILED, GetLastError(), format_message(GetLastError()).c_str());

        if ((uint64_t)filesize.QuadPart < *offset + *clonelen) {
            LARGE_INTEGER sizeli;

            sizeli.QuadPart = *offset + *clonelen;

            if (SetFilePointer(dest, sizeli.LowPart, &sizeli.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
                throw string_error(IDS_RECV_SETFILEPOINTER_FAILED, GetLastError(), format_message(GetLastError()).c_str());

            if (!SetEndOfFile(dest))
                throw string_error(IDS_RECV_SETENDOFFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        }

        ded.FileHandle = src;
        ded.SourceFileOffset.QuadPart = *cloneoffset;
        ded.TargetFileOffset.QuadPart = *offset;
        ded.ByteCount.QuadPart = *clonelen;

        Status = NtFsControlFile(dest, nullptr, nullptr, nullptr, &iosb, FSCTL_DUPLICATE_EXTENTS_TO_FILE, &ded, sizeof(DUPLICATE_EXTENTS_DATA),
                                 nullptr, 0);
        if (!NT_SUCCESS(Status))
            throw string_error(IDS_RECV_DUPLICATE_EXTENTS_FAILED, Status, format_ntstatus(Status).c_str());
    }
}

void BtrfsRecv::cmd_truncate(HWND hwnd, btrfs_send_command* cmd, uint8_t* data) {
    uint64_t* size;
    ULONG sizelen;
    wstring pathu;
    LARGE_INTEGER sizeli;
    DWORD att;

    {
        char* path;
        ULONG pathlen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        pathu = utf8_to_utf16(string(path, pathlen));
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_SIZE, (void**)&size, &sizelen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"size");

    if (sizelen < sizeof(uint64_t))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"size", sizelen, sizeof(uint64_t));

    att = GetFileAttributesW((subvolpath + pathu).c_str());
    if (att == INVALID_FILE_ATTRIBUTES)
        throw string_error(IDS_RECV_GETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());

    if (att & FILE_ATTRIBUTE_READONLY) {
        if (!SetFileAttributesW((subvolpath + pathu).c_str(), att & ~FILE_ATTRIBUTE_READONLY))
            throw string_error(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
    }

    {
        win_handle h = CreateFileW((subvolpath + pathu).c_str(), FILE_WRITE_DATA, 0, nullptr, OPEN_EXISTING,
                                   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, nullptr);

        if (h == INVALID_HANDLE_VALUE)
            throw string_error(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());

        sizeli.QuadPart = *size;

        if (SetFilePointer(h, sizeli.LowPart, &sizeli.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
            throw string_error(IDS_RECV_SETFILEPOINTER_FAILED, GetLastError(), format_message(GetLastError()).c_str());

        if (!SetEndOfFile(h))
            throw string_error(IDS_RECV_SETENDOFFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
    }

    if (att & FILE_ATTRIBUTE_READONLY) {
        if (!SetFileAttributesW((subvolpath + pathu).c_str(), att))
            throw string_error(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
    }
}

void BtrfsRecv::cmd_chmod(HWND hwnd, btrfs_send_command* cmd, uint8_t* data) {
    win_handle h;
    uint32_t* mode;
    ULONG modelen;
    wstring pathu;
    btrfs_set_inode_info bsii;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;

    {
        char* path;
        ULONG pathlen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        pathu = utf8_to_utf16(string(path, pathlen));
    }

    if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_MODE, (void**)&mode, &modelen))
        throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"mode");

    if (modelen < sizeof(uint32_t))
        throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"mode", modelen, sizeof(uint32_t));

    h = CreateFileW((subvolpath + pathu).c_str(), WRITE_DAC, 0, nullptr, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        throw string_error(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());

    memset(&bsii, 0, sizeof(btrfs_set_inode_info));

    bsii.mode_changed = true;
    bsii.st_mode = *mode;

    Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), nullptr, 0);
    if (!NT_SUCCESS(Status))
        throw string_error(IDS_RECV_SETINODEINFO_FAILED, Status, format_ntstatus(Status).c_str());
}

void BtrfsRecv::cmd_chown(HWND hwnd, btrfs_send_command* cmd, uint8_t* data) {
    win_handle h;
    uint32_t *uid, *gid;
    ULONG uidlen, gidlen;
    wstring pathu;
    btrfs_set_inode_info bsii;

    {
        char* path;
        ULONG pathlen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        pathu = utf8_to_utf16(string(path, pathlen));
    }

    h = CreateFileW((subvolpath + pathu).c_str(), FILE_WRITE_ATTRIBUTES | WRITE_OWNER | WRITE_DAC, 0, nullptr, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        throw string_error(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());

    memset(&bsii, 0, sizeof(btrfs_set_inode_info));

    if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_UID, (void**)&uid, &uidlen)) {
        if (uidlen < sizeof(uint32_t))
            throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"uid", uidlen, sizeof(uint32_t));

        bsii.uid_changed = true;
        bsii.st_uid = *uid;
    }

    if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_GID, (void**)&gid, &gidlen)) {
        if (gidlen < sizeof(uint32_t))
            throw string_error(IDS_RECV_SHORT_PARAM, funcname, L"gid", gidlen, sizeof(uint32_t));

        bsii.gid_changed = true;
        bsii.st_gid = *gid;
    }

    if (bsii.uid_changed || bsii.gid_changed) {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), nullptr, 0);
        if (!NT_SUCCESS(Status))
            throw string_error(IDS_RECV_SETINODEINFO_FAILED, Status, format_ntstatus(Status).c_str());
    }
}

static __inline uint64_t unix_time_to_win(BTRFS_TIME* t) {
    return (t->seconds * 10000000) + (t->nanoseconds / 100) + 116444736000000000;
}

void BtrfsRecv::cmd_utimes(HWND hwnd, btrfs_send_command* cmd, uint8_t* data) {
    wstring pathu;
    win_handle h;
    FILE_BASIC_INFO fbi;
    BTRFS_TIME* time;
    ULONG timelen;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;

    {
        char* path;
        ULONG pathlen;

        if (!find_tlv(data, cmd->length, BTRFS_SEND_TLV_PATH, (void**)&path, &pathlen))
            throw string_error(IDS_RECV_MISSING_PARAM, funcname, L"path");

        pathu = utf8_to_utf16(string(path, pathlen));
    }

    h = CreateFileW((subvolpath + pathu).c_str(), FILE_WRITE_ATTRIBUTES, 0, nullptr, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        throw string_error(IDS_RECV_CANT_OPEN_FILE, funcname, pathu.c_str(), GetLastError(), format_message(GetLastError()).c_str());

    memset(&fbi, 0, sizeof(FILE_BASIC_INFO));

    if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_OTIME, (void**)&time, &timelen) && timelen >= sizeof(BTRFS_TIME))
        fbi.CreationTime.QuadPart = unix_time_to_win(time);

    if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_ATIME, (void**)&time, &timelen) && timelen >= sizeof(BTRFS_TIME))
        fbi.LastAccessTime.QuadPart = unix_time_to_win(time);

    if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_MTIME, (void**)&time, &timelen) && timelen >= sizeof(BTRFS_TIME))
        fbi.LastWriteTime.QuadPart = unix_time_to_win(time);

    if (find_tlv(data, cmd->length, BTRFS_SEND_TLV_CTIME, (void**)&time, &timelen) && timelen >= sizeof(BTRFS_TIME))
        fbi.ChangeTime.QuadPart = unix_time_to_win(time);

    Status = NtSetInformationFile(h, &iosb, &fbi, sizeof(FILE_BASIC_INFO), FileBasicInformation);
    if (!NT_SUCCESS(Status))
        throw ntstatus_error(Status);
}

static void delete_directory(const wstring& dir) {
    WIN32_FIND_DATAW fff;

    fff_handle h = FindFirstFileW((dir + L"*").c_str(), &fff);

    if (h == INVALID_HANDLE_VALUE)
        return;

    do {
        wstring file;

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

    RemoveDirectoryW(dir.c_str());
}

static bool check_csum(btrfs_send_command* cmd, uint8_t* data) {
    uint32_t crc32 = cmd->csum, calc;

    cmd->csum = 0;

    calc = calc_crc32c(0, (uint8_t*)cmd, sizeof(btrfs_send_command));

    if (cmd->length > 0)
        calc = calc_crc32c(calc, data, cmd->length);

    return calc == crc32 ? true : false;
}

void BtrfsRecv::do_recv(const win_handle& f, uint64_t* pos, uint64_t size, const win_handle& parent) {
    try {
        btrfs_send_header header;
        bool ended = false;

        if (!ReadFile(f, &header, sizeof(btrfs_send_header), nullptr, nullptr))
            throw string_error(IDS_RECV_READFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());

        *pos += sizeof(btrfs_send_header);

        if (memcmp(header.magic, BTRFS_SEND_MAGIC, sizeof(header.magic)))
            throw string_error(IDS_RECV_NOT_A_SEND_STREAM);

        if (header.version > 1)
            throw string_error(IDS_RECV_UNSUPPORTED_VERSION, header.version);

        SendMessageW(GetDlgItem(hwnd, IDC_RECV_PROGRESS), PBM_SETRANGE32, 0, (LPARAM)65536);

        lastwritefile = INVALID_HANDLE_VALUE;
        lastwritepath = L"";
        lastwriteatt = 0;

        while (true) {
            btrfs_send_command cmd;
            uint8_t* data = nullptr;
            ULONG progress;

            if (cancelling)
                break;

            progress = (ULONG)((float)*pos * 65536.0f / (float)size);
            SendMessageW(GetDlgItem(hwnd, IDC_RECV_PROGRESS), PBM_SETPOS, progress, 0);

            if (!ReadFile(f, &cmd, sizeof(btrfs_send_command), nullptr, nullptr)) {
                if (GetLastError() != ERROR_HANDLE_EOF)
                    throw string_error(IDS_RECV_READFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());

                break;
            }

            *pos += sizeof(btrfs_send_command);

            if (cmd.length > 0) {
                if (*pos + cmd.length > size)
                    throw string_error(IDS_RECV_FILE_TRUNCATED);

                data = (uint8_t*)malloc(cmd.length);
                if (!data)
                    throw string_error(IDS_OUT_OF_MEMORY);
            }

            try {
                if (data) {
                    if (!ReadFile(f, data, cmd.length, nullptr, nullptr))
                        throw string_error(IDS_RECV_READFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());

                    *pos += cmd.length;
                }

                if (!check_csum(&cmd, data))
                    throw string_error(IDS_RECV_CSUM_ERROR);

                if (cmd.cmd == BTRFS_SEND_CMD_END) {
                    ended = true;
                    break;
                }

                if (lastwritefile != INVALID_HANDLE_VALUE && cmd.cmd != BTRFS_SEND_CMD_WRITE) {
                    if (lastwriteatt & FILE_ATTRIBUTE_READONLY) {
                        if (!SetFileAttributesW((subvolpath + lastwritepath).c_str(), lastwriteatt))
                            throw string_error(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                    }

                    CloseHandle(lastwritefile);

                    lastwritefile = INVALID_HANDLE_VALUE;
                    lastwritepath = L"";
                    lastwriteatt = 0;
                }

                switch (cmd.cmd) {
                    case BTRFS_SEND_CMD_SUBVOL:
                        cmd_subvol(hwnd, &cmd, data, parent);
                    break;

                    case BTRFS_SEND_CMD_SNAPSHOT:
                        cmd_snapshot(hwnd, &cmd, data, parent);
                    break;

                    case BTRFS_SEND_CMD_MKFILE:
                    case BTRFS_SEND_CMD_MKDIR:
                    case BTRFS_SEND_CMD_MKNOD:
                    case BTRFS_SEND_CMD_MKFIFO:
                    case BTRFS_SEND_CMD_MKSOCK:
                    case BTRFS_SEND_CMD_SYMLINK:
                        cmd_mkfile(hwnd, &cmd, data);
                    break;

                    case BTRFS_SEND_CMD_RENAME:
                        cmd_rename(hwnd, &cmd, data);
                    break;

                    case BTRFS_SEND_CMD_LINK:
                        cmd_link(hwnd, &cmd, data);
                    break;

                    case BTRFS_SEND_CMD_UNLINK:
                        cmd_unlink(hwnd, &cmd, data);
                    break;

                    case BTRFS_SEND_CMD_RMDIR:
                        cmd_rmdir(hwnd, &cmd, data);
                    break;

                    case BTRFS_SEND_CMD_SET_XATTR:
                        cmd_setxattr(hwnd, &cmd, data);
                    break;

                    case BTRFS_SEND_CMD_REMOVE_XATTR:
                        cmd_removexattr(hwnd, &cmd, data);
                    break;

                    case BTRFS_SEND_CMD_WRITE:
                        cmd_write(hwnd, &cmd, data);
                    break;

                    case BTRFS_SEND_CMD_CLONE:
                        cmd_clone(hwnd, &cmd, data);
                    break;

                    case BTRFS_SEND_CMD_TRUNCATE:
                        cmd_truncate(hwnd, &cmd, data);
                    break;

                    case BTRFS_SEND_CMD_CHMOD:
                        cmd_chmod(hwnd, &cmd, data);
                    break;

                    case BTRFS_SEND_CMD_CHOWN:
                        cmd_chown(hwnd, &cmd, data);
                    break;

                    case BTRFS_SEND_CMD_UTIMES:
                        cmd_utimes(hwnd, &cmd, data);
                    break;

                    case BTRFS_SEND_CMD_UPDATE_EXTENT:
                        // does nothing
                    break;

                    default:
                        throw string_error(IDS_RECV_UNKNOWN_COMMAND, cmd.cmd);
                }
            } catch (...) {
                if (data) free(data);
                throw;
            }

            if (data) free(data);
        }

        if (lastwritefile != INVALID_HANDLE_VALUE) {
            if (lastwriteatt & FILE_ATTRIBUTE_READONLY) {
                if (!SetFileAttributesW((subvolpath + lastwritepath).c_str(), lastwriteatt))
                    throw string_error(IDS_RECV_SETFILEATTRIBUTES_FAILED, GetLastError(), format_message(GetLastError()).c_str());
            }

            CloseHandle(lastwritefile);
        }

        if (!ended && !cancelling)
            throw string_error(IDS_RECV_FILE_TRUNCATED);

        if (!cancelling) {
            NTSTATUS Status;
            IO_STATUS_BLOCK iosb;
            btrfs_received_subvol brs;

            brs.generation = stransid;
            brs.uuid = subvol_uuid;

            Status = NtFsControlFile(dir, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_RECEIVED_SUBVOL, &brs, sizeof(btrfs_received_subvol),
                                    nullptr, 0);
            if (!NT_SUCCESS(Status))
                throw string_error(IDS_RECV_RECEIVED_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());
        }

        CloseHandle(dir);

        if (master != INVALID_HANDLE_VALUE)
            CloseHandle(master);
    } catch (...) {
        if (subvolpath != L"") {
            ULONG attrib;

            attrib = GetFileAttributesW(subvolpath.c_str());
            attrib &= ~FILE_ATTRIBUTE_READONLY;

            if (SetFileAttributesW(subvolpath.c_str(), attrib))
                delete_directory(subvolpath);
        }

        throw;
    }
}

DWORD BtrfsRecv::recv_thread() {
    LARGE_INTEGER size;
    uint64_t pos = 0;
    bool b = true;

    running = true;

    try {
        win_handle f = CreateFileW(streamfile.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (f == INVALID_HANDLE_VALUE)
            throw string_error(IDS_RECV_CANT_OPEN_FILE, funcname, streamfile.c_str(), GetLastError(), format_message(GetLastError()).c_str());

        if (!GetFileSizeEx(f, &size))
            throw string_error(IDS_RECV_GETFILESIZEEX_FAILED, GetLastError(), format_message(GetLastError()).c_str());

        {
            win_handle parent = CreateFileW(dirpath.c_str(), FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                            nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_POSIX_SEMANTICS, nullptr);
            if (parent == INVALID_HANDLE_VALUE)
                throw string_error(IDS_RECV_CANT_OPEN_PATH, dirpath.c_str(), GetLastError(), format_message(GetLastError()).c_str());

            do {
                do_recv(f, &pos, size.QuadPart, parent);
            } while (pos < (uint64_t)size.QuadPart);
        }
    } catch (const exception& e) {
        auto msg = utf8_to_utf16(e.what());

        SetDlgItemTextW(hwnd, IDC_RECV_MSG, msg.c_str());

        SendMessageW(GetDlgItem(hwnd, IDC_RECV_PROGRESS), PBM_SETSTATE, PBST_ERROR, 0);

        b = false;
    }

    if (b && hwnd) {
        wstring s;

        SendMessageW(GetDlgItem(hwnd, IDC_RECV_PROGRESS), PBM_SETPOS, 65536, 0);

        if (num_received == 1) {
            load_string(module, IDS_RECV_SUCCESS, s);
            SetDlgItemTextW(hwnd, IDC_RECV_MSG, s.c_str());
        } else {
            wstring t;

            load_string(module, IDS_RECV_SUCCESS_PLURAL, s);

            wstring_sprintf(t, s, num_received);

            SetDlgItemTextW(hwnd, IDC_RECV_MSG, t.c_str());
        }

        load_string(module, IDS_RECV_BUTTON_OK, s);

        SetDlgItemTextW(hwnd, IDCANCEL, s.c_str());
    }

    thread = nullptr;
    running = false;

    return 0;
}

static DWORD WINAPI global_recv_thread(LPVOID lpParameter) {
    BtrfsRecv* br = (BtrfsRecv*)lpParameter;

    return br->recv_thread();
}

INT_PTR CALLBACK BtrfsRecv::RecvProgressDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            try {
                this->hwnd = hwndDlg;
                thread = CreateThread(nullptr, 0, global_recv_thread, this, 0, nullptr);

                if (!thread)
                    throw string_error(IDS_RECV_CREATETHREAD_FAILED, GetLastError(), format_message(GetLastError()).c_str());
            } catch (const exception& e) {
                auto msg = utf8_to_utf16(e.what());

                SetDlgItemTextW(hwnd, IDC_RECV_MSG, msg.c_str());

                SendMessageW(GetDlgItem(hwnd, IDC_RECV_PROGRESS), PBM_SETSTATE, PBST_ERROR, 0);
            }
        break;

        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    switch (LOWORD(wParam)) {
                        case IDOK:
                        case IDCANCEL:
                            if (running) {
                                wstring s;

                                cancelling = true;

                                if (!load_string(module, IDS_RECV_CANCELLED, s))
                                    throw last_error(GetLastError());

                                SetDlgItemTextW(hwnd, IDC_RECV_MSG, s.c_str());
                                SendMessageW(GetDlgItem(hwnd, IDC_RECV_PROGRESS), PBM_SETPOS, 0, 0);

                                if (!load_string(module, IDS_RECV_BUTTON_OK, s))
                                    throw last_error(GetLastError());

                                SetDlgItemTextW(hwnd, IDCANCEL, s.c_str());
                            } else
                                EndDialog(hwndDlg, 1);

                            return true;
                    }
                break;
            }
        break;
    }

    return false;
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
        return false;
}

void BtrfsRecv::Open(HWND hwnd, const wstring& file, const wstring& path, bool quiet) {
#ifndef __REACTOS__
    uint32_t cpuInfo[4];
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
            throw last_error(GetLastError());
    }
}

#ifdef __REACTOS__
extern "C" {
#endif

void CALLBACK RecvSubvolGUIW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    try {
        OPENFILENAMEW ofn;
        WCHAR file[MAX_PATH];
        win_handle token;
        TOKEN_PRIVILEGES* tp;
        LUID luid;
        ULONG tplen;

        set_dpi_aware();

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            throw last_error(GetLastError());

        tplen = offsetof(TOKEN_PRIVILEGES, Privileges[0]) + (3 * sizeof(LUID_AND_ATTRIBUTES));
        tp = (TOKEN_PRIVILEGES*)malloc(tplen);
        if (!tp)
            throw string_error(IDS_OUT_OF_MEMORY);

        tp->PrivilegeCount = 3;

        if (!LookupPrivilegeValueW(nullptr, L"SeManageVolumePrivilege", &luid)) {
            free(tp);
            throw last_error(GetLastError());
        }

        tp->Privileges[0].Luid = luid;
        tp->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!LookupPrivilegeValueW(nullptr, L"SeSecurityPrivilege", &luid)) {
            free(tp);
            throw last_error(GetLastError());
        }

        tp->Privileges[1].Luid = luid;
        tp->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

        if (!LookupPrivilegeValueW(nullptr, L"SeRestorePrivilege", &luid)) {
            free(tp);
            throw last_error(GetLastError());
        }

        tp->Privileges[2].Luid = luid;
        tp->Privileges[2].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(token, false, tp, tplen, nullptr, nullptr)) {
            free(tp);
            throw last_error(GetLastError());
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
            BtrfsRecv recv;

            recv.Open(hwnd, file, lpszCmdLine, false);
        }

        free(tp);
    } catch (const exception& e) {
        error_message(hwnd, e.what());
    }
}

void CALLBACK RecvSubvolW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    try {
        vector<wstring> args;

        command_line_to_args(lpszCmdLine, args);

        if (args.size() >= 2) {
            win_handle token;
            TOKEN_PRIVILEGES* tp;
            ULONG tplen;
            LUID luid;

            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
                return;

            tplen = offsetof(TOKEN_PRIVILEGES, Privileges[0]) + (3 * sizeof(LUID_AND_ATTRIBUTES));
            tp = (TOKEN_PRIVILEGES*)malloc(tplen);
            if (!tp)
                return;

            tp->PrivilegeCount = 3;

            if (!LookupPrivilegeValueW(nullptr, L"SeManageVolumePrivilege", &luid)) {
                free(tp);
                return;
            }

            tp->Privileges[0].Luid = luid;
            tp->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            if (!LookupPrivilegeValueW(nullptr, L"SeSecurityPrivilege", &luid)) {
                free(tp);
                return;
            }

            tp->Privileges[1].Luid = luid;
            tp->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

            if (!LookupPrivilegeValueW(nullptr, L"SeRestorePrivilege", &luid)) {
                free(tp);
                return;
            }

            tp->Privileges[2].Luid = luid;
            tp->Privileges[2].Attributes = SE_PRIVILEGE_ENABLED;

            if (!AdjustTokenPrivileges(token, false, tp, tplen, nullptr, nullptr)) {
                free(tp);
                return;
            }

            free(tp);

            BtrfsRecv br;
            br.Open(nullptr, args[0], args[1], true);
        }
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
}

#ifdef __REACTOS__
} /* extern "C" */
#endif
