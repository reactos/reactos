#include "fitz-base.h"
#include "fitz-stream.h"

/* TODO: complete rewrite with error checking and use fitz memctx */

/*
<rillian> so to use a global_ctx, you run your global data through a normal ctx
<rillian> then call jbig2_make_global_ctx with the normal context
<rillian> that does the (currently null) conversion
<maskros> make_global_ctx after i fed it all the global stream data?
<rillian> maskros: yes
<rillian> and you pass the new global ctx object to jbig2_ctx_new() when you
+create the per-page ctx
*/

#ifdef WIN32 /* Microsoft Visual C++ */

typedef signed char             int8_t;
typedef short int               int16_t;
typedef int                     int32_t;
typedef __int64                 int64_t;

typedef unsigned char             uint8_t;
typedef unsigned short int        uint16_t;
typedef unsigned int              uint32_t;

#else
#include <inttypes.h>
#endif

#include <jbig2.h>

typedef struct fz_jbig2d_s fz_jbig2d;

struct fz_jbig2d_s
{
	fz_filter super;
	Jbig2Ctx *ctx;
	Jbig2GlobalCtx *gctx;
	Jbig2Image *page;
	int idx;
};

fz_error *
fz_newjbig2d(fz_filter **fp, fz_obj *params)
{
	FZ_NEWFILTER(fz_jbig2d, d, jbig2d);
	d->ctx = jbig2_ctx_new(nil, JBIG2_OPTIONS_EMBEDDED, nil, nil, nil);
	d->page = nil;
	d->idx = 0;
	return nil;
}

void
fz_dropjbig2d(fz_filter *filter)
{
	fz_jbig2d *d = (fz_jbig2d*)filter;
	jbig2_ctx_free(d->ctx);
}

fz_error *
fz_setjbig2dglobalstream(fz_filter *filter, unsigned char *buf, int len)
{
	fz_jbig2d *d = (fz_jbig2d*)filter;
	jbig2_data_in(d->ctx, buf, len);
	d->gctx = jbig2_make_global_ctx(d->ctx);
	d->ctx = jbig2_ctx_new(nil, JBIG2_OPTIONS_EMBEDDED, d->gctx, nil, nil);
	return nil;
}

fz_error *
fz_processjbig2d(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	fz_jbig2d *d = (fz_jbig2d*)filter;
	int len;
	int i;

	while (1)
	{
		if (in->rp == in->wp) {
			if (!in->eof)
				return fz_ioneedin;

			if (!d->page) {
				jbig2_complete_page(d->ctx);
				d->page = jbig2_page_out(d->ctx);
			}

			if (out->wp == out->ep)
				return fz_ioneedout;

			len = out->ep - out->wp;
			if (d->idx + len > d->page->height * d->page->stride)
				len = d->page->height * d->page->stride - d->idx;

			/* XXX memcpy(out->wp, d->page->data + d->idx, len); */
			for (i = 0; i < len; i++)
				out->wp[i] = ~ d->page->data[d->idx + i];

			out->wp += len;
			d->idx += len;

			if (d->idx == d->page->height * d->page->stride) {
				jbig2_release_page(d->ctx, d->page);
				out->eof = 1;
				return fz_iodone;
			}
		}
		else {
			len = in->wp - in->rp;
			jbig2_data_in(d->ctx, in->rp, len);
			in->rp += len;
		}
	}
}

