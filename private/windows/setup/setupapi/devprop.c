/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    devprop.c

Abstract:

    Device Installer functions for property sheet support.

Author:

    Lonny McMichael (lonnym) 07-Sep-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Private routine prototypes.
//
BOOL
CALLBACK
pSetupAddPropPage(
    IN HPROPSHEETPAGE hPage,
    IN LPARAM         lParam
   );


//
// Define the context structure that gets passed to pSetupAddPropPage as lParam.
//
typedef struct _SP_PROPPAGE_ADDPROC_CONTEXT {

    BOOL              NoCancelOnFailure;   // input
    LPPROPSHEETHEADER PropertySheetHeader; // input
    DWORD             PageListSize;        // input
    DWORD             NumPages;            // output

} SP_PROPPAGE_ADDPROC_CONTEXT, *PSP_PROPPAGE_ADDPROC_CONTEXT;


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiGetClassDevPropertySheetsA(
    IN  HDEVINFO           DeviceInfoSet,
    IN  PSP_DEVINFO_DATA   DeviceInfoData,                  OPTIONAL
    IN  LPPROPSHEETHEADERA PropertySheetHeader,
    IN  DWORD              PropertySheetHeaderPageListSize,
    OUT PDWORD             RequiredSize,                    OPTIONAL
    IN  DWORD              PropertySheetType
    )
{
    PROPSHEETHEADERW UnicodePropertySheetHeader;
    DWORD Err = NO_ERROR;

    //
    // Make sure we're running interactively.
    //
    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        SetLastError(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
        return FALSE;
    }

    //
    // None of the fields that we care about in this structure contain
    // characters.  Thus, we'll simply copy over the fields we need into
    // our unicode property sheet header, and pass that into the W-API.
    //
    // The fields that we care about are the following:
    //
    //     dwFlags (in)
    //     nPages  (in/out)
    //     phpage  (in/out)
    //
    ZeroMemory(&UnicodePropertySheetHeader, sizeof(UnicodePropertySheetHeader));

    try {

        UnicodePropertySheetHeader.dwFlags = PropertySheetHeader->dwFlags;
        UnicodePropertySheetHeader.nPages  = PropertySheetHeader->nPages;
        UnicodePropertySheetHeader.phpage  = PropertySheetHeader->phpage;

        if(SetupDiGetClassDevPropertySheetsW(DeviceInfoSet,
                                             DeviceInfoData,
                                             &UnicodePropertySheetHeader,
                                             PropertySheetHeaderPageListSize,
                                             RequiredSize,
                                             PropertySheetType)) {

            PropertySheetHeader->nPages = UnicodePropertySheetHeader.nPages;
            PropertySheetHeader->phpage = UnicodePropertySheetHeader.phpage;

        } else {
            Err = GetLastError();
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupDiGetClassDevPropertySheetsW(
    IN  HDEVINFO           DeviceInfoSet,
    IN  PSP_DEVINFO_DATA   DeviceInfoData,                  OPTIONAL
    IN  LPPROPSHEETHEADERW PropertySheetHeader,
    IN  DWORD              PropertySheetHeaderPageListSize,
    OUT PDWORD             RequiredSize,                    OPTIONAL
    IN  DWORD              PropertySheetType
    )
{
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    UNREFERENCED_PARAMETER(PropertySheetHeader);
    UNREFERENCED_PARAMETER(PropertySheetHeaderPageListSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    UNREFERENCED_PARAMETER(PropertySheetType);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiGetClassDevPropertySheets(
    IN  HDEVINFO           DeviceInfoSet,
    IN  PSP_DEVINFO_DATA   DeviceInfoData,                  OPTIONAL
    IN  LPPROPSHEETHEADER  PropertySheetHeader,
    IN  DWORD              PropertySheetHeaderPageListSize,
    OUT PDWORD             RequiredSize,                    OPTIONAL
    IN  DWORD              PropertySheetType
    )
/*++

Routine Description:

    This routine adds property sheets to the supplied property sheet
    header for the device information set or element.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set for
        which property sheets are to be retrieved.

    DeviceInfoData - Optionally, supplies the address of a SP_DEVINFO_DATA
        structure for which property sheets are to be retrieved.  If this
        parameter is not specified, then property sheets are retrieved based
        on the global class driver list associated with the device information
        set itself.

    PropertySheetHeader - Supplies the property sheet header to which the
        property sheets are to be added.

        NOTE:  PropertySheetHeader->dwFlags _must not_ have the PSH_PROPSHEETPAGE
        flag set, or this API will fail with ERROR_INVALID_FLAGS.

    PropertySheetHeaderPageListSize - Specifies the size of the
        HPROPSHEETPAGE array pointed to by the PropertySheetHeader->phpage.
        Note that this is _not_ the same value as PropertySheetHeader->nPages.
        The latter specifies the number of page handles currently in the
        list.  The number of pages that may be added by this routine equals
        PropertySheetHeaderPageListSize - PropertySheetHeader->nPages.  If the
        property page provider attempts to add more pages than the property
        sheet header list can hold, this API will fail, and GetLastError will
        return ERROR_INSUFFICIENT_BUFFER.  However, any pages that have already
        been added will be in the PropertySheetHeader->phpage list, and the
        nPages field will contain the correct count.  It is the caller's
        responsibility to destroy all property page handles in this list via
        DestroyPropertySheetPage (unless the caller goes ahead and uses
        PropertySheetHeader in a call to PropertySheet).

    RequiredSize - Optionally, supplies the address of a variable that receives
        the number of property page handles added to the PropertySheetHeader.  If
        this API fails with ERROR_INSUFFICIENT_BUFFER, this variable will be set
        to the total number of property pages that the property page provider(s)
        _attempted to add_ (i.e., including those which were not successfully
        added because the PropertySheetHeader->phpage array wasn't big enough).

        Note:  This number will not equal PropertySheetHeader->nPages upon return
        if either (a) there were already property pages in the list before this
        API was called, or (b) the call failed with ERROR_INSUFFICIENT_BUFFER.

    PropertySheetType - Specifies what type of property sheets are to be
        retrieved.  May be one of the following values:

        DIGCDP_FLAG_BASIC - Retrieve basic property sheets (typically, for
                            CPL applets).

        DIGCDP_FLAG_ADVANCED - Retrieve advanced property sheets (typically,
                               for the Device Manager).

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    PDEVINSTALL_PARAM_BLOCK InstallParamBlock;
    LPGUID ClassGuid;
    HKEY hk;
    SP_PROPSHEETPAGE_REQUEST PropPageRequest;
    SP_PROPPAGE_ADDPROC_CONTEXT PropPageAddProcContext;
    SP_ADDPROPERTYPAGE_DATA PropertyPageData;
    UINT OriginalPageCount;

    //
    // Make sure we're running interactively.
    //
    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        SetLastError(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
        return FALSE;
    }

    //
    // Make sure the caller passed us a valid PropertySheetType.
    //
    if((PropertySheetType != DIGCDP_FLAG_BASIC) && (PropertySheetType != DIGCDP_FLAG_ADVANCED)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;
    DevInfoElem = NULL;
    hk = INVALID_HANDLE_VALUE;

    try {
        //
        // Make sure the property sheet header doesn't have the PSH_PROPSHEETPAGE flag set.
        //
        if(PropertySheetHeader->dwFlags & PSH_PROPSHEETPAGE) {
            Err = ERROR_INVALID_FLAGS;
            goto clean0;
        }

        //
        // Also, ensure that the parts of the property sheet header we'll be dealing with
        // look reasonable.
        //
        OriginalPageCount = PropertySheetHeader->nPages;

        if((OriginalPageCount > PropertySheetHeaderPageListSize) ||
           (PropertySheetHeaderPageListSize && !(PropertySheetHeader->phpage))) {

            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        if(DeviceInfoData) {
            //
            // Then we are to retrieve property sheets for a particular device.
            //
            if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                       DeviceInfoData,
                                                       NULL))
            {
                InstallParamBlock = &(DevInfoElem->InstallParamBlock);
                ClassGuid = &(DevInfoElem->ClassGuid);

            } else {
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }

        } else {
            //
            // We're retrieving (advanced) property pages for the set's class.
            //
            if(pDeviceInfoSet->HasClassGuid) {
                InstallParamBlock = &(pDeviceInfoSet->InstallParamBlock);
                ClassGuid = &(pDeviceInfoSet->ClassGuid);
            } else {
                Err = ERROR_NO_ASSOCIATED_CLASS;
                goto clean0;
            }
        }

        //
        // Fill in a property sheet request structure for later use.
        //
        PropPageRequest.cbSize         = sizeof(SP_PROPSHEETPAGE_REQUEST);
        PropPageRequest.DeviceInfoSet  = DeviceInfoSet;
        PropPageRequest.DeviceInfoData = DeviceInfoData;

        //
        // Fill in the context structure for use later on by our AddPropPageProc
        // callback.
        //
        PropPageAddProcContext.PropertySheetHeader = PropertySheetHeader;
        PropPageAddProcContext.PageListSize = PropertySheetHeaderPageListSize;
        PropPageAddProcContext.NumPages = 0;
        //
        // If the caller supplied the RequiredSize output parameter, then we don't
        // want to abort the callback process, even if we run out of space in the
        // hPage list.
        //
        PropPageAddProcContext.NoCancelOnFailure = RequiredSize ? TRUE : FALSE;


        //
        // Check if we should be getting Basic or Advanced Property Sheets.
        // Essentially, CPL's will want BASIC sheets, and the Device Manager
        // will want advanced sheets.
        //
        if(PropertySheetType == DIGCDP_FLAG_BASIC) {
            //
            // The BasicProperties32 entrypoint is only supplied via a device's
            // driver key.  Thus, a device information element must be specified
            // when basic property pages are requested.
            //
            // NOTE: this is different from setupx, which enumerates _all_ lpdi's
            // in the list, retrieving basic properties for each.  This doesn't
            // seem to have any practical application, and if it is really
            // required, then the caller can loop through each devinfo element
            // themselves, and retrieve basic property pages for each one.
            //
            if(!DevInfoElem) {
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }

            //
            // If the basic property page provider has not been loaded, then load
            // it and get the function address for the BasicProperties32 function.
            //
            if(!InstallParamBlock->hinstBasicPropProvider) {

                hk = SetupDiOpenDevRegKey(DeviceInfoSet,
                                          DeviceInfoData,
                                          DICS_FLAG_GLOBAL,
                                          0,
                                          DIREG_DRV,
                                          KEY_READ
                                         );

                if(hk != INVALID_HANDLE_VALUE) {

                    try {
                        Err = GetModuleEntryPoint(hk,
                                                  pszBasicProperties32,
                                                  pszBasicPropDefaultProc,
                                                  &(InstallParamBlock->hinstBasicPropProvider),
                                                  &((FARPROC)InstallParamBlock->EnumBasicPropertiesEntryPoint),
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  SetupapiVerifyNoProblem,
                                                  NULL,
                                                  DRIVERSIGN_NONE,
                                                  TRUE
                                                 );

                        if(Err == ERROR_DI_DO_DEFAULT) {
                            //
                            // The BasicProperties32 value wasn't present--this is not an error.
                            //
                            Err = NO_ERROR;

                        } else if(Err != NO_ERROR) {
                            Err = ERROR_INVALID_PROPPAGE_PROVIDER;
                        }

                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        Err = ERROR_INVALID_PROPPAGE_PROVIDER;
                        if(InstallParamBlock->hinstBasicPropProvider) {
                            FreeLibrary(InstallParamBlock->hinstBasicPropProvider);
                            InstallParamBlock->hinstBasicPropProvider = NULL;
                        }
                        InstallParamBlock->EnumBasicPropertiesEntryPoint = NULL;
                    }

                    RegCloseKey(hk);
                    hk = INVALID_HANDLE_VALUE;

                    if(Err != NO_ERROR) {
                        goto clean0;
                    }
                }
            }

            //
            // If there is a basic property page provider entry point, then call it.
            //
            if(InstallParamBlock->EnumBasicPropertiesEntryPoint) {

                PropPageRequest.PageRequested = SPPSR_ENUM_BASIC_DEVICE_PROPERTIES;

                //
                // We must first release the HDEVINFO lock, so we don't run into any weird
                // deadlock issues
                //
                UnlockDeviceInfoSet(pDeviceInfoSet);
                pDeviceInfoSet = NULL;

                InstallParamBlock->EnumBasicPropertiesEntryPoint(
                                             &PropPageRequest,
                                             pSetupAddPropPage,
                                             (LPARAM)&PropPageAddProcContext
                                            );
            }

            //
            // Now use the new DIF_ADDPROPERTYPAGE_BASIC call to see if any 
            // class-/co-installers want to add basic property pages as well.
            //
            memset(&PropertyPageData, 0, sizeof(SP_ADDPROPERTYPAGE_DATA));
            PropertyPageData.ClassInstallHeader.InstallFunction = DIF_ADDPROPERTYPAGE_BASIC;
            PropertyPageData.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            PropertyPageData.hwndWizardDlg = PropertySheetHeader->hwndParent;

            Err = DoInstallActionWithParams(DIF_ADDPROPERTYPAGE_BASIC,
                                            DeviceInfoSet,
                                            DeviceInfoData,
                                            &PropertyPageData.ClassInstallHeader,
                                            sizeof(SP_ADDPROPERTYPAGE_DATA),
                                            INSTALLACTION_CALL_CI);
                                            
            if (ERROR_DI_DO_DEFAULT == Err) {

                //
                // The class-/co-installers do not have any pages to add--this is not an error.
                //
                Err = NO_ERROR;
            }
                                            
            if ((NO_ERROR == Err) &&
                (PropertyPageData.NumDynamicPages > 0))
            {
                DWORD NumPages = 0;
                
                while (NumPages < PropertyPageData.NumDynamicPages) {

                    pSetupAddPropPage(PropertyPageData.DynamicPages[NumPages++],
                                      (LPARAM)&PropPageAddProcContext
                                      );
                }                
            }

        } else {
            //
            // We're retrieving advanced property pages.  We want to look for EnumPropPages32
            // entries in both the class key and (if we're talking about a specific device) in
            // the device's driver key.
            //
            if(!InstallParamBlock->hinstClassPropProvider) {

                hk = SetupDiOpenClassRegKey(ClassGuid, KEY_READ);

                if(hk != INVALID_HANDLE_VALUE) {

                    try {
                        Err = GetModuleEntryPoint(hk,
                                                  pszEnumPropPages32,
                                                  pszEnumPropDefaultProc,
                                                  &(InstallParamBlock->hinstClassPropProvider),
                                                  &((FARPROC)InstallParamBlock->ClassEnumPropPagesEntryPoint),
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  SetupapiVerifyNoProblem,
                                                  NULL,
                                                  DRIVERSIGN_NONE,
                                                  TRUE
                                                 );

                        if(Err == ERROR_DI_DO_DEFAULT) {
                            //
                            // The EnumPropPages32 value wasn't present--this is not an error.
                            //
                            Err = NO_ERROR;

                        } else if(Err != NO_ERROR) {
                            Err = ERROR_INVALID_PROPPAGE_PROVIDER;
                        }

                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        Err = ERROR_INVALID_PROPPAGE_PROVIDER;
                        if(InstallParamBlock->hinstClassPropProvider) {
                            FreeLibrary(InstallParamBlock->hinstClassPropProvider);
                            InstallParamBlock->hinstClassPropProvider = NULL;
                        }
                        InstallParamBlock->ClassEnumPropPagesEntryPoint = NULL;
                    }

                    RegCloseKey(hk);
                    hk = INVALID_HANDLE_VALUE;

                    if(Err != NO_ERROR) {
                        goto clean0;
                    }
                }
            }

            if(DevInfoElem && !InstallParamBlock->hinstDevicePropProvider) {

                hk = SetupDiOpenDevRegKey(DeviceInfoSet,
                                          DeviceInfoData,
                                          DICS_FLAG_GLOBAL,
                                          0,
                                          DIREG_DRV,
                                          KEY_READ
                                         );

                if(hk != INVALID_HANDLE_VALUE) {

                    try {
                        Err = GetModuleEntryPoint(hk,
                                                  pszEnumPropPages32,
                                                  pszEnumPropDefaultProc,
                                                  &(InstallParamBlock->hinstDevicePropProvider),
                                                  &((FARPROC)InstallParamBlock->DeviceEnumPropPagesEntryPoint),
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  SetupapiVerifyNoProblem,
                                                  NULL,
                                                  DRIVERSIGN_NONE,
                                                  TRUE
                                                 );

                        if(Err == ERROR_DI_DO_DEFAULT) {
                            //
                            // The EnumPropPages32 value wasn't present--this is not an error.
                            //
                            Err = NO_ERROR;

                        } else if(Err != NO_ERROR) {
                            Err = ERROR_INVALID_PROPPAGE_PROVIDER;
                        }

                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        Err = ERROR_INVALID_PROPPAGE_PROVIDER;
                        if(InstallParamBlock->hinstDevicePropProvider) {
                            FreeLibrary(InstallParamBlock->hinstDevicePropProvider);
                            InstallParamBlock->hinstDevicePropProvider = NULL;
                        }
                        InstallParamBlock->DeviceEnumPropPagesEntryPoint = NULL;
                    }

                    RegCloseKey(hk);
                    hk = INVALID_HANDLE_VALUE;

                    if(Err != NO_ERROR) {
                        goto clean0;
                    }
                }
            }

            //
            // Clear the DI_GENERALPAGE_ADDED, DI_DRIVERPAGE_ADDED, and DI_RESOURCEPAGE_ADDED flags.
            //
            InstallParamBlock->Flags &= ~(DI_GENERALPAGE_ADDED | DI_RESOURCEPAGE_ADDED | DI_DRIVERPAGE_ADDED);

            PropPageRequest.PageRequested = SPPSR_ENUM_ADV_DEVICE_PROPERTIES;

            //
            // We must first release the HDEVINFO lock, so we don't run into any weird
            // deadlock issues
            //
            UnlockDeviceInfoSet(pDeviceInfoSet);
            pDeviceInfoSet = NULL;

            //
            // If there is an advanced property page provider for this class, then call it.
            //
            if(InstallParamBlock->ClassEnumPropPagesEntryPoint) {

                InstallParamBlock->ClassEnumPropPagesEntryPoint(
                                             &PropPageRequest,
                                             pSetupAddPropPage,
                                             (LPARAM)&PropPageAddProcContext
                                            );
            }

            //
            // If there is an advanced property page provider for this particular device, then call it.
            //
            if(InstallParamBlock->DeviceEnumPropPagesEntryPoint) {

                InstallParamBlock->DeviceEnumPropPagesEntryPoint(
                                             &PropPageRequest,
                                             pSetupAddPropPage,
                                             (LPARAM)&PropPageAddProcContext
                                            );
            }

            //
            // Now use the new DIF_ADDPROPERTYPAGE_ADVANCED call to see if any 
            // class-/co-installers want to add advanced property pages as well.
            //
            memset(&PropertyPageData, 0, sizeof(SP_ADDPROPERTYPAGE_DATA));
            PropertyPageData.ClassInstallHeader.InstallFunction = DIF_ADDPROPERTYPAGE_ADVANCED;
            PropertyPageData.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            PropertyPageData.hwndWizardDlg = PropertySheetHeader->hwndParent;

            Err = DoInstallActionWithParams(DIF_ADDPROPERTYPAGE_ADVANCED,
                                            DeviceInfoSet,
                                            DeviceInfoData,
                                            &PropertyPageData.ClassInstallHeader,
                                            sizeof(SP_ADDPROPERTYPAGE_DATA),
                                            INSTALLACTION_CALL_CI);
                                            
            if (ERROR_DI_DO_DEFAULT == Err) {

                //
                // The class-/co-installers do not have any pages to add--this is not an error.
                //
                Err = NO_ERROR;
            }

            if ((NO_ERROR == Err) ||
                (PropertyPageData.NumDynamicPages > 0))
            {
                DWORD NumPages = 0;

                while (NumPages < PropertyPageData.NumDynamicPages) {

                    pSetupAddPropPage(PropertyPageData.DynamicPages[NumPages++],
                                      (LPARAM)&PropPageAddProcContext
                                      );
                }                
            } 
        }

        if(RequiredSize) {
            *RequiredSize = PropPageAddProcContext.NumPages;
        }

        if((OriginalPageCount + PropPageAddProcContext.NumPages) > PropertySheetHeaderPageListSize) {
            Err = ERROR_INSUFFICIENT_BUFFER;
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;

        if(hk != INVALID_HANDLE_VALUE) {
            RegCloseKey(hk);
        }

        //
        // Reference the following variable so the compiler will respect our statement ordering
        // w.r.t. assignment.
        //
        pDeviceInfoSet = pDeviceInfoSet;
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
CALLBACK
pSetupAddPropPage(
    IN HPROPSHEETPAGE hPage,
    IN LPARAM         lParam
   )
/*++

Routine Description:

    This is the callback routine that is passed to property page providers.
    This routine is called for each property page that the provider wishes to
    add.

Arguments:

    hPage - Supplies a handle to the property page being added.

    lParam - Supplies a pointer to a context structure used when adding the new
        property page handle.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.

--*/
{
    PSP_PROPPAGE_ADDPROC_CONTEXT Context = (PSP_PROPPAGE_ADDPROC_CONTEXT)lParam;

    //
    // Regardless of whether we successfully add this new hPage, we want to keep
    // a count of how many pages we were asked to add.
    //
    Context->NumPages++;

    if(Context->PropertySheetHeader->nPages < Context->PageListSize) {
        Context->PropertySheetHeader->phpage[Context->PropertySheetHeader->nPages++] = hPage;
        return TRUE;
    }

    return Context->NoCancelOnFailure;
}


BOOL
CALLBACK
ExtensionPropSheetPageProc(
    IN LPVOID lpv,
    IN LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    IN LPARAM lParam
    )
{
    PSP_PROPSHEETPAGE_REQUEST PropPageRequest = (PSP_PROPSHEETPAGE_REQUEST)lpv;
    HPROPSHEETPAGE hPropSheetPage = NULL;
    BOOL b = FALSE;

    //
    // Make sure we're running interactively.
    //
    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        SetLastError(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
        return FALSE;
    }

    try {

        if(PropPageRequest->cbSize != sizeof(SP_PROPSHEETPAGE_REQUEST)) {
            goto clean0;
        }

        switch(PropPageRequest->PageRequested) {

            case SPPSR_SELECT_DEVICE_RESOURCES :

                if(!(hPropSheetPage = GetResourceSelectionPage(PropPageRequest->DeviceInfoSet,
                                                               PropPageRequest->DeviceInfoData))) {
                    goto clean0;
                }
                break;

            default :
                //
                // Don't know what to do with this request.
                //
                goto clean0;
        }

        if(lpfnAddPropSheetPageProc(hPropSheetPage, lParam)) {
            //
            // Page successfully handed off to requestor.  Reset our handle so that we don't
            // try to free it.
            //
            hPropSheetPage = NULL;
            b = TRUE;
        }

clean0: ; // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Access the hPropSheetPage variable, so that the compiler will respect our statement
        // order w.r.t. assignment.
        //
        hPropSheetPage = hPropSheetPage;
    }

    if(hPropSheetPage) {
        //
        // Property page was successfully created, but never handed off to requestor.  Free
        // it now.
        //
        DestroyPropertySheetPage(hPropSheetPage);
    }

    return b;
}

