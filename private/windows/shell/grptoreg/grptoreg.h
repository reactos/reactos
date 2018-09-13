/*++ BUILD Version: 0002    // Increment this if a change has global effects

/****************************************************************************/
/*                                                                          */
/*  GRPTOREG.H -                                                            */
/*                                                                          */
/*      Include for the conversion of group files (.grp) to the registry.   */
/*      Extracted from progman.h.                                           */
/*                                                                          */
/*        Created:  4/10/92       JohanneC                                  */
/*                                                                          */
/****************************************************************************/

#include <setjmp.h>
#include <windows.h>

#ifndef RC_INVOKED
#include <port1632.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Typedefs                                                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

typedef struct tagITEM {
    struct tagITEM *pNext;              /* link to next item */
    int             iItem;              /* index in group */
    DWORD           dwDDEId;            /* id used for Apps querying Progman */
                                        /* for its properties via DDE */
    RECT            rcIcon;             /* icon rectangle */
    HICON           hIcon;              /* the actual icon */
    RECT            rcTitle;            /* title rectangle */
} ITEM, *PITEM;

typedef struct tagGROUP {
    struct tagGROUP *pNext;               /* link to next group            */
    HWND            hwnd;                 /* hwnd of group window          */
    HANDLE          hGroup;               /* global handle of group object */
    PITEM           pItems;               /* pointer to first item         */
    LPSTR           lpKey;                /* name of group key             */
    WORD            wIndex;               /* index in PROGMAN.INI of group */
    BOOL            fRO;                  /* group file is readonly        */
    FILETIME        ftLastWriteTime;
    HBITMAP         hbm;                  /* bitmap 'o icons               */
    WORD            fLoaded;
} GROUP, *PGROUP;

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

//
// New format for UNICODE groups for Windows NT 1.0a
//
typedef struct tagGROUPDEF_U {
    DWORD   dwMagic;        /* magical bytes 'PMCC' */
    DWORD   cbGroup;        /* length of group segment */
    RECT    rcNormal;       /* rectangle of normal window */
    POINT   ptMin;          /* point of icon */
    WORD    wCheckSum;      /* adjust this for zero sum of file */
    WORD    nCmdShow;       /* min, max, or normal state */
    DWORD   pName;          /* name of group */
                            /* these four change interpretation */
    WORD    cxIcon;         /* width of icons */
    WORD    cyIcon;         /* hieght of icons */
    WORD    wIconFormat;    /* planes and BPP in icons */
    WORD    wReserved;      /* This word is no longer used. */

    WORD    cItems;         /* number of items in group */
    WORD    Reserved1;
    DWORD   Reserved2;
    DWORD   rgiItems[1];    /* array of ITEMDEF offsets */
} GROUPDEF_U, *PGROUPDEF_U;
typedef GROUPDEF_U *LPGROUPDEF_U;

/* the pointers in the above structures are short pointers relative to the
 * beginning of the segments.  This macro converts the short pointer into
 * a long pointer including the proper segment/selector value.        It assumes
 * that its argument is an lvalue somewhere in a group segment, for example,
 * PTR(lpgd->pName) returns a pointer to the group name, but k=lpgd->pName;
 * PTR(k) is obviously wrong as it will use either SS or DS for its segment,
 * depending on the storage class of k.
 */
#define PTR(base, offset) (LPSTR)((PBYTE)base + offset)


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

#define TAG_MAGIC GROUP_MAGIC

    /* range 8000 - 80FF > global
     * range 8100 - 81FF > per item
     * all others reserved
     */

#define ID_MAGIC                0x8000
    /* data: the string 'TAGS'
     */

#define ID_LASTTAG              0xFFFF
    /* the last tag in the file
     */


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Defines                                                                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* magic number for .GRP file validation
 */
#define GROUP_MAGIC             0x43434D50L  /* 'PMCC' */
#define GROUP_UNICODE           0x43554D50L  /* 'PMUC' */

#define CCHGROUP                5 // length of the string "Group"

#define CGROUPSMAX              40      // The max number of groups allowed.
#define MAXGROUPNAMELEN         30

#define MAX_MESSAGE_LENGTH      100

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Globals                                                                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/
BOOL        bMinOnRun           = FALSE;
BOOL        bArranging          = FALSE;
BOOL        bAutoArrange        = FALSE;
BOOL        bSaveSettings       = TRUE;
BOOL        fNoRun              = FALSE;
BOOL        fNoClose            = FALSE;
BOOL        fNoSave             = FALSE;
BOOL        fNoFileMenu         = FALSE;
DWORD       dwEditLevel         = 0;

char        szINIFile[]         = "PROGMAN.INI";
char        szNULL[]            = "";
char        szStartup[]         = "startup";
char        szGroups[]          = "UNICODE Groups";
char        szOrder[]           = "UNICODE Order";
char        szWindow[]          = "Window";
char        szAutoArrange[]     = "AutoArrange";
char        szSaveSettings[]    = "SaveSettings";
char        szMinOnRun[]        = "MinOnRun";
char        szSettings[]        = "Settings";
char        szNoRun[]           = "NoRun";
char        szNoClose[]         = "NoClose";
char        szEditLevel[]       = "EditLevel";
char        szRestrict[]        = "Restrictions";
char        szNoFileMenu[]      = "NoFileMenu";
char        szNoSave[]          = "NoSaveSettings";
char        szSystemBoot[]      = "Boot";
char        szSystemDisplay[]   = "display.drv";
char        szPMWindowSetting[160];
char        szStartupGroup[MAXGROUPNAMELEN+1];
char        szGroupsOrder[CGROUPSMAX*3+7];
WORD        pGroupIndexes[CGROUPSMAX+1];

CHAR        szProgramManager[]  = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager";   // registry key for groups
CHAR        szProgramGroups[]   = "UNICODE Program Groups";   // registry key for groups
CHAR        szCommonGroups[]    = "SOFTWARE\\Program Groups";   // registry key for common groups

CHAR        szMessage[MAX_MESSAGE_LENGTH];
BOOL        bNoError            = TRUE;

HKEY hkeyCommonGroups = NULL;
HKEY hkeyProgramGroups = NULL;
HKEY hkeyProgramManager = NULL;
HKEY hkeyPMSettings = NULL;
HKEY hkeyPMRestrict = NULL;
HKEY hkeyPMGroups = NULL;

SECURITY_ATTRIBUTES PGrpSecAttr;                  // for personal groups
PSECURITY_ATTRIBUTES pPGrpSecAttr = NULL;
SECURITY_ATTRIBUTES CGrpSecAttr;                  // for common groups
PSECURITY_ATTRIBUTES pCGrpSecAttr = NULL;

#include "secdesc.h"
#include "security.h"
