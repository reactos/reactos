/*	File: foo.h (Created: 01-Nov-1991)
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

// #define	FOO_MACRO	1

#define	FB_SIZE	2048

struct stFooBuffer
	{
	unsigned int usSend;
	unsigned char acSend[FB_SIZE];
	};

typedef	struct stFooBuffer	stFB;

#if defined(FOO_MACRO)

#define	fooComSendChar(h,pB,c)				\
	{										\
	stFB *xyxz; xyxz = (pB);				\
	xyxz->acSend[xyxz->usSend++]=(c);		\
	if (xyxz->usSend>=FB_SIZE)				\
		{									\
		ComSndBufrSend((h),					\
					(void *)xyxz->acSend,	\
					xyxz->usSend,			\
					100);					\
		xyxz->usSend=0;						\
		}									\
	}										\

#else

extern int fooComSendChar(HCOM h, stFB *pB, BYTE c);

#endif

extern int fooComSendClear(HCOM h, stFB *pB);

extern int fooComSendPush(HCOM h, stFB *pB);

extern int fooComSendCharNow(HCOM h, stFB *pB, BYTE c);

