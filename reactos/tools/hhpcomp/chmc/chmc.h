/*

  Copyright (C) 2010 Alex Andreotti <alex.andreotti@gmail.com>

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
#ifndef CHMC_CHMC_H
#define CHMC_CHMC_H

#include <stdlib.h>
#include <limits.h>

#include "chm.h"
#include "list.h"

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

#define CHMC_DIR_UUID \
	"\x10\xfd\x01\x7c\xaa\x7b\xd0\x11\x9e\x0c\x00\xa0\xc9\x22\xe6\xec"
#define CHMC_STREAM_UUID \
	"\x11\xfd\x01\x7c\xaa\x7b\xd0\x11\x9e\x0c\x00\xa0\xc9\x22\xe6\xec"
#define CHMC_SYSTEM_UUID \
	"\x6a\x92\x02\x5d\x2e\x21\xd0\x11\x9d\xf9\x00\xa0\xc9\x22\xe6\xec"

struct chmcIndexHeader {
	char signature[4];
	Int32 unknown_4;
	Int32 unknown_8;
	Int32 num_of_topic;
	Int32 unknown_10;
	Int32 off_img_list;
	Int32 unknown_18;
	Int32 img_type_folder;
	Int32 background;
	Int32 foreground;
	Int32 off_font;
	Int32 win_style;
	Int32 ex_win_style;
	Int32 unknown_34;
	Int32 off_frame_name;
	Int32 off_win_name;
	Int32 num_of_info;
	Int32 unknown_44;
	Int32 num_of_merge_files;
	Int32 unknown_4c;
	Int32 merge_files_offs[1004];
};

/* Sys Info Entry codes */
#define SIEC_DEFTOPIC   2
#define SIEC_TITLE      3
#define SIEC_LCASEFILE  6
#define SIEC_DEFWINDOW  5

/* present in files with Binary Index turned on. (eg: af 08 63 ac)
   The entry in the #URLTBL file that points to the sitemap index had
   the same first DWORD */
#define SIEC_HAVE_BINDX 7
#define SIEC_NUMOFINFOT 12

/* The #IDXHDR file contains exactly the same bytes (len 4096) */
#define SIEC_IDXHDR     13

#define SIEC_INFOCHKSUM 15
#define SIEC_DEFFONT 16

#define SIEC_TIMESTAMP 10
#define SIEC_COMPVER 9
#define SIEC_SYSINFO 4

/* NOTE use only as pointer */
#define _CHMC_SYS_ENTRY_HDR_LEN (sizeof(UInt16)*2)
struct chmcSystemEntry {
	UInt16 code; /* FIXME check unsigned */
	UInt16 len; /* FIXME check unsigned */
	UChar data[65535];
};

/* NOTE use only as pointer */
#define _CHMC_SYS_ENTRY_NODE_HDR_LEN \
	(sizeof(struct chmcSystemEntryNode *)+_CHMC_SYS_ENTRY_HDR_LEN)

struct chmcSystemEntryNode {
	struct chmcSystemEntryNode *next;
	struct chmcSystemEntry entry;
};

/* HHA Version 4.72.7294 and earlier */
#define _CHMC_SYS_INFO_V4_72_7294_LEN (28)
/* HHA Version 4.72.8086 and later */
#define _CHMC_SYS_INFO_V4_72_8086_LEN (36)
struct chmcSystemInfo {
	UInt32 lcid;
	UInt32 dbcs;
	UInt32 full_search;
	UInt32 klinks;
	UInt32 alinks;
	UInt64 timestamp;
	UInt32 unknown_1c; // >= 8086 only
	UInt32 unknown_20; // >= 8086 only
};


/* /usr/include/freetype2/freetype/ttnameid.h maybe useful */
#define CHMC_MS_LCID_EN_US (0x0409)

#define _CHMC_SYSTEM_HDR_LEN (sizeof(Int32)+sizeof(struct chmcSystemInfo))
struct chmcSystem {
	Int32 version;
	struct chmcSystemInfo info;

/* private: */
	struct chmcSystemEntryNode *_entries;
	UInt32 _size; /* keep track for alloc before save */
};

#define _CHMC_CHUNK_LEN (4096)
#define CHMC_PMGL_DATA_LEN (_CHMC_CHUNK_LEN - _CHMC_PMGL_LEN - 2)

struct chmcPmglChunk {
	struct chmcPmglHeader header;
	UChar data[CHMC_PMGL_DATA_LEN];
	UInt16 entries_count;
};

struct chmcPmglChunkNode {
	struct list_head list;
	int data_len;
	int index_len;
	struct chmcPmglChunk chunk;
};

#define CHMC_PMGI_DATA_LEN (_CHMC_CHUNK_LEN - _CHMC_PMGI_LEN - 2)

struct chmcPmgiChunk {
	struct chmcPmgiHeader header;
	UChar data[CHMC_PMGI_DATA_LEN];
	UInt16 entries_count;
};

struct chmcPmgiChunkNode {
	struct list_head list;
	int data_len;
	int index_len;
	struct chmcPmgiChunk chunk;
};

#define CHMC_TNFL_STATIC (1 << 0) /* don't free() */

struct chmcTreeNode {
	struct list_head list;
	UInt32 flags;
	UInt32 sect_id;
	char *name;
	UInt16 prefixlen;
	UChar *buf;
	UInt64 offset;
	UInt64 len;
};

struct chmcStringChunk {
	struct list_head list;
	UInt16 used;
	UChar data[4096];
};

struct chmcConfig {
	const char *title;
	const char *tmpdir;
	const char *hhc;
	const char *hhk;
	const char *deftopic;
	UInt16 language;
};

struct chmcFile {
	int fd;
	struct chmcItsfHeader itsf;
	struct chmcSect0 sect0;
	struct chmcItspHeader itsp;
	int sections_num;
	struct list_head sections_list;
	struct chmcSection **sections;
	struct list_head pmgl_list;
	struct chmcPmglChunkNode *pmgl_last;
	struct list_head entries_list;
	int entries_num;
	struct chmcTreeNode **sort_entries;
	struct list_head pmgi_list;
	struct chmcPmgiChunkNode *pmgi_last;
	struct chmcSystem system;
	struct chmcIndexHeader idxhdr;
	UChar *strings;
	UInt32 strings_offset;
	UInt32 strings_len;
	struct chmcConfig *config;
};

#define CHMC_SECTNAME_MAXLEN (64)

struct chmcSection {
	struct list_head list;
	char name[CHMC_SECTNAME_MAXLEN];
	UInt64 offset;
	UInt64 len;
	char filename[PATH_MAX];
	int fd;
	struct chmcLzxcResetTable reset_table_header;
	struct chmcLzxcControlData control_data;
	struct list_head mark_list;
	int mark_count;
};

#define _CHMC_RSTTBL_MARK (sizeof(struct chmcResetTableMark))

struct chmcResetTableMark {
	UInt64 at;
	struct list_head list;
};

struct chmcUrlStrEntry {
	UInt32 url_offset;
	UInt32 framename_offset;
};

struct chmcUtlTblEntry {
	UInt32 unknown;
	UInt32 topic_index;
	UInt32 urlstr_offset;
};

struct chmcTopicEntry {
	UInt32 tocidx_offset;
	UInt32 strings_offset;
	UInt32 urltbl_offset;
	short in_content;
	short unknown;
};


int chmc_init(struct chmcFile *chm, const char *filename,
              struct chmcConfig *config);
void chmc_sections_done(struct chmcFile *chm);
void chmc_term(struct chmcFile *chm);
int chmc_tree_done(struct chmcFile *chm);

#define chmc_dump(fmt, ...) fprintf(stderr, fmt , ## __VA_ARGS__)

#endif /* CHMC_CHMC_H */
