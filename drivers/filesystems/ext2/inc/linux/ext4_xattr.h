/*
 * Copyright (c) 2015 Grzegorz Kostka (kostka.grzegorz@gmail.com)
 * Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup lwext4
 * @{
 */
/**
 * @file  ext4_xattr.h
 * @brief Extended Attribute manipulation.
 */

#ifndef EXT4_XATTR_H_
#define EXT4_XATTR_H_

#include <ext2fs.h>
#include <linux/rbtree.h>

/* Extended Attribute(EA) */

/* Magic value in attribute blocks */
#define EXT4_XATTR_MAGIC		0xEA020000

/* Maximum number of references to one attribute block */
#define EXT4_XATTR_REFCOUNT_MAX		1024

/* Name indexes */
#define EXT4_XATTR_INDEX_USER			1
#define EXT4_XATTR_INDEX_POSIX_ACL_ACCESS	2
#define EXT4_XATTR_INDEX_POSIX_ACL_DEFAULT	3
#define EXT4_XATTR_INDEX_TRUSTED		4
#define	EXT4_XATTR_INDEX_LUSTRE			5
#define EXT4_XATTR_INDEX_SECURITY	        6
#define EXT4_XATTR_INDEX_SYSTEM			7
#define EXT4_XATTR_INDEX_RICHACL		8
#define EXT4_XATTR_INDEX_ENCRYPTION		9

#pragma pack(push, 1)

struct ext4_xattr_header {
	__le32 h_magic;	/* magic number for identification */
	__le32 h_refcount;	/* reference count */
	__le32 h_blocks;	/* number of disk blocks used */
	__le32 h_hash;		/* hash value of all attributes */
	__le32 h_checksum;	/* crc32c(uuid+id+xattrblock) */
				/* id = inum if refcount=1, blknum otherwise */
	__le32 h_reserved[3];	/* zero right now */
};

struct ext4_xattr_ibody_header {
	__le32 h_magic;	/* magic number for identification */
};

struct ext4_xattr_entry {
	__u8 e_name_len;	/* length of name */
	__u8 e_name_index;	/* attribute name index */
	__le16 e_value_offs;	/* offset in disk block of value */
	__le32 e_value_block;	/* disk block attribute is stored on (n/i) */
	__le32 e_value_size;	/* size of attribute value */
	__le32 e_hash;		/* hash value of name and value */
};

#pragma pack(pop)

#define EXT4_GOOD_OLD_INODE_SIZE	EXT2_GOOD_OLD_INODE_SIZE

#define EXT4_XATTR_PAD_BITS		2
#define EXT4_XATTR_PAD		(1<<EXT4_XATTR_PAD_BITS)
#define EXT4_XATTR_ROUND		(EXT4_XATTR_PAD-1)
#define EXT4_XATTR_LEN(name_len) \
	(((name_len) + EXT4_XATTR_ROUND + \
	sizeof(struct ext4_xattr_entry)) & ~EXT4_XATTR_ROUND)
#define EXT4_XATTR_NEXT(entry) \
	((struct ext4_xattr_entry *)( \
	 (char *)(entry) + EXT4_XATTR_LEN((entry)->e_name_len)))
#define EXT4_XATTR_SIZE(size) \
	(((size) + EXT4_XATTR_ROUND) & ~EXT4_XATTR_ROUND)
#define EXT4_XATTR_NAME(entry) \
	((char *)((entry) + 1))

#define EXT4_XATTR_IHDR(raw_inode) \
	((struct ext4_xattr_ibody_header *) \
		((char *)raw_inode + \
		EXT4_GOOD_OLD_INODE_SIZE + \
		(raw_inode)->i_extra_isize))
#define EXT4_XATTR_IFIRST(hdr) \
	((struct ext4_xattr_entry *)((hdr)+1))

#define EXT4_XATTR_BHDR(block) \
	((struct ext4_xattr_header *)((block)->b_data))
#define EXT4_XATTR_ENTRY(ptr) \
	((struct ext4_xattr_entry *)(ptr))
#define EXT4_XATTR_BFIRST(block) \
	EXT4_XATTR_ENTRY(EXT4_XATTR_BHDR(block)+1)
#define EXT4_XATTR_IS_LAST_ENTRY(entry) \
	(*(__le32 *)(entry) == 0)

#define EXT4_ZERO_XATTR_VALUE ((void *)-1)


struct ext4_xattr_item {
	/* This attribute should be stored in inode body */
	BOOL in_inode;
	BOOL is_data;

	__u8 name_index;
	char  *name;
	size_t name_len;
	void  *data;
	size_t data_size;

	struct rb_node node;
	struct list_head list_node;
};

struct ext4_xattr_ref {
	PEXT2_IRP_CONTEXT IrpContext;
	BOOL block_loaded;
	struct buffer_head *block_bh;
	PEXT2_MCB inode_ref;

	PEXT2_INODE OnDiskInode;
	BOOL IsOnDiskInodeDirty;

	BOOL   dirty;
	size_t ea_size;
	size_t inode_size_rem;
	size_t block_size_rem;
	PEXT2_VCB fs;

	void *iter_arg;
	struct ext4_xattr_item *iter_from;

	struct rb_root root;
	struct list_head ordered_list;
};

#define EXT4_XATTR_ITERATE_CONT 0
#define EXT4_XATTR_ITERATE_STOP 1
#define EXT4_XATTR_ITERATE_PAUSE 2

int ext4_fs_get_xattr_ref(PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB fs, PEXT2_MCB inode_ref,
	struct ext4_xattr_ref *ref);

int ext4_fs_put_xattr_ref(struct ext4_xattr_ref *ref);

int ext4_fs_set_xattr(struct ext4_xattr_ref *ref, __u8 name_index,
		      const char *name, size_t name_len, const void *data,
		      size_t data_size, BOOL replace);

int ext4_fs_set_xattr_ordered(struct ext4_xattr_ref *ref, __u8 name_index,
	const char *name, size_t name_len, const void *data,
	size_t data_size);

int ext4_fs_remove_xattr(struct ext4_xattr_ref *ref, __u8 name_index,
			 const char *name, size_t name_len);

int ext4_fs_get_xattr(struct ext4_xattr_ref *ref, __u8 name_index,
		      const char *name, size_t name_len, void *buf,
		      size_t buf_size, size_t *data_size);

void ext4_fs_xattr_iterate(struct ext4_xattr_ref *ref,
	int(*iter)(struct ext4_xattr_ref *ref,
		struct ext4_xattr_item *item,
		BOOL is_last));

void ext4_fs_xattr_iterate_reset(struct ext4_xattr_ref *ref);

const char *ext4_extract_xattr_name(const char *full_name, size_t full_name_len,
			      __u8 *name_index, size_t *name_len,
			      BOOL *found);

const char *ext4_get_xattr_name_prefix(__u8 name_index,
				       size_t *ret_prefix_len);

void ext4_xattr_purge_items(struct ext4_xattr_ref *xattr_ref);

#endif
/**
 * @}
 */
