//***************************************************************************
//
//    QuickRes for Windows NT and Windows 9x
//
//    Tray app to change your display resolution quickly.
//
//    written by ToddLa
//
//    03/03/96 - ChrisW : Get to build on NT
//    03/28/96 - MDesai : Finish porting; add submenus with frequencies;
//                        Test for valid devmode.
//    04/23/96 - MDesai : option for 'showing tested modes only'
//    10/01/96 - MDesai : fix all win95-specific bugs
//    11/03/98 - MDesai : 'multimonitor aware'
//
//***************************************************************************


#include "QuickRes.h"


PTCHAR szAppName;

HINSTANCE    hInstApp;
HICON        AppIcon;


//
// options, properties, about and exit...
// monitor menu
//

HMENU        MainMenu;
HMENU        MonitorMenu=NULL;

//
// number of monitors/display devices installed
// pointer to monitorinfo struct for each monitor
//

INT              iMonitors;
LPQRMONITORINFO  pMonitors;


//
// Waiting for a Popup - don't process any tray messages
//

BOOL Waiting=FALSE;


//
//  Flags: update registry, show restart modes, sort order
//         also where the freq menu(s) go.
//
WORD QuickResFlags;
WORD FreqMenuLocation;



//
//***************************************************************************
//
//  GetResourceString( UINT )
//
//  Load a resource string into a LPTSTR - the memory for the string
//  is dynamically allocated.  The callee must free the memory!
//
//***************************************************************************
//

LPTSTR GetResourceString ( UINT ResourceID )
{


    INT    BuffSize=RESOURCE_STRINGLEN;     // current max size of string
    PTCHAR BigBuf;                          // buffer to find size of resource
    PTCHAR ResBuf;                          // buffer for resource
    INT    len;                             // length of the resource


    while (1)
    {

        //
        //  Allocate hopefully oversized buffer
        //

        if( !(BigBuf= LocalAlloc( LPTR, BuffSize ) ) )
        {
            return NULL;
        }


        //
        //  Try to read string into BigBuf to get its length
        //

        if ( !(len = LoadString(hInstApp, ResourceID, BigBuf, BuffSize)) )
        {
            return NULL;
        }


        //
        //  Buffer is too small - try again.
        //

        if( len >= BuffSize-1 )
        {
            BuffSize <<= 1;
            LocalFree ( BigBuf );
        }

        else
        {

            //
            //  Reallocate properly sized string buffer,
            //  and copy string into it
            //

            len = ( len + 1 ) * sizeof( TCHAR );

            if (ResBuf = LocalAlloc( LPTR, len ))
            {
                lstrcpyn ( ResBuf, BigBuf, len );
            }

            LocalFree ( BigBuf );

            return( ResBuf );

        }

    }

}


//
//***************************************************************************
//
// GetModeName( PDEVMODE, PTCHAR*, PTCHAR* )
//
// Translate devmode into user friendly strings-
// one for resolution and color depth; one for refresh rate
//
//***************************************************************************
//

void GetModeName(PDEVMODE pDevMode, PTCHAR *szMode, PTCHAR *szFreq )
{

    PTCHAR FmtRes=NULL;             // Format strings for
    PTCHAR FmtHz=NULL;              // resolution and Hz


    //
    // Load format string corresponding to devmode
    //

    FmtRes = GetResourceString ( IDS_CRES + BPP(pDevMode) );


    //
    // Use Default Freq string if necessary
    //
    if (fShowFreqs)
    {
        if( HZ(pDevMode) == 0 || HZ(pDevMode) == 1)
        {
            FmtHz = GetResourceString ( IDS_DEFHERTZ );
        }
        else
        {
            FmtHz = GetResourceString ( IDS_HERTZ );
        }
    }

    //
    //  return separate resolution and frequency strings
    //  need to convert "%d"-> "12345", add byte for '\0'
    //

    if (FmtRes)
    {
        if (*szMode = LocalAlloc( LPTR, sizeof(TCHAR)*
                                (lstrlen(FmtRes)+2*INT_FORMAT_TO_5_DIGITS+1 ) ))
        {
            wsprintf(*szMode, FmtRes, XRES(pDevMode), YRES(pDevMode) );
        }

        LocalFree ( FmtRes );
    }


    if (fShowFreqs && FmtHz)
    {

        if (*szFreq = LocalAlloc ( LPTR, sizeof(TCHAR)*
                                   (lstrlen(FmtHz)+INT_FORMAT_TO_5_DIGITS+1) ))
        {
            wsprintf(*szFreq, FmtHz, HZ(pDevMode));
        }

        LocalFree ( FmtHz );
    }

}



//
//***************************************************************************
//
//   GetCurrentDevMode( INT, PDEVMODE )
//
//   Get a pointer to the current devmode into *pDM
//
//***************************************************************************
//

PDEVMODE GetCurrentDevMode(INT iDisplay, PDEVMODE pDM)
{

    UINT uRet=0;

    pDM->dmSize= sizeof(DEVMODE);

    //
    // NT specific; returns current devmode
    //

    if (fShowFreqs)
    {
        uRet = EnumDisplaySettings( pMonitors[iDisplay].DeviceName, (DWORD)ENUM_CURRENT_SETTINGS, pDM );
    }

    if (!uRet)
    {
        //
        //  ENUM_CURRENT_SETTINGS doesnt work on win95
        //  Get current settings via GetDeviceCaps
        //

        HDC  hDC;
        UINT HorzRes;
        UINT VertRes;
        UINT BPP;
        UINT VRefresh;
        UINT Index;

        hDC      =  GetDC( NULL );
        HorzRes  =  GetDeviceCaps( hDC, HORZRES );
        VertRes  =  GetDeviceCaps( hDC, VERTRES );
        BPP      =  GetDeviceCaps( hDC, BITSPIXEL ) * GetDeviceCaps( hDC, PLANES );
        VRefresh =  GetDeviceCaps( hDC, VREFRESH );

        //
        //  Enumerate all settings until one matches our current settings
        //

        for ( Index=0;
              EnumDisplaySettings( pMonitors[iDisplay].DeviceName, Index, pDM);
              Index++ )
        {
            if ( HorzRes ==  XRES(pDM) &&
                 VertRes ==  YRES(pDM) &&
                 BPP     ==  BPP(pDM)
               )
            {
                //
                // if frequency matters, then check for it
                //

                if (!fShowFreqs || (VRefresh == HZ(pDM)) )
                    break;
            }
        }

        ReleaseDC (NULL, hDC);
    }

    return pDM;
}



//
//***************************************************************************
//
//  SetMode( HWND, UINT )
//
//  Set the new devmode and update registry on request using
//  the CDS_UPDATEREGISTRY flag.  If user wants to change and
//  restart, then we need to update the registry and restart.
//
//***************************************************************************
//

BOOL SetMode( HWND hwnd, INT iDisplay, UINT index )
{
    DWORD    CDSret=0;                  // ret value, ChangeDisplaySettings
    DWORD    CDSFlags=0;                // 2nd param of call to CDS
    INT_PTR  DialogBoxRet=0;            // IDYES/NO/ABORT/CANCEL
    LPDEVMODEINFO pSave;                // save ptr in iDisplay to remember orig mode
    LPDEVMODEINFO pdm;                  // new mode to be set
    BOOL     bChange=FALSE;             // changing modes or not
    LPQRMONITORINFO pCurrMon;           // ptr to current monitorinfo strcut


    pCurrMon = &pMonitors[iDisplay];

    //
    //  Save current mode; find ptr to new mode
    //

    pSave = pCurrMon->pCurrentdm;
    pdm   = &(pCurrMon->pModes[index]);

    //
    //  If user wants to update registry
    //

    if( fUpdateReg )
    {
        CDSFlags |= CDS_UPDATEREGISTRY;
    }


    //
    //  Tell CDS what fields may be changing
    //  Also, keep appwndproc from doing anything while we are testing
    //

    pdm->dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
    Waiting=TRUE;


    //
    //  Call CDS and update registry on request.  (If it is
    //  a known bad mode give user chance to change his mind.)
    //

    if( (VALIDMODE(pdm) != MODE_INVALID ) ||
           ( MsgBox( IDS_INVALIDMODE, 0, MB_YESNO | MB_ICONQUESTION )==IDYES ) )
    {

        CDSret = ChangeDisplaySettingsEx( pCurrMon->DeviceName, &(pdm->dm), NULL, CDSFlags, 0);

        if (CDSret == DISP_CHANGE_SUCCESSFUL)
        {
            //
            //  Even though it may be temporary, current dm has changed.
            //  Need to reset pCurrentdm to point to new current DM.
            //  Change tooltip to reflect old settings
            //

            pCurrMon->pCurrentdm = pdm;

            TrayMessage(hwnd, NIM_MODIFY, TRAY_ID, AppIcon);

            //
            //  Return value claims that it 'worked.' But, it may not visible
            //  to the user (e.g. the mode is unsupported by the monitor).
            //  If the User has not already approved this new resolution,
            //  then make the user approve the change, or we default back to
            //  the last devmode.
            //

            if ( fGoodMode(pdm) )
            {
                //
                //  VALID or BESTHZ modes - go ahead and change
                //

                bChange = TRUE;
            }

            else
            {
                //
                //  Ask user if it looks okay
                //  Flag the mode based on return value.
                //

                switch( DialogBoxRet = DialogBoxParam( hInstApp,
                                          MAKEINTRESOURCE(KeepNewRes),
                                          NULL,
                                          KeepNewResDlgProc,
                                          iDisplay) )
                {

                                    //
                                    //  There should NOT be a break after
                                    //  IDYES.  Fall thru by design.
                                    //
                    case IDYES:     bChange = TRUE;

                    case IDABORT:   VALIDMODE(pdm) = MODE_VALID;
                                    break;

                    case IDNO:
                    case IDCANCEL:  VALIDMODE(pdm) = MODE_INVALID;
                                    break;

                }   // switch

            }   //  else - MODE_INVALID

        }

        if (CDSret != DISP_CHANGE_SUCCESSFUL)
        {
            //
            // Requires restart.  Ask user if thats okay.
            //

            if (CDSret == DISP_CHANGE_RESTART)
            {

                if ( MsgBox(IDS_RESTART, 0, MB_YESNO | MB_ICONQUESTION) == IDYES )
                {

                    //
                    //  After restart all modes will need to be tested again?
                    //

                    SetDevmodeFlags ( iDisplay, TRUE );


                    //
                    //  Call CDS again to update registry
                    //

                    ChangeDisplaySettingsEx( pCurrMon->DeviceName, &(pdm->dm), NULL,
                                             (CDSFlags | CDS_UPDATEREGISTRY), 0);

                    ExitWindowsEx(EWX_REBOOT, 0);
                }

            }
            else
            {

                 //
                 // Tell user we cannot change to this devmode
                 //

                 MsgBox(IDS_CANTSETMODE, 0, MB_OK | MB_ICONEXCLAMATION);
            }

        }   // end else != DISP_CHANGE_SUCCESSFUL


        if (bChange)
        {
            //
            //  Changing to a valid mode; destroy and rebuild menu
            //  Mark mode we were just in as valid (if it wasnt already)
            //

            VALIDMODE(pSave) |= MODE_VALID;

            //
            //  This is the new "Best Hz" mode.  The old mode is 'only valid'.
            //

            if ((FreqMenuLocation == IDD_ONEMENUMOBILE) ||
                (FreqMenuLocation == IDD_ONEMENUBOTTOM) )
            {
                VALIDMODE(pCurrMon->pCurrentdm) = MODE_BESTHZ;
            }

            DestroyModeMenu( iDisplay, TRUE, FALSE );
        }

        else    // !bChange
        {

            //
            //  Change back to last good devmode; do not have to recheck menuitems
            //

            pCurrMon->pCurrentdm = pSave;


            //
            //  Change back, and reset registry IF we had set it above
            //  Change tooltip to reflect old settings
            //

            if (CDSret != DISP_CHANGE_RESTART)
            {
                ChangeDisplaySettingsEx( pCurrMon->DeviceName, &(pCurrMon->pCurrentdm->dm),
                                         NULL, CDSFlags, 0);
            }

            TrayMessage(hwnd, NIM_MODIFY, TRAY_ID, AppIcon);

        }  // bChange


    }  // endif


    //
    //  Save new settings for this devmode to the registry.
    //  Even if quickres does not exit gracefully, preferences
    //  will be saved.
    //

    if (fRememberModes)
    {
        SaveAllSettings();
    }

    //
    //  Show modemenu again; allow appwndproc to process messages
    //

    if (!bChange)
    {
        SetTimer(hwnd, TRAY_ID, 10, NULL);
    }

    Waiting=FALSE;


    //
    // if Current hasnt changed then we return false
    //

    return (bChange);

}


//
//********************************************************************
//
//  CompareDevmodes ( LPDEVMODEINFO, LPDEVMODEINFO )
//
//  Compares 2 devmodes -
//  Returns 0 if equal, -1 if first > second, +1 if first < second
//
//  msb to lsb: xres, yres, bpp, hertz
//********************************************************************
//

int _cdecl CompareDevmodes( LPDEVMODEINFO pDmi1, LPDEVMODEINFO pDmi2 )
{
    INT compare;
    LPDEVMODE pDm1 = &(pDmi1->dm);
    LPDEVMODE pDm2 = &(pDmi2->dm);


    //
    //  Compare Xs, then Ys, BPP, and Hz.  If !fShowFreqs
    //  then compare only Xs, Ys, and BPP.
    //

    if ( !fSortByBPP || ((compare= BPP(pDm1) - BPP(pDm2)) == 0))
    {
        if( (compare= ( XRES(pDm1) - XRES(pDm2) ) ) == 0 )
        {
            if( (compare= ( YRES(pDm1) - YRES(pDm2) ) ) == 0 )
            {
                if ( fSortByBPP || ((compare= BPP(pDm1) - BPP(pDm2)) == 0))
                {
                   compare = fShowFreqs ? (HZ(pDm1) - HZ(pDm2))  :  0;
                }
            }
        }
    }

    //
    //  Set return value as -1, 0, or 1 only
    //

    if( compare < 0)
    {
        compare= -1;
    }

    else
    {
        if( compare > 0 )
        {
            compare= 1;
        }
    }

    return( compare );

}


//
//********************************************************************
//
//  CheckMenuItemCurrentMode ( INT )
//
//  Traverse all menu items and check the Hz value corresponding
//  to the current mode.  Also, highlight the current resolution/
//  BPP as defaultmenuitem
//
//********************************************************************
//

void CheckMenuItemCurrentMode( INT iDisplay )
{

    int i;                          //  counter
    DEVMODEINFO dmi;                //  temporary storage for current DM
    LPQRMONITORINFO lpqrmi;         //  temporary ptr
    HMENU hMenu;                    //  Frequency submenu for a given Res/BPP
    UINT  MenuItem;                 //  Menu item for exact devmode
    DWORD dwSta;                    //  returns status variable



    lpqrmi = &pMonitors[iDisplay];

    //
    // Need a pointer to the current devmode.  This function will search
    // pModes trying to match the devmode pointed to by pCurrentdm.
    // After the 1st time through, pCurrentdm will be a ptr IN pModes
    //

    if (!lpqrmi->pCurrentdm)
    {
        //
        // Get current devmode
        //

        GetCurrentDevMode(iDisplay, &(dmi.dm));
        lpqrmi->pCurrentdm = &dmi;
    }


    //
    // Uncheck all menu items
    //

    for( i=0; i<lpqrmi->iModes; i++ )
    {

        hMenu = lpqrmi->FreqMenu[FREQMENU( &lpqrmi->pModes[i] )];

        MenuItem= MENUITEM( &lpqrmi->pModes[i] );

        //
        //  Uncheck the Hz in the FreqMenu (if applicable); uncheck item on mode menu
        //

        if (hMenu)
        {
            dwSta= CheckMenuItem(hMenu, MenuItem, MF_BYCOMMAND|MF_UNCHECKED);
            CheckMenuItem(lpqrmi->ModeMenu, FREQMENU( &lpqrmi->pModes[i] ), MF_BYPOSITION  | MF_UNCHECKED );
        }

        CheckMenuItem(lpqrmi->ModeMenu, MenuItem, MF_BYCOMMAND  | MF_UNCHECKED );
    }



    //
    // Check the current one
    //

    for( i=0; i<lpqrmi->iModes; i++ )
    {

        //
        // Go through the array looking for a match of the current devmode
        //

        if( ( CompareDevmodes( lpqrmi->pCurrentdm, &lpqrmi->pModes[i] ) ) == 0 )
        {

            //
            //  Found it!
            //  Get the menu item ID for this devmode and which
            //  frequency submenu it is a part of.
            //

            hMenu = lpqrmi->FreqMenu[FREQMENU( &lpqrmi->pModes[i] )];
            MenuItem= MENUITEM( &lpqrmi->pModes[i] );


            //
            // Save this ptr in the pCurrentdm variable
            // check menu item on mode menu and check mode
            // on frequency submenu (if applicable)
            //

            lpqrmi->pCurrentdm = &lpqrmi->pModes[i];

            if (hMenu)
            {
                dwSta= CheckMenuItem(hMenu, MenuItem, MF_BYCOMMAND|MF_CHECKED);
                CheckMenuItem(lpqrmi->ModeMenu, FREQMENU(&lpqrmi->pModes[i]), MF_BYPOSITION | MF_CHECKED );
            }
            else
            {
                CheckMenuItem(lpqrmi->ModeMenu, MenuItem, MF_BYCOMMAND  | MF_CHECKED );
            }

            break;
        }
    }
}


//
//********************************************************************
//
//   DestroyModeMenu( INT iDisplay, BOOL bRebuild, BOOL bNeedtoSort )
//
//   Free all frequency submenus and the mode menu
//
//********************************************************************
//

void DestroyModeMenu( INT iDisplay, BOOL bRebuild, BOOL bNeedtoSort)
{

    int i;
    LPQRMONITORINFO lpqrmi;         //  temporary ptr


    lpqrmi = &pMonitors[iDisplay];

    //
    //  Free all frequency submenus
    //

    for ( i = 0; i < lpqrmi->iModes; i++ )
    {

        if (IsMenu(lpqrmi->FreqMenu[i]))
        {
            DestroyMenu( lpqrmi->FreqMenu[i] );
            lpqrmi->FreqMenu[i] = NULL;
        }

    }


    //
    //  Free the mode menu (resolutions/BPP)
    //

    if (lpqrmi->ModeMenu)
    {
        DestroyMenu(lpqrmi->ModeMenu);
        lpqrmi->ModeMenu = NULL;

        if (iMonitors==1)
        {
            DestroyMenu(MonitorMenu);
            MonitorMenu = NULL;
        }
    }


    if (bRebuild)
    {
        lpqrmi->ModeMenu = GetModeMenu( iDisplay, bNeedtoSort );

        if (iMonitors==1)
        {
            MonitorMenu = lpqrmi->ModeMenu;
            AppendMainMenu();
        }
        else
        {
            //  If ModifyMenu replaces a menu item that opens a drop-down menu or submenu, the
            //  function destroys the old drop-down menu/submenu & frees the memory used by it.

            ModifyMenu(MonitorMenu, iDisplay, MF_BYPOSITION | MF_POPUP,
                       (UINT_PTR)lpqrmi->ModeMenu, pMonitors[iDisplay].MonitorName);
        }
    }
}


//
//********************************************************************
//
//   HandleFreqMenu( )
//
//   Either append submenu to res/bpp, save it for later, or
//   ditch it and put all Hz entries on mode menu.
//   If there is only one Hz for a given Res, we dont need it.
//
//********************************************************************
//
VOID HandleFreqMenu( INT iDisplay, int FreqCount, int ResCounter, int pFirst)
{

    PTCHAR Res=NULL;
    PTCHAR Hz=NULL;
    LPQRMONITORINFO lpqrmi;         //  temporary ptr


    lpqrmi = &pMonitors[iDisplay];
    GetModeName(&lpqrmi->pModes[pFirst].dm, &Res, &Hz);

    //
    //  Dont use submenus if there is only 1 Hz
    //  This is always true when freqmenulocation==IDD_ALLMODEMENU
    //  OR not showing frequency menus
    //  Concatenate Res & Hz into one string (IF fShowFreqs)
    //

    if ( FreqCount == 1 )
    {
        if (fShowFreqs)
        {
            PTCHAR ResHz;

            if (ResHz=LocalAlloc( LPTR, sizeof(TCHAR)*
                              (lstrlen(Res)+lstrlen(Hz)+1) ))
            {
                wsprintf(ResHz,TEXT("%s%s"),Res,Hz);
                AppendMenu(lpqrmi->ModeMenu, MF_STRING,
                           (iDisplay+1)*MENU_RES+pFirst, ResHz);
            }

            LocalFree(ResHz);
        }
        else
        {
            AppendMenu(lpqrmi->ModeMenu, MF_STRING,
                       (iDisplay+1)*MENU_RES+pFirst, Res);
        }
    }

    else
    {
        int i=0;
        int nAppended=0;

        //
        //  Create Popup and append all Hz strings
        //  Append FreqCount items, possibly skipping over some modes
        //

        lpqrmi->FreqMenu[ResCounter] = CreatePopupMenu();

        for (i=0; nAppended < FreqCount; i++)
        {

            PTCHAR LoopRes=NULL;
            PTCHAR LoopHz=NULL;

            //
            //  Skip untested modes if requested.  FreqCount does NOT
            //  include skipped modes, so we count up with nAppended, not i.
            //

            if ( !fShowTestedModes || fGoodMode(&lpqrmi->pModes[pFirst+i]) )
            {

                GetModeName(&lpqrmi->pModes[pFirst+i].dm,&LoopRes,&LoopHz);
                AppendMenu(lpqrmi->FreqMenu[ResCounter],MF_STRING,
                           (iDisplay+1)*MENU_RES+pFirst+i,LoopHz);
                nAppended++;

                LocalFree(LoopRes);
                LocalFree(LoopHz);
                LoopRes=NULL;
                LoopHz=NULL;
            }
        }


        //
        //  Hang menu off side of each bpp/res
        //

        if (FreqMenuLocation == IDD_SUBMENUS)
        {
            AppendMenu(lpqrmi->ModeMenu,MF_POPUP,
                       (UINT_PTR)lpqrmi->FreqMenu[ResCounter],Res);
        }

        else
        {
            //
            //  Only show submenu for the current mode
            //  Use BESTHZ mode or the VALID mode with the
            //  lowest frequency.
            //

            if ( (FreqMenuLocation == IDD_ONEMENUMOBILE) ||
                 (FreqMenuLocation == IDD_ONEMENUBOTTOM) )
            {

                int BestHz=0;
                int index;

                //
                //  Start with highest freq (pFirst+i-1)
                //  and work down to pFirst looking for BestHz.
                //  if we find BESTHZ use that one, else
                //  use last VALIDMODE we get before loop ends
                //

                for (index=pFirst+i-1 ; index >= pFirst; index--)
                {
                    if ( VALIDMODE(&lpqrmi->pModes[index]) == MODE_BESTHZ )
                    {
                        BestHz = index;
                        break;
                    }
                    else
                    {
                        if (VALIDMODE(&lpqrmi->pModes[index])!=MODE_INVALID)
                        {
                            BestHz = index;
                        }
                    }
                }

                //
                //  No valid/besthz modes.  Use smallest Hz for that Res
                //

                if (!BestHz)
                {
                    BestHz = pFirst;
                }

                AppendMenu(lpqrmi->ModeMenu,MF_STRING,
                           (iDisplay+1)*MENU_RES+BestHz,Res);
            }
        }
    }

    LocalFree(Res);
    LocalFree(Hz);

}


//
//********************************************************************
//
//   GetModeMenu( INT, BOOL )
//
//   Build the mode menu with each resolution/BPP having a
//   pointer to its own frequency submenu
//
//********************************************************************
//

HMENU GetModeMenu ( INT iDisplay, BOOL bNeedtoSort )
{

    int  n;                        // counter
    BOOL bMajorChange=FALSE;       // change in the major sort order field
    BOOL bMinorChange=FALSE;       // change in the minor sort order field
    int  FreqCount=0;             // number of freqs on the current submenu
    int  ResCounter=0;            // Res/Color defines the freqmenu #
    INT   FirstMode=-1;            // index in pmodes; 1st mode for given res/bpp
    LPQRMONITORINFO lpqrmi;        //  temporary ptr


    lpqrmi = &pMonitors[iDisplay];


    if (!lpqrmi->ModeMenu)
    {
        lpqrmi->ModeMenu = CreatePopupMenu();


        if (bNeedtoSort)
        {
            qsort( (void*)  lpqrmi->pModes,
                   (size_t) lpqrmi->iModes,
                   (size_t) sizeof(DEVMODEINFO),
                   ( int (_cdecl*)(const void*,const void*) ) CompareDevmodes );

            lpqrmi->pCurrentdm = NULL;
        }


        //
        // For each devmode, add res/color to menu.
        // Make a submenu of frequencies for each res/color
        //

        for (n=0; n < lpqrmi->iModes; n++)
        {
            LPDEVMODEINFO  pDM = &lpqrmi->pModes[n];

            //
            // Tested successfully or might require restart
            //

            if ( ( (CDSTEST(pDM) == DISP_CHANGE_SUCCESSFUL) ||
                   (fShowModesThatNeedRestart && (CDSTEST(pDM) == DISP_CHANGE_RESTART)) ) &&

                 ( !fShowTestedModes || fGoodMode(pDM) )     )
            {


                //
                //  Check for change in the major/minor sort item
                //  *only after we 'initialize' firstmode below
                //

                if (FirstMode == -1)
                {

                //
                //  First time thru, initialize FirstMode,counter
                //

                    FirstMode = n;
                    FreqCount=0;

                }

                else
                {
                    if( BPP(&lpqrmi->pModes[FirstMode].dm) != BPP(&pDM->dm) )
                    {
                        bMajorChange = fSortByBPP;
                        bMinorChange = !fSortByBPP;
                    }

                    if( ( XRES(&lpqrmi->pModes[FirstMode].dm) != XRES(&pDM->dm) ) ||
                        ( YRES(&lpqrmi->pModes[FirstMode].dm) != YRES(&pDM->dm) ) )
                    {
                        bMajorChange |= !fSortByBPP;
                        bMinorChange |= fSortByBPP;
                    }


                    //
                    //  The BPP and/or the Resolution changed.
                    //

                    if ( bMajorChange || bMinorChange )
                    {

                        //
                        //  Appends a Res/BPP and a submenu if applicable
                        //

                        HandleFreqMenu(iDisplay,FreqCount,ResCounter,FirstMode);
                        ResCounter++;

                        //
                        //  Need a separator when major sort item changes
                        //

                        if ( bMajorChange )
                        {
                            AppendMenu(lpqrmi->ModeMenu,MF_SEPARATOR,0,NULL);
                            ResCounter++;
                        }


                        //
                        // n is first mode for the new res/bpp
                        // reset counter, flags
                        //

                        FirstMode  = n;
                        FreqCount= 0;
                        bMajorChange = FALSE;
                        bMinorChange = FALSE;
                    }
                }


                //
                //  Fill in fields for this mode; inc freqcount
                //


                MENUITEM( pDM ) = (iDisplay+1)*MENU_RES+n;
                FREQMENU( pDM ) = ResCounter;
                FreqCount++;


                //
                //  ALLMODEMENU - Force menu append every time
                //

                if (FreqMenuLocation == IDD_ALLMODEMENU)
                {
                   bMinorChange = TRUE;
                }


            }

        }  // end for


        //
        //  NO VALID MODES!!!  Certainly the current mode should be valid.  Make
        //  this mode VALID. Setup FreqCount, FirstMode for the last HandleFreqMenu
        //

        if (FirstMode == -1)
        {
            DEVMODEINFO DisplayModeInfo;

            DisplayModeInfo.dm.dmSize= sizeof(DEVMODE);
            GetCurrentDevMode(iDisplay, &DisplayModeInfo.dm);

            for (n=0; CompareDevmodes(&DisplayModeInfo,&lpqrmi->pModes[n]) != 0; n++ )
            {
            }

            VALIDMODE(&lpqrmi->pModes[n]) = MODE_BESTHZ;
            FirstMode = n;
            FreqCount = 1;

        }


        //
        //  Handle the FreqMenu for the last Res/BPP.
        //

        HandleFreqMenu(iDisplay,FreqCount,ResCounter,FirstMode);


        //
        //  Update menu checks; mode status
        //

        CheckMenuItemCurrentMode( iDisplay );


        //
        //  Put Hz menu next to current mode, or at the bottom
        //

        if (FreqMenuLocation == IDD_ONEMENUMOBILE)
        {
            MENUITEMINFO mii;

            ZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_SUBMENU;
            mii.hSubMenu = lpqrmi->FreqMenu[FREQMENU(lpqrmi->pCurrentdm)];
            SetMenuItemInfo(lpqrmi->ModeMenu, FREQMENU(lpqrmi->pCurrentdm), MF_BYPOSITION, &mii);
        }

        else
        {

            if (FreqMenuLocation == IDD_ONEMENUBOTTOM)
            {
                PTCHAR szRefRate;
                UINT flags=MF_POPUP;

                szRefRate = GetResourceString(IDS_REFRESHRATE);

                if ( !lpqrmi->FreqMenu[FREQMENU(lpqrmi->pCurrentdm)] )
                {
                    flags = MF_GRAYED;
                }

                AppendMenu(lpqrmi->ModeMenu,MF_SEPARATOR,0,NULL);
                AppendMenu(lpqrmi->ModeMenu, flags,
                           (UINT_PTR)lpqrmi->FreqMenu[FREQMENU(lpqrmi->pCurrentdm)],
                           szRefRate);

                LocalFree(szRefRate);
            }
        }
    }

    return (lpqrmi->ModeMenu);
}


//
//********************************************************************
//
//   AppendMainMenu( VOID )
//
//   Append main menu (from .rc file) to monitor menu
//
//********************************************************************
//

VOID AppendMainMenu()
{

#ifdef MAINWITHMODE

    int  n;                        // counter

    //
    //  Add main menu to bottom of mode menu.  These menu
    //  items come from MainMenu as defined in .rc file
    //

    AppendMenu(MonitorMenu,MF_SEPARATOR,0,NULL);

    for (n=0; n < GetMenuItemCount(MainMenu); n++)
    {

        MENUITEMINFO mii;

        //
        //  Set up mii struct to retrieve the length of
        //  each menu item string via GetMenuItemInfo().
        //

        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_ID;

        mii.cch = GetMenuString(MainMenu, n, NULL, 0, MF_BYPOSITION) +1;

        //
        //  Allocate enough memory and read in the string.
        //

        if (mii.dwTypeData = LocalAlloc( LPTR, mii.cch*sizeof(TCHAR) ))
        {

            //
            //  Read in the string, get it's ID and append to the menu
            //

            if (GetMenuString(MainMenu, n, mii.dwTypeData, mii.cch,MF_BYPOSITION))
            {
                mii.wID=GetMenuItemID(MainMenu, n);

                AppendMenu(MonitorMenu, MF_STRING, mii.wID, mii.dwTypeData);
            }

            LocalFree(mii.dwTypeData);
        }
    }

    SetMenuDefaultItem(MonitorMenu,MENU_PROPERTIES,MF_BYCOMMAND);

#endif

}


//
//********************************************************************
//
//   GetMonitorMenu( BOOL )
//
//   Build all mode menus with each resolution/BPP having a
//   pointer to its own frequency submenu
//
//********************************************************************
//

HMENU GetMonitorMenu ( BOOL bNeedtoSort )
{

    if (!MonitorMenu)
    {
        //
        //  Use Modemenu of iDisplay==0 as the monitor menu
        //

        if (iMonitors == 1)
        {
            MonitorMenu = GetModeMenu(0, bNeedtoSort);
        }
        else
        {
            INT    iDisplay;

            MonitorMenu = CreatePopupMenu();

            for (iDisplay=0; iDisplay < iMonitors; iDisplay++)
            {
                //
                //  append each monitor name to the main monitor menu
                //

                AppendMenu( MonitorMenu, MF_POPUP,
                            (UINT_PTR)GetModeMenu(iDisplay, bNeedtoSort),
                            pMonitors[iDisplay].MonitorName );
            }

        }

        AppendMainMenu();

    }

    return MonitorMenu;
}


//
//********************************************************************
//
//   BuildDevmodeList( )
//
//   Enumerate all devmodes into an array; sort them, and filter
//   out duplicate modes, 4bpp modes (if there is an 8bpp mode at
//   the same resolution), and modes with X Resolution < 640 pixels -
//   we dont want to expose ModeX modes.
//
//********************************************************************
//

BOOL BuildDevmodeList( )
{

    DEVMODE DisplayMode;          // temporary devmode storage
    int     nModes,n,iDisplay;    // counters
    BOOL    bShrink=FALSE;        // set if iModes ever decreases
    LPQRMONITORINFO  lpqrmi;
    LPDEVMODEINFO    lpdm;

    //
    // Find the number of monitors/displaydevices
    // alloc a monitorinfo struct per monitor
    //

    iMonitors = max(GetSystemMetrics(SM_CMONITORS),1);
    pMonitors = GlobalAlloc(GPTR, iMonitors*sizeof(QRMONITORINFO));


    if (iMonitors > 1)
    {
        HINSTANCE hInst;
        FARPROC lpfn;
        DISPLAY_DEVICE DispDev;
        PTCHAR szMonitorRes;

        szMonitorRes = GetResourceString(IDS_MONITOR);

        DispDev.cb = sizeof(DispDev);

        if (hInst=LoadLibrary(TEXT("user32.dll")))
        {
            if (lpfn=GetProcAddress(hInst, ENUMDISPLAYDEVICES ) )
            {
                for (iDisplay=0; iDisplay < iMonitors; iDisplay++)
                {

                    DWORD dwSize;

                    //
                    //  For each display, get the monitor name & Device name
                    //  Alloc enough memory for MonitorName for iDisplay < 1000.
                    //

                    (lpfn)(NULL, iDisplay, &DispDev, 0);

                    pMonitors[iDisplay].DeviceName  = GlobalAlloc(GPTR, sizeof(TCHAR)*(lstrlen(DispDev.DeviceName)+1));
                    lstrcpy(pMonitors[iDisplay].DeviceName, DispDev.DeviceName);

                    dwSize = lstrlen(DispDev.DeviceString) + lstrlen(szMonitorRes);
                    if (DispDev.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
                        dwSize += lstrlen(TEXT("Primary"));
             
                    pMonitors[iDisplay].MonitorName = GlobalAlloc( GPTR, sizeof(TCHAR)*dwSize );


//  special case primary monitor later
//
//                    if (DispDev.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
//                    {
//                        wsprintf( pMonitors[iDisplay].MonitorName, szMonitorRes,
//                                  TEXT("Primary "), TEXT(""), DispDev.DeviceString );
//                        pMonitors[iDisplay].bPrimary = TRUE;
//                    }
//                    else
                    {
                        TCHAR index[8];

                        _itot(iDisplay+1,index,8);
                        lstrcat(index,TEXT(" "));
                        wsprintf( pMonitors[iDisplay].MonitorName, szMonitorRes,
                                  TEXT(""), index, DispDev.DeviceString);
                        pMonitors[iDisplay].bPrimary = FALSE;
                    }
                }
            }

            FreeLibrary(hInst);
        }

        LocalFree(szMonitorRes);
    }
    else
    {
        pMonitors[0].DeviceName  = NULL;
        pMonitors[0].MonitorName = NULL;
    }


    DisplayMode.dmSize= sizeof(DEVMODE);


    for (iDisplay=0; iDisplay < iMonitors; iDisplay++)
    {

        lpqrmi = &pMonitors[iDisplay];

        lpqrmi->ModeMenu = NULL;
        lpqrmi->FreqMenu = NULL;
        lpqrmi->iModes = 0;
        lpqrmi->pModes = NULL;
        lpqrmi->pCurrentdm = NULL;


        //
        // Find the number of modes known by driver for each monitor
        //

        for( nModes=0; EnumDisplaySettings(pMonitors[iDisplay].DeviceName, nModes, &DisplayMode); nModes++)
        {
        }

        //
        // Get space for all modes
        //

        lpqrmi->pModes = (LPDEVMODEINFO) GlobalAlloc( GPTR, nModes*sizeof(DEVMODEINFO) );
        lpdm = lpqrmi->pModes;

        if( !lpdm )
        {
            DestroyModeMenu( iDisplay, FALSE, FALSE );
            return FALSE;
        }


        //
        //  Get all display modes into the pModes array
        //

        for( n=0; n<nModes; n++ )
        {
            lpdm[n].dm.dmSize= sizeof(DEVMODE);

            //
            // Get next mode into next spot in pModes
            //

            EnumDisplaySettings( pMonitors[iDisplay].DeviceName, n, &lpdm[n].dm );

            //
            //  If any Hz is NOT 0 or 1 (default), then turn on Freq flag.
            //  This will be true on NT.  Win95 will always return 0 or 1.
            //

            if ( HZ(&lpdm[n].dm)  &&  (HZ(&lpdm[n].dm) != 1) )
                QuickResFlags |= QF_SHOWFREQS;
        }


        //
        // sort them according to QF_SORTBYBPP :
        //  (1) BPP X Y HZ  or   (2) X Y BPP HZ
        //

        qsort( (void*)  lpdm,
               (size_t) nModes,
               (size_t) sizeof(DEVMODEINFO),
               ( int (_cdecl*)(const void*,const void*) ) CompareDevmodes );


        //
        //   Filter out any duplicate devmodes return by the driver
        //   and any modes with x resolution < 640 pixels.  We dont
        //   want to show ModeX modes (320x200, 320x240, etc.)
        //

        if (nModes > 1 )
        {

            for (n=0; n+1 < nModes; )
            {

                if (XRES(&lpdm[n].dm) < 640)
                {
                    nModes--;
                    bShrink = TRUE;
                    MoveMemory( &lpdm[n],
                                &lpdm[n+1],
                                (nModes-n)*sizeof(DEVMODEINFO) );
                }
                else
                {
                    //
                    //  If consecutive devmodes are identical, then copy the next
                    //  one over the dup and decrement iModes (# of devmodes).
                    //

                    while ( CompareDevmodes(&lpdm[n],&lpdm[n+1]) == 0 )
                    {
                        //
                        //  Don't go past the last devmode
                        //

                        if (n+2 < nModes--)
                        {
                            bShrink = TRUE;
                            MoveMemory( &lpdm[n],
                                        &lpdm[n+1],
                                        (nModes-n)*sizeof(DEVMODEINFO) );
                        }
                        else
                        {
                            break;
                        }
                    }

                    n++;
                }
            }
        }


        //
        // Check CDS return value for all modes and eliminate all 4bpp
        // modes that have a corresponding 8bpp mode at the same res
        //

        for (n=0; n < nModes; n++)
        {

            CDSTEST(&lpdm[n]) = (WORD)ChangeDisplaySettingsEx( lpqrmi->DeviceName, &lpdm[n].dm,
                                                                    NULL, CDS_TEST, 0);

            //
            //  Filter out all 4BPP modes that have an 8BPP mode at the same resolution
            //

            if (BPP(&lpdm[n].dm)==8)
            {
                INT i;

                for (i=0; i < n; )
                {

                    if ( (BPP (&lpdm[i].dm) == 4)    &&
                         (XRES(&lpdm[n].dm) == XRES(&lpdm[i].dm)) &&
                         (YRES(&lpdm[n].dm) == YRES(&lpdm[i].dm))    )
                    {
                        nModes--;
                        bShrink = TRUE;
                        MoveMemory( &lpdm[i],
                                    &lpdm[i+1],
                                    (nModes-i)*sizeof(DEVMODEINFO) );
                        n--;
                    }
                    else
                    {
                        i++;
                    }
                }
            }
        }

        //
        //  nModes might have decreased; might as well free up some memory.
        //  Note that iModes could NOT have increased, so the ReAlloc will be okay.
        //

        if (bShrink)
        {
            lpqrmi->pModes = (LPDEVMODEINFO) GlobalReAlloc(lpqrmi->pModes,
                                                           nModes*sizeof(DEVMODEINFO),
                                                           GMEM_MOVEABLE );
        }

        lpqrmi->iModes = nModes;

        //
        //  At most, we need 1 freqmenu per mode (actually it's always < #modes.)
        //  Note : separators take up 1 (unused) hmenu in the array, so it would
        //  that we need hmenus > #modes, when "all modes on main menu".  But,
        //  in that case, we dont use the freq submenus at all. :)
        //

        lpqrmi->FreqMenu = (HMENU*) GlobalAlloc(GPTR,nModes*sizeof(HMENU));
        for (n=0; n < nModes; n++)
        {
            lpqrmi->FreqMenu[n] = NULL;
        }


        //
        //  Get modeflags from registry or zero out modeflags[]
        //

        GetDevmodeFlags(iDisplay);


        //
        //  Call GetModeMenu to put all strings/popups in place
        //  Current mode will be the best until user changes it.
        //

        GetModeMenu( iDisplay, FALSE );

        VALIDMODE(lpqrmi->pCurrentdm) = MODE_BESTHZ;

    }

    return TRUE;
}


//
//********************************************************************
//
//   DoProperties( )
//
//   Calls the control panel applet to show 'Display Properties'
//   specifically the display settings
//
//********************************************************************
//

void DoProperties( )
{
    STARTUPINFO          si;
    PROCESS_INFORMATION  pi;
    TCHAR lpszProperties[64] = DISPLAYPROPERTIES;

    GetStartupInfo( &si );


    //
    // Start it up.
    //

    CreateProcess(NULL, lpszProperties, NULL, NULL, FALSE,
                  0,    NULL,           NULL, &si,  &pi);


    //
    //  Dont care what wait return value is, but we want
    //  to 'disable' tray icon for a minute or until the
    //  user kills desk.cpl
    //

    WaitForSingleObject( pi.hProcess, 60*1000 );


    CloseHandle ( pi.hThread );
    CloseHandle ( pi.hProcess );
}


//
//********************************************************************
//
//   AppWndProc(HWND, UINT, WPARAM, LPARAM)
//
//   Main window proc to process messages
//
//********************************************************************
//

LRESULT CALLBACK AppWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    POINT pt;                  // Get cursor pos for the menu placement
    INT   i;

    switch (msg)
    {

        case WM_CREATE:

            //
            //   Add icon to tray next to time
            //

            TrayMessage(hwnd, NIM_ADD, TRAY_ID, AppIcon);

            break;


        case WM_DESTROY:

            //
            //   Remove icon from tray.
            //

            TrayMessage(hwnd, NIM_DELETE, TRAY_ID, NULL );

            PostQuitMessage(0);

            break;


        case WM_SETTINGCHANGE:
        case WM_DISPLAYCHANGE:

            //
            //  New settings.  Reset pCurrentdm as index in pModes
            //  No need to destroy/rebuild the mode menu.
            //

            if (!Waiting)
            {
                for (i=0; i<iMonitors; i++)
                {
                    pMonitors[i].pCurrentdm = NULL;
                    CheckMenuItemCurrentMode(i);
                }
            }

            break;


        case WM_DEVICECHANGE:

            if (wParam == DBT_CONFIGCHANGED ||
                wParam == DBT_MONITORCHANGE)
            {

                //
                //  Will need a new menu; clear all devmode flags
                //  destroy all mode menus, and the monitor menu
                //

                for (i=0; i<iMonitors; i++)
                {
                    DestroyModeMenu( i, FALSE, FALSE );
                    SetDevmodeFlags( i, TRUE );
                }

                if (MonitorMenu)
                {
                    DestroyMenu(MonitorMenu);
                    MonitorMenu = NULL;
                }

                //
                //  Regenerate monitor and all mode menus
                //

                MonitorMenu = GetMonitorMenu( TRUE );
            }
            break;


        case WM_COMMAND:
        {

            switch (LOWORD(wParam))
            {

                case MENU_CLOSE:

                    PostMessage(hwnd, WM_CLOSE, 0, 0);

                    break;

                case MENU_PROPERTIES:

                    //
                    // Start control panel applet
                    //

                    DoProperties();

                    break;


                case MENU_ABOUT:

                    //
                    // Show a generic about box
                    //

                    MsgBox(IDS_ABOUT, 0, MB_OK );

                    break;


                case MENU_OPTIONS:

                    //
                    // After showing options dlg box, show mode menu again
                    //

                    if (fShowFreqs)
                        DialogBox(hInstApp, MAKEINTRESOURCE(NTOptions), NULL, NTOptionsDlgProc);
                    else
                        DialogBox(hInstApp, MAKEINTRESOURCE(W95Options),NULL, W95OptionsDlgProc);

                    SetTimer(hwnd, TRAY_ID, 10, NULL);

                    break;


                default:
                {

                    //
                    // Change devmode to pModes[OffsetPdev]
                    //

                    INT OffsetPdev;
                    INT iDisplay;

                    //
                    // The menu item is an offset from MENU_RES
                    // of the selected item.
                    //

                    iDisplay   = LOWORD(wParam) / MENU_RES - 1;
                    OffsetPdev = LOWORD(wParam) % MENU_RES;

                    //
                    // Check that the offset is within range
                    //

                    if( OffsetPdev >= 0 && OffsetPdev < pMonitors[iDisplay].iModes )
                    {

                        //
                        // if different from current devmode then change it
                        //

                        if ( CompareDevmodes( &pMonitors[iDisplay].pModes[OffsetPdev],
                                               pMonitors[iDisplay].pCurrentdm) )
                        {
                            SetMode(hwnd, iDisplay, OffsetPdev);
                        }

                    }

                }

                break;

            }

            break;

        }


        case WM_TIMER:

            //
            // Left click was not a double-click
            //

            KillTimer(hwnd, TRAY_ID);
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);

            //
            // Create and/or Get resolutions menu
            //

            TrackPopupMenu(GetMonitorMenu( FALSE ), TPM_LEFTBUTTON,
                           pt.x, pt.y, 0, hwnd, NULL);

            break;


        case TRAY_MSG:
        {

            //
            // No messages processed while waiting on
            // a dlg/msg box to return
            //

            if (!Waiting)
            {

                switch (lParam)
                {
                    case WM_RBUTTONUP:

                        //
                        // Properties, about, Exit
                        //

                        SetForegroundWindow(hwnd);
                        GetCursorPos(&pt);

                        TrackPopupMenu(MainMenu, TPM_RIGHTBUTTON,
                                       pt.x, pt.y, 0, hwnd, NULL);

                        break;


                    case WM_LBUTTONDOWN:

                        //
                        // Resolutions menu
                        //

                        SetTimer(hwnd, TRAY_ID, GetDoubleClickTime()+10, NULL);

                        break;



                    case WM_LBUTTONDBLCLK:

                        //
                        // start control panel applet
                        //

                        KillTimer(hwnd, TRAY_ID);
                        DoProperties();

                        break;
                }

            }

        }

        break;

    }

    return DefWindowProc(hwnd,msg,wParam,lParam);

}


//
//********************************************************************
//
//  MsgBox(int, UINT, UINT)
//
//  Generic messagebox function that can print a value into
//  a format string
//
//********************************************************************
//

int MsgBox(int id, UINT value, UINT flags)
{

    PTCHAR msgboxtext=NULL;           // message box body text
    INT  ret = 0;
    MSGBOXPARAMS mb;


    //
    //  Ignore tray clicks while msgbox is up, and
    //  Show at least an OK button.
    //

    Waiting = TRUE;
    if (flags == 0)
    {
        flags = MB_OK | MB_USERICON;
    }


    //
    //  Can print a value into a format string, if value!=0.
    //

    if (value)
    {
        PTCHAR msgboxfmt;                // body test format

        if (msgboxfmt = GetResourceString ( id ))
        {
            if (msgboxtext = LocalAlloc ( LPTR, sizeof(TCHAR)*
                             (lstrlen(msgboxfmt)+INT_FORMAT_TO_5_DIGITS+1)))
            {
                wsprintf(msgboxtext,msgboxfmt,value);
            }

            LocalFree( msgboxfmt );
        }
    }

    else
    {
       msgboxtext = GetResourceString ( id );
    }


    if (msgboxtext)
    {

        mb.cbSize               = sizeof(mb);
        mb.hwndOwner            = NULL;
        mb.hInstance            = hInstApp;
        mb.lpszText             = msgboxtext;
        mb.lpszCaption          = szAppName;
        mb.dwStyle              = flags;
        mb.lpszIcon             = szAppName;
        mb.dwContextHelpId      = 0;
        mb.lpfnMsgBoxCallback   = NULL;
        mb.dwLanguageId         = MAKELANGID (LANG_NEUTRAL, SUBLANG_NEUTRAL);;


        //
        //  Special API for the about box. otherwise, use Messageboxindirect
        //

        if (id == IDS_ABOUT)
        {
            ret = ShellAbout(mb.hwndOwner, mb.lpszCaption, mb.lpszText, AppIcon);
        }

        else
        {

            if (flags & MB_USERICON)
            {
                //
                //  only use MessageBoxIndirect if we have to.
                //  has problems on win9x.
                //

                ret = MessageBoxIndirect(&mb);
            }

            else
            {
                //
                //  MessageBoxEx works great on both NT and Win95
                //

                ret = MessageBoxEx ( mb.hwndOwner, mb.lpszText, mb.lpszCaption,
                                     mb.dwStyle,   (WORD)mb.dwLanguageId );
            }
        }

        //
        //  Free string memory; start processing tray msgs again
        //

        LocalFree( msgboxtext );
    }

    Waiting = FALSE;

    return ret;

}


//
//********************************************************************
//
//  WinMain
//
//********************************************************************
//

int NEAR PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
    WNDCLASS cls;
    MSG      msg;
    HWND     hwnd;
    INT      iDisplay;

    hInstApp = hInst;
    szAppName = GetResourceString( IDS_TITLE );


    //
    //   App is already running.  Do not start a 2nd instance
    //

    if ( FindWindow( szAppName, szAppName ) )
    {
        return 0;
    }


    AppIcon = LoadIcon(hInst,szAppName);


    //
    //  Register a class for the main application window
    //

    cls.lpszClassName  = szAppName;
    cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    cls.hInstance      = hInstApp;
    cls.hIcon          = AppIcon;
    cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
    cls.lpszMenuName   = szAppName;
    cls.style          = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    cls.lpfnWndProc    = (WNDPROC)AppWndProc;
    cls.cbWndExtra     = 0;
    cls.cbClsExtra     = 0;

    if (!RegisterClass(&cls))
        return FALSE;

    hwnd = CreateWindow(szAppName,
                        szAppName,
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                        NULL, NULL,
                        hInstApp, NULL);


    //
    //  Properties, about, exit - properties is the default
    //

    MainMenu = GetSubMenu(GetMenu(hwnd), 0);
    SetMenuDefaultItem(MainMenu,MENU_PROPERTIES,MF_BYCOMMAND);


    //
    //  Get flags from registry and build the modemenu
    //  from scratch.
    //

    GetQuickResFlags( );

    if (!BuildDevmodeList())
    {
        return FALSE;
    }


    //
    // Update tray tooltip to be current resolution
    //

    TrayMessage( hwnd, NIM_MODIFY, TRAY_ID, AppIcon );


    //
    // Polling messages from event queue
    //

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


    //
    //  write flags to registry
    //

    SaveAllSettings();


    //
    //  Free up dynamically allocated globals.
    //

    LocalFree ( szAppName );

    for (iDisplay=0; iDisplay<iMonitors; iDisplay++)
    {
        if (pMonitors[iDisplay].DeviceName)
            GlobalFree(pMonitors[iDisplay].DeviceName);

        if (pMonitors[iDisplay].MonitorName)
            GlobalFree(pMonitors[iDisplay].MonitorName);

        GlobalFree(pMonitors[iDisplay].pModes);

        GlobalFree(pMonitors[iDisplay].FreqMenu);
    }

    GlobalFree(pMonitors);


    return (int)msg.wParam;
}


//
//********************************************************************
//
//   TrayMessage (HWND, DWORD, UINT, HICON )
//
//   Add/remove icon to/from tray next to the time
//
//********************************************************************
//

BOOL TrayMessage(HWND hwnd, DWORD msg, UINT id, HICON hIcon )
{

    NOTIFYICONDATA tnd;
    PTCHAR Res=NULL;
    PTCHAR Hz=NULL;
    UINT   uDisplay=0;
    UINT   uNewlen=0;

    tnd.cbSize           = sizeof(NOTIFYICONDATA);
    tnd.hWnd             = hwnd;
    tnd.uID              = id;
    tnd.szTip[0]         = '\0';
    tnd.uFlags           = NIF_MESSAGE|NIF_ICON|NIF_TIP;
    tnd.uCallbackMessage = TRAY_MSG;
    tnd.hIcon            = hIcon;


    //
    //  Changing tooltip text to match current resolution
    //  (Make sure pCurrentdm is valid / not NULL.)
    //

    if (msg == NIM_MODIFY)
    {

        do
        {

            if (pMonitors[uDisplay].pCurrentdm)
            {
                GetModeName(&(pMonitors[uDisplay].pCurrentdm->dm), &Res, &Hz);

                //
                //  calculate how long the string will be (need to be sure it's < 64)
                //  old tip + new Res + Hz (if applicable) + ", " (if its not the 1st mon)
                //

                uNewlen = lstrlen(tnd.szTip) + lstrlen(Res);

                if (uDisplay > 0)
                   uNewlen += 2;

                if (fShowFreqs)
                   uNewlen += lstrlen(Hz);


                if (uNewlen < 64)
                {
                    //
                    //  this displays information will fit in the tooltip
                    //  add ", " if not 1st mon, then the Res, then Hz (if applicable)
                    //

                    if ( uDisplay > 0 )
                        lstrcat(tnd.szTip,TEXT(", "));

                    lstrcat(tnd.szTip,Res);

                    if (fShowFreqs)
                        lstrcat(tnd.szTip,Hz);
                }

                LocalFree(Res);
                LocalFree(Hz);

                ++uDisplay;
            }

      }  while (uDisplay < (UINT)iMonitors);

    }

    //
    //  Adding the tray icon - Current devmode
    //  is not known so use AppName as tip
    //

    else
    {
        wsprintf(tnd.szTip, szAppName);
    }

    return Shell_NotifyIcon( msg, &tnd );
}



//
//*****************************************************************************
//
//  KeepNewResDlgProc(HWND, UINT, WPARAM, LPARAM )
//
//  User must enter Yes to keep new res, or we default back to the old res.
//
//*****************************************************************************
//

INT_PTR FAR PASCAL KeepNewResDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    //
    //  Save strings as static pointers, and free them only in YES/NO/ABORT/CANCEL,
    //  because they will otherwise disappear from a Win95 dialog box immediately
    //  after they are free'd.
    //

    static int     NOTimeOut;                  // countdown to 0
    static PTCHAR  NewResString=NULL;          // user friendly name for devmode
    static PTCHAR  NewHzString=NULL;           // and frequency
    static PTCHAR  szAt=NULL;                  // ", at"
    static PTCHAR  TotalString=NULL;           // "<wid x ht>, at <freq>"


    switch (message)
    {

       case WM_INITDIALOG:     // initialize values and focus

       {

            //
            //  Initialize values and focus
            //

            DEVMODE dm;


            //
            //  Ignore tray messages while waiting for yes/no.
            //  Wait KEEP_RES_TIMEOUT seconds.
            //

            Waiting=TRUE;


            //
            // Get current devmode; lparam is the iDisplay
            //
            GetCurrentDevMode( (INT)lParam, &dm );


            //
            // Get user friendly strings (concatenate Res & Hz, if applicable)
            //

            GetModeName( &dm, &NewResString, &NewHzString);

            if (NewResString)
            {
                if (fShowFreqs && NewHzString)
                {
                    szAt = GetResourceString ( IDS_AT );
                    
                    //
                    // Replace 2nd text item of msgbox
                    //

                    if (TotalString = LocalAlloc ( LPTR, sizeof(TCHAR)*
                                                 ( lstrlen(NewResString)+
                                                   lstrlen(NewHzString)+
                                                   lstrlen(szAt)+
                                                   1 ) ))
                    {
                        lstrcpy(TotalString, NewResString);
                        lstrcat(TotalString, szAt);
                        lstrcat(TotalString, NewHzString);

                        SetDlgItemText(hDlg, IDTEXT2, TotalString);
                    }
                }
                else
                {
                    SetDlgItemText(hDlg, IDTEXT2, NewResString);
                }
            }

            //
            //  Set timeout length and start waiting
            //

            NOTimeOut=KEEP_RES_TIMEOUT;

            SetTimer(hDlg,IDD_COUNTDOWN,1000,NULL);

            return (TRUE);

            break;

        }


        case WM_TIMER:

            {
                PTCHAR NoTextFmt=NULL;           // "NO: %d"
                PTCHAR NoText=NULL;              // e.g. "NO: 15"

                //
                // Still counting down
                //

                if ( NOTimeOut >= 0 )
                {
                    //
                    // Get format string for NO Button.
                    // Write it to NoText String and to dlg box
                    //

                    NoTextFmt = GetResourceString ( IDS_NOTEXT );

                    if (NoTextFmt)
                    {
                        NoText = LocalAlloc ( LPTR, sizeof(TCHAR)*
                                                    ( lstrlen(NoTextFmt)+1 ) );
                        wsprintf(NoText, NoTextFmt, NOTimeOut--);

                        SetDlgItemText(hDlg, IDNO, NoText);

                        LocalFree ( NoTextFmt );
                        LocalFree ( NoText );
                    }

                }

                else
                {
                    //
                    // Give up on the user - return NO
                    //

                    KillTimer(hDlg, IDD_COUNTDOWN);
                    SendMessage(hDlg, WM_COMMAND, IDNO, 0);
                }

                return (TRUE);

            }

            break;


        case WM_COMMAND:

            //
            // Start processing tray messages again
            //

            Waiting=FALSE;

            switch (LOWORD(wParam))

            {

                //
                //  return value based on the button pressed
                //

                case IDYES :
                case IDNO :
                case IDABORT :
                case IDCANCEL :

                    //
                    //  LocalFree handles NULL pointers gracefully (does nothing)
                    //

                    LocalFree ( szAt );
                    LocalFree ( NewResString );
                    LocalFree ( NewHzString );
                    LocalFree ( TotalString );

                    EndDialog(hDlg, LOWORD(wParam));
                    return (TRUE);

                    break;

                default:

                    break;

            } // switch (wParam)

            break;

       default:

             break;

    } // switch (message)


    return (FALSE);     // Didn't process a message


} // KeepNewResDlgProc()



//
//*****************************************************************************
//
//  NTOptionsDlgProc(HWND, UINT, WPARAM, LPARAM )
//
//
//
//*****************************************************************************
//

INT_PTR FAR PASCAL NTOptionsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    INT i;
    static WORD SaveQRFlags;

    switch (message)
    {

        case WM_INITDIALOG:

            //
            // Stop processing tray messages; check buttons properly
            //

            Waiting = TRUE;
            SaveQRFlags = QuickResFlags;

            CheckRadioButton(hDlg,IDD_SORT_RES,IDD_SORT_BPP,
                                  (fSortByBPP ? IDD_SORT_BPP : IDD_SORT_RES) );
            CheckRadioButton(hDlg,IDD_SUBMENUS,IDD_ALLMODEMENU, FreqMenuLocation );

            CheckDlgButton(hDlg, IDD_UPDATEREG, fUpdateReg );
            CheckDlgButton(hDlg, IDD_REMMODES,  fRememberModes );
            CheckDlgButton(hDlg, IDD_RESTARTREQ,  fShowModesThatNeedRestart );
            CheckDlgButton(hDlg, IDD_SHOWTESTED,  fShowTestedModes );

            return TRUE;
            break;


        case WM_COMMAND:


            switch (LOWORD(wParam))

            {

                //
                //  Update buttons : sorting by BPP or Res?
                //

                case IDD_SORT_RES:
                case IDD_SORT_BPP:
                    CheckRadioButton(hDlg,IDD_SORT_RES,IDD_SORT_BPP,LOWORD(wParam));
                    return TRUE;
                    break;

                //
                //  Update buttons : where to display freq menus?
                //

                case IDD_SUBMENUS:
                case IDD_ONEMENUMOBILE:
                case IDD_ONEMENUBOTTOM:
                case IDD_ALLMODEMENU:
                    CheckRadioButton(hDlg,IDD_SUBMENUS,IDD_ALLMODEMENU,LOWORD(wParam));
                    return TRUE;
                    break;


                //
                //  Clear all registry remembered settings
                //  Make user verify he did this on purpose
                //

                case IDD_CLEARREG:
                    if (MsgBox(IDS_CLEARREG,
                               0,
                               MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL)
                         ==    IDYES)
                    {

                        //
                        //  Reset flags for all monitors; destroy and rebuild
                        //  each mode menu
                        //

                        for (i=0; i<iMonitors; i++)
                        {
                            SetDevmodeFlags(i, TRUE);
                            VALIDMODE(pMonitors[i].pCurrentdm) = MODE_BESTHZ;
                            DestroyModeMenu( i, TRUE, FALSE);
                        }
                    }

                    return TRUE;
                    break;


                //
                //  XOR QuickResFlags on and off
                //

                case IDD_UPDATEREG:
                    QuickResFlags ^= QF_UPDATEREG;
                    return TRUE;
                    break;

                case IDD_REMMODES:
                    QuickResFlags ^= QF_REMMODES;
                    return TRUE;
                    break;

                case IDD_RESTARTREQ:
                    QuickResFlags ^= QF_SHOWRESTART;
                    return TRUE;
                    break;

                case IDD_SHOWTESTED:
                    QuickResFlags ^= QF_SHOWTESTED;
                    return TRUE;
                    break;


                case IDOK:
                {

                    BOOL bRebuild    = FALSE;
                    BOOL bNeedToSort = FALSE;


                    //
                    //  See if sort order has changed.
                    //

                    if ( (IsDlgButtonChecked (hDlg, IDD_SORT_RES) &&  fSortByBPP) ||
                         (IsDlgButtonChecked (hDlg, IDD_SORT_BPP) && !fSortByBPP) )
                    {
                        QuickResFlags ^= QF_SORT_BYBPP;
                        bNeedToSort = TRUE;
                    }


                    //
                    //  If "show modes that require restart", or "show tested "modes only"
                    //  changed, then rebuild is required
                    //

                    if ( (fShowModesThatNeedRestart != (SaveQRFlags & QF_SHOWRESTART)) ||
                         (fShowTestedModes          != (SaveQRFlags & QF_SHOWTESTED))
                       )
                    {
                        bRebuild = TRUE;
                    }


                    //
                    // see if FreqMenuLocation has changed
                    //

                    if (!IsDlgButtonChecked (hDlg, FreqMenuLocation))
                    {
                        WORD i;

                        //
                        //  Freq menu location has changed; update & ask for rebuild
                        //

                        bRebuild = TRUE;

                        for ( i=IDD_SUBMENUS; i <= IDD_ALLMODEMENU; i++ )
                        {
                            if (IsDlgButtonChecked (hDlg, i))
                            {
                                FreqMenuLocation = i;
                            }
                        }
                    }


                    //
                    //  If rebuilding and or resorting, destroy old menus & rebuild
                    //

                    if ( bNeedToSort || bRebuild )
                    {
                        for (i=0; i<iMonitors; i++)
                            DestroyModeMenu( i, TRUE, bNeedToSort);
                    }


                    SaveAllSettings();

                    Waiting = FALSE;
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                    break;
                }

                case IDCANCEL :

                    Waiting = FALSE;
                    QuickResFlags = SaveQRFlags;
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                    break;

                default:

                    break;

            } // switch (wParam)

            break;

       default:

             break;

    } // switch (message)


    return FALSE;     // Didn't process a message


} // NTOptionsDlgProc()



//
//*****************************************************************************
//
//  W95OptionsDlgProc(HWND, UINT, WPARAM, LPARAM )
//
//
//
//*****************************************************************************
//

INT_PTR FAR PASCAL W95OptionsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    INT i;
    static WORD SaveQRFlags;

    switch (message)
    {

        case WM_INITDIALOG:

            //
            // Stop processing tray messages; check buttons properly
            //

            Waiting = TRUE;
            SaveQRFlags = QuickResFlags;

            CheckRadioButton(hDlg,IDD_SORT_RES,IDD_SORT_BPP,
                                  (fSortByBPP ? IDD_SORT_BPP : IDD_SORT_RES) );

            CheckDlgButton(hDlg, IDD_UPDATEREG, fUpdateReg );
            CheckDlgButton(hDlg, IDD_REMMODES,  fRememberModes );
            CheckDlgButton(hDlg, IDD_RESTARTREQ,  fShowModesThatNeedRestart );
            CheckDlgButton(hDlg, IDD_SHOWTESTED,  fShowTestedModes );

            return TRUE;
            break;


        case WM_COMMAND:


            switch (LOWORD(wParam))

            {

                //
                //  Update buttons : sorting by BPP or Res?
                //

                case IDD_SORT_RES:
                case IDD_SORT_BPP:
                    CheckRadioButton(hDlg,IDD_SORT_RES,IDD_SORT_BPP,LOWORD(wParam));
                    return TRUE;
                    break;

                //
                //  Clear all registry remembered settings
                //  Make user verify he did this on purpose
                //

                case IDD_CLEARREG:
                    if (MsgBox(IDS_CLEARREG,
                               0,
                               MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL)
                         ==    IDYES)
                    {

                        //
                        //  Reset flags for all monitors; destroy and rebuild
                        //  each mode menu
                        //

                        for (i=0; i<iMonitors; i++)
                        {
                            SetDevmodeFlags(i, TRUE);
                            VALIDMODE(pMonitors[i].pCurrentdm) = MODE_BESTHZ;
                            DestroyModeMenu( i, TRUE, FALSE);
                        }
                    }

                    return TRUE;
                    break;


                //
                //  XOR QuickResFlags on and off
                //

                case IDD_UPDATEREG:
                    QuickResFlags ^= QF_UPDATEREG;
                    return TRUE;
                    break;

                case IDD_REMMODES:
                    QuickResFlags ^= QF_REMMODES;
                    return TRUE;
                    break;

                case IDD_RESTARTREQ:
                    QuickResFlags ^= QF_SHOWRESTART;
                    return TRUE;
                    break;

                case IDD_SHOWTESTED:
                    QuickResFlags ^= QF_SHOWTESTED;
                    return TRUE;
                    break;

                case IDOK:
                {
                    BOOL bNeedToSort = FALSE;

                    //
                    //  Note if the sort order has changed
                    //

                    if ( (IsDlgButtonChecked (hDlg, IDD_SORT_RES) &&  fSortByBPP) ||
                         (IsDlgButtonChecked (hDlg, IDD_SORT_BPP) && !fSortByBPP) )
                    {
                        QuickResFlags ^= QF_SORT_BYBPP;
                        bNeedToSort = TRUE;
                    }


                    //
                    //  If "sort order", "show modes that require restart", or "show tested
                    //  "modes only" changed, then destroy and rebuild old menu (resort if nec.)
                    //

                    if ( bNeedToSort ||
                         (fShowModesThatNeedRestart != (SaveQRFlags & QF_SHOWRESTART)) ||
                         (fShowTestedModes          != (SaveQRFlags & QF_SHOWTESTED))
                       )
                    {
                        for (i=0; i<iMonitors; i++)
                            DestroyModeMenu( i, TRUE, bNeedToSort);
                    }

                    SaveAllSettings();

                    //
                    //  No break after IDOK, by design.
                    //  IDOK AND IDCANCEL : start processing tray clicks,
                    //  and return ok/cancel as return value.
                    //
                }

                case IDCANCEL :

                    Waiting = FALSE;
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                    break;

                default:

                    break;

            } // switch (wParam)

            break;

       default:

             break;

    } // switch (message)


    return FALSE;     // Didn't process a message


} // W95OptionsDlgProc()
