#include "Quickres.h"
#include "tchar.h"

extern HINSTANCE hInstApp;
extern LPQRMONITORINFO pMonitors;
extern INT       iMonitors;
extern WORD      QuickResFlags;
extern WORD      FreqMenuLocation;


#ifdef SAVEFLAGS

//
//****************************************************************************
//
//  CreateQuickResKey(  )
//
//  Create 'Quickres' key in the registry if it doesnt exist
//
//****************************************************************************
//

BOOL CreateQuickResKey( )
{

        HKEY   hKeyOpen;                      // key we try to open
        HKEY   hKeyCreate;                    // key to create: Quickres

        DWORD  RegReturn;                     // for reg APIs
        DWORD  Disposition;

        INT   i;                              // counter

        BOOL   bRet = FALSE;                  // return value



        //
        // Get the key which the QuickRes key will go under
        //

        if(  (RegReturn = RegOpenKeyEx(HKEY_CURRENT_USER,
                                       REGSTR_SOFTWARE,
                                       0,
                                       KEY_CREATE_SUB_KEY,
                                       &hKeyOpen))           == ERROR_SUCCESS )
        {

            //
            // Create my quickres key
            //

            if (RegReturn=RegCreateKeyEx(hKeyOpen,
                                         QUICKRES_KEY,
                                         0,
                                         0,
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_SET_VALUE,
                                         NULL,
                                         &hKeyCreate,
                                         &Disposition)       == ERROR_SUCCESS)
            {
                bRet = TRUE;
            }

            RegCloseKey(hKeyOpen);

        }

    return bRet;

}


//
//****************************************************************************
//
//   ReadRegistryValue( LPTSTR, PDWORD, PVOID, PDWORD)
//
//   Read a quickres value from the registry (either modes, flags or BPP)
//
//****************************************************************************
//

BOOL ReadRegistryValue( LPTSTR ValueName, PDWORD KeyType, PVOID Value, PDWORD RegKeySize )
{

    HKEY   hKeyOpen;                              // Reg Key with mode flags
    LONG   RegReturn;                             // ret value for reg APIs
    BOOL   ret=FALSE;                             // return value


    //
    // Try to open the quickres key
    //

    if( (RegReturn=RegOpenKeyEx(HKEY_CURRENT_USER,
                                REGSTR_QUICKRES,
                                0,
                                KEY_QUERY_VALUE,
                                &hKeyOpen))           == ERROR_SUCCESS )
    {

        //
        // Try to get the value
        //

        if ( (RegReturn=RegQueryValueEx(hKeyOpen,
                                        ValueName,
                                        NULL,
                                        KeyType,
                                        (LPBYTE)Value,
                                        RegKeySize))      == ERROR_SUCCESS )
        {
            ret = TRUE;
        }

        RegCloseKey(hKeyOpen);

    }

    else
    {
        CreateQuickResKey();
    }

    return ret;
}



//
//****************************************************************************
//
//   SetRegistryValue( UINT, UINT, PVOID, UINT )
//
//   Set requested value (modes, flags, or BPP) in the registry
//
//****************************************************************************
//

VOID SetRegistryValue(LPTSTR ValueName, UINT ValueType, PVOID Value, UINT size)
{

    HKEY   hKeyOpen;                           // Quickres key
    LONG   RegReturn;                          // reg APIs return value


    //
    //  try to open QuickRes key
    //

    if( (RegReturn=RegOpenKeyEx(HKEY_CURRENT_USER,
                                REGSTR_QUICKRES,
                                0,
                                KEY_WRITE,
                                &hKeyOpen))           == ERROR_SUCCESS )

    {

        //
        // Set the value under that key
        //

        RegSetValueEx( hKeyOpen,
                       ValueName,
                       0,
                       ValueType,
                       (LPBYTE)Value,
                       size );

        RegCloseKey(hKeyOpen);

    }

}


#endif  // SAVEFLAGS


//
//****************************************************************************
//
//   SetDevmodeFlags( INT, BOOL )
//
//   Upload value in ModeFlags, and current BPP to registry
//
//****************************************************************************
//

VOID SetDevmodeFlags( INT iDisplay, BOOL ClearAll )
{

#ifdef SAVEFLAGS

    PBYTE  ModeFlags;
    INT    i;
    TCHAR  RegModes[16] = REGDEVMODES;

    //
    //  Alloc  Modes/4 bytes so each mode has 2 bits
    //

    ModeFlags = LocalAlloc ( LPTR, (pMonitors[iDisplay].iModes+3) >> 2 );

    if (ClearAll)
    {
        //
        //  Clear out all Mode flags if requested
        //  need to set current mode as the only valid one
        //

        for ( i=0; i < pMonitors[iDisplay].iModes; i++)
        {
            VALIDMODE(&pMonitors[iDisplay].pModes[i]) = MODE_UNTESTED;
        }

    }

    //
    //  Pack valid mode flags into ModeFlags[]
    //

    for ( i=0; i < pMonitors[iDisplay].iModes; i++)
    {
        ModeFlags[i>>2] |= (VALIDMODE(&pMonitors[iDisplay].pModes[i]) << ((i%4)<<1) );
    }


    //
    //  Store modeflags in the registry
    //

    if (iDisplay)
    {
        TCHAR buff[4];

        _itot(iDisplay, buff, 4);
        lstrcat(RegModes, buff);
    }

    SetRegistryValue(RegModes, REG_BINARY,
                     ModeFlags, (pMonitors[iDisplay].iModes+3) >> 2 );


    LocalFree ( ModeFlags );

#endif

}


//
//****************************************************************************
//
//   GetDevmodeFlags( )
//
//   Read value from registry into global variable ModeFlags
//
//****************************************************************************
//

VOID GetDevmodeFlags( INT iDisplay )
{

    INT    i;

#ifdef SAVEFLAGS

    PBYTE  ModeFlags;
    DWORD  KeyType;
    DWORD  SavedBPP;
    DWORD  RegKeySize =  (pMonitors[iDisplay].iModes+3) >> 2 ;
    BOOL   bClear=FALSE;
    TCHAR  RegModes[16] = REGDEVMODES;


    ModeFlags = LocalAlloc ( LPTR, RegKeySize );


    //
    // Try to read value
    //

    if (iDisplay)
    {
        TCHAR buff[4];

        _itot(iDisplay, buff, 4);
        lstrcat(RegModes, buff);
    }

    if ( ReadRegistryValue(RegModes, &KeyType,
                           ModeFlags, &RegKeySize) )
    {

        //
        //  Changing BPP on the fly IS allowed on NT 4.0 - NOT on Win95
        //  NT ONLY : 'Good' modes are still good even if they are different BPP
        //  fShowFreqs is essentially the NT vs Win95 flag.
        //

        if (!fShowFreqs)
        {
            //
            // Make sure user hasnt changed BPP via the desktop applet
            //

            RegKeySize = sizeof( DWORD );


            if (ReadRegistryValue(REGBPP, &KeyType,
                               &SavedBPP, &RegKeySize))
            {

               //
               //  If BPP HAS changed, modeflags is now bogus.
               //  clear the flags.  Tell the user.
               //

               DEVMODE dm;

               GetCurrentDevMode(iDisplay, &dm);

               if ( SavedBPP != BPP(&dm) )
               {

                   bClear=TRUE;
                   MsgBox(IDS_CHANGEDBPP, SavedBPP, MB_OK|MB_ICONEXCLAMATION);

               }
            }

        }
    }
    else
    {

        //
        //  Couldnt read value from registry.
        //  Assume no modes work; clear the flags
        //

        bClear = TRUE;
    }


    if (bClear)
    {
        SetDevmodeFlags( iDisplay, TRUE );
    }

    else
    {
        //
        //  Unpack ModeFlags into a field in each devmode
        //  2 bits per devmode - shift right, and with %11
        //

        for (i=0; i < pMonitors[iDisplay].iModes; i++ )
        {
            VALIDMODE(&pMonitors[iDisplay].pModes[i]) = ((ModeFlags[i>>2]) >> ((i%4)<<1)) & 0x03;
        }

    }

    LocalFree ( ModeFlags );


#else


    //
    //  Not reading from the registry - assign all as untested
    //

    for (i=0; i < pMonitors[iDisplay].iModes; i++ )
    {
        VALIDMODE(&pMonitors[iDisplay].pModes[i]) = MODE_UNTESTED;
    }

#endif

}


//
//****************************************************************************
//
//   SetQuickResFlags( )
//
//   Upload QuickResFlags value to registry
//
//****************************************************************************
//

VOID SetQuickResFlags( )
{

#ifdef SAVEFLAGS

    DWORD BothFlags = (FreqMenuLocation << (8*sizeof(WORD))) | QuickResFlags;

    SetRegistryValue(REGFLAGS, REG_DWORD,
                     &BothFlags, sizeof(DWORD));

#endif

}


//
//****************************************************************************
//
//   GetQuickResFlags( )
//
//   Read value from registry into global QuickResFlags
//
//****************************************************************************
//

VOID GetQuickResFlags( )
{


#ifdef SAVEFLAGS


    DWORD KeyType;
    DWORD RegKeySize=sizeof(DWORD);
    DWORD BothFlags;

    //
    // Try to read value
    //

    if (!ReadRegistryValue(REGFLAGS, &KeyType,
                           &BothFlags, &RegKeySize) )
    {

        //
        // assume a flag value, and create it.
        //

        QuickResFlags = QF_SHOWRESTART | QF_REMMODES;
        FreqMenuLocation= IDD_SUBMENUS;
        SetQuickResFlags();


        #ifdef DONTPANIC

            MsgBox(IDS_DONTPANIC, KEEP_RES_TIMEOUT, MB_OK | MB_ICONEXCLAMATION );

        #endif

    }
    else
    {
        if ( !( FreqMenuLocation = (WORD)(BothFlags >> (8*sizeof(WORD))) ) )
            FreqMenuLocation = IDD_SUBMENUS;

        QuickResFlags = (WORD)(0xFFFF & BothFlags);
    }

    //
    //  Do this always!
    //

    QuickResFlags |= QF_HIDE_4BPP;

#else


#ifdef DONTPANIC

    MsgBox(IDS_DONTPANIC, KEEP_RES_TIMEOUT, MB_OK | MB_ICONEXCLAMATION );

#endif

    QuickResFlags = QF_SHOWRESTART | QF_REMMODES | QF_HIDE_4BPP;


#endif

}


//
//****************************************************************************
//
//   SaveAllSettings( )
//
//   Write QuickResFlags, devmode flags, and BPP to the registry
//
//****************************************************************************
//

VOID SaveAllSettings()
{

    SetQuickResFlags( );

    if (fRememberModes)
    {
        INT iDisplay;

        for (iDisplay = 0; iDisplay < iMonitors; iDisplay++)
        {
            SetDevmodeFlags( iDisplay, FALSE );
        }
    }

    SetRegistryValue(REGBPP, REG_DWORD,
                     &( BPP(&pMonitors[0].pCurrentdm->dm) ), sizeof(DWORD) );

    // bugbug :  only pays attention to monitor 0 (is this still necessary?)
}
