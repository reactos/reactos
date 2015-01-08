
#include <win32k.h>

#include <dde.h>

DBG_DEFAULT_CHANNEL(UserMisc);

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
} DDE_PROP, *PDDE_PROP;



int
APIENTRY
IntDDEPostCallback(
   IN PWND pWnd,
   IN UINT Msg,
   IN WPARAM wParam,
   IN OUT LPARAM *lParam,
   IN PVOID Buffer,
   IN int size)
{
   NTSTATUS Status;
   ULONG ArgumentLength, ResultLength;
   PVOID Argument, ResultPointer;
   PDDEPOSTGET_CALLBACK_ARGUMENTS Common;
   int origSize = size;

   ResultPointer = NULL;
   ResultLength = ArgumentLength = sizeof(DDEPOSTGET_CALLBACK_ARGUMENTS)+size;

   Argument = IntCbAllocateMemory(ArgumentLength);
   if (NULL == Argument)
   {
      return FALSE;
   }

   Common = (PDDEPOSTGET_CALLBACK_ARGUMENTS) Argument;

   Common->size    = size;
   Common->hwnd    = UserHMGetHandle(pWnd);
   Common->message = Msg;
   Common->wParam  = wParam;
   Common->lParam  = *lParam;
   RtlCopyMemory(&Common->buffer, Buffer, size);

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

   if (Common->size != 0 && size <= origSize)
   {
      RtlCopyMemory(Buffer, &Common->buffer, size); // ResultLength);
   }

   size    = Common->size;
   *lParam = Common->lParam;

   IntCbFreeMemory(Argument);

   return size ? size : -1;
}

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
   HGDIOBJ Object = NULL;
   PVOID Buffer = NULL;
   int size = 128;
   LPARAM lp = *lParam;

   if (pWnd->head.pti->ppi != gptiCurrent->ppi)
   {
      ERR("Posting long DDE 0x%x\n",Msg);
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
            ERR("DDE Posted WM_DDE_TERMINATE\n");
            return TRUE;
         }
         ERR("Invalid DDE Client Window handle\n");
         return FALSE;
      }

      if (Msg == WM_DDE_TERMINATE )
      {
         //// FIXME Remove Stuff if any...

         // Do not bother to callback.
         return TRUE;
      }

      Buffer = ExAllocatePoolWithTag(PagedPool, size, USERTAG_DDE);

      if ((size = IntDDEPostCallback(pWnd, Msg, wParam, &lp, Buffer, size)) == 0)
      {
         ERR("DDE Post Callback return 0 0x%x\n", Msg);
      }

      if (size != -1 && size > 128)
      {
         ERR("FIXME: DDE Post need more bytes %d\n",size);
      }

      if (size == -1)
      {
         size = 0;
         ExFreePoolWithTag(Buffer, USERTAG_DDE);
         Buffer = NULL;
      }

      ERR("DDE Post size %d 0x%x\n",size, Msg);

      switch(Msg)
      {
          case WM_DDE_POKE:
          {
              DDEPOKE *pddePoke = Buffer;
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
              DDEDATA *pddeData = Buffer;
              switch(pddeData->cfFormat)
              {
                 case CF_BITMAP:
                 case CF_DIB:
                 case CF_PALETTE:
                    RtlCopyMemory(&Object, pddeData->Value, sizeof(HGDIOBJ));
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

      pddeData = ExAllocatePoolWithTag(PagedPool, sizeof(DDE_DATA), USERTAG_DDE2);

      pddeData->cbSize       = size;
      pddeData->pvBuffer     = Buffer;
      pddeData->lParam       = lp;
 
      ERR("DDE Post lParam c=%08lx\n",lp);
      *lParam = lp;
 
      // Attach this data packet to the user message.
      *ExtraInfo = (LONG_PTR)pddeData;
   }
   return TRUE;
}

VOID APIENTRY
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
      return;
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
      return;
   }

   ERR("DDE Get Msg 0x%x\n",pMsg->message);

   pddeData = (PDDE_DATA)ExtraInfo;

   if ( pddeData )
   {
      ERR("DDE Get 1 size %d lParam c=%08lx lp c=%08lx\n",pddeData->cbSize, pMsg->lParam, pddeData->lParam);

      pMsg->lParam = pddeData->lParam; // This might be a hack... Need to backtrace lParam from post queue.

      Ret = IntDDEGetCallback( pWnd, pMsg, pddeData->pvBuffer, pddeData->cbSize);
      if (!Ret)
      {
         ERR("DDE Get CB failed\n");
      }

      ERR("DDE Get 2 size %d lParam c=%08lx\n",pddeData->cbSize, pMsg->lParam);

      if (pddeData->pvBuffer) ExFreePoolWithTag(pddeData->pvBuffer, USERTAG_DDE);

      ExFreePoolWithTag(pddeData, USERTAG_DDE2);

      return;
   }
   ERR("DDE Get No DDE Data found!\n");
   return;
}

BOOL FASTCALL
IntDdeSendMessageHook(PWND pWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
   PWND pWndServer;
   PDDE_PROP pddeProp;

   if (Msg == WM_DDE_ACK)
   {
      ERR("Sending WM_DDE_ACK Client hwnd %p\n",pWnd->head.h);
   }

   if (pWnd->head.pti->ppi != gptiCurrent->ppi)
   {
      ERR("Sending long DDE 0x%x\n",Msg);

      // Allow only Acknowledge and Initiate to be sent across borders.
      if (Msg != WM_DDE_ACK )
      {
         if (Msg == WM_DDE_INITIATE) return TRUE;
         return FALSE;
      }

      ERR("Sending long WM_DDE_ACK\n");

      pWndServer = UserGetWindowObject((HWND)wParam);
      if (pWndServer == NULL)
      {
         ERR("Invalid DDE Server Window handle\n");
         return FALSE;
      }

      // Setup property so this conversation can be tracked.
      pddeProp = ExAllocatePoolWithTag(PagedPool, sizeof(DDE_PROP), USERTAG_DDE1);

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

