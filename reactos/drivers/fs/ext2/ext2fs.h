BOOLEAN Ext2ReadSectors(IN PDEVICE_OBJECT pDeviceObject,
			IN ULONG	DiskSector,
                        IN ULONG        SectorCount,
			IN PVOID	Buffer);

#define BLOCKSIZE (1024)

struct ext2_super_block {
	ULONG	s_inodes_count;		/* Inodes count */
	ULONG	s_blocks_count;		/* Blocks count */
	ULONG	s_r_blocks_count;	/* Reserved blocks count */
	ULONG	s_free_blocks_count;	/* Free blocks count */
	ULONG	s_free_inodes_count;	/* Free inodes count */
	ULONG	s_first_data_block;	/* First Data Block */
	ULONG	s_log_block_size;	/* Block size */
	LONG	s_log_frag_size;	/* Fragment size */
        ULONG	s_blocks_per_group;	/* # Blocks per group */
	ULONG	s_frags_per_group;	/* # Fragments per group */
	ULONG	s_inodes_per_group;	/* # Inodes per group */
	ULONG	s_mtime;		/* Mount time */
	ULONG	s_wtime;		/* Write time */
	USHORT	s_mnt_count;		/* Mount count */
	SHORT	s_max_mnt_count;	/* Maximal mount count */
	USHORT	s_magic;		/* Magic signature */
	USHORT	s_state;		/* File system state */
	USHORT	s_errors;		/* Behaviour when detecting errors */
	USHORT	s_minor_rev_level; 	/* minor revision level */
	ULONG	s_lastcheck;		/* time of last check */
	ULONG	s_checkinterval;	/* max. time between checks */
	ULONG	s_creator_os;		/* OS */
	ULONG	s_rev_level;		/* Revision level */
	USHORT	s_def_resuid;		/* Default uid for reserved blocks */
	USHORT	s_def_resgid;		/* Default gid for reserved blocks */
	/*
	 * These fields are for EXT2_DYNAMIC_REV superblocks only.
	 *
	 * Note: the difference between the compatible feature set and
	 * the incompatible feature set is that if there is a bit set
	 * in the incompatible feature set that the kernel doesn't
	 * know about, it should refuse to mount the filesystem.
	 * 
	 * e2fsck's requirements are more strict; if it doesn't know
	 * about a feature in either the compatible or incompatible
	 * feature set, it must abort and not try to meddle with
	 * things it doesn't understand...
	 */
	ULONG	s_first_ino; 		/* First non-reserved inode */
	USHORT   s_inode_size; 		/* size of inode structure */
	USHORT	s_block_group_nr; 	/* block group # of this superblock */
	ULONG	s_feature_compat; 	/* compatible feature set */
	ULONG	s_feature_incompat; 	/* incompatible feature set */
	ULONG	s_feature_ro_compat; 	/* readonly-compatible feature set */
	ULONG	s_reserved[230];	/* Padding to the end of the block */
};

/*
 * Codes for operating systems
 */
#define EXT2_OS_LINUX		0
#define EXT2_OS_HURD		1
#define EXT2_OS_MASIX		2
#define EXT2_OS_FREEBSD		3
#define EXT2_OS_LITES		4

/*
 * Revision levels
 */
#define EXT2_GOOD_OLD_REV	0	/* The good old (original) format */
#define EXT2_DYNAMIC_REV	1 	/* V2 format w/ dynamic inode sizes */

#define EXT2_CURRENT_REV	EXT2_GOOD_OLD_REV
#define EXT2_MAX_SUPP_REV	EXT2_DYNAMIC_REV

/*
 * The second extended file system magic number
 */
#define EXT2_SUPER_MAGIC	0xEF53

/*
 * Constants relative to the data blocks
 */
#define	EXT2_NDIR_BLOCKS		12
#define	EXT2_IND_BLOCK			EXT2_NDIR_BLOCKS
#define	EXT2_DIND_BLOCK			(EXT2_IND_BLOCK + 1)
#define	EXT2_TIND_BLOCK			(EXT2_DIND_BLOCK + 1)
#define	EXT2_N_BLOCKS			(EXT2_TIND_BLOCK + 1)


/*
 * Structure of an inode on the disk
 */
struct ext2_inode {
	USHORT	i_mode;		/* File mode */
	USHORT	i_uid;		/* Owner Uid */
	ULONG	i_size;		/* Size in bytes */
	ULONG	i_atime;	/* Access time */
	ULONG	i_ctime;	/* Creation time */
	ULONG	i_mtime;	/* Modification time */
	ULONG	i_dtime;	/* Deletion Time */
	USHORT	i_gid;		/* Group Id */
	USHORT	i_links_count;	/* Links count */
	ULONG	i_blocks;	/* Blocks count */
	ULONG	i_flags;	/* File flags */
	union {
		struct {
			ULONG  l_i_reserved1;
		} linux1;
		struct {
			ULONG  h_i_translator;
		} hurd1;
		struct {
			ULONG  m_i_reserved1;
		} masix1;
	} osd1;				/* OS dependent 1 */
	ULONG	i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
	ULONG	i_version;	/* File version (for NFS) */
	ULONG	i_file_acl;	/* File ACL */
	ULONG	i_dir_acl;	/* Directory ACL */
	ULONG	i_faddr;	/* Fragment address */
	union {
		struct {
			UCHAR	l_i_frag;	/* Fragment number */
			UCHAR	l_i_fsize;	/* Fragment size */
			USHORT	i_pad1;
			ULONG	l_i_reserved2[2];
		} linux2;
		struct {
			UCHAR	h_i_frag;	/* Fragment number */
			UCHAR	h_i_fsize;	/* Fragment size */
			USHORT	h_i_mode_high;
			USHORT	h_i_uid_high;
			USHORT	h_i_gid_high;
			ULONG	h_i_author;
		} hurd2;
		struct {
			UCHAR	m_i_frag;	/* Fragment number */
			UCHAR	m_i_fsize;	/* Fragment size */
			USHORT	m_pad1;
			ULONG	m_i_reserved2[2];
		} masix2;
	} osd2;				/* OS dependent 2 */
};

#if defined(__KERNEL__) || defined(__linux__)
#define i_reserved1	osd1.linux1.l_i_reserved1
#define i_frag		osd2.linux2.l_i_frag
#define i_fsize		osd2.linux2.l_i_fsize
#define i_reserved2	osd2.linux2.l_i_reserved2
#endif

#ifdef	__hurd__
#define i_translator	osd1.hurd1.h_i_translator
#define i_frag		osd2.hurd2.h_i_frag;
#define i_fsize		osd2.hurd2.h_i_fsize;
#define i_uid_high	osd2.hurd2.h_i_uid_high
#define i_gid_high	osd2.hurd2.h_i_gid_high
#define i_author	osd2.hurd2.h_i_author
#endif

#ifdef	__masix__
#define i_reserved1	osd1.masix1.m_i_reserved1
#define i_frag		osd2.masix2.m_i_frag
#define i_fsize		osd2.masix2.m_i_fsize
#define i_reserved2	osd2.masix2.m_i_reserved2
#endif

/*
 * Constants relative to the data blocks
 */
#define	EXT2_NDIR_BLOCKS		12
#define	EXT2_IND_BLOCK			EXT2_NDIR_BLOCKS
#define	EXT2_DIND_BLOCK			(EXT2_IND_BLOCK + 1)
#define	EXT2_TIND_BLOCK			(EXT2_DIND_BLOCK + 1)
#define	EXT2_N_BLOCKS			(EXT2_TIND_BLOCK + 1)

/*
 * Inode flags
 */
#define	EXT2_SECRM_FL			0x00000001 /* Secure deletion */
#define	EXT2_UNRM_FL			0x00000002 /* Undelete */
#define	EXT2_COMPR_FL			0x00000004 /* Compress file */
#define EXT2_SYNC_FL			0x00000008 /* Synchronous updates */
#define EXT2_IMMUTABLE_FL		0x00000010 /* Immutable file */
#define EXT2_APPEND_FL			0x00000020 /* writes to file may only append */
#define EXT2_NODUMP_FL			0x00000040 /* do not dump file */
#define EXT2_RESERVED_FL		0x80000000 /* reserved for ext2 lib */
	

/*
 * Structure of a blocks group descriptor
 */
struct ext2_group_desc
{
	ULONG	bg_block_bitmap;		/* Blocks bitmap block */
	ULONG	bg_inode_bitmap;		/* Inodes bitmap block */
	ULONG	bg_inode_table;		/* Inodes table block */
	USHORT	bg_free_blocks_count;	/* Free blocks count */
	USHORT	bg_free_inodes_count;	/* Free inodes count */
	USHORT	bg_used_dirs_count;	/* Directories count */
	USHORT	bg_pad;
	ULONG	bg_reserved[3];
};

#define EXT2_NAME_LEN 255

struct ext2_dir_entry {
	ULONG	inode;			/* Inode number */
	USHORT	rec_len;		/* Directory entry length */
	USHORT	name_len;		/* Name length */
	char	name[EXT2_NAME_LEN];	/* File name */
};

typedef struct
{
   PDEVICE_OBJECT StorageDevice;
   struct ext2_super_block* superblock;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


VOID Ext2ReadInode(PDEVICE_EXTENSION DeviceExt,
		   ULONG ino,
		   struct ext2_inode* inode);
struct ext2_group_desc* Ext2LoadGroupDesc(PDEVICE_EXTENSION DeviceExt,
					  ULONG block_group);

typedef struct _EXT2_FCB
{
   struct ext2_inode inode;
} EXT2_FCB, *PEXT2_FCB;

ULONG Ext2BlockMap(PDEVICE_EXTENSION DeviceExt,
		  struct ext2_inode* inode,
		  ULONG offset);
NTSTATUS Ext2OpenFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject, 
		      PWSTR FileName);
NTSTATUS Ext2ReadFile(PDEVICE_EXTENSION DeviceExt, 
		      PFILE_OBJECT FileObject,
		      PVOID Buffer, 
		      ULONG Length, 
                      LARGE_INTEGER Offset);
NTSTATUS Ext2Create(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS Ext2DirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
