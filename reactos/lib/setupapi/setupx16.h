/*
 * Copyright 2000 Andreas Mohr for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __SETUPX16_H
#define __SETUPX16_H

#include "wine/windef16.h"

typedef UINT16 HINF16;
typedef UINT16 LOGDISKID16;
typedef UINT16 VHSTR;

#define LINE_LEN	256

/* error codes stuff */

typedef UINT16 RETERR16;
#define OK		0
#define IP_ERROR	(UINT16)100
#define TP_ERROR	(UINT16)200
#define VCP_ERROR	(UINT16)300
#define GEN_ERROR	(UINT16)400
#define DI_ERROR	(UINT16)500

enum {
	ERR_IP_INVALID_FILENAME = IP_ERROR+1,
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

/****** virtual copy operations ******/

typedef DWORD LPEXPANDVTBL;

typedef struct {
	DWORD		dwSoFar;
	DWORD		dwTotal;
} VCPPROGRESS, *LPVCPPROGRESS;

typedef struct {
	WORD		cbSize;
	LOGDISKID16	ldid;
	VHSTR		vhstrRoot;
	VHSTR		vhstrVolumeLabel;
	VHSTR		vhstrDiskName;
	WORD		wVolumeTime;
	WORD		wVolumeDate;
	DWORD		dwSerialNumber;
	WORD		fl;
	LPARAM		lparamRef;

	VCPPROGRESS	prgFileRead;
	VCPPROGRESS	prgByteRead;

	VCPPROGRESS	prgFileWrite;
	VCPPROGRESS	prgByteWrite;
} VCPDISKINFO, *LPVCPDISKINFO;

typedef struct {
	LOGDISKID16	ldid;
	VHSTR		vhstrDir;
	VHSTR		vhstrFileName;
} VCPFILESPEC, *LPVCPFILESPEC;

typedef struct {
	UINT16		uiMDate;
	UINT16		uiMTime;
	UINT16		uiADate;
	UINT16		uiATime;
	UINT16		uiAttr;
	DWORD		llenIn;
	DWORD		llenOut;
} VCPFATTR, *LPVCPFATTR;

typedef struct {
	UINT16		uDate;
	UINT16		uTime;
	DWORD		dwSize;
} VCPFILESTAT, *LPVCPFILESTAT;

typedef struct
{
	HFILE16		hFileSrc;
	HFILE16		hFileDst;
	VCPFATTR	fAttr;
	WORD		dosError;
	VHSTR		vhstrFileName;
	WPARAM		vcpm;
} VIRTNODEEX, *LPVIRTNODEEX;

typedef struct {
	WORD		cbSize;
	VCPFILESPEC	vfsSrc;
	VCPFILESPEC	vfsDst;
	WORD		fl;
	LPARAM		lParam;
	LPEXPANDVTBL	lpExpandVtbl;
	LPVIRTNODEEX	lpvnex;
	VHSTR		vhstrDstFinalName;
	VCPFILESTAT	vFileStat;
} VIRTNODE, *LPVIRTNODE;

typedef struct {
	WORD		cbSize;
	VCPPROGRESS	prgDiskRead;
	VCPPROGRESS	prgFileRead;
	VCPPROGRESS	prgByteRead;

	VCPPROGRESS	prgDiskWrite;
	VCPPROGRESS	prgFileWrite;
	VCPPROGRESS	prgByteWrite;

	LPVCPDISKINFO	lpvdiIn;
	LPVCPDISKINFO	lpvdiOut;
	LPVIRTNODE	lpvn;
} VCPSTATUS, *LPVCPSTATUS;

#define CNFL_BACKUP		0x0001
#define CNFL_DELETEONFAILURE	0x0002
#define CNFL_RENAMEONSUCCESS	0x0004
#define CNFL_CONTINUATION	0x0008
#define CNFL_SKIPPED		0x0010
#define CNFL_IGNOREERRORS	0x0020
#define CNFL_RETRYFILE		0x0040
#define CNFL_COPIED		0x0080
#define VNFL_UNIQUE		0x0000
#define VNFL_MULTIPLEOK		0x0100
#define VNFL_DESTROYOLD		0x0200
#define VNFL_COPY		0x0000
#define VNFL_DELETE		0x0800
#define VNFL_RENAME		0x1000
#define VNFL_NODE_TYPE		(VNFL_RENAME|VNFL_DELETE|VNFL_COPY)
#define VNFL_CREATED		0x2000
#define VNFL_REJECTED		0x4000
#define VNFL_DEVICEINSTALLER	0x8000

enum {
	ERR_VCP_IOFAIL = VCP_ERROR+1,
	ERR_VCP_STRINGTOOLONG,
	ERR_VCP_NOMEM,
	ERR_VCP_QUEUEFULL,
	ERR_VCP_NOVHSTR,
	ERR_VCP_OVERFLOW,
	ERR_VCP_BADARG,
	ERR_VCP_UNINIT,
	ERR_VCP_NOTFOUND,
	ERR_VCP_BUSY,
	ERR_VCP_INTERRUPTED,
	ERR_VCP_BADDEST,
	ERR_VCP_SKIPPED,
	ERR_VCP_IO,
	ERR_VCP_LOCKED,
	ERR_VCP_WRONGDISK,
	ERR_VCP_CHANGEMODE,
	ERR_VCP_LDDINVALID,
	ERR_VCP_LDDFIND,
	ERR_VCP_LDDUNINIT,
	ERR_VCP_LDDPATH_INVALID,
	ERR_VCP_NOEXPANSION,
	ERR_VCP_NOTOPEN,
	ERR_VCP_NO_DIGITAL_SIGNATURE_CATALOG,
	ERR_VCP_NO_DIGITAL_SIGNATURE_FILE
};

#define VCPN_OK		0
#define VCPN_PROCEED	0
#define VCPN_ABORT	-1
#define VCPN_RETRY	-2
#define VCPN_IGNORE	-3
#define VCPN_SKIP	-4
#define VCPN_FORCE	-5
#define VCPN_DEFER	-6
#define VCPN_FAIL	-7
#define VCPN_RETRYFILE	-8

#define VCPFL_ABANDON		0x00
#define VCPFL_BACKUP		0x01
#define VCPFL_COPY		0x02
#define VCPFL_BACKUPANDCOPY	(VCPFL_BACKUP|VCPFL_COPY)
#define VCPFL_INSPECIFIEDORDER	0x04
#define VCPFL_DELETE		0x08
#define VCPFL_RENAME		0x10
#define VCPFL_ALL		(VCPFL_COPY|VCPFL_DELETE|VCPFL_RENAME)

#define CFNL_BACKUP		0x0001
#define CFNL_DELETEONFAILURE	0x0002
#define CFNL_RENAMEONSUCCESS	0x0004
#define CFNL_CONTINUATION	0x0008
#define CFNL_SKIPPED		0x0010
#define CFNL_IGNOREERRORS	0x0020
#define CFNL_RETRYFILE		0x0040
#define CFNL_COPIED		0x0080
#define VFNL_MULTIPLEOK		0x0100
#define VFNL_DESTROYOLD		0x0200
#define VFNL_NOW		0x0400
#define VFNL_COPY		0x0000
#define VFNL_DELETE		0x0800
#define VFNL_RENAME		0x1000
#define VFNL_CREATED		0x2000
#define VFNL_REJECTED		0x4000
#define VCPM_DISKCLASS		0x01
#define VCPM_DISKFIRST		0x0100
#define VCPM_DISKLAST		0x01ff

enum {
	VCPM_DISKCREATEINFO = VCPM_DISKFIRST,
	VCPM_DISKGETINFO,
	VCPM_DISKDESTROYINFO,
	VCPM_DISKPREPINFO,
	VCPM_DISKENSURE,
	VCPM_DISKPROMPT,
	VCPM_DISKFORMATBEGIN,
	VCPM_DISKFORMATTING,
	VCPM_DISKFORMATEND
};

#define VCPM_FILEINCLASS	0x02
#define VCPM_FILEOUTCLASS	0x03
#define VCPM_FILEFIRSTIN	0x0200
#define VCPM_FILEFIRSTOUT	0x0300
#define VCPM_FILELAST		0x03ff

enum {
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
	VCPM_FILERENAME
};

#define VCPM_NODECLASS		0x04
#define VCPM_NODEFIRST		0x0400
#define VCPM_NODELAST		0x04ff

enum {
	VCPM_NODECREATE = VCPM_NODEFIRST,
	VCPM_NODEACCEPT,
	VCPM_NODEREJECT,
	VCPM_NODEDESTROY,
	VCPM_NODECHANGEDESTDIR,
	VCPM_NODECOMPARE
};

#define VCPM_TALLYCLASS		0x05
#define VCPM_TALLYFIRST		0x0500
#define VCPM_TALLYLAST		0x05ff

enum {
	VCPM_TALLYSTART = VCPM_TALLYFIRST,
	VCPM_TALLYEND,
	VCPM_TALLYFILE,
	VCPM_TALLYDISK
};

#define VCPM_VERCLASS		0x06
#define VCPM_VERFIRST		0x0600
#define VCPM_VERLAST		0x06ff

enum {
	VCPM_VERCHECK = VCPM_VERFIRST,
	VCPM_VERCHECKDONE,
	VCPM_VERRESOLVECONFLICT
};

#define VCPM_VSTATCLASS		0x07
#define VCPM_VSTATFIRST		0x0700
#define VCPM_VSTATLAST		0x07ff

enum {
	VCPM_VSTATSTART = VCPM_VSTATFIRST,
	VCPM_VSTATEND,
	VCPM_VSTATREAD,
	VCPM_VSTATWRITE,
	VCPM_VSTATNEWDISK,
	VCPM_VSTATCLOSESTART,
	VCPM_VSTATCLOSEEND,
	VCPM_VSTATBACKUPSTART,
	VCPM_VSTATBACKUPEND,
	VCPM_VSTATRENAMESTART,
	VCPM_VSTATRENAMEEND,
	VCPM_VSTATCOPYSTART,
	VCPM_VSTATCOPYEND,
	VCPM_VSTATDELETESTART,
	VCPM_VSTATDELETEEND,
	VCPM_VSTATPATHCHECKSTART,
	VCPM_VSTATPATHCHECKEND,
	VCPM_VSTATCERTIFYSTART,
	VCPM_VSTATCERTIFYEND,
	VCPM_VSTATUSERABORT,
	VCPM_VSTATYIELD
};

#define VCPM_PATHCLASS		0x08
#define VCPM_PATHFIRST		0x0800
#define VCPM_PATHLAST		0x08ff

enum {
	VCPM_BUILDPATH = VCPM_PATHFIRST,
	VCPM_UNIQUEPATH,
	VCPM_CHECKPATH
};

#define VCPM_PATCHCLASS		0x09
#define VCPM_PATCHFIRST		0x0900
#define VCPM_PATCHLAST		0x09ff

enum {
	VCPM_FILEPATCHBEFORECPY = VCPM_PATCHFIRST,
	VCPM_FILEPATCHAFTERCPY,
	VCPM_FILEPATCHINFOPEN,
	VCPM_FILEPATCHINFCLOSE
};

#define VCPM_CERTCLASS		0x0a
#define VCPM_CERTFIRST		0x0a00
#define VCPM_CERTLAST		0x0aff

enum {
	VCPM_FILECERTIFY = VCPM_CERTFIRST,
	VCPM_FILECERTIFYWARN
};

typedef LRESULT (CALLBACK *VIFPROC)(LPVOID lpvObj, UINT16 uMsg, WPARAM wParam, LPARAM lParam, LPARAM lparamRef);

typedef int (CALLBACK *VCPENUMPROC)(LPVIRTNODE lpvn, LPARAM lparamRef);

RETERR16 WINAPI VcpOpen16(VIFPROC vifproc, LPARAM lparamMsgRef);

/* VcpQueueCopy flags */
#define VNLP_SYSCRITICAL	0x0001
#define VNLP_SETUPCRITICAL	0x0002
#define VNLP_NOVERCHECK		0x0004
#define VNLP_FORCETEMP		0x0008
#define VNLP_IFEXISTS		0x0010
#define VNLP_KEEPNEWER		0x0020
#define VNLP_PATCHIFEXIST	0x0040
#define VNLP_NOPATCH		0x0080
#define VNLP_CATALOGCERT	0x0100
#define VNLP_NEEDCERTIFY	0x0200
#define VNLP_COPYIFEXISTS	0x0400

RETERR16 WINAPI VcpQueueCopy16(
	LPCSTR lpszSrcFileName, LPCSTR lpszDstFileName,
	LPCSTR lpszSrcDir, LPCSTR lpszDstDir,
	LOGDISKID16 ldidSrc, LOGDISKID16 ldidDst,
	LPEXPANDVTBL lpExpandVtbl,
	WORD fl, LPARAM lParam
);
RETERR16 VcpFlush16(WORD fl, LPCSTR lpszBackupDest);
RETERR16 WINAPI VcpClose16(WORD fl, LPCSTR lpszBackupDest);

/* VcpExplain flags */
enum {
	VCPEX_SRC_DISK,
	VCPEX_SRC_CABINET,
	VCPEX_SRC_LOCN,
	VCPEX_DST_LOCN,
	VCPEX_SRC_FILE,
	VCPEX_DST_FILE,
	VCPEX_DST_FILE_FINAL,
	VCPEX_DOS_ERROR,
	VCPEX_MESSAGE,
	VCPEX_DOS_SOLUTION,
	VCPEX_SRC_FULL,
	VCPEX_DST_FULL,
	VCPEX_DST_FULL_FINAL
};

LPCSTR WINAPI VcpExplain16(LPVIRTNODE lpVn, DWORD dwWhat);

/****** logical disk management ******/

typedef struct _LOGDISKDESC_S { /* ldd */
	WORD        cbSize;       /* struct size */
	LOGDISKID16 ldid;         /* logical disk ID */
	LPSTR       pszPath;      /* path this descriptor points to */
	LPSTR       pszVolLabel;  /* volume label of the disk related to it */
	LPSTR       pszDiskName;  /* name of this disk */
	WORD        wVolTime;     /* modification time of volume label */
	WORD   	    wVolDate;     /* modification date */
	DWORD       dwSerNum;     /* serial number of disk */
	WORD        wFlags;
} LOGDISKDESC_S, *LPLOGDISKDESC;

/** logical disk identifiers (LDID) **/

/* predefined LDIDs */
#define LDID_PREDEF_START	0x0001
#define LDID_PREDEF_END		0x7fff

/* registry-assigned LDIDs */
#define LDID_VAR_START		0x7000
#define LDID_VAR_END		0x7fff

/* dynamically assigned LDIDs */
#define LDID_ASSIGN_START	0x8000
#define LDID_ASSIGN_END		0xbfff

#define LDID_NULL		0
#define LDID_ABSOLUTE		((UINT)-1)
#define LDID_SRCPATH		1		/* setup source path */
#define LDID_SETUPTEMP		2		/* setup temp dir */
#define LDID_UNINSTALL		3		/* uninstall dir */
#define LDID_BACKUP		4		/* backup dir */
#define LDID_SETUPSCRATCH	5		/* setup scratch dir */
#define LDID_WIN		10		/* win dir */
#define LDID_SYS		11		/* win system dir */
#define LDID_IOS		12		/* win Iosubsys dir */
#define LDID_CMD		13		/* win command dir */
#define LDID_CPL		14		/* win control panel dir */
#define LDID_PRINT		15		/* win printer dir */
#define LDID_MAIL		16		/* win mail dir */
#define LDID_INF		17		/* win inf dir */
#define LDID_HELP		18		/* win help dir */
#define LDID_WINADMIN		19		/* admin dir */
#define LDID_FONTS		20		/* win fonts dir */
#define LDID_VIEWERS		21		/* win viewers dir */
#define LDID_VMM32		22		/* win VMM32 dir */
#define LDID_COLOR		23		/* win color mngment dir */
#define LDID_APPS		24		/* win apps dir */
#define LDID_SHARED		25		/* win shared dir */
#define LDID_WINBOOT		26		/* guaranteed win boot drive */
#define LDID_MACHINE		27		/* machine specific files */
#define LDID_HOST_WINBOOT	28
#define LDID_BOOT		30		/* boot drive root dir */
#define LDID_BOOT_HOST		31		/* boot drive host root dir */
#define LDID_OLD_WINBOOT	32		/* root subdir */
#define LDID_OLD_WIN		33		/* old windows dir */

/* flags for GenInstall() */
#define GENINSTALL_DO_FILES		1
#define GENINSTALL_DO_INI		2
#define GENINSTALL_DO_REG		4
#define GENINSTALL_DO_INI2REG		8
#define GENINSTALL_DO_CFGAUTO		16
#define GENINSTALL_DO_LOGCONFIG		32
#define GENINSTALL_DO_REGSRCPATH	64
#define GENINSTALL_DO_PERUSER		128

#define GEINISTALL_DO_INIREG		14
#define GENINSTALL_DO_ALL		255

/*
 * flags for InstallHinfSection()
 * 128 can be added, too. This means that the .inf file is provided by you
 * instead of being a 32 bit file (i.e. Windows .inf file).
 * In this case all files you install must be in the same dir
 * as your .inf file on the install disk.
 */
#define HOW_NEVER_REBOOT		0
#define HOW_ALWAYS_SILENT_REBOOT	1
#define HOW_ALWAYS_PROMPT_REBOOT	2
#define HOW_SILENT_REBOOT		3
#define HOW_PROMPT_REBOOT		4

/****** device installation stuff ******/

#define MAX_CLASS_NAME_LEN	32
#define MAX_DEVNODE_ID_LEN	256
#define MAX_GUID_STR		50

typedef struct _DEVICE_INFO
{
	UINT16              cbSize;
	struct _DEVICE_INFO *lpNextDi;
	char                szDescription[LINE_LEN];
	DWORD               dnDevnode;
	HKEY                hRegKey;
	char                szRegSubkey[MAX_DEVNODE_ID_LEN];
	char                szClassName[MAX_CLASS_NAME_LEN];
	DWORD               Flags;
	HWND16              hwndParent;
	/*LPDRIVER_NODE*/ LPVOID      lpCompatDrvList;
	/*LPDRIVER_NODE*/ LPVOID      lpClassDrvList;
	/*LPDRIVER_NODE*/ LPVOID      lpSelectedDriver;
	ATOM                atDriverPath;
	ATOM                atTempInfFile;
	HINSTANCE16         hinstClassInstaller;
	HINSTANCE16         hinstClassPropProvidor;
	HINSTANCE16         hinstDevicePropProvidor;
	HINSTANCE16         hinstBasicPropProvidor;
	FARPROC16           fpClassInstaller;
	FARPROC16           fpClassEnumPropPages;
	FARPROC16           fpDeviceEnumPropPages;
	FARPROC16           fpEnumBasicProperties;
	DWORD               dwSetupReserved;
	DWORD               dwClassInstallReserved;
	/*GENCALLBACKPROC*/ LPVOID     gicpGenInstallCallBack;

	LPARAM              gicplParam;
	UINT16              InfType;

	HINSTANCE16         hinstPrivateProblemHandler;
	FARPROC16           fpPrivateProblemHandler;
	LPARAM              lpClassInstallParams;
	struct _DEVICE_INFO *lpdiChildList;
	DWORD               dwFlagsEx;
	/*LPDRIVER_INFO*/ LPVOID       lpCompatDrvInfoList;
	/*LPDRIVER_INFO*/ LPVOID      lpClassDrvInfoList;
	char                szClassGUID[MAX_GUID_STR];
} DEVICE_INFO16, *LPDEVICE_INFO16, **LPLPDEVICE_INFO16;


extern void WINAPI GenFormStrWithoutPlaceHolders16(LPSTR,LPCSTR,HINF16);
extern RETERR16 WINAPI IpOpen16(LPCSTR,HINF16 *);
extern RETERR16 WINAPI IpClose16(HINF16);
extern RETERR16 WINAPI CtlSetLdd16(LPLOGDISKDESC);
extern RETERR16 WINAPI CtlGetLdd16(LPLOGDISKDESC);
extern RETERR16 WINAPI CtlFindLdd16(LPLOGDISKDESC);
extern RETERR16 WINAPI CtlAddLdd16(LPLOGDISKDESC);
extern RETERR16 WINAPI CtlDelLdd16(LOGDISKID16);
extern RETERR16 WINAPI CtlGetLddPath16(LOGDISKID16 ldid, LPSTR szPath);
extern RETERR16 WINAPI GenInstall16(HINF16,LPCSTR,WORD);

typedef struct tagLDD_LIST {
        LPLOGDISKDESC pldd;
        struct tagLDD_LIST *next;
} LDD_LIST;

#define INIT_LDD(ldd, LDID) \
  do { \
   memset(&(ldd), 0, sizeof(LOGDISKDESC_S)); \
   (ldd).cbSize = sizeof(LOGDISKDESC_S); \
   ldd.ldid = LDID; \
  } while(0)

#endif /* __SETUPX16_H */
