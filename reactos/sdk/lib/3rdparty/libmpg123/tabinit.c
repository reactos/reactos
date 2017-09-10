/*
	tabinit.c: initialize tables...

	copyright ?-2008 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123lib_intern.h"
#include "debug.h"

/* That altivec alignment part here should not hurt generic code, I hope */
#ifdef OPT_ALTIVEC
static ALIGNED(16) real cos64[16];
static ALIGNED(16) real cos32[8];
static ALIGNED(16) real cos16[4];
static ALIGNED(16) real cos8[2];
static ALIGNED(16) real cos4[1];
#elif defined(REAL_IS_FIXED) && defined(PRECALC_TABLES)
static real cos64[16] = 
{
	8398725,8480395,8647771,8909416,9279544,9780026,10443886,11321405,
	12491246,14081950,16316987,19619946,24900150,34523836,57170182,170959967
};
static real cos32[8] =
{
	8429197,8766072,9511743,10851869,13223040,17795219,28897867,85583072
};
static real cos16[4] =
{
	8552951,10088893,15099095,42998586
};
static real cos8[2] =
{
	9079764,21920489
};
static real cos4[1] =
{
	11863283
};
#else
static real cos64[16],cos32[8],cos16[4],cos8[2],cos4[1];
#endif

real *pnts[] = { cos64,cos32,cos16,cos8,cos4 };


static long intwinbase[] = {
     0,    -1,    -1,    -1,    -1,    -1,    -1,    -2,    -2,    -2,
    -2,    -3,    -3,    -4,    -4,    -5,    -5,    -6,    -7,    -7,
    -8,    -9,   -10,   -11,   -13,   -14,   -16,   -17,   -19,   -21,
   -24,   -26,   -29,   -31,   -35,   -38,   -41,   -45,   -49,   -53,
   -58,   -63,   -68,   -73,   -79,   -85,   -91,   -97,  -104,  -111,
  -117,  -125,  -132,  -139,  -147,  -154,  -161,  -169,  -176,  -183,
  -190,  -196,  -202,  -208,  -213,  -218,  -222,  -225,  -227,  -228,
  -228,  -227,  -224,  -221,  -215,  -208,  -200,  -189,  -177,  -163,
  -146,  -127,  -106,   -83,   -57,   -29,     2,    36,    72,   111,
   153,   197,   244,   294,   347,   401,   459,   519,   581,   645,
   711,   779,   848,   919,   991,  1064,  1137,  1210,  1283,  1356,
  1428,  1498,  1567,  1634,  1698,  1759,  1817,  1870,  1919,  1962,
  2001,  2032,  2057,  2075,  2085,  2087,  2080,  2063,  2037,  2000,
  1952,  1893,  1822,  1739,  1644,  1535,  1414,  1280,  1131,   970,
   794,   605,   402,   185,   -45,  -288,  -545,  -814, -1095, -1388,
 -1692, -2006, -2330, -2663, -3004, -3351, -3705, -4063, -4425, -4788,
 -5153, -5517, -5879, -6237, -6589, -6935, -7271, -7597, -7910, -8209,
 -8491, -8755, -8998, -9219, -9416, -9585, -9727, -9838, -9916, -9959,
 -9966, -9935, -9863, -9750, -9592, -9389, -9139, -8840, -8492, -8092,
 -7640, -7134, -6574, -5959, -5288, -4561, -3776, -2935, -2037, -1082,
   -70,   998,  2122,  3300,  4533,  5818,  7154,  8540,  9975, 11455,
 12980, 14548, 16155, 17799, 19478, 21189, 22929, 24694, 26482, 28289,
 30112, 31947, 33791, 35640, 37489, 39336, 41176, 43006, 44821, 46617,
 48390, 50137, 51853, 53534, 55178, 56778, 58333, 59838, 61289, 62684,
 64019, 65290, 66494, 67629, 68692, 69679, 70590, 71420, 72169, 72835,
 73415, 73908, 74313, 74630, 74856, 74992, 75038 };

void prepare_decode_tables()
{
#if !defined(REAL_IS_FIXED) || !defined(PRECALC_TABLES)
  int i,k,kr,divv;
  real *costab;

  for(i=0;i<5;i++)
  {
    kr=0x10>>i; divv=0x40>>i;
    costab = pnts[i];
    for(k=0;k<kr;k++)
      costab[k] = DOUBLE_TO_REAL(1.0 / (2.0 * cos(M_PI * ((double) k * 2.0 + 1.0) / (double) divv)));
  }
#endif
}

#ifdef OPT_MMXORSSE
#if !defined(OPT_X86_64) && !defined(OPT_NEON) && !defined(OPT_NEON64) && !defined(OPT_AVX)
void make_decode_tables_mmx_asm(long scaleval, float* decwin_mmx, float *decwins);
void make_decode_tables_mmx(mpg123_handle *fr)
{
	debug("MMX decode tables");
	/* Take care: The scale should be like before, when we didn't have float output all around. */
	make_decode_tables_mmx_asm((long)((fr->lastscale < 0 ? fr->p.outscale : fr->lastscale)*SHORT_SCALE), fr->decwin_mmx, fr->decwins);
	debug("MMX decode tables done");
}
#else

/* This mimics round() as defined in C99. We stay C89. */
static int rounded(double f)
{
	return (int)(f>0 ? floor(f+0.5) : ceil(f-0.5));
}

/* x86-64 doesn't use asm version */
void make_decode_tables_mmx(mpg123_handle *fr)
{
	int i,j,val;
	int idx = 0;
	short *ptr = (short *)fr->decwins;
	/* Scale is always based on 1.0 . */
	double scaleval = -0.5*(fr->lastscale < 0 ? fr->p.outscale : fr->lastscale);
	debug1("MMX decode tables with scaleval %g", scaleval);
	for(i=0,j=0;i<256;i++,j++,idx+=32)
	{
		if(idx < 512+16)
		fr->decwin_mmx[idx+16] = fr->decwin_mmx[idx] = DOUBLE_TO_REAL((double) intwinbase[j] * scaleval);
		
		if(i % 32 == 31)
		idx -= 1023;
		if(i % 64 == 63)
		scaleval = - scaleval;
	}
	
	for( /* i=256 */ ;i<512;i++,j--,idx+=32)
	{
		if(idx < 512+16)
		fr->decwin_mmx[idx+16] = fr->decwin_mmx[idx] = DOUBLE_TO_REAL((double) intwinbase[j] * scaleval);
		
		if(i % 32 == 31)
		idx -= 1023;
		if(i % 64 == 63)
		scaleval = - scaleval;
	}
	
	for(i=0; i<512; i++) {
		if(i&1) val = rounded(fr->decwin_mmx[i]*0.5);
		else val = rounded(fr->decwin_mmx[i]*-0.5);
		if(val > 32767) val = 32767;
		else if(val < -32768) val = -32768;
		ptr[i] = val;
	}
	for(i=512; i<512+32; i++) {
		if(i&1) val = rounded(fr->decwin_mmx[i]*0.5);
		else val = 0;
		if(val > 32767) val = 32767;
		else if(val < -32768) val = -32768;
		ptr[i] = val;
	}
	for(i=0; i<512; i++) {
		val = rounded(fr->decwin_mmx[511-i]*-0.5);
		if(val > 32767) val = 32767;
		else if(val < -32768) val = -32768;
		ptr[512+32+i] = val;
	}
	debug("decode tables done");
}
#endif
#endif

#ifdef REAL_IS_FIXED
/* Need saturating multiplication that keeps table values in 32 bit range,
   with the option to swap sign at will (so -2**31 is out).
   This code is far from the decoder core and so assembly optimization might
   be overkill. */
static int32_t sat_mul32(int32_t a, int32_t b)
{
	int64_t prod = (int64_t)a * (int64_t)b;
	/* TODO: record the clipping? An extra flag? */
	if(prod >  2147483647L) return  2147483647L;
	if(prod < -2147483647L) return -2147483647L;
	return (int32_t)prod;
}
#endif

void make_decode_tables(mpg123_handle *fr)
{
	int i,j;
	int idx = 0;
	double scaleval;
#ifdef REAL_IS_FIXED
	real scaleval_long;
#endif
	/* Scale is always based on 1.0 . */
	scaleval = -0.5*(fr->lastscale < 0 ? fr->p.outscale : fr->lastscale);
	debug1("decode tables with scaleval %g", scaleval);
#ifdef REAL_IS_FIXED
	scaleval_long = DOUBLE_TO_REAL_15(scaleval);
	debug1("decode table with fixed scaleval %li", (long)scaleval_long);
	if(scaleval_long > 28618 || scaleval_long < -28618)
	{
		/* TODO: Limit the scaleval itself or limit the multiplication afterwards?
		   The former basically disables significant amplification for fixed-point
		   decoders, but avoids (possibly subtle) distortion. */
		/* This would limit the amplification instead:
		   scaleval_long = scaleval_long < 0 ? -28618 : 28618; */
		if(NOQUIET) warning("Desired amplification may introduce distortion.");
	}
#endif
	for(i=0,j=0;i<256;i++,j++,idx+=32)
	{
		if(idx < 512+16)
#ifdef REAL_IS_FIXED
		fr->decwin[idx+16] = fr->decwin[idx] =
			REAL_SCALE_WINDOW(sat_mul32(intwinbase[j],scaleval_long));
#else
		fr->decwin[idx+16] = fr->decwin[idx] = DOUBLE_TO_REAL((double) intwinbase[j] * scaleval);
#endif

		if(i % 32 == 31)
		idx -= 1023;
		if(i % 64 == 63)
#ifdef REAL_IS_FIXED
		scaleval_long = - scaleval_long;
#else
		scaleval = - scaleval;
#endif
	}

	for( /* i=256 */ ;i<512;i++,j--,idx+=32)
	{
		if(idx < 512+16)
#ifdef REAL_IS_FIXED
		fr->decwin[idx+16] = fr->decwin[idx] =
			REAL_SCALE_WINDOW(sat_mul32(intwinbase[j],scaleval_long));
#else
		fr->decwin[idx+16] = fr->decwin[idx] = DOUBLE_TO_REAL((double) intwinbase[j] * scaleval);
#endif

		if(i % 32 == 31)
		idx -= 1023;
		if(i % 64 == 63)
#ifdef REAL_IS_FIXED
		scaleval_long = - scaleval_long;
#else
		scaleval = - scaleval;
#endif
	}
#if defined(OPT_X86_64) || defined(OPT_ALTIVEC) || defined(OPT_SSE) || defined(OPT_SSE_VINTAGE) || defined(OPT_ARM) || defined(OPT_NEON) || defined(OPT_NEON64) || defined(OPT_AVX)
	if(  fr->cpu_opts.type == x86_64
	  || fr->cpu_opts.type == altivec
	  || fr->cpu_opts.type == sse
	  || fr->cpu_opts.type == sse_vintage
	  || fr->cpu_opts.type == arm
	  || fr->cpu_opts.type == neon
	  || fr->cpu_opts.type == neon64
	  || fr->cpu_opts.type == avx )
	{ /* for float SSE / AltiVec / ARM decoder */
		for(i=512; i<512+32; i++)
		{
			fr->decwin[i] = (i&1) ? fr->decwin[i] : 0;
		}
		for(i=0; i<512; i++)
		{
			fr->decwin[512+32+i] = -fr->decwin[511-i];
		}
#if defined(OPT_NEON) || defined(OPT_NEON64)
		if(fr->cpu_opts.type == neon || fr->cpu_opts.type == neon64)
		{
			for(i=0; i<512; i+=2)
			{
				fr->decwin[i] = -fr->decwin[i];
			}
		}
#endif
	}
#endif
	debug("decode tables done");
}

#ifndef NO_8BIT
int make_conv16to8_table(mpg123_handle *fr)
{
  int i;
	int mode = fr->af.dec_enc;

  /*
   * ????: 8.0 is right but on SB cards '2.0' is a better value ???
   */
  const double mul = 8.0;

  if(!fr->conv16to8_buf){
    fr->conv16to8_buf = (unsigned char *) malloc(8192);
    if(!fr->conv16to8_buf) {
      fr->err = MPG123_ERR_16TO8TABLE;
      if(NOQUIET) error("Can't allocate 16 to 8 converter table!");
      return -1;
    }
    fr->conv16to8 = fr->conv16to8_buf + 4096;
  }

	switch(mode)
	{
	case MPG123_ENC_ULAW_8:
	{
		double m=127.0 / log(256.0);
		int c1;

		for(i=-4096;i<4096;i++)
		{
			/* dunno whether this is a valid transformation rule ?!?!? */
			if(i < 0)
			c1 = 127 - (int) (log( 1.0 - 255.0 * (double) i*mul / 32768.0 ) * m);
			else
			c1 = 255 - (int) (log( 1.0 + 255.0 * (double) i*mul / 32768.0 ) * m);
			if(c1 < 0 || c1 > 255)
			{
				if(NOQUIET) error2("Converror %d %d",i,c1);
				return -1;
			}
			if(c1 == 0)
			c1 = 2;
			fr->conv16to8[i] = (unsigned char) c1;
		}
	}
	break;
	case MPG123_ENC_SIGNED_8:
		for(i=-4096;i<4096;i++)
		fr->conv16to8[i] = i>>5;
	break;
	case MPG123_ENC_UNSIGNED_8:
		for(i=-4096;i<4096;i++)
		fr->conv16to8[i] = (i>>5)+128;
	break;
	case MPG123_ENC_ALAW_8:
	{
		/*
			Let's believe Wikipedia (http://en.wikipedia.org/wiki/G.711) that this
			is the correct table:

			s0000000wxyza... 	n000wxyz  [0-31] -> [0-15]
			s0000001wxyza... 	n001wxyz  [32-63] -> [16-31]
			s000001wxyzab... 	n010wxyz  [64-127] -> [32-47]
			s00001wxyzabc... 	n011wxyz  [128-255] -> [48-63]
			s0001wxyzabcd... 	n100wxyz  [256-511] -> [64-79]
			s001wxyzabcde... 	n101wxyz  [512-1023] -> [80-95]
			s01wxyzabcdef... 	n110wxyz  [1024-2047] -> [96-111]
			s1wxyzabcdefg... 	n111wxyz  [2048-4095] -> [112-127]

			Let's extend to -4096, too.
			Also, bytes are xored with 0x55 for transmission.

			Since it sounds OK, I assume it is fine;-)
		*/
		for(i=0; i<64; ++i)
		fr->conv16to8[i] = ((unsigned int)i)>>1;
		for(i=64; i<128; ++i)
		fr->conv16to8[i] = ((((unsigned int)i)>>2) & 0xf) | (2<<4);
		for(i=128; i<256; ++i)
		fr->conv16to8[i] = ((((unsigned int)i)>>3) & 0xf) | (3<<4);
		for(i=256; i<512; ++i)
		fr->conv16to8[i] = ((((unsigned int)i)>>4) & 0xf) | (4<<4);
		for(i=512; i<1024; ++i)
		fr->conv16to8[i] = ((((unsigned int)i)>>5) & 0xf) | (5<<4);
		for(i=1024; i<2048; ++i)
		fr->conv16to8[i] = ((((unsigned int)i)>>6) & 0xf) | (6<<4);
		for(i=2048; i<4096; ++i)
		fr->conv16to8[i] = ((((unsigned int)i)>>7) & 0xf) | (7<<4);

		for(i=-4095; i<0; ++i)
		fr->conv16to8[i] = fr->conv16to8[-i] | 0x80;

		fr->conv16to8[-4096] = fr->conv16to8[-4095];

		for(i=-4096;i<4096;i++)
		{
			/* fr->conv16to8[i] = - i>>5; */
			/* fprintf(stderr, "table %i %i\n", i<<AUSHIFT, fr->conv16to8[i]); */
			fr->conv16to8[i] ^= 0x55;
		}
	}
	break;
	default:
		fr->err = MPG123_ERR_16TO8TABLE;
		if(NOQUIET) error("Unknown 8 bit encoding choice.");
		return -1;
	break;
	}

	return 0;
}
#endif

