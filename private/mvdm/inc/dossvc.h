/** SVC Defines
 *
 *  Revision history:
 *
 *  sudeepb 27-Feb-1991 Created
 */

/* VHE - Virtual Hard Error packet.
 *
 *   DEM makes fbInt24 to true if an hard error happens.
 */

typedef struct vhe_s {
    char  vhe_fbInt24;      // Was there a hard error?
    char  vhe_HrdErrCode;   // If hard error then this is the error code
    char  vhe_bDriveNum;    // If so on which drive
} VHE;

typedef VHE *PVHE;

/* DEMEXTERR - Extended Error structure. The following structure contains the
 * DOS extended error elements which are loosely coupled in the DOS data
 * segment
 *
 */

/* XLATOFF */
#include <packon.h>

typedef struct _DEMEXTERR {
    UCHAR   ExtendedErrorLocus;
    USHORT  ExtendedError;
    UCHAR   ExtendedErrorAction;
    UCHAR   ExtendedErrorClass;
    PUCHAR  ExtendedErrorPointer;
} DEMEXTERR;

typedef DEMEXTERR* PDEMEXTERR;


/* SYSDEV - Device chain node
 */

typedef struct _SYSDEV {

    ULONG   sdevNext;       // REAL mode pointer to next device.  -1 for end of chain.
    char    sdevIgnore[6];
    UCHAR   sdevDevName[8]; // device name

} SYSDEV;

typedef SYSDEV UNALIGNED *PSYSDEV;

#ifndef _DEMINCLUDED_
extern DECLSPEC_IMPORT PSYSDEV pDeviceChain;
#else
extern PSYSDEV pDeviceChain;
#endif


/* XLATON */

/* XLATOFF */
#include <packoff.h>
/* XLATON */

/* Note : To add a new SVC:
 *      New SVC gets the current value of SVC_LASTSVC. Increment
 *      the SVC_LASTSVC value. Add the appropriate SVC handler
 *      in apfnSVC (file dem\demdisp.c) at the end.
 *    To delete a SVC :
 *      Move each SVC one level up. Appropriatly adjust the
 *      apfnSVC (file dem\demdisp.c).
 */

/* SVC - Supervisory Call macro.
 *
 *   This macro is used by NTDOS and NTBIO to call DEM.
 *
 */

#define NTVDMDBG 1

/* ASM
include bop.inc

svc macro   func
    BOP BOP_DOS
    db  func
    endm
*/
#define SVC_DEMCHGFILEPTR               0x00
#define SVC_DEMCHMOD            0x01
#define SVC_DEMCLOSE            0x02
#define SVC_DEMCREATE           0x03
#define SVC_DEMCREATEDIR        0x04
#define SVC_DEMDELETE           0x05
#define SVC_DEMDELETEDIR        0x06
#define SVC_DEMDELETEFCB        0x07
#define SVC_DEMFILETIMES        0x08
#define SVC_DEMFINDFIRST        0x09
#define SVC_DEMFINDFIRSTFCB     0x0a
#define SVC_DEMFINDNEXT         0x0b
#define SVC_DEMFINDNEXTFCB      0x0c
#define SVC_DEMGETBOOTDRIVE     0x0d
#define SVC_DEMGETDRIVEFREESPACE    0x0e
#define SVC_DEMGETDRIVES        0x0f
#define SVC_DEMGSETMEDIAID      0x10
#define SVC_DEMLOADDOS          0x11
#define SVC_DEMOPEN         0x12
#define SVC_DEMQUERYCURRENTDIR      0x13
#define SVC_DEMQUERYDATE        0x14
#define SVC_DEMQUERYTIME        0x15
#define SVC_DEMREAD         0x16
#define SVC_DEMRENAME           0x17
#define SVC_DEMSETCURRENTDIR        0x18
#define SVC_DEMSETDATE          0x19
#define SVC_DEMSETDEFAULTDRIVE      0x1a
#define SVC_DEMSETDTALOCATION       0x1b
#define SVC_DEMSETTIME          0x1c
#define SVC_DEMSETV86KERNELADDR     0x1d
#define SVC_DEMWRITE            0x1e
#define SVC_GETDRIVEINFO        0x1f
#define SVC_DEMRENAMEFCB        0x20
#define SVC_DEMIOCTL            0x21
#define SVC_DEMCREATENEW        0x22
#define SVC_DEMDISKRESET        0x23
#define SVC_DEMSETDPB           0x24
#define SVC_DEMGETDPB           0x25
#define SVC_DEMSLEAZEFUNC       0x26
#define SVC_DEMCOMMIT           0x27
#define SVC_DEMEXTHANDLE        0x28
#define SVC_DEMABSDRD           0x29
#define SVC_DEMABSDWRT          0x2a
#define SVC_DEMGSETCDPG         0x2b
#define SVC_DEMCREATEFCB        0x2c
#define SVC_DEMOPENFCB          0x2d
#define SVC_DEMCLOSEFCB         0x2e
#define SVC_DEMFCBIO            0x2f
#define SVC_DEMDATE16           0x30
#define SVC_DEMGETFILEINFO      0x31
#define SVC_DEMSETHARDERRORINFO     0x32
#define SVC_DEMRETRY            0x33
#define SVC_DEMLOADDOSAPPSYM        0x34
#define SVC_DEMFREEDOSAPPSYM        0x35
#define SVC_DEMENTRYDOSAPP              0x36
#define SVC_DEMDOSDISPCALL              0x37
#define SVC_DEMDOSDISPRET               0x38
#define SVC_OUTPUT_STRING               0x39
#define SVC_INPUT_STRING        0x3A
#define SVC_ISDEBUG         0x3B
#define SVC_PDBTERMINATE        0x3C
#define SVC_DEMEXITVDM          0x3D
#define SVC_DEMWOWFILES         0x3E
#define SVC_DEMLOCKOPER         0x3F
#define SVC_DEMDRIVEFROMHANDLE  0x40
#define SVC_DEMGETCOMPUTERNAME  0x41
#define SVC_DEMFASTREAD         0x42
#define SVC_DEMFASTWRITE        0x43
#define SVC_DEMCHECKPATH        0x44
#define SVC_DEMSYSTEMSYMBOLOP   0x45
#define SVC_DEMGETDPBLIST       0x46

#define SVC_DEMPIPEFILEDATAEOF  0x47
#define SVC_DEMPIPEFILEEOF      0x48
#define SVC_DEMLFNENTRY         0x49
#define SVC_SETDOSVARLOCATION   0x4A
#define SVC_DEMLASTSVC          0x4B


/*
 *   Equates used in the DEMxxxSYSTEMxxx calls
 */
#define SYMOP_LOAD 1
#define SYMOP_FREE 2
#define SYMOP_MOVE 3
#define SYMOP_CLEANUP 0x80

#define ID_NTIO 1
#define ID_NTDOS 2
