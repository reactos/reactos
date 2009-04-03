/*
 * Mpeg Layer-1 audio decoder
 * --------------------------
 * copyright (c) 1995 by Michael Hipp, All rights reserved. See also 'README'
 * This file has been copied from mpglib.
 * near unoptimzed ...
 *
 * may have a few bugs after last optimization ...
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

#include "mpg123.h"

static void I_step_one(unsigned int balloc[], unsigned int scale_index[2][SBLIMIT],struct frame *fr)
{
  unsigned int *ba=balloc;
  unsigned int *sca = (unsigned int *) scale_index;

  if(fr->stereo) {
    int i;
    int jsbound = fr->jsbound;
    for (i=0;i<jsbound;i++) {
      *ba++ = getbits(4);
      *ba++ = getbits(4);
    }
    for (i=jsbound;i<SBLIMIT;i++)
      *ba++ = getbits(4);

    ba = balloc;

    for (i=0;i<jsbound;i++) {
      if ((*ba++))
        *sca++ = getbits(6);
      if ((*ba++))
        *sca++ = getbits(6);
    }
    for (i=jsbound;i<SBLIMIT;i++)
      if ((*ba++)) {
        *sca++ =  getbits(6);
        *sca++ =  getbits(6);
      }
  }
  else {
    int i;
    for (i=0;i<SBLIMIT;i++)
      *ba++ = getbits(4);
    ba = balloc;
    for (i=0;i<SBLIMIT;i++)
      if ((*ba++))
        *sca++ = getbits(6);
  }
}

static void I_step_two(real fraction[2][SBLIMIT],unsigned int balloc[2*SBLIMIT],
                       unsigned int scale_index[2][SBLIMIT],struct frame *fr)
{
  int i,n;
  int smpb[2*SBLIMIT]; /* values: 0-65535 */
  int *sample;
  register unsigned int *ba;
  register unsigned int *sca = (unsigned int *) scale_index;

  if(fr->stereo) {
    int jsbound = fr->jsbound;
    register real *f0 = fraction[0];
    register real *f1 = fraction[1];
    ba = balloc;
    for (sample=smpb,i=0;i<jsbound;i++)  {
      if ((n = *ba++))
        *sample++ = getbits(n+1);
      if ((n = *ba++))
        *sample++ = getbits(n+1);
    }
    for (i=jsbound;i<SBLIMIT;i++)
      if ((n = *ba++))
        *sample++ = getbits(n+1);

    ba = balloc;
    for (sample=smpb,i=0;i<jsbound;i++) {
      if((n=*ba++))
        *f0++ = (real) ( ((-1)<<n) + (*sample++) + 1) * muls[n+1][*sca++];
      else
        *f0++ = 0.0;
      if((n=*ba++))
        *f1++ = (real) ( ((-1)<<n) + (*sample++) + 1) * muls[n+1][*sca++];
      else
        *f1++ = 0.0;
    }
    for (i=jsbound;i<SBLIMIT;i++) {
      if ((n=*ba++)) {
        real samp = ( ((-1)<<n) + (*sample++) + 1);
        *f0++ = samp * muls[n+1][*sca++];
        *f1++ = samp * muls[n+1][*sca++];
      }
      else
        *f0++ = *f1++ = 0.0;
    }
  }
  else {
    register real *f0 = fraction[0];
    ba = balloc;
    for (sample=smpb,i=0;i<SBLIMIT;i++)
      if ((n = *ba++))
        *sample++ = getbits(n+1);
    ba = balloc;
    for (sample=smpb,i=0;i<SBLIMIT;i++) {
      if((n=*ba++))
        *f0++ = (real) ( ((-1)<<n) + (*sample++) + 1) * muls[n+1][*sca++];
      else
        *f0++ = 0.0;
    }
  }
}

int do_layer1(struct frame *fr,unsigned char *pcm_sample,int *pcm_point)
{
  int clip=0;
  int i,stereo = fr->stereo;
  unsigned int balloc[2*SBLIMIT];
  unsigned int scale_index[2][SBLIMIT];
  real fraction[2][SBLIMIT];
  int single = fr->single;

  fr->jsbound = (fr->mode == MPG_MD_JOINT_STEREO) ? (fr->mode_ext<<2)+4 : 32;

  if(stereo == 1 || single == 3)
    single = 0;

  I_step_one(balloc,scale_index,fr);

  for (i=0;i<SCALE_BLOCK;i++)
  {
    I_step_two(fraction,balloc,scale_index,fr);

      if(single >= 0) {
        clip += synth_1to1_mono(fr->mp,fraction[single],pcm_sample,pcm_point);
      }
      else {
        int p1 = *pcm_point;
        clip += synth_1to1(fr->mp,fraction[0],0,pcm_sample,&p1);
        clip += synth_1to1(fr->mp,fraction[1],1,pcm_sample,pcm_point);
      }
  }

  return clip;
}
