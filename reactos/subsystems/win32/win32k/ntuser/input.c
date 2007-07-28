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
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>
#include <ntddkbd.h>

#define NDEBUG
#include <debug.h>

extern BYTE gQueueKeyStateTable[];

/* GLOBALS *******************************************************************/


static HANDLE MouseDeviceHandle;
static HANDLE MouseThreadHandle;
static CLIENT_ID MouseThreadId;
static HANDLE KeyboardThreadHandle;
static CLIENT_ID KeyboardThreadId;
static HANDLE KeyboardDeviceHandle;
static KEVENT InputThreadsStart;
static BOOLEAN InputThreadsRunning = FALSE;

/* FUNCTIONS *****************************************************************/
ULONG FASTCALL
IntSystemParametersInfo(UINT uiAction, UINT uiParam,PVOID pvParam, UINT fWinIni);
DWORD IntLastInputTick(BOOL LastInputTickSetGet);

#define ClearMouseInput(mi) \
  mi.dx = 0; \
  mi.dy = 0; \
  mi.mouseData = 0; \
  mi.dwFlags = 0;

#define SendMouseEvent(mi) \
  if(mi.dx != 0 || mi.dy != 0) \
    mi.dwFlags |= MOUSEEVENTF_MOVE; \
  if(mi.dwFlags) \
    IntMouseInput(&mi); \
  ClearMouseInput(mi);


DWORD IntLastInputTick(BOOL LastInputTickSetGet)
{
	static DWORD LastInputTick = 0;
	if (LastInputTickSetGet == TRUE)
	{
		LARGE_INTEGER TickCount;
        KeQueryTickCount(&TickCount);
        LastInputTick = TickCount.u.LowPart * (KeQueryTimeIncrement() / 10000);
	}
    return LastInputTick;
}

BOOL
STDCALL
NtUserGetLastInputInfo(PLASTINPUTINFO plii)
{
    BOOL ret = TRUE;
    
    UserEnterShared();
    
    _SEH_TRY
    {
        if (ProbeForReadUint(&plii->cbSize) != sizeof(LASTINPUTINFO))
        {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            ret = FALSE;
            _SEH_LEAVE; 
        }

        ProbeForWrite(plii, sizeof(LASTINPUTINFO), sizeof(DWORD));
        
        plii->dwTime = IntLastInputTick(FALSE);
    }
    _SEH_HANDLE
    {
        SetLastNtError(_SEH_GetExceptionCode());
        ret = FALSE;
    }
    _SEH_END;
   
    UserLeave(); 
    
    return ret;
}


VOID FASTCALL
ProcessMouseInputData(PMOUSE_INPUT_DATA Data, ULONG InputCount)
{
   PMOUSE_INPUT_DATA mid;
   MOUSEINPUT mi;
   ULONG i;

   ClearMouseInput(mi);
   mi.time = 0;
   mi.dwExtraInfo = 0;
   for(i = 0; i < InputCount; i++)
   {
      mid = (Data + i);
      mi.dx += mid->LastX;
      mi.dy += mid->LastY;

      if(mid->ButtonFlags)
      {
         if(mid->ButtonFlags & MOUSE_LEFT_BUTTON_DOWN)
         {
            mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
            SendMouseEvent(mi);
         }
         if(mid->ButtonFlags & MOUSE_LEFT_BUTTON_UP)
         {
            mi.dwFlags |= MOUSEEVENTF_LEFTUP;
            SendMouseEvent(mi);
         }
         if(mid->ButtonFlags & MOUSE_MIDDLE_BUTTON_DOWN)
         {
            mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
            SendMouseEvent(mi);
         }
         if(mid->ButtonFlags & MOUSE_MIDDLE_BUTTON_UP)
         {
            mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
            SendMouseEvent(mi);
         }
         if(mid->ButtonFlags & MOUSE_RIGHT_BUTTON_DOWN)
         {
            mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
            SendMouseEvent(mi);
         }
         if(mid->ButtonFlags & MOUSE_RIGHT_BUTTON_UP)
         {
            mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
            SendMouseEvent(mi);
         }
         if(mid->ButtonFlags & MOUSE_BUTTON_4_DOWN)
         {
            mi.mouseData |= XBUTTON1;
            mi.dwFlags |= MOUSEEVENTF_XDOWN;
            SendMouseEvent(mi);
         }
         if(mid->ButtonFlags & MOUSE_BUTTON_4_UP)
         {
            mi.mouseData |= XBUTTON1;
            mi.dwFlags |= MOUSEEVENTF_XUP;
            SendMouseEvent(mi);
         }
         if(mid->ButtonFlags & MOUSE_BUTTON_5_DOWN)
         {
            mi.mouseData |= XBUTTON2;
            mi.dwFlags |= MOUSEEVENTF_XDOWN;
            SendMouseEvent(mi);
         }
         if(mid->ButtonFlags & MOUSE_BUTTON_5_UP)
         {
            mi.mouseData |= XBUTTON2;
            mi.dwFlags |= MOUSEEVENTF_XUP;
            SendMouseEvent(mi);
         }
         if(mid->ButtonFlags & MOUSE_WHEEL)
         {
            mi.mouseData = mid->ButtonData;
            mi.dwFlags |= MOUSEEVENTF_WHEEL;
            SendMouseEvent(mi);
         }
      }
   }

   SendMouseEvent(mi);
}




VOID STDCALL
MouseThreadMain(PVOID StartContext)
{
   UNICODE_STRING MouseDeviceName = RTL_CONSTANT_STRING(L"\\Device\\PointerClass0");
   OBJECT_ATTRIBUTES MouseObjectAttributes;
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;

   InitializeObjectAttributes(&MouseObjectAttributes,
                              &MouseDeviceName,
                              0,
                              NULL,
                              NULL);
   do
   {
      LARGE_INTEGER DueTime;
      KEVENT Event;
      DueTime.QuadPart = (LONGLONG)(-10000000);
      KeInitializeEvent(&Event, NotificationEvent, FALSE);
      Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, &DueTime);
      Status = NtOpenFile(&MouseDeviceHandle,
                       FILE_ALL_ACCESS,
                       &MouseObjectAttributes,
                       &Iosb,
                       0,
                       FILE_SYNCHRONOUS_IO_ALERT);
   } while (!NT_SUCCESS(Status));

   KeSetPriorityThread(&PsGetCurrentThread()->Tcb,
                       LOW_REALTIME_PRIORITY + 3);

   for(;;)
   {
      /*
       * Wait to start input.
       */
      DPRINT("Mouse Input Thread Waiting for start event\n");
      Status = KeWaitForSingleObject(&InputThreadsStart,
                                     0,
                                     KernelMode,
                                     TRUE,
                                     NULL);
      DPRINT("Mouse Input Thread Starting...\n");

      /*
       * Receive and process mouse input.
       */
      while(InputThreadsRunning)
      {
         MOUSE_INPUT_DATA MouseInput;
         Status = NtReadFile(MouseDeviceHandle,
                             NULL,
                             NULL,
                             NULL,
                             &Iosb,
                             &MouseInput,
                             sizeof(MOUSE_INPUT_DATA),
                             NULL,
                             NULL);
         if(Status == STATUS_ALERTED && !InputThreadsRunning)
         {
            break;
         }
         if(Status == STATUS_PENDING)
         {
            NtWaitForSingleObject(MouseDeviceHandle, FALSE, NULL);
            Status = Iosb.Status;
         }
         if(!NT_SUCCESS(Status))
         {
            DPRINT1("Win32K: Failed to read from mouse.\n");
            return; //(Status);
         }
         DPRINT("MouseEvent\n");         
		 IntLastInputTick(TRUE);

         UserEnterExclusive();

         ProcessMouseInputData(&MouseInput, Iosb.Information / sizeof(MOUSE_INPUT_DATA));

         UserLeave();
      }
      DPRINT("Mouse Input Thread Stopped...\n");
   }
}

/* Returns a value that indicates if the key is a modifier key, and
 * which one.
 */
static UINT STDCALL
IntKeyboardGetModifiers(KEYBOARD_INPUT_DATA *InputData)
{
   if (InputData->Flags & KEY_E1)
      return 0;

   if (!(InputData->Flags & KEY_E0))
   {
      switch (InputData->MakeCode)
      {
         case 0x2a: /* left shift */
         case 0x36: /* right shift */
            return MOD_SHIFT;

         case 0x1d: /* left control */
            return MOD_CONTROL;

         case 0x38: /* left alt */
            return MOD_ALT;

         default:
            return 0;
      }
   }
   else
   {
      switch (InputData->MakeCode)
      {
         case 0x1d: /* right control */
            return MOD_CONTROL;

         case 0x38: /* right alt */
            return MOD_ALT;

         case 0x5b: /* left gui (windows) */
         case 0x5c: /* right gui (windows) */
            return MOD_WIN;

         default:
            return 0;
      }
   }
}

/* Asks the keyboard driver to send a small table that shows which
 * lights should connect with which scancodes
 */
static NTSTATUS STDCALL
IntKeyboardGetIndicatorTrans(HANDLE KeyboardDeviceHandle,
                             PKEYBOARD_INDICATOR_TRANSLATION *IndicatorTrans)
{
   NTSTATUS Status;
   DWORD Size = 0;
   IO_STATUS_BLOCK Block;
   PKEYBOARD_INDICATOR_TRANSLATION Ret;

   Size = sizeof(KEYBOARD_INDICATOR_TRANSLATION);

   Ret = ExAllocatePoolWithTag(PagedPool,
                               Size,
                               TAG_KEYBOARD);

   while (Ret)
   {
      Status = NtDeviceIoControlFile(KeyboardDeviceHandle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &Block,
                                     IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION,
                                     NULL, 0,
                                     Ret, Size);

      if (Status != STATUS_BUFFER_TOO_SMALL)
         break;

      ExFreePool(Ret);

      Size += sizeof(KEYBOARD_INDICATOR_TRANSLATION);

      Ret = ExAllocatePoolWithTag(PagedPool,
                                  Size,
                                  TAG_KEYBOARD);
   }

   if (!Ret)
      return STATUS_INSUFFICIENT_RESOURCES;

   if (Status != STATUS_SUCCESS)
   {
      ExFreePool(Ret);
      return Status;
   }

   *IndicatorTrans = Ret;
   return Status;
}

/* Sends the keyboard commands to turn on/off the lights.
 */
static NTSTATUS STDCALL
IntKeyboardUpdateLeds(HANDLE KeyboardDeviceHandle,
                      PKEYBOARD_INPUT_DATA KeyInput,
                      PKEYBOARD_INDICATOR_TRANSLATION IndicatorTrans)
{
   NTSTATUS Status;
   UINT Count;
   static KEYBOARD_INDICATOR_PARAMETERS Indicators;
   IO_STATUS_BLOCK Block;

   if (!IndicatorTrans)
      return STATUS_NOT_SUPPORTED;

   if (KeyInput->Flags & (KEY_E0 | KEY_E1 | KEY_BREAK))
      return STATUS_SUCCESS;

   for (Count = 0; Count < IndicatorTrans->NumberOfIndicatorKeys; Count++)
   {
      if (KeyInput->MakeCode == IndicatorTrans->IndicatorList[Count].MakeCode)
      {
         Indicators.LedFlags ^=
            IndicatorTrans->IndicatorList[Count].IndicatorFlags;

         /* Update the lights on the hardware */

         Status = NtDeviceIoControlFile(KeyboardDeviceHandle,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &Block,
                                        IOCTL_KEYBOARD_SET_INDICATORS,
                                        &Indicators, sizeof(Indicators),
                                        NULL, 0);

         return Status;
      }
   }

   return STATUS_SUCCESS;
}

static VOID STDCALL
IntKeyboardSendWinKeyMsg()
{
   PWINDOW_OBJECT Window;
   MSG Mesg;

   if (!(Window = UserGetWindowObject(InputWindowStation->ShellWindow)))
   {
      DPRINT1("Couldn't find window to send Windows key message!\n");
      return;
   }

   Mesg.hwnd = InputWindowStation->ShellWindow;
   Mesg.message = WM_SYSCOMMAND;
   Mesg.wParam = SC_TASKLIST;
   Mesg.lParam = 0;

   /* The QS_HOTKEY is just a guess */
   MsqPostMessage(Window->MessageQueue, &Mesg, FALSE, QS_HOTKEY);
}

static VOID STDCALL
co_IntKeyboardSendAltKeyMsg()
{
   co_MsqPostKeyboardMessage(WM_SYSCOMMAND,SC_KEYMENU,0);
}

static VOID STDCALL
KeyboardThreadMain(PVOID StartContext)
{
   UNICODE_STRING KeyboardDeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
   OBJECT_ATTRIBUTES KeyboardObjectAttributes;
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;
   MSG msg;
   PUSER_MESSAGE_QUEUE FocusQueue;
   struct _ETHREAD *FocusThread;
   extern NTSTATUS Win32kInitWin32Thread(PETHREAD Thread);

   PKEYBOARD_INDICATOR_TRANSLATION IndicatorTrans = NULL;
   UINT ModifierState = 0;
   USHORT LastMakeCode = 0;
   USHORT LastFlags = 0;
   UINT RepeatCount = 0;

   InitializeObjectAttributes(&KeyboardObjectAttributes,
                              &KeyboardDeviceName,
                              0,
                              NULL,
                              NULL);
   do
   {
      LARGE_INTEGER DueTime;
      KEVENT Event;
      DueTime.QuadPart = (LONGLONG)(-10000000);
      KeInitializeEvent(&Event, NotificationEvent, FALSE);
      Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, &DueTime);
      Status = NtOpenFile(&KeyboardDeviceHandle,
                       FILE_ALL_ACCESS,
                       &KeyboardObjectAttributes,
                       &Iosb,
                       0,
                       FILE_SYNCHRONOUS_IO_ALERT);
   } while (!NT_SUCCESS(Status));

   /* Not sure if converting this thread to a win32 thread is such
      a great idea. Since we're posting keyboard messages to the focus
      window message queue, we'll be (indirectly) doing sendmessage
      stuff from this thread (for WH_KEYBOARD_LL processing), which
      means we need our own message queue. If keyboard messages were
      instead queued to the system message queue, the thread removing
      the message from the system message queue would be responsible
      for WH_KEYBOARD_LL processing and we wouldn't need this thread
      to be a win32 thread. */
   Status = Win32kInitWin32Thread(PsGetCurrentThread());
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Win32K: Failed making keyboard thread a win32 thread.\n");
      return; //(Status);
   }

   KeSetPriorityThread(&PsGetCurrentThread()->Tcb,
                       LOW_REALTIME_PRIORITY + 3);

   IntKeyboardGetIndicatorTrans(KeyboardDeviceHandle,
                                &IndicatorTrans);

   for (;;)
   {
      /*
       * Wait to start input.
       */
      DPRINT( "Keyboard Input Thread Waiting for start event\n" );
      Status = KeWaitForSingleObject(&InputThreadsStart,
                                     0,
                                     KernelMode,
                                     TRUE,
                                     NULL);

      DPRINT( "Keyboard Input Thread Starting...\n" );
      /*
       * Receive and process keyboard input.
       */
      while (InputThreadsRunning)
      {
         BOOLEAN NumKeys = 1;
         KEYBOARD_INPUT_DATA KeyInput;
         KEYBOARD_INPUT_DATA NextKeyInput;
         LPARAM lParam = 0;
         UINT fsModifiers, fsNextModifiers;
         struct _ETHREAD *Thread;
         HWND hWnd;
         int id;

         DPRINT("KeyInput @ %08x\n", &KeyInput);

         Status = NtReadFile (KeyboardDeviceHandle,
                              NULL,
                              NULL,
                              NULL,
                              &Iosb,
                              &KeyInput,
                              sizeof(KEYBOARD_INPUT_DATA),
                              NULL,
                              NULL);

         if(Status == STATUS_ALERTED && !InputThreadsRunning)
         {
            break;
         }
         if(Status == STATUS_PENDING)
         {
            NtWaitForSingleObject(KeyboardDeviceHandle, FALSE, NULL);
            Status = Iosb.Status;
         }
         if(!NT_SUCCESS(Status))
         {
            DPRINT1("Win32K: Failed to read from mouse.\n");
            return; //(Status);
         }

         DPRINT("KeyRaw: %s %04x\n",
                (KeyInput.Flags & KEY_BREAK) ? "up" : "down",
                KeyInput.MakeCode );

         if (Status == STATUS_ALERTED && !InputThreadsRunning)
            break;

         if (!NT_SUCCESS(Status))
         {
            DPRINT1("Win32K: Failed to read from keyboard.\n");
            return; //(Status);
         }

		 /* Set LastInputTick */
		 IntLastInputTick(TRUE);

         /* Update modifier state */
         fsModifiers = IntKeyboardGetModifiers(&KeyInput);

         if (fsModifiers)
         {
            if (KeyInput.Flags & KEY_BREAK)
            {
               ModifierState &= ~fsModifiers;
            }
            else
            {
               ModifierState |= fsModifiers;

               if (ModifierState == fsModifiers &&
                     (fsModifiers == MOD_ALT || fsModifiers == MOD_WIN))
               {
                  /* First send out special notifications
                   * (For alt, the message that turns on accelerator
                   * display, not sure what for win. Both TODO though.)
                   */

                  /* Read the next key before sending this one */
                  do
                  {
                     Status = NtReadFile (KeyboardDeviceHandle,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &Iosb,
                                          &NextKeyInput,
                                          sizeof(KEYBOARD_INPUT_DATA),
                                          NULL,
                                          NULL);
                     DPRINT("KeyRaw: %s %04x\n",
                            (NextKeyInput.Flags & KEY_BREAK) ? "up":"down",
                            NextKeyInput.MakeCode );

                     if (Status == STATUS_ALERTED && !InputThreadsRunning)
                        goto KeyboardEscape;

                  }
                  while ((!(NextKeyInput.Flags & KEY_BREAK)) &&
                         NextKeyInput.MakeCode == KeyInput.MakeCode);
                  /* ^ Ignore repeats, they'll be KEY_MAKE and the same
                   *   code. I'm not caring about the counting, not sure
                   *   if that matters. I think not.
                   */

                  /* If the ModifierState is now empty again, send a
                   * special notification and eat both keypresses
                   */

                  fsNextModifiers = IntKeyboardGetModifiers(&NextKeyInput);

                  if (fsNextModifiers)
                     ModifierState ^= fsNextModifiers;

                  if (ModifierState == 0)
                  {
                     if (fsModifiers == MOD_WIN)
                        IntKeyboardSendWinKeyMsg();
                     else if (fsModifiers == MOD_ALT)
                        co_IntKeyboardSendAltKeyMsg();
                     continue;
                  }

                  NumKeys = 2;
               }
            }
         }

         for (;NumKeys;memcpy(&KeyInput, &NextKeyInput, sizeof(KeyInput)),
               NumKeys--)
         {
            lParam = 0;

            IntKeyboardUpdateLeds(KeyboardDeviceHandle,
                                  &KeyInput,
                                  IndicatorTrans);

            /* While we are working, we set up lParam. The format is:
             *  0-15: The number of times this key has autorepeated
             * 16-23: The keyboard scancode
             *    24: Set if it's and extended key (I assume KEY_E0 | KEY_E1)
             *        Note that E1 is only used for PAUSE (E1-1D-45) and
             *        E0-45 happens not to be anything.
             *    29: Alt is pressed ('Context code')
             *    30: Previous state, if the key was down before this message
             *        This is a cheap way to ignore autorepeat keys
             *    31: 1 if the key is being pressed
             */

            /* If it's a KEY_MAKE (which is 0, so test using !KEY_BREAK)
             * and it's the same key as the last one, increase the repeat
             * count.
             */

            if (!(KeyInput.Flags & KEY_BREAK))
            {
               if (((KeyInput.Flags & (KEY_E0 | KEY_E1)) == LastFlags) &&
                     (KeyInput.MakeCode == LastMakeCode))
               {
                  RepeatCount++;
                  lParam |= (1 << 30);
               }
               else
               {
                  RepeatCount = 0;
                  LastFlags = KeyInput.Flags & (KEY_E0 | KEY_E1);
                  LastMakeCode = KeyInput.MakeCode;
               }
            }
            else
            {
               LastFlags = 0;
               LastMakeCode = 0; /* Should never match */
               lParam |= (1 << 30) | (1 << 31);
            }

            lParam |= RepeatCount;

            lParam |= (KeyInput.MakeCode & 0xff) << 16;

            if (KeyInput.Flags & KEY_E0)
               lParam |= (1 << 24);

            if (ModifierState & MOD_ALT)
            {
               lParam |= (1 << 29);

               if (!(KeyInput.Flags & KEY_BREAK))
                  msg.message = WM_SYSKEYDOWN;
               else
                  msg.message = WM_SYSKEYUP;
            }
            else
            {
               if (!(KeyInput.Flags & KEY_BREAK))
                  msg.message = WM_KEYDOWN;
               else
                  msg.message = WM_KEYUP;
            }

            /* Find the target thread whose locale is in effect */
               FocusQueue = IntGetFocusMessageQueue();

            /* This might cause us to lose hot keys, which are important
             * (ctrl-alt-del secure attention sequence). Not sure if it
             * can happen though.
             */
            if (!FocusQueue)
               continue;

            msg.lParam = lParam;
            msg.hwnd = FocusQueue->FocusWindow;

            FocusThread = FocusQueue->Thread;

            if (!(FocusThread && FocusThread->Tcb.Win32Thread &&
                  ((PW32THREAD)FocusThread->Tcb.Win32Thread)->KeyboardLayout))
               continue;

            /* This function uses lParam to fill wParam according to the
             * keyboard layout in use.
             */
            W32kKeyProcessMessage(&msg,
                                  ((PW32THREAD)FocusThread->Tcb.Win32Thread)->KeyboardLayout->KBTables,
                                  KeyInput.Flags & KEY_E0 ? 0xE0 :
                                  (KeyInput.Flags & KEY_E1 ? 0xE1 : 0));

            if (GetHotKey(ModifierState,
                          msg.wParam,
                          &Thread,
                          &hWnd,
                          &id))
            {
               if (!(KeyInput.Flags & KEY_BREAK))
               {
                  DPRINT("Hot key pressed (hWnd %lx, id %d)\n", hWnd, id);
                  MsqPostHotKeyMessage (Thread,
                                        hWnd,
                                        (WPARAM)id,
                                        MAKELPARAM((WORD)ModifierState,
                                                   (WORD)msg.wParam));
               }
               continue; /* Eat key up motion too */
            }

            /*
             * Post a keyboard message.
             */
            co_MsqPostKeyboardMessage(msg.message,msg.wParam,msg.lParam);
         }
      }

KeyboardEscape:
      DPRINT( "KeyboardInput Thread Stopped...\n" );
   }
}


NTSTATUS STDCALL
NtUserAcquireOrReleaseInputOwnership(BOOLEAN Release)
{
   return STATUS_SUCCESS;
}


NTSTATUS FASTCALL
InitInputImpl(VOID)
{
   NTSTATUS Status;

   KeInitializeEvent(&InputThreadsStart, NotificationEvent, FALSE);

   Status = PsCreateSystemThread(&KeyboardThreadHandle,
                                 THREAD_ALL_ACCESS,
                                 NULL,
                                 NULL,
                                 &KeyboardThreadId,
                                 KeyboardThreadMain,
                                 NULL);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Win32K: Failed to create keyboard thread.\n");
   }

   /* Initialize the default keyboard layout */
   if(!UserInitDefaultKeyboardLayout())
   {
      DPRINT1("Failed to initialize default keyboard layout!\n");
   }

   Status = PsCreateSystemThread(&MouseThreadHandle,
                                 THREAD_ALL_ACCESS,
                                 NULL,
                                 NULL,
                                 &MouseThreadId,
                                 MouseThreadMain,
                                 NULL);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Win32K: Failed to create mouse thread.\n");
   }

   InputThreadsRunning = TRUE;
   KeSetEvent(&InputThreadsStart, IO_NO_INCREMENT, FALSE);

   return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
CleanupInputImp(VOID)
{
   return(STATUS_SUCCESS);
}

BOOL
STDCALL
NtUserDragDetect(
   HWND hWnd,
   LONG x,
   LONG y)
{
   UNIMPLEMENTED
   return 0;
}

BOOL FASTCALL
IntBlockInput(PW32THREAD W32Thread, BOOL BlockIt)
{
   PW32THREAD OldBlock;
   ASSERT(W32Thread);

   if(!W32Thread->Desktop || (W32Thread->IsExiting && BlockIt))
   {
      /*
       * fail blocking if exiting the thread
       */

      return FALSE;
   }

   /*
    * FIXME - check access rights of the window station
    *         e.g. services running in the service window station cannot block input
    */
   if(!ThreadHasInputAccess(W32Thread) ||
         !IntIsActiveDesktop(W32Thread->Desktop))
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
   }

   ASSERT(W32Thread->Desktop);
   OldBlock = W32Thread->Desktop->BlockInputThread;
   if(OldBlock)
   {
      if(OldBlock != W32Thread)
      {
         SetLastWin32Error(ERROR_ACCESS_DENIED);
         return FALSE;
      }
      W32Thread->Desktop->BlockInputThread = (BlockIt ? W32Thread : NULL);
      return OldBlock == NULL;
   }

   W32Thread->Desktop->BlockInputThread = (BlockIt ? W32Thread : NULL);
   return OldBlock == NULL;
}

BOOL
STDCALL
NtUserBlockInput(
   BOOL BlockIt)
{
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserBlockInput\n");
   UserEnterExclusive();

   RETURN( IntBlockInput(PsGetCurrentThreadWin32Thread(), BlockIt));

CLEANUP:
   DPRINT("Leave NtUserBlockInput, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL FASTCALL
IntSwapMouseButton(PWINSTATION_OBJECT WinStaObject, BOOL Swap)
{
   PSYSTEM_CURSORINFO CurInfo;
   BOOL res;

   CurInfo = IntGetSysCursorInfo(WinStaObject);
   res = CurInfo->SwapButtons;
   CurInfo->SwapButtons = Swap;
   return res;
}

BOOL FASTCALL
IntMouseInput(MOUSEINPUT *mi)
{
   const UINT SwapBtnMsg[2][2] =
      {
         {
            WM_LBUTTONDOWN, WM_RBUTTONDOWN
         },
         {WM_LBUTTONUP, WM_RBUTTONUP}
      };
   const WPARAM SwapBtn[2] =
      {
         MK_LBUTTON, MK_RBUTTON
      };
   POINT MousePos = {0}, OrgPos;
   PSYSTEM_CURSORINFO CurInfo;
   PWINSTATION_OBJECT WinSta;
   BOOL DoMove, SwapButtons;
   MSG Msg;
   HBITMAP hBitmap;
   BITMAPOBJ *BitmapObj;
   SURFOBJ *SurfObj;
   PDC dc;
   PWINDOW_OBJECT DesktopWindow;

#if 1

   HDC hDC;

   /* FIXME - get the screen dc from the window station or desktop */
   if(!(hDC = IntGetScreenDC()))
   {
      return FALSE;
   }
#endif

   ASSERT(mi);
#if 0

   WinSta = PsGetCurrentProcessWin32Process()->WindowStation;
#else
   /* FIXME - ugly hack but as long as we're using this dumb callback from the
   mouse class driver, we can't access the window station from the calling
   process */
   WinSta = InputWindowStation;
#endif

   ASSERT(WinSta);

   CurInfo = IntGetSysCursorInfo(WinSta);

   if(!mi->time)
   {
      LARGE_INTEGER LargeTickCount;
      KeQueryTickCount(&LargeTickCount);
      mi->time = MsqCalculateMessageTime(&LargeTickCount);
   }

   SwapButtons = CurInfo->SwapButtons;
   DoMove = FALSE;

   IntGetCursorLocation(WinSta, &MousePos);
   OrgPos.x = MousePos.x;
   OrgPos.y = MousePos.y;

   if(mi->dwFlags & MOUSEEVENTF_MOVE)
   {
      if(mi->dwFlags & MOUSEEVENTF_ABSOLUTE)
      {
         MousePos.x = mi->dx;
         MousePos.y = mi->dy;
      }
      else
      {
         MousePos.x += mi->dx;
         MousePos.y += mi->dy;
      }

      DesktopWindow = IntGetWindowObject(WinSta->ActiveDesktop->DesktopWindow);

      if (DesktopWindow)
      {
         if(MousePos.x >= DesktopWindow->ClientRect.right)
            MousePos.x = DesktopWindow->ClientRect.right - 1;
         if(MousePos.y >= DesktopWindow->ClientRect.bottom)
            MousePos.y = DesktopWindow->ClientRect.bottom - 1;
         ObmDereferenceObject(DesktopWindow);
      }

      if(MousePos.x < 0)
         MousePos.x = 0;
      if(MousePos.y < 0)
         MousePos.y = 0;

      if(CurInfo->CursorClipInfo.IsClipped)
      {
         /* The mouse cursor needs to be clipped */

         if(MousePos.x >= (LONG)CurInfo->CursorClipInfo.Right)
            MousePos.x = (LONG)CurInfo->CursorClipInfo.Right;
         if(MousePos.x < (LONG)CurInfo->CursorClipInfo.Left)
            MousePos.x = (LONG)CurInfo->CursorClipInfo.Left;
         if(MousePos.y >= (LONG)CurInfo->CursorClipInfo.Bottom)
            MousePos.y = (LONG)CurInfo->CursorClipInfo.Bottom;
         if(MousePos.y < (LONG)CurInfo->CursorClipInfo.Top)
            MousePos.y = (LONG)CurInfo->CursorClipInfo.Top;
      }

      DoMove = (MousePos.x != OrgPos.x || MousePos.y != OrgPos.y);
   }

   if (DoMove)
   {
      dc = DC_LockDc(hDC);
      if (dc)
      {
         hBitmap = dc->w.hBitmap;
         DC_UnlockDc(dc);

         BitmapObj = BITMAPOBJ_LockBitmap(hBitmap);
         if (BitmapObj)
         {
            SurfObj = &BitmapObj->SurfObj;

            IntEngMovePointer(SurfObj, MousePos.x, MousePos.y, &(GDIDEV(SurfObj)->Pointer.Exclude));
            /* Only now, update the info in the GDIDEVICE, so EngMovePointer can
            * use the old values to move the pointer image */
            GDIDEV(SurfObj)->Pointer.Pos.x = MousePos.x;
            GDIDEV(SurfObj)->Pointer.Pos.y = MousePos.y;

            BITMAPOBJ_UnlockBitmap(BitmapObj);
         }
      }
   }

   /*
    * Insert the messages into the system queue
    */

   Msg.wParam = CurInfo->ButtonsDown;
   Msg.lParam = MAKELPARAM(MousePos.x, MousePos.y);
   Msg.pt = MousePos;
   if(DoMove)
   {
      Msg.message = WM_MOUSEMOVE;
      MsqInsertSystemMessage(&Msg);
   }

   Msg.message = 0;
   if(mi->dwFlags & MOUSEEVENTF_LEFTDOWN)
   {
      gQueueKeyStateTable[VK_LBUTTON] |= 0xc0;
      Msg.message = SwapBtnMsg[0][SwapButtons];
      CurInfo->ButtonsDown |= SwapBtn[SwapButtons];
      MsqInsertSystemMessage(&Msg);
   }
   else if(mi->dwFlags & MOUSEEVENTF_LEFTUP)
   {
      gQueueKeyStateTable[VK_LBUTTON] &= ~0x80;
      Msg.message = SwapBtnMsg[1][SwapButtons];
      CurInfo->ButtonsDown &= ~SwapBtn[SwapButtons];
      MsqInsertSystemMessage(&Msg);
   }
   if(mi->dwFlags & MOUSEEVENTF_MIDDLEDOWN)
   {
      gQueueKeyStateTable[VK_MBUTTON] |= 0xc0;
      Msg.message = WM_MBUTTONDOWN;
      CurInfo->ButtonsDown |= MK_MBUTTON;
      MsqInsertSystemMessage(&Msg);
   }
   else if(mi->dwFlags & MOUSEEVENTF_MIDDLEUP)
   {
      gQueueKeyStateTable[VK_MBUTTON] &= ~0x80;
      Msg.message = WM_MBUTTONUP;
      CurInfo->ButtonsDown &= ~MK_MBUTTON;
      MsqInsertSystemMessage(&Msg);
   }
   if(mi->dwFlags & MOUSEEVENTF_RIGHTDOWN)
   {
      gQueueKeyStateTable[VK_RBUTTON] |= 0xc0;
      Msg.message = SwapBtnMsg[0][!SwapButtons];
      CurInfo->ButtonsDown |= SwapBtn[!SwapButtons];
      MsqInsertSystemMessage(&Msg);
   }
   else if(mi->dwFlags & MOUSEEVENTF_RIGHTUP)
   {
      gQueueKeyStateTable[VK_RBUTTON] &= ~0x80;
      Msg.message = SwapBtnMsg[1][!SwapButtons];
      CurInfo->ButtonsDown &= ~SwapBtn[!SwapButtons];
      MsqInsertSystemMessage(&Msg);
   }

   if((mi->dwFlags & (MOUSEEVENTF_XDOWN | MOUSEEVENTF_XUP)) &&
         (mi->dwFlags & MOUSEEVENTF_WHEEL))
   {
      /* fail because both types of events use the mouseData field */
      return FALSE;
   }

   if(mi->dwFlags & MOUSEEVENTF_XDOWN)
   {
      Msg.message = WM_XBUTTONDOWN;
      if(mi->mouseData & XBUTTON1)
      {
         gQueueKeyStateTable[VK_XBUTTON1] |= 0xc0;
         Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON1);
         CurInfo->ButtonsDown |= XBUTTON1;
         MsqInsertSystemMessage(&Msg);
      }
      if(mi->mouseData & XBUTTON2)
      {
         gQueueKeyStateTable[VK_XBUTTON2] |= 0xc0;
         Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON2);
         CurInfo->ButtonsDown |= XBUTTON2;
         MsqInsertSystemMessage(&Msg);
      }
   }
   else if(mi->dwFlags & MOUSEEVENTF_XUP)
   {
      Msg.message = WM_XBUTTONUP;
      if(mi->mouseData & XBUTTON1)
      {
         gQueueKeyStateTable[VK_XBUTTON1] &= ~0x80;
         Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON1);
         CurInfo->ButtonsDown &= ~XBUTTON1;
         MsqInsertSystemMessage(&Msg);
      }
      if(mi->mouseData & XBUTTON2)
      {
         gQueueKeyStateTable[VK_XBUTTON2] &= ~0x80;
         Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, XBUTTON2);
         CurInfo->ButtonsDown &= ~XBUTTON2;
         MsqInsertSystemMessage(&Msg);
      }
   }
   if(mi->dwFlags & MOUSEEVENTF_WHEEL)
   {
      Msg.message = WM_MOUSEWHEEL;
      Msg.wParam = MAKEWPARAM(CurInfo->ButtonsDown, mi->mouseData);
      MsqInsertSystemMessage(&Msg);
   }

   return TRUE;
}

BOOL FASTCALL
IntKeyboardInput(KEYBDINPUT *ki)
{
   return FALSE;
}

UINT
STDCALL
NtUserSendInput(
   UINT nInputs,
   LPINPUT pInput,
   INT cbSize)
{
   PW32THREAD W32Thread;
   UINT cnt;
   DECLARE_RETURN(UINT);

   DPRINT("Enter NtUserSendInput\n");
   UserEnterExclusive();

   W32Thread = PsGetCurrentThreadWin32Thread();
   ASSERT(W32Thread);

   if(!W32Thread->Desktop)
   {
      RETURN( 0);
   }

   if(!nInputs || !pInput || (cbSize != sizeof(INPUT)))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   /*
    * FIXME - check access rights of the window station
    *         e.g. services running in the service window station cannot block input
    */
   if(!ThreadHasInputAccess(W32Thread) ||
         !IntIsActiveDesktop(W32Thread->Desktop))
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      RETURN( 0);
   }

   cnt = 0;
   while(nInputs--)
   {
      INPUT SafeInput;
      NTSTATUS Status;

      Status = MmCopyFromCaller(&SafeInput, pInput++, sizeof(INPUT));
      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN( cnt);
      }

      switch(SafeInput.type)
      {
         case INPUT_MOUSE:
            if(IntMouseInput(&SafeInput.mi))
            {
               cnt++;
            }
            break;
         case INPUT_KEYBOARD:
            if(IntKeyboardInput(&SafeInput.ki))
            {
               cnt++;
            }
            break;
         case INPUT_HARDWARE:
            break;
#ifndef NDEBUG

         default:
            DPRINT1("SendInput(): Invalid input type: 0x%x\n", SafeInput.type);
            break;
#endif

      }
   }

   RETURN( cnt);

CLEANUP:
   DPRINT("Leave NtUserSendInput, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/* EOF */
