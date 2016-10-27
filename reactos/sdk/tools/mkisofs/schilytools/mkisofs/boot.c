/* @(#)boot.c	1.28 16/10/23 Copyright 1999-2016 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)boot.c	1.28 16/10/23 Copyright 1999-2016 J. Schilling";
#endif
/*
 *	Support for generic boot (sector 0..16)
 *	and to boot Sun sparc and Sun x86 systems.
 *
 *	Copyright (c) 1999-2016 J. Schilling
 */
/*@@C@@*/

#include "mkisofs.h"
#include <schily/fcntl.h>
#include <schily/utypes.h>
#include <schily/intcvt.h>
#include <schily/schily.h>
#include "sunlabel.h"

extern	int	use_sunx86boot;

LOCAL struct sun_label cd_label;
LOCAL struct x86_label sx86_label;
LOCAL struct pc_part	fdisk_part;
LOCAL char	*boot_files[NDKMAP];	/* Change this for > 8 x86 parts */

LOCAL	void	init_sparc_label	__PR((void));
LOCAL	void	init_sunx86_label	__PR((void));
EXPORT	int	sparc_boot_label	__PR((char *label));
EXPORT	int	sunx86_boot_label	__PR((char *label));
EXPORT	int	scan_sparc_boot		__PR((char *files));
EXPORT	int	scan_sunx86_boot	__PR((char *files));
EXPORT	int	make_sun_label		__PR((void));
EXPORT	int	make_sunx86_label	__PR((void));
LOCAL	void	dup_sun_label		__PR((int part));
LOCAL	int	sunboot_write		__PR((FILE *outfile));
LOCAL	int	sunlabel_size		__PR((UInt32_t starting_extent));
LOCAL	int	sunlabel_write		__PR((FILE *outfile));
LOCAL	int	genboot_size		__PR((UInt32_t starting_extent));
LOCAL	int	genboot_write		__PR((FILE *outfile));

/*
 * Set the virtual geometry in the disk label.
 * If we like to make the geometry variable, we may change
 * dkl_ncyl and dkl_pcyl later.
 */
LOCAL void
init_sparc_label()
{
	i_to_4_byte(cd_label.dkl_vtoc.v_version, V_VERSION);
	i_to_2_byte(cd_label.dkl_vtoc.v_nparts, NDKMAP);
	i_to_4_byte(cd_label.dkl_vtoc.v_sanity, VTOC_SANE);

	i_to_2_byte(cd_label.dkl_rpm, CD_RPM);
	i_to_2_byte(cd_label.dkl_pcyl, CD_PCYL);
	i_to_2_byte(cd_label.dkl_apc, CD_APC);
	i_to_2_byte(cd_label.dkl_intrlv, CD_INTRLV);
	i_to_2_byte(cd_label.dkl_ncyl, CD_NCYL);
	i_to_2_byte(cd_label.dkl_acyl, CD_ACYL);
	i_to_2_byte(cd_label.dkl_nhead, CD_NHEAD);
	i_to_2_byte(cd_label.dkl_nsect, CD_NSECT);

	cd_label.dkl_magic[0] =	DKL_MAGIC_0;
	cd_label.dkl_magic[1] =	DKL_MAGIC_1;
}

LOCAL void
init_sunx86_label()
{
	li_to_4_byte(sx86_label.dkl_vtoc.v_sanity, VTOC_SANE);
	li_to_4_byte(sx86_label.dkl_vtoc.v_version, V_VERSION);
	li_to_2_byte(sx86_label.dkl_vtoc.v_sectorsz, 512);
	li_to_2_byte(sx86_label.dkl_vtoc.v_nparts, NX86MAP);

	li_to_4_byte(sx86_label.dkl_pcyl, CD_PCYL);
	li_to_4_byte(sx86_label.dkl_ncyl, CD_NCYL);
	li_to_2_byte(sx86_label.dkl_acyl, CD_ACYL);
	li_to_2_byte(sx86_label.dkl_bcyl, 0);

	li_to_4_byte(sx86_label.dkl_nhead, CD_NHEAD);
	li_to_4_byte(sx86_label.dkl_nsect, CD_NSECT);
	li_to_2_byte(sx86_label.dkl_intrlv, CD_INTRLV);
	li_to_2_byte(sx86_label.dkl_skew, 0);
	li_to_2_byte(sx86_label.dkl_apc, CD_APC);
	li_to_2_byte(sx86_label.dkl_rpm, CD_RPM);

	li_to_2_byte(sx86_label.dkl_write_reinstruct, 0);
	li_to_2_byte(sx86_label.dkl_read_reinstruct, 0);

	li_to_2_byte(sx86_label.dkl_magic, DKL_MAGIC);
}

/*
 * For command line parser: set ASCII label.
 */
EXPORT int
sparc_boot_label(label)
	char	*label;
{
	strncpy(cd_label.dkl_ascilabel, label, 127);
	cd_label.dkl_ascilabel[127] = '\0';
	return (1);
}

EXPORT int
sunx86_boot_label(label)
	char	*label;
{
	strncpy(sx86_label.dkl_vtoc.v_asciilabel, label, 127);
	sx86_label.dkl_vtoc.v_asciilabel[127] = '\0';
	return (1);
}

/*
 * Parse the command line argument for boot images.
 */
EXPORT int
scan_sparc_boot(files)
	char	*files;
{
	char		*p;
	int		i = 1;
	struct stat	statbuf;
	int		status;
extern	int		use_sparcboot;
extern	int		use_sunx86boot;

	if (use_sunx86boot)
		comerrno(EX_BAD,
		_("-sparc-boot and -sunx86-boot are mutual exclusive.\n"));
	use_sparcboot++;

	init_sparc_label();

	do {
		if (i >= NDKMAP)
			comerrno(EX_BAD, _("Too many boot partitions.\n"));
		boot_files[i++] = files;
		if ((p = strchr(files, ',')) != NULL)
			*p++ = '\0';
		files = p;
	} while (p);

	i_to_2_byte(cd_label.dkl_vtoc.v_part[0].p_tag,  V_USR);
	i_to_2_byte(cd_label.dkl_vtoc.v_part[0].p_flag, V_RONLY);
	for (i = 0; i < NDKMAP; i++) {
		p = boot_files[i];
		if (p == NULL || *p == '\0')
			continue;
		if (strcmp(p, "...") == '\0')
			break;

		status = stat_filter(p, &statbuf);
		if (status < 0 || access(p, R_OK) < 0)
			comerr(_("Cannot access '%s'.\n"), p);

		i_to_4_byte(cd_label.dkl_map[i].dkl_nblk,
			roundup(statbuf.st_size, CD_CYLSIZE)/512);

		i_to_2_byte(cd_label.dkl_vtoc.v_part[i].p_tag,  V_ROOT);
		i_to_2_byte(cd_label.dkl_vtoc.v_part[i].p_flag, V_RONLY);
	}
	return (1);
}

EXPORT int
scan_sunx86_boot(files)
	char	*files;
{
	char		*p;
	int		i = 0;
	struct stat	statbuf;
	int		status;
extern	int		use_sparcboot;
extern	int		use_sunx86boot;

	if (use_sparcboot)
		comerrno(EX_BAD,
		_("-sparc-boot and -sunx86-boot are mutual exclusive.\n"));
	use_sunx86boot++;


	init_sunx86_label();

	do {
		if (i >= NDKMAP)
			comerrno(EX_BAD, _("Too many boot partitions.\n"));
		boot_files[i++] = files;
		if ((p = strchr(files, ',')) != NULL)
			*p++ = '\0';
		files = p;
	} while (p);

	li_to_2_byte(sx86_label.dkl_vtoc.v_part[0].p_tag,  V_ROOT);  /* UFS */
	li_to_2_byte(sx86_label.dkl_vtoc.v_part[0].p_flag, V_RONLY);
	li_to_2_byte(sx86_label.dkl_vtoc.v_part[1].p_tag,  V_USR);   /* ISO */
	li_to_2_byte(sx86_label.dkl_vtoc.v_part[1].p_flag, V_RONLY);
	li_to_2_byte(sx86_label.dkl_vtoc.v_part[2].p_tag,  0);	    /* ALL */
	li_to_2_byte(sx86_label.dkl_vtoc.v_part[2].p_flag, 0);
	for (i = 0; i < NDKMAP; i++) {
		p = boot_files[i];
		if (p == NULL || *p == '\0')
			continue;
		if (i == 1 || i == 2) {
			comerrno(EX_BAD,
			_("Partition %d may not have a filename.\n"), i);
		}

		status = stat_filter(p, &statbuf);
		if (status < 0 || access(p, R_OK) < 0)
			comerr(_("Cannot access '%s'.\n"), p);

		li_to_4_byte(sx86_label.dkl_vtoc.v_part[i].p_size,
			roundup(statbuf.st_size, CD_CYLSIZE)/512);

		if (i > 2) {
			li_to_2_byte(sx86_label.dkl_vtoc.v_part[i].p_tag,  V_USR);
			li_to_2_byte(sx86_label.dkl_vtoc.v_part[i].p_flag, V_RONLY);
		}
	}
	return (1);
}

/*
 * Finish the Sun disk label and compute the size of the additional data.
 */
EXPORT int
make_sun_label()
{
	int	last;
	int	cyl = 0;
	int	nblk;
	int	bsize;
	int	i;
	char	*p;

	/*
	 * Compute the size of the padding for the iso9660 image
	 * to allow the next partition to start on a cylinder boundary.
	 */
	last = roundup(last_extent, (CD_CYLSIZE/SECTOR_SIZE));

	i_to_4_byte(cd_label.dkl_map[0].dkl_nblk, last*4);
	bsize = 0;
	for (i = 0; i < NDKMAP; i++) {
		p = boot_files[i];
		if (p != NULL && strcmp(p, "...") == '\0') {
			dup_sun_label(i);
			break;
		}
		if ((nblk = a_to_4_byte(cd_label.dkl_map[i].dkl_nblk)) == 0)
			continue;

		i_to_4_byte(cd_label.dkl_map[i].dkl_cylno, cyl);
		cyl += nblk / (CD_CYLSIZE/512);
		if (i > 0)
			bsize += nblk;
	}
	bsize /= 4;
	return (last-last_extent+bsize);
}

/*
 * A typical Solaris boot/install CD from a Sun CD set looks
 * this way:
 *
 * UFS	Part 0 tag 2 flag 10 start 3839 size 1314560
 * ISO	Part 1 tag 4 flag 10 start    0 size    3839
 * ALL	Part 2 tag 0 flag  0 start    0 size 1318400
 */
EXPORT int
make_sunx86_label()
{
	int	last;
	int	cyl = 0;
	int	nblk;
	int	bsize;
	int	i;
	int	partoff = 1;	/* The offset of the Solaris 0x82 partition */

	/*
	 * Compute the size of the padding for the iso9660 image
	 * to allow the next partition to start on a cylinder boundary.
	 */
	last = roundup(last_extent, (CD_CYLSIZE/SECTOR_SIZE));

	li_to_4_byte(sx86_label.dkl_vtoc.v_part[1].p_size, last*4);

	/*
	 * Note that the Solaris fdisk partition with fdisk signature 0x82
	 * is created at fixed offset 1 sector == 512 Bytes by this
	 * implementation.
	 * We need subtract this partition offset from all absolute
	 * partition offsets in order to get offsets relative to the
	 * Solaris primary partition.
	 */
	bsize = 0;
	for (i = 0; i < NDKMAP; i++) {
		if (i == 2)		/* Never include the whole disk in */
			continue;	/* size/offset computations	   */

		if ((nblk = la_to_4_byte(sx86_label.dkl_vtoc.v_part[i].p_size)) == 0)
			continue;

		li_to_4_byte(sx86_label.dkl_vtoc.v_part[i].p_start,
						cyl*(CD_CYLSIZE/512)-partoff);
		cyl += nblk / (CD_CYLSIZE/512);
		if (i == 0 || i > 2)
			bsize += nblk;
	}
	li_to_4_byte(sx86_label.dkl_vtoc.v_part[0].p_start, last*4-partoff);
	li_to_4_byte(sx86_label.dkl_vtoc.v_part[1].p_start, 0);
	li_to_4_byte(sx86_label.dkl_vtoc.v_part[1].p_size, last*4-partoff);
	li_to_4_byte(sx86_label.dkl_vtoc.v_part[2].p_start, 0);
	li_to_4_byte(sx86_label.dkl_vtoc.v_part[2].p_size, last*4+bsize);

	fdisk_part.part[0].pr_status = STATUS_ACTIVE;
	fdisk_part.part[0].pr_type   = TYPE_SOLARIS;
	li_to_4_byte(fdisk_part.part[0].pr_partoff, partoff);
	li_to_4_byte(fdisk_part.part[0].pr_nsect, last*4+bsize-partoff);
	fdisk_part.magic[0] = 0x55;
	fdisk_part.magic[1] = 0xAA;

	bsize /= 4;
	return (last-last_extent+bsize);
}

/*
 * Duplicate a partition of the Sun disk label until all partitions are filled up.
 */
LOCAL void
dup_sun_label(part)
	int	part;
{
	int	cyl;
	int	nblk;
	int	i;


	if (part < 1 || part >= NDKMAP)
		part = 1;
	cyl = a_to_4_byte(cd_label.dkl_map[part-1].dkl_cylno);
	nblk = a_to_4_byte(cd_label.dkl_map[part-1].dkl_nblk);

	for (i = part; i < NDKMAP; i++) {
		i_to_4_byte(cd_label.dkl_map[i].dkl_cylno, cyl);
		i_to_4_byte(cd_label.dkl_map[i].dkl_nblk, nblk);

		i_to_2_byte(cd_label.dkl_vtoc.v_part[i].p_tag,  V_ROOT);
		i_to_2_byte(cd_label.dkl_vtoc.v_part[i].p_flag, V_RONLY);
	}
}

/*
 * Write out Sun boot partitions.
 */
LOCAL int
sunboot_write(outfile)
	FILE	*outfile;
{
	char	buffer[SECTOR_SIZE];
	int	i;
	int	n;
	int	nblk;
	int	amt;
	int	f;
	char	*p;

	memset(buffer, 0, sizeof (buffer));

	/*
	 * Write padding to the iso9660 image to allow the
	 * boot partitions to start on a cylinder boundary.
	 */
	amt = roundup(last_extent_written, (CD_CYLSIZE/SECTOR_SIZE)) - last_extent_written;
	for (n = 0; n < amt; n++) {
		xfwrite(buffer, SECTOR_SIZE, 1, outfile, 0, FALSE);
		last_extent_written++;
	}
	if (use_sunx86boot)
		i = 0;
	else
		i = 1;
	for (; i < NDKMAP; i++) {
		if (use_sunx86boot && (i == 1 || i == 2))
			continue;
		p = boot_files[i];
		if (p == NULL || *p == '\0')
			continue;
		if (p != NULL && strcmp(p, "...") == '\0')
			break;
		if (use_sunx86boot) {
			if ((nblk = la_to_4_byte(sx86_label.dkl_vtoc.v_part[i].p_size)) == 0)
				continue;
		} else {
			if ((nblk = a_to_4_byte(cd_label.dkl_map[i].dkl_nblk)) == 0)
				continue;
		}
		if ((f = open(boot_files[i], O_RDONLY| O_BINARY)) < 0)
			comerr(_("Cannot open '%s'.\n"), boot_files[i]);

		amt = nblk / 4;
		for (n = 0; n < amt; n++) {
			memset(buffer, 0, sizeof (buffer));
			if (read(f, buffer, SECTOR_SIZE) < 0)
				comerr(_("Read error on '%s'.\n"), boot_files[i]);
			xfwrite(buffer, SECTOR_SIZE, 1, outfile, 0, FALSE);
			last_extent_written++;
		}
		close(f);
	}
	fprintf(stderr, _("Total extents including %s boot = %u\n"),
				use_sunx86boot ? "Solaris x86":"sparc",
				last_extent_written - session_start);
	return (0);
}

/*
 * Do size management for the Sun disk label that is located in the first
 * sector of a disk.
 */
LOCAL int
sunlabel_size(starting_extent)
	UInt32_t	starting_extent;
{
	if (last_extent != session_start)
		comerrno(EX_BAD, _("Cannot create sparc boot on offset != 0.\n"));
	last_extent++;
	return (0);
}

/*
 * Compute the checksum and write a Sun disk label to the first sector
 * of the disk.
 * If the -generic-boot option has been specified too, overlay the
 * Sun disk label on the first 512 bytes of the generic boot code.
 */
LOCAL int
sunlabel_write(outfile)
	FILE	*outfile;
{
		char	buffer[SECTOR_SIZE];
	register char	*p;
	register short	count = (512/2) - 1;
		int	f;

	memset(buffer, 0, sizeof (buffer));
	if (genboot_image) {
		if ((f = open(genboot_image, O_RDONLY| O_BINARY)) < 0)
			comerr(_("Cannot open '%s'.\n"), genboot_image);

		if (read(f, buffer, SECTOR_SIZE) < 0)
			comerr(_("Read error on '%s'.\n"), genboot_image);
		close(f);
	}

	if (use_sunx86boot) {
		if (sx86_label.dkl_vtoc.v_asciilabel[0] == '\0')
			strcpy(sx86_label.dkl_vtoc.v_asciilabel, CD_X86LABEL);

		p = (char *)&sx86_label;
		sx86_label.dkl_cksum[0] = 0;
		sx86_label.dkl_cksum[1] = 0;
		while (count-- > 0) {
			sx86_label.dkl_cksum[0] ^= *p++;
			sx86_label.dkl_cksum[1] ^= *p++;
		}
		memcpy(&buffer[0x1BE], fdisk_part.part, sizeof (fdisk_part.part));
		p = &buffer[510];
		*p++ = 0x55;
		*p   = 0xAA;
		memcpy(&buffer[1024], &sx86_label, 512);
	} else {
		/*
		 * If we don't already have a Sun disk label text
		 * set up the default.
		 */
		if (cd_label.dkl_ascilabel[0] == '\0')
			strcpy(cd_label.dkl_ascilabel, CD_DEFLABEL);

		p = (char *)&cd_label;
		cd_label.dkl_cksum[0] = 0;
		cd_label.dkl_cksum[1] = 0;
		while (count--) {
			cd_label.dkl_cksum[0] ^= *p++;
			cd_label.dkl_cksum[1] ^= *p++;
		}
		memcpy(buffer, &cd_label, 512);
	}

	xfwrite(buffer, SECTOR_SIZE, 1, outfile, 0, FALSE);
	last_extent_written++;
	return (0);
}

/*
 * Do size management for the generic boot code on sectors 0..16.
 */
LOCAL int
genboot_size(starting_extent)
	UInt32_t	starting_extent;
{
	if (last_extent > (session_start + 1))
		comerrno(EX_BAD, _("Cannot create generic boot on offset != 0.\n"));
	last_extent = session_start + 16;
	return (0);
}

/*
 * Write the generic boot code to sectors 0..16.
 * If there is a Sun disk label, start writing at sector 1.
 */
LOCAL int
genboot_write(outfile)
	FILE	*outfile;
{
	char	buffer[SECTOR_SIZE];
	int	i;
	int	f;

	if ((f = open(genboot_image, O_RDONLY| O_BINARY)) < 0)
		comerr(_("Cannot open '%s'.\n"), genboot_image);

	for (i = 0; i < 16; i++) {
		memset(buffer, 0, sizeof (buffer));
		if (read(f, buffer, SECTOR_SIZE) < 0)
			comerr(_("Read error on '%s'.\n"), genboot_image);

		if (i != 0 || last_extent_written == session_start) {
			xfwrite(buffer, SECTOR_SIZE, 1, outfile, 0, FALSE);
			last_extent_written++;
		}
	}
	close(f);
	return (0);
}

struct output_fragment sunboot_desc	= {NULL, NULL,		NULL,	sunboot_write,  "Sun Boot" };
struct output_fragment sunlabel_desc	= {NULL, sunlabel_size,	NULL,	sunlabel_write, "Sun Disk Label" };
struct output_fragment genboot_desc	= {NULL, genboot_size,	NULL,	genboot_write,  "Generic Boot" };
