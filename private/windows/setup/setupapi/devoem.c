/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    devoem.c

Abstract:

    Device Installer functions for dealing with OEM drivers.

Author:

    Lonny McMichael (lonnym) 10-Aug-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


BOOL
WINAPI
SetupDiAskForOEMDisk(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )
/*++

Routine Description:

    This routine displays a dialog asking for the path to an OEM install disk.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device being installed.

    DeviceInfoData - Optionally, supplies the address of a SP_DEVINFO_DATA
        structure for the device being installed.  If this parameter is not
        specified, then the driver being installed is associated with the
        global class driver list of the device information set itself.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the user cancels the dialog, the return value is FALSE, and GetLastError
    will return ERROR_CANCELLED.
    If the function fails, the return value is FALSE, and GetLastError returns
    an ERROR_* code.

Remarks:

    This routine will allow browsing of local and network drives for OEM install
    files.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    TCHAR Title[MAX_TITLE_LEN];
    PDEVINSTALL_PARAM_BLOCK dipb;
    TCHAR PathBuffer[MAX_PATH];
    UINT PromptResult;
    LONG DriverPathId;

    //
    // Make sure we're running interactively.
    //
    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        SetLastError(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
        return FALSE;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {

        if(DeviceInfoData) {
            //
            // Then we are to prompt for an OEM driver for a particular device.
            //
            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }
            dipb = &(DevInfoElem->InstallParamBlock);
        } else {
            dipb = &(pDeviceInfoSet->InstallParamBlock);
        }

        if(!LoadString(MyDllModuleHandle,
                       IDS_OEMTITLE,
                       Title,
                       SIZECHARS(Title))) {
            Title[0] = TEXT('\0');
        }

        PromptResult = SetupPromptForDisk(dipb->hwndParent,
                                          (*Title) ? Title : NULL,
                                          NULL,
                                          pszOemInfDefaultPath,
                                          pszInfWildcard,
                                          NULL,
                                          IDF_OEMDISK | IDF_NOCOMPRESSED | IDF_NOSKIP,
                                          PathBuffer,
                                          SIZECHARS(PathBuffer),
                                          NULL
                                         );

        if(PromptResult == DPROMPT_CANCEL) {
            Err = ERROR_CANCELLED;
        } else {
            //
            // A choice was made--replace old path with new one.
            //
            if((DriverPathId = pStringTableAddString(
                                   pDeviceInfoSet->StringTable,
                                   PathBuffer,
                                   STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                   NULL,0)) == -1) {

                Err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                dipb->DriverPath = DriverPathId;

            }
        }


clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiSelectOEMDrv(
    IN     HWND             hwndParent,    OPTIONAL
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )
/*++

Routine Description:

    This routine selects a driver for a device using an OEM path supplied by
    the user.

Arguments:

    hwndParent - Optionally, supplies a window handle that will be the parent
        of any dialogs created during this routine.  This parameter may be
        used to override the hwndParent field in the install parameters block
        of the specified device information set or element.

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device being installed.

    DeviceInfoData - Optionally, supplies the address of a SP_DEVINFO_DATA
        structure for the device being installed.  If this parameter is not
        specified, then the driver being installed is associated with the
        global class driver list of the device information set itself.

        This is an IN OUT parameter because the class GUID of this device
        information element will be updated upon return to reflect the class
        of the most-compatible driver found, if a compatible driver list was
        built.

Return Value:

    If the function succeeds (i.e., a driver is selected successfully), the
    return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    This routine will first ask the user for the OEM path, and will then call
    the class installer to select a driver from that OEM path.

--*/

{
    DWORD Err;

    //
    // Make sure we're running interactively.
    //
    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        SetLastError(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
        return FALSE;
    }

    Err = SelectOEMDriver(hwndParent, DeviceInfoSet, DeviceInfoData, FALSE);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


DWORD
SelectOEMDriver(
    IN     HWND             hwndParent,     OPTIONAL
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN     BOOL             IsWizard
    )
/*++

Routine Description:

    This is the worker routine that actually allows for the selection of an OEM driver.

Arguments:

    hwndParent - Optionally, supplies the window handle that is to be the parent for any
        selection UI.  If this parameter is not supplied, then the hwndParent field of
        the devinfo set or element will be used.

    DeviceInfoSet - Supplies the handle of the device info set for which an OEM driver
        selection is to be performed.

    DeviceInfoData - Optionally, supplies the address of the device information element to
        select a driver for.  If this parameter is not supplied, then an OEM driver for
        the global class driver list will be selected.

        If a compatible driver was found for this device, the device information element
        will have its class GUID updated upon return to reflect the device's new class.

    IsWizard - Specifies whether this routine is being called in the context of a select
        device wizard page.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is an ERROR_* code.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem = NULL;
    PDEVINSTALL_PARAM_BLOCK dipb;
    DWORD Err = NO_ERROR;
    HWND hwndSave;
    LONG DriverPathSave;
    DWORD DriverPathFlagsSave;
    BOOL bRestoreHwnd = FALSE, bRestoreDriverPath = FALSE, bUnlockDevInfoElem = FALSE;
    BOOL bDontSave = FALSE;
    UINT NewClassDriverCount;
    BOOL bAskAgain = TRUE;
    TCHAR Title[MAX_TITLE_LEN];
    DWORD SavedFlags;
    HCURSOR hOldCursor;

    PDRIVER_NODE lpOrgCompat;
    PDRIVER_NODE lpOrgCompatTail;
    UINT         OrgCompatCount;
    PDRIVER_NODE lpOrgClass;
    PDRIVER_NODE lpOrgClassTail;
    UINT         OrgClassCount;
    PDRIVER_NODE lpOrgSel;
    DWORD        dwOrgSelType;
    DWORD        dwOrgFlags;
    DWORD        dwOrgFlagsEx;
    BOOL         bRestoreDeviceInfo = FALSE;

    pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet);

    try {

        if(DeviceInfoData) {
            //
            // Then we're working with a particular device.
            //
            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }

            //
            // If the DevInfoElem isn't already locked, then lock it now, because
            // we're going to be calling the class installer, and we don't want to
            // allow it to delete this element!
            //
            if(!(DevInfoElem->DiElemFlags & DIE_IS_LOCKED)) {
                DevInfoElem->DiElemFlags |= DIE_IS_LOCKED;
                bUnlockDevInfoElem = TRUE;
            }

            dipb = &(DevInfoElem->InstallParamBlock);

        } else {
            dipb = &(pDeviceInfoSet->InstallParamBlock);
        }

        //
        // Make this selection window the parent window for OEM stuff
        //
        if(hwndParent) {
            hwndSave = dipb->hwndParent;
            dipb->hwndParent = hwndParent;
            bRestoreHwnd = TRUE;
        }

        //
        // Don't assume there is no old OEM path.  Save old one and
        // pretend there is no old one in case of cancel.
        //
        DriverPathSave = dipb->DriverPath;
        dipb->DriverPath = -1;

        //
        // Clear the DI_ENUMSINGLEINF flag, because we're going to be getting
        // a path to a directory, _not_ to an individual INF.  Also, clear the
        // DI_COMPAT_FROM_CLASS flag, because we don't want to build the compatible
        // driver list based on any class driver list.
        //
        DriverPathFlagsSave = dipb->Flags & (DI_ENUMSINGLEINF | DI_COMPAT_FROM_CLASS);
        dipb->Flags &= ~(DI_ENUMSINGLEINF | DI_COMPAT_FROM_CLASS);
        bRestoreDriverPath = TRUE;

        //
        // DO NOT break out of the following while loop unless bAskAgain is set to FALSE.
        // There is a check after this loop that assumes that if bAskAgain is still TRUE,
        // then an error occurred in SetupDiAskForOEMDisk, and GetLastError() is called
        // to determine what that error was.
        //
        while(bAskAgain && SetupDiAskForOEMDisk(DeviceInfoSet, DeviceInfoData)) {

            bAskAgain = FALSE;

            //
            // Save the Original List info, in case we get
            // an empty list on the user's selected path.
            //
            // (Note: we don't attempt to save/restore our driver enumeration
            // hints.)
            //
            if(!bDontSave) {

                if(DevInfoElem) {
                    lpOrgCompat     = DevInfoElem->CompatDriverHead;
                    lpOrgCompatTail = DevInfoElem->CompatDriverTail;
                    OrgCompatCount  = DevInfoElem->CompatDriverCount;

                    lpOrgClass      = DevInfoElem->ClassDriverHead;
                    lpOrgClassTail  = DevInfoElem->ClassDriverTail;
                    OrgClassCount   = DevInfoElem->ClassDriverCount;

                    lpOrgSel        = DevInfoElem->SelectedDriver;
                    dwOrgSelType    = DevInfoElem->SelectedDriverType;
                } else {
                    lpOrgClass      = pDeviceInfoSet->ClassDriverHead;
                    lpOrgClassTail  = pDeviceInfoSet->ClassDriverTail;
                    OrgClassCount   = pDeviceInfoSet->ClassDriverCount;

                    lpOrgSel        = pDeviceInfoSet->SelectedClassDriver;
                    dwOrgSelType    = lpOrgSel ? SPDIT_CLASSDRIVER : SPDIT_NODRIVER;
                }

                dwOrgFlags = dipb->Flags;
                dwOrgFlagsEx = dipb->FlagsEx;

                bRestoreDeviceInfo = TRUE;
            }

            if(DevInfoElem) {
                DevInfoElem->CompatDriverHead = DevInfoElem->CompatDriverTail = NULL;
                DevInfoElem->CompatDriverCount = 0;
                DevInfoElem->CompatDriverEnumHint = NULL;
                DevInfoElem->CompatDriverEnumHintIndex = INVALID_ENUM_INDEX;

                DevInfoElem->ClassDriverHead = DevInfoElem->ClassDriverTail = NULL;
                DevInfoElem->ClassDriverCount = 0;
                DevInfoElem->ClassDriverEnumHint = NULL;
                DevInfoElem->ClassDriverEnumHintIndex = INVALID_ENUM_INDEX;

                DevInfoElem->SelectedDriver = NULL;
                DevInfoElem->SelectedDriverType = SPDIT_NODRIVER;
            } else {
                lpOrgCompat     = NULL; // just so we won't ever try to free this list.

                pDeviceInfoSet->ClassDriverHead = pDeviceInfoSet->ClassDriverTail = NULL;
                pDeviceInfoSet->ClassDriverCount = 0;
                pDeviceInfoSet->ClassDriverEnumHint = NULL;
                pDeviceInfoSet->ClassDriverEnumHintIndex = INVALID_ENUM_INDEX;

                pDeviceInfoSet->SelectedClassDriver = NULL;
            }

            SavedFlags = dipb->Flags & (DI_SHOWOEM | DI_NODI_DEFAULTACTION);

            dipb->Flags   &= ~(DI_DIDCOMPAT | DI_DIDCLASS | DI_MULTMFGS | DI_SHOWOEM);
            dipb->FlagsEx &= ~(DI_FLAGSEX_DIDINFOLIST | DI_FLAGSEX_DIDCOMPATINFO);

            if(IsWizard) {
                //
                // We don't want default action taken in the wizard case.
                //
                dipb->Flags |= DI_NODI_DEFAULTACTION;
            }

            //
            // Unlock the HDEVINFO before handling the Select Device.  Otherwise, our
            // multi-threaded dialog will deadlock!
            //
            UnlockDeviceInfoSet(pDeviceInfoSet);
            pDeviceInfoSet = NULL;

            if(_SetupDiCallClassInstaller(DIF_SELECTDEVICE,
                                          DeviceInfoSet,
                                          DeviceInfoData,
                                          CALLCI_LOAD_HELPERS | CALLCI_CALL_HELPERS)) {
                Err = NO_ERROR;
            } else {
                Err = GetLastError();
            }

            //
            // Now, re-acquire the lock on our device information set.
            //
            pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet);
            MYASSERT(pDeviceInfoSet);

            //
            // Restore the saved flags.
            //
            dipb->Flags = (dipb->Flags & ~(DI_SHOWOEM | DI_NODI_DEFAULTACTION)) | SavedFlags;

            //
            // If the class installer returned ERROR_DI_DO_DEFAULT, then
            // they either did not process the DIF_SELECTDEVICE, or they
            // have setup our device info structure with an OEM INF.
            //
            switch(Err) {

                case ERROR_DI_DO_DEFAULT :
                    //
                    // This case is only handled if we're in a wizard.  Otherwise, send it down
                    // for default processing.
                    //
                    if(!IsWizard) {
                        goto DefaultHandling;
                    }

                    //
                    // This will be the most likely return, since we are not allowing the
                    // default handler to be called.  So we will build a new class Drv list
                    // If it is empty we will ask again, otherwise we will accept the new
                    // selection and go on.
                    //
                    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

                    SetupDiBuildDriverInfoList(DeviceInfoSet,
                                               DeviceInfoData,
                                               SPDIT_CLASSDRIVER
                                              );

                    SetCursor(hOldCursor);

                    if(DevInfoElem) {
                        NewClassDriverCount = DevInfoElem->ClassDriverCount;
                    } else {
                        NewClassDriverCount = pDeviceInfoSet->ClassDriverCount;
                    }

                    if(!NewClassDriverCount) {
                        //
                        // Error.
                        //
                        if(!LoadString(MyDllModuleHandle,
                                       IDS_SELECT_DEVICE,
                                       Title,
                                       SIZECHARS(Title))) {
                            *Title = TEXT('\0');
                        }

                        FormatMessageBox(MyDllModuleHandle,
                                         NULL,
                                         MSG_NO_DEVICEINFO_ERROR,
                                         Title,
                                         MB_OK | MB_TASKMODAL
                                        );

                        bDontSave = TRUE;

                        //
                        // Clean up anything that happened to get put in here.
                        //
                        if(DevInfoElem &&
                           (DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_DIDCOMPATINFO)) {
                            //
                            // The class installer built a compatible driver list--kill it here.
                            //
                            DestroyDriverNodes(DevInfoElem->CompatDriverHead, pDeviceInfoSet);

                            DevInfoElem->CompatDriverHead = DevInfoElem->CompatDriverTail = NULL;
                            DevInfoElem->CompatDriverCount = 0;
                            DevInfoElem->InstallParamBlock.Flags   &= ~DI_DIDCOMPAT;
                            DevInfoElem->InstallParamBlock.FlagsEx &= ~DI_FLAGSEX_DIDCOMPATINFO;

                            DevInfoElem->SelectedDriver = NULL;
                            DevInfoElem->SelectedDriverType = SPDIT_NODRIVER;
                        }
                        dipb->DriverPath = -1;

                        bAskAgain = TRUE;
                        break;
                    }
                    //
                    // Allow to fall through to handling of NO_ERROR which does clean-up for us.
                    //

                case NO_ERROR :
                    //
                    // Destroy the original lists
                    //
                    if(bRestoreDeviceInfo) {
                        DestroyDriverNodes(lpOrgCompat, pDeviceInfoSet);
                        DereferenceClassDriverList(pDeviceInfoSet, lpOrgClass);

                        bRestoreDeviceInfo = FALSE;
                    }

                    bRestoreDriverPath = FALSE;
                    break;

                case ERROR_DI_BAD_PATH :
                    //
                    // Pop up an error messagebox, then go try again.
                    //
                    if(!LoadString(MyDllModuleHandle,
                                   IDS_SELECT_DEVICE,
                                   Title,
                                   SIZECHARS(Title))) {
                        *Title = TEXT('\0');
                    }

                    FormatMessageBox(MyDllModuleHandle,
                                     NULL,
                                     MSG_NO_DEVICEINFO_ERROR,
                                     Title,
                                     MB_OK | MB_TASKMODAL
                                    );

                    bDontSave = TRUE;
                    dipb->DriverPath = -1;
                    bAskAgain = TRUE;

                    //
                    // Allow to fall through to default processing to delete the current
                    // driver list(s).
                    //

                default :
DefaultHandling:
                    //
                    // Destroy the current driver list(s).
                    //
                    if(DevInfoElem) {

                        DestroyDriverNodes(DevInfoElem->CompatDriverHead, pDeviceInfoSet);
                        DevInfoElem->CompatDriverHead = DevInfoElem->CompatDriverTail = NULL;
                        DevInfoElem->CompatDriverCount = 0;

                        DereferenceClassDriverList(pDeviceInfoSet, DevInfoElem->ClassDriverHead);
                        DevInfoElem->ClassDriverHead = DevInfoElem->ClassDriverTail = NULL;
                        DevInfoElem->ClassDriverCount = 0;

                        DevInfoElem->SelectedDriver = NULL;
                        DevInfoElem->SelectedDriverType = SPDIT_NODRIVER;

                    } else {

                        DereferenceClassDriverList(pDeviceInfoSet, pDeviceInfoSet->ClassDriverHead);
                        pDeviceInfoSet->ClassDriverHead = pDeviceInfoSet->ClassDriverTail = NULL;
                        pDeviceInfoSet->ClassDriverCount = 0;

                        pDeviceInfoSet->SelectedClassDriver = NULL;
                    }
            }
        }

        if(bAskAgain) {
            //
            // Then SetupDiAskForOEMDisk failed.  Retrieve the error code.
            //
            Err = GetLastError();
        }

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = GetLastError();
        //
        // Access the following variables so that the compiler will respect
        // the statement ordering in the try clause.
        //
        bRestoreDeviceInfo = bRestoreDeviceInfo;
        bUnlockDevInfoElem = bUnlockDevInfoElem;
        bRestoreHwnd = bRestoreHwnd;
        bRestoreDriverPath = bRestoreDriverPath;
        pDeviceInfoSet = pDeviceInfoSet;
    }

    //
    // If we need to restore any state, then we must make sure that we have the HDEVINFO
    // locked.
    //
    if(bRestoreDeviceInfo || bUnlockDevInfoElem || bRestoreHwnd || bRestoreDriverPath) {

        if(!pDeviceInfoSet) {
            pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet);
            MYASSERT(pDeviceInfoSet);
        }

        try {
            //
            // If necessary, restore the original list(s).
            //
            if(bRestoreDeviceInfo) {

                if(DevInfoElem) {

                    DestroyDriverNodes(DevInfoElem->CompatDriverHead, pDeviceInfoSet);
                    DevInfoElem->CompatDriverHead = lpOrgCompat;
                    DevInfoElem->CompatDriverTail = lpOrgCompatTail;
                    DevInfoElem->CompatDriverCount = OrgCompatCount;
                    lpOrgCompat = NULL;

                    DereferenceClassDriverList(pDeviceInfoSet, DevInfoElem->ClassDriverHead);
                    DevInfoElem->ClassDriverHead = lpOrgClass;
                    DevInfoElem->ClassDriverTail = lpOrgClassTail;
                    DevInfoElem->ClassDriverCount = OrgClassCount;
                    lpOrgClass = NULL;

                    DevInfoElem->SelectedDriver = lpOrgSel;
                    DevInfoElem->SelectedDriverType = dwOrgSelType;

                } else {

                    DereferenceClassDriverList(pDeviceInfoSet, pDeviceInfoSet->ClassDriverHead);
                    pDeviceInfoSet->ClassDriverHead = lpOrgClass;
                    pDeviceInfoSet->ClassDriverTail = lpOrgClassTail;
                    pDeviceInfoSet->ClassDriverCount = OrgClassCount;
                    lpOrgClass = NULL;

                    pDeviceInfoSet->SelectedClassDriver = lpOrgSel;
                }

                dipb->Flags = dwOrgFlags;
                dipb->FlagsEx = dwOrgFlagsEx;
            }

            //
            // If we locked the DevInfoElem just for this API, then unlock it now.
            //
            if(bUnlockDevInfoElem) {
                MYASSERT(DevInfoElem);
                DevInfoElem->DiElemFlags &= ~DIE_IS_LOCKED;
            }

            //
            // If the install param block needs its parent hwnd restored, do so now.
            //
            if(bRestoreHwnd) {
                dipb->hwndParent = hwndSave;
            }

            //
            // Likewise, restore the old driver path if necessary.
            //
            if(bRestoreDriverPath) {
                dipb->DriverPath = DriverPathSave;
                dipb->Flags |= DriverPathFlagsSave;
            }

            ;   // nothing to do

        } except(EXCEPTION_EXECUTE_HANDLER) {

            if(bRestoreDeviceInfo) {
                //
                // If we hit an exception before we got a chance to restore any of our stored-away
                // driver lists, then clean those up here.
                //
                if(DevInfoElem) {
                    if(lpOrgCompat) {
                        DestroyDriverNodes(lpOrgCompat, pDeviceInfoSet);
                    }
                    if(lpOrgClass) {
                        DereferenceClassDriverList(pDeviceInfoSet, lpOrgClass);
                    }
                } else {
                    if(lpOrgClass) {
                        DereferenceClassDriverList(pDeviceInfoSet, lpOrgClass);
                    }
                }
            }
        }
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    return Err;
}

