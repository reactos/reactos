//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       comp.hxx
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    04-16-97   DanpoZ (Danpo Zhang)   Created
//
//----------------------------------------------------------------------------
#ifndef COMP_HXX_
#define COMP_HXX_

#include <windows.h>

#if 0
#define DllImport    __declspec( dllimport )
#else
#define DllImport
#endif
#define DllExport    __declspec( dllexport )

extern "C" {

    typedef HRESULT (WINAPI *PFNCODEC_INIT_COMPRESSION)(VOID);
    typedef HRESULT (WINAPI *PFNCODEC_INIT_DECOMPRESSION)(VOID);

    typedef VOID (WINAPI *PFNCODEC_DEINIT_COMPRESSION)(VOID);
    typedef VOID (WINAPI *PFNCODEC_DEINIT_DECOMPRESSION)(VOID);

    typedef HRESULT (WINAPI *PFNCODEC_CREATE_COMPRESSION)(PVOID *context);
    typedef HRESULT (WINAPI *PFNCODEC_CREATE_DECOMPRESSION)(PVOID *context);

    typedef HRESULT (WINAPI *PFNCODEC_COMPRESS)(
	    PVOID		context, 
	    CONST PBYTE	input, 
	    LONG		input_size, 
	    PBYTE		output, 
	    LONG		output_size,
	    PLONG		input_used,
	    PLONG		output_used,
	    INT			compression_level
    );

    typedef HRESULT (WINAPI *PFNCODEC_DECOMPRESS)(
	    PVOID		context,
	    CONST PBYTE	input,
	    LONG		input_size,
	    PBYTE		output,
	    LONG		output_size,
	    PLONG		input_used,
	    PLONG		output_used
    );

    typedef VOID (WINAPI *PFNCODEC_DESTROY_COMPRESSION)(PVOID);
    typedef VOID (WINAPI *PFNCODEC_DESTROY_DECOMPRESSION)(PVOID);

    typedef HRESULT (WINAPI *PFNCODEC_RESET_COMPRESSION)(PVOID);
    typedef HRESULT (WINAPI *PFNCODEC_RESET_DECOMPRESSION)(PVOID);
}

#endif
