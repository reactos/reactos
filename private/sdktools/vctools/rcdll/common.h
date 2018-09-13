/*++

(C) Copyright Microsoft Corporation 1988-1992

Module Name:

    common.h

Author:

    Floyd A Rogers 2/7/92

Revision History:
        Floyd Rogers
        Created
--*/

#define IN
#define OUT
#define INOUT

//
// An ID_WORD indicates the following WORD is an ordinal rather 
// than a string
// 

#define ID_WORD 0xffff

typedef struct _STRING {
        DWORD discriminant;       // long to make the rest of the struct aligned
	union u {
		struct {
		  struct _STRING *pnext;
                  DWORD  ulOffsetToString;
		  USHORT cbD;
		  USHORT cb;
		  WCHAR  sz[1];
		} ss;
		WORD     Ordinal;
	} uu;
} STRING, *PSTRING, **PPSTRING;

#define IS_STRING 1
#define IS_ID     2

// defines to make deferencing easier
#define OffsetToString uu.ss.ulOffsetToString
#define cbData         uu.ss.cbD
#define cbsz           uu.ss.cb
#define szStr          uu.ss.sz
#define pn             uu.ss.pnext

typedef struct _RESNAME {
    struct _RESNAME *pnext;    // The first three fields should be the
    PSTRING Name;              // same in both res structures
    DWORD   OffsetToData;      //

    PSTRING Type;
    struct _RESNAME *pnextRes;
    RESADDITIONAL	*pAdditional;
    DWORD   OffsetToDataEntry;
    USHORT  ResourceNumber;
    USHORT  NumberOfLanguages;
    POBJLST pObjLst;
} RESNAME, *PRESNAME, **PPRESNAME;

typedef struct _RESTYPE {
    struct _RESTYPE *pnext;    // The first three fields should be the
    PSTRING Type;              // same in both res structures
    DWORD   OffsetToData;      //

    struct _RESNAME *NameHeadID;
    struct _RESNAME *NameHeadName;
    DWORD  NumberOfNamesID;
    DWORD  NumberOfNamesName;
    POBJLST pObjLst;
} RESTYPE, *PRESTYPE, **PPRESTYPE;
