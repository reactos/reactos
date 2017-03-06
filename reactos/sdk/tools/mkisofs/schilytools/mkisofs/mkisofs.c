/* @(#)mkisofs.c	1.289 17/01/05 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)mkisofs.c	1.289 17/01/05 joerg";
#endif
/*
 * Program mkisofs.c - generate iso9660 filesystem  based upon directory
 * tree on hard disk.
 *
 * Written by Eric Youngdale (1993).
 *
 * Copyright 1993 Yggdrasil Computing, Incorporated
 * Copyright (c) 1997-2017 J. Schilling
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

/* APPLE_HYB James Pearson j.pearson@ge.ucl.ac.uk 22/2/2000 */
/* MAC UDF images by HELIOS Software GmbH support@helios.de */
/* HFS+ by HELIOS Software GmbH support@helios.de */

/* DUPLICATES_ONCE Alex Kopylov cdrtools@bootcd.ru 19.06.2004 */

#ifdef	USE_FIND
#include <schily/walk.h>
#include <schily/find.h>
#endif
#include "mkisofs.h"
#include "rock.h"
#include <schily/errno.h>
#include <schily/time.h>
#include <schily/fcntl.h>
#include <schily/ctype.h>
#include "match.h"
#include <schily/schily.h>
#include <schily/nlsdefs.h>
#include <schily/checkerr.h>
#ifdef UDF
#include "udf.h"
#endif

#include <schily/io.h>				/* for setmode() prototype */
#include <schily/getargs.h>

#ifdef VMS
#include "vms.h"
#endif

#ifdef	no_more_needed
#include <schily/resource.h>
#endif	/* no_more_needed */

#include "../cdrecord/version.h"

struct directory *root = NULL;
int		path_ind;

char	version_string[] = VERSION;

char		*outfile;
FILE		*discimage;
UInt32_t	next_extent	= 0;
UInt32_t	last_extent	= 0;
UInt32_t	session_start	= 0;
unsigned int	path_table_size	= 0;
unsigned int	path_table[4]	= {0, };
unsigned int	path_blocks	= 0;


unsigned int	jpath_table_size = 0;
unsigned int	jpath_table[4]	= {0, };
unsigned int	jpath_blocks	= 0;

struct iso_directory_record root_record;
struct iso_directory_record jroot_record;

char	*extension_record = NULL;
UInt32_t extension_record_extent = 0;
int	extension_record_size = 0;
BOOL	archive_isreg;
dev_t	archive_dev;
ino_t	archive_ino;

/* These variables are associated with command line options */
int	check_oldnames = 0;
int	check_session = 0;
int	use_eltorito = 0;
int	hard_disk_boot = 0;
int	not_bootable = 0;
int	no_emul_boot = 0;
int	load_addr = 0;
int	load_size = 0;
int	boot_info_table = 0;
int	use_sparcboot = 0;
int	use_sunx86boot = 0;
int	use_genboot = 0;
int	use_RockRidge = 0;
int	use_XA = 0;
int	osecsize = 0;	/* Output-sector size, 0 means default secsize 2048 */
int	use_Joliet = 0;
int	jlen = JMAX;	/* maximum Joliet file name length */
/*
 *	Verbose levels currently used:
 *
 *	1+	Boot information
 *	1+	Rcfile information
 *	1+	Name mapping information
 *	1+	Progress information
 *	2+	Version informaton
 *	2+	Output Extent (of_write) information
 *	2+	PReP boot information
 *	3+	HFS warnings
 *	3+	Dump dirtree
 */
int	verbose = 1;
int	debug = 0;
int	gui = 0;
BOOL	legacy = FALSE;		/* Implement legacy support for historic CLI */
int	all_files = 1;		/* New default is to include all files */
BOOL	Hflag = FALSE;		/* Follow links on cmdline (-H)	*/
BOOL	follow_links = FALSE;	/* Follow all links (-L)	*/
#if	defined(__MINGW32__) || defined(_MSC_VER)
/*
 * Never cache inodes on DOS or Win-DOS.
 */
int	cache_inodes = 0;
#else
int	cache_inodes = -1;	/* Cache inodes if OS has unique inodes */
#endif
int	rationalize = 0;	/* Need to call stat_fix()	*/
int	rationalize_uid = 0;
int	rationalize_gid = 0;
int	rationalize_filemode = 0;
int	rationalize_dirmode = 0;
uid_t	uid_to_use = 0;		/* when rationalizing uid */
gid_t	gid_to_use = 0;		/* when rationalizing gid */
int	filemode_to_use = 0;	/* if non-zero, when rationalizing file mode */
int	dirmode_to_use = 0;	/* if non-zero, when rationalizing dir mode */
int	new_dir_mode = 0555;	/* neither -new-dir-mode nor -dir-mode used */
int	generate_tables = 0;
int	dopad = 1;	/* Now default to do padding */
int	print_size = 0;
int	split_output = 0;
char	*icharset = NULL;	/* input charset to convert to UNICODE */
char	*ocharset = NULL;	/* output charset to convert from UNICODE */
char	*preparer = PREPARER_DEFAULT;
char	*publisher = PUBLISHER_DEFAULT;
char	*appid = APPID_DEFAULT;
char	*copyright = COPYRIGHT_DEFAULT;
char	*biblio = BIBLIO_DEFAULT;
char	*abstract = ABSTRACT_DEFAULT;
char	*volset_id = VOLSET_ID_DEFAULT;
char	*volume_id = VOLUME_ID_DEFAULT;
char	*system_id = SYSTEM_ID_DEFAULT;
char	*boot_catalog;
char	*boot_image = BOOT_IMAGE_DEFAULT;
char	*genboot_image = BOOT_IMAGE_DEFAULT;
int	ucs_level = 3;		/* We now have Unicode tables so use level 3 */
int	volume_set_size = 1;
int	volume_sequence_number = 1;
/* -------------------------------------------------------------------------- */
char	*merge_image;		/* CLI Parameter for -M option		    */
char	*check_image;		/* CLI Parameter for -check-session option  */
char	*reloc_root = NULL;	/* CLI Parameter for -root option	    */
char	*reloc_old_root = NULL;	/* CLI Parameter for -oldroot option	    */
extern char	*cdrecord_data;	/* CLI Parameter for -C option		    */
int	disable_deep_reloc;	/* CLI Parameter for -D option		    */
char	*dirmode_str;		/* CLI Parameter for -dir-mode option	    */
char	*filemode_str;		/* CLI Parameter for -file-mode option	    */
char	*gid_str;		/* CLI Parameter for -gid option	    */
int	help;			/* CLI Parameter for -help option	    */
int	joliet_long;		/* CLI Parameter for -joliet-long option    */
char	*jcharset;		/* CLI Parameter for -jcharset option	    */
int	max_filenames;		/* CLI Parameter for -max-iso9660-filenames option */
char	*log_file;		/* CLI Parameter for -log-file option	    */
char	*new_dirmode_str;	/* CLI Parameter for -new-dir-mode option   */
char	*pathnames;		/* CLI Parameter for -help option	    */
int	rationalize_rr;		/* CLI Parameter for -path-list option	    */
char	*sectype;		/* CLI Parameter for -s option		    */
char	*uid_str;		/* CLI Parameter for -uid option	    */
int	untranslated_filenames;	/* CLI Parameter for -U option		    */
int	pversion;		/* CLI Parameter for -version option	    */
int	rationalize_xa;		/* CLI Parameter for -xa option		    */
ldate	modification_date;	/* CLI Parameter for -modification-date	    */
#ifdef	APPLE_HYB
char	*afpfile = "";		/* CLI Parameter for -map option	    */
char	*root_info;		/* CLI Parameter for -root-info option	    */
#endif
BOOL	nodesc = FALSE;		/* Whether not to descend directories	    */
#ifdef	USE_FIND
BOOL	dofind	  = FALSE;	/* -find option found		*/
int	find_ac	  = 0;		/* ac past -find option		*/
char	*const *find_av = NULL;	/* av past -find option		*/
int	find_pac  = 0;		/* ac for first find primary	*/
char	*const *find_pav = NULL; /* av for first find primary	*/
findn_t	*find_node;		/* syntaxtree from find_parse()	*/
void	*plusp;			/* residual for -exec ...{} +	*/
int	find_patlen;		/* len for -find pattern state	*/

LOCAL 	int		walkflags = WALK_CHDIR | WALK_PHYS | WALK_NOEXIT;
LOCAL	int		maxdepth = -1;
LOCAL	int		mindepth = -1;
EXPORT	struct WALK	walkstate;
#else
LOCAL 	int		walkflags = 0;
#endif

extern	time_t		begun;
extern	struct timeval	tv_begun;

LOCAL	BOOL		data_change_warn;

struct eltorito_boot_entry_info *first_boot_entry = NULL;
struct eltorito_boot_entry_info *last_boot_entry = NULL;
struct eltorito_boot_entry_info *current_boot_entry = NULL;

int	use_graft_ptrs;		/* Use graft points */
int	match_igncase;		/* Ignore case with -exclude-list and -hide* */
int	jhide_trans_tbl;	/* Hide TRANS.TBL from Joliet tree */
int	hide_rr_moved;		/* Name RR_MOVED .rr_moved in Rock Ridge tree */
int	omit_period = 0;	/* Violates iso9660, but these are a pain */
int	transparent_compression = 0; /* So far only works with linux */
int	omit_version_number = 0; /* May violate iso9660, but noone uses vers */
int	no_rr = 0;		/* Do not use RR attributes from old session */
int	force_rr = 0;		/* Force to use RR attributes from old session */
Uint	RR_relocation_depth = 6; /* Violates iso9660, but most systems work */
int	do_largefiles = 0;	/* Whether to allow multi-extent files */
off_t	maxnonlarge = (off_t)0xFFFFFFFF;
int	iso9660_level = 1;
int	iso9660_namelen = LEN_ISONAME; /* 31 characters, may be set to 37 */
int	full_iso9660_filenames = 0; /* Full 31 character iso9660 filenames */
int	nolimitpathtables = 0;	/* Don't limit size of pathtable. Violates iso9660 */
int	relaxed_filenames = 0;	/* For Amiga.  Disc will not work with DOS */
int	allow_lowercase = 0;	/* Allow lower case letters */
int	no_allow_lowercase = 0;	/* Do not allow lower case letters */
int	allow_multidot = 0;	/* Allow more than on dot in filename */
int	iso_translate = 1;	/* 1 == enables '#', '-' and '~' removal */
int	allow_leading_dots = 0;	/* DOS cannot read names with leading dots */
#ifdef	VMS
int	use_fileversion = 1;	/* Use file version # from filesystem */
#else
int	use_fileversion = 0;	/* Use file version # from filesystem */
#endif
int	split_SL_component = 1;	/* circumvent a bug in the SunOS driver */
int	split_SL_field = 1;	/* circumvent a bug in the SunOS */
char	*trans_tbl;		/* default name for translation table */
int	stream_media_size = 0;	/* # of blocks on the media */
char	*stream_filename;	/* Default stream file name */

#ifdef APPLE_HYB
int	donotwrite_macpart = 0;	/* Do not write "hfs" hybrid with UDF */
int	apple_hyb = 0;		/* -hfs HFS hybrid flag */
int	no_apple_hyb = 0;	/* -no-hfs HFS hybrid flag */
int	apple_ext = 0;		/* create HFS extensions flag */
int	apple_both = 0;		/* common flag (for above) */
int	hfs_extra = 0;		/* extra HFS blocks added to end of ISO vol */
int	use_mac_name = 0;	/* use Mac name for ISO/Joliet/RR flag */
hce_mem	*hce;			/* libhfs/mkisofs extras */
char	*hfs_boot_file = 0;	/* name of HFS boot file */
int	gen_pt = 0;		/* generate HFS partition table */
char	*autoname = 0;		/* AutoStart filename */
char	*magic_file = 0;	/* name of magic file */
int	probe = 0;		/* search files for HFS/Unix type */
int	nomacfiles = 0;		/* don't look for Mac/Unix files */
int	hfs_select = 0;		/* Mac/Unix types to select */
int	create_dt = 1;		/* create the Desktp files */
int	afe_size = 0;		/* Apple File Exchange block size */
int	hfs_last = MAG_LAST;	/* process magic file after map file */
char	*deftype;		/* default Apple TYPE */
char	*defcreator;		/* default Apple CREATOR */
char	*hfs_volume_id = NULL;	/* HFS volume ID */
int	icon_pos = 0;		/* Keep icon position */
char	*hfs_icharset = NULL;	/* input HFS charset name */
char    *hfs_ocharset = NULL;	/* output HFS charset name */
int	hfs_lock = 1;		/* lock HFS volume (read-only) */
char	*hfs_bless = NULL;	/* name of folder to 'bless' (System Folder) */
char	*hfs_parms = NULL;	/* low level HFS parameters */

#ifdef PREP_BOOT
char	*prep_boot_image[4];
int	use_prep_boot = 0;
int	use_chrp_boot = 0;
#endif	/* PREP_BOOT */
#else	/* APPLE_HYB */
int	donotwrite_macpart = 1;	/* Do not write "hfs" hybrid with UDF */
#endif	/* APPLE_HYB */

#ifdef UDF
int	rationalize_udf = 0;	/* -udf (rationalized UDF)	*/
int	use_udf = 0;		/* -udf or -UDF			*/
int	create_udfsymlinks = 1;	/* include symlinks in UDF	*/
#endif

#ifdef DVD_AUD_VID
int	dvd_audio = 0;
int	dvd_hybrid = 0;
int	dvd_video = 0;
int	dvd_aud_vid_flag = DVD_SPEC_NONE;
#endif

#ifdef SORTING
int	do_sort = 0;		/* sort file data */
#endif /* SORTING */

/*
 * inode numbers for zero sized files start from this number and count
 * backwards. This is done to allow unique inode numbers even on multi-session
 * disks.
 */
UInt32_t null_inodes = NULL_INO_MAX;
BOOL	correct_inodes = TRUE;	/* TRUE: add a "correct inodes" fingerprint */
BOOL	rrip112 = TRUE;		/* TRUE: create Rock Ridge V 1.12	    */
BOOL	long_rr_time = FALSE;	/* TRUE: use long (17 Byte) time format	    */

#ifdef	DUPLICATES_ONCE
int	duplicates_once = 0;	/* encode duplicate files once */
#endif

siconvt_t	*in_nls = NULL;  /* input UNICODE conversion table */
siconvt_t	*out_nls = NULL; /* output UNICODE conversion table */
#ifdef APPLE_HYB
siconvt_t	*hfs_inls = NULL; /* input HFS UNICODE conversion table */
siconvt_t	*hfs_onls = NULL; /* output HFS UNICODE conversion table */
#endif /* APPLE_HYB */

struct rcopts {
	char		*tag;
	char		**variable;
};

struct rcopts rcopt[] = {
	{"PREP", &preparer},
	{"PUBL", &publisher},
	{"APPI", &appid},
	{"COPY", &copyright},
	{"BIBL", &biblio},
	{"ABST", &abstract},
	{"VOLS", &volset_id},
	{"VOLI", &volume_id},
	{"SYSI", &system_id},
#ifdef APPLE_HYB
	{"HFS_TYPE", &deftype},
	{"HFS_CREATOR", &defcreator},
#endif	/* APPLE_HYB */
	{NULL, NULL}
};

#ifdef	USE_FIND
LOCAL	int	getfind		__PR((char *arg, long *valp,
					int *pac, char *const **pav));
#endif

LOCAL	int	getH		__PR((const char *arg, void *valp, int *pac, char *const **pav, const char *opt));
LOCAL	int	getL		__PR((const char *arg, void *valp, int *pac, char *const **pav, const char *opt));
LOCAL	int	getP		__PR((const char *arg, void *valp, int *pac, char *const **pav, const char *opt));
LOCAL	int	dolegacy	__PR((const char *arg, void *valp, int *pac, char *const **pav, const char *opt));

LOCAL	int	get_boot_image	__PR((char *opt_arg));
LOCAL	int	get_hd_boot	__PR((char *opt_arg));
LOCAL	int	get_ne_boot	__PR((char *opt_arg));
LOCAL	int	get_no_boot	__PR((char *opt_arg));
LOCAL	int	get_boot_addr	__PR((char *opt_arg));
LOCAL	int	get_boot_size	__PR((char *opt_arg));
LOCAL	int	get_boot_platid	__PR((char *opt_arg));
LOCAL	int	get_boot_table	__PR((char *opt_arg));
#ifdef	APPLE_HYB
#ifdef PREP_BOOT
LOCAL	int	get_prep_boot	__PR((char *opt_arg));
LOCAL	int	get_chrp_boot	__PR((char *opt_arg));
#endif
LOCAL	int	get_bsize	__PR((char *opt_arg));

LOCAL	int	hfs_cap		__PR((void));
LOCAL	int	hfs_neta	__PR((void));
LOCAL	int	hfs_dbl		__PR((void));
LOCAL	int	hfs_esh		__PR((void));
LOCAL	int	hfs_fe		__PR((void));
LOCAL	int	hfs_sgi		__PR((void));
LOCAL	int	hfs_mbin	__PR((void));
LOCAL	int	hfs_sgl		__PR((void));
LOCAL	int	hfs_dave	__PR((void));
LOCAL	int	hfs_sfm		__PR((void));
LOCAL	int	hfs_xdbl	__PR((void));
LOCAL	int	hfs_xhfs	__PR((void));
LOCAL	int	hfs_nohfs	__PR((void));
#endif	/* APPLE_HYB */

LOCAL	void	ldate_error	__PR((char *arg));
LOCAL	char	*strntoi	__PR((char *p, int n, int *ip));
LOCAL	int	mosize		__PR((int y, int m));
LOCAL	char	*parse_date	__PR((char *arg, struct tm *tp));
LOCAL	int	get_ldate	__PR((char *opt_arg, void *valp));

LOCAL int
get_boot_image(opt_arg)
	char	*opt_arg;
{
	do_sort++;		/* We sort bootcat/botimage */
	use_eltorito++;
	boot_image = opt_arg;	/* pathname of the boot image */
					/* on disk */
	if (boot_image == NULL || *boot_image == '\0') {
		comerrno(EX_BAD,
		_("Required Eltorito boot image pathname missing\n"));
	}
	get_boot_entry();
	current_boot_entry->boot_image = boot_image;
	return (1);
}

LOCAL int
get_hd_boot(opt_arg)
	char	*opt_arg;
{
	use_eltorito++;
	hard_disk_boot++;
	get_boot_entry();
	current_boot_entry->hard_disk_boot = 1;
	return (1);
}

LOCAL int
get_ne_boot(opt_arg)
	char	*opt_arg;
{
	use_eltorito++;
	no_emul_boot++;
	get_boot_entry();
	current_boot_entry->no_emul_boot = 1;
	return (1);
}

LOCAL int
get_no_boot(opt_arg)
	char	*opt_arg;
{
	use_eltorito++;
	not_bootable++;
	get_boot_entry();
	current_boot_entry->not_bootable = 1;
	return (1);
}

LOCAL int
get_boot_addr(opt_arg)
	char	*opt_arg;
{
	long	val;
	char	*ptr;

	use_eltorito++;
	val = strtol(opt_arg, &ptr, 0);
	if (*ptr || val < 0 || val >= 0x10000) {
		comerrno(EX_BAD, _("Boot image load address invalid.\n"));
	}
	load_addr = val;
	get_boot_entry();
	current_boot_entry->load_addr = load_addr;
	return (1);
}

LOCAL int
get_boot_size(opt_arg)
	char	*opt_arg;
{
	long	val;
	char	*ptr;

	use_eltorito++;
	val = strtol(opt_arg, &ptr, 0);
	if (*ptr || val < 0 || val >= 0x10000) {
		comerrno(EX_BAD,
		_("Boot image load size invalid.\n"));
	}
	load_size = val;
	get_boot_entry();
	current_boot_entry->load_size = load_size;
	return (1);
}

LOCAL int
get_boot_platid(opt_arg)
	char	*opt_arg;
{
	long	val;
	char	*ptr;

	use_eltorito++;
	if (streql(opt_arg, "x86")) {
		val = EL_TORITO_ARCH_x86;
	} else if (streql(opt_arg, "PPC")) {
		val = EL_TORITO_ARCH_PPC;
	} else if (streql(opt_arg, "Mac")) {
		val = EL_TORITO_ARCH_MAC;
	} else if (streql(opt_arg, "efi")) {
		val = EL_TORITO_ARCH_EFI;
	} else {
		val = strtol(opt_arg, &ptr, 0);
		if (*ptr || val < 0 || val >= 0x100) {
			comerrno(EX_BAD, _("Bad boot system ID.\n"));
		}
	}

	/*
	 * If there is already a boot entry and the boot file name has been set
	 * for this boot entry and the new platform id differs from the
	 * previous value, we start a new boot section.
	 */
	if (current_boot_entry &&
	    current_boot_entry->boot_image != NULL &&
	    current_boot_entry->boot_platform != val) {
		new_boot_entry();
	}
	get_boot_entry();
	current_boot_entry->type |= ELTORITO_BOOT_ID;
	current_boot_entry->boot_platform = val;
	return (1);
}

LOCAL int
get_boot_table(opt_arg)
	char	*opt_arg;
{
	use_eltorito++;
	boot_info_table++;
	get_boot_entry();
	current_boot_entry->boot_info_table = 1;
	return (1);
}

#ifdef	APPLE_HYB
#ifdef PREP_BOOT
LOCAL int
get_prep_boot(opt_arg)
	char	*opt_arg;
{
	use_prep_boot++;
	if (use_prep_boot > 4 - use_chrp_boot) {
		comerrno(EX_BAD,
		_("Maximum of 4 PRep+CHRP partition entries are allowed\n"));
	}
	/* pathname of the boot image on cd */
	prep_boot_image[use_prep_boot - 1] = opt_arg;
	if (prep_boot_image[use_prep_boot - 1] == NULL) {
		comerrno(EX_BAD,
		_("Required PReP boot image pathname missing\n"));
	}
	return (1);
}

LOCAL int
get_chrp_boot(opt_arg)
	char	*opt_arg;
{
	if (use_chrp_boot)
		return (1);		/* silently allow duplicates */
	use_chrp_boot = 1;
	if (use_prep_boot > 3) {
		comerrno(EX_BAD,
		_("Maximum of 4 PRep+CHRP partition entries are allowed\n"));
	}
	return (1);
}
#endif	/* PREP_BOOT */


LOCAL int
get_bsize(opt_arg)
	char	*opt_arg;
{
	afe_size = atoi(opt_arg);
	hfs_select |= DO_FEU;
	hfs_select |= DO_FEL;
	return (1);
}

LOCAL int
hfs_cap()
{
	hfs_select |= DO_CAP;
	return (1);
}

LOCAL int
hfs_neta()
{
	hfs_select |= DO_NETA;
	return (1);
}

LOCAL int
hfs_dbl()
{
	hfs_select |= DO_DBL;
	return (1);
}

LOCAL int
hfs_esh()
{
	hfs_select |= DO_ESH;
	return (1);
}

LOCAL int
hfs_fe()
{
	hfs_select |= DO_FEU;
	hfs_select |= DO_FEL;
	return (1);
}

LOCAL int
hfs_sgi()
{
	hfs_select |= DO_SGI;
	return (1);
}

LOCAL int
hfs_mbin()
{
	hfs_select |= DO_MBIN;
	return (1);
}

LOCAL int
hfs_sgl()
{
	hfs_select |= DO_SGL;
	return (1);
}

LOCAL int
hfs_dave()
{
	hfs_select |= DO_DAVE;
	return (1);
}


LOCAL int
hfs_sfm()
{
	hfs_select |= DO_SFM;
	return (1);
}

LOCAL int
hfs_xdbl()
{
	hfs_select |= DO_XDBL;
	return (1);
}

LOCAL int
hfs_xhfs()
{
#ifdef	IS_MACOS_X
	hfs_select |= DO_XHFS;
#else
	errmsgno(EX_BAD,
	_("Warning: --osx-hfs only works on MacOS X ... ignoring\n"));
#endif
	return (1);
}

LOCAL int
hfs_nohfs()
{
	no_apple_hyb = 1;
	return (1);
}

#endif	/* APPLE_HYB */

LOCAL void
ldate_error(arg)
	char	*arg;
{
	comerrno(EX_BAD, _("Ilegal date specification '%s'.\n"), arg);
}

LOCAL char *
strntoi(p, n, ip)
	char	*p;
	int	n;
	int	*ip;
{
	int	i = 0;
	int	digits = 0;
	int	c;

	while (*p) {
		if (digits >= n)
			break;
		c = *p;
		if (c < '0' || c > '9')
			break;
		p++;
		digits++;
		i *= 10;
		i += c - '0';
	}
	*ip = i;

	return (p);
}

#define	dysize(A) (((A)%4)? 365 : (((A)%100) == 0 && ((A)%400)) ? 365 : 366)

static int dmsize[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

LOCAL int
mosize(y, m)
	int	y;
	int	m;
{

	if (m == 1 && dysize(y) == 366)
		return (29);
	return (dmsize[m]);
}

LOCAL char *
parse_date(arg, tp)
	char	*arg;
	struct tm *tp;
{
	char	*oarg = arg;
	char	*p;

	tp->tm_mon = tp->tm_hour = tp->tm_min = tp->tm_sec = 0;
	tp->tm_mday = 1;
	tp->tm_isdst = -1;

	p = strchr(arg, '/');
	if (p == NULL)
		p = strchr(arg, '-');
	if (p) {
		if ((p - arg) != 2 && (p - arg) != 4)
			ldate_error(oarg);
		p = strntoi(arg, 4, &tp->tm_year);
		if ((p - arg) != 2 && (p - arg) != 4)
			ldate_error(oarg);
		if ((p - arg) == 2) {
			if (tp->tm_year < 69)
				tp->tm_year += 100;
		} else {
			tp->tm_year -= 1900;
		}
		if (*p == '/' || *p == '-')
			p++;
	} else if (strlen(arg) < 4) {
		ldate_error(oarg);
	} else {
		p = strntoi(arg, 4, &tp->tm_year);
		if ((p - arg) != 4)
			ldate_error(oarg);
		tp->tm_year -= 1900;
	}
	if (*p == '\0' || strchr(".+-", *p))
		return (p);
	arg = p;
	p = strntoi(arg, 2, &tp->tm_mon);
	if ((p - arg) != 2)
		ldate_error(oarg);
	tp->tm_mon -= 1;
	if (tp->tm_mon < 0 || tp->tm_mon >= 12)
		ldate_error(oarg);
	if (*p == '/' || *p == '-')
		p++;
	if (*p == '\0' || strchr(".+-", *p))
		return (p);
	arg = p;
	p = strntoi(arg, 2, &tp->tm_mday);
	if ((p - arg) != 2)
		ldate_error(oarg);
	if (tp->tm_mday < 1 || tp->tm_mday > mosize(tp->tm_year+1900, tp->tm_mon))
		ldate_error(oarg);
	if (*p == ' ')
		p++;
	if (*p == '\0' || strchr(".+-", *p))
		return (p);
	arg = p;
	p = strntoi(arg, 2, &tp->tm_hour);
	if ((p - arg) != 2)
		ldate_error(oarg);
	if (tp->tm_hour > 23)
		ldate_error(oarg);
	if (*p == ':')
		p++;
	if (*p == '\0' || strchr(".+-", *p))
		return (p);
	arg = p;
	p = strntoi(arg, 2, &tp->tm_min);
	if ((p - arg) != 2)
		ldate_error(oarg);
	if (tp->tm_min > 59)
		ldate_error(oarg);
	if (*p == ':')
		p++;
	if (*p == '\0' || strchr(".+-", *p))
		return (p);
	arg = p;
	p = strntoi(arg, 2, &tp->tm_sec);
	if ((p - arg) != 2)
		ldate_error(oarg);
	if (tp->tm_sec > 61)
		ldate_error(oarg);
	return (p);
}

/*
 * Parse a date spec similar to:
 * YYYY[MM[DD[HH[MM[SS]]]]][.hh][+-GHGM]
 */
LOCAL int
get_ldate(opt_arg, valp)
	char	*opt_arg;
	void	*valp;
{
	time_t	t;
	int	usec = 0;
	int	gmtoff = -100;
	struct tm tm;
	char	*p;
	char	*arg;

	p = parse_date(opt_arg, &tm);
	if (*p == '.') {
		p++;
		arg = p;
		p = strntoi(arg, 2, &usec);
		if ((p - arg) != 2)
			ldate_error(opt_arg);
		if (usec > 99)
			ldate_error(opt_arg);
		usec *= 10000;
	}
	if (*p == ' ')
		p++;
	if (*p == '+' || *p == '-') {
		int	i;
		char	*s = p++;

		arg = p;
		p = strntoi(arg, 2, &gmtoff);
		if ((p - arg) != 2)
			ldate_error(opt_arg);
		arg = p;
		p = strntoi(arg, 2, &i);
		if ((p - arg) != 2)
			ldate_error(opt_arg);
		if (i > 59)
			ldate_error(opt_arg);
		gmtoff *= 60;
		gmtoff += i;
		if (gmtoff % 15)
			ldate_error(opt_arg);
		if (*s == '-')
			gmtoff *= -1;
		gmtoff /= 15;
		if (gmtoff < -48 || gmtoff > 52)
			ldate_error(opt_arg);
	}
	if (*p != '\0')
		ldate_error(opt_arg);

	seterrno(0);
	t = mktime(&tm);
	if (t == -1 && geterrno() != 0)
		comerr(_("Date '%s' is out of range.\n"), opt_arg);

	((ldate *)valp)->l_sec = t;
	((ldate *)valp)->l_usec = usec;
	((ldate *)valp)->l_gmtoff = gmtoff;

	return (1);
}

#ifdef	USE_FIND
/* ARGSUSED */
LOCAL int
getfind(arg, valp, pac, pav)
	char	*arg;
	long	*valp;	/* Not used until we introduce a ptr to opt struct */
	int	*pac;
	char	*const	**pav;
{
	dofind = TRUE;
	find_ac = *pac;
	find_av = *pav;
	find_ac--, find_av++;
	return (NOARGS);
}
#endif

/* ARGSUSED */
LOCAL int
getH(arg, valp, pac, pav, opt)	/* Follow symlinks encountered on cmdline */
	const char	*arg;
	void		*valp;
	int		*pac;
	char	*const	**pav;
	const char	*opt;
{
/*error("getH\n");*/
	if (opt[0] == '-' && opt[1] == 'H' && opt[2] == '\0') {
#ifdef	APPLE_HYB
		if (legacy) {
			errmsgno(EX_BAD, _("The option '-H' is deprecated since 2002.\n"));
			errmsgno(EX_BAD, _("The option '-H' was disabled in 2006.\n"));
			errmsgno(EX_BAD, _("Use '-map' instead of '-H' as documented instead of using the legacy mode.\n"));
			afpfile = (char *)arg;
			return (1);
		}
#endif
		return (BADFLAG);	/* POSIX -H is not yet active */
	}
	follow_links = FALSE;
	Hflag = TRUE;
#ifdef	USE_FIND
	*(int *)valp |= WALK_ARGFOLLOW;
#endif
	return (1);
}

/* ARGSUSED */
LOCAL int
getL(arg, valp, pac, pav, opt)	/* Follow all symlinks */
	const char	*arg;
	void		*valp;
	int		*pac;
	char	*const	**pav;
	const char	*opt;
{
/*error("getL\n");*/
	if (opt[0] == '-' && opt[1] == 'L' && opt[2] == '\0') {
		if (legacy) {
			errmsgno(EX_BAD, _("The option '-L' is deprecated since 2002.\n"));
			errmsgno(EX_BAD, _("The option '-L' was disabled in 2006.\n"));
			errmsgno(EX_BAD,
				_("Use '-allow-leading-dots' instead of '-L' as documented instead of using the legacy mode.\n"));
			allow_leading_dots = TRUE;
			return (1);
		}
		return (BADFLAG);	/* POSIX -L is not yet active */
	}
	follow_links = TRUE;
	Hflag = FALSE;
#ifdef	USE_FIND
	*(int *)valp |= WALK_ALLFOLLOW;
#endif
	return (1);
}

/* ARGSUSED */
LOCAL int
getP(arg, valp, pac, pav, opt)	/* Do not follow symlinks */
	const char	*arg;
	void		*valp;
	int		*pac;
	char	*const	**pav;
	const char	*opt;
{
/*error("getP\n");*/
	if (opt[0] == '-' && opt[1] == 'P' && opt[2] == '\0') {
		if (legacy) {
			errmsgno(EX_BAD, _("The option '-P' is deprecated since 2002.\n"));
			errmsgno(EX_BAD, _("The option '-P' was disabled in 2006.\n"));
			errmsgno(EX_BAD, _("Use '-publisher' instead of '-P' as documented instead of using the legacy mode.\n"));
			publisher = (char *)arg;
			return (1);
		}
		return (BADFLAG);	/* POSIX -P is not yet active */
	}
	follow_links = FALSE;
	Hflag = FALSE;
#ifdef	USE_FIND
	*(int *)valp &= ~(WALK_ARGFOLLOW | WALK_ALLFOLLOW);
#endif
	return (1);
}

LOCAL struct ga_flags *gl_flags;
/* ARGSUSED */
LOCAL int
dolegacy(arg, valp, pac, pav, opt)	/* Follow symlinks encountered on cmdline */
	const char	*arg;
	void		*valp;
	int		*pac;
	char	*const	**pav;
	const char	*opt;
{
	legacy = TRUE;
#ifdef	APPLE_HYB
	gl_flags[2].ga_format = "H&";	/* Apple Hybrid "-map" */
#endif
#ifdef	OPT_L_HAS_ARG			/* never ;-) */
	gl_flags[4].ga_format = "L&";	/* -allow-leading-dots */
#endif
	gl_flags[6].ga_format = "P&";	/* -publisher */
	return (1);
}

struct mki_option {
	/*
	 * The long option information.
	 */
	struct ga_flags	opt;
	/*
	 * The documentation string.  If this is NULL or empty, this is a
	 * synonym for the previous option.
	 *
	 * If the string starts with a '-', the (long) option is a
	 * double dash option.
	 *
	 * If the next character in the string is a ^A (\1), then the following
	 * characters are the argument name for the option. The arg string ends
	 * before a \1 or a \2 is seen. A \1 is skipped.
	 *
	 * The rest of the string is the real documentation string. If it
	 * starts with \2, then the option is hidden from the online help.
	 */
	const char	*doc;
};


LOCAL int	save_pname = 0;

LOCAL const struct mki_option mki_options[] =
{
#ifdef	USE_FIND
	{{"find~", NULL, (getpargfun)getfind },
	__("\1file... [find expr.]\1Option separator: Use find command line to the right")},
#endif
	/*
	 * The options -H/-L/-P did have different meaning in old releases of
	 * mkisofs. In October 2002, mkisofs introduced a warning that the
	 * short options -H/-L/-P should not be used with the old meaning
	 * anymore. The long options should be used instead. The options
	 * -H/-L/-P previously have been associated with the following long
	 * options:
	 *
	 *	-H	-map				2000..2002
	 *	-L	-allow-leading-dots		1995..2002
	 *	-P	-publisher			1993..2002
	 *
	 * Since December 2006, the short options -H/-L/-P have been disabled
	 * for their old meaning.
	 *
	 * Warning: for the -legacy mode, the entries for -H/-L/-P need to stay
	 * at their current index in mki_options[] or dolegacy() needs to be
	 * changed to fit the modification.
	 */
	{{"posix-H~", &walkflags, getH },
	__("Follow symbolic links encountered on command line")},
	{{"H~", &walkflags, getH },
#ifdef	LATER
	NULL},
#else
	__("\2")},
#endif
	{{"posix-L~", &walkflags, getL },
	__("Follow all symbolic links")},
	{{"L~", &walkflags, getL },
#ifdef	LATER
	NULL},
#else
	__("\2")},
#endif
	{{"posix-P~", &walkflags, getP },
	__("Do not follow symbolic links (default)")},
	{{"P~", &walkflags, getP },
#ifdef	LATER
	NULL},
#else
	__("\2")},
#endif

	{{"abstract*", &abstract },
	__("\1FILE\1Set Abstract filename")},
	{{"A*,appid*", &appid },
	__("\1ID\1Set Application ID")},
	{{"biblio*", &biblio },
	__("\1FILE\1Set Bibliographic filename")},
	/*
	 * If this includes UWIN, we need to modify the condition.
	 */
#if !defined(__MINGW32__) && !defined(_MSC_VER)
	{{"cache-inodes", &cache_inodes },
	__("Cache inodes (needed to detect hard links)")},
#endif
	{{"no-cache-inodes%0", &cache_inodes },
	__("Do not cache inodes (if filesystem has no unique inodes)")},
	{{"rrip110%0", &rrip112 },
	__("Create old Rock Ridge V 1.10")},
	{{"rrip112", &rrip112 },
	__("Create new Rock Ridge V 1.12 (default)")},
#ifdef	DUPLICATES_ONCE
	{{"duplicates-once", &duplicates_once},
	__("Optimize storage by encoding duplicate files once")},
#endif
	{{"check-oldnames", &check_oldnames },
	__("Check all imported ISO9660 names from old session")},
	{{"check-session*", &check_image },
	__("\1FILE\1Check all ISO9660 names from previous session")},
	{{"copyright*", &copyright },
	__("\1FILE\1Set Copyright filename")},
	{{"debug+", &debug },
	__("Set debug flag")},
	{{"ignore-error", &ignerr },
	__("Ignore errors")},
	{{"b& ,eltorito-boot&", NULL, (getpargfun)get_boot_image },
	__("\1FILE\1Set El Torito boot image name")},
	{{"eltorito-alt-boot~", NULL, (getpargfun)new_boot_entry },
	__("Start specifying alternative El Torito boot parameters")},
	{{"eltorito-platform&", NULL, (getpargfun)get_boot_platid },
	__("\1ID\1Set El Torito platform id for the next boot entry")},
	{{"B&,sparc-boot&", NULL, (getpargfun)scan_sparc_boot },
	__("\1FILES\1Set sparc boot image names")},
	{{"sunx86-boot&", NULL, (getpargfun)scan_sunx86_boot },
	__("\1FILES\1Set sunx86 boot image names")},
	{{"G*,generic-boot*", &genboot_image },
	__("\1FILE\1Set generic boot image name")},
	{{"sparc-label&", NULL, (getpargfun)sparc_boot_label },
	__("\1label text\1Set sparc boot disk label")},
	{{"sunx86-label&", NULL, (getpargfun)sunx86_boot_label },
	__("\1label text\1Set sunx86 boot disk label")},
	{{"c* ,eltorito-catalog*", &boot_catalog },
	__("\1FILE\1Set El Torito boot catalog name")},
	{{"C*,cdrecord-params*", &cdrecord_data },
	__("\1PARAMS\1Magic paramters from cdrecord")},
	{{"d,omit-period", &omit_period },
	__("Omit trailing periods from filenames (violates ISO9660)")},
	{{"data-change-warn", &data_change_warn },
	__("Treat data/size changes as warning only")},
	{{"dir-mode*", &dirmode_str },
	__("\1mode\1Make the mode of all directories this mode.")},
	{{"D,disable-deep-relocation", &disable_deep_reloc },
	__("Disable deep directory relocation (violates ISO9660)")},
	{{"file-mode*", &filemode_str },
	__("\1mode\1Make the mode of all plain files this mode.")},
	{{"errctl&", NULL, (getpargfun)errconfig },
	__("\1name\1Read error control defs from file or inline.")},
	{{"f,follow-links", &follow_links },
	__("Follow symbolic links")},
	{{"gid*", &gid_str },
	__("\1gid\1Make the group owner of all files this gid.")},
	{{"graft-points", &use_graft_ptrs },
	__("Allow to use graft points for filenames")},
	{{"root*", &reloc_root },
	__("\1DIR\1Set root directory for all new files and directories")},
	{{"old-root*", &reloc_old_root },
	__("\1DIR\1Set root directory in previous session that is searched for files")},
	{{"help", &help },
	__("Print option help")},
	{{"hide& ", NULL, (getpargfun)i_add_match },
	__("\1GLOBFILE\1Hide ISO9660/RR file")},
	{{"hide-list&", NULL, (getpargfun)i_add_list },
	__("\1FILE\1File with list of ISO9660/RR files to hide")},
	{{"hidden& ", NULL, (getpargfun)h_add_match },
	__("\1GLOBFILE\1Set hidden attribute on ISO9660 file")},
	{{"hidden-list&", NULL, (getpargfun)h_add_list },
	__("\1FILE\1File with list of ISO9660 files with hidden attribute")},
	{{"hide-joliet& ", NULL, (getpargfun)j_add_match },
	__("\1GLOBFILE\1Hide Joliet file")},
	{{"hide-joliet-list&", NULL, (getpargfun)j_add_list },
	__("\1FILE\1File with list of Joliet files to hide")},
#ifdef UDF
	{{"hide-udf& ", NULL, (getpargfun)u_add_match },
	__("\1GLOBFILE\1Hide UDF file")},
	{{"hide-udf-list&", NULL, (getpargfun)u_add_list },
	__("\1FILE\1File with list of UDF files to hide")},
#endif
	{{"hide-joliet-trans-tbl", &jhide_trans_tbl},
	__("Hide TRANS.TBL from Joliet tree")},
	{{"hide-rr-moved", &hide_rr_moved },
	__("Rename RR_MOVED to .rr_moved in Rock Ridge tree")},
	{{"gui", &gui},
	__("Switch behaviour for GUI")},
	{{"input-charset*", &icharset },
	__("\1CHARSET\1Local input charset for file name conversion")},
	{{"output-charset*", &ocharset },
	__("\1CHARSET\1Output charset for file name conversion")},
	{{"iso-level#", &iso9660_level },
	__("\1LEVEL\1Set ISO9660 conformance level (1..3) or 4 for ISO9660 version 2")},
	{{"J,joliet", &use_Joliet },
	__("Generate Joliet directory information")},
	{{"joliet-long", &joliet_long },
	__("Allow Joliet file names to be 103 Unicode characters")},
	{{"jcharset*", &jcharset },
	__("\1CHARSET\1Local charset for Joliet directory information")},
	{{"l,full-iso9660-filenames", &full_iso9660_filenames },
	__("Allow full 31 character filenames for ISO9660 names")},
	{{"max-iso9660-filenames", &max_filenames },
	__("Allow 37 character filenames for ISO9660 names (violates ISO9660)")},

	{{"allow-leading-dots", &allow_leading_dots },
	__("Allow ISO9660 filenames to start with '.' (violates ISO9660)")},
	{{"ldots", &allow_leading_dots },
	__("Allow ISO9660 filenames to start with '.' (violates ISO9660)")},

	{{"log-file*", &log_file },
	__("\1LOG_FILE\1Re-direct messages to LOG_FILE")},
	{{"long-rr-time%1", &long_rr_time },
	__("Use long Rock Ridge time format")},
	{{"m& ,exclude& ", NULL, (getpargfun)add_match },
	__("\1GLOBFILE\1Exclude file name")},
	{{"exclude-list&", NULL, (getpargfun)add_list},
	__("\1FILE\1File with list of file names to exclude")},

	{{"hide-ignorecase", &match_igncase },
	__("Ignore case with -exclude-list and -hide* options")},
	{{"exclude-ignorecase", &match_igncase },
	NULL},

	{{"modification-date&", &modification_date, (getpargfun)get_ldate },
	__("\1DATE\1Set the modification date field of the PVD")},
	{{"nobak%0", &all_files },
	__("Do not include backup files")},
	{{"no-bak%0", &all_files},
	__("Do not include backup files")},
	{{"pad", &dopad },
	__("Pad output to a multiple of 32k (default)")},
	{{"no-pad%0", &dopad },
	__("Do not pad output to a multiple of 32k")},
	{{"no-limit-pathtables", &nolimitpathtables },
	__("Allow more than 65535 parent directories (violates ISO9660)")},
	{{"no-long-rr-time%0", &long_rr_time },
	__("Use short Rock Ridge time format")},
	{{"M*,prev-session*", &merge_image },
	__("\1FILE\1Set path to previous session to merge")},
	{{"dev*", &merge_image },
	__("\1SCSIdev\1Set path to previous session to merge")},
	{{"N,omit-version-number", &omit_version_number },
	__("Omit version number from ISO9660 filename (violates ISO9660)")},
	{{"new-dir-mode*", &new_dirmode_str },
	__("\1mode\1Mode used when creating new directories.")},
	{{"force-rr", &force_rr },
	__("Inhibit automatic Rock Ridge detection for previous session")},
	{{"no-rr", &no_rr },
	__("Inhibit reading of Rock Ridge attributes from previous session")},
	{{"no-split-symlink-components%0", &split_SL_component },
	__("Inhibit splitting symlink components")},
	{{"no-split-symlink-fields%0", &split_SL_field },
	__("Inhibit splitting symlink fields")},
	{{"o* ,output* ", &outfile },
	__("\1FILE\1Set output file name")},
	{{"path-list*", &pathnames },
	__("\1FILE\1File with list of pathnames to process")},
	{{"p* ,preparer*", &preparer },
	__("\1PREP\1Set Volume preparer")},
	{{"print-size", &print_size },
	__("Print estimated filesystem size and exit")},
	{{"publisher*", &publisher },
	__("\1PUB\1Set Volume publisher")},
	{{"quiet%0", &verbose },
	__("Run quietly")},
	{{"r,rational-rock", &rationalize_rr },
	__("Generate rationalized Rock Ridge directory information")},
	{{"R,rock", &use_RockRidge },
	__("Generate Rock Ridge directory information")},
	{{"s* ,sectype*", &sectype },
	__("\1TYPE\1Set output sector type to e.g. data/xa1/raw")},
	{{"short-rr-time%0", &long_rr_time },
	__("Use short Rock Ridge time format")},

#ifdef SORTING
	{ {"sort&", NULL, add_sort_list },
	__("\1FILE\1Sort file content locations according to rules in FILE")},
	{ {"isort&", NULL, add_sort_list },
	__("\1FILE\1Sort file content locations according to rules in FILE (ignore case)")},
#endif /* SORTING */

	{{"split-output", &split_output },
	__("Split output into files of approx. 1GB size")},
	{{"stream-file-name*", &stream_filename },
	__("\1FILE_NAME\1Set the stream file ISO9660 name (incl. version)")},
	{{"stream-media-size#", &stream_media_size },
	__("\1#\1Set the size of your CD media in sectors")},
	{{"sysid*", &system_id },
	__("\1ID\1Set System ID")},
	{{"T,translation-table", &generate_tables },
	__("Generate translation tables for systems that don't understand long filenames")},
	{{"table-name*", &trans_tbl },
	__("\1TABLE_NAME\1Translation table file name")},
	{{"ucs-level#", &ucs_level },
	__("\1LEVEL\1Set Joliet UCS level (1..3)")},

#ifdef UDF
	{{"udf", &rationalize_udf },
	__("Generate rationalized UDF file system")},
	{{"UDF", &use_udf },
	__("Generate UDF file system")},
	{{"udf-symlinks", &create_udfsymlinks },
	__("Create symbolic links on UDF image (default)")},
	{{"no-udf-symlinks%0", &create_udfsymlinks },
	__("Do not reate symbolic links on UDF image")},
#endif

#ifdef DVD_AUD_VID
	{{"dvd-audio", &dvd_audio },
	"Generate DVD-Audio compliant UDF file system"},
	{{"dvd-hybrid", &dvd_hybrid },
	"Generate a hybrid (DVD-Audio and DVD-Video) compliant UDF file system"},
	{{"dvd-video", &dvd_video },
	__("Generate DVD-Video compliant UDF file system")},
#endif

	{{"uid*", &uid_str },
	__("\1uid\1Make the owner of all files this uid.")},
	{{"U,untranslated-filenames", &untranslated_filenames },
	/* CSTYLED */
	__("Allow Untranslated filenames (for HPUX & AIX - violates ISO9660). Forces -l, -d, -N, -allow-leading-dots, -relaxed-filenames, -allow-lowercase, -allow-multidot")},
	{{"relaxed-filenames", &relaxed_filenames },
	__("Allow 7 bit ASCII except lower case characters (violates ISO9660)")},
	{{"no-iso-translate%0", &iso_translate },
	__("Do not translate illegal ISO characters '~', '-' and '#' (violates ISO9660)")},
	{{"allow-lowercase", &allow_lowercase },
	__("Allow lower case characters in addition to the current character set (violates ISO9660)")},
	{{"no-allow-lowercase", &no_allow_lowercase },
	__("Do not allow lower case characters in addition to the current character set.")},
	{{"+allow-lowercase", &no_allow_lowercase },
	NULL},
	{{"allow-multidot", &allow_multidot },
	__("Allow more than one dot in filenames (e.g. .tar.gz) (violates ISO9660)")},
	{{"use-fileversion", &use_fileversion },
	__("\1LEVEL\1Use file version # from filesystem")},
	{{"v+,verbose+", &verbose },
	__("Verbose")},
	{{"version", &pversion },
	__("Print the current version")},
	{{"V*,volid*", &volume_id },
	__("\1ID\1Set Volume ID")},
	{{"volset* ", &volset_id },
	__("\1ID\1Set Volume set ID")},
	{{"volset-size#", &volume_set_size },
	__("\1#\1Set Volume set size")},
	{{"volset-seqno#", &volume_sequence_number },
	__("\1#\1Set Volume set sequence number")},
	{{"x& ,old-exclude&", NULL, (getpargfun)add_match },
	__("\1FILE\1Exclude file name(depreciated)")},
	{{"hard-disk-boot~", NULL, (getpargfun)get_hd_boot },
	__("Boot image is a hard disk image")},
	{{"no-emul-boot~", NULL, (getpargfun)get_ne_boot },
	__("Boot image is 'no emulation' image")},
	{{"no-boot~", NULL, (getpargfun)get_no_boot },
	__("Boot image is not bootable")},
	{{"boot-load-seg&", NULL, (getpargfun)get_boot_addr },
	__("\1#\1Set load segment for boot image")},
	{{"boot-load-size&", NULL, (getpargfun)get_boot_size },
	__("\1#\1Set numbers of load sectors")},
	{{"boot-info-table~", NULL, (getpargfun)get_boot_table },
	__("Patch boot image with info table")},
	{{"XA", &use_XA },
	__("Generate XA directory attributes")},
	{{"xa", &rationalize_xa },
	__("Generate rationalized XA directory attributes")},
	{{"z,transparent-compression", &transparent_compression },
	__("Enable transparent compression of files")},

#ifdef APPLE_HYB
	{{"hfs-type*", &deftype },
	__("\1TYPE\1Set HFS default TYPE")},
	{{"hfs-creator", &defcreator },
	__("\1CREATOR\1Set HFS default CREATOR")},
	{{"g,apple", &apple_ext },
	__("Add Apple ISO9660 extensions")},
	{{"h,hfs", &apple_hyb },
	__("Create ISO9660/HFS hybrid")},
	{{"map*", &afpfile },
	__("\1MAPPING_FILE\1Map file extensions to HFS TYPE/CREATOR")},
	{{"magic*", &magic_file },
	__("\1FILE\1Magic file for HFS TYPE/CREATOR")},
	{{"probe", &probe },
	__("Probe all files for Apple/Unix file types")},
	{{"mac-name", &use_mac_name },
	__("Use Macintosh name for ISO9660/Joliet/RockRidge file name")},
	{{"no-mac-files", &nomacfiles },
	__("Do not look for Unix/Mac files (depreciated)")},
	{{"boot-hfs-file*", &hfs_boot_file },
	__("\1FILE\1Set HFS boot image name")},
	{{"part", &gen_pt },
	__("Generate HFS partition table")},
	{{"cluster-size&", NULL, (getpargfun)get_bsize },
	__("\1SIZE\1Cluster size for PC Exchange Macintosh files")},
	{{"auto*", &autoname },
	__("\1FILE\1Set HFS AutoStart file name")},
	{{"no-desktop%0", &create_dt },
	__("Do not create the HFS (empty) Desktop files")},
	{{"hide-hfs& ", NULL, (getpargfun)hfs_add_match },
	__("\1GLOBFILE\1Hide HFS file")},
	{{"hide-hfs-list&", NULL, (getpargfun)hfs_add_list },
	__("\1FILE\1List of HFS files to hide")},
	{{"hfs-volid*", &hfs_volume_id },
	__("\1HFS_VOLID\1Volume name for the HFS partition")},
	{{"icon-position", &icon_pos },
	__("Keep HFS icon position")},
	{{"root-info*", &root_info },
	__("\1FILE\1finderinfo for root folder")},
	{{"input-hfs-charset*", &hfs_icharset },
	__("\1CHARSET\1Local input charset for HFS file name conversion")},
	{{"output-hfs-charset*", &hfs_ocharset },
	__("\1CHARSET\1Output charset for HFS file name conversion")},
	{{"hfs-unlock%0", &hfs_lock },
	__("Leave HFS Volume unlocked")},
	{{"hfs-bless*", &hfs_bless },
	__("\1FOLDER_NAME\1Name of Folder to be blessed")},
	{{"hfs-parms*", &hfs_parms },
	__("\1PARAMETERS\1Comma separated list of HFS parameters")},
#ifdef PREP_BOOT
	{{"prep-boot&", NULL, (getpargfun)get_prep_boot },
	__("\1FILE\1PReP boot image file -- up to 4 are allowed")},
	{{"chrp-boot&", NULL, (getpargfun)get_chrp_boot },
	__("Add CHRP boot header")},
#endif	/* PREP_BOOT */
	{{"cap~", NULL, (getpargfun)hfs_cap },
	__("-Look for AUFS CAP Macintosh files")},
	{{"netatalk~", NULL, (getpargfun)hfs_neta},
	__("-Look for NETATALK Macintosh files")},
	{{"double~", NULL, (getpargfun)hfs_dbl },
	__("-Look for AppleDouble Macintosh files")},
	{{"ethershare~", NULL, (getpargfun)hfs_esh },
	__("-Look for Helios EtherShare Macintosh files")},
	{{"exchange~", NULL, (getpargfun)hfs_fe },
	__("-Look for PC Exchange Macintosh files")},
	{{"sgi~", NULL, (getpargfun)hfs_sgi },
	__("-Look for SGI Macintosh files")},
	{{"macbin~", NULL, (getpargfun)hfs_mbin },
	__("-Look for MacBinary Macintosh files")},
	{{"single~", NULL, (getpargfun)hfs_sgl },
	__("-Look for AppleSingle Macintosh files")},
	{{"ushare~", NULL, (getpargfun)hfs_esh },
	__("-Look for IPT UShare Macintosh files")},
	{{"xinet~", NULL, (getpargfun)hfs_sgi },
	__("-Look for XINET Macintosh files")},
	{{"dave~", NULL, (getpargfun)hfs_dave },
	__("-Look for DAVE Macintosh files")},
	{{"sfm~", NULL, (getpargfun)hfs_sfm },
	__("-Look for SFM Macintosh files")},
	{{"osx-double~", NULL, (getpargfun)hfs_xdbl },
	__("-Look for MacOS X AppleDouble Macintosh files")},
	{{"osx-hfs~", NULL, (getpargfun)hfs_xhfs },
	__("-Look for MacOS X HFS Macintosh files")},
	{{"no-hfs~", NULL, (getpargfun)hfs_nohfs },
	__("Do not create ISO9660/HFS hybrid")},
#endif	/* APPLE_HYB */
	{{"legacy~", NULL, dolegacy },
	__("\2")},
};

#define	OPTION_COUNT (sizeof mki_options / sizeof (mki_options[0]))

LOCAL	void	read_rcfile	__PR((char *appname));
LOCAL	void	susage		__PR((int excode));
LOCAL	void	usage		__PR((int excode));
EXPORT	int	iso9660_date	__PR((char *result, time_t crtime));
LOCAL	void	hide_reloc_dir	__PR((void));
LOCAL	char	*get_pnames	__PR((int argc, char *const *argv, int opt,
					char *pname, int pnsize, FILE *fp));
EXPORT	int	main		__PR((int argc, char *argv[]));
LOCAL	void	list_locales	__PR((void));
EXPORT	char	*findgequal	__PR((char *s));
LOCAL	char	*escstrcpy	__PR((char *to, size_t tolen, char *from));
struct directory *get_graft	__PR((char *arg, char *graft_point, size_t glen,
						char *nodename, size_t nlen,
						char **short_namep, BOOL do_insert));
EXPORT	void	*e_malloc	__PR((size_t size));
EXPORT	char	*e_strdup	__PR((const char *s));
LOCAL	void	ovstrcpy	__PR((char *p2, char *p1));
LOCAL	void	checkarch	__PR((char *name));

LOCAL void
read_rcfile(appname)
	char		*appname;
{
	FILE		*rcfile = (FILE *)NULL;
	struct rcopts	*rco;
	char		*pnt,
			*pnt1;
	char		linebuffer[256];
	static char	rcfn[] = ".mkisofsrc";
	char		filename[1000];
	int		linum;

	strlcpy(filename, rcfn, sizeof (filename));
	if (access(filename, R_OK) == 0)
		rcfile = fopen(filename, "r");
	if (!rcfile && errno != ENOENT)
		errmsg(_("Cannot open '%s'.\n"), filename);

	if (!rcfile) {
		pnt = getenv("MKISOFSRC");
		if (pnt && strlen(pnt) <= sizeof (filename)) {
			strlcpy(filename, pnt, sizeof (filename));
			if (access(filename, R_OK) == 0)
				rcfile = fopen(filename, "r");
			if (!rcfile && errno != ENOENT)
				errmsg(_("Cannot open '%s'.\n"), filename);
		}
	}
	if (!rcfile) {
		pnt = getenv("HOME");
		if (pnt && strlen(pnt) + strlen(rcfn) + 2 <=
							sizeof (filename)) {
			strlcpy(filename, pnt, sizeof (filename));
			if (strlen(rcfn) + 2 <=
			    (sizeof (filename) - strlen(filename))) {
				strcat(filename, "/");
				strcat(filename, rcfn);
			}
			if (access(filename, R_OK) == 0)
				rcfile = fopen(filename, "r");
			if (!rcfile && errno != ENOENT)
				errmsg(_("Cannot open '%s'.\n"), filename);
		}
	}
	if (!rcfile && strlen(appname) + sizeof (rcfn) + 2 <=
							sizeof (filename)) {
		strlcpy(filename, appname, sizeof (filename));
		pnt = strrchr(filename, '/');
		if (pnt) {
			strlcpy(pnt + 1, rcfn,
				sizeof (filename) - (pnt + 1 - filename));
			if (access(filename, R_OK) == 0)
				rcfile = fopen(filename, "r");
			if (!rcfile && errno != ENOENT)
				errmsg(_("Cannot open '%s'.\n"), filename);
		}
	}
	if (!rcfile)
		return;
	if (verbose > 0) {
		fprintf(stderr, _("Using \"%s\"\n"), filename);
	}
	/* OK, we got it.  Now read in the lines and parse them */
	linum = 0;
	while (fgets(linebuffer, sizeof (linebuffer), rcfile)) {
		char	*name;
		char	*name_end;

		++linum;
		/* skip any leading white space */
		pnt = linebuffer;
		while (*pnt == ' ' || *pnt == '\t')
			++pnt;
		/*
		 * If we are looking at a # character, this line is a comment.
		 */
		if (*pnt == '#')
			continue;
		/*
		 * The name should begin in the left margin.  Make sure it is
		 * in upper case.  Stop when we see white space or a comment.
		 */
		name = pnt;
		while (*pnt && (isalpha((unsigned char) *pnt) || *pnt == '_')) {
			if (islower((unsigned char) *pnt))
				*pnt = toupper((unsigned char) *pnt);
			pnt++;
		}
		if (name == pnt) {
			fprintf(stderr, _("%s:%d: name required\n"), filename,
					linum);
			continue;
		}
		name_end = pnt;
		/* Skip past white space after the name */
		while (*pnt == ' ' || *pnt == '\t')
			pnt++;
		/* silently ignore errors in the rc file. */
		if (*pnt != '=') {
			fprintf(stderr, _("%s:%d: equals sign required after '%.*s'\n"),
						filename, linum,
						/* XXX Should not be > int */
						(int)(name_end-name), name);
			continue;
		}
		/* Skip pas the = sign, and any white space following it */
		pnt++;	/* Skip past '=' sign */
		while (*pnt == ' ' || *pnt == '\t')
			pnt++;

		/* now it is safe to NUL terminate the name */

		*name_end = 0;

		/* Now get rid of trailing newline */

		pnt1 = pnt;
		while (*pnt1) {
			if (*pnt1 == '\n') {
				*pnt1 = 0;
				break;
			}
			pnt1++;
		}
		/* OK, now figure out which option we have */
		for (rco = rcopt; rco->tag; rco++) {
			if (strcmp(rco->tag, name) == 0) {
				*rco->variable = e_strdup(pnt);
				break;
			}
		}
		if (rco->tag == NULL) {
			fprintf(stderr, _("%s:%d: field name \"%s\" unknown\n"),
				filename, linum,
				name);
		}
	}
	if (ferror(rcfile))
		errmsg(_("Read error on '%s'.\n"), filename);
	fclose(rcfile);
}

char	*path_table_l = NULL;
char	*path_table_m = NULL;

char	*jpath_table_l = NULL;
char	*jpath_table_m = NULL;

int	goof = 0;

#ifndef TRUE
#define	TRUE 1
#endif

#ifndef FALSE
#define	FALSE 0
#endif

LOCAL void
susage(excode)
	int		excode;
{
	const char	*program_name = "mkisofs";

#ifdef	USE_FIND
	fprintf(stderr, _("Usage: %s [options] [-find] file... [find expression]\n"), program_name);
#else
	fprintf(stderr, _("Usage: %s [options] file...\n"), program_name);
#endif
	fprintf(stderr, _("\nUse %s -help\n"), program_name);
	fprintf(stderr, _("to get a list all of valid options.\n"));
#ifdef	USE_FIND
	fprintf(stderr, _("\nUse %s -find -help\n"), program_name);
	fprintf(stderr, _("to get a list of all valid -find options.\n"));
#endif
	error(_("\nMost important Options:\n"));
	error(_("	-posix-H		Follow sylinks encountered on command line\n"));
	error(_("	-posix-L		Follow all symlinks\n"));
	error(_("	-posix-P		Do not follow symlinks (default)\n"));
	error(_("	-o FILE, -output FILE	Set output file name\n"));
	error(_("	-R, -rock		Generate Rock Ridge directory information\n"));
	error(_("	-r, -rational-rock	Generate rationalized Rock Ridge directory info\n"));
	error(_("	-J, -joliet		Generate Joliet directory information\n"));
	error(_("	-print-size		Print estimated filesystem size and exit\n"));
#ifdef	UDF
	error(_("	-UDF			Generate UDF file system\n"));
#endif
#ifdef	DVD_AUD_VID
	error(_("	-dvd-audio		Generate DVD-Audio compliant UDF file system\n"));
	error(_("	-dvd-video		Generate DVD-Video compliant UDF file system\n"));
	error(_("	-dvd-hybrid		Generate a hybrid (DVD-Audio/DVD-Video) compliant UDF file system\n"));
#endif
	error(_("	-iso-level LEVEL	Set ISO9660 level (1..3) or 4 for ISO9660 v 2\n"));
	error(_("	-V ID, -volid ID	Set Volume ID\n"));
	error(_("	-graft-points		Allow to use graft points for filenames\n"));
	error(_("	-M FILE, -prev-session FILE	Set path to previous session to merge\n"));

	exit(excode);
}

const char *optend	__PR((const char *fmt));
const char *
optend(fmt)
	const char	*fmt;
{
	int		c;
	const char	*ofmt = fmt;

	for (; *fmt != '\0'; fmt++) {
		c = *fmt;
		if (c == '\\') {
			if (*++fmt == '\0')
				break;
			continue;
		}
		if (c == ',' || c == '%' || c == '*' || c == '?' ||
		    c == '#' || c == '&' || c == '~')
			break;
		if (fmt > ofmt && c == '+')
			break;

	}
	return (fmt);
}

int	printopts	__PR((FILE *f, const char *fmt, const char *arg, int twod));
int
printopts(f, fmt, arg, twod)
	FILE		*f;
	const char	*fmt;
	const char	*arg;
	int		twod;
{
	const char	*p;
	int		len = 0;
	int		optlen;
	int		arglen = 0;

	if (arg) {
		if (*arg == '-' || *arg == '\\')
			arg++;

		if (*arg == '\1') {
			p = ++arg;
			while (*p != '\0' && *p != '\1' && *p != '\2')
				p++;
			arglen = p - arg;
			if (arglen == 0)
				arg = NULL;
		} else {
			arg = NULL;
		}
	}
	for (p = optend(fmt); p > fmt; p = optend(fmt)) {
		optlen = p - fmt;
		len += fprintf(f, "%s%.*s%s%.*s",
				*fmt == '+' ? "" :
					(optlen > 1 && twod) ? "--" : "-",
				(int)(p - fmt), fmt,
				arg != NULL ? " " : "",
				arglen, arg != NULL ? arg : "");
		fmt = p;
		while (*fmt != '\0' && *fmt != ',')
			fmt++;
		if (*fmt == ',') {
			fmt++;
			len += fprintf(f, ", ");
		}
	}
	return (len);
}

const char	*docstr	__PR((const char *str, int *no_help));
const char *
docstr(str, no_help)
	const char	*str;
	int		*no_help;
{
	if (no_help)
		*no_help = 0;
	if (str == NULL)
		return (str);

	if (*str == '-' || *str == '\\')
		str++;

	if (*str == '\1') {
		str++;
		while (*str != '\0' && *str != '\1' && *str != '\2')
			str++;
	}
	if (*str == '\1') {
		str++;
	} else if (*str == '\2') {
		str++;
		if (no_help)
			*no_help = 1;
	}

	if (*str == '\0')
		return (NULL);
	return (str);
}

LOCAL void
usage(excode)
	int		excode;
{
	const char	*program_name = "mkisofs";

	int	i;

#ifdef	USE_FIND
	fprintf(stderr, _("Usage: %s [options] [-find] file... [find expression]\n"), program_name);
#else
	fprintf(stderr, _("Usage: %s [options] file...\n"), program_name);
#endif

	fprintf(stderr, _("Options:\n"));
	for (i = 0; i < (int)OPTION_COUNT; i++) {
		if (docstr(mki_options[i].doc, NULL) != NULL) {
			int	len;
			int	j;

			fprintf(stderr, "  ");
			len = 2;
			j = i;
			do {
				int		twodash;
				int		no_help;
				const char 	*doc;

				doc = mki_options[j].doc;
				twodash = (doc != NULL && *doc == '-');
				doc = docstr(doc, &no_help);

				if (!no_help) {
					/*
					 * If more options for one doc, then
					 * print a comma as separator.
					 */
					if (j > i)
						len += fprintf(stderr, ", ");
					len += printopts(stderr,
						mki_options[j].opt.ga_format,
						mki_options[j].doc,
						twodash);
				}
				++j;
			}
			while (j < (int)OPTION_COUNT &&
				docstr(mki_options[j].doc, NULL) == NULL);

			if (len >= 30) {
				fprintf(stderr, "\n");
				len = 0;
			}
			for (; len < 30; len++)
				fputc(' ', stderr);

			fprintf(stderr, "%s\n",
					docstr(mki_options[i].doc, NULL));
		}
	}
	exit(excode);
}


/*
 * Fill in date in the iso9660 format
 * This takes 7 bytes and supports year 1900 .. 2155 with 1 second granularity
 *
 * The standards  state that the timezone offset is in multiples of 15
 * minutes, and is what you add to GMT to get the localtime.  The U.S.
 * is always at a negative offset, from -5h to -8h (can vary a little
 * with DST,  I guess).  The Linux iso9660 filesystem has had the sign
 * of this wrong for ages (mkisofs had it wrong too until February 1997).
 */
EXPORT int
iso9660_date(result, crtime)
	char	*result;
	time_t	crtime;
{
	struct tm	local;
	struct tm	gmt;

	local = *localtime(&crtime);
	gmt   = *gmtime(&crtime);

	result[0] = local.tm_year;
	result[1] = local.tm_mon + 1;
	result[2] = local.tm_mday;
	result[3] = local.tm_hour;
	result[4] = local.tm_min;
	result[5] = local.tm_sec;

	local.tm_min  -= gmt.tm_min;
	local.tm_hour -= gmt.tm_hour;
	local.tm_yday -= gmt.tm_yday;
	local.tm_year -= gmt.tm_year;
	if (local.tm_year)		/* Hit new-year limit	*/
		local.tm_yday = local.tm_year;	/* yday = +-1	*/

	result[6] = (local.tm_min + 60 *
			(local.tm_hour + 24 * local.tm_yday)) / 15;

	return (0);
}

/*
 * Fill in date in the iso9660 long format
 * This takes 17 bytes and supports year 0 .. 9999 with 10ms granularity
 * If iso9660_ldate() is called with gmtoff set to -100, the gmtoffset for
 * the related time is computed via localtime(). For the modification
 * date in the PVD, the date and the GMT offset may be defined via the
 * -modification-date command line option.
 */
EXPORT int
iso9660_ldate(result, crtime, nsec, gmtoff)
	char	*result;
	time_t	crtime;
	int	nsec;
	int	gmtoff;
{
	struct tm	local;
	struct tm	gmt;

	local = *localtime(&crtime);
	gmt   = *gmtime(&crtime);

	/*
	 * There was a comment here about breaking in the year 2000.
	 * That's not true, in 2000 tm_year == 100, so 1900+tm_year == 2000.
	 */
	sprintf(result, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d%2.2d",
		1900 + local.tm_year,
		local.tm_mon + 1, local.tm_mday,
		local.tm_hour, local.tm_min, local.tm_sec,
		nsec / 10000000);

	if (gmtoff == -100) {
		local.tm_min  -= gmt.tm_min;
		local.tm_hour -= gmt.tm_hour;
		local.tm_yday -= gmt.tm_yday;
		local.tm_year -= gmt.tm_year;
		if (local.tm_year)		/* Hit new-year limit	*/
			local.tm_yday = local.tm_year;	/* yday = +-1	*/

		result[16] = (local.tm_min + 60 *
				(local.tm_hour + 24 * local.tm_yday)) / 15;
	} else {
		result[16] = gmtoff;
	}
	return (0);
}

/* hide "./rr_moved" if all its contents are hidden */
LOCAL void
hide_reloc_dir()
{
	struct directory_entry *s_entry;

	for (s_entry = reloc_dir->contents; s_entry; s_entry = s_entry->next) {
		if (strcmp(s_entry->name, ".") == 0 ||
				strcmp(s_entry->name, "..") == 0)
			continue;

		if ((s_entry->de_flags & INHIBIT_ISO9660_ENTRY) == 0)
			return;
	}

	/* all entries are hidden, so hide this directory */
	reloc_dir->dir_flags |= INHIBIT_ISO9660_ENTRY;
	reloc_dir->self->de_flags |= INHIBIT_ISO9660_ENTRY;
}

/*
 * get pathnames from the command line, and then from given file
 */
LOCAL char *
get_pnames(argc, argv, opt, pname, pnsize, fp)
	int	argc;
	char	* const *argv;
	int	opt;
	char	*pname;
	int	pnsize;
	FILE	*fp;
{
	int	len;

	/* we may of already read the first line from the pathnames file */
	if (save_pname) {
		save_pname = 0;
		return (pname);
	}

#ifdef	USE_FIND
	if (dofind && opt < (find_pav - argv))
		return (argv[opt]);
	else if (!dofind && opt < argc)
		return (argv[opt]);
#else
	if (opt < argc)
		return (argv[opt]);
#endif

	if (fp == NULL)
		return ((char *)0);

	if (fgets(pname, pnsize, fp)) {
		/* Discard newline */
		len = strlen(pname);
		if (pname[len - 1] == '\n') {
			pname[len - 1] = '\0';
		}
		return (pname);
	}
	return ((char *)0);
}

EXPORT int
main(argc, argv)
	int		argc;
	char		*argv[];
{
	int	cac = argc;
	char	* const *cav = argv;

	struct directory_entry de;

#ifdef HAVE_SBRK
	unsigned long	mem_start;
#endif
	struct stat	statbuf;
	struct iso_directory_record *mrootp = NULL;
	struct output_fragment *opnt;
	struct ga_flags	flags[OPTION_COUNT + 1];
	int		c;
	int		n;
	char		*node = NULL;
	FILE		*pfp = NULL;
	char		pname[2*PATH_MAX + 1 + 1];	/* may be too short */
	char		*arg;				/* if '\\' present  */
	char		nodename[PATH_MAX + 1];
	int		no_path_names = 1;
	int		warn_violate = 0;
	int		have_cmd_line_pathspec = 0;
	int		rationalize_all = 0;
	int		argind = 0;
#ifdef APPLE_HYB
	int		hfs_ct = 0;
#endif	/* APPLE_HYB */


#ifdef __EMX__
	/* This gives wildcard expansion with Non-Posix shells with EMX */
	_wildcard(&argc, &argv);
#endif
	save_args(argc, argv);

#if	defined(USE_NLS)
	setlocale(LC_ALL, "");
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "mkisofs"	/* Use this only if it weren't */
#endif
	arg = searchfileinpath("share/locale", F_OK,
					SIP_ANY_FILE|SIP_NO_PATH, NULL);
	if (arg)
		(void) bindtextdomain(TEXT_DOMAIN, arg);
	else
#ifdef	PROTOTYPES
	(void) bindtextdomain(TEXT_DOMAIN, INS_BASE "/share/locale");
#else
	(void) bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
#endif
	(void) textdomain(TEXT_DOMAIN);
#endif


	if (argc < 2) {
		errmsgno(EX_BAD, _("Missing pathspec.\n"));
		susage(1);
	}
	/* Get the defaults from the .mkisofsrc file */
	read_rcfile(argv[0]);

	{
		int		i;

		for (i = 0; i < (int)OPTION_COUNT; i++) {
			flags[i] = mki_options[i].opt;
		}
		flags[i].ga_format = NULL;
		gl_flags = flags;
	}
	time(&begun);
	gettimeofday(&tv_begun, NULL);
	modification_date.l_sec  = tv_begun.tv_sec;
	modification_date.l_usec = tv_begun.tv_usec;
	modification_date.l_gmtoff = -100;

	cac--;
	cav++;
	c = getvargs(&cac, &cav, GA_NO_PROPS, flags);
	if (c < 0) {
		if (c == BADFLAG && strchr(cav[0], '=')) {
			argind = argc - cac;
			goto args_ok;
		}
		error(_("Bad Option '%s' (error %d %s).\n"),
					cav[0], c, getargerror(c));
		susage(EX_BAD);
	}
args_ok:
	if (argind == 0)
		argind = argc - cac;
	path_ind = argind;
	if (cac > 0)
		have_cmd_line_pathspec = 1;
	if (help)
		usage(0);
	if (pversion) {
		printf(_("mkisofs %s (%s-%s-%s)\n\n\
Copyright (C) 1993-1997 %s\n\
Copyright (C) 1997-2017 %s\n"),
			version_string,
			HOST_CPU, HOST_VENDOR, HOST_OS,
			_("Eric Youngdale"),
			_("Joerg Schilling"));
#ifdef	APPLE_HYB
		printf(_("Copyright (C) 1997-2001 James Pearson\n"));
#endif
#ifdef	UDF
		printf(_("Copyright (C) 2006      HELIOS Software GmbH\n"));
#endif
#ifdef	OPTION_SILO_BOOT
		printf(_("Warning: this is unofficial (modified) version of mkisofs that incorporates\n"));
		printf(_("	support for a non Sparc compliant boot method called SILO.\n"));
		printf(_("	The official method to create Sparc boot CDs is to use -sparc-boot\n"));
		printf(_("	In case of problems first test with an official version of mkisofs.\n"));
#endif
		exit(0);
	}
#ifdef	USE_FIND
	if (dofind) {
		finda_t	fa;

		cac = find_ac;
		cav = find_av;
		find_firstprim(&cac, &cav);
		find_pac = cac;
		find_pav = cav;
		argind = argc - find_ac;

		if (cac > 0) {
			find_argsinit(&fa);
			fa.walkflags = walkflags;
			fa.Argc = cac;
			fa.Argv = (char **)cav;
			find_node = find_parse(&fa);
			if (fa.primtype == FIND_ERRARG)
				comexit(fa.error);
			if (fa.primtype != FIND_ENDARGS)
				comerrno(EX_BAD, _("Incomplete expression.\n"));
			plusp = fa.plusp;
			find_patlen = fa.patlen;
			walkflags = fa.walkflags;
			maxdepth = fa.maxdepth;
			mindepth = fa.mindepth;

			if (find_node &&
			    !(check_image || print_size) &&
			    (outfile == NULL ||
			    (outfile[0] == '-' && outfile[1] == '\0'))) {
				if (find_pname(find_node, "-exec") ||
				    find_pname(find_node, "-exec+") ||
				    find_pname(find_node, "-ok"))
					comerrno(EX_BAD,
					_("Cannot -exec with '-o -'.\n"));
			}
		}

		if (find_ac <= 0 || find_ac == find_pac) {
			errmsgno(EX_BAD, _("Missing pathspec for -find.\n"));
			susage(EX_BAD);
		}
	}
#endif
#ifdef DVD_AUD_VID

	dvd_aud_vid_flag = (dvd_audio ? DVD_SPEC_AUDIO : 0)
			| (dvd_hybrid ? DVD_SPEC_HYBRD : 0)
			| (dvd_video ? DVD_SPEC_VIDEO : 0);

#endif

	if (abstract) {
		if (strlen(abstract) > 37) {
			comerrno(EX_BAD,
			_("Abstract filename string too long (cur. %lld max. 37 chars).\n"),
			(Llong)strlen(abstract));
		}
	}
	if (appid) {
		if (strlen(appid) > 128) {
			comerrno(EX_BAD,
			_("Application-id string too long (cur. %lld max. 128 chars).\n"),
			(Llong)strlen(appid));
		}
	}
	if (biblio) {
		if (strlen(biblio) > 37) {
			comerrno(EX_BAD,
			_("Bibliographic filename string too long (cur. %lld max. 37 chars).\n"),
			(Llong)strlen(biblio));
		}
	}
#ifdef	DUPLICATES_ONCE
	/*
	 * If -duplicates-once was specified, do not implicitly enable
	 * -cache-inodes.
	 */
	if (cache_inodes < 0 && duplicates_once)
		cache_inodes = 0;
#endif
#if	defined(IS_CYGWIN)
	/*
	 * If we have 64 bit inode numbers, Cygwin should be able to work
	 * correctly on NTFS, otherwise disable caching unless it has
	 * been enforced via -cache-inodes.
	 */
	if (cache_inodes < 0 && sizeof (ino_t) < 8)
		cache_inodes = 0;
#endif
	if (cache_inodes < 0)
		cache_inodes = 1;
#ifdef	DUPLICATES_ONCE
	if (!cache_inodes && !duplicates_once) {
#else
	if (!cache_inodes) {
#endif
		correct_inodes = FALSE;
		if (use_RockRidge) {
			errmsgno(EX_BAD,
			_("Warning: Cannot write inode/link information with -no-cache-inodes.\n"));
		} else {
			errmsgno(EX_BAD,
			_("Warning: Cannot add inode hints with -no-cache-inodes.\n"));
		}
	}
#if	defined(__MINGW32__)
	if (cache_inodes) {
		cache_inodes = 0;
		correct_inodes = FALSE;
		errmsgno(EX_BAD,
		_("Warning: cannot -cache-inodes on this platform - ignoring.\n"));
	}
#endif
	if (!correct_inodes)
		rrip112 = FALSE;
	if (check_image) {
		check_session++;
		check_oldnames++;
		merge_image = check_image;
		outfile = DEV_NULL;
		/*
		 * cdrecord_data is handled specially in multi.c
		 * as we cannot write to all strings.
		 * If mkisofs is called with -C xx,yy
		 * our default is overwritten.
		 */
/*		cdrecord_data = "0,0";*/
	}
	if (copyright) {
		if (strlen(copyright) > 37) {
			comerrno(EX_BAD,
			_("Copyright filename string too long (cur. %lld max. 37 chars).\n"),
			(Llong)strlen(copyright));
		}
	}
	if (genboot_image)
		use_genboot++;
	if (boot_catalog)
		use_eltorito++;
	else
		boot_catalog = BOOT_CATALOG_DEFAULT;
	if (omit_period && iso9660_level < 4)
		warn_violate++;
	if (data_change_warn)
		errconfig("WARN|GROW|SHRINK *");
	if (dirmode_str) {
		char	*end = 0;

		rationalize++;
		use_RockRidge++;
		rationalize_dirmode++;

		/*
		 * If -new-dir-mode was specified, new_dir_mode is set
		 * up later with a value that matches the -new-dir-mode arg.
		 */
		new_dir_mode = dirmode_to_use = strtol(dirmode_str, &end, 8);
		if (!end || *end != 0 ||
		    dirmode_to_use < 0 || dirmode_to_use > 07777) {
			comerrno(EX_BAD, _("Bad mode for -dir-mode\n"));
		}
	}
	if (disable_deep_reloc)
		RR_relocation_depth = 0xFFFFFFFF;
	if (filemode_str) {
		char	*end = 0;

		rationalize++;
		use_RockRidge++;
		rationalize_filemode++;

		filemode_to_use = strtol(filemode_str, &end, 8);
		if (!end || *end != 0 ||
		    filemode_to_use < 0 || filemode_to_use > 07777) {
			comerrno(EX_BAD, _("Bad mode for -file-mode\n"));
		}
	}
#ifdef	__warn_follow__
	if (follow_links) {
			errmsgno(EX_BAD,
			_("Warning: -follow-links does not always work correctly; be careful.\n"));
	}
#endif
	if (gid_str) {
		char	*end = 0;

		rationalize++;
		use_RockRidge++;
		rationalize_gid++;

		gid_to_use = strtol(gid_str, &end, 0);
		if (!end || *end != 0) {
			comerrno(EX_BAD, _("Bad value for -gid\n"));
		}
	}
	switch (iso9660_level) {

	case 1:
		/*
		 * Only on file section
		 * 8.3 d or d1 characters for files
		 * 8   d or d1 characters for directories
		 */
		break;
	case 2:
		/*
		 * Only on file section
		 */
		break;
	case 3:
		/*
		 * No restrictions
		 */
		do_largefiles++;
		break;
	case 4:
		/*
		 * This is ISO-9660:1988 (ISO-9660 version 2)
		 */
		do_largefiles++;
		iso9660_namelen = MAX_ISONAME_V2; /* allow 207 chars */
		full_iso9660_filenames++;	/* 31+ chars	*/
		omit_version_number++;
		RR_relocation_depth = 0xFFFFFFFF;

		/*
		 * From -U ...
		 */
		omit_period++;			/* trailing dot */
		allow_leading_dots++;
		relaxed_filenames++;		/* all chars	*/
		allow_lowercase++;		/* even lowcase	*/
		allow_multidot++;		/* > 1 dots	*/
		break;

	default:
		comerrno(EX_BAD, _("Illegal iso9660 Level %d, use 1..3 or 4.\n"),
					iso9660_level);
	}

	if (joliet_long) {
		use_Joliet++;
		jlen = JLONGMAX;
	}
	if (jcharset) {
		use_Joliet++;
		icharset = jcharset;
	}
	if (max_filenames && iso9660_level < 4) {
		iso9660_namelen = MAX_ISONAME_V1; /* allow 37 chars */
		full_iso9660_filenames++;
		omit_version_number++;
		warn_violate++;
	}
	if (allow_leading_dots && iso9660_level < 4)
		warn_violate++;
	if (omit_version_number && iso9660_level < 4)
		warn_violate++;
	if (new_dirmode_str) {
		char	*end = 0;

		rationalize++;

		new_dir_mode = strtol(new_dirmode_str, &end, 8);
		if (!end || *end != 0 ||
		    new_dir_mode < 0 || new_dir_mode > 07777) {
			comerrno(EX_BAD, _("Bad mode for -new-dir-mode\n"));
		}
	}
	if (sectype) {
		if (strcmp(sectype, "data") == 0)
			osecsize = 2048;
		else if (strcmp(sectype, "xa1") == 0)
			osecsize = 2056;
		else if (strcmp(sectype, "raw") == 0) {
			osecsize = 2352;
			comerrno(EX_BAD,
				_("Unsupported sector type '%s'.\n"),
				sectype);
		}
	}
	if (preparer) {
		if (strlen(preparer) > 128) {
			comerrno(EX_BAD, _("Preparer string too long (cur. %lld max. 128 chars).\n"),
			(Llong)strlen(preparer));
		}
	}
	if (publisher) {
		if (strlen(publisher) > 128) {
			comerrno(EX_BAD,
				_("Publisher string too long (cur. %lld max. 128 chars).\n"),
				(Llong)strlen(publisher));
		}
	}
	if (rationalize_rr) {
		rationalize_all++;
		use_RockRidge++;
	}
	if (stream_filename) {
		if (strlen(stream_filename) > MAX_ISONAME)
			comerrno(EX_BAD,
				_("stream-file-name too long (%llu), max is %d.\n"),
				(Ullong)strlen(stream_filename), MAX_ISONAME);
		if (strchr(stream_filename, '/'))
			comerrno(EX_BAD, _("Illegal character '/' in stream-file-name.\n"));
		iso9660_level = 4;
	} else {
		stream_filename = omit_version_number ? "STREAM.IMG" : "STREAM.IMG;1";
	}
	if (system_id) {
		if (strlen(system_id) > 32) {
			comerrno(EX_BAD,
					_("System ID string too long\n"));
		}
	}
	if (trans_tbl)
		generate_tables++;
	else
		trans_tbl = "TRANS.TBL";
	if (ucs_level < 1 || ucs_level > 3)
		comerrno(EX_BAD, _("Illegal UCS Level %d, use 1..3.\n"),
						ucs_level);
#ifdef	DVD_AUD_VID
	if (dvd_aud_vid_flag) {
		if (!use_udf)
			rationalize_udf++;
	}
#endif
#ifdef	UDF
	if (rationalize_udf) {
		rationalize_all++;
		use_udf++;
	}
#endif
	if (uid_str) {
		char	*end = 0;

		rationalize++;
		use_RockRidge++;
		rationalize_uid++;

		uid_to_use = strtol(uid_str, &end, 0);
		if (!end || *end != 0) {
			comerrno(EX_BAD, _("Bad value for -uid\n"));
		}
	}
	if (untranslated_filenames && iso9660_level < 4) {
		/*
		 * Minimal (only truncation of 31+ characters)
		 * translation of filenames.
		 *
		 * Forces -l, -d, -N, -allow-leading-dots,
		 * -relaxed-filenames,
		 * -allow-lowercase, -allow-multidot
		 *
		 * This is for HP-UX, which does not recognize ANY
		 * extentions (Rock Ridge, Joliet), causing pain when
		 * loading software. pfs_mount can be used to read the
		 * extensions, but the untranslated filenames can be
		 * read by the "native" cdfs mounter. Completely
		 * violates iso9660.
		 */
		full_iso9660_filenames++;	/* 31 chars	*/
		omit_period++;			/* trailing dot */
		allow_leading_dots++;
		omit_version_number++;
		relaxed_filenames++;		/* all chars	*/
		allow_lowercase++;		/* even lowcase	*/
		allow_multidot++;		/* > 1 dots	*/
		warn_violate++;
	}
	if (no_allow_lowercase)
		allow_lowercase = 0;
	if (relaxed_filenames && iso9660_level < 4)
		warn_violate++;
	if (iso_translate == 0 && iso9660_level < 4)
		warn_violate++;
	if (allow_lowercase && iso9660_level < 4)
		warn_violate++;
	if (allow_multidot && iso9660_level < 4)
		warn_violate++;
	if (volume_id) {
		if (strlen(volume_id) > 32) {
			comerrno(EX_BAD,
				_("Volume ID string too long (cur. %lld max. 32 chars).\n"),
				(Llong)strlen(volume_id));
		}
	}
	if (volset_id) {
		if (strlen(volset_id) > 128) {
			comerrno(EX_BAD,
			_("Volume set ID string too long (cur. %lld max. 128 chars).\n"),
			(Llong)strlen(volset_id));
		}
	}
	if (volume_set_size) {
		if (volume_set_size <= 0) {
			comerrno(EX_BAD,
			_("Illegal Volume Set Size %d\n"), volume_set_size);
		}
		if (volume_set_size > 1) {
			comerrno(EX_BAD,
			_("Volume Set Size > 1 not yet supported\n"));
		}
	}
	if (volume_sequence_number) {
		if (volume_sequence_number > volume_set_size) {
			comerrno(EX_BAD,
			_("Volume set sequence number too big\n"));
		}
	}
	if (rationalize_xa) {
		rationalize_all++;
		use_XA++;
	}
	if (transparent_compression) {
#ifdef VMS
		comerrno(EX_BAD,
		_("Transparent compression not supported with VMS\n"));
#endif
	}
#ifdef APPLE_HYB
	if (deftype) {
		hfs_ct++;
		if (strlen(deftype) != 4) {
			comerrno(EX_BAD,
			_("HFS default TYPE string has illegal length.\n"));
		}
	} else {
		deftype = APPLE_TYPE_DEFAULT;
	}
	if (defcreator) {
		hfs_ct++;
		if (strlen(defcreator) != 4) {
			comerrno(EX_BAD,
			_("HFS default CREATOR string has illegal length.\n"));
		}
	} else {
		defcreator = APPLE_CREATOR_DEFAULT;
	}
	if (afpfile && *afpfile != '\0')
		hfs_last = MAP_LAST;
	if (magic_file)
		hfs_last = MAG_LAST;
	if (nomacfiles) {
		errmsgno(EX_BAD,
		_("Warning: -no-mac-files no longer used ... ignoring\n"));
	}
	if (hfs_boot_file)
		gen_pt = 1;
	if (root_info)
		icon_pos = 1;
	if (hfs_icharset)
		use_mac_name = 1;
	if (hfs_parms)
		hfs_parms = e_strdup(hfs_parms);

	if (apple_hyb && apple_ext) {
		comerrno(EX_BAD, _("Can't have both -apple and -hfs options\n"));
	}
	/*
	 * if -probe, -macname, any hfs selection and/or mapping file is given,
	 * but no HFS option, then select apple_hyb
	 */
	if (!apple_hyb && !apple_ext) {
		if (*afpfile || probe || use_mac_name || hfs_select ||
				hfs_boot_file || magic_file ||
				hfs_ishidden() || gen_pt || autoname ||
				afe_size || icon_pos || hfs_ct ||
				hfs_icharset || hfs_ocharset) {
			apple_hyb = 1;
#ifdef UDF
			if ((DO_XHFS & hfs_select) && use_udf) {
				donotwrite_macpart = 1;
				if (!no_apple_hyb) {
					error(
					_("Warning: no HFS hybrid will be created with -udf and --osx-hfs\n"));
				}
			}
#endif
		}
	}
#ifdef UDF
	if (!use_udf && create_udfsymlinks)
		create_udfsymlinks = 0;
#if 0
	if (use_RockRidge && use_udf && create_udfsymlinks) {
		error(_("Warning: cannot create UDF symlinks with activated Rock Ridge\n"));
		create_udfsymlinks = 0;
	}
#endif
#endif
	if (no_apple_hyb) {
		donotwrite_macpart = 1;
	}
	if (apple_hyb && !donotwrite_macpart && do_largefiles > 0) {
		do_largefiles = 0;
		maxnonlarge = (off_t)0x7FFFFFFF;
		error(_("Warning: cannot support large files with -hfs\n"));
	}
#ifdef UDF
	if (apple_hyb && use_udf && !donotwrite_macpart) {
		comerrno(EX_BAD, _("Can't have -hfs with -udf\n"));
	}
#endif
	if (apple_ext && hfs_boot_file) {
		comerrno(EX_BAD, _("Can't have -hfs-boot-file with -apple\n"));
	}
	if (apple_ext && autoname) {
		comerrno(EX_BAD, _("Can't have -auto with -apple\n"));
	}
	if (apple_hyb && (use_sparcboot || use_sunx86boot)) {
		comerrno(EX_BAD, _("Can't have -hfs with -sparc-boot/-sunx86-boot\n"));
	}
	if (apple_hyb && use_genboot) {
		comerrno(EX_BAD, _("Can't have -hfs with -generic-boot\n"));
	}
#ifdef PREP_BOOT
	if (apple_ext && use_prep_boot) {
		comerrno(EX_BAD, _("Can't have -prep-boot with -apple\n"));
	}
#endif	/* PREP_BOOT */

	if (apple_hyb || apple_ext)
		apple_both = 1;

	if (probe)
		/* we need to search for all types of Apple/Unix files */
		hfs_select = ~0;

	if (apple_both && verbose && !(hfs_select || *afpfile || magic_file)) {
		errmsgno(EX_BAD,
		_("Warning: no Apple/Unix files will be decoded/mapped\n"));
	}
	if (apple_both && verbose && !afe_size &&
					(hfs_select & (DO_FEU | DO_FEL))) {
		errmsgno(EX_BAD,
		_("Warning: assuming PC Exchange cluster size of 512 bytes\n"));
		afe_size = 512;
	}
	if (apple_both) {
		/* set up the TYPE/CREATOR mappings */
		hfs_init(afpfile, 0, hfs_select);
	}
	if (apple_ext && !use_RockRidge) {
#ifdef	nonono
		/* use RockRidge to set the SystemUse field ... */
		use_RockRidge++;
		rationalize_all++;
#else
		/* EMPTY */
#endif
	}
	if (apple_ext && !(use_XA || use_RockRidge)) {
		comerrno(EX_BAD, _("Need either -XA/-xa or -R/-r for -apple to become active.\n"));
	}
#endif	/* APPLE_HYB */

	/*
	 * if the -hide-joliet option has been given, set the Joliet option
	 */
	if (!use_Joliet && j_ishidden())
		use_Joliet++;
#ifdef	UDF
	/*
	 * if the -hide-udf option has been given, set the UDF option
	 */
	if (!use_udf && u_ishidden())
		use_udf++;
#endif

	if (rationalize_all) {
		rationalize++;
		rationalize_uid++;
		rationalize_gid++;
		rationalize_filemode++;
		rationalize_dirmode++;
	}

	/*
	 * XXX This is a hack until we have a decent separate name handling
	 * XXX for UDF filenames.
	 */
#ifdef DVD_AUD_VID
	if (dvd_aud_vid_flag && use_Joliet) {
		use_Joliet = 0;
		error(_("Warning: Disabling Joliet support for DVD-Video/DVD-Audio.\n"));
	}
#endif
#ifdef	UDF
	if (use_udf && !use_Joliet)
		jlen = 255;
#endif

	if (use_RockRidge && (iso9660_namelen > MAX_ISONAME_V2_RR))
		iso9660_namelen = MAX_ISONAME_V2_RR;

	if (warn_violate)
		error(_("Warning: creating filesystem that does not conform to ISO-9660.\n"));
	if (iso9660_level > 3)
		error(_("Warning: Creating ISO-9660:1999 (version 2) filesystem.\n"));
	if (iso9660_namelen > LEN_ISONAME)
		error(_("Warning: ISO-9660 filenames longer than %d may cause buffer overflows in the OS.\n"),
			LEN_ISONAME);
	if (use_Joliet && !use_RockRidge) {
		error(_("Warning: creating filesystem with (nonstandard) Joliet extensions\n"));
		error(_("         but without (standard) Rock Ridge extensions.\n"));
		error(_("         It is highly recommended to add Rock Ridge\n"));
	}
	if (transparent_compression) {
		error(_("Warning: using transparent compression. This is a nonstandard Rock Ridge\n"));
		error(_("         extension. The resulting filesystem can only be transparently\n"));
		error(_("         read on Linux. On other operating systems you need to call\n"));
		error(_("         mkzftree by hand to decompress the files.\n"));
	}
	if (transparent_compression && !use_RockRidge) {
		error(_("Warning: transparent decompression is a Linux Rock Ridge extension, but\n"));
		error(_("         creating filesystem without Rock Ridge attributes; files\n"));
		error(_("         will not be transparently decompressed.\n"));
	}

#if	defined(USE_NLS) && defined(HAVE_NL_LANGINFO) && defined(CODESET)
	/*
	 * If the locale has not been set up, nl_langinfo() returns the
	 * name of the default codeset. This should be either "646",
	 * "ISO-646", "ASCII", or something similar. Unfortunately, the
	 * POSIX standard does not include a list of valid locale names,
	 * so ne need to find all values in use.
	 *
	 * Observed:
	 * Solaris 	"646"
	 * Linux	"ANSI_X3.4-1968"	strange value from Linux...
	 */
	if (icharset == NULL) {
		char	*codeset = nl_langinfo(CODESET);
		Uchar	*p;

		if (codeset != NULL)
			codeset = e_strdup(codeset);
		if (codeset == NULL)			/* Should not happen */
			goto setcharset;
		if (*codeset == '\0')			/* Invalid locale    */
			goto setcharset;

		for (p = (Uchar *)codeset; *p != '\0'; p++) {
			if (islower(*p))
				*p = toupper(*p);
		}
		p = (Uchar *)strstr(codeset, "ISO");
		if (p != NULL) {
			if (*p == '_' || *p == '-')
				p++;
			codeset = (char *)p;
		}
		if (strcmp("646", codeset) != 0 &&
		    strcmp("ASCII", codeset) != 0 &&
		    strcmp("US-ASCII", codeset) != 0 &&
		    strcmp("US_ASCII", codeset) != 0 &&
		    strcmp("USASCII", codeset) != 0 &&
		    strcmp("ANSI_X3.4-1968", codeset) != 0)
			icharset = nl_langinfo(CODESET);

		if (codeset != NULL)
			free(codeset);

		if (verbose > 0 && icharset != NULL) {
			error(_("Setting input-charset to '%s' from locale.\n"),
				icharset);
		}
	}
setcharset:
	/*
	 * Allow to switch off locale with -input-charset "".
	 */
	if (icharset != NULL && *icharset == '\0')
		icharset = NULL;
#endif
	if (icharset == NULL) {
#if	(defined(__CYGWIN32__) || defined(__CYGWIN__) || defined(__DJGPP__)) && !defined(IS_CYGWIN_1)
		icharset = "cp437";
#else
		icharset = "default";
#endif
	}
	in_nls = sic_open(icharset);

	/*
	 * set the output charset to the same as the input or the given output
	 * charset
	 */
	if (ocharset == NULL) {
		ocharset = in_nls ? in_nls->sic_name : NULL;
	}
	out_nls = sic_open(ocharset);

	if (in_nls == NULL || out_nls == NULL) { /* Unknown charset specified */
		fprintf(stderr, _("Unknown charset '%s'.\nKnown charsets are:\n"),
		in_nls == NULL ? icharset : (ocharset ? ocharset : "NULL"));
		list_locales();
		exit(EX_BAD);
	}
#ifdef	USE_ICONV
	/*
	 * XXX If we ever allow this, we neeed to fix the call to conv_charset()
	 * XXX in name.c::iso9660_file_length().
	 */
	if ((in_nls->sic_cd2uni != NULL || out_nls->sic_cd2uni != NULL) &&
	    (in_nls->sic_name != out_nls->sic_name)) {
		errmsgno(EX_BAD,
		_("Iconv based locales may change file name length.\n"));
		comerrno(EX_BAD,
		_("Cannot yet have different -input-charset/-output-charset.\n"));
	}
#endif

#ifdef APPLE_HYB
	if (hfs_icharset == NULL || strcmp(hfs_icharset, "mac-roman") == 0) {
		hfs_icharset = "cp10000";
	}
	hfs_inls = sic_open(hfs_icharset);

	if (hfs_ocharset == NULL) {
		hfs_ocharset = hfs_inls ? hfs_inls->sic_name : NULL;
	}
	if (hfs_ocharset == NULL || strcmp(hfs_ocharset, "mac-roman") == 0) {
		hfs_ocharset = "cp10000";
	}
	hfs_onls = sic_open(hfs_ocharset);

	if (use_mac_name)
		apple_hyb = 1;
	if (apple_hyb && (hfs_inls == NULL || hfs_onls == NULL)) {
		fprintf(stderr, _("Unknown HFS charset '%s'.\nKnown charsets are:\n"),
		hfs_inls == NULL ? hfs_icharset : (hfs_ocharset ? hfs_ocharset : "NULL"));
		list_locales();
		exit(EX_BAD);
	}
#ifdef	USE_ICONV
	if (apple_hyb &&
	    ((hfs_inls->sic_cd2uni != NULL || hfs_onls->sic_cd2uni != NULL) &&
	    (hfs_inls->sic_name != hfs_onls->sic_name))) {
		errmsgno(EX_BAD,
		_("Iconv based locales may change file name length.\n"));
		comerrno(EX_BAD,
		_("Cannot yet have different -input-hfs-charset/-output-hfs-charset.\n"));
	}
#endif
#endif /* APPLE_HYB */

	if (merge_image != NULL) {
		if (open_merge_image(merge_image) < 0) {
			/* Complain and die. */
			comerr(_("Unable to open previous session image '%s'.\n"),
				merge_image);
		}
	}
	/* We don't need root privilleges anymore. */
#ifdef	HAVE_SETREUID
	if (setreuid(-1, getuid()) < 0)
#else
#ifdef	HAVE_SETEUID
	if (seteuid(getuid()) < 0)
#else
	if (setuid(getuid()) < 0)
#endif
#endif
		comerr(_("Panic cannot set back effective uid.\n"));


#ifdef	no_more_needed
#ifdef __NetBSD__
	{
		int		resource;
		struct rlimit	rlp;

		if (getrlimit(RLIMIT_DATA, &rlp) == -1)
			errmsg(_("Warning: Cannot get rlimit.\n"));
		else {
			rlp.rlim_cur = 33554432;
			if (setrlimit(RLIMIT_DATA, &rlp) == -1)
				errmsg(_("Warning: Cannot set rlimit.\n"));
		}
	}
#endif
#endif	/* no_more_needed */
#ifdef HAVE_SBRK
	mem_start = (unsigned long) sbrk(0);
#endif

	if (verbose > 1) {
		fprintf(stderr, "%s (%s-%s-%s)\n",
				version_string,
				HOST_CPU, HOST_VENDOR, HOST_OS);
	}
	if (cdrecord_data == NULL && !check_session && merge_image != NULL) {
		comerrno(EX_BAD,
		_("Multisession usage bug: Must specify -C if -M is used.\n"));
	}
	if (cdrecord_data != NULL && merge_image == NULL) {
		errmsgno(EX_BAD,
		_("Warning: -C specified without -M: old session data will not be merged.\n"));
	}
#ifdef APPLE_HYB
	if (merge_image != NULL && apple_hyb) {
		errmsgno(EX_BAD,
		_("Warning: files from previous sessions will not be included in the HFS volume.\n"));
	}
#endif	/* APPLE_HYB */

	/*
	 * see if we have a list of pathnames to process
	 */
	if (pathnames) {
		/* "-" means take list from the standard input */
		if (strcmp(pathnames, "-") != 0) {
			if ((pfp = fopen(pathnames, "r")) == NULL) {
				comerr(_("Unable to open pathname list %s.\n"),
								pathnames);
			}
		} else
			pfp = stdin;
	}

	/* The first step is to scan the directory tree, and take some notes */

	if ((arg = get_pnames(argc, argv, argind, pname,
					sizeof (pname), pfp)) == NULL) {
		if (check_session == 0 && !stream_media_size) {
			errmsgno(EX_BAD, _("Missing pathspec.\n"));
			susage(1);
		}
	}

	/*
	 * if we don't have a pathspec, then save the pathspec found
	 * in the pathnames file (stored in pname) - we don't want
	 * to skip this pathspec when we read the pathnames file again
	 */
	if (!have_cmd_line_pathspec && !stream_media_size) {
		save_pname = 1;
	}
	if (stream_media_size) {
#ifdef	UDF
		if (use_XA || use_RockRidge || use_udf || use_Joliet)
#else
		if (use_XA || use_RockRidge || use_Joliet)
#endif
			comerrno(EX_BAD,
			_("Cannot use XA, Rock Ridge, UDF or Joliet with -stream-media-size\n"));
		if (merge_image)
			comerrno(EX_BAD,
			_("Cannot use multi session with -stream-media-size\n"));
		if (use_eltorito || use_sparcboot || use_sunx86boot ||
#ifdef APPLE_HYB
		    use_genboot || use_prep_boot || hfs_boot_file)
#else
		    use_genboot)
#endif
			comerrno(EX_BAD,
			_("Cannot use boot options with -stream-media-size\n"));
#ifdef APPLE_HYB
		if (apple_hyb)
			comerrno(EX_BAD,
			_("Cannot use Apple hybrid options with -stream-media-size\n"));
#endif
	}

	if (use_RockRidge) {
		/* BEGIN CSTYLED */
#if 1
		extension_record = generate_rr_extension_record("RRIP_1991A",
			"THE ROCK RIDGE INTERCHANGE PROTOCOL PROVIDES SUPPORT FOR POSIX FILE SYSTEM SEMANTICS",
			"PLEASE CONTACT DISC PUBLISHER FOR SPECIFICATION SOURCE.  SEE PUBLISHER IDENTIFIER IN PRIMARY VOLUME DESCRIPTOR FOR CONTACT INFORMATION.",
			&extension_record_size);
#else
		extension_record = generate_rr_extension_record("IEEE_P1282",
			"THE IEEE P1282 PROTOCOL PROVIDES SUPPORT FOR POSIX FILE SYSTEM SEMANTICS",
			"PLEASE CONTACT THE IEEE STANDARDS DEPARTMENT, PISCATAWAY, NJ, USA FOR THE P1282 SPECIFICATION.",
			&extension_record_size);
#endif
		/* END CSTYLED */
	}
	checkarch(outfile);
	if (log_file) {
		FILE		*lfp;
		int		i;

		/* open log file - test that we can open OK */
		if ((lfp = fopen(log_file, "w")) == NULL) {
			comerr(_("Can't open logfile: '%s'.\n"), log_file);
		}
		fclose(lfp);

		/* redirect all stderr message to log_file */
		fprintf(stderr, _("re-directing all messages to %s\n"), log_file);
		fflush(stderr);

		/* associate stderr with the log file */
		if (freopen(log_file, "w", stderr) == NULL) {
			comerr(_("Can't open logfile: '%s'.\n"), log_file);
		}
		if (verbose > 1) {
			for (i = 0; i < argc; i++)
				fprintf(stderr, "%s ", argv[i]);

			fprintf(stderr, "\n%s (%s-%s-%s)\n",
				version_string,
				HOST_CPU, HOST_VENDOR, HOST_OS);
		}
	}
	/* Find name of root directory. */
	if (arg != NULL)
		node = findgequal(arg);
	if (!use_graft_ptrs)
		node = NULL;
	if (node == NULL) {
		if (use_graft_ptrs && arg != NULL)
			node = escstrcpy(nodename, sizeof (nodename), arg);
		else
			node = arg;
	} else {
		/*
		 * Remove '\\' escape chars which are located
		 * before '\\' and '=' chars
		 */
		node = escstrcpy(nodename, sizeof (nodename), ++node);
	}

	/*
	 * See if boot catalog file exists in root directory, if not we will
	 * create it.
	 */
	if (use_eltorito)
		init_boot_catalog(node);

	/*
	 * Find the device and inode number of the root directory. Record this
	 * in the hash table so we don't scan it more than once.
	 */
	stat_filter(node, &statbuf);
	add_directory_hash(statbuf.st_dev, STAT_INODE(statbuf));

	memset(&de, 0, sizeof (de));

	/*
	 * PO:
	 * Isn't root NULL at this time anyway?
	 * I think it is created by the first call to
	 * find_or_create_directory() below.
	 */
	de.filedir = root;	/* We need this to bootstrap */

	if (cdrecord_data != NULL && merge_image == NULL) {
		/*
		 * in case we want to add a new session, but don't want to
		 * merge old one
		 */
		get_session_start(NULL);
	}
	if (merge_image != NULL) {
		char	sector[SECTOR_SIZE];
		UInt32_t extent;

		errno = 0;
		mrootp = merge_isofs(merge_image);
		if (mrootp == NULL) {
			/* Complain and die. */
			if (errno == 0)
				errno = -1;
			comerr(_("Unable to find previous session PVD '%s'.\n"),
				merge_image);
		}
		memcpy(de.isorec.extent, mrootp->extent, 8);

		/*
		 * Look for RR Attributes in '.' entry of root dir.
		 * This is the first ISO directory entry in the root dir.
		 */
		extent = get_733(mrootp->extent);
		readsecs(extent, sector, 1);
		c = rr_flags((struct iso_directory_record *)sector);
		if (c & RR_FLAG_XA)
			fprintf(stderr, _("XA signatures found\n"));
		if (c & RR_FLAG_AA)
			fprintf(stderr, _("AA signatures found\n"));
		if (c & ~(RR_FLAG_XA|RR_FLAG_AA)) {
			extern	int	su_version;
			extern	int	rr_version;
			extern	char	er_id[];

			if (c & RR_FLAG_SP) {
				fprintf(stderr, _("SUSP signatures version %d found\n"), su_version);
				if (c & RR_FLAG_ER) {
					if (rr_version < 1) {
						fprintf(stderr,
							_("No valid Rock Ridge signature found\n"));
						if (!force_rr)
							no_rr++;
					} else {
						fprintf(stderr,
							_("Rock Ridge signatures version %d found\n"),
						rr_version);
						fprintf(stderr,
							_("Rock Ridge id '%s'\n"), er_id);
					}
				}
			} else {
				fprintf(stderr, _("Bad Rock Ridge signatures found (SU record missing)\n"));
				if (!force_rr)
					no_rr++;
			}
		} else {
			fprintf(stderr, _("No SUSP/Rock Ridge present\n"));
			if ((c & (RR_FLAG_XA|RR_FLAG_AA)) == 0) {
				if (!force_rr)
					no_rr++;
			}
		}
		if (no_rr)
			fprintf(stderr, _("Disabling Rock Ridge / XA / AA\n"));
	}
	/*
	 * Create an empty root directory. If we ever scan it for real,
	 * we will fill in the contents.
	 */
	find_or_create_directory(NULL, "", &de, TRUE);

#ifdef APPLE_HYB
	/* may need to set window layout of the volume */
	if (root_info)
		set_root_info(root_info);
#endif /* APPLE_HYB */

	/*
	 * Scan the actual directory (and any we find below it) for files to
	 * write out to the output image.  Note - we take multiple source
	 * directories and keep merging them onto the image.
	 */
	if (check_session)
		goto path_done;

#ifdef	USE_FIND
	if (dofind) {
extern		int	walkfunc	__PR((char *nm, struct stat *fs, int type, struct WALK *state));

		walkinitstate(&walkstate);
		if (find_patlen > 0) {
			walkstate.patstate = ___malloc(sizeof (int) * find_patlen,
						_("space for pattern state"));
		}

		find_timeinit(time(0));
		walkstate.walkflags	= walkflags;
		walkstate.maxdepth	= maxdepth;
		walkstate.mindepth	= mindepth;
		walkstate.lname		= NULL;
		walkstate.tree		= find_node;
		walkstate.err		= 0;
		walkstate.pflags	= 0;

		nodesc = TRUE;
		for (;
		    (arg = get_pnames(argc, argv, argind, pname, sizeof (pname),
							pfp)) != NULL;
								argind++) {
			/*
			 * Make silly GCC happy and double initialize graft_dir.
			 */
			struct directory *graft_dir = NULL;
			char		graft_point[PATH_MAX + 1];
			struct wargs	wa;
			char		*snp;

			graft_point[0] = '\0';
			snp = NULL;
			if (use_graft_ptrs)
				graft_dir = get_graft(arg,
						    graft_point, sizeof (graft_point),
						    nodename, sizeof (nodename),
						    &snp, FALSE);
			if (graft_point[0] != '\0') {
				arg = nodename;
				wa.dir = graft_dir;
			} else {
				wa.dir = root;
			}
			wa.name = snp;
			walkstate.auxp = &wa;
			walkstate.auxi = strlen(arg);
			treewalk(arg, walkfunc, &walkstate);
			no_path_names = 0;
		}
		find_plusflush(plusp, &walkstate);
	} else
#endif

	while ((arg = get_pnames(argc, argv, argind, pname,
					sizeof (pname), pfp)) != NULL) {
		char		graft_point[PATH_MAX + 1];

		get_graft(arg, graft_point, sizeof (graft_point),
				nodename, sizeof (nodename), NULL, TRUE);
		argind++;
		no_path_names = 0;
	}

path_done:
	if (pfp && pfp != stdin)
		fclose(pfp);

	/*
	 * exit if we don't have any pathnames to process
	 * - not going to happen at the moment as we have to have at least one
	 * path on the command line
	 */
	if (no_path_names && !check_session && !stream_media_size) {
		errmsgno(EX_BAD, _("No pathnames found.\n"));
		susage(1);
	}
	/*
	 * Now merge in any previous sessions.  This is driven on the source
	 * side, since we may need to create some additional directories.
	 */
	if (merge_image != NULL) {
		if (merge_previous_session(root, mrootp,
					reloc_root, reloc_old_root) < 0) {
			comerrno(EX_BAD, _("Cannot merge previous session.\n"));
		}
		close_merge_image();

		/*
		 * set up parent_dir and filedir in relocated entries which
		 * were read from previous session so that
		 * finish_cl_pl_entries can do its job
		 */
		match_cl_re_entries();
		free(mrootp);
	}
#ifdef APPLE_HYB
	/* free up any HFS filename mapping memory */
	if (apple_both)
		clean_hfs();
#endif	/* APPLE_HYB */

	/* hide "./rr_moved" if all its contents have been hidden */
	if (reloc_dir && i_ishidden())
		hide_reloc_dir();

	/* insert the boot catalog if required */
	if (use_eltorito)
		insert_boot_cat();

	/*
	 * Free up any matching memory
	 */
	for (n = 0; n < MAX_MAT; n++)
		gen_del_match(n);

#ifdef SORTING
	del_sort();
#endif /* SORTING */

	/*
	 * Sort the directories in the required order (by ISO9660).  Also,
	 * choose the names for the 8.3 filesystem if required, and do any
	 * other post-scan work.
	 */
	goof += sort_tree(root);

	if (goof) {
		comerrno(EX_BAD, _("ISO9660/Rock Ridge tree sort failed.\n"));
	}
#ifdef UDF
	if (use_Joliet || use_udf) {
#else
	if (use_Joliet) {
#endif
		goof += joliet_sort_tree(root);
	}
	if (goof) {
		comerrno(EX_BAD, _("Joliet tree sort failed.\n"));
	}
	/*
	 * Fix a couple of things in the root directory so that everything is
	 * self consistent. Fix this up so that the path tables get done right.
	 */
	root->self = root->contents;

	/* OK, ready to write the file.  Open it up, and generate the thing. */
	if (print_size) {
		discimage = fopen(DEV_NULL, "wb");
		if (!discimage) {
			comerr(_("Unable to open /dev/null\n"));
		}
	} else if (outfile != NULL &&
			!(outfile[0] == '-' && outfile[1] == '\0')) {
		discimage = fopen(outfile, "wb");
		if (!discimage) {
			comerr(_("Unable to open disc image file '%s'.\n"), outfile);
		}
	} else {
		discimage = stdout;
		setmode(fileno(stdout), O_BINARY);
	}
#ifdef	HAVE_SETVBUF
	setvbuf(discimage, NULL, _IOFBF, 64*1024);
#endif

	/* Now assign addresses on the disc for the path table. */

	path_blocks = ISO_BLOCKS(path_table_size);
	if (path_blocks & 1)
		path_blocks++;

	jpath_blocks = ISO_BLOCKS(jpath_table_size);
	if (jpath_blocks & 1)
		jpath_blocks++;

	/*
	 * Start to set up the linked list that we use to track the contents
	 * of the disc.
	 */
#ifdef APPLE_HYB
#ifdef PREP_BOOT
	if ((apple_hyb && !donotwrite_macpart) || use_prep_boot || use_chrp_boot)
#else	/* PREP_BOOT */
	if (apple_hyb && !donotwrite_macpart)
#endif	/* PREP_BOOT */
		outputlist_insert(&hfs_desc);
#endif	/* APPLE_HYB */
	if (use_sparcboot || use_sunx86boot)
		outputlist_insert(&sunlabel_desc);
	if (use_genboot)
		outputlist_insert(&genboot_desc);
	outputlist_insert(&startpad_desc);

	/* PVD for disc. */
	outputlist_insert(&voldesc_desc);

	/* SVD for El Torito. MUST be immediately after the PVD! */
	if (use_eltorito) {
		outputlist_insert(&torito_desc);
	}
	/* Enhanced PVD for disc. neded if we write ISO-9660:1999 */
	if (iso9660_level > 3)
		outputlist_insert(&xvoldesc_desc);

	/* SVD for Joliet. */
	if (use_Joliet) {
		outputlist_insert(&joliet_desc);
	}
	/* Finally the last volume descriptor. */
	outputlist_insert(&end_vol);

#ifdef UDF
	if (use_udf) {
		outputlist_insert(&udf_vol_recognition_area_frag);
	}
#endif

	/* Insert the version descriptor. */
	outputlist_insert(&version_desc);

#ifdef UDF
	if (use_udf) {
		/*
		 * Most of the space before sector 256 is wasted when
		 * UDF is turned on. The waste could be reduced by
		 * putting the ISO9660/Joliet structures before the
		 * pad_to_sector_256; the problem is that they might
		 * overshoot sector 256, so there would have to be some
		 * ugly logic to detect this case and rearrange things
		 * appropriately. I don't know if it's worth it.
		 */
		outputlist_insert(&udf_pad_to_sector_32_frag);
		outputlist_insert(&udf_main_seq_frag);
		outputlist_insert(&udf_main_seq_copy_frag);
		outputlist_insert(&udf_integ_seq_frag);
		outputlist_insert(&udf_pad_to_sector_256_frag);
		outputlist_insert(&udf_anchor_vol_desc_frag);
		outputlist_insert(&udf_file_set_desc_frag);
		outputlist_insert(&udf_dirtree_frag);
		outputlist_insert(&udf_file_entries_frag);
	}
#endif

	/* Now start with path tables and directory tree info. */
	if (!stream_media_size)
		outputlist_insert(&pathtable_desc);
	else
		outputlist_insert(&strpath_desc);

	if (use_Joliet) {
		outputlist_insert(&jpathtable_desc);
	}

	if (!stream_media_size)
		outputlist_insert(&dirtree_desc);

	if (use_Joliet) {
		outputlist_insert(&jdirtree_desc);
	}
	outputlist_insert(&dirtree_clean);

	if (extension_record) {
		outputlist_insert(&extension_desc);
	}

	if (!stream_media_size) {
		outputlist_insert(&files_desc);
	} else {
		outputlist_insert(&strfile_desc);
		outputlist_insert(&strdir_desc);
	}

	/*
	 * Allow room for the various headers we will be writing.
	 * There will always be a primary and an end volume descriptor.
	 */
	last_extent = session_start;

	/*
	 * Calculate the size of all of the components of the disc, and assign
	 * extent numbers.
	 */
	for (opnt = out_list; opnt; opnt = opnt->of_next) {
		opnt->of_start_extent = last_extent;
		if (opnt->of_size != NULL) {
			if (verbose > 2)
				fprintf(stderr, _("Computing size: %-40sStart Block %u\n"),
					opnt->of_name, last_extent);
			(*opnt->of_size) (last_extent);
		}
	}

	/*
	 * Generate the contents of any of the sections that we want to
	 * generate. Not all of the fragments will do anything here
	 * - most will generate the data on the fly when we get to the write
	 * pass.
	 */
	for (opnt = out_list; opnt; opnt = opnt->of_next) {
		if (opnt->of_generate != NULL) {
			if (verbose > 2)
				fprintf(stderr, _("Generating content: %-40s\n"),
					opnt->of_name);
			(*opnt->of_generate) ();
		}
	}

	/*
	 * Padding just after the ISO-9660 filesystem.
	 *
	 * files_desc does not have an of_size function. For this
	 * reason, we must insert us after the files content has been
	 * generated.
	 */
#ifdef UDF
	if (use_udf) {
		/* Single anchor volume descriptor pointer at end */
		outputlist_insert(&udf_end_anchor_vol_desc_frag);
		if (udf_end_anchor_vol_desc_frag.of_size != NULL) {
			(*udf_end_anchor_vol_desc_frag.of_size) (last_extent);
		}
		if (dopad) {
			/*
			 * Pad with anchor volume descriptor pointer
			 * blocks instead of zeroes.
			 */
			outputlist_insert(&udf_padend_avdp_frag);
			if (udf_padend_avdp_frag.of_size != NULL) {
				(*udf_padend_avdp_frag.of_size) (last_extent);
			}
		}
	} else
#endif
	if (dopad && !(use_sparcboot || use_sunx86boot)) {
		outputlist_insert(&endpad_desc);
		if (endpad_desc.of_size != NULL) {
			(*endpad_desc.of_size) (last_extent);
		}
	}
	c = 0;
	if (use_sparcboot) {
		if (dopad) {
			/* Padding before the boot partitions. */
			outputlist_insert(&interpad_desc);
			if (interpad_desc.of_size != NULL) {
				(*interpad_desc.of_size) (last_extent);
			}
		}
		c = make_sun_label();
		last_extent += c;
		outputlist_insert(&sunboot_desc);
		if (dopad) {
			outputlist_insert(&endpad_desc);
			if (endpad_desc.of_size != NULL) {
				(*endpad_desc.of_size) (last_extent);
			}
		}
	} else if (use_sunx86boot) {
		if (dopad) {
			/* Padding before the boot partitions. */
			outputlist_insert(&interpad_desc);
			if (interpad_desc.of_size != NULL) {
				(*interpad_desc.of_size) (last_extent);
			}
		}
		c = make_sunx86_label();
		last_extent += c;
		outputlist_insert(&sunboot_desc);
		if (dopad) {
			outputlist_insert(&endpad_desc);
			if (endpad_desc.of_size != NULL) {
				(*endpad_desc.of_size) (last_extent);
			}
		}
	}
	if (print_size > 0) {
		if (verbose > 0)
			fprintf(stderr,
			_("Total extents scheduled to be written = %u\n"),
			(last_extent - session_start));
		printf("%u\n", (last_extent - session_start));
		exit(0);
	}
	/*
	 * Now go through the list of fragments and write the data that
	 * corresponds to each one.
	 */
	for (opnt = out_list; opnt; opnt = opnt->of_next) {
		Uint	oext;

		oext = last_extent_written;
		if (opnt->of_start_extent != 0 &&
		    opnt->of_start_extent != last_extent_written) {
			/*
			 * Consistency check.
			 * XXX Should make sure that all entries have
			 * XXXX of_start_extent set up correctly.
			 */
			comerrno(EX_BAD,
			_("Implementation botch: %s should start at %u but starts at %u.\n"),
			opnt->of_name, opnt->of_start_extent, last_extent_written);
		}
		if (opnt->of_write != NULL) {
			if (verbose > 1)
				fprintf(stderr, _("Writing:   %-40sStart Block %u\n"),
					opnt->of_name, last_extent_written);
			(*opnt->of_write) (discimage);
			if (verbose > 1)
				fprintf(stderr, _("Done with: %-40sBlock(s)    %u\n"),
					opnt->of_name, last_extent_written-oext);
		}
	}
	if (last_extent != last_extent_written) {
		comerrno(EX_BAD,
		_("Implementation botch: FS should end at %u but ends at %u.\n"),
				last_extent, last_extent_written);
	}

	if (verbose > 0) {
#ifdef HAVE_SBRK
		fprintf(stderr, _("Max brk space used %x\n"),
			(unsigned int)(((unsigned long) sbrk(0)) - mem_start));
#endif
		fprintf(stderr, _("%u extents written (%u MB)\n"),
			(last_extent-session_start),
			(last_extent-session_start) >> 9);
	}
#ifdef VMS
	return (1);
#else
	return (0);
#endif
}

LOCAL void
list_locales()
{
	int	n;

	n = sic_list(stdout);
	if (n <= 0) {
		const char	*ins_base = sic_base();

		if (ins_base == NULL)
			ins_base = "$INS_BASE/lib/siconv/";
		errmsgno(EX_BAD, _("Installation problem: '%s' %s.\n"),
			ins_base, n < 0 ? _("missing"):_("incomplete"));
		if (n == 0) {
			errmsgno(EX_BAD,
			_("Check '%s' for missing translation tables.\n"),
			ins_base);
		}
	}
#ifdef	USE_ICONV
	if (n > 0) {
		errmsgno(EX_BAD,
		_("'iconv -l' lists more available names.\n"));
	}
#endif
}

/*
 * Find unescaped equal sign in graft pointer string.
 */
EXPORT char *
findgequal(s)
	char	*s;
{
	char	*p = s;

	while ((p = strchr(p, '=')) != NULL) {
		if (p > s && p[-1] != '\\')
			return (p);
		p++;
	}
	return (NULL);
}

/*
 * Find unescaped equal sign in string.
 */
LOCAL char *
escstrcpy(to, tolen, from)
	char	*to;
	size_t	tolen;
	char	*from;
{
	char	*p = to;

	if (debug)
		error("FROM: '%s'\n", from);

	to[0] = '\0';
	if (tolen > 0) {
		to[--tolen] = '\0';	/* Fill in last nul char   */
	}
	while ((*p = *from++) != '\0' && tolen-- > 0) {
		if (*p == '\\') {
			if ((*p = *from++) == '\0')
				break;
			if (*p != '\\' && *p != '=') {
				p[1] = p[0];
				*p++ = '\\';
			}
		}
		p++;
	}
	if (debug)
		error("ESC:  '%s'\n", to);
	return (to);
}

struct directory *
get_graft(arg, graft_point, glen, nodename, nlen, short_namep, do_insert)
	char		*arg;
	char		*graft_point;
	size_t		glen;
	char		*nodename;
	size_t		nlen;
	char		**short_namep;
	BOOL		do_insert;
{
	char		*node = NULL;
	struct directory_entry de;
	struct directory *graft_dir = root;
	struct stat	st;
	char		*short_name;
	int		status;

	fillbytes(&de, sizeof (de), '\0');
	/*
	 * We would like a syntax like:
	 *
	 *	/tmp=/usr/tmp/xxx
	 *
	 * where the user can specify a place to graft each component
	 * of the tree.  To do this, we may have to create directories
	 * along the way, of course. Secondly, I would like to allow
	 * the user to do something like:
	 *
	 *	/home/baz/RMAIL=/u3/users/baz/RMAIL
	 *
	 * so that normal files could also be injected into the tree
	 * at an arbitrary point.
	 *
	 * The idea is that the last component of whatever is being
	 * entered would take the name from the last component of
	 * whatever the user specifies.
	 *
	 * The default will be that the file is injected at the root of
	 * the image tree.
	 */
	node = findgequal(arg);
	if (!use_graft_ptrs)
		node = NULL;
	/*
	 * Remove '\\' escape chars which are located
	 * before '\\' and '=' chars ---> below in escstrcpy()
	 */

	short_name = NULL;

	if (node != NULL || reloc_root) {
		char		*pnt;
		char		*xpnt;
		size_t		len;

		/* insert -root prefix */
		if (reloc_root != NULL) {
			strlcpy(graft_point, reloc_root, glen);
			len = strlen(graft_point);

			if ((len < (glen -1)) &&
			    (len == 0 || graft_point[len-1] != '/')) {
				graft_point[len++] = '/';
				graft_point[len] = '\0';
			}
		} else {
			len = 0;
		}

		if (node) {
			*node = '\0';
			escstrcpy(&graft_point[len], glen - len, arg);
			*node = '=';
		}

		/*
		 * Remove unwanted "./" & "/" sequences from start...
		 */
		do {
			xpnt = graft_point;
			while (xpnt[0] == '.' && xpnt[1] == '/')
				xpnt += 2;
			while (*xpnt == PATH_SEPARATOR) {
				xpnt++;
			}
			/*
			 * The string becomes shorter, there is no need to check
			 * the length. Make sure to support overlapping strings.
			 */
			ovstrcpy(graft_point, xpnt);
		} while (xpnt > graft_point);

		if (node) {
			node = escstrcpy(nodename, nlen, ++node);
		} else {
			node = arg;
		}

		graft_dir = root;
		xpnt = graft_point;

		/*
		 * If "node" points to a directory, then graft_point
		 * needs to point to a directory too.
		 */
		if (follow_links)
			status = stat_filter(node, &st);
		else
			status = lstat_filter(node, &st);
		if (status == 0 && S_ISDIR(st.st_mode)) {
			len = strlen(graft_point);

			if ((len < (glen -1)) &&
			    (len == 0 || graft_point[len-1] != '/')) {
				graft_point[len++] = '/';
				graft_point[len] = '\0';
			}
		}
		if (debug)
			error("GRAFT:'%s'\n", xpnt);
		/*
		 * Loop down deeper and deeper until we find the
		 * correct insertion spot.
		 * Canonicalize the filename while parsing it.
		 */
		for (;;) {
			do {
				while (xpnt[0] == '.' && xpnt[1] == '/')
					xpnt += 2;
				while (xpnt[0] == '/')
					xpnt += 1;
				if (xpnt[0] == '.' && xpnt[1] == '.' && xpnt[2] == '/') {
					if (graft_dir && graft_dir != root) {
						graft_dir = graft_dir->parent;
						xpnt += 2;
					}
				}
			} while ((xpnt[0] == '/') || (xpnt[0] == '.' && xpnt[1] == '/'));
			pnt = strchr(xpnt, PATH_SEPARATOR);
			if (pnt == NULL) {
				if (*xpnt != '\0') {
					short_name = xpnt;
					if (short_namep)
						*short_namep = xpnt;
				}
				break;
			}
			*pnt = '\0';
			if (debug) {
				error("GRAFT Point:'%s' in '%s : %s' (%s)\n",
					xpnt,
					graft_dir->whole_name,
					graft_dir->de_name,
					graft_point);
			}
			graft_dir = find_or_create_directory(graft_dir,
				graft_point,
				NULL, TRUE);
			*pnt = PATH_SEPARATOR;
			xpnt = pnt + 1;
		}
	} else {
		graft_dir = root;
		if (use_graft_ptrs)
			node = escstrcpy(nodename, nlen, arg);
		else
			node = arg;
	}

	/*
	 * Now see whether the user wants to add a regular file, or a
	 * directory at this point.
	 */
	if (follow_links || Hflag)
		status = stat_filter(node, &st);
	else
		status = lstat_filter(node, &st);
	if (status != 0) {
		/*
		 * This is a fatal error - the user won't be getting
		 * what they want if we were to proceed.
		 */
		comerr(_("Invalid node - '%s'.\n"), node);
	} else {
		if (S_ISDIR(st.st_mode)) {
			if (debug) {
				error(_("graft_dir: '%s : %s', node: '%s', (scan)\n"),
					graft_dir->whole_name,
					graft_dir->de_name, node);
			}
			if (!do_insert)
				return (graft_dir);
			if (!scan_directory_tree(graft_dir,
							node, &de)) {
				exit(1);
			}
			if (debug) {
				error(_("scan done\n"));
			}
		} else {
			if (short_name == NULL) {
				short_name = strrchr(node,
						PATH_SEPARATOR);
				if (short_name == NULL ||
						short_name < node) {
					short_name = node;
				} else {
					short_name++;
				}
			}
			if (debug) {
				error(_("graft_dir: '%s : %s', node: '%s', short_name: '%s'\n"),
					graft_dir->whole_name,
					graft_dir->de_name, node,
					short_name);
			}
			if (!do_insert)
				return (graft_dir);
			if (!insert_file_entry(graft_dir, node,
							short_name, NULL, 0)) {
				/*
				 * Should we ignore this?
				 */
/*				exit(1);*/
				/* EMPTY */
			}
		}
	}
	return (graft_dir);
}

EXPORT void *
e_malloc(size)
	size_t		size;
{
	void		*pt = 0;

	if (size == 0)
		size = 1;
	if ((pt = malloc(size)) == NULL) {
		comerr(_("Not enough memory\n"));
	}
	/*
	 * Not all code is clean yet.
	 * Filling all allocated data with zeroes will help
	 * to avoid core dumps.
	 */
	memset(pt, 0, size);
	return (pt);
}

EXPORT char *
e_strdup(s)
	const	char	*s;
{
	char	*ret = strdup(s);

	if (s == NULL)
		comerr(_("Not enough memory for strdup(%s)\n"), s);
	return (ret);
}

/*
 * A strcpy() that works with overlapping buffers
 */
LOCAL void
ovstrcpy(p2, p1)
	register char	*p2;
	register char	*p1;
{
	while ((*p2++ = *p1++) != '\0')
		;
}

LOCAL void
checkarch(name)
	char	*name;
{
	struct stat	stbuf;

	archive_isreg = FALSE;
	archive_dev = (dev_t)0;
	archive_ino = (ino_t)0;

	if (name == NULL)
		return;
	if (stat(name, &stbuf) < 0)
		return;

	if (S_ISREG(stbuf.st_mode)) {
		archive_dev = stbuf.st_dev;
		archive_ino = stbuf.st_ino;
		archive_isreg = TRUE;
	} else if (((stbuf.st_mode & S_IFMT) == 0) ||
			S_ISFIFO(stbuf.st_mode) ||
			S_ISSOCK(stbuf.st_mode)) {
		/*
		 * This is a pipe or similar on different UNIX implementations.
		 * (stbuf.st_mode & S_IFMT) == 0 may happen in stange cases.
		 */
		archive_dev = NODEV;
		archive_ino = (ino_t)-1;
	}
}
