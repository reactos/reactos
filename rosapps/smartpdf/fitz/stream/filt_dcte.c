#include "fitz-base.h"
#include "fitz-stream.h"

#include "filt_dctc.h"

typedef struct fz_dcte_s fz_dcte;

struct mydstmgr
{
	struct jpeg_destination_mgr super;
	fz_buffer *buf;
};

struct fz_dcte_s
{
	fz_filter super;

	struct jpeg_compress_struct cinfo;
	struct mydstmgr dst;
	struct myerrmgr err;
	int stage;

	int columns;
	int rows;
	int colors;
};

static void myinitdest(j_compress_ptr cinfo) { /* empty */ }
static boolean myemptybuf(j_compress_ptr cinfo) { return FALSE; }
static void mytermdest(j_compress_ptr cinfo) { /* empty */ }

fz_error *
fz_newdcte(fz_filter **fp, fz_obj *params)
{
	fz_error *err;
	fz_obj *obj;
	int i;

	FZ_NEWFILTER(fz_dcte, e, dcte);

	e->stage = 0;

	obj = fz_dictgets(params, "Columns");
	if (!obj) { fz_free(e); return fz_throw("ioerror in dcte: missing Columns parameter"); }
	e->columns = fz_toint(obj);

	obj = fz_dictgets(params, "Rows");
	if (!obj) { fz_free(e); return fz_throw("ioerror in dcte: missing Rows parameter"); }
	e->rows = fz_toint(obj);

	obj = fz_dictgets(params, "Colors");
	if (!obj) { fz_free(e); return fz_throw("ioerror in dcte: missing Colors parameter"); }
	e->colors = fz_toint(obj);

	/* setup error callback first thing */
	myiniterr(&e->err);
	e->cinfo.err = (struct jpeg_error_mgr*) &e->err;

	if (setjmp(e->err.jb)) {
		err = fz_throw("ioerror in dcte: %s", e->err.msg);
		fz_free(e);
		return err;
	}

	jpeg_create_compress(&e->cinfo);

	/* prepare destination manager */
	e->cinfo.dest = (struct jpeg_destination_mgr *) &e->dst;
	e->dst.super.init_destination = myinitdest;
	e->dst.super.empty_output_buffer = myemptybuf;
	e->dst.super.term_destination = mytermdest;

	e->dst.super.next_output_byte = nil;
	e->dst.super.free_in_buffer = 0;

	/* prepare required encoding options */
	e->cinfo.image_width = e->columns;
	e->cinfo.image_height = e->rows;
	e->cinfo.input_components = e->colors;

	switch (e->colors) {
		case 1: e->cinfo.in_color_space = JCS_GRAYSCALE; break;
		case 3: e->cinfo.in_color_space = JCS_RGB; break;
		case 4: e->cinfo.in_color_space = JCS_CMYK; break;
		default: e->cinfo.in_color_space = JCS_UNKNOWN; break;
	}

	jpeg_set_defaults(&e->cinfo);

	/* FIXME check this */
	obj = fz_dictgets(params, "ColorTransform");
	if (obj) {
		int colortransform = fz_toint(obj);
		if (e->colors == 3 && !colortransform)
			jpeg_set_colorspace(&e->cinfo, JCS_RGB);
		if (e->colors == 4 && colortransform)
			jpeg_set_colorspace(&e->cinfo, JCS_YCCK);
		if (e->colors == 4 && !colortransform)
			jpeg_set_colorspace(&e->cinfo, JCS_CMYK);
	}

	obj = fz_dictgets(params, "HSamples");
	if (obj && fz_isarray(obj)) {
		fz_obj *o;
		for (i = 0; i < e->colors; i++) {
			o = fz_arrayget(obj, i);
			e->cinfo.comp_info[i].h_samp_factor = fz_toint(o);
		}
	}

	obj = fz_dictgets(params, "VSamples");
	if (obj && fz_isarray(obj)) {
		fz_obj *o;
		for (i = 0; i < e->colors; i++) {
			o = fz_arrayget(obj, i);
			e->cinfo.comp_info[i].v_samp_factor = fz_toint(o);
		}
	}

	/* TODO: quant-tables and huffman-tables */

	return nil;
}

void
fz_dropdcte(fz_filter *filter)
{
	fz_dcte *e = (fz_dcte*)filter;

	if (setjmp(e->err.jb)) {
		fprintf(stderr, "ioerror in dcte: jpeg_destroy_compress: %s", e->err.msg);
		return;
	}

	jpeg_destroy_compress(&e->cinfo);
}

/* Adobe says zigzag order. IJG > v6a says natural order. */
#if JPEG_LIB_VERSION >= 61
#define unzigzag(x) unzigzagorder[x]
/* zigzag array position of n'th element of natural array order */
static const unsigned char unzigzagorder[] =
{
	0, 1, 5, 6, 14, 15, 27, 28,
	2, 4, 7, 13, 16, 26, 29, 42,
	3, 8, 12, 17, 25, 30, 41, 43,
	9, 11, 18, 24, 31, 40, 44, 53,
	10, 19, 23, 32, 39, 45, 52, 54,
	20, 22, 33, 38, 46, 51, 55, 60,
	21, 34, 37, 47, 50, 56, 59, 61,
	35, 36, 48, 49, 57, 58, 62, 63
};
#else
#define unzigzag(x) (x)
#endif

fz_error *
fz_setquanttables(fz_dcte *e, unsigned int **qtables, int qfactor)
{
	int i, j;
	unsigned int table[64];

	if (setjmp(e->err.jb)) {
		return fz_throw("ioerror in dcte: %s", e->err.msg);
	}

	/* TODO: check for duplicate tables */

	for (i = 0; i < e->colors; i++) {
		for (j = 0; j < 64; j++) {
			table[j] = unzigzag(qtables[i][j]);
		}
		jpeg_add_quant_table(&e->cinfo, i, table, qfactor, TRUE);
		e->cinfo.comp_info[i].quant_tbl_no = i;
	}

	return nil;
}

fz_error *
fz_processdcte(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	fz_dcte *e = (fz_dcte*)filter;
	JSAMPROW scanline[1];
	int stride;
	int i;

	e->dst.buf = out;
	e->dst.super.free_in_buffer = out->ep - out->wp;
	e->dst.super.next_output_byte = out->wp;

	if (setjmp(e->err.jb)) {
		return fz_throw("ioerror in dcte: %s", e->err.msg);
	}

	switch (e->stage)
	{
		case 0:
			/* must have enough space for markers, typically 600 bytes or so */
			if (out->wp + 1024 > out->ep)
				goto needoutput;

			jpeg_start_compress(&e->cinfo, TRUE);

			/* TODO: write Adobe ColorTransform marker */

			/* fall through */
			e->stage = 1;

		case 1:
			stride = e->columns * e->colors;

			while (e->cinfo.next_scanline < e->cinfo.image_height)
			{
				if (in->rp + stride > in->wp)
					goto needinput;

				scanline[0] = in->rp;

				i = jpeg_write_scanlines(&e->cinfo, scanline, 1);

				if (i == 0)
					goto needoutput;

				in->rp += stride;
			}

			/* fall through */
			e->stage = 2;

		case 2:
			/* must have enough space for end markers */
			if (out->wp + 100 > out->ep)
				goto needoutput;

			/* finish compress cannot suspend! */
			jpeg_finish_compress(&e->cinfo);

			e->stage = 3;
			out->eof = 1;
			out->wp = out->ep - e->dst.super.free_in_buffer;
			return fz_iodone;
	}

needinput:
	out->wp = out->ep - e->dst.super.free_in_buffer;
	return fz_ioneedin;

needoutput:
	out->wp = out->ep - e->dst.super.free_in_buffer;
	return fz_ioneedout;
}

