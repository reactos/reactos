/*
	optimize: get a grip on the different optimizations

	copyright 2006-9 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, inspired by 3DNow stuff in mpg123.[hc]

	Currently, this file contains the struct and function to choose an optimization variant and works only when OPT_MULTI is in effect.
*/

#define I_AM_OPTIMIZE
#include "mpg123lib_intern.h" /* includes optimize.h */
#include "debug.h"

#if ((defined OPT_X86) || (defined OPT_X86_64) || (defined OPT_NEON) || (defined OPT_NEON64)) && (defined OPT_MULTI)
#include "getcpuflags.h"
static struct cpuflags cpu_flags;
#else
/* Faking stuff for non-multi builds. The same code for synth function choice is used.
   Just no runtime dependency of result... */
#define cpu_flags nothing
#define cpu_i586(s)     1
#define cpu_fpu(s)      1
#define cpu_mmx(s)      1
#define cpu_3dnow(s)    1
#define cpu_3dnowext(s) 1
#define cpu_sse(s)      1
#define cpu_sse2(s)     1
#define cpu_sse3(s)     1
#define cpu_avx(s)      1
#define cpu_neon(s)     1
#endif

/* Ugly macros to build conditional synth function array values. */

#ifndef NO_8BIT
#define IF8(synth) synth,
#else
#define IF8(synth)
#endif

#ifndef NO_SYNTH32

#ifndef NO_REAL
#define IFREAL(synth) synth,
#else
#define IFREAL(synth)
#endif

#ifndef NO_32BIT
#define IF32(synth) synth
#else
#define IF32(synth)
#endif

#else

#define IFREAL(synth)
#define IF32(synth)

#endif

#ifndef NO_16BIT
#	define OUT_SYNTHS(synth_16, synth_8, synth_real, synth_32) { synth_16, IF8(synth_8) IFREAL(synth_real) IF32(synth_32) }
#else
#	define OUT_SYNTHS(synth_16, synth_8, synth_real, synth_32) { IF8(synth_8) IFREAL(synth_real) IF32(synth_32) }
#endif

/* The call of left and right plain synth, wrapped.
   This may be replaced by a direct stereo optimized synth. */
static int synth_stereo_wrap(real *bandPtr_l, real *bandPtr_r, mpg123_handle *fr)
{
	int clip;
	clip  = (fr->synth)(bandPtr_l, 0, fr, 0);
	clip += (fr->synth)(bandPtr_r, 1, fr, 1);
	return clip;
}

static const struct synth_s synth_base =
{
	{ /* plain */
		 OUT_SYNTHS(synth_1to1, synth_1to1_8bit, synth_1to1_real, synth_1to1_s32)
#		ifndef NO_DOWNSAMPLE
		,OUT_SYNTHS(synth_2to1, synth_2to1_8bit, synth_2to1_real, synth_2to1_s32)
		,OUT_SYNTHS(synth_4to1, synth_4to1_8bit, synth_4to1_real, synth_4to1_s32)
#		endif
#		ifndef NO_NTOM
		,OUT_SYNTHS(synth_ntom, synth_ntom_8bit, synth_ntom_real, synth_ntom_s32)
#		endif
	},
	{ /* stereo, by default only wrappers over plain synth */
		 OUT_SYNTHS(synth_stereo_wrap, synth_stereo_wrap, synth_stereo_wrap, synth_stereo_wrap)
#		ifndef NO_DOWNSAMPLE
		,OUT_SYNTHS(synth_stereo_wrap, synth_stereo_wrap, synth_stereo_wrap, synth_stereo_wrap)
		,OUT_SYNTHS(synth_stereo_wrap, synth_stereo_wrap, synth_stereo_wrap, synth_stereo_wrap)
#		endif
#		ifndef NO_NTOM
		,OUT_SYNTHS(synth_stereo_wrap, synth_stereo_wrap, synth_stereo_wrap, synth_stereo_wrap)
#		endif
	},
	{ /* mono2stereo */
		 OUT_SYNTHS(synth_1to1_m2s, synth_1to1_8bit_m2s, synth_1to1_real_m2s, synth_1to1_s32_m2s)
#		ifndef NO_DOWNSAMPLE
		,OUT_SYNTHS(synth_2to1_m2s, synth_2to1_8bit_m2s, synth_2to1_real_m2s, synth_2to1_s32_m2s)
		,OUT_SYNTHS(synth_4to1_m2s, synth_4to1_8bit_m2s, synth_4to1_real_m2s, synth_4to1_s32_m2s)
#		endif
#		ifndef NO_NTOM
		,OUT_SYNTHS(synth_ntom_m2s, synth_ntom_8bit_m2s, synth_ntom_real_m2s, synth_ntom_s32_m2s)
#		endif
	},
	{ /* mono*/
		 OUT_SYNTHS(synth_1to1_mono, synth_1to1_8bit_mono, synth_1to1_real_mono, synth_1to1_s32_mono)
#		ifndef NO_DOWNSAMPLE
		,OUT_SYNTHS(synth_2to1_mono, synth_2to1_8bit_mono, synth_2to1_real_mono, synth_2to1_s32_mono)
		,OUT_SYNTHS(synth_4to1_mono, synth_4to1_8bit_mono, synth_4to1_real_mono, synth_4to1_s32_mono)
#		endif
#		ifndef NO_NTOM
		,OUT_SYNTHS(synth_ntom_mono, synth_ntom_8bit_mono, synth_ntom_real_mono, synth_ntom_s32_mono)
#endif
	}
};

#ifdef OPT_X86
/* More plain synths for i386 */
const func_synth plain_i386[r_limit][f_limit] =
{ /* plain */
	 OUT_SYNTHS(synth_1to1_i386, synth_1to1_8bit_i386, synth_1to1_real_i386, synth_1to1_s32_i386)
#	ifndef NO_DOWNSAMPLE
	,OUT_SYNTHS(synth_2to1_i386, synth_2to1_8bit_i386, synth_2to1_real_i386, synth_2to1_s32_i386)
	,OUT_SYNTHS(synth_4to1_i386, synth_4to1_8bit_i386, synth_4to1_real_i386, synth_4to1_s32_i386)
#	endif
#	ifndef NO_NTOM
	,OUT_SYNTHS(synth_ntom, synth_ntom_8bit, synth_ntom_real, synth_ntom_s32)
#	endif
};
#endif


enum optdec defdec(void){ return defopt; }

enum optcla decclass(const enum optdec type)
{
	return
	(
		   type == mmx
		|| type == sse
		|| type == sse_vintage
		|| type == dreidnowext
		|| type == dreidnowext_vintage
		|| type == x86_64
		|| type == neon
		|| type == neon64
		|| type == avx
	) ? mmxsse : normal;
}

static int find_synth(func_synth synth,  const func_synth synths[r_limit][f_limit])
{
	enum synth_resample ri;
	enum synth_format   fi;
	for(ri=0; ri<r_limit; ++ri)
	for(fi=0; fi<f_limit; ++fi)
	if(synth == synths[ri][fi])
	return TRUE;

	return FALSE;
}


#if defined(OPT_SSE) || defined(OPT_SSE_VINTAGE)
/* After knowing that it is either vintage or current SSE,
   this separates the two. In case of non-OPT_MULTI, only one
   of OPT_SSE and OPT_SSE_VINTAGE is active. */
static enum optdec sse_or_vintage(mpg123_handle *fr)
{
	enum optdec type;
	type = sse_vintage;
#	ifdef OPT_SSE
#	ifdef OPT_MULTI
	if(fr->cpu_opts.the_dct36 == dct36_sse)
#	endif
	type = sse;
#	endif
	return type;
}
#endif

/* Determine what kind of decoder is actually active
   This depends on runtime choices which may cause fallback to i386 or generic code. */
static int find_dectype(mpg123_handle *fr)
{
	enum optdec type = nodec;
	/* Direct and indirect usage, 1to1 stereo decoding.
	   Concentrating on the plain stereo synth should be fine, mono stuff is derived. */
	func_synth basic_synth = fr->synth;
#ifndef NO_8BIT
#ifndef NO_16BIT
	if(basic_synth == synth_1to1_8bit_wrap)
	basic_synth = fr->synths.plain[r_1to1][f_16]; /* That is what's really below the surface. */
#endif
#endif

	if(FALSE) ; /* Just to initialize the else if ladder. */
#ifndef NO_16BIT
#if defined(OPT_3DNOWEXT) || defined(OPT_3DNOWEXT_VINTAGE)
	else if(basic_synth == synth_1to1_3dnowext)
	{
		type = dreidnowext;
#		ifdef OPT_3DNOWEXT_VINTAGE
#		ifdef OPT_MULTI
		if(fr->cpu_opts.the_dct36 == dct36_3dnowext)
#		endif
		type = dreidnowext_vintage;
#		endif
	}
#endif
#if defined(OPT_SSE) || defined(OPT_SSE_VINTAGE)
	else if(basic_synth == synth_1to1_sse)
	{
		type = sse_or_vintage(fr);
	}
#endif
#if defined(OPT_3DNOW) || defined(OPT_3DNOW_VINTAGE)
	else if(basic_synth == synth_1to1_3dnow)
	{
		type = dreidnow;
#		ifdef OPT_3DNOW_VINTAGE
#		ifdef OPT_MULTI
		if(fr->cpu_opts.the_dct36 == dct36_3dnow)
#		endif
		type = dreidnow_vintage;
#		endif
	}
#endif
#ifdef OPT_MMX
	else if(basic_synth == synth_1to1_mmx) type = mmx;
#endif
#ifdef OPT_I586_DITHER
	else if(basic_synth == synth_1to1_i586_dither) type = ifuenf_dither;
#endif
#ifdef OPT_I586
	else if(basic_synth == synth_1to1_i586) type = ifuenf;
#endif
#ifdef OPT_ALTIVEC
	else if(basic_synth == synth_1to1_altivec) type = altivec;
#endif
#ifdef OPT_X86_64
	else if(basic_synth == synth_1to1_x86_64) type = x86_64;
#endif
#ifdef OPT_AVX
	else if(basic_synth == synth_1to1_avx) type = avx;
#endif
#ifdef OPT_ARM
	else if(basic_synth == synth_1to1_arm) type = arm;
#endif
#ifdef OPT_NEON
	else if(basic_synth == synth_1to1_neon) type = neon;
#endif
#ifdef OPT_NEON64
	else if(basic_synth == synth_1to1_neon64) type = neon64;
#endif
#ifdef OPT_GENERIC_DITHER
	else if(basic_synth == synth_1to1_dither) type = generic_dither;
#endif
#ifdef OPT_DITHER /* either i586 or generic! */
#ifndef NO_DOWNSAMPLE
	else if
	(
		   basic_synth == synth_2to1_dither
		|| basic_synth == synth_4to1_dither
	) type = generic_dither;
#endif
#endif
#endif /* 16bit */

#ifndef NO_SYNTH32

#ifndef NO_REAL
#if defined(OPT_SSE) || defined(OPT_SSE_VINTAGE)
	else if(basic_synth == synth_1to1_real_sse)
	{
		type = sse_or_vintage(fr);
	}
#endif
#ifdef OPT_X86_64
	else if(basic_synth == synth_1to1_real_x86_64) type = x86_64;
#endif
#ifdef OPT_AVX
	else if(basic_synth == synth_1to1_real_avx) type = avx;
#endif
#ifdef OPT_ALTIVEC
	else if(basic_synth == synth_1to1_real_altivec) type = altivec;
#endif
#ifdef OPT_NEON
	else if(basic_synth == synth_1to1_real_neon) type = neon;
#endif
#ifdef OPT_NEON64
	else if(basic_synth == synth_1to1_real_neon64) type = neon64;
#endif

#endif /* real */

#ifndef NO_32BIT
#if defined(OPT_SSE) || defined(OPT_SSE_VINTAGE)
	else if(basic_synth == synth_1to1_s32_sse)
	{
		type = sse_or_vintage(fr);
	}
#endif
#ifdef OPT_X86_64
	else if(basic_synth == synth_1to1_s32_x86_64) type = x86_64;
#endif
#ifdef OPT_AVX
	else if(basic_synth == synth_1to1_s32_avx) type = avx;
#endif
#ifdef OPT_ALTIVEC
	else if(basic_synth == synth_1to1_s32_altivec) type = altivec;
#endif
#ifdef OPT_NEON
	else if(basic_synth == synth_1to1_s32_neon) type = neon;
#endif
#ifdef OPT_NEON64
	else if(basic_synth == synth_1to1_s32_neon64) type = neon64;
#endif
#endif /* 32bit */

#endif /* any 32 bit synth */

#ifdef OPT_X86
	else if(find_synth(basic_synth, plain_i386))
	type = idrei;
#endif

	else if(find_synth(basic_synth, synth_base.plain))
	type = generic;



#ifdef OPT_I486
	/* i486 is special ... the specific code is in use for 16bit 1to1 stereo
	   otherwise we have i386 active... but still, the distinction doesn't matter*/
	type = ivier;
#endif

	if(type != nodec)
	{
		fr->cpu_opts.type = type;
		fr->cpu_opts.class = decclass(type);

		debug3("determined active decoder type %i (%s) of class %i", type, decname[type], fr->cpu_opts.class);
		return MPG123_OK;
	}
	else
	{
		if(NOQUIET) error("Unable to determine active decoder type -- this is SERIOUS b0rkage!");

		fr->err = MPG123_BAD_DECODER_SETUP;
		return MPG123_ERR;
	}
}

/* set synth functions for current frame, optimizations handled by opt_* macros */
int set_synth_functions(mpg123_handle *fr)
{
	enum synth_resample resample = r_none;
	enum synth_format basic_format = f_none; /* Default is always 16bit, or whatever. */

	/* Select the basic output format, different from 16bit: 8bit, real. */
	if(FALSE){}
#ifndef NO_16BIT
	else if(fr->af.dec_enc & MPG123_ENC_16)
	basic_format = f_16;
#endif
#ifndef NO_8BIT
	else if(fr->af.dec_enc & MPG123_ENC_8)
	basic_format = f_8;
#endif
#ifndef NO_REAL
	else if(fr->af.dec_enc & MPG123_ENC_FLOAT)
	basic_format = f_real;
#endif
#ifndef NO_32BIT
	/* 24 bit integer means decoding to 32 bit first. */
	else if(fr->af.dec_enc & MPG123_ENC_32 || fr->af.dec_enc & MPG123_ENC_24)
	basic_format = f_32;
#endif

	/* Make sure the chosen format is compiled into this lib. */
	if(basic_format == f_none)
	{
		if(NOQUIET) error("set_synth_functions: This output format is disabled in this build!");

		return -1;
	}

	/* Be explicit about downsampling variant. */
	switch(fr->down_sample)
	{
		case 0: resample = r_1to1; break;
#ifndef NO_DOWNSAMPLE
		case 1: resample = r_2to1; break;
		case 2: resample = r_4to1; break;
#endif
#ifndef NO_NTOM
		case 3: resample = r_ntom; break;
#endif
	}

	if(resample == r_none)
	{
		if(NOQUIET) error("set_synth_functions: This resampling mode is not supported in this build!");

		return -1;
	}

	debug2("selecting synth: resample=%i format=%i", resample, basic_format);
	/* Finally selecting the synth functions for stereo / mono. */
	fr->synth = fr->synths.plain[resample][basic_format];
	fr->synth_stereo = fr->synths.stereo[resample][basic_format];
	fr->synth_mono = fr->af.channels==2
		? fr->synths.mono2stereo[resample][basic_format] /* Mono MPEG file decoded to stereo. */
		: fr->synths.mono[resample][basic_format];       /* Mono MPEG file decoded to mono. */

	if(find_dectype(fr) != MPG123_OK) /* Actually determine the currently active decoder breed. */
	{
		fr->err = MPG123_BAD_DECODER_SETUP;
		return MPG123_ERR;
	}

	if(frame_buffers(fr) != 0)
	{
		fr->err = MPG123_NO_BUFFERS;
		if(NOQUIET) error("Failed to set up decoder buffers!");

		return MPG123_ERR;
	}

#ifndef NO_8BIT
	if(basic_format == f_8)
	{
		if(make_conv16to8_table(fr) != 0)
		{
			if(NOQUIET) error("Failed to set up conv16to8 table!");
			/* it's a bit more work to get proper error propagation up */
			return -1;
		}
	}
#endif

#ifdef OPT_MMXORSSE
	/* Special treatment for MMX, SSE and 3DNowExt stuff.
	   The real-decoding SSE for x86-64 uses normal tables! */
	if(fr->cpu_opts.class == mmxsse
#	ifndef NO_REAL
	   && basic_format != f_real
#	endif
#	ifndef NO_32BIT
	   && basic_format != f_32
#	endif
#	ifdef ACCURATE_ROUNDING
	   && fr->cpu_opts.type != sse
	   && fr->cpu_opts.type != sse_vintage
	   && fr->cpu_opts.type != x86_64
	   && fr->cpu_opts.type != neon
	   && fr->cpu_opts.type != neon64
	   && fr->cpu_opts.type != avx
#	endif
	  )
	{
#ifndef NO_LAYER3
		init_layer3_stuff(fr, init_layer3_gainpow2_mmx);
#endif
#ifndef NO_LAYER12
		init_layer12_stuff(fr, init_layer12_table_mmx);
#endif
		fr->make_decode_tables = make_decode_tables_mmx;
	}
	else
#endif
	{
#ifndef NO_LAYER3
		init_layer3_stuff(fr, init_layer3_gainpow2);
#endif
#ifndef NO_LAYER12
		init_layer12_stuff(fr, init_layer12_table);
#endif
		fr->make_decode_tables = make_decode_tables;
	}

	/* We allocated the table buffers just now, so (re)create the tables. */
	fr->make_decode_tables(fr);

	return 0;
}

int frame_cpu_opt(mpg123_handle *fr, const char* cpu)
{
	const char* chosen = ""; /* the chosen decoder opt as string */
	enum optdec want_dec = nodec;
	int done = 0;
	int auto_choose = 0;
#ifdef OPT_DITHER
	int dithered = FALSE; /* If some dithered decoder is chosen. */
#endif

	want_dec = dectype(cpu);
	auto_choose = want_dec == autodec;
	/* Fill whole array of synth functions with generic code first. */
	fr->synths = synth_base;

#ifndef OPT_MULTI
	{
		if(!auto_choose && want_dec != defopt)
		{
			if(NOQUIET) error2("you wanted decoder type %i, I only have %i", want_dec, defopt);
		}
		auto_choose = TRUE; /* There will be only one choice anyway. */
	}
#endif

	fr->cpu_opts.type = nodec;
#ifdef OPT_MULTI
#ifndef NO_LAYER3
#if (defined OPT_3DNOW_VINTAGE || defined OPT_3DNOWEXT_VINTAGE || defined OPT_SSE || defined OPT_X86_64 || defined OPT_AVX || defined OPT_NEON || defined OPT_NEON64)
	fr->cpu_opts.the_dct36 = dct36;
#endif
#endif
#endif
	/* covers any i386+ cpu; they actually differ only in the synth_1to1 function, mostly... */
#ifdef OPT_X86
	if(cpu_i586(cpu_flags))
	{
#		ifdef OPT_MULTI
		debug2("standard flags: 0x%08x\textended flags: 0x%08x", cpu_flags.std, cpu_flags.ext);
#		endif
#		ifdef OPT_SSE
		if(   !done && (auto_choose || want_dec == sse)
		   && cpu_sse(cpu_flags) && cpu_mmx(cpu_flags) )
		{
			chosen = dn_sse;
			fr->cpu_opts.type = sse;
#ifdef OPT_MULTI
#			ifndef NO_LAYER3
			/* if(cpu_fast_sse(cpu_flags)) */ fr->cpu_opts.the_dct36 = dct36_sse;
#			endif
#endif
#			ifndef NO_16BIT
			fr->synths.plain[r_1to1][f_16] = synth_1to1_sse;
#			ifdef ACCURATE_ROUNDING
			fr->synths.stereo[r_1to1][f_16] = synth_1to1_stereo_sse;
#			endif
#			endif
#			ifndef NO_REAL
			fr->synths.plain[r_1to1][f_real] = synth_1to1_real_sse;
			fr->synths.stereo[r_1to1][f_real] = synth_1to1_real_stereo_sse;
#			endif
#			ifndef NO_32BIT
			fr->synths.plain[r_1to1][f_32] = synth_1to1_s32_sse;
			fr->synths.stereo[r_1to1][f_32] = synth_1to1_s32_stereo_sse;
#			endif
			done = 1;
		}
#		endif
#		ifdef OPT_SSE_VINTAGE
		if(   !done && (auto_choose || want_dec == sse_vintage)
		   && cpu_sse(cpu_flags) && cpu_mmx(cpu_flags) )
		{
			chosen = dn_sse_vintage;
			fr->cpu_opts.type = sse_vintage;
#			ifndef NO_16BIT
			fr->synths.plain[r_1to1][f_16] = synth_1to1_sse;
#			ifdef ACCURATE_ROUNDING
			fr->synths.stereo[r_1to1][f_16] = synth_1to1_stereo_sse;
#			endif
#			endif
#			ifndef NO_REAL
			fr->synths.plain[r_1to1][f_real] = synth_1to1_real_sse;
			fr->synths.stereo[r_1to1][f_real] = synth_1to1_real_stereo_sse;
#			endif
#			ifndef NO_32BIT
			fr->synths.plain[r_1to1][f_32] = synth_1to1_s32_sse;
			fr->synths.stereo[r_1to1][f_32] = synth_1to1_s32_stereo_sse;
#			endif
			done = 1;
		}
#		endif
#		ifdef OPT_3DNOWEXT
		if(   !done && (auto_choose || want_dec == dreidnowext)
		   && cpu_3dnow(cpu_flags)
		   && cpu_3dnowext(cpu_flags)
		   && cpu_mmx(cpu_flags) )
		{
			chosen = dn_dreidnowext;
			fr->cpu_opts.type = dreidnowext;
#			ifndef NO_16BIT
			fr->synths.plain[r_1to1][f_16] = synth_1to1_3dnowext;
#			endif
			done = 1;
		}
#		endif
#		ifdef OPT_3DNOWEXT_VINTAGE
		if(   !done && (auto_choose || want_dec == dreidnowext_vintage)
		   && cpu_3dnow(cpu_flags)
		   && cpu_3dnowext(cpu_flags)
		   && cpu_mmx(cpu_flags) )
		{
			chosen = dn_dreidnowext_vintage;
			fr->cpu_opts.type = dreidnowext_vintage;
#ifdef OPT_MULTI
#			ifndef NO_LAYER3
			fr->cpu_opts.the_dct36 = dct36_3dnowext;
#			endif
#endif
#			ifndef NO_16BIT
			fr->synths.plain[r_1to1][f_16] = synth_1to1_3dnowext;
#			endif
			done = 1;
		}
#		endif
#		ifdef OPT_3DNOW
		if(    !done && (auto_choose || want_dec == dreidnow)
		    && cpu_3dnow(cpu_flags) && cpu_mmx(cpu_flags) )
		{
			chosen = dn_dreidnow;
			fr->cpu_opts.type = dreidnow;
#			ifndef NO_16BIT
			fr->synths.plain[r_1to1][f_16] = synth_1to1_3dnow;
#			endif
			done = 1;
		}
#		endif
#		ifdef OPT_3DNOW_VINTAGE
		if(    !done && (auto_choose || want_dec == dreidnow_vintage)
		    && cpu_3dnow(cpu_flags) && cpu_mmx(cpu_flags) )
		{
			chosen = dn_dreidnow_vintage;
			fr->cpu_opts.type = dreidnow_vintage;
#ifdef OPT_MULTI
#			ifndef NO_LAYER3
			fr->cpu_opts.the_dct36 = dct36_3dnow;
#			endif
#endif
#			ifndef NO_16BIT
			fr->synths.plain[r_1to1][f_16] = synth_1to1_3dnow;
#			endif
			done = 1;
		}
#		endif
		#ifdef OPT_MMX
		if(   !done && (auto_choose || want_dec == mmx)
		   && cpu_mmx(cpu_flags) )
		{
			chosen = dn_mmx;
			fr->cpu_opts.type = mmx;
#			ifndef NO_16BIT
			fr->synths.plain[r_1to1][f_16] = synth_1to1_mmx;
#			endif
			done = 1;
		}
		#endif
		#ifdef OPT_I586
		if(!done && (auto_choose || want_dec == ifuenf))
		{
			chosen = "i586/pentium";
			fr->cpu_opts.type = ifuenf;
#			ifndef NO_16BIT
			fr->synths.plain[r_1to1][f_16] = synth_1to1_i586;
#			endif
			done = 1;
		}
		#endif
		#ifdef OPT_I586_DITHER
		if(!done && (auto_choose || want_dec == ifuenf_dither))
		{
			chosen = "dithered i586/pentium";
			fr->cpu_opts.type = ifuenf_dither;
			dithered = TRUE;
#			ifndef NO_16BIT
			fr->synths.plain[r_1to1][f_16] = synth_1to1_i586_dither;
#			ifndef NO_DOWNSAMPLE
			fr->synths.plain[r_2to1][f_16] = synth_2to1_dither;
			fr->synths.plain[r_4to1][f_16] = synth_4to1_dither;
#			endif
#			endif
			done = 1;
		}
		#endif
	}
	#ifdef OPT_I486
	/* That won't cooperate in multi opt mode - forcing i486 in layer3.c
	   But still... here it is... maybe for real use in future. */
	if(!done && (auto_choose || want_dec == ivier))
	{
		chosen = dn_ivier;
		fr->cpu_opts.type = ivier;
		done = 1;
	}
	#endif
	#ifdef OPT_I386
	if(!done && (auto_choose || want_dec == idrei))
	{
		chosen = dn_idrei;
		fr->cpu_opts.type = idrei;
		done = 1;
	}
	#endif

	if(done)
	{
		/*
			We have chosen some x86 decoder... fillup some i386 stuff.
			There is an open question about using dithered synth_1to1 for 8bit wrappers.
			For quality it won't make sense, but wrapped i586_dither wrapped may still be faster...
		*/
		enum synth_resample ri;
		enum synth_format   fi;
#		ifndef NO_8BIT
#		ifndef NO_16BIT /* possibility to use a 16->8 wrapper... */
		if(fr->synths.plain[r_1to1][f_16] != synth_base.plain[r_1to1][f_16])
		{
			fr->synths.plain[r_1to1][f_8] = synth_1to1_8bit_wrap;
			fr->synths.mono[r_1to1][f_8] = synth_1to1_8bit_wrap_mono;
			fr->synths.mono2stereo[r_1to1][f_8] = synth_1to1_8bit_wrap_m2s;
		}
#		endif
#		endif
		for(ri=0; ri<r_limit; ++ri)
		for(fi=0; fi<f_limit; ++fi)
		{
			if(fr->synths.plain[ri][fi] == synth_base.plain[ri][fi])
			fr->synths.plain[ri][fi] = plain_i386[ri][fi];
		}
	}

#endif /* OPT_X86 */

#ifdef OPT_AVX
	if(!done && (auto_choose || want_dec == avx) && cpu_avx(cpu_flags))
	{
		chosen = "x86-64 (AVX)";
		fr->cpu_opts.type = avx;
#ifdef OPT_MULTI
#		ifndef NO_LAYER3
		fr->cpu_opts.the_dct36 = dct36_avx;
#		endif
#endif
#		ifndef NO_16BIT
		fr->synths.plain[r_1to1][f_16] = synth_1to1_avx;
		fr->synths.stereo[r_1to1][f_16] = synth_1to1_stereo_avx;
#		endif
#		ifndef NO_REAL
		fr->synths.plain[r_1to1][f_real] = synth_1to1_real_avx;
		fr->synths.stereo[r_1to1][f_real] = synth_1to1_real_stereo_avx;
#		endif
#		ifndef NO_32BIT
		fr->synths.plain[r_1to1][f_32] = synth_1to1_s32_avx;
		fr->synths.stereo[r_1to1][f_32] = synth_1to1_s32_stereo_avx;
#		endif
		done = 1;
	}
#endif

#ifdef OPT_X86_64
	if(!done && (auto_choose || want_dec == x86_64))
	{
		chosen = "x86-64 (SSE)";
		fr->cpu_opts.type = x86_64;
#ifdef OPT_MULTI
#		ifndef NO_LAYER3
		fr->cpu_opts.the_dct36 = dct36_x86_64;
#		endif
#endif
#		ifndef NO_16BIT
		fr->synths.plain[r_1to1][f_16] = synth_1to1_x86_64;
		fr->synths.stereo[r_1to1][f_16] = synth_1to1_stereo_x86_64;
#		endif
#		ifndef NO_REAL
		fr->synths.plain[r_1to1][f_real] = synth_1to1_real_x86_64;
		fr->synths.stereo[r_1to1][f_real] = synth_1to1_real_stereo_x86_64;
#		endif
#		ifndef NO_32BIT
		fr->synths.plain[r_1to1][f_32] = synth_1to1_s32_x86_64;
		fr->synths.stereo[r_1to1][f_32] = synth_1to1_s32_stereo_x86_64;
#		endif
		done = 1;
	}
#endif

#	ifdef OPT_ALTIVEC
	if(!done && (auto_choose || want_dec == altivec))
	{
		chosen = dn_altivec;
		fr->cpu_opts.type = altivec;
#		ifndef NO_16BIT
		fr->synths.plain[r_1to1][f_16] = synth_1to1_altivec;
		fr->synths.stereo[r_1to1][f_16] = synth_1to1_stereo_altivec;
#		endif
#		ifndef NO_REAL
		fr->synths.plain[r_1to1][f_real] = synth_1to1_real_altivec;
		fr->synths.stereo[r_1to1][f_real] = synth_1to1_real_stereo_altivec;
#		endif
#		ifndef NO_32BIT
		fr->synths.plain[r_1to1][f_32] = synth_1to1_s32_altivec;
		fr->synths.stereo[r_1to1][f_32] = synth_1to1_s32_stereo_altivec;
#		endif
		done = 1;
	}
#	endif

#	ifdef OPT_NEON
	if(!done && (auto_choose || want_dec == neon) && cpu_neon(cpu_flags))
	{
		chosen = dn_neon;
		fr->cpu_opts.type = neon;
#ifdef OPT_MULTI
#		ifndef NO_LAYER3
		fr->cpu_opts.the_dct36 = dct36_neon;
#		endif
#endif
#		ifndef NO_16BIT
		fr->synths.plain[r_1to1][f_16] = synth_1to1_neon;
		fr->synths.stereo[r_1to1][f_16] = synth_1to1_stereo_neon;
#		endif
#		ifndef NO_REAL
		fr->synths.plain[r_1to1][f_real] = synth_1to1_real_neon;
		fr->synths.stereo[r_1to1][f_real] = synth_1to1_real_stereo_neon;
#		endif
#		ifndef NO_32BIT
		fr->synths.plain[r_1to1][f_32] = synth_1to1_s32_neon;
		fr->synths.stereo[r_1to1][f_32] = synth_1to1_s32_stereo_neon;
#		endif
		done = 1;
	}
#	endif

#	ifdef OPT_ARM
	if(!done && (auto_choose || want_dec == arm))
	{
		chosen = dn_arm;
		fr->cpu_opts.type = arm;
#		ifndef NO_16BIT
		fr->synths.plain[r_1to1][f_16] = synth_1to1_arm;
#		endif
		done = 1;
	}
#	endif

#	ifdef OPT_NEON64
	if(!done && (auto_choose || want_dec == neon64) && cpu_neon(cpu_flags))
	{
		chosen = dn_neon64;
		fr->cpu_opts.type = neon64;
#ifdef OPT_MULTI
#		ifndef NO_LAYER3
		fr->cpu_opts.the_dct36 = dct36_neon64;
#		endif
#endif
#		ifndef NO_16BIT
		fr->synths.plain[r_1to1][f_16] = synth_1to1_neon64;
		fr->synths.stereo[r_1to1][f_16] = synth_1to1_stereo_neon64;
#		endif
#		ifndef NO_REAL
		fr->synths.plain[r_1to1][f_real] = synth_1to1_real_neon64;
		fr->synths.stereo[r_1to1][f_real] = synth_1to1_real_stereo_neon64;
#		endif
#		ifndef NO_32BIT
		fr->synths.plain[r_1to1][f_32] = synth_1to1_s32_neon64;
		fr->synths.stereo[r_1to1][f_32] = synth_1to1_s32_stereo_neon64;
#		endif
		done = 1;
	}
#	endif

#	ifdef OPT_GENERIC
	if(!done && (auto_choose || want_dec == generic))
	{
		chosen = dn_generic;
		fr->cpu_opts.type = generic;
		done = 1;
	}
#	endif

#ifdef OPT_GENERIC_DITHER
	if(!done && (auto_choose || want_dec == generic_dither))
	{
		chosen = "dithered generic";
		fr->cpu_opts.type = generic_dither;
		dithered = TRUE;
#		ifndef NO_16BIT
		fr->synths.plain[r_1to1][f_16] = synth_1to1_dither;
#		ifndef NO_DOWNSAMPLE
		fr->synths.plain[r_2to1][f_16] = synth_2to1_dither;
		fr->synths.plain[r_4to1][f_16] = synth_4to1_dither;
#		endif
#		endif
		done = 1;
	}
#endif

	fr->cpu_opts.class = decclass(fr->cpu_opts.type);

#	ifndef NO_8BIT
#	ifndef NO_16BIT /* possibility to use a 16->8 wrapper... */
	/* Last chance to use some optimized routine via generic wrappers (for 8bit). */
	if(     fr->cpu_opts.type != ifuenf_dither
	     && fr->cpu_opts.type != generic_dither
	     && fr->synths.plain[r_1to1][f_16] != synth_base.plain[r_1to1][f_16] )
	{
		fr->synths.plain[r_1to1][f_8] = synth_1to1_8bit_wrap;
		fr->synths.mono[r_1to1][f_8] = synth_1to1_8bit_wrap_mono;
		fr->synths.mono2stereo[r_1to1][f_8] = synth_1to1_8bit_wrap_m2s;
	}
#	endif
#	endif

#ifdef OPT_DITHER
	if(done && dithered)
	{
		/* run-time dither noise table generation */
		if(!frame_dither_init(fr))
		{
			if(NOQUIET) error("Dither noise setup failed!");
			return 0;
		}
	}
#endif

	if(done)
	{
		if(VERBOSE) fprintf(stderr, "Decoder: %s\n", chosen);
		return 1;
	}
	else
	{
		if(NOQUIET) error("Could not set optimization!");
		return 0;
	}
}

enum optdec dectype(const char* decoder)
{
	enum optdec dt;
	if(   (decoder == NULL)
	   || (decoder[0] == 0) )
	return autodec;

	for(dt=autodec; dt<nodec; ++dt)
	if(!strcasecmp(decoder, decname[dt])) return dt;

	return nodec; /* If we found nothing... */
}

#ifdef OPT_MULTI

/* same number of entries as full list, but empty at beginning */
static const char *mpg123_supported_decoder_list[] =
{
	#ifdef OPT_SSE
	NULL,
	#endif
	#ifdef OPT_SSE_VINTAGE
	NULL,
	#endif
	#ifdef OPT_3DNOWEXT
	NULL,
	#endif
	#ifdef OPT_3DNOWEXT_VINTAGE
	NULL,
	#endif
	#ifdef OPT_3DNOW
	NULL,
	#endif
	#ifdef OPT_3DNOW_VINTAGE
	NULL,
	#endif
	#ifdef OPT_MMX
	NULL,
	#endif
	#ifdef OPT_I586
	NULL,
	#endif
	#ifdef OPT_I586_DITHER
	NULL,
	#endif
	#ifdef OPT_I486
	NULL,
	#endif
	#ifdef OPT_I386
	NULL,
	#endif
	#ifdef OPT_ALTIVEC
	NULL,
	#endif
	#ifdef OPT_AVX
	NULL,
	#endif
	#ifdef OPT_X86_64
	NULL,
	#endif
	#ifdef OPT_ARM
	NULL,
	#endif
	#ifdef OPT_NEON
	NULL,
	#endif
	#ifdef OPT_NEON64
	NULL,
	#endif
	#ifdef OPT_GENERIC_FLOAT
	NULL,
	#endif
#	ifdef OPT_GENERIC
	NULL,
#	endif
#	ifdef OPT_GENERIC_DITHER
	NULL,
#	endif
	NULL
};
#endif

static const char *mpg123_decoder_list[] =
{
	#ifdef OPT_SSE
	dn_sse,
	#endif
	#ifdef OPT_SSE_VINTAGE
	dn_sse_vintage,
	#endif
	#ifdef OPT_3DNOWEXT
	dn_dreidnowext,
	#endif
	#ifdef OPT_3DNOWEXT_VINTAGE
	dn_dreidnowext_vintage,
	#endif
	#ifdef OPT_3DNOW
	dn_dreidnow,
	#endif
	#ifdef OPT_3DNOW_VINTAGE
	dn_dreidnow_vintage,
	#endif
	#ifdef OPT_MMX
	dn_mmx,
	#endif
	#ifdef OPT_I586
	dn_ifuenf,
	#endif
	#ifdef OPT_I586_DITHER
	dn_ifuenf_dither,
	#endif
	#ifdef OPT_I486
	dn_ivier,
	#endif
	#ifdef OPT_I386
	dn_idrei,
	#endif
	#ifdef OPT_ALTIVEC
	dn_altivec,
	#endif
	#ifdef OPT_AVX
	dn_avx,
	#endif
	#ifdef OPT_X86_64
	dn_x86_64,
	#endif
	#ifdef OPT_ARM
	dn_arm,
	#endif
	#ifdef OPT_NEON
	dn_neon,
	#endif
	#ifdef OPT_NEON64
	dn_neon64,
	#endif
	#ifdef OPT_GENERIC
	dn_generic,
	#endif
	#ifdef OPT_GENERIC_DITHER
	dn_generic_dither,
	#endif
	NULL
};

void check_decoders(void )
{
#ifndef OPT_MULTI
	/* In non-multi mode, only the full list (one entry) is used. */
	return;
#else
	const char **d = mpg123_supported_decoder_list;
#if (defined OPT_X86) || (defined OPT_X86_64) || (defined OPT_NEON) || (defined OPT_NEON64)
	getcpuflags(&cpu_flags);
#endif
#ifdef OPT_X86
	if(cpu_i586(cpu_flags))
	{
		/* not yet: if(cpu_sse2(cpu_flags)) printf(" SSE2");
		if(cpu_sse3(cpu_flags)) printf(" SSE3"); */
#ifdef OPT_SSE
		if(cpu_sse(cpu_flags)) *(d++) = dn_sse;
#endif
#ifdef OPT_SSE_VINTAGE
		if(cpu_sse(cpu_flags)) *(d++) = dn_sse_vintage;
#endif
#ifdef OPT_3DNOWEXT
		if(cpu_3dnowext(cpu_flags)) *(d++) = dn_dreidnowext;
#endif
#ifdef OPT_3DNOWEXT_VINTAGE
		if(cpu_3dnowext(cpu_flags)) *(d++) = dn_dreidnowext_vintage;
#endif
#ifdef OPT_3DNOW
		if(cpu_3dnow(cpu_flags)) *(d++) = dn_dreidnow;
#endif
#ifdef OPT_3DNOW_VINTAGE
		if(cpu_3dnow(cpu_flags)) *(d++) = dn_dreidnow_vintage;
#endif
#ifdef OPT_MMX
		if(cpu_mmx(cpu_flags)) *(d++) = dn_mmx;
#endif
#ifdef OPT_I586
		*(d++) = dn_ifuenf;
#endif
#ifdef OPT_I586_DITHER
		*(d++) = dn_ifuenf_dither;
#endif
	}
#endif
/* just assume that the i486 built is run on a i486 cpu... */
#ifdef OPT_I486
	*(d++) = dn_ivier;
#endif
#ifdef OPT_ALTIVEC
	*(d++) = dn_altivec;
#endif
/* every supported x86 can do i386, any cpu can do generic */
#ifdef OPT_I386
	*(d++) = dn_idrei;
#endif
#ifdef OPT_AVX
	if(cpu_avx(cpu_flags)) *(d++) = dn_avx;
#endif
#ifdef OPT_X86_64
	*(d++) = dn_x86_64;
#endif
#ifdef OPT_ARM
	*(d++) = dn_arm;
#endif
#ifdef OPT_NEON
	if(cpu_neon(cpu_flags)) *(d++) = dn_neon;
#endif
#ifdef OPT_NEON64
	if(cpu_neon(cpu_flags)) *(d++) = dn_neon64;
#endif
#ifdef OPT_GENERIC
	*(d++) = dn_generic;
#endif
#ifdef OPT_GENERIC_DITHER
	*(d++) = dn_generic_dither;
#endif
#endif /* ndef OPT_MULTI */
}

const char* attribute_align_arg mpg123_current_decoder(mpg123_handle *mh)
{
	if(mh == NULL) return NULL;

	return decname[mh->cpu_opts.type];
}

const char attribute_align_arg **mpg123_decoders(void){ return mpg123_decoder_list; }
const char attribute_align_arg **mpg123_supported_decoders(void)
{
#ifdef OPT_MULTI
	return mpg123_supported_decoder_list;
#else
	return mpg123_decoder_list;
#endif
}
