#include "fitz-base.h"
#include "fitz-stream.h"

#include <zlib.h>

typedef struct fz_flate_s fz_flate;

struct fz_flate_s
{
	fz_filter super;
	z_stream z;
};

static void *
zmalloc(void *opaque, unsigned int items, unsigned int size)
{
	fz_memorycontext *mctx = (fz_memorycontext*)opaque;
	return mctx->malloc(mctx, items * size);
}

fz_error *
fz_newflated(fz_filter **fp, fz_obj *params)
{
	fz_error *eo;
	fz_obj *obj;
	int zipfmt;
	int ei;

	FZ_NEWFILTER(fz_flate, f, flated);

	f->z.zalloc = zmalloc;
	f->z.zfree = (void(*)(void*,void*))fz_currentmemorycontext()->free;
	f->z.opaque = fz_currentmemorycontext();
	f->z.next_in = nil;
	f->z.avail_in = 0;

	zipfmt = 0;

	if (params)
	{
		obj = fz_dictgets(params, "ZIP");
		if (obj) zipfmt = fz_tobool(obj);
	}

	if (zipfmt)
	{
		/* if windowbits is negative the zlib header is skipped */
		ei = inflateInit2(&f->z, -15);
	}
	else
		ei = inflateInit(&f->z);

	if (ei != Z_OK)
	{
		eo = fz_throw("ioerror: inflateInit: %s", f->z.msg);
		fz_free(f);
		return eo;
	}

	return nil;
}

void
fz_dropflated(fz_filter *f)
{
	z_streamp zp = &((fz_flate*)f)->z;
	int err;

	err = inflateEnd(zp);
	if (err != Z_OK)
		fprintf(stderr, "inflateEnd: %s", zp->msg);
}

fz_error *
fz_processflated(fz_filter *f, fz_buffer *in, fz_buffer *out)
{
	z_streamp zp = &((fz_flate*)f)->z;
	int err;

	if (in->rp == in->wp && !in->eof)
		return fz_ioneedin;
	if (out->wp == out->ep)
		return fz_ioneedout;

	zp->next_in = in->rp;
	zp->avail_in = in->wp - in->rp;

	zp->next_out = out->wp;
	zp->avail_out = out->ep - out->wp;

	err = inflate(zp, Z_NO_FLUSH);

	in->rp = in->wp - zp->avail_in;
	out->wp = out->ep - zp->avail_out;

	if (err == Z_STREAM_END)
	{
		out->eof = 1;
		return fz_iodone;
	}
	else if (err == Z_OK)
	{
		if (in->rp == in->wp && !in->eof)
			return fz_ioneedin;
		if (out->wp == out->ep)
			return fz_ioneedout;
		return fz_ioneedin; /* hmm, what's going on here? */
	}
	else
	{
		return fz_throw("ioerror: inflate: %s", zp->msg);
	}
}

fz_error *
fz_newflatee(fz_filter **fp, fz_obj *params)
{
	fz_obj *obj;
	fz_error *eo;
	int effort;
	int zipfmt;
	int ei;

	FZ_NEWFILTER(fz_flate, f, flatee);

	effort = -1;
	zipfmt = 0;

	if (params)
	{
		obj = fz_dictgets(params, "Effort");
		if (obj) effort = fz_toint(obj);
		obj = fz_dictgets(params, "ZIP");
		if (obj) effort = fz_tobool(obj);
	}

	f->z.zalloc = zmalloc;
	f->z.zfree = (void(*)(void*,void*))fz_currentmemorycontext()->free;
	f->z.opaque = fz_currentmemorycontext();
	f->z.next_in = nil;
	f->z.avail_in = 0;

	if (zipfmt)
		ei = deflateInit2(&f->z, effort, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
	else
		ei = deflateInit(&f->z, effort);

	if (ei != Z_OK)
	{
		eo = fz_throw("ioerror: deflateInit: %s", f->z.msg);
		fz_free(f);
		return eo;
	}

	return nil;
}

void
fz_dropflatee(fz_filter *f)
{
	z_streamp zp = &((fz_flate*)f)->z;
	int err;

	err = deflateEnd(zp);
	if (err != Z_OK)
		fprintf(stderr, "deflateEnd: %s", zp->msg);
	
	fz_free(f);
}

fz_error *
fz_processflatee(fz_filter *f, fz_buffer *in, fz_buffer *out)
{
	z_streamp zp = &((fz_flate*)f)->z;
	int err;

	if (in->rp == in->wp && !in->eof)
		return fz_ioneedin;
	if (out->wp == out->ep)
		return fz_ioneedout;

	zp->next_in = in->rp;
	zp->avail_in = in->wp - in->rp;

	zp->next_out = out->wp;
	zp->avail_out = out->ep - out->wp;

	err = deflate(zp, in->eof ? Z_FINISH : Z_NO_FLUSH);

	in->rp = in->wp - zp->avail_in;
	out->wp = out->ep - zp->avail_out;

	if (err == Z_STREAM_END)
	{
		out->eof = 1;
		return fz_iodone;
	}
	else if (err == Z_OK)
	{
		if (in->rp == in->wp && !in->eof)
			return fz_ioneedin;
		if (out->wp == out->ep)
			return fz_ioneedout;
		return fz_ioneedin; /* hmm? */
	}
	else
	{
		return fz_throw("ioerror: deflate: %s", zp->msg);
	}
}

