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
	virtual unsigned long Compress(void* OutputBuffer,
		void* InputBuffer,
		unsigned long InputLength,
		unsigned long* OutputLength);
	/* Uncompresses a data block */
	virtual unsigned long Uncompress(void* OutputBuffer,
		void* InputBuffer,
		unsigned long InputLength,
		unsigned long* OutputLength);
};

#endif /* __RAW_H */

/* EOF */
