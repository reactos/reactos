/* command.h - This file has all the defines and data structures nneded
 *	       for command.com/command.dll communication.
 *
 * Sudeepb 17-Sep-1991 Created
 */

/* ASM
include bop.inc

CMDSVC	macro	func
	BOP	BOP_CMD
	db	func
	endm
*/

#define SVC_CMDEXITVDM		0
#define SVC_CMDGETNEXTCMD	1
#define	SVC_CMDCOMSPEC		2
#define SVC_CMDSAVEWORLD	3
#define SVC_CMDGETCURDIR	4
#define SVC_CMDSETINFO          5
#define SVC_GETSTDHANDLE        6
#define SVC_CMDCHECKBINARY      7
#define SVC_CMDEXEC             8
#define SVC_CMDINITCONSOLE      9
#define SVC_EXECCOMSPEC32       10
#define SVC_RETURNEXITCODE      11
#define SVC_GETCONFIGSYS        12
#define SVC_GETAUTOEXECBAT      13
#define SVC_GETKBDLAYOUT        14
#define SVC_GETINITENVIRONMENT  15
#define SVC_GETSTARTINFO        16
#define SVC_SETWINTITLE         17
#define SVC_GETCURSORPOS        30
#define SVC_CMDLASTSVC          31

#define ALL_HANDLES	7
#define HANDLE_STDIN	0
#define HANDLE_STDOUT	1
#define HANDLE_STDERR	2
#define MASK_STDIN	1
#define MASK_STDOUT	2
#define MASK_STDERR     4


// define extention type for command.com.
// these value must be in sync with 16bits. Please refer to
// tmisc1.asm in mvdm\dos\v86\cmd\command
#define BAD_EXTENTION	    0	    // never returned from 32 bits
#define BAT_EXTENTION	    2
#define EXE_EXTENTION	    4
#define COM_EXTENTION	    8
#define MAX_STD_EXTENTION   COM_EXTENTION

// THIS VALUE IS VERY IMPORTANT.
// 0(zero) means there is no such a program file and command.com would spit out
// well-known "bad command or file name".
// Give it to any value larger than than MAX_STD_EXTENTION) makes command.com
// by-passing any other checking and passing down the program file as is
// to DOS which doesn't have tight executable file extention convention.
// The reason that we allow program file with "non-standard" extention to
// pass through command.com and reach DOS is we receive the program pathname
// from CreateProcess which doesn't limit any file extention.
//


#define UNKNOWN_EXTENTION   MAX_STD_EXTENTION + 1


/* XLATOFF */

typedef struct _SAVEWORLD {
    USHORT  ax;
    USHORT  bx;
    USHORT  cx;
    USHORT  dx;
    USHORT  cs;
    USHORT  ss;
    USHORT  ds;
    USHORT  es;
    USHORT  si;
    USHORT  di;
    USHORT  bp;
    USHORT  sp;
    USHORT  ip;
    USHORT  flag;
    ULONG   ImageSize;
} SAVEWORLD, *PSAVEWORLD;

extern BOOL CMDInit (int argc, char *argv[]);
extern BOOL CMDRebootVDM (void);

extern BOOL fEnableInt10;

/* XLATON */

/** CMDINFO - Communication record of command.com **/

/* XLATOFF */
#pragma pack(2)
/* XLATON */

typedef struct _CMDINFO {		/**/
    USHORT	EnvSeg;
    USHORT	EnvSize;
    USHORT	CurDrive;
    USHORT	NumDrives;
    USHORT	CmdLineSeg;
    USHORT	CmdLineOff;
    USHORT	CmdLineSize;
    USHORT	ReturnCode;
    USHORT      bStdHandles;
    ULONG       pRdrInfo;
    USHORT	CodePage;
    USHORT      fTSRExit;
    USHORT      fBatStatus;
    USHORT      ExecPathSeg;
    USHORT      ExecPathOff;
    USHORT      ExecPathSize;
    USHORT	ExecExtType;
} CMDINFO;
typedef CMDINFO UNALIGNED *PCMDINFO;

/* XLATOFF */
#pragma pack()
/* XLATON */


BOOL CmdDispatch (ULONG);
