/*
 * Copyright (c) Michael Hipp and other authors of the mpglib project.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include        <stdio.h>
#include        <string.h>
#include        <signal.h>

#ifndef WIN32
#include        <sys/signal.h>
#include        <unistd.h>
#endif

#include        <math.h>

#ifdef _WIN32
# undef WIN32
# define WIN32

# define M_PI       3.14159265358979323846
# define M_SQRT2	1.41421356237309504880
# define REAL_IS_FLOAT

#endif

#ifdef REAL_IS_FLOAT
#  define real float
#elif defined(REAL_IS_LONG_DOUBLE)
#  define real long double
#else
#  define real double
#endif

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif

#ifndef FALSE
#define         FALSE                   0
#endif
#ifndef TRUE
#define         TRUE                    1
#endif

#define         SBLIMIT                 32
#define         SSLIMIT                 18

#define         SCALE_BLOCK             12


#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

#define MAXFRAMESIZE 1792


struct frame {
    int stereo;
    int jsbound;
    int single;
    int lsf;
    int mpeg25;
    int header_change;
    int lay;
    int error_protection;
    int bitrate_index;
    int sampling_frequency;
    int padding;
    int extension;
    int mode;
    int mode_ext;
    int copyright;
    int original;
    int emphasis;
    int framesize; /* computed framesize */

    /* layer2 stuff */
    int II_sblimit;
    const struct al_table *alloc;

    struct mpstr *mp;
};

struct parameter {
	int quiet;	/* shut up! */
	int tryresync;  /* resync stream after error */
	int verbose;    /* verbose level */
	int checkrange;
};

extern unsigned int   get1bit(void);
extern unsigned int   getbits(int);
extern unsigned int   getbits_fast(int);
extern int set_pointer(struct mpstr *,long);

extern unsigned char *wordpointer;
extern int bitindex;

extern void make_decode_tables(long scaleval);
extern int do_layer3(struct frame *fr,unsigned char *,int *);
extern int do_layer2(struct frame *fr,unsigned char *,int *);
extern int do_layer1(struct frame *fr,unsigned char *,int *);
extern int decode_header(struct frame *fr,unsigned long newhead);



struct gr_info_s {
      int scfsi;
      unsigned part2_3_length;
      unsigned big_values;
      unsigned scalefac_compress;
      unsigned block_type;
      unsigned mixed_block_flag;
      unsigned table_select[3];
      unsigned subblock_gain[3];
      unsigned maxband[3];
      unsigned maxbandl;
      unsigned maxb;
      unsigned region1start;
      unsigned region2start;
      unsigned preflag;
      unsigned scalefac_scale;
      unsigned count1table_select;
      real *full_gain[3];
      real *pow2gain;
};

struct III_sideinfo
{
  unsigned main_data_begin;
  unsigned private_bits;
  struct {
    struct gr_info_s gr[2];
  } ch[2];
};

extern int synth_1to1 (struct mpstr *,real *,int,unsigned char *,int *);
extern int synth_1to1_mono (struct mpstr *,real *,unsigned char *,int *);

extern void init_layer3(int);
extern void init_layer2(void);
extern void make_decode_tables(long scale);
extern void dct64(real *,real *,real *);

extern unsigned char *conv16to8;
extern real muls[27][64];
extern real decwin[512+32];
extern real *pnts[5];
