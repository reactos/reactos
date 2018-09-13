/*
 * jdcolor.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains output colorspace conversion routines.
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
#define unscale(x)      (((x) + SCALE_RND) >> SCALE_PREC)
#define clip(x)         (((long)(x) & ~0xff) ? (((long)(x) < 0) ? 0 : 255) : (long)(x))

#endif


/* Private subobject */

typedef struct {
  struct jpeg_color_deconverter pub; /* public fields */

  /* Private state for YCC->RGB conversion */
  int * Cr_r_tab;		/* => table for Cr to R conversion */
  int * Cb_b_tab;		/* => table for Cb to B conversion */
  INT32 * Cr_g_tab;		/* => table for Cr to G conversion */
  INT32 * Cb_g_tab;		/* => table for Cb to G conversion */

#ifdef NIFTY
  /* Private state for the PhotoYCC->RGB conversion tables */
  coef_c1 *C1;
  coef_c2 *C2;
  short *xy;
#endif

} my_color_deconverter;


/* Added header info - CRK */
extern void MYCbCr2RGB(
  int columns,	  
  unsigned char *inY,
  unsigned char *inU,
  unsigned char *inV,
  unsigned char *outRGB);

extern void MYCbCrA2RGBA(
  int columns,	  
  unsigned char *inY,
  unsigned char *inU,
  unsigned char *inV,
  unsigned char *inA,
  unsigned char *outRGBA);

extern void MYCbCrA2RGBALegacy(
  int columns,	  
  unsigned char *inY,
  unsigned char *inU,
  unsigned char *inV,
  unsigned char *inA,
  unsigned char *outRGBA);
// These constants correspond to CCIR 601-1
// R = [256*Y + 359*(Cr-128)] / 256
// G = [256*Y - 88*(Cb-128) - 183*(Cr-128)] / 256
// B = [256*Y + 454*(Cb-128)] / 256
//Conventional floating point equations:
//	R = Y + 1.40200 * Cr
//	G = Y - 0.34414 * Cb - 0.71414 * Cr
//	B = Y + 1.77200 * Cb

//Ry=0100 Ru=0000 Rv=0167
//Gy=0100 Gu=FFA8 Gv=FF49
//By=0100 Bu=01C6 Bv=0000
// constants for YCbCr->RGB and YCbCrA->RGBA
static __int64 const_0		= 0x0000000000000000;
static __int64 const_sub128	= 0x0080008000800080;
static __int64 const_VUmul	= 0xFF49FFA8FF49FFA8;
static __int64 const_YVmul	= 0x0100016701000167;
static __int64 const_YUmul	= 0x010001C6010001C6;
static __int64 mask_highd	= 0xFFFFFFFF00000000;
static __int64 const_invert	= 0x00FFFFFF00FFFFFF;


//These constants correspond to the original FPX SDK
// R = [256*Y + 410*(Cr-128)] / 256
// G = [256*Y - 85*(Cb-128) - 205*(Cr-128)] / 256
// B = [256*Y + 512*(Cb-128)] / 256
//Conventional floating point equations:
// R = Y + 1.60000*(Cr)
// G = Y - 0.33333*(Cb) - 0.80000*(Cr)
// B = Y + 2.00000*(Cb)

//Ry=0100 Ru=0000 Rv=019A
//Gy=0100 Gu=FFAB Gv=FF33
//By=0100 Bu=0200 Bv=0000
// constants for YCbCr->RGB and YCbCrA->RGBA
//const __int64 const_0		= 0x0000000000000000;
//const __int64 const_sub128= 0x0080008000800080;
//const __int64 const_VUmul	= 0xFF33FFABFF33FFAB;
//const __int64 const_YVmul	= 0x0100019A0100019A;
//const __int64 const_YUmul	= 0x0001000200010002;
//const __int64 mask_highd	= 0xFFFFFFFF00000000;
//const __int64 const_invert= 0x00FFFFFF00FFFFFF;

/* End of added info - CRK */



typedef my_color_deconverter * my_cconvert_ptr;

#ifdef NIFTY

/*
 * Initialize tables for PhotoYCC->RGB colorspace conversion.
 */

LOCAL (void)
build_pycc_rgb_table (j_decompress_ptr cinfo)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr)cinfo->cconvert;
  INT32 i;

  cconvert->C1 = (coef_c1 *)
	(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_IMAGE,
				   256 * SIZEOF(coef_c1));
  cconvert->C2 = (coef_c2 *)
	(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_IMAGE,
				   256 * SIZEOF(coef_c2));
  cconvert->xy = (short *)
	(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_IMAGE,
				   256 * SIZEOF(short));

  for (i = 0; i < 256; i++) {
    cconvert->xy[i] = (short)((double)i * 1.3584 * SCALE);
    cconvert->C2[i].r = (short)(i * 1.8215 * SCALE);
    cconvert->C1[i].g = (short)(i * -0.4303 * SCALE);
    cconvert->C2[i].g = (short)(i * -0.9271 * SCALE);
    cconvert->C1[i].b = (short)(i * 2.2179 * SCALE);
  }
}

/*
 * PhotoYCC->RGB colorspace conversion.
 */
METHODDEF (void)
pycc_rgb_convert (j_decompress_ptr cinfo,
                 JSAMPIMAGE input_buf, JDIMENSION input_row,
                 JSAMPARRAY output_buf, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr)cinfo->cconvert;
  register JSAMPROW inptr0, inptr1, inptr2;
  register JSAMPROW outptr;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;
  unsigned char y, c1, c2;
  short ri, gi, bi,
        offsetR, offsetG, offsetB;
  register short *xy = cconvert->xy;
  register coef_c1 *C1 = cconvert->C1;
  register coef_c2 *C2 = cconvert->C2;
 
/*
  for (i = 0; i < 256; i++) {
    xy[i] = (short)((double)i * 1.3584 * SCALE);
    C2[i].r = (short)(i * 1.8215 * SCALE);
    C1[i].g = (short)(i * -0.4303 * SCALE);
    C2[i].g = (short)(i * -0.9271 * SCALE);
    C1[i].b = (short)(i * 2.2179 * SCALE);
  }
*/
 
  offsetR = (short)(-249.55 * SCALE);
  offsetG = (short)( 194.14 * SCALE);
  offsetB = (short)(-345.99 * SCALE);
 
  while (--num_rows >= 0) {
    inptr0 = input_buf[0][input_row];
    inptr1 = input_buf[1][input_row];
    inptr2 = input_buf[2][input_row];
    input_row++;
    outptr = *output_buf++;
    for (col = 0; col < num_cols; col++) {
      y = GETJSAMPLE(inptr0[col]);
      c1 = GETJSAMPLE(inptr1[col]);
      c2 = GETJSAMPLE(inptr2[col]);
 
      ri = xy[y] + C2[c2].r + offsetR;
      gi = xy[y] + C1[c1].g + C2[c2].g + offsetG;
      bi = xy[y] + C1[c1].b + offsetB;
 
      ri = (short)unscale(ri);
      gi = (short)unscale(gi);
      bi = (short)unscale(bi);
 
      outptr[RGB_RED] = (JSAMPLE)clip(ri);
      outptr[RGB_GREEN] = (JSAMPLE)clip(gi);
      outptr[RGB_BLUE] = (JSAMPLE)clip(bi);
      outptr+=3;
    }
  }
}


/*
 * PhotoYCC->RGBA colorspace conversion.
 */
METHODDEF (void)
pycc_rgba_convert (j_decompress_ptr cinfo,
                 JSAMPIMAGE input_buf, JDIMENSION input_row,
                 JSAMPARRAY output_buf, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr)cinfo->cconvert;
  register JSAMPROW inptr0, inptr1, inptr2;
  register JSAMPROW outptr;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;
  unsigned char y, c1, c2;
  short ri, gi, bi,
        offsetR, offsetG, offsetB;
  register short *xy = cconvert->xy;
  register coef_c1 *C1 = cconvert->C1;
  register coef_c2 *C2 = cconvert->C2;
 
  offsetR = (short)(-249.55 * SCALE);
  offsetG = (short)( 194.14 * SCALE);
  offsetB = (short)(-345.99 * SCALE);
 
  while (--num_rows >= 0) {
    inptr0 = input_buf[0][input_row];
    inptr1 = input_buf[1][input_row];
    inptr2 = input_buf[2][input_row];
    input_row++;
    outptr = *output_buf++;
    for (col = 0; col < num_cols; col++) {
      y = GETJSAMPLE(inptr0[col]);
      c1 = GETJSAMPLE(inptr1[col]);
      c2 = GETJSAMPLE(inptr2[col]);
 
      ri = xy[y] + C2[c2].r + offsetR;
      gi = xy[y] + C1[c1].g + C2[c2].g + offsetG;
      bi = xy[y] + C1[c1].b + offsetB;
 
      ri = (short)unscale(ri);
      gi = (short)unscale(gi);
      bi = (short)unscale(bi);
 
      outptr[RGB_RED] = (JSAMPLE)clip(ri);
      outptr[RGB_GREEN] = (JSAMPLE)clip(gi);
      outptr[RGB_BLUE] = (JSAMPLE)clip(bi);
	  outptr[3] = 255;
      outptr+=4;
    }
  }
}

#endif

/**************** YCbCr -> RGB conversion: most common case **************/

/*
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 *	R = Y                + 1.40200 * Cr
 *	G = Y - 0.34414 * Cb - 0.71414 * Cr
 *	B = Y + 1.77200 * Cb
 * where Cb and Cr represent the incoming values less CENTERJSAMPLE.
 * (These numbers are derived from TIFF 6.0 section 21, dated 3-June-92.)
 *
 * To avoid floating-point arithmetic, we represent the fractional constants
 * as integers scaled up by 2^16 (about 4 digits precision); we have to divide
 * the products by 2^16, with appropriate rounding, to get the correct answer.
 * Notice that Y, being an integral input, does not contribute any fraction
 * so it need not participate in the rounding.
 *
 * For even more speed, we avoid doing any multiplications in the inner loop
 * by precalculating the constants times Cb and Cr for all possible values.
 * For 8-bit JSAMPLEs this is very reasonable (only 256 entries per table);
 * for 12-bit samples it is still acceptable.  It's not very reasonable for
 * 16-bit samples, but if you want lossless storage you shouldn't be changing
 * colorspace anyway.
 * The Cr=>R and Cb=>B values can be rounded to integers in advance; the
 * values for the G calculation are left scaled up, since we must add them
 * together before rounding.
 */

#define SCALEBITS	16	/* speediest right-shift on some machines */
#define ONE_HALF	((INT32) 1 << (SCALEBITS-1))
#define FIX(x)		((INT32) ((x) * (1L<<SCALEBITS) + 0.5))


/*
 * Initialize tables for YCC->RGB colorspace conversion.
 */

LOCAL(void)
build_ycc_rgb_table (j_decompress_ptr cinfo)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  int i;
  INT32 x;
  SHIFT_TEMPS

  cconvert->Cr_r_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  cconvert->Cb_b_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  cconvert->Cr_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));
  cconvert->Cb_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));

  for (i = 0, x = -CENTERJSAMPLE; i <= MAXJSAMPLE; i++, x++) {
    /* i is the actual input pixel value, in the range 0..MAXJSAMPLE */
    /* The Cb or Cr value we are thinking of is x = i - CENTERJSAMPLE */
    /* Cr=>R value is nearest int to 1.40200 * x */
    cconvert->Cr_r_tab[i] = (int)
		    RIGHT_SHIFT(FIX(1.40200) * x + ONE_HALF, SCALEBITS);
    /* Cb=>B value is nearest int to 1.77200 * x */
    cconvert->Cb_b_tab[i] = (int)
		    RIGHT_SHIFT(FIX(1.77200) * x + ONE_HALF, SCALEBITS);
    /* Cr=>G value is scaled-up -0.71414 * x */
    cconvert->Cr_g_tab[i] = (- FIX(0.71414)) * x;
    /* Cb=>G value is scaled-up -0.34414 * x */
    /* We also add in ONE_HALF so that need not do it in inner loop */
    cconvert->Cb_g_tab[i] = (- FIX(0.34414)) * x + ONE_HALF;
  }
}


/*
 * Convert some rows of samples to the output colorspace.
 *
 * Note that we change from noninterleaved, one-plane-per-component format
 * to interleaved-pixel format.  The output buffer is therefore three times
 * as wide as the input buffer.
 * A starting row offset is provided only for the input buffer.  The caller
 * can easily adjust the passed output_buf value to accommodate any row
 * offset required on that side.
 */


METHODDEF(void)
ycc_rgb_convert (j_decompress_ptr cinfo,
		 JSAMPIMAGE input_buf, JDIMENSION input_row,
		 JSAMPARRAY output_buf, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int y, cb, cr;
  register JSAMPROW outptr;
  register JSAMPROW inptr0, inptr1, inptr2;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;
// Alignment variables - CRK
  JDIMENSION tail_cols = num_cols&7;
  JDIMENSION mmx_cols=num_cols&~7;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  register int * Crrtab = cconvert->Cr_r_tab;
  register int * Cbbtab = cconvert->Cb_b_tab;
  register INT32 * Crgtab = cconvert->Cr_g_tab;
  register INT32 * Cbgtab = cconvert->Cb_g_tab;
  SHIFT_TEMPS
  
//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)
  if(vfMMXMachine) { //MMX Code - CRK
	while (--num_rows >= 0) {
		inptr0 = input_buf[0][input_row];
		inptr1 = input_buf[1][input_row];
		inptr2 = input_buf[2][input_row];
		input_row++;
		outptr = *output_buf++;
		MYCbCr2RGB(mmx_cols, inptr0, inptr1, inptr2, outptr);
		
		outptr += 3*mmx_cols;
		for (col = mmx_cols; col < num_cols; col++) {
		  y  = GETJSAMPLE(inptr0[col]);
		  cb = GETJSAMPLE(inptr1[col]);
		  cr = GETJSAMPLE(inptr2[col]);
		  /* Range-limiting is essential due to noise introduced by DCT losses. */
		  outptr[RGB_RED] =   range_limit[y + Crrtab[cr]];
		  outptr[RGB_GREEN] = range_limit[y +
				      ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
							 SCALEBITS))];
		  outptr[RGB_BLUE] =  range_limit[y + Cbbtab[cb]];
		  outptr += RGB_PIXELSIZE;
		}
	}
__asm emms	
  }
  else 
      
      
#endif //defined (_X86_)     
        {
	while	(--num_rows >= 0) {
		inptr0 = input_buf[0][input_row];
		inptr1 = input_buf[1][input_row];
		inptr2 = input_buf[2][input_row];
		input_row++;
		outptr = *output_buf++;

		for (col = 0; col < num_cols; col++) {
		  y  = GETJSAMPLE(inptr0[col]);
		  cb = GETJSAMPLE(inptr1[col]);
		  cr = GETJSAMPLE(inptr2[col]);
		  /* Range-limiting is essential due to noise introduced by DCT losses. */
		  outptr[RGB_RED] =   range_limit[y + Crrtab[cr]];
		  outptr[RGB_GREEN] = range_limit[y +
				      ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
							 SCALEBITS))];
		  outptr[RGB_BLUE] =  range_limit[y + Cbbtab[cb]];
		  outptr += RGB_PIXELSIZE;
		}	
	}
  }
}



/**************** Cases other than YCbCr -> RGB **************/


/*
 * Color conversion for no colorspace change: just copy the data,
 * converting from separate-planes to interleaved representation.
 */

METHODDEF(void)
null_convert (j_decompress_ptr cinfo,
	      JSAMPIMAGE input_buf, JDIMENSION input_row,
	      JSAMPARRAY output_buf, int num_rows)
{
  register JSAMPROW inptr, outptr;
  register JDIMENSION count;
  register int num_components = cinfo->num_components;
  JDIMENSION num_cols = cinfo->output_width;
  int ci;

  while (--num_rows >= 0) {
    for (ci = 0; ci < num_components; ci++) {
      inptr = input_buf[ci][input_row];
      outptr = output_buf[0] + ci;
      for (count = num_cols; count > 0; count--) {
	*outptr = *inptr++;	/* needn't bother with GETJSAMPLE() here */
	outptr += num_components;
      }
    }
    input_row++;
    output_buf++;
  }
}


/*
 * Color conversion for grayscale: just copy the data.
 * This also works for YCbCr -> grayscale conversion, in which
 * we just copy the Y (luminance) component and ignore chrominance.
 */

METHODDEF(void)
grayscale_convert (j_decompress_ptr cinfo,
		   JSAMPIMAGE input_buf, JDIMENSION input_row,
		   JSAMPARRAY output_buf, int num_rows)
{
  jcopy_sample_rows(input_buf[0], (int) input_row, output_buf, 0,
		    num_rows, cinfo->output_width);
}

#ifdef NIFTY

//Not really a colour conversion but special one for Picture It!
//Copies 3 channel data and adds an alpha
METHODDEF(void)
rgb_rgba_convert (j_decompress_ptr cinfo,
	      JSAMPIMAGE input_buf, JDIMENSION input_row,
	      JSAMPARRAY output_buf, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register JSAMPROW outptr;
  register JSAMPROW inptr0, inptr1, inptr2;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;
  /* copy these pointers into registers if possible */
  SHIFT_TEMPS
 
  while (--num_rows >= 0) {
    inptr0 = input_buf[0][input_row];
    inptr1 = input_buf[1][input_row];
    inptr2 = input_buf[2][input_row];
    input_row++;
    outptr = *output_buf++;
    for (col = 0; col < num_cols; col++) {
      outptr[0] = GETJSAMPLE(inptr0[col]);
      outptr[1] = GETJSAMPLE(inptr1[col]);
      outptr[2] = GETJSAMPLE(inptr2[col]);
      /* Alpha is added as fully opaque */
      outptr[3] = 255;  /* don't need GETJSAMPLE here */
      outptr += 4;
    }
  }
}



METHODDEF (void)
ycbcra_rgba_convert (j_decompress_ptr cinfo,
                   JSAMPIMAGE input_buf, JDIMENSION input_row,
                   JSAMPARRAY output_buf, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int y, cb, cr;
  register JSAMPROW outptr;
  register JSAMPROW inptr0, inptr1, inptr2, inptr3;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;
  // Alignment variables - CRK
  JDIMENSION tail_cols = num_cols&7;
  JDIMENSION mmx_cols=num_cols&~7;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  register int * Crrtab = cconvert->Cr_r_tab;
  register int * Cbbtab = cconvert->Cb_b_tab;
  register INT32 * Crgtab = cconvert->Cr_g_tab;
  register INT32 * Cbgtab = cconvert->Cb_g_tab;
  SHIFT_TEMPS 
  
//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)

  if(vfMMXMachine) { //MMX Code - CRK
	while (--num_rows >= 0) {
		inptr0 = input_buf[0][input_row];
		inptr1 = input_buf[1][input_row];
		inptr2 = input_buf[2][input_row];
		inptr3 = input_buf[3][input_row];
		input_row++;
		outptr = *output_buf++;
		MYCbCrA2RGBA(mmx_cols, inptr0, inptr1, inptr2, inptr3, outptr);
		
		outptr += 4*mmx_cols;
		for (col = mmx_cols; col < num_cols; col++) {
		  y  = GETJSAMPLE(inptr0[col]);
		  cb = GETJSAMPLE(inptr1[col]);
		  cr = GETJSAMPLE(inptr2[col]);
		  /* Range-limiting is essential due to noise introduced by DCT losses. */
		  outptr[RGB_RED] =   range_limit[y + Crrtab[cr]];
		  outptr[RGB_GREEN] = range_limit[y +
				      ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
							 SCALEBITS))];
		  outptr[RGB_BLUE] =  range_limit[y + Cbbtab[cb]];
		  outptr[3] = inptr3[col];
		  outptr += 4;
		}
	}	
	__asm emms
  }
  else 
      
#endif // defined (_X86_)
      {
	while (--num_rows >= 0) {
		inptr0 = input_buf[0][input_row];
		inptr1 = input_buf[1][input_row];
		inptr2 = input_buf[2][input_row];
		inptr3 = input_buf[3][input_row];
		input_row++;
		outptr = *output_buf++;
		for (col = 0; col < num_cols; col++) {
		  y  = GETJSAMPLE(inptr0[col]);
		  cb = GETJSAMPLE(inptr1[col]);
		  cr = GETJSAMPLE(inptr2[col]);
		  /* Range-limiting is essential due to noise introduced by DCT losses. */
		  outptr[RGB_RED] = range_limit[(y + Crrtab[cr])];   /* red */
		  outptr[RGB_GREEN] = range_limit[(y +                 /* green */
		                          ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
		                                             SCALEBITS)))];
		  outptr[RGB_BLUE] = range_limit[(y + Cbbtab[cb])];   /* blue */
		  /* Alpha passes through unchanged */
		  outptr[3] = inptr3[col];  /* don't need GETJSAMPLE here */
		  outptr += 4;
		}
	 }
  }
}





METHODDEF (void)
ycbcralegacy_rgba_convert (j_decompress_ptr cinfo,
                   JSAMPIMAGE input_buf, JDIMENSION input_row,
                   JSAMPARRAY output_buf, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int y, cb, cr;
  register JSAMPROW outptr;
  register JSAMPROW inptr0, inptr1, inptr2, inptr3;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;
  // Alignment variables - CRK
  JDIMENSION tail_cols = num_cols&7;
  JDIMENSION mmx_cols=num_cols&~7;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  register int * Crrtab = cconvert->Cr_r_tab;
  register int * Cbbtab = cconvert->Cb_b_tab;
  register INT32 * Crgtab = cconvert->Cr_g_tab;
  register INT32 * Cbgtab = cconvert->Cb_g_tab;
  SHIFT_TEMPS

//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)

  if(vfMMXMachine) { //MMX Code - CRK
	while (--num_rows >= 0) {
		inptr0 = input_buf[0][input_row];
		inptr1 = input_buf[1][input_row];
		inptr2 = input_buf[2][input_row];
		inptr3 = input_buf[3][input_row];
		input_row++;
		outptr = *output_buf++;
		MYCbCrA2RGBALegacy(mmx_cols, inptr0, inptr1, inptr2, inptr3, outptr);
		
		outptr += 4*mmx_cols;
		for (col = mmx_cols; col < num_cols; col++) {
		  y  = GETJSAMPLE(inptr0[col]);
		  cb = GETJSAMPLE(inptr1[col]);
		  cr = GETJSAMPLE(inptr2[col]);
		  /* Range-limiting is essential due to noise introduced by DCT losses. */
		  outptr[RGB_RED] =   range_limit[MAXJSAMPLE - (y + Crrtab[cr])];
		  outptr[RGB_GREEN] = range_limit[MAXJSAMPLE - (y +
				      ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
							 SCALEBITS)))];
		  outptr[RGB_BLUE] =  range_limit[MAXJSAMPLE - (y + Cbbtab[cb])];
		  outptr[3] = inptr3[col];
		  outptr += 4;
		}
	 }	
	__asm emms
  }
  else 
      
#endif // defined (_X86_)
      {
	while (--num_rows >= 0) {
		inptr0 = input_buf[0][input_row];
		inptr1 = input_buf[1][input_row];
		inptr2 = input_buf[2][input_row];
		inptr3 = input_buf[3][input_row];
		input_row++;
		outptr = *output_buf++;
		for (col = 0; col < num_cols; col++) {
		  y  = GETJSAMPLE(inptr0[col]);
		  cb = GETJSAMPLE(inptr1[col]);
		  cr = GETJSAMPLE(inptr2[col]);
		  /* Range-limiting is essential due to noise introduced by DCT losses. */
		  outptr[RGB_RED] = range_limit[MAXJSAMPLE - (y + Crrtab[cr])];   /* red */
		  outptr[RGB_GREEN] = range_limit[MAXJSAMPLE - (y +                 /* green */
		              ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
			                 SCALEBITS)))];
		  outptr[RGB_BLUE] = range_limit[MAXJSAMPLE - (y + Cbbtab[cb])];   /* blue */
		  /* Alpha passes through unchanged */
		  outptr[3] = inptr3[col];  /* don't need GETJSAMPLE here */
		  outptr += 4;
		}
	}
  }
}



METHODDEF (void)
ycbcr_rgba_convert (j_decompress_ptr cinfo,
		 JSAMPIMAGE input_buf, JDIMENSION input_row,
		 JSAMPARRAY output_buf, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int y, cb, cr;
  register JSAMPROW outptr;
  register JSAMPROW inptr0, inptr1, inptr2;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  register int * Crrtab = cconvert->Cr_r_tab;
  register int * Cbbtab = cconvert->Cb_b_tab;
  register INT32 * Crgtab = cconvert->Cr_g_tab;
  register INT32 * Cbgtab = cconvert->Cb_g_tab;
  SHIFT_TEMPS

  while (--num_rows >= 0) {
    inptr0 = input_buf[0][input_row];
    inptr1 = input_buf[1][input_row];
    inptr2 = input_buf[2][input_row];
    input_row++;
    outptr = *output_buf++;
    for (col = 0; col < num_cols; col++) {
      y  = GETJSAMPLE(inptr0[col]);
      cb = GETJSAMPLE(inptr1[col]);
      cr = GETJSAMPLE(inptr2[col]);
      /* Range-limiting is essential due to noise introduced by DCT losses. */
      outptr[RGB_RED] =   range_limit[y + Crrtab[cr]];
      outptr[RGB_GREEN] = range_limit[y +
			      ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
						 SCALEBITS))];
      outptr[RGB_BLUE] =  range_limit[y + Cbbtab[cb]];
	  outptr[3] = 255;
      outptr += 4;
    }
  }
}

#endif

/*
 * Adobe-style YCCK->CMYK conversion.
 * We convert YCbCr to R=1-C, G=1-M, and B=1-Y using the same
 * conversion as above, while passing K (black) unchanged.
 * We assume build_ycc_rgb_table has been called.
 */

METHODDEF(void)
ycck_cmyk_convert (j_decompress_ptr cinfo,
		   JSAMPIMAGE input_buf, JDIMENSION input_row,
		   JSAMPARRAY output_buf, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int y, cb, cr;
  register JSAMPROW outptr;
  register JSAMPROW inptr0, inptr1, inptr2, inptr3;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  register int * Crrtab = cconvert->Cr_r_tab;
  register int * Cbbtab = cconvert->Cb_b_tab;
  register INT32 * Crgtab = cconvert->Cr_g_tab;
  register INT32 * Cbgtab = cconvert->Cb_g_tab;
  SHIFT_TEMPS

  while (--num_rows >= 0) {
    inptr0 = input_buf[0][input_row];
    inptr1 = input_buf[1][input_row];
    inptr2 = input_buf[2][input_row];
    inptr3 = input_buf[3][input_row];
    input_row++;
    outptr = *output_buf++;
    for (col = 0; col < num_cols; col++) {
      y  = GETJSAMPLE(inptr0[col]);
      cb = GETJSAMPLE(inptr1[col]);
      cr = GETJSAMPLE(inptr2[col]);
      /* Range-limiting is essential due to noise introduced by DCT losses. */
      outptr[0] = range_limit[MAXJSAMPLE - (y + Crrtab[cr])];	/* red */
      outptr[1] = range_limit[MAXJSAMPLE - (y +			/* green */
			      ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
						 SCALEBITS)))];
      outptr[2] = range_limit[MAXJSAMPLE - (y + Cbbtab[cb])];	/* blue */
      /* K passes through unchanged */
      outptr[3] = inptr3[col];	/* don't need GETJSAMPLE here */
      outptr += 4;
    }
  }
}


/*
 * Empty method for start_pass.
 */

METHODDEF(void)
start_pass_dcolor (j_decompress_ptr cinfo)
{
  /* no work needed */
}


/*
 * Module initialization routine for output colorspace conversion.
 */

GLOBAL(void)
jinit_color_deconverter (j_decompress_ptr cinfo)
{
  my_cconvert_ptr cconvert;
  int ci;

  cconvert = (my_cconvert_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_color_deconverter));
  cinfo->cconvert = (struct jpeg_color_deconverter *) cconvert;
  cconvert->pub.start_pass = start_pass_dcolor;

  /* Make sure num_components agrees with jpeg_color_space */
  switch (cinfo->jpeg_color_space) {
  case JCS_GRAYSCALE:
    if (cinfo->num_components != 1)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;
#ifdef NIFTY
  case JCS_YCC:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;
  case JCS_YCCA:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;
  case JCS_RGBA:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;
  case JCS_YCbCrA:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;
  case JCS_YCbCrALegacy:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;

#endif
  case JCS_RGB:
  case JCS_YCbCr:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;

  case JCS_CMYK:
  case JCS_YCCK:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;

  default:			/* JCS_UNKNOWN can be anything */
    if (cinfo->num_components < 1)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;
  }

  /* Set out_color_components and conversion method based on requested space.
   * Also clear the component_needed flags for any unused components,
   * so that earlier pipeline stages can avoid useless computation.
   */

  switch (cinfo->out_color_space) {
  case JCS_GRAYSCALE:
    cinfo->out_color_components = 1;
    if (cinfo->jpeg_color_space == JCS_GRAYSCALE ||
	cinfo->jpeg_color_space == JCS_YCbCr) {
      cconvert->pub.color_convert = grayscale_convert;
      /* For color->grayscale conversion, only the Y (0) component is needed */
      for (ci = 1; ci < cinfo->num_components; ci++)
	cinfo->comp_info[ci].component_needed = FALSE;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_RGB:
    cinfo->out_color_components = RGB_PIXELSIZE;
    if (cinfo->jpeg_color_space == JCS_YCbCr) {
      cconvert->pub.color_convert = ycc_rgb_convert;
      build_ycc_rgb_table(cinfo);
    } else if (cinfo->jpeg_color_space == JCS_RGB && RGB_PIXELSIZE == 3) {
      cconvert->pub.color_convert = null_convert;
#ifdef NIFTY
    } else if (cinfo->jpeg_color_space == JCS_YCC) {
      cconvert->pub.color_convert = pycc_rgb_convert;
      build_pycc_rgb_table(cinfo);
#endif
	} else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

#ifdef NIFTY
  case JCS_RGBA:
    cinfo->out_color_components = 4;
    if (cinfo->jpeg_color_space == JCS_YCbCrA) {
      cconvert->pub.color_convert = ycbcra_rgba_convert;
      build_ycc_rgb_table(cinfo);
	}else if (cinfo->jpeg_color_space == JCS_YCbCrALegacy) {
      cconvert->pub.color_convert = ycbcralegacy_rgba_convert;
      build_ycc_rgb_table(cinfo);
    }else if (cinfo->jpeg_color_space == JCS_YCbCr) {
      cconvert->pub.color_convert = ycbcr_rgba_convert;
      build_ycc_rgb_table(cinfo);
	}else if (cinfo->jpeg_color_space == JCS_RGBA) {
      cconvert->pub.color_convert = null_convert;
    }else if (cinfo->jpeg_color_space == JCS_RGB) {
      cconvert->pub.color_convert = rgb_rgba_convert;
	}else if (cinfo->jpeg_color_space == JCS_YCC) {
      cconvert->pub.color_convert = pycc_rgba_convert;
      build_pycc_rgb_table(cinfo);
	} else {
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    }
    break;
#endif


  case JCS_CMYK:
    cinfo->out_color_components = 4;
    if (cinfo->jpeg_color_space == JCS_YCCK) {
      cconvert->pub.color_convert = ycck_cmyk_convert;
      build_ycc_rgb_table(cinfo);
    } else if (cinfo->jpeg_color_space == JCS_CMYK) {
      cconvert->pub.color_convert = null_convert;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  default:
    /* Permit null conversion to same output space */
    if (cinfo->out_color_space == cinfo->jpeg_color_space) {
      cinfo->out_color_components = cinfo->num_components;
      cconvert->pub.color_convert = null_convert;
    } else			/* unsupported non-null conversion */
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;
  }

  if (cinfo->quantize_colors)
    cinfo->output_components = 1; /* single colormapped output component */
  else
    cinfo->output_components = cinfo->out_color_components;
}


//  MMX assembly code editions begin here - CRK

//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)
    
void MYCbCr2RGB(
  int columns,	  
  unsigned char *inY,
  unsigned char *inU,
  unsigned char *inV,
  unsigned char *outRGB)
{
  _asm {
	// Inits
	mov		eax, inY
	mov		ecx, inV

	mov		edi, columns
	mov		ebx, inU

	shr		edi, 2				; number of loops = cols/4 
	mov		edx, outRGB

YUVtoRGB:
	movd	mm0, [eax]			; 0/0/0/0/Y3/Y2/Y1/Y0
	pxor	mm7, mm7			; use mm7 as const_0 to achieve better pairing at start

	movd	mm2, [ebx]			; 0/0/0/0/U3/U2/U1/U0
	punpcklbw	mm0, mm7		; Y3/Y2/Y1/Y0

	movd	mm3, [ecx]			; 0/0/0/0/V3/V2/V1/V0
	punpcklbw	mm2, mm7		; U3/U2/U1/U0
	
	psubsw	mm2, const_sub128	; U3'/U2'/U1'/U0'
	punpcklbw	mm3, mm7		; V3/V2/V1/V0

	psubsw	mm3, const_sub128	; V3'/V2'/V1'/V0'
	movq	mm4, mm2
	
	punpcklwd	mm2, mm3		; V1'/U1'/V0'/U0'
	movq	mm1, mm0			

	pmaddwd	mm2, const_VUmul	; gvV1'+guU1'/gvV0'+guU0'
	psllw	mm1, 8				; Y3*256/Y2*256/Y1*256/Y0*256

	movq	mm6, mm1
	punpcklwd	mm1, mm7		; Y1*256/Y0*256
	
	punpckhwd	mm6, mm7		; Y3*256/Y2*256
	movq	mm5, mm4

	punpckhwd	mm5, mm3		; V3'/U3'/V2'/U2'
	paddd	mm2, mm1			; G1*256/G0*256		(mm1 free)

	pmaddwd	mm5, const_VUmul	; gvV3'+guU3'/gvV2'+guU2'
	movq	mm1, mm3			;		(using mm1)	
	
	punpcklwd	mm3, mm0		; Y1/V1'/Y0/V0'
	movq	mm7, mm4			; This wipes out the zero constant
	
	pmaddwd	mm3, const_YVmul	; ryY1+rvV1'/ryY0+rvV0'
	psrad	mm2, 8				; G1/G0

	paddd	mm5, mm6			; G3*256/G2*256		(mm6 free)
	punpcklwd	mm4, mm0		; Y1/U1'/Y0/U0'

	pmaddwd	mm4, const_YUmul	; byY1+buU1'/byY0'+buU0'
	psrad	mm5, 8				; G3/G2

	psrad	mm3, 8				; R1/R0

	punpckhwd	mm7 , mm0		; Y3/U3'/Y2/U2'
	
	psrad	mm4, 8				; B1/B0
	movq	mm6, mm3

	pmaddwd	mm7, const_YUmul	; byY3+buU3'/byY2'+buU2'
	punpckhwd	mm1, mm0		; Y3/V3'/Y2/V2'		
	
	pmaddwd	mm1, const_YVmul	; ryY3+rvV3'/ryY2+rvV2'
	punpckldq	mm3, mm2		; G0/R0

	punpckhdq	mm6, mm2		; G1/R1			(mm2 free)
	movq	mm0, mm4

	psrad	mm7, 8				; B3/B2
	
	punpckldq	mm4, const_0	; 0/B0

	punpckhdq	mm0, const_0	; 0/B1

	psrad	mm1, 8				; R3/R2

	packssdw	mm3, mm4		; 0/B0/G0/R0	(mm4 free)
	movq	mm2, mm1

	packssdw	mm6, mm0		; 0/B1/G1/R1	(mm0 free)

	packuswb mm3, mm6			; 0/B1/G1/R1/0/B0/G0/R0  (mm6 free)

	punpckldq	mm2, mm5		; G2/R2
	movq	mm4, mm7

	punpckhdq	mm1, mm5		; G3/R3 (mm5 done)

	punpckldq	mm7, const_0	; 0/B2		(change this line for alpha code)

	punpckhdq	mm4, const_0	; 0/B3		(change this line for alpha code)

	movq		mm0, mm3		
	packssdw	mm2, mm7		; 0/B2/G2/R2

	pand		mm3, mask_highd	; 0/B1/G1/R1/0/0/0/0
	packssdw	mm1, mm4		; 0/B3/G3/R3

	psrlq		mm3, 8			; 0/0/B1/G1/R1/0/0/0
	add			edx, 12

	por			mm0, mm3		; 0/0/?/?/R1/B0/G0/R0 
	packuswb    mm2, mm1		; 0/B3/G3/R3/0/B2/G2/R2

	psrlq		mm3, 32			; 0/0/0/0/0/0/B1/G1
	add			eax, 4

	movd		[edx][-12], mm0		; correct for add		
	punpcklwd	mm3, mm2		; 0/B2/0/0/G2/R2/B1/G1

	psrlq		mm2, 24			; 0/0/0/0/B3/G3/R3/0
	add			ecx, 4

	movd		[edx][-8], mm3	; correct for previous add
	psrlq		mm3, 48			; 0/0/0/0/0/0/0/B2
	
	por			mm2, mm3		; 0/0/0/0/B3/G3/R3/0
	add			ebx, 4

	movd		[edx][-4], mm2	; correct for previous add

	dec			edi
	jnz			YUVtoRGB		; Do 12 more bytes if not zero

	//emms       // commented out since it is done after the IDCT

  } // end of _asm
}

void MYCbCrA2RGBA(
  int columns,	  
  unsigned char *inY,
  unsigned char *inU,
  unsigned char *inV,
  unsigned char *inA,
  unsigned char *outRGBA)
{

	__int64		tempA;
  _asm {
	// Inits
	mov		eax, inY
	mov		ecx, inV

	mov		edi, columns
	mov		ebx, inU

	shr		edi, 2				; number of loops = cols/4 
	mov		edx, outRGBA

	mov		esi, inA

YUVAtoRGBA:
	movd	mm0, [eax]			; 0/0/0/0/Y3/Y2/Y1/Y0
	pxor	mm7, mm7			; added this in to achieve better pairing at start

	movd	mm2, [ebx]			; 0/0/0/0/U3/U2/U1/U0
	punpcklbw	mm0, mm7		; Y3/Y2/Y1/Y0

	movd	mm3, [ecx]			; 0/0/0/0/V3/V2/V1/V0
	punpcklbw	mm2, mm7		; U3/U2/U1/U0
	
	psubsw	mm2, const_sub128	; U3'/U2'/U1'/U0'
	punpcklbw	mm3, mm7		; V3/V2/V1/V0

	psubsw	mm3, const_sub128	; V3'/V2'/V1'/V0'
	movq	mm4, mm2
	
	punpcklwd	mm2, mm3		; V1'/U1'/V0'/U0'
	movq	mm1, mm0		

	pmaddwd	mm2, const_VUmul	; guU1'+gvV1'/guU0'+gvV0'
	psllw	mm1, 8				; Y3*256/Y2*256/Y1*256/Y0*256

	movq	mm6, mm1
	punpcklwd	mm1, mm7		; Y1*256/Y0*256

	punpckhwd	mm6, mm7		; Y3*256/Y2*256
	movq	mm5, mm4

	punpckhwd	mm5, mm3		; V3'/U3'/V2'/U2'
	paddd	mm2, mm1			; G1*256/G0*256		(mm1 free)

	pmaddwd	mm5, const_VUmul	; gvV3'+guU3'/gvV2'+guU2'
	movq	mm1, mm3			;		(using mm1)	
	
	punpcklwd	mm3, mm0		; Y1/V1'/Y0/V0'
	movq	mm7, mm4			; This wipes out the zero constant
	
	pmaddwd	mm3, const_YVmul	; ryY1+rvV1'/ryY0+rvV0'
	psrad	mm2, 8				; G1/G0 

	paddd	mm5, mm6			; G3*256/G2*256		(mm6 free)
	punpcklwd	mm4, mm0		; Y1/U1'/Y0/U0'

	pmaddwd	mm4, const_YUmul	; byY1+buU1'/byY0'+buU0'
	psrad	mm5, 8				; G3/G2

	psrad	mm3, 8				; R1/R0

	punpckhwd	mm7 , mm0		; Y3/U3'/Y2/U2'
	movq	mm6, mm3

	pmaddwd	mm7, const_YUmul	; byY3+buU3'/byY2'+buU2'
	punpckhwd	mm1, mm0		; Y3/V3'/Y2/V2'		

	pmaddwd	mm1, const_YVmul	; ryY3+rvV3'/ryY2+rvV2'
	punpckldq	mm3, mm2		; G0/R0

	punpckhdq	mm6, mm2		; G1/R1			(mm2 free)

	movd	mm2, [esi]			; 0/0/0/0/A3/A2/A1/A0
	psrad	mm4, 8				; B1/B0

	punpcklbw	mm2, const_0	; A3/A2/A1/A0

	psrad	mm1, 8				; R3/R2
	movq	mm0, mm4			; B1/B0

	movq	tempA, mm2
	psrad	mm7, 8				; B3/B2

	punpcklwd	mm2, const_0	; A1/A0

	punpckldq	mm4, mm2		; A0/B0

	punpckhdq	mm0, mm2		; A1/B1
	movq	mm2, mm1

	packssdw	mm3, mm4		; A0/B0/G0/R0	(mm4 free)

	packssdw	mm6, mm0		; A1/B1/G1/R1	(mm0 free)
	movq	mm4, mm7

	packuswb mm3, mm6			; A1/B1/G1/R1/A0/B0/G0/R0  (mm6 free)
	movq		mm6, tempA		; A3/A2/A1/A0

	punpckldq	mm2, mm5		; G2/R2

	movq	[edx], mm3
	punpckhdq	mm1, mm5		; G3/R3 (mm5 done)

	punpckhwd	mm6, const_0	; A3/A2

	punpckldq	mm7, mm6		; A2/B2	
	add			eax, 4

	punpckhdq	mm4, mm6		; A3/B3		
	add			ebx, 4
		
	packssdw	mm2, mm7		; A2/B2/G2/R2
	add			ecx, 4

	packssdw	mm1, mm4		; A3/B3/G3/R3
	add			edx, 16

	packuswb    mm2, mm1		; A3/B3/G3/R3/A2/B2/G2/R2
	add			esi, 4

	movq	[edx][-8], mm2		; Post-add correction on address

	dec			edi
	jnz			YUVAtoRGBA		; Do 12 more bytes if not zero

	//emms       // commented out since it is done after the IDCT

  } // end of _asm
}

void MYCbCrA2RGBALegacy(
  int columns,	  
  unsigned char *inY,
  unsigned char *inU,
  unsigned char *inV,
  unsigned char *inA,
  unsigned char *outRGBA)
{

	__int64		tempA;
  _asm {
	// Inits

	mov		eax, inY
	mov		ecx, inV

	mov		edi, columns
	mov		ebx, inU

	shr		edi, 2				; number of loops = cols/4 
	mov		edx, outRGBA

	mov		esi, inA

YUVAtoRGBA:
	movd	mm0, [eax]			; 0/0/0/0/Y3/Y2/Y1/Y0
	pxor	mm7, mm7			; added this in to achieve better pairing at start

	movd	mm2, [ebx]			; 0/0/0/0/U3/U2/U1/U0
	punpcklbw	mm0, mm7		; Y3/Y2/Y1/Y0

	movd	mm3, [ecx]			; 0/0/0/0/V3/V2/V1/V0
	punpcklbw	mm2, mm7		; U3/U2/U1/U0
	
	psubsw	mm2, const_sub128	; U3'/U2'/U1'/U0'
	punpcklbw	mm3, mm7		; V3/V2/V1/V0

	psubsw	mm3, const_sub128	; V3'/V2'/V1'/V0'
	movq	mm4, mm2
	
	punpcklwd	mm2, mm3		; V1'/U1'/V0'/U0'
	movq	mm1, mm0		

	pmaddwd	mm2, const_VUmul	; guU1'+gvV1'/guU0'+gvV0'
	psllw	mm1, 8				; Y3*256/Y2*256/Y1*256/Y0*256

	movq	mm6, mm1
	punpcklwd	mm1, mm7		; Y1*256/Y0*256
	
	punpckhwd	mm6, mm7		; Y3*256/Y2*256
	movq	mm5, mm4

	punpckhwd	mm5, mm3		; V3'/U3'/V2'/U2'
	paddd	mm2, mm1			; G1*256/G0*256		(mm1 free)

	pmaddwd	mm5, const_VUmul	; gvV3'+guU3'/gvV2'+guU2'
	movq	mm1, mm3			;		(using mm1)	
	
	punpcklwd	mm3, mm0		; Y1/V1'/Y0/V0'
	movq	mm7, mm4			; This wipes out the zero constant
	
	pmaddwd	mm3, const_YVmul	; ryY1+rvV1'/ryY0+rvV0'
	psrad	mm2, 8				; G1/G0 

	paddd	mm5, mm6			; G3*256/G2*256		(mm6 free)
	punpcklwd	mm4, mm0		; Y1/U1'/Y0/U0'

	pmaddwd	mm4, const_YUmul	; byY1+buU1'/byY0'+buU0'
	punpckhwd	mm1, mm0		; Y3/V3'/Y2/V2'		
	
	psrad	mm3, 8				; R1/R0

	punpckhwd	mm7, mm0		; Y3/U3'/Y2/U2'
	movq	mm6, mm3

	pmaddwd	mm7, const_YUmul	; byY3+buU3'/byY2'+buU2'
	psrad	mm4, 8				; B1/B0

	pmaddwd	mm1, const_YVmul	; ryY3+rvV3'/ryY2+rvV2'
	punpckldq	mm3, mm2		; G0/R0
		
	punpckhdq	mm6, mm2		; G1/R1			(mm2 free)

	movd	mm2, [esi]			; 0/0/0/0/A3/A2/A1/A0
	psrad	mm7, 8				; B3/B2	

	punpcklbw	mm2, const_0	; A3/A2/A1/A0
	
	psrad	mm1, 8				; R3/R2
	movq	mm0, mm4			; B1/B0

	movq	tempA, mm2
	psrad	mm5, 8				; G3/G2

	punpcklwd	mm2, const_0	; A1/A0

	punpckldq	mm4, mm2		; A0/B0

	punpckhdq	mm0, mm2		; A1/B1
	movq	mm2, mm1

	packssdw	mm3, mm4		; A0/B0/G0/R0	(mm4 free)

	packssdw	mm6, mm0		; A1/B1/G1/R1	(mm0 free)
	movq	mm4, mm7

	packuswb mm3, mm6			; A1/B1/G1/R1/A0/B0/G0/R0  (mm6 free)
	add			esi, 4

	movq		mm6, tempA		; A3/A2/A1/A0
	punpckldq	mm2, mm5		; G2/R2

	pxor	mm3, const_invert	; Invert all RGB values
	punpckhdq	mm1, mm5		; G3/R3 (mm5 done)

	punpckhwd	mm6, const_0	; A3/A2

	movq	[edx], mm3
	punpckldq	mm7, mm6		; A2/B2	

	punpckhdq	mm4, mm6		; A3/B3		
	add			eax, 4

	packssdw	mm2, mm7		; A2/B2/G2/R2
	add			ebx, 4
		
	packssdw	mm1, mm4		; A3/B3/G3/R3
	add			ecx, 4

	packuswb    mm2, mm1		; A3/B3/G3/R3/A2/B2/G2/R2
	add			edx, 16

	pxor		mm2, const_invert	; invert all RGB values

	movq	[edx][-8], mm2		; Post-add correction on address

	dec			edi
	jnz			YUVAtoRGBA		; Do 12 more bytes if not zero

	//emms       // commented out since it is done after the IDCT
  } // end of _asm
}

#endif // defined (_X86_)
