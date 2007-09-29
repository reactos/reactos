#include "fitz-base.h"
#include "fitz-stream.h"

/* TODO: bpc */

/*
 * We use the Jasper JPEG2000 coder for now.
 */

#include <jasper/jasper.h>

static char *colorspacename(jas_clrspc_t clrspc)
{
	switch (jas_clrspc_fam(clrspc))
	{
	case JAS_CLRSPC_FAM_UNKNOWN: return "Unknown";
	case JAS_CLRSPC_FAM_XYZ: return "XYZ";
	case JAS_CLRSPC_FAM_LAB: return "Lab";
	case JAS_CLRSPC_FAM_GRAY: return "Gray";
	case JAS_CLRSPC_FAM_RGB: return "RGB";
	case JAS_CLRSPC_FAM_YCBCR: return "YCbCr";
	}
	return "Unknown";
}

typedef struct fz_jpxd_s fz_jpxd;

struct fz_jpxd_s
{
	fz_filter super;
	jas_stream_t *stream;
	jas_image_t *image;
	int offset;
	int stage;
};

fz_error *
fz_newjpxd(fz_filter **fp, fz_obj *params)
{
	int err;

	FZ_NEWFILTER(fz_jpxd, d, jpxd);

	err = jas_init();
	if (err) {
		fz_free(d);
		return fz_throw("ioerror in jpxd: jas_init()");
	}

	d->stream = jas_stream_memopen(nil, 0);
	if (!d->stream) {
		fz_free(d);
		return fz_throw("ioerror in jpxd: jas_stream_memopen()");
	}

	d->image = nil;
	d->offset = 0;
	d->stage = 0;

	return nil;
}

void
fz_dropjpxd(fz_filter *filter)
{
	fz_jpxd *d = (fz_jpxd*)filter;
	if (d->stream) jas_stream_close(d->stream);
	if (d->image) jas_image_destroy(d->image);
}

fz_error *
fz_processjpxd(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	fz_jpxd *d = (fz_jpxd*)filter;
	int n, bpc, w, h;
	int i, x, y;

	switch (d->stage)
	{
	case 0: goto input;
	case 1: goto decode;
	case 2: goto output;
	}

input:
	while (in->rp < in->wp) {
		n = jas_stream_write(d->stream, in->rp, in->wp - in->rp);
		in->rp += n;
	}

	if (!in->eof)
		return fz_ioneedin;

	d->stage = 1;

decode:
	jas_stream_seek(d->stream, 0, 0);

	d->image = jas_image_decode(d->stream, -1, 0);
	if (!d->image)
		return fz_throw("ioerror in jpxd: unable to decode image data");

	fprintf(stderr, "P%c\n# JPX %d x %d n=%d bpc=%d colorspace=%04x %s\n%d %d\n%d\n",
		jas_image_numcmpts(d->image) == 1 ? '5' : '6',

		jas_image_width(d->image),
		jas_image_height(d->image),
		jas_image_numcmpts(d->image),
		jas_image_cmptprec(d->image, 0),
		jas_image_clrspc(d->image),
		colorspacename(jas_image_clrspc(d->image)),

		jas_image_width(d->image),
		jas_image_height(d->image),
		(1 << jas_image_cmptprec(d->image, 0)) - 1);

	d->stage = 2;

output:
	w = jas_image_width(d->image);
	h = jas_image_height(d->image);
	n = jas_image_numcmpts(d->image);
	bpc = jas_image_cmptprec(d->image, 0);	/* use precision of first component for all... */

	while (d->offset < w * h)
	{
		y = d->offset / w;
		x = d->offset - y * w;

		/* FIXME bpc != 8 */

		if (out->wp + n >= out->ep)
			return fz_ioneedout;

		for (i = 0; i < n; i++)
			*out->wp++ = jas_image_readcmptsample(d->image, i, x, y);

		d->offset ++;
	}

	out->eof = 1;
	return fz_iodone;
}

