//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       analyze.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"
#include <regstr.h>
#include <infstr.h>



INT_PTR CALLBACK
NDW_AnalyzeDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HICON hicon;
    HWND hwndParentDlg = GetParent(hDlg);
    PNEWDEVWIZ NewDevWiz;
    PSP_INSTALLWIZARD_DATA  InstallWizard;

    if (wMsg == WM_INITDIALOG) {

        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);
        NewDevWiz->AnalyzeResult = 0;
        return TRUE;
    }

    NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    InstallWizard = &NewDevWiz->InstallDynaWiz;

    switch (wMsg) {

    case WM_DESTROY:

        hicon = (HICON)LOWORD(SendDlgItemMessage(hDlg,IDC_CLASSICON,STM_GETICON,0,0));
        if (hicon) {

            DestroyIcon(hicon);
        }

        break;


    case WM_NOTIFY:

        switch (((NMHDR FAR *)lParam)->code) {

        case PSN_SETACTIVE: {

            int PrevPage;
            DWORD RegisterError = ERROR_SUCCESS;
            SP_DRVINFO_DATA DriverInfoData;

            PrevPage = NewDevWiz->PrevPage;
            NewDevWiz->PrevPage = IDD_NEWDEVWIZ_ANALYZEDEV;

            if (PrevPage == IDD_WIZARDEXT_POSTANALYZE) {
                
                break;
            }

            //
            // Get info on currently selected device, since this could change
            // as the user move back and forth between wizard pages
            // we do this on each activate.
            //
            if (!SetupDiGetSelectedDevice(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData
                                          ))
            {
                RegisterError = GetLastError();
#if DBG
                DbgPrint("Add Hardware: AnalyzeDlgProc no selected device %x\n",
                         RegisterError
                         );
#endif
            }

            //
            // Set the class Icon
            //

            if (SetupDiLoadClassIcon(&NewDevWiz->DeviceInfoData.ClassGuid, &hicon, NULL)) {
                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
                
                if (hicon) {

                    DestroyIcon(hicon);
                }
            }

            SetDriverDescription(hDlg, IDC_NDW_DESCRIPTION, NewDevWiz);
            PropSheet_SetWizButtons(hwndParentDlg, PSWIZB_BACK | PSWIZB_NEXT);

            //
            // need to determine conflict warning.
            //

            if (RegisterError != ERROR_SUCCESS) {

                SetDlgText(hDlg, IDC_NDW_TEXT, IDS_NDW_ANALYZEERR1, IDS_NDW_ANALYZEERR3);
            
            }
            
            else {
                
                SetDlgText(hDlg, IDC_NDW_TEXT, IDS_NDW_STDCFG1, IDS_NDW_STDCFG2);
            }

            if (InstallWizard->DynamicPageFlags & DYNAWIZ_FLAG_PAGESADDED) {

                if (RegisterError == ERROR_SUCCESS ||
                    !(InstallWizard->DynamicPageFlags & DYNAWIZ_FLAG_ANALYZE_HANDLECONFLICT))
                {
                   SetDlgMsgResult(hDlg, wMsg, IDD_DYNAWIZ_ANALYZE_NEXTPAGE);
                }
            }

            break;
        }

        case PSN_WIZBACK:

            if (NewDevWiz->WizExtPostAnalyze.hPropSheet) {
                
                PropSheet_RemovePage(hwndParentDlg,
                                     (WPARAM)-1,
                                     NewDevWiz->WizExtPostAnalyze.hPropSheet
                                     );
                
                NewDevWiz->WizExtPostAnalyze.hPropSheet = NULL;
            }

            SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_PREANALYZE);
            break;

        case PSN_WIZNEXT:

            //
            // Add the PostAnalyze Page and jump to it
            //

            NewDevWiz->WizExtPostAnalyze.hPropSheet = CreateWizExtPage(IDD_WIZARDEXT_POSTANALYZE,
                                                                       WizExtPostAnalyzeDlgProc,
                                                                       NewDevWiz
                                                                       );

            if (NewDevWiz->WizExtPostAnalyze.hPropSheet) {

                PropSheet_AddPage(hwndParentDlg, NewDevWiz->WizExtPostAnalyze.hPropSheet);
            }

            SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_POSTANALYZE);

           break;

        }
        break;


    default:
        return(FALSE);
    
    }
    
    return(TRUE);
}

INT_PTR CALLBACK
WizExtPreAnalyzeDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ )GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;


    switch (wMsg) {
       
    case WM_INITDIALOG: {
           
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        NewDevWiz = (PNEWDEVWIZ )lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);
        break;
    }

    case WM_DESTROY:
        break;


   case WM_NOTIFY:
       
       switch (((NMHDR FAR *)lParam)->code) {
           
       case PSN_SETACTIVE:

           PrevPageId = NewDevWiz->PrevPage;
           NewDevWiz->PrevPage = IDD_WIZARDEXT_PREANALYZE;

           if (PrevPageId == IDD_WIZARDEXT_SELECT) {

               //
               // Moving forward on first page
               //


               //
               // if we are not doing the old fashioned DYNAWIZ
               // Add ClassWizard Extension pages for preanalyze
               //

               if (NewDevWiz->ManualInstall &&
                   !(NewDevWiz->InstallDynaWiz.DynamicPageFlags & DYNAWIZ_FLAG_PAGESADDED))
               {
                   AddClassWizExtPages(hwndParentDlg,
                                       NewDevWiz,
                                       &NewDevWiz->WizExtPreAnalyze.DeviceWizardData,
                                       DIF_NEWDEVICEWIZARD_PREANALYZE
                                       );
               }


               //
               // Add the end page, which is PreAnalyze end
               //

               NewDevWiz->WizExtPreAnalyze.hPropSheetEnd = CreateWizExtPage(IDD_WIZARDEXT_PREANALYZE_END,
                                                                            WizExtPreAnalyzeEndDlgProc,
                                                                            NewDevWiz
                                                                            );

               if (NewDevWiz->WizExtPreAnalyze.hPropSheetEnd) {
                   
                   PropSheet_AddPage(hwndParentDlg, NewDevWiz->WizExtPreAnalyze.hPropSheetEnd);
               }

               PropSheet_PressButton(hwndParentDlg, PSBTN_NEXT);

           }
           else {

                //
                // Moving backwards from PreAnalyze end on PreAanalyze
                //

                //
                // Clean up proppages added.
                //

                if (NewDevWiz->WizExtPreAnalyze.hPropSheetEnd) {
                    
                    PropSheet_RemovePage(hwndParentDlg,
                                         (WPARAM)-1,
                                         NewDevWiz->WizExtPreAnalyze.hPropSheetEnd
                                         );
                    NewDevWiz->WizExtPreAnalyze.hPropSheetEnd = NULL;
                }


                RemoveClassWizExtPages(hwndParentDlg,
                                       &NewDevWiz->WizExtPreAnalyze.DeviceWizardData
                                       );




                //
                // Jump back
                // Note: The target pages don't set PrevPage, so set it for them
                //
                NewDevWiz->PrevPage = IDD_WIZARDEXT_SELECT;
                
                if (NewDevWiz->InstallDynaWiz.DynamicPageFlags & DYNAWIZ_FLAG_PAGESADDED) {
                    
                    SetDlgMsgResult(hDlg, wMsg, IDD_DYNAWIZ_ANALYZE_PREVPAGE);
                }
                
                else {
                    
                    SetDlgMsgResult(hDlg, wMsg, IDD_DYNAWIZ_SELECTDEV_PAGE);
                }
            }

           break;

       case PSN_WIZNEXT:
           SetDlgMsgResult(hDlg, wMsg, 0);
           break;
           
       }
       break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR CALLBACK
WizExtPreAnalyzeEndDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ )GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;


    switch (wMsg) {
       
    case WM_INITDIALOG: {
           
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        NewDevWiz = (PNEWDEVWIZ )lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);
        break;
    }

    case WM_DESTROY:
        break;

    case WM_NOTIFY:
       
        switch (((NMHDR FAR *)lParam)->code) {
           
        case PSN_SETACTIVE:

            PrevPageId = NewDevWiz->PrevPage;
            NewDevWiz->PrevPage = IDD_WIZARDEXT_PREANALYZE_END;

            if (PrevPageId == IDD_NEWDEVWIZ_ANALYZEDEV) {
                
                //
                // Moving backwards from analyzepage
                //

                //
                // Jump back
                //


                PropSheet_PressButton(hwndParentDlg, PSBTN_BACK);

            }
            
            else {
                
                //
                // Moving forward on end page
                //

                SetDlgMsgResult(hDlg, wMsg, IDD_NEWDEVWIZ_ANALYZEDEV);
            }


           break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            SetDlgMsgResult(hDlg, wMsg, 0);
            break;
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR CALLBACK
WizExtPostAnalyzeDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ )GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;


    switch (wMsg) {
       
    case WM_INITDIALOG: {
           
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
           NewDevWiz = (PNEWDEVWIZ )lppsp->lParam;
           SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);
           break;
    }

    case WM_DESTROY:
        break;

    case WM_NOTIFY:
       
        switch (((NMHDR FAR *)lParam)->code) {
           
        case PSN_SETACTIVE:

            PrevPageId = NewDevWiz->PrevPage;
            NewDevWiz->PrevPage = IDD_WIZARDEXT_POSTANALYZE;

            if (PrevPageId == IDD_NEWDEVWIZ_ANALYZEDEV) {
                //
                // Moving forward on first page
                //

                //
                // if we are not doing the old fashioned DYNAWIZ
                // Add ClassWizard Extension pages for postanalyze
                //

                if (NewDevWiz->ManualInstall &&
                    !(NewDevWiz->InstallDynaWiz.DynamicPageFlags & DYNAWIZ_FLAG_PAGESADDED))
                {
                    AddClassWizExtPages(hwndParentDlg,
                                        NewDevWiz,
                                        &NewDevWiz->WizExtPostAnalyze.DeviceWizardData,
                                        DIF_NEWDEVICEWIZARD_POSTANALYZE
                                        );
                }


                //
                // Add the end page, which is PostAnalyze end
                //

                NewDevWiz->WizExtPostAnalyze.hPropSheetEnd = CreateWizExtPage(IDD_WIZARDEXT_POSTANALYZE_END,
                                                                             WizExtPostAnalyzeEndDlgProc,
                                                                              NewDevWiz
                                                                              );

                if (NewDevWiz->WizExtPostAnalyze.hPropSheetEnd) {
                    
                    PropSheet_AddPage(hwndParentDlg, NewDevWiz->WizExtPostAnalyze.hPropSheetEnd);
                }

                PropSheet_PressButton(hwndParentDlg, PSBTN_NEXT);

            }
            
            else  {
                
                //
                // Moving backwards from PostAnalyze end on PostAnalyze
                //

                //
                // Clean up proppages added.
                //

                if (NewDevWiz->WizExtPostAnalyze.hPropSheetEnd) {
                    
                    PropSheet_RemovePage(hwndParentDlg,
                                         (WPARAM)-1,
                                         NewDevWiz->WizExtPostAnalyze.hPropSheetEnd
                                         );
                    
                    NewDevWiz->WizExtPostAnalyze.hPropSheetEnd = NULL;
                }


                RemoveClassWizExtPages(hwndParentDlg,
                                       &NewDevWiz->WizExtPostAnalyze.DeviceWizardData
                                       );

            }

            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            SetDlgMsgResult(hDlg, wMsg, 0);
            break;
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR CALLBACK
WizExtPostAnalyzeEndDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ )GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;

    switch (wMsg) {
       
    case WM_INITDIALOG: {
           
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
           NewDevWiz = (PNEWDEVWIZ )lppsp->lParam;
           SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);
           break;
    }

    case WM_DESTROY:
        break;

    case WM_NOTIFY:
       
        switch (((NMHDR FAR *)lParam)->code) {
           
        case PSN_SETACTIVE:
            PrevPageId = NewDevWiz->PrevPage;
            NewDevWiz->PrevPage = IDD_WIZARDEXT_POSTANALYZE_END;

            if (PrevPageId == IDD_NEWDEVWIZ_INSTALLDEV) {

                //
                // Moving backwards from finishpage
                //

                //
                // Jump back
                //

                PropSheet_PressButton(hwndParentDlg, PSBTN_BACK);
            }
            
            else  {
                 
                //
                // Moving forward on End page
                //

                SetDlgMsgResult(hDlg, wMsg, IDD_NEWDEVWIZ_INSTALLDEV);
            }

            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            SetDlgMsgResult(hDlg, wMsg, 0);
            break;
        }
        break;
 
    default:
        return(FALSE);
    }

    return(TRUE);
}
