/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Callback to usermode support
 * FILE:             win32ss/user/ntuser/callback.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * NOTES:            Please use the Callback Memory Management functions for
 *                   callbacks to make sure, the memory is freed on thread
 *                   termination!
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserCallback);


/* CALLBACK MEMORY MANAGEMENT ************************************************/

typedef struct _INT_CALLBACK_HEADER
{
   /* List entry in the THREADINFO structure */
   LIST_ENTRY ListEntry;
}
INT_CALLBACK_HEADER, *PINT_CALLBACK_HEADER;

PVOID FASTCALL
IntCbAllocateMemory(ULONG Size)
{
   PINT_CALLBACK_HEADER Mem;
   PTHREADINFO W32Thread;

   if(!(Mem = ExAllocatePoolWithTag(PagedPool, Size + sizeof(INT_CALLBACK_HEADER),
                                    USERTAG_CALLBACK)))
   {
      return NULL;
   }

   RtlZeroMemory(Mem, Size + sizeof(INT_CALLBACK_HEADER));
   W32Thread = PsGetCurrentThreadWin32Thread();
   ASSERT(W32Thread);

   /* Insert the callback memory into the thread's callback list */

   InsertTailList(&W32Thread->W32CallbackListHead, &Mem->ListEntry);

   return (Mem + 1);
}

VOID FASTCALL
IntCbFreeMemory(PVOID Data)
{
   PINT_CALLBACK_HEADER Mem;
   PTHREADINFO W32Thread;

   W32Thread = PsGetCurrentThreadWin32Thread();
   ASSERT(W32Thread);

   if (W32Thread->TIF_flags & TIF_INCLEANUP)
   {
      ERR("CbFM Thread is already in cleanup\n");
      return;
   }

   ASSERT(Data);

   Mem = ((PINT_CALLBACK_HEADER)Data - 1);

   /* Remove the memory block from the thread's callback list */
   RemoveEntryList(&Mem->ListEntry);

   /* Free memory */
   ExFreePoolWithTag(Mem, USERTAG_CALLBACK);
}

VOID FASTCALL
IntCleanupThreadCallbacks(PTHREADINFO W32Thread)
{
   PLIST_ENTRY CurrentEntry;
   PINT_CALLBACK_HEADER Mem;

   while (!IsListEmpty(&W32Thread->W32CallbackListHead))
   {
      CurrentEntry = RemoveHeadList(&W32Thread->W32CallbackListHead);
      Mem = CONTAINING_RECORD(CurrentEntry, INT_CALLBACK_HEADER,
                              ListEntry);

      /* Free memory */
      ExFreePoolWithTag(Mem, USERTAG_CALLBACK);
   }
}

//
// Pass the Current Window handle and pointer to the Client Callback.
// This will help user space programs speed up read access with the window object.
//
static VOID
IntSetTebWndCallback (HWND * hWnd, PWND * pWnd, PVOID * pActCtx)
{
  HWND hWndS = *hWnd;
  PWND Window = UserGetWindowObject(*hWnd);
  PCLIENTINFO ClientInfo = GetWin32ClientInfo();

  *hWnd = ClientInfo->CallbackWnd.hWnd;
  *pWnd = ClientInfo->CallbackWnd.pWnd;
  *pActCtx = ClientInfo->CallbackWnd.pActCtx;

  if (Window)
  {
     ClientInfo->CallbackWnd.hWnd = hWndS;
     ClientInfo->CallbackWnd.pWnd = DesktopHeapAddressToUser(Window);
     ClientInfo->CallbackWnd.pActCtx = Window->pActCtx;
  }
  else //// What if Dispatching WM_SYS/TIMER with NULL window? Fix AbiWord Crash when sizing.
  {
     ClientInfo->CallbackWnd.hWnd = hWndS;
     ClientInfo->CallbackWnd.pWnd = Window;
     ClientInfo->CallbackWnd.pActCtx = 0;
  }
}

static VOID
IntRestoreTebWndCallback (HWND hWnd, PWND pWnd, PVOID pActCtx)
{
  PCLIENTINFO ClientInfo = GetWin32ClientInfo();

  ClientInfo->CallbackWnd.hWnd = hWnd;
  ClientInfo->CallbackWnd.pWnd = pWnd;
  ClientInfo->CallbackWnd.pActCtx = pActCtx;
}

/* FUNCTIONS *****************************************************************/

/* Calls ClientLoadLibrary in user32 */
BOOL
NTAPI
co_IntClientLoadLibrary(PUNICODE_STRING pstrLibName,
                        PUNICODE_STRING pstrInitFunc,
                        BOOL Unload,
                        BOOL ApiHook)
{
   PVOID ResultPointer;
   ULONG ResultLength;
   ULONG ArgumentLength;
   PCLIENT_LOAD_LIBRARY_ARGUMENTS pArguments;
   NTSTATUS Status;
   BOOL bResult;
   ULONG_PTR pLibNameBuffer = 0, pInitFuncBuffer = 0;

   /* Do not allow the desktop thread to do callback to user mode */
   ASSERT(PsGetCurrentThreadWin32Thread() != gptiDesktopThread);

   TRACE("co_IntClientLoadLibrary: %S, %S, %d, %d\n", pstrLibName->Buffer, pstrLibName->Buffer, Unload, ApiHook);

   /* Calculate the size of the argument */
   ArgumentLength = sizeof(CLIENT_LOAD_LIBRARY_ARGUMENTS);
   if(pstrLibName)
   {
       pLibNameBuffer = ArgumentLength;
       ArgumentLength += pstrLibName->Length + sizeof(WCHAR);
   }
   if(pstrInitFunc)
   {
       pInitFuncBuffer = ArgumentLength;
       ArgumentLength += pstrInitFunc->Length + sizeof(WCHAR);
   }

   /* Allocate the argument */
   pArguments = IntCbAllocateMemory(ArgumentLength);
   if(pArguments == NULL)
   {
       return FALSE;
   }

   /* Fill the argument */
   pArguments->Unload = Unload;
   pArguments->ApiHook = ApiHook;
   if(pstrLibName)
   {
       /* Copy the string to the callback memory */
       pLibNameBuffer += (ULONG_PTR)pArguments;
       pArguments->strLibraryName.Buffer = (PWCHAR)pLibNameBuffer;
       pArguments->strLibraryName.MaximumLength = pstrLibName->Length + sizeof(WCHAR);
       RtlCopyUnicodeString(&pArguments->strLibraryName, pstrLibName);

       /* Fix argument pointer to be relative to the argument */
       pLibNameBuffer -= (ULONG_PTR)pArguments;
       pArguments->strLibraryName.Buffer = (PWCHAR)(pLibNameBuffer);
   }

   if(pstrInitFunc)
   {
       /* Copy the strings to the callback memory */
       pInitFuncBuffer += (ULONG_PTR)pArguments;
       pArguments->strInitFuncName.Buffer = (PWCHAR)pInitFuncBuffer;
       pArguments->strInitFuncName.MaximumLength = pstrInitFunc->Length + sizeof(WCHAR);
       RtlCopyUnicodeString(&pArguments->strInitFuncName, pstrInitFunc);

       /* Fix argument pointers to be relative to the argument */
       pInitFuncBuffer -= (ULONG_PTR)pArguments;
       pArguments->strInitFuncName.Buffer = (PWCHAR)(pInitFuncBuffer);
   }

   /* Do the callback */
   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_CLIENTLOADLIBRARY,
                               pArguments,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   /* Free the argument */
   IntCbFreeMemory(pArguments);

   if(!NT_SUCCESS(Status))
   {
       return FALSE;
   }

   _SEH2_TRY
   {
       /* Probe and copy the usermode result data */
       ProbeForRead(ResultPointer, sizeof(HMODULE), 1);
       bResult = *(BOOL*)ResultPointer;
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       bResult = FALSE;
   }
   _SEH2_END;

   return bResult;
}

VOID APIENTRY
co_IntCallSentMessageCallback(SENDASYNCPROC CompletionCallback,
                              HWND hWnd,
                              UINT Msg,
                              ULONG_PTR CompletionCallbackContext,
                              LRESULT Result)
{
   SENDASYNCPROC_CALLBACK_ARGUMENTS Arguments;
   PVOID ResultPointer, pActCtx;
   PWND pWnd;
   ULONG ResultLength;
   NTSTATUS Status;

   /* Do not allow the desktop thread to do callback to user mode */
   ASSERT(PsGetCurrentThreadWin32Thread() != gptiDesktopThread);

   Arguments.Callback = CompletionCallback;
   Arguments.Wnd = hWnd;
   Arguments.Msg = Msg;
   Arguments.Context = CompletionCallbackContext;
   Arguments.Result = Result;

   IntSetTebWndCallback (&hWnd, &pWnd, &pActCtx);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_SENDASYNCPROC,
                               &Arguments,
                               sizeof(SENDASYNCPROC_CALLBACK_ARGUMENTS),
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   IntRestoreTebWndCallback (hWnd, pWnd, pActCtx);

   if (!NT_SUCCESS(Status))
   {
      ERR("KeUserModeCallback failed with %lx\n", Status);
      return;
   }
   return;
}

LRESULT APIENTRY
co_IntCallWindowProc(WNDPROC Proc,
                     BOOLEAN IsAnsiProc,
                     HWND Wnd,
                     UINT Message,
                     WPARAM wParam,
                     LPARAM lParam,
                     INT lParamBufferSize)
{
   WINDOWPROC_CALLBACK_ARGUMENTS StackArguments = { 0 };
   PWINDOWPROC_CALLBACK_ARGUMENTS Arguments;
   NTSTATUS Status;
   PVOID ResultPointer, pActCtx;
   PWND pWnd;
   ULONG ResultLength;
   ULONG ArgumentLength;
   LRESULT Result;

   TRACE("co_IntCallWindowProc(Proc %p, IsAnsiProc: %s, Wnd %p, Message %u, wParam %Iu, lParam %Id, lParamBufferSize %d)\n",
       Proc, IsAnsiProc ? "TRUE" : "FALSE", Wnd, Message, wParam, lParam, lParamBufferSize);

   /* Do not allow the desktop thread to do callback to user mode */
   ASSERT(PsGetCurrentThreadWin32Thread() != gptiDesktopThread);

   if (lParamBufferSize != -1)
   {
      ArgumentLength = sizeof(WINDOWPROC_CALLBACK_ARGUMENTS) + lParamBufferSize;
      Arguments = IntCbAllocateMemory(ArgumentLength);
      if (NULL == Arguments)
      {
         ERR("Unable to allocate buffer for window proc callback\n");
         return -1;
      }
      RtlMoveMemory((PVOID) ((char *) Arguments + sizeof(WINDOWPROC_CALLBACK_ARGUMENTS)),
                    (PVOID) lParam, lParamBufferSize);
   }
   else
   {
      Arguments = &StackArguments;
      ArgumentLength = sizeof(WINDOWPROC_CALLBACK_ARGUMENTS);
   }
   Arguments->Proc = Proc;
   Arguments->IsAnsiProc = IsAnsiProc;
   Arguments->Wnd = Wnd;
   Arguments->Msg = Message;
   Arguments->wParam = wParam;
   Arguments->lParam = lParam;
   Arguments->lParamBufferSize = lParamBufferSize;
   ResultPointer = NULL;
   ResultLength = ArgumentLength;

   IntSetTebWndCallback (&Wnd, &pWnd, &pActCtx);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_WINDOWPROC,
                               Arguments,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);
   if (!NT_SUCCESS(Status))
   {
      ERR("Error Callback to User space Status %lx Message %d\n",Status,Message);
      UserEnterCo();
      return 0;
   }

   _SEH2_TRY
   {
      /* Simulate old behaviour: copy into our local buffer */
      RtlMoveMemory(Arguments, ResultPointer, ArgumentLength);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      ERR("Failed to copy result from user mode, Message %u lParam size %d!\n", Message, lParamBufferSize);
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;

   UserEnterCo();

   IntRestoreTebWndCallback (Wnd, pWnd, pActCtx);

   if (!NT_SUCCESS(Status))
   {
     ERR("Call to user mode failed! 0x%08lx\n",Status);
      if (lParamBufferSize != -1)
      {
         IntCbFreeMemory(Arguments);
      }
      return -1;
   }
   Result = Arguments->Result;

   if (lParamBufferSize != -1)
   {
      PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
      // Is this message being processed from inside kernel space?
      BOOL InSendMessage = (pti->pcti->CTI_flags & CTI_INSENDMESSAGE);

      TRACE("Copy lParam Message %u lParam %d!\n", Message, lParam);
      switch (Message)
      {
          default:
            TRACE("Don't copy lParam, Message %u Size %d lParam %d!\n", Message, lParamBufferSize, lParam);
            break;
          // Write back to user/kernel space. Also see g_MsgMemory.
          case WM_CREATE:
          case WM_GETMINMAXINFO:
          case WM_GETTEXT:
          case WM_NCCALCSIZE:
          case WM_NCCREATE:
          case WM_STYLECHANGING:
          case WM_WINDOWPOSCHANGING:
          case WM_SIZING:
          case WM_MOVING:
          case WM_MEASUREITEM:
          case WM_NEXTMENU:
            TRACE("Copy lParam, Message %u Size %d lParam %d!\n", Message, lParamBufferSize, lParam);
            if (InSendMessage)
               // Copy into kernel space.
               RtlMoveMemory((PVOID) lParam,
                             (PVOID) ((char *) Arguments + sizeof(WINDOWPROC_CALLBACK_ARGUMENTS)),
                              lParamBufferSize);
            else
            {
             _SEH2_TRY
             { // Copy into user space.
               RtlMoveMemory((PVOID) lParam,
                             (PVOID) ((char *) Arguments + sizeof(WINDOWPROC_CALLBACK_ARGUMENTS)),
                              lParamBufferSize);
             }
             _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
             {
                ERR("Failed to copy lParam to user space, Message %u!\n", Message);
             }
             _SEH2_END;
            }
            break;
      }
      IntCbFreeMemory(Arguments);
   }

   return Result;
}

HMENU APIENTRY
co_IntLoadSysMenuTemplate(VOID)
{
   LRESULT Result = 0;
   NTSTATUS Status;
   PVOID ResultPointer;
   ULONG ResultLength;

   /* Do not allow the desktop thread to do callback to user mode */
   ASSERT(PsGetCurrentThreadWin32Thread() != gptiDesktopThread);

   ResultPointer = NULL;
   ResultLength = sizeof(LRESULT);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_LOADSYSMENUTEMPLATE,
                               &ResultPointer,
                               0,
                               &ResultPointer,
                               &ResultLength);
   if (NT_SUCCESS(Status))
   {
      /* Simulate old behaviour: copy into our local buffer */
      _SEH2_TRY
      {
        ProbeForRead(ResultPointer, sizeof(LRESULT), 1);
        Result = *(LRESULT*)ResultPointer;
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
        Result = 0;
      }
      _SEH2_END;
   }

   UserEnterCo();

   return (HMENU)Result;
}

extern HCURSOR gDesktopCursor;

BOOL APIENTRY
co_IntLoadDefaultCursors(VOID)
{
   NTSTATUS Status;
   PVOID ResultPointer;
   ULONG ResultLength;
   BOOL DefaultCursor = TRUE;

   /* Do not allow the desktop thread to do callback to user mode */
   ASSERT(PsGetCurrentThreadWin32Thread() != gptiDesktopThread);

   ResultPointer = NULL;
   ResultLength = sizeof(HCURSOR);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_LOADDEFAULTCURSORS,
                               &DefaultCursor,
                               sizeof(BOOL),
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   if (!NT_SUCCESS(Status))
   {
      return FALSE;
   }

   /* HACK: The desktop class doen't have a proper cursor yet, so set it here */
    gDesktopCursor = *((HCURSOR*)ResultPointer);

   return TRUE;
}

static INT iTheId = -2; // Set it out of range.

LRESULT APIENTRY
co_IntCallHookProc(INT HookId,
                   INT Code,
                   WPARAM wParam,
                   LPARAM lParam,
                   HOOKPROC Proc,
                   INT Mod,
                   ULONG_PTR offPfn,
                   BOOLEAN Ansi,
                   PUNICODE_STRING ModuleName)
{
   ULONG ArgumentLength;
   PVOID Argument = NULL;
   LRESULT Result = 0;
   NTSTATUS Status;
   PVOID ResultPointer;
   ULONG ResultLength;
   PHOOKPROC_CALLBACK_ARGUMENTS Common;
   CBT_CREATEWNDW *CbtCreateWnd = NULL;
   PCHAR Extra;
   PHOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS CbtCreatewndExtra = NULL;
   PTHREADINFO pti;
   PWND pWnd;
   PMSG pMsg = NULL;
   BOOL Hit = FALSE;
   UINT lParamSize = 0;
   CWPSTRUCT* pCWP = NULL;
   CWPRETSTRUCT* pCWPR = NULL;

   ASSERT(Proc);
   /* Do not allow the desktop thread to do callback to user mode */
   ASSERT(PsGetCurrentThreadWin32Thread() != gptiDesktopThread);

   pti = PsGetCurrentThreadWin32Thread();
   if (pti->TIF_flags & TIF_INCLEANUP)
   {
      ERR("Thread is in cleanup and trying to call hook %d\n", Code);
      return 0;
   }

   ArgumentLength = sizeof(HOOKPROC_CALLBACK_ARGUMENTS);

   switch(HookId)
   {
      case WH_CBT:
         TRACE("WH_CBT: Code %d\n", Code);
         switch(Code)
         {
            case HCBT_CREATEWND:
               pWnd = UserGetWindowObject((HWND) wParam);
               if (!pWnd)
               {
                  ERR("WH_CBT HCBT_CREATEWND wParam bad hWnd!\n");
                  goto Fault_Exit;
               }
               TRACE("HCBT_CREATEWND AnsiCreator %s, AnsiHook %s\n", pWnd->state & WNDS_ANSICREATOR ? "True" : "False", Ansi ? "True" : "False");
              // Due to KsStudio.exe, just pass the callers original pointers
              // except class which point to kernel space if not an atom.
              // Found by, Olaf Siejka
               CbtCreateWnd = (CBT_CREATEWNDW *) lParam;
               ArgumentLength += sizeof(HOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS);
               break;

            case HCBT_MOVESIZE:
               ArgumentLength += sizeof(RECTL);
               break;
            case HCBT_ACTIVATE:
               ArgumentLength += sizeof(CBTACTIVATESTRUCT);
               break;
            case HCBT_CLICKSKIPPED:
               ArgumentLength += sizeof(MOUSEHOOKSTRUCT);
               break;
/*   ATM pass on */
            case HCBT_KEYSKIPPED:
            case HCBT_MINMAX:
            case HCBT_SETFOCUS:
            case HCBT_SYSCOMMAND:
/*   These types pass through. */
            case HCBT_DESTROYWND:
            case HCBT_QS:
               break;
            default:
               ERR("Trying to call unsupported CBT hook %d\n", Code);
               goto Fault_Exit;
         }
         break;
     case WH_KEYBOARD_LL:
         ArgumentLength += sizeof(KBDLLHOOKSTRUCT);
         break;
     case WH_MOUSE_LL:
         ArgumentLength += sizeof(MSLLHOOKSTRUCT);
         break;
     case WH_MOUSE:
         ArgumentLength += sizeof(MOUSEHOOKSTRUCT);
         break;
     case WH_CALLWNDPROC:
     {
         pCWP = (CWPSTRUCT*) lParam;
         ArgumentLength = sizeof(CWP_Struct);
         if ( pCWP->message == WM_CREATE || pCWP->message == WM_NCCREATE )
         {
             lParamSize = sizeof(CREATESTRUCTW);
         }
         else
             lParamSize = lParamMemorySize(pCWP->message, pCWP->wParam, pCWP->lParam);
         ArgumentLength += lParamSize;
         break;
      }
      case WH_CALLWNDPROCRET:
      {
         pCWPR = (CWPRETSTRUCT*) lParam;
         ArgumentLength = sizeof(CWPR_Struct);
         if ( pCWPR->message == WM_CREATE || pCWPR->message == WM_NCCREATE )
         {
             lParamSize = sizeof(CREATESTRUCTW);
         }
         else
             lParamSize = lParamMemorySize(pCWPR->message, pCWPR->wParam, pCWPR->lParam);
         ArgumentLength += lParamSize;
         break;
      }
      case WH_MSGFILTER:
      case WH_SYSMSGFILTER:
      case WH_GETMESSAGE:
         ArgumentLength += sizeof(MSG);
         break;
      case WH_FOREGROUNDIDLE:
      case WH_KEYBOARD:
      case WH_SHELL:
         break;
      default:
         ERR("Trying to call unsupported window hook %d\n", HookId);
         goto Fault_Exit;
   }

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      ERR("HookProc callback %d failed: out of memory %d\n",HookId,ArgumentLength);
      goto Fault_Exit;
   }
   Common = (PHOOKPROC_CALLBACK_ARGUMENTS) Argument;
   Common->HookId = HookId;
   Common->Code = Code;
   Common->wParam = wParam;
   Common->lParam = lParam;
   Common->Proc = Proc;
   Common->Mod = Mod;
   Common->offPfn = offPfn;
   Common->Ansi = Ansi;
   Common->lParamSize = lParamSize;
   if (ModuleName->Buffer && ModuleName->Length)
   {
      RtlCopyMemory(&Common->ModuleName, ModuleName->Buffer, ModuleName->Length);
      // If ModuleName->Buffer NULL while in destroy,
      //    this will make User32:Hook.c complain about not loading the library module.
      // Fix symptom for CORE-10549.
   }
   Extra = (PCHAR) Common + sizeof(HOOKPROC_CALLBACK_ARGUMENTS);

   switch(HookId)
   {
      case WH_CBT:
         switch(Code)
         { // Need to remember this is not the first time through! Call Next Hook?
            case HCBT_CREATEWND:
               CbtCreatewndExtra = (PHOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS) Extra;
               RtlCopyMemory( &CbtCreatewndExtra->Cs, CbtCreateWnd->lpcs, sizeof(CREATESTRUCTW) );
               CbtCreatewndExtra->WndInsertAfter = CbtCreateWnd->hwndInsertAfter;
               CbtCreatewndExtra->Cs.lpszClass   = CbtCreateWnd->lpcs->lpszClass;
               CbtCreatewndExtra->Cs.lpszName    = CbtCreateWnd->lpcs->lpszName;
               Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
               //ERR("HCBT_CREATEWND: hWnd %p Csw %p Name %p Class %p\n", Common->wParam, CbtCreateWnd->lpcs, CbtCreateWnd->lpcs->lpszName, CbtCreateWnd->lpcs->lpszClass);
               break;
            case HCBT_CLICKSKIPPED:
               RtlCopyMemory(Extra, (PVOID) lParam, sizeof(MOUSEHOOKSTRUCT));
               Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
               break;
            case HCBT_MOVESIZE:
               RtlCopyMemory(Extra, (PVOID) lParam, sizeof(RECTL));
               Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
               break;
            case HCBT_ACTIVATE:
               RtlCopyMemory(Extra, (PVOID) lParam, sizeof(CBTACTIVATESTRUCT));
               Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
               break;
         }
         break;
      case WH_KEYBOARD_LL:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(KBDLLHOOKSTRUCT));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;
      case WH_MOUSE_LL:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(MSLLHOOKSTRUCT));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;
      case WH_MOUSE:
         RtlCopyMemory(Extra, (PVOID) lParam, sizeof(MOUSEHOOKSTRUCT));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;
      case WH_CALLWNDPROC:
      {
         PCWP_Struct pcwps = (PCWP_Struct)Common;
         RtlCopyMemory( &pcwps->cwps, pCWP, sizeof(CWPSTRUCT));
         /* For CALLWNDPROC and CALLWNDPROCRET, we must be wary of the fact that
          * lParam could be a pointer to a buffer. This buffer must be exported
          * to user space too */
         if ( lParamSize )
         {
             RtlCopyMemory( &pcwps->Extra, (PVOID)pCWP->lParam, lParamSize );
         }
      }
         break;
      case WH_CALLWNDPROCRET:
      {
         PCWPR_Struct pcwprs = (PCWPR_Struct)Common;
         RtlCopyMemory( &pcwprs->cwprs, pCWPR, sizeof(CWPRETSTRUCT));
         if ( lParamSize )
         {
             RtlCopyMemory( &pcwprs->Extra, (PVOID)pCWPR->lParam, lParamSize );
         }
      }
         break;
      case WH_MSGFILTER:
      case WH_SYSMSGFILTER:
      case WH_GETMESSAGE:
         pMsg = (PMSG)lParam;
         RtlCopyMemory(Extra, (PVOID) pMsg, sizeof(MSG));
         Common->lParam = (LPARAM) (Extra - (PCHAR) Common);
         break;
      case WH_FOREGROUNDIDLE:
      case WH_KEYBOARD:
      case WH_SHELL:
         break;
   }

   ResultPointer = NULL;
   ResultLength = ArgumentLength;

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_HOOKPROC,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   if (!NT_SUCCESS(Status))
   {
      if ( iTheId != HookId ) // Hook ID can change.
      {
          ERR("Failure to make Callback %d! Status 0x%x ArgumentLength %d\n",HookId,Status,ArgumentLength);
          iTheId = HookId;
      }
      goto Fault_Exit;
   }

   if (ResultPointer)
   {
      _SEH2_TRY
      {
         /* Simulate old behaviour: copy into our local buffer */
         RtlMoveMemory(Argument, ResultPointer, ArgumentLength);
         Result = Common->Result;
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Result = 0;
         Hit = TRUE;
      }
      _SEH2_END;
   }
   else
   {
      ERR("ERROR: Hook %d Code %d ResultPointer 0x%p ResultLength %u\n",HookId,Code,ResultPointer,ResultLength);
   }

   /* Support write backs... SEH is in UserCallNextHookEx. */
   switch (HookId)
   {
      case WH_CBT:
      {
         switch (Code)
         {
            case HCBT_CREATEWND:
               if (CbtCreatewndExtra)
               {/*
                  The parameters could have been changed, include the coordinates
                  and dimensions of the window. We copy it back.
                 */
                  CbtCreateWnd->hwndInsertAfter = CbtCreatewndExtra->WndInsertAfter;
                  CbtCreateWnd->lpcs->x  = CbtCreatewndExtra->Cs.x;
                  CbtCreateWnd->lpcs->y  = CbtCreatewndExtra->Cs.y;
                  CbtCreateWnd->lpcs->cx = CbtCreatewndExtra->Cs.cx;
                  CbtCreateWnd->lpcs->cy = CbtCreatewndExtra->Cs.cy;
               }
            break;
            case HCBT_MOVESIZE:
               if (Extra && lParam)
               {
                  RtlCopyMemory((PVOID) lParam, Extra, sizeof(RECTL));
               }
            break;
         }
      }
      // "The GetMsgProc hook procedure can examine or modify the message."
      case WH_GETMESSAGE:
         if (pMsg)
         {
            RtlCopyMemory((PVOID) pMsg, Extra, sizeof(MSG));
         }
         break;
   }

Fault_Exit:
   if (Hit)
   {
      ERR("Exception CallHookProc HookId %d Code %d\n",HookId,Code);
   }
   if (Argument) IntCbFreeMemory(Argument);

   return Result;
}

//
// Events are notifications w/o results.
//
LRESULT
APIENTRY
co_IntCallEventProc(HWINEVENTHOOK hook,
                           DWORD event,
                             HWND hWnd,
                         LONG idObject,
                          LONG idChild,
                   DWORD dwEventThread,
                   DWORD dwmsEventTime,
                     WINEVENTPROC Proc,
                               INT Mod,
                     ULONG_PTR offPfn)
{
   LRESULT Result = 0;
   NTSTATUS Status;
   PEVENTPROC_CALLBACK_ARGUMENTS Common;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer;

   ArgumentLength = sizeof(EVENTPROC_CALLBACK_ARGUMENTS);

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      ERR("EventProc callback failed: out of memory\n");
      return 0;
   }
   Common = (PEVENTPROC_CALLBACK_ARGUMENTS) Argument;
   Common->hook = hook;
   Common->event = event;
   Common->hwnd = hWnd;
   Common->idObject = idObject;
   Common->idChild = idChild;
   Common->dwEventThread = dwEventThread;
   Common->dwmsEventTime = dwmsEventTime;
   Common->Proc = Proc;
   Common->Mod = Mod;
   Common->offPfn = offPfn;

   ResultPointer = NULL;
   ResultLength = sizeof(LRESULT);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_EVENTPROC,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   IntCbFreeMemory(Argument);

   if (!NT_SUCCESS(Status))
   {
      return 0;
   }

   return Result;
}

//
// Callback Load Menu and results.
//
HMENU
APIENTRY
co_IntCallLoadMenu( HINSTANCE hModule,
                    PUNICODE_STRING pMenuName )
{
   LRESULT Result = 0;
   NTSTATUS Status;
   PLOADMENU_CALLBACK_ARGUMENTS Common;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer;

   ArgumentLength = sizeof(LOADMENU_CALLBACK_ARGUMENTS);

   ArgumentLength += pMenuName->Length + sizeof(WCHAR);

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      ERR("LoadMenu callback failed: out of memory\n");
      return 0;
   }
   Common = (PLOADMENU_CALLBACK_ARGUMENTS) Argument;

   Common->hModule = hModule;
   if (pMenuName->Length)
      RtlCopyMemory(&Common->MenuName, pMenuName->Buffer, pMenuName->Length);
   else
      Common->InterSource = pMenuName->Buffer;

   ResultPointer = NULL;
   ResultLength = sizeof(LRESULT);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_LOADMENU,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   if (NT_SUCCESS(Status))
   {
      Result = *(LRESULT*)ResultPointer;
   }
   else
   {
      Result = 0;
   }

   IntCbFreeMemory(Argument);

   return (HMENU)Result;
}

NTSTATUS
APIENTRY
co_IntClientThreadSetup(VOID)
{
   NTSTATUS Status;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer;

   /* Do not allow the desktop thread to do callback to user mode */
   ASSERT(PsGetCurrentThreadWin32Thread() != gptiDesktopThread);

   ArgumentLength = ResultLength = 0;
   Argument = ResultPointer = NULL;

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_CLIENTTHREADSTARTUP,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   return Status;
}

HANDLE FASTCALL
co_IntCopyImage(HANDLE hnd, UINT type, INT desiredx, INT desiredy, UINT flags)
{
   HANDLE Handle;
   NTSTATUS Status;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer;
   PCOPYIMAGE_CALLBACK_ARGUMENTS Common;

   ArgumentLength = ResultLength = 0;
   Argument = ResultPointer = NULL;

   ArgumentLength = sizeof(COPYIMAGE_CALLBACK_ARGUMENTS);

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      ERR("CopyImage callback failed: out of memory\n");
      return 0;
   }
   Common = (PCOPYIMAGE_CALLBACK_ARGUMENTS) Argument;

   Common->hImage = hnd;
   Common->uType = type;
   Common->cxDesired = desiredx;
   Common->cyDesired = desiredy;
   Common->fuFlags = flags;

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_COPYIMAGE,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);


   UserEnterCo();

   if (NT_SUCCESS(Status))
   {
      Handle = *(HANDLE*)ResultPointer;
   }
   else
   {
      ERR("CopyImage callback failed!\n");
      Handle = NULL;
   }

   IntCbFreeMemory(Argument);

   return Handle;
}

BOOL
APIENTRY
co_IntGetCharsetInfo(LCID Locale, PCHARSETINFO pCs)
{
   NTSTATUS Status;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer;
   PGET_CHARSET_INFO Common;

   ArgumentLength = sizeof(GET_CHARSET_INFO);

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      ERR("GetCharsetInfo callback failed: out of memory\n");
      return 0;
   }
   Common = (PGET_CHARSET_INFO) Argument;

   Common->Locale = Locale;

   ResultPointer = NULL;
   ResultLength = ArgumentLength;

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_GETCHARSETINFO,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   if (NT_SUCCESS(Status))
   {
      _SEH2_TRY
      {
         /* Need to copy into our local buffer */
         RtlMoveMemory(Argument, ResultPointer, ArgumentLength);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         ERR("Failed to copy result from user mode!\n");
         Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END;
   }

   UserEnterCo();

   RtlCopyMemory(pCs, &Common->Cs, sizeof(CHARSETINFO));

   IntCbFreeMemory(Argument);

   if (!NT_SUCCESS(Status))
   {
      ERR("GetCharsetInfo Failed!!\n");
      return FALSE;
   }

   return TRUE;
}

BOOL FASTCALL
co_IntSetWndIcons(VOID)
{
   NTSTATUS Status;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer;
   PSETWNDICONS_CALLBACK_ARGUMENTS Common;

   ResultPointer = NULL;
   ResultLength = ArgumentLength = sizeof(SETWNDICONS_CALLBACK_ARGUMENTS);

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      ERR("Set Window Icons callback failed: out of memory\n");
      return FALSE;
   }
   Common = (PSETWNDICONS_CALLBACK_ARGUMENTS) Argument;

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_SETWNDICONS,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);


   UserEnterCo();

   if (!NT_SUCCESS(Status))
   {
      ERR("Set Window Icons callback failed!\n");
      IntCbFreeMemory(Argument);
      return FALSE;
   }

   RtlMoveMemory(Common, ResultPointer, ArgumentLength);
   gpsi->hIconSmWindows = Common->hIconSmWindows;
   gpsi->hIconWindows   = Common->hIconWindows;

   IntLoadSystenIcons(Common->hIconSample,   OIC_SAMPLE);
   IntLoadSystenIcons(Common->hIconHand,     OIC_HAND);
   IntLoadSystenIcons(Common->hIconQuestion, OIC_QUES);
   IntLoadSystenIcons(Common->hIconBang,     OIC_BANG);
   IntLoadSystenIcons(Common->hIconNote,     OIC_NOTE);
   IntLoadSystenIcons(gpsi->hIconWindows,    OIC_WINLOGO);
   IntLoadSystenIcons(gpsi->hIconSmWindows,  OIC_WINLOGO+1);

   ERR("hIconSmWindows %p hIconWindows %p \n",gpsi->hIconSmWindows,gpsi->hIconWindows);

   IntCbFreeMemory(Argument);

   return TRUE;
}

VOID FASTCALL
co_IntDeliverUserAPC(VOID)
{
   ULONG ResultLength;
   PVOID ResultPointer;
   NTSTATUS Status;
   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_DELIVERUSERAPC,
                               0,
                               0,
                               &ResultPointer,
                               &ResultLength);


   UserEnterCo();

   if (!NT_SUCCESS(Status))
   {
      ERR("Delivering User APC callback failed!\n");
   }
}

VOID FASTCALL
co_IntSetupOBM(VOID)
{
   NTSTATUS Status;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer;
   PSETOBM_CALLBACK_ARGUMENTS Common;

   ResultPointer = NULL;
   ResultLength = ArgumentLength = sizeof(SETOBM_CALLBACK_ARGUMENTS);

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      ERR("Set Window Icons callback failed: out of memory\n");
      return;
   }
   Common = (PSETOBM_CALLBACK_ARGUMENTS) Argument;

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_SETOBM,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);


   UserEnterCo();

   if (!NT_SUCCESS(Status))
   {
      ERR("Set Window Icons callback failed!\n");
      IntCbFreeMemory(Argument);
      return;
   }

   RtlMoveMemory(Common, ResultPointer, ArgumentLength);
   RtlCopyMemory(gpsi->oembmi, Common->oembmi, sizeof(gpsi->oembmi));

   IntCbFreeMemory(Argument);
}

//
//  Called from Kernel GDI sides, no UserLeave/EnterCo required.
//
LRESULT
APIENTRY
co_UserCBClientPrinterThunk( PVOID pkt, INT InSize, PVOID pvOutData, INT OutSize )
{
   NTSTATUS Status;
   PVOID ResultPointer;

   Status = KeUserModeCallback( USER32_CALLBACK_UMPD,
                                pkt,
                                InSize,
                               &ResultPointer,
                               (PULONG)&OutSize );


   if (!NT_SUCCESS(Status))
   {
      ERR("User UMPD callback failed!\n");
      return 1;
   }

   if (OutSize) RtlMoveMemory( pvOutData, ResultPointer, OutSize );

   return 0;
}

// Win: ClientImmProcessKey
DWORD
APIENTRY
co_IntImmProcessKey(HWND hWnd, HKL hKL, UINT vKey, LPARAM lParam, DWORD dwHotKeyID)
{
    DWORD ret = 0;
    NTSTATUS Status;
    ULONG ResultLength = sizeof(DWORD);
    PVOID ResultPointer = NULL;
    IMMPROCESSKEY_CALLBACK_ARGUMENTS Common = { hWnd, hKL, vKey, lParam, dwHotKeyID };

    UserLeaveCo();
    Status = KeUserModeCallback(USER32_CALLBACK_IMMPROCESSKEY,
                                &Common,
                                sizeof(Common),
                                &ResultPointer,
                                &ResultLength);
    UserEnterCo();

    if (NT_SUCCESS(Status))
        ret = *(LPDWORD)ResultPointer;

    return ret;
}

/* EOF */
