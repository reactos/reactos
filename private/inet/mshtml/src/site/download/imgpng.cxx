#include "headers.hxx"

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

#if 0

#ifndef X_PNG_H_
#define X_PNG_H_
#include "png.h"
#endif

// Urk! no verify!
#if DBG==1
#define VERIFY( exp ) if ( !exp ) AssertSz( FALSE, "Verify failed." )
#define DBTRACE( x ) OutputDebugStringA( x "\n" );
#else
#define VERIFY( exp ) ( exp )
#define DBTRACE( x ) 
#endif

typedef unsigned char PNGSIG[8];

#define cyReadRows 8;

static BOOL ProcessImage(png_structp ppng_struct,
						 png_infop ppng_info,
						 BITMAPINFOHEADER* pBMI,
						 IMGLOADNOTIFYFN* pNotifyFn,
						 IMPFLTSTATUS* pStatus)
{
	int iBitsSize, iPass, cPass;
	int iY, iStorageWidth;
	RGBQUAD *argbPNG = NULL;	// GIF color table
	RGBQUAD *argbButch = NULL;	// color table specified by our client, Butch
	ULONG cbCtabSize = 0;
	unsigned char **ppchRows;			// vector of pointers to row starts

	DBTRACE( "PNG ProcessImage" );

  	// we can't handle 16 bits/channel, so get LibPNG to downsample for us
	if ( ppng_info->bit_depth == 16 )
		png_set_strip_16( ppng_struct );

	// prep the library for interlace mode, if needed.
	
	if ( ppng_info->interlace_type != 0 )
		cPass = png_set_interlace_handling( ppng_struct );
	else
		cPass =  1;
	

	if ( ppng_info->valid & PNG_INFO_bKGD )
		png_set_background( ppng_struct, &(ppng_info->background),
							PNG_BACKGROUND_GAMMA_FILE, 1, 1.0 );
	else
	{
	// Fake the background for now
	png_color_16 pngc16;

	// I see a red pel and I want to paint it black...
	pngc16.index = 0;
	pngc16.red = 0;
	pngc16.blue = 0;
	pngc16.green = 0;
	pngc16.gray = 0;

	png_set_background( ppng_struct, &pngc16,
						PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0 );
	}


	// Make the image header callback with the bmi data we read earlier
	pStatus->iState = ImgFltImpImgHdr;
	if (!pNotifyFn(pStatus)) goto $abort;

	// make a local copy so we can assert it prior to dithering
	// Or, as Bill Fernandez once said, "Note the colors..."
	cbCtabSize = pStatus->dwClrTabSize * sizeof(RGBQUAD);
	argbPNG = (RGBQUAD *)GlobalAlloc( GMEM_FIXED, cbCtabSize );
	if ( argbPNG == NULL )
		goto $abort;
	else
		CopyMemory( argbPNG, pStatus->pClrTab, cbCtabSize ); 


	// compute the image size
	iBitsSize = pBMI->biHeight * DIBStorageWidth(pBMI);

     // Make the bits allocation callback
	pStatus->iState = ImgFltImpAllocBits;
	pStatus->dwBitsSize = iBitsSize;
	pStatus->dwFlags |= ImgFltColorsNegotiable;
	if (!pNotifyFn(pStatus)) goto $abort;
    AssertSz(pStatus->pBits, "ImportPng - client didn't allocate bits!" );

	// It was very nice of Butch to tell us about the colors he wanted, but seeing as
	// we aren't prepared to dither one scanline at a time, let's do something slimey.
	// Let's slam our colors onto the bitmap so that it looks right during the progressive
	// phase, then switch 'em back just prior to dithering after the last pass...

	argbButch = (RGBQUAD *)GlobalAlloc( GMEM_FIXED, cbCtabSize );
	if ( argbButch != NULL )
		GetDIBColorTable( pStatus->hdcTarget, 0, pStatus->dwClrTabSize, argbButch ); 
	else
		goto $abort;

	if ( pStatus->hdcTarget != NULL )
	{
		SetDIBColorTable( pStatus->hdcTarget, 0, pStatus->dwClrTabSize, argbPNG );	
	}
	CopyMemory( pStatus->pClrTab, argbPNG, cbCtabSize );

	// Now, allocate a vector of pointers into the destination bits
	ppchRows = (unsigned char **)GlobalAlloc( GMEM_FIXED, ppng_info->height * sizeof(LPVOID) );
	if ( ppchRows == NULL )
		goto $abort;

	iStorageWidth = DIBStorageWidth(pBMI);

	AssertSz( pBMI->biHeight >= 0, "IportPng - can't handle top-down destination." );
	for ( iY = pBMI->biHeight - 1; iY >= 0; iY-- )
		ppchRows[iY] = pStatus->pBits + (pBMI->biHeight - iY - 1) * iStorageWidth;

	// now read the pixels in cyReadRows at a time for each pass

	for ( iPass = cPass; iPass > 0; iPass-- )
	{
		int cyRead;

		for ( iY = 0; iY < pBMI->biHeight; iY += cyRead )
		{
			cyRead = cyReadRows;
					
			if ( iY + cyRead >= pBMI->biHeight )
				cyRead = pBMI->biHeight - iY;

			png_read_rows( ppng_struct, NULL, &ppchRows[iY], cyRead );

			// make a data callback
			pStatus->iState = ImgFltImpImgData;
			pStatus->dwBandLineFirst = iY;
			pStatus->dwBandSize = cyRead;
			if (!pNotifyFn(pStatus)) goto $abort;
		}
	}

//	png_read_image( ppng_struct, ppchRows );

	if ( pStatus->hdcTarget != NULL )
	{
		// now do an IN-PLACE dither on the image
		ColorMap cmap;
 
		cmap.SetColors( argbButch, pStatus->dwClrTabSize, NULL );

		Ditherer ditherer( &cmap );

		// Set the source, which has the bitmap, but with our color table
		ditherer.SetSource( pStatus->hdcTarget );

		struct  {
			// keep together (as a BITMAPINFO struct)
			BITMAPINFOHEADER bmi;
			RGBQUAD clrs[256];
		} pngbitmap;             // bitmap header

		pngbitmap.bmi = *pBMI;
		CopyMemory( pngbitmap.clrs, argbButch, cbCtabSize );

		// Set the dest to the same bits, but with Butch's color table
		ditherer.SetDestination( (BITMAPINFO *)&pngbitmap, pStatus->pBits );

		ditherer.Dither( 0, pngbitmap.bmi.biHeight );

		// Restore the proper color table.
		// BUGBUG: must wait for last update to be drawn on main UI thread before playing
		// this game
		CopyMemory( pStatus->pClrTab, argbButch, cbCtabSize );
		SetDIBColorTable( pStatus->hdcTarget, 0, pStatus->dwClrTabSize, pStatus->pClrTab );	

		// make a data callback
		pStatus->iState = ImgFltImpImgData;
		pStatus->dwBandLineFirst = 0;
		pStatus->dwBandSize = pngbitmap.bmi.biHeight;
		if (!pNotifyFn(pStatus)) goto $abort;
	}

	// make the image complete callback
	pStatus->iState = ImgFltImpImgComplete;
	if (!pNotifyFn(pStatus)) goto $abort;

	return TRUE;

$abort:

	if ( argbPNG != NULL )
		GlobalFree( argbPNG );

	if ( argbButch != NULL )
		GlobalFree( argbButch );

	if ( ppchRows != NULL )
		GlobalFree( ppchRows );

	return FALSE;
}


// Import a GIF file
BOOL ImportPng
(
    LPSTREAM			pstream,
    BITMAPINFOHEADER*   pBMI,
    IMGLOADNOTIFYFN*    pNotifyFn,
    IMPFLTSTATUS*       pStatus
)
{
	BOOL bResult = FALSE;
	png_structp	ppng_struct = NULL;
	png_infop	ppng_info = NULL;

	// make the inital callback
    AssertSz(pNotifyFn, "ImportPng - no callback" );
    AssertSz(pStatus, "ImportPng - no status struct" );
	pStatus->iState = ImgFltImpBegin;
	if (!pNotifyFn(pStatus)) goto $abort;

    // get the current file position
	// (we might not be at the start)

	LARGE_INTEGER liSeek;
	ULARGE_INTEGER uliPos;
	liSeek.LowPart = 0;
	liSeek.HighPart = 0;
	// don't move, but do get the pos
	VERIFY( SUCCEEDED(pstream->Seek( liSeek, STREAM_SEEK_CUR, &uliPos )) );
	AssertSz( uliPos.HighPart == 0, "Too far out into stream." );

	// check it's a PNG file
	// LibPng does this, and since we can't seek backwards, we must skip this check
	/*
    PNGSIG pngsig;
    ULONG ulBytes;
	VERIFY( SUCCEEDED( pstream->Read( pngsig, sizeof(pngsig), &ulBytes ) ) );
    if (ulBytes != sizeof(pngsig)) {
        // too small for a GIF file
        goto $abort;
    }
	pStatus->dwFileBytesRead +=	ulBytes;

    // check we have the magic id at the start
    if ( !png_check_sig(pngsig, sizeof(pngsig)) ) {
        // not a PNG file
        goto $abort;
    }
	*/

	ppng_struct = (png_struct *)GlobalAlloc( GMEM_FIXED, sizeof(png_struct) );
	if ( ppng_struct == NULL )
		goto $abort;

	ppng_info = (png_info *)GlobalAlloc( GMEM_FIXED, sizeof(png_info) );
	if ( ppng_info == NULL )
		goto $abort;

	if ( setjmp( ppng_struct->jmpbuf ) )
		goto $abort;

	png_info_init( ppng_info );
	png_read_init( ppng_struct );
	png_init_io( ppng_struct, pstream );
	png_read_info( ppng_struct, ppng_info );

	// copy the size info
	pBMI->biWidth = ppng_info->width;
	pBMI->biHeight = ppng_info->height;

	switch ( ppng_info->color_type )
	{
	case 0: // pure grayscale
		pBMI->biBitCount = ppng_info->bit_depth;
		break;

	case 2: // rgb triples
		pBMI->biBitCount = 24; // we'll have to get LibPNG to downsample 48 bpp.
		break;

	case 3: // indexed colors
		pBMI->biBitCount = ppng_info->bit_depth;
		break;

	case 4: // grayscale/alpha pairs
		pBMI->biBitCount = ppng_info->bit_depth;
		break;

	case 6: // rgba quads
		pBMI->biBitCount = 24; // we'll have to get LibPNG to downsample 48 bpp.
		break;

	default:
		break;
	}

	pBMI->biPlanes = 1;
	pBMI->biCompression = BI_RGB;

	// make the file header callback
	pStatus->iState = ImgFltImpFileHdr;
	
	STATSTG statstg;
	VERIFY( SUCCEEDED( pstream->Stat( &statstg, STATFLAG_NONAME ) ) );
	AssertSz( (statstg.cbSize.HighPart == 0), "File too big." );

	pStatus->dwFileSize = statstg.cbSize.LowPart - uliPos.LowPart;
	pStatus->dwImageCount = 1;
	pStatus->dwMaxImageWidth = pBMI->biWidth;
	pStatus->dwMaxImageHeight = pBMI->biHeight;
	pStatus->dwImageBitCount = pBMI->biBitCount;
	if (!pNotifyFn(pStatus)) goto $abort;

	// see if there is a color table associated with the image
	if ( (ppng_info->color_type & 0x01) && ppng_info->valid & PNG_INFO_PLTE )
	{ 
		// get the size of the table
		int iClrTabSize = ppng_info->num_palette;
		png_colorp ppng_color ;
		RGBQUAD	*prgbq;

        AssertSz(iClrTabSize <= 256, "ImportPng - too many colors in palette." );
		pBMI->biClrUsed = iClrTabSize;

		 // Make the color table alloc callback
		pStatus->iState = ImgFltImpAllocClrTab;
		pStatus->dwImageCount = 1;
		pStatus->dwImageWidth = pBMI->biWidth;
		pStatus->dwImageHeight = pBMI->biHeight;
		pStatus->dwImageBytesRead = 0;
		pStatus->dwImageBitCount = pBMI->biBitCount;
		pStatus->dwClrTabSize = pBMI->biClrUsed;
		if (!pNotifyFn(pStatus)) goto $abort;
        AssertSz(pStatus->pClrTab, "ImportPng - client failed color table allocation." );
		
		for ( ppng_color = ppng_info->palette, prgbq = pStatus->pClrTab;
			  iClrTabSize > 0;
			  iClrTabSize--, ppng_color++, prgbq++ )
		{
			prgbq->rgbRed = ppng_color->red;
			prgbq->rgbGreen = ppng_color->green;
			prgbq->rgbBlue = ppng_color->blue;
		}

		// make a color table callback
		pStatus->iState = ImgFltImpClrTab;
		if (!pNotifyFn(pStatus)) goto $abort;
	}

	// process the data blocks

	// if we get here, it's all done
	bResult = ProcessImage( ppng_struct, ppng_info, pBMI, pNotifyFn, pStatus );

$abort:
	// clean up after abort
	if (!bResult) {
		LARGE_INTEGER liSeek;
		liSeek.LowPart = liSeek.HighPart = 0;

		// restore the file position
		pstream->Seek( liSeek, STREAM_SEEK_SET, NULL );
	}
  
	if ( ppng_struct != NULL )
		GlobalFree( ppng_struct );

	if ( ppng_info != NULL )
		GlobalFree( ppng_info );

	// make the file complete callback
	pStatus->iState = ImgFltImpFileComplete;
	pNotifyFn(pStatus);

	return bResult;
}

#endif

CImgFilt * ImgFiltCreatePng(CRITICAL_SECTION * pcs)
{
    return(NULL);
}
