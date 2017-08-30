#pragma once

typedef struct
{
    BYTE X;
    BYTE Y;
    LPCSTR Buffer;
    DWORD Flags;
} MUI_ENTRY, *PMUI_ENTRY;

typedef struct
{
    LPCSTR ErrorText;
    LPCSTR ErrorStatus;
} MUI_ERROR;

typedef struct
{
    LONG Number;
    MUI_ENTRY * MuiEntry;
} MUI_PAGE;

typedef struct
{
    LONG Number;
    LPSTR String;
} MUI_STRING;

typedef struct
{
    PWCHAR LanguageID;
    PWCHAR LanguageDescriptor;
    const MUI_PAGE * MuiPages;
    const MUI_ERROR * MuiErrors;
    const MUI_STRING * MuiStrings;
} MUI_LANGUAGE_RESOURCE;

#if 0
BOOLEAN
IsLanguageAvailable(
    PWCHAR LanguageId);
#endif

VOID
MUIDisplayPage(
    ULONG PageNumber);

VOID
MUIClearPage(
    ULONG PageNumber);

VOID
MUIDisplayError(
    ULONG ErrorNum,
    PINPUT_RECORD Ir,
    ULONG WaitEvent,
    ...);

VOID
SetConsoleCodePage(VOID);

LPSTR
MUIGetString(
    ULONG Number);

#define STRING_PLEASEWAIT                1
#define STRING_INSTALLCREATEPARTITION    2
#define STRING_INSTALLCREATELOGICAL      60
#define STRING_INSTALLDELETEPARTITION    3
#define STRING_DELETEPARTITION           59
#define STRING_PARTITIONSIZE             4
#define STRING_CHOOSENEWPARTITION        5
#define STRING_CHOOSE_NEW_EXTENDED_PARTITION  57
#define STRING_CHOOSE_NEW_LOGICAL_PARTITION   61
#define STRING_HDDSIZE                   6
#define STRING_CREATEPARTITION           7
#define STRING_PARTFORMAT                8
#define STRING_NONFORMATTEDPART          9
#define STRING_NONFORMATTEDSYSTEMPART    62
#define STRING_NONFORMATTEDOTHERPART     63
#define STRING_INSTALLONPART             10
#define STRING_CHECKINGPART              11
#define STRING_CONTINUE                  12
#define STRING_QUITCONTINUE              13
#define STRING_REBOOTCOMPUTER            14
#define STRING_TXTSETUPFAILED            15
#define STRING_COPYING                   16
#define STRING_SETUPCOPYINGFILES         17
#define STRING_REGHIVEUPDATE             20
#define STRING_IMPORTFILE                21
#define STRING_DISPLAYETTINGSUPDATE      22
#define STRING_LOCALESETTINGSUPDATE      23
#define STRING_KEYBOARDSETTINGSUPDATE    24
#define STRING_CODEPAGEINFOUPDATE        25
#define STRING_DONE                      26
#define STRING_REBOOTCOMPUTER2           27
#define STRING_CONSOLEFAIL1              28
#define STRING_CONSOLEFAIL2              29
#define STRING_CONSOLEFAIL3              30
#define STRING_FORMATTINGDISK            31
#define STRING_CHECKINGDISK              32
#define STRING_FORMATDISK1               33
#define STRING_FORMATDISK2               34
#define STRING_KEEPFORMAT                35
#define STRING_HDINFOPARTCREATE_1        36
#define STRING_HDINFOPARTCREATE_2        37
#define STRING_HDDINFOUNK2               38
#define STRING_HDINFOPARTDELETE_1        39
#define STRING_HDINFOPARTDELETE_2        40
#define STRING_HDINFOPARTZEROED_1        41
#define STRING_HDDINFOUNK4               42
#define STRING_HDINFOPARTEXISTS_1        43
#define STRING_HDDINFOUNK5               44
#define STRING_HDINFOPARTSELECT_1        45
#define STRING_HDINFOPARTSELECT_2        46
#define STRING_NEWPARTITION              47
#define STRING_UNPSPACE                  48
#define STRING_MAXSIZE                   49
#define STRING_UNFORMATTED               50
#define STRING_EXTENDED_PARTITION        58
#define STRING_FORMATUNUSED              51
#define STRING_FORMATUNKNOWN             52
#define STRING_KB                        53
#define STRING_MB                        54
#define STRING_GB                        55
#define STRING_ADDKBLAYOUTS              56
