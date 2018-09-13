//------------------------------------------------------------------------------------
//
//      File: REGUTILS.CPP
//
//      Helper functions that handle reading and writing strings to the system registry.
//
//------------------------------------------------------------------------------------

#include "precomp.hxx"
#pragma hdrstop

#define MAX_VALUELEN    16

const TCHAR c_szSoftwareClassesFmt[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\");

// our own atoi function so we don't have to link to the C runtimes...
INT AtoI( LPCTSTR pValue )
{
    INT i = 0;
    INT iSign = 1;

    while( pValue && *pValue )
    {
        if (*pValue == TEXT('-'))
        {
            iSign = -1;
        }
        else
        {
            i = (i*10) + (*pValue - TEXT('0'));
        }
        pValue++;
    }

    return (i * iSign);
}

void ItoA ( INT val, LPTSTR buf, UINT radix )
{
    LPTSTR p;               /* pointer to traverse string */
    LPTSTR firstdig;        /* pointer to first digit */
    TCHAR temp;             /* temp char */
    INT digval;             /* value of digit */
    
    p = buf;
    
    if (val < 0) {
        /* negative, so output '-' and negate */
        *p++ = TEXT('-');
        val = -val;
    }
    
    firstdig = p;           /* save pointer to first digit */
    
    do {
        digval = (val % radix);
        val /= radix;       /* get next digit */
        
        /* convert to ascii and store */
        if (digval > 9)
            *p++ = (TCHAR) (digval - 10 + TEXT('a'));  /* a letter */
        else
            *p++ = (TCHAR) (digval + TEXT('0'));       /* a digit */
    } while (val > 0);
    
    /* We now have the digit of the number in the buffer, but in reverse
    order.  Thus we reverse them now. */
    
    *p-- = TEXT('\0');            /* terminate string; p points to last digit */
    
    do {
        temp = *p;
        *p = *firstdig;
        *firstdig = temp;   /* swap *p and *firstdig */
        --p;
        ++firstdig;         /* advance to next two digits */
    } while (firstdig < p); /* repeat until halfway */
}


//------------------------------------------------------------------------------------
//
//      IconSet/GetRegValueString()
//
//      Versions of Get/SetRegValueString that go to the user classes section.
//      This can be overridden by the bClasses flag, which will only write the
//      value to the HKEY_CLASSES_ROOT section.
//
//      Returns: success of string setting / retrieval
//
//------------------------------------------------------------------------------------
BOOL IconSetRegValueString(LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPCTSTR lpszValue)
{
    TCHAR szRegPath[MAX_PATH];

    lstrcpyn(szRegPath, c_szSoftwareClassesFmt, ARRAYSIZE(szRegPath));
    lstrcatn(szRegPath, lpszSubKey, ARRAYSIZE(szRegPath));
    return SetRegValueString(HKEY_CURRENT_USER, szRegPath, lpszValName, lpszValue);

}


BOOL IconGetRegValueString(LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPTSTR lpszValue, int iMaxSize)
{
    TCHAR szRegPath[MAX_PATH];

    lstrcpyn(szRegPath, c_szSoftwareClassesFmt, ARRAYSIZE(szRegPath));
    lstrcatn(szRegPath, lpszSubKey, ARRAYSIZE(szRegPath));

    if (!GetRegValueString(HKEY_CURRENT_USER, szRegPath, lpszValName, lpszValue, iMaxSize ))
        return GetRegValueString(HKEY_CLASSES_ROOT, lpszSubKey, lpszValName, lpszValue, iMaxSize);

    return TRUE;
}


//------------------------------------------------------------------------------------
//
//      GetRegValueString()
//
//      Just a little helper routine, gets an individual string value from the
//      registry and returns it to the caller. Takes care of registry headaches,
//      including a paranoid length check before getting the string.
//
//      Returns: success of string retrieval
//
//------------------------------------------------------------------------------------
BOOL GetRegValueString( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPTSTR lpszValue, int iMaxSize )
{
    HKEY hKey;                   // cur open key
    BOOL bOK = TRUE;
    DWORD dwSize, dwType;
    LONG lRet = RegOpenKeyEx( hMainKey, lpszSubKey, (DWORD)0, KEY_QUERY_VALUE, (PHKEY)&hKey );
    if( lRet != ERROR_SUCCESS )
    {
        return FALSE;
    }

    
    // now do our paranoid check of data size
    lRet = RegQueryValueEx(hKey, lpszValName, NULL, &dwType, NULL, &dwSize);
    
    if( ERROR_SUCCESS == lRet )
    {     // saw something there
        // here's the size check before getting the data
        if( dwSize > (DWORD)iMaxSize )
        { // if string too big
            //                      Assert(FALSE, "Humongous registry string; can't GetRegValue...\n");
            bOK = FALSE;               // can't read, so very bad news
        }
        else
        {                        
            // size is OK to continue
            // now really get the value
            lRet = RegQueryValueEx( hKey, lpszValName, NULL, &dwType, (LPBYTE)lpszValue, &dwSize );
            if( ERROR_SUCCESS != lRet )
                bOK = FALSE;
        }
    }
    else
    {
        bOK = FALSE;
    }
    
    // close subkey
    RegCloseKey( hKey );
    
    return (bOK);
}

//------------------------------------------------------------------------------------
//
//      GetRegValueInt()
//
//      Just a little helper routine, gets an individual string value from the
//      registry and returns it to the caller as an int. Takes care of registry headaches,
//      including a paranoid length check before getting the string.
//
//      Returns: success of string retrieval
//
//------------------------------------------------------------------------------------
BOOL GetRegValueInt( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, int* piValue )
{
    TCHAR szValue[16];
    BOOL bOK = GetRegValueString( hMainKey, lpszSubKey, lpszValName, szValue, 16 );
    *piValue = AtoI( szValue );

    return bOK;
}

//------------------------------------------------------------------------------------
//
//      SetRegValueString()
//
//      Just a little helper routine that takes string and writes it to the     registry.
//
//      Returns: success writing to Registry, should be always TRUE.
//
//------------------------------------------------------------------------------------
BOOL SetRegValueString( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPCTSTR lpszValue )
{
    HKEY hKey;                       // cur open key
    BOOL bOK = TRUE;
    
    // open this subkey
    LONG lRet = RegOpenKeyEx( hMainKey, lpszSubKey, 0, KEY_SET_VALUE, &hKey );
    
    // check that you got a good key here
    if( lRet != ERROR_SUCCESS )
    {
        DWORD dwDisposition;
        
        // **********************************************************************************
        // based on the sketchy documentation we have for this Reg* and Error stuff, we're
        // guessing that you've ended up here because this totally standard, Windows-defined
        // subkey name just doesn't happen to be defined for the current user.
        // **********************************************************************************
        
        // SO: Just create this subkey for this user, and maybe it will get used after you create and set it.
        // still successful so long as can create new subkey to write to
        
        lRet = RegCreateKeyEx( hMainKey, lpszSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE, NULL, &hKey, &dwDisposition );
        if( lRet != ERROR_SUCCESS )
        {
            bOK = FALSE;
        }
    }
    lRet = SHRegSetPath( hKey, NULL, lpszValName, lpszValue, 0);
    bOK = bOK && (lRet == ERROR_SUCCESS);
    RegCloseKey( hKey );
    return (bOK);
}

//------------------------------------------------------------------------------------
//
//      SetRegValueInt()
//
//      Just a little helper routine that takes an int and writes it as a string to the
//      registry.
//
//      Returns: success writing to Registry, should be always TRUE.
//
//------------------------------------------------------------------------------------
BOOL SetRegValueInt( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, int iValue )
{
    TCHAR lpszValue[16];

    ItoA( iValue, lpszValue, 10 );
    return SetRegValueString( hMainKey, lpszSubKey, lpszValName, lpszValue );
}


//------------------------------------------------------------------------------------
//
//      SetRegValueDword()
//
//      Just a little helper routine that takes an dword and writes it to the
//      supplied location.
//
//      Returns: success writing to Registry, should be always TRUE.
//
//------------------------------------------------------------------------------------
BOOL SetRegValueDword( HKEY hk, LPCTSTR pSubKey, LPCTSTR pValue, DWORD dwVal )
{
    HKEY hkey = NULL;
    BOOL bRet = FALSE;

    if (ERROR_SUCCESS == RegOpenKey( hk, pSubKey, &hkey ))
    {
        if (ERROR_SUCCESS == RegSetValueEx(hkey, pValue, 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(DWORD)))
        {
            bRet = TRUE;
        }
    }


    if (hkey)
    {
        RegCloseKey( hkey );
    }

    return bRet;
}


//------------------------------------------------------------------------------------
//
//      GetRegValueDword()
//
//      Just a little helper routine that takes an dword and writes it to the
//      supplied location.
//
//      Returns: success writing to Registry, should be always TRUE.
//
//------------------------------------------------------------------------------------
DWORD GetRegValueDword( HKEY hk, LPCTSTR pSubKey, LPCTSTR pValue )
{
    HKEY hkey = NULL;
    DWORD dwVal = REG_BAD_DWORD;

    if (ERROR_SUCCESS == RegOpenKey( hk, pSubKey, &hkey ))
    {
        DWORD dwType, dwSize = sizeof(DWORD);

        RegQueryValueEx( hkey, pValue, NULL, &dwType, (LPBYTE)&dwVal, &dwSize );
    }


    if (hkey)
    {
        RegCloseKey( hkey );
    }

    return dwVal;
}
