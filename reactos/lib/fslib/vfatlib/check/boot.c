/* boot.c  -  Read and analyze ia PC/MS-DOS boot sector */

/* Written 1993 by Werner Almesberger */

/* FAT32, VFAT, Atari format support, and various fixes additions May 1998
 * by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de> */

#include "vfatlib.h"

#define NDEBUG
#include <debug.h>


#define ROUND_TO_MULTIPLE(n,m) ((n) && (m) ? (n)+(m)-1-((n)-1)%(m) : 0)
    /* don't divide by zero */

static struct {
    __u8 media;
    char *descr;
} mediabytes[] = {
    { 0xf0, "5.25\" or 3.5\" HD floppy" },
    { 0xf8, "hard disk" },
    { 0xf9, "3,5\" 720k floppy 2s/80tr/9sec or "
            "5.25\" 1.2M floppy 2s/80tr/15sec" },
    { 0xfa, "5.25\" 320k floppy 1s/80tr/8sec" },
    { 0xfb, "3.5\" 640k floppy 2s/80tr/8sec" },
    { 0xfc, "5.25\" 180k floppy 1s/40tr/9sec" },
    { 0xfd, "5.25\" 360k floppy 2s/40tr/9sec" },
    { 0xfe, "5.25\" 160k floppy 1s/40tr/8sec" },
    { 0xff, "5.25\" 320k floppy 2s/40tr/8sec" },
};

#if defined __alpha || defined __ia64__ || defined __s390x__ || defined __x86_64__ || defined __ppc64__
/* Unaligned fields must first be copied byte-wise */
#define GET_UNALIGNED_W(f)			\
    ({						\
	unsigned short __v;			\
	memcpy( &__v, &f, sizeof(__v) );	\
	CF_LE_W( *(unsigned short *)&f );	\
    })
#else
#define GET_UNALIGNED_W(f) CF_LE_W( *(unsigned short *)&f )
#endif


static char *get_media_descr( unsigned char media )
{
    int i;

    for( i = 0; i < sizeof(mediabytes)/sizeof(*mediabytes); ++i ) {
	if (mediabytes[i].media == media)
	    return( mediabytes[i].descr );
    }
    return( "undefined" );
}

static void dump_boot(DOS_FS *fs,struct boot_sector *b,unsigned lss)
{
    unsigned short sectors;
	char id[9];

    VfatPrint("Boot sector contents:\n");

	strncpy(id,(char*)b->system_id,8);
	id[8] = 0;
	VfatPrint("System ID \"%s\"\n",id);

    VfatPrint("Media byte 0x%02x (%s)\n",b->media,get_media_descr(b->media));
    VfatPrint("%10d bytes per logical sector\n",GET_UNALIGNED_W(b->sector_size));
    VfatPrint("%10d bytes per cluster\n",fs->cluster_size);
    VfatPrint("%10d reserved sector%s\n",CF_LE_W(b->reserved),
	   CF_LE_W(b->reserved) == 1 ? "" : "s");
    VfatPrint("First FAT starts at byte %llu (sector %llu)\n",
	   (__u64)fs->fat_start,
	   (__u64)fs->fat_start/lss);
    VfatPrint("%10d FATs, %d bit entries\n",b->fats,fs->fat_bits);
    VfatPrint("%10d bytes per FAT (= %u sectors)\n",fs->fat_size,
	   fs->fat_size/lss);
    if (!fs->root_cluster) {
	VfatPrint("Root directory starts at byte %llu (sector %llu)\n",
	       (__u64)fs->root_start,
	       (__u64)fs->root_start/lss);
	VfatPrint("%10d root directory entries\n",fs->root_entries);
    }
    else {
	VfatPrint( "Root directory start at cluster %lu (arbitrary size)\n",
		fs->root_cluster);
    }
    VfatPrint("Data area starts at byte %llu (sector %llu)\n",
	   (__u64)fs->data_start,
	   (__u64)fs->data_start/lss);
    VfatPrint("%10lu data clusters (%llu bytes)\n",fs->clusters,
	   (__u64)fs->clusters*fs->cluster_size);
    VfatPrint("%u sectors/track, %u heads\n",CF_LE_W(b->secs_track),
	   CF_LE_W(b->heads));
    VfatPrint("%10u hidden sectors\n", CF_LE_L(b->hidden));
    sectors = GET_UNALIGNED_W( b->sectors );
    VfatPrint("%10u sectors total\n", sectors ? sectors : CF_LE_L(b->total_sect));
}

static void check_backup_boot(DOS_FS *fs, struct boot_sector *b, int lss)
{
    struct boot_sector b2;

    if (!fs->backupboot_start) {
	VfatPrint( "There is no backup boot sector.\n" );
	if (CF_LE_W(b->reserved) < 3) {
	    VfatPrint( "And there is no space for creating one!\n" );
	    return;
	}
	if (FsCheckFlags & FSCHECK_INTERACTIVE)
	    VfatPrint( "1) Create one\n2) Do without a backup\n" );
	else VfatPrint( "  Auto-creating backup boot block.\n" );
	if (!(FsCheckFlags & FSCHECK_INTERACTIVE) || get_key("12","?") == '1') {
	    int bbs;
	    /* The usual place for the backup boot sector is sector 6. Choose
	     * that or the last reserved sector. */
	    if (CF_LE_W(b->reserved) >= 7 && CF_LE_W(b->info_sector) != 6)
		bbs = 6;
	    else {
		bbs = CF_LE_W(b->reserved) - 1;
		if (bbs == CF_LE_W(b->info_sector))
		    --bbs; /* this is never 0, as we checked reserved >= 3! */
	    }
	    fs->backupboot_start = bbs*lss;
	    b->backup_boot = CT_LE_W(bbs);
	    fs_write(fs->backupboot_start,sizeof(*b),b);
	    fs_write((off_t)offsetof(struct boot_sector,backup_boot),
		     sizeof(b->backup_boot),&b->backup_boot);
	    VfatPrint( "Created backup of boot sector in sector %d\n", bbs );
	    return;
	}
	else return;
    }

    fs_read(fs->backupboot_start,sizeof(b2),&b2);
    if (memcmp(b,&b2,sizeof(b2)) != 0) {
	/* there are any differences */
	__u8 *p, *q;
	int i, pos, first = 1;
	char buf[20];

	VfatPrint( "There are differences between boot sector and its backup.\n" );
	VfatPrint( "Differences: (offset:original/backup)\n  " );
	pos = 2;
	for( p = (__u8 *)b, q = (__u8 *)&b2, i = 0; i < sizeof(b2);
	     ++p, ++q, ++i ) {
	    if (*p != *q) {
		sprintf( buf, "%s%u:%02x/%02x", first ? "" : ", ",
			 (unsigned)(p-(__u8 *)b), *p, *q );
		if (pos + strlen(buf) > 78) VfatPrint( "\n  " ), pos = 2;
		VfatPrint( "%s", buf );
		pos += strlen(buf);
		first = 0;
	    }
	}
	VfatPrint( "\n" );

	if (FsCheckFlags & FSCHECK_INTERACTIVE)
	    VfatPrint( "1) Copy original to backup\n"
		    "2) Copy backup to original\n"
		    "3) No action\n" );
	else VfatPrint( "  Not automatically fixing this.\n" );
	switch ((FsCheckFlags & FSCHECK_INTERACTIVE) ? get_key("123","?") : '3') {
	  case '1':
	    fs_write(fs->backupboot_start,sizeof(*b),b);
	    break;
	  case '2':
	    fs_write(0,sizeof(b2),&b2);
	    break;
	  default:
	    break;
	}
    }
}

static void init_fsinfo(struct info_sector *i)
{
    i->magic = CT_LE_L(0x41615252);
    i->signature = CT_LE_L(0x61417272);
    i->free_clusters = CT_LE_L(-1);
    i->next_cluster = CT_LE_L(2);
    i->boot_sign = CT_LE_W(0xaa55);
}

static void read_fsinfo(DOS_FS *fs, struct boot_sector *b,int lss)
{
    struct info_sector i;

    if (!b->info_sector) {
	VfatPrint( "No FSINFO sector\n" );
	if (FsCheckFlags & FSCHECK_INTERACTIVE)
	    VfatPrint( "1) Create one\n2) Do without FSINFO\n" );
	else VfatPrint( "  Not automatically creating it.\n" );
	if ((FsCheckFlags & FSCHECK_INTERACTIVE) && get_key("12","?") == '1') {
	    /* search for a free reserved sector (not boot sector and not
	     * backup boot sector) */
	    __u32 s;
	    for( s = 1; s < CF_LE_W(b->reserved); ++s )
		if (s != CF_LE_W(b->backup_boot)) break;
	    if (s > 0 && s < CF_LE_W(b->reserved)) {
		init_fsinfo(&i);
		fs_write((off_t)s*lss,sizeof(i),&i);
		b->info_sector = CT_LE_W(s);
		fs_write((off_t)offsetof(struct boot_sector,info_sector),
			 sizeof(b->info_sector),&b->info_sector);
		if (fs->backupboot_start)
		    fs_write(fs->backupboot_start+
			     offsetof(struct boot_sector,info_sector),
			     sizeof(b->info_sector),&b->info_sector);
	    }
	    else {
		VfatPrint( "No free reserved sector found -- "
			"no space for FSINFO sector!\n" );
		return;
	    }
	}
	else return;
    }

    fs->fsinfo_start = CF_LE_W(b->info_sector)*lss;
    fs_read(fs->fsinfo_start,sizeof(i),&i);

    if (i.magic != CT_LE_L(0x41615252) ||
	i.signature != CT_LE_L(0x61417272) ||
	i.boot_sign != CT_LE_W(0xaa55)) {
	VfatPrint( "FSINFO sector has bad magic number(s):\n" );
	if (i.magic != CT_LE_L(0x41615252))
	    VfatPrint( "  Offset %llu: 0x%08x != expected 0x%08x\n",
		    (__u64)offsetof(struct info_sector,magic),
		    CF_LE_L(i.magic),0x41615252);
	if (i.signature != CT_LE_L(0x61417272))
	    VfatPrint( "  Offset %llu: 0x%08x != expected 0x%08x\n",
		    (__u64)offsetof(struct info_sector,signature),
		    CF_LE_L(i.signature),0x61417272);
	if (i.boot_sign != CT_LE_W(0xaa55))
	    VfatPrint( "  Offset %llu: 0x%04x != expected 0x%04x\n",
		    (__u64)offsetof(struct info_sector,boot_sign),
		    CF_LE_W(i.boot_sign),0xaa55);
	if (FsCheckFlags & FSCHECK_INTERACTIVE)
	    VfatPrint( "1) Correct\n2) Don't correct (FSINFO invalid then)\n" );
	else VfatPrint( "  Auto-correcting it.\n" );
	if (!(FsCheckFlags & FSCHECK_INTERACTIVE) || get_key("12","?") == '1') {
	    init_fsinfo(&i);
	    fs_write(fs->fsinfo_start,sizeof(i),&i);
	}
	else fs->fsinfo_start = 0;
    }

    if (fs->fsinfo_start)
	fs->free_clusters = CF_LE_L(i.free_clusters);
}

void read_boot(DOS_FS *fs)
{
    struct boot_sector b;
    unsigned total_sectors;
    unsigned short logical_sector_size, sectors;
    unsigned fat_length;
    loff_t data_size;

    fs_read(0,sizeof(b),&b);
    logical_sector_size = GET_UNALIGNED_W(b.sector_size);
    if (!logical_sector_size) die("Logical sector size is zero.");
    fs->cluster_size = b.cluster_size*logical_sector_size;
    if (!fs->cluster_size) die("Cluster size is zero.");
    if (b.fats != 2 && b.fats != 1)
	die("Currently, only 1 or 2 FATs are supported, not %d.\n",b.fats);
    fs->nfats = b.fats;
    sectors = GET_UNALIGNED_W(b.sectors);
    total_sectors = sectors ? sectors : CF_LE_L(b.total_sect);
    if (FsCheckFlags & FSCHECK_VERBOSE) VfatPrint("Checking we can access the last sector of the filesystem\n");
    /* Can't access last odd sector anyway, so round down */
    fs_test((loff_t)((total_sectors & ~1)-1)*(loff_t)logical_sector_size,
	    logical_sector_size);
    fat_length = CF_LE_W(b.fat_length) ?
		 CF_LE_W(b.fat_length) : CF_LE_L(b.fat32_length);
    fs->fat_start = (off_t)CF_LE_W(b.reserved)*logical_sector_size;
    fs->root_start = ((off_t)CF_LE_W(b.reserved)+b.fats*fat_length)*
      logical_sector_size;
    fs->root_entries = GET_UNALIGNED_W(b.dir_entries);
    fs->data_start = fs->root_start+ROUND_TO_MULTIPLE(fs->root_entries <<
      MSDOS_DIR_BITS,logical_sector_size);
    data_size = (loff_t)total_sectors*logical_sector_size-fs->data_start;
    fs->clusters = data_size/fs->cluster_size;
    fs->root_cluster = 0; /* indicates standard, pre-FAT32 root dir */
    fs->fsinfo_start = 0; /* no FSINFO structure */
    fs->free_clusters = -1; /* unknown */
    if (!b.fat_length && b.fat32_length) {
	fs->fat_bits = 32;
	fs->root_cluster = CF_LE_L(b.root_cluster);
	if (!fs->root_cluster && fs->root_entries)
	    /* M$ hasn't specified this, but it looks reasonable: If
	     * root_cluster is 0 but there is a separate root dir
	     * (root_entries != 0), we handle the root dir the old way. Give a
	     * warning, but convertig to a root dir in a cluster chain seems
	     * to complex for now... */
	    VfatPrint( "Warning: FAT32 root dir not in cluster chain! "
		    "Compability mode...\n" );
	else if (!fs->root_cluster && !fs->root_entries)
	    die("No root directory!");
	else if (fs->root_cluster && fs->root_entries)
	    VfatPrint( "Warning: FAT32 root dir is in a cluster chain, but "
		    "a separate root dir\n"
		    "  area is defined. Cannot fix this easily.\n" );

	fs->backupboot_start = CF_LE_W(b.backup_boot)*logical_sector_size;
	check_backup_boot(fs,&b,logical_sector_size);

	read_fsinfo(fs,&b,logical_sector_size);
    }
    else {
	/* On real MS-DOS, a 16 bit FAT is used whenever there would be too
	 * much clusters otherwise. */
	fs->fat_bits = (fs->clusters > MSDOS_FAT12) ? 16 : 12;
    }

    /* On FAT32, the high 4 bits of a FAT entry are reserved */
    fs->eff_fat_bits = (fs->fat_bits == 32) ? 28 : fs->fat_bits;
    fs->fat_size = fat_length*logical_sector_size;
    if (fs->clusters > ((__u64)fs->fat_size*8/fs->fat_bits)-2)
	die("File system has %d clusters but only space for %d FAT entries.",
	  fs->clusters,((__u64)fs->fat_size*8/fs->fat_bits)-2);
    if (!fs->root_entries && !fs->root_cluster)
	die("Root directory has zero size.");
    if (fs->root_entries & (MSDOS_DPS-1))
	die("Root directory (%d entries) doesn't span an integral number of "
	  "sectors.",fs->root_entries);
    if (logical_sector_size & (SECTOR_SIZE-1))
	die("Logical sector size (%d bytes) is not a multiple of the physical "
	  "sector size.",logical_sector_size);
    /* ++roman: On Atari, these two fields are often left uninitialized */
    if ((!b.secs_track || !b.heads))
	die("Invalid disk format in boot sector.");
    if (FsCheckFlags & FSCHECK_VERBOSE) dump_boot(fs,&b,logical_sector_size);
}

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
