/***************************************************************************
 *
 *  compdef.h
 *
 *  Copyright (c) 1993-1994  Microsoft Corporation
 *
 *  32-bit Thunks for msvideo.dll
 *
 *  Structures for mapping compression manager calls
 *
 **************************************************************************/

/**************************************************************************\

 Thunking of compman APIs

 Functions:

    ICInstall is NOT supported from 16-bit to 32-bit

    ICRemove is also NOT supported

    ICInfo - tries the 32-bit side first

    Handles

        Are pointers to the PIC table.

        The 16-bit side handle manager will store these and call us back
        with our 32-bit handles so the thunking routines here receive
        real 32-bit handles.

        There is NO handle cleanup (why not ???) on task termination

/*
 *  Make sure the compiler doesn't think it knows better about packing
 *  The 16-bit stack is effectively pack(2)
 */

#pragma pack(2)

/*
 *  Note that everything is in the reverse order to keep with the PASCAL
 *  calling convention on the other side
 */


/****************************************************************************

   compman entry point parameter lists

 ****************************************************************************/

typedef struct {
    DWORD   dwSize;                 // sizeof(ICINFOA)
    DWORD   fccType;                // compressor type     'vidc' 'audc'
    DWORD   fccHandler;             // compressor sub-type 'rle ' 'jpeg' 'pcm '
    DWORD   dwFlags;                // flags LOWORD is type specific
    DWORD   dwVersion;              // version of the driver
    DWORD   dwVersionICM;           // version of the ICM used
    char    szName[16];             // short name
    char    szDescription[128];     // long name
    char    szDriver[128];          // driver that contains compressor
}   ICINFO16;

#define ICINFOA ICINFO16

typedef struct {
#ifdef ICINFOA
    ICINFOA  *lpicinfo;  // Why is there no ASCII 32-bit API?
#else
    ICINFO   *lpicinfo;  // Why is there no ASCII 32-bit API?
#endif
    DWORD    fccHandler;
    DWORD    fccType;
} UNALIGNED *PICInfoParms16;

typedef struct {
    DWORD    dw2;
    DWORD    dw1;
    WORD     msg;
    DWORD    hic;
} UNALIGNED *PICSendMessageParms16;

typedef struct {
    WORD     wMode;
    DWORD    fccHandler;
    DWORD    fccType;
} UNALIGNED *PICOpenParms16;

typedef struct {
    DWORD    hic;
} UNALIGNED *PICCloseParms16;

typedef struct {
    DWORD    dwFlags;
    WORD     hpal;
    WORD     hwnd;
    WORD     hdc;
    short    xDst;
    short    yDst;
    short    dxDst;
    short    dyDst;
    LPBITMAPINFOHEADER lpbi;
    short    xSrc;
    short    ySrc;
    short    dxSrc;
    short    dySrc;
    DWORD    dwRate;
    DWORD    dwScale;
} ICDRAWBEGIN16;

#pragma pack()


