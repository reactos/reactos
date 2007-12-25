#ifndef MUI_H__
#define MUI_H__

typedef struct
{
   BYTE X;
   BYTE Y;
   CHAR * Buffer;
   BYTE Flags;
}MUI_ENTRY, *PMUI_ENTRY;

typedef struct
{
    long Number;
    MUI_ENTRY * MuiEntry;
}MUI_PAGE;

typedef struct
{
    PWCHAR LanguageID;
    PWCHAR LanguageKeyboardLayoutID;
    PWCHAR LanguageDescriptor;
    MUI_PAGE * MuiPages;
}MUI_LANGUAGE;

#define TEXT_NORMAL            0
#define TEXT_HIGHLIGHT         1
#define TEXT_UNDERLINE         2
#define TEXT_STATUS            4

#define TEXT_ALIGN_DEFAULT     5
#define TEXT_ALIGN_RIGHT       6
#define TEXT_ALIGN_LEFT        7
#define TEXT_ALIGN_CENTER      8

VOID
MUIDisplayPage (ULONG PageNumber);

#endif
