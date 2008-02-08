#ifndef MUI_H__
#define MUI_H__

typedef struct
{
   BYTE X;
   BYTE Y;
   LPCSTR Buffer;
   BYTE Flags;
}MUI_ENTRY, *PMUI_ENTRY;

typedef struct
{
    LPCSTR ErrorText;
    LPCSTR ErrorStatus;
}MUI_ERROR;

typedef struct
{
    LONG Number;
    MUI_ENTRY * MuiEntry;
}MUI_PAGE;

typedef struct
{
    LONG Number;
    LPSTR String;
} MUI_STRING;

typedef struct
{
    PWCHAR LanguageID;
    PWCHAR LanguageKeyboardLayoutID;
    PWCHAR ACPage;
    PWCHAR OEMCPage;
    PWCHAR MACCPage;
    PWCHAR LanguageDescriptor;
    const MUI_PAGE * MuiPages;
    const MUI_ERROR * MuiErrors;
    const MUI_STRING * MuiStrings;
}MUI_LANGUAGE;


#define TEXT_NORMAL            0
#define TEXT_HIGHLIGHT         1
#define TEXT_UNDERLINE         2
#define TEXT_STATUS            4

#define TEXT_ALIGN_DEFAULT     8
#define TEXT_ALIGN_RIGHT       16
#define TEXT_ALIGN_LEFT        32
#define TEXT_ALIGN_CENTER      64

VOID
MUIDisplayPage (ULONG PageNumber);

VOID
MUIDisplayError (ULONG ErrorNum, PINPUT_RECORD Ir, ULONG WaitEvent);

LPCWSTR
MUIDefaultKeyboardLayout(VOID);

BOOLEAN
AddCodePage(VOID);

VOID
SetConsoleCodePage(VOID);

LPSTR
MUIGetString(ULONG Number);

#define STRING_INSTALLCREATEPARTITION 1
#define STRING_INSTALLDELETEPARTITION 2
#define STRING_CREATEPARTITION 3
#define STRING_PARTITIONSIZE 4
#define STRING_PLEASEWAIT 5
#define STRING_CHOOSENEWPARTITION 6
#define STRING_COPYING 7
#define STRING_SETUPCOPYINGFILES 8
#define STRING_PAGEDMEM 9
#define STRING_NONPAGEDMEM 10
#define STRING_FREEMEM 11


#endif
