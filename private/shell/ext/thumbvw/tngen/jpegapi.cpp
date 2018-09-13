
/* jpegapi.cpp -- interface layer for painless JPEG compression of NIFty images.
 * Written by Ajai Sehgal 3/10/96
 * (c) Copyright Microsoft Corporation
 *
 *	08-27-1997 (kurtgeis) Pushed exception handling into library.  Changed
 *	all entry points to return HRESULTs.  Added width and heigth to parameters.
 */
#pragma warning(disable:4005)
#include "stdafx.h"
#include "jpegapi.h"
#include "jmemsys.h"

/********************************************************************************/

/* JPEGCompressHeader()
 *
 * Arguments:
 *		tuQuality	"Quality" of the resulting JPEG (0..100, 100=best)
 *
 * Returns:
 *		HRESULT
 */
HRESULT JPEGCompressHeader(BYTE *prgbJPEGHeaderBuf, UINT tuQuality, ULONG *pcbOut, HANDLE *phJpegC, J_COLOR_SPACE ColorSpace)
{
	HRESULT			hr = S_OK;

	try
	{
		jpeg_compress_struct *spjcs = new jpeg_compress_struct;
		struct jpeg_error_mgr *jem = new jpeg_error_mgr;

		spjcs->err = jpeg_std_error(jem);	// Init the error handler
		jpeg_create_compress(spjcs);		// Init the compression object
		
		if (ColorSpace == JCS_GRAYSCALE)
		{
			spjcs->in_color_space = JCS_GRAYSCALE;
			spjcs->input_components = 1;
		}
		else
		{
			spjcs->in_color_space = JCS_RGBA;
			spjcs->input_components = 4;
		}
		jpeg_set_defaults(spjcs);		// Init the compression engine with the defaults
		
		jpeg_set_quality(spjcs, tuQuality, TRUE);
		
		jpeg_set_colorspace(spjcs,ColorSpace);

		jpeg_mem_dest(spjcs, prgbJPEGHeaderBuf);	// Init the "destination manager"
		
		spjcs->comps_in_scan = 0;
		
		spjcs->write_JFIF_header = FALSE;

		jpeg_write_tables(spjcs);

		jpeg_suppress_tables(spjcs, TRUE);

		*pcbOut = spjcs->bytes_in_buffer;

		*phJpegC = (HANDLE) spjcs;
	}
	catch( THROWN thrownHR )
	{
		hr = thrownHR.Hr();
	}

	return hr;
}

/* JPEGDecompressHeader()
 *
 * Arguments:
 *		*prgbJPEGBuf : pointer to the JPEG header data as read from file
 *		*phJpegD:	pointer to Handle of the JPEG decompression object returned
 *
 * Returns:
 *		HRESULT
 */
HRESULT JPEGDecompressHeader(BYTE *prgbJPEGHeaderBuf, HANDLE *phJpegD, ULONG ulBufferSize)
{
	HRESULT			hr = S_OK;

	try
	{
		jpeg_decompress_struct * spjds = new jpeg_decompress_struct;
		struct jpeg_error_mgr *jem = new jpeg_error_mgr;

		spjds->err = jpeg_std_error(jem);	// Init the error handler
		
		jpeg_create_decompress(spjds);	// Init the decompression object
		

	// Now we need to "read" it into the decompression object...

		jpeg_mem_src(spjds, prgbJPEGHeaderBuf, ulBufferSize);
		
		jpeg_read_header(spjds, FALSE);

		spjds->out_color_space = JCS_RGBA;
		
		*phJpegD = (HANDLE) spjds;
	}
	catch( THROWN thrownHR )
	{
		hr = thrownHR.Hr();
	}

	return hr;
}

// DestroyJPEGCompress
//
// Release all the JPEG stuff from the handle we gave to the user
//
HRESULT DestroyJPEGCompressHeader(HANDLE hJpegC)
{
	HRESULT			hr = S_OK;

	try
	{
		struct jpeg_compress_struct *pjcs = (struct jpeg_compress_struct *)hJpegC;
		jpeg_destroy_compress(pjcs);
		delete pjcs->err;
		delete pjcs;
	}
	catch( THROWN thrownHR )
	{
		hr = thrownHR.Hr();
	}

	return hr;
}

// DestroyJPEGDecompressHeader
//
// Release all the JPEG stuff from the handle we gave to the user
//
HRESULT DestroyJPEGDecompressHeader(HANDLE hJpegD)
{
	HRESULT			hr = S_OK;

	try
	{
		struct jpeg_decompress_struct *pjds = (struct jpeg_decompress_struct *)hJpegD;
		jpeg_destroy_decompress(pjds);
		delete pjds->err;
		delete pjds;
	}
	catch( THROWN thrownHR )
	{
		hr = thrownHR.Hr();
	}

	return hr;
}

/* JPEGFromRGBA()
 *
 * Arguments:
 *		prgbImage	A raw image buffer (4 bytes/pixel, RGBA order)
 *		cpxlAcross	Width of the image, in pixels
 *		cpxlDown	Height of the image, in pixels
 *		tuQuality	"Quality" of the resulting JPEG (0..100, 100=best)
 *		A memory buffer containing the complete JPEG compressed version of the
 *		given image. NULL on error.
 *
 * Returns:
 *		HRESULT
 */

HRESULT JPEGFromRGBA(BYTE *prgbImage, BYTE *prgbJPEGBuf, UINT tuQuality, ULONG *pcbOut, HANDLE hJpegC, J_COLOR_SPACE ColorSpace, UINT nWidth, UINT nHeight )
{
	HRESULT			hr = S_OK;

	try
	{
		struct jpeg_compress_struct *pjcs = (jpeg_compress_struct *)hJpegC;

		JSAMPROW rgrow[1];
		

//
// On non X86 architectures use only C code
//
#if defined (_X86_)
		pjcs->dct_method = JDCT_ISLOW;
#else
		pjcs->dct_method = JDCT_FLOAT;
#endif		
                pjcs->image_width = nWidth;
		pjcs->image_height = nHeight;
		pjcs->data_precision = 8;		/* 8 bits / sample */
		pjcs->bytes_in_buffer = 0;
		pjcs->write_JFIF_header = FALSE;

		if (ColorSpace == JCS_GRAYSCALE)
		{
			pjcs->input_components = 1;
		}
		else
		{
			pjcs->input_components = 4;
		}

		jpeg_set_colorspace(pjcs,ColorSpace);
 
		jpeg_set_quality(pjcs, tuQuality, TRUE);

		jpeg_suppress_tables(pjcs, TRUE);

		jpeg_mem_dest(pjcs, prgbJPEGBuf);	// Init the "destination manager"
		
		jpeg_start_compress(pjcs, FALSE);

		rgrow[0] = (JSAMPROW)prgbImage;

		while (pjcs->next_scanline < nHeight )
		{
			jpeg_write_scanlines(pjcs, rgrow, 1);

			rgrow[0] += nWidth * pjcs->input_components; //input_components is the equivalent of # of bytes
		}

		jpeg_finish_compress(pjcs);		// Finish up compressing

		
		*pcbOut = pjcs->bytes_in_buffer;
	}
	catch( THROWN thrownHR )
	{
		hr = thrownHR.Hr();
	}
	
	return hr;
}

/* RGBAFromJPEG()
 *
 * Arguments:
 *		prgbJPEG: A JPEG data stream, as returned by JPEGFromRGBA()
 *		A memory buffer containing the reconstructed image in RGBA format.
 *		NULL on error.
 *
 * Returns:
 *		HRESULT
*/

HRESULT RGBAFromJPEG(BYTE *prgbJPEG, BYTE *prgbImage, HANDLE hJpegD, ULONG ulBufferSize, BYTE bJPEGConversionType, ULONG *pulReturnedNumChannels, UINT nWidth, UINT nHeight )
{
	HRESULT			hr = S_OK;

	try
	{
		struct jpeg_decompress_struct *pjds;
		jpeg_decompress_struct * spjds = new jpeg_decompress_struct;
		struct jpeg_error_mgr *jem;
		jpeg_error_mgr * spjem = new jpeg_error_mgr;
		
		if ( hJpegD == NULL )
		{
		
			spjds->err = jpeg_std_error(spjem);	// Init the error handler
			jpeg_create_decompress(spjds);		// Init the decompression object in the case
			pjds = spjds;						// that the headers are with the tiles.
			jem = spjem;	
		}
		else if ( hJpegD == NULL )
		{
			// This should never happen.  The decompression header was not set up return an
			// error indication by setting the return value to kpvNil.
			return E_FAIL;
		}
		else
		{
			pjds = (struct jpeg_decompress_struct *)hJpegD;
		}

		JSAMPROW rgrow[1];
			
		// Set the various image parameters.
		pjds->data_precision = 8;
		pjds->image_width = nWidth;
		pjds->image_height = nHeight;
		
		jpeg_mem_src(pjds, prgbJPEG, ulBufferSize);	// Init the "source manager"

		jpeg_read_header(pjds, TRUE);
		
		switch (bJPEGConversionType)
		{
		case 1:
			pjds->out_color_space = JCS_RGBA;
			if (pjds->jpeg_color_space != JCS_RGBA)
			{
				if ( 4 == pjds->num_components) 
					pjds->jpeg_color_space = JCS_YCbCrALegacy;
				else
					pjds->jpeg_color_space = JCS_YCbCr;
			}
			*pulReturnedNumChannels = 4;
			break;
		case 2:
			pjds->out_color_space = JCS_RGBA;

			if ( 4 == pjds->num_components) 
				pjds->jpeg_color_space = JCS_YCbCrA;
			else
				pjds->jpeg_color_space = JCS_YCbCr;

			pjds->jpeg_color_space = JCS_YCbCrA;
			*pulReturnedNumChannels = 4;
			break;
		default:
			pjds->out_color_space = JCS_UNKNOWN; 
			pjds->jpeg_color_space = JCS_UNKNOWN;
			*pulReturnedNumChannels = pjds->num_components;
		}

//
// On non X86 architectures use only C code
//
#if defined (_X86_)
		pjds->dct_method = JDCT_ISLOW;
#else
    		pjds->dct_method = JDCT_FLOAT;
#endif

		jpeg_start_decompress(pjds);

		rgrow[0] = (JSAMPROW)prgbImage;

		while (pjds->output_scanline < pjds->output_height)
		{
			jpeg_read_scanlines(pjds, rgrow, 1);
			rgrow[0] += pjds->output_width * *pulReturnedNumChannels;
		}
		
		jpeg_finish_decompress(pjds);	// Finish up decompressing

		if (hJpegD == NULL)
			jpeg_destroy_decompress(pjds);  //Destroy the decompression object if it
											//was locally allocated as in when the header
											//is part of the tile.
		delete spjem;
                delete spjds;
	}
	catch( THROWN thrownHR )
	{
		hr = thrownHR.Hr();
	}

	return hr;
}
