
/* jpegapi.h -- header file for JPEG image compression interface.
 * Written by Ajai Sehgal 
 * (c) Copyright Microsoft Corporation
 *
 *	08-27-1997 (kurtgeis) Removed dependency on Gromit/Fletcher funky data
 *	types and includes.
 */

#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"		/* get library error codes too */

#ifndef __JPEGAPI_H__
#define __JPEGAPI_H__

// kurtgeis: Moved out of Fletcher code base.  Pushed exception handling down
// into the library, so that the actual calls could return HRESULTs.
// Class to wrap a thrown HRESULT
class THROWN
{
public:
	// Default constructor
	THROWN()
	{
		m_hr = S_OK;
	}

	// THROWN( HRESULT)
	//
	// Purpose:
	//		Construct a throw object for an hresult.
	//
	THROWN( HRESULT hr )
	{
		m_hr = hr;
	}

	HRESULT Hr() { return m_hr; }		// The HRESULT thrown
	
 private:
 	HRESULT m_hr;						// Associated HResult;
};

// Destroy the JPEG handle
HRESULT DestroyJPEGCompressHeader(HANDLE hJpegC);
HRESULT DestroyJPEGDecompressHeader(HANDLE hJpegD);

// Takes the parameters for a tile write and creates a JPEG table for it
HRESULT JPEGCompressHeader(BYTE *prgbJPEGBuf, UINT tuQuality, ULONG *pcbOut, HANDLE *phJpegC, J_COLOR_SPACE ColorSpace );
HRESULT JPEGDecompressHeader(BYTE *prgbJPEGBuf, HANDLE *phJpegD, ULONG ulBufferSize );

// Takes a raw RGBA image buffer and spits back a JPEG data stream.
HRESULT JPEGFromRGBA(BYTE *prgbImage, BYTE *prgbJPEGBuf,UINT tuQuality, ULONG *pcbOut, HANDLE hJpegC,J_COLOR_SPACE ColorSpace, UINT nWidth, UINT nHeight );

// Takes a JPEG data stream and spits back a raw RGBA image buffer.
// iraklis's comment: the second argument is the RGBA buffer to be 
// loaded with the decompressed tile; we are 
// asserting that it is of the right size (i.e. sizeof (TIL))
HRESULT RGBAFromJPEG(BYTE *prgbJPEG, BYTE *prgbImage, HANDLE hJpegD, ULONG ulBufferSize, BYTE bJPEGConversions, ULONG *pulReturnedNumChannels, UINT nWidth, UINT nHeight );


#endif	// __JPEGAPI_H__
