#pragma once

typedef struct
{
    SHORT X;
    SHORT Y;
    PCSTR Buffer;
    DWORD Flags;
    INT TextID;
} MUI_ENTRY, *PMUI_ENTRY;

typedef struct
{
    PCSTR ErrorText;
    PCSTR ErrorStatus;
} MUI_ERROR;

typedef struct
{
    LONG Number;
    MUI_ENTRY * MuiEntry;
} MUI_PAGE;

typedef struct
{
    LONG Number;
    PCSTR String;
} MUI_STRING;

typedef struct
{
    LCID LanguageID; // LocaleID;
    PCWSTR LanguageDescriptor;
    const MUI_PAGE * MuiPages;
    const MUI_ERROR * MuiErrors;
    const MUI_STRING * MuiStrings;
} MUI_LANGUAGE_RESOURCE;

#if 0
BOOLEAN
IsLanguageAvailable(
    _In_ LANGID LanguageId);
#endif

extern LANGID SelectedLanguageId;
#if 0
VOID
MUISetCurrentLanguage(
    _In_ LANGID LanguageId);
#endif

VOID
MUIDisplayPage(
    ULONG PageNumber);

VOID
MUIClearPage(
    ULONG PageNumber);

VOID
MUIDisplayErrorV(
    IN ULONG ErrorNum,
    OUT PINPUT_RECORD Ir,
    IN ULONG WaitEvent,
    IN va_list args);

VOID
__cdecl
MUIDisplayError(
    ULONG ErrorNum,
    PINPUT_RECORD Ir,
    ULONG WaitEvent,
    ...);

VOID
SetConsoleCodePage(VOID);

PCSTR
MUIGetString(
    ULONG Number);

const MUI_ENTRY *
MUIGetEntry(
    IN ULONG Page,
    IN INT TextID);

VOID
MUIClearText(
    IN ULONG Page,
    IN INT TextID);

VOID
MUIClearStyledText(
    IN ULONG Page,
    IN INT TextID,
    IN INT Flags);

VOID
MUISetText(
    IN ULONG Page,
    IN INT TextID);

VOID
MUISetStyledText(
    IN ULONG Page,
    IN INT TextID,
    IN INT Flags);

/* Special characters */
extern CHAR CharBullet;
extern CHAR CharBlock;
extern CHAR CharHalfBlock;
extern CHAR CharUpArrow;
extern CHAR CharDownArrow;
extern CHAR CharHorizontalLine;
extern CHAR CharVerticalLine;
extern CHAR CharUpperLeftCorner;
extern CHAR CharUpperRightCorner;
extern CHAR CharLowerLeftCorner;
extern CHAR CharLowerRightCorner;
extern CHAR CharVertLineAndRightHorizLine;
extern CHAR CharLeftHorizLineAndVertLine;
extern CHAR CharDoubleHorizontalLine;
extern CHAR CharDoubleVerticalLine;
extern CHAR CharDoubleUpperLeftCorner;
extern CHAR CharDoubleUpperRightCorner;
extern CHAR CharDoubleLowerLeftCorner;
extern CHAR CharDoubleLowerRightCorner;

/* MUI Text IDs */

/* Static MUI Text */
#define TEXT_ID_STATIC (-1)

/* Dynamic MUI Text IDs */
#define TEXT_ID_FORMAT_PROMPT 1

/* MUI Strings */
#define STRING_PLEASEWAIT                   1
#define STRING_INSTALLCREATEPARTITION       2
#define STRING_INSTALLCREATELOGICAL         3
#define STRING_INSTALLDELETEPARTITION       4
#define STRING_DELETEPARTITION              5
#define STRING_PARTITIONSIZE                6
#define STRING_CHOOSE_NEW_PARTITION           7
#define STRING_CHOOSE_NEW_EXTENDED_PARTITION  8
#define STRING_CHOOSE_NEW_LOGICAL_PARTITION   9
#define STRING_HDPARTSIZE                   10
#define STRING_CREATEPARTITION              11
#define STRING_NEWPARTITION                 12
#define STRING_PARTFORMAT                   13
#define STRING_NONFORMATTEDPART             14
#define STRING_NONFORMATTEDSYSTEMPART       15
#define STRING_NONFORMATTEDOTHERPART        16
#define STRING_INSTALLONPART                17
#define STRING_CONTINUE                     18
#define STRING_QUITCONTINUE                 19
#define STRING_REBOOTCOMPUTER               20
#define STRING_DELETING                     21
#define STRING_MOVING                       22
#define STRING_RENAMING                     23
#define STRING_COPYING                      24
#define STRING_SETUPCOPYINGFILES            25
#define STRING_REGHIVEUPDATE                26
#define STRING_IMPORTFILE                   27
#define STRING_DISPLAYSETTINGSUPDATE        28
#define STRING_LOCALESETTINGSUPDATE         29
#define STRING_KEYBOARDSETTINGSUPDATE       30
#define STRING_CODEPAGEINFOUPDATE           31
#define STRING_DONE                         32
#define STRING_REBOOTCOMPUTER2              33
#define STRING_CONSOLEFAIL1                 34
#define STRING_CONSOLEFAIL2                 35
#define STRING_CONSOLEFAIL3                 36
#define STRING_FORMATTINGPART               37
#define STRING_CHECKINGDISK                 38
#define STRING_FORMATDISK1                  39
#define STRING_FORMATDISK2                  40
#define STRING_KEEPFORMAT                   41
#define STRING_HDDISK1                      42
#define STRING_HDDISK2                      43
#define STRING_PARTTYPE                     44
#define STRING_HDDINFO1                     45
#define STRING_HDDINFO2                     46
#define STRING_UNPSPACE                     47
#define STRING_MAXSIZE                      48
#define STRING_UNFORMATTED                  49
#define STRING_EXTENDED_PARTITION           50
#define STRING_FORMATUNUSED                 51
#define STRING_FORMATUNKNOWN                52
#define STRING_KB                           53
#define STRING_MB                           54
#define STRING_GB                           55
#define STRING_ADDKBLAYOUTS                 56
#define STRING_REBOOTPROGRESSBAR            57
