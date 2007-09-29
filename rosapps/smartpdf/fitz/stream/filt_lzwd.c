#include "fitz-base.h"
#include "fitz-stream.h"

/* TODO: error checking */

enum
{
	MINBITS = 9,
	MAXBITS = 12,
	NUMCODES = (1 << MAXBITS),
	LZW_CLEAR = 256,
	LZW_EOD = 257,
	LZW_FIRST = 258
};

typedef struct lzw_code_s lzw_code;

struct lzw_code_s
{
	int prev;					/* prev code (in string) */
	unsigned short length;		/* string len, including this token */
	unsigned char value;		/* data value */
	unsigned char firstchar;	/* first token of string */
};

typedef struct fz_lzwd_s fz_lzwd;

struct fz_lzwd_s
{
	fz_filter super;

	int earlychange;

	unsigned int word;		/* bits loaded from data */
	int bidx;

	int resume;				/* resume output of code from needout */
	int codebits;			/* num bits/code */
	int code;				/* current code */
	int oldcode;			/* previously recognized code */
	int nextcode;			/* next free entry */
	lzw_code table[NUMCODES];
};

fz_error *
fz_newlzwd(fz_filter **fp, fz_obj *params)
{
	int i;

	FZ_NEWFILTER(fz_lzwd, lzw, lzwd);

	lzw->earlychange = 0;

	if (params)
	{
		fz_obj *obj;
		obj = fz_dictgets(params, "EarlyChange");
		if (obj) lzw->earlychange = fz_toint(obj) != 0;
	}

	lzw->bidx = 32;
	lzw->word = 0;

	for (i = 0; i < 256; i++)
	{
		lzw->table[i].value = i;
		lzw->table[i].firstchar = i;
		lzw->table[i].length = 1;
		lzw->table[i].prev = -1;
	}

	for (i = LZW_FIRST; i < NUMCODES; i++)
	{
		lzw->table[i].value = 0;
		lzw->table[i].firstchar = 0;
		lzw->table[i].length = 0;
		lzw->table[i].prev = -1;
	}

	lzw->codebits = MINBITS;
	lzw->code = -1;
	lzw->nextcode = LZW_FIRST;
	lzw->oldcode = -1;
	lzw->resume = 0;

	return nil;
}

void
fz_droplzwd(fz_filter *filter)
{
}

static inline void eatbits(fz_lzwd *lzw, int nbits)
{
	lzw->word <<= nbits;
	lzw->bidx += nbits;
}

static inline fz_error * fillbits(fz_lzwd *lzw, fz_buffer *in)
{
	while (lzw->bidx >= 8)
	{
		if (in->rp + 1 > in->wp)
			return fz_ioneedin;
		lzw->bidx -= 8;
		lzw->word |= *in->rp << lzw->bidx;
		in->rp ++;
	}
	return nil;
}

static inline void unstuff(fz_lzwd *lzw, fz_buffer *in)
{
	int i = (32 - lzw->bidx) / 8;
	while (i-- && in->rp > in->bp)
		in->rp --;
}

fz_error *
fz_processlzwd(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	fz_lzwd *lzw = (fz_lzwd*)filter;
	unsigned char *s;
	int len;

	if (lzw->resume)
	{
		lzw->resume = 0;
		goto output;
	}

	while (1)
	{
		if (fillbits(lzw, in))
		{
			if (in->eof)
			{
				if (lzw->bidx > 32 - lzw->codebits)
				{
					out->eof = 1;
					unstuff(lzw, in);
					return fz_iodone;
				}
			}
			else
			{
				return fz_ioneedin;
			}
		}

		lzw->code = lzw->word >> (32 - lzw->codebits);

		if (lzw->code == LZW_EOD)
		{
			eatbits(lzw, lzw->codebits);
			out->eof = 1;
			unstuff(lzw, in);
			return fz_iodone;
		}

		if (lzw->code == LZW_CLEAR)
		{
			int oldcodebits = lzw->codebits;

			lzw->codebits = MINBITS;
			lzw->nextcode = LZW_FIRST;

			lzw->code = lzw->word >> (32 - oldcodebits - MINBITS) & ((1 << MINBITS) - 1);

			if (lzw->code == LZW_EOD)
			{
				eatbits(lzw, oldcodebits + MINBITS);
				out->eof = 1;
				unstuff(lzw, in);
				return fz_iodone;
			}

			if (out->wp + 1 > out->ep)
				return fz_ioneedout;

			*out->wp++ = lzw->code;

			lzw->oldcode = lzw->code;

			eatbits(lzw, oldcodebits + MINBITS);

			continue;
		}

		eatbits(lzw, lzw->codebits);

		/* if stream starts without a clear code, oldcode is undefined... */
		if (lzw->oldcode == -1)
		{
			lzw->oldcode = lzw->code;
			goto output;
		}

		/* add new entry to the code table */
		lzw->table[lzw->nextcode].prev = lzw->oldcode;
		lzw->table[lzw->nextcode].firstchar = lzw->table[lzw->oldcode].firstchar;
		lzw->table[lzw->nextcode].length = lzw->table[lzw->oldcode].length + 1;
		if (lzw->code < lzw->nextcode)
			lzw->table[lzw->nextcode].value = lzw->table[lzw->code].firstchar;
		else
			lzw->table[lzw->nextcode].value = lzw->table[lzw->nextcode].firstchar;

		lzw->nextcode ++;

		if (lzw->nextcode >= (1 << lzw->codebits) - lzw->earlychange - 1)
		{
			lzw->codebits ++;
			if (lzw->codebits > MAXBITS)
				lzw->codebits = MAXBITS;	/* FIXME */
		}

		lzw->oldcode = lzw->code;

output:
		/* code maps to a string, copy to output (in reverse...) */
		if (lzw->code > 255)
		{
			if (out->wp + lzw->table[lzw->code].length > out->ep)
			{
				lzw->resume = 1;
				return fz_ioneedout;
			}

			len = lzw->table[lzw->code].length;
			s = out->wp + len;

			do
			{
				*(--s) = lzw->table[lzw->code].value;
				lzw->code = lzw->table[lzw->code].prev;
			} while (lzw->code >= 0 && s > out->wp);
			out->wp += len;
		}

		/* ... or just a single character */
		else
		{
			if (out->wp + 1 > out->ep)
			{
				lzw->resume = 1;
				return fz_ioneedout;
			}

			*out->wp++ = lzw->code;
		}
	}
}

