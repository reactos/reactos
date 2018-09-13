
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/


// menu commands

// Find menu
#define IDM_OPENFILE        100
#define IDM_SAVEFILE        101
#define IDM_SAVEFILEAS      102
#define IDM_PRINT           103
#define IDM_EXIT            104

// Options menu
#define IDM_ENTERNEW        200
#define IDM_CHOOSECOLOR     201
#define IDM_CHOOSEFONT      202
#define IDM_FINDTEXT        203
#define IDM_REPLACETEXT     204
#define IDM_STANDARD        205
#define IDM_HOOK            206
#define IDM_CUSTOM          207

// Settings
#define IDM_PERCEPUAL       400
#define IDM_COLOR           401
#define IDM_SATURATION      402
#define IDM_ASCII           403
#define IDM_BINARY          404
#define IDM_AUTO            405
#define IDM_ABC             406
#define IDM_DEFG            407
#define IDM_INP_AUTO        408
#define IDM_INP_GRAY        409
#define IDM_INP_RGB         410
#define IDM_INP_CMYK        411
#define IDM_CSA             412
#define IDM_CRD             413
#define IDM_INTENT          414
#define IDM_PROFCRD         415
// Help menu
#define IDM_ABOUT           300


// Dialog box constants
#define IDEDIT              500

// string constants

#define IDS_DIALOGFAILURE     1
#define IDS_STRUCTSIZE        2
#define IDS_INITIALIZATION    3
#define IDS_NOTEMPLATE        4
#define IDS_NOHINSTANCE       5
#define IDS_LOADSTRFAILURE    6
#define IDS_FINDRESFAILURE    7
#define IDS_LOADRESFAILURE    8
#define IDS_LOCKRESFAILURE    9
#define IDS_MEMALLOCFAILURE  10
#define IDS_MEMLOCKFAILURE   11
#define IDS_NOHOOK           12
#define IDS_SETUPFAILURE     13
#define IDS_PARSEFAILURE     14
#define IDS_RETDEFFAILURE    15
#define IDS_LOADDRVFAILURE   16
#define IDS_GETDEVMODEFAIL   17
#define IDS_INITFAILURE      18
#define IDS_NODEVICES        19
#define IDS_NODEFAULTPRN     20
#define IDS_DNDMMISMATCH     21
#define IDS_CREATEICFAILURE  22
#define IDS_PRINTERNOTFOUND  23
#define IDS_NOFONTS          24
#define IDS_SUBCLASSFAILURE  25
#define IDS_INVALIDFILENAME  26
#define IDS_BUFFERTOOSMALL   27
#define IDS_FILTERSTRING     28
#define IDS_UNKNOWNERROR     29

// constants

#define FILE_LEN            128

// Function prototypes

// procs
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK About(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK EnterNew(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK FileOpenHookProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK FileSaveHookProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ChooseColorHookProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ChooseFontHookProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK FindTextHookProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ReplaceTextHookProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK PrintDlgHookProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK PrintSetupHookProc(HWND, UINT, WPARAM, LPARAM);

//functions
BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
BOOL OpenNewFile( HWND );
BOOL SaveToFile( HWND );
BOOL SaveAs( HWND );
void SearchFile( LPFINDREPLACE );
BOOL ChooseNewFont( HWND );
BOOL ChooseNewColor( HWND );
void PrintFile( HWND );
void CallFindText( HWND );
void CallReplaceText( HWND );
void ProcessCDError(DWORD, HWND);

void    ColorSpaceControl( LPSTR FileName, LPSTR SaveFileName, WORD InpDrvClrSp,
        DWORD Intent, WORD CSAType, BOOL AllowBinary);
void    CreateCRDControl( LPSTR FileName, LPSTR SaveFileName,
        DWORD Inter_Intent, BOOL AllowBinary);
void    CreateINTENTControl(LPSTR FileName, LPSTR SaveFileName, DWORD Inter_Intent);
void    CreateProfCRDControl(LPSTR DevProfile, LPSTR TargetProfile, 
        LPSTR SaveFileName, DWORD Inter_Intent, BOOL AllowBinary);
