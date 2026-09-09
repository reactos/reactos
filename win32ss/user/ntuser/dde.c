/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Dynamic Data Exchange
 * FILE:             win32ss/user/ntuser/dde.c
 * PROGRAMER:
 */

#include <win32k.h>

#include <dde.h>

DBG_DEFAULT_CHANNEL(UserMisc);

//
//  Default information used to support client impersonation.
//
SECURITY_QUALITY_OF_SERVICE gqosDefault = {sizeof(SECURITY_QUALITY_OF_SERVICE),SecurityImpersonation,SECURITY_STATIC_TRACKING,TRUE};

typedef struct _DDEIMP
{
  SECURITY_QUALITY_OF_SERVICE qos;
  SECURITY_CLIENT_CONTEXT ClientContext;
  WORD cRefInit;
  WORD cRefConv;
} DDEIMP, *PDDEIMP;

typedef struct _DDE_DATA
{
  LPARAM lParam;
  int cbSize;
  PVOID pvBuffer;
} DDE_DATA, *PDDE_DATA;

typedef struct _DDE_PROP
{
  PWND spwnd;
  PWND spwndPartner;
  PDDEIMP pddei;
} DDE_PROP, *PDDE_PROP;


//
//  DDE Posting message callback to user side.
//
int
APIENTRY
IntDDEPostCallback(
   IN PWND pWnd,
   IN UINT Msg,
   IN WPARAM wParam,
   IN OUT LPARAM *lParam,
   IN OUT PVOID *Buffer)
{
   NTSTATUS Status;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer;
   PDDEPOSTGET_CALLBACK_ARGUMENTS Common;
   int size = 0;
   ResultPointer = NULL;
   ResultLength = ArgumentLength = sizeof(DDEPOSTGET_CALLBACK_ARGUMENTS);

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      return FALSE;
   }

   Common = (PDDEPOSTGET_CALLBACK_ARGUMENTS) Argument;

   Common->pvData  = 0;
   Common->size    = 0;
   Common->hwnd    = UserHMGetHandle(pWnd);
   Common->message = Msg;
   Common->wParam  = wParam;
   Common->lParam  = *lParam;

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_DDEPOST,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   if (!NT_SUCCESS(Status) || ResultPointer == NULL )
   {
      ERR("DDE Post callback failed!\n");
      IntCbFreeMemory(Argument);
      return 0;
   }

   /* What comes back from user mode is a user mode pointer plus a length, and
      nothing checked either of them: a short answer made the copy below read
      past the end of the returned buffer, and an invalid pointer bugchecked
      the kernel. */
   if (ResultLength < ArgumentLength)
   {
      ERR("DDE Post callback returned a short result: %lu < %lu\n",
          ResultLength, ArgumentLength);
      IntCbFreeMemory(Argument);
      return 0;
   }

   Status = STATUS_SUCCESS;
   _SEH2_TRY
   {
      ProbeForRead(ResultPointer, ArgumentLength, 1);
      RtlCopyMemory(Common, ResultPointer, ArgumentLength);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      ERR("DDE Post callback returned an invalid result pointer 0x%p\n", ResultPointer);
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;

   if (!NT_SUCCESS(Status))
   {
      IntCbFreeMemory(Argument);
      return 0;
   }

   size    = Common->size;
   *lParam = Common->lParam;
   *Buffer = Common->pvData;

   IntCbFreeMemory(Argument);

   return size ? size : -1;
}

//
//  DDE Get/Peek message callback to user side.
//
BOOL
APIENTRY
IntDDEGetCallback(
   IN PWND pWnd,
   IN OUT PMSG pMsg,
   IN PVOID Buffer,
   IN int size)
{
   NTSTATUS Status;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer;
   PDDEPOSTGET_CALLBACK_ARGUMENTS Common;

   ResultPointer = NULL;
   ResultLength = ArgumentLength = sizeof(DDEPOSTGET_CALLBACK_ARGUMENTS)+size;

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      return FALSE;
   }

   Common = (PDDEPOSTGET_CALLBACK_ARGUMENTS) Argument;

   Common->size    = size;
   Common->hwnd    = pMsg->hwnd;
   Common->message = pMsg->message;
   Common->wParam  = pMsg->wParam;
   Common->lParam  = pMsg->lParam;

   if (size && Buffer) RtlCopyMemory(&Common->buffer, Buffer, size);

   UserLeaveCo();

   Status = KeUserModeCallback(USER32_CALLBACK_DDEGET,
                               Argument,
                               ArgumentLength,
                               &ResultPointer,
                               &ResultLength);

   UserEnterCo();

   if (!NT_SUCCESS(Status) || ResultPointer == NULL )
   {
      ERR("DDE Get callback failed!\n");
      IntCbFreeMemory(Argument);
      return FALSE;
   }

   /* Same as in IntDDEPostCallback(): validate what user mode handed back
      before reading from it. */
   if (ResultLength < ArgumentLength)
   {
      ERR("DDE Get callback returned a short result: %lu < %lu\n",
          ResultLength, ArgumentLength);
      IntCbFreeMemory(Argument);
      return FALSE;
   }

   Status = STATUS_SUCCESS;
   _SEH2_TRY
   {
      ProbeForRead(ResultPointer, ArgumentLength, 1);
      RtlMoveMemory(Common, ResultPointer, ArgumentLength);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      ERR("DDE Get callback returned an invalid result pointer 0x%p\n", ResultPointer);
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;

   if (!NT_SUCCESS(Status))
   {
      IntCbFreeMemory(Argument);
      return FALSE;
   }

   pMsg->lParam = Common->lParam;

   IntCbFreeMemory(Argument);

   return TRUE;
}

//
//  DDE Post message hook, intercept DDE messages before going on to the target Processes Thread queue.
//
BOOL
APIENTRY
IntDdePostMessageHook(
   IN PWND pWnd,
   IN UINT Msg,
   IN WPARAM wParam,
   IN OUT LPARAM *lParam,
   IN OUT LONG_PTR *ExtraInfo)
{
   PWND pWndClient;
   PDDE_DATA pddeData;
   NTSTATUS Status;
   int size;
   HGDIOBJ Object = NULL;
   PVOID userBuf = NULL;
   PVOID Buffer = NULL;
   LPARAM lp = *lParam;

   if (pWnd->head.pti->ppi != gptiCurrent->ppi)
   {
      TRACE("Posting long DDE 0x%x\n",Msg);
      // Initiate is sent only across borders.
      if (Msg == WM_DDE_INITIATE)
      {
         return FALSE;
      }

      pWndClient = UserGetWindowObject((HWND)wParam);
      if (pWndClient == NULL)
      {
         // This is terminating so post it.
         if ( Msg == WM_DDE_TERMINATE)
         {
            TRACE("DDE Posted WM_DDE_TERMINATE\n");
            return TRUE;
         }
         TRACE("Invalid DDE Client Window handle\n");
         return FALSE;
      }

      if ( Msg == WM_DDE_REQUEST || Msg == WM_DDE_UNADVISE )
      {
         // Do not bother to callback after validation.
         return TRUE;
      }

      if ( Msg == WM_DDE_TERMINATE )
      {
         //// FIXME Remove Stuff if any...

         // Do not bother to callback.
         return TRUE;
      }

      if ( Msg == WM_DDE_EXECUTE && *lParam == 0)
      {
         // Do not bother to do a callback.
         TRACE("DDE Post EXECUTE lParam 0\n");
         return FALSE;
      }

      // Callback.
      if ((size = IntDDEPostCallback(pWnd, Msg, wParam, &lp, &userBuf)) == 0)
      {
         ERR("DDE Post Callback return 0 0x%x\n", Msg);
         return FALSE;
      }

      // No error HACK.
      if (size == -1)
      {
         size = 0;
      }
      else
      {
         /* The size and the data pointer both come straight from the user mode
            callback, so they cannot be trusted: an invalid pointer used to
            bugcheck the kernel, and a kernel address got copied into a buffer
            that is later handed to another process. */
         if (size <= 0 ||
             userBuf == NULL ||
             (ULONG_PTR)userBuf > (ULONG_PTR)MmHighestUserAddress)
         {
             ERR("DDE Post callback returned bogus data: 0x%p, size %d\n", userBuf, size);
             return FALSE;
         }

         // Set buffer with users data size.
         Buffer = ExAllocatePoolWithTag(PagedPool, size, USERTAG_DDE);
         if (Buffer == NULL)
         {
             ERR("Failed to allocate %i bytes.\n", size);
             return FALSE;
         }
         // The user memory is freed after the Acknowledgment or at Termination,
         // but it can already be gone by the time we get here.
         Status = STATUS_SUCCESS;
         _SEH2_TRY
         {
             ProbeForRead(userBuf, size, 1);
             RtlCopyMemory(Buffer, userBuf, size);
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
             ERR("DDE Post: invalid user buffer 0x%p, size %d\n", userBuf, size);
             Status = _SEH2_GetExceptionCode();
         }
         _SEH2_END;

         if (!NT_SUCCESS(Status))
         {
             ExFreePoolWithTag(Buffer, USERTAG_DDE);
             return FALSE;
         }
      }

      TRACE("DDE Post size %d 0x%x\n",size, Msg);

      switch(Msg)
      {
          case WM_DDE_POKE:
          {
              DDEPOKE *pddePoke = Buffer;
              /* When the callback returned -1 there is no buffer at all, and a
                 buffer that is too small for the header must not be read either.
                 NT_ASSERT is compiled out in release builds, so the old code
                 dereferenced NULL here. */
              if (pddePoke == NULL ||
                  (ULONG)size < FIELD_OFFSET(DDEPOKE, Value) + sizeof(HGDIOBJ))
              {
                  ERR("DDE POKE: buffer too small or absent (%d)\n", size);
                  break;
              }
              switch(pddePoke->cfFormat)
              {
                 case CF_BITMAP:
                 case CF_DIB:
                 case CF_PALETTE:
                    RtlCopyMemory(&Object, pddePoke->Value, sizeof(HGDIOBJ));
                    break;
                 default:
                    break;
              }
              break;
          }
          case WM_DDE_DATA:
          {
              DDEDATA *pddeData2 = Buffer;
              if (pddeData2 == NULL ||
                  (ULONG)size < FIELD_OFFSET(DDEDATA, Value) + sizeof(HGDIOBJ))
              {
                  ERR("DDE DATA: buffer too small or absent (%d)\n", size);
                  break;
              }
              switch(pddeData2->cfFormat)
              {
                 case CF_BITMAP:
                 case CF_DIB:
                 case CF_PALETTE:
                    RtlCopyMemory(&Object, pddeData2->Value, sizeof(HGDIOBJ));
                    break;
                 default:
                    break;
              }
              break;
          }
          default:
              break;
      }

      if (Object)
      {
         // Give gdi object to the other process.
         GreSetObjectOwner(Object, pWnd->head.pti->ppi->W32Pid);
      }

      pddeData = ExAllocatePoolWithTag(PagedPool, sizeof(DDE_DATA), USERTAG_DDE5);
      if (pddeData == NULL)
      {
         ERR("Failed to allocate DDE_DATA\n");
         ExFreePoolWithTag(Buffer, USERTAG_DDE);
         return FALSE;
      }

      pddeData->cbSize       = size;
      pddeData->pvBuffer     = Buffer;
      pddeData->lParam       = lp;

      TRACE("DDE Post lParam c=%08lx\n",lp);
      *lParam = lp;

      // Attach this data packet to the user message.
      *ExtraInfo = (LONG_PTR)pddeData;
   }
   return TRUE;
}

//
//  DDE Get/Peek message hook, take preprocessed information and recombined it for the current Process Thread.
//
BOOL APIENTRY
IntDdeGetMessageHook(PMSG pMsg, LONG_PTR ExtraInfo)
{
   PWND pWnd, pWndClient;
   PDDE_DATA pddeData;
   PDDE_PROP pddeProp;
   BOOL Ret;

   pWnd = UserGetWindowObject(pMsg->hwnd);
   if (pWnd == NULL)
   {
      ERR("DDE Get Window is dead. %p\n", pMsg->hwnd);
      return TRUE;
   }

   if (pMsg->message == WM_DDE_TERMINATE)
   {
      pddeProp = (PDDE_PROP)UserGetProp(pWnd, AtomDDETrack, TRUE);
      if (pddeProp)
      {
         pWndClient = UserGetWindowObject((HWND)pMsg->wParam);
         if (pWndClient == NULL)
         {
            ERR("DDE Get Client WM_DDE_TERMINATE\n");
         }

         UserRemoveProp(pWnd, AtomDDETrack, TRUE);
         ExFreePoolWithTag(pddeProp, USERTAG_DDE1);
      }
      return TRUE;
   }

   TRACE("DDE Get Msg 0x%x\n",pMsg->message);

   pddeData = (PDDE_DATA)ExtraInfo;

   if ( pddeData )
   {
      TRACE("DDE Get size %d lParam c=%08lx lp c=%08lx\n",pddeData->cbSize, pMsg->lParam, pddeData->lParam);

      // Callback.
      Ret = IntDDEGetCallback( pWnd, pMsg, pddeData->pvBuffer, pddeData->cbSize);
      if (!Ret)
      {
         ERR("DDE Get CB failed\n");
      }

      if (pddeData->pvBuffer) ExFreePoolWithTag(pddeData->pvBuffer, USERTAG_DDE);

      ExFreePoolWithTag(pddeData, USERTAG_DDE5);

      return Ret;
   }
   TRACE("DDE Get No DDE Data found!\n");
   return TRUE;
}

//
//  DDE Send message hook, intercept DDE messages and associate them in a partnership with property.
//
BOOL FASTCALL
IntDdeSendMessageHook(PWND pWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
   PWND pWndServer;
   PDDE_PROP pddeProp;

   if (pWnd->head.pti->ppi != gptiCurrent->ppi)
   {
      TRACE("Sending long DDE 0x%x\n",Msg);

      // Allow only Acknowledge and Initiate to be sent across borders.
      if (Msg != WM_DDE_ACK )
      {
         if (Msg == WM_DDE_INITIATE) return TRUE;
         return FALSE;
      }

      TRACE("Sending long WM_DDE_ACK\n");

      pWndServer = UserGetWindowObject((HWND)wParam);
      if (pWndServer == NULL)
      {
         ERR("Invalid DDE Server Window handle\n");
         return FALSE;
      }

      // Setup property so this conversation can be tracked.
      pddeProp = ExAllocatePoolWithTag(PagedPool, sizeof(DDE_PROP), USERTAG_DDE1);
      if (pddeProp == NULL)
      {
         ERR("failed to allocate DDE_PROP\n");
         return FALSE;
      }

      pddeProp->spwnd        = pWndServer;
      pddeProp->spwndPartner = pWnd;

      UserSetProp(pWndServer, AtomDDETrack, (HANDLE)pddeProp, TRUE);
   }
   return TRUE;
}


BOOL
APIENTRY
NtUserDdeGetQualityOfService(
   IN HWND hwndClient,
   IN HWND hWndServer,
   OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
   STUB

   return 0;
}

BOOL
APIENTRY
NtUserDdeSetQualityOfService(
   IN  HWND hwndClient,
   IN  PSECURITY_QUALITY_OF_SERVICE pqosNew,
   OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
   STUB

   return 0;
}

BOOL
APIENTRY
NtUserImpersonateDdeClientWindow(
   HWND hWndClient,
   HWND hWndServer)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserDdeInitialize(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4)
{
   STUB

   return 0;
}

