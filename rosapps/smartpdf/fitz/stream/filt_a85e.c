#include "fitz-base.h"
#include "fitz-stream.h"

typedef struct fz_a85e_s fz_a85e;

struct fz_a85e_s
{
	fz_filter super;
	int c;
};

fz_error *
fz_newa85e(fz_filter **fp, fz_obj *params)
{
	FZ_NEWFILTER(fz_a85e, f, a85e);
	f->c = 0;
	return nil;
}

void
fz_dropa85e(fz_filter *f)
{
}

fz_error *
fz_processa85e(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	fz_a85e *f = (fz_a85e*)filter;
	unsigned long word;
	int count;
	int n;

	n = 0;

	while (1)
	{
		if (f->c >= 70) {
			if (out->wp + 1 > out->ep)
				return fz_ioneedout;
			*out->wp++ = '\n';
			f->c = 0;
			n ++;
		}

		if (in->rp + 4 <= in->wp)
		{
			word =	(in->rp[0] << 24) |
					(in->rp[1] << 16) |
					(in->rp[2] << 8) |
					(in->rp[3]);
			if (word == 0) {
				if (out->wp + 1 > out->ep)
					return fz_ioneedout;
				*out->wp++ = 'z';
				f->c ++;
				n ++;
			}
			else {
				unsigned long v1, v2, v3, v4;

				if (out->wp + 5 > out->ep)
					return fz_ioneedout;

				v4 = word / 85;
				v3 = v4 / 85;
				v2 = v3 / 85;
				v1 = v2 / 85;

				*out->wp++ = (v1 % 85) + '!';
				*out->wp++ = (v2 % 85) + '!';
				*out->wp++ = (v3 % 85) + '!';
				*out->wp++ = (v4 % 85) + '!';
				*out->wp++ = (word % 85) + '!';
				f->c += 5;
				n += 5;
			}
			in->rp += 4;
		}

		else if (in->eof)
		{
			unsigned long divisor;

			if (in->rp == in->wp)
				goto needinput; /* handle clean eof here */

			count = in->wp - in->rp;

			if (out->wp + count + 3 > out->ep)
				return fz_ioneedout;

			word = 0;
			switch (count) {
				case 3: word |= in->rp[2] << 8;
				case 2: word |= in->rp[1] << 16;
				case 1: word |= in->rp[0] << 24;
			}
			in->rp += count;

			divisor = 85L * 85 * 85 * 85;
			while (count-- >= 0) {
				*out->wp++ = ((word / divisor) % 85) + '!';
				divisor /= 85;
			}

			*out->wp++ = '~';
			*out->wp++ = '>';
			out->eof = 1;
			return fz_iodone;
		}

		else {
			goto needinput;
		}
	}

needinput:
	if (in->eof) {
		if (out->wp + 2 > out->ep)
			return fz_ioneedout;
		*out->wp++ = '~';
		*out->wp++ = '>';
		out->eof = 1;
		return fz_iodone;
	}
	return fz_ioneedin;
}

