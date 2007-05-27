/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        tools/cabman/mszip.h
 * PURPOSE:     CAB codec for MSZIP compressed data
 */
#ifndef __MSZIP_H
#define __MSZIP_H

#include "cabinet.h"
#include <zlib.h>

#define MSZIP_MAGIC 0x4B43


/* Classes */

class CMSZipCodec : public CCABCodec {
public:
	/* Default constructor */
	CMSZipCodec();
	/* Default destructor */
	virtual ~CMSZipCodec();
	/* Compresses a data block */
	virtual uint32_t Compress(void* OutputBuffer,
	                          void* InputBuffer,
	                          uint32_t InputLength,
	                          uint32_t* OutputLength);
	/* Uncompresses a data block */
	virtual uint32_t Uncompress(void* OutputBuffer,
	                            void* InputBuffer,
	                            uint32_t InputLength,
	                            uint32_t* OutputLength);
private:
	int Status;
	z_stream ZStream; /* Zlib stream */
};

#endif /* __MSZIP_H */

/* EOF */
