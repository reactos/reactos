/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        apps/cabman/raw.cpp
 * PURPOSE:     CAB codec for uncompressed "raw" data
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 21/03-2001 Created
 */
#include <windows.h>
#include "raw.h"


/* CRawCodec */

CRawCodec::CRawCodec()
/*
 * FUNCTION: Default constructor
 */
{
}


CRawCodec::~CRawCodec()
/*
 * FUNCTION: Default destructor
 */
{
}


ULONG CRawCodec::Compress(PVOID OutputBuffer,
                          PVOID InputBuffer,
                          DWORD InputLength,
                          PDWORD OutputLength)
/*
 * FUNCTION: Compresses data in a buffer
 * ARGUMENTS:
 *     OutputBuffer = Pointer to buffer to place compressed data
 *     InputBuffer  = Pointer to buffer with data to be compressed
 *     InputLength  = Length of input buffer
 *     OutputLength = Address of buffer to place size of compressed data
 */
{
    CopyMemory(OutputBuffer, InputBuffer, InputLength);
    *OutputLength = InputLength;
    return CS_SUCCESS;
}


ULONG CRawCodec::Uncompress(PVOID OutputBuffer,
                            PVOID InputBuffer,
                            DWORD InputLength,
                            PDWORD OutputLength)
/*
 * FUNCTION: Uncompresses data in a buffer
 * ARGUMENTS:
 *     OutputBuffer = Pointer to buffer to place uncompressed data
 *     InputBuffer  = Pointer to buffer with data to be uncompressed
 *     InputLength  = Length of input buffer
 *     OutputLength = Address of buffer to place size of uncompressed data
 */
{
    CopyMemory(OutputBuffer, InputBuffer, InputLength);
    *OutputLength = InputLength;
    return CS_SUCCESS;
}

/* EOF */
