/*  MIDI.H
**
**  Copyright (C) Microsoft, 1990, All Rights Reserved.
**
**
**  Multimedia Control Panel MIDI Applet.
**
**  Display a list of all installed MIDI devices, allow user to configure
**  existing or install new ones.
**
*/ 

//---------------------------------------------------------------------------
#ifndef RC_INVOKED
//---------------------------------------------------------------------------


#ifndef MIDI_DPF
 #define MIDI_DPF AuxDebugEx
#endif

void PASCAL UpdateListBox(HWND hDlg);

#define  MAX_ALIAS     80
#define  NUM_CHANNEL   16
#define  NUM_TABSTOPS  1

#define  BITONE        1

// Common states
#define  CHANGE_NONE    0

// Instrument states
#define  CHANGE_ACTIVE  1
#define  CHANGE_CHANNEL 2

// Driver states
#define  CHANGE_REMOVE  4
#define  CHANGE_ADD     8

#define  IS_INSTRUMENT(hwnd, i)  (!((LPINSTRUMENT)ListBox_GetItemData(hwnd, i))->fDevice)

typedef struct tag_Driver FAR * LPDRIVER;

#ifndef NUMELMS
 #define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif

#define SetDlgData(h,lp)  SetWindowLongPtr (h, DWLP_USER, (LONG_PTR)lp)
#define GetDlgData(h)     (LPVOID)GetWindowLongPtr (h, DWLP_USER)

// lParam of PROPSHEETPAGE points to a struct like this.
// it is freed in ReleasePropSheet() function.
//
typedef struct _midi_ps_args {
    LPFNMMEXTPROPSHEETCALLBACK  lpfnMMExtPSCallback;
    LPARAM                      lParam;
    TCHAR                       szTitle[1];
    } MPSARGS, * PMPSARGS;

// info for internal & external midi instruments
//
typedef struct _instrum * PINSTRUM;
typedef struct _instrum {
    PINSTRUM      piParent;
    BOOL          bExternal;
    BOOL          bActive;
    BOOL          fGSSynth;
    UINT          uID;
    TCHAR         szKey[MAX_ALIAS];
    TCHAR         szFriendly[MAX_ALIAS];
    } INSTRUM;

// the the loaded array of all instruments, used to refresh
// lists of all instruments and the treeview's of instruments
// and how they are connected
//
typedef struct _midi_instrums {
    HKEY          hkMidi;
    UINT          nInstr;
    BOOL          bHasExternal;
    PINSTRUM      api[128];
    } MCMIDI, * PMCMIDI;

//------------------ function prototypes ------------------------------------

// midi.c
//
INT_PTR CALLBACK MidiCplDlgProc (
   HWND hWnd,
   UINT uMsgId,
   WPARAM wParam,
   LPARAM lParam);

LONG WINAPI GetAlias (
    HKEY  hKey,
    LPTSTR szSub,
    LPTSTR pszAlias,
    DWORD cbAlias,
    BOOL *pbExtern,
    BOOL *pbActive);

void WINAPI LoadInstruments (
    PMCMIDI pmcm,
    BOOL    bDriverAsAlias);

void WINAPI FreeInstruments (
    PMCMIDI pmcm);

PINSTRUM WINAPI FindInstrument (
    PMCMIDI pmcm,
    LPTSTR  pszFriendly);

void WINAPI KickMapper (
    HWND hWnd);

// iface.c
//
BOOL WINAPI InitIface (
   HINSTANCE hInst,
   DWORD     dwReason,
   LPVOID    lpReserved);

// class.c
//
INT_PTR CALLBACK MidiClassDlgProc (
   HWND hWnd,
   UINT uMsgId,
   WPARAM wParam,
   LPARAM lParam);

INT_PTR CALLBACK MidiInstrumentDlgProc (
   HWND hWnd,
   UINT uMsgId,
   WPARAM wParam,
   LPARAM lParam);

INT_PTR MidiInstrumentsWizard (
    HWND  hWnd,
    PMCMIDI pmcm,       // optional
    LPTSTR pszCmd);

//void LoadInstrumentsIntoTree (
//    HWND     hWnd,
//    UINT     uId,
//    LPTSTR   pszSelect,
//    HKEY *   phkMidi);

BOOL WINAPI RemoveInstrumentByKeyName (
    LPCTSTR pszKey);


// containing struct for what would otherwise be global variables
// only one instance of this structure is used.  (declared in main.c)
//
struct _globalstate {
    int                   cRef;

    BOOL                  fLoadedRegInfo;
    TCHAR                 szPlayCmdLn[128];
    TCHAR                 szOpenCmdLn[128];
    TCHAR                 szNewCmdLn[128];

    HWND                  hWndDeviceList;
    HWND                  hWndInstrList;
    //HWND                  hWndMainList;

    BOOL                  fChangeInput;
    BOOL                  fChangeOutput;
    BOOL                  fInputPort;
    DWORD                 dwReconfigFlags;

    WORD                  wHelpMessage;
    WORD                  wFill;

    UINT                  nDrivers;
    UINT                  nMaxDrivers;
    //DRIVER                aDrivers[16];
    };
extern struct _globalstate gs;

#endif // ifndef RC_INVOKED
