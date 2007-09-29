#include "fitz-base.h"
#include "fitz-stream.h"

typedef struct fz_ahxd_s fz_ahxd;

struct fz_ahxd_s
{
	fz_filter super;
	int odd;
	int a;
};

static inline int iswhite(int a)
{
	switch (a) {
	case '\n': case '\r': case '\t': case ' ':
	case '\0': case '\f': case '\b': case 0177:
		return 1;
	}
	return 0;
}

static inline int ishex(int a)
{
	return (a >= 'A' && a <= 'F') ||
		(a >= 'a' && a <= 'f') ||
		(a >= '0' && a <= '9');
}

static inline int fromhex(int a)
{
	if (a >= 'A' && a <= 'F')
		return a - 'A' + 0xA;
	if (a >= 'a' && a <= 'f')
		return a - 'a' + 0xA;
	if (a >= '0' && a <= '9')
		return a - '0';
	return 0;
}

fz_error *
fz_newahxd(fz_filter **fp, fz_obj *params)
{
	FZ_NEWFILTER(fz_ahxd, f, ahxd);
	f->odd = 0;
	f->a = 0;
	return nil;
}

void
fz_dropahxd(fz_filter *f)
{
}

fz_error *
fz_processahxd(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	fz_ahxd *f = (fz_ahxd*)filter;
	int b, c;

	while (1)
	{
		if (in->rp == in->wp)
			return fz_ioneedin;

		if (out->wp == out->ep)
			return fz_ioneedout;

		c = *in->rp++;

		if (ishex(c)) {
			if (!f->odd) {
				f->a = fromhex(c);
				f->odd = 1;
			}
			else {
				b = fromhex(c);
				*out->wp++ = (f->a << 4) | b;
				f->odd = 0;
			}
		}

		else if (c == '>') {
			if (f->odd)
				*out->wp++ = (f->a << 4);
			out->eof = 1;
			return fz_iodone;
		}

		else if (!iswhite(c)) {
			return fz_throw("ioerror: bad data in ahxd: '%c'", c);
		}
	}
}

void
fz_pushbackahxd(fz_filter *filter, fz_buffer *in, fz_buffer *out, int n)
{
	int k;

	assert(filter->process == fz_processahxd);
	assert(out->wp - n >= out->rp);

	k = 0;
	while (k < n * 2) {
		in->rp --;
		if (ishex(*in->rp))
			k ++;
	}

	out->wp -= n;
}

