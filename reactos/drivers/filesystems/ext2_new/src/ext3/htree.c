/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             lock.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 *                   Copied from linux/lib/halfmd4.c
 *                               linux/fs/ext3/hash.c
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

#ifdef EXT2_HTREE_INDEX

#define DX_DEBUG 0

#if DX_DEBUG
#define dxtrace(command) command
#else
#define dxtrace(command)
#endif

#ifndef swap
#define swap(type, x, y) do { type z = x; x = y; y = z; } while (0)
#endif

/* F, G and H are basic MD4 functions: selection, majority, parity */
#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) (((x) & (y)) + (((x) ^ (y)) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

/*
 * The generic round function.  The application is so specific that
 * we don't bother protecting all the arguments with parens, as is generally
 * good macro practice, in favor of extra legibility.
 * Rotation is separate from addition to prevent recomputation
 */
#define ROUND(f, a, b, c, d, x, s)	\
	(a += f(b, c, d) + x, a = (a << s) | (a >> (32 - s)))
#define K1 0
#define K2 013240474631
#define K3 015666365641

/*
 * Basic cut-down MD4 transform.  Returns only 32 bits of result.
 */
__u32 half_md4_transform(__u32 buf[4], __u32 const in[8])
{
    __u32 a = buf[0], b = buf[1], c = buf[2], d = buf[3];

    /* Round 1 */
    ROUND(F, a, b, c, d, in[0] + K1,  3);
    ROUND(F, d, a, b, c, in[1] + K1,  7);
    ROUND(F, c, d, a, b, in[2] + K1, 11);
    ROUND(F, b, c, d, a, in[3] + K1, 19);
    ROUND(F, a, b, c, d, in[4] + K1,  3);
    ROUND(F, d, a, b, c, in[5] + K1,  7);
    ROUND(F, c, d, a, b, in[6] + K1, 11);
    ROUND(F, b, c, d, a, in[7] + K1, 19);

    /* Round 2 */
    ROUND(G, a, b, c, d, in[1] + K2,  3);
    ROUND(G, d, a, b, c, in[3] + K2,  5);
    ROUND(G, c, d, a, b, in[5] + K2,  9);
    ROUND(G, b, c, d, a, in[7] + K2, 13);
    ROUND(G, a, b, c, d, in[0] + K2,  3);
    ROUND(G, d, a, b, c, in[2] + K2,  5);
    ROUND(G, c, d, a, b, in[4] + K2,  9);
    ROUND(G, b, c, d, a, in[6] + K2, 13);

    /* Round 3 */
    ROUND(H, a, b, c, d, in[3] + K3,  3);
    ROUND(H, d, a, b, c, in[7] + K3,  9);
    ROUND(H, c, d, a, b, in[2] + K3, 11);
    ROUND(H, b, c, d, a, in[6] + K3, 15);
    ROUND(H, a, b, c, d, in[1] + K3,  3);
    ROUND(H, d, a, b, c, in[5] + K3,  9);
    ROUND(H, c, d, a, b, in[0] + K3, 11);
    ROUND(H, b, c, d, a, in[4] + K3, 15);

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;

    return buf[1]; /* "most hashed" word */
}

#define DELTA 0x9E3779B9

static void TEA_transform(__u32 buf[4], __u32 const in[])
{
    __u32	sum = 0;
    __u32	b0 = buf[0], b1 = buf[1];
    __u32	a = in[0], b = in[1], c = in[2], d = in[3];
    int	n = 16;

    do {
        sum += DELTA;
        b0 += ((b1 << 4)+a) ^ (b1+sum) ^ ((b1 >> 5)+b);
        b1 += ((b0 << 4)+c) ^ (b0+sum) ^ ((b0 >> 5)+d);
    } while (--n);

    buf[0] += b0;
    buf[1] += b1;
}


/* The old legacy hash */
static __u32 dx_hack_hash_unsigned(const char *name, int len)
{
	__u32 hash, hash0 = 0x12a3fe2d, hash1 = 0x37abe8f9;
	const unsigned char *ucp = (const unsigned char *) name;

	while (len--) {
		hash = hash1 + (hash0 ^ (((int) *ucp++) * 7152373));

		if (hash & 0x80000000)
			hash -= 0x7fffffff;
		hash1 = hash0;
		hash0 = hash;
	}
	return hash0 << 1;
}

static __u32 dx_hack_hash_signed(const char *name, int len)
{
	__u32 hash, hash0 = 0x12a3fe2d, hash1 = 0x37abe8f9;
	const signed char *scp = (const signed char *) name;

	while (len--) {
		hash = hash1 + (hash0 ^ (((int) *scp++) * 7152373));

		if (hash & 0x80000000)
			hash -= 0x7fffffff;
		hash1 = hash0;
		hash0 = hash;
	}
	return hash0 << 1;
}

static void str2hashbuf_signed(const char *msg, int len, __u32 *buf, int num)
{
	__u32	pad, val;
	int	i;
	const signed char *scp = (const signed char *) msg;

	pad = (__u32)len | ((__u32)len << 8);
	pad |= pad << 16;

	val = pad;
	if (len > num*4)
		len = num * 4;
	for (i = 0; i < len; i++) {
		if ((i % 4) == 0)
			val = pad;
		val = ((int) scp[i]) + (val << 8);
		if ((i % 4) == 3) {
			*buf++ = val;
			val = pad;
			num--;
		}
	}
	if (--num >= 0)
		*buf++ = val;
	while (--num >= 0)
		*buf++ = pad;
}

static void str2hashbuf_unsigned(const char *msg, int len, __u32 *buf, int num)
{
	__u32	pad, val;
	int	i;
	const unsigned char *ucp = (const unsigned char *) msg;

	pad = (__u32)len | ((__u32)len << 8);
	pad |= pad << 16;

	val = pad;
	if (len > num*4)
		len = num * 4;
	for (i = 0; i < len; i++) {
		if ((i % 4) == 0)
			val = pad;
		val = ((int) ucp[i]) + (val << 8);
		if ((i % 4) == 3) {
			*buf++ = val;
			val = pad;
			num--;
		}
	}
	if (--num >= 0)
		*buf++ = val;
	while (--num >= 0)
		*buf++ = pad;
}


#endif /* EXT2_HTREE_INDEX */

__u32 ext3_current_time(struct inode *in)
{
    LARGE_INTEGER   SysTime;
    KeQuerySystemTime(&SysTime);

    return Ext2LinuxTime(SysTime);
}

void ext3_warning (struct super_block * sb, const char * function,
                   char * fmt, ...)
{
#if DX_DEBUG
    va_list args;

    va_start(args, fmt);
    printk("EXT3-fs warning (device %s): %s: ",
           sb->s_id, function);
    printk(fmt, args);
    printk("\n");
    va_end(args);
#endif
}


/* ext3_bread is safe for meta-data blocks. it's not safe to read file data,
   since file data is managed by file cache, not volume cache */
struct buffer_head *ext3_bread(struct ext2_icb *icb, struct inode *inode,
                                           unsigned long block, int *err)
{
    struct buffer_head * bh = NULL;
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       lbn = 0, num = 0;

    PEXT2_MCB   Mcb = CONTAINING_RECORD(inode, EXT2_MCB, Inode);

    /* for symlink file, read it's target instead */
    if (NULL != Mcb && IsMcbSymLink(Mcb))
        Mcb = Mcb->Target;
    if (NULL == Mcb) {
        *err = -EINVAL;
        return NULL;
    }

    /* mapping file offset to ext2 block */
    if (INODE_HAS_EXTENT(&Mcb->Inode)) {
        status = Ext2MapExtent(icb, inode->i_sb->s_priv,
                               Mcb, block, FALSE,
                               &lbn, &num);
    } else {
        status = Ext2MapIndirect(icb, inode->i_sb->s_priv,
                                 Mcb, block, FALSE,
                                 &lbn, &num);
    }

    if (!NT_SUCCESS(status)) {
        *err = Ext2LinuxError(status);
        return bh;
    }

    bh = sb_getblk(inode->i_sb, lbn);
    if (!bh) {
        *err = -ENOMEM;
        return bh;
    }
    if (buffer_uptodate(bh))
        return bh;

    *err = bh_submit_read(bh);
    if (*err) {
	    brelse(bh);
	    return NULL;
    }
    return bh;
}

struct buffer_head *ext3_append(struct ext2_icb *icb, struct inode *inode,
                                            ext3_lblk_t *block, int *err)
{
    PEXT2_MCB   mcb = CONTAINING_RECORD(inode, EXT2_MCB, Inode);
    PEXT2_FCB   dcb = mcb->Fcb;
    NTSTATUS    status;

    ASSERT(dcb);
    ASSERT(inode == dcb->Inode);

    /* allocate new block since there's no space for us */
    *block = (ext3_lblk_t)(inode->i_size >> inode->i_sb->s_blocksize_bits);
    dcb->Header.AllocationSize.QuadPart += dcb->Vcb->BlockSize;
    status = Ext2ExpandFile(icb, dcb->Vcb, mcb, &(dcb->Header.AllocationSize));
    if (NT_SUCCESS(status)) {

        /* update Dcb */
        dcb->Header.ValidDataLength = dcb->Header.FileSize = dcb->Header.AllocationSize;
        mcb->Inode.i_size = dcb->Header.AllocationSize.QuadPart;

        /* save parent directory's inode */
        Ext2SaveInode(icb, dcb->Vcb, inode);
    }

    return ext3_bread(icb, inode, *block, err);
}


void ext3_inc_count(struct inode *inode)
{
    inode->i_nlink++;
}

void ext3_dec_count(struct inode *inode)
{
    inode->i_nlink--;
}

unsigned char ext3_type_by_mode(umode_t mode)
{
    unsigned char type = 0;

    switch (mode & S_IFMT) {
    case S_IFREG:
        type = EXT3_FT_REG_FILE;
        break;
    case S_IFDIR:
        type = EXT3_FT_DIR;
        break;
    case S_IFCHR:
        type =  EXT3_FT_CHRDEV;
        break;
    case S_IFBLK:
        type = EXT3_FT_BLKDEV;
        break;
    case S_IFIFO:
        type = EXT3_FT_FIFO;
        break;
    case S_IFSOCK:
        type = EXT3_FT_SOCK;
        break;
    case S_IFLNK:
        type = EXT3_FT_SYMLINK;
    }

    return type;
};

void ext3_set_de_type(struct super_block *sb,
                      struct ext3_dir_entry_2 *de,
                      umode_t mode)
{
    if (EXT3_HAS_INCOMPAT_FEATURE(sb, EXT3_FEATURE_INCOMPAT_FILETYPE))
        de->file_type = ext3_type_by_mode(mode);
}

/*
 * ext3_mark_inode_dirty is somewhat expensive, so unlike ext2 we
 * do not perform it in these functions.  We perform it at the call site,
 * if it is needed.
 */
int ext3_mark_inode_dirty(struct ext2_icb *icb, struct inode *in)
{
    if (Ext2SaveInode(icb, in->i_sb->s_priv, in))
        return 0;

    return -ENOMEM;
}

void ext3_update_dx_flag(struct inode *inode)
{
    if (!EXT3_HAS_COMPAT_FEATURE(inode->i_sb,
                                 EXT3_FEATURE_COMPAT_DIR_INDEX))
        EXT3_I(inode)->i_flags &= ~EXT3_INDEX_FL;
}

/*
 * Add a new entry into a directory (leaf) block.  If de is non-NULL,
 * it points to a directory entry which is guaranteed to be large
 * enough for new directory entry.  If de is NULL, then
 * add_dirent_to_buf will attempt search the directory block for
 * space.  It will return -ENOSPC if no space is available, and -EIO
 * and -EEXIST if directory entry already exists.
 *
 * NOTE!  bh is NOT released in the case where ENOSPC is returned.  In
 * all other cases bh is released.
 */
int add_dirent_to_buf(struct ext2_icb *icb, struct dentry *dentry,
                      struct inode *inode, struct ext3_dir_entry_2 *de,
                      struct buffer_head *bh)
{
    struct inode *dir = dentry->d_parent->d_inode;
    const char	*name = dentry->d_name.name;
    int		namelen = dentry->d_name.len;
    unsigned int	offset = 0;
    unsigned short	reclen;
    int		nlen, rlen, err;
    char		*top;

    reclen = EXT3_DIR_REC_LEN(namelen);
    if (!de) {
        de = (struct ext3_dir_entry_2 *)bh->b_data;
        top = bh->b_data + dir->i_sb->s_blocksize - reclen;
        while ((char *) de <= top) {
            if (!ext3_check_dir_entry("ext3_add_entry", dir, de,
                                      bh, offset)) {
                brelse(bh);
                return -EIO;
            }
            if (ext3_match(namelen, name, de)) {
                brelse(bh);
                return -EEXIST;
            }
            nlen = EXT3_DIR_REC_LEN(de->name_len);
            rlen = ext3_rec_len_from_disk(de->rec_len);
            if ((de->inode? rlen - nlen: rlen) >= reclen)
                break;
            de = (struct ext3_dir_entry_2 *)((char *)de + rlen);
            offset += rlen;
        }
        if ((char *) de > top)
            return -ENOSPC;
    }

    /* By now the buffer is marked for journaling */
    nlen = EXT3_DIR_REC_LEN(de->name_len);
    rlen = ext3_rec_len_from_disk(de->rec_len);
    if (de->inode) {
        struct ext3_dir_entry_2 *de1 = (struct ext3_dir_entry_2 *)((char *)de + nlen);
        de1->rec_len = ext3_rec_len_to_disk(rlen - nlen);
        de->rec_len = ext3_rec_len_to_disk(nlen);
        de = de1;
    }
    de->file_type = EXT3_FT_UNKNOWN;
    if (inode) {
        de->inode = cpu_to_le32(inode->i_ino);
        ext3_set_de_type(dir->i_sb, de, inode->i_mode);
    } else
        de->inode = 0;
    de->name_len = (__u8)namelen;
    memcpy(de->name, name, namelen);

    /*
     * XXX shouldn't update any times until successful
     * completion of syscall, but too many callers depend
     * on this.
     *
     * XXX similarly, too many callers depend on
     * ext4_new_inode() setting the times, but error
     * recovery deletes the inode, so the worst that can
     * happen is that the times are slightly out of date
     * and/or different from the directory change time.
     */
    dir->i_mtime = dir->i_ctime = ext3_current_time(dir);
    ext3_update_dx_flag(dir);
    dir->i_version++;
    ext3_mark_inode_dirty(icb, dir);
    set_buffer_dirty(bh);
    brelse(bh);
    return 0;
}

#ifdef EXT2_HTREE_INDEX

/*
 * Returns the hash of a filename.  If len is 0 and name is NULL, then
 * this function can be used to test whether or not a hash version is
 * supported.
 *
 * The seed is an 4 longword (32 bits) "secret" which can be used to
 * uniquify a hash.  If the seed is all zero's, then some default seed
 * may be used.
 *
 * A particular hash version specifies whether or not the seed is
 * represented, and whether or not the returned hash is 32 bits or 64
 * bits.  32 bit hashes will return 0 for the minor hash.
 */
int ext3_dirhash(const char *name, int len, struct dx_hash_info *hinfo)
{
    __u32	hash;
    __u32	minor_hash = 0;
    const char	*p;
    int		i;
    __u32 		in[8], buf[4];

	void		(*str2hashbuf)(const char *, int, __u32 *, int) =
				str2hashbuf_signed;

    /* Initialize the default seed for the hash checksum functions */
    buf[0] = 0x67452301;
    buf[1] = 0xefcdab89;
    buf[2] = 0x98badcfe;
    buf[3] = 0x10325476;

	/* Check to see if the seed is all zero's */
	if (hinfo->seed) {
		for (i = 0; i < 4; i++) {
			if (hinfo->seed[i])
				break;
		}
		if (i < 4)
			memcpy(buf, hinfo->seed, sizeof(buf));
	}

	switch (hinfo->hash_version) {
	case DX_HASH_LEGACY_UNSIGNED:
		hash = dx_hack_hash_unsigned(name, len);
		break;
	case DX_HASH_LEGACY:
		hash = dx_hack_hash_signed(name, len);
		break;
	case DX_HASH_HALF_MD4_UNSIGNED:
		str2hashbuf = str2hashbuf_unsigned;
	case DX_HASH_HALF_MD4:
		p = name;
		while (len > 0) {
			(*str2hashbuf)(p, len, in, 8);
			half_md4_transform(buf, in);
			len -= 32;
			p += 32;
		}
		minor_hash = buf[2];
		hash = buf[1];
		break;
	case DX_HASH_TEA_UNSIGNED:
		str2hashbuf = str2hashbuf_unsigned;
	case DX_HASH_TEA:
		p = name;
		while (len > 0) {
			(*str2hashbuf)(p, len, in, 4);
			TEA_transform(buf, in);
			len -= 16;
			p += 16;
		}
		hash = buf[0];
		minor_hash = buf[1];
		break;
	default:
		hinfo->hash = 0;
		return -1;
	}
	hash = hash & ~1;
	if (hash == (EXT4_HTREE_EOF_32BIT << 1))
		hash = (EXT4_HTREE_EOF_32BIT - 1) << 1;
	hinfo->hash = hash;
	hinfo->minor_hash = minor_hash;
	return 0;
}
EXPORT_SYMBOL(ext3_dirhash);


/*
 * These functions convert from the major/minor hash to an f_pos
 * value.
 *
 * Currently we only use major hash numer.  This is unfortunate, but
 * on 32-bit machines, the same VFS interface is used for lseek and
 * llseek, so if we use the 64 bit offset, then the 32-bit versions of
 * lseek/telldir/seekdir will blow out spectacularly, and from within
 * the ext2 low-level routine, we don't know if we're being called by
 * a 64-bit version of the system call or the 32-bit version of the
 * system call.  Worse yet, NFSv2 only allows for a 32-bit readdir
 * cookie.  Sigh.
 */
#define hash2pos(major, minor)	(major >> 1)
#define pos2maj_hash(pos)	((pos << 1) & 0xffffffff)
#define pos2min_hash(pos)	(0)

/*
 * This structure holds the nodes of the red-black tree used to store
 * the directory entry in hash order.
 */
struct fname {
    __u32		hash;
    __u32		minor_hash;
    struct rb_node	rb_hash;
    struct fname	*next;
    __u32		inode;
    __u8		name_len;
    __u8		file_type;
    char		name[0];
};

/*
 * This functoin implements a non-recursive way of freeing all of the
 * nodes in the red-black tree.
 */
static void free_rb_tree_fname(struct rb_root *root)
{
    struct rb_node	*n = root->rb_node;
    struct rb_node	*parent;
    struct fname	*fname;

    while (n) {
        /* Do the node's children first */
        if ((n)->rb_left) {
            n = n->rb_left;
            continue;
        }
        if (n->rb_right) {
            n = n->rb_right;
            continue;
        }
        /*
         * The node has no children; free it, and then zero
         * out parent's link to it.  Finally go to the
         * beginning of the loop and try to free the parent
         * node.
         */
        parent = rb_parent(n);
        fname = rb_entry(n, struct fname, rb_hash);
        while (fname) {
            struct fname * old = fname;
            fname = fname->next;
            kfree (old);
        }
        if (!parent)
            root->rb_node = NULL;
        else if (parent->rb_left == n)
            parent->rb_left = NULL;
        else if (parent->rb_right == n)
            parent->rb_right = NULL;
        n = parent;
    }
    root->rb_node = NULL;
}


static struct dir_private_info *create_dir_info(loff_t pos)
{
    struct dir_private_info *p;

    p = kmalloc(sizeof(struct dir_private_info), GFP_KERNEL);
    if (!p)
        return NULL;
    p->root.rb_node = NULL;
    p->curr_node = NULL;
    p->extra_fname = NULL;
    p->last_pos = 0;
    p->curr_hash = (__u32)pos2maj_hash(pos);
    p->curr_minor_hash = (__u32)pos2min_hash(pos);
    p->next_hash = 0;
    return p;
}

void ext3_htree_free_dir_info(struct dir_private_info *p)
{
    free_rb_tree_fname(&p->root);
    kfree(p);
}

/*
 * Given a directory entry, enter it into the fname rb tree.
 */
int ext3_htree_store_dirent(struct file *dir_file, __u32 hash,
                            __u32 minor_hash,
                            struct ext3_dir_entry_2 *dirent)
{
    struct rb_node **p, *parent = NULL;
    struct fname * fname, *new_fn;
    struct dir_private_info *info;
    int extra_data = 0;
    int len;

    info = (struct dir_private_info *) dir_file->private_data;
    p = &info->root.rb_node;

    /* Create and allocate the fname structure */
    if (dirent->file_type & EXT3_DIRENT_LUFID)
        extra_data = ext3_get_dirent_data_len(dirent);

    len = sizeof(struct fname) + dirent->name_len + extra_data;
    new_fn = kmalloc(len, GFP_KERNEL);
    if (!new_fn)
        return -ENOMEM;
    memset(new_fn, 0, len);
    new_fn->hash = hash;
    new_fn->minor_hash = minor_hash;
    new_fn->inode = le32_to_cpu(dirent->inode);
    new_fn->name_len = dirent->name_len;
    new_fn->file_type = dirent->file_type;
    memcpy(&new_fn->name[0], &dirent->name[0],
           dirent->name_len + extra_data);
    new_fn->name[dirent->name_len] = 0;

    while (*p) {
        parent = *p;
        fname = rb_entry(parent, struct fname, rb_hash);

        /*
         * If the hash and minor hash match up, then we put
         * them on a linked list.  This rarely happens...
         */
        if ((new_fn->hash == fname->hash) &&
                (new_fn->minor_hash == fname->minor_hash)) {
            new_fn->next = fname->next;
            fname->next = new_fn;
            return 0;
        }

        if (new_fn->hash < fname->hash)
            p = &(*p)->rb_left;
        else if (new_fn->hash > fname->hash)
            p = &(*p)->rb_right;
        else if (new_fn->minor_hash < fname->minor_hash)
            p = &(*p)->rb_left;
        else /* if (new_fn->minor_hash > fname->minor_hash) */
            p = &(*p)->rb_right;
    }

    rb_link_node(&new_fn->rb_hash, parent, p);
    rb_insert_color(&new_fn->rb_hash, &info->root);
    return 0;
}

static unsigned char ext3_filetype_table[] = {
    DT_UNKNOWN, DT_REG, DT_DIR, DT_CHR, DT_BLK, DT_FIFO, DT_SOCK, DT_LNK
};

static unsigned char get_dtype(struct super_block *sb, int filetype)
{
    if (!EXT3_HAS_INCOMPAT_FEATURE(sb, EXT3_FEATURE_INCOMPAT_FILETYPE) ||
            (filetype >= EXT3_FT_MAX))
        return DT_UNKNOWN;

    return (ext3_filetype_table[filetype]);
}

/*
 * This is a helper function for ext3_dx_readdir.  It calls filldir
 * for all entres on the fname linked list.  (Normally there is only
 * one entry on the linked list, unless there are 62 bit hash collisions.)
 */
static int call_filldir(struct file * filp, void * cookie,
                        filldir_t filldir, struct fname *fname)
{
    struct dir_private_info *info = filp->private_data;
    loff_t	curr_pos;
    struct inode *inode = filp->f_dentry->d_inode;
    struct super_block * sb;
    int error;

    sb = inode->i_sb;

    if (!fname) {
        printk("call_filldir: called with null fname?!?\n");
        return 0;
    }
    curr_pos = hash2pos(fname->hash, fname->minor_hash);
    while (fname) {
        error = filldir(cookie, fname->name,
                        fname->name_len, (ULONG)curr_pos,
                        fname->inode, get_dtype(sb, fname->file_type));
        if (error) {
            filp->f_pos = curr_pos;
            info->extra_fname = fname;
            return error;
        }
        fname = fname->next;
    }
    return 0;
}

struct fake_dirent
{
    __le32 inode;
    __le16 rec_len;
    __u8 name_len;
    __u8 file_type;
};

struct dx_countlimit
{
    __le16 limit;
    __le16 count;
};

struct dx_entry
{
    __le32 hash;
    __le32 block;
};

/*
 * dx_root_info is laid out so that if it should somehow get overlaid by a
 * dirent the two low bits of the hash version will be zero.  Therefore, the
 * hash version mod 4 should never be 0.  Sincerely, the paranoia department.
 */

struct dx_root
{
    struct fake_dirent dot;
    char dot_name[4];
    struct fake_dirent dotdot;
    char dotdot_name[4];
    struct dx_root_info
    {
        __le32 reserved_zero;
        __u8 hash_version;
        __u8 info_length; /* 8 */
        __u8 indirect_levels;
        __u8 unused_flags;
    }
    info;
    struct dx_entry	entries[0];
};

struct dx_node
{
    struct fake_dirent fake;
    struct dx_entry	entries[0];
};


struct dx_frame
{
    struct buffer_head *bh;
    struct dx_entry *entries;
    struct dx_entry *at;
};

struct dx_map_entry
{
    __u32 hash;
    __u16 offs;
    __u16 size;
};

#if defined(__REACTOS__) && !defined(_MSC_VER)
struct ext3_dir_entry_2 *
            do_split(struct ext2_icb *icb, struct inode *dir,
                     struct buffer_head **bh,struct dx_frame *frame,
                     struct dx_hash_info *hinfo, int *error);
#endif

/*
 * Future: use high four bits of block for coalesce-on-delete flags
 * Mask them off for now.
 */

static inline unsigned dx_get_block (struct dx_entry *entry)
{
    return le32_to_cpu(entry->block) & 0x00ffffff;
}

static inline void dx_set_block (struct dx_entry *entry, unsigned value)
{
    entry->block = cpu_to_le32(value);
}

static inline unsigned dx_get_hash (struct dx_entry *entry)
{
    return le32_to_cpu(entry->hash);
}

static inline void dx_set_hash (struct dx_entry *entry, unsigned value)
{
    entry->hash = cpu_to_le32(value);
}

static inline unsigned dx_get_count (struct dx_entry *entries)
{
    return le16_to_cpu(((struct dx_countlimit *) entries)->count);
}

static inline unsigned dx_get_limit (struct dx_entry *entries)
{
    return le16_to_cpu(((struct dx_countlimit *) entries)->limit);
}

static inline void dx_set_count (struct dx_entry *entries, unsigned value)
{
    ((struct dx_countlimit *) entries)->count = cpu_to_le16(value);
}

static inline void dx_set_limit (struct dx_entry *entries, unsigned value)
{
    ((struct dx_countlimit *) entries)->limit = cpu_to_le16(value);
}

static inline unsigned dx_root_limit (struct inode *dir, unsigned infosize)
{
    unsigned entry_space = dir->i_sb->s_blocksize - EXT3_DIR_REC_LEN(1) -
                           EXT3_DIR_REC_LEN(2) - infosize;
    return 0? 20: entry_space / sizeof(struct dx_entry);
}

static inline unsigned dx_node_limit (struct inode *dir)
{
    unsigned entry_space = dir->i_sb->s_blocksize - EXT3_DIR_REC_LEN(0);
    return 0? 22: entry_space / sizeof(struct dx_entry);
}

/*
 * Debug
 */
#if DX_DEBUG
static void dx_show_index (char * label, struct dx_entry *entries)
{
    int i, n = dx_get_count (entries);
    printk("%s index ", label);
    for (i = 0; i < n; i++)
    {
        printk("%x->%u ", i? dx_get_hash(entries + i): 0, dx_get_block(entries + i));
    }
    printk("\n");
}

struct stats
{
    unsigned names;
    unsigned space;
    unsigned bcount;
};

struct stats dx_show_leaf(struct ext2_icb *icb, struct dx_hash_info *hinfo,
                                      struct ext3_dir_entry_2 *de, int size, int show_names)
{
    struct stats rc;
    unsigned names = 0, space = 0;
    char *base = (char *) de;
    struct dx_hash_info h = *hinfo;

    printk("names: ");
    while ((char *) de < base + size)
    {
        if (de->inode)
        {
            if (show_names)
            {
                int len = de->name_len;
                char *name = de->name;
                while (len--) printk("%c", *name++);
                ext3_dirhash(de->name, de->name_len, &h);
                printk(":%x.%u ", h.hash,
                       ((char *) de - base));
            }
            space += EXT3_DIR_REC_LEN(de->name_len);
            names++;
        }
        de = (struct ext3_dir_entry_2 *) ((char *) de + le16_to_cpu(de->rec_len));
    }
    printk("(%i)\n", names);

    rc.names = names;
    rc.space = space;
    rc.bcount = 1;

    return rc;
}

struct stats dx_show_entries(struct ext2_icb *icb, struct dx_hash_info *hinfo,
                                         struct inode *dir, struct dx_entry *entries, int levels)
{
    unsigned blocksize = dir->i_sb->s_blocksize;
    unsigned count = dx_get_count (entries), names = 0, space = 0, i;
    unsigned bcount = 0;
    struct buffer_head *bh;
    struct stats rc;
    int err;

    printk("%i indexed blocks...\n", count);
    for (i = 0; i < count; i++, entries++)
    {
        u32 block = dx_get_block(entries), hash = i? dx_get_hash(entries): 0;
        u32 range = i < count - 1? (dx_get_hash(entries + 1) - hash): ~hash;
        struct stats stats;
        printk("%s%3u:%03u hash %8x/%8x ",levels?"":"   ", i, block, hash, range);
        if (!(bh = ext3_bread (icb, dir, block, &err))) continue;
        stats = levels?
                dx_show_entries(icb, hinfo, dir, ((struct dx_node *) bh->b_data)->entries, levels - 1):
                dx_show_leaf(icb, hinfo, (struct ext3_dir_entry_2 *) bh->b_data, blocksize, 0);
        names += stats.names;
        space += stats.space;
        bcount += stats.bcount;
        brelse (bh);
    }
    if (bcount)
        printk("%snames %u, fullness %u (%u%%)\n", levels?"":"   ",
               names, space/bcount,(space/bcount)*100/blocksize);

    rc.names = names;
    rc.space = space;
    rc.bcount = 1;

    return rc;
}
#endif /* DX_DEBUG */


int ext3_save_inode ( struct ext2_icb *icb, struct inode *in)
{
    return Ext2SaveInode(icb, in->i_sb->s_priv, in);
}

/*
 * Probe for a directory leaf block to search.
 *
 * dx_probe can return ERR_BAD_DX_DIR, which means there was a format
 * error in the directory index, and the caller should fall back to
 * searching the directory normally.  The callers of dx_probe **MUST**
 * check for this error code, and make sure it never gets reflected
 * back to userspace.
 */
static struct dx_frame *
            dx_probe(struct ext2_icb *icb, struct dentry *dentry, struct inode *dir,
                     struct dx_hash_info *hinfo, struct dx_frame *frame_in, int *err)
{
    unsigned count, indirect;
    struct dx_entry *at, *entries, *p, *q, *m;
    struct dx_root *root;
    struct buffer_head *bh;
    struct dx_frame *frame = frame_in;
    u32 hash;

    frame->bh = NULL;
    if (dentry)
        dir = dentry->d_parent->d_inode;
    if (!(bh = ext3_bread (icb, dir, 0, err)))
        goto fail;
    root = (struct dx_root *) bh->b_data;
    if (root->info.hash_version != DX_HASH_TEA &&
            root->info.hash_version != DX_HASH_HALF_MD4 &&
            root->info.hash_version != DX_HASH_LEGACY) {
        ext3_warning(dir->i_sb, __FUNCTION__,
                     "Unrecognised inode hash code %d",
                     root->info.hash_version);
        brelse(bh);
        *err = ERR_BAD_DX_DIR;
        goto fail;
    }
    hinfo->hash_version = root->info.hash_version;
    hinfo->seed = EXT3_SB(dir->i_sb)->s_hash_seed;
    if (dentry)
        ext3_dirhash(dentry->d_name.name, dentry->d_name.len, hinfo);
    hash = hinfo->hash;

    if (root->info.unused_flags & 1) {
        ext3_warning(dir->i_sb, __FUNCTION__,
                     "Unimplemented inode hash flags: %#06x",
                     root->info.unused_flags);
        brelse(bh);
        *err = ERR_BAD_DX_DIR;
        goto fail;
    }

    if ((indirect = root->info.indirect_levels) > 1) {
        ext3_warning(dir->i_sb, __FUNCTION__,
                     "Unimplemented inode hash depth: %#06x",
                     root->info.indirect_levels);
        brelse(bh);
        *err = ERR_BAD_DX_DIR;
        goto fail;
    }

    entries = (struct dx_entry *) (((char *)&root->info) +
                                   root->info.info_length);

    if (dx_get_limit(entries) != dx_root_limit(dir,
            root->info.info_length)) {
        ext3_warning(dir->i_sb, __FUNCTION__,
                     "dx entry: limit != root limit");
        brelse(bh);
        *err = ERR_BAD_DX_DIR;
        goto fail;
    }

    dxtrace(printk("Look up %x", hash));
    while (1)
    {
        count = dx_get_count(entries);
        if (!count || count > dx_get_limit(entries)) {
            ext3_warning(dir->i_sb, __FUNCTION__,
                         "dx entry: no count or count > limit");
            brelse(bh);
            *err = ERR_BAD_DX_DIR;
            goto fail2;
        }

        p = entries + 1;
        q = entries + count - 1;
        while (p <= q)
        {
            m = p + (q - p)/2;
            if (dx_get_hash(m) > hash)
                q = m - 1;
            else
                p = m + 1;
        }

        if (0) // linear search cross check
        {
            unsigned n = count - 1;
            at = entries;
            while (n--)
            {
                if (dx_get_hash(++at) > hash)
                {
                    at--;
                    break;
                }
            }
            ASSERT(at == p - 1);
        }

        at = p - 1;
        frame->bh = bh;
        frame->entries = entries;
        frame->at = at;
        if (!indirect--) return frame;
        if (!(bh = ext3_bread(icb, dir, dx_get_block(at), err)))
            goto fail2;
        at = entries = ((struct dx_node *) bh->b_data)->entries;
        if (dx_get_limit(entries) != dx_node_limit (dir)) {
            ext3_warning(dir->i_sb, __FUNCTION__,
                         "dx entry: limit != node limit");
            brelse(bh);
            *err = ERR_BAD_DX_DIR;
            goto fail2;
        }
        frame++;
        frame->bh = NULL;
    }
fail2:
    while (frame >= frame_in) {
        brelse(frame->bh);
        frame--;
    }
fail:
    if (*err == ERR_BAD_DX_DIR)
        ext3_warning(dir->i_sb, __FUNCTION__,
                     "Corrupt dir inode %ld, running e2fsck is "
                     "recommended.", dir->i_ino);
    return NULL;
}

static void dx_release (struct dx_frame *frames)
{
    if (frames[0].bh == NULL)
        return;

    if (((struct dx_root *) frames[0].bh->b_data)->info.indirect_levels)
        brelse(frames[1].bh);
    brelse(frames[0].bh);
}

/*
 * This function increments the frame pointer to search the next leaf
 * block, and reads in the necessary intervening nodes if the search
 * should be necessary.  Whether or not the search is necessary is
 * controlled by the hash parameter.  If the hash value is even, then
 * the search is only continued if the next block starts with that
 * hash value.  This is used if we are searching for a specific file.
 *
 * If the hash value is HASH_NB_ALWAYS, then always go to the next block.
 *
 * This function returns 1 if the caller should continue to search,
 * or 0 if it should not.  If there is an error reading one of the
 * index blocks, it will a negative error code.
 *
 * If start_hash is non-null, it will be filled in with the starting
 * hash of the next page.
 */
int ext3_htree_next_block(struct ext2_icb *icb, struct inode *dir,
                          __u32 hash, struct dx_frame *frame,
                          struct dx_frame *frames, __u32 *start_hash)
{
    struct dx_frame *p;
    struct buffer_head *bh;
    int err, num_frames = 0;
    __u32 bhash;

    p = frame;
    /*
     * Find the next leaf page by incrementing the frame pointer.
     * If we run out of entries in the interior node, loop around and
     * increment pointer in the parent node.  When we break out of
     * this loop, num_frames indicates the number of interior
     * nodes need to be read.
     */
    while (1) {
        if (++(p->at) < p->entries + dx_get_count(p->entries))
            break;
        if (p == frames)
            return 0;
        num_frames++;
        p--;
    }

    /*
     * If the hash is 1, then continue only if the next page has a
     * continuation hash of any value.  This is used for readdir
     * handling.  Otherwise, check to see if the hash matches the
     * desired contiuation hash.  If it doesn't, return since
     * there's no point to read in the successive index pages.
     */
    bhash = dx_get_hash(p->at);
    if (start_hash)
        *start_hash = bhash;
    if ((hash & 1) == 0) {
        if ((bhash & ~1) != hash)
            return 0;
    }
    /*
     * If the hash is HASH_NB_ALWAYS, we always go to the next
     * block so no check is necessary
     */
    while (num_frames--) {
        if (!(bh = ext3_bread(icb, dir, dx_get_block(p->at), &err)))
            return err; /* Failure */
        p++;
        brelse (p->bh);
        p->bh = bh;
        p->at = p->entries = ((struct dx_node *) bh->b_data)->entries;
    }
    return 1;
}

/*
 * This function fills a red-black tree with information from a
 * directory block.  It returns the number directory entries loaded
 * into the tree.  If there is an error it is returned in err.
 */
int htree_dirblock_to_tree(struct ext2_icb *icb, struct file *dir_file,
                           struct inode *dir, int block,
                           struct dx_hash_info *hinfo,
                           __u32 start_hash, __u32 start_minor_hash)
{
    struct buffer_head *bh;
    struct ext3_dir_entry_2 *de, *top;
    int err, count = 0;

    dxtrace(printk("In htree dirblock_to_tree: block %d\n", block));
    if (!(bh = ext3_bread (icb, dir, block, &err)))
        return err;

    de = (struct ext3_dir_entry_2 *) bh->b_data;
    top = (struct ext3_dir_entry_2 *) ((char *) de +
                                       dir->i_sb->s_blocksize -
                                       EXT3_DIR_REC_LEN(0));
    for (; de < top; de = ext3_next_entry(de)) {
        if (!ext3_check_dir_entry("htree_dirblock_to_tree", dir, de, bh,
                                  ((unsigned long)block<<EXT3_BLOCK_SIZE_BITS(dir->i_sb))
                                  + (unsigned long)((char *)de - bh->b_data))) {
            /* On error, skip the f_pos to the next block. */
            dir_file->f_pos = (dir_file->f_pos |
                               (dir->i_sb->s_blocksize - 1)) + 1;
            brelse (bh);
            return count;
        }
        ext3_dirhash(de->name, de->name_len, hinfo);
        if ((hinfo->hash < start_hash) ||
                ((hinfo->hash == start_hash) &&
                 (hinfo->minor_hash < start_minor_hash)))
            continue;
        if (de->inode == 0)
            continue;
        if ((err = ext3_htree_store_dirent(dir_file,
                                           hinfo->hash, hinfo->minor_hash, de)) != 0) {
            brelse(bh);
            return err;
        }
        count++;
    }
    brelse(bh);
    return count;
}

/*
 * This function fills a red-black tree with information from a
 * directory.  We start scanning the directory in hash order, starting
 * at start_hash and start_minor_hash.
 *
 * This function returns the number of entries inserted into the tree,
 * or a negative error code.
 */
int ext3_htree_fill_tree(struct ext2_icb *icb, struct file *dir_file,
                         __u32 start_hash, __u32 start_minor_hash,
                         __u32 *next_hash)
{
    struct dx_hash_info hinfo;
    struct ext3_dir_entry_2 *de;
    struct dx_frame frames[2], *frame;
    int block, err = 0;
    struct inode *dir;
    int count = 0;
    int ret;
    __u32 hashval;

    dxtrace(printk("In htree_fill_tree, start hash: %x:%x\n", start_hash,
                   start_minor_hash));
    dir = dir_file->f_dentry->d_inode;
    if (!(EXT3_I(dir)->i_flags & EXT3_INDEX_FL)) {
        hinfo.hash_version = EXT3_SB(dir->i_sb)->s_def_hash_version;
        hinfo.seed = EXT3_SB(dir->i_sb)->s_hash_seed;
        count = htree_dirblock_to_tree(icb, dir_file, dir, 0, &hinfo,
                                       start_hash, start_minor_hash);
        *next_hash = ~0;
        return count;
    }

    hinfo.hash = start_hash;
    hinfo.minor_hash = 0;
    frame = dx_probe(icb, NULL, dir_file->f_dentry->d_inode, &hinfo, frames, &err);
    if (!frame)
        return err;

    /* Add '.' and '..' from the htree header */
    if (!start_hash && !start_minor_hash) {
        de = (struct ext3_dir_entry_2 *) frames[0].bh->b_data;
        if ((err = ext3_htree_store_dirent(dir_file, 0, 0, de)) != 0)
            goto errout;
        count++;
    }
    if (start_hash < 2 || (start_hash ==2 && start_minor_hash==0)) {
        de = (struct ext3_dir_entry_2 *) frames[0].bh->b_data;
        de = ext3_next_entry(de);
        if ((err = ext3_htree_store_dirent(dir_file, 2, 0, de)) != 0)
            goto errout;
        count++;
    }

    while (1) {
        block = dx_get_block(frame->at);
        ret = htree_dirblock_to_tree(icb, dir_file, dir, block, &hinfo,
                                     start_hash, start_minor_hash);
        if (ret < 0) {
            err = ret;
            goto errout;
        }
        count += ret;
        hashval = ~0;
        ret = ext3_htree_next_block(icb, dir, HASH_NB_ALWAYS,
                                    frame, frames, &hashval);
        *next_hash = hashval;
        if (ret < 0) {
            err = ret;
            goto errout;
        }
        /*
         * Stop if:  (a) there are no more entries, or
         * (b) we have inserted at least one entry and the
         * next hash value is not a continuation
         */
        if ((ret == 0) ||
                (count && ((hashval & 1) == 0)))
            break;
    }
    dx_release(frames);
    dxtrace(printk("Fill tree: returned %d entries, next hash: %x\n",
                   count, *next_hash));
    return count;
errout:
    dx_release(frames);

    return (err);
}


/*
 * Directory block splitting, compacting
 */

/*
 * Create map of hash values, offsets, and sizes, stored at end of block.
 * Returns number of entries mapped.
 */
static int dx_make_map (struct ext3_dir_entry_2 *de, int size,
                        struct dx_hash_info *hinfo, struct dx_map_entry *map_tail)
{
    int count = 0;
    char *base = (char *) de;
    struct dx_hash_info h = *hinfo;

    while ((char *) de < base + size)
    {
        if (de->name_len && de->inode) {
            ext3_dirhash(de->name, de->name_len, &h);
            map_tail--;
            map_tail->hash = h.hash;
            map_tail->offs = (u16) ((char *) de - base);
            map_tail->size = le16_to_cpu(de->rec_len);
            count++;
            cond_resched();
        }
        /* XXX: do we need to check rec_len == 0 case? -Chris */
        de = (struct ext3_dir_entry_2 *) ((char *) de + le16_to_cpu(de->rec_len));
    }
    return count;
}

/* Sort map by hash value */
static void dx_sort_map (struct dx_map_entry *map, unsigned count)
{
    struct dx_map_entry *p, *q, *top = map + count - 1;
    int more;
    /* Combsort until bubble sort doesn't suck */
    while (count > 2)
    {
        count = count*10/13;
        if (count - 9 < 2) /* 9, 10 -> 11 */
            count = 11;
        for (p = top, q = p - count; q >= map; p--, q--)
            if (p->hash < q->hash)
                swap(struct dx_map_entry, *p, *q);
    }
    /* Garden variety bubble sort */
    do {
        more = 0;
        q = top;
        while (q-- > map)
        {
            if (q[1].hash >= q[0].hash)
                continue;
            swap(struct dx_map_entry, *(q+1), *q);
            more = 1;
        }
    } while (more);
}

static void dx_insert_block(struct dx_frame *frame, u32 hash, u32 block)
{
    struct dx_entry *entries = frame->entries;
    struct dx_entry *old = frame->at, *new = old + 1;
    unsigned int count = dx_get_count(entries);

    ASSERT(count < dx_get_limit(entries));
    ASSERT(old < entries + count);
    memmove(new + 1, new, (char *)(entries + count) - (char *)(new));
    dx_set_hash(new, hash);
    dx_set_block(new, block);
    dx_set_count(entries, count + 1);
}

struct buffer_head *
            ext3_dx_find_entry(struct ext2_icb *icb, struct dentry *dentry,
                               struct ext3_dir_entry_2 **res_dir, int *err)
{
    struct super_block * sb;
    struct dx_hash_info	hinfo = {0};
    u32 hash;
    struct dx_frame frames[2], *frame;
    struct ext3_dir_entry_2 *de, *top;
    struct buffer_head *bh;
    unsigned long block;
    int retval;
    int namelen = dentry->d_name.len;
    const __u8 *name = dentry->d_name.name;
    struct inode *dir = dentry->d_parent->d_inode;

    sb = dir->i_sb;
    /* NFS may look up ".." - look at dx_root directory block */
    if (namelen > 2 || name[0] != '.'||(name[1] != '.' && name[1] != '\0')) {
        if (!(frame = dx_probe(icb, dentry, NULL, &hinfo, frames, err)))
            return NULL;
    } else {
        frame = frames;
        frame->bh = NULL;			/* for dx_release() */
        frame->at = (struct dx_entry *)frames;	/* hack for zero entry*/
        dx_set_block(frame->at, 0);		/* dx_root block is 0 */
    }
    hash = hinfo.hash;
    do {
        block = dx_get_block(frame->at);
        if (!(bh = ext3_bread (icb, dir, block, err)))
            goto errout;
        de = (struct ext3_dir_entry_2 *) bh->b_data;
        top = (struct ext3_dir_entry_2 *) ((char *) de + sb->s_blocksize -
                                           EXT3_DIR_REC_LEN(0));
        for (; de < top; de = ext3_next_entry(de))
            if (ext3_match (namelen, name, de)) {
                if (!ext3_check_dir_entry("ext3_find_entry",
                                          dir, de, bh,
                                          (block<<EXT3_BLOCK_SIZE_BITS(sb))
                                          + (unsigned long)((char *)de - bh->b_data))) {
                    brelse (bh);
                    goto errout;
                }
                *res_dir = de;
                dx_release (frames);
                return bh;
            }
        brelse (bh);
        /* Check to see if we should continue to search */
        retval = ext3_htree_next_block(icb, dir, hash, frame,
                                       frames, NULL);
        if (retval < 0) {
            ext3_warning(sb, __FUNCTION__,
                         "error reading index page in directory #%lu",
                         dir->i_ino);
            *err = retval;
            goto errout;
        }
    } while (retval == 1);

    *err = -ENOENT;
errout:
    dxtrace(printk("%s not found\n", name));
    dx_release (frames);
    return NULL;
}

int ext3_dx_readdir(struct file *filp, filldir_t filldir,
                    void * context)
{
    struct dir_private_info *info = filp->private_data;
    struct inode *inode = filp->f_dentry->d_inode;
    struct fname *fname;
    PEXT2_FILLDIR_CONTEXT fc = context;
    int	ret;

    if (!info) {
        info = create_dir_info(filp->f_pos);
        if (!info)
            return -ENOMEM;
        filp->private_data = info;
    }

    if (filp->f_pos == EXT3_HTREE_EOF)
        return 0;	/* EOF */

    /* Some one has messed with f_pos; reset the world */
    if (info->last_pos != filp->f_pos) {
        free_rb_tree_fname(&info->root);
        info->curr_node = NULL;
        info->extra_fname = NULL;
        info->curr_hash = (__u32)pos2maj_hash(filp->f_pos);
        info->curr_minor_hash = (__u32)pos2min_hash(filp->f_pos);
    }

    /*
     * If there are any leftover names on the hash collision
     * chain, return them first.
     */
    if (info->extra_fname) {
        if (call_filldir(filp, context, filldir, info->extra_fname))
            goto finished;
        info->extra_fname = NULL;
        goto next_node;
    } else if (!info->curr_node)
        info->curr_node = rb_first(&info->root);

    while (1) {
        /*
         * Fill the rbtree if we have no more entries,
         * or the inode has changed since we last read in the
         * cached entries.
         */
        if ((!info->curr_node) ||
                (filp->f_version != inode->i_version)) {
            info->curr_node = NULL;
            free_rb_tree_fname(&info->root);
            filp->f_version = inode->i_version;
            ret = ext3_htree_fill_tree(fc->efc_irp, filp, info->curr_hash,
                                       info->curr_minor_hash, &info->next_hash);
            if (ret < 0)
                return ret;
            if (ret == 0) {
                filp->f_pos = EXT3_HTREE_EOF;
                break;
            }
            info->curr_node = rb_first(&info->root);
        }

        fname = rb_entry(info->curr_node, struct fname, rb_hash);
        info->curr_hash = fname->hash;
        info->curr_minor_hash = fname->minor_hash;
        if (call_filldir(filp, context, filldir, fname))
            break;
next_node:
        info->curr_node = rb_next(info->curr_node);
        if (info->curr_node) {
            fname = rb_entry(info->curr_node, struct fname,
                             rb_hash);
            info->curr_hash = fname->hash;
            info->curr_minor_hash = fname->minor_hash;
        } else {
            if (info->next_hash == ~0) {
                filp->f_pos = EXT3_HTREE_EOF;
                break;
            }
            info->curr_hash = info->next_hash;
            info->curr_minor_hash = 0;
        }
    }
finished:
    info->last_pos = filp->f_pos;
    return 0;
}

int ext3_release_dir (struct inode * inode, struct file * filp)
{
    if (filp->private_data) {
        ext3_htree_free_dir_info(filp->private_data);
        filp->private_data = NULL;
    }

    return 0;
}

/*
 * Returns 0 for success, or a negative error value
 */
int ext3_dx_add_entry(struct ext2_icb *icb, struct dentry *dentry,
                      struct inode *inode)
{
    struct dx_frame frames[2], *frame;
    struct dx_entry *entries, *at;
    struct dx_hash_info hinfo;
    struct buffer_head * bh;
    struct inode *dir = dentry->d_parent->d_inode;
    struct super_block * sb = dir->i_sb;
    struct ext3_dir_entry_2 *de;
    int err;

    frame = dx_probe(icb, dentry, NULL, &hinfo, frames, &err);
    if (!frame)
        return err;
    entries = frame->entries;
    at = frame->at;

    if (!(bh = ext3_bread(icb, dir, dx_get_block(frame->at), &err)))
        goto cleanup;

    err = add_dirent_to_buf(icb, dentry, inode, NULL, bh);
    if (err != -ENOSPC) {
        bh = NULL;
        goto cleanup;
    }

    /* Block full, should compress but for now just split */
    dxtrace(printk("using %u of %u node entries\n",
                   dx_get_count(entries), dx_get_limit(entries)));
    /* Need to split index? */
    if (dx_get_count(entries) == dx_get_limit(entries)) {
        u32 newblock;
        unsigned icount = dx_get_count(entries);
        int levels = (int)(frame - frames);
        struct dx_entry *entries2;
        struct dx_node *node2;
        struct buffer_head *bh2;

        if (levels && (dx_get_count(frames->entries) ==
                       dx_get_limit(frames->entries))) {
            ext3_warning(sb, __FUNCTION__,
                         "Directory index full!");
            err = -ENOSPC;
            goto cleanup;
        }
        bh2 = ext3_append (icb, dir, &newblock, &err);
        if (!(bh2))
            goto cleanup;
        node2 = (struct dx_node *)(bh2->b_data);
        entries2 = node2->entries;
        node2->fake.rec_len = cpu_to_le16(sb->s_blocksize);
        node2->fake.inode = 0;

        if (levels) {
            unsigned icount1 = icount/2, icount2 = icount - icount1;
            unsigned hash2 = dx_get_hash(entries + icount1);
            dxtrace(printk("Split index %i/%i\n", icount1, icount2));

            memcpy ((char *) entries2, (char *) (entries + icount1),
                    icount2 * sizeof(struct dx_entry));
            dx_set_count (entries, icount1);
            dx_set_count (entries2, icount2);
            dx_set_limit (entries2, dx_node_limit(dir));

            /* Which index block gets the new entry? */
            if ((unsigned int)(at - entries) >= icount1) {
                frame->at = at = at - entries - icount1 + entries2;
                frame->entries = entries = entries2;
                swap(struct buffer_head *, frame->bh, bh2);
            }
            dx_insert_block (frames + 0, hash2, newblock);
            dxtrace(dx_show_index ("node", frames[1].entries));
            dxtrace(dx_show_index ("node",
                                   ((struct dx_node *) bh2->b_data)->entries));
            set_buffer_dirty(bh2);
            brelse (bh2);
        } else {
            dxtrace(printk("Creating second level index...\n"));
            memcpy((char *) entries2, (char *) entries,
                   icount * sizeof(struct dx_entry));
            dx_set_limit(entries2, dx_node_limit(dir));

            /* Set up root */
            dx_set_count(entries, 1);
            dx_set_block(entries + 0, newblock);
            ((struct dx_root *) frames[0].bh->b_data)->info.indirect_levels = 1;

            /* Add new access path frame */
            frame = frames + 1;
            frame->at = at = at - entries + entries2;
            frame->entries = entries = entries2;
            frame->bh = bh2;
        }
        // ext3_journal_dirty_metadata(handle, frames[0].bh);
        set_buffer_dirty(frames[0].bh);
    }
    de = do_split(icb, dir, &bh, frame, &hinfo, &err);
    if (!de)
        goto cleanup;
    err = add_dirent_to_buf(icb, dentry, inode, de, bh);
    bh = NULL;
    goto cleanup;

cleanup:
    if (bh)
        brelse(bh);
    dx_release(frames);
    return err;
}

/*
 * Move count entries from end of map between two memory locations.
 * Returns pointer to last entry moved.
 */
struct ext3_dir_entry_2 *
            dx_move_dirents(char *from, char *to, struct dx_map_entry *map, int count)
{
    unsigned rec_len = 0;

    while (count--) {
        struct ext3_dir_entry_2 *de = (struct ext3_dir_entry_2 *) (from + map->offs);
        rec_len = EXT3_DIR_REC_LEN(de->name_len);
        memcpy (to, de, rec_len);
        ((struct ext3_dir_entry_2 *) to)->rec_len =
            cpu_to_le16(rec_len);
        de->inode = 0;
        map++;
        to += rec_len;
    }
    return (struct ext3_dir_entry_2 *) (to - rec_len);
}

/*
 * Compact each dir entry in the range to the minimal rec_len.
 * Returns pointer to last entry in range.
 */
struct ext3_dir_entry_2* dx_pack_dirents(char *base, int size)
{
    struct ext3_dir_entry_2 *next, *to, *prev, *de = (struct ext3_dir_entry_2 *) base;
    unsigned rec_len = 0;

    prev = to = de;
    while ((char*)de < base + size) {
        next = (struct ext3_dir_entry_2 *) ((char *) de +
                                            le16_to_cpu(de->rec_len));
        if (de->inode && de->name_len) {
            rec_len = EXT3_DIR_REC_LEN(de->name_len);
            if (de > to)
                memmove(to, de, rec_len);
            to->rec_len = cpu_to_le16(rec_len);
            prev = to;
            to = (struct ext3_dir_entry_2 *) (((char *) to) + rec_len);
        }
        de = next;
    }
    return prev;
}

/*
 * Split a full leaf block to make room for a new dir entry.
 * Allocate a new block, and move entries so that they are approx. equally full.
 * Returns pointer to de in block into which the new entry will be inserted.
 */
struct ext3_dir_entry_2 *
            do_split(struct ext2_icb *icb, struct inode *dir,
                     struct buffer_head **bh,struct dx_frame *frame,
                     struct dx_hash_info *hinfo, int *error)
{
    unsigned blocksize = dir->i_sb->s_blocksize;
    unsigned count, continued;
    struct buffer_head *bh2;
    u32 newblock;
    u32 hash2;
    struct dx_map_entry *map;
    char *data1 = (*bh)->b_data, *data2;
    unsigned split, move, size;
    struct ext3_dir_entry_2 *de = NULL, *de2;
    int	err, i;

    bh2 = ext3_append (icb, dir, &newblock, error);
    if (!(bh2)) {
        brelse(*bh);
        *bh = NULL;
        goto errout;
    }

    data2 = bh2->b_data;

    /* create map in the end of data2 block */
    map = (struct dx_map_entry *) (data2 + blocksize);
    count = dx_make_map ((struct ext3_dir_entry_2 *) data1,
                         blocksize, hinfo, map);
    map -= count;
    dx_sort_map (map, count);
    /* Split the existing block in the middle, size-wise */
    size = 0;
    move = 0;
    for (i = count-1; i >= 0; i--) {
        /* is more than half of this entry in 2nd half of the block? */
        if (size + map[i].size/2 > blocksize/2)
            break;
        size += map[i].size;
        move++;
    }
    /* map index at which we will split */
    split = count - move;
    hash2 = map[split].hash;
    continued = hash2 == map[split - 1].hash;
    dxtrace(printk("Split block %i at %x, %i/%i\n",
                   dx_get_block(frame->at), hash2, split, count-split));

    /* Fancy dance to stay within two buffers */
    de2 = dx_move_dirents(data1, data2, map + split, count - split);
    de = dx_pack_dirents(data1,blocksize);
    de->rec_len = cpu_to_le16(data1 + blocksize - (char *) de);
    de2->rec_len = cpu_to_le16(data2 + blocksize - (char *) de2);
    dxtrace(dx_show_leaf (icb, hinfo, (struct ext3_dir_entry_2 *) data1, blocksize, 1));
    dxtrace(dx_show_leaf (icb, hinfo, (struct ext3_dir_entry_2 *) data2, blocksize, 1));

    /* Which block gets the new entry? */
    if (hinfo->hash >= hash2)
    {
        swap(struct buffer_head *, *bh, bh2);
        de = de2;
    }
    dx_insert_block (frame, hash2 + continued, newblock);
    set_buffer_dirty(bh2);
    set_buffer_dirty(frame->bh);

    brelse (bh2);
    dxtrace(dx_show_index ("frame", frame->entries));
errout:
    return de;
}

/*
 * This converts a one block unindexed directory to a 3 block indexed
 * directory, and adds the dentry to the indexed directory.
 */
int make_indexed_dir(struct ext2_icb *icb, struct dentry *dentry,
                     struct inode *inode, struct buffer_head *bh)
{
    struct inode	*dir = dentry->d_parent->d_inode;
    const char	*name = dentry->d_name.name;
    int		namelen = dentry->d_name.len;
    struct buffer_head *bh2;
    struct dx_root	*root;
    struct dx_frame	frames[2], *frame;
    struct dx_entry *entries;
    struct ext3_dir_entry_2	*de, *de2;
    char		*data1, *top;
    unsigned	len;
    int		retval;
    unsigned	blocksize;
    struct dx_hash_info hinfo;
    ext3_lblk_t  block;
    struct fake_dirent *fde;

    blocksize =  dir->i_sb->s_blocksize;
    dxtrace(printk("Creating index: inode %lu\n", dir->i_ino));

    root = (struct dx_root *) bh->b_data;

    /* The 0th block becomes the root, move the dirents out */
    fde = &root->dotdot;
    de = (struct ext3_dir_entry_2 *)((char *)fde +
                                     ext3_rec_len_from_disk(fde->rec_len));
    if ((char *) de >= (((char *) root) + blocksize)) {
        DEBUG(DL_ERR, ( "%s: invalid rec_len for '..' in inode %lu", __FUNCTION__,
                       dir->i_ino));
        brelse(bh);
        return -EIO;
    }
    len = (unsigned int)((char *) root + blocksize - (char *) de);

    /* Allocate new block for the 0th block's dirents */
    bh2 = ext3_append(icb, dir, &block, &retval);
    if (!(bh2)) {
        brelse(bh);
        return retval;
    }
    EXT3_I(dir)->i_flags |= EXT3_INDEX_FL;
    data1 = bh2->b_data;

    memcpy (data1, de, len);
    de = (struct ext3_dir_entry_2 *) data1;
    top = data1 + len;
    while ((char *)(de2 = ext3_next_entry(de)) < top)
        de = de2;
    de->rec_len = ext3_rec_len_to_disk(blocksize + (__u32)(data1 - (char *)de));
    /* Initialize the root; the dot dirents already exist */
    de = (struct ext3_dir_entry_2 *) (&root->dotdot);
    de->rec_len = ext3_rec_len_to_disk(blocksize - EXT3_DIR_REC_LEN(2));
    memset (&root->info, 0, sizeof(root->info));
    root->info.info_length = sizeof(root->info);
    root->info.hash_version = (__u8)(EXT3_SB(dir->i_sb)->s_def_hash_version);
    entries = root->entries;
    dx_set_block(entries, 1);
    dx_set_count(entries, 1);
    dx_set_limit(entries, dx_root_limit(dir, sizeof(root->info)));

    /* Initialize as for dx_probe */
    hinfo.hash_version = root->info.hash_version;
    hinfo.seed = EXT3_SB(dir->i_sb)->s_hash_seed;
    ext3_dirhash(name, namelen, &hinfo);
    frame = frames;
    frame->entries = entries;
    frame->at = entries;
    frame->bh = bh;
    bh = bh2;
    /* bh and bh2 are to be marked as dirty in do_split */
    de = do_split(icb, dir, &bh, frame, &hinfo, &retval);
    dx_release (frames);
    if (!(de))
        return retval;

    return add_dirent_to_buf(icb, dentry, inode, de, bh);
}

#else /* EXT2_HTREE_INDEX */

int ext3_release_dir (struct inode * inode, struct file * filp)
{
    return 0;
}

#endif /* !EXT2_HTREE_INDEX */

/*
 *	ext3_add_entry()
 *
 * adds a file entry to the specified directory, using the same
 * semantics as ext3_find_entry(). It returns NULL if it failed.
 *
 * NOTE!! The inode part of 'de' is left at 0 - which means you
 * may not sleep between calling this and putting something into
 * the entry, as someone else might have used it while you slept.
 */
int ext3_add_entry(struct ext2_icb *icb, struct dentry *dentry, struct inode *inode)
{
    struct inode *dir = dentry->d_parent->d_inode;
    struct buffer_head *bh;
    struct ext3_dir_entry_2 *de;
    struct super_block *sb;
    int	retval;
#ifdef EXT2_HTREE_INDEX
    int	dx_fallback=0;
#endif
    unsigned blocksize;
    ext3_lblk_t block, blocks;

    sb = dir->i_sb;
    blocksize = sb->s_blocksize;
    if (!dentry->d_name.len)
        return -EINVAL;

#ifdef EXT2_HTREE_INDEX
    if (is_dx(dir)) {
        retval = ext3_dx_add_entry(icb, dentry, inode);
        if (!retval || (retval != ERR_BAD_DX_DIR))
            return retval;
        EXT3_I(dir)->i_flags &= ~EXT3_INDEX_FL;
        dx_fallback++;
        ext3_save_inode(icb, dir);
    }
#endif

    blocks = (ext3_lblk_t)(dir->i_size >> sb->s_blocksize_bits);
    for (block = 0; block < blocks; block++) {
        bh = ext3_bread(icb, dir, block, &retval);
        if (!bh)
            return retval;
        retval = add_dirent_to_buf(icb, dentry, inode, NULL, bh);
        if (retval != -ENOSPC)
            return retval;

#ifdef EXT2_HTREE_INDEX
        if (blocks == 1 && !dx_fallback &&
                EXT3_HAS_COMPAT_FEATURE(sb, EXT3_FEATURE_COMPAT_DIR_INDEX))
            return make_indexed_dir(icb, dentry, inode, bh);
#endif

        brelse(bh);
    }
    bh = ext3_append(icb, dir, &block, &retval);
    if (!bh)
        return retval;
    de = (struct ext3_dir_entry_2 *) bh->b_data;
    de->inode = 0;
    de->rec_len = ext3_rec_len_to_disk(blocksize);
    return add_dirent_to_buf(icb, dentry, inode, de, bh);
}

/*
 * ext3_delete_entry deletes a directory entry by merging it with the
 * previous entry
 */
int ext3_delete_entry(struct ext2_icb *icb, struct inode *dir,
                      struct ext3_dir_entry_2 *de_del,
                      struct buffer_head *bh)
{
    struct ext3_dir_entry_2 *de, *pde = NULL;
    size_t i = 0;

    de = (struct ext3_dir_entry_2 *) bh->b_data;
    while (i < bh->b_size) {
        if (!ext3_check_dir_entry("ext3_delete_entry", dir, de, bh, i))
            return -EIO;
        if (de == de_del)  {
            if (pde)
                pde->rec_len = ext3_rec_len_to_disk(
                                   ext3_rec_len_from_disk(pde->rec_len) +
                                   ext3_rec_len_from_disk(de->rec_len));
            else
                de->inode = 0;
            dir->i_version++;
            /* ext3_journal_dirty_metadata(handle, bh); */
            set_buffer_dirty(bh);
            return 0;
        }
        i += ext3_rec_len_from_disk(de->rec_len);
        pde = de;
        de = ext3_next_entry(de);
    }
    return -ENOENT;
}

/*
 * routine to check that the specified directory is empty (for rmdir)
 */
int ext3_is_dir_empty(struct ext2_icb *icb, struct inode *inode)
{
    unsigned int offset;
    struct buffer_head *bh;
    struct ext3_dir_entry_2 *de, *de1;
    struct super_block *sb;
    int err = 0;

    sb = inode->i_sb;
    if (inode->i_size < EXT3_DIR_REC_LEN(1) + EXT3_DIR_REC_LEN(2) ||
            !(bh = ext3_bread(icb, inode, 0, &err))) {
        if (err)
            ext3_error(inode->i_sb, __FUNCTION__,
                       "error %d reading directory #%lu offset 0",
                       err, inode->i_ino);
        else
            ext3_warning(inode->i_sb, __FUNCTION__,
                         "bad directory (dir #%lu) - no data block",
                         inode->i_ino);
        return 1;
    }
    de = (struct ext3_dir_entry_2 *) bh->b_data;
    de1 = ext3_next_entry(de);
    if (le32_to_cpu(de->inode) != inode->i_ino ||
            !le32_to_cpu(de1->inode) ||
            strcmp(".", de->name) ||
            strcmp("..", de1->name)) {
        ext3_warning(inode->i_sb, "empty_dir",
                     "bad directory (dir #%lu) - no `.' or `..'",
                     inode->i_ino);
        brelse(bh);
        return 1;
    }
    offset = ext3_rec_len_from_disk(de->rec_len) +
             ext3_rec_len_from_disk(de1->rec_len);
    de = ext3_next_entry(de1);
    while (offset < inode->i_size) {
        if (!bh ||
                (void *) de >= (void *) (bh->b_data+sb->s_blocksize)) {
            err = 0;
            brelse(bh);
            bh = ext3_bread(icb, inode, offset >> EXT3_BLOCK_SIZE_BITS(sb), &err);
            if (!bh) {
                if (err)
                    ext3_error(sb, __FUNCTION__, "error %d reading directory"
                               " #%lu offset %u", err, inode->i_ino, offset);
                offset += sb->s_blocksize;
                continue;
            }
            de = (struct ext3_dir_entry_2 *) bh->b_data;
        }
        if (!ext3_check_dir_entry("empty_dir", inode, de, bh, offset)) {
            de = (struct ext3_dir_entry_2 *)(bh->b_data +
                                             sb->s_blocksize);
            offset = (offset | (sb->s_blocksize - 1)) + 1;
            continue;
        }
        if (le32_to_cpu(de->inode)) {
            brelse(bh);
            return 0;
        }
        offset += ext3_rec_len_from_disk(de->rec_len);
        de = ext3_next_entry(de);
    }
    brelse(bh);
    return 1;
}

/*
 * Returns 0 if not found, -1 on failure, and 1 on success
 */
static inline int search_dirblock(struct buffer_head * bh,
                                  struct inode *dir,
                                  struct dentry *dentry,
                                  unsigned long offset,
                                  struct ext3_dir_entry_2 ** res_dir)
{
    struct ext3_dir_entry_2 * de;
    char * dlimit;
    int de_len;
    const char *name = dentry->d_name.name;
    int namelen = dentry->d_name.len;

    de = (struct ext3_dir_entry_2 *) bh->b_data;
    dlimit = bh->b_data + dir->i_sb->s_blocksize;
    while ((char *) de < dlimit) {
        /* this code is executed quadratically often */
        /* do minimal checking `by hand' */

        if ((char *) de + namelen <= dlimit &&
                ext3_match (namelen, name, de)) {
            /* found a match - just to be sure, do a full check */
            if (!ext3_check_dir_entry("ext3_find_entry",
                                      dir, de, bh, offset))
                return -1;
            *res_dir = de;
            return 1;
        }
        /* prevent looping on a bad block */
        de_len = ext3_rec_len_from_disk(de->rec_len);

        if (de_len <= 0)
            return -1;
        offset += de_len;
        de = (struct ext3_dir_entry_2 *) ((char *) de + de_len);
    }
    return 0;
}

/*
 * define how far ahead to read directories while searching them.
 */
#define NAMEI_RA_CHUNKS  2
#define NAMEI_RA_BLOCKS  4
#define NAMEI_RA_SIZE	     (NAMEI_RA_CHUNKS * NAMEI_RA_BLOCKS)
#define NAMEI_RA_INDEX(c,b)  (((c) * NAMEI_RA_BLOCKS) + (b))

/*
 *	ext4_find_entry()
 *
 * finds an entry in the specified directory with the wanted name. It
 * returns the cache buffer in which the entry was found, and the entry
 * itself (as a parameter - res_dir). It does NOT read the inode of the
 * entry - you'll have to do that yourself if you want to.
 *
 * The returned buffer_head has ->b_count elevated.  The caller is expected
 * to brelse() it when appropriate.
 */
struct buffer_head * ext3_find_entry (struct ext2_icb *icb,
                                                  struct dentry *dentry,
                                                  struct ext3_dir_entry_2 ** res_dir)
{
    struct inode *dir = dentry->d_parent->d_inode;
    struct super_block *sb = dir->i_sb;
    struct buffer_head *bh_use[NAMEI_RA_SIZE];
    struct buffer_head *bh, *ret = NULL;
    ext3_lblk_t start, block, b;
    int ra_max = 0;		/* Number of bh's in the readahead
				   buffer, bh_use[] */
    int ra_ptr = 0;		/* Current index into readahead
				   buffer */
    int num = 0;
    ext3_lblk_t  nblocks;
    int i, err;
    int namelen = dentry->d_name.len;

    *res_dir = NULL;
    if (namelen > EXT3_NAME_LEN)
        return NULL;

#ifdef EXT2_HTREE_INDEX
    if (is_dx(dir)) {
        bh = ext3_dx_find_entry(icb, dentry, res_dir, &err);
        /*
         * On success, or if the error was file not found,
         * return.  Otherwise, fall back to doing a search the
         * old fashioned way.
         */
        if (bh || (err != ERR_BAD_DX_DIR))
            return bh;
        dxtrace(printk("ext4_find_entry: dx failed, "
                       "falling back\n"));
    }
#endif

    nblocks = (ext3_lblk_t)(dir->i_size >> EXT3_BLOCK_SIZE_BITS(sb));
    start = 0;
    block = start;
restart:
    do {
        /*
         * We deal with the read-ahead logic here.
         */
        if (ra_ptr >= ra_max) {
            /* Refill the readahead buffer */
            ra_ptr = 0;
            b = block;
            for (ra_max = 0; ra_max < NAMEI_RA_SIZE; ra_max++) {
                /*
                 * Terminate if we reach the end of the
                 * directory and must wrap, or if our
                 * search has finished at this block.
                 */
                if (b >= nblocks || (num && block == start)) {
                    bh_use[ra_max] = NULL;
                    break;
                }
                num++;
                bh = ext3_bread(icb, dir, b++, &err);
                bh_use[ra_max] = bh;
            }
        }
        if ((bh = bh_use[ra_ptr++]) == NULL)
            goto next;
        wait_on_buffer(bh);
        if (!buffer_uptodate(bh)) {
            /* read error, skip block & hope for the best */
            ext3_error(sb, __FUNCTION__, "reading directory #%lu "
                       "offset %lu", dir->i_ino,
                       (unsigned long)block);
            brelse(bh);
            goto next;
        }
        i = search_dirblock(bh, dir, dentry,
                            block << EXT3_BLOCK_SIZE_BITS(sb), res_dir);
        if (i == 1) {
            ret = bh;
            goto cleanup_and_exit;
        } else {
            brelse(bh);
            if (i < 0)
                goto cleanup_and_exit;
        }
next:
        if (++block >= nblocks)
            block = 0;
    } while (block != start);

    /*
     * If the directory has grown while we were searching, then
     * search the last part of the directory before giving up.
     */
    block = nblocks;
    nblocks = (ext3_lblk_t)(dir->i_size >> EXT3_BLOCK_SIZE_BITS(sb));
    if (block < nblocks) {
        start = 0;
        goto restart;
    }

cleanup_and_exit:
    /* Clean up the read-ahead blocks */
    for (; ra_ptr < ra_max; ra_ptr++)
        brelse(bh_use[ra_ptr]);
    return ret;
}