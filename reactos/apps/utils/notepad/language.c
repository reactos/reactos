/*
 *  Notepad
 *
 *  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
 *  Copyright 1998 Karl Backstr”m <karl_b@geocities.com>
 */

#include <stdio.h>
#include "windows.h"
#include "main.h"
#include "language.h"

CHAR STRING_MENU_Xx[]      = "MENU_Xx";
CHAR STRING_PAGESETUP_Xx[] = "DIALOG_PAGESETUP_Xx";

void LANGUAGE_UpdateWindowCaption(void) {
  /* Sets the caption of the main window according to Globals.szFileName:
      Notepad - (untitled)      if no file is open
      Notepad - [filename]      if a file is given
  */
  
  CHAR szCaption[MAX_STRING_LEN];
  CHAR szUntitled[MAX_STRING_LEN];

  LoadString(Globals.hInstance, IDS_NOTEPAD, szCaption, sizeof(szCaption));
  
  if (strlen(Globals.szFileName)>0) {
      lstrcat(szCaption, " - [");
      lstrcat(szCaption, Globals.szFileName);
      lstrcat(szCaption, "]");
  }
  else
  {
      LoadString(Globals.hInstance, IDS_UNTITLED, szUntitled, sizeof(szUntitled));
      lstrcat(szCaption, " - ");
      lstrcat(szCaption, szUntitled);
  }
    
  SetWindowText(Globals.hMainWnd, szCaption);
  
}



static BOOL LANGUAGE_LoadStringOther(UINT num, UINT ids, LPSTR str, UINT len)
{
  BOOL bOk;

  ids -= Globals.wStringTableOffset;
  ids += num * 0x100;

  bOk = LoadString(Globals.hInstance, ids, str, len);

  return(bOk);
}



VOID LANGUAGE_SelectByName(LPCSTR lang)
{
  INT i;
  CHAR newlang[3];

  for (i = 0; i <= MAX_LANGUAGE_NUMBER; i++)
    if (LANGUAGE_LoadStringOther(i, IDS_LANGUAGE_ID, newlang, sizeof(newlang)) &&
        !lstrcmp(lang, newlang))
      {
        LANGUAGE_SelectByNumber(i);
        return;
      }

  /* Fallback */
    for (i = 0; i <= MAX_LANGUAGE_NUMBER; i++)
    if (LANGUAGE_LoadStringOther(i, IDS_LANGUAGE_ID, newlang, sizeof(newlang)))
      {
        LANGUAGE_SelectByNumber(i);
        return;
      }

  MessageBox(Globals.hMainWnd, "No language found", "FATAL ERROR", MB_OK);
  PostQuitMessage(1);
}

VOID LANGUAGE_SelectByNumber(UINT num)
{
  INT    i;
  CHAR   lang[3];
  CHAR   item[MAX_STRING_LEN];
  HMENU  hMainMenu;

  /* Select string table */
  Globals.wStringTableOffset = num * 0x100;

  /* Get Language id */
  LoadString(Globals.hInstance, IDS_LANGUAGE_ID, lang, sizeof(lang));

  /* Set frame caption */
  LANGUAGE_UpdateWindowCaption();
  
  /* Change Resource names */
  lstrcpyn(STRING_MENU_Xx      + sizeof(STRING_MENU_Xx)      - 3, lang, 3);
  lstrcpyn(STRING_PAGESETUP_Xx + sizeof(STRING_PAGESETUP_Xx) - 3, lang, 3);

  /* Create menu */
  hMainMenu = LoadMenu(Globals.hInstance, STRING_MENU_Xx);
    Globals.hFileMenu     = GetSubMenu(hMainMenu, 0);
    Globals.hEditMenu     = GetSubMenu(hMainMenu, 1);
    Globals.hSearchMenu   = GetSubMenu(hMainMenu, 2);
    Globals.hLanguageMenu = GetSubMenu(hMainMenu, 3);
    Globals.hHelpMenu     = GetSubMenu(hMainMenu, 4);

  /* Remove dummy item */
  RemoveMenu(Globals.hLanguageMenu, 0, MF_BYPOSITION);
  /* Add language items */
  for (i = 0; i <= MAX_LANGUAGE_NUMBER; i++)
    if (LANGUAGE_LoadStringOther(i, IDS_LANGUAGE_MENU_ITEM, item, sizeof(item)))
      AppendMenu(Globals.hLanguageMenu, MF_STRING | MF_BYCOMMAND,
                 NP_FIRST_LANGUAGE + i, item);

  SetMenu(Globals.hMainWnd, hMainMenu);

  /* Destroy old menu */
  if (Globals.hMainMenu) DestroyMenu(Globals.hMainMenu);
  Globals.hMainMenu = hMainMenu;
}

VOID LANGUAGE_DefaultHandle(WPARAM wParam)
{
  if ((wParam >=NP_FIRST_LANGUAGE) && (wParam<=NP_LAST_LANGUAGE))
          LANGUAGE_SelectByNumber(wParam - NP_FIRST_LANGUAGE);
     else printf("Unimplemented menu command %i\n", wParam);
}
