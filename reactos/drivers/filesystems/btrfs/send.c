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

#include "btrfs_drv.h"

typedef struct send_dir {
    LIST_ENTRY list_entry;
    UINT64 inode;
    BOOL dummy;
    BTRFS_TIME atime;
    BTRFS_TIME mtime;
    BTRFS_TIME ctime;
    struct send_dir* parent;
    UINT16 namelen;
    char* name;
    LIST_ENTRY deleted_children;
} send_dir;

typedef struct {
    LIST_ENTRY list_entry;
    UINT64 inode;
    BOOL dir;
    send_dir* sd;
    char tmpname[64];
} orphan;

typedef struct {
    LIST_ENTRY list_entry;
    ULONG namelen;
    char name[1];
} deleted_child;

typedef struct {
    LIST_ENTRY list_entry;
    send_dir* sd;
    UINT16 namelen;
    char name[1];
} ref;

typedef struct {
    send_dir* sd;
    UINT64 last_child_inode;
    LIST_ENTRY list_entry;
} pending_rmdir;

typedef struct {
    UINT64 offset;
    LIST_ENTRY list_entry;
    ULONG datalen;
    EXTENT_DATA data;
} send_ext;

typedef struct {
    device_extension* Vcb;
    root* root;
    root* parent;
    UINT8* data;
    ULONG datalen;
    ULONG num_clones;
    root** clones;
    LIST_ENTRY orphans;
    LIST_ENTRY dirs;
    LIST_ENTRY pending_rmdirs;
    KEVENT buffer_event;
    send_dir* root_dir;
    send_info* send;

    struct {
        UINT64 inode;
        BOOL deleting;
        BOOL new;
        UINT64 gen;
        UINT64 uid;
        UINT64 olduid;
        UINT64 gid;
        UINT64 oldgid;
        UINT64 mode;
        UINT64 oldmode;
        UINT64 size;
        UINT64 flags;
        BTRFS_TIME atime;
        BTRFS_TIME mtime;
        BTRFS_TIME ctime;
        BOOL file;
        char* path;
        orphan* o;
        send_dir* sd;
        LIST_ENTRY refs;
        LIST_ENTRY oldrefs;
        LIST_ENTRY exts;
        LIST_ENTRY oldexts;
    } lastinode;
} send_context;

#define MAX_SEND_WRITE 0xc000 // 48 KB
#define SEND_BUFFER_LENGTH 0x100000 // 1 MB

static NTSTATUS find_send_dir(send_context* context, UINT64 dir, UINT64 generation, send_dir** psd, BOOL* added_dummy);
static NTSTATUS wait_for_flush(send_context* context, traverse_ptr* tp1, traverse_ptr* tp2);

static void send_command(send_context* context, UINT16 cmd) {
    btrfs_send_command* bsc = (btrfs_send_command*)&context->data[context->datalen];

    bsc->cmd = cmd;
    bsc->csum = 0;

    context->datalen += sizeof(btrfs_send_command);
}

static void send_command_finish(send_context* context, ULONG pos) {
    btrfs_send_command* bsc = (btrfs_send_command*)&context->data[pos];

    bsc->length = context->datalen - pos - sizeof(btrfs_send_command);
    bsc->csum = calc_crc32c(0, (UINT8*)bsc, context->datalen - pos);
}

static void send_add_tlv(send_context* context, UINT16 type, void* data, UINT16 length) {
    btrfs_send_tlv* tlv = (btrfs_send_tlv*)&context->data[context->datalen];

    tlv->type = type;
    tlv->length = length;

    if (length > 0 && data)
        RtlCopyMemory(&tlv[1], data, length);

    context->datalen += sizeof(btrfs_send_tlv) + length;
}

static char* uint64_to_char(UINT64 num, char* buf) {
    char *tmp, tmp2[20];

    if (num == 0) {
        buf[0] = '0';
        return buf + 1;
    }

    tmp = &tmp2[20];
    while (num > 0) {
        tmp--;
        *tmp = (num % 10) + '0';
        num /= 10;
    }

    RtlCopyMemory(buf, tmp, tmp2 + sizeof(tmp2) - tmp);

    return &buf[tmp2 + sizeof(tmp2) - tmp];
}

static NTSTATUS get_orphan_name(send_context* context, UINT64 inode, UINT64 generation, char* name) {
    char *ptr, *ptr2;
    UINT64 index = 0;
    KEY searchkey;

    name[0] = 'o';

    ptr = uint64_to_char(inode, &name[1]);
    *ptr = '-'; ptr++;
    ptr = uint64_to_char(generation, ptr);
    *ptr = '-'; ptr++;
    ptr2 = ptr;

    searchkey.obj_id = SUBVOL_ROOT_INODE;
    searchkey.obj_type = TYPE_DIR_ITEM;

    do {
        NTSTATUS Status;
        traverse_ptr tp;

        ptr = uint64_to_char(index, ptr);
        *ptr = 0;

        searchkey.offset = calc_crc32c(0xfffffffe, (UINT8*)name, (ULONG)(ptr - name));

        Status = find_item(context->Vcb, context->root, &tp, &searchkey, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08x\n", Status);
            return Status;
        }

        if (!keycmp(searchkey, tp.item->key))
            goto cont;

        if (context->parent) {
            Status = find_item(context->Vcb, context->parent, &tp, &searchkey, FALSE, NULL);
            if (!NT_SUCCESS(Status)) {
                ERR("find_item returned %08x\n", Status);
                return Status;
            }

            if (!keycmp(searchkey, tp.item->key))
                goto cont;
        }

        return STATUS_SUCCESS;

cont:
        index++;
        ptr = ptr2;
    } while (TRUE);
}

static void add_orphan(send_context* context, orphan* o) {
    LIST_ENTRY* le;

    le = context->orphans.Flink;
    while (le != &context->orphans) {
        orphan* o2 = CONTAINING_RECORD(le, orphan, list_entry);

        if (o2->inode > o->inode) {
            InsertHeadList(o2->list_entry.Blink, &o->list_entry);
            return;
        }

        le = le->Flink;
    }

    InsertTailList(&context->orphans, &o->list_entry);
}

static NTSTATUS send_read_symlink(send_context* context, UINT64 inode, char** link, UINT16* linklen) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    EXTENT_DATA* ed;

    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = 0;

    Status = find_item(context->Vcb, context->root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    if (keycmp(tp.item->key, searchkey)) {
        ERR("could not find (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        return STATUS_INTERNAL_ERROR;
    }

    if (tp.item->size < sizeof(EXTENT_DATA)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
            tp.item->size, sizeof(EXTENT_DATA));
        return STATUS_INTERNAL_ERROR;
    }

    ed = (EXTENT_DATA*)tp.item->data;

    if (ed->type != EXTENT_TYPE_INLINE) {
        WARN("symlink data was not inline, returning blank string\n");
        *link = NULL;
        *linklen = 0;
        return STATUS_SUCCESS;
    }

    if (tp.item->size < offsetof(EXTENT_DATA, data[0]) + ed->decoded_size) {
        ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
            tp.item->size, offsetof(EXTENT_DATA, data[0]) + ed->decoded_size);
        return STATUS_INTERNAL_ERROR;
    }

    *link = (char*)ed->data;
    *linklen = (UINT16)ed->decoded_size;

    return STATUS_SUCCESS;
}

static NTSTATUS send_inode(send_context* context, traverse_ptr* tp, traverse_ptr* tp2) {
    NTSTATUS Status;
    INODE_ITEM* ii;

    if (tp2 && !tp) {
        INODE_ITEM* ii2 = (INODE_ITEM*)tp2->item->data;

        if (tp2->item->size < sizeof(INODE_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp2->item->key.obj_id, tp2->item->key.obj_type, tp2->item->key.offset,
                tp2->item->size, sizeof(INODE_ITEM));
            return STATUS_INTERNAL_ERROR;
        }

        context->lastinode.inode = tp2->item->key.obj_id;
        context->lastinode.deleting = TRUE;
        context->lastinode.gen = ii2->generation;
        context->lastinode.mode = ii2->st_mode;
        context->lastinode.flags = ii2->flags;
        context->lastinode.o = NULL;
        context->lastinode.sd = NULL;

        return STATUS_SUCCESS;
    }

    ii = (INODE_ITEM*)tp->item->data;

    if (tp->item->size < sizeof(INODE_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset,
            tp->item->size, sizeof(INODE_ITEM));
        return STATUS_INTERNAL_ERROR;
    }

    context->lastinode.inode = tp->item->key.obj_id;
    context->lastinode.deleting = FALSE;
    context->lastinode.gen = ii->generation;
    context->lastinode.uid = ii->st_uid;
    context->lastinode.gid = ii->st_gid;
    context->lastinode.mode = ii->st_mode;
    context->lastinode.size = ii->st_size;
    context->lastinode.atime = ii->st_atime;
    context->lastinode.mtime = ii->st_mtime;
    context->lastinode.ctime = ii->st_ctime;
    context->lastinode.flags = ii->flags;
    context->lastinode.file = FALSE;
    context->lastinode.o = NULL;
    context->lastinode.sd = NULL;

    if (context->lastinode.path) {
        ExFreePool(context->lastinode.path);
        context->lastinode.path = NULL;
    }

    if (tp2) {
        INODE_ITEM* ii2 = (INODE_ITEM*)tp2->item->data;
        LIST_ENTRY* le;

        if (tp2->item->size < sizeof(INODE_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp2->item->key.obj_id, tp2->item->key.obj_type, tp2->item->key.offset,
                tp2->item->size, sizeof(INODE_ITEM));
            return STATUS_INTERNAL_ERROR;
        }

        context->lastinode.oldmode = ii2->st_mode;
        context->lastinode.olduid = ii2->st_uid;
        context->lastinode.oldgid = ii2->st_gid;

        if ((ii2->st_mode & __S_IFREG) == __S_IFREG && (ii2->st_mode & __S_IFLNK) != __S_IFLNK && (ii2->st_mode & __S_IFSOCK) != __S_IFSOCK)
            context->lastinode.file = TRUE;

        context->lastinode.new = FALSE;

        le = context->orphans.Flink;
        while (le != &context->orphans) {
            orphan* o2 = CONTAINING_RECORD(le, orphan, list_entry);

            if (o2->inode == tp->item->key.obj_id) {
                context->lastinode.o = o2;
                break;
            } else if (o2->inode > tp->item->key.obj_id)
                break;

            le = le->Flink;
        }
    } else
        context->lastinode.new = TRUE;

    if (tp->item->key.obj_id == SUBVOL_ROOT_INODE) {
        send_dir* sd;

        Status = find_send_dir(context, tp->item->key.obj_id, ii->generation, &sd, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("find_send_dir returned %08x\n", Status);
            return Status;
        }

        sd->atime = ii->st_atime;
        sd->mtime = ii->st_mtime;
        sd->ctime = ii->st_ctime;
        context->root_dir = sd;
    } else if (!tp2) {
        ULONG pos = context->datalen;
        UINT16 cmd;
        send_dir* sd;

        char name[64];
        orphan* o;

        // skip creating orphan directory if we've already done so
        if (ii->st_mode & __S_IFDIR) {
            LIST_ENTRY* le;

            le = context->orphans.Flink;
            while (le != &context->orphans) {
                orphan* o2 = CONTAINING_RECORD(le, orphan, list_entry);

                if (o2->inode == tp->item->key.obj_id) {
                    context->lastinode.o = o2;
                    o2->sd->atime = ii->st_atime;
                    o2->sd->mtime = ii->st_mtime;
                    o2->sd->ctime = ii->st_ctime;
                    o2->sd->dummy = FALSE;
                    return STATUS_SUCCESS;
                } else if (o2->inode > tp->item->key.obj_id)
                    break;

                le = le->Flink;
            }
        }

        if ((ii->st_mode & __S_IFSOCK) == __S_IFSOCK)
            cmd = BTRFS_SEND_CMD_MKSOCK;
        else if ((ii->st_mode & __S_IFLNK) == __S_IFLNK)
            cmd = BTRFS_SEND_CMD_SYMLINK;
        else if ((ii->st_mode & __S_IFCHR) == __S_IFCHR || (ii->st_mode & __S_IFBLK) == __S_IFBLK)
            cmd = BTRFS_SEND_CMD_MKNOD;
        else if ((ii->st_mode & __S_IFDIR) == __S_IFDIR)
            cmd = BTRFS_SEND_CMD_MKDIR;
        else if ((ii->st_mode & __S_IFIFO) == __S_IFIFO)
            cmd = BTRFS_SEND_CMD_MKFIFO;
        else {
            cmd = BTRFS_SEND_CMD_MKFILE;
            context->lastinode.file = TRUE;
        }

        send_command(context, cmd);

        Status = get_orphan_name(context, tp->item->key.obj_id, ii->generation, name);
        if (!NT_SUCCESS(Status)) {
            ERR("get_orphan_name returned %08x\n", Status);
            return Status;
        }

        send_add_tlv(context, BTRFS_SEND_TLV_PATH, name, (UINT16)strlen(name));
        send_add_tlv(context, BTRFS_SEND_TLV_INODE, &tp->item->key.obj_id, sizeof(UINT64));

        if (cmd == BTRFS_SEND_CMD_MKNOD || cmd == BTRFS_SEND_CMD_MKFIFO || cmd == BTRFS_SEND_CMD_MKSOCK) {
            UINT64 rdev = makedev((ii->st_rdev & 0xFFFFFFFFFFF) >> 20, ii->st_rdev & 0xFFFFF), mode = ii->st_mode;

            send_add_tlv(context, BTRFS_SEND_TLV_RDEV, &rdev, sizeof(UINT64));
            send_add_tlv(context, BTRFS_SEND_TLV_MODE, &mode, sizeof(UINT64));
        } else if (cmd == BTRFS_SEND_CMD_SYMLINK && ii->st_size > 0) {
            char* link;
            UINT16 linklen;

            Status = send_read_symlink(context, tp->item->key.obj_id, &link, &linklen);
            if (!NT_SUCCESS(Status)) {
                ERR("send_read_symlink returned %08x\n", Status);
                return Status;
            }

            send_add_tlv(context, BTRFS_SEND_TLV_PATH_LINK, link, linklen);
        }

        send_command_finish(context, pos);

        if (ii->st_mode & __S_IFDIR) {
            Status = find_send_dir(context, tp->item->key.obj_id, ii->generation, &sd, NULL);
            if (!NT_SUCCESS(Status)) {
                ERR("find_send_dir returned %08x\n", Status);
                return Status;
            }

            sd->dummy = FALSE;
        } else
            sd = NULL;

        context->lastinode.sd = sd;

        o = ExAllocatePoolWithTag(PagedPool, sizeof(orphan), ALLOC_TAG);
        if (!o) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        o->inode = tp->item->key.obj_id;
        o->dir = (ii->st_mode & __S_IFDIR && ii->st_size > 0) ? TRUE : FALSE;
        strcpy(o->tmpname, name);
        o->sd = sd;
        add_orphan(context, o);

        context->lastinode.path = ExAllocatePoolWithTag(PagedPool, strlen(o->tmpname) + 1, ALLOC_TAG);
        if (!context->lastinode.path) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        strcpy(context->lastinode.path, o->tmpname);

        context->lastinode.o = o;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS send_add_dir(send_context* context, UINT64 inode, send_dir* parent, char* name, UINT16 namelen, BOOL dummy, LIST_ENTRY* lastentry, send_dir** psd) {
    LIST_ENTRY* le;
    send_dir* sd = ExAllocatePoolWithTag(PagedPool, sizeof(send_dir), ALLOC_TAG);

    if (!sd) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    sd->inode = inode;
    sd->dummy = dummy;
    sd->parent = parent;

    if (!dummy) {
        sd->atime = context->lastinode.atime;
        sd->mtime = context->lastinode.mtime;
        sd->ctime = context->lastinode.ctime;
    }

    if (namelen > 0) {
        sd->name = ExAllocatePoolWithTag(PagedPool, namelen, ALLOC_TAG);
        if (!sd->name) {
            ERR("out of memory\n");
            ExFreePool(sd);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        memcpy(sd->name, name, namelen);
    } else
        sd->name = NULL;

    sd->namelen = namelen;

    InitializeListHead(&sd->deleted_children);

    if (lastentry)
        InsertHeadList(lastentry, &sd->list_entry);
    else {
        le = context->dirs.Flink;
        while (le != &context->dirs) {
            send_dir* sd2 = CONTAINING_RECORD(le, send_dir, list_entry);

            if (sd2->inode > sd->inode) {
                InsertHeadList(sd2->list_entry.Blink, &sd->list_entry);

                if (psd)
                    *psd = sd;

                return STATUS_SUCCESS;
            }

            le = le->Flink;
        }

        InsertTailList(&context->dirs, &sd->list_entry);
    }

    if (psd)
        *psd = sd;

    return STATUS_SUCCESS;
}

static __inline UINT16 find_path_len(send_dir* parent, UINT16 namelen) {
    UINT16 len = namelen;

    while (parent && parent->namelen > 0) {
        len += parent->namelen + 1;
        parent = parent->parent;
    }

    return len;
}

static void find_path(char* path, send_dir* parent, char* name, ULONG namelen) {
    ULONG len = namelen;

    RtlCopyMemory(path, name, namelen);

    while (parent && parent->namelen > 0) {
        RtlMoveMemory(path + parent->namelen + 1, path, len);
        RtlCopyMemory(path, parent->name, parent->namelen);
        path[parent->namelen] = '/';
        len += parent->namelen + 1;

        parent = parent->parent;
    }
}

static void send_add_tlv_path(send_context* context, UINT16 type, send_dir* parent, char* name, UINT16 namelen) {
    UINT16 len = find_path_len(parent, namelen);

    send_add_tlv(context, type, NULL, len);

    if (len > 0)
        find_path((char*)&context->data[context->datalen - len], parent, name, namelen);
}

static NTSTATUS found_path(send_context* context, send_dir* parent, char* name, UINT16 namelen) {
    ULONG pos = context->datalen;

    if (context->lastinode.o) {
        send_command(context, BTRFS_SEND_CMD_RENAME);

        send_add_tlv_path(context, BTRFS_SEND_TLV_PATH, context->root_dir, context->lastinode.o->tmpname, (UINT16)strlen(context->lastinode.o->tmpname));

        send_add_tlv_path(context, BTRFS_SEND_TLV_PATH_TO, parent, name, namelen);

        send_command_finish(context, pos);
    } else {
        send_command(context, BTRFS_SEND_CMD_LINK);

        send_add_tlv_path(context, BTRFS_SEND_TLV_PATH, parent, name, namelen);

        send_add_tlv(context, BTRFS_SEND_TLV_PATH_LINK, context->lastinode.path, context->lastinode.path ? (UINT16)strlen(context->lastinode.path) : 0);

        send_command_finish(context, pos);
    }

    if (context->lastinode.o) {
        UINT16 pathlen;

        if (context->lastinode.o->sd) {
            if (context->lastinode.o->sd->name)
                ExFreePool(context->lastinode.o->sd->name);

            context->lastinode.o->sd->name = ExAllocatePoolWithTag(PagedPool, namelen, ALLOC_TAG);
            if (!context->lastinode.o->sd->name) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(context->lastinode.o->sd->name, name, namelen);
            context->lastinode.o->sd->namelen = namelen;
            context->lastinode.o->sd->parent = parent;
        }

        if (context->lastinode.path)
            ExFreePool(context->lastinode.path);

        pathlen = find_path_len(parent, namelen);
        context->lastinode.path = ExAllocatePoolWithTag(PagedPool, pathlen + 1, ALLOC_TAG);
        if (!context->lastinode.path) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        find_path(context->lastinode.path, parent, name, namelen);
        context->lastinode.path[pathlen] = 0;

        RemoveEntryList(&context->lastinode.o->list_entry);
        ExFreePool(context->lastinode.o);

        context->lastinode.o = NULL;
    }

    return STATUS_SUCCESS;
}

static void send_utimes_command_dir(send_context* context, send_dir* sd, BTRFS_TIME* atime, BTRFS_TIME* mtime, BTRFS_TIME* ctime) {
    ULONG pos = context->datalen;

    send_command(context, BTRFS_SEND_CMD_UTIMES);

    send_add_tlv_path(context, BTRFS_SEND_TLV_PATH, sd->parent, sd->name, sd->namelen);

    send_add_tlv(context, BTRFS_SEND_TLV_ATIME, atime, sizeof(BTRFS_TIME));
    send_add_tlv(context, BTRFS_SEND_TLV_MTIME, mtime, sizeof(BTRFS_TIME));
    send_add_tlv(context, BTRFS_SEND_TLV_CTIME, ctime, sizeof(BTRFS_TIME));

    send_command_finish(context, pos);
}

static NTSTATUS find_send_dir(send_context* context, UINT64 dir, UINT64 generation, send_dir** psd, BOOL* added_dummy) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    char name[64];

    le = context->dirs.Flink;
    while (le != &context->dirs) {
        send_dir* sd2 = CONTAINING_RECORD(le, send_dir, list_entry);

        if (sd2->inode > dir)
            break;
        else if (sd2->inode == dir) {
            *psd = sd2;

            if (added_dummy)
                *added_dummy = FALSE;

            return STATUS_SUCCESS;
        }

        le = le->Flink;
    }

    if (dir == SUBVOL_ROOT_INODE) {
        Status = send_add_dir(context, dir, NULL, NULL, 0, FALSE, le, psd);
        if (!NT_SUCCESS(Status)) {
            ERR("send_add_dir returned %08x\n", Status);
            return Status;
        }

        if (added_dummy)
            *added_dummy = FALSE;

        return STATUS_SUCCESS;
    }

    if (context->parent) {
        KEY searchkey;
        traverse_ptr tp;

        searchkey.obj_id = dir;
        searchkey.obj_type = TYPE_INODE_REF; // directories should never have an extiref
        searchkey.offset = 0xffffffffffffffff;

        Status = find_item(context->Vcb, context->parent, &tp, &searchkey, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08x\n", Status);
            return Status;
        }

        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            INODE_REF* ir = (INODE_REF*)tp.item->data;
            send_dir* parent;

            if (tp.item->size < sizeof(INODE_REF) || tp.item->size < offsetof(INODE_REF, name[0]) + ir->n) {
                ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                return STATUS_INTERNAL_ERROR;
            }

            if (tp.item->key.offset == SUBVOL_ROOT_INODE)
                parent = context->root_dir;
            else {
                Status = find_send_dir(context, tp.item->key.offset, generation, &parent, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("find_send_dir returned %08x\n", Status);
                    return Status;
                }
            }

            Status = send_add_dir(context, dir, parent, ir->name, ir->n, TRUE, NULL, psd);
            if (!NT_SUCCESS(Status)) {
                ERR("send_add_dir returned %08x\n", Status);
                return Status;
            }

            if (added_dummy)
                *added_dummy = FALSE;

            return STATUS_SUCCESS;
        }
    }

    Status = get_orphan_name(context, dir, generation, name);
    if (!NT_SUCCESS(Status)) {
        ERR("get_orphan_name returned %08x\n", Status);
        return Status;
    }

    Status = send_add_dir(context, dir, NULL, name, (UINT16)strlen(name), TRUE, le, psd);
    if (!NT_SUCCESS(Status)) {
        ERR("send_add_dir returned %08x\n", Status);
        return Status;
    }

    if (added_dummy)
        *added_dummy = TRUE;

    return STATUS_SUCCESS;
}

static NTSTATUS send_inode_ref(send_context* context, traverse_ptr* tp, BOOL tree2) {
    NTSTATUS Status;
    UINT64 inode = tp ? tp->item->key.obj_id : 0, dir = tp ? tp->item->key.offset : 0;
    LIST_ENTRY* le;
    INODE_REF* ir;
    UINT16 len;
    send_dir* sd = NULL;
    orphan* o2 = NULL;

    if (inode == dir) // root
        return STATUS_SUCCESS;

    if (tp->item->size < sizeof(INODE_REF)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset,
            tp->item->size, sizeof(INODE_REF));
        return STATUS_INTERNAL_ERROR;
    }

    if (dir != SUBVOL_ROOT_INODE) {
        BOOL added_dummy;

        Status = find_send_dir(context, dir, context->root->root_item.ctransid, &sd, &added_dummy);
        if (!NT_SUCCESS(Status)) {
            ERR("find_send_dir returned %08x\n", Status);
            return Status;
        }

        // directory has higher inode number than file, so might need to be created
        if (added_dummy) {
            BOOL found = FALSE;

            le = context->orphans.Flink;
            while (le != &context->orphans) {
                o2 = CONTAINING_RECORD(le, orphan, list_entry);

                if (o2->inode == dir) {
                    found = TRUE;
                    break;
                } else if (o2->inode > dir)
                    break;

                le = le->Flink;
            }

            if (!found) {
                ULONG pos = context->datalen;

                send_command(context, BTRFS_SEND_CMD_MKDIR);

                send_add_tlv_path(context, BTRFS_SEND_TLV_PATH, NULL, sd->name, sd->namelen);

                send_add_tlv(context, BTRFS_SEND_TLV_INODE, &dir, sizeof(UINT64));

                send_command_finish(context, pos);

                o2 = ExAllocatePoolWithTag(PagedPool, sizeof(orphan), ALLOC_TAG);
                if (!o2) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                o2->inode = dir;
                o2->dir = TRUE;
                memcpy(o2->tmpname, sd->name, sd->namelen);
                o2->tmpname[sd->namelen] = 0;
                o2->sd = sd;
                add_orphan(context, o2);
            }
        }
    } else
        sd = context->root_dir;

    len = tp->item->size;
    ir = (INODE_REF*)tp->item->data;

    while (len > 0) {
        ref* r;

        if (len < sizeof(INODE_REF) || len < offsetof(INODE_REF, name[0]) + ir->n) {
            ERR("(%llx,%x,%llx) was truncated\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset);
            return STATUS_INTERNAL_ERROR;
        }

        r = ExAllocatePoolWithTag(PagedPool, offsetof(ref, name[0]) + ir->n, ALLOC_TAG);
        if (!r) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        r->sd = sd;
        r->namelen = ir->n;
        RtlCopyMemory(r->name, ir->name, ir->n);

        InsertTailList(tree2 ? &context->lastinode.oldrefs : &context->lastinode.refs, &r->list_entry);

        len -= (UINT16)offsetof(INODE_REF, name[0]) + ir->n;
        ir = (INODE_REF*)&ir->name[ir->n];
    }

    return STATUS_SUCCESS;
}

static NTSTATUS send_inode_extref(send_context* context, traverse_ptr* tp, BOOL tree2) {
    INODE_EXTREF* ier;
    UINT16 len;

    if (tp->item->size < sizeof(INODE_EXTREF)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset,
            tp->item->size, sizeof(INODE_EXTREF));
        return STATUS_INTERNAL_ERROR;
    }

    len = tp->item->size;
    ier = (INODE_EXTREF*)tp->item->data;

    while (len > 0) {
        NTSTATUS Status;
        send_dir* sd = NULL;
        orphan* o2 = NULL;
        ref* r;

        if (len < sizeof(INODE_EXTREF) || len < offsetof(INODE_EXTREF, name[0]) + ier->n) {
            ERR("(%llx,%x,%llx) was truncated\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset);
            return STATUS_INTERNAL_ERROR;
        }

        if (ier->dir != SUBVOL_ROOT_INODE) {
            LIST_ENTRY* le;
            BOOL added_dummy;

            Status = find_send_dir(context, ier->dir, context->root->root_item.ctransid, &sd, &added_dummy);
            if (!NT_SUCCESS(Status)) {
                ERR("find_send_dir returned %08x\n", Status);
                return Status;
            }

            // directory has higher inode number than file, so might need to be created
            if (added_dummy) {
                BOOL found = FALSE;

                le = context->orphans.Flink;
                while (le != &context->orphans) {
                    o2 = CONTAINING_RECORD(le, orphan, list_entry);

                    if (o2->inode == ier->dir) {
                        found = TRUE;
                        break;
                    } else if (o2->inode > ier->dir)
                        break;

                    le = le->Flink;
                }

                if (!found) {
                    ULONG pos = context->datalen;

                    send_command(context, BTRFS_SEND_CMD_MKDIR);

                    send_add_tlv_path(context, BTRFS_SEND_TLV_PATH, NULL, sd->name, sd->namelen);
                    send_add_tlv(context, BTRFS_SEND_TLV_INODE, &ier->dir, sizeof(UINT64));

                    send_command_finish(context, pos);

                    o2 = ExAllocatePoolWithTag(PagedPool, sizeof(orphan), ALLOC_TAG);
                    if (!o2) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    o2->inode = ier->dir;
                    o2->dir = TRUE;
                    memcpy(o2->tmpname, sd->name, sd->namelen);
                    o2->tmpname[sd->namelen] = 0;
                    o2->sd = sd;
                    add_orphan(context, o2);
                }
            }
        } else
            sd = context->root_dir;

        r = ExAllocatePoolWithTag(PagedPool, offsetof(ref, name[0]) + ier->n, ALLOC_TAG);
        if (!r) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        r->sd = sd;
        r->namelen = ier->n;
        RtlCopyMemory(r->name, ier->name, ier->n);

        InsertTailList(tree2 ? &context->lastinode.oldrefs : &context->lastinode.refs, &r->list_entry);

        len -= (UINT16)offsetof(INODE_EXTREF, name[0]) + ier->n;
        ier = (INODE_EXTREF*)&ier->name[ier->n];
    }

    return STATUS_SUCCESS;
}

static void send_subvol_header(send_context* context, root* r, file_ref* fr) {
    ULONG pos = context->datalen;

    send_command(context, context->parent ? BTRFS_SEND_CMD_SNAPSHOT : BTRFS_SEND_CMD_SUBVOL);

    send_add_tlv(context, BTRFS_SEND_TLV_PATH, fr->dc->utf8.Buffer, fr->dc->utf8.Length);

    send_add_tlv(context, BTRFS_SEND_TLV_UUID, r->root_item.rtransid == 0 ? &r->root_item.uuid : &r->root_item.received_uuid, sizeof(BTRFS_UUID));
    send_add_tlv(context, BTRFS_SEND_TLV_TRANSID, &r->root_item.ctransid, sizeof(UINT64));

    if (context->parent) {
        send_add_tlv(context, BTRFS_SEND_TLV_CLONE_UUID,
                     context->parent->root_item.rtransid == 0 ? &context->parent->root_item.uuid : &context->parent->root_item.received_uuid, sizeof(BTRFS_UUID));
        send_add_tlv(context, BTRFS_SEND_TLV_CLONE_CTRANSID, &context->parent->root_item.ctransid, sizeof(UINT64));
    }

    send_command_finish(context, pos);
}

static void send_chown_command(send_context* context, char* path, UINT64 uid, UINT64 gid) {
    ULONG pos = context->datalen;

    send_command(context, BTRFS_SEND_CMD_CHOWN);

    send_add_tlv(context, BTRFS_SEND_TLV_PATH, path, path ? (UINT16)strlen(path) : 0);
    send_add_tlv(context, BTRFS_SEND_TLV_UID, &uid, sizeof(UINT64));
    send_add_tlv(context, BTRFS_SEND_TLV_GID, &gid, sizeof(UINT64));

    send_command_finish(context, pos);
}

static void send_chmod_command(send_context* context, char* path, UINT64 mode) {
    ULONG pos = context->datalen;

    send_command(context, BTRFS_SEND_CMD_CHMOD);

    mode &= 07777;

    send_add_tlv(context, BTRFS_SEND_TLV_PATH, path, path ? (UINT16)strlen(path) : 0);
    send_add_tlv(context, BTRFS_SEND_TLV_MODE, &mode, sizeof(UINT64));

    send_command_finish(context, pos);
}

static void send_utimes_command(send_context* context, char* path, BTRFS_TIME* atime, BTRFS_TIME* mtime, BTRFS_TIME* ctime) {
    ULONG pos = context->datalen;

    send_command(context, BTRFS_SEND_CMD_UTIMES);

    send_add_tlv(context, BTRFS_SEND_TLV_PATH, path, path ? (UINT16)strlen(path) : 0);
    send_add_tlv(context, BTRFS_SEND_TLV_ATIME, atime, sizeof(BTRFS_TIME));
    send_add_tlv(context, BTRFS_SEND_TLV_MTIME, mtime, sizeof(BTRFS_TIME));
    send_add_tlv(context, BTRFS_SEND_TLV_CTIME, ctime, sizeof(BTRFS_TIME));

    send_command_finish(context, pos);
}

static void send_truncate_command(send_context* context, char* path, UINT64 size) {
    ULONG pos = context->datalen;

    send_command(context, BTRFS_SEND_CMD_TRUNCATE);

    send_add_tlv(context, BTRFS_SEND_TLV_PATH, path, path ? (UINT16)strlen(path) : 0);
    send_add_tlv(context, BTRFS_SEND_TLV_SIZE, &size, sizeof(UINT64));

    send_command_finish(context, pos);
}

static NTSTATUS send_unlink_command(send_context* context, send_dir* parent, UINT16 namelen, char* name) {
    ULONG pos = context->datalen;
    UINT16 pathlen;

    send_command(context, BTRFS_SEND_CMD_UNLINK);

    pathlen = find_path_len(parent, namelen);
    send_add_tlv(context, BTRFS_SEND_TLV_PATH, NULL, pathlen);

    find_path((char*)&context->data[context->datalen - pathlen], parent, name, namelen);

    send_command_finish(context, pos);

    return STATUS_SUCCESS;
}

static void send_rmdir_command(send_context* context, UINT16 pathlen, char* path) {
    ULONG pos = context->datalen;

    send_command(context, BTRFS_SEND_CMD_RMDIR);
    send_add_tlv(context, BTRFS_SEND_TLV_PATH, path, pathlen);
    send_command_finish(context, pos);
}

static NTSTATUS get_dir_last_child(send_context* context, UINT64* last_inode) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;

    *last_inode = 0;

    searchkey.obj_id = context->lastinode.inode;
    searchkey.obj_type = TYPE_DIR_INDEX;
    searchkey.offset = 2;

    Status = find_item(context->Vcb, context->parent, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    do {
        traverse_ptr next_tp;

        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            DIR_ITEM* di = (DIR_ITEM*)tp.item->data;

            if (tp.item->size < sizeof(DIR_ITEM) || tp.item->size < offsetof(DIR_ITEM, name[0]) + di->m + di->n) {
                ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                return STATUS_INTERNAL_ERROR;
            }

            if (di->key.obj_type == TYPE_INODE_ITEM)
                *last_inode = max(*last_inode, di->key.obj_id);
        } else
            break;

        if (find_next_item(context->Vcb, &tp, &next_tp, FALSE, NULL))
            tp = next_tp;
        else
            break;
    } while (TRUE);

    return STATUS_SUCCESS;
}

static NTSTATUS add_pending_rmdir(send_context* context, UINT64 last_inode) {
    pending_rmdir* pr;
    LIST_ENTRY* le;

    pr = ExAllocatePoolWithTag(PagedPool, sizeof(pending_rmdir), ALLOC_TAG);
    if (!pr) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pr->sd = context->lastinode.sd;
    pr->last_child_inode = last_inode;

    le = context->pending_rmdirs.Flink;
    while (le != &context->pending_rmdirs) {
        pending_rmdir* pr2 = CONTAINING_RECORD(le, pending_rmdir, list_entry);

        if (pr2->last_child_inode > pr->last_child_inode) {
            InsertHeadList(pr2->list_entry.Blink, &pr->list_entry);
            return STATUS_SUCCESS;
        }

        le = le->Flink;
    }

    InsertTailList(&context->pending_rmdirs, &pr->list_entry);

    return STATUS_SUCCESS;
}

static NTSTATUS look_for_collision(send_context* context, send_dir* sd, char* name, ULONG namelen, UINT64* inode, BOOL* dir) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    DIR_ITEM* di;
    UINT16 len;

    searchkey.obj_id = sd->inode;
    searchkey.obj_type = TYPE_DIR_ITEM;
    searchkey.offset = calc_crc32c(0xfffffffe, (UINT8*)name, namelen);

    Status = find_item(context->Vcb, context->parent, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }

    if (keycmp(tp.item->key, searchkey))
        return STATUS_SUCCESS;

    di = (DIR_ITEM*)tp.item->data;
    len = tp.item->size;

    do {
        if (len < sizeof(DIR_ITEM) || len < offsetof(DIR_ITEM, name[0]) + di->m + di->n) {
            ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
            return STATUS_INTERNAL_ERROR;
        }

        if (di->n == namelen && RtlCompareMemory(di->name, name, namelen) == namelen) {
            *inode = di->key.obj_type == TYPE_INODE_ITEM ? di->key.obj_id : 0;
            *dir = di->type == BTRFS_TYPE_DIRECTORY ? TRUE: FALSE;
            return STATUS_OBJECT_NAME_COLLISION;
        }

        di = (DIR_ITEM*)&di->name[di->m + di->n];
        len -= (UINT16)offsetof(DIR_ITEM, name[0]) + di->m + di->n;
    } while (len > 0);

    return STATUS_SUCCESS;
}

static NTSTATUS make_file_orphan(send_context* context, UINT64 inode, BOOL dir, UINT64 generation, ref* r) {
    NTSTATUS Status;
    ULONG pos = context->datalen;
    send_dir* sd = NULL;
    orphan* o;
    LIST_ENTRY* le;
    char name[64];

    if (!dir) {
        deleted_child* dc;

        dc = ExAllocatePoolWithTag(PagedPool, offsetof(deleted_child, name[0]) + r->namelen, ALLOC_TAG);
        if (!dc) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        dc->namelen = r->namelen;
        RtlCopyMemory(dc->name, r->name, r->namelen);
        InsertTailList(&r->sd->deleted_children, &dc->list_entry);
    }

    le = context->orphans.Flink;
    while (le != &context->orphans) {
        orphan* o2 = CONTAINING_RECORD(le, orphan, list_entry);

        if (o2->inode == inode) {
            send_command(context, BTRFS_SEND_CMD_UNLINK);

            send_add_tlv_path(context, BTRFS_SEND_TLV_PATH, r->sd, r->name, r->namelen);

            send_command_finish(context, pos);

            return STATUS_SUCCESS;
        } else if (o2->inode > inode)
            break;

        le = le->Flink;
    }

    Status = get_orphan_name(context, inode, generation, name);
    if (!NT_SUCCESS(Status)) {
        ERR("get_orphan_name returned %08x\n", Status);
        return Status;
    }

    if (dir) {
        Status = find_send_dir(context, inode, generation, &sd, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("find_send_dir returned %08x\n", Status);
            return Status;
        }

        sd->dummy = TRUE;

        send_command(context, BTRFS_SEND_CMD_RENAME);

        send_add_tlv_path(context, BTRFS_SEND_TLV_PATH, r->sd, r->name, r->namelen);
        send_add_tlv_path(context, BTRFS_SEND_TLV_PATH_TO, context->root_dir, name, (UINT16)strlen(name));

        send_command_finish(context, pos);

        if (sd->name)
            ExFreePool(sd->name);

        sd->namelen = (UINT16)strlen(name);
        sd->name = ExAllocatePoolWithTag(PagedPool, sd->namelen, ALLOC_TAG);
        if (!sd->name) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(sd->name, name, sd->namelen);
        sd->parent = context->root_dir;
    } else {
        send_command(context, BTRFS_SEND_CMD_RENAME);

        send_add_tlv_path(context, BTRFS_SEND_TLV_PATH, r->sd, r->name, r->namelen);

        send_add_tlv_path(context, BTRFS_SEND_TLV_PATH_TO, context->root_dir, name, (UINT16)strlen(name));

        send_command_finish(context, pos);
    }

    o = ExAllocatePoolWithTag(PagedPool, sizeof(orphan), ALLOC_TAG);
    if (!o) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    o->inode = inode;
    o->dir = TRUE;
    strcpy(o->tmpname, name);
    o->sd = sd;
    add_orphan(context, o);

    return STATUS_SUCCESS;
}

static NTSTATUS flush_refs(send_context* context, traverse_ptr* tp1, traverse_ptr* tp2) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    ref *nameref = NULL, *nameref2 = NULL;

    if (context->lastinode.mode & __S_IFDIR) { // directory
        ref* r = IsListEmpty(&context->lastinode.refs) ? NULL : CONTAINING_RECORD(context->lastinode.refs.Flink, ref, list_entry);
        ref* or = IsListEmpty(&context->lastinode.oldrefs) ? NULL : CONTAINING_RECORD(context->lastinode.oldrefs.Flink, ref, list_entry);

        if (or && !context->lastinode.o) {
            ULONG len = find_path_len(or->sd, or->namelen);

            context->lastinode.path = ExAllocatePoolWithTag(PagedPool, len + 1, ALLOC_TAG);
            if (!context->lastinode.path) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            find_path(context->lastinode.path, or->sd, or->name, or->namelen);
            context->lastinode.path[len] = 0;

            if (!context->lastinode.sd) {
                Status = find_send_dir(context, context->lastinode.inode, context->lastinode.gen, &context->lastinode.sd, FALSE);
                if (!NT_SUCCESS(Status)) {
                    ERR("find_send_dir returned %08x\n", Status);
                    return Status;
                }
            }
        }

        if (r && or) {
            UINT64 inode;
            BOOL dir;

            Status = look_for_collision(context, r->sd, r->name, r->namelen, &inode, &dir);
            if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION) {
                ERR("look_for_collision returned %08x\n", Status);
                return Status;
            }

            if (Status == STATUS_OBJECT_NAME_COLLISION && inode > context->lastinode.inode) {
                Status = make_file_orphan(context, inode, dir, context->parent->root_item.ctransid, r);
                if (!NT_SUCCESS(Status)) {
                    ERR("make_file_orphan returned %08x\n", Status);
                    return Status;
                }
            }

            if (context->lastinode.o) {
                Status = found_path(context, r->sd, r->name, r->namelen);
                if (!NT_SUCCESS(Status)) {
                    ERR("found_path returned %08x\n", Status);
                    return Status;
                }

                if (!r->sd->dummy)
                    send_utimes_command_dir(context, r->sd, &r->sd->atime, &r->sd->mtime, &r->sd->ctime);
            } else if (r->sd != or->sd || r->namelen != or->namelen || RtlCompareMemory(r->name, or->name, r->namelen) != r->namelen) { // moved or renamed
                ULONG pos = context->datalen, len;

                send_command(context, BTRFS_SEND_CMD_RENAME);

                send_add_tlv(context, BTRFS_SEND_TLV_PATH, context->lastinode.path, (UINT16)strlen(context->lastinode.path));

                send_add_tlv_path(context, BTRFS_SEND_TLV_PATH_TO, r->sd, r->name, r->namelen);

                send_command_finish(context, pos);

                if (!r->sd->dummy)
                    send_utimes_command_dir(context, r->sd, &r->sd->atime, &r->sd->mtime, &r->sd->ctime);

                if (context->lastinode.sd->name)
                    ExFreePool(context->lastinode.sd->name);

                context->lastinode.sd->name = ExAllocatePoolWithTag(PagedPool, r->namelen, ALLOC_TAG);
                if (!context->lastinode.sd->name) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(context->lastinode.sd->name, r->name, r->namelen);
                context->lastinode.sd->parent = r->sd;

                if (context->lastinode.path)
                    ExFreePool(context->lastinode.path);

                len = find_path_len(r->sd, r->namelen);
                context->lastinode.path = ExAllocatePoolWithTag(PagedPool, len + 1, ALLOC_TAG);
                if (!context->lastinode.path) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                find_path(context->lastinode.path, r->sd, r->name, r->namelen);
                context->lastinode.path[len] = 0;
            }
        } else if (r && !or) { // new
            Status = found_path(context, r->sd, r->name, r->namelen);
            if (!NT_SUCCESS(Status)) {
                ERR("found_path returned %08x\n", Status);
                return Status;
            }

            if (!r->sd->dummy)
                send_utimes_command_dir(context, r->sd, &r->sd->atime, &r->sd->mtime, &r->sd->ctime);
        } else { // deleted
            UINT64 last_inode;

            Status = get_dir_last_child(context, &last_inode);
            if (!NT_SUCCESS(Status)) {
                ERR("get_dir_last_child returned %08x\n", Status);
                return Status;
            }

            if (last_inode <= context->lastinode.inode) {
                send_rmdir_command(context, (UINT16)strlen(context->lastinode.path), context->lastinode.path);

                if (!or->sd->dummy)
                    send_utimes_command_dir(context, or->sd, &or->sd->atime, &or->sd->mtime, &or->sd->ctime);
            } else {
                char name[64];
                ULONG pos = context->datalen;

                Status = get_orphan_name(context, context->lastinode.inode, context->lastinode.gen, name);
                if (!NT_SUCCESS(Status)) {
                    ERR("get_orphan_name returned %08x\n", Status);
                    return Status;
                }

                send_command(context, BTRFS_SEND_CMD_RENAME);
                send_add_tlv(context, BTRFS_SEND_TLV_PATH, context->lastinode.path, (UINT16)strlen(context->lastinode.path));
                send_add_tlv(context, BTRFS_SEND_TLV_PATH_TO, name, (UINT16)strlen(name));
                send_command_finish(context, pos);

                if (context->lastinode.sd->name)
                    ExFreePool(context->lastinode.sd->name);

                context->lastinode.sd->name = ExAllocatePoolWithTag(PagedPool, strlen(name), ALLOC_TAG);
                if (!context->lastinode.sd->name) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(context->lastinode.sd->name, name, strlen(name));
                context->lastinode.sd->namelen = (UINT16)strlen(name);
                context->lastinode.sd->dummy = TRUE;
                context->lastinode.sd->parent = NULL;

                send_utimes_command(context, NULL, &context->root_dir->atime, &context->root_dir->mtime, &context->root_dir->ctime);

                Status = add_pending_rmdir(context, last_inode);
                if (!NT_SUCCESS(Status)) {
                    ERR("add_pending_rmdir returned %08x\n", Status);
                    return Status;
                }
            }
        }

        while (!IsListEmpty(&context->lastinode.refs)) {
            r = CONTAINING_RECORD(RemoveHeadList(&context->lastinode.refs), ref, list_entry);
            ExFreePool(r);
        }

        while (!IsListEmpty(&context->lastinode.oldrefs)) {
            or = CONTAINING_RECORD(RemoveHeadList(&context->lastinode.oldrefs), ref, list_entry);
            ExFreePool(or);
        }

        return STATUS_SUCCESS;
    } else {
        if (!IsListEmpty(&context->lastinode.oldrefs)) {
            ref* or = CONTAINING_RECORD(context->lastinode.oldrefs.Flink, ref, list_entry);
            ULONG len = find_path_len(or->sd, or->namelen);

            context->lastinode.path = ExAllocatePoolWithTag(PagedPool, len + 1, ALLOC_TAG);
            if (!context->lastinode.path) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            find_path(context->lastinode.path, or->sd, or->name, or->namelen);
            context->lastinode.path[len] = 0;
            nameref = or;
        }

        // remove unchanged refs
        le = context->lastinode.oldrefs.Flink;
        while (le != &context->lastinode.oldrefs) {
            ref* or = CONTAINING_RECORD(le, ref, list_entry);
            LIST_ENTRY* le2;
            BOOL matched = FALSE;

            le2 = context->lastinode.refs.Flink;
            while (le2 != &context->lastinode.refs) {
                ref* r = CONTAINING_RECORD(le2, ref, list_entry);

                if (r->sd == or->sd && r->namelen == or->namelen && RtlCompareMemory(r->name, or->name, r->namelen) == r->namelen) {
                    RemoveEntryList(&r->list_entry);
                    ExFreePool(r);
                    matched = TRUE;
                    break;
                }

                le2 = le2->Flink;
            }

            if (matched) {
                le = le->Flink;
                RemoveEntryList(&or->list_entry);
                ExFreePool(or);
                continue;
            }

            le = le->Flink;
        }

        while (!IsListEmpty(&context->lastinode.refs)) {
            ref* r = CONTAINING_RECORD(RemoveHeadList(&context->lastinode.refs), ref, list_entry);
            UINT64 inode;
            BOOL dir;

            if (context->parent) {
                Status = look_for_collision(context, r->sd, r->name, r->namelen, &inode, &dir);
                if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION) {
                    ERR("look_for_collision returned %08x\n", Status);
                    return Status;
                }

                if (Status == STATUS_OBJECT_NAME_COLLISION && inode > context->lastinode.inode) {
                    Status = make_file_orphan(context, inode, dir, context->lastinode.gen, r);
                    if (!NT_SUCCESS(Status)) {
                        ERR("make_file_orphan returned %08x\n", Status);
                        return Status;
                    }
                }
            }

            if (context->datalen > SEND_BUFFER_LENGTH) {
                Status = wait_for_flush(context, tp1, tp2);
                if (!NT_SUCCESS(Status)) {
                    ERR("wait_for_flush returned %08x\n", Status);
                    return Status;
                }

                if (context->send->cancelling)
                    return STATUS_SUCCESS;
            }

            Status = found_path(context, r->sd, r->name, r->namelen);
            if (!NT_SUCCESS(Status)) {
                ERR("found_path returned %08x\n", Status);
                return Status;
            }

            if (!r->sd->dummy)
                send_utimes_command_dir(context, r->sd, &r->sd->atime, &r->sd->mtime, &r->sd->ctime);

            if (nameref && !nameref2)
                nameref2 = r;
            else
                ExFreePool(r);
        }

        while (!IsListEmpty(&context->lastinode.oldrefs)) {
            ref* or = CONTAINING_RECORD(RemoveHeadList(&context->lastinode.oldrefs), ref, list_entry);
            BOOL deleted = FALSE;

            le = or->sd->deleted_children.Flink;
            while (le != &or->sd->deleted_children) {
                deleted_child* dc = CONTAINING_RECORD(le, deleted_child, list_entry);

                if (dc->namelen == or->namelen && RtlCompareMemory(dc->name, or->name, or->namelen) == or->namelen) {
                    RemoveEntryList(&dc->list_entry);
                    ExFreePool(dc);
                    deleted = TRUE;
                    break;
                }

                le = le->Flink;
            }

            if (!deleted) {
                if (context->datalen > SEND_BUFFER_LENGTH) {
                    Status = wait_for_flush(context, tp1, tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("wait_for_flush returned %08x\n", Status);
                        return Status;
                    }

                    if (context->send->cancelling)
                        return STATUS_SUCCESS;
                }

                Status = send_unlink_command(context, or->sd, or->namelen, or->name);
                if (!NT_SUCCESS(Status)) {
                    ERR("send_unlink_command returned %08x\n", Status);
                    return Status;
                }

                if (!or->sd->dummy)
                    send_utimes_command_dir(context, or->sd, &or->sd->atime, &or->sd->mtime, &or->sd->ctime);
            }

            if (or == nameref && nameref2) {
                UINT16 len = find_path_len(nameref2->sd, nameref2->namelen);

                if (context->lastinode.path)
                    ExFreePool(context->lastinode.path);

                context->lastinode.path = ExAllocatePoolWithTag(PagedPool, len + 1, ALLOC_TAG);
                if (!context->lastinode.path) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                find_path(context->lastinode.path, nameref2->sd, nameref2->name, nameref2->namelen);
                context->lastinode.path[len] = 0;

                ExFreePool(nameref2);
            }

            ExFreePool(or);
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS wait_for_flush(send_context* context, traverse_ptr* tp1, traverse_ptr* tp2) {
    NTSTATUS Status;
    KEY key1, key2;

    if (tp1)
        key1 = tp1->item->key;

    if (tp2)
        key2 = tp2->item->key;

    ExReleaseResourceLite(&context->Vcb->tree_lock);

    KeClearEvent(&context->send->cleared_event);
    KeSetEvent(&context->buffer_event, 0, TRUE);
    KeWaitForSingleObject(&context->send->cleared_event, Executive, KernelMode, FALSE, NULL);

    ExAcquireResourceSharedLite(&context->Vcb->tree_lock, TRUE);

    if (context->send->cancelling)
        return STATUS_SUCCESS;

    if (tp1) {
        Status = find_item(context->Vcb, context->root, tp1, &key1, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08x\n", Status);
            return Status;
        }

        if (keycmp(tp1->item->key, key1)) {
            ERR("readonly subvolume changed\n");
            return STATUS_INTERNAL_ERROR;
        }
    }

    if (tp2) {
        Status = find_item(context->Vcb, context->parent, tp2, &key2, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08x\n", Status);
            return Status;
        }

        if (keycmp(tp2->item->key, key2)) {
            ERR("readonly subvolume changed\n");
            return STATUS_INTERNAL_ERROR;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS add_ext_holes(LIST_ENTRY* exts, UINT64 size) {
    UINT64 lastoff = 0;
    LIST_ENTRY* le;

    le = exts->Flink;
    while (le != exts) {
        send_ext* ext = CONTAINING_RECORD(le, send_ext, list_entry);

        if (ext->offset > lastoff) {
            send_ext* ext2 = ExAllocatePoolWithTag(PagedPool, offsetof(send_ext, data.data) + sizeof(EXTENT_DATA2), ALLOC_TAG);
            EXTENT_DATA2* ed2;

            if (!ext2) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ed2 = (EXTENT_DATA2*)ext2->data.data;

            ext2->offset = lastoff;
            ext2->datalen = offsetof(EXTENT_DATA, data) + sizeof(EXTENT_DATA2);
            ext2->data.decoded_size = ed2->num_bytes = ext->offset - lastoff;
            ext2->data.type = EXTENT_TYPE_REGULAR;
            ed2->address = ed2->size = ed2->offset = 0;

            InsertHeadList(le->Blink, &ext2->list_entry);
        }

        if (ext->data.type == EXTENT_TYPE_INLINE)
            lastoff = ext->offset + ext->data.decoded_size;
        else {
            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->data.data;
            lastoff = ext->offset + ed2->num_bytes;
        }

        le = le->Flink;
    }

    if (size > lastoff) {
        send_ext* ext2 = ExAllocatePoolWithTag(PagedPool, offsetof(send_ext, data.data) + sizeof(EXTENT_DATA2), ALLOC_TAG);
        EXTENT_DATA2* ed2;

        if (!ext2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ed2 = (EXTENT_DATA2*)ext2->data.data;

        ext2->offset = lastoff;
        ext2->datalen = offsetof(EXTENT_DATA, data) + sizeof(EXTENT_DATA2);
        ext2->data.decoded_size = ed2->num_bytes = size - lastoff;
        ext2->data.type = EXTENT_TYPE_REGULAR;
        ed2->address = ed2->size = ed2->offset = 0;

        InsertTailList(exts, &ext2->list_entry);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS divide_ext(send_ext* ext, UINT64 len, BOOL trunc) {
    send_ext* ext2;
    EXTENT_DATA2 *ed2a, *ed2b;

    if (ext->data.type == EXTENT_TYPE_INLINE) {
        if (!trunc) {
            ext2 = ExAllocatePoolWithTag(PagedPool, (ULONG)(offsetof(send_ext, data.data) + ext->data.decoded_size - len), ALLOC_TAG);

            if (!ext2) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            ext2->offset = ext->offset + len;
            ext2->datalen = (ULONG)(ext->data.decoded_size - len);
            ext2->data.decoded_size = ext->data.decoded_size - len;
            ext2->data.compression = ext->data.compression;
            ext2->data.encryption = ext->data.encryption;
            ext2->data.encoding = ext->data.encoding;
            ext2->data.type = ext->data.type;
            RtlCopyMemory(ext2->data.data, ext->data.data + len, (ULONG)(ext->data.decoded_size - len));

            InsertHeadList(&ext->list_entry, &ext2->list_entry);
        }

        ext->data.decoded_size = len;

        return STATUS_SUCCESS;
    }

    ed2a = (EXTENT_DATA2*)ext->data.data;

    if (!trunc) {
        ext2 = ExAllocatePoolWithTag(PagedPool, offsetof(send_ext, data.data) + sizeof(EXTENT_DATA2), ALLOC_TAG);

        if (!ext2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ed2b = (EXTENT_DATA2*)ext2->data.data;

        ext2->offset = ext->offset + len;
        ext2->datalen = offsetof(EXTENT_DATA, data) + sizeof(EXTENT_DATA2);

        ext2->data.compression = ext->data.compression;
        ext2->data.encryption = ext->data.encryption;
        ext2->data.encoding = ext->data.encoding;
        ext2->data.type = ext->data.type;
        ed2b->num_bytes = ed2a->num_bytes - len;

        if (ed2a->size == 0) {
            ext2->data.decoded_size = ed2b->num_bytes;
            ext->data.decoded_size = len;

            ed2b->address = ed2b->size = ed2b->offset = 0;
        } else {
            ext2->data.decoded_size = ext->data.decoded_size;

            ed2b->address = ed2a->address;
            ed2b->size = ed2a->size;
            ed2b->offset = ed2a->offset + len;
        }

        InsertHeadList(&ext->list_entry, &ext2->list_entry);
    }

    ed2a->num_bytes = len;

    return STATUS_SUCCESS;
}

static NTSTATUS sync_ext_cutoff_points(send_context* context) {
    NTSTATUS Status;
    send_ext *ext1, *ext2;

    ext1 = CONTAINING_RECORD(context->lastinode.exts.Flink, send_ext, list_entry);
    ext2 = CONTAINING_RECORD(context->lastinode.oldexts.Flink, send_ext, list_entry);

    do {
        UINT64 len1, len2;
        EXTENT_DATA2 *ed2a, *ed2b;

        ed2a = ext1->data.type == EXTENT_TYPE_INLINE ? NULL : (EXTENT_DATA2*)ext1->data.data;
        ed2b = ext2->data.type == EXTENT_TYPE_INLINE ? NULL : (EXTENT_DATA2*)ext2->data.data;

        len1 = ed2a ? ed2a->num_bytes : ext1->data.decoded_size;
        len2 = ed2b ? ed2b->num_bytes : ext2->data.decoded_size;

        if (len1 < len2) {
            Status = divide_ext(ext2, len1, FALSE);
            if (!NT_SUCCESS(Status)) {
                ERR("divide_ext returned %08x\n", Status);
                return Status;
            }
        } else if (len2 < len1) {
            Status = divide_ext(ext1, len2, FALSE);
            if (!NT_SUCCESS(Status)) {
                ERR("divide_ext returned %08x\n", Status);
                return Status;
            }
        }

        if (ext1->list_entry.Flink == &context->lastinode.exts || ext2->list_entry.Flink == &context->lastinode.oldexts)
            break;

        ext1 = CONTAINING_RECORD(ext1->list_entry.Flink, send_ext, list_entry);
        ext2 = CONTAINING_RECORD(ext2->list_entry.Flink, send_ext, list_entry);
    } while (TRUE);

    ext1 = CONTAINING_RECORD(context->lastinode.exts.Blink, send_ext, list_entry);
    ext2 = CONTAINING_RECORD(context->lastinode.oldexts.Blink, send_ext, list_entry);

    Status = divide_ext(ext1, context->lastinode.size - ext1->offset, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("divide_ext returned %08x\n", Status);
        return Status;
    }

    Status = divide_ext(ext2, context->lastinode.size - ext2->offset, TRUE);
    if (!NT_SUCCESS(Status)) {
        ERR("divide_ext returned %08x\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

static BOOL send_add_tlv_clone_path(send_context* context, root* r, UINT64 inode) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    UINT16 len = 0;
    UINT64 num;
    UINT8* ptr;

    num = inode;

    while (num != SUBVOL_ROOT_INODE) {
        searchkey.obj_id = num;
        searchkey.obj_type = TYPE_INODE_EXTREF;
        searchkey.offset = 0xffffffffffffffff;

        Status = find_item(context->Vcb, r, &tp, &searchkey, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08x\n", Status);
            return FALSE;
        }

        if (tp.item->key.obj_id != searchkey.obj_id || (tp.item->key.obj_type != TYPE_INODE_REF && tp.item->key.obj_type != TYPE_INODE_EXTREF)) {
            ERR("could not find INODE_REF for inode %llx\n", searchkey.obj_id);
            return FALSE;
        }

        if (len > 0)
            len++;

        if (tp.item->key.obj_type == TYPE_INODE_REF) {
            INODE_REF* ir = (INODE_REF*)tp.item->data;

            if (tp.item->size < sizeof(INODE_REF) || tp.item->size < offsetof(INODE_REF, name[0]) + ir->n) {
                ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                return FALSE;
            }

            len += ir->n;
            num = tp.item->key.offset;
        } else {
            INODE_EXTREF* ier = (INODE_EXTREF*)tp.item->data;

            if (tp.item->size < sizeof(INODE_EXTREF) || tp.item->size < offsetof(INODE_EXTREF, name[0]) + ier->n) {
                ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                return FALSE;
            }

            len += ier->n;
            num = ier->dir;
        }
    }

    send_add_tlv(context, BTRFS_SEND_TLV_CLONE_PATH, NULL, len);
    ptr = &context->data[context->datalen];

    num = inode;

    while (num != SUBVOL_ROOT_INODE) {
        searchkey.obj_id = num;
        searchkey.obj_type = TYPE_INODE_EXTREF;
        searchkey.offset = 0xffffffffffffffff;

        Status = find_item(context->Vcb, r, &tp, &searchkey, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08x\n", Status);
            return FALSE;
        }

        if (tp.item->key.obj_id != searchkey.obj_id || (tp.item->key.obj_type != TYPE_INODE_REF && tp.item->key.obj_type != TYPE_INODE_EXTREF)) {
            ERR("could not find INODE_REF for inode %llx\n", searchkey.obj_id);
            return FALSE;
        }

        if (num != inode) {
            ptr--;
            *ptr = '/';
        }

        if (tp.item->key.obj_type == TYPE_INODE_REF) {
            INODE_REF* ir = (INODE_REF*)tp.item->data;

            RtlCopyMemory(ptr - ir->n, ir->name, ir->n);
            ptr -= ir->n;
            num = tp.item->key.offset;
        } else {
            INODE_EXTREF* ier = (INODE_EXTREF*)tp.item->data;

            RtlCopyMemory(ptr - ier->n, ier->name, ier->n);
            ptr -= ier->n;
            num = ier->dir;
        }
    }

    return TRUE;
}

static BOOL try_clone_edr(send_context* context, send_ext* se, EXTENT_DATA_REF* edr) {
    NTSTATUS Status;
    root* r = NULL;
    KEY searchkey;
    traverse_ptr tp;
    EXTENT_DATA2* seed2 = (EXTENT_DATA2*)se->data.data;

    if (context->parent && edr->root == context->parent->id)
        r = context->parent;

    if (!r && context->num_clones > 0) {
        ULONG i;

        for (i = 0; i < context->num_clones; i++) {
            if (context->clones[i]->id == edr->root && context->clones[i] != context->root) {
                r = context->clones[i];
                break;
            }
        }
    }

    if (!r)
        return FALSE;

    searchkey.obj_id = edr->objid;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = 0;

    Status = find_item(context->Vcb, r, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return FALSE;
    }

    while (TRUE) {
        traverse_ptr next_tp;

        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            if (tp.item->size < sizeof(EXTENT_DATA))
                ERR("(%llx,%x,%llx) has size %u, not at least %u as expected\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
            else {
                EXTENT_DATA* ed = (EXTENT_DATA*)tp.item->data;

                if (ed->type == EXTENT_TYPE_REGULAR) {
                    if (tp.item->size < offsetof(EXTENT_DATA, data[0]) + sizeof(EXTENT_DATA2))
                        ERR("(%llx,%x,%llx) has size %u, not %u as expected\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset,
                            tp.item->size, offsetof(EXTENT_DATA, data[0]) + sizeof(EXTENT_DATA2));
                    else {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;

                        if (ed2->address == seed2->address && ed2->size == seed2->size && seed2->offset <= ed2->offset && seed2->offset + seed2->num_bytes >= ed2->offset + ed2->num_bytes) {
                            UINT64 clone_offset = tp.item->key.offset + ed2->offset - seed2->offset;
                            UINT64 clone_len = min(context->lastinode.size - se->offset, ed2->num_bytes);

                            if (clone_offset % context->Vcb->superblock.sector_size == 0 && clone_len % context->Vcb->superblock.sector_size == 0) {
                                ULONG pos = context->datalen;

                                send_command(context, BTRFS_SEND_CMD_CLONE);

                                send_add_tlv(context, BTRFS_SEND_TLV_OFFSET, &se->offset, sizeof(UINT64));
                                send_add_tlv(context, BTRFS_SEND_TLV_CLONE_LENGTH, &clone_len, sizeof(UINT64));
                                send_add_tlv(context, BTRFS_SEND_TLV_PATH, context->lastinode.path, context->lastinode.path ? (UINT16)strlen(context->lastinode.path) : 0);
                                send_add_tlv(context, BTRFS_SEND_TLV_CLONE_UUID, r->root_item.rtransid == 0 ? &r->root_item.uuid : &r->root_item.received_uuid, sizeof(BTRFS_UUID));
                                send_add_tlv(context, BTRFS_SEND_TLV_CLONE_CTRANSID, &r->root_item.ctransid, sizeof(UINT64));

                                if (!send_add_tlv_clone_path(context, r, tp.item->key.obj_id))
                                    context->datalen = pos;
                                else {
                                    send_add_tlv(context, BTRFS_SEND_TLV_CLONE_OFFSET, &clone_offset, sizeof(UINT64));

                                    send_command_finish(context, pos);

                                    return TRUE;
                                }
                            }
                        }
                    }
                }
            }
        } else if (tp.item->key.obj_id > searchkey.obj_id || (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type > searchkey.obj_type))
            break;

        if (find_next_item(context->Vcb, &tp, &next_tp, FALSE, NULL))
            tp = next_tp;
        else
            break;
    }

    return FALSE;
}

static BOOL try_clone(send_context* context, send_ext* se) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)se->data.data;
    EXTENT_ITEM* ei;
    UINT64 rc = 0;

    searchkey.obj_id = ed2->address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = ed2->size;

    Status = find_item(context->Vcb, context->Vcb->extent_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return FALSE;
    }

    if (keycmp(tp.item->key, searchkey)) {
        ERR("(%llx,%x,%llx) not found\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        return FALSE;
    }

    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return FALSE;
    }

    ei = (EXTENT_ITEM*)tp.item->data;

    if (tp.item->size > sizeof(EXTENT_ITEM)) {
        UINT32 len = tp.item->size - sizeof(EXTENT_ITEM);
        UINT8* ptr = (UINT8*)&ei[1];

        while (len > 0) {
            UINT8 secttype = *ptr;
            ULONG sectlen = get_extent_data_len(secttype);
            UINT64 sectcount = get_extent_data_refcount(secttype, ptr + sizeof(UINT8));

            len--;

            if (sectlen > len) {
                ERR("(%llx,%x,%llx): %x bytes left, expecting at least %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, len, sectlen);
                return FALSE;
            }

            if (sectlen == 0) {
                ERR("(%llx,%x,%llx): unrecognized extent type %x\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, secttype);
                return FALSE;
            }

            rc += sectcount;

            if (secttype == TYPE_EXTENT_DATA_REF) {
                EXTENT_DATA_REF* sectedr = (EXTENT_DATA_REF*)(ptr + sizeof(UINT8));

                if (try_clone_edr(context, se, sectedr))
                    return TRUE;
            }

            len -= sectlen;
            ptr += sizeof(UINT8) + sectlen;
        }
    }

    if (rc >= ei->refcount)
        return FALSE;

    searchkey.obj_type = TYPE_EXTENT_DATA_REF;
    searchkey.offset = 0;

    Status = find_item(context->Vcb, context->Vcb->extent_root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return FALSE;
    }

    while (TRUE) {
        traverse_ptr next_tp;

        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            if (tp.item->size < sizeof(EXTENT_DATA_REF))
                ERR("(%llx,%x,%llx) has size %u, not %u as expected\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA_REF));
            else {
                if (try_clone_edr(context, se, (EXTENT_DATA_REF*)tp.item->data))
                    return TRUE;
            }
        } else if (tp.item->key.obj_id > searchkey.obj_id || (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type > searchkey.obj_type))
            break;

        if (find_next_item(context->Vcb, &tp, &next_tp, FALSE, NULL))
            tp = next_tp;
        else
            break;
    }

    return FALSE;
}

static NTSTATUS flush_extents(send_context* context, traverse_ptr* tp1, traverse_ptr* tp2) {
    NTSTATUS Status;

    if ((IsListEmpty(&context->lastinode.exts) && IsListEmpty(&context->lastinode.oldexts)) || context->lastinode.size == 0)
        return STATUS_SUCCESS;

    if (context->parent) {
        Status = add_ext_holes(&context->lastinode.exts, context->lastinode.size);
        if (!NT_SUCCESS(Status)) {
            ERR("add_ext_holes returned %08x\n", Status);
            return Status;
        }

        Status = add_ext_holes(&context->lastinode.oldexts, context->lastinode.size);
        if (!NT_SUCCESS(Status)) {
            ERR("add_ext_holes returned %08x\n", Status);
            return Status;
        }

        Status = sync_ext_cutoff_points(context);
        if (!NT_SUCCESS(Status)) {
            ERR("sync_ext_cutoff_points returned %08x\n", Status);
            return Status;
        }
    }

    while (!IsListEmpty(&context->lastinode.exts)) {
        send_ext* se = CONTAINING_RECORD(RemoveHeadList(&context->lastinode.exts), send_ext, list_entry);
        send_ext* se2 = context->parent ? CONTAINING_RECORD(RemoveHeadList(&context->lastinode.oldexts), send_ext, list_entry) : NULL;
        ULONG pos;
        EXTENT_DATA2* ed2;

        if (se2) {
            if (se->data.type == EXTENT_TYPE_INLINE && se2->data.type == EXTENT_TYPE_INLINE &&
                RtlCompareMemory(se->data.data, se2->data.data, (ULONG)se->data.decoded_size) == (ULONG)se->data.decoded_size) {
                ExFreePool(se);
                ExFreePool(se2);
                continue;
            }

            if (se->data.type == EXTENT_TYPE_REGULAR && se2->data.type == EXTENT_TYPE_REGULAR) {
                EXTENT_DATA2 *ed2a, *ed2b;

                ed2a = (EXTENT_DATA2*)se->data.data;
                ed2b = (EXTENT_DATA2*)se2->data.data;

                if (RtlCompareMemory(ed2a, ed2b, sizeof(EXTENT_DATA2)) == sizeof(EXTENT_DATA2)) {
                    ExFreePool(se);
                    ExFreePool(se2);
                    continue;
                }
            }
        }

        if (se->data.type == EXTENT_TYPE_INLINE) {
            pos = context->datalen;

            send_command(context, BTRFS_SEND_CMD_WRITE);

            send_add_tlv(context, BTRFS_SEND_TLV_PATH, context->lastinode.path, context->lastinode.path ? (UINT16)strlen(context->lastinode.path) : 0);
            send_add_tlv(context, BTRFS_SEND_TLV_OFFSET, &se->offset, sizeof(UINT64));

            if (se->data.compression == BTRFS_COMPRESSION_NONE)
                send_add_tlv(context, BTRFS_SEND_TLV_DATA, se->data.data, (UINT16)se->data.decoded_size);
            else if (se->data.compression == BTRFS_COMPRESSION_ZLIB || se->data.compression == BTRFS_COMPRESSION_LZO) {
                ULONG inlen = se->datalen - (ULONG)offsetof(EXTENT_DATA, data[0]);

                send_add_tlv(context, BTRFS_SEND_TLV_DATA, NULL, (UINT16)se->data.decoded_size);
                RtlZeroMemory(&context->data[context->datalen - se->data.decoded_size], (ULONG)se->data.decoded_size);

                if (se->data.compression == BTRFS_COMPRESSION_ZLIB) {
                    Status = zlib_decompress(se->data.data, inlen, &context->data[context->datalen - se->data.decoded_size], (UINT32)se->data.decoded_size);
                    if (!NT_SUCCESS(Status)) {
                        ERR("zlib_decompress returned %08x\n", Status);
                        ExFreePool(se);
                        if (se2) ExFreePool(se2);
                        return Status;
                    }
                } else if (se->data.compression == BTRFS_COMPRESSION_LZO) {
                    if (inlen < sizeof(UINT32)) {
                        ERR("extent data was truncated\n");
                        ExFreePool(se);
                        if (se2) ExFreePool(se2);
                        return STATUS_INTERNAL_ERROR;
                    } else
                        inlen -= sizeof(UINT32);

                    Status = lzo_decompress(se->data.data + sizeof(UINT32), inlen, &context->data[context->datalen - se->data.decoded_size], (UINT32)se->data.decoded_size, sizeof(UINT32));
                    if (!NT_SUCCESS(Status)) {
                        ERR("lzo_decompress returned %08x\n", Status);
                        ExFreePool(se);
                        if (se2) ExFreePool(se2);
                        return Status;
                    }
                }
            } else {
                ERR("unhandled compression type %x\n", se->data.compression);
                ExFreePool(se);
                if (se2) ExFreePool(se2);
                return STATUS_NOT_IMPLEMENTED;
            }

            send_command_finish(context, pos);

            ExFreePool(se);
            if (se2) ExFreePool(se2);
            continue;
        }

        ed2 = (EXTENT_DATA2*)se->data.data;

        if (ed2->size != 0 && (context->parent || context->num_clones > 0)) {
            if (try_clone(context, se)) {
                ExFreePool(se);
                if (se2) ExFreePool(se2);
                continue;
            }
        }

        if (ed2->size == 0) { // write sparse
            UINT64 off, offset;

            for (off = ed2->offset; off < ed2->offset + ed2->num_bytes; off += MAX_SEND_WRITE) {
                UINT16 length = (UINT16)min(min(ed2->offset + ed2->num_bytes - off, MAX_SEND_WRITE), context->lastinode.size - se->offset - off);

                if (context->datalen > SEND_BUFFER_LENGTH) {
                    Status = wait_for_flush(context, tp1, tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("wait_for_flush returned %08x\n", Status);
                        ExFreePool(se);
                        if (se2) ExFreePool(se2);
                        return Status;
                    }

                    if (context->send->cancelling) {
                        ExFreePool(se);
                        if (se2) ExFreePool(se2);
                        return STATUS_SUCCESS;
                    }
                }

                pos = context->datalen;

                send_command(context, BTRFS_SEND_CMD_WRITE);

                send_add_tlv(context, BTRFS_SEND_TLV_PATH, context->lastinode.path, context->lastinode.path ? (UINT16)strlen(context->lastinode.path) : 0);

                offset = se->offset + off;
                send_add_tlv(context, BTRFS_SEND_TLV_OFFSET, &offset, sizeof(UINT64));

                send_add_tlv(context, BTRFS_SEND_TLV_DATA, NULL, length);
                RtlZeroMemory(&context->data[context->datalen - length], length);

                send_command_finish(context, pos);
            }
        } else if (se->data.compression == BTRFS_COMPRESSION_NONE) {
            UINT64 off, offset;
            UINT8* buf;

            buf = ExAllocatePoolWithTag(NonPagedPool, MAX_SEND_WRITE + (2 * context->Vcb->superblock.sector_size), ALLOC_TAG);
            if (!buf) {
                ERR("out of memory\n");
                ExFreePool(se);
                if (se2) ExFreePool(se2);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            for (off = ed2->offset; off < ed2->offset + ed2->num_bytes; off += MAX_SEND_WRITE) {
                UINT16 length = (UINT16)min(ed2->offset + ed2->num_bytes - off, MAX_SEND_WRITE);
                ULONG skip_start;
                UINT64 addr = ed2->address + off;
                UINT32* csum;

                if (context->datalen > SEND_BUFFER_LENGTH) {
                    Status = wait_for_flush(context, tp1, tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("wait_for_flush returned %08x\n", Status);
                        ExFreePool(buf);
                        ExFreePool(se);
                        if (se2) ExFreePool(se2);
                        return Status;
                    }

                    if (context->send->cancelling) {
                        ExFreePool(buf);
                        ExFreePool(se);
                        if (se2) ExFreePool(se2);
                        return STATUS_SUCCESS;
                    }
                }

                skip_start = addr % context->Vcb->superblock.sector_size;
                addr -= skip_start;

                if (context->lastinode.flags & BTRFS_INODE_NODATASUM)
                    csum = NULL;
                else {
                    UINT32 len;

                    len = (UINT32)sector_align(length + skip_start, context->Vcb->superblock.sector_size) / context->Vcb->superblock.sector_size;

                    csum = ExAllocatePoolWithTag(PagedPool, len * sizeof(UINT32), ALLOC_TAG);
                    if (!csum) {
                        ERR("out of memory\n");
                        ExFreePool(buf);
                        ExFreePool(se);
                        if (se2) ExFreePool(se2);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    Status = load_csum(context->Vcb, csum, addr, len, NULL);
                    if (!NT_SUCCESS(Status)) {
                        ERR("load_csum returned %08x\n", Status);
                        ExFreePool(csum);
                        ExFreePool(buf);
                        ExFreePool(se);
                        if (se2) ExFreePool(se2);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                Status = read_data(context->Vcb, addr, (UINT32)sector_align(length + skip_start, context->Vcb->superblock.sector_size),
                                   csum, FALSE, buf, NULL, NULL, NULL, 0, FALSE, NormalPagePriority);
                if (!NT_SUCCESS(Status)) {
                    ERR("read_data returned %08x\n", Status);
                    ExFreePool(buf);
                    ExFreePool(se);
                    if (se2) ExFreePool(se2);
                    if (csum) ExFreePool(csum);
                    return Status;
                }

                if (csum)
                    ExFreePool(csum);

                pos = context->datalen;

                send_command(context, BTRFS_SEND_CMD_WRITE);

                send_add_tlv(context, BTRFS_SEND_TLV_PATH, context->lastinode.path, context->lastinode.path ? (UINT16)strlen(context->lastinode.path) : 0);

                offset = se->offset + off;
                send_add_tlv(context, BTRFS_SEND_TLV_OFFSET, &offset, sizeof(UINT64));

                length = min((UINT16)(context->lastinode.size - se->offset - off), length);
                send_add_tlv(context, BTRFS_SEND_TLV_DATA, buf + skip_start, length);

                send_command_finish(context, pos);
            }

            ExFreePool(buf);
        } else {
            UINT8 *buf, *compbuf;
            UINT64 off;
            UINT32* csum;

            buf = ExAllocatePoolWithTag(PagedPool, (ULONG)se->data.decoded_size, ALLOC_TAG);
            if (!buf) {
                ERR("out of memory\n");
                ExFreePool(se);
                if (se2) ExFreePool(se2);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            compbuf = ExAllocatePoolWithTag(PagedPool, (ULONG)ed2->size, ALLOC_TAG);
            if (!compbuf) {
                ERR("out of memory\n");
                ExFreePool(buf);
                ExFreePool(se);
                if (se2) ExFreePool(se2);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            if (context->lastinode.flags & BTRFS_INODE_NODATASUM)
                csum = NULL;
            else {
                UINT32 len;

                len = (UINT32)(ed2->size / context->Vcb->superblock.sector_size);

                csum = ExAllocatePoolWithTag(PagedPool, len * sizeof(UINT32), ALLOC_TAG);
                if (!csum) {
                    ERR("out of memory\n");
                    ExFreePool(compbuf);
                    ExFreePool(buf);
                    ExFreePool(se);
                    if (se2) ExFreePool(se2);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                Status = load_csum(context->Vcb, csum, ed2->address, len, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("load_csum returned %08x\n", Status);
                    ExFreePool(csum);
                    ExFreePool(compbuf);
                    ExFreePool(buf);
                    ExFreePool(se);
                    if (se2) ExFreePool(se2);
                    return Status;
                }
            }

            Status = read_data(context->Vcb, ed2->address, (UINT32)ed2->size, csum, FALSE, compbuf, NULL, NULL, NULL, 0, FALSE, NormalPagePriority);
            if (!NT_SUCCESS(Status)) {
                ERR("read_data returned %08x\n", Status);
                ExFreePool(compbuf);
                ExFreePool(buf);
                ExFreePool(se);
                if (se2) ExFreePool(se2);
                if (csum) ExFreePool(csum);
                return Status;
            }

            if (csum)
                ExFreePool(csum);

            if (se->data.compression == BTRFS_COMPRESSION_ZLIB) {
                Status = zlib_decompress(compbuf, (UINT32)ed2->size, buf, (UINT32)se->data.decoded_size);
                if (!NT_SUCCESS(Status)) {
                    ERR("zlib_decompress returned %08x\n", Status);
                    ExFreePool(compbuf);
                    ExFreePool(buf);
                    ExFreePool(se);
                    if (se2) ExFreePool(se2);
                    return Status;
                }
            } else if (se->data.compression == BTRFS_COMPRESSION_LZO) {
                Status = lzo_decompress(&compbuf[sizeof(UINT32)], (UINT32)ed2->size, buf, (UINT32)se->data.decoded_size, sizeof(UINT32));
                if (!NT_SUCCESS(Status)) {
                    ERR("lzo_decompress returned %08x\n", Status);
                    ExFreePool(compbuf);
                    ExFreePool(buf);
                    ExFreePool(se);
                    if (se2) ExFreePool(se2);
                    return Status;
                }
            }

            ExFreePool(compbuf);

            for (off = ed2->offset; off < ed2->offset + ed2->num_bytes; off += MAX_SEND_WRITE) {
                UINT16 length = (UINT16)min(ed2->offset + ed2->num_bytes - off, MAX_SEND_WRITE);
                UINT64 offset;

                if (context->datalen > SEND_BUFFER_LENGTH) {
                    Status = wait_for_flush(context, tp1, tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("wait_for_flush returned %08x\n", Status);
                        ExFreePool(buf);
                        ExFreePool(se);
                        if (se2) ExFreePool(se2);
                        return Status;
                    }

                    if (context->send->cancelling) {
                        ExFreePool(buf);
                        ExFreePool(se);
                        if (se2) ExFreePool(se2);
                        return STATUS_SUCCESS;
                    }
                }

                pos = context->datalen;

                send_command(context, BTRFS_SEND_CMD_WRITE);

                send_add_tlv(context, BTRFS_SEND_TLV_PATH, context->lastinode.path, context->lastinode.path ? (UINT16)strlen(context->lastinode.path) : 0);

                offset = se->offset + off;
                send_add_tlv(context, BTRFS_SEND_TLV_OFFSET, &offset, sizeof(UINT64));

                length = min((UINT16)(context->lastinode.size - se->offset - off), length);
                send_add_tlv(context, BTRFS_SEND_TLV_DATA, &buf[off], length);

                send_command_finish(context, pos);
            }

            ExFreePool(buf);
        }

        ExFreePool(se);
        if (se2) ExFreePool(se2);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS finish_inode(send_context* context, traverse_ptr* tp1, traverse_ptr* tp2) {
    LIST_ENTRY* le;

    if (!IsListEmpty(&context->lastinode.refs) || !IsListEmpty(&context->lastinode.oldrefs)) {
        NTSTATUS Status = flush_refs(context, tp1, tp2);
        if (!NT_SUCCESS(Status)) {
            ERR("flush_refs returned %08x\n", Status);
            return Status;
        }

        if (context->send->cancelling)
            return STATUS_SUCCESS;
    }

    if (!context->lastinode.deleting) {
        if (context->lastinode.file) {
            NTSTATUS Status = flush_extents(context, tp1, tp2);
            if (!NT_SUCCESS(Status)) {
                ERR("flush_extents returned %08x\n", Status);
                return Status;
            }

            if (context->send->cancelling)
                return STATUS_SUCCESS;

            send_truncate_command(context, context->lastinode.path, context->lastinode.size);
        }

        if (context->lastinode.new || context->lastinode.uid != context->lastinode.olduid || context->lastinode.gid != context->lastinode.oldgid)
            send_chown_command(context, context->lastinode.path, context->lastinode.uid, context->lastinode.gid);

        if (((context->lastinode.mode & __S_IFLNK) != __S_IFLNK || ((context->lastinode.mode & 07777) != 0777)) &&
            (context->lastinode.new || context->lastinode.mode != context->lastinode.oldmode))
            send_chmod_command(context, context->lastinode.path, context->lastinode.mode);

        send_utimes_command(context, context->lastinode.path, &context->lastinode.atime, &context->lastinode.mtime, &context->lastinode.ctime);
    }

    while (!IsListEmpty(&context->lastinode.exts)) {
        ExFreePool(CONTAINING_RECORD(RemoveHeadList(&context->lastinode.exts), send_ext, list_entry));
    }

    while (!IsListEmpty(&context->lastinode.oldexts)) {
        ExFreePool(CONTAINING_RECORD(RemoveHeadList(&context->lastinode.oldexts), send_ext, list_entry));
    }

    if (context->parent) {
        le = context->pending_rmdirs.Flink;

        while (le != &context->pending_rmdirs) {
            pending_rmdir* pr = CONTAINING_RECORD(le, pending_rmdir, list_entry);

            if (pr->last_child_inode <= context->lastinode.inode) {
                le = le->Flink;

                send_rmdir_command(context, pr->sd->namelen, pr->sd->name);

                RemoveEntryList(&pr->sd->list_entry);

                if (pr->sd->name)
                    ExFreePool(pr->sd->name);

                while (!IsListEmpty(&pr->sd->deleted_children)) {
                    deleted_child* dc = CONTAINING_RECORD(RemoveHeadList(&pr->sd->deleted_children), deleted_child, list_entry);
                    ExFreePool(dc);
                }

                ExFreePool(pr->sd);

                RemoveEntryList(&pr->list_entry);
                ExFreePool(pr);
            } else
                break;
        }
    }

    context->lastinode.inode = 0;
    context->lastinode.o = NULL;

    if (context->lastinode.path) {
        ExFreePool(context->lastinode.path);
        context->lastinode.path = NULL;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS send_extent_data(send_context* context, traverse_ptr* tp, traverse_ptr* tp2) {
    NTSTATUS Status;

    if (tp && tp2 && tp->item->size == tp2->item->size && RtlCompareMemory(tp->item->data, tp2->item->data, tp->item->size) == tp->item->size)
        return STATUS_SUCCESS;

    if (!IsListEmpty(&context->lastinode.refs) || !IsListEmpty(&context->lastinode.oldrefs)) {
        Status = flush_refs(context, tp, tp2);
        if (!NT_SUCCESS(Status)) {
            ERR("flush_refs returned %08x\n", Status);
            return Status;
        }

        if (context->send->cancelling)
            return STATUS_SUCCESS;
    }

    if ((context->lastinode.mode & __S_IFLNK) == __S_IFLNK)
        return STATUS_SUCCESS;

    if (tp) {
        EXTENT_DATA* ed;
        EXTENT_DATA2* ed2 = NULL;

        if (tp->item->size < sizeof(EXTENT_DATA)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset,
                tp->item->size, sizeof(EXTENT_DATA));
            return STATUS_INTERNAL_ERROR;
        }

        ed = (EXTENT_DATA*)tp->item->data;

        if (ed->encryption != BTRFS_ENCRYPTION_NONE) {
            ERR("unknown encryption type %u\n", ed->encryption);
            return STATUS_INTERNAL_ERROR;
        }

        if (ed->encoding != BTRFS_ENCODING_NONE) {
            ERR("unknown encoding type %u\n", ed->encoding);
            return STATUS_INTERNAL_ERROR;
        }

        if (ed->compression != BTRFS_COMPRESSION_NONE && ed->compression != BTRFS_COMPRESSION_ZLIB && ed->compression != BTRFS_COMPRESSION_LZO) {
            ERR("unknown compression type %u\n", ed->compression);
            return STATUS_INTERNAL_ERROR;
        }

        if (ed->type == EXTENT_TYPE_REGULAR) {
            if (tp->item->size < offsetof(EXTENT_DATA, data[0]) + sizeof(EXTENT_DATA2)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset,
                    tp->item->size, offsetof(EXTENT_DATA, data[0]) + sizeof(EXTENT_DATA2));
                return STATUS_INTERNAL_ERROR;
            }

            ed2 = (EXTENT_DATA2*)ed->data;
        } else if (ed->type == EXTENT_TYPE_INLINE) {
            if (tp->item->size < offsetof(EXTENT_DATA, data[0]) + ed->decoded_size && ed->compression == BTRFS_COMPRESSION_NONE) {
                ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset,
                    tp->item->size, offsetof(EXTENT_DATA, data[0]) + ed->decoded_size);
                return STATUS_INTERNAL_ERROR;
            }
        }

        if ((ed->type == EXTENT_TYPE_INLINE || (ed->type == EXTENT_TYPE_REGULAR && ed2->size != 0)) && ed->decoded_size != 0) {
            send_ext* se = ExAllocatePoolWithTag(PagedPool, offsetof(send_ext, data) + tp->item->size, ALLOC_TAG);

            if (!se) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            se->offset = tp->item->key.offset;
            se->datalen = tp->item->size;
            RtlCopyMemory(&se->data, tp->item->data, tp->item->size);
            InsertTailList(&context->lastinode.exts, &se->list_entry);
        }
    }

    if (tp2) {
        EXTENT_DATA* ed;
        EXTENT_DATA2* ed2 = NULL;

        if (tp2->item->size < sizeof(EXTENT_DATA)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp2->item->key.obj_id, tp2->item->key.obj_type, tp2->item->key.offset,
                tp2->item->size, sizeof(EXTENT_DATA));
            return STATUS_INTERNAL_ERROR;
        }

        ed = (EXTENT_DATA*)tp2->item->data;

        if (ed->encryption != BTRFS_ENCRYPTION_NONE) {
            ERR("unknown encryption type %u\n", ed->encryption);
            return STATUS_INTERNAL_ERROR;
        }

        if (ed->encoding != BTRFS_ENCODING_NONE) {
            ERR("unknown encoding type %u\n", ed->encoding);
            return STATUS_INTERNAL_ERROR;
        }

        if (ed->compression != BTRFS_COMPRESSION_NONE && ed->compression != BTRFS_COMPRESSION_ZLIB && ed->compression != BTRFS_COMPRESSION_LZO) {
            ERR("unknown compression type %u\n", ed->compression);
            return STATUS_INTERNAL_ERROR;
        }

        if (ed->type == EXTENT_TYPE_REGULAR) {
            if (tp2->item->size < offsetof(EXTENT_DATA, data[0]) + sizeof(EXTENT_DATA2)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp2->item->key.obj_id, tp2->item->key.obj_type, tp2->item->key.offset,
                    tp2->item->size, offsetof(EXTENT_DATA, data[0]) + sizeof(EXTENT_DATA2));
                return STATUS_INTERNAL_ERROR;
            }

            ed2 = (EXTENT_DATA2*)ed->data;
        } else if (ed->type == EXTENT_TYPE_INLINE) {
            if (tp2->item->size < offsetof(EXTENT_DATA, data[0]) + ed->decoded_size) {
                ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp2->item->key.obj_id, tp2->item->key.obj_type, tp2->item->key.offset,
                    tp2->item->size, offsetof(EXTENT_DATA, data[0]) + ed->decoded_size);
                return STATUS_INTERNAL_ERROR;
            }
        }

        if ((ed->type == EXTENT_TYPE_INLINE || (ed->type == EXTENT_TYPE_REGULAR && ed2->size != 0)) && ed->decoded_size != 0) {
            send_ext* se = ExAllocatePoolWithTag(PagedPool, offsetof(send_ext, data) + tp2->item->size, ALLOC_TAG);

            if (!se) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            se->offset = tp2->item->key.offset;
            se->datalen = tp2->item->size;
            RtlCopyMemory(&se->data, tp2->item->data, tp2->item->size);
            InsertTailList(&context->lastinode.oldexts, &se->list_entry);
        }
    }

    return STATUS_SUCCESS;
}

typedef struct {
    UINT16 namelen;
    char* name;
    UINT16 value1len;
    char* value1;
    UINT16 value2len;
    char* value2;
    LIST_ENTRY list_entry;
} xattr_cmp;

static NTSTATUS send_xattr(send_context* context, traverse_ptr* tp, traverse_ptr* tp2) {
    if (tp && tp2 && tp->item->size == tp2->item->size && RtlCompareMemory(tp->item->data, tp2->item->data, tp->item->size) == tp->item->size)
        return STATUS_SUCCESS;

    if (!IsListEmpty(&context->lastinode.refs) || !IsListEmpty(&context->lastinode.oldrefs)) {
        NTSTATUS Status = flush_refs(context, tp, tp2);
        if (!NT_SUCCESS(Status)) {
            ERR("flush_refs returned %08x\n", Status);
            return Status;
        }

        if (context->send->cancelling)
            return STATUS_SUCCESS;
    }

    if (tp && tp->item->size < sizeof(DIR_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset,
            tp->item->size, sizeof(DIR_ITEM));
        return STATUS_INTERNAL_ERROR;
    }

    if (tp2 && tp2->item->size < sizeof(DIR_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp2->item->key.obj_id, tp2->item->key.obj_type, tp2->item->key.offset,
            tp2->item->size, sizeof(DIR_ITEM));
        return STATUS_INTERNAL_ERROR;
    }

    if (tp && !tp2) {
        ULONG len;
        DIR_ITEM* di;

        len = tp->item->size;
        di = (DIR_ITEM*)tp->item->data;

        do {
            ULONG pos;

            if (len < sizeof(DIR_ITEM) || len < offsetof(DIR_ITEM, name[0]) + di->m + di->n) {
                ERR("(%llx,%x,%llx) was truncated\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset);
                return STATUS_INTERNAL_ERROR;
            }

            pos = context->datalen;
            send_command(context, BTRFS_SEND_CMD_SET_XATTR);
            send_add_tlv(context, BTRFS_SEND_TLV_PATH, context->lastinode.path, context->lastinode.path ? (UINT16)strlen(context->lastinode.path) : 0);
            send_add_tlv(context, BTRFS_SEND_TLV_XATTR_NAME, di->name, di->n);
            send_add_tlv(context, BTRFS_SEND_TLV_XATTR_DATA, &di->name[di->n], di->m);
            send_command_finish(context, pos);

            len -= (ULONG)offsetof(DIR_ITEM, name[0]) + di->m + di->n;
            di = (DIR_ITEM*)&di->name[di->m + di->n];
        } while (len > 0);
    } else if (!tp && tp2) {
        ULONG len;
        DIR_ITEM* di;

        len = tp2->item->size;
        di = (DIR_ITEM*)tp2->item->data;

        do {
            ULONG pos;

            if (len < sizeof(DIR_ITEM) || len < offsetof(DIR_ITEM, name[0]) + di->m + di->n) {
                ERR("(%llx,%x,%llx) was truncated\n", tp2->item->key.obj_id, tp2->item->key.obj_type, tp2->item->key.offset);
                return STATUS_INTERNAL_ERROR;
            }

            pos = context->datalen;
            send_command(context, BTRFS_SEND_CMD_REMOVE_XATTR);
            send_add_tlv(context, BTRFS_SEND_TLV_PATH, context->lastinode.path, context->lastinode.path ? (UINT16)strlen(context->lastinode.path) : 0);
            send_add_tlv(context, BTRFS_SEND_TLV_XATTR_NAME, di->name, di->n);
            send_command_finish(context, pos);

            len -= (ULONG)offsetof(DIR_ITEM, name[0]) + di->m + di->n;
            di = (DIR_ITEM*)&di->name[di->m + di->n];
        } while (len > 0);
    } else {
        ULONG len;
        DIR_ITEM* di;
        LIST_ENTRY xattrs;

        InitializeListHead(&xattrs);

        len = tp->item->size;
        di = (DIR_ITEM*)tp->item->data;

        do {
            xattr_cmp* xa;

            if (len < sizeof(DIR_ITEM) || len < offsetof(DIR_ITEM, name[0]) + di->m + di->n) {
                ERR("(%llx,%x,%llx) was truncated\n", tp->item->key.obj_id, tp->item->key.obj_type, tp->item->key.offset);
                return STATUS_INTERNAL_ERROR;
            }

            xa = ExAllocatePoolWithTag(PagedPool, sizeof(xattr_cmp), ALLOC_TAG);
            if (!xa) {
                ERR("out of memory\n");

                while (!IsListEmpty(&xattrs)) {
                    ExFreePool(CONTAINING_RECORD(RemoveHeadList(&xattrs), xattr_cmp, list_entry));
                }

                return STATUS_INSUFFICIENT_RESOURCES;
            }

            xa->namelen = di->n;
            xa->name = di->name;
            xa->value1len = di->m;
            xa->value1 = di->name + di->n;
            xa->value2len = 0;
            xa->value2 = NULL;

            InsertTailList(&xattrs, &xa->list_entry);

            len -= (ULONG)offsetof(DIR_ITEM, name[0]) + di->m + di->n;
            di = (DIR_ITEM*)&di->name[di->m + di->n];
        } while (len > 0);

        len = tp2->item->size;
        di = (DIR_ITEM*)tp2->item->data;

        do {
            xattr_cmp* xa;
            LIST_ENTRY* le;
            BOOL found = FALSE;

            if (len < sizeof(DIR_ITEM) || len < offsetof(DIR_ITEM, name[0]) + di->m + di->n) {
                ERR("(%llx,%x,%llx) was truncated\n", tp2->item->key.obj_id, tp2->item->key.obj_type, tp2->item->key.offset);
                return STATUS_INTERNAL_ERROR;
            }

            le = xattrs.Flink;
            while (le != &xattrs) {
                xa = CONTAINING_RECORD(le, xattr_cmp, list_entry);

                if (xa->namelen == di->n && RtlCompareMemory(xa->name, di->name, di->n) == di->n) {
                    xa->value2len = di->m;
                    xa->value2 = di->name + di->n;
                    found = TRUE;
                    break;
                }

                le = le->Flink;
            }

            if (!found) {
                xa = ExAllocatePoolWithTag(PagedPool, sizeof(xattr_cmp), ALLOC_TAG);
                if (!xa) {
                    ERR("out of memory\n");

                    while (!IsListEmpty(&xattrs)) {
                        ExFreePool(CONTAINING_RECORD(RemoveHeadList(&xattrs), xattr_cmp, list_entry));
                    }

                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                xa->namelen = di->n;
                xa->name = di->name;
                xa->value1len = 0;
                xa->value1 = NULL;
                xa->value2len = di->m;
                xa->value2 = di->name + di->n;

                InsertTailList(&xattrs, &xa->list_entry);
            }

            len -= (ULONG)offsetof(DIR_ITEM, name[0]) + di->m + di->n;
            di = (DIR_ITEM*)&di->name[di->m + di->n];
        } while (len > 0);

        while (!IsListEmpty(&xattrs)) {
            xattr_cmp* xa = CONTAINING_RECORD(RemoveHeadList(&xattrs), xattr_cmp, list_entry);

            if (xa->value1len != xa->value2len || !xa->value1 || !xa->value2 || RtlCompareMemory(xa->value1, xa->value2, xa->value1len) != xa->value1len) {
                ULONG pos;

                if (!xa->value1) {
                    pos = context->datalen;
                    send_command(context, BTRFS_SEND_CMD_REMOVE_XATTR);
                    send_add_tlv(context, BTRFS_SEND_TLV_PATH, context->lastinode.path, context->lastinode.path ? (UINT16)strlen(context->lastinode.path) : 0);
                    send_add_tlv(context, BTRFS_SEND_TLV_XATTR_NAME, xa->name, xa->namelen);
                    send_command_finish(context, pos);
                } else {
                    pos = context->datalen;
                    send_command(context, BTRFS_SEND_CMD_SET_XATTR);
                    send_add_tlv(context, BTRFS_SEND_TLV_PATH, context->lastinode.path, context->lastinode.path ? (UINT16)strlen(context->lastinode.path) : 0);
                    send_add_tlv(context, BTRFS_SEND_TLV_XATTR_NAME, xa->name, xa->namelen);
                    send_add_tlv(context, BTRFS_SEND_TLV_XATTR_DATA, xa->value1, xa->value1len);
                    send_command_finish(context, pos);
                }
            }

            ExFreePool(xa);
        }
    }

    return STATUS_SUCCESS;
}

_Function_class_(KSTART_ROUTINE)
#ifdef __REACTOS__
static void NTAPI send_thread(void* ctx) {
#else
static void send_thread(void* ctx) {
#endif
    send_context* context = (send_context*)ctx;
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp, tp2;

    InterlockedIncrement(&context->root->send_ops);

    if (context->parent)
        InterlockedIncrement(&context->parent->send_ops);

    if (context->clones) {
        ULONG i;

        for (i = 0; i < context->num_clones; i++) {
            InterlockedIncrement(&context->clones[i]->send_ops);
        }
    }

    ExAcquireResourceExclusiveLite(&context->Vcb->tree_lock, TRUE);

    flush_subvol_fcbs(context->root);

    if (context->parent)
        flush_subvol_fcbs(context->parent);

    if (context->Vcb->need_write)
        Status = do_write(context->Vcb, NULL);
    else
        Status = STATUS_SUCCESS;

    free_trees(context->Vcb);

    if (!NT_SUCCESS(Status)) {
        ERR("do_write returned %08x\n", Status);
        ExReleaseResourceLite(&context->Vcb->tree_lock);
        goto end;
    }

    ExConvertExclusiveToSharedLite(&context->Vcb->tree_lock);

    searchkey.obj_id = searchkey.offset = 0;
    searchkey.obj_type = 0;

    Status = find_item(context->Vcb, context->root, &tp, &searchkey, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        ExReleaseResourceLite(&context->Vcb->tree_lock);
        goto end;
    }

    if (context->parent) {
        BOOL ended1 = FALSE, ended2 = FALSE;
        Status = find_item(context->Vcb, context->parent, &tp2, &searchkey, FALSE, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("find_item returned %08x\n", Status);
            ExReleaseResourceLite(&context->Vcb->tree_lock);
            goto end;
        }

        do {
            traverse_ptr next_tp;

            if (context->datalen > SEND_BUFFER_LENGTH) {
                KEY key1 = tp.item->key, key2 = tp2.item->key;

                ExReleaseResourceLite(&context->Vcb->tree_lock);

                KeClearEvent(&context->send->cleared_event);
                KeSetEvent(&context->buffer_event, 0, TRUE);
                KeWaitForSingleObject(&context->send->cleared_event, Executive, KernelMode, FALSE, NULL);

                if (context->send->cancelling)
                    goto end;

                ExAcquireResourceSharedLite(&context->Vcb->tree_lock, TRUE);

                if (!ended1) {
                    Status = find_item(context->Vcb, context->root, &tp, &key1, FALSE, NULL);
                    if (!NT_SUCCESS(Status)) {
                        ERR("find_item returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }

                    if (keycmp(tp.item->key, key1)) {
                        ERR("readonly subvolume changed\n");
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                }

                if (!ended2) {
                    Status = find_item(context->Vcb, context->parent, &tp2, &key2, FALSE, NULL);
                    if (!NT_SUCCESS(Status)) {
                        ERR("find_item returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }

                    if (keycmp(tp2.item->key, key2)) {
                        ERR("readonly subvolume changed\n");
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                }
            }

            while (!ended1 && !ended2 && tp.tree->header.address == tp2.tree->header.address) {
                Status = skip_to_difference(context->Vcb, &tp, &tp2, &ended1, &ended2);
                if (!NT_SUCCESS(Status)) {
                    ERR("skip_to_difference returned %08x\n", Status);
                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                    goto end;
                }
            }

            if (!ended1 && !ended2 && !keycmp(tp.item->key, tp2.item->key)) {
                BOOL no_next = FALSE, no_next2 = FALSE;

                TRACE("~ %llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);

                if (context->lastinode.inode != 0 && tp.item->key.obj_id > context->lastinode.inode) {
                    Status = finish_inode(context, ended1 ? NULL : &tp, ended2 ? NULL : &tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("finish_inode returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }

                    if (context->send->cancelling) {
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                }

                if (tp.item->key.obj_type == TYPE_INODE_ITEM) {
                    if (tp.item->size == tp2.item->size && tp.item->size > 0 && RtlCompareMemory(tp.item->data, tp2.item->data, tp.item->size) == tp.item->size) {
                        UINT64 inode = tp.item->key.obj_id;

                        while (TRUE) {
                            if (!find_next_item(context->Vcb, &tp, &next_tp, FALSE, NULL)) {
                                ended1 = TRUE;
                                break;
                            }

                            tp = next_tp;

                            if (tp.item->key.obj_id != inode)
                                break;
                        }

                        while (TRUE) {
                            if (!find_next_item(context->Vcb, &tp2, &next_tp, FALSE, NULL)) {
                                ended2 = TRUE;
                                break;
                            }

                            tp2 = next_tp;

                            if (tp2.item->key.obj_id != inode)
                                break;
                        }

                        no_next = TRUE;
                    } else if (tp.item->size > sizeof(UINT64) && tp2.item->size > sizeof(UINT64) && *(UINT64*)tp.item->data != *(UINT64*)tp2.item->data) {
                        UINT64 inode = tp.item->key.obj_id;

                        Status = send_inode(context, NULL, &tp2);
                        if (!NT_SUCCESS(Status)) {
                            ERR("send_inode returned %08x\n", Status);
                            ExReleaseResourceLite(&context->Vcb->tree_lock);
                            goto end;
                        }

                        while (TRUE) {
                            if (!find_next_item(context->Vcb, &tp2, &next_tp, FALSE, NULL)) {
                                ended2 = TRUE;
                                break;
                            }

                            tp2 = next_tp;

                            if (tp2.item->key.obj_id != inode)
                                break;

                            if (tp2.item->key.obj_type == TYPE_INODE_REF) {
                                Status = send_inode_ref(context, &tp2, TRUE);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("send_inode_ref returned %08x\n", Status);
                                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                                    goto end;
                                }
                            } else if (tp2.item->key.obj_type == TYPE_INODE_EXTREF) {
                                Status = send_inode_extref(context, &tp2, TRUE);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("send_inode_extref returned %08x\n", Status);
                                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                                    goto end;
                                }
                            }
                        }

                        Status = finish_inode(context, ended1 ? NULL : &tp, ended2 ? NULL : &tp2);
                        if (!NT_SUCCESS(Status)) {
                            ERR("finish_inode returned %08x\n", Status);
                            ExReleaseResourceLite(&context->Vcb->tree_lock);
                            goto end;
                        }

                        if (context->send->cancelling) {
                            ExReleaseResourceLite(&context->Vcb->tree_lock);
                            goto end;
                        }

                        no_next2 = TRUE;

                        Status = send_inode(context, &tp, NULL);
                        if (!NT_SUCCESS(Status)) {
                            ERR("send_inode returned %08x\n", Status);
                            ExReleaseResourceLite(&context->Vcb->tree_lock);
                            goto end;
                        }
                    } else {
                        Status = send_inode(context, &tp, &tp2);
                        if (!NT_SUCCESS(Status)) {
                            ERR("send_inode returned %08x\n", Status);
                            ExReleaseResourceLite(&context->Vcb->tree_lock);
                            goto end;
                        }
                    }
                } else if (tp.item->key.obj_type == TYPE_INODE_REF) {
                    Status = send_inode_ref(context, &tp, FALSE);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_inode_ref returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }

                    Status = send_inode_ref(context, &tp2, TRUE);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_inode_ref returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                } else if (tp.item->key.obj_type == TYPE_INODE_EXTREF) {
                    Status = send_inode_extref(context, &tp, FALSE);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_inode_extref returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }

                    Status = send_inode_extref(context, &tp2, TRUE);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_inode_extref returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                } else if (tp.item->key.obj_type == TYPE_EXTENT_DATA) {
                    Status = send_extent_data(context, &tp, &tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_extent_data returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }

                    if (context->send->cancelling) {
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                } else if (tp.item->key.obj_type == TYPE_XATTR_ITEM) {
                    Status = send_xattr(context, &tp, &tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_xattr returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }

                    if (context->send->cancelling) {
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                }

                if (!no_next) {
                    if (find_next_item(context->Vcb, &tp, &next_tp, FALSE, NULL))
                        tp = next_tp;
                    else
                        ended1 = TRUE;

                    if (!no_next2) {
                        if (find_next_item(context->Vcb, &tp2, &next_tp, FALSE, NULL))
                            tp2 = next_tp;
                        else
                            ended2 = TRUE;
                    }
                }
            } else if (ended2 || (!ended1 && !ended2 && keycmp(tp.item->key, tp2.item->key) == -1)) {
                TRACE("A %llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);

                if (context->lastinode.inode != 0 && tp.item->key.obj_id > context->lastinode.inode) {
                    Status = finish_inode(context, ended1 ? NULL : &tp, ended2 ? NULL : &tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("finish_inode returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }

                    if (context->send->cancelling) {
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                }

                if (tp.item->key.obj_type == TYPE_INODE_ITEM) {
                    Status = send_inode(context, &tp, NULL);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_inode returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                } else if (tp.item->key.obj_type == TYPE_INODE_REF) {
                    Status = send_inode_ref(context, &tp, FALSE);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_inode_ref returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                } else if (tp.item->key.obj_type == TYPE_INODE_EXTREF) {
                    Status = send_inode_extref(context, &tp, FALSE);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_inode_extref returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                } else if (tp.item->key.obj_type == TYPE_EXTENT_DATA) {
                    Status = send_extent_data(context, &tp, NULL);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_extent_data returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }

                    if (context->send->cancelling) {
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                } else if (tp.item->key.obj_type == TYPE_XATTR_ITEM) {
                    Status = send_xattr(context, &tp, NULL);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_xattr returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }

                    if (context->send->cancelling) {
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                }

                if (find_next_item(context->Vcb, &tp, &next_tp, FALSE, NULL))
                    tp = next_tp;
                else
                    ended1 = TRUE;
            } else if (ended1 || (!ended1 && !ended2 && keycmp(tp.item->key, tp2.item->key) == 1)) {
                TRACE("B %llx,%x,%llx\n", tp2.item->key.obj_id, tp2.item->key.obj_type, tp2.item->key.offset);

                if (context->lastinode.inode != 0 && tp2.item->key.obj_id > context->lastinode.inode) {
                    Status = finish_inode(context, ended1 ? NULL : &tp, ended2 ? NULL : &tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("finish_inode returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }

                    if (context->send->cancelling) {
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                }

                if (tp2.item->key.obj_type == TYPE_INODE_ITEM) {
                    Status = send_inode(context, NULL, &tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_inode returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                } else if (tp2.item->key.obj_type == TYPE_INODE_REF) {
                    Status = send_inode_ref(context, &tp2, TRUE);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_inode_ref returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                } else if (tp2.item->key.obj_type == TYPE_INODE_EXTREF) {
                    Status = send_inode_extref(context, &tp2, TRUE);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_inode_extref returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                } else if (tp2.item->key.obj_type == TYPE_EXTENT_DATA && !context->lastinode.deleting) {
                    Status = send_extent_data(context, NULL, &tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_extent_data returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }

                    if (context->send->cancelling) {
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                } else if (tp2.item->key.obj_type == TYPE_XATTR_ITEM && !context->lastinode.deleting) {
                    Status = send_xattr(context, NULL, &tp2);
                    if (!NT_SUCCESS(Status)) {
                        ERR("send_xattr returned %08x\n", Status);
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }

                    if (context->send->cancelling) {
                        ExReleaseResourceLite(&context->Vcb->tree_lock);
                        goto end;
                    }
                }

                if (find_next_item(context->Vcb, &tp2, &next_tp, FALSE, NULL))
                    tp2 = next_tp;
                else
                    ended2 = TRUE;
            }
        } while (!ended1 || !ended2);
    } else {
        do {
            traverse_ptr next_tp;

            if (context->datalen > SEND_BUFFER_LENGTH) {
                KEY key = tp.item->key;

                ExReleaseResourceLite(&context->Vcb->tree_lock);

                KeClearEvent(&context->send->cleared_event);
                KeSetEvent(&context->buffer_event, 0, TRUE);
                KeWaitForSingleObject(&context->send->cleared_event, Executive, KernelMode, FALSE, NULL);

                if (context->send->cancelling)
                    goto end;

                ExAcquireResourceSharedLite(&context->Vcb->tree_lock, TRUE);

                Status = find_item(context->Vcb, context->root, &tp, &key, FALSE, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("find_item returned %08x\n", Status);
                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                    goto end;
                }

                if (keycmp(tp.item->key, key)) {
                    ERR("readonly subvolume changed\n");
                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                    Status = STATUS_INTERNAL_ERROR;
                    goto end;
                }
            }

            if (context->lastinode.inode != 0 && tp.item->key.obj_id > context->lastinode.inode) {
                Status = finish_inode(context, &tp, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("finish_inode returned %08x\n", Status);
                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                    goto end;
                }

                if (context->send->cancelling) {
                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                    goto end;
                }
            }

            if (tp.item->key.obj_type == TYPE_INODE_ITEM) {
                Status = send_inode(context, &tp, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("send_inode returned %08x\n", Status);
                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                    goto end;
                }
            } else if (tp.item->key.obj_type == TYPE_INODE_REF) {
                Status = send_inode_ref(context, &tp, FALSE);
                if (!NT_SUCCESS(Status)) {
                    ERR("send_inode_ref returned %08x\n", Status);
                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                    goto end;
                }
            } else if (tp.item->key.obj_type == TYPE_INODE_EXTREF) {
                Status = send_inode_extref(context, &tp, FALSE);
                if (!NT_SUCCESS(Status)) {
                    ERR("send_inode_extref returned %08x\n", Status);
                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                    goto end;
                }
            } else if (tp.item->key.obj_type == TYPE_EXTENT_DATA) {
                Status = send_extent_data(context, &tp, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("send_extent_data returned %08x\n", Status);
                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                    goto end;
                }

                if (context->send->cancelling) {
                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                    goto end;
                }
            } else if (tp.item->key.obj_type == TYPE_XATTR_ITEM) {
                Status = send_xattr(context, &tp, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("send_xattr returned %08x\n", Status);
                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                    goto end;
                }

                if (context->send->cancelling) {
                    ExReleaseResourceLite(&context->Vcb->tree_lock);
                    goto end;
                }
            }

            if (find_next_item(context->Vcb, &tp, &next_tp, FALSE, NULL))
                tp = next_tp;
            else
                break;
        } while (TRUE);
    }

    if (context->lastinode.inode != 0) {
        Status = finish_inode(context, NULL, NULL);
        if (!NT_SUCCESS(Status)) {
            ERR("finish_inode returned %08x\n", Status);
            ExReleaseResourceLite(&context->Vcb->tree_lock);
            goto end;
        }

        ExReleaseResourceLite(&context->Vcb->tree_lock);

        if (context->send->cancelling)
            goto end;
    } else
        ExReleaseResourceLite(&context->Vcb->tree_lock);

    KeClearEvent(&context->send->cleared_event);
    KeSetEvent(&context->buffer_event, 0, TRUE);
    KeWaitForSingleObject(&context->send->cleared_event, Executive, KernelMode, FALSE, NULL);

    Status = STATUS_SUCCESS;

end:
    if (!NT_SUCCESS(Status)) {
        KeSetEvent(&context->buffer_event, 0, FALSE);

        if (context->send->ccb)
            context->send->ccb->send_status = Status;
    }

    ExAcquireResourceExclusiveLite(&context->Vcb->send_load_lock, TRUE);

    while (!IsListEmpty(&context->orphans)) {
        orphan* o = CONTAINING_RECORD(RemoveHeadList(&context->orphans), orphan, list_entry);
        ExFreePool(o);
    }

    while (!IsListEmpty(&context->dirs)) {
        send_dir* sd = CONTAINING_RECORD(RemoveHeadList(&context->dirs), send_dir, list_entry);

        if (sd->name)
            ExFreePool(sd->name);

        while (!IsListEmpty(&sd->deleted_children)) {
            deleted_child* dc = CONTAINING_RECORD(RemoveHeadList(&sd->deleted_children), deleted_child, list_entry);
            ExFreePool(dc);
        }

        ExFreePool(sd);
    }

    ZwClose(context->send->thread);
    context->send->thread = NULL;

    if (context->send->ccb)
        context->send->ccb->send = NULL;

    RemoveEntryList(&context->send->list_entry);
    ExFreePool(context->send);
    ExFreePool(context->data);

    InterlockedDecrement(&context->Vcb->running_sends);
    InterlockedDecrement(&context->root->send_ops);

    if (context->parent)
        InterlockedDecrement(&context->parent->send_ops);

    ExReleaseResourceLite(&context->Vcb->send_load_lock);

    if (context->clones) {
        ULONG i;

        for (i = 0; i < context->num_clones; i++) {
            InterlockedDecrement(&context->clones[i]->send_ops);
        }

        ExFreePool(context->clones);
    }

    ExFreePool(context);

    PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS send_subvol(device_extension* Vcb, void* data, ULONG datalen, PFILE_OBJECT FileObject, PIRP Irp) {
    NTSTATUS Status;
    fcb* fcb;
    ccb* ccb;
    root* parsubvol = NULL;
    send_context* context;
    send_info* send;
    ULONG num_clones = 0;
    root** clones = NULL;

    if (!FileObject || !FileObject->FsContext || !FileObject->FsContext2 || FileObject->FsContext == Vcb->volume_fcb)
        return STATUS_INVALID_PARAMETER;

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), Irp->RequestorMode))
        return STATUS_PRIVILEGE_NOT_HELD;

    fcb = FileObject->FsContext;
    ccb = FileObject->FsContext2;

    if (fcb->inode != SUBVOL_ROOT_INODE || fcb == Vcb->root_fileref->fcb)
        return STATUS_INVALID_PARAMETER;

    if (!Vcb->readonly && !(fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY))
        return STATUS_INVALID_PARAMETER;

    if (data) {
        btrfs_send_subvol* bss = (btrfs_send_subvol*)data;
        HANDLE parent;

#if defined(_WIN64)
        if (IoIs32bitProcess(Irp)) {
            btrfs_send_subvol32* bss32 = (btrfs_send_subvol32*)data;

            if (datalen < offsetof(btrfs_send_subvol32, num_clones))
                return STATUS_INVALID_PARAMETER;

            parent = Handle32ToHandle(bss32->parent);

            if (datalen >= offsetof(btrfs_send_subvol32, clones[0]))
                num_clones = bss32->num_clones;

            if (datalen < offsetof(btrfs_send_subvol32, clones[0]) + (num_clones * sizeof(UINT32)))
                return STATUS_INVALID_PARAMETER;
        } else {
#endif
            if (datalen < offsetof(btrfs_send_subvol, num_clones))
                return STATUS_INVALID_PARAMETER;

            parent = bss->parent;

            if (datalen >= offsetof(btrfs_send_subvol, clones[0]))
                num_clones = bss->num_clones;

            if (datalen < offsetof(btrfs_send_subvol, clones[0]) + (num_clones * sizeof(HANDLE)))
                return STATUS_INVALID_PARAMETER;
#if defined(_WIN64)
        }
#endif

        if (parent) {
            PFILE_OBJECT fileobj;
            struct _fcb* parfcb;

            Status = ObReferenceObjectByHandle(parent, 0, *IoFileObjectType, Irp->RequestorMode, (void**)&fileobj, NULL);
            if (!NT_SUCCESS(Status)) {
                ERR("ObReferenceObjectByHandle returned %08x\n", Status);
                return Status;
            }

            if (fileobj->DeviceObject != FileObject->DeviceObject) {
                ObDereferenceObject(fileobj);
                return STATUS_INVALID_PARAMETER;
            }

            parfcb = fileobj->FsContext;

            if (!parfcb || parfcb == Vcb->root_fileref->fcb || parfcb == Vcb->volume_fcb || parfcb->inode != SUBVOL_ROOT_INODE) {
                ObDereferenceObject(fileobj);
                return STATUS_INVALID_PARAMETER;
            }

            parsubvol = parfcb->subvol;
            ObDereferenceObject(fileobj);

            if (!Vcb->readonly && !(parsubvol->root_item.flags & BTRFS_SUBVOL_READONLY))
                return STATUS_INVALID_PARAMETER;

            if (parsubvol == fcb->subvol)
                return STATUS_INVALID_PARAMETER;
        }

        if (num_clones > 0) {
            ULONG i;

            clones = ExAllocatePoolWithTag(PagedPool, sizeof(root*) * num_clones, ALLOC_TAG);
            if (!clones) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            for (i = 0; i < num_clones; i++) {
                HANDLE h;
                PFILE_OBJECT fileobj;
                struct _fcb* clonefcb;

#if defined(_WIN64)
                if (IoIs32bitProcess(Irp)) {
                    btrfs_send_subvol32* bss32 = (btrfs_send_subvol32*)data;

                    h = Handle32ToHandle(bss32->clones[i]);
                } else
#endif
                    h = bss->clones[i];

                Status = ObReferenceObjectByHandle(h, 0, *IoFileObjectType, Irp->RequestorMode, (void**)&fileobj, NULL);
                if (!NT_SUCCESS(Status)) {
                    ERR("ObReferenceObjectByHandle returned %08x\n", Status);
                    ExFreePool(clones);
                    return Status;
                }

                if (fileobj->DeviceObject != FileObject->DeviceObject) {
                    ObDereferenceObject(fileobj);
                    ExFreePool(clones);
                    return STATUS_INVALID_PARAMETER;
                }

                clonefcb = fileobj->FsContext;

                if (!clonefcb || clonefcb == Vcb->root_fileref->fcb || clonefcb == Vcb->volume_fcb || clonefcb->inode != SUBVOL_ROOT_INODE) {
                    ObDereferenceObject(fileobj);
                    ExFreePool(clones);
                    return STATUS_INVALID_PARAMETER;
                }

                clones[i] = clonefcb->subvol;
                ObDereferenceObject(fileobj);

                if (!Vcb->readonly && !(clones[i]->root_item.flags & BTRFS_SUBVOL_READONLY)) {
                    ExFreePool(clones);
                    return STATUS_INVALID_PARAMETER;
                }
            }
        }
    }

    ExAcquireResourceExclusiveLite(&Vcb->send_load_lock, TRUE);

    if (ccb->send) {
        WARN("send operation already running\n");
        ExReleaseResourceLite(&Vcb->send_load_lock);
        return STATUS_DEVICE_NOT_READY;
    }

    context = ExAllocatePoolWithTag(NonPagedPool, sizeof(send_context), ALLOC_TAG);
    if (!context) {
        ERR("out of memory\n");
        ExReleaseResourceLite(&Vcb->send_load_lock);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Vcb = Vcb;
    context->root = fcb->subvol;
    context->parent = parsubvol;
    InitializeListHead(&context->orphans);
    InitializeListHead(&context->dirs);
    InitializeListHead(&context->pending_rmdirs);
    context->lastinode.inode = 0;
    context->lastinode.path = NULL;
    context->lastinode.sd = NULL;
    context->root_dir = NULL;
    context->num_clones = num_clones;
    context->clones = clones;
    InitializeListHead(&context->lastinode.refs);
    InitializeListHead(&context->lastinode.oldrefs);
    InitializeListHead(&context->lastinode.exts);
    InitializeListHead(&context->lastinode.oldexts);

    context->data = ExAllocatePoolWithTag(PagedPool, SEND_BUFFER_LENGTH + (2 * MAX_SEND_WRITE), ALLOC_TAG); // give ourselves some wiggle room
    if (!context->data) {
        ExFreePool(context);
        ExReleaseResourceLite(&Vcb->send_load_lock);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->datalen = 0;

    send_subvol_header(context, fcb->subvol, ccb->fileref); // FIXME - fileref needs some sort of lock here

    KeInitializeEvent(&context->buffer_event, NotificationEvent, FALSE);

    send = ExAllocatePoolWithTag(NonPagedPool, sizeof(send_info), ALLOC_TAG);
    if (!send) {
        ERR("out of memory\n");
        ExFreePool(context->data);
        ExFreePool(context);
        ExReleaseResourceLite(&Vcb->send_load_lock);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&send->cleared_event, NotificationEvent, FALSE);

    send->context = context;
    context->send = send;

    ccb->send = send;
    send->ccb = ccb;
    ccb->send_status = STATUS_SUCCESS;

    send->cancelling = FALSE;

    InterlockedIncrement(&Vcb->running_sends);

    Status = PsCreateSystemThread(&send->thread, 0, NULL, NULL, NULL, send_thread, context);
    if (!NT_SUCCESS(Status)) {
        ERR("PsCreateSystemThread returned %08x\n", Status);
        ccb->send = NULL;
        InterlockedDecrement(&Vcb->running_sends);
        ExFreePool(send);
        ExFreePool(context->data);
        ExFreePool(context);
        ExReleaseResourceLite(&Vcb->send_load_lock);
        return Status;
    }

    InsertTailList(&Vcb->send_ops, &send->list_entry);
    ExReleaseResourceLite(&Vcb->send_load_lock);

    return STATUS_SUCCESS;
}

NTSTATUS read_send_buffer(device_extension* Vcb, PFILE_OBJECT FileObject, void* data, ULONG datalen, ULONG_PTR* retlen, KPROCESSOR_MODE processor_mode) {
    ccb* ccb;
    send_context* context;

    ccb = FileObject ? FileObject->FsContext2 : NULL;
    if (!ccb)
        return STATUS_INVALID_PARAMETER;

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(SE_MANAGE_VOLUME_PRIVILEGE), processor_mode))
        return STATUS_PRIVILEGE_NOT_HELD;

    ExAcquireResourceExclusiveLite(&Vcb->send_load_lock, TRUE);

    if (!ccb->send) {
        ExReleaseResourceLite(&Vcb->send_load_lock);
        return !NT_SUCCESS(ccb->send_status) ? ccb->send_status : STATUS_END_OF_FILE;
    }

    context = (send_context*)ccb->send->context;

    KeWaitForSingleObject(&context->buffer_event, Executive, KernelMode, FALSE, NULL);

    if (datalen == 0) {
        ExReleaseResourceLite(&Vcb->send_load_lock);
        return STATUS_SUCCESS;
    }

    RtlCopyMemory(data, context->data, min(datalen, context->datalen));

    if (datalen < context->datalen) { // not empty yet
        *retlen = datalen;
        RtlMoveMemory(context->data, &context->data[datalen], context->datalen - datalen);
        context->datalen -= datalen;
        ExReleaseResourceLite(&Vcb->send_load_lock);
    } else {
        *retlen = context->datalen;
        context->datalen = 0;
        ExReleaseResourceLite(&Vcb->send_load_lock);

        KeClearEvent(&context->buffer_event);
        KeSetEvent(&ccb->send->cleared_event, 0, FALSE);
    }

    return STATUS_SUCCESS;
}
