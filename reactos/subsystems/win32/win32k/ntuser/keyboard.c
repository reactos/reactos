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
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Messages
 * FILE:             subsys/win32k/ntuser/keyboard.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>


/* Lock modifiers */
#define CAPITAL_BIT   0x80000000
#define NUMLOCK_BIT   0x40000000
#define MOD_BITS_MASK 0x3fffffff
#define MOD_KCTRL     0x02
/* Key States */
#define KS_DOWN_MASK     0xc0
#define KS_DOWN_BIT      0x80
#define KS_LOCK_BIT    0x01
/* lParam bits */
#define LP_EXT_BIT       (1<<24)
/* From kbdxx.c -- Key changes with numlock */
#define KNUMP         0x400


BYTE gQueueKeyStateTable[256];



/* FUNCTIONS *****************************************************************/

/* Initialization -- Right now, just zero the key state and init the lock */
NTSTATUS FASTCALL InitKeyboardImpl(VOID)
{
   RtlZeroMemory(&gQueueKeyStateTable,0x100);
   return STATUS_SUCCESS;
}

/*** Statics used by TranslateMessage ***/

/*** Shift state code was out of hand, sorry. --- arty */

static UINT DontDistinguishShifts( UINT ret )
{
   if( ret == VK_LSHIFT || ret == VK_RSHIFT )
      ret = VK_LSHIFT;
   if( ret == VK_LCONTROL || ret == VK_RCONTROL )
      ret = VK_LCONTROL;
   if( ret == VK_LMENU || ret == VK_RMENU )
      ret = VK_LMENU;
   return ret;
}

static VOID APIENTRY SetKeyState(DWORD key, DWORD vk, DWORD ext, BOOL down)
{
   ASSERT(vk <= 0xff);

   /* Special handling for toggles like numpad and caps lock */
   if (vk == VK_CAPITAL || vk == VK_NUMLOCK)
   {
      if (down)
         gQueueKeyStateTable[vk] ^= KS_LOCK_BIT;
   }

   if (ext && vk == VK_LSHIFT)
      vk = VK_RSHIFT;
   if (ext && vk == VK_LCONTROL)
      vk = VK_RCONTROL;
   if (ext && vk == VK_LMENU)
      vk = VK_RMENU;

   if (down)
      gQueueKeyStateTable[vk] |= KS_DOWN_BIT;
   else
      gQueueKeyStateTable[vk] &= ~KS_DOWN_MASK;

   if (vk == VK_LSHIFT || vk == VK_RSHIFT)
   {
      if ((gQueueKeyStateTable[VK_LSHIFT] & KS_DOWN_BIT) ||
            (gQueueKeyStateTable[VK_RSHIFT] & KS_DOWN_BIT))
      {
         gQueueKeyStateTable[VK_SHIFT] |= KS_DOWN_BIT;
      }
      else
      {
         gQueueKeyStateTable[VK_SHIFT] &= ~KS_DOWN_MASK;
      }
   }

   if (vk == VK_LCONTROL || vk == VK_RCONTROL)
   {
      if ((gQueueKeyStateTable[VK_LCONTROL] & KS_DOWN_BIT) ||
            (gQueueKeyStateTable[VK_RCONTROL] & KS_DOWN_BIT))
      {
         gQueueKeyStateTable[VK_CONTROL] |= KS_DOWN_BIT;
      }
      else
      {
         gQueueKeyStateTable[VK_CONTROL] &= ~KS_DOWN_MASK;
      }
   }

   if (vk == VK_LMENU || vk == VK_RMENU)
   {
      if ((gQueueKeyStateTable[VK_LMENU] & KS_DOWN_BIT) ||
            (gQueueKeyStateTable[VK_RMENU] & KS_DOWN_BIT))
      {
         gQueueKeyStateTable[VK_MENU] |= KS_DOWN_BIT;
      }
      else
      {
         gQueueKeyStateTable[VK_MENU] &= ~KS_DOWN_MASK;
      }
   }
}

VOID DumpKeyState( PBYTE KeyState )
{
   int i;

   DbgPrint( "KeyState { " );
   for( i = 0; i < 0x100; i++ )
   {
      if( KeyState[i] )
         DbgPrint( "%02x(%02x) ", i, KeyState[i] );
   }
   DbgPrint( "};\n" );
}

static BYTE KeysSet( PKBDTABLES pkKT, PBYTE KeyState,
                     int FakeModLeft, int FakeModRight )
{
   if( !KeyState || !pkKT )
      return 0;

   /* Search special codes first */
   if( FakeModLeft && KeyState[FakeModLeft] )
      return KeyState[FakeModLeft];
   else if( FakeModRight && KeyState[FakeModRight] )
      return KeyState[FakeModRight];

   return 0;
}

/* Search the keyboard layout modifiers table for the shift bit.  I don't
 * want to count on the shift bit not moving, because it can be specified
 * in the layout */

static DWORD FASTCALL GetShiftBit( PKBDTABLES pkKT, DWORD Vk )
{
   int i;

   for( i = 0; pkKT->pCharModifiers->pVkToBit[i].Vk; i++ )
      if( pkKT->pCharModifiers->pVkToBit[i].Vk == Vk )
         return pkKT->pCharModifiers->pVkToBit[i].ModBits;

   return 0;
}

static DWORD ModBits( PKBDTABLES pkKT, PBYTE KeyState )
{
   DWORD ModBits = 0;

   if( !KeyState )
      return 0;

   /* DumpKeyState( KeyState ); */

   if (KeysSet( pkKT, KeyState, VK_LSHIFT, VK_RSHIFT ) &
         KS_DOWN_BIT)
      ModBits |= GetShiftBit( pkKT, VK_SHIFT );

   if (KeysSet( pkKT, KeyState, VK_SHIFT, 0 ) &
         KS_DOWN_BIT)
      ModBits |= GetShiftBit( pkKT, VK_SHIFT );

   if (KeysSet( pkKT, KeyState, VK_LCONTROL, VK_RCONTROL ) &
         KS_DOWN_BIT )
      ModBits |= GetShiftBit( pkKT, VK_CONTROL );

   if (KeysSet( pkKT, KeyState, VK_CONTROL, 0 ) &
         KS_DOWN_BIT )
      ModBits |= GetShiftBit( pkKT, VK_CONTROL );

   if (KeysSet( pkKT, KeyState, VK_LMENU, VK_RMENU ) &
         KS_DOWN_BIT )
      ModBits |= GetShiftBit( pkKT, VK_MENU );

   /* Handle Alt+Gr */
   if (KeysSet( pkKT, KeyState, VK_RMENU, 0 ) &
         KS_DOWN_BIT )
      ModBits |= GetShiftBit( pkKT, VK_CONTROL );

   /* Deal with VK_CAPITAL */
   if (KeysSet( pkKT, KeyState, VK_CAPITAL, 0 ) & KS_LOCK_BIT)
   {
      ModBits |= CAPITAL_BIT;
   }

   /* Deal with VK_NUMLOCK */
   if (KeysSet( pkKT, KeyState, VK_NUMLOCK, 0 ) & KS_LOCK_BIT)
   {
      ModBits |= NUMLOCK_BIT;
   }

   DPRINT( "Current Mod Bits: %x\n", ModBits );

   return ModBits;
}

static BOOL TryToTranslateChar(WORD wVirtKey,
                               DWORD ModBits,
                               PBOOL pbDead,
                               PBOOL pbLigature,
                               PWCHAR pwcTranslatedChar,
                               PKBDTABLES keyLayout )
{
   PVK_TO_WCHAR_TABLE vtwTbl;
   PVK_TO_WCHARS10 vkPtr;
   size_t size_this_entry;
   int nMod;
   DWORD CapsMod = 0, CapsState = 0;

   CapsState = ModBits & ~MOD_BITS_MASK;
   ModBits = ModBits & MOD_BITS_MASK;

   DPRINT ( "TryToTranslate: %04x %x\n", wVirtKey, ModBits );

   if (ModBits > keyLayout->pCharModifiers->wMaxModBits)
   {
      return FALSE;
   }

   for (nMod = 0; keyLayout->pVkToWcharTable[nMod].nModifications; nMod++)
   {
      vtwTbl = &keyLayout->pVkToWcharTable[nMod];
      size_this_entry = vtwTbl->cbSize;
      vkPtr = (PVK_TO_WCHARS10)((BYTE *)vtwTbl->pVkToWchars);
      while(vkPtr->VirtualKey)
      {
         if( wVirtKey == (vkPtr->VirtualKey & 0xff) )
         {
            CapsMod = keyLayout->pCharModifiers->ModNumber
                      [ModBits ^
                       ((CapsState & CAPITAL_BIT) ? vkPtr->Attributes : 0)];

            if( CapsMod >= keyLayout->pVkToWcharTable[nMod].nModifications )
            {
               return FALSE;
            }

            if( vkPtr->wch[CapsMod] == WCH_NONE )
            {
               return FALSE;
            }

            *pbDead = vkPtr->wch[CapsMod] == WCH_DEAD;
            *pbLigature = vkPtr->wch[CapsMod] == WCH_LGTR;
            *pwcTranslatedChar = vkPtr->wch[CapsMod];

            DPRINT("%d %04x: CapsMod %08x CapsState %08x Char %04x\n",
                   nMod, wVirtKey,
                   CapsMod, CapsState, *pwcTranslatedChar);

            if( *pbDead )
            {
               vkPtr = (PVK_TO_WCHARS10)(((BYTE *)vkPtr) + size_this_entry);
               if( vkPtr->VirtualKey != 0xff )
               {
                  DPRINT( "Found dead key with no trailer in the table.\n" );
                  DPRINT( "VK: %04x, ADDR: %p\n", wVirtKey, vkPtr );
                  return FALSE;
               }
               *pwcTranslatedChar = vkPtr->wch[CapsMod];
            }
            return TRUE;
         }
         vkPtr = (PVK_TO_WCHARS10)(((BYTE *)vkPtr) + size_this_entry);
      }
   }
   return FALSE;
}

static
int APIENTRY
ToUnicodeInner(UINT wVirtKey,
               UINT wScanCode,
               PBYTE lpKeyState,
               LPWSTR pwszBuff,
               int cchBuff,
               UINT wFlags,
               PKBDTABLES pkKT)
{
   WCHAR wcTranslatedChar;
   BOOL bDead;
   BOOL bLigature;

   if( !pkKT )
      return 0;

   if( TryToTranslateChar( wVirtKey,
                           ModBits( pkKT, lpKeyState ),
                           &bDead,
                           &bLigature,
                           &wcTranslatedChar,
                           pkKT ) )
   {
      if( bLigature )
      {
         DPRINT("Not handling ligature (yet)\n" );
         return 0;
      }

      if( cchBuff > 0 )
         pwszBuff[0] = wcTranslatedChar;

      return bDead ? -1 : 1;
   }

   return 0;
}


DWORD FASTCALL UserGetKeyState(DWORD key)
{
   DWORD ret = 0;

   if( key < 0x100 )
   {
      ret = ((DWORD)(gQueueKeyStateTable[key] & KS_DOWN_BIT) << 8 ) |
            (gQueueKeyStateTable[key] & KS_LOCK_BIT);
   }

   return ret;
}


SHORT
APIENTRY
NtUserGetKeyState(
   INT key)
{
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetKeyState\n");
   UserEnterExclusive();

   RETURN(UserGetKeyState(key));

CLEANUP:
   DPRINT("Leave NtUserGetKeyState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



DWORD FASTCALL UserGetAsyncKeyState(DWORD key)
{
   DWORD ret = 0;

   if( key < 0x100 )
   {
      ret = ((DWORD)(gQueueKeyStateTable[key] & KS_DOWN_BIT) << 8 ) |
            (gQueueKeyStateTable[key] & KS_LOCK_BIT);
   }

   return ret;
}



SHORT
APIENTRY
NtUserGetAsyncKeyState(
   INT key)
{
   DECLARE_RETURN(SHORT);

   DPRINT("Enter NtUserGetAsyncKeyState\n");
   UserEnterExclusive();

   RETURN((SHORT)UserGetAsyncKeyState(key));

CLEANUP:
   DPRINT("Leave NtUserGetAsyncKeyState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



BOOL FASTCALL
IntTranslateKbdMessage(LPMSG lpMsg,
                       HKL dwhkl)
{
   PTHREADINFO pti;
   static INT dead_char = 0;
   LONG UState = 0;
   WCHAR wp[2] = { 0 };
   MSG NewMsg = { 0 };
   PKBDTABLES keyLayout;
   BOOL Result = FALSE;
   DWORD ScanCode = 0;

   pti = PsGetCurrentThreadWin32Thread();
   keyLayout = pti->KeyboardLayout->KBTables;
   if( !keyLayout )
      return FALSE;

   if (lpMsg->message != WM_KEYDOWN && lpMsg->message != WM_SYSKEYDOWN)
      return FALSE;

   ScanCode = (lpMsg->lParam >> 16) & 0xff;

   /* All messages have to contain the cursor point. */
   IntGetCursorLocation(pti->Desktop->WindowStation,
                        &NewMsg.pt);

   UState = ToUnicodeInner(lpMsg->wParam, HIWORD(lpMsg->lParam) & 0xff,
                           gQueueKeyStateTable, wp, 2, 0,
                           keyLayout );

   if (UState == 1)
   {
      NewMsg.message = (lpMsg->message == WM_KEYDOWN) ? WM_CHAR : WM_SYSCHAR;
      if (dead_char)
      {
         ULONG i;
         WCHAR first, second;
         DPRINT("PREVIOUS DEAD CHAR: %c\n", dead_char);

         for( i = 0; keyLayout->pDeadKey[i].dwBoth; i++ )
         {
            first = keyLayout->pDeadKey[i].dwBoth >> 16;
            second = keyLayout->pDeadKey[i].dwBoth;
            if (first == dead_char && second == wp[0])
            {
               wp[0] = keyLayout->pDeadKey[i].wchComposed;
               dead_char = 0;
               break;
            }
         }

         DPRINT("FINAL CHAR: %c\n", wp[0]);
      }

      if (dead_char)
      {
         NewMsg.hwnd = lpMsg->hwnd;
         NewMsg.wParam = dead_char;
         NewMsg.lParam = lpMsg->lParam;
         dead_char = 0;
         MsqPostMessage(pti->MessageQueue, &NewMsg, FALSE, QS_KEY);
      }

      NewMsg.hwnd = lpMsg->hwnd;
      NewMsg.wParam = wp[0];
      NewMsg.lParam = lpMsg->lParam;
      DPRINT( "CHAR='%c' %04x %08x\n", wp[0], wp[0], lpMsg->lParam );
      MsqPostMessage(pti->MessageQueue, &NewMsg, FALSE, QS_KEY);
      Result = TRUE;
   }
   else if (UState == -1)
   {
      NewMsg.message =
         (lpMsg->message == WM_KEYDOWN) ? WM_DEADCHAR : WM_SYSDEADCHAR;
      NewMsg.hwnd = lpMsg->hwnd;
      NewMsg.wParam = wp[0];
      NewMsg.lParam = lpMsg->lParam;
      dead_char = wp[0];
      MsqPostMessage(pti->MessageQueue, &NewMsg, FALSE, QS_KEY);
      Result = TRUE;
   }

   return Result;
}

DWORD
APIENTRY
NtUserGetKeyboardState(
   LPBYTE lpKeyState)
{
   BOOL Result = TRUE;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetKeyboardState\n");
   UserEnterShared();

   if (lpKeyState)
   {
      if(!NT_SUCCESS(MmCopyToCaller(lpKeyState, gQueueKeyStateTable, 256)))
         Result = FALSE;
   }

   RETURN(Result);

CLEANUP:
   DPRINT("Leave NtUserGetKeyboardState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL
APIENTRY
NtUserSetKeyboardState(LPBYTE lpKeyState)
{
   BOOL Result = TRUE;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserSetKeyboardState\n");
   UserEnterExclusive();

   if (lpKeyState)
   {
      if(! NT_SUCCESS(MmCopyFromCaller(gQueueKeyStateTable, lpKeyState, 256)))
         Result = FALSE;
   }

   RETURN(Result);

CLEANUP:
   DPRINT("Leave NtUserSetKeyboardState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

static UINT VkToScan( UINT Code, BOOL ExtCode, PKBDTABLES pkKT )
{
   int i;

   for( i = 0; i < pkKT->bMaxVSCtoVK; i++ )
   {
      if( pkKT->pusVSCtoVK[i] == Code )
      {
         return i;
      }
   }

   return 0;
}

UINT ScanToVk( UINT Code, BOOL ExtKey, PKBDTABLES pkKT )
{
   if( !pkKT )
   {
      DPRINT("ScanToVk: No layout\n");
      return 0;
   }

   if( ExtKey )
   {
      int i;

      for( i = 0; pkKT->pVSCtoVK_E0[i].Vsc; i++ )
      {
         if( pkKT->pVSCtoVK_E0[i].Vsc == Code )
            return pkKT->pVSCtoVK_E0[i].Vk & 0xff;
      }
      for( i = 0; pkKT->pVSCtoVK_E1[i].Vsc; i++ )
      {
         if( pkKT->pVSCtoVK_E1[i].Vsc == Code )
            return pkKT->pVSCtoVK_E1[i].Vk & 0xff;
      }

      return 0;
   }
   else
   {
      if( Code >= pkKT->bMaxVSCtoVK )
      {
         return 0;
      }
      return pkKT->pusVSCtoVK[Code] & 0xff;
   }
}

/*
 * Map a virtual key code, or virtual scan code, to a scan code, key code,
 * or unshifted unicode character.
 *
 * Code: See Below
 * Type:
 * 0 -- Code is a virtual key code that is converted into a virtual scan code
 *      that does not distinguish between left and right shift keys.
 * 1 -- Code is a virtual scan code that is converted into a virtual key code
 *      that does not distinguish between left and right shift keys.
 * 2 -- Code is a virtual key code that is converted into an unshifted unicode
 *      character.
 * 3 -- Code is a virtual scan code that is converted into a virtual key code
 *      that distinguishes left and right shift keys.
 * KeyLayout: Keyboard layout handle (currently, unused)
 *
 * @implemented
 */

static UINT IntMapVirtualKeyEx( UINT Code, UINT Type, PKBDTABLES keyLayout )
{
   UINT ret = 0;

   switch( Type )
   {
      case 0:
         if( Code == VK_RSHIFT )
            Code = VK_LSHIFT;
         if( Code == VK_RMENU )
            Code = VK_LMENU;
         if( Code == VK_RCONTROL )
            Code = VK_LCONTROL;
         ret = VkToScan( Code, FALSE, keyLayout );
         break;

      case 1:
         ret =
            DontDistinguishShifts
            (IntMapVirtualKeyEx( Code, 3, keyLayout ) );
         break;

      case 2:
         {
            WCHAR wp[2] = {0};

            ret = VkToScan( Code, FALSE, keyLayout );
            ToUnicodeInner( Code, ret, 0, wp, 2, 0, keyLayout );
            ret = wp[0];
         }
         break;

      case 3:

         ret = ScanToVk( Code, FALSE, keyLayout );
         break;
   }

   return ret;
}

UINT
APIENTRY
NtUserMapVirtualKeyEx( UINT Code, UINT Type, DWORD keyboardId, HKL dwhkl )
{
   PTHREADINFO pti;
   PKBDTABLES keyLayout;
   DECLARE_RETURN(UINT);

   DPRINT("Enter NtUserMapVirtualKeyEx\n");
   UserEnterExclusive();

   pti = PsGetCurrentThreadWin32Thread();
   keyLayout = pti ? pti->KeyboardLayout->KBTables : 0;

   if( !keyLayout )
      RETURN(0);

   RETURN(IntMapVirtualKeyEx( Code, Type, keyLayout ));

CLEANUP:
   DPRINT("Leave NtUserMapVirtualKeyEx, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


int
APIENTRY
NtUserToUnicodeEx(
   UINT wVirtKey,
   UINT wScanCode,
   PBYTE lpKeyState,
   LPWSTR pwszBuff,
   int cchBuff,
   UINT wFlags,
   HKL dwhkl )
{
   PTHREADINFO pti;
   BYTE KeyStateBuf[0x100];
   PWCHAR OutPwszBuff = 0;
   int ret = 0;
   DECLARE_RETURN(int);

   DPRINT("Enter NtUserSetKeyboardState\n");
   UserEnterShared();//faxme: this syscall doesnt seem to need any locking...


   if( !NT_SUCCESS(MmCopyFromCaller(KeyStateBuf,
                                    lpKeyState,
                                    sizeof(KeyStateBuf))) )
   {
      DPRINT1( "Couldn't copy key state from caller.\n" );
      RETURN(0);
   }
   
   /* Virtual code is correct and key is pressed currently? */
   if (wVirtKey < 0x100 && KeyStateBuf[wVirtKey] & KS_DOWN_BIT)
   {
      OutPwszBuff = ExAllocatePoolWithTag(NonPagedPool,sizeof(WCHAR) * cchBuff, TAG_STRING);
      if( !OutPwszBuff )
      {
         DPRINT1( "ExAllocatePool(%d) failed\n", sizeof(WCHAR) * cchBuff);
         RETURN(0);
      }
      RtlZeroMemory( OutPwszBuff, sizeof( WCHAR ) * cchBuff );

      pti = PsGetCurrentThreadWin32Thread();
      ret = ToUnicodeInner( wVirtKey,
                            wScanCode,
                            KeyStateBuf,
                            OutPwszBuff,
                            cchBuff,
                            wFlags,
                            pti ? pti->KeyboardLayout->KBTables : 0 );

      MmCopyToCaller(pwszBuff,OutPwszBuff,sizeof(WCHAR)*cchBuff);
      ExFreePoolWithTag(OutPwszBuff, TAG_STRING);
   }
   else
      ret = 0;

   RETURN(ret);

CLEANUP:
   DPRINT("Leave NtUserSetKeyboardState, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

static int W32kSimpleToupper( int ch )
{
   if( ch >= 'a' && ch <= 'z' )
      ch = ch - 'a' + 'A';
   return ch;
}

DWORD
APIENTRY
NtUserGetKeyNameText( LONG lParam, LPWSTR lpString, int nSize )
{
   PTHREADINFO pti;
   int i;
   DWORD ret = 0;
   UINT CareVk = 0;
   UINT VkCode = 0;
   UINT ScanCode = (lParam >> 16) & 0xff;
   BOOL ExtKey = lParam & (1<<24) ? TRUE : FALSE;
   PKBDTABLES keyLayout;
   VSC_LPWSTR *KeyNames;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetKeyNameText\n");
   UserEnterShared();

   pti = PsGetCurrentThreadWin32Thread();
   keyLayout = pti ? pti->KeyboardLayout->KBTables : 0;

   if( !keyLayout || nSize < 1 )
      RETURN(0);

   if( lParam & (1<<25) )
   {
      CareVk = VkCode = ScanToVk( ScanCode, ExtKey, keyLayout );
      if( VkCode == VK_LSHIFT || VkCode == VK_RSHIFT )
         VkCode = VK_LSHIFT;
      if( VkCode == VK_LCONTROL || VkCode == VK_RCONTROL )
         VkCode = VK_LCONTROL;
      if( VkCode == VK_LMENU || VkCode == VK_RMENU )
         VkCode = VK_LMENU;
   }
   else
   {
      VkCode = ScanToVk( ScanCode, ExtKey, keyLayout );
   }

   KeyNames = 0;

   if( CareVk != VkCode )
      ScanCode = VkToScan( VkCode, ExtKey, keyLayout );

   if( ExtKey )
      KeyNames = keyLayout->pKeyNamesExt;
   else
      KeyNames = keyLayout->pKeyNames;

   for( i = 0; KeyNames[i].pwsz; i++ )
   {
      if( KeyNames[i].vsc == ScanCode )
      {
         UINT StrLen = wcslen(KeyNames[i].pwsz);
         UINT StrMax = StrLen > (nSize - 1) ? (nSize - 1) : StrLen;
         WCHAR null_wc = 0;
         if( NT_SUCCESS( MmCopyToCaller( lpString,
                                         KeyNames[i].pwsz,
                                         StrMax * sizeof(WCHAR) ) ) &&
               NT_SUCCESS( MmCopyToCaller( lpString + StrMax,
                                           &null_wc,
                                           sizeof( WCHAR ) ) ) )
         {
            ret = StrMax;
            break;
         }
      }
   }

   if( ret == 0 )
   {
      WCHAR UCName[2];

      UCName[0] = W32kSimpleToupper(IntMapVirtualKeyEx( VkCode, 2, keyLayout ));
      UCName[1] = 0;
      ret = 1;

      if( !NT_SUCCESS(MmCopyToCaller( lpString, UCName, 2 * sizeof(WCHAR) )) )
         RETURN(0);
   }

   RETURN(ret);

CLEANUP:
   DPRINT("Leave NtUserGetKeyNameText, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * Filter this message according to the current key layout, setting wParam
 * appropriately.
 */

VOID FASTCALL
W32kKeyProcessMessage(LPMSG Msg,
                      PKBDTABLES KeyboardLayout,
                      BYTE Prefix)
{
   DWORD ScanCode = 0, ModifierBits = 0;
   DWORD i = 0;
   DWORD BaseMapping = 0;
   DWORD RawVk = 0;
   static WORD NumpadConversion[][2] =
      { { VK_DELETE, VK_DECIMAL },
        { VK_INSERT, VK_NUMPAD0 },
        { VK_END,    VK_NUMPAD1 },
        { VK_DOWN,   VK_NUMPAD2 },
        { VK_NEXT,   VK_NUMPAD3 },
        { VK_LEFT,   VK_NUMPAD4 },
        { VK_CLEAR,  VK_NUMPAD5 },
        { VK_RIGHT,  VK_NUMPAD6 },
        { VK_HOME,   VK_NUMPAD7 },
        { VK_UP,     VK_NUMPAD8 },
        { VK_PRIOR,  VK_NUMPAD9 },
        { 0,0 } };
   PVSC_VK VscVkTable = NULL;

   if( !KeyboardLayout || !Msg ||
         (Msg->message != WM_KEYDOWN && Msg->message != WM_SYSKEYDOWN &&
          Msg->message != WM_KEYUP   && Msg->message != WM_SYSKEYUP) )
   {
      return;
   }

   /* arty -- handle numpad -- On real windows, the actual key produced
    * by the messaging layer is different based on the state of numlock. */
   ModifierBits = ModBits(KeyboardLayout,gQueueKeyStateTable);

   /* Get the raw scan code, so we can look up whether the key is a numpad
    * key
    *
    * Shift and the LP_EXT_BIT cancel. */
   ScanCode = (Msg->lParam >> 16) & 0xff;
   BaseMapping = Msg->wParam =
                    IntMapVirtualKeyEx( ScanCode, 1, KeyboardLayout );
   if( Prefix == 0 )
   {
      if( ScanCode >= KeyboardLayout->bMaxVSCtoVK )
         RawVk = 0xff;
      else
         RawVk = KeyboardLayout->pusVSCtoVK[ScanCode];
   }
   else
   {
      if( Prefix == 0xE0 )
      {
         /* ignore shift codes */
         if( ScanCode == 0x2A || ScanCode == 0x36 )
         {
            return;
         }
         VscVkTable = KeyboardLayout->pVSCtoVK_E0;
      }
      else if( Prefix == 0xE1 )
      {
         VscVkTable = KeyboardLayout->pVSCtoVK_E1;
      }

      RawVk = 0xff;
      while (VscVkTable->Vsc)
      {
         if( VscVkTable->Vsc == ScanCode )
         {
            RawVk = VscVkTable->Vk;
         }
         VscVkTable++;
      }
   }

   if ((ModifierBits & NUMLOCK_BIT) &&
         !(ModifierBits & GetShiftBit(KeyboardLayout, VK_SHIFT)) &&
         (RawVk & KNUMP) &&
         !(Msg->lParam & LP_EXT_BIT))
   {
      /* The key in question is a numpad key.  Search for a translation. */
      for (i = 0; NumpadConversion[i][0]; i++)
      {
         if ((BaseMapping & 0xff) == NumpadConversion[i][0]) /* RawVk? */
         {
            Msg->wParam = NumpadConversion[i][1];
            break;
         }
      }
   }

   DPRINT("Key: [%04x -> %04x]\n", BaseMapping, Msg->wParam);

   /* Now that we have the VK, we can set the keymap appropriately
    * This is a better place for this code, as it's guaranteed to be
    * run, unlike translate message. */
   if (Msg->message == WM_KEYDOWN || Msg->message == WM_SYSKEYDOWN)
   {
      SetKeyState( ScanCode, Msg->wParam, Msg->lParam & LP_EXT_BIT,
                   TRUE ); /* Strike key */
   }
   else if (Msg->message == WM_KEYUP || Msg->message == WM_SYSKEYUP)
   {
      SetKeyState( ScanCode, Msg->wParam, Msg->lParam & LP_EXT_BIT,
                   FALSE ); /* Release key */
   }

   /* We need to unset SYSKEYDOWN if the ALT key is an ALT+Gr */
   if( gQueueKeyStateTable[VK_RMENU] & KS_DOWN_BIT )
   {
      if( Msg->message == WM_SYSKEYDOWN )
         Msg->message = WM_KEYDOWN;
      else
         Msg->message = WM_KEYUP;
   }

}



DWORD FASTCALL
UserGetKeyboardType(
   DWORD TypeFlag)
{
   switch(TypeFlag)
   {
      case 0:        /* Keyboard type */
         return 4;    /* AT-101 */
      case 1:        /* Keyboard Subtype */
         return 0;    /* There are no defined subtypes */
      case 2:        /* Number of F-keys */
         return 12;   /* We're doing an 101 for now, so return 12 F-keys */
      default:
         DPRINT1("Unknown type!\n");
         return 0;    /* The book says 0 here, so 0 */
   }
}


/*
    Based on TryToTranslateChar, instead of processing VirtualKey match,
    look for wChar match.
 */
DWORD
APIENTRY
NtUserVkKeyScanEx(
   WCHAR wChar,
   HKL KeyboardLayout,
   DWORD Unknown2)
{
/* FIXME: currently, this routine doesnt seem to need any locking */
   PKBDTABLES KeyLayout;
   PVK_TO_WCHAR_TABLE vtwTbl;
   PVK_TO_WCHARS10 vkPtr;
   size_t size_this_entry;
   int nMod;
   DWORD CapsMod = 0, CapsState = 0;

   DPRINT("NtUserVkKeyScanEx() wChar %d, KbdLayout 0x%p\n", wChar, KeyboardLayout);

   if(!KeyboardLayout)
      return -1;
   KeyLayout = UserHklToKbl(KeyboardLayout)->KBTables;

   for (nMod = 0; KeyLayout->pVkToWcharTable[nMod].nModifications; nMod++)
   {
      vtwTbl = &KeyLayout->pVkToWcharTable[nMod];
      size_this_entry = vtwTbl->cbSize;
      vkPtr = (PVK_TO_WCHARS10)((BYTE *)vtwTbl->pVkToWchars);

      while(vkPtr->VirtualKey)
      {
         /*
            0x01 Shift key
            0x02 Ctrl key
            0x04 Alt key
            Should have only 8 valid possibilities. Including zero.
          */
         for(CapsState = 0; CapsState < vtwTbl->nModifications; CapsState++)
         {
            if(vkPtr->wch[CapsState] == wChar)
            {
               CapsMod = KeyLayout->pCharModifiers->ModNumber[CapsState];
               DPRINT("nMod %d wC %04x: CapsMod %08x CapsState %08x MaxModBits %08x\n",
                      nMod, wChar, CapsMod, CapsState, KeyLayout->pCharModifiers->wMaxModBits);
               return ((CapsMod << 8)|(vkPtr->VirtualKey & 0xff));
            }
         }
         vkPtr = (PVK_TO_WCHARS10)(((BYTE *)vkPtr) + size_this_entry);
      }
   }
   return -1;
}


/* EOF */
