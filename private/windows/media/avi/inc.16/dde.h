/*****************************************************************************\
*                                                                             *
* dde.h -       Dynamic Data Exchange structures and definitions              *
*                                                                             *
* Copyright (c) 1992-1994, Microsoft Corp.	All rights reserved	      *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_DDE
#define _INC_DDE

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

/* DDE window messages */

#define WM_DDE_FIRST	    0x03E0
#define WM_DDE_INITIATE     (WM_DDE_FIRST)
#define WM_DDE_TERMINATE    (WM_DDE_FIRST+1)
#define WM_DDE_ADVISE	    (WM_DDE_FIRST+2)
#define WM_DDE_UNADVISE     (WM_DDE_FIRST+3)
#define WM_DDE_ACK	    (WM_DDE_FIRST+4)
#define WM_DDE_DATA	    (WM_DDE_FIRST+5)
#define WM_DDE_REQUEST	    (WM_DDE_FIRST+6)
#define WM_DDE_POKE	    (WM_DDE_FIRST+7)
#define WM_DDE_EXECUTE	    (WM_DDE_FIRST+8)
#define WM_DDE_LAST	    (WM_DDE_FIRST+8)

/****************************************************************************\
*       DDEACK structure
*
*       Structure of wStatus (LOWORD(lParam)) in WM_DDE_ACK message
*       sent in response to a WM_DDE_DATA, WM_DDE_REQUEST, WM_DDE_POKE,
*       WM_DDE_ADVISE, or WM_DDE_UNADVISE message.
*
\****************************************************************************/

typedef struct tagDDEACK
{
    WORD    bAppReturnCode:8,
            reserved:6,
            fBusy:1,
            fAck:1;
} DDEACK;

/****************************************************************************\
*       DDEADVISE structure
*
*       WM_DDE_ADVISE parameter structure for hOptions (LOWORD(lParam))
*
\****************************************************************************/

typedef struct tagDDEADVISE
{
    WORD    reserved:14,
            fDeferUpd:1,
            fAckReq:1;
    short   cfFormat;
} DDEADVISE;

/****************************************************************************\
*       DDEDATA structure
*
*       WM_DDE_DATA parameter structure for hData (LOWORD(lParam)).
*       The actual size of this structure depends on the size of
*       the Value array.
*
\****************************************************************************/

typedef struct tagDDEDATA
{
    WORD    unused:12,
            fResponse:1,
            fRelease:1,
            reserved:1,
            fAckReq:1;
    short   cfFormat;
    BYTE     Value[1];
} DDEDATA;


/****************************************************************************\
*       DDEPOKE structure
*
*       WM_DDE_POKE parameter structure for hData (LOWORD(lParam)).
*       The actual size of this structure depends on the size of
*       the Value array.
*
\****************************************************************************/

typedef struct tagDDEPOKE
{
    WORD    unused:13,  /* Earlier versions of DDE.H incorrectly */
                        /* 12 unused bits.                       */
            fRelease:1,
            fReserved:2;
    short   cfFormat;
    BYTE    Value[1];   /* This member was named rgb[1] in previous */
                        /* versions of DDE.H                        */

} DDEPOKE;

/****************************************************************************\
* The following typedef's were used in previous versions of the Windows SDK.
* They are still valid.  The above typedef's define exactly the same structures
* as those below.  The above typedef names are recommended, however, as they
* are more meaningful.
*
* Note that the DDEPOKE structure typedef'ed in earlier versions of DDE.H did
* not correctly define the bit positions.
\****************************************************************************/

typedef struct tagDDELN
{
    WORD    unused:13,
            fRelease:1,
            fDeferUpd:1,
            fAckReq:1;
    short   cfFormat;
} DDELN;

typedef struct tagDDEUP
{
    WORD    unused:12,
            fAck:1,
            fRelease:1,
            fReserved:1,
            fAckReq:1;
    short   cfFormat;
    BYTE    rgb[1];
} DDEUP;

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* RC_INVOKED */

#endif  /* _INC_DDE */
