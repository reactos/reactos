#include "fitz-base.h"
#include "fitz-stream.h"

typedef struct fz_ahxe_s fz_ahxe;

struct fz_ahxe_s
{
	fz_filter super;
	int c;
};

static const char tohex[16] = "0123456789ABCDEF";

fz_error *
fz_newahxe(fz_filter **fp, fz_obj *params)
{
	FZ_NEWFILTER(fz_ahxe, f, ahxe);
	f->c = 0;
	return nil;
}

void
fz_dropahxe(fz_filter *f)
{
}

fz_error *
fz_processahxe(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	fz_ahxe *f = (fz_ahxe*)filter;
	int a, b, c;

	while (1)
	{
		if (in->rp == in->wp)
			goto needinput;

		if (out->wp + 2 >= out->ep) /* can write 3 bytes from 1 */
			return fz_ioneedout;

		c = *in->rp++;
		a = tohex[(c >> 4) & 0x0f];
		b = tohex[c & 0x0f];

		*out->wp++ = a;
		*out->wp++ = b;

		f->c += 2;
		if (f->c == 60) {
			*out->wp++ = '\n';
			f->c = 0;
		}
	}

needinput:
	if (in->eof) {
		if (out->wp == out->ep)
			return fz_ioneedout;
		*out->wp++ = '>';
		out->eof = 1;
		return fz_iodone;
	}
	return fz_ioneedin;
}

