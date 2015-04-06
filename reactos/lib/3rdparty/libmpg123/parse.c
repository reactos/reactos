/*
	parse: spawned from common; clustering around stream/frame parsing

	copyright ?-2014 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp & Thomas Orgis
*/

#include "mpg123lib_intern.h"

#include <sys/stat.h>
#include <fcntl.h>

#include "getbits.h"

#if defined (WANT_WIN32_SOCKETS)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

/* a limit for number of frames in a track; beyond that unsigned long may not be enough to hold byte addresses */
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifndef ULONG_MAX
/* hm, is this portable across preprocessors? */
#define ULONG_MAX ((unsigned long)-1)
#endif
#define TRACK_MAX_FRAMES ULONG_MAX/4/1152

#include "mpeghead.h"

#include "debug.h"

#define bsbufid(fr) (fr)->bsbuf==(fr)->bsspace[0] ? 0 : ((fr)->bsbuf==fr->bsspace[1] ? 1 : ( (fr)->bsbuf==(fr)->bsspace[0]+512 ? 2 : ((fr)->bsbuf==fr->bsspace[1]+512 ? 3 : -1) ) )

/* PARSE_GOOD and PARSE_BAD have to be 1 and 0 (TRUE and FALSE), others can vary. */
enum parse_codes
{
	 PARSE_MORE = MPG123_NEED_MORE
	,PARSE_ERR  = MPG123_ERR
	,PARSE_END  = 10 /* No more audio data to find. */
	,PARSE_GOOD = 1 /* Everything's fine. */
	,PARSE_BAD  = 0 /* Not fine (invalid data). */
	,PARSE_RESYNC = 2 /* Header not good, go into resync. */
	,PARSE_AGAIN  = 3 /* Really start over, throw away and read a new header, again. */
};

/* bitrates for [mpeg1/2][layer] */
static const int tabsel_123[2][3][16] =
{
	{
		{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
		{0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
		{0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,}
	},
	{
		{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
		{0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
		{0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,}
	}
};

static const long freqs[9] = { 44100, 48000, 32000, 22050, 24000, 16000 , 11025 , 12000 , 8000 };

static int decode_header(mpg123_handle *fr,unsigned long newhead, int *freeformat_count);
static int skip_junk(mpg123_handle *fr, unsigned long *newheadp, long *headcount);
static int do_readahead(mpg123_handle *fr, unsigned long newhead);
static int wetwork(mpg123_handle *fr, unsigned long *newheadp);

/* These two are to be replaced by one function that gives all the frame parameters (for outsiders).*/
/* Those functions are unsafe regarding bad arguments (inside the mpg123_handle), but just returning anything would also be unsafe, the caller code has to be trusted. */

int frame_bitrate(mpg123_handle *fr)
{
	return tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index];
}

long frame_freq(mpg123_handle *fr)
{
	return freqs[fr->sampling_frequency];
}

/* compiler is smart enought to inline this one or should I really do it as macro...? */
static int head_check(unsigned long head)
{
	if
	(
		((head & HDR_SYNC) != HDR_SYNC)
		||
		/* layer: 01,10,11 is 1,2,3; 00 is reserved */
		(!(HDR_LAYER_VAL(head)))
		||
		/* 1111 means bad bitrate */
		(HDR_BITRATE_VAL(head) == 0xf)
		||
		/* sampling freq: 11 is reserved */
		(HDR_SAMPLERATE_VAL(head) == 0x3)
		/* here used to be a mpeg 2.5 check... re-enabled 2.5 decoding due to lack of evidence that it is really not good */
	)
	{
		return FALSE;
	}
	/* if no check failed, the header is valid (hopefully)*/
	else
	{
		return TRUE;
	}
}

/* This is moderately sized buffers. Int offset is enough. */
static unsigned long bit_read_long(unsigned char *buf, int *offset)
{
	unsigned long val =  /* 32 bit value */
		(((unsigned long) buf[*offset])   << 24)
	|	(((unsigned long) buf[*offset+1]) << 16)
	|	(((unsigned long) buf[*offset+2]) << 8)
	|	 ((unsigned long) buf[*offset+3]);
	*offset += 4;
	return val;
}

static unsigned short bit_read_short(unsigned char *buf, int *offset)
{
	unsigned short val = /* 16 bit value */
		(((unsigned short) buf[*offset]  ) << 8)
	|	 ((unsigned short) buf[*offset+1]);
	*offset += 2;
	return val;
}

static int check_lame_tag(mpg123_handle *fr)
{
	int i;
	unsigned long xing_flags;
	unsigned long long_tmp;
	/*
		going to look for Xing or Info at some position after the header
		                                   MPEG 1  MPEG 2/2.5 (LSF)
		Stereo, Joint Stereo, Dual Channel  32      17
		Mono                                17       9
	*/
	int lame_offset = (fr->stereo == 2)
	? (fr->lsf ? 17 : 32)
	: (fr->lsf ? 9  : 17);

	if(fr->p.flags & MPG123_IGNORE_INFOFRAME) goto check_lame_tag_no;

	debug("do we have lame tag?");
	/*
		Note: CRC or not, that does not matter here.
		But, there is any combination of Xing flags in the wild. There are headers
		without the search index table! I cannot assume a reasonable minimal size
		for the actual data, have to check if each byte of information is present.
		But: 4 B Info/Xing + 4 B flags is bare minimum.
	*/
	if(fr->framesize < lame_offset+8) goto check_lame_tag_no;

	/* only search for tag when all zero before it (apart from checksum) */
	for(i=2; i < lame_offset; ++i) if(fr->bsbuf[i] != 0) goto check_lame_tag_no;

	debug("possibly...");
	if
	(
		   (fr->bsbuf[lame_offset]   == 'I')
		&& (fr->bsbuf[lame_offset+1] == 'n')
		&& (fr->bsbuf[lame_offset+2] == 'f')
		&& (fr->bsbuf[lame_offset+3] == 'o')
	)
	{
		/* We still have to see what there is */
	}
	else if
	(
		   (fr->bsbuf[lame_offset]   == 'X')
		&& (fr->bsbuf[lame_offset+1] == 'i')
		&& (fr->bsbuf[lame_offset+2] == 'n')
		&& (fr->bsbuf[lame_offset+3] == 'g')
	)
	{
		fr->vbr = MPG123_VBR; /* Xing header means always VBR */
	}
	else goto check_lame_tag_no;

	/* we have one of these headers... */
	if(VERBOSE2) fprintf(stderr, "Note: Xing/Lame/Info header detected\n");
	lame_offset += 4; 
	xing_flags = bit_read_long(fr->bsbuf, &lame_offset);
	debug1("Xing: flags 0x%08lx", xing_flags);

	/* From now on, I have to carefully check if the announced data is actually
	   there! I'm always returning 'yes', though.  */
	#define check_bytes_left(n) if(fr->framesize < lame_offset+n) \
		goto check_lame_tag_yes
	if(xing_flags & 1) /* total bitstream frames */
	{
		check_bytes_left(4); long_tmp = bit_read_long(fr->bsbuf, &lame_offset);
		if(fr->p.flags & MPG123_IGNORE_STREAMLENGTH)
		{
			if(VERBOSE3) fprintf(stderr
			,	"Note: Ignoring Xing frames because of MPG123_IGNORE_STREAMLENGTH\n");
		}
		else
		{
			/* Check for endless stream, but: TRACK_MAX_FRAMES sensible at all? */
			fr->track_frames = long_tmp > TRACK_MAX_FRAMES ? 0 : (off_t) long_tmp;
#ifdef GAPLESS
			/* All or nothing: Only if encoder delay/padding is known, we'll cut
			   samples for gapless. */
			if(fr->p.flags & MPG123_GAPLESS)
			frame_gapless_init(fr, fr->track_frames, 0, 0);
#endif
			if(VERBOSE3) fprintf(stderr, "Note: Xing: %lu frames\n", long_tmp);
		}
	}
	if(xing_flags & 0x2) /* total bitstream bytes */
	{
		check_bytes_left(4); long_tmp = bit_read_long(fr->bsbuf, &lame_offset);
		if(fr->p.flags & MPG123_IGNORE_STREAMLENGTH)
		{
			if(VERBOSE3) fprintf(stderr
			,	"Note: Ignoring Xing bytes because of MPG123_IGNORE_STREAMLENGTH\n");
		}
		else
		{
			/* The Xing bitstream length, at least as interpreted by the Lame
			   encoder, encompasses all data from the Xing header frame on,
			   ignoring leading ID3v2 data. Trailing tags (ID3v1) seem to be 
			   included, though. */
			if(fr->rdat.filelen < 1)
			fr->rdat.filelen = (off_t) long_tmp + fr->audio_start; /* Overflow? */
			else
			{
				if((off_t)long_tmp != fr->rdat.filelen - fr->audio_start && NOQUIET)
				{ /* 1/filelen instead of 1/(filelen-start), my decision */
					double diff = 100.0/fr->rdat.filelen
					            * ( fr->rdat.filelen - fr->audio_start
					                - (off_t)long_tmp );
					if(diff < 0.) diff = -diff;

					if(VERBOSE3) fprintf(stderr
					,	"Note: Xing stream size %lu differs by %f%% from determined/given file size!\n"
					,	long_tmp, diff);

					if(diff > 1. && NOQUIET) fprintf(stderr
					,	"Warning: Xing stream size off by more than 1%%, fuzzy seeking may be even more fuzzy than by design!\n");
				}
			}

			if(VERBOSE3) fprintf(stderr, "Note: Xing: %lu bytes\n", long_tmp);
		}
	}
	if(xing_flags & 0x4) /* TOC */
	{
		check_bytes_left(100);
		frame_fill_toc(fr, fr->bsbuf+lame_offset);
		lame_offset += 100;
	}
	if(xing_flags & 0x8) /* VBR quality */
	{
		check_bytes_left(4); long_tmp = bit_read_long(fr->bsbuf, &lame_offset);
		if(VERBOSE3) fprintf(stderr, "Note: Xing: quality = %lu\n", long_tmp);
	}
	/*
		Either zeros/nothing, or:
			0-8: LAME3.90a
			9: revision/VBR method
			10: lowpass
			11-18: ReplayGain
			19: encoder flags
			20: ABR 
			21-23: encoder delays
	*/
	check_bytes_left(24); /* I'm interested in 24 B of extra info. */
	if(fr->bsbuf[lame_offset] != 0)
	{
		unsigned char lame_vbr;
		float replay_gain[2] = {0,0};
		float peak = 0;
		float gain_offset = 0; /* going to be +6 for old lame that used 83dB */
		char nb[10];
		off_t pad_in;
		off_t pad_out;
		memcpy(nb, fr->bsbuf+lame_offset, 9);
		nb[9] = 0;
		if(VERBOSE3) fprintf(stderr, "Note: Info: Encoder: %s\n", nb);
		if(!strncmp("LAME", nb, 4))
		{
			/* Lame versions before 3.95.1 used 83 dB reference level, later
			   versions 89 dB. We stick with 89 dB as being "normal", adding
			   6 dB. */
			unsigned int major, minor;
			char rest[6];
			rest[0] = 0;
			if(sscanf(nb+4, "%u.%u%s", &major, &minor, rest) >= 2)
			{
				debug3("LAME: %u/%u/%s", major, minor, rest);
				/* We cannot detect LAME 3.95 reliably (same version string as
				   3.95.1), so this is a blind spot. Everything < 3.95 is safe,
				   though. */
				if(major < 3 || (major == 3 && minor < 95))
				{
					gain_offset = 6;
					if(VERBOSE3) fprintf(stderr
					,	"Note: Info: Old LAME detected, using ReplayGain preamp of %f dB.\n"
					,	gain_offset);
				}
			}
			else if(VERBOSE3) fprintf(stderr
			,	"Note: Info: Cannot determine LAME version.\n");
		}
		lame_offset += 9; /* 9 in */ 

		/* The 4 big bits are tag revision, the small bits vbr method. */
		lame_vbr = fr->bsbuf[lame_offset] & 15;
		lame_offset += 1; /* 10 in */
		if(VERBOSE3)
		{
			fprintf(stderr, "Note: Info: rev %u\n", fr->bsbuf[lame_offset] >> 4);
			fprintf(stderr, "Note: Info: vbr mode %u\n", lame_vbr);
		}
		switch(lame_vbr)
		{
			/* from rev1 proposal... not sure if all good in practice */
			case 1:
			case 8: fr->vbr = MPG123_CBR; break;
			case 2:
			case 9: fr->vbr = MPG123_ABR; break;
			default: fr->vbr = MPG123_VBR; /* 00==unknown is taken as VBR */
		}
		lame_offset += 1; /* 11 in, skipping lowpass filter value */

		/* ReplayGain peak ampitude, 32 bit float -- why did I parse it as int
		   before??	Ah, yes, Lame seems to store it as int since some day in 2003;
		   I've only seen zeros anyway until now, bah! */
		if
		(
			   (fr->bsbuf[lame_offset]   != 0)
			|| (fr->bsbuf[lame_offset+1] != 0)
			|| (fr->bsbuf[lame_offset+2] != 0)
			|| (fr->bsbuf[lame_offset+3] != 0)
		)
		{
			debug("Wow! Is there _really_ a non-zero peak value? Now is it stored as float or int - how should I know?");
			/* byte*peak_bytes = (byte*) &peak;
			... endianess ... just copy bytes to avoid floating point operation on unaligned memory?
			peak_bytes[0] = ...
			peak = *(float*) (fr->bsbuf+lame_offset); */
		}
		if(VERBOSE3) fprintf(stderr
		,	"Note: Info: peak = %f (I won't use this)\n", peak);
		peak = 0; /* until better times arrived */
		lame_offset += 4; /* 15 in */

		/* ReplayGain values - lame only writes radio mode gain...
		   16bit gain, 3 bits name, 3 bits originator, sign (1=-, 0=+),
		   dB value*10 in 9 bits (fixed point) ignore the setting if name or
		   originator == 000!
		   radio      0 0 1 0 1 1 1 0 0 1 1 1 1 1 0 1
		   audiophile 0 1 0 0 1 0 0 0 0 0 0 1 0 1 0 0 */
		for(i =0; i < 2; ++i)
		{
			unsigned char gt     =  fr->bsbuf[lame_offset] >> 5;
			unsigned char origin = (fr->bsbuf[lame_offset] >> 2) & 0x7;
			float factor         = (fr->bsbuf[lame_offset] & 0x2) ? -0.1 : 0.1;
			unsigned short gain  = bit_read_short(fr->bsbuf, &lame_offset) & 0x1ff; /* 19 in (2 cycles) */
			if(origin == 0 || gt < 1 || gt > 2) continue;

			--gt;
			replay_gain[gt] = factor * (float) gain;
			/* Apply gain offset for automatic origin. */
			if(origin == 3) replay_gain[gt] += gain_offset;
		}
		if(VERBOSE3) 
		{
			fprintf(stderr, "Note: Info: Radio Gain = %03.1fdB\n"
			,	replay_gain[0]);
			fprintf(stderr, "Note: Info: Audiophile Gain = %03.1fdB\n"
			,	replay_gain[1]);
		}
		for(i=0; i < 2; ++i)
		{
			if(fr->rva.level[i] <= 0)
			{
				fr->rva.peak[i] = 0; /* TODO: use parsed peak? */
				fr->rva.gain[i] = replay_gain[i];
				fr->rva.level[i] = 0;
			}
		}

		lame_offset += 1; /* 20 in, skipping encoding flags byte */

		/* ABR rate */
		if(fr->vbr == MPG123_ABR)
		{
			fr->abr_rate = fr->bsbuf[lame_offset];
			if(VERBOSE3) fprintf(stderr, "Note: Info: ABR rate = %u\n"
			,	fr->abr_rate);
		}
		lame_offset += 1; /* 21 in */
	
		/* Encoder delay and padding, two 12 bit values
		   ... lame does write them from int. */
		pad_in  = ( (((int) fr->bsbuf[lame_offset])   << 4)
		          | (((int) fr->bsbuf[lame_offset+1]) >> 4) );
		pad_out = ( (((int) fr->bsbuf[lame_offset+1]) << 8)
		          |  ((int) fr->bsbuf[lame_offset+2])       ) & 0xfff;
		lame_offset += 3; /* 24 in */
		if(VERBOSE3) fprintf(stderr, "Note: Encoder delay = %i; padding = %i\n"
		,	(int)pad_in, (int)pad_out);
		#ifdef GAPLESS
		if(fr->p.flags & MPG123_GAPLESS)
		frame_gapless_init(fr, fr->track_frames, pad_in, pad_out);
		#endif
		/* final: 24 B LAME data */
	}

check_lame_tag_yes:
	/* switch buffer back ... */
	fr->bsbuf = fr->bsspace[fr->bsnum]+512;
	fr->bsnum = (fr->bsnum + 1) & 1;
	return 1;
check_lame_tag_no:
	return 0;
}

/* Just tell if the header is some mono. */
static int header_mono(unsigned long newhead)
{
	return HDR_CHANNEL_VAL(newhead) == MPG_MD_MONO ? TRUE : FALSE;
}

/* true if the two headers will work with the same decoding routines */
static int head_compatible(unsigned long fred, unsigned long bret)
{
	return ( (fred & HDR_CMPMASK) == (bret & HDR_CMPMASK)
		&&       header_mono(fred) == header_mono(bret)    );
}

static void halfspeed_prepare(mpg123_handle *fr)
{
	/* save for repetition */
	if(fr->p.halfspeed && fr->lay == 3)
	{
		debug("halfspeed - reusing old bsbuf ");
		memcpy (fr->ssave, fr->bsbuf, fr->ssize);
	}
}

/* If this returns 1, the next frame is the repetition. */
static int halfspeed_do(mpg123_handle *fr)
{
	/* Speed-down hack: Play it again, Sam (the frame, I mean). */
	if (fr->p.halfspeed) 
	{
		if(fr->halfphase) /* repeat last frame */
		{
			debug("repeat!");
			fr->to_decode = fr->to_ignore = TRUE;
			--fr->halfphase;
			fr->bitindex = 0;
			fr->wordpointer = (unsigned char *) fr->bsbuf;
			if(fr->lay == 3) memcpy (fr->bsbuf, fr->ssave, fr->ssize);
			if(fr->error_protection) fr->crc = getbits(fr, 16); /* skip crc */
			return 1;
		}
		else
		{
			fr->halfphase = fr->p.halfspeed - 1;
		}
	}
	return 0;
}

/* 
	Temporary macro until we got this worked out.
	Idea is to filter out special return values that shall trigger direct jumps to end / resync / read again. 
	Particularily, the generic ret==PARSE_BAD==0 and ret==PARSE_GOOD==1 are not affected.
*/
#define JUMP_CONCLUSION(ret) \
{ \
if(ret < 0){ debug1("%s", ret == MPG123_NEED_MORE ? "need more" : "read error"); goto read_frame_bad; } \
else if(ret == PARSE_AGAIN) goto read_again; \
else if(ret == PARSE_RESYNC) goto init_resync; \
else if(ret == PARSE_END){ ret=0; goto read_frame_bad; } \
}

/*
	That's a big one: read the next frame. 1 is success, <= 0 is some error
	Special error READER_MORE means: Please feed more data and try again.
*/
int read_frame(mpg123_handle *fr)
{
	/* TODO: rework this thing */
	int freeformat_count = 0;
	unsigned long newhead;
	off_t framepos;
	int ret;
	/* stuff that needs resetting if complete frame reading fails */
	int oldsize  = fr->framesize;
	int oldphase = fr->halfphase;

	/* The counter for the search-first-header loop.
	   It is persistent outside the loop to prevent seemingly endless loops
	   when repeatedly headers are found that do not have valid followup headers. */
	long headcount = 0;

	fr->fsizeold=fr->framesize;       /* for Layer3 */

	if(halfspeed_do(fr) == 1) return 1;

read_again:
	/* In case we are looping to find a valid frame, discard any buffered data before the current position.
	   This is essential to prevent endless looping, always going back to the beginning when feeder buffer is exhausted. */
	if(fr->rd->forget != NULL) fr->rd->forget(fr);

	debug2("trying to get frame %"OFF_P" at %"OFF_P, (off_p)fr->num+1, (off_p)fr->rd->tell(fr));
	if((ret = fr->rd->head_read(fr,&newhead)) <= 0){ debug1("need more? (%i)", ret); goto read_frame_bad;}

init_resync:

#ifdef SKIP_JUNK
	if(!fr->firsthead && !head_check(newhead))
	{
		ret = skip_junk(fr, &newhead, &headcount);
		JUMP_CONCLUSION(ret);
	}
#endif

	ret = head_check(newhead);
	if(ret) ret = decode_header(fr, newhead, &freeformat_count);

	JUMP_CONCLUSION(ret); /* That only continues for ret == PARSE_BAD or PARSE_GOOD. */
	if(ret == PARSE_BAD)
	{ /* Header was not good. */
		ret = wetwork(fr, &newhead); /* Messy stuff, handle junk, resync ... */
		JUMP_CONCLUSION(ret);
		/* Normally, we jumped already. If for some reason everything's fine to continue, do continue. */
		if(ret != PARSE_GOOD) goto read_frame_bad;
	}

	if(!fr->firsthead)
	{
		ret = do_readahead(fr, newhead);
		/* readahead can fail mit NEED_MORE, in which case we must also make the just read header available again for next go */
		if(ret < 0) fr->rd->back_bytes(fr, 4);
		JUMP_CONCLUSION(ret);
	}

	/* Now we should have our valid header and proceed to reading the frame. */

	/* if filepos is invalid, so is framepos */
	framepos = fr->rd->tell(fr) - 4;
	/* flip/init buffer for Layer 3 */
	{
		unsigned char *newbuf = fr->bsspace[fr->bsnum]+512;
		/* read main data into memory */
		if((ret=fr->rd->read_frame_body(fr,newbuf,fr->framesize))<0)
		{
			/* if failed: flip back */
			debug("need more?");
			goto read_frame_bad;
		}
		fr->bsbufold = fr->bsbuf;
		fr->bsbuf = newbuf;
	}
	fr->bsnum = (fr->bsnum + 1) & 1;

	if(!fr->firsthead)
	{
		fr->firsthead = newhead; /* _now_ it's time to store it... the first real header */
		/* This is the first header of our current stream segment.
		   It is only the actual first header of the whole stream when fr->num is still below zero!
		   Think of resyncs where firsthead has been reset for format flexibility. */
		if(fr->num < 0)
		{
			fr->audio_start = framepos;
			/* Only check for LAME  tag at beginning of whole stream
			   ... when there indeed is one in between, it's the user's problem. */
			if(fr->lay == 3 && check_lame_tag(fr) == 1)
			{ /* ...in practice, Xing/LAME tags are layer 3 only. */
				if(fr->rd->forget != NULL) fr->rd->forget(fr);

				fr->oldhead = 0;
				goto read_again;
			}
			/* now adjust volume */
			do_rva(fr);
		}

		debug2("fr->firsthead: %08lx, audio_start: %li", fr->firsthead, (long int)fr->audio_start);
	}

  fr->bitindex = 0;
  fr->wordpointer = (unsigned char *) fr->bsbuf;
	/* Question: How bad does the floating point value get with repeated recomputation?
	   Also, considering that we can play the file or parts of many times. */
	if(++fr->mean_frames != 0)
	{
		fr->mean_framesize = ((fr->mean_frames-1)*fr->mean_framesize+compute_bpf(fr)) / fr->mean_frames ;
	}
	++fr->num; /* 0 for first frame! */
	debug4("Frame %"OFF_P" %08lx %i, next filepos=%"OFF_P, 
	(off_p)fr->num, newhead, fr->framesize, (off_p)fr->rd->tell(fr));
	if(!(fr->state_flags & FRAME_FRANKENSTEIN) && (
		(fr->track_frames > 0 && fr->num >= fr->track_frames)
#ifdef GAPLESS
		|| (fr->gapless_frames > 0 && fr->num >= fr->gapless_frames)
#endif
	))
	{
		fr->state_flags |= FRAME_FRANKENSTEIN;
		if(NOQUIET) fprintf(stderr, "\nWarning: Encountered more data after announced end of track (frame %"OFF_P"/%"OFF_P"). Frankenstein!\n", (off_p)fr->num, 
#ifdef GAPLESS
		fr->gapless_frames > 0 ? (off_p)fr->gapless_frames : 
#endif
		(off_p)fr->track_frames);
	}

	halfspeed_prepare(fr);

	/* index the position */
	fr->input_offset = framepos;
#ifdef FRAME_INDEX
	/* Keep track of true frame positions in our frame index.
	   but only do so when we are sure that the frame number is accurate... */
	if((fr->state_flags & FRAME_ACCURATE) && FI_NEXT(fr->index, fr->num))
	fi_add(&fr->index, framepos);
#endif

	if(fr->silent_resync > 0) --fr->silent_resync;

	if(fr->rd->forget != NULL) fr->rd->forget(fr);

	fr->to_decode = fr->to_ignore = TRUE;
	if(fr->error_protection) fr->crc = getbits(fr, 16); /* skip crc */

	/*
		Let's check for header change after deciding that the new one is good
		and actually having read a frame.

		header_change > 1: decoder structure has to be updated
		Preserve header_change value from previous runs if it is serious.
		If we still have a big change pending, it should be dealt with outside,
		fr->header_change set to zero afterwards.
	*/
	if(fr->header_change < 2)
	{
		fr->header_change = 2; /* output format change is possible... */
		if(fr->oldhead)        /* check a following header for change */
		{
			if(fr->oldhead == newhead) fr->header_change = 0;
			else
			/* Headers that match in this test behave the same for the outside world.
			   namely: same decoding routines, same amount of decoded data. */
			if(head_compatible(fr->oldhead, newhead))
			fr->header_change = 1;
			else
			{
				fr->state_flags |= FRAME_FRANKENSTEIN;
				if(NOQUIET)
				fprintf(stderr, "\nWarning: Big change (MPEG version, layer, rate). Frankenstein stream?\n");
			}
		}
		else if(fr->firsthead && !head_compatible(fr->firsthead, newhead))
		{
			fr->state_flags |= FRAME_FRANKENSTEIN;
			if(NOQUIET)
			fprintf(stderr, "\nWarning: Big change from first (MPEG version, layer, rate). Frankenstein stream?\n");
		}
	}

	fr->oldhead = newhead;

	return 1;
read_frame_bad:
	/* Also if we searched for valid data in vein, we can forget skipped data.
	   Otherwise, the feeder would hold every dead old byte in memory until the first valid frame! */
	if(fr->rd->forget != NULL) fr->rd->forget(fr);

	fr->silent_resync = 0;
	if(fr->err == MPG123_OK) fr->err = MPG123_ERR_READER;
	fr->framesize = oldsize;
	fr->halfphase = oldphase;
	/* That return code might be inherited from some feeder action, or reader error. */
	return ret;
}


/*
 * read ahead and find the next MPEG header, to guess framesize
 * return value: success code
 *  PARSE_GOOD: found a valid frame size (stored in the handle).
 * <0: error codes, possibly from feeder buffer (NEED_MORE)
 *  PARSE_BAD: cannot get the framesize for some reason and shall silentry try the next possible header (if this is no free format stream after all...)
 */
static int guess_freeformat_framesize(mpg123_handle *fr)
{
	long i;
	int ret;
	unsigned long head;
	if(!(fr->rdat.flags & (READER_SEEKABLE|READER_BUFFERED)))
	{
		if(NOQUIET) error("Cannot look for freeformat frame size with non-seekable and non-buffered stream!");

		return PARSE_BAD;
	}
	if((ret=fr->rd->head_read(fr,&head))<=0)
	return ret;

	/* We are already 4 bytes into it */
	for(i=4;i<MAXFRAMESIZE+4;i++)
	{
		if((ret=fr->rd->head_shift(fr,&head))<=0) return ret;

		/* No head_check needed, the mask contains all relevant bits. */
		if((head & HDR_SAMEMASK) == (fr->oldhead & HDR_SAMEMASK))
		{
			fr->rd->back_bytes(fr,i+1);
			fr->framesize = i-3;
			return PARSE_GOOD; /* Success! */
		}
	}
	fr->rd->back_bytes(fr,i);
	return PARSE_BAD;
}


/*
 * decode a header and write the information
 * into the frame structure
 * Return values are compatible with those of read_frame, namely:
 *  1: success
 *  0: no valid header
 * <0: some error
 * You are required to do a head_check() before calling!
 */
static int decode_header(mpg123_handle *fr,unsigned long newhead, int *freeformat_count)
{
#ifdef DEBUG /* Do not waste cycles checking the header twice all the time. */
	if(!head_check(newhead))
	{
		error1("trying to decode obviously invalid header 0x%08lx", newhead);
	}
#endif
	/* For some reason, the layer and sampling freq settings used to be wrapped
	   in a weird conditional including MPG123_NO_RESYNC. What was I thinking?
	   This information has to be consistent. */
	fr->lay = 4 - HDR_LAYER_VAL(newhead);

	if(HDR_VERSION_VAL(newhead) & 0x2)
	{
		fr->lsf = (HDR_VERSION_VAL(newhead) & 0x1) ? 0 : 1;
		fr->mpeg25 = 0;
		fr->sampling_frequency = HDR_SAMPLERATE_VAL(newhead) + (fr->lsf*3);
	}
	else
	{
		fr->lsf = 1;
		fr->mpeg25 = 1;
		fr->sampling_frequency = 6 + HDR_SAMPLERATE_VAL(newhead);
	}

	#ifdef DEBUG
	/* seen a file where this varies (old lame tag without crc, track with crc) */
	if((HDR_CRC_VAL(newhead)^0x1) != fr->error_protection) debug("changed crc bit!");
	#endif
	fr->error_protection = HDR_CRC_VAL(newhead)^0x1;
	fr->bitrate_index    = HDR_BITRATE_VAL(newhead);
	fr->padding          = HDR_PADDING_VAL(newhead);
	fr->extension        = HDR_PRIVATE_VAL(newhead);
	fr->mode             = HDR_CHANNEL_VAL(newhead);
	fr->mode_ext         = HDR_CHANEX_VAL(newhead);
	fr->copyright        = HDR_COPYRIGHT_VAL(newhead);
	fr->original         = HDR_ORIGINAL_VAL(newhead);
	fr->emphasis         = HDR_EMPHASIS_VAL(newhead);
	fr->freeformat       = !(newhead & HDR_BITRATE);

	fr->stereo = (fr->mode == MPG_MD_MONO) ? 1 : 2;

	/* we can't use tabsel_123 for freeformat, so trying to guess framesize... */
	if(fr->freeformat)
	{
		/* when we first encounter the frame with freeformat, guess framesize */
		if(fr->freeformat_framesize < 0)
		{
			int ret;
			*freeformat_count += 1;
			if(*freeformat_count > 5)
			{
				if(VERBOSE3) error("You fooled me too often. Refusing to guess free format frame size _again_.");
				return PARSE_BAD;
			}
			ret = guess_freeformat_framesize(fr);
			if(ret == PARSE_GOOD)
			{
				fr->freeformat_framesize = fr->framesize - fr->padding;
				if(VERBOSE2)
				fprintf(stderr, "Note: free format frame size %li\n", fr->freeformat_framesize);
			}
			else
			{
				if(ret == MPG123_NEED_MORE)
				debug("Need more data to guess free format frame size.");
				else if(VERBOSE3)
				error("Encountered free format header, but failed to guess frame size.");

				return ret;
			}
		}
		/* freeformat should be CBR, so the same framesize can be used at the 2nd reading or later */
		else
		{
			fr->framesize = fr->freeformat_framesize + fr->padding;
		}
	}

	switch(fr->lay)
	{
#ifndef NO_LAYER1
		case 1:
			fr->spf = 384;
			fr->do_layer = do_layer1;
			if(!fr->freeformat)
			{
				fr->framesize  = (long) tabsel_123[fr->lsf][0][fr->bitrate_index] * 12000;
				fr->framesize /= freqs[fr->sampling_frequency];
				fr->framesize  = ((fr->framesize+fr->padding)<<2)-4;
			}
		break;
#endif
#ifndef NO_LAYER2
		case 2:
			fr->spf = 1152;
			fr->do_layer = do_layer2;
			if(!fr->freeformat)
			{
				debug2("bitrate index: %i (%i)", fr->bitrate_index, tabsel_123[fr->lsf][1][fr->bitrate_index] );
				fr->framesize = (long) tabsel_123[fr->lsf][1][fr->bitrate_index] * 144000;
				fr->framesize /= freqs[fr->sampling_frequency];
				fr->framesize += fr->padding - 4;
			}
		break;
#endif
#ifndef NO_LAYER3
		case 3:
			fr->spf = fr->lsf ? 576 : 1152; /* MPEG 2.5 implies LSF.*/
			fr->do_layer = do_layer3;
			if(fr->lsf)
			fr->ssize = (fr->stereo == 1) ? 9 : 17;
			else
			fr->ssize = (fr->stereo == 1) ? 17 : 32;

			if(fr->error_protection)
			fr->ssize += 2;

			if(!fr->freeformat)
			{
				fr->framesize  = (long) tabsel_123[fr->lsf][2][fr->bitrate_index] * 144000;
				fr->framesize /= freqs[fr->sampling_frequency]<<(fr->lsf);
				fr->framesize = fr->framesize + fr->padding - 4;
			}
		break;
#endif 
		default:
			if(NOQUIET) error1("Layer type %i not supported in this build!", fr->lay); 

			return PARSE_BAD;
	}
	if (fr->framesize > MAXFRAMESIZE)
	{
		if(NOQUIET) error1("Frame size too big: %d", fr->framesize+4-fr->padding);

		return PARSE_BAD;
	}
	return PARSE_GOOD;
}

void set_pointer(mpg123_handle *fr, long backstep)
{
	fr->wordpointer = fr->bsbuf + fr->ssize - backstep;
	if (backstep)
	memcpy(fr->wordpointer,fr->bsbufold+fr->fsizeold-backstep,backstep);

	fr->bitindex = 0; 
}

/********************************/

double compute_bpf(mpg123_handle *fr)
{
	double bpf;

	switch(fr->lay) 
	{
		case 1:
			bpf = tabsel_123[fr->lsf][0][fr->bitrate_index];
			bpf *= 12000.0 * 4.0;
			bpf /= freqs[fr->sampling_frequency] <<(fr->lsf);
		break;
		case 2:
		case 3:
			bpf = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index];
			bpf *= 144000;
			bpf /= freqs[fr->sampling_frequency] << (fr->lsf);
		break;
		default:
			bpf = 1.0;
	}

	return bpf;
}

int attribute_align_arg mpg123_spf(mpg123_handle *mh)
{
	if(mh == NULL) return MPG123_ERR;

	return mh->firsthead ? mh->spf : MPG123_ERR;
}

double attribute_align_arg mpg123_tpf(mpg123_handle *fr)
{
	static int bs[4] = { 0,384,1152,1152 };
	double tpf;
	if(fr == NULL || !fr->firsthead) return MPG123_ERR;

	tpf = (double) bs[fr->lay];
	tpf /= freqs[fr->sampling_frequency] << (fr->lsf);
	return tpf;
}

int attribute_align_arg mpg123_position(mpg123_handle *fr, off_t no, off_t buffsize,
	off_t  *current_frame,   off_t  *frames_left,
	double *current_seconds, double *seconds_left)
{
	double tpf;
	double dt = 0.0;
	off_t cur, left;
	double curs, lefts;

	if(!fr || !fr->rd) return MPG123_ERR;

	no += fr->num; /* no starts out as offset */
	cur = no;
	tpf = mpg123_tpf(fr);
	if(buffsize > 0 && fr->af.rate > 0 && fr->af.channels > 0)
	{
		dt = (double) buffsize / fr->af.rate / fr->af.channels;
		if(fr->af.encoding & MPG123_ENC_16) dt *= 0.5;
	}

	left = 0;

	if((fr->track_frames != 0) && (fr->track_frames >= fr->num)) left = no < fr->track_frames ? fr->track_frames - no : 0;
	else
	if(fr->rdat.filelen >= 0)
	{
		double bpf;
		off_t t = fr->rd->tell(fr);
		bpf = fr->mean_framesize ? fr->mean_framesize : compute_bpf(fr);
		left = (off_t)((double)(fr->rdat.filelen-t)/bpf);
		/* no can be different for prophetic purposes, file pointer is always associated with fr->num! */
		if(fr->num != no)
		{
			if(fr->num > no) left += fr->num - no;
			else
			{
				if(left >= (no - fr->num)) left -= no - fr->num;
				else left = 0; /* uh, oh! */
			}
		}
		/* I totally don't understand why we should re-estimate the given correct(?) value */
		/* fr->num = (unsigned long)((double)t/bpf); */
	}

	/* beginning with 0 or 1?*/
	curs = (double) no*tpf-dt;
	lefts = (double)left*tpf+dt;
#if 0
	curs = curs < 0 ? 0.0 : curs;
#endif
	if(left < 0 || lefts < 0)
	{ /* That is the case for non-seekable streams. */
		left  = 0;
		lefts = 0.0;
	}
	if(current_frame != NULL) *current_frame = cur;
	if(frames_left   != NULL) *frames_left   = left;
	if(current_seconds != NULL) *current_seconds = curs;
	if(seconds_left    != NULL) *seconds_left   = lefts;
	return MPG123_OK;
}

int get_songlen(mpg123_handle *fr,int no)
{
	double tpf;
	
	if(!fr)
		return 0;
	
	if(no < 0) {
		if(!fr->rd || fr->rdat.filelen < 0)
			return 0;
		no = (int) ((double) fr->rdat.filelen / compute_bpf(fr));
	}

	tpf = mpg123_tpf(fr);
	return (int) (no*tpf);
}

/* first attempt of read ahead check to find the real first header; cannot believe what junk is out there! */
static int do_readahead(mpg123_handle *fr, unsigned long newhead)
{
	unsigned long nexthead = 0;
	int hd = 0;
	off_t start, oret;
	int ret;

	if( ! (!fr->firsthead && fr->rdat.flags & (READER_SEEKABLE|READER_BUFFERED)) )
	return PARSE_GOOD;

	start = fr->rd->tell(fr);

	debug2("doing ahead check with BPF %d at %"OFF_P, fr->framesize+4, (off_p)start);
	/* step framesize bytes forward and read next possible header*/
	if((oret=fr->rd->skip_bytes(fr, fr->framesize))<0)
	{
		if(oret==READER_ERROR && NOQUIET) error("cannot seek!");

		return oret == MPG123_NEED_MORE ? PARSE_MORE : PARSE_ERR;
	}

	/* Read header, seek back. */
	hd = fr->rd->head_read(fr,&nexthead);
	if( fr->rd->back_bytes(fr, fr->rd->tell(fr)-start) < 0 )
	{
		if(NOQUIET) error("Cannot seek back!");

		return PARSE_ERR;
	}
	if(hd == MPG123_NEED_MORE) return PARSE_MORE;

	debug1("After fetching next header, at %"OFF_P, (off_p)fr->rd->tell(fr));
	if(!hd)
	{
		if(NOQUIET) warning("Cannot read next header, a one-frame stream? Duh...");
		return PARSE_END;
	}

	debug2("does next header 0x%08lx match first 0x%08lx?", nexthead, newhead);
	if(!head_check(nexthead) || !head_compatible(newhead, nexthead))
	{
		debug("No, the header was not valid, start from beginning...");
		fr->oldhead = 0; /* start over */
		/* try next byte for valid header */
		if((ret=fr->rd->back_bytes(fr, 3))<0)
		{
			if(NOQUIET) error("Cannot seek 3 bytes back!");

			return PARSE_ERR;
		}
		return PARSE_AGAIN;
	}
	else return PARSE_GOOD;
}

static int handle_id3v2(mpg123_handle *fr, unsigned long newhead)
{
	int ret;
	fr->oldhead = 0; /* Think about that. Used to be present only for skipping of junk, not resync-style wetwork. */
	ret = parse_new_id3(fr, newhead);
	if     (ret < 0) return ret;
#ifndef NO_ID3V2
	else if(ret > 0){ debug("got ID3v2"); fr->metaflags  |= MPG123_NEW_ID3|MPG123_ID3; }
	else debug("no useful ID3v2");
#endif
	return PARSE_AGAIN;
}

/* Advance a byte in stream to get next possible header and forget 
   buffered data if possible (for feed reader). */
#define FORGET_INTERVAL 1024 /* Used by callers to set forget flag each <n> bytes. */
static int forget_head_shift(mpg123_handle *fr, unsigned long *newheadp, int forget)
{
	int ret;
	if((ret=fr->rd->head_shift(fr,newheadp))<=0) return ret;
	/* Try to forget buffered data as early as possible to speed up parsing where
	   new data needs to be added for resync (and things would be re-parsed again
	   and again because of the start from beginning after hitting end). */
	if(forget && fr->rd->forget != NULL)
	{
		/* Ensure that the last 4 bytes stay in buffers for reading the header
		   anew. */
		if(!fr->rd->back_bytes(fr, 4))
		{
			fr->rd->forget(fr);
			fr->rd->back_bytes(fr, -4);
		}
	}
	return ret; /* No surprise here, error already triggered early return. */
}

/* watch out for junk/tags on beginning of stream by invalid header */
static int skip_junk(mpg123_handle *fr, unsigned long *newheadp, long *headcount)
{
	int ret;
	int freeformat_count = 0;
	long limit = 65536;
	unsigned long newhead = *newheadp;
	unsigned int forgetcount = 0;
	/* check for id3v2; first three bytes (of 4) are "ID3" */
	if((newhead & (unsigned long) 0xffffff00) == (unsigned long) 0x49443300)
	{
		return handle_id3v2(fr, newhead);
	}
	else if(VERBOSE2 && fr->silent_resync == 0) fprintf(stderr,"Note: Junk at the beginning (0x%08lx)\n",newhead);

	/* I even saw RIFF headers at the beginning of MPEG streams ;( */
	if(newhead == ('R'<<24)+('I'<<16)+('F'<<8)+'F')
	{
		if(VERBOSE2 && fr->silent_resync == 0) fprintf(stderr, "Note: Looks like a RIFF header.\n");

		if((ret=fr->rd->head_read(fr,&newhead))<=0) return ret;

		while(newhead != ('d'<<24)+('a'<<16)+('t'<<8)+'a')
		{
			if(++forgetcount > FORGET_INTERVAL) forgetcount = 0;
			if((ret=forget_head_shift(fr,&newhead,!forgetcount))<=0) return ret;
		}
		if((ret=fr->rd->head_read(fr,&newhead))<=0) return ret;

		if(VERBOSE2 && fr->silent_resync == 0) fprintf(stderr,"Note: Skipped RIFF header!\n");

		fr->oldhead = 0;
		*newheadp = newhead;
		return PARSE_AGAIN;
	}

	/*
		Unhandled junk... just continue search for a header, stepping in single bytes through next 64K.
		This is rather identical to the resync loop.
	*/
	debug("searching for header...");
	*newheadp = 0; /* Invalidate the external value. */
	ret = 0; /* We will check the value after the loop. */

	/* We prepare for at least the 64K bytes as usual, unless
	   user explicitly wanted more (even infinity). Never less. */
	if(fr->p.resync_limit < 0 || fr->p.resync_limit > limit)
	limit = fr->p.resync_limit;

	do
	{
		++(*headcount);
		if(limit >= 0 && *headcount >= limit) break;				

		if(++forgetcount > FORGET_INTERVAL) forgetcount = 0;
		if((ret=forget_head_shift(fr, &newhead, !forgetcount))<=0) return ret;

		if(head_check(newhead) && (ret=decode_header(fr, newhead, &freeformat_count))) break;
	} while(1);
	if(ret<0) return ret;

	if(limit >= 0 && *headcount >= limit)
	{
		if(NOQUIET) error1("Giving up searching valid MPEG header after %li bytes of junk.", *headcount);
		return PARSE_END;
	}
	else debug1("hopefully found one at %"OFF_P, (off_p)fr->rd->tell(fr));

	/* If the new header ist good, it is already decoded. */
	*newheadp = newhead;
	return PARSE_GOOD;
}

/* The newhead is bad, so let's check if it is something special, otherwise just resync. */
static int wetwork(mpg123_handle *fr, unsigned long *newheadp)
{
	int ret = PARSE_ERR;
	unsigned long newhead = *newheadp;
	*newheadp = 0;

	/* Classic ID3 tags. Read, then start parsing again. */
	if((newhead & 0xffffff00) == ('T'<<24)+('A'<<16)+('G'<<8))
	{
		fr->id3buf[0] = (unsigned char) ((newhead >> 24) & 0xff);
		fr->id3buf[1] = (unsigned char) ((newhead >> 16) & 0xff);
		fr->id3buf[2] = (unsigned char) ((newhead >> 8)  & 0xff);
		fr->id3buf[3] = (unsigned char) ( newhead        & 0xff);

		if((ret=fr->rd->fullread(fr,fr->id3buf+4,124)) < 0) return ret;

		fr->metaflags  |= MPG123_NEW_ID3|MPG123_ID3;
		fr->rdat.flags |= READER_ID3TAG; /* that marks id3v1 */
		if(VERBOSE3) fprintf(stderr,"Note: Skipped ID3v1 tag.\n");

		return PARSE_AGAIN;
	}
	/* This is similar to initial junk skipping code... */
	/* Check for id3v2; first three bytes (of 4) are "ID3" */
	if((newhead & (unsigned long) 0xffffff00) == (unsigned long) 0x49443300)
	{
		return handle_id3v2(fr, newhead);
	}
	else if(NOQUIET && fr->silent_resync == 0)
	{
		fprintf(stderr,"Note: Illegal Audio-MPEG-Header 0x%08lx at offset %"OFF_P".\n",
			newhead, (off_p)fr->rd->tell(fr)-4);
	}

	/* Now we got something bad at hand, try to recover. */

	if(NOQUIET && (newhead & 0xffffff00) == ('b'<<24)+('m'<<16)+('p'<<8)) fprintf(stderr,"Note: Could be a BMP album art.\n");

	if( !(fr->p.flags & MPG123_NO_RESYNC) )
	{
		long try = 0;
		long limit = fr->p.resync_limit;
		unsigned int forgetcount = 0;

		/* If a resync is needed the bitreservoir of previous frames is no longer valid */
		fr->bitreservoir = 0;

		if(NOQUIET && fr->silent_resync == 0) fprintf(stderr, "Note: Trying to resync...\n");

		do /* ... shift the header with additional single bytes until be found something that could be a header. */
		{
			++try;
			if(limit >= 0 && try >= limit) break;				

			if(++forgetcount > FORGET_INTERVAL) forgetcount = 0;
			if((ret=forget_head_shift(fr,&newhead,!forgetcount)) <= 0)
			{
				*newheadp = newhead;
				if(NOQUIET) fprintf (stderr, "Note: Hit end of (available) data during resync.\n");

				return ret ? ret : PARSE_END;
			}
			if(VERBOSE3) debug3("resync try %li at %"OFF_P", got newhead 0x%08lx", try, (off_p)fr->rd->tell(fr),  newhead);
		} while(!head_check(newhead));

		*newheadp = newhead;
		if(NOQUIET && fr->silent_resync == 0) fprintf (stderr, "Note: Skipped %li bytes in input.\n", try);

		/* Now we either got something that could be a header, or we gave up. */
		if(limit >= 0 && try >= limit)
		{
			if(NOQUIET)
			error1("Giving up resync after %li bytes - your stream is not nice... (maybe increasing resync limit could help).", try);

			fr->err = MPG123_RESYNC_FAIL;
			return PARSE_ERR;
		}
		else
		{
			debug1("Found possibly valid header 0x%lx... unsetting oldhead to reinit stream.", newhead);
			fr->oldhead = 0;
			return PARSE_RESYNC;
		}
	}
	else
	{
		if(NOQUIET) error("not attempting to resync...");

		fr->err = MPG123_OUT_OF_SYNC;
		return PARSE_ERR;
	}
	/* Control never goes here... we return before that. */
}
