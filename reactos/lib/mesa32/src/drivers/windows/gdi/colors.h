/*	File name	:	colors.h
 *  Version		:	2.3
 *
 *  Header file for display driver for Mesa 2.3  under 
 *	Windows95 and WindowsNT 
 *	This file defines macros and global variables  needed
 *	for converting color format
 *
 *	Copyright (C) 1996-  Li Wei
 *  Address		:		Institute of Artificial Intelligence
 *				:			& Robotics
 *				:		Xi'an Jiaotong University
 *  Email		:		liwei@aiar.xjtu.edu.cn
 *  Web page	:		http://sun.aiar.xjtu.edu.cn
 *
 *  This file and its associations are partially based on the 
 *  Windows NT driver for Mesa, written by Mark Leaming
 *  (mark@rsinc.com).
 */

/* $Log: ddcolors.h 1997/6/14 by Li Wei(liwei@aiar.xjtu.edu.cn)
 * Macros for pixel format defined
 */

/*
 * $Log: colors.h,v $
 * Revision 1.1  2004/07/16 22:12:32  blight
 * Import Mesa-6.0.1 for use as software OpenGL renderer.
 *
 * Revision 1.1  2003/07/24 03:47:46  kschultz
 * Source code for GDI driver.
 *
 * Revision 1.3  2002/01/15 18:14:34  kschultz
 * Fixed pixel color component problem and clear code for 24-bit Windows
 *   devices.  (Jeff Lewis)
 *
 * Revision 1.2  2002/01/15 18:11:36  kschultz
 * Remove trailing CR's.   No logical changes.
 *
 * Revision 1.1.1.1  1999/08/19 00:55:42  jtg
 * Imported sources
 *
 * Revision 1.2  1999/01/03 03:08:57  brianp
 * Ted Jump's changes
 *
 * Revision 1.1  1999/01/03 03:08:12  brianp
 * Initial revision
 *
 * Revision 2.0.2  1997/4/30 15:58:00  CST by Li Wei(liwei@aiar.xjtu.edu.cn)
 * Add LUTs need for dithering
 */

/*
 * $Log: colors.h,v $
 * Revision 1.1  2004/07/16 22:12:32  blight
 * Import Mesa-6.0.1 for use as software OpenGL renderer.
 *
 * Revision 1.1  2003/07/24 03:47:46  kschultz
 * Source code for GDI driver.
 *
 * Revision 1.3  2002/01/15 18:14:34  kschultz
 * Fixed pixel color component problem and clear code for 24-bit Windows
 *   devices.  (Jeff Lewis)
 *
 * Revision 1.2  2002/01/15 18:11:36  kschultz
 * Remove trailing CR's.   No logical changes.
 *
 * Revision 1.1.1.1  1999/08/19 00:55:42  jtg
 * Imported sources
 *
 * Revision 1.2  1999/01/03 03:08:57  brianp
 * Ted Jump's changes
 *
 * Revision 1.1  1999/01/03 03:08:12  brianp
 * Initial revision
 *
 * Revision 2.0.1  1997/4/29 15:52:00  CST by Li Wei(liwei@aiar.xjtu.edu.cn)
 * Add BGR8 Macro
 */
 
/*
 * $Log: colors.h,v $
 * Revision 1.1  2004/07/16 22:12:32  blight
 * Import Mesa-6.0.1 for use as software OpenGL renderer.
 *
 * Revision 1.1  2003/07/24 03:47:46  kschultz
 * Source code for GDI driver.
 *
 * Revision 1.3  2002/01/15 18:14:34  kschultz
 * Fixed pixel color component problem and clear code for 24-bit Windows
 *   devices.  (Jeff Lewis)
 *
 * Revision 1.2  2002/01/15 18:11:36  kschultz
 * Remove trailing CR's.   No logical changes.
 *
 * Revision 1.1.1.1  1999/08/19 00:55:42  jtg
 * Imported sources
 *
 * Revision 1.2  1999/01/03 03:08:57  brianp
 * Ted Jump's changes
 *
 * Revision 1.1  1999/01/03 03:08:12  brianp
 * Initial revision
 *
 * Revision 2.0  1996/11/15 10:55:00  CST by Li Wei(liwei@aiar.xjtu.edu.cn)
 * Initial revision
 */
/* Values for wmesa->pixelformat: */

#define PF_8A8B8G8R	3	/* 32-bit TrueColor:  8-A, 8-B, 8-G, 8-R */
#define PF_8R8G8B	4	/* 32-bit TrueColor:  8-R, 8-G, 8-B */
#define PF_5R6G5B	5	/* 16-bit TrueColor:  5-R, 6-G, 5-B bits */
#define PF_DITHER8	6	/* Dithered RGB using a lookup table */
#define PF_LOOKUP	7	/* Undithered RGB using a lookup table */
#define PF_GRAYSCALE	10	/* Grayscale or StaticGray */
#define PF_BADFORMAT	11
#define PF_INDEX8		12

char ColorMap16[] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,
0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,
0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,
0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,
0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,
0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,
0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,
0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,
0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,
0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,
0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,
0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,
0x16,0x16,0x16,0x16,0x16,0x16,0x16,0x16,
0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,
0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
0x19,0x19,0x19,0x19,0x19,0x19,0x19,0x19,
0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
0x1B,0x1B,0x1B,0x1B,0x1B,0x1B,0x1B,0x1B,
0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,
0x1D,0x1D,0x1D,0x1D,0x1D,0x1D,0x1D,0x1D,
0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,
0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F};

#define BGR8(r,g,b)		(unsigned)(((BYTE)(b & 0xc0 | (g & 0xe0)>>2 | (r & 0xe0)>>5)))
#ifdef DDRAW
#define BGR16(r,g,b)	((WORD)(((BYTE)(ColorMap16[b]) | ((BYTE)(g&0xfc) << 3)) | (((WORD)(BYTE)(ColorMap16[r])) << 11)))
#else
#define BGR16(r,g,b)	((WORD)(((BYTE)(ColorMap16[b]) | ((BYTE)(ColorMap16[g]) << 5)) | (((WORD)(BYTE)(ColorMap16[r])) << 10)))
#endif
#define BGR24(r,g,b) (unsigned long)((DWORD)(((BYTE)(b)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(r))<<16)))

#define BGR32(r,g,b)	(unsigned long)((DWORD)(((BYTE)(b)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(r))<<16)))



/*
 * If pixelformat==PF_8A8B8G8R:
 */
#define PACK_8A8B8G8R( R, G, B, A )	\
	( ((A) << 24) | ((B) << 16) | ((G) << 8) | (R) )


/*
 * If pixelformat==PF_8R8G8B:
 */
#define PACK_8R8G8B( R, G, B)	 ( ((R) << 16) | ((G) << 8) | (B) )


/*
 * If pixelformat==PF_5R6G5B:
 */


#ifdef DDRAW
#define PACK_5R6G5B( R, G, B) ((WORD)(((BYTE)(ColorMap16[B]) | ((BYTE)(G&0xfc) << 3)) | (((WORD)(BYTE)(ColorMap16[R])) << 11)))
#else
#define PACK_5R6G5B( R, G, B)	((WORD)(((BYTE)(ColorMap16[B]) | ((BYTE)(ColorMap16[G]) << 5)) | (((WORD)(BYTE)(ColorMap16[R])) << 10)))
#endif
/*----------------------------------------------------------------------------

Division lookup tables.  These tables compute 0-255 divided by 51 and
modulo 51.  These tables could approximate gamma correction.

*/

char unsigned const aDividedBy51Rounded[256] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
};

char unsigned const aDividedBy51[256] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 
};

char unsigned const aModulo51[256] =
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
  38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 0, 1, 2, 3, 4, 5, 6,
  7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
  49, 50, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
  18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
  36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 0, 1, 2, 3,
  4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
  23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 0, 
};

/*----------------------------------------------------------------------------

Multiplication LUTs.  These compute 0-5 times 6 and 36.

*/

char unsigned const aTimes6[6] =
{
  0, 6, 12, 18, 24, 30
};

char unsigned const aTimes36[6] =
{
  0, 36, 72, 108, 144, 180
};


/*----------------------------------------------------------------------------

Dither matrices for 8 bit to 2.6 bit halftones.

*/

char unsigned const aHalftone16x16[256] =
{
  0, 44, 9, 41, 3, 46, 12, 43, 1, 44, 10, 41, 3, 46, 12, 43,
  34, 16, 25, 19, 37, 18, 28, 21, 35, 16, 26, 19, 37, 18, 28, 21,
  38, 6, 47, 3, 40, 9, 50, 6, 38, 7, 47, 4, 40, 9, 49, 6,
  22, 28, 13, 31, 25, 31, 15, 34, 22, 29, 13, 32, 24, 31, 15, 34,
  2, 46, 12, 43, 1, 45, 10, 42, 2, 45, 11, 42, 1, 45, 11, 42,
  37, 18, 27, 21, 35, 17, 26, 20, 36, 17, 27, 20, 36, 17, 26, 20,
  40, 8, 49, 5, 38, 7, 48, 4, 39, 8, 48, 5, 39, 7, 48, 4,
  24, 30, 15, 33, 23, 29, 13, 32, 23, 30, 14, 33, 23, 29, 14, 32,
  2, 46, 12, 43, 0, 44, 10, 41, 3, 47, 12, 44, 0, 44, 10, 41,
  37, 18, 27, 21, 35, 16, 25, 19, 37, 19, 28, 22, 35, 16, 25, 19,
  40, 9, 49, 5, 38, 7, 47, 4, 40, 9, 50, 6, 38, 6, 47, 3,
  24, 30, 15, 34, 22, 29, 13, 32, 25, 31, 15, 34, 22, 28, 13, 31,
  1, 45, 11, 42, 2, 46, 11, 42, 1, 45, 10, 41, 2, 46, 11, 43,
  36, 17, 26, 20, 36, 17, 27, 21, 35, 16, 26, 20, 36, 18, 27, 21,
  39, 8, 48, 4, 39, 8, 49, 5, 38, 7, 48, 4, 39, 8, 49, 5,
  23, 29, 14, 33, 24, 30, 14, 33, 23, 29, 13, 32, 24, 30, 14, 33,
};

char unsigned const aHalftone8x8[64] =
{
   0, 38,  9, 47,  2, 40, 11, 50,
  25, 12, 35, 22, 27, 15, 37, 24,
   6, 44,  3, 41,  8, 47,  5, 43,
  31, 19, 28, 15, 34, 21, 31, 18,
   1, 39, 11, 49,  0, 39, 10, 48,
  27, 14, 36, 23, 26, 13, 35, 23,
   7, 46,  4, 43,  7, 45,  3, 42,
  33, 20, 30, 17, 32, 19, 29, 16,
};

char unsigned const aHalftone4x4_1[16] =
{
  0, 25, 6, 31,
  38, 12, 44, 19,
  9, 35, 3, 28,
  47, 22, 41, 15
};

char unsigned const aHalftone4x4_2[16] =
{
  41, 3, 9, 28,
  35, 15, 22, 47,
  6, 25, 38, 0,
  19, 44, 31, 12
};

/***************************************************************************
  aWinGHalftoneTranslation

  Translates a 2.6 bit-per-pixel halftoned representation into the
  slightly rearranged WinG Halftone Palette.
*/

char unsigned const aWinGHalftoneTranslation[216] =
{
  0,
  29,
  30,
  31,
  32,
  249,
  33,
  34,
  35,
  36,
  37,
  38,
  39,
  40,
  41,
  42,
  43,
  44,
  45,
  46,
  47,
  48,
  49,
  50,
  51,
  52,
  53,
  54,
  55,
  56,
  250,
  250,
  57,
  58,
  59,
  251,
  60,
  61,
  62,
  63,
  64,
  65,
  66,
  67,
  68,
  69,
  70,
  71,
  72,
  73,
  74,
  75,
  76,
  77,
  78,
  79,
  80,
  81,
  82,
  83,
  84,
  85,
  86,
  87,
  88,
  89,
  250,
  90,
  91,
  92,
  93,
  94,
  95,
  96,
  97,
  98,
  99,
  100,
  101,
  102,
  103,
  104,
  105,
  106,
  107,
  108,
  109,
  110,
  111,
  227,
  112,
  113,
  114,
  115,
  116,
  117,
  118,
  119,
  151,
  120,
  121,
  122,
  123,
  124,
  228,
  125,
  126,
  229,
  133,
  162,
  135,
  131,
  132,
  137,
  166,
  134,
  140,
  130,
  136,
  143,
  138,
  139,
  174,
  141,
  142,
  177,
  129,
  144,
  145,
  146,
  147,
  148,
  149,
  150,
  157,
  152,
  153,
  154,
  155,
  156,
  192,
  158,
  159,
  160,
  161,
  196,
  163,
  164,
  165,
  127,
  199,
  167,
  168,
  169,
  170,
  171,
  172,
  173,
  207,
  175,
  176,
  210,
  178,
  179,
  180,
  181,
  182,
  183,
  184,
  185,
  186,
  187,
  188,
  189,
  190,
  191,
  224,
  193,
  194,
  195,
  252,
  252,
  197,
  198,
  128,
  253,
  252,
  200,
  201,
  202,
  203,
  204,
  205,
  206,
  230,
  208,
  209,
  231,
  211,
  212,
  213,
  214,
  215,
  216,
  217,
  218,
  219,
  220,
  221,
  222,
  254,
  223,
  232,
  225,
  226,
  255,
};
