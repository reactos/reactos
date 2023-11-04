#ifndef MPG123_H_OPTIMIZE
#define MPG123_H_OPTIMIZE
/*
	optimize: get a grip on the different optimizations

	copyright 2007-2013 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, taking from mpg123.[hc]

	for building mpg123 with one optimization only, you have to choose exclusively between
	OPT_GENERIC (generic C code for everyone)
	OPT_GENERIC_DITHER (same with dithering for 1to1)
	OPT_I386 (Intel i386)
	OPT_I486 (Somewhat special code for i486; does not work together with others.)
	OPT_I586 (Intel Pentium)
	OPT_I586_DITHER (Intel Pentium with dithering/noise shaping for enhanced quality)
	OPT_MMX (Intel Pentium and compatibles with MMX, fast, but not the best accuracy)
	OPT_3DNOW (AMD 3DNow!, K6-2/3, Athlon, compatibles...)
	OPT_3DNOW_VINTAGE
	OPT_3DNOWEXT (AMD 3DNow! extended, generally Athlon, compatibles...)
	OPT_3DNOWEXT_VINTAGE
	OPT_SSE
	OPT_SSE_VINTAGE
	OPT_ALTIVEC (Motorola/IBM PPC with AltiVec under MacOSX)
	OPT_X86_64 (x86-64 / AMD64 / Intel 64)
	OPT_AVX

	or you define OPT_MULTI and give a combination which makes sense (do not include i486, do not mix altivec and x86).

	I still have to examine the dynamics of this here together with REAL_IS_FIXED.
	Basic point is: Don't use REAL_IS_FIXED with something else than generic or i386.

	Also, one should minimize code size by really ensuring that only functions that are really needed are included.
	Currently, all generic functions will be always there (to be safe for fallbacks for advanced decoders).
	Strictly, at least the synth_1to1 should not be necessary for single-decoder mode.
*/


/* Runtime optimization interface now here: */

/* Nedit inline Perl script to generate decoder list and name mapping in one place
   optimize.c defining I_AM_OPTIMIZE to get the names

perl <<'EOT'
## order is important (autodec first, nodec last)
@names=
(
 ['autodec', 'auto']
,['generic', 'generic']
,['generic_dither', 'generic_dither']
,['idrei', 'i386']
,['ivier', 'i486']
,['ifuenf', 'i586']
,['ifuenf_dither', 'i586_dither']
,['mmx', 'MMX']
,['dreidnow', '3DNow']
,['dreidnowext', '3DNowExt']
,['altivec', 'AltiVec']
,['sse', 'SSE']
,['x86_64', 'x86-64']
,['arm','ARM']
,['neon','NEON']
,['avx','AVX']
,['dreidnow_vintage', '3DNow_vintage']
,['dreidnowext_vintage', '3DNowExt_vintage']
,['sse_vintage', 'SSE_vintage']
,['nodec', 'nodec']
);

print "enum optdec\n{\n";
for my $n (@names)
{
	$name = $n->[0];
	$enum = $name eq 'autodec' ? $name = " $name=0" : ",$name";
	print "\t$enum\n"
}
print "};\n";
print "##ifdef I_AM_OPTIMIZE\n";
for my $n (@names)
{
	my $key = $n->[0];
	my $val = $n->[1];
	print "static const char dn_$key\[\] = \"$val\";\n";
}
print "static const char* decname[] =\n{\n";
for my $n (@names)
{
	my $key = $n->[0];
	print "\t".($key eq 'autodec' ? ' ' : ',')."dn_$key\n";
}
print "};\n##endif"
EOT
*/
enum optdec
{
	 autodec=0
	,generic
	,generic_dither
	,idrei
	,ivier
	,ifuenf
	,ifuenf_dither
	,mmx
	,dreidnow
	,dreidnowext
	,altivec
	,sse
	,x86_64
	,arm
	,neon
	,neon64
	,avx
	,dreidnow_vintage
	,dreidnowext_vintage
	,sse_vintage
	,nodec
};
#ifdef I_AM_OPTIMIZE
static const char dn_autodec[] = "auto";
static const char dn_generic[] = "generic";
static const char dn_generic_dither[] = "generic_dither";
static const char dn_idrei[] = "i386";
static const char dn_ivier[] = "i486";
static const char dn_ifuenf[] = "i586";
static const char dn_ifuenf_dither[] = "i586_dither";
static const char dn_mmx[] = "MMX";
static const char dn_dreidnow[] = "3DNow";
static const char dn_dreidnowext[] = "3DNowExt";
static const char dn_altivec[] = "AltiVec";
static const char dn_sse[] = "SSE";
static const char dn_x86_64[] = "x86-64";
static const char dn_arm[] = "ARM";
static const char dn_neon[] = "NEON";
static const char dn_neon64[] = "NEON64";
static const char dn_avx[] = "AVX";
static const char dn_dreidnow_vintage[] = "3DNow_vintage";
static const char dn_dreidnowext_vintage[] = "3DNowExt_vintage";
static const char dn_sse_vintage[] = "SSE_vintage";
static const char dn_nodec[] = "nodec";
static const char* decname[] =
{
	 dn_autodec
	,dn_generic
	,dn_generic_dither
	,dn_idrei
	,dn_ivier
	,dn_ifuenf
	,dn_ifuenf_dither
	,dn_mmx
	,dn_dreidnow
	,dn_dreidnowext
	,dn_altivec
	,dn_sse
	,dn_x86_64
	,dn_arm
	,dn_neon
	,dn_neon64
	,dn_avx
	,dn_dreidnow_vintage
	,dn_dreidnowext_vintage
	,dn_sse_vintage
	,dn_nodec
};
#endif

enum optcla { nocla=0, normal, mmxsse };

/*  - Set up the table of synth functions for current decoder choice. */
int frame_cpu_opt(mpg123_handle *fr, const char* cpu);
/*  - Choose, from the synth table, the synth functions to use for current output format/rate. */
int set_synth_functions(mpg123_handle *fr);
/*  - Parse decoder name and return numerical code. */
enum optdec dectype(const char* decoder);
/*  - Return the default decoder type. */
enum optdec defdec(void);
/*  - Return the class of a decoder type (mmxsse or normal). */
enum optcla decclass(const enum optdec);

/* Now comes a whole lot of definitions, for multi decoder mode and single decoder mode.
   Because of the latter, it may look redundant at times. */

/* this is included in mpg123.h, which includes config.h */
#ifdef CCALIGN
#ifdef _MSC_VER
#define ALIGNED(a) __declspec(align(a))
#else
#define ALIGNED(a) __attribute__((aligned(a)))
#endif
#else
#define ALIGNED(a)
#endif

/* Safety catch for invalid decoder choice. */
#ifdef REAL_IS_FIXED
#if (defined OPT_I486)  || (defined OPT_I586) || (defined OPT_I586_DITHER) \
 || (defined OPT_MMX)   || (defined OPT_SSE)  || (defined_OPT_ALTIVEC) \
 || (defined OPT_3DNOW) || (defined OPT_3DNOWEXT) || (defined OPT_X86_64) \
 || (defined OPT_3DNOW_VINTAGE) || (defined OPT_3DNOWEXT_VINTAGE) \
 || (defined OPT_SSE_VINTAGE) \
 || (defined OPT_NEON) || (defined OPT_NEON64) || (defined OPT_AVX) \
 || (defined OPT_GENERIC_DITHER)
#error "Bad decoder choice together with fixed point math!"
#endif
#endif

#if (defined NO_LAYER1 && defined NO_LAYER2)
#define NO_LAYER12
#endif

#ifdef OPT_GENERIC
#ifndef OPT_MULTI
#	define defopt generic
#endif
#endif

#ifdef OPT_GENERIC_DITHER
#define OPT_DITHER
#ifndef OPT_MULTI
#	define defopt generic_dither
#endif
#endif

/* i486 is special... always alone! */
#ifdef OPT_I486
#define OPT_X86
#define defopt ivier
#ifdef OPT_MULTI
#error "i486 can only work alone!"
#endif
#define FIR_BUFFER_SIZE  128
#define FIR_SIZE 16
#endif

#ifdef OPT_I386
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt idrei
#endif
#endif

#ifdef OPT_I586
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt ifuenf
#endif
#endif

#ifdef OPT_I586_DITHER
#define OPT_X86
#define OPT_DITHER
#ifndef OPT_MULTI
#	define defopt ifuenf_dither
#endif
#endif

/* We still have some special code around MMX tables. */

#ifdef OPT_MMX
#define OPT_MMXORSSE
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt mmx
#endif
#endif

#ifdef OPT_SSE
#define OPT_MMXORSSE
#define OPT_MPLAYER
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt sse
#	define opt_dct36(fr) dct36_sse
#endif
#endif

#ifdef OPT_SSE_VINTAGE
#define OPT_MMXORSSE
#define OPT_MPLAYER
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt sse
#endif
#endif

#ifdef OPT_3DNOWEXT
#define OPT_MMXORSSE
#define OPT_MPLAYER
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt dreidnowext
#endif
#endif

/* same as above but also using 3DNowExt dct36 */
#ifdef OPT_3DNOWEXT_VINTAGE
#define OPT_MMXORSSE
#define OPT_MPLAYER
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt dreidnowext_vintage
#	define opt_dct36(fr) dct36_3dnowext
#endif
#endif

#ifdef OPT_MPLAYER
extern const int costab_mmxsse[];
#endif

/* 3dnow used to use synth_1to1_i586 for mono / 8bit conversion - was that intentional? */
/* I'm trying to skip the pentium code here ... until I see that that is indeed a bad idea */
#ifdef OPT_3DNOW
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt dreidnow
#endif
#endif

/* same as above but also using 3DNow dct36 */
#ifdef OPT_3DNOW_VINTAGE
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt dreidnow_vintage
#	define opt_dct36(fr) dct36_3dnow
#endif
#endif

#ifdef OPT_ALTIVEC
#ifndef OPT_MULTI
#	define defopt altivec
#endif
#endif

#ifdef OPT_X86_64
#define OPT_MMXORSSE
#ifndef OPT_MULTI
#	define defopt x86_64
#	define opt_dct36(fr) dct36_x86_64
#endif
#endif

#ifdef OPT_AVX
#define OPT_MMXORSSE
#ifndef OPT_MULTI
#	define defopt avx
#	define opt_dct36(fr) dct36_avx
#endif
#endif

#ifdef OPT_ARM
#ifndef OPT_MULTI
#	define defopt arm
#endif
#endif

#ifdef OPT_NEON
#define OPT_MMXORSSE
#ifndef OPT_MULTI
#	define defopt neon
#	define opt_dct36(fr) dct36_neon
#endif
#endif

#ifdef OPT_NEON64
#define OPT_MMXORSSE
#ifndef OPT_MULTI
#	define defopt neon64
#	define opt_dct36(fr) dct36_neon64
#endif
#endif

/* used for multi opt mode and the single 3dnow mode to have the old 3dnow test flag still working */
void check_decoders(void);

/*
	Now come two blocks of standard definitions for multi-decoder mode and single-decoder mode.
	Most stuff is so automatic that it's indeed generated by some inline shell script.
	Remember to use these scripts when possible, instead of direct repetitive hacking.
*/

#ifdef OPT_MULTI

#	define defopt nodec

#	if (defined OPT_3DNOW_VINTAGE || defined OPT_3DNOWEXT_VINTAGE || defined OPT_SSE || defined OPT_X86_64 || defined OPT_AVX || defined OPT_NEON || defined OPT_NEON64)
#		define opt_dct36(fr) ((fr)->cpu_opts.the_dct36)
#	endif

#endif /* OPT_MULTI else */

#	ifndef opt_dct36
#		define opt_dct36(fr) dct36
#	endif

#endif /* MPG123_H_OPTIMIZE */

