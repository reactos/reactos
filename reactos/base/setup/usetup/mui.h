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
    CHAR * ErrorText;
    CHAR * ErrorStatus;
}MUI_ERROR;

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
    MUI_ERROR * MuiErrors;
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

VOID
MUIDefaultKeyboardLayout(WCHAR * KeyboardLayout);


#endif
