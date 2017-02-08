/* @(#)stream.c	1.17 15/12/08 Copyright 2002-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)stream.c	1.17 15/12/08 Copyright 2002-2015 J. Schilling";
#endif
/*
 *	ISO-9660 stream (pipe) file module for mkisofs
 *
 *	Copyright (c) 2002-2015 J. Schilling
 *	Implemented after an idea from M.H. Voase
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "mkisofs.h"
#include "iso9660.h"
#include <schily/schily.h>

LOCAL int	size_str_file	__PR((UInt32_t starting_extent));
LOCAL int	size_str_dir	__PR((UInt32_t starting_extent));
LOCAL int	size_str_path	__PR((UInt32_t starting_extent));

LOCAL int	gen_str_path	__PR((void));

LOCAL int	write_str_file	__PR((FILE *outfile));
LOCAL int	write_str_dir	__PR((FILE *outfile));
LOCAL int	write_str_path	__PR((FILE *outfile));

extern int		stream_media_size;
extern char		*stream_filename;
extern time_t		begun;

LOCAL unsigned int	avail_extent;
LOCAL unsigned int	stream_extent;
LOCAL unsigned int	stream_size;
LOCAL unsigned int	stream_pad;
LOCAL char		*l_path;
LOCAL char		*m_path;
LOCAL struct iso_directory_record s_dir;
LOCAL int		stream_finished = 0;

/*
 * Compute the size of the file
 */
LOCAL int
size_str_file(starting_extent)
	UInt32_t	starting_extent;
{
	int	n;
extern	int	dopad;

	stream_extent = last_extent;	/* Start of stream file content */

	avail_extent = stream_media_size;
	n = last_extent;		/* Room for FS blocks before file */
	n += 1;				/* Room for the directory block */
	stream_pad = 0;
	if (n < 50) {
		stream_pad = 50 - n;
		n = 50;			/* Make net. size easy to compute */
	}
	if (dopad)
		n += 150;		/* Room for final padding */

	if (n >= avail_extent) {
		comerrno(EX_BAD,
			_("-stream-media-size %d but must be at least %d\n"),
			avail_extent, n+2);
	}
	avail_extent -= n;

	last_extent += avail_extent + stream_pad;

	return (0);
}

/*
 * The size of the directory record - one sector
 */
LOCAL int
size_str_dir(starting_extent)
	UInt32_t	starting_extent;
{
	root->extent = last_extent;
	last_extent += 1;
	return (0);
}

/*
 * The size of the path tables - two sectors
 */
LOCAL int
size_str_path(starting_extent)
	UInt32_t	starting_extent;
{
	path_table[0] = starting_extent;
	path_table[1] = 0;
	path_table[2] = path_table[0] + 1;
	path_table[3] = 0;
	last_extent += 2 * 1;
	return (0);
}

/*
 * Generate the path table data
 */
LOCAL int
gen_str_path()
{
	/*
	 * Basically add the root directory entry
	 */
	l_path = (char *)e_malloc(SECTOR_SIZE);
	m_path = (char *)e_malloc(SECTOR_SIZE);
	memset(l_path, 0, SECTOR_SIZE);
	memset(m_path, 0, SECTOR_SIZE);
	l_path[0] = 1;
	m_path[0] = 1;
	set_731(l_path + 2, root->extent);
	set_732(m_path + 2, root->extent);
	set_721(l_path + 6, 1);
	set_722(m_path + 6, 1);
	l_path[8] = '\0'; l_path[9] = '\0';
	m_path[8] = '\0'; m_path[9] = '\0';
	return (0);
}

/*
 * Write the file content
 */
LOCAL int
write_str_file(outfile)
	FILE	*outfile;
{
	unsigned int	idx = 0;
	unsigned int	iso_blocks;
	int		count;
	char		*buf;

	buf = e_malloc(SECTOR_SIZE);
	stream_size = 0;
	while ((idx + SECTOR_SIZE) < (avail_extent * SECTOR_SIZE)) {
		memset(buf, 0, SECTOR_SIZE);
		count = fread(buf, 1, SECTOR_SIZE, stdin);
		if (count <= 0) {
			stream_finished = 1;
			break;
		}
		idx += count;
		xfwrite(buf, count, 1, outfile, 0, FALSE);
	}

	stream_size = idx;
	iso_blocks = ISO_BLOCKS(idx);
	memset(buf, 0, SECTOR_SIZE);
	if (SECTOR_SIZE * iso_blocks - idx)
		xfwrite(buf, SECTOR_SIZE * iso_blocks - idx, 1, outfile, 0, FALSE);
	/*
	 * If we didn't fill the available area, pad to directory block
	 */
	for (count = 0; count < (avail_extent - iso_blocks); count++)
		xfwrite(buf, SECTOR_SIZE, 1, outfile, 0, FALSE);

	for (count = 0; count < stream_pad; count++)
		xfwrite(buf, SECTOR_SIZE, 1, outfile, 0, FALSE);

	last_extent_written += avail_extent + stream_pad;
	free(buf);
	return (0);
}

/*
 * Generate and write the directory record data
 */
LOCAL int
write_str_dir(outfile)
	FILE	*outfile;
{
	int	reclen;
	char	*buf;

	buf = e_malloc(SECTOR_SIZE); memset(buf, 0, SECTOR_SIZE);
	memset(&s_dir, 0, sizeof (struct iso_directory_record));
	s_dir.length[0] = 34;			/* BAD: Hardcoded - Will fix, MHV */
	s_dir.ext_attr_length[0] = 0;
	set_733((char *)s_dir.extent, root->extent);
	set_733((char *)s_dir.size, SECTOR_SIZE);
	iso9660_date(s_dir.date, begun);
	s_dir.flags[0] = ISO_DIRECTORY;
	s_dir.file_unit_size[0] = 0;
	s_dir.interleave[0] = 0;
	set_723((char *)s_dir.volume_sequence_number, volume_sequence_number);
	s_dir.name_len[0] = 1;
	s_dir.name[0] = 0;	/* "." */
	xfwrite(&s_dir, offsetof(struct iso_directory_record, name[0]) + 1, 1, outfile, 0, FALSE);
	s_dir.name[0] = 1;	/* ".." */
	xfwrite(&s_dir, offsetof(struct iso_directory_record, name[0]) + 1, 1, outfile, 0, FALSE);
	memset(&s_dir, 0, sizeof (struct iso_directory_record));
	reclen = offsetof(struct iso_directory_record, name[0]) +
				strlen(stream_filename);
	if (reclen & 1)
		reclen++;
	s_dir.length[0] = reclen;
	s_dir.ext_attr_length[0] = 0;
	set_733((char *)s_dir.extent, stream_extent);
	set_733((char *)s_dir.size, stream_size);
	iso9660_date(s_dir.date, begun);
	s_dir.flags[0] = 0;
	s_dir.file_unit_size[0] = 0;
	set_723((char *)s_dir.volume_sequence_number, volume_sequence_number);
	s_dir.name_len[0] = strlen(stream_filename);
	memcpy(s_dir.name, stream_filename, s_dir.name_len[0]);
	xfwrite(&s_dir, reclen, 1, outfile, 0, FALSE);

	/*
	 * This calc is: 2 single char directory entries (34) + an additional entry
	 * with filename length stream_filename + round up for even lenght count
	 */
	xfwrite(buf, SECTOR_SIZE - ((2 * 34) + reclen), 1, outfile, 0, FALSE);
	free(buf);
	last_extent_written++;
	return (0);
}

/*
 * Generate the path table data
 */
LOCAL int
write_str_path(outfile)
	FILE	*outfile;
{
	xfwrite(l_path, SECTOR_SIZE, 1, outfile, 0, FALSE);
	xfwrite(m_path, SECTOR_SIZE, 1, outfile, 0, FALSE);
	last_extent_written += 2;
	free(l_path);
	free(m_path);
	path_table_l = NULL;
	path_table_m = NULL;
	return (0);
}

struct output_fragment strfile_desc  = { NULL, size_str_file, NULL,	   write_str_file, "Stream File" };
struct output_fragment strdir_desc  = { NULL, size_str_dir,  NULL,	   write_str_dir,  "Stream File Directory"  };
struct output_fragment strpath_desc = { NULL, size_str_path, gen_str_path, write_str_path, "Stream File Path table" };
