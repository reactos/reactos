/*
 * Support for a subset of PKZIP format v4.5:
 *   - no encryption
 *   - no multi-disk
 *   - only Store and Deflate
 *   - ZIP64 format (long long sizes and offsets) [TODO]
 *
 * TODO for Metro compliance: compare file names by unescaping %XX
 * and then converting to upper-case NFC.
 */

#include "fitz.h"
#include "samus.h"

typedef struct sa_zipent_s sa_zipent;

struct sa_zipent_s
{
	unsigned offset;
	unsigned csize;
	unsigned usize;
	char *name;
};

struct sa_zip_s
{
	fz_stream *file;
	int len;
	sa_zipent *table;
};

static inline unsigned int read2(fz_stream *f)
{
	unsigned char a = fz_readbyte(f);
	unsigned char b = fz_readbyte(f);
	return (b << 8) | a;
}

static inline unsigned int read4(fz_stream *f)
{
	unsigned char a = fz_readbyte(f);
	unsigned char b = fz_readbyte(f);
	unsigned char c = fz_readbyte(f);
	unsigned char d = fz_readbyte(f);
	return (d << 24) | (c << 16) | (b << 8) | a;
}

static fz_error *readzipdir(sa_zip *zip, int startoffset)
{
	unsigned sign;
	unsigned csize, usize;
	unsigned namesize, metasize, comsize;
	unsigned offset;
	int i;

	fz_seek(zip->file, startoffset, 0);

	for (i = 0; i < zip->len; i++)
	{
		sign = read4(zip->file);
		if (sign != 0x02014b50)
			return fz_throw("ioerror: unknown zip signature");

		(void) read2(zip->file);	/* version made by */
		(void) read2(zip->file);	/* version to extract */
		(void) read2(zip->file);	/* general */
		(void) read2(zip->file);	/* method */
		(void) read2(zip->file);	/* last mod file time */
		(void) read2(zip->file);	/* last mod file date */
		(void) read4(zip->file);	/* crc-32 */
		csize = read4(zip->file);
		usize = read4(zip->file);
		namesize = read2(zip->file);
		metasize = read2(zip->file);
		comsize = read2(zip->file);
		(void) read2(zip->file);	/* disk number start */
		(void) read2(zip->file);	/* int file atts */
		(void) read4(zip->file);	/* ext file atts */
		offset = read4(zip->file);

		zip->table[i].offset = offset;
		zip->table[i].csize = csize;
		zip->table[i].usize = usize;

		zip->table[i].name = fz_malloc(namesize + 1);
		if (!zip->table[i].name)
			return fz_outofmem;
		fz_read(zip->file, zip->table[i].name, namesize);
		zip->table[i].name[namesize] = 0;

		fz_seek(zip->file, metasize, 1);
		fz_seek(zip->file, comsize, 1);
	}

	return nil;
}

static fz_error *readzipendofdir(sa_zip *zip, int startoffset)
{
	unsigned sign;
	unsigned count;
	unsigned offset;

	fz_seek(zip->file, startoffset, 0);

	sign = read4(zip->file);
	if (sign != 0x06054b50)
		return fz_throw("ioerror: unknown zip signature");

	(void) read2(zip->file);	/* this disk */
	(void) read2(zip->file);	/* start disk */
	(void) read2(zip->file);	/* ents in this disk */
	count = read2(zip->file);	/* ents in central directory */
	(void) read4(zip->file);	/* size of central directory */
	offset = read4(zip->file);	/* offset to central directory */

	zip->len = count;
	zip->table = fz_malloc(zip->len * sizeof(sa_zipent));
	if (!zip->table)
		return fz_outofmem;

	memset(zip->table, 0, zip->len * sizeof(sa_zipent));

	return readzipdir(zip, offset);
}

static fz_error *findzipendofdir(sa_zip *zip)
{
	char buf[512];
	int back, maxback, filesize;
	int n, i;

	filesize = fz_seek(zip->file, 0, 2);
	if (filesize < 0)
		return fz_ioerror(zip->file);

	maxback = MIN(filesize, 0xFFFF + sizeof buf);
	back = MIN(maxback, sizeof buf);

	while (back < maxback)
	{
		fz_seek(zip->file, filesize - back, 0);
		n = fz_read(zip->file, buf, sizeof buf);
		if (n < 0)
			return fz_ioerror(zip->file);

		for (i = n - 4; i > 0; i--)
			if (!memcmp(buf + i, "\120\113\5\6", 4))
				return readzipendofdir(zip, filesize - back + i);

		back += sizeof buf - 4;
	}

	return fz_throw("ioerror: could not find central directory in zip");
}

/*
 * Open a ZIP archive for reading.
 * Load the table of contents.
 */
fz_error *
sa_openzip(sa_zip **zipp, char *filename)
{
	fz_error *error;
	sa_zip *zip;

	zip = *zipp = fz_malloc(sizeof(sa_zip));
	if (!zip)
		return fz_outofmem;

	zip->file = nil;
	zip->len = 0;
	zip->table = nil;

	error = fz_openrfile(&zip->file, filename);
	if (error)
		return error;

	return findzipendofdir(zip);
}

/*
 * Free the table of contents and close the underlying file.
 */
void
sa_closezip(sa_zip *zip)
{
	int i;

	if (zip->file)
		fz_dropstream(zip->file);

	for (i = 0; i < zip->len; i++)
		if (zip->table[i].name)
			fz_free(zip->table[i].name);

	fz_free(zip->table);
}

/*
 * Print a table of contents of the zip archive
 */
void
sa_debugzip(sa_zip *zip)
{
	int i;

	for (i = 0; i < zip->len; i++)
	{
		printf("%8u ", zip->table[i].usize);
		if (zip->table[i].usize)
			printf("%3d%% ", zip->table[i].csize * 100 / zip->table[i].usize);
		else
			printf(" --- ");
		printf("%s\n", zip->table[i].name);
	}
}

int
sa_accesszipentry(sa_zip *zip, char *name)
{
	int i;
	for (i = 0; i < zip->len; i++)
		if (!sa_strcmp(name, zip->table[i].name))
			return 1;
	return 0;
}

/*
 * Seek and push decoding filter to read an individual file in the zip archive.
 */
static fz_error *reallyopenzipentry(fz_stream **stmp, sa_zip *zip, int idx)
{
	fz_error *error;
	fz_filter *filter;
	fz_obj *obj;
	unsigned sign, version, general, method;
	unsigned csize, usize;
	unsigned namesize, metasize;
	int t;

	t = fz_seek(zip->file, zip->table[idx].offset, 0);
	if (t < 0)
		return fz_ioerror(zip->file);

	sign = read4(zip->file);
	if (sign != 0x04034b50)
		return fz_throw("ioerror: unknown zip signature");

	version = read2(zip->file);
	general = read2(zip->file);
	method = read2(zip->file);
	(void) read2(zip->file);	/* time */
	(void) read2(zip->file);	/* date */
	(void) read4(zip->file);	/* crc-32 */
	csize = read4(zip->file);
	usize = read4(zip->file);
	namesize = read2(zip->file);
	metasize = read2(zip->file);

	if ((version & 0xff) > 45)
		return fz_throw("ioerror: unsupported zip version");

	if (general & 0x0001)
		return fz_throw("ioerror: encrypted zip entry");

	t = fz_seek(zip->file, namesize + metasize, 1);
	if (t < 0)
		return fz_ioerror(zip->file);

	switch (method)
	{	
	case 0:
		error = fz_newnullfilter(&filter, csize);
		if (error)
			return error;
		break;

	case 8:
		error = fz_packobj(&obj, "<</ZIP true>>");
		if (error)
			return error;
		error = fz_newflated(&filter, obj);
		fz_dropobj(obj);
		if (error)
			return error;
		break;

	default:
		return fz_throw("ioerror: unsupported compression method");
		break;
	}

	error = fz_openrfilter(stmp, filter, zip->file);
	fz_dropfilter(filter);
	if (error)
		return error;

	return nil;
}

fz_error *
sa_openzipentry(fz_stream **stmp, sa_zip *zip, char *name)
{
	int i;

	for (i = 0; i < zip->len; i++)
		if (!sa_strcmp(name, zip->table[i].name))
			return reallyopenzipentry(stmp, zip, i);

	return fz_throw("ioerror: file not found in zip: '%s'", name);
}

