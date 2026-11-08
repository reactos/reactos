/* $NetBSD: udf_write.c,v 1.11 2021/09/01 14:17:46 andvar Exp $ */

/*
 * Copyright (c) 2006, 2008, 2013 Reinoud Zandijk
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * 
 */
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include "../compat.h"
__RCSID("$NetBSD: udf_write.c,v 1.11 2021/09/01 14:17:46 andvar Exp $");


#if !HAVE_NBTOOL_CONFIG_H
#define _EXPOSE_MMC
//#include <sys/cdio.h>
#else
#include "udf/cdio_mmc_structs.h"
#endif

#include "udf_create.h"
#include "udf_write.h"
#include "newfs_udf.h"


union dscrptr *terminator_dscr;

static int
udf_write_phys(void *blob, uint32_t location, uint32_t sects)
{
	uint32_t phys, cnt;
	uint8_t *bpos;
	int error;

	for (cnt = 0; cnt < sects; cnt++) {
		bpos  = (uint8_t *) blob;
		bpos += context.sector_size * cnt;

		phys = location + cnt;
		error = udf_write_sector(bpos, phys);
		if (error)
			return error;
	}
	return 0;
}


static int
udf_write_dscr_phys(union dscrptr *dscr, uint32_t location,
	uint32_t sects)
{
	dscr->tag.tag_loc = udf_rw32(location);
	(void) udf_validate_tag_and_crc_sums(dscr);

	return udf_write_phys(dscr, location, sects);
}


int
udf_write_dscr_virt(union dscrptr *dscr, uint32_t location, uint32_t vpart,
	uint32_t sects)
{
	struct file_entry *fe;
	struct extfile_entry *efe;
	struct extattrhdr_desc *extattrhdr;
	uint32_t phys, mirror;
	int error;

	extattrhdr = NULL;
	if (udf_rw16(dscr->tag.id) == TAGID_FENTRY) {
		fe = (struct file_entry *) dscr;
		if (udf_rw32(fe->l_ea) > 0)
			extattrhdr = (struct extattrhdr_desc *) fe->data;
	}
	if (udf_rw16(dscr->tag.id) == TAGID_EXTFENTRY) {
		efe = (struct extfile_entry *) dscr;
		if (udf_rw32(efe->l_ea) > 0)
			extattrhdr = (struct extattrhdr_desc *) efe->data;
	}
	if (extattrhdr) {
		extattrhdr->tag.tag_loc = udf_rw32(location);
		udf_validate_tag_and_crc_sums((union dscrptr *) extattrhdr);
	}

	dscr->tag.tag_loc = udf_rw32(location);
	udf_validate_tag_and_crc_sums(dscr);

	/* determine physical location */
	phys = context.vtop_offset[vpart];
	if (context.vtop_tp[vpart] == UDF_VTOP_TYPE_VIRT) {
		udf_vat_update(location, context.data_alloc_pos);
		phys += context.data_alloc_pos++;
	} else {
		phys += location;
	}

	error = udf_write_phys(dscr, phys, sects);
	if (error)
		return error;
	if ((format_flags & FORMAT_META) && vpart == context.metadata_part) {
		mirror = context.vtop_offset[context.data_part] +
			layout.meta_mirror_start_lba + location;
		return udf_write_phys(dscr, mirror, sects);
	}
	return 0;
}


void
udf_metadata_alloc(int nblk, struct long_ad *pos)
{
	memset(pos, 0, sizeof(*pos));
	pos->len	  = udf_rw32(nblk * context.sector_size);
	pos->loc.lb_num   = udf_rw32(context.metadata_alloc_pos);
	pos->loc.part_num = udf_rw16(context.metadata_part);

	udf_mark_allocated(context.metadata_alloc_pos, context.metadata_part,
		nblk);

	context.metadata_alloc_pos += nblk;
	if (context.metadata_part == context.data_part)
		context.data_alloc_pos = context.metadata_alloc_pos;
}


void
udf_data_alloc(int nblk, struct long_ad *pos)
{
	memset(pos, 0, sizeof(*pos));
	pos->len	  = udf_rw32(nblk * context.sector_size);
	pos->loc.lb_num   = udf_rw32(context.data_alloc_pos);
	pos->loc.part_num = udf_rw16(context.data_part);

	udf_mark_allocated(context.data_alloc_pos, context.data_part, nblk);
	context.data_alloc_pos += nblk;
	if (context.metadata_part == context.data_part)
		context.metadata_alloc_pos = context.data_alloc_pos;
}



/* --------------------------------------------------------------------- */

/*
 * udf_derive_format derives the format_flags from the disc's mmc_discinfo.
 * The resulting flags uniquely define a disc format. Note there are at least
 * 7 distinct format types defined in UDF.
 */

#define UDF_VERSION(a) \
	(((a) == 0x100) || ((a) == 0x102) || ((a) == 0x150) || ((a) == 0x200) || \
	 ((a) == 0x201) || ((a) == 0x250) || ((a) == 0x260))

int
udf_derive_format(int req_enable, int req_disable, int force)
{
	/* disc writability, formatted, appendable */
	if ((mmc_discinfo.mmc_cur & MMC_CAP_RECORDABLE) == 0) {
		(void)printf("Can't newfs readonly device\n");
		return EROFS;
	}
	if (mmc_discinfo.mmc_cur & MMC_CAP_SEQUENTIAL) {
		/* sequentials need sessions appended */
		if (mmc_discinfo.disc_state == MMC_STATE_CLOSED) {
			(void)printf("Can't append session to a closed disc\n");
			return EROFS;
		}
		if ((mmc_discinfo.disc_state != MMC_STATE_EMPTY) && !force) {
			(void)printf("Disc not empty! Use -F to force "
			    "initialisation\n");
			return EROFS;
		}
	} else {
		/* check if disc (being) formatted or has been started on */
		if (mmc_discinfo.disc_state == MMC_STATE_EMPTY) {
			(void)printf("Disc is not formatted\n");
			return EROFS;
		}
	}

	/* determine UDF format */
	format_flags = 0;
	if (mmc_discinfo.mmc_cur & MMC_CAP_REWRITABLE) {
		/* all rewritable media */
		format_flags |= FORMAT_REWRITABLE;
		if (context.min_udf >= 0x0250) {
			/* standard dictates meta as default */
			format_flags |= FORMAT_META;
		}

		if ((mmc_discinfo.mmc_cur & MMC_CAP_HW_DEFECTFREE) == 0) {
			/* sparables for defect management */
			if (context.min_udf >= 0x150)
				format_flags |= FORMAT_SPARABLE;
		}
	} else {
		/* all once recordable media */
		format_flags |= FORMAT_WRITEONCE;
		if (mmc_discinfo.mmc_cur & MMC_CAP_SEQUENTIAL) {
			format_flags |= FORMAT_SEQUENTIAL;

			if (mmc_discinfo.mmc_cur & MMC_CAP_PSEUDOOVERWRITE) {
				/* logical overwritable */
				format_flags |= FORMAT_LOW;
			} else {
				/* have to use VAT for overwriting */
				format_flags |= FORMAT_VAT;
			}
		} else {
			/* rare WORM devices, but BluRay has one, strat4096 */
			format_flags |= FORMAT_WORM;
		}
	}

	/* enable/disable requests */
	if (req_disable & FORMAT_META) {
		format_flags &= ~(FORMAT_META | FORMAT_LOW);
		req_disable  &= ~FORMAT_META;
	}
	if ((format_flags & FORMAT_VAT) & UDF_512_TRACK)
		format_flags |= FORMAT_TRACK512;

	if (req_enable & FORMAT_READONLY) {
		format_flags |= FORMAT_READONLY;
	}

	/* determine partition/media access type */
	media_accesstype = UDF_ACCESSTYPE_NOT_SPECIFIED;
	if (mmc_discinfo.mmc_cur & MMC_CAP_REWRITABLE) {
		media_accesstype = UDF_ACCESSTYPE_OVERWRITABLE;
		if (mmc_discinfo.mmc_cur & MMC_CAP_ERASABLE)
			media_accesstype = UDF_ACCESSTYPE_REWRITEABLE;
	} else {
		/* all once recordable media */
		media_accesstype = UDF_ACCESSTYPE_WRITE_ONCE;
	}
	if (mmc_discinfo.mmc_cur & MMC_CAP_PSEUDOOVERWRITE)
		media_accesstype = UDF_ACCESSTYPE_PSEUDO_OVERWITE;

	/* patch up media accesstype */
	if (req_enable & FORMAT_READONLY) {
		/* better now */
		media_accesstype = UDF_ACCESSTYPE_READ_ONLY;
	}

	/* adjust minimum version limits */
	if (format_flags & FORMAT_VAT)
		context.min_udf = MAX(context.min_udf, 0x0150);
	if (format_flags & FORMAT_SPARABLE)
		context.min_udf = MAX(context.min_udf, 0x0150);
	if (format_flags & FORMAT_META)
		context.min_udf = MAX(context.min_udf, 0x0250);
	if (format_flags & FORMAT_LOW)
		context.min_udf = MAX(context.min_udf, 0x0260);

	/* adjust maximum version limits not to tease or break things */
	if (!(format_flags & (FORMAT_META | FORMAT_LOW)) &&
	    (context.max_udf > 0x200))
		context.max_udf = 0x201;

	if ((format_flags & (FORMAT_VAT | FORMAT_SPARABLE)) == 0)
		if (context.max_udf <= 0x150)
			context.min_udf = 0x102;

	/* limit Ecma 167 descriptor if possible/needed */
	context.dscrver = 3;
	if ((context.min_udf < 0x200) || (context.max_udf < 0x200)) {
		context.dscrver = 2;
		context.max_udf = 0x150;	/* last version < 0x200 */
	}

	/* is it possible ? */
	if (context.min_udf > context.max_udf) {
		(void)printf("Initialisation prohibited by specified maximum "
		    "UDF version 0x%04x. Minimum version required 0x%04x\n",
		    context.max_udf, context.min_udf);
		return EPERM;
	}

	if (!UDF_VERSION(context.min_udf) || !UDF_VERSION(context.max_udf)) {
		printf("Choose UDF version numbers from "
			"0x102, 0x150, 0x200, 0x201, 0x250 and 0x260\n");
		printf("Default version is 0x201\n");
		return EPERM;
	}

	return 0;
}

#undef UDF_VERSION


/* --------------------------------------------------------------------- */

int
udf_proces_names(void)
{
	struct timeval time_of_day;
	uint32_t primary_nr;
	uint64_t volset_nr;
	if (context.logvol_name == NULL)
		context.logvol_name = strdup("anonymous");
	if (context.primary_name == NULL) {
		if (mmc_discinfo.disc_flags & MMC_DFLAGS_DISCIDVALID) {
			primary_nr = mmc_discinfo.disc_id;
		} else {
			primary_nr = (uint32_t) random();
		}
		context.primary_name = calloc(32, 1);
		sprintf(context.primary_name, "%08"PRIx32, primary_nr);
	}
	if (context.volset_name == NULL) {
		if (mmc_discinfo.disc_flags & MMC_DFLAGS_BARCODEVALID) {
			volset_nr = mmc_discinfo.disc_barcode;
		} else {
			(void)gettimeofday(&time_of_day, NULL);
			volset_nr  =  (uint64_t) random();
			volset_nr |= ((uint64_t) time_of_day.tv_sec) << 32;
		}
		context.volset_name = calloc(128,1);
		sprintf(context.volset_name, "%16"PRIx64, volset_nr);
	}
	if (context.fileset_name == NULL)
		context.fileset_name = strdup("anonymous");

	/* check passed/created identifiers */
	if (strlen(context.logvol_name)  > 128) {
		(void)printf("Logical volume name too long\n");
		return EINVAL;
	}
	if (strlen(context.primary_name) >  32) {
		(void)printf("Primary volume name too long\n");
		return EINVAL;
	}
	if (strlen(context.volset_name)  > 128) {
		(void)printf("Volume set name too long\n");
		return EINVAL;
	}
	if (strlen(context.fileset_name) > 32) {
		(void)printf("Fileset name too long\n");
		return EINVAL;
	}

	/* signal all OK */
	return 0;
}

/* --------------------------------------------------------------------- */

static int
udf_write_iso9660_vrs(void)
{
	struct vrs_desc *iso9660_vrs_desc;
	uint32_t pos;
	int error, cnt, dpos;

	/* create ISO/Ecma-167 identification descriptors */
	if ((iso9660_vrs_desc = calloc(1, context.sector_size)) == NULL)
		return ENOMEM;

	/*
	 * All UDF formats should have their ISO/Ecma-167 descriptors written
	 * except when not possible due to track reservation in the case of
	 * VAT
	 */
	if ((format_flags & FORMAT_TRACK512) == 0) {
		dpos = (2048 + context.sector_size - 1) / context.sector_size;

		/* wipe at least 6 times 2048 byte `sectors' */
		for (cnt = 0; cnt < 6 *dpos; cnt++) {
			pos = layout.iso9660_vrs + cnt;
			if ((error = udf_write_sector(iso9660_vrs_desc, pos))) {
				free(iso9660_vrs_desc);
				return error;
			}
		}

		/* common VRS fields in all written out ISO descriptors */
		iso9660_vrs_desc->struct_type = 0;
		iso9660_vrs_desc->version     = 1;
		pos = layout.iso9660_vrs;

		/* BEA01, NSR[23], TEA01 */
		memcpy(iso9660_vrs_desc->identifier, "BEA01", 5);
		if ((error = udf_write_sector(iso9660_vrs_desc, pos))) {
			free(iso9660_vrs_desc);
			return error;
		}
		pos += dpos;

		if (context.dscrver == 2)
			memcpy(iso9660_vrs_desc->identifier, "NSR02", 5);
		else
			memcpy(iso9660_vrs_desc->identifier, "NSR03", 5);
		;
		if ((error = udf_write_sector(iso9660_vrs_desc, pos))) {
			free(iso9660_vrs_desc);
			return error;
		}
		pos += dpos;

		memcpy(iso9660_vrs_desc->identifier, "TEA01", 5);
		if ((error = udf_write_sector(iso9660_vrs_desc, pos))) {
			free(iso9660_vrs_desc);
			return error;
		}
	}

	free(iso9660_vrs_desc);
	/* return success */
	return 0;
}


/* --------------------------------------------------------------------- */

/*
 * Main function that creates and writes out disc contents based on the
 * format_flags's that uniquely define the type of disc to create.
 */

int
udf_do_newfs_prefix(void)
{
	union dscrptr *zero_dscr;
	union dscrptr *dscr;
	struct mmc_trackinfo ti;
	uint32_t sparable_blocks;
	uint32_t sector_size, blockingnr;
	uint32_t cnt, loc, len;
	int sectcopy;
	int error, integrity_type;
	int data_part, metadata_part;

	/* init */
	sector_size = mmc_discinfo.sector_size;

	/* determine span/size */
	ti.tracknr = mmc_discinfo.first_track_last_session;
	error = udf_update_trackinfo(&mmc_discinfo, &ti);
	if (error)
		return error;

	if (mmc_discinfo.sector_size < context.sector_size) {
		fprintf(stderr, "Impossible to format: sectorsize too small\n");
		return EIO;
	}
	context.sector_size = sector_size;

	/* determine blockingnr */
	blockingnr = ti.packet_size;
	if (blockingnr <= 1) {
		/* paranoia on blockingnr */
		switch (mmc_discinfo.mmc_profile) {
		case 0x08 : /* CDROM */
		case 0x09 : /* CD-R    */
		case 0x0a : /* CD-RW   */
			blockingnr = 32;	/* UDF requirement */
			break;
		case 0x10 : /* DVDROM */
		case 0x11 : /* DVD-R (DL) */
		case 0x12 : /* DVD-RAM */
		case 0x1b : /* DVD+R      */
		case 0x2b : /* DVD+R Dual layer */
		case 0x13 : /* DVD-RW restricted overwrite */
		case 0x14 : /* DVD-RW sequential */
			blockingnr = 16;	/* SCSI definition */
			break;
		case 0x40 : /* BDROM */
		case 0x41 : /* BD-R Sequential recording (SRM) */
		case 0x42 : /* BD-R Random recording (RRM) */
		case 0x43 : /* BD-RE */
		case 0x51 : /* HD DVD-R   */
		case 0x52 : /* HD DVD-RW  */
			blockingnr = 32;	/* SCSI definition */
			break;
		default:
			break;
		}
	}
	if (blockingnr <= 0) {
		printf("Can't fixup blockingnumber for device "
			"type %d\n", mmc_discinfo.mmc_profile);

		printf("Device is not returning valid blocking"
			" number and media type is unknown.\n");

		return EINVAL;
	}
	wrtrack_skew = ti.track_start % blockingnr;
	if (udf_physical_alignment > 1)
		wrtrack_skew = (udf_physical_alignment -
			udf_physical_offset) % udf_physical_alignment;

	if (mmc_discinfo.mmc_class == MMC_CLASS_CD) {
		/* not too much for CD-RW, still 20MiB */
		sparable_blocks = 32;
	} else {
		/* take a value for DVD*RW mainly, BD is `defect free' */
		sparable_blocks = 512;
	}

	/* get layout */
	error = udf_calculate_disc_layout(format_flags, context.min_udf,
		wrtrack_skew,
		ti.track_start, mmc_discinfo.last_possible_lba,
		context.sector_size, blockingnr, sparable_blocks,
		meta_fract);
	if (error)
		return error;

	/* cache partition for we need it often */
	data_part     = context.data_part;
	metadata_part = context.metadata_part;

	/* Create sparing table descriptor if applicable */
	if (format_flags & FORMAT_SPARABLE) {
		if ((error = udf_create_sparing_tabled()))
			return error;

		if (check_surface) {
			if ((error = udf_surface_check()))
				return error;
		}
	}

	/* Create a generic terminator descriptor (later reused) */
	terminator_dscr = calloc(1, sector_size);
	if (terminator_dscr == NULL)
		return ENOMEM;
	udf_create_terminator(terminator_dscr, 0);

	/*
	 * Start with wipeout of VRS1 upto start of partition. This allows
	 * formatting for sequentials with the track reservation and it 
	 * cleans old rubbish on rewritables. For sequentials without the
	 * track reservation all is wiped from track start.
	 */
	if ((zero_dscr = calloc(1, context.sector_size)) == NULL)
		return ENOMEM;

	loc = (format_flags & FORMAT_TRACK512) ? layout.vds1 : ti.track_start;
	for (; loc < layout.part_start_lba; loc++) {
		if ((error = udf_write_sector(zero_dscr, loc))) {
			free(zero_dscr);
			return error;
		}
	}
	free(zero_dscr);

	/* Create anchors */
	for (cnt = 0; cnt < 3; cnt++) {
		if ((error = udf_create_anchor(cnt))) {
			return error;
		}
	}

	/* 
	 * Create the two Volume Descriptor Sets (VDS) each containing the
	 * following descriptors : primary volume, partition space,
	 * unallocated space, logical volume, implementation use and the
	 * terminator
	 */

	/* start of volume recognition sequence building */
	context.vds_seq = 0;

	/* Create primary volume descriptor */
	if ((error = udf_create_primaryd()))
		return error;

	/* Create partition descriptor */
	if ((error = udf_create_partitiond(context.data_part, media_accesstype)))
		return error;

	/* Create unallocated space descriptor */
	if ((error = udf_create_unalloc_spaced()))
		return error;

	/* Create logical volume descriptor */
	if ((error = udf_create_logical_dscr(format_flags)))
		return error;

	/* Create implementation use descriptor */
	/* TODO input of fields 1,2,3 and passing them */
	if ((error = udf_create_impvold(NULL, NULL, NULL)))
		return error;

	/* write out what we've created so far */

	/* writeout iso9660 vrs */
	if ((error = udf_write_iso9660_vrs()))
		return error;

	/* Writeout anchors */
	for (cnt = 0; cnt < 3; cnt++) {
		dscr = (union dscrptr *) context.anchors[cnt];
		loc  = layout.anchors[cnt];
		if ((error = udf_write_dscr_phys(dscr, loc, 1)))
			return error;

		/* sequential media has only one anchor */
		if (format_flags & FORMAT_SEQUENTIAL)
			break;
	}

	/* write out main and secondary VRS */
	for (sectcopy = 1; sectcopy <= 2; sectcopy++) {
		loc = (sectcopy == 1) ? layout.vds1 : layout.vds2;

		/* primary volume descriptor */
		dscr = (union dscrptr *) context.primary_vol;
		error = udf_write_dscr_phys(dscr, loc, 1);
		if (error)
			return error;
		loc++;

		/* partition descriptor(s) */
		for (cnt = 0; cnt < UDF_PARTITIONS; cnt++) {
			dscr = (union dscrptr *) context.partitions[cnt];
			if (dscr) {
				error = udf_write_dscr_phys(dscr, loc, 1);
				if (error)
					return error;
				loc++;
			}
		}

		/* unallocated space descriptor */
		dscr = (union dscrptr *) context.unallocated;
		error = udf_write_dscr_phys(dscr, loc, 1);
		if (error)
			return error;
		loc++;

		/* logical volume descriptor */
		dscr = (union dscrptr *) context.logical_vol;
		error = udf_write_dscr_phys(dscr, loc, 1);
		if (error)
			return error;
		loc++;

		/* implementation use descriptor */
		dscr = (union dscrptr *) context.implementation;
		error = udf_write_dscr_phys(dscr, loc, 1);
		if (error)
			return error;
		loc++;

		/* terminator descriptor */
		error = udf_write_dscr_phys(terminator_dscr, loc, 1);
		if (error)
			return error;
		loc++;
	}

	/* writeout the two sparable table descriptors (if needed) */
	if (format_flags & FORMAT_SPARABLE) {
		for (sectcopy = 1; sectcopy <= 2; sectcopy++) {
			loc  = (sectcopy == 1) ? layout.spt_1 : layout.spt_2;
			dscr = (union dscrptr *) context.sparing_table;
			len  = layout.sparing_table_dscr_lbas;

			/* writeout */
			error = udf_write_dscr_phys(dscr, loc, len);
			if (error)
				return error;
		}
	}

	/*
	 * Create unallocated space bitmap descriptor. Sequential recorded
	 * media report their own free/used space; no free/used space tables
	 * should be recorded for these.
	 */
	if ((format_flags & (FORMAT_SEQUENTIAL | FORMAT_READONLY)) == 0) {
		error = udf_create_space_bitmap(
				layout.alloc_bitmap_dscr_size,
				layout.part_size_lba,
				&context.part_unalloc_bits[data_part]);
		if (error)
			return error;
		/* TODO: freed space bitmap if applicable */

		/* mark space allocated for the unallocated space bitmap */
		udf_mark_allocated(layout.unalloc_space, data_part,
			layout.alloc_bitmap_dscr_size);
	}

	/*
	 * Create metadata partition file entries and allocate and init their
	 * space and free space maps.
	 */
	if (format_flags & FORMAT_META) {
		error = udf_create_space_bitmap(
				layout.meta_bitmap_dscr_size,
				layout.meta_part_size_lba,
				&context.part_unalloc_bits[metadata_part]);
		if (error)
			return error;
	
		error = udf_create_meta_files();
		if (error)
			return error;

		/* mark space allocated for meta partition and its bitmap */
		udf_mark_allocated(layout.meta_file,   data_part, 1);
		udf_mark_allocated(layout.meta_mirror, data_part, 1);
		udf_mark_allocated(layout.meta_bitmap, data_part, 1);
		udf_mark_allocated(layout.meta_part_start_lba, data_part,
			layout.meta_part_size_lba);
		udf_mark_allocated(layout.meta_mirror_start_lba, data_part,
			layout.meta_part_size_lba);

		/* mark space allocated for the unallocated space bitmap */
		udf_mark_allocated(layout.meta_bitmap_space, data_part,
			layout.meta_bitmap_dscr_size);
	}

	/* create logical volume integrity descriptor */
	context.num_files = 0;
	context.num_directories = 0;
	integrity_type = UDF_INTEGRITY_OPEN;
	if ((error = udf_create_lvintd(integrity_type)))
		return error;

	/* writeout initial open integrity sequence + terminator */
	loc = layout.lvis;
	dscr = (union dscrptr *) context.logvol_integrity;
	error = udf_write_dscr_phys(dscr, loc, 1);
	if (error)
		return error;
	loc++;
	error = udf_write_dscr_phys(terminator_dscr, loc, 1);
	if (error)
		return error;

	/* create VAT if needed */
	if (format_flags & FORMAT_VAT) {
		context.vat_allocated = context.sector_size;
		context.vat_contents  = malloc(context.vat_allocated);
		assert(context.vat_contents);

		udf_prepend_VAT_file();
	}

	/* create FSD and writeout */
	if ((error = udf_create_fsd()))
		return error;
	udf_mark_allocated(layout.fsd, metadata_part, 1);

	dscr = (union dscrptr *) context.fileset_desc;
	error = udf_write_dscr_virt(dscr, layout.fsd, metadata_part, 1);

	return error;
}


/* specific routine for newfs to create empty rootdirectory */
int
udf_do_rootdir(void) {
	union dscrptr *root_dscr;
	int error;

	/* create root directory and write out */
	assert(context.unique_id == 0x10);
	context.unique_id = 0;
	if ((error = udf_create_new_rootdir(&root_dscr)))
		return error;
	udf_mark_allocated(layout.rootdir, context.metadata_part, 1);

	error = udf_write_dscr_virt(root_dscr,
		layout.rootdir, context.metadata_part, 1);

	free(root_dscr);

	return error;
}


int
udf_do_newfs_postfix(void)
{
	union dscrptr *vat_dscr;
	union dscrptr *dscr;
	struct long_ad vatdata_pos;
	uint32_t loc, len, phys, sects;
	int data_part, metadata_part;
	int error;

	/* cache partition for we need it often */
	data_part     = context.data_part;
	metadata_part = context.metadata_part;

	if ((format_flags & FORMAT_SEQUENTIAL) == 0) {
		/* update lvint and mark it closed */
		udf_update_lvintd(UDF_INTEGRITY_CLOSED);

		/* overwrite initial terminator */
		loc = layout.lvis+1;
		dscr = (union dscrptr *) context.logvol_integrity;
		error = udf_write_dscr_phys(dscr, loc, 1);
		if (error)
			return error;
		loc++;
	
		/* mark end of integrity descriptor sequence again */
		error = udf_write_dscr_phys(terminator_dscr, loc, 1);
		if (error)
			return error;
	}

	/* write out unallocated space bitmap on non sequential media */
	if ((format_flags & (FORMAT_SEQUENTIAL | FORMAT_READONLY)) == 0) {
		/* writeout unallocated space bitmap */
		loc  = layout.unalloc_space;
		dscr = (union dscrptr *) (context.part_unalloc_bits[data_part]);
		len  = layout.alloc_bitmap_dscr_size;
		error = udf_write_dscr_virt(dscr, loc, data_part, len);
		if (error)
			return error;
	}

	if (format_flags & FORMAT_META) {
		loc = layout.meta_file;
		dscr = (union dscrptr *) context.meta_file;
		error = udf_write_dscr_virt(dscr, loc, data_part, 1);
		if (error)
			return error;
	
		loc = layout.meta_mirror;
		dscr = (union dscrptr *) context.meta_mirror;
		error = udf_write_dscr_virt(dscr, loc, data_part, 1);
		if (error)
			return error;

		loc = layout.meta_bitmap;
		dscr = (union dscrptr *) context.meta_bitmap;
		error = udf_write_dscr_virt(dscr, loc, data_part, 1);
		if (error)
			return error;

		/* writeout unallocated space bitmap */
		loc  = layout.meta_bitmap_space;
		dscr = (union dscrptr *)
			(context.part_unalloc_bits[metadata_part]);
		len  = layout.meta_bitmap_dscr_size;
		error = udf_write_dscr_virt(dscr, loc, data_part, len);
		if (error)
			return error;
	}

	/* create a VAT and account for FSD+root */
	vat_dscr = NULL;
	if (format_flags & FORMAT_VAT) {
		/* update lvint to reflect the newest values (no writeout) */
		udf_update_lvintd(UDF_INTEGRITY_CLOSED);

		error = udf_append_VAT_file();
		if (error)
			return error;

		/* write out VAT data */
		sects = UDF_ROUNDUP(context.vat_size, context.sector_size) /
			context.sector_size;
		layout.vat = context.data_alloc_pos;
		udf_data_alloc(sects, &vatdata_pos);

		loc = udf_rw32(vatdata_pos.loc.lb_num);
		phys = context.vtop_offset[context.data_part] + loc;

		error = udf_write_phys(context.vat_contents, phys, sects);
		if (error)
			return error;
		loc += sects;

		/* create new VAT descriptor */
		error = udf_create_VAT(&vat_dscr);
		if (error)
			return error;
		context.data_alloc_pos++;
		loc++;

		error = udf_write_dscr_virt(vat_dscr, loc, metadata_part, 1);
		free(vat_dscr);
		if (error)
			return error;
	}

	/* done */
	return 0;
}
