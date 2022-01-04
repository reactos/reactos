/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     BTRFS support for FreeLoader
 * COPYRIGHT:   Copyright 2018 Victor Perevertkin (victor@perevertkin.ru)
 */

/* Some code was taken from u-boot, https://github.com/u-boot/u-boot/tree/master/fs/btrfs */

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(FILESYSTEM);

#define TAG_BTRFS_INFO 'IftB'
#define TAG_BTRFS_CHUNK_MAP 'CftB'
#define TAG_BTRFS_NODE  'NftB'
#define TAG_BTRFS_FILE  'FftB'
#define TAG_BTRFS_LINK  'LftB'

#define INVALID_INODE _UI64_MAX
#define INVALID_ADDRESS _UI64_MAX
#define READ_ERROR _UI64_MAX

typedef struct _BTRFS_INFO
{
    ULONG DeviceId;
    struct btrfs_super_block SuperBlock;
    struct btrfs_chunk_map ChunkMap;
    struct btrfs_root_item FsRoot;
    struct btrfs_root_item ExtentRoot;
} BTRFS_INFO;

PBTRFS_INFO BtrFsVolumes[MAX_FDS];

/* compare function used for bin_search */
typedef int (*cmp_func)(const void *ptr1, const void *ptr2);

/* simple but useful bin search, used for chunk search and btree search */
static int bin_search(void *ptr, int item_size, void *cmp_item, cmp_func func,
                      int min, int max, int *slot)
{
    int low = min;
    int high = max;
    int mid;
    int ret;
    unsigned long offset;
    UCHAR *item;

    while (low < high)
    {
        mid = (low + high) / 2;
        offset = mid * item_size;

        item = (UCHAR *) ptr + offset;
        ret = func((void *) item, cmp_item);

        if (ret < 0)
        {
            low = mid + 1;
        }
        else if (ret > 0)
        {
            high = mid;
        }
        else
        {
            *slot = mid;
            return 0;
        }
    }
    *slot = low;
    return 1;
}

static int btrfs_comp_chunk_map(struct btrfs_chunk_map_item *m1,
                                struct btrfs_chunk_map_item *m2)
{
    if (m1->logical > m2->logical)
        return 1;
    if (m1->logical < m2->logical)
        return -1;
    return 0;
}

/* insert a new chunk mapping item */
static void insert_chunk_item(struct btrfs_chunk_map *chunk_map, struct btrfs_chunk_map_item *item)
{
    int ret;
    int slot;
    int i;

    if (chunk_map->map == NULL)
    {
        /* first item */
        chunk_map->map_length = BTRFS_MAX_CHUNK_ENTRIES;
        chunk_map->map = FrLdrTempAlloc(chunk_map->map_length * sizeof(chunk_map->map[0]), TAG_BTRFS_CHUNK_MAP);
        chunk_map->map[0] = *item;
        chunk_map->cur_length = 1;
        return;
    }
    ret = bin_search(chunk_map->map, sizeof(*item), item,
                     (cmp_func) btrfs_comp_chunk_map, 0,
                     chunk_map->cur_length, &slot);
    if (ret == 0)/* already in map */
        return;

    if (chunk_map->cur_length == BTRFS_MAX_CHUNK_ENTRIES)
    {
        /* should be impossible */
        TRACE("too many chunk items\n");
        return;
    }

    for (i = chunk_map->cur_length; i > slot; i--)
        chunk_map->map[i] = chunk_map->map[i - 1];

    chunk_map->map[slot] = *item;
    chunk_map->cur_length++;
}

static inline void insert_map(struct btrfs_chunk_map *chunk_map,
                              const struct btrfs_disk_key *key,
                              struct btrfs_chunk *chunk)
{
    struct btrfs_stripe *stripe = &chunk->stripe;
    struct btrfs_stripe *stripe_end = stripe + chunk->num_stripes;
    struct btrfs_chunk_map_item item;

    item.logical = key->offset;
    item.length = chunk->length;
    for (; stripe < stripe_end; stripe++)
    {
        TRACE("stripe: %p\n", stripe);
        item.devid = stripe->devid;
        item.physical = stripe->offset;
        TRACE("inserting chunk log: %llx len: %llx devid: %llx phys: %llx\n",
              item.logical, item.length, item.devid, item.physical);
        insert_chunk_item(chunk_map, &item);
    }

#if 0
    struct btrfs_chunk_map_item *itm;
    int i;

    TRACE("insert finished. Printing chunk map:\n------------------------------\n");

    for (i = 0; i < chunk_map->cur_length; i++)
    {
        itm = &chunk_map->map[i];
        TRACE("%llx..%llx -> %llx..%llx, devid: %llu\n",
              itm->logical,
              itm->logical + itm->length,
              itm->physical,
              itm->physical + itm->length,
              itm->devid);
    }
#endif
}

static inline unsigned long btrfs_chunk_item_size(int num_stripes)
{
    return sizeof(struct btrfs_chunk) +
           sizeof(struct btrfs_stripe) * (num_stripes - 1);
}

static inline void init_path(const struct btrfs_super_block *sb, struct btrfs_path *path)
{
    RtlZeroMemory(path, sizeof(*path));
    path->tree_buf = FrLdrTempAlloc(sb->nodesize, TAG_BTRFS_NODE);
}

static inline void free_path(struct btrfs_path *path)
{
    if (path->tree_buf) FrLdrTempFree(path->tree_buf, TAG_BTRFS_NODE);
}

static inline struct btrfs_item *path_current_item(struct btrfs_path *path)
{
    return &path->tree_buf->leaf.items[path->slots[0]];
}

static inline UCHAR *path_current_data(struct btrfs_path *path)
{
    return (UCHAR *) path->tree_buf->leaf.items + path_current_item(path)->offset;
}

static inline const struct btrfs_disk_key *path_current_disk_key(struct btrfs_path *path)
{
    return &path_current_item(path)->key;
}


static int btrfs_comp_keys(const struct btrfs_disk_key *k1,
                           const struct btrfs_disk_key *k2)
{
    if (k1->objectid > k2->objectid)
        return 1;
    if (k1->objectid < k2->objectid)
        return -1;
    if (k1->type > k2->type)
        return 1;
    if (k1->type < k2->type)
        return -1;
    if (k1->offset > k2->offset)
        return 1;
    if (k1->offset < k2->offset)
        return -1;
    return 0;
}

/* compare keys but ignore offset, is useful to enumerate all same kind keys */
static int btrfs_comp_keys_type(const struct btrfs_disk_key *k1,
                                const struct btrfs_disk_key *k2)
{
    if (k1->objectid > k2->objectid)
        return 1;
    if (k1->objectid < k2->objectid)
        return -1;
    if (k1->type > k2->type)
        return 1;
    if (k1->type < k2->type)
        return -1;
    return 0;
}

/*
 * from sys_chunk_array or chunk_tree, we can convert a logical address to
 * a physical address we can not support multi device case yet
 */
static u64 logical_physical(struct btrfs_chunk_map *chunk_map, u64 logical)
{
    struct btrfs_chunk_map_item item;
    int slot, ret;

    item.logical = logical;
    ret = bin_search(chunk_map->map, sizeof(chunk_map->map[0]), &item,
                     (cmp_func) btrfs_comp_chunk_map, 0,
                     chunk_map->cur_length, &slot);
    if (ret == 0)
        slot++;
    else if (slot == 0)
        return INVALID_ADDRESS;
    if (logical >= chunk_map->map[slot - 1].logical + chunk_map->map[slot - 1].length)
        return INVALID_ADDRESS;

    TRACE("Address translation: 0x%llx -> 0x%llx\n", logical,
          chunk_map->map[slot - 1].physical + logical - chunk_map->map[slot - 1].logical);

    return chunk_map->map[slot - 1].physical + logical - chunk_map->map[slot - 1].logical;
}

static BOOLEAN disk_read(ULONG DeviceId, u64 physical, void *dest, u32 count)
{
    LARGE_INTEGER Position;
    ULONG Count;
    ARC_STATUS Status;

    if (!dest)
        return FALSE;

    Position.QuadPart = physical;
    Status = ArcSeek(DeviceId, &Position, SeekAbsolute);
    if (Status != ESUCCESS)
    {
        ERR("ArcSeek returned status %lu\n", Status);
        return FALSE;
    }

    Status = ArcRead(DeviceId, dest, count, &Count);
    if (Status != ESUCCESS || Count != count)
    {
        ERR("ArcRead returned status %lu\n", Status);
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN
_BtrFsSearchTree(PBTRFS_INFO BtrFsInfo, u64 loffset, u8 level,
                 struct btrfs_disk_key *key, struct btrfs_path *path)
{
    union tree_buf *tree_buf = path->tree_buf;
    int slot, ret, lvl;
    u64 physical, logical = loffset;

    TRACE("BtrFsSearchTree called: offset: 0x%llx, level: %u (%llu %u %llu)\n",
          loffset, level, key->objectid, key->type, key->offset);

    if (!tree_buf)
    {
        ERR("Path struct is not allocated\n");
        return FALSE;
    }

    for (lvl = level; lvl >= 0; lvl--)
    {
        physical = logical_physical(&BtrFsInfo->ChunkMap, logical);

        if (!disk_read(BtrFsInfo->DeviceId, physical, &tree_buf->header,
                       BtrFsInfo->SuperBlock.nodesize))
        {
            ERR("Error when reading tree node, loffset: 0x%llx, poffset: 0x%llx, level: %u\n",
                logical, physical, lvl);
            return FALSE;
        }

        if (tree_buf->header.level != lvl)
        {
            ERR("Error when searching in tree: expected lvl=%u but got %u\n",
                lvl, tree_buf->header.level);
            return FALSE;
        }

        TRACE("BtrFsSearchTree loop, level %u, loffset: 0x%llx\n", lvl, logical);

        if (lvl)
        {
            ret = bin_search(tree_buf->node.ptrs,
                             sizeof(struct btrfs_key_ptr),
                             key, (cmp_func) btrfs_comp_keys,
                             path->slots[lvl],
                             tree_buf->header.nritems, &slot);
            TRACE("Inner node, min=%lu max=%lu\n", path->slots[0], tree_buf->header.nritems);
            if (ret && slot > path->slots[lvl])
                --slot;
        }
        else
        {
            ret = bin_search(tree_buf->leaf.items,
                             sizeof(struct btrfs_item),
                             key, (cmp_func) btrfs_comp_keys,
                             path->slots[0],
                             tree_buf->header.nritems, &slot);
            TRACE("Leaf node, min=%lu max=%lu\n", path->slots[0], tree_buf->header.nritems);
            if (slot == tree_buf->header.nritems)
                --slot;
        }

        path->itemsnr[lvl] = tree_buf->header.nritems;
        path->offsets[lvl] = logical;
        path->slots[lvl] = slot;

        logical = tree_buf->node.ptrs[slot].blockptr;
    }

    TRACE("Found slot no=%lu\n", slot);

    TRACE("BtrFsSearchTree found item (%llu %u %llu) offset: %lu, size: %lu, returning %lu\n",
          path_current_disk_key(path)->objectid, path_current_disk_key(path)->type, path_current_disk_key(path)->offset,
          path_current_item(path)->offset, path_current_item(path)->size, !ret);

    return !ret;
}

static inline BOOLEAN
BtrFsSearchTree(PBTRFS_INFO BtrFsInfo, const struct btrfs_root_item *root,
                struct btrfs_disk_key *key, struct btrfs_path *path)
{
    return _BtrFsSearchTree(BtrFsInfo, root->bytenr, root->level, key, path);
}

static inline BOOLEAN
BtrFsSearchTreeType(PBTRFS_INFO BtrFsInfo, const struct btrfs_root_item *root,
                    u64 objectid, u8 type, struct btrfs_path *path)
{
    struct btrfs_disk_key key;

    key.objectid = objectid;
    key.type = type;
    key.offset = 0;

    if (!_BtrFsSearchTree(BtrFsInfo, root->bytenr, root->level, &key, path))
        return FALSE;

    if (path_current_disk_key(path)->objectid && !btrfs_comp_keys_type(&key, path_current_disk_key(path)))
        return TRUE;
    else
        return FALSE;
}

/* return 0 if slot found */
static int next_slot(PBTRFS_INFO BtrFsInfo,
                     struct btrfs_disk_key *key, struct btrfs_path *path)
{
    int slot, level = 1;

    if (!path->itemsnr[0])
        return 1;
    slot = path->slots[0] + 1;
    if (slot >= path->itemsnr[0])
    {
        /* jumping to next leaf */
        while (level < BTRFS_MAX_LEVEL)
        {
            if (!path->itemsnr[level]) /* no more nodes */
                return 1;
            slot = path->slots[level] + 1;
            if (slot >= path->itemsnr[level])
            {
                level++;
                continue;
            }
            path->slots[level] = slot;
            path->slots[level - 1] = 0; /* reset low level slots info */
            path->itemsnr[level - 1] = 0;
            path->offsets[level - 1] = 0;
            _BtrFsSearchTree(BtrFsInfo, path->offsets[level], level, key, path);
            break;
        }
        if (level == BTRFS_MAX_LEVEL)
            return 1;
        goto out;
    }
    path->slots[0] = slot;

out:
    if (path_current_disk_key(path)->objectid && !btrfs_comp_keys_type(key, path_current_disk_key(path)))
        return 0;
    else
        return 1;
}

static int prev_slot(struct btrfs_disk_key *key,
                     struct btrfs_path *path)
{
    if (!path->slots[0])
        return 1;
    --path->slots[0];
    if (path_current_disk_key(path)->objectid && !btrfs_comp_keys_type(key, path_current_disk_key(path)))
        return 0;
    else
        return 1;
}

/*
 * read chunk_array in super block
 */
static void btrfs_read_sys_chunk_array(PBTRFS_INFO BtrFsInfo)
{
    const struct btrfs_super_block *sb = &BtrFsInfo->SuperBlock;
    struct btrfs_disk_key *key;
    struct btrfs_chunk *chunk;
    u16 cur;

    /* read chunk array in superblock */
    TRACE("reading chunk array\n-----------------------------\n");
    cur = 0;
    while (cur < sb->sys_chunk_array_size)
    {
        key = (struct btrfs_disk_key *) (sb->sys_chunk_array + cur);
        TRACE("chunk key objectid: %llx, offset: %llx, type: %u\n",
              key->objectid, key->offset, key->type);
        cur += sizeof(*key);
        chunk = (struct btrfs_chunk *) (sb->sys_chunk_array + cur);
        TRACE("chunk length: %llx\n", chunk->length);
        TRACE("chunk owner: %llu\n", chunk->owner);
        TRACE("chunk stripe_len: %llx\n", chunk->stripe_len);
        TRACE("chunk type: %llu\n", chunk->type);
        TRACE("chunk io_align: %u\n", chunk->io_align);
        TRACE("chunk io_width: %u\n", chunk->io_width);
        TRACE("chunk sector_size: %u\n", chunk->sector_size);
        TRACE("chunk num_stripes: %u\n", chunk->num_stripes);
        TRACE("chunk sub_stripes: %u\n", chunk->sub_stripes);

        cur += btrfs_chunk_item_size(chunk->num_stripes);
        TRACE("read_sys_chunk_array() cur=%d\n", cur);
        insert_map(&BtrFsInfo->ChunkMap, key, chunk);
    }
}

/*
 * read chunk items from chunk_tree and insert them to chunk map
 * */
static void btrfs_read_chunk_tree(PBTRFS_INFO BtrFsInfo)
{
    const struct btrfs_super_block *sb = &BtrFsInfo->SuperBlock;
    struct btrfs_disk_key ignore_key;
    struct btrfs_disk_key search_key;
    struct btrfs_chunk *chunk;
    struct btrfs_path path;

    if (!(sb->flags & BTRFS_SUPER_FLAG_METADUMP))
    {
        if (sb->num_devices > 1)
            TRACE("warning: only support single device btrfs\n");

        ignore_key.objectid = BTRFS_DEV_ITEMS_OBJECTID;
        ignore_key.type = BTRFS_DEV_ITEM_KEY;

        /* read chunk from chunk_tree */
        search_key.objectid = BTRFS_FIRST_CHUNK_TREE_OBJECTID;
        search_key.type = BTRFS_CHUNK_ITEM_KEY;
        search_key.offset = 0;
        init_path(sb, &path);
        _BtrFsSearchTree(BtrFsInfo, sb->chunk_root, sb->chunk_root_level, &search_key, &path);
        do
        {
            /* skip information about underlying block
             * devices.
             */
            if (!btrfs_comp_keys_type(&ignore_key, path_current_disk_key(&path)))
                continue;
            if (btrfs_comp_keys_type(&search_key, path_current_disk_key(&path)))
                break;

            chunk = (struct btrfs_chunk *) (path_current_data(&path));
            insert_map(&BtrFsInfo->ChunkMap, path_current_disk_key(&path), chunk);
        } while (!next_slot(BtrFsInfo, &search_key, &path));
        free_path(&path);
    }
}

////////////////////////////////////////
///////////// DIR ITEM
////////////////////////////////////////

static BOOLEAN verify_dir_item(struct btrfs_dir_item *item, u32 start, u32 total)
{
    u16 max_len = BTRFS_NAME_MAX;
    u32 end;

    if (item->type >= BTRFS_FT_MAX)
    {
        ERR("Invalid dir item type: %i\n", item->type);
        return TRUE;
    }

    if (item->type == BTRFS_FT_XATTR)
        max_len = 255; /* XATTR_NAME_MAX */

    end = start + sizeof(*item) + item->name_len;
    if (item->name_len > max_len || end > total)
    {
        ERR("Invalid dir item name len: %u\n", item->name_len);
        return TRUE;
    }

    return FALSE;
}


static struct btrfs_dir_item *
BtrFsMatchDirItemName(struct btrfs_path *path, const char *name, int name_len)
{
    struct btrfs_dir_item *item = (struct btrfs_dir_item *) path_current_data(path);
    u32 cur = 0, this_len;
    const char *name_ptr;

    while (cur < path_current_item(path)->size)
    {
        this_len = sizeof(*item) + item->name_len + item->data_len;
        name_ptr = (const char *) item + sizeof(*item);

        if (verify_dir_item(item, cur, this_len))
            return NULL;
        if (item->name_len == name_len && !memcmp(name_ptr, name, name_len))
            return item;

        cur += this_len;
        item = (struct btrfs_dir_item *) ((u8 *) item + this_len);
    }

    return NULL;
}

static BOOLEAN
BtrFsLookupDirItem(PBTRFS_INFO BtrFsInfo,
                  const struct btrfs_root_item *root, u64 dir,
                  const char *name, int name_len,
                  struct btrfs_dir_item *item)
{
    struct btrfs_path path;
    struct btrfs_disk_key key;
    struct btrfs_dir_item *res = NULL;

    key.objectid = dir;
    key.type = BTRFS_DIR_ITEM_KEY;
    key.offset = btrfs_crc32c(name, name_len);
    init_path(&BtrFsInfo->SuperBlock, &path);

    if (!BtrFsSearchTree(BtrFsInfo, root, &key, &path))
    {
        free_path(&path);
        return FALSE;
    }

    res = BtrFsMatchDirItemName(&path, name, name_len);
    if (res)
        *item = *res;
    free_path(&path);

    return res != NULL;
}

static BOOLEAN
BtrFsLookupDirItemI(PBTRFS_INFO BtrFsInfo,
                   const struct btrfs_root_item *root, u64 dir_haystack,
                   const char *name, int name_len,
                   struct btrfs_dir_item *ret_item)
{
    struct btrfs_path path;
    struct btrfs_disk_key key;
    struct btrfs_dir_item *item;
    char *name_buf;
    BOOLEAN result = FALSE;

    key.objectid = dir_haystack;
    key.type = BTRFS_DIR_INDEX_KEY;
    key.offset = 0;
    init_path(&BtrFsInfo->SuperBlock, &path);

    BtrFsSearchTree(BtrFsInfo, root, &key, &path);

    if (btrfs_comp_keys_type(&key, path_current_disk_key(&path)))
        goto cleanup;

    do
    {
        item = (struct btrfs_dir_item *) path_current_data(&path);
        // TRACE("slot: %ld, KEY (%llu %u %llu) %.*s\n",
        //       path.slots[0], path.item.key.objectid, path.item.key.type,
        //       path.item.key.offset, item->name_len, (char *)item + sizeof(*item));

        if (verify_dir_item(item, 0, sizeof(*item) + item->name_len))
            continue;
        if (item->type == BTRFS_FT_XATTR)
            continue;

        name_buf = (char *) item + sizeof(*item);
        TRACE("Compare names %.*s and %.*s\n", name_len, name, item->name_len, name_buf);

        if (name_len == item->name_len && _strnicmp(name, name_buf, name_len) == 0)
        {
            *ret_item = *item;
            result = TRUE;
            goto cleanup;
        }

    } while (!next_slot(BtrFsInfo, &key, &path));

cleanup:
    free_path(&path);
    return result;
}

////////////////////////////////////////
///////////// EXTENTS
////////////////////////////////////////

static u64 btrfs_read_extent_inline(struct btrfs_path *path,
                                    struct btrfs_file_extent_item *extent,
                                    u64 offset, u64 size, char *out)
{
    u32 dlen;
    const char *cbuf;
    const int data_off = offsetof(
        struct btrfs_file_extent_item, disk_bytenr);

    cbuf = (const char *) extent + data_off;
    dlen = extent->ram_bytes;

    TRACE("read_extent_inline offset=%llu size=%llu gener=%llu\n", offset, size, extent->generation);

    if (offset > dlen)
    {
        ERR("Tried to read offset (%llu) beyond extent length (%lu)\n", offset, dlen);
        return READ_ERROR;
    }

    if (size > dlen - offset)
        size = dlen - offset;

    if (extent->compression == BTRFS_COMPRESS_NONE)
    {
        TRACE("read_extent_inline %lu, \n", data_off);
        memcpy(out, cbuf + offset, size);
        return size;
    }

    ERR("No compression supported right now\n");
    return READ_ERROR;
}

static u64 btrfs_read_extent_reg(PBTRFS_INFO BtrFsInfo,
                                 struct btrfs_path *path,
                                 struct btrfs_file_extent_item *extent,
                                 u64 offset, u64 size, char *out)
{
    u64 physical, dlen;
    char *temp_out;
    dlen = extent->num_bytes;

    if (offset > dlen)
    {
        ERR("Tried to read offset (%llu) beyond extent length (%lu)\n", offset, dlen);
        return READ_ERROR;
    }

    if (size > dlen - offset)
        size = dlen - offset;

    /* Handle sparse extent */
    if (extent->disk_bytenr == 0 && extent->disk_num_bytes == 0)
    {
        RtlZeroMemory(out, size);
        return size;
    }

    physical = logical_physical(&BtrFsInfo->ChunkMap, extent->disk_bytenr);
    if (physical == INVALID_ADDRESS)
    {
        ERR("Unable to convert logical address to physical: %llu\n", extent->disk_bytenr);
        return READ_ERROR;
    }

    if (extent->compression == BTRFS_COMPRESS_NONE)
    {
        physical += extent->offset + offset;

        /* If somebody tried to do unaligned access */
        if (physical & (SECTOR_SIZE - 1))
        {
            u32 shift;

            temp_out = FrLdrTempAlloc(SECTOR_SIZE, TAG_BTRFS_FILE);

            if (!disk_read(BtrFsInfo->DeviceId,
                           ALIGN_DOWN_BY(physical, SECTOR_SIZE),
                           temp_out, SECTOR_SIZE))
            {
                FrLdrTempFree(temp_out, TAG_BTRFS_FILE);
                return READ_ERROR;
            }

            shift = (u32)(physical & (SECTOR_SIZE - 1));

            if (size <= SECTOR_SIZE - shift)
            {
                memcpy(out, temp_out + shift, size);
                FrLdrTempFree(temp_out, TAG_BTRFS_FILE);
                return size;
            }

            memcpy(out, temp_out + shift, SECTOR_SIZE - shift);
            FrLdrTempFree(temp_out, TAG_BTRFS_FILE);

            if (!disk_read(BtrFsInfo->DeviceId,
                           physical + SECTOR_SIZE - shift,
                           out + SECTOR_SIZE - shift,
                           size - SECTOR_SIZE + shift))
            {
                return READ_ERROR;
            }
        } else
        {
            if (!disk_read(BtrFsInfo->DeviceId, physical, out, size))
                return READ_ERROR;
        }

        return size;
    }

    ERR("No compression supported right now\n");
    return READ_ERROR;
}

static u64 btrfs_file_read(PBTRFS_INFO BtrFsInfo,
                           const struct btrfs_root_item *root,
                           u64 inr, u64 offset, u64 size, char *buf)
{
    struct btrfs_path path;
    struct btrfs_disk_key key;
    struct btrfs_file_extent_item *extent;
    int res = 0;
    u64 rd, seek_pointer = READ_ERROR, offset_in_extent;
    BOOLEAN find_res;

    TRACE("btrfs_file_read inr=%llu offset=%llu size=%llu\n", inr, offset, size);

    key.objectid = inr;
    key.type = BTRFS_EXTENT_DATA_KEY;
    key.offset = offset;
    init_path(&BtrFsInfo->SuperBlock, &path);

    find_res = BtrFsSearchTree(BtrFsInfo, root, &key, &path);

    /* if we found greater key, switch to the previous one */
    if (!find_res && btrfs_comp_keys(&key, path_current_disk_key(&path)) < 0)
    {
        if (prev_slot(&key, &path))
            goto out;

    } else if (btrfs_comp_keys_type(&key, path_current_disk_key(&path)))
    {
        goto out;
    }

    seek_pointer = offset;

    do
    {
        TRACE("Current extent: (%llu %u %llu) \n",
              path_current_disk_key(&path)->objectid,
              path_current_disk_key(&path)->type,
              path_current_disk_key(&path)->offset);

        extent = (struct btrfs_file_extent_item *) path_current_data(&path);

        offset_in_extent = seek_pointer;
        /* check if we need clean extent offset when switching to the next extent */
        if ((seek_pointer) >= path_current_disk_key(&path)->offset)
            offset_in_extent -= path_current_disk_key(&path)->offset;

        if (extent->type == BTRFS_FILE_EXTENT_INLINE)
        {
            rd = btrfs_read_extent_inline(&path, extent, offset_in_extent, size, buf);
        }
        else
        {
            rd = btrfs_read_extent_reg(BtrFsInfo, &path, extent, offset_in_extent, size, buf);
        }

        if (rd == READ_ERROR)
        {
            ERR("Error while reading extent\n");
            seek_pointer = READ_ERROR;
            goto out;
        }

        buf += rd;
        seek_pointer += rd;
        size -= rd;
        TRACE("file_read size=%llu rd=%llu seek_pointer=%llu\n", size, rd, seek_pointer);

        if (!size)
            break;
    } while (!(res = next_slot(BtrFsInfo, &key, &path)));

    if (res)
    {
        seek_pointer = READ_ERROR;
        goto out;
    }

    seek_pointer -= offset;
out:
    free_path(&path);
    return seek_pointer;
}

////////////////////////////////////////
///////////// INODE
////////////////////////////////////////


static u64 btrfs_lookup_inode_ref(PBTRFS_INFO BtrFsInfo,
                                  const struct btrfs_root_item *root, u64 inr,
                                  struct btrfs_inode_ref *refp, char *name)
{
    struct btrfs_path path;
    struct btrfs_inode_ref *ref;
    u64 ret = INVALID_INODE;
    init_path(&BtrFsInfo->SuperBlock, &path);

    if (BtrFsSearchTreeType(BtrFsInfo, root, inr, BTRFS_INODE_REF_KEY, &path))
    {
        ref = (struct btrfs_inode_ref *) path_current_data(&path);

        if (refp)
            *refp = *ref;
        ret = path_current_disk_key(&path)->offset;
    }

    free_path(&path);
    return ret;
}

static int btrfs_lookup_inode(PBTRFS_INFO BtrFsInfo,
                              const struct btrfs_root_item *root,
                              struct btrfs_disk_key *location,
                              struct btrfs_inode_item *item,
                              struct btrfs_root_item *new_root)
{
    const struct btrfs_root_item tmp_root = *root;
    struct btrfs_path path;
    int res = -1;

//    if (location->type == BTRFS_ROOT_ITEM_KEY) {
//        if (btrfs_find_root(location->objectid, &tmp_root, NULL))
//            return -1;
//
//        location->objectid = tmp_root.root_dirid;
//        location->type = BTRFS_INODE_ITEM_KEY;
//        location->offset = 0;
//    }
    init_path(&BtrFsInfo->SuperBlock, &path);
    TRACE("Searching inode (%llu %u %llu)\n", location->objectid, location->type, location->offset);

    if (BtrFsSearchTree(BtrFsInfo, &tmp_root, location, &path))
    {
        if (item)
            *item = *((struct btrfs_inode_item *) path_current_data(&path));

        if (new_root)
            *new_root = tmp_root;

        res = 0;
    }

    free_path(&path);
    return res;
}

static BOOLEAN btrfs_readlink(PBTRFS_INFO BtrFsInfo,
                              const struct btrfs_root_item *root,
                              u64 inr, char **target)
{
    struct btrfs_path path;
    struct btrfs_file_extent_item *extent;
    char *data_ptr;
    BOOLEAN res = FALSE;

    init_path(&BtrFsInfo->SuperBlock, &path);

    if (!BtrFsSearchTreeType(BtrFsInfo, root, inr, BTRFS_EXTENT_DATA_KEY, &path))
        goto out;

    extent = (struct btrfs_file_extent_item *) path_current_data(&path);
    if (extent->type != BTRFS_FILE_EXTENT_INLINE)
    {
        ERR("Extent for symlink %llu not of INLINE type\n", inr);
        goto out;
    }

    if (extent->compression != BTRFS_COMPRESS_NONE)
    {
        ERR("Symlink %llu extent data compressed!\n", inr);
        goto out;
    }
    else if (extent->encryption != 0)
    {
        ERR("Symlink %llu extent data encrypted!\n", inr);
        goto out;
    }
    else if (extent->ram_bytes >= BtrFsInfo->SuperBlock.sectorsize)
    {
        ERR("Symlink %llu extent data too long (%llu)!\n", inr, extent->ram_bytes);
        goto out;
    }

    data_ptr = (char *) extent + offsetof(
        struct btrfs_file_extent_item, disk_bytenr);

    *target = FrLdrTempAlloc(extent->ram_bytes + 1, TAG_BTRFS_LINK);
    if (!*target)
    {
        ERR("Cannot allocate %llu bytes\n", extent->ram_bytes + 1);
        goto out;
    }

    memcpy(*target, data_ptr, extent->ram_bytes);
    (*target)[extent->ram_bytes] = '\0';

    res = TRUE;

out:
    free_path(&path);
    return res;
}

/* inr must be a directory (for regular files with multiple hard links this
   function returns only one of the parents of the file) */
static u64 get_parent_inode(PBTRFS_INFO BtrFsInfo,
                            const struct btrfs_root_item *root, u64 inr,
                            struct btrfs_inode_item *inode_item)
{
    struct btrfs_disk_key key;
    u64 res;

    if (inr == BTRFS_FIRST_FREE_OBJECTID)
    {
//        if (root->objectid != btrfs_info.fs_root.objectid) {
//            u64 parent;
//            struct btrfs_root_ref ref;
//
//            parent = btrfs_lookup_root_ref(root->objectid, &ref,
//                                           NULL);
//            if (parent == -1ULL)
//                return -1ULL;
//
//            if (btrfs_find_root(parent, root, NULL))
//                return -1ULL;
//
//            inr = ref.dirid;
//        }

        if (inode_item)
        {
            key.objectid = inr;
            key.type = BTRFS_INODE_ITEM_KEY;
            key.offset = 0;

            if (btrfs_lookup_inode(BtrFsInfo, root, &key, inode_item, NULL))
                return INVALID_INODE;
        }

        return inr;
    }

    res = btrfs_lookup_inode_ref(BtrFsInfo, root, inr, NULL, NULL);
    if (res == INVALID_INODE)
        return INVALID_INODE;

    if (inode_item)
    {
        key.objectid = res;
        key.type = BTRFS_INODE_ITEM_KEY;
        key.offset = 0;

        if (btrfs_lookup_inode(BtrFsInfo, root, &key, inode_item, NULL))
            return INVALID_INODE;
    }

    return res;
}

static inline int next_length(const char *path)
{
    int res = 0;
    while (*path != '\0' && *path != '/' && *path != '\\' && res <= BTRFS_NAME_MAX)
        ++res, ++path;
    return res;
}

static inline const char *skip_current_directories(const char *cur)
{
    while (1)
    {
        if (cur[0] == '/' || cur[0] == '\\')
            ++cur;
        else if (cur[0] == '.' && (cur[1] == '/' || cur[1] == '\\'))
            cur += 2;
        else
            break;
    }

    return cur;
}

static u64 btrfs_lookup_path(PBTRFS_INFO BtrFsInfo,
                             const struct btrfs_root_item *root, u64 inr, const char *path,
                             u8 *type_p, struct btrfs_inode_item *inode_item_p, int symlink_limit)
{
    struct btrfs_dir_item item;
    struct btrfs_inode_item inode_item;
    u8 type = BTRFS_FT_DIR;
    int len, have_inode = 0;
    const char *cur = path;
    struct btrfs_disk_key key;
    char *link_target = NULL;

    if (*cur == '/' || *cur == '\\')
    {
        ++cur;
        inr = root->root_dirid;
    }

    do
    {
        cur = skip_current_directories(cur);

        len = next_length(cur);
        if (len > BTRFS_NAME_MAX)
        {
            ERR("%s: Name too long at \"%.*s\"\n", BTRFS_NAME_MAX, cur);
            return INVALID_INODE;
        }

        if (len == 1 && cur[0] == '.')
            break;

        if (len == 2 && cur[0] == '.' && cur[1] == '.')
        {
            cur += 2;
            inr = get_parent_inode(BtrFsInfo, root, inr, &inode_item);
            if (inr == INVALID_INODE)
                return INVALID_INODE;

            type = BTRFS_FT_DIR;
            continue;
        }

        if (!*cur)
            break;

        if (!BtrFsLookupDirItem(BtrFsInfo, root, inr, cur, len, &item))
        {
            TRACE("Try to find case-insensitive, path=%s inr=%llu s=%.*s\n", path, inr, len, cur);
            if (!BtrFsLookupDirItemI(BtrFsInfo, root, inr, cur, len, &item))
                return INVALID_INODE;
        }

        type = item.type;
        have_inode = 1;
        if (btrfs_lookup_inode(BtrFsInfo, root, &item.location, &inode_item, NULL))
            return INVALID_INODE;

        if (type == BTRFS_FT_SYMLINK && symlink_limit >= 0)
        {
            if (!symlink_limit)
            {
                TRACE("%s: Too much symlinks!\n");
                return INVALID_INODE;
            }

            /* btrfs_readlink allocates link_target by itself */
            if (!btrfs_readlink(BtrFsInfo, root, item.location.objectid, &link_target))
                return INVALID_INODE;

            inr = btrfs_lookup_path(BtrFsInfo, root, inr, link_target, &type, &inode_item, symlink_limit - 1);

            FrLdrTempFree(link_target, TAG_BTRFS_LINK);

            if (inr == INVALID_INODE)
                return INVALID_INODE;
        } else if (type != BTRFS_FT_DIR && cur[len])
        {
            TRACE("%s: \"%.*s\" not a directory\n", (int) (cur - path + len), path);
            return INVALID_INODE;
        } else
        {
            inr = item.location.objectid;
        }

        cur += len;
    } while (*cur);

    if (type_p)
        *type_p = type;

    if (inode_item_p)
    {
        if (!have_inode)
        {
            key.objectid = inr;
            key.type = BTRFS_INODE_ITEM_KEY;
            key.offset = 0;

            if (btrfs_lookup_inode(BtrFsInfo, root, &key, &inode_item, NULL))
                return INVALID_INODE;
        }

        *inode_item_p = inode_item;
    }

    return inr;
}


ARC_STATUS BtrFsClose(ULONG FileId)
{
    pbtrfs_file_info phandle = FsGetDeviceSpecific(FileId);
    TRACE("BtrFsClose %lu\n", FileId);

    FrLdrTempFree(phandle, TAG_BTRFS_FILE);
    return ESUCCESS;
}

ARC_STATUS BtrFsGetFileInformation(ULONG FileId, FILEINFORMATION *Information)
{
    pbtrfs_file_info phandle = FsGetDeviceSpecific(FileId);

    RtlZeroMemory(Information, sizeof(*Information));
    Information->EndingAddress.QuadPart = phandle->inode.size;
    Information->CurrentAddress.QuadPart = phandle->position;

    TRACE("BtrFsGetFileInformation(%lu) -> FileSize = %llu, FilePointer = 0x%llx\n",
          FileId, Information->EndingAddress.QuadPart, Information->CurrentAddress.QuadPart);

    return ESUCCESS;
}

ARC_STATUS BtrFsOpen(CHAR *Path, OPENMODE OpenMode, ULONG *FileId)
{
    PBTRFS_INFO BtrFsInfo;
    ULONG DeviceId;
    u64 inr;
    u8 type;

    btrfs_file_info temp_file_info;
    pbtrfs_file_info phandle;

    TRACE("BtrFsOpen %s\n", Path);

    /* Check parameters */
    if (OpenMode != OpenReadOnly)
        return EACCES;

    /* Get underlying device */
    DeviceId = FsGetDeviceId(*FileId);
    BtrFsInfo = BtrFsVolumes[DeviceId];

    inr = btrfs_lookup_path(BtrFsInfo, &BtrFsInfo->FsRoot,
                            BtrFsInfo->FsRoot.root_dirid,
                            Path, &type, &temp_file_info.inode, 40);

    if (inr == INVALID_INODE)
    {
        TRACE("Cannot lookup file %s\n", Path);
        return ENOENT;
    }

    if (type != BTRFS_FT_REG_FILE)
    {
        TRACE("Not a regular file: %s\n", Path);
        return EISDIR;
    }

    TRACE("found inode inr=%llu size=%llu\n", inr, temp_file_info.inode.size);

    temp_file_info.inr = inr;
    temp_file_info.position = 0;

    phandle = FrLdrTempAlloc(sizeof(btrfs_file_info), TAG_BTRFS_FILE);
    if (!phandle)
        return ENOMEM;

    RtlCopyMemory(phandle, &temp_file_info, sizeof(btrfs_file_info));
    phandle->Volume = BtrFsInfo;

    FsSetDeviceSpecific(*FileId, phandle);
    return ESUCCESS;
}

ARC_STATUS BtrFsRead(ULONG FileId, VOID *Buffer, ULONG Size, ULONG *BytesRead)
{
    pbtrfs_file_info phandle = FsGetDeviceSpecific(FileId);
    u64 rd;

    TRACE("BtrFsRead %lu, size=%lu \n", FileId, Size);

    if (!Size)
        Size = phandle->inode.size;

    if (Size > phandle->inode.size)
        Size = phandle->inode.size;

    rd = btrfs_file_read(phandle->Volume, &phandle->Volume->FsRoot,
                         phandle->inr, phandle->position, Size, Buffer);
    if (rd == READ_ERROR)
    {
        TRACE("An error occured while reading file %lu\n", FileId);
        return ENOENT;
    }

    phandle->position += rd;
    *BytesRead = rd;
    return ESUCCESS;
}

ARC_STATUS BtrFsSeek(ULONG FileId, LARGE_INTEGER *Position, SEEKMODE SeekMode)
{
    pbtrfs_file_info phandle = FsGetDeviceSpecific(FileId);
    LARGE_INTEGER NewPosition = *Position;

    switch (SeekMode)
    {
        case SeekAbsolute:
            break;
        case SeekRelative:
            NewPosition.QuadPart += phandle->position;
            break;
        default:
            ASSERT(FALSE);
            return EINVAL;
    }

    if (NewPosition.QuadPart >= phandle->inode.size)
        return EINVAL;

    phandle->position = NewPosition.QuadPart;
    return ESUCCESS;
}

const DEVVTBL BtrFsFuncTable =
{
    BtrFsClose,
    BtrFsGetFileInformation,
    BtrFsOpen,
    BtrFsRead,
    BtrFsSeek,
    L"btrfs",
};

const DEVVTBL *BtrFsMount(ULONG DeviceId)
{
    PBTRFS_INFO BtrFsInfo;
    struct btrfs_path path;
    struct btrfs_root_item fs_root_item;

    TRACE("Enter BtrFsMount(%lu)\n", DeviceId);

    BtrFsInfo = FrLdrTempAlloc(sizeof(BTRFS_INFO), TAG_BTRFS_INFO);
    if (!BtrFsInfo)
        return NULL;
    RtlZeroMemory(BtrFsInfo, sizeof(BTRFS_INFO));

    /* Read the SuperBlock */
    if (!disk_read(DeviceId, BTRFS_SUPER_INFO_OFFSET,
                   &BtrFsInfo->SuperBlock, sizeof(BtrFsInfo->SuperBlock)))
    {
        FrLdrTempFree(BtrFsInfo, TAG_BTRFS_INFO);
        return NULL;
    }

    /* Check if SuperBlock is valid. If yes, return BTRFS function table */
    if (BtrFsInfo->SuperBlock.magic != BTRFS_MAGIC_N)
    {
        FrLdrTempFree(BtrFsInfo, TAG_BTRFS_INFO);
        return NULL;
    }

    BtrFsInfo->DeviceId = DeviceId;
    TRACE("BtrFsMount(%lu) superblock magic ok\n", DeviceId);

    btrfs_init_crc32c();

    btrfs_read_sys_chunk_array(BtrFsInfo);
    btrfs_read_chunk_tree(BtrFsInfo);

    /* setup roots */
    fs_root_item.bytenr = BtrFsInfo->SuperBlock.root;
    fs_root_item.level = BtrFsInfo->SuperBlock.root_level;

    init_path(&BtrFsInfo->SuperBlock, &path);
    if (!BtrFsSearchTreeType(BtrFsInfo, &fs_root_item, BTRFS_FS_TREE_OBJECTID, BTRFS_ROOT_ITEM_KEY, &path))
    {
        free_path(&path);
        FrLdrTempFree(BtrFsInfo, TAG_BTRFS_INFO);
        return NULL;
    }

    BtrFsInfo->FsRoot = *(struct btrfs_root_item *) path_current_data(&path);

    free_path(&path);

    /* Remember BTRFS volume information */
    BtrFsVolumes[DeviceId] = BtrFsInfo;

    TRACE("BtrFsMount(%lu) success\n", DeviceId);
    return &BtrFsFuncTable;
}
