/*++ BUILD Version: 0002    // Increment this if a change has global effects

/****************************************************************************/
/*                                                                          */
/*  CONVGRP.H -                                                             */
/*                                                                          */
/*      Conversion from Win3.1 16 bit .grp file to NT 32bit .grp files for  */
/*      the Program Manager                                                 */
/*                                                                          */
/*  Created: 10-15-92   Johanne Caron                                       */
/*                                                                          */
/****************************************************************************/
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <windows.h>



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Typedefs                                                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*
 * .GRP File format structures -
 */
typedef struct tagGROUPDEF {
    DWORD   dwMagic;        /* magical bytes 'PMCC' */
    WORD    wCheckSum;      /* adjust this for zero sum of file */
    WORD    cbGroup;        /* length of group segment */
    RECT    rcNormal;       /* rectangle of normal window */
    POINT   ptMin;          /* point of icon */
    WORD    nCmdShow;       /* min, max, or normal state */
    WORD    pName;          /* name of group */
                            /* these four change interpretation */
    WORD    cxIcon;         /* width of icons */
    WORD    cyIcon;         /* hieght of icons */
    WORD    wIconFormat;    /* planes and BPP in icons */
    WORD    wReserved;      /* This word is no longer used. */

    WORD    cItems;         /* number of items in group */
    WORD    rgiItems[1];    /* array of ITEMDEF offsets */
} GROUPDEF, *PGROUPDEF;
typedef GROUPDEF *LPGROUPDEF;

typedef struct tagITEMDEF {
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
} ITEMDEF, *PITEMDEF;
typedef ITEMDEF *LPITEMDEF;


/* the pointers in the above structures are short pointers relative to the
 * beginning of the segments.  This macro converts the short pointer into
 * a long pointer including the proper segment/selector value.        It assumes
 * that its argument is an lvalue somewhere in a group segment, for example,
 * PTR(lpgd->pName) returns a pointer to the group name, but k=lpgd->pName;
 * PTR(k) is obviously wrong as it will use either SS or DS for its segment,
 * depending on the storage class of k.
 */
#define PTR(base, offset) (LPSTR)((PBYTE)base + offset)

/* PTR2 is used for those cases where a variable already contains an offset
 * (The "case that doesn't work", above)
 */
#define PTR2(lp,offset) ((LPSTR)MAKELONG(offset,HIWORD(lp)))

/* this macro is used to retrieve the i-th item in the group segment.  Note
 * that this pointer will NOT be NULL for an unused slot.
 */
#define ITEM(lpgd,i) ((LPITEMDEF)PTR(lpgd, lpgd->rgiItems[i]))

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Tag Stuff                                                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

typedef struct _tag
  {
    WORD wID;                   // tag identifier
    WORD dummy1;                // need this for alignment!
    int wItem;                  // (unde the covers 32 bit point!)item the tag belongs to
    WORD cb;                    // size of record, including id and count
    WORD dummy2;                // need this for alignment!
    BYTE rgb[1];
  } PMTAG, FAR * LPPMTAG;

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
#define CITEMSMAX 50

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

//
// This is the structure of the .grp files in Windows3.1
//

/* .GRP File format structures -
 */
typedef struct tagGROUPDEF16
  {
    DWORD	  dwMagic;	      /* magical bytes 'PMCC' */
    WORD	  wCheckSum;	      /* adjust this for zero sum of file */
    WORD	  cbGroup;	      /* length of group segment */
    WORD	  nCmdShow;	      /* min, max, or normal state */
    SMALL_RECT rcNormal;	      /* rectangle of normal window */
    POINTS	  ptMin;	      /* point of icon */
    WORD	  pName;	      /* name of group */
				    /* these four change interpretation */
    WORD	  cxIcon;	      /* width of icons */
    WORD	  cyIcon;	      /* hieght of icons */
    WORD	  wIconFormat;	      /* planes and BPP in icons */
    WORD	  wReserved;	      /* This word is no longer used. */

    WORD	  cItems;	      /* number of items in group */
    WORD	  rgiItems[1];	      /* array of ITEMDEF offsets */
  } GROUPDEF16;
typedef GROUPDEF16 *LPGROUPDEF16;

/* this macro is used to retrieve the i-th item in the group segment.  Note
 * that this pointer will NOT be NULL for an unused slot.
 */
#define ITEM16(lpgd16,i) ((LPBYTE)PTR(lpgd16, lpgd16->rgiItems[i]))

#if 0
//
// These structures are not needed for the conversion but it is useful to
// understand what is going on.
//
typedef struct tagITEMDEF16
  {
    POINTS    pt;		      /* location of item icon in group */
                                      // NB This is read when a group is
                                      // loaded and updated when a group is
                                      // written.  All painting/moving is
                                      // done using the icon and title rects
                                      // in an ITEM.  So if you want to know
                                      // where an item is use it's icon rect
                                      // not it's point.
    WORD	  iIcon;	      /* index of item icon */
    WORD	  cbHeader;	      /* size of icon header */
    WORD	  cbANDPlane;	      /* size of and part of icon */
    WORD	  cbXORPlane;	      /* size of xor part of icon */
    WORD	  pHeader;	      /* file offset of icon header */
    WORD	  pANDPlane;	      /* file offset of AND plane */
    WORD	  pXORPlane;	      /* file offset of XOR plane */
    WORD	  pName;	      /* file offset of name string */
    WORD	  pCommand;	      /* file offset of command string */
    WORD	  pIconPath;	      /* file offset of icon path */
  } ITEMDEF16;
typedef ITEMDEF16 *LPITEMDEF16;

typedef struct _tag16
  {
    WORD wID;			// tag identifier
    WORD wItem; 		// item the tag belongs to
    WORD cb;			// size of record, including id and count
    BYTE rgb[1];
  } TAG16, * LPTAG16;

#endif

//
// Globals
//

HANDLE hInst;
