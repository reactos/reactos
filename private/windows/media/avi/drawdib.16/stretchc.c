/*
 * StretchC.C
 *
 * StretchBlt for DIBs
 *
 * C version of stretch.asm: StretchDIB optimised for AVI.
 *
 * NOTES
 *	- does not handle mirroring in x or y
 *	- does not handle pixel translation
 *	- will not work in place.
 *
 * AUTHOR
 *      C version by Geraint Davies
 */

#include <windows.h>
#include "drawdibi.h"
#include "stretch.h"

/* Outline:
 *
 * we select a y-stretching function depending on the ratio (eg 1:N or N:1).
 * it copies scanlines from source to destination, duplicating or omitting
 * scanlines as necessary to fit the destination. It copies each scanline
 * via the X_FUNC function we passed as an argument: this copies one scanline
 * duplicating or omitting pixels to fit the destination: we select an X_FUNC
 * depending on the bit-depth as well as the x-stretching ratio.
 *
 * both x and y stretching functions use the following basic model for deciding
 * when to insert/omit elements:
 *
 * 	delta = <larger extent> -1;
 *
 *      for (number of destination elements) {
 *
 *		copy one element
 *		advance pointer to larger region
 *		delta -= <smaller extent>
 *		if (delta < 0) {
 *			delta += <larger extent>;
 *			advance pointer to smaller region
 *		}
 *	}
 */


/* stretch proportions */
#define STRETCH_1_1	1
#define STRETCH_1_2	2
#define STRETCH_1_4	3
#define STRETCH_1_N	4
#define STRETCH_N_1	5
#define STRETCH_4_1	6
#define STRETCH_2_1	7



/*
 * an X_FUNC is a function that copies one scanline, stretching or shrinking it
 * to fit a destination scanline. Pick an X_FUNC depending on
 * bitdepth and stretch ratio (1:1, 1:2, 1:4, 1:N, N:1, 4:1, 2:1)
 *
 * the x_fract argument is the delta fraction: it is a representation
 * of the smaller extent (whichever that is) as a fraction of the larger,
 * and is used when stretching or shrinking to advance the pointer to the
 * smaller scanline every (fract) pixels of the larger.
 * Thus if we are expanding 1:8, x_fract will be 1/8, we will advance the
 * source pointer once every 8 pixels, and thus copy each source pixel to
 * 8 dest pixels. Note that if shrinking 8:1, x_fract will still be 1/8
 * and we will use it to control advancement of the dest pointer.
 * the fraction is multiplied by 65536.
 */
typedef void (*X_FUNC) (LPBYTE lpSrc,
			LPBYTE lpDst,
			int SrcXE,
			int DstXE,
			int x_fract);


void X_Stretch_1_1_8Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE, int DstXE, int x_fract);
void X_Stretch_1_2_8Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE, int DstXE, int x_fract);
void X_Stretch_1_4_8Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE, int DstXE, int x_fract);
void X_Stretch_1_N_8Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE, int DstXE, int x_fract);
void X_Stretch_N_1_8Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE, int DstXE, int x_fract);

void X_Stretch_1_1_16Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE, int DstXE, int x_fract);
void X_Stretch_1_2_16Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE, int DstXE, int x_fract);
void X_Stretch_1_N_16Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE, int DstXE, int x_fract);
void X_Stretch_N_1_16Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE, int DstXE, int x_fract);

void X_Stretch_1_1_24Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE, int DstXE, int x_fract);
void X_Stretch_1_N_24Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE, int DstXE, int x_fract);
void X_Stretch_N_1_24Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE, int DstXE, int x_fract);


/*
 * Y_Stretch_* functions copy DstYE scanlines (using
 * an X_FUNC to copy each scanline) omitting or duplicating scanlines to
 * fit the destination extent. Pick a Y_ depending on the ratio
 * (1:N, N:1...)
 */

void Y_Stretch_1_N(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE,int SrcYE, int DstXE,
		   int DstYE, int SrcWidth, int DstWidth, int x_fract,
		   X_FUNC x_func);

void Y_Stretch_N_1(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE,int SrcYE, int DstXE,
		   int DstYE, int SrcWidth, int DstWidth, int x_fract,
		   X_FUNC x_func);

/*
 * special case y-stretch functions for 1:2 in both dimensions for 8 and 16 bits
 * takes no X_FUNC arg. Will do entire stretch.
 */
void Stretch_1_2_8Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE,int SrcYE, int DstXE,
		   int DstYE, int SrcWidth, int DstWidth, int x_fract);


void Stretch_1_2_16Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE,int SrcYE, int DstXE,
		   int DstYE, int SrcWidth, int DstWidth, int x_fract);

/* straight copy of one scanline of count bytes */
void X_CopyScanline(LPBYTE lpSrc, LPBYTE lpDst, int count);


/* -------------------------------------------------------------------- */

/*
 * StretchFactor
 *
 * calculate the stretch factor (proportion of source extent to destination
 * extent: 1:1, 1:2, 1:4, 1:N, N:1, 4:1,or 2:1) and also the
 * delta fraction (see above comment on X_FUNC). This is the ratio of
 * the smaller extent to the larger extent, represented as a fraction
 * multiplied by 65536.
 *
 * returns: the stretch factor  (stores the delta fraction in *pfract)
 */

int
StretchFactor(int SrcE, int DstE, int *pfract)
{


	if (SrcE == DstE) {
		if (pfract != NULL) {
			pfract = 0;	     	
		}

		return(STRETCH_1_1);

	}


	if (SrcE > DstE) {
		if (pfract != NULL) {
			*pfract = ( (DstE << 16) / SrcE) & 0xffff;
		}

		if (SrcE == (DstE * 2)) {
			return(STRETCH_2_1);
		} else if (SrcE == (DstE * 4)) {
			return(STRETCH_4_1);
		} else {
			return(STRETCH_N_1);
		}

	} else {

		/* calculate delta fraction based on smallest / largest */
		if (pfract != NULL) {
			*pfract = ( (SrcE << 16) / DstE) & 0xffff;
		}
	
		if (DstE == (SrcE * 2)) {
			return(STRETCH_1_2);
		} else if (DstE == (SrcE * 4)) {
			return(STRETCH_1_4);
		} else {
			return(STRETCH_1_N);
		}
	}
}


/* -------------------------------------------------------------------- */

/*
 * StretchDIB
 *
 */

void FAR PASCAL
StretchDIB(
	LPBITMAPINFOHEADER biDst,   //	--> BITMAPINFO of destination
	LPVOID	lpvDst,		    //	--> to destination bits
	int	DstX,		    //	Destination origin - x coordinate
	int	DstY,		    //	Destination origin - y coordinate
	int	DstXE,		    //	x extent of the BLT
	int	DstYE,		    //	y extent of the BLT
	LPBITMAPINFOHEADER biSrc,   //	--> BITMAPINFO of source
	LPVOID	lpvSrc,		    //	--> to source bits
	int	SrcX,		    //	Source origin - x coordinate
	int	SrcY,		    //	Source origin - y coordinate
	int	SrcXE,		    //	x extent of the BLT
	int	SrcYE	 	    //	y extent of the BLT
	)
{

	int nBits;
	int SrcWidth, DstWidth;
	LPBYTE lpDst = lpvDst, lpSrc = lpvSrc;
	int x_fract;
	int x_factor;
	int y_factor;
	X_FUNC xfunc;
	

	/*
	 * check that bit depths are same and 8, 16 or 24
	 */

	if ((nBits = biDst->biBitCount) != biSrc->biBitCount) {
		return;
	}

	if ( (nBits != 8 ) && (nBits != 16) && (nBits != 24)) {
		return;
	}

	/*
	 * check that extents are not bad
	 */
	if ( (SrcXE <= 0) || (SrcYE <= 0) || (DstXE <= 0) || (DstYE <= 0)) {
		return;
	}

	/*
	 * calculate width of one scan line in bytes, rounded up to
	 * DWORD boundary.
	 */
	SrcWidth = (((biSrc->biWidth * nBits) + 31) & ~31) / 8;
	DstWidth = (((biDst->biWidth * nBits) + 31) & ~31) / 8;

	/*
	 * set initial source and dest pointers
	 */
	lpSrc += (SrcY * SrcWidth) + ((SrcX * nBits) / 8);
	lpDst += (DstY * DstWidth) + ((DstX * nBits) / 8);


	/*
	 * calculate stretch proportions (1:1, 1:2, 1:N, N:1 etc) and
	 * also the fractional stretch factor. (we are not interested in
	 * the y stretch fraction - this is only used in x stretching.
	 */

	y_factor = StretchFactor(SrcYE, DstYE, NULL);
	x_factor = StretchFactor(SrcXE, DstXE, &x_fract);

	/*
	 * we have special case routines for 1:2 in both dimensions
	 * for 8 and 16 bits
	 */
	if ((y_factor == x_factor) && (y_factor == STRETCH_1_2)) {

	 	if (nBits == 8) {
            StartCounting();
			Stretch_1_2_8Bits(lpSrc, lpDst, SrcXE, SrcYE,
					  DstXE, DstYE, SrcWidth, DstWidth,
					  x_fract);
            EndCounting("8 bit");
			return;

		} else if (nBits == 16) {
            StartCounting();
			Stretch_1_2_16Bits(lpSrc, lpDst, SrcXE, SrcYE,
					  DstXE, DstYE, SrcWidth, DstWidth,
					  x_fract);
            EndCounting("16 bit");
			return;
		}
	}


	/* pick an X stretch function */
	switch(nBits) {

	case 8:
		switch(x_factor) {
		case STRETCH_1_1:
			xfunc = X_Stretch_1_1_8Bits;
			break;

		case STRETCH_1_2:
			xfunc = X_Stretch_1_2_8Bits;
			break;

		case STRETCH_1_4:
			xfunc = X_Stretch_1_4_8Bits;
			break;

		case STRETCH_1_N:
			xfunc = X_Stretch_1_N_8Bits;
			break;

		case STRETCH_N_1:
		case STRETCH_4_1:
		case STRETCH_2_1:
			xfunc = X_Stretch_N_1_8Bits;
			break;

		}
		break;

	case 16:
		switch(x_factor) {
		case STRETCH_1_1:
			xfunc = X_Stretch_1_1_16Bits;
			break;

		case STRETCH_1_2:
			xfunc = X_Stretch_1_2_16Bits;
			break;

		case STRETCH_1_4:
		case STRETCH_1_N:
			xfunc = X_Stretch_1_N_16Bits;
			break;

		case STRETCH_N_1:
		case STRETCH_4_1:
		case STRETCH_2_1:
			xfunc = X_Stretch_N_1_16Bits;
			break;

		}
		break;

	case 24:
		switch(x_factor) {
		case STRETCH_1_1:
			xfunc = X_Stretch_1_1_24Bits;
			break;

		case STRETCH_1_2:
		case STRETCH_1_4:
		case STRETCH_1_N:
			xfunc = X_Stretch_1_N_24Bits;
			break;

		case STRETCH_N_1:
		case STRETCH_4_1:
		case STRETCH_2_1:
			xfunc = X_Stretch_N_1_24Bits;
			break;

		}
		break;

	}


	/*
	 * now call appropriate stretching function depending
	 * on the y stretch factor
	 */
	switch (y_factor) {
	case STRETCH_1_1:
	case STRETCH_1_2:
	case STRETCH_1_4:
	case STRETCH_1_N:
		Y_Stretch_1_N(lpSrc, lpDst, SrcXE, SrcYE,
			      DstXE, DstYE, SrcWidth, DstWidth, x_fract, xfunc);
		break;

	case STRETCH_N_1:
	case STRETCH_4_1:
	case STRETCH_2_1:
		Y_Stretch_N_1(lpSrc, lpDst, SrcXE, SrcYE,
			      DstXE, DstYE, SrcWidth, DstWidth, x_fract, xfunc);
		break;

	}
	return;
}


/* ---- y stretching -------------------------------------------- */

/*
 * call an X_FUNC to copy scanlines from lpSrc to lpDst. Duplicate or
 * omit scanlines to stretch SrcYE to DstYE.
 */


/*
 * Y_Stretch_1_N
 *
 * write DstYE scanlines based on SrcYE scanlines, DstYE > SrcYE
 *
 */

void
Y_Stretch_1_N(LPBYTE lpSrc,
              LPBYTE lpDst,
              int SrcXE,
              int SrcYE,
              int DstXE,
              int DstYE,
	      int SrcWidth,
	      int DstWidth,
              int x_fract,
              X_FUNC x_func)
{

	int ydelta;
	int i;
	LPBYTE lpPrev = NULL;

	ydelta = DstYE -1;

	for (i = 0; i < DstYE; i++) {

		/* have we already stretched this scanline ? */
		if (lpPrev == NULL) {
			/* no - copy one scanline */
			(*x_func)(lpSrc, lpDst, SrcXE, DstXE, x_fract);
			lpPrev = lpDst;
		} else {	
			/* yes - this is a duplicate scanline. do
			 * a straight copy of one that has already
			 * been stretched/shrunk
			 */
			X_CopyScanline(lpPrev, lpDst, DstWidth);
		}

		/* advance dest pointer */
		lpDst += DstWidth;

		/* should we advance source pointer this time ? */
		if ( (ydelta -= SrcYE) < 0) {
			ydelta += DstYE;
			lpSrc += SrcWidth;
			lpPrev = NULL;
		}
	}
}


/*
 * Y_Stretch_N_1
 *
 * write DstYE scanlines based on SrcYE scanlines, DstYE < SrcYE
 *
 */
void
Y_Stretch_N_1(LPBYTE lpSrc,
              LPBYTE lpDst,
              int SrcXE,
              int SrcYE,
              int DstXE,
              int DstYE,
	      int SrcWidth,
	      int DstWidth,
              int x_fract,
              X_FUNC x_func)
{

	int ydelta;
	int i;

	ydelta = SrcYE -1;

	for (i = 0; i < DstYE; i++) {

		/* copy one scanline */
		(*x_func)(lpSrc, lpDst, SrcXE, DstXE, x_fract);

		/* advance dest pointer */
		lpDst += DstWidth;

		/* how many times do we advance source pointer this time ? */
		do {
			lpSrc += SrcWidth;
			ydelta -= DstYE;
		} while (ydelta >= 0);

		ydelta += SrcYE;
	}
}

/* ---8-bit X stretching -------------------------------------------------- */

/*
 * X_Stretch_1_N_8Bits
 *
 * copy one scan line, stretching 1:N (DstXE > SrcXE). For 8-bit depth.
 */
void
X_Stretch_1_N_8Bits(LPBYTE lpSrc,
		    LPBYTE lpDst,
		    int SrcXE,
		    int DstXE,
		    int x_fract)
{
	int xdelta;
	int i;

	xdelta = DstXE -1;

	for (i = 0; i < DstXE; i++) {

		/* copy one byte and advance dest */
		*lpDst++ = *lpSrc;

		/* should we advance source pointer this time ? */
		if ( (xdelta -= SrcXE) < 0) {
			xdelta += DstXE;
			lpSrc++;
		}
	}
}


/*
 * X_Stretch_N_1_8Bits
 *
 * copy one scan line, shrinking N:1 (DstXE < SrcXE). For 8-bit depth.
 */
void
X_Stretch_N_1_8Bits(LPBYTE lpSrc,
		    LPBYTE lpDst,
		    int SrcXE,
		    int DstXE,
		    int x_fract)
{
	int xdelta;
	int i;

	xdelta = SrcXE -1;

	for (i = 0; i < DstXE; i++) {

		/* copy one byte and advance dest */
		*lpDst++ = *lpSrc;

		/* how many times do we advance source pointer this time ? */
		do {
			lpSrc++;
			xdelta -= DstXE;
		} while (xdelta >= 0);

		xdelta += SrcXE;
	}
}

/*
 * copy one scanline of count bytes from lpSrc to lpDst. used by 1:1
 * scanline functions for all bit depths
 */
void
X_CopyScanline(LPBYTE lpSrc, LPBYTE lpDst, int count)
{
	int i;

	/*
	 * if the alignment of lpSrc and lpDst is the same, then
	 * we can get them aligned and do a faster copy
	 */
        if (((DWORD) lpSrc & 0x3) == ( (DWORD) lpDst & 0x3)) {
		
		/* align on WORD boundary */
		if ( (DWORD) lpSrc & 0x1) {
			*lpDst++ = *lpSrc++;
			count--;
		}

		/* align on DWORD boundary */
		if ((DWORD) lpSrc & 0x2) {
			* ((LPWORD) lpDst) = *((LPWORD) lpSrc);
			lpDst += sizeof(WORD);
			lpSrc += sizeof(WORD);
			count -= sizeof(WORD);
		}

		/* copy whole DWORDS */
		for ( i = (count / 4); i > 0; i--) {
			*((LPDWORD) lpDst) =  *((LPDWORD) lpSrc);
			lpSrc += sizeof(DWORD);
			lpDst += sizeof(DWORD);
		}
	} else {
		/* the lpSrc and lpDst pointers are different
		 * alignment, so leave them unaligned and
		 * copy all the whole DWORDs
		 */
                for (i = (count / 4); i> 0; i--) {
			*( (DWORD UNALIGNED FAR *) lpDst) =
				*((DWORD UNALIGNED FAR *) lpSrc);
			lpSrc += sizeof(DWORD);
			lpDst += sizeof(DWORD);
		}
	}

	/* in either case, copy last (up to 3) bytes. */
	for ( i = count % 4; i > 0; i--) {
		*lpDst++ = *lpSrc++;
	}
}
		
/*
 * X_Stretch_1_1_8Bits
 *
 * copy a scanline with no change (1:1)
 */
void
X_Stretch_1_1_8Bits(LPBYTE lpSrc,
		    LPBYTE lpDst,
		    int SrcXE,
		    int DstXE,
		    int x_fract)
{

	X_CopyScanline(lpSrc, lpDst, DstXE);
}


/*
 * X_Stretch_1_2_8Bits
 *
 * copy a scanline, doubling all the pixels (1:2)
 */
void
X_Stretch_1_2_8Bits(LPBYTE lpSrc,
		    LPBYTE lpDst,
		    int SrcXE,
		    int DstXE,
		    int x_fract)
{
   	WORD wPix;
	int i;

	for (i = 0; i < SrcXE; i++) {
		
		/* get a pixel and double it */
		wPix = *lpSrc++;
		wPix |= (wPix << 8);
		* ((WORD UNALIGNED *) lpDst) = wPix;
		lpDst += sizeof(WORD);
	}
}


/*
 * X_Stretch_1_4_8Bits
 *
 * copy a scanline, quadrupling all the pixels (1:4)
 */
void
X_Stretch_1_4_8Bits(LPBYTE lpSrc,
		    LPBYTE lpDst,
		    int SrcXE,
		    int DstXE,
		    int x_fract)
{
	DWORD dwPix;
	int i;

	for (i = 0; i < SrcXE; i++) {

		/* get a pixel and make four copies of it */
		dwPix = *lpSrc++;
		dwPix |= (dwPix <<8);
		dwPix |= (dwPix << 16);
		* ((DWORD UNALIGNED *) lpDst) = dwPix;
		lpDst += sizeof(DWORD);
	}
}


/*  -- 16-bit X functions -----------------------------------------------*/

/*
 * copy one scan-line of 16 bits with no change (1:1)
 */
void
X_Stretch_1_1_16Bits(LPBYTE lpSrc,
		    LPBYTE lpDst,
		    int SrcXE,
		    int DstXE,
		    int x_fract)
{

	X_CopyScanline(lpSrc, lpDst, DstXE * sizeof(WORD));

}


/*
 * copy one scanline of 16 bpp duplicating each pixel
 */
void
X_Stretch_1_2_16Bits(LPBYTE lpSrc,
		    LPBYTE lpDst,
		    int SrcXE,
		    int DstXE,
		    int x_fract)
{

   	DWORD dwPix;
	int i;

	for (i = 0; i < SrcXE; i++) {
		
		/* get a pixel and double it */
		dwPix = * ((WORD *)lpSrc);
		dwPix |= (dwPix << 16);
		* ((DWORD UNALIGNED *) lpDst) = dwPix;

		lpDst += sizeof(DWORD);
		lpSrc += sizeof(WORD);
	}

}

/*
 * copy one scanline of 16 bits, stretching 1:n (dest > source)
 */
void
X_Stretch_1_N_16Bits(LPBYTE lpSrc,
		    LPBYTE lpDst,
		    int SrcXE,
		    int DstXE,
		    int x_fract)
{
	int xdelta;
	int i;

	xdelta = DstXE -1;

	for (i = 0; i < DstXE; i++) {

		/* copy one pixel and advance dest */
		*((WORD *) lpDst) = *((WORD *) lpSrc);

		lpDst += sizeof(WORD);

		/* should we advance source pointer this time ? */
		if ( (xdelta -= SrcXE) < 0) {
			xdelta += DstXE;
			lpSrc += sizeof(WORD);
		}
	}
}

/*
 * copy one scanline of 16bits, shrinking n:1 (dest < source)
 */
void
X_Stretch_N_1_16Bits(LPBYTE lpSrc,
		    LPBYTE lpDst,
		    int SrcXE,
		    int DstXE,
		    int x_fract)
{

	int xdelta;
	int i;

	xdelta = SrcXE -1;

	for (i = 0; i < DstXE; i++) {

		/* copy one pixel and advance dest */
		*((WORD *) lpDst) = *((WORD *)lpSrc);

		lpDst += sizeof(WORD);

		/* how many times do we advance source pointer this time ? */
		do {
			lpSrc += sizeof(WORD);
			xdelta -= DstXE;
		} while (xdelta >= 0);

		xdelta += SrcXE;
	}

}


/* 24-bits ---------------------------------------------------------*/

/*
 * copy one 24-bpp scanline as is (1:1)
 */
void
X_Stretch_1_1_24Bits(LPBYTE lpSrc,
		    LPBYTE lpDst,
		    int SrcXE,
		    int DstXE,
		    int x_fract)
{
	X_CopyScanline(lpSrc, lpDst, DstXE * 3);
}

/*
 * copy one 24-bpp scanline stretching 1:n (dest > source)
 */
void
X_Stretch_1_N_24Bits(LPBYTE lpSrc,
		    LPBYTE lpDst,
		    int SrcXE,
		    int DstXE,
		    int x_fract)
{

	int xdelta;
	int i;

	xdelta = DstXE -1;

	for (i = 0; i < DstXE; i++) {
		/* copy first word of pixel and advance dest */
		*((WORD UNALIGNED *) lpDst) = *((WORD UNALIGNED *) lpSrc);

		lpDst += sizeof(WORD);

		/* copy third byte and advance dest */
		*lpDst++ = lpSrc[sizeof(WORD)];

		/* should we advance source pointer this time ? */
		if ( (xdelta -= SrcXE) < 0) {
			xdelta += DstXE;
			lpSrc += 3;
		}
	}
}

/*
 * copy one scanline of 24 bits, shrinking n:1 (dest < source)
 */
void
X_Stretch_N_1_24Bits(LPBYTE lpSrc,
		    LPBYTE lpDst,
		    int SrcXE,
		    int DstXE,
		    int x_fract)
{
	int xdelta;
	int i;

	xdelta = SrcXE -1;

	for (i = 0; i < DstXE; i++) {

		/* copy first word of pixel and advance dest */
		*((WORD UNALIGNED *) lpDst) = *((WORD UNALIGNED *) lpSrc);

		lpDst += sizeof(WORD);

		/* copy third byte and advance dest */
		*lpDst++ = lpSrc[sizeof(WORD)];


		/* how many times do we advance source pointer this time ? */
		do {
			lpSrc += 3;
			xdelta -= DstXE;
		} while (xdelta >= 0);

		xdelta += SrcXE;
	}
}		

/* -- special-case 1:2 -------------------------------------------*/

/*
 * stretch 1:2 in both directions, for 8 bits.
 *
 * An experiment was done on x86 to only write every other line during
 * the stretch and when the whole frame was done to use memcpy to fill
 * in the gaps.  This is slower than doing the stretch in a single pass.
 */
void
Stretch_1_2_8Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE,int SrcYE, int DstXE,
		   int DstYE, int SrcWidth, int DstWidth, int x_fract)
{

	int SrcInc, DstInc;
	int i, j;
	WORD wPix;
	DWORD dwPix4;

	/* amount to advance source by at the end of each scan */
	SrcInc = SrcWidth - SrcXE;


	/* amount to advance dest by at the end of each scan - note
	 * that we write two scans at once, so advance past the next
	 * scan line
	 */
	DstInc = (DstWidth * 2) - DstXE;

	/*
	 * we would like to copy the pixels DWORD at a time. this means
	 * being aligned. if we are currently aligned on a WORD boundary,
	 * then copy one pixel to get aligned. If we are on a byte
	 * boundary, we can never get aligned, so use the slower loop.
	 */
	if ( ((DWORD)lpDst) & 1) {

		/*
		 * dest is byte aligned - so we can never align it
		 * by writing WORDs - use slow loop.
		 */
		for (i = 0; i < SrcYE; i++) {
	
			for (j = 0; j < SrcXE; j++) {
	
				/* get a pixel and double it */
	
				wPix = *lpSrc++;
				wPix |= (wPix<<8);
	
	
				/* write doubled pixel to this scanline */
	
				*( (WORD UNALIGNED *) lpDst) = wPix;
	
				/* write double pixel to next scanline */
				*( (WORD UNALIGNED *) (lpDst + DstWidth)) = wPix;
	
				lpDst += sizeof(WORD);
			}
			lpSrc += SrcInc;
			lpDst += DstInc;
		}
		return;
	}

	/*
	 * this will be the aligned version. align each scan line
	 */
	for ( i = 0; i < SrcYE; i++) {

		/* count of pixels remaining */
		j = SrcXE;

		/* align this scan line */
		if (((DWORD)lpDst) & 2) {

			/* word aligned - copy one doubled pixel and we are ok */
			wPix = *lpSrc++;
			wPix |= (wPix << 8);
	
			*( (WORD *) lpDst) = wPix;
  			*( (WORD *) (lpDst + DstWidth)) = wPix;
			lpDst += sizeof(WORD);

			j -= 1;
		}


		/* now dest is aligned - so loop eating two pixels at a time
		 * until there is at most one left
		 */
               	for ( ; j > 1; j -= 2) {

			/* read two pixels and double them */
			wPix = * ((WORD UNALIGNED *) lpSrc);
			lpSrc += sizeof(WORD);

			dwPix4 = (wPix & 0xff) | ((wPix & 0xff) << 8);
			dwPix4 |= ((wPix & 0xff00) << 8) | ((wPix & 0xff00) << 16);
			*((DWORD *) lpDst) = dwPix4;
  			*((DWORD *) (lpDst + DstWidth)) = dwPix4;

			lpDst += sizeof(DWORD);
		}

		/* odd byte remaining ? */
		if (j > 0) {
			/* word aligned - copy one doubled pixel and we are ok */
			wPix = *lpSrc++;
			wPix |= (wPix << 8);
	
			*( (WORD *) lpDst) = wPix;
			*( (WORD *) (lpDst + DstWidth)) = wPix;
			lpDst += sizeof(WORD);

			j -= 1;
		}
		lpSrc += SrcInc;
		lpDst += DstInc;
	}
}



/* ----------------------------------------------------------------*/

/*
 * stretch 1:2 in both directions, for 16-bits
 */

void
Stretch_1_2_16Bits(LPBYTE lpSrc, LPBYTE lpDst, int SrcXE,int SrcYE, int DstXE,
		   int DstYE, int SrcWidth, int DstWidth, int x_fract)

{
	int SrcInc, DstInc;
	int i, j;
	DWORD dwPix;

	/* amount to advance source by at the end of each scan */
	SrcInc = SrcWidth - (SrcXE * sizeof(WORD));


	/* amount to advance dest by at the end of each scan - note
	 * that we write two scans at once, so advance past the next
	 * scan line
	 */
	DstInc = (DstWidth * 2) - (DstXE * sizeof(WORD));

	for (i = 0; i < SrcYE; i++) {

		for (j = 0; j < SrcXE; j++) {

			/* get a pixel and double it */

			dwPix = *((WORD *)lpSrc);
			dwPix |= (dwPix<<16);

			lpSrc += sizeof(WORD);

			/* write doubled pixel to this scanline */

			*( (DWORD UNALIGNED *) lpDst) = dwPix;

			/* write double pixel to next scanline */
			*( (DWORD UNALIGNED *) (lpDst + DstWidth)) = dwPix;

			lpDst += sizeof(DWORD);
		}
	        lpSrc += SrcInc;
		lpDst += DstInc;

	}
}
