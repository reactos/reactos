/**************************************************/
/*					                              */
/*					                              */
/*	Registry Key Function		                  */
/*					                              */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include 	"stdafx.h"
#include 	"eudcedit.h"
#include 	"registry.h"
#include	"util.h"

static TCHAR subkey1[] = TEXT("EUDC");
static TCHAR subkey2[] = TEXT("System\\CurrentControlSet\\control\\Nls\\Codepage\\EUDCCodeRange");
static TCHAR SubKey[MAX_PATH];

#ifdef IN_FONTS_DIR // IsFileUnderWindowsRoot()
LPTSTR
IsFileUnderWindowsRoot(
LPTSTR TargetPath)
{
    TCHAR  WindowsRoot[MAX_PATH+1];
    UINT  WindowsRootLength;

    WindowsRootLength = GetSystemWindowsDirectory(WindowsRoot,MAX_PATH);

    if( lstrcmpi(WindowsRoot,TargetPath) == 0)
        return (TargetPath + WindowsRootLength);

    return NULL;
}

void AdjustTypeFace(WCHAR *orgName, WCHAR *newName)
{ 
  if (!lstrcmpW(orgName, L"\x5b8b\x4f53"))
    lstrcpy(newName, TEXT("Simsun"));
  else if (!lstrcmpW(orgName, L"\x65b0\x7d30\x660e\x9ad4"))
    lstrcpy(newName, TEXT("PMingLiU"));
  else if (!lstrcmpW(orgName, L"\xFF2d\xFF33\x0020\xFF30\x30b4\x30b7\x30c3\x30af"))
    lstrcpy(newName, TEXT("MS PGothic"));
  else if (!lstrcmpW(orgName, L"\xad74\xb9bc"))
    lstrcpy(newName, TEXT("Gulim"));
  else
    lstrcpy(newName, orgName);
}

#endif // IN_FONTS_DIR

/****************************************/
/*					*/
/*	Inquiry EUDC registry		*/
/*					*/
/****************************************/
BOOL
InqTypeFace(
TCHAR 	*typeface,
TCHAR 	*filename,
INT 	bufsiz)
{
	HKEY 	phkey;
	DWORD 	cb, dwType;
	LONG 	rc;
	TCHAR	FaceName[LF_FACESIZE];
	TCHAR	SysName[LF_FACESIZE];
#ifdef BUILD_ON_WINNT // InqTypeFace()
    TCHAR    FileName[MAX_PATH];
#endif // BUILD_ON_WINNT

	GetStringRes(SysName, IDS_SYSTEMEUDCFONT_STR);
	if( !lstrcmp(typeface, SysName)){
		lstrcpy(FaceName,TEXT("SystemDefaultEUDCFont"));
  }else {
#ifdef IN_FONTS_DIR
    AdjustTypeFace(typeface, FaceName);
#else
    lstrcpy(FaceName, typeface);
#endif
  }
	if( RegOpenKeyEx( HKEY_CURRENT_USER, (LPCTSTR)SubKey, 0,
	    KEY_ALL_ACCESS, &phkey) != ERROR_SUCCESS){
		return FALSE;
	}

#ifdef IN_FONTS_DIR // InqTypeFace()
	cb = (DWORD)MAX_PATH*sizeof(WORD)/sizeof(BYTE);
	rc = RegQueryValueEx(phkey, FaceName, 0, &dwType, 
		(LPBYTE)FileName, &cb);
	RegCloseKey(phkey);

    /*
     * if there is some error or no data, just return false.
     */
    if ((rc != ERROR_SUCCESS) || (FileName[0] == '\0')) {
        return (FALSE);
    }

    /*
     * expand %SystemRoot% to Windows direcotry.
     */
    ExpandEnvironmentStrings((LPCTSTR)FileName,(LPTSTR)filename,bufsiz);
#else
	cb = (DWORD)bufsiz*sizeof(WORD)/sizeof(BYTE);
	rc = RegQueryValueEx(phkey, (TCHAR *)FaceName, 0, &dwType, 
		(LPBYTE)filename, &cb);
	RegCloseKey(phkey);

	if ((rc != ERROR_SUCCESS) || (filename[0] == '\0')) {
        return (FALSE);
    }
#endif // IN_FONTS_DIR

#ifdef BUILD_ON_WINNT // InqTypeFace()
    /*
     * if this is not 'full path'. Build 'full path'.
     *
     *   EUDC.TTE -> C:\WINNT40\FONTS\EUDC.TTE
     *               0123456...
     *
     * 1. filename should have drive letter.
     * 2. filename should have one '\\' ,at least, for root.
     */
    if ((filename[1] != ':') || (Mytcsstr((const TCHAR *)filename,TEXT("\\")) == NULL)) {
        /* backup original.. */
        lstrcpy(FileName, (const TCHAR *)filename);

        /* Get windows directory */
        GetSystemWindowsDirectory((TCHAR *)filename, MAX_PATH);

#ifdef IN_FONTS_DIR // InqTypeFace()
        lstrcat((TCHAR *)filename, TEXT("\\FONTS\\"));
#else
        strcat((char *)filename, "\\");
#endif // IN_FONTS_DIR
        lstrcat((TCHAR *) filename, FileName);
    }
#endif // BUILD_ON_WINNT

#ifdef IN_FONTS_DIR // InqTypeFace()
	return (TRUE);
#else
	return rc == ERROR_SUCCESS && filename[0] != '\0' ? TRUE : FALSE;
#endif
}

/****************************************/
/*					*/
/*	Registry EUDC font and file	*/
/*					*/
/****************************************/
BOOL 
RegistTypeFace(
TCHAR 	*typeface, 
TCHAR	*filename)
{
	HKEY 	phkey;
	LONG 	rc;
	TCHAR	FaceName[LF_FACESIZE];
	TCHAR	SysName[LF_FACESIZE];
#ifdef IN_FONTS_DIR // RegistTypeFace()
    LPTSTR   SaveFileName;
    TCHAR    FileName[MAX_PATH];
#endif // IN_FONTS_DIR

	GetStringRes((TCHAR *)SysName, IDS_SYSTEMEUDCFONT_STR);
	if( !lstrcmp((const TCHAR *)typeface, (const TCHAR *)SysName)){
		lstrcpy(FaceName, TEXT("SystemDefaultEUDCFont"));
  }else{
#ifdef IN_FONTS_DIR
    AdjustTypeFace(typeface, FaceName);
#else
    lstrcpy(FaceName, (const TCHAR *)typeface);
#endif
  }
	if( RegOpenKeyEx( HKEY_CURRENT_USER, (LPCTSTR)SubKey, 0,
	    KEY_ALL_ACCESS, &phkey) != ERROR_SUCCESS){
		return FALSE;
	}

#ifdef IN_FONTS_DIR // RegistTypeFace()
    /*
     * if registry data contains full path, and the file is under windows
     * directory, replace the hardcodeed path with %SystemRoot%....
     */
    if( (SaveFileName = IsFileUnderWindowsRoot((LPTSTR)filename)) != NULL) {
        lstrcpy(FileName, TEXT("%SystemRoot%"));
        if( *SaveFileName != '\\' ) lstrcat(FileName, TEXT("\\"));
        lstrcat(FileName, SaveFileName );
    } else {
        lstrcpy(FileName, (TCHAR *)filename );
    }
	rc = RegSetValueEx( phkey, (LPCTSTR)FaceName, 0,
		REG_SZ, (const BYTE *)FileName, (lstrlen((LPCTSTR)FileName)+1)*sizeof(WORD)/sizeof(BYTE));
#else
	rc = RegSetValueEx( phkey, (LPCTSTR)FaceName, 0,
		REG_SZ, (const BYTE *)filename, (lstrlen((LPCTSTR)filename)+1)*sizeof(WORD)/sizeof(BYTE));
#endif // IN_FONTS_DIR
	RegCloseKey(phkey);
	return rc == ERROR_SUCCESS ? TRUE : FALSE;
}

/****************************************/
/*					*/
/*	Delete Registry string		*/
/*					*/
/****************************************/
BOOL 
DeleteReg( 
TCHAR	*typeface)
{
	HKEY phkey;
	LONG rc;
	TCHAR	FaceName[LF_FACESIZE];
	TCHAR	SysName[LF_FACESIZE];

	GetStringRes((TCHAR *)SysName, IDS_SYSTEMEUDCFONT_STR);
	if( !lstrcmp((const TCHAR *)typeface, (const TCHAR *)SysName)){
		lstrcpy((TCHAR *)FaceName, TEXT("SystemDefaultEUDCFont"));
  }else{
#ifdef IN_FONTS_DIR
    AdjustTypeFace(typeface, FaceName);
#else
    lstrcpy((TCHAR *)FaceName, (const TCHAR *)typeface);
#endif
  }
	if( RegOpenKeyEx(HKEY_CURRENT_USER, (LPCTSTR)SubKey, 0,
	    KEY_ALL_ACCESS, &phkey) != ERROR_SUCCESS){
		return FALSE;
	}
	rc = RegDeleteValue( phkey, (LPTSTR)FaceName);
	RegCloseKey(phkey);

	return rc == ERROR_SUCCESS ? TRUE : FALSE;
}

/****************************************/
/*					*/
/*	Create Registry Subkey		*/
/*					*/
/****************************************/
BOOL
CreateRegistrySubkey()
{
	HKEY 	phkey;
	DWORD 	dwdisp;
    int	    LocalCP;
	TCHAR	CodePage[10];
	int	result;

	/* New Registry	*/
	LocalCP = GetACP();

  	wsprintf( CodePage, TEXT("%d"), LocalCP);
    lstrcpy(SubKey, subkey1);
	lstrcat(SubKey, TEXT("\\"));
	lstrcat(SubKey, CodePage);

	if( RegOpenKeyEx( HKEY_CURRENT_USER, (LPCTSTR)SubKey, 0,
	    KEY_ALL_ACCESS, &phkey) != ERROR_SUCCESS){
		result = RegCreateKeyEx(HKEY_CURRENT_USER, 
			(LPCTSTR)SubKey, 0, TEXT(""),
			REG_OPTION_NON_VOLATILE, 
			KEY_ALL_ACCESS, NULL, &phkey, &dwdisp);
		if( result == ERROR_SUCCESS)
			RegCloseKey( phkey);
		else	return FALSE;
	}else 	RegCloseKey(phkey);

	return TRUE;
}

/****************************************/
/*					*/
/*	Inquiry Code range registry	*/
/*					*/
/****************************************/
BOOL 
InqCodeRange( 
TCHAR 	*Codepage, 
BYTE 	*Coderange, 
INT 	bufsiz)
{
	HKEY phkey;
	DWORD cb, dwType;
	LONG rc;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)subkey2, 0,
	    KEY_READ, &phkey) != ERROR_SUCCESS) {
		return FALSE;
	}
	cb = (DWORD)bufsiz * sizeof(WORD)/sizeof(BYTE);
	rc = RegQueryValueEx(phkey, (TCHAR *)Codepage, 0, &dwType, 
		(LPBYTE)Coderange, &cb);

	RegCloseKey(phkey);

	return rc == ERROR_SUCCESS && Coderange[0] != '\0' ? TRUE : FALSE;
}

BOOL
DeleteRegistrySubkey()
{
	HKEY 	phkey;

	if( RegOpenKeyEx( HKEY_CURRENT_USER, (LPCTSTR)SubKey, 0,
	    KEY_ALL_ACCESS, &phkey) == ERROR_SUCCESS){
		RegCloseKey(phkey);
		return RegDeleteKey(HKEY_CURRENT_USER, (LPCTSTR)SubKey);
	
	}

	return TRUE;
}

BOOL
FindFontSubstitute(TCHAR *orgFontName, TCHAR *sbstFontName)
{
  static TCHAR fsKey[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes");

  *sbstFontName = 0;
  lstrcpy(sbstFontName, orgFontName);
	HKEY phkey;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)fsKey, 0,
	    KEY_QUERY_VALUE, &phkey) != ERROR_SUCCESS) {
		return FALSE;
	}

  DWORD valueNameSize = LF_FACESIZE + 50; //should be facename + ',' + codepage
  TCHAR valueName[LF_FACESIZE + 50]; 
  DWORD valueType;
  DWORD valueDataSize = (LF_FACESIZE + 50) * sizeof(TCHAR); //should be facename + ',' + codepage
  BYTE  valueData[(LF_FACESIZE + 50) * sizeof(TCHAR)];
  LONG  ret;
  DWORD idx = 0;
  while ((ret = RegEnumValue(phkey, idx, valueName, &valueNameSize, 0, 
                        &valueType, valueData, &valueDataSize)) != ERROR_NO_MORE_ITEMS)
  {
    if (ret != ERROR_SUCCESS)
    {
      RegCloseKey(phkey);
      return FALSE;
    }
    Truncate(valueName, _T(','));
    if (!lstrcmpi(valueName, orgFontName))
    {
      Truncate((TCHAR *)valueData, _T(','));
      lstrcpy(sbstFontName, (TCHAR *)valueData);
      break;
    }
    idx ++;
    valueNameSize = LF_FACESIZE + 50;
    valueDataSize = (LF_FACESIZE + 50) * sizeof(TCHAR); 
  } 
  
  RegCloseKey(phkey);
  return TRUE;
}

void Truncate(TCHAR *str, TCHAR delim)
{
  TCHAR *pchr = _tcschr(str, delim);
  if (pchr)
    *pchr = 0;
}
