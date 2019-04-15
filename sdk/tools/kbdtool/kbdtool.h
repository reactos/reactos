/*
 * PROJECT:         ReactOS Build Tools [Keyboard Layout Compiler]
 * LICENSE:         BSD - See COPYING.BSD in the top level directory
 * FILE:            tools/kbdtool/kbdtool.h
 * PURPOSE:         Main Header File
 * PROGRAMMERS:     ReactOS Foundation
 */

/* INCLUDES *******************************************************************/

#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <typedefs.h>

#define KEYWORD_COUNT 17

#define CHAR_NORMAL_KEY   0
#define CHAR_DEAD_KEY     1
#define CHAR_OTHER_KEY    2
#define CHAR_INVALID_KEY  3
#define CHAR_LIGATURE_KEY 4

typedef struct tagKEYNAME
{
    ULONG Code;
    PCHAR Name;
    struct tagKEYNAME* Next;
} KEYNAME, *PKEYNAME;

typedef struct tagSCVK
{
    USHORT ScanCode;
    UCHAR VirtualKey;
    PCHAR Name;
    BOOLEAN Processed;
} SCVK, *PSCVK;

typedef struct tagVKNAME
{
    ULONG VirtualKey;
    PCHAR Name;
} VKNAME, *PVKNAME;

typedef struct tagLAYOUTENTRY
{
    USHORT ScanCode;
    UCHAR VirtualKey;
    UCHAR OriginalVirtualKey;
    ULONG Cap;
    ULONG StateCount;
    ULONG CharData[8];
    ULONG DeadCharData[8];
    UCHAR LigatureCharData[8];
    ULONG OtherCharData[8];
    struct LAYOUTENTRY* CapData;
    PCHAR Name;
    ULONG Processed;
    ULONG LineCount;
} LAYOUTENTRY, *PLAYOUTENTRY;

typedef struct tagLAYOUT
{
    LAYOUTENTRY Entry[110];
} LAYOUT, *PLAYOUT;

ULONG
DoOutput(
    IN ULONG StateCount,
    IN PULONG ShiftStates,
    IN PKEYNAME DescriptionData,
    IN PKEYNAME LanguageData,
    IN PVOID AttributeData,
    IN PVOID DeadKeyData,
    IN PVOID LigatureData,
    IN PKEYNAME KeyNameData,
    IN PKEYNAME KeyNameExtData,
    IN PKEYNAME KeyNameDeadData
);

PCHAR
getVKName(
    IN ULONG VirtualKey,
    IN BOOLEAN Prefix
);

ULONG
DoParsing(
    VOID
);

extern BOOLEAN Verbose, UnicodeFile, SanityCheck, FallbackDriver;
extern PCHAR gpszFileName;
extern FILE* gfpInput;
extern VKNAME VKName[];
extern VKNAME Modifiers[];
extern SCVK ScVk[];
extern PCHAR StateLabel[];
extern PCHAR CapState[];
extern LAYOUT g_Layout;
extern CHAR gVKeyName[32];
extern CHAR gKBDName[10];
extern CHAR gCopyright[256];
extern CHAR gDescription[256];
extern CHAR gCompany[256];
extern CHAR gLocaleName[256];
extern ULONG gVersion, gSubVersion;

/* EOF */
