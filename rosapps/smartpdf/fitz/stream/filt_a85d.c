#include "fitz-base.h"
#include "fitz-stream.h"

typedef struct fz_a85d_s fz_a85d;

struct fz_a85d_s
{
	fz_filter super;
	unsigned long word;
	int count;
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

fz_error *
fz_newa85d(fz_filter **fp, fz_obj *params)
{
	FZ_NEWFILTER(fz_a85d, f, a85d);
	f->word = 0;
	f->count = 0;
	return nil;
}

void
fz_dropa85d(fz_filter *f)
{
}

fz_error *
fz_processa85d(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	fz_a85d *f = (fz_a85d*)filter;
	int c;

	while (1)
	{
		if (in->rp == in->wp)
			return fz_ioneedin;

		c = *in->rp++;

		if (c >= '!' && c <= 'u') {
			if (f->count == 4) {
				if (out->wp + 4 > out->ep) {
					in->rp --;
					return fz_ioneedout;
				}

				f->word = f->word * 85 + (c - '!');

				*out->wp++ = (f->word >> 24) & 0xff;
				*out->wp++ = (f->word >> 16) & 0xff;
				*out->wp++ = (f->word >> 8) & 0xff;
				*out->wp++ = (f->word) & 0xff;

				f->word = 0;
				f->count = 0;
			}
			else {
				f->word = f->word * 85 + (c - '!');
				f->count ++;
			}
		}

		else if (c == 'z' && f->count == 0) {
			if (out->wp + 4 > out->ep) {
				in->rp --;
				return fz_ioneedout;
			}
			*out->wp++ = 0;
			*out->wp++ = 0;
			*out->wp++ = 0;
			*out->wp++ = 0;
		}

		else if (c == '~') {
			if (in->rp == in->wp) {
				in->rp --;
				return fz_ioneedin;
			}

			c = *in->rp++;

			if (c != '>') {
				return fz_throw("ioerror: bad eod marker in a85d");
			}

			if (out->wp + f->count - 1 > out->ep) {
				in->rp -= 2;
				return fz_ioneedout;
			}

			switch (f->count) {
			case 0:
				break;
			case 1:
				return fz_throw("ioerror: partial final byte in a85d");
			case 2:
				f->word = f->word * (85L * 85 * 85) + 0xffffffL;
				goto o1;
			case 3:
				f->word = f->word * (85L * 85) + 0xffffL;
				goto o2;
			case 4:
				f->word = f->word * 85 + 0xffL;
				*(out->wp+2) = f->word >> 8;
o2:				*(out->wp+1) = f->word >> 16;
o1:				*(out->wp+0) = f->word >> 24;
				out->wp += f->count - 1;
				break;
			}
			out->eof = 1;
			return fz_iodone;
		}

		else if (!iswhite(c)) {
			return fz_throw("ioerror: bad data in a85d: '%c'", c);
		}
	}
}

