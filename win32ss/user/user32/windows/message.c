/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/windows/message.c
 * PURPOSE:         Messages
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */

#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

#ifdef __i386__
/* For bad applications which provide bad (non stdcall) WndProc */
extern
LRESULT
__cdecl
CALL_EXTERN_WNDPROC(
     WNDPROC WndProc,
     HWND hWnd,
     UINT Msg,
     WPARAM wParam,
     LPARAM lParam);
#else
#  define CALL_EXTERN_WNDPROC(proc, h, m, w, l) proc(h, m, w, l)
#endif

/* From wine: */
/* flag for messages that contain pointers */
/* 32 messages per entry, messages 0..31 map to bits 0..31 */

#define SET(msg) (1 << ((msg) & 31))

static const unsigned int message_pointer_flags[] =
{
    /* 0x00 - 0x1f */
    SET(WM_CREATE) | SET(WM_SETTEXT) | SET(WM_GETTEXT) |
    SET(WM_WININICHANGE) | SET(WM_DEVMODECHANGE),
    /* 0x20 - 0x3f */
    SET(WM_GETMINMAXINFO) | SET(WM_DRAWITEM) | SET(WM_MEASUREITEM) | SET(WM_DELETEITEM) |
    SET(WM_COMPAREITEM),
    /* 0x40 - 0x5f */
    SET(WM_WINDOWPOSCHANGING) | SET(WM_WINDOWPOSCHANGED) | SET(WM_COPYDATA) | SET(WM_COPYGLOBALDATA) | SET(WM_HELP),
    /* 0x60 - 0x7f */
    SET(WM_STYLECHANGING) | SET(WM_STYLECHANGED),
    /* 0x80 - 0x9f */
    SET(WM_NCCREATE) | SET(WM_NCCALCSIZE) | SET(WM_GETDLGCODE),
    /* 0xa0 - 0xbf */
    SET(EM_GETSEL) | SET(EM_GETRECT) | SET(EM_SETRECT) | SET(EM_SETRECTNP),
    /* 0xc0 - 0xdf */
    SET(EM_REPLACESEL) | SET(EM_GETLINE) | SET(EM_SETTABSTOPS),
    /* 0xe0 - 0xff */
    SET(SBM_GETRANGE) | SET(SBM_SETSCROLLINFO) | SET(SBM_GETSCROLLINFO) | SET(SBM_GETSCROLLBARINFO),
    /* 0x100 - 0x11f */
    0,
    /* 0x120 - 0x13f */
    0,
    /* 0x140 - 0x15f */
    SET(CB_GETEDITSEL) | SET(CB_ADDSTRING) | SET(CB_DIR) | SET(CB_GETLBTEXT) |
    SET(CB_INSERTSTRING) | SET(CB_FINDSTRING) | SET(CB_SELECTSTRING) |
    SET(CB_GETDROPPEDCONTROLRECT) | SET(CB_FINDSTRINGEXACT),
    /* 0x160 - 0x17f */
    0,
    /* 0x180 - 0x19f */
    SET(LB_ADDSTRING) | SET(LB_INSERTSTRING) | SET(LB_GETTEXT) | SET(LB_SELECTSTRING) |
    SET(LB_DIR) | SET(LB_FINDSTRING) |
    SET(LB_GETSELITEMS) | SET(LB_SETTABSTOPS) | SET(LB_ADDFILE) | SET(LB_GETITEMRECT),
    /* 0x1a0 - 0x1bf */
    SET(LB_FINDSTRINGEXACT),
    /* 0x1c0 - 0x1df */
    0,
    /* 0x1e0 - 0x1ff */
    0,
    /* 0x200 - 0x21f */
    SET(WM_NEXTMENU) | SET(WM_SIZING) | SET(WM_MOVING) | SET(WM_DEVICECHANGE),
    /* 0x220 - 0x23f */
    SET(WM_MDICREATE) | SET(WM_MDIGETACTIVE) | SET(WM_DROPOBJECT) |
    SET(WM_QUERYDROPOBJECT) | SET(WM_DRAGLOOP) | SET(WM_DRAGSELECT) | SET(WM_DRAGMOVE),
    /* 0x240 - 0x25f */
    0,
    /* 0x260 - 0x27f */
    0,
    /* 0x280 - 0x29f */
    0,
    /* 0x2a0 - 0x2bf */
    0,
    /* 0x2c0 - 0x2df */
    0,
    /* 0x2e0 - 0x2ff */
    0,
    /* 0x300 - 0x31f */
    SET(WM_ASKCBFORMATNAME)
};

/* check whether a given message type includes pointers */
static inline int is_pointer_message( UINT message, WPARAM wparam )
{
    if (message >= 8*sizeof(message_pointer_flags)) return FALSE;
    if (message == WM_DEVICECHANGE && !(wparam & 0x8000)) return FALSE;
    return (message_pointer_flags[message / 32] & SET(message)) != 0;
}

#undef SET

/* check whether a combobox expects strings or ids in CB_ADDSTRING/CB_INSERTSTRING */
static BOOL FASTCALL combobox_has_strings( HWND hwnd )
{
    DWORD style = GetWindowLongA( hwnd, GWL_STYLE );
    return (!(style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) || (style & CBS_HASSTRINGS));
}

/* check whether a listbox expects strings or ids in LB_ADDSTRING/LB_INSERTSTRING */
static BOOL FASTCALL listbox_has_strings( HWND hwnd )
{
    DWORD style = GetWindowLongA( hwnd, GWL_STYLE );
    return (!(style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) || (style & LBS_HASSTRINGS));
}

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

BOOL FASTCALL
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

HGLOBAL FASTCALL
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

DWORD FASTCALL get_input_codepage( void )
{
    DWORD cp;
    int ret;
    HKL hkl = GetKeyboardLayout( 0 );
    ret = GetLocaleInfoW( LOWORD(hkl), LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER, (WCHAR *)&cp, sizeof(cp) / sizeof(WCHAR) );
    if (!ret) cp = CP_ACP;
    return cp;
}

static WPARAM FASTCALL map_wparam_char_WtoA( WPARAM wParam, DWORD len )
{
    WCHAR wch = wParam;
    BYTE ch[2];
    DWORD cp = get_input_codepage();

    len = WideCharToMultiByte( cp, 0, &wch, 1, (LPSTR)ch, len, NULL, NULL );
    if (len == 2)
       return MAKEWPARAM( (ch[0] << 8) | ch[1], HIWORD(wParam) );
    else
    return MAKEWPARAM( ch[0], HIWORD(wParam) );
}

/***********************************************************************
 *		map_wparam_AtoW
 *
 * Convert the wparam of an ASCII message to Unicode.
 */
static WPARAM FASTCALL
map_wparam_AtoW( UINT message, WPARAM wparam )
{
    char ch[2];
    WCHAR wch[2];

    wch[0] = wch[1] = 0;
    switch(message)
    {
    case WM_CHAR:
        /* WM_CHAR is magic: a DBCS char can be sent/posted as two consecutive WM_CHAR
         * messages, in which case the first char is stored, and the conversion
         * to Unicode only takes place once the second char is sent/posted.
         */
#if 0
        if (mapping != WMCHAR_MAP_NOMAPPING) // NlsMbCodePageTag
        {
            PCLIENTINFO pci = GetWin32ClientInfo();

            struct wm_char_mapping_data *data = get_user_thread_info()->wmchar_data;

            BYTE low = LOBYTE(wparam);

            if (HIBYTE(wparam))
            {
                ch[0] = low;
                ch[1] = HIBYTE(wparam);
                RtlMultiByteToUnicodeN( wch, sizeof(wch), NULL, ch, 2 );
                TRACE( "map %02x,%02x -> %04x mapping %u\n", (BYTE)ch[0], (BYTE)ch[1], wch[0], mapping );
                if (data) data->lead_byte[mapping] = 0;
            }
            else if (data && data->lead_byte[mapping])
            {
                ch[0] = data->lead_byte[mapping];
                ch[1] = low;
                RtlMultiByteToUnicodeN( wch, sizeof(wch), NULL, ch, 2 );
                TRACE( "map stored %02x,%02x -> %04x mapping %u\n", (BYTE)ch[0], (BYTE)ch[1], wch[0], mapping );
                data->lead_byte[mapping] = 0;
            }
            else if (!IsDBCSLeadByte( low ))
            {
                ch[0] = low;
                RtlMultiByteToUnicodeN( wch, sizeof(wch), NULL, ch, 1 );
                TRACE( "map %02x -> %04x\n", (BYTE)ch[0], wch[0] );
                if (data) data->lead_byte[mapping] = 0;
            }
            else  /* store it and wait for trail byte */
            {
                if (!data)
                {
                    if (!(data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data) )))
                        return FALSE;
                    get_user_thread_info()->wmchar_data = data;
                }
                TRACE( "storing lead byte %02x mapping %u\n", low, mapping );
                data->lead_byte[mapping] = low;
                return FALSE;
            }
            wparam = MAKEWPARAM(wch[0], wch[1]);
            break;
        }
#endif
        /* else fall through */
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:
        ch[0] = LOBYTE(wparam);
        ch[1] = HIBYTE(wparam);
        RtlMultiByteToUnicodeN( wch, sizeof(wch), NULL, ch, 2 );
        wparam = MAKEWPARAM(wch[0], wch[1]);
        break;
    case WM_IME_CHAR:
        ch[0] = HIBYTE(wparam);
        ch[1] = LOBYTE(wparam);
        if (ch[0]) RtlMultiByteToUnicodeN( wch, sizeof(wch[0]), NULL, ch, 2 );
        else RtlMultiByteToUnicodeN( wch, sizeof(wch[0]), NULL, ch + 1, 1 );
        wparam = MAKEWPARAM(wch[0], HIWORD(wparam));
        break;
    }
    return wparam;
}

static
BOOL FASTCALL
MsgiUMToKMMessage(PMSG UMMsg, PMSG KMMsg, BOOL Posted)
{
  *KMMsg = *UMMsg;

  switch (UMMsg->message)
    {
      case WM_COPYDATA:
        {
          PCOPYDATASTRUCT pUMCopyData = (PCOPYDATASTRUCT)UMMsg->lParam;
          PCOPYDATASTRUCT pKMCopyData;

          pKMCopyData = HeapAlloc(GetProcessHeap(), 0, sizeof(COPYDATASTRUCT) + pUMCopyData->cbData);
          if (!pKMCopyData)
          {
              SetLastError(ERROR_OUTOFMEMORY);
              return FALSE;
          }

          pKMCopyData->dwData = pUMCopyData->dwData;
          pKMCopyData->cbData = pUMCopyData->cbData;
          pKMCopyData->lpData = pKMCopyData + 1;

          RtlCopyMemory(pKMCopyData + 1, pUMCopyData->lpData, pUMCopyData->cbData);

          KMMsg->lParam = (LPARAM)pKMCopyData;
        }
        break;

      case WM_COPYGLOBALDATA:
        {
           KMMsg->lParam = (LPARAM)GlobalLock((HGLOBAL)UMMsg->lParam);;
           TRACE("WM_COPYGLOBALDATA get data ptr %p\n",KMMsg->lParam);
        }
        break;

      default:
        break;
    }

  return TRUE;
}

static
VOID FASTCALL
MsgiUMToKMCleanup(PMSG UMMsg, PMSG KMMsg)
{
  switch (KMMsg->message)
    {
      case WM_COPYDATA:
        HeapFree(GetProcessHeap(), 0, (LPVOID) KMMsg->lParam);
        break;
      case WM_COPYGLOBALDATA:
        TRACE("WM_COPYGLOBALDATA cleanup return\n");
        GlobalUnlock((HGLOBAL)UMMsg->lParam);
        GlobalFree((HGLOBAL)UMMsg->lParam);
        break;
      default:
        break;
    }

  return;
}

static BOOL FASTCALL
MsgiKMToUMMessage(PMSG KMMsg, PMSG UMMsg)
{
  *UMMsg = *KMMsg;

  if (KMMsg->lParam == 0) return TRUE;

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

      case WM_COPYDATA:
        {
          PCOPYDATASTRUCT pKMCopyData = (PCOPYDATASTRUCT)KMMsg->lParam;
          pKMCopyData->lpData = pKMCopyData + 1;
        }
        break;

      case WM_COPYGLOBALDATA:
        {
           PVOID Data;
           HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, KMMsg->wParam);
           Data = GlobalLock(hGlobal);
           if (Data) RtlCopyMemory(Data, (PVOID)KMMsg->lParam, KMMsg->wParam);
           GlobalUnlock(hGlobal);
           TRACE("WM_COPYGLOBALDATA to User hGlobal %p\n",hGlobal);
           UMMsg->lParam = (LPARAM)hGlobal;
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
#ifdef TODO // Kept as historic.
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

//
//  Ansi to Unicode -> callout
//
static BOOL FASTCALL
MsgiAnsiToUnicodeMessage(HWND hwnd, LPMSG UnicodeMsg, LPMSG AnsiMsg)
{
  UNICODE_STRING UnicodeString;

  *UnicodeMsg = *AnsiMsg;

  switch (AnsiMsg->message)
    {
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
      {
        LPWSTR Buffer;
        if (!AnsiMsg->lParam) break;
        Buffer = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, AnsiMsg->wParam * sizeof(WCHAR));
        //ERR("WM_GETTEXT A2U Size %d\n",AnsiMsg->wParam);
        if (!Buffer) return FALSE;
        UnicodeMsg->lParam = (LPARAM)Buffer;
        break;
      }

    case LB_GETTEXT:
      {
        DWORD Size = 1024 * sizeof(WCHAR);
        if (!AnsiMsg->lParam || !listbox_has_strings( AnsiMsg->hwnd )) break;
        /*Size = SendMessageW( AnsiMsg->hwnd, LB_GETTEXTLEN, AnsiMsg->wParam, 0 );
        if (Size == LB_ERR)
        {
           ERR("LB_GETTEXT LB_ERR\n");
           Size = sizeof(ULONG_PTR);
        }
        Size = (Size + 1) * sizeof(WCHAR);*/
        UnicodeMsg->lParam = (LPARAM) RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
        if (!UnicodeMsg->lParam) return FALSE;
        break;
      }

    case CB_GETLBTEXT:
      {
        DWORD Size = 1024 * sizeof(WCHAR);
        if (!AnsiMsg->lParam || !combobox_has_strings( AnsiMsg->hwnd )) break;
        /*Size = SendMessageW( AnsiMsg->hwnd, CB_GETLBTEXTLEN, AnsiMsg->wParam, 0 );
        if (Size == LB_ERR)
        {
           ERR("CB_GETTEXT LB_ERR\n");
           Size = sizeof(ULONG_PTR);
        }
        Size = (Size + 1) * sizeof(WCHAR);*/
        UnicodeMsg->lParam = (LPARAM) RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
        if (!UnicodeMsg->lParam) return FALSE;
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
        if (!AnsiMsg->lParam) break;
        RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)AnsiMsg->lParam);
        UnicodeMsg->lParam = (LPARAM)UnicodeString.Buffer;
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
        if (AnsiMsg->lParam && listbox_has_strings(AnsiMsg->hwnd))
          {
            RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)AnsiMsg->lParam);
            UnicodeMsg->lParam = (LPARAM)UnicodeString.Buffer;
          }
        break;
      }

    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
      {
        if (AnsiMsg->lParam && combobox_has_strings(AnsiMsg->hwnd))
          {
            RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)AnsiMsg->lParam);
            UnicodeMsg->lParam = (LPARAM)UnicodeString.Buffer;
          }
        break;
      }

    case WM_NCCREATE:
    case WM_CREATE:
      {
        struct s
        {
           CREATESTRUCTW cs;          /* new structure */
           MDICREATESTRUCTW mdi_cs;   /* MDI info */
           LPCWSTR lpszName;          /* allocated Name */
           LPCWSTR lpszClass;         /* allocated Class */
        };

        struct s *xs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct s));
        if (!xs)
          {
            return FALSE;
          }
        xs->cs = *(CREATESTRUCTW *)AnsiMsg->lParam;
        if (!IS_INTRESOURCE(xs->cs.lpszName))
          {
            RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)xs->cs.lpszName);
            xs->lpszName = xs->cs.lpszName = UnicodeString.Buffer;
          }
        if (!IS_ATOM(xs->cs.lpszClass))
          {
            RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)xs->cs.lpszClass);
            xs->lpszClass = xs->cs.lpszClass = UnicodeString.Buffer;
          }

        if (GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_MDICHILD)
        {
           xs->mdi_cs = *(MDICREATESTRUCTW *)xs->cs.lpCreateParams;
           xs->mdi_cs.szTitle = xs->cs.lpszName;
           xs->mdi_cs.szClass = xs->cs.lpszClass;
           xs->cs.lpCreateParams = &xs->mdi_cs;
        }

        UnicodeMsg->lParam = (LPARAM)xs;
        break;
      }

    case WM_MDICREATE:
      {
        MDICREATESTRUCTW *cs =
            (MDICREATESTRUCTW *)HeapAlloc(GetProcessHeap(), 0, sizeof(*cs));

        if (!cs)
          {
            return FALSE;
          }

        *cs = *(MDICREATESTRUCTW *)AnsiMsg->lParam;

        if (!IS_ATOM(cs->szClass))
          {
            RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)cs->szClass);
            cs->szClass = UnicodeString.Buffer;
          }

        RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)cs->szTitle);
        cs->szTitle = UnicodeString.Buffer;

        UnicodeMsg->lParam = (LPARAM)cs;
        break;
      }

    case WM_GETDLGCODE:
      if (UnicodeMsg->lParam)
      {
         MSG newmsg = *(MSG *)UnicodeMsg->lParam;
         newmsg.wParam = map_wparam_AtoW( newmsg.message, newmsg.wParam);
      }
      break;

    case WM_CHARTOITEM:
    case WM_MENUCHAR:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case EM_SETPASSWORDCHAR:
    case WM_IME_CHAR:
      UnicodeMsg->wParam = map_wparam_AtoW( AnsiMsg->message, AnsiMsg->wParam );
      break;
    case EM_GETLINE:
      ERR("FIXME EM_GETLINE A2U\n");
      break;

    }

  return TRUE;
}

static BOOL FASTCALL
MsgiAnsiToUnicodeCleanup(LPMSG UnicodeMsg, LPMSG AnsiMsg)
{
  UNICODE_STRING UnicodeString;

  switch (AnsiMsg->message)
    {
    case LB_GETTEXT:
        if (!listbox_has_strings( UnicodeMsg->hwnd )) break;
    case CB_GETLBTEXT:
        if (UnicodeMsg->message == CB_GETLBTEXT && !combobox_has_strings( UnicodeMsg->hwnd )) break;
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
      {
        if (!UnicodeMsg->lParam) break;
        RtlFreeHeap(GetProcessHeap(), 0, (PVOID) UnicodeMsg->lParam);
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
        if (!UnicodeMsg->lParam) break;
        RtlInitUnicodeString(&UnicodeString, (PCWSTR)UnicodeMsg->lParam);
        RtlFreeUnicodeString(&UnicodeString);
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
        if (UnicodeMsg->lParam && listbox_has_strings(AnsiMsg->hwnd))
          {
            RtlInitUnicodeString(&UnicodeString, (PCWSTR)UnicodeMsg->lParam);
            RtlFreeUnicodeString(&UnicodeString);
          }
        break;
      }

    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
      {
        if (UnicodeMsg->lParam && combobox_has_strings(AnsiMsg->hwnd))
          {
            RtlInitUnicodeString(&UnicodeString, (PCWSTR)UnicodeMsg->lParam);
            RtlFreeUnicodeString(&UnicodeString);
          }
        break;
      }

    case WM_NCCREATE:
    case WM_CREATE:
      {
        struct s
        {
           CREATESTRUCTW cs;          /* new structure */
           MDICREATESTRUCTW mdi_cs;   /* MDI info */
           LPWSTR lpszName;           /* allocated Name */
           LPWSTR lpszClass;          /* allocated Class */
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
        break;
      }

    case WM_MDICREATE:
      {
        MDICREATESTRUCTW *cs = (MDICREATESTRUCTW *)UnicodeMsg->lParam;
        RtlInitUnicodeString(&UnicodeString, (PCWSTR)cs->szTitle);
        RtlFreeUnicodeString(&UnicodeString);
        if (!IS_ATOM(cs->szClass))
          {
            RtlInitUnicodeString(&UnicodeString, (PCWSTR)cs->szClass);
            RtlFreeUnicodeString(&UnicodeString);
          }
        HeapFree(GetProcessHeap(), 0, cs);
        break;
      }
    }

  return(TRUE);
}

/*
 *    callout return -> Unicode Result to Ansi Result
 */
static BOOL FASTCALL
MsgiAnsiToUnicodeReply(LPMSG UnicodeMsg, LPMSG AnsiMsg, LRESULT *Result)
{
  LPWSTR Buffer = (LPWSTR)UnicodeMsg->lParam;
  LPSTR AnsiBuffer = (LPSTR)AnsiMsg->lParam;

  switch (AnsiMsg->message)
    {
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
      {
        if (UnicodeMsg->wParam)
        {
           DWORD len = 0;
           if (*Result) RtlUnicodeToMultiByteN( AnsiBuffer, UnicodeMsg->wParam - 1, &len, Buffer, strlenW(Buffer) * sizeof(WCHAR));
           AnsiBuffer[len] = 0;
           *Result = len;
           //ERR("WM_GETTEXT U2A Result %d Size %d\n",*Result,AnsiMsg->wParam);
        }
        break;
      }
    case LB_GETTEXT:
      {
        if (!AnsiBuffer || !listbox_has_strings( UnicodeMsg->hwnd )) break;
        if (*Result >= 0)
        {
           DWORD len;
           RtlUnicodeToMultiByteN( AnsiBuffer, ~0u, &len, Buffer, (strlenW(Buffer) + 1) * sizeof(WCHAR) );
           *Result = len - 1;
        }
        break;
      }
    case CB_GETLBTEXT:
      {
        if (!AnsiBuffer || !combobox_has_strings( UnicodeMsg->hwnd )) break;
        if (*Result >= 0)
        {
           DWORD len;
           RtlUnicodeToMultiByteN( AnsiBuffer, ~0u, &len, Buffer, (strlenW(Buffer) + 1) * sizeof(WCHAR) );
           *Result = len - 1;
        }
        break;
      }
    }

  MsgiAnsiToUnicodeCleanup(UnicodeMsg, AnsiMsg);

  return TRUE;
}

//
//  Unicode to Ansi callout ->
//
static BOOL FASTCALL
MsgiUnicodeToAnsiMessage(HWND hwnd, LPMSG AnsiMsg, LPMSG UnicodeMsg)
{
  ANSI_STRING AnsiString;
  UNICODE_STRING UnicodeString;

  *AnsiMsg = *UnicodeMsg;

  switch(UnicodeMsg->message)
    {
      case WM_CREATE:
      case WM_NCCREATE:
        {
          MDICREATESTRUCTA *pmdi_cs;
          CREATESTRUCTA* CsA;
          CREATESTRUCTW* CsW;
          ULONG NameSize, ClassSize;
          NTSTATUS Status;

          CsW = (CREATESTRUCTW*)(UnicodeMsg->lParam);
          RtlInitUnicodeString(&UnicodeString, CsW->lpszName);
          NameSize = RtlUnicodeStringToAnsiSize(&UnicodeString);
          if (NameSize == 0)
            {
              return FALSE;
            }
          ClassSize = 0;
          if (!IS_ATOM(CsW->lpszClass))
            {
              RtlInitUnicodeString(&UnicodeString, CsW->lpszClass);
              ClassSize = RtlUnicodeStringToAnsiSize(&UnicodeString);
              if (ClassSize == 0)
                {
                  return FALSE;
                }
            }

          CsA = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(CREATESTRUCTA) + sizeof(MDICREATESTRUCTA) + NameSize + ClassSize);
          if (NULL == CsA)
            {
              return FALSE;
            }
          memcpy(CsA, CsW, sizeof(CREATESTRUCTW));

          /* pmdi_cs starts right after CsA */
          pmdi_cs = (MDICREATESTRUCTA*)(CsA + 1);

          RtlInitUnicodeString(&UnicodeString, CsW->lpszName);
          RtlInitEmptyAnsiString(&AnsiString, (PCHAR)(pmdi_cs + 1), NameSize);
          Status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);
          if (! NT_SUCCESS(Status))
            {
              RtlFreeHeap(GetProcessHeap(), 0, CsA);
              return FALSE;
            }
          CsA->lpszName = AnsiString.Buffer;
          if (!IS_ATOM(CsW->lpszClass))
            {
              RtlInitUnicodeString(&UnicodeString, CsW->lpszClass);
              RtlInitEmptyAnsiString(&AnsiString, (PCHAR)(pmdi_cs + 1) + NameSize, ClassSize);
              Status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);
              if (! NT_SUCCESS(Status))
                {
                  RtlFreeHeap(GetProcessHeap(), 0, CsA);
                  return FALSE;
                }
              CsA->lpszClass = AnsiString.Buffer;
            }

          if (GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_MDICHILD)
          {
             *pmdi_cs = *(MDICREATESTRUCTA *)CsW->lpCreateParams;
             pmdi_cs->szTitle = CsA->lpszName;
             pmdi_cs->szClass = CsA->lpszClass;
             CsA->lpCreateParams = pmdi_cs;
          }

          AnsiMsg->lParam = (LPARAM)CsA;
          break;
        }
      case WM_GETTEXT:
      case WM_ASKCBFORMATNAME:
        {
          if (!UnicodeMsg->lParam) break;
          /* Ansi string might contain MBCS chars so we need 2 * the number of chars */
          AnsiMsg->lParam = (LPARAM) RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, UnicodeMsg->wParam * 2);
          //ERR("WM_GETTEXT U2A Size %d\n",AnsiMsg->wParam);

          if (!AnsiMsg->lParam) return FALSE;
          break;
        }

    case LB_GETTEXT:
      {
        DWORD Size = 1024;
        if (!UnicodeMsg->lParam || !listbox_has_strings( UnicodeMsg->hwnd )) break;
        /*Size = SendMessageA( UnicodeMsg->hwnd, LB_GETTEXTLEN, UnicodeMsg->wParam, 0 );
        if (Size == LB_ERR)
        {
           ERR("LB_GETTEXT LB_ERR\n");
           Size = sizeof(ULONG_PTR);
        }
        Size = (Size + 1) * sizeof(WCHAR);*/
        AnsiMsg->lParam = (LPARAM) RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
        if (!AnsiMsg->lParam) return FALSE;
        break;
      }

    case CB_GETLBTEXT:
      {
        DWORD Size = 1024;
        if (!UnicodeMsg->lParam || !combobox_has_strings( UnicodeMsg->hwnd )) break;
        /*Size = SendMessageA( UnicodeMsg->hwnd, CB_GETLBTEXTLEN, UnicodeMsg->wParam, 0 );
        if (Size == LB_ERR)
        {
           ERR("CB_GETTEXT LB_ERR\n");
           Size = sizeof(ULONG_PTR);
        }
        Size = (Size + 1) * sizeof(WCHAR);*/
        AnsiMsg->lParam = (LPARAM) RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
        if (!AnsiMsg->lParam) return FALSE;
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
          if (!UnicodeMsg->lParam) break;
          RtlInitUnicodeString(&UnicodeString, (PWSTR) UnicodeMsg->lParam);
          if (! NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiString,
                                                        &UnicodeString,
                                                        TRUE)))
            {
              return FALSE;
            }
          AnsiMsg->lParam = (LPARAM) AnsiString.Buffer;
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
          if (UnicodeMsg->lParam && listbox_has_strings(AnsiMsg->hwnd))
            {
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

      case CB_ADDSTRING:
      case CB_INSERTSTRING:
      case CB_FINDSTRING:
      case CB_FINDSTRINGEXACT:
      case CB_SELECTSTRING:
        {
          if (UnicodeMsg->lParam && combobox_has_strings(AnsiMsg->hwnd))
            {
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
              if (! NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiString,
                                                            &UnicodeString,
                                                            TRUE)))
                {
                  HeapFree(GetProcessHeap(), 0, cs);
                  return FALSE;
                }
              cs->szClass = AnsiString.Buffer;
            }

          RtlInitUnicodeString(&UnicodeString, (LPCWSTR)cs->szTitle);
          if (! NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiString,
                                                        &UnicodeString,
                                                        TRUE)))
            {
              if (!IS_ATOM(cs->szClass))
                {
                  RtlInitAnsiString(&AnsiString, cs->szClass);
                  RtlFreeAnsiString(&AnsiString);
                }

              HeapFree(GetProcessHeap(), 0, cs);
              return FALSE;
            }
          cs->szTitle = AnsiString.Buffer;

          AnsiMsg->lParam = (LPARAM)cs;
          break;
        }

      case WM_GETDLGCODE:
        if (UnicodeMsg->lParam)
        {
           MSG newmsg = *(MSG *)UnicodeMsg->lParam;
           switch(newmsg.message)
           {
              case WM_CHAR:
              case WM_DEADCHAR:
              case WM_SYSCHAR:
              case WM_SYSDEADCHAR:
                newmsg.wParam = map_wparam_char_WtoA( newmsg.wParam, 1 );
                break;
              case WM_IME_CHAR:
                newmsg.wParam = map_wparam_char_WtoA( newmsg.wParam, 2 );
                break;
           }
        }
        break;

      case WM_CHAR:
        {
           WCHAR wch = UnicodeMsg->wParam;
           char ch[2];
           DWORD cp = get_input_codepage();
           DWORD len = WideCharToMultiByte( cp, 0, &wch, 1, ch, 2, NULL, NULL );
           AnsiMsg->wParam = (BYTE)ch[0];
           if (len == 2) AnsiMsg->wParam = (BYTE)ch[1];
        }
        break;

      case WM_CHARTOITEM:
      case WM_MENUCHAR:
      case WM_DEADCHAR:
      case WM_SYSCHAR:
      case WM_SYSDEADCHAR:
      case EM_SETPASSWORDCHAR:
          AnsiMsg->wParam = map_wparam_char_WtoA(UnicodeMsg->wParam,1);
          break;

      case WM_IME_CHAR:
          AnsiMsg->wParam = map_wparam_char_WtoA(UnicodeMsg->wParam,2);
          break;
      case EM_GETLINE:
          ERR("FIXME EM_GETLINE U2A\n");
          break;
    }
  return TRUE;
}

static BOOL FASTCALL
MsgiUnicodeToAnsiCleanup(LPMSG AnsiMsg, LPMSG UnicodeMsg)
{
  ANSI_STRING AnsiString;

  switch(UnicodeMsg->message)
    {
      case LB_GETTEXT:
        if (!listbox_has_strings( AnsiMsg->hwnd )) break;
      case CB_GETLBTEXT:
        if (AnsiMsg->message == CB_GETLBTEXT && !combobox_has_strings( AnsiMsg->hwnd )) break;
      case WM_GETTEXT:
      case WM_ASKCBFORMATNAME:
        {
          if (!AnsiMsg->lParam) break;
          RtlFreeHeap(GetProcessHeap(), 0, (PVOID) AnsiMsg->lParam);
          break;
        }
      case WM_CREATE:
      case WM_NCCREATE:
        {
          CREATESTRUCTA* Cs;

          Cs = (CREATESTRUCTA*) AnsiMsg->lParam;
          RtlFreeHeap(GetProcessHeap(), 0, Cs);
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
          if (!AnsiMsg->lParam) break;
          RtlInitAnsiString(&AnsiString, (PSTR) AnsiMsg->lParam);
          RtlFreeAnsiString(&AnsiString);
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
          if (AnsiMsg->lParam && listbox_has_strings(AnsiMsg->hwnd))
            {
              RtlInitAnsiString(&AnsiString, (PSTR) AnsiMsg->lParam);
              RtlFreeAnsiString(&AnsiString);
            }
          break;
        }

      case CB_ADDSTRING:
      case CB_INSERTSTRING:
      case CB_FINDSTRING:
      case CB_FINDSTRINGEXACT:
      case CB_SELECTSTRING:
        {
          if (AnsiMsg->lParam && combobox_has_strings(AnsiMsg->hwnd))
            {
              RtlInitAnsiString(&AnsiString, (PSTR) AnsiMsg->lParam);
              RtlFreeAnsiString(&AnsiString);
            }
          break;
        }

      case WM_MDICREATE:
        {
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

/*
 *    callout return -> Ansi Result to Unicode Result
 */
static BOOL FASTCALL
MsgiUnicodeToAnsiReply(LPMSG AnsiMsg, LPMSG UnicodeMsg, LRESULT *Result)
{
  LPSTR Buffer = (LPSTR) AnsiMsg->lParam;
  LPWSTR UBuffer = (LPWSTR) UnicodeMsg->lParam;

  switch (UnicodeMsg->message)
    {
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
      {
        DWORD len = AnsiMsg->wParam;// * 2;
        if (len)
        {
           if (*Result)
           {
              RtlMultiByteToUnicodeN( UBuffer, AnsiMsg->wParam*sizeof(WCHAR), &len, Buffer, strlen(Buffer)+1 );
              *Result = len/sizeof(WCHAR) - 1;  /* do not count terminating null */
              //ERR("WM_GETTEXT U2A Result %d Size %d\n",*Result,AnsiMsg->wParam);
           }
           UBuffer[*Result] = 0;
        }
        break;
      }
    case LB_GETTEXT:
      {
        if (!UBuffer || !listbox_has_strings( UnicodeMsg->hwnd )) break;
        if (*Result >= 0)
        {
           DWORD len;
           RtlMultiByteToUnicodeN( UBuffer, ~0u, &len, Buffer, strlen(Buffer) + 1 );
           *Result = len / sizeof(WCHAR) - 1;
        }
        break;
      }
    case CB_GETLBTEXT:
      {
        if (!UBuffer || !combobox_has_strings( UnicodeMsg->hwnd )) break;
        if (*Result >= 0)
        {
           DWORD len;
           RtlMultiByteToUnicodeN( UBuffer, ~0u, &len, Buffer, strlen(Buffer) + 1 );
           *Result = len / sizeof(WCHAR) - 1;
        }
        break;
      }
    }

  MsgiUnicodeToAnsiCleanup(AnsiMsg, UnicodeMsg);

  return TRUE;
}


LRESULT
WINAPI
DesktopWndProcA( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  LRESULT Result;
  MSG AnsiMsg, UcMsg;

  TRACE("Desktop A Class Atom! hWnd 0x%x, Msg %d\n", hwnd, message);

  AnsiMsg.hwnd = hwnd;
  AnsiMsg.message = message;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;
  AnsiMsg.time = 0;
  AnsiMsg.pt.x = 0;
  AnsiMsg.pt.y = 0;

  // Desktop is always Unicode so convert Ansi here.
  if (!MsgiAnsiToUnicodeMessage(hwnd, &UcMsg, &AnsiMsg))
  {
     return FALSE;
  }

  Result = DesktopWndProcW(hwnd, message, UcMsg.wParam, UcMsg.lParam);

  MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);

  return Result;
 }

/*
 * @implemented
 */
LPARAM
WINAPI
GetMessageExtraInfo(VOID)
{
  return NtUserxGetMessageExtraInfo();
}


/*
 * @implemented
 */
DWORD
WINAPI
GetMessagePos(VOID)
{
  return NtUserxGetMessagePos();
}


/*
 * @implemented
 */
LONG WINAPI
GetMessageTime(VOID)
{
  return NtUserGetThreadState(THREADSTATE_GETMESSAGETIME);
}


/*
 * @implemented
 */
BOOL
WINAPI
InSendMessage(VOID)
{
  PCLIENTTHREADINFO pcti = GetWin32ClientInfo()->pClientThreadInfo;
  if ( pcti )
  {
    if (pcti->CTI_flags & CTI_INSENDMESSAGE)
    {
       return TRUE;
    }
  }
  return(NtUserGetThreadState(THREADSTATE_INSENDMESSAGE) != ISMEX_NOSEND);
}


/*
 * @implemented
 */
DWORD
WINAPI
InSendMessageEx(
  LPVOID lpReserved)
{
  PCLIENTTHREADINFO pcti = GetWin32ClientInfo()->pClientThreadInfo;
  if (pcti && !(pcti->CTI_flags & CTI_INSENDMESSAGE))
     return ISMEX_NOSEND;
  else
     return NtUserGetThreadState(THREADSTATE_INSENDMESSAGE);
}


/*
 * @implemented
 */
BOOL
WINAPI
ReplyMessage(LRESULT lResult)
{
  return NtUserxReplyMessage(lResult);
}


/*
 * @implemented
 */
LPARAM
WINAPI
SetMessageExtraInfo(
  LPARAM lParam)
{
  return NtUserxSetMessageExtraInfo(lParam);
}

LRESULT FASTCALL
IntCallWindowProcW(BOOL IsAnsiProc,
                   WNDPROC WndProc,
                   PWND pWnd,
                   HWND hWnd,
                   UINT Msg,
                   WPARAM wParam,
                   LPARAM lParam)
{
  MSG AnsiMsg;
  MSG UnicodeMsg;
  BOOL Hook = FALSE, MsgOverride = FALSE, Dialog, DlgOverride = FALSE;
  LRESULT Result = 0, PreResult = 0;
  DWORD Data = 0;

  if (WndProc == NULL)
  {
      WARN("IntCallWindowsProcW() called with WndProc = NULL!\n");
      return FALSE;
  }

  if (pWnd)
     Dialog = (pWnd->fnid == FNID_DIALOG);
  else
     Dialog = FALSE;

  Hook = BeginIfHookedUserApiHook();
  if (Hook)
  {
     if (Dialog)
        DlgOverride = IsMsgOverride( Msg, &guah.DlgProcArray);
     MsgOverride = IsMsgOverride( Msg, &guah.WndProcArray);
  }

  if (IsAnsiProc)
  {
      UnicodeMsg.hwnd = hWnd;
      UnicodeMsg.message = Msg;
      UnicodeMsg.wParam = wParam;
      UnicodeMsg.lParam = lParam;
      UnicodeMsg.time = 0;
      UnicodeMsg.pt.x = 0;
      UnicodeMsg.pt.y = 0;
       if (! MsgiUnicodeToAnsiMessage(hWnd, &AnsiMsg, &UnicodeMsg))
      {
          goto Exit;
      }

      if (Hook && (MsgOverride || DlgOverride))
      {
         _SEH2_TRY
         {
            if (!DlgOverride)
               PreResult = guah.PreWndProc(AnsiMsg.hwnd, AnsiMsg.message, AnsiMsg.wParam, AnsiMsg.lParam, (ULONG_PTR)&Result, &Data );
            else
               PreResult = guah.PreDefDlgProc(AnsiMsg.hwnd, AnsiMsg.message, AnsiMsg.wParam, AnsiMsg.lParam, (ULONG_PTR)&Result, &Data );
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
            ERR("Got exception in hooked PreWndProc, dlg:%d!\n", DlgOverride);
         }
         _SEH2_END;
      }

      if (PreResult) goto Exit;

      if (!Dialog)
      Result = CALL_EXTERN_WNDPROC(WndProc, AnsiMsg.hwnd, AnsiMsg.message, AnsiMsg.wParam, AnsiMsg.lParam);
      else
      {
      _SEH2_TRY
      {
         Result = CALL_EXTERN_WNDPROC(WndProc, AnsiMsg.hwnd, AnsiMsg.message, AnsiMsg.wParam, AnsiMsg.lParam);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         ERR("Exception Dialog Ansi %p Msg %d pti %p Wndpti %p\n",WndProc,Msg,GetW32ThreadInfo(),pWnd->head.pti);
      }
      _SEH2_END;
      }

      if (Hook && (MsgOverride || DlgOverride))
      {
         _SEH2_TRY
         {
            if (!DlgOverride)
               guah.PostWndProc(AnsiMsg.hwnd, AnsiMsg.message, AnsiMsg.wParam, AnsiMsg.lParam, (ULONG_PTR)&Result, &Data );
            else
               guah.PostDefDlgProc(AnsiMsg.hwnd, AnsiMsg.message, AnsiMsg.wParam, AnsiMsg.lParam, (ULONG_PTR)&Result, &Data );
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
            ERR("Got exception in hooked PostWndProc, dlg:%d!\n", DlgOverride);
         }
         _SEH2_END;
      }

      if (! MsgiUnicodeToAnsiReply(&AnsiMsg, &UnicodeMsg, &Result))
      {
          goto Exit;
      }
  }
  else
  {
      if (Hook && (MsgOverride || DlgOverride))
      {
         _SEH2_TRY
         {
            if (!DlgOverride)
               PreResult = guah.PreWndProc(hWnd, Msg, wParam, lParam, (ULONG_PTR)&Result, &Data );
            else
               PreResult = guah.PreDefDlgProc(hWnd, Msg, wParam, lParam, (ULONG_PTR)&Result, &Data );
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
            ERR("Got exception in hooked PreWndProc, dlg:%d!\n", DlgOverride);
         }
         _SEH2_END;
      }

      if (PreResult) goto Exit;

      if (!Dialog)
      Result = CALL_EXTERN_WNDPROC(WndProc, hWnd, Msg, wParam, lParam);
      else
      {
      _SEH2_TRY
      {
         Result = CALL_EXTERN_WNDPROC(WndProc, hWnd, Msg, wParam, lParam);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         ERR("Exception Dialog unicode %p Msg %d pti %p Wndpti %p\n",WndProc, Msg,GetW32ThreadInfo(),pWnd->head.pti);
      }
      _SEH2_END;
      }

      if (Hook && (MsgOverride || DlgOverride))
      {
         _SEH2_TRY
         {
            if (!DlgOverride)
               guah.PostWndProc(hWnd, Msg, wParam, lParam, (ULONG_PTR)&Result, &Data );
            else
               guah.PostDefDlgProc(hWnd, Msg, wParam, lParam, (ULONG_PTR)&Result, &Data );
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
            ERR("Got exception in hooked PostWndProc, dlg:%d!\n", DlgOverride);
         }
         _SEH2_END;
      }
  }

Exit:
  if (Hook) EndUserApiHook();
  return Result;
}

static LRESULT FASTCALL
IntCallWindowProcA(BOOL IsAnsiProc,
                   WNDPROC WndProc,
                   PWND pWnd,
                   HWND hWnd,
                   UINT Msg,
                   WPARAM wParam,
                   LPARAM lParam)
{
  MSG AnsiMsg;
  MSG UnicodeMsg;
  BOOL Hook = FALSE, MsgOverride = FALSE, Dialog, DlgOverride = FALSE;
  LRESULT Result = 0, PreResult = 0;
  DWORD Data = 0;

  TRACE("IntCallWindowProcA: IsAnsiProc : %s, WndProc %p, pWnd %p, hWnd %p, Msg %u, wParam %Iu, lParam %Iu.\n",
      IsAnsiProc ? "TRUE" : "FALSE", WndProc, pWnd, hWnd, Msg, wParam, lParam);

  if (WndProc == NULL)
  {
      WARN("IntCallWindowsProcA() called with WndProc = NULL!\n");
      return FALSE;
  }

  if (pWnd)
     Dialog = (pWnd->fnid == FNID_DIALOG);
  else
     Dialog = FALSE;

  Hook = BeginIfHookedUserApiHook();
  if (Hook)
  {
     if (Dialog)
        DlgOverride = IsMsgOverride( Msg, &guah.DlgProcArray);
     MsgOverride = IsMsgOverride( Msg, &guah.WndProcArray);
  }

  if (IsAnsiProc)
  {
      if (Hook && (MsgOverride || DlgOverride))
      {
         _SEH2_TRY
         {
            if (!DlgOverride)
               PreResult = guah.PreWndProc(hWnd, Msg, wParam, lParam, (ULONG_PTR)&Result, &Data );
            else
               PreResult = guah.PreDefDlgProc(hWnd, Msg, wParam, lParam, (ULONG_PTR)&Result, &Data );
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
            ERR("Got exception in hooked PreWndProc, dlg:%d!\n", DlgOverride);
         }
         _SEH2_END;
      }

      if (PreResult) goto Exit;

      if (!Dialog)
      Result = CALL_EXTERN_WNDPROC(WndProc, hWnd, Msg, wParam, lParam);
      else
      {
      _SEH2_TRY
      {
         Result = CALL_EXTERN_WNDPROC(WndProc, hWnd, Msg, wParam, lParam);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         ERR("Exception Dialog Ansi %p Msg %d pti %p Wndpti %p\n",WndProc,Msg,GetW32ThreadInfo(),pWnd->head.pti);
      }
      _SEH2_END;
      }

      if (Hook && (MsgOverride || DlgOverride))
      {
         _SEH2_TRY
         {
            if (!DlgOverride)
               guah.PostWndProc(hWnd, Msg, wParam, lParam, (ULONG_PTR)&Result, &Data );
            else
               guah.PostDefDlgProc(hWnd, Msg, wParam, lParam, (ULONG_PTR)&Result, &Data );
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
            ERR("Got exception in hooked PostWndProc, dlg:%d!\n", DlgOverride);
         }
         _SEH2_END;
      }
  }
  else
  {
      AnsiMsg.hwnd = hWnd;
      AnsiMsg.message = Msg;
      AnsiMsg.wParam = wParam;
      AnsiMsg.lParam = lParam;
      AnsiMsg.time = 0;
      AnsiMsg.pt.x = 0;
      AnsiMsg.pt.y = 0;
      if (! MsgiAnsiToUnicodeMessage(hWnd, &UnicodeMsg, &AnsiMsg))
      {
          goto Exit;
      }

      if (Hook && (MsgOverride || DlgOverride))
      {
         _SEH2_TRY
         {
            if (!DlgOverride)
               PreResult = guah.PreWndProc(UnicodeMsg.hwnd, UnicodeMsg.message, UnicodeMsg.wParam, UnicodeMsg.lParam, (ULONG_PTR)&Result, &Data );
            else
               PreResult = guah.PreDefDlgProc(UnicodeMsg.hwnd, UnicodeMsg.message, UnicodeMsg.wParam, UnicodeMsg.lParam, (ULONG_PTR)&Result, &Data );
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
            ERR("Got exception in hooked PreWndProc, dlg:%d!\n", DlgOverride);
         }
         _SEH2_END;
      }

      if (PreResult) goto Exit;

      if (!Dialog)
      Result = CALL_EXTERN_WNDPROC(WndProc, UnicodeMsg.hwnd, UnicodeMsg.message, UnicodeMsg.wParam, UnicodeMsg.lParam);
      else
      {
      _SEH2_TRY
      {
         Result = CALL_EXTERN_WNDPROC(WndProc, UnicodeMsg.hwnd, UnicodeMsg.message, UnicodeMsg.wParam, UnicodeMsg.lParam);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         ERR("Exception Dialog unicode %p Msg %d pti %p Wndpti %p\n",WndProc, Msg,GetW32ThreadInfo(),pWnd->head.pti);
      }
      _SEH2_END;
      }

      if (Hook && (MsgOverride || DlgOverride))
      {
         _SEH2_TRY
         {
            if (!DlgOverride)
               guah.PostWndProc(UnicodeMsg.hwnd, UnicodeMsg.message, UnicodeMsg.wParam, UnicodeMsg.lParam, (ULONG_PTR)&Result, &Data );
            else
               guah.PostDefDlgProc(UnicodeMsg.hwnd, UnicodeMsg.message, UnicodeMsg.wParam, UnicodeMsg.lParam, (ULONG_PTR)&Result, &Data );
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
            ERR("Got exception in hooked PostWndProc, dlg:%d!\n", DlgOverride);
         }
         _SEH2_END;
      }

      if (! MsgiAnsiToUnicodeReply(&UnicodeMsg, &AnsiMsg, &Result))
      {
         goto Exit;
      }
  }

Exit:
  if (Hook) EndUserApiHook();
  return Result;
}


static LRESULT WINAPI
IntCallMessageProc(IN PWND Wnd, IN HWND hWnd, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam, IN BOOL Ansi)
{
    WNDPROC WndProc;
    BOOL IsAnsi;
    PCLS Class;

    Class = DesktopPtrToUser(Wnd->pcls);
    WndProc = NULL;

    if ( Wnd->head.pti != GetW32ThreadInfo())
    {  // Must be inside the same thread!
       SetLastError( ERROR_MESSAGE_SYNC_ONLY );
       return 0;
    }
  /*
      This is the message exchange for user32. If there's a need to monitor messages,
      do it here!
   */
    TRACE("HWND %p, MSG %u, WPARAM %p, LPARAM %p, Ansi %d\n", hWnd, Msg, wParam, lParam, Ansi);
//    if (Class->fnid <= FNID_GHOST && Class->fnid >= FNID_BUTTON )
    if (Class->fnid <= FNID_GHOST && Class->fnid >= FNID_FIRST )
    {
       if (Ansi)
       {
          if (GETPFNCLIENTW(Class->fnid) == Wnd->lpfnWndProc)
             WndProc = GETPFNCLIENTA(Class->fnid);
       }
       else
       {
          if (GETPFNCLIENTA(Class->fnid) == Wnd->lpfnWndProc)
             WndProc = GETPFNCLIENTW(Class->fnid);
       }

       IsAnsi = Ansi;

       if (!WndProc)
       {
          IsAnsi = !Wnd->Unicode;
          WndProc = Wnd->lpfnWndProc;
       }
    }
    else
    {
       IsAnsi = !Wnd->Unicode;
       WndProc = Wnd->lpfnWndProc;
    }
/*
   Message caller can be Ansi/Unicode and the receiver can be Unicode/Ansi or
   the same.
 */
    if (!Ansi)
        return IntCallWindowProcW(IsAnsi, WndProc, Wnd, hWnd, Msg, wParam, lParam);
    else
        return IntCallWindowProcA(IsAnsi, WndProc, Wnd, hWnd, Msg, wParam, lParam);
}


/*
 * @implemented
 */
LRESULT WINAPI
CallWindowProcA(WNDPROC lpPrevWndFunc,
		HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam)
{
    PWND pWnd;
    PCALLPROCDATA CallProc;

    if (lpPrevWndFunc == NULL)
    {
        WARN("CallWindowProcA: lpPrevWndFunc == NULL!\n");
        return 0;
    }

    pWnd = ValidateHwnd(hWnd);

    if (!IsCallProcHandle(lpPrevWndFunc))
        return IntCallWindowProcA(TRUE, lpPrevWndFunc, pWnd, hWnd, Msg, wParam, lParam);
    else
    {
        CallProc = ValidateCallProc((HANDLE)lpPrevWndFunc);
        if (CallProc != NULL)
        {
            return IntCallWindowProcA(!(CallProc->wType & UserGetCPDA2U),
                                        CallProc->pfnClientPrevious,
                                        pWnd,
                                        hWnd,
                                        Msg,
                                        wParam,
                                        lParam);
        }
        else
        {
            WARN("CallWindowProcA: can not dereference WndProcHandle\n");
            return 0;
        }
    }
}


/*
 * @implemented
 */
LRESULT WINAPI
CallWindowProcW(WNDPROC lpPrevWndFunc,
		HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam)
{
    PWND pWnd;
    PCALLPROCDATA CallProc;

    /* FIXME - can the first parameter be NULL? */
    if (lpPrevWndFunc == NULL)
    {
        WARN("CallWindowProcA: lpPrevWndFunc == NULL!\n");
        return 0;
    }

    pWnd = ValidateHwnd(hWnd);

    if (!IsCallProcHandle(lpPrevWndFunc))
        return IntCallWindowProcW(FALSE, lpPrevWndFunc, pWnd, hWnd, Msg, wParam, lParam);
    else
    {
        CallProc = ValidateCallProc((HANDLE)lpPrevWndFunc);
        if (CallProc != NULL)
        {
            return IntCallWindowProcW(!(CallProc->wType & UserGetCPDA2U),
                                        CallProc->pfnClientPrevious,
                                        pWnd,
                                        hWnd,
                                        Msg,
                                        wParam,
                                        lParam);
        }
        else
        {
            WARN("CallWindowProcW: can not dereference WndProcHandle\n");
            return 0;
        }
    }
}


/*
 * @implemented
 */
LRESULT
WINAPI
DECLSPEC_HOTPATCH
DispatchMessageA(CONST MSG *lpmsg)
{
    LRESULT Ret = 0;
    MSG UnicodeMsg;
    PWND Wnd;

    if ( lpmsg->message & ~WM_MAXIMUM )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (lpmsg->hwnd != NULL)
    {
        Wnd = ValidateHwnd(lpmsg->hwnd);
        if (!Wnd) return 0;
    }
    else
        Wnd = NULL;

    if (is_pointer_message(lpmsg->message, lpmsg->wParam))
    {
       SetLastError( ERROR_MESSAGE_SYNC_ONLY );
       return 0;
    }

    if ((lpmsg->message == WM_TIMER || lpmsg->message == WM_SYSTIMER) && lpmsg->lParam != 0)
    {
        WNDPROC WndProc = (WNDPROC)lpmsg->lParam;

        if ( lpmsg->message == WM_SYSTIMER )
           return NtUserDispatchMessage( (PMSG)lpmsg );

        if (!NtUserValidateTimerCallback(lpmsg->lParam))
        {
           WARN("Validating Timer Callback failed!\n");
           return 0;
        }

       _SEH2_TRY // wine does this. Hint: Prevents call to another thread....
       {
           Ret = WndProc(lpmsg->hwnd,
                         lpmsg->message,
                         lpmsg->wParam,
                         GetTickCount());
       }
       _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
       {
           ERR("Exception in Timer Callback!\n");
       }
       _SEH2_END;
    }
    else if (Wnd != NULL)
    {
       if ( (lpmsg->message != WM_PAINT) && !(Wnd->state & WNDS_SERVERSIDEWINDOWPROC) )
       {
           Ret = IntCallMessageProc(Wnd,
                                    lpmsg->hwnd,
                                    lpmsg->message,
                                    lpmsg->wParam,
                                    lpmsg->lParam,
                                    TRUE);
       }
       else
       {
          if (!MsgiAnsiToUnicodeMessage(lpmsg->hwnd, &UnicodeMsg, (LPMSG)lpmsg))
          {
             return FALSE;
          }

          Ret = NtUserDispatchMessage(&UnicodeMsg);

          if (!MsgiAnsiToUnicodeReply(&UnicodeMsg, (LPMSG)lpmsg, &Ret))
          {
             return FALSE;
          }
       }
    }

    return Ret;
}


/*
 * @implemented
 */
LRESULT
WINAPI
DECLSPEC_HOTPATCH
DispatchMessageW(CONST MSG *lpmsg)
{
    LRESULT Ret = 0;
    PWND Wnd;
    BOOL Hit = FALSE;

    if ( lpmsg->message & ~WM_MAXIMUM )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (lpmsg->hwnd != NULL)
    {
        Wnd = ValidateHwnd(lpmsg->hwnd);
        if (!Wnd) return 0;
    }
    else
        Wnd = NULL;

    if (is_pointer_message(lpmsg->message, lpmsg->wParam))
    {
       SetLastError( ERROR_MESSAGE_SYNC_ONLY );
       return 0;
    }

    if ((lpmsg->message == WM_TIMER || lpmsg->message == WM_SYSTIMER) && lpmsg->lParam != 0)
    {
        WNDPROC WndProc = (WNDPROC)lpmsg->lParam;

        if ( lpmsg->message == WM_SYSTIMER )
           return NtUserDispatchMessage( (PMSG) lpmsg );

        if (!NtUserValidateTimerCallback(lpmsg->lParam))
        {
           WARN("Validating Timer Callback failed!\n");
           return 0;
        }

       _SEH2_TRY
       {
           Ret = WndProc(lpmsg->hwnd,
                         lpmsg->message,
                         lpmsg->wParam,
                         GetTickCount());
       }
       _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
       {
          Hit = TRUE;
       }
       _SEH2_END;
    }
    else if (Wnd != NULL)
    {
       if ( (lpmsg->message != WM_PAINT) && !(Wnd->state & WNDS_SERVERSIDEWINDOWPROC) )
       {
           Ret = IntCallMessageProc(Wnd,
                                    lpmsg->hwnd,
                                    lpmsg->message,
                                    lpmsg->wParam,
                                    lpmsg->lParam,
                                    FALSE);
       }
       else
         Ret = NtUserDispatchMessage( (PMSG) lpmsg );
    }

    if (Hit)
    {
       WARN("Exception in Timer Callback WndProcW!\n");
    }
    return Ret;
}

static VOID
IntConvertMsgToAnsi(LPMSG lpMsg)
{
    CHAR ch[2];
    WCHAR wch[2];

    switch (lpMsg->message)
    {
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
        case WM_MENUCHAR:
            wch[0] = LOWORD(lpMsg->wParam);
            wch[1] = HIWORD(lpMsg->wParam);
            ch[0] = ch[1] = 0;
            WideCharToMultiByte(CP_THREAD_ACP, 0, wch, 2, ch, 2, NULL, NULL);
            lpMsg->wParam = MAKEWPARAM(ch[0] | (ch[1] << 8), 0);
            break;
    }
}

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
GetMessageA(LPMSG lpMsg,
            HWND hWnd,
            UINT wMsgFilterMin,
            UINT wMsgFilterMax)
{
  BOOL Res;

  if ( (wMsgFilterMin|wMsgFilterMax) & ~WM_MAXIMUM )
  {
     SetLastError( ERROR_INVALID_PARAMETER );
     return FALSE;
  }

  Res = NtUserGetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
  if (-1 == (int) Res)
  {
      return Res;
  }

  IntConvertMsgToAnsi(lpMsg);

  return Res;
}

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
GetMessageW(LPMSG lpMsg,
            HWND hWnd,
            UINT wMsgFilterMin,
            UINT wMsgFilterMax)
{
  BOOL Res;

  if ( (wMsgFilterMin|wMsgFilterMax) & ~WM_MAXIMUM )
  {
     SetLastError( ERROR_INVALID_PARAMETER );
     return FALSE;
  }

  Res = NtUserGetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
  if (-1 == (int) Res)
  {
      return Res;
  }

  return Res;
}

BOOL WINAPI
PeekMessageWorker( PMSG pMsg,
                   HWND hWnd,
                   UINT wMsgFilterMin,
                   UINT wMsgFilterMax,
                   UINT wRemoveMsg)
{
  PCLIENTINFO pci;
  PCLIENTTHREADINFO pcti;
  pci = GetWin32ClientInfo();
  pcti = pci->pClientThreadInfo;

  if (!hWnd && pci && pcti)
  {
     pci->cSpins++;

     if ((pci->cSpins >= 100) && (pci->dwTIFlags & TIF_SPINNING))
     {  // Yield after 100 spin cycles and ready to swap vinyl.
        if (!(pci->dwTIFlags & TIF_WAITFORINPUTIDLE))
        {  // Not waiting for idle event.
           if (!pcti->fsChangeBits && !pcti->fsWakeBits)
           {  // No messages are available.
              if ((GetTickCount() - pcti->timeLastRead) > 1000)
              {  // Up the msg read count if over 1 sec.
                 NtUserGetThreadState(THREADSTATE_UPTIMELASTREAD);
              }
              pci->cSpins = 0;
              ZwYieldExecution();
              FIXME("seeSpins!\n");
              return FALSE;
           }
        }
     }
  }
  return NtUserPeekMessage(pMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
PeekMessageA(LPMSG lpMsg,
	     HWND hWnd,
	     UINT wMsgFilterMin,
	     UINT wMsgFilterMax,
	     UINT wRemoveMsg)
{
  BOOL Res;

  Res = PeekMessageWorker(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  if (-1 == (int) Res || !Res)
  {
      return FALSE;
  }

  IntConvertMsgToAnsi(lpMsg);

  return Res;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
PeekMessageW(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg)
{
  BOOL Res;

  Res = PeekMessageWorker(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  if (-1 == (int) Res || !Res)
  {
      return FALSE;
  }

  return Res;
}

/*
 * @implemented
 */
BOOL
WINAPI
PostMessageA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  LRESULT Ret;

  /* Check for combo box or a list box to send names. */
  if (Msg == CB_DIR || Msg == LB_DIR)
  {
  /*
     Set DDL_POSTMSGS, so use the PostMessage function to send messages to the
     combo/list box. Forces a call like DlgDirListComboBox.
  */
    //wParam |= DDL_POSTMSGS;
    return NtUserPostMessage(hWnd, Msg, wParam, lParam);
  }

  /* No drop files or current Process, just post message. */
  if ( (Msg != WM_DROPFILES) ||
       ( NtUserQueryWindow( hWnd, QUERY_WINDOW_UNIQUE_PROCESS_ID) ==
                  PtrToUint(NtCurrentTeb()->ClientId.UniqueProcess) ) )
  {
    return NtUserPostMessage(hWnd, Msg, wParam, lParam);
  }

  /* We have drop files and this is not the same process for this window. */

  /* Just incase, check wParam for Global memory handle and send size. */
  Ret = SendMessageA( hWnd,
                      WM_COPYGLOBALDATA,
                      (WPARAM)GlobalSize((HGLOBAL)wParam), // Zero if not a handle.
                      (LPARAM)wParam);                     // Send wParam as lParam.

  if ( Ret ) return NtUserPostMessage(hWnd, Msg, (WPARAM)Ret, lParam);

  return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
PostMessageW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  LRESULT Ret;

  /* Check for combo box or a list box to send names. */
  if (Msg == CB_DIR || Msg == LB_DIR)
  {
  /*
     Set DDL_POSTMSGS, so use the PostMessage function to send messages to the
     combo/list box. Forces a call like DlgDirListComboBox.
  */
    //wParam |= DDL_POSTMSGS;
    return NtUserPostMessage(hWnd, Msg, wParam, lParam);
  }

  /* No drop files or current Process, just post message. */
  if ( (Msg != WM_DROPFILES) ||
       ( NtUserQueryWindow( hWnd, QUERY_WINDOW_UNIQUE_PROCESS_ID) ==
                  PtrToUint(NtCurrentTeb()->ClientId.UniqueProcess) ) )
  {
    return NtUserPostMessage(hWnd, Msg, wParam, lParam);
  }

  /* We have drop files and this is not the same process for this window. */

  /* Just incase, check wParam for Global memory handle and send size. */
  Ret = SendMessageW( hWnd,
                      WM_COPYGLOBALDATA,
                      (WPARAM)GlobalSize((HGLOBAL)wParam), // Zero if not a handle.
                      (LPARAM)wParam);                     // Send wParam as lParam.

  if ( Ret ) return NtUserPostMessage(hWnd, Msg, (WPARAM)Ret, lParam);

  return FALSE;
}

/*
 * @implemented
 */
VOID
WINAPI
PostQuitMessage(
  int nExitCode)
{
    NtUserxPostQuitMessage(nExitCode);
}


/*
 * @implemented
 */
BOOL
WINAPI
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
WINAPI
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
LRESULT WINAPI
SendMessageW(HWND Wnd,
	     UINT Msg,
	     WPARAM wParam,
	     LPARAM lParam)
{
  MSG UMMsg, KMMsg;
  LRESULT Result;
  BOOL Ret;
  PWND Window;
  PTHREADINFO ti = GetW32ThreadInfo();

  if ( Msg & ~WM_MAXIMUM )
  {
     SetLastError( ERROR_INVALID_PARAMETER );
     return 0;
  }

  if (Wnd != HWND_TOPMOST && Wnd != HWND_BROADCAST && (Msg < WM_DDE_FIRST || Msg > WM_DDE_LAST))
  {
      Window = ValidateHwnd(Wnd);

      if ( Window != NULL &&
           Window->head.pti == ti &&
          !ISITHOOKED(WH_CALLWNDPROC) &&
          !ISITHOOKED(WH_CALLWNDPROCRET) &&
          !(Window->state & WNDS_SERVERSIDEWINDOWPROC) )
      {
          /* NOTE: We can directly send messages to the window procedure
                   if *all* the following conditions are met:

                   * Window belongs to calling thread
                   * The calling thread is not being hooked for CallWndProc
                   * Not calling a server side proc:
                     Desktop, Switch, ScrollBar, Menu, IconTitle, or hWndMessage
           */

          return IntCallMessageProc(Window, Wnd, Msg, wParam, lParam, FALSE);
      }
  }

  UMMsg.hwnd = Wnd;
  UMMsg.message = Msg;
  UMMsg.wParam = wParam;
  UMMsg.lParam = lParam;
  UMMsg.time = 0;
  UMMsg.pt.x = 0;
  UMMsg.pt.y = 0;

  if (! MsgiUMToKMMessage(&UMMsg, &KMMsg, FALSE))
  {
     return FALSE;
  }

  Ret = NtUserMessageCall( Wnd,
                           KMMsg.message,
                           KMMsg.wParam,
                           KMMsg.lParam,
                          (ULONG_PTR)&Result,
                           FNID_SENDMESSAGE,
                           FALSE);
  if (!Ret)
  {
     ERR("SendMessageW Error\n");
  }

  MsgiUMToKMCleanup(&UMMsg, &KMMsg);

  return Result;
}


/*
 * @implemented
 */
LRESULT WINAPI
SendMessageA(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  MSG AnsiMsg, UcMsg, KMMsg;
  LRESULT Result;
  BOOL Ret;
  PWND Window;
  PTHREADINFO ti = GetW32ThreadInfo();

  if ( Msg & ~WM_MAXIMUM )
  {
     SetLastError( ERROR_INVALID_PARAMETER );
     return 0;
  }

  if (Wnd != HWND_TOPMOST && Wnd != HWND_BROADCAST && (Msg < WM_DDE_FIRST || Msg > WM_DDE_LAST))
  {
      Window = ValidateHwnd(Wnd);

      if ( Window != NULL &&
           Window->head.pti == ti &&
          !ISITHOOKED(WH_CALLWNDPROC) &&
          !ISITHOOKED(WH_CALLWNDPROCRET) &&
          !(Window->state & WNDS_SERVERSIDEWINDOWPROC) )
      {
          /* NOTE: We can directly send messages to the window procedure
                   if *all* the following conditions are met:

                   * Window belongs to calling thread
                   * The calling thread is not being hooked for CallWndProc
                   * Not calling a server side proc:
                     Desktop, Switch, ScrollBar, Menu, IconTitle, or hWndMessage
           */

          return IntCallMessageProc(Window, Wnd, Msg, wParam, lParam, TRUE);
      }
  }

  AnsiMsg.hwnd = Wnd;
  AnsiMsg.message = Msg;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;
  AnsiMsg.time = 0;
  AnsiMsg.pt.x = 0;
  AnsiMsg.pt.y = 0;

  if (!MsgiAnsiToUnicodeMessage(Wnd, &UcMsg, &AnsiMsg))
  {
     return FALSE;
  }

  if (!MsgiUMToKMMessage(&UcMsg, &KMMsg, FALSE))
  {
      MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);
      return FALSE;
  }

  Ret = NtUserMessageCall( Wnd,
                           KMMsg.message,
                           KMMsg.wParam,
                           KMMsg.lParam,
                          (ULONG_PTR)&Result,
                           FNID_SENDMESSAGE,
                           TRUE);
  if (!Ret)
  {
     ERR("SendMessageA Error\n");
  }

  MsgiUMToKMCleanup(&UcMsg, &KMMsg);
  MsgiAnsiToUnicodeReply(&UcMsg, &AnsiMsg, &Result);

  return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
SendMessageCallbackA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData)
{
  BOOL Result;
  MSG AnsiMsg, UcMsg;
  CALL_BACK_INFO CallBackInfo;

  if (is_pointer_message(Msg, wParam))
  {
     SetLastError( ERROR_MESSAGE_SYNC_ONLY );
     return FALSE;
  }

  CallBackInfo.CallBack = lpCallBack;
  CallBackInfo.Context = dwData;

  AnsiMsg.hwnd = hWnd;
  AnsiMsg.message = Msg;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;
  AnsiMsg.time = 0;
  AnsiMsg.pt.x = 0;
  AnsiMsg.pt.y = 0;

  if (!MsgiAnsiToUnicodeMessage(hWnd, &UcMsg, &AnsiMsg))
  {
      return FALSE;
  }

  Result = NtUserMessageCall( hWnd,
                              UcMsg.message,
                              UcMsg.wParam,
                              UcMsg.lParam,
                             (ULONG_PTR)&CallBackInfo,
                              FNID_SENDMESSAGECALLBACK,
                              TRUE);

  MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);

  return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
SendMessageCallbackW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData)
{
  CALL_BACK_INFO CallBackInfo;

  if (is_pointer_message(Msg, wParam))
  {
     SetLastError( ERROR_MESSAGE_SYNC_ONLY );
     return FALSE;
  }

  CallBackInfo.CallBack = lpCallBack;
  CallBackInfo.Context = dwData;

  return NtUserMessageCall(hWnd,
                            Msg,
                         wParam,
                         lParam,
       (ULONG_PTR)&CallBackInfo,
       FNID_SENDMESSAGECALLBACK,
                          FALSE);
}

/*
 * @implemented
 */
LRESULT
WINAPI
SendMessageTimeoutA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  UINT fuFlags,
  UINT uTimeout,
  PDWORD_PTR lpdwResult)
{
  MSG AnsiMsg, UcMsg, KMMsg;
  LRESULT Result;
  DOSENDMESSAGE dsm;

  if ( Msg & ~WM_MAXIMUM || fuFlags & ~(SMTO_NOTIMEOUTIFNOTHUNG|SMTO_ABORTIFHUNG|SMTO_BLOCK))
  {
     SetLastError( ERROR_INVALID_PARAMETER );
     return 0;
  }

  if (lpdwResult) *lpdwResult = 0;

  SPY_EnterMessage(SPY_SENDMESSAGE, hWnd, Msg, wParam, lParam);

  dsm.uFlags = fuFlags;
  dsm.uTimeout = uTimeout;
  dsm.Result = 0;

  AnsiMsg.hwnd = hWnd;
  AnsiMsg.message = Msg;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;
  AnsiMsg.time = 0;
  AnsiMsg.pt.x = 0;
  AnsiMsg.pt.y = 0;

  if (! MsgiAnsiToUnicodeMessage(hWnd, &UcMsg, &AnsiMsg))
  {
      return FALSE;
  }

  if (!MsgiUMToKMMessage(&UcMsg, &KMMsg, FALSE))
  {
      MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);
      return FALSE;
  }

  Result = NtUserMessageCall( hWnd,
                              KMMsg.message,
                              KMMsg.wParam,
                              KMMsg.lParam,
                             (ULONG_PTR)&dsm,
                              FNID_SENDMESSAGEWTOOPTION,
                              TRUE);

  MsgiUMToKMCleanup(&UcMsg, &KMMsg);
  MsgiAnsiToUnicodeReply(&UcMsg, &AnsiMsg, &Result);

  if (lpdwResult) *lpdwResult = dsm.Result;

  SPY_ExitMessage(SPY_RESULT_OK, hWnd, Msg, Result, wParam, lParam);

  return Result;
}


/*
 * @implemented
 */
LRESULT
WINAPI
SendMessageTimeoutW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  UINT fuFlags,
  UINT uTimeout,
  PDWORD_PTR lpdwResult)
{
  LRESULT Result;
  DOSENDMESSAGE dsm;
  MSG UMMsg, KMMsg;

  if ( Msg & ~WM_MAXIMUM || fuFlags & ~(SMTO_NOTIMEOUTIFNOTHUNG|SMTO_ABORTIFHUNG|SMTO_BLOCK))
  {
     SetLastError( ERROR_INVALID_PARAMETER );
     return 0;
  }

  if (lpdwResult) *lpdwResult = 0;

  SPY_EnterMessage(SPY_SENDMESSAGE, hWnd, Msg, wParam, lParam);

  dsm.uFlags = fuFlags;
  dsm.uTimeout = uTimeout;
  dsm.Result = 0;

  UMMsg.hwnd = hWnd;
  UMMsg.message = Msg;
  UMMsg.wParam = wParam;
  UMMsg.lParam = lParam;
  UMMsg.time = 0;
  UMMsg.pt.x = 0;
  UMMsg.pt.y = 0;
  if (! MsgiUMToKMMessage(&UMMsg, &KMMsg, TRUE))
  {
     return FALSE;
  }

  Result = NtUserMessageCall( hWnd,
                              KMMsg.message,
                              KMMsg.wParam,
                              KMMsg.lParam,
                             (ULONG_PTR)&dsm,
                              FNID_SENDMESSAGEWTOOPTION,
                              FALSE);

  MsgiUMToKMCleanup(&UMMsg, &KMMsg);

  if (lpdwResult) *lpdwResult = dsm.Result;

  SPY_ExitMessage(SPY_RESULT_OK, hWnd, Msg, Result, wParam, lParam);

  return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
SendNotifyMessageA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  BOOL Ret;
  MSG AnsiMsg, UcMsg;

  if (is_pointer_message(Msg, wParam))
  {
     SetLastError( ERROR_MESSAGE_SYNC_ONLY );
     return FALSE;
  }

  AnsiMsg.hwnd = hWnd;
  AnsiMsg.message = Msg;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;
  AnsiMsg.time = 0;
  AnsiMsg.pt.x = 0;
  AnsiMsg.pt.y = 0;
  if (! MsgiAnsiToUnicodeMessage(hWnd, &UcMsg, &AnsiMsg))
  {
     return FALSE;
  }
  Ret = SendNotifyMessageW(hWnd, UcMsg.message, UcMsg.wParam, UcMsg.lParam);

  MsgiAnsiToUnicodeCleanup(&UcMsg, &AnsiMsg);

  return Ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
SendNotifyMessageW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  LRESULT Result;

  if (is_pointer_message(Msg, wParam))
  {
     SetLastError( ERROR_MESSAGE_SYNC_ONLY );
     return FALSE;
  }

  Result = NtUserMessageCall( hWnd,
                              Msg,
                              wParam,
                              lParam,
                              0,
                              FNID_SENDNOTIFYMESSAGE,
                              FALSE);

  return Result;
}

/*
 * @implemented
 */
BOOL WINAPI
TranslateMessageEx(CONST MSG *lpMsg, UINT Flags)
{
    switch (lpMsg->message)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            return(NtUserTranslateMessage((LPMSG)lpMsg, Flags));

        default:
            if ( lpMsg->message & ~WM_MAXIMUM )
               SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }
}


/*
 * @implemented
 */
BOOL WINAPI
TranslateMessage(CONST MSG *lpMsg)
{
    BOOL ret;

    // https://learn.microsoft.com/en-us/previous-versions/aa912145(v=msdn.10)
    if (LOWORD(lpMsg->wParam) == VK_PROCESSKEY)
    {
        ret = IMM_FN(ImmTranslateMessage)(lpMsg->hwnd,
                                          lpMsg->message,
                                          lpMsg->wParam,
                                          lpMsg->lParam);
        if (ret)
            return ret;
    }

    ret = TranslateMessageEx((LPMSG)lpMsg, 0);
    return ret;
}


/*
 * @implemented
 */
UINT WINAPI
RegisterWindowMessageA(LPCSTR lpString)
{
  UNICODE_STRING String;
  UINT Atom;

  if (!RtlCreateUnicodeStringFromAsciiz(&String, (PCSZ)lpString))
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
UINT WINAPI
RegisterWindowMessageW(LPCWSTR lpString)
{
  UNICODE_STRING String;

  RtlInitUnicodeString(&String, lpString);
  return(NtUserRegisterWindowMessage(&String));
}

/*
 * @implemented
 */
HWND WINAPI
GetCapture(VOID)
{
  return (HWND)NtUserGetThreadState(THREADSTATE_CAPTUREWINDOW);
}

/*
 * @implemented
 */
BOOL WINAPI
ReleaseCapture(VOID)
{
  return NtUserxReleaseCapture();
}


/*
 * @implemented
 */
DWORD
WINAPI
RealGetQueueStatus(UINT flags)
{
   if (flags & ~(QS_ALLINPUT|QS_ALLPOSTMESSAGE|QS_SMRESULT))
   {
      SetLastError( ERROR_INVALID_FLAGS );
      return 0;
   }
   /** ATM, we do not support QS_RAWINPUT, but we need to support apps that pass
    ** this flag along, while also working around QS_RAWINPUT checks in winetests.
    ** Just set the last error to ERROR_INVALID_FLAGS but do not fail the call.
    **/
   if (flags & QS_RAWINPUT)
   {
      SetLastError(ERROR_INVALID_FLAGS);
      flags &= ~QS_RAWINPUT;
   }
   /**/
   return NtUserxGetQueueStatus(flags);
}


/*
 * @implemented
 */
BOOL WINAPI GetInputState(VOID)
{
   PCLIENTTHREADINFO pcti = GetWin32ClientInfo()->pClientThreadInfo;

   if ((!pcti) || (pcti->fsChangeBits & (QS_KEY|QS_MOUSEBUTTON)))
      return (BOOL)NtUserGetThreadState(THREADSTATE_GETINPUTSTATE);

   return FALSE;
}


NTSTATUS WINAPI
User32CallWindowProcFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PWINDOWPROC_CALLBACK_ARGUMENTS CallbackArgs;
  MSG KMMsg, UMMsg;
  PWND pWnd = NULL;
  PCLIENTINFO pci = GetWin32ClientInfo();

  /* Make sure we don't try to access mem beyond what we were given */
  if (ArgumentLength < sizeof(WINDOWPROC_CALLBACK_ARGUMENTS))
    {
      return STATUS_INFO_LENGTH_MISMATCH;
    }

  CallbackArgs = (PWINDOWPROC_CALLBACK_ARGUMENTS) Arguments;
  KMMsg.hwnd = CallbackArgs->Wnd;
  KMMsg.message = CallbackArgs->Msg;
  KMMsg.wParam = CallbackArgs->wParam;
  KMMsg.time = 0;
  KMMsg.pt.x = 0;
  KMMsg.pt.y = 0;
  /* Check if lParam is really a pointer and adjust it if it is */
  if (0 <= CallbackArgs->lParamBufferSize)
    {
      if (ArgumentLength != sizeof(WINDOWPROC_CALLBACK_ARGUMENTS)
                            + CallbackArgs->lParamBufferSize)
        {
          return STATUS_INFO_LENGTH_MISMATCH;
        }
      KMMsg.lParam = (LPARAM) ((char *) CallbackArgs + sizeof(WINDOWPROC_CALLBACK_ARGUMENTS));
     switch(KMMsg.message)
     {
        case WM_CREATE:
        {
            TRACE("WM_CREATE CB %p lParam %p\n",CallbackArgs, KMMsg.lParam);
            break;
        }
        case WM_NCCREATE:
        {
            TRACE("WM_NCCREATE CB %p lParam %p\n",CallbackArgs, KMMsg.lParam);
            break;
        }
        case WM_SYSTIMER:
        {
            TRACE("WM_SYSTIMER %p\n",KMMsg.hwnd);
            break;
        }
        case WM_SIZING:
        {
           PRECT prect = (PRECT) KMMsg.lParam;
           TRACE("WM_SIZING 1 t %d l %d r %d b %d\n",prect->top,prect->left,prect->right,prect->bottom);
           break;
        }
        default:
           break;
     }
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

  if (pci->CallbackWnd.hWnd == UMMsg.hwnd)
     pWnd = pci->CallbackWnd.pWnd;

  CallbackArgs->Result = IntCallWindowProcW( CallbackArgs->IsAnsiProc,
                                             CallbackArgs->Proc,
                                             pWnd,
                                             UMMsg.hwnd,
                                             UMMsg.message,
                                             UMMsg.wParam,
                                             UMMsg.lParam);

  if (! MsgiKMToUMReply(&KMMsg, &UMMsg, &CallbackArgs->Result))
    {
    }

  if (0 <= CallbackArgs->lParamBufferSize)
  {
     switch(KMMsg.message)
     {
        case WM_SIZING:
        {
           PRECT prect = (PRECT) KMMsg.lParam;
           TRACE("WM_SIZING 2 t %d l %d r %d b %d\n",prect->top,prect->left,prect->right,prect->bottom);
           break;
        }
        default:
           break;
     }
  }
  return ZwCallbackReturn(CallbackArgs, ArgumentLength, STATUS_SUCCESS);
}

/*
 * @implemented
 */
BOOL WINAPI SetMessageQueue(int cMessagesMax)
{
  /* Function does nothing on 32 bit windows */
  return TRUE;
}
typedef DWORD (WINAPI * RealGetQueueStatusProc)(UINT flags);
typedef DWORD (WINAPI * RealMsgWaitForMultipleObjectsExProc)(DWORD nCount, CONST HANDLE *lpHandles, DWORD dwMilliseconds, DWORD dwWakeMask, DWORD dwFlags);
typedef BOOL (WINAPI * RealInternalGetMessageProc)(LPMSG,HWND,UINT,UINT,UINT,BOOL);
typedef BOOL (WINAPI * RealWaitMessageExProc)(DWORD,UINT);

typedef struct _USER_MESSAGE_PUMP_ADDRESSES {
	DWORD cbSize;
	RealInternalGetMessageProc NtUserRealInternalGetMessage;
	RealWaitMessageExProc NtUserRealWaitMessageEx;
	RealGetQueueStatusProc RealGetQueueStatus;
	RealMsgWaitForMultipleObjectsExProc RealMsgWaitForMultipleObjectsEx;
} USER_MESSAGE_PUMP_ADDRESSES, * PUSER_MESSAGE_PUMP_ADDRESSES;

DWORD
WINAPI
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
	NtUserRealInternalGetMessage,
	NtUserRealWaitMessageEx,
	RealGetQueueStatus,
	RealMsgWaitForMultipleObjectsEx
};

DWORD gfMessagePumpHook = 0;

BOOL WINAPI IsInsideMessagePumpHook()
{  // FF uses this and polls it when Min/Max
   PCLIENTTHREADINFO pcti = GetWin32ClientInfo()->pClientThreadInfo;
   return (gfMessagePumpHook && pcti && (pcti->dwcPumpHook > 0));
}

void WINAPI ResetMessagePumpHook(PUSER_MESSAGE_PUMP_ADDRESSES Addresses)
{
	Addresses->cbSize = sizeof(USER_MESSAGE_PUMP_ADDRESSES);
	Addresses->NtUserRealInternalGetMessage = NtUserRealInternalGetMessage;
	Addresses->NtUserRealWaitMessageEx = NtUserRealWaitMessageEx;
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
	if(NtUserxInitMessagePump()) {
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
		if(NtUserxUnInitMessagePump()) {
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

DWORD WINAPI
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
   PCLIENTINFO pci;
   PCLIENTTHREADINFO pcti;

   if (dwFlags & ~(MWMO_WAITALL | MWMO_ALERTABLE | MWMO_INPUTAVAILABLE))
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return WAIT_FAILED;
   }

   pci = GetWin32ClientInfo();
   if (!pci) return WAIT_FAILED;

   pcti = pci->pClientThreadInfo;
   if (pcti && ( !nCount || !(dwFlags & MWMO_WAITALL) ))
   {
      if ( (pcti->fsChangeBits & LOWORD(dwWakeMask)) ||
           ( (dwFlags & MWMO_INPUTAVAILABLE) && (pcti->fsWakeBits & LOWORD(dwWakeMask)) ) )
      {
         //FIXME("Return Chg 0x%x Wake 0x%x Mask 0x%x nCnt %d\n",pcti->fsChangeBits, pcti->fsWakeBits, dwWakeMask, nCount);
         return nCount;
      }
   }

   MessageQueueHandle = NtUserxMsqSetWakeMask(MAKELONG(dwWakeMask, dwFlags));
   if (MessageQueueHandle == NULL)
   {
      SetLastError(0); /* ? */
      return WAIT_FAILED;
   }

   RealHandles = HeapAlloc(GetProcessHeap(), 0, (nCount + 1) * sizeof(HANDLE));
   if (RealHandles == NULL)
   {
      NtUserxMsqClearWakeMask();
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return WAIT_FAILED;
   }

   RtlCopyMemory(RealHandles, pHandles, nCount * sizeof(HANDLE));
   RealHandles[nCount] = MessageQueueHandle;

   //FIXME("1 Chg 0x%x Wake 0x%x Mask 0x%x nCnt %d\n",pcti->fsChangeBits, pcti->fsWakeBits, dwWakeMask, nCount);

   Result = WaitForMultipleObjectsEx( nCount + 1,
                                      RealHandles,
                                      dwFlags & MWMO_WAITALL,
                                      dwMilliseconds,
                                      dwFlags & MWMO_ALERTABLE );

   //FIXME("2 Chg 0x%x Wake 0x%x Mask 0x%x nCnt %d\n",pcti->fsChangeBits, pcti->fsWakeBits, dwWakeMask, nCount);

   HeapFree(GetProcessHeap(), 0, RealHandles);
   NtUserxMsqClearWakeMask();

   // wine hack! MSDN: If dwMilliseconds is zero,,specified objects are not signaled; it always returns immediately.
   if (!Result && !nCount && !dwMilliseconds) Result = WAIT_TIMEOUT;

   //FIXME("Result 0X%x\n",Result);
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
DWORD WINAPI
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
  InitializeCriticalSection(&gcsMPH);

  return TRUE;
}

VOID FASTCALL MessageCleanup(VOID)
{
  DeleteCriticalSection(&DdeCrst);
  DeleteCriticalSection(&gcsMPH);
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

LONG
WINAPI
IntBroadcastSystemMessage(
    DWORD dwflags,
    LPDWORD lpdwRecipients,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    PBSMINFO pBSMInfo,
    BOOL Ansi)
{
    BROADCASTPARM parm;
    DWORD recips = BSM_ALLCOMPONENTS;
    BOOL ret = -1; // Set to return fail
    static const DWORD all_flags = ( BSF_QUERY | BSF_IGNORECURRENTTASK | BSF_FLUSHDISK | BSF_NOHANG
                                   | BSF_POSTMESSAGE | BSF_FORCEIFHUNG | BSF_NOTIMEOUTIFNOTHUNG
                                   | BSF_ALLOWSFW | BSF_SENDNOTIFYMESSAGE | BSF_RETURNHDESK | BSF_LUID );

    if ((dwflags & ~all_flags) ||
        (!pBSMInfo && (dwflags & (BSF_RETURNHDESK|BSF_LUID))) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if(uiMessage >= WM_USER && uiMessage < 0xC000)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (dwflags & BSF_FORCEIFHUNG) dwflags |= BSF_NOHANG;

    if (dwflags & BSF_QUERY) dwflags &= ~BSF_SENDNOTIFYMESSAGE|BSF_POSTMESSAGE;

    if (!lpdwRecipients)
        lpdwRecipients = &recips;

    if (*lpdwRecipients & ~(BSM_APPLICATIONS|BSM_ALLDESKTOPS|BSM_INSTALLABLEDRIVERS|BSM_NETDRIVER|BSM_VXDS))
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       return 0;
    }

    if ( pBSMInfo && (dwflags & BSF_QUERY) )
    {
       if (pBSMInfo->cbSize != sizeof(BSMINFO))
       {
           SetLastError(ERROR_INVALID_PARAMETER);
           return 0;
       }
    }

    parm.hDesk = NULL;
    parm.hWnd = NULL;
    parm.flags = dwflags;
    parm.recipients = *lpdwRecipients;

    if (dwflags & BSF_LUID) parm.luid = pBSMInfo->luid;

    ret = NtUserMessageCall(GetDesktopWindow(),
                                     uiMessage,
                                        wParam,
                                        lParam,
                              (ULONG_PTR)&parm,
                   FNID_BROADCASTSYSTEMMESSAGE,
                                          Ansi);

    if (!ret)
    {
       if ( pBSMInfo && (dwflags & BSF_QUERY) )
       {
          pBSMInfo->hdesk = parm.hDesk;
          pBSMInfo->hwnd = parm.hWnd;
       }
    }
    return ret;
}

/*
 * @implemented
 */
LONG
WINAPI
BroadcastSystemMessageA(
  DWORD dwFlags,
  LPDWORD lpdwRecipients,
  UINT uiMessage,
  WPARAM wParam,
  LPARAM lParam)
{
  return IntBroadcastSystemMessage( dwFlags, lpdwRecipients, uiMessage, wParam, lParam, NULL, TRUE );
}

/*
 * @implemented
 */
LONG
WINAPI
BroadcastSystemMessageW(
  DWORD dwFlags,
  LPDWORD lpdwRecipients,
  UINT uiMessage,
  WPARAM wParam,
  LPARAM lParam)
{
  return IntBroadcastSystemMessage( dwFlags, lpdwRecipients, uiMessage, wParam, lParam, NULL, FALSE );
}

/*
 * @implemented
 */
LONG
WINAPI
BroadcastSystemMessageExA(
    DWORD dwflags,
    LPDWORD lpdwRecipients,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    PBSMINFO pBSMInfo)
{
  return IntBroadcastSystemMessage( dwflags, lpdwRecipients, uiMessage, wParam, lParam , pBSMInfo, TRUE );
}

/*
 * @implemented
 */
LONG
WINAPI
BroadcastSystemMessageExW(
    DWORD dwflags,
    LPDWORD lpdwRecipients,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    PBSMINFO pBSMInfo)
{
  return IntBroadcastSystemMessage( dwflags, lpdwRecipients, uiMessage, wParam, lParam , pBSMInfo, FALSE );
}

