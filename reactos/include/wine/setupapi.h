/*
 * Copyright (C) 2000 James Hatheway
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

#ifndef _INC_SETUPAPI
#define _INC_SETUPAPI

#include <commctrl.h>

/* setupapi doesn't use the normal convention, it adds an underscore before A/W */
#ifdef __WINESRC__
# define DECL_WINELIB_SETUPAPI_TYPE_AW(type)  /* nothing */
#else   /* __WINESRC__ */
# define DECL_WINELIB_SETUPAPI_TYPE_AW(type)  typedef WINELIB_NAME_AW(type##_) type;
#endif  /* __WINESRC__ */

/* Define type for handle to a loaded inf file */
typedef PVOID HINF;

/* Define type for handle to a device information set */
typedef PVOID HDEVINFO;

/* Define type for setup file queue */
typedef PVOID HSPFILEQ;

typedef PVOID HDSKSPC;

/* inf structure. */
typedef struct _INFCONTEXT
{
   PVOID Inf;
   PVOID CurrentInf;
   UINT  Section;
   UINT  Line;
} INFCONTEXT, *PINFCONTEXT;

typedef struct _SP_ALTPLATFORM_INFO_V2
{
    DWORD cbSize;
    DWORD Platform;
    DWORD MajorVersion;
    DWORD MinorVersion;
    WORD  ProcessorArchitecture;
    union
    {
        WORD  Reserved;
        WORD  Flags;
    } DUMMYUNIONNAME;
    DWORD FirstValidatedMajorVersion;
    DWORD FirstValidatedMinorVersion;
} SP_ALTPLATFORM_INFO_V2, *PSP_ALTPLATFORM_INFO_V2;

#define SP_ALTPLATFORM_FLAGS_VERSION_RANGE 0x0001

typedef struct _SP_ALTPLATFORM_INFO_V1
{
    DWORD cbSize;
    DWORD Platform;
    DWORD MajorVersion;
    DWORD MinorVersion;
    WORD  ProcessorArchitecture;
    WORD  Reserved;
} SP_ALTPLATFORM_INFO_V1, *PSP_ALTPLATFORM_INFO_V1;

typedef SP_ALTPLATFORM_INFO_V2 SP_ALTPLATFORM_INFO;
typedef PSP_ALTPLATFORM_INFO_V2 PSP_ALTPLATFORM_INFO;

typedef struct _SP_FILE_COPY_PARAMS_A
{
    DWORD    cbSize;
    HSPFILEQ QueueHandle;
    PCSTR    SourceRootPath;
    PCSTR    SourcePath;
    PCSTR    SourceFilename;
    PCSTR    SourceDescription;
    PCSTR    SourceTagfile;
    PCSTR    TargetDirectory;
    PCSTR    TargetFilename;
    DWORD    CopyStyle;
    HINF     LayoutInf;
    PCSTR    SecurityDescriptor;
} SP_FILE_COPY_PARAMS_A, *PSP_FILE_COPY_PARAMS_A;

typedef struct _SP_FILE_COPY_PARAMS_W
{
    DWORD    cbSize;
    HSPFILEQ QueueHandle;
    PCWSTR   SourceRootPath;
    PCWSTR   SourcePath;
    PCWSTR   SourceFilename;
    PCWSTR   SourceDescription;
    PCWSTR   SourceTagfile;
    PCWSTR   TargetDirectory;
    PCWSTR   TargetFilename;
    DWORD    CopyStyle;
    HINF     LayoutInf;
    PCWSTR   SecurityDescriptor;
} SP_FILE_COPY_PARAMS_W, *PSP_FILE_COPY_PARAMS_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_FILE_COPY_PARAMS)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_FILE_COPY_PARAMS)

typedef struct _SP_REGISTER_CONTROL_STATUSA
{
    DWORD    cbSize;
    PCSTR    FileName;
    DWORD    Win32Error;
    DWORD    FailureCode;
} SP_REGISTER_CONTROL_STATUSA, *PSP_REGISTER_CONTROL_STATUSA;

typedef struct _SP_REGISTER_CONTROL_STATUSW
{
    DWORD    cbSize;
    PCWSTR   FileName;
    DWORD    Win32Error;
    DWORD    FailureCode;
} SP_REGISTER_CONTROL_STATUSW, *PSP_REGISTER_CONTROL_STATUSW;

DECL_WINELIB_TYPE_AW(SP_REGISTER_CONTROL_STATUS)
DECL_WINELIB_TYPE_AW(PSP_REGISTER_CONTROL_STATUS)

#define SPREG_SUCCESS       0x00000000
#define SPREG_LOADLIBRARY   0x00000001
#define SPREG_GETPROCADDR   0x00000002
#define SPREG_REGSVR        0x00000003
#define SPREG_DLLINSTALL    0x00000004
#define SPREG_TIMEOUT       0x00000005
#define SPREG_UNKNOWN       0xffffffff

typedef UINT (CALLBACK *PSP_FILE_CALLBACK_A)( PVOID Context, UINT Notification,
                                              UINT_PTR Param1, UINT_PTR Param2 );
typedef UINT (CALLBACK *PSP_FILE_CALLBACK_W)( PVOID Context, UINT Notification,
                                              UINT_PTR Param1, UINT_PTR Param2 );
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_FILE_CALLBACK)

#define LINE_LEN                    256
#define MAX_INF_STRING_LENGTH       4096
#define MAX_TITLE_LEN               60
#define MAX_INSTRUCTION_LEN         256
#define MAX_LABEL_LEN               30
#define MAX_SERVICE_NAME_LEN        256
#define MAX_SUBTITLE_LEN            256
#define SP_MAX_MACHINENAME_LENGTH   (MAX_PATH + 3)

/* Device Information structure (references a device instance that is a member
   of a device information set) */
typedef struct _SP_DEVINFO_DATA
{
   DWORD cbSize;
   GUID  ClassGuid;
   DWORD DevInst;   /* DEVINST handle */
   DWORD Reserved;
} SP_DEVINFO_DATA, *PSP_DEVINFO_DATA;

typedef struct _SP_DEVICE_INTERFACE_DATA
{
   DWORD      cbSize;
   GUID       InterfaceClassGuid;
   DWORD      Flags;
   ULONG_PTR  Reserved;
} SP_DEVICE_INTERFACE_DATA, *PSP_DEVICE_INTERFACE_DATA;

typedef struct _SP_DEVICE_INTERFACE_DETAIL_DATA_A
{
   DWORD      cbSize;
   CHAR       DevicePath[ANYSIZE_ARRAY];
} SP_DEVICE_INTERFACE_DETAIL_DATA_A, *PSP_DEVICE_INTERFACE_DETAIL_DATA_A;

typedef struct _SP_DEVICE_INTERFACE_DETAIL_DATA_W
{
   DWORD      cbSize;
   WCHAR      DevicePath[ANYSIZE_ARRAY];
} SP_DEVICE_INTERFACE_DETAIL_DATA_W, *PSP_DEVICE_INTERFACE_DETAIL_DATA_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_DEVICE_INTERFACE_DETAIL_DATA)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_DEVICE_INTERFACE_DETAIL_DATA)

typedef struct _SP_DEVINFO_LIST_DETAIL_DATA_A {
    DWORD  cbSize;
    GUID   ClassGuid;
    HANDLE RemoteMachineHandle;
    CHAR   RemoteMachineName[SP_MAX_MACHINENAME_LENGTH];
} SP_DEVINFO_LIST_DETAIL_DATA_A, *PSP_DEVINFO_LIST_DETAIL_DATA_A;

typedef struct _SP_DEVINFO_LIST_DETAIL_DATA_W {
    DWORD  cbSize;
    GUID   ClassGuid;
    HANDLE RemoteMachineHandle;
    WCHAR  RemoteMachineName[SP_MAX_MACHINENAME_LENGTH];
} SP_DEVINFO_LIST_DETAIL_DATA_W, *PSP_DEVINFO_LIST_DETAIL_DATA_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_DEVINFO_LIST_DETAIL_DATA)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_DEVINFO_LIST_DETAIL_DATA)

typedef struct _FILE_IN_CABINET_INFO_A {
  LPCSTR NameInCabinet;
  DWORD FileSize;
  DWORD Win32Error;
  WORD DosDate;
  WORD DosTime;
  WORD DosAttribs;
  CHAR FullTargetName[MAX_PATH];
} FILE_IN_CABINET_INFO_A, *PFILE_IN_CABINET_INFO_A;

typedef struct _FILE_IN_CABINET_INFO_W {
  LPCWSTR NameInCabinet;
  DWORD FileSize;
  DWORD Win32Error;
  WORD DosDate;
  WORD DosTime;
  WORD DosAttribs;
  WCHAR FullTargetName[MAX_PATH];
} FILE_IN_CABINET_INFO_W, *PFILE_IN_CABINET_INFO_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(FILE_IN_CABINET_INFO)
DECL_WINELIB_SETUPAPI_TYPE_AW(PFILE_IN_CABINET_INFO)

typedef struct _CABINET_INFO_A {
  PCSTR CabinetPath;
  PCSTR CabinetFile;
  PCSTR DiskName;
  USHORT SetId;
  USHORT CabinetNumber;
} CABINET_INFO_A, *PCABINET_INFO_A;

typedef struct _CABINET_INFO_W {
  PCWSTR CabinetPath;
  PCWSTR CabinetFile;
  PCWSTR DiskName;
  USHORT SetId;
  USHORT CabinetNumber;
} CABINET_INFO_W, *PCABINET_INFO_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(CABINET_INFO);
DECL_WINELIB_SETUPAPI_TYPE_AW(PCABINET_INFO);

typedef struct _SP_INF_INFORMATION {
    DWORD InfStyle;
    DWORD InfCount;
    BYTE VersionData[ANYSIZE_ARRAY];
} SP_INF_INFORMATION, *PSP_INF_INFORMATION;

#define INF_STYLE_NONE           0x00
#define INF_STYLE_OLDNT          0x01
#define INF_STYLE_WIN4           0x02
#define INF_STYLE_CACHE_ENABLE   0x10
#define INF_STYLE_CACHE_DISABLE  0x20

#define FILEOP_COPY              0
#define FILEOP_RENAME            1
#define FILEOP_DELETE            2
#define FILEOP_BACKUP            3

#define FILEOP_ABORT             0
#define FILEOP_DOIT              1
#define FILEOP_SKIP              2
#define FILEOP_RETRY             FILEOP_DOIT
#define FILEOP_NEWPATH           4

typedef struct _FILEPATHS_A
{
    PCSTR  Target;
    PCSTR  Source;
    UINT   Win32Error;
    DWORD  Flags;
} FILEPATHS_A, *PFILEPATHS_A;

typedef struct _FILEPATHS_W
{
    PCWSTR Target;
    PCWSTR Source;
    UINT   Win32Error;
    DWORD  Flags;
} FILEPATHS_W, *PFILEPATHS_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(FILEPATHS)
DECL_WINELIB_SETUPAPI_TYPE_AW(PFILEPATHS)

#define SPFILENOTIFY_STARTQUEUE           0x0001
#define SPFILENOTIFY_ENDQUEUE             0x0002
#define SPFILENOTIFY_STARTSUBQUEUE        0x0003
#define SPFILENOTIFY_ENDSUBQUEUE          0x0004
#define SPFILENOTIFY_STARTDELETE          0x0005
#define SPFILENOTIFY_ENDDELETE            0x0006
#define SPFILENOTIFY_DELETEERROR          0x0007
#define SPFILENOTIFY_STARTRENAME          0x0008
#define SPFILENOTIFY_ENDRENAME            0x0009
#define SPFILENOTIFY_RENAMEERROR          0x000a
#define SPFILENOTIFY_STARTCOPY            0x000b
#define SPFILENOTIFY_ENDCOPY              0x000c
#define SPFILENOTIFY_COPYERROR            0x000d
#define SPFILENOTIFY_NEEDMEDIA            0x000e
#define SPFILENOTIFY_QUEUESCAN            0x000f
#define SPFILENOTIFY_CABINETINFO          0x0010
#define SPFILENOTIFY_FILEINCABINET        0x0011
#define SPFILENOTIFY_NEEDNEWCABINET       0x0012
#define SPFILENOTIFY_FILEEXTRACTED        0x0013
#define SPFILENOTIFY_FILEOPDELAYED        0x0014
#define SPFILENOTIFY_STARTBACKUP          0x0015
#define SPFILENOTIFY_BACKUPERROR          0x0016
#define SPFILENOTIFY_ENDBACKUP            0x0017
#define SPFILENOTIFY_QUEUESCAN_EX         0x0018
#define SPFILENOTIFY_STARTREGISTRATION    0x0019
#define SPFILENOTIFY_ENDREGISTRATION      0x0020
#define SPFILENOTIFY_QUEUESCAN_SIGNERINFO 0x0040

#define SPFILENOTIFY_LANGMISMATCH         0x00010000
#define SPFILENOTIFY_TARGETEXISTS         0x00020000
#define SPFILENOTIFY_TARGETNEWER          0x00040000

#define SPINST_LOGCONFIG                  0x00000001
#define SPINST_INIFILES                   0x00000002
#define SPINST_REGISTRY                   0x00000004
#define SPINST_INI2REG                    0x00000008
#define SPINST_FILES                      0x00000010
#define SPINST_BITREG                     0x00000020
#define SPINST_REGSVR                     0x00000040
#define SPINST_UNREGSVR                   0x00000080
#define SPINST_PROFILEITEMS               0x00000100
#define SPINST_COPYINF                    0x00000200
#define SPINST_ALL                        0x000003ff
#define SPINST_SINGLESECTION              0x00010000
#define SPINST_LOGCONFIG_IS_FORCED        0x00020000
#define SPINST_LOGCONFIGS_ARE_OVERRIDES   0x00040000
#define SPINST_REGISTERCALLBACKAWARE      0x00080000

#define SP_COPY_DELETESOURCE              0x00000001
#define SP_COPY_REPLACEONLY               0x00000002
#define SP_COPY_NEWER                     0x00000004
#define SP_COPY_NEWER_OR_SAME             SP_COPY_NEWER
#define SP_COPY_NOOVERWRITE               0x00000008
#define SP_COPY_NODECOMP                  0x00000010
#define SP_COPY_LANGUAGEAWARE             0x00000020
#define SP_COPY_SOURCE_ABSOLUTE           0x00000040
#define SP_COPY_SOURCEPATH_ABSOLUTE       0x00000080
#define SP_COPY_IN_USE_NEEDS_REBOOT       0x00000100
#define SP_COPY_FORCE_IN_USE              0x00000200
#define SP_COPY_NOSKIP                    0x00000400
#define SP_FLAG_CABINETCONTINUATION       0x00000800
#define SP_COPY_FORCE_NOOVERWRITE         0x00001000
#define SP_COPY_FORCE_NEWER               0x00002000
#define SP_COPY_WARNIFSKIP                0x00004000
#define SP_COPY_NOBROWSE                  0x00008000
#define SP_COPY_NEWER_ONLY                0x00010000
#define SP_COPY_SOURCE_SIS_MASTER         0x00020000
#define SP_COPY_OEMINF_CATALOG_ONLY       0x00040000
#define SP_COPY_REPLACE_BOOT_FILE         0x00080000
#define SP_COPY_NOPRUNE                   0x00100000
#define SP_COPY_OEM_F6_INF                0x00200000

#define FLG_ADDREG_DELREG_BIT             0x00008000
#define FLG_ADDREG_BINVALUETYPE           0x00000001
#define FLG_ADDREG_NOCLOBBER              0x00000002
#define FLG_ADDREG_DELVAL                 0x00000004
#define FLG_ADDREG_APPEND                 0x00000008
#define FLG_ADDREG_KEYONLY                0x00000010
#define FLG_ADDREG_OVERWRITEONLY          0x00000020
#define FLG_ADDREG_64BITKEY               0x00001000
#define FLG_ADDREG_KEYONLY_COMMON         0x00002000
#define FLG_ADDREG_32BITKEY               0x00004000
#define FLG_ADDREG_TYPE_SZ                0x00000000
#define FLG_ADDREG_TYPE_MULTI_SZ          0x00010000
#define FLG_ADDREG_TYPE_EXPAND_SZ         0x00020000
#define FLG_ADDREG_TYPE_BINARY           (0x00000000 | FLG_ADDREG_BINVALUETYPE)
#define FLG_ADDREG_TYPE_DWORD            (0x00010000 | FLG_ADDREG_BINVALUETYPE)
#define FLG_ADDREG_TYPE_NONE             (0x00020000 | FLG_ADDREG_BINVALUETYPE)
#define FLG_ADDREG_TYPE_MASK             (0xFFFF0000 | FLG_ADDREG_BINVALUETYPE)

#define FLG_DELREG_VALUE                 (0x00000000)
#define FLG_DELREG_TYPE_MASK             FLG_ADDREG_TYPE_MASK
#define FLG_DELREG_TYPE_SZ               FLG_ADDREG_TYPE_SZ
#define FLG_DELREG_TYPE_MULTI_SZ         FLG_ADDREG_TYPE_MULTI_SZ
#define FLG_DELREG_TYPE_EXPAND_SZ        FLG_ADDREG_TYPE_EXPAND_SZ
#define FLG_DELREG_TYPE_BINARY           FLG_ADDREG_TYPE_BINARY
#define FLG_DELREG_TYPE_DWORD            FLG_ADDREG_TYPE_DWORD
#define FLG_DELREG_TYPE_NONE             FLG_ADDREG_TYPE_NONE
#define FLG_DELREG_64BITKEY              FLG_ADDREG_64BITKEY
#define FLG_DELREG_KEYONLY_COMMON        FLG_ADDREG_KEYONLY_COMMON
#define FLG_DELREG_32BITKEY              FLG_ADDREG_32BITKEY
#define FLG_DELREG_OPERATION_MASK        (0x000000FE)
#define FLG_DELREG_MULTI_SZ_DELSTRING    (FLG_DELREG_TYPE_MULTI_SZ | FLG_ADDREG_DELREG_BIT | 0x00000002)

#define FLG_REGSVR_DLLREGISTER           0x00000001
#define FLG_REGSVR_DLLINSTALL            0x00000002

/* Class installer function codes */
#define DIF_SELECTDEVICE                    0x01
#define DIF_INSTALLDEVICE                   0x02
#define DIF_ASSIGNRESOURCES                 0x03
#define DIF_PROPERTIES                      0x04
#define DIF_REMOVE                          0x05
#define DIF_FIRSTTIMESETUP                  0x06
#define DIF_FOUNDDEVICE                     0x07
#define DIF_SELECTCLASSDRIVERS              0x08
#define DIF_VALIDATECLASSDRIVERS            0x09
#define DIF_INSTALLCLASSDRIVERS             0x0a
#define DIF_CALCDISKSPACE                   0x0b
#define DIF_DESTROYPRIVATEDATA              0x0c
#define DIF_VALIDATEDRIVER                  0x0d
#define DIF_MOVEDEVICE                      0x0e
#define DIF_DETECT                          0x0f
#define DIF_INSTALLWIZARD                   0x10
#define DIF_DESTROYWIZARDDATA               0x11
#define DIF_PROPERTYCHANGE                  0x12
#define DIF_ENABLECLASS                     0x13
#define DIF_DETECTVERIFY                    0x14
#define DIF_INSTALLDEVICEFILES              0x15
#define DIF_UNREMOVE                        0x16
#define DIF_SELECTBESTCOMPATDRV             0x17
#define DIF_ALLOW_INSTALL                   0x18
#define DIF_REGISTERDEVICE                  0x19
#define DIF_NEWDEVICEWIZARD_PRESELECT       0x1a
#define DIF_NEWDEVICEWIZARD_SELECT          0x1b
#define DIF_NEWDEVICEWIZARD_PREANALYZE      0x1c
#define DIF_NEWDEVICEWIZARD_POSTANALYZE     0x1d
#define DIF_NEWDEVICEWIZARD_FINISHINSTALL   0x1e
#define DIF_UNUSED1                         0x1f
#define DIF_INSTALLINTERFACES               0x20
#define DIF_DETECTCANCEL                    0x21
#define DIF_REGISTER_COINSTALLERS           0x22
#define DIF_ADDPROPERTYPAGE_ADVANCED        0x23
#define DIF_ADDPROPERTYPAGE_BASIC           0x24
#define DIF_RESERVED1                       0x25
#define DIF_TROUBLESHOOTER                  0x26
#define DIF_POWERMESSAGEWAKE                0x27
#define DIF_ADDREMOTEPROPERTYPAGE_ADVANCED  0x28
#define DIF_UPDATEDRIVER_UI                 0x29
#define DIF_RESERVED2                       0x30

/* Directory ids */
#define DIRID_ABSOLUTE                (-1)
#define DIRID_ABSOLUTE_16BIT          0xffff
#define DIRID_NULL                    0
#define DIRID_SRCPATH                 1
#define DIRID_WINDOWS                 10
#define DIRID_SYSTEM                  11
#define DIRID_DRIVERS                 12
#define DIRID_IOSUBSYS                DIRID_DRIVERS
#define DIRID_INF                     17
#define DIRID_HELP                    18
#define DIRID_FONTS                   20
#define DIRID_VIEWERS                 21
#define DIRID_COLOR                   23
#define DIRID_APPS                    24
#define DIRID_SHARED                  25
#define DIRID_BOOT                    30
#define DIRID_SYSTEM16                50
#define DIRID_SPOOL                   51
#define DIRID_SPOOLDRIVERS            52
#define DIRID_USERPROFILE             53
#define DIRID_LOADER                  54
#define DIRID_PRINTPROCESSOR          55
#define DIRID_DEFAULT                 DIRID_SYSTEM

#define DIRID_COMMON_STARTMENU        16406
#define DIRID_COMMON_PROGRAMS         16407
#define DIRID_COMMON_STARTUP          16408
#define DIRID_COMMON_DESKTOPDIRECTORY 16409
#define DIRID_COMMON_FAVORITES        16415
#define DIRID_COMMON_APPDATA          16419
#define DIRID_PROGRAM_FILES           16422
#define DIRID_SYSTEM_X86              16425
#define DIRID_PROGRAM_FILES_X86       16426
#define DIRID_PROGRAM_FILES_COMMON    16427
#define DIRID_PROGRAM_FILES_COMMONX86 16428
#define DIRID_COMMON_TEMPLATES        16429
#define DIRID_COMMON_DOCUMENTS        16430

#define DIRID_USER                    0x8000


/* Error code */

#define ERROR_EXPECTED_SECTION_NAME       (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0)
#define ERROR_BAD_SECTION_NAME_LINE       (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|1)
#define ERROR_SECTION_NAME_TOO_LONG       (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|2)
#define ERROR_GENERAL_SYNTAX              (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|3)
#define ERROR_WRONG_INF_STYLE             (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x100)
#define ERROR_SECTION_NOT_FOUND           (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x101)
#define ERROR_LINE_NOT_FOUND              (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x102)
#define ERROR_NO_BACKUP                   (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x103)
#define ERROR_NO_ASSOCIATED_CLASS         (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x200)
#define ERROR_CLASS_MISMATCH              (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x201)
#define ERROR_DUPLICATE_FOUND             (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x202)
#define ERROR_NO_DRIVER_SELECTED          (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x203)
#define ERROR_KEY_DOES_NOT_EXIST          (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x204)
#define ERROR_INVALID_DEVINST_NAME        (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x205)
#define ERROR_INVALID_CLASS               (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x206)
#define ERROR_DEVINST_ALREADY_EXISTS      (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x207)
#define ERROR_DEVINFO_NOT_REGISTERED      (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x208)
#define ERROR_INVALID_REG_PROPERTY        (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x209)
#define ERROR_NO_INF                      (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x20A)
#define ERROR_NO_SUCH_DEVINST             (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x20B)
#define ERROR_CANT_LOAD_CLASS_ICON        (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x20C)
#define ERROR_INVALID_CLASS_INSTALLER     (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x20D)
#define ERROR_DI_DO_DEFAULT               (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x20E)
#define ERROR_DI_NOFILECOPY               (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x20F)
#define ERROR_INVALID_HWPROFILE           (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x210)
#define ERROR_NO_DEVICE_SELECTED          (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x211)
#define ERROR_DEVINFO_LIST_LOCKED         (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x212)
#define ERROR_DEVINFO_DATA_LOCKED         (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x213)
#define ERROR_DI_BAD_PATH                 (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x214)
#define ERROR_NO_CLASSINSTALL_PARAMS      (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x215)
#define ERROR_FILEQUEUE_LOCKED            (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x216)
#define ERROR_BAD_SERVICE_INSTALLSECT     (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x217)
#define ERROR_NO_CLASS_DRIVER_LIST        (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x218)
#define ERROR_NO_ASSOCIATED_SERVICE       (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x219)
#define ERROR_NO_DEFAULT_DEVICE_INTERFACE (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x21A)
#define ERROR_DEVICE_INTERFACE_ACTIVE     (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x21B)
#define ERROR_DEVICE_INTERFACE_REMOVED    (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x21C)
#define ERROR_BAD_INTERFACE_INSTALLSECT   (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x21D)
#define ERROR_NO_SUCH_INTERFACE_CLASS     (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x21E)
#define ERROR_INVALID_REFERENCE_STRING    (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x21F)
#define ERROR_INVALID_MACHINENAME         (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x220)
#define ERROR_REMOTE_COMM_FAILURE         (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x221)
#define ERROR_MACHINE_UNAVAILABLE         (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x222)
#define ERROR_NO_CONFIGMGR_SERVICES       (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x223)
#define ERROR_INVALID_PROPPAGE_PROVIDER   (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x224)
#define ERROR_NO_SUCH_DEVICE_INTERFACE    (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x225)
#define ERROR_DI_POSTPROCESSING_REQUIRED  (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x226)
#define ERROR_INVALID_COINSTALLER         (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x227)
#define ERROR_NO_COMPAT_DRIVERS           (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x228)
#define ERROR_NO_DEVICE_ICON              (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x229)
#define ERROR_INVALID_INF_LOGCONFIG       (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x22A)
#define ERROR_DI_DONT_INSTALL             (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x22B)
#define ERROR_INVALID_FILTER_DRIVER       (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x22C)
#define ERROR_NON_WINDOWS_NT_DRIVER       (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x22D)
#define ERROR_NON_WINDOWS_DRIVER          (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x22E)
#define ERROR_NO_CATALOG_FOR_OEM_INF      (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x22F)
#define ERROR_DEVINSTALL_QUEUE_NONNATIVE  (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x230)
#define ERROR_NOT_DISABLEABLE             (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x231)
#define ERROR_CANT_REMOVE_DEVINST         (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x232)
#define ERROR_INVALID_TARGET              (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x233)
#define ERROR_DRIVER_NONNATIVE            (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x234)
#define ERROR_IN_WOW64                    (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x235)
#define ERROR_SET_SYSTEM_RESTORE_POINT    (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x236)
#define ERROR_INCORRECTLY_COPIED_INF      (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x237)
#define ERROR_SCE_DISABLED                (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x238)
#define ERROR_NO_DEFAULT_INTERFACE_DEVICE ERROR_NO_DEFAULT_DEVICE_INTERFACE
#define ERROR_INTERFACE_DEVICE_ACTIVE     ERROR_DEVICE_INTERFACE_ACTIVE
#define ERROR_INTERFACE_DEVICE_REMOVED    ERROR_DEVICE_INTERFACE_REMOVED
#define ERROR_NO_SUCH_INTERFACE_DEVICE    ERROR_NO_SUCH_DEVICE_INTERFACE
#define ERROR_NOT_INSTALLED               (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x1000)

/* flags for SetupDiGetClassDevs */
#define DIGCF_DEFAULT         0x00000001
#define DIGCF_PRESENT         0x00000002
#define DIGCF_ALLCLASSES      0x00000004
#define DIGCF_PROFILE         0x00000008
#define DIGCF_DEVICEINTERFACE 0x00000010

/* setup device registry property codes */
#define SPDRP_DEVICEDESC                  0x00000000
#define SPDRP_HARDWAREID                  0x00000001
#define SPDRP_COMPATIBLEIDS               0x00000002
#define SPDRP_UNUSED0                     0x00000003
#define SPDRP_SERVICE                     0x00000004
#define SPDRP_UNUSED1                     0x00000005
#define SPDRP_UNUSED2                     0x00000006
#define SPDRP_CLASS                       0x00000007
#define SPDRP_CLASSGUID                   0x00000008
#define SPDRP_DRIVER                      0x00000009
#define SPDRP_CONFIGFLAGS                 0x0000000a
#define SPDRP_MFG                         0x0000000b
#define SPDRP_FRIENDLYNAME                0x0000000c
#define SPDRP_LOCATION_INFORMATION        0x0000000d
#define SPDRP_PHYSICAL_DEVICE_OBJECT_NAME 0x0000000e
#define SPDRP_CAPABILITIES                0x0000000f
#define SPDRP_UI_NUMBER                   0x00000010
#define SPDRP_UPPERFILTERS                0x00000011
#define SPDRP_LOWERFILTERS                0x00000012
#define SPDRP_BUSTYPEGUID                 0x00000013
#define SPDRP_LEGACYBUSTYPE               0x00000014
#define SPDRP_BUSNUMBER                   0x00000015
#define SPDRP_ENUMERATOR_NAME             0x00000016
#define SPDRP_SECURITY                    0x00000017
#define SPDRP_SECURITY_SDS                0x00000018
#define SPDRP_DEVTYPE                     0x00000019
#define SPDRP_EXCLUSIVE                   0x0000001a
#define SPDRP_CHARACTERISTICS             0x0000001b
#define SPDRP_ADDRESS                     0x0000001c
#define SPDRP_UI_NUMBER_DESC_FORMAT       0x0000001d
#define SPDRP_DEVICE_POWER_DATA           0x0000001e
#define SPDRP_REMOVAL_POLICY              0x0000001f
#define SPDRP_REMOVAL_POLICY_HW_DEFAULT   0x00000020
#define SPDRP_REMOVAL_POLICY_OVERRIDE     0x00000021
#define SPDRP_INSTALL_STATE               0x00000022
#define SPDRP_MAXIMUM_PROPERTY            0x00000023

void     WINAPI InstallHinfSectionA( HWND hwnd, HINSTANCE handle, LPCSTR cmdline, INT show );
void     WINAPI InstallHinfSectionW( HWND hwnd, HINSTANCE handle, LPCWSTR cmdline, INT show );
#define         InstallHinfSection WINELIB_NAME_AW(InstallHinfSection)
HINF     WINAPI SetupOpenInfFileA( PCSTR name, PCSTR pszclass, DWORD style, UINT *error );
HINF     WINAPI SetupOpenInfFileW( PCWSTR name, PCWSTR pszclass, DWORD style, UINT *error );
#define         SetupOpenInfFile WINELIB_NAME_AW(SetupOpenInfFile)
BOOL     WINAPI SetupOpenAppendInfFileA( PCSTR, HINF, UINT * );
BOOL     WINAPI SetupOpenAppendInfFileW( PCWSTR, HINF, UINT * );
#define         SetupOpenAppendInfFile WINELIB_NAME_AW(SetupOpenAppendInfFile)
void     WINAPI SetupCloseInfFile( HINF hinf );
BOOL     WINAPI SetupGetLineByIndexA( HINF, PCSTR, DWORD, INFCONTEXT * );
BOOL     WINAPI SetupGetLineByIndexW( HINF, PCWSTR, DWORD, INFCONTEXT * );
#define         SetupGetLineByIndex WINELIB_NAME_AW(SetupGetLineByIndex)
LONG     WINAPI SetupGetLineCountA( HINF hinf, PCSTR section );
LONG     WINAPI SetupGetLineCountW( HINF hinf, PCWSTR section );
#define         SetupGetLineCount WINELIB_NAME_AW(SetupGetLineCount)
BOOL     WINAPI SetupFindFirstLineA( HINF hinf, PCSTR section, PCSTR key, INFCONTEXT *context );
BOOL     WINAPI SetupFindFirstLineW( HINF hinf, PCWSTR section, PCWSTR key, INFCONTEXT *context );
#define         SetupFindFirstLine WINELIB_NAME_AW(SetupFindFirstLine)
BOOL     WINAPI SetupFindNextLine( PINFCONTEXT context_in, PINFCONTEXT context_out );
BOOL     WINAPI SetupFindNextMatchLineA( PINFCONTEXT context_in, PCSTR key, PINFCONTEXT context_out );
BOOL     WINAPI SetupFindNextMatchLineW( PINFCONTEXT context_in, PCWSTR key, PINFCONTEXT context_out );
#define         SetupFindNextMatchLine WINELIB_NAME_AW(SetupFindNextMatchLine)
BOOL     WINAPI SetupGetLineTextA( PINFCONTEXT context, HINF hinf, PCSTR section_name,PCSTR key_name, PSTR buffer, DWORD size, PDWORD required );
BOOL     WINAPI SetupGetLineTextW( PINFCONTEXT context, HINF hinf, PCWSTR section_name, PCWSTR key_name, PWSTR buffer, DWORD size, PDWORD required );
#define         SetupGetLineText WINELIB_NAME_AW(SetupGetLineText)
DWORD    WINAPI SetupGetFieldCount( PINFCONTEXT context );
BOOL     WINAPI SetupGetIntField( PINFCONTEXT context, DWORD index, PINT result );
BOOL     WINAPI SetupGetStringFieldA( PINFCONTEXT context, DWORD index, PSTR buffer, DWORD size, PDWORD required );
BOOL     WINAPI SetupGetStringFieldW( PINFCONTEXT context, DWORD index, PWSTR buffer, DWORD size, PDWORD required );
#define         SetupGetStringField WINELIB_NAME_AW(SetupGetStringField)
BOOL     WINAPI SetupGetBinaryField( PINFCONTEXT context, DWORD index, BYTE *buffer, DWORD size, LPDWORD required );
BOOL     WINAPI SetupGetMultiSzFieldA( PINFCONTEXT context, DWORD index, PSTR buffer, DWORD size, LPDWORD required );
BOOL     WINAPI SetupGetMultiSzFieldW( PINFCONTEXT context, DWORD index, PWSTR buffer, DWORD size, LPDWORD required );
#define         SetupGetMultiSzField WINELIB_NAME_AW(SetupGetMultiSzField)
BOOL     WINAPI SetupSetDirectoryIdA( HINF, DWORD, PCSTR );
BOOL     WINAPI SetupSetDirectoryIdW( HINF, DWORD, PCWSTR );
#define         SetupSetDirectoryId WINELIB_NAME_AW(SetupSetDirectoryId)
HSPFILEQ WINAPI SetupOpenFileQueue(void);
BOOL     WINAPI SetupCloseFileQueue( HSPFILEQ );
BOOL     WINAPI SetupSetFileQueueAlternatePlatformA( HSPFILEQ, PSP_ALTPLATFORM_INFO, PCSTR );
BOOL     WINAPI SetupSetFileQueueAlternatePlatformW( HSPFILEQ, PSP_ALTPLATFORM_INFO, PCWSTR );
#define         SetupSetFileQueueAlternatePlatform WINELIB_NAME_AW(SetupSetFileQueueAlternatePlatform)
BOOL     WINAPI SetupQueueCopyA(HSPFILEQ,PCSTR,PCSTR,PCSTR,PCSTR,PCSTR,PCSTR,PCSTR,DWORD);
BOOL     WINAPI SetupQueueCopyW(HSPFILEQ,PCWSTR,PCWSTR,PCWSTR,PCWSTR,PCWSTR,PCWSTR,PCWSTR,DWORD);
#define         SetupQueueCopy WINELIB_NAME_AW(SetupQueueCopy)
BOOL     WINAPI SetupQueueCopyIndirectA( PSP_FILE_COPY_PARAMS_A );
BOOL     WINAPI SetupQueueCopyIndirectW( PSP_FILE_COPY_PARAMS_W );
#define         SetupQueueCopyIndirect WINELIB_NAME_AW(SetupQueueCopyIndirect)
BOOL     WINAPI SetupQueueDefaultCopyA( HSPFILEQ, HINF, PCSTR, PCSTR, PCSTR, DWORD );
BOOL     WINAPI SetupQueueDefaultCopyW( HSPFILEQ, HINF, PCWSTR, PCWSTR, PCWSTR, DWORD );
#define         SetupQueueDefaultCopy WINELIB_NAME_AW(SetupQueueDefaultCopy)
BOOL     WINAPI SetupQueueDeleteA( HSPFILEQ, PCSTR, PCSTR );
BOOL     WINAPI SetupQueueDeleteW( HSPFILEQ, PCWSTR, PCWSTR );
#define         SetupQueueDelete WINELIB_NAME_AW(SetupQueueDelete)
BOOL     WINAPI SetupQueueRenameA( HSPFILEQ, PCSTR, PCSTR, PCSTR, PCSTR );
BOOL     WINAPI SetupQueueRenameW( HSPFILEQ, PCWSTR, PCWSTR, PCWSTR, PCWSTR );
#define         SetupQueueRename WINELIB_NAME_AW(SetupQueueRename)
BOOL     WINAPI SetupCommitFileQueueA( HWND, HSPFILEQ, PSP_FILE_CALLBACK_A, PVOID );
BOOL     WINAPI SetupCommitFileQueueW( HWND, HSPFILEQ, PSP_FILE_CALLBACK_W, PVOID );
#define         SetupCommitFileQueue WINELIB_NAME_AW(SetupCommitFileQueue)
BOOL     WINAPI SetupScanFileQueueA( HSPFILEQ, DWORD, HWND, PSP_FILE_CALLBACK_A, PVOID, PDWORD );
BOOL     WINAPI SetupScanFileQueueW( HSPFILEQ, DWORD, HWND, PSP_FILE_CALLBACK_W, PVOID, PDWORD );
#define         SetupScanFileQueue WINELIB_NAME_AW(SetupScanFileQueue)
BOOL     WINAPI SetupGetFileQueueCount( HSPFILEQ, UINT, PUINT );
BOOL     WINAPI SetupGetFileQueueFlags( HSPFILEQ, PDWORD );
BOOL     WINAPI SetupSetFileQueueFlags( HSPFILEQ, DWORD, DWORD );
BOOL     WINAPI SetupQueueCopySectionA( HSPFILEQ, PCSTR, HINF, HINF, PCSTR, DWORD );
BOOL     WINAPI SetupQueueCopySectionW( HSPFILEQ, PCWSTR, HINF, HINF, PCWSTR, DWORD );
#define         SetupQueueCopySection WINELIB_NAME_AW(SetupQueueCopySection)
BOOL     WINAPI SetupQueueDeleteSectionA( HSPFILEQ, HINF, HINF, PCSTR );
BOOL     WINAPI SetupQueueDeleteSectionW( HSPFILEQ, HINF, HINF, PCWSTR );
#define         SetupQueueDeleteSection WINELIB_NAME_AW(SetupQueueDeleteSection)
BOOL     WINAPI SetupQueueRenameSectionA( HSPFILEQ, HINF, HINF, PCSTR );
BOOL     WINAPI SetupQueueRenameSectionW( HSPFILEQ, HINF, HINF, PCWSTR );
#define         SetupQueueRenameSection WINELIB_NAME_AW(SetupQueueRenameSection)
PVOID    WINAPI SetupInitDefaultQueueCallback( HWND );
PVOID    WINAPI SetupInitDefaultQueueCallbackEx( HWND, HWND, UINT, DWORD, PVOID );
void     WINAPI SetupTermDefaultQueueCallback( PVOID );
UINT     WINAPI SetupDefaultQueueCallbackA( PVOID, UINT, UINT_PTR, UINT_PTR );
UINT     WINAPI SetupDefaultQueueCallbackW( PVOID, UINT, UINT_PTR, UINT_PTR );
#define         SetupDefaultQueueCallback WINELIB_NAME_AW(SetupDefaultQueueCallback)
BOOL     WINAPI SetupDiDestroyDeviceInfoList(HDEVINFO);
BOOL     WINAPI SetupDiEnumDeviceInterfaces(HDEVINFO, PSP_DEVINFO_DATA, const GUID *, DWORD, PSP_DEVICE_INTERFACE_DATA);
HDEVINFO WINAPI SetupDiGetClassDevsA(CONST GUID *,LPCSTR,HWND,DWORD);
HDEVINFO WINAPI SetupDiGetClassDevsW(CONST GUID *,LPCWSTR,HWND,DWORD);
#define         SetupDiGetClassDevs WINELIB_NAME_AW(SetupDiGetClassDevs)
BOOL     WINAPI SetupDiGetDeviceInterfaceDetailA(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_A,
                                                 DWORD, PDWORD, PSP_DEVINFO_DATA);
BOOL     WINAPI SetupDiGetDeviceInterfaceDetailW(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_W,
                                                 DWORD, PDWORD, PSP_DEVINFO_DATA);
#define         SetupDiGetDeviceInterfaceDetail WINELIB_NAME_AW(SetupDiGetDeviceInterfaceDetail)
BOOL     WINAPI SetupDiGetDeviceRegistryPropertyA(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);
BOOL     WINAPI SetupInstallFilesFromInfSectionA( HINF, HINF, HSPFILEQ, PCSTR, PCSTR, UINT );
BOOL     WINAPI SetupInstallFilesFromInfSectionW( HINF, HINF, HSPFILEQ, PCWSTR, PCWSTR, UINT );
#define         SetupInstallFilesFromInfSection WINELIB_NAME_AW(SetupInstallFilesFromInfSection)
BOOL     WINAPI SetupInstallFromInfSectionA(HWND,HINF,PCSTR,UINT,HKEY,PCSTR,UINT,
                                            PSP_FILE_CALLBACK_A,PVOID,HDEVINFO,PSP_DEVINFO_DATA);
BOOL     WINAPI SetupInstallFromInfSectionW(HWND,HINF,PCWSTR,UINT,HKEY,PCWSTR,UINT,
                                            PSP_FILE_CALLBACK_W,PVOID,HDEVINFO,PSP_DEVINFO_DATA);
#define         SetupInstallFromInfSection WINELIB_NAME_AW(SetupInstallFromInfSection)
BOOL     WINAPI SetupIterateCabinetA(PCSTR, DWORD, PSP_FILE_CALLBACK_A, PVOID);
BOOL     WINAPI SetupIterateCabinetW(PCWSTR, DWORD, PSP_FILE_CALLBACK_W, PVOID);
#define         SetupIterateCabinet WINELIB_NAME_AW(SetupIterateCabinet)

#undef DECL_WINELIB_SETUPAPI_TYPE_AW

#endif /* _INC_SETUPAPI */
