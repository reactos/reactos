//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       sthread.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"

BOOL
FixUpDriverListForInet(
    PNEWDEVWIZ NewDevWiz
    )
/*++
    If the best driver is an old Internet driver then it must also be the
    currently installed driver.  If it is not the currently installed driver
    then we will mark it with DNF_BAD_DRIVER and call DIF_SELECTBESTCOMPATDRV
    again.  We will keep doing this until the best driver is either NOT a Internet
    driver, or it is the currently installed driver.
    
    One issue here is if the class installer does not pay attention to the DNF_BAD_DRIVER
    flag and returns it as the best driver anyway.  If this happens then we will return
    FALSE which will cause us to re-build the list filtering out all Old Internet drivers.

--*/
{
    BOOL bReturn = TRUE;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;
    DWORD NumberOfDrivers = 0;
    DWORD NumberOfIterations = 0;

    ZeroMemory(&DriverInfoData, sizeof(SP_DRVINFO_DATA));
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

    while (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 SPDIT_COMPATDRIVER,
                                 NumberOfDrivers++,
                                 &DriverInfoData)) {
        ;
    }

    //
    // We need to do this over and over again until we get a non Internet Driver or 
    // the currently installed driver.
    //
    while (NumberOfIterations++ <= NumberOfDrivers) {
    
        //
        // Get the best selected driver
        //
        if (SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     &DriverInfoData
                                     ))
        {
            //
            // If it is the currently installed driver then we are fine
            //
            if (IsInstalledDriver(NewDevWiz, &DriverInfoData)) {
    
                break;
            }
    
            ZeroMemory(&DriverInstallParams, sizeof(SP_DRVINSTALL_PARAMS));
            DriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
    
            if (SetupDiGetDriverInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DriverInfoData,
                                              &DriverInstallParams))  {
    
                if (DriverInstallParams.Flags & DNF_OLD_INET_DRIVER) {
    
                    //
                    // If the best driver is already marked with DNF_BAD_DRIVER then we
                    // have a class installer that is picking a Bad driver as the best.
                    // This is not a good idea, so this API will return FALSE which will
                    // cause the entire driver list to get re-built filtering out all
                    // Old Internet drivers.
                    //
                    if (DriverInstallParams.Flags & DNF_BAD_DRIVER) {

                        bReturn = FALSE;
                        break;
                    }
                    
                    //
                    // The best driver is an OLD Internet driver, so mark it
                    // with DNF_BAD_DRIVER and call DIF_SELECTBESTCOMPATDRV
                    //
                    else {
                    
                        //
                        // BUGBUG: Is this the right thing to do?
                        //
                        DriverInstallParams.Flags = DNF_BAD_DRIVER;
        
                        if (!SetupDiSetDriverInstallParams(NewDevWiz->hDeviceInfo,
                                                           &NewDevWiz->DeviceInfoData,
                                                           &DriverInfoData,
                                                           &DriverInstallParams
                                                           )) {
                            //
                            // If the API fails then just break;
                            //
                            break;
                        }
        
                        if (!SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,
                                                       NewDevWiz->hDeviceInfo,
                                                       &NewDevWiz->DeviceInfoData
                                                       )) {
                            
                            //
                            // If the API fails then just break;
                            //
                            break;
                        }
                    }
                } else {
                    
                    //
                    // The selected driver is not an Internet driver so were good.
                    //
                    break;
                }
            } else {

                //
                // If the API fails then just break
                //
                break;
            }
        } else {

            //
            // If the API fails then just break
            //
            break;
        }
    }

    //
    // If we went through every driver and we still haven't selected a best one then the 
    // class installer is probably removing the DNF_BAD_DRIVER flag and keeps choose the 
    // same Old Internet driver over and over again.  If this happens then we will just
    // return FALSE which will cause us to rebuild the driver list without any old Internet
    // drivers and then select the best driver.
    //
    if (NumberOfIterations > NumberOfDrivers) {

        bReturn = FALSE;
    }

    return bReturn;
}

void
DoDriverSearch(
    PNEWDEVWIZ NewDevWiz,
    PSEARCHTHREAD SearchThread
    )
/*++

    This function builds up the list of drivers.  It does this based on the 
    SearchThread->Options.

--*/
{
    SP_DEVINSTALL_PARAMS DeviceInstallParams;

    //
    // Throw away previously built drivers list, in case
    // the user has copied files around, or inserted new disks.
    //
    SetupDiSetSelectedDriver(NewDevWiz->hDeviceInfo,
                             &NewDevWiz->DeviceInfoData,
                             NULL
                             );


    SetupDiDestroyDriverInfoList(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 SPDIT_COMPATDRIVER
                                 );


    //
    // Set the Device Install Params to set the parent window handle
    // and to tell setupapi to append the driver list.
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      ))
    {
        DeviceInstallParams.hwndParent = SearchThread->hDlg;

        if (SearchThread->Options & SEARCH_DEFAULT_EXCLUDE_OLD_INET) {

            DeviceInstallParams.FlagsEx |= DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS;
        }
        
        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      );
    }


    //
    //Search any single INFs (this only comes in through the UpdateDriverForPlugAndPlayDevices
    // API.
    //
    if (!SearchThread->CancelRequest && (SearchThread->Options & SEARCH_SINGLEINF)) {

        SP_DRVINFO_DATA DrvInfoData;

        SetDriverPath(NewDevWiz, NewDevWiz->SingleInfPath, FALSE);

        //
        // OR in the DI_ENUMSINGLEINF flag so that we only look at this specific INF
        //
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          ))
        {
            DeviceInstallParams.Flags |= DI_ENUMSINGLEINF;

            SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          );
        }        
        
        //
        // Build up the list in this specific INF file
        //
        SetupDiBuildDriverInfoList(NewDevWiz->hDeviceInfo,
                                   &NewDevWiz->DeviceInfoData,
                                   SPDIT_COMPATDRIVER
                                   );

        //
        // Clear the DI_ENUMSINGLEINF flag and set the DI_FLAGSEX_APPENDDRIVERLIST
        // flag in case we build from the default INF path next.
        //
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          ))
        {
            DeviceInstallParams.Flags &= ~DI_ENUMSINGLEINF;
            DeviceInstallParams.FlagsEx |= DI_FLAGSEX_APPENDDRIVERLIST;

            SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          );
        }        

        //
        // At this point we should have a list of drivers in the INF that the caller
        // of the UpdateDriverForPlugAndPlayDevices specified.  If the list is empty
        // then the INF they passed us cannot be used on the Hardware Id that they
        // passed in.  In this case we will SetLastError to ERROR_DI_BAD_PATH.
        //
        ZeroMemory(&DrvInfoData, sizeof(SP_DRVINFO_DATA));
        DrvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

        if (!SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData,
                                  SPDIT_COMPATDRIVER,
                                  0,
                                  &DrvInfoData
                                  )) {

            //
            // We wern't able to find any drivers in the specified INF that match
            // the specified hardware ID.
            //
            NewDevWiz->LastError = ERROR_DI_BAD_PATH;
        }
    }


    //
    //Search the default INF path
    //
    if (!SearchThread->CancelRequest && 
        ((SearchThread->Options & SEARCH_DEFAULT) ||
         (SearchThread->Options & SEARCH_DEFAULT_EXCLUDE_OLD_INET)))
        
    {
        SetDriverPath(NewDevWiz, NULL, FALSE);
        SetupDiBuildDriverInfoList(NewDevWiz->hDeviceInfo,
                                   &NewDevWiz->DeviceInfoData,
                                   SPDIT_COMPATDRIVER
                                   );
    }

    //
    // Set the DI_FLAGSEX_APPENDDRIVERLIST after we have searched
    // the default INF path
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      ))
    {
        DeviceInstallParams.FlagsEx |= DI_FLAGSEX_APPENDDRIVERLIST;
        
        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      );
    }


    //
    //Search any extra paths that the user specified in the wizard
    //
    if (!SearchThread->CancelRequest && (SearchThread->Options & SEARCH_PATH)) 
    {
        SetDriverPath(NewDevWiz, NewDevWiz->BrowsePath, FALSE);
        
        SetupDiBuildDriverInfoList(NewDevWiz->hDeviceInfo,
                                   &NewDevWiz->DeviceInfoData,
                                   SPDIT_COMPATDRIVER
                                   );
    }


    //
    //Search any Windows Update paths.
    //
    if (!SearchThread->CancelRequest && (SearchThread->Options & SEARCH_WINDOWSUPDATE)) 
    {
        BOOL bOldInetDriversAllowed = TRUE;

        SetDriverPath(NewDevWiz, NewDevWiz->BrowsePath, FALSE);

        //
        // We need to OR in the DI_FLAGSEX_INET_DRIVER flag so that setupapi will
        // mark in the INFs PNF that it is from the Internet.  This is important 
        // because we don't want to ever use an Internet INF again since we don't
        // have the drivers locally.
        //
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          ))
        {
            //
            // When searching using Windows Update we must allow old Internet drivers.  We need
            // to do this since it is posible to backup old Internet drivers and then reinstall 
            // them.
            //
            bOldInetDriversAllowed = (DeviceInstallParams.FlagsEx & DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS)
                ? FALSE : TRUE;
            
            DeviceInstallParams.FlagsEx |= DI_FLAGSEX_INET_DRIVER;
            DeviceInstallParams.FlagsEx &= ~DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS;


            SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          );
        }        
        
        SetupDiBuildDriverInfoList(NewDevWiz->hDeviceInfo,
                                   &NewDevWiz->DeviceInfoData,
                                   SPDIT_COMPATDRIVER
                                   );

        if (!bOldInetDriversAllowed) {

            //
            // Old Internet drivers were not allowed so we need to reset the DI_FLAGSEX_EXLCUED_OLD_INET_DRIVERS
            // FlagsEx
            //
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              ))
            {
                DeviceInstallParams.FlagsEx |= DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS;

                SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              );
            }        
        }
    }


    //
    //Search all floppy drives
    //
    if (!SearchThread->CancelRequest && (SearchThread->Options & SEARCH_FLOPPY) )
    {
        UINT DriveNumber=0;

        while (!SearchThread->CancelRequest &&
               (DriveNumber = GetNextDriveByType(DRIVE_REMOVABLE, ++DriveNumber)))
        {
            SearchDriveForDrivers(NewDevWiz, DriveNumber);
        }
    }


    //
    //Search all CD-ROM drives
    //
    if (!SearchThread->CancelRequest && (SearchThread->Options & SEARCH_CDROM))
    {
        UINT DriveNumber=0;

        while (!SearchThread->CancelRequest &&
               (DriveNumber = GetNextDriveByType(DRIVE_CDROM, ++DriveNumber)))
        {
            SearchDriveForDrivers(NewDevWiz, DriveNumber);
        }
    }


    //
    //Search the Internet using CDM.DLL
    //
    if (!SearchThread->CancelRequest && (SearchThread->Options & SEARCH_INET)) 
    {
        SetDriverPath(NewDevWiz, NULL, TRUE);

        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          ))
        {
            DeviceInstallParams.FlagsEx |= DI_FLAGSEX_DRIVERLIST_FROM_URL;

            SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          );
        }        

        SetupDiBuildDriverInfoList(NewDevWiz->hDeviceInfo,
                                   &NewDevWiz->DeviceInfoData,
                                   SPDIT_COMPATDRIVER
                                   );

        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          ))
        {
            DeviceInstallParams.FlagsEx &= ~DI_FLAGSEX_DRIVERLIST_FROM_URL;

            SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          );
        }
    }
}

void
SearchDriversInf(
    PNEWDEVWIZ NewDevWiz,
    PSEARCHTHREAD SearchThread
    )
{
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    SP_DRVINSTALL_PARAMS DriverInstallParams;
    SP_DRVINFO_DATA DriverInfoData;

    //
    // Build up the list of drivers based on the SearchThread->Options
    //
    DoDriverSearch(NewDevWiz, SearchThread);

    //
    //Pick the best driver from the list we just created
    //
    SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,
                              NewDevWiz->hDeviceInfo,
                              &NewDevWiz->DeviceInfoData
                              );
    
    if (!SearchThread->CancelRequest) 
    {
        //
        // We don't allow old Windows Update drivers to be the Best driver
        // unless it is also the Currently installed driver.  So, if the Best
        // driver is an Windows Update driver and not the currently installed driver
        // then we need to re-compute the best driver after marking the node as 
        // BAD.
        //
        // The worst case is that a bad class installer keeps choosing a bad internet
        // driver again and again.  If this is the case then FixUpDriverListForInet
        // will return FALSE and we will re-do the driver search and filter out all
        // of the Old Internet drivers.
        //
        if (!FixUpDriverListForInet(NewDevWiz)) {
    
            //
            // Re-build the entire driver list and filter out all Old Internet drivers
            //
            SearchThread->Options &= ~SEARCH_DEFAULT;
            SearchThread->Options |= SEARCH_DEFAULT_EXCLUDE_OLD_INET;
    
            DoDriverSearch(NewDevWiz, SearchThread);
    
            //
            //Pick the best driver from the list we just created
            //
            SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,
                                      NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData
                                      );
        }

        DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        if (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData,
                                  SPDIT_COMPATDRIVER,
                                  0,
                                  &DriverInfoData
                                  ))
        {
            SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;

            SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     &DriverInfoData
                                     );


            DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
            if (SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                        &NewDevWiz->DeviceInfoData,
                                        &DriverInfoData,
                                        &DriverInfoDetailData,
                                        sizeof(DriverInfoDetailData),
                                        NULL
                                        )
                ||
                GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {

                DriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
                if (SetupDiGetDriverInstallParams(NewDevWiz->hDeviceInfo,
                                                  &NewDevWiz->DeviceInfoData,
                                                  &DriverInfoData,
                                                  &DriverInstallParams) &&
                   (DriverInstallParams.Flags & DNF_INET_DRIVER)) {

                    //
                    // Driver is from the Internet
                    //
                    UpdateFileInfo(SearchThread->hDlg,
                                   TEXT(""),
                                   DRVUPD_INTERNETICON,
                                   DRVUPD_PATHPART
                                   );
                    
                } else {

                    //
                    // Driver is not from the Internet
                    //
                    UpdateFileInfo(SearchThread->hDlg,
                                   DriverInfoDetailData.InfFileName,
                                   DRVUPD_SHELLICON,
                                   DRVUPD_PATHPART
                                   );
                }                                   
            }
        }

        else 
        {
            UpdateFileInfo(SearchThread->hDlg,
                           TEXT(""),
                           DRVUPD_SHELLICON,
                           DRVUPD_PATHPART
                           );
        }
    }

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      ))
    {
        DeviceInstallParams.FlagsEx &= ~DI_FLAGSEX_APPENDDRIVERLIST;
        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      );
    }
}




DWORD
SearchDriversThread(
    PVOID pvNewDevWiz
    )
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ) pvNewDevWiz;
    PSEARCHTHREAD SearchThread;
    DWORD Error = ERROR_SUCCESS;

    UINT Msg;
    WPARAM wParam;
    LPARAM lParam;
    DWORD Delay;

    try {

        if (!NewDevWiz || !NewDevWiz->SearchThread) {
            
            return ERROR_INVALID_PARAMETER;
        }
        
        SearchThread = NewDevWiz->SearchThread;
    
        while (TRUE) {
    
            SetEvent(SearchThread->ReadyEvent);
            
            if (WaitForSingleObject(SearchThread->RequestEvent, (DWORD)-1)) {
                
                Error = GetLastError();
                break;
            }
    
            if (SearchThread->Function == SEARCH_NULL) {
                
                Msg = 0;
                Delay = 0;
            }
            
            else if (SearchThread->Function == SEARCH_EXIT) {
                
                break;
            }
            
            else if (SearchThread->Function == SEARCH_DELAY) {
                
                Delay = SearchThread->Param;
                Msg = WUM_DELAYTIMER;
                wParam = TRUE;
                lParam = ERROR_SUCCESS;
            }
            
            else if (SearchThread->Function == SEARCH_DRIVERS) {
                
                Delay = 0;
                Msg = WUM_SEARCHDRIVERS;
                wParam = TRUE;
                lParam = ERROR_SUCCESS;
    
                try {
                   
                    SearchDriversInf(NewDevWiz, SearchThread);
                }
                except (EXCEPTION_EXECUTE_HANDLER) {
                   
                    lParam = GetExceptionCode();
                }
    
            }
    
            else {
                
                Error = ERROR_INVALID_FUNCTION;
                break;
            }
    
            SearchThread->Function = SEARCH_NULL;
            WaitForSingleObject(SearchThread->CancelEvent, Delay);
            
            if (Msg && SearchThread->hDlg) {
                
                PostMessage(SearchThread->hDlg, Msg, wParam, lParam);
            }
        }

    } except(NdwUnhandledExceptionFilter(GetExceptionInformation())) {
        
        Error = RtlNtStatusToDosError(GetExceptionCode());
    }

    return Error;
}


BOOL
SearchThreadRequest(
   PSEARCHTHREAD SearchThread,
   HWND    hDlg,
   UCHAR   Function,
   ULONG   Options,
   ULONG   Param
   )
{
    MSG Msg;
    DWORD WaitReturn;


    while ((WaitReturn = MsgWaitForMultipleObjects(1,
                                                   &SearchThread->ReadyEvent,
                                                   FALSE,
                                                   INFINITE,
                                                   QS_ALLINPUT
                                                   ))
            == WAIT_OBJECT_0 + 1)
    {
        while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {
               
            if (!IsDialogMessage(SearchThread->hDlg,&Msg)) {
                   
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }



    if (WaitReturn == WAIT_OBJECT_0) {
        
        ResetEvent(SearchThread->CancelEvent);
        SearchThread->CancelRequest = FALSE;
        SearchThread->hDlg = hDlg;
        SearchThread->Function = Function;
        SearchThread->Options = Options;
        SearchThread->Param = Param;
        SetEvent(SearchThread->RequestEvent);
        return TRUE;
    }

    return FALSE;
}



VOID
CancelSearchRequest(
    PNEWDEVWIZ NewDevWiz
    )
{
    PSEARCHTHREAD SearchThread;

    SearchThread = NewDevWiz->SearchThread;

    if (SearchThread->hDlg) {

        //
        // Cancel drivers search, and then request a NULL operation
        // to get in sync with the search thread.
        //

        if (SearchThread->Function == SEARCH_DRIVERS) {
            
            SetupDiCancelDriverInfoSearch(NewDevWiz->hDeviceInfo);
        }

        SearchThread->CancelRequest = TRUE;
        SetEvent(SearchThread->CancelEvent);
        SearchThreadRequest(SearchThread,
                            NULL,
                            SEARCH_NULL,
                            SEARCH_DEFAULT,
                            0
                            );
    }
}


LONG
CreateSearchThread(
   PNEWDEVWIZ NewDevWiz
   )
{
    PSEARCHTHREAD SearchThread = NewDevWiz->SearchThread;
    DWORD  ThreadId;

    SearchThread->hDlg      = NULL;
    SearchThread->Options   = SEARCH_NULL;
    SearchThread->Function  = 0;

    if (!(SearchThread->RequestEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) ||
        !(SearchThread->ReadyEvent   = CreateEvent(NULL, FALSE, FALSE, NULL)) ||
        !(SearchThread->CancelEvent  = CreateEvent(NULL, FALSE, FALSE, NULL)) ||
        !(SearchThread->hThread      = CreateThread(NULL,
                                                    0,
                                                    SearchDriversThread,
                                                    NewDevWiz,
                                                    0,
                                                    &ThreadId
                                                    )))
    {

        if (SearchThread->RequestEvent) {
            
            CloseHandle(SearchThread->RequestEvent);
        }

        if (SearchThread->ReadyEvent) {
            
            CloseHandle(SearchThread->ReadyEvent);
        }

        if (SearchThread->CancelEvent) {
            
            CloseHandle(SearchThread->CancelEvent);
        }

        return GetLastError();
    }

    return ERROR_SUCCESS;
}


void
DestroySearchThread(
   PSEARCHTHREAD SearchThread
   )
{
    //
    // Signal search thread to exit,
    //


    if (SearchThread->hThread) {
        DWORD ExitCode;

        if (GetExitCodeThread(SearchThread->hThread, &ExitCode) &&
            ExitCode == STILL_ACTIVE)
        {
           SearchThreadRequest(SearchThread, NULL, SEARCH_EXIT, 0, 0);
        }

        WaitForSingleObject(SearchThread->hThread, (DWORD)-1);
        CloseHandle(SearchThread->hThread);
        SearchThread->hThread = NULL;
    }


    if (SearchThread->ReadyEvent) {
        
        CloseHandle(SearchThread->ReadyEvent);
        SearchThread->ReadyEvent = NULL;
    }

    if (SearchThread->RequestEvent) {
        
        CloseHandle(SearchThread->RequestEvent);
        SearchThread->RequestEvent = NULL;
    }

    if (SearchThread->CancelEvent) {
        
        CloseHandle(SearchThread->CancelEvent);
        SearchThread->CancelEvent = NULL;
    }
}
