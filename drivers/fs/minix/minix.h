#include <ddk/ntddk.h>
#include <ddk/ntifs.h>

#define MINIX_ROOT_INO 1

/* Not the same as the bogus LINK_MAX in <linux/limits.h>. Oh well. */
#define MINIX_LINK_MAX	250

#define MINIX_I_MAP_SLOTS	8
#define MINIX_Z_MAP_SLOTS	64
#define MINIX_SUPER_MAGIC	0x137F		/* original minix fs */
#define MINIX_SUPER_MAGIC2	0x138F		/* minix fs, 30 char names */
#define MINIX2_SUPER_MAGIC	0x2468		/* minix V2 fs */
#define MINIX2_SUPER_MAGIC2	0x2478		/* minix V2 fs, 30 char names */
#define MINIX_VALID_FS		0x0001		/* Clean fs. */
#define MINIX_ERROR_FS		0x0002		/* fs has errors. */

#define MINIX_INODES_PER_BLOCK ((BLOCKSIZE)/(sizeof (struct minix_inode)))
#define MINIX2_INODES_PER_BLOCK ((BLOCKSIZE)/(sizeof (struct minix2_inode)))

#define MINIX_V1		0x0001		/* original minix fs */
#define MINIX_V2		0x0002		/* minix V2 fs */


/*
 * This is the original minix inode layout on disk.
 * Note the 8-bit gid and atime and ctime.
 */
struct minix_inode {
	unsigned short int i_mode;
	unsigned short int i_uid;
	unsigned long i_size;
	unsigned long i_time;
	unsigned char  i_gid;
	unsigned char  i_nlinks;
	unsigned short int i_zone[9];
};

/*
 * The new minix inode has all the time entries, as well as
 * long block numbers and a third indirect block (7+1+1+1
 * instead of 7+1+1). Also, some previously 8-bit values are
 * now 16-bit. The inode is now 64 bytes instead of 32.
 */
struct minix2_inode {
	unsigned short int i_mode;
	unsigned short int i_nlinks;
	unsigned short int i_uid;
	unsigned short int i_gid;
	unsigned long i_size;
	unsigned long i_atime;
	unsigned long i_mtime;
	unsigned long i_ctime;
	unsigned long i_zone[10];
};

/*
 * minix super-block data on disk
 */
struct minix_super_block {
	unsigned short int s_ninodes;
	unsigned short int s_nzones;
	unsigned short int s_imap_blocks;
	unsigned short int s_zmap_blocks;
	unsigned short int s_firstdatazone;
	unsigned short int s_log_zone_size;
	unsigned long s_max_size;
	unsigned short int s_magic;
	unsigned short int s_state;
	unsigned long s_zones;
};

struct minix_dir_entry {
	unsigned short int inode;
	char name[0];
};
#define MINIX_DIR_ENTRY_SIZE (sizeof(struct minix_dir_entry)+30)

BOOLEAN MinixReadSector(IN PDEVICE_OBJECT pDeviceObject,
			IN ULONG	DiskSector,
			IN PVOID	Buffer);
BOOLEAN MinixWriteSector(IN PDEVICE_OBJECT pDeviceObject,
			IN ULONG	DiskSector,
			IN PVOID	Buffer);

#define BLOCKSIZE (1024)

//extern PDRIVER_OBJECT DriverObject;

typedef struct
{
   PDEVICE_OBJECT AttachedDevice;
   struct minix_inode root_inode;
   char superblock_buf[BLOCKSIZE];
   struct minix_super_block* sb;
   PFILE_OBJECT FileObject;
} MINIX_DEVICE_EXTENSION, *PMINIX_DEVICE_EXTENSION;

typedef struct
{
   struct minix_inode inode;
} MINIX_FSCONTEXT, *PMINIX_FSCONTEXT;

NTSTATUS STDCALL MinixCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL MinixClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL MinixWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL MinixRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL MinixDirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

ULONG MinixNewInode(PDEVICE_OBJECT Volume,
		    MINIX_DEVICE_EXTENSION* DeviceExt,
		    struct minix_inode* new_inode);
NTSTATUS MinixWriteInode(PDEVICE_OBJECT Volume,
			 MINIX_DEVICE_EXTENSION* DeviceExt,
			 ULONG ino, 
			 struct minix_inode* result);
NTSTATUS MinixReadInode(PDEVICE_OBJECT DeviceObject,
			MINIX_DEVICE_EXTENSION* DeviceExt,
			ULONG ino, 
			struct minix_inode* result);
NTSTATUS MinixDeleteInode(PDEVICE_OBJECT Volume,
			  MINIX_DEVICE_EXTENSION* DeviceExt,
			  ULONG ino);

NTSTATUS MinixReadBlock(PDEVICE_OBJECT DeviceObject,
			PMINIX_DEVICE_EXTENSION DeviceExt,
			struct minix_inode* inode, 
			ULONG FileOffset,
			PULONG DiskOffset);

BOOLEAN MinixReadPage(PDEVICE_OBJECT DeviceObject,
		      ULONG Offset,
		      PVOID Buffer);
