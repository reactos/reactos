#include "fitz-base.h"
#include "fitz-stream.h"

typedef struct fz_nullfilter_s fz_nullfilter;

struct fz_nullfilter_s
{
	fz_filter super;
	int len;
	int cur;
};

fz_error *
fz_newnullfilter(fz_filter **fp, int len)
{
	FZ_NEWFILTER(fz_nullfilter, f, nullfilter);
	f->len = len;
	f->cur = 0;
	return nil;
}

void
fz_dropnullfilter(fz_filter *f)
{
}

fz_error *
fz_processnullfilter(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	fz_nullfilter *f = (fz_nullfilter*)filter;
	int n;

	n = MIN(in->wp - in->rp, out->ep - out->wp);
	if (f->len >= 0)
	    n = MIN(n, f->len - f->cur);

	if (n) {
		memcpy(out->wp, in->rp, n);
		in->rp += n;
		out->wp += n;
		f->cur += n;
	}

	if (f->cur == f->len)
		return fz_iodone;
	if (in->rp == in->wp)
		return fz_ioneedin;
	if (out->wp == out->ep)
		return fz_ioneedout;

	return fz_throw("braindead programmer in nullfilter");
}

