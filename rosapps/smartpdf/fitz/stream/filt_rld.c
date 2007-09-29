#include "fitz-base.h"
#include "fitz-stream.h"

fz_error *
fz_newrld(fz_filter **fp, fz_obj *params)
{
	FZ_NEWFILTER(fz_filter, f, rld);
	return nil;
}

void
fz_droprld(fz_filter *rld)
{
}

fz_error *
fz_processrld(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	int run, i;
	unsigned char c;

	while (1)
	{
		if (in->rp == in->wp)
		{
			if (in->eof)
			{
				out->eof = 1;
				return fz_iodone;
			}
			return fz_ioneedin;
		}

		if (out->wp == out->ep)
			return fz_ioneedout;

		run = *in->rp++;

		if (run == 128) {
			out->eof = 1;
			return fz_iodone;
		}

		else if (run < 128) {
			run = run + 1;
			if (in->rp + run > in->wp) {
				in->rp --;
				return fz_ioneedin;
			}
			if (out->wp + run > out->ep) {
				in->rp --;
				return fz_ioneedout;
			}
			for (i = 0; i < run; i++)
				*out->wp++ = *in->rp++;
		}

		else if (run > 128) {
			run = 257 - run;
			if (in->rp + 1 > in->wp) {
				in->rp --;
				return fz_ioneedin;
			}
			if (out->wp + run > out->ep) {
				in->rp --;
				return fz_ioneedout;
			}
			c = *in->rp++;
			for (i = 0; i < run; i++)
				*out->wp++ = c;
		}
	}
}

