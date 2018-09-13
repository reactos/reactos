/*++

(C) Copyright Microsoft Corporation 1988-1992

Module Name:

    updres.h

Author:

    Floyd A Rogers 2/7/92

Revision History:
        Floyd Rogers
        Created
--*/

#define	DEFAULT_CODEPAGE	1252
#define	MAJOR_RESOURCE_VERSION	4
#define	MINOR_RESOURCE_VERSION	0

#define BUTTONCODE	0x80
#define EDITCODE	0x81
#define STATICCODE	0x82
#define LISTBOXCODE	0x83
#define SCROLLBARCODE	0x84
#define COMBOBOXCODE	0x85

#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2
#define	MAXSTR		(256+1)

//
// An ID_WORD indicates the following WORD is an ordinal rather
// than a string
//

#define ID_WORD 0xffff

//typedef	WCHAR	*PWCHAR;

typedef struct MY_STRING {
	ULONG discriminant;       // long to make the rest of the struct aligned
	union u {
		struct {
		  struct MY_STRING *pnext;
		  ULONG  ulOffsetToString;
		  USHORT cbD;
		  USHORT cb;
		  WCHAR  *sz;
		} ss;
		WORD     Ordinal;
	} uu;
} SDATA, *PSDATA, **PPSDATA;

#define IS_STRING 1
#define IS_ID     2

// defines to make deferencing easier
#define OffsetToString uu.ss.ulOffsetToString
#define cbData         uu.ss.cbD
#define cbsz           uu.ss.cb
#define szStr          uu.ss.sz

typedef struct _RESNAME {
        struct _RESNAME *pnext;	// The first three fields should be the
        PSDATA Name;		// same in both res structures
        ULONG   OffsetToData;

        PSDATA	Type;
	ULONG	SectionNumber;
        ULONG	DataSize;
        ULONG_PTR   OffsetToDataEntry;
        USHORT  ResourceNumber;
        USHORT  NumberOfLanguages;
        WORD	LanguageId;
} RESNAME, *PRESNAME, **PPRESNAME;

typedef struct _RESTYPE {
        struct _RESTYPE *pnext;	// The first three fields should be the
        PSDATA Type;		// same in both res structures
        ULONG   OffsetToData;

        struct _RESNAME *NameHeadID;
        struct _RESNAME *NameHeadName;
        ULONG  NumberOfNamesID;
        ULONG  NumberOfNamesName;
} RESTYPE, *PRESTYPE, **PPRESTYPE;

typedef struct _UPDATEDATA {
        ULONG	cbStringTable;
        PSDATA	StringHead;
        PRESNAME	ResHead;
        PRESTYPE	ResTypeHeadID;
        PRESTYPE	ResTypeHeadName;
        LONG	Status;
        HANDLE	hFileName;
} UPDATEDATA, *PUPDATEDATA;

//
// Round up a byte count to a power of 2:
//
#define ROUNDUP(cbin, align) (((cbin) + (align) - 1) & ~((align) - 1))

//
// Return the remainder, given a byte count and a power of 2:
//
#define REMAINDER(cbin,align) (((align)-((cbin)&((align)-1)))&((align)-1))

#define CBLONG		(sizeof(LONG))
#define BUFSIZE		(4L * 1024L)

/* functions for adding/deleting resources to update list */

LONG
AddResource(
    IN PSDATA Type,
    IN PSDATA Name,
    IN WORD Language,
    IN PUPDATEDATA pupd,
    IN PVOID lpData,
    IN ULONG  cb
    );

PSDATA
AddStringOrID(
    LPCWSTR     lp,
    PUPDATEDATA pupd
    );

BOOL
InsertResourceIntoLangList(
    PUPDATEDATA pUpd,
    PSDATA Type,
    PSDATA Name,
    PRESTYPE pType,
    PRESNAME pName,
    INT	idLang,
    INT	fName,
    INT cb,
    PVOID lpData
    );

BOOL
DeleteResourceFromList(
    PUPDATEDATA pUpd,
    PRESTYPE pType,
    PRESNAME pName,
    INT	idLang,
    INT	fType,
    INT	fName
    );

/* Prototypes for Enumeration done in BeginUpdateResource */

BOOL
EnumTypesFunc(
    HANDLE hModule,
    LPWSTR lpType,
    LPARAM lParam
    );

BOOL
EnumNamesFunc(
    HANDLE hModule,
    LPWSTR lpName,
    LPWSTR lpType,
    LPARAM lParam
    );

BOOL
EnumLangsFunc(
    HANDLE hModule,
    LPWSTR lpType,
    LPWSTR lpName,
    WORD languages,
    LPARAM lParam
    );

/* Prototypes for genral worker functions in updres.c */

LONG
WriteResFile(
    IN HANDLE	hUpdate,
    IN WCHAR	*pDstname
    );

VOID
FreeData(
    PUPDATEDATA pUpd
    );

PRESNAME
WriteResSection(
    PUPDATEDATA pUpdate,
    INT outfh,
    ULONG align,
    ULONG cbLeft,
    PRESNAME pResSave
    );

LONG
PatchRVAs(
    int	inpfh,
    int	outfh,
    PIMAGE_SECTION_HEADER po32,
    ULONG pagedelta,
    PIMAGE_NT_HEADERS pNew,
    ULONG OldSize);

LONG
PatchDebug(
    int	inpfh,
    int	outfh,
    PIMAGE_SECTION_HEADER po32DebugOld,
    PIMAGE_SECTION_HEADER po32DebugNew,
    PIMAGE_SECTION_HEADER po32DebugDirOld,
    PIMAGE_SECTION_HEADER po32DebugDirNew,
    PIMAGE_NT_HEADERS pOld,
    PIMAGE_NT_HEADERS pNew,
    ULONG ibMaxDbgOffsetOld,
    PULONG pPointerToRawData);
