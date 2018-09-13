/*
 ***************************************************************
 *  mmcpl.h
 *
 *  Header file for mm control applet.
 *
 *
 *  History:
 *
 *      January -by- VijR
 *          Created.
 ***************************************************************
 */

#ifndef MMCPL_H
#define MMCPL_H

#define _INC_OLE
#ifndef STRICT
#define STRICT
#endif

#pragma warning( disable: 4103)
#include <windows.h>        // also includes windowsx.h
#include <shellapi.h>       // for registration functions
#include <windowsx.h>
#include <ole2.h>
#include <mmsystem.h>
#include <setupapi.h>

#include <shlobj.h>         // Shell OLE interfaces
#include <string.h>
#ifndef INITGUID
#include <shlobjp.h>
#endif
#include <commdlg.h>

#include "rcids.h"          // Resource declaration

#define PUBLIC          FAR PASCAL
#define CPUBLIC         FAR _cdecl
#define PRIVATE         NEAR PASCAL

//#include "utils.h"          // Common macros


/* Temporarily here until someone defines these for 16 bit side again. */
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS            0L
#endif

/*
 ***************************************************************
 * Constants and Definitions
 ***************************************************************
 */
#define     MIDI        TEXT("MIDI")
#define     ACM         TEXT("ACM")
#define     MSACM       TEXT("MSACM.")
#define     WAVE        TEXT("Wave")
#define     MIDIMAP     TEXT("MidiMapper")
#define     WAVEMAP     TEXT("WaveMapper")
#define     AUX         TEXT("Aux")
#define     MIXER       TEXT("Mixer")
#define     JOYSTICK    TEXT("Joystick")
#define     MCI         TEXT("MCI")
#define     ICM         TEXT("ICM")
#define     ICMSTR      TEXT("VIDC.")
#define     ICMSTR2     TEXT("VIDS.")
#define     VIDCAP      TEXT("MSVIDEO")

#define     AUDIO       TEXT("Audio")
#define     CDAUDIO     TEXT("CDAudio")
#define     VIDEO       TEXT("Video")


#define MAXSTR                  256    // maximum size of a string or filename
#define SZCODE                  const TCHAR
#define INTCODE                 const int
#define WINDOWS_DEFAULTENTRY    1
#define NONE_ENTRY              0

#define MAXNAME                 32      // Maximum name length
#define MAXLNAME                64
#define MAXMESSAGE              128     // Maximum resource string message
#define MAXSTRINGLEN            256     // Maximum output string length
#define MAXINTLEN               7       // Maximum interger string length
#define MAXLONGLEN              11      // Maximum long string length
#define MAXMSGLEN               512     // Maximum message length



#define WAVE_ID         0
#define MIDI_ID         1
#define MIXER_ID        2
#define AUX_ID          3
#define MCI_ID          4
#define ACM_ID          5
#define ICM_ID          6
#define VIDCAP_ID       7
#define JOYSTICK_ID     8

DEFINE_GUID(CLSID_mmsePropSheetHandler, 0x00022613L, 0x0000, 0x0000, 0xC0, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x46);


/*
 ***************************************************************
 * Internal STRUCTS used by mm.cpl
 ***************************************************************
 */

typedef struct _ClassNode
{
    short iNode;    //1 if class, 2 if device , 3-> ACM, 4->instrument
    TCHAR szClassName[64];
    TCHAR szClass[16];
    HICON hIcon;
} CLASSNODE, * PCLASSNODE;


typedef struct iResources
{
    short   iNode;
    TCHAR    szFriendlyName[MAXSTR];
    TCHAR    szDesc[MAXSTR];
    TCHAR    szParam[64];
    TCHAR    szFile[MAXSTR];
    TCHAR    szDrvEntry[64];
    TCHAR    szClass[16];
    HDRVR   hDriver;
    DWORD   dnDevNode;
    short   fQueryable;     // 0 -> can't, 1 -> can, -1 -> need to check
    short   iClassID;
    int     fStatus; //0 -> Disabled, 1-> Enabled and entry in reg, 2->Enabled but no entry in reg (i.e.old dev), 3->Enabled and ACTIVE, 4->inactive
    PCLASSNODE  pcn;
}IRESOURCE, *PIRESOURCE;

typedef struct _Instrument
{
    short   iNode;
    TCHAR    szFriendlyName[MAXSTR];
    TCHAR    szInstr[64];
    PIRESOURCE     pDev;
}INSTRUMENT, * PINSTRUMENT;

typedef struct event
{
    short   iNode;    //1 if module 2 if event
    short   fHasSound;
    LPTSTR    pszEvent;
    LPTSTR    pszEventLabel;
    LPTSTR    pszPath;
    struct event *npNextEvent;
} EVENT,  *PEVENT;

typedef struct module
{
    short   iNode;    //1 if module 2 if event
    LPTSTR    pszKey;
    LPTSTR    pszLabel;
    BOOL    fChange;
    PEVENT  npList;
} MODULE, *PMODULE;

typedef struct _AudioDlgInfo
{
    UINT    uPrefIn;
    UINT    uPrefOut;
    UINT    uPrefMIDIOut;
    UINT    cNumOutDevs;
    UINT    cNumInDevs;
    UINT    cNumMIDIOutDevs;
    BOOL    fPrefOnly;
} AUDIODLGINFO, * PAUDIODLGINFO;


typedef struct
{
    HDEVINFO            hDevInfo;
    PSP_DEVINFO_DATA    pDevInfoData;

} ALLDEVINFO, *PALLDEVINFO;

/*
 ***************************************************************
 * Globals and Strings used to loadstring resources at startup
 ***************************************************************
 */
#ifdef __cplusplus
extern "C" {
#endif

extern TCHAR        gszNone[];
extern TCHAR        gszRemoveScheme[];
extern TCHAR        gszChangeScheme[];
extern SZCODE      gszWindowsHlp[];
extern SZCODE      gszNull[];

extern SZCODE      cszWavExt[];
extern SZCODE      cszMIDIExt[];
extern SZCODE      cszRMIExt[];
extern SZCODE      cszAVIExt[];
extern SZCODE      cszSlash[];

extern SZCODE      cszAUDIO[];
extern SZCODE      cszVIDEO[];
extern SZCODE      cszCDAUDIO[];
extern SZCODE      cszMIDI[];

extern TCHAR       gszDevEnabled[];
extern TCHAR       gszDevDisabled[];
extern TCHAR       gszGeneral[];

extern HINSTANCE ghInstance;

extern INT_PTR PASCAL GetVerDesc (LPCTSTR pstrFile, LPTSTR pstrDesc);
extern BOOL PASCAL GetExeDesc (LPTSTR szFile, LPTSTR pszBuff, int cchBuff);
extern BOOL PASCAL LoadDesc(LPCTSTR pstrFile, LPTSTR pstrDesc);
extern void AddExt(LPTSTR pszFile, LPCTSTR cszExt);
extern BOOL PASCAL ValidateRegistry(void);


#define GEI_MODNAME         0x01
#define GEI_DESCRIPTION     0x02
#define GEI_FLAGS           0x03
#define GEI_EXEHDR          0x04
#define GEI_FAPI            0x05

MMRESULT GetWaveID(UINT *puWaveID);

void PASCAL ShowPropSheet(LPCTSTR            pszTitle,
    DLGPROC             pfnDialog,
    UINT                idTemplate,
    HWND                hWndParent,
    LPTSTR               pszCaption,
    LPARAM              lParam);

void PASCAL ShowMidiPropSheet(LPPROPSHEETHEADER ppshExt, LPCTSTR pszTitle,
    HWND                hWndParent,
    short               iMidiPropType,
    LPTSTR               pszCaption,
    HTREEITEM           hti,
    LPARAM              lParam1,
    LPARAM              lParam2);

void PASCAL ShowWithMidiDevPropSheet(LPCTSTR            pszTitle,
    DLGPROC             pfnDialog,
    UINT                idTemplate,
    HWND                hWndParent,
    LPTSTR               pszCaption,
    HTREEITEM           hti,
    LPARAM lParam, LPARAM lParamExt1, LPARAM lParamExt2);

#define MT_WAVE 1
#define MT_MIDI 2
#define MT_AVI  3
#define MT_ASF  4
#define MT_ERROR 0
BOOL mmpsh_ShowFileDetails(LPTSTR pszCaption, HWND hwndParent, LPTSTR pszFile, short iMediaType);

int FAR PASCAL lstrncmpi(LPCTSTR    lszKey,
    LPCTSTR    lszClass,
    int    iSize);


INT_PTR mmse_MessageBox(HWND hwndP,  LPTSTR szMsg, LPTSTR szTitle, UINT uStyle);


void PASCAL GetPropSheet(LPCTSTR            pszTitle,
    LPCTSTR              pszClass,
    DLGPROC             pfnDialog,
    UINT                idTemplate,
    HWND                hWndParent,
    HICON               hClassIcon,
    LPPROPSHEETHEADER ppsh, HPROPSHEETPAGE  * lphpsp);

BOOL      PASCAL ErrorBox               (HWND, int, LPTSTR);
int       PASCAL DisplayMessage(HWND hDlg, int iResTitle, int iResMsg, UINT uStyle);


BOOL ACMEnumCodecs(void);
void ACMCleanUp(void);
void ACMNodeChange(HWND hDlg);

BOOL CALLBACK MMExtPropSheetCallback(DWORD dwFunc, DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwInstance);
typedef BOOL (CALLBACK FAR * LPFNMMEXTPROPSHEETCALLBACK)(DWORD dwFunc, DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwInstance);

//Following are functions currently supported by the callback functions:
//dwInstance parameter which is passed to the external module when its entry point
//is called MUST always be passed back along with all the functions.

#define MM_EPS_GETNODEDESC      0x00000001
    //Gets the description displayed in the tree for the node for which the External Prop. Sheet is up.
    //For 'simple' property sheet this just returns a sheet Name
    //dwParam1 is a pointer to a string buffer in which the description is returned

#define MM_EPS_GETNODEID        0x00000002
    //Gets the Reg. Key Path offset from the MediaResources key
    //For 'simple' property sheet this just returns a sheet class
    //dwParam1 is a pointer to a string buffer in which the Key Path is returned

#define MM_EPS_ADDSHEET         0x00000003
    //Adds a property sheet for the current node in the tree.
    //dwParam1 = HPROPSHEETPAGE for the page being added.

#define MM_EPS_TREECHANGE       0x00000004
    //Notifies the CPL that the tree styructure has change. On receiving this function
    //the CPL rebuilds the subtree at this level and all levels below it.

#define MM_EPS_BLIND_TREECHANGE    0x00000005
    //Notification from MIDI sheet that something has changed in the MIDI subtree.
    //BLIND because the sheet was not launched from the Adv. Tab, so we dont know
    //what the heck he is talking about.

BOOL CALLBACK  AddSimpleMidiPages (LPTSTR    pszTitle, LPFNMMEXTPROPSHEETCALLBACK  lpfnAddPropSheetPage,LPARAM lParam);
BOOL CALLBACK  AddMidiPages (LPCTSTR    pszTitle, LPFNMMEXTPROPSHEETCALLBACK  lpfnAddPropSheetPage,LPARAM lParam);
BOOL CALLBACK  AddDevicePages (LPCTSTR    pszTitle, LPFNMMEXTPROPSHEETCALLBACK  lpfnAddPropSheetPage,LPARAM lParam);
BOOL CALLBACK  AddInstrumentPages (LPCTSTR    pszTitle, LPFNMMEXTPROPSHEETCALLBACK  lpfnAddPropSheetPage,LPARAM lParam);

DWORD WINAPI RunOnceSchemeInit (HWND hwnd, HINSTANCE hInst, LPTSTR szCmd, int nShow);
BOOL WINAPI SetRunOnceSchemeInit (void);

#ifdef FIX_BUG_15451
void ShowDriverSettings (HWND hDlg, LPTSTR pszName);
#endif // FIX_BUG_15451

#ifdef __cplusplus
} // extern "C"
#endif

#define MIDI_CLASS_PROP 1
#define MIDI_DEVICE_PROP 2
#define MIDI_INSTRUMENT_PROP 3


#define WM_ACMMAP_ACM_NOTIFY        (WM_USER + 100)

/*
 ***************************************************************
 * DEBUG Definitions
 ***************************************************************
 */
#ifdef ASSERT
#undef ASSERT
#endif
#ifdef DEBUG
#define STATIC
#ifdef DEBUG_TRACE
#define DPF_T    dprintf
#else
#define DPF_T 1 ? (void)0 : (void)
#endif
void FAR cdecl dprintf(LPSTR szFormat, ...);
#define DPF    dprintf
#define ddd    dprintf
#define ASSERT(f)                                                       \
    {                                                                   \
        if (!(f))                                                       \
            DPF("ERROR-ERROR#####: Assertion failed in %s on line %d @@@@@",__FILE__, __LINE__);                          \
    }


#else
#define STATIC static
#define ASSERT(f)
#define DPF 1 ? (void)0 : (void)
#define DPF_T 1 ? (void)0 : (void)
#endif

#endif // MMCPL_H
