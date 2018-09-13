/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    win31io.h

Abstract:

    This header file contains the Win 3.1 Group/REG.DAT data
    structure definitions, as well as the 32 bit group definitions.
    Why these aren't in the shell32.dll somewhere I don't know

Author:

    Steve Wood (stevewo) 22-Feb-1993

Revision History:

--*/

#ifndef _WIN31IO_
#define _WIN31IO_

#include "win31evt.h"

typedef struct GROUP_DEF {
    union {
        DWORD   dwMagic;    /* magical bytes 'PMCC' */
        DWORD   dwCurrentSize;  /* During conversion only */
    };
    WORD    wCheckSum;      /* adjust this for zero sum of file */
    WORD    cbGroup;        /* length of group segment (does NOT include tags) */
    RECT    rcNormal;       /* rectangle of normal window */
    POINT   ptMin;          /* point of icon */
    WORD    nCmdShow;       /* min, max, or normal state */
    WORD    pName;          /* name of group */
                            /* these four change interpretation */
    WORD    cxIcon;         /* width of icons */
    WORD    cyIcon;         /* hieght of icons */
    WORD    wIconFormat;    /* planes and BPP in icons */
    WORD    wReserved;      /* This word is no longer used. */
                            /* Used internally to hold total size of group, including tags */
    WORD    cItems;         /* number of items in group */
    WORD    rgiItems[1];    /* array of ITEMDEF offsets */
} GROUP_DEF, *PGROUP_DEF;

#define NSLOTS 16           /* initial number of items entries */

typedef struct ITEM_DEF {
    POINT   pt;             /* location of item icon in group */
    WORD    idIcon;         /* id of item icon */
    WORD    wIconVer;       /* icon version */
    WORD    cbIconRes;      /* size of icon resource */
    WORD    indexIcon;      /* index of item icon */
    WORD    dummy2;         /* - not used anymore */
    WORD    pIconRes;       /* offset of icon resource */
    WORD    dummy3;         /* - not used anymore */
    WORD    pName;          /* offset of name string */
    WORD    pCommand;       /* offset of command string */
    WORD    pIconPath;      /* offset of icon path */
} ITEM_DEF, *PITEM_DEF;


/* the pointers in the above structures are short pointers relative to the
 * beginning of the segments.  This macro converts the short pointer into
 * a long pointer including the proper segment/selector value.        It assumes
 * that its argument is an lvalue somewhere in a group segment, for example,
 * PTR(lpgd->pName) returns a pointer to the group name, but k=lpgd->pName;
 * PTR(k) is obviously wrong as it will use either SS or DS for its segment,
 * depending on the storage class of k.
 */
#define PTR( base, offset ) (LPSTR)((PBYTE)base + offset)

/* this macro is used to retrieve the i-th item in the group segment.  Note
 * that this pointer will NOT be NULL for an unused slot.
 */
#define ITEM( lpgd, i ) ((PITEM_DEF)PTR( lpgd, lpgd->rgiItems[i] ))

#define VER31           0x030A
#define VER30           0x0300
#define VER20           0x0201

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Tag Stuff                                                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

typedef struct _TAG_DEF {
    WORD wID;                   // tag identifier
    WORD dummy1;                // need this for alignment!
    int wItem;                  // (unde the covers 32 bit point!)item the tag belongs to
    WORD cb;                    // size of record, including id and count
    WORD dummy2;                // need this for alignment!
    BYTE rgb[1];
} TAG_DEF, *PTAG_DEF;

#define GROUP_MAGIC 0x43434D50L  /* 'PMCC' */
#define PMTAG_MAGIC GROUP_MAGIC

    /* range 8000 - 80FF > global
     * range 8100 - 81FF > per item
     * all others reserved
     */

#define ID_MAINTAIN             0x8000
    /* bit used to indicate a tag that should be kept even if the writer
     * doesn't recognize it.
     */

#define ID_MAGIC                0x8000
    /* data: the string 'TAGS'
     */

#define ID_WRITERVERSION        0x8001
    /* data: string in the form [9]9.99[Z].99
     */

#define ID_APPLICATIONDIR       0x8101
    /* data: ASCIZ string of directory where application may be
     * located.
     * this is defined as application dir rather than default dir
     * since the default dir is explicit in the 3.0 command line and
     * must stay there.  The true "new information" is the application
     * directory.  If not present, search the path.
     */

#define ID_HOTKEY               0x8102
    /* data: WORD hotkey index
     */

#define ID_MINIMIZE             0x8103
    /* data none
     */

#define ID_LASTTAG              0xFFFF
    /* the last tag in the file
     */

    /*
     * Maximium number of items allowed in a group
     */
#define CITEMSMAX   50


    /*
     * Maximium number of groups allowed in PROGMAN
     */

#define CGROUPSMAX  40

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

//
// This is the structure of the .grp files in Windows3.1
//

/* .GRP File format structures -
 */
typedef struct _GROUP_DEF16 {
    DWORD         dwMagic;      /* magical bytes 'PMCC' */
    WORD          wCheckSum;    /* adjust this for zero sum of file */
    WORD          cbGroup;      /* length of group segment (does NOT include tags) */
    WORD          nCmdShow;     /* min, max, or normal state */
    SMALL_RECT    rcNormal;     /* rectangle of normal window */
    POINTS        ptMin;        /* point of icon */
    WORD          pName;        /* name of group */
                                /* these four change interpretation */
    WORD          cxIcon;       /* width of icons */
    WORD          cyIcon;       /* hieght of icons */
    WORD          wIconFormat;  /* planes and BPP in icons */
    WORD          wReserved;    /* This word is no longer used. */
                                /* Used internally to hold total size of group, including tags */

    WORD          cItems;       /* number of items in group */
    WORD          rgiItems[1];  /* array of ITEMDEF offsets */
} GROUP_DEF16, *PGROUP_DEF16;

/* this macro is used to retrieve the i-th item in the group segment.  Note
 * that this pointer will NOT be NULL for an unused slot.
 */
#define ITEM16( lpgd16, i ) ((PITEM_DEF16)PTR( lpgd16, lpgd16->rgiItems[i] ))

//
// These structures are not needed for the conversion but it is useful to
// understand what is going on.
//
typedef struct _ITEM_DEF16 {
    POINTS    pt;               /* location of item icon in group */
    WORD          iIcon;        /* index of item icon */
    WORD          cbHeader;     /* size of icon header */
    WORD          cbANDPlane;   /* size of and part of icon */
    WORD          cbXORPlane;   /* size of xor part of icon */
    WORD          pHeader;      /* file offset of icon header */
    WORD          pANDPlane;    /* file offset of AND plane */
    WORD          pXORPlane;    /* file offset of XOR plane */
    WORD          pName;        /* file offset of name string */
    WORD          pCommand;     /* file offset of command string */
    WORD          pIconPath;    /* file offset of icon path */
} ITEM_DEF16, *PITEM_DEF16;


typedef struct _CURSORSHAPE_16 {
    WORD xHotSpot;
    WORD yHotSpot;
    WORD cx;
    WORD cy;
    WORD cbWidth;  /* Bytes per row, accounting for word alignment. */
    BYTE Planes;
    BYTE BitsPixel;
} CURSORSHAPE_16, *PCURSORSHAPE_16;



typedef struct _TAG_DEF16 {
    WORD wID;			// tag identifier
    WORD wItem; 		// item the tag belongs to
    WORD cb;			// size of record, including id and count
    BYTE rgb[1];
} TAG_DEF16, *PTAG_DEF16;

typedef struct _ICON_HEADER16 {
    WORD xHotSpot;
    WORD yHotSpot;
    WORD cx;
    WORD cy;
    WORD cbWidth;  /* Bytes per row, accounting for word alignment. */
    BYTE Planes;
    BYTE BitsPixel;
} ICON_HEADER16, *PICON_HEADER16;


#pragma pack(2)

typedef struct _REG_KEY16 {     // key nodes
    WORD iNext;                 // next sibling key
    WORD iChild;                // first child key
    WORD iKey;                  // string defining key
    WORD iValue;                // string defining value of key-tuple
} REG_KEY16, *PREG_KEY16;

typedef struct _REG_STRING16 {
    WORD iNext;                 // next string in chain
    WORD cRef;                  // reference count
    WORD cb;                    // length of string
    WORD irgb;                  // offset in string segment
} REG_STRING16, *PREG_STRING16;

typedef union _REG_NODE16 {     // a node may be...
    REG_KEY16 key;              //      a key
    REG_STRING16 str;           //      a string
} REG_NODE16, *PREG_NODE16;

typedef struct _REG_HEADER16 {
    DWORD dwMagic;              // magic number
    DWORD dwVersion;            // version number
    DWORD dwHdrSize;            // size of header
    DWORD dwNodeTable;          // offset of node table
    DWORD dwNTSize;             // size of node table
    DWORD dwStringValue;        // offset of string values
    DWORD dwSVSize;             // size of string values
    WORD nHash;                 // number of initial string table entries
    WORD iFirstFree;            // first free node
} REG_HEADER16, *PREG_HEADER16;

#define MAGIC_NUMBER 0x43434853L        // 'SHCC'
#define VERSION_NUMBER 0x30312E33L      // '3.10'

#pragma pack()

//
// Routines defined in group32.c
//

ULONG
QueryNumberOfPersonalGroupNames(
    HANDLE CurrentUser,
    PHANDLE GroupNamesKey,
    PHANDLE SettingsKey
    );

BOOL
NewPersonalGroupName(
    HANDLE GroupNamesKey,
    PWSTR GroupName,
    ULONG GroupNumber
    );

BOOL
DoesExistGroup(
    HANDLE GroupsKey,
    PWSTR GroupName
    );

PGROUP_DEF
LoadGroup(
    HANDLE GroupsKey,
    PWSTR GroupFileName
    );

BOOL
UnloadGroup(
    PGROUP_DEF Group
    );


BOOL
ExtendGroup(
    PGROUP_DEF Group,
    BOOL AppendToGroup,
    DWORD cb
    );

WORD
AddDataToGroup(
    PGROUP_DEF Group,
    PBYTE Data,
    DWORD cb
    );

BOOL
AddTagToGroup(
    PGROUP_DEF Group,
    WORD wID,
    WORD wItem,
    WORD cb,
    PBYTE rgb
    );

PGROUP_DEF
CreateGroupFromGroup16(
    LPSTR GroupName,
    PGROUP_DEF16 Group16
    );

BOOL
SaveGroup(
    HANDLE GroupsKey,
    PWSTR GroupName,
    PGROUP_DEF Group
    );

BOOL
DeleteGroup(
    HANDLE GroupsKey,
    PWSTR GroupName
    );

#if DBG
BOOL
DumpGroup(
    PWSTR GroupFileName,
    PGROUP_DEF Group
    );
#endif


//
// Routines defined in group16.c
//

PGROUP_DEF16
LoadGroup16(
    PWSTR GroupFileName
    );

BOOL
UnloadGroup16(
    PGROUP_DEF16 Group
    );

#if DBG
BOOL
DumpGroup16(
    PWSTR GroupFileName,
    PGROUP_DEF16 Group
    );
#endif



//
// Routines defined in regdat16.c
//

PREG_HEADER16
LoadRegistry16(
    PWSTR RegistryFileName
    );

BOOL
UnloadRegistry16(
    PREG_HEADER16 Registry
    );

BOOL
CreateRegistryClassesFromRegistry16(
    HANDLE SoftwareRoot,
    PREG_HEADER16 Registry
    );

#if DBG
BOOL
DumpRegistry16(
    PREG_HEADER16 Registry
    );
#endif


#endif // _WIN31IO_
