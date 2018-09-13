/*++

Copyright (c) 1994-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    instime.h

Abstract:

    This module contains the information for the IME Installer for the
    Regional Options applet.

Revision History:

--*/



#ifndef INSTIME_H_
#define INSTIME_H_



//
//  Constant Declarations.
//

#define ARRAYSIZE(f)              (sizeof(f) / sizeof(f[0]))
#define SIZEOF(c)                 (sizeof(c))
#define IS_IME(h)                 ((HIWORD(h) & 0xf000) == 0xe000)

#define DISTR_INF_WILDCARD        (TEXT("*.inf"))
#define DISTR_OEMINF_DEFAULTPATH  (TEXT("A:\\"))

#define SZ_IME_FILE               TEXT("IME_file")
#define SZ_LAYOUT_TEXT            TEXT("Layout_Text")
#define SZ_LAYOUT_FILE            TEXT("Layout_File")
#define SZ_LAYOUT_ID              TEXT("Layout_ID")
#define SZ_LAYOUT                 TEXT("Keyboard_Layout")
#define SZ_UNINSTALL              TEXT("Uninstall")

#define SZ_FILE                   TEXT("File")
#define SZ_SECTION                TEXT("Section")
#define SZ_DESCRIPTION            TEXT("Description")
#define SZ_CLASS_IME              TEXT("IME")
#define SZ_SECTION_IME            TEXT("IME")
#define SZ_CLASS_KBDLAYOUT        TEXT("KBDLAYOUT")
#define SZ_SECTION_KBDLAYOUT      TEXT("KBDLAYOUT")

#define MAX_SECT_NAME_LEN         64
#define LINE_LEN                  256
#define MAX_PAGES                 15

#define INF_IME                   1
#define INF_KBDLAYOUT             2


//
//  Private messages.
//
#define WMPRIV_POKEFOCUS          (WM_APP + 0)




//
//  Typedef Declarations.
//

typedef struct _tagINF_NODE
{
    TCHAR szInfFile[MAX_PATH];
    TCHAR szSection[MAX_SECT_NAME_LEN];
    TCHAR szDesc[LINE_LEN];
    TCHAR szIMEFile[LINE_LEN];
    TCHAR szKbdLayoutFile[LINE_LEN];
    TCHAR szKbdLayoutID[LINE_LEN];
    TCHAR szKbdLayout[LINE_LEN];
    TCHAR szSrcPath[MAX_PATH];
    int   nInfType;
    struct _tagINF_NODE *Next;
} INF_NODE, *PINF_NODE;



#endif
