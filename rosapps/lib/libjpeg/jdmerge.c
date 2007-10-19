/*
 * jdmerge.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains code for merged upsampling/color conversion.
 *
 * This file combines functions from jdsample.c and jdcolor.c;
 * read those files first to understand what's going on.
 *
 * When the chroma components are to be upsampled by simple replication
 * (ie, box filtering), we can save some work in color conversion by
 * calculating all the output pixels corresponding to a pair of chroma
 * samples at one time.  In the conversion equations
 *	R = Y           + K1 * Cr
 *	G = Y + K2 * Cb + K3 * Cr
 *	B = Y + K4 * Cb
 * only the Y term varies among the group of pixels corresponding to a pair
 * of chroma samples, so the rest of the terms can be calculated just once.
 * At typical sampling ratios, this eliminates half or three-quarters of the
 * multiplications needed for color conversion.
 *
 * This file currently provides implementations for the following cases:
 *	YCbCr => RGB color conversion only.
 *	Sampling ratios of 2h1v or 2h2v.
 *	No scaling needed at upsample time.
 *	Corner-aligned (non-CCIR601) sampling alignment.
 * Other special cases could be added, but in most applications these are
 * the only common cases.  (For uncommon cases we fall back on the more
 * general code in jdsample.c and jdcolor.c.)
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

#ifdef UPSAMPLE_MERGING_SUPPORTED

#ifdef HAVE_MMX_INTEL_MNEMONICS
  __int64 const1 = 0x59BA0000D24B59BA;       // Cr_r Cr_b Cr_g Cr_r
  __int64 const2 = 0x00007168E9FA0000;		 // Cb-r Cb_b Cb_g Cb_r
  __int64 const5 = 0x0000D24B59BA0000;		 // Cr_b Cr_g Cr_r Cr_b
  __int64 const6 = 0x7168E9FA00007168;		 // Cb_b Cb_g Cb_r Cb_b

  // constants for factors (One_Half/fix(x)) << 2

  __int64 const05 = 0x0001000000000001;	// Cr_r Cr_b Cr_g Cr_r
  __int64 const15 = 0x00000001FFFA0000;	// Cb-r Cb_b Cb_g Cb_r
  __int64 const45 = 0x0000000000010000;	// Cr_b Cr_g Cr_r Cr_b
  __int64 const55 = 0x0001FFFA00000001;	// Cb_b Cb_g Cb_r Cb_b
#endif

/* Private subobject */

typedef struct {
  struct jpeg_upsampler pub;	/* public fields */

  /* Pointer to routine to do actual upsampling/conversion of one row group */
  JMETHOD(void, upmethod, (j_decompress_ptr cinfo,
			   JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
			   JSAMPARRAY output_buf));

  /* Private state for YCC->RGB conversion */
  int * Cr_r_tab;		/* => table for Cr to R conversion */
  int * Cb_b_tab;		/* => table for Cb to B conversion */
  INT32 * Cr_g_tab;		/* => table for Cr to G conversion */
  INT32 * Cb_g_tab;		/* => table for Cb to G conversion */

  /* For 2:1 vertical sampling, we produce two output rows at a time.
   * We need a "spare" row buffer to hold the second output row if the
   * application provides just a one-row buffer; we also use the spare
   * to discard the dummy last row if the image height is odd.
   */
  JSAMPROW spare_row;
  boolean spare_full;		/* T if spare buffer is occupied */

  JDIMENSION out_row_width;	/* samples per output row */
  JDIMENSION rows_to_go;	/* counts rows remaining in image */
} my_upsampler;

typedef my_upsampler * my_upsample_ptr;

#define SCALEBITS	16	/* speediest right-shift on some machines */
#define ONE_HALF	((INT32) 1 << (SCALEBITS-1))
#define FIX(x)		((INT32) ((x) * (1L<<SCALEBITS) + 0.5))


/*
 * Initialize tables for YCC->RGB colorspace conversion.
 * This is taken directly from jdcolor.c; see that file for more info.
 */

LOCAL(void)
build_ycc_rgb_table (j_decompress_ptr cinfo)
{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;
  int i;
  INT32 x;
  SHIFT_TEMPS

  upsample->Cr_r_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  upsample->Cb_b_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  upsample->Cr_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));
  upsample->Cb_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));

  for (i = 0, x = -CENTERJSAMPLE; i <= MAXJSAMPLE; i++, x++) {
    /* i is the actual input pixel value, in the range 0..MAXJSAMPLE */
    /* The Cb or Cr value we are thinking of is x = i - CENTERJSAMPLE */
    /* Cr=>R value is nearest int to 1.40200 * x */
    upsample->Cr_r_tab[i] = (int)
		    RIGHT_SHIFT(FIX(1.40200) * x + ONE_HALF, SCALEBITS);
    /* Cb=>B value is nearest int to 1.77200 * x */
    upsample->Cb_b_tab[i] = (int)
		    RIGHT_SHIFT(FIX(1.77200) * x + ONE_HALF, SCALEBITS);
    /* Cr=>G value is scaled-up -0.71414 * x */
    upsample->Cr_g_tab[i] = (- FIX(0.71414)) * x;
    /* Cb=>G value is scaled-up -0.34414 * x */
    /* We also add in ONE_HALF so that need not do it in inner loop */
    upsample->Cb_g_tab[i] = (- FIX(0.34414)) * x + ONE_HALF;
  }
}


/*
 * Initialize for an upsampling pass.
 */

METHODDEF(void)
start_pass_merged_upsample (j_decompress_ptr cinfo)
{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;

  /* Mark the spare buffer empty */
  upsample->spare_full = FALSE;
  /* Initialize total-height counter for detecting bottom of image */
  upsample->rows_to_go = cinfo->output_height;
}


/*
 * Control routine to do upsampling (and color conversion).
 *
 * The control routine just handles the row buffering considerations.
 */

METHODDEF(void)
merged_2v_upsample (j_decompress_ptr cinfo,
		    JSAMPIMAGE input_buf, JDIMENSION *in_row_group_ctr,
		    JDIMENSION in_row_groups_avail,
		    JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
		    JDIMENSION out_rows_avail)
/* 2:1 vertical sampling case: may need a spare row. */
{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;
  JSAMPROW work_ptrs[2];
  JDIMENSION num_rows;		/* number of rows returned to caller */

  if (upsample->spare_full) {
    /* If we have a spare row saved from a previous cycle, just return it. */
    jcopy_sample_rows(& upsample->spare_row, 0, output_buf + *out_row_ctr, 0,
		      1, upsample->out_row_width);
    num_rows = 1;
    upsample->spare_full = FALSE;
  } else {
    /* Figure number of rows to return to caller. */
    num_rows = 2;
    /* Not more than the distance to the end of the image. */
    if (num_rows > upsample->rows_to_go)
      num_rows = upsample->rows_to_go;
    /* And not more than what the client can accept: */
    out_rows_avail -= *out_row_ctr;
    if (num_rows > out_rows_avail)
      num_rows = out_rows_avail;
    /* Create output pointer array for upsampler. */
    work_ptrs[0] = output_buf[*out_row_ctr];
    if (num_rows > 1) {
      work_ptrs[1] = output_buf[*out_row_ctr + 1];
    } else {
      work_ptrs[1] = upsample->spare_row;
      upsample->spare_full = TRUE;
    }
    /* Now do the upsampling. */
    (*upsample->upmethod) (cinfo, input_buf, *in_row_group_ctr, work_ptrs);
  }

  /* Adjust counts */
  *out_row_ctr += num_rows;
  upsample->rows_to_go -= num_rows;
  /* When the buffer is emptied, declare this input row group consumed */
  if (! upsample->spare_full)
    (*in_row_group_ctr)++;
}


METHODDEF(void)
merged_1v_upsample (j_decompress_ptr cinfo,
		    JSAMPIMAGE input_buf, JDIMENSION *in_row_group_ctr,
		    JDIMENSION in_row_groups_avail,
		    JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
		    JDIMENSION out_rows_avail)
/* 1:1 vertical sampling case: much easier, never need a spare row. */
{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;

  /* Just do the upsampling. */
  (*upsample->upmethod) (cinfo, input_buf, *in_row_group_ctr,
			 output_buf + *out_row_ctr);
  /* Adjust counts */
  (*out_row_ctr)++;
  (*in_row_group_ctr)++;
}


/*
 * These are the routines invoked by the control routines to do
 * the actual upsampling/conversion.  One row group is processed per call.
 *
 * Note: since we may be writing directly into application-supplied buffers,
 * we have to be honest about the output width; we can't assume the buffer
 * has been rounded up to an even width.
 */


/*
 * Upsample and color convert for the case of 2:1 horizontal and 1:1 vertical.
 */

METHODDEF(void)
h2v1_merged_upsample (j_decompress_ptr cinfo,
		      JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
		      JSAMPARRAY output_buf)
{


 my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;
  register int y, cred, cgreen, cblue;
  int cb, cr;
  register JSAMPROW outptr;
  JSAMPROW inptr0, inptr1, inptr2;
  JDIMENSION col;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  int * Crrtab = upsample->Cr_r_tab;
  int * Cbbtab = upsample->Cb_b_tab;
  INT32 * Crgtab = upsample->Cr_g_tab;
  INT32 * Cbgtab = upsample->Cb_g_tab;
  SHIFT_TEMPS

  inptr0 = input_buf[0][in_row_group_ctr];
  inptr1 = input_buf[1][in_row_group_ctr];
  inptr2 = input_buf[2][in_row_group_ctr];
  outptr = output_buf[0];
  /* Loop for each pair of output pixels */
  for (col = cinfo->output_width >> 1; col > 0; col--) {
    /* Do the chroma part of the calculation */
    cb = GETJSAMPLE(*inptr1++);
    cr = GETJSAMPLE(*inptr2++);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    /* Fetch 2 Y values and emit 2 pixels */
    y  = GETJSAMPLE(*inptr0++);
    outptr[RGB_RED] =   range_limit[y + cred];
    outptr[RGB_GREEN] = range_limit[y + cgreen];
    outptr[RGB_BLUE] =  range_limit[y + cblue];
    outptr += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr0++);
    outptr[RGB_RED] =   range_limit[y + cred];
    outptr[RGB_GREEN] = range_limit[y + cgreen];
    outptr[RGB_BLUE] =  range_limit[y + cblue];
    outptr += RGB_PIXELSIZE;
  }
  /* If image width is odd, do the last output column separately */
  if (cinfo->output_width & 1) {
    cb = GETJSAMPLE(*inptr1);
    cr = GETJSAMPLE(*inptr2);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    y  = GETJSAMPLE(*inptr0);
    outptr[RGB_RED] =   range_limit[y + cred];
    outptr[RGB_GREEN] = range_limit[y + cgreen];
    outptr[RGB_BLUE] =  range_limit[y + cblue];
  }
}


/*
 * Upsample and color convert for the case of 2:1 horizontal and 2:1 vertical.
 */

#ifdef HAVE_MMX_INTEL_MNEMONICS
__inline METHODDEF(void)
h2v2_merged_upsample_orig (j_decompress_ptr cinfo,
		      JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
		      JSAMPARRAY output_buf);
__inline METHODDEF(void)
h2v2_merged_upsample_mmx (j_decompress_ptr cinfo,
		      JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
		      JSAMPARRAY output_buf);
#endif

METHODDEF(void)
h2v2_merged_upsample (j_decompress_ptr cinfo,
		      JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
		      JSAMPARRAY output_buf);

#ifdef HAVE_MMX_INTEL_MNEMONICS
METHODDEF(void)
h2v2_merged_upsample (j_decompress_ptr cinfo,
		      JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
		      JSAMPARRAY output_buf)
{
if (MMXAvailable && (cinfo->image_width >= 8))
	h2v2_merged_upsample_mmx (cinfo, input_buf, in_row_group_ctr, output_buf);
else
	h2v2_merged_upsample_orig (cinfo, input_buf, in_row_group_ctr, output_buf);

}

__inline METHODDEF(void)
h2v2_merged_upsample_orig (j_decompress_ptr cinfo,
		      JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
		      JSAMPARRAY output_buf)
{

  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;
  register int y, cred, cgreen, cblue;
  int cb, cr;
  register JSAMPROW outptr0, outptr1;
  JSAMPROW inptr00, inptr01, inptr1, inptr2;
  JDIMENSION col;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  int * Crrtab = upsample->Cr_r_tab;
  int * Cbbtab = upsample->Cb_b_tab;
  INT32 * Crgtab = upsample->Cr_g_tab;
  INT32 * Cbgtab = upsample->Cb_g_tab;
  SHIFT_TEMPS

  inptr00 = input_buf[0][in_row_group_ctr*2];
  inptr01 = input_buf[0][in_row_group_ctr*2 + 1];
  inptr1 = input_buf[1][in_row_group_ctr];
  inptr2 = input_buf[2][in_row_group_ctr];
  outptr0 = output_buf[0];
  outptr1 = output_buf[1];
  /* Loop for each group of output pixels */
  for (col = cinfo->output_width >> 1; col > 0; col--) {
    /* Do the chroma part of the calculation */
    cb = GETJSAMPLE(*inptr1++);
    cr = GETJSAMPLE(*inptr2++);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    /* Fetch 4 Y values and emit 4 pixels */
    y  = GETJSAMPLE(*inptr00++);
    outptr0[RGB_RED] =   range_limit[y + cred];
    outptr0[RGB_GREEN] = range_limit[y + cgreen];
    outptr0[RGB_BLUE] =  range_limit[y + cblue];
    outptr0 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr00++);
    outptr0[RGB_RED] =   range_limit[y + cred];
    outptr0[RGB_GREEN] = range_limit[y + cgreen];
    outptr0[RGB_BLUE] =  range_limit[y + cblue];
    outptr0 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr01++);
    outptr1[RGB_RED] =   range_limit[y + cred];
    outptr1[RGB_GREEN] = range_limit[y + cgreen];
    outptr1[RGB_BLUE] =  range_limit[y + cblue];
    outptr1 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr01++);
    outptr1[RGB_RED] =   range_limit[y + cred];
    outptr1[RGB_GREEN] = range_limit[y + cgreen];
    outptr1[RGB_BLUE] =  range_limit[y + cblue];
    outptr1 += RGB_PIXELSIZE;
  }
  /* If image width is odd, do the last output column separately */
  if (cinfo->output_width & 1) {
    cb = GETJSAMPLE(*inptr1);
    cr = GETJSAMPLE(*inptr2);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    y  = GETJSAMPLE(*inptr00);
    outptr0[RGB_RED] =   range_limit[y + cred];
    outptr0[RGB_GREEN] = range_limit[y + cgreen];
    outptr0[RGB_BLUE] =  range_limit[y + cblue];
    y  = GETJSAMPLE(*inptr01);
    outptr1[RGB_RED] =   range_limit[y + cred];
    outptr1[RGB_GREEN] = range_limit[y + cgreen];
    outptr1[RGB_BLUE] =  range_limit[y + cblue];
  }
}

/*
 * Upsample and color convert for the case of 2:1 horizontal and 2:1 vertical.
 */
__inline METHODDEF(void)
h2v2_merged_upsample_mmx (j_decompress_ptr cinfo,
		      JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
		      JSAMPARRAY output_buf)
{
	// added for MMX
  __int64 const128 = 0x0080008000800080;
  __int64 empty = 0x0000000000000000;
  __int64 davemask = 0x0000FFFFFFFF0000;
  ////////////////////////////////

  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;
  register int y, cred, cgreen, cblue;
  int cb, cr;
  register JSAMPROW outptr0, outptr1;
  JSAMPROW inptr00, inptr01, inptr1, inptr2;
  JDIMENSION col;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  int * Crrtab = upsample->Cr_r_tab;
  int * Cbbtab = upsample->Cb_b_tab;
  INT32 * Crgtab = upsample->Cr_g_tab;
  INT32 * Cbgtab = upsample->Cb_g_tab;
  SHIFT_TEMPS


  // Added for MMX
  register int width = cinfo->image_width;
  int cols = cinfo->output_width;
  int cols_asm = (cols >> 3);
  int diff = cols - (cols_asm<<3);
  int cols_asm_copy = cols_asm;

 ///////////////////////////////////////

  inptr00 = input_buf[0][in_row_group_ctr*2];
  inptr01 = input_buf[0][in_row_group_ctr*2 + 1];
  inptr1 = input_buf[1][in_row_group_ctr];
  inptr2 = input_buf[2][in_row_group_ctr];
  outptr0 = output_buf[0];
  outptr1 = output_buf[1];
  /* Loop for each group of output pixels */


  _asm
  {
	  mov esi, inptr00

	  mov eax, inptr01

	  mov ebx, inptr2

	  mov ecx, inptr1

	  mov edi, outptr0

	  mov edx, outptr1

do_next16:

	  movd mm0, [ebx]			; Cr7 Cr6.....Cr1 Cr0

	  pxor mm6, mm6

	  punpcklbw mm0, mm0		; Cr3 Cr3 Cr2 Cr2 Cr1 Cr1 Cr0 Cr0

	  movq mm7, const128

	  punpcklwd mm0, mm0		; Cr1 Cr1 Cr1 Cr1 Cr0 Cr0 Cr0 Cr0

	  movq mm4, mm0

	  punpcklbw mm0, mm6		; Cr0 Cr0 Cr0 Cr0

	  psubsw mm0, mm7			; Cr0 - 128:Cr0-128:Cr0-128:Cr0 -128

	  movd mm1, [ecx]			; Cb7 Cb6...... Cb1 Cb0

	  psllw mm0, 2				; left shift by 2 bits

	  punpcklbw mm1, mm1		; Cb3 Cb3 Cb2 Cb2 Cb1 Cb1 Cb0 Cb0

	  paddsw mm0, const05		; add (one_half/fix(x)) << 2

	  punpcklwd mm1, mm1		; Cb1 Cb1 Cb1 Cb1 Cb0 Cb0 Cb0 Cb0

	  movq mm5, mm1

	  pmulhw mm0, const1		; multiply by (fix(x) >> 1)

	  punpcklbw mm1, mm6		; Cb0 Cb0 Cb0 Cb0

	  punpckhbw mm4, mm6		; Cr1 Cr1 Cr1 Cr1

	  psubsw mm1, mm7			; Cb0 - 128:Cb0-128:Cb0-128:Cb0 -128

	  punpckhbw mm5, mm6		; Cb1 Cb1 Cb1 Cb1

	  psllw mm1, 2				; left shift by 2 bits

	  paddsw mm1, const15		; add (one_half/fix(x)) << 2

	  psubsw mm4, mm7			; Cr1 - 128:Cr1-128:Cr1-128:Cr1 -128

	  psubsw mm5, mm7			; Cb1 - 128:Cb1-128:Cb1-128:Cb1 -128

	  pmulhw mm1, const2		; multiply by (fix(x) >> 1)

	  psllw mm4, 2				; left shift by 2 bits

	  psllw mm5, 2				; left shift by 2 bits

	  paddsw mm4, const45		; add (one_half/fix(x)) << 2

	  movd mm7, [esi]			;  Y13 Y12 Y9 Y8 Y5 Y4 Y1 Y0

	  pmulhw mm4, const5		; multiply by (fix(x) >> 1)

	  movq mm6, mm7

	  punpcklbw mm7, mm7		; Y5 Y5 Y4 Y4 Y1 Y1 Y0 Y0

	  paddsw mm5, const55		; add (one_half/fix(x)) << 2

	  paddsw  mm0, mm1			; cred0 cbl0 cgr0 cred0

	  movq mm1, mm7

	  pmulhw mm5, const6		; multiply by (fix(x) >> 1)

	  movq	mm2, mm0			; cred0 cbl0 cgr0 cred0

	  punpcklwd mm7, mm6		; Y5 Y4 Y1 Y1 Y1 Y0 Y0 Y0

	  pand mm2, davemask		; 0 cbl0 cgr0 0

	  psrlq mm1, 16				; 0 0 Y5 Y5 Y4 Y4 Y1 Y1

	  psrlq	mm2, 16				; 0 0 cbl0 cgr0

	  punpcklbw mm7, empty		; Y1 Y0 Y0 Y0

	  paddsw mm4, mm5			; cbl1 cgr1 cred1 cbl1

	  movq	mm3, mm4			; cbl1 cgr1 cred1 cbl1

	  pand	mm3, davemask		; 0 cgr1 cred1 0

	  paddsw mm7, mm0			; r1 b0 g0 r0

	  psllq	mm3, 16				; cgr1 cred1 0 0

	  movq mm6, mm1				; 0 0 Y5 Y5 Y4 Y4 Y1 Y1

	  por	mm2, mm3			; cgr1 cred1 cbl0 cgr0

	  punpcklbw mm6, empty		; Y4 Y4 Y1 Y1

	  movd mm3, [eax]			; Y15 Y14 Y11 Y10 Y7 Y6 Y3 Y2

	  paddsw mm6, mm2			; g4 r4 b1 g1

	  packuswb mm7, mm6			; g4 r4 b1 g1 r1 b0 g0 r0

	  movq mm6, mm3				; Y15 Y14 Y11 Y10 Y7 Y6 Y3 Y2

	  punpcklbw mm3, mm3		; Y7 Y7 Y6 Y6 Y3 Y3 Y2 Y2

	  movq [edi], mm7			; move to memory g4 r4 b1 g1 r1 b0 g0 r0

	  movq mm5, mm3				; Y7 Y7 Y6 Y6 Y3 Y3 Y2 Y2

	  punpcklwd mm3, mm6		; X X X X Y3 Y2 Y2 Y2

	  punpcklbw mm3, empty		; Y3 Y2 Y2 Y2

	  psrlq mm5, 16				; 0 0 Y7 Y7 Y6 Y6 Y3 Y3

	  paddsw mm3, mm0			; r3 b2 g2 r2

	  movq mm6, mm5				; 0 0 Y7 Y7 Y6 Y6 Y3 Y3

	  movq mm0, mm1				; 0 0 Y5 Y5 Y4 Y4 Y1 Y1

	  punpckldq mm6, mm6		; X X X X Y6 Y6 Y3 Y3

	  punpcklbw mm6, empty		; Y6 Y6 Y3 Y3

	  psrlq mm1, 24				; 0 0 0 0 0 Y5 Y5 Y4

	  paddsw mm6, mm2			; g6 r6 b3 g3

	  packuswb mm3, mm6			; g6 r6 b3 g3 r3 b2 g2 r2

	  movq mm2, mm5				; 0 0 Y7 Y7 Y6 Y6 Y3 Y3

	  psrlq mm0, 32				; 0 0 0 0 0 0 Y5 Y5

	  movq [edx], mm3			; move to memory g6 r6 b3 g3 r3 b2 g2 r2

	  punpcklwd mm1, mm0		; X X X X Y5 Y5 Y5 Y4

	  psrlq mm5, 24				; 0 0 0 0 0 Y7 Y7 Y6

	  movd mm0, [ebx]			; Cr9 Cr8.....Cr3 Cr2

	  psrlq mm2, 32	   			; 0 0 0 0 0 0 Y7 Y7

	  psrlq	mm0, 16

	  punpcklbw mm1, empty		; Y5 Y5 Y5 Y4

	  punpcklwd mm5, mm2		; X X X X Y7 Y7 Y7 Y6

	  paddsw mm1, mm4			; b5 g5 r5 b4

	  punpcklbw mm5, empty		; Y7 Y7 Y7 Y6

	  pxor mm6, mm6				; clear mm6 registr

	  punpcklbw mm0, mm0		; X X X X Cr3 Cr3 Cr2 Cr2

	  paddsw mm5, mm4			; b7 g7 r7 b6

	  punpcklwd mm0, mm0		; Cr3 Cr3 Cr3 Cr3 Cr2 Cr2 Cr2 Cr2

	  movq mm4, mm0

	  movd mm3, [ecx]			; Cb9 Cb8...... Cb3 Cb2

	  punpcklbw mm0, mm6		; Cr2 Cr2 Cr2 Cr2

	  psrlq	mm3, 16

	  psubsw mm0, const128		; Cr2 - 128:Cr2-128:Cr2-128:Cr2 -128

	  punpcklbw mm3, mm3		; X X X X Cb3 Cb3 Cb2 Cb2

	  psllw mm0, 2				; left shift by 2 bits

	  paddsw mm0, const05		; add (one_half/fix(x)) << 2

	  punpcklwd mm3, mm3		; Cb3 Cb3 Cb3 Cb3 Cb2 Cb2 Cb2 Cb2

	  movq mm7, mm3

	  pmulhw mm0, const1		; multiply by (fix(x) >> 1)

	  punpcklbw mm3, mm6		; Cb2 Cb2 Cb2 Cb2

	  psubsw mm3, const128		; Cb0 - 128:Cb0-128:Cb0-128:Cb0 -128

	  punpckhbw mm4, mm6		; Cr3 Cr3 Cr3 Cr3

	  psllw mm3, 2				; left shift by 2 bits

	  paddsw mm3, const15		; add (one_half/fix(x)) << 2

	  punpckhbw mm7, mm6		; Cb3 Cb3 Cb3 Cb3

	  pmulhw mm3, const2		; multiply by (fix(x) >> 1)

	  psubsw mm7, const128		; Cb3 - 128:Cb3-128:Cb3-128:Cb3 -128

	  paddsw  mm0, mm3			; cred2 cbl2 cgr2 cred2

	  psllw mm7, 2				; left shift by 2 bits

	  psubsw mm4, const128		; Cr3 - 128:Cr3-128:Cr3-128:Cr3 -128

	  movd mm3, [esi+4]			;  Y21 Y20 Y17 Y16 Y13 Y12 Y9 Y8

	  psllw mm4, 2				; left shift by 2 bits

	  paddsw mm7, const55		; add (one_half/fix(x)) << 2

	  movq mm6, mm3				;  Y21 Y20 Y17 Y16 Y13 Y12 Y9 Y8

	  movq	mm2, mm0

	  pand mm2, davemask

	  punpcklbw mm3, mm3		; Y13 Y13 Y12 Y12 Y9 Y9 Y8 Y8

	  psrlq	mm2, 16

	  paddsw mm4, const45		; add (one_half/fix(x)) << 2

	  punpcklwd mm3, mm6		; X X X X Y9 Y8 Y8 Y8

	  pmulhw mm4, const5		; multiply by (fix(x) >> 1)

	  pmulhw mm7, const6		; multiply by (fix(x) >> 1)

	  punpcklbw mm3, empty		; Y9 Y8 Y8 Y8

	  paddsw mm4, mm7			; cbl3 cgr3 cred3 cbl3

	  paddsw mm3, mm0			; r9 b8 g8 r8

	  movq	mm7, mm4

	  packuswb mm1, mm3			; r9 b8 g8 r8 b5 g5 r5 b4

	  movd mm3, [eax+4]			; Y23 Y22 Y19 Y18 Y15 Y14 Y11 Y10

	  pand	mm7, davemask

	  psrlq mm6, 8				; 0 Y21 Y20 Y17 Y16 Y13 Y12 Y9

	  psllq	mm7, 16

	  movq [edi+8], mm1			; move to memory r9 b8 g8 r8 b5 g5 r5 b4

	  por	mm2, mm7

	  movq mm7, mm3				; Y23 Y22 Y19 Y18 Y15 Y14 Y11 Y10

	  punpcklbw mm3, mm3		; X X X X Y11 Y11 Y10 Y10

	  pxor mm1, mm1

	  punpcklwd mm3, mm7		; X X X X Y11 Y10 Y10 Y10

	  punpcklbw mm3, mm1		; Y11 Y10 Y10 Y10

	  psrlq mm7, 8				; 0 Y23 Y22 Y19 Y18 Y15 Y14 Y11

	  paddsw mm3, mm0			; r11 b10 g10 r10

	  movq mm0, mm7				; 0 Y23 Y22 Y19 Y18 Y15 Y14 Y11

	  packuswb mm5, mm3			; r11 b10 g10 r10 b7 g7 r7 b6

	  punpcklbw mm7, mm7		; X X X X Y14 Y14 Y11 Y11

	  movq [edx+8], mm5			; move to memory r11 b10 g10 r10 b7 g7 r7 b6

	  movq mm3, mm6				; 0 Y21 Y20 Y17 Y16 Y13 Y12 Y9

	  punpcklbw mm6, mm6		; X X X X Y12 Y12 Y9 Y9

	  punpcklbw mm7, mm1		; Y14 Y14 Y11 Y11

	  punpcklbw mm6, mm1		; Y12 Y12 Y9 Y9

	  paddsw mm7, mm2			; g14 r14 b11 g11

	  paddsw mm6, mm2			; g12 r12 b9 g9

	  psrlq mm3, 8				; 0 0 Y21 Y20 Y17 Y16 Y13 Y12

	  movq mm1, mm3				; 0 0 Y21 Y20 Y17 Y16 Y13 Y12

	  punpcklbw mm3, mm3		; X X X X Y13 Y13 Y12 Y12

	  add esi, 8

	  psrlq mm3, 16				; X X X X X X Y13 Y13 modified on 09/24

	  punpcklwd mm1, mm3		; X X X X Y13 Y13 Y13 Y12

	  add eax, 8

	  psrlq mm0, 8				; 0 0 Y23 Y22 Y19 Y18 Y15 Y14

	  punpcklbw mm1, empty		; Y13 Y13 Y13 Y12

	  movq mm5, mm0				; 0 0 Y23 Y22 Y19 Y18 Y15 Y14

	  punpcklbw mm0, mm0		; X X X X Y15 Y15 Y14 Y14

	  paddsw mm1, mm4			; b13 g13 r13 b12

	  psrlq mm0, 16				; X X X X X X Y15 Y15

	  add edi, 24

	  punpcklwd mm5, mm0		; X X X X Y15 Y15 Y15 Y14

	  packuswb mm6, mm1			; b13 g13 r13 b12 g12 r12 b9 g9

	  add edx, 24

	  punpcklbw mm5, empty		; Y15 Y15 Y15 Y14

	  add ebx, 4

	  paddsw mm5, mm4			; b15 g15 r15 b14

	  movq [edi-8], mm6		; move to memory b13 g13 r13 b12 g12 r12 b9 g9

	  packuswb mm7, mm5			; b15 g15 r15 b14 g14 r14 b11 g11

	  add ecx, 4

	  movq [edx-8], mm7		; move to memory b15 g15 r15 b14 g14 r14 b11 g11

	  dec cols_asm

	  jnz do_next16

	  EMMS

	  }


  inptr1 += (cols_asm_copy<<2);

  inptr2 += (cols_asm_copy<<2);

  inptr00 += (cols_asm_copy<<3);

  inptr01 += (cols_asm_copy<<3);

  outptr0 += cols_asm_copy*24;

  outptr1 += cols_asm_copy*24;

  //for (col = cinfo->output_width >> 1; col > 0; col--) {
      /* Do the chroma part of the calculation */
    /*cb = GETJSAMPLE(*inptr1++);
    cr = GETJSAMPLE(*inptr2++);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];*/
    /* Fetch 4 Y values and emit 4 pixels */
    /*y  = GETJSAMPLE(*inptr00++);
    outptr0[RGB_RED] =   range_limit[y + cred];
    outptr0[RGB_GREEN] = range_limit[y + cgreen];
    outptr0[RGB_BLUE] =  range_limit[y + cblue];
    outptr0 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr00++);
    outptr0[RGB_RED] =   range_limit[y + cred];
    outptr0[RGB_GREEN] = range_limit[y + cgreen];
    outptr0[RGB_BLUE] =  range_limit[y + cblue];
    outptr0 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr01++);
    outptr1[RGB_RED] =   range_limit[y + cred];
    outptr1[RGB_GREEN] = range_limit[y + cgreen];
    outptr1[RGB_BLUE] =  range_limit[y + cblue];
    outptr1 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr01++);
    outptr1[RGB_RED] =   range_limit[y + cred];
    outptr1[RGB_GREEN] = range_limit[y + cgreen];
    outptr1[RGB_BLUE] =  range_limit[y + cblue];
    outptr1 += RGB_PIXELSIZE;
  }	  */


  for (col = diff >> 1; col > 0; col--) {
      /* Do the chroma part of the calculation */
    cb = GETJSAMPLE(*inptr1++);
    cr = GETJSAMPLE(*inptr2++);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    /* Fetch 4 Y values and emit 4 pixels */
    y  = GETJSAMPLE(*inptr00++);
    outptr0[RGB_RED] =   range_limit[y + cred];
    outptr0[RGB_GREEN] = range_limit[y + cgreen];
    outptr0[RGB_BLUE] =  range_limit[y + cblue];
    outptr0 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr00++);
    outptr0[RGB_RED] =   range_limit[y + cred];
    outptr0[RGB_GREEN] = range_limit[y + cgreen];
    outptr0[RGB_BLUE] =  range_limit[y + cblue];
    outptr0 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr01++);
    outptr1[RGB_RED] =   range_limit[y + cred];
    outptr1[RGB_GREEN] = range_limit[y + cgreen];
    outptr1[RGB_BLUE] =  range_limit[y + cblue];
    outptr1 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr01++);
    outptr1[RGB_RED] =   range_limit[y + cred];
    outptr1[RGB_GREEN] = range_limit[y + cgreen];
    outptr1[RGB_BLUE] =  range_limit[y + cblue];
    outptr1 += RGB_PIXELSIZE;
  }


  /* If image width is odd, do the last output column separately */
  //if (cinfo->output_width & 1) {
  if (diff & 1) {
    cb = GETJSAMPLE(*inptr1);
    cr = GETJSAMPLE(*inptr2);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    y  = GETJSAMPLE(*inptr00);
    outptr0[RGB_RED] =   range_limit[y + cred];
    outptr0[RGB_GREEN] = range_limit[y + cgreen];
    outptr0[RGB_BLUE] =  range_limit[y + cblue];
    y  = GETJSAMPLE(*inptr01);
    outptr1[RGB_RED] =   range_limit[y + cred];
    outptr1[RGB_GREEN] = range_limit[y + cgreen];
    outptr1[RGB_BLUE] =  range_limit[y + cblue];
  }
}
#else


METHODDEF(void)
h2v2_merged_upsample (j_decompress_ptr cinfo,
		      JSAMPIMAGE input_buf, JDIMENSION in_row_group_ctr,
		      JSAMPARRAY output_buf)
{
  my_upsample_ptr upsample = (my_upsample_ptr) cinfo->upsample;
  register int y, cred, cgreen, cblue;
  int cb, cr;
  register JSAMPROW outptr0, outptr1;
  JSAMPROW inptr00, inptr01, inptr1, inptr2;
  JDIMENSION col;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  int * Crrtab = upsample->Cr_r_tab;
  int * Cbbtab = upsample->Cb_b_tab;
  INT32 * Crgtab = upsample->Cr_g_tab;
  INT32 * Cbgtab = upsample->Cb_g_tab;
  SHIFT_TEMPS

  inptr00 = input_buf[0][in_row_group_ctr*2];
  inptr01 = input_buf[0][in_row_group_ctr*2 + 1];
  inptr1 = input_buf[1][in_row_group_ctr];
  inptr2 = input_buf[2][in_row_group_ctr];
  outptr0 = output_buf[0];
  outptr1 = output_buf[1];
  /* Loop for each group of output pixels */
  for (col = cinfo->output_width >> 1; col > 0; col--) {
    /* Do the chroma part of the calculation */
    cb = GETJSAMPLE(*inptr1++);
    cr = GETJSAMPLE(*inptr2++);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    /* Fetch 4 Y values and emit 4 pixels */
    y  = GETJSAMPLE(*inptr00++);
    outptr0[RGB_RED] =   range_limit[y + cred];
    outptr0[RGB_GREEN] = range_limit[y + cgreen];
    outptr0[RGB_BLUE] =  range_limit[y + cblue];
    outptr0 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr00++);
    outptr0[RGB_RED] =   range_limit[y + cred];
    outptr0[RGB_GREEN] = range_limit[y + cgreen];
    outptr0[RGB_BLUE] =  range_limit[y + cblue];
    outptr0 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr01++);
    outptr1[RGB_RED] =   range_limit[y + cred];
    outptr1[RGB_GREEN] = range_limit[y + cgreen];
    outptr1[RGB_BLUE] =  range_limit[y + cblue];
    outptr1 += RGB_PIXELSIZE;
    y  = GETJSAMPLE(*inptr01++);
    outptr1[RGB_RED] =   range_limit[y + cred];
    outptr1[RGB_GREEN] = range_limit[y + cgreen];
    outptr1[RGB_BLUE] =  range_limit[y + cblue];
    outptr1 += RGB_PIXELSIZE;
  }
  /* If image width is odd, do the last output column separately */
  if (cinfo->output_width & 1) {
    cb = GETJSAMPLE(*inptr1);
    cr = GETJSAMPLE(*inptr2);
    cred = Crrtab[cr];
    cgreen = (int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];
    y  = GETJSAMPLE(*inptr00);
    outptr0[RGB_RED] =   range_limit[y + cred];
    outptr0[RGB_GREEN] = range_limit[y + cgreen];
    outptr0[RGB_BLUE] =  range_limit[y + cblue];
    y  = GETJSAMPLE(*inptr01);
    outptr1[RGB_RED] =   range_limit[y + cred];
    outptr1[RGB_GREEN] = range_limit[y + cgreen];
    outptr1[RGB_BLUE] =  range_limit[y + cblue];
  }
}
#endif


/*
 * Module initialization routine for merged upsampling/color conversion.
 *
 * NB: this is called under the conditions determined by use_merged_upsample()
 * in jdmaster.c.  That routine MUST correspond to the actual capabilities
 * of this module; no safety checks are made here.
 */

GLOBAL(void)
jinit_merged_upsampler (j_decompress_ptr cinfo)
{
  my_upsample_ptr upsample;

  upsample = (my_upsample_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_upsampler));
  cinfo->upsample = (struct jpeg_upsampler *) upsample;
  upsample->pub.start_pass = start_pass_merged_upsample;
  upsample->pub.need_context_rows = FALSE;

  upsample->out_row_width = cinfo->output_width * cinfo->out_color_components;

  if (cinfo->max_v_samp_factor == 2) {
    upsample->pub.upsample = merged_2v_upsample;
    upsample->upmethod = h2v2_merged_upsample;
    /* Allocate a spare row buffer */
    upsample->spare_row = (JSAMPROW)
      (*cinfo->mem->alloc_large) ((j_common_ptr) cinfo, JPOOL_IMAGE,
		(size_t) (upsample->out_row_width * SIZEOF(JSAMPLE)));
  } else {
    upsample->pub.upsample = merged_1v_upsample;
    upsample->upmethod = h2v1_merged_upsample;
    /* No spare row needed */
    upsample->spare_row = NULL;
  }

  build_ycc_rgb_table(cinfo);
}

#endif /* UPSAMPLE_MERGING_SUPPORTED */
