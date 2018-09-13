/*
 * api.c
 *
 * Externally declared APIs
 */
#include <stdio.h>
#include <crtdbg.h>

#define DECLARE_DATA
#include "inflate.h"
#include "deflate.h"
#include "api_int.h"
#include "infgzip.h"
#include "fasttbl.h"
#include "crc32.h"


//
// Initialise global compression 
//
HRESULT WINAPI InitCompression(VOID)
{
	inflateInit();
	return S_OK;
}


//
// Initialise global decompression 
//
HRESULT WINAPI InitDecompression(VOID)
{
	deflateInit();
	return S_OK;
}


//
// De-init global compression
//
VOID WINAPI DeInitCompression(VOID)
{
}


//
// De-init global decompression
//
VOID WINAPI DeInitDecompression(VOID)
{
}


//
// Create a compression context
//
HRESULT WINAPI CreateCompression(PVOID *context, ULONG flags)
{
	t_encoder_context *ec;

    *context = (PVOID) LocalAlloc(LMEM_FIXED, sizeof(t_encoder_context));

    if (*context == NULL)
        return E_OUTOFMEMORY;

    ec = (t_encoder_context *) (*context);

    // no encoders initialised yet
	ec->std_encoder     = NULL;
    ec->optimal_encoder = NULL;
    ec->fast_encoder    = NULL;

    if (flags & COMPRESSION_FLAG_DO_GZIP)
        ec->using_gzip      = TRUE;
    else
        ec->using_gzip      = FALSE;

    InternalResetCompression(ec);

	return S_OK;
}


//
// Destroy a compression context
//
VOID WINAPI DestroyCompression(PVOID void_context)
{
    t_encoder_context *context = (t_encoder_context *) void_context;

    _ASSERT(void_context != NULL);

    if (context->std_encoder != NULL)
        LocalFree((PVOID) context->std_encoder);

    if (context->optimal_encoder != NULL)
        LocalFree((PVOID) context->optimal_encoder);

    if (context->fast_encoder != NULL)
        LocalFree((PVOID) context->fast_encoder);

	LocalFree(void_context);
}


//
// Create a decompression context
//
HRESULT WINAPI CreateDecompression(PVOID *context, ULONG flags)
{
	*context = (PVOID) LocalAlloc(LMEM_FIXED, sizeof(t_decoder_context));

    if (*context == NULL)
        return E_OUTOFMEMORY;

    if (flags & DECOMPRESSION_FLAG_DO_GZIP)
        ((t_decoder_context *) (*context))->using_gzip = TRUE;
    else
        ((t_decoder_context *) (*context))->using_gzip = FALSE;

	return ResetDecompression(*context);
}


//
// Destroy decompression context
//
VOID WINAPI DestroyDecompression(PVOID void_context)
{
    LocalFree(void_context);
}


//
// Reset compression context
//
HRESULT WINAPI ResetCompression(PVOID void_context)
{
	t_encoder_context *context = (t_encoder_context *) void_context;

    InternalResetCompression(context);

    // BUGBUG This forces a realloc of the particular compressor we are using
    // each time we reset, but if we don't do this then we are stuck with one
    // compressor (fast,std,optimal) forever until we destroy the context.
    // Should create a workaround for this problem.  Luckily, IIS creates a
    // new context all the time, and doesn't call reset (so says davidtr).
    DestroyIndividualCompressors(context);

	return S_OK;
}


//
// Reset decompression context
//
HRESULT WINAPI ResetDecompression(PVOID void_context)
{
	t_decoder_context *context = (t_decoder_context *) void_context;

    if (context->using_gzip)
    {
    	context->state	= STATE_READING_GZIP_HEADER;
        context->gzip_header_substate = 0;
        DecoderInitGzipVariables(context);
    }
    else
    {
	    context->state	= STATE_READING_BFINAL_NEED_TO_INIT_BITBUF;
    }

    context->bufpos = 0;
	context->bitcount = -16;

	return S_OK;
}
