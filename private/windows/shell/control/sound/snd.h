

/*
**  SND.H
**
**  Example header to create a DLL to control applet(s) to be
**  displayed by the Multimedia Control Panel.
**
**  History:
**
**      Fri Apr 27 1990 -by- ToddLa
**          Created.
*/

/*---------------------------------------------------------------------------*/

#include <multimed.h>

#define DLG_SOUND       42
#define DLG_CHANGED     45

#define ID_DIR          100
#define LB_NAMES        101
#define LB_FILES        102
#define ID_VOLUME       103
#define ID_PLAY         104
#define ID_EDIT         105
#define ID_BEEP         106
#define ID_CHANGED      108

#define ICON_EXCLAIM        32515

#define IDS_UNABLETITLE     3
#define IDS_UNABLEMESSAGE   4
#define IDS_WARNINGTITLE    5
#define IDS_WARNINGMESSAGE  6
#define IDS_NONE            7
#define IDS_APPNAME         8
#define IDS_NODEVICE        9
#define IDS_HELPFILE    IDS_CONTROL_HLP
#define IDS_WRITEERR        11

/*---------------------------------------------------------------------------*/

#define CODE                                     //_based(_segname("_CODE"))
typedef char    CODE SZCODE;

/*---------------------------------------------------------------------------*/

extern  char aszSoundHlp[];
extern  char aszErrorPlayTitle[];
extern  char aszErrorPlayMessage[];
extern  char aszWarningTitle[];
extern  char aszWarningMessage[];
extern  char aszNoSound[];
extern  char aszNoDevice[];
extern  char aszWriteErr[];
extern  char aszAppName[];
extern  HINSTANCE hInstance;
extern  DWORD dwContext;
extern  UINT uHelpMessage;

/*---------------------------------------------------------------------------*/

BOOL SoundDlg( HWND   hwnd
             , UINT   wMsg
             , WPARAM wParam
             , LPARAM lParam
             );
/*---------------------------------------------------------------------------*/
