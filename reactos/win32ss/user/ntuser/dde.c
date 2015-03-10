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

   RtlCopyMemory(Common, ResultPointer, ArgumentLength);

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

   RtlMoveMemory(Common, ResultPointer, ArgumentLength);

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
         // Set buffer with users data size.
         Buffer = ExAllocatePoolWithTag(PagedPool, size, USERTAG_DDE);
         if (Buffer == NULL)
         {
             ERR("Failed to allocate %i bytes.\n", size);
             return FALSE;
         }
         // No SEH? Yes, the user memory is freed after the Acknowledgment or at Termination.
         RtlCopyMemory(Buffer, userBuf, size);
      }

      TRACE("DDE Post size %d 0x%x\n",size, Msg);

      switch(Msg)
      {
          case WM_DDE_POKE:
          {
              DDEPOKE *pddePoke = Buffer;
              NT_ASSERT(pddePoke != NULL);
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
              NT_ASSERT(pddeData2 != NULL);
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
      pddeProp = (PDDE_PROP)UserGetProp(pWnd, AtomDDETrack);
      if (pddeProp)
      {
         pWndClient = UserGetWindowObject((HWND)pMsg->wParam);
         if (pWndClient == NULL)
         {
            ERR("DDE Get Client WM_DDE_TERMINATE\n");
         }

         IntRemoveProp(pWnd, AtomDDETrack);
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

      IntSetProp(pWndServer, AtomDDETrack, (HANDLE)pddeProp);
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

