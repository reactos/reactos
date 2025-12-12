/*
 * Copyright (C) 2000 James Hatheway
 * Copyright (C) 2007 Juan Lang
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _INC_SETUPAPI
#define _INC_SETUPAPI

#include <commctrl.h>
#include <devpropdef.h>

#ifdef _WIN64
#include <pshpack8.h>
#else
#include <pshpack1.h>
#endif

/* setupapi doesn't use the normal convention, it adds an underscore before A/W */
#ifdef WINE_NO_UNICODE_MACROS
# define DECL_WINELIB_SETUPAPI_TYPE_AW(type)  /* nothing */
#else
# define DECL_WINELIB_SETUPAPI_TYPE_AW(type)  typedef WINELIB_NAME_AW(type##_) type;
#endif

#ifdef _SETUPAPI_
#define WINSETUPAPI
#else
#define WINSETUPAPI DECLSPEC_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Define type for handle to a loaded inf file */
typedef PVOID HINF;

/* Define type for handle to a device information set */
typedef PVOID HDEVINFO;

/* Define type for handle to a setup log file */
typedef PVOID HSPFILELOG;

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

typedef UINT DI_FUNCTION;

typedef struct _SP_CLASSINSTALL_HEADER
{
  DWORD       cbSize;
  DI_FUNCTION InstallFunction;
} SP_CLASSINSTALL_HEADER, *PSP_CLASSINSTALL_HEADER;

typedef struct _SP_ENABLECLASS_PARAMS
{
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    GUID                   ClassGuid;
    DWORD                  EnableMessage;
} SP_ENABLECLASS_PARAMS, *PSP_ENABLECLASS_PARAMS;

/* SP_ENABLECLASS_PARAMS EnableMessage values */
#define ENABLECLASS_QUERY   0
#define ENABLECLASS_SUCCESS 1
#define ENABLECLASS_FAILURE 2

typedef struct _SP_PROPCHANGE_PARAMS
{
  SP_CLASSINSTALL_HEADER  ClassInstallHeader;
  DWORD  StateChange;
  DWORD  Scope;
  DWORD  HwProfile;
} SP_PROPCHANGE_PARAMS, *PSP_PROPCHANGE_PARAMS;

/* SP_PROPCHANGE_PARAMS StateChange values */
#define DICS_ENABLE      0x00000001
#define DICS_DISABLE     0x00000002
#define DICS_PROPCHANGE  0x00000003
#define DICS_START       0x00000004
#define DICS_STOP        0x00000005
/* SP_PROPCHANGE_PARAMS Scope values */
#define DICS_FLAG_GLOBAL         0x00000001
#define DICS_FLAG_CONFIGSPECIFIC 0x00000002
#define DICS_FLAG_CONFIGGENERAL  0x00000004


typedef struct _SP_DEVINSTALL_PARAMS_A
{
    DWORD               cbSize;
    DWORD               Flags;
    DWORD               FlagsEx;
    HWND                hwndParent;
    PSP_FILE_CALLBACK_A InstallMsgHandler;
    PVOID               InstallMsgHandlerContext;
    HSPFILEQ            FileQueue;
    ULONG_PTR           ClassInstallReserved;
    DWORD               Reserved;
    CHAR                DriverPath[MAX_PATH];
} SP_DEVINSTALL_PARAMS_A, *PSP_DEVINSTALL_PARAMS_A;

typedef struct _SP_DEVINSTALL_PARAMS_W
{
    DWORD               cbSize;
    DWORD               Flags;
    DWORD               FlagsEx;
    HWND                hwndParent;
    PSP_FILE_CALLBACK_W InstallMsgHandler;
    PVOID               InstallMsgHandlerContext;
    HSPFILEQ            FileQueue;
    ULONG_PTR           ClassInstallReserved;
    DWORD               Reserved;
    WCHAR               DriverPath[MAX_PATH];
} SP_DEVINSTALL_PARAMS_W, *PSP_DEVINSTALL_PARAMS_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_DEVINSTALL_PARAMS)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_DEVINSTALL_PARAMS)

/* SP_DEVINSTALL_PARAMS Flags values */
#define DI_SHOWOEM             0x00000001
#define DI_SHOWCOMPAT          0x00000002
#define DI_SHOWCLASS           0x00000004
#define DI_SHOWALL             0x00000007
#define DI_NOVCP               0x00000008
#define DI_DIDCOMPAT           0x00000010
#define DI_DIDCLASS            0x00000020
#define DI_AUTOASSIGNRES       0x00000040
#define DI_NEEDRESTART         0x00000080
#define DI_NEEDREBOOT          0x00000100
#define DI_NOBROWSE            0x00000200
#define DI_MULTMFGS            0x00000400
#define DI_DISABLED            0x00000800
#define DI_GENERALPAGE_ADDED   0x00001000
#define DI_RESOURCEPAGE_ADDED  0x00002000
#define DI_PROPERTIES_CHANGE   0x00004000
#define DI_INF_IS_SORTED       0x00080000
#define DI_ENUMSINGLEINF       0x00010000
#define DI_DONOTCALLCONFIGMG   0x00020000
#define DI_INSTALLDISABLED     0x00040000
#define DI_COMPAT_FROM_CLASS   0x00080000
#define DI_CLASSINSTALLPARAMS  0x00100000
#define DI_NODI_DEFAULTACTION  0x00200000
#define DI_QUIETINSTALL        0x00800000
#define DI_NOFILECOPY          0x01000000
#define DI_FORCECOPY           0x02000000
#define DI_DRIVERPAGE_ADDED    0x04000000
#define DI_USECI_SELECTSTRINGS 0x08000000
#define DI_OVERRIDE_INFFLAGS   0x10000000
#define DI_PROPS_NOCHANGEUSAGE 0x20000000
#define DI_NOSELECTICONS       0x40000000
#define DI_NOWRITE_IDS         0x80000000
/* SP_DEVINSTALL_PARAMS FlagsEx values */
#define DI_FLAGSEX_USEOLDINFSEARCH          0x00000001
#define DI_FLAGSEX_RESERVED2                0x00000002
#define DI_FLAGSEX_CI_FAILED                0x00000004
#define DI_FLAGSEX_FINISHINSTALL_ACTION     0x00000008
#define DI_FLAGSEX_DIDINFOLIST              0x00000010
#define DI_FLAGSEX_DIDCOMPATINFO            0x00000020
#define DI_FLAGSEX_FILTERCLASSES            0x00000040
#define DI_FLAGSEX_SETFAILEDINSTALL         0x00000080
#define DI_FLAGSEX_DEVICECHANGE             0x00000100
#define DI_FLAGSEX_ALWAYSWRITEIDS           0x00000200
#define DI_FLAGSEX_PROPCHANGE_PENDING       0x00000400
#define DI_FLAGSEX_ALLOWEXCLUDEDDRVS        0x00000800
#define DI_FLAGSEX_NOUIONQUERYREMOVE        0x00001000
#define DI_FLAGSEX_USECLASSFORCOMPAT        0x00002000
#define DI_FLAGSEX_RESERVED3                0x00004000
#define DI_FLAGSEX_NO_DRVREG_MODIFY         0x00008000
#define DI_FLAGSEX_IN_SYSTEM_SETUP          0x00010000
#define DI_FLAGSEX_INET_DRIVER              0x00020000
#define DI_FLAGSEX_APPENDDRIVERLIST         0x00040000
#define DI_FLAGSEX_PREINSTALLBACKUP         0x00080000
#define DI_FLAGSEX_BACKUPONREPLACE          0x00100000
#define DI_FLAGSEX_DRIVERLIST_FROM_URL      0x00200000
#define DI_FLAGSEX_RESERVED1                0x00400000
#define DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS 0x00800000
#define DI_FLAGSEX_POWERPAGE_ADDED          0x01000000
#define DI_FLAGSEX_FILTERSIMILARDRIVERS     0x02000000
#define DI_FLAGSEX_INSTALLEDDRIVER          0x04000000
#define DI_FLAGSEX_NO_CLASSLIST_NODE_MERGE  0x08000000
#define DI_FLAGSEX_ALTPLATFORM_DRVSEARCH    0x10000000
#define DI_FLAGSEX_RESTART_DEVICE_ONLY      0x20000000
#define DI_FLAGSEX_RECURSIVESEARCH          0x40000000
#define DI_FLAGSEX_SEARCH_PUBLISHED_INFS    0x80000000

typedef struct _SP_REMOVEDEVICE_PARAMS
{
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    DWORD                  Scope;
    DWORD                  HwProfile;
} SP_REMOVEDEVICE_PARAMS, *PSP_REMOVEDEVICE_PARAMS;

/* SP_REMOVEDEVICE_PARAMS Scope values */
#define DI_REMOVEDEVICE_GLOBAL         0x00000001
#define DI_REMOVEDEVICE_CONFIGSPECIFIC 0x00000002

typedef struct _SP_UNREMOVEDEVICE_PARAMS
{
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    DWORD                  Scope;
    DWORD                  HwProfile;
} SP_UNREMOVEDEVICE_PARAMS, *PSP_UNREMOVEDEVICE_PARAMS;

/* SP_UNREMOVEDEVICE_PARAMS Scope values */
#define DI_UNREMOVEDEVICE_CONFIGSPECIFIC 0x00000002

typedef struct _SP_SELECTDEVICE_PARAMS_A
{
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    CHAR                   Title[MAX_TITLE_LEN];
    CHAR                   Instructions[MAX_INSTRUCTION_LEN];
    CHAR                   ListLabel[MAX_LABEL_LEN];
    CHAR                   SubTitle[MAX_SUBTITLE_LEN];
    BYTE                   Reserved[2];
} SP_SELECTDEVICE_PARAMS_A, *PSP_SELECTDEVICE_PARAMS_A;

typedef struct _SP_SELECTDEVICE_PARAMS_W
{
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    WCHAR                  Title[MAX_TITLE_LEN];
    WCHAR                  Instructions[MAX_INSTRUCTION_LEN];
    WCHAR                  ListLabel[MAX_LABEL_LEN];
    WCHAR                  SubTitle[MAX_SUBTITLE_LEN];
} SP_SELECTDEVICE_PARAMS_W, *PSP_SELECTDEVICE_PARAMS_W;

typedef BOOL (CALLBACK *PDETECT_PROGRESS_NOTIFY)(PVOID ProgressNotifyParam,
        DWORD DetectComplete);

typedef struct _SP_DETECTDEVICE_PARAMS
{
    SP_CLASSINSTALL_HEADER  ClassInstallHeader;
    PDETECT_PROGRESS_NOTIFY DetectProgressNotify;
    PVOID                   ProgressNotifyParam;
} SP_DETECTDEVICE_PARAMS, *PSP_DETECTDEVICE_PARAMS;

#define MAX_INSTALLWIZARD_DYNAPAGES 20

typedef struct _SP_INSTALLWIZARD_DATA
{
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    DWORD                  Flags;
    HPROPSHEETPAGE         DynamicPages[MAX_INSTALLWIZARD_DYNAPAGES];
    DWORD                  NumDynamicPages;
    DWORD                  DynamicPageFlags;
    DWORD                  PrivateFlags;
    LPARAM                 PrivateData;
    HWND                   hwndWizardDlg;
} SP_INSTALLWIZARD_DATA, *PSP_INSTALLWIZARD_DATA;

/* SP_INSTALLWIZARD_DATA Flags values */
#define NDW_INSTALLFLAG_DIDFACTDEFS        0x00000001
#define NDW_INSTALLFLAG_HARDWAREALLREADYIN 0x00000002
#define NDW_INSTALLFLAG_NEEDRESTART        DI_NEEDRESTART
#define NDW_INSTALLFLAG_NEEDREBOOT         DI_NEEDREBOOT
#define NDW_INSTALLFLAG_NEEDSHUTDOWN       0x00000200
#define NDW_INSTALLFLAG_EXPRESSINTRO       0x00000400
#define NDW_INSTALLFLAG_SKIPISDEVINSTALLED 0x00000800
#define NDW_INSTALLFLAG_NODETECTEDDEVS     0x00001000
#define NDW_INSTALLFLAG_INSTALLSPECIFIC    0x00002000
#define NDW_INSTALLFLAG_SKIPCLASSLIST      0x00004000
#define NDW_INSTALLFLAG_CI_PICKED_OEM      0x00008000
#define NDW_INSTALLFLAG_PCMCIAMODE         0x00010000
#define NDW_INSTALLFLAG_PCMCIADEVICE       0x00020000
#define NDW_INSTALLFLAG_USERCANCEL         0x00040000
#define NDW_INSTALLFLAG_KNOWNCLASS         0x00080000
/* SP_INSTALLWIZARD_DATA DynamicPageFlags values */
#define DYNAWIZ_FLAG_PAGESADDED             0x00000001
#define DYNAWIZ_FLAG_INSTALLDET_NEXT        0x00000002
#define DYNAWIZ_FLAG_INSTALLDET_PREV        0x00000004
#define DYNAWIZ_FLAG_ANALYZE_HANDLECONFLICT 0x00000008

/* Resource IDs */
#define MIN_IDD_DYNAWIZ_RESOURCE_ID 10000
#define MAX_IDD_DYNAWIZ_RESOURCE_ID 11000

#define IDD_DYNAWIZ_FIRSTPAGE                10000
#define IDD_DYNAWIZ_SELECT_PREVPAGE          10001
#define IDD_DYNAWIZ_SELECT_NEXTPAGE          10002
#define IDD_DYNAWIZ_ANALYZE_PREVPAGE         10003
#define IDD_DYNAWIZ_ANALYZE_NEXTPAGE         10004
#define IDD_DYNAWIZ_INSTALLDETECTED_PREVPAGE 10006
#define IDD_DYNAWIZ_INSTALLDETECTED_NEXTPAGE 10007
#define IDD_DYNAWIZ_INSTALLDETECTED_NODEVS   10008
#define IDD_DYNAWIZ_SELECTDEV_PAGE           10009
#define IDD_DYNAWIZ_ANALYZEDEV_PAGE          10010
#define IDD_DYNAWIZ_INSTALLDETECTEDDEVS_PAGE 10011
#define IDD_DYNAWIZ_SELECTCLASS_PAGE         10012

#define IDI_RESOURCEFIRST        159
#define IDI_RESOURCE             IDI_RESOURCEFIRST
#define IDI_RESOURCELAST         161
#define IDI_RESOURCEOVERLAYFIRST 161
#define IDI_RESOURCEOVERLAYLAST  161

#define IDI_CLASSICON_OVERLAYFIRST 500
#define IDI_CLASSICON_OVERLAYLAST  502
#define IDI_PROBLEM_OVL            500
#define IDI_DISABLED_OVL           501
#define IDI_FORCED_OVL             502

typedef struct _SP_NEWDEVICEWIZARD_DATA
{
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    DWORD                  Flags;
    HPROPSHEETPAGE         DynamicPages[MAX_INSTALLWIZARD_DYNAPAGES];
    DWORD                  NumDynamicPages;
    HWND                   hwndWizardDlg;
} SP_NEWDEVICEWIZARD_DATA, *PSP_NEWDEVICEWIZARD_DATA;

typedef SP_NEWDEVICEWIZARD_DATA  SP_ADDPROPERTYPAGE_DATA;
typedef PSP_NEWDEVICEWIZARD_DATA PSP_ADDPROPERTYPAGE_DATA;

typedef struct _SP_TROUBLESHOOTER_PARAMS_A
{
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    CHAR                   ChmFile[MAX_PATH];
    CHAR                   HtmlTroubleShooter[MAX_PATH];
} SP_TROUBLESHOOTER_PARAMS_A, *PSP_TROUBLESHOOTER_PARAMS_A;

typedef struct _SP_TROUBLESHOOTER_PARAMS_W
{
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    WCHAR                  ChmFile[MAX_PATH];
    WCHAR                  HtmlTroubleShooter[MAX_PATH];
} SP_TROUBLESHOOTER_PARAMS_W, *PSP_TROUBLESHOOTER_PARAMS_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_TROUBLESHOOTER_PARAMS)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_TROUBLESHOOTER_PARAMS)

typedef struct _SP_POWERMESSAGEWAKE_PARAMS_A
{
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    CHAR                   PowerMessageWake[LINE_LEN * 2];
} SP_POWERMESSAGEWAKE_PARAMS_A, *PSP_POWERMESSAGEWAKE_PARAMS_A;

typedef struct _SP_POWERMESSAGEWAKE_PARAMS_W
{
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    WCHAR                  PowerMessageWake[LINE_LEN * 2];
} SP_POWERMESSAGEWAKE_PARAMS_W, *PSP_POWERMESSAGEWAKE_PARAMS_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_POWERMESSAGEWAKE_PARAMS)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_POWERMESSAGEWAKE_PARAMS)

typedef struct _SP_DRVINFO_DATA_V1_A
{
    DWORD     cbSize;
    DWORD     DriverType;
    ULONG_PTR Reserved;
    CHAR      Description[LINE_LEN];
    CHAR      MfgName[LINE_LEN];
    CHAR      ProviderName[LINE_LEN];
} SP_DRVINFO_DATA_V1_A, *PSP_DRVINFO_DATA_V1_A;

typedef struct _SP_DRVINFO_DATA_V1_W
{
    DWORD     cbSize;
    DWORD     DriverType;
    ULONG_PTR Reserved;
    WCHAR     Description[LINE_LEN];
    WCHAR     MfgName[LINE_LEN];
    WCHAR     ProviderName[LINE_LEN];
} SP_DRVINFO_DATA_V1_W, *PSP_DRVINFO_DATA_V1_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_DRVINFO_DATA_V1)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_DRVINFO_DATA_V1)

typedef struct _SP_DRVINFO_DATA_V2_A
{
    DWORD     cbSize;
    DWORD     DriverType;
    ULONG_PTR Reserved;
    CHAR      Description[LINE_LEN];
    CHAR      MfgName[LINE_LEN];
    CHAR      ProviderName[LINE_LEN];
    FILETIME  DriverDate;
    DWORDLONG DriverVersion;
} SP_DRVINFO_DATA_V2_A, *PSP_DRVINFO_DATA_V2_A;

typedef struct _SP_DRVINFO_DATA_V2_W
{
    DWORD     cbSize;
    DWORD     DriverType;
    ULONG_PTR Reserved;
    WCHAR     Description[LINE_LEN];
    WCHAR     MfgName[LINE_LEN];
    WCHAR     ProviderName[LINE_LEN];
    FILETIME  DriverDate;
    DWORDLONG DriverVersion;
} SP_DRVINFO_DATA_V2_W, *PSP_DRVINFO_DATA_V2_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_DRVINFO_DATA_V2)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_DRVINFO_DATA_V2)

typedef SP_DRVINFO_DATA_V2_A SP_DRVINFO_DATA_A;
typedef PSP_DRVINFO_DATA_V2_A PSP_DRVINFO_DATA_A;
typedef SP_DRVINFO_DATA_V2_W SP_DRVINFO_DATA_W;
typedef PSP_DRVINFO_DATA_V2_W PSP_DRVINFO_DATA_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_DRVINFO_DATA)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_DRVINFO_DATA)

typedef struct _SP_DRVINFO_DETAIL_DATA_A
{
    DWORD     cbSize;
    FILETIME  InfDate;
    DWORD     CompatIDsOffset;
    DWORD     CompatIDsLength;
    ULONG_PTR Reserved;
    CHAR      SectionName[LINE_LEN];
    CHAR      InfFileName[MAX_PATH];
    CHAR      DrvDescription[LINE_LEN];
    CHAR      HardwareID[ANYSIZE_ARRAY];
} SP_DRVINFO_DETAIL_DATA_A, *PSP_DRVINFO_DETAIL_DATA_A;

typedef struct _SP_DRVINFO_DETAIL_DATA_W
{
    DWORD     cbSize;
    FILETIME  InfDate;
    DWORD     CompatIDsOffset;
    DWORD     CompatIDsLength;
    ULONG_PTR Reserved;
    WCHAR     SectionName[LINE_LEN];
    WCHAR     InfFileName[MAX_PATH];
    WCHAR     DrvDescription[LINE_LEN];
    WCHAR     HardwareID[ANYSIZE_ARRAY];
} SP_DRVINFO_DETAIL_DATA_W, *PSP_DRVINFO_DETAIL_DATA_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_DRVINFO_DETAIL_DATA)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_DRVINFO_DETAIL_DATA)

typedef struct _SP_DRVINSTALL_PARAMS
{
    DWORD     cbSize;
    DWORD     Rank;
    DWORD     Flags;
    DWORD_PTR PrivateData;
    DWORD     Reserved;
} SP_DRVINSTALL_PARAMS, *PSP_DRVINSTALL_PARAMS;

/* SP_DRVINSTALL_PARAMS Flags values */
#define DNF_DUPDESC               0x00000001
#define DNF_OLDDRIVER             0x00000002
#define DNF_EXCLUDEFROMLIST       0x00000004
#define DNF_NODRIVER              0x00000008
#define DNF_LEGACYINF             0x00000010
#define DNF_CLASS_DRIVER          0x00000020
#define DNF_COMPATIBLE_DRIVER     0x00000040
#define DNF_INET_DRIVER           0x00000080
#define DNF_UNUSED1               0x00000100
#define DNF_INDEXED_DRIVER        0x00000200
#define DNF_OLD_INET_DRIVER       0x00000400
#define DNF_BAD_DRIVER            0x00000800
#define DNF_DUPPROVIDER           0x00001000
#define DNF_INF_IS_SIGNED         0x00002000
#define DNF_OEM_F6_INF            0x00004000
#define DNF_DUPDRIVERVER          0x00008000
#define DNF_BASIC_DRIVER          0x00010000
#define DNF_AUTHENTICODE_SIGNED   0x00020000
#define DNF_INSTALLEDDRIVER       0x00040000
#define DNF_ALWAYSEXCLUDEFROMLIST 0x00080000
/* SP_DRVINSTALL_PARAMS Rank values */
#define DRIVER_HARDWAREID_RANK             0x00000fff
#define DRIVER_COMPATID_RANK               0x00003fff
#define DRIVER_UNTRUSTED_RANK              0x00008000
#define DRIVER_UNTRUSTED_HARDWAREID_RANK   0x00008fff
#define DRIVER_UNTRUSTED_COMPATID_RANK     0x0000bfff
#define DRIVER_W9X_SUSPECT_RANK            0x0000c000
#define DRIVER_W9X_SUSPECT_HARDWAREID_RANK 0x0000cfff

/* Device Information structure (references a device instance that is a member
   of a device information set) */
typedef struct _SP_DEVINFO_DATA
{
   DWORD     cbSize;
   GUID      ClassGuid;
   DWORD     DevInst;   /* DEVINST handle */
   ULONG_PTR Reserved;
} SP_DEVINFO_DATA, *PSP_DEVINFO_DATA;

typedef struct _SP_DEVICE_INTERFACE_DATA
{
   DWORD      cbSize;
   GUID       InterfaceClassGuid;
   DWORD      Flags;
   ULONG_PTR  Reserved;
} SP_DEVICE_INTERFACE_DATA, *PSP_DEVICE_INTERFACE_DATA;

#define SPINT_ACTIVE  0x00000001
#define SPINT_DEFAULT 0x00000002
#define SPINT_REMOVED 0x00000004

typedef SP_DEVICE_INTERFACE_DATA  SP_INTERFACE_DEVICE_DATA;
typedef PSP_DEVICE_INTERFACE_DATA PSP_INTERFACE_DEVICE_DATA;
#define SPID_ACTIVE  SPINT_ACTIVE
#define SPID_DEFAULT SPINT_DEFAULT
#define SPID_REMOVED SPINT_REMOVED

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

typedef DWORD (CALLBACK *PSP_DETSIG_CMPPROC)(HDEVINFO, PSP_DEVINFO_DATA,
 PSP_DEVINFO_DATA, PVOID);

typedef struct _COINSTALLER_CONTEXT_DATA
{
    BOOL  PostProcessing;
    DWORD InstallResult;
    PVOID PrivateData;
} COINSTALLER_CONTEXT_DATA, *PCOINSTALLER_CONTEXT_DATA;

typedef struct _SP_CLASSIMAGELIST_DATA
{
    DWORD      cbSize;
    HIMAGELIST ImageList;
    ULONG_PTR  Reserved;
} SP_CLASSIMAGELIST_DATA, *PSP_CLASSIMAGELIST_DATA;

typedef struct _SP_PROPSHEETPAGE_REQUEST
{
    DWORD            cbSize;
    DWORD            PageRequested;
    HDEVINFO         DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
} SP_PROPSHEETPAGE_REQUEST, *PSP_PROPSHEETPAGE_REQUEST;

/* SP_PROPSHEETPAGE_REQUEST PageRequested values */
#define SPPSR_SELECT_DEVICE_RESOURCES      1
#define SPPSR_ENUM_BASIC_DEVICE_PROPERTIES 2
#define SPPSR_ENUM_ADV_DEVICE_PROPERTIES   3

typedef struct _SP_BACKUP_QUEUE_PARAMS_V1_A
{
    DWORD cbSize;
    CHAR  FullInfPath[MAX_PATH];
    INT   FilenameOffset;
} SP_BACKUP_QUEUE_PARAMS_V1_A, *PSP_BACKUP_QUEUE_PARAMS_V1_A;

typedef struct _SP_BACKUP_QUEUE_PARAMS_V1_W
{
    DWORD cbSize;
    WCHAR FullInfPath[MAX_PATH];
    INT   FilenameOffset;
} SP_BACKUP_QUEUE_PARAMS_V1_W, *PSP_BACKUP_QUEUE_PARAMS_V1_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_BACKUP_QUEUE_PARAMS_V1)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_BACKUP_QUEUE_PARAMS_V1)

typedef struct _SP_BACKUP_QUEUE_PARAMS_V2_A
{
    DWORD cbSize;
    CHAR  FullInfPath[MAX_PATH];
    INT   FilenameOffset;
    CHAR  ReinstallInstance[MAX_PATH];
} SP_BACKUP_QUEUE_PARAMS_V2_A, *PSP_BACKUP_QUEUE_PARAMS_V2_A;

typedef struct _SP_BACKUP_QUEUE_PARAMS_V2_W
{
    DWORD cbSize;
    WCHAR FullInfPath[MAX_PATH];
    INT   FilenameOffset;
    WCHAR ReinstallInstance[MAX_PATH];
} SP_BACKUP_QUEUE_PARAMS_V2_W, *PSP_BACKUP_QUEUE_PARAMS_V2_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_BACKUP_QUEUE_PARAMS_V2)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_BACKUP_QUEUE_PARAMS_V2)

typedef SP_BACKUP_QUEUE_PARAMS_V2_A SP_BACKUP_QUEUE_PARAMS_A;
typedef PSP_BACKUP_QUEUE_PARAMS_V2_A PSP_BACKUP_QUEUE_PARAMS_A;
typedef SP_BACKUP_QUEUE_PARAMS_V2_W SP_BACKUP_QUEUE_PARAMS_W;
typedef PSP_BACKUP_QUEUE_PARAMS_V2_W PSP_BACKUP_QUEUE_PARAMS_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_BACKUP_QUEUE_PARAMS)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_BACKUP_QUEUE_PARAMS)

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

DECL_WINELIB_SETUPAPI_TYPE_AW(CABINET_INFO)
DECL_WINELIB_SETUPAPI_TYPE_AW(PCABINET_INFO)

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

#define COPYFLG_WARN_IF_SKIP                  0x00000001
#define COPYFLG_NOSKIP                        0x00000002
#define COPYFLG_NOVERSIONCHECK                0x00000004
#define COPYFLG_FORCE_FILE_IN_USE             0x00000008
#define COPYFLG_NO_OVERWRITE                  0x00000010
#define COPYFLG_NO_VERSION_DIALOG             0x00000020
#define COPYFLG_OVERWRITE_OLDER_ONLY          0x00000040
#define COPYFLG_PROTECTED_WINDOWS_DRIVER_FILE 0x00000100
#define COPYFLG_REPLACEONLY                   0x00000400
#define COPYFLG_NODECOMP                      0x00000800
#define COPYFLG_REPLACE_BOOT_FILE             0x00001000
#define COPYFLG_NOPRUNE                       0x00002000
#define COPYFLG_IN_USE_TRY_RENAME             0x00004000

#define DELFLG_IN_USE  0x00000001
#define DELFLG_IN_USE1 0x00010000

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

typedef struct _FILEPATHS_SIGNERINFO_A
{
    PCSTR Target;
    PCSTR Source;
    UINT  Win32Error;
    DWORD Flags;
    PCSTR DigitalSigner;
    PCSTR Version;
    PCSTR CatalogFile;
} FILEPATHS_SIGNERINFO_A, *PFILEPATHS_SIGNERINFO_A;

typedef struct _FILEPATHS_SIGNERINFO_W
{
    PCWSTR Target;
    PCWSTR Source;
    UINT   Win32Error;
    DWORD  Flags;
    PCWSTR DigitalSigner;
    PCWSTR Version;
    PCWSTR CatalogFile;
} FILEPATHS_SIGNERINFO_W, *PFILEPATHS_SIGNERINFO_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(FILEPATHS_SIGNERINFO)

typedef struct _SOURCE_MEDIA_A
{
    PCSTR Reserved;
    PCSTR Tagfile;
    PCSTR Description;
    PCSTR SourcePath;
    PCSTR SourceFile;
    DWORD Flags;
} SOURCE_MEDIA_A, *PSOURCE_MEDIA_A;

typedef struct _SOURCE_MEDIA_W
{
    PCWSTR Reserved;
    PCWSTR Tagfile;
    PCWSTR Description;
    PCWSTR SourcePath;
    PCWSTR SourceFile;
    DWORD  Flags;
} SOURCE_MEDIA_W, *PSOURCE_MEDIA_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SOURCE_MEDIA)

typedef struct _SP_ORIGINAL_FILE_INFO_A
{
    DWORD cbSize;
    CHAR  OriginalInfName[MAX_PATH];
    CHAR  OriginalCatalogName[MAX_PATH];
} SP_ORIGINAL_FILE_INFO_A, *PSP_ORIGINAL_FILE_INFO_A;

typedef struct _SP_ORIGINAL_FILE_INFO_W
{
    DWORD cbSize;
    WCHAR  OriginalInfName[MAX_PATH];
    WCHAR  OriginalCatalogName[MAX_PATH];
} SP_ORIGINAL_FILE_INFO_W, *PSP_ORIGINAL_FILE_INFO_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_ORIGINAL_FILE_INFO)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_ORIGINAL_FILE_INFO)

typedef struct _SP_INF_SIGNER_INFO_A
{
    DWORD cbSize;
    CHAR  CatalogFile[MAX_PATH];
    CHAR  DigitalSigner[MAX_PATH];
    CHAR  DigitalSignerVersion[MAX_PATH];
} SP_INF_SIGNER_INFO_A, *PSP_INF_SIGNER_INFO_A;

typedef struct _SP_INF_SIGNER_INFO_W
{
    DWORD cbSize;
    WCHAR CatalogFile[MAX_PATH];
    WCHAR DigitalSigner[MAX_PATH];
    WCHAR DigitalSignerVersion[MAX_PATH];
} SP_INF_SIGNER_INFO_W, *PSP_INF_SIGNER_INFO_W;

DECL_WINELIB_SETUPAPI_TYPE_AW(SP_INF_SIGNER_INFO)
DECL_WINELIB_SETUPAPI_TYPE_AW(PSP_INF_SIGNER_INFO)

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

#define SPSVCINST_TAGTOFRONT               0x00000001
#define SPSVCINST_ASSOCSERVICE             0x00000002
#define SPSVCINST_DELETEEVENTLOGENTRY      0x00000004
#define SPSVCINST_NOCLOBBER_DISPLAYNAME    0x00000008
#define SPSVCINST_NOCLOBBER_STARTTYPE      0x00000010
#define SPSVCINST_NOCLOBBER_ERRORCONTROL   0x00000020
#define SPSVCINST_NOCLOBBER_LOADORDERGROUP 0x00000040
#define SPSVCINST_NOCLOBBER_DEPENDENCIES   0x00000080
#define SPSVCINST_NOCLOBBER_DESCRIPTION    0x00000100
#define SPSVCINST_STOPSERVICE              0x00000200
#define SPSVCINST_CLOBBER_SECURITY         0x00000400
#define SPSVCINST_STARTSERVICE             0x00000800

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

#define SP_BACKUP_BACKUPPASS 0x00000001
#define SP_BACKUP_DEMANDPASS 0x00000002
#define SP_BACKUP_SPECIAL    0x00000004
#define SP_BACKUP_BOOTFILE   0x00000008

#define SPOST_NONE  0
#define SPOST_PATH  1
#define SPOST_URL   2
#define SPOST_MAX   3

#define SPQ_SCAN_FILE_PRESENCE            0x00000001
#define SPQ_SCAN_FILE_VALIDITY            0x00000002
#define SPQ_SCAN_USE_CALLBACK             0x00000004
#define SPQ_SCAN_USE_CALLBACKEX           0x00000008
#define SPQ_SCAN_INFORM_USER              0x00000010
#define SPQ_SCAN_PRUNE_COPY_QUEUE         0x00000020
#define SPQ_SCAN_USE_CALLBACK_SIGNERINFO  0x00000040
#define SPQ_SCAN_PRUNE_DELREN             0x00000080

#define SPQ_DELAYED_COPY 0x00000001

#define SPQ_FLAG_BACKUP_AWARE      0x00000001
#define SPQ_FLAG_ABORT_IF_UNSIGNED 0x00000002
#define SPQ_FLAG_FILES_MODIFIED    0x00000004
#define SPQ_FLAG_VALID             0x00000007

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

#define FLG_PROFITEM_CURRENTUSER         0x00000001
#define FLG_PROFITEM_DELETE              0x00000002
#define FLG_PROFITEM_GROUP               0x00000004
#define FLG_PROFITEM_CSIDL               0x00000008

#define DI_NOVCP 0x00000008

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
#define DIF_FINISHINSTALL_ACTION            0x2a
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
#define DIRID_DRIVER_STORE            13
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
#define ERROR_UNKNOWN_EXCEPTION           (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x239)
#define ERROR_PNP_REGISTRY_ERROR          (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x23A)
#define ERROR_REMOTE_REQUEST_UNSUPPORTED  (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x23B)
#define ERROR_NOT_AN_INSTALLED_OEM_INF    (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x23C)
#define ERROR_INF_IN_USE_BY_DEVICES       (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x23D)
#define ERROR_DI_FUNCTION_OBSOLETE        (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x23E)
#define ERROR_NO_AUTHENTICODE_CATALOG     (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x23F)
#define ERROR_AUTHENTICODE_DISALLOWED     (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x240)
#define ERROR_AUTHENTICODE_TRUSTED_PUBLISHER (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x241)
#define ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x242)
#define ERROR_AUTHENTICODE_PUBLISHER_NOT_TRUSTED (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x243)
#define ERROR_SIGNATURE_OSATTRIBUTE_MISMATCH (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x244)
#define ERROR_ONLY_VALIDATE_VIA_AUTHENTICODE (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x245)
#define ERROR_DEVICE_INSTALLER_NOT_READY  (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x246)
#define ERROR_DRIVER_STORE_ADD_FAILED     (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x247)
#define ERROR_DEVICE_INSTALL_BLOCKED      (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x248)
#define ERROR_DRIVER_INSTALL_BLOCKED      (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x249)
#define ERROR_WRONG_INF_TYPE              (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x24A)
#define ERROR_FILE_HASH_NOT_IN_CATALOG    (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x24B)
#define ERROR_DRIVER_STORE_DELETE_FAILED  (APPLICATION_ERROR_MASK|ERROR_SEVERITY_ERROR|0x24C)
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

/* Flags for SetupDiOpenClassRegKeyEx */
#define DIOCR_INSTALLER       0x00000001
#define DIOCR_INTERFACE       0x00000002

/* Flags for SetupDiBuildClassInfoList(Ex) */
#define DIBCI_NOINSTALLCLASS  0x00000001
#define DIBCI_NODISPLAYCLASS  0x00000002

/* Flags for SetupDiCreateDeviceInfo */
#define DICD_GENERATE_ID       0x00000001
#define DICD_INHERIT_CLASSDRVS 0x00000002

/* Flags for SetupDiOpenDeviceInfo */
#define DIOD_INHERIT_CLASSDRVS 0x00000002
#define DIOD_CANCEL_REMOVE     0x00000004

/* Flags for SetupDiOpenDeviceInterface */
#define DIODI_NO_ADD 0x00000001

/* Flags for SetupDiRegisterDeviceInfo */
#define SPRDI_FIND_DUPS 0x00000001

/* Values for SetupDi*Info* DriverType */
#define SPDIT_NODRIVER     0x00000000
#define SPDIT_CLASSDRIVER  0x00000001
#define SPDIT_COMPATDRIVER 0x00000002

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
#define SPDRP_BASE_CONTAINERID            0x00000024
#define SPDRP_MAXIMUM_PROPERTY            0x00000025

#define DPROMPT_SUCCESS        0
#define DPROMPT_CANCEL         1
#define DPROMPT_SKIPFILE       2
#define DPROMPT_BUFFERTOOSMALL 3
#define DPROMPT_OUTOFMEMORY    4

#define SETDIRID_NOT_FULL_PATH 0x00000001

#define IDF_NOBROWSE     0x00000001
#define IDF_NOSKIP       0x00000002
#define IDF_NODETAILS    0x00000004
#define IDF_NOCOMPRESSED 0x00000008
#define IDF_CHECKFIRST   0x00000100
#define IDF_NOBEEP       0x00000200
#define IDF_NOFOREGROUND 0x00000400
#define IDF_WARNIFSKIP   0x00000800

#define IDF_NOREMOVABLEMEDIAPROMPT 0x00001000
#define IDF_USEDISKNAMEASPROMPT    0x00002000
#define IDF_OEMDISK                0x80000000

#define INFINFO_INF_SPEC_IS_HINF        1
#define INFINFO_INF_NAME_IS_ABSOLUTE    2
#define INFINFO_DEFAULT_SEARCH          3
#define INFINFO_REVERSE_DEFAULT_SEARCH  4
#define INFINFO_INF_PATH_LIST_SEARCH    5

#define LogSeverity         DWORD
#define LogSevInformation   0x00000000
#define LogSevWarning       0x00000001
#define LogSevError         0x00000002
#define LogSevFatalError    0x00000003
#define LogSevMaximum       0x00000004

#define SRCINFO_PATH           1
#define SRCINFO_TAGFILE        2
#define SRCINFO_DESCRIPTION    3
#define SRCINFO_FLAGS          4
#define SRCINFO_TAGFILE2       5

#define SRC_FLAGS_CABFILE      (0x0010)

#define FILE_COMPRESSION_NONE       0
#define FILE_COMPRESSION_WINLZA     1
#define FILE_COMPRESSION_MSZIP      2
#define FILE_COMPRESSION_NTCAB      3

#define SPDSL_IGNORE_DISK              0x00000001
#define SPDSL_DISALLOW_NEGATIVE_ADJUST 0x00000002

/* SetupInitializeFileLog Flags values */
#define SPFILELOG_SYSTEMLOG 0x00000001
#define SPFILELOG_FORCENEW  0x00000002
#define SPFILELOG_QUERYONLY 0x00000004

/* SetupLogFile Flags values */
#define SPFILELOG_OEMFILE 0x00000001

/* SetupDiCreateDevRegKey, SetupDiOpenDevRegKey, and SetupDiDeleteDevRegKey
 * KeyType values
 */
#define DIREG_DEV  0x00000001
#define DIREG_DRV  0x00000002
#define DIREG_BOTH 0x00000004

/* SetupDiDrawMiniIcon Flags values */
#define DMI_MASK    0x00000001
#define DMI_BKCOLOR 0x00000002
#define DMI_USERECT 0x00000004

/* SetupDiGetClassDevPropertySheets PropertySheetType values */
#define DIGCDP_FLAG_BASIC           0x00000001
#define DIGCDP_FLAG_ADVANCED        0x00000002
#define DIGCDP_FLAG_REMOTE_BASIC    0x00000003
#define DIGCDP_FLAG_REMOTE_ADVANCED 0x00000004

typedef enum {
    SetupFileLogSourceFilename,
    SetupFileLogChecksum,
    SetupFileLogDiskTagfile,
    SetupFileLogDiskDescription,
    SetupFileLogOtherInfo,
    SetupFileLogMax
} SetupFileLogInfo;

/* SetupDiGetWizardPage PageType values */
#define SPWPT_SELECTDEVICE 0x00000001
/* SetupDiGetWizardPage Flags values */
#define SPWP_USE_DEVINFO_DATA 0x00000001

/* SetupDiGetCustomDeviceProperty Flags values */
#define DICUSTOMDEVPROP_MERGE_MULTISZ 0x00000001

/* SetupConfigureWmiFromInfSection Flags values */
#define SCWMI_CLOBBER_SECURITY 0x00000001

/* SetupUninstallOEMInf Flags values */
#define SUOI_FORCEDELETE 0x00000001

WINSETUPAPI void     WINAPI InstallHinfSectionA( HWND hwnd, HINSTANCE handle, PCSTR cmdline, INT show );
WINSETUPAPI void     WINAPI InstallHinfSectionW( HWND hwnd, HINSTANCE handle, PCWSTR cmdline, INT show );
#define                     InstallHinfSection WINELIB_NAME_AW(InstallHinfSection)
WINSETUPAPI BOOL     WINAPI SetupAddSectionToDiskSpaceListA(HDSKSPC, HINF, HINF, PCSTR, UINT, PVOID, UINT);
WINSETUPAPI BOOL     WINAPI SetupAddSectionToDiskSpaceListW(HDSKSPC, HINF, HINF, PCWSTR, UINT, PVOID, UINT);
#define                     SetupAddSectionToDiskSpaceList WINELIB_NAME_AW(SetupAddSectionToDiskSpaceList)
WINSETUPAPI BOOL     WINAPI SetupAddToDiskSpaceListA(HDSKSPC, PCSTR, LONGLONG, UINT, PVOID, UINT);
WINSETUPAPI BOOL     WINAPI SetupAddToDiskSpaceListW(HDSKSPC, PCWSTR, LONGLONG, UINT, PVOID, UINT);
#define                     SetupAddToDiskSpaceList WINELIB_NAME_AW(SetupAddToDiskSpaceList)
WINSETUPAPI BOOL     WINAPI SetupAddToSourceListA(DWORD, PCSTR);
WINSETUPAPI BOOL     WINAPI SetupAddToSourceListW(DWORD, PCWSTR);
#define                     SetupAddToSourceList WINELIB_NAME_AW(SetupAddToSourceList)
WINSETUPAPI BOOL     WINAPI SetupAdjustDiskSpaceListA(HDSKSPC, LPCSTR, LONGLONG, PVOID, UINT);
WINSETUPAPI BOOL     WINAPI SetupAdjustDiskSpaceListW(HDSKSPC, LPCWSTR, LONGLONG, PVOID, UINT);
#define                     SetupAdjustDiskSpaceList WINELIB_NAME_AW(SetupAdjustDiskSpaceList)
WINSETUPAPI BOOL     WINAPI SetupCancelTemporarySourceList(void);
WINSETUPAPI BOOL     WINAPI SetupConfigureWmiFromInfSectionA(HINF, PCSTR, DWORD);
WINSETUPAPI BOOL     WINAPI SetupConfigureWmiFromInfSectionW(HINF, PCWSTR, DWORD);
#define                     SetupConfigureWmiFromInfSection WINELIB_NAME_AW(SetupConfigureWmiFromInfSection)
WINSETUPAPI UINT     WINAPI SetupBackupErrorA(HWND, PCSTR, PCSTR, PCSTR, UINT, DWORD);
WINSETUPAPI UINT     WINAPI SetupBackupErrorW(HWND, PCWSTR, PCWSTR, PCWSTR, UINT, DWORD);
#define                     SetupBackupError WINELIB_NAME_AW(SetupBackupError)
WINSETUPAPI BOOL     WINAPI SetupCloseFileQueue( HSPFILEQ );
WINSETUPAPI void     WINAPI SetupCloseInfFile( HINF hinf );
WINSETUPAPI void     WINAPI SetupCloseLog(void);
WINSETUPAPI BOOL     WINAPI SetupCommitFileQueueA( HWND, HSPFILEQ, PSP_FILE_CALLBACK_A, PVOID );
WINSETUPAPI BOOL     WINAPI SetupCommitFileQueueW( HWND, HSPFILEQ, PSP_FILE_CALLBACK_W, PVOID );
#define                     SetupCommitFileQueue WINELIB_NAME_AW(SetupCommitFileQueue)
WINSETUPAPI UINT     WINAPI SetupCopyErrorA( HWND, PCSTR, PCSTR, PCSTR, PCSTR, PCSTR, UINT, DWORD, PSTR, DWORD, PDWORD );
WINSETUPAPI UINT     WINAPI SetupCopyErrorW( HWND, PCWSTR, PCWSTR, PCWSTR, PCWSTR, PCWSTR, UINT, DWORD, PWSTR, DWORD, PDWORD );
#define                     SetupCopyError WINELIB_NAME_AW(SetupCopyError)
WINSETUPAPI BOOL     WINAPI SetupCopyOEMInfA( PCSTR, PCSTR, DWORD, DWORD, PSTR, DWORD, PDWORD, PSTR * );
WINSETUPAPI BOOL     WINAPI SetupCopyOEMInfW( PCWSTR, PCWSTR, DWORD, DWORD, PWSTR, DWORD, PDWORD, PWSTR * );
#define                     SetupCopyOEMInf WINELIB_NAME_AW(SetupCopyOEMInf)
WINSETUPAPI HDSKSPC  WINAPI SetupCreateDiskSpaceListA(PVOID, DWORD, UINT);
WINSETUPAPI HDSKSPC  WINAPI SetupCreateDiskSpaceListW(PVOID, DWORD, UINT);
#define                     SetupCreateDiskSpaceList WINELIB_NAME_AW(SetupCreateDiskSpaceList)
WINSETUPAPI DWORD    WINAPI SetupDecompressOrCopyFileA( PCSTR, PCSTR, PUINT );
WINSETUPAPI DWORD    WINAPI SetupDecompressOrCopyFileW( PCWSTR, PCWSTR, PUINT );
#define                     SetupDecompressOrCopyFile WINELIB_NAME_AW(SetupDecompressOrCopyFile)
WINSETUPAPI UINT     WINAPI SetupDefaultQueueCallbackA( PVOID, UINT, UINT_PTR, UINT_PTR );
WINSETUPAPI UINT     WINAPI SetupDefaultQueueCallbackW( PVOID, UINT, UINT_PTR, UINT_PTR );
#define                     SetupDefaultQueueCallback WINELIB_NAME_AW(SetupDefaultQueueCallback)
WINSETUPAPI UINT     WINAPI SetupDeleteErrorA( HWND, PCSTR, PCSTR, UINT, DWORD );
WINSETUPAPI UINT     WINAPI SetupDeleteErrorW( HWND, PCWSTR, PCWSTR, UINT, DWORD );
#define                     SetupDeleteError WINELIB_NAME_AW(SetupDeleteError)
WINSETUPAPI BOOL     WINAPI SetupDestroyDiskSpaceList(HDSKSPC);
WINSETUPAPI BOOL     WINAPI SetupDiAskForOEMDisk(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiBuildClassInfoList(DWORD, LPGUID, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupDiBuildClassInfoListExA(DWORD, LPGUID, DWORD, PDWORD, PCSTR, PVOID);
WINSETUPAPI BOOL     WINAPI SetupDiBuildClassInfoListExW(DWORD, LPGUID, DWORD, PDWORD, PCWSTR, PVOID);
#define                     SetupDiBuildClassInfoListEx WINELIB_NAME_AW(SetupDiBuildClassInfoListEx)
WINSETUPAPI BOOL     WINAPI SetupDiBuildDriverInfoList(HDEVINFO, PSP_DEVINFO_DATA, DWORD);
WINSETUPAPI BOOL     WINAPI SetupDiCallClassInstaller(DI_FUNCTION, HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiCancelDriverInfoSearch(HDEVINFO);
WINSETUPAPI BOOL     WINAPI SetupDiChangeState(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiClassGuidsFromNameA(LPCSTR, LPGUID, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupDiClassGuidsFromNameW(LPCWSTR, LPGUID, DWORD, PDWORD);
#define                     SetupDiClassGuidsFromName WINELIB_NAME_AW(SetupDiClassGuidsFromName)
WINSETUPAPI BOOL     WINAPI SetupDiClassGuidsFromNameExA(LPCSTR, LPGUID, DWORD, PDWORD, LPCSTR, PVOID);
WINSETUPAPI BOOL     WINAPI SetupDiClassGuidsFromNameExW(LPCWSTR, LPGUID, DWORD, PDWORD, LPCWSTR, PVOID);
#define                     SetupDiClassGuidsFromNameEx WINELIB_NAME_AW(SetupDiClassGuidsFromNameEx)
WINSETUPAPI BOOL     WINAPI SetupDiClassNameFromGuidA(const GUID*, PSTR, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupDiClassNameFromGuidW(const GUID*, PWSTR, DWORD, PDWORD);
#define                     SetupDiClassNameFromGuid WINELIB_NAME_AW(SetupDiClassNameFromGuid)
WINSETUPAPI BOOL     WINAPI SetupDiClassNameFromGuidExA(const GUID*, PSTR, DWORD, PDWORD, PCSTR, PVOID);
WINSETUPAPI BOOL     WINAPI SetupDiClassNameFromGuidExW(const GUID*, PWSTR, DWORD, PDWORD, PCWSTR, PVOID);
#define                     SetupDiClassNameFromGuidEx WINELIB_NAME_AW(SetupDiClassNameFromGuidEx)
WINSETUPAPI HDEVINFO WINAPI SetupDiCreateDeviceInfoList(const GUID *, HWND);
WINSETUPAPI HDEVINFO WINAPI SetupDiCreateDeviceInfoListExA(const GUID *, HWND, PCSTR, PVOID);
WINSETUPAPI HDEVINFO WINAPI SetupDiCreateDeviceInfoListExW(const GUID *, HWND, PCWSTR, PVOID);
#define                     SetupDiCreateDeviceInfoListEx WINELIB_NAME_AW(SetupDiCreateDeviceInfoListEx)
WINSETUPAPI BOOL     WINAPI SetupDiCreateDeviceInfoA(HDEVINFO, PCSTR, const GUID*, PCSTR, HWND, DWORD,PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiCreateDeviceInfoW(HDEVINFO, PCWSTR, const GUID*, PCWSTR, HWND, DWORD,PSP_DEVINFO_DATA);
#define                     SetupDiCreateDeviceInfo WINELIB_NAME_AW(SetupDiCreateDeviceInfo)
WINSETUPAPI BOOL     WINAPI SetupDiCreateDeviceInterfaceA(HDEVINFO, PSP_DEVINFO_DATA, const GUID *, PCSTR, DWORD, PSP_DEVICE_INTERFACE_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiCreateDeviceInterfaceW(HDEVINFO, PSP_DEVINFO_DATA, const GUID *, PCWSTR, DWORD, PSP_DEVICE_INTERFACE_DATA);
#define                     SetupDiCreateDeviceInterface WINELIB_NAME_AW(SetupDiCreateDeviceInterface)
WINSETUPAPI HKEY     WINAPI SetupDiCreateDeviceInterfaceRegKeyA(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, DWORD, REGSAM, HINF, PCSTR);
WINSETUPAPI HKEY     WINAPI SetupDiCreateDeviceInterfaceRegKeyW(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, DWORD, REGSAM, HINF, PCWSTR);
#define                     SetupDiCreateDeviceInterfaceRegKey WINELIB_NAME_AW(SetupDiCreateDeviceInterfaceRegKey)
WINSETUPAPI HKEY     WINAPI SetupDiCreateDevRegKeyA(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, HINF, PCSTR);
WINSETUPAPI HKEY     WINAPI SetupDiCreateDevRegKeyW(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, HINF, PCWSTR);
#define                     SetupDiCreateDevRegKey WINELIB_NAME_AW(SetupDiCreateDevRegKey)
WINSETUPAPI BOOL     WINAPI SetupDiDeleteDeviceInfo(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiDeleteDeviceInterfaceData(HDEVINFO, PSP_DEVICE_INTERFACE_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiDeleteDeviceInterfaceRegKey(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, DWORD);
WINSETUPAPI BOOL     WINAPI SetupDiDeleteDevRegKey(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD);
WINSETUPAPI BOOL     WINAPI SetupDiDestroyClassImageList(PSP_CLASSIMAGELIST_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiDestroyDeviceInfoList(HDEVINFO);
WINSETUPAPI BOOL     WINAPI SetupDiDestroyDriverInfoList(HDEVINFO, PSP_DEVINFO_DATA, DWORD);
WINSETUPAPI INT      WINAPI SetupDiDrawMiniIcon(HDC, RECT, INT, DWORD);
WINSETUPAPI BOOL     WINAPI SetupDiEnumDeviceInfo(HDEVINFO, DWORD, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiEnumDeviceInterfaces(HDEVINFO, PSP_DEVINFO_DATA, const GUID *, DWORD, PSP_DEVICE_INTERFACE_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiEnumDriverInfoA(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, PSP_DRVINFO_DATA_A);
WINSETUPAPI BOOL     WINAPI SetupDiEnumDriverInfoW(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, PSP_DRVINFO_DATA_W);
#define                     SetupDiEnumDriverInfo WINELIB_NAME_AW(SetupDiEnumDriverInfo)
WINSETUPAPI BOOL     WINAPI SetupDiGetActualModelsSectionA(PINFCONTEXT, PSP_ALTPLATFORM_INFO, PSTR, DWORD, PDWORD, PVOID);
WINSETUPAPI BOOL     WINAPI SetupDiGetActualModelsSectionW(PINFCONTEXT, PSP_ALTPLATFORM_INFO, PWSTR, DWORD, PDWORD, PVOID);
#define                     SetupDiGetActualModelsSection WINELIB_NAME_AW(SetupDiGetActualModelsSection)
WINSETUPAPI BOOL     WINAPI SetupDiGetActualSectionToInstallA(HINF, PCSTR, PSTR, DWORD, PDWORD, PSTR *);
WINSETUPAPI BOOL     WINAPI SetupDiGetActualSectionToInstallW(HINF, PCWSTR, PWSTR, DWORD, PDWORD, PWSTR *);
#define                     SetupDiGetActualSectionToInstall WINELIB_NAME_AW(SetupDiGetActualSectionToInstall)
WINSETUPAPI BOOL     WINAPI SetupDiGetActualSectionToInstallExA(HINF, PCSTR, PSP_ALTPLATFORM_INFO, PSTR, DWORD, PDWORD, PSTR *, PVOID);
WINSETUPAPI BOOL     WINAPI SetupDiGetActualSectionToInstallExW(HINF, PCWSTR, PSP_ALTPLATFORM_INFO, PWSTR, DWORD, PDWORD, PWSTR *, PVOID);
#define                     SetupDiGetActualSectionToInstallEx WINELIB_NAME_AW(SetupDiGetActualSectionToInstallEx)
WINSETUPAPI BOOL     WINAPI SetupDiGetClassBitmapIndex(const GUID *, PINT);
WINSETUPAPI BOOL     WINAPI SetupDiGetClassDescriptionA(const GUID*, PSTR, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupDiGetClassDescriptionW(const GUID*, PWSTR, DWORD, PDWORD);
#define                     SetupDiGetClassDescription WINELIB_NAME_AW(SetupDiGetClassDescription)
WINSETUPAPI BOOL     WINAPI SetupDiGetClassDescriptionExA(const GUID*, PSTR, DWORD, PDWORD, PCSTR, PVOID);
WINSETUPAPI BOOL     WINAPI SetupDiGetClassDescriptionExW(const GUID*, PWSTR, DWORD, PDWORD, PCWSTR, PVOID);
#define                     SetupDiGetClassDescriptionEx WINELIB_NAME_AW(SetupDiGetClassDescriptionEx)
WINSETUPAPI BOOL     WINAPI SetupDiGetClassDevPropertySheetsA(HDEVINFO, PSP_DEVINFO_DATA, LPPROPSHEETHEADERA, DWORD, PDWORD, DWORD);
WINSETUPAPI BOOL     WINAPI SetupDiGetClassDevPropertySheetsW(HDEVINFO, PSP_DEVINFO_DATA, LPPROPSHEETHEADERW, DWORD, PDWORD, DWORD);
#define                     SetupDiGetClassDevPropertySheets WINELIB_NAME_AW(SetupDiGetClassDevPropertySheets)
WINSETUPAPI HDEVINFO WINAPI SetupDiGetClassDevsA(const GUID *,LPCSTR,HWND,DWORD);
WINSETUPAPI HDEVINFO WINAPI SetupDiGetClassDevsW(const GUID *,LPCWSTR,HWND,DWORD);
#define                     SetupDiGetClassDevs WINELIB_NAME_AW(SetupDiGetClassDevs)
WINSETUPAPI HDEVINFO WINAPI SetupDiGetClassDevsExA(const GUID *, PCSTR, HWND, DWORD, HDEVINFO, PCSTR, PVOID);
WINSETUPAPI HDEVINFO WINAPI SetupDiGetClassDevsExW(const GUID *, PCWSTR, HWND, DWORD, HDEVINFO, PCWSTR, PVOID);
#define                     SetupDiGetClassDevsEx WINELIB_NAME_AW(SetupDiGetClassDevsEx)
WINSETUPAPI BOOL     WINAPI SetupDiGetClassImageIndex(PSP_CLASSIMAGELIST_DATA, const GUID *, PINT);
WINSETUPAPI BOOL     WINAPI SetupDiGetClassImageList(PSP_CLASSIMAGELIST_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiGetClassImageListExA(PSP_CLASSIMAGELIST_DATA, PCSTR, PVOID);
WINSETUPAPI BOOL     WINAPI SetupDiGetClassImageListExW(PSP_CLASSIMAGELIST_DATA, PCWSTR, PVOID);
#define                     SetupDiGetClassImageListEx WINELIB_NAME_AW(SetupDiGetClassImageListEx)
WINSETUPAPI BOOL     WINAPI SetupDiGetClassInstallParamsA(HDEVINFO, PSP_DEVINFO_DATA, PSP_CLASSINSTALL_HEADER, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupDiGetClassInstallParamsW(HDEVINFO, PSP_DEVINFO_DATA, PSP_CLASSINSTALL_HEADER, DWORD, PDWORD);
#define                     SetupDiGetClassInstallParams WINELIB_NAME_AW(SetupDiGetClassInstallParams)
WINSETUPAPI BOOL     WINAPI SetupDiGetClassRegistryPropertyA(const GUID *, DWORD, PDWORD, PBYTE, DWORD, PDWORD, PCSTR, PVOID);
WINSETUPAPI BOOL     WINAPI SetupDiGetClassRegistryPropertyW(const GUID *, DWORD, PDWORD, PBYTE, DWORD, PDWORD, PCWSTR, PVOID);
#define                     SetupDiGetClassRegistryProperty WINELIB_NAME_AW(SetupDiGetClassRegistryProperty)
WINSETUPAPI BOOL     WINAPI SetupDiGetCustomDevicePropertyA(HDEVINFO, PSP_DEVINFO_DATA, PCSTR, DWORD, PDWORD, PBYTE, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupDiGetCustomDevicePropertyW(HDEVINFO, PSP_DEVINFO_DATA, PCWSTR, DWORD, PDWORD, PBYTE, DWORD, PDWORD);
#define                     SetupDiGetCustomDeviceProperty WINELIB_NAME_AW(SetupDiGetCustomDeviceProperty)
WINSETUPAPI BOOL     WINAPI SetupDiGetDeviceInfoListClass(HDEVINFO, LPGUID);
WINSETUPAPI BOOL     WINAPI SetupDiGetDeviceInfoListDetailA(HDEVINFO, PSP_DEVINFO_LIST_DETAIL_DATA_A);
WINSETUPAPI BOOL     WINAPI SetupDiGetDeviceInfoListDetailW(HDEVINFO, PSP_DEVINFO_LIST_DETAIL_DATA_W);
#define                     SetupDiGetDeviceInfoListDetail WINELIB_NAME_AW(SetupDiGetDeviceInfoListDetail)
WINSETUPAPI BOOL     WINAPI SetupDiGetDeviceInstallParamsA(HDEVINFO, PSP_DEVINFO_DATA, PSP_DEVINSTALL_PARAMS_A);
WINSETUPAPI BOOL     WINAPI SetupDiGetDeviceInstallParamsW(HDEVINFO, PSP_DEVINFO_DATA, PSP_DEVINSTALL_PARAMS_W);
#define                     SetupDiGetDeviceInstallParams WINELIB_NAME_AW(SetupDiGetDeviceInstallParams)
WINSETUPAPI BOOL     WINAPI SetupDiGetDeviceInstanceIdA(HDEVINFO, PSP_DEVINFO_DATA, PSTR, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupDiGetDeviceInstanceIdW(HDEVINFO, PSP_DEVINFO_DATA, PWSTR, DWORD, PDWORD);
#define                     SetupDiGetDeviceInstanceId WINELIB_NAME_AW(SetupDiGetDeviceInstanceId)
WINSETUPAPI BOOL     WINAPI SetupDiGetDeviceInterfaceAlias(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, const GUID *, PSP_DEVICE_INTERFACE_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiGetDeviceInterfaceDetailA(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_A, DWORD, PDWORD, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiGetDeviceInterfaceDetailW(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_W, DWORD, PDWORD, PSP_DEVINFO_DATA);
#define                     SetupDiGetDeviceInterfaceDetail WINELIB_NAME_AW(SetupDiGetDeviceInterfaceDetail)
WINSETUPAPI BOOL WINAPI SetupDiGetDevicePropertyKeys(HDEVINFO, PSP_DEVINFO_DATA, DEVPROPKEY *, DWORD, DWORD *, DWORD);
WINSETUPAPI BOOL     WINAPI SetupDiGetDevicePropertyW(HDEVINFO, PSP_DEVINFO_DATA, const DEVPROPKEY *, DEVPROPTYPE *, BYTE *, DWORD, DWORD *, DWORD);
#define                     SetupDiGetDeviceProperty WINELIB_NAME_AW(SetupDiGetDeviceProperty)  /* note: A function doesn't exist */
WINSETUPAPI BOOL     WINAPI SetupDiGetDeviceRegistryPropertyA(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupDiGetDeviceRegistryPropertyW(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);
#define                     SetupDiGetDeviceRegistryProperty WINELIB_NAME_AW(SetupDiGetDeviceRegistryProperty)
WINSETUPAPI BOOL     WINAPI SetupDiGetDriverInfoDetailA(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA_A, PSP_DRVINFO_DETAIL_DATA_A, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupDiGetDriverInfoDetailW(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA_W, PSP_DRVINFO_DETAIL_DATA_W, DWORD, PDWORD);
#define                     SetupDiGetDriverInfoDetail WINELIB_NAME_AW(SetupDiGetDriverInfoDetail)
WINSETUPAPI BOOL     WINAPI SetupDiGetDriverInstallParamsA(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA_A, PSP_DRVINSTALL_PARAMS);
WINSETUPAPI BOOL     WINAPI SetupDiGetDriverInstallParamsW(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA_W, PSP_DRVINSTALL_PARAMS);
#define                     SetupDiGetDriverInstallParams WINELIB_NAME_AW(SetupDiGetDriverInstallParams)
WINSETUPAPI BOOL     WINAPI SetupDiGetHwProfileFriendlyNameA(DWORD, PSTR, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupDiGetHwProfileFriendlyNameW(DWORD, PWSTR, DWORD, PDWORD);
#define                     SetupDiGetHwProfileFriendlyName WINELIB_NAME_AW(SetupDiGetHwProfileFriendlyName)
WINSETUPAPI BOOL     WINAPI SetupDiGetHwProfileFriendlyNameExA(DWORD, PSTR, DWORD, PDWORD, PCSTR, PVOID);
WINSETUPAPI BOOL     WINAPI SetupDiGetHwProfileFriendlyNameExW(DWORD, PWSTR, DWORD, PDWORD, PCWSTR, PVOID);
#define                     SetupDiGetHwProfileFriendlyNameEx WINELIB_NAME_AW(SetupDiGetHwProfileFriendlyNameEx)
WINSETUPAPI BOOL     WINAPI SetupDiGetHwProfileList(PDWORD, DWORD, PDWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupDiGetHwProfileListExA(PDWORD, DWORD, PDWORD, PDWORD, PCSTR, PVOID);
WINSETUPAPI BOOL     WINAPI SetupDiGetHwProfileListExW(PDWORD, DWORD, PDWORD, PDWORD, PCWSTR, PVOID);
#define                     SetupDiGetHwProfileListEx WINELIB_NAME_AW(SetupDiGetHwProfileListEx)
WINSETUPAPI BOOL     WINAPI SetupDiGetINFClassA(PCSTR, LPGUID, PSTR, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupDiGetINFClassW(PCWSTR, LPGUID, PWSTR, DWORD, PDWORD);
#define                     SetupDiGetINFClass WINELIB_NAME_AW(SetupDiGetINFClass)
WINSETUPAPI BOOL     WINAPI SetupDiGetSelectedDevice(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiGetSelectedDriverA(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA_A);
WINSETUPAPI BOOL     WINAPI SetupDiGetSelectedDriverW(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA_W);
#define                     SetupDiGetSelectedDriver WINELIB_NAME_AW(SetupDiGetSelectedDriver)
WINSETUPAPI HPROPSHEETPAGE WINAPI SetupDiGetWizardPage(HDEVINFO, PSP_DEVINFO_DATA, PSP_INSTALLWIZARD_DATA, DWORD, DWORD);
WINSETUPAPI BOOL     WINAPI SetupDiInstallClassA(HWND, PCSTR, DWORD, HSPFILEQ);
WINSETUPAPI BOOL     WINAPI SetupDiInstallClassW(HWND, PCWSTR, DWORD, HSPFILEQ);
#define                     SetupDiInstallClass WINELIB_NAME_AW(SetupDiInstallClass)
WINSETUPAPI BOOL     WINAPI SetupDiInstallClassExA(HWND, PCSTR, DWORD, HSPFILEQ, const GUID *, PVOID, PVOID);
WINSETUPAPI BOOL     WINAPI SetupDiInstallClassExW(HWND, PCWSTR, DWORD, HSPFILEQ, const GUID *, PVOID, PVOID);
#define                     SetupDiInstallClassEx WINELIB_NAME_AW(SetupDiInstallClassEx)
WINSETUPAPI BOOL     WINAPI SetupDiInstallDevice(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiInstallDeviceInterfaces(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiInstallDriverFiles(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiLoadClassIcon(const GUID *, HICON *, PINT);
WINSETUPAPI HKEY     WINAPI SetupDiOpenClassRegKey(const GUID*, REGSAM);
WINSETUPAPI HKEY     WINAPI SetupDiOpenClassRegKeyExA(const GUID*, REGSAM, DWORD, PCSTR, PVOID);
WINSETUPAPI HKEY     WINAPI SetupDiOpenClassRegKeyExW(const GUID*, REGSAM, DWORD, PCWSTR, PVOID);
#define                     SetupDiOpenClassRegKeyEx WINELIB_NAME_AW(SetupDiOpenClassRegKeyEx)
WINSETUPAPI BOOL     WINAPI SetupDiOpenDeviceInfoA(HDEVINFO, PCSTR, HWND, DWORD, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiOpenDeviceInfoW(HDEVINFO, PCWSTR, HWND, DWORD, PSP_DEVINFO_DATA);
#define                     SetupDiOpenDeviceInfo WINELIB_NAME_AW(SetupDiOpenDeviceInfo)
WINSETUPAPI BOOL     WINAPI SetupDiOpenDeviceInterfaceA(HDEVINFO, PCSTR, DWORD, PSP_DEVICE_INTERFACE_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiOpenDeviceInterfaceW(HDEVINFO, PCWSTR, DWORD, PSP_DEVICE_INTERFACE_DATA);
#define                     SetupDiOpenDeviceInterface WINELIB_NAME_AW(SetupDiOpenDeviceInterface)
WINSETUPAPI HKEY     WINAPI SetupDiOpenDeviceInterfaceRegKey(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, DWORD, REGSAM);
WINSETUPAPI HKEY     WINAPI SetupDiOpenDevRegKey(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, REGSAM);
WINSETUPAPI BOOL     WINAPI SetupDiRegisterCoDeviceInstallers(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiRegisterDeviceInfo(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PSP_DETSIG_CMPPROC, PVOID, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiRemoveDevice(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiRemoveDeviceInterface(HDEVINFO, PSP_DEVICE_INTERFACE_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiRestartDevices(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiSelectBestCompatDrv(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiSelectDevice(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiSelectOEMDrv(HWND, HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiSetClassInstallParamsA(HDEVINFO, PSP_DEVINFO_DATA, PSP_CLASSINSTALL_HEADER, DWORD);
WINSETUPAPI BOOL     WINAPI SetupDiSetClassInstallParamsW(HDEVINFO, PSP_DEVINFO_DATA, PSP_CLASSINSTALL_HEADER, DWORD);
#define                     SetupDiSetClassInstallParams WINELIB_NAME_AW(SetupDiSetClassInstallParams)
WINSETUPAPI BOOL     WINAPI SetupDiSetClassRegistryPropertyA(const GUID *, DWORD, const BYTE *, DWORD, PCSTR, PVOID);
WINSETUPAPI BOOL     WINAPI SetupDiSetClassRegistryPropertyW(const GUID *, DWORD, const BYTE *, DWORD, PCWSTR, PVOID);
#define                     SetupDiSetClassRegistryProperty WINELIB_NAME_AW(SetupDiSetClassRegistryProperty)
WINSETUPAPI BOOL     WINAPI SetupDiSetDeviceInterfaceDefault(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, DWORD, PVOID);
WINSETUPAPI BOOL     WINAPI SetupDiSetDeviceInstallParamsA(HDEVINFO, PSP_DEVINFO_DATA, PSP_DEVINSTALL_PARAMS_A);
WINSETUPAPI BOOL     WINAPI SetupDiSetDeviceInstallParamsW(HDEVINFO, PSP_DEVINFO_DATA, PSP_DEVINSTALL_PARAMS_W);
#define                     SetupDiSetDeviceInstallParams WINELIB_NAME_AW(SetupDiSetDeviceInstallParams)
WINSETUPAPI BOOL     WINAPI SetupDiSetDevicePropertyW(HDEVINFO, PSP_DEVINFO_DATA, const DEVPROPKEY *, DEVPROPTYPE, const BYTE *, DWORD, DWORD);
#define                     SetupDiSetDeviceProperty WINELIB_NAME_AW(SetupDiSetDeviceProperty) /* note: A function doesn't exist */
WINSETUPAPI BOOL     WINAPI SetupDiSetDeviceRegistryPropertyA(HDEVINFO, PSP_DEVINFO_DATA, DWORD, const BYTE *, DWORD);
WINSETUPAPI BOOL     WINAPI SetupDiSetDeviceRegistryPropertyW(HDEVINFO, PSP_DEVINFO_DATA, DWORD, const BYTE *, DWORD);
#define                     SetupDiSetDeviceRegistryProperty WINELIB_NAME_AW(SetupDiSetDeviceRegistryProperty)
WINSETUPAPI BOOL     WINAPI SetupDiSetDriverInstallParamsA(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA_A, PSP_DRVINSTALL_PARAMS);
WINSETUPAPI BOOL     WINAPI SetupDiSetDriverInstallParamsW(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA_W, PSP_DRVINSTALL_PARAMS);
#define                     SetupDiSetDriverInstallParams WINELIB_NAME_AW(SetupDiSetDriverInstallParams)
WINSETUPAPI BOOL     WINAPI SetupDiSetSelectedDevice(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupDiSetSelectedDriverA(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA_A);
WINSETUPAPI BOOL     WINAPI SetupDiSetSelectedDriverW(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA_W);
#define                     SetupDiSetSelectedDriver WINELIB_NAME_AW(SetupDiSetSelectedDriver)
WINSETUPAPI BOOL     WINAPI SetupDiUnremoveDevice(HDEVINFO, PSP_DEVINFO_DATA);
WINSETUPAPI HDSKSPC  WINAPI SetupDuplicateDiskSpaceListA(HDSKSPC, PVOID, DWORD, UINT);
WINSETUPAPI HDSKSPC  WINAPI SetupDuplicateDiskSpaceListW(HDSKSPC, PVOID, DWORD, UINT);
#define                     SetupDuplicateDiskSpaceList WINELIB_NAME_AW(SetupDuplicateDiskSpaceList)
WINSETUPAPI BOOL     WINAPI SetupEnumInfSectionsA(HINF, UINT, PSTR, DWORD, DWORD *);
WINSETUPAPI BOOL     WINAPI SetupEnumInfSectionsW(HINF, UINT, PWSTR, DWORD, DWORD *);
#define                     SetupEnumInfSections WINELIB_NAME_AW(SetupEnumInfSections)
WINSETUPAPI BOOL     WINAPI SetupFindFirstLineA( HINF hinf, PCSTR section, PCSTR key, INFCONTEXT *context );
WINSETUPAPI BOOL     WINAPI SetupFindFirstLineW( HINF hinf, PCWSTR section, PCWSTR key, INFCONTEXT *context );
#define                     SetupFindFirstLine WINELIB_NAME_AW(SetupFindFirstLine)
WINSETUPAPI BOOL     WINAPI SetupFindNextLine( PINFCONTEXT context_in, PINFCONTEXT context_out );
WINSETUPAPI BOOL     WINAPI SetupFindNextMatchLineA( PINFCONTEXT context_in, PCSTR key, PINFCONTEXT context_out );
WINSETUPAPI BOOL     WINAPI SetupFindNextMatchLineW( PINFCONTEXT context_in, PCWSTR key, PINFCONTEXT context_out );
#define                     SetupFindNextMatchLine WINELIB_NAME_AW(SetupFindNextMatchLine)
WINSETUPAPI BOOL     WINAPI SetupFreeSourceListA(PCSTR **, UINT);
WINSETUPAPI BOOL     WINAPI SetupFreeSourceListW(PCWSTR **, UINT);
#define                     SetupFreeSourceList WINELIB_NAME_AW(SetupFreeSourceList)
WINSETUPAPI BOOL     WINAPI SetupGetBackupInformationA(HSPFILEQ, PSP_BACKUP_QUEUE_PARAMS_A BackupParams);
WINSETUPAPI BOOL     WINAPI SetupGetBackupInformationW(HSPFILEQ, PSP_BACKUP_QUEUE_PARAMS_W BackupParams);
#define                     SetupGetBackupInformation WINELIB_NAME_AW(SetupGetBackupInformation)
WINSETUPAPI BOOL     WINAPI SetupGetBinaryField( PINFCONTEXT context, DWORD index, BYTE *buffer, DWORD size, LPDWORD required );
WINSETUPAPI DWORD    WINAPI SetupGetFieldCount( PINFCONTEXT context );
WINSETUPAPI DWORD    WINAPI SetupGetFileCompressionInfoA(PCSTR, PSTR *, PDWORD, PDWORD, PUINT);
WINSETUPAPI DWORD    WINAPI SetupGetFileCompressionInfoW(PCWSTR, PWSTR *, PDWORD, PDWORD, PUINT);
#define                     SetupGetFileCompressionInfo WINELIB_NAME_AW(SetupGetFileCompressionInfo)
WINSETUPAPI BOOL     WINAPI SetupGetFileCompressionInfoExA(PCSTR, PSTR, DWORD, PDWORD, PDWORD, PDWORD, PUINT);
WINSETUPAPI BOOL     WINAPI SetupGetFileCompressionInfoExW(PCWSTR, PWSTR, DWORD, PDWORD, PDWORD, PDWORD, PUINT);
#define                     SetupGetFileCompressionInfoEx WINELIB_NAME_AW(SetupGetFileCompressionInfoEx)
WINSETUPAPI BOOL     WINAPI SetupGetFileQueueCount( HSPFILEQ, UINT, PUINT );
WINSETUPAPI BOOL     WINAPI SetupGetFileQueueFlags( HSPFILEQ, PDWORD );
WINSETUPAPI BOOL     WINAPI SetupGetInfFileListA(PCSTR, DWORD, PSTR, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupGetInfFileListW(PCWSTR, DWORD, PWSTR, DWORD, PDWORD);
#define                     SetupGetInfFileList WINELIB_NAME_AW(SetupGetFileList)
WINSETUPAPI BOOL     WINAPI SetupGetInfInformationA( LPCVOID, DWORD, PSP_INF_INFORMATION, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupGetInfInformationW( LPCVOID, DWORD, PSP_INF_INFORMATION, DWORD, PDWORD);
#define                     SetupGetInfInformation WINELIB_NAME_AW(SetupGetInfInformation)
WINSETUPAPI BOOL     WINAPI SetupGetIntField( PINFCONTEXT context, DWORD index, PINT result );
WINSETUPAPI BOOL     WINAPI SetupGetLineByIndexA( HINF, PCSTR, DWORD, INFCONTEXT * );
WINSETUPAPI BOOL     WINAPI SetupGetLineByIndexW( HINF, PCWSTR, DWORD, INFCONTEXT * );
#define                     SetupGetLineByIndex WINELIB_NAME_AW(SetupGetLineByIndex)
WINSETUPAPI LONG     WINAPI SetupGetLineCountA( HINF hinf, PCSTR section );
WINSETUPAPI LONG     WINAPI SetupGetLineCountW( HINF hinf, PCWSTR section );
#define                     SetupGetLineCount WINELIB_NAME_AW(SetupGetLineCount)
WINSETUPAPI BOOL     WINAPI SetupGetLineTextA( PINFCONTEXT context, HINF hinf, PCSTR section_name,PCSTR key_name, PSTR buffer, DWORD size, PDWORD required );
WINSETUPAPI BOOL     WINAPI SetupGetLineTextW( PINFCONTEXT context, HINF hinf, PCWSTR section_name, PCWSTR key_name, PWSTR buffer, DWORD size, PDWORD required );
#define                     SetupGetLineText WINELIB_NAME_AW(SetupGetLineText)
WINSETUPAPI BOOL     WINAPI SetupGetMultiSzFieldA( PINFCONTEXT context, DWORD index, PSTR buffer, DWORD size, LPDWORD required );
WINSETUPAPI BOOL     WINAPI SetupGetMultiSzFieldW( PINFCONTEXT context, DWORD index, PWSTR buffer, DWORD size, LPDWORD required );
#define                     SetupGetMultiSzField WINELIB_NAME_AW(SetupGetMultiSzField)
WINSETUPAPI BOOL     WINAPI SetupGetNonInteractiveMode(void);
WINSETUPAPI BOOL     WINAPI SetupGetSourceFileLocationA( HINF hinf, PINFCONTEXT context, PCSTR filename, PUINT source_id, PSTR buffer, DWORD buffer_size, PDWORD required_size );
WINSETUPAPI BOOL     WINAPI SetupGetSourceFileLocationW( HINF hinf, PINFCONTEXT context, PCWSTR filename, PUINT source_id, PWSTR buffer, DWORD buffer_size, PDWORD required_size );
#define                     SetupGetSourceFileLocation WINELIB_NAME_AW(SetupGetSourceFileLocation)
WINSETUPAPI BOOL     WINAPI SetupGetSourceFileSizeA(HINF, PINFCONTEXT, PCSTR, PCSTR, PDWORD, UINT);
WINSETUPAPI BOOL     WINAPI SetupGetSourceFileSizeW(HINF, PINFCONTEXT, PCWSTR, PCWSTR, PDWORD, UINT);
#define                     SetupGetSourceFileSize WINELIB_NAME_AW(SetupGetSourceFileSize)
WINSETUPAPI BOOL     WINAPI SetupGetSourceInfoA( HINF hinf, UINT source_id, UINT info, PSTR buffer, DWORD buffer_size, LPDWORD required_size );
WINSETUPAPI BOOL     WINAPI SetupGetSourceInfoW( HINF hinf, UINT source_id, UINT info, PWSTR buffer, DWORD buffer_size, LPDWORD required_size );
#define                     SetupGetSourceInfo WINELIB_NAME_AW(SetupGetSourceInfo)
WINSETUPAPI BOOL     WINAPI SetupGetStringFieldA( PINFCONTEXT context, DWORD index, PSTR buffer, DWORD size, PDWORD required );
WINSETUPAPI BOOL     WINAPI SetupGetStringFieldW( PINFCONTEXT context, DWORD index, PWSTR buffer, DWORD size, PDWORD required );
#define                     SetupGetStringField WINELIB_NAME_AW(SetupGetStringField)
WINSETUPAPI BOOL     WINAPI SetupGetTargetPathA( HINF hinf, PINFCONTEXT context, PCSTR section, PSTR buffer, DWORD buffer_size, PDWORD required_size );
WINSETUPAPI BOOL     WINAPI SetupGetTargetPathW( HINF hinf, PINFCONTEXT context, PCWSTR section, PWSTR buffer, DWORD buffer_size, PDWORD required_size );
#define                     SetupGetTargetPath WINELIB_NAME_AW(SetupGetTargetPath)
WINSETUPAPI PVOID    WINAPI SetupInitDefaultQueueCallback( HWND );
WINSETUPAPI PVOID    WINAPI SetupInitDefaultQueueCallbackEx( HWND, HWND, UINT, DWORD, PVOID );
WINSETUPAPI HSPFILELOG WINAPI SetupInitializeFileLogA(PCSTR, DWORD);
WINSETUPAPI HSPFILELOG WINAPI SetupInitializeFileLogW(PCWSTR, DWORD);
#define                       SetupInitializeFileLog WINELIB_NAME_AW(SetupInitializeFileLog)
WINSETUPAPI BOOL     WINAPI SetupInstallFileA(HINF, PINFCONTEXT, PCSTR, PCSTR, PCSTR, DWORD, PSP_FILE_CALLBACK_A, PVOID);
WINSETUPAPI BOOL     WINAPI SetupInstallFileW(HINF, PINFCONTEXT, PCWSTR, PCWSTR, PCWSTR, DWORD, PSP_FILE_CALLBACK_W, PVOID);
#define                     SetupInstallFile WINELIB_NAME_AW(SetupInstallFile)
WINSETUPAPI BOOL     WINAPI SetupInstallFileExA(HINF, PINFCONTEXT, PCSTR, PCSTR, PCSTR, DWORD, PSP_FILE_CALLBACK_A, PVOID, PBOOL);
WINSETUPAPI BOOL     WINAPI SetupInstallFileExW(HINF, PINFCONTEXT, PCWSTR, PCWSTR, PCWSTR, DWORD, PSP_FILE_CALLBACK_W, PVOID, PBOOL);
#define                     SetupInstallFileEx WINELIB_NAME_AW(SetupInstallFileEx)
WINSETUPAPI BOOL     WINAPI SetupInstallFilesFromInfSectionA( HINF, HINF, HSPFILEQ, PCSTR, PCSTR, UINT );
WINSETUPAPI BOOL     WINAPI SetupInstallFilesFromInfSectionW( HINF, HINF, HSPFILEQ, PCWSTR, PCWSTR, UINT );
#define                     SetupInstallFilesFromInfSection WINELIB_NAME_AW(SetupInstallFilesFromInfSection)
WINSETUPAPI BOOL     WINAPI SetupInstallFromInfSectionA(HWND,HINF,PCSTR,UINT,HKEY,PCSTR,UINT,PSP_FILE_CALLBACK_A,PVOID,HDEVINFO,PSP_DEVINFO_DATA);
WINSETUPAPI BOOL     WINAPI SetupInstallFromInfSectionW(HWND,HINF,PCWSTR,UINT,HKEY,PCWSTR,UINT,PSP_FILE_CALLBACK_W,PVOID,HDEVINFO,PSP_DEVINFO_DATA);
#define                     SetupInstallFromInfSection WINELIB_NAME_AW(SetupInstallFromInfSection)
WINSETUPAPI BOOL     WINAPI SetupInstallServicesFromInfSectionA(HINF, PCSTR, DWORD);
WINSETUPAPI BOOL     WINAPI SetupInstallServicesFromInfSectionW(HINF, PCWSTR, DWORD);
#define                     SetupInstallServicesFromInfSection WINELIB_NAME_AW(SetupInstallServicesFromInfSection)
WINSETUPAPI BOOL     WINAPI SetupInstallServicesFromInfSectionExA(HINF, PCSTR, DWORD, HDEVINFO, PSP_DEVINFO_DATA, PVOID, PVOID);
WINSETUPAPI BOOL     WINAPI SetupInstallServicesFromInfSectionExW(HINF, PCWSTR, DWORD, HDEVINFO, PSP_DEVINFO_DATA, PVOID, PVOID);
#define                     SetupInstallServicesFromInfSectionEx WINELIB_NAME_AW(SetupInstallServicesFromInfSectionEx)
WINSETUPAPI BOOL     WINAPI SetupIterateCabinetA(PCSTR, DWORD, PSP_FILE_CALLBACK_A, PVOID);
WINSETUPAPI BOOL     WINAPI SetupIterateCabinetW(PCWSTR, DWORD, PSP_FILE_CALLBACK_W, PVOID);
#define                     SetupIterateCabinet WINELIB_NAME_AW(SetupIterateCabinet)
WINSETUPAPI BOOL     WINAPI SetupLogErrorA(LPCSTR,LogSeverity);
WINSETUPAPI BOOL     WINAPI SetupLogErrorW(LPCWSTR,LogSeverity);
#define                     SetupLogError WINELIB_NAME_AW(SetupLogError)
WINSETUPAPI BOOL     WINAPI SetupLogFileA(HSPFILELOG, PCSTR, PCSTR, PCSTR, DWORD, PCSTR, PCSTR, PCSTR, DWORD);
WINSETUPAPI BOOL     WINAPI SetupLogFileW(HSPFILELOG, PCWSTR, PCWSTR, PCWSTR, DWORD, PCWSTR, PCWSTR, PCWSTR, DWORD);
#define                     SetupLogFile WINELIB_NAME_AW(SetupLogFile)
WINSETUPAPI BOOL     WINAPI SetupOpenAppendInfFileA( PCSTR, HINF, UINT * );
WINSETUPAPI BOOL     WINAPI SetupOpenAppendInfFileW( PCWSTR, HINF, UINT * );
#define                     SetupOpenAppendInfFile WINELIB_NAME_AW(SetupOpenAppendInfFile)
WINSETUPAPI HSPFILEQ WINAPI SetupOpenFileQueue(void);
WINSETUPAPI HINF     WINAPI SetupOpenInfFileA( PCSTR name, PCSTR pszclass, DWORD style, UINT *error );
WINSETUPAPI HINF     WINAPI SetupOpenInfFileW( PCWSTR name, PCWSTR pszclass, DWORD style, UINT *error );
#define                     SetupOpenInfFile WINELIB_NAME_AW(SetupOpenInfFile)
WINSETUPAPI BOOL     WINAPI SetupOpenLog(BOOL);
WINSETUPAPI HINF     WINAPI SetupOpenMasterInf( VOID );
WINSETUPAPI BOOL     WINAPI SetupPrepareQueueForRestoreA(HSPFILEQ, PCSTR, DWORD);
WINSETUPAPI BOOL     WINAPI SetupPrepareQueueForRestoreW(HSPFILEQ, PCWSTR, DWORD);
#define                     SetupPrepareQueueForRestore WINELIB_NAME_AW(SetupPrepareQueueForRestore)
WINSETUPAPI UINT     WINAPI SetupPromptForDiskA(HWND, PCSTR, PCSTR, PCSTR, PCSTR, PCSTR, DWORD, PSTR, DWORD, PDWORD);
WINSETUPAPI UINT     WINAPI SetupPromptForDiskW(HWND, PCWSTR, PCWSTR, PCWSTR, PCWSTR, PCWSTR, DWORD, PWSTR, DWORD, PDWORD);
#define                     SetupPromptForDisk WINELIB_NAME_AW(SetupPromptForDisk)
WINSETUPAPI INT      WINAPI SetupPromptReboot( HSPFILEQ, HWND, BOOL);
WINSETUPAPI BOOL     WINAPI SetupQueryDrivesInDiskSpaceListA(HDSKSPC, PSTR, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupQueryDrivesInDiskSpaceListW(HDSKSPC, PWSTR, DWORD, PDWORD);
#define                     SetupQueryDrivesInDiskSpaceList WINELIB_NAME_AW(SetupQueryDrivesInDiskSpaceList)
WINSETUPAPI BOOL     WINAPI SetupQueryFileLogA(HSPFILELOG, PCSTR, PCSTR, SetupFileLogInfo, PSTR, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupQueryFileLogW(HSPFILELOG, PCWSTR, PCWSTR, SetupFileLogInfo, PWSTR, DWORD, PDWORD);
#define                     SetupQueryFileLog WINELIB_NAME_AW(SetupQueryFileLog)
WINSETUPAPI BOOL     WINAPI SetupQueryInfFileInformationA(PSP_INF_INFORMATION, UINT, PSTR, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupQueryInfFileInformationW(PSP_INF_INFORMATION, UINT, PWSTR, DWORD, PDWORD);
#define                     SetupQueryInfFileInformation WINELIB_NAME_AW(SetupQueryInFileInformation)
WINSETUPAPI BOOL     WINAPI SetupQueryInfOriginalFileInformationA(PSP_INF_INFORMATION, UINT, PSP_ALTPLATFORM_INFO, PSP_ORIGINAL_FILE_INFO_A);
WINSETUPAPI BOOL     WINAPI SetupQueryInfOriginalFileInformationW(PSP_INF_INFORMATION, UINT, PSP_ALTPLATFORM_INFO, PSP_ORIGINAL_FILE_INFO_W);
#define                     SetupQueryInfOriginalFileInformation WINELIB_NAME_AW(SetupQueryInfOriginalFileInformation)
WINSETUPAPI BOOL     WINAPI SetupQueryInfVersionInformationA(PSP_INF_INFORMATION, UINT, PCSTR, PSTR, DWORD, PDWORD);
WINSETUPAPI BOOL     WINAPI SetupQueryInfVersionInformationW(PSP_INF_INFORMATION, UINT, PCWSTR, PWSTR, DWORD, PDWORD);
#define                     SetupQueryInfVersionInformation WINELIB_NAME_AW(SetupQueryInfVersionInformation)
WINSETUPAPI BOOL     WINAPI SetupQuerySourceListA(DWORD, PCSTR **, PUINT);
WINSETUPAPI BOOL     WINAPI SetupQuerySourceListW(DWORD, PCWSTR **, PUINT);
#define                     SetupQuerySourceList WINELIB_NAME_AW(SetupQuerySourceList)
WINSETUPAPI BOOL     WINAPI SetupQuerySpaceRequiredOnDriveA(HDSKSPC, PCSTR, LONGLONG *, PVOID, UINT);
WINSETUPAPI BOOL     WINAPI SetupQuerySpaceRequiredOnDriveW(HDSKSPC, PCWSTR, LONGLONG *, PVOID, UINT);
#define                     SetupQuerySpaceRequiredOnDrive WINELIB_NAME_AW(SetupQuerySpaceRequiredOnDrive)
WINSETUPAPI BOOL     WINAPI SetupQueueCopyA(HSPFILEQ,PCSTR,PCSTR,PCSTR,PCSTR,PCSTR,PCSTR,PCSTR,DWORD);
WINSETUPAPI BOOL     WINAPI SetupQueueCopyW(HSPFILEQ,PCWSTR,PCWSTR,PCWSTR,PCWSTR,PCWSTR,PCWSTR,PCWSTR,DWORD);
#define                     SetupQueueCopy WINELIB_NAME_AW(SetupQueueCopy)
WINSETUPAPI BOOL     WINAPI SetupQueueCopyIndirectA( PSP_FILE_COPY_PARAMS_A );
WINSETUPAPI BOOL     WINAPI SetupQueueCopyIndirectW( PSP_FILE_COPY_PARAMS_W );
#define                     SetupQueueCopyIndirect WINELIB_NAME_AW(SetupQueueCopyIndirect)
WINSETUPAPI BOOL     WINAPI SetupQueueCopySectionA( HSPFILEQ, PCSTR, HINF, HINF, PCSTR, DWORD );
WINSETUPAPI BOOL     WINAPI SetupQueueCopySectionW( HSPFILEQ, PCWSTR, HINF, HINF, PCWSTR, DWORD );
#define                     SetupQueueCopySection WINELIB_NAME_AW(SetupQueueCopySection)
WINSETUPAPI BOOL     WINAPI SetupQueueDefaultCopyA( HSPFILEQ, HINF, PCSTR, PCSTR, PCSTR, DWORD );
WINSETUPAPI BOOL     WINAPI SetupQueueDefaultCopyW( HSPFILEQ, HINF, PCWSTR, PCWSTR, PCWSTR, DWORD );
#define                     SetupQueueDefaultCopy WINELIB_NAME_AW(SetupQueueDefaultCopy)
WINSETUPAPI BOOL     WINAPI SetupQueueDeleteA( HSPFILEQ, PCSTR, PCSTR );
WINSETUPAPI BOOL     WINAPI SetupQueueDeleteW( HSPFILEQ, PCWSTR, PCWSTR );
#define                     SetupQueueDelete WINELIB_NAME_AW(SetupQueueDelete)
WINSETUPAPI BOOL     WINAPI SetupQueueDeleteSectionA( HSPFILEQ, HINF, HINF, PCSTR );
WINSETUPAPI BOOL     WINAPI SetupQueueDeleteSectionW( HSPFILEQ, HINF, HINF, PCWSTR );
#define                     SetupQueueDeleteSection WINELIB_NAME_AW(SetupQueueDeleteSection)
WINSETUPAPI BOOL     WINAPI SetupQueueRenameA( HSPFILEQ, PCSTR, PCSTR, PCSTR, PCSTR );
WINSETUPAPI BOOL     WINAPI SetupQueueRenameW( HSPFILEQ, PCWSTR, PCWSTR, PCWSTR, PCWSTR );
#define                     SetupQueueRename WINELIB_NAME_AW(SetupQueueRename)
WINSETUPAPI BOOL     WINAPI SetupQueueRenameSectionA( HSPFILEQ, HINF, HINF, PCSTR );
WINSETUPAPI BOOL     WINAPI SetupQueueRenameSectionW( HSPFILEQ, HINF, HINF, PCWSTR );
#define                     SetupQueueRenameSection WINELIB_NAME_AW(SetupQueueRenameSection)
WINSETUPAPI BOOL     WINAPI SetupRemoveFileLogEntryA(HSPFILELOG, PCSTR, PCSTR);
WINSETUPAPI BOOL     WINAPI SetupRemoveFileLogEntryW(HSPFILELOG, PCWSTR, PCWSTR);
#define                     SetupRemoveFileLogEntry WINELIB_NAME_AW(SetupRemoveFileLogEntry)
WINSETUPAPI BOOL     WINAPI SetupRemoveFromDiskSpaceListA(HDSKSPC, PCSTR, UINT, PVOID, UINT);
WINSETUPAPI BOOL     WINAPI SetupRemoveFromDiskSpaceListW(HDSKSPC, PCWSTR, UINT, PVOID, UINT);
#define                     SetupRemoveFromDiskSpaceList WINELIB_NAME_AW(SetupRemoveFromDiskSpaceList)
WINSETUPAPI BOOL     WINAPI SetupRemoveInstallSectionFromDiskSpaceListA(HDSKSPC, HINF, HINF, PCSTR, PVOID, UINT);
WINSETUPAPI BOOL     WINAPI SetupRemoveInstallSectionFromDiskSpaceListW(HDSKSPC, HINF, HINF, PCWSTR, PVOID, UINT);
#define                     SetupRemoveInstallSectionFromDiskSpaceList WINELIB_NAME_AW(SetupRemoveInstallSectionFromDiskSpaceList)
WINSETUPAPI BOOL     WINAPI SetupRemoveSectionFromDiskSpaceListA(HDSKSPC, HINF, HINF, PCSTR, UINT, PVOID, UINT);
WINSETUPAPI BOOL     WINAPI SetupRemoveSectionFromDiskSpaceListW(HDSKSPC, HINF, HINF, PCWSTR, UINT, PVOID, UINT);
#define                     SetupRemoveSectionFromDiskSpaceList WINELIB_NAME_AW(SetupRemoveSectionFromDiskSpaceList)
WINSETUPAPI BOOL     WINAPI SetupRemoveFromSourceListA(DWORD, PCSTR);
WINSETUPAPI BOOL     WINAPI SetupRemoveFromSourceListW(DWORD, PCWSTR);
#define                     SetupRemoveFromSourceList WINELIB_NAME_AW(SetupRemoveFromSourceList)
WINSETUPAPI UINT     WINAPI SetupRenameErrorA( HWND, PCSTR, PCSTR, PCSTR, UINT, DWORD );
WINSETUPAPI UINT     WINAPI SetupRenameErrorW( HWND, PCWSTR, PCWSTR, PCWSTR, UINT, DWORD );
#define                     SetupRenameError WINELIB_NAME_AW(SetupRenameError)
WINSETUPAPI BOOL     WINAPI SetupScanFileQueueA( HSPFILEQ, DWORD, HWND, PSP_FILE_CALLBACK_A, PVOID, PDWORD );
WINSETUPAPI BOOL     WINAPI SetupScanFileQueueW( HSPFILEQ, DWORD, HWND, PSP_FILE_CALLBACK_W, PVOID, PDWORD );
#define                     SetupScanFileQueue WINELIB_NAME_AW(SetupScanFileQueue)
WINSETUPAPI BOOL     WINAPI SetupSetDirectoryIdA( HINF, DWORD, PCSTR );
WINSETUPAPI BOOL     WINAPI SetupSetDirectoryIdW( HINF, DWORD, PCWSTR );
#define                     SetupSetDirectoryId WINELIB_NAME_AW(SetupSetDirectoryId)
WINSETUPAPI BOOL     WINAPI SetupSetDirectoryIdExA( HINF, DWORD, PCSTR, DWORD, DWORD, PVOID );
WINSETUPAPI BOOL     WINAPI SetupSetDirectoryIdExW( HINF, DWORD, PCWSTR, DWORD, DWORD, PVOID );
#define                     SetupSetDirectoryIdEx WINELIB_NAME_AW(SetupSetDirectoryIdEx)
WINSETUPAPI BOOL     WINAPI SetupSetFileQueueAlternatePlatformA( HSPFILEQ, PSP_ALTPLATFORM_INFO, PCSTR );
WINSETUPAPI BOOL     WINAPI SetupSetFileQueueAlternatePlatformW( HSPFILEQ, PSP_ALTPLATFORM_INFO, PCWSTR );
#define                     SetupSetFileQueueAlternatePlatform WINELIB_NAME_AW(SetupSetFileQueueAlternatePlatform)
WINSETUPAPI BOOL     WINAPI SetupSetFileQueueFlags( HSPFILEQ, DWORD, DWORD );
WINSETUPAPI BOOL     WINAPI SetupSetNonInteractiveMode(BOOL);
WINSETUPAPI BOOL     WINAPI SetupSetPlatformPathOverrideA(PCSTR);
WINSETUPAPI BOOL     WINAPI SetupSetPlatformPathOverrideW(PCWSTR);
#define                     SetupSetPlatformPathOverride WINELIB_NAME_AW(SetupSetPlatformPathOverride)
WINSETUPAPI BOOL     WINAPI SetupSetSourceListA(DWORD, PCSTR *, UINT);
WINSETUPAPI BOOL     WINAPI SetupSetSourceListW(DWORD, PCWSTR *, UINT);
#define                     SetupSetSourceList WINELIB_NAME_AW(SetupSetSourceList)
WINSETUPAPI void     WINAPI SetupTermDefaultQueueCallback( PVOID );
WINSETUPAPI BOOL     WINAPI SetupTerminateFileLog(HSPFILELOG);
WINSETUPAPI BOOL     WINAPI SetupUninstallOEMInfA(PCSTR, DWORD, PVOID);
WINSETUPAPI BOOL     WINAPI SetupUninstallOEMInfW(PCWSTR, DWORD, PVOID);
#define                     SetupUninstallOEMInf WINELIB_NAME_AW(SetupUninstallOEMInf)
WINSETUPAPI BOOL     WINAPI SetupUninstallNewlyCopiedInfs(HSPFILEQ, DWORD, PVOID);
WINSETUPAPI BOOL     WINAPI SetupVerifyInfFileA(PCSTR, PSP_ALTPLATFORM_INFO, PSP_INF_SIGNER_INFO_A);
WINSETUPAPI BOOL     WINAPI SetupVerifyInfFileW(PCWSTR, PSP_ALTPLATFORM_INFO, PSP_INF_SIGNER_INFO_W);
#define                     SetupVerifyInfFile WINELIB_NAME_AW(SetupVerifyInfFile)

#ifdef __cplusplus
}
#endif

#undef DECL_WINELIB_SETUPAPI_TYPE_AW

#include <poppack.h>

#endif /* _INC_SETUPAPI */
