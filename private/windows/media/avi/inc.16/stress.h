/*****************************************************************************\
*                                                                             *
* stress.h -    Stress functions definitions                                  *
*                                                                             *
* Version 1.0								      *
*                                                                             *
* Copyright (c) 1992-1994, Microsoft Corp. All rights reserved.		      *
*                                                                             *
*******************************************************************************/

#ifndef _INC_STRESS
#define _INC_STRESS

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

/****** Simple types & common helper macros *********************************/

#ifndef _INC_WINDOWS    /* If included with 3.0 headers... */
#define UINT        WORD
#define WINAPI      FAR PASCAL
#endif  /* _INC_WINDOWS */

/* stuff for AllocDiskSpace() */
#define  EDS_WIN     1
#define  EDS_CUR     2
#define  EDS_TEMP    3


/* function prototypes */
BOOL    WINAPI AllocMem(DWORD);
void    WINAPI FreeAllMem(void);
int     WINAPI AllocFileHandles(int);
void    WINAPI UnAllocFileHandles(void);
int     WINAPI GetFreeFileHandles(void);
int     WINAPI AllocDiskSpace(long,UINT);
void    WINAPI UnAllocDiskSpace(UINT);
BOOL    WINAPI AllocUserMem(UINT);
void    WINAPI FreeAllUserMem(void);
BOOL    WINAPI AllocGDIMem(UINT);
void    WINAPI FreeAllGDIMem(void);

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* RC_INVOKED */

#endif  /* _INC_STRESS */
