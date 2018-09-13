/*
 * jccolor.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains input colorspace conversion routines.
 */
#pragma warning( disable : 4799 )
#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

#ifdef NIFTY

#include <math.h>

#define SCALE_PREC      5
#define SCALE_RND       (1 << (SCALE_PREC - 1))
#define SCALE           (1 << SCALE_PREC)
#define unscale(x)      (((long)(x) + SCALE_RND) >> SCALE_PREC)
#define clip(x)         (((long)(x) & ~0xff) ? (((long)(x) < 0) ? 0 : 255) : (long)(x))

#endif


/* Private subobject */

typedef struct {
  struct jpeg_color_converter pub; /* public fields */

  /* Private state for RGB->YCC conversion */
  INT32 * rgb_ycc_tab;		/* => table for RGB to YCbCr conversion */
} my_color_converter;

typedef my_color_converter * my_cconvert_ptr;

extern void MRGB2YCbCr(
int rows,
int cols,
unsigned char *inRGB,
unsigned char *outY,
unsigned char *outU,
unsigned char *outV);

extern void MRGBA2YCbCrA(
int rows,
int cols,
unsigned char *inRGB,
unsigned char *outY,
unsigned char *outU,
unsigned char *outV,
unsigned char *outA);

extern void MRGBA2YCbCrALegacy(
int rows,
int cols,
unsigned char *inRGB,
unsigned char *outY,
unsigned char *outU,
unsigned char *outV,
unsigned char *outA);

// ******************************************************************
// Macros and Constants
#define INT32 long int
#define FCONVERSION_BITS 15
#define ICONVERSION_BITS  8

const __int64  const_0			= 0x0000000000000000;
const __int64  const_1			= 0x0001000100010001;
const __int64  const_128		= 0x0080008000800080;
// These constants correspond to CCIR 601-1
// Y  = [ (9798*R + 19235*G +  3736*B) / 32768]
// Cb = [(-5529*R - 10855*G + 16384*B) / 32768] + 128
// Cr = [(16384*R - 13720*G -  2664*B) / 32768] + 128
//Conventional floating point equations:
// Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B
// Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B  + 0.5
// Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B  + 0.5
//Yr = 2646 Yg = 4b23 Yb = 0e98
//Ur = ea67 Ug = d599 Ub = 4000
//Vr = 4000 Vg = ca68 Vb = f598
// constants for RGB->YCrCb
const __int64  const_YR0GR		= 0x264600004B232646;
const __int64  const_YBG0B		= 0x0E984B2300000E98;
const __int64  const_UR0GR		= 0xEA670000D599EA67;
const __int64  const_UBG0B		= 0x4000D59900004000;
const __int64  const_VR0GR		= 0x40000000CA684000;
const __int64  const_VBG0B		= 0xF598CA680000F598;

// constants for RGBA->YCrCbA
const __int64  const2_YGRGR		= 0x4B2326464B232646;
const __int64  const2_Y0B0B		= 0x00000E9800000E98;
const __int64  const2_UGRGR		= 0xD599EA67D599EA67;
const __int64  const2_U0B0B		= 0x0000400000004000;
const __int64  const2_VGRGR		= 0xCA684000CA684000;
const __int64  const2_V0B0B		= 0x0000F5980000F598;
const __int64  const2_A			= 0x0001000000010000;
const __int64  const2_Legacy	= 0x00FFFFFF00FFFFFF;




// These constants correspond to the original FPX SDK
// ... using 2^15
//Y  = [ (9869*R + 19738*G +  3290*B) / 32768]
//Cb = [(-4935*R -  9869*G + 14739*B) / 32768] + 128
//Cr = [(14312*R - 12336*G -  2056*B) / 32768] + 128
//Conventional floating point equations:
// Y  =  0.30118*R + 0.60235*G + 0.10039*B
// Cb = -0.15059*R - 0.30118*G + 0.44981*B + 0.5
// Cr =  0.43676*R - 0.37647*G - 0.06274*G + 0.5
//Yr = 268d Yg = 4d1a Yb = 0cda
//Ur = ecb9 Ug = d973 Ub = 3993
//Vr = 37e8 Vg = cfd0 Vb = f7f8
// constants for RGB->YCrCb
//const __int64  const_YR0GR		= 0x268D00004D1A268D;
//const __int64  const_YBG0B		= 0x0CDA4D1A00000CDA;
//const __int64  const_UR0GR		= 0xECB90000D973ECB9;
//const __int64  const_UBG0B		= 0x3993D97300003993;
//const __int64  const_VR0GR		= 0x37E80000CFD037E8;
//const __int64  const_VBG0B		= 0xF7F8CFD00000F7F8;

// constants for RGBA->YCrCbA
//const __int64  const2_YGRGR		= 0x4D1A268D4D1A268D;
//const __int64  const2_Y0B0B		= 0x00000CDA00000CDA;
//const __int64  const2_UGRGR		= 0xD973ECB9D973ECB9;
//const __int64  const2_U0B0B		= 0x0000399300003993;
//const __int64  const2_VGRGR		= 0xCFD037E8CFD037E8;
//const __int64  const2_V0B0B		= 0x0000F7F80000F7F8;
//const __int64  const2_A			= 0x0001000000010000;
//const __int64  const2_Legacy	= 0x00FFFFFF00FFFFFF;

// ... using 2^8
//const __int64  const_X0YY0		= 0x0000010001000000;
//const __int64  const_RVUVU		= 0x019A0000019A0000;
//const __int64  const_GVUVU		= 0xFF33FFABFF33FFAB;
//const __int64  const_BVUVU		= 0x0000020000000200;
__int64  temp0, tempY, tempU, tempV, tempA;


/**************** RGB -> YCbCr conversion: most common case **************/

/*
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 *	Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B
 *	Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B  + CENTERJSAMPLE
 *	Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B  + CENTERJSAMPLE
 * (These numbers are derived from TIFF 6.0 section 21, dated 3-June-92.)
 * Note: older versions of the IJG code used a zero offset of MAXJSAMPLE/2,
 * rather than CENTERJSAMPLE, for Cb and Cr.  This gave equal positive and
 * negative swings for Cb/Cr, but meant that grayscale values (Cb=Cr=0)
 * were not represented exactly.  Now we sacrifice exact representation of
 * maximum red and maximum blue in order to get exact grayscales.
 *
 * To avoid floating-point arithmetic, we represent the fractional constants
 * as integers scaled up by 2^16 (about 4 digits precision); we have to divide
 * the products by 2^16, with appropriate rounding, to get the correct answer.
 *
 * For even more speed, we avoid doing any multiplications in the inner loop
 * by precalculating the constants times R,G,B for all possible values.
 * For 8-bit JSAMPLEs this is very reasonable (only 256 entries per table);
 * for 12-bit samples it is still acceptable.  It's not very reasonable for
 * 16-bit samples, but if you want lossless storage you shouldn't be changing
 * colorspace anyway.
 * The CENTERJSAMPLE offsets and the rounding fudge-factor of 0.5 are included
 * in the tables to save adding them separately in the inner loop.
 */

#define SCALEBITS	16	/* speediest right-shift on some machines */
#define CBCR_OFFSET	((INT32) CENTERJSAMPLE << SCALEBITS)
#define ONE_HALF	((INT32) 1 << (SCALEBITS-1))
#define FIX(x)		((INT32) ((x) * (1L<<SCALEBITS) + 0.5))

/* We allocate one big table and divide it up into eight parts, instead of
 * doing eight alloc_small requests.  This lets us use a single table base
 * address, which can be held in a register in the inner loops on many
 * machines (more than can hold all eight addresses, anyway).
 */

#define R_Y_OFF		0			/* offset to R => Y section */
#define G_Y_OFF		(1*(MAXJSAMPLE+1))	/* offset to G => Y section */
#define B_Y_OFF		(2*(MAXJSAMPLE+1))	/* etc. */
#define R_CB_OFF	(3*(MAXJSAMPLE+1))
#define G_CB_OFF	(4*(MAXJSAMPLE+1))
#define B_CB_OFF	(5*(MAXJSAMPLE+1))
#define R_CR_OFF	B_CB_OFF		/* B=>Cb, R=>Cr are the same */
#define G_CR_OFF	(6*(MAXJSAMPLE+1))
#define B_CR_OFF	(7*(MAXJSAMPLE+1))
#define TABLE_SIZE	(8*(MAXJSAMPLE+1))

#ifdef NIFTY
/*
 * Initialize for RGB->PhotoYCC colorspace conversion.
 */
METHODDEF (void)
rgb_pycc_start (j_compress_ptr cinfo)
{
 
}

/*
 * RGB->PhotoYCC colorspace convertion.
 */
METHODDEF (void)
rgb_pycc_convert (j_compress_ptr cinfo,
                 JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
                 JDIMENSION output_row, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr)cinfo->cconvert;
  register JSAMPROW inptr;
  register JSAMPROW outptr0, outptr1, outptr2;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->image_width;
  unsigned char r, g, b;
 
  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr0 = output_buf[0][output_row];
    outptr1 = output_buf[1][output_row];
    outptr2 = output_buf[2][output_row];
    output_row++;
    for (col = 0; col < num_cols; col++) {
      r = GETJSAMPLE(inptr[RGB_RED]);
      g = GETJSAMPLE(inptr[RGB_GREEN]);
      b = GETJSAMPLE(inptr[RGB_BLUE]);
      inptr+=RGB_PIXELSIZE;
 
      /* Y */
      outptr0[col] = (JSAMPLE)((float)((float)r * 0.2200179046) + (float)((float)g * 0.4322754970) + (float)((float)b * 0.0838667868));
      /* C1 */
      outptr1[col] = (JSAMPLE)((float)((float)r * -0.1347546425) - (float)((float)g * 0.2647563169) + (float)((float)b * 0.3995109594) + 156);
      /* C2 */
      outptr2[col] = (JSAMPLE)((float)((float)r * 0.3849177482) - (float)((float)g * 0.3223733380) + (float)((float)b * 0.0625444102) + 137);
    }
  }
}

#endif

/*
 * Initialize for RGB->YCC colorspace conversion.
 */

METHODDEF(void)
rgb_ycc_start (j_compress_ptr cinfo)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  INT32 * rgb_ycc_tab;
  INT32 i;

  /* Allocate and fill in the conversion tables. */
  cconvert->rgb_ycc_tab = rgb_ycc_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(TABLE_SIZE * SIZEOF(INT32)));

  for (i = 0; i <= MAXJSAMPLE; i++) {
    rgb_ycc_tab[i+R_Y_OFF] = FIX(0.29900) * i;
    rgb_ycc_tab[i+G_Y_OFF] = FIX(0.58700) * i;
    rgb_ycc_tab[i+B_Y_OFF] = FIX(0.11400) * i     + ONE_HALF;
    rgb_ycc_tab[i+R_CB_OFF] = (-FIX(0.16874)) * i;
    rgb_ycc_tab[i+G_CB_OFF] = (-FIX(0.33126)) * i;
    /* We use a rounding fudge-factor of 0.5-epsilon for Cb and Cr.
     * This ensures that the maximum output will round to MAXJSAMPLE
     * not MAXJSAMPLE+1, and thus that we don't have to range-limit.
     */
    rgb_ycc_tab[i+B_CB_OFF] = FIX(0.50000) * i    + CBCR_OFFSET + ONE_HALF-1;
/*  B=>Cb and R=>Cr tables are the same
    rgb_ycc_tab[i+R_CR_OFF] = FIX(0.50000) * i    + CBCR_OFFSET + ONE_HALF-1;
*/
    rgb_ycc_tab[i+G_CR_OFF] = (-FIX(0.41869)) * i;
    rgb_ycc_tab[i+B_CR_OFF] = (-FIX(0.08131)) * i;
  }
}


/*
 * Convert some rows of samples to the JPEG colorspace.
 *
 * Note that we change from the application's interleaved-pixel format
 * to our internal noninterleaved, one-plane-per-component format.
 * The input buffer is therefore three times as wide as the output buffer.
 *
 * A starting row offset is provided only for the output buffer.  The caller
 * can easily adjust the passed input_buf value to accommodate any row
 * offset required on that side.
 */


METHODDEF(void)
rgb_ycc_convert (j_compress_ptr cinfo,
		 JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
		 JDIMENSION output_row, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int r, g, b;
  register INT32 * ctab = cconvert->rgb_ycc_tab;
  register JSAMPROW inptr;
  register JSAMPROW outptr0, outptr1, outptr2;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->image_width;
  JDIMENSION tail_cols = num_cols&7;
  JDIMENSION mmx_cols=num_cols&~7;

  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr0 = output_buf[0][output_row];
    outptr1 = output_buf[1][output_row];
    outptr2 = output_buf[2][output_row];
    output_row++;

//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)
	if (vfMMXMachine)
	{

		MRGB2YCbCr(	(int)(1), mmx_cols, inptr, outptr0, outptr1, outptr2);

		inptr += 3*mmx_cols;
		for (col = mmx_cols; col < num_cols; col++) {
		  r = GETJSAMPLE(inptr[RGB_RED]);
		  g = GETJSAMPLE(inptr[RGB_GREEN]);
		  b = GETJSAMPLE(inptr[RGB_BLUE]);
	      inptr += RGB_PIXELSIZE;
	      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
	       * must be too; we do not need an explicit range-limiting operation.
	       * Hence the value being shifted is never negative, and we don't
	       * need the general RIGHT_SHIFT macro.
	       */
	      /* Y */
	      outptr0[col] = (JSAMPLE)
			((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
			 >> SCALEBITS);
	      /* Cb */
	      outptr1[col] = (JSAMPLE)
			((ctab[r+R_CB_OFF] + ctab[g+G_CB_OFF] + ctab[b+B_CB_OFF])
			 >> SCALEBITS);
	      /* Cr */
	      outptr2[col] = (JSAMPLE)
			((ctab[r+R_CR_OFF] + ctab[g+G_CR_OFF] + ctab[b+B_CR_OFF])
			 >> SCALEBITS);
		}
	}
	else
#endif
	{
	    for (col = 0; col < num_cols; col++) {
	      r = GETJSAMPLE(inptr[RGB_RED]);
	      g = GETJSAMPLE(inptr[RGB_GREEN]);
	      b = GETJSAMPLE(inptr[RGB_BLUE]);
	      inptr += RGB_PIXELSIZE;
	      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
	       * must be too; we do not need an explicit range-limiting operation.
	       * Hence the value being shifted is never negative, and we don't
	       * need the general RIGHT_SHIFT macro.
	       */
	      /* Y */
	      outptr0[col] = (JSAMPLE)
			((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
			 >> SCALEBITS);
	      /* Cb */
	      outptr1[col] = (JSAMPLE)
			((ctab[r+R_CB_OFF] + ctab[g+G_CB_OFF] + ctab[b+B_CB_OFF])
			 >> SCALEBITS);
	      /* Cr */
	      outptr2[col] = (JSAMPLE)
			((ctab[r+R_CR_OFF] + ctab[g+G_CR_OFF] + ctab[b+B_CR_OFF])
			 >> SCALEBITS);
	    }
	}
  }
}




/**************** Cases other than RGB -> YCbCr **************/


/*
 * Convert some rows of samples to the JPEG colorspace.
 * This version handles RGB->grayscale conversion, which is the same
 * as the RGB->Y portion of RGB->YCbCr.
 * We assume rgb_ycc_start has been called (we only use the Y tables).
 */

METHODDEF(void)
rgb_gray_convert (j_compress_ptr cinfo,
		  JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
		  JDIMENSION output_row, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int r, g, b;
  register INT32 * ctab = cconvert->rgb_ycc_tab;
  register JSAMPROW inptr;
  register JSAMPROW outptr;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->image_width;

  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr = output_buf[0][output_row];
    output_row++;
    for (col = 0; col < num_cols; col++) {
      r = GETJSAMPLE(inptr[RGB_RED]);
      g = GETJSAMPLE(inptr[RGB_GREEN]);
      b = GETJSAMPLE(inptr[RGB_BLUE]);
      inptr += RGB_PIXELSIZE;
      /* Y */
      outptr[col] = (JSAMPLE)
		((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
		 >> SCALEBITS);
    }
  }
}

#ifdef NIFTY

METHODDEF (void)
rgba_ycbcra_convert (j_compress_ptr cinfo,
		   JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
		   JDIMENSION output_row, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int r, g, b;
  register INT32 * ctab = cconvert->rgb_ycc_tab;
  register JSAMPROW inptr;
  register JSAMPROW outptr0, outptr1, outptr2, outptr3;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->image_width;
  JDIMENSION tail_cols = num_cols&7;
  JDIMENSION mmx_cols=num_cols&~7;
 
  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr0 = output_buf[0][output_row];
    outptr1 = output_buf[1][output_row];
	outptr2 = output_buf[2][output_row];
    outptr3 = output_buf[3][output_row];
    output_row++;

//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)
        if (vfMMXMachine)
	{

		MRGBA2YCbCrA(	(int)(1), mmx_cols, inptr, outptr0, outptr1, outptr2, outptr3);

		inptr += 4*mmx_cols;
	    for (col = mmx_cols; col < num_cols; col++) {
	      r = GETJSAMPLE(inptr[0]);
	      g = GETJSAMPLE(inptr[1]);
	      b = GETJSAMPLE(inptr[2]);
	      /* Alpha passes through as-is */
	      outptr3[col] = inptr[3];  /* don't need GETJSAMPLE here */
	      inptr += 4;
	      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
	       * must be too; we do not need an explicit range-limiting operation.
	       * Hence the value being shifted is never negative, and we don't
	       * need the general RIGHT_SHIFT macro.
	       */
	      /* Y */
	      outptr0[col] = (JSAMPLE)
	                ((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
		             >> SCALEBITS);
	      /* Cb */
	      outptr1[col] = (JSAMPLE)
	                ((ctab[r+R_CB_OFF] + ctab[g+G_CB_OFF] + ctab[b+B_CB_OFF])
	                 >> SCALEBITS);
	      /* Cr */
	      outptr2[col] = (JSAMPLE)
	                ((ctab[r+R_CR_OFF] + ctab[g+G_CR_OFF] + ctab[b+B_CR_OFF])
	                 >> SCALEBITS);
	    }
	}
	else
 #endif // defined (_X86_)
        {

	    for (col = 0; col < num_cols; col++) {
	      r = GETJSAMPLE(inptr[0]);
	      g = GETJSAMPLE(inptr[1]);
	      b = GETJSAMPLE(inptr[2]);
	      /* Alpha passes through as-is */
	      outptr3[col] = inptr[3];  /* don't need GETJSAMPLE here */
	      inptr += 4;
	      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
	       * must be too; we do not need an explicit range-limiting operation.
	       * Hence the value being shifted is never negative, and we don't
	       * need the general RIGHT_SHIFT macro.
	       */
	      /* Y */
	      outptr0[col] = (JSAMPLE)
	                ((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
		             >> SCALEBITS);
	      /* Cb */
	      outptr1[col] = (JSAMPLE)
	                ((ctab[r+R_CB_OFF] + ctab[g+G_CB_OFF] + ctab[b+B_CB_OFF])
	                 >> SCALEBITS);
	      /* Cr */
	      outptr2[col] = (JSAMPLE)
	                ((ctab[r+R_CR_OFF] + ctab[g+G_CR_OFF] + ctab[b+B_CR_OFF])
	                 >> SCALEBITS);
	    }
	}

  }
}


METHODDEF (void)
rgba_ycbcralegacy_convert (j_compress_ptr cinfo,
		   JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
		   JDIMENSION output_row, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int r, g, b;
  register INT32 * ctab = cconvert->rgb_ycc_tab;
  register JSAMPROW inptr;
  register JSAMPROW outptr0, outptr1, outptr2, outptr3;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->image_width;
  JDIMENSION tail_cols = num_cols&7;
  JDIMENSION mmx_cols=num_cols&~7;
 
  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr0 = output_buf[0][output_row];
    outptr1 = output_buf[1][output_row];
    outptr2 = output_buf[2][output_row];
    outptr3 = output_buf[3][output_row];
    output_row++;

//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)
	if (vfMMXMachine)
	{

		MRGBA2YCbCrALegacy(	(int)(1), mmx_cols, inptr, outptr0, outptr1, outptr2, outptr3);

		inptr += 4*mmx_cols;
	    for (col = mmx_cols; col < num_cols; col++) {
	      r = MAXJSAMPLE - GETJSAMPLE(inptr[0]);
	      g = MAXJSAMPLE - GETJSAMPLE(inptr[1]);
	      b = MAXJSAMPLE - GETJSAMPLE(inptr[2]);
	      /* Alpha passes through as-is */
	      outptr3[col] = inptr[3];  /* don't need GETJSAMPLE here */
	      inptr += 4;
	      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
	       * must be too; we do not need an explicit range-limiting operation.
	       * Hence the value being shifted is never negative, and we don't
	       * need the general RIGHT_SHIFT macro.
	       */
	      /* Y */
	      outptr0[col] = (JSAMPLE)
	                ((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
	                 >> SCALEBITS);
	      /* Cb */
	      outptr1[col] = (JSAMPLE)
	                ((ctab[r+R_CB_OFF] + ctab[g+G_CB_OFF] + ctab[b+B_CB_OFF])
	                 >> SCALEBITS);
	      /* Cr */
	      outptr2[col] = (JSAMPLE)
	                ((ctab[r+R_CR_OFF] + ctab[g+G_CR_OFF] + ctab[b+B_CR_OFF])
	                 >> SCALEBITS);
	    }
	}
	else
#endif // defined (_X86_)
        {
	    for (col = 0; col < num_cols; col++) {
	      r = MAXJSAMPLE - GETJSAMPLE(inptr[0]);
	      g = MAXJSAMPLE - GETJSAMPLE(inptr[1]);
	      b = MAXJSAMPLE - GETJSAMPLE(inptr[2]);
	      /* Alpha passes through as-is */
	      outptr3[col] = inptr[3];  /* don't need GETJSAMPLE here */
	      inptr += 4;
	      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
	       * must be too; we do not need an explicit range-limiting operation.
	       * Hence the value being shifted is never negative, and we don't
	       * need the general RIGHT_SHIFT macro.
	       */
	      /* Y */
	      outptr0[col] = (JSAMPLE)
	                ((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
	                 >> SCALEBITS);
	      /* Cb */
	      outptr1[col] = (JSAMPLE)
	                ((ctab[r+R_CB_OFF] + ctab[g+G_CB_OFF] + ctab[b+B_CB_OFF])
	                 >> SCALEBITS);
	      /* Cr */
	      outptr2[col] = (JSAMPLE)
	                ((ctab[r+R_CR_OFF] + ctab[g+G_CR_OFF] + ctab[b+B_CR_OFF])
	                 >> SCALEBITS);
	    }
	}
  }
}



#endif

/*
 * Convert some rows of samples to the JPEG colorspace.
 * This version handles Adobe-style CMYK->YCCK conversion,
 * where we convert R=1-C, G=1-M, and B=1-Y to YCbCr using the same
 * conversion as above, while passing K (black) unchanged.
 * We assume rgb_ycc_start has been called.
 */

METHODDEF(void)
cmyk_ycck_convert (j_compress_ptr cinfo,
		   JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
		   JDIMENSION output_row, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int r, g, b;
  register INT32 * ctab = cconvert->rgb_ycc_tab;
  register JSAMPROW inptr;
  register JSAMPROW outptr0, outptr1, outptr2, outptr3;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->image_width;

  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr0 = output_buf[0][output_row];
    outptr1 = output_buf[1][output_row];
    outptr2 = output_buf[2][output_row];
    outptr3 = output_buf[3][output_row];
    output_row++;
    for (col = 0; col < num_cols; col++) {
      r = MAXJSAMPLE - GETJSAMPLE(inptr[0]);
      g = MAXJSAMPLE - GETJSAMPLE(inptr[1]);
      b = MAXJSAMPLE - GETJSAMPLE(inptr[2]);
      /* K passes through as-is */
      outptr3[col] = inptr[3];	/* don't need GETJSAMPLE here */
      inptr += 4;
      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
       * must be too; we do not need an explicit range-limiting operation.
       * Hence the value being shifted is never negative, and we don't
       * need the general RIGHT_SHIFT macro.
       */
      /* Y */
      outptr0[col] = (JSAMPLE)
		((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
		 >> SCALEBITS);
      /* Cb */
      outptr1[col] = (JSAMPLE)
		((ctab[r+R_CB_OFF] + ctab[g+G_CB_OFF] + ctab[b+B_CB_OFF])
		 >> SCALEBITS);
      /* Cr */
      outptr2[col] = (JSAMPLE)
		((ctab[r+R_CR_OFF] + ctab[g+G_CR_OFF] + ctab[b+B_CR_OFF])
		 >> SCALEBITS);
    }
  }
}


/*
 * Convert some rows of samples to the JPEG colorspace.
 * This version handles grayscale output with no conversion.
 * The source can be either plain grayscale or YCbCr (since Y == gray).
 */

METHODDEF(void)
grayscale_convert (j_compress_ptr cinfo,
		   JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
		   JDIMENSION output_row, int num_rows)
{
  register JSAMPROW inptr;
  register JSAMPROW outptr;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->image_width;
  int instride = cinfo->input_components;

  while (--num_rows >= 0) {
    inptr = *input_buf++;
    outptr = output_buf[0][output_row];
    output_row++;
    for (col = 0; col < num_cols; col++) {
      outptr[col] = inptr[0];	/* don't need GETJSAMPLE() here */
      inptr += instride;
    }
  }
}


/*
 * Convert some rows of samples to the JPEG colorspace.
 * This version handles multi-component colorspaces without conversion.
 * We assume input_components == num_components.
 */

METHODDEF(void)
null_convert (j_compress_ptr cinfo,
	      JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
	      JDIMENSION output_row, int num_rows)
{
  register JSAMPROW inptr;
  register JSAMPROW outptr;
  register JDIMENSION col;
  register int ci;
  int nc = cinfo->num_components;
  JDIMENSION num_cols = cinfo->image_width;

  while (--num_rows >= 0) {
    /* It seems fastest to make a separate pass for each component. */
    for (ci = 0; ci < nc; ci++) {
      inptr = *input_buf;
      outptr = output_buf[ci][output_row];
      for (col = 0; col < num_cols; col++) {
	outptr[col] = inptr[ci]; /* don't need GETJSAMPLE() here */
	inptr += nc;
      }
    }
    input_buf++;
    output_row++;
  }
}


/*
 * Empty method for start_pass.
 */

METHODDEF(void)
null_method (j_compress_ptr cinfo)
{
  /* no work needed */
}


/*
 * Module initialization routine for input colorspace conversion.
 */

GLOBAL(void)
jinit_color_converter (j_compress_ptr cinfo)
{
  my_cconvert_ptr cconvert;

  cconvert = (my_cconvert_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_color_converter));
  cinfo->cconvert = (struct jpeg_color_converter *) cconvert;
  /* set start_pass to null method until we find out differently */
  cconvert->pub.start_pass = null_method;

  /* Make sure input_components agrees with in_color_space */
  switch (cinfo->in_color_space) {
  case JCS_GRAYSCALE:
    if (cinfo->input_components != 1)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;

#ifdef NIFTY
    case JCS_YCC:
    if (cinfo->input_components != 3)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;

    case JCS_RGBA:
    if (cinfo->input_components != 4)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;

    case JCS_YCbCrA:
    if (cinfo->input_components != 4)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;
    
	case JCS_YCbCrALegacy:
    if (cinfo->input_components != 4)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;

    case JCS_YCCA:
    if (cinfo->input_components != 4)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;
#endif

	case JCS_RGB:

#if RGB_PIXELSIZE != 3
    if (cinfo->input_components != RGB_PIXELSIZE)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;
#endif /* else share code with YCbCr */

  case JCS_YCbCr:
    if (cinfo->input_components != 3)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;

  case JCS_CMYK:
  case JCS_YCCK:
    if (cinfo->input_components != 4)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;

  default:			/* JCS_UNKNOWN can be anything */
    if (cinfo->input_components < 1)
      ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
    break;
  }

  /* Check num_components, set conversion method based on requested space */
  switch (cinfo->jpeg_color_space) {
  case JCS_GRAYSCALE:
    if (cinfo->num_components != 1)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_GRAYSCALE)
      cconvert->pub.color_convert = grayscale_convert;
    else if (cinfo->in_color_space == JCS_RGB) {
      cconvert->pub.start_pass = rgb_ycc_start;
      cconvert->pub.color_convert = rgb_gray_convert;
    } else if (cinfo->in_color_space == JCS_YCbCr)
      cconvert->pub.color_convert = grayscale_convert;
    else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;
#ifdef NIFTY
  /* Store and compress data as PhotoYCC */
  /* Only current option is to start with PhotoYCC
   * although I do include the function RGB->PhotoYCC
   * in the compressor, I don't think it's a good idea
   * to rotate to PhotoYCC from RGB in this context.
   * If subsampling is required, then just use YCrCb.
   */
  case JCS_YCC:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_YCC)
      cconvert->pub.color_convert = null_convert;
    else
      if (cinfo->in_color_space == JCS_RGB) {
	/* this is where the RGB->PhotoYCC could be called */
        ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);	
      } else {
        ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
      }
    break;

  case JCS_YCCA:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_YCCA)
      cconvert->pub.color_convert = null_convert;
    else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_RGBA:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_RGBA) {
      cconvert->pub.color_convert = null_convert;
    } else {
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    }
    break;

  case JCS_YCbCrA:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_YCbCrA)
      cconvert->pub.color_convert = null_convert;
    else if (cinfo->in_color_space == JCS_RGBA) {
      cconvert->pub.start_pass = rgb_ycc_start;
      cconvert->pub.color_convert = rgba_ycbcra_convert;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_YCbCrALegacy:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_YCbCrALegacy)
      cconvert->pub.color_convert = null_convert;
    else if (cinfo->in_color_space == JCS_RGBA) {
      cconvert->pub.start_pass = rgb_ycc_start;
      cconvert->pub.color_convert = rgba_ycbcralegacy_convert;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;


#endif
  case JCS_RGB:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_RGB && RGB_PIXELSIZE == 3)
      cconvert->pub.color_convert = null_convert;
    else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_YCbCr:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_RGB) {
      cconvert->pub.start_pass = rgb_ycc_start;
      cconvert->pub.color_convert = rgb_ycc_convert;
    } else if (cinfo->in_color_space == JCS_YCbCr)
      cconvert->pub.color_convert = null_convert;
    else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_CMYK:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_CMYK)
      cconvert->pub.color_convert = null_convert;
    else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_YCCK:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    if (cinfo->in_color_space == JCS_CMYK) {
      cconvert->pub.start_pass = rgb_ycc_start;
      cconvert->pub.color_convert = cmyk_ycck_convert;
    } else if (cinfo->in_color_space == JCS_YCCK)
      cconvert->pub.color_convert = null_convert;
    else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  default:			/* allow null conversion of JCS_UNKNOWN */
    if (cinfo->jpeg_color_space != cinfo->in_color_space ||
	cinfo->num_components != cinfo->input_components)
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    cconvert->pub.color_convert = null_convert;
    break;
  }
}


//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)

void MRGB2YCbCr(
	int rows,
	int cols,
	unsigned char *inRGB,
	unsigned char *outY,
	unsigned char *outU,
	unsigned char *outV)
{
// make global to ensure proper stack alignment
//	__int64  temp0, tempY, tempU, tempV;

	__asm {

		// initializations
//DS - IJG will always call with rows=1, so don't multiply
//		mov			eax, rows
//		mov			ebx, cols
//		mul			ebx							;number pixels
// reorder to take advantage of v-pipe
		mov			esi, cols
		mov			eax, inRGB

		shr			esi, 3						;number of loops = (rows*cols)/8
		mov			edx, outV

		mov			edi, esi					;loop counter in edi
		mov			ecx, outU

		mov			ebx, outY

		// top of loop

	RGBtoYUV:

		movq		mm1, [eax]					;load #1 G2R2B1G1R1B0G0R0 -> mm1
		pxor		mm6, mm6					;0 -> mm6

		movq		mm0, mm1					;G2R2B1G1R1B0G0R0 -> mm0
		psrlq		mm1, 16						;00G2R2B1G1R1B0 -> mm1

		punpcklbw	mm0, const_0				;R1B0G0R0 -> mm0
		movq		mm7, mm1					;00G2R2B1G1R1B0	-> mm7

		punpcklbw	mm1, const_0				;B1G1R1B0 -> mm1
		movq		mm2, mm0					;R1B0G0R0 -> mm2

		pmaddwd		mm0, const_YR0GR			;yrR1,ygG0+yrR0 -> mm0
		movq		mm3, mm1					;B1G1R1B0 -> mm3

		pmaddwd		mm1, const_YBG0B			;ybB1+ygG1,ybB0 -> mm1
		movq		mm4, mm2					;R1B0G0R0 -> mm4

		pmaddwd		mm2, const_UR0GR			;urR1,ugG0+urR0 -> mm2
		movq		mm5, mm3					;B1G1R1B0 -> mm5

		pmaddwd		mm3, const_UBG0B			;ubB1+ugG1,ubB0 -> mm3
		punpckhbw	mm7, mm6					;00G2R2 -> mm7

		pmaddwd		mm4, const_VR0GR			;vrR1,vgG0+vrR0 -> mm4
		paddd		mm0, mm1					;Y1Y0 -> mm0

		pmaddwd		mm5, const_VBG0B			;vbB1+vgG1,vbB0 -> mm5
//		nop

		movq		mm1, [eax][8]				;load #2 R5B4G4R4B3G3R3B2 -> mm1
		paddd		mm2, mm3					;U1U0 -> mm2

		movq		mm6, mm1					;R5B4G4R4B3G3R3B2 -> mm6
//		nop

		punpcklbw	mm1, const_0				;B3G3R3B2 -> mm1
		paddd		mm4, mm5					;V1V0 -> mm4

		movq		mm5, mm1					;B3G3R3B2 -> mm5
 		psllq		mm1, 32						;R3B200 -> mm1

		paddd		mm1, mm7					;R3B200 + 00G2R2 = R3B2G2R2 -> mm1
//		nop

		punpckhbw	mm6, const_0				;R5B4G4R4 -> mm6
		movq		mm3, mm1					;R3B2G2R2 -> mm3

		pmaddwd		mm1, const_YR0GR			;yrR3,ygG2+yrR2 -> mm1
		movq		mm7, mm5					;B3G3R3B2 -> mm7

		pmaddwd		mm5, const_YBG0B			;ybB3+ygG3,ybB2 -> mm5
		psrad		mm0, FCONVERSION_BITS		;32-bit scaled Y1Y0 -> mm0

		movq		temp0, mm6					;R5B4G4R4 -> temp0
		movq		mm6, mm3					;R3B2G2R2 -> mm6

		pmaddwd		mm6, const_UR0GR			;urR3,ugG2+urR2 -> mm6
		psrad		mm2, FCONVERSION_BITS		;32-bit scaled U1U0 -> mm2

		paddd		mm1, mm5					;Y3Y2 -> mm1
		movq		mm5, mm7					;B3G3R3B2 -> mm5

		pmaddwd		mm7, const_UBG0B			;ubB3+ugG3,ubB2
		psrad		mm1, FCONVERSION_BITS		;32-bit scaled Y3Y2 -> mm1

		pmaddwd		mm3, const_VR0GR			;vrR3,vgG2+vgR2
		packssdw	mm0, mm1					;Y3Y2Y1Y0 -> mm0

		pmaddwd		mm5, const_VBG0B			;vbB3+vgG3,vbB2 -> mm5
		psrad		mm4, FCONVERSION_BITS		;32-bit scaled V1V0 -> mm4	

		movq		mm1, [eax][16]				;load #3 B7G7R7B6G6R6B5G5 -> mm7
		paddd		mm6, mm7					;U3U2 -> mm6

		movq		mm7, mm1					;B7G7R7B6G6R6B5G5 -> mm1
		psrad		mm6, FCONVERSION_BITS		;32-bit scaled U3U2 -> mm6

		paddd		mm3, mm5					;V3V2 -> mm3
		psllq		mm7, 16						;R7B6G6R6B5G500 -> mm7

		movq		mm5, mm7					;R7B6G6R6B5G500 -> mm5
		psrad		mm3, FCONVERSION_BITS		;32-bit scaled V3V2 -> mm3

		movq		tempY, mm0					;32-bit scaled Y3Y2Y1Y0 -> tempY
		packssdw	mm2, mm6					;32-bit scaled U3U2U1U0 -> mm2

		movq		mm0, temp0					;R5B4G4R4 -> mm0
//		nop 

		punpcklbw	mm7, const_0				;B5G500 -> mm7
		movq		mm6, mm0					;R5B4G4R4 -> mm6

		movq		tempU, mm2					;32-bit scaled U3U2U1U0	-> tempU
		psrlq		mm0, 32						;00R5B4 -> mm0

		paddw		mm7, mm0					;B5G5R5B4 -> mm7
		movq		mm2, mm6					;B5B4G4R4 -> mm2

		pmaddwd		mm2, const_YR0GR			;yrR5,ygG4+yrR4 -> mm2
		movq		mm0, mm7					;B5G5R5B4 -> mm0

		pmaddwd		mm7, const_YBG0B			;ybB5+ygG5,ybB4 -> mm7
		packssdw	mm4, mm3					;32-bit scaled V3V2V1V0 -> mm4

		add			eax, 24						;increment RGB count
//		nop										;//JS

		movq		tempV, mm4					;32-bit scaled V3V2V1V0 -> tempV
		movq		mm4, mm6					;B5B4G4R4 -> mm4

		pmaddwd		mm6, const_UR0GR			;urR5,ugG4+urR4
		movq		mm3, mm0					;B5G5R5B4 -> mm0

		pmaddwd		mm0, const_UBG0B			;ubB5+ugG5,ubB4
		paddd		mm2, mm7					;Y5Y4 -> mm2

		pmaddwd		mm4, const_VR0GR			;vrR5,vgG4+vrR4 -> mm4
		pxor		mm7, mm7					;0 -> mm7

		pmaddwd		mm3, const_VBG0B			;vbB5+vgG5,vbB4 -> mm3
		punpckhbw	mm1, mm7					;B7G7R7B6 -> mm1

		paddd		mm0, mm6					;U5U4 -> mm0
		movq		mm6, mm1					;B7G7R7B6 -> mm6

		pmaddwd		mm6, const_YBG0B			;ybB7+ygG7,ybB6 -> mm6
		punpckhbw	mm5, mm7					;R7B6G6R6 -> mm5

		movq		mm7, mm5					;R7B6G6R6 -> mm7
		paddd		mm3, mm4					;V5V4 -> mm3

		pmaddwd		mm5, const_YR0GR			;yrR7,ygG6+yrR6 -> mm5
		movq		mm4, mm1					;B7G7R7B6 -> mm4

		pmaddwd		mm4,const_UBG0B				;ubB7+ugG7,ubB6 -> mm4
		psrad		mm0, FCONVERSION_BITS		;32-bit scaled U5U4 -> mm0

		psrad		mm2, FCONVERSION_BITS		;32-bit scaled Y5Y4 -> mm2	
		nop										;//JS

		paddd		mm6, mm5					;Y7Y6 -> mm6
		movq		mm5, mm7					;R7B6G6R6 -> mm5

		pmaddwd		mm7, const_UR0GR			;urR7,ugG6+ugR6 -> mm7
		psrad		mm3, FCONVERSION_BITS		;32-bit scaled V5V4 -> mm3

		pmaddwd		mm1, const_VBG0B			;vbB7+vgG7,vbB6 -> mm1
		psrad		mm6, FCONVERSION_BITS		;32-bit scaled Y7Y6 -> mm6

		packssdw	mm2, mm6					;Y7Y6Y5Y4 -> mm2
//		nop										;//JS

		pmaddwd		mm5, const_VR0GR			;vrR7,vgG6+vrR6 -> mm5
		paddd		mm7, mm4					;U7U6 -> mm7

		psrad		mm7, FCONVERSION_BITS		;32-bit scaled U7U6 -> mm7
//		nop

		movq		mm6, tempY					;32-bit scaled Y3Y2Y1Y0 -> mm6
		packssdw	mm0, mm7					;32-bit scaled U7U6U5U4 -> mm0

		movq		mm4, tempU					;32-bit scaled U3U2U1U0 -> mm4
		packuswb	mm6, mm2					;all 8 Y values -> mm6

		movq		mm7, const_128				;128,128,128,128 -> mm7
		paddd		mm1, mm5					;V7V6  -> mm1

		paddw		mm0, mm7					;add offset to U7U6U5U4
//		nop

		paddw		mm4, mm7					;add offset to U3U2U1U0
		psrad		mm1, FCONVERSION_BITS		;32-bit scaled V7V6 -> mm1

		movq		[ebx], mm6					;store Y
		packuswb	mm4, mm0					;all 8 U values -> mm4

		movq		mm5, tempV					;32-bit scaled V3V2V1V0 -> mm5
		packssdw	mm3, mm1					;V7V6V5V4 -> mm3

		paddw		mm5, mm7					;add offset to 	V3V2V1V0
		paddw		mm3, mm7					;add offset to 	V7V6V5V4

		movq		[ecx], mm4					;store U
		packuswb	mm5, mm3					;all 8 V values -> mm5

		add			ebx, 8						;increment Y count
		add			ecx, 8						;increment U count

		movq		[edx], mm5					;store V
//		nop

		add			edx, 8						;increment V count
//		nop

		dec			edi							;decrement loop counter
		jnz			RGBtoYUV					;do 24 more bytes if not 0

//JS  The following emms instruction is purposely commented out.
	//emms       // commented out since it is done after the DCT

	} // end of __asm

} // end of MRGB2YCbCr

void MRGBA2YCbCrA(
	int rows,
	int cols,
	unsigned char *inRGBA,
	unsigned char *outY,
	unsigned char *outU,
	unsigned char *outV,
	unsigned char *outA)
{
//	make global to align on stack properly
//	__int64  tempY, tempU, tempV, tempA;


	
// written by Dave Shade - Intel Corp.
// Feb '97
//
// This color space conversion routine converts 
// true color pixels from RGBA to YCbCrA 
// one pass through the loop processes 4 pixels
// there is no provision for cols not an even multiple of 4

	__asm {

		// initializations
//DS - IJG will always call with rows=1, so don't multiply
//		mov			eax, rows
//		mov			ebx, cols
//		mul			ebx					;number pixels
// reorder to take advantage of Pentium v-pipe
		mov			edi, cols
		mov			eax, inRGBA

		shr			edi, 2				;number of loops = (rows*cols)/4
		mov			edx, outV

		mov			ecx, outU
		mov			esi, outA

		mov			ebx, outY

		// top of loop

	RGBAtoYUVA:

		movq		mm3, [eax+8]		;load #1 A1B1G1R1A0B0G0R0 -> mm3
		pxor		mm6, mm6			;0 -> mm6

		movq		mm4, mm3			;A1B1G1R1A0B0G0R0 -> mm4
		psrlq		mm3, 32				;00000000A1B1G1R1 -> mm3

		punpcklwd	mm4, mm3			;A1B1A0B0G1R1G0R0 -> mm4
		add			esi, 4

		movq		mm0, mm4			;A1B1A0B0G1R1G0R0 -> mm0
		punpckhbw	mm4, mm6			;A1B1A0B0 -> mm4

		movq		mm3, mm4			;A1B1A0B0 -> mm3
		punpcklbw	mm0, mm6			;G1R1G0R0 -> mm0

		pmaddwd		mm3, const2_Y0B0B	;ybB1,ybB0 -> mm3
		movq		mm1, mm0			;G1R1G0R0 -> mm1

		pmaddwd		mm0, const2_YGRGR	;yrG1+ygR1,ygG0+yrR0 -> mm0
		movq		mm5, mm4			;A1B1A0B0 -> mm5

		pmaddwd		mm4, const2_U0B0B	;ubB1,ubB0 -> mm4
		movq		mm2, mm1			;G1R1G0R0 -> mm2

		pmaddwd		mm1, const2_UGRGR	;urG1+ugR1,ugG0+urR0 -> mm1
		movq		mm7, mm5			;A1B1A0B0 -> mm7

		pmaddwd		mm5, const2_V0B0B	;vbB1,vbB0 -> mm5
		paddd		mm0, mm3			;Y1Y0 -> mm0

		pmaddwd		mm2, const2_VGRGR	;vgG1+vrR1,vgG0+vrR0 -> mm2
		psrad		mm0, FCONVERSION_BITS	;32 bit scaled Y1Y0

		movq		mm3, [eax]		;*load #2 A3B3G3R3A2B2G2R2 -> mm3
		paddd		mm1, mm4			;U1U0 -> mm2

		pmaddwd		mm7, const2_A		;1*A1,1*A0
		psrad		mm1, FCONVERSION_BITS	;32 bit scaled U1U0

		movq		tempY, mm0			;write out Y1Y0 in 32 bit format
		paddd		mm2, mm5			;V1V0 -> mm2
		
		movq		mm4, mm3			;*A3B3G3R3A2B2G2R2 -> mm4
		psrad		mm2, FCONVERSION_BITS	;32bit scaled V1V0

		movq		tempU, mm1			;write out U1U0 in 32 bit format
		psrlq		mm3, 32				;*00000000A3B3G3R3 -> mm3

		movq		tempV, mm2			;write out V1V0 in 32 bit format
		punpcklwd	mm4, mm3			;*A3B3A2B2G3R3G2R2 -> mm4

		movq		tempA, mm7
		movq		mm0, mm4			;*A3B3A2B2G3R3G2R2 -> mm0

		punpckhbw	mm4, mm6			;*A3B3A2B2 -> mm4
		add			eax, 16

		movq		mm3, mm4			;*A3B3A2B2 -> mm3
		punpcklbw	mm0, mm6			;*G3R3G2R2 -> mm0

		pmaddwd		mm3, const2_Y0B0B	;*ybB3,ybB2 -> mm3
		movq		mm1, mm0			;*G3R3G2R2 -> mm1

		pmaddwd		mm0, const2_YGRGR	;*yrG3+ygR3,ygG2+yrR2 -> mm0
		movq		mm5, mm4			;*A3B3A2B2 -> mm5

		pmaddwd		mm4, const2_U0B0B	;*ubB3,ubB2 -> mm4
		movq		mm2, mm1			;*G3R3G2R2 -> mm2

		pmaddwd		mm1, const2_UGRGR	;*urG3+ugR3,ugG2+urR2 -> mm1
		movq		mm7, mm5			;*A3B3A2B2 -> mm7

		pmaddwd		mm5, const2_V0B0B	;*vbB3,vbB2 -> mm5
		paddd		mm0, mm3			;*Y3Y2 -> mm0

		pmaddwd		mm2, const2_VGRGR	;*vgG3+vrR3,vgG2+vrR2 -> mm2
		psrad		mm0, FCONVERSION_BITS

		pmaddwd		mm7, const2_A		;* 1*A3,1*A2
		paddd		mm1, mm4			;*U3U2 -> mm2

		movq		mm6, const_128
		psrad		mm1, FCONVERSION_BITS

		packssdw	mm0, tempY			;*pack Y3Y2,Y1Y0 -> mm0
		paddd		mm2, mm5			;*V3V2 -> mm2

		psrad		mm2, FCONVERSION_BITS
		add			ebx, 4

		packssdw	mm1, tempU			;*pack U3U2,U1U0 -> mm1

		
		packssdw	mm2, tempV			;*pack V3V2,V1V0 -> mm2
		paddw		mm1, mm6			;add 128

		packssdw	mm7, tempA			;*pack A3A2,A1A0 -> mm7
		paddw		mm2, mm6			;add 128

		packuswb	mm0, mm0
		add			ecx, 4

		packuswb	mm1, mm1
		add			edx, 4

		movd		[ebx-4], mm0
		packuswb	mm2, mm2

		movd		[ecx-4], mm1
		packuswb	mm7, mm7

		movd		[edx-4], mm2

		movd		[esi-4], mm7
		
		dec			edi
		jnz			RGBAtoYUVA


//JS  The following emms instruction is purposely commented out.
	//emms       // commented out since it is done after the DCT


	} // end of __asm

} // end of MRGBA2YCbCrA

void MRGBA2YCbCrALegacy(
	int rows,
	int cols,
	unsigned char *inRGBA,
	unsigned char *outY,
	unsigned char *outU,
	unsigned char *outV,
	unsigned char *outA)
{
// ensure proper stack alignment by making global
//	__int64  tempY, tempU, tempV, tempA;

// written by Dave Shade - Intel Corp.
// Feb '97
//
// This color space conversion routine converts 
// true color pixels from RGBA to YCbCrA 
// This routine subtracts the RGB components from 255 before converting them
// one pass through the loop processes 4 pixels
// there is no provision for cols not an even multiple of 4

	__asm {

		// initializations
//DS - IJG will always call with rows=1, so don't multiply
//		mov			eax, rows
//		mov			ebx, cols
//		mul			ebx					;number pixels
// reorder to take advantage of Pentium v-pipe
		mov			edi, cols
		mov			eax, inRGBA

		shr			edi, 2				;number of loops = (rows*cols)/4
		mov			edx, outV

		mov			ecx, outU
		mov			esi, outA

		mov			ebx, outY

		// top of loop

	RGBAtoYUVALegacy:

		movq		mm3, [eax+8]		;load #1 A1B1G1R1A0B0G0R0 -> mm3
		pxor		mm6, mm6			;0 -> mm6
		
		pxor		mm3, const2_Legacy	; subtract MaxJSample FlashPix rev. 1 "thing"

		movq		mm4, mm3			;A1B1G1R1A0B0G0R0 -> mm4
		psrlq		mm3, 32				;00000000A1B1G1R1 -> mm3

		punpcklwd	mm4, mm3			;A1B1A0B0G1R1G0R0 -> mm4
		add			esi, 4				;opportunistically increment pointer

		movq		mm0, mm4			;A1B1A0B0G1R1G0R0 -> mm0
		punpckhbw	mm4, mm6			;A1B1A0B0 -> mm4

		movq		mm3, mm4			;A1B1A0B0 -> mm3
		punpcklbw	mm0, mm6			;G1R1G0R0 -> mm0

		pmaddwd		mm3, const2_Y0B0B	;ybB1,ybB0 -> mm3
		movq		mm1, mm0			;G1R1G0R0 -> mm1

		pmaddwd		mm0, const2_YGRGR	;yrG1+ygR1,ygG0+yrR0 -> mm0
		movq		mm5, mm4			;A1B1A0B0 -> mm5

		pmaddwd		mm4, const2_U0B0B	;ubB1,ubB0 -> mm4
		movq		mm2, mm1			;G1R1G0R0 -> mm2

		pmaddwd		mm1, const2_UGRGR	;urG1+ugR1,ugG0+urR0 -> mm1
		movq		mm7, mm5			;A1B1A0B0 -> mm7

		pmaddwd		mm5, const2_V0B0B	;vbB1,vbB0 -> mm5
		paddd		mm0, mm3			;Y1Y0 -> mm0

		pmaddwd		mm2, const2_VGRGR	;vgG1+vrR1,vgG0+vrR0 -> mm2
		psrad		mm0, FCONVERSION_BITS	;32 bit scaled Y1Y0

		psrld		mm7, 16				;shift A1A0 down 
		
		movq		mm3, [eax]			;*load #2 A3B3G3R3A2B2G2R2 -> mm3
		paddd		mm1, mm4			;U1U0 -> mm2

		pxor		mm3, const2_Legacy
		psrad		mm1, FCONVERSION_BITS	;32 bit scaled U1U0

		movq		tempY, mm0			;write out Y1Y0 in 32 bit format
		paddd		mm2, mm5			;V1V0 -> mm2
		
		movq		mm4, mm3			;*A3B3G3R3A2B2G2R2 -> mm4
		psrad		mm2, FCONVERSION_BITS	;32bit scaled V1V0

		movq		tempU, mm1			;write out U1U0 in 32 bit format
		psrlq		mm3, 32				;*00000000A3B3G3R3 -> mm3

		movq		tempV, mm2			;write out V1V0 in 32 bit format
		punpcklwd	mm4, mm3			;*A3B3A2B2G3R3G2R2 -> mm4

		movq		tempA, mm7
		movq		mm0, mm4			;*A3B3A2B2G3R3G2R2 -> mm0

		punpckhbw	mm4, mm6			;*A3B3A2B2 -> mm4
		add			eax, 16				;opportunistically increment pointer

		movq		mm3, mm4			;*A3B3A2B2 -> mm3
		punpcklbw	mm0, mm6			;*G3R3G2R2 -> mm0

		pmaddwd		mm3, const2_Y0B0B	;*ybB3,ybB2 -> mm3
		movq		mm1, mm0			;*G3R3G2R2 -> mm1

		pmaddwd		mm0, const2_YGRGR	;*yrG3+ygR3,ygG2+yrR2 -> mm0
		movq		mm5, mm4			;*A3B3A2B2 -> mm5

		pmaddwd		mm4, const2_U0B0B	;*ubB3,ubB2 -> mm4
		movq		mm2, mm1			;*G3R3G2R2 -> mm2

		pmaddwd		mm1, const2_UGRGR	;*urG3+ugR3,ugG2+urR2 -> mm1
		movq		mm7, mm5			;*A3B3A2B2 -> mm7

		pmaddwd		mm5, const2_V0B0B	;*vbB3,vbB2 -> mm5
		paddd		mm0, mm3			;*Y3Y2 -> mm0

		pmaddwd		mm2, const2_VGRGR	;*vgG3+vrR3,vgG2+vrR2 -> mm2
		psrad		mm0, FCONVERSION_BITS	;shift Y3Y2 by 15 bits

		psrld		mm7, 16				;shift the alpha values down
		paddd		mm1, mm4			;*U3U2 -> mm2

		movq		mm6, const_128		; load mm6 with 128
		psrad		mm1, FCONVERSION_BITS	;shift U3U2 by 15 bits

		packssdw	mm0, tempY			;*pack Y3Y2,Y1Y0 -> mm0
		paddd		mm2, mm5			;*V3V2 -> mm2

		packssdw	mm1, tempU			;*pack U3U2,U1U0 -> mm1

		psrad		mm2, FCONVERSION_BITS	;shift V3V2 by 15 bits
		add			ebx, 4				;opportunistically increment pointer

		packssdw	mm2, tempV			;pack V3V2,V1V0 -> mm2
		paddw		mm1, mm6			;add 128

		packssdw	mm7, tempA			;pack A3A2,A1A0 -> mm7
		paddw		mm2, mm6			;add 128

		packuswb	mm0, mm0			;pack Y3Y2Y1Y0 from 16 bit to 8 bit
		add			ecx, 4				;opportunistically increment pointer

		packuswb	mm1, mm1			;pack U3U2U1U0 from 16 bit to 8 bit
		add			edx, 4				;opportunistically increment pointer

		movd		[ebx-4], mm0		;write out Y3Y2Y1Y0
		packuswb	mm2, mm2			;pack V3V2V1V0 from 16 bit to 8 bit

		movd		[ecx-4], mm1		;write out U3U2U1U0
		packuswb	mm7, mm7			;pack A3A2A1A0 from 16 bit to 8 bits

		movd		[edx-4], mm2		;write out V3V2V1V0

		movd		[esi-4], mm7		;write out A3A2A1A0
		
		dec			edi					;subtract 4 from number of pixels
		jnz			RGBAtoYUVALegacy


//JS  The following emms instruction is purposely commented out.
	//emms       // commented out since it is done after the DCT


	} // end of __asm

} // end of MRGBA2YCbCrALegacy

#endif // defined (_X86_)
