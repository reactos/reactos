#include "fitz-base.h"
#include "fitz-stream.h"

/* TODO: rewrite!
 * make it non-optimal or something,
 * just not this horrid mess...
 */

#define noDEBUG

typedef struct fz_rle_s fz_rle;

struct fz_rle_s
{
	fz_filter super;
	int reclen;
	int curlen;
	int state;
	int run;
	unsigned char buf[128];
};

enum {
	ZERO,
	ONE,
	DIFF,
	SAME,
	END
};

fz_error *
fz_newrle(fz_filter **fp, fz_obj *params)
{
	FZ_NEWFILTER(fz_rle, enc, rle);

	if (params)
		enc->reclen = fz_toint(params);
	else
		enc->reclen = 0;

	enc->curlen = 0;
	enc->state = ZERO;
	enc->run = 0;

	return nil;
}

void
fz_droprle(fz_filter *enc)
{
}

static fz_error *
putone(fz_rle *enc, fz_buffer *in, fz_buffer *out)
{
	if (out->wp + 2 >= out->ep)
		return fz_ioneedout;

#ifdef DEBUG
fprintf(stderr, "one '%c'\n", enc->buf[0]);
#endif

	*out->wp++ = 0;
	*out->wp++ = enc->buf[0];

	return nil;
}

static fz_error *
putsame(fz_rle *enc, fz_buffer *in, fz_buffer *out)
{
	if (out->wp + enc->run >= out->ep)
		return fz_ioneedout;

#ifdef DEBUG
fprintf(stderr, "same %d x '%c'\n", enc->run, enc->buf[0]);
#endif

	*out->wp++ = 257 - enc->run;
	*out->wp++ = enc->buf[0];
	return nil;
}

static fz_error *
putdiff(fz_rle *enc, fz_buffer *in, fz_buffer *out)
{
	int i;
	if (out->wp + enc->run >= out->ep)
		return fz_ioneedout;

#ifdef DEBUG
fprintf(stderr, "diff %d\n", enc->run);
#endif

	*out->wp++ = enc->run - 1;
	for (i = 0; i < enc->run; i++)
		*out->wp++ = enc->buf[i];
	return nil;
}

static fz_error *
puteod(fz_rle *enc, fz_buffer *in, fz_buffer *out)
{
	if (out->wp + 1 >= out->ep)
		return fz_ioneedout;

#ifdef DEBUG
fprintf(stderr, "eod\n");
#endif

	*out->wp++ = 128;
	return nil;
}

static fz_error *
savebuf(fz_rle *enc, fz_buffer *in, fz_buffer *out)
{
	switch (enc->state)
	{
		case ZERO: return nil;
		case ONE: return putone(enc, in, out);
		case SAME: return putsame(enc, in, out);
		case DIFF: return putdiff(enc, in, out);
		case END: return puteod(enc, in, out);
		default: assert(!"invalid state in rle"); return nil;
	}
}

fz_error *
fz_processrle(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	fz_rle *enc = (fz_rle*)filter;
	fz_error *error;
	unsigned char c;

	while (1)
	{

		if (enc->reclen && enc->curlen == enc->reclen) {
			error = savebuf(enc, in, out);
			if (error) return error;
#ifdef DEBUG
fprintf(stderr, "--record--\n");
#endif
			enc->state = ZERO;
			enc->curlen = 0;
		}

		if (in->rp == in->wp) {
			if (in->eof) {
				if (enc->state != END) {
					error = savebuf(enc, in, out);
					if (error) return error;
				}
				enc->state = END;
			}
			else
				return fz_ioneedin;
		}

		c = *in->rp;

		switch (enc->state)
		{
		case ZERO:
			enc->state = ONE;
			enc->run = 1;
			enc->buf[0] = c;
			break;

		case ONE:
			enc->state = DIFF;
			enc->run = 2;
			enc->buf[1] = c;
			break;

		case DIFF:
			/* out of space */
			if (enc->run == 128) {
				error = putdiff(enc, in, out);
				if (error) return error;

				enc->state = ONE;
				enc->run = 1;
				enc->buf[0] = c;
			}

			/* run of three that are the same */
			else if ((enc->run > 1) &&
				(c == enc->buf[enc->run - 1]) &&
				(c == enc->buf[enc->run - 2]))
			{
				if (enc->run >= 3) {
					enc->run -= 2;	/* skip prev two for diff run */
					error = putdiff(enc, in, out);
					if (error) return error;
				}

				enc->state = SAME;
				enc->run = 3;
				enc->buf[0] = c;
			}

			/* keep on collecting */
			else {
				enc->buf[enc->run++] = c;
			}
			break;

		case SAME:
			if (enc->run == 128 || c != enc->buf[0]) {
				error = putsame(enc, in, out);
				if (error) return error;

				enc->state = ONE;
				enc->run = 1;
				enc->buf[0] = c;
			}
			else {
				enc->run ++;
			}
			break;

		case END:
			error = puteod(enc, in, out);
			if (error) return error;
			
			out->eof = 1;
			return fz_iodone;
		}

		in->rp ++;

		enc->curlen ++;

	}
}

