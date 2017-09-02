/* @(#)mkisofs.h	1.152 16/12/13 joerg */
/*
 * Header file mkisofs.h - assorted structure definitions and typecasts.
 *
 * Written by Eric Youngdale (1993).
 *
 * Copyright 1993 Yggdrasil Computing, Incorporated
 * Copyright (c) 1999,2000-2016 J. Schilling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* APPLE_HYB James Pearson j.pearson@ge.ucl.ac.uk 23/2/2000 */

/* DUPLICATES_ONCE Alex Kopylov cdrtools@bootcd.ru 19.06.2004 */

#include <schily/mconfig.h>	/* Must be before stdio.h for LARGEFILE support */
#include <schily/stdio.h>
#include <schily/types.h>
#include <schily/time.h>
#include <schily/stat.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* Needed for for LARGEFILE support */
#include <schily/string.h>
#include <schily/dirent.h>
#include <schily/utypes.h>
#include <schily/standard.h>
#include <schily/nlsdefs.h>
#include <schily/ctype.h>
#include <schily/libport.h>	/* Define missing prototypes */
#include "scsi.h"

#ifdef	DVD_AUD_VID
#ifndef	UDF
#define	UDF
#endif
#endif

#ifdef	USE_LARGEFILES
/*
 * XXX Hack until fseeko()/ftello() are available everywhere or until
 * XXX we know a secure way to let autoconf ckeck for fseeko()/ftello()
 * XXX without defining FILE_OFFSETBITS to 64 in confdefs.h
 */
#	define	fseek	fseeko
#	define	ftell	ftello
#endif

#ifndef	HAVE_LSTAT
#ifndef	VMS
#define	lstat	stat
#endif
#endif

#include "iso9660.h"
#include "defaults.h"
#include <schily/siconv.h>

extern siconvt_t	*in_nls;	/* input UNICODE conversion table */
extern siconvt_t	*out_nls;	/* output UNICODE conversion table */
extern siconvt_t	*hfs_inls;	/* input HFS UNICODE conversion table */
extern siconvt_t	*hfs_onls;	/* output HFS UNICODE conversion table */

/*
 * Structure used to pass arguments via trewalk() to walkfun().
 */
struct wargs {
	void	*dir;			/* Pointer to struct directory *root */
	char	*name;			/* NULL or alternative short name    */
};

#ifdef APPLE_HYB
#include "mactypes.h"
#include "hfs.h"

struct hfs_info {
	unsigned char	finderinfo[32];
	char		name[HFS_MAX_FLEN + 1];
	/* should have fields for dates here as well */
	char		*keyname;
	struct hfs_info *next;
};

#endif	/* APPLE_HYB */

/*
 * Our version of "struct timespec".
 * Currently only used with UDF.
 */
typedef struct timesp {
	time_t	tv_sec;
	Int32_t	tv_nsec;
} timesp;

struct directory_entry {
	struct directory_entry *next;
	struct directory_entry *jnext;
	struct iso_directory_record isorec;
	unsigned int	starting_block;
	off_t		size;
	int		mxpart;		/* Extent number	  */
	unsigned short	priority;
	unsigned char	jreclen;	/* Joliet record len */
	char		*name;
	char		*table;
	char		*whole_name;
	struct directory *filedir;
	struct directory_entry *parent_rec;
	struct directory_entry *mxroot;	/* Pointer to orig entry */
	unsigned int	de_flags;
#ifdef UDF
	mode_t	mode;	/* used for udf */
	dev_t	rdev;	/* used for udf devices */
	uid_t	uid;	/* used for udf */
	gid_t	gid;	/* used for udf */
	timesp	atime;	/* timespec for atime */
	timesp	mtime;	/* timespec for mtime */
	timesp	ctime;	/* timespec for ctime */
#endif
	ino_t		inode;		/* Used in the hash table */
	dev_t		dev;		/* Used in the hash table */
	unsigned char	*rr_attributes;
	unsigned int	rr_attr_size;
	unsigned int	total_rr_attr_size;
	unsigned int	got_rr_name;
#ifdef APPLE_HYB
	struct directory_entry *assoc;	/* entry has a resource fork */
	hfsdirent	*hfs_ent;	/* HFS parameters */
	off_t		hfs_off;	/* offset to real start of fork */
	int		hfs_type;	/* type of HFS Unix file */
#endif	/* APPLE_HYB */
#ifdef SORTING
	int		sort;		/* sort weight for entry */
#endif /* SORTING */
#ifdef UDF
	int		udf_file_entry_sector;	/* also used as UDF unique ID */
#endif
#ifdef	DUPLICATES_ONCE
	unsigned char	*digest_fast;
	unsigned char	*digest_full;
#endif
};

struct file_hash {
	struct file_hash *next;
	ino_t		inode;		/* Used in the hash table */
	dev_t		dev;		/* Used in the hash table */
	nlink_t		nlink;		/* Used to compute new link count */
	unsigned int	starting_block;
	off_t		size;
#if	defined(SORTING) || defined(DUPLICATES_ONCE)
	struct directory_entry *de;
#endif /* SORTING */
};


/*
 * This structure is used to control the output of fragments to the cdrom
 * image.  Everything that will be written to the output image will eventually
 * go through this structure.   There are two pieces - first is the sizing where
 * we establish extent numbers for everything, and the second is when we actually
 * generate the contents and write it to the output image.
 *
 * This makes it trivial to extend mkisofs to write special things in the image.
 * All you need to do is hook an additional structure in the list, and the rest
 * works like magic.
 *
 * The three passes each do the following:
 *
 * The 'size' pass determines the size of each component and assigns the extent number
 * for that component.
 *
 * The 'generate' pass will adjust the contents and pointers as required now that extent
 * numbers are assigned.  In some cases, the contents of the record are also generated.
 *
 * The 'write' pass actually writes the data to the disc.
 */
struct output_fragment {
	struct output_fragment *of_next;
	int		(*of_size)	__PR((UInt32_t));
	int		(*of_generate)	__PR((void));
	int		(*of_write)	__PR((FILE *));
	char		*of_name;			/* Textual description */
	unsigned int	of_start_extent;		/* For consist check */
};

extern struct output_fragment *out_list;
extern struct output_fragment *out_tail;

extern struct output_fragment startpad_desc;
extern struct output_fragment voldesc_desc;
extern struct output_fragment xvoldesc_desc;
extern struct output_fragment joliet_desc;
extern struct output_fragment torito_desc;
extern struct output_fragment end_vol;
extern struct output_fragment version_desc;
extern struct output_fragment pathtable_desc;
extern struct output_fragment jpathtable_desc;
extern struct output_fragment dirtree_desc;
extern struct output_fragment dirtree_clean;
extern struct output_fragment jdirtree_desc;
extern struct output_fragment extension_desc;
extern struct output_fragment files_desc;
extern struct output_fragment interpad_desc;
extern struct output_fragment endpad_desc;
extern struct output_fragment sunboot_desc;
extern struct output_fragment sunlabel_desc;
extern struct output_fragment genboot_desc;
extern struct output_fragment strfile_desc;
extern struct output_fragment strdir_desc;
extern struct output_fragment strpath_desc;

#ifdef APPLE_HYB
extern struct output_fragment hfs_desc;

#endif	/* APPLE_HYB */
#ifdef DVD_AUD_VID
/*
 * This structure holds the information necessary to create a valid
 * DVD-Video image. Basically it's how much to pad the files so the
 * file offsets described in the video_ts.ifo and vts_xx_0.ifo are
 * the correct one in the image that we create.
 */
typedef struct {
	int	realsize_ifo;
	int	realsize_menu;
	int	realsize_bup;
	int	size_ifo;
	int	size_menu;
	int	size_title;
	int	size_bup;
	int	pad_ifo;
	int	pad_menu;
	int	pad_title;
	int	pad_bup;
	int	number_of_vob_files;
	int	realsize_vob[10];
} title_set_t;

typedef struct {
	int		num_titles;
	title_set_t	*title_set;
} title_set_info_t;
#endif /* DVD_AUD_VID */

/*
 * This structure describes one complete directory.  It has pointers
 * to other directories in the overall tree so that it is clear where
 * this directory lives in the tree, and it also must contain pointers
 * to the contents of the directory.  Note that subdirectories of this
 * directory exist twice in this stucture.  Once in the subdir chain,
 * and again in the contents chain.
 */
struct directory {
	struct directory *next;		/* Next directory at same level as this one */
	struct directory *subdir;	/* First subdirectory in this directory */
	struct directory *parent;
	struct directory_entry *contents;
	struct directory_entry *jcontents;
	struct directory_entry *self;
	char		*whole_name;	/* Entire source path */
	char		*de_path;	/* Entire path iside ISO-9660 */
	char		*de_name;	/* Last path name component */
	unsigned int	ce_bytes;	/* Number of bytes of CE entries read */
					/* for this dir */
	unsigned int	depth;
	unsigned int	size;
	unsigned int	extent;
	unsigned int	jsize;
	unsigned int	jextent;
	unsigned int	path_index;
	unsigned int	jpath_index;
	unsigned short	dir_flags;
	unsigned short	dir_nlink;
#ifdef APPLE_HYB
	hfsdirent	*hfs_ent;	/* HFS parameters */
	struct hfs_info	*hfs_info;	/* list of info for all entries in dir */
#endif	/* APPLE_HYB */
#ifdef SORTING
	int		sort;		/* sort weight for child files */
#endif /* SORTING */
};

struct deferred_write {
	struct deferred_write *next;
	char		*table;
	unsigned int	extent;
	off_t		size;
	char		*name;
	struct directory_entry *s_entry;
	unsigned int	pad;
	off_t		off;
	unsigned int	dw_flags;
#ifdef APPLE_HYB
	int		hfstype;
#endif
};

struct eltorito_boot_entry_info {
	struct eltorito_boot_entry_info *next;
	char		*boot_image;
	int		not_bootable;
	int		no_emul_boot;
	int		hard_disk_boot;
	int		boot_info_table;
	int		load_size;
	int		load_addr;

#define	ELTORITO_BOOT_ID	1
#define	ELTORITO_SECTION_HEADER 2
	int		type;
	/*
	 * Valid if (type & ELTORITO_BOOT_ID) != 0
	 */
	int		boot_platform;

};

typedef struct ldate {
	time_t	l_sec;
	int	l_usec;
	int	l_gmtoff;
} ldate;

extern int	goof;
extern struct directory *root;
extern struct directory *reloc_dir;
extern UInt32_t next_extent;
extern UInt32_t last_extent;
extern UInt32_t last_extent_written;
extern UInt32_t session_start;

extern unsigned int path_table_size;
extern unsigned int path_table[4];
extern unsigned int path_blocks;
extern char	*path_table_l;
extern char	*path_table_m;

extern unsigned int jpath_table_size;
extern unsigned int jpath_table[4];
extern unsigned int jpath_blocks;
extern char	*jpath_table_l;
extern char	*jpath_table_m;

extern struct iso_directory_record root_record;
extern struct iso_directory_record jroot_record;

extern int	check_oldnames;
extern int	check_session;
extern int	use_eltorito;
extern int	hard_disk_boot;
extern int	not_bootable;
extern int	no_emul_boot;
extern int	load_addr;
extern int	load_size;
extern int	boot_info_table;
extern int	use_RockRidge;
extern int	osecsize;
extern int	use_XA;
extern int	use_Joliet;
extern int	rationalize;
extern int	rationalize_uid;
extern int	rationalize_gid;
extern int	rationalize_filemode;
extern int	rationalize_dirmode;
extern uid_t	uid_to_use;
extern gid_t	gid_to_use;
extern int	filemode_to_use;
extern int	dirmode_to_use;
extern int	new_dir_mode;
extern int	follow_links;
extern int	cache_inodes;
#ifdef	DUPLICATES_ONCE
extern int	duplicates_once;
#endif
extern int	verbose;
extern int	debug;
extern int	gui;
extern int	all_files;
extern int	generate_tables;
extern int	print_size;
extern int	split_output;
extern int	use_graft_ptrs;
extern int	jhide_trans_tbl;
extern int	hide_rr_moved;
extern int	omit_period;
extern int	omit_version_number;
extern int	no_rr;
extern int	transparent_compression;
extern Uint	RR_relocation_depth;
extern int	do_largefiles;
extern off_t	maxnonlarge;
extern int	iso9660_level;
extern int	iso9660_namelen;
extern int	full_iso9660_filenames;
extern int	nolimitpathtables;
extern int	relaxed_filenames;
extern int	allow_lowercase;
extern int	allow_multidot;
extern int	iso_translate;
extern int	allow_leading_dots;
extern int	use_fileversion;
extern int	split_SL_component;
extern int	split_SL_field;
extern char	*trans_tbl;

#define	JMAX		64	/* maximum Joliet file name length (spec) */
#define	JLONGMAX	103	/* out of spec Joliet file name length */
extern int	jlen;		/* selected maximum Joliet file name length */

#ifdef DVD_AUD_VID

#define	DVD_SPEC_NONE	0x0
#define	DVD_SPEC_VIDEO	0x1
#define	DVD_SPEC_AUDIO	0x2
#define	DVD_SPEC_HYBRD	(DVD_SPEC_VIDEO | DVD_SPEC_AUDIO)
extern int	dvd_audio;
extern int	dvd_hybrid;
extern int	dvd_video;
extern int	dvd_aud_vid_flag;
#endif /* DVD_AUD_VID */


extern int	donotwrite_macpart;

#ifdef APPLE_HYB
extern int	apple_hyb;	/* create HFS hybrid */
extern int	apple_ext;	/* use Apple extensions */
extern int	apple_both;	/* common flag (for above) */
extern int	hfs_extra;	/* extra ISO extents (hfs_ce_size) */
extern hce_mem	*hce;		/* libhfs/mkisofs extras */
extern int	use_mac_name;	/* use Mac name for ISO9660/Joliet/RR */
extern int	create_dt;	/* create the Desktp files */
extern char	*hfs_boot_file;	/* name of HFS boot file */
extern char	*magic_file;	/* magic file for CREATOR/TYPE matching */
extern int	hfs_last;	/* order in which to process map/magic files */
extern char	*deftype;	/* default Apple TYPE */
extern char	*defcreator;	/* default Apple CREATOR */
extern int	gen_pt;		/* generate HFS partition table */
extern char	*autoname;	/* Autostart filename */
extern int	afe_size;	/* Apple File Exchange block size */
extern char	*hfs_volume_id;	/* HFS volume ID */
extern int	icon_pos;	/* Keep Icon position */
extern int	hfs_lock;	/* lock HFS volume (read-only) */
extern char	*hfs_bless;	/* name of folder to 'bless' (System Folder) */
extern char	*hfs_parms;	/* low level HFS parameters */

#define	MAP_LAST	1	/* process magic then map file */
#define	MAG_LAST	2	/* process map then magic file */

#ifndef PREP_BOOT
#define	PREP_BOOT
#endif	/* PREP_BOOT */

#ifdef PREP_BOOT
extern char	*prep_boot_image[4];
extern int	use_prep_boot;
extern int	use_chrp_boot;

#endif	/* PREP_BOOT */
#endif	/* APPLE_HYB */

#ifdef SORTING
extern int	do_sort;
#endif /* SORTING */

/* tree.c */
extern int stat_filter __PR((char *, struct stat *));
extern int lstat_filter __PR((char *, struct stat *));
extern int sort_tree __PR((struct directory *));
extern void attach_dot_entries __PR((struct directory * dirnode,
					struct stat * this_stat,
					struct stat * parent_stat));
extern struct directory *
		find_or_create_directory __PR((struct directory *,
				char *,
				struct directory_entry * self, int));
extern void	finish_cl_pl_entries __PR((void));
extern int	scan_directory_tree __PR((struct directory * this_dir,
				char *path,
				struct directory_entry * self));

extern int	insert_file_entry __PR((struct directory *, char *,
				char *, struct stat *, int));

extern	struct directory_entry *
		dup_directory_entry	__PR((struct directory_entry *s_entry));
extern void generate_iso9660_directories __PR((struct directory *, FILE *));
extern void dump_tree __PR((struct directory * node));
extern struct directory_entry *search_tree_file __PR((struct
				directory * node, char *filename));
extern void init_fstatbuf __PR((void));
extern struct stat root_statbuf;
extern struct stat fstatbuf;

/* eltorito.c */
extern void init_boot_catalog __PR((const char *path));
extern void insert_boot_cat __PR((void));
extern void get_boot_entry	__PR((void));
extern int  new_boot_entry	__PR((void));
extern void ex_boot_enoent	__PR((char *msg, char *pname));

/* boot.c */
extern int sparc_boot_label __PR((char *label));
extern int sunx86_boot_label __PR((char *label));
extern int scan_sparc_boot __PR((char *files));
extern int scan_sunx86_boot __PR((char *files));
extern int make_sun_label __PR((void));
extern int make_sunx86_label __PR((void));

/* isonum.c */
extern void set_721 __PR((void *, UInt32_t));
extern void set_722 __PR((void *, UInt32_t));
extern void set_723 __PR((void *, UInt32_t));
extern void set_731 __PR((void *, UInt32_t));
extern void set_732 __PR((void *, UInt32_t));
extern void set_733 __PR((void *, UInt32_t));

extern UInt32_t get_711 __PR((void *));
extern UInt32_t get_721 __PR((void *));
extern UInt32_t get_723 __PR((void *));
extern UInt32_t get_731 __PR((void *));
extern UInt32_t get_732 __PR((void *));
extern UInt32_t get_733 __PR((void *));

/* write.c */
extern int sort_directory __PR((struct directory_entry **, int));
extern void generate_one_directory __PR((struct directory *, FILE *));
extern void memcpy_max __PR((char *, char *, int));
extern int oneblock_size __PR((UInt32_t starting_extent));
extern struct iso_primary_descriptor vol_desc;
extern void xfwrite __PR((void *buffer, int size, int count, FILE * file, int submode, BOOL islast));
extern void outputlist_insert __PR((struct output_fragment * frag));

#ifdef APPLE_HYB
extern Ulong get_adj_size __PR((int Csize));
extern int adj_size __PR((int Csize, UInt32_t start_extent, int extra));
extern void adj_size_other __PR((struct directory * dpnt));
extern int insert_padding_file __PR((int size));
extern int gen_mac_label __PR((struct deferred_write *));

#ifdef PREP_BOOT
extern void gen_prepboot_label __PR((unsigned char *));

#endif	/* PREP_BOOT */
#endif	/* APPLE_HYB */

/* multi.c */

extern FILE	*in_image;
extern BOOL	ignerr;
extern int open_merge_image __PR((char *path));
extern int close_merge_image __PR((void));
extern struct iso_directory_record *
			merge_isofs __PR((char *path));
extern unsigned char	*parse_xa __PR((unsigned char *pnt, int *lenp,
				struct directory_entry * dpnt));
extern int	rr_flags	__PR((struct iso_directory_record *idr));
extern int merge_previous_session __PR((struct directory *,
				struct iso_directory_record *, char *, char *));
extern int get_session_start __PR((int *));

/* joliet.c */
#ifdef	UDF
extern	void	convert_to_unicode	__PR((unsigned char *buffer,
			int size, char *source, siconvt_t *inls));
extern	int	joliet_strlen		__PR((const char *string, size_t maxlen,
						siconvt_t *inls));
#endif
extern void conv_charset __PR((unsigned char *to, size_t *tosizep,
				unsigned char *from, size_t *fromsizep,
				siconvt_t *,
				siconvt_t *));
extern int joliet_sort_tree __PR((struct directory * node));

/* match.c */
extern int matches __PR((char *));
extern int add_match __PR((char *));

/* files.c */
struct dirent	*readdir_add_files __PR((char **, char *, DIR *));

/* name.c */

extern void iso9660_check	__PR((struct iso_directory_record *idr, struct directory_entry *ndr));
extern int iso9660_file_length __PR((const char *name,
				struct directory_entry * sresult, int flag));

/* various */
extern int iso9660_date __PR((char *, time_t));
extern int iso9660_ldate __PR((char *, time_t, int, int));
extern void add_hash __PR((struct directory_entry *));
extern struct file_hash *find_hash __PR((struct directory_entry *spnt));

extern void flush_hash __PR((void));
extern void add_directory_hash __PR((dev_t, ino_t));
extern struct file_hash *find_directory_hash __PR((dev_t, ino_t));
extern void flush_file_hash __PR((void));
extern int delete_file_hash __PR((struct directory_entry *));
extern struct directory_entry *find_file_hash __PR((char *));
extern void add_file_hash __PR((struct directory_entry *));

extern int	generate_xa_rr_attributes __PR((char *, char *,
				struct directory_entry *,
				struct stat *, struct stat *,
				int deep_flag));
extern char	*generate_rr_extension_record __PR((char *id,
				char *descriptor,
				char *source, int *size));

extern int	check_prev_session __PR((struct directory_entry **, int len,
				struct directory_entry *,
				struct stat *,
				struct stat *,
				struct directory_entry **));

extern void	match_cl_re_entries __PR((void));
extern void	finish_cl_pl_for_prev_session __PR((void));
extern char	*find_rr_attribute __PR((unsigned char *pnt, int len, char *attr_type));

extern void	udf_set_extattr_macresfork __PR((unsigned char *buf, off_t size, unsigned rba));
extern void	udf_set_extattr_freespace __PR((unsigned char *buf, off_t size, unsigned rba));
extern int	udf_get_symlinkcontents __PR((char *, char *, off_t *));

/* inode.c */
extern	void	do_inode		__PR((struct directory *dpnt));
extern	void	do_dir_nlink		__PR((struct directory *dpnt));

#ifdef APPLE_HYB
/* volume.c */
extern int make_mac_volume __PR((struct directory * dpnt, UInt32_t start_extent));
extern int write_fork __PR((hfsfile * hfp, long tot));

/* apple.c */

extern void del_hfs_info __PR((struct hfs_info *));
extern int get_hfs_dir __PR((char *, char *, struct directory_entry *));
extern int get_hfs_info __PR((char *, char *, struct directory_entry *));
extern int get_hfs_rname __PR((char *, char *, char *));
extern int hfs_exclude __PR((char *));
extern void print_hfs_info __PR((struct directory_entry *));
extern void hfs_init __PR((char *, unsigned short, unsigned int));
extern void delete_rsrc_ent __PR((struct directory_entry *));
extern void clean_hfs __PR((void));
extern void perr __PR((char *));
extern void set_root_info __PR((char *));
extern int file_is_resource __PR((char *fname, int hfstype));
extern int hfs_excludepath __PR((char *));

/* desktop.c */

extern int make_desktop __PR((hfsvol *, int));

/* mac_label.c */

#ifdef	_MAC_LABEL_H
#ifdef PREP_BOOT
extern void	gen_prepboot_label __PR((MacLabel * mac_label));
#endif
extern int	gen_mac_label __PR((defer *));
#endif
extern int	autostart __PR((void));

/* libfile */

extern char	*get_magic_match __PR((const char *));
extern void	clean_magic __PR((void));

#endif	/* APPLE_HYB */

#ifdef	USE_FIND
/*
 * The callback function for treewalk() from walk.c
 */
#ifdef	_SCHILY_WALK_H
EXPORT	int	walkfunc	__PR((char *nm, struct stat *fs, int type,
					struct WALK *state));
#endif
#endif

extern char	*extension_record;
extern UInt32_t	extension_record_extent;
/*extern int	n_data_extents;*/
extern	BOOL	archive_isreg;
extern	dev_t	archive_dev;
extern	ino_t	archive_ino;

/*
 * These are a few goodies that can be specified on the command line, and are
 * filled into the root record
 */
extern char	*preparer;
extern char	*publisher;
extern char	*copyright;
extern char	*biblio;
extern char	*abstract;
extern char	*appid;
extern char	*volset_id;
extern char	*system_id;
extern char	*volume_id;
extern char	*boot_catalog;
extern char	*boot_image;
extern char	*genboot_image;
extern int	ucs_level;
extern int	volume_set_size;
extern int	volume_sequence_number;

extern struct eltorito_boot_entry_info *first_boot_entry;
extern struct eltorito_boot_entry_info *last_boot_entry;
extern struct eltorito_boot_entry_info *current_boot_entry;

extern	UInt32_t null_inodes;
extern	BOOL	correct_inodes;
extern	BOOL	rrip112;
extern	BOOL	long_rr_time;	/* TRUE: use long (17 Byte) time format	    */

extern char	*findgequal	__PR((char *));
extern void	*e_malloc	__PR((size_t));
extern char	*e_strdup	__PR((const char *));

/*
 * Note: always use these macros to avoid problems.
 *
 * ISO_ROUND_UP(X)	may cause an integer overflow and thus give
 *			incorrect results. So avoid it if possible.
 *
 * ISO_BLOCKS(X)	is overflow safe. Prefer this when ever it is possible.
 */
#define	SECTOR_SIZE	(2048)
#define	ISO_ROUND_UP(X)	(((X) + (SECTOR_SIZE - 1)) & ~(SECTOR_SIZE - 1))
#define	ISO_BLOCKS(X)	(((X) / SECTOR_SIZE) + (((X)%SECTOR_SIZE)?1:0))

#define	ROUND_UP(X, Y)	(((X + (Y - 1)) / Y) * Y)

#ifdef APPLE_HYB
/*
 * ISO blocks == 2048, HFS blocks == 512
 */
#define	HFS_BLK_CONV	(SECTOR_SIZE/HFS_BLOCKSZ)

#define	HFS_ROUND_UP(X)	ISO_ROUND_UP(((X)*HFS_BLOCKSZ))	/* XXX ??? */
#define	HFS_BLOCKS(X)	(ISO_BLOCKS(X) * HFS_BLK_CONV)

#define	USE_MAC_NAME(E)	(use_mac_name && ((E)->hfs_ent != NULL) && (E)->hfs_type)
#endif	/* APPLE_HYB */

/*
 * Inode and device values for special purposes.
 */
#define	PREV_SESS_DEV	((dev_t)-2)
#define	TABLE_INODE	((ino_t)-2)
#define	UNCACHED_INODE	((ino_t)-1)
#define	UNCACHED_DEVICE	((dev_t)-1)

/*
 * The highest value used for the inodes we assign to files that do not have
 * a starting block address (zero length files, symlinks, dev nodes, pipes,
 * socket).
 * We need to make sure that these numbers are valid ISO-9660 block addresses,
 * this is why we use unsigned 32-bit integer values.
 * We need to make sure that the inode numbers assigned for zero sized files
 * is in a proper range, this is why we use numbers above the range of block
 * addresses we use in the image. We start counting backwards from 0xFFFFFFF0
 * to leave enough space for special numbers from the range listed above.
 */
#define	NULL_INO_MAX	((UInt32_t)0xFFFFFFF0)

#ifdef VMS
#define	STAT_INODE(X)	(X.st_ino[0])
#define	PATH_SEPARATOR	']'
#define	SPATH_SEPARATOR	""
#else
#define	STAT_INODE(X)	(X.st_ino)
#define	PATH_SEPARATOR	'/'
#define	SPATH_SEPARATOR	"/"
#endif

/*
 * When using multi-session, indicates that we can reuse the
 * TRANS.TBL information for this directory entry. If this flag
 * is set for all entries in a directory, it means we can just
 * reuse the TRANS.TBL and not generate a new one.
 */
#define	SAFE_TO_REUSE_TABLE_ENTRY  0x01		/* de_flags only  */
#define	DIR_HAS_DOT		   0x02		/* dir_flags only */
#define	DIR_HAS_DOTDOT		   0x04		/* dir_flags only */
#define	INHIBIT_JOLIET_ENTRY	   0x08
#define	INHIBIT_RR_ENTRY	   0x10		/* not used	  */
#define	RELOCATED_DIRECTORY	   0x20		/* de_flags only  */
#define	INHIBIT_ISO9660_ENTRY	   0x40
#define	MEMORY_FILE		   0x80		/* de_flags only  */
#define	HIDDEN_FILE		   0x100	/* de_flags only  */
#define	DIR_WAS_SCANNED		   0x200	/* dir_flags only */
#define	RESOURCE_FORK		   0x400	/* de_flags only  */
#define	IS_SYMLINK		   0x800	/* de_flags only  */
#define	MULTI_EXTENT		   0x1000	/* de_flags only  */
#define	INHIBIT_UDF_ENTRY	   0x2000

/*
 * Volume sequence number to use in all of the iso directory records.
 */
#define	DEF_VSN		1

/*
 * Make sure we have a definition for this.  If not, take a very conservative
 * guess.
 * POSIX requires the max pathname component lenght to be defined in limits.h
 * If variable, it may be undefined. If undefined, there should be
 * a definition for _POSIX_NAME_MAX in limits.h or in unistd.h
 * As _POSIX_NAME_MAX is defined to 14, we cannot use it.
 * XXX Eric's wrong comment:
 * XXX From what I can tell SunOS is the only one with this trouble.
 */
#include <schily/limits.h>

#ifndef NAME_MAX
#ifdef FILENAME_MAX
#define	NAME_MAX	FILENAME_MAX
#else
#define	NAME_MAX	256
#endif
#endif

#ifndef PATH_MAX
#ifdef FILENAME_MAX
#define	PATH_MAX	FILENAME_MAX
#else
#define	PATH_MAX	1024
#endif
#endif

/*
 * Cygwin seems to have PATH_MAX == 260 which is less than the usable
 * path length. We raise PATH_MAX to at least 1024 for now for all platforms
 * unless someone reports problems with mkisofs memory size.
 */
#if	PATH_MAX < 1024
#undef	PATH_MAX
#define	PATH_MAX	1024
#endif

/*
 * XXX JS: Some structures have odd lengths!
 * Some compilers (e.g. on Sun3/mc68020) padd the structures to even length.
 * For this reason, we cannot use sizeof (struct iso_path_table) or
 * sizeof (struct iso_directory_record) to compute on disk sizes.
 * Instead, we use offsetof(..., name) and add the name size.
 * See iso9660.h
 */
#ifndef	offsetof
#define	offsetof(TYPE, MEMBER)	((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifdef UDF
extern int	use_udf;
#endif
extern int	create_udfsymlinks;

#if !defined(HAVE_MEMSET) && !defined(memset)
#define	memset(s, c, n)		fillbytes(s, n, c)
#endif
#if !defined(HAVE_MEMCHR) && !defined(memchr)
#define	memchr(s, c, n)		findbytes(s, n, c)
#endif
#if !defined(HAVE_MEMCPY) && !defined(memcpy)
#define	memcpy(s1, s2, n)	movebytes(s2, s1, n)
#endif
#if !defined(HAVE_MEMMOVE) && !defined(memmove)
#define	memmove(s1, s2, n)	movebytes(s2, s1, n)
#endif
