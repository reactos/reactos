#include "fitz-base.h"
#include "fitz-stream.h"

#define noDEBUG 1

enum
{
	MINBITS = 9,
	MAXBITS = 12,
	MAXBYTES = 2,
	NUMCODES = (1 << MAXBITS),
	LZW_CLEAR = 256,
	LZW_EOD = 257,
	LZW_FIRST = 258,
	HSIZE = 9001,		/* 91% occupancy (???) */
	HSHIFT = (13 - 8)
};

typedef struct lzw_hash_s lzw_hash;

struct lzw_hash_s
{
	int hash;
	int code;
};

typedef struct fz_lzwe_s fz_lzwe;

struct fz_lzwe_s
{
	fz_filter super;

	int earlychange;

	int bidx;		/* partial bits used in out->wp */
	unsigned char bsave;	/* partial byte saved between process() calls */

	int resume;
	int code;
	int fcode;
	int hcode;

	int codebits;
	int oldcode;
	int nextcode;

	lzw_hash table[HSIZE];
};

static void
clearhash(fz_lzwe *lzw)
{
	int i;
	for (i = 0; i < HSIZE; i++)
		lzw->table[i].hash = -1;
}

fz_error *
fz_newlzwe(fz_filter **fp, fz_obj *params)
{
	FZ_NEWFILTER(fz_lzwe, lzw, lzwe);

	lzw->earlychange = 0;

	if (params)
	{
		fz_obj *obj;
		obj = fz_dictgets(params, "EarlyChange");
		if (obj) lzw->earlychange = fz_toint(obj) != 0;
	}

	lzw->bidx = 0;
	lzw->bsave = 0;

	lzw->resume = 0;
	lzw->code = -1;
	lzw->hcode = -1;
	lzw->fcode = -1;

	lzw->codebits = MINBITS;
	lzw->nextcode = LZW_FIRST;
	lzw->oldcode = -1;	/* generates LZW_CLEAR */

	clearhash(lzw);

	return nil;
}

void
fz_droplzwe(fz_filter *filter)
{
}

static void
putcode(fz_lzwe *lzw, fz_buffer *out, int code)
{
	int nbits = lzw->codebits;

	while (nbits > 0)
	{
		if (lzw->bidx == 0)
		{
			*out->wp = 0;
		}

		/* code does not fit: shift right */
		if (nbits > (8 - lzw->bidx))
		{
			*out->wp |= code >> (nbits - (8 - lzw->bidx));
			nbits = nbits - (8 - lzw->bidx);
			lzw->bidx = 0;
			out->wp ++;
		}

		/* shift left */
		else
		{
			*out->wp |= code << ((8 - lzw->bidx) - nbits);
			lzw->bidx += nbits;
			if (lzw->bidx == 8)
			{
				lzw->bidx = 0;
				out->wp ++;
			}
			nbits = 0;
		}
	}
}


static fz_error *
compress(fz_lzwe *lzw, fz_buffer *in, fz_buffer *out)
{
	if (lzw->resume)
	{
		lzw->resume = 0;
		goto resume;
	}

	/* at start of data, output a clear code */
	if (lzw->oldcode == -1)
	{
		if (out->wp + 3 > out->ep)
			return fz_ioneedout;

		if (in->rp + 1 > in->wp)
		{
			if (in->eof)
				goto eof;
			return fz_ioneedin;
		}

		putcode(lzw, out, LZW_CLEAR);

		lzw->oldcode = *in->rp++;
	}

begin:
	while (1)
	{
		if (in->rp + 1 > in->wp)
		{
			if (in->eof)
				goto eof;
			return fz_ioneedin;
		}

		/* read character */
		lzw->code = *in->rp++;

		/* hash string + character */
		lzw->fcode = (lzw->code << MAXBITS) + lzw->oldcode;
		lzw->hcode = (lzw->code << HSHIFT) ^ lzw->oldcode;

		/* primary hash */
		if (lzw->table[lzw->hcode].hash == lzw->fcode)
		{
			lzw->oldcode = lzw->table[lzw->hcode].code;
			continue;
		}

		/* secondary hash */
		if (lzw->table[lzw->hcode].hash != -1)
		{
			int disp = HSIZE - lzw->hcode;
			if (lzw->hcode == 0)
				disp = 1;
			do
			{
				lzw->hcode = lzw->hcode - disp;
				if (lzw->hcode < 0)
					lzw->hcode += HSIZE;
				if (lzw->table[lzw->hcode].hash == lzw->fcode)
				{
					lzw->oldcode = lzw->table[lzw->hcode].code;
					goto begin;
				}
			} while (lzw->table[lzw->hcode].hash != -1);
		}

resume:
		/* new entry: emit code and add to table */

		/* reserve space for this code and an eventual CLEAR code */
		if (out->wp + 5 > out->ep)
		{
			lzw->resume = 1;
			return fz_ioneedout;
		}

		putcode(lzw, out, lzw->oldcode);

		lzw->oldcode = lzw->code;
		lzw->table[lzw->hcode].code = lzw->nextcode;
		lzw->table[lzw->hcode].hash = lzw->fcode;

		lzw->nextcode ++;

		/* table is full: emit clear code and reset */
		if (lzw->nextcode == NUMCODES - 1)
		{
			putcode(lzw, out, LZW_CLEAR);
			clearhash(lzw);
			lzw->nextcode = LZW_FIRST;
			lzw->codebits = MINBITS;
		}

		/* check if next entry will be too big for the code size */
		else if (lzw->nextcode >= (1 << lzw->codebits) - lzw->earlychange)
		{
			lzw->codebits ++;
		}
	}

eof:
	if (out->wp + 5 > out->ep)
		return fz_ioneedout;

	putcode(lzw, out, lzw->oldcode);
	putcode(lzw, out, LZW_EOD);

	out->eof = 1;
	return fz_iodone;
}

fz_error *
fz_processlzwe(fz_filter *filter, fz_buffer *in, fz_buffer *out)
{
	fz_lzwe *lzw = (fz_lzwe*)filter;
	fz_error *error;

	/* restore partial bits */
	*out->wp = lzw->bsave;

	error = compress(lzw, in, out);

	/* save partial bits */
	lzw->bsave = *out->wp;

	return error;
}

