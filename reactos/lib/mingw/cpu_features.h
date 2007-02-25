#ifndef _CPU_FEATURES_H
#define _CPU_FEATURES_H

#include <stdbool.h>

#define  _CRT_CMPXCHG8B		0x0001
#define  _CRT_CMOV		0x0002
#define  _CRT_MMX		0x0004
#define  _CRT_FXSR		0x0008
#define  _CRT_SSE		0x0010
#define  _CRT_SSE2		0x0020
#define  _CRT_SSE3		0x0040
#define  _CRT_CMPXCHG16B	0x0080
#define  _CRT_3DNOW		0x0100
#define  _CRT_3DNOWP		0x0200

extern unsigned int __cpu_features;

/* Currently we use this in fpenv  functions */
#define __HAS_SSE  __cpu_features & _CRT_SSE

void  __cpu_features_init (void);


#endif
