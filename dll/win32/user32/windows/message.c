/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/message.c
 * PURPOSE:         Messages
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */

#include <user32.h>

#include <wine/debug.h>

/* DDE message exchange
 *
 * - Session initialization
 *   Client sends a WM_DDE_INITIATE message, usually a broadcast message. lParam of
 *   this message contains a pair of global atoms, the Application and Topic atoms.
 *   The client must destroy the atoms.
 *   Server window proc handles the WM_DDE_INITIATE message and if the Application
 *   and Topic atoms are recognized sends a WM_DDE_ACK message to the client. lParam
 *   of the reply message contains another pair of global atoms (Application and
 *   Topic again), which must be destroyed by the server.
 *
 * - Execute
 *   Client posts a WM_DDE_EXECUTE message to the server window. lParam of that message
 *   is a global memory handle containing the string to execute. After the command has
 *   been executed the server posts a WM_DDE_ACK message to the client, which contains
 *   a packed lParam which in turn contains that global memory handle. The client takes
 *   ownership of both the packed lParam (meaning it needs to call FreeDDElParam() on
 *   it and the global memory handle.
 *   This might work nice and easy in Win3.1, but things are more complicated for NT.
 *   Global memory handles in NT are not really global, they're still local to the
 *   process. So, what happens under the hood is that PostMessage must handle the
 *   WM_DDE_EXECUTE message specially. It will obtain the contents of the global memory
 *   area, repack that into a new structure together with the original memory handle
 *   and pass that off to the win32k. Win32k will marshall that data over to the target
 *   (server) process where it will be unpacked and stored in a newly allocated global
 *   memory area. The handle of that area will then be sent to the window proc, after
 *   storing it together with the "original" (client) handle in a table.
 *   The server will eventually post the WM_DDE_ACK response, containing the global
 *   memory handle it received. PostMessage must then lookup that memory handle (only
 *   valid in the server process) and replace it with the corresponding client memory
 *   handle. To avoid memory leaks, the server-side global memory block must be freed.
 *   Also, the WM_DDE_ACK lParam (a PackDDElParam() result) is unpacked and the
 *   individual components are handed to win32k.sys to post to the client side. Since
 *   the server side app hands over ownership of the packed lParam when it calls
 *   PostMessage(), the packed lParam needs to be freed on the server side too.
 *   When the WM_DDE_ACK message (containing the client-side global memory handle)
 *   arrives at the client side a new lParam is PackDDElParam()'ed and this is handed
 *   to the client side window proc which is expected to free/reuse it.
 */

/* since the WM_DDE_ACK response to a WM_DDE_EXECUTE message should contain the handle
 * to the memory handle, we keep track (in the server side) of all pairs of handle
 * used (the client passes its value and the content of the memory handle), and
 * the server stored both values (the client, and the local one, created after the
 * content). When a ACK message is generated, the list of pair is searched for a
 * matching pair, so that the client memory handle can be returned.
 */
typedef struct tagDDEPAIR
{
  HGLOBAL     ClientMem;
  HGLOBAL     ServerMem;
} DDEPAIR, *PDDEPAIR;

static PDDEPAIR DdePairs = NULL;
static unsigned DdeNumAlloc = 0;
static unsigned DdeNumUsed = 0;
static CRITICAL_SECTION DdeCrst;

static BOOL FASTCALL
DdeAddPair(HGLOBAL ClientMem, HGLOBAL ServerMem)
{
  unsigned i;

  EnterCriticalSection(&DdeCrst);

  /* now remember the pair of hMem on both sides */
  if (DdeNumUsed == DdeNumAlloc)
    {
#define GROWBY  4
      PDDEPAIR New;
      if (NULL != DdePairs)
        {
          New = HeapReAlloc(GetProcessHeap(), 0, DdePairs,
                            (DdeNumAlloc + GROWBY) * sizeof(DDEPAIR));
        }
      else
        {
          New = HeapAlloc(GetProcessHeap(), 0,
                          (DdeNumAlloc + GROWBY) * sizeof(DDEPAIR));
        }

      if (NULL == New)
        {
          LeaveCriticalSection(&DdeCrst);
          return FALSE;
        }
      DdePairs = New;
      /* zero out newly allocated part */
      memset(&DdePairs[DdeNumAlloc], 0, GROWBY * sizeof(DDEPAIR));
      DdeNumAlloc += GROWBY;
#undef GROWBY
    }

  for (i = 0; i < DdeNumAlloc; i++)
    {
      if (NULL == DdePairs[i].ServerMem)
        {
          DdePairs[i].ClientMem = ClientMem;
          DdePairs[i].ServerMem = ServerMem;
          DdeNumUsed++;
          break;
        }
    }
  LeaveCriticalSection(&DdeCrst);

  return TRUE;
}

static HGLOBAL FASTCALL
DdeGetPair(HGLOBAL ServerMem)
{
  unsigned i;
  HGLOBAL Ret = NULL;

  EnterCriticalSection(&DdeCrst);
  for (i = 0; i < DdeNumAlloc; i++)
    {
      if (DdePairs[i].ServerMem == ServerMem)
        {
          /* free this pair */
          DdePairs[i].ServerMem = 0;
          DdeNumUsed--;
          Ret = DdePairs[i].ClientMem;
          break;
        }
    }
  LeaveCriticalSection(&DdeCrst);

  return Ret;
}

static BOOL FASTCALL
MsgiUMToKMMessage(PMSG UMMsg, PMSG KMMsg, BOOL Posted)
{
  *KMMsg = *UMMsg;

  switch (UMMsg->message)
    {
      case WM_DDE_ACK:
        {
          PKMDDELPARAM DdeLparam;
          DdeLparam = HeapAlloc(GetProcessHeap(), 0, sizeof(KMDDELPARAM));
          if (NULL == DdeLparam)
            {
              return FALSE;
            }
          if (Posted)
            {
              DdeLparam->Packed = TRUE;
              if (! UnpackDDElParam(UMMsg->message, UMMsg->lParam,
                                    &DdeLparam->Value.Packed.uiLo,
                                    &DdeLparam->Value.Packed.uiHi))
                {
                  return FALSE;
                }
              if (0 != HIWORD(DdeLparam->Value.Packed.uiHi))
                {
                  /* uiHi should contain a hMem from WM_DDE_EXECUTE */
                  HGLOBAL h = DdeGetPair((HGLOBAL) DdeLparam->Value.Packed.uiHi);
                  if (NULL != h)
                    {
                      GlobalFree((HGLOBAL) DdeLparam->Value.Packed.uiHi);
                      DdeLparam->Value.Packed.uiHi = (UINT) h;
                    }
                }
              FreeDDElParam(UMMsg->message, UMMsg->lParam);
            }
          else
            {
              DdeLparam->Packed = FALSE;
              DdeLparam->Value.Unpacked = UMMsg->lParam;
            }
          KMMsg->lParam = (LPARAM) DdeLparam;
        }
        break;

      case WM_DDE_EXECUTE:
        {
          SIZE_T Size;
          PKMDDEEXECUTEDATA KMDdeExecuteData;
          PVOID Data;

          Size = GlobalSize((HGLOBAL) UMMsg->lParam);
          Data = GlobalLock((HGLOBAL) UMMsg->lParam);
          if (NULL == Data)
            {
              SetLastError(ERROR_INVALID_HANDLE);
              return FALSE;
            }
          KMDdeExecuteData = HeapAlloc(GetProcessHeap(), 0, sizeof(KMDDEEXECUTEDATA) + Size);
          if (NULL == KMDdeExecuteData)
            {
              SetLastError(ERROR_OUTOFMEMORY);
              return FALSE;
            }
          KMDdeExecuteData->Sender = (HWND) UMMsg->wParam;
          KMDdeExecuteData->ClientMem = (HGLOBAL) UMMsg->lParam;
          memcpy((PVOID) (KMDdeExecuteData + 1), Data, Size);
          KMMsg->wParam = sizeof(KMDDEEXECUTEDATA) + Size;
          KMMsg->lParam = (LPARAM) KMDdeExecuteData;
          GlobalUnlock((HGLOBAL) UMMsg->lParam);
        }
        break;

      case WM_COPYDATA:
        {
          PCOPYDATASTRUCT pUMCopyData = (PCOPYDATASTRUCT)UMMsg->lParam;
          PCOPYDATASTRUCT pKMCopyData;

          pKMCopyData = HeapAlloc(GetProcessHeap(), 0,
                                  sizeof(COPYDATASTRUCT) + pUMCopyData->cbData);
          if (pKMCopyData == NULL)
            {
              SetLastError(ERROR_OUTOFMEMORY);
              return FALSE;
            }

          pKMCopyData->dwData = pUMCopyData->dwData;
          pKMCopyData->cbData = pUMCopyData->cbData;
          pKMCopyData->lpData = pKMCopyData + 1;

          RtlCopyMemory(pKMCopyData + 1, pUMCopyData->lpData,
                        pUMCopyData->cbData);

          KMMsg->lParam = (LPARAM)pKMCopyData;
        }
        break;

      default:
        break;
    }

  return TRUE;
}

static VOID FASTCALL
MsgiUMToKMCleanup(PMSG UMMsg, PMSG KMMsg)
{
  switch (KMMsg->message)
    {
      case WM_DDE_ACK:
      case WM_DDE_EXECUTE:
      case WM_COPYDATA:
        HeapFree(GetProcessHeap(), 0, (LPVOID) KMMsg->lParam);
        break;
      default:
        break;
    }

  return;
}

static BOOL FASTCALL
MsgiUMToKMReply(PMSG UMMsg, PMSG KMMsg, LRESULT *Result)
{
  MsgiUMToKMCleanup(UMMsg, KMMsg);

  return TRUE;
}

static BOOL FASTCALL
MsgiKMToUMMessage(PMSG KMMsg, PMSG UMMsg)
{
  *UMMsg = *KMMsg;

  switch (UMMsg->message)
    {
      case WM_CREATE:
      case WM_NCCREATE:
        {
          CREATESTRUCTW *Cs = (CREATESTRUCTW *) KMMsg->lParam;
          PCHAR Class;
          Cs->lpszName = (LPCWSTR) ((PCHAR) Cs + (DWORD_PTR) Cs->lpszName);
          Class = (PCHAR) Cs + (DWORD_PTR) Cs->lpszClass;
          if (L'A' == *((WCHAR *) Class))
            {
              Class += sizeof(WCHAR);
              Cs->lpszClass = (LPCWSTR)(DWORD_PTR) (*((ATOM *) Class));
            }
          else
            {
              ASSERT(L'S' == *((WCHAR *) Class));
              Class += sizeof(WCHAR);
              Cs->lpszClass = (LPCWSTR) Class;
            }
        }
        break;

      case WM_DDE_ACK:
        {
          PKMDDELPARAM DdeLparam = (PKMDDELPARAM) KMMsg->lParam;
          if (DdeLparam->Packed)
            {
              UMMsg->lParam = PackDDElParam(KMMsg->message,
                                            DdeLparam->Value.Packed.uiLo,
                                            DdeLparam->Value.Packed.uiHi);
            }
          else
            {
              UMMsg->lParam = DdeLparam->Value.Unpacked;
            }
        }
        break;

      case WM_DDE_EXECUTE:
        {
          PKMDDEEXECUTEDATA KMDdeExecuteData;
          HGLOBAL GlobalData;
          PVOID Data;

          KMDdeExecuteData = (PKMDDEEXECUTEDATA) KMMsg->lParam;
          GlobalData = GlobalAlloc(GMEM_MOVEABLE, KMMsg->wParam - sizeof(KMDDEEXECUTEDATA));
          if (NULL == GlobalData)
            {
              return FALSE;
            }
          Data = GlobalLock(GlobalData);
          if (NULL == Data)
            {
              GlobalFree(GlobalData);
              return FALSE;
            }
          memcpy(Data, (PVOID) (KMDdeExecuteData + 1), KMMsg->wParam - sizeof(KMDDEEXECUTEDATA));
          GlobalUnlock(GlobalData);
          if (! DdeAddPair(KMDdeExecuteData->ClientMem, GlobalData))
            {
              GlobalFree(GlobalData);
              return FALSE;
            }
          UMMsg->wParam = (WPARAM) KMDdeExecuteData->Sender;
          UMMsg->lParam = (LPARAM) GlobalData;
        }
        break;

      case WM_COPYDATA:
        {
          PCOPYDATASTRUCT pKMCopyData = (PCOPYDATASTRUCT)KMMsg->lParam;
          pKMCopyData->lpData = pKMCopyData + 1;
        }
        break;

      default:
        break;
    }

  return TRUE;
}

static VOID FASTCALL
MsgiKMToUMCleanup(PMSG KMMsg, PMSG UMMsg)
{
  switch (KMMsg->message)
    {
      case WM_DDE_EXECUTE:
#ifdef TODO
        HeapFree(GetProcessHeap(), 0, (LPVOID) KMMsg->lParam);
        GlobalUnlock((HGLOBAL) UMMsg->lParam);
#endif
        break;
      default:
        break;
    }

  return;
}

static BOOL FASTCALL
MsgiKMToUMReply(PMSG KMMsg, PMSG UMMsg, LRESULT *Result)
{
  MsgiKMToUMCleanup(KMMsg, UMMsg);

  return TRUE;
}

static BOOL FASTCALL
MsgiAnsiToUnicodeMessage(LPMSG UnicodeMsg, LPMSG AnsiMsg)
{
  *UnicodeMsg = *AnsiMsg;
  switch (AnsiMsg->message)
    {
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
      {
        LPWSTR Buffer = HeapAlloc(GetProcessHeap(), 0,
           AnsiMsg->wParam * sizeof(WCHAR));
        if (!Buffer)
          {
            return FALSE;
          }
        UnicodeMsg->lParam = (LPARAM)Buffer;
        break;
      }

    /* AnsiMsg->lParam is string (0-terminated) */
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
      {
        goto ConvertLParamString;
      }

    case LB_ADDSTRING:
    case LB_ADDSTRING_LOWER:
    case LB_ADDSTRING_UPPER:
    case LB_INSERTSTRING:
    case LB_INSERTSTRING_UPPER:
    case LB_INSERTSTRING_LOWER:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
      {
        DWORD dwStyle = GetWindowLongW(AnsiMsg->hwnd, GWL_STYLE);
        if (!(dwStyle & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) &&
            (dwStyle & LBS_HASSTRINGS))
          {
            goto ConvertLParamString;
          }
        break;
      }

    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
      {
        DWORD dwStyle = GetWindowLongW(AnsiMsg->hwnd, GWL_STYLE);
        if (!(dwStyle & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) &&
            (dwStyle & CBS_HASSTRINGS))
          {
            UNICODE_STRING UnicodeString;

ConvertLParamString:
            RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)AnsiMsg->lParam);
            UnicodeMsg->lParam = (LPARAM)UnicodeString.Buffer;
          }
        break;
      }

    case WM_NCCREATE:
    case WM_CREATE:
      {
        UNICODE_STRING UnicodeBuffer;
        struct s
        {
           CREATESTRUCTW cs;    /* new structure */
           LPCWSTR lpszName;    /* allocated Name */
           LPCWSTR lpszClass;   /* allocated Class */
        };
        struct s *xs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct s));
        if (!xs)
          {
            return FALSE;
          }
        xs->cs = *(CREATESTRUCTW *)AnsiMsg->lParam;
        if (!IS_INTRESOURCE(xs->cs.lpszName))
          {
            RtlCreateUnicodeStringFromAsciiz(&UnicodeBuffer, (LPSTR)xs->cs.lpszName);
            xs->lpszName = xs->cs.lpszName = UnicodeBuffer.Buffer;
          }
        if (!IS_ATOM(xs->cs.lpszClass))
          {
            RtlCreateUnicodeStringFromAsciiz(&UnicodeBuffer, (LPSTR)xs->cs.lpszClass);
            xs->lpszClass = xs->cs.lpszClass = UnicodeBuffer.Buffer;
          }
        UnicodeMsg->lParam = (LPARAM)xs;
        break;
      }

    case WM_MDICREATE:
      {
        UNICODE_STRING UnicodeBuffer;
        MDICREATESTRUCTW *cs =
            (MDICREATESTRUCTW *)HeapAlloc(GetProcessHeap(), 0, sizeof(*cs));

        if (!cs)
          {
            return FALSE;
          }

        *cs = *(MDICREATESTRUCTW *)AnsiMsg->lParam;

        if (!IS_ATOM(cs->szClass))
          {
            RtlCreateUnicodeStringFromAsciiz(&UnicodeBuffer, (LPSTR)cs->szClass);
            cs->szClass = UnicodeBuffer.Buffer;
          }

        RtlCreateUnicodeStringFromAsciiz(&UnicodeBuffer, (LPSTR)cs->szTitle);
        cs->szTitle = UnicodeBuffer.Buffer;

        UnicodeMsg->lParam = (LPARAM)cs;
        break;
      }
    }

  return TRUE;
}


static BOOL FASTCALL
MsgiAnsiToUnicodeCleanup(LPMSG UnicodeMsg, LPMSG AnsiMsg)
{
  switch (AnsiMsg->message)
    {
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
      {
        HeapFree(GetProcessHeap(), 0, (PVOID) UnicodeMsg->lParam);
        break;
      }

    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
      {
        goto FreeLParamString;
      }

    case LB_ADDSTRING:
    case LB_ADDSTRING_LOWER:
    case LB_ADDSTRING_UPPER:
    case LB_INSERTSTRING:
    case LB_INSERTSTRING_UPPER:
    case LB_INSERTSTRING_LOWER:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
      {
        DWORD dwStyle = GetWindowLongW(AnsiMsg->hwnd, GWL_STYLE);
        if (!(dwStyle & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) &&
            (dwStyle & LBS_HASSTRINGS))
          {
            goto FreeLParamString;
          }
        break;
      }

    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
      {
        DWORD dwStyle = GetWindowLongW(AnsiMsg->hwnd, GWL_STYLE);
        if (!(dwStyle & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) &&
            (dwStyle & CBS_HASSTRINGS))
          {
            UNICODE_STRING UnicodeString;

FreeLParamString:
            RtlInitUnicodeString(&UnicodeString, (PCWSTR)UnicodeMsg->lParam);
            RtlFreeUnicodeString(&UnicodeString);
          }
        break;
      }


    case WM_NCCREATE:
    case WM_CREATE:
      {
	UNICODE_STRING UnicodeString;
        struct s
        {
           CREATESTRUCTW cs;	/* new structure */
           LPWSTR lpszName;	/* allocated Name */
           LPWSTR lpszClass;	/* allocated Class */
        };
        struct s *xs = (struct s *)UnicodeMsg->lParam;
        if (xs->lpszName)
          {
            RtlInitUnicodeString(&UnicodeString, (PCWSTR)xs->lpszName);
            RtlFreeUnicodeString(&UnicodeString);
          }
        if (xs->lpszClass)
          {
            RtlInitUnicodeString(&UnicodeString, (PCWSTR)xs->lpszClass);
            RtlFreeUnicodeString(&UnicodeString);
          }
        HeapFree(GetProcessHeap(), 0, xs);
      }
      break;

    case WM_MDICREATE:
      {
	UNICODE_STRING UnicodeString;
        MDICREATESTRUCTW *cs = (MDICREATESTRUCTW *)UnicodeMsg->lParam;
        RtlInitUnicodeString(&UnicodeString, (PCWSTR)cs->szTitle);
        RtlFreeUnicodeString(&UnicodeString);
        if (!IS_ATOM(cs->szClass))
          {
            RtlInitUnicodeString(&UnicodeString, (PCWSTR)cs->szClass);
            RtlFreeUnicodeString(&UnicodeString);
          }
        HeapFree(GetProcessHeap(), 0, cs);
      }
      break;
    }
  return(TRUE);
}


static BOOL FASTCALL
MsgiAnsiToUnicodeReply(LPMSG UnicodeMsg, LPMSG AnsiMsg, LRESULT *Result)
{
  switch (AnsiMsg->message)
    {
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
      {
        LPWSTR Buffer = (LPWSTR)UnicodeMsg->lParam;
        LPSTR AnsiBuffer = (LPSTR)AnsiMsg->lParam;
        if (UnicodeMsg->wParam > 0 &&
            !WideCharToMultiByte(CP_ACP, 0, Buffer, -1,
            AnsiBuffer, UnicodeMsg->wParam, NULL, NULL))
          {
            AnsiBuffer[UnicodeMsg->wParam - 1] = 0;
          }
        break;
      }
    }

  MsgiAnsiToUnicodeCleanup(UnicodeMsg, AnsiMsg);

  return TRUE;
}


static BOOL FASTCALL
MsgiUnicodeToAnsiMessage(LPMSG AnsiMsg, LPMSG UnicodeMsg)
{
  *AnsiMsg = *UnicodeMsg;

  switch(UnicodeMsg->message)
    {
      case WM_CREATE:
      case WM_NCCREATE:
        {
          CREATESTRUCTA* CsA;
          CREATESTRUCTW* CsW;
          UNICODE_STRING UString;
          ANSI_STRING AString;
          NTSTATUS Status;

          CsW = (CREATESTRUCTW*)(UnicodeMsg->lParam);
          CsA = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(CREATESTRUCTA));
          if (NULL == CsA)
            {
              return FALSE;
            }
          memcpy(CsA, CsW, sizeof(CREATESTRUCTW));

          RtlInitUnicodeString(&UString, CsW->lpszName);
          Status = RtlUnicodeStringToAnsiString(&AString, &UString, TRUE);
          if (! NT_SUCCESS(Status))
            {
              RtlFreeHeap(GetProcessHeap(), 0, CsA);
              return FALSE;
            }
          CsA->lpszName = AString.Buffer;
          if (HIWORD((ULONG)CsW->lpszClass) != 0)
            {
              RtlInitUnicodeString(&UString, CsW->lpszClass);
              Status = RtlUnicodeStringToAnsiString(&AString, &UString, TRUE);
              if (! NT_SUCCESS(Status))
                {
                  RtlInitAnsiString(&AString, CsA->lpszName);
                  RtlFreeAnsiString(&AString);
                  RtlFreeHeap(GetProcessHeap(), 0, CsA);
                  return FALSE;
                }
              CsA->lpszClass = AString.Buffer;
            }
          AnsiMsg->lParam = (LPARAM)CsA;
          break;
        }
      case WM_GETTEXT:
        {
          /* Ansi string might contain MBCS chars so we need 2 * the number of chars */
          AnsiMsg->wParam = UnicodeMsg->wParam * 2;
          AnsiMsg->lParam = (LPARAM) RtlAllocateHeap(GetProcessHeap(), 0, AnsiMsg->wParam);
          if (NULL == (PVOID) AnsiMsg->lParam)
            {
              return FALSE;
            }
          break;
        }
      case WM_SETTEXT:
      case CB_DIR:
      case LB_DIR:
      case LB_ADDFILE:
        {
          goto ConvertLParamString;
	}

      case LB_ADDSTRING:
      case LB_ADDSTRING_LOWER:
      case LB_ADDSTRING_UPPER:
      case LB_INSERTSTRING:
      case LB_INSERTSTRING_UPPER:
      case LB_INSERTSTRING_LOWER:
      case LB_FINDSTRING:
      case LB_FINDSTRINGEXACT:
      case LB_SELECTSTRING:
        {
          DWORD dwStyle = GetWindowLongW(AnsiMsg->hwnd, GWL_STYLE);
          if (!(dwStyle & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) &&
              (dwStyle & LBS_HASSTRINGS))
            {
              goto ConvertLParamString;
            }
          break;
        }

      case CB_ADDSTRING:
      case CB_INSERTSTRING:
      case CB_FINDSTRING:
      case CB_FINDSTRINGEXACT:
      case CB_SELECTSTRING:
        {
          DWORD dwStyle = GetWindowLongW(AnsiMsg->hwnd, GWL_STYLE);
          if (!(dwStyle & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) &&
               (dwStyle & CBS_HASSTRINGS))
            {
              ANSI_STRING AnsiString;
              UNICODE_STRING UnicodeString;

ConvertLParamString:
              RtlInitUnicodeString(&UnicodeString, (PWSTR) UnicodeMsg->lParam);
              if (! NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiString,
                                                            &UnicodeString,
                                                            TRUE)))
                {
                  return FALSE;
                }
              AnsiMsg->lParam = (LPARAM) AnsiString.Buffer;
            }
          break;
        }

      case WM_MDICREATE:
        {
          ANSI_STRING AnsiBuffer;
          UNICODE_STRING UnicodeString;
          MDICREATESTRUCTA *cs =
              (MDICREATESTRUCTA *)HeapAlloc(GetProcessHeap(), 0, sizeof(*cs));

          if (!cs)
            {
              return FALSE;
            }

          *cs = *(MDICREATESTRUCTA *)UnicodeMsg->lParam;

          if (!IS_ATOM(cs->szClass))
            {
              RtlInitUnicodeString(&UnicodeString, (LPCWSTR)cs->szClass);
              if (! NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiBuffer,
                                                            &UnicodeString,
                                                            TRUE)))
                {
                  return FALSE;
                }
              cs->szClass = AnsiBuffer.Buffer;
            }

          RtlInitUnicodeString(&UnicodeString, (LPCWSTR)cs->szTitle);
          if (! NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiBuffer,
                                                        &UnicodeString,
                                                        TRUE)))
            {
              if (!IS_ATOM(cs->szClass))
                {
                  RtlInitAnsiString(&AnsiBuffer, cs->szClass);
                  RtlFreeAnsiString(&AnsiBuffer);
                }
              return FALSE;
            }
          cs->szTitle = AnsiBuffer.Buffer;

          AnsiMsg->lParam = (LPARAM)cs;
          break;
        }
    }

  return TRUE;
}


static BOOL FASTCALL
MsgiUnicodeToAnsiCleanup(LPMSG AnsiMsg, LPMSG UnicodeMsg)
{
  switch(UnicodeMsg->message)
    {
      case WM_GETTEXT:
        {
          RtlFreeHeap(GetProcessHeap(), 0, (PVOID) AnsiMsg->lParam);
          break;
        }
      case WM_SETTEXT:
        {
          goto FreeLParamString;
        }
      case WM_CREATE:
      case WM_NCCREATE:
        {
          CREATESTRUCTA* Cs;
          ANSI_STRING AString;

          Cs = (CREATESTRUCTA*) AnsiMsg->lParam;
          RtlInitAnsiString(&AString, Cs->lpszName);
          RtlFreeAnsiString(&AString);
          if (HIWORD((ULONG)Cs->lpszClass) != 0)
            {
              RtlInitAnsiString(&AString, Cs->lpszClass);
              RtlFreeAnsiString(&AString);
            }
          RtlFreeHeap(GetProcessHeap(), 0, Cs);
          break;
        }

      case LB_ADDSTRING:
      case LB_ADDSTRING_LOWER:
      case LB_ADDSTRING_UPPER:
      case LB_INSERTSTRING:
      case LB_INSERTSTRING_UPPER:
      case LB_INSERTSTRING_LOWER:
      case LB_FINDSTRING:
      case LB_FINDSTRINGEXACT:
      case LB_SELECTSTRING:
        {
          DWORD dwStyle = GetWindowLongW(AnsiMsg->hwnd, GWL_STYLE);
          if (!(dwStyle & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) &&
              (dwStyle & LBS_HASSTRINGS))
            {
              goto FreeLParamString;
            }
          break;
        }

      case CB_ADDSTRING:
      case CB_INSERTSTRING:
      case CB_FINDSTRING:
      case CB_FINDSTRINGEXACT:
      case CB_SELECTSTRING:
        {
          DWORD dwStyle = GetWindowLongW(AnsiMsg->hwnd, GWL_STYLE);
          if (!(dwStyle & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) &&
               (dwStyle & CBS_HASSTRINGS))
            {
              ANSI_STRING AString;

FreeLParamString:
              RtlInitAnsiString(&AString, (PSTR) AnsiMsg->lParam);
              RtlFreeAnsiString(&AString);
            }
          break;
        }

      case WM_MDICREATE:
        {
          ANSI_STRING AnsiString;
          MDICREATESTRUCTA *cs = (MDICREATESTRUCTA *)AnsiMsg->lParam;
          RtlInitAnsiString(&AnsiString, (PCSTR)cs->szTitle);
          RtlFreeAnsiString(&AnsiString);
          if (!IS_ATOM(cs->szClass))
            {
              RtlInitAnsiString(&AnsiString, (PCSTR)cs->szClass);
              RtlFreeAnsiString(&AnsiString);
            }
          HeapFree(GetProcessHeap(), 0, cs);
          break;
        }

    }

  return TRUE;
}


static BOOL FASTCALL
MsgiUnicodeToAnsiReply(LPMSG AnsiMsg, LPMSG UnicodeMsg, LRESULT *Result)
{
  switch (UnicodeMsg->message)
    {
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
      {
        LPSTR Buffer = (LPSTR) AnsiMsg->lParam;
        LPWSTR UBuffer = (LPWSTR) UnicodeMsg->lParam;
        if (0 < AnsiMsg->wParam &&
            ! MultiByteToWideChar(CP_ACP, 0, Buffer, -1, UBuffer, UnicodeMsg->wParam))
          {
            UBuffer[UnicodeMsg->wParam - 1] = L'\0';
          }
        break;
      }
    }

  MsgiUnicodeToAnsiCleanup(AnsiMsg, UnicodeMsg);

  return TRUE;
}

typedef struct tagMSGCONVERSION
{
  BOOL InUse;
  BOOL Ansi;
  MSG KMMsg;
  MSG UnicodeMsg;
  MSG AnsiMsg;
  PMSG FinalMsg;
  ULONG LParamSize;
} MSGCONVERSION, *PMSGCONVERSION;

static PMSGCONVERSION MsgConversions = NULL;
static unsigned MsgConversionNumAlloc = 0;
static unsigned MsgConversionNumUsed = 0;
static CRITICAL_SECTION MsgConversionCrst;

static BOOL FASTCALL
MsgConversionAdd(PMSGCONVERSION Conversion)
{
  unsigned i;

  EnterCriticalSection(&MsgConversionCrst);

  if (MsgConversionNumUsed == MsgConversionNumAlloc)
    {
#define GROWBY  4
      PMSGCONVERSION New;
      if (NULL != MsgConversions)
        {
          New = HeapReAlloc(GetProcessHeap(), 0, MsgConversions,
                            (MsgConversionNumAlloc + GROWBY) * sizeof(MSGCONVERSION));
        }
      else
        {
          New = HeapAlloc(GetProcessHeap(), 0,
                          (MsgConversionNumAlloc + GROWBY) * sizeof(MSGCONVERSION));
        }

      if (NULL == New)
        {
          LeaveCriticalSection(&MsgConversionCrst);
          return FALSE;
        }
      MsgConversions = New;
      /* zero out newly allocated part */
      memset(MsgConversions + MsgConversionNumAlloc, 0, GROWBY * sizeof(MSGCONVERSION));
      MsgConversionNumAlloc += GROWBY;
#undef GROWBY
    }

  for (i = 0; i < MsgConversionNumAlloc; i++)
    {
      if (! MsgConversions[i].InUse)
        {
          MsgConversions[i] = *Conversion;
          MsgConversions[i].InUse = TRUE;
          MsgConversionNumUsed++;
          break;
        }
    }
  LeaveCriticalSection(&MsgConversionCrst);

  return TRUE;
}

static void FASTCALL
MsgConversionCleanup(CONST MSG *Msg, BOOL Ansi, BOOL CheckMsgContents, LRESULT *Result)
{
  BOOL Found;
  PMSGCONVERSION Conversion;
  LRESULT Dummy;

  EnterCriticalSection(&MsgConversionCrst);
  for (Conversion = MsgConversions;
       Conversion < MsgConversions + MsgConversionNumAlloc;
       Conversion++)
    {
      if (Conversion->InUse &&
          ((Ansi && Conversion->Ansi) ||
           (! Ansi && ! Conversion->Ansi)))
        {
          Found = (Conversion->FinalMsg == Msg);
          if (! Found && CheckMsgContents)
            {
              if (Ansi)
                {
                  Found = (0 == memcmp(Msg, &Conversion->AnsiMsg, sizeof(MSG)));
                }
              else
                {
                  Found = (0 == memcmp(Msg, &Conversion->UnicodeMsg, sizeof(MSG)));
                }
            }
          if (Found)
            {
              if (Ansi)
                {
                  MsgiUnicodeToAnsiReply(&Conversion->AnsiMsg, &Conversion->UnicodeMsg,
                                         NULL == Result ? &Dummy : Result);
                }
              MsgiKMToUMReply(&Conversion->KMMsg, &Conversion->UnicodeMsg,
                              NULL == Result ? &Dummy : Result);
              if (0 != Conversion->LParamSize)
                {
                  NtFreeVirtualMemory(NtCurrentProcess(), (PVOID *) &Conversion->KMMsg.lParam,
                                      &Conversion->LParamSize, MEM_DECOMMIT);
                }
              Conversion->InUse = FALSE;
              MsgConversionNumUsed--;
            }
        }
    }
  LeaveCriticalSection(&MsgConversionCrst);
}

/*
 * @implemented
 */
LPARAM
STDCALL
GetMessageExtraInfo(VOID)
{
  return (LPARAM)NtUserCallNoParam(NOPARAM_ROUTINE_GETMESSAGEEXTRAINFO);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetMessagePos(VOID)
{
  PUSER32_THREAD_DATA ThreadData = User32GetThreadData();
  return(MAKELONG(ThreadData->LastMessage.pt.x, ThreadData->LastMessage.pt.y));
}


/*
 * @implemented
 */
LONG STDCALL
GetMessageTime(VOID)
{
  PUSER32_THREAD_DATA ThreadData = User32GetThreadData();
  return(ThreadData->LastMessage.time);
}


/*
 * @unimplemented
 */
BOOL
STDCALL
InSendMessage(VOID)
{
  static DWORD ShowNotImplemented = TRUE;
  if (ShowNotImplemented)
    {
      DbgPrint("InSendMessage is unimplemented\n");
      ShowNotImplemented = FALSE;
    }
  /* return(NtUserGetThreadState(THREADSTATE_INSENDMESSAGE) != ISMEX_NOSEND); */
  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
InSendMessageEx(
  LPVOID lpReserved)
{
  /* return NtUserGetThreadState(THREADSTATE_INSENDMESSAGE); */
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ReplyMessage(
  LRESULT lResult)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
LPARAM
STDCALL
SetMessageExtraInfo(
  LPARAM lParam)
{
  return NtUserSetMessageExtraInfo(lParam);
}

LRESULT FASTCALL
IntCallWindowProcW(BOOL IsAnsiProc,
                   WNDPROC WndProc,
                   HWND hWnd,
                   UINT Msg,
                   WPARAM wParam,
                   LPARAM lParam)
{
  MSG AnsiMsg;
  MSG UnicodeMsg;
  LRESULT Result;

  if (WndProc == NULL)
  {
      DPRINT("IntCallWindowsProcW() called with WndProc = NULL!\n");
      return FALSE;
  }

  if (IsAnsiProc)
    {
      UnicodeMsg.hwnd = hWnd;
      UnicodeMsg.message = Msg;
      UnicodeMsg.wParam = wParam;
      UnicodeMsg.lParam = lParam;
      if (! MsgiUnicodeToAnsiMessage(&AnsiMsg, &UnicodeMsg))
        {
          return FALSE;
        }
      Result = WndProc(AnsiMsg.hwnd, AnsiMsg.message, AnsiMsg.wParam, AnsiMsg.lParam);

      if (! MsgiUnicodeToAnsiReply(&AnsiMsg, &UnicodeMsg, &Result))
        {
          return FALSE;
        }
      return Result;
    }
  else
    {
      return WndProc(hWnd, Msg, wParam, lParam);
    }
}

static LRESULT FASTCALL
IntCallWindowProcA(BOOL IsAnsiProc,
                   WNDPROC WndProc,
                   HWND hWnd,
                   UINT Msg,
                   WPARAM wParam,
                   LPARAM lParam)
{
  MSG AnsiMsg;
  MSG UnicodeMsg;
  LRESULT Result;

  if (WndProc == NULL)
  {
      DPRINT1("IntCallWindowsProcA() called with WndProc = NULL!\n");
      return FALSE;
  }

  if (IsAnsiProc)
    {
      return WndProc(hWnd, Msg, wParam, lParam);
    }
  else
    {
      AnsiMsg.hwnd = hWnd;
      AnsiMsg.message = Msg;
      AnsiMsg.wParam = wParam;
      AnsiMsg.lParam = lParam;
      if (! MsgiAnsiToUnicodeMessage(&UnicodeMsg, &AnsiMsg))
        {
          return FALSE;
        }
      Result = WndProc(UnicodeMsg.hwnd, UnicodeMsg.message,
                       UnicodeMsg.wParam, UnicodeMsg.lParam);

      if (! MsgiAnsiToUnicodeReply(&UnicodeMsg, &AnsiMsg, &Result))
        {
          return FALSE;
        }
      return Result;
    }
}

static BOOL __inline
IsCallProcHandle(IN WNDPROC lpWndProc)
{
    /* FIXME - check for 64 bit architectures... */
    return ((ULONG_PTR)lpWndProc & 0xFFFF0000) == 0xFFFF0000;
}


/*
 * @implemented
 */
LRESULT STDCALL
CallWindowProcA(WNDPROC lpPrevWndFunc,
		HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam)
{
  WNDPROC_INFO wpInfo;

  /* FIXME - can the first parameter be NULL? */
  if (lpPrevWndFunc == NULL)
    lpPrevWndFunc = (WNDPROC)NtUserGetWindowLong(hWnd, GWLP_WNDPROC, TRUE);

  if (!IsCallProcHandle(lpPrevWndFunc) ||
      !NtUserDereferenceWndProcHandle((HANDLE)lpPrevWndFunc,
                                      &wpInfo))
    {
      return IntCallWindowProcA(TRUE, lpPrevWndFunc, hWnd, Msg, wParam, lParam);
    }
  else
    {
      return IntCallWindowProcA(!wpInfo.IsUnicode, wpInfo.WindowProc,
                                hWnd, Msg, wParam, lParam);
    }
}


/*
 * @implemented
 */
LRESULT STDCALL
CallWindowProcW(WNDPROC lpPrevWndFunc,
		HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam)
{
  WNDPROC_INFO wpInfo;

  /* FIXME - can the first parameter be NULL? */
  if (lpPrevWndFunc == NULL)
    lpPrevWndFunc = (WNDPROC)NtUserGetWindowLong(hWnd, GWLP_WNDPROC, FALSE);

  if (!IsCallProcHandle(lpPrevWndFunc) ||
      !NtUserDereferenceWndProcHandle((HANDLE)lpPrevWndFunc,
                                      &wpInfo))
    {
      return IntCallWindowProcW(FALSE, lpPrevWndFunc, hWnd, Msg, wParam, lParam);
    }
  else
    {
      return IntCallWindowProcW(!wpInfo.IsUnicode, wpInfo.WindowProc,
                                hWnd, Msg, wParam, lParam);
    }
}


/*
 * @implemented
 */
LRESULT STDCALL
DispatchMessageA(CONST MSG *lpmsg)
{
  NTUSERDISPATCHMESSAGEINFO Info;
  LRESULT Result;

  Info.Ansi = TRUE;
  Info.Msg = *lpmsg;
  Result = NtUserDispatchMessage(&Info);
  if (! Info.HandledByKernel)
    {
      /* We need to send the message ourselves */
      SPY_EnterMessage(SPY_DISPATCHMESSAGE, Info.Msg.hwnd, Info.Msg.message,
                       Info.Msg.wParam, Info.Msg.lParam);
      Result = IntCallWindowProcA(Info.Ansi, Info.Proc, Info.Msg.hwnd,
                                  Info.Msg.message, Info.Msg.wParam, Info.Msg.lParam);
      SPY_ExitMessage(SPY_RESULT_OK, Info.Msg.hwnd, Info.Msg.message, Result,
                      Info.Msg.wParam, Info.Msg.lParam);
    }
  MsgConversionCleanup(lpmsg, TRUE, TRUE, &Result);

  return Result;
}


/*
 * @implemented
 */
LRESULT STDCALL
DispatchMessageW(CONST MSG *lpmsg)
{
  NTUSERDISPATCHMESSAGEINFO Info;
  LRESULT Result;

  Info.Ansi = FALSE;
  Info.Msg = *lpmsg;
  Result = NtUserDispatchMessage(&Info);
  if (! Info.HandledByKernel)
    {
      /* We need to send the message ourselves */
      SPY_EnterMessage(SPY_DISPATCHMESSAGE, Info.Msg.hwnd, Info.Msg.message,
                       Info.Msg.wParam, Info.Msg.lParam);
      Result = IntCallWindowProcW(Info.Ansi, Info.Proc, Info.Msg.hwnd,
                                  Info.Msg.message, Info.Msg.wParam, Info.Msg.lParam);
      SPY_ExitMessage(SPY_RESULT_OK, Info.Msg.hwnd, Info.Msg.message, Result,
                      Info.Msg.wParam, Info.Msg.lParam);
    }
  MsgConversionCleanup(lpmsg, FALSE, TRUE, &Result);

  return Result;
}


/*
 * @implemented
 */
BOOL STDCALL
GetMessageA(LPMSG lpMsg,
	    HWND hWnd,
	    UINT wMsgFilterMin,
	    UINT wMsgFilterMax)
{
  BOOL Res;
  MSGCONVERSION Conversion;
  NTUSERGETMESSAGEINFO Info;
  PUSER32_THREAD_DATA ThreadData = User32GetThreadData();

  MsgConversionCleanup(lpMsg, TRUE, FALSE, NULL);
  Res = NtUserGetMessage(&Info, hWnd, wMsgFilterMin, wMsgFilterMax);
  if (-1 == (int) Res)
    {
      return Res;
    }
  Conversion.LParamSize = Info.LParamSize;
  Conversion.KMMsg = Info.Msg;

  if (! MsgiKMToUMMessage(&Conversion.KMMsg, &Conversion.UnicodeMsg))
    {
      return (BOOL) -1;
    }
  if (! MsgiUnicodeToAnsiMessage(&Conversion.AnsiMsg, &Conversion.UnicodeMsg))
    {
      MsgiKMToUMCleanup(&Info.Msg, &Conversion.UnicodeMsg);
      return (BOOL) -1;
    }
  *lpMsg = Conversion.AnsiMsg;
  Conversion.Ansi = TRUE;
  Conversion.FinalMsg = lpMsg;
  MsgConversionAdd(&Conversion);
  if (Res && lpMsg->message != WM_PAINT && lpMsg->message != WM_QUIT)
    {
      ThreadData->LastMessage = Info.Msg;
    }

  return Res;
}


/*
 * @implemented
 */
BOOL STDCALL
GetMessageW(LPMSG lpMsg,
	    HWND hWnd,
	    UINT wMsgFilterMin,
	    UINT wMsgFilterMax)
{
  BOOL Res;
  MSGCONVERSION Conversion;
  NTUSERGETMESSAGEINFO Info;
  PUSER32_THREAD_DATA ThreadData = User32GetThreadData();

  MsgConversionCleanup(lpMsg, FALSE, FALSE, NULL);
  Res = NtUserGetMessage(&Info, hWnd, wMsgFilterMin, wMsgFilterMax);
  if (-1 == (int) Res)
    {
      return Res;
    }
  Conversion.LParamSize = Info.LParamSize;
  Conversion.KMMsg = Info.Msg;

  if (! MsgiKMToUMMessage(&Conversion.KMMsg, &Conversion.UnicodeMsg))
    {
      return (BOOL) -1;
    }
  *lpMsg = Conversion.UnicodeMsg;
  Conversion.Ansi = FALSE;
  Conversion.FinalMsg = lpMsg;
  MsgConversionAdd(&Conversion);
  if (Res && lpMsg->message != WM_PAINT && lpMsg->message != WM_QUIT)
    {
      ThreadData->LastMessage = Info.Msg;
    }

  return Res;
}


/*
 * @implemented
 */
BOOL STDCALL
PeekMessageA(LPMSG lpMsg,
	     HWND hWnd,
	     UINT wMsgFilterMin,
	     UINT wMsgFilterMax,
	     UINT wRemoveMsg)
{
  BOOL Res;
  MSGCONVERSION Conversion;
  NTUSERGETMESSAGEINFO Info;
  PUSER32_THREAD_DATA ThreadData = User32GetThreadData();

  MsgConversionCleanup(lpMsg, TRUE, FALSE, NULL);
  Res = NtUserPeekMessage(&Info, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  if (-1 == (int) Res || ! Res)
    {
      return Res;
    }
  Conversion.LParamSize = Info.LParamSize;
  Conversion.KMMsg = Info.Msg;

  if (! MsgiKMToUMMessage(&Conversion.KMMsg, &Conversion.UnicodeMsg))
    {
      return (BOOL) -1;
    }
  if (! MsgiUnicodeToAnsiMessage(&Conversion.AnsiMsg, &Conversion.UnicodeMsg))
    {
      MsgiKMToUMCleanup(&Info.Msg, &Conversion.UnicodeMsg);
      return (BOOL) -1;
    }
  *lpMsg = Conversion.AnsiMsg;
  Conversion.Ansi = TRUE;
  Conversion.FinalMsg = lpMsg;
  MsgConversionAdd(&Conversion);
  if (Res && lpMsg->message != WM_PAINT && lpMsg->message != WM_QUIT)
    {
      ThreadData->LastMessage = Info.Msg;
    }

  return Res;
}


/*
 * @implemented
 */
BOOL
STDCALL
PeekMessageW(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg)
{
  BOOL Res;
  MSGCONVERSION Conversion;
  NTUSERGETMESSAGEINFO Info;
  PUSER32_THREAD_DATA ThreadData = User32GetThreadData();

  MsgConversionCleanup(lpMsg, FALSE, FALSE, NULL);
  Res = NtUserPeekMessage(&Info, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  if (-1 == (int) Res || ! Res)
    {
      return Res;
    }
  Conversion.LParamSize = Info.LParamSize;
  Conversion.KMMsg = Info.Msg;

  if (! MsgiKMToUMMessage(&Conversion.KMMsg, &Conversion.UnicodeMsg))
    {
      return (BOOL) -1;
    }
  *lpMsg = Conversion.UnicodeMsg;
  Conversion.Ansi = FALSE;
  Conversion.FinalMsg = lpMsg;
  MsgConversionAdd(&Conversion);
  if (Res && lpMsg->message != WM_PAINT && lpMsg->message != WM_QUIT)
    {
      ThreadData->LastMessage = Info.Msg;
    }

  return Res;
}


/*
 * @implemented
 */
BOOL
STDCALL
PostMessageA(
  HWND Wnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  MSG AnsiMsg, UcMsg;
  MSG KMMsg;
  LRESULT Result;

  AnsiMsg.hwnd = Wnd;
  AnsiMsg.message = Msg;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;
  if (! MsgiAnsiToUnicodeMessage(&UcMsg, &AnsiMsg))
    {
      return FALSE;
    }

  if (! MsgiUMToKMMessage(&UcMsg, &KMMsg, TRUE))
    {
      MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);
      return FALSE;
    }
  Result = NtUserPostMessage(KMMsg.hwnd, KMMsg.message,
                             KMMsg.wParam, KMMsg.lParam);
  MsgiUMToKMCleanup(&UcMsg, &KMMsg);
  MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);

  return Result;
}


/*
 * @implemented
 */
BOOL
STDCALL
PostMessageW(
  HWND Wnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  MSG UMMsg, KMMsg;
  LRESULT Result;

  UMMsg.hwnd = Wnd;
  UMMsg.message = Msg;
  UMMsg.wParam = wParam;
  UMMsg.lParam = lParam;
  if (! MsgiUMToKMMessage(&UMMsg, &KMMsg, TRUE))
    {
      return FALSE;
    }
  Result = NtUserPostMessage(KMMsg.hwnd, KMMsg.message,
                             KMMsg.wParam, KMMsg.lParam);
  MsgiUMToKMCleanup(&UMMsg, &KMMsg);

  return Result;
}


/*
 * @implemented
 */
VOID
STDCALL
PostQuitMessage(
  int nExitCode)
{
  (void) NtUserPostMessage(NULL, WM_QUIT, nExitCode, 0);
}


/*
 * @implemented
 */
BOOL
STDCALL
PostThreadMessageA(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return NtUserPostThreadMessage(idThread, Msg, wParam, lParam);
}


/*
 * @implemented
 */
BOOL
STDCALL
PostThreadMessageW(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return NtUserPostThreadMessage(idThread, Msg, wParam, lParam);
}


/*
 * @implemented
 */
LRESULT STDCALL
SendMessageW(HWND Wnd,
	     UINT Msg,
	     WPARAM wParam,
	     LPARAM lParam)
{
  MSG UMMsg, KMMsg;
  NTUSERSENDMESSAGEINFO Info;
  LRESULT Result;

  UMMsg.hwnd = Wnd;
  UMMsg.message = Msg;
  UMMsg.wParam = wParam;
  UMMsg.lParam = lParam;
  if (! MsgiUMToKMMessage(&UMMsg, &KMMsg, FALSE))
    {
      return FALSE;
    }
  Info.Ansi = FALSE;
  Result = NtUserSendMessage(KMMsg.hwnd, KMMsg.message,
                             KMMsg.wParam, KMMsg.lParam, &Info);
  if (! Info.HandledByKernel)
    {
      MsgiUMToKMCleanup(&UMMsg, &KMMsg);
      /* We need to send the message ourselves */
      Result = IntCallWindowProcW(Info.Ansi, Info.Proc, UMMsg.hwnd, UMMsg.message,
                                  UMMsg.wParam, UMMsg.lParam);
    }
  else if (! MsgiUMToKMReply(&UMMsg, &KMMsg, &Result))
    {
      return FALSE;
    }

  return Result;
}


/*
 * @implemented
 */
LRESULT STDCALL
SendMessageA(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  MSG AnsiMsg, UcMsg;
  MSG KMMsg;
  LRESULT Result;
  NTUSERSENDMESSAGEINFO Info;

  AnsiMsg.hwnd = Wnd;
  AnsiMsg.message = Msg;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;
  if (! MsgiAnsiToUnicodeMessage(&UcMsg, &AnsiMsg))
    {
      return FALSE;
    }

  if (! MsgiUMToKMMessage(&UcMsg, &KMMsg, FALSE))
    {
      MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);
      return FALSE;
    }
  Info.Ansi = TRUE;
  Result = NtUserSendMessage(KMMsg.hwnd, KMMsg.message,
                             KMMsg.wParam, KMMsg.lParam, &Info);
  if (! Info.HandledByKernel)
    {
      /* We need to send the message ourselves */
      if (Info.Ansi)
        {
          /* Ansi message and Ansi window proc, that's easy. Clean up
             the Unicode message though */
          MsgiUMToKMCleanup(&UcMsg, &KMMsg);
          MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);
          Result = IntCallWindowProcA(Info.Ansi, Info.Proc, Wnd, Msg, wParam, lParam);
        }
      else
        {
          /* Unicode winproc. Although we started out with an Ansi message we
             already converted it to Unicode for the kernel call. Reuse that
             message to avoid another conversion */
          Result = IntCallWindowProcW(Info.Ansi, Info.Proc, UcMsg.hwnd,
                                      UcMsg.message, UcMsg.wParam, UcMsg.lParam);
          if (! MsgiAnsiToUnicodeReply(&UcMsg, &AnsiMsg, &Result))
            {
              return FALSE;
            }
        }
    }
  /* Message sent by kernel. Convert back to Ansi */
  else if (! MsgiUMToKMReply(&UcMsg, &KMMsg, &Result) ||
           ! MsgiAnsiToUnicodeReply(&UcMsg, &AnsiMsg, &Result))
    {
      return FALSE;
    }

  return Result;
}


/*
 * @implemented
 */
BOOL
STDCALL
SendMessageCallbackA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData)
{
  return NtUserSendMessageCallback(
    hWnd,
    Msg,
    wParam,
    lParam,
    lpCallBack,
    dwData);
}


/*
 * @implemented
 */
BOOL
STDCALL
SendMessageCallbackW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData)
{
  return NtUserSendMessageCallback(
    hWnd,
    Msg,
    wParam,
    lParam,
    lpCallBack,
    dwData);
}


/*
 * @implemented
 */
LRESULT
STDCALL
SendMessageTimeoutA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  UINT fuFlags,
  UINT uTimeout,
  PDWORD_PTR lpdwResult)
{
  MSG AnsiMsg;
  MSG UcMsg;
  LRESULT Result;
  NTUSERSENDMESSAGEINFO Info;

  AnsiMsg.hwnd = hWnd;
  AnsiMsg.message = Msg;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;
  if (! MsgiAnsiToUnicodeMessage(&UcMsg, &AnsiMsg))
    {
      return FALSE;
    }

  SPY_EnterMessage(SPY_SENDMESSAGE, hWnd, Msg, wParam, lParam);

  Info.Ansi = TRUE;
  Result = NtUserSendMessageTimeout(UcMsg.hwnd, UcMsg.message,
                                    UcMsg.wParam, UcMsg.lParam,
                                    fuFlags, uTimeout, (ULONG_PTR*)lpdwResult, &Info);
  if(!Result)
  {
      SPY_ExitMessage(SPY_RESULT_OK, hWnd, Msg, Result, wParam, lParam);
      return FALSE;
  }
  if (! Info.HandledByKernel)
    {
      /* We need to send the message ourselves */
      if (Info.Ansi)
        {
          /* Ansi message and Ansi window proc, that's easy. Clean up
             the Unicode message though */
          MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);
          Result = IntCallWindowProcA(Info.Ansi, Info.Proc, hWnd, Msg, wParam, lParam);
        }
      else
        {
          /* Unicode winproc. Although we started out with an Ansi message we
             already converted it to Unicode for the kernel call. Reuse that
             message to avoid another conversion */
          Result = IntCallWindowProcW(Info.Ansi, Info.Proc, UcMsg.hwnd,
                                      UcMsg.message, UcMsg.wParam, UcMsg.lParam);
          if (! MsgiAnsiToUnicodeReply(&UcMsg, &AnsiMsg, &Result))
            {
                SPY_ExitMessage(SPY_RESULT_OK, hWnd, Msg, Result, wParam, lParam);
                return FALSE;
            }
        }
      if(lpdwResult)
        *lpdwResult = Result;
      Result = TRUE;
    }
  else
    {
      /* Message sent by kernel. Convert back to Ansi */
      if (! MsgiAnsiToUnicodeReply(&UcMsg, &AnsiMsg, &Result))
        {
            SPY_ExitMessage(SPY_RESULT_OK, hWnd, Msg, Result, wParam, lParam);
            return FALSE;
        }
    }

  SPY_ExitMessage(SPY_RESULT_OK, hWnd, Msg, Result, wParam, lParam);
  return Result;
}


/*
 * @implemented
 */
LRESULT
STDCALL
SendMessageTimeoutW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  UINT fuFlags,
  UINT uTimeout,
  PDWORD_PTR lpdwResult)
{
  NTUSERSENDMESSAGEINFO Info;
  LRESULT Result;

  SPY_EnterMessage(SPY_SENDMESSAGE, hWnd, Msg, wParam, lParam);

  Info.Ansi = FALSE;
  Result = NtUserSendMessageTimeout(hWnd, Msg, wParam, lParam, fuFlags, uTimeout,
                                    lpdwResult, &Info);
  if (! Info.HandledByKernel)
    {
      /* We need to send the message ourselves */
      Result = IntCallWindowProcW(Info.Ansi, Info.Proc, hWnd, Msg, wParam, lParam);
      if(lpdwResult)
        *lpdwResult = Result;

      SPY_ExitMessage(SPY_RESULT_OK, hWnd, Msg, Result, wParam, lParam);
      return TRUE;
    }

  SPY_ExitMessage(SPY_RESULT_OK, hWnd, Msg, Result, wParam, lParam);
  return Result;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SendNotifyMessageA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  MSG AnsiMsg, UcMsg;
  MSG KMMsg;
  LRESULT Result;

  AnsiMsg.hwnd = hWnd;
  AnsiMsg.message = Msg;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;
  if (! MsgiAnsiToUnicodeMessage(&UcMsg, &AnsiMsg))
    {
      return FALSE;
    }

  if (! MsgiUMToKMMessage(&UcMsg, &KMMsg, TRUE))
    {
      MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);
      return FALSE;
    }
  Result = NtUserSendNotifyMessage(KMMsg.hwnd, KMMsg.message,
                                   KMMsg.wParam, KMMsg.lParam);
  MsgiUMToKMCleanup(&UcMsg, &KMMsg);
  MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);

  return Result;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SendNotifyMessageW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  MSG UMMsg, KMMsg;
  LRESULT Result;

  UMMsg.hwnd = hWnd;
  UMMsg.message = Msg;
  UMMsg.wParam = wParam;
  UMMsg.lParam = lParam;
  if (! MsgiUMToKMMessage(&UMMsg, &KMMsg, TRUE))
    {
      return FALSE;
    }
  Result = NtUserSendNotifyMessage(KMMsg.hwnd, KMMsg.message,
                                   KMMsg.wParam, KMMsg.lParam);
  MsgiUMToKMCleanup(&UMMsg, &KMMsg);

  return Result;
}


/*
 * @implemented
 */
BOOL STDCALL
TranslateMessageEx(CONST MSG *lpMsg, DWORD unk)
{
  return(NtUserTranslateMessage((LPMSG)lpMsg, (HKL)unk));
}


/*
 * @implemented
 */
BOOL STDCALL
TranslateMessage(CONST MSG *lpMsg)
{
  return(TranslateMessageEx((LPMSG)lpMsg, 0));
}


/*
 * @implemented
 */
BOOL
STDCALL
WaitMessage(VOID)
{
  return NtUserWaitMessage();
}


/*
 * @implemented
 */
UINT STDCALL
RegisterWindowMessageA(LPCSTR lpString)
{
  UNICODE_STRING String;
  BOOLEAN Result;
  UINT Atom;

  Result = RtlCreateUnicodeStringFromAsciiz(&String, (PCSZ)lpString);
  if (!Result)
    {
      return(0);
    }
  Atom = NtUserRegisterWindowMessage(&String);
  RtlFreeUnicodeString(&String);
  return(Atom);
}


/*
 * @implemented
 */
UINT STDCALL
RegisterWindowMessageW(LPCWSTR lpString)
{
  UNICODE_STRING String;

  RtlInitUnicodeString(&String, lpString);
  return(NtUserRegisterWindowMessage(&String));
}

/*
 * @implemented
 */
HWND STDCALL
SetCapture(HWND hWnd)
{
  return(NtUserSetCapture(hWnd));
}

/*
 * @implemented
 */
HWND STDCALL
GetCapture(VOID)
{
  return(NtUserGetCapture());
}

/*
 * @implemented
 */
BOOL STDCALL
ReleaseCapture(VOID)
{
  NtUserSetCapture(NULL);
  return(TRUE);
}


/*
 * @unimplemented
 */
DWORD
STDCALL
RealGetQueueStatus(UINT flags)
{
   DWORD ret;
   WORD changed_bits, wake_bits;

#if 0 /* wine stuff. don't know what it does... */

   /* check for pending X events */
   if (USER_Driver.pMsgWaitForMultipleObjectsEx)
      USER_Driver.pMsgWaitForMultipleObjectsEx( 0, NULL, 0, 0, 0 );
#endif

   ret = NtUserGetQueueStatus(TRUE /*ClearChanges*/);

   changed_bits = LOWORD(ret);
   wake_bits = HIWORD(ret);

   return MAKELONG(changed_bits & flags, wake_bits & flags);
}


/*
 * @unimplemented
 */
BOOL STDCALL GetInputState(VOID)
{
   DWORD ret;
   WORD  wake_bits;

#if 0 /* wine stuff. don't know what it does... */

   /* check for pending X events */
   if (USER_Driver.pMsgWaitForMultipleObjectsEx)
     USER_Driver.pMsgWaitForMultipleObjectsEx( 0, NULL, 0, 0, 0 );
#endif

   ret = NtUserGetQueueStatus(FALSE /*ClearChanges*/);

   wake_bits = HIWORD(ret);

   return wake_bits & (QS_KEY | QS_MOUSEBUTTON);
}


NTSTATUS STDCALL
User32CallWindowProcFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PWINDOWPROC_CALLBACK_ARGUMENTS CallbackArgs;
  MSG KMMsg, UMMsg;

  /* Make sure we don't try to access mem beyond what we were given */
  if (ArgumentLength < sizeof(WINDOWPROC_CALLBACK_ARGUMENTS))
    {
      return STATUS_INFO_LENGTH_MISMATCH;
    }

  CallbackArgs = (PWINDOWPROC_CALLBACK_ARGUMENTS) Arguments;
  KMMsg.hwnd = CallbackArgs->Wnd;
  KMMsg.message = CallbackArgs->Msg;
  KMMsg.wParam = CallbackArgs->wParam;
  /* Check if lParam is really a pointer and adjust it if it is */
  if (0 <= CallbackArgs->lParamBufferSize)
    {
      if (ArgumentLength != sizeof(WINDOWPROC_CALLBACK_ARGUMENTS)
                            + CallbackArgs->lParamBufferSize)
        {
          return STATUS_INFO_LENGTH_MISMATCH;
        }
      KMMsg.lParam = (LPARAM) ((char *) CallbackArgs + sizeof(WINDOWPROC_CALLBACK_ARGUMENTS));
    }
  else
    {
      if (ArgumentLength != sizeof(WINDOWPROC_CALLBACK_ARGUMENTS))
        {
          return STATUS_INFO_LENGTH_MISMATCH;
        }
      KMMsg.lParam = CallbackArgs->lParam;
    }

  if (WM_NCCALCSIZE == CallbackArgs->Msg && CallbackArgs->wParam)
    {
      NCCALCSIZE_PARAMS *Params = (NCCALCSIZE_PARAMS *) KMMsg.lParam;
      Params->lppos = (PWINDOWPOS) (Params + 1);
    }

  if (! MsgiKMToUMMessage(&KMMsg, &UMMsg))
    {
    }

  CallbackArgs->Result = IntCallWindowProcW(CallbackArgs->IsAnsiProc, CallbackArgs->Proc,
                                            UMMsg.hwnd, UMMsg.message,
                                            UMMsg.wParam, UMMsg.lParam);

  if (! MsgiKMToUMReply(&KMMsg, &UMMsg, &CallbackArgs->Result))
    {
    }

  return ZwCallbackReturn(CallbackArgs, ArgumentLength, STATUS_SUCCESS);
}

/*
 * @implemented
 */
BOOL STDCALL SetMessageQueue(int cMessagesMax)
{
  /* Function does nothing on 32 bit windows */
  return TRUE;
}
typedef DWORD (WINAPI * RealGetQueueStatusProc)(UINT flags);
typedef DWORD (WINAPI * RealMsgWaitForMultipleObjectsExProc)(DWORD nCount, CONST HANDLE *lpHandles, DWORD dwMilliseconds, DWORD dwWakeMask, DWORD dwFlags);

typedef struct _USER_MESSAGE_PUMP_ADDRESSES {
	DWORD cbSize;
	//NtUserRealInternalGetMessageProc NtUserRealInternalGetMessage;
	//NtUserRealWaitMessageExProc NtUserRealWaitMessageEx;
	RealGetQueueStatusProc RealGetQueueStatus;
	RealMsgWaitForMultipleObjectsExProc RealMsgWaitForMultipleObjectsEx;
} USER_MESSAGE_PUMP_ADDRESSES, * PUSER_MESSAGE_PUMP_ADDRESSES;

DWORD
STDCALL
RealMsgWaitForMultipleObjectsEx(
  DWORD nCount,
  CONST HANDLE *pHandles,
  DWORD dwMilliseconds,
  DWORD dwWakeMask,
  DWORD dwFlags);

typedef BOOL (WINAPI * MESSAGEPUMPHOOKPROC)(BOOL Unregistering,PUSER_MESSAGE_PUMP_ADDRESSES MessagePumpAddresses);

CRITICAL_SECTION gcsMPH;
MESSAGEPUMPHOOKPROC gpfnInitMPH;
DWORD gcLoadMPH = 0;
USER_MESSAGE_PUMP_ADDRESSES gmph = {sizeof(USER_MESSAGE_PUMP_ADDRESSES),
	//NtUserRealInternalGetMessage,
	//NtUserRealInternalWaitMessageEx,
	RealGetQueueStatus,
	RealMsgWaitForMultipleObjectsEx
};

DWORD gfMessagePumpHook = 0;

BOOL WINAPI IsInsideMessagePumpHook()
{
	if(!gfMessagePumpHook)
		return FALSE;

    /* This code checks if we're inside SendMessage. */
#if 0
	/* Since our TEB doesnt match that of real windows, testing this value is useless until we know what it does
	PUCHAR NtTeb = (PUCHAR)NtCurrentTeb();

	if(!*(PLONG*)&NtTeb[0x708])
		return FALSE;

	if(**(PLONG*)&NtTeb[0x708] <= 0)
		return FALSE;*/
#endif

	return TRUE;
}

void WINAPI ResetMessagePumpHook(PUSER_MESSAGE_PUMP_ADDRESSES Addresses)
{
	Addresses->cbSize = sizeof(USER_MESSAGE_PUMP_ADDRESSES);
	//Addresses->NtUserRealInternalGetMessage = (NtUserRealInternalGetMessageProc)NtUserRealInternalGetMessage;
	//Addresses->NtUserRealWaitMessageEx = (NtUserRealWaitMessageExProc)NtUserRealInternalWaitMessageEx;
	Addresses->RealGetQueueStatus = RealGetQueueStatus;
	Addresses->RealMsgWaitForMultipleObjectsEx = RealMsgWaitForMultipleObjectsEx;
}

BOOL WINAPI RegisterMessagePumpHook(MESSAGEPUMPHOOKPROC Hook)
{
	EnterCriticalSection(&gcsMPH);
	if(!Hook) {
		SetLastError(ERROR_INVALID_PARAMETER);
		LeaveCriticalSection(&gcsMPH);
		return FALSE;
	}
	if(!gcLoadMPH) {
		USER_MESSAGE_PUMP_ADDRESSES Addresses;
		gpfnInitMPH = Hook;
		ResetMessagePumpHook(&Addresses);
		if(!Hook(FALSE, &Addresses) || !Addresses.cbSize) {
			LeaveCriticalSection(&gcsMPH);
			return FALSE;
		}
		memcpy(&gmph, &Addresses, Addresses.cbSize);
	} else {
		if(gpfnInitMPH != Hook) {
			LeaveCriticalSection(&gcsMPH);
			return FALSE;
		}
	}
	if(NtUserCallNoParam(NOPARAM_ROUTINE_INIT_MESSAGE_PUMP)) {
		LeaveCriticalSection(&gcsMPH);
		return FALSE;
	}
	if (!gcLoadMPH++) {
		InterlockedExchange((PLONG)&gfMessagePumpHook, 1);
	}
	LeaveCriticalSection(&gcsMPH);
	return TRUE;
}

BOOL WINAPI UnregisterMessagePumpHook(VOID)
{
	EnterCriticalSection(&gcsMPH);
	if(gcLoadMPH > 0) {
		if(NtUserCallNoParam(NOPARAM_ROUTINE_UNINIT_MESSAGE_PUMP)) {
			gcLoadMPH--;
			if(!gcLoadMPH) {
				InterlockedExchange((PLONG)&gfMessagePumpHook, 0);
				gpfnInitMPH(TRUE, NULL);
				ResetMessagePumpHook(&gmph);
				gpfnInitMPH = 0;
			}
			LeaveCriticalSection(&gcsMPH);
			return TRUE;
		}
	}
	LeaveCriticalSection(&gcsMPH);
	return FALSE;
}

DWORD WINAPI GetQueueStatus(UINT flags)
{
	return IsInsideMessagePumpHook() ? gmph.RealGetQueueStatus(flags) : RealGetQueueStatus(flags);
}

/**
 * @name RealMsgWaitForMultipleObjectsEx
 *
 * Wait either for either message arrival or for one of the passed events
 * to be signalled.
 *
 * @param nCount
 *        Number of handles in the pHandles array.
 * @param pHandles
 *        Handles of events to wait for.
 * @param dwMilliseconds
 *        Timeout interval.
 * @param dwWakeMask
 *        Mask specifying on which message events we should wakeup.
 * @param dwFlags
 *        Wait type (see MWMO_* constants).
 *
 * @implemented
 */

DWORD STDCALL
RealMsgWaitForMultipleObjectsEx(
   DWORD nCount,
   const HANDLE *pHandles,
   DWORD dwMilliseconds,
   DWORD dwWakeMask,
   DWORD dwFlags)
{
   LPHANDLE RealHandles;
   HANDLE MessageQueueHandle;
   DWORD Result;

   if (dwFlags & ~(MWMO_WAITALL | MWMO_ALERTABLE | MWMO_INPUTAVAILABLE))
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return WAIT_FAILED;
   }

/*
   if (dwFlags & MWMO_INPUTAVAILABLE)
   {
      RealGetQueueStatus(dwWakeMask);
   }
   */

   MessageQueueHandle = NtUserMsqSetWakeMask(dwWakeMask);
   if (MessageQueueHandle == NULL)
   {
      SetLastError(0); /* ? */
      return WAIT_FAILED;
   }

   RealHandles = HeapAlloc(GetProcessHeap(), 0, (nCount + 1) * sizeof(HANDLE));
   if (RealHandles == NULL)
   {
      NtUserMsqClearWakeMask();
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return WAIT_FAILED;
   }

   RtlCopyMemory(RealHandles, pHandles, nCount * sizeof(HANDLE));
   RealHandles[nCount] = MessageQueueHandle;

   Result = WaitForMultipleObjectsEx(nCount + 1, RealHandles,
                                     dwFlags & MWMO_WAITALL,
                                     dwMilliseconds, dwFlags & MWMO_ALERTABLE);

   HeapFree(GetProcessHeap(), 0, RealHandles);
   NtUserMsqClearWakeMask();

   return Result;
}

/*
 * @implemented
 */
DWORD WINAPI
MsgWaitForMultipleObjectsEx(
   DWORD nCount,
   CONST HANDLE *lpHandles,
   DWORD dwMilliseconds,
   DWORD dwWakeMask,
   DWORD dwFlags)
{
   return IsInsideMessagePumpHook() ? gmph.RealMsgWaitForMultipleObjectsEx(nCount, lpHandles, dwMilliseconds, dwWakeMask, dwFlags) : RealMsgWaitForMultipleObjectsEx(nCount, lpHandles,dwMilliseconds, dwWakeMask, dwFlags);
}

/*
 * @implemented
 */
DWORD STDCALL
MsgWaitForMultipleObjects(
   DWORD nCount,
   CONST HANDLE *lpHandles,
   BOOL fWaitAll,
   DWORD dwMilliseconds,
   DWORD dwWakeMask)
{
   return MsgWaitForMultipleObjectsEx(nCount, lpHandles, dwMilliseconds,
                                      dwWakeMask, fWaitAll ? MWMO_WAITALL : 0);
}


BOOL FASTCALL MessageInit(VOID)
{
  InitializeCriticalSection(&DdeCrst);
  InitializeCriticalSection(&MsgConversionCrst);
  InitializeCriticalSection(&gcsMPH);

  return TRUE;
}

VOID FASTCALL MessageCleanup(VOID)
{
  DeleteCriticalSection(&DdeCrst);
  DeleteCriticalSection(&MsgConversionCrst);
  DeleteCriticalSection(&gcsMPH);
}

/***********************************************************************
 *		map_wparam_AtoW
 *
 * Convert the wparam of an ASCII message to Unicode.
 */
static WPARAM
map_wparam_AtoW( UINT message, WPARAM wparam )
{
    switch(message)
    {
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:
        {
            char ch[2];
            WCHAR wch[2];
            ch[0] = (wparam & 0xff);
            ch[1] = (wparam >> 8);
            MultiByteToWideChar(CP_ACP, 0, ch, 2, wch, 2);
            wparam = MAKEWPARAM(wch[0], wch[1]);
        }
        break;
    case WM_IME_CHAR:
        {
            char ch[2];
            WCHAR wch;
            ch[0] = (wparam >> 8);
            ch[1] = (wparam & 0xff);
            if (ch[0]) MultiByteToWideChar(CP_ACP, 0, ch, 2, &wch, 1);
            else MultiByteToWideChar(CP_ACP, 0, &ch[1], 1, &wch, 1);
            wparam = MAKEWPARAM( wch, HIWORD(wparam) );
        }
        break;
    }
    return wparam;
}

/*
 * @implemented
 */
BOOL WINAPI
IsDialogMessageA( HWND hwndDlg, LPMSG pmsg )
{
    MSG msg = *pmsg;
    msg.wParam = map_wparam_AtoW( msg.message, msg.wParam );
    return IsDialogMessageW( hwndDlg, &msg );
}
