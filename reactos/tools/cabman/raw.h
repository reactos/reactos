/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        tools/cabman/raw.h
 * PURPOSE:     CAB codec for uncompressed data
 */
#ifndef __RAW_H
#define __RAW_H

#include "cabinet.h"


/* Classes */

class CRawCodec : public CCABCodec {
public:
	/* Default constructor */
	CRawCodec();
	/* Default destructor */
	virtual ~CRawCodec();
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
};

#endif /* __RAW_H */

/* EOF */
