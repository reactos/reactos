/*  DRIVERS.C
**
**  Copyright (C) Microsoft, 1990, All Rights Reserved.
**
**  Multimedia Control Panel Applet for installing/configuring installable
**  device drivers.  See the ispec doc DRIVERS.DOC for more information.
**
**  History:
**
**      Tue Jul 31 1990 -by- MichaelE
**          Created.
**
**      Thu Oct 25 1990 -by- MichaelE
**          Added restart, horz. scroll, added SKIPDESC reading desc. strings.
**
**      Sat Oct 27 1990 -by- MichaelE
**          Added FileCopy.  Uses SULIB.LIB and LZCOPY.LIB. Finished stuff
**          for case of installing a driver with more than one type.
**
**      May 1991 -by- JohnYG
**          Added and replaced too many things to list.  Better management
**          of removed drivers, correct usage of DRV_INSTALL/DRV_REMOVE,
**          installing VxD's, replaced "Unknown" dialog with an OEMSETUP.INF
**          method, proper "Cancel" method, fixed many potential UAE's.
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>
#include <windows.h>
#include <mmsystem.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <cpl.h>
#include <cphelp.h>
#include <commctrl.h>
#include <mmcpl.h>
#include <mmddkp.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>
#include <regstr.h>

#include "drivers.h"
#include "sulib.h"
#include "utils.h"
#include "medhelp.h"
#include "midi.h"
#ifdef FIX_BUG_15451
#include "trayvol.h"
#endif // FIX_BUG_15451

#ifndef cchLENGTH
#define cchLENGTH(_sz)  (sizeof(_sz) / sizeof((_sz)[0]))
#endif

#ifndef TreeView_GetGrandParent
#define TreeView_GetGrandParent(_htree,_hti) \
        TreeView_GetParent(_htree,TreeView_GetParent(_htree,_hti))
#endif

/*
 * Enable the definition below to cause MCI devices to be listed by their
 * internal descriptions in the tree, rather than their descriptions as
 * read from Drivers.Desc.
 *
 */
// #define GET_MCI_DEVICE_DESCRIPTIONS_FROM_THEIR_DEVICES

typedef struct
{
        int idIcon;
        int idName;
        int idInfo;
        BOOL bEnabled;
        DWORD dwContext;
        LPTSTR pszHelp;
} APPLET_INFO;

#define NUM_APPLETS     1
#define OBJECT_SIZE     1024

BOOL     bBadOemSetup;
BOOL     bRestart = FALSE;
int      iRestartMessage = 0;
BOOL     bInstallBootLine = FALSE;
BOOL     bCopyVxD;
BOOL     bFindOEM = FALSE;
BOOL     bRelated = FALSE;
BOOL     bCopyEvenIfOlder = FALSE;
BOOL     bDriversAppInUse;
BOOL     bCopyingRelated;
BOOL     bDescFileValid;
HANDLE   myInstance;
HWND     hAdvDlgTree;
UINT     wHelpMessage;
DWORD    dwContext;
PINF     pinfOldDefault = NULL;
TCHAR     szDriversHlp[24];
TCHAR     szLastQuery[20];
TCHAR     szSetupInf[18];
TCHAR     szKnown[250];
TCHAR     szRestartDrv[80];
TCHAR     szUnlisted[150];
TCHAR     szRelatedDesc[30];
TCHAR     szAppName[26];
TCHAR     szDrivers[12];
TCHAR     szRemove[12];
TCHAR     szControlIni[20];
TCHAR     szSysIni[20];
TCHAR     szMCI[6];
TCHAR     szOutOfRemoveSpace[54];
TCHAR     szDriversDesc[38];
TCHAR     szUserDrivers[38];

// Where the source of files to copy is - user updates

TCHAR     szDirOfSrc[MAX_PATH];
TCHAR     szAddDriver[36];
TCHAR     szNoDesc[36];
TCHAR     szError[20];
TCHAR     szRemoveOrNot[250];
TCHAR     szRemoveOrNotStrict[250];
TCHAR     szStringBuf[128];
TCHAR     szMDrivers[38];
TCHAR     szMDrivers32[38];
TCHAR     szFullPath[MAXFILESPECLEN];
TCHAR     szSystem[MAX_PATH];
TCHAR     szOemInf[MAX_PATH];
TCHAR     aszClose[16];
TCHAR     szFileError[50];

#ifdef FIX_BUG_15451
TCHAR    szDriverWhichNeedsSettings[MAX_PATH]; // See MMCPL.C
#endif // FIX_BUG_15451

static   HANDLE   hIList;
static   HANDLE   hWndMain;

/*
 *  Global flag telling us if we're allowed to write to ini files
 */

 BOOL IniFileReadAllowed;
 BOOL IniFileWriteAllowed;


/*
 *** Stuff for keeping track of the TreeView window
 *
 */

#define GetString(_psz,_id) LoadString(myInstance,(_id),(_psz),sizeof((_psz))/sizeof(TCHAR))

static struct   // aDriverKeyword
   {
   LPTSTR       psz;    // text found as alias for driver
   DriverClass  dc; // DriverClass inferred from keyword
   }
aDriverKeyword[] =  // (used by GuessDriverClass())
   {
      { TEXT("waveaudio"),   dcMCI      },    // (sort in inverse alphabetical;
      { TEXT("wavemap"),     dcWAVE     },    //  in particular, longer names first)
      { TEXT("wave"),        dcWAVE     },
      { TEXT("vids"),        dcVCODEC   },
      { TEXT("vidc"),        dcVCODEC   },
      { TEXT("sequencer"),   dcMCI      },
      { TEXT("msvideo"),     dcVIDCAP   },
      { TEXT("msacm"),       dcACODEC   },
      { TEXT("mpegvideo"),   dcMCI      },
      { TEXT("mixer"),       dcMIXER    },
      { TEXT("midimapper"),  dcMIDI     },
      { TEXT("midi"),        dcMIDI     },
      { TEXT("mci"),         dcMCI      },
      { TEXT("icm"),         dcVCODEC   },
      { TEXT("cdaudio"),     dcMCI      },
      { TEXT("avivideo"),    dcMCI      },
      { TEXT("aux"),         dcAUX      },
      { TEXT("acm"),         dcACODEC   },
      { TEXT("joy"),         dcJOY      }
   };

#define nDriverKEYWORDS ((int)(sizeof(aDriverKeyword) / \
                               sizeof(aDriverKeyword[0])))

static struct   // aKeywordDesc
   {
   DriverClass  dc; // DriverClass
   LPTSTR       psz;    // alias which best describes class
   }
aKeywordDesc[] =    // (used by DriverClassToClassNode())
   {
      { dcWAVE,    TEXT("wave")     },
      { dcMIXER,   TEXT("mixer")    },
      { dcVIDCAP,  TEXT("msvideo")  },
      { dcVCODEC,  TEXT("icm")      },
      { dcAUX,     TEXT("aux")      },
      { dcACODEC,  TEXT("acm")      },
      { dcMIDI,    TEXT("midi")     },
      { dcJOY,     TEXT("joystick") }
   };

#define nKeywordDESCS   ((int)(sizeof(aKeywordDesc) / \
                               sizeof(aKeywordDesc[0])))

static struct   // aDriverRoot
   {
   DriverClass  dc; // corresponding driver classification
   BOOL         fAlwaysMake;    // TRUE if should exist even w/o child
   int          idIcon; // icon for items under this tree
   int          idDesc; // description string for parent
   int          idEnable;   // string to describe enabling action
   int          idDisable;  // string to describe disabling action
   HTREEITEM    hti;    // item within tree
   DWORD        dwBit;  // bit mask representing this node
   }
aDriverRoot[] = // (order will define order in display)
   {
      { dcINVALID, TRUE,  IDI_MMICON, IDS_MM_HEADER,
                                      0,
                                      0 },
      { dcWAVE,    TRUE,  IDI_WAVE,   IDS_WAVE_HEADER,
                                      IDS_ENABLEAUDIO,
                                      IDS_DISABLEAUDIO },
      { dcMIDI,    TRUE,  IDI_MIDI,   IDS_MIDI_HEADER,
                                      IDS_ENABLEMIDI,
                                      IDS_DISABLEMIDI },
      { dcMIXER,   TRUE,  IDI_MIXER,  IDS_MIXER_HEADER,
                                      IDS_ENABLEMIXER,
                                      IDS_DISABLEMIXER },
      { dcAUX,     TRUE,  IDI_AUX,    IDS_AUX_HEADER,
                                      IDS_ENABLEAUX,
                                      IDS_DISABLEAUX },
      { dcMCI,     TRUE,  IDI_MCI,    IDS_MCI_HEADER,
                                      IDS_ENABLEMCI,
                                      IDS_DISABLEMCI },
      { dcVCODEC,  TRUE,  IDI_ICM,    IDS_ICM_HEADER,
                                      IDS_ENABLEICM,
                                      IDS_DISABLEICM },
      { dcACODEC,  TRUE,  IDI_ACM,    IDS_ACM_HEADER,
                                      IDS_ENABLEACM,
                                      IDS_DISABLEACM },
      { dcVIDCAP,  TRUE,  IDI_VIDEO,  IDS_VIDCAP_HEADER,
                                      IDS_ENABLECAP,
                                      IDS_DISABLECAP },
      { dcJOY,     TRUE, IDI_JOYSTICK,IDS_JOYSTICK_HEADER,
                                      IDS_ENABLEJOY,
                                      IDS_DISABLEJOY },
      { dcOTHER,   FALSE, IDI_MMICON, IDS_OTHER_HEADER,
                                      IDS_ENABLEJOY,
                                      IDS_DISABLEJOY },
   };

#define nDriverROOTS ((int)(sizeof(aDriverRoot) / sizeof(aDriverRoot[0])))

static LPCTSTR aDriversToSKIP[] =
   {
   TEXT( "MMDRV.DLL" ),
   TEXT( "MIDIMAP.DLL" ),
   TEXT( "MSACM32.DRV" )
   };


static TCHAR cszMMDRVDLL[]    = TEXT("MMDRV.DLL");
static TCHAR cszAliasKERNEL[] = TEXT("KERNEL");
static TCHAR cszRegValueLOADTYPE[] = TEXT("Load Type");

#define nDriversToSKIP ((int)( sizeof(aDriversToSKIP)   \
                             / sizeof(aDriversToSKIP[0]) ))

static HIMAGELIST  hImageList = NULL;   // image list for treeview in advdlg
DriverClass g_dcFilterClass = dcINVALID;

short       DriverClassToRootIndex        (DriverClass);
DriverClass GuessDriverClass              (PIDRIVER);
#ifdef FIX_BUG_15451
DriverClass GuessDriverClassFromAlias     (LPTSTR);
#endif // FIX_BUG_15451
DriverClass GuessDriverClassFromTreeItem  (HTREEITEM hti);
BOOL        EnsureRootIndexExists         (HWND, short);
HTREEITEM   AdvDlgFindTopLevel            (void);
BOOL        InitAdvDlgTree                (HWND);
void        FreeAdvDlgTree                (HWND);
void        TreeContextMenu               (HWND, HWND);

int         lstrnicmp      (LPTSTR, LPTSTR, size_t);
LPTSTR      lstrchr        (LPTSTR, TCHAR);
void        lstrncpy       (LPTSTR, LPTSTR, size_t);

void        ShowDeviceProperties          (HWND, HTREEITEM);

PIDRIVER    FindIDriverByTreeItem         (HTREEITEM);

#ifdef FIX_BUG_15451
HTREEITEM   FindTreeItemByDriverName      (LPTSTR);
#endif // FIX_BUG_15451

// We want to run "control joy.cpl" when the joystick devices
// are highlight and the user clicks Add/Remove/Properties buttons.
BOOL RunJoyControlPanel(void);  //qzheng

/*
 ***
 *
 */


DWORD GetFileDateTime     (LPTSTR);
LPTSTR  GetProfile          (LPTSTR,LPTSTR, LPTSTR, LPTSTR, int);
void  AddIDrivers         (HWND, LPTSTR, LPTSTR);
HTREEITEM AddIDriver      (HWND, PIDRIVER, DriverClass);
BOOL  AddIDriverByName    (HWND, LPCWSTR, DriverClass);
PIDRIVER GetSelectedIDriver (HWND);
BOOL  FillTreeFromWinMM   (HWND);
BOOL  FillTreeFromMSACM   (HWND);
BOOL  FillTreeFromMCI     (HWND);
BOOL  FillTreeFromMIDI    (HWND);
BOOL  FillTreeFromRemaining (HWND);
void  FillTreeFromRemainingBySection (HWND, long ii, LPCTSTR, DriverClass);
BOOL CALLBACK FillTreeFromMSACMQueryCallback (HACMDRIVERID, DWORD, DWORD);
int __cdecl FillTreeFromMSACMSortCallback (const void *p1, const void *p2);
BOOL  InitAvailable       (HWND, int);
void  RemoveAvailable     (HWND);
BOOL  UserInstalled       (LPTSTR);
INT_PTR  RestartDlg          (HWND, unsigned, WPARAM, LPARAM);
INT_PTR  AddUnlistedDlg      (HWND, unsigned, WPARAM, LPARAM);
INT_PTR   AvailableDriversDlg (HWND, unsigned, WPARAM, LPARAM);
BOOL  AdvDlg              (HWND, unsigned, UINT, LONG);
LONG  CPlApplet           (HWND, unsigned, UINT, LONG);
void  ReBoot              (HWND);
BOOL  GetMappable         (PIDRIVER);
BOOL  SetMappable         (PIRESOURCE, BOOL);

#define REGSTR_PATH_WAVEMAPPER  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Wave Mapper")
#define REGSTR_VALUE_MAPPABLE   TEXT("Mappable")

#define REGSTR_PATH_MCI         TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\MCI")
#define REGSTR_PATH_MCI32       TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\MCI32")
#define REGSTR_PATH_DRIVERS     TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Drivers")
#define REGSTR_PATH_DRIVERS32   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32")


/*
 * REALLOC - Allows expansion of GlobalAlloc()'d block while retaining contents
 *
 *    Newly-allocated portions of memory are initialized to zero, while
 *    the contents of reallocated portions of memory are retained.
 *
 * Parameters:
 *    LPVOID   _pData....allocated array
 *    mysize_t _cOld.....current count of elements in allocated array
 *    mysize_t _cNew.....minimum number of elements requested
 *    mysize_t _cDelta...granularity of increase
 *
 * Example:
 *    {
 *    mysize_t   cElements = 0; // Number of elements allocated so far
 *    DataType  *aElements = NULL;  // Allocated array of DataType
 *
 *    // At this point, cElements == 0 (obviously)
 *
 *    REALLOC (aElements, cElements, 10, 16)
 *
 *    // The line above requested 10 elements, and indicated that elements
 *    // should be allocated in increments of 16.  So cElements is 16 at this
 *    // point (thus, sizeof(aElements) == sizeof(DataType)*16).
 *
 *    REALLOC (aElements, cElements, 12, 16)
 *
 *    // The line above requested 12 elements.  Since cElements is already 16,
 *    // REALLOC knows that 12 elements are already available--and does nothing.
 *
 *    REALLOC (aElements, cElements, 17, 16)
 *
 *    // The line above requested 17 elements, in increments of 16.  aElements
 *    // has been reallocated to contain 32 elements, and cElements is
 *    // therefore 32.
 *
 *    GlobalFree ((HGLOBAL)aElements);  // All done!
 *    aElements = NULL;
 *    cElements = 0;
 *    }
 *
 *
 */

typedef signed long mysize_t;

#ifdef REALLOC
#undef REALLOC
#endif
#define REALLOC(_pData,_cOld,_cNew,_cDelta) \
        ReallocFn (sizeof(*_pData), (void **)&_pData, &_cOld, _cNew, _cDelta)

#ifdef DivRoundUp
#undef DivRoundUp
#endif
#define DivRoundUp(_a,_b) ( (LONG)(((_a) + (_b) -1) / (_b)) )

#ifdef RoundUp
#undef RoundUp
#endif
#define RoundUp(_a,_b) ( DivRoundUp(_a,_b) * (LONG)_b )

BOOL ReallocFn (mysize_t cbElement,
                LPVOID *ppOld, mysize_t *pcOld, mysize_t cNew, mysize_t cDelta)
{
   LPVOID     pNew;
   mysize_t   cbOld, cbNew;

            // First check if we actually need to reallocate or not.
            // It's possible that {ppOld} was already allocated with
            // enough space.
            //
   if ( ((*ppOld) != NULL)  && (cNew <= (*pcOld)) )
      return TRUE;

            // Oh well.  Determine how much space we need, and how much
            // is allocated now.
            //
   cNew  = RoundUp (cNew, cDelta);
   cbNew = cbElement * cNew;
   cbOld = (ppOld == NULL) ? 0 : (cbElement * (*pcOld));

            // Allocate the space and copy over the original contents.
            // Zero-fill the remaining space.
            //
   if ((pNew = (LPVOID)GlobalAlloc (GMEM_FIXED, cbNew)) == NULL)
      return FALSE;

   if (cbOld != 0)
      {
      memcpy (pNew, *ppOld, cbOld);
      GlobalFree ((HGLOBAL)(*ppOld));
      }

   memset (&((char*)pNew)[ cbOld ], 0x00, cbNew -cbOld);

            // Finally, update the passed-in pointers and we're done.
            //
   *pcOld = cNew;
   *ppOld = pNew;

   return TRUE;
}


/*
 * PIDRIVER ARRAY
 *
 * The AddIDrivers() routine LocalAlloc()'s a single IDRIVER structure
 * for each installed device driver.  Pointers to these structures are
 * retained in the InstalledDriver array, and indices into this array
 * are stored as the LPARAM values for each tree item.  Each element
 * in the array stores not only a pointer to the driver's IDRIVER structure,
 * but also a DWORD which, as a combination of aDriverRoot[].dwBit values,
 * reflects the tree root items under which the driver has tree items.
 *
 */

typedef struct  // InstalledDriver
   {
   PIDRIVER  pIDriver;     // Pointer to AddIDrivers()'s PIDRIVER structure
   DWORD     dwBits;       // Combination of aDriverRoot[].dwBit flags
   } InstalledDriver;

InstalledDriver  *aInstalledDrivers = NULL;
mysize_t          cInstalledDrivers = 0;

#define NOPIDRIVER  ((LPARAM)-1)



/*
 *  CheckSectionAccess()
 *
 *  See if we can read/write to a given section
 */


 BOOL CheckSectionAccess(TCHAR *szIniFile, TCHAR *SectionName)
 {
     static TCHAR TestKey[] = TEXT("TestKey!!!");
     static TCHAR TestData[] = TEXT("TestData");
     static TCHAR ReturnData[50];

    /*
     *   Check we can write, read back and delete our key
     */

     return WritePrivateProfileString(SectionName,
                                      TestKey,
                                      TestData,
                                      szIniFile) &&

            GetPrivateProfileString(SectionName,
                                    TestKey,
                                    TEXT(""),
                                    ReturnData,
                                    sizeof(ReturnData) / sizeof(TCHAR),
                                    szIniFile) == (DWORD)wcslen(TestData) &&

            WritePrivateProfileString(SectionName,
                                      TestKey,
                                      NULL,
                                      szIniFile);
 }


/*
 *  CheckIniAccess()
 *
 *  Checks access to our 2 .ini file sections - DRIVERS_SECTION and
 *  MCI_SECTION by just writing and reading some junk
 *
 *  Basically if we don't have access to these sections we're not
 *  going to allow Add and Remove.  The individual MCI drivers must
 *  take care not to put their data into non-writeable storage although
 *  this completely messes up the default parameters thing so we're going
 *  to put these into a well-known key in the win.ini file (ie per user).
 *
 */

 BOOL CheckIniAccess(void)
 {
     return CheckSectionAccess(szSysIni, szDrivers) &&
            CheckSectionAccess(szSysIni, szMCI) &&
            CheckSectionAccess(szControlIni, szUserDrivers) &&
            CheckSectionAccess(szControlIni, szDriversDesc) &&
            CheckSectionAccess(szControlIni, szRelatedDesc);
 }

/*
 *  QueryRemoveDrivers()
 *
 *  Ask the user if they're sure.  If the Driver is one required by the
 *  system (ie not listed in [Userinstallable.drivers] in control.ini)
 *  warn the user of that too.
 */

 BOOL QueryRemoveDrivers(HWND hDlg, LPTSTR szKey, LPTSTR szDesc)
 {
     TCHAR bufout[MAXSTR];

     if (UserInstalled(szKey))
          wsprintf(bufout, szRemoveOrNot, (LPTSTR)szDesc);
     else
          wsprintf(bufout, szRemoveOrNotStrict, (LPTSTR)szDesc);

     return (MessageBox(hDlg, bufout, szRemove,
                    MB_ICONEXCLAMATION | MB_TASKMODAL | MB_YESNO) == IDYES );
 }

/*
 *  GetProfile()
 *
 *  Get private profile strings.
 */

 LPTSTR GetProfile(LPTSTR pstrAppName, LPTSTR pstrKeyName, LPTSTR pstrIniFile,
                 LPTSTR pstrRet, int cbSize)
 {
     TCHAR szNULL[2];

     szNULL[0] = TEXT('\0');
     GetPrivateProfileString(pstrAppName, (pstrKeyName==NULL) ? NULL :
         (LPTSTR)pstrKeyName, szNULL, pstrRet, cbSize/sizeof(TCHAR), pstrIniFile);
     return(pstrRet);
 }

/*********************************************************************
 *
 *  AddIDrivers()
 *
 *  Add drivers in the passed key strings list to the InstalledDrivers array
 *
 *********************************************************************/

void AddIDrivers(HWND hWnd, LPTSTR pstrKeys, LPTSTR pstrSection)
{
    PIDRIVER    pIDriver;
    LPTSTR        pstrKey;
    LPTSTR        pstrDesc;

    pstrKey = pstrKeys;
    pstrDesc = (LPTSTR)LocalAlloc(LPTR, MAXSTR);

   /*
    *  parse key strings for profile, and make IDRIVER structs
    */

    while ( *pstrKey )
    {
        pIDriver = (PIDRIVER)LocalAlloc(LPTR, sizeof(IDRIVER));
        if ( pIDriver )
        {
            LPTSTR        pstr;

            if (*GetProfile(pstrSection, pstrKey, szSysIni, pIDriver->szFile,
                sizeof(pIDriver->szFile)) == TEXT('\0'))
            {
                LocalFree((HANDLE)pIDriver);
                goto nextkey;
            }

            for ( pstr=pIDriver->szFile; *pstr && (*pstr!=COMMA) &&
                (*pstr!=SPACE); pstr++ )
                    ;
            *pstr = TEXT('\0');

#ifdef TRASHDRIVERDESC
            if (bDescFileValid)
#endif
              /*
               *  try to load the cached description
               */

               GetProfile(szDriversDesc,
                          pIDriver->szFile,
                          szControlIni,
                          pIDriver->szDesc,
                          sizeof(pIDriver->szDesc));

           /*
            *  if we failed, then try to get the information from
            *  mmdriver.inf or the exehdr
            */

            if (pIDriver->szDesc[0] == TEXT('\0'))
            {
               if (LoadDescFromFile(pIDriver, pstrKey, pstrDesc) != DESC_NOFILE)
               {
                   if (!*pstrDesc)
                   {
                       /*
                        *  failed to load a description.
                        *  The file isn't in setup.inf
                        *  and doesn't have exehdr information
                        */

                        lstrcpy(pIDriver->szDesc, pIDriver->szFile);
                        lstrcat(pIDriver->szDesc, szNoDesc);
                   }
                   else
                        lstrcpy(pIDriver->szDesc, pstrDesc);

                   WritePrivateProfileString(szDriversDesc, pIDriver->szFile,
                               pIDriver->szDesc, szControlIni);
               } else {
                    LocalFree((HANDLE)pIDriver);
                    goto nextkey;
               }
            }

            wcsncpy(pIDriver->szAlias, pstrKey, sizeof(pIDriver->szAlias)/sizeof(TCHAR));
            pIDriver->szAlias[sizeof(pIDriver->szAlias)/sizeof(TCHAR) - 1] = TEXT('\0');
            wcscpy(pIDriver->wszAlias, pIDriver->szAlias);

            wcsncpy(pIDriver->szSection, pstrSection,sizeof(pIDriver->szSection)/sizeof(TCHAR));
            pIDriver->szSection[sizeof(pIDriver->szSection)/sizeof(TCHAR) - 1] = TEXT('\0');
            wcscpy(pIDriver->wszSection, pIDriver->szSection);

            pIDriver->KernelDriver = IsFileKernelDriver(pIDriver->szFile);
            pIDriver->fQueryable = pIDriver->KernelDriver ? 0 : -1;

            pIDriver->lp = 0L;

            if (!AddIDriverToArray (pIDriver))
               LocalFree((HANDLE)pIDriver);
        }
        else
           break;  //ERROR Low Memory

nextkey: while (*pstrKey++);
    }
    LocalFree((HANDLE)pstrDesc);
}


/*
 * AddIDriverToArray - Adds the given PIDRIVER to the InstalledDrivers array
 *
 */

BOOL AddIDriverToArray (PIDRIVER pIDriver)
{
    mysize_t  ii;

    if (pIDriver == NULL)
    {
        return FALSE;
    }

            // Don't create duplicate entries in this array; one PIDRIVER
            // per driver-file is sufficient.
            //
    for (ii = 0; ii < cInstalledDrivers; ++ii)
    {
        if (aInstalledDrivers[ ii ].pIDriver != NULL)
        {
            if (!lstrcmpi (aInstalledDrivers[ ii ].pIDriver->szFile,
                           pIDriver->szFile))
            {
                return FALSE;
            }
        }
    }

            // To reduce repetitive calls to GlobalAlloc(), we'll allocate
            // space for an additional 50 InstalledDriver entries within
            // the aInstalledDrivers array each time we run out of space.
            //
#define nDriverEntriesToAllocAtONCE   50

    for (ii = 0; ii < cInstalledDrivers; ++ii)
    {
        if (aInstalledDrivers[ ii ].pIDriver == NULL)
            break;
    }

    if (ii >= cInstalledDrivers)
    {
        if (!REALLOC (aInstalledDrivers,   // Array
                      cInstalledDrivers,   // Current size of array
                      1+ii,                // Requested size of array
                      nDriverEntriesToAllocAtONCE))
        {
            return FALSE;
        }
    }

    aInstalledDrivers[ ii ].pIDriver = pIDriver;
    aInstalledDrivers[ ii ].dwBits = 0L;

    return TRUE;
}


/*********************************************************************
 *
 *  FindInstallableDriversSection()
 *
 *********************************************************************/

PINF FindInstallableDriversSection(PINF pinf)
{
    PINF pinfFound;

    pinfFound = infFindSection(pinf, szMDrivers32);

    if (pinfFound == NULL) {
        pinfFound = infFindSection(pinf, szMDrivers);
    }

    return pinfFound;
}

//NOTE: Returns nSize as a count of bytes, not characters (later calls expect this)
int GetINISectionSize(LPCTSTR pstrSection, LPCTSTR pstrFile)
{
    int ncbSize = 0;
    int ncbMaxSize = 0;

    while (ncbSize >= ncbMaxSize)
    {
        TCHAR szNULL[2];
        LPTSTR pStr = NULL;

        szNULL[0] = TEXT('\0');

        ncbMaxSize += SECTION; //allocate another 512 bytes

        pStr = (LPTSTR)LocalAlloc(LPTR, ncbMaxSize);

        if (!pStr)
        {
            //we're trying to allocate too much memory ...
            //drop out and use the last smaller size that worked
            break;
        }

        ncbSize = GetPrivateProfileString(pstrSection, NULL, szNULL, pStr, ncbMaxSize/sizeof(TCHAR), pstrFile);
        ncbSize = (ncbSize+2) * sizeof(TCHAR);  //convert to byte count, adding two chars
                                                //to account for terminating null and API's truncation

        LocalFree(pStr);
    }

    return (ncbSize);
}


/*********************************************************************
 *
 *  InitInstalled()
 *
 *  Add the drivers installed in [DRIVERS] and [MCI] to the Installed
 *  Drivers list box.
 *
 *********************************************************************/

BOOL InitInstalled(HWND hWnd, LPTSTR pstrSection)
{
    BOOL    bSuccess=FALSE;
    LPTSTR    pstr;
    int nSize = SECTION;

#ifdef TRASHDRIVERDESC
    UINT    wTime;
    BOOL    fForce;
    TCHAR    szOut[10];

    wTime = LOWORD(GetFileDateTime(szControlIni)) >> 1;
    if (fForce = (GetPrivateProfileInt((LPTSTR)szUserDrivers,
                   (LPTSTR)szLastQuery,  0, (LPTSTR)szControlIni) != wTime))
    {
        wsprintf(szOut, TEXT("%d"), wTime);
        WritePrivateProfileString((LPTSTR)szUserDrivers, (LPTSTR)szLastQuery,
                                        szOut, (LPTSTR)szControlIni);
        WritePrivateProfileString((LPTSTR)szDriversDesc, NULL, NULL,
                                                (LPTSTR)szControlIni);
        bDescFileValid = FALSE;
    }
    else
        bDescFileValid = TRUE;
#endif

    nSize = GetINISectionSize(pstrSection, szSysIni);

    pstr = (LPTSTR)LocalAlloc(LPTR, nSize);
    if ( pstr )
    {
        if (*GetProfile(pstrSection, NULL, szSysIni, pstr, nSize ))
        {
            AddIDrivers(hWnd,pstr,pstrSection);
            bSuccess = TRUE;
        }

        LocalFree((HANDLE)pstr);
    }

    return(bSuccess);
}


/*
 * RefreshAdvDlgTree - Clears the Devices tree, and fills it back in
 *
 */

void RefreshAdvDlgTree (void)
{
   if (hAdvDlgTree != NULL)
      {
      SendMessage (hAdvDlgTree, WM_SETREDRAW, FALSE, 0L);

      FreeAdvDlgTree (hAdvDlgTree);
      InitAdvDlgTree (hAdvDlgTree);
      InitInstalled (GetParent (hAdvDlgTree), szDrivers);
      InitInstalled (GetParent (hAdvDlgTree), szMCI);
      FillTreeInAdvDlg (hAdvDlgTree, NULL);

      SendMessage (hAdvDlgTree, WM_SETREDRAW, TRUE, 0L);
      }
}


/*
 * FillTreeInAdvDlg - Adds TreeItems for each entry in aInstalledDrivers
 *
 * If pIDriver is specified, the first treeitem to mention that driver
 * will be highlighted.
 *
 */

BOOL FillTreeInAdvDlg (HWND hTree, PIDRIVER pIDriver)
{
   if (!FillTreeFromWinMM (hTree))
      return FALSE;

   if (!FillTreeFromMSACM (hTree))
      return FALSE;

   if (!FillTreeFromMCI (hTree))
      return FALSE;

   if (!FillTreeFromMIDI (hTree))
      return FALSE;

   if (!FillTreeFromRemaining (hTree))
      return FALSE;

   if (pIDriver != NULL)    // Do we have to highlight a pIDriver?
      {
      short idr;

      for (idr = 0; idr < nDriverROOTS; idr++)
         {
         HTREEITEM  hti;

         if ((hti = aDriverRoot[ idr ].hti) == NULL)
            continue;

         for (hti = TreeView_GetChild (hTree, hti);
              hti != NULL;
              hti = TreeView_GetNextSibling (hTree, hti))
            {
            if (pIDriver == FindIDriverByTreeItem (hti))
               {
               TreeView_SelectItem (hTree, hti);
               break;
               }
            }

         if (hti != NULL)   // Found and selected a TreeItem?
            break;  // Then we're done!
         }
      }

   return TRUE;
}


/*
 * FillTreeFromWinMM - Adds tree items for all WinMM-controlled MM devices
 *
 * This routine adds tree items under the following DriverClasses:
 *    dcWAVE    - waveOut*
 *    dcMIXER   - mixer*
 *    dcAUX     - aux*
 *
 */

BOOL FillTreeFromWinMM (HWND hTree)
{
   UINT     iDevice;
   UINT     cDevices;
   WCHAR    szDriver[ cchRESOURCE ];

            // Add entries for each waveOut device
            //
   cDevices = waveOutGetNumDevs ();
   for (iDevice = 0; iDevice < cDevices; ++iDevice)
      {
      if (waveOutMessage ((HWAVEOUT)iDevice,
                          DRV_QUERYFILENAME,
                          (DWORD_PTR)szDriver,
                          (DWORD_PTR)cchRESOURCE) == MMSYSERR_NOERROR)
         {
         AddIDriverByName (hTree, szDriver, dcWAVE);
         }
      }

            // Add entries for each mixer device
            //
   cDevices = mixerGetNumDevs ();
   for (iDevice = 0; iDevice < cDevices; ++iDevice)
      {
      if (mixerMessage ((HMIXER)iDevice,
                        DRV_QUERYFILENAME,
                        (DWORD_PTR)szDriver,
                        (DWORD_PTR)cchRESOURCE) == MMSYSERR_NOERROR)
         {
         AddIDriverByName (hTree, szDriver, dcMIXER);
         }
      }

            // Add entries for each aux device
            //
   cDevices = auxGetNumDevs ();
   for (iDevice = 0; iDevice < cDevices; ++iDevice)
      {
      if (auxOutMessage (iDevice,
                         DRV_QUERYFILENAME,
                         (DWORD_PTR)szDriver,
                         (DWORD_PTR)cchRESOURCE) == MMSYSERR_NOERROR)
         {
         AddIDriverByName (hTree, szDriver, dcAUX);
         }
      }

   return TRUE;
}


/*
 * FillTreeFromMSACM - Adds tree items for all MSACM-controlled MM devices
 *
 * This routine adds tree items under the following DriverClasses:
 *    dcACODEC  - acmDriverEnum()
 *
 * Note that, since audio codecs are supposed to be sorted in the tree,
 * all audio codec treeitems are first deleted from the tree (if there
 * are any at this point) then all audio codecs are added in their
 * sorted order.
 *
 */

typedef struct
   {
   DWORD     dwPriority;    // priority of this audio codec
   PIDRIVER  pIDriver;      // matching driver file (or NULL)
   WORD      wMid;          // manufacturer ID
   WORD      wPid;          // product ID
   TCHAR      szDesc[ ACMDRIVERDETAILS_LONGNAME_CHARS ];
   } AudioCodec;

AudioCodec  *pCodecs;
mysize_t     cCodecs;

extern BOOL gfLoadedACM;   // From MSACMCPL.C

BOOL FillTreeFromMSACM (HWND hTree)
{
   MMRESULT  mmr;
   short     idr;
   mysize_t  ii;

   if (!gfLoadedACM)
      {
      if (LoadACM())
         gfLoadedACM = TRUE;
      }
   if (!gfLoadedACM)
      return FALSE;

            // Step one: get rid of any audio codecs listed in the tree
            //
   if ((idr = DriverClassToRootIndex (dcACODEC)) != -1)
      {
      if (aDriverRoot[ idr ].hti != NULL)
         {
         HTREEITEM hti;

         while ((hti = TreeView_GetChild (hTree, aDriverRoot[ idr ].hti)) != 0)
            {
            TreeView_DeleteItem (hTree, hti);

            if (hti == TreeView_GetChild (hTree, aDriverRoot[ idr ].hti))
               break;  // if it didn't delete, make sure we don't loop forever!
            }
         }

      for (ii = 0; ii < cInstalledDrivers; ++ii)
         {
         aInstalledDrivers[ ii ].dwBits &= ~aDriverRoot[ idr ].dwBit;
         }
      }

            // Step two: query ACM to obtain the list of codecs
            //
   pCodecs = NULL;
   cCodecs = 0;

   mmr = (MMRESULT)acmDriverEnum (FillTreeFromMSACMQueryCallback,
                        0,
                        ACM_DRIVERENUMF_NOLOCAL | ACM_DRIVERENUMF_DISABLED);

               // Step three: sort the list of codecs and add each to the tree
               //
   if ((mmr == MMSYSERR_NOERROR) && (pCodecs != NULL))
      {
      mysize_t  iiDr;

      qsort (pCodecs, (size_t)cCodecs, sizeof(AudioCodec),
             FillTreeFromMSACMSortCallback);

                  // Assign lp=wMid+wPid for each audio codec we find
                  //
      for (iiDr = 0; iiDr < cInstalledDrivers; ++iiDr)
         {
         if (aInstalledDrivers[ iiDr ].pIDriver == NULL)
            continue;
         if (aInstalledDrivers[ iiDr ].pIDriver->lp != 0L)  // already did this
            continue;

         if (GuessDriverClass (aInstalledDrivers[ iiDr ].pIDriver) == dcACODEC)
            {
            HANDLE hDriver;

            hDriver = OpenDriver (aInstalledDrivers[iiDr].pIDriver->wszAlias,
                                  aInstalledDrivers[iiDr].pIDriver->wszSection,
                                  0L);
            if (hDriver != NULL)
               {
               ACMDRIVERDETAILSW add;
               memset ((TCHAR *)&add, 0x00, sizeof(add));
               add.cbStruct = sizeof(add);
               SendDriverMessage (hDriver, ACMDM_DRIVER_DETAILS, (LONG_PTR)&add, 0);
               CloseDriver (hDriver, 0L, 0L);

               aInstalledDrivers[ iiDr ].pIDriver->lp = MAKELONG( add.wMid,
                                                                  add.wPid );
               }
            }
         }

                  // Search for installed drivers with matching lp=wMid+wPid's
                  //
      for (iiDr = 0; iiDr < cInstalledDrivers; ++iiDr)
         {
         if (aInstalledDrivers[ iiDr ].pIDriver == NULL)
            continue;

         if ((aInstalledDrivers[ iiDr ].pIDriver->szAlias[0] == TEXT('\0')) ||
             (GuessDriverClass (aInstalledDrivers[iiDr].pIDriver) == dcACODEC))
            {
            for (ii = 0; ii < cCodecs; ++ii)
               {
               if (pCodecs[ ii ].dwPriority == 0)
                  continue;

               if ( (pCodecs[ ii ].wMid ==
                     LOWORD( aInstalledDrivers[ iiDr ].pIDriver->lp )) &&
                    (pCodecs[ ii ].wPid ==
                     HIWORD( aInstalledDrivers[ iiDr ].pIDriver->lp )) )
                  {
                  pCodecs[ ii ].pIDriver = aInstalledDrivers[ iiDr ].pIDriver;
                  break;
                  }
               }
            }
         }

                  // Add each in-use entry in pCodecs to the treeview
                  //
      for (ii = 0; ii < cCodecs; ++ii)
         {
         if (pCodecs[ ii ].dwPriority == 0)
            continue;

                     // The PCM converter, for instance, won't have a matching
                     // PID.  So create a bogus one--the lack of an szAlias
                     // will let us know it's bogus--and insert it into the
                     // aInstalledDrivers array.
                     //
         if (pCodecs[ ii ].pIDriver == NULL)
            {
            PIDRIVER  pid = (PIDRIVER)LocalAlloc(LPTR, sizeof(IDRIVER));

            if (pid != NULL)
               {
               memset (pid, 0x00, sizeof(IDRIVER));
               pid->lp = MAKELONG( pCodecs[ ii ].wMid, pCodecs[ ii ].wPid );
               lstrcpy (pid->szDesc, pCodecs[ ii ].szDesc);

               if (!AddIDriverToArray (pid))
                  LocalFree ((HLOCAL)pid);
               else
                  {
                  pCodecs[ ii ].pIDriver = pid;
                  }
               }
            }

         if (pCodecs[ ii ].pIDriver != NULL)
            {
            AddIDriver (hTree, pCodecs[ ii ].pIDriver, dcACODEC);
            }
         }
      }

               // Cleanup
               //
   if (pCodecs != NULL)
      {
      GlobalFree ((HGLOBAL)pCodecs);
      pCodecs = NULL;
      cCodecs = 0;
      }

   return (mmr == MMSYSERR_NOERROR) ? TRUE : FALSE;
}


/*
 * FillTreeFromMCI - Adds tree items for all MCI devices
 *
 * This routine adds tree items under the following DriverClasses:
 *    dcMCI     - mciSendCommand
 *
 */

BOOL FillTreeFromMCI (HWND hTree)
{
   MCI_SYSINFO_PARMS mciSysInfo;
   TCHAR szAlias[ cchRESOURCE ];

            // How many MCI devices does WinMM know about?
            //
   memset ((TCHAR *)&mciSysInfo, 0x00, sizeof(mciSysInfo));
   mciSysInfo.lpstrReturn = szAlias;
   mciSysInfo.dwRetSize = cchLENGTH(szAlias);
   mciSysInfo.wDeviceType = MCI_ALL_DEVICE_ID;

   if (mciSendCommand (MCI_ALL_DEVICE_ID,
                       MCI_SYSINFO,
                       MCI_SYSINFO_QUANTITY,
                       (DWORD_PTR)&mciSysInfo) == 0)
      {
      DWORD    iDevice;
      DWORD    cDevices;

      cDevices = *((DWORD *)(mciSysInfo.lpstrReturn));

               // Get the name of each MCI device in turn.
               //
      for (iDevice = 0; iDevice < cDevices; ++iDevice)
         {
         mysize_t ii;

         memset ((TCHAR *)&mciSysInfo, 0x00, sizeof(mciSysInfo));
         mciSysInfo.lpstrReturn = szAlias;
         mciSysInfo.dwRetSize = cchLENGTH(szAlias);
         mciSysInfo.wDeviceType = MCI_ALL_DEVICE_ID;
         mciSysInfo.dwNumber = 1+iDevice;  // note: 1-based, not 0-based!

         if (mciSendCommand (MCI_ALL_DEVICE_ID,
                             MCI_SYSINFO,
                             MCI_SYSINFO_NAME,
                             (DWORD_PTR)&mciSysInfo) != 0)
            {
            continue;
            }

                  // Got an alias--search the InstalledDrivers array
                  // and try to find a matching PIDRIVER.
                  //
         for (ii = 0; ii < cInstalledDrivers; ++ii)
            {
            if (aInstalledDrivers[ ii ].pIDriver == NULL)
               continue;
            if (!lstrcmpi (aInstalledDrivers[ ii ].pIDriver->szAlias, szAlias))
               {
#ifdef GET_MCI_DEVICE_DESCRIPTIONS_FROM_THEIR_DEVICES
               MCI_OPEN_PARMS  mciOpen;
               MCI_INFO_PARMS  mciInfo;
               MCIERROR        rc;

                        // It's an installed, functioning, happy MCI device.
                        // Open it up and see what it calls itself; update the
                        // description in the PIDRIVER (what's in there was
                        // obtained from the registry, thus from MMDRIVER.INF,
                        // and we instead want what Media Player lists in its
                        // Device menu).
                        //
               memset ((TCHAR *)&mciOpen, 0x00, sizeof(mciOpen));
               mciOpen.lpstrDeviceType = szAlias;

               rc = mciSendCommand (0,MCI_OPEN,MCI_OPEN_TYPE,(DWORD)&mciOpen);
               if (rc == MCIERR_MUST_USE_SHAREABLE)
                  {
                  rc = mciSendCommand (0, MCI_OPEN,
                                       MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE,
                                       (DWORD)&mciOpen);
                  }
               if (rc == 0)
                  {
                  TCHAR szDesc[ cchRESOURCE ];
                  szDesc[0] = 0;

                  mciInfo.lpstrReturn = szDesc;
                  mciInfo.dwRetSize = cchLENGTH(szDesc);

                  if (mciSendCommand (mciOpen.wDeviceID,
                                      MCI_INFO,
                                      MCI_INFO_PRODUCT,
                                      (DWORD)&mciInfo) == 0)
                     {
                     lstrcpy (aInstalledDrivers[ ii ].pIDriver->szDesc, szDesc);
                     }

                  mciSendCommand (mciOpen.wDeviceID, MCI_CLOSE, 0L, 0);
                  }
#endif // GET_MCI_DEVICE_DESCRIPTIONS_FROM_THEIR_DEVICES

               AddIDriver (hTree, aInstalledDrivers[ ii ].pIDriver, dcMCI);
               break;
               }
            }
         }
      }

   return TRUE;
}


/*
 * FillTreeFromMIDI - Adds tree items for all MIDI devices and instruments
 *
 * This routine adds tree items under the following DriverClasses:
 *    dcMIDI    - LoadInstruments() provides necessary data
 *
 */

BOOL FillTreeFromMIDI (HWND hTree)
{
   MCMIDI mcm;
   UINT   iiRoot;
   int    idrMIDI;

   if ((idrMIDI = DriverClassToRootIndex (dcMIDI)) == -1)
   return FALSE;

            // First load in all relevant information regarding MIDI
            // instruments.  Fortunately, all that work is encapsulated
            // nicely within one routine.
            //
   memset (&mcm, 0x00, sizeof(mcm));
   LoadInstruments (&mcm, FALSE);

            // Each entry in mcm's api array is one of three things:
            //  - a parent (say, a sound card)
            //  - a child (say, an external instrument)
            //  - the "(none)" thing that we want to skip
            //
            // Add each parent to the tree, and when we find a parent,
            // add all its children.
            //
   for (iiRoot = 0; iiRoot < mcm.nInstr; ++iiRoot)
      {
      TCHAR    szName[ MAXSTR ];
      LPTSTR   pch;
      PIDRIVER pid;

      if (mcm.api[ iiRoot ] == NULL)
         continue;
      if (mcm.api[ iiRoot ]->piParent != NULL)
         continue;
      if (mcm.api[ iiRoot ]->szKey[0] == TEXT('\0'))
         continue;

               // Found a parent!  If we can match it to an installed driver,
               // add it to the tree.  Note that mcm.api[]->szKey will
               // be of the form "MMDRV.DLL<0000>"--we need to strip off
               // the "<0000>" before we can match this thing to a PIDRIVER.
               //
      lstrcpy (szName, mcm.api[ iiRoot ]->szKey);
      if ((pch = lstrchr (szName, TEXT('<'))) != NULL)
         *pch = TEXT('\0');

      if ((pid = FindIDriverByName (szName)) != NULL)
         {
         HTREEITEM hti;
         UINT      ii;
         TV_ITEM   tvi;

         if ((hti = AddIDriver (hTree, pid, dcMIDI)) == NULL)
            continue;

#if 0
         tvi.mask = TVIF_TEXT;
         tvi.hItem = hti;
         tvi.pszText = mcm.api[ iiRoot ]->szFriendly;
         TreeView_SetItem(hTree, &tvi);
#endif

                  // We've added this parent.  See if it has any children,
                  // and if so, stick 'em in the tree.
                  //
         for (ii = 0; ii < mcm.nInstr; ++ii)
            {
            PINSTRUM lp;
            TV_INSERTSTRUCT ti;

            if (mcm.api[ ii ] == NULL)
               continue;
            if (mcm.api[ ii ]->piParent != mcm.api[ iiRoot ])
               continue;

                     // Yep--it's got a parent.  Allocate a second copy
                     // of this PINSTRUM; that copy will be our LPARAM value.
                     //
            if ((lp = (PINSTRUM)LocalAlloc(LPTR,sizeof (INSTRUM))) == NULL)
               continue;
            memcpy ((TCHAR *)lp, (TCHAR *)mcm.api[ ii ], sizeof (INSTRUM));

                     // Now add a treeitem for this instrument.
                     //
            ti.hParent = hti;
            ti.hInsertAfter = TVI_LAST;
            ti.item.mask = TVIF_TEXT | TVIF_PARAM |
                           TVIF_IMAGE | TVIF_SELECTEDIMAGE;
            ti.item.iImage = (int)idrMIDI;
            ti.item.iSelectedImage = (int)idrMIDI;
            ti.item.pszText = mcm.api[ ii ]->szFriendly;
            ti.item.lParam = (LPARAM)lp;

            if (!TreeView_InsertItem (hTree, &ti))
               break;
            }
         }
      }

            // Done--cleanup and we're out of here.
            //
   FreeInstruments (&mcm);

   if (mcm.hkMidi)
      RegCloseKey (mcm.hkMidi);

   return TRUE;
}


            // To reduce repetitive calls to GlobalAlloc(), we'll allocate
            // space for an additional 10 AudioCodec entries within
            // the pCodecs array each time we run out of space.
            //
#define nAudioCodecEntriesToAllocAtONCE   10

BOOL CALLBACK FillTreeFromMSACMQueryCallback (HACMDRIVERID hadid,
                                              DWORD dwUser,
                                              DWORD fdwSupport)
{
   short       ii;
   AudioCodec *pac;
   ACMDRIVERDETAILS add;

            // Find or create a place in which to store information
            // about this codec
            //
   for (ii = 0; ii < cCodecs; ++ii)
      {
      if (pCodecs[ ii ].dwPriority == 0)
         break;
      }
   if (ii >= cCodecs)
      {
        if (!REALLOC (pCodecs, cCodecs, 1+ii, nAudioCodecEntriesToAllocAtONCE))
            return FALSE;
      }
   pac = &pCodecs[ ii ];    // for shorthand

            // Find out about this codec
            //
   memset ((TCHAR *)&add, 0x00, sizeof(add));
   add.cbStruct = sizeof(add);
   if (acmDriverDetails (hadid, &add, 0) == MMSYSERR_NOERROR)
      {
      acmMetrics ((HACMOBJ)hadid,ACM_METRIC_DRIVER_PRIORITY,&pac->dwPriority);

      lstrcpy (pac->szDesc, add.szLongName);

      pac->wMid = add.wMid;
      pac->wPid = add.wPid;

      pac->pIDriver = NULL;
      }

   return TRUE; // keep counting
}


int __cdecl FillTreeFromMSACMSortCallback (const void *p1, const void *p2)
{
   if (((AudioCodec *)p1)->dwPriority == 0)
      return 1;
   if (((AudioCodec *)p2)->dwPriority == 0)
      return -1;
   return ((AudioCodec *)p1)->dwPriority - ((AudioCodec *)p2)->dwPriority;
}



/*
 * FillTreeFromRemaining - Adds tree items for all remaining MM devices
 *
 * This routine adds a single tree item for each entry in the aInstalledDrivers
 * array which is not already represented somewhere in the tree.  The
 * classification is based on the driver's alias--if that fails, it is lumped
 * under dcOTHER.
 *
 */

BOOL FillTreeFromRemaining (HWND hTree)
{
   mysize_t ii;

   for (ii = 0; ii < cInstalledDrivers; ++ii)
      {
      UINT iiSkipCheck;

      if (aInstalledDrivers[ ii ].pIDriver == NULL)
         continue;
      if (aInstalledDrivers[ ii ].pIDriver->szAlias[0] == TEXT('\0'))
         continue;

            // (don't do this for any not-to-be-displayed drivers)
            //
      for (iiSkipCheck = 0; iiSkipCheck < nDriversToSKIP; iiSkipCheck++)
         {
         if (!FileNameCmp ((LPTSTR)aDriversToSKIP[ iiSkipCheck ],
                           (LPTSTR)aInstalledDrivers[ ii ].pIDriver->szFile))
            break;
         }
      if (iiSkipCheck < nDriversToSKIP)
         continue;

            // Zip through the {drivers,drivers32,mci,mci32} sections, to
            // try to classify this driver. If we find a classification
            // for which we haven't already added an entry in the tree,
            // add another.
            //
      FillTreeFromRemainingBySection (hTree,
                                      ii,
                                      REGSTR_PATH_DRIVERS,
                                      dcINVALID);

      FillTreeFromRemainingBySection (hTree,
                                      ii,
                                      REGSTR_PATH_DRIVERS32,
                                      dcINVALID);

      FillTreeFromRemainingBySection (hTree,
                                      ii,
                                      REGSTR_PATH_MCI,
                                      dcMCI);

      FillTreeFromRemainingBySection (hTree,
                                      ii,
                                      REGSTR_PATH_MCI32,
                                      dcMCI);

            // If the dwBits element is zero, then this driver hasn't
            // already been assigned a treeitem elsewhere.  In that event,
            // call AddIDriver() with dcOTHER--to tell it to lump this
            // driver under "Other Drivers".
            //
      if (aInstalledDrivers[ ii ].dwBits == 0)
         {
         AddIDriver (hTree, aInstalledDrivers[ ii ].pIDriver, dcOTHER);
         }
      }

   return TRUE;
}


void FillTreeFromRemainingBySection (HWND hTree,
                                     long iiDriver,
                                     LPCTSTR pszSection,
                                     DriverClass dcSection)
{
   HKEY hk;
   UINT ii;

   if (RegOpenKey (HKEY_LOCAL_MACHINE, pszSection, &hk))
      return;

   for (ii = 0; ; ++ii)
      {
      TCHAR  szLHS[ cchRESOURCE ];
      TCHAR  szRHS[ cchRESOURCE ];
      DWORD  dw1;
      DWORD  dw2;
      DWORD  dw3;

      dw1 = cchRESOURCE;
      dw3 = cchRESOURCE;
      if (RegEnumValue (hk, ii,  szLHS, &dw1,
                        0, &dw2, (LPBYTE)szRHS, &dw3) != ERROR_SUCCESS)
         {
         break;
         }

      if (!FileNameCmp (szRHS, aInstalledDrivers[ iiDriver ].pIDriver->szFile))
         {
         DriverClass dc;

         if ((dc = dcSection) == dcINVALID)
            dc = GuessDriverClassFromAlias (szLHS);

         if ((dc == dcINVALID) || (dc == dcOTHER))
            continue;

         (void)AddIDriver (hTree, aInstalledDrivers[ iiDriver ].pIDriver, dc);
         }
      }

   RegCloseKey (hk);
}


#ifdef FIX_BUG_15451
HWND MakeThisCPLLookLikeTheOldCPL (HWND hWndCPL)
{
   TCHAR  szTitle[ cchRESOURCE ];
   HWND   hWndOldCPL = NULL;

   GetWindowText (hWndCPL, szTitle, cchRESOURCE);

   for (hWndOldCPL = GetWindow (hWndCPL, GW_HWNDFIRST);
        hWndOldCPL != NULL;
        hWndOldCPL = GetWindow (hWndOldCPL, GW_HWNDNEXT))
      {
      TCHAR  szTitleTest[ cchRESOURCE ];
      GetWindowText (hWndOldCPL, szTitleTest, cchRESOURCE);
      if ( (!lstrcmpi (szTitle, szTitleTest)) && (hWndCPL != hWndOldCPL) )
         {
         RECT  rOld;
         GetWindowRect (hWndOldCPL, &rOld);
         SetWindowPos (hWndCPL, hWndOldCPL, rOld.left, rOld.top,
                       0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
         SetWindowPos (hWndOldCPL, hWndCPL, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
         }
      }

    return hWndOldCPL;
}


HWND MakeThisDialogLookLikeTheOldDialog (HWND hDlg)
{
   TCHAR  szTitle[ cchRESOURCE ];
   RECT   rOld;
   POINT  pt;
   HWND   hWndOldDlg;

   GetWindowText (hDlg, szTitle, cchRESOURCE);

   for (hWndOldDlg = GetWindow (hDlg, GW_HWNDFIRST);
        hWndOldDlg != NULL;
        hWndOldDlg = GetWindow (hWndOldDlg, GW_HWNDNEXT))
      {
      TCHAR  szTitleTest[ cchRESOURCE ];
      GetWindowText (hWndOldDlg, szTitleTest, cchRESOURCE);
      if ( (!lstrcmpi (szTitle, szTitleTest)) && (hDlg != hWndOldDlg) )
         {
         GetWindowRect (hWndOldDlg, &rOld);

         SetWindowPos (hDlg, NULL, rOld.left, rOld.top, 0, 0,
                       SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

         return hWndOldDlg;
         }
      }

   return NULL;
}



BOOL WaitForNewCPLWindow (HWND hWndMyDlg)
{
   TCHAR  szTitle[ cchRESOURCE ];
   HWND   hWnd;
   DWORD  tickStart;

#define msecMAXWAIT   5000

   hWndMyDlg = GetParent (hWndMyDlg); // (hWndMyDlg was a property sheet)

   GetWindowText (hWndMyDlg, szTitle, cchRESOURCE);

   for (tickStart = GetTickCount();
        GetTickCount() - tickStart < msecMAXWAIT;)
      {
      MSG msg;

      for (hWnd = GetWindow (hWndMyDlg, GW_HWNDFIRST);
           hWnd != NULL;
           hWnd = GetWindow (hWnd, GW_HWNDNEXT))
         {
         TCHAR  szTitleTest[ cchRESOURCE ];
         if (!IsWindowVisible (hWnd))
            continue;
         GetWindowText (hWnd, szTitleTest, cchRESOURCE);
         if ( (!lstrcmpi (szTitle, szTitleTest)) && (hWnd != hWndMyDlg) )
            {
            PropSheet_PressButton (hWndMyDlg, PSBTN_CANCEL);
            hWnd = GetParent (GetParent (hAdvDlgTree));
            PropSheet_PressButton (hWnd, PSBTN_CANCEL);
            return TRUE;
            }
         }

      if (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
         {
         if (GetMessage (&msg, NULL, 0, 0))
            {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
            }
         }
      }

   return FALSE;
}
#endif // FIX_BUG_15451


        // Prevent any more REMOVE button presses
        // Otherwise one can get stacked up and cause trouble,
        // particularly if it is assocated with a driver that
        // is automatically removed.  We have to use a static
        // as any focus changes cause the button to change state.
        //
static long fWorking = 0;




/********************************************************************
 *
 *  AdvDlg ()
 *
 *  Display list of installed installable drivers.  Return TRUE/FALSE
 *  indicating if should restart windows.
 *
 ********************************************************************/

const static DWORD aAdvDlgHelpIds[] = {  // Context Help IDs
    IDC_ADV_TREE,    IDH_GENERIC_DEVICES,
    ID_ADV_PROP,     IDH_ADV_PROPERTIES,
    ID_ADV_REMOVE,   IDH_MMCPL_DEVPROP_REMOVE,
    ID_ADV_ADD,      IDH_ADV_ADDDRIVER,
//  ID_ADV_TSHOOT,   IDH_ADV_TSHOOT,        // BUGBUG need context help

    0, 0
};

void MapDriverClass(DWORD_PTR dwSetupClass)
{
    g_dcFilterClass = dcINVALID;

    switch (dwSetupClass)
    {
        case IS_MS_MMMCI :
        {
            g_dcFilterClass = dcMCI;    
        }
        break;

        case IS_MS_MMVID :
        {
            g_dcFilterClass = dcVCODEC;    
        }
        break;

        case IS_MS_MMACM :
        {
            g_dcFilterClass = dcACODEC;    
        }
        break;

        case IS_MS_MMVCD :
        {
            g_dcFilterClass = dcVIDCAP;    
        }
        break;

        case IS_MS_MMDRV :
        {
            g_dcFilterClass = dcLEGACY;
        }
        break;
    }
}

BOOL AdvDlg (HWND hDlg, UINT uMsg, UINT wParam, LONG lParam)
{
    HANDLE          hWndI, hWnd;
    PIDRIVER        pIDriver;
    DWORD_PTR       dwType = 0;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
#ifdef FIX_BUG_15451
        if (szDriverWhichNeedsSettings[0] != TEXT('\0'))
        {
            MakeThisCPLLookLikeTheOldCPL (GetParent(hDlg));
        }
#endif // FIX_BUG_15451

            wsStartWait();

            if (lParam)
            {
                dwType = ((LPPROPSHEETPAGE) lParam)->lParam;
            }

            MapDriverClass(dwType);

            hWndI = GetDlgItem(hDlg, IDC_ADV_TREE);
            SendMessage(hWndI,WM_SETREDRAW, FALSE, 0L);

            InitAdvDlgTree (hWndI); // initialize the treeview display

           /*
            *  Handle the fact that we may not be able to update our .ini
            *  sections
            *
            */

            IniFileWriteAllowed = CheckIniAccess();

                  // Note nasty sneaky hack: using (A|B) instead of (A&&B)
                  // makes both functions evaluate in either success or
                  // failure cases.
                  //
            IniFileReadAllowed = ( InitInstalled (hDlg, szDrivers) |
                                   InitInstalled (hDlg, szMCI) );

            FillTreeInAdvDlg (GetDlgItem (hDlg, IDC_ADV_TREE), NULL);

            wsEndWait();

            if ((!IniFileReadAllowed) || (!IniFileWriteAllowed))
            {
                TCHAR szCantAdd[120];
                EnableWindow(GetDlgItem(hDlg, ID_ADV_ADD),FALSE);
                EnableWindow(GetDlgItem(hDlg, ID_ADV_REMOVE),FALSE);
                LoadString(myInstance,IDS_CANTADD,szCantAdd,sizeof(szCantAdd)/sizeof(TCHAR));
                MessageBox(hDlg, szCantAdd, szError,
                                MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
            }


            SendMessage (hWndI, WM_SETREDRAW, TRUE, 0L);

            break;

        case WM_COMMAND:
            hWndI = GetDlgItem(hDlg, IDC_ADV_TREE);
            hWndMain = hDlg;

            pIDriver = GetSelectedIDriver (hWndI);

            switch ( LOWORD(wParam ))
            {
                case ID_ADV_PROP:
                    {
                HTREEITEM    htiCur = TreeView_GetSelection (hWndI);
                DriverClass  dc = GuessDriverClassFromTreeItem (htiCur);

                        if (fWorking)
                            break;

                        ++fWorking; // Just starting an operation

                        if( dc == dcJOY )
                            // We want to run "control joy.cpl" when the joystick devices
                            // are highlight and the user clicks Properties buttons.
                            RunJoyControlPanel();
            else
                        ShowDeviceProperties (hDlg, TreeView_GetSelection(hWndI));

                    --fWorking; // Finished with this operation
                    }
                    break;

                case ID_WHATSTHIS:
                    {
                    WinHelp((HWND)GetDlgItem (hDlg, IDC_ADV_TREE),
                            gszWindowsHlp, HELP_WM_HELP,
                           (UINT_PTR)(LPTSTR)aAdvDlgHelpIds);
                    }
                    break;

                case ID_ADV_REMOVE:
                    {
                    HWND         hTree = GetDlgItem (hDlg, IDC_ADV_TREE);
                    HTREEITEM    htiCur = TreeView_GetSelection (hTree);
                    DriverClass  dc = GuessDriverClassFromTreeItem (htiCur);
                    PIDRIVER     pid;
                    LONG_PTR         Status;

                    if ((!IniFileReadAllowed) || (!IniFileWriteAllowed))
                       break;   // (button should be disabled)

                    if( dc == dcJOY ) {
                         RunJoyControlPanel();
                break;
            }

                    if (TreeView_GetParent (hAdvDlgTree, htiCur) &&
                        TreeView_GetGrandParent (hAdvDlgTree, htiCur) &&
                        (GuessDriverClassFromTreeItem (
                                 TreeView_GetGrandParent (hAdvDlgTree, htiCur)
                                                      ) == dcMIDI))
                       {
                       TV_ITEM tvi;
                       PINSTRUM pin;

                       tvi.mask = TVIF_PARAM;
                       tvi.hItem = htiCur;
                       tvi.lParam = 0;
                       TreeView_GetItem(hAdvDlgTree, &tvi);

                       if ((pin = (PINSTRUM)tvi.lParam) != NULL)
                          {
                          RemoveInstrumentByKeyName (pin->szKey);
                          RefreshAdvDlgTree ();
                          KickMapper (hDlg);
                          }

                       break;
                       }

                    if ((pid = FindIDriverByTreeItem (htiCur)) == NULL)
                       break;

                    if (dc == dcLEGACY)
                    {
                        dc = GuessDriverClass(pid);
                    }

                    if (pid->szAlias[0] == TEXT('\0'))
                       {
                       TCHAR szCantRemove[ cchRESOURCE ];
                       GetString(szCantRemove, IDS_ACMREMOVEFAIL);
                       MessageBox(hDlg, szCantRemove, szError,
                                  MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
                       break;
                       }

                    if (fWorking)
                       break;
                    ++fWorking; // Just starting an operation

                    if (QueryRemoveDrivers (hDlg, pid->szAlias, pid->szDesc))
                       {
                       if ((Status = PostRemove (pid, TRUE)) != DRVCNF_CANCEL)
                          {
                          switch (dc)
                             {
                             case dcMIDI:
                                   break;
                             case dcACODEC:
                                   acmDeleteCodec (LOWORD(pid->lp),
                                                   HIWORD(pid->lp));
                                   break;
                             default:
                                   SetRunOnceSchemeInit ();
                                   break;
                             }

                          iRestartMessage = IDS_RESTART_REM;

                          if (Status == DRVCNF_RESTART)
                             {
                             DialogBox (myInstance,
                                        MAKEINTRESOURCE(DLG_RESTART),
                                        hDlg,
                                        RestartDlg);
                             }
                          }
                       }

                    --fWorking; // Finished with this operation
                    }
                    break;

                case ID_ADV_ADD:
                    {
                HTREEITEM    htiCur = TreeView_GetSelection (hWndI);
                DriverClass  dc = GuessDriverClassFromTreeItem (htiCur);

                        if ((!IniFileReadAllowed) || (!IniFileWriteAllowed))
                            break;  // (button should be disabled)

                        if( dc == dcJOY ) {
                            RunJoyControlPanel();
                break;
            }

                    if (fWorking)
                       break;
                    ++fWorking; // Just starting an operation

                    bCopyEvenIfOlder = FALSE;

                    DialogBox(myInstance, MAKEINTRESOURCE(DLG_KNOWN), hDlg,
                              AvailableDriversDlg);

                    bCopyEvenIfOlder = FALSE;

                    --fWorking; // Finished with this operation
                    }
                    break;


                case ID_ADV_TSHOOT:
                    {
                        TCHAR szCommand[ MAX_PATH ];
                        STARTUPINFO si;
                        PROCESS_INFORMATION pi;
                        LoadString(myInstance,IDS_TSHOOT, szCommand, sizeof(szCommand)/sizeof(TCHAR));
                        ZeroMemory(&si, sizeof(si));
                        si.cb = sizeof(si);
                        si.dwFlags = STARTF_USESHOWWINDOW;
                        si.wShowWindow = SW_NORMAL;
                        if (CreateProcess(NULL, szCommand, NULL, NULL, FALSE, 0, 0, NULL, &si, &pi)) {
                            CloseHandle(pi.hThread);
                            CloseHandle(pi.hProcess);
                        }
                    }
                    break;

#ifdef FIX_BUG_15451
        case ID_INIT:
            if (szDriverWhichNeedsSettings[0] != TEXT('\0'))
               {
               HTREEITEM hti;

               if ((hti = FindTreeItemByDriverName (
                        szDriverWhichNeedsSettings)) != 0)
               {
               TreeView_Expand (hAdvDlgTree,
                        TreeView_GetParent(hAdvDlgTree,hti),
                        TVE_EXPAND);
               TreeView_SelectItem(hAdvDlgTree,hti);
               FORWARD_WM_COMMAND(hDlg,ID_ADV_PROP,0,0,PostMessage);
               }
               else
               {
               szDriverWhichNeedsSettings[0] = 0;
               }
               }
                    break;
#endif // FIX_BUG_15451

                default:
                    return(FALSE);
            }
            break;

      case WM_NOTIFY:
            {
            NMHDR         *lpnm   = (NMHDR *)lParam;
            LPNM_TREEVIEW  lpnmtv = (LPNM_TREEVIEW)lParam;

            switch (lpnm->code)
               {
               case PSN_KILLACTIVE:
                     FORWARD_WM_COMMAND (hDlg, IDOK, 0, 0, SendMessage);
                    break;

               case PSN_APPLY:
                     FORWARD_WM_COMMAND (hDlg, ID_APPLY, 0, 0, SendMessage);
                    break;

               case PSN_SETACTIVE:
                     FORWARD_WM_COMMAND (hDlg, ID_INIT, 0, 0, SendMessage);
                    break;

               case PSN_RESET:
                     FORWARD_WM_COMMAND (hDlg, IDCANCEL, 0, 0, SendMessage);
                    break;

               case NM_DBLCLK:
                              // show properties or expand/collapse tree node.
                              //
                     if (lpnm->idFrom == (UINT)IDC_ADV_TREE)
                        {
                        HWND           hTree =  GetDlgItem (hDlg, IDC_ADV_TREE);
                        HTREEITEM      htiCur = TreeView_GetSelection (hTree);
                        TV_HITTESTINFO tvht;

                        if (!htiCur)
                           break;

                        GetCursorPos (&tvht.pt);
                        ScreenToClient (hTree, &tvht.pt);
                        TreeView_HitTest (hTree, &tvht);

                        if ( (tvht.flags & TVHT_ONITEM) &&
                             (TreeView_GetChild (hTree, htiCur) == NULL) &&
                             (IsWindowEnabled (GetDlgItem(hDlg,ID_ADV_PROP))) )
                           {
                           FORWARD_WM_COMMAND(hDlg,ID_ADV_PROP,0,0,PostMessage);
                           }
                        }
                    break;

               case NM_RCLICK:
                     TreeContextMenu (hDlg, GetDlgItem (hDlg, IDC_ADV_TREE));
                     return TRUE;
                    break;
               }
            }
           break;


        // The TreeView has its own right-click handling, and presents a
        // "What's This?" automatically--so don't handle WM_CONTEXTMENU
        // for that control.
        //
        case WM_CONTEXTMENU:
            if (wParam != (WPARAM)GetDlgItem (hDlg, IDC_ADV_TREE))
            {
                WinHelp((HWND)wParam, gszWindowsHlp, HELP_CONTEXTMENU,
                       (UINT_PTR)(LPTSTR)aAdvDlgHelpIds);
            }
            break;

        case WM_HELP:
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, gszWindowsHlp,
                  HELP_WM_HELP, (UINT_PTR)(LPTSTR)aAdvDlgHelpIds);
            break;

        case WM_DESTROY:
            FreeAdvDlgTree (GetDlgItem (hDlg, IDC_ADV_TREE));
            return FALSE;
            break;

        default:
            return FALSE;
            break;
    }
    return(TRUE);
}


/*
 *** TreeContextMenu
 *
 * This function displays the context menu that pops up when the
 * user right clicks on any of the tree view items.
 *
 */
void TreeContextMenu (HWND hWnd, HWND hKeyTreeWnd)
{
   DWORD MessagePos;
   POINT MessagePoint;
   TV_HITTESTINFO TVHitTestInfo;
   HMENU hContextMenu;
   HMENU hContextPopupMenu;
   TV_ITEM TVItem;
   int MenuCommand;
   TCHAR szCollapse[32];

            // dont bring up a menu unless click is on the item.
            //
   MessagePos = GetMessagePos();
   MessagePoint.x = LOWORD(MessagePos);
   MessagePoint.y = HIWORD(MessagePos);

   TVHitTestInfo.pt = MessagePoint;
   ScreenToClient(hKeyTreeWnd, &TVHitTestInfo.pt);
   TVItem.hItem = TreeView_HitTest(hKeyTreeWnd, &TVHitTestInfo);

   if (TVItem.hItem == NULL)
      return;

   hContextMenu = LoadMenu(ghInstance, MAKEINTRESOURCE(POPUP_TREE_CONTEXT));
   if (hContextMenu == NULL)
      return;

   hContextPopupMenu = GetSubMenu (hContextMenu, 0);

   TVItem.mask = TVIF_STATE | TVIF_HANDLE | TVIF_CHILDREN | TVIF_PARAM;
   TreeView_GetItem(hKeyTreeWnd, &TVItem);

               // show collapse item because we are expanded?
               //
   if (TVItem.state & TVIS_EXPANDED)
      {
      LoadString(ghInstance, IDS_COLLAPSE, szCollapse, sizeof(szCollapse)/sizeof(TCHAR));
      ModifyMenu(hContextPopupMenu, ID_TOGGLE, MF_BYCOMMAND | MF_STRING,
                 ID_TOGGLE, szCollapse);
      }
   SetMenuDefaultItem (hContextPopupMenu, ID_TOGGLE, MF_BYCOMMAND);

   if (TVItem.cChildren == 0) //gray expand/collaps if no children
      {
      SetMenuDefaultItem(hContextPopupMenu, ID_ADV_PROP, MF_BYCOMMAND);
      EnableMenuItem(hContextPopupMenu, ID_TOGGLE, MF_GRAYED |MF_BYCOMMAND);
      }

   TreeView_SelectItem (hKeyTreeWnd, TVItem.hItem);
   MenuCommand = TrackPopupMenuEx (hContextPopupMenu,
                                   TPM_RETURNCMD | TPM_RIGHTBUTTON |
                                   TPM_LEFTALIGN | TPM_TOPALIGN,
                                   MessagePoint.x, MessagePoint.y,
                                   hWnd, NULL);

   DestroyMenu (hContextMenu);
   FORWARD_WM_COMMAND(hWnd, MenuCommand, 0, 0, SendMessage);
}



/*--------------------------------------------------------------------------*
 *                                                                          *
 *                                                                          *
 *  LB_AVAILABLE Dialog Routines                                            *
 *                                                                          *
 *                                                                          *
 *--------------------------------------------------------------------------*/

/*
 *  DLG: LB_AVAILABLE
 *
 *  InitAvailable()
 *
 *  Add the available drivers from mmdriver.inf to the passed list box.
 *  The format of [Installable.drivers] in setup.inf is:
 *  profile=disk#:driverfile,"type1,type2","Installable driver Description","vxd1.386,vxd2.386","opt1,2,3"
 *
 *  for example:
 *
 *  driver1=6:sndblst.drv,"midi,wave","SoundBlaster MIDI and Waveform drivers","vdmad.386,vadmad.386","3,260"
 */

BOOL InitAvailable(HWND hWnd, int iLine)
{
    PINF    pinf;
    BOOL    bInitd=FALSE;
    LPTSTR    pstrKey;
    int     iIndex;
    TCHAR    szDesc[MAX_INF_LINE_LEN];

    SendMessage(hWnd,WM_SETREDRAW, FALSE, 0L);

   /*
    *  Parse the list of keywords and load their strings
    */

    for (pinf = FindInstallableDriversSection(NULL); pinf; pinf = infNextLine(pinf))
    {
        //
        // found at least one keyname!
        //
        bInitd = TRUE;
        if ( (pstrKey = (LPTSTR)LocalAlloc(LPTR, MAX_SYS_INF_LEN)) != NULL )
                infParseField(pinf, 0, pstrKey);
        else
            break;
       /*
        *  add the installable driver's description to listbox, and filename as data
        */

        infParseField(pinf, 3, szDesc);

        if ( (iIndex = (int)SendMessage(hWnd, LB_ADDSTRING, 0, (LONG_PTR)(LPTSTR)szDesc)) != LB_ERR )

            SendMessage(hWnd, LB_SETITEMDATA, iIndex, (LONG_PTR)pstrKey);

    }

    if (iLine == UNLIST_LINE)
    {
        //
        // Add the "Install unlisted..." choice to the top of the list
        // box.
        LoadString(myInstance, IDS_UPDATED, szDesc, sizeof(szDesc)/sizeof(TCHAR));
        if ((iIndex = (int)(LONG)SendMessage(hWnd, LB_INSERTSTRING, 0, (LPARAM)(LPTSTR)szDesc)) != LB_ERR)
            SendMessage(hWnd, LB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)0);
     }
     if (bInitd)

         SendMessage(hWnd, LB_SETCURSEL, 0, 0L );


     SendMessage(hWnd,WM_SETREDRAW, TRUE, 0L);
     return(bInitd);
}


/*
 *  DLG: LB_AVAILABLE
 *
 *  RemoveAvailable()
 *
 *  Remove all drivers from the listbox and free all storage associated with
 *  the keyname
 */

void RemoveAvailable(HWND hWnd)
{
    int iIndex;
    HWND hWndA;
    LPTSTR pstrKey;

    hWndA = GetDlgItem(hWnd, LB_AVAILABLE);
    iIndex = (int)SendMessage(hWndA, LB_GETCOUNT, 0, 0L);
    while ( iIndex-- > 0)
    {
        if (( (pstrKey = (LPTSTR)SendMessage(hWndA, LB_GETITEMDATA, iIndex,
            0L)) != (LPTSTR)LB_ERR ) && pstrKey)
            LocalFree((HLOCAL)pstrKey);
    }
}


/*
 *  DLG: LB_AVAILABLE
 *
 *  AvailableDriversDlg()
 *
 *  List the available installable drivers or return FALSE if there are none.
 */

const static DWORD aAvailDlgHelpIds[] = {  // Context Help IDs
    LB_AVAILABLE,    IDH_ADD_DRIVER_LIST,
    ID_DRVSTRING,    IDH_ADD_DRIVER_LIST,

    0, 0
};

INT_PTR AvailableDriversDlg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPTSTR    pstrKey;    //-jyg- added

    HWND    hWndA;
    int     iIndex;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            ShowWindow(hWnd, TRUE);
            wsStartWait();
            if (pinfOldDefault)
            {
                infSetDefault(pinfOldDefault);
                pinfOldDefault = NULL;
            }

            if ( !InitAvailable(hWndA = GetDlgItem(hWnd, LB_AVAILABLE), UNLIST_LINE))
            {
               /*
                *  We weren't able to find the [installable.drivers] section
                *  of the
                *  mmdriver.inf OR it was corrupt.  Go ahead and query the
                *  user to find an oemsetup.inf to make our default.  This
                *  is a bad state.
                */
                EndDialog(hWnd, FALSE);
                bFindOEM = TRUE;
                wcscpy(szDrv, szOemInf);
                if (DialogBox(myInstance, MAKEINTRESOURCE(DLG_INSERTDISK),
                        hWnd,  AddDriversDlg) == TRUE)
                    PostMessage(hWnd, WM_INITDIALOG, 0, 0L);
                else
                    pinfOldDefault = infSetDefault(pinfOldDefault);

                bFindOEM = FALSE;
            }
            wsEndWait();
            break;

        case WM_COMMAND:

            switch ( LOWORD(wParam ))
            {
                case LB_AVAILABLE:

                    // Hm... We've picked it.

                    if ( HIWORD(wParam) == LBN_DBLCLK )
                        SendMessage(hWnd, WM_COMMAND, IDOK, 0L);
                    break;

                case IDOK:

                   /*
                    *  We've made our selection
                    */

                    hWndA = GetDlgItem(hWnd, LB_AVAILABLE);

                    if ( (iIndex = (int)SendMessage(hWndA, LB_GETCURSEL, 0, 0L)) != LB_ERR)
                    {
                        if (!iIndex)
                        {
                           /*
                            *  The first entry is for OEMs
                            */

                            INT_PTR iFound;
                            bBadOemSetup = FALSE;

                            bCopyEvenIfOlder = TRUE;
                            bFindOEM = TRUE;
                            hMesgBoxParent = hWnd;
                            while ((iFound = DialogBox(myInstance,
                                    MAKEINTRESOURCE(DLG_INSERTDISK), hWnd,
                                            AddDriversDlg)) == 2);
                            if (iFound == 1)
                            {
                                    RemoveAvailable(hWnd);
                                    SendDlgItemMessage(hWnd, LB_AVAILABLE,
                                            LB_RESETCONTENT, 0, 0L);
                                    PostMessage(hWnd, WM_INITDIALOG, 0, 0L);
                            }
                            bFindOEM = FALSE;
                        }
                        else
                        {
                           /*
                            *  The user selected an entry from our .inf
                            */

                            wsStartWait();

                           /*
                            *  The  data associated with the list item is
                            *  the driver key name (field 0 in the inf file).
                            */

                            pstrKey = (LPTSTR)SendMessage(hWndA, LB_GETITEMDATA, iIndex, 0L);
                            bCopyingRelated = FALSE;
                            bQueryExist = TRUE;

                            if (InstallDrivers(hWndMain, hWnd, pstrKey))
                            {
                               RefreshAdvDlgTree ();
                               wsEndWait();


                                SetRunOnceSchemeInit ();

                              /*
                               *  If bRestart is true then the system must
                               *  be restarted to activate these changes
                               */

                               if (bRestart)
                               {
                                  iRestartMessage= IDS_RESTART_ADD;
                                  DialogBox(myInstance,
                                          MAKEINTRESOURCE(DLG_RESTART), hWnd,
                                              RestartDlg);
                               }
                            }
                            else
                               wsEndWait();

                            bRestart = FALSE;
                            bRelated = FALSE;
                        }
                    }
                    EndDialog(hWnd, FALSE);
                    break;

                case IDCANCEL:
                    EndDialog(hWnd, FALSE);
                    break;

                default:
                    return(FALSE);
            }
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, gszWindowsHlp, HELP_CONTEXTMENU,
                  (UINT_PTR)(LPTSTR)aAvailDlgHelpIds);
            break;

        case WM_HELP:
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, gszWindowsHlp,
                  HELP_WM_HELP, (UINT_PTR)(LPTSTR)aAvailDlgHelpIds);
            break;

        case WM_DESTROY:
            //
            // free the strings added as DATAITEM to the avail list

            RemoveAvailable(hWnd);
            return(FALSE);

        default:
            return FALSE;
         break;
    }
    return(TRUE);
}


BOOL DriversDllInitialize( IN PVOID hInstance
                  , IN DWORD ulReason
                  , IN PCONTEXT pctx OPTIONAL
                  )
{
    if (ulReason != DLL_PROCESS_ATTACH)
        return TRUE;

    myInstance = hInstance;
    LoadString(myInstance, IDS_CLOSE,  aszClose, sizeof(aszClose)/sizeof(TCHAR));
    LoadString(myInstance, IDS_DRIVERDESC, szDriversDesc, sizeof(szDriversDesc)/sizeof(TCHAR));
    LoadString(myInstance, IDS_FILE_ERROR, szFileError, sizeof(szFileError)/sizeof(TCHAR));
    LoadString(myInstance, IDS_INSTALLDRIVERS, szMDrivers, sizeof(szMDrivers)/sizeof(TCHAR));
    LoadString(myInstance, IDS_INSTALLDRIVERS32, szMDrivers32, sizeof(szMDrivers)/sizeof(TCHAR));
    LoadString(myInstance, IDS_RELATEDDESC, szRelatedDesc, sizeof(szRelatedDesc)/sizeof(TCHAR));
    LoadString(myInstance, IDS_USERINSTALLDRIVERS, szUserDrivers, sizeof(szUserDrivers)/sizeof(TCHAR));
    LoadString(myInstance, IDS_UNLISTED, (LPTSTR)szUnlisted, sizeof(szUnlisted)/sizeof(TCHAR));
    LoadString(myInstance, IDS_KNOWN, szKnown, sizeof(szKnown)/sizeof(TCHAR));
    LoadString(myInstance, IDS_OEMSETUP, szOemInf, sizeof(szOemInf)/sizeof(TCHAR));
    LoadString(myInstance, IDS_SYSTEM, szSystem, sizeof(szSystem)/sizeof(TCHAR));
    LoadString(myInstance, IDS_OUT_OF_REMOVE_SPACE, szOutOfRemoveSpace, sizeof(szOutOfRemoveSpace)/sizeof(TCHAR));
    LoadString(myInstance, IDS_NO_DESCRIPTION, szNoDesc, sizeof(szNoDesc)/sizeof(TCHAR));
    LoadString(myInstance, IDS_ERRORBOX, szError, sizeof(szError)/sizeof(TCHAR));
    LoadString(myInstance, IDS_REMOVEORNOT, szRemoveOrNot, sizeof(szRemoveOrNot)/sizeof(TCHAR));
    LoadString(myInstance, IDS_REMOVEORNOTSTRICT, szRemoveOrNotStrict, sizeof(szRemoveOrNotStrict)/sizeof(TCHAR));
    LoadString(myInstance, IDS_SETUPINF, szSetupInf, sizeof(szSetupInf)/sizeof(TCHAR));
    LoadString(myInstance, IDS_APPNAME, szAppName, sizeof(szAppName)/sizeof(TCHAR));

    LoadString(myInstance, IDS_DRIVERS, szDrivers, sizeof(szDrivers)/sizeof(TCHAR));
    LoadString(myInstance, IDS_REMOVE, szRemove, sizeof(szRemove)/sizeof(TCHAR));
    LoadString(myInstance, IDS_CONTROLINI, szControlIni, sizeof(szControlIni)/sizeof(TCHAR));
    LoadString(myInstance, IDS_SYSINI, szSysIni, sizeof(szSysIni)/sizeof(TCHAR));
    LoadString(myInstance, IDS_MCI, szMCI, sizeof(szMCI)/sizeof(TCHAR));
    LoadString(myInstance, IDS_DEFDRIVE, szDirOfSrc, sizeof(szDirOfSrc)/sizeof(TCHAR));
    LoadString(myInstance, IDS_CONTROL_HLP_FILE, szDriversHlp, sizeof(szDriversHlp)/sizeof(TCHAR));
    LoadString(myInstance, IDS_LASTQUERY, szLastQuery, sizeof(szLastQuery)/sizeof(TCHAR));

    return TRUE;
}

void DeleteCPLCache(void)
{
    HKEY hKeyCache;

    if (ERROR_SUCCESS ==
        RegOpenKey(HKEY_CURRENT_USER,
                   TEXT("Control Panel\\Cache\\multimed.cpl"),
                   &hKeyCache)) {
        for ( ; ; ) {
            TCHAR Name[50];

            if (ERROR_SUCCESS ==
                RegEnumKey(hKeyCache,
                           0,
                           Name,
                           sizeof(Name)/sizeof(TCHAR))) {
                HKEY hSubKey;

                RegDeleteKey(hKeyCache, Name);
            } else {
                break;    // leave loop
            }
        }

        RegDeleteKey(hKeyCache, NULL);
        RegCloseKey(hKeyCache);
    }
}


/*
** RestartDlg()
**
** Offer user the choice to (not) restart windows.
*/
INT_PTR RestartDlg(HWND hDlg, unsigned uiMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uiMessage)
    {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
               case IDCANCEL:
                    //
                    // don't restart windows
                    //
                    EndDialog(hDlg, FALSE);
                    break;

                case IDOK:
                    //
                    // do restart windows, *dont* dismiss dialog incase
                    // the user canceled it.
                    //
                    ReBoot(hDlg);
                    SetActiveWindow(hDlg);
                    //EndDialog(hDlg, TRUE);
                    break;

                default:
                    return FALSE;
            }
            return TRUE;

        case WM_INITDIALOG:
              /*
              **  Delete the control panel's cache so it will get it
              **  right!
              */

              DeleteCPLCache();


              if (iRestartMessage)
              {
                TCHAR szMesg1[300];
                TCHAR szMesg2[300];

                LoadString(myInstance, iRestartMessage, szMesg1, sizeof(szMesg1)/sizeof(TCHAR));
                wsprintf(szMesg2, szMesg1, (LPTSTR)szRestartDrv);
                SetDlgItemText(hDlg, IDS_RESTARTTEXT, (LPTSTR)szMesg2);

                if (iRestartMessage == IDS_RESTART_NOSOUND)
                {
                  PostMessage (hDlg, WM_NEXTDLGCTL,
                               (WPARAM)GetDlgItem(hDlg,IDOK), (LPARAM)TRUE);
                }
              }
              return TRUE;

        case WM_KEYUP:
            if (wParam == VK_F3)
                //
                // don't restart windows
                //
                EndDialog(hDlg, FALSE);
            break;

        default:
            break;
    }
    return FALSE;
}

/*
 * UserInstalled()
 *
 *
 */

BOOL UserInstalled(LPTSTR szKey)
{
        TCHAR buf[MAXSTR];

        if (*GetProfile(szUserDrivers, (LPTSTR)szKey, szControlIni, buf, sizeof(buf)) != TEXT('\0'))
            return(TRUE);
        else
            return(FALSE);
}

/*
 *   AddUnlistedDlg()
 *
 *   The following function processes requests by the user to install unlisted
 *   or updated drivers.
 *
 *   PARAMETERS:  The normal Dialog box parameters
 *   RETURN VALUE:  The usual Dialog box return value
 */

INT_PTR AddUnlistedDlg(HWND hDlg, unsigned nMsg, WPARAM wParam, LPARAM lParam)
{
  switch (nMsg)
  {
      case WM_INITDIALOG:
      {
          HWND hListDrivers;
          BOOL bFoundDrivers;

          wsStartWait();
          hListDrivers = GetDlgItem(hDlg, LB_UNLISTED);

          /* Search for drivers */
          bFoundDrivers = InitAvailable(hListDrivers, NO_UNLIST_LINE);
          if (!bFoundDrivers)
          {
                //
                // We weren't able to find the MMDRIVERS section of the
                // setup.inf OR it was corrupt.  Go ahead and query the
                // user to find an oemsetup.inf to make our default.  This
                // is a bad state.
                //

                INT_PTR iFound;

                bFindOEM = TRUE;
                bBadOemSetup = TRUE;
                while ((iFound = DialogBox(myInstance,
                        MAKEINTRESOURCE(DLG_INSERTDISK), hMesgBoxParent,
                                AddDriversDlg)) == 2);
                bFindOEM = FALSE;
                if (iFound == 1)
                {
                        SendDlgItemMessage(hDlg, LB_AVAILABLE,
                                LB_RESETCONTENT, 0, 0L);
                        PostMessage(hDlg, WM_INITDIALOG, 0, 0L);
                }
                EndDialog(hDlg, FALSE);
          }
          SendMessage(hListDrivers, LB_SETCURSEL, 0, 0L);
          wsEndWait();

          break;
        }

      case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDH_DLG_ADD_UNKNOWN:
              goto DoHelp;

            case LB_UNLISTED:
              if (HIWORD(wParam) != LBN_DBLCLK)
                  break;

              // else Fall through here
            case IDOK:
            {
             HWND hWndA;
             int iIndex;
             LPTSTR pstrKey;

             hWndA = GetDlgItem(hDlg, LB_UNLISTED);
             if ( (iIndex = (int)SendMessage(hWndA, LB_GETCURSEL, 0, 0L))
                                                             != LB_ERR)
             {
                wsStartWait();
                pstrKey = (LPTSTR)SendMessage(hWndA, LB_GETITEMDATA, iIndex, 0L);
                bCopyingRelated = FALSE;
                bQueryExist = TRUE;
                if (InstallDrivers(hWndMain, hDlg, pstrKey))
                {
                   RefreshAdvDlgTree ();
                   wsEndWait();

                   SetRunOnceSchemeInit ();

                   if (bRestart)
                   {
                      iRestartMessage= IDS_RESTART_ADD;
                      DialogBox(myInstance,   MAKEINTRESOURCE(DLG_RESTART),
                                                      hDlg, RestartDlg);
                   }
                 }
                 else
                   wsEndWait();
                 bRelated = FALSE;
                 bRestart = FALSE;
              }
              EndDialog(hDlg, FALSE);
            }
            break;

            case IDCANCEL:
              EndDialog(hDlg, wParam);
              break;

            default:
              return FALSE;
          }
        break;

      case WM_HELP:
DoHelp:
        WinHelp (hDlg, gszWindowsHlp, HELP_CONTEXT, IDH_MMCPL_DEVPROP_ENABLE);
        break;

      default:
        return FALSE;
   }
   return TRUE;
}
/*
 *  ReBoot()
 *
 *  Restart the system.  If this fails we put up a message box
 */

 void ReBoot(HWND hDlg)
 {
     DWORD Error;
     BOOLEAN WasEnabled;

    /*
     *  We must adjust our privilege level to be allowed to restart the
     *  system
     */

     RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                         TRUE,
                         FALSE,
                         &WasEnabled
                       );
    /*
     *  Try to reboot the system
     */

     if (!ExitWindowsEx(EWX_REBOOT, 0xFFFFFFFF)) {

         Error = GetLastError();

        /*
         *  Put up a message box if we failed
         */

         if (Error != NO_ERROR) {
            TCHAR szCantRestart[80];
            LoadString(myInstance,
                       Error == ERROR_PRIVILEGE_NOT_HELD  ||
                       Error == ERROR_NOT_ALL_ASSIGNED  ||
                       Error == ERROR_ACCESS_DENIED ?
                           IDS_CANNOT_RESTART_PRIVILEGE :
                           IDS_CANNOT_RESTART_UNKNOWN,
                       szCantRestart,
                       sizeof(szCantRestart)/sizeof(TCHAR));

            MessageBox(hDlg, szCantRestart, szError,
                       MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
         }
     }
 }


void OpenDriverError(HWND hDlg, LPTSTR szDriver, LPTSTR szFile)
{
        TCHAR szMesg[MAXSTR];
        TCHAR szMesg2[MAXSTR];

        LoadString(myInstance, IDS_INSTALLING_DRIVERS, szMesg, sizeof(szMesg)/sizeof(TCHAR));
        wsprintf(szMesg2, szMesg, szDriver, szFile);
        MessageBox(hDlg, szMesg2, szError, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);

}


/*
 *** AddIDriver - Adds a treeitem referencing the given PIDRIVER
 *
 * Note that the listed PIDRIVER should already have been added to the
 * aInstalledDrivers array (via AddIDriverToArray()) before calling this
 * routine.
 *
 */

HTREEITEM AddIDriver (HWND hTree, PIDRIVER pIDriver, DriverClass dc)
{
   short           idr;
   TV_INSERTSTRUCT ti;
   HTREEITEM       hti;
   short           ii;
   TCHAR           szFile[ _MAX_FNAME +1 +_MAX_EXT +1 ];
   TCHAR           szExt[ _MAX_EXT +1 ];
   TCHAR           szDesc[ cchRESOURCE ];

            // don't add an entry for one of the to-be-skipped drivers
            //
   lsplitpath (pIDriver->szFile, NULL, NULL, szFile, szExt);

   if (szExt[0] != TEXT('\0'))
      lstrcat (szFile, szExt);

    //check to see if we're trying to put a PNP driver into the legacy tree
    if (g_dcFilterClass == dcLEGACY)
    {
        if ((dc == dcWAVE) ||
            (dc == dcMIDI) ||
            (dc == dcMIXER) ||
            (dc == dcAUX))
        {
            if (IsPnPDriver(szFile))
            {
                return FALSE;
            }
        }
    }

   if (dc != dcMIDI)
      {
      for (ii = 0; ii < nDriversToSKIP; ii++)
         {
         if (!lstrcmpi (szFile, aDriversToSKIP[ ii ]))
            return FALSE;
         }
      }

            // If we were given a DriverClass, then the caller has
            // specified where we should create an entry--add the "Audio for"
            // (etc) tag before the description, and add it.
            //
            // Otherwise, determine where this driver belongs in the tree
            //
   if (dc != dcINVALID)
      {
      TCHAR   szTag[ cchRESOURCE ];

      switch (dc)
         {
         case dcWAVE:   GetString (szTag, IDS_AUDIOFOR);
                       break;
         case dcMIDI:   GetString (szTag, IDS_MIDIFOR);
                       break;
         case dcMIXER:  GetString (szTag, IDS_MIXERFOR);
                       break;
         case dcAUX:    GetString (szTag, IDS_AUXFOR);
                       break;
         default:       lstrcpy (szTag, TEXT("%s"));
                       break;
         }

      wsprintf (szDesc, szTag, pIDriver->szDesc);
      }
   else
      {
      if ((dc = GuessDriverClass (pIDriver)) == dcINVALID)
         return FALSE;

      lstrcpy (szDesc, pIDriver->szDesc);
      }

            // map that classification into an index within the
            // root entries of the tree (aDriverRoot[])
            //
   if ((idr = DriverClassToRootIndex (dc)) == -1)
      return FALSE;

            // if this driver already has an entry under this DriverClass,
            // then don't add another.
            //
   for (ii =0; ii < cInstalledDrivers; ++ii)
      {
      if (aInstalledDrivers[ ii ].pIDriver == pIDriver)
         break;
      }
   if (ii >= cInstalledDrivers)
      {
      ii = (short)NOPIDRIVER;
      }
   else if (aInstalledDrivers[ ii ].dwBits & aDriverRoot[ idr ].dwBit)
      {
      return FALSE; // Already have an entry here!
      }

            // since not all roots need exist all the time, make sure
            // this classification HAS a root in the tree
            //
   if (!EnsureRootIndexExists (hTree, idr))
      return FALSE;

            // finally, insert an item into the tree for this driver
            // note that for audio codecs to be sorted properly, they must
            // be added via this routine in their appropriate order--ie,
            // call this routine for the highest-priority codec first.
            //
   ti.hParent = aDriverRoot[ idr ].hti;
   ti.hInsertAfter = (dc == dcACODEC) ? TVI_LAST : TVI_SORT;
   ti.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
   ti.item.iImage = (int)idr;
   ti.item.iSelectedImage = (int)idr;
   ti.item.pszText = szDesc;
   ti.item.lParam = ii;

   if ((hti = TreeView_InsertItem (hTree, &ti)) == NULL)
      return FALSE;

   if (ii != NOPIDRIVER)
      aInstalledDrivers[ ii ].dwBits |= aDriverRoot[ idr ].dwBit;

   return hti;
}


BOOL AddIDriverByName (HWND hTree, LPCWSTR wszFile, DriverClass dc)
{
   LPTSTR pch;
   TCHAR tszFile[ cchRESOURCE ];
   PIDRIVER pid;

#ifdef UNICODE
   lstrcpy (tszFile, wszFile);
#else
   wcstombs (tszFile, wszFile, cchRESOURCE);
#endif

            // Strip off any trailing whitespace
            //
   if (tszFile[0] == TEXT('\0'))
      return FALSE;

   for (pch = &tszFile[ lstrlen(tszFile)-1 ];
        pch >= tszFile && (*pch == TEXT('\t') || *pch == TEXT(' '));
        --pch)
      ;
   *(1+pch) = TEXT('\0');

            // If this is MMDRV.DLL, then it's possibly providing the
            // user-mode component for kernel-mode drivers.  Since it's
            // apparently impossible to determine the name of the .SYS
            // file which is providing a "\\.\WaveIn0" device (etc),
            // we'll use a hack: Check around for anyone registered
            // under the alias "Kernel", and use that.
            //
   if (!lstrcmpi (tszFile, cszMMDRVDLL))
      {
      mysize_t ii;
      for (ii = 0; ii < cInstalledDrivers; ++ii)
         {
         if (aInstalledDrivers[ ii ].pIDriver == NULL)
            continue;
         if (!lstrnicmp (aInstalledDrivers[ ii ].pIDriver->szAlias,
                         cszAliasKERNEL,
                         lstrlen(cszAliasKERNEL)))
            {
            lstrcpy (tszFile, aInstalledDrivers[ ii ].pIDriver->szFile);
            break;
            }
         }
      if (ii >= cInstalledDrivers)
         return FALSE;
      }

            // Find the driver in the aInstalledDriver array, and add
            // an entry for it in the tree.
            //
   if ((pid = FindIDriverByName (tszFile)) == NULL)
      return FALSE;

   if (AddIDriver (hTree, pid, dc) == NULL)
      return FALSE;

   return TRUE;
}


/*
 *** RemoveIDriver - Removes (and optionally frees) an IDRIVER from hAdvDlgTree
 *
 */

void RemoveIDriver (HWND hTree, PIDRIVER pIDriver, BOOL fFreeToo)
{
   mysize_t   ii;
   short      idr;
   HTREEITEM  hti;

            // Find each TreeItem which references this entry.
            //
   for (idr = 0; idr < nDriverROOTS; idr++)
      {
      if ((hti = aDriverRoot[ idr ].hti) == NULL)
         continue;

      hti = TreeView_GetChild (hTree, hti);
      while (hti != NULL)
         {
         if (pIDriver != FindIDriverByTreeItem (hti))
            {
            hti = TreeView_GetNextSibling (hTree, hti);
            continue;
            }

         // We found a tree item which uses this driver, so delete the
         // item.  Also note that this may cause the driver's parent
         // node to no longer be necessary.
         //

         TreeView_DeleteItem (hTree, hti);
         hti = TreeView_GetChild (hTree, aDriverRoot[ idr ].hti);

         if (!aDriverRoot[ idr ].fAlwaysMake)   // may no longer need parent?
            {
            if (hti == NULL)                    // parent now has no children?
               {
               TreeView_DeleteItem (hTree, aDriverRoot[ idr ].hti);
               aDriverRoot[ idr ].hti = NULL;
               }
            }
         }
      }

            // See if we can find the given pIDriver within the
            // aInstalledDriver array.
            //
   for (ii = 0; ii < cInstalledDrivers; ++ii)
      {
      if (aInstalledDrivers[ ii ].pIDriver == pIDriver)
         {
         aInstalledDrivers[ ii ].dwBits = 0L;   // no longer in tree at all

         if (fFreeToo)
            {
            LocalFree ((HANDLE)aInstalledDrivers[ ii ].pIDriver);
            aInstalledDrivers[ ii ].pIDriver = NULL;
            }

         break;   // There's only one entry in this array for each pIDriver
         }
      }
}


#ifdef FIX_BUG_15451
HTREEITEM FindTreeItemByDriverName (LPTSTR pszName)
{
   PIDRIVER   pid;
   short      idr;
   HTREEITEM  hti;

   if ((pid = FindIDriverByName (pszName)) == NULL)
      return (HTREEITEM)0;

   for (idr = 0; idr < nDriverROOTS; idr++)
      {
      if ((hti = aDriverRoot[ idr ].hti) == NULL)
         continue;

      for (hti = TreeView_GetChild (hAdvDlgTree, hti);
           hti != NULL;
           hti = TreeView_GetNextSibling (hAdvDlgTree, hti))
         {
         if (pid == FindIDriverByTreeItem (hti))
            {
            return hti;
            }
         }
     }

   return (HTREEITEM)0;
}
#endif // FIX_BUG_15451


PIDRIVER FindIDriverByTreeItem (HTREEITEM hti)
{
   TV_ITEM tvi;

   tvi.mask = TVIF_PARAM;
   tvi.hItem = hti;
   TreeView_GetItem (hAdvDlgTree, &tvi);

   if ( (tvi.lParam < 0) ||
        (tvi.lParam >= cInstalledDrivers) )
      {
      return NULL;
      }

   return aInstalledDrivers[ tvi.lParam ].pIDriver;
}


/*
 *** FindIDriverByName - Returns the first found IDRIVER structure with a name
 *
 */

PIDRIVER FindIDriverByName (LPTSTR szFile)
{
   mysize_t ii;

   for (ii = 0; ii < cInstalledDrivers; ++ii)
      {
      if (aInstalledDrivers[ ii ].pIDriver == NULL)
         continue;
      if (aInstalledDrivers[ ii ].pIDriver->szAlias[0] == TEXT('\0'))
         continue;

      if (!FileNameCmp (aInstalledDrivers[ ii ].pIDriver->szFile, szFile))
         return aInstalledDrivers[ ii ].pIDriver;
      }

    return NULL;
}


PIDRIVER FindIDriverByResource (PIRESOURCE pir)
{
   return FindIDriverByName (FileName( pir->szFile ));
}


/*
 *** GetSelectedIDriver - Returns the IDRIVER structure the user has selected
 *
 */

PIDRIVER GetSelectedIDriver (HWND hTree)
{
   HTREEITEM  htiCur = TreeView_GetSelection (hTree);

   if (htiCur == NULL)
      return NULL;

   return FindIDriverByTreeItem (htiCur);
}


/*
 *** DriverClassToRootIndex - obtain idr for which {aDriverRoot[idr].dc == dc}
 *
 * The array index for aDriverRoot[] is NOT a DriverClass--that is,
 * aDriverRoot[ PickAnyDC ].dc is not necessarily equal to PickAnyDC.
 * Given a DC, this routine finds the index into aDriverRoot which references
 * that DC.
 *
 */

short DriverClassToRootIndex (DriverClass dc)
{
   short  idr;

   for (idr = 0; idr < nDriverROOTS; idr++)
      {
      if (aDriverRoot[ idr ].dc == dc)
         return idr;
      }

   return -1;
}


/*
 *** GetDriverClass - guess a DriverClass based on an IDRIVER structure
 *
 * The registry has several different aliases which it uses in the LHS
 * of HKLM\software\microsoft\windowsnt\drivers,drivers32,etc to indicate
 * the classification of a particular driver.  These include:
 *
 * AUX, MIDI, MIDIMAPPER, MIXER, MSACM.*, VIDC.* WAVE, WAVEMAPPER
 *
 * as well as others--the full array of known entries is tracked within
 * aDriverKeywords[].  In addition, any of these may be followed by a
 * string of digits by which they are distinguished.  This routine parses
 * these keywords and returns a corresponding DriverClass enum.
 *
 */

DriverClass GuessDriverClass (PIDRIVER pid)
{
#ifdef FIX_BUG_15451
   return GuessDriverClassFromAlias (pid->szAlias);
}



DriverClass GuessDriverClassFromAlias (LPTSTR pszAlias)
{
#endif // FIX_BUG_15451
   TCHAR   szAlias[ cchRESOURCE ];
   TCHAR  *pch;
   short   ii;

#ifdef FIX_BUG_15451
   lstrcpy (szAlias, pszAlias); // Make a local copy so we can munge it
#else // FIX_BUG_15451
   lstrcpy (szAlias, pid->szAlias); // Make a local copy so we can munge it
#endif // FIX_BUG_15451

   if ((pch = lstrchr (szAlias, TEXT('.'))) != NULL)
      *pch = TEXT('0');

   for (ii = 0; ii < nDriverKEYWORDS; ii++)
      {
      if (!lstrnicmp (szAlias,
                      aDriverKeyword[ii].psz,
                      lstrlen (aDriverKeyword[ii].psz)))
         {
         return aDriverKeyword[ii].dc;
         }
      }

   return dcOTHER;
}


DriverClass GuessDriverClassFromTreeItem (HTREEITEM hti)
{
   short  idr;

   for (idr = 0; idr < nDriverROOTS; idr++)
      {
      if (hti == aDriverRoot[idr].hti)
         return aDriverRoot[idr].dc;
      }

   return (g_dcFilterClass);
}



/*
 *** EnsureRootIndexExists - makes sure a given parent exists in hAdvDlgTree
 *
 */

BOOL EnsureRootIndexExists (HWND hTree, short idr)
{
   TV_INSERTSTRUCT ti;
   TCHAR           szDesc[ cchRESOURCE ];
   HWND            hwndParent = NULL;
   HWND            hwndName   = NULL;

            // If we already HAVE a root in the tree, we're done.
            //
   if (aDriverRoot[ idr ].hti != NULL)
      return TRUE;

    if (g_dcFilterClass != dcINVALID)
    {
        if (g_dcFilterClass == dcLEGACY)
        {
            if ((aDriverRoot[ idr ].dc != dcWAVE) &&
                (aDriverRoot[ idr ].dc != dcMIDI) &&
                (aDriverRoot[ idr ].dc != dcMIXER) &&
                (aDriverRoot[ idr ].dc != dcAUX))
            {
                return FALSE;
            }
        }
        else if (aDriverRoot[ idr ].dc != g_dcFilterClass)
        {
            return FALSE;
        }
    }

   aDriverRoot[idr].hti = TVI_ROOT;
   
   LoadString (myInstance, aDriverRoot[idr].idDesc, szDesc, cchRESOURCE);

    if ((g_dcFilterClass == dcINVALID) || (g_dcFilterClass == dcLEGACY))
    {
       ti.hInsertAfter = TVI_LAST;
       ti.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
       ti.item.iImage = idr;
       ti.item.iSelectedImage = idr;
       ti.item.pszText = szDesc;
       ti.item.lParam = NOPIDRIVER;

       if (aDriverRoot[idr].dc == dcINVALID)
          ti.hParent = TVI_ROOT;
       else
          ti.hParent = AdvDlgFindTopLevel ();

       if ((aDriverRoot[idr].hti = TreeView_InsertItem (hTree, &ti)) == NULL)
          return FALSE;
    }

    if (g_dcFilterClass != dcINVALID)
    {
       hwndParent = GetParent(hTree);
       if (hwndParent)
       {
            hwndName = GetDlgItem(hwndParent,IDC_DEVICECLASS);
            if (hwndName)
            {
                if (g_dcFilterClass == dcLEGACY)
                {
                    LoadString (myInstance, IDS_WAVE_HEADER, szDesc, cchRESOURCE);
                }

                SetWindowText(hwndName,szDesc);
            }
       }
    }

   return TRUE;
}


/*
 **** AdvDlgFindTopLevel - Finds the HTREEITEM associated with the tree root
 *
 * If there's a "Multimedia Devices" tree item under which the other roots
 * are collected, this will return that item.  Otherwise, it returns TVI_ROOT.
 *
 */

HTREEITEM AdvDlgFindTopLevel (void)
{
   short  idr;

   for (idr = 0; idr < nDriverROOTS; idr++)
      {
      if (aDriverRoot[idr].dc == dcINVALID)
         return aDriverRoot[idr].hti;
      }

   return TVI_ROOT;
}



/*
 **** InitAdvDlgTree - Prepares the AdvDlg's treeview to display devices
 *
 */

BOOL InitAdvDlgTree (HWND hTree)
{
   int   cxIcon, cyIcon;
   short idr;

   #ifdef UNICODE
   TreeView_SetUnicodeFormat(hTree,TRUE);
   #endif

    // Make sure we start with a clean slate
    //
   hAdvDlgTree = hTree;
   SendMessage (hTree, WM_SETREDRAW, FALSE, 0L);
   
   for (idr = 0; idr < nDriverROOTS; idr++)
   {
      aDriverRoot[ idr ].hti = NULL;
      aDriverRoot[ idr ].dwBit = ((DWORD)1) << idr;
   }

            // Create an imagelist for the icons in the treeview
            //
   cxIcon = (int)GetSystemMetrics (SM_CXSMICON);
   cyIcon = (int)GetSystemMetrics (SM_CYSMICON);

   if ((hImageList = ImageList_Create (cxIcon, cyIcon,
                                       TRUE, nDriverROOTS, 1)) == NULL)
      return FALSE;

   for (idr = 0; idr < nDriverROOTS; idr++)
      {
      HICON hi = LoadImage (myInstance,
                            MAKEINTRESOURCE( aDriverRoot[idr].idIcon ),
                            IMAGE_ICON, cxIcon, cyIcon, LR_DEFAULTCOLOR);
      ImageList_AddIcon (hImageList, hi);
      }

   TreeView_SetImageList (hTree, hImageList, TVSIL_NORMAL);


    if (g_dcFilterClass == dcINVALID)
    {
            // Create the root nodes that are supposed to exist
            // even without children (note that not all are)
            //
   for (idr = 0; idr < nDriverROOTS; idr++)
      {
      if (aDriverRoot[ idr ].dc == dcINVALID)
         {
         if (!EnsureRootIndexExists (hTree, idr))
            return FALSE;
         }
      }
   for (idr = 0; idr < nDriverROOTS; idr++)
      {
      if (aDriverRoot[ idr ].dc != dcINVALID && aDriverRoot[ idr ].fAlwaysMake)
         {
         if (!EnsureRootIndexExists (hTree, idr))
            return FALSE;
         }
      }
    }
            // Expand the tree somewhat, so the user doesn't get
            // greeted by a blank page
            //
   TreeView_Expand (hTree, AdvDlgFindTopLevel(), TVE_EXPAND);

   SendMessage (hTree, WM_SETREDRAW, TRUE, 0L);
   return TRUE;
}


/*
 **** FreeAdvDlgTree - Removes and frees all items within the AdvDlg treeview
 *
 */

void FreeAdvDlgTree (HWND hTree)
{
   short  idr;

            // Delete all leaf nodes
            //
   for (idr = 0; idr < nDriverROOTS; idr++)
      {
      HTREEITEM  hti;

      if (aDriverRoot[idr].dc == dcINVALID)
         continue;
      if (aDriverRoot[idr].hti == NULL)
         continue;

      while ((hti = TreeView_GetChild (hTree, aDriverRoot[idr].hti)) != NULL)
         {
         if (aDriverRoot[ idr ].dc == dcMIDI)
            {
            HTREEITEM  htiInstrument;

            while ((htiInstrument = TreeView_GetChild (hTree, hti)) != NULL)
               {
               TV_ITEM tvi;
               tvi.mask = TVIF_PARAM;
               tvi.hItem = htiInstrument;
               tvi.lParam = 0;

               TreeView_GetItem(hTree, &tvi);

               if (tvi.lParam != 0)
                  LocalFree ((HANDLE)tvi.lParam);

               TreeView_DeleteItem (hTree, htiInstrument);
               }
            }

         TreeView_DeleteItem (hTree, hti);
         }
      }

            // Delete everything else
            //
   TreeView_DeleteAllItems (hTree);

            // Delete the tree's image list
            //
   if (hImageList)
      {
      TreeView_SetImageList (hTree, NULL, TVSIL_NORMAL);
      ImageList_Destroy (hImageList);
      hImageList = NULL;
      }

            // Delete the InstalledDrivers array
            //
   if (aInstalledDrivers != NULL)
      {
      mysize_t  ii;
      for (ii = 0; ii < cInstalledDrivers; ++ii)
         {
         if (aInstalledDrivers[ ii ].pIDriver != NULL)
            {
            LocalFree ((HANDLE)aInstalledDrivers[ ii ].pIDriver);
            aInstalledDrivers[ ii ].pIDriver = NULL;
            }
         }

      GlobalFree ((HGLOBAL)aInstalledDrivers);
      aInstalledDrivers = NULL;
      cInstalledDrivers = 0;
      }
}


int lstrnicmp (LPTSTR pszA, LPTSTR pszB, size_t cch)
{
#ifdef UNICODE
   size_t  cchA, cchB;
   TCHAR  *pch;

   for (cchA = 1, pch = pszA; cchA < cch; cchA++, pch++)
      {
      if (*pch == TEXT('\0'))
         break;
      }
   for (cchB = 1, pch = pszB; cchB < cch; cchB++, pch++)
      {
      if (*pch == TEXT('\0'))
         break;
      }

   return (CompareStringW (GetThreadLocale(), NORM_IGNORECASE,
                           pszA, cchA, pszB, cchB)
          )-2;  // CompareStringW returns {1,2,3} instead of {-1,0,1}.
#else
   return _strnicmp (pszA, pszB, cch);
#endif
}


LPTSTR lstrchr (LPTSTR pszTarget, TCHAR ch)
{
   size_t ich;
   if (pszTarget == NULL)
      return NULL;
   for (ich = 0; pszTarget[ich] != TEXT('\0'); ich++)
      {
      if (pszTarget[ich] == ch)
         return &pszTarget[ ich ];
      }

   return NULL;
}


void lsplitpath (LPTSTR pszSource,
                 LPTSTR pszDrive, LPTSTR pszPath, LPTSTR pszName, LPTSTR pszExt)
{
   LPTSTR   pszLastSlash = NULL;
   LPTSTR   pszLastDot = NULL;
   LPTSTR   pch;
   size_t   cchCopy;

        /*
         * NOTE: This routine was snitched out of USERPRI.LIB 'cause the
         * one in there doesn't split the extension off the name properly.
         *
         * We assume that the path argument has the following form, where any
         * or all of the components may be missing.
         *
         *      <drive><dir><fname><ext>
         *
         * and each of the components has the following expected form(s)
         *
         *  drive:
         *      0 to _MAX_DRIVE-1 characters, the last of which, if any, is a
         *      ':'
         *  dir:
         *      0 to _MAX_DIR-1 characters in the form of an absolute path
         *      (leading '/' or '\') or relative path, the last of which, if
         *      any, must be a '/' or '\'.  E.g -
         *      absolute path:
         *          \top\next\last\     ; or
         *          /top/next/last/
         *      relative path:
         *          top\next\last\      ; or
         *          top/next/last/
         *      Mixed use of '/' and '\' within a path is also tolerated
         *  fname:
         *      0 to _MAX_FNAME-1 characters not including the '.' character
         *  ext:
         *      0 to _MAX_EXT-1 characters where, if any, the first must be a
         *      '.'
         *
         */

             // extract drive letter and :, if any
             //
   if (*(pszSource + _MAX_DRIVE - 2) == TEXT(':'))
      {
      if (pszDrive)
         {
         lstrncpy (pszDrive, pszSource, _MAX_DRIVE-1);
         pszDrive[ _MAX_DRIVE-1 ] = TEXT('\0');
         }
      pszSource += _MAX_DRIVE-1;
      }
    else if (pszDrive)
      {
      *pszDrive = TEXT('\0');
      }

          // extract path string, if any.  pszSource now points to the first
          // character of the path, if any, or the filename or extension, if
          // no path was specified.  Scan ahead for the last occurence, if
          // any, of a '/' or '\' path separator character.  If none is found,
          // there is no path.  We will also note the last '.' character found,
          // if any, to aid in handling the extension.
          //
   for (pch = pszSource; *pch != TEXT('\0'); pch++)
      {
      if (*pch == TEXT('/') || *pch == TEXT('\\'))
         pszLastSlash = pch;
      else if (*pch == TEXT('.'))
         pszLastDot = pch;
      }

          // if we found a '\\' or '/', fill in pszPath
          //
   if (pszLastSlash)
      {
      if (pszPath)
         {
         cchCopy = (size_t)min((UINT)_MAX_DIR-1, (pszLastSlash-pszSource) + 1);
         lstrncpy (pszPath, pszSource, cchCopy);
         pszPath[ cchCopy ] = 0;
         }
      pszSource = pszLastSlash +1;
      }
   else if (pszPath)
      {
      *pszPath = TEXT('\0');
      }

             // extract file name and extension, if any.  Path now points to
             // the first character of the file name, if any, or the extension
             // if no file name was given.  Dot points to the '.' beginning the
             // extension, if any.
             //

   if (pszLastDot && (pszLastDot >= pszSource))
      {
               // found the marker for an extension -
               // copy the file name up to the '.'.
               //
      if (pszName)
         {
         cchCopy = (size_t)min( (UINT)_MAX_DIR-1, (pszLastDot-pszSource) );
         lstrncpy (pszName, pszSource, cchCopy);
         pszName[ cchCopy ] = 0;
         }

               // now we can get the extension
               //
      if (pszExt)
         {
         lstrncpy (pszExt, pszLastDot, _MAX_EXT -1);
         pszExt[ _MAX_EXT-1 ] = TEXT('\0');
         }
      }
   else
      {
               // found no extension, give empty extension and copy rest of
               // string into fname.
               //
      if (pszName)
         {
         lstrncpy (pszName, pszSource, _MAX_FNAME -1);
         pszName[ _MAX_FNAME -1 ] = TEXT('\0');
         }

      if (pszExt)
         {
         *pszExt = TEXT('\0');
         }
      }

}

void lstrncpy (LPTSTR pszTarget, LPTSTR pszSource, size_t cch)
{
   size_t ich;
   for (ich = 0; ich < cch; ich++)
      {
      if ((pszTarget[ich] = pszSource[ich]) == TEXT('\0'))
         break;
      }
}



/*
 * DEVICE PROPERTY SHEETS _____________________________________________________
 *
 */

         // General flag macros
         //
#define SetFlag(obj, f)             do {obj |= (f);} while (0)
#define ToggleFlag(obj, f)          do {obj ^= (f);} while (0)
#define ClearFlag(obj, f)           do {obj &= ~(f);} while (0)
#define IsFlagSet(obj, f)           (BOOL)(((obj) & (f)) == (f))
#define IsFlagClear(obj, f)         (BOOL)(((obj) & (f)) != (f))

BOOL          InstrumentToResource      (PIRESOURCE, HTREEITEM);
BOOL          DriverToResource          (HWND,PIRESOURCE,PIDRIVER,DriverClass);
DriverClass   OldClassIDToDriverClass   (int);
void          FreeClassNode             (PCLASSNODE);
PIDRIVER      FindIDriverByResource     (PIRESOURCE);
void          EnableDriverService       (PIRESOURCE, BOOL);

BOOL PASCAL DoDevPropCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
STATIC void SetDevStatus(int iStatus, HWND hDlg);
INT_PTR CALLBACK ACMDlg(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);


/*
 *** DriverToResource - create a PIRESOURCE structure from a PIDRIVER structure
 *
 * The Win95 code uses PIRESOURCEs (among other structures) to keep track of
 * the item in the Advanced tab's treeview; the old WinNT code used PIDRIVER
 * structures to keep track of the items in its listbox.  The tree now uses
 * PIDRIVER structures, but we create PIRESOURCE structures out of 'em before
 * passing control off to the Win95 properties dialogs, which came through
 * largely unmodified.
 *
 * BTW, I retained the PIDRIVER structures as the main treeview structure
 * because it prevented difficulty in porting over the Install/Remove Driver
 * code from the old NT code.
 *
 */

BOOL DriverToResource (HWND hPar, PIRESOURCE pir, PIDRIVER pid, DriverClass dc)
{
   if (dc == dcINVALID)
      {
      if ((dc = GuessDriverClass (pid)) == dcINVALID)
         return FALSE;
      }

   if ((pir->pcn = (PCLASSNODE)LocalAlloc (LPTR, sizeof(CLASSNODE))) == NULL)
      return FALSE;

   if (!DriverClassToClassNode (pir->pcn, dc))
      {
      LocalFree ((HANDLE)pir->pcn);
      return FALSE;
      }

   if (dc == dcACODEC)
      pir->iNode = 3;   // 1=class, 2=device, 3=acm, 4=instmt
// else if (dc == dcINSTRUMENT)
//    pir->iNode = 4;   // 1=class, 2=device, 3=acm, 4=instmt
   else
      pir->iNode = 2;   // 1=class, 2=device, 3=acm, 4=instmt

   lstrcpy (pir->szFriendlyName, pid->szDesc);
   lstrcpy (pir->szDesc,         pid->szDesc);
   lstrcpy (pir->szFile,         pid->szFile);
   lstrcpy (pir->szDrvEntry,     pid->szAlias);
   lstrcpy (pir->szClass,        pir->pcn->szClass);

   pid->fQueryable = IsConfigurable (pid, hPar);
   pir->fQueryable = (short)pid->fQueryable;
   pir->iClassID = (short)DriverClassToOldClassID (dc);
   pir->szParam[0] = 0;
   pir->dnDevNode = 0;
   pir->hDriver = NULL;

            // Find fStatus, which despite its name is really a series of
            // flags--in Win95 it's composed of DEV_* flags (from the old
            // mmcpl.h), but those are tied with PNP.  Here, we use the
            // dwStatus* flags:
            //
   pir->fStatus = (int)GetDriverStatus (pid);

   return TRUE;
}


BOOL InstrumentToResource (PIRESOURCE pir, HTREEITEM hti)
{
   TV_ITEM tvi;
   PINSTRUM pin;

   tvi.mask = TVIF_PARAM;
   tvi.hItem = hti;
   tvi.lParam = 0;
   TreeView_GetItem(hAdvDlgTree, &tvi);

   if ((pin = (PINSTRUM)tvi.lParam) == NULL)
      return FALSE;


   if ((pir->pcn = (PCLASSNODE)LocalAlloc (LPTR, sizeof(CLASSNODE))) == NULL)
      return FALSE;

   if (!DriverClassToClassNode (pir->pcn, dcMIDI))
      {
      LocalFree ((HANDLE)pir->pcn);
      return FALSE;
      }

   pir->iNode = 4;  // 1=class, 2=device, 3=acm, 4=instmt

   lstrcpy (pir->szFriendlyName, pin->szFriendly);
   lstrcpy (pir->szDesc,         pin->szKey);
// lstrcpy (pir->szFile,         TEXT("unused"));
// lstrcpy (pir->szDrvEntry,     TEXT("unused"));
   lstrcpy (pir->szClass,        pir->pcn->szClass);

   pir->fQueryable = FALSE;
   pir->iClassID = MIDI_ID;
   pir->szParam[0] = 0;
   pir->dnDevNode = 0;
   pir->hDriver = NULL;
   pir->fStatus = 0;

   return TRUE;
}


BOOL DriverClassToClassNode (PCLASSNODE pcn, DriverClass dc)
{
   short  idr;
   short  ii;
   int    cxIcon, cyIcon;

   if ((idr = DriverClassToRootIndex (dc)) == -1)
      return FALSE;

   pcn->iNode = 1;  // 1=class, 2=device, 3=acm, 4=instmt

   GetString (pcn->szClassName, aDriverRoot[idr].idDesc);
   pcn->szClass[0] = TEXT('\0');

   for (ii = 0; ii < nKeywordDESCS; ii++)
      {
      if (aKeywordDesc[ii].dc == dc)
         {
         lstrcpy (pcn->szClass, aKeywordDesc[ii].psz);
         break;
         }
      }

   cxIcon = (int)GetSystemMetrics (SM_CXICON);
   cyIcon = (int)GetSystemMetrics (SM_CYICON);

   pcn->hIcon = LoadImage (myInstance,
                           MAKEINTRESOURCE( aDriverRoot[ idr ].idIcon ),
                           IMAGE_ICON, cxIcon, cyIcon, LR_DEFAULTCOLOR);

   return TRUE;
}


int DriverClassToOldClassID (DriverClass dc)
{
   switch (dc)
      {
      case dcWAVE:   return WAVE_ID;      break;
      case dcMIDI:   return MIDI_ID;      break;
      case dcMIXER:  return MIXER_ID;     break;
      case dcAUX:    return AUX_ID;       break;
      case dcMCI:    return MCI_ID;       break;
      case dcACODEC: return ACM_ID;       break;
      case dcVCODEC: return ICM_ID;       break;
      case dcVIDCAP: return VIDCAP_ID;    break;
      case dcJOY:    return JOYSTICK_ID;  break;
      default:       return JOYSTICK_ID;  break;
      }
}


DriverClass OldClassIDToDriverClass (int ii)
{
   switch (ii)
      {
      case WAVE_ID:      return dcWAVE;    break;
      case MIDI_ID:      return dcMIDI;    break;
      case MIXER_ID:     return dcMIXER;   break;
      case AUX_ID:       return dcAUX;     break;
      case MCI_ID:       return dcMCI;     break;
      case ACM_ID:       return dcACODEC;  break;
      case ICM_ID:       return dcVCODEC;  break;
      case VIDCAP_ID:    return dcVIDCAP;  break;
      case JOYSTICK_ID:  return dcJOY;     break;
      default:           return dcOTHER;
      }
}


void FreeIResource (PIRESOURCE pir)
{
   if (pir->pcn != NULL)
      {
      FreeClassNode (pir->pcn);
      LocalFree ((HANDLE)pir->pcn);
      pir->pcn = NULL;
      }
}


void FreeClassNode (PCLASSNODE pcn)
{
   if (pcn->hIcon != NULL)
      {
      DestroyIcon (pcn->hIcon);
      pcn->hIcon = NULL;
      }
}


DWORD GetDriverStatus (PIDRIVER pid)
{
   DWORD     dwStatus;
   SC_HANDLE scManager;
   SC_HANDLE scDriver;
   TCHAR     szName[ cchRESOURCE ];

   dwStatus = 0;

   lsplitpath (pid->szFile, NULL, NULL, szName, NULL);

            // First step: determine if the driver has a service
            //
   if ((scManager = OpenSCManager (NULL, NULL, GENERIC_READ)) != NULL)
      {
      if ((scDriver = OpenService (scManager, szName, GENERIC_READ)) != NULL)
         {
         QUERY_SERVICE_CONFIG  qsc;
         SERVICE_STATUS        ss;
         DWORD                 cbReq;
         void                 *pqsc;

         SetFlag (dwStatus, dwStatusHASSERVICE);

                  // Great!  It has a service.  Find out if the service
                  // is actively running, and whether it is disabled.
                  //
         if (QueryServiceConfig (scDriver, &qsc, sizeof(qsc), &cbReq))
            {
            if (qsc.dwStartType != SERVICE_DISABLED)
               {
               SetFlag (dwStatus, dwStatusSvcENABLED);
               }
            }
         else if ((pqsc = (void *)LocalAlloc (LPTR, cbReq)) != NULL)
            {
            if (QueryServiceConfig (scDriver,
                                    (QUERY_SERVICE_CONFIG *)pqsc,
                                    cbReq, &cbReq))
               {
               if ( ((QUERY_SERVICE_CONFIG *)pqsc)->dwStartType
                     != SERVICE_DISABLED)
                  {
                  SetFlag (dwStatus, dwStatusSvcENABLED);
                  }
               }

            LocalFree ((HANDLE)pqsc);
            }

         if (QueryServiceStatus (scDriver, &ss))
            {
            if ((ss.dwCurrentState != SERVICE_STOPPED) &&
                (ss.dwCurrentState != SERVICE_STOP_PENDING))
               {
               SetFlag (dwStatus, dwStatusSvcSTARTED);
               }
            }

         CloseServiceHandle (scDriver);
         }

      CloseServiceHandle (scManager);
      }

            // If no service, see if we can talk to the driver itself
            //
   if (!IsFlagSet (dwStatus, dwStatusHASSERVICE))
      {
      HANDLE hDriver;

      if ((hDriver = OpenDriver (pid->wszAlias, pid->wszSection, 0L)) != NULL)
         {
         SetFlag (dwStatus, dwStatusDRIVEROK);

         CloseDriver (hDriver, 0L, 0L);
         }
      }

            // If it's a wave device, can we map through it?
            //
   if (GetMappable (pid))
      {
      SetFlag (dwStatus, dwStatusMAPPABLE);
      }

   return dwStatus;
}


void GetTreeItemNodeDesc (LPTSTR pszTarget, PIRESOURCE pir)
{
   lstrcpy (pszTarget, pir->szFriendlyName);
}


void GetTreeItemNodeID (LPTSTR pszTarget, PIRESOURCE pir)
{
   DriverClass dc;
   CLASSNODE cn;

   *pszTarget = 0;  // In case we fail later

   dc = OldClassIDToDriverClass (pir->iClassID);
   if (!DriverClassToClassNode (&cn, dc))
      return;

   switch (pir->iNode)  // 1=class, 2=device, 3=acm, 4=instmt
      {
      case 1:   // class
         lstrcpy (pszTarget, cn.szClass);
         break;

      case 2:   // instrument
         wsprintf (pszTarget, TEXT("%s\\%s"), cn.szClass, pir->szDrvEntry);
         break;

      case 4:   // instrument
         lstrcpy (pszTarget, pir->szDesc);
         break;

      default:
         lstrcpy (pszTarget, pir->szDesc);
         break;
      }

   FreeClassNode (&cn);
}


void ShowDeviceProperties (HWND hPar, HTREEITEM hti)
{
   IRESOURCE   ir;
   CLASSNODE   cn;
   DEVTREENODE dtn;
   short       idr;
   TCHAR        szTitle[ cchRESOURCE ];
   TCHAR        szTab[ cchRESOURCE ];
   DriverClass dc;
   PIDRIVER    pid;

   if (hti == NULL)
      return;

   if (TreeView_GetParent (hAdvDlgTree, hti) &&
       TreeView_GetGrandParent (hAdvDlgTree, hti) &&
       (GuessDriverClassFromTreeItem (
                       TreeView_GetGrandParent (hAdvDlgTree, hti)
                                     ) == dcMIDI))
      {
      if (InstrumentToResource (&ir, hti))
         {
         ShowMidiPropSheet (NULL,
                            ir.szFriendlyName,
                            hPar,
                            MIDI_INSTRUMENT_PROP,
                            ir.szFriendlyName,
                            hti,
                            (LPARAM)&ir,
                            (LPARAM)hAdvDlgTree);

         FreeIResource (&ir);
         }
      return;
      }
   else if ((pid = FindIDriverByTreeItem (hti)) != NULL)
      {
      dc = GuessDriverClassFromTreeItem (TreeView_GetParent(hAdvDlgTree,hti));
      if (dc == dcINVALID)
         {
         if ((dc = GuessDriverClass (pid)) == dcINVALID)
            return;
         }
      }
   else
      {
      if ((dc = GuessDriverClassFromTreeItem (hti)) == dcINVALID)
         return;
      }

   if (g_dcFilterClass != dcINVALID)
   {
        if ((dc == dcOTHER) || (dc == dcINVALID))
        {
            dc = g_dcFilterClass;
        }
   }

   if ((idr = DriverClassToRootIndex (dc)) == -1)
      return;

   if (pid == NULL) // Just want class properties?
      {
      if (!DriverClassToClassNode (&cn, dc))
         return;

      if (dc == dcMIDI) // MIDI class properties?
         {
         GetString (szTitle, aDriverRoot[idr].idDesc);

         ShowMidiPropSheet (NULL,
                            szTitle,
                            hPar,
                            MIDI_CLASS_PROP,
                            szTitle,
                            hti,
                            (LPARAM)&cn,
                            (LPARAM)hAdvDlgTree);
         }
      else // Generic class properties?  (nothing to do, really)
         {
         GetString (szTab,   IDS_GENERAL);
         GetString (szTitle, aDriverRoot[idr].idDesc);

         dtn.lParam = (LPARAM)&cn;
         dtn.hwndTree = hAdvDlgTree;

         ShowPropSheet (szTab,
                        DevPropDlg,
                        DLG_DEV_PROP,
                        hPar,
                        szTitle,
                        (LPARAM)&dtn);
         }

      FreeClassNode (&cn);
      }
   else
      {
      switch (dc)
         {
         case dcACODEC:
               GetString (szTab, IDS_GENERAL);

               ShowPropSheet (szTab,
                              ACMDlg,
                              DLG_ACMDEV_PROP,
                              hPar,
                              pid->szDesc,
                              pid->lp);

               // Re-sort the Audio Codec entries, in case their priorities
               // have changed.  Then find treeview item for this codec,
               // and select it.
               //
               {
               HTREEITEM  hti;
               short      idr;

               SendMessage (hAdvDlgTree, WM_SETREDRAW, FALSE, 0L);
               FillTreeFromMSACM (hAdvDlgTree);
               SendMessage (hAdvDlgTree, WM_SETREDRAW, TRUE, 0L);

               if ((idr = DriverClassToRootIndex (dcACODEC)) != -1)
                  {
                  if ((hti = aDriverRoot[ idr ].hti) != NULL)
                     {
                     for (hti = TreeView_GetChild (hAdvDlgTree, hti);
                          hti != NULL;
                          hti = TreeView_GetNextSibling (hAdvDlgTree, hti))
                        {
                        if (pid == FindIDriverByTreeItem (hti))
                           {
                           TreeView_SelectItem (hAdvDlgTree, hti);
                           break;
                           }
                        }
                     }
                  }
               }
              break;

         case dcMIDI:
               if (!DriverToResource (hPar, &ir, pid, dc))
                  break;

               GetString (szTab, IDS_GENERAL);

               dtn.lParam = (LPARAM)&ir;
               dtn.hwndTree = hAdvDlgTree;

               ShowWithMidiDevPropSheet (szTab,
                                         DevPropDlg,
                                         DLG_DEV_PROP,
                                         hPar,
                                         pid->szDesc,
                                         hti,
                                         (LPARAM)&dtn,
                                         (LPARAM)&ir,
                                         (LPARAM)hAdvDlgTree);

               FreeIResource (&ir);
              break;

         case dcWAVE:
               if (!DriverToResource (hPar, &ir, pid, dc))
                  break;

               GetString (szTab, IDS_GENERAL);

               dtn.lParam = (LPARAM)&ir;
               dtn.hwndTree = hAdvDlgTree;

               ShowPropSheet (szTab,
                              DevPropDlg,
                              DLG_WAVDEV_PROP,
                              hPar,
                              pid->szDesc,
                              (LPARAM)&dtn);

               FreeIResource (&ir);
              break;

         default:
               if (!DriverToResource (hPar, &ir, pid, dc))
                  break;

               GetString (szTab, IDS_GENERAL);

               dtn.lParam = (LPARAM)&ir;
               dtn.hwndTree = hAdvDlgTree;

               ShowPropSheet (szTab,
                              DevPropDlg,
                              DLG_DEV_PROP,
                              hPar,
                              pid->szDesc,
                              (LPARAM)&dtn);

               FreeIResource (&ir);
              break;
         }
      }
}



#include "medhelp.h"

const static DWORD aDevPropHelpIds[] = {  // Context Help IDs
    ID_DEV_SETTINGS,     IDH_MMCPL_DEVPROP_SETTINGS,
    IDC_DEV_ICON,        NO_HELP,
    IDC_DEV_DESC,        NO_HELP,
    IDC_DEV_STATUS,      NO_HELP,
    IDC_ENABLE,          IDH_MMCPL_DEVPROP_ENABLE,
    IDC_DISABLE,         IDH_MMCPL_DEVPROP_DISABLE,
    IDC_DONOTMAP,        IDH_MMCPL_DEVPROP_DONT_MAP,
    0, 0
};

/*
 ***************************************************************
 * DlgProc for device Property sheet.
 ***************************************************************
 */
INT_PTR CALLBACK DevPropDlg (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR   *lpnm;

    switch (uMsg)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_KILLACTIVE:
                    FORWARD_WM_COMMAND(hDlg, IDOK, 0, 0, SendMessage);
                    break;

                case PSN_APPLY:
                    FORWARD_WM_COMMAND(hDlg, ID_APPLY, 0, 0, SendMessage);
                    break;

                case PSN_SETACTIVE:
                    FORWARD_WM_COMMAND(hDlg, ID_INIT, 0, 0, SendMessage);
                    break;

                case PSN_RESET:
                    FORWARD_WM_COMMAND(hDlg, IDCANCEL, 0, 0, SendMessage);
                    break;
            }
            break;

        case WM_INITDIALOG:
        {
            PIRESOURCE pIResource;
            PCLASSNODE pcn;
            PDEVTREENODE pdtn = (PDEVTREENODE)(((LPPROPSHEETPAGE)lParam)->lParam);

            SetWindowLongPtr(hDlg, DWLP_USER, ((LPPROPSHEETPAGE)lParam)->lParam);

            if (*((short *)(pdtn->lParam)) == 1)
            {
                HWND hwndTree = pdtn->hwndTree;
                HTREEITEM  htiCur = TreeView_GetSelection(hwndTree);
                TCHAR   sz[cchRESOURCE];
                TV_ITEM tvi;

                tvi.mask = TVIF_CHILDREN;
                tvi.hItem = htiCur;
                TreeView_GetItem(hwndTree, &tvi);

                pcn = (PCLASSNODE)(pdtn->lParam);
                //set class icon.
                SendDlgItemMessage(hDlg, IDC_DEV_ICON, STM_SETICON, (WPARAM)pcn->hIcon , 0L);
                SetWindowText(GetDlgItem(hDlg, IDC_DEV_DESC), pcn->szClassName);
                DestroyWindow(GetDlgItem(hDlg, IDC_ENABLE));
                DestroyWindow(GetDlgItem(hDlg, IDC_DISABLE));
                DestroyWindow(GetDlgItem(hDlg, ID_DEV_SETTINGS));

                GetString (sz, (tvi.cChildren) ? IDS_NOPROP : IDS_NODEVS);
                SetWindowText(GetDlgItem(hDlg, IDC_DEV_STATUS), sz);
            }
            else
            {
                pIResource = (PIRESOURCE)(pdtn->lParam);
                SendDlgItemMessage(hDlg, IDC_DEV_ICON, STM_SETICON, (WPARAM)pIResource->pcn->hIcon , 0L);
                SetWindowText(GetDlgItem(hDlg, IDC_DEV_DESC), pIResource->szDesc);
                if (!IsFlagSet(pIResource->fStatus, dwStatusHASSERVICE) &&
                    !IsFlagSet(pIResource->fStatus, dwStatusDRIVEROK))
                {
                    SetDevStatus(pIResource->fStatus, hDlg);
                }
                else
                {
                    if (pIResource->iClassID == WAVE_ID)
                    {
                        CheckDlgButton (hDlg,
    IDC_DONOTMAP,
    IsFlagClear(pIResource->fStatus,
              dwStatusMAPPABLE));
                    }

                    if (!pIResource->fQueryable || pIResource->fQueryable == -1)
                        EnableWindow(GetDlgItem(hDlg, ID_DEV_SETTINGS), FALSE);
                    if (!IsFlagSet (pIResource->fStatus, dwStatusHASSERVICE))
                    {
                        DestroyWindow(GetDlgItem(hDlg, IDC_ENABLE));
                        DestroyWindow(GetDlgItem(hDlg, IDC_DISABLE));
                    }
                    else
                    {
                        TCHAR szStatusStr[MAXSTR];
                        DriverClass dc;
                        short idr;

                        dc = OldClassIDToDriverClass (pIResource->iClassID);
                        idr = DriverClassToRootIndex (dc);

                        if (idr == -1)
                        {
                           DestroyWindow (GetDlgItem(hDlg, IDC_ENABLE));
                           DestroyWindow (GetDlgItem(hDlg, IDC_DISABLE));
                        }
                        else
                        {
                           GetString (szStatusStr, aDriverRoot[idr].idEnable);
                           SetDlgItemText(hDlg, IDC_ENABLE, szStatusStr);
                           GetString (szStatusStr, aDriverRoot[idr].idDisable);
                           SetDlgItemText(hDlg, IDC_DISABLE, szStatusStr);
                        }
                    }
                }
                SetDevStatus(pIResource->fStatus, hDlg);
            }

#ifdef FIX_BUG_15451
            if (szDriverWhichNeedsSettings[0] != TEXT('\0'))
            {
                MakeThisDialogLookLikeTheOldDialog (GetParent(hDlg));
            }
#endif // FIX_BUG_15451
            break;
        }

        case WM_DESTROY:
            break;

        case WM_DROPFILES:
            break;

        case WM_CONTEXTMENU:
            WinHelp ((HWND) wParam, NULL, HELP_CONTEXTMENU,
                    (UINT_PTR) (LPTSTR) aDevPropHelpIds);
            return TRUE;

        case WM_HELP:
        {
            LPHELPINFO lphi = (LPVOID) lParam;
            WinHelp (lphi->hItemHandle, NULL, HELP_WM_HELP,
                    (UINT_PTR) (LPTSTR) aDevPropHelpIds);
            return TRUE;
        }

        case WM_COMMAND:
            HANDLE_WM_COMMAND(hDlg, wParam, lParam, DoDevPropCommand);
            break;
    }
    return FALSE;
}

/*
 ***************************************************************
 *
 ***************************************************************
 */
BOOL PASCAL DoDevPropCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    PDEVTREENODE pdtn = (PDEVTREENODE)GetWindowLongPtr(hDlg, DWLP_USER);
    PIRESOURCE pIResource;
    static int fDevStatus;

    if (!pdtn)
        return FALSE;
    pIResource = (PIRESOURCE)(pdtn->lParam);

    switch (id)
    {

    case ID_APPLY:
        if ((pIResource->iNode == 2) && (fDevStatus != pIResource->fStatus))
        {
            if ( (IsFlagSet (fDevStatus, dwStatusMAPPABLE)) !=
                 (IsFlagSet (pIResource->fStatus, dwStatusMAPPABLE)) )
            {
                SetMappable (pIResource,
                             (BOOL)IsFlagSet (fDevStatus, dwStatusMAPPABLE));
            }

            if ( (IsFlagSet (fDevStatus, dwStatusHASSERVICE)) &&
                 (IsFlagSet (fDevStatus, dwStatusSvcENABLED) !=
                  IsFlagSet (pIResource->fStatus, dwStatusSvcENABLED)) )
            {
#if 0 // TODO: Multiportmidi
                if ( (pIResource->iClassID == MIDI_ID) &&
                     (IsFlagSet(fDevStatus, DEV_MULTIPORTMIDI)) )
                {
                    EnableMultiPortMIDI (pIResource,
                                         IsFlagSet (fDevStatus,
                dwStatusSvcENABLED));
                }
                else
#endif
                {
                    EnableDriverService (pIResource,
                                         IsFlagSet (fDevStatus,
                dwStatusSvcENABLED));
                }
            }
            DisplayMessage(hDlg, IDS_CHANGESAVED, IDS_RESTART, MB_OK);
        }
        return TRUE;

    case IDOK:
        return TRUE;
    case IDCANCEL:
        break;

    case ID_INIT:
        if (pIResource->iNode == 2)
            fDevStatus = pIResource->fStatus;
#ifdef FIX_BUG_15451
        if (szDriverWhichNeedsSettings[0] != TEXT('\0'))
        {
            FORWARD_WM_COMMAND(hDlg,ID_DEV_SETTINGS,0,0,PostMessage);
        }
#endif // FIX_BUG_15451
        break;

    case IDC_DONOTMAP:
        if(Button_GetCheck(GetDlgItem(hDlg, IDC_DONOTMAP)))
            ClearFlag(fDevStatus, dwStatusMAPPABLE);
        else
            SetFlag(fDevStatus, dwStatusMAPPABLE);
        PropSheet_Changed(GetParent(hDlg),hDlg);
        break;

    case IDC_ENABLE:
        if (IsFlagSet (fDevStatus, dwStatusHASSERVICE))
        {
           SetFlag(fDevStatus, dwStatusSvcENABLED);
           SetDevStatus(fDevStatus, hDlg);
           PropSheet_Changed(GetParent(hDlg),hDlg);
#if 0 // TODO: Multiportmidi
           if (IsFlagSet(fDevStatus, DEV_MULTIPORTMIDI))
           {
               DisplayMessage(hDlg, IDS_ENABLE, IDS_ENABLEMULTIPORTMIDI, MB_OK);
           }
#endif
        }
        break;

    case IDC_DISABLE:
        if (IsFlagSet (fDevStatus, dwStatusHASSERVICE))
        {
           ClearFlag(fDevStatus, dwStatusSvcENABLED);
           SetDevStatus(fDevStatus, hDlg);
           PropSheet_Changed(GetParent(hDlg),hDlg);
#if 0 // TODO: Multiportmidi
           if (IsFlagSet(fDevStatus, DEV_MULTIPORTMIDI))
           {
               DisplayMessage(hDlg, IDS_DISABLE, IDS_DISABLEMULTIPORTMIDI, MB_OK);
           }
#endif
        }
        break;

    case ID_DEV_SETTINGS:
#ifdef FIX_BUG_15451
        if (szDriverWhichNeedsSettings[0] != TEXT('\0'))
        {
            ConfigureDriver (hDlg, szDriverWhichNeedsSettings);
            szDriverWhichNeedsSettings[0] = 0;
        }
        else
        {
            PIDRIVER  pid;

            if ((pid = FindIDriverByResource (pIResource)) == NULL)
                break;

            ShowDriverSettings (hDlg, pid->szFile);
        }
#else // FIX_BUG_15451
    {
        PIDRIVER  pid;
        HANDLE    hDriver;

        if ((pid = FindIDriverByResource (pIResource)) == NULL)
            break;

        if ((hDriver = OpenDriver (pid->wszAlias, pid->wszSection, 0L)) == 0)
        {
            OpenDriverError(hDlg, pid->szDesc, pid->szFile);
        }
        else
        {
            DRVCONFIGINFO   DrvConfigInfo;
            InitDrvConfigInfo(&DrvConfigInfo, pid);
            if ((SendDriverMessage(
                     hDriver,
                     DRV_CONFIGURE,
                     (LONG)hDlg,
                     (LONG)(LPDRVCONFIGINFO)&DrvConfigInfo) ==
                DRVCNF_RESTART))
            {
               iRestartMessage= 0;
               DialogBox(myInstance,
                  MAKEINTRESOURCE(DLG_RESTART), hDlg, RestartDlg);
            }
            CloseDriver(hDriver, 0L, 0L);
        }
        }
#endif // FIX_BUG_15451
        break;
    }
    return FALSE;
}


/*
 ***************************************************************
 * Check the status flag for the device and display the appropriate text the
 *  the device properties prop sheet.
 ***************************************************************
 */
STATIC void SetDevStatus(int iStatus, HWND hDlg)
{
    HWND hwndS = GetDlgItem(hDlg, IDC_DEV_STATUS);
    TCHAR szStatus[cchRESOURCE];

    if (IsFlagSet (iStatus, dwStatusHASSERVICE))
    {
        if (IsFlagSet (iStatus, dwStatusSvcENABLED))
        {
            if (IsFlagSet (iStatus, dwStatusSvcSTARTED))
                GetString (szStatus, IDS_DEVENABLEDOK);
            else
                GetString (szStatus, IDS_DEVENABLEDNOTOK);

            CheckRadioButton (hDlg, IDC_ENABLE, IDC_DISABLE, IDC_ENABLE);
        }
        else // service has been disabled
        {
            if (IsFlagSet (iStatus, dwStatusSvcSTARTED))
                GetString (szStatus, IDS_DEVDISABLEDOK);
            else
                GetString (szStatus, IDS_DEVDISABLED);

            CheckRadioButton(hDlg, IDC_ENABLE, IDC_DISABLE, IDC_DISABLE);
        }

        SetWindowText(hwndS, szStatus);
    }
    else // driver does not have a service, and thus can't be disabled
    {
        if (IsFlagSet (iStatus, dwStatusDRIVEROK))
            GetString (szStatus, IDS_DEVENABLEDOK);
        else
            GetString (szStatus, IDS_DEVENABLEDNODRIVER);

        CheckRadioButton(hDlg, IDC_ENABLE, IDC_DISABLE, IDC_ENABLE);
        SetWindowText(hwndS, szStatus);
    }
}


/*
 *** EnableDriverService - enable or disable a service-based driver
 *
 * If Enable, the service will be set to start==system_start.
 * If !Enable, the service will be set to start==disabled.
 *
 */

void EnableDriverService (PIRESOURCE pir, BOOL fEnable)
{
   SC_HANDLE scManager;
   SC_HANDLE scDriver;
   TCHAR     szName[ cchRESOURCE ];

   lsplitpath (pir->szFile, NULL, NULL, szName, NULL);

            // First step: determine if the driver has a service
            //
   if ((scManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS)) != NULL)
      {
      if ((scDriver = OpenService (scManager, szName, SERVICE_ALL_ACCESS)) != 0)
         {
         QUERY_SERVICE_CONFIG  qsc;
         SERVICE_STATUS        ss;
         DWORD                 cbReq;
         void                 *pqsc;

         ChangeServiceConfig (scDriver,
                              SERVICE_NO_CHANGE,
                              (fEnable) ? SERVICE_SYSTEM_START
                                        : SERVICE_DISABLED,
                              SERVICE_NO_CHANGE,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);

         CloseServiceHandle (scDriver);
         }

      CloseServiceHandle (scManager);
      }
}


/*
 ***************************************************************
 * SetMappable
 *
 * Sets the "Mappable" value for wave devices in the registry.
 * The registry key is created if necessary
 *
 ***************************************************************
 */

BOOL SetMappable (PIRESOURCE pIResource, BOOL fMappable)
{
    TCHAR   szFile[ _MAX_FNAME +1 +_MAX_EXT +1 ];
    TCHAR   szExt[ _MAX_EXT +1 ];
    TCHAR   szRegKey[MAX_PATH+1];
    DWORD   dwMappable;
    HKEY    hKey;

    dwMappable = (fMappable) ? 1 : 0;

    lsplitpath (pIResource->szFile, NULL, NULL, szFile, szExt);
    if (szExt[0] != TEXT('\0'))
        lstrcat (szFile, szExt);

    wsprintf (szRegKey, TEXT("%s\\%s"), REGSTR_PATH_WAVEMAPPER, szFile);

    if (RegCreateKey (HKEY_LOCAL_MACHINE, szRegKey, &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    if (RegSetValueEx (hKey,
                       REGSTR_VALUE_MAPPABLE,
                       (DWORD)0,
                       REG_DWORD,
                       (void *)&dwMappable,
                       sizeof(dwMappable)) != ERROR_SUCCESS)
    {
        RegCloseKey (hKey);
        return FALSE;
    }

    RegCloseKey (hKey);

    if (fMappable)
    {
        SetFlag(pIResource->fStatus, dwStatusMAPPABLE);
    }
    else
    {
        ClearFlag(pIResource->fStatus, dwStatusMAPPABLE);
    }

    return TRUE;
}


BOOL GetMappable (PIDRIVER pIDriver)
{
    TCHAR   szFile[ _MAX_FNAME +1 +_MAX_EXT +1 ];
    TCHAR   szExt[ _MAX_EXT +1 ];
    TCHAR   szRegKey[MAX_PATH+1];
    DWORD   dwMappable;
    DWORD   dwSize;
    DWORD   dwType;
    HKEY    hKey;

    lsplitpath (pIDriver->szFile, NULL, NULL, szFile, szExt);
    if (szExt[0] != TEXT('\0'))
        lstrcat (szFile, szExt);

    wsprintf (szRegKey, TEXT("%s\\%s"), REGSTR_PATH_WAVEMAPPER, szFile);

    if (RegOpenKey (HKEY_LOCAL_MACHINE, szRegKey, &hKey) != ERROR_SUCCESS)
    {
        return TRUE;
    }

    dwSize = sizeof(dwMappable);
    if (RegQueryValueEx (hKey,
                         REGSTR_VALUE_MAPPABLE,
                         NULL,
                         &dwType,
                         (void *)&dwMappable,
                         &dwSize) != ERROR_SUCCESS)
    {
        RegCloseKey (hKey);
        return TRUE;
    }

    RegCloseKey (hKey);

    return (dwMappable) ? TRUE : FALSE;
}


#ifdef FIX_BUG_15451

        // FixDriverService - work around known problems with sound drivers
        //
        // If there was a functioning kernel-mode service in place before the
        // config dialog was presented, then there should still be one
        // afterwards.  However, there are two known problems with the
        // currently-release drivers:
        //
        // 1) The service may not have shut down properly, and is stuck
        //    in a STOP_PENDING state(*).  If this is the case, we need to
        //    ensure that the service is set to load on SYSTEM_START, and
        //    tell the user that the machine has to be rebooted before sound
        //    will work again.
        //
        // 2) The service may have failed to restart properly, and is
        //    stopped(**).  If this is the case, and LoadType!=0, try setting
        //    LoadType=1 and starting the service.
        //
        // (*) -- bug #15451 in NT/SUR, where pending IRPs and open mixer
        //        handles prevent the service from shutting down
        //
        // (**) -- bug #XXXXX in NT/SUR, where some RISC machines stop the
        //         service, set LoadType=1, and fail to restart the service
        //         after you cancel their config dialog
        //
BOOL FixDriverService (PIDRIVER pid)
{
    SC_HANDLE scManager;
    SC_HANDLE scDriver;
    SERVICE_STATUS ss;
    BOOL rc = FALSE;
    TCHAR szName[ cchRESOURCE ];

    lsplitpath (pid->szFile, NULL, NULL, szName, NULL);

    // First step: open the service...even if it's hosed, we should
    // still be able to do this.
    //
    if ((scManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS)) == 0)
    {
        return FALSE;
    }
    if ((scDriver = OpenService (scManager, szName, SERVICE_ALL_ACCESS)) == 0)
    {
        CloseServiceHandle (scManager);
        return FALSE;
    }

    // Now check its status.  Look for STOP_PENDING and STOPPED states.
    //
    if (QueryServiceStatus (scDriver, &ss))
    {
        if (ss.dwCurrentState == SERVICE_STOP_PENDING)
        {
            // The service didn't stop properly--we'll have to reboot.
            // Make sure the service is configured such that it will start
            // properly when we restart.
            //
            ChangeServiceConfig (scDriver,
                                 SERVICE_NO_CHANGE,
                                 SERVICE_SYSTEM_START,   // Enable this puppy!
                                 SERVICE_NO_CHANGE,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);

            // Tell the caller that a reboot is mandatory
            rc = TRUE;
        }
        else if (ss.dwCurrentState == SERVICE_STOPPED)
        {
            TCHAR szKey[ cchRESOURCE ];
            HKEY  hkParams;

            // The service stopped, but didn't restart properly--it could
            // just be the LoadType problem.  To fix this, we'll need
            // to enumerate all the keys underneath the key
            // HKLM\System\CurrentControlSet\Services\(thisdriver)\Parameters,
            // and look for ...\Parameters\*\LoadType==(DWORD)1.
            //
            // First step is to open HKLM\System\CCS\Services\(driver)\Parms.
            //
            wsprintf (szKey, TEXT("%s\\%s\\Parameters"),
                             REGSTR_PATH_SERVICES,
                             szName);

            if (RegOpenKey (HKEY_LOCAL_MACHINE, szKey, &hkParams) == 0)
            {
                DWORD cSubKeys;
                DWORD iSubKey;
                BOOL fFixedLoadType = FALSE;

                // Find out how many subkeys there are under here--the
                // keys we want are named "Device0" through "Device(n-1)"
                //
                RegQueryInfoKey (hkParams,
                                 NULL,          // lpClass
                                 NULL,          // lpcbClass
                                 NULL,          // lpReserved
                                 &cSubKeys,     // Whoops!  We want this.
                                 NULL,          // lpcbMaxSubKeyLen
                                 NULL,          // lpcbMaxClassLen
                                 NULL,          // lpcValues
                                 NULL,          // lpcbMaxValueNameLen
                                 NULL,          // lpcbMaxValueLen
                                 NULL,          // lpcbSecurityDescriptor
                                 NULL);         // lpftLastWriteTime

                // Open each subkey in turn, and look for a LoadType=
                // which is bogus.
                //
                for (iSubKey = 0; iSubKey < cSubKeys; ++iSubKey)
                {
                    HKEY hk;
                    TCHAR szSubKey[ cchRESOURCE ];
                    wsprintf (szSubKey, TEXT("Device%lu"), (LONG)iSubKey);

                    if (RegOpenKey (hkParams, szSubKey, &hk) == ERROR_SUCCESS)
                    {
                        DWORD dwLoadType;
                        DWORD dwType;
                        DWORD dwSize = sizeof(dwType);

                        if (RegQueryValueEx (hk,
                                             cszRegValueLOADTYPE,
                                             NULL,
                                             &dwType,
                                             (void *)&dwLoadType,
                                             &dwSize) == 0)
                        {
                            if (dwLoadType == 1)
                            {
                                dwLoadType = 0;
                                fFixedLoadType = TRUE;

                                RegSetValueEx (hk,
                                               cszRegValueLOADTYPE,
                                               0,
                                               REG_DWORD,
                                               (void *)&dwLoadType,
                                               sizeof(dwLoadType));
                            }
                        }

                        RegCloseKey (hk);
                    }
                }

                // If we fixed a LoadType value, try to restart the service.
                //
                if (fFixedLoadType)
                {
                    if (StartService (scDriver, 0, NULL))
                    {
                        ChangeServiceConfig (scDriver,
                                             SERVICE_NO_CHANGE,
                                             SERVICE_SYSTEM_START,
                                             SERVICE_NO_CHANGE,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL);
                    }
                }
            }
        }
    }


    // Clean up.  We'll return TRUE if a reboot is necessary, and FALSE
    // otherwise.
    //
    CloseServiceHandle (scDriver);
    CloseServiceHandle (scManager);
    return rc;
}


void ConfigureDriver (HWND hDlg, LPTSTR pszName)
{
    PIDRIVER  pid;
    HANDLE    hDriver;
    BOOL      fShowTrayVol;
    BOOL      fRestartDialog = FALSE;

    if ((pid = FindIDriverByName (pszName)) == NULL)
        return;

    fShowTrayVol = GetTrayVolumeEnabled();
    if (fShowTrayVol)
        SetTrayVolumeEnabled(FALSE);

    if ((hDriver = OpenDriver (pid->wszAlias, pid->wszSection, 0L)) == 0)
    {
        OpenDriverError(hDlg, pid->szDesc, pid->szFile);
    }
    else
    {
        DWORD dwStatus = GetDriverStatus (pid);
        BOOL fHadService = IsFlagSet (dwStatus, dwStatusSvcSTARTED) &&
                           IsFlagSet (dwStatus, dwStatusHASSERVICE);

        DRVCONFIGINFO   DrvConfigInfo;
        InitDrvConfigInfo(&DrvConfigInfo, pid);
        if ((SendDriverMessage(
                 hDriver,
                 DRV_CONFIGURE,
                 (LONG_PTR)hDlg,
                 (LONG_PTR)(LPDRVCONFIGINFO)&DrvConfigInfo) ==
            DRVCNF_RESTART))
        {
            iRestartMessage = 0;
            fRestartDialog = TRUE;
        }
        CloseDriver(hDriver, 0L, 0L);

        // If there was a functioning kernel-mode service in place before the
        // config dialog was presented, then we should verify that there is
        // still one in place now.  See FixDriverService() for details.
        //
        if (fHadService)
        {
            dwStatus = GetDriverStatus (pid);

            if (!IsFlagSet (dwStatus, dwStatusSvcSTARTED) ||
                !IsFlagSet (dwStatus, dwStatusHASSERVICE))
            {
                if (FixDriverService (pid))
                {
                    iRestartMessage = IDS_RESTART_NOSOUND;
                    fRestartDialog = TRUE;
                }
            }
        }
    }


    if (fShowTrayVol)
        SetTrayVolumeEnabled(TRUE);

    if (fRestartDialog)
    {
        DialogBox(myInstance,MAKEINTRESOURCE(DLG_RESTART),hDlg,RestartDlg);
    }
}


BOOL fDeviceHasMixers (LPTSTR pszName)
{
    HKEY  hk;
    UINT  ii;
    BOOL  rc = FALSE;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_DRIVERS32, &hk))
    {
        return FALSE;
    }

    for (ii = 0; ; ++ii)
    {
        TCHAR  szLHS[ cchRESOURCE ];
        TCHAR  szRHS[ cchRESOURCE ];
        DWORD  dw1;
        DWORD  dw2;
        DWORD  dw3;

        dw1 = cchRESOURCE;
        dw3 = cchRESOURCE;
        if (RegEnumValue (hk, ii,  szLHS, &dw1,
                          0, &dw2, (LPBYTE)szRHS, &dw3) != ERROR_SUCCESS)
        {
            break;
        }

        if ( (GuessDriverClassFromAlias (szLHS) == dcMIXER) &&
             (!FileNameCmp (pszName, szRHS)) )
        {
            rc = TRUE;
            break;
        }
    }

    RegCloseKey (hk);
    return rc;
}
#endif // FIX_BUG_15451


TCHAR c_tszControlExeS[] = TEXT("control.exe %s");

BOOL RunJoyControlPanel(void)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR tsz[MAX_PATH];
    BOOL  fRtn;

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    wsprintf(tsz, c_tszControlExeS, TEXT("joy.cpl"));
    if (CreateProcess(0, tsz, 0, 0, 0, 0, 0, 0, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    fRtn = TRUE;
    } else {
        fRtn = FALSE;
    }

    return fRtn;
}
