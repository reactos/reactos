/* $NetBSD: udf_create.h,v 1.7 2013/08/09 15:11:08 reinoud Exp $ */

/*
 * Copyright (c) 2006, 2008 Reinoud Zandijk
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

#ifndef _FS_UDF_UDF_CREATE_H_
#define _FS_UDF_UDF_CREATE_H_

#include <sys/types.h>
#include <sys/stat.h>
#if !HAVE_NBTOOL_CONFIG_H
#include "ecma167-udf.h"
#else
#include "../../sys/fs/udf/ecma167-udf.h"
#endif
#include "udf_bswap.h"
#include "udf_osta.h"


/* format flags indicating properties of disc to create */
#define FORMAT_WRITEONCE	0x00001
#define FORMAT_SEQUENTIAL	0x00002
#define FORMAT_REWRITABLE	0x00004
#define FORMAT_SPARABLE		0x00008
#define FORMAT_META		0x00010
#define FORMAT_LOW		0x00020
#define FORMAT_VAT		0x00040
#define FORMAT_WORM		0x00080
#define FORMAT_TRACK512		0x00100
#define FORMAT_INVALID		0x00200
#define FORMAT_READONLY		0x00400
#define FORMAT_FLAGBITS \
    "\10\1WRITEONCE\2SEQUENTIAL\3REWRITABLE\4SPARABLE\5META\6LOW" \
    "\7VAT\10WORM\11TRACK512\12INVALID\13READONLY"


/* structure space */
#define UDF_ANCHORS		4	/* 256, 512, N-256, N */
#define UDF_PARTITIONS		4	/* overkill */
#define UDF_PMAPS		4	/* overkill */

/* misc constants */
#define UDF_MAX_NAMELEN		255			/* as per SPEC */

/* translation constants */
#define UDF_VTOP_RAWPART UDF_PMAPS	/* [0..UDF_PMAPS> are normal     */

/* virtual to physical mapping types */
#define UDF_VTOP_TYPE_RAW            0
#define UDF_VTOP_TYPE_UNKNOWN        0
#define UDF_VTOP_TYPE_PHYS           1
#define UDF_VTOP_TYPE_VIRT           2
#define UDF_VTOP_TYPE_SPARABLE       3
#define UDF_VTOP_TYPE_META           4

#define UDF_TRANS_ZERO		((uint64_t) -1)
#define UDF_TRANS_UNMAPPED	((uint64_t) -2)
#define UDF_TRANS_INTERN	((uint64_t) -3)
#define UDF_MAX_SECTOR		((uint64_t) -10)	/* high water mark */

/* handys */
#define UDF_ROUNDUP(val, gran) \
	((uint64_t) (gran) * (((uint64_t)(val) + (gran)-1) / (gran)))

#define UDF_ROUNDDOWN(val, gran) \
	((uint64_t) (gran) * (((uint64_t)(val)) / (gran)))


/* disc offsets for various structures and their sizes */
struct udf_disclayout {
	uint32_t wrtrack_skew;

	uint32_t iso9660_vrs;
	uint32_t anchors[UDF_ANCHORS];
	uint32_t vds_size, vds1, vds2;
	uint32_t lvis_size, lvis;

	uint32_t first_lba, last_lba;
	uint32_t sector_size;
	uint32_t blockingnr, align_blockingnr, sparable_blockingnr;
	uint32_t meta_blockingnr, meta_alignment;

	/* sparables */
	uint32_t sparable_blocks;
	uint32_t sparable_area, sparable_area_size;
	uint32_t sparing_table_dscr_lbas;
	uint32_t spt_1, spt_2;

	/* bitmaps */
	uint32_t alloc_bitmap_dscr_size;
	uint32_t unalloc_space, freed_space;

	uint32_t meta_bitmap_dscr_size;
	uint32_t meta_bitmap_space;

	/* metadata partition */
	uint32_t meta_file, meta_mirror, meta_bitmap;
	uint32_t meta_part_start_lba, meta_mirror_start_lba;
	uint32_t meta_part_size_lba;

	/* main partition */
	uint32_t part_start_lba, part_size_lba;

	uint32_t fsd, rootdir, vat;

};


/* all info about discs and descriptors building */
struct udf_create_context {
	/* descriptors */
	int	 dscrver;	/* 2 or 3	*/
	int	 min_udf;	/* hex		*/
	int	 max_udf;	/* hex		*/
	int	 serialnum;	/* format serialno */

	int	 gmtoff;	/* in minutes	*/

	/* XXX to layout? */
	uint32_t	 sector_size;

	/* identification */
	char	*logvol_name;
	char	*primary_name;
	char	*volset_name;
	char	*fileset_name;

	char const *app_name;
	char const *impl_name;
	int	 app_version_main;
	int	 app_version_sub;

	/* building */
	int	 vds_seq;	/* for building functions  */
	int	 unique_id;	/* only first few are used */

	/* constructed structures */
	struct anchor_vdp	*anchors[UDF_ANCHORS];	/* anchors to VDS    */
	struct pri_vol_desc	*primary_vol;		/* identification    */
	struct logvol_desc	*logical_vol;		/* main mapping v->p */
	struct unalloc_sp_desc	*unallocated;		/* free UDF space    */
	struct impvol_desc	*implementation;	/* likely reduntant  */
	struct logvol_int_desc	*logvol_integrity;	/* current integrity */
	struct part_desc	*partitions[UDF_PARTITIONS]; /* partitions   */

	/* XXX to layout? */
	int	data_part;
	int	metadata_part;

	/* block numbers as offset in partition */
	uint32_t metadata_alloc_pos;
	uint32_t data_alloc_pos;

	/* derived; points *into* other structures */
	struct udf_logvol_info	*logvol_info;		/* inside integrity  */

	/* fileset and root directories */
	struct fileset_desc	*fileset_desc;		/* normally one      */

	/* logical to physical translations */
	int 			 vtop[UDF_PMAPS+1];	/* vpartnr trans     */
	int			 vtop_tp[UDF_PMAPS+1];	/* type of trans     */
	uint64_t		 vtop_offset[UDF_PMAPS+1]; /* offset in lb   */

	/* sparable */
	struct udf_sparing_table*sparing_table;		/* replacements      */

	/* VAT file */
	uint32_t		 vat_size;		/* length */
	uint32_t		 vat_allocated;		/* allocated length */
	uint32_t		 vat_start;		/* offset 1st entry */
	uint8_t			*vat_contents;		/* the VAT */

	/* meta data partition */
	struct extfile_entry	*meta_file;
	struct extfile_entry	*meta_mirror;
	struct extfile_entry	*meta_bitmap;

	/* lvint */
	int	 num_files;
	int	 num_directories;
	uint32_t part_size[UDF_PARTITIONS];
	uint32_t part_free[UDF_PARTITIONS];

	struct space_bitmap_desc*part_unalloc_bits[UDF_PARTITIONS];
	struct space_bitmap_desc*part_freed_bits  [UDF_PARTITIONS];
};


/* globals */

extern struct udf_create_context context;
extern struct udf_disclayout     layout;
extern uint32_t udf_physical_alignment;
extern uint32_t udf_physical_offset;

/* prototypes */
void udf_init_create_context(void);
int a_udf_version(const char *s, const char *id_type);

int udf_calculate_disc_layout(int format_flags, int min_udf,
	uint32_t wrtrack_skew,
	uint32_t first_lba, uint32_t last_lba,
	uint32_t sector_size, uint32_t blockingnr,
	uint32_t sparable_blocks,
	float meta_fract);

void udf_osta_charset(struct charspec *charspec);
void udf_encode_osta_id(char *osta_id, uint16_t len, char *text);

void udf_set_regid(struct regid *regid, char const *name);
void udf_add_domain_regid(struct regid *regid);
void udf_add_udf_regid(struct regid *regid);
void udf_add_impl_regid(struct regid *regid);
void udf_add_app_regid(struct regid *regid);

int  udf_validate_tag_sum(union dscrptr *dscr);
int  udf_validate_tag_and_crc_sums(union dscrptr *dscr);

void udf_set_timestamp_now(struct timestamp *timestamp);

void udf_inittag(struct desc_tag *tag, int tagid, uint32_t loc);
int udf_create_anchor(int num);

void udf_create_terminator(union dscrptr *dscr, uint32_t loc);
int udf_create_primaryd(void);
int udf_create_partitiond(int part_num, int part_accesstype);
int udf_create_unalloc_spaced(void);
int udf_create_sparing_tabled(void);
int udf_create_space_bitmap(uint32_t dscr_size, uint32_t part_size_lba,
	struct space_bitmap_desc **sbdp);
int udf_create_logical_dscr(int format_flags);
int udf_create_impvold(char *field1, char *field2, char *field3);
int udf_create_fsd(void);
int udf_create_lvintd(int type);
void udf_update_lvintd(int type);

int udf_register_bad_block(uint32_t location);
void udf_mark_allocated(uint32_t start_lb, int partnr, uint32_t blocks);

int udf_create_new_fe(struct file_entry **fep, int file_type,
	struct stat *st);
int udf_create_new_efe(struct extfile_entry **efep, int file_type,
	struct stat *st);

int udf_encode_symlink(uint8_t **pathbufp, uint32_t *pathlenp, char *target);

void udf_advance_uniqueid(void);
int udf_fidsize(struct fileid_desc *fid);
void udf_create_fid(uint32_t diroff, struct fileid_desc *fid,
	char *name, int namelen, struct long_ad *ref);
int udf_create_parentfid(struct fileid_desc *fid, struct long_ad *parent);

int udf_create_meta_files(void);
int udf_create_new_rootdir(union dscrptr **dscr);

int udf_create_VAT(union dscrptr **vat_dscr);
void udf_prepend_VAT_file(void);
void udf_vat_update(uint32_t virt, uint32_t phys);
int udf_append_VAT_file(void);

#endif /* _FS_UDF_UDF_CREATE_H_ */

