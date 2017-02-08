/* @(#)eltorito.c	1.52 15/11/23 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)eltorito.c	1.52 15/11/23 joerg";

#endif
/*
 * Program eltorito.c - Handle El Torito specific extensions to iso9660.
 *
 *
 *  Written by Michael Fulbright <msf@redhat.com> (1996).
 *
 * Copyright 1996 RedHat Software, Incorporated
 * Copyright (c) 1999-2015 J. Schilling
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

#include "mkisofs.h"
#include <schily/fcntl.h>
#include <schily/utypes.h>
#include <schily/intcvt.h>
#include "match.h"
#include "diskmbr.h"
#include "bootinfo.h"
#include <schily/schily.h>

#undef MIN
#define	MIN(a, b) (((a) < (b))? (a): (b))

EXPORT	void	init_boot_catalog	__PR((const char *path));
EXPORT	void	insert_boot_cat		__PR((void));
LOCAL	void	get_torito_desc		__PR((struct eltorito_boot_descriptor *boot_desc));
LOCAL  void	fill_boot_shdr		__PR((struct eltorito_sectionheader_entry *boot_shdr_entry,
						int arch));
LOCAL	void	fill_boot_desc		__PR((struct eltorito_defaultboot_entry *boot_desc_entry,
						struct eltorito_boot_entry_info *boot_entry));
EXPORT	void	get_boot_entry		__PR((void));
EXPORT	int	new_boot_entry		__PR((void));
EXPORT	void	ex_boot_enoent		__PR((char *msg, char *pname));
LOCAL	int	tvd_write		__PR((FILE *outfile));


LOCAL	struct eltorito_validation_entry valid_desc;
LOCAL	struct eltorito_boot_descriptor	gboot_desc;
LOCAL	struct disk_master_boot_record	disk_mbr;
LOCAL	unsigned int			bcat_de_flags;
LOCAL	char				*bootcat_path; /* name of bootcatalog */

/*
 * Make sure any existing boot catalog is excluded
 */
EXPORT void
init_boot_catalog(path)
	const char	*path;
{
#ifdef	SORTING
	struct eltorito_boot_entry_info	*cbe;

	for (cbe = first_boot_entry;
	    cbe != NULL;
	    cbe = cbe->next) {
		char	*p;

		if (cbe->boot_image == NULL)
			comerrno(EX_BAD, _("Missing boot image name, use -eltorito-boot option.\n"));
		p = (char *)e_malloc(strlen(cbe->boot_image) + strlen(path) + 2);
		strcpy(p, path);
		if (p[strlen(p) - 1] != '/') {
			strcat(p, "/");
		}
		strcat(p, cbe->boot_image);
		add_sort_match(p, sort_matches(p, 1));
		free(p);
	}
#endif
	bootcat_path = (char *)e_malloc(strlen(boot_catalog) + strlen(path) + 2);
	strcpy(bootcat_path, path);
	if (bootcat_path[strlen(bootcat_path) - 1] != '/') {
		strcat(bootcat_path, "/");
	}
	strcat(bootcat_path, boot_catalog);

	/*
	 * we are going to create a virtual catalog file
	 * - so make sure any existing is excluded
	 */
	add_match(bootcat_path);

	/* flag the file as a memory file */
	bcat_de_flags = MEMORY_FILE;

	/* find out if we want to "hide" this file */
	if (i_matches(boot_catalog) || i_matches(bootcat_path))
		bcat_de_flags |= INHIBIT_ISO9660_ENTRY;

	if (j_matches(boot_catalog) || j_matches(bootcat_path))
		bcat_de_flags |= INHIBIT_JOLIET_ENTRY;

	if (u_matches(boot_catalog) || u_matches(bootcat_path))
		bcat_de_flags |= INHIBIT_UDF_ENTRY;

} /* init_boot_catalog(... */

/*
 * Create a boot catalog file in memory - mkisofs already uses this type of
 * file for the TRANS.TBL files. Therefore the boot catalog is set up in
 * similar way
 */
EXPORT void
insert_boot_cat()
{
	struct directory_entry	*de;
	struct directory_entry	*s_entry;
	char			*p1;
	char			*p2;
	char			*p3;
	struct directory	*this_dir;
	struct directory	*dir;
	char			*buffer;

	init_fstatbuf();

	buffer = (char *)e_malloc(SECTOR_SIZE);
	memset(buffer, 0, SECTOR_SIZE);

	/*
	 * try to find the directory that will contain the boot.cat file
	 * - not very neat, but I can't think of a better way
	 */
	p1 = e_strdup(boot_catalog);

	/* get dirname (p1) and basename (p2) of boot.cat */
	if ((p2 = strrchr(p1, '/')) != NULL) {
		*p2 = '\0';
		p2++;

		/* find the dirname directory entry */
		de = search_tree_file(root, p1);
		if (!de) {
			ex_boot_enoent(_("catalog directory"), p1);
			/* NOTREACHED */
		}
		this_dir = 0;

		/* get the basename (p3) of the directory */
		if ((p3 = strrchr(p1, '/')) != NULL)
			p3++;
		else
			p3 = p1;

		/* find the correct sub-directory entry */
		for (dir = de->filedir->subdir; dir; dir = dir->next)
			if (strcmp(dir->de_name, p3) == 0)
				this_dir = dir;

		if (this_dir == 0) {
			ex_boot_enoent(_("catalog directory"), p3);
			/* NOTREACHED */
		}
	} else {
		/* boot.cat is in the root directory */
		this_dir = root;
		p2 = p1;
	}

	/*
	 * make a directory entry in memory (using the same set up as for table
	 * entries
	 */
	s_entry = (struct directory_entry *)
		e_malloc(sizeof (struct directory_entry));
	memset(s_entry, 0, sizeof (struct directory_entry));
	s_entry->next = this_dir->contents;
	this_dir->contents = s_entry;

#ifdef SORTING
	/* inherit any sort weight from parent directory */
	s_entry->sort = this_dir->sort;
	s_entry->sort += 2;

	/* see if this entry should have a new weighting */
	if (do_sort) {
		s_entry->sort = sort_matches(bootcat_path, s_entry->sort);
	}
#endif /* SORTING */

	s_entry->isorec.flags[0] = ISO_FILE;
	s_entry->priority = 32768;
	iso9660_date(s_entry->isorec.date, fstatbuf.st_mtime);
	s_entry->inode = TABLE_INODE;
	s_entry->dev = (dev_t)UNCACHED_DEVICE;
	set_723(s_entry->isorec.volume_sequence_number,
						volume_sequence_number);
	set_733((char *)s_entry->isorec.size, SECTOR_SIZE);
	s_entry->size = SECTOR_SIZE;
	s_entry->filedir = this_dir;
	s_entry->name = e_strdup(p2);
	iso9660_file_length(p2, s_entry, 0);

	/* flag file as necessary */
	s_entry->de_flags = bcat_de_flags;

	if ((use_XA || use_RockRidge) &&
	    !(bcat_de_flags & INHIBIT_ISO9660_ENTRY)) {
		fstatbuf.st_mode = 0444 | S_IFREG;
		fstatbuf.st_nlink = 1;
		generate_xa_rr_attributes("",
			p2, s_entry,
			&fstatbuf, &fstatbuf, 0);
	}
	/*
	 *  memory files are stored at s_entry->table
	 * - but this is also used for each s_entry to generate
	 * TRANS.TBL entries. So if we are generating tables,
	 * store the TRANS.TBL data here for the moment
	 */
	if (generate_tables && !(bcat_de_flags & INHIBIT_ISO9660_ENTRY)) {
		sprintf(buffer, "F\t%s\n", s_entry->name);

		/* copy the TRANS.TBL entry info and clear the buffer */
		s_entry->table = e_strdup(buffer);
		memset(buffer, 0, SECTOR_SIZE);

		/*
		 * store the (empty) file data in the
		 * unused s_entry->whole_name element for the time being
		 * - this will be transferred to s_entry->table after any
		 * TRANS.TBL processing later
		 */
		s_entry->whole_name = buffer;
	} else {
		/* store the (empty) file data in the s_entry->table element */
		s_entry->table = buffer;
		s_entry->whole_name = NULL;
	}
}

LOCAL void
get_torito_desc(boot_desc)
	struct eltorito_boot_descriptor	*boot_desc;
{
	int			checksum;
	unsigned char		*checksum_ptr;
	struct directory_entry	*de2;	/* Boot catalog */
	int			i;
	int			offset;
	int			arch = 0;
	int			nentries = 0;
	struct eltorito_defaultboot_entry boot_desc_record;
	struct eltorito_sectionheader_entry boot_shdr_record;
#ifdef	__needed__
	struct eltorito_section_entry boot_section_record;
#endif
	struct eltorito_sectionheader_entry *last_section_header = 0;

	memset(boot_desc, 0, sizeof (*boot_desc));
	boot_desc->type[0] = 0;
	strncpy(boot_desc->id, ISO_STANDARD_ID, sizeof (boot_desc->id));
	boot_desc->version[0] = 1;

	memcpy(boot_desc->system_id, EL_TORITO_ID, sizeof (EL_TORITO_ID));

	/*
	 * search from root of iso fs to find boot catalog
	 * - we already know where the boot catalog is
	 * - we created it above - but lets search for it anyway
	 * - good sanity check!
	 */
	de2 = search_tree_file(root, boot_catalog);
	if (!de2 || !(de2->de_flags & MEMORY_FILE)) {
		ex_boot_enoent(_("catalog"), boot_catalog);
		/* NOTREACHED */
	}
	set_731(boot_desc->bootcat_ptr,
		(unsigned int) get_733(de2->isorec.extent));

	/*
	 * If the platform id for the first (default) boot entry has not been
	 * explicitly set, we default to EL_TORITO_ARCH_x86
	 */
	if ((first_boot_entry->type & ELTORITO_BOOT_ID) == 0) {
		first_boot_entry->boot_platform = EL_TORITO_ARCH_x86;
	}
	arch = first_boot_entry->boot_platform;

	/*
	 * we have the boot image, so write boot catalog information
	 * Next we write out the primary descriptor for the disc
	 */
	memset(&valid_desc, 0, sizeof (valid_desc));
	valid_desc.headerid[0] = 1;
	valid_desc.arch[0] = arch;	/* Platform id for the default boot */

	/*
	 * we'll shove start of publisher id into id field,
	 * may get truncated but who really reads this stuff!
	 */
	if (publisher)
		memcpy_max(valid_desc.id, publisher,
						MIN(23, strlen(publisher)));

	valid_desc.key1[0] = (char)0x55;
	valid_desc.key2[0] = (char)0xAA;

	/* compute the checksum */
	checksum = 0;
	checksum_ptr = (unsigned char *) &valid_desc;
	/* Set checksum to 0 before computing checksum */
	set_721(valid_desc.cksum, 0);
	for (i = 0; i < (int)sizeof (valid_desc); i += 2) {
		checksum += (unsigned int) checksum_ptr[i];
		checksum += ((unsigned int) checksum_ptr[i + 1]) * 256;
	}

	/* now find out the real checksum */
	checksum = -checksum;
	set_721(valid_desc.cksum, (unsigned int) checksum);

	/* now write it to the virtual boot catalog */
	memcpy(de2->table, &valid_desc, 32);

	for (current_boot_entry = first_boot_entry, offset = sizeof (valid_desc);
		current_boot_entry != NULL;
		current_boot_entry = current_boot_entry->next,
		offset += sizeof (boot_desc_record)) {
		int	newarch = arch;

		if (current_boot_entry->type & ELTORITO_BOOT_ID)
			newarch = current_boot_entry->boot_platform;
		else
			current_boot_entry->boot_platform = arch;

		/*
		 * El Torito has no such limitation but we currently have...
		 */
		if (offset >= (SECTOR_SIZE - sizeof (boot_desc_record))) {
			comerrno(EX_BAD,
			_("Too many El Torito boot entries\n"));
		}
		if (current_boot_entry == first_boot_entry) {
			;
			/* EMPTY */
		} else if ((current_boot_entry == first_boot_entry->next) ||
			    (arch != newarch) ||
			    (current_boot_entry->type & ELTORITO_SECTION_HEADER)) {
			if (last_section_header)
				set_721(&last_section_header->entry_count, nentries);
			nentries = 1;
			last_section_header = (struct eltorito_sectionheader_entry *)
							(de2->table + offset);
			fill_boot_shdr(&boot_shdr_record, newarch);
			memcpy(de2->table + offset, &boot_shdr_record,
						sizeof (boot_shdr_record));
			offset += sizeof (boot_desc_record);
		} else {
			nentries++;	/* Add entry to this section header */
		}
		/*
		 * This works because a section entry has the same essential
		 * layout as a default entry (and we do not populate the
		 * selection criteria fields).
		 */
		fill_boot_desc(&boot_desc_record, current_boot_entry);
		memcpy(de2->table + offset, &boot_desc_record,
					sizeof (boot_desc_record));
	}

	if (last_section_header) {
		set_721(&last_section_header->entry_count, nentries);
		last_section_header->header_id[0] = EL_TORITO_SHDR_ID_LAST_SHDR;
	}

} /* get_torito_desc(... */

LOCAL void
fill_boot_shdr(boot_shdr_entry, arch)
	struct eltorito_sectionheader_entry	*boot_shdr_entry;
	int					arch;
{
	memset(boot_shdr_entry, 0, sizeof (struct eltorito_sectionheader_entry));
	boot_shdr_entry->header_id[0] = EL_TORITO_SHDR_ID_SHDR;
	boot_shdr_entry->platform_id[0] = arch;
}

LOCAL void
fill_boot_desc(boot_desc_entry, boot_entry)
	struct eltorito_defaultboot_entry *boot_desc_entry;
	struct eltorito_boot_entry_info *boot_entry;
{
	struct directory_entry	*de;	/* Boot file */
	int			bootmbr;
	int			i;
	int			nsectors;
	int			geosec;

	if (!boot_desc_entry || !boot_entry)
		return;

	/* now adjust boot catalog lets find boot image first */
	de = search_tree_file(root, boot_entry->boot_image);
	if (!de) {
		ex_boot_enoent(_("image"), boot_entry->boot_image);
		/* NOTREACHED */
	}
	/* now make the initial/default entry for boot catalog */
	memset(boot_desc_entry, 0, sizeof (*boot_desc_entry));
	boot_desc_entry->boot_id[0] = (char)boot_entry->not_bootable ?
				EL_TORITO_NOT_BOOTABLE : EL_TORITO_BOOTABLE;

	/* use default BIOS loadpnt */
	set_721(boot_desc_entry->loadseg, boot_entry->load_addr);

	/*
	 * figure out size of boot image in 512-byte sectors.
	 * However, round up to the nearest integral CD (2048-byte) sector.
	 * This is only used for no-emulation booting.
	 */
	nsectors = boot_entry->load_size ? boot_entry->load_size :
				ISO_BLOCKS(de->size) * (SECTOR_SIZE/512);

	if (verbose > 0) {
		fprintf(stderr,
			_("Size of boot image is %d sectors -> "), nsectors);
	}

	if (boot_entry->hard_disk_boot) {
		/* sanity test hard disk boot image */
		boot_desc_entry->boot_media[0] = EL_TORITO_MEDIA_HD;
		if (verbose > 0)
			fprintf(stderr, _("Emulating a hard disk\n"));

		/* read MBR */
		bootmbr = open(de->whole_name, O_RDONLY | O_BINARY);
		if (bootmbr == -1) {
			comerr(_("Error opening boot image '%s' for read.\n"),
							de->whole_name);
		}
		if (read(bootmbr, &disk_mbr, sizeof (disk_mbr)) !=
							sizeof (disk_mbr)) {
			comerr(_("Error reading MBR from boot image '%s'.\n"),
							de->whole_name);
		}
		close(bootmbr);
		if (la_to_u_2_byte(disk_mbr.magic) != MBR_MAGIC) {
			errmsgno(EX_BAD,
			_("Warning: boot image '%s' MBR is not a boot sector.\n"),
							de->whole_name);
		}
		/* find partition type */
		boot_desc_entry->sys_type[0] = PARTITION_UNUSED;
		for (i = 0; i < PARTITION_COUNT; ++i) {
			int		s_cyl_sec;
			int		e_cyl_sec;

			s_cyl_sec =
			la_to_u_2_byte(disk_mbr.partition[i].s_cyl_sec);
			e_cyl_sec =
			la_to_u_2_byte(disk_mbr.partition[i].e_cyl_sec);

			if (disk_mbr.partition[i].type != PARTITION_UNUSED) {
				if (boot_desc_entry->sys_type[0] !=
							PARTITION_UNUSED) {
					comerrno(EX_BAD,
					_("Boot image '%s' has multiple partitions.\n"),
							de->whole_name);
				}
				boot_desc_entry->sys_type[0] =
						disk_mbr.partition[i].type;

				/* a few simple sanity warnings */
				if (!boot_entry->not_bootable &&
				    disk_mbr.partition[i].status !=
							PARTITION_ACTIVE) {
					fprintf(stderr,
					_("Warning: partition not marked active.\n"));
				}
				if (MBR_CYLINDER(s_cyl_sec) != 0 ||
					disk_mbr.partition[i].s_head != 1 ||
					MBR_SECTOR(s_cyl_sec != 1)) {
					fprintf(stderr,
					_("Warning: partition does not start at 0/1/1.\n"));
				}
				geosec = (MBR_CYLINDER(e_cyl_sec) + 1) *
					(disk_mbr.partition[i].e_head + 1) *
					MBR_SECTOR(e_cyl_sec);
				if (geosec != nsectors) {
					fprintf(stderr,
					_("Warning: image size does not match geometry (%d)\n"),
						geosec);
				}
#ifdef DEBUG_TORITO
				fprintf(stderr, "Partition start %u/%u/%u\n",
					MBR_CYLINDER(s_cyl_sec),
					disk_mbr.partition[i].s_head,
					MBR_SECTOR(s_cyl_sec));
				fprintf(stderr, "Partition end %u/%u/%u\n",
					MBR_CYLINDER(e_cyl_sec),
					disk_mbr.partition[i].e_head,
					MBR_SECTOR(e_cyl_sec));
#endif
			}
		}
		if (boot_desc_entry->sys_type[0] == PARTITION_UNUSED) {
			comerrno(EX_BAD,
					_("Boot image '%s' has no partitions.\n"),
							de->whole_name);
		}
#ifdef DEBUG_TORITO
		fprintf(stderr, "Partition type %u\n",
						boot_desc_entry->sys_type[0]);
#endif
	/* load single boot sector, in this case the MBR */
		nsectors = 1;

	} else if (boot_entry->no_emul_boot) {
		/*
		 * no emulation is a simple image boot of all the sectors
		 * in the boot image
		 */
		boot_desc_entry->boot_media[0] = EL_TORITO_MEDIA_NOEMUL;
		if (verbose > 0)
			fprintf(stderr, _("No emulation\n"));

	} else {
		/* choose size of emulated floppy based on boot image size */
		if (nsectors == 2880) {
			boot_desc_entry->boot_media[0] = EL_TORITO_MEDIA_144FLOP;
			if (verbose > 0)
				fprintf(stderr, _("Emulating a 1440 kB floppy\n"));

		} else if (nsectors == 5760) {
			boot_desc_entry->boot_media[0] = EL_TORITO_MEDIA_288FLOP;
			if (verbose > 0)
				fprintf(stderr, _("Emulating a 2880 kB floppy\n"));

		} else if (nsectors == 2400) {
			boot_desc_entry->boot_media[0] = EL_TORITO_MEDIA_12FLOP;
			if (verbose > 0)
				fprintf(stderr, _("Emulating a 1200 kB floppy\n"));

		} else {
			comerrno(EX_BAD,
			_("Error - boot image '%s' has not an allowable size.\n"),
							de->whole_name);
		}

		/* load single boot sector for floppies */
		nsectors = 1;
	}

	/* fill in boot image details */
#ifdef DEBUG_TORITO
	fprintf(stderr, "Boot %u sectors\n", nsectors);
	fprintf(stderr, "Extent of boot images is %d\n",
				get_733(de->isorec.extent));
#endif
	set_721(boot_desc_entry->nsect, (unsigned int) nsectors);
	set_731(boot_desc_entry->bootoff,
		(unsigned int) get_733(de->isorec.extent));


	/* If the user has asked for it, patch the boot image */
	if (boot_entry->boot_info_table) {
		int		bootimage;
		unsigned int	bi_checksum;
		unsigned int	total_len;
		static char	csum_buffer[SECTOR_SIZE];
		int		len;
		struct mkisofs_boot_info bi_table;

		bootimage = open(de->whole_name, O_RDWR | O_BINARY);
		if (bootimage == -1) {
			comerr(
			_("Error opening boot image file '%s' for update.\n"),
							de->whole_name);
		}
	/* Compute checksum of boot image, sans 64 bytes */
		total_len = 0;
		bi_checksum = 0;
		while ((len = read(bootimage, csum_buffer, SECTOR_SIZE)) > 0) {
			if (total_len & 3) {
				comerrno(EX_BAD,
				_("Odd alignment at non-end-of-file in boot image '%s'.\n"),
							de->whole_name);
			}
			if (total_len < 64)
				memset(csum_buffer, 0, 64 - total_len);
			if (len < SECTOR_SIZE)
				memset(csum_buffer + len, 0, SECTOR_SIZE-len);
			for (i = 0; i < SECTOR_SIZE; i += 4)
				bi_checksum += get_731(&csum_buffer[i]);
			total_len += len;
		}

		if (total_len != de->size) {
			comerrno(EX_BAD,
			_("Boot image file '%s' changed underneath us!\n"),
						de->whole_name);
		}
		/* End of file, set position to byte 8 */
		lseek(bootimage, (off_t)8, SEEK_SET);
		memset(&bi_table, 0, sizeof (bi_table));
		/* Is it always safe to assume PVD is at session_start+16? */
		set_731(bi_table.bi_pvd, session_start + 16);
		set_731(bi_table.bi_file, de->starting_block);
		set_731(bi_table.bi_length, de->size);
		set_731(bi_table.bi_csum, bi_checksum);

		write(bootimage, &bi_table, sizeof (bi_table));
		close(bootimage);
	}
} /* fill_boot_desc(... */

EXPORT void
get_boot_entry()
{
	if (current_boot_entry)
		return;

	current_boot_entry = (struct eltorito_boot_entry_info *)
			e_malloc(sizeof (struct eltorito_boot_entry_info));
	memset(current_boot_entry, 0, sizeof (*current_boot_entry));

	if (!first_boot_entry) {
		first_boot_entry = current_boot_entry;
		last_boot_entry = current_boot_entry;
		current_boot_entry->boot_platform = EL_TORITO_ARCH_x86;
	} else {
		current_boot_entry->boot_platform = last_boot_entry->boot_platform;
		last_boot_entry->next = current_boot_entry;
		last_boot_entry = current_boot_entry;
	}
}

EXPORT int
new_boot_entry()
{
	current_boot_entry = NULL;
	return (1);
}

/*
 * Exit with a boot no entry message.
 */
EXPORT void
ex_boot_enoent(msg, pname)
	char	*msg;
	char	*pname;
{
	comerrno(EX_BAD, _("Uh oh, I cant find the boot %s '%s' inside the target tree.\n"), msg, pname);
	/* NOTREACHED */
}

/*
 * Function to write the EVD for the disc.
 */
LOCAL int
tvd_write(outfile)
	FILE	*outfile;
{
	/* check the boot image is not NULL */
	if (!boot_image) {
		comerrno(EX_BAD, _("No boot image specified.\n"));
	}
	/* Next we write out the boot volume descriptor for the disc */
	get_torito_desc(&gboot_desc);
	xfwrite(&gboot_desc, SECTOR_SIZE, 1, outfile, 0, FALSE);
	last_extent_written++;
	return (0);
}

struct output_fragment torito_desc = {NULL, oneblock_size, NULL, tvd_write, "Eltorito Volume Descriptor"};
