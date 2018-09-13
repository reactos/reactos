/*
1. on startup, check to see if we're administrator
  a) use RegOpenKeyEx on HKEY_USERS\.DEFAULT\Software with read/write
    access writes.  if it fails, we're not administrator
  b) if not, grey menu option
2. on startup
  a) use RegOpenKeyEx on HKEY_CURRENTUSER\Software...
  b) if it fails, create these keys with default values.
3. creating keys
  a) RegCreateKeyEx
  b) RegSetValue
  c) RegCloseKey

*/

#include "windows.h"

BOOL InitializeUserRegIfNeeded( void );
BOOL AreUserRegEntriesWritable( void );
BOOL DoAccessRegEntriesExist( HKEY hkeyRoot );
BOOL IsAdministrator( void );
BOOL CheckRegEntry( HKEY hkeyRoot, LPSTR lpsz, REGSAM sam );
LONG OpenAccessRegKeyW( HKEY hkeyRoot, LPSTR lpstr, PHKEY phkey );
BOOL CloseAccessRegKey( HKEY hkey );
BOOL SetRegString( HKEY hkey, LPSTR lpszEntry, LPSTR lpszValue );
BOOL SetDefaultRegEntries( HKEY hkeyRoot );

DWORD CopyKey( HKEY hkeySrc, HKEY hkeyDst, LPSTR szKey );
DWORD SaveDefaultSettings( void );


char szAccessRegPath[] = "Control Panel\\Accessibility";

/********************************************************************/
BOOL InitializeUserRegIfNeeded( void )
    {
    BOOL f = TRUE;
    DWORD dwOS;
    WORD wWinVer;
    BOOL fNT;

    dwOS = GetVersion();
    fNT = (HIWORD(dwOS) & 0x8000) == 0;
    wWinVer = LOWORD(GetVersion());

    // do the following only on Windows NT Version 3.5 (0x3203)
    // or Version 3.51 (0x3303).
    // on Chicago this is taken care of by the Enable VxD.
    if (fNT)
        {
        f = DoAccessRegEntriesExist( HKEY_CURRENT_USER );
        if( f != TRUE )
            return SetDefaultRegEntries( HKEY_CURRENT_USER );
        }
    return f;
    }

/********************************************************************/
// NOT CURRENTLY USED
BOOL AreUserRegEntriesWritable( void )
    {
    char sz[128];
    strcpy( sz, szAccessRegPath );
    strcat( sz, "\\StickyKeys" );
    return CheckRegEntry( HKEY_USERS, sz, KEY_ALL_ACCESS );
    }

/********************************************************************/
// NOT CURRENTLY USED
BOOL IsAdministrator( void )
    {
    return CheckRegEntry( HKEY_USERS, "\\Software", KEY_ALL_ACCESS );
    }

/********************************************************************/
BOOL DoAccessRegEntriesExist( HKEY hkeyRoot )
    {
    char sz[128];
    strcpy( sz, szAccessRegPath );
    strcat( sz, "\\StickyKeys" );
    return CheckRegEntry( hkeyRoot, sz, KEY_READ ); // execute means readonly
    }

/********************************************************************/
BOOL CheckRegEntry( HKEY hkeyRoot, LPSTR lpsz, REGSAM sam )
    {
    LONG dw;
    HKEY hkey;
    dw = RegOpenKey( hkeyRoot, lpsz, &hkey );
 // dw = RegOpenKeyEx( hkeyRoot, lpsz, 0, sam, &hkey );
    if( dw == ERROR_SUCCESS )
	{
	if( RegCloseKey( hkey ) != ERROR_SUCCESS )
	    {
	    //  _assert
	    }
	return TRUE;
	}
    else
	return FALSE;
    }

/********************************************************************/
LONG OpenAccessRegKeyW( HKEY hkeyRoot, LPSTR lpstr, PHKEY phkey )
    {
    LONG dw;
    LONG dwDisposition;
    char sz[128];
    strcpy( sz, szAccessRegPath );
    strcat( sz, "\\" );
    strcat( sz, lpstr );
//    dw = RegOpenKey( hkeyRoot, sz, phkey );
//    dw = RegOpenKey( hkeyRoot, "\\Software", phkey );
//    dw = RegOpenKey( hkeyRoot, "\\FOOBAR", phkey );
//    dw = RegOpenKey( hkeyRoot, "Software", phkey );
//    dw = RegOpenKey( hkeyRoot, "FOOBAR", phkey );
//    dw = RegOpenKey( hkeyRoot, "Software\\Microsoft\\Accessibility\\StickyKeys", phkey );
    dw = RegCreateKeyEx( hkeyRoot,
	    sz,
	    0,
	    NULL,                // CLASS NAME??
	    0,                   // by default is non-volatile
	    KEY_ALL_ACCESS,
	    NULL,                // default security descriptor
	    phkey,
	    &dwDisposition );    // yes we throw this away
    if( dw != ERROR_SUCCESS )
	{
	// should do something
	}
    return dw;
    }

/********************************************************************/
BOOL CloseAccessRegKey( HKEY hkey )
    {
    DWORD dw;
    dw = RegCloseKey( hkey );
    if( dw == ERROR_SUCCESS )
	return TRUE;
    else
	return FALSE;
    }

/********************************************************************/
BOOL SetRegString( HKEY hkey, LPSTR lpszEntry, LPSTR lpszValue )
    {
    DWORD dwResult;
    dwResult = RegSetValueEx( hkey,
			      lpszEntry,
			      0,
			      REG_SZ,
			      lpszValue,
			      strlen( lpszValue ) + sizeof( TCHAR ) );
    if( dwResult != ERROR_SUCCESS )
	{
	; // should do something like print a message
	return FALSE;
	}
    else
	return TRUE;
    }

/********************************************************************/
// NOTE THAT THE DEFAULTS INCLUDED HERE ARE FOR DAYTONA ONLY, NOT CHICAGO
//
BOOL SetDefaultRegEntries( HKEY hkeyRoot )
    {
    HKEY hkey;

    OpenAccessRegKeyW( hkeyRoot, "StickyKeys", &hkey );
#ifndef MANYFLAGS
    SetRegString( hkey, "Flags",                 "466" );
   // -------- flag --------------- value --------- default ------
   // #define SKF_STICKYKEYSON    0x00000001          0
   // #define SKF_AVAILABLE       0x00000002          2
   // #define SKF_HOTKEYACTIVE    0x00000004          0
   // #define SKF_CONFIRMHOTKEY   0x00000008          0
   // #define SKF_HOTKEYSOUND     0x00000010         10
   // #define SKF_INDICATOR       0x00000020          0
   // #define SKF_AUDIBLEFEEDBACK 0x00000040         40
   // #define SKF_TRISTATE        0x00000080         80
   // #define SKF_TWOKEYSOFF      0x00000100        100
   // ----------------------------------------- total = 0x1d2 = 466
   //
#else
    SetRegString( hkey, "On",                    "0" );
    SetRegString( hkey, "OnOffFeedback",         "1" );
    SetRegString( hkey, "AudibleFeedback",       "1" );
    SetRegString( hkey, "TriState",              "1" );
    SetRegString( hkey, "TwoKeysOff",            "1" );
    SetRegString( hkey, "HotkeyActive",          "0" );
    SetRegString( hkey, "Available",             "1" );
    SetRegString( hkey, "ConfirmHotkey",         "0" );
#endif
    CloseAccessRegKey( hkey );

    OpenAccessRegKeyW( hkeyRoot, "Keyboard Response", &hkey );
#ifndef MANYFLAGS
    SetRegString( hkey, "Flags",                 "82" );
    // -------- flag --------------- value --------- default ------
    // #define FKF_FILTERKEYSON    0x00000001           0
    // #define FKF_AVAILABLE       0x00000002           2
    // #define FKF_HOTKEYACTIVE    0x00000004           0
    // #define FKF_CONFIRMHOTKEY   0x00000008           0
    // #define FKF_HOTKEYSOUND     0x00000010          10
    // #define FKF_INDICATOR       0x00000020           0
    // #define FKF_CLICKON         0x00000040          40
    // ----------------------------------------- total = 0x52 = 82
#else
    SetRegString( hkey, "On",                    "0" );
    SetRegString( hkey, "OnOffFeedback",         "1" );
    SetRegString( hkey, "ClickOn",               "0" );
    SetRegString( hkey, "HotkeyActive",          "0" );
    SetRegString( hkey, "Available",             "1" );
    SetRegString( hkey, "ConfirmHotkey",         "0" );
#endif
    SetRegString( hkey, "DelayBeforeAcceptance", "1000" );
    SetRegString( hkey, "AutoRepeatRate",        "500" );
    SetRegString( hkey, "AutoRepeatDelay",       "1000" );
    SetRegString( hkey, "BounceTime",            "0" );
    CloseAccessRegKey( hkey );


    OpenAccessRegKeyW( hkeyRoot, "MouseKeys", &hkey );
#ifndef MANYFLAGS
    SetRegString( hkey, "Flags",                 "18" );
    // -------- flag --------------- value --------- default ------
    // #define MKF_MOUSEKEYSON     0x00000001           0
    // #define MKF_AVAILABLE       0x00000002           2
    // #define MKF_HOTKEYACTIVE    0x00000004           0
    // #define MKF_CONFIRMHOTKEY   0x00000008           0
    // #define MKF_HOTKEYSOUND     0x00000010          10
    // #define MKF_INDICATOR       0x00000020           0
    // #define MKF_NOMODIFIERS     0x00000040           0
    // #define MKF_REPLACENUMBERS  0x00000080           0
    // ----------------------------------------- total = 0x12 = 18
    //
#else
    SetRegString( hkey, "On",                    "0" );
    SetRegString( hkey, "OnOffFeedback",         "1" );
    SetRegString( hkey, "HotkeyActive",          "0" );
    SetRegString( hkey, "Available",             "1" );
    SetRegString( hkey, "ConfirmHotkey",         "0" );
#endif
    SetRegString( hkey, "MaximumSpeed",          "40" );
    SetRegString( hkey, "TimeToMaximumSpeed",    "3000" );
    CloseAccessRegKey( hkey );


    OpenAccessRegKeyW( hkeyRoot, "ToggleKeys", &hkey );
#ifndef MANYFLAGS
    SetRegString( hkey, "Flags",                 "18" );
    // -------- flag --------------- value --------- default ------
    // #define TKF_TOGGLEKEYSON    0x00000001           0
    // #define TKF_AVAILABLE       0x00000002           2
    // #define TKF_HOTKEYACTIVE    0x00000004           0
    // #define TKF_CONFIRMHOTKEY   0x00000008           0
    // #define TKF_HOTKEYSOUND     0x00000010          10
    // #define TKF_INDICATOR       0x00000020           0
    // ----------------------------------------- total = 0x12 = 18
    //
#else
    SetRegString( hkey, "On",                    "0" );
    SetRegString( hkey, "OnOffFeedback",         "1" );
    SetRegString( hkey, "HotkeyActive",          "0" );
    SetRegString( hkey, "Available",             "1" );
    SetRegString( hkey, "ConfirmHotkey",         "0" );
#endif
    CloseAccessRegKey( hkey );


    OpenAccessRegKeyW( hkeyRoot, "TimeOut", &hkey );
#ifndef MANYFLAGS
    SetRegString( hkey, "Flags",                 "2" );
    //
    // -------- flag --------------- value --------- default ------
    // #define ATF_TIMEOUTON       0x00000001           0
    // #define ATF_ONOFFFEEDBACK   0x00000002           2
    // ----------------------------------------- total = 0x2 = 2
    //
#else
    SetRegString( hkey, "On",                    "0" );
    SetRegString( hkey, "OnOffFeedback",         "1" );
#endif
    SetRegString( hkey, "TimeToWait",            "300000" );
    CloseAccessRegKey( hkey );


    OpenAccessRegKeyW( hkeyRoot, "SoundSentry", &hkey );
#ifndef MANYFLAGS
    SetRegString( hkey, "Flags",                 "2" );
    //
    // -------- flag --------------- value --------- default ------
    // #define SSF_SOUNDSENTRYON   0x00000001           0
    // #define SSF_AVAILABLE       0x00000002           1
    // #define SSF_INDICATOR       0x00000004           0
    // ----------------------------------------- total = 0x2 = 2
#else
    SetRegString( hkey, "On",                    "0" );
    SetRegString( hkey, "Available",             "1" );
#endif
    SetRegString( hkey, "FSTextEffect",          "0" );
    SetRegString( hkey, "WindowsEffect",         "0" );
    CloseAccessRegKey( hkey );


    OpenAccessRegKeyW( hkeyRoot, "ShowSounds", &hkey );
    SetRegString( hkey, "On",                    "0" );
    CloseAccessRegKey( hkey );


    return TRUE;
    }


/***********************************************************************/

DWORD SaveDefaultSettings( void )
    {
    DWORD iStatus;
    HKEY hkeyDst;

    iStatus  = RegOpenKey( HKEY_USERS, ".DEFAULT", &hkeyDst );
    if( iStatus != ERROR_SUCCESS )
	return iStatus;
    iStatus = CopyKey( HKEY_CURRENT_USER, hkeyDst, szAccessRegPath );
    RegCloseKey( hkeyDst );
    return iStatus;
    }

/***********************************************************************/
// CopyKey( hKey, hKeyDst, name )
//     create the destination key
//     for each value
//         CopyValue
//     for each subkey
//         CopyKey

DWORD CopyKey( HKEY hkeySrc, HKEY hkeyDst, LPSTR szKey )
    {
    HKEY hkeyOld, hkeyNew;
    char szValue[128];
    char szData[128];
    char szBuffer[128];
    DWORD iStatus;
    UINT nValue, nKey;
    UINT iValueLen, iDataLen;

    iStatus = RegOpenKey( hkeySrc, szKey, &hkeyOld );
    if( iStatus != ERROR_SUCCESS )
	return iStatus;
    iStatus = RegOpenKey( hkeyDst, szKey, &hkeyNew );
    if( iStatus != ERROR_SUCCESS )
	{
	iStatus = RegCreateKey( hkeyDst, szKey, &hkeyNew );
	if( iStatus != ERROR_SUCCESS )
	    {
	    RegCloseKey( hkeyOld );
	    return iStatus;
	    }
	}
    //*********** copy the values **************** //

    for( nValue = 0, iValueLen=sizeof szValue, iDataLen=sizeof szValue;
	 ERROR_SUCCESS == (iStatus = RegEnumValue(hkeyOld,
						  nValue,
						  szValue,
						  &iValueLen,
						  NULL, // reserved
						  NULL, // don't need type
						  szData,
						  &iDataLen ) );
	 nValue ++, iValueLen=sizeof szValue, iDataLen=sizeof szValue )
	 {
	 iStatus = RegSetValueEx( hkeyNew,
				  szValue,
				  0, // reserved
				  REG_SZ,
				  szData,
				  strlen( szData ) + 1 );
	 }
    if( iStatus != ERROR_NO_MORE_ITEMS )
	{
	RegCloseKey( hkeyOld );
	RegCloseKey( hkeyNew );
	return iStatus;
	}

    //*********** copy the subtrees ************** //

    for( nKey = 0;
	 ERROR_SUCCESS == (iStatus = RegEnumKey(hkeyOld,nKey,szBuffer,sizeof(szBuffer)));
	 nKey ++ )
	 {
	 iStatus = CopyKey( hkeyOld, hkeyNew, szBuffer );
	 if( iStatus != ERROR_NO_MORE_ITEMS && iStatus != ERROR_SUCCESS )
		{
		RegCloseKey( hkeyOld );
		RegCloseKey( hkeyNew );
		return iStatus;
		}
	 }
    RegCloseKey( hkeyOld );
    RegCloseKey( hkeyNew );
    if( iStatus == ERROR_NO_MORE_ITEMS )
	return ERROR_SUCCESS;
    else
	return iStatus;
    }
