#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "CurSchme.h"

DWORD g_dwSchemeSource;

static LPCTSTR g_rgszCursorNames[] = 
{
	__TEXT("Arrow"),    
	__TEXT("Help"),       
	__TEXT("AppStarting"),
	__TEXT("Wait"),       
	__TEXT("Crosshair"),  
	__TEXT("IBeam"),      
	__TEXT("NWPen"),      
	__TEXT("No"),         
	__TEXT("SizeNS"),     
	__TEXT("SizeWE"),     
	__TEXT("SizeNWSE"),   
	__TEXT("SizeNESW"),   
	__TEXT("SizeAll"),    
	__TEXT("UpArrow"),    
	__TEXT("Hand"),       
	NULL // This is the default value
};


#define CCURSORS   (sizeof(g_rgszCursorNames) / sizeof(g_rgszCursorNames[0]))

TCHAR g_szOrigCursors[CCURSORS][_MAX_PATH];
DWORD g_dwOrigSchemeSource = 0;

const TCHAR g_szCursorRegPath[] = REGSTR_PATH_CURSORS;
const TCHAR szSchemeSource[] = TEXT("Scheme Source");


TCHAR g_szSchemeNames[8][100]; // HACK - We have to make sure the scheme names are less than 100 characters



typedef
LANGID
(WINAPI *pfnGetUserDefaultUILanguage)(
    void
    );
typedef
LANGID
(WINAPI *pfnGetSystemDefaultUILanguage)(
    void
    );


BOOL IsMUI_Enabled()
{

    OSVERSIONINFO verinfo;
    LANGID        rcLang;
    HMODULE       hModule;
    pfnGetUserDefaultUILanguage gpfnGetUserDefaultUILanguage;     
    pfnGetSystemDefaultUILanguage gpfnGetSystemDefaultUILanguage; 
    static        g_bPFNLoaded=FALSE;
    static        g_bMUIStatus=FALSE;


    if(g_bPFNLoaded)
       return g_bMUIStatus;

    g_bPFNLoaded = TRUE;

    verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);    
    GetVersionEx( &verinfo) ;

    if (verinfo.dwMajorVersion == 5)        
    {   

       hModule = GetModuleHandle(TEXT("kernel32.dll"));
       if (hModule)
       {
          gpfnGetSystemDefaultUILanguage =
          (pfnGetSystemDefaultUILanguage)GetProcAddress(hModule,"GetSystemDefaultUILanguage");
          if (gpfnGetSystemDefaultUILanguage)
          {
             rcLang = (LANGID) gpfnGetSystemDefaultUILanguage();
             if (rcLang == 0x409 )
             {  
                gpfnGetUserDefaultUILanguage =
                (pfnGetUserDefaultUILanguage)GetProcAddress(hModule,"GetUserDefaultUILanguage");
                
                if (gpfnGetUserDefaultUILanguage)
                {
                   if (rcLang != (LANGID)gpfnGetUserDefaultUILanguage() )
                   {
                       g_bMUIStatus = TRUE;
                   }

                }
             }
          }
       }
    }
    return g_bMUIStatus;
}

void LoadCursorSchemeNames()
{  
	static BOOL g_bSchemeNamesLoaded = FALSE;
   
	if(g_bSchemeNamesLoaded)
		return;
	g_bSchemeNamesLoaded = TRUE;
   if (!IsMUI_Enabled())
   {
	   LoadString(g_hInstDll, IDS_CURSOR_SCHEME_WINDOWS_STANDARD_LARGE     , g_szSchemeNames[0], 100);
   	LoadString(g_hInstDll, IDS_CURSOR_SCHEME_WINDOWS_STANDARD_EXTRALARGE, g_szSchemeNames[1], 100);
   	LoadString(g_hInstDll, IDS_CURSOR_SCHEME_WINDOWS_BLACK              , g_szSchemeNames[2], 100);
   	LoadString(g_hInstDll, IDS_CURSOR_SCHEME_WINDOWS_BLACK_LARGE        , g_szSchemeNames[3], 100);
   	LoadString(g_hInstDll, IDS_CURSOR_SCHEME_WINDOWS_BLACK_EXTRALARGE   , g_szSchemeNames[4], 100);
   	LoadString(g_hInstDll, IDS_CURSOR_SCHEME_WINDOWS_INVERTED           , g_szSchemeNames[5], 100);
   	LoadString(g_hInstDll, IDS_CURSOR_SCHEME_WINDOWS_INVERTED_LARGE     , g_szSchemeNames[6], 100);
   	LoadString(g_hInstDll, IDS_CURSOR_SCHEME_WINDOWS_INVERTED_EXTRALARGE, g_szSchemeNames[7], 100);
   }
   else
   {     
      lstrcpy(g_szSchemeNames[0],IDSENG_CURSOR_SCHEME_WINDOWS_STANDARD_LARGE);    
      lstrcpy(g_szSchemeNames[1],IDSENG_CURSOR_SCHEME_WINDOWS_STANDARD_EXTRALARGE);
      lstrcpy(g_szSchemeNames[2],IDSENG_CURSOR_SCHEME_WINDOWS_BLACK);
      lstrcpy(g_szSchemeNames[3],IDSENG_CURSOR_SCHEME_WINDOWS_BLACK_LARGE);
      lstrcpy(g_szSchemeNames[4],IDSENG_CURSOR_SCHEME_WINDOWS_BLACK_EXTRALARGE);
      lstrcpy(g_szSchemeNames[5],IDSENG_CURSOR_SCHEME_WINDOWS_INVERTED);
      lstrcpy(g_szSchemeNames[6],IDSENG_CURSOR_SCHEME_WINDOWS_INVERTED_LARGE);
      lstrcpy(g_szSchemeNames[7],IDSENG_CURSOR_SCHEME_WINDOWS_INVERTED_EXTRALARGE);
   }
   
	// Load the current cursor settings
	HKEY hkCursors;
	if (ERROR_SUCCESS == RegOpenKey( HKEY_CURRENT_USER, g_szCursorRegPath, &hkCursors ))
	{
		for(int i=0;i<CCURSORS;i++)
		{
			g_szOrigCursors[i][0] = 0;
			DWORD dwCount = _MAX_PATH * sizeof(TCHAR);
			DWORD dwType;
			RegQueryValueEx( hkCursors,
					         g_rgszCursorNames[i],
					         NULL,
					         &dwType,
					         (LPBYTE)g_szOrigCursors[i],
					         &dwCount );
      
		}
		// Get the scheme source value
		DWORD dwLen = sizeof(g_dwOrigSchemeSource);
		if (RegQueryValueEx( hkCursors, szSchemeSource, NULL, NULL, (unsigned char *)&g_dwOrigSchemeSource, &dwLen ) != ERROR_SUCCESS)
			g_dwOrigSchemeSource = 1;
		RegCloseKey(hkCursors);
	}
	else
		_ASSERTE(FALSE);

}

static const TCHAR c_szRegPathCursorSchemes[] = REGSTR_PATH_CURSORS TEXT( "\\Schemes" );
static const TCHAR c_szRegPathSystemSchemes[] = REGSTR_PATH_SETUP TEXT("\\Control Panel\\Cursors\\Schemes");



// ApplyScheme(int nScheme)
// '0' Scheme loaded in g_szOrigScheme
// '1' Windows Default
// '2' Standard Large
// '3' Standard Ex Large
// '4' Black
// '5' Black Large
// '6' Black Ex Large
// '7' Inverted
// '8' Inverted Large
// '9' Inverted Ex Large
void ApplyCursorScheme(int nScheme)
{
	LoadCursorSchemeNames();
	HKEY hkCursors;
    DWORD dwPosition;

    // Initially for default cursor, The registry "\\ControlPanel\Cursors" is not created 
    // so. Create the registry values: a-anilk
	if(ERROR_SUCCESS != RegCreateKeyEx( HKEY_CURRENT_USER, g_szCursorRegPath, 0L, TEXT(""), 
        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkCursors, &dwPosition ))
		return;

	int i;

	DWORD dwSchemeSource;

	switch(nScheme)
	{
	case 0: // Original scheme
		dwSchemeSource = g_dwOrigSchemeSource;
		for(i=0;i<CCURSORS;i++)
			RegSetValueEx( hkCursors, g_rgszCursorNames[i], 0L, REG_SZ, (CONST LPBYTE)g_szOrigCursors[i], (lstrlen(g_szOrigCursors[i])+1)*sizeof(TCHAR));
		break;
	case 1: // Windows default
		dwSchemeSource = 0;
		for(i=0;i<CCURSORS;i++)
			RegSetValueEx( hkCursors, g_rgszCursorNames[i], 0L, REG_SZ, (CONST LPBYTE)"", sizeof(TCHAR));
		break;
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		{
			dwSchemeSource = 2; // Assume System schemes
			HKEY hkScheme;
			// Try to find the 'system' schemes first
			if(ERROR_SUCCESS != RegOpenKey( HKEY_LOCAL_MACHINE, c_szRegPathSystemSchemes, &hkScheme ))
			{
				// Couldn't find system schemes, try looking in user schemes
				dwSchemeSource = 1; // User schemes
				if(ERROR_SUCCESS != RegOpenKey( HKEY_CURRENT_USER, c_szRegPathCursorSchemes, &hkScheme ))
					return;
			}

			DWORD dwCount = 0;
			DWORD dwType;
			long nResult;
			if(ERROR_SUCCESS != (nResult = RegQueryValueEx( hkScheme, g_szSchemeNames[nScheme - 2], NULL, &dwType, NULL, &dwCount )))
				dwCount = 1; // The value probably was not there.  Fake it and allocate 1 byte.

			LPTSTR lpszData = (LPTSTR)new BYTE[dwCount]; // NOTE: For Unicode, RegQueryValueEx still returns the 'Byte' size not 'Char count'
			lpszData[0] = 0;

			if(ERROR_SUCCESS == nResult)
				RegQueryValueEx( hkScheme, g_szSchemeNames[nScheme - 2], NULL, &dwType, (LPBYTE)lpszData, &dwCount );


			LPTSTR lpszCurrentValue = lpszData;
			LPTSTR lpszFinalNULL = lpszData + lstrlen(lpszData);
			// Parse the information
			for(i=0;i<CCURSORS;i++)
			{
				// Hack to set the default value
				if(CCURSORS - 1 == i)
				{
					lpszCurrentValue = g_szSchemeNames[nScheme - 2];
					RegSetValueEx( hkCursors, NULL, 0L, REG_SZ, (CONST LPBYTE)lpszCurrentValue, (lstrlen(lpszCurrentValue)+1)*sizeof(TCHAR));
				}
				else
				{
					// Find next comma
					LPTSTR lpszComma = _tcschr(lpszCurrentValue, __TEXT(','));
					// Turn it into a zero
					if(lpszComma)
						*lpszComma = 0;
					RegSetValueEx( hkCursors, g_rgszCursorNames[i], 0L, REG_SZ, (CONST LPBYTE)lpszCurrentValue, (lstrlen(lpszCurrentValue)+1)*sizeof(TCHAR));
					lpszCurrentValue = min(lpszFinalNULL, lpszCurrentValue + lstrlen(lpszCurrentValue) + 1);
				}

			}
			delete [] lpszData;
			RegCloseKey(hkScheme);
		}
		break;
	default:
		_ASSERTE(FALSE);

	}

	// Save the 'Scheme Source'
	RegSetValueEx(hkCursors, szSchemeSource, 0, REG_DWORD, (unsigned char *)&dwSchemeSource, sizeof(dwSchemeSource));
	
	RegCloseKey(hkCursors);
	SystemParametersInfo( SPI_SETCURSORS, 0, 0, SPIF_SENDCHANGE );
}


