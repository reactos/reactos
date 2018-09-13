/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* 
 * MEDBITS.H
 * 
 * Contains definition of the change structure for MBIT and HDIB
 * resources.  It should be used by all users of these resource types.
 * 
 * This file requires "windows.h" and "mediaman.h"
 */

#ifndef _MEDBITS_H_
#define _MEDBITS_H_

/*  Defintions that windows forgot  */
typedef RGBQUAD FAR	*LPRGBQUAD;
typedef RGBQUAD NEAR	*NPRGBQUAD;
typedef PALETTEENTRY NEAR *NPPALETTEENTRY;

/*
 *  MDIB HANDLER
 */

#define medtypeMDIB	medFOURCC('M', 'D', 'I', 'B')

/*  Associated physical handlers  */
#define medtypeRDIB	medFOURCC('R', 'D', 'I', 'B')
#define medtypePCX	medFOURCC('P', 'C', 'X', ' ')
#define medtypePICT	medFOURCC('P', 'I', 'C', 'T')
#define medtypeGIF	medFOURCC('G', 'I', 'F', ' ')
#define medtypeTGA	medFOURCC('T', 'G', 'A', ' ')
#define medtypeRLE	medFOURCC('R', 'L', 'E', ' ')
#define medtypeRRLE	medFOURCC('R', 'R', 'L', 'E')


/* 
 * Structure used for creation of MDIB resources.
 */
typedef struct _MDIBCreateStruct {
	DWORD		dwWidth;
	DWORD		dwHeight;
	WORD		wDepth;
	MEDID		medidPalette;
	WORD		wPalSize;
	BOOL		fRGBQuads;
	LPPALETTEENTRY	lpPalEntries;
} MDIBCreateStruct;
typedef MDIBCreateStruct FAR *FPMDIBCreateStruct;

#define mdibMAXPALETTESIZE	256

/*  MDIB messages  */
#define	MDIB_GETPALETTE		(MED_USER + 1)
#define MDIB_GETPALSIZE		(MED_USER + 2)
#define MDIB_GETPALMEDID	(MED_USER + 3)
#define MDIB_SETPALMEDID	(MED_USER + 4)
#define MDIB_SETSIZE		(MED_USER + 6)
#define MDIB_SETDEPTH		(MED_USER + 7)
#define MDIB_REMAP		(MED_USER + 8)

/*  Flags for MDIB_SETPALMEDID  */
#define MDIBSP_COPYPALRES	0x0001

// This flag was never implemented.  It never will be.  (davidmay 12/18/90)
// /*  Flags for accessing MDIB resources  */
// #define MDIBLOAD_NOYIELDING	0x0001

/*  MedUser-Notification messages for MDIB handler  */
#define MDIBCH_SIZE		(MED_USER + 2)
#define MDIBCH_DEPTH		(MED_USER + 3)
#define MDIBCH_REMAP		(MED_USER + 4)
#define MDIBCH_NEWPALMEDID	(MED_USER + 5)
#define MDIBCH_PALCHANGE	(MED_USER + 6)


/********************************************************/

/*
 *  MPAL HANDLER
 */

#define medtypeMPAL	medFOURCC('M','P','A','L')
#define medtypeDIBP	medFOURCC('D','I','B','P')
#define medtypeRDBP	medFOURCC('R','D','B','P')

typedef struct _MPALCreateStruct {
	HPALETTE	hPalette;
	WORD		wSize;
	BOOL		fRGBQuads;
	LPSTR		lpPalEntries;
} MPALCreateStruct;
typedef MPALCreateStruct FAR *FPMPALCreateStruct;
typedef MPALCreateStruct NEAR *NPMPALCreateStruct;
	

#define	MPAL_COPYPAL	(MED_USER + 1)	// return GDI copy of palette.
#define MPAL_GETPALETTE	(MED_USER + 2)	// get the current GDI palette object.
#define MPAL_SETPALETTE	(MED_USER + 3)	// set to new GDI palette. remaps
#define MPAL_GETPALSIZE	(MED_USER + 4)

#define MPAL_APPEND	(MED_USER + 5)
#define MPAL_DELETE	(MED_USER + 6)
#define MPAL_REPLACE	(MED_USER + 7)
#define MPAL_MOVE	(MED_USER + 8)

/*  Structures for MPAL_REPLACE and MPAL_DELETE  */
typedef struct {
	int		iEntryIndex;
	int		iNewIndex;
	WORD		wFlags;
} MPALEntry;
typedef MPALEntry FAR *FPMPALEntry;
typedef MPALEntry NEAR *NPMPALEntry;

#define MPALENT_CLOSEST		1
#define MPALENT_OLDINDEX	2
#define MPALENT_REPLACE		3

typedef struct {
	WORD		wNumEntries;
	MPALEntry	aDeletedEntries[1];
} MPALDeleteStruct;
typedef MPALDeleteStruct NEAR *NPMPALDeleteStruct;
typedef MPALDeleteStruct FAR *FPMPALDeleteStruct;

typedef struct {
	WORD		wNumEntries;
	WORD		wInsertPoint;
	WORD		wMovedEntries[1];
} MPALMoveStruct;
typedef MPALMoveStruct NEAR *NPMPALMoveStruct;
typedef MPALMoveStruct FAR *FPMPALMoveStruct;

typedef struct {
	WORD		wNumEntries;
	WORD		wInsertPoint;
	PALETTEENTRY	aInsertedColors[1];
} MPALReplaceStruct;
typedef MPALReplaceStruct NEAR *NPMPALReplaceStruct;
typedef MPALReplaceStruct FAR *FPMPALReplaceStruct;


/*  Change messages from/for the palette resource  */
#define MPALCH_CHANGE		(MED_USER + 1)

typedef struct {
	WORD		wNumNewEntries;
	WORD		wNumOldEntries;	// size of array
	HPALETTE	hPalette;
	WORD		wAction;
	WORD		wEntries[1];
} MPALRemapStruct;
typedef MPALRemapStruct NEAR *NPMPALRemapStruct;
typedef MPALRemapStruct FAR *FPMPALRemapStruct;

/*  Flags for hibyte of wEntries array in MPALChangeStruct  */
#define MPALR_FLAGMASK	0xff00
#define MPALR_ADD	0x8000
#define MPALR_DELETE	0x4000		// contains closest match index
#define MPALR_MOVE	0x2000		// contains new index
#define MPALR_EDIT	0x1000
#define MPALR_NEWOBJECT	0x0800

#endif  /*  _MEDBITS_H_  */
