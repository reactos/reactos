/*++

Copyright (c) 1993-1998 Microsoft Corporation

Module Name:

    diutil.c

Abstract:

    Device Installer utility routines.

Author:

    Lonny McMichael (lonnym) 10-May-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <initguid.h>

//
// Define and initialize all device class GUIDs.
// (This must only be done once per module!)
//
#include <devguid.h>

//
// Define and initialize a global variable, GUID_NULL
// (from coguid.h)
//
DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

//
// Define the period in miliseconds to wait between attempts to lock the SCM database
//
#define ACQUIRE_SCM_LOCK_INTERVAL 500

//
// Define the number of attempts at locking the SCM database should be made
//
#define ACQUIRE_SCM_LOCK_ATTEMPTS 5

//
// Declare global string variables used throughout device
// installer routines.
//
// These strings are defined in regstr.h:
//
CONST TCHAR pszNoUseClass[]                      = REGSTR_VAL_NOUSECLASS,
            pszNoInstallClass[]                  = REGSTR_VAL_NOINSTALLCLASS,
            pszNoDisplayClass[]                  = REGSTR_VAL_NODISPLAYCLASS,
            pszDeviceDesc[]                      = REGSTR_VAL_DEVDESC,
            pszDevicePath[]                      = REGSTR_VAL_DEVICEPATH,
            pszPathSetup[]                       = REGSTR_PATH_SETUP,
            pszKeySetup[]                        = REGSTR_KEY_SETUP,
            pszPathRunOnce[]                     = REGSTR_PATH_RUNONCE,
            pszSourcePath[]                      = REGSTR_VAL_SRCPATH,
            pszSvcPackPath[]                     = REGSTR_VAL_SVCPAKSRCPATH,
            pszDriverCachePath[]                 = REGSTR_VAL_DRIVERCACHEPATH,
            pszBootDir[]                         = REGSTR_VAL_BOOTDIR,
            pszInsIcon[]                         = REGSTR_VAL_INSICON,
            pszInstaller32[]                     = REGSTR_VAL_INSTALLER_32,
            pszEnumPropPages32[]                 = REGSTR_VAL_ENUMPROPPAGES_32,
            pszInfPath[]                         = REGSTR_VAL_INFPATH,
            pszInfSection[]                      = REGSTR_VAL_INFSECTION,
            pszDrvDesc[]                         = REGSTR_VAL_DRVDESC,
            pszHardwareID[]                      = REGSTR_VAL_HARDWAREID,
            pszCompatibleIDs[]                   = REGSTR_VAL_COMPATIBLEIDS,
            pszDriver[]                          = REGSTR_VAL_DRIVER,
            pszConfigFlags[]                     = REGSTR_VAL_CONFIGFLAGS,
            pszMfg[]                             = REGSTR_VAL_MFG,
            pszService[]                         = REGSTR_VAL_SERVICE,
            pszProviderName[]                    = REGSTR_VAL_PROVIDER_NAME,
            pszFriendlyName[]                    = REGSTR_VAL_FRIENDLYNAME,
            pszServicesRegPath[]                 = REGSTR_PATH_SERVICES,
            pszLegacyInfOption[]                 = REGSTR_VAL_LEGACYINFOPT,
            pszInfSectionExt[]                   = REGSTR_VAL_INFSECTIONEXT,
            pszDeviceClassesPath[]               = REGSTR_PATH_DEVICE_CLASSES,
            pszDeviceInstance[]                  = REGSTR_VAL_DEVICE_INSTANCE,
            pszDefault[]                         = REGSTR_VAL_DEFAULT,
            pszControl[]                         = REGSTR_KEY_CONTROL,
            pszLinked[]                          = REGSTR_VAL_LINKED,
            pszDeviceParameters[]                = REGSTR_KEY_DEVICEPARAMETERS,
            pszLocationInformation[]             = REGSTR_VAL_LOCATION_INFORMATION,
            pszCapabilities[]                    = REGSTR_VAL_CAPABILITIES,
            pszUiNumber[]                        = REGSTR_VAL_UI_NUMBER,
            pszUpperFilters[]                    = REGSTR_VAL_UPPERFILTERS,
            pszLowerFilters[]                    = REGSTR_VAL_LOWERFILTERS,
            pszMatchingDeviceId[]                = REGSTR_VAL_MATCHINGDEVID,
            pszBasicProperties32[]               = REGSTR_VAL_BASICPROPERTIES_32,
            pszCoInstallers32[]                  = REGSTR_VAL_COINSTALLERS_32,
            pszPathCoDeviceInstallers[]          = REGSTR_PATH_CODEVICEINSTALLERS,
            pszSystem[]                          = REGSTR_KEY_SYSTEM,
            pszDrvSignPath[]                     = REGSTR_PATH_DRIVERSIGN,
            pszNonDrvSignPath[]                  = REGSTR_PATH_NONDRIVERSIGN,
            pszDrvSignPolicyPath[]               = REGSTR_PATH_DRIVERSIGN_POLICY,
            pszNonDrvSignPolicyPath[]            = REGSTR_PATH_NONDRIVERSIGN_POLICY,
            pszDrvSignPolicyValue[]              = REGSTR_VAL_POLICY,
            pszDrvSignBehaviorOnFailedVerifyDS[] = REGSTR_VAL_BEHAVIOR_ON_FAILED_VERIFY,
            pszDriverDate[]                      = REGSTR_VAL_DRIVERDATE,
            pszDriverDateData[]                  = REGSTR_VAL_DRIVERDATEDATA,
            pszDriverVersion[]                   = REGSTR_VAL_DRIVERVERSION,
            pszDevSecurity[]                     = REGSTR_VAL_DEVICE_SECURITY_DESCRIPTOR,
            pszDevType[]                         = REGSTR_VAL_DEVICE_TYPE,
            pszExclusive[]                       = REGSTR_VAL_DEVICE_EXCLUSIVE,
            pszCharacteristics[]                 = REGSTR_VAL_DEVICE_CHARACTERISTICS,
            pszUiNumberDescFormat[]              = REGSTR_VAL_UI_NUMBER_DESC_FORMAT;


//
// Other misc. global strings (defined in devinst.h):
//
CONST TCHAR pszInfWildcard[]              = DISTR_INF_WILDCARD,
            pszOemInfWildcard[]           = DISTR_OEMINF_WILDCARD,
            pszCiDefaultProc[]            = DISTR_CI_DEFAULTPROC,
            pszSpaceLparen[]              = DISTR_SPACE_LPAREN,
            pszRparen[]                   = DISTR_RPAREN,
            pszUniqueSubKey[]             = DISTR_UNIQUE_SUBKEY,
            pszOemInfGenerate[]           = DISTR_OEMINF_GENERATE,
            pszOemInfDefaultPath[]        = DISTR_OEMINF_DEFAULTPATH,
            pszDefaultService[]           = DISTR_DEFAULT_SERVICE,
            pszGuidNull[]                 = DISTR_GUID_NULL,
            pszEventLog[]                 = DISTR_EVENTLOG,
            pszGroupOrderListPath[]       = DISTR_GROUPORDERLIST_PATH,
            pszServiceGroupOrderPath[]    = DISTR_SERVICEGROUPORDER_PATH,
            pszOptions[]                  = DISTR_OPTIONS,
            pszOptionsText[]              = DISTR_OPTIONSTEXT,
            pszLanguagesSupported[]       = DISTR_LANGUAGESSUPPORTED,
            pszRunOnceExe[]               = DISTR_RUNONCE_EXE,
            pszGrpConv[]                  = DISTR_GRPCONV,
            pszGrpConvNoUi[]              = DISTR_GRPCONV_NOUI,
            pszDefaultSystemPartition[]   = DISTR_DEFAULT_SYSPART,
            pszBasicPropDefaultProc[]     = DISTR_BASICPROP_DEFAULTPROC,
            pszEnumPropDefaultProc[]      = DISTR_ENUMPROP_DEFAULTPROC,
            pszCoInstallerDefaultProc[]   = DISTR_CODEVICEINSTALL_DEFAULTPROC,
            pszDriverObjectPathPrefix[]   = DISTR_DRIVER_OBJECT_PATH_PREFIX,
            pszDriverSigningClasses[]     = DISTR_DRIVER_SIGNING_CLASSES;


//
// Define flag bitmask indicating which flags are controlled internally by the
// device installer routines, and thus are read-only to clients.
//
#define DI_FLAGS_READONLY    ( DI_DIDCOMPAT | DI_DIDCLASS | DI_MULTMFGS )

#define DI_FLAGSEX_READONLY  (  DI_FLAGSEX_DIDINFOLIST     \
                              | DI_FLAGSEX_DIDCOMPATINFO   \
                              | DI_FLAGSEX_IN_SYSTEM_SETUP )

#define DNF_FLAGS_READONLY   (  DNF_DUPDESC           \
                              | DNF_OLDDRIVER         \
                              | DNF_LEGACYINF         \
                              | DNF_CLASS_DRIVER      \
                              | DNF_COMPATIBLE_DRIVER \
                              | DNF_INET_DRIVER       \
                              | DNF_INDEXED_DRIVER    \
                              | DNF_OLD_INET_DRIVER   \
                              | DNF_DUPPROVIDER       )

//
// Define flag bitmask indicating which flags are illegal.
//
#define DI_FLAGS_ILLEGAL    ( 0x00400000L )  // setupx DI_NOSYNCPROCESSING flag
#define DI_FLAGSEX_ILLEGAL  ( 0xFF000008L )  // all flags not currently defined
#define DNF_FLAGS_ILLEGAL   ( 0xFFFFF500L )  // ""

#define NDW_INSTALLFLAG_ILLEGAL (~( NDW_INSTALLFLAG_DIDFACTDEFS        \
                                  | NDW_INSTALLFLAG_HARDWAREALLREADYIN \
                                  | NDW_INSTALLFLAG_NEEDRESTART        \
                                  | NDW_INSTALLFLAG_NEEDREBOOT         \
                                  | NDW_INSTALLFLAG_NEEDSHUTDOWN       \
                                  | NDW_INSTALLFLAG_EXPRESSINTRO       \
                                  | NDW_INSTALLFLAG_SKIPISDEVINSTALLED \
                                  | NDW_INSTALLFLAG_NODETECTEDDEVS     \
                                  | NDW_INSTALLFLAG_INSTALLSPECIFIC    \
                                  | NDW_INSTALLFLAG_SKIPCLASSLIST      \
                                  | NDW_INSTALLFLAG_CI_PICKED_OEM      \
                                  | NDW_INSTALLFLAG_PCMCIAMODE         \
                                  | NDW_INSTALLFLAG_PCMCIADEVICE       \
                                  | NDW_INSTALLFLAG_USERCANCEL         \
                                  | NDW_INSTALLFLAG_KNOWNCLASS         ))

#define DYNAWIZ_FLAG_ILLEGAL (~( DYNAWIZ_FLAG_PAGESADDED             \
                               | DYNAWIZ_FLAG_INSTALLDET_NEXT        \
                               | DYNAWIZ_FLAG_INSTALLDET_PREV        \
                               | DYNAWIZ_FLAG_ANALYZE_HANDLECONFLICT ))

#define NEWDEVICEWIZARD_FLAG_ILLEGAL (~(0)) // no flags are legal presently


//
// Declare data used in GUID->string conversion (from ole32\common\ccompapi.cxx).
//
static const BYTE GuidMap[] = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
                                8, 9, '-', 10, 11, 12, 13, 14, 15 };

static const TCHAR szDigits[] = TEXT("0123456789ABCDEF");


PDEVICE_INFO_SET
AllocateDeviceInfoSet(
    VOID
    )
/*++

Routine Description:

    This routine allocates a device information set structure, zeroes it,
    and initializes the synchronization lock for it.

Arguments:

    none.

Return Value:

    If the function succeeds, the return value is a pointer to the new
    device information set.

    If the function fails, the return value is NULL.

--*/
{
    PDEVICE_INFO_SET p;

    if(p = MyMalloc(sizeof(DEVICE_INFO_SET))) {

        ZeroMemory(p, sizeof(DEVICE_INFO_SET));

        p->MachineName = -1;
        p->InstallParamBlock.DriverPath = -1;
        p->InstallParamBlock.CoInstallerCount = -1;

        //
        // If we're in GUI-mode setup on Windows NT, we'll automatically set
        // the DI_FLAGSEX_IN_SYSTEM_SETUP flag in the devinstall parameter
        // block for this devinfo set.
        //
        if(GuiSetupInProgress) {
            p->InstallParamBlock.FlagsEx |= DI_FLAGSEX_IN_SYSTEM_SETUP;
        }

        //
        // If we're in non-interactive mode, set the "be quiet" bits.
        //
        if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
            p->InstallParamBlock.Flags   |= DI_QUIETINSTALL;
            p->InstallParamBlock.FlagsEx |= DI_FLAGSEX_NOUIONQUERYREMOVE;
        }

        //
        // Initialize our enumeration 'hints'
        //
        p->DeviceInfoEnumHintIndex = INVALID_ENUM_INDEX;
        p->ClassDriverEnumHintIndex = INVALID_ENUM_INDEX;


        if(p->StringTable = pStringTableInitialize(0)) {

            if (CreateLogContext(NULL, &(p->InstallParamBlock.LogContext)) == NO_ERROR) {
                //
                // succeeded
                //
                if(InitializeSynchronizedAccess(&(p->Lock))) {
                    return p;
                }

                DeleteLogContext(p->InstallParamBlock.LogContext);
            }

            pStringTableDestroy(p->StringTable);
        }
        MyFree(p);
    }

    return NULL;
}


VOID
DestroyDeviceInfoElement(
    IN HDEVINFO         hDevInfo,
    IN PDEVICE_INFO_SET pDeviceInfoSet,
    IN PDEVINFO_ELEM    DeviceInfoElement
    )
/*++

Routine Description:

    This routine destroys the specified device information element, freeing
    all resources associated with it.
    ASSUMES THAT THE CALLING ROUTINE HAS ALREADY ACQUIRED THE LOCK!

Arguments:

    hDevInfo - Supplies a handle to the device information set whose internal
        representation is given by pDeviceInfoSet.  This opaque handle is
        actually the same pointer as pDeviceInfoSet, but we want to keep this
        distinction clean, so that in the future we can change our implementation
        (e.g., hDevInfo might represent an offset in an array of DEVICE_INFO_SET
        elements).

    pDeviceInfoSet - Supplies a pointer to the device information set of which
        the devinfo element is a member.  This set contains the class driver list
        object list that must be used in destroying the class driver list.

    DeviceInfoElement - Supplies a pointer to the device information element
        to be destroyed.

Return Value:

    None.

--*/
{
    DWORD i;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode, NextInterfaceDeviceNode;
    CONFIGRET cr;

    MYASSERT(hDevInfo && (hDevInfo != INVALID_HANDLE_VALUE));

    //
    // Free resources contained in the install parameters block.  Do this
    // before anything else, because we'll be calling the class installer
    // with DIF_DESTROYPRIVATEDATA, and we want everything to be in a
    // consistent state when we do (plus, it may need to destroy private
    // data it's stored with individual driver nodes).
    //
    DestroyInstallParamBlock(hDevInfo,
                             pDeviceInfoSet,
                             DeviceInfoElement,
                             &(DeviceInfoElement->InstallParamBlock)
                            );

    //
    // Dereference the class driver list.
    //
    DereferenceClassDriverList(pDeviceInfoSet, DeviceInfoElement->ClassDriverHead);

    //
    // Destroy compatible driver list.
    //
    DestroyDriverNodes(DeviceInfoElement->CompatDriverHead, pDeviceInfoSet);

    //
    // If this is a non-registered device instance, then delete any registry
    // keys the caller may have created during the lifetime of this element.
    //
    if(DeviceInfoElement->DevInst && !(DeviceInfoElement->DiElemFlags & DIE_IS_REGISTERED)) {

        pSetupDeleteDevRegKeys(DeviceInfoElement->DevInst,
                               DICS_FLAG_GLOBAL | DICS_FLAG_CONFIGSPECIFIC,
                               (DWORD)-1,
                               DIREG_BOTH,
                               TRUE
                              );

        //
        // BugBug (jamiehun) not .... DevInst_Ex ???
        //
        cr = CM_Uninstall_DevInst(DeviceInfoElement->DevInst, 0);
        if (cr != CR_SUCCESS) {
            //
            // BugBug (jamiehun) anything we should do here?
            //
        }
    }

    //
    // Free any interface device lists that may be associated with this devinfo element.
    //
    if(DeviceInfoElement->InterfaceClassList) {

        for(i = 0; i < DeviceInfoElement->InterfaceClassListSize; i++) {

            for(InterfaceDeviceNode = DeviceInfoElement->InterfaceClassList[i].InterfaceDeviceNode;
                InterfaceDeviceNode;
                InterfaceDeviceNode = NextInterfaceDeviceNode) {

                NextInterfaceDeviceNode = InterfaceDeviceNode->Next;
                MyFree(InterfaceDeviceNode);
            }
        }

        MyFree(DeviceInfoElement->InterfaceClassList);
    }

    //
    // Zero the signature field containing the address of the containing devinfo
    // set.  This will keep us thinking an SP_DEVINFO_DATA is still valid after
    // the underlying element has been deleted.
    //
    DeviceInfoElement->ContainingDeviceInfoSet = NULL;

    MyFree(DeviceInfoElement);
}


DWORD
DestroyDeviceInfoSet(
    IN HDEVINFO         hDevInfo,      OPTIONAL
    IN PDEVICE_INFO_SET pDeviceInfoSet
    )
/*++

Routine Description:

    This routine frees a device information set, and all resources
    used on its behalf.

Arguments:

    hDevInfo - Optionally, supplies a handle to the device information set
        whose internal representation is given by pDeviceInfoSet.  This
        opaque handle is actually the same pointer as pDeviceInfoSet, but
        we want to keep this distinction clean, so that in the future we
        can change our implementation (e.g., hDevInfo might represent an
        offset in an array of DEVICE_INFO_SET elements).

        This parameter will only be NULL if we're cleaning up half-way
        through the creation of a device information set.

    pDeviceInfoSet - supplies a pointer to the device information set
        to be freed.

Return Value:

    If successful, the return code is NO_ERROR, otherwise, it is an
    ERROR_* code.

--*/
{
    PDEVINFO_ELEM NextElem;
    PDRIVER_NODE DriverNode, NextNode;
    PMODULE_HANDLE_LIST_NODE NextModuleHandleNode;
    DWORD i;

    //
    // We have to make sure that the wizard refcount is zero, and that
    // we haven't acquired the lock more than once (i.e., we're nested
    // more than one level deep in Di calls.
    //
    if(pDeviceInfoSet->WizPageList ||
       (pDeviceInfoSet->LockRefCount > 1)) {

        return ERROR_DEVINFO_LIST_LOCKED;
    }

    //
    // Destroy all the device information elements in this set.  Make sure
    // that we maintain consistency while removing devinfo elements, because
    // we may be calling the class installer.  This means that the device
    // installer APIs still have to work, even while we're tearing down the
    // list.
    //
    while(pDeviceInfoSet->DeviceInfoHead) {
        //
        // We'd better not have any device info elements locked by wizard
        // pages, since our wizard refcount is zero!
        //
        MYASSERT(!(pDeviceInfoSet->DeviceInfoHead->DiElemFlags & DIE_IS_LOCKED));

        NextElem = pDeviceInfoSet->DeviceInfoHead->Next;
        DestroyDeviceInfoElement(hDevInfo, pDeviceInfoSet, pDeviceInfoSet->DeviceInfoHead);

        MYASSERT(pDeviceInfoSet->DeviceInfoCount > 0);
        pDeviceInfoSet->DeviceInfoCount--;

        //
        // If this element was the currently selected device for this
        // set, then reset the device selection.
        //
        if(pDeviceInfoSet->SelectedDevInfoElem == pDeviceInfoSet->DeviceInfoHead) {
            pDeviceInfoSet->SelectedDevInfoElem = NULL;
        }

        pDeviceInfoSet->DeviceInfoHead = NextElem;
    }

    MYASSERT(pDeviceInfoSet->DeviceInfoCount == 0);
    pDeviceInfoSet->DeviceInfoTail = NULL;

    //
    // Free resources contained in the install parameters block.  Do this
    // before anything else, because we'll be calling the class installer
    // with DIF_DESTROYPRIVATEDATA, and we want everything to be in a
    // consistent state when we do (plus, it may need to destroy private
    // data it's stored with individual driver nodes).
    //
    DestroyInstallParamBlock(hDevInfo,
                             pDeviceInfoSet,
                             NULL,
                             &(pDeviceInfoSet->InstallParamBlock)
                            );

    //
    // Destroy class driver list.
    //
    if(pDeviceInfoSet->ClassDriverHead) {
        //
        // We've already destroyed all device information elements, so there should be
        // exactly one driver list object remaining--the one referenced by the global
        // class driver list.  Also, it's refcount should be 1.
        //
        MYASSERT(
            (pDeviceInfoSet->ClassDrvListObjectList) &&
            (!pDeviceInfoSet->ClassDrvListObjectList->Next) &&
            (pDeviceInfoSet->ClassDrvListObjectList->RefCount == 1) &&
            (pDeviceInfoSet->ClassDrvListObjectList->DriverListHead == pDeviceInfoSet->ClassDriverHead)
           );

        MyFree(pDeviceInfoSet->ClassDrvListObjectList);
        DestroyDriverNodes(pDeviceInfoSet->ClassDriverHead, pDeviceInfoSet);
    }

    //
    // Free the interface class GUID list (if there is one).
    //
    if(pDeviceInfoSet->GuidTable) {
        MyFree(pDeviceInfoSet->GuidTable);
    }

    //
    // Destroy the associated string table.
    //
    pStringTableDestroy(pDeviceInfoSet->StringTable);

    //
    // Destroy the lock (we have to do this after we've made all necessary calls
    // to the class installer, because after the lock is freed, the HDEVINFO set
    // is inaccessible).
    //
    DestroySynchronizedAccess(&(pDeviceInfoSet->Lock));

    //
    // If there are any module handles left to be freed, do that now.
    //
    for(; pDeviceInfoSet->ModulesToFree; pDeviceInfoSet->ModulesToFree = NextModuleHandleNode) {

        NextModuleHandleNode = pDeviceInfoSet->ModulesToFree->Next;

        for(i = 0; i < pDeviceInfoSet->ModulesToFree->ModuleCount; i++) {
            MYASSERT(pDeviceInfoSet->ModulesToFree->ModuleList[i]);
            FreeLibrary(pDeviceInfoSet->ModulesToFree->ModuleList[i]);
        }

        MyFree(pDeviceInfoSet->ModulesToFree);
    }

    //
    // If this is a remote HDEVINFO set, then disconnect from the remote machine.
    //
    if(pDeviceInfoSet->hMachine) {
        CM_Disconnect_Machine(pDeviceInfoSet->hMachine);
    }

    //
    // Now, destroy the container itself.
    //
    MyFree(pDeviceInfoSet);

    return NO_ERROR;
}


VOID
DestroyInstallParamBlock(
    IN HDEVINFO                hDevInfo,         OPTIONAL
    IN PDEVICE_INFO_SET        pDeviceInfoSet,
    IN PDEVINFO_ELEM           DevInfoElem,      OPTIONAL
    IN PDEVINSTALL_PARAM_BLOCK InstallParamBlock
    )
/*++

Routine Description:

    This routine frees any resources contained in the specified install
    parameter block.  THE BLOCK ITSELF IS NOT FREED!

Arguments:

    hDevInfo - Optionally, supplies a handle to the device information set
        containing the element whose parameter block is to be destroyed.

        If this parameter is not supplied, then we're cleaning up after
        failing part-way through a SetupDiCreateDeviceInfoList.

    pDeviceInfoSet - Supplies a pointer to the device information set of which
        the devinfo element is a member.

    DevInfoElem - Optionally, supplies the address of the device information
        element whose parameter block is to be destroyed.  If the parameter
        block being destroyed is associated with the set itself, then this
        parameter will be NULL.

    InstallParamBlock - Supplies the address of the install parameter
        block whose resources are to be freed.

Return Value:

    None.

--*/
{
    SP_DEVINFO_DATA DeviceInfoData;
    LONG i;

    if(InstallParamBlock->UserFileQ) {
        //
        // If there's a user-supplied file queue stored in this installation
        // parameter block, then decrement the refcount on it.  Make sure we
        // do this before calling the class installer with DIF_DESTROYPRIVATEDATA,
        // or else they won't be able to close the queue.
        //
        MYASSERT(((PSP_FILE_QUEUE)(InstallParamBlock->UserFileQ))->LockRefCount);

        ((PSP_FILE_QUEUE)(InstallParamBlock->UserFileQ))->LockRefCount--;
    }

    if(hDevInfo && (hDevInfo != INVALID_HANDLE_VALUE)) {
        //
        // Call the class installer/co-installers (if there are any) to let them
        // clean up any private data they may have.
        //
        if(DevInfoElem) {
            //
            // Generate an SP_DEVINFO_DATA structure from our device information
            // element (if we have one).
            //
            DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                             DevInfoElem,
                                             &DeviceInfoData
                                            );
        }

        InvalidateHelperModules(hDevInfo,
                                (DevInfoElem ? &DeviceInfoData : NULL),
                                IHM_FREE_IMMEDIATELY
                               );
    }

    if(InstallParamBlock->ClassInstallHeader) {
        MyFree(InstallParamBlock->ClassInstallHeader);
    }

    //
    // Get rid of the log context sitting in here.
    //
    DeleteLogContext(InstallParamBlock->LogContext);
}


PDEVICE_INFO_SET
AccessDeviceInfoSet(
    IN HDEVINFO DeviceInfoSet
    )
/*++

Routine Description:

    This routine locks the specified device information set, and returns
    a pointer to the structure for its internal representation.  It also
    increments the lock refcount on this set, so that it can't be destroyed
    if the lock has been acquired multiple times.

    After access to the set is completed, the caller must call
    UnlockDeviceInfoSet with the pointer returned by this function.

Arguments:

    DeviceInfoSet - Supplies the pointer to the device information set
        to be accessed.

Return Value:

    If the function succeeds, the return value is a pointer to the
    device information set.

    If the function fails, the return value is NULL.

Remarks:

    If the method for accessing a device information set's internal
    representation via its handle changes (e.g., instead of a pointer, it's an
    index into a table), then RollbackDeviceInfoSet and CommitDeviceInfoSet
    must also be changed.  Also, we cast an HDEVINFO to a PDEVICE_INFO_SET
    when specifying the containing device information set in a call to
    pSetupOpenAndAddNewDevInfoElem in devinfo.c!SetupDiGetClassDevsEx (only
    when we're working with a cloned devinfo set).

--*/
{
    PDEVICE_INFO_SET p;

    try {
        p = (PDEVICE_INFO_SET)DeviceInfoSet;
        if(LockDeviceInfoSet(p)) {
            p->LockRefCount++;
        } else {
            p = NULL;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        p = NULL;
    }

    return p;
}


PDEVICE_INFO_SET
CloneDeviceInfoSet(
    IN HDEVINFO hDevInfo
    )
/*++

Routine Description:

    This routine locks the specified device information set, then returns a
    clone of the structure used for its internal representation.  Device
    information elements or device interface nodes may subsequently be added to
    this cloned devinfo set, and the results can be committed via
    CommitDeviceInfoSet.  If the changes must be backed out (e.g., because an
    error was encountered while adding the additional elements to the set), the
    routine RollbackDeviceInfoSet must be called.

    After access to the set is completed (and the changes have either been
    committed or rolled back per the discussion above), the caller must call
    UnlockDeviceInfoSet with the pointer returned by CommitDeviceInfoSet or
    RollbackDeviceInfoSet.

Arguments:

    hDevInfo - Supplies the handle of the device information set to be cloned.

Return Value:

    If the function succeeds, the return value is a pointer to the
    device information set.

    If the function fails, the return value is NULL.

Remarks:

    The device information set handle specified to this routine MUST NOT BE
    USED until the changes are either committed or rolled back.  Also, the
    PDEVICE_INFO_SET returned by this routine must not be treated like an
    HDEVINFO handle--it is not.

--*/
{
    PDEVICE_INFO_SET p, NewDevInfoSet;
    BOOL b;
    PVOID StringTable;

    try {
        p = (PDEVICE_INFO_SET)hDevInfo;
        if(LockDeviceInfoSet(p)) {
            p->LockRefCount++;
        } else {
            p = NULL;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        p = NULL;
    }

    if(!p) {
        return NULL;
    }

    NewDevInfoSet = NULL;
    StringTable = NULL;
    b = FALSE;

    try {
        //
        // OK, we successfully locked the device information set.  Now, make a copy
        // of the internal structure to return to the caller.
        //
        NewDevInfoSet = MyMalloc(sizeof(DEVICE_INFO_SET));
        if(!NewDevInfoSet) {
            goto clean0;
        }

        CopyMemory(NewDevInfoSet, p, sizeof(DEVICE_INFO_SET));

        //
        // Duplicate the string table contained in this device information set.
        //
        StringTable = pStringTableDuplicate(p->StringTable);
        if(!StringTable) {
            goto clean0;
        }

        NewDevInfoSet->StringTable = StringTable;

        //
        // We've successfully cloned the device information set!
        //
        b = TRUE;

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Access the following variables so the compiler will respect our
        // statement ordering w.r.t. assignment.
        //
        NewDevInfoSet = NewDevInfoSet;
        StringTable = StringTable;
    }

    if(!b) {
        //
        // We failed to make a copy of the device information set--free any
        // memory we may have allocated, and unlock the original devinfo set
        // before returning failure.
        //
        if(NewDevInfoSet) {
            MyFree(NewDevInfoSet);
        }
        if(StringTable) {
            pStringTableDestroy(StringTable);
        }
        UnlockDeviceInfoSet(p);
        return NULL;
    }

    return NewDevInfoSet;
}


PDEVICE_INFO_SET
RollbackDeviceInfoSet(
    IN HDEVINFO hDevInfo,
    IN PDEVICE_INFO_SET ClonedDeviceInfoSet
    )
/*++

Routine Description:

    This routine rolls back the specified hDevInfo to a known good state that
    was saved when the set was cloned via a prior call to CloneDeviceInfoSet.

Arguments:

    hDevInfo - Supplies the handle of the device information set to be rolled
        back.

    ClonedDeviceInfoSet - Supplies the address of the internal structure
        representing the hDevInfo set's cloned (and potentially, modified)
        information.  Upon successful return, this structure will be freed.

Return Value:

    If the function succeeds, the return value is a pointer to the rolled-back
    device information set structure.

    If the function fails, the return value is NULL.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem, NextDevInfoElem;
    DWORD i, InterfaceDeviceCount;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode, NextInterfaceDeviceNode;

    //
    // Retrieve a pointer to the hDevInfo set's internal representation (we
    // don't need to acquire the lock, because we did that when we cloned the
    // originally cloned the structure).
    //
    // NOTE:  If the method for accessing an HDEVINFO set's internal
    // representation ever changes (i.e., the AccessDeviceInfoSet routine),
    // then this code will need to be modified accordingly.
    //
    pDeviceInfoSet = (PDEVICE_INFO_SET)hDevInfo;

    //
    // Make sure no additional locks have been acquired against the cloned
    // DEVICE_INFO_SET.
    //
    MYASSERT(pDeviceInfoSet->LockRefCount == ClonedDeviceInfoSet->LockRefCount);

    //
    // Do some validation to see whether it looks like only new device
    // information elements were added onto the end of our existing list (i.e.,
    // it's invalid to add new elements within the existing list, or to remove
    // elements from the existing list).
    //
#if ASSERTS_ON
    if(pDeviceInfoSet->DeviceInfoHead) {

        DWORD DevInfoElemCount = 1;

        MYASSERT(pDeviceInfoSet->DeviceInfoHead == ClonedDeviceInfoSet->DeviceInfoHead);
        for(DevInfoElem = ClonedDeviceInfoSet->DeviceInfoHead;
            DevInfoElem->Next;
            DevInfoElem = DevInfoElem->Next, DevInfoElemCount++) {

            if(DevInfoElem == pDeviceInfoSet->DeviceInfoTail) {
                break;
            }
        }
        //
        // Did we find the original tail?
        //
        MYASSERT(DevInfoElem == pDeviceInfoSet->DeviceInfoTail);
        //
        // And did we traverse the same number of nodes in getting there that
        // was in the original list?
        //
        MYASSERT(DevInfoElemCount == pDeviceInfoSet->DeviceInfoCount);
    }
#endif

    //
    // Destroy any newly-added members of the device information element list.
    //
    for(DevInfoElem = (pDeviceInfoSet->DeviceInfoTail ? pDeviceInfoSet->DeviceInfoTail->Next : pDeviceInfoSet->DeviceInfoHead);
        DevInfoElem;
        DevInfoElem = NextDevInfoElem) {

        NextDevInfoElem = DevInfoElem->Next;

        MYASSERT(!DevInfoElem->ClassDriverCount);
        MYASSERT(!DevInfoElem->CompatDriverCount);

        //
        // Free any interface device lists that may be associated with this
        // devinfo element.
        //
        if(DevInfoElem->InterfaceClassList) {

            for(i = 0; i < DevInfoElem->InterfaceClassListSize; i++) {

                for(InterfaceDeviceNode = DevInfoElem->InterfaceClassList[i].InterfaceDeviceNode;
                    InterfaceDeviceNode;
                    InterfaceDeviceNode = NextInterfaceDeviceNode) {

                    NextInterfaceDeviceNode = InterfaceDeviceNode->Next;
                    MyFree(InterfaceDeviceNode);
                }
            }

            MyFree(DevInfoElem->InterfaceClassList);
        }

        MyFree(DevInfoElem);
    }

    if(pDeviceInfoSet->DeviceInfoTail) {
        pDeviceInfoSet->DeviceInfoTail->Next = NULL;
    }

    //
    // At this point, we've trimmed our device information element list back to
    // what it was prior to the cloning of the device information set.  However,
    // we may have added new device interface nodes onto the interface class
    // lists of existing devinfo elements.  Go and truncate any such nodes.
    //
    for(DevInfoElem = pDeviceInfoSet->DeviceInfoHead;
        DevInfoElem;
        DevInfoElem = DevInfoElem->Next) {

        if(DevInfoElem->InterfaceClassList) {

            for(i = 0; i < DevInfoElem->InterfaceClassListSize; i++) {

                if(DevInfoElem->InterfaceClassList[i].InterfaceDeviceTruncateNode) {
                    //
                    // One or more device interface nodes were added to this
                    // list.  Find the tail of the list as it existed prior to
                    // cloning, and truncate from there.
                    //
                    InterfaceDeviceNode = NULL;
                    InterfaceDeviceCount = 0;
                    for(NextInterfaceDeviceNode = DevInfoElem->InterfaceClassList[i].InterfaceDeviceNode;
                        NextInterfaceDeviceNode;
                        InterfaceDeviceNode = NextInterfaceDeviceNode, NextInterfaceDeviceNode = NextInterfaceDeviceNode->Next) {

                        if(NextInterfaceDeviceNode == DevInfoElem->InterfaceClassList[i].InterfaceDeviceTruncateNode) {
                            break;
                        }

                        //
                        // We haven't encountered the truncate point yet--
                        // increment the count of device interface nodes we've
                        // traversed so far.
                        //
                        InterfaceDeviceCount++;
                    }

                    //
                    // We'd better have found the node to truncate in our list!
                    //
                    MYASSERT(NextInterfaceDeviceNode);

                    //
                    // Truncate the list, and destroy all newly-added device
                    // interface nodes.
                    //
                    if(InterfaceDeviceNode) {
                        InterfaceDeviceNode->Next = NULL;
                    } else {
                        DevInfoElem->InterfaceClassList[i].InterfaceDeviceNode = NULL;
                    }
                    DevInfoElem->InterfaceClassList[i].InterfaceDeviceCount = InterfaceDeviceCount;

                    for(InterfaceDeviceNode = NextInterfaceDeviceNode;
                        InterfaceDeviceNode;
                        InterfaceDeviceNode = NextInterfaceDeviceNode) {

                        NextInterfaceDeviceNode = InterfaceDeviceNode->Next;
                        MyFree(InterfaceDeviceNode);
                    }

                    //
                    // Reset the truncate node pointer.
                    //
                    DevInfoElem->InterfaceClassList[i].InterfaceDeviceTruncateNode = NULL;
                }
            }
        }
    }

    //
    // OK, our device information element list and device interface node lists
    // are exactly as they were before the cloning took place.  However, it's
    // possible that we allocated (or reallocated) a new buffer for our
    // GUID table, so we need to update that GUID table pointer and size in our
    // original device information set structure.
    //
    pDeviceInfoSet->GuidTable     = ClonedDeviceInfoSet->GuidTable;
    pDeviceInfoSet->GuidTableSize = ClonedDeviceInfoSet->GuidTableSize;

    //
    // The device information set has been successfully rolled back.  Free the
    // memory associated with the clone.
    //
    pStringTableDestroy(ClonedDeviceInfoSet->StringTable);
    MyFree(ClonedDeviceInfoSet);

    //
    // Return the original (rolled-back) device information set structure to
    // the caller.
    //
    return pDeviceInfoSet;
}


PDEVICE_INFO_SET
CommitDeviceInfoSet(
    IN HDEVINFO hDevInfo,
    IN PDEVICE_INFO_SET ClonedDeviceInfoSet
    )
/*++

Routine Description:

    This routine commits the changes that have been made to a cloned device
    information set.  The clone was generated via a prior call to
    CloneDeviceInfoSet.

Arguments:

    hDevInfo - Supplies the handle of the device information set whose changes
        are to be committed.

    ClonedDeviceInfoSet - Supplies the address of the internal structure
        representing the hDevInfo set's cloned (and potentially, modified)
        information.  Upon successful return, this structure will be freed.

Return Value:

    If the function succeeds, the return value is a pointer to the committed
    device information set structure.

    If the function fails, the return value is NULL.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    DWORD i;

    //
    // Retrieve a pointer to the hDevInfo set's internal representation (we
    // don't need to acquire the lock, because we did that when we cloned the
    // originally cloned the structure).
    //
    // NOTE:  If the method for accessing an HDEVINFO set's internal
    // representation ever changes (i.e., the AccessDeviceInfoSet routine),
    // then this code will need to be modified accordingly.
    //
    pDeviceInfoSet = (PDEVICE_INFO_SET)hDevInfo;

    //
    // Make sure no additional locks have been acquired against the cloned
    // DEVICE_INFO_SET.
    //
    MYASSERT(pDeviceInfoSet->LockRefCount == ClonedDeviceInfoSet->LockRefCount);

    //
    // Free the old string table.
    //
    pStringTableDestroy(pDeviceInfoSet->StringTable);

    //
    // Now copy the cloned device information set structure into the 'real' one.
    //
    CopyMemory(pDeviceInfoSet, ClonedDeviceInfoSet, sizeof(DEVICE_INFO_SET));

    //
    // Now we have to go through each device information element's interface
    // class list and reset the InterfaceDeviceTruncateNode fields to indicate
    // that all the new device interface nodes that were added have been
    // committed.
    //
    for(DevInfoElem = pDeviceInfoSet->DeviceInfoHead;
        DevInfoElem;
        DevInfoElem = DevInfoElem->Next) {

        for(i = 0; i < DevInfoElem->InterfaceClassListSize; i++) {
            DevInfoElem->InterfaceClassList[i].InterfaceDeviceTruncateNode = NULL;
        }
    }

    //
    // Free the cloned device information set structure.
    //
    MyFree(ClonedDeviceInfoSet);

    //
    // We've successfully committed the changes into the original device
    // information set structure--return that structure.
    //
    return pDeviceInfoSet;
}


PDEVINFO_ELEM
FindDevInfoByDevInst(
    IN  PDEVICE_INFO_SET  DeviceInfoSet,
    IN  DEVINST           DevInst,
    OUT PDEVINFO_ELEM    *PrevDevInfoElem OPTIONAL
    )
/*++

Routine Description:

    This routine searches through all (registered) elements of a
    device information set, looking for one that corresponds to the
    specified device instance handle.  If a match is found, a pointer
    to the device information element is returned.

Arguments:

    DeviceInfoSet - Specifies the set to be searched.

    DevInst - Specifies the device instance handle to search for.

    PrevDevInfoElem - Optionaly, supplies the address of the variable that
        receives a pointer to the device information element immediately
        preceding the matching element.  If the element was found at the
        front of the list, then this variable will be set to NULL.

Return Value:

    If a device information element is found, the return value is a
    pointer to that element, otherwise, the return value is NULL.

--*/
{
    PDEVINFO_ELEM cur, prev;

    for(cur = DeviceInfoSet->DeviceInfoHead, prev = NULL;
        cur;
        prev = cur, cur = cur->Next)
    {
        if((cur->DiElemFlags & DIE_IS_REGISTERED) && (cur->DevInst == DevInst)) {

            if(PrevDevInfoElem) {
                *PrevDevInfoElem = prev;
            }
            return cur;
        }
    }

    return NULL;
}


BOOL
DevInfoDataFromDeviceInfoElement(
    IN  PDEVICE_INFO_SET DeviceInfoSet,
    IN  PDEVINFO_ELEM    DevInfoElem,
    OUT PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine fills in a SP_DEVINFO_DATA structure based on the
    information in the supplied DEVINFO_ELEM structure.

    Note:  The supplied DeviceInfoData structure must have its cbSize
    field filled in correctly, or the call will fail.

Arguments:

    DeviceInfoSet - Supplies a pointer to the device information set
        containing the specified element.

    DevInfoElem - Supplies a pointer to the DEVINFO_ELEM structure
        containing information to be used in filling in the
        SP_DEVINFO_DATA buffer.

    DeviceInfoData - Supplies a pointer to the buffer that will
        receive the filled-in SP_DEVINFO_DATA structure

Return Value:

    If the function succeeds, the return value is TRUE, otherwise, it
    is FALSE.

--*/
{
    if(DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)) {
        return FALSE;
    }

    ZeroMemory(DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData->cbSize = sizeof(SP_DEVINFO_DATA);

    CopyMemory(&(DeviceInfoData->ClassGuid),
               &(DevInfoElem->ClassGuid),
               sizeof(GUID)
              );

    DeviceInfoData->DevInst = DevInfoElem->DevInst;

    //
    // The 'Reserved' field actually contains a pointer to the
    // corresponding device information element.
    //
    DeviceInfoData->Reserved = (ULONG_PTR)DevInfoElem;

    return TRUE;
}


PDEVINFO_ELEM
FindAssociatedDevInfoElem(
    IN  PDEVICE_INFO_SET  DeviceInfoSet,
    IN  PSP_DEVINFO_DATA  DeviceInfoData,
    OUT PDEVINFO_ELEM    *PreviousElement OPTIONAL
    )
/*++

Routine Description:

    This routine returns the devinfo element for the specified
    SP_DEVINFO_DATA, or NULL if no devinfo element exists.

Arguments:

    DeviceInfoSet - Specifies the set to be searched.

    DeviceInfoData - Supplies a pointer to a device information data
        buffer specifying the device information element to retrieve.

    PreviousElement - Optionally, supplies the address of a
        DEVINFO_ELEM pointer that receives the element that precedes
        the specified element in the linked list.  If the returned
        element is located at the front of the list, then this value
        will be set to NULL.

Return Value:

    If a device information element is found, the return value is a
    pointer to that element, otherwise, the return value is NULL.

--*/
{
    PDEVINFO_ELEM DevInfoElem, CurElem, PrevElem;

    if((DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)) ||
       !(DevInfoElem = (PDEVINFO_ELEM)DeviceInfoData->Reserved)) {

        return NULL;
    }

    if(PreviousElement) {
        //
        // The caller requested that the preceding element be returned
        // (probably because the element is about to be deleted).  Since
        // this is a singly-linked list, we'll search through the list
        // until we find the desired element.
        //
        for(CurElem = DeviceInfoSet->DeviceInfoHead, PrevElem = NULL;
            CurElem;
            PrevElem = CurElem, CurElem = CurElem->Next) {

            if(CurElem == DevInfoElem) {
                //
                // We found the element in our set.
                //
                if(PreviousElement) {
                    *PreviousElement = PrevElem;
                }
                return CurElem;
            }
        }

    } else {
        //
        // The caller doesn't care what the preceding element is, so we
        // can go right to the element, and validate it by ensuring that
        // the ContainingDeviceInfoSet field at the location pointed to
        // by DevInfoElem matches the devinfo set where this guy is supposed
        // to exist.
        //
        if(DevInfoElem->ContainingDeviceInfoSet == DeviceInfoSet) {
            return DevInfoElem;
        }
    }

    return NULL;
}


BOOL
DrvInfoDataFromDriverNode(
    IN  PDEVICE_INFO_SET DeviceInfoSet,
    IN  PDRIVER_NODE     DriverNode,
    IN  DWORD            DriverType,
    OUT PSP_DRVINFO_DATA DriverInfoData
    )
/*++

Routine Description:

    This routine fills in a SP_DRVINFO_DATA structure based on the
    information in the supplied DRIVER_NODE structure.

    Note:  The supplied DriverInfoData structure must have its cbSize
    field filled in correctly, or the call will fail.

Arguments:

    DeviceInfoSet - Supplies a pointer to the device information set
        in which the driver node is located.

    DriverNode - Supplies a pointer to the DRIVER_NODE structure
        containing information to be used in filling in the
        SP_DRVNFO_DATA buffer.

    DriverType - Specifies what type of driver this is.  This value
        may be either SPDIT_CLASSDRIVER or SPDIT_COMPATDRIVER.

    DriverInfoData - Supplies a pointer to the buffer that will
        receive the filled-in SP_DRVINFO_DATA structure

Return Value:

    If the function succeeds, the return value is TRUE, otherwise, it
    is FALSE.

--*/
{
    PTSTR StringPtr;
    DWORD DriverInfoDataSize;

    if((DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA)) &&
       (DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V1))) {
        return FALSE;
    }

    DriverInfoDataSize = DriverInfoData->cbSize;

    ZeroMemory(DriverInfoData, DriverInfoDataSize);
    DriverInfoData->cbSize = DriverInfoDataSize;

    DriverInfoData->DriverType = DriverType;

    MYASSERT(DriverNode->DevDescriptionDisplayName != -1);
    StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                         DriverNode->DevDescriptionDisplayName
                                        );
    lstrcpy(DriverInfoData->Description,
            StringPtr
           );

    MYASSERT(DriverNode->MfgDisplayName != -1);
    StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                         DriverNode->MfgDisplayName
                                        );
    lstrcpy(DriverInfoData->MfgName,
            StringPtr
           );

    if(DriverNode->ProviderDisplayName != -1) {

        StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DriverNode->ProviderDisplayName
                                            );
        lstrcpy(DriverInfoData->ProviderName,
                StringPtr
               );

    }

    //
    // The 'Reserved' field actually contains a pointer to the
    // corresponding driver node.
    //
    DriverInfoData->Reserved = (ULONG_PTR)DriverNode;

    //
    //new NT 5 fields
    //
    if (DriverInfoDataSize == sizeof(SP_DRVINFO_DATA)) {
        DriverInfoData->DriverDate = DriverNode->DriverDate;
        DriverInfoData->DriverVersion = DriverNode->DriverVersion;
    }

    return TRUE;
}


PDRIVER_NODE
FindAssociatedDriverNode(
    IN  PDRIVER_NODE      DriverListHead,
    IN  PSP_DRVINFO_DATA  DriverInfoData,
    OUT PDRIVER_NODE     *PreviousNode    OPTIONAL
    )
/*++

Routine Description:

    This routine searches through all driver nodes in a driver node
    list, looking for one that corresponds to the specified driver
    information structure.  If a match is found, a pointer to the
    driver node is returned.

Arguments:

    DriverListHead - Supplies a pointer to the head of linked list
        of driver nodes to be searched.

    DriverInfoData - Supplies a pointer to a driver information buffer
        specifying the driver node to retrieve.

    PreviousNode - Optionally, supplies the address of a DRIVER_NODE
        pointer that receives the node that precedes the specified
        node in the linked list.  If the returned node is located at
        the front of the list, then this value will be set to NULL.

Return Value:

    If a driver node is found, the return value is a pointer to that
    node, otherwise, the return value is NULL.

--*/
{
    PDRIVER_NODE DriverNode, CurNode, PrevNode;

    if(((DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA)) &&
        (DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V1))) ||
       !(DriverNode = (PDRIVER_NODE)DriverInfoData->Reserved)) {

        return NULL;
    }

    for(CurNode = DriverListHead, PrevNode = NULL;
        CurNode;
        PrevNode = CurNode, CurNode = CurNode->Next) {

        if(CurNode == DriverNode) {
            //
            // We found the driver node in our list.
            //
            if(PreviousNode) {
                *PreviousNode = PrevNode;
            }
            return CurNode;
        }
    }

    return NULL;
}


PDRIVER_NODE
SearchForDriverNode(
    IN  PVOID             StringTable,
    IN  PDRIVER_NODE      DriverListHead,
    IN  PSP_DRVINFO_DATA  DriverInfoData,
    OUT PDRIVER_NODE     *PreviousNode    OPTIONAL
    )
/*++

Routine Description:

    This routine searches through all driver nodes in a driver node
    list, looking for one that matches the fields in the specified
    driver information structure (the 'Reserved' field is ignored).
    If a match is found, a pointer to the driver node is returned.

Arguments:

    StringTable - Supplies the string table that should be used in
        retrieving string IDs for driver look-up.

    DriverListHead - Supplies a pointer to the head of linked list
        of driver nodes to be searched.

    DriverInfoData - Supplies a pointer to a driver information buffer
        specifying the driver parameters we're looking for.

    PreviousNode - Optionally, supplies the address of a DRIVER_NODE
        pointer that receives the node that precedes the specified
        node in the linked list.  If the returned node is located at
        the front of the list, then this value will be set to NULL.

Return Value:

    If a driver node is found, the return value is a pointer to that
    node, otherwise, the return value is NULL.

--*/
{
    PDRIVER_NODE CurNode, PrevNode;
    LONG DevDescription, MfgName, ProviderName;
    TCHAR TempString[LINE_LEN];
    DWORD TempStringLength;
    BOOL  Match;

    MYASSERT((DriverInfoData->cbSize == sizeof(SP_DRVINFO_DATA)) ||
             (DriverInfoData->cbSize == sizeof(SP_DRVINFO_DATA_V1)));

    //
    // Retrieve the string IDs for the 3 driver parameters we'll be
    // matching against.
    //
    lstrcpyn(TempString, DriverInfoData->Description, SIZECHARS(TempString));
    if((DevDescription = pStringTableLookUpString(
                             StringTable,
                             TempString,
                             &TempStringLength,
                             NULL,
                             NULL,
                             STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                             NULL,0)) == -1) {

        return NULL;
    }

    lstrcpyn(TempString, DriverInfoData->MfgName, SIZECHARS(TempString));
    if((MfgName = pStringTableLookUpString(
                             StringTable,
                             TempString,
                             &TempStringLength,
                             NULL,
                             NULL,
                             STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                             NULL,0)) == -1) {

        return NULL;
    }

    //
    // ProviderName may be empty...
    //
    if(*(DriverInfoData->ProviderName)) {

        lstrcpyn(TempString, DriverInfoData->ProviderName, SIZECHARS(TempString));
        if((ProviderName = pStringTableLookUpString(
                                 StringTable,
                                 TempString,
                                 &TempStringLength,
                                 NULL,
                                 NULL,
                                 STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                 NULL,0)) == -1) {

            return NULL;
        }

    } else {
        ProviderName = -1;
    }

    for(CurNode = DriverListHead, PrevNode = NULL;
        CurNode;
        PrevNode = CurNode, CurNode = CurNode->Next)
    {
        //
        // Check first on DevDescription (least likely to match), then on MfgName, and finally
        // on ProviderName. On NT 5 and later we will also check the DriverDate and DriverVersion.
        //
        if(CurNode->DevDescription == DevDescription) {

            if(CurNode->MfgName == MfgName) {

                if(CurNode->ProviderName == ProviderName) {

                    //
                    //On NT 5 and later, also compare the DriverDate and DriverVersion
                    //
                    if (DriverInfoData->cbSize == sizeof(SP_DRVINFO_DATA)) {

                        //
                        //Assume that we have a match
                        //
                        Match = TRUE;

                        //
                        //If the DriverDate passed in is not 0 then make sure it matches
                        //
                        if (DriverInfoData->DriverDate.dwLowDateTime != 0 ||
                            DriverInfoData->DriverDate.dwHighDateTime != 0) {

                            if ((CurNode->DriverDate.dwLowDateTime != DriverInfoData->DriverDate.dwLowDateTime) ||
                                (CurNode->DriverDate.dwHighDateTime != DriverInfoData->DriverDate.dwHighDateTime)) {

                                Match = FALSE;
                            }
                        }

                        //
                        //If the DriverVersion passed in is not 0 then make sure it matches
                        //
                        else if (DriverInfoData->DriverVersion != 0) {

                            if (CurNode->DriverVersion != DriverInfoData->DriverVersion) {

                                Match = FALSE;
                            }
                        }

                        if (Match) {

                            //
                            // We found the driver node in our list.
                            //
                            if(PreviousNode) {
                                *PreviousNode = PrevNode;
                            }
                            return CurNode;
                        }

                    } else {

                        //
                        // We found the driver node in our list.
                        //
                        if(PreviousNode) {
                            *PreviousNode = PrevNode;
                        }
                        return CurNode;
                    }
                }
            }
        }
    }

    return NULL;
}


DWORD
DrvInfoDetailsFromDriverNode(
    IN  PDEVICE_INFO_SET        DeviceInfoSet,
    IN  PDRIVER_NODE            DriverNode,
    OUT PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData, OPTIONAL
    IN  DWORD                   BufferSize,
    OUT PDWORD                  RequiredSize          OPTIONAL
    )
/*++

Routine Description:

    This routine fills in a SP_DRVINFO_DETAIL_DATA structure based on the
    information in the supplied DRIVER_NODE structure.

    If the buffer is supplied, and is valid, this routine is guaranteed to
    fill in all statically-sized fields, and as many IDs as will fit in the
    variable-length multi-sz buffer.

    Note:  If supplied, the DriverInfoDetailData structure must have its
    cbSize field filled in correctly, or the call will fail. Here correctly
    means sizeof(SP_DRVINFO_DETAIL_DATA), which we use as a signature.
    This is entirely separate from BufferSize. See below.

Arguments:

    DeviceInfoSet - Supplies a pointer to the device information set
        in which the driver node is located.

    DriverNode - Supplies a pointer to the DRIVER_NODE structure
        containing information to be used in filling in the
        SP_DRVNFO_DETAIL_DATA buffer.

    DriverInfoDetailData - Optionally, supplies a pointer to the buffer
        that will receive the filled-in SP_DRVINFO_DETAIL_DATA structure.
        If this buffer is not supplied, then the caller is only interested
        in what the RequiredSize for the buffer is.

    BufferSize - Supplies size of the DriverInfoDetailData buffer, in
        bytes.  If DriverInfoDetailData is not specified, then this
        value must be zero. This value must be at least the size
        of the fixed part of the structure (ie,
        offsetof(SP_DRVINFO_DETAIL_DATA,HardwareID)) plus sizeof(TCHAR),
        which gives us enough room to store the fixed part plus
        a terminating nul to guarantee we return at least a valid
        empty multi_sz.

    RequiredSize - Optionally, supplies the address of a variable that
        receives the number of bytes required to store the data. Note that
        depending on structure alignment and the data itself, this may
        actually be *smaller* than sizeof(SP_DRVINFO_DETAIL_DATA).

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

--*/
{
    PTSTR StringPtr, BufferPtr;
    DWORD IdListLen, CompatIdListLen, StringLen, TotalLen, i;
    DWORD Err = ERROR_INSUFFICIENT_BUFFER;

    #define FIXEDPARTLEN offsetof(SP_DRVINFO_DETAIL_DATA,HardwareID)

    if(DriverInfoDetailData) {
        //
        // Check validity of the DriverInfoDetailData buffer on the way in,
        // and make sure we have enough room for the fixed part
        // of the structure plus the extra nul that will terminate the
        // multi_sz.
        //
        if((DriverInfoDetailData->cbSize != sizeof(SP_DRVINFO_DETAIL_DATA))
        || (BufferSize < (FIXEDPARTLEN+sizeof(TCHAR)))) {

            return ERROR_INVALID_USER_BUFFER;
        }
        //
        // The buffer is large enough to contain at least the fixed-length part
        // of the structure.
        //
        Err = NO_ERROR;

    } else if(BufferSize) {
        return ERROR_INVALID_USER_BUFFER;
    }

    if(DriverInfoDetailData) {

        ZeroMemory(DriverInfoDetailData,FIXEDPARTLEN);
        DriverInfoDetailData->cbSize = FIXEDPARTLEN + sizeof(TCHAR);

        DriverInfoDetailData->InfDate = DriverNode->InfDate;

        MYASSERT(DriverNode->InfSectionName != -1);
        StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DriverNode->InfSectionName
                                            );
        lstrcpyn(DriverInfoDetailData->SectionName, StringPtr,sizeof(DriverInfoDetailData->SectionName)/sizeof(TCHAR));

        MYASSERT(DriverNode->InfFileName != -1);
        StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DriverNode->InfFileName
                                            );
        lstrcpyn(DriverInfoDetailData->InfFileName, StringPtr,sizeof(DriverInfoDetailData->InfFileName)/sizeof(TCHAR));

        MYASSERT(DriverNode->DrvDescription != -1);
        StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DriverNode->DrvDescription
                                            );
        lstrcpyn(DriverInfoDetailData->DrvDescription, StringPtr,sizeof(DriverInfoDetailData->DrvDescription)/sizeof(TCHAR));

        //
        // Initialize the multi_sz to be empty.
        //
        DriverInfoDetailData->HardwareID[0] = 0;

        //
        // The 'Reserved' field actually contains a pointer to the
        // corresponding driver node.
        //
        DriverInfoDetailData->Reserved = (ULONG_PTR)DriverNode;
    }

    //
    // Now, build the multi-sz buffer containing the hardware and compatible IDs.
    //
    if(DriverNode->HardwareId == -1) {
        //
        // If there's no HardwareId, then we know there are no compatible IDs, so
        // we can return right now.
        //
        if(RequiredSize) {
            *RequiredSize = FIXEDPARTLEN + sizeof(TCHAR);
        }
        return Err;
    }

    if(DriverInfoDetailData) {
        BufferPtr = DriverInfoDetailData->HardwareID;
        IdListLen = (BufferSize - FIXEDPARTLEN) / sizeof(TCHAR);
    } else {
        IdListLen = 0;
    }

    //
    // Retrieve the HardwareId.
    //
    StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                         DriverNode->HardwareId
                                        );

    TotalLen = StringLen = lstrlen(StringPtr) + 1; // include nul terminator

    if(StringLen < IdListLen) {
        MYASSERT(Err == NO_ERROR);
        CopyMemory(BufferPtr,
                   StringPtr,
                   StringLen * sizeof(TCHAR)
                  );
        BufferPtr += StringLen;
        IdListLen -= StringLen;
        DriverInfoDetailData->CompatIDsOffset = StringLen;
    } else {
        if(RequiredSize) {
            //
            // Since the caller requested the required size, we can't just return
            // here.  Set the error, so we'll know not to bother trying to fill
            // the buffer.
            //
            Err = ERROR_INSUFFICIENT_BUFFER;
        } else {
            return ERROR_INSUFFICIENT_BUFFER;
        }
    }

    //
    // Remember the size of the buffer left over for CompatibleIDs.
    //
    CompatIdListLen = IdListLen;

    //
    // Now retrieve the CompatibleIDs.
    //
    for(i = 0; i < DriverNode->NumCompatIds; i++) {

        MYASSERT(DriverNode->CompatIdList[i] != -1);

        StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DriverNode->CompatIdList[i]
                                            );
        StringLen = lstrlen(StringPtr) + 1;

        if(Err == NO_ERROR) {

            if(StringLen < IdListLen) {
                CopyMemory(BufferPtr,
                           StringPtr,
                           StringLen * sizeof(TCHAR)
                          );
                BufferPtr += StringLen;
                IdListLen -= StringLen;

            } else {

                Err = ERROR_INSUFFICIENT_BUFFER;
                if(!RequiredSize) {
                    //
                    // We've run out of buffer, and the caller doesn't care what
                    // the total required size is, so bail now.
                    //
                    break;
                }
            }
        }

        TotalLen += StringLen;
    }

    if(DriverInfoDetailData) {
        //
        // Append the additional terminating nul.  Note that we've been saving the
        // last character position in the buffer all along, so we're guaranteed to
        // be inside the buffer.
        //
        MYASSERT(BufferPtr < (PTSTR)((PBYTE)DriverInfoDetailData + BufferSize));
        *BufferPtr = 0;

        //
        // Store the length of the CompatibleIDs list.  Note that this is the length
        // of the list actually returned, which may be less than the length of the
        // entire list (if the caller-supplied buffer wasn't large enough).
        //
        if(CompatIdListLen -= IdListLen) {
            //
            // If this list is non-empty, then add a character for the extra nul
            // terminating the multi-sz list.
            //
            CompatIdListLen++;
        }
        DriverInfoDetailData->CompatIDsLength = CompatIdListLen;
    }

    if(RequiredSize) {
        *RequiredSize = FIXEDPARTLEN + ((TotalLen + 1) * sizeof(TCHAR));
    }

    return Err;
}


PDRIVER_LIST_OBJECT
GetAssociatedDriverListObject(
    IN  PDRIVER_LIST_OBJECT  ObjectListHead,
    IN  PDRIVER_NODE         DriverListHead,
    OUT PDRIVER_LIST_OBJECT *PrevDriverListObject OPTIONAL
    )
/*++

Routine Description:

    This routine searches through a driver list object list, and returns a
    pointer to the driver list object containing the list specified by
    DrvListHead.  It also optionally returns the preceding object in the list
    (used when extracting the driver list object from the linked list).

Arguments:

    ObjectListHead - Specifies the linked list of driver list objects to be
        searched.

    DriverListHead - Specifies the driver list to be searched for.

    PrevDriverListObject - Optionaly, supplies the address of the variable that
        receives a pointer to the driver list object immediately preceding the
        matching object.  If the object was found at the front of the list, then
        this variable will be set to NULL.

Return Value:

    If the matching driver list object is found, the return value is a pointer
    to that element, otherwise, the return value is NULL.

--*/
{
    PDRIVER_LIST_OBJECT prev = NULL;

    while(ObjectListHead) {

        if(ObjectListHead->DriverListHead == DriverListHead) {

            if(PrevDriverListObject) {
                *PrevDriverListObject = prev;
            }

            return ObjectListHead;
        }

        prev = ObjectListHead;
        ObjectListHead = ObjectListHead->Next;
    }

    return NULL;
}


VOID
DereferenceClassDriverList(
    IN PDEVICE_INFO_SET DeviceInfoSet,
    IN PDRIVER_NODE     DriverListHead OPTIONAL
    )
/*++

Routine Description:

    This routine dereferences the class driver list object associated with the
    supplied DriverListHead.  If the refcount goes to zero, the object is destroyed,
    and all associated memory is freed.

Arguments:

    DeviceInfoSet - Supplies the address of the device information set containing the
        linked list of class driver list objects.

    DriverListHead - Optionally, supplies a pointer to the header of the driver list
        to be dereferenced.  If this parameter is not supplied, the routine does nothing.

Return Value:

    None.

--*/
{
    PDRIVER_LIST_OBJECT DrvListObject, PrevDrvListObject;

    if(DriverListHead) {

        DrvListObject = GetAssociatedDriverListObject(DeviceInfoSet->ClassDrvListObjectList,
                                                      DriverListHead,
                                                      &PrevDrvListObject
                                                     );
        MYASSERT(DrvListObject && DrvListObject->RefCount);

        if(!(--DrvListObject->RefCount)) {

            if(PrevDrvListObject) {
                PrevDrvListObject->Next = DrvListObject->Next;
            } else {
                DeviceInfoSet->ClassDrvListObjectList = DrvListObject->Next;
            }
            MyFree(DrvListObject);

            DestroyDriverNodes(DriverListHead, DeviceInfoSet);
        }
    }
}


DWORD
GetDevInstallParams(
    IN  PDEVICE_INFO_SET        DeviceInfoSet,
    IN  PDEVINSTALL_PARAM_BLOCK DevInstParamBlock,
    OUT PSP_DEVINSTALL_PARAMS   DeviceInstallParams
    )
/*++

Routine Description:

    This routine fills in a SP_DEVINSTALL_PARAMS structure based on the
    installation parameter block supplied.

    Note:  The DeviceInstallParams structure must have its cbSize field
    filled in correctly, or the call will fail.

Arguments:

    DeviceInfoSet - Supplies the address of the device information set
        containing the parameters to be retrieved.  (This parameter is
        used to gain access to the string table for some of the string
        parameters).

    DevInstParamBlock - Supplies the address of an installation parameter
        block containing the parameters to be used in filling out the
        return buffer.

    DeviceInstallParams - Supplies the address of a buffer that will
        receive the filled-in SP_DEVINSTALL_PARAMS structure.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

--*/
{
    PTSTR StringPtr;

    if(DeviceInstallParams->cbSize != sizeof(SP_DEVINSTALL_PARAMS)) {
        return ERROR_INVALID_USER_BUFFER;
    }

    //
    // Fill in parameters.
    //
    ZeroMemory(DeviceInstallParams, sizeof(SP_DEVINSTALL_PARAMS));
    DeviceInstallParams->cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    DeviceInstallParams->Flags                    = DevInstParamBlock->Flags;
    DeviceInstallParams->FlagsEx                  = DevInstParamBlock->FlagsEx;
    DeviceInstallParams->hwndParent               = DevInstParamBlock->hwndParent;
    DeviceInstallParams->InstallMsgHandler        = DevInstParamBlock->InstallMsgHandler;
    DeviceInstallParams->InstallMsgHandlerContext = DevInstParamBlock->InstallMsgHandlerContext;
    DeviceInstallParams->FileQueue                = DevInstParamBlock->UserFileQ;
    DeviceInstallParams->ClassInstallReserved     = DevInstParamBlock->ClassInstallReserved;
    //
    // The Reserved field is currently unused.
    //

    if(DevInstParamBlock->DriverPath != -1) {
        StringPtr = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DevInstParamBlock->DriverPath
                                            );
        lstrcpy(DeviceInstallParams->DriverPath, StringPtr);
    }

    return NO_ERROR;
}


DWORD
GetClassInstallParams(
    IN  PDEVINSTALL_PARAM_BLOCK DevInstParamBlock,
    OUT PSP_CLASSINSTALL_HEADER ClassInstallParams, OPTIONAL
    IN  DWORD                   BufferSize,
    OUT PDWORD                  RequiredSize        OPTIONAL
    )
/*++

Routine Description:

    This routine fills in a buffer with the class installer parameters (if any)
    contained in the installation parameter block supplied.

    Note:  If supplied, the ClassInstallParams structure must have the cbSize
    field of the embedded SP_CLASSINSTALL_HEADER structure set to the size, in bytes,
    of the header.  If this is not set correctly, the call will fail.

Arguments:

    DevInstParamBlock - Supplies the address of an installation parameter block
        containing the class installer parameters to be used in filling out the
        return buffer.

    DeviceInstallParams - Optionally, supplies the address of a buffer
        that will receive the class installer parameters structure currently
        stored in the installation parameters block.  If this parameter is not
        supplied, then BufferSize must be zero.

    BufferSize - Supplies the size, in bytes, of the DeviceInstallParams
        buffer, or zero if DeviceInstallParams is not supplied.

    RequiredSize - Optionally, supplies the address of a variable that
        receives the number of bytes required to store the data.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

--*/
{
    //
    // First, see whether we have any class install params, and if not, return
    // ERROR_NO_CLASSINSTALL_PARAMS.
    //
    if(!DevInstParamBlock->ClassInstallHeader) {
        return ERROR_NO_CLASSINSTALL_PARAMS;
    }

    if(ClassInstallParams) {

        if((BufferSize < sizeof(SP_CLASSINSTALL_HEADER)) ||
           (ClassInstallParams->cbSize != sizeof(SP_CLASSINSTALL_HEADER))) {

            return ERROR_INVALID_USER_BUFFER;
        }

    } else if(BufferSize) {
        return ERROR_INVALID_USER_BUFFER;
    }

    //
    // Store required size in output parameter (if requested).
    //
    if(RequiredSize) {
        *RequiredSize = DevInstParamBlock->ClassInstallParamsSize;
    }

    //
    // See if supplied buffer is large enough.
    //
    if(BufferSize < DevInstParamBlock->ClassInstallParamsSize) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    CopyMemory((PVOID)ClassInstallParams,
               (PVOID)DevInstParamBlock->ClassInstallHeader,
               DevInstParamBlock->ClassInstallParamsSize
              );

    return NO_ERROR;
}


DWORD
SetDevInstallParams(
    IN OUT PDEVICE_INFO_SET        DeviceInfoSet,
    IN     PSP_DEVINSTALL_PARAMS   DeviceInstallParams,
    OUT    PDEVINSTALL_PARAM_BLOCK DevInstParamBlock,
    IN     BOOL                    MsgHandlerIsNativeCharWidth
    )
/*++

Routine Description:

    This routine updates an internal parameter block based on the parameters
    supplied in a SP_DEVINSTALL_PARAMS structure.

    Note:  The supplied DeviceInstallParams structure must have its cbSize
    field filled in correctly, or the call will fail.

Arguments:

    DeviceInfoSet - Supplies the address of the device information set
        containing the parameters to be set.

    DeviceInstallParams - Supplies the address of a buffer containing the new
        installation parameters.

    DevInstParamBlock - Supplies the address of an installation parameter
        block to be updated.

    MsgHandlerIsNativeCharWidth - supplies a flag indicating whether the
        InstallMsgHandler in the DeviceInstallParams structure points to
        a callback routine that is expecting arguments in the 'native'
        character format. A value of FALSE is meaningful only in the
        Unicode build and specifies that the callback routine wants
        ANSI parameters.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

--*/
{
    DWORD DriverPathLen;
    LONG StringId;
    TCHAR TempString[MAX_PATH];
    HSPFILEQ OldQueueHandle = NULL;
    BOOL bRestoreQueue = FALSE;

    if(DeviceInstallParams->cbSize != sizeof(SP_DEVINSTALL_PARAMS)) {
        return ERROR_INVALID_USER_BUFFER;
    }

    //
    // No validation is currently required for the hwndParent, InstallMsgHandler,
    // InstallMsgHandlerContext, or ClassInstallReserved fields.
    //

    //
    // Validate Flags(Ex)
    //
    if((DeviceInstallParams->Flags & DI_FLAGS_ILLEGAL) ||
       (DeviceInstallParams->FlagsEx & DI_FLAGSEX_ILLEGAL)) {

        return ERROR_INVALID_FLAGS;
    }

    //
    // Make sure that if DI_CLASSINSTALLPARAMS is being set, that we really do have
    // class install parameters.
    //
    if((DeviceInstallParams->Flags & DI_CLASSINSTALLPARAMS) &&
       !(DevInstParamBlock->ClassInstallHeader)) {

        return ERROR_NO_CLASSINSTALL_PARAMS;
    }

    //
    // Make sure that if DI_NOVCP is being set, that we have a caller-supplied file queue.
    //
    if((DeviceInstallParams->Flags & DI_NOVCP) &&
       ((DeviceInstallParams->FileQueue == NULL) || (DeviceInstallParams->FileQueue == INVALID_HANDLE_VALUE))) {

        return ERROR_INVALID_FLAGS;
    }

    //
    // Validate that the DriverPath string is properly NULL-terminated.
    //
    if((DriverPathLen = lstrlen(DeviceInstallParams->DriverPath)) >= MAX_PATH) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Validate the caller-supplied file queue.
    //
    if((DeviceInstallParams->FileQueue == NULL) || (DeviceInstallParams->FileQueue == INVALID_HANDLE_VALUE)) {
        //
        // Store the current file queue handle (if any) to be released later.
        //
        OldQueueHandle = DevInstParamBlock->UserFileQ;
        DevInstParamBlock->UserFileQ = NULL;
        bRestoreQueue = TRUE;

    } else {
        //
        // The caller supplied a file queue handle.  There's presently not much validation we can do
        // on it, so assume it's valid.
        //
        if(DeviceInstallParams->FileQueue != DevInstParamBlock->UserFileQ) {
            //
            // The caller has supplied a file queue handle that's different from the one we currently
            // have stored.  Remember the old handle (in case we need to restore), and store the new
            // handle.  Also, increment the lock refcount on the new handle (enclose in try/except in
            // case it's a bogus one).
            //
            OldQueueHandle = DevInstParamBlock->UserFileQ;
            bRestoreQueue = TRUE;

            try {
                ((PSP_FILE_QUEUE)(DeviceInstallParams->FileQueue))->LockRefCount++;
                DevInstParamBlock->UserFileQ = DeviceInstallParams->FileQueue;
            } except(EXCEPTION_EXECUTE_HANDLER) {
                DevInstParamBlock->UserFileQ = OldQueueHandle;
                bRestoreQueue = FALSE;
            }

            if(!bRestoreQueue) {
                //
                // The file queue handle we were given was invalid.
                //
                return ERROR_INVALID_PARAMETER;
            }
        }
    }

    //
    // Store the specified driver path.
    //
    if(DriverPathLen) {
        lstrcpy(TempString, DeviceInstallParams->DriverPath);
        if((StringId = pStringTableAddString(DeviceInfoSet->StringTable,
                                             TempString,
                                             STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                             NULL,0
                                            )) == -1) {
            //
            // We couldn't add the new driver path string to the string table.  Restore the old
            // file queue (if necessary) and return an out-of-memory error.
            //
            if(bRestoreQueue) {

                if(DevInstParamBlock->UserFileQ) {
                    try {
                        ((PSP_FILE_QUEUE)(DevInstParamBlock->UserFileQ))->LockRefCount--;
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        ;   // nothing to do
                    }
                }
                DevInstParamBlock->UserFileQ = OldQueueHandle;
            }
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        DevInstParamBlock->DriverPath = StringId;
    } else {
        DevInstParamBlock->DriverPath = -1;
    }

    //
    // Should be smooth sailing from here on out.  Decrement the refcount on the old queue handle,
    // if there was one.
    //
    if(OldQueueHandle) {
        try {
            MYASSERT(((PSP_FILE_QUEUE)OldQueueHandle)->LockRefCount);
            ((PSP_FILE_QUEUE)OldQueueHandle)->LockRefCount--;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ; // nothing to do
        }
    }

    //
    // Ignore attempts at modifying read-only flags.
    //
    DevInstParamBlock->Flags   = (DeviceInstallParams->Flags & ~DI_FLAGS_READONLY) |
                                 (DevInstParamBlock->Flags   &  DI_FLAGS_READONLY);

    DevInstParamBlock->FlagsEx = (DeviceInstallParams->FlagsEx & ~DI_FLAGSEX_READONLY) |
                                 (DevInstParamBlock->FlagsEx   &  DI_FLAGSEX_READONLY);

    //
    // Additionally, if we're in non-interactive mode, make sure not to clear
    // our "be quiet" flags.
    //
    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        DevInstParamBlock->Flags   |= DI_QUIETINSTALL;
        DevInstParamBlock->FlagsEx |= DI_FLAGSEX_NOUIONQUERYREMOVE;
    }

    //
    // Store the rest of the parameters.
    //
    DevInstParamBlock->hwndParent               = DeviceInstallParams->hwndParent;
    DevInstParamBlock->InstallMsgHandler        = DeviceInstallParams->InstallMsgHandler;
    DevInstParamBlock->InstallMsgHandlerContext = DeviceInstallParams->InstallMsgHandlerContext;
    DevInstParamBlock->ClassInstallReserved     = DeviceInstallParams->ClassInstallReserved;

    DevInstParamBlock->InstallMsgHandlerIsNativeCharWidth = MsgHandlerIsNativeCharWidth;

    return NO_ERROR;
}


DWORD
SetClassInstallParams(
    IN OUT PDEVICE_INFO_SET        DeviceInfoSet,
    IN     PSP_CLASSINSTALL_HEADER ClassInstallParams,    OPTIONAL
    IN     DWORD                   ClassInstallParamsSize,
    OUT    PDEVINSTALL_PARAM_BLOCK DevInstParamBlock
    )
/*++

Routine Description:

    This routine updates an internal class installer parameter block based on
    the parameters supplied in a class installer parameter buffer.  If this
    buffer is not supplied, then the existing class installer parameters (if
    any) are cleared.

Arguments:

    DeviceInfoSet - Supplies the address of the device information set
        for which class installer parameters are to be set.

    ClassInstallParams - Optionally, supplies the address of a buffer containing
        the class installer parameters to be used.    The SP_CLASSINSTALL_HEADER
        structure at the beginning of the buffer must have its cbSize field set to
        be sizeof(SP_CLASSINSTALL_HEADER), and the InstallFunction field must be
        set to the DI_FUNCTION code reflecting the type of parameters supplied in
        the rest of the buffer.

        If this parameter is not supplied, then the current class installer parameters
        (if any) will be cleared for the specified device information set or element.

    ClassInstallParamsSize - Supplies the size, in bytes, of the ClassInstallParams
        buffer.  If the buffer is not supplied (i.e., the class installer parameters
        are to be cleared), then this value must be zero.

    DevInstParamBlock - Supplies the address of an installation parameter
        block to be updated.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

--*/
{
    PBYTE NewParamBuffer;

    if(ClassInstallParams) {

        if((ClassInstallParamsSize < sizeof(SP_CLASSINSTALL_HEADER)) ||
           (ClassInstallParams->cbSize != sizeof(SP_CLASSINSTALL_HEADER))) {

            return ERROR_INVALID_USER_BUFFER;
        }

    } else {
        //
        // We are to clear any existing class installer parameters.
        //
        if(ClassInstallParamsSize) {
            return ERROR_INVALID_USER_BUFFER;
        }

        if(DevInstParamBlock->ClassInstallHeader) {
            MyFree(DevInstParamBlock->ClassInstallHeader);
            DevInstParamBlock->ClassInstallHeader = NULL;
            DevInstParamBlock->ClassInstallParamsSize = 0;
            DevInstParamBlock->Flags &= ~DI_CLASSINSTALLPARAMS;
        }

        return NO_ERROR;
    }


    //
    // Validate the new class install parameters w.r.t. the value of the specified
    // InstallFunction code.
    //
    switch(ClassInstallParams->InstallFunction) {

        case DIF_ENABLECLASS :
            //
            // We should have a SP_ENABLECLASS_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_ENABLECLASS_PARAMS)) {

                PSP_ENABLECLASS_PARAMS EnableClassParams;

                EnableClassParams = (PSP_ENABLECLASS_PARAMS)ClassInstallParams;
                //
                // Don't bother validating GUID--just validate EnableMessage field.
                //
                if((EnableClassParams->EnableMessage >= ENABLECLASS_QUERY) &&
                   (EnableClassParams->EnableMessage <= ENABLECLASS_FAILURE)) {
                    //
                    // parameter set validated.
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_MOVEDEVICE :
            //
            // We should have a SP_MOVEDEV_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_MOVEDEV_PARAMS)) {

                PSP_MOVEDEV_PARAMS MoveDevParams;

                MoveDevParams = (PSP_MOVEDEV_PARAMS)ClassInstallParams;
                if(FindAssociatedDevInfoElem(DeviceInfoSet,
                                             &(MoveDevParams->SourceDeviceInfoData),
                                             NULL)) {
                    //
                    // parameter set validated.
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_PROPERTYCHANGE :
            //
            // We should have a SP_PROPCHANGE_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_PROPCHANGE_PARAMS)) {

                PSP_PROPCHANGE_PARAMS PropChangeParams;

                PropChangeParams = (PSP_PROPCHANGE_PARAMS)ClassInstallParams;
                if((PropChangeParams->StateChange >= DICS_ENABLE) &&
                   (PropChangeParams->StateChange <= DICS_STOP)) {

                    //
                    // Validate Scope specifier--even though these values are defined like
                    // flags, they are mutually exclusive, so treat them like ordinals.
                    //
                    if((PropChangeParams->Scope == DICS_FLAG_GLOBAL) ||
                       (PropChangeParams->Scope == DICS_FLAG_CONFIGSPECIFIC) ||
                       (PropChangeParams->Scope == DICS_FLAG_CONFIGGENERAL)) {

                        //
                        // DICS_START and DICS_STOP are always config specific.
                        //
                        if(((PropChangeParams->StateChange == DICS_START) || (PropChangeParams->StateChange == DICS_STOP)) &&
                           (PropChangeParams->Scope != DICS_FLAG_CONFIGSPECIFIC)) {

                            goto BadPropChangeParams;
                        }

                        //
                        // parameter set validated
                        //
                        // NOTE: Even though DICS_FLAG_CONFIGSPECIFIC indicates
                        // that the HwProfile field specifies a hardware profile,
                        // there's no need to do validation on that.
                        //
                        break;
                    }
                }
            }

BadPropChangeParams:
            return ERROR_INVALID_PARAMETER;

        case DIF_REMOVE :
            //
            // We should have a SP_REMOVEDEVICE_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_REMOVEDEVICE_PARAMS)) {

                PSP_REMOVEDEVICE_PARAMS RemoveDevParams;

                RemoveDevParams = (PSP_REMOVEDEVICE_PARAMS)ClassInstallParams;
                if((RemoveDevParams->Scope == DI_REMOVEDEVICE_GLOBAL) ||
                   (RemoveDevParams->Scope == DI_REMOVEDEVICE_CONFIGSPECIFIC)) {
                    //
                    // parameter set validated
                    //
                    // NOTE: Even though DI_REMOVEDEVICE_CONFIGSPECIFIC indicates
                    // that the HwProfile field specifies a hardware profile,
                    // there's no need to do validation on that.
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_UNREMOVE :
            //
            // We should have a SP_UNREMOVEDEVICE_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_UNREMOVEDEVICE_PARAMS)) {

                PSP_UNREMOVEDEVICE_PARAMS UnremoveDevParams;

                UnremoveDevParams = (PSP_UNREMOVEDEVICE_PARAMS)ClassInstallParams;
                if(UnremoveDevParams->Scope == DI_UNREMOVEDEVICE_CONFIGSPECIFIC) {
                    //
                    // parameter set validated
                    //
                    // NOTE: Even though DI_UNREMOVEDEVICE_CONFIGSPECIFIC indicates
                    // that the HwProfile field specifies a hardware profile,
                    // there's no need to do validation on that.
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_SELECTDEVICE :
            //
            // We should have a SP_SELECTDEVICE_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_SELECTDEVICE_PARAMS)) {

                PSP_SELECTDEVICE_PARAMS SelectDevParams;

                SelectDevParams = (PSP_SELECTDEVICE_PARAMS)ClassInstallParams;
                //
                // Validate that the string fields are properly NULL-terminated.
                //
                if((lstrlen(SelectDevParams->Title) < (MAX_TITLE_LEN - 1)) &&
                   (lstrlen(SelectDevParams->Instructions) < (MAX_INSTRUCTION_LEN - 1)) &&
                   (lstrlen(SelectDevParams->ListLabel) < (MAX_LABEL_LEN - 1)) &&
                   (lstrlen(SelectDevParams->SubTitle) < (MAX_SUBTITLE_LEN - 1))) {
                    //
                    // parameter set validated
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_INSTALLWIZARD :
            //
            // We should have a SP_INSTALLWIZARD_DATA structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_INSTALLWIZARD_DATA)) {

                PSP_INSTALLWIZARD_DATA InstallWizData;
                DWORD i;

                InstallWizData = (PSP_INSTALLWIZARD_DATA)ClassInstallParams;
                //
                // Validate the propsheet handle list.
                //
                if(InstallWizData->NumDynamicPages <= MAX_INSTALLWIZARD_DYNAPAGES) {

                    for(i = 0; i < InstallWizData->NumDynamicPages; i++) {
                        //
                        // For now, just verify that all handles are non-NULL.
                        //
                        if(!(InstallWizData->DynamicPages[i])) {
                            //
                            // Invalid property sheet page handle
                            //
                            return ERROR_INVALID_PARAMETER;
                        }
                    }

                    //
                    // Handles are verified, now verify Flags.
                    //
                    if(!(InstallWizData->Flags & NDW_INSTALLFLAG_ILLEGAL)) {

                        if(!(InstallWizData->DynamicPageFlags & DYNAWIZ_FLAG_ILLEGAL)) {
                            //
                            // parameter set validated
                            //
                            break;
                        }
                    }
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_NEWDEVICEWIZARD_PRESELECT :
        case DIF_NEWDEVICEWIZARD_SELECT :
        case DIF_NEWDEVICEWIZARD_PREANALYZE :
        case DIF_NEWDEVICEWIZARD_POSTANALYZE :
        case DIF_NEWDEVICEWIZARD_FINISHINSTALL :
        case DIF_ADDPROPERTYPAGE_ADVANCED:
        case DIF_ADDPROPERTYPAGE_BASIC:
            //
            // We should have a SP_NEWDEVICEWIZARD_DATA structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_NEWDEVICEWIZARD_DATA)) {

                PSP_NEWDEVICEWIZARD_DATA NewDevWizData;
                DWORD i;

                NewDevWizData = (PSP_NEWDEVICEWIZARD_DATA)ClassInstallParams;
                //
                // Validate the propsheet handle list.
                //
                if(NewDevWizData->NumDynamicPages <= MAX_INSTALLWIZARD_DYNAPAGES) {

                    for(i = 0; i < NewDevWizData->NumDynamicPages; i++) {
                        //
                        // For now, just verify that all handles are non-NULL.
                        //
                        if(!(NewDevWizData->DynamicPages[i])) {
                            //
                            // Invalid property sheet page handle
                            //
                            return ERROR_INVALID_PARAMETER;
                        }
                    }

                    //
                    // Handles are verified, now verify Flags.
                    //
                    if(!(NewDevWizData->Flags & NEWDEVICEWIZARD_FLAG_ILLEGAL)) {
                        //
                        // parameter set validated
                        //
                        break;
                    }
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_DETECT :
            //
            // We should have a SP_DETECTDEVICE_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_DETECTDEVICE_PARAMS)) {

                PSP_DETECTDEVICE_PARAMS DetectDeviceParams;

                DetectDeviceParams = (PSP_DETECTDEVICE_PARAMS)ClassInstallParams;
                //
                // Make sure there's an entry point for the progress notification callback.
                //
                if(DetectDeviceParams->DetectProgressNotify) {
                    //
                    // parameter set validated.
                    //
                    break;
                }
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_GETWINDOWSUPDATEINFO:
            //
            // We should have a SP_WINDOWSUPDATE_PARAMS structure.
            //
            if(ClassInstallParamsSize == sizeof(SP_WINDOWSUPDATE_PARAMS)) {

                break;
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_TROUBLESHOOTER:
            //
            // We should have a SP_TROUBLESHOOTER_PARAMS structure.
            //
            if (ClassInstallParamsSize == sizeof(SP_TROUBLESHOOTER_PARAMS)) {

                break;
            }
            return ERROR_INVALID_PARAMETER;

        case DIF_POWERMESSAGEWAKE:
            //
            // We should have a SP_POWERMESSAGEWAKE_PARAMS structure.
            //
            if (ClassInstallParamsSize == sizeof(SP_POWERMESSAGEWAKE_PARAMS)) {

                break;
            }
            return ERROR_INVALID_PARAMETER;

        default :
            //
            // Some generic buffer.  No validation to be done.
            //
            break;
    }

    //
    // The class install parameters have been validated.  Allocate a buffer for the
    // new parameter structure.
    //
    if(!(NewParamBuffer = MyMalloc(ClassInstallParamsSize))) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    try {

        CopyMemory(NewParamBuffer,
                   ClassInstallParams,
                   ClassInstallParamsSize
                  );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        MyFree(NewParamBuffer);
        NewParamBuffer = NULL;
    }

    if(!NewParamBuffer) {
        //
        // Then an error occurred and we couldn't store the new parameters.
        //
        return ERROR_INVALID_PARAMETER;
    }

    if(DevInstParamBlock->ClassInstallHeader) {
        MyFree(DevInstParamBlock->ClassInstallHeader);
    }
    DevInstParamBlock->ClassInstallHeader = (PSP_CLASSINSTALL_HEADER)NewParamBuffer;
    DevInstParamBlock->ClassInstallParamsSize = ClassInstallParamsSize;
    DevInstParamBlock->Flags |= DI_CLASSINSTALLPARAMS;

    return NO_ERROR;
}


DWORD
GetDrvInstallParams(
    IN  PDRIVER_NODE          DriverNode,
    OUT PSP_DRVINSTALL_PARAMS DriverInstallParams
    )
/*++

Routine Description:

    This routine fills in a SP_DRVINSTALL_PARAMS structure based on the
    driver node supplied

    Note:  The supplied DriverInstallParams structure must have its cbSize
    field filled in correctly, or the call will fail.

Arguments:

    DriverNode - Supplies the address of the driver node containing the
        installation parameters to be retrieved.

    DriverInstallParams - Supplies the address of a SP_DRVINSTALL_PARAMS
        structure that will receive the installation parameters.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

NOTE:

    This routine _does not_ set the Win98-compatible DNF_CLASS_DRIVER or
    DNF_COMPATIBLE_DRIVER flags that indicate whether or not the driver node is
    from a class or compatible driver list, respectively.

--*/
{
    if(DriverInstallParams->cbSize != sizeof(SP_DRVINSTALL_PARAMS)) {
        return ERROR_INVALID_USER_BUFFER;
    }

    //
    // Copy the parameters.
    //
    DriverInstallParams->Rank = DriverNode->Rank;
    DriverInstallParams->Flags = DriverNode->Flags;
    DriverInstallParams->PrivateData = DriverNode->PrivateData;

    //
    // The 'Reserved' field of the SP_DRVINSTALL_PARAMS structure isn't currently
    // used.
    //

    return NO_ERROR;
}


DWORD
SetDrvInstallParams(
    IN  PSP_DRVINSTALL_PARAMS DriverInstallParams,
    OUT PDRIVER_NODE          DriverNode
    )
/*++

Routine Description:

    This routine sets the driver installation parameters for the specified
    driver node based on the caller-supplied SP_DRVINSTALL_PARAMS structure.

    Note:  The supplied DriverInstallParams structure must have its cbSize
    field filled in correctly, or the call will fail.

Arguments:

    DriverInstallParams - Supplies the address of a SP_DRVINSTALL_PARAMS
        structure containing the installation parameters to be used.

    DriverNode - Supplies the address of the driver node whose installation
        parameters are to be set.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, an ERROR_* code is returned.

--*/
{
    if(DriverInstallParams->cbSize != sizeof(SP_DRVINSTALL_PARAMS)) {
        return ERROR_INVALID_USER_BUFFER;
    }

    //
    // Validate the flags.
    //
    if(DriverInstallParams->Flags & DNF_FLAGS_ILLEGAL) {
        return ERROR_INVALID_FLAGS;
    }

    //
    // No validation currently being done on Rank and PrivateData fields.
    //
    // We're ready to copy the parameters.
    //
    DriverNode->Rank = DriverInstallParams->Rank;
    DriverNode->PrivateData = DriverInstallParams->PrivateData;
    //
    // Ignore attempts at modifying read-only flags.
    //
    DriverNode->Flags = (DriverInstallParams->Flags & ~DNF_FLAGS_READONLY) |
                        (DriverNode->Flags          &  DNF_FLAGS_READONLY);

    return NO_ERROR;
}


LONG
AddMultiSzToStringTable(
    IN  PVOID   StringTable,
    IN  PTCHAR  MultiSzBuffer,
    OUT PLONG   StringIdList,
    IN  DWORD   StringIdListSize,
    IN  BOOL    CaseSensitive,
    OUT PTCHAR *UnprocessedBuffer    OPTIONAL
    )
/*++

Routine Description:

    This routine adds every string in the MultiSzBuffer to the specified
    string table, and stores the resulting IDs in the supplied output buffer.

Arguments:

    StringTable - Supplies the handle of the string table to add the strings to.

    MultiSzBuffer - Supplies the address of the REG_MULTI_SZ buffer containing
        the strings to be added.

    StringIdList - Supplies the address of an array of LONGs that receives the
        list of IDs for the added strings (the ordering of the IDs in this
        list will be the same as the ordering of the strings in the MultiSzBuffer.

    StringIdListSize - Supplies the size, in LONGs, of the StringIdList.  If the
        number of strings in MultiSzBuffer exceeds this amount, then only the
        first StringIdListSize strings will be added, and the position in the
        buffer where processing was halted will be stored in UnprocessedBuffer.

    CaseSensitive - Specifies whether the string should be added case-sensitively.

    UnprocessedBuffer - Optionally, supplies the address of a character pointer
        that receives the position where processing was aborted because the
        StringIdList buffer was filled.  If all strings in the MultiSzBuffer were
        processed, then this pointer will be set to NULL.

Return Value:

    If successful, the return value is the number of strings added.
    If failure, the return value is -1 (this happens if a string cannot be
    added because of an out-of-memory condition).

--*/
{
    PTSTR CurString;
    LONG StringCount = 0;

    for(CurString = MultiSzBuffer;
        (*CurString && (StringCount < (LONG)StringIdListSize));
        CurString += (lstrlen(CurString)+1)) {

        if((StringIdList[StringCount] = pStringTableAddString(
                                            StringTable,
                                            CurString,
                                            CaseSensitive
                                                ? STRTAB_CASE_SENSITIVE
                                                : STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                            NULL,0
                                           )) == -1)
        {
            StringCount = -1;
            break;
        }

        StringCount++;
    }

    if(UnprocessedBuffer) {
        *UnprocessedBuffer = (*CurString ? CurString : NULL);
    }

    return StringCount;
}


LONG
LookUpStringInDevInfoSet(
    IN HDEVINFO DeviceInfoSet,
    IN PTSTR    String,
    IN BOOL     CaseSensitive
    )
/*++

Routine Description:

    This routine looks up the specified string in the string table associated with
    the specified device information set.

Arguments:

    DeviceInfoSet - Supplies the pointer to the device information set containing
        the string table to look the string up in.

    String - Specifies the string to be looked up.  This string is not specified as
        const, so that the lookup routine may modify it (i.e., lower-case it) without
        having to allocate a temporary buffer.

    CaseSensitive - If TRUE, then a case-sensitive lookup is performed, otherwise, the
        lookup is case-insensitive.

Return Value:

    If the function succeeds, the return value is the string's ID in the string table.
    device information set.

    If the function fails, the return value is -1.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    LONG StringId;
    DWORD StringLen;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        return -1;
    }

    try {

        StringId = pStringTableLookUpString(pDeviceInfoSet->StringTable,
                                            String,
                                            &StringLen,
                                            NULL,
                                            NULL,
                                            STRTAB_BUFFER_WRITEABLE |
                                                (CaseSensitive ? STRTAB_CASE_SENSITIVE
                                                               : STRTAB_CASE_INSENSITIVE),
                                            NULL,0
                                           );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        StringId = -1;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    return StringId;
}


BOOL
ShouldClassBeExcluded(
    IN LPGUID ClassGuid,
    IN BOOL   ExcludeNoInstallClass
    )
/*++

Routine Description:

    This routine determines whether a class should be excluded from
    some operation, based on whether it has a NoInstallClass or
    NoUseClass value entry in its registry key.

Arguments:

    ClassGuidString - Supplies the address of the class GUID to be
        filtered.

    ExcludeNoInstallClass - TRUE if NoInstallClass classes should be
        excluded and FALSE if they should not be excluded.

Return Value:

    If the class should be excluded, the return value is TRUE, otherwise
    it is FALSE.

--*/
{
    HKEY hk;
    BOOL ExcludeClass = FALSE;

    if((hk = SetupDiOpenClassRegKey(ClassGuid, KEY_READ)) != INVALID_HANDLE_VALUE) {

        try {

            if(RegQueryValueEx(hk,
                               pszNoUseClass,
                               NULL,
                               NULL,
                               NULL,
                               NULL) == ERROR_SUCCESS) {

                ExcludeClass = TRUE;

            } else if (ExcludeNoInstallClass &&
                       RegQueryValueEx(hk,
                                  pszNoInstallClass,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL) == ERROR_SUCCESS) {

                ExcludeClass = TRUE;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            //
            // Nothing to do.
            //
            ;
        }

        RegCloseKey(hk);
    }

    return ExcludeClass;
}


BOOL
ClassGuidFromInfVersionNode(
    IN  PINF_VERSION_NODE VersionNode,
    OUT LPGUID            ClassGuid
    )
/*++

Routine Description:

    This routine retrieves the class GUID for the INF whose version node
    is specified.  If the version node doesn't have a ClassGUID value,
    then the Class value is retrieved, and all class GUIDs matching this
    class name are retrieved.  If there is exactly 1 match found, then
    this GUID is returned, otherwise, the routine fails.

Arguments:

    VersionNode - Supplies the address of an INF version node that
        must contain either a ClassGUID or Class entry.

    ClassGuid - Supplies the address of the variable that receives the
        class GUID.

Return Value:

    If a class GUID was retrieved, the return value is TRUE, otherwise,
    it is FALSE.

--*/
{
    PCTSTR GuidString, NameString;
    DWORD NumGuids;

    if(GuidString = pSetupGetVersionDatum(VersionNode, pszClassGuid)) {

        if(pSetupGuidFromString(GuidString, ClassGuid) == NO_ERROR) {
            return TRUE;
        }

    } else {

        NameString = pSetupGetVersionDatum(VersionNode, pszClass);
        if(NameString &&
           SetupDiClassGuidsFromName(NameString,
                                     ClassGuid,
                                     1,
                                     &NumGuids) && NumGuids) {
            return TRUE;
        }
    }

    return FALSE;
}


DWORD
EnumSingleInf(
    IN     PCTSTR                       InfName,
    IN OUT LPWIN32_FIND_DATA            InfFileData,
    IN     DWORD                        SearchControl,
    IN     PSP_ENUMINF_CALLBACK_ROUTINE EnumInfCallback,
    IN     PSETUP_LOG_CONTEXT           LogContext,
    IN OUT PVOID                        Context
    )
/*++

Routine Description:

    This routine finds and opens the specified INF, and calls the
    supplied callback routine for it.

Arguments:

    InfName - Supplies the name of the INF to call the callback for.

    InfFileData - Supplies data returned from FindFirstFile/FindNextFile
        for this INF.  This parameter is used as input if the
        INFINFO_INF_NAME_IS_ABSOLUTE SearchControl value is specified.
        If any other SearchControl value is specified, then this buffer
        is used to retrieve the Win32 Find Data for the specified INF.

    SearchControl - Specifies where the INF should be searched for.  May
        be one of the following values:

        INFINFO_INF_NAME_IS_ABSOLUTE - Open the specified INF name as-is.
        INFINFO_DEFAULT_SEARCH - Look in INF dir, then System32
        INFINFO_REVERSE_DEFAULT_SEARCH - reverse of the above
        INFINFO_INF_PATH_LIST_SEARCH - search each dir in 'DevicePath' list
                                       (stored in registry).

    EnumInfCallback - Supplies the address of the callback routine
        to use.  The prototype for this callback is as follows:

        typedef BOOL (*PSP_ENUMINF_CALLBACK_ROUTINE) (
            IN     PCTSTR                  InfFullPath,
            IN     LPWIN32_FIND_DATA       InfFileData,
            IN     PSETUP_LOG_CONTEXT      LogContext,
            IN OUT LPVOID                  Context
            );

        The callback routine returns TRUE to continue enumeration,
        or FALSE to abort it.

    Context - Supplies the address of a buffer that the callback may
        use to retrieve/return data.

Return Value:

    If the function succeeds, and the enumeration callback returned
    TRUE (continue enumeration), the return value is NO_ERROR.

    If the function succeeds, and the enumeration callback returned
    FALSE (abort enumeration), the return value is ERROR_CANCELLED.

    If the function fails, the return value is an ERROR_* status code.

--*/
{
    TCHAR PathBuffer[MAX_PATH];
    PCTSTR InfFullPath;
    DWORD Err;

    if(SearchControl == INFINFO_INF_NAME_IS_ABSOLUTE) {
        InfFullPath = InfName;
    } else {
        //
        // The specified INF name should be searched for based
        // on the SearchControl type.
        //
        if(Err = SearchForInfFile(InfName,
                                  InfFileData,
                                  SearchControl,
                                  PathBuffer,
                                  SIZECHARS(PathBuffer),
                                  NULL) != NO_ERROR) {
            return Err;
        } else {
            InfFullPath = PathBuffer;
        }
    }

    //
    // Call the supplied callback routine.
    //
    Err = EnumInfCallback(InfFullPath, InfFileData, LogContext, Context) ? NO_ERROR : ERROR_CANCELLED;

    return Err;
}


DWORD
EnumInfsInDirPathList(
    IN     PCTSTR                       DirPathList,   OPTIONAL
    IN     DWORD                        SearchControl,
    IN     PSP_ENUMINF_CALLBACK_ROUTINE EnumInfCallback,
    IN     BOOL                         IgnoreNonCriticalErrors,
    IN     PSETUP_LOG_CONTEXT           LogContext,
    IN OUT PVOID                        Context
    )
/*++

Routine Description:

    This routine enumerates all INFs present in the search list specified
    by SearchControl, and calls the supplied callback routine for each.

Arguments:

    DirPathList - Optionally, specifies the search path listing all
        directories to be enumerated.  This string may contain multiple
        paths, separated by semicolons (;).  If this parameter is not
        specified, then the SearchControl value will determine the
        search path to be used.

    SearchControl - Specifies the set of directories to be enumerated.
        If SearchPath is specified, this parameter is ignored.  May be
        one of the following values:

        INFINFO_DEFAULT_SEARCH : enumerate %windir%\inf, then
            %windir%\system32

        INFINFO_REVERSE_DEFAULT_SEARCH : reverse of the above

        INFINFO_INF_PATH_LIST_SEARCH : enumerate INFs in each of the
            directories listed in the DevicePath value entry under:

            HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion.

    EnumInfCallback - Supplies the address of the callback routine
        to use.  The prototype for this callback is as follows:

        typedef BOOL (*PSP_ENUMINF_CALLBACK_ROUTINE) (
            IN     PCTSTR                  InfFullPath,
            IN     LPWIN32_FIND_DATA       InfFileData,
            IN     PSETUP_LOG_CONTEXT      LogContext,
            IN OUT LPVOID                  Context
            );

        The callback must return TRUE to continue enumeration, or
        FALSE to abort.

    IgnoreNonCriticalErrors - If TRUE, then all errors are ignored
        except those that prevent enumeration from continuing.

    Context - Supplies the address of a buffer that the callback may
        use to retrieve/return data.

Return Value:

    If the function succeeds, and enumeration has not been aborted,
    then the return value is NO_ERROR.

    If the function succeeds, and enumeration has been aborted,
    then the return value is ERROR_CANCELLED.

    If the function fails, the return value is an ERROR_* status code.

--*/
{
    DWORD Err = NO_ERROR;
    PCTSTR PathList, CurPath;
    BOOL FreePathList = TRUE;

    if(DirPathList) {
        //
        // Use the specified search path(s).
        //
        PathList = GetFullyQualifiedMultiSzPathList(DirPathList);

    } else if(SearchControl == INFINFO_INF_PATH_LIST_SEARCH) {
        //
        // Use our global list of INF search paths.
        //
        PathList = InfSearchPaths;
        FreePathList = FALSE;

    } else {
        //
        // Retrieve the path list.
        //
        PathList = AllocAndReturnDriverSearchList(SearchControl);
    }

    if(!PathList) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    //
    // Now enumerate the INFs in each path in our MultiSz list.
    //
    for(CurPath = PathList;
        *CurPath;
        CurPath += lstrlen(CurPath) + 1) {

        if((Err = EnumInfsInDirectory(CurPath,
                                      EnumInfCallback,
                                      IgnoreNonCriticalErrors,
                                      LogContext,
                                      Context)) != NO_ERROR) {
            break;
        }
    }

    if(FreePathList) {
        MyFree(PathList);
    }

clean0:

    if((Err == ERROR_CANCELLED) || !IgnoreNonCriticalErrors) {
        return Err;
    } else {
        return NO_ERROR;
    }
}


DWORD
EnumInfsInDirectory(
    IN     PCTSTR                       DirPath,
    IN     PSP_ENUMINF_CALLBACK_ROUTINE EnumInfCallback,
    IN     BOOL                         IgnoreNonCriticalErrors,
    IN     PSETUP_LOG_CONTEXT           LogContext,
    IN OUT PVOID                        Context
    )
/*++

Routine Description:

    This routine enumerates all of the INF files in the specified directory,
    and calls the supplied callback routine for each one.

Arguments:

    DirPath - Supplies the (fully-qualified) path of the directory to be enumerated.

    EnumInfCallback - Supplies the address of the callback routine
        to use.  The prototype for this callback is as follows:

        typedef BOOL (*PSP_ENUMINF_CALLBACK_ROUTINE) (
            IN     PCTSTR                  InfFullPath,
            IN     LPWIN32_FIND_DATA       InfFileData,
            IN     PSETUP_LOG_CONTEXT      LogContext,
            IN OUT LPVOID                  Context
            );

        The callback must return TRUE to continue enumeration, or
        FALSE to abort.

    IgnoreNonCriticalErrors - If TRUE, then all errors are ignored
        except those that prevent enumeration from continuing.

    Context - Supplies the address of a buffer that the callback may
        use to retrieve/return data.

Return Value:

    If the function succeeds, and enumeration has not been aborted,
    then the return value is NO_ERROR.

    If the function succeeds, and enumeration has been aborted,
    then the return value is ERROR_CANCELLED.

    If the function fails, the return value is an ERROR_* status code.

--*/
{
    TCHAR  PathBuffer[MAX_PATH];
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    DWORD Err;
    PTSTR CurrentInfFile;
    DWORD slot_enum = 0;

    //
    // Build a file spec to find all INFs in specified directory
    // (i.e., <DirPath>\*.INF)
    //
    lstrcpy(PathBuffer, DirPath);
    ConcatenatePaths(PathBuffer,
                     pszInfWildcard,
                     SIZECHARS(PathBuffer),
                     NULL
                    );

    slot_enum = AllocLogInfoSlot(LogContext,FALSE);
    //
    // Log what directory we're looking in.
    //
    WriteLogEntry(
        LogContext,
        slot_enum,
        MSG_LOG_ENUMERATING_FILES,
        NULL,
        PathBuffer);

    FindHandle = FindFirstFile(PathBuffer, &FindData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        Err = GetLastError();
        goto clean0;
    } else {
        //
        // Get a pointer to the end of the path part of the string
        // (minus the wildcard filename), so that we can append
        // each filename to it.
        //
        CurrentInfFile = _tcsrchr(PathBuffer, TEXT('\\')) + 1;
    }

    try {

        do {
            //
            // Build the full pathname.
            //
            lstrcpy(CurrentInfFile, FindData.cFileName);

            //
            // Now, enumerate this INF.
            //
            if((Err = EnumSingleInf(PathBuffer,
                                    &FindData,
                                    INFINFO_INF_NAME_IS_ABSOLUTE,
                                    EnumInfCallback,
                                    LogContext,
                                    Context)) != NO_ERROR) {

                if((Err == ERROR_CANCELLED) || !IgnoreNonCriticalErrors) {
                    break;
                }
            }

            if(FindNextFile(FindHandle, &FindData)) {
                Err = NO_ERROR;
            } else {
                Err = GetLastError();
            }

        } while(Err == NO_ERROR);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_DATA;
    }

    FindClose(FindHandle);

clean0:

    if(slot_enum) {
        ReleaseLogInfoSlot(LogContext,slot_enum);
    }

    if((Err == ERROR_CANCELLED) || !IgnoreNonCriticalErrors) {
        return Err;
    } else {
        return NO_ERROR;
    }
}


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
    )
/*++

Routine Description:

    This routine creates a new driver node, and initializes it with
    the supplied information.

Arguments:

    Rank - The rank match of the driver node being created.  This is a
        value in [0..n], where a lower number indicates a higher level of
        compatibility between the driver represented by the node, and the
        device being installed.

    DevDescription - Supplies the description of the device that will be
        supported by this driver.

    DrvDescription - Supplies the description of this driver.

    ProviderName - Supplies the name of the provider of this INF.

    MfgName - Supplies the name of the manufacturer of this device.

    InfDate - Supplies the address of the variable containing the date
        when the INF was last written to.

    InfFileName - Supplies the full name of the INF file for this driver.

    InfSectionName - Supplies the name of the install section in the INF
        that would be used to install this driver.

    StringTable - Supplies the string table that the specified strings are
        to be added to.

    InfClassGuidIndex - Supplies the index into the containing HDEVINFO set's
        GUID table where the class GUID for this INF is stored.

    DriverNode - Supplies the address of a DRIVER_NODE pointer that will
        receive a pointer to the newly-allocated node.

Return Value:

    If the function succeeds, the return value is NO_ERROR, otherwise the
    ERROR_* code is returned.

--*/
{
    PDRIVER_NODE pDriverNode;
    DWORD Err = ERROR_NOT_ENOUGH_MEMORY;
    TCHAR TempString[MAX_PATH];  // an INF path is the longest string we'll store in here.

    if(!(pDriverNode = MyMalloc(sizeof(DRIVER_NODE)))) {
        return Err;
    }

    //
    // Initialize the various fields in the driver node structure.
    //
    ZeroMemory(pDriverNode, sizeof(DRIVER_NODE));

    pDriverNode->Rank = Rank;
    pDriverNode->InfDate = *InfDate;
    pDriverNode->HardwareId = -1;

    pDriverNode->GuidIndex = InfClassGuidIndex;

    //
    // Now, add the strings to the associated string table, and store the string IDs.
    //
    // Cast the DrvDescription string being added case-sensitively as PTSTR instead of PCTSTR.
    // Case sensitive string additions don't modify the buffer passed in, so we're safe in
    // doing so.
    //
    if((pDriverNode->DrvDescription = pStringTableAddString(StringTable,
                                                            (PTSTR)DrvDescription,
                                                            STRTAB_CASE_SENSITIVE,
                                                            NULL,0)) == -1) {
        goto clean0;
    }

    //
    // For DevDescription, ProviderName, and MfgName, we use the string table IDs to do fast
    // comparisons for driver nodes.  Thus, we need to store case-insensitive IDs.  However,
    // these strings are also used for display, so we have to store them in their case-sensitive
    // form as well.
    //
    // We must first copy the strings into a modifiable buffer, since we're going to need to add
    // them case-insensitively.
    //
    lstrcpyn(TempString, DevDescription, SIZECHARS(TempString));
    if((pDriverNode->DevDescriptionDisplayName = pStringTableAddString(StringTable,
                                                                       TempString,
                                                                       STRTAB_CASE_SENSITIVE,
                                                                       NULL,0)) == -1) {
        goto clean0;
    }

    if((pDriverNode->DevDescription = pStringTableAddString(
                                          StringTable,
                                          TempString,
                                          STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                          NULL,0)) == -1) {
        goto clean0;
    }

    if(ProviderName) {
        lstrcpyn(TempString, ProviderName, SIZECHARS(TempString));
        if((pDriverNode->ProviderDisplayName = pStringTableAddString(
                                                    StringTable,
                                                    TempString,
                                                    STRTAB_CASE_SENSITIVE,
                                                    NULL,0)) == -1) {
            goto clean0;
        }

        if((pDriverNode->ProviderName = pStringTableAddString(
                                            StringTable,
                                            TempString,
                                            STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                            NULL,0)) == -1) {
            goto clean0;
        }

    } else {
        pDriverNode->ProviderName = pDriverNode->ProviderDisplayName = -1;
    }

    lstrcpyn(TempString, MfgName, SIZECHARS(TempString));
    if((pDriverNode->MfgDisplayName = pStringTableAddString(StringTable,
                                                            TempString,
                                                            STRTAB_CASE_SENSITIVE,
                                                            NULL,0)) == -1) {
        goto clean0;
    }

    if((pDriverNode->MfgName = pStringTableAddString(
                                    StringTable,
                                    TempString,
                                    STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                    NULL,0)) == -1) {
        goto clean0;
    }

    lstrcpyn(TempString, InfFileName, SIZECHARS(TempString));
    if((pDriverNode->InfFileName = pStringTableAddString(
                                        StringTable,
                                        TempString,
                                        STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                        NULL,0)) == -1) {
        goto clean0;
    }

    //
    // Add INF section name case-sensitively, since we may have a legacy driver node, which requires
    // that the original case be maintained.
    //
    if((pDriverNode->InfSectionName = pStringTableAddString(StringTable,
                                                            (PTSTR)InfSectionName,
                                                            STRTAB_CASE_SENSITIVE,
                                                            NULL,0)) == -1) {
        goto clean0;
    }

    //
    // If we get to here, then we've successfully stored all strings.
    //
    Err = NO_ERROR;

clean0:

    if(Err == NO_ERROR) {
        *DriverNode = pDriverNode;
    } else {
        DestroyDriverNodes(pDriverNode, (PDEVICE_INFO_SET)NULL);
    }

    return Err;
}

BOOL
_RemoveCDMDirectory(
    PTSTR Path
    )
/*++

Routine Description:

    This routine recursively deletes the specified directory and all the
    files in it.


Arguments:

    Path - Path to remove.

Return Value:

    TRUE - if the directory was sucessfully deleted.
    FALSE - if the directory was not successfully deleted.

--*/
{
    WIN32_FIND_DATA FindFileData;
    HANDLE          hFind;
    BOOL            bFind = TRUE;
    BOOL            Ret = TRUE;
    TCHAR           szTemp[MAX_PATH];
    TCHAR           FindPath[MAX_PATH];
    DWORD           dwAttributes;

    //
    //If this is a directory then tack on *.* to the end of the path
    //
    lstrcpy(FindPath, Path);
    dwAttributes = GetFileAttributes(Path);
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        if (FindPath[lstrlen(FindPath)] != TEXT('\\')) {

            lstrcat(FindPath, TEXT("\\*.*"));

        } else {

            lstrcat(FindPath, TEXT("*.*"));
        }
    }

    hFind = FindFirstFile(FindPath, &FindFileData);

    while (hFind != INVALID_HANDLE_VALUE && bFind == TRUE) {

        lstrcpy(szTemp, Path);

        if (szTemp[lstrlen(szTemp) - 1] != TEXT('\\')) {

            lstrcat(szTemp, TEXT("\\"));
        }

        lstrcat(szTemp, FindFileData.cFileName);

        //
        //This is a directory
        //
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (FindFileData.cFileName[0] != TEXT('.'))) {

            if (!_RemoveCDMDirectory(szTemp)) {

                Ret = FALSE;
            }

            RemoveDirectory(szTemp);
        }

        //
        //This is a file
        //
        else if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

            DeleteFile(szTemp);
        }

        bFind = FindNextFile(hFind, &FindFileData);
    }

    FindClose(hFind);

    //
    //Remove the root directory
    //
    dwAttributes = GetFileAttributes(Path);
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        if (!RemoveDirectory(Path)) {

            Ret = FALSE;
        }
    }

    return Ret;
}

BOOL
RemoveCDMDirectory(
  IN PTSTR FullPathName
  )
/*++

Routine Description:

    This routine deletes the Code Download Manager temporary directory. It will
    only delete the directory and all of it's contents if:

    1) it is a subdirectory of the Windows TEMP directory.

    Note that we also assume that this is a full path (including a filename at the end).
    We will strip off the filename and remove the entire directory where this INF file
    is located.

Arguments:

    FullPathName - Full path to the directory that might be deleted.

Return Value:

    TRUE - if the directory was sucessfully deleted.
    FALSE - if the directory was not successfully deleted.

--*/
{
    TCHAR Directory[MAX_PATH];
    TCHAR TempPath[MAX_PATH];
    PTSTR FileName;

    //
    //First stip off the file name so we are just left with the directory.
    //
    lstrcpy(Directory, FullPathName);

    if ((FileName = (PTSTR)MyGetFileTitle((PCTSTR)Directory))) {

        *FileName = TEXT('\0');
    }

    if (Directory[0] == TEXT('\0')) {

        return FALSE;
    }

    if (GetTempPath(sizeof(TempPath) / sizeof(TCHAR), TempPath) == 0) {

        lstrcpy(TempPath, TEXT("UNKNOWN"));
    }

    //
    // Only remove this directory if it is a subdirectory of the TEMP path
    //
    if (_tcsnicmp(TempPath, Directory, sizeof(TempPath) / sizeof(TCHAR))) {

        //
        //Remove the directory
        //
        return _RemoveCDMDirectory(Directory);

    } else {

            TCHAR Debug[512];
            wsprintf(Debug, TEXT("SETUPAPI: RemoveCDMDirectory(%s) -> bogus path\n"), FullPathName);
            OutputDebugString(Debug);
            MYASSERT (FALSE);
    }

    return FALSE;
}

VOID
DestroyDriverNodes(
    IN PDRIVER_NODE DriverNode,
    IN PDEVICE_INFO_SET pDeviceInfoSet
    )
/*++

Routine Description:

    This routine destroys the specified driver node linked list, freeing
    all resources associated with it.

Arguments:

    DriverNode - Supplies a pointer to the head of the driver node linked
    list to be destroyed.

Return Value:

    None.

--*/
{
    PDRIVER_NODE NextNode;
    PTSTR szInfFileName = NULL;

    while(DriverNode) {

        NextNode = DriverNode->Next;

        if(DriverNode->CompatIdList) {
            MyFree(DriverNode->CompatIdList);
        }

        //
        // If this driver was from the Internet then we want to delete the directory where
        // it lives.
        //
        if (pDeviceInfoSet && (DriverNode->Flags & PDNF_CLEANUP_SOURCE_PATH)) {

            szInfFileName = NULL;
            szInfFileName = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                     DriverNode->InfFileName);

            if (*szInfFileName) {

                RemoveCDMDirectory(szInfFileName);
            }
        }

        MyFree(DriverNode);

        DriverNode = NextNode;
    }
}


PTSTR
GetFullyQualifiedMultiSzPathList(
    IN PCTSTR PathList
    )
/*++

Routine Description:

    This routine takes a list of semicolon-delimited directory paths, and returns a
    newly-allocated buffer containing a multi-sz list of those paths, fully qualified.
    The buffer returned from this routine must be freed with MyFree().

Arguments:

    PathList - list of directories to be converted.

Return Value:

    If the function succeeds, the return value is a pointer to the allocated buffer
    containing the multi-sz list.

    If failure (due to out-of-memory), the return value is NULL.

--*/
{
    TCHAR PathListBuffer[MAX_PATH + 1];  // extra char 'cause this is a multi-sz list
    PTSTR CurPath, CharPos, NewBuffer, TempPtr;
    DWORD RequiredSize;

    //
    // First, convert this semicolon-delimited list into a multi-sz list.
    //
    lstrcpy(PathListBuffer, PathList);
    RequiredSize = DelimStringToMultiSz(PathListBuffer,
                                        SIZECHARS(PathListBuffer),
                                        TEXT(';')
                                       );

    RequiredSize = (RequiredSize * MAX_PATH * sizeof(TCHAR)) + sizeof(TCHAR);

    if(!(NewBuffer = MyMalloc(RequiredSize * sizeof(TCHAR)))) {
        return NULL;
    }

    //
    // Now fill in the buffer with the fully-qualified directory paths.
    //
    CharPos = NewBuffer;

    for(CurPath = PathListBuffer; *CurPath; CurPath += (lstrlen(CurPath) + 1)) {

        RequiredSize = GetFullPathName(CurPath,
                                       MAX_PATH,
                                       CharPos,
                                       &TempPtr
                                      );
        if(!RequiredSize || (RequiredSize >= MAX_PATH)) {
            //
            // If we start failing because MAX_PATH isn't big enough anymore, we
            // wanna know about it!
            //
            MYASSERT(RequiredSize < MAX_PATH);
            MyFree(NewBuffer);
            return NULL;
        }

        CharPos += (RequiredSize + 1);
    }

    *(CharPos++) = TEXT('\0');  // add extra NULL to terminate the multi-sz list.

    //
    // Trim this buffer down to just the size required (this should never fail, but
    // it's no big deal if it does).
    //
    if(TempPtr = MyRealloc(NewBuffer, (DWORD)((PBYTE)CharPos - (PBYTE)NewBuffer))) {
        return TempPtr;
    }

    return NewBuffer;
}


BOOL
InitMiniIconList(
    VOID
    )
/*++

Routine Description:

    This routine initializes the global mini-icon list, including setting up
    the synchronization lock.  When this global structure is no longer needed,
    DestroyMiniIconList must be called.

Arguments:

    None.

Return Value:

    If the function succeeds, the return value is TRUE, otherwise it is FALSE.

--*/
{
    ZeroMemory(&GlobalMiniIconList, sizeof(MINI_ICON_LIST));
    return InitializeSynchronizedAccess(&GlobalMiniIconList.Lock);
}


BOOL
DestroyMiniIconList(
    VOID
    )
/*++

Routine Description:

    This routine destroys the global mini-icon list created by a call to
    InitMiniIconList.

Arguments:

    None.

Return Value:

    If the function succeeds, the return value is TRUE, otherwise it is FALSE.

--*/
{

    if(LockMiniIconList(&GlobalMiniIconList)) {
        DestroyMiniIcons();
        DestroySynchronizedAccess(&GlobalMiniIconList.Lock);
        return TRUE;
    }

    return FALSE;
}


DWORD
GetModuleEntryPoint(
    IN  HKEY                  hk,                  OPTIONAL
    IN  LPCTSTR               RegistryValue,
    IN  LPCTSTR               DefaultProcName,
    OUT HINSTANCE            *phinst,
    OUT FARPROC              *pEntryPoint,
    OUT BOOL                 *pMustAbort,          OPTIONAL
    IN  PSETUP_LOG_CONTEXT    LogContext,          OPTIONAL
    IN  HWND                  Owner,               OPTIONAL
    IN  SetupapiVerifyProblem Problem,
    IN  LPCTSTR               DeviceDesc,          OPTIONAL
    IN  DWORD                 DriverSigningPolicy,
    IN  DWORD                 NoUI
    )
/*++

Routine Description:

    This routine is used to retrieve the procedure address of a specified
    function in a specified module.

Arguments:

    hk - Optionally, supplies an open registry key that contains a value entry
        specifying the module (and optionally, the entry point) to be retrieved.
        If this parameter is not specified (set to INVALID_HANDLE_VALUE), then
        the RegistryValue parameter is interpreted as the data itself, instead
        of the value containing the entry.

    RegistryValue - If hk is supplied, this specifies the name of the registry
        value that contains the module and entry point information.  Otherwise,
        it contains the actual data specifying the module/entry point to be
        used.

    DefaultProcName - Supplies the name of a default procedure to use if one
        is not specified in the registry value.

    phinst - Supplies the address of a variable that receives a handle to the
        specified module, if it is successfully loaded and the entry point found.

    pEntryPoint - Supplies the address of a function pointer that receives the
        specified entry point in the loaded module.

    pMustAbort - Optionally, supplies the address of a boolean variable that is
        set upon return to indicate whether a failure (i.e., return code other
        than NO_ERROR) should abort the device installer action underway.  This
        variable is always set to FALSE when the function succeeds.

        If this argument is not supplied, then the arguments below are ignored.

    LogContext - Optionally, supplies the log context to be used when logging
        entries into the setupapi logfile.  Not used if pMustAbort isn't
        specified.

    Owner - Optionally, supplies window to own driver signing dialogs, if any.
        Not used if pMustAbort isn't specified.

    Problem - Supplies the problem type to use if driver signing error occurs.
        Not used if pMustAbort isn't specified.

    DeviceDesc - Optionally, supplies the device description to use if driver
        signing error occurs.  Not used if pMustAbort isn't specified.

    DriverSigningPolicy - Supplies policy to be employed if a driver signing
        error is encountered.  Not used if pMustAbort isn't specified.

    NoUI - Set to true if driver signing popups are to be suppressed (e.g.,
        because the user has previously responded to a warning dialog and
        elected to proceed.  Not used if pMustAbort isn't specified.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the specified value entry could not be found, the return value is
    ERROR_DI_DO_DEFAULT.
    If any other error is encountered, an ERROR_* code is returned.

Remarks:

    This function is useful for loading a class installer or property provider,
    and receiving the procedure address specified.  The syntax of the registry
    entry is: value=dll[,proc name] where dll is the name of the module to load,
    and proc name is an optional procedure to search for.  If proc name is not
    specified, the procedure specified by DefaultProcName will be used.

--*/
{
    DWORD Err;
    DWORD RegDataType, BufferSize;
    TCHAR TempBuffer[MAX_PATH];
    TCHAR ModulePath[MAX_PATH];
#if UNICODE
    //
    // this used to be ModulePath reused... to reduce chance of breaking anything, kept size the same
    // I've seperated this out as we want ModulePath for logging
    //
    CHAR ProcBuffer[MAX_PATH*sizeof(TCHAR)];
#endif
    PTSTR StringPtr;
    PSTR  ProcName;   // ANSI-only, because it's used for GetProcAddress.

    *phinst = NULL;
    *pEntryPoint = NULL;

    if(pMustAbort) {
        *pMustAbort = FALSE;
    }

    if(hk != INVALID_HANDLE_VALUE) {
        //
        // See if the specified value entry is present (and of the right data type).
        //
        BufferSize = sizeof(TempBuffer);
        if((RegQueryValueEx(hk,
                            RegistryValue,
                            NULL,
                            &RegDataType,
                            (PBYTE)TempBuffer,
                            &BufferSize) != ERROR_SUCCESS) ||
           (RegDataType != REG_SZ)) {

            return ERROR_DI_DO_DEFAULT;
        }

    } else {
        //
        // Copy the specified data into the buffer as if we'd just retrieved it from
        // the registry.
        //
        BufferSize = (lstrlen(RegistryValue) + 1) * sizeof(TCHAR);
        CopyMemory(TempBuffer, RegistryValue, BufferSize);
    }

    lstrcpyn(ModulePath, SystemDirectory, MAX_PATH);

    //
    // Find the beginning of the entry point name, if present.
    //
    for(StringPtr = TempBuffer + ((BufferSize / sizeof(TCHAR)) - 2);
        StringPtr >= TempBuffer;
        StringPtr--) {

        if(*StringPtr == TEXT(',')) {
            *(StringPtr++) = TEXT('\0');
            break;
        }
        //
        // If we hit a double-quote mark, then set the character pointer
        // to the beginning of the string so we'll terminate the search.
        //
        if(*StringPtr == TEXT('\"')) {
            StringPtr = TempBuffer;
        }
    }

    if(StringPtr > TempBuffer) {
        //
        // We encountered a comma in the string.  Scan forward from that point
        // to ensure that there aren't any leading spaces in the entry point
        // name.
        //
        for(; (*StringPtr && IsWhitespace(StringPtr)); StringPtr++);

        if(!(*StringPtr)) {
            //
            // Then there was no entry point given after all.
            //
            StringPtr = TempBuffer;
        }
    }

    ConcatenatePaths(ModulePath, TempBuffer, MAX_PATH, NULL);

    //
    // If requested, check the digital signature of this module before loading
    // it.
    //
    if(pMustAbort) {

        Err = VerifyFile(LogContext,
                         NULL,
                         NULL,
                         0,
                         MyGetFileTitle(ModulePath),
                         ModulePath,
                         NULL,
                         NULL,
                         FALSE,
                         NULL,
                         NULL,
                         NULL
                        );

        if(Err != NO_ERROR) {

            if(!HandleFailedVerification(Owner,
                                         Problem,
                                         ModulePath,
                                         DeviceDesc,
                                         DriverSigningPolicy,
                                         NoUI,
                                         Err,
                                         LogContext,
                                         NULL,
                                         NULL)) {
                //
                // The operation should be aborted.
                //
                *pMustAbort = TRUE;
                return Err;
            }
        }
    }

    if(!(*phinst = LoadLibrary(ModulePath))) {
        Err = GetLastError();
        if (LogContext) {
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                MSG_LOG_MOD_LOADFAIL_ERROR,
                NULL,
                ModulePath);
            WriteLogError(
                LogContext,
                DRIVER_LOG_ERROR,
                Err);
        }
        return Err;
    }

    //
    // We've successfully loaded the module, now get the entry point.
    // (GetProcAddress is an ANSI-only API, so if we're compiled UNICODE,
    // we have to convert the proc name to ANSI here.
    //
#ifdef UNICODE
    ProcName = ProcBuffer;
#endif

    if(StringPtr > TempBuffer) {
        //
        // An entry point was specified in the value entry--use it instead
        // of the default provided.
        //
#ifdef UNICODE
        WideCharToMultiByte(CP_ACP,
                            0,
                            StringPtr,
                            -1,
                            ProcName,
                            sizeof(ProcBuffer),
                            NULL,
                            NULL
                           );
#else // !UNICODE
        ProcName = StringPtr;
#endif // !UNICODE

    } else {
        //
        // No entry point was specified--use default.
        //
#ifdef UNICODE
        WideCharToMultiByte(CP_ACP,
                            0,
                            DefaultProcName,
                            -1,
                            ProcName,
                            sizeof(ProcBuffer),
                            NULL,
                            NULL
                           );
#else // !UNICODE
        ProcName = (PSTR)DefaultProcName;
#endif // !UNICODE

    }

    if(!(*pEntryPoint = (FARPROC)GetProcAddress(*phinst, ProcName))) {
        Err = GetLastError();
        FreeLibrary(*phinst);
        *phinst = NULL;
        if (LogContext) {
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                MSG_LOG_MOD_PROCFAIL_ERROR,
                NULL,
                ModulePath,
                (StringPtr > TempBuffer ? StringPtr : DefaultProcName));
            WriteLogError(
                LogContext,
                DRIVER_LOG_ERROR,
                Err);
        }
        return Err;
    }
    if (LogContext) {
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_VERBOSE1,
            MSG_LOG_MOD_LIST_PROC,
            NULL,
            ModulePath,
            (StringPtr > TempBuffer ? StringPtr : DefaultProcName));
    }

    return NO_ERROR;
}


DWORD
pSetupGuidFromString(
    IN  PCTSTR GuidString,
    OUT LPGUID Guid
    )
/*++

Routine Description:

    This routine converts the character representation of a GUID into its binary
    form (a GUID struct).  The GUID is in the following form:

    {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}

    where 'x' is a hexadecimal digit.

Arguments:

    GuidString - Supplies a pointer to the null-terminated GUID string.  The

    Guid - Supplies a pointer to the variable that receives the GUID structure.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, the return value is RPC_S_INVALID_STRING_UUID.

--*/
{
    TCHAR UuidBuffer[GUID_STRING_LEN - 1];

    //
    // Since we're using a RPC UUID routine, we need to strip off the surrounding
    // curly braces first.
    //
    if(*GuidString++ != TEXT('{')) {
        return RPC_S_INVALID_STRING_UUID;
    }

    lstrcpyn(UuidBuffer, GuidString, SIZECHARS(UuidBuffer));

    if((lstrlen(UuidBuffer) != GUID_STRING_LEN - 2) ||
       (UuidBuffer[GUID_STRING_LEN - 3] != TEXT('}'))) {

        return RPC_S_INVALID_STRING_UUID;
    }

    UuidBuffer[GUID_STRING_LEN - 3] = TEXT('\0');

    return ((UuidFromString(UuidBuffer, Guid) == RPC_S_OK) ? NO_ERROR : RPC_S_INVALID_STRING_UUID);
}


DWORD
pSetupStringFromGuid(
    IN  CONST GUID *Guid,
    OUT PTSTR       GuidString,
    IN  DWORD       GuidStringSize
    )
/*++

Routine Description:

    This routine converts a GUID into a null-terminated string which represents
    it.  This string is of the form:

    {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}

    where x represents a hexadecimal digit.

    This routine comes from ole32\common\ccompapi.cxx.  It is included here to avoid linking
    to ole32.dll.  (The RPC version allocates memory, so it was avoided as well.)

Arguments:

    Guid - Supplies a pointer to the GUID whose string representation is
        to be retrieved.

    GuidString - Supplies a pointer to character buffer that receives the
        string.  This buffer must be _at least_ 39 (GUID_STRING_LEN) characters
        long.

Return Value:

    If success, the return value is NO_ERROR.
    if failure, the return value is

--*/
{
    CONST BYTE *GuidBytes;
    INT i;

    if(GuidStringSize < GUID_STRING_LEN) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    GuidBytes = (CONST BYTE *)Guid;

    *GuidString++ = TEXT('{');

    for(i = 0; i < sizeof(GuidMap); i++) {

        if(GuidMap[i] == '-') {
            *GuidString++ = TEXT('-');
        } else {
            *GuidString++ = szDigits[ (GuidBytes[GuidMap[i]] & 0xF0) >> 4 ];
            *GuidString++ = szDigits[ (GuidBytes[GuidMap[i]] & 0x0F) ];
        }
    }

    *GuidString++ = TEXT('}');
    *GuidString   = TEXT('\0');

    return NO_ERROR;
}


BOOL
pSetupIsGuidNull(
    IN CONST GUID *Guid
    )
{
    return IsEqualGUID(Guid, &GUID_NULL);
}


VOID
GetRegSubkeysFromDeviceInterfaceName(
    IN OUT PTSTR  DeviceInterfaceName,
    OUT    PTSTR *SubKeyName
    )
/*++

Routine Description:

    This routine breaks up a device interface path into 2 parts--the symbolic link
    name and the (optional) reference string.  It then munges the symbolic link
    name part into the subkey name as it appears under the interface class key.

    NOTE: The algorithm for parsing the device interface name must be kept in sync
    with the kernel-mode implementation of IoOpenDeviceInterfaceRegistryKey.

Arguments:

    DeviceInterfaceName - Supplies the name of the device interface to be parsed
        into registry subkey names.  Upon return, this name will have been terminated
        at the backslash preceding the reference string (if there is one), and all
        backslashes will have been replaced with '#' characters.

    SubKeyName - Supplies the address of a character pointer that receives the
        address of the reference string (within the DeviceInterfaceName string).
        If there is no reference string, this parameter will be filled in with NULL.

Return Value:

    none

--*/
{
    PTSTR p;

    //
    // Scan across the name to find the beginning of the refstring component (if
    // there is one).  The format of the symbolic link name is:
    //
    // \\?\munged_name[\refstring]
    //
    MYASSERT(DeviceInterfaceName[0] == TEXT('\\'));
    MYASSERT(DeviceInterfaceName[1] == TEXT('\\'));
    //
    // Allow both '\\.\' and '\\?\' for now, since Memphis currently uses the former.
    //
    MYASSERT((DeviceInterfaceName[2] == TEXT('?')) || (DeviceInterfaceName[2] == TEXT('.')));
    MYASSERT(DeviceInterfaceName[3] == TEXT('\\'));

    p = _tcschr(&(DeviceInterfaceName[4]), TEXT('\\'));

    if(p) {
        *p = TEXT('\0');
        *SubKeyName = p + 1;
    } else {
        *SubKeyName = NULL;
    }

    for(p = DeviceInterfaceName; *p; p++) {
        if(*p == TEXT('\\')) {
            *p = TEXT('#');
        }
    }
}


LONG
OpenDeviceInterfaceSubKey(
    IN     HKEY   hKeyInterfaceClass,
    IN     PCTSTR DeviceInterfaceName,
    IN     REGSAM samDesired,
    OUT    PHKEY  phkResult,
    OUT    PTSTR  OwningDevInstName,    OPTIONAL
    IN OUT PDWORD OwningDevInstNameSize OPTIONAL
    )
/*++

Routine Description:

    This routine munges the specified device interface symbolic link name into
    a subkey name that is then opened underneath the specified interface class key.

    NOTE:  This munging algorithm must be kept in sync with the kernel-mode routines
    that generate these keys (e.g., IoRegisterDeviceInterface).

Arguments:

    hKeyInterfaceClass - Supplies the handle of the currently-open interface class key
        under which the device interface subkey is to be opened.

    DeviceInterfaceName - Supplies the symbolic link name ('\\?\' form) of the device
        interface for which the subkey is to be opened.

    samDesired - Specifies the access desired on the key to be opened.

    phkResult - Supplies the address of a variable that receives the registry handle,
        if successfully opened.

    OwningDevInstName - Optionally, supplies a character buffer that receives the name
        of the device instance that owns this interface.

    OwningDevInstNameSize - Optionally, supplies the address of a variable that, on input,
        contains the size of the OwningDevInstName buffer (in bytes).  Upon return, it
        receives that actual number of bytes stored in OwningDevInstName (including
        terminating NULL).

Return Value:

    If success, the return value is ERROR_SUCCESS.
    if failure, the return value is either ERROR_NOT_ENOUGH_MEMORY, ERROR_MORE_DATA, or
    ERROR_NO_SUCH_INTERFACE_DEVICE.

--*/
{
    DWORD BufferLength;
    LONG Err;
    PTSTR TempBuffer = NULL, RefString;
    TCHAR NoRefStringSubKeyName[2];
    HKEY hKey;
    DWORD RegDataType;

    Err = ERROR_SUCCESS;
    hKey = INVALID_HANDLE_VALUE;

    try {
        //
        // We need to allocate a temporary buffer to hold the symbolic link name while we munge it.
        //
        BufferLength = (lstrlen(DeviceInterfaceName) + 1) * sizeof(TCHAR);

        if(!(TempBuffer = MyMalloc(BufferLength))) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        memcpy(TempBuffer, DeviceInterfaceName, BufferLength);

        //
        // Parse this device interface name into the (munged) symbolic link name and
        // (optional) refstring.
        //
        GetRegSubkeysFromDeviceInterfaceName(TempBuffer, &RefString);

        //
        // Now open the symbolic link subkey under the interface class key.
        //
        if(ERROR_SUCCESS != RegOpenKeyEx(hKeyInterfaceClass,
                                         TempBuffer,
                                         0,
                                         KEY_READ,
                                         &hKey)) {
            //
            // Ensure the key handle is still invalid, so we won't try to free it.
            //
            hKey = INVALID_HANDLE_VALUE;
            Err = ERROR_NO_SUCH_DEVICE_INTERFACE;
            goto clean0;
        }

        //
        // If the caller requested it, retrieve the device instance that owns this interface.
        //
        if(OwningDevInstName) {

            Err = RegQueryValueEx(hKey,
                                  pszDeviceInstance,
                                  NULL,
                                  &RegDataType,
                                  (LPBYTE)OwningDevInstName,
                                  OwningDevInstNameSize
                                 );

            if((Err != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
                if(Err != ERROR_MORE_DATA) {
                    Err = ERROR_NO_SUCH_DEVICE_INTERFACE;
                }
                goto clean0;
            }
        }

        //
        // Now open up the subkey representing the particular 'instance' of this interface
        // (this is based on the refstring).
        //
        if(RefString) {
            //
            // Back up the pointer one character.  We know we're somewhere within TempBuffer
            // (but not at the beginning) so this is safe.
            //
            RefString--;
        } else {
            RefString = NoRefStringSubKeyName;
            NoRefStringSubKeyName[1] = TEXT('\0');
        }
        *RefString = TEXT('#');

        if(ERROR_SUCCESS != RegOpenKeyEx(hKey,
                                         RefString,
                                         0,
                                         samDesired,
                                         phkResult)) {

            Err = ERROR_NO_SUCH_DEVICE_INTERFACE;
            goto clean0;
        }

clean0:
        ; // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Access the following variables so that the compiler will respect statement
        // ordering w.r.t. their assignment.
        //
        TempBuffer = TempBuffer;
        hKey = hKey;
    }

    if(TempBuffer) {
        MyFree(TempBuffer);
    }

    if(hKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKey);
    }

    return Err;
}


LONG
AddOrGetGuidTableIndex(
    IN PDEVICE_INFO_SET  DeviceInfoSet,
    IN CONST GUID       *ClassGuid,
    IN BOOL              AddIfNotPresent
    )
/*++

Routine Description:

    This routine retrieves the index of a class GUID within the devinfo set's GUID
    list (optionally, adding the GUID if not already present).
    This is used to allow DWORD comparisons instead of 16-byte GUID comparisons
    (and to save space).

Arguments:

    DeviceInfoSet - Supplies a pointer to the device information set containing the
        list of class GUIDs for which an index is to be retrieved.

    InterfaceClassGuid - Supplies a pointer to the GUID for which an index is to
        be added/retrieved.

    AddIfNotPresent - If TRUE, the class GUID will be added to the list if it's not
        already there.

Return Value:

    If success, the return value is an index into the devinfo set's GuidTable array.

    If failure, the return value is -1.  If adding, this indicates an out-of-memory
    condition.  If simply retrieving, then this indicates that the GUID is not in
    the list.

--*/
{
    LONG i;
    LPGUID NewGuidList;

    for(i = 0; (DWORD)i < DeviceInfoSet->GuidTableSize; i++) {

        if(IsEqualGUID(ClassGuid, &(DeviceInfoSet->GuidTable[i]))) {
            return i;
        }
    }

    if(AddIfNotPresent) {

        if(DeviceInfoSet->GuidTable) {
            NewGuidList = MyRealloc(DeviceInfoSet->GuidTable, (i + 1) * sizeof(GUID));
        } else {
            NewGuidList = MyMalloc(sizeof(GUID));
        }

        if(NewGuidList) {

            CopyMemory(&(NewGuidList[i]),
                       ClassGuid,
                       sizeof(GUID)
                      );

            DeviceInfoSet->GuidTable     = NewGuidList;
            DeviceInfoSet->GuidTableSize = i + 1;

            return i;

        } else {
            //
            // We couldn't allocate/grow the list; return -1 indicating an out-of-memory condition.
            //
            return -1;
        }

    } else {
        //
        // We didn't find the interface class GUID in our list, and we aren't supposed
        // to add it.
        //
        return -1;
    }
}


PINTERFACE_CLASS_LIST
AddOrGetInterfaceClassList(
    IN PDEVICE_INFO_SET DeviceInfoSet,
    IN PDEVINFO_ELEM    DevInfoElem,
    IN LONG             InterfaceClassGuidIndex,
    IN BOOL             AddIfNotPresent
    )
/*++

Routine Description:

    This routine retrieves the interface device list of the specified class that
    is 'owned' by the specified devinfo element.  This list can optionally be
    created if it doesn't already exist.

Arguments:

    DeviceInfoSet - Supplies a pointer to the device information set containing the
        devinfo element for which an interface device list is to be retrieved.

    DevInfoElem - Supplies a pointer to the devinfo element for which an interface
        device list is to be retrieved.

    InterfaceClassGuidIndex - Supplies the index of the interface class GUID within
        the hdevinfo set's InterfaceClassGuidList array.

    AddIfNotPresent - If TRUE, then a new interface device list of the specified class
        will be created for this devinfo element, if it doesn't already exist.

Return Value:

    If successful, the return value is a pointer to the requested interface device list
    for this devinfo element.

    If failure, the return value is NULL.  If AddIfNotPresent is TRUE, then this
    indicates an out-of-memory condition, otherwise, it indicates that the requested
    interface class list was not present for the devinfo element.

--*/
{
    DWORD i;
    PINTERFACE_CLASS_LIST NewClassList;

    for(i = 0; i < DevInfoElem->InterfaceClassListSize; i++) {

        if(DevInfoElem->InterfaceClassList[i].GuidIndex == InterfaceClassGuidIndex) {

            return (&(DevInfoElem->InterfaceClassList[i]));
        }
    }

    //
    // The requested interface class list doesn't presently exist for this devinfo element.
    //
    if(AddIfNotPresent) {

        if(DevInfoElem->InterfaceClassList) {
            NewClassList = MyRealloc(DevInfoElem->InterfaceClassList, (i + 1) * sizeof(INTERFACE_CLASS_LIST));
        } else {
            NewClassList = MyMalloc(sizeof(INTERFACE_CLASS_LIST));
        }

        if(NewClassList) {

            ZeroMemory(&(NewClassList[i]), sizeof(INTERFACE_CLASS_LIST));

            NewClassList[i].GuidIndex = InterfaceClassGuidIndex;

            DevInfoElem->InterfaceClassList     = NewClassList;
            DevInfoElem->InterfaceClassListSize = i + 1;

            return (&(DevInfoElem->InterfaceClassList[i]));

        } else {
            //
            // We couldn't allocate/grow this list; return NULL indicating an out-of-memory condition.
            //
            return NULL;
        }

    } else {
        //
        // We aren't supposed to add the class list if it doesn't already exist.
        //
        return NULL;
    }
}


BOOL
InterfaceDeviceDataFromNode(
    IN  PINTERFACE_DEVICE_NODE     InterfaceDeviceNode,
    IN  CONST GUID                *InterfaceClassGuid,
    OUT PSP_DEVICE_INTERFACE_DATA  InterfaceDeviceData
    )
/*++

Routine Description:

    This routine fills in a PSP_DEVICE_INTERFACE_DATA structure based
    on the information in the supplied interface device node.

    Note:  The supplied InterfaceDeviceData structure must have its cbSize
    field filled in correctly, or the call will fail.

Arguments:

    InterfaceDeviceNode - Supplies the address of the interface device node
        to be used in filling in the interface device data buffer.

    InterfaceClassGuid - Supplies a pointer to the class GUID for this
        interface device.

    InterfaceDeviceData - Supplies the address of the buffer to retrieve
        the interface device data.

Return Value:

    If the function succeeds, the return value is TRUE, otherwise, it
    is FALSE.

--*/
{
    if(InterfaceDeviceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA)) {
        return FALSE;
    }

    CopyMemory(&(InterfaceDeviceData->InterfaceClassGuid),
               InterfaceClassGuid,
               sizeof(GUID)
              );

    InterfaceDeviceData->Flags = InterfaceDeviceNode->Flags;

    InterfaceDeviceData->Reserved = (ULONG_PTR)InterfaceDeviceNode;

    return TRUE;
}


PDEVINFO_ELEM
FindDevInfoElemForInterfaceDevice(
    IN PDEVICE_INFO_SET          DeviceInfoSet,
    IN PSP_DEVICE_INTERFACE_DATA InterfaceDeviceData
    )
/*++

Routine Description:

    This routine searches through all elements of a device information
    set, looking for one that corresponds to the devinfo element pointer
    stored in the OwningDevInfoElem backpointer of the interface device
    node referenced in the Reserved field of the interface device data.  If a
    match is found, a pointer to the device information element is returned.

Arguments:

    DeviceInfoSet - Specifies the set to be searched.

    InterfaceDeviceData - Supplies a pointer to the interface device data
        for which the corresponding devinfo element is to be returned.

Return Value:

    If a device information element is found, the return value is a
    pointer to that element, otherwise, the return value is NULL.

--*/
{
    PDEVINFO_ELEM DevInfoElem;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode;

    if(InterfaceDeviceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA)) {
        return NULL;
    }

    //
    // The Reserved field contains a pointer to the underlying interface device node.
    //
    InterfaceDeviceNode = (PINTERFACE_DEVICE_NODE)(InterfaceDeviceData->Reserved);

    for(DevInfoElem = DeviceInfoSet->DeviceInfoHead;
        DevInfoElem;
        DevInfoElem = DevInfoElem->Next) {

        if(DevInfoElem == InterfaceDeviceNode->OwningDevInfoElem) {

            return DevInfoElem;
        }
    }

    return NULL;
}


DWORD
MapCrToSpError(
    IN CONFIGRET CmReturnCode,
    IN DWORD     Default
    )
/*++

Routine Description:

    This routine maps some CM error return codes to setup api (Win32) return codes,
    and maps everything else to the value specied by Default.

Arguments:

    CmReturnCode - Specifies the ConfigMgr return code to be mapped.

    Default - Specifies the default value to use if no explicit mapping applies.

Return Value:

    Setup API (Win32) error code.

--*/
{
    switch(CmReturnCode) {

        case CR_SUCCESS :
            return NO_ERROR;

        case CR_CALL_NOT_IMPLEMENTED :
            return ERROR_CALL_NOT_IMPLEMENTED;

        case CR_OUT_OF_MEMORY :
            return ERROR_NOT_ENOUGH_MEMORY;

        case CR_INVALID_POINTER :
            return ERROR_INVALID_USER_BUFFER;

        case CR_INVALID_DEVINST :
            return ERROR_NO_SUCH_DEVINST;

        case CR_INVALID_DEVICE_ID :
            return ERROR_INVALID_DEVINST_NAME;

        case CR_ALREADY_SUCH_DEVINST :
            return ERROR_DEVINST_ALREADY_EXISTS;

        case CR_INVALID_REFERENCE_STRING :
            return ERROR_INVALID_REFERENCE_STRING;

        case CR_INVALID_MACHINENAME :
            return ERROR_INVALID_MACHINENAME;

        case CR_REMOTE_COMM_FAILURE :
            return ERROR_REMOTE_COMM_FAILURE;

        case CR_MACHINE_UNAVAILABLE :
            return ERROR_MACHINE_UNAVAILABLE;

        case CR_NO_CM_SERVICES :
            return ERROR_NO_CONFIGMGR_SERVICES;

        case CR_ACCESS_DENIED :
            return ERROR_ACCESS_DENIED;

        case CR_NOT_DISABLEABLE:
            return ERROR_NOT_DISABLEABLE;

        default :
            return Default;
    }
}


DWORD
AcquireSCMLock(
    IN SC_HANDLE SCMHandle,
    OUT SC_LOCK *pSCMLock
    )
/*++

Routine Description:

    This routine attempts to lock the SCM database.  If it is already locked it will retry
    ACQUIRE_SCM_LOCK_ATTEMPTS times at intervals of ACQUIRE_SCM_LOCK_INTERVAL.

Arguments:

    SCMHandle - supplies a handle to the SCM to lock
    pSCMLock - receives the lock handle

Return Value:

    NO_ERROR if the lock is acquired, otherwise a Win32 error code

Remarks:

    The value of *pSCMLock is guaranteed to be NULL if the lock is not acquired

--*/
{
    DWORD Err;
    ULONG Attempts = ACQUIRE_SCM_LOCK_ATTEMPTS;

    MYASSERT(pSCMLock);
    *pSCMLock = NULL;

    while((*pSCMLock = LockServiceDatabase(SCMHandle)) == NULL && Attempts > 0) {
        //
        // Check if the error is that someone else has locked the SCM
        //
        if((Err = GetLastError()) == ERROR_SERVICE_DATABASE_LOCKED) {
            Attempts--;
            //
            // Sleep for specified time
            //
            Sleep(ACQUIRE_SCM_LOCK_INTERVAL);
        } else {
            //
            // Unrecoverable error occured - return it
            //
            return Err;
        }
    }

    if(!*pSCMLock) {
        //
        // We have been unable to lock the SCM
        //
        return ERROR_SERVICE_DATABASE_LOCKED;
    }

    return NO_ERROR;

}


DWORD
InvalidateHelperModules(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN DWORD            Flags
    )
/*++

Routine Description:

    This routine resets the list of 'helper modules' (class installer, property
    page providers, and co-installers), and either frees them immediately or
    migrates the module handles to the devinfo set's list of things to clean up
    when the HDEVINFO is destroyed.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        a list of 'helper' modules to be invalidated.

    DeviceInfoData - Optionally, specifies a particular device information
        element containing a list of 'helper' modules to be invalidated.  If
        this parameter is not specified, then the list of modules for the
        set itself will be invalidated.

    Flags - Supplies flags that control the behavior of this routine.  May be
        a combination of the following values:

        IHM_COINSTALLERS_ONLY - If this flag is set, only the co-installers list
                                will be invalidated.  Otherwise, the class
                                installer and property page providers will also
                                be invalidated.

        IHM_FREE_IMMEDIATELY  - If this flag is set, then the modules will be
                                freed immediately.  Otherwise, the modules will
                                be added to the HDEVINFO set's list of things to
                                clean up at handle close time.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is ERROR_NOT_ENOUGH_MEMORY.
    (This routine cannot fail if the IHM_FREE_IMMEDIATELY flag is set.)

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err, i;
    PDEVINFO_ELEM DevInfoElem;
    PDEVINSTALL_PARAM_BLOCK InstallParamBlock;
    DWORD NumModulesToInvalidate;
    PMODULE_HANDLE_LIST_NODE NewModuleHandleNode;
    BOOL UnlockDevInfoElem;
    LONG CoInstallerIndex;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        //
        // The handle's no longer valid--the user must've already destroyed the
        // set.  We have nothing to do.
        //
        return NO_ERROR;
    }

    Err = NO_ERROR;
    UnlockDevInfoElem = FALSE;
    DevInfoElem = NULL;
    NewModuleHandleNode = NULL;

    try {
        //
        // If we're invalidating helper modules for a particular devinfo element,
        // then find that element.
        //
        if(DeviceInfoData) {
            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                //
                // The element must've been deleted--we've nothing to do.
                //
                goto clean0;
            }
            InstallParamBlock = &(DevInfoElem->InstallParamBlock);
        } else {
            InstallParamBlock = &(pDeviceInfoSet->InstallParamBlock);
        }

        //
        // Count the number of module handles we need to free/migrate.
        //
        if(InstallParamBlock->CoInstallerCount == -1) {
            NumModulesToInvalidate = 0;
        } else {
            MYASSERT(InstallParamBlock->CoInstallerCount >= 0);
            NumModulesToInvalidate = (DWORD)(InstallParamBlock->CoInstallerCount);
        }

        if(!(Flags & IHM_COINSTALLERS_ONLY)) {
            if(InstallParamBlock->hinstClassInstaller) {
                NumModulesToInvalidate++;
            }
            if(InstallParamBlock->hinstClassPropProvider) {
                NumModulesToInvalidate++;
            }
            if(InstallParamBlock->hinstDevicePropProvider) {
                NumModulesToInvalidate++;
            }
            if(InstallParamBlock->hinstBasicPropProvider) {
                NumModulesToInvalidate++;
            }
        }

        if(NumModulesToInvalidate) {
            //
            // If we can't unload these modules at this time, then create a node to store
            // these module handles until the devinfo set is destroyed.
            //
            if(!(Flags & IHM_FREE_IMMEDIATELY)) {

                NewModuleHandleNode = MyMalloc(offsetof(MODULE_HANDLE_LIST_NODE, ModuleList)
                                               + (NumModulesToInvalidate * sizeof(HINSTANCE))
                                              );

                if(!NewModuleHandleNode) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean0;
                }
            }

            //
            // Give the class installers/co-installers a DIF_DESTROYPRIVATEDATA
            // notification.  NOTE: We don't unlock the HDEVINFO set here, so the
            // class/co-installers can't make any calls where nesting level > 1
            // is disallowed.  This means that SetupDiSelectDevice, for example,
            // will fail if the class installer tries to call it now.  This is
            // necessary, because otherwise it would deadlock.
            //
            if(DevInfoElem) {
                //
                // (Also, lock down the devinfo element, so that the class/co-installers
                // can't make any 'dangerous' calls (e.g., SetupDiDeleteDeviceInfo),
                // during the clean-up notification.)
                //
                if(!(DevInfoElem->DiElemFlags & DIE_IS_LOCKED)) {
                    DevInfoElem->DiElemFlags |= DIE_IS_LOCKED;
                    UnlockDevInfoElem = TRUE;
                }
            }

            _SetupDiCallClassInstaller(DIF_DESTROYPRIVATEDATA,
                                       DeviceInfoSet,
                                       DeviceInfoData,
                                       CALLCI_CALL_HELPERS
                                      );

            //
            // Clear the flag we set above...
            //
            if(UnlockDevInfoElem) {
                DevInfoElem->DiElemFlags &= ~DIE_IS_LOCKED;
                UnlockDevInfoElem = FALSE;
            }

            //
            // Store the module handles in the node we allocated, and link it into the
            // list of module handles associated with this devinfo set.
            //
            i = 0;

            if(!(Flags & IHM_COINSTALLERS_ONLY)) {
                //
                // Either free the modules now, or store them in our 'to do' list...
                //
                if(Flags & IHM_FREE_IMMEDIATELY) {
                    if(InstallParamBlock->hinstClassInstaller) {
                        FreeLibrary(InstallParamBlock->hinstClassInstaller);
                    }
                    if(InstallParamBlock->hinstClassPropProvider) {
                        FreeLibrary(InstallParamBlock->hinstClassPropProvider);
                    }
                    if(InstallParamBlock->hinstDevicePropProvider) {
                        FreeLibrary(InstallParamBlock->hinstDevicePropProvider);
                    }
                    if(InstallParamBlock->hinstBasicPropProvider) {
                        FreeLibrary(InstallParamBlock->hinstBasicPropProvider);
                    }
                } else {
                    if(InstallParamBlock->hinstClassInstaller) {
                        NewModuleHandleNode->ModuleList[i++] = InstallParamBlock->hinstClassInstaller;
                    }
                    if(InstallParamBlock->hinstClassPropProvider) {
                        NewModuleHandleNode->ModuleList[i++] = InstallParamBlock->hinstClassPropProvider;
                    }
                    if(InstallParamBlock->hinstDevicePropProvider) {
                        NewModuleHandleNode->ModuleList[i++] = InstallParamBlock->hinstDevicePropProvider;
                    }
                    if(InstallParamBlock->hinstBasicPropProvider) {
                        NewModuleHandleNode->ModuleList[i++] = InstallParamBlock->hinstBasicPropProvider;
                    }
                }
            }

            for(CoInstallerIndex = 0;
                CoInstallerIndex < InstallParamBlock->CoInstallerCount;
                CoInstallerIndex++)
            {
                if(Flags & IHM_FREE_IMMEDIATELY) {
                    FreeLibrary(InstallParamBlock->CoInstallerList[CoInstallerIndex].hinstCoInstaller);
                } else {
                    NewModuleHandleNode->ModuleList[i++] =
                        InstallParamBlock->CoInstallerList[CoInstallerIndex].hinstCoInstaller;
                }
            }

            //
            // Unless we're freeing these modules immediately, our modules-to-free list
            // index should now match the number of modules we're supposed to be
            // invalidating.
            //
            MYASSERT((Flags & IHM_FREE_IMMEDIATELY) || (i == NumModulesToInvalidate));

            if(!(Flags & IHM_FREE_IMMEDIATELY)) {

                NewModuleHandleNode->ModuleCount = NumModulesToInvalidate;

                NewModuleHandleNode->Next = pDeviceInfoSet->ModulesToFree;
                pDeviceInfoSet->ModulesToFree = NewModuleHandleNode;

                //
                // Now, clear the node pointer, so we won't try to free it if we hit an exception.
                //
                NewModuleHandleNode = NULL;
            }

            //
            // Clear all the module handles (and entry points).  They will be retrieved
            // anew the next time they're needed.
            //
            if(!(Flags & IHM_COINSTALLERS_ONLY)) {
                InstallParamBlock->hinstClassInstaller           = NULL;
                InstallParamBlock->ClassInstallerEntryPoint      = NULL;

                InstallParamBlock->hinstClassPropProvider        = NULL;
                InstallParamBlock->ClassEnumPropPagesEntryPoint  = NULL;

                InstallParamBlock->hinstDevicePropProvider       = NULL;
                InstallParamBlock->DeviceEnumPropPagesEntryPoint = NULL;

                InstallParamBlock->hinstBasicPropProvider        = NULL;
                InstallParamBlock->EnumBasicPropertiesEntryPoint = NULL;
            }

            if(InstallParamBlock->CoInstallerCount != -1) {
                if(InstallParamBlock->CoInstallerList) {
                    MyFree(InstallParamBlock->CoInstallerList);
                    InstallParamBlock->CoInstallerList = NULL;
                }
            }
        }

        //
        // Set the co-installer count back to -1, even if their weren't any co-installers
        // to unload.  That will ensure that we'll re-load the co-installers for the next
        // class installer request we receive.
        //
        InstallParamBlock->CoInstallerCount = -1;

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // We should never encounter an exception, but if we do, just make sure we
        // do any necessary clean-up.  Don't return an error in this case--the only
        // error this routine is supposed to return is out-of-memory.
        //
        if(UnlockDevInfoElem) {
            MYASSERT(DevInfoElem);
            DevInfoElem->DiElemFlags &= ~DIE_IS_LOCKED;
        }
        if(NewModuleHandleNode) {
            MyFree(NewModuleHandleNode);
        }
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    return Err;
}


DWORD
DoInstallActionWithParams(
    IN DI_FUNCTION             InstallFunction,
    IN HDEVINFO                DeviceInfoSet,
    IN PSP_DEVINFO_DATA        DeviceInfoData,         OPTIONAL
    IN OUT PSP_CLASSINSTALL_HEADER ClassInstallParams,     OPTIONAL
    IN DWORD                   ClassInstallParamsSize,
    IN DWORD                   Flags
    )
/*++

Routine Description:

    This routine performs a requested installation action, using the specified
    class install parameters.  Any existing class install parameters are
    preserved.

Arguments:

    InstallFunction - Specifies the DIF_* action to be performed.

    DeviceInfoSet - Supplies a handle to the device information set for which
        the installation action is to be performed.

    DeviceInfoData - Optionally, supplies the address of a device information
        structure specifying a particular element for which the installation
        action is to be performed.

    ClassInstallParams - Optionally, supplies the address of a class install
        parameter buffer to be used for this action.  If this parameter is not
        specified, then no class install params will be available to the class
        installer during this call (even if there were pre-existing parameters
        coming into this function).

    ClassInstallParamsSize - Supplies the size, in bytes, of the ClassInstallParams
        buffer, or zero if ClassInstallParams is not specified.

    Flags - Supplies flags that control the behavior of this routine.  May be
        a combination of the following values:

        INSTALLACTION_CALL_CI - Call the class installer for this action request.

        INSTALLACTION_NO_DEFAULT - Don't perform the default action (if this flag
            is specified without INSTALLACTION_CALL_CI, then this routine is a
            no-op).

Return Value:

    If the request was handled successfully, the return value is NO_ERROR.

    If the request was not handled (but no error occurred), the return value is
    ERROR_DI_DO_DEFAULT.

    Otherwise, the return value is a Win32 error code indicating the cause of
    failure.

--*/
{
    PBYTE OldCiParams;
    DWORD OldCiParamsSize, Err;
    SP_PROPCHANGE_PARAMS PropChangeParams;
    SP_DEVINSTALL_PARAMS DevInstallParams;
    DWORD FlagsToClear;

    //
    // Retrieve any existing class install parameters, then write out
    // parameters for DIF_PROPERTYCHANGE.
    //
    OldCiParams = NULL;
    OldCiParamsSize = 0;

    while(!SetupDiGetClassInstallParams(DeviceInfoSet,
                                        DeviceInfoData,
                                        (PSP_CLASSINSTALL_HEADER)OldCiParams,
                                        OldCiParamsSize,
                                        &OldCiParamsSize)) {
        Err = GetLastError();

        //
        // Before going any further, free our existing buffer (if there is one).
        //
        if(OldCiParams) {
            MyFree(OldCiParams);
            OldCiParams = NULL;
        }

        if(Err == ERROR_INSUFFICIENT_BUFFER) {
            //
            // Allocate a buffer of the size required, and try again.
            //
            MYASSERT(OldCiParamsSize >= sizeof(SP_CLASSINSTALL_HEADER));

            if(!(OldCiParams = MyMalloc(OldCiParamsSize))) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }

            ((PSP_CLASSINSTALL_HEADER)OldCiParams)->cbSize = sizeof(SP_CLASSINSTALL_HEADER);

        } else {
            //
            // Treat any other error as if there are no class install params
            // (since ERROR_NO_CLASSINSTALL_PARAMS is really the only error
            // we should ever see here anyway).
            //
            OldCiParamsSize = 0;
            break;
        }
    }

    //
    // Retrieve the device install params for the set or element we're working with.
    //
    DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if(!SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                      DeviceInfoData,
                                      &DevInstallParams)) {
        Err = GetLastError();
        goto clean0;
    }

    FlagsToClear = 0;

    //
    // It's possible that the class install params we just retrieved are 'turned off'
    // (i.e., the DI_CLASSINSTALLPARAMS bit is cleared).  Check for that condition now,
    // so we can restore the parameters to the same state later.
    //
    if(OldCiParams && !(DevInstallParams.Flags & DI_CLASSINSTALLPARAMS)) {
        FlagsToClear |= DI_CLASSINSTALLPARAMS;
    }

    //
    // If the caller doesn't want us to do the default action, then check to see whether
    // we need to temporarily set the DI_NODI_DEFAULTACTION flag.
    //
    if((Flags & INSTALLACTION_NO_DEFAULT) &&
       !(DevInstallParams.Flags & DI_NODI_DEFAULTACTION)) {

        FlagsToClear |= DI_NODI_DEFAULTACTION;

        DevInstallParams.Flags |= DI_NODI_DEFAULTACTION;

        if(!SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                          DeviceInfoData,
                                          &DevInstallParams)) {
            Err = GetLastError();
            goto clean0;
        }
    }

    if(!SetupDiSetClassInstallParams(DeviceInfoSet,
                                     DeviceInfoData,
                                     ClassInstallParams,
                                     ClassInstallParamsSize)) {

        Err = GetLastError();
        goto clean1;
    }

    //
    // OK, now call the class installer.
    //
    if(_SetupDiCallClassInstaller(InstallFunction,
                                  DeviceInfoSet,
                                  DeviceInfoData,
                                  (Flags & INSTALLACTION_CALL_CI) ? (CALLCI_LOAD_HELPERS | CALLCI_CALL_HELPERS) : 0)) {
        //
        // mission accomplished
        //
        Err = NO_ERROR;
    } else {
        Err = GetLastError();
    }

    //
    // Save the class install params results in the ClassInstallParams
    // value that was passed in.
    //
    if (ClassInstallParams) {

        SetupDiGetClassInstallParams(DeviceInfoSet,
                                     DeviceInfoData,
                                     ClassInstallParams,
                                     ClassInstallParamsSize,
                                     NULL);
    }

    //
    // Restore the previous class install params.
    //
    SetupDiSetClassInstallParams(DeviceInfoSet,
                                 DeviceInfoData,
                                 (PSP_CLASSINSTALL_HEADER)OldCiParams,
                                 OldCiParamsSize
                                );

clean1:

    if(FlagsToClear) {

        if(SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                         DeviceInfoData,
                                         &DevInstallParams)) {

            DevInstallParams.Flags &= ~FlagsToClear;

            SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                          DeviceInfoData,
                                          &DevInstallParams
                                         );
        }
    }

clean0:

    if(OldCiParams) {
        MyFree(OldCiParams);
    }

    return Err;
}


BOOL
GetBestDeviceDesc(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,  OPTIONAL
    OUT PTSTR            DeviceDescBuffer
    )
/*++

Routine Description:

    This routine retrieves the best possible description to be displayed for
    the specified devinfo set or element (e.g., for driver signing popups). We
    will try to retrieve this string by doing the following things (in order)
    until one of them succeeds:

        1.  If there's a selected driver, retrieve the DeviceDesc in that
            driver node.
        2.  If this is for a device information element, then use devnode's
            DeviceDesc property.
        3.  Retrieve the description of the class (via
            SetupDiGetClassDescription).
        4.  Use the (localized) string "Unknown driver software package".

    ASSUMES THAT THE CALLING ROUTINE HAS ALREADY ACQUIRED THE LOCK!

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set for which
        a description is to be retrieved (unless DeviceInfoData is also
        supplied, in which case we retrieve the description for that particular
        element instead.

    DeviceInfoData - Optionally, supplies the device information element for
        which a description is to be retrieved.

    DeviceDescBuffer - Supplies the address of a character buffer that must be
        at least LINE_LEN characters long.  Upon successful return, this buffer
        will be filled in with a device description

Return Value:

    TRUE if some description was retrieved, FALSE otherwise.

--*/
{
    SP_DRVINFO_DATA DriverInfoData;
    GUID ClassGuid;
    BOOL b;

    //
    // First, see if there's a selected driver for this device information set
    // or element.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if(SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &DriverInfoData)) {
        //
        // Copy the description into the caller-supplied buffer and return.
        //
        lstrcpy(DeviceDescBuffer, DriverInfoData.Description);
        return TRUE;
    }

    //
    // OK, next try to retrieve the DeviceDesc property (if we're working on a
    // device information element.
    //
    if(DeviceInfoData) {

        if(SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                            DeviceInfoData,
                                            SPDRP_DEVICEDESC,
                                            NULL,
                                            (PBYTE)DeviceDescBuffer,
                                            LINE_LEN * sizeof(TCHAR),
                                            NULL)) {
            return TRUE;
        }
    }

    //
    // Next, try to retrieve the class's friendly name.
    //
    if(DeviceInfoData) {
        CopyMemory(&ClassGuid, &(DeviceInfoData->ClassGuid), sizeof(GUID));
    } else {
        b = SetupDiGetDeviceInfoListClass(DeviceInfoSet, &ClassGuid);
        MYASSERT(b);
        if(!b) {
            return FALSE;
        }
    }

    if(SetupDiGetClassDescription(&ClassGuid,
                                  DeviceDescBuffer,
                                  LINE_LEN,
                                  NULL)) {
        return TRUE;

    } else {
        //
        // We have a class that isn't already installed.  Therefore, we just
        // give it a generic description.
        //
        if(LoadString(MyDllModuleHandle,
                      IDS_UNKNOWN_DRIVER,
                      DeviceDescBuffer,
                      LINE_LEN)) {

            return TRUE;
        }
    }

    return FALSE;
}

