#include "fitz-base.h"
#include "fitz-stream.h"

typedef struct fz_arc4c_s fz_arc4c;

struct fz_arc4c_s
{
	fz_filter super;
	fz_arc4 arc4;
};

fz_error *
fz_newarc4filter(fz_filter **fp, unsigned char *key, unsigned keylen)
{
	FZ_NEWFILTER(fz_arc4c, f, arc4filter);
	fz_arc4init(&f->arc4, key, keylen);
	return nil;
}

void
fz_droparc4filter(fz_filter *f)
{
}

fz_error *
fz_processarc4filter(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	fz_arc4c *f = (fz_arc4c*)filter;
	int n;

	while (1)
	{
		if (in->rp + 1 > in->wp) {
			if (in->eof)
				return fz_iodone;
			return fz_ioneedin;
		}
		if (out->wp + 1 > out->ep)
			return fz_ioneedout;

		n = MIN(in->wp - in->rp, out->ep - out->wp);
		fz_arc4encrypt(&f->arc4, out->wp, in->rp, n);
		in->rp += n;
		out->wp += n;
	}
}

