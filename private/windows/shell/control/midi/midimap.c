/*
 * MIDIMAP.C
 *
 * Copyright (C) 1990 Microsoft Corporation.  All rights reserved.
 *
 * File I/O support routines for MIDI mapper and control panel.
 *
 * History:
 *
 * gregsi       30-Apr-91 midimap.ini -> midimap.cfg
 * t-mikemc     27-Sep-90 mapKeyMapInSetup, mapPatchMapInSetup, mapExists,
 *                        and mapGetUsageCount now all return DWORDS.
 *                        mapGetSize can return an error now.
 * t-mikemc     24-Sep-90 Added count to MMTABLE data structure.  Made
 *                        deleting setups and patchmaps update usage count
 *                        of patchmaps and keymaps, respectively.
 * t-mikemc     23-Sep-90 Finally got that wNameID taken out of memory
 *                        data structures.
 * t-mikemc     22-Sep-90 Made MmaperrWriteSetup look for invalid port ID's
 *                        and write "invalid" device names correctly.
 * t-mikemc     21-Sep-90 Mucked about with the mapEnumerate function to
 *                        allow for enumeration of ports accessed by a setup.
 * t-mikemc     31-Jul-90 Added volume mapping and mapGetUsageCount API.
 * t-mikemc     15-Jul-90 Slew-o-changes! Munged Write/Read/Enum/Delete
 *                        API's together into single functions.  Finished
 *                        up work on 'use-count' for patch and key maps.
 *                        Commented and generally cleaned up lots-o-code.
 * t-mikemc     05-Jul-90 Added mapPatchMapInSetup and mapKeyMapInSetup
 *                        functions.
 * t-mikemc     13-Jun-90 Removed wSize element from setup data structure.
 *                        Setups now read in size of each referenced
 *                        patchmap.
 * t-mikemc     20-May-90 Added DWORD dwFlags to all internal file data
 *                        structures.
 * t-mikemc     11-May-90 Changed enumeration functions so they send
 *                        name and description.
 * t-mikemc     19-Apr-90 Changed routines to use binary file format.
 * t-mikemc     17-Apr-90 Created.
 */

//      Please note that the use of "goto" statements in this module isn't
//      disgusting.
//
//      brucemo

//      Well it disgusts me.  I put readability before writeability.
//      The compiler generates no less than 17 messages saying that a
//      variable may be used before initialised, and that means that
//      either the thing is full of bugs (so Dijkstra was right) or
//      else it's having a tough time following the flow of the logic.
//      Me too.
//      For good measure, the optimisation in compilers is almost always
//      inhibited by goto.
//      LaurieGr

#include <windows.h>
#include <string.h>
#define MMNOMCI
#define MMNOJOY
#define MMNOSOUND
#define MMNOWAVE
#include <mmsystem.h>
#if defined(WIN32)
#include <port1632.h>
#endif //WIN32
#include "hack.h"
#include "midimap.h"
#include "midi.h"
#include "extern.h"

/*-=-=-=-=- Global Definitions  -=-=-=-=-*/

#define MM_VERSION      1               // mapfile version number

#define MM_NUMSETUP     100             // number of setups
#define MM_NUMPATCH     100             // number of patchmaps
#define MM_NUMKEY       100             // number of keymaps

#define MAP_FOPEN       1               // open file
#define MAP_FCREATE     2               // create/open file
#define MAP_FCLOSE      3               // close file

#define MAKEID(id)      ((LPSTR) (LONG) (id))

#define LSB             1               // LSB for usage byte
#define MSB             128             // MSB for usage byte

#define MAX_UNIQUENAME  32              // max length of unique name

/*-=-=-=-=- Internal Data Structures    -=-=-=-=-*/

#pragma pack(1)

// Internal file data structures

typedef struct midimapkey_tag {
                  WORD    wUsing;                 // number of patchmaps using this
                  BYTE    bKMap[MIDIPATCHSIZE];   // translate table for key map
                  DWORD   dwFlags;                // flags
} MMKEY;
typedef MMKEY UNALIGNED FAR *LPMMKEY;

typedef struct midimappatch_tag {
                  WORD    wUsing;                 // number of setups using this
                  BYTE    bVMax;                  // max volume scalar
                  WORD    wPMap[MIDIPATCHSIZE];   // lobyte=xlat table, hibyte=volume  // a PATCHARRAY?
                  DWORD   dwSize;                 // size of patchmap
                  WORD    idxKMapNames[MIDIPATCHSIZE];    // keymap name table indexes // a KEYARRAY?
                  DWORD   dwFlags;                // flags
} MMPATCH;
typedef MMPATCH UNALIGNED FAR *LPMMPATCH;

typedef struct midimapchannel_tag {
                  WORD    wChannel;               // port channel of device
                  BYTE    szDevice[MAXPNAMELEN];  // device name
                  WORD    idxPMapName;            // patchmap name table index
                  DWORD   dwFlags;                // flags
} MMCHANNEL;
typedef MMCHANNEL UNALIGNED FAR *LPMMCHANNEL;

typedef struct midimapsetup_tag {
                  MMCHANNEL chMap[16];            // array of channel maps
                  DWORD   dwFlags;                // flags
} MMSETUP;
typedef MMSETUP UNALIGNED FAR *LPMMSETUP;

typedef struct midimaptableentry_tag {
                  BYTE    szName[MMAP_MAXNAME];   // name of map
                  BYTE    szDesc[MMAP_MAXDESC];   // description of map
                  WORD    idxEntry;               // index of this entry in table
                  DWORD   doData;                 // file offset of data structure
} MMTABLEENTRY;
typedef MMTABLEENTRY UNALIGNED FAR *LPMMTABLEENTRY;

typedef struct midimaptableheader_tag {
                  WORD    wEntrys;                // number of entries in table
                  WORD    wUsed;                  // number of entries being used
} MMTABLEHEADER;
typedef MMTABLEHEADER UNALIGNED FAR *LPMMTABLEHEADER;

typedef struct midimapheader_tag {
                  WORD    wVersion;               // version number of file
                  DWORD   dwGarbage;              // garbage collection bytes
                  WORD    idxCurSetup;            // current setup table index
                  WORD    oSetup;                 // setup table offset
                  WORD    oPatch;                 // patchmap table offset
                  WORD    oKey;                   // keymap table offset
} MMHEADER;

// Internal memory data structures.

typedef struct midimaptable_tag {
                  WORD            wCount;         // times this table has been opened
                  MMTABLEHEADER   mmHeader;       // header for this table
                  HANDLE          hEntrys;        // handle to array of entrys
} MMTABLE;
typedef MMTABLE UNALIGNED FAR *LPMMTABLE;

typedef struct uniquemap_tag {
                  BYTE    szName[MAX_UNIQUENAME];// unique map or port name
                  DWORD   dwOffset;               // offset from base or device ID
                  HANDLE  hNext;                  // next one
} UNIQUEMAP;
typedef UNIQUEMAP UNALIGNED FAR *LPUNIQUEMAP;

#pragma pack()

/*-=-=-=-=- Function Prototypes -=-=-=-=-*/

#define STATIC /**/
STATIC MMAPERR  NEAR PASCAL     MmaperrAddMap(UINT, LPVOID, LPDWORD);
STATIC MMAPERR  NEAR PASCAL     MmaperrChangeUsing(UINT, UINT, int);
STATIC MMAPERR  NEAR PASCAL     MmaperrEnumPorts(ENUMPROC, UINT, HWND, LPSTR);
STATIC MMAPERR  NEAR PASCAL     MmaperrFileAccess(int, int);
STATIC VOID     NEAR PASCAL     VFreeUniqueList(LPHANDLE);
STATIC VOID     NEAR PASCAL     VFreeTable(UINT);
STATIC MMAPERR  NEAR PASCAL     MmaperrGarbage(UINT);
STATIC DWORD    NEAR PASCAL     DwGetMapSize(UINT, LPSTR);
STATIC DWORD    NEAR PASCAL     DwGetSetupSize(LPSTR);
STATIC LPMMTABLEENTRY NEAR PASCAL LpGetTableEntry(HANDLE, LPSTR);
STATIC LPSTR    NEAR PASCAL     LszGetUniqueAtOffset (DWORD, HANDLE);
STATIC DWORD    NEAR PASCAL     LiNotUnique(LPSTR, LPHANDLE, DWORD);
STATIC MMAPERR  NEAR PASCAL     MmaperrReadKeymap (LPSTR, LPMIDIKEYMAP);
STATIC DWORD    NEAR PASCAL     DwReadPatchmap(LPSTR, LPMIDIPATCHMAP, BOOL);
STATIC MMAPERR  NEAR PASCAL     MmaperrReadSetup(LPSTR, LPMIDIMAP);
STATIC MMAPERR  NEAR PASCAL     MmaperrReadTable(UINT);
STATIC MMAPERR  NEAR PASCAL     MmaperrWriteKeymap(LPMIDIKEYMAP);
STATIC MMAPERR  NEAR PASCAL     MmaperrWritePatchmap(PATCHMAP FAR*);
STATIC MMAPERR  NEAR PASCAL     MmaperrWriteSetup (SETUP FAR*);
STATIC MMAPERR  NEAR PASCAL     MmaperrWriteTabEntry(UINT,
                                        UINT, LPMMTABLEENTRY);
STATIC MMAPERR  NEAR PASCAL     MmaperrWriteTabHeader(UINT, LPMMTABLEHEADER);

/*-=-=-=-=- Global Constants    -=-=-=-=-*/
TCHAR RegEntry[] =
    TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Midimap");
TCHAR RegCurrent[] = TEXT("Mapping Name");

/*-=-=-=-=- Global Variables    -=-=-=-=-*/

char aszMapperPath[MAXPATHLEN+1]; // A Path Buffer to mapper file (real or temp)

static HANDLE   hPortList,      // Unique port names list.
                hPatchList,     // Unique patchmaps list.
                hKeyList;       // Unique keymaps list.
static HANDLE   hSetupTable,    // Setup table handle.
                hPatchTable,    // Patch table handle.
                hKeyTable;      // Key table handle.
static HFILE    iFile = HFILE_ERROR;    // File handle.
static UINT     ucFileOpen;     // times the (.CFG?) file has been opened

static BOOL     fEditing;

//      -       -       -       -       -       -       -       -       -

void FAR PASCAL mapConnect(LPSTR lpszTmpPath)
{
        // mapFileVersion will force the mapper to open the file
        // so we can change the filename from under it anyway
        lstrcpy(aszMapperPath,lpszTmpPath);
        fEditing = TRUE;
}

void FAR PASCAL mapDisconnect(void)
{
        // We can't leave the temporary file open.  Otherwise someone
        // might reference it after the applet disconnects.

        if (iFile != HFILE_ERROR)
        {
                ucFileOpen = 1; // Force it to close
                (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
        }

        fEditing = FALSE;
}

/*
 * @doc INTERNAL
 *
 * @api int | mapFileVersion | This function obtains the version number of the
 *      user's midi data file.
 *
 * @rdesc Returns a DWORD, the high word of which is an error code.  If the
 *      error code is MMAPERR_SUCCESS, the low word contains the version
 *      number.
 */

DWORD FAR PASCAL mapFileVersion (void)
{
        MMHEADER        mmHeader;
        DWORD   dwRet;
        MMAPERR mmaperr;

        // This kludge is here in case MMSYSTEM crashes inside one of the
        // midimap functions that has the file open. In this case, iFile
        // will be non-null and the cpl applet will think there is no
        // midimap.cfg file.

        hPortList = NULL;
        hPatchList = NULL;
        hKeyList = NULL;
        hSetupTable = NULL;
        hPatchTable = NULL;
        hKeyTable = NULL;
        iFile = HFILE_ERROR;
        ucFileOpen = 0;
        mmaperr = MmaperrFileAccess(MAP_FOPEN,OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)
                return MAKELONG(0, mmaperr);

        dwRet = ( _lread(iFile, (LPSTR)&mmHeader,sizeof(MMHEADER))
                    != sizeof(MMHEADER)
                )
                ? MAKELONG(0, MMAPERR_READ)
                : MAKELONG(mmHeader.wVersion, MMAPERR_SUCCESS);
        (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
        return dwRet;
} /* mapFileVersion */

//      -       -       -       -       -       -       -       -       -

/*
 * @doc INTERNAL
 *
 * @api BOOL | mapInitMapFile | This function initializes the user's
 *      midi data file.  It creates a file in the user's windows directory,
 *      and fills it with the necessary header information.  If the file
 *      exists, it will be truncated to zero length first.
 *
 * @rdesc Returns non-zero if successful, otherwise zero.
 */

//      Issues:
//
//      1.      Need to install "DeleteFile" code.

MMAPERR FAR PASCAL mapInitMapFile (void)
{
        SETUP           setup;
        MMHEADER        mmHeader;
        MMTABLEHEADER   mmtHeader;
        MMTABLEENTRY    mmtEntry;
        MMAPERR mmaperr;
        WORD            wNum        ;

        if ((mmaperr = MmaperrFileAccess(MAP_FCREATE,
                0)) != MMAPERR_SUCCESS)
                return mmaperr;
        mmHeader.wVersion = MM_VERSION;
        mmHeader.dwGarbage = 0L;
        mmHeader.idxCurSetup = 0;       // Set later in mapSetCurrentSetup
        mmHeader.oSetup = (wNum         = sizeof(MMHEADER));
        mmHeader.oPatch = ( wNum += sizeof(MMTABLEHEADER) + MM_NUMSETUP*sizeof(MMTABLEENTRY) );
        mmHeader.oKey = (wNum += sizeof(MMTABLEHEADER) + MM_NUMPATCH*sizeof(MMTABLEENTRY) );
        if (_lwrite(iFile, (LPSTR)&mmHeader, sizeof(MMHEADER))
           != sizeof(MMHEADER)
           ) {
                mmaperr = MMAPERR_WRITE;
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
exit01:         // Delete file here.
                return mmaperr;
        }
        _fmemset(&mmtEntry, 0, sizeof(MMTABLEENTRY));
        mmtHeader.wUsed = 0;
        {   UINT i;
            for (i = 0; i < 3; i++) {
                    switch (i) {
                    case 0:
                            mmtHeader.wEntrys = MM_NUMSETUP;
                            break;
                    case 1:
                            mmtHeader.wEntrys = MM_NUMPATCH;
                            break;
                    case 2:
                            mmtHeader.wEntrys = MM_NUMKEY;
                            break;
                    }
                    if (_lwrite(iFile, (LPSTR)&mmtHeader, sizeof(MMTABLEHEADER))
                       != sizeof(MMTABLEHEADER)
                       ) {
                            mmaperr = MMAPERR_WRITE;
                            goto exit00;
                    }
                    for (wNum         = mmtHeader.wEntrys; wNum        ; wNum        --)
                            if (_lwrite(iFile, (LPSTR)&mmtEntry,
                                    sizeof(MMTABLEENTRY)) !=
                                    sizeof(MMTABLEENTRY)) {
                                    mmaperr = MMAPERR_WRITE;
                                    goto exit00;
                            }
            }
        }
        _fmemset(&setup, 0, sizeof(SETUP));
        LoadString( hLibInst
                  , IDS_VANILLANAME
                  , setup.aszSetupName
                  , sizeof(setup.aszSetupName)
                  );
        LoadString( hLibInst
                  , IDS_VANILLADESC
                  , setup.aszSetupDescription
                  , sizeof(setup.aszSetupDescription)
                  );
        {   UINT wChan;
            for (wChan = 0; wChan < 16; wChan++) {
                    setup.channels[wChan].wDeviceID = MMAP_ID_NOPORT;
                    setup.channels[wChan].wChannel = (WORD)wChan;
            }
        }
        if ((mmaperr = MmaperrWriteSetup(&setup)) != MMAPERR_SUCCESS)
                goto exit00;
        if ((mmaperr = mapSetCurrentSetup(MAKEID(1))) != MMAPERR_SUCCESS)
                goto exit00;
        if ((mmaperr = MmaperrFileAccess(MAP_FCLOSE, 0)) != MMAPERR_SUCCESS)
                goto exit01;            // 01, not 00.
        return MMAPERR_SUCCESS;
} /* mapInitMapFile */

//      -       -       -       -       -       -       -       -       -

/*
 * @doc INTERNAL
 *
 * @api UINT | mapSetCurrentSetup | This function sets the name of the current
 *      setup.
 *
 * @parm LPSTR | lpSetupName | Specifies the name of a setup which you
 *      want to be the current setup.
 *
 * @rdesc The return value is currently undefined.
 */

//      Issues:
//
//      1.      None.

MMAPERR FAR PASCAL mapSetCurrentSetup (LPSTR lpSetupName)
{
        LPMMTABLEENTRY  lpmmEntry;
        MMHEADER        mmHeader;
        MMAPERR mmaperr;

        HKEY    hKey;
        LONG    lRet;

        /*
        ** See if the information is stored in the registry.
        ** If so write the stuff there.  Otherwise fall thru and try to
        ** write the stuff into the mapper file.
        */
        lRet = RegOpenKey( HKEY_LOCAL_MACHINE, RegEntry, &hKey );

        if ( lRet == ERROR_SUCCESS ) {

              lRet = RegSetValueEx( hKey, RegCurrent, 0L, REG_SZ,
                                    (LPBYTE)lpSetupName,
                                    sizeof(TCHAR) * (1 + lstrlen(lpSetupName)));
              RegCloseKey( hKey );

              if ( lRet == ERROR_SUCCESS) {
                  return MMAPERR_SUCCESS;
              }
        }

        if ((mmaperr = MmaperrFileAccess(MAP_FOPEN,
                OF_READWRITE)) != MMAPERR_SUCCESS)
                return mmaperr;
        if ((mmaperr = MmaperrReadTable(MMAP_SETUP)) != MMAPERR_SUCCESS) {
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        lpmmEntry = LpGetTableEntry(hSetupTable,
                lpSetupName);
        if (_llseek(iFile, 0L, 0) == -1L) {
                mmaperr = MMAPERR_READ;
exit01:         VFreeTable(MMAP_SETUP);
                goto exit00;
        }
        if (_lread(iFile, (LPSTR)&mmHeader,
                sizeof(MMHEADER)) != sizeof(MMHEADER)) {
                mmaperr = MMAPERR_READ;
                goto exit01;
        }
        mmHeader.idxCurSetup = lpmmEntry->idxEntry;
        if (_llseek(iFile, 0L, 0) == -1L) {
                mmaperr = MMAPERR_WRITE;
                goto exit01;
        }
        if (_lwrite(iFile, (LPSTR)&mmHeader,
                sizeof(MMHEADER)) != sizeof(MMHEADER)) {
                mmaperr = MMAPERR_WRITE;
                goto exit01;
        }
        VFreeTable(MMAP_SETUP);
        if ((mmaperr = MmaperrFileAccess(MAP_FCLOSE, 0)) != MMAPERR_SUCCESS)
                return mmaperr;
        return MMAPERR_SUCCESS;
} /* mapSetCurrentSetup */

//      -       -       -       -       -       -       -       -       -

/*
 * @doc INTERNAL
 *
 * @api LPSTR | mapGetCurrentSetup | This function retrieves the name of the
 * current setup.
 *
 * @parm LPSTR | lpBuf | Specifies a buffer into which the current setup
 *      name will be copied.
 *
 * @parm UINT | wSize | Specifies the size in bytes of <p lpBuf>.
 *
 * @rdesc Returns an MMAP error, or zero on success.
 *
 * @comm You should make sure that the supplied buffer and size are at
 *      least MMAP_MAXNAME characters (defined in MMSYSTEM.H).
 */

//      Issues:
//
//      1.      None.

MMAPERR FAR PASCAL mapGetCurrentSetup(
        LPSTR   lpBuf,
        UINT    uSize)
{
        LPMMTABLEENTRY  lpmmEntry;
        MMHEADER        mmHeader;
        MMAPERR mmaperr;
        LPSTR           lpEntryName;
        LPSTR           lp;

        HKEY    hKey;
        LONG    lRet;

        /*
        ** See if the information is stored in the registry.
        ** If so read the stuff from there.  Otherwise fall thru and try to
        ** get the stuff from the mapper file.
        */
        lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                             RegEntry,
                             0L,
                             KEY_QUERY_VALUE,
                             &hKey );

        if (lRet == ERROR_SUCCESS) {

            DWORD  dwType, dwLen;

            dwLen = uSize;

            lRet = RegQueryValueEx( hKey, RegCurrent, 0L, &dwType,
                                    (LPBYTE)lpBuf, &dwLen );
            RegCloseKey( hKey );

            if ( lRet == ERROR_SUCCESS) {
                return MMAPERR_SUCCESS;
            }
        }

        /* attempt to open file, read setup table */
        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        mmaperr = MmaperrReadTable(MMAP_SETUP);
        if (mmaperr != MMAPERR_SUCCESS) {
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        // read the file header
        if (_llseek(iFile, 0L, 0) == -1L) {
                mmaperr = MMAPERR_READ;
exit01:         VFreeTable(MMAP_SETUP);
                goto exit00;
        }
        if (_lread(iFile, (LPSTR)&mmHeader, sizeof(MMHEADER))
           != sizeof(MMHEADER)
           ) {
                mmaperr = MMAPERR_READ;
                goto exit01;
        }
        lpmmEntry = LpGetTableEntry(hSetupTable, MAKEID(mmHeader.idxCurSetup)); // Always succeed.
        // lstrncmp (lpBuf, lpmmEntry->szName, uSize - 1);
        for (lp = lpBuf, lpEntryName = (LPSTR)(lpmmEntry->szName); --uSize; )
                *lp++ = *lpEntryName++;
        *lp = '\0';
        // free table, close file and leave
        VFreeTable(MMAP_SETUP);
        (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
        return MMAPERR_SUCCESS;
} /* mapGetCurrentSetup */

//      -       -       -       -       -       -       -       -       -

MMAPERR FAR PASCAL mapReadSetup(
        LPSTR   lszSetupName,
        SETUP FAR*      lpSetup)
{
        LPMMCHANNEL     lpmmChan;
        LPMMTABLEENTRY  lpmmEntry;
        MMSETUP mmSetup;
        MMAPERR mmaperr;
        UINT    wNumDevs;
        BOOL    fNoPort;
        int     i;
        UINT    wDev;

        // open file, read in pertinent tables
        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)  {
                return mmaperr;
        }
        fNoPort = FALSE;
        mmaperr = MmaperrReadTable(MMAP_SETUP | MMAP_PATCH | MMAP_KEY);
        if (mmaperr != MMAPERR_SUCCESS) {
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        lpmmEntry = LpGetTableEntry(hSetupTable, lszSetupName);
        if (lpmmEntry == NULL) {
                mmaperr = MMAPERR_INVALIDSETUP;
exit01:         VFreeTable(MMAP_SETUP | MMAP_PATCH | MMAP_KEY);
                goto exit00;
        }
        // read in setup data from file
        if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                mmaperr = MMAPERR_READ;
                goto exit01;
        }
        if (_lread(iFile, (LPSTR)&mmSetup, sizeof(MMSETUP)) != sizeof(MMSETUP)){
                mmaperr = MMAPERR_READ;
                goto exit01;
        }
        // copy over setup name, description, and flags
        lstrcpy(lpSetup->aszSetupName, (LPCSTR)(lpmmEntry->szName));
        lstrcpy(lpSetup->aszSetupDescription, (LPCSTR)(lpmmEntry->szDesc));
        lpSetup->dFlags = mmSetup.dwFlags;
        // grab the number of devices in the current environment
        wNumDevs = midiOutGetNumDevs();
        // set up a couple of optimization-pointers
        lpmmChan = (LPMMCHANNEL)(mmSetup.chMap); // MIPS silliness
        for (i = 0; i < 16; i++, lpmmChan++) {
                // convert device name to device ID.  If device name doesn't
                // exist in the current environment, set ID to MMAP_ID_NOPORT
                if (!*lpmmChan->szDevice)
                        wDev = wNumDevs;
                else
                    for (wDev = 0; wDev < wNumDevs; wDev++) {
                        MIDIOUTCAPS     moCaps;

                        midiOutGetDevCaps(wDev, &moCaps, sizeof(MIDIOUTCAPS));
                        if (!lstrcmpi( moCaps.szPname
                                     , (LPCSTR)(lpmmChan->szDevice)))
                                    break;
                    }
                // copy over channel and flag info
                lpSetup->channels[i].wChannel = lpmmChan->wChannel;
                lpSetup->channels[i].dFlags = lpmmChan->dwFlags;
                if (wDev < wNumDevs)
                        lpSetup->channels[i].wDeviceID = (WORD)wDev;
                else {
                        lpSetup->channels[i].wDeviceID = MMAP_ID_NOPORT;
                        // this error code only if there was a port
                        // name but it does not exist on the current system.
                        if (*lpmmChan->szDevice)
                                fNoPort = TRUE;
                }
                // if channel has no patchmap then on to next channel
                if (!lpmmChan->idxPMapName) {
                        lstrcpy(lpSetup->channels[i].aszPatchName, szNone);
                        continue;
                }
                lpmmEntry = LpGetTableEntry(hPatchTable, MAKEID(lpmmChan->idxPMapName));
                lstrcpy( lpSetup->channels[i].aszPatchName
                       , (LPCSTR)(lpmmEntry->szName));
        }
        VFreeTable(MMAP_SETUP | MMAP_PATCH | MMAP_KEY);
        MmaperrFileAccess(MAP_FCLOSE, 0);
        if (fNoPort)
                return MMAPERR_INVALIDPORT;
        return MMAPERR_SUCCESS;
}

MMAPERR FAR PASCAL mapReadPatchMap(
        LPSTR   lszPatchMapName,
        PATCHMAP FAR*   lpPatch)
{
        LPMMTABLEENTRY  lpmmEntry;
        MMPATCH mmPatch;
        UNALIGNED WORD  *lpwKey;
        MMAPERR mmaperr;
        int     i;

        // open file, read in pertinent tables
        mmaperr = MmaperrFileAccess(MAP_FOPEN,OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        mmaperr = MmaperrReadTable(MMAP_PATCH | MMAP_KEY);
        if (mmaperr != MMAPERR_SUCCESS) {
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        lpmmEntry = LpGetTableEntry(hPatchTable, lszPatchMapName);
        if (lpmmEntry == NULL) {
                mmaperr = MMAPERR_INVALIDPATCH;
exit01:         VFreeTable(MMAP_PATCH | MMAP_KEY);
                goto exit00;
        }
        // read in patch data from file
        if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                mmaperr = MMAPERR_READ;
                goto exit01;
        }
        if ( _lread(iFile, (LPSTR)&mmPatch, sizeof(MMPATCH))
           != sizeof(MMPATCH)
           ) {
                mmaperr = MMAPERR_READ;
                goto exit01;
        }
        // copy data from file structure to memory structure
        lstrcpy(lpPatch->aszPatchMapName, (LPCSTR)(lpmmEntry->szName));
        lstrcpy(lpPatch->aszPatchMapDescription, (LPCSTR)(lpmmEntry->szDesc));
        // set up an optimization pointer
        lpwKey = mmPatch.idxKMapNames;
        for (i = 0; i < MIDIPATCHSIZE; i++, lpwKey++) {
                lpPatch->keymaps[i].bVolume = HIBYTE(mmPatch.wPMap[i]);
                lpPatch->keymaps[i].bDestination = LOBYTE(mmPatch.wPMap[i]);
                if (!*lpwKey) {
                        lstrcpy(lpPatch->keymaps[i].aszKeyMapName, szNone);
                        continue;
                }
                lpmmEntry = LpGetTableEntry(hKeyTable, MAKEID(*lpwKey));
                lstrcpy( lpPatch->keymaps[i].aszKeyMapName
                       , (LPCSTR)(lpmmEntry->szName));
        }
        // free table, close file and leave
        VFreeTable(MMAP_PATCH | MMAP_KEY);
        (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
        return MMAPERR_SUCCESS;
}

//      -       -       -       -       -       -       -       -       -

/* @doc INTERNAL
 *
 * @api DWORD | mapRead | Read a map specified by <p lpName> into a buffer
 *      specified by <p lpvBuf>.  This includes any patchmaps or keymaps
 *      that the map may contain.
 *
 * @parm UINT | wFlag | Specifies the type of map to be read into memory.
 *      It may be any of the following:
 *
 *      @flag MMAP_SETUP | Read a setup.
 *      @flag MMAP_PATCH | Read a patchmap.
 *      @flag MMAP_KEY | Read a keymap.
 *
 * @parm LPSTR | lpName | Specifies the name of the map of which you
 *      want read into memory.
 *
 * @rdesc Returns an MMAP error code, or zero on success.
 */

//      Issues:
//
//      1.      None.

MMAPERR FAR PASCAL mapRead(
        UINT    uFlag,
        LPSTR   lpName,
        LPVOID  lpvBuf)
{
        switch (uFlag) {
                DWORD   dwRet;

        case MMAP_SETUP:
                return MmaperrReadSetup(lpName, (LPMIDIMAP)lpvBuf);
        case MMAP_PATCH:
                dwRet = DwReadPatchmap(lpName,(LPMIDIPATCHMAP)lpvBuf, FALSE);
                if (dwRet < MMAPERR_MAXERROR)
                        return LOWORD(dwRet);
                return MMAPERR_SUCCESS;
        case MMAP_KEY:
                return MmaperrReadKeymap(lpName, (LPMIDIKEYMAP)lpvBuf);
        }
} /* mapRead */

//      -       -       -       -       -       -       -       -       -

//      MmaperrReadSetup
//
//      Read a setup into a buffer.  This includes any patchmaps or keymaps
//      that the setup may contain.
//
//      NOTE:  This function will return MMAPERR_INVALIDPORT if the setup to
//      be read accesses ports that are not available on the system.  That is,
//      if it accesses ports not listed as a 'midix=xxx.drv' entry under the
//      [drivers] section in system.ini.  The entire setup WILL BE read into
//      memory but the uDeviceID for the channels with inaccessible ports will
//      be set to MMAP_ID_NOPORT.  This error also has NO EFFECT on the
//      active/inactive status of any such channels.
//
//      Issues:
//
//      1.      There's a comment about something breaking a segment
//              boundary in here, but if it does break the segment boundary
//              the way it's dealt with it looks like it will wipe out.
//
//      2.      Freeing the "hKeyList" chain may be unnecessary if the
//              patch map read routine cleans up after itself properly.

STATIC  MMAPERR NEAR PASCAL MmaperrReadSetup(
        LPSTR   lpName,
        LPMIDIMAP       lpmMap)
{
        LPMIDICHANNELMAP lpChan;
        LPMMCHANNEL     lpmmChan;
        LPMIDIPATCHMAP  lpPatch;
        LPMMTABLEENTRY  lpmmEntry;
        MMSETUP mmSetup;
        MIDIOUTCAPS     moCaps;
        MMAPERR mmaperr;
        DWORD   dwOffset;
        UINT    wNumDevs;
        BOOL    fNoPort;
        int     i;
        UINT    wDev;

        // open file, read in pertinent tables
        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        fNoPort = FALSE;
        mmaperr = MmaperrReadTable(MMAP_SETUP | MMAP_PATCH | MMAP_KEY);
        if (mmaperr != MMAPERR_SUCCESS) {
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        dwOffset = sizeof(MIDIMAP);
        lpmmEntry = LpGetTableEntry(hSetupTable, lpName);
        if (lpmmEntry == NULL) {
                mmaperr = MMAPERR_INVALIDSETUP;
exit01:         VFreeTable(MMAP_SETUP | MMAP_PATCH | MMAP_KEY);
                goto exit00;
        }
        // read in setup data from file
        if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                mmaperr = MMAPERR_READ;
                goto exit01;
        }
        if (_lread(iFile, (LPSTR)&mmSetup, sizeof(MMSETUP)) != sizeof(MMSETUP)){
                mmaperr = MMAPERR_READ;
                goto exit01;
        }
        // copy over setup name, description, and flags
        lstrcpy(lpmMap->szName, (LPCSTR)(lpmmEntry->szName));
        lstrcpy(lpmMap->szDesc, (LPCSTR)(lpmmEntry->szDesc));
        lpmMap->dwFlags = mmSetup.dwFlags;
        // grab the number of devices in the current environment
        wNumDevs = midiOutGetNumDevs();
        // set up a couple of optimization-pointers
        lpChan = lpmMap->chMap;
        lpmmChan = (LPMMCHANNEL)(mmSetup.chMap); // MIPS silliness
        for (i = 0; i < 16; i++, lpChan++, lpmmChan++) {
                // convert device name to device ID.  If device name doesn't
                // exist in the current environment, set ID to MMAP_ID_NOPORT
                if (!*lpmmChan->szDevice)
                        wDev = wNumDevs;
                else
                        for (wDev = 0; wDev < wNumDevs; wDev++) {
                                midiOutGetDevCaps(wDev, &moCaps, sizeof(MIDIOUTCAPS));
                                if (!lstrcmpi(moCaps.szPname,
                                              (LPCSTR)(lpmmChan->szDevice)))
                                        break;
                        }
                // copy over channel and flag info
                lpChan->wChannel = lpmmChan->wChannel;
                lpChan->dwFlags = lpmmChan->dwFlags;
                if (wDev < wNumDevs)
                        lpChan->wDeviceID = (WORD)wDev;
                else {
                        lpChan->wDeviceID = MMAP_ID_NOPORT;
                        // this error code only if there was a port
                        // name but it does not exist on the current system.
                        if (*lpmmChan->szDevice)
                                fNoPort = TRUE;
                }
                // if channel has no patchmap then on to next channel
                if (!lpmmChan->idxPMapName)
                        continue;
                // channel has a patchmap - if its not unique, point offset
                // to first occurance, otherwise offset = dwOffset.
                // assuming this patchmap ID is valid!
                lpmmEntry = LpGetTableEntry(hPatchTable,
                        MAKEID(lpmmChan->idxPMapName));
                lpChan->oPMap = LiNotUnique( (LPSTR)(lpmmEntry->szName)
                                           , &hPatchList
                                           , dwOffset
                                           );
                if (lpChan->oPMap == -1L) {
                        mmaperr = MMAPERR_MEMORY;
exit02:                 VFreeUniqueList(&hPatchList);
                        VFreeUniqueList(&hKeyList);
                        goto exit01;
                } else if (!lpChan->oPMap) {
                        DWORD   dwRet;

                        lpChan->oPMap = dwOffset;
                        // setup patchmap pointer; could break segment bounds
                        lpPatch = (LPMIDIPATCHMAP)((LPSTR)lpmMap + dwOffset);
                        // read in patchmap (this also updates global offset)
                        dwRet = DwReadPatchmap(0L, lpPatch, TRUE);
                        if (dwRet < MMAPERR_MAXERROR) {
                                mmaperr = LOWORD(dwRet);
                                goto exit02;
                        }
                        dwOffset += dwRet;
                }
        }
        VFreeUniqueList(&hPatchList);
        VFreeUniqueList(&hKeyList);
        VFreeTable(MMAP_SETUP | MMAP_PATCH | MMAP_KEY);
        (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
        if (fNoPort)
                return MMAPERR_INVALIDPORT;
        return MMAPERR_SUCCESS;
} /* MmaperrReadSetup */

//      -       -       -       -       -       -       -       -       -

//      DwReadPatchmap
//
//      Read a patchmap into a buffer.  This includes any keymaps the patchmap
//      may contain.
//
//      Remarks:
//
//      1.      Use of "fInSetup" flag is very bogus, may cause problems, is
//              probably wrong.

STATIC  DWORD NEAR PASCAL DwReadPatchmap(
        LPSTR   lpName,
        LPMIDIPATCHMAP  lpPatch,
        BOOL    fInSetup)
{
        LPMMTABLEENTRY  lpmmEntry;
        LPMIDIKEYMAP    lpKey;
        MMPATCH mmPatch;
        UNALIGNED WORD *lpwKey;
        MMAPERR mmaperr;
        DWORD   dwOffset;
        int     i;

        // open file, read in pertinent tables
        mmaperr = MmaperrFileAccess(MAP_FOPEN,OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)
                return MAKELONG(mmaperr, 0);
        mmaperr = MmaperrReadTable(MMAP_PATCH |MMAP_KEY);
        if (mmaperr != MMAPERR_SUCCESS) {
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return MAKELONG(mmaperr, 0);
        }
        // save global offset then initialize it to patch size
        dwOffset = sizeof(MIDIPATCHMAP);
        lpmmEntry = LpGetTableEntry(hPatchTable, lpName);
        if (lpmmEntry == NULL) {
                mmaperr = MMAPERR_INVALIDPATCH;
exit01:         VFreeTable(MMAP_PATCH | MMAP_KEY);
                goto exit00;
        }
        // read in patch data from file
        if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                mmaperr = MMAPERR_READ;
                goto exit01;
        }
        if (_lread(iFile, (LPSTR)&mmPatch,sizeof(MMPATCH)) != sizeof(MMPATCH)) {
                mmaperr = MMAPERR_READ;
                goto exit01;
        }
        // copy data from file structure to memory structure
        lstrcpy(lpPatch->szName, (LPCSTR)(lpmmEntry->szName));
        lstrcpy(lpPatch->szDesc, (LPCSTR)(lpmmEntry->szDesc));
        _fmemcpy((LPSTR)lpPatch->wPMap, (LPSTR)mmPatch.wPMap, MIDIPATCHSIZE * sizeof(WORD));
        _fmemset((LPSTR)lpPatch->okMaps, 0, MIDIPATCHSIZE * sizeof(DWORD));
        lpPatch->dwFlags = mmPatch.dwFlags;
        lpPatch->bVMax = mmPatch.bVMax;
        // set up an optimization pointer
        lpwKey = mmPatch.idxKMapNames;
        for (i = 0; i < MIDIPATCHSIZE; i++, lpwKey++) {
            // if no keymap for this patch then continue
            if (!*lpwKey)
                    continue;
            // patch has a keymap - if its unique point offset to
            // to first occurance, otherwise offset = dwOffset.
            // assuming this keymap ID is valid!
            lpmmEntry = LpGetTableEntry(hKeyTable, MAKEID(*lpwKey));
            lpPatch->okMaps[i] = LiNotUnique( (LPSTR)(lpmmEntry->szName)
                                            , &hKeyList, dwOffset);
            if (lpPatch->okMaps[i] == -1L) {
                mmaperr = MMAPERR_MEMORY;
                goto exit01;
            } else if (!lpPatch->okMaps[i]) {
                lpPatch->okMaps[i] = dwOffset;
                // set the keymap pointer; could break segment bounds
                lpKey = (LPMIDIKEYMAP)((LPSTR)lpPatch + dwOffset);
                // read in the keymap, update global offset
                mmaperr = MmaperrReadKeymap(0L, lpKey);
                if (mmaperr != MMAPERR_SUCCESS) {
                    VFreeUniqueList(&hKeyList);
                    goto exit01;
                }
                dwOffset += sizeof(MIDIKEYMAP);
            }
        }
        if (!fInSetup) {
                // if we're not called from ReadSetup, free the unique
                // keymap name list.
                VFreeUniqueList(&hKeyList);
        }
        // free table, close file and leave
        VFreeTable(MMAP_PATCH | MMAP_KEY);
        (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
        return dwOffset;
} /* DwReadPatchmap */

//      -       -       -       -       -       -       -       -       -

//      MmaperrReadKeymap
//
//      Read a keymap into a buffer.
//
//      Remarks:
//
//      1.      None.

STATIC  MMAPERR NEAR PASCAL MmaperrReadKeymap(
        LPSTR   lpName,
        LPMIDIKEYMAP    lpKey)
{
        LPMMTABLEENTRY  lpmmEntry;
        MMKEY           mmKey;
        MMAPERR mmaperr;

        // open file, read in pertinent tables
        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        mmaperr = MmaperrReadTable(MMAP_KEY);
        if (mmaperr != MMAPERR_SUCCESS) {
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        // assuming keymap exists
        lpmmEntry = LpGetTableEntry(hKeyTable, lpName);
        // read in key data from file
        if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                mmaperr = MMAPERR_READ;
exit01:         VFreeTable(MMAP_KEY);
                goto exit00;
        }
        if (_lread(iFile, (LPSTR)&mmKey, sizeof(mmKey)) != sizeof(mmKey)) {
                mmaperr = MMAPERR_READ;
                goto exit01;
        }
        // copy data from file structure to memory structure
        lstrcpy(lpKey->szName, (LPCSTR)(lpmmEntry->szName));
        lstrcpy(lpKey->szDesc, (LPCSTR)(lpmmEntry->szDesc));
        _fmemcpy(lpKey->bKMap, (LPCSTR)(mmKey.bKMap), MIDIPATCHSIZE * sizeof(BYTE));
        lpKey->dwFlags = mmKey.dwFlags;
        // free table, close file and leave
        VFreeTable(MMAP_KEY);
        (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
        return MMAPERR_SUCCESS;
} /* MmaperrReadKeymap */

//      -       -       -       -       -       -       -       -       -

/* @doc INTERNAL
 *
 * @api DWORD | mapWrite | Write a map specified by <p lpvMap> to the
 *      midi data file.
 *
 * @parm UINT | uFlag | Specifies the type of map to write.
 *      It may be any of the following:
 *
 *      @flag MMAP_SETUP | Write a setup.
 *      @flag MMAP_PATCH | Write a patchmap.
 *      @flag MMAP_KEY   | Write a keymap.
 *
 * @parm LPVOID | lpvMap | Specifies the map to write to the midi data file.
 *
 * @rdesc Returns non-zero if it could do the write, zero if it failed.
 */

//      Remarks:
//
//      1.      None.

MMAPERR FAR PASCAL mapWrite(
        UINT    uFlag,
        LPVOID  lpvMap)
{
        switch (uFlag) {
        case MMAP_SETUP :
                return MmaperrWriteSetup((SETUP FAR*)lpvMap);
        case MMAP_PATCH :
                return MmaperrWritePatchmap((PATCHMAP FAR*)lpvMap);
        case MMAP_KEY :
                return MmaperrWriteKeymap((LPMIDIKEYMAP)lpvMap);
        }
} /* mapWrite */

//      -       -       -       -       -       -       -       -       -

//      MmaperrWriteSetup
//
//      Write a setup to the midi data file.

STATIC  MMAPERR NEAR PASCAL MmaperrWriteSetup(SETUP FAR* lpSetup)
{
        HANDLE          hPatchUsage;
        LPMMTABLE       lpmmTable;
        LPMMTABLEENTRY  lpmmEntry;
        LPMMCHANNEL     lpmmChan;
        LPBYTE          lpbPatchUsage = NULL;  // Kill spurious use before set diagnostic
        MMSETUP         mmOldSetup;
        MMSETUP         mmSetup;
        MIDIOUTCAPS     moCaps;
        MMAPERR mmaperr;
        DWORD   doData;
        UINT    wNumDevs;
        UINT    wOldDevs;
        UINT    uSize;
        BOOL    fExists;
        UINT    uIndex;
        UINT    wDev;

        // open file, read in pertinent tables
        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READWRITE);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        mmaperr = MmaperrReadTable(MMAP_SETUP | MMAP_PATCH);
        if (mmaperr != MMAPERR_SUCCESS) {
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        // check to see if setup name already exists
        fExists = FALSE;
        lpmmEntry = LpGetTableEntry(hSetupTable, lpSetup->aszSetupName);
        if (lpmmEntry != NULL) {
            // setup name exists.  if the description has changed, we
            // have to re-write the table entry with new description
            if (lstrcmpi((LPCSTR)(lpmmEntry->szDesc)
                        , lpSetup->aszSetupDescription)) {
                lstrcpy( (LPSTR)(lpmmEntry->szDesc)
                       , lpSetup->aszSetupDescription);
                mmaperr = MmaperrWriteTabEntry(MMAP_SETUP, 0, lpmmEntry);
                if (mmaperr != MMAPERR_SUCCESS) {
exit01:             VFreeTable(MMAP_SETUP | MMAP_PATCH);
                    goto exit00;
                }
            }
            doData = lpmmEntry->doData;
            fExists = TRUE;
        }
        // name non-existent, so add a new table entry
        else if ((mmaperr = MmaperrAddMap(MMAP_SETUP, (LPVOID)lpSetup, &doData))
                != MMAPERR_SUCCESS
                )
                goto exit01;
        // zero-out the new setup data structure
        _fmemset((LPSTR)&mmSetup, 0, sizeof(MMSETUP));
        // obtain the number of entrys in the patch table
        lpmmTable = (LPMMTABLE)GlobalLock(hPatchTable);
        uSize = lpmmTable->mmHeader.wEntrys;
        GlobalUnlock(hPatchTable);
        // if there are any patchmaps
        hPatchUsage = NULL;
        if (uSize) {
                // Create table which is 'number-of-patchmaps' in length
                hPatchUsage = GlobalAlloc(GHND, (LONG)uSize);
                if (hPatchUsage == NULL) {
                        mmaperr = MMAPERR_MEMORY;
                        goto exit01;
                }
                lpbPatchUsage = (LPBYTE)GlobalLock(hPatchUsage);
                // if this is not a new map
                if (fExists) {
                        // read in old setup data from file
                        if (_llseek(iFile, doData, 0) == -1L) {
                                mmaperr = MMAPERR_READ;
exit02:                         if (hPatchUsage != NULL) {
                                        GlobalUnlock(hPatchUsage);
                                        GlobalFree(hPatchUsage);
                                }
                                goto exit01;
                        }
                        if ( _lread(iFile, (LPSTR)&mmOldSetup,sizeof(MMSETUP))
                           != sizeof(MMSETUP)
                           ) {
                                mmaperr = MMAPERR_READ;
                                goto exit02;
                        }
                        // set LSB of usage table indices where old setup
                        // referenced any respective patchmaps
                        for (uIndex = 0; uIndex < 16; uIndex++)
                        {
                            UINT    u;
                            u = mmOldSetup.chMap[uIndex].idxPMapName;
                            if (u)
                                 lpbPatchUsage[u - 1] = LSB;
                        }
                }
        }
        // get the number of devices
        wNumDevs = wOldDevs = midiOutGetNumDevs();
        // enumerate any invalid ports from old setup this is not a new map
        if (fExists) {
                uIndex = 0;
                lpmmChan = (LPMMCHANNEL)(mmOldSetup.chMap); // MIPS silliness
        } else {
                uIndex = 16;
                lpmmChan = 0;                           // wasn't set. LKG
        }
        for (; uIndex < 16; uIndex++, lpmmChan++) {
            // find out if the port is in the current environment
            wDev = *lpmmChan->szDevice ? 0 : wNumDevs;
            for (; wDev < wNumDevs; wDev++) {
                    midiOutGetDevCaps(wDev, &moCaps, sizeof(MIDIOUTCAPS));
                    if (!lstrcmpi(moCaps.szPname, (LPCSTR)(lpmmChan->szDevice)))
                            break;
            }
            // if not, add the unique port name to the invalid port list
            if (wDev == wNumDevs) {
                DWORD   dwUniqueRet;

                // unique-list entry will save this offset
                dwUniqueRet = LiNotUnique( (LPSTR)(lpmmChan->szDevice)
                                         , &hPortList
                                         , (DWORD)wOldDevs);
                if (dwUniqueRet == -1L) {
                        mmaperr = MMAPERR_MEMORY;
exit03:                 VFreeUniqueList(&hPortList);
                        goto exit02;
                } else if (!dwUniqueRet)
                        wOldDevs++;
            }
        }
        // copy over the data for each channel
        lpmmChan = (LPMMCHANNEL)(mmSetup.chMap); // MIPS silliness
        for (uIndex = 0; uIndex < 16; uIndex++, lpmmChan++) {
                // convert the device ID to a device name.
                if (lpSetup->channels[uIndex].wDeviceID == MMAP_ID_NOPORT)
                        *lpmmChan->szDevice = 0;
                else if (lpSetup->channels[uIndex].wDeviceID >= wNumDevs)
                    lstrcpy( (LPSTR)(lpmmChan->szDevice)
                           , LszGetUniqueAtOffset( (DWORD)lpSetup->channels[uIndex].wDeviceID
                                                 , hPortList
                                                 )
                           );
                else {
                        midiOutGetDevCaps( lpSetup->channels[uIndex].wDeviceID
                                         , &moCaps
                                         , sizeof(MIDIOUTCAPS)
                                         );
                        lstrcpy((LPSTR)(lpmmChan->szDevice), moCaps.szPname);
                }
                // copy over channel number and flags.
                lpmmChan->wChannel = lpSetup->channels[uIndex].wChannel;
                lpmmChan->dwFlags = lpSetup->channels[uIndex].dFlags;
                if (lpmmChan->dwFlags & MMAP_PATCHMAP) {
                        // do I want a uniqueness-check in here?
                        // get the table entry
                        lpmmEntry = LpGetTableEntry(hPatchTable,
                                lpSetup->channels[uIndex].aszPatchName);
                        // set patch name table entry index
                        lpmmChan->idxPMapName = lpmmEntry->idxEntry;
                        // set MSB at current table index where
                        // this setup references a patchmap
                        if (uSize)
                                lpbPatchUsage[lpmmChan->idxPMapName - 1] |= MSB;
                }
                // not needed if MMAP_PATCHMAP isn't there, seems clean though
                else
                        lpmmChan->idxPMapName = 0;
        }
        // if there are any patchmaps
        if (uSize) {
            // Check for differences in patchmap referencing.  Compare
            // what patchmaps are now being used to what patchmaps were
            // being used before.  Change the using count accordingly.

            for (uIndex = 0; uIndex < uSize; uIndex++, lpbPatchUsage++)
                if ((*lpbPatchUsage & MSB) &&
                    (!(*lpbPatchUsage & LSB))) {
                    mmaperr = MmaperrChangeUsing(MMAP_PATCH,uIndex + 1, 1);
                    if (mmaperr != MMAPERR_SUCCESS)
                            goto exit03;
                } else if (! (*lpbPatchUsage & MSB)
                          && (*lpbPatchUsage & LSB)
                          )
                    mmaperr = MmaperrChangeUsing(MMAP_PATCH,uIndex + 1, -1);
                    if (mmaperr != MMAPERR_SUCCESS)
                            goto exit03;
            // nuke the usage table
            GlobalUnlock(hPatchUsage);
            GlobalFree(hPatchUsage);
            hPatchUsage = NULL;
        }
        // copy over the setup flags.
        mmSetup.dwFlags = lpSetup->dFlags;
        // write the setup
        if (_llseek(iFile, doData, 0) == -1L) {
                mmaperr = MMAPERR_WRITE;
                goto exit03;
        }
        if ( _lwrite(iFile, (LPSTR)&mmSetup, sizeof(MMSETUP))
           != sizeof(MMSETUP)
           ) {
                mmaperr = MMAPERR_WRITE;
                goto exit03;
        }
        // free the unique invalid port name list
        VFreeUniqueList(&hPortList);
        // free tables, close file and leave
        VFreeTable(MMAP_SETUP | MMAP_PATCH);
        mmaperr = MmaperrFileAccess(MAP_FCLOSE, 0);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        return MMAPERR_SUCCESS;
} /* MmaperrWriteSetup */

//      -       -       -       -       -       -       -       -       -

//      MmaperrWritePatchmap
//
//      Write a patchmap to the midi data file.

STATIC  MMAPERR NEAR PASCAL MmaperrWritePatchmap(
        PATCHMAP FAR*   lpPatch)
{
        KEYMAP FAR*     keymap;
        HANDLE          hKeyUsage;
        LPMMTABLE       lpmmTable;
        LPMMTABLEENTRY  lpmmEntry;
        MMPATCH mmOldPatch;
        MMPATCH mmPatch;
        UNALIGNED WORD *lpidxNames;
        LPBYTE  lpbKeyUsage = NULL;  // Kill spurious use before set diagnostic
        DWORD   doData;
        DWORD   dwOffset;
        UINT    uSize = 0;
        BOOL    fExists = FALSE;
        MMAPERR mmaperr;
        UINT    uIndex;

        // open file, read in pertinent tables
        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READWRITE);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        mmaperr = MmaperrReadTable(MMAP_PATCH | MMAP_KEY);
        if (mmaperr != MMAPERR_SUCCESS) {
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        // check to see if patch name already exists
        lpmmEntry = LpGetTableEntry(hPatchTable, lpPatch->aszPatchMapName);
        if (lpmmEntry != NULL) {
            // patch name exists.  if the description has changed, we
            // have to re-write the table entry with new description
            if (lstrcmpi( (LPSTR)(lpmmEntry->szDesc)
                        , lpPatch->aszPatchMapDescription)) {
                lstrcpy( (LPSTR)(lpmmEntry->szDesc)
                       , lpPatch->aszPatchMapDescription);
                mmaperr = MmaperrWriteTabEntry(MMAP_PATCH, 0, lpmmEntry);
                if (mmaperr != MMAPERR_SUCCESS) {
exit01:                 VFreeTable(MMAP_PATCH | MMAP_KEY);
                        goto exit00;
                }
            }
            doData = lpmmEntry->doData;
            fExists = TRUE;
        }
        // name is non-existent, so add a new table entry
        else {
                mmaperr = MmaperrAddMap(MMAP_PATCH,lpPatch, &doData);
                if (mmaperr != MMAPERR_SUCCESS)
                        goto exit01;
        }
        // zero-out the patch data structure.
        _fmemset((LPSTR)&mmPatch, 0, sizeof(MMPATCH));
        // obtain the number of entrys in the key table
        lpmmTable = (LPMMTABLE)GlobalLock(hKeyTable);
        uSize = lpmmTable->mmHeader.wEntrys;
        GlobalUnlock(hKeyTable);
        // Create table which is 'number-of-keymaps' in length
        hKeyUsage = NULL;
        if (uSize) {
                hKeyUsage = GlobalAlloc(GHND, (LONG)uSize);
                if (hKeyUsage == NULL) {
                        mmaperr = MMAPERR_MEMORY;
                        goto exit01;
                }
                lpbKeyUsage = (LPBYTE)GlobalLock(hKeyUsage);
        }
        // if not a new patchmap, we need old usage count
        if (fExists) {
                // read in old patch data from file
                if (_llseek(iFile, doData, 0) == -1L) {
                        mmaperr = MMAPERR_READ;
exit02:                 if (hKeyUsage != NULL) {
                                GlobalUnlock(hKeyUsage);
                                GlobalFree(hKeyUsage);
                        }
                        goto exit01;
                }
                if ( _lread(iFile, (LPSTR)&mmOldPatch, sizeof(MMPATCH))
                   != sizeof(MMPATCH)
                   ) {
                        mmaperr = MMAPERR_READ;
                        goto exit02;
                }
                mmPatch.wUsing = mmOldPatch.wUsing;
                // if there are keymaps and map is not new, set LSB
                // of usage table indices where old patchmap referenced
                // any respective keymaps
                if (uSize) {
                        lpidxNames = mmOldPatch.idxKMapNames;
                        for (uIndex = 0; uIndex < MIDIPATCHSIZE; uIndex++, lpidxNames++)
                                if (*lpidxNames)
                                        lpbKeyUsage[*lpidxNames - 1] = LSB;
                }
        }
        dwOffset = sizeof(MIDIPATCHMAP);
        // set max volume scalar to minimum value
        mmPatch.bVMax = 1;
        // copy over data for each patch entry
        keymap = lpPatch->keymaps;
        lpidxNames = mmPatch.idxKMapNames;
        for (uIndex = 0; uIndex < MIDIPATCHSIZE; uIndex++, keymap++, lpidxNames++) {
                DWORD   dwUniqueRet;

                mmPatch.wPMap[uIndex] = ((WORD)keymap->bVolume << 8) | keymap->bDestination;

                // if the current volume is greater than the max volume
                // then set max volume to it
                if (keymap->bVolume > mmPatch.bVMax)
                        mmPatch.bVMax = keymap->bVolume;

                // if patch has no keymap, zero out keymap table entry
                // index in file data structure and continue
                if (!lstrcmpi(keymap->aszKeyMapName, szNone)) {
                        *lpidxNames = 0;
                        continue;
                }
                // get the table entry
                lpmmEntry = LpGetTableEntry(hKeyTable, keymap->aszKeyMapName);
                // set keymap table entry index in file data structure
                *lpidxNames = lpmmEntry->idxEntry;
                // if keymap is unique, add size to global offset
                dwUniqueRet = LiNotUnique(keymap->aszKeyMapName, &hKeyList, dwOffset);
                if (dwUniqueRet == -1L) {
                        mmaperr = MMAPERR_MEMORY;
exit03:                 VFreeUniqueList(&hKeyList);
                        goto exit02;
                } else if (!dwUniqueRet) {
                        DWORD   dwSize;

                        dwSize = DwGetMapSize(MMAP_KEY,keymap->aszKeyMapName);
                        if (dwSize < MMAPERR_MAXERROR) {
                                mmaperr = LOWORD(dwSize);
                                goto exit03;
                        }
                        dwOffset += dwSize;
                        // set MSB at current usage table index where
                        // this patchmap references a keymap
                        if (uSize)
                                lpbKeyUsage[*lpidxNames - 1] |= MSB;
                }
        }
        // if there are any keymaps
        if (uSize) {
            // Check for differences in keymap referencing.  Compare
            // what keymaps are now being used to what keymaps were being
            // used before.  Change the using count accordingly.

            for (uIndex = 0; uIndex < uSize; uIndex++, lpbKeyUsage++)
                if ((*lpbKeyUsage & MSB) && (!(*lpbKeyUsage & LSB))) {
                    mmaperr = MmaperrChangeUsing(MMAP_KEY,uIndex + 1, 1);
                    if (mmaperr != MMAPERR_SUCCESS)
                            goto exit03;
                } else if (! (*lpbKeyUsage & MSB) &&
                    (*lpbKeyUsage & LSB))
                    mmaperr = MmaperrChangeUsing(MMAP_KEY, uIndex + 1, -1);
                    if (mmaperr != MMAPERR_SUCCESS)
                            goto exit03;
            // nuke the usage table
            GlobalUnlock(hKeyUsage);
            GlobalFree(hKeyUsage);
            hKeyUsage = NULL;
        }
        // set patch size to global offset.
        mmPatch.dwSize = dwOffset;
        // write the patchmap
        if (_llseek(iFile, doData, 0) == -1L) {
                mmaperr = MMAPERR_WRITE;
                goto exit03;
        }
        if ( _lwrite(iFile, (LPSTR)&mmPatch,sizeof(MMPATCH))
           != sizeof(MMPATCH)
           ) {
                mmaperr = MMAPERR_WRITE;
                goto exit03;
        }
        // free the unique keymap name list
        VFreeUniqueList(&hKeyList);
        // free tables, close file and leave
        VFreeTable(MMAP_PATCH | MMAP_KEY);
        mmaperr = MmaperrFileAccess(MAP_FCLOSE, 0);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        return MMAPERR_SUCCESS;
} /* MmaperrWritePatchmap */

//      -       -       -       -       -       -       -       -       -

//      MmaperrWriteKeymap
//
//      Write a keymap to the midi data file.
//
//      1.      If this routine fails it will corrupt the database.

STATIC  MMAPERR NEAR PASCAL MmaperrWriteKeymap (LPMIDIKEYMAP lpKey)
{
        LPMMTABLEENTRY  lpmmEntry;
        MMKEY   mmOldKey;
        MMKEY   mmKey;
        MMAPERR mmaperr;
        DWORD   doData;
        BOOL    fExists;

        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READWRITE);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        mmaperr = MmaperrReadTable(MMAP_KEY);
        if (mmaperr != MMAPERR_SUCCESS) {
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        lpmmEntry = LpGetTableEntry(hKeyTable, lpKey->szName);
        if (lpmmEntry != NULL) {
            if (lstrcmpi((LPSTR)(lpmmEntry->szDesc), lpKey->szDesc)) {
                lstrcpy((LPSTR)(lpmmEntry->szDesc), lpKey->szDesc);
                mmaperr = MmaperrWriteTabEntry(MMAP_KEY, 0, lpmmEntry);
                if (mmaperr != MMAPERR_SUCCESS) {
exit01:             VFreeTable(MMAP_KEY);
                    goto exit00;
                }
            }
            doData = lpmmEntry->doData;
            fExists = TRUE;
        } else {
                mmaperr = MmaperrAddMap(MMAP_KEY, (LPVOID)lpKey, &doData);
                if (mmaperr != MMAPERR_SUCCESS)
                        goto exit01;
                mmKey.wUsing = 0;
                fExists = FALSE;
        }
        // zero-out the keymap data structure
        _fmemset((LPSTR)&mmKey, 0, sizeof(MMKEY));
        // if not a new keymap, get old usage count
        if (fExists) {
                if ( _llseek(iFile, doData, 0) == -1L) {
                        mmaperr = MMAPERR_READ;
                        goto exit01;
                }
                if ( _lread(iFile, (LPSTR)&mmOldKey, sizeof(MMKEY))
                   != sizeof(MMKEY)
                   ) {
                        mmaperr = MMAPERR_READ;
                        goto exit01;
                }
                mmKey.wUsing = mmOldKey.wUsing;
        }
        // copy over flags and keymap data
        mmKey.dwFlags = lpKey->dwFlags;
        _fmemcpy( (LPSTR)(mmKey.bKMap)
                , lpKey->bKMap
                , MIDIPATCHSIZE * sizeof(BYTE));
        if (_llseek(iFile, doData, 0) == -1L) {
                mmaperr = MMAPERR_WRITE;
                goto exit01;
        }
        if (_lwrite(iFile, (LPSTR)&mmKey, sizeof(MMKEY)) != sizeof(MMKEY)) {
                mmaperr = MMAPERR_WRITE;
                goto exit01;
        }
        VFreeTable(MMAP_KEY);
        mmaperr = MmaperrFileAccess(MAP_FCLOSE, 0);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        return MMAPERR_SUCCESS;
} /* MmaperrWriteKeymap */

//      -       -       -       -       -       -       -       -       -

//      MmaperrAddMap
//
//      Generic add map routine.
//      Add a map table entry, increment table use count.
//
//      Issues:
//
//      1.      None.

STATIC  MMAPERR NEAR PASCAL MmaperrAddMap(
        UINT    uFlag,
        LPVOID  lpvMap,
        LPDWORD lpdoData)
{
        HANDLE  hTable;
        LPMMTABLE       lpmmTable;
        LPMMTABLEENTRY  lpmmEntry;
        LPSTR   lpName;
        LPSTR   lpDesc;
        MMTABLEENTRY    mmEntry;
        MMAPERR mmaperr;
        DWORD   doData;
        WORD    wEntry;
        WORD    wEntrys;

        switch (uFlag) {
        case MMAP_SETUP :
                hTable = hSetupTable;
                lpName = ((SETUP FAR*)lpvMap)->aszSetupName;
                lpDesc = ((SETUP FAR*)lpvMap)->aszSetupDescription;
                break;
        case MMAP_PATCH :
                hTable = hPatchTable;
                lpName = ((PATCHMAP FAR*)lpvMap)->aszPatchMapName;
                lpDesc = ((PATCHMAP FAR*)lpvMap)->aszPatchMapDescription;
                break;
        case MMAP_KEY :
                hTable = hKeyTable;
                lpName = ((LPMIDIKEYMAP)lpvMap)->szName;
                lpDesc = ((LPMIDIKEYMAP)lpvMap)->szDesc;
                break;
        default:lpDesc = NULL;  // Kill spurious use before set diagnostic
                lpName = NULL;  // Kill spurious use before set diagnostic
                hTable = NULL;  // Kill spurious use before set diagnostic
        }
        lpmmTable = (LPMMTABLE)GlobalLock(hTable);
        wEntrys = lpmmTable->mmHeader.wEntrys;
        lpmmEntry = (LPMMTABLEENTRY)GlobalLock(lpmmTable->hEntrys);
        for (wEntry = 0; wEntry < wEntrys; wEntry++, lpmmEntry++)
                if (!*lpmmEntry->szName)
                        break;
        GlobalUnlock(lpmmTable->hEntrys);
        if (wEntry == wEntrys) {
                // Filled table;
                mmaperr = MMAPERR_FULL;
                goto exit00;
        }
        _fmemset(&mmEntry, 0, sizeof(MMTABLEENTRY));
        lstrcpy((LPSTR)(mmEntry.szName), lpName);
        lstrcpy((LPSTR)(mmEntry.szDesc), lpDesc);
        mmEntry.idxEntry = wEntry + 1;
        doData = mmEntry.doData = _llseek(iFile, 0L, 2);
        if (doData == -1L) {
                mmaperr = MMAPERR_WRITE;
exit00:         GlobalUnlock(hTable);
                return mmaperr;
        }
        mmaperr = MmaperrWriteTabEntry(uFlag, 0, &mmEntry);
        if (mmaperr != MMAPERR_SUCCESS)
                goto exit00;
        lpmmTable->mmHeader.wUsed++;
        mmaperr = MmaperrWriteTabHeader( uFlag
                                     , (LPMMTABLEHEADER)(&lpmmTable->mmHeader));
        if (mmaperr != MMAPERR_SUCCESS)
                goto exit00;
        GlobalUnlock(hTable);
        *lpdoData = doData;
        return MMAPERR_SUCCESS;
} /* MmaperrAddMap */

//      -       -       -       -       -       -       -       -       -

/*
 * @doc INTERNAL
 *
 * @api DWORD | mapGetSize | This function retrieves the size in bytes
 *      that will be required to read an entire map into memory.  This
 *      includes the size of any patchmaps and keymaps that the map may
 *      contain.
 *
 * @parm UINT | uFlag | Specifies the type of map of which to get the size.
 *      It may be any of the following:
 *
 *      @flag MMAP_SETUP | Get the size of a setup.
 *      @flag MMAP_PATCH | Get the size of a patchmap.
 *      @flag MMAP_KEY | Get the size of a keymap.
 *
 * @parm LPSTR | lpName | Specifies the name of the map of which you
 *      want to get the size.
 *
 * @rdesc An MMAP error code, or the size in bytes required to read in the
 *      map specified by <p lpName>.  If the return value is greater than
 *      MMAPERR_MAXERROR then the return value is a size and not an error.
 */

//
//      This function returns either an error code or the size of the thing.
//      The way you tell the difference is to guess.  The best way to guess
//      is to compare the return value against MMAPERR_MAXERROR, and if it is
//      greater than or equal to that value you've got a size, not an error.
//
//      Issues:
//
//      1.      None.

DWORD   FAR PASCAL mapGetSize(
        UINT    uFlag,
        LPSTR   lpName)
{
        switch (uFlag) {
        case MMAP_SETUP :
                return DwGetSetupSize(lpName);
        case MMAP_PATCH :
        case MMAP_KEY :
                return DwGetMapSize(uFlag, lpName);
        }
} /* mapGetSize */

//      -       -       -       -       -       -       -       -       -

//      DwGetSetupSize
//
//      Get the size of a setup.
//
//      This function returns either an error code or the size of the thing.
//      The way you tell the difference is to guess.  The best way to guess
//      is to compare the return value against MMAPERR_MAXERROR, and if it is
//      greater than or equal to that value you've got a size, not an error.
//
//      Issues:
//
//      1.      None.

STATIC  DWORD NEAR PASCAL DwGetSetupSize(
        LPSTR   lpName)
{
        LPMMTABLEENTRY  lpmmEntry;
        LPMMCHANNEL     lpmmChan;
        MMAPERR mmaperr;
        MMSETUP mmSetup;
        DWORD   dwRet;
        DWORD   dwOffset;
        int     i;

        mmaperr = MmaperrFileAccess(MAP_FOPEN,OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)
                return (DWORD)mmaperr;
        mmaperr = MmaperrReadTable(MMAP_SETUP | MMAP_PATCH);
        if (mmaperr != MMAPERR_SUCCESS) {
                dwRet = (DWORD)mmaperr;
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return dwRet;
        }
        // if name isn't in the file, get outta here
        lpmmEntry = LpGetTableEntry(hSetupTable, lpName);
        if (lpmmEntry == NULL) {
                dwRet = (DWORD)MMAPERR_INVALIDSETUP;
exit01:         VFreeTable(MMAP_SETUP | MMAP_PATCH);
                goto exit00;
        }
        // intialize offset
        dwOffset = sizeof(MIDIMAP);
        // read in the setup data
        if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                dwRet = (DWORD)MMAPERR_READ;
                goto exit01;
        }
        if (_lread(iFile, (LPSTR)&mmSetup,
                sizeof(MMSETUP)) != sizeof(MMSETUP)) {
                dwRet = (DWORD)MMAPERR_READ;
                goto exit01;
        }
        lpmmChan = (LPMMCHANNEL)(mmSetup.chMap); // MIPS silliness
        for (i = 0; i < 16; i++, lpmmChan++) {
                DWORD   dwUniqueRet;

                // if no patchmap then continue
                if (!lpmmChan->idxPMapName)
                        continue;
                // assuming patchmap actually exists here
                lpmmEntry = LpGetTableEntry(hPatchTable,
                        MAKEID(lpmmChan->idxPMapName));
                // if patchmap is unique, add size to global offset
                dwUniqueRet = LiNotUnique( (LPSTR)(lpmmEntry->szName)
                                         , &hPatchList
                                         , dwOffset);
                if (dwUniqueRet == -1L) {
                        dwRet = (DWORD)MMAPERR_MEMORY;
                        goto exit01;
                } else if (!dwUniqueRet) {
                        dwRet = DwGetMapSize(MMAP_PATCH,NULL);
                        if (dwRet < MMAPERR_MAXERROR) {
                                VFreeUniqueList(&hPatchList);
                                goto exit01;
                        }
                        dwOffset += dwRet;
                }
        }
        // free the unique patchmap list
        VFreeUniqueList(&hPatchList);
        VFreeTable(MMAP_SETUP | MMAP_PATCH);
        (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
        return (dwOffset < (DWORD)MMAPERR_MAXERROR) ? (DWORD)MMAPERR_INVALIDSETUP : dwOffset;
} /* DwGetSetupSize */

//      -       -       -       -       -       -       -       -       -

//      DwGetMapSize
//
//      Get the size of a patchmap or keymap.
//
//      This function returns either an error code or the size of the thing.
//      The way you tell the difference is to guess.  The best way to guess
//      is to compare the return value against MMAPERR_MAXERROR, and if it is
//      greater than or equal to that value you've got a size, not an error.
//
//      Remarks:
//
//      1.      None.

STATIC  DWORD NEAR PASCAL DwGetMapSize(
        UINT    uFlag,
        LPSTR   lpName)
{
        LPMMTABLEENTRY lpmmEntry;
        LPHANDLE lphTable;
        MMPATCH mmPatch;
        MMAPERR mmaperr;
        DWORD   dwRet;

        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)
                return (DWORD)mmaperr;
        mmaperr = MmaperrReadTable(uFlag);
        if (mmaperr != MMAPERR_SUCCESS) {
                dwRet = (DWORD)mmaperr;
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return dwRet;
        }
        lphTable = (uFlag == MMAP_PATCH) ? &hPatchTable : &hKeyTable;
        lpmmEntry = LpGetTableEntry(*lphTable, lpName);
        if (lpmmEntry == NULL) {
                dwRet = (uFlag == MMAP_PATCH)
                        ? (DWORD)MMAPERR_INVALIDPATCH
                        : (DWORD)MMAPERR_INVALIDKEY;
                VFreeTable(uFlag);
                goto exit00;
        }
        switch (uFlag) {
        case MMAP_PATCH :
                // patchmaps have the size stored in the file.
                if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                        dwRet = (DWORD)MMAPERR_READ;
                        break;
                }
                if ( _lread(iFile, (LPSTR)&mmPatch, sizeof(MMPATCH))
                   != sizeof(MMPATCH)
                   ) {
                        dwRet = (DWORD)MMAPERR_READ;
                        break;
                }
                dwRet = mmPatch.dwSize;
                if (dwRet < (DWORD)MMAPERR_MAXERROR)
                        dwRet = MMAPERR_INVALIDPATCH;
                break;
        case MMAP_KEY:
                // keymaps are all the same size.
                dwRet = sizeof(MIDIKEYMAP);
                break;
        default: dwRet = (DWORD)MMAPERR_READ;
        }
        VFreeTable(uFlag);
        (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
        return dwRet;
} /* DwGetMapSize */

//      -       -       -       -       -       -       -       -       -

/*
 * @doc INTERNAL
 *
 * @api DWORD | mapDelete | This function deletes a map from the midi
 *       data file.
 *
 * @parm UINT | uFlag | Specifies the type of map to be deleted.
 *      It may be any of the following:
 *
 *      @flag MMAP_SETUP | Delete a setup.
 *      @flag MMAP_PATCH | Delete a patchmap.
 *      @flag MMAP_KEY | Delete a keymap.
 *
 * @parm LPSTR | lpName | Specifies the name of the map to be deleted.
 *
 * @rdesc Returns a MMAP error code or zero on success.
 */

//      Issues:
//
//      1.      If this function fails it will corrupt the database.

MMAPERR FAR PASCAL mapDelete(
        UINT    uFlag,
        LPSTR   lpName)
{
        HANDLE          hUsage;
        LPMMCHANNEL     lpmmChan = NULL; // Kill spurious use before set diag
        LPMMTABLE       lpmmTable;
        LPMMTABLEHEADER lpmmHeader;
        LPMMTABLEENTRY  lpmmEntry;
        LPHANDLE        lphTable;
        LPHANDLE        lphUsageTable;
        UNALIGNED WORD  *lpidxNames = NULL;   // Kill spurious use before set diagnostic
        LPBYTE  lpbUsage;
        MMSETUP mmSetup;
        MMPATCH mmPatch;
        MMAPERR mmaperr;
        UINT    uGarbage;            // size of thing getting deleted
        UINT    uSize;
        UINT    uIdx;                // index of deleted entry in table
        UINT    uUsageFlag = 0;      // SETUP or PATCH => same as uFlag, else 0
        UINT    uNumPos;             // # of positions; setup=16, patch=128
        int     idxEntry;            // table index
        UINT    uIndex;

        // grab the appropriate table, structure size and usage info
        switch (uFlag) {
        case MMAP_SETUP:
                lphTable = &hSetupTable;
                uGarbage = sizeof(MIDIMAP);
                lphUsageTable = &hPatchTable;
                uUsageFlag = MMAP_PATCH;
                uNumPos = 16;
                break;
        case MMAP_PATCH:
                lphTable = &hPatchTable;
                lphUsageTable = &hKeyTable;
                uUsageFlag = MMAP_KEY;
                uGarbage = sizeof(MIDIPATCHMAP);
                uNumPos = MIDIPATCHSIZE;
                break;
        case MMAP_KEY:
                lphTable = &hKeyTable;
                uGarbage = sizeof(MIDIKEYMAP);
                uNumPos = 0;     // Kill spurious diagnostic
                lphUsageTable = NULL; // Kill spurious diagnostic
                break;
        default:uGarbage = 0;
                lphTable = NULL; // kill spurious use before set diagnostic
                lphUsageTable = NULL; // Kill spurious diagnostic
                uNumPos = 0;     // kill spurious use before set diagnostic
        }
        // open file, read in appropriate table(s)
        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READWRITE);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        mmaperr = MmaperrReadTable(uFlag |uUsageFlag);
        if (mmaperr != MMAPERR_SUCCESS) {
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        lpmmEntry = LpGetTableEntry(*lphTable, lpName);
        if (lpmmEntry == NULL) {
                switch (uFlag) {
                case MMAP_SETUP:
                        mmaperr = MMAPERR_INVALIDSETUP;
                        break;
                case MMAP_PATCH:
                        mmaperr = MMAPERR_INVALIDPATCH;
                        break;
                case MMAP_KEY:
                        mmaperr = MMAPERR_INVALIDKEY;
                        break;
                }
exit01:         VFreeTable(uFlag | uUsageFlag);
                goto exit00;
        }
        // save the table index
        idxEntry = lpmmEntry->idxEntry;
        if (uUsageFlag) {
                // obtain the number of entrys in the usage table
                lpmmTable = (LPMMTABLE)GlobalLock(*lphUsageTable);
                uSize = lpmmTable->mmHeader.wEntrys;
                GlobalUnlock(*lphUsageTable);
                // read in setup or patchmap, set optimization pointer
                if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                        mmaperr = MMAPERR_READ;
                        goto exit01;
                }
                if (uUsageFlag == MMAP_PATCH) {
                        if (_lread(iFile, (LPSTR)&mmSetup,
                                sizeof(MMSETUP)) != sizeof(MMSETUP)) {
                                mmaperr = MMAPERR_READ;
                                goto exit01;
                        }
                        lpmmChan =(LPMMCHANNEL)(mmSetup.chMap); // MIPS silliness
                } else {
                        if (_lread(iFile, (LPSTR)&mmPatch,
                                sizeof(MMPATCH)) != sizeof(MMPATCH)) {
                                mmaperr = MMAPERR_READ;
                                goto exit01;
                        }
                        lpidxNames = mmPatch.idxKMapNames;
                }
                // create table which is 'number-of-maps' in length
                if (uSize) {
                    hUsage = GlobalAlloc(GHND, (LONG)uSize);
                    if (hUsage == NULL) {
                            mmaperr = MMAPERR_MEMORY;
                            goto exit01;
                    }
                    lpbUsage = (LPBYTE)GlobalLock(hUsage);
                    // set flags in table saying which patchmaps or
                    // keymaps this setup or patchmap references,
                    // respectively
                    for (uIndex = 0; uIndex < uNumPos; uIndex++) {
                        if (uUsageFlag == MMAP_PATCH) {
                            uIdx = lpmmChan->idxPMapName;
                            lpmmChan++;
                        } else {
                            uIdx = *lpidxNames;
                            lpidxNames++;
                        }
                        if (uIdx)
                            lpbUsage[uIdx - 1] = 1;
                    }
                    // go through table and decrement usage count
                    // of any entrys that are non zero
                    for (uIndex = 0; uIndex < uSize; uIndex++, lpbUsage++) {
                        if (!*lpbUsage)
                            continue;
                        mmaperr = MmaperrChangeUsing(uUsageFlag,uIndex + 1, -1);
//                                                              uIndex + -1, 1)
                        if (mmaperr != MMAPERR_SUCCESS) {
                            GlobalUnlock(hUsage);
                            GlobalFree(hUsage);
                            goto exit01;
                        }
                    }
                    GlobalUnlock(hUsage);
                    GlobalFree(hUsage);
                }
        }
        _fmemset((LPSTR)(lpmmEntry->szName), 0L, MMAP_MAXNAME);
        _fmemset((LPSTR)(lpmmEntry->szDesc), 0L, MMAP_MAXDESC);
        lpmmEntry->doData = 0L;
        lpmmEntry->idxEntry = 0L;
        mmaperr = MmaperrWriteTabEntry(uFlag, idxEntry, lpmmEntry);
        if (mmaperr != MMAPERR_SUCCESS)
                goto exit01;                    // Yes, 01, not 02.
        lpmmTable = (LPMMTABLE)GlobalLock(*lphTable);
        lpmmHeader = (LPMMTABLEHEADER)(&lpmmTable->mmHeader);
        lpmmHeader->wUsed--;
        mmaperr = MmaperrWriteTabHeader(uFlag,lpmmHeader);
        if (mmaperr != MMAPERR_SUCCESS) {
                GlobalUnlock(*lphTable);
                goto exit01;                    // Yes, 01, not 02.
        }
        GlobalUnlock(*lphTable);
        mmaperr = MmaperrGarbage(uGarbage);
        if (mmaperr != MMAPERR_SUCCESS)
                goto exit01;                    // Yes, 01, not 02.
        VFreeTable(uFlag | uUsageFlag);
        mmaperr = MmaperrFileAccess(MAP_FCLOSE, 0);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        return MMAPERR_SUCCESS;
} /* mapDelete */

//      -       -       -       -       -       -       -       -       -

/*
 * @doc INTERNAL
 *
 * @api DWORD | mapEnumerate | This function enumerates the names and
 *      descriptions of all the maps of the specified type found in the
 *      midi data file, or the names and device id's of the ports a setup
 *      accesses.
 *
 * @parm UINT | uFlag | Specifies the type of enumerate to be performed.  It
 *       may be any one of the following values:
 *
 * @flag MMAP_SETUP | Enumerate setup names.
 * @flag MMAP_PATCH | Enumerate patchmap names.
 * @flag MMAP_KEY   | Enumerate keymap names.
 * @flag MMAP_PORTS | Enumerate ports in a setup.
 *
 * @parm ENUMPROC | lpfnCallback | Specifies the procedure-instance address of
 *      the callback function to be called with the name of each map.
 *
 * @parm DWORD | dwUser | If <p uFlag> is <f MMAP_SETUP>, <f MMAP_PATCH>, or
 *      <f MMAP_KEY>, this parameter specifies a user variable that will be
 *      passed to the callback function along with the name of each map.  If
 *      <p uFlag> is <f MMAP_PORTS>, this parameter specifies a far pointer
 *      to a setup name.
 *
 * @rdesc Returns a MMAP error code or zero on success.
 *
 * @comm <p lpfnCallBack> must be obtained using the <f MakeProcInstance>
 *      call, must use the pascal calling convention, must be declared FAR,
 *      and must be under the EXPORTS section in your applications .DEF file.
 *
 * @cb BOOL FAR PASCAL | EnumerateCallback | The callback function for
 *      <f mapEnumerate>.
 *
 * @parm LPSTR | lpName | If <p uFlag> is <f MMAP_SETUP>, <f MMAP_PATCH>, or
 *      <f MMAP_KEY>, this parameter specifies the name of a map of that type.
 *      If <p uFlag> is <f MMAP_PORTS>, this parameter is a number in the
 *      range of 1 to 16 which specifies the channel on which the supplied
 *      port <p lpDesc> is mapped.
 *
 * @parm LPSTR | lpDesc | If <p uFlag> is <f MMAP_SETUP>, <f MMAP_PATCH>, or
 *      <f MMAP_KEY>, this parameter specifies the description of the map in
 *      the <p lpName> parameter.  If <p uFlag> is <f MMAP_PORTS>, this
 *      parameter specifies the name of a port accessed by the setup.
 *
 * @parm DWORD | dwUser | If <p uFlag> is <f MMAP_SETUP>, <f MMAP_PATCH>, or
 *      <f MMAP_KEY>, this parameter specifies a user variable.  If <p uFlag>
 *      is <f MMAP_PORTS>, this parameter specifies the device ID of the
 *      supplied port <p lpDesc>.  In this case, if the port is not available
 *      in the current environment, the device ID is equal to the constant
 *      MMAP_ID_NOPORT.
 *
 * @rdesc It should return non-zero as long as it wants to continue being
 *      called with successive map or port names.  <f mapEnumerate> will stop
 *      enumerating when the callback function returns zero.
 */

//      Issues:
//
//      1.      It regards elements with a zero first byte as "not counting"
//              in the count of elements in a table.  If this is a bad
//              assertion, this routine has a bug.

MMAPERR FAR PASCAL mapEnumerate ( UINT uFlag
                                , ENUMPROC lpfnCallback
                                , UINT uCase        // passed to lpfnCallback
                                , HWND hCombo       // passed to lpfnCallback
                                , LPSTR lpSetupName // passed to lpfnCallback
                                )
{
        LPHANDLE        lphTable;
        LPMMTABLE       lpmmTable;
        LPMMTABLEENTRY  lpmmEntry;
        UINT            wEnum;
        UINT            wUsed;
        MMAPERR mmaperr;

        switch (uFlag) {
        case MMAP_SETUP :
                lphTable = &hSetupTable;
                break;
        case MMAP_PATCH :
                lphTable = &hPatchTable;
                break;
        case MMAP_KEY :
                lphTable = &hKeyTable;
                break;
        case MMAP_PORTS :
                return MmaperrEnumPorts(lpfnCallback, uCase, hCombo, lpSetupName);
        default: lphTable = NULL;  // kill spurious use before set diagnostic
        }
        mmaperr = MmaperrReadTable(uFlag);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        lpmmTable = (LPMMTABLE)GlobalLock(*lphTable);
        lpmmEntry = (LPMMTABLEENTRY)GlobalLock(lpmmTable->hEntrys);
        wUsed = lpmmTable->mmHeader.wUsed;
        for (wEnum = 0; wEnum < wUsed; lpmmEntry++) {
                if (!*lpmmEntry->szName)
                        continue;
                if (!(*lpfnCallback)( (LPSTR)lpmmEntry->szName
                                    , (LPSTR)lpmmEntry->szDesc
                                    , uCase
                                    , hCombo
                                    , lpSetupName
                                    )
                   )
                        break;
                wEnum++;
        }
        GlobalUnlock(lpmmTable->hEntrys);
        GlobalUnlock(*lphTable);
        VFreeTable(uFlag);
        return MMAPERR_SUCCESS;
} /* mapEnumerate */

//      -       -       -       -       -       -       -       -       -

//      MmaperrEnumPorts
//
//      Enumerate the ports in a setup.
//
//      Side-effects:
//
//      1.      None.
//
//      Remarks:
//
//      1.      If the enumeration function fails, this function won't
//              let you know.

STATIC  MMAPERR NEAR PASCAL MmaperrEnumPorts(
        ENUMPROC    lpfnCallback,
        UINT        uCase,          // unused
        HWND        hCombo,         // unused
        LPSTR       lpSetupName)
{
        LPMMTABLEENTRY  lpmmEntry;
        LPMMCHANNEL     lpmmChan;
        MIDIOUTCAPS     moCaps;
        MMSETUP mmSetup;
        MMAPERR mmaperr;
        UINT    wNumDevs;
        UINT    uDeviceID;
        int     i;

        // open file, read in setup table
        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        mmaperr = MmaperrReadTable(MMAP_SETUP);
        if (mmaperr != MMAPERR_SUCCESS) {
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        // assuming setup actually exists
        lpmmEntry = LpGetTableEntry(hSetupTable, lpSetupName);
        if (lpmmEntry == NULL) {
                mmaperr = MMAPERR_INVALIDSETUP;
exit01:         VFreeTable(MMAP_SETUP);
                goto exit00;
        }
        // read in setup data from file
        if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                mmaperr = MMAPERR_READ;
                goto exit01;
        }
        if (_lread(iFile, (LPSTR)&mmSetup, sizeof(MMSETUP)) != sizeof(MMSETUP)) {
                mmaperr = MMAPERR_READ;
                goto exit01;
        }
        // grab the number of devices in the current environment
        wNumDevs = midiOutGetNumDevs();
        // set an optimization-pointer
        lpmmChan = (LPMMCHANNEL)(mmSetup.chMap); // MIPS silliness
        for (i = 0; i < 16; i++, lpmmChan++) {
                // if no device name, then nothing to enumerate
                if (!*lpmmChan->szDevice)
                        continue;
                // convert device name to device ID.  If device name doesn't
                // exist in the current environment, set ID to MMAP_ID_NOPORT
                for (uDeviceID = 0; uDeviceID < wNumDevs; uDeviceID++) {
                        midiOutGetDevCaps(uDeviceID, &moCaps, sizeof(MIDIOUTCAPS));
                        if (!lstrcmpi( moCaps.szPname
                                     , (LPCSTR)(lpmmChan->szDevice)))
                                break;
                }
                if (uDeviceID == wNumDevs)
                        uDeviceID = MMAP_ID_NOPORT;
                if (!(*lpfnCallback)( (LPSTR)(DWORD)i + 1
                                    , (LPSTR)(lpmmChan->szDevice)
                                    , uCase         // garbage parameter
                                    , hCombo        // garbage parameter
                                    , (LPSTR)uDeviceID
                                    )
                   )
                        break;
        }
        // free table, close file and leave
        VFreeTable(MMAP_SETUP);
        (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
        return MMAPERR_SUCCESS;
} /* MmaperrEnumPorts */

//      -       -       -       -       -       -       -       -       -

/*
 * @doc INTERNAL
 *
 * @api DWORD | mapGetUsageCount | Get the usage count for a patchmap or
 *      keymap.
 *
 * @parm UINT | uFlag | Specifies the type of map to get the usage count for.
 *      It may be any one of the following values:
 *
 * @flag MMAP_PATCH | Get the usage count for a patchmap.
 * @flag MMAP_KEY | Get the usage count for a keymap.
 *
 * @parm LPSTR | lpName | Specifies the name of the map to get the usage
 *      count for.
 *
 * @rdesc Returns a MMAP error code, or the usage count for the map
 *      specified by <p lpName> in the high-order word.
 *
 * @comm Usage counts are used in order to determine whether it is okay to
 *      delete a patchmap or keymap.  If a setup accesses a patchmap, the
 *      usage count for that patchmap will be 1, and hence it should not be
 *      deleted.  The same is true for patchmaps accessing keymaps.  The
 *      mapDelete function does not look for or care about this number, so
 *      it is up to YOU to determine whether a patchmap or keymap has a zero
 *      usage count before calling mapDelete.
 */

//      Side-effects:
//
//      1.      None.

DWORD   FAR PASCAL mapGetUsageCount(
        UINT    uFlag,
        LPSTR   lpName)
{
        LPMMTABLEENTRY  lpmmEntry;
        MMPATCH mmPatch;
        MMKEY   mmKey;
        MMAPERR mmaperr;
        DWORD   dwRet;

        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)
                return MAKELONG(mmaperr, 0);
        mmaperr = MmaperrReadTable(uFlag);
        if (mmaperr != MMAPERR_SUCCESS) {
                dwRet = MAKELONG(mmaperr, 0);
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return dwRet;
        }
        switch (uFlag) {
        case MMAP_PATCH:
                lpmmEntry = LpGetTableEntry(hPatchTable, lpName);
                if (lpmmEntry != NULL) {
                        if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                                dwRet = MAKELONG(MMAPERR_READ, 0);
exit01:                         VFreeTable(uFlag);
                                goto exit00;
                        }
                        if ( _lread(iFile, (LPSTR)&mmPatch, sizeof(MMPATCH))
                           != sizeof(MMPATCH)
                           ) {
                                dwRet = MAKELONG(MMAPERR_READ, 0);
                                goto exit01;
                        }
                        dwRet = MAKELONG(MMAPERR_SUCCESS, mmPatch.wUsing);
exit02:                 VFreeTable(uFlag);
                        (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                        return dwRet;
                } else {
                        dwRet = MAKELONG(MMAPERR_INVALIDPATCH, 0);
                        goto exit01;
                }
                break;          // This "break" is actually unreachable.
        case MMAP_KEY:
                lpmmEntry = LpGetTableEntry(hKeyTable, lpName);
                if (lpmmEntry != NULL) {
                        if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                                dwRet = MAKELONG(MMAPERR_READ, 0);
                                goto exit01;
                        }
                        if (_lread(iFile, (LPSTR)&mmKey, sizeof(MMKEY))
                           != sizeof(MMKEY)
                           ) {
                                dwRet = MAKELONG(MMAPERR_READ, 0);
                                goto exit01;
                        }
                        dwRet = MAKELONG (MMAPERR_SUCCESS, mmKey.wUsing);
                        goto exit02;
                } else {
                        dwRet = MAKELONG(MMAPERR_INVALIDPATCH, 0);
                        goto exit01;
                }
                break;          // This "break" is actually unreachable.
        }
} /* mapGetUsageCount */

//      -       -       -       -       -       -       -       -       -

/*
 * @doc INTERNAL
 *
 * @api DWORD | mapPatchMapInSetup | Determine if a patchmap is used within a
 *      setup.
 *
 * @parm LPSTR | lpPatchName | Specifies the name of the patchmap.
 *
 * @parm LPSTR | lpSetupName | Specifies the name of the setup.
 *
 * @rdesc Returns a MMAP error code, or non-zero in the high-order word if the
 *      given patchmap is used within the given setup.
 *
 * @xref mapKeyMapInSetup
 */

//      Side-effects:
//
//      1.      None.

DWORD   FAR PASCAL mapPatchMapInSetup(
        LPSTR   lpPatchName,
        LPSTR   lpSetupName)
{
        LPMMTABLEENTRY  lpmmEntry;
        LPMMCHANNEL     lpmmChan;
        MMSETUP mmSetup;
        DWORD   dwRet;
        MMAPERR mmaperr;
        int     i;

        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)
                return MAKELONG(mmaperr, 0);
        mmaperr = MmaperrReadTable(MMAP_SETUP | MMAP_PATCH);
        if (mmaperr != MMAPERR_SUCCESS) {
                dwRet = MAKELONG(mmaperr, 0);
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return dwRet;
        }
        lpmmEntry = LpGetTableEntry(hSetupTable, lpSetupName);
        if (lpmmEntry == NULL) {
                dwRet = MAKELONG(MMAPERR_INVALIDSETUP, 0);
exit01:         VFreeTable(MMAP_SETUP | MMAP_PATCH);
                goto exit00;
        }
        if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                dwRet = MAKELONG(MMAPERR_READ, 0);
                goto exit01;
        }
        if (_lread(iFile, (LPSTR)&mmSetup, sizeof(MMSETUP)) != sizeof(MMSETUP)){
                dwRet = MAKELONG(MMAPERR_READ, 0);
                goto exit01;
        }
        lpmmChan = (LPMMCHANNEL)(mmSetup.chMap); // MIPS silliness
        for (i = 0; i < 16; i++, lpmmChan++) {
                if (!lpmmChan->idxPMapName)
                        continue;
                lpmmEntry = LpGetTableEntry(hPatchTable,
                        MAKEID(lpmmChan->idxPMapName));
                if (!lstrcmpi(lpPatchName, (LPCSTR)(lpmmEntry->szName)))
                        break;
        }
        VFreeTable(MMAP_SETUP | MMAP_PATCH);
        (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
        return MAKELONG(MMAPERR_SUCCESS, (i < 16) ? 1 : 0);
} /* mapPatchMapInSetup */

//      -       -       -       -       -       -       -       -       -

/*
 * @doc INTERNAL
 *
 * @api DWORD | mapKeyMapInSetup | Determine if a keymap is used within a
 *      setup.
 *
 * @parm LPSTR | lpKeyName | Specifies the name of the keymap.
 *
 * @parm LPSTR | lpSetupName | Specifies the name of the setup.
 *
 * @rdesc Returns a MMAP error code, or non-zero in the high-order word
 *      if the given keymap is used within the given patchmap.
 *
 * @xref mapPatchMapInSetup
 */

DWORD   FAR PASCAL mapKeyMapInSetup(
        LPSTR   lpKeyName,
        LPSTR   lpSetupName)
{
        LPMMTABLEENTRY  lpmmEntry;
        LPMMCHANNEL     lpmmChan;
        MMSETUP mmSetup;
        MMPATCH mmPatch;
        UNALIGNED WORD  *lpidxKMap;
        DWORD   dwRet;
        MMAPERR mmaperr;
        int     i;
        int     j;

        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)
                return MAKELONG(mmaperr, 0);
        mmaperr = MmaperrReadTable(MMAP_SETUP | MMAP_PATCH | MMAP_KEY);
        if (mmaperr != MMAPERR_SUCCESS) {
                dwRet = MAKELONG(mmaperr, 0);
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return dwRet;
        }
        if ((lpmmEntry = LpGetTableEntry(hSetupTable, lpSetupName)) == NULL) {
                dwRet = MAKELONG(MMAPERR_INVALIDSETUP, 0);
exit01:         VFreeTable(MMAP_SETUP | MMAP_PATCH | MMAP_KEY);
                goto exit00;
        }
        if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                dwRet = MAKELONG(MMAPERR_READ, 0);
                goto exit01;
        }
        if (_lread(iFile, (LPSTR)&mmSetup, sizeof(MMSETUP)) != sizeof(MMSETUP)){
                dwRet = MAKELONG(MMAPERR_READ, 0);
                goto exit01;
        }
        lpmmChan = (LPMMCHANNEL)(mmSetup.chMap);  // MIPS silliness
        dwRet = MAKELONG(MMAPERR_SUCCESS, 0);
        for (i = 0; i < 16; i++, lpmmChan++) {
                DWORD   dwUniqueRet;

                if (!lpmmChan->idxPMapName)
                        continue;
                lpmmEntry = LpGetTableEntry(hPatchTable,
                        MAKEID(lpmmChan->idxPMapName));
                dwUniqueRet = LiNotUnique( (LPSTR)(lpmmEntry->szName)
                                         , &hPatchList
                                         , 0L);
                        //  0L in the "dwOffset" field  because it's never used.
                if (dwUniqueRet == -1L) {
                        dwRet = MAKELONG(MMAPERR_MEMORY, 0);
exit02:                 VFreeUniqueList(&hPatchList);
                        goto exit01;
                } else if (dwUniqueRet)
                        continue;
                if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                        dwRet = MAKELONG(MMAPERR_READ, 0);
                        goto exit02;
                }
                if ( _lread(iFile, (LPSTR)&mmPatch, sizeof(MMPATCH))
                   != sizeof(MMPATCH)
                   ) {
                        dwRet = MAKELONG(MMAPERR_READ, 0);
                        goto exit02;
                }
                lpidxKMap = mmPatch.idxKMapNames;
                for (j = 0; j < MIDIPATCHSIZE; j++, lpidxKMap++) {
                        if (!*lpidxKMap)
                                continue;
                        lpmmEntry = LpGetTableEntry(hKeyTable,
                                MAKEID(*lpidxKMap));
                        if (!lstrcmpi(lpKeyName, (LPCSTR)(lpmmEntry->szName))) {
                                dwRet = MAKELONG(MMAPERR_SUCCESS, 1);
                                break;
                        }
                }
                if (j < MIDIPATCHSIZE)
                        break;
        }
        VFreeUniqueList(&hPatchList);
        // free tables, close file and leave
        VFreeTable(MMAP_SETUP | MMAP_PATCH | MMAP_KEY);
        (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
        return dwRet;
} /* mapKeyMapInSetup */

//      -       -       -       -       -       -       -       -       -

/*
 * @doc INTERNAL
 *
 * @api DWORD | mapExists | Deterine if a map exists by the given name.
 *
 * @parm UINT | uFlag | Specifies the type of map of which existence is
 *      to be determined. It may be any of the following:
 *
 *      @flag MMAP_SETUP | Determine if a setup exists.
 *      @flag MMAP_PATCH | Determine if a patchmap exists.
 *      @flag MMAP_KEY |  Determine if a keymap exists.
 *
 * @parm LPSTR | lpName | Specifies the name of the map.
 *
 * @rdesc Returns a MMAP error code in the low word.  The the call succeeds,
 *      returns non-zero in the high-order word if a map exists by the given
 *      name.
 */

//      Side-effects:
//
//      1.      None.

DWORD FAR PASCAL mapExists(
        UINT    uFlag,
        LPSTR   lpName)
{
        LPHANDLE        lphTable;
        LPMMTABLEENTRY  lpmmEntry;
        MMAPERR mmaperr;

        switch (uFlag) {
        case MMAP_SETUP:
                lphTable = &hSetupTable;
                break;
        case MMAP_PATCH:
                lphTable = &hPatchTable;
                break;
        case MMAP_KEY:
                lphTable = &hKeyTable;
                break;
        default:lphTable = NULL; // kill spurious use before sete diagnostic
        }
        if ((mmaperr = MmaperrReadTable(uFlag)) != MMAPERR_SUCCESS)
                return MAKELONG(mmaperr, 0);
        lpmmEntry = LpGetTableEntry(*lphTable, lpName);
        VFreeTable(uFlag);
        return MAKELONG(MMAPERR_SUCCESS, (lpmmEntry != NULL) ? 1 : 0);
} /* mapExists */

//      -       -       -       -       -       -       -       -       -

//      LiNotUnique
//
//      Return 0L if the supplied name is unique, and in this case add it to
//      the end of the list with the offset given in parm 3.
//
//      Return -1 if the GlobalAlloc fails so that the unique name
//      cannot be added.
//
//      Otherwise return an offset from the base of the current setup
//      or patchmap where the map exists, or the channel number which
//      accesses the invalid port.
//
//      Uniqueness is determined by the presence of the name on its respective
//      list; hPatchList for patchmap names; hKeyList for keymap names;
//      hPortList for port names.  It's up to the caller to give us the
//      right list to search.
//

STATIC  DWORD NEAR PASCAL LiNotUnique(
        LPSTR   lpName,
        LPHANDLE lphList,
        DWORD   dwOffset)
{
        LPUNIQUEMAP  lpumEntry = NULL; //; kill spurious use before set diag
        HANDLE  hCur;
        HANDLE  hPrev = NULL;

        for (hCur = *lphList; hCur;) {
                lpumEntry = (LPUNIQUEMAP)GlobalLock(hCur);
                if (!lstrcmpi((LPCSTR)(lpumEntry->szName), lpName)) {
                        DWORD   liRet;

                        liRet = lpumEntry->dwOffset;
                        GlobalUnlock(hCur);
                        return liRet;
                }
                hPrev = hCur;
                hCur = lpumEntry->hNext;
                if (hCur != NULL) {
                        GlobalUnlock(hPrev);
                }

        }
        hCur = GlobalAlloc(GHND, (DWORD)sizeof(UNIQUEMAP));
        if (hCur  == NULL) {
                if (hPrev != NULL) {
                        GlobalUnlock(hPrev);
                }
                return (DWORD)-1L;
        }
        if (hPrev != NULL) {
                lpumEntry->hNext = hCur;
                GlobalUnlock(hPrev);
        } else
                *lphList = hCur;
        lpumEntry = (LPUNIQUEMAP)GlobalLock(hCur);
        lstrcpy((LPSTR)(lpumEntry->szName), lpName);
        lpumEntry->dwOffset = dwOffset;
        lpumEntry->hNext = NULL;
        GlobalUnlock(hCur);
        return 0L;
} /* LiNotUnique */

//      -       -       -       -       -       -       -       -       -

//      LszGetUniqueAtOffset
//
//      Return a unique name given an offset.
//
//      Remarks:
//
//      1.      I don't think this can fail, although there's a weird case
//              if you run out of list.  In this case you get an empty string.
//              I'm not sure if this is bad or not.

STATIC  LPSTR NEAR PASCAL LszGetUniqueAtOffset(
        DWORD   dwOffset,
        HANDLE  hList)
{
        static char szUnique[MAX_UNIQUENAME];

        *szUnique = 0;
        while (hList) {
                HANDLE  hNext;
                LPUNIQUEMAP lpumEntry;

                lpumEntry = (LPUNIQUEMAP)GlobalLock(hList);
                if (dwOffset == lpumEntry->dwOffset) {
                        lstrcpy(szUnique, (LPCSTR)(lpumEntry->szName));
                        GlobalUnlock(hList);
                        break;
                }
                hNext = lpumEntry->hNext;
                GlobalUnlock(hList);
                hList = hNext;
        }
        return (LPSTR)szUnique;
} /* LszGetUniqueAtOffset */

//      -       -       -       -       -       -       -       -       -

//
//      VFreeUniqueList
//
//      Free up the memory associated with a unique name list.
//
//      Side-effects:
//
//      1.      None.

STATIC  VOID NEAR PASCAL VFreeUniqueList(
        LPHANDLE        lphList)
{
        // there was a code generation compiler bug
        // it thought that hPrev and *lphList were synonyms.  Worrying!!
        HANDLE          hPrev;
        HANDLE          hList;
        hList = *lphList;
        *lphList = NULL;
        for (; hList != NULL; ) {
                LPUNIQUEMAP     lpumEntry;

                hPrev = hList;
                lpumEntry = (LPUNIQUEMAP)GlobalLock(hPrev);
                hList = lpumEntry->hNext;
                GlobalUnlock(hPrev);
                GlobalFree(hPrev);
        }
} /* VFreeUniqueList */

//      -       -       -       -       -       -       -       -       -

//      MmaperrChangeUsing
//
//      Change the usage count of a patchmap or keymap.
//
//      Side-effects:
//
//      1.      None.

STATIC  MMAPERR NEAR PASCAL MmaperrChangeUsing(
        UINT    uFlag,
        UINT    uIdx,
        int     iVal)
{
        HANDLE  hTable;
        LPMMTABLEENTRY  lpmmEntry;
        MMPATCH mmPatch;
        MMAPERR mmaperr;
        MMKEY   mmKey;

        if ((mmaperr = MmaperrFileAccess(MAP_FOPEN,
                OF_READWRITE)) != MMAPERR_SUCCESS)
                return mmaperr;
        if ((mmaperr = MmaperrReadTable(uFlag)) != MMAPERR_SUCCESS) {
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        hTable = (uFlag == MMAP_PATCH) ? hPatchTable : hKeyTable;
        lpmmEntry = LpGetTableEntry(hTable,             // No error return
                MAKEID(uIdx));                          //  is possible.
        if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                mmaperr = MMAPERR_READ;
exit01:         VFreeTable(uFlag);
                goto exit00;
        }
        switch (uFlag) {
        case MMAP_PATCH:
                if ( _lread(iFile, (LPSTR)&mmPatch, sizeof(MMPATCH))
                   != sizeof(MMPATCH)
                   ) {
                        mmaperr = MMAPERR_READ;
                        goto exit01;
                }
                mmPatch.wUsing += iVal;
                if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                        mmaperr = MMAPERR_WRITE;
                        goto exit01;
                }
                if ( _lwrite(iFile, (LPSTR)&mmPatch, sizeof(MMPATCH))
                   != sizeof(MMPATCH)
                   ) {
                        mmaperr = MMAPERR_WRITE;
                        goto exit01;
                }
                break;
        case MMAP_KEY:
                if (_lread(iFile, (LPSTR)&mmKey, sizeof(MMKEY)) != sizeof(MMKEY)) {
                        mmaperr = MMAPERR_READ;
                        goto exit01;
                }
                mmKey.wUsing += iVal;
                if (_llseek(iFile, lpmmEntry->doData, 0) == -1L) {
                        mmaperr = MMAPERR_WRITE;
                        goto exit01;
                }
                if (_lwrite(iFile, (LPSTR)&mmKey, sizeof(MMKEY)) != sizeof(MMKEY)) {
                        mmaperr = MMAPERR_WRITE;
                        goto exit01;
                }
                break;
        }
        VFreeTable(uFlag);
        if ((mmaperr = MmaperrFileAccess(MAP_FCLOSE, 0)) != MMAPERR_SUCCESS)
                return mmaperr;
        return MMAPERR_SUCCESS;
} /* MmaperrChangeUsing */

//      -       -       -       -       -       -       -       -       -

//
//      MmaperrReadTable
//
//      Read in table(s) of names/descriptions from the midimap file.
//
//      Returns boolean.
//
//      Side-effects:
//
//      1.      None anymore.

STATIC  MMAPERR NEAR PASCAL MmaperrReadTable(
        UINT    APIn)
{
        MMHEADER        mmHeader;
        LPMMTABLE       lpmmTable;
        LPMMTABLEENTRY  lpmmEntry;
        LPHANDLE        lpTable;
        MMAPERR mmaperr;
        UINT    oPos;
        UINT    uTableSize;
        UINT    mmap;
        UINT    mmapCompleted;

        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READ);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        if (_llseek(iFile, 0L, 0) == -1L) {
                mmaperr = MMAPERR_READ;
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        if (_lread(iFile, (LPSTR)&mmHeader, sizeof(MMHEADER)) != sizeof(MMHEADER)) {
                mmaperr = MMAPERR_READ;
                goto exit00;
        }
        mmapCompleted = 0;
        for (mmap = 1; mmap < 5; mmap <<= 1) {
                if (!(APIn & mmap))
                        continue;
                switch (mmap) {
                default:
                        continue;
                case MMAP_SETUP:
                        lpTable = &hSetupTable;
                        oPos = mmHeader.oSetup;
                        break;
                case MMAP_PATCH:
                        lpTable = &hPatchTable;
                        oPos = mmHeader.oPatch;
                        break;
                case MMAP_KEY:
                        lpTable = &hKeyTable;
                        oPos = mmHeader.oKey;
                        break;
                }
                if (*lpTable)
                        lpmmTable = (LPMMTABLE)GlobalLock(*lpTable);
                else {
                        *lpTable = GlobalAlloc(GHND, (DWORD)sizeof(MMTABLE));
                        if (*lpTable == NULL) {
                                mmaperr = MMAPERR_MEMORY;
exit01:                         VFreeTable(mmapCompleted);
                                goto exit00;
                        }
                        lpmmTable = (LPMMTABLE)GlobalLock(*lpTable);
                        if (_llseek(iFile, (LONG)oPos, 0) == -1L) {
                                mmaperr = MMAPERR_READ;
exit02:                         GlobalUnlock(*lpTable);
                                GlobalFree(*lpTable);
                                *lpTable = NULL;
                                goto exit01;
                        }
                        if ( _lread(iFile, (LPSTR)&lpmmTable->mmHeader,sizeof(MMTABLEHEADER))
                           != sizeof(MMTABLEHEADER)
                           ) {
                                mmaperr = MMAPERR_READ;
                                goto exit02;
                        }
                        uTableSize = lpmmTable->mmHeader.wEntrys *
                                sizeof(MMTABLEENTRY);
                        lpmmTable->hEntrys = GlobalAlloc( GHND
                                                        , (DWORD)uTableSize
                                                        );
                        if (lpmmTable->hEntrys == NULL) {
                                mmaperr = MMAPERR_MEMORY;
                                goto exit02;
                        }
                        lpmmEntry = (LPMMTABLEENTRY)GlobalLock(
                                                          lpmmTable->hEntrys);
                        if ( _lread(iFile, (LPSTR)lpmmEntry, uTableSize)
                           != uTableSize
                           ) {
                                mmaperr = MMAPERR_READ;
/*exit03:*/                     GlobalUnlock(lpmmTable->hEntrys);
                                GlobalFree(lpmmTable->hEntrys);
                                goto exit02;
                        }
                        GlobalUnlock(lpmmTable->hEntrys);
                }
                lpmmTable->wCount++;
                GlobalUnlock(*lpTable);
                mmapCompleted |= mmap;
        }
        (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
        return MMAPERR_SUCCESS;
} /* MmaperrReadTable */

//      -       -       -       -       -       -       -       -       -

//
//      VFreeTable
//
//      Free a table (or tables) from memory.
//
//      Side-effects:
//
//      1.      None.

STATIC  VOID NEAR PASCAL VFreeTable(
        UINT    APIn)
{
        LPMMTABLE       lpmmTable;
        LPHANDLE        lpTable;
        UINT    mmap;

        for (mmap = 1; mmap < 5; mmap <<= 1) {
                if (!(APIn & mmap))
                        continue;
                switch (mmap) {
                default:
                        continue;
                case MMAP_SETUP:
                        lpTable = &hSetupTable;
                        break;
                case MMAP_PATCH:
                        lpTable = &hPatchTable;
                        break;
                case MMAP_KEY:
                        lpTable = &hKeyTable;
                        break;
                }
                if (!*lpTable)
                        continue;
                lpmmTable = (LPMMTABLE)GlobalLock(*lpTable);
                if (--lpmmTable->wCount) {
                        GlobalUnlock(*lpTable);
                        continue;
                }
                GlobalFree(lpmmTable->hEntrys);
                GlobalUnlock(*lpTable);
                GlobalFree(*lpTable);
                *lpTable = NULL;
        }
} /* VFreeTable */

//      -       -       -       -       -       -       -       -       -

//
//      LpGetTableEntry
//
//      Given a name or index, return a table entry.  If lpName is NULL, it
//      assumes you have already called this and simply returns whatever
//      happens to be in the local static mmEntry.
//
//      Side-effects:
//
//      1.      None.
//
//      Remarks:
//
//      1.      This can't return an error if "lpName" is NULL, or if the
//              HIWORD of it is zero.

STATIC  LPMMTABLEENTRY NEAR PASCAL LpGetTableEntry(
        HANDLE  hTable,
        LPSTR   lpName         // Name or index
        )
{
        static MMTABLEENTRY mmEntry;
        LPMMTABLE       lpmmTable;
        LPMMTABLEENTRY  lpmmEntry;
        UINT    wNum        ;
        UINT    wUsed;

        if (lpName == NULL) {
                return &mmEntry;
        }
        lpmmTable = (LPMMTABLE)GlobalLock(hTable);
        lpmmEntry = (LPMMTABLEENTRY)GlobalLock(lpmmTable->hEntrys);
        wNum  = 0;
        if (!HIWORD(lpName)) {
                lpmmEntry += LOWORD ((LONG)lpName) - 1;
                wUsed = 1;
        } else {
                /* search through storage handled by lpmmTable->hEntrys for lpName */
                /* set wNum to its number and lpmmEntry to the entry               */
                wUsed = lpmmTable->mmHeader.wUsed;
                for (; wNum         < wUsed; lpmmEntry++) {
                        if (!*lpmmEntry->szName) {
                                continue;
                        }

                        if (!lstrcmpi((LPCSTR)(lpmmEntry->szName), lpName))
                                break;
                        wNum++;
                }
        }
        if (wNum < wUsed)
                mmEntry = *lpmmEntry;
        GlobalUnlock(lpmmTable->hEntrys);
        GlobalUnlock(hTable);
        if (wNum == wUsed)
                return NULL;
        return (LPMMTABLEENTRY)&mmEntry;
} /* LpGetTableEntry */

//      -       -       -       -       -       -       -       -       -

/*
 * @doc INTERNAL
 *
 * @api MMAPERR | MmaperrWriteTabEntry | Write a table entry to disk.
 *
 * uIdx may be a table entry index (1 ... n) or it may be zero if
 * you want to use lpmmEntry->idxEntry as the index.
 */

//      Side-effects of failure:
//
//      1.      If the "close" fails something bad has happened to the file.
//
//      Remarks:
//
//      1.      This function used to modify the in-memory table first, then
//              go on to tackle the on-disk entry.  It now does this in
//              reverse order in order to avoid a situation where the
//              in-memory table is changed but the disk representation isn't.

STATIC  MMAPERR NEAR PASCAL MmaperrWriteTabEntry(
        UINT    uFlag,
        UINT    uIdx,
        LPMMTABLEENTRY  lpmmEntry)
{
        HANDLE          hTable;
        LPMMTABLE       lpmmTable;
        LPMMTABLEENTRY  lpmmOldEntry;
        MMHEADER        mmHeader;
        UNALIGNED WORD  *lpoTable;
        MMAPERR mmaperr;

        switch (uFlag) {
        case MMAP_SETUP:
                lpoTable = &mmHeader.oSetup;
                hTable = hSetupTable;
                break;
        case MMAP_PATCH:
                lpoTable = &mmHeader.oPatch;
                hTable = hPatchTable;
                break;
        case MMAP_KEY:
                lpoTable = &mmHeader.oKey;
                hTable = hKeyTable;
                break;
        default:hTable = NULL;   // Kill spurious use before set diag
                lpoTable = NULL; // Kill spurious use before set diag
        }
        if (!uIdx)
                uIdx = lpmmEntry->idxEntry;
        uIdx--;
        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READWRITE);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        if (_llseek(iFile, 0L, 0) == -1L) {
                mmaperr = MMAPERR_READ;
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
exit01:         return mmaperr;
        }
        if ( _lread(iFile, (LPSTR)&mmHeader, sizeof(MMHEADER))
           != sizeof(MMHEADER)
           ) {
                mmaperr = MMAPERR_READ;
                goto exit00;                            // 00, not 01.
        }
        if (_llseek( iFile
                   , (LONG) ( *lpoTable
                            + sizeof(MMTABLEHEADER)
                            + uIdx * sizeof(MMTABLEENTRY)
                            )
                   , 0
                   ) == -1L
           ) {
                mmaperr = MMAPERR_WRITE;
                goto exit00;                            // 00, not 01.
        }
        if ( _lwrite(iFile, (LPSTR)lpmmEntry, sizeof(MMTABLEENTRY))
           != sizeof(MMTABLEENTRY)
           ) {
                mmaperr = MMAPERR_WRITE;
                goto exit00;                            // 00, not 01.
        }
        mmaperr = MmaperrFileAccess(MAP_FCLOSE, 0);
        if (mmaperr != MMAPERR_SUCCESS)
                goto exit01;
        // if the table is in memory, modify the in-memory table entry
        if (hTable) {
                lpmmTable = (LPMMTABLE)GlobalLock(hTable);
                lpmmOldEntry = (LPMMTABLEENTRY)GlobalLock(lpmmTable->hEntrys);
                lpmmOldEntry += uIdx;
                *lpmmOldEntry = *lpmmEntry;
                GlobalUnlock(lpmmTable->hEntrys);
                GlobalUnlock(hTable);
        }
        return MMAPERR_SUCCESS;
} /* MmaperrWriteTabEntry */

//      -       -       -       -       -       -       -       -       -

//      MmaperrWriteTabHeader
//
//      Write a table header.
//
//      Side-effects of failure:
//
//      1.      If the "close" fails something bad has happened to the file.

STATIC  MMAPERR NEAR PASCAL MmaperrWriteTabHeader(
        UINT    uFlag,
        LPMMTABLEHEADER lpmmHeader)
{
        MMHEADER        mmHeader;
        UNALIGNED WORD  *lpoTable;
        MMAPERR mmaperr;

        switch (uFlag) {
        case MMAP_SETUP:
                lpoTable = &mmHeader.oSetup;
                break;
        case MMAP_PATCH:
                lpoTable = &mmHeader.oPatch;
                break;
        case MMAP_KEY:
                lpoTable = &mmHeader.oKey;
                break;
        default:lpoTable = NULL; // Kill spurious use before set diagnostic
        }
        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READWRITE);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        if (_llseek(iFile, 0L, 0) == -1L) {
                mmaperr = MMAPERR_READ;
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        if (_lread(iFile, (LPSTR)&mmHeader, sizeof(MMHEADER)) != sizeof(MMHEADER))
        {
                mmaperr = MMAPERR_READ;
                goto exit00;
        }
        if (_llseek(iFile, (LONG)*lpoTable, 0) == -1L) {
                mmaperr = MMAPERR_WRITE;
                goto exit00;
        }
        if ( _lwrite(iFile, (LPSTR)lpmmHeader, sizeof(MMTABLEHEADER))
           != sizeof(MMTABLEHEADER)
           ) {
                mmaperr = MMAPERR_WRITE;
                goto exit00;
        }
        return MmaperrFileAccess(MAP_FCLOSE, 0);
} /* MmaperrWriteTabHeader */

//      MmaperrGarbage
//
//      Increase the garbage-byte count.
//
//      Side-effects:
//
//      1.      None.

STATIC  MMAPERR NEAR PASCAL MmaperrGarbage(
        UINT    uBytes)
{
        MMHEADER mmHeader;
        MMAPERR mmaperr;

        mmaperr = MmaperrFileAccess(MAP_FOPEN, OF_READWRITE);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        if (_llseek(iFile, 0L, 0) == -1L) {
                mmaperr = MMAPERR_READ;
exit00:         (VOID)MmaperrFileAccess(MAP_FCLOSE, 0);
                return mmaperr;
        }
        if (_lread(iFile, (LPSTR)&mmHeader, sizeof(MMHEADER)) != sizeof(MMHEADER)) {
                mmaperr = MMAPERR_READ;
                goto exit00;
        }
        mmHeader.dwGarbage += uBytes;
        if (_llseek(iFile, 0L, 0) == -1L) {
                mmaperr = MMAPERR_WRITE;
                goto exit00;
        }
        if (_lwrite(iFile, (LPSTR)&mmHeader, sizeof(MMHEADER)) != sizeof(MMHEADER)) {
                mmaperr = MMAPERR_WRITE;
                goto exit00;
        }
        return MmaperrFileAccess(MAP_FCLOSE, 0);
} /* MmaperrGarbage */

//      -       -       -       -       -       -       -       -       -

//
//      MmaperrFileAccess
//
//      Control the global file descriptor.
//
//      Returns TRUE on success, FALSE on error.
//
//      Remarks:
//
//      1.      Opens/closes file potentially.
//      2.      Modifies "iFile".
//      3.      Modifies "ucFileOpen".

MMAPERR NEAR PASCAL MmaperrFileAccess(
        int     iFunc,    /* MAP_FOPEN, MAP_FCREATE or MAP_FCLOSE */
        int     iMode)
{
    OFSTRUCT of;

    if (!fEditing)
    {
        char    szMapCfg[MMAP_MAXCFGNAME];

        GetSystemDirectory(aszMapperPath, sizeof(aszMapperPath));
        LoadString(hLibInst,IDS_MIDIMAPCFG,szMapCfg,MMAP_MAXCFGNAME);
        lstrcat(aszMapperPath, szMapCfg);
    }

    switch (iFunc) {
    case MAP_FOPEN :
        if (iFile != HFILE_ERROR) {
                ucFileOpen++;
                break;
        }
        iFile = OpenFile(aszMapperPath, &of, iMode);
        if (iFile == HFILE_ERROR) {
                if (of.nErrCode == 0x05)
                        return MMAPERR_OPEN_READONLY;
                else
                        return MMAPERR_OPEN;
        }
        ucFileOpen++;
        break;
    case MAP_FCREATE :
        iFile = OpenFile(aszMapperPath, &of,iMode|OF_CREATE|OF_READWRITE);
        if (iFile == HFILE_ERROR)
                return MMAPERR_CREATE;
        ucFileOpen++;
        break;
    case MAP_FCLOSE :
        if (!--ucFileOpen) {
                _lclose(iFile);
                iFile = HFILE_ERROR;
        }
        break;
    }
    return MMAPERR_SUCCESS;
} /* MmaperrFileAccess */

//      -       -       -       -       -       -       -       -       -
