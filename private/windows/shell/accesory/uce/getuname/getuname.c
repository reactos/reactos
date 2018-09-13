////////////////////////////////////////////////////////////////
//
//  UName.dll
//
//
// Copyright (c) 1997-1999 Microsoft Corporation.
////////////////////////////////////////////////////////////////
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "GetUName.h"

BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)


{
  return 1;

  hinstDll = hInst;
  UNREFERENCED_PARAMETER(ul_reason_being_called);
  UNREFERENCED_PARAMETER(lpReserved);
}
   

////////////////////////////////////////////////////////////////
const TCHAR *Lead[] =
{
  TEXT("Kiyeok "),
  TEXT("Ssangkiyeok "),
  TEXT("Nieun "),
  TEXT("Tikeut "),
  TEXT("Ssangtikeut "),
  TEXT("Rieul "),
  TEXT("Mieum "),
  TEXT("Pieup "),
  TEXT("Ssangpieup "),
  TEXT("Sios "),
  TEXT("Ssangsios "),
  TEXT("Ieung "),
  TEXT("Cieuc "),
  TEXT("Ssangcieuc "),
  TEXT("Chieuch "),
  TEXT("Khieukh "),
  TEXT("Thieuth "),
  TEXT("Phieuph "),
  TEXT("Hieuh ")
};

const TCHAR *Vowel[] =
{
  TEXT("A"),
  TEXT("Ae"),
  TEXT("Ya"),
  TEXT("Yae"),
  TEXT("Eo"),
  TEXT("E"),
  TEXT("Yeo"),
  TEXT("Ye"),
  TEXT("O"),
  TEXT("Wa"),
  TEXT("Wae"),
  TEXT("Oe"),
  TEXT("Yo"),
  TEXT("U"),
  TEXT("Weo"),
  TEXT("We"),
  TEXT("Wi"),
  TEXT("Yu"),
  TEXT("Eu"),
  TEXT("Yi"),
  TEXT("I")
 };

const TCHAR *Trail[] =
{
  TEXT(""),
  TEXT(" Kiyeok"),
  TEXT(" Ssangkiyeok"),
  TEXT(" Kiyeoksios"),
  TEXT(" Nieun"),
  TEXT(" Nieuncieuc"),
  TEXT(" Nieunhieuh"),
  TEXT(" Tikeut"),
  TEXT(" Rieul"),
  TEXT(" Rieulkiyeok"),
  TEXT(" Rieulmieum"),
  TEXT(" Rieulpieup"),
  TEXT(" Rieulsios"),
  TEXT(" Rieulthieuth"),
  TEXT(" Rieulphieuph"),
  TEXT(" Rieulhieuh"),
  TEXT(" Mieum"),
  TEXT(" Pieup"),
  TEXT(" Pieupsios"),
  TEXT(" Sios"),
  TEXT(" Ssangsios"),
  TEXT(" Ieung"),
  TEXT(" Cieuc"),
  TEXT(" Chieuch"),
  TEXT(" Khieukh"),
  TEXT(" Thieuth"),
  TEXT(" Phieuph"),
  TEXT(" Hieuh")
 };
 
#define BUFLEN 256
////////////////////////////////////////////////////////////////
int APIENTRY GetUName(WORD wChar, LPTSTR lpUName)
{
  
  WCHAR  i;
  TCHAR buffer[BUFLEN];
  TCHAR szHangul[BUFLEN];
  TCHAR szCJK1[BUFLEN];
  TCHAR szCJK2[BUFLEN];
  TCHAR szEUDC[BUFLEN];
  TCHAR szUndef[BUFLEN];

	if ( hinstDll == NULL )
	{
		hinstDll = GetModuleHandle( TEXT("getuname") );
	}
 	i = IDS_UNAME + wChar;
    if(LoadString( hinstDll, i, buffer, BUFLEN))
	{
	lstrcpy( lpUName, buffer );
	return OTHERS;
    }

  LoadString(hinstDll, IDS_HANGULSYL,    szHangul, BUFLEN);
  LoadString(hinstDll, IDS_CJKUNDEFIDEO, szCJK1,   BUFLEN);
  LoadString(hinstDll, IDS_CJKCOMPIDEO,  szCJK2,   BUFLEN);
  LoadString(hinstDll, IDS_PRIVATECHAR,  szEUDC,   BUFLEN);
  LoadString(hinstDll, IDS_UNDEFINED,    szUndef,  BUFLEN);

  if(ISCJK1(wChar))
  {
    lstrcpy(lpUName, szCJK1);
    return CJK1;
  }

  if(ISHANGUL(wChar))
  {
    wChar -= HANGUL_BEG;
    lstrcpy(lpUName, szHangul);
    lstrcat(lpUName, Lead[(wChar / (NUM_VOWEL * NUM_TRAIL))            ]);
    lstrcat(lpUName, Vowel[(wChar % (NUM_VOWEL * NUM_TRAIL)) / NUM_TRAIL]);
    lstrcat(lpUName, Trail[(wChar % (NUM_VOWEL * NUM_TRAIL)) % NUM_TRAIL]);
    return HANGUL;
  }

  if(ISEUDC(wChar))
  {
    lstrcpy(lpUName, szEUDC);
    return EUDC;
  }

  if(ISCJK2(wChar))
  {
    lstrcpy(lpUName, szCJK2);
    return CJK2;
  }

  lstrcpy(lpUName, szUndef);
  return UNDEFINED;
}
