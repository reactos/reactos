//
// Copyright (c) 1997-1999 Microsoft Corporation.
//
#ifdef BUILD_ON_WINNT
/*
 * To avoid multipule definition, this is already defined in w31jeudc.cpp
 */
#else
/*
 File Structure */

struct W31_Header {
	char	identify[72];
	short	segCnt;		/* ??? */
unsigned short	sCode,
		eCode;
	short	cCnt;
	long	ofsCmap;
	short	sizCmap;
	long	ofsFil;
	short	sizFil;
	long	ofsStbl;	/* search tbl*/
	short	sizStbl;
	long	ofsBdatSub;
	};

struct BDatSubTbl {
	long	tail;
	long	ptrOfs;
	long	head;
	short	filler2;
	/* Following Pointer tbl. */
	};
struct BMPHeader {
	long	bitmapSiz;
	short	xsiz, ysiz;
	};
#endif // BUILD_ON_WINNT

extern int OpenW31JEUDC( TCHAR *path);
extern void CloseW31JEUDC();
extern int  GetW31JEUDCFont(int  rec, LPBYTE buf,int  bufsiz,int  *xsiz,int  *ysiz);
extern int  PutW31JEUDCFont(int  rec, LPBYTE buf,int  xsiz,int  ysiz);
/* EOF */
