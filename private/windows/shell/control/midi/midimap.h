/*
 * MIDIMAP.H
 *
 * Copyright (C) 1990 Microsoft Corporation.  All rights reserved.
 */

/****************************************************************************

   MIDI output mapping definitions

*****************************************************************************/

/*
 * Error code definitions.
 */


typedef UINT    MMAPERR;

#define MMAPERR_SUCCESS     (MMAPERR)0  // Nothing bad happened.
#define MMAPERR_FILENOTFOUND    (MMAPERR)1  // midimap.cfg not found
#define MMAPERR_INVALIDFLAG (MMAPERR)2  // invalid flag was passed
#define MMAPERR_INVALIDPORT (MMAPERR)3  // setup accesses invalid port
#define MMAPERR_INVALIDSETUP    (MMAPERR)4  // invalid setup name
#define MMAPERR_INVALIDPATCH    (MMAPERR)5  // invalid patchmap name
#define MMAPERR_INVALIDKEY  (MMAPERR)6  // invalid keymap name
#define MMAPERR_MEMORY      (MMAPERR)7  // "Out of memory."
#define MMAPERR_READ        (MMAPERR)8  // "Can't read file."
#define MMAPERR_WRITE       (MMAPERR)9  // "Can't write file."
#define MMAPERR_OPEN        (MMAPERR)10 // "Can't open file."
#define MMAPERR_OPEN_READONLY   (MMAPERR)11 // "Can't open file (read-only)."
#define MMAPERR_CREATE      (MMAPERR)12 // "Can't create file."
#define MMAPERR_UNDEFINED   (MMAPERR)13 // "Internal error."
#define MMAPERR_FULL        (MMAPERR)14 // "Map table full."

#define MMAPERR_MAXERROR    (MMAPERR)15

/*
 * Map flag definitions.
 */
#define MMAP_SETUP      0x01        // map is a setup
#define MMAP_PATCH      0x02        // map is a patchmap
#define MMAP_KEY        0x04        // map is a keymap

#define MMAP_PORTS      0xFF        // enumerate ports

/*
 * String length definitions.
 */
#define MMAP_MAXNAME        16      // max map name length
#define MMAP_MAXDESC        32      // max map description length
#define MMAP_MAXCFGNAME     24      // max file name len for cfg file

/*
 * MIDICHANNELMAP definitions.
 */
// dwFlags
#define MMAP_ACTIVE     0x00000001  // is this channel active?
#define MMAP_PATCHMAP       0x00000002  // is there a patch map for
                        // this channel?

#define MMAP_ID_NOPORT      0xFFFF      // chnl mapped to [none] entry

/****************************************************************************

   MIDI output mapping support structures

*****************************************************************************/

typedef struct midichannelmap_tag {
    DWORD       dwFlags;        // flags for channel map
    WORD        wDeviceID;      // port ID of device
    WORD        wChannel;       // port channel of device
    DWORD       oPMap;          // offset of patch map
} MIDICHANNELMAP, FAR *LPMIDICHANNELMAP;

typedef struct midimap_tag {
    DWORD       dwFlags;        // flags (none defined yet)
    BYTE        szName[MMAP_MAXNAME];   // name
    BYTE        szDesc[MMAP_MAXDESC];   // description
    MIDICHANNELMAP  chMap[16];      // array of channel maps
} MIDIMAP, FAR *LPMIDIMAP;

typedef struct midipatchmap_tag {
    DWORD       dwFlags;        // flags (none defined yet)
    BYTE        szName[MMAP_MAXNAME];   // name
    BYTE        szDesc[MMAP_MAXDESC];   // description
    WORD        wPMap[MIDIPATCHSIZE];   // lobyte=patch, hibyte=volume
    DWORD       okMaps[MIDIPATCHSIZE];  // offsets of key maps
    BYTE        bVMax;          // max volume scalar
    BYTE        bDummy;         // alignment byte
    WORD        wDummy;         // alignment word
} MIDIPATCHMAP, FAR *LPMIDIPATCHMAP;

typedef struct midikeymap_tag {
    DWORD       dwFlags;        // flags (none defined yet)
    BYTE        szName[MMAP_MAXNAME];   // name
    BYTE        szDesc[MMAP_MAXDESC];   // description
    BYTE        bKMap[MIDIPATCHSIZE];   // xlat table for key number
} MIDIKEYMAP, FAR *LPMIDIKEYMAP;

/****************************************************************************

   MIDI output mapping support routines

*****************************************************************************/

typedef BOOL    (CALLBACK *ENUMPROC) (LPSTR, LPSTR, UINT, HWND, LPSTR);

MMAPERR     FAR PASCAL     mapSetCurrentSetup (LPSTR);
MMAPERR     FAR PASCAL     mapGetCurrentSetup (LPSTR, UINT);

MMAPERR     FAR PASCAL     mapDelete (UINT, LPSTR);
MMAPERR     FAR PASCAL     mapEnumerate ( UINT uFlag
                                        , ENUMPROC lpfnCallback
                                        , UINT uCase        // passed to lpfnCallback
                                        , HWND hCombo       // passed to lpfnCallback
                                        , LPSTR lpSetupName // passed to lpfnCallback
                                        );

DWORD       FAR PASCAL     mapGetSize (UINT, LPSTR);
DWORD       FAR PASCAL     mapGetPatchSize(LPMIDIPATCHMAP lpPatch);

MMAPERR     FAR PASCAL     mapRead (UINT, LPSTR, LPVOID);
MMAPERR     FAR PASCAL     mapWrite (UINT, LPVOID);

DWORD       FAR PASCAL     mapGetUsageCount (UINT, LPSTR);
DWORD       FAR PASCAL     mapPatchMapInSetup (LPSTR, LPSTR);
DWORD       FAR PASCAL     mapKeyMapInSetup (LPSTR, LPSTR);
DWORD       FAR PASCAL     mapExists (UINT, LPSTR);

MMAPERR     FAR PASCAL     mapInitMapFile (void);
DWORD       FAR PASCAL     mapFileVersion (void);

// services from midi.c
void        FAR PASCAL     mapUnlock(void);
BOOL        FAR PASCAL     mapLock(void);

// death fix
void        FAR PASCAL mapConnect(LPSTR);
void        FAR PASCAL mapDisconnect(void);

/************************************************************************/
/*
**  Edit structures.
*/

typedef struct tagChannel {
    DWORD   dFlags;
    WORD    wDeviceID;
    WORD    wChannel;
    char    aszPatchName[MMAP_MAXNAME];
}   CHANNEL;

typedef struct tagSetup {
    DWORD   dFlags;
    char    aszSetupName[MMAP_MAXNAME];
    char    aszSetupDescription[MMAP_MAXDESC];
    CHANNEL channels[16];
}   SETUP;

typedef struct tagKeyMap {
    BYTE    bVolume;
    BYTE    bDestination;
    char    aszKeyMapName[MMAP_MAXNAME];
}   KEYMAP;

typedef struct tagPatchMap {
    char    aszPatchMapName[MMAP_MAXNAME];
    char    aszPatchMapDescription[MMAP_MAXDESC];
    KEYMAP  keymaps[128];
}   PATCHMAP;

MMAPERR FAR PASCAL mapReadSetup(
    LPSTR   lszSetupName,
    SETUP FAR*  lpSetup);
MMAPERR FAR PASCAL mapReadPatchMap(
    LPSTR   lszPatchMapName,
    PATCHMAP FAR*   lpPatch);

/* place window so as to avoid going off screen */
VOID PlaceWindow(HWND hwnd);

/************************************************************************/

/*
** Values used to communicate between the caller of mapEnumerate and the
** function called by mapEnumerate for each element.
*/

#define MMENUM_BASIC     0
#define MMENUM_INTOCOMBO 1
#define MMENUM_DELETE    2
