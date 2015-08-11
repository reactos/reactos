/*

  Copyright(C) 2010 Alex Andreotti <alex.andreotti@gmail.com>

  This file is part of chmc.

  chmc is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  chmc is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with chmc.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "chmc.h"

#include <fcntl.h>

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "err.h"


#include "encint.h"

#include <stdint.h>
#include "../lzx_compress/lzx_config.h"
#include "../lzx_compress/lzx_compress.h"

#define PACKAGE_STRING "hhpcomp development version"

int chmc_section_add(struct chmcFile *chm, const char *name);
struct chmcSection * chmc_section_create(struct chmcFile *chm,
                                         const char *name);
void chmc_reset_table_init(struct chmcLzxcResetTable *reset_table);
void chmc_control_data_init(struct chmcLzxcControlData *control_data);
int chmc_namelist_create(struct chmcFile *chm, int len);
struct chmcTreeNode * chmc_add_meta(struct chmcFile *chm,
                                    const char *metaname, int sect_id,
                                    UChar *buf, UInt64 len);
struct chmcTreeNode *chmc_add_entry(struct chmcFile *chm, const char *name,
                                    UInt16 prefixlen, int sect_id,
                                    UChar *buf, UInt64 offset, UInt64 len);
void chmc_sections_free(struct chmcFile *chm);
void chmc_section_destroy(struct chmcSection *section);
void chmc_pmgi_free(struct chmcFile *chm);
void chmc_pmgl_free(struct chmcFile *chm);
void chmc_pmgl_destroy(struct chmcPmglChunkNode *node);
void chmc_pmgi_destroy(struct chmcPmgiChunkNode *node);
void chmc_entries_free(struct chmcFile *chm);
void chmc_entry_destroy(struct chmcTreeNode *node);
int chmc_add_tree(struct chmcFile *chm, const char *dir);
static int _add_tree_file( struct dir_tree_global *dtg,
                           struct dir_tree_local *dtl );
static int _add_tree_dir(struct dir_tree_global *dtg,
                         struct dir_tree_local *dtl);
struct chmcTreeNode *chmc_add_file(struct chmcFile *chm, const char *filename,
                                   UInt16 prefixlen, int sect_id, UChar *buf,
                                   UInt64 len);
struct chmcTreeNode *chmc_add_dir(struct chmcFile *chm, const char *dir);
struct chmcTreeNode *chmc_add_empty(struct chmcFile *chm, const char *file);

int chmc_crunch_lzx(struct chmcFile *chm, int sect_id);
static int _lzx_at_eof(void *arg);
static int _lzx_put_bytes(void *arg, int n, void *buf);
static void _lzx_mark_frame(void *arg, uint32_t uncomp, uint32_t comp);
static int _lzx_get_bytes(void *arg, int n, void *buf);

int chmc_compressed_add_mark(struct chmcFile *chm, UInt64 at);
int chmc_control_data_done(struct chmcFile *chm);
int chmc_reset_table_done(struct chmcFile *chm);
void chmc_pmgl_done(struct chmcFile *chm);

void chmc_entries_qsort(struct chmcFile *chm);
static int _entry_cmp(struct chmcTreeNode **pa, struct chmcTreeNode **pb);

struct chmcSection *chmc_section_lookup(struct chmcFile *chm, int id);

struct chmcPmglChunkNode *chmc_pmgl_create(void);
void chmc_pmgl_add(struct chmcFile *chm, struct chmcPmglChunkNode *pmgl);
void chmc_pmgl_init(struct chmcPmglChunkNode *node);
int chmc_pmgi_add_entry(struct chmcFile *chm, const char *name, int pmgl_id);
void chmc_pmgi_add(struct chmcFile *chm, struct chmcPmgiChunkNode *pmgi);
void chmc_string_init(struct chmcStringChunk *node);

struct chmcLzxInfo
{
	struct chmcFile *chm;
	struct chmcSection *section;
	int fd;
	UInt32 fd_offset;
	UInt32 done;
	UInt32 todo;
	struct list_head *pos;
	int error;
	int eof;
};

static const short chmc_transform_list[] = {
	0x7b, 0x37, 0x46, 0x43, 0x32, 0x38, 0x39,
	0x34, 0x30, 0x2d, 0x39, 0x44, 0x33, 0x31,
	0x2d, 0x31, 0x31, 0x44, 0x30 };

int chmc_init(struct chmcFile *chm, const char *filename,
              struct chmcConfig *config)
{
	struct chmcItsfHeader *itsf = &chm->itsf;
	struct chmcSect0 *sect0 = &chm->sect0;
	struct chmcItspHeader *itsp = &chm->itsp;
	struct chmcSystem *system = &chm->system;
	struct chmcSystemInfo *sysinfo = &chm->system.info;
	struct chmcIndexHeader *idxhdr = &chm->idxhdr;

	assert(chm);
	assert(filename);

	chmcerr_clean();

	memset(chm, 0, sizeof(struct chmcFile));

	chm->config = config;

	if (strcmp(filename, "-") != 0) {
		chm->fd = creat(filename, 0644);
		if (chm->fd < 0) {
			chmcerr_set(errno, strerror(errno));
			chmcerr_return_msg("creat file '%s'", filename);
		}
	} else {
		chm->fd = fileno(stdout);
	}

	memcpy(itsf->signature, "ITSF", 4);
	itsf->version = 3;
	itsf->header_len = _CHMC_ITSF_V3_LEN;
	itsf->unknown_000c = 1;

	itsf->lang_id = chm->config->language;
	memcpy(itsf->dir_uuid, CHMC_DIR_UUID, 16);
	memcpy(itsf->stream_uuid, CHMC_STREAM_UUID, 16);
	itsf->dir_offset = _CHMC_ITSF_V3_LEN + _CHMC_SECT0_LEN;

	itsf->sect0_offset = _CHMC_ITSF_V3_LEN;
	itsf->sect0_len = _CHMC_SECT0_LEN;

	sect0->file_len = _CHMC_ITSF_V3_LEN
		+ _CHMC_SECT0_LEN
		+ _CHMC_ITSP_V1_LEN;

	sect0->unknown_0000 = 510;

	memcpy(itsp->signature, "ITSP", 4);
	itsp->version = 1;
	itsp->header_len = _CHMC_ITSP_V1_LEN;
	itsp->unknown_000c = 10;
	itsp->block_len = _CHMC_CHUNK_LEN;
	itsp->blockidx_intvl = CHM_IDX_INTVL;
	itsp->index_depth = 2;

	itsp->unknown_0028 = -1;
	itsp->lang_id = CHMC_MS_LCID_EN_US;
	memcpy(itsp->system_uuid, CHMC_SYSTEM_UUID, 16);
	itsp->header_len2 = _CHMC_ITSP_V1_LEN;
	memset(itsp->unknown_0048, -1, 12);

	system->version = 3;
	system->_size = _CHMC_SYSTEM_HDR_LEN + sizeof(struct chmcIndexHeader);

	sysinfo->lcid = CHMC_MS_LCID_EN_US;

	memcpy(idxhdr->signature, "T#SM", 4);
	idxhdr->unknown_4 = 28582569; // FIXME got from some chm
	idxhdr->unknown_8 = 1;
	//   idxhdr->full_search = 1;
	//   idxhdr->klinks = 1;
	//   idxhdr->alinks = 0;
	//   idxhdr->timestamp = ???;

	//   idxhdr->num_of_topic = 2; // sorry??
	idxhdr->off_img_list = -1;
	//   idxhdr->img_type_folder;
	idxhdr->background = -1;
	idxhdr->foreground = -1;
	idxhdr->off_font = -1;
	idxhdr->win_style = -1;
	idxhdr->ex_win_style = -1;
	idxhdr->unknown_34 = -1;
	idxhdr->off_frame_name = -1;
	idxhdr->off_win_name = -1;
	//   idxhdr->num_of_info;
	idxhdr->unknown_44 = 1;
	//   idxhdr->num_of_merge_files;
	//   idxhdr->unknown_4c;

	INIT_LIST_HEAD(&chm->sections_list);
	INIT_LIST_HEAD(&chm->pmgl_list);
	INIT_LIST_HEAD(&chm->entries_list);
	INIT_LIST_HEAD(&chm->pmgi_list);

	chm->strings = malloc(4096);
	memset(chm->strings, 0, 4096);
	chm->strings_len = 4096;
	chm->strings_offset = 1;

	if (chmc_section_add(chm, "Uncompressed") != CHMC_NOERR)
		chmcerr_return_msg("adding section: Uncompressed");

	if (chmc_section_add(chm, "MSCompressed") != CHMC_NOERR)
		chmcerr_return_msg("adding section: MSCompressed");

	chmc_sections_done(chm);

	return CHMC_NOERR;
}

int chmc_section_add(struct chmcFile *chm, const char *name)
{
	struct chmcSection *section;

	assert(chm);
	assert(name);

	section = chmc_section_create(chm, name);
	if (!section)
		return chmcerr_code();

	list_add_tail(&section->list, &chm->sections_list);
	chm->sections_num++;

	return CHMC_NOERR;
}

struct chmcSection *chmc_section_create(struct chmcFile *chm,
                                        const char *name)
{
	struct chmcSection *section;

	assert(name);

	section = calloc(1, sizeof(struct chmcSection));
	if (section) {
		const char *tmpdir;
		int len;

		len = strlen(name);
		memcpy(section->name, name, len + 1);
		section->offset = 0;
		section->len = 0;

		tmpdir = NULL;
		if (chm->config != NULL)
			tmpdir = chm->config->tmpdir;
		if (tmpdir == NULL)
			tmpdir = "/tmp/";

		len = strlen(tmpdir);
		if (len >= PATH_MAX - 12) {
			chmcerr_set(errno, strerror(errno));
			chmcerr_msg("tmpdir too long: '%s'", tmpdir);
			goto fail;
		}

		strcat(section->filename, tmpdir);
		if (section->filename[len - 1] != '/')
			strcat(section->filename, "/");

		if (strcmp("MSCompressed", name) == 0)
			strcat(section->filename, "chmcCXXXXXX");
		else
			strcat(section->filename, "chmcUXXXXXX");

		section->fd = mkstemp(section->filename);
		fprintf(stderr, "temp file: %s\n", section->filename);
		if (section->fd < 0) {
			chmcerr_set(errno, strerror(errno));
			chmcerr_msg("creat() file '%s'", section->filename);
			goto fail;
		}
		else if (strcmp(section->name, "MSCompressed") == 0) {
			chmc_reset_table_init(&section->reset_table_header);
			chmc_control_data_init(&section->control_data);
			INIT_LIST_HEAD(&section->mark_list);
			section->mark_count = 0;
		}
	} else {
		chmcerr_set(errno, strerror(errno));
		chmcerr_msg("section '%s' allocation failed", name);
	}

	return section;

 fail:
	free(section);
	return NULL;
}

void chmc_reset_table_init(struct chmcLzxcResetTable *reset_table)
{
	reset_table->version = 2;
	reset_table->block_count = 0;
	reset_table->entry_size = 8;
	reset_table->table_offset = _CHMC_LZXC_RESETTABLE_V1_LEN;
	reset_table->uncompressed_len = 0;
	reset_table->compressed_len = 0;
	reset_table->block_len = 0x8000;
}

void chmc_control_data_init(struct chmcLzxcControlData *control_data)
{
	control_data->size = 6;
	memcpy(control_data->signature, "LZXC", 4);
	control_data->version = 2;
	control_data->resetInterval = 2;
	control_data->windowSize = 2;
	control_data->windowsPerReset = 1;
	control_data->unknown_18 = 0;
}

void chmc_sections_done(struct chmcFile *chm)
{
	int len;
	int i;

	assert(chm);

	chm->sections = malloc(sizeof(struct chmcSection *) * chm->sections_num);
	if (chm->sections) {
		struct chmcSection *section;
		struct list_head *pos;

		i = 0;
		len = 4;
		list_for_each(pos, &chm->sections_list) {
			section = list_entry(pos, struct chmcSection, list);
			len += 4 + strlen(section->name) * 2;
			chm->sections[i++] = section;
		}
		chmc_namelist_create(chm, len);
	} else
		BUG_ON("FIXME: %s: %d\n", __FILE__, __LINE__);
}

int chmc_namelist_create(struct chmcFile *chm, int len)
{
	UInt16 *namelist;

	namelist = malloc(len);
	if (namelist) {
		struct chmcSection *section;
		int i, j, k, name_len;

		k = 0;
		namelist[k++] = len >> 1;
		namelist[k++] = chm->sections_num;
		for( i=0; i < chm->sections_num; i++ ) {
			section = chm->sections[i];

			name_len = strlen(section->name);
			namelist[k++] = name_len;
			for( j=0; j < name_len; j++ )
				namelist[k++] = section->name[j];
			namelist[k++] = 0;
		}
		chmc_add_meta(chm, "::DataSpace/NameList", 0, (UChar *)namelist, len);
	}
	else
		return CHMC_ENOMEM;

	return CHMC_NOERR;
}

struct chmcTreeNode *chmc_add_empty(struct chmcFile *chm, const char *file)
{
	assert(chm);
	return chmc_add_entry(chm, file, 0, 0, NULL, 0, 0);
}

struct chmcTreeNode *chmc_add_meta(struct chmcFile *chm, const char *metaname,
                                   int sect_id,
              UChar *buf, UInt64 len)
{
	struct chmcSection *section;
	struct chmcTreeNode *node;

	assert(chm);

	if (sect_id >= chm->sections_num)
		return NULL;

	section = chm->sections[sect_id];

	node = chmc_add_entry(chm, metaname, 0, sect_id, buf, section->offset, len);

	if ((node) && (len > 0))
		section->offset += len;

	return node;
}

struct chmcTreeNode *chmc_add_entry(struct chmcFile *chm, const char *name,
                                    UInt16 prefixlen, int sect_id, UChar *buf,
                                    UInt64 offset, UInt64 len)
{
	struct chmcTreeNode *node;

	assert(chm);

	if (sect_id >= (chm->sections_num)) {
		fprintf(stderr,"sect_id %d >= chm->sections_num %d\n",
		        sect_id, chm->sections_num);
		return NULL;
	}

	node = malloc(sizeof(struct chmcTreeNode));
	if (node) {
		node->flags = 0;
		node->name = strdup( name );
		node->prefixlen = prefixlen;
		node->sect_id = sect_id;
		node->buf = buf;
		node->offset = offset;
		node->len = len;
		list_add_tail(&node->list, &chm->entries_list);
		chm->entries_num++;
	}
	else
		BUG_ON("FIXME: %s: %d\n", __FILE__, __LINE__);

	return node;
}

void chmc_term(struct chmcFile *chm)
{
	assert(chm);
	assert(chm->fd > -1);

	free(chm->strings);

	chmc_entries_free(chm);
	chmc_pmgl_free(chm);
	chmc_pmgi_free(chm);
	if (chm->sections)
		free(chm->sections);
	chmc_sections_free(chm);

	if (chm->fd != fileno(stdout))
		close(chm->fd);
}

void chmc_sections_free(struct chmcFile *chm)
{
	struct chmcSection *section;
	struct list_head *pos, *q;

	assert(chm);

	list_for_each_safe(pos, q, &chm->sections_list) {
		section = list_entry(pos, struct chmcSection, list);
		list_del(pos);
		chmc_section_destroy(section);
	}
}

void chmc_section_destroy(struct chmcSection *section)
{
	assert(section);
	assert(section->fd > -1);

	if (strcmp(section->name, "MSCompressed") == 0) {
		struct list_head *pos, *q;
		struct chmcResetTableMark *mark;

		list_for_each_safe(pos, q, &section->mark_list) {
			mark = list_entry(pos, struct chmcResetTableMark, list);
			list_del(pos);
			free(mark);
		}
	}

	close(section->fd);
	unlink(section->filename);
	free(section);
}

void chmc_pmgi_free(struct chmcFile *chm)
{
	struct chmcPmgiChunkNode *node;
	struct list_head *pos, *q;

	assert(chm);

	list_for_each_safe(pos, q, &chm->pmgi_list) {
		node = list_entry(pos, struct chmcPmgiChunkNode, list);
		list_del(pos);
		chmc_pmgi_destroy(node);
	}
}

void chmc_pmgl_free(struct chmcFile *chm)
{
	struct chmcPmglChunkNode *node;
	struct list_head *pos, *q;

	assert(chm);

	list_for_each_safe(pos, q, &chm->pmgl_list) {
		node = list_entry(pos, struct chmcPmglChunkNode, list);
		list_del(pos);
		chmc_pmgl_destroy(node);
	}
}

void chmc_entries_free( struct chmcFile *chm )
{
	struct chmcTreeNode *node;
	struct list_head *pos, *q;

	assert(chm);

	list_for_each_safe(pos, q, &chm->entries_list) {
		node = list_entry(pos, struct chmcTreeNode, list);
		list_del(pos);
		chmc_entry_destroy(node);
	}

	free(chm->sort_entries);
}

UInt32 chmc_strings_add( struct chmcFile *chm, const char *s)
{
	UInt32 len, off;

	/* FIXME null are errors */

	if (!s || *s == '\0')
		return 0;

	len = strlen(s);

	off = chm->strings_offset;

	if (off + len + 1 < chm->strings_len) {

		memcpy(&chm->strings[off], s, len + 1);
		chm->strings_offset += len + 1;

	} else {
		/* realloc strings */
		/* if the string truncate copy til end of chunk
		   then re-copy from 0 of new */
		BUG_ON("FIXME: %s: %d: handle more chunk for strings\n",
		       __FILE__, __LINE__);
	}

	return off;
}

void chmc_entry_destroy( struct chmcTreeNode *node )
{
	assert(node);
	assert(node->name);

	free(node->name);
	if (node->buf && !(node->flags & CHMC_TNFL_STATIC))
		free(node->buf);
	free(node);
}

struct chmcTreeNode *chmc_add_file(struct chmcFile *chm, const char *filename,
                                   UInt16 prefixlen, int sect_id, UChar *buf,
                                   UInt64 len)
{
	struct chmcSection *section;
	struct chmcTreeNode *node;

	assert(chm);

	if (sect_id >= chm->sections_num)
		return NULL;

	section = chm->sections[sect_id];

	node = chmc_add_entry(chm, filename, prefixlen, sect_id, NULL,
	                      section->offset, len);

	if ((node) && (len > 0))
		section->offset += len;

	return node;
}

struct chmcTreeNode *chmc_add_dir(struct chmcFile *chm, const char *dir)
{
	assert(chm);

	return chmc_add_entry(chm, dir, 0, 0, NULL, 0, 0);
}

static inline void *chmc_syscat_mem(void *d, void *s, unsigned long len)
{
	memcpy(d, s, len);

	return d + len;
}

static void *chmc_syscat_entry(Int16 code, void *d, void *s, Int16 len)
{
	d = chmc_syscat_mem(d, &code, 2);
	d = chmc_syscat_mem(d, &len, 2);

	return chmc_syscat_mem(d, s, len);
}

/* #define DEFAULT_TOPIC "index.htm" */
/* #define TITLE "hello world" */
/* #define LCASEFILE "test" */

int chmc_system_done(struct chmcFile *chm)
{
	struct chmcSystem *system;
	struct chmcSystemInfo *sysinfo;
	struct chmcIndexHeader *idxhdr;
	void *sysp, *p;

	assert(chm);

	system = &chm->system;
	sysinfo = &system->info;
	idxhdr = &chm->idxhdr;

	// TODO should be set from application
	//   system->_size +=   (_CHMC_SYS_ENTRY_HDR_LEN + sizeof(UInt32)) /* timestamp */
	//                    + (_CHMC_SYS_ENTRY_HDR_LEN + sizeof(PACKAGE_STRING)) /* compiler */
	//                    + (_CHMC_SYS_ENTRY_HDR_LEN + sizeof(UInt32)) /* eof */
	//                    + (_CHMC_SYS_ENTRY_HDR_LEN + sizeof(DEFAULT_TOPIC))
	//                    + (_CHMC_SYS_ENTRY_HDR_LEN + sizeof(TITLE))
	//                    + 32;

	sysp = malloc(16384);
	if (sysp) {
		UInt32 val;
		UInt16 code, len;
		const char *entry_val;

		p = chmc_syscat_mem(sysp, &system->version, sizeof(system->version));

		val = 0;
		p = chmc_syscat_entry(SIEC_TIMESTAMP, p, &val, sizeof(val));
		p = chmc_syscat_entry(SIEC_COMPVER, p,
		                      /*"HHA Version 4.74.8702"*/
		                      PACKAGE_STRING,
		                      sizeof(PACKAGE_STRING)
		                      /*strlen("HHA Version 4.74.8702")+1*/);
		p = chmc_syscat_entry(SIEC_SYSINFO, p,
		                      sysinfo, sizeof(struct chmcSystemInfo));

		if (chm->config != NULL && chm->config->deftopic != NULL)
			entry_val = chm->config->deftopic;
		else
			entry_val = "index.htm";
		p = chmc_syscat_entry(SIEC_DEFTOPIC, p, (void *)entry_val,
		                      strlen(entry_val)+1);

		if (chm->config != NULL && chm->config->title != NULL)
			entry_val = chm->config->title;
		else
			entry_val = "untitled";
		p = chmc_syscat_entry(SIEC_TITLE, p, (void *)entry_val,
		                      strlen(entry_val)+1);
		//       p = chmc_syscat_entry(SIEC_DEFFONT, p, &val, sizeof(val));
		p = chmc_syscat_entry(SIEC_LCASEFILE, p, "siec_lcasefile",
		                      strlen("siec_lcasefile")+1);
		p = chmc_syscat_entry(SIEC_DEFWINDOW, p,
		                      "MsdnHelp", strlen("MsdnHelp")+1);

		val = 0;
		p = chmc_syscat_entry(SIEC_NUMOFINFOT, p, &val, sizeof(val));

		p = chmc_syscat_entry(SIEC_IDXHDR, p,
		                      idxhdr, sizeof(struct chmcIndexHeader));


		val = 0;
		p = chmc_syscat_entry(SIEC_INFOCHKSUM, p, &val, sizeof(val));

		system->_size = p - sysp;
		chmc_add_meta(chm, "/#SYSTEM", 0, sysp, system->_size);
		return CHMC_NOERR;
	}

	chmcerr_set(CHMC_ENOMEM, "system done: malloc %d bytes",
	            system->_size);

	return CHMC_ENOMEM;
}

int chmc_tree_done( struct chmcFile *chm )
{
	struct chmcItsfHeader *itsf;
	struct chmcSect0 *sect0;
	struct chmcItspHeader *itsp;
	struct chmcTreeNode *ctrl;
	UInt32 str_index;
	const char *val;

	assert(chm);

	itsf = &chm->itsf;
	sect0 = &chm->sect0;
	itsp = &chm->itsp;

	chmc_add_dir(chm, "/");

	ctrl = chmc_add_meta(chm, "::DataSpace/Storage/MSCompressed/Transform/List",
	                     0, (UChar *)chmc_transform_list,
	                     sizeof(chmc_transform_list));
	if (ctrl)
		ctrl->flags |= CHMC_TNFL_STATIC;

	chmc_system_done(chm);

	if (chm->config != NULL && chm->config->deftopic != NULL)
		val = chm->config->deftopic;
	else
		val = "index.htm";

	str_index = chmc_strings_add(chm, val);

#if 0
	// FIXME just a test
	{
		UChar *p;
		int len;
		struct chmcTopicEntry topicEntry;
		//     struct chmcUrlStrEntry urlStrEntry;

		p = malloc(4096);
		if (p) {
			memset(p, 0, 4096);
			len = 0;

			topicEntry.tocidx_offset = 4096;
			topicEntry.strings_offset = -1;
			topicEntry.urltbl_offset = 0;
			topicEntry.in_content = 6;
			topicEntry.unknown = 0;

			memcpy(p, &topicEntry, sizeof(struct chmcTopicEntry));
			len += sizeof(struct chmcTopicEntry);

			chm->idxhdr.num_of_topic++;

			chmc_add_meta(chm, "/#TOPICS", 1, (UChar *)p, len);
		} else
			BUG_ON("FIXME: %s: %d\n", __FILE__, __LINE__);
	}
#endif

	ctrl = chmc_add_meta(chm, "/#IDXHDR", 1, (void *)&chm->idxhdr,
	                     sizeof(struct chmcIndexHeader));
	if (ctrl)
		ctrl->flags |= CHMC_TNFL_STATIC;

	{
		UInt32 *p;
		p = malloc(8+196);
		if (p) {
			const char *val;
			memset(p+2, 0, 196);

			p[0] = 1;
			p[1] = 196;

			p[2+0] = 196;
			//       p[2+2] = 1;
			//       p[2+3] = 0x00000532;
			//       p[2+4] = 0x00062520;

			//       p[2+8] = 86;
			//       p[2+9] = 51;
			//       p[2+10] = 872;
			//       p[2+11] = 558;

			//       p[2+19] = 220;

			//       p[2+27] = 0x00000041;
			//       p[2+28] = 14462;

			if (chm->config != NULL && chm->config->title != NULL)
				val = chm->config->title;
			else
				val = "untitled";
			p[2+5] = chmc_strings_add(chm, val);

			if (chm->config != NULL && chm->config->hhc != NULL)
				val = chm->config->hhc;
			else
				val = "toc.hhc";
			p[2+24] = chmc_strings_add(chm, val);

			if (chm->config != NULL && chm->config->hhk != NULL)
				val = chm->config->hhc;
			else
				val = "toc.hhk";
			p[2+25] = chmc_strings_add(chm, val);
			p[2+26] = str_index;

			chmc_add_meta(chm, "/#WINDOWS", 1, (UChar *)p, 8+196);
		} else
			BUG_ON("FIXME: %s: %d\n", __FILE__, __LINE__);
	}

	ctrl = chmc_add_meta(chm, "/#STRINGS", 1, (void *)chm->strings,
	                     chm->strings_len);
	if (ctrl)
		ctrl->flags |= CHMC_TNFL_STATIC;

#if 0
	// FIXME just a test
	{
		UChar *p;
		int len;
		struct chmcUrlStrEntry urlStrEntry;

		urlStrEntry.url_offset = 0;
		urlStrEntry.framename_offset = 0;

		p = malloc(4096);
		if (p) {
			memset(p, 0, 4096);
			*p = 0x42;
			len = 1;

			memcpy(p + len, &urlStrEntry, sizeof(struct chmcUrlStrEntry));
			len += sizeof(struct chmcUrlStrEntry);
			len += sprintf(p + len, "index.htm" ) + 1;

			memcpy(p + len, &urlStrEntry, sizeof(struct chmcUrlStrEntry));
			len += sizeof(struct chmcUrlStrEntry);
			len += sprintf(p + len, "test.htm" ) + 1;

			chmc_add_meta(chm, "/#URLSTR", 1, (UChar *)p, len);
		} else
			BUG_ON("FIXME: %s: %d\n", __FILE__, __LINE__);
	}
#endif

	//   chmc_add_entry(chm, "/#URLTBL", 0, 1, NULL, 0, 0);
	//   chmc_add_entry(chm, "/#TOPICS", 0, 1, NULL, 0, 0);

	// NOTE NOTE NOTE add any meta compressed before crunch ;-)

	chmc_crunch_lzx(chm, 1);

	chmc_control_data_done(chm);
	chmc_reset_table_done(chm);

	chmc_add_empty(chm, "/#ITBITS");

	// NOTE in this implementation compressed Content should be the last file
	//      added to section 0

	chmc_add_meta(chm, "::DataSpace/Storage/MSCompressed/Content", 0, NULL,
	              chm->sections[1]->offset);

	chmc_entries_qsort(chm);
	chmc_uncompressed_done(chm);
	chmc_pmgl_done(chm);

	chmc_pmgi_done(chm);

	itsf->dir_len =     _CHMC_ITSP_V1_LEN
		+ (_CHMC_CHUNK_LEN * itsp->num_blocks);

	itsf->data_offset = _CHMC_ITSF_V3_LEN
		+ _CHMC_SECT0_LEN
		+ _CHMC_ITSP_V1_LEN
		+ (_CHMC_CHUNK_LEN * itsp->num_blocks);

	sect0->file_len +=  _CHMC_CHUNK_LEN * itsp->num_blocks;

	chmc_write(chm);

	{
		struct chmcSection *section;
		struct list_head *pos;
		UChar buf[4096];

		list_for_each(pos, &chm->sections_list) {
			section = list_entry(pos, struct chmcSection, list);
			chmc_appendfile(chm, section->filename, buf, 4096);
		}
	}

	return CHMC_NOERR;
}

int chmc_crunch_lzx(struct chmcFile *chm, int sect_id)
{
	struct chmcLzxInfo lzx_info;

	lzx_data *lzxd;
	int subd_ok = 1;
	int do_reset = 1;
	int block_size;
	lzx_results lzxr;
	int wsize_code = 16;

	assert(chm);

	if ((wsize_code < 15) || (wsize_code > 21)) {
		fprintf(stderr, "window size must be between 15 and 21 inclusive\n");
		return CHMC_EINVAL;
	}

	lzx_info.chm = chm;
	lzx_info.section = chm->sections[sect_id];
	lzx_info.done = 0;
	lzx_info.todo = lzx_info.section->offset;
	lzx_info.pos = chm->entries_list.next;
	lzx_info.error = 0;
	lzx_info.eof = 0;

	lzx_info.fd = -1;
	lzx_info.fd_offset = 0;

	chmc_compressed_add_mark(lzx_info.chm, 0);
	lzx_info.section->reset_table_header.block_count++;

	/* undocumented fact, according to Caie --
	   block size cannot exceed window size.  (why not?) */
	/* The block size must not be larger than the window size.
	   While the compressor will create apparently-valid LZX files
	   if this restriction is violated, some decompressors
	   will not handle them. */

	block_size = 1 << wsize_code;

	//  lzx_info.section->control_data.windowSize = wsize_code;
	//  lzx_info.section->control_data.windowsPerReset = block_size;

	lzx_init(&lzxd, wsize_code,
	         _lzx_get_bytes, &lzx_info, _lzx_at_eof,
	         _lzx_put_bytes, &lzx_info,
	         _lzx_mark_frame, &lzx_info);

	while(! _lzx_at_eof(&lzx_info)) {
		if (do_reset)
			lzx_reset(lzxd);
		lzx_compress_block(lzxd, block_size, subd_ok);
	}
	lzx_finish(lzxd, &lzxr);

	return CHMC_NOERR;
}

static int _lzx_at_eof(void *arg)
{
	struct chmcLzxInfo *lzx_info = (struct chmcLzxInfo *)arg;

	return lzx_info->error || lzx_info->done >= lzx_info->todo || lzx_info->eof;
}

static int _lzx_put_bytes(void *arg, int n, void *buf)
{
	struct chmcLzxInfo *lzx_info = (struct chmcLzxInfo *)arg;
	struct chmcSect0 *sect0 = &lzx_info->chm->sect0;
	ssize_t wx;

	wx = write(lzx_info->section->fd, buf, n);
	sect0->file_len += wx;
	lzx_info->section->len += wx;

	return wx;
}

static void _lzx_mark_frame(void *arg, uint32_t uncomp, uint32_t comp)
{
	struct chmcLzxInfo *lzx_info = (struct chmcLzxInfo *)arg;
	struct chmcSection *section = lzx_info->chm->sections[1];

	UInt64 compressed;

	chmc_dump( "Aligned data at %d(in compressed stream, %d) (%lu/%lu)\n",
	           uncomp, comp, lzx_info->done, lzx_info->todo );

	compressed = comp;

	section->reset_table_header.block_count++;

	chmc_compressed_add_mark( lzx_info->chm, compressed );

	section->reset_table_header.uncompressed_len = uncomp;
	section->reset_table_header.compressed_len = comp;
}

static int _lzx_get_bytes(void *arg, int n, void *buf)
{
	struct chmcLzxInfo *lzx_info = (struct chmcLzxInfo *)arg;
	struct chmcFile *chm = lzx_info->chm;
	struct chmcTreeNode *node;

	int todo;
	int done;
	int toread;
	int rx;

	todo = n;
	done = 0;

	// compression state machine
	// lzx compressor ask for block input bytes
	// need to keep current entry file and offset trought blocks
	// until last entry
	while (todo) {
		// end of entris reached?
		if (lzx_info->pos == &chm->entries_list) {
			lzx_info->eof = 1;
			break;
		}

		node = list_entry( lzx_info->pos, struct chmcTreeNode, list );

		// skip empty files and directories
		if (node->len == 0
		   || strcmp("MSCompressed", chm->sections[node->sect_id]->name)) {
			lzx_info->pos = lzx_info->pos->next;
			continue;
		}
		else
			if (node->buf) {
				// have len and buffer, it's mallocated not file
			}
			else
				if (lzx_info->fd == -1) {
					// open file if it isn't
					lzx_info->fd = open(node->name, O_RDONLY);
					if (lzx_info->fd < 0) {
						chmc_error("%s: %d: error %d: '%s' %s\n",
						           __FILE__, __LINE__,
						           errno, node->name, strerror(errno));
						lzx_info->error = 1;
						break;
					}
				}

		// read till the end of the file or till the lzx buffer is filled
		toread = node->len - lzx_info->fd_offset;
		if (toread > todo)
			toread = todo;

		if (toread <= 0)
			continue;

		// read input
		if (node->buf) {
			memcpy(buf + (n - todo), &node->buf[lzx_info->fd_offset], toread);
			rx = toread;
		}
		else
			{
				rx = read(lzx_info->fd, buf + (n - todo), toread);
				if (rx <= 0) {
					chmc_error("read error\n");
					lzx_info->error = 2;
					break;
				}
			}

		todo -= rx;
		lzx_info->fd_offset += rx;
		done += rx;
		lzx_info->done += rx;

		// end of current file reached, goto next entry
		if (lzx_info->fd_offset == node->len) {
			if (lzx_info->fd > -1)
				close(lzx_info->fd);
			lzx_info->fd = -1;
			lzx_info->fd_offset = 0;
			lzx_info->pos = lzx_info->pos->next;
		}
	}

	return done;
}

int chmc_compressed_add_mark(struct chmcFile *chm, UInt64 at)
{
	struct chmcSection *section;
	struct chmcResetTableMark *mark;

	assert(chm);

	section = chm->sections[1];

	mark = malloc(_CHMC_RSTTBL_MARK);
	if (mark) {
		mark->at = at;
		chmc_dump("[%d] at: %jd\n", section->mark_count, at);
		list_add_tail(&mark->list, &section->mark_list);
		section->mark_count++;
		return CHMC_NOERR;
	}

	return CHMC_ENOMEM;
}

int chmc_control_data_done(struct chmcFile *chm)
{
	struct chmcTreeNode *ctrl;

	ctrl = chmc_add_meta(chm, "::DataSpace/Storage/MSCompressed/ControlData",
	                     0, (UChar *)&chm->sections[1]->control_data,
	                     _CHMC_LZXC_V2_LEN);

	if (ctrl) {
		ctrl->flags |= CHMC_TNFL_STATIC;
		return CHMC_NOERR;
	}

	return CHMC_ENOMEM;
}

int chmc_reset_table_done(struct chmcFile *chm)
{
	struct chmcSection *section;
	struct chmcLzxcResetTable *reset_table;
	struct list_head *pos;
	struct chmcResetTableMark *mark;

	UInt64 *at;
	int i, len;

	section = chm->sections[1];

	len = _CHMC_LZXC_RESETTABLE_V1_LEN + (section->mark_count * sizeof(UInt64));

	reset_table = malloc(len);

	if (reset_table) {
		memcpy(reset_table, &section->reset_table_header,
		       _CHMC_LZXC_RESETTABLE_V1_LEN);
		at = (void *)reset_table + _CHMC_LZXC_RESETTABLE_V1_LEN;

		i = 0;
		list_for_each(pos, &section->mark_list) {
			mark = list_entry(pos, struct chmcResetTableMark, list);
			at[i++] = mark->at;
		}

		chmc_add_dir(chm, "::DataSpace/Storage/MSCompressed/Transform/"
		             "{7FC28940-9D31-11D0-9B27-00A0C91E9C7C}/InstanceData/");
		chmc_add_meta(chm, "::DataSpace/Storage/MSCompressed/Transform/"
		              "{7FC28940-9D31-11D0-9B27-00A0C91E9C7C}"
		              "/InstanceData/ResetTable",
		              0, (UChar *)reset_table, len);

		{ // TODO FIXME do better
			UInt64 *uncompressed_len = malloc(8);
			if (uncompressed_len) {
				*uncompressed_len = reset_table->uncompressed_len;
				chmc_add_meta(chm, "::DataSpace/Storage/MSCompressed/SpanInfo",
				              0, (UChar *)uncompressed_len, 8);
			}
		}

		return CHMC_NOERR;
	}

	return CHMC_ENOMEM;
}

void chmc_entries_qsort(struct chmcFile *chm)
{
	struct chmcTreeNode *node;
	struct list_head *pos;
	int i;

	assert(chm);

	chm->sort_entries = malloc(sizeof(struct chmcTreeNode *)
	                           * chm->entries_num);

	i = 0;
	list_for_each(pos, &chm->entries_list) {
		node = list_entry(pos, struct chmcTreeNode, list);
		chm->sort_entries[i++] = node;
	}

	qsort(chm->sort_entries, chm->entries_num, sizeof(struct chmcTreeNode *),
	      (__compar_fn_t) _entry_cmp);
}

static int _entry_cmp(struct chmcTreeNode **pa, struct chmcTreeNode **pb)
{
	struct chmcTreeNode *a = *pa, *b = *pb;

	return strcmp( &a->name[a->prefixlen], &b->name[b->prefixlen] );
}

int chmc_uncompressed_done(struct chmcFile *chm)
{
	struct chmcSect0 *sect0 = &chm->sect0;
	struct chmcTreeNode *node;
	struct list_head *pos;
	ssize_t wx;

	list_for_each(pos, &chm->entries_list) {
		node = list_entry( pos, struct chmcTreeNode, list );

		if (strcmp( "MSCompressed", chm->sections[node->sect_id]->name ) == 0)
			continue;

		if ((node->buf) && (node->len > 0)) {
			wx = write(chm->sections[node->sect_id]->fd, node->buf, node->len);
			sect0->file_len += wx;
		}
	}

	return CHMC_NOERR;
}

void chmc_pmgl_done(struct chmcFile *chm)
{
	struct chmcTreeNode *entry;
	int i;

	assert(chm);

	for(i=0; i < chm->entries_num; i++) {
		entry = chm->sort_entries[i];
		chmc_pmgl_add_entry(chm, entry);
	}
}

int chmc_pmgl_add_entry(struct chmcFile *chm, struct chmcTreeNode *entry)
{
	struct chmcPmglChunkNode *pmgl;
	struct chmcPmglChunk *chunk;
	struct chmcSection *section;
	struct chmcItspHeader *itsp = &chm->itsp;

	UChar *p;
	UInt16 *idx;
	int name_len;
	int outlen;
	int should_idx, idx_intlv;
	int free;

	assert(chm);
	assert(entry);

	// check section bound
	section = chmc_section_lookup(chm, entry->sect_id);
	if (!section)
		chmcerr_set_return(CHMC_ENOMEM, "section %d lookup failed: ",
		                   entry->sect_id);

	// check chunk space for new entry
	name_len = strlen(&entry->name[entry->prefixlen]);

	outlen = chmc_encint_len(name_len);
	outlen += name_len;
	outlen += chmc_encint_len(entry->sect_id);
	outlen += chmc_encint_len(entry->offset);
	outlen += chmc_encint_len(entry->len);

	// look for current pmgl chunk, create if doesn't exist
	if (!chm->pmgl_last) {
		pmgl = chmc_pmgl_create();
		if (pmgl)
			chmc_pmgl_add(chm, pmgl);
		else
			chmcerr_set_return(CHMC_ENOMEM, "pmgl chunk: ");
	}
	else
		pmgl = chm->pmgl_last;

	do {

		chunk = &chm->pmgl_last->chunk;

		idx_intlv = 1 + ( 1 << itsp->blockidx_intvl );
		should_idx = ( ( chunk->entries_count > 0 )
		               && ! ( ( chunk->entries_count + 1 ) % idx_intlv )
		               ? 2 : 0 );

		free = sizeof(chunk->data) - pmgl->data_len - pmgl->index_len
			- should_idx;

		// current(last) chunk doesn't have enough room? force new one
		if (outlen + should_idx > free) {
			//chm->pmgl_last = NULL;
			pmgl = chmc_pmgl_create();
			if ( pmgl )
				chmc_pmgl_add(chm, pmgl);
			else
				chmcerr_set_return(CHMC_ENOMEM, "pmgl chunk: ");

			continue;
		}

		p = (void *)&chunk->data[pmgl->data_len];

		if (should_idx) {
			idx = (void *)&chunk->data[CHMC_PMGL_DATA_LEN] - pmgl->index_len;
			*idx = (void *)p - (void *)&chunk->data;
		}

		p += chmc_encint(name_len, p);
		memcpy(p, &entry->name[entry->prefixlen], name_len);
		p += name_len;
		p += chmc_encint(entry->sect_id, p);
		p += chmc_encint(entry->offset, p);
		p += chmc_encint(entry->len, p);

		pmgl->data_len += outlen;
		pmgl->index_len += should_idx;

		chunk->entries_count++;
		chunk->header.free_space -= outlen;
		break;

	} while (1);

	return CHMC_NOERR;
}

struct chmcSection *chmc_section_lookup(struct chmcFile *chm, int id)
{
	struct chmcSection *current;
	struct list_head *pos;
	int i;

	assert(chm);

	i = 0;
	list_for_each(pos, &chm->sections_list) {
		current = list_entry(pos, struct chmcSection, list);
		if (i == id)
			return current;
		i++;
	}

	return NULL;
}

struct chmcPmglChunkNode *chmc_pmgl_create(void)
{
	struct chmcPmglChunkNode *node;

	node = malloc(sizeof(struct chmcPmglChunkNode));
	if (node)
		chmc_pmgl_init(node);

	return node;
}

void chmc_pmgl_init(struct chmcPmglChunkNode *node)
{
	struct chmcPmglChunk *chunk;

	assert(node);

	node->data_len = 0;
	node->index_len = 0;

	chunk = &node->chunk;

	memcpy(chunk->header.signature, "PMGL", 4);

	// FIXME check it is the right len
	chunk->header.free_space = CHMC_PMGL_DATA_LEN + 2;
	chunk->header.unknown_0008 = 0;
	chunk->header.block_prev = -1;
	chunk->header.block_next = -1;

	memset(chunk->data, 0, CHMC_PMGL_DATA_LEN);
}

void chmc_pmgi_init(struct chmcPmgiChunkNode *node)
{
	struct chmcPmgiChunk *chunk;

	assert(node);

	node->data_len = 0;
	node->index_len = 0;

	chunk = &node->chunk;

	memcpy(chunk->header.signature, "PMGI", 4);

	// FIXME check it is the right len
	chunk->header.free_space = CHMC_PMGI_DATA_LEN + 2;
	//  chunk->header.unknown_0008 = 0;
	//  chunk->header.block_prev = -1;
	//  chunk->header.block_next = -1;

	memset(chunk->data, 0, CHMC_PMGI_DATA_LEN);
}



struct chmcPmgiChunkNode *chmc_pmgi_create(void)
{
	struct chmcPmgiChunkNode *node;

	node = malloc(sizeof(struct chmcPmgiChunkNode));
	if (node)
		chmc_pmgi_init(node);

	return node;
}

void chmc_pmgl_destroy(struct chmcPmglChunkNode *node)
{
	assert(node);
	free(node);
}

void chmc_pmgi_destroy(struct chmcPmgiChunkNode *node)
{
	assert(node);
	free(node);
}

void chmc_pmgl_add(struct chmcFile *chm, struct chmcPmglChunkNode *pmgl)
{
	struct chmcItspHeader *itsp = &chm->itsp;
	struct chmcPmglHeader *hdr;

	assert(chm);
	assert(pmgl);

	list_add_tail(&pmgl->list, &chm->pmgl_list);

	itsp->index_last = itsp->num_blocks;

	hdr = &pmgl->chunk.header;
	hdr->block_prev = itsp->num_blocks - 1;

	if (chm->pmgl_last) {
		hdr = &chm->pmgl_last->chunk.header;
		hdr->block_next = itsp->num_blocks;
	}

	itsp->num_blocks++;

	chm->pmgl_last = pmgl;
}

int chmc_pmgi_done(struct chmcFile *chm)
{
	struct chmcItspHeader *itsp = &chm->itsp;
	struct chmcPmglChunkNode *pmgl;
	struct list_head *pos;

	int i, j;
	char name[256]; //FIXME use malloc
	UInt32 name_len;

	assert(chm);

	// only one pml, omitted pmgi
	if (itsp->num_blocks == 1) {
		itsp->index_depth = 1;
		itsp->index_root = -1;
		itsp->index_last = 0;
		return CHMC_NOERR;
	}

	itsp->index_root = itsp->num_blocks;

	i = 0;
	list_for_each(pos, &chm->pmgl_list) {
		pmgl = list_entry(pos, struct chmcPmglChunkNode, list);
		j = chmc_decint(&pmgl->chunk.data[0], &name_len);
		if (name_len <= 255) {
			memcpy(name, &pmgl->chunk.data[j], name_len);
			name[name_len] = '\0';
			chmc_pmgi_add_entry(chm, name, i);
		}
		else
			BUG_ON("name_len >= 255(%lu) %.*s\n", name_len, 255,
			       &pmgl->chunk.data[j]);
		i++;
	}

	return CHMC_NOERR;
}

int chmc_pmgi_add_entry(struct chmcFile *chm, const char *name, int pmgl_id)
{
	struct chmcPmgiChunkNode *pmgi;
	struct chmcPmgiChunk *chunk;
	struct chmcItspHeader *itsp = &chm->itsp;

	UChar *p;
	UInt16 *idx;
	int name_len;
	int outlen;
	int should_idx, idx_intlv;
	int free;

	assert(chm);

	// check chunk space for new entry
	name_len = strlen(name);

	outlen = chmc_encint_len(name_len);
	outlen += name_len;
	outlen += chmc_encint_len(pmgl_id);

	// look for current pmgi chunk, create if doesn't exist
	if (!chm->pmgi_last) {
		pmgi = chmc_pmgi_create();
		if (pmgi)
			chmc_pmgi_add(chm, pmgi);
		else
			chmcerr_set_return(CHMC_ENOMEM, "pmgi chunk: ");
	}
	else
		pmgi = chm->pmgi_last;

	do {

		chunk = &chm->pmgi_last->chunk;

		idx_intlv = 1 + ( 1 << itsp->blockidx_intvl );
		should_idx = ( ( chunk->entries_count > 0 )
		               && ! ( ( chunk->entries_count + 1 ) % idx_intlv )
		               ? 2 : 0 );

		free = sizeof(chunk->data) - pmgi->data_len -
			pmgi->index_len - should_idx;

		// current(last) chunk doesn't have enough room? force new one
		if (outlen + should_idx > free) {
			pmgi = chmc_pmgi_create();
			if (pmgi)
				chmc_pmgi_add(chm, pmgi);
			else
				chmcerr_set_return(CHMC_ENOMEM, "pmgi chunk: ");

			continue;
		}

		p = (void *)&chunk->data[pmgi->data_len];

		if (should_idx) {
			idx = (void *)&chunk->data[CHMC_PMGI_DATA_LEN] - pmgi->index_len;
			*idx = (void *)p - (void *)&chunk->data;
		}

		p += chmc_encint(name_len, p);
		memcpy(p, name, name_len);
		p += name_len;
		p += chmc_encint(pmgl_id, p);

		pmgi->data_len += outlen;
		pmgi->index_len += should_idx;

		chunk->entries_count++;
		chunk->header.free_space -= outlen;
		break;

	} while (1);

	return CHMC_NOERR;
}

void chmc_pmgi_add(struct chmcFile *chm, struct chmcPmgiChunkNode *pmgi)
{
	struct chmcItspHeader *itsp = &chm->itsp;

	assert(chm);
	assert(pmgi);

	list_add_tail(&pmgi->list, &chm->pmgi_list);
	itsp->num_blocks++;

	chm->pmgi_last = pmgi;
}

int chmc_write(struct chmcFile *chm)
{
	struct chmcItsfHeader *itsf = &chm->itsf;
	struct chmcSect0 *sect0 = &chm->sect0;
	struct chmcItspHeader *itsp = &chm->itsp;

	struct chmcPmglChunkNode *pmgl;
	struct chmcPmgiChunkNode *pmgi;
	struct list_head *pos;

	assert(chm);

	write(chm->fd, itsf, _CHMC_ITSF_V3_LEN);
	write(chm->fd, sect0, _CHMC_SECT0_LEN);
	write(chm->fd, itsp, _CHMC_ITSP_V1_LEN);

	list_for_each(pos, &chm->pmgl_list) {
		pmgl = list_entry(pos, struct chmcPmglChunkNode, list);
		write(chm->fd, &pmgl->chunk, _CHMC_CHUNK_LEN);
	}

	if (itsp->num_blocks > 1) {
		list_for_each( pos, &chm->pmgi_list ) {
			pmgi = list_entry(pos, struct chmcPmgiChunkNode, list);
			write(chm->fd, &pmgi->chunk, _CHMC_CHUNK_LEN);
		}
	}

	return CHMC_NOERR;
}

int chmc_appendfile(struct chmcFile *chm, const char *filename, void *buf,
                size_t size )
{
	struct stat statbuf;
	int in;
	off_t todo, toread;
	ssize_t rx;

	if (stat(filename, &statbuf) < 0)
		return errno;

	in = open(filename, O_RDONLY);
	if (in >= 0) {
		todo = statbuf.st_size;

		while (todo) {
			toread = size;
			if (toread > todo)
				toread = todo;

			rx = read(in, buf, toread);
			if (rx > 0) {
				write(chm->fd, buf, rx);
				todo -= rx;
			}
		}

		close(in);
	}
	else
		BUG_ON("open %s\n", filename);

	return CHMC_NOERR;
}
