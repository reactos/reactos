/*
	format: routines to deal with audio (output) format

	copyright 2008-20 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, starting with parts of the old audio.c, with only faintly manage to show now

	A Major change from mpg123 <= 1.18 is that all encodings are only really
	disabled when done so via specific build configuration. Otherwise, the
	missing support of decoders to produce a certain format is augmented by
	postprocessing that converts the samples. This means happily creating
	data with higher resolution from less accurate decoder output.

	The main point is to still offer float encoding when the decoding core uses
	a fixed point representation that has only 16 bit output. Actually, that's
	the only point: A fixed-point build needs to create float from 16 bit, also
	32 or 24 bit from the same source. That's all there is to it: Everything else
	is covered by fallback synth functions. It may be a further step to check if
	there are cases where conversion in postprocessing works well enough to omit
	a certain specialized decoder ... but usually, they are justified by some
	special way to get from float to integer to begin with.

	I won't cover the case of faking double output with float/s16 decoders here.
	Double precision output is a thing for experimental builds anyway. Mostly
	theoretical and without a point.
*/

#include "mpg123lib_intern.h"
#include "sample.h"
#include "debug.h"

/* static int chans[NUM_CHANNELS] = { 1 , 2 }; */
static const long my_rates[MPG123_RATES] = /* only the standard rates */
{
	 8000, 11025, 12000,
	16000, 22050, 24000,
	32000, 44100, 48000,
};

static const int my_encodings[MPG123_ENCODINGS] =
{
	MPG123_ENC_SIGNED_16,
	MPG123_ENC_UNSIGNED_16,
	MPG123_ENC_SIGNED_32,
	MPG123_ENC_UNSIGNED_32,
	MPG123_ENC_SIGNED_24,
	MPG123_ENC_UNSIGNED_24,
	/* Floating point range, see below. */
	MPG123_ENC_FLOAT_32,
	MPG123_ENC_FLOAT_64,
	/* 8 bit range, see below. */
	MPG123_ENC_SIGNED_8,
	MPG123_ENC_UNSIGNED_8,
	MPG123_ENC_ULAW_8,
	MPG123_ENC_ALAW_8
};

/* Make that match the above table.
   And yes, I still don't like this kludgy stuff. */
/* range[0] <= i < range[1] for forced floating point */
static const int enc_float_range[2] = { 6, 8 };
/* same for 8 bit encodings */
static const int enc_8bit_range[2] = { 8, 12 };
// for 24 bit quality (24 and 32 bit integers)
static const int enc_24bit_range[2] = { 2, 6 };
// for completeness, the 16 bits
static const int enc_16bit_range[2] = { 0, 2};

/*
	Only one type of float is supported.
	Actually, double is a very special experimental case not occuring in normal
	builds. Might actually get rid of it.

	Remember here: Also with REAL_IS_FIXED, I want to be able to produce float
	output (f32) via post-processing.
*/
# ifdef REAL_IS_DOUBLE
#  define MPG123_FLOAT_ENC MPG123_ENC_FLOAT_64
# else
#  define MPG123_FLOAT_ENC MPG123_ENC_FLOAT_32
# endif

/* The list of actually possible encodings. */
static const int good_encodings[] =
{
#ifndef NO_16BIT
	MPG123_ENC_SIGNED_16,
	MPG123_ENC_UNSIGNED_16,
#endif
#ifndef NO_32BIT
	MPG123_ENC_SIGNED_32,
	MPG123_ENC_UNSIGNED_32,
	MPG123_ENC_SIGNED_24,
	MPG123_ENC_UNSIGNED_24,
#endif
#ifndef NO_REAL
	MPG123_FLOAT_ENC,
#endif
#ifndef NO_8BIT
	MPG123_ENC_SIGNED_8,
	MPG123_ENC_UNSIGNED_8,
	MPG123_ENC_ULAW_8,
	MPG123_ENC_ALAW_8
#endif
};

/* Check if encoding is a valid one in this build.
   ...lazy programming: linear search. */
static int good_enc(const int enc)
{
	size_t i;
	for(i=0; i<sizeof(good_encodings)/sizeof(int); ++i)
	if(enc == good_encodings[i]) return TRUE;

	return FALSE;
}

void attribute_align_arg mpg123_rates(const long **list, size_t *number)
{
	if(list   != NULL) *list   = my_rates;
	if(number != NULL) *number = sizeof(my_rates)/sizeof(long);
}

/* Now that's a bit tricky... One build of the library knows only a subset of the encodings. */
void attribute_align_arg mpg123_encodings(const int **list, size_t *number)
{
	if(list   != NULL) *list   = good_encodings;
	if(number != NULL) *number = sizeof(good_encodings)/sizeof(int);
}

int attribute_align_arg mpg123_encsize(int encoding)
{
	return MPG123_SAMPLESIZE(encoding);
}

/*	char audio_caps[NUM_CHANNELS][MPG123_RATES+1][MPG123_ENCODINGS]; */

static int rate2num(mpg123_pars *mp, long r)
{
	int i;
	for(i=0;i<MPG123_RATES;i++) if(my_rates[i] == r) return i;
#ifndef NO_NTOM
	if(mp && mp->force_rate != 0 && mp->force_rate == r) return MPG123_RATES;
#endif

	return -1;
}

static int enc2num(int encoding)
{
	int i;
	for(i=0;i<MPG123_ENCODINGS;++i)
	if(my_encodings[i] == encoding) return i;

	return -1;
}

static int cap_fit(mpg123_pars *p, struct audioformat *nf, int f0, int f2)
{
	int i;
	int c  = nf->channels-1;
	int rn = rate2num(p, nf->rate);
	if(rn >= 0)	for(i=f0;i<f2;i++)
	{
		if(p->audio_caps[c][rn][i])
		{
			nf->encoding = my_encodings[i];
			return 1;
		}
	}
	return 0;
}

static int imin(int a, int b)
{
	return a < b ? a : b;
}

static int imax(int a, int b)
{
	return a > b ? a : b;
}

// Find a possible encoding with given rate and channel count,
// try differing channel count, too.
// This updates the given format and returns TRUE if an encoding
// was found.
static int enc_chan_fit( mpg123_pars *p, long rate, struct audioformat *nnf
,	int f0, int f2, int try_float )
{
#define ENCRANGE(range) imax(f0, range[0]), imin(f2, range[1])
	struct audioformat nf = *nnf;
	nf.rate = rate;
	if(cap_fit(p, &nf, ENCRANGE(enc_16bit_range)))
		goto eend;
	if(cap_fit(p, &nf, ENCRANGE(enc_24bit_range)))
		goto eend;
	if(try_float &&
		cap_fit(p, &nf, ENCRANGE(enc_float_range)))
		goto eend;
	if(cap_fit(p, &nf, ENCRANGE(enc_8bit_range)))
		goto eend;

	/* try again with different stereoness */
	if(nf.channels == 2 && !(p->flags & MPG123_FORCE_STEREO)) nf.channels = 1;
	else if(nf.channels == 1 && !(p->flags & MPG123_FORCE_MONO)) nf.channels = 2;

	if(cap_fit(p, &nf, ENCRANGE(enc_16bit_range)))
		goto eend;
	if(cap_fit(p, &nf, ENCRANGE(enc_24bit_range)))
		goto eend;
	if(try_float &&
		cap_fit(p, &nf, ENCRANGE(enc_float_range)))
		goto eend;
	if(cap_fit(p, &nf, ENCRANGE(enc_8bit_range)))
		goto eend;
	return FALSE;
eend:
	*nnf = nf;
	return TRUE;
#undef ENCRANGE
}

/* match constraints against supported audio formats, store possible setup in frame
  return: -1: error; 0: no format change; 1: format change */
int frame_output_format(mpg123_handle *fr)
{
	struct audioformat nf;
	int f0=0;
	int f2=MPG123_ENCODINGS+1; // Include all encodings by default.
	mpg123_pars *p = &fr->p;
	int try_float = (p->flags & MPG123_FLOAT_FALLBACK) ? 0 : 1;
	/* initialize new format, encoding comes later */
	nf.channels = fr->stereo;

	// I intended the forcing stuff to be weaved into the format support table,
	// but this probably will never happen, as this would change library behaviour.
	// One could introduce an additional effective format table that takes for
	// forcings into account, but that would have to be updated on any flag
	// change. Tedious.
	if(p->flags & MPG123_FORCE_8BIT)
	{
		f0 = enc_8bit_range[0];
		f2 = enc_8bit_range[1];
	}
	if(p->flags & MPG123_FORCE_FLOAT)
	{
		try_float = 1;
		f0 = enc_float_range[0];
		f2 = enc_float_range[1];
	}

	/* force stereo is stronger */
	if(p->flags & MPG123_FORCE_MONO)   nf.channels = 1;
	if(p->flags & MPG123_FORCE_STEREO) nf.channels = 2;

	// Strategy update: Avoid too early triggering of the NtoM decoder.
	// Main target is the native rate, with any encoding.
	// Then, native rate with any channel count and any encoding.
	// Then, it's down_sample from native rate.
	// As last resort: NtoM rate.
	// So the priority is 1. rate 2. channels 3. encoding.
	// As encodings go, 16 bit is tranditionally preferred as efficient choice.
	// Next in line are wider float and integer encodings, then 8 bit as
	// last resort.

#ifndef NO_NTOM
	if(p->force_rate)
	{
		if(enc_chan_fit(p, p->force_rate, &nf, f0, f2, try_float))
			goto end;
		// Keep the order consistent if float is considered fallback only.
		if(!try_float &&
			enc_chan_fit(p, p->force_rate, &nf, f0, f2, TRUE))
				goto end;

		merror( "Unable to set up output format! Constraints: %s%s%liHz."
		,	( p->flags & MPG123_FORCE_STEREO ? "stereo, " :
				(p->flags & MPG123_FORCE_MONO ? "mono, " : "") )
		,	( p->flags & MPG123_FORCE_FLOAT ? "float, " :
				(p->flags & MPG123_FORCE_8BIT ? "8bit, " : "") )
		,	p->force_rate );
/*		if(NOQUIET && p->verbose <= 1) print_capabilities(fr); */

		fr->err = MPG123_BAD_OUTFORMAT;
		return -1;
	}
#endif
	// Native decoder rate first.
	if(enc_chan_fit(p, frame_freq(fr)>>p->down_sample, &nf, f0, f2, try_float))
		goto end;
	// Then downsamplings.
	if(p->flags & MPG123_AUTO_RESAMPLE && p->down_sample < 2)
	{
		if(enc_chan_fit( p, frame_freq(fr)>>(p->down_sample+1), &nf
		,	f0, f2, try_float ))
			goto end;
		if(p->down_sample < 1 && enc_chan_fit( p, frame_freq(fr)>>2, &nf
		,	f0, f2, try_float ))
			goto end;
	}
	// And again the whole deal with float fallback.
	if(!try_float)
	{
		if(enc_chan_fit(p, frame_freq(fr)>>p->down_sample, &nf, f0, f2, TRUE))
			goto end;
		// Then downsamplings.
		if(p->flags & MPG123_AUTO_RESAMPLE && p->down_sample < 2)
		{
			if(enc_chan_fit( p, frame_freq(fr)>>(p->down_sample+1), &nf
			,	f0, f2, TRUE ))
				goto end;
			if(p->down_sample < 1 && enc_chan_fit( p, frame_freq(fr)>>2, &nf
			,	f0, f2, TRUE ))
				goto end;
		}
	}
#ifndef NO_NTOM
	// Try to find any rate that works and resample using NtoM hackery.
	if(  p->flags & MPG123_AUTO_RESAMPLE && fr->p.down_sample == 0)
	{
		int i;
		int rn = rate2num(p, frame_freq(fr));
		int rrn;
		if(rn < 0) return 0;
		/* Try higher rates first. */
		for(rrn=rn+1; rrn<MPG123_RATES; ++rrn)
			if(enc_chan_fit(p, my_rates[rrn], &nf, f0, f2, try_float))
				goto end;
		/* Then lower rates. */
		for(i=f0;i<f2;i++) for(rrn=rn-1; rrn>=0; --rrn)
			if(enc_chan_fit(p, my_rates[rrn], &nf, f0, f2, try_float))
				goto end;
		// And again for float fallback.
		if(!try_float)
		{
			/* Try higher rates first. */
			for(rrn=rn+1; rrn<MPG123_RATES; ++rrn)
				if(enc_chan_fit(p, my_rates[rrn], &nf, f0, f2, TRUE))
					goto end;
			/* Then lower rates. */
			for(i=f0;i<f2;i++) for(rrn=rn-1; rrn>=0; --rrn)
				if(enc_chan_fit(p, my_rates[rrn], &nf, f0, f2, TRUE))
					goto end;
		}
	}
#endif

	/* Here is the _bad_ end. */
	merror( "Unable to set up output format! Constraints: %s%s%li, %li or %liHz."
	,	( p->flags & MPG123_FORCE_STEREO ? "stereo, " :
			(p->flags & MPG123_FORCE_MONO ? "mono, " : "") )
	,	( p->flags & MPG123_FORCE_FLOAT ? "float, " :
			(p->flags & MPG123_FORCE_8BIT ? "8bit, " : "") )
	,	frame_freq(fr),  frame_freq(fr)>>1, frame_freq(fr)>>2 );
/*	if(NOQUIET && p->verbose <= 1) print_capabilities(fr); */

	fr->err = MPG123_BAD_OUTFORMAT;
	return -1;

end: /* Here is the _good_ end. */
	/* we had a successful match, now see if there's a change */
	if(nf.rate == fr->af.rate && nf.channels == fr->af.channels && nf.encoding == fr->af.encoding)
	{
		debug2("Old format with %i channels, and FORCE_MONO=%li", nf.channels, p->flags & MPG123_FORCE_MONO);
		return 0; /* the same format as before */
	}
	else /* a new format */
	{
		debug1("New format with %i channels!", nf.channels);
		fr->af.rate = nf.rate;
		fr->af.channels = nf.channels;
		fr->af.encoding = nf.encoding;
		/* Cache the size of one sample in bytes, for ease of use. */
		fr->af.encsize = mpg123_encsize(fr->af.encoding);
		if(fr->af.encsize < 1)
		{
			error1("Some unknown encoding??? (%i)", fr->af.encoding);

			fr->err = MPG123_BAD_OUTFORMAT;
			return -1;
		}
		/* Set up the decoder synth format. Might differ. */
#ifdef NO_SYNTH32
		/* Without high-precision synths, 16 bit signed is the basis for
		   everything higher than 8 bit. */
		if(fr->af.encsize > 2)
		fr->af.dec_enc = MPG123_ENC_SIGNED_16;
		else
		{
#endif
			switch(fr->af.encoding)
			{
#ifndef NO_32BIT
			case MPG123_ENC_SIGNED_24:
			case MPG123_ENC_UNSIGNED_24:
			case MPG123_ENC_UNSIGNED_32:
				fr->af.dec_enc = MPG123_ENC_SIGNED_32;
			break;
#endif
#ifndef NO_16BIT
			case MPG123_ENC_UNSIGNED_16:
				fr->af.dec_enc = MPG123_ENC_SIGNED_16;
			break;
#endif
			default:
				fr->af.dec_enc = fr->af.encoding;
			}
#ifdef NO_SYNTH32
		}
#endif
		fr->af.dec_encsize = mpg123_encsize(fr->af.dec_enc);
		return 1;
	}
}

int attribute_align_arg mpg123_format_none(mpg123_handle *mh)
{
	int r;
	if(mh == NULL) return MPG123_BAD_HANDLE;

	r = mpg123_fmt_none(&mh->p);
	if(r != MPG123_OK){ mh->err = r; r = MPG123_ERR; }

	return r;
}

int attribute_align_arg mpg123_fmt_none(mpg123_pars *mp)
{
	if(mp == NULL) return MPG123_BAD_PARS;

	if(PVERB(mp,3)) fprintf(stderr, "Note: Disabling all formats.\n");

	memset(mp->audio_caps,0,sizeof(mp->audio_caps));
	return MPG123_OK;
}

int attribute_align_arg mpg123_format_all(mpg123_handle *mh)
{
	int r;
	if(mh == NULL) return MPG123_BAD_HANDLE;

	r = mpg123_fmt_all(&mh->p);
	if(r != MPG123_OK){ mh->err = r; r = MPG123_ERR; }

	return r;
}

int attribute_align_arg mpg123_fmt_all(mpg123_pars *mp)
{
	size_t rate, ch, enc;
	if(mp == NULL) return MPG123_BAD_PARS;

	if(PVERB(mp,3)) fprintf(stderr, "Note: Enabling all formats.\n");

	for(ch=0;   ch   < NUM_CHANNELS;     ++ch)
	for(rate=0; rate < MPG123_RATES+1;   ++rate)
	for(enc=0;  enc  < MPG123_ENCODINGS; ++enc)
	mp->audio_caps[ch][rate][enc] = good_enc(my_encodings[enc]) ? 1 : 0;

	return MPG123_OK;
}

int attribute_align_arg mpg123_format2(mpg123_handle *mh, long rate, int channels, int encodings)
{
	int r;
	if(mh == NULL) return MPG123_BAD_HANDLE;
	r = mpg123_fmt2(&mh->p, rate, channels, encodings);
	if(r != MPG123_OK){ mh->err = r; r = MPG123_ERR; }

	return r;
}

// Keep old behaviour.
int attribute_align_arg mpg123_format(mpg123_handle *mh, long rate, int channels, int encodings)
{
	int r;
	if(mh == NULL) return MPG123_BAD_HANDLE;
	r = mpg123_fmt(&mh->p, rate, channels, encodings);
	if(r != MPG123_OK){ mh->err = r; r = MPG123_ERR; }

	return r;
}

int attribute_align_arg mpg123_fmt2(mpg123_pars *mp, long rate, int channels, int encodings)
{
	int ie, ic, ratei, r1, r2;
	int ch[2] = {0, 1};
	if(mp == NULL) return MPG123_BAD_PARS;
	if(!(channels & (MPG123_MONO|MPG123_STEREO))) return MPG123_BAD_CHANNEL;

	if(PVERB(mp,3)) fprintf(stderr, "Note: Want to enable format %li/%i for encodings 0x%x.\n", rate, channels, encodings);

	if(!(channels & MPG123_STEREO)) ch[1] = 0;     /* {0,0} */
	else if(!(channels & MPG123_MONO)) ch[0] = 1; /* {1,1} */
	if(rate)
	{
		r1 = rate2num(mp, rate);
		r2 = r1+1;
	}
	else
	{
		r1 = 0;
		r2 = MPG123_RATES+1; /* including forced rate */
	}

	if(r1 < 0) return MPG123_BAD_RATE;

	/* now match the encodings */
	for(ratei = r1; ratei < r2; ++ratei)
	for(ic = 0; ic < 2; ++ic)
	{
		for(ie = 0; ie < MPG123_ENCODINGS; ++ie)
		if(good_enc(my_encodings[ie]) && ((my_encodings[ie] & encodings) == my_encodings[ie]))
		mp->audio_caps[ch[ic]][ratei][ie] = 1;

		if(ch[0] == ch[1]) break; /* no need to do it again */
	}

	return MPG123_OK;
}

// Keep old behaviour, error on rate=0.
int attribute_align_arg mpg123_fmt(mpg123_pars *mp, long rate, int channels, int encodings)
{
	return (rate == 0)
	?	MPG123_BAD_RATE
	:	mpg123_fmt2(mp, rate, channels, encodings);
}

int attribute_align_arg mpg123_format_support(mpg123_handle *mh, long rate, int encoding)
{
	if(mh == NULL) return 0;
	else return mpg123_fmt_support(&mh->p, rate, encoding);
}

int attribute_align_arg mpg123_fmt_support(mpg123_pars *mp, long rate, int encoding)
{
	int ch = 0;
	int ratei, enci;
	ratei = rate2num(mp, rate);
	enci  = enc2num(encoding);
	if(mp == NULL || ratei < 0 || enci < 0) return 0;
	if(mp->audio_caps[0][ratei][enci]) ch |= MPG123_MONO;
	if(mp->audio_caps[1][ratei][enci]) ch |= MPG123_STEREO;
	return ch;
}

/* Call this one to ensure that any valid format will be something different than this. */
void invalidate_format(struct audioformat *af)
{
	af->encoding = 0;
	af->rate     = 0;
	af->channels = 0;
}

/* Number of bytes the decoder produces. */
off_t decoder_synth_bytes(mpg123_handle *fr, off_t s)
{
	return s * fr->af.dec_encsize * fr->af.channels;
}

/* Samples/bytes for output buffer after post-processing. */
/* take into account: channels, bytes per sample -- NOT resampling!*/
off_t samples_to_bytes(mpg123_handle *fr , off_t s)
{
	return s * fr->af.encsize * fr->af.channels;
}

off_t bytes_to_samples(mpg123_handle *fr , off_t b)
{
	return b / fr->af.encsize / fr->af.channels;
}

/* Number of bytes needed for decoding _and_ post-processing. */
off_t outblock_bytes(mpg123_handle *fr, off_t s)
{
	int encsize = (fr->af.encoding & MPG123_ENC_24)
	? 4 /* Intermediate 32 bit. */
	: (fr->af.encsize > fr->af.dec_encsize
		? fr->af.encsize
		: fr->af.dec_encsize);
	return s * encsize * fr->af.channels;
}

#ifndef NO_32BIT

/* Remove every fourth byte, facilitating conversion from 32 bit to 24 bit integers.
   This has to be aware of endianness, of course. */
static void chop_fourth_byte(struct outbuffer *buf)
{
	unsigned char *wpos = buf->data;
	unsigned char *rpos = buf->data;
	size_t blocks = buf->fill/4;
	size_t i;
	for(i=0; i<blocks; ++i,wpos+=3,rpos+=4)
		DROP4BYTE(wpos, rpos)
	buf->fill = wpos-buf->data;
}

static void conv_s32_to_u32(struct outbuffer *buf)
{
	size_t i;
	int32_t  *ssamples = (int32_t*)  buf->data;
	uint32_t *usamples = (uint32_t*) buf->data;
	size_t count = buf->fill/sizeof(int32_t);

	for(i=0; i<count; ++i)
		usamples[i] = CONV_SU32(ssamples[i]);
}

#endif


/* We always assume that whole numbers are written!
   partials will be cut out. */

static const char *bufsizeerr = "Fatal: Buffer too small for postprocessing!";


#ifndef NO_16BIT

static void conv_s16_to_u16(struct outbuffer *buf)
{
	size_t i;
	int16_t  *ssamples = (int16_t*) buf->data;
	uint16_t *usamples = (uint16_t*)buf->data;
	size_t count = buf->fill/sizeof(int16_t);

	for(i=0; i<count; ++i)
		usamples[i] = CONV_SU16(ssamples[i]);
}

#ifndef NO_REAL
static void conv_s16_to_f32(struct outbuffer *buf)
{
	ssize_t i;
	int16_t *in = (int16_t*) buf->data;
	float  *out = (float*)   buf->data;
	size_t count = buf->fill/sizeof(int16_t);
	/* Does that make any sense? In x86, there is an actual instruction to divide
	   float by integer ... but then, if we have that FPU, we don't really need
	   fixed point decoder hacks ...? */
	float scale = 1./SHORT_SCALE;

	if(buf->size < count*sizeof(float))
	{
		error1("%s", bufsizeerr);
		return;
	}

	/* Work from the back since output is bigger. */
	for(i=count-1; i>=0; --i)
	out[i] = (float)in[i] * scale;

	buf->fill = count*sizeof(float);
}
#endif

#ifndef NO_32BIT
static void conv_s16_to_s32(struct outbuffer *buf)
{
	ssize_t i;
	int16_t  *in = (int16_t*) buf->data;
	int32_t *out = (int32_t*) buf->data;
	size_t count = buf->fill/sizeof(int16_t);

	if(buf->size < count*sizeof(int32_t))
	{
		error1("%s", bufsizeerr);
		return;
	}

	/* Work from the back since output is bigger. */
	for(i=count-1; i>=0; --i)
	{
		out[i] = in[i];
		/* Could just shift bytes, but would have to mess with sign bit. */
		out[i] *= S32_RESCALE;
	}

	buf->fill = count*sizeof(int32_t);
}
#endif
#endif

#include "swap_bytes_impl.h"

void swap_endian(struct outbuffer *buf, int block)
{
	size_t count;

	if(block >= 2)
	{
		count = buf->fill/(unsigned int)block;
		swap_bytes(buf->data, (size_t)block, count);
	}
}

void postprocess_buffer(mpg123_handle *fr)
{
	/*
		This caters for the final output formats that are never produced by
		decoder synth directly (wide unsigned and 24 bit formats) or that are
		missing because of limited decoder precision (16 bit synth but 32 or
		24 bit output).
	*/
	switch(fr->af.dec_enc)
	{
#ifndef NO_32BIT
	case MPG123_ENC_SIGNED_32:
		switch(fr->af.encoding)
		{
		case MPG123_ENC_UNSIGNED_32:
			conv_s32_to_u32(&fr->buffer);
		break;
		case MPG123_ENC_UNSIGNED_24:
			conv_s32_to_u32(&fr->buffer);
			chop_fourth_byte(&fr->buffer);
		break;
		case MPG123_ENC_SIGNED_24:
			chop_fourth_byte(&fr->buffer);
		break;
		}
	break;
#endif
#ifndef NO_16BIT
	case MPG123_ENC_SIGNED_16:
		switch(fr->af.encoding)
		{
		case MPG123_ENC_UNSIGNED_16:
			conv_s16_to_u16(&fr->buffer);
		break;
#ifndef NO_REAL
		case MPG123_ENC_FLOAT_32:
			conv_s16_to_f32(&fr->buffer);
		break;
#endif
#ifndef NO_32BIT
		case MPG123_ENC_SIGNED_32:
			conv_s16_to_s32(&fr->buffer);
		break;
		case MPG123_ENC_UNSIGNED_32:
			conv_s16_to_s32(&fr->buffer);
			conv_s32_to_u32(&fr->buffer);
		break;
		case MPG123_ENC_UNSIGNED_24:
			conv_s16_to_s32(&fr->buffer);
			conv_s32_to_u32(&fr->buffer);
			chop_fourth_byte(&fr->buffer);
		break;
		case MPG123_ENC_SIGNED_24:
			conv_s16_to_s32(&fr->buffer);
			chop_fourth_byte(&fr->buffer);
		break;
#endif
		}
	break;
#endif
	}
	if(fr->p.flags & MPG123_FORCE_ENDIAN)
	{
		if(
#ifdef WORDS_BIGENDIAN
			!(
#endif
				fr->p.flags & MPG123_BIG_ENDIAN
#ifdef WORDS_BIGENDIAN
			)
#endif
		)
			swap_endian(&fr->buffer, mpg123_encsize(fr->af.encoding));
	}
}
