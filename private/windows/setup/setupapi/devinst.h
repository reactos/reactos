/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    devinst.h

Abstract:

    Private header file for setup device installation routines.

Author:

    Lonny McMichael (lonnym) 10-May-1995

Revision History:

--*/


//
// For now, define the size (in characters) of a GUID string,
// including terminating NULL.
// BUGBUG (lonnym): This should really be defined somewhere else.
//
#define GUID_STRING_LEN (39)

//
// Define the maximum number of IDs that may be present in an ID list
// (either HardwareID or CompatibleIDs).
//
#define MAX_HCID_COUNT (64)

//
// Global strings used by device installer routines.  Sizes are included
// so that we can do sizeof() instead of lstrlen() to determine string
// length.
//
// The content of the following strings is defined in regstr.h:
//
extern CONST TCHAR pszNoUseClass[SIZECHARS(REGSTR_VAL_NOUSECLASS)],
                   pszNoInstallClass[SIZECHARS(REGSTR_VAL_NOINSTALLCLASS)],
                   pszNoDisplayClass[SIZECHARS(REGSTR_VAL_NODISPLAYCLASS)],
                   pszDeviceDesc[SIZECHARS(REGSTR_VAL_DEVDESC)],
                   pszDevicePath[SIZECHARS(REGSTR_VAL_DEVICEPATH)],
                   pszPathSetup[SIZECHARS(REGSTR_PATH_SETUP)],
                   pszKeySetup[SIZECHARS(REGSTR_KEY_SETUP)],
                   pszPathRunOnce[SIZECHARS(REGSTR_PATH_RUNONCE)],
                   pszSourcePath[SIZECHARS(REGSTR_VAL_SRCPATH)],
                   pszSvcPackPath[SIZECHARS(REGSTR_VAL_SVCPAKSRCPATH)],
                   pszDriverCachePath[SIZECHARS(REGSTR_VAL_DRIVERCACHEPATH)],
                   pszBootDir[SIZECHARS(REGSTR_VAL_BOOTDIR)],
                   pszInsIcon[SIZECHARS(REGSTR_VAL_INSICON)],
                   pszInstaller32[SIZECHARS(REGSTR_VAL_INSTALLER_32)],
                   pszEnumPropPages32[SIZECHARS(REGSTR_VAL_ENUMPROPPAGES_32)],
                   pszInfPath[SIZECHARS(REGSTR_VAL_INFPATH)],
                   pszInfSection[SIZECHARS(REGSTR_VAL_INFSECTION)],
                   pszDrvDesc[SIZECHARS(REGSTR_VAL_DRVDESC)],
                   pszHardwareID[SIZECHARS(REGSTR_VAL_HARDWAREID)],
                   pszCompatibleIDs[SIZECHARS(REGSTR_VAL_COMPATIBLEIDS)],
                   pszDriver[SIZECHARS(REGSTR_VAL_DRIVER)],
                   pszConfigFlags[SIZECHARS(REGSTR_VAL_CONFIGFLAGS)],
                   pszMfg[SIZECHARS(REGSTR_VAL_MFG)],
                   pszService[SIZECHARS(REGSTR_VAL_SERVICE)],
                   pszProviderName[SIZECHARS(REGSTR_VAL_PROVIDER_NAME)],
                   pszFriendlyName[SIZECHARS(REGSTR_VAL_FRIENDLYNAME)],
                   pszServicesRegPath[SIZECHARS(REGSTR_PATH_SERVICES)],
                   pszLegacyInfOption[SIZECHARS(REGSTR_VAL_LEGACYINFOPT)],
                   pszInfSectionExt[SIZECHARS(REGSTR_VAL_INFSECTIONEXT)],
                   pszDeviceClassesPath[SIZECHARS(REGSTR_PATH_DEVICE_CLASSES)],
                   pszDeviceInstance[SIZECHARS(REGSTR_VAL_DEVICE_INSTANCE)],
                   pszDefault[SIZECHARS(REGSTR_VAL_DEFAULT)],
                   pszControl[SIZECHARS(REGSTR_KEY_CONTROL)],
                   pszLinked[SIZECHARS(REGSTR_VAL_LINKED)],
                   pszDeviceParameters[SIZECHARS(REGSTR_KEY_DEVICEPARAMETERS)],
                   pszLocationInformation[SIZECHARS(REGSTR_VAL_LOCATION_INFORMATION)],
                   pszCapabilities[SIZECHARS(REGSTR_VAL_CAPABILITIES)],
                   pszUiNumber[SIZECHARS(REGSTR_VAL_UI_NUMBER)],
                   pszUpperFilters[SIZECHARS(REGSTR_VAL_UPPERFILTERS)],
                   pszLowerFilters[SIZECHARS(REGSTR_VAL_LOWERFILTERS)],
                   pszMatchingDeviceId[SIZECHARS(REGSTR_VAL_MATCHINGDEVID)],
                   pszBasicProperties32[SIZECHARS(REGSTR_VAL_BASICPROPERTIES_32)],
                   pszCoInstallers32[SIZECHARS(REGSTR_VAL_COINSTALLERS_32)],
                   pszPathCoDeviceInstallers[SIZECHARS(REGSTR_PATH_CODEVICEINSTALLERS)],
                   pszSystem[SIZECHARS(REGSTR_KEY_SYSTEM)],
                   pszDrvSignPath[SIZECHARS(REGSTR_PATH_DRIVERSIGN)],
                   pszNonDrvSignPath[SIZECHARS(REGSTR_PATH_NONDRIVERSIGN)],
                   pszDrvSignPolicyPath[SIZECHARS(REGSTR_PATH_DRIVERSIGN_POLICY)],
                   pszNonDrvSignPolicyPath[SIZECHARS(REGSTR_PATH_NONDRIVERSIGN_POLICY)],
                   pszDrvSignPolicyValue[SIZECHARS(REGSTR_VAL_POLICY)],
                   pszDrvSignBehaviorOnFailedVerifyDS[SIZECHARS(REGSTR_VAL_BEHAVIOR_ON_FAILED_VERIFY)],
                   pszDriverDate[SIZECHARS(REGSTR_VAL_DRIVERDATE)],
                   pszDriverDateData[SIZECHARS(REGSTR_VAL_DRIVERDATEDATA)],
                   pszDriverVersion[SIZECHARS(REGSTR_VAL_DRIVERVERSION)],
                   pszDevSecurity[SIZECHARS(REGSTR_VAL_DEVICE_SECURITY_DESCRIPTOR)],
                   pszDevType[SIZECHARS(REGSTR_VAL_DEVICE_TYPE)],
                   pszExclusive[SIZECHARS(REGSTR_VAL_DEVICE_EXCLUSIVE)],
                   pszCharacteristics[SIZECHARS(REGSTR_VAL_DEVICE_CHARACTERISTICS)],
                   pszUiNumberDescFormat[SIZECHARS(REGSTR_VAL_UI_NUMBER_DESC_FORMAT)];


//
// Other misc. global strings:
//
#define DISTR_INF_WILDCARD                (TEXT("*.inf"))
#define DISTR_OEMINF_WILDCARD             (TEXT("oem*.inf"))
#define DISTR_CI_DEFAULTPROC              (TEXT("ClassInstall"))
#define DISTR_SPACE_LPAREN                (TEXT(" ("))
#define DISTR_RPAREN                      (TEXT(")"))
#define DISTR_UNIQUE_SUBKEY               (TEXT("\\%04u"))
#define DISTR_OEMINF_GENERATE             (TEXT("%s\\oem%d.inf"))
#define DISTR_OEMINF_DEFAULTPATH          (TEXT("A:\\"))
#define DISTR_DEFAULT_SERVICE             (TEXT("Default Service"))
#define DISTR_GUID_NULL                   (TEXT("{00000000-0000-0000-0000-000000000000}"))
#define DISTR_EVENTLOG                    (TEXT("\\EventLog"))
#define DISTR_GROUPORDERLIST_PATH         (REGSTR_PATH_CURRENT_CONTROL_SET TEXT("\\GroupOrderList"))
#define DISTR_SERVICEGROUPORDER_PATH      (REGSTR_PATH_CURRENT_CONTROL_SET TEXT("\\ServiceGroupOrder"))
#define DISTR_OPTIONS                     (TEXT("Options"))
#define DISTR_OPTIONSTEXT                 (TEXT("OptionsText"))
#define DISTR_LANGUAGESSUPPORTED          (TEXT("LanguagesSupported"))
#define DISTR_RUNONCE_EXE                 (TEXT("runonce"))
#define DISTR_GRPCONV                     (TEXT("grpconv -o"))
#define DISTR_GRPCONV_NOUI                (TEXT("grpconv -u"))
#define DISTR_DEFAULT_SYSPART             (TEXT("C:\\"))
#define DISTR_BASICPROP_DEFAULTPROC       (TEXT("BasicProperties"))
#define DISTR_ENUMPROP_DEFAULTPROC        (TEXT("EnumPropPages"))
#define DISTR_CODEVICEINSTALL_DEFAULTPROC (TEXT("CoDeviceInstall"))
#define DISTR_DRIVER_OBJECT_PATH_PREFIX   (TEXT("\\DRIVER\\"))      // must be uppercase!
#define DISTR_DRIVER_SIGNING_CLASSES      (TEXT("DriverSigningClasses"))

extern CONST TCHAR pszInfWildcard[SIZECHARS(DISTR_INF_WILDCARD)],
                   pszOemInfWildcard[SIZECHARS(DISTR_OEMINF_WILDCARD)],
                   pszCiDefaultProc[SIZECHARS(DISTR_CI_DEFAULTPROC)],
                   pszSpaceLparen[SIZECHARS(DISTR_SPACE_LPAREN)],
                   pszRparen[SIZECHARS(DISTR_RPAREN)],
                   pszUniqueSubKey[SIZECHARS(DISTR_UNIQUE_SUBKEY)],
                   pszOemInfGenerate[SIZECHARS(DISTR_OEMINF_GENERATE)],
                   pszOemInfDefaultPath[SIZECHARS(DISTR_OEMINF_DEFAULTPATH)],
                   pszDefaultService[SIZECHARS(DISTR_DEFAULT_SERVICE)],
                   pszGuidNull[SIZECHARS(DISTR_GUID_NULL)],
                   pszEventLog[SIZECHARS(DISTR_EVENTLOG)],
                   pszGroupOrderListPath[SIZECHARS(DISTR_GROUPORDERLIST_PATH)],
                   pszServiceGroupOrderPath[SIZECHARS(DISTR_SERVICEGROUPORDER_PATH)],
                   pszOptions[SIZECHARS(DISTR_OPTIONS)],
                   pszOptionsText[SIZECHARS(DISTR_OPTIONSTEXT)],
                   pszLanguagesSupported[SIZECHARS(DISTR_LANGUAGESSUPPORTED)],
                   pszRunOnceExe[SIZECHARS(DISTR_RUNONCE_EXE)],
                   pszGrpConv[SIZECHARS(DISTR_GRPCONV)],
                   pszGrpConvNoUi[SIZECHARS(DISTR_GRPCONV_NOUI)],
                   pszDefaultSystemPartition[SIZECHARS(DISTR_DEFAULT_SYSPART)],
                   pszBasicPropDefaultProc[SIZECHARS(DISTR_BASICPROP_DEFAULTPROC)],
                   pszEnumPropDefaultProc[SIZECHARS(DISTR_ENUMPROP_DEFAULTPROC)],
                   pszCoInstallerDefaultProc[SIZECHARS(DISTR_CODEVICEINSTALL_DEFAULTPROC)],
                   pszDriverObjectPathPrefix[SIZECHARS(DISTR_DRIVER_OBJECT_PATH_PREFIX)],
                   pszDriverSigningClasses[SIZECHARS(DISTR_DRIVER_SIGNING_CLASSES)];

//
// Global translation array for finding CM_DRP_* ordinal
// given property name or SPDRP_* value.
//
extern STRING_TO_DATA InfRegValToDevRegProp[];
extern STRING_TO_DATA InfRegValToClassRegProp[];

//
// Define a macro that does the DI-to-CM property translation
//
#define SPDRP_TO_CMDRP(i) (InfRegValToDevRegProp[(i)].Data)
//
// Class registry translation uses the same table
//
#define SPCRP_TO_CMCRP(i) (InfRegValToClassRegProp[(i)].Data)

//
// Define callback routine for EnumSingleInf,
// EnumInfsInDirPathList & EnumInfsInDirectory.
//
typedef BOOL (*PSP_ENUMINF_CALLBACK_ROUTINE) (
    IN     PCTSTR               InfName,
    IN     LPWIN32_FIND_DATA    InfFileData,
    IN     PSETUP_LOG_CONTEXT   LogContext,
    IN OUT PVOID                Context
    );

//
// Define a value indicating a no-match ranking.
//
#define RANK_NO_MATCH (0xFFFFFFFF)

//
// Driver ranking bases. Lower Ranks are better.  Rank 0 is the best possible Rank.
// Any Rank less than 0x00001000 is a HardwareID match that is considered a good match.
//
#define RANK_HWID_INF_HWID_BASE 0x00000000      // For match with Hardware's HardwareID and INF's HardwareID
#define RANK_HWID_INF_CID_BASE  0x00001000      // For match with Hardware's HardwareID and INF's CompatibleID
#define RANK_CID_INF_HWID_BASE  0x00002000      // For match with Hardware's CompatibleID and INF's HardwareID
#define RANK_CID_INF_CID_BASE   0x00003000      // For match with Hardware's CompatibleID and INF's CompatibleID

//
// Define special value used to indicate that one of our enumeration 'hint'
// indices is invalid.
//
#define INVALID_ENUM_INDEX  (0xFFFFFFFF)

//
// Define prototype of callback function supplied by class installers.
//
typedef DWORD (CALLBACK* CLASS_INSTALL_PROC) (
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    );

//
// Define prototype of property sheet provider function--basically, an
// ExtensionPropSheetPageProc function with a (potentially) different name.
//
typedef BOOL (CALLBACK* PROPSHEET_PROVIDER_PROC) (
    IN PSP_PROPSHEETPAGE_REQUEST PropPageRequest,
    IN LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    IN LPARAM lParam
    );

//
// Define prototype of the co-installer function.
//
typedef DWORD (CALLBACK* COINSTALLER_PROC) (
    IN     DI_FUNCTION               InstallFunction,
    IN     HDEVINFO                  DeviceInfoSet,
    IN     PSP_DEVINFO_DATA          DeviceInfoData, OPTIONAL
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    );


//
// Define structure for the internal representation of a single
// driver information node.
//
typedef struct _DRIVER_NODE {

    struct _DRIVER_NODE *Next;

    UINT Rank;

    FILETIME InfDate;

    LONG DrvDescription;

    //
    // Have to have both forms of the strings below because we must have both
    // case-insensitive (i.e., atom-like) behavior, and keep the original case
    // for display.
    //
    LONG DevDescription;
    LONG DevDescriptionDisplayName;

    LONG ProviderName;
    LONG ProviderDisplayName;

    LONG MfgName;
    LONG MfgDisplayName;

    LONG InfFileName;

    LONG InfSectionName;

    //
    // The following field is only valid if this is a legacy INF driver node.  It
    // tells us what language to use when running the INF interpreter.
    //
    LONG LegacyInfLang;

    LONG HardwareId;

    DWORD NumCompatIds;

    PLONG CompatIdList;

    //
    // Store the index of the device ID that a compatible match was based on.  If
    // this was a HardwareId match, this value is -1, otherwise, it is the index
    // into the CompatIdList array of the device ID that matched.
    //
    LONG MatchingDeviceId;

    DWORD Flags;

    DWORD_PTR PrivateData;

    //
    // Store the GUID index INF's class from which this node came.  We need to do this,
    // in order to easily determine the class of the driver node (e.g., so that we
    // can change the device's class when a new driver node is selected).
    //
    LONG GuidIndex;

    FILETIME  DriverDate;
    DWORDLONG DriverVersion;

} DRIVER_NODE, *PDRIVER_NODE;


//
// Define structure to contain a co-installer entry.
//
typedef struct _COINSTALLER_NODE {
    HINSTANCE hinstCoInstaller;
    COINSTALLER_PROC CoInstallerEntryPoint;
} COINSTALLER_NODE, *PCOINSTALLER_NODE;

//
// Define structure containing context information about co-installer
// callbacks for the duration of a DIF call.
//
typedef struct _COINSTALLER_INTERNAL_CONTEXT {
    COINSTALLER_CONTEXT_DATA Context;
    BOOL                     DoPostProcessing;
    COINSTALLER_PROC         CoInstallerEntryPoint;
} COINSTALLER_INTERNAL_CONTEXT, *PCOINSTALLER_INTERNAL_CONTEXT;


//
// Define structure for the internal storage of device installation
// parameters.
//
typedef struct _DEVINSTALL_PARAM_BLOCK {

    //
    // Flags for controlling installation and UI functions.
    //
    DWORD Flags;
    DWORD FlagsEx;

    //
    // Specifies the window handle that will own UI related to this
    // installation.  MAY BE NULL.
    //
    HWND hwndParent;

    //
    // Installation message handling parameters.
    //
    PSP_FILE_CALLBACK InstallMsgHandler;
    PVOID             InstallMsgHandlerContext;
    BOOL              InstallMsgHandlerIsNativeCharWidth;

    //
    // Handle to a caller-supplied copy-queue.  If this handle is present,
    // then file copy/rename/delete operations will be queued to this handle
    // instead of being acted upon.  This will only happen if the DI_NOVCP
    // bit is set in the Flags field.
    // If no caller-supplied queue is present, this value is NULL
    // (_not_ INVALID_HANDLE_VALUE).
    //
    HSPFILEQ UserFileQ;

    //
    // Private DWORD reserved for Class Installer usage.
    //
    ULONG_PTR ClassInstallReserved;

    //
    // Specifies the string table index of an optional INF file
    // path.  If the string is not supplied, its index will be -1.
    //
    LONG DriverPath;

    //
    // Pointer to class installer parameters.  The first field of any class
    // installer parameter block is always a SP_CLASSINSTALL_HEADER structure.
    // The cbSize field of that structure gives the size, in bytes, of the header
    // (used for versioning), and the InstallFunction field gives the DI_FUNCTION
    // code that indicates how the parameter buffer is to be interpreted.
    // MAY BE NULL!
    //
    PSP_CLASSINSTALL_HEADER ClassInstallHeader;
    DWORD ClassInstallParamsSize;

    //
    // THE FOLLOWING PARAMETERS ARE NOT EXPOSED TO CALLERS (i.e., via
    // SetupDi(Get|Set)DeviceInstallParams).
    //

    HINSTANCE hinstClassInstaller;
    CLASS_INSTALL_PROC ClassInstallerEntryPoint;

    HINSTANCE hinstClassPropProvider;
    PROPSHEET_PROVIDER_PROC ClassEnumPropPagesEntryPoint;

    HINSTANCE hinstDevicePropProvider;
    PROPSHEET_PROVIDER_PROC DeviceEnumPropPagesEntryPoint;

    HINSTANCE hinstBasicPropProvider;
    PROPSHEET_PROVIDER_PROC EnumBasicPropertiesEntryPoint;

    //
    // Maintain a list of co-installers to be called along with the class installer.
    // The count will be -1 if the list hasn't been retrieved yet.
    //
    LONG CoInstallerCount;
    PCOINSTALLER_NODE CoInstallerList;

    //
    // Logging context -- this is only here because this struct is shared
    // by both DEVINFO_ELEM and DEVINFO_SET.
    //
    PSETUP_LOG_CONTEXT LogContext;

} DEVINSTALL_PARAM_BLOCK, *PDEVINSTALL_PARAM_BLOCK;


//
// Define structures used for associating lists of interface devices with
// devinfo elements.
//
typedef struct _INTERFACE_DEVICE_NODE {

    struct _INTERFACE_DEVICE_NODE *Next;

    //
    // String table ID for this interface device's symbolic link name.
    //
    LONG SymLinkName;

    //
    // Store the interface class GUID index in each node.  We need to do this,
    // in order to easily determine the class of the node.
    //
    LONG GuidIndex;

    //
    // The Flags field contains the same flags as the client sees in their
    // SP_INTERFACE_DEVICE_DATA structure.
    //
    DWORD Flags;

    //
    // Store a back-pointer to the devinfo element, because interface devices
    // may be enumerated outside the context of a device information element, and
    // we need to know how to get back to the owning device instance.
    //
    struct _DEVINFO_ELEM *OwningDevInfoElem;

} INTERFACE_DEVICE_NODE, *PINTERFACE_DEVICE_NODE;

typedef struct _INTERFACE_CLASS_LIST {
    LONG                   GuidIndex;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode;
    PINTERFACE_DEVICE_NODE InterfaceDeviceTruncateNode;  // used for rollback.
    DWORD                  InterfaceDeviceCount;
} INTERFACE_CLASS_LIST, *PINTERFACE_CLASS_LIST;


//
// Define flags for DiElemFlags field of DEVINFO_ELEM structure.
//
#define DIE_IS_PHANTOM      (0x00000001) // is this a phantom (not live) devinst?
#define DIE_IS_REGISTERED   (0x00000002) // has this devinst been registered?
#define DIE_IS_LOCKED       (0x00000004) // are we explicitly locked during some UI
                                         // operation (e.g., wizard)?

//
// Define structure for the internal representation of a single
// device information element.
//
typedef struct _DEVINFO_ELEM {
    //
    // Store the address of the containing devinfo set at the beginning of
    // this structure.  This is used for validation of a caller-supplied
    // SP_DEVINFO_DATA, and is more efficient than the previous method of
    // searching through all devinfo elements in the set to make sure the
    // specified element exists in the set.  This field should be zeroed
    // out when this element is destroyed.
    //
    struct _DEVICE_INFO_SET *ContainingDeviceInfoSet;

    //
    // Pointer to the next element in the set.
    //
    struct _DEVINFO_ELEM *Next;

    //
    // Specifies the device instance handle for this device.  This will
    // be a phantom device instance handle if DIE_IS_PHANTOM is set.
    //
    // This should always contain a handle, unless the device instance
    // handle could not be re-opened after a re-enumeration (in which case,
    // the DI_NEEDREBOOT flag will be set), or if the device information
    // element was globally removed or config-specific removed from the last
    // hardware profile.
    //
    DEVINST DevInst;

    //
    // Specifies the GUID for this device's class.
    //
    GUID ClassGuid;

    //
    // Specifies flags pertaining to this device information element.
    // These DIE_* flags are for internal use only.
    //
    DWORD DiElemFlags;

    //
    // List of class drivers for this element.
    //
    UINT          ClassDriverCount;
    PDRIVER_NODE  ClassDriverHead;
    PDRIVER_NODE  ClassDriverTail;

    //
    // class drivernode index 'hint' to speed up enumeration via
    // SetupDiEnumDriverInfo
    //
    PDRIVER_NODE ClassDriverEnumHint;       // may be NULL
    DWORD        ClassDriverEnumHintIndex;  // may be INVALID_ENUM_INDEX

    //
    // List of compatible drivers for this element.
    //
    UINT          CompatDriverCount;
    PDRIVER_NODE  CompatDriverHead;
    PDRIVER_NODE  CompatDriverTail;

    //
    // compatible drivernode index 'hint' to speed up enumeration via
    // SetupDiEnumDriverInfo
    //
    PDRIVER_NODE CompatDriverEnumHint;       // may be NULL
    DWORD        CompatDriverEnumHintIndex;  // may be INVALID_ENUM_INDEX

    //
    // Pointer to selected driver for this element (may be
    // NULL if none currently selected).  Whether this is a
    // class or compatible driver is specified by the
    // SelectedDriverType field.
    //
    PDRIVER_NODE  SelectedDriver;
    DWORD         SelectedDriverType;

    //
    // Installation parameter block.
    //
    DEVINSTALL_PARAM_BLOCK InstallParamBlock;

    //
    // Specifies the string table index of the device description.
    // If no description is known, this value will be -1.
    //
    // We store this string twice--once case-sensitively and once case-insensitively,
    // because we need it for displaying _and_ for fast lookup.
    //
    LONG DeviceDescription;
    LONG DeviceDescriptionDisplayName;

    //
    // Maintain an array of interface device lists.  These lists represent the interface
    // devices owned by this device instance (but only those that have been retrieved, e.g.
    // by calling SetupDiGetClassDevs(...DIGCF_INTERFACEDEVICE...)
    //
    // (This array pointer may be NULL.)
    //
    PINTERFACE_CLASS_LIST InterfaceClassList;
    DWORD                 InterfaceClassListSize;

    //
    // Extra (non-class installer) data associated with each device information element.
    // Only exposed via private API for use during GUI-mode setup.
    //
    DWORD Context;

} DEVINFO_ELEM, *PDEVINFO_ELEM;


//
// Structure containing dialog data for wizard pages.  (Amalgamation of
// DIALOGDATA structures defined in setupx and sysdm.)
//
typedef struct _SP_DIALOGDATA {

    INT             iBitmap;              // index into mini-icon bitmap

    HDEVINFO        DevInfoSet;           // DevInfo set we're working with
    PDEVINFO_ELEM   DevInfoElem;          // if DD_FLAG_USE_DEVINFO_ELEM flag set
    UINT            flags;

    HWND            hwndDrvList;          // window of the driver list
    HWND            hwndMfgList;          // window of the Manufacturer list

    INT             ListType;             // IDC_NDW_PICKDEV_SHOWALL or IDC_NDW_PICKDEV_SHOWCOMPAT

    BOOL            bKeeplpCompatDrvList;
    BOOL            bKeeplpClassDrvList;
    BOOL            bKeeplpSelectedDrv;

    LONG            iCurDesc;             // string table index for the description of currently
                                          // selected driver (or to-be-selected driver)

    BOOL            AuxThreadRunning;       // Is our class driver search thread still running?
    DWORD           PendingAction;          // What (if anything) should we do when it finishes?
    int             CurSelectionForSuccess; // If we have a pending successful return, what is the
                                            // listbox index for the successful selection?

} SP_DIALOGDATA, *PSP_DIALOGDATA;

//
// Flags for SP_DIALOGDATA.flags:
//
#define DD_FLAG_USE_DEVINFO_ELEM   0x00000001
#define DD_FLAG_IS_DIALOGBOX       0x00000002
#define DD_FLAG_CLASSLIST_FAILED   0x00000004

//
// Pending Action codes used in the NEWDEVWIZ_DATA structure to indicate what
// should happen as soon as the auxilliary class driver search thread notifies us
// of its termination.
//
#define PENDING_ACTION_NONE             0
#define PENDING_ACTION_SELDONE          1
#define PENDING_ACTION_SHOWCLASS        2
#define PENDING_ACTION_CANCEL           3
#define PENDING_ACTION_OEM              4
#define PENDING_ACTION_WINDOWSUPDATE    5

//
// Define structure used for internal state storage by Device Installer
// wizard pages.  (From NEWDEVWIZ_INSTANCE struct in Win95 sysdm.)
//
typedef struct _NEWDEVWIZ_DATA {

    SP_INSTALLWIZARD_DATA InstallData;

    SP_DIALOGDATA         ddData;

    BOOL                  bInit;
    UINT_PTR              idTimer;

} NEWDEVWIZ_DATA, *PNEWDEVWIZ_DATA;

//
// Define wizard page object structure used to ensure that wizard page
// buffer is kept as long as needed, and destroyed when no longer in use.
//
typedef struct _WIZPAGE_OBJECT {

    struct _WIZPAGE_OBJECT *Next;

    DWORD RefCount;

    PNEWDEVWIZ_DATA ndwData;

} WIZPAGE_OBJECT, *PWIZPAGE_OBJECT;


//
// Define driver list object structure used in the device information set
// to keep track of the various class driver lists that devinfo elements
// have referenced.
//
typedef struct _DRIVER_LIST_OBJECT {

    struct _DRIVER_LIST_OBJECT *Next;

    DWORD RefCount;

    //
    // We keep track of what parameters were used to create this driver
    // list, so that we can copy them to a new devinfo element during
    // inheritance.
    //
    DWORD ListCreationFlags;
    DWORD ListCreationFlagsEx;
    LONG ListCreationDriverPath;

    //
    // Also, keep track of what class this list was built for.  Although a
    // device's class may change, this GUID remains constant.
    //
    GUID ClassGuid;

    //
    // Actual driver list.  (This is also used as an ID used to find the
    // driver list object given a driver list head.  We can do this, since
    // we know that once a driver list is built, the head element never
    // changes.)
    //
    PDRIVER_NODE DriverListHead;

} DRIVER_LIST_OBJECT, *PDRIVER_LIST_OBJECT;


//
// Define node that tracks addition module handles to be unloaded when the
// device information set is destroyed.  This is used when a class installer,
// property page provider, or co-installer becomes invalid (e.g., as a result
// of a change in the device's class), but we can't unload the module yet.
//
typedef struct _MODULE_HANDLE_LIST_NODE {

    struct _MODULE_HANDLE_LIST_NODE *Next;

    DWORD ModuleCount;
    HINSTANCE ModuleList[ANYSIZE_ARRAY];

} MODULE_HANDLE_LIST_NODE, *PMODULE_HANDLE_LIST_NODE;

//
// Define structure for the internal representation of a
// device information set.
//
typedef struct _DEVICE_INFO_SET {

    //
    // Specifies whether there is a class GUID associated
    // with this set, and if so, what it is.
    //
    BOOL          HasClassGuid;
    GUID          ClassGuid;

    //
    // List of class drivers for this set.
    //
    UINT          ClassDriverCount;
    PDRIVER_NODE  ClassDriverHead;
    PDRIVER_NODE  ClassDriverTail;

    //
    // class drivernode index 'hint' to speed up enumeration via
    // SetupDiEnumDriverInfo
    //
    PDRIVER_NODE ClassDriverEnumHint;       // may be NULL
    DWORD        ClassDriverEnumHintIndex;  // may be INVALID_ENUM_INDEX

    //
    // Pointer to selected class driver for this device information
    // set (may be NULL if none currently selected).
    //
    PDRIVER_NODE  SelectedClassDriver;

    //
    // List of device information elements in the set.
    //
    UINT          DeviceInfoCount;
    PDEVINFO_ELEM DeviceInfoHead;
    PDEVINFO_ELEM DeviceInfoTail;

    //
    // devinfo element index 'hint' to speed up enumeration via
    // SetupDiEnumDeviceInfo
    //
    PDEVINFO_ELEM DeviceInfoEnumHint;       // may be NULL
    DWORD         DeviceInfoEnumHintIndex;  // may be INVALID_ENUM_INDEX

    //
    // Pointer to selected device for this device information set (may
    // be NULL if none currently selected).  This is used during
    // installation wizard.
    //
    PDEVINFO_ELEM SelectedDevInfoElem;

    //
    // Installation parameter block (for global class driver list, if
    // present).
    //
    DEVINSTALL_PARAM_BLOCK InstallParamBlock;

    //
    // Private string table.
    //
    PVOID StringTable;

    //
    // Maintain a list of currently-active wizard objects.  This allows us
    // to do the refcounting correctly for each object, and to keep the
    // set from being deleted until all wizard objects are destroyed.
    //
    PWIZPAGE_OBJECT WizPageList;

    //
    // Maintain a list of class driver lists that are currently being referenced
    // by various devinfo elements, as well as by the device info set itself
    // (i.e., for the current global class driver list.)
    //
    PDRIVER_LIST_OBJECT ClassDrvListObjectList;

    //
    // Maintain a reference count on how many times a thread has acquired
    // the lock on this device information set.  This indicates how deeply
    // nested we currently are in device installer calls.  The set can only
    // be deleted if this count is 1.
    //
    DWORD LockRefCount;

    //
    // Maintain a list of additional module handles we need to do a FreeLibrary
    // on when this device information set is destroyed.
    //
    PMODULE_HANDLE_LIST_NODE ModulesToFree;

    //
    // Maintain an array of class GUIDs for all driver nodes and device
    // interfaces used by members of this set.  (May be NULL.)
    //
    LPGUID GuidTable;
    DWORD  GuidTableSize;

    //
    // ConfigMgr machine name (string id) and handle, if this is a remote HDEVINFO set.
    //
    LONG     MachineName;   // -1 if local
    HMACHINE hMachine;      // NULL if local

    //
    // Synchronization
    //
    MYLOCK Lock;

} DEVICE_INFO_SET, *PDEVICE_INFO_SET;

#define LockDeviceInfoSet(d)   BeginSynchronizedAccess(&((d)->Lock))

#define UnlockDeviceInfoSet(d)          \
                                        \
    ((d)->LockRefCount)--;              \
    EndSynchronizedAccess(&((d)->Lock))


//
// Define structures for global mini-icon storage.
//
typedef struct _CLASSICON {

    CONST GUID        *ClassGuid;
    UINT               MiniBitmapIndex;
    struct _CLASSICON *Next;

} CLASSICON, *PCLASSICON;

typedef struct _MINI_ICON_LIST {

    //
    // HDC for memory containing mini-icon bitmap
    //
    HDC hdcMiniMem;

    //
    // Handle to the bitmap image for the mini-icons
    //
    HBITMAP hbmMiniImage;

    //
    // Handle to the bitmap image for the mini-icon mask.
    //
    HBITMAP hbmMiniMask;

    //
    // Number of mini-icons in the bitmap
    //
    UINT NumClassImages;

    //
    // Head of list for installer-provided class icons.
    //
    PCLASSICON ClassIconList;

    //
    // Synchronization
    //
    MYLOCK Lock;

} MINI_ICON_LIST, *PMINI_ICON_LIST;

#define LockMiniIconList(d)   BeginSynchronizedAccess(&((d)->Lock))
#define UnlockMiniIconList(d) EndSynchronizedAccess(&((d)->Lock))

//
// Global mini-icon list.
//
extern MINI_ICON_LIST GlobalMiniIconList;




typedef struct _CLASS_IMAGE_LIST {

    //
    // Index of the "Unknown" class image
    //
    int         UnknownImageIndex;

    //
    // List of class guids
    //
    LPGUID      ClassGuidList;

    //
    // Head of linked list of class icons.
    //
    PCLASSICON  ClassIconList;

    //
    // Synchronization
    //
    MYLOCK      Lock;

} CLASS_IMAGE_LIST, *PCLASS_IMAGE_LIST;


#define LockImageList(d)   BeginSynchronizedAccess(&((d)->Lock))
#define UnlockImageList(d) EndSynchronizedAccess(&((d)->Lock))


typedef struct _DRVSEARCH_INPROGRESS_NODE {

    struct _DRVSEARCH_INPROGRESS_NODE *Next;

    //
    // Handle of device information set for which driver list is
    // currently being built.
    //
    HDEVINFO DeviceInfoSet;

    //
    // Flag indicating that the driver search should be aborted.
    //
    BOOL CancelSearch;

    //
    // Event handle that auxiliary thread waits on once it has set
    // the 'CancelSearch' flag (and once it has release the list
    // lock).  When the thread doing the search notices the cancel
    // request, it will signal the event, thus the waiting thread
    // can ensure that the search has been cancelled before it returns.
    //
    HANDLE SearchCancelledEvent;

} DRVSEARCH_INPROGRESS_NODE, *PDRVSEARCH_INPROGRESS_NODE;

typedef struct _DRVSEARCH_INPROGRESS_LIST {

    //
    // Head of linked list containing nodes for each device information
    // set for which a driver search is currently underway.
    //
    PDRVSEARCH_INPROGRESS_NODE DrvSearchHead;

    //
    // Synchronization
    //
    MYLOCK Lock;

} DRVSEARCH_INPROGRESS_LIST, *PDRVSEARCH_INPROGRESS_LIST;

#define LockDrvSearchInProgressList(d)   BeginSynchronizedAccess(&((d)->Lock))
#define UnlockDrvSearchInProgressList(d) EndSynchronizedAccess(&((d)->Lock))

//
// Global "Driver Search In-Progress" list.
//
extern DRVSEARCH_INPROGRESS_LIST GlobalDrvSearchInProgressList;


//
// Device Information Set manipulation routines
//
PDEVICE_INFO_SET
AllocateDeviceInfoSet(
    VOID
    );

VOID
DestroyDeviceInfoElement(
    IN HDEVINFO         hDevInfo,
    IN PDEVICE_INFO_SET pDeviceInfoSet,
    IN PDEVINFO_ELEM    DeviceInfoElement
    );

DWORD
DestroyDeviceInfoSet(
    IN HDEVINFO         hDevInfo,      OPTIONAL
    IN PDEVICE_INFO_SET pDeviceInfoSet
    );

PDEVICE_INFO_SET
AccessDeviceInfoSet(
    IN HDEVINFO DeviceInfoSet
    );

PDEVICE_INFO_SET
CloneDeviceInfoSet(
    IN HDEVINFO hDevInfo
    );

PDEVICE_INFO_SET
RollbackDeviceInfoSet(
    IN HDEVINFO hDevInfo,
    IN PDEVICE_INFO_SET ClonedDeviceInfoSet
    );

PDEVICE_INFO_SET
CommitDeviceInfoSet(
    IN HDEVINFO hDevInfo,
    IN PDEVICE_INFO_SET ClonedDeviceInfoSet
    );

PDEVINFO_ELEM
FindDevInfoByDevInst(
    IN  PDEVICE_INFO_SET  DeviceInfoSet,
    IN  DEVINST           DevInst,
    OUT PDEVINFO_ELEM    *PrevDevInfoElem OPTIONAL
    );

BOOL
DevInfoDataFromDeviceInfoElement(
    IN  PDEVICE_INFO_SET DeviceInfoSet,
    IN  PDEVINFO_ELEM    DevInfoElem,
    OUT PSP_DEVINFO_DATA DeviceInfoData
    );

PDEVINFO_ELEM
FindAssociatedDevInfoElem(
    IN  PDEVICE_INFO_SET  DeviceInfoSet,
    IN  PSP_DEVINFO_DATA  DeviceInfoData,
    OUT PDEVINFO_ELEM    *PreviousElement OPTIONAL
    );


//
// Driver Node manipulation routines.
//
DWORD
CreateDriverNode(
    IN  UINT          Rank,
    IN  PCTSTR        DevDescription,
    IN  PCTSTR        DrvDescription,
    IN  PCTSTR        ProviderName,   OPTIONAL
    IN  PCTSTR        MfgName,
    IN  PFILETIME     InfDate,
    IN  PCTSTR        InfFileName,
    IN  PCTSTR        InfSectionName,
    IN  PVOID         StringTable,
    IN  LONG          InfClassGuidIndex,
    OUT PDRIVER_NODE *DriverNode
    );

PDRIVER_LIST_OBJECT
GetAssociatedDriverListObject(
    IN  PDRIVER_LIST_OBJECT  ObjectListHead,
    IN  PDRIVER_NODE         DriverListHead,
    OUT PDRIVER_LIST_OBJECT *PrevDriverListObject OPTIONAL
    );

VOID
DereferenceClassDriverList(
    IN PDEVICE_INFO_SET DeviceInfoSet,
    IN PDRIVER_NODE     DriverListHead OPTIONAL
    );

VOID
DestroyDriverNodes(
    IN PDRIVER_NODE DriverNode,
    IN PDEVICE_INFO_SET pDeviceInfoSet
    );

BOOL
DrvInfoDataFromDriverNode(
    IN  PDEVICE_INFO_SET DeviceInfoSet,
    IN  PDRIVER_NODE     DriverNode,
    IN  DWORD            DriverType,
    OUT PSP_DRVINFO_DATA DriverInfoData
    );

PDRIVER_NODE
FindAssociatedDriverNode(
    IN  PDRIVER_NODE      DriverListHead,
    IN  PSP_DRVINFO_DATA  DriverInfoData,
    OUT PDRIVER_NODE     *PreviousNode    OPTIONAL
    );

PDRIVER_NODE
SearchForDriverNode(
    IN  PVOID             StringTable,
    IN  PDRIVER_NODE      DriverListHead,
    IN  PSP_DRVINFO_DATA  DriverInfoData,
    OUT PDRIVER_NODE     *PreviousNode    OPTIONAL
    );

DWORD
DrvInfoDetailsFromDriverNode(
    IN  PDEVICE_INFO_SET        DeviceInfoSet,
    IN  PDRIVER_NODE            DriverNode,
    OUT PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData, OPTIONAL
    IN  DWORD                   BufferSize,
    OUT PDWORD                  RequiredSize          OPTIONAL
    );


//
// Installation parameter manipulation routines
//
DWORD
GetDevInstallParams(
    IN  PDEVICE_INFO_SET        DeviceInfoSet,
    IN  PDEVINSTALL_PARAM_BLOCK DevInstParamBlock,
    OUT PSP_DEVINSTALL_PARAMS   DeviceInstallParams
    );

DWORD
GetClassInstallParams(
    IN  PDEVINSTALL_PARAM_BLOCK DevInstParamBlock,
    OUT PSP_CLASSINSTALL_HEADER ClassInstallParams, OPTIONAL
    IN  DWORD                   BufferSize,
    OUT PDWORD                  RequiredSize        OPTIONAL
    );

DWORD
SetDevInstallParams(
    IN OUT PDEVICE_INFO_SET        DeviceInfoSet,
    IN     PSP_DEVINSTALL_PARAMS   DeviceInstallParams,
    OUT    PDEVINSTALL_PARAM_BLOCK DevInstParamBlock,
    IN     BOOL                    MsgHandlerIsNativeCharWidth
    );

DWORD
SetClassInstallParams(
    IN OUT PDEVICE_INFO_SET        DeviceInfoSet,
    IN     PSP_CLASSINSTALL_HEADER ClassInstallParams,    OPTIONAL
    IN     DWORD                   ClassInstallParamsSize,
    OUT    PDEVINSTALL_PARAM_BLOCK DevInstParamBlock
    );

VOID
DestroyInstallParamBlock(
    IN HDEVINFO                hDevInfo,         OPTIONAL
    IN PDEVICE_INFO_SET        pDeviceInfoSet,
    IN PDEVINFO_ELEM           DevInfoElem,      OPTIONAL
    IN PDEVINSTALL_PARAM_BLOCK InstallParamBlock
    );

DWORD
GetDrvInstallParams(
    IN  PDRIVER_NODE          DriverNode,
    OUT PSP_DRVINSTALL_PARAMS DriverInstallParams
    );

DWORD
SetDrvInstallParams(
    IN  PSP_DRVINSTALL_PARAMS DriverInstallParams,
    OUT PDRIVER_NODE          DriverNode
    );


//
// Device Instance manipulation routines
//

#if 0   // This functionality is performed by CM APIs.

BOOL
ValidateDeviceInstanceId(
    IN  PCTSTR DeviceInstanceId
    );

VOID
CopyFixedUpDeviceId(
    OUT PTSTR  DestinationString,
    IN  PCTSTR SourceString,
    IN  DWORD  SourceStringLen
    );

#endif   // This functionality is performed by CM APIs.


//
// String Table helper functions
//
LONG
AddMultiSzToStringTable(
    IN  PVOID   StringTable,
    IN  PTCHAR  MultiSzBuffer,
    OUT PLONG   StringIdList,
    IN  DWORD   StringIdListSize,
    IN  BOOL    CaseSensitive,
    OUT PTCHAR *UnprocessedBuffer    OPTIONAL
    );

LONG
LookUpStringInDevInfoSet(
    IN HDEVINFO DeviceInfoSet,
    IN PTSTR    String,
    IN BOOL     CaseSensitive
    );


//
// INF processing functions
//
DWORD
EnumSingleInf(
    IN     PCTSTR                       InfName,
    IN OUT LPWIN32_FIND_DATA            InfFileData,
    IN     DWORD                        SearchControl,
    IN     PSP_ENUMINF_CALLBACK_ROUTINE EnumInfCallback,
    IN     PSETUP_LOG_CONTEXT           LogContext,
    IN OUT PVOID                        Context
    );

DWORD
EnumInfsInDirPathList(
    IN     PCTSTR                       DirPathList, OPTIONAL
    IN     DWORD                        SearchControl,
    IN     PSP_ENUMINF_CALLBACK_ROUTINE EnumInfCallback,
    IN     BOOL                         IgnoreNonCriticalErrors,
    IN     PSETUP_LOG_CONTEXT           LogContext,
    IN OUT PVOID                        Context
    );

DWORD
EnumInfsInDirectory(
    IN     PCTSTR                       DirPath,
    IN     PSP_ENUMINF_CALLBACK_ROUTINE EnumInfCallback,
    IN     BOOL                         IgnoreNonCriticalErrors,
    IN     PSETUP_LOG_CONTEXT           LogContext,
    IN OUT PVOID                        Context
    );

PTSTR
GetFullyQualifiedMultiSzPathList(
    IN PCTSTR PathList
    );

BOOL
ShouldClassBeExcluded(
    IN LPGUID ClassGuid,
    IN BOOL   ExcludeNoInstallClass
    );

BOOL
ClassGuidFromInfVersionNode(
    IN  PINF_VERSION_NODE VersionNode,
    OUT LPGUID            ClassGuid
    );

VOID
AppendLoadIncludedInfs(
    IN HINF   hDeviceInf,
    IN PCTSTR InfFileName,
    IN PCTSTR InfSectionName,
    IN BOOL   AppendLayoutInfs
    );

DWORD
InstallFromInfSectionAndNeededSections(
    IN HWND              Owner,             OPTIONAL
    IN HINF              InfHandle,
    IN PCTSTR            SectionName,
    IN UINT              Flags,
    IN HKEY              RelativeKeyRoot,   OPTIONAL
    IN PCTSTR            SourceRootPath,    OPTIONAL
    IN UINT              CopyFlags,
    IN PSP_FILE_CALLBACK MsgHandler,
    IN PVOID             Context,           OPTIONAL
    IN HDEVINFO          DeviceInfoSet,     OPTIONAL
    IN PSP_DEVINFO_DATA  DeviceInfoData,    OPTIONAL
    IN HSPFILEQ          UserFileQ          OPTIONAL
    );

DWORD
MarkQueueForDeviceInstall(
    IN HSPFILEQ QueueHandle,
    IN HINF     DeviceInfHandle,
    IN PCTSTR   DeviceDesc       OPTIONAL
    );


//
// Icon list manipulation functions.
//
BOOL
InitMiniIconList(
    VOID
    );

BOOL
DestroyMiniIconList(
    VOID
    );


//
// "Driver Search In-Progress" list functions.
//
BOOL
InitDrvSearchInProgressList(
    VOID
    );

BOOL
DestroyDrvSearchInProgressList(
    VOID
    );


//
// 'helper module' manipulation functions.
//
DWORD
GetModuleEntryPoint(
    IN  HKEY                  hk,                  OPTIONAL
    IN  LPCTSTR               RegistryValue,
    IN  LPCTSTR               DefaultProcName,
    OUT HINSTANCE            *phinst,
    OUT FARPROC              *pEntryPoint,
    OUT BOOL                 *pMustAbort,
    IN  PSETUP_LOG_CONTEXT    LogContext,
    IN  HWND                  Owner,
    IN  SetupapiVerifyProblem Problem,
    IN  LPCTSTR               DeviceDesc,          OPTIONAL
    IN  DWORD                 DriverSigningPolicy,
    IN  DWORD                 NoUI
    );

//
// Define flags for InvalidateHelperModules
//
#define IHM_COINSTALLERS_ONLY     0x00000001
#define IHM_FREE_IMMEDIATELY      0x00000002

DWORD
InvalidateHelperModules(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN DWORD            Flags
    );

//
// Define flags for _SetupDiCallClassInstaller
//
#define CALLCI_LOAD_HELPERS     0x00000001
#define CALLCI_CALL_HELPERS     0x00000002
#define CALLCI_ALLOW_DRVSIGN_UI 0x00000004

BOOL
_SetupDiCallClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,      OPTIONAL
    IN DWORD            Flags
    );


//
// OEM driver selection routines.
//
DWORD
SelectOEMDriver(
    IN HWND             hwndParent,     OPTIONAL
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN BOOL             IsWizard
    );


//
// Registry helper routines.
//
DWORD
pSetupDeleteDevRegKeys(
    IN DEVINST DevInst,
    IN DWORD   Scope,
    IN DWORD   HwProfile,
    IN DWORD   KeyType,
    IN BOOL    DeleteUserKeys
    );

VOID
GetRegSubkeysFromDeviceInterfaceName(
    IN OUT PTSTR  DeviceInterfaceName,
    OUT    PTSTR *SubKeyName
    );

LONG
OpenDeviceInterfaceSubKey(
    IN     HKEY   hKeyInterfaceClass,
    IN     PCTSTR DeviceInterfaceName,
    IN     REGSAM samDesired,
    OUT    PHKEY  phkResult,
    OUT    PTSTR  OwningDevInstName,    OPTIONAL
    IN OUT PDWORD OwningDevInstNameSize OPTIONAL
    );


//
// Guid table routines.
//
LONG
AddOrGetGuidTableIndex(
    IN PDEVICE_INFO_SET  DeviceInfoSet,
    IN CONST GUID       *ClassGuid,
    IN BOOL              AddIfNotPresent
    );


//
// Interface device routines.
//
PINTERFACE_CLASS_LIST
AddOrGetInterfaceClassList(
    IN PDEVICE_INFO_SET DeviceInfoSet,
    IN PDEVINFO_ELEM    DevInfoElem,
    IN LONG             InterfaceClassGuidIndex,
    IN BOOL             AddIfNotPresent
    );

BOOL
InterfaceDeviceDataFromNode(
    IN  PINTERFACE_DEVICE_NODE     InterfaceDeviceNode,
    IN  CONST GUID                *InterfaceClassGuid,
    OUT PSP_DEVICE_INTERFACE_DATA  InterfaceDeviceData
    );

PDEVINFO_ELEM
FindDevInfoElemForInterfaceDevice(
    IN PDEVICE_INFO_SET          DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA InterfaceDeviceData
    );


//
// Service installation routines.
//
typedef struct _SVCNAME_NODE {
    struct _SVCNAME_NODE *Next;
    TCHAR Name[MAX_SERVICE_NAME_LEN];
    BOOL DeleteEventLog;
    TCHAR EventLogType[256];
    TCHAR EventLogName[256];
    DWORD Flags;
} SVCNAME_NODE, *PSVCNAME_NODE;

//
// Define an additional (private) SPSVCINST flag for
// InstallNtService.
//
#define SPSVCINST_NO_DEVINST_CHECK  (0x80000000)

DWORD
InstallNtService(
    IN  PDEVINFO_ELEM    DevInfoElem,        OPTIONAL
    IN  HINF             hDeviceInf,
    IN  PCTSTR                   InfFileName,            OPTIONAL
    IN  PCTSTR           szSectionName,      OPTIONAL
    OUT PSVCNAME_NODE   *ServicesToDelete,   OPTIONAL
    IN  DWORD            Flags,
    OUT PBOOL            NullDriverInstalled
    );

//
// Ansi/Unicode conversion routines.
//
DWORD
pSetupDiDevInstParamsAnsiToUnicode(
    IN  PSP_DEVINSTALL_PARAMS_A AnsiDevInstParams,
    OUT PSP_DEVINSTALL_PARAMS_W UnicodeDevInstParams
    );

DWORD
pSetupDiDevInstParamsUnicodeToAnsi(
    IN  PSP_DEVINSTALL_PARAMS_W UnicodeDevInstParams,
    OUT PSP_DEVINSTALL_PARAMS_A AnsiDevInstParams
    );

DWORD
pSetupDiSelDevParamsAnsiToUnicode(
    IN  PSP_SELECTDEVICE_PARAMS_A AnsiSelDevParams,
    OUT PSP_SELECTDEVICE_PARAMS_W UnicodeSelDevParams
    );

DWORD
pSetupDiSelDevParamsUnicodeToAnsi(
    IN  PSP_SELECTDEVICE_PARAMS_W UnicodeSelDevParams,
    OUT PSP_SELECTDEVICE_PARAMS_A AnsiSelDevParams
    );

DWORD
pSetupDiDrvInfoDataAnsiToUnicode(
    IN  PSP_DRVINFO_DATA_A AnsiDrvInfoData,
    OUT PSP_DRVINFO_DATA_W UnicodeDrvInfoData
    );

DWORD
pSetupDiDrvInfoDataUnicodeToAnsi(
    IN  PSP_DRVINFO_DATA_W UnicodeDrvInfoData,
    OUT PSP_DRVINFO_DATA_A AnsiDrvInfoData
    );

DWORD
pSetupDiDevInfoSetDetailDataUnicodeToAnsi(
    IN  PSP_DEVINFO_LIST_DETAIL_DATA_W UnicodeDevInfoSetDetails,
    OUT PSP_DEVINFO_LIST_DETAIL_DATA_A AnsiDevInfoSetDetails
    );

//
// Misc. utility routines
//
DWORD
MapCrToSpError(
    IN CONFIGRET CmReturnCode,
    IN DWORD     Default
    );

VOID
SetDevnodeNeedsRebootProblemWithArg2(
    IN DEVINST  DevInst,
    IN HMACHINE hMachine,
    IN PSETUP_LOG_CONTEXT LogContext,
    IN DWORD    Reason,                  OPTIONAL
    IN ULONG_PTR Arg1,                   OPTIONAL
    IN ULONG_PTR Arg2                    OPTIONAL
    );

#define SetDevnodeNeedsRebootProblemWithArg(DevInst,hMachine,LogContext,Reason,Arg) SetDevnodeNeedsRebootProblemWithArg2(DevInst,hMachine,LogContext,Reason,Arg,0)
#define SetDevnodeNeedsRebootProblem(DevInst,hMachine,LogContext,Reason) SetDevnodeNeedsRebootProblemWithArg2(DevInst,hMachine,LogContext,Reason,0,0)

BOOL
GetBestDeviceDesc(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,  OPTIONAL
    OUT PTSTR            DeviceDescBuffer
    );

