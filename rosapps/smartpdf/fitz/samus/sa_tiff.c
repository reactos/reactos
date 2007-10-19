/*
 * Minimal TIFF image loader. Baseline TIFF 6.0 with CMYK support.
 * Limited bit depth and extra samples support, as per Metro.
 */

#include "fitz.h"
#include "samus.h"

typedef struct sa_tiff_s sa_tiff;

struct sa_tiff_s
{
	/* file and byte order */
	fz_stream *file;
	unsigned order;

	/* where we can find the strips of image data */
	unsigned rowsperstrip;
	unsigned *stripoffsets;
	unsigned *stripbytecounts;

	/* colormap */
	unsigned *colormap;

	/* assorted tags */
	unsigned subfiletype;
	unsigned photometric;
	unsigned compression;
	unsigned imagewidth;
	unsigned imagelength;
	unsigned samplesperpixel;
	unsigned bitspersample;
	unsigned planar;
	unsigned xresolution;
	unsigned yresolution;
	unsigned resolutionunit;
	unsigned fillorder;
	unsigned g3opts;
	unsigned g4opts;
};

enum
{
	TII = 0x4949, /* 'II' */
	TMM = 0x4d4d, /* 'MM' */
	TBYTE = 1,
	TASCII = 2,
	TSHORT = 3,
	TLONG = 4,
	TRATIONAL = 5
};

#define NewSubfileType				254
#define ImageWidth					256
#define ImageLength					257
#define BitsPerSample				258
#define Compression					259
#define PhotometricInterpretation	262
#define FillOrder					266
#define StripOffsets				273
#define SamplesPerPixel				277
#define RowsPerStrip				278
#define StripByteCounts				279
#define XResolution					282
#define YResolution					283
#define PlanarConfiguration			284
#define T4Options					292
#define T6Options					293
#define ResolutionUnit				296
#define ColorMap					320

static const unsigned char bitrev[256] =
{
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

static inline unsigned readshort(sa_tiff *tiff)
{
	unsigned a = fz_readbyte(tiff->file);
	unsigned b = fz_readbyte(tiff->file);
	if (tiff->order == TII)
		return (b << 8) | a;
	return (a << 8) | b;
}

static inline unsigned readlong(sa_tiff *tiff)
{
	unsigned a = fz_readbyte(tiff->file);
	unsigned b = fz_readbyte(tiff->file);
	unsigned c = fz_readbyte(tiff->file);
	unsigned d = fz_readbyte(tiff->file);
	if (tiff->order == TII)
		return (d << 24) | (c << 16) | (b << 8) | a;
	return (a << 24) | (b << 16) | (c << 8) | d;
}

static void
readtagval(unsigned *p, sa_tiff *tiff, unsigned type, unsigned ofs, unsigned n)
{
	fz_seek(tiff->file, ofs, 0);
	while (n--)
	{
		switch (type)
		{
		case TRATIONAL:
			*p = readlong(tiff);
			*p = *p / readlong(tiff);
			p ++;
			break;
		case TBYTE: *p++ = fz_readbyte(tiff->file); break;
		case TSHORT: *p++ = readshort(tiff); break;
		case TLONG: *p++ = readlong(tiff); break;
		default: *p++ = 0; break;
		}
	}
}

static void
tiffdebug(sa_tiff *tiff)
{
	int i, n;

	printf("TIFF <<\n");
	printf("\t/NewSubfileType %u\n", tiff->subfiletype);
	printf("\t/PhotometricInterpretation %u\n", tiff->photometric);
	printf("\t/Compression %u\n", tiff->compression);
	printf("\t/ImageWidth %u\n", tiff->imagewidth);
	printf("\t/ImageLength %u\n", tiff->imagelength);
	printf("\t/BitsPerSample %u\n", tiff->bitspersample);
	printf("\t/SamplesPerPixel %u\n", tiff->samplesperpixel);
	printf("\t/PlanarConfiguration %u\n", tiff->planar);
	printf("\t/XResolution %u\n", tiff->xresolution);
	printf("\t/YResolution %u\n", tiff->yresolution);
	printf("\t/ResolutionUnit %u\n", tiff->resolutionunit);
	printf("\t/FillOrder %u\n", tiff->fillorder);
	printf("\t/T4Options %u\n", tiff->g3opts);
	printf("\t/T6Options %u\n", tiff->g4opts);

	printf("\t/ColorMap $%p\n", tiff->colormap);

	n = (tiff->imagelength + tiff->rowsperstrip - 1) / tiff->rowsperstrip;

	printf("\t/RowsPerStrip %u\n", tiff->rowsperstrip);

	if (tiff->stripoffsets)
	{
		printf("\t/StripOffsets [\n");
		for (i = 0; i < n; i++)
			printf("\t\t%u\n", tiff->stripoffsets[i]);
		printf("\t]\n");
	}

	if (tiff->stripbytecounts)
	{
		printf("\t/StripByteCounts [\n");
		for (i = 0; i < n; i++)
			printf("\t\t%u\n", tiff->stripbytecounts[i]);
		printf("\t]\n");
	}

	printf(">>\n");
}

static fz_error *
tiffreaduncompressed(sa_tiff *tiff, unsigned char *mem, unsigned len)
{
	printf("uncompressed %d bytes\n", len);
	return nil;
}

static fz_error *
tiffreadfiltered(sa_tiff *tiff,
		unsigned char *mem, unsigned len, fz_filter *filter)
{
	fz_error *error;
	fz_buffer *buf;
	fz_buffer *out;
	int count = 0;

	printf("compressed %d bytes\n", len);

	error = fz_newbufferwithmemory(&buf, mem, len);
	if (error)
		return error;

	error = fz_newbuffer(&out, FZ_BUFSIZE);
	if (error)
	{
		fz_dropbuffer(buf);
		return error;
	}

	buf->eof = 1;

	while (1)
	{
		error = fz_process(filter, buf, out);

		printf("  + %d bytes\n", out->wp - out->rp);

		count += out->wp - out->rp;

		if (error == fz_ioneedin)
		{
			error = fz_throw("ioerror: premature eof in filter");
			goto cleanup;
		}
		else if (error == fz_ioneedout)
		{
			if (out->wp - out->rp == 0)
			{
				error = fz_growbuffer(out);
				if (error)
					goto cleanup;
			}
			out->rp = out->bp;
			out->wp = out->bp;
		}
		else if (error == fz_iodone)
		{
			break;
		}
		else
			goto cleanup;
	}

	printf("  = %d bytes\n", count);

	return nil;

cleanup:
printf("cleanup\n");
	fz_dropbuffer(out);
	fz_dropbuffer(buf);
	return error;
}

static fz_error *
tiffreadpackbits(sa_tiff *tiff, unsigned char *mem, unsigned len)
{
	fz_error *error;
	fz_filter *filter;

	error = fz_newrld(&filter, 0);
	if (error)
		return error;

	error = tiffreadfiltered(tiff, mem, len, filter);
	fz_dropfilter(filter);
	return error;
}

static fz_error *
tiffreadfax(sa_tiff *tiff, unsigned char *mem, unsigned len, int comp)
{
	fz_error *error;
	fz_filter *filter;
	fz_obj *params;

	switch (comp)
	{
	default:
	case 2:
		error = fz_packobj(&params,
				"<< /EncodedByteAlign true /EndOfLine false /EndOfBlock false "
				"/K 0 /Columns %i /Rows %i /BlackIs1 %b >>",
				tiff->imagewidth,
				tiff->imagelength,
				tiff->photometric == 0);
		break;
	case 3:
		error = fz_packobj(&params,
				"<< /EndOfLine false /EndOfBlock false "
				"/K %i /Columns %i /Rows %i /BlackIs1 %b >>",
				(tiff->g3opts & 1) ? tiff->rowsperstrip : 0,
				tiff->imagewidth,
				tiff->imagelength,
				tiff->photometric == 0);
		break;
	case 4:
		error = fz_packobj(&params,
				"<< /EndOfLine false /EndOfBlock false "
				"/K -1 /Columns %i /Rows %i /BlackIs1 %b >>",
				tiff->imagewidth,
				tiff->imagelength,
				tiff->photometric == 0);
		break;
	}

	if (error)
		return error;

	error = fz_newfaxd(&filter, params);
	fz_dropobj(params);
	if (error)
		return error;

	error = tiffreadfiltered(tiff, mem, len, filter);
	fz_dropfilter(filter);
	return error;
}

static fz_error *
tiffreadlzw(sa_tiff *tiff, unsigned char *mem, unsigned len)
{
	fz_error *error;
	fz_filter *filter;
	fz_obj *params;

	error = fz_packobj(&params, "<</EarlyChange 0>>");
	if (error)
		return error;

	error = fz_newlzwd(&filter, params);
	fz_dropobj(params);
	if (error)
		return error;

	error = tiffreadfiltered(tiff, mem, len, filter);
	fz_dropfilter(filter);
	return error;
}

static fz_error *
tiffreadstrips(sa_tiff *tiff)
{
	/* switch on compression to create a filter */
	/* feed each strip to the filter */
	/* read out the data and pack the samples into an sa_image */

	/* type 32773 / packbits -- nothing special (same row-padding as PDF) */
	/* type 2 / ccitt rle -- no EOL, no RTC, rows are byte-aligned */
	/* type 3 and 4 / g3 and g4 -- each strip starts new section */
	/* type 5 / lzw -- each strip is handled separately */

	fz_error *error;
	unsigned char *mem;

	int row;
	int strip;
	int len;
	int i;

printf("TIFF ");
printf("w=%d h=%d n=%d bpc=%d ",
		tiff->imagewidth, tiff->imagelength,
		tiff->samplesperpixel, tiff->bitspersample);

	switch (tiff->photometric)
	{
	case 0: printf("WhiteIsZero "); break;
	case 1: printf("BlackIsZero "); break;
	case 2: printf("RGB "); break;
	case 3: printf("RGBPal "); break;
	case 5: printf("CMYK "); break;
	default: return fz_throw("ioerror: unknown TIFF color space: %d", tiff->photometric);
	}

	switch (tiff->compression)
	{
	case 1: printf("Uncompressed "); break;
	case 2: printf("CCITT "); break;
	case 3: printf("FaxG3"); break;
	case 4: printf("FaxG4"); break;
	case 5: printf("LZW "); break;
	case 32773: printf("PackBits "); break;
	default:
		return fz_throw("ioerror: unknown TIFF compression: %d", tiff->compression);
	}

	printf("\n");

	strip = 0;
	for (row = 0; row < tiff->imagelength; row += tiff->rowsperstrip)
	{
		unsigned offset = tiff->stripoffsets[strip];
		unsigned bytecount = tiff->stripbytecounts[strip];

		mem = fz_malloc(bytecount);
		if (!mem)
			return fz_outofmem;

		fz_seek(tiff->file, offset, 0);
		len = fz_read(tiff->file, mem, bytecount);
		if (len < 0)
		{
			fz_free(mem);
			return fz_ioerror(tiff->file);
		}

		if (tiff->fillorder == 2)
			for (i = 0; i < len; i++)
				mem[i] = bitrev[mem[i]];

		switch (tiff->compression)
		{
		case 1: error = tiffreaduncompressed(tiff, mem, len); break;
		case 2: error = tiffreadfax(tiff, mem, len, 2); break;
		case 3: error = tiffreadfax(tiff, mem, len, 3); break;
		case 4: error = tiffreadfax(tiff, mem, len, 4); break;
		case 5: error = tiffreadlzw(tiff, mem, len); break;
		case 32773: error = tiffreadpackbits(tiff, mem, len); break;
		}

		fz_free(mem);

		if (error)
			return error;

		strip ++;
	}

	if (tiff->photometric == 3 && tiff->colormap)
	{
		/* TODO expand RGBPal datain buf via colormap to output */
	}
	else
	{
		/* TODO copy buf to output */
	}

	return nil;
}

static fz_error *
tiffreadtag(sa_tiff *tiff, unsigned offset)
{
	unsigned tag;
	unsigned type;
	unsigned count;
	unsigned value;

	fz_seek(tiff->file, offset, 0);
	tag = readshort(tiff);
	type = readshort(tiff);
	count = readlong(tiff);

	if ((type == TBYTE && count <= 4) ||
			(type == TSHORT && count <= 2) ||
			(type == TLONG && count <= 1))
		value = fz_tell(tiff->file);
	else
		value = readlong(tiff);

	switch (tag)
	{
	case NewSubfileType:
		readtagval(&tiff->subfiletype, tiff, type, value, 1);
		break;
	case ImageWidth:
		readtagval(&tiff->imagewidth, tiff, type, value, 1);
		break;
	case ImageLength:
		readtagval(&tiff->imagelength, tiff, type, value, 1);
		break;
	case BitsPerSample:
		readtagval(&tiff->bitspersample, tiff, type, value, 1);
		break;
	case Compression:
		readtagval(&tiff->compression, tiff, type, value, 1);
		break;
	case PhotometricInterpretation:
		readtagval(&tiff->photometric, tiff, type, value, 1);
		break;
	case FillOrder:
		readtagval(&tiff->fillorder, tiff, type, value, 1);
		break;
	case SamplesPerPixel:
		readtagval(&tiff->samplesperpixel, tiff, type, value, 1);
		break;
	case RowsPerStrip:
		readtagval(&tiff->rowsperstrip, tiff, type, value, 1);
		break;
	case XResolution:
		readtagval(&tiff->xresolution, tiff, type, value, 1);
		break;
	case YResolution:
		readtagval(&tiff->yresolution, tiff, type, value, 1);
		break;
	case PlanarConfiguration:
		readtagval(&tiff->planar, tiff, type, value, 1);
		break;
	case T4Options:
		readtagval(&tiff->g3opts, tiff, type, value, 1);
		break;
	case T6Options:
		readtagval(&tiff->g4opts, tiff, type, value, 1);
		break;
	case ResolutionUnit:
		readtagval(&tiff->resolutionunit, tiff, type, value, 1);
		break;

	case StripOffsets:
		tiff->stripoffsets = fz_malloc(count * sizeof(unsigned));
		if (!tiff->stripoffsets)
			return fz_outofmem;
		readtagval(tiff->stripoffsets, tiff, type, value, count);
		break;

	case StripByteCounts:
		tiff->stripbytecounts = fz_malloc(count * sizeof(unsigned));
		if (!tiff->stripbytecounts)
			return fz_outofmem;
		readtagval(tiff->stripbytecounts, tiff, type, value, count);
		break;

	case ColorMap:
		tiff->colormap = fz_malloc(count * sizeof(unsigned));
		if (!tiff->colormap)
			return fz_outofmem;
		readtagval(tiff->colormap, tiff, type, value, count);
		break;

	default:
		/*
		printf("unknown tag: %d t=%d n=%d\n", tag, type, count);
		*/
		break;
	}

	return nil;
}

static fz_error *
tiffreadifd(sa_tiff *tiff, unsigned offset)
{
	fz_error *error;
	unsigned count;
	unsigned i;

	fz_seek(tiff->file, offset, 0);
	count = readshort(tiff);

	offset += 2;
	for (i = 0; i < count; i++)
	{
		error = tiffreadtag(tiff, offset);
		if (error)
			return error;
		offset += 12;
	}

	if (getenv("TIFFDEBUG"))
		tiffdebug(tiff);

	return tiffreadstrips(tiff);
}

static fz_error *
tiffreadifh(sa_tiff *tiff, fz_stream *file)
{
	unsigned version;
	unsigned offset;

	memset(tiff, 0, sizeof(sa_tiff));
	tiff->file = file;

	/* tag defaults, where applicable */
	tiff->bitspersample = 1;
	tiff->compression = 1;
	tiff->samplesperpixel = 1;
	tiff->resolutionunit = 2;
	tiff->rowsperstrip = 0xFFFFFFFF;
	tiff->fillorder = 1;
	tiff->planar = 1;
	tiff->subfiletype = 0;

	/* get byte order marker */
	tiff->order = TII;
	tiff->order = readshort(tiff);
	if (tiff->order != TII && tiff->order != TMM)
		return fz_throw("ioerror: not a TIFF file");

	/* check version */
	version = readshort(tiff);
	if (version != 42)
		return fz_throw("ioerror: not a TIFF file");

	/* get offset of IFD and then read it */
	offset = readlong(tiff);
	return tiffreadifd(tiff, offset);
}

fz_error *
sa_readtiff(fz_stream *file)
{
	fz_error *error;
	fz_buffer *buf;
	fz_stream *newfile;
	sa_tiff tiff;
	int n;

	/* TIFF requires random access. In Metro TIFFs are embedded in ZIP files.
	 * Compressed streams are not seekable, so we copy the data to an
	 * in-memory data buffer instead of reading from the original stream.
	 */

	n = fz_readall(&buf, file);
	if (n < 0)
		return fz_ioerror(file);

printf("readall -> %d\n", n);

	error = fz_openrbuffer(&newfile, buf);
	if (error)
	{
		fz_dropbuffer(buf);
		return error;
	}

	error = tiffreadifh(&tiff, newfile);

	fz_free(tiff.colormap);
	fz_free(tiff.stripoffsets);
	fz_free(tiff.stripbytecounts);

	fz_dropstream(newfile);
	fz_dropbuffer(buf);

	return error;
}

