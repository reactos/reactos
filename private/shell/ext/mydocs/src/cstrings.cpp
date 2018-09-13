/*----------------------------------------------------------------------------
/ Title;
/   cstrings.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   Constant strings used by this app
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#pragma hdrstop

TCHAR const c_szRegistrySettings[]  = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders");
TCHAR const c_szDocumentSettings[]  = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Documents");
TCHAR const c_szPolicyDocumentSettings[] = TEXT("Software\\Policies\\Microsoft\\Explorer\\Documents");
TCHAR const c_szMyDocsCLSID[]       = TEXT("{450d8fba-ad25-11d0-98a8-0800361b1103}");
TCHAR const c_szCLSIDFormat[]       = TEXT("CLSID\\{450d8fba-ad25-11d0-98a8-0800361b1103}");
TCHAR const c_szAllSpecial[]        = TEXT("AllSpecialItems");
TCHAR const c_szSpecificSpecial[]   = TEXT("SpecificSpecialItems\\");
TCHAR const c_szFileName[]          = CFSTR_FILENAMEA;               // "FileName"
TCHAR const c_szCommand[]           = TEXT("command");
TCHAR const c_szConfig[]            = TEXT("Config");
TCHAR const c_szShellNew[]          = TEXT("ShellNew");
TCHAR const c_szPersonal[]          = TEXT("Personal");
TCHAR const c_szHidden[]            = TEXT("HideMyDocsFolder");
TCHAR const c_szPlaceRealPath[]     = TEXT("RealPath");
TCHAR const c_szPlaceDefaultIcon[]  = TEXT("DefaultIcon");
TCHAR const c_szShellInfo[]         = TEXT(".ShellClassInfo");
#ifndef WINNT
TCHAR const c_szProfRecKey[]        = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ProfileReconciliation");
#endif
TCHAR const c_szStarDotStar[]       = TEXT("*.*");
TCHAR const c_szDesktopIni[]        = TEXT("\\desktop.ini");
TCHAR const c_szPolicy[]            = TEXT("Software\\Policies\\Microsoft\\Windows\\Explorer");
TCHAR const c_szDisableChange[]     = TEXT("DisablePersonalDirChange");
TCHAR const c_szPerUserCLSID[]      = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CSLID\\{450d8fba-ad25-11d0-98a8-0800361b1103}");
TCHAR const c_szCLSID[]             = TEXT("CLSID");
TCHAR const c_szCLSID2[]            = TEXT("CLSID2");
TCHAR const c_szExplorerSettings[]  = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer");
TCHAR const c_szReadOnlyBit[]       = TEXT("UseReadOnlyForSystemFolders");
TCHAR const c_szNameSpaceKey[]      = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\NameSpace\\");
TCHAR const c_szMyDocsValue[]       = TEXT("My Documents");
TCHAR const c_szMyDocsExt[]         = TEXT(".mydocs");
