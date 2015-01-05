
#include <win32k.h>

#include <dde.h>

DBG_DEFAULT_CHANNEL(UserMisc);


BOOL FASTCALL
IntDdeSendMessageHook(PWND pWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
   PWND pWndServer;

   if (pWnd->head.pti->ppi != gptiCurrent->ppi)
   {
      // Allow only Acknowledge and Initiate to be sent across borders.
      if (Msg != WM_DDE_ACK )
      {
         if (Msg == WM_DDE_INITIATE) return TRUE;
         return FALSE;
      }

      pWndServer = UserGetWindowObject((HWND)wParam);
      if (pWndServer == NULL)
      {
         ERR("Invalid DDE Server Window handle\n");
         return FALSE;
      }
      ERR("Sending DDE 0x%x\n",Msg);
   }
   return TRUE;
}

BOOL FASTCALL
IntDdePostMessageHook(PWND pWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
   PWND pWndClient;

   if (pWnd->head.pti->ppi != gptiCurrent->ppi)
   {
      // Initiate is sent only across borders.
      if (Msg == WM_DDE_INITIATE)
      {
         return FALSE;
      }

      pWndClient = UserGetWindowObject((HWND)wParam);
      if (pWndClient == NULL)
      {
          // This is terminating so post it.
          if ( Msg == WM_DDE_TERMINATE) return TRUE;
          ERR("Invalid DDE Client Window handle\n");
          return FALSE;
      }
      ERR("Posting and do CB DDE 0x%x\n",Msg);
   }
   return TRUE;
}

VOID FASTCALL
IntDdeGetMessageHook(PMSG pMsg)
{
   if (pMsg->message == WM_DDE_TERMINATE)
   {
      return;
   }
   ERR("Do Callback Msg 0x%x\n",pMsg->message);
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

