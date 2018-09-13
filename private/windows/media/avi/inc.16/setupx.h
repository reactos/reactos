//**********************************************************************
//
// SETUPX.H
//
//  Copyright (c) 1993 - Microsoft Corp.
//  All rights reserved.
//  Microsoft Confidential
//
// Public include file for Chicago Setup services.
//
// 12/1/93      DONALDM     Added LPCLASS_INFO, and function protos for
//                          exports in SETUP4.DLL
// 12/4/943     DONALDM     Moved SHELL.H include and Chicago specific
//                          helper functions to SETUP4.H
// 1/11/94      DONALDM     Added members to DEVICE_INFO to better handle
//                          ClassInstaller load/unload.
// 1/11/94      DONALDM     Added some new DIF_ messages for Net guys.
// 2/25/94      DONALDM     Fixed a bug with the DIREG_ flags
//**********************************************************************

#ifndef SETUPX_INC
#define SETUPX_INC   1                   // SETUPX.H signature

/***************************************************************************/
// setup PropertySheet support
// NOTE:  Always include PRST.H because it is needed later for Class Installer
// stuff, and optionally define the SU prop sheet stuff.
/***************************************************************************/
#include <prsht.h>
#ifndef NOPRSHT
HPROPSHEETPAGE  WINAPI SUCreatePropertySheetPage(LPCPROPSHEETPAGE lppsp);
BOOL            WINAPI SUDestroyPropertySheetPage(HPROPSHEETPAGE hPage);
int             WINAPI SUPropertySheet(LPCPROPSHEETHEADER lppsh);
#endif // NOPRSHT

typedef UINT RETERR;             // Return Error code type.

#define OK 0                     // success error code

#define IP_ERROR       (100)    // Inf parsing
#define TP_ERROR       (200)    // Text processing module
#define VCP_ERROR      (300)    // Virtual copy module
#define GEN_ERROR      (400)    // Generic Installer
#define DI_ERROR       (500)    // Device Installer

// err2ids mappings
enum ERR_MAPPINGS {
    E2I_VCPM,           // Maps VCPM to strings
    E2I_SETUPX,         // Maps setupx returns to strings
    E2I_SETUPX_MODULE,  // Maps setupx returns to appropriate module
    E2I_DOS_SOLUTION,   // Maps DOS Extended errors to solutions
    E2I_DOS_REASON,     // Maps DOS extended errors to strings.
    E2I_DOS_MEDIA,      // Maps DOS extended errors to media icon.
};

#ifndef NOVCP

/***************************************************************************/
//
// Logical Disk ID definitions
//
/***************************************************************************/

// DECLARE_HANDLE(VHSTR);           /* VHSTR = VirtCopy Handle to STRing */
typedef UINT VHSTR;         /* VirtCopy Handle to String */

VHSTR   WINAPI vsmStringAdd(LPCSTR lpszName);
int WINAPI vsmStringDelete(VHSTR vhstr);
VHSTR   WINAPI vsmStringFind(LPCSTR lpszName);
int WINAPI vsmGetStringName(VHSTR vhstr, LPSTR lpszBuffer, int cbBuffer);
int WINAPI vsmStringCompare(VHSTR vhstrA, VHSTR vhstrB);
LPCSTR  WINAPI vsmGetStringRawName(VHSTR vhstr);
void    WINAPI vsmStringCompact(void);

typedef UINT LOGDISKID;          /* ldid */

// Logical Disk Descriptor: Structure which describes the physical attributes
// of a logical disk. Every logical disk is assigned a logical disk
// identifier (LDID), and is described by a logical disk descriptor (LDD).
//
// The cbSize structure member must always be set to sizeof(LOGDISKDESC_S),
// but all other unused structure members should be NULL or 0. No validation
// is performed on the size of string arrays; all string pointers, if
// non-NULL and they are to receive a string, must point at string arrays
// whose sizes are as follows:
//      sizeof( szPath )    = MAX_PATH_LEN
//      sizeof( szVolLabel) = MAX_FILENAME_LEN
//      sizeof( szName )    = MAX_STRING_LEN
#define MAX_PATH_LEN        260     // Max. path length.
#define MAX_FILENAME_LEN    20      // Max. filename length. ( > sizeof( "x:\\12345678.123" )


typedef struct _LOGDISKDESC_S { /* ldd */
    WORD        cbSize;                 // Size of this structure (bytes)
    LOGDISKID   ldid;                   // Logical Disk ID.
    LPSTR       pszPath;                // Ptr. to associated Path string.
    LPSTR       pszVolLabel;            // Ptr. to Volume Label string.
    LPSTR       pszDiskName;            // Ptr. to Disk Name string.
    WORD        wVolTime;               // Volume label modification time.
    WORD        wVolDate;               // Volume label modification date.
    DWORD       dwSerNum;               // Disk serial number.
    WORD        wFlags;                 // Flags.
} LOGDISKDESC_S, FAR *LPLOGDISKDESC;


// Range for pre-defined LDIDs.
#define LDID_PREDEF_START   0x0001  // Start of range
#define LDID_PREDEF_END     0x7FFF  // End of range

// Range for dynamically assigned LDIDs.
#define LDID_ASSIGN_START   0x8000  // Start of range
#define LDID_ASSIGN_END     0xBFFF  // End of range

// Pre-defined Logical Disk Identifiers (LDID).
//
#define LDID_NULL       0               // Null (undefined) LDID.
#define LDID_ABSOLUTE   ((UINT)-1)      // Absolute path

// source path of windows install, this is typically A:\ or a net drive
#define LDID_SRCPATH    1   // source of instilation
// temporary setup directory used by setup, this is only valid durring
// regular install
#define LDID_SETUPTEMP  2   // temporary setup dir for install
// path to uninstall location, this is where we backup files that will
// be overwritten
#define LDID_UNINSTALL  3   // uninstall (backup) dir.
// backup path for the copy engine, this should not be used
#define LDID_BACKUP     4   // BUGBUG: backup dir for the copy engine, not used

// windows directory, this is the destinatio of the insallation
#define LDID_WIN        10  // destination Windows dir (just user files).
#define LDID_SYS        11  // destination Windows System dir.
#define LDID_IOS        12  // destination Windows Iosubsys dir.
#define LDID_CMD        13  // destination Windows Command (DOS) dir.
#define LDID_CPL        14  // destination Windows Control Panel dir.
#define LDID_PRINT      15  // destination Windows Printer dir.
#define LDID_MAIL       16  // destination Mail dir.
#define LDID_INF        17  // destination Windows *.INF dir.
#define LDID_HELP       18  // destination Windows Help dir.
#define LDID_WINADMIN   19  // admin stuff.

// Shared dirs for net install.
#define LDID_SHARED     25  // Bulk of windows files.
#define LDID_WINBOOT    26  // guarenteed boot device for windows.
#define LDID_MACHINE    27  // machine specific files.
#define LDID_HOST_WINBOOT   28

// boot and old win and dos dirs.
#define LDID_BOOT       30  // Root dir of boot drive
#define LDID_BOOT_HOST  31  // Root dir of boot drive host
#define LDID_OLD_WINBOOT    32  // Subdir off of Root (optional)
#define LDID_OLD_WIN    33  // old windows directory (if it exists)
#define LDID_OLD_DOS    34  // old dos directory (if it exists)


// Convert Ascii drive letter to Integer drive number ('A'=1, 'B'=2, ...).
#define DriveAtoI( chDrv )      ((int)(chDrv & 31))

// Convert Integer drive number to Ascii drive letter (1='A', 2='B', ...).
#define DriveItoA( iDrv )       ((char) (iDrv - 1 + 'A'))


// BUGBUG: change the names of these

RETERR WINAPI CtlSetLdd     ( LPLOGDISKDESC );
RETERR WINAPI CtlGetLdd     ( LPLOGDISKDESC );
RETERR WINAPI CtlFindLdd    ( LPLOGDISKDESC );
RETERR WINAPI CtlAddLdd     ( LPLOGDISKDESC );
RETERR WINAPI CtlDelLdd     ( LOGDISKID  );
RETERR WINAPI CtlGetLddPath ( LOGDISKID, LPSTR );


/***************************************************************************/
//
// Virtual File Copy definitions
//
/***************************************************************************/


typedef DWORD LPEXPANDVTBL;         /* BUGBUG -- clean this up */

enum _ERR_VCP
{
    ERR_VCP_IOFAIL = (VCP_ERROR + 1),       // File I/O failure
    ERR_VCP_STRINGTOOLONG,                  // String length limit exceeded
    ERR_VCP_NOMEM,                          // Insufficient memory to comply
    ERR_VCP_NOVHSTR,                        // No string handles available
    ERR_VCP_OVERFLOW,                       // Reference count would overflow
    ERR_VCP_BADARG,                         // Invalid argument to function
    ERR_VCP_UNINIT,                         // String library not initialized
    ERR_VCP_NOTFOUND ,                      // String not found in string table
    ERR_VCP_BUSY,                           // Can't do that now
    ERR_VCP_INTERRUPTED,                    // User interrupted operation
    ERR_VCP_BADDEST,                        // Invalid destination directory
    ERR_VCP_SKIPPED,                        // User skipped operation
    ERR_VCP_IO,                             // Hardware error encountered
    ERR_VCP_LOCKED,                         // List is locked
    ERR_VCP_WRONGDISK,                      // The wrong disk is in the drive
    ERR_VCP_CHANGEMODE,                     //
    ERR_VCP_LDDINVALID,                // Logical Disk ID Invalid.
    ERR_VCP_LDDFIND,                   // Logical Disk ID not found.
    ERR_VCP_LDDUNINIT,                 // Logical Disk Descriptor Uninitialized.
    ERR_VCP_LDDPATH_INVALID,
    ERR_VCP_NOEXPANSION,                // Failed to load expansion dll
    ERR_VCP_NOTOPEN,                    // Copy session not open
};


/*****************************************************************************
 *              Structures
 *****************************************************************************/

/*---------------------------------------------------------------------------*
 *                  VCPPROGRESS
 *---------------------------------------------------------------------------*/

typedef struct tagVCPPROGRESS { /* prg */
    DWORD   dwSoFar;            /* Number of units copied so far */
    DWORD   dwTotal;            /* Number of units to copy */
} VCPPROGRESS, FAR *LPVCPPROGRESS;

/*---------------------------------------------------------------------------*
 *                  VCPDISKINFO
 *---------------------------------------------------------------------------*/


typedef struct tagVCPDISKINFO {
    WORD        cbSize;         /* Size of this structure in bytes */
    LOGDISKID   ldid;           /* Logical disk ID */
    VHSTR       vhstrRoot;      /* Location of root directory */
    VHSTR       vhstrVolumeLabel;/* Volume label */
    VHSTR       vhstrDiskName;  // Printed name on the disk.
    WORD        wVolumeTime;    /* Volume label modification time */
    WORD        wVolumeDate;    /* Volume label modification date */
    DWORD       dwSerialNumber; /* Disk serial number */
    WORD        fl;             /* Flags */
    LPARAM      lparamRef;      /* Reference data for client */

    VCPPROGRESS prgFileRead;    /* Progress info */
    VCPPROGRESS prgByteRead;

    VCPPROGRESS prgFileWrite;
    VCPPROGRESS prgByteWrite;

} VCPDISKINFO, FAR *LPVCPDISKINFO;

#define VDIFL_VALID     0x0001  /* Fields are valid from a prev. call */
#define VDIFL_EXISTS    0x0002  /* Disk exists; do not format */

RETERR WINAPI DiskInfoFromLdid(LOGDISKID ldid, LPVCPDISKINFO lpdi);


/*---------------------------------------------------------------------------*
 *                  VCPFILESPEC
 *---------------------------------------------------------------------------*/

typedef struct tagVCPFILESPEC { /* vfs */
    LOGDISKID   ldid;           /* Logical disk */
    VHSTR       vhstrDir;       /* Directory withing logical disk */
    VHSTR       vhstrFileName;  /* Filename within directory */
} VCPFILESPEC, FAR *LPVCPFILESPEC;

/*---------------------------------------------------------------------------*
 *              VCPFATTR
 *---------------------------------------------------------------------------*/

/*
 * BUGBUG -- explain diffce between llenIn and llenOut wrt compression.
 */
typedef struct tagVCPFATTR {
    UINT    uiMDate;            /* Modification date */
    UINT    uiMTime;            /* Modification time */
    UINT    uiADate;            /* Access date */
    UINT    uiATime;            /* Access time */
    UINT    uiAttr;             /* File attribute bits */
    DWORD   llenIn;             /* Original file length */
    DWORD   llenOut;            /* Final file length */
                                /* (after decompression) */
} VCPFATTR, FAR *LPVCPFATTR;

/*---------------------------------------------------------------------------*
 *                  VIRTNODEEX
 *---------------------------------------------------------------------------*/
typedef struct tagVIRTNODEEX
{    /* vnex */
    HFILE           hFileSrc;
    HFILE           hFileDst;
    VCPFATTR        fAttr;
    WORD            dosError;   // The first/last error encountered
    VHSTR           vhstrFileName;  // The original destination name.
    WPARAM          vcpm;   // The message that was being processed.
} VIRTNODEEX, FAR *LPCVIRTNODEEX, FAR *LPVIRTNODEEX ;


/*---------------------------------------------------------------------------*
 *                  VIRTNODE
 *---------------------------------------------------------------------------*/

typedef struct tagVIRTNODE {    /* vn */
    WORD            cbSize;
    VCPFILESPEC     vfsSrc;
    VCPFILESPEC     vfsDst;
    WORD            fl;
    LPARAM          lParam;
    LPEXPANDVTBL    lpExpandVtbl;
    LPVIRTNODEEX    lpvnex;
} VIRTNODE, FAR *LPCVIRTNODE, FAR *LPVIRTNODE ;


/*---------------------------------------------------------------------------*
 *              VCPDESTINFO
 *---------------------------------------------------------------------------*/

typedef struct tagVCPDESTINFO { /* destinfo */
    WORD    flDevAttr;          /* Device attributes */
    LONG    cbCapacity;         /* Disk capacity */
    WORD    cbCluster;          /* Bytes per cluster */
    WORD    cRootDir;           /* Size of root directory */
} VCPDESTINFO, FAR *LPVCPDESTINFO;

#define DIFL_FIXED      0x0001  /* Nonremoveable media */
#define DIFL_CHANGELINE 0x0002  /* Change line support */

// Now also used by the virtnode as we dont have copy nodes any more.
// #define CNFL_BACKUP             0x0001  /* This is a backup node */
#define CNFL_DELETEONFAILURE    0x0002  /* Dest should be deleted on failure */
#define CNFL_RENAMEONSUCCESS    0x0004  /* Dest needs to be renamed */
#define CNFL_CONTINUATION       0x0008  /* Dest is continued onto difft disk */
#define CNFL_SKIPPED            0x0010  /* User asked to skip file */
#define CNFL_IGNOREERRORS       0x0020  // An error has occured on this file already
#define CNFL_RETRYFILE          0x0040  // Retry the file (error ocurred)
#define CNFL_COPIED				0x0080  // Node has already been copied.

// BUGBUG: verify the use and usefullness of these flags
// #define VNFL_UNIQUE          0x0000  /* Default */
#define VNFL_MULTIPLEOK         0x0100  /* Do not search PATH for duplicates */
#define VNFL_DESTROYOLD         0x0200  /* Do not back up files */
// #define VNFL_NOW             0x0400  /* Use by vcp Flush */
// To deternime what kind of node it is.
#define VNFL_COPY			    0x0000  // A simple copy node.
#define VNFL_DELETE             0x0800  // A delete node
#define VNFL_RENAME             0x1000  // A rename node
#define VNFL_NODE_TYPE		    ( VNFL_RENAME|VNFL_DELETE|VNFL_COPY )
    /* Read-only flag bits */
#define VNFL_CREATED            0x2000  /* VCPM_NODECREATE has been sent */
#define VNFL_REJECTED           0x4000  /* Node has been rejected */

#define VNFL_DEVICEINSTALLER    0x8000     /* Node was added by the Device Installer */


/*---------------------------------------------------------------------------*
 *                  VCPSTATUS
 *---------------------------------------------------------------------------*/

typedef struct tagVCPSTATUS {   /* vstat */
    WORD    cbSize;             /* Size of this structure */

    VCPPROGRESS prgDiskRead;
    VCPPROGRESS prgFileRead;
    VCPPROGRESS prgByteRead;

    VCPPROGRESS prgDiskWrite;
    VCPPROGRESS prgFileWrite;
    VCPPROGRESS prgByteWrite;

    LPVCPDISKINFO lpvdiIn;      /* Current input disk */
    LPVCPDISKINFO lpvdiOut;     /* Current output disk */
    LPVIRTNODE    lpvn;            /* Current file */

} VCPSTATUS, FAR *LPVCPSTATUS;

/*---------------------------------------------------------------------------*
 *                  VCPVERCONFLICT
 *---------------------------------------------------------------------------*/

typedef struct tagVCPVERCONFLICT {

    LPCSTR  lpszOldFileName;
    LPCSTR  lpszNewFileName;
    DWORD   dwConflictType;     /* Same values as VerInstallFiles */
    LPVOID  lpvinfoOld;         /* Version information resources */
    LPVOID  lpvinfoNew;
    WORD    wAttribOld;         /* File attributes for original */
    LPARAM  lparamRef;          /* Reference data for callback */

} VCPVERCONFLICT, FAR *LPVCPVERCONFLICT;

/*****************************************************************************
 *              Callback functions
 *****************************************************************************/

typedef LRESULT (CALLBACK *VIFPROC)(LPVOID lpvObj, UINT uMsg, WPARAM wParam, LPARAM lParam, LPARAM lparamRef);

LRESULT CALLBACK vcpDefCallbackProc(LPVOID lpvObj, UINT uMsg, WPARAM wParam, LPARAM lParam, LPARAM lparamRef);

// callback for default UI.
// lparamRef --> a VCPUIINFO structure
LRESULT CALLBACK vcpUICallbackProc(LPVOID lpvObj, UINT uMsg, WPARAM wParam, LPARAM lParam, LPARAM lparamRef);


/*---------------------------------------------------------------------------*
 *                  VCPUIINFO
 *
 * This structure is passed in as the lparamRef of vcpUICallbackProc.
 *
 * on using vcpUICallbackProc:
 * - to use, have vcpUICallbackProc as the callback for vcpOpen with
 *   an appropriately filled in VCPUIINFO structure as the lparamRef.
 *
 * - based on flags, hwndProgress is created and maintained
 * - lpfnStatCallback is called with only status messages
 *     returning VCPM_ABORT indicates that the copy should be aborted
 * - if hwndProgress is non-NULL, the control with idProgress will
 *     receive progress gauge messages as appropriate
 *
 *---------------------------------------------------------------------------*/
#define VCPUI_CREATEPROGRESS 0x0001 // callback should create and manage progress gauge dialog
#define VCPUI_NOBROWSE       0x0002 // no browse button in InsertDisk
#define VCPUI_RENAMEREQUIRED 0x0004 // as a result of a file being in use at copy, reboot required

typedef struct {
    UINT flags;
    HWND hwndParent;            // window of parent
    HWND hwndProgress;          // window to get progress updates (nonzero ids)
    UINT idPGauge;              // id for progress gauge
    VIFPROC lpfnStatCallback;   // callback for status info (or NULL)
    LPARAM lUserData;           // caller definable data
    LOGDISKID ldidCurrent;      // reserved.  do not touch.
} VCPUIINFO, FAR *LPVCPUIINFO;

/******************************************************************************
 *          Callback notification codes
 *****************************************************************************/

    /* BUGBUG -- VCPN_ABORT should match VCPERROR_INTERRUPTED */

#define VCPN_OK         0       /* All is hunky-dory */
#define VCPN_PROCEED        0   /* The same as VCPN_OK */

#define VCPN_ABORT      (-1)    /* Cancel current operation */
#define VCPN_RETRY      (-2)    /* Retry current operation */
#define VCPN_IGNORE     (-3)    /* Ignore error and continue */
#define VCPN_SKIP       (-4)    /* Skip this file and continue */
#define VCPN_FORCE      (-5)    /* Force an action */
#define VCPN_DEFER      (-6)    /* Save the action for later */
#define VCPN_FAIL       (-7)    /* Return failure back to caller */
#define VCPN_RETRYFILE  (-8)    // An error ocurred during file copy, do it again.

/*****************************************************************************
 *          Callback message numbers
 *****************************************************************************/

#define VCPM_CLASSOF(uMsg)  HIBYTE(uMsg)
#define VCPM_TYPEOF(uMsg)   (0x00FF & (uMsg))   // LOBYTE(uMsg)

/*---------------------------------------------------------------------------*
 *          ERRORs
 *---------------------------------------------------------------------------*/

#define VCPM_ERRORCLASSDELTA    0x80
#define VCPM_ERRORDELTA         0x8000      /* Where the errors go */

/*---------------------------------------------------------------------------*
 *          Disk information callbacks
 *---------------------------------------------------------------------------*/

#define VCPM_DISKCLASS      0x01
#define VCPM_DISKFIRST      0x0100
#define VCPM_DISKLAST       0x01FF

enum tagVCPM_DISK {

    VCPM_DISKCREATEINFO = VCPM_DISKFIRST,
    VCPM_DISKGETINFO,
    VCPM_DISKDESTROYINFO,
    VCPM_DISKPREPINFO,

    VCPM_DISKENSURE,
    VCPM_DISKPROMPT,

    VCPM_DISKFORMATBEGIN,
    VCPM_DISKFORMATTING,
    VCPM_DISKFORMATEND,

    /* Remaining messages reserved for future use */
};

/*---------------------------------------------------------------------------*
 *          File copy callbacks
 *---------------------------------------------------------------------------*/

// BUGBUG: this needs to be merged back with other internal errors
#define VCPERROR_IO         (VCP_ERROR - ERR_VCP_IO)            /* Hardware error encountered */

#define VCPM_FILEINCLASS    0x02
#define VCPM_FILEOUTCLASS   0x03
#define VCPM_FILEFIRSTIN    0x0200
#define VCPM_FILEFIRSTOUT   0x0300
#define VCPM_FILELAST       0x03FF

enum tagVCPM_FILE {
    VCPM_FILEOPENIN = VCPM_FILEFIRSTIN,
    VCPM_FILEGETFATTR,
    VCPM_FILECLOSEIN,
    VCPM_FILECOPY,
    VCPM_FILENEEDED,

    VCPM_FILEOPENOUT = VCPM_FILEFIRSTOUT,
    VCPM_FILESETFATTR,
    VCPM_FILECLOSEOUT,
    VCPM_FILEFINALIZE,
    VCPM_FILEDELETE,
    VCPM_FILERENAME,
    /* Remaining messages reserved for future use */
};

/*---------------------------------------------------------------------------*
 *          VIRTNODE callbacks
 *---------------------------------------------------------------------------*/

#define VCPM_NODECLASS  0x04
#define VCPM_NODEFIRST  0x0400
#define VCPM_NODELAST   0x04FF

enum tagVCPM_NODE {
    VCPM_NODECREATE = VCPM_NODEFIRST,
    VCPM_NODEACCEPT,
    VCPM_NODEREJECT,
    VCPM_NODEDESTROY,
    VCPM_NODECHANGEDESTDIR,
    VCPM_NODECOMPARE,
    /* Remaining messages reserved for future use */
};

/*---------------------------------------------------------------------------*
 *          TALLY callbacks
 *---------------------------------------------------------------------------*/

#define VCPM_TALLYCLASS     0x05
#define VCPM_TALLYFIRST     0x0500
#define VCPM_TALLYLAST      0x05FF

enum tagVCPM_TALLY {
    VCPM_TALLYSTART = VCPM_TALLYFIRST,
    VCPM_TALLYEND,
    VCPM_TALLYFILE,
    VCPM_TALLYDISK,
    /* Remaining messages reserved for future use */
};

/*---------------------------------------------------------------------------*
 *          VER callbacks
 *---------------------------------------------------------------------------*/

#define VCPM_VERCLASS       0x06
#define VCPM_VERFIRST       0x0600
#define VCPM_VERLAST        0x06FF

enum tagVCPM_VER {
    VCPM_VERCHECK = VCPM_VERFIRST,
    VCPM_VERCHECKDONE,
    VCPM_VERRESOLVECONFLICT,
    /* Remaining messages reserved for future use */
};

/*---------------------------------------------------------------------------*
 *          VSTAT callbacks
 *---------------------------------------------------------------------------*/

#define VCPM_VSTATCLASS     0x07
#define VCPM_VSTATFIRST     0x0700
#define VCPM_VSTATLAST      0x07FF

enum tagVCPM_VSTAT {
    VCPM_VSTATSTART = VCPM_VSTATFIRST,
    VCPM_VSTATEND,
    VCPM_VSTATREAD,
    VCPM_VSTATWRITE,
    VCPM_VSTATNEWDISK,

    VCPM_VSTATCLOSESTART,       // Start of VCP close
    VCPM_VSTATCLOSEEND,         // upon leaving VCP close
    VCPM_VSTATBACKUPSTART,      // Backup is beginning
    VCPM_VSTATBACKUPEND,        // Backup is finished
    VCPM_VSTATRENAMESTART,      // Rename phase start/end
    VCPM_VSTATRENAMEEND,
    VCPM_VSTATCOPYSTART,        // Acutal copy phase
    VCPM_VSTATCOPYEND,
    VCPM_VSTATDELETESTART,      // Delete phase
    VCPM_VSTATDELETEEND,
    VCPM_VSTATPATHCHECKSTART,   // Check for valid paths
    VCPM_VSTATPATHCHECKEND,
    VCPM_VSTATUSERABORT,        // User wants to quit.
    /* Remaining messages reserved for future use */
};

/*---------------------------------------------------------------------------*
 *          Destination info callbacks
 *---------------------------------------------------------------------------*/

/* BUGBUG -- find a reasonable message range for this */
#define VCPM_PATHCLASS      0x08
#define VCPM_PATHFIRST      0x0800
#define VCPM_PATHLAST       0x08FF

enum tagVCPM_PATH{
    VCPM_BUILDPATH = VCPM_PATHFIRST,
    VCPM_UNIQUEPATH,
    VCPM_CHECKPATH,
};

// #define VCPM_BUILDPATH      0x83

/*****************************************************************************/
void WINAPI VcpAddMRUPath( LPCSTR lpszPath );
#define SZ_INSTALL_LOCATIONS "InstallLocationsMRU"


RETERR WINAPI VcpOpen(VIFPROC vifproc, LPARAM lparamMsgRef);

RETERR WINAPI VcpClose(WORD fl, LPCSTR lpszBackupDest);

RETERR WINAPI VcpFlush(WORD fl, LPCSTR lpszBackupDest);

#define VCPFL_ABANDON           0x0000  /* Abandon all pending file copies */
#define VCPFL_BACKUP            0x0001  /* Perform backup */
#define VCPFL_COPY              0x0002  /* Copy files */
#define VCPFL_BACKUPANDCOPY     (VCPFL_BACKUP | VCPFL_COPY)
#define VCPFL_INSPECIFIEDORDER  0x0004  /* Do not sort before copying */
#define VCPFL_DELETE            0x0008
#define VCPFL_RENAME            0x0010
#define VCPFL_DIAMOND           0x0020

typedef int (CALLBACK *VCPENUMPROC)(LPVIRTNODE lpvn, LPARAM lparamRef);

int WINAPI vcpEnumFiles(VCPENUMPROC vep, LPARAM lparamRef);

enum tag_VCPM_EXPLAIN{
    VCPEX_SRC_DISK,
    VCPEX_SRC_CABINET,
    VCPEX_SRC_LOCN,
    VCPEX_DST_LOCN,
    VCPEX_SRC_FILE,
    VCPEX_DST_FILE,
    VCPEX_DOS_ERROR,
    VCPEX_MESSAGE,
    VCPEX_DOS_SOLUTION,
    VCPEX_SRC_FULL,
    VCPEX_DST_FULL,
};

LPCSTR WINAPI VcpExplain( LPVIRTNODE lpVn, DWORD dwWhat );

/* Flag bits that can be set via VcpQueueCopy */


// Various Lparams for files
#define VNLP_SYSCRITICAL    0x0001  // This file should not be skipped
#define VNLP_SETUPCRITICAL  0x0002  // This file cannot be skipped

// VcpEnumFiles Flags.

#define VEN_OP      0x00ff      /* Operation field */

#define VEN_NOP     0x0000      /* Do nothing */
#define VEN_DELETE  0x0001      /* Delete current item */
#define VEN_SET     0x0002      /* Change value of current item */

#define VEN_FL      0xff00      /* Flags field */

#define VEN_STOP    0x0100      /* Stop enumeration after this item */
#define VEN_ERROR   0x8000      /* Stop enumeration after this item
                                 * and ignore the OP field */



// BUGBUG: add the other VCP stuff necessary to use this

// BUGBUG: remove the lpsz*Dir fields, make overload the LDID with them

RETERR WINAPI VcpQueueCopy(LPCSTR lpszSrcFileName, LPCSTR lpszDstFileName,
                LPCSTR lpszSrcDir, LPCSTR lpszDstDir,
                LOGDISKID ldidSrc, LOGDISKID ldidDst,
                LPEXPANDVTBL lpExpandVtbl, WORD fl,
                LPARAM lParam);

RETERR WINAPI VcpQueueDelete( LPCSTR lpszDstFileName,
                              LPCSTR lpszDstDir,
                              LOGDISKID ldidDst,
                              LPARAM lParamRef );

RETERR WINAPI VcpQueueRename( LPCSTR      lpszSrcFileName,
                            LPCSTR      lpszDstFileName,
                            LPCSTR      lpszSrcDir,
                            LPCSTR      lpszDstDir,
                            LOGDISKID   ldidSrc,
                            LOGDISKID   ldidDst,
                            LPARAM      lParam );

#endif // NOVCP



#ifndef NOINF
/***************************************************************************/
//
// Inf Parser API declaration and definitions
//
/***************************************************************************/

enum _ERR_IP
{
    ERR_IP_INVALID_FILENAME = (IP_ERROR + 1),
    ERR_IP_ALLOC_ERR,
    ERR_IP_INVALID_SECT_NAME,
    ERR_IP_OUT_OF_HANDLES,
    ERR_IP_INF_NOT_FOUND,
    ERR_IP_INVALID_INFFILE,
    ERR_IP_INVALID_HINF,
    ERR_IP_INVALID_FIELD,
    ERR_IP_SECT_NOT_FOUND,
    ERR_IP_END_OF_SECTION,
    ERR_IP_PROFILE_NOT_FOUND,
    ERR_IP_LINE_NOT_FOUND,
    ERR_IP_FILEREAD,
    ERR_IP_TOOMANYINFFILES,
    ERR_IP_INVALID_SAVERESTORE,
    ERR_IP_INVALID_INFTYPE
};

#define INFTYPE_TEXT                0
#define INFTYPE_EXECUTABLE          1

#define MAX_SECT_NAME_LEN    32

typedef struct _INF NEAR * HINF;
typedef struct _INFLINE FAR * HINFLINE;            // tolken to inf line

RETERR  WINAPI IpOpen(LPCSTR pszFileSpec, HINF FAR * lphInf);
RETERR  WINAPI IpOpenEx(LPCSTR pszFileSpec, HINF FAR * lphInf, UINT InfType);
RETERR  WINAPI IpOpenAppend(LPCSTR pszFileSpec, HINF hInf);
RETERR  WINAPI IpOpenAppendEx(LPCSTR pszFileSpec, HINF hInf, UINT InfType);
RETERR  WINAPI IpSaveRestorePosition(HINF hInf, BOOL bSave);
RETERR  WINAPI IpClose(HINF hInf);
RETERR  WINAPI IpGetLineCount(HINF hInf, LPCSTR lpszSection, int FAR * lpCount);
RETERR  WINAPI IpFindFirstLine(HINF hInf, LPCSTR lpszSect, LPCSTR lpszKey, HINFLINE FAR * lphRet);
RETERR  WINAPI IpFindNextLine(HINF hInf, HINFLINE FAR * lphRet);
RETERR  WINAPI IpFindNextMatchLine(HINF hInf, LPCSTR lpszKey, HINFLINE FAR * lphRet);
RETERR  WINAPI IpGetProfileString(HINF hInf, LPCSTR lpszSec, LPCSTR lpszKey, LPSTR lpszBuf, int iBufSize);
RETERR  WINAPI IpGetFieldCount(HINF hInf, HINFLINE hInfLine, int FAR * lpCount);
RETERR  WINAPI IpGetFileName(HINF hInf, LPSTR lpszBuf, int iBufSize);
RETERR  WINAPI IpGetIntField(HINF hInf, HINFLINE hInfLine, int iField, int FAR * lpVal);
RETERR  WINAPI IpGetLongField(HINF hInf, HINFLINE hInfLine, int iField, long FAR * lpVal);
RETERR  WINAPI IpGetStringField(HINF hInf, HINFLINE hInfLine, int iField, LPSTR lpBuf, int iBufSize, int FAR * lpuCount);
RETERR  WINAPI IpGetVersionString(LPSTR lpszInfFile, LPSTR lpszValue, LPSTR lpszBuf, int cbBuf, LPSTR lpszDefaultValue);

#endif // NOINF



#ifndef NOTEXTPROC
/***************************************************************************/
//
// Text processing API declaration and definitions
//
/***************************************************************************/

/* Relative/absolute positioning */
#define SEC_SET 1       // Absolute positioning (relative to the start)
#define SEC_END 2       // Realtive to the end
#define SEC_CUR 3       // Relative to the current line.

#define SEC_OPENALWAYS          1   // Always open a section, no error if it does not exist
#define SEC_OPENEXISTING        2   // Open an existing section, an error given if it does not exist.
#define SEC_OPENNEWALWAYS       3   // Open a section (present or not) and discard its contents.
#define SEC_OPENNEWEXISTING     4   // Open an existing section (discarding its contents). Error if not existing

// Flags for TP_OpenFile().
//
  // Use autoexec/config.sys key delimiters
  //
#define TP_WS_KEEP      1

  // If TP code running under SETUP, the foll. flag specifies whether
  // to cache this file or not! Use this, if you want to read a whole
  // file in when doing the TpOpenSection()!
  //
#define TP_WS_DONTCACHE 2

// The following are simple errors
enum {
    ERR_TP_NOT_FOUND = (TP_ERROR + 1),  // line, section, file etc.
                    // not necessarily terminal
    ERR_TP_NO_MEM,      // couldn't perform request - generally terminal
    ERR_TP_READ,        // could not read the disc - terminal
    ERR_TP_WRITE,       // could not write the data - terminal.
    ERR_TP_INVALID_REQUEST, // Multitude of sins - not necessarily terminal.
    ERR_TP_INVALID_LINE         // Invalid line from DELETE_LINE etc.
};

/* Data handles */
DECLARE_HANDLE(HTP);
typedef HTP FAR * LPHTP;

/* File handles */
DECLARE_HANDLE(HFN);
typedef HFN FAR * LPHFN;

typedef UINT TFLAG;
typedef UINT LINENUM, FAR * LPLINENUM;

#define MAX_REGPATH     256     // Max Registry Path Length
#define LINE_LEN        256     // BUGBUG: max line length?
#define SECTION_LEN     32      // BUGBUG: max length of a section name?
#define MAX_STRING_LEN  512     // BUGBUG: review this

/* Function prototypes */
RETERR  WINAPI TpOpenFile(LPCSTR Filename, LPHFN phFile, TFLAG Flag);
RETERR  WINAPI TpCloseFile(HFN hFile);
RETERR  WINAPI TpOpenSection(HFN hfile, LPHTP phSection, LPCSTR Section, TFLAG flag);
RETERR  WINAPI TpCloseSection(HTP Section);
RETERR  WINAPI TpCommitSection(HFN hFile, HTP hSection, LPCSTR Section, TFLAG flag);
LINENUM WINAPI TpGetLine(HTP hSection, LPCSTR key, LPCSTR value, int rel, int orig, LPLINENUM lpLineNum );
LINENUM WINAPI TpGetNextLine(HTP hSection, LPCSTR key, LPCSTR value, LPLINENUM lpLineNum );
RETERR  WINAPI TpInsertLine(HTP hSection, LPCSTR key, LPCSTR value, int rel, int orig, TFLAG flag);
RETERR  WINAPI TpReplaceLine(HTP hSection, LPCSTR key, LPCSTR value, int rel, int orig, TFLAG flag);
RETERR  WINAPI TpDeleteLine(HTP hSection, int rel, int orig,TFLAG flag);
RETERR  WINAPI TpMoveLine(HTP hSection, int src_rel, int src_orig, int dest_rel, int dest_orig, TFLAG flag);
RETERR  WINAPI TpGetLineContents(HTP hSection, LPSTR buffer, UINT bufsize, UINT FAR * lpActualSize,int rel, int orig, TFLAG flag);

// UINT    WINAPI TpGetWindowsDirectory(LPSTR lpDest, UINT size);
// UINT    WINAPI TpGetSystemDirectory(LPSTR lpDest, UINT size);

int  WINAPI TpGetPrivateProfileString(LPCSTR lpszSect, LPCSTR lpszKey, LPCSTR lpszDefault, LPSTR lpszReturn, int nSize, LPCSTR lpszFile);
int  WINAPI TpWritePrivateProfileString(LPCSTR lpszSect, LPCSTR lpszKey, LPCSTR lpszString, LPCSTR lpszFile);
int  WINAPI TpGetProfileString(LPCSTR lpszSect, LPCSTR lpszKey, LPCSTR lpszDefault, LPSTR lpszReturn, int nSize);
BOOL WINAPI TpWriteProfileString(LPCSTR lpszSect , LPCSTR lpszKey , LPCSTR lpszString);

#endif // NOTEXTPROC



#ifndef NOGENINSTALL
/***************************************************************************/
//
// Generic Installer prototypes and definitions
//
/***************************************************************************/

enum _ERR_GENERIC
{
    ERR_GEN_LOW_MEM = GEN_ERROR+1,  // Insufficient memory.
    ERR_GEN_INVALID_FILE,           // Invalid INF file.
    ERR_GEN_LOGCONFIG,              // Can't process LogConfig=.
    ERR_GEN_CFGAUTO,                // Can't process CONFIG.SYS/AUTOEXEC.BAT
    ERR_GEN_UPDINI,                 // Can't process UpdateInis=.
    ERR_GEN_UPDINIFIELDS,           // Can't process UpdateIniFields=.
    ERR_GEN_ADDREG,                 // Can't process AddReg=.
    ERR_GEN_DELREG,                 // Can't process DelReg=.
    ERR_GEN_INI2REG,                // Can't process Ini2Reg=.
    ERR_GEN_FILE_COPY,              // Can't process CopyFiles=.
    ERR_GEN_FILE_DEL,               // Can't process DelFiles=.
    ERR_GEN_FILE_REN,               // Can't process RenFiles=.
    ERR_GEN_REG_API,                // Error returned by Reg API.
    ERR_GEN_DO_FILES,               // can't do Copy, Del or RenFiles.
};

// The cbSize field will always be set to sizeof(GENCALLBACKINFO_S).
// All unused fields (for the operation) will be not be initialized.
// For example, when the operation is GENO_DELFILE, the Src fields will
// not have any sensible values (Dst fields will be set correctly) as
// VcpQueueDelete only accepts Dst parameters.
//
/***************************************************************************
 * GenCallbackINFO structure passed to GenInstall CallBack functions.
 ***************************************************************************/
typedef struct _GENCALLBACKINFO_S { /* gen-callback struc */
    WORD         cbSize;                 // Size of this structure (bytes).
    WORD         wOperation;             // Operation being performed.
    LOGDISKID    ldidSrc;                // Logical Disk ID for Source.
    LPCSTR       pszSrcSubDir;           // Source sub-dir off of the LDID.
    LPCSTR       pszSrcFileName;         // Source file name (base name).
    LOGDISKID    ldidDst;                // Logical Disk ID for Dest.
    LPCSTR       pszDstSubDir;           // Dest. sub-dir off of the LDID.
    LPCSTR       pszDstFileName;         // Dest. file name (base name).
    LPEXPANDVTBL lpExpandVtbl;           // BUGBUG needed? NULL right now!
    WORD         wflags;                 // flags for VcpQueueCopy.
    LPARAM       lParam;                 // LPARAM to the Vcp API.
} GENCALLBACKINFO_S, FAR *LPGENCALLBACKINFO;

/***************************************************************************
 * GenCallback notification codes -- callback proc returns 1 of foll. values.
 ***************************************************************************/
#define GENN_OK         0       /* All is hunky-dory. Do the VCP operation */
#define GENN_PROCEED    0       /* The same as GENN_OK */

#define GENN_ABORT      (-1)    /* Cancel current GenInstall altogether */
#define GENN_SKIP       (-2)    /* Skip this file and continue */

/***************************************************************************
 * VCP Operation being performed by GenInstall() -- wOperation values in
 * GENCALLBACKINFO structure above.
 ***************************************************************************/
#define GENO_COPYFILE   1       /* VCP copyfile being done */
#define GENO_DELFILE    2       /* VCP delfile being done */
#define GENO_RENFILE    3       /* VCP renfile being done */


typedef LRESULT (CALLBACK *GENCALLBACKPROC)(LPGENCALLBACKINFO lpGenInfo,
                                                            LPARAM lparamRef);

RETERR WINAPI GenInstall( HINF hinfFile, LPCSTR szInstallSection, WORD wFlags );
RETERR WINAPI GenInstallEx( HINF hInf, LPCSTR szInstallSection, WORD wFlags,
                                HKEY hRegKey, GENCALLBACKPROC CallbackProc,
                                LPARAM lparam);
RETERR WINAPI GenWinInitRename(LPCSTR szNew, LPSTR szOld, LOGDISKID ldid);
RETERR WINAPI GenCopyLogConfig2Reg(HINF hInf, HKEY hRegKey,
                                                LPCSTR szLogConfigSection);
void   WINAPI GenFormStrWithoutPlaceHolders( LPSTR szDst, LPCSTR szSrc,
                                                                HINF hInf ) ;

// A devnode is just a DWORD and this is easier than
// having to include configmg.h for everybody
RETERR WINAPI GenInfLCToDevNode(ATOM atInfFileName, LPSTR lpszSectionName,
                                BOOL bInstallSec, UINT InfType, 
                                DWORD dnDevNode);

// Bit fields for GenInstall() (for wFlags parameter) -- these can be OR-ed!
#define GENINSTALL_DO_FILES     1
#define GENINSTALL_DO_INI       2
#define GENINSTALL_DO_REG       4
#define GENINSTALL_DO_INI2REG   8
#define GENINSTALL_DO_CFGAUTO   16
#define GENINSTALL_DO_LOGCONFIG 32
#define GENINSTALL_DO_INIREG    (GENINSTALL_DO_INI | \
                                 GENINSTALL_DO_REG | \
                                 GENINSTALL_DO_INI2REG)
#define GENINSTALL_DO_ALL       (GENINSTALL_DO_FILES | \
                                    GENINSTALL_DO_INIREG | \
                                    GENINSTALL_DO_CFGAUTO | \
                                    GENINSTALL_DO_LOGCONFIG)

#endif // NOGENINSTALL



#ifndef NODEVICENSTALL
/***************************************************************************/
//
// Device Installer prototypes and definitions
//
/***************************************************************************/

enum _ERR_DEVICE_INSTALL
{
    ERR_DI_INVALID_DEVICE_ID = DI_ERROR,    // Incorrectly formed device IDF
    ERR_DI_INVALID_COMPATIBLE_DEVICE_LIST,  // Invalid compatible device list
    ERR_DI_REG_API,                         // Error returned by Reg API.
    ERR_DI_LOW_MEM,                         // Insufficient memory to complete
    ERR_DI_BAD_DEV_INFO,                    // Device Info struct invalid
    ERR_DI_INVALID_CLASS_INSTALLER,         // Registry entry / DLL invalid
    ERR_DI_DO_DEFAULT,                      // Take default action
    ERR_DI_USER_CANCEL,                     // the user cancelled the operation
    ERR_DI_NOFILECOPY,                      // No need to copy files (in install)
    ERR_DI_BAD_CLASS_INFO,                  // Class Info Struct invalid
    ERR_DI_BAD_INF,                         //  Bad INF file encountered
    ERR_DI_BAD_MOVEDEV_PARAMS,	            // Bad Move Device Params struct
    ERR_DI_NO_INF,		                    // No INF found on OEM disk
    ERR_DI_BAD_PROPCHANGE_PARAMS,           // Bad property change param struct
    ERR_DI_BAD_SELECTDEVICE_PARAMS,          // Bad Select Device Parameters
    ERR_DI_BAD_REMOVEDEVICE_PARAMS          // Bad Remove Device Parameters
};



typedef struct _DRIVER_NODE {
    struct _DRIVER_NODE FAR* lpNextDN;
    UINT    Rank;
    UINT    InfType;
    unsigned    InfDate;
    LPSTR   lpszDescription;        // Compatibility: Contains the Device Desc.
    LPSTR   lpszSectionName;
    ATOM    atInfFileName;
    ATOM    atMfgName;
    ATOM    atProviderName;
    DWORD   Flags;
    DWORD   dwPrivateData;
    LPSTR   lpszDrvDescription;     // New contains an driver description
    LPSTR   lpszHardwareID;
    LPSTR   lpszCompatIDs;
}   DRIVER_NODE, NEAR* PDRIVER_NODE, FAR* LPDRIVER_NODE, FAR* FAR* LPLPDRIVER_NODE;

#define DNF_DUPDESC    0x00000001   // Multiple providers have same desc
#define DNF_OLDDRIVER  0x00000002   // Driver node specifies old/current driver

// possible types of "INF" files
#define INFTYPE_WIN4        1
#define INFTYPE_WIN3        2
#define INFTYPE_COMBIDRV    3
#define INFTYPE_PPD         4
#define INFTYPE_WPD     5
#define INFTYPE_CLASS_SPEC1 6
#define INFTYPE_CLASS_SPEC2 7
#define INFTYPE_CLASS_SPEC3 8
#define INFTYPE_CLASS_SPEC4 9


#define MAX_CLASS_NAME_LEN   32
#define MAX_DRIVER_INST_LEN  10

// NOTE:  Keep this in sync with confimg.h in \DDK\INC
#define MAX_DEVNODE_ID_LEN  256

typedef struct _DEVICE_INFO
{
    UINT        cbSize;
    struct _DEVICE_INFO FAR* lpNextDi;
    char                szDescription[LINE_LEN];
    DWORD       dnDevnode;
    HKEY        hRegKey;
    char        szRegSubkey[MAX_DEVNODE_ID_LEN]; 
    char        szClassName[MAX_CLASS_NAME_LEN];
    DWORD       Flags;
    HWND        hwndParent;
    LPDRIVER_NODE   lpCompatDrvList;
    LPDRIVER_NODE   lpClassDrvList;
    LPDRIVER_NODE   lpSelectedDriver;
    ATOM    atDriverPath;
    ATOM    atTempInfFile;
    HINSTANCE   hinstClassInstaller;            // The Class Installer module
    HINSTANCE   hinstClassPropProvidor;         // The Class Property Providor
    HINSTANCE   hinstDevicePropProvidor;        // The Device Property Providor
    HINSTANCE   hinstBasicPropProvidor;         // The Basic Property Providor hinst
    FARPROC     fpClassInstaller;               // ClassInstall entry point
    FARPROC     fpClassEnumPropPages;           // Class EnumPropPages entry point
    FARPROC     fpDeviceEnumPropPages;          // Device EnumPropPages entry point
    FARPROC     fpEnumBasicProperties;          // Basic Property Page enum entry
    DWORD       dwSetupReserved;                // Reserved for SETUP's use
    DWORD       dwClassInstallReserved;         // Reserved for Class Installer use
    GENCALLBACKPROC gicpGenInstallCallBack;     // Set by Caller of DiInstallDevice if 
                                                // they want GenInstall callbacks
    LPARAM          gicplParam;                 // lParam for GenInstall Callback                                                
    UINT            InfType;                    // The type of INF file it ENUMSINGLEINF is
                                                // specified
    HINSTANCE   hinstPrivateProblemHandler;     // The Private Problem DLL for a Specific Device
    FARPROC     fpPrivateProblemHandler;        // The Private Problem Handler entry point
    LPARAM      lpClassInstallParams;           // Class Install functions specific Parameters
    struct _DEVICE_INFO FAR* lpdiChildList;     // Pointer to children Device Info list.
} DEVICE_INFO, FAR * LPDEVICE_INFO, FAR * FAR * LPLPDEVICE_INFO;

#define ASSERT_DI_STRUC(lpdi) if (lpdi->cbSize != sizeof(DEVICE_INFO)) return (ERR_DI_BAD_DEV_INFO)

typedef struct _CLASS_INFO
{
    UINT        cbSize;
    struct _CLASS_INFO FAR* lpNextCi;
    LPDEVICE_INFO   lpdi;
    char                szDescription[LINE_LEN];
    char        szClassName[MAX_CLASS_NAME_LEN];
} CLASS_INFO, FAR * LPCLASS_INFO, FAR * FAR * LPLPCLASS_INFO;
#define ASSERT_CI_STRUC(lpci) if (lpci->cbSize != sizeof(CLASS_INFO)) return (ERR_DI_BAD_CLASS_INFO)


// flags for device choosing (InFlags)
#define DI_SHOWOEM                  0x00000001L     // support Other... button
#define DI_SHOWCOMPAT               0x00000002L     // show compatibility list
#define DI_SHOWCLASS                0x00000004L     // show class list
#define DI_SHOWALL                  0x00000007L
#define DI_NOVCP                    0x00000008L     // Don't do vcpOpen/vcpClose.
#define DI_DIDCOMPAT                0x00000010L     // Searched for compatible devices
#define DI_DIDCLASS                 0x00000020L     // Searched for class devices
#define DI_AUTOASSIGNRES            0x00000040L    // No UI for resources if possible

// flags returned by DiInstallDevice to indicate need to reboot/restart
#define DI_NEEDRESTART              0x00000080L     // Restart required to take effect
#define DI_NEEDREBOOT               0x00000100L     // Reboot required to take effect

// flags for device installation
#define DI_NOBROWSE                 0x00000200L     // no Browse... in InsertDisk

// Flags set by DiBuildClassDrvList
#define DI_MULTMFGS                 0x00000400L     // Set if multiple manufacturers in
                                                    // class driver list
// Flag indicates that device is disabled
#define DI_DISABLED                 0x00000800L     // Set if device disabled

// Flags for Device/Class Properties
#define DI_GENERALPAGE_ADDED        0x00001000L
#define DI_RESOURCEPAGE_ADDED       0x00002000L

// Flag to indicate the setting properties for this Device (or class) caused a change
// so the Dev Mgr UI probably needs to be updatd.
#define DI_PROPERTIES_CHANGE        0x00004000L

// Flag to indicate that the sorting from the INF file should be used.
#define DI_INF_IS_SORTED            0x00008000L

#define DI_ENUMSINGLEINF            0x00010000L

// The following flags can be used to install a device disabled
// and to prevent CONFIGMG being called when a device is installed
#define DI_DONOTCALLCONFIGMG        0x00020000L
#define DI_INSTALLDISABLED          0x00040000L

// This flag is set of this LPDI is really just an LPCI, ie
// it only contains class info, NO DRIVER/DEVICE INFO
#define DI_CLASSONLY                0x00080000L

// This flag is set if the Class Install params are valid
#define DI_CLASSINSTALLPARAMS       0x00100000L

// This flag is set if the caller of DiCallClassInstaller does NOT
// want the internal default action performed if the Class installer
// return ERR_DI_DO_DEFAULT
#define DI_NODI_DEFAULTACTION       0x00200000L

// BUGBUG. This is a hack for M6 Net setup.  Net Setup does not work correctly
// if we process devnode syncronously.  This WILL be removed for M7 when
// Net setup is fixed to work with DiInstallDevice
#define DI_NOSYNCPROCESSING         0x00400000L

// flags for device installation
#define DI_QUIETINSTALL             0x00800000L     // don't confuse the user with
                                                    // questions or excess info
#define DI_NOFILECOPY               0x01000000L     // No file Copy necessary
#define DI_FORCECOPY                0x02000000L     // Force files to be copied from install path
#define DI_DRIVERPAGE_ADDED         0x04000000L     // Prop providor added Driver page.
#define DI_USECI_SELECTSTRINGS      0x08000000L     // Use Class Installer Provided strings in the Select Device Dlg
#define DI_OVERRIDE_INFFLAGS        0x10000000L     // Override INF flags
#define DI_PROPS_NOCHANGEUSAGE      0x20000000L     // No Enable/Disable in General Props

#define DI_NOSELECTICONS	    0x40000000L     // No small icons in select device dialogs

#define DI_NOWRITE_IDS		    0x80000000L     // Don't write HW & Compat IDs on install


// Defines for class installer functions
#define DIF_SELECTDEVICE            0x0001
#define DIF_INSTALLDEVICE           0x0002
#define DIF_ASSIGNRESOURCES         0x0003
#define DIF_PROPERTIES              0x0004
#define DIF_REMOVE                  0x0005
#define DIF_FIRSTTIMESETUP          0x0006
#define DIF_FOUNDDEVICE             0x0007
#define DIF_SELECTCLASSDRIVERS      0x0008
#define DIF_VALIDATECLASSDRIVERS    0x0009
#define DIF_INSTALLCLASSDRIVERS     0x000A
#define DIF_CALCDISKSPACE           0x000B
#define DIF_DESTROYPRIVATEDATA      0x000C
#define DIF_VALIDATEDRIVER          0x000D
#define DIF_MOVEDEVICE              0x000E
#define DIF_DETECT                  0x000F
#define DIF_INSTALLWIZARD           0x0010
#define DIF_DESTROYWIZARDDATA       0x0011
#define DIF_PROPERTYCHANGE          0x0012

typedef UINT        DI_FUNCTION;    // Function type for device installer

// DIF_MOVEDEVICE parameter struct.
typedef struct _MOVEDEV_PARAMS
{
    UINT            cbSize;
    LPDEVICE_INFO   lpdiOldDev;     // References the Device Begin Moved
} MOVEDEV_PARAMS, FAR * LPMOVEDEV_PARAMS;
#define ASSERT_MOVEDEVPARAMS_STRUC(lpmdp) if (lpmdp->cbSize != sizeof(MOVEDEV_PARAMS)) return (ERR_DI_BAD_MOVEDEV_PARAMS)

// DIF_PROPCHANGE parameter struct.
typedef struct _PROPCHANGE_PARAMS
{
    UINT            cbSize;
    DWORD           dwStateChange;
    DWORD           dwFlags;
    DWORD           dwConfigID;
} PROPCHANGE_PARAMS, FAR * LPPROPCHANGE_PARAMS;
#define ASSERT_PROPCHANGEPARAMS_STRUC(lpmdp) if (lpmdp->cbSize != sizeof(PROPCHANGE_PARAMS)) return (ERR_DI_BAD_PROPCHANGE_PARAMS)

#define MAX_TITLE_LEN           30
#define MAX_INSTRUCTION_LEN     256
#define MAX_LABEL_LEN           30
// DIF_SELECTDEVICE parameter struct.
typedef struct _SELECTDEVICE_PARAMS
{
    UINT            cbSize;
    char            szTitle[MAX_TITLE_LEN];
    char            szInstructions[MAX_INSTRUCTION_LEN];
    char            szListLabel[MAX_LABEL_LEN];
} SELECTDEVICE_PARAMS, FAR * LPSELECTDEVICE_PARAMS;
#define ASSERT_SELECTDEVICEPARAMS_STRUC(p) if (p->cbSize != sizeof(SELECTDEVICE_PARAMS)) return (ERR_DI_BAD_SELECTDEVICE_PARAMS)

#define DI_REMOVEDEVICE_GLOBAL                  0x00000001
#define DI_REMOVEDEVICE_CONFIGSPECIFIC          0x00000002
typedef struct _REMOVEDEVICE_PARAMS
{
    UINT            cbSize;
    DWORD           dwFlags;
    DWORD           dwConfigID;
} REMOVEDEVICE_PARAMS, FAR * LPREMOVEDEVICE_PARAMS;
#define ASSERT_REMOVEDPARAMS_STRUC(p) if (p->cbSize != sizeof(REMOVEDEVICE_PARAMS)) return (ERR_DI_BAD_REMOVEDEVICE_PARAMS)

// DIF_INSTALLWIZARD  Wizard Data
#define MAX_INSTALLWIZARD_DYNAPAGES             20

// Use this ID for the first page that the install wizard should dynamically jump to.
#define IDD_DYNAWIZ_FIRSTPAGE                   10000

// Use this ID for the page that the Select Device dialog should go back to
#define IDD_DYNAWIZ_SELECT_PREVPAGE             10001

// Use this ID for the page that the Select Device dialog should go to next
#define IDD_DYNAWIZ_SELECT_NEXTPAGE             10002
               
// Use this ID for the page that the Analyze dialog should go back to
// This will only be used in the event that there is a problem, and the user
// selects Back from the analyze proc.
#define IDD_DYNAWIZ_ANALYZE_PREVPAGE            10003

// Use this ID for the page that the Analyze dialog should go to if it continue from
// the analyze proc.  the wAnalyzeResult in the INSTALLDATA struct will
// contain the anaysis results.
#define IDD_DYNAWIZ_ANALYZE_NEXTPAGE            10004

// This dialog will be selected if the user chooses back from the
// Install Detected Devices dialog.
#define IDD_DYNAWIZ_INSTALLDETECTED_PREVPAGE    10006

// This dialog will be selected if the user chooses Next from the
// Install Detected Devices dialog.
#define IDD_DYNAWIZ_INSTALLDETECTED_NEXTPAGE    10007

// This is the ID of the dialog to select if detection does not
// find any new devices
#define IDD_DYNAWIZ_INSTALLDETECTED_NODEVS      10008
   
// This is the ID of the Select Device Wizard page.
#define IDD_DYNAWIZ_SELECTDEV_PAGE              10009

// This is the ID of the Analyze Device Wizard page.
#define IDD_DYNAWIZ_ANALYZEDEV_PAGE             10010

// This is the ID of the Install Detected Devs Wizard page.
#define IDD_DYNAWIZ_INSTALLDETECTEDDEVS_PAGE    10011

// This is the ID of the Select Class Wizard page.
#define IDD_DYNAWIZ_SELECTCLASS_PAGE            10012

// This flag is set if a Class installer has added pages to the
// install wizard.
#define DYNAWIZ_FLAG_PAGESADDED             0x00000001

// The following flags will control the button states when displaying
// the InstallDetectedDevs dialog.  
#define DYNAWIZ_FLAG_INSTALLDET_NEXT        0x00000002
#define DYNAWIZ_FLAG_INSTALLDET_PREV        0x00000004

// Set this flag if you jump to the analyze page, and want it to
// handle conflicts for you.  NOTE.  You will not get control back 
// in the event of a conflict if you set this flag.
#define DYNAWIZ_FLAG_ANALYZE_HANDLECONFLICT 0x00000008

#define ANALYZE_FACTDEF_OK      1
#define ANALYZE_STDCFG_OK       2
#define ANALYZE_CONFLICT        3
#define ANALYZE_NORESOURCES     4
#define ANALYZE_ERROR           5
#define ANALYZE_PNP_DEV         6


typedef struct InstallWizardData_tag        
{
    UINT            cbSize;
    
    LPDEVICE_INFO   lpdiOriginal;
    LPDEVICE_INFO   lpdiSelected;
    DWORD           dwFlags;
    LPVOID          lpConfigData;
    WORD            wAnalyzeResult;
        
    // The following fields are used when a Class Installer Extends the Install Wizard
    HPROPSHEETPAGE  hpsDynamicPages[MAX_INSTALLWIZARD_DYNAPAGES];
    WORD            wNumDynaPages;
    DWORD           dwDynaWizFlags;
    DWORD           dwPrivateFlags;
    LPARAM          lpPrivateData;
    LPSTR           lpExtraRunDllParams;    
} INSTALLWIZDATA, * PINSTALLWIZDATA , FAR *LPINSTALLWIZDATA;

RETERR WINAPI DiCreateDeviceInfo(
    LPLPDEVICE_INFO lplpdi,     // Ptr to ptr to dev info
    LPCSTR      lpszDescription,    // If non-null then description string
    DWORD       hDevnode,       // BUGBUG -- MAKE A DEVNODE
    HKEY        hkey,       // Registry hkey for dev info
    LPCSTR      lpszRegsubkey,  // If non-null then reg subkey string
    LPCSTR      lpszClassName,  // If non-null then class name string
    HWND        hwndParent);    // If non-null then hwnd of parent

RETERR WINAPI DiGetClassDevs(
    LPLPDEVICE_INFO lplpdi,     // Ptr to ptr to dev info
    LPCSTR      lpszClassName,  // Must be name of class
    HWND        hwndParent,     // If non-null then hwnd of parent
    int         iFlags);        // Options

RETERR WINAPI DiGetClassDevsEx(
    LPLPDEVICE_INFO lplpdi,     // Ptr to ptr to dev info
    LPCSTR      lpszClassName,  // Must be name of class
    LPCSTR      lpszEnumerator, // Must be name of enumerator, or NULL
    HWND        hwndParent,     // If non-null then hwnd of parent
    int         iFlags);        // Options
    
#define DIGCF_DEFAULT           0x0001  // NOT IMPLEMENTED!
#define DIGCF_PRESENT           0x0002
#define DIGCF_ALLCLASSES        0x0004
#define DIGCF_PROFILE           0x0008

// API to return the Class name of an INF File
RETERR WINAPI DiGetINFClass(LPSTR lpszMWDPath, UINT InfType, LPSTR lpszClassName, DWORD dwcbClassName);

RETERR WINAPI PASCAL DiCreateDevRegKey(
    LPDEVICE_INFO   lpdi,
    LPHKEY      lphk,
    HINF        hinf,
    LPCSTR      lpszInfSection,
    int         iFlags);
    
RETERR WINAPI PASCAL DiDeleteDevRegKey(LPDEVICE_INFO lpdi, int  iFlags);


RETERR WINAPI PASCAL DiOpenDevRegKey(
    LPDEVICE_INFO   lpdi,
    LPHKEY      lphk,
    int         iFlags);

#define DIREG_DEV	0x0001		// Open/Create/Delete device key
#define DIREG_DRV	0x0002		// Open/Create/Delete driver key
#define DIREG_BOTH	0x0004		// Delete both driver and Device key

RETERR WINAPI DiReadRegLogConf
(
    LPDEVICE_INFO       lpdi,
    LPSTR               lpszConfigName,
    LPBYTE FAR          *lplpbLogConf,
    LPDWORD             lpdwSize
);

RETERR WINAPI DiReadRegConf
(
    LPDEVICE_INFO       lpdi,
    LPBYTE FAR          *lplpbLogConf,
    LPDWORD             lpdwSize,
    DWORD               dwFlags
);

#define DIREGLC_FORCEDCONFIG        0x00000001
#define DIREGLC_BOOTCONFIG          0x00000002

RETERR WINAPI DiCopyRegSubKeyValue
(
    HKEY    hkKey,
    LPSTR   lpszFromSubKey,
    LPSTR   lpszToSubKey,
    LPSTR   lpszValueToCopy
);

RETERR WINAPI DiDestroyClassInfoList(LPCLASS_INFO lpci);
RETERR WINAPI DiBuildClassInfoList(LPLPCLASS_INFO lplpci);

#define DIBCI_NOINSTALLCLASS        0x000000001
#define DIBCI_NODISPLAYCLASS        0x000000002

RETERR WINAPI DiBuildClassInfoListEx(LPLPCLASS_INFO lplpci, DWORD dwFlags);
RETERR WINAPI DiGetDeviceClassInfo(LPLPCLASS_INFO lplpci, LPDEVICE_INFO lpdi);

RETERR WINAPI DiDestroyDeviceInfoList(LPDEVICE_INFO lpdi);
RETERR WINAPI DiSelectDevice( LPDEVICE_INFO lpdi );
RETERR WINAPI DiSelectOEMDrv(HWND hDlg, LPDEVICE_INFO lpdi);

// Callback for diInstallDevice vcpOpen.  Basically calls vcpUICallback for everthing
// except when DI_FORCECOPY is active, in which case copies get defaulted to
// VCPN_FORCE
LRESULT CALLBACK diInstallDeviceUICallbackProc(LPVOID lpvObj, UINT uMsg, WPARAM wParam, LPARAM lParam, LPARAM lparamRef);
RETERR WINAPI DiInstallDevice( LPDEVICE_INFO lpdi );
RETERR WINAPI DiInstallDriverFiles(LPDEVICE_INFO lpdi);

RETERR WINAPI DiRemoveDevice( LPDEVICE_INFO lpdi );
RETERR WINAPI DiAssignResources( LPDEVICE_INFO lpdi );
RETERR WINAPI DiAskForOEMDisk(LPDEVICE_INFO lpdi);

RETERR WINAPI DiCallClassInstaller(DI_FUNCTION diFctn, LPDEVICE_INFO lpdi);

RETERR WINAPI DiBuildCompatDrvList(LPDEVICE_INFO lpdi);
RETERR WINAPI DiBuildClassDrvList(LPDEVICE_INFO lpdi);

typedef RETERR (CALLBACK *OLDINFPROC)(HINF hinf, LPCSTR lpszNewInf, LPARAM lParam);
RETERR WINAPI DiBuildClassDrvListFromOldInf(LPDEVICE_INFO lpdi, LPCSTR lpszSection, OLDINFPROC lpfnOldInfProc, LPARAM lParam);

RETERR WINAPI DiDestroyDriverNodeList(LPDRIVER_NODE lpdn);

RETERR  WINAPI  DiMoveDuplicateDevNode(LPDEVICE_INFO lpdiNewDev);

// The following export will load a dll and find the specified proc name
typedef RETERR (FAR PASCAL *DIINSTALLERPROPERTIES)(LPDEVICE_INFO);

RETERR WINAPI GetFctn(HKEY hk, LPSTR lpszRegVal, LPSTR lpszDefProcName,
                      HINSTANCE FAR * lphinst, FARPROC FAR *lplpfn);
                        
RETERR
WINAPI
DiCreateDriverNode(
    LPLPDRIVER_NODE lplpdn,
    UINT    Rank,
    UINT    InfType,
    unsigned    InfDate,
    LPCSTR  lpszDevDescription,
    LPCSTR  lpszDrvDescription,
    LPCSTR  lpszProviderName,
    LPCSTR  lpszMfgName,
    LPCSTR  lpszInfFileName,
    LPCSTR  lpszSectionName,
    DWORD   dwPrivateData);


RETERR WINAPI DiLoadClassIcon(
    LPCSTR  szClassName,
    HICON FAR *lphiLargeIcon,
    int FAR *lpiMiniIconIndex);


RETERR WINAPI DiInstallDrvSection(
    LPCSTR  lpszInfFileName,
    LPCSTR  lpszSection,
    LPCSTR  lpszClassName,
    LPCSTR  lpszDescription,
    DWORD   dwFlags);


RETERR WINAPI DiChangeState(LPDEVICE_INFO lpdi, DWORD dwStateChange, DWORD dwFlags, LPARAM lParam);

#define DICS_ENABLE                 0x00000001
#define DICS_DISABLE                0x00000002
#define DICS_PROPCHANGE             0x00000003

#define DICS_FLAG_GLOBAL            0x00000001
#define DICS_FLAG_CONFIGSPECIFIC    0x00000002

RETERR WINAPI DiInstallClass(LPCSTR lpszInfFileName, DWORD dwFlags);

RETERR WINAPI DiOpenClassRegKey(LPHKEY lphk, LPCSTR lpszClassName);

// support routine for dealing with class mini icons
int WINAPI PASCAL DiDrawMiniIcon(HDC hdc, RECT rcLine, int iMiniIcon, UINT flags);
BOOL WINAPI DiGetClassBitmapIndex(LPCSTR lpszClass, int FAR *lpiMiniIconIndex);

// internal calls for display class
#define DISPLAY_SETMODE_SUCCESS     0x0001
#define DISPLAY_SETMODE_DRVCHANGE   0x0002
#define DISPLAY_SETMODE_FONTCHANGE  0x0004

UINT WINAPI Display_SetMode(LPDEVICE_INFO lpdi, UINT uColorRes, int iXRes, int iYRes);
RETERR WINAPI Display_ClassInstaller(DI_FUNCTION diFctn, LPDEVICE_INFO lpdi);
RETERR WINAPI Display_OpenFontSizeKey(LPHKEY lphkFontSize);
BOOL WINAPI Display_SetFontSize(LPCSTR lpszFontSize);

RETERR WINAPI DiIsThereNeedToCopy(HWND hwnd, DWORD dwFlags);

#define DINTC_NOCOPYDEFAULT	    0x00000001

// API for the mouse class installer
RETERR WINAPI Mouse_ClassInstaller(DI_FUNCTION diFctn, LPDEVICE_INFO lpdi);
#endif // NODEVICEINSTALL

// API for determining if a Driver file is currently part of VMM32.VxD
BOOL WINAPI bIsFileInVMM32
(
    LPSTR   lpszFileName
);


/***************************************************************************/
//
// setup reg DB calls, use just like those in kernel
//
/***************************************************************************/

DWORD WINAPI SURegOpenKey(HKEY hKey, LPSTR lpszSubKey, HKEY FAR *lphkResult);
DWORD WINAPI SURegCloseKey(HKEY hKey);
DWORD WINAPI SURegCreateKey(HKEY hKey, LPSTR lpszSubKey, HKEY FAR *lphkResult);
DWORD WINAPI SURegDeleteKey(HKEY hKey, LPSTR lpszSubKey);
DWORD WINAPI SURegEnumKey(HKEY hKey, DWORD dwIdx, LPSTR lpszBuffer, DWORD dwBufSize);
DWORD WINAPI SURegQueryValue16(HKEY hKey, LPSTR lpszSubKey, LPSTR lpValueBuf, DWORD FAR *ldwBufSize);
DWORD WINAPI SURegSetValue16(HKEY hKey, LPSTR lpszSubKey, DWORD dwType, LPBYTE lpszValue, DWORD dwValSize);
DWORD WINAPI SURegDeleteValue(HKEY hKey,LPSTR lpszValue);
DWORD WINAPI SURegEnumValue(HKEY hKey,DWORD dwIdx, LPSTR lpszValue, DWORD FAR *lpcbValue, DWORD FAR *lpdwReserved, DWORD FAR *lpdwType, LPBYTE lpbData, DWORD FAR *lpcbData);
DWORD WINAPI SURegQueryValueEx(HKEY hKey,LPSTR lpszValueName, DWORD FAR *lpdwReserved,DWORD FAR *lpdwType,LPSTR lpValueBuf, DWORD FAR *ldwBufSize);
DWORD WINAPI SURegSetValueEx(HKEY hKey,LPSTR lpszValueName, DWORD dwReserved, DWORD dwType, LPBYTE lpszValue, DWORD dwValSize);
DWORD WINAPI SURegSaveKey(HKEY hKey, LPCSTR lpszFileName, LPVOID lpsa);
DWORD WINAPI SURegLoadKey(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszFileName);
DWORD WINAPI SURegUnLoadKey(HKEY hKey, LPCSTR lpszSubKey);

DWORD WINAPI SURegFlush(VOID);
DWORD WINAPI SURegInit(VOID);    // should be called before any other Reg APIs


/***************************************************************************/
// setup FormatMessage support
/***************************************************************************/
#define MB_LOG  (UINT)-1

UINT FAR CDECL suFormatMessage(HINSTANCE hAppInst, LPCSTR lpcFormat, LPSTR szMessage, UINT uSize,
    ...);
UINT WINAPI suvFormatMessage(HINSTANCE hAppInst, LPCSTR lpcFormat, LPSTR szMessage, UINT uSize,
    LPVOID FAR * lpArgs);
int WINCAPI _loadds suFormatMessageBox(HINSTANCE hAppInst, HWND hwndParent, LPCSTR lpcText, LPCSTR lpcTitle,
    UINT uStyle, ...);

WORD WINAPI suErrorToIds( WORD Value, WORD Class );

/***************************************************************************/
// setup Version Conflict support
/***************************************************************************/

LPVOID WINAPI suVerConflictInit(BOOL fYesToLangMismatch);
void WINAPI suVerConflictTerm(LPVOID lpvData);
LRESULT WINAPI suVerConflict(HWND hwnd, LPVCPVERCONFLICT lpvc, LPVOID lpvData);


/***************************************************************************/
// setup Help support
/***************************************************************************/

BOOL WINAPI suHelp( HWND hwndApp, HWND hwndDlg );

//***************************************************************************/
// setup Emergency Boot Disk (EBD) creation fn.
//***************************************************************************/

RETERR WINAPI suCreateEBD( HWND hWnd, VIFPROC CopyCallbackProc, LPARAM lpuii );

//***************************************************************************
// Misc SETUPX.DLL support functions.
//***************************************************************************


RETERR WINAPI SUGetSetSetupFlags(LPDWORD lpdwFlags, BOOL bSet);

RETERR WINAPI CfgSetupMerge( int uFlags );


#ifndef LPLPSTR
    typedef LPSTR (FAR *LPLPSTR);
#endif





//***************************************************************************
//
// ENUMS for accessing config.sys/autoexec.bat line objects using the
// array returned by ParseConfigLine()..
//
//***************************************************************************

enum    CFGLINE_STRINGS                     // Config.sys/autoexec.bat objects
{
    CFG_KEYLEAD,                            // Keyword leading whitespaces
    CFG_KEYWORD,                            // Keyword
    CFG_KEYTRAIL,                           // Keyword trailing delimiters
    CFG_UMBINFO,                            // Load high info
    CFG_DRVLETTER,                          // Drive letter for cmd path
    CFG_PATH,                               // Command path
    CFG_COMMAND,                            // Command base name
    CFG_EXT,                                // Command extension including '.'
    CFG_ARGS,                               // Command arguments
    CFG_FREE,                               // Free area at end of buffer
    CFG_END
};

// DJM  This will be included soon
/*---------------------------------------------------------------------------*
 *                  SUB String Data 
 *---------------------------------------------------------------------------*/

typedef struct _SUBSTR_DATA {
    LPSTR lpFirstSubstr;
    LPSTR lpCurSubstr;
    LPSTR lpLastSubstr;
}   SUBSTR_DATA;


typedef SUBSTR_DATA*		PSUBSTR_DATA;
typedef SUBSTR_DATA NEAR*	NPSUBSTR_DATA;
typedef SUBSTR_DATA FAR*	LPSUBSTR_DATA;

BOOL WINAPI InitSubstrData(LPSUBSTR_DATA lpSubstrData, LPSTR lpStr);
BOOL WINAPI GetFirstSubstr(LPSUBSTR_DATA lpSubstrData);
BOOL WINAPI GetNextSubstr(LPSUBSTR_DATA lpSubStrData);

/*---------------------------------------------------------------------------*
 *                  Misc. Di functions
 *---------------------------------------------------------------------------*/
BOOL WINAPI DiBuildPotentialDuplicatesList
(
    LPDEVICE_INFO   lpdi, 
    LPSTR           lpDuplicateList, 
    DWORD           cbSize,
    LPDWORD         lpcbData,
    LPSTR           lpstrDupType
);

//***************************************************************************
#endif      // SETUPX_INC
