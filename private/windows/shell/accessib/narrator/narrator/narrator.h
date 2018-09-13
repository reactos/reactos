// Narrator Globals
#define WM_MSRREPEAT WM_APP+1 //A key for us to process
#define WM_MSRSPEAK WM_APP+2
#define WM_MUTE WM_APP+3
#define WM_MSRHELP WM_APP+4
#define WM_MSRQUIT WM_APP+5
#define WM_MSRCONFIGURE WM_APP+6

#define MSR_ECHOALNUM		  1
#define MSR_ECHOBACK		  2
#define MSR_ECHOSPACE		  4
#define MSR_ECHODELETE		  8
#define MSR_ECHOTAB			 16
#define MSR_ECHOENTER		 32
#define MSR_ECHOMODIFIERS	 64
#define MSR_ECHOGRAPHICS	128

#define ID_PROPERTYFIRST    100
#define ID_NAME         100
#define ID_ROLE         101
#define ID_STATE        102
#define ID_LOCATION     103
#define ID_DESCRIPTION  104
#define ID_VALUE        105
#define ID_HELP         106
#define ID_SHORTCUT     107
#define ID_DEFAULT      108
#define ID_PARENT       109
#define ID_CHILDREN     110
#define ID_SELECTION    111
#define ID_WINDOW       112
#define ID_PROPERTYLAST 112
#define UMR_MACHINE_KEY HKEY_LOCAL_MACHINE
#define UM_REGISTRY_KEY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Accessibility\\Utility Manager")


typedef struct tagObjectInfo
{
    HWND hwnd;
    long* plObj;
    VARIANT varChild;
} OBJINFO;

typedef tagObjectInfo* LPOBJINFO;

// Macros and function prototypes for debugging
#ifdef DEBUG
  #define _DEBUG
#endif
#ifdef _DEBUG
  void FAR CDECL PrintIt(LPTSTR strFmt, ...);
  #define DBPRINTF PrintIt
#else
  #define DBPRINTF        1 ? (void)0 : (void)
#endif
