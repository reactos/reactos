#include "fitz-base.h"
#include "fitz-stream.h"

#include "filt_faxe.h"
#include "filt_faxc.h"

/* TODO: honor Rows param */

typedef struct fz_faxe_s fz_faxe;

struct fz_faxe_s
{
	fz_filter super;

	int k;
	int endofline;
	int encodedbytealign;
	int columns;
	int endofblock;
	int blackis1;

	int stride;
	int ridx;		/* how many rows in total */
	int bidx;		/* how many bits are already used in out->wp */
	unsigned char bsave;	/* partial byte saved between process() calls */

	int stage;
	int a0, c;		/* mid-line coding state */

	unsigned char *ref;
	unsigned char *src;
};

fz_error *
fz_newfaxe(fz_filter **fp, fz_obj *params)
{
	fz_obj *obj;

	FZ_NEWFILTER(fz_faxe, fax, faxe);

	fax->ref = nil;
	fax->src = nil;

	fax->k = 0;
	fax->endofline = 0;
	fax->encodedbytealign = 0;
	fax->columns = 1728;
	fax->endofblock = 1;
	fax->blackis1 = 0;

	obj = fz_dictgets(params, "K");
	if (obj) fax->k = fz_toint(obj);

	obj = fz_dictgets(params, "EndOfLine");
	if (obj) fax->endofline = fz_tobool(obj);

	obj = fz_dictgets(params, "EncodedByteAlign");
	if (obj) fax->encodedbytealign = fz_tobool(obj);

	obj = fz_dictgets(params, "Columns");
	if (obj) fax->columns = fz_toint(obj);

	obj = fz_dictgets(params, "EndOfBlock");
	if (obj) fax->endofblock = fz_tobool(obj);

	obj = fz_dictgets(params, "BlackIs1");
	if (obj) fax->blackis1 = fz_tobool(obj);

	fax->stride = ((fax->columns - 1) >> 3) + 1;
	fax->bidx = 0;
	fax->ridx = 0;

	fax->stage = 0;
	fax->a0 = -1;
	fax->c = 0;

	fax->ref = fz_malloc(fax->stride);
	if (!fax->ref) { fz_free(fax); return fz_outofmem; }

	fax->src = fz_malloc(fax->stride);
	if (!fax->src) { fz_free(fax); fz_free(fax->ref); return fz_outofmem; }

	memset(fax->ref, 0, fax->stride);
	memset(fax->src, 0, fax->stride);

	return nil;
}

void
fz_dropfaxe(fz_filter *p)
{
	fz_faxe *fax = (fz_faxe*) p;
	fz_free(fax->src);
	fz_free(fax->ref);
}

enum { codebytes = 2 };

static inline int runbytes(int run)
{
	int m = (run / 64) / 40 + 1;	/* number of makeup codes */
	return codebytes * (m + 1); /* bytes for makeup + term codes */
}

static void
putbits(fz_faxe *fax, fz_buffer *out, int code, int nbits)
{
	while (nbits > 0)
	{
		if (fax->bidx == 0)
			*out->wp = 0;

		/* code does not fit: shift right */
		if (nbits > (8 - fax->bidx))
		{
			*out->wp |= code >> (nbits - (8 - fax->bidx));
			nbits = nbits - (8 - fax->bidx);
			fax->bidx = 0;
			out->wp ++;
		}

		/* shift left */
		else
		{
			*out->wp |= code << ((8 - fax->bidx) - nbits);
			fax->bidx += nbits;
			if (fax->bidx == 8)
			{
				fax->bidx = 0;
				out->wp ++;
			}
			nbits = 0;
		}
	}
}

static inline void
putcode(fz_faxe *fax, fz_buffer *out, const cfe_code *run)
{
	putbits(fax, out, run->code, run->nbits);
}

static void
putrun(fz_faxe *fax, fz_buffer *out, int run, int c)
{
	int m;

	const cf_runs *codetable = c ? &cf_black_runs : &cf_white_runs;

	if (run > 63)
	{
		m = run / 64;
		while (m > 40)
		{
			putcode(fax, out, &codetable->makeup[40]);
			m -= 40;
		}
		if (m > 0)
		{
			putcode(fax, out, &codetable->makeup[m]);
		}
		putcode(fax, out, &codetable->termination[run % 64]);
	}
	else
	{
		putcode(fax, out, &codetable->termination[run]);
	}
}

static fz_error *
enc1d(fz_faxe *fax, unsigned char *line, fz_buffer *out)
{
	int run;

	if (fax->a0 < 0)
		fax->a0 = 0;

	while (fax->a0 < fax->columns)
	{
		run = getrun(line, fax->a0, fax->columns, fax->c);

		if (out->wp + 1 + runbytes(run) > out->ep)
			return fz_ioneedout;

		putrun(fax, out, run, fax->c);

		fax->a0 += run;
		fax->c = !fax->c;
	}

	return 0;
}

static fz_error *
enc2d(fz_faxe *fax, unsigned char *ref, unsigned char *src, fz_buffer *out)
{
	int a1, a2, b1, b2;
	int run1, run2, n;

	while (fax->a0 < fax->columns)
	{
		a1 = findchanging(src, fax->a0, fax->columns);
		b1 = findchangingcolor(ref, fax->a0, fax->columns, !fax->c);
		b2 = findchanging(ref, b1, fax->columns);

		/* pass */
		if (b2 < a1)
		{
			if (out->wp + 1 + codebytes > out->ep)
				return fz_ioneedout;

			putcode(fax, out, &cf2_run_pass);

			fax->a0 = b2;
		}

		/* vertical */
		else if (ABS(b1 - a1) <= 3)
		{
			if (out->wp + 1 + codebytes > out->ep)
				return fz_ioneedout;

			putcode(fax, out, &cf2_run_vertical[b1 - a1 + 3]);

			fax->a0 = a1;
			fax->c = !fax->c;
		}

		/* horizontal */
		else
		{
			a2 = findchanging(src, a1, fax->columns);
			run1 = a1 - fax->a0;
			run2 = a2 - a1;
			n = codebytes + runbytes(run1) + runbytes(run2);

			if (out->wp + 1 + n > out->ep)
				return fz_ioneedout;

			putcode(fax, out, &cf2_run_horizontal);
			putrun(fax, out, run1, fax->c);
			putrun(fax, out, run2, !fax->c);

			fax->a0 = a2;
		}
	}

	return 0;
}

static fz_error *
process(fz_faxe *fax, fz_buffer *in, fz_buffer *out)
{
	fz_error *error;
	int i, n;

	while (1)
	{
		if (in->rp == in->wp && in->eof)
			goto rtc;

		switch (fax->stage)
		{
		case 0:
			if (fax->encodedbytealign)
			{
				if (fax->endofline)
				{
					if (out->wp + 1 + 1 > out->ep)
						return fz_ioneedout;

					/* make sure that EOL ends on a byte border */
					putbits(fax, out, 0, (12 - fax->bidx) & 7);
				}
				else
				{
					if (fax->bidx)
					{
						if (out->wp + 1 > out->ep)
							return fz_ioneedout;
						fax->bidx = 0;
						out->wp ++;
					}
				}
			}

			fax->stage ++;

		case 1:
			if (fax->endofline)
			{
				if (out->wp + 1 + codebytes + 1 > out->ep)
					return fz_ioneedout;

				if (fax->k > 0)
				{
					if (fax->ridx % fax->k == 0)
						putcode(fax, out, &cf2_run_eol_1d);
					else
						putcode(fax, out, &cf2_run_eol_2d);
				}
				else
				{
					putcode(fax, out, &cf_run_eol);
				}
			}

			fax->stage ++;

		case 2:
			if (in->rp + fax->stride > in->wp)
			{
				if (in->eof) /* XXX barf here? */
					goto rtc;
				return fz_ioneedin;
			}

			memcpy(fax->ref, fax->src, fax->stride);
			memcpy(fax->src, in->rp, fax->stride);

			if (!fax->blackis1)
			{
				for (i = 0; i < fax->stride; i++)
					fax->src[i] = ~fax->src[i];
			}

			in->rp += fax->stride;

			fax->c = 0;
			fax->a0 = -1;

			fax->stage ++;
			
		case 3:
			error = 0; /* to silence compiler */

			if (fax->k < 0)
				error = enc2d(fax, fax->ref, fax->src, out);

			else if (fax->k == 0)
				error = enc1d(fax, fax->src, out);

			else if (fax->k > 0)
			{
				if (fax->ridx % fax->k == 0)
					error = enc1d(fax, fax->src, out);
				else
					error = enc2d(fax, fax->ref, fax->src, out);
			}

			if (error)
				return error;

			fax->ridx ++;

			fax->stage = 0;
		}
	}

rtc:
	if (fax->endofblock)
	{
		n = fax->k < 0 ? 2 : 6;

		if (out->wp + 1 + codebytes * n > out->ep)
			return fz_ioneedout;

		for (i = 0; i < n; i++)
		{
			putcode(fax, out, &cf_run_eol);
			if (fax->k > 0)
				putbits(fax, out, 1, 1);
		}
	}

	if (fax->bidx)
		out->wp ++;
	out->eof = 1;

	return fz_iodone;
}

fz_error *
fz_processfaxe(fz_filter *p, fz_buffer *in, fz_buffer *out)
{
	fz_faxe *fax = (fz_faxe*) p;
	fz_error *error;

	/* restore partial bits */
	*out->wp = fax->bsave;

	error = process(fax, in, out);

	/* save partial bits */
	fax->bsave = *out->wp;

	return error;
}

