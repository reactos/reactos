/****************************************************************************/
/*                                                                          */
/*  CONVGRP.H -                                                             */
/*                                                                          */
/*      Header file for the conversion from Windows NT 1.0 program group    */
/*      format with ANSI strings to Windows NT 1.0a group format            */
/*      with UNICODE strings.                                               */
/*                                                                          */
/*  Created: 09-10-93   Johanne Caron                                       */
/*                                                                          */
/****************************************************************************/

//  -- old 32 bit ansi group structures --
typedef struct tagGROUPDEF_A {
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
} GROUPDEF_A, *PGROUPDEF_A;
typedef GROUPDEF_A *LPGROUPDEF_A;

typedef struct tagITEMDEF_A {
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
} ITEMDEF_A, *PITEMDEF_A;
typedef ITEMDEF_A *LPITEMDEF_A;

//
// Defined in pmgseg.c
//

DWORD PASCAL AddThing(HANDLE hGroup, LPTSTR lpStuff, DWORD cbStuff);
WORD PASCAL AddUpGroupFile(LPGROUPDEF lpgd);
INT PASCAL AddTag(HANDLE h, int item, WORD id, LPTSTR lpbuf, int cb);
DWORD PASCAL SizeofGroup(LPGROUPDEF lpgd);
int ConvertToUnicodeGroup(LPGROUPDEF_A lpGroupORI, LPHANDLE lphNewGroup);

