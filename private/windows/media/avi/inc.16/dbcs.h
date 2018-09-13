/*****************************************************************************\
*                                                                             *
* dbcs.h - DBCS functions prototypes for DOS apps.			      *
*									      *
* Version 1.0								      *
*                                                                             *
* Copyright (c) 1993-1994, Microsoft Corp.	All rights reserved.	      *
*                                                                             *
\*****************************************************************************/

extern int IsDBCSLeadByte(unsigned char uch);
extern unsigned char far *AnsiNext(unsigned char far *puch);
extern unsigned char far *AnsiPrev(unsigned char far *psz, unsigned char far *puch);
