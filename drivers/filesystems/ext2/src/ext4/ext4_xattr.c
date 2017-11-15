/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public Licens
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-
 */

#include <ext2fs.h>
#include <linux/module.h>
#include <linux/ext4_xattr.h>

static ext4_fsblk_t ext4_new_meta_blocks(void *icb, struct inode *inode,
	ext4_fsblk_t goal,
	unsigned int flags,
	unsigned long *count, int *errp)
{
	NTSTATUS status;
	ULONG blockcnt = (count) ? *count : 1;
	ULONG block = 0;

	status = Ext2NewBlock((PEXT2_IRP_CONTEXT)icb,
		inode->i_sb->s_priv,
		0, (ULONG)goal,
		&block,
		&blockcnt);
	if (count)
		*count = blockcnt;

	if (!NT_SUCCESS(status)) {
		*errp = Ext2LinuxError(status);
		return 0;
	}
	inode->i_blocks += (blockcnt * (inode->i_sb->s_blocksize >> 9));
	return block;
}

static void ext4_free_blocks(void *icb, struct inode *inode,
	ext4_fsblk_t block, int count, int flags)
{
	Ext2FreeBlock((PEXT2_IRP_CONTEXT)icb, inode->i_sb->s_priv, (ULONG)block, count);
	inode->i_blocks -= count * (inode->i_sb->s_blocksize >> 9);
	return;
}

static inline ext4_fsblk_t ext4_inode_to_goal_block(struct inode *inode)
{
	PEXT2_VCB Vcb;
	Vcb = inode->i_sb->s_priv;
	return (inode->i_ino - 1) / BLOCKS_PER_GROUP;
}

#define NAME_HASH_SHIFT 5
#define VALUE_HASH_SHIFT 16

static inline void ext4_xattr_compute_hash(struct ext4_xattr_header *header,
					   struct ext4_xattr_entry *entry)
{
	__u32 hash = 0;
	char *name = EXT4_XATTR_NAME(entry);
	int n;

	for (n = 0; n < entry->e_name_len; n++) {
		hash = (hash << NAME_HASH_SHIFT) ^
		       (hash >> (8 * sizeof(hash) - NAME_HASH_SHIFT)) ^ *name++;
	}

	if (entry->e_value_block == 0 && entry->e_value_size != 0) {
		__le32 *value =
		    (__le32 *)((char *)header + le16_to_cpu(entry->e_value_offs));
		for (n = (le32_to_cpu(entry->e_value_size) + EXT4_XATTR_ROUND) >>
			 EXT4_XATTR_PAD_BITS;
		     n; n--) {
			hash = (hash << VALUE_HASH_SHIFT) ^
			       (hash >> (8 * sizeof(hash) - VALUE_HASH_SHIFT)) ^
			       le32_to_cpu(*value++);
		}
	}
	entry->e_hash = cpu_to_le32(hash);
}

#define BLOCK_HASH_SHIFT 16

/*
 * ext4_xattr_rehash()
 *
 * Re-compute the extended attribute hash value after an entry has changed.
 */
static void ext4_xattr_rehash(struct ext4_xattr_header *header,
			      struct ext4_xattr_entry *entry)
{
	struct ext4_xattr_entry *here;
	__u32 hash = 0;

	ext4_xattr_compute_hash(header, entry);
	here = EXT4_XATTR_ENTRY(header + 1);
	while (!EXT4_XATTR_IS_LAST_ENTRY(here)) {
		if (!here->e_hash) {
			/* Block is not shared if an entry's hash value == 0 */
			hash = 0;
			break;
		}
		hash = (hash << BLOCK_HASH_SHIFT) ^
		       (hash >> (8 * sizeof(hash) - BLOCK_HASH_SHIFT)) ^
		       le32_to_cpu(here->e_hash);
		here = EXT4_XATTR_NEXT(here);
	}
	header->h_hash = cpu_to_le32(hash);
}

#if CONFIG_META_CSUM_ENABLE
static __u32
ext4_xattr_block_checksum(PEXT2_MCB inode_ref,
			  ext4_fsblk_t blocknr,
			  struct ext4_xattr_header *header)
{
	__u32 checksum = 0;
	__u64 le64_blocknr = blocknr;
	struct ext4_sblock *sb = &inode_ref->fs->sb;

	if (ext4_sb_feature_ro_com(sb, EXT4_FRO_COM_METADATA_CSUM)) {
		__u32 orig_checksum;

		/* Preparation: temporarily set bg checksum to 0 */
		orig_checksum = header->h_checksum;
		header->h_checksum = 0;
		/* First calculate crc32 checksum against fs uuid */
		checksum = ext4_crc32c(EXT4_CRC32_INIT, sb->uuid,
				sizeof(sb->uuid));
		/* Then calculate crc32 checksum block number */
		checksum = ext4_crc32c(checksum, &le64_blocknr,
				     sizeof(le64_blocknr));
		/* Finally calculate crc32 checksum against 
		 * the entire xattr block */
		checksum = ext4_crc32c(checksum, header,
				   ext4_sb_get_block_size(sb));
		header->h_checksum = orig_checksum;
	}
	return checksum;
}
#else
#define ext4_xattr_block_checksum(...) 0
#endif

static void
ext4_xattr_set_block_checksum(PEXT2_MCB inode_ref,
			      ext4_fsblk_t blocknr,
			      struct ext4_xattr_header *header)
{
	/* TODO: Need METADATA_CSUM supports. */
	header->h_checksum = 0;
}

static int ext4_xattr_item_cmp(struct rb_node *_a,
			       struct rb_node *_b)
{
	int result;
	struct ext4_xattr_item *a, *b;
	a = container_of(_a, struct ext4_xattr_item, node);
	a = container_of(_a, struct ext4_xattr_item, node);
	b = container_of(_b, struct ext4_xattr_item, node);

	if (a->is_data && !b->is_data)
		return -1;
	
	if (!a->is_data && b->is_data)
		return 1;

	result = a->name_index - b->name_index;
	if (result)
		return result;

	if (a->name_len < b->name_len)
		return -1;

	if (a->name_len > b->name_len)
		return 1;

	return memcmp(a->name, b->name, a->name_len);
}

//
// Red-black tree insert routine.
//

static struct ext4_xattr_item *
ext4_xattr_item_search(struct ext4_xattr_ref *xattr_ref,
		       struct ext4_xattr_item *name)
{
	struct rb_node *new = xattr_ref->root.rb_node;

	while (new) {
		struct ext4_xattr_item *node =
			container_of(new, struct ext4_xattr_item, node);
		int result = ext4_xattr_item_cmp(&name->node, new);

		if (result < 0)
			new = new->rb_left;
		else if (result > 0)
			new = new->rb_right;
		else
			return node;

	}

	return NULL;
}

static void ext4_xattr_item_insert(struct ext4_xattr_ref *xattr_ref,
				   struct ext4_xattr_item *item)
{
	rb_insert(&xattr_ref->root, &item->node,
	      ext4_xattr_item_cmp);
	list_add_tail(&item->list_node, &xattr_ref->ordered_list);
}

static void ext4_xattr_item_remove(struct ext4_xattr_ref *xattr_ref,
				   struct ext4_xattr_item *item)
{
	rb_erase(&item->node, &xattr_ref->root);
	list_del_init(&item->list_node);
}

static struct ext4_xattr_item *
ext4_xattr_item_alloc(__u8 name_index, const char *name, size_t name_len)
{
	struct ext4_xattr_item *item;
	item = kzalloc(sizeof(struct ext4_xattr_item) + name_len, GFP_NOFS);
	if (!item)
		return NULL;

	item->name_index = name_index;
	item->name = (char *)(item + 1);
	item->name_len = name_len;
	item->data = NULL;
	item->data_size = 0;
	INIT_LIST_HEAD(&item->list_node);

	memcpy(item->name, name, name_len);

	if (name_index == EXT4_XATTR_INDEX_SYSTEM &&
	    name_len == 4 &&
	    !memcmp(name, "data", 4))
		item->is_data = TRUE;
	else
		item->is_data = FALSE;

	return item;
}

static int ext4_xattr_item_alloc_data(struct ext4_xattr_item *item,
				      const void *orig_data, size_t data_size)
{
	void *data = NULL;
	ASSERT(!item->data);
	data = kmalloc(data_size, GFP_NOFS);
	if (!data)
		return -ENOMEM;

	if (orig_data)
		memcpy(data, orig_data, data_size);

	item->data = data;
	item->data_size = data_size;
	return 0;
}

static void ext4_xattr_item_free_data(struct ext4_xattr_item *item)
{
	ASSERT(item->data);
	kfree(item->data);
	item->data = NULL;
	item->data_size = 0;
}

static int ext4_xattr_item_resize_data(struct ext4_xattr_item *item,
				       size_t new_data_size)
{
	if (new_data_size != item->data_size) {
		void *new_data;
		new_data = kmalloc(new_data_size, GFP_NOFS);
		if (!new_data)
			return -ENOMEM;

		memcpy(new_data, item->data, item->data_size);
		kfree(item->data);

		item->data = new_data;
		item->data_size = new_data_size;
	}
	return 0;
}

static void ext4_xattr_item_free(struct ext4_xattr_item *item)
{
	if (item->data)
		ext4_xattr_item_free_data(item);

	kfree(item);
}

static void *ext4_xattr_entry_data(struct ext4_xattr_ref *xattr_ref,
				   struct ext4_xattr_entry *entry,
				   BOOL in_inode)
{
	char *ret;
	int block_size;
	if (in_inode) {
		struct ext4_xattr_ibody_header *header;
		struct ext4_xattr_entry *first_entry;
		int inode_size = xattr_ref->fs->InodeSize;
		header = EXT4_XATTR_IHDR(xattr_ref->OnDiskInode);
		first_entry = EXT4_XATTR_IFIRST(header);

		ret = ((char *)first_entry + le16_to_cpu(entry->e_value_offs));
		if (ret + EXT4_XATTR_SIZE(le32_to_cpu(entry->e_value_size)) -
			(char *)xattr_ref->OnDiskInode > inode_size)
			ret = NULL;

		return ret;

	}
	block_size = xattr_ref->fs->BlockSize;
	ret = ((char *)xattr_ref->block_bh->b_data + le16_to_cpu(entry->e_value_offs));
	if (ret + EXT4_XATTR_SIZE(le32_to_cpu(entry->e_value_size)) -
			(char *)xattr_ref->block_bh->b_data > block_size)
		ret = NULL;
	return ret;
}

static int ext4_xattr_block_fetch(struct ext4_xattr_ref *xattr_ref)
{
	int ret = 0;
	size_t size_rem;
	void *data;
	struct ext4_xattr_entry *entry = NULL;

	ASSERT(xattr_ref->block_bh->b_data);
	entry = EXT4_XATTR_BFIRST(xattr_ref->block_bh);

	size_rem = xattr_ref->fs->BlockSize;
	for (; size_rem > 0 && !EXT4_XATTR_IS_LAST_ENTRY(entry);
	     entry = EXT4_XATTR_NEXT(entry),
	     size_rem -= EXT4_XATTR_LEN(entry->e_name_len)) {
		struct ext4_xattr_item *item;
		char *e_name = EXT4_XATTR_NAME(entry);

		data = ext4_xattr_entry_data(xattr_ref, entry, FALSE);
		if (!data) {
			ret = -EIO;
			goto Finish;
		}

		item = ext4_xattr_item_alloc(entry->e_name_index, e_name,
					     (size_t)entry->e_name_len);
		if (!item) {
			ret = -ENOMEM;
			goto Finish;
		}
		if (ext4_xattr_item_alloc_data(
			item, data, le32_to_cpu(entry->e_value_size)) != 0) {
			ext4_xattr_item_free(item);
			ret = -ENOMEM;
			goto Finish;
		}
		ext4_xattr_item_insert(xattr_ref, item);
		xattr_ref->block_size_rem -=
			EXT4_XATTR_SIZE(item->data_size) +
			EXT4_XATTR_LEN(item->name_len);
		xattr_ref->ea_size += EXT4_XATTR_SIZE(item->data_size) +
				      EXT4_XATTR_LEN(item->name_len);
	}

Finish:
	return ret;
}

static int ext4_xattr_inode_fetch(struct ext4_xattr_ref *xattr_ref)
{
	void *data;
	size_t size_rem;
	int ret = 0;
	struct ext4_xattr_ibody_header *header = NULL;
	struct ext4_xattr_entry *entry = NULL;
	int inode_size = xattr_ref->fs->InodeSize;

	header = EXT4_XATTR_IHDR(xattr_ref->OnDiskInode);
	entry = EXT4_XATTR_IFIRST(header);

	size_rem = inode_size - EXT4_GOOD_OLD_INODE_SIZE -
		   xattr_ref->OnDiskInode->i_extra_isize;
	for (; size_rem > 0 && !EXT4_XATTR_IS_LAST_ENTRY(entry);
	     entry = EXT4_XATTR_NEXT(entry),
	     size_rem -= EXT4_XATTR_LEN(entry->e_name_len)) {
		struct ext4_xattr_item *item;
		char *e_name = EXT4_XATTR_NAME(entry);

		data = ext4_xattr_entry_data(xattr_ref, entry, TRUE);
		if (!data) {
			ret = -EIO;
			goto Finish;
		}

		item = ext4_xattr_item_alloc(entry->e_name_index, e_name,
					     (size_t)entry->e_name_len);
		if (!item) {
			ret = -ENOMEM;
			goto Finish;
		}
		if (ext4_xattr_item_alloc_data(
			item, data, le32_to_cpu(entry->e_value_size)) != 0) {
			ext4_xattr_item_free(item);
			ret = -ENOMEM;
			goto Finish;
		}
		item->in_inode = TRUE;
		ext4_xattr_item_insert(xattr_ref, item);
		xattr_ref->inode_size_rem -=
			EXT4_XATTR_SIZE(item->data_size) +
			EXT4_XATTR_LEN(item->name_len);
		xattr_ref->ea_size += EXT4_XATTR_SIZE(item->data_size) +
				      EXT4_XATTR_LEN(item->name_len);
	}

Finish:
	return ret;
}

static __s32 ext4_xattr_inode_space(struct ext4_xattr_ref *xattr_ref)
{
	int inode_size = xattr_ref->fs->InodeSize;
	int size_rem = inode_size - EXT4_GOOD_OLD_INODE_SIZE -
			    xattr_ref->OnDiskInode->i_extra_isize;
	return size_rem;
}

static __s32 ext4_xattr_block_space(struct ext4_xattr_ref *xattr_ref)
{
	return xattr_ref->fs->BlockSize;
}

static int ext4_xattr_fetch(struct ext4_xattr_ref *xattr_ref)
{
	int ret = 0;
	int inode_size = xattr_ref->fs->InodeSize;
	if (inode_size > EXT4_GOOD_OLD_INODE_SIZE) {
		ret = ext4_xattr_inode_fetch(xattr_ref);
		if (ret != 0)
			return ret;
	}

	if (xattr_ref->block_loaded)
		ret = ext4_xattr_block_fetch(xattr_ref);

	xattr_ref->dirty = FALSE;
	return ret;
}

static struct ext4_xattr_item *
ext4_xattr_lookup_item(struct ext4_xattr_ref *xattr_ref, __u8 name_index,
		       const char *name, size_t name_len)
{
	struct ext4_xattr_item tmp = {
		FALSE,
		FALSE,
		name_index,
		(char *)name, /*won't touch this string*/
		name_len,
	};
	if (name_index == EXT4_XATTR_INDEX_SYSTEM &&
	    name_len == 4 &&
	    !memcmp(name, "data", 4))
		tmp.is_data = TRUE;

	return ext4_xattr_item_search(xattr_ref, &tmp);
}

static struct ext4_xattr_item *
ext4_xattr_insert_item(struct ext4_xattr_ref *xattr_ref, __u8 name_index,
		       const char *name, size_t name_len, const void *data,
		       size_t data_size,
		       int *err)
{
	struct ext4_xattr_item *item;
	item = ext4_xattr_item_alloc(name_index, name, name_len);
	if (!item) {
		if (err)
			*err = -ENOMEM;

		return NULL;
	}

	item->in_inode = TRUE;
	if (xattr_ref->inode_size_rem <
	   EXT4_XATTR_SIZE(data_size) +
	   EXT4_XATTR_LEN(item->name_len)) {
		if (xattr_ref->block_size_rem <
		   EXT4_XATTR_SIZE(data_size) +
		   EXT4_XATTR_LEN(item->name_len)) {
			if (err)
				*err = -ENOSPC;

			return NULL;
		}

		item->in_inode = FALSE;
	}
	if (ext4_xattr_item_alloc_data(item, data, data_size) != 0) {
		ext4_xattr_item_free(item);
		if (err)
			*err = -ENOMEM;

		return NULL;
	}
	ext4_xattr_item_insert(xattr_ref, item);
	xattr_ref->ea_size +=
	    EXT4_XATTR_SIZE(item->data_size) + EXT4_XATTR_LEN(item->name_len);
	if (item->in_inode) {
		xattr_ref->inode_size_rem -=
			EXT4_XATTR_SIZE(item->data_size) +
			EXT4_XATTR_LEN(item->name_len);
	} else {
		xattr_ref->block_size_rem -=
			EXT4_XATTR_SIZE(item->data_size) +
			EXT4_XATTR_LEN(item->name_len);
	}
	xattr_ref->dirty = TRUE;
	if (err)
		*err = 0;

	return item;
}

static struct ext4_xattr_item *
ext4_xattr_insert_item_ordered(struct ext4_xattr_ref *xattr_ref, __u8 name_index,
	const char *name, size_t name_len, const void *data,
	size_t data_size,
	int *err)
{
	struct ext4_xattr_item *item, *last_item = NULL;
	item = ext4_xattr_item_alloc(name_index, name, name_len);
	if (!item) {
		if (err)
			*err = -ENOMEM;

		return NULL;
	}

	if (!list_empty(&xattr_ref->ordered_list))
		last_item = list_entry(xattr_ref->ordered_list.prev,
					struct ext4_xattr_item,
					list_node);

	item->in_inode = TRUE;
	if ((xattr_ref->inode_size_rem <
		EXT4_XATTR_SIZE(data_size) +
		EXT4_XATTR_LEN(item->name_len))
			||
		(last_item && !last_item->in_inode)) {
		if (xattr_ref->block_size_rem <
			EXT4_XATTR_SIZE(data_size) +
			EXT4_XATTR_LEN(item->name_len)) {
			if (err)
				*err = -ENOSPC;

			return NULL;
		}

		item->in_inode = FALSE;
	}
	if (ext4_xattr_item_alloc_data(item, data, data_size) != 0) {
		ext4_xattr_item_free(item);
		if (err)
			*err = -ENOMEM;

		return NULL;
	}
	ext4_xattr_item_insert(xattr_ref, item);
	xattr_ref->ea_size +=
		EXT4_XATTR_SIZE(item->data_size) + EXT4_XATTR_LEN(item->name_len);
	if (item->in_inode) {
		xattr_ref->inode_size_rem -=
			EXT4_XATTR_SIZE(item->data_size) +
			EXT4_XATTR_LEN(item->name_len);
	}
	else {
		xattr_ref->block_size_rem -=
			EXT4_XATTR_SIZE(item->data_size) +
			EXT4_XATTR_LEN(item->name_len);
	}
	xattr_ref->dirty = TRUE;
	if (err)
		*err = 0;

	return item;
}

static int ext4_xattr_remove_item(struct ext4_xattr_ref *xattr_ref,
				  __u8 name_index, const char *name,
				  size_t name_len)
{
	int ret = -ENOENT;
	struct ext4_xattr_item *item =
	    ext4_xattr_lookup_item(xattr_ref, name_index, name, name_len);
	if (item) {
		if (item == xattr_ref->iter_from) {
			struct rb_node *next_node;
			next_node = rb_next(&item->node);
			if (next_node)
				xattr_ref->iter_from =
					container_of(next_node,
						     struct ext4_xattr_item,
						     node);
			else
				xattr_ref->iter_from = NULL;
		}

		xattr_ref->ea_size -= EXT4_XATTR_SIZE(item->data_size) +
				      EXT4_XATTR_LEN(item->name_len);

		if (item->in_inode) {
			xattr_ref->inode_size_rem +=
				EXT4_XATTR_SIZE(item->data_size) +
				EXT4_XATTR_LEN(item->name_len);
		} else {
			xattr_ref->block_size_rem +=
				EXT4_XATTR_SIZE(item->data_size) +
				EXT4_XATTR_LEN(item->name_len);
		}

		ext4_xattr_item_remove(xattr_ref, item);
		ext4_xattr_item_free(item);
		xattr_ref->dirty = TRUE;
		ret = 0;
	}
	return ret;
}

static int ext4_xattr_resize_item(struct ext4_xattr_ref *xattr_ref,
				  struct ext4_xattr_item *item,
				  size_t new_data_size)
{
	int ret = 0;
	BOOL to_inode = FALSE, to_block = FALSE;
	size_t old_data_size = item->data_size;
	size_t orig_room_size = item->in_inode ?
		xattr_ref->inode_size_rem :
		xattr_ref->block_size_rem;

	/*
	 * Check if we can hold this entry in both in-inode and
	 * on-block form
	 *
	 * More complicated case: we do not allow entries stucking in
	 * the middle between in-inode space and on-block space, so
	 * the entry has to stay in either inode space or block space.
	 */
	if (item->in_inode) {
		if (xattr_ref->inode_size_rem +
			       EXT4_XATTR_SIZE(old_data_size) <
			       EXT4_XATTR_SIZE(new_data_size)) {
			if (xattr_ref->block_size_rem <
				       EXT4_XATTR_SIZE(new_data_size) +
				       EXT4_XATTR_LEN(item->name_len))
				return -ENOSPC;

			to_block = TRUE;
		}
	} else {
		if (xattr_ref->block_size_rem +
				EXT4_XATTR_SIZE(old_data_size) <
				EXT4_XATTR_SIZE(new_data_size)) {
			if (xattr_ref->inode_size_rem <
					EXT4_XATTR_SIZE(new_data_size) +
					EXT4_XATTR_LEN(item->name_len))
				return -ENOSPC;

			to_inode = TRUE;
		}
	}
	ret = ext4_xattr_item_resize_data(item, new_data_size);
	if (ret)
		return ret;

	xattr_ref->ea_size =
	    xattr_ref->ea_size -
	    EXT4_XATTR_SIZE(old_data_size) +
	    EXT4_XATTR_SIZE(new_data_size);

	/*
	 * This entry may originally lie in inode space or block space,
	 * and it is going to be transferred to another place.
	 */
	if (to_block) {
		xattr_ref->inode_size_rem +=
			EXT4_XATTR_SIZE(old_data_size) +
			EXT4_XATTR_LEN(item->name_len);
		xattr_ref->block_size_rem -=
			EXT4_XATTR_SIZE(new_data_size) +
			EXT4_XATTR_LEN(item->name_len);
		item->in_inode = FALSE;
	} else if (to_inode) {
		xattr_ref->block_size_rem +=
			EXT4_XATTR_SIZE(old_data_size) +
			EXT4_XATTR_LEN(item->name_len);
		xattr_ref->inode_size_rem -=
			EXT4_XATTR_SIZE(new_data_size) +
			EXT4_XATTR_LEN(item->name_len);
		item->in_inode = TRUE;
	} else {
		/*
		 * No need to transfer as there is enough space for the entry
		 * to stay in inode space or block space it used to be.
		 */
		orig_room_size +=
			EXT4_XATTR_SIZE(old_data_size);
		orig_room_size -=
			EXT4_XATTR_SIZE(new_data_size);
		if (item->in_inode)
			xattr_ref->inode_size_rem = orig_room_size;
		else
			xattr_ref->block_size_rem = orig_room_size;

	}
	xattr_ref->dirty = TRUE;
	return ret;
}

void ext4_xattr_purge_items(struct ext4_xattr_ref *xattr_ref)
{
	struct rb_node *first_node;
	struct ext4_xattr_item *item = NULL;
	first_node = rb_first(&xattr_ref->root);
	if (first_node)
		item = container_of(first_node, struct ext4_xattr_item,
				    node);

	while (item) {
		struct rb_node *next_node;
		struct ext4_xattr_item *next_item = NULL;
		next_node = rb_next(&item->node);
		if (next_node)
			next_item = container_of(next_node, struct ext4_xattr_item,
						 node);
		else
			next_item = NULL;

		ext4_xattr_item_remove(xattr_ref, item);
		ext4_xattr_item_free(item);

		item = next_item;
	}
	xattr_ref->ea_size = 0;
	if (ext4_xattr_inode_space(xattr_ref) <
	   sizeof(struct ext4_xattr_ibody_header))
		xattr_ref->inode_size_rem = 0;
	else
		xattr_ref->inode_size_rem =
		       ext4_xattr_inode_space(xattr_ref) -
		       sizeof(struct ext4_xattr_ibody_header);

	xattr_ref->block_size_rem =
		ext4_xattr_block_space(xattr_ref) -
		sizeof(struct ext4_xattr_header);
}

static int ext4_xattr_try_alloc_block(struct ext4_xattr_ref *xattr_ref)
{
	int ret = 0;

	ext4_fsblk_t xattr_block = 0;
	xattr_block = xattr_ref->inode_ref->Inode.i_file_acl;
	if (!xattr_block) {
		ext4_fsblk_t goal =
			ext4_inode_to_goal_block(&xattr_ref->inode_ref->Inode);

		xattr_block = ext4_new_meta_blocks(xattr_ref->IrpContext,
						  &xattr_ref->inode_ref->Inode,
					      goal, 0, NULL,
					      &ret);
		if (ret != 0)
			goto Finish;

		xattr_ref->block_bh = extents_bwrite(&xattr_ref->fs->sb, xattr_block);
		if (!xattr_ref->block_bh) {
			ext4_free_blocks(xattr_ref->IrpContext, &xattr_ref->inode_ref->Inode,
					       xattr_block, 1, 0);
			ret = -ENOMEM;
			goto Finish;
		}

		xattr_ref->inode_ref->Inode.i_file_acl = xattr_block;
		xattr_ref->IsOnDiskInodeDirty = TRUE;
		xattr_ref->block_loaded = TRUE;
	}

Finish:
	return ret;
}

static void ext4_xattr_try_free_block(struct ext4_xattr_ref *xattr_ref)
{
	ext4_fsblk_t xattr_block;
	xattr_block = xattr_ref->inode_ref->Inode.i_file_acl;
	xattr_ref->inode_ref->Inode.i_file_acl = 0;
	extents_brelse(xattr_ref->block_bh);
	xattr_ref->block_bh = NULL;
	ext4_free_blocks(xattr_ref->IrpContext, &xattr_ref->inode_ref->Inode,
		xattr_block, 1, 0);
	xattr_ref->IsOnDiskInodeDirty = TRUE;
	xattr_ref->block_loaded = FALSE;
}

static void ext4_xattr_set_block_header(struct ext4_xattr_ref *xattr_ref)
{
	struct ext4_xattr_header *block_header = NULL;
	block_header = EXT4_XATTR_BHDR(xattr_ref->block_bh);

	memset(block_header, 0, sizeof(struct ext4_xattr_header));
	block_header->h_magic = EXT4_XATTR_MAGIC;
	block_header->h_refcount = cpu_to_le32(1);
	block_header->h_blocks = cpu_to_le32(1);
}

static void
ext4_xattr_set_inode_entry(struct ext4_xattr_item *item,
			   struct ext4_xattr_ibody_header *ibody_header,
			   struct ext4_xattr_entry *entry, void *ibody_data_ptr)
{
	entry->e_name_len = (__u8)item->name_len;
	entry->e_name_index = item->name_index;
	entry->e_value_offs =
	   cpu_to_le16((char *)ibody_data_ptr - (char *)EXT4_XATTR_IFIRST(ibody_header));
	entry->e_value_block = 0;
	entry->e_value_size = cpu_to_le32(item->data_size);
}

static void ext4_xattr_set_block_entry(struct ext4_xattr_item *item,
				       struct ext4_xattr_header *block_header,
				       struct ext4_xattr_entry *block_entry,
				       void *block_data_ptr)
{
	block_entry->e_name_len = (__u8)item->name_len;
	block_entry->e_name_index = item->name_index;
	block_entry->e_value_offs =
	    cpu_to_le16((char *)block_data_ptr - (char *)block_header);
	block_entry->e_value_block = 0;
	block_entry->e_value_size = cpu_to_le32(item->data_size);
}

static int ext4_xattr_write_to_disk(struct ext4_xattr_ref *xattr_ref)
{
	int ret = 0;
	BOOL block_modified = FALSE;
	void *ibody_data = NULL;
	void *block_data = NULL;
	size_t inode_size_rem, block_size_rem;
	struct ext4_xattr_ibody_header *ibody_header = NULL;
	struct ext4_xattr_header *block_header = NULL;
	struct ext4_xattr_entry *entry = NULL;
	struct ext4_xattr_entry *block_entry = NULL;
	struct ext4_xattr_item *item = NULL;

	inode_size_rem = ext4_xattr_inode_space(xattr_ref);
	block_size_rem = ext4_xattr_block_space(xattr_ref);
	if (inode_size_rem > sizeof(struct ext4_xattr_ibody_header)) {
		ibody_header = EXT4_XATTR_IHDR(xattr_ref->OnDiskInode);
		entry = EXT4_XATTR_IFIRST(ibody_header);
	}

	if (!xattr_ref->dirty)
		goto Finish;
	/* If there are enough spaces in the ibody EA table.*/
	if (inode_size_rem > sizeof(struct ext4_xattr_ibody_header)) {
		memset(ibody_header, 0, inode_size_rem);
		ibody_header->h_magic = EXT4_XATTR_MAGIC;
		ibody_data = (char *)ibody_header + inode_size_rem;
		inode_size_rem -= sizeof(struct ext4_xattr_ibody_header);

		xattr_ref->IsOnDiskInodeDirty = TRUE;
	}
	/* If we need an extra block to hold the EA entries*/
	if (xattr_ref->ea_size > inode_size_rem) {
		if (!xattr_ref->block_loaded) {
			ret = ext4_xattr_try_alloc_block(xattr_ref);
			if (ret != 0)
				goto Finish;
		}
		memset(xattr_ref->block_bh->b_data, 0, xattr_ref->fs->BlockSize);
		block_header = EXT4_XATTR_BHDR(xattr_ref->block_bh);
		block_entry = EXT4_XATTR_BFIRST(xattr_ref->block_bh);
		ext4_xattr_set_block_header(xattr_ref);
		block_data = (char *)block_header + block_size_rem;
		block_size_rem -= sizeof(struct ext4_xattr_header);

		extents_mark_buffer_dirty(xattr_ref->block_bh);
	} else {
		/* We don't need an extra block.*/
		if (xattr_ref->block_loaded) {
			block_header = EXT4_XATTR_BHDR(xattr_ref->block_bh);
			le32_add_cpu(&block_header->h_refcount, -1);
			if (!block_header->h_refcount) {
				ext4_xattr_try_free_block(xattr_ref);
				block_header = NULL;
			} else {
				block_entry =
				    EXT4_XATTR_BFIRST(xattr_ref->block_bh);
				block_data =
				    (char *)block_header + block_size_rem;
				block_size_rem -=
				    sizeof(struct ext4_xattr_header);
				xattr_ref->inode_ref->Inode.i_file_acl = 0;

				xattr_ref->IsOnDiskInodeDirty = TRUE;
				extents_mark_buffer_dirty(xattr_ref->block_bh);
			}
		}
	}

	list_for_each_entry(item, &xattr_ref->ordered_list, struct ext4_xattr_item, list_node) {
		if (item->in_inode) {
			ibody_data = (char *)ibody_data -
				     EXT4_XATTR_SIZE(item->data_size);
			ext4_xattr_set_inode_entry(item, ibody_header, entry,
						   ibody_data);
			memcpy(EXT4_XATTR_NAME(entry), item->name,
			       item->name_len);
			memcpy(ibody_data, item->data, item->data_size);
			entry = EXT4_XATTR_NEXT(entry);
			inode_size_rem -= EXT4_XATTR_SIZE(item->data_size) +
					  EXT4_XATTR_LEN(item->name_len);

			xattr_ref->IsOnDiskInodeDirty = TRUE;
			continue;
		}
		if (EXT4_XATTR_SIZE(item->data_size) +
			EXT4_XATTR_LEN(item->name_len) >
		    block_size_rem) {
			ret = -ENOSPC;
			DbgPrint("ext4_xattr.c: IMPOSSIBLE -ENOSPC AS WE DID INSPECTION!\n");
			ASSERT(0);
		}
		block_data =
		    (char *)block_data - EXT4_XATTR_SIZE(item->data_size);
		ext4_xattr_set_block_entry(item, block_header, block_entry,
					   block_data);
		memcpy(EXT4_XATTR_NAME(block_entry), item->name,
		       item->name_len);
		memcpy(block_data, item->data, item->data_size);
		ext4_xattr_compute_hash(block_header, block_entry);
		block_entry = EXT4_XATTR_NEXT(block_entry);
		block_size_rem -= EXT4_XATTR_SIZE(item->data_size) +
				  EXT4_XATTR_LEN(item->name_len);

		block_modified = TRUE;
	}
	xattr_ref->dirty = FALSE;
	if (block_modified) {
		ext4_xattr_rehash(block_header,
				  EXT4_XATTR_BFIRST(xattr_ref->block_bh));
		ext4_xattr_set_block_checksum(xattr_ref->inode_ref,
					      xattr_ref->block_bh->b_blocknr,
					      block_header);
		extents_mark_buffer_dirty(xattr_ref->block_bh);
	}

Finish:
	return ret;
}

void ext4_fs_xattr_iterate(struct ext4_xattr_ref *ref,
			   int (*iter)(struct ext4_xattr_ref *ref,
				     struct ext4_xattr_item *item,
					 BOOL is_last))
{
	struct ext4_xattr_item *item;
	if (!ref->iter_from) {
		struct list_head *first_node;
		first_node = ref->ordered_list.next;
		if (first_node && first_node != &ref->ordered_list) {
			ref->iter_from =
				list_entry(first_node,
					     struct ext4_xattr_item,
					     list_node);
		}
	}

	item = ref->iter_from;
	while (item) {
		struct list_head *next_node;
		struct ext4_xattr_item *next_item;
		int ret = EXT4_XATTR_ITERATE_CONT;
		next_node = item->list_node.next;
		if (next_node && next_node != &ref->ordered_list)
			next_item = list_entry(next_node, struct ext4_xattr_item,
						 list_node);
		else
			next_item = NULL;
		if (iter)
			ret = iter(ref, item, !next_item);

		if (ret != EXT4_XATTR_ITERATE_CONT) {
			if (ret == EXT4_XATTR_ITERATE_STOP)
				ref->iter_from = NULL;

			break;
		}
		item = next_item;
	}
}

void ext4_fs_xattr_iterate_reset(struct ext4_xattr_ref *ref)
{
	ref->iter_from = NULL;
}

int ext4_fs_set_xattr(struct ext4_xattr_ref *ref, __u8 name_index,
		      const char *name, size_t name_len, const void *data,
		      size_t data_size, BOOL replace)
{
	int ret = 0;
	struct ext4_xattr_item *item =
	    ext4_xattr_lookup_item(ref, name_index, name, name_len);
	if (replace) {
		if (!item) {
			ret = -ENODATA;
			goto Finish;
		}
		if (item->data_size != data_size)
			ret = ext4_xattr_resize_item(ref, item, data_size);

		if (ret != 0) {
			goto Finish;
		}
		memcpy(item->data, data, data_size);
	} else {
		if (item) {
			ret = -EEXIST;
			goto Finish;
		}
		item = ext4_xattr_insert_item(ref, name_index, name, name_len,
					      data, data_size, &ret);
	}
Finish:
	return ret;
}

int ext4_fs_set_xattr_ordered(struct ext4_xattr_ref *ref, __u8 name_index,
	const char *name, size_t name_len, const void *data,
	size_t data_size)
{
	int ret = 0;
	struct ext4_xattr_item *item =
		ext4_xattr_lookup_item(ref, name_index, name, name_len);
	if (item) {
		ret = -EEXIST;
		goto Finish;
	}
	item = ext4_xattr_insert_item_ordered(ref, name_index, name, name_len,
		data, data_size, &ret);
Finish:
	return ret;
}

int ext4_fs_remove_xattr(struct ext4_xattr_ref *ref, __u8 name_index,
			 const char *name, size_t name_len)
{
	return ext4_xattr_remove_item(ref, name_index, name, name_len);
}

int ext4_fs_get_xattr(struct ext4_xattr_ref *ref, __u8 name_index,
		      const char *name, size_t name_len, void *buf,
		      size_t buf_size, size_t *data_size)
{
	int ret = 0;
	size_t item_size = 0;
	struct ext4_xattr_item *item =
	    ext4_xattr_lookup_item(ref, name_index, name, name_len);

	if (!item) {
		ret = -ENODATA;
		goto Finish;
	}
	item_size = item->data_size;
	if (buf_size > item_size)
		buf_size = item_size;

	if (buf)
		memcpy(buf, item->data, buf_size);

Finish:
	if (data_size)
		*data_size = item_size;

	return ret;
}

int ext4_fs_get_xattr_ref(PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB fs, PEXT2_MCB inode_ref,
			  struct ext4_xattr_ref *ref)
{
	int rc;
	ext4_fsblk_t xattr_block;
	xattr_block = inode_ref->Inode.i_file_acl;
	memset(&ref->root, 0, sizeof(struct rb_root));
	ref->ea_size = 0;
	ref->iter_from = NULL;
	if (xattr_block) {
		ref->block_bh = extents_bread(&fs->sb, xattr_block);
		if (!ref->block_bh)
			return -EIO;

		ref->block_loaded = TRUE;
	} else
		ref->block_loaded = FALSE;

	ref->inode_ref = inode_ref;
	ref->fs = fs;
	INIT_LIST_HEAD(&ref->ordered_list);

	ref->OnDiskInode = Ext2AllocateInode(fs);
	if (!ref->OnDiskInode) {
		if (xattr_block) {
			extents_brelse(ref->block_bh);
			ref->block_bh = NULL;
		}
		return -ENOMEM;
	}
	if (!Ext2LoadInodeXattr(fs, &inode_ref->Inode, ref->OnDiskInode)) {
		if (xattr_block) {
			extents_brelse(ref->block_bh);
			ref->block_bh = NULL;
		}

		Ext2DestroyInode(fs, ref->OnDiskInode);
		return -EIO;
	}
	ref->IsOnDiskInodeDirty = FALSE;

	if (ext4_xattr_inode_space(ref) <
	   sizeof(struct ext4_xattr_ibody_header) +
	   sizeof(__u32))
		ref->inode_size_rem = 0;
	else {
		ref->inode_size_rem =
			ext4_xattr_inode_space(ref) -
			sizeof(struct ext4_xattr_ibody_header);
	}

	ref->block_size_rem =
		ext4_xattr_block_space(ref) -
		sizeof(struct ext4_xattr_header) -
		sizeof(__u32);

	rc = ext4_xattr_fetch(ref);
	if (rc != 0) {
		ext4_xattr_purge_items(ref);
		if (xattr_block) {
			extents_brelse(ref->block_bh);
			ref->block_bh = NULL;
		}

		Ext2DestroyInode(fs, ref->OnDiskInode);
		return rc;
	}
	ref->IrpContext = IrpContext;
	return 0;
}

int ext4_fs_put_xattr_ref(struct ext4_xattr_ref *ref)
{
	int ret;
	sector_t orig_file_acl = ref->inode_ref->Inode.i_file_acl;
	ret = ext4_xattr_write_to_disk(ref);
	if (ref->IsOnDiskInodeDirty) {
		ASSERT(ref->fs->InodeSize > EXT4_GOOD_OLD_INODE_SIZE);

		/* As we may do block allocation in ext4_xattr_write_to_disk */
		if (ret)
			ref->inode_ref->Inode.i_file_acl = orig_file_acl;

		if (!ret) {
			ret = Ext2SaveInode(ref->IrpContext, ref->fs, &ref->inode_ref->Inode)
				? 0 : -EIO;
			if (!ret) {
				ret = Ext2SaveInodeXattr(ref->IrpContext,
						ref->fs,
						&ref->inode_ref->Inode,
						ref->OnDiskInode)
					? 0 : -EIO;
			}
		}
		ref->IsOnDiskInodeDirty = FALSE;
	}
	if (ref->block_loaded) {
		if (!ret)
			extents_brelse(ref->block_bh);
		else
			extents_bforget(ref->block_bh);

		ref->block_bh = NULL;
		ref->block_loaded = FALSE;
	}
	ext4_xattr_purge_items(ref);
	Ext2DestroyInode(ref->fs, ref->OnDiskInode);
	ref->OnDiskInode = NULL;
	ref->inode_ref = NULL;
	ref->fs = NULL;
	return ret;
}

struct xattr_prefix {
	const char *prefix;
	__u8 name_index;
};

static const struct xattr_prefix prefix_tbl[] = {
    {"user.", EXT4_XATTR_INDEX_USER},
    {"system.posix_acl_access", EXT4_XATTR_INDEX_POSIX_ACL_ACCESS},
    {"system.posix_acl_default", EXT4_XATTR_INDEX_POSIX_ACL_DEFAULT},
    {"trusted.", EXT4_XATTR_INDEX_TRUSTED},
    {"security.", EXT4_XATTR_INDEX_SECURITY},
    {"system.", EXT4_XATTR_INDEX_SYSTEM},
    {"system.richacl", EXT4_XATTR_INDEX_RICHACL},
    {NULL, 0},
};

const char *ext4_extract_xattr_name(const char *full_name, size_t full_name_len,
			      __u8 *name_index, size_t *name_len,
			      BOOL *found)
{
	int i;
	ASSERT(name_index);
	ASSERT(found);

	*found = FALSE;

	if (!full_name_len) {
		if (name_len)
			*name_len = 0;

		return NULL;
	}

	for (i = 0; prefix_tbl[i].prefix; i++) {
		size_t prefix_len = strlen(prefix_tbl[i].prefix);
		if (full_name_len >= prefix_len &&
		    !memcmp(full_name, prefix_tbl[i].prefix, prefix_len)) {
			BOOL require_name =
				prefix_tbl[i].prefix[prefix_len - 1] == '.';
			*name_index = prefix_tbl[i].name_index;
			if (name_len)
				*name_len = full_name_len - prefix_len;

			if (!(full_name_len - prefix_len) && require_name)
				return NULL;

			*found = TRUE;
			if (require_name)
				return full_name + prefix_len;

			return NULL;
		}
	}
	if (name_len)
		*name_len = 0;

	return NULL;
}

const char *ext4_get_xattr_name_prefix(__u8 name_index,
				       size_t *ret_prefix_len)
{
	int i;

	for (i = 0; prefix_tbl[i].prefix; i++) {
		size_t prefix_len = strlen(prefix_tbl[i].prefix);
		if (prefix_tbl[i].name_index == name_index) {
			if (ret_prefix_len)
				*ret_prefix_len = prefix_len;

			return prefix_tbl[i].prefix;
		}
	}
	if (ret_prefix_len)
		*ret_prefix_len = 0;

	return NULL;
}
