/****************************************************************************

    ACCESS.H

    This file defines the definitions for in the ACCESS.C program

****************************************************************************/

/* declaration of procedures and functions */

/* from ACCESS.C */

void MakeHelpPathName(char*);                    /* Function deriving help file path */
LRESULT APIENTRY hookDialogBoxMsg(int,WPARAM,LPARAM);    /* Function to trap for F1 in dialog boxes */
//HOOKPROC hookDialogBoxMsg(int,WPARAM,LPARAM);    /* Function to trap for F1 in dialog boxes */

int PASCAL WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
LRESULT APIENTRY AccessWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT APIENTRY About(HWND, UINT, WPARAM, LPARAM);
BOOL    APIENTRY Help(HWND, UINT, WPARAM, LPARAM);
short OkAccessMessage (HWND, WORD);
short AccessMessageBox (HWND hWnd, WORD wnumber, UINT iFlags );
void OkAccessMsg( HWND hWnd, WORD wMsg,...);

/* from DIALOGS.C */
/* dialogs procedure routines */

LRESULT APIENTRY AdjustSticKeys(HWND, UINT, WPARAM, LPARAM);
LRESULT APIENTRY AdjustFilterKeys(HWND, UINT, WPARAM, LPARAM);
LRESULT APIENTRY AdjustMouseKeys(HWND, UINT, WPARAM, LPARAM);
LRESULT APIENTRY AdjustTimeOut(HWND, UINT, WPARAM, LPARAM);
LRESULT APIENTRY AdjustSerialKeys(HWND, UINT, WPARAM, LPARAM);
LRESULT APIENTRY AdjustToggleKeys(HWND, UINT, WPARAM, LPARAM);
LRESULT APIENTRY AdjustShowSounds(HWND, UINT, WPARAM, LPARAM);
LRESULT APIENTRY AdjustSoundSentry(HWND, UINT, WPARAM, LPARAM);
void OkFiltersMessage (HWND, WORD);

extern INT fShowSoundsOn;

/* from INIT.C */

void  InitFeatures(HWND,HANDLE);
void  SaveFeatures(void);
BOOL MySystemParametersInfo( UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);

/* from REG.C */

BOOL InitializeUserRegIfNeeded( void );
DWORD SaveDefaultSettings( void );



#define  IDS_SAVE_TO_WIN_INI    1
#define  IDS_CLOSE_MESSAGE      2
#define  IDS_FILTERS_1          3
#define  IDS_FILTERS_2          4
#define  IDS_FILTERS_3          5
//#define    IDS_FILTERS_4        6
//#define    IDS_FILTERS_5        7

#define  IDS_HELP_MESSAGE       8
#define  IDS_TITLE              9

#define  IDS_FILTERS_6         10
#define  IDS_INIT_1            11
#define  IDS_INIT_2            14
#define  IDS_SHOW_1            12
#define  IDS_SHOW_2            13

#define  IDS_MENU_MESSAGE      15
#define  IDS_KYBDBAD_MESSAGE   16
#define  IDS_MOUBAD_MESSAGE    17

#define  IDS_NONE              18

#define  IDS_SS_WINCAPTION     20
#define  IDS_SS_WINWINDOW      21
#define  IDS_SS_WINDESKTOP     22

#define  IDS_SS_FSBORDER       30
#define  IDS_SS_FSSCREEN       31

#define  IDS_SS_GRAPHICSBORDER 40
#define  IDS_SS_GRAPHICSSCREEN 41

#define  IDS_SAVE_DISABLED     42
#define  IDS_SK_DISABLED       43

#define  IDS_SAVE_DEFAULT       44
#define  IDS_ACCESS_DENIED      45
#define  IDS_ERROR_CODE         46

#define  IDS_SAVE_FIRST         60
#define  IDS_BAD_OS_VER         61
#define  IDS_UNABLE_TO_START    62
#define  IDS_BAD_SK_PORT        63
#define  IDS_BAD_SK_BAUD        64

#define  MAXEFFECTSTRING      128

#define  Stickeys     800
#define  FilterKeys   801
#define  MouseKeys    802
#define  SerialKeys   803
#define  TimeOut      804
#define  ToggleKeys   805
#define  ShowSounds   806
#define  AboutBox     807


// these should be in winuser.h but are not yet in Chicago project.
#ifndef WF_NONE
/*
 * Define window flash parameters
 */
#define WF_NONE     0
#define WF_TITLE    1
#define WF_WINDOW   2
#define WF_DISPLAY  3

/*
 * Define graphic flash parameters
 */
#define GF_NONE     0
#define GF_DISPLAY  3

/*
 * Define text flash parameters
 */
#define TF_NONE      0
#define TF_CHARS     1
#define TF_BORDER    2
#define TF_DISPLAY   3

#endif

#ifndef GRAPHICSMODE
#define GRAPHICSMODE
#endif
#ifndef BORDERFLASH
#define BORDERFLASH
#endif
#ifndef i386
#define i386
#endif
#ifndef NOSAVE
//#define NOSAVE
#endif

typedef struct tagMYSERIALKEYS
    {
    UINT    cbSize;
    UINT    iComName;
    UINT    iBaudRate;
    UINT    iCurrentComName;
    UINT    iCurrentBaudRate;
    BOOL    fCommOpen;
    BOOL    fSerialKeysOn;
    } MYSERIALKEYS, *LPMYSERIALKEYS;

// #define MYSK    YesHackIt

#ifndef SPI_GETSERIALKEYS
#define SPI_GETSERIALKEYS           62
#endif
#ifndef SPI_SETSERIALKEYS
#define SPI_SETSERIALKEYS           63
#endif

// #ifndef SERKF_SERIALKEYSON
// typedef struct tagSERIALKEYS
// {
//     DWORD   cbSize;
//     DWORD   dwFlags;
//     LPSTR   lpszActivePort;
//     LPSTR   lpszPort;
//     DWORD   iBaudRate;
//     DWORD   iPortState;
//     DWORD   iActive;
// }   SERIALKEYS, *LPSERIALKEYS;
//
// /* flags for SERIALKEYS dwFlags field */
// #define SERKF_SERIALKEYSON  0x00000001
// #define SERKF_AVAILABLE     0x00000002
// #define SERKF_INDICATOR     0x00000004
// #endif

#ifndef HCF_HIGHCONTRASTON
typedef struct tagHIGHCONTRAST      
{                                   
    DWORD   cbSize;
    DWORD   dwFlags;
}   HIGHCONTRAST, *LPHIGHCONTRAST;

/* flags for HIGHCONTRAST dwFlags field */
#define HCF_HIGHCONTRASTON  0x00000001
#define HCF_AVAILABLE       0x00000002
#define HCF_HOTKEYACTIVE    0x00000004
#define HCF_CONFIRMHOTKEY   0x00000008
#define HCF_HOTKEYSOUND     0x00000010
#define HCF_INDICATOR       0x00000020
#define HCF_HOTKEYAVAILABLE 0x00000040
#endif

// SetFlag is a statement
#define SetFlag( dw, m, f ) if(f) { dw |= m; } else { dw &= ~m; }
// TestFlag is an operator
#define TestFlag( dw, m ) ( (dw & m) ? TRUE : FALSE )
