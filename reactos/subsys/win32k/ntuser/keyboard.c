/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: keyboard.c,v 1.8 2003/08/13 20:24:05 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Messages
 * FILE:             subsys/win32k/ntuser/keyboard.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <internal/safe.h>
#include <internal/kbd.h>
#include <include/guicheck.h>
#include <include/msgqueue.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/object.h>
#include <include/winsta.h>

#define NDEBUG
#include <debug.h>

DWORD ModBits = 0;
BYTE QueueKeyStateTable[256];
static PVOID pkKeyboardLayout = 0;

/* arty -- These should be phased out for the general kbdxx.dll tables */

struct accent_char
{
    BYTE ac_accent;
    BYTE ac_char;
    BYTE ac_result;
};

static const struct accent_char accent_chars[] =
{
/* A good idea should be to read /usr/X11/lib/X11/locale/iso8859-x/Compose */
    {'`', 'A', '\300'},  {'`', 'a', '\340'},
    {'\'', 'A', '\301'}, {'\'', 'a', '\341'},
    {'^', 'A', '\302'},  {'^', 'a', '\342'},
    {'~', 'A', '\303'},  {'~', 'a', '\343'},
    {'"', 'A', '\304'},  {'"', 'a', '\344'},
    {'O', 'A', '\305'},  {'o', 'a', '\345'},
    {'0', 'A', '\305'},  {'0', 'a', '\345'},
    {'A', 'A', '\305'},  {'a', 'a', '\345'},
    {'A', 'E', '\306'},  {'a', 'e', '\346'},
    {',', 'C', '\307'},  {',', 'c', '\347'},
    {'`', 'E', '\310'},  {'`', 'e', '\350'},
    {'\'', 'E', '\311'}, {'\'', 'e', '\351'},
    {'^', 'E', '\312'},  {'^', 'e', '\352'},
    {'"', 'E', '\313'},  {'"', 'e', '\353'},
    {'`', 'I', '\314'},  {'`', 'i', '\354'},
    {'\'', 'I', '\315'}, {'\'', 'i', '\355'},
    {'^', 'I', '\316'},  {'^', 'i', '\356'},
    {'"', 'I', '\317'},  {'"', 'i', '\357'},
    {'-', 'D', '\320'},  {'-', 'd', '\360'},
    {'~', 'N', '\321'},  {'~', 'n', '\361'},
    {'`', 'O', '\322'},  {'`', 'o', '\362'},
    {'\'', 'O', '\323'}, {'\'', 'o', '\363'},
    {'^', 'O', '\324'},  {'^', 'o', '\364'},
    {'~', 'O', '\325'},  {'~', 'o', '\365'},
    {'"', 'O', '\326'},  {'"', 'o', '\366'},
    {'/', 'O', '\330'},  {'/', 'o', '\370'},
    {'`', 'U', '\331'},  {'`', 'u', '\371'},
    {'\'', 'U', '\332'}, {'\'', 'u', '\372'},
    {'^', 'U', '\333'},  {'^', 'u', '\373'},
    {'"', 'U', '\334'},  {'"', 'u', '\374'},
    {'\'', 'Y', '\335'}, {'\'', 'y', '\375'},
    {'T', 'H', '\336'},  {'t', 'h', '\376'},
    {'s', 's', '\337'},  {'"', 'y', '\377'},
    {'s', 'z', '\337'},  {'i', 'j', '\377'},
	/* iso-8859-2 uses this */
    {'<', 'L', '\245'},  {'<', 'l', '\265'},	/* caron */
    {'<', 'S', '\251'},  {'<', 's', '\271'},
    {'<', 'T', '\253'},  {'<', 't', '\273'},
    {'<', 'Z', '\256'},  {'<', 'z', '\276'},
    {'<', 'C', '\310'},  {'<', 'c', '\350'},
    {'<', 'E', '\314'},  {'<', 'e', '\354'},
    {'<', 'D', '\317'},  {'<', 'd', '\357'},
    {'<', 'N', '\322'},  {'<', 'n', '\362'},
    {'<', 'R', '\330'},  {'<', 'r', '\370'},
    {';', 'A', '\241'},  {';', 'a', '\261'},	/* ogonek */
    {';', 'E', '\312'},  {';', 'e', '\332'},
    {'\'', 'Z', '\254'}, {'\'', 'z', '\274'},	/* acute */
    {'\'', 'R', '\300'}, {'\'', 'r', '\340'},
    {'\'', 'L', '\305'}, {'\'', 'l', '\345'},
    {'\'', 'C', '\306'}, {'\'', 'c', '\346'},
    {'\'', 'N', '\321'}, {'\'', 'n', '\361'},
/*  collision whith S, from iso-8859-9 !!! */
    {',', 'S', '\252'},  {',', 's', '\272'},	/* cedilla */
    {',', 'T', '\336'},  {',', 't', '\376'},
    {'.', 'Z', '\257'},  {'.', 'z', '\277'},	/* dot above */
    {'/', 'L', '\243'},  {'/', 'l', '\263'},	/* slash */
    {'/', 'D', '\320'},  {'/', 'd', '\360'},
    {'(', 'A', '\303'},  {'(', 'a', '\343'},	/* breve */
    {'\275', 'O', '\325'}, {'\275', 'o', '\365'},	/* double acute */
    {'\275', 'U', '\334'}, {'\275', 'u', '\374'},
    {'0', 'U', '\332'},  {'0', 'u', '\372'},	/* ring above */
	/* iso-8859-3 uses this */
    {'/', 'H', '\241'},  {'/', 'h', '\261'},	/* slash */
    {'>', 'H', '\246'},  {'>', 'h', '\266'},	/* circumflex */
    {'>', 'J', '\254'},  {'>', 'j', '\274'},
    {'>', 'C', '\306'},  {'>', 'c', '\346'},
    {'>', 'G', '\330'},  {'>', 'g', '\370'},
    {'>', 'S', '\336'},  {'>', 's', '\376'},
/*  collision whith G( from iso-8859-9 !!!   */
    {'(', 'G', '\253'},  {'(', 'g', '\273'},	/* breve */
    {'(', 'U', '\335'},  {'(', 'u', '\375'},
/*  collision whith I. from iso-8859-3 !!!   */
    {'.', 'I', '\251'},  {'.', 'i', '\271'},	/* dot above */
    {'.', 'C', '\305'},  {'.', 'c', '\345'},
    {'.', 'G', '\325'},  {'.', 'g', '\365'},
	/* iso-8859-4 uses this */
    {',', 'R', '\243'},  {',', 'r', '\263'},	/* cedilla */
    {',', 'L', '\246'},  {',', 'l', '\266'},
    {',', 'G', '\253'},  {',', 'g', '\273'},
    {',', 'N', '\321'},  {',', 'n', '\361'},
    {',', 'K', '\323'},  {',', 'k', '\363'},
    {'~', 'I', '\245'},  {'~', 'i', '\265'},	/* tilde */
    {'-', 'E', '\252'},  {'-', 'e', '\272'},	/* macron */
    {'-', 'A', '\300'},  {'-', 'a', '\340'},
    {'-', 'I', '\317'},  {'-', 'i', '\357'},
    {'-', 'O', '\322'},  {'-', 'o', '\362'},
    {'-', 'U', '\336'},  {'-', 'u', '\376'},
    {'/', 'T', '\254'},  {'/', 't', '\274'},	/* slash */
    {'.', 'E', '\314'},  {'.', 'e', '\344'},	/* dot above */
    {';', 'I', '\307'},  {';', 'i', '\347'},	/* ogonek */
    {';', 'U', '\331'},  {';', 'u', '\371'},
	/* iso-8859-9 uses this */
	/* iso-8859-9 has really bad choosen G( S, and I. as they collide
	 * whith the same letters on other iso-8859-x (that is they are on
	 * different places :-( ), if you use turkish uncomment these and
	 * comment out the lines in iso-8859-2 and iso-8859-3 sections
	 * FIXME: should be dynamic according to chosen language
	 *	  if/when Wine has turkish support.  
	 */ 
/*  collision whith G( from iso-8859-3 !!!   */
/*  {'(', 'G', '\320'},  {'(', 'g', '\360'}, */	/* breve */
/*  collision whith S, from iso-8859-2 !!! */
/*  {',', 'S', '\336'},  {',', 's', '\376'}, */	/* cedilla */
/*  collision whith I. from iso-8859-3 !!!   */
/*  {'.', 'I', '\335'},  {'.', 'i', '\375'}, */	/* dot above */
};

/* FUNCTIONS *****************************************************************/

/*** Statics used by TranslateMessage ***/

static VOID STDCALL SetKeyState(DWORD key, BOOL down) {
  if( key >= 'a' && key <= 'z' ) key += 'A' - 'a';
  QueueKeyStateTable[key] = down;
}

static BOOL SetModKey( PKBDTABLES pkKT, WORD wVK, BOOL down ) {
  int i;
  
  for( i = 0; pkKT->pCharModifiers->pVkToBit[i].Vk; i++ ) {
    DbgPrint( "vk[%d] = { %04x, %x }\n", i, 
	pkKT->pCharModifiers->pVkToBit[i].Vk,
	pkKT->pCharModifiers->pVkToBit[i].ModBits );
    if( pkKT->pCharModifiers->pVkToBit[i].Vk == wVK ) {
      if( down ) ModBits |= pkKT->pCharModifiers->pVkToBit[i].ModBits;
      else ModBits &= ~pkKT->pCharModifiers->pVkToBit[i].ModBits;
      DbgPrint( "ModBits: %x\n", ModBits );
      return TRUE;
    }
  }

  return FALSE;
}

static BOOL TryToTranslateChar( WORD wVirtKey,
				PVK_TO_WCHAR_TABLE vtwTbl, 
				DWORD ModBits,
				PBOOL pbDead,
				PBOOL pbLigature,
				PWCHAR pwcTranslatedChar ) {
  int i,j;
  size_t size_this_entry = vtwTbl->cbSize;
  int nStates = vtwTbl->nModifications;
  PVK_TO_WCHARS10 vkPtr;

  for( i = 0;; i++ ) {
    vkPtr = (PVK_TO_WCHARS10)
      (((BYTE *)vtwTbl->pVkToWchars) + i * size_this_entry);

    if( !vkPtr->VirtualKey ) return FALSE;
    if( wVirtKey == vkPtr->VirtualKey ) {
      for( j = 0; j < nStates; j++ ) {
	if( j == (int) ModBits ) { /* OK, we found a wchar with the correct
				shift state and vk */
	  *pbDead = vkPtr->wch[j] == WCH_DEAD;
	  *pbLigature = vkPtr->wch[j] == WCH_LGTR;
	  *pwcTranslatedChar = vkPtr->wch[j];
	  if( *pbDead ) {
	    i++;
	    vkPtr = (PVK_TO_WCHARS10)
	      (((BYTE *)vtwTbl->pVkToWchars) + i * size_this_entry);
	    if( vkPtr->VirtualKey != 0xff ) {
	      DPRINT( "Found dead key with no trailer in the table.\n" );
	      DPRINT( "VK: %04x, ADDR: %08x\n", wVirtKey, (int)vkPtr );
	      return FALSE;
	    }
	    *pwcTranslatedChar = vkPtr->wch[j];
	  }
	  return TRUE;
	}
      }
    }
  }
}

static
int STDCALL
ToUnicodeInner(UINT wVirtKey,
	       UINT wScanCode,
	       PBYTE lpKeyState,
	       LPWSTR pwszBuff,
	       int cchBuff,
	       UINT wFlags,
	       DWORD ModBits,
	       PKBDTABLES pkKT)
{
  int i;

  DbgPrint("wVirtKey=%08x, wScanCode=%08x, lpKeyState=[], "
	   "pwszBuff=%S, cchBuff=%d, wFlags=%x\n",
	   wVirtKey, wScanCode, /* lpKeyState, */ pwszBuff,
	   cchBuff, wFlags );

  for( i = 0; pkKT->pVkToWcharTable[i].nModifications; i++ ) {
    WCHAR wcTranslatedChar;
    BOOL bDead;
    BOOL bLigature;

    if( TryToTranslateChar( wVirtKey,
			    &pkKT->pVkToWcharTable[i], 
			    ModBits,
			    &bDead,
			    &bLigature,
			    &wcTranslatedChar ) ) {
      if( bLigature ) {
	DPRINT("Not handling ligature (yet)\n" );
	return 0;
      }

      if( cchBuff > 0 ) pwszBuff[0] = wcTranslatedChar;

      if( bDead ) return -1;
      else return 1;
    }
  }

  return 0;
}

DWORD
STDCALL
NtUserGetKeyState(
  DWORD key)
{
  DWORD ret;

    if (key >= 'a' && key <= 'z') key += 'A' - 'a';
    ret = ((DWORD)(QueueKeyStateTable[key] & 0x80) << 8 ) |
              (QueueKeyStateTable[key] & 0x80) |
              (QueueKeyStateTable[key] & 0x01);
    return ret;
}

int STDCALL ToUnicode( UINT wVirtKey,
		       UINT wScanCode,
		       PBYTE lpKeyState,
		       LPWSTR pwszBuff,
		       int cchBuff,
		       UINT wFlags ) {
  return ToUnicodeInner( wVirtKey,
			 wScanCode,
			 QueueKeyStateTable,
			 pwszBuff,
			 cchBuff,
			 wFlags,
			 ModBits,
			 pkKeyboardLayout );
}

typedef PVOID (*KbdLayerDescriptor)(VOID);
NTSTATUS STDCALL LdrGetProcedureAddress(PVOID module,
					PANSI_STRING import_name,
					DWORD flags,
					PVOID *func_addr);

void InitKbdLayout( PVOID *pkKeyboardLayout ) {
  HMODULE kbModule = 0;
  ANSI_STRING kbdProcedureName;
  //NTSTATUS Status;

  KbdLayerDescriptor layerDescGetFn;

  kbModule = EngLoadImage(L"\\SystemRoot\\system32\\kbdus.dll");

  if( !kbModule ) {
    DbgPrint( "Foo: No kbdus.dll\n" );
    return;
  }

  RtlInitAnsiString( &kbdProcedureName, "KbdLayerDescriptor" );
  LdrGetProcedureAddress((PVOID)kbModule,
			 &kbdProcedureName,
			 0,
			 (PVOID*)&layerDescGetFn);
  if( layerDescGetFn ) {
    *pkKeyboardLayout = layerDescGetFn();
  }
}

BOOL STDCALL
NtUserTranslateMessage(LPMSG lpMsg,
		       DWORD Unknown1) /* Used to pass the kbd layout */
{
  static INT dead_char = 0;
  LONG UState = 0;
  WCHAR wp[2] = { 0 };
  MSG NewMsg = { 0 };
  PUSER_MESSAGE UMsg;

  /* FIXME: Should pass current keyboard layout for this thread. */
  /* At the moment, the keyboard layout is global. */
  /* Also, we're fixed at kbdus.dll ... */
  if( !pkKeyboardLayout ) InitKbdLayout( &pkKeyboardLayout );
  if( !pkKeyboardLayout ) {
    DbgPrint( "Not Translating due to empty layout.\n" );
    return FALSE;
  }

  if (lpMsg->message != WM_KEYDOWN && lpMsg->message != WM_SYSKEYDOWN)
    {
      if (lpMsg->message == WM_KEYUP) {
	DbgPrint( "About to SetKeyState( %04x, FALSE );\n", lpMsg->wParam );
	SetKeyState( lpMsg->wParam, FALSE ); /* Release key */
        DbgPrint( "About to SetModKey();\n" );
	SetModKey( pkKeyboardLayout, lpMsg->wParam, FALSE );
	/* Release Mod if any */
	DbgPrint( "Done with keys.\n" );
      }
      return(FALSE);
    }

  DbgPrint( "About to SetKeyState( %04x, TRUE );\n", lpMsg->wParam );
  SetKeyState( lpMsg->wParam, TRUE ); /* Strike key */

  /* Pass 1: Search for modifiers */
  DbgPrint( "About to SetModKey();\n" );
  if( SetModKey( pkKeyboardLayout, lpMsg->wParam, TRUE ) ) return TRUE;
  DbgPrint( "Done with keys.\n" );

  /* Pass 2: Get Unicode Character */
  DbgPrint( "Calling ToUnicodeString()\n" );
  UState = ToUnicodeInner(lpMsg->wParam, HIWORD(lpMsg->lParam),
			  QueueKeyStateTable, wp, 2, 0, ModBits,
			  pkKeyboardLayout);

  DbgPrint( "UState is %d after key %04x\n", UState, wp[0] );
  if (UState == 1)
    {
      NewMsg.message = (lpMsg->message == WM_KEYDOWN) ? WM_CHAR : WM_SYSCHAR;
      if (dead_char)
        {
	  ULONG i;
	  
	  if (wp[0] == ' ') wp[0] =  dead_char;
	  if (dead_char == 0xa2) dead_char = '(';
	  else if (dead_char == 0xa8) dead_char = '"';
	  else if (dead_char == 0xb2) dead_char = ';';
	  else if (dead_char == 0xb4) dead_char = '\'';
	  else if (dead_char == 0xb7) dead_char = '<';
	  else if (dead_char == 0xb8) dead_char = ',';
	  else if (dead_char == 0xff) dead_char = '.';
	  for (i = 0; i < sizeof(accent_chars)/sizeof(accent_chars[0]); i++)
	    {
	      if ((accent_chars[i].ac_accent == dead_char) &&
		  (accent_chars[i].ac_char == wp[0]))
                {
		  wp[0] = accent_chars[i].ac_result;
		  break;
                }
	      dead_char = 0;
	    }
        }
      NewMsg.hwnd = lpMsg->hwnd;
      NewMsg.wParam = wp[0];
      NewMsg.lParam = lpMsg->lParam;
      UMsg = MsqCreateMessage(&NewMsg);
      DbgPrint( "CHAR='%c' %04x %08x\n", wp[0], wp[0], lpMsg->lParam );
      MsqPostMessage(PsGetWin32Thread()->MessageQueue, UMsg);
      return(TRUE);
    }
  else if (UState == -1)
    {
      NewMsg.message = 
	(lpMsg->message == WM_KEYDOWN) ? WM_DEADCHAR : WM_SYSDEADCHAR;
      NewMsg.hwnd = lpMsg->hwnd;
      NewMsg.wParam = wp[0];
      NewMsg.lParam = lpMsg->lParam;
      dead_char = wp[0];
      UMsg = MsqCreateMessage(&NewMsg);
      MsqPostMessage(PsGetWin32Thread()->MessageQueue, UMsg);
      return(TRUE);
    }
  return(FALSE);
}

HWND STDCALL
NtUserSetFocus(HWND hWnd)
{
  return W32kSetFocusWindow(hWnd);
}

DWORD
STDCALL
NtUserGetKeyboardState(
  LPBYTE lpKeyState)
{
  if (lpKeyState) {
	if(!NT_SUCCESS(MmCopyToCaller(lpKeyState, QueueKeyStateTable, 256)))
		return FALSE;
  }
  return TRUE;
}

DWORD
STDCALL
NtUserSetKeyboardState(
  LPBYTE lpKeyState)
{
	if (lpKeyState) {
		if(! NT_SUCCESS(MmCopyFromCaller(QueueKeyStateTable, lpKeyState, 256)))
			return FALSE;
	}
    return TRUE;
}


/* EOF */
