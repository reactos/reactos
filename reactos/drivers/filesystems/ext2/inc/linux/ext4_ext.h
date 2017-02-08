#ifndef _LINUX_EXT4_EXT
#define _LINUX_EXT4_EXT

/*
 * This is the extent tail on-disk structure.
 * All other extent structures are 12 bytes long.  It turns out that
 * block_size % 12 >= 4 for at least all powers of 2 greater than 512, which
 * covers all valid ext4 block sizes.  Therefore, this tail structure can be
 * crammed into the end of the block without having to rebalance the tree.
 */
struct ext4_extent_tail {
	uint32_t et_checksum; /* crc32c(uuid+inum+extent_block) */
};

/*
 * This is the extent on-disk structure.
 * It's used at the bottom of the tree.
 */
typedef struct ext4_extent {
    uint32_t ee_block; /* first logical block extent covers */
    uint16_t ee_len; /* number of blocks covered by extent */
    uint16_t ee_start_hi; /* high 16 bits of physical block */
    uint32_t ee_start_lo; /* low 32 bits of physical block */
} __attribute__ ((__packed__)) EXT4_EXTENT;

/*
 * This is index on-disk structure.
 * It's used at all the levels except the bottom.
 */
typedef struct ext4_extent_idx {
    uint32_t  ei_block;       /* index covers logical blocks from 'block' */
    uint32_t  ei_leaf_lo;     /* pointer to the physical block of the next *
                                 * level. leaf or next index could be there */
    uint16_t  ei_leaf_hi;     /* high 16 bits of physical block */
    uint16_t   ei_unused;
}__attribute__ ((__packed__)) EXT4_EXTENT_IDX;

/*
 * Each block (leaves and indexes), even inode-stored has header.
 */
typedef struct ext4_extent_header {
    uint16_t  eh_magic;       /* probably will support different formats */
    uint16_t  eh_entries;     /* number of valid entries */
    uint16_t  eh_max;         /* capacity of store in entries */
    uint16_t  eh_depth;       /* has tree real underlying blocks? */
    uint32_t  eh_generation;  /* generation of the tree */
}__attribute__ ((__packed__)) EXT4_EXTENT_HEADER;


#define EXT4_EXT_MAGIC          0xf30a
#define get_ext4_header(i)      ((struct ext4_extent_header *) (i)->i_block)

#define EXT4_EXTENT_TAIL_OFFSET(hdr)                                           \
	(sizeof(struct ext4_extent_header) +                                   \
	 (sizeof(struct ext4_extent) * (hdr)->eh_max))

static inline struct ext4_extent_tail *
find_ext4_extent_tail(struct ext4_extent_header *eh)
{
	return (struct ext4_extent_tail *)(((char *)eh) +
					   EXT4_EXTENT_TAIL_OFFSET(eh));
}

/*
 * Array of ext4_ext_path contains path to some extent.
 * Creation/lookup routines use it for traversal/splitting/etc.
 * Truncate uses it to simulate recursive walking.
 */
struct ext4_ext_path
{
	ext4_fsblk_t p_block;
	int p_depth;
	int p_maxdepth;
	struct ext4_extent *p_ext;
	struct ext4_extent_idx *p_idx;
	struct ext4_extent_header *p_hdr;
	struct buffer_head *p_bh;
};

/*
 * structure for external API
 */

/*
 * EXT_INIT_MAX_LEN is the maximum number of blocks we can have in an
 * initialized extent. This is 2^15 and not (2^16 - 1), since we use the
 * MSB of ee_len field in the extent datastructure to signify if this
 * particular extent is an initialized extent or an uninitialized (i.e.
 * preallocated).
 * EXT_UNINIT_MAX_LEN is the maximum number of blocks we can have in an
 * uninitialized extent.
 * If ee_len is <= 0x8000, it is an initialized extent. Otherwise, it is an
 * uninitialized one. In other words, if MSB of ee_len is set, it is an
 * uninitialized extent with only one special scenario when ee_len = 0x8000.
 * In this case we can not have an uninitialized extent of zero length and
 * thus we make it as a special case of initialized extent with 0x8000 length.
 * This way we get better extent-to-group alignment for initialized extents.
 * Hence, the maximum number of blocks we can have in an *initialized*
 * extent is 2^15 (32768) and in an *uninitialized* extent is 2^15-1 (32767).
 */
#define EXT_INIT_MAX_LEN (1UL << 15)
#define EXT_UNWRITTEN_MAX_LEN	(EXT_INIT_MAX_LEN - 1)

#define EXT_EXTENT_SIZE sizeof(struct ext4_extent)
#define EXT_INDEX_SIZE sizeof(struct ext4_extent_idx)

#define EXT_FIRST_EXTENT(__hdr__)                                              \
	((struct ext4_extent *)(((char *)(__hdr__)) +                          \
				sizeof(struct ext4_extent_header)))
#define EXT_FIRST_INDEX(__hdr__)                                               \
	((struct ext4_extent_idx *)(((char *)(__hdr__)) +                      \
				    sizeof(struct ext4_extent_header)))
#define EXT_HAS_FREE_INDEX(__path__)                                           \
	((__path__)->p_hdr->eh_entries < (__path__)->p_hdr->eh_max)
#define EXT_LAST_EXTENT(__hdr__)                                               \
	(EXT_FIRST_EXTENT((__hdr__)) + (__hdr__)->eh_entries - 1)
#define EXT_LAST_INDEX(__hdr__)                                                \
	(EXT_FIRST_INDEX((__hdr__)) + (__hdr__)->eh_entries - 1)
#define EXT_MAX_EXTENT(__hdr__)                                                \
	(EXT_FIRST_EXTENT((__hdr__)) + (__hdr__)->eh_max - 1)
#define EXT_MAX_INDEX(__hdr__)                                                 \
	(EXT_FIRST_INDEX((__hdr__)) + (__hdr__)->eh_max - 1)

static inline struct ext4_extent_header *ext_inode_hdr(struct inode *inode)
{
	return get_ext4_header(inode);
}

static inline struct ext4_extent_header *ext_block_hdr(struct buffer_head *bh)
{
	return (struct ext4_extent_header *)bh->b_data;
}

static inline unsigned short ext_depth(struct inode *inode)
{
	return ext_inode_hdr(inode)->eh_depth;
}

static inline void ext4_ext_mark_uninitialized(struct ext4_extent *ext)
{
	/* We can not have an uninitialized extent of zero length! */
	ext->ee_len |= EXT_INIT_MAX_LEN;
}

static inline int ext4_ext_is_uninitialized(struct ext4_extent *ext)
{
	/* Extent with ee_len of 0x8000 is treated as an initialized extent */
	return (ext->ee_len > EXT_INIT_MAX_LEN);
}

static inline uint16_t ext4_ext_get_actual_len(struct ext4_extent *ext)
{
	return (ext->ee_len <= EXT_INIT_MAX_LEN
		    ? ext->ee_len
		    : (ext->ee_len - EXT_INIT_MAX_LEN));
}

static inline void ext4_ext_mark_initialized(struct ext4_extent *ext)
{
	ext->ee_len = ext4_ext_get_actual_len(ext);
}

static inline void ext4_ext_mark_unwritten(struct ext4_extent *ext)
{
	/* We can not have an unwritten extent of zero length! */
	ext->ee_len |= EXT_INIT_MAX_LEN;
}

static inline int ext4_ext_is_unwritten(struct ext4_extent *ext)
{
	/* Extent with ee_len of 0x8000 is treated as an initialized extent */
	return (ext->ee_len > EXT_INIT_MAX_LEN);
}

/*
 * ext4_ext_pblock:
 * combine low and high parts of physical block number into ext4_fsblk_t
 */
static inline ext4_fsblk_t ext4_ext_pblock(struct ext4_extent *ex)
{
	ext4_fsblk_t block;

	block = ex->ee_start_lo;
	block |= ((ext4_fsblk_t)ex->ee_start_hi << 31) << 1;
	return block;
}

/*
 * ext4_idx_pblock:
 * combine low and high parts of a leaf physical block number into ext4_fsblk_t
 */
static inline ext4_fsblk_t ext4_idx_pblock(struct ext4_extent_idx *ix)
{
	ext4_fsblk_t block;

	block = ix->ei_leaf_lo;
	block |= ((ext4_fsblk_t)ix->ei_leaf_hi << 31) << 1;
	return block;
}

/*
 * ext4_ext_store_pblock:
 * stores a large physical block number into an extent struct,
 * breaking it into parts
 */
static inline void ext4_ext_store_pblock(struct ext4_extent *ex,
					 ext4_fsblk_t pb)
{
	ex->ee_start_lo = (uint32_t)(pb & 0xffffffff);
	ex->ee_start_hi = (uint16_t)((pb >> 31) >> 1) & 0xffff;
}

/*
 * ext4_idx_store_pblock:
 * stores a large physical block number into an index struct,
 * breaking it into parts
 */
static inline void ext4_idx_store_pblock(struct ext4_extent_idx *ix,
					 ext4_fsblk_t pb)
{
	ix->ei_leaf_lo = (uint32_t)(pb & 0xffffffff);
	ix->ei_leaf_hi = (uint16_t)((pb >> 31) >> 1) & 0xffff;
}

#define ext4_ext_dirty(icb, handle, inode, path)                                           \
	__ext4_ext_dirty("", __LINE__, (icb), (handle), (inode), (path))

#define INODE_HAS_EXTENT(i) ((i)->i_flags & EXT2_EXTENTS_FL)

static inline uint64_t ext_to_block(EXT4_EXTENT *extent)
{
    uint64_t block;

    block = (uint64_t)extent->ee_start_lo;
    block |= ((uint64_t) extent->ee_start_hi << 31) << 1;

    return block;
}

static inline uint64_t idx_to_block(EXT4_EXTENT_IDX *idx)
{
    uint64_t block;

    block = (uint64_t)idx->ei_leaf_lo;
    block |= ((uint64_t) idx->ei_leaf_hi << 31) << 1;

    return block;
}


int ext4_ext_get_blocks(void *icb, handle_t *handle, struct inode *inode, ext4_fsblk_t iblock,
			unsigned long max_blocks, struct buffer_head *bh_result,
			int create, int flags);
int ext4_ext_tree_init(void *icb, handle_t *handle, struct inode *inode);
int ext4_ext_truncate(void *icb, struct inode *inode, unsigned long start);

#endif	/* _LINUX_EXT4_EXT */
