/*
	layer1.c: the layer 1 decoder

	copyright 1995-2009 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp

	may have a few bugs after last optimization ... 
*/

#include "mpg123lib_intern.h"
#include "getbits.h"
#include "debug.h"

/*
	Allocation value is not allowed to be 15. Initially, libmad showed me the
	error that mpg123 used to ignore. Then, I found a quote on that in
	Shlien, S. (1994): Guide to MPEG-1 Audio Standard. 
	IEEE Transactions on Broadcasting 40, 4

	"To avoid conflicts with the synchronization code, code '1111' is defined
	to be illegal."
*/
static int check_balloc(mpg123_handle *fr, unsigned int *balloc, unsigned int *end)
{
	unsigned int *ba;
	for(ba=balloc; ba != end; ++ba)
	if(*ba == 15)
	{
		if(NOQUIET) error("Illegal bit allocation value.");
		return -1;
	}

	return 0;
}

#define NEED_BITS(fr, num) \
	if((fr)->bits_avail < num) \
	{ \
		if(NOQUIET) \
			error2("%u bits needed, %li available", num, (fr)->bits_avail); \
		return -1; \
	} \

static int I_step_one(unsigned int balloc[], unsigned int scale_index[2][SBLIMIT],mpg123_handle *fr)
{
	unsigned int *ba=balloc;
	unsigned int *sca = (unsigned int *) scale_index;

	if(fr->stereo == 2)
	{
		int i;
		int jsbound = fr->jsbound;
		unsigned int needbits = jsbound*2*4 + (SBLIMIT-jsbound)*4;

		NEED_BITS(fr, needbits);
		needbits = 0;
		for(i=0;i<jsbound;i++)
		{
			ba[0] = getbits_fast(fr, 4);
			ba[1] = getbits_fast(fr, 4);
			needbits += ((ba[0]?1:0)+(ba[1]?1:0))*6;
			ba+=2;
		}
		for(i=jsbound;i<SBLIMIT;i++)
		{
			*ba = getbits_fast(fr, 4);
			needbits += (*ba?1:0)*12;
			++ba;
		}

		if(check_balloc(fr, balloc, ba)) return -1;

		ba = balloc;
		NEED_BITS(fr, needbits)
		for(i=0;i<jsbound;i++)
		{
			if ((*ba++))
				*sca++ = getbits_fast(fr, 6);
			if ((*ba++))
				*sca++ = getbits_fast(fr, 6);
		}
		for (i=jsbound;i<SBLIMIT;i++) if((*ba++))
		{
			*sca++ =  getbits_fast(fr, 6);
			*sca++ =  getbits_fast(fr, 6);
		}
	}
	else
	{
		int i;
		unsigned int needbits = SBLIMIT*4;

		NEED_BITS(fr, needbits)
		needbits = 0;
		for(i=0;i<SBLIMIT;i++)
		{
			*ba = getbits_fast(fr, 4);
			needbits += (*ba?1:0)*6;
			++ba;
		}

		if(check_balloc(fr, balloc, ba)) return -1;

		ba = balloc;
		NEED_BITS(fr, needbits)
		for (i=0;i<SBLIMIT;i++)
			if ((*ba++))
				*sca++ = getbits_fast(fr, 6);
	}

	return 0;
}

/* Something sane in place of undefined (-1)<<n. Well, not really. */
#define MINUS_SHIFT(n) ( (int)(((unsigned int)-1)<<(n)) )

static int I_step_two(real fraction[2][SBLIMIT],unsigned int balloc[2*SBLIMIT], unsigned int scale_index[2][SBLIMIT],mpg123_handle *fr)
{
	int i,n;
	int smpb[2*SBLIMIT]; /* values: 0-65535 */
	int *sample;
	register unsigned int *ba;
	register unsigned int *sca = (unsigned int *) scale_index;

	if(fr->stereo == 2)
	{
		unsigned int needbits = 0;
		int jsbound = fr->jsbound;
		register real *f0 = fraction[0];
		register real *f1 = fraction[1];

		ba = balloc;
		for(sample=smpb,i=0;i<jsbound;i++)
		{
			if((n=*ba++))
				needbits += n+1;
			if((n=*ba++))
				needbits += n+1;
		}
		for(i=jsbound;i<SBLIMIT;i++) 
			if((n = *ba++))
				needbits += n+1;
		NEED_BITS(fr, needbits)

		ba = balloc;
		for(sample=smpb,i=0;i<jsbound;i++)
		{
			if((n = *ba++)) *sample++ = getbits(fr, n+1);

			if((n = *ba++)) *sample++ = getbits(fr, n+1);
		}
		for(i=jsbound;i<SBLIMIT;i++) 
		if((n = *ba++))
		*sample++ = getbits(fr, n+1);

		ba = balloc;
		for(sample=smpb,i=0;i<jsbound;i++)
		{
			if((n=*ba++))
			*f0++ = REAL_MUL_SCALE_LAYER12(DOUBLE_TO_REAL_15(MINUS_SHIFT(n) + (*sample++) + 1), fr->muls[n+1][*sca++]);
			else *f0++ = DOUBLE_TO_REAL(0.0);

			if((n=*ba++))
			*f1++ = REAL_MUL_SCALE_LAYER12(DOUBLE_TO_REAL_15(MINUS_SHIFT(n) + (*sample++) + 1), fr->muls[n+1][*sca++]);
			else *f1++ = DOUBLE_TO_REAL(0.0);
		}
		for(i=jsbound;i<SBLIMIT;i++)
		{
			if((n=*ba++))
			{
				real samp = DOUBLE_TO_REAL_15(MINUS_SHIFT(n) + (*sample++) + 1);
				*f0++ = REAL_MUL_SCALE_LAYER12(samp, fr->muls[n+1][*sca++]);
				*f1++ = REAL_MUL_SCALE_LAYER12(samp, fr->muls[n+1][*sca++]);
			}
			else *f0++ = *f1++ = DOUBLE_TO_REAL(0.0);
		}
		for(i=fr->down_sample_sblimit;i<32;i++)
		fraction[0][i] = fraction[1][i] = 0.0;
	}
	else
	{
		unsigned int needbits = 0;
		register real *f0 = fraction[0];

		ba = balloc;
		for(sample=smpb,i=0;i<SBLIMIT;i++)
			if((n = *ba++))
				needbits += n+1;
		NEED_BITS(fr, needbits);

		ba = balloc;
		for(sample=smpb,i=0;i<SBLIMIT;i++)
		if ((n = *ba++))
		*sample++ = getbits(fr, n+1);


		ba = balloc;
		for(sample=smpb,i=0;i<SBLIMIT;i++)
		{
			if((n=*ba++))
			*f0++ = REAL_MUL_SCALE_LAYER12(DOUBLE_TO_REAL_15(MINUS_SHIFT(n) + (*sample++) + 1), fr->muls[n+1][*sca++]);
			else *f0++ = DOUBLE_TO_REAL(0.0);
		}
		for(i=fr->down_sample_sblimit;i<32;i++)
		fraction[0][i] = DOUBLE_TO_REAL(0.0);
	}
	return 0;
}

int do_layer1(mpg123_handle *fr)
{
	int clip=0;
	int i,stereo = fr->stereo;
	unsigned int balloc[2*SBLIMIT];
	unsigned int scale_index[2][SBLIMIT];
	real (*fraction)[SBLIMIT] = fr->layer1.fraction; /* fraction[2][SBLIMIT] */
	int single = fr->single;

	fr->jsbound = (fr->mode == MPG_MD_JOINT_STEREO) ? (fr->mode_ext<<2)+4 : 32;

	if(stereo == 1 || single == SINGLE_MIX) /* I don't see mixing handled here */
	single = SINGLE_LEFT;

	if(I_step_one(balloc,scale_index,fr))
	{
		if(NOQUIET)
			error("Aborting layer I decoding after step one.");
		return clip;
	}

	for(i=0;i<SCALE_BLOCK;i++)
	{
		if(I_step_two(fraction,balloc,scale_index,fr))
		{
			if(NOQUIET)
				error("Aborting layer I decoding after step two.");
			return clip;
		}

		if(single != SINGLE_STEREO)
		clip += (fr->synth_mono)(fraction[single], fr);
		else
		clip += (fr->synth_stereo)(fraction[0], fraction[1], fr);
	}

	return clip;
}


