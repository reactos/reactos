/*
* COPYRIGHT:        See COPYING in the top level directory
* PROJECT:          ReactOS kernel
* PURPOSE:          Messages
* FILE:             subsys/win32k/ntuser/message.c
* PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
* REVISION HISTORY:
*       06-06-2001  CSH  Created
*/

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

BOOLEAN NTAPI PsGetProcessExitProcessCalled(PEPROCESS Process);

#define PM_BADMSGFLAGS ~((QS_RAWINPUT << 16)|PM_QS_SENDMESSAGE|PM_QS_PAINT|PM_QS_POSTMESSAGE|PM_QS_INPUT|PM_NOYIELD|PM_REMOVE)

typedef struct
{
    UINT uFlags;
    UINT uTimeout;
    ULONG_PTR Result;
}
DOSENDMESSAGE, *PDOSENDMESSAGE;

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
IntInitMessageImpl(VOID)
{
    return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IntCleanupMessageImpl(VOID)
{
    return STATUS_SUCCESS;
}

#define MMS_SIZE_WPARAM      -1
#define MMS_SIZE_WPARAMWCHAR -2
#define MMS_SIZE_LPARAMSZ    -3
#define MMS_SIZE_SPECIAL     -4
#define MMS_FLAG_READ        0x01
#define MMS_FLAG_WRITE       0x02
#define MMS_FLAG_READWRITE   (MMS_FLAG_READ | MMS_FLAG_WRITE)
typedef struct tagMSGMEMORY
{
    UINT Message;
    UINT Size;
    INT Flags;
}
MSGMEMORY, *PMSGMEMORY;

static MSGMEMORY MsgMemory[] =
{
    { WM_CREATE, MMS_SIZE_SPECIAL, MMS_FLAG_READWRITE },
    { WM_DDE_ACK, sizeof(KMDDELPARAM), MMS_FLAG_READ },
    { WM_DDE_EXECUTE, MMS_SIZE_WPARAM, MMS_FLAG_READ },
    { WM_GETMINMAXINFO, sizeof(MINMAXINFO), MMS_FLAG_READWRITE },
    { WM_GETTEXT, MMS_SIZE_WPARAMWCHAR, MMS_FLAG_WRITE },
    { WM_NCCALCSIZE, MMS_SIZE_SPECIAL, MMS_FLAG_READWRITE },
    { WM_NCCREATE, MMS_SIZE_SPECIAL, MMS_FLAG_READWRITE },
    { WM_SETTEXT, MMS_SIZE_LPARAMSZ, MMS_FLAG_READ },
    { WM_STYLECHANGED, sizeof(STYLESTRUCT), MMS_FLAG_READ },
    { WM_STYLECHANGING, sizeof(STYLESTRUCT), MMS_FLAG_READWRITE },
    { WM_COPYDATA, MMS_SIZE_SPECIAL, MMS_FLAG_READ },
    { WM_WINDOWPOSCHANGED, sizeof(WINDOWPOS), MMS_FLAG_READ },
    { WM_WINDOWPOSCHANGING, sizeof(WINDOWPOS), MMS_FLAG_READWRITE },
};

static PMSGMEMORY FASTCALL
FindMsgMemory(UINT Msg)
{
    PMSGMEMORY MsgMemoryEntry;

    /* See if this message type is present in the table */
    for (MsgMemoryEntry = MsgMemory;
    MsgMemoryEntry < MsgMemory + sizeof(MsgMemory) / sizeof(MSGMEMORY);
    MsgMemoryEntry++)
    {
        if (Msg == MsgMemoryEntry->Message)
        {
            return MsgMemoryEntry;
        }
    }

    return NULL;
}

static UINT FASTCALL
MsgMemorySize(PMSGMEMORY MsgMemoryEntry, WPARAM wParam, LPARAM lParam)
{
    CREATESTRUCTW *Cs;
    PUNICODE_STRING WindowName;
    PUNICODE_STRING ClassName;
    UINT Size = 0;

    _SEH2_TRY
    {
        if (MMS_SIZE_WPARAM == MsgMemoryEntry->Size)
        {
            Size = (UINT)wParam;
        }
        else if (MMS_SIZE_WPARAMWCHAR == MsgMemoryEntry->Size)
        {
            Size = (UINT) (wParam * sizeof(WCHAR));
        }
        else if (MMS_SIZE_LPARAMSZ == MsgMemoryEntry->Size)
        {
            Size = (UINT) ((wcslen((PWSTR) lParam) + 1) * sizeof(WCHAR));
        }
        else if (MMS_SIZE_SPECIAL == MsgMemoryEntry->Size)
        {
            switch(MsgMemoryEntry->Message)
            {
            case WM_CREATE:
            case WM_NCCREATE:
                Cs = (CREATESTRUCTW *) lParam;
                WindowName = (PUNICODE_STRING) Cs->lpszName;
                ClassName = (PUNICODE_STRING) Cs->lpszClass;
                Size = sizeof(CREATESTRUCTW) + WindowName->Length + sizeof(WCHAR);
                if (IS_ATOM(ClassName->Buffer))
                {
                    Size += sizeof(WCHAR) + sizeof(ATOM);
                }
                else
                {
                    Size += sizeof(WCHAR) + ClassName->Length + sizeof(WCHAR);
                }
                break;

            case WM_NCCALCSIZE:
                Size = wParam ? sizeof(NCCALCSIZE_PARAMS) + sizeof(WINDOWPOS) : sizeof(RECT);
                break;

            case WM_COPYDATA:
                Size = sizeof(COPYDATASTRUCT) + ((PCOPYDATASTRUCT)lParam)->cbData;
                break;

            case WM_COPYGLOBALDATA:
                Size = wParam;
                break;

            default:
                ASSERT(FALSE);
                Size = 0;
                break;
            }
        }
        else
        {
            Size = MsgMemoryEntry->Size;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("Exception caught in MsgMemorySize()! Status: 0x%x\n", _SEH2_GetExceptionCode());
        Size = 0;
    }
    _SEH2_END;
    return Size;
}

static NTSTATUS
PackParam(LPARAM *lParamPacked, UINT Msg, WPARAM wParam, LPARAM lParam, BOOL NonPagedPoolNeeded)
{
    NCCALCSIZE_PARAMS *UnpackedNcCalcsize;
    NCCALCSIZE_PARAMS *PackedNcCalcsize;
    CREATESTRUCTW *UnpackedCs;
    CREATESTRUCTW *PackedCs;
    PLARGE_STRING WindowName;
    PUNICODE_STRING ClassName;
    POOL_TYPE PoolType;
    UINT Size;
    PCHAR CsData;

    *lParamPacked = lParam;

    if (NonPagedPoolNeeded)
        PoolType = NonPagedPool;
    else
        PoolType = PagedPool;

    if (WM_NCCALCSIZE == Msg && wParam)
    {

        UnpackedNcCalcsize = (NCCALCSIZE_PARAMS *) lParam;
        PackedNcCalcsize = ExAllocatePoolWithTag(PoolType,
        sizeof(NCCALCSIZE_PARAMS) + sizeof(WINDOWPOS),
        TAG_MSG);

        if (NULL == PackedNcCalcsize)
        {
            DPRINT1("Not enough memory to pack lParam\n");
            return STATUS_NO_MEMORY;
        }
        RtlCopyMemory(PackedNcCalcsize, UnpackedNcCalcsize, sizeof(NCCALCSIZE_PARAMS));
        PackedNcCalcsize->lppos = (PWINDOWPOS) (PackedNcCalcsize + 1);
        RtlCopyMemory(PackedNcCalcsize->lppos, UnpackedNcCalcsize->lppos, sizeof(WINDOWPOS));
        *lParamPacked = (LPARAM) PackedNcCalcsize;
    }
    else if (WM_CREATE == Msg || WM_NCCREATE == Msg)
    {
        UnpackedCs = (CREATESTRUCTW *) lParam;
        WindowName = (PLARGE_STRING) UnpackedCs->lpszName;
        ClassName = (PUNICODE_STRING) UnpackedCs->lpszClass;
        Size = sizeof(CREATESTRUCTW) + WindowName->Length + sizeof(WCHAR);
        if (IS_ATOM(ClassName->Buffer))
        {
            Size += sizeof(WCHAR) + sizeof(ATOM);
        }
        else
        {
            Size += sizeof(WCHAR) + ClassName->Length + sizeof(WCHAR);
        }
        PackedCs = ExAllocatePoolWithTag(PoolType, Size, TAG_MSG);
        if (NULL == PackedCs)
        {
            DPRINT1("Not enough memory to pack lParam\n");
            return STATUS_NO_MEMORY;
        }
        RtlCopyMemory(PackedCs, UnpackedCs, sizeof(CREATESTRUCTW));
        CsData = (PCHAR) (PackedCs + 1);
        PackedCs->lpszName = (LPCWSTR) (CsData - (PCHAR) PackedCs);
        RtlCopyMemory(CsData, WindowName->Buffer, WindowName->Length);
        CsData += WindowName->Length;
        *((WCHAR *) CsData) = L'\0';
        CsData += sizeof(WCHAR);
        PackedCs->lpszClass = (LPCWSTR) (CsData - (PCHAR) PackedCs);
        if (IS_ATOM(ClassName->Buffer))
        {
            *((WCHAR *) CsData) = L'A';
            CsData += sizeof(WCHAR);
            *((ATOM *) CsData) = (ATOM)(DWORD_PTR) ClassName->Buffer;
            CsData += sizeof(ATOM);
        }
        else
        {
            *((WCHAR *) CsData) = L'S';
            CsData += sizeof(WCHAR);
            RtlCopyMemory(CsData, ClassName->Buffer, ClassName->Length);
            CsData += ClassName->Length;
            *((WCHAR *) CsData) = L'\0';
            CsData += sizeof(WCHAR);
        }
        ASSERT(CsData == (PCHAR) PackedCs + Size);
        *lParamPacked = (LPARAM) PackedCs;
    }

    else if (PoolType == NonPagedPool)
    {
        PMSGMEMORY MsgMemoryEntry;
        PVOID PackedData;

        MsgMemoryEntry = FindMsgMemory(Msg);

        if ((!MsgMemoryEntry) || (MsgMemoryEntry->Size < 0))
        {
            /* Keep previous behavior */
            return STATUS_SUCCESS;
        }
        PackedData = ExAllocatePoolWithTag(NonPagedPool, MsgMemorySize(MsgMemoryEntry, wParam, lParam), TAG_MSG);
        RtlCopyMemory(PackedData, (PVOID)lParam, MsgMemorySize(MsgMemoryEntry, wParam, lParam));
        *lParamPacked = (LPARAM)PackedData;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
UnpackParam(LPARAM lParamPacked, UINT Msg, WPARAM wParam, LPARAM lParam, BOOL NonPagedPoolUsed)
{
    NCCALCSIZE_PARAMS *UnpackedParams;
    NCCALCSIZE_PARAMS *PackedParams;
    PWINDOWPOS UnpackedWindowPos;

    if (lParamPacked == lParam)
    {
        return STATUS_SUCCESS;
    }

    if (WM_NCCALCSIZE == Msg && wParam)
    {
        PackedParams = (NCCALCSIZE_PARAMS *) lParamPacked;
        UnpackedParams = (NCCALCSIZE_PARAMS *) lParam;
        UnpackedWindowPos = UnpackedParams->lppos;
        RtlCopyMemory(UnpackedParams, PackedParams, sizeof(NCCALCSIZE_PARAMS));
        UnpackedParams->lppos = UnpackedWindowPos;
        RtlCopyMemory(UnpackedWindowPos, PackedParams + 1, sizeof(WINDOWPOS));
        ExFreePool((PVOID) lParamPacked);

        return STATUS_SUCCESS;
    }
    else if (WM_CREATE == Msg || WM_NCCREATE == Msg)
    {
        ExFreePool((PVOID) lParamPacked);

        return STATUS_SUCCESS;
    }
    else if (NonPagedPoolUsed)
    {
        PMSGMEMORY MsgMemoryEntry;
        MsgMemoryEntry = FindMsgMemory(Msg);
        if (MsgMemoryEntry->Size < 0)
        {
            /* Keep previous behavior */
            return STATUS_INVALID_PARAMETER;
        }

        if (MsgMemory->Flags == MMS_FLAG_READWRITE)
        {
            //RtlCopyMemory((PVOID)lParam, (PVOID)lParamPacked, MsgMemory->Size);
        }
        ExFreePool((PVOID) lParamPacked);
        return STATUS_SUCCESS;
    }

    ASSERT(FALSE);

    return STATUS_INVALID_PARAMETER;
}

//
// Wakeup any thread/process waiting on idle input.
//
VOID FASTCALL
IdlePing(VOID)
{
   PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();
   PUSER_MESSAGE_QUEUE ForegroundQueue;
   PTHREADINFO pti, ptiForeground = NULL;

   ForegroundQueue = IntGetFocusMessageQueue();

   if (ForegroundQueue)
      ptiForeground = ForegroundQueue->Thread->Tcb.Win32Thread;
     
   pti = PsGetCurrentThreadWin32Thread();

   if ( pti && pti->pDeskInfo && pti == ptiForeground )
   {
      if ( pti->fsHooks & HOOKID_TO_FLAG(WH_FOREGROUNDIDLE) ||
           pti->pDeskInfo->fsHooks & HOOKID_TO_FLAG(WH_FOREGROUNDIDLE) )
      {
         co_HOOK_CallHooks(WH_FOREGROUNDIDLE,HC_ACTION,0,0);
      }
   }

   DPRINT("IdlePing ppi 0x%x\n",ppi);
   if ( ppi && ppi->InputIdleEvent )
   {
      DPRINT("InputIdleEvent\n");
      KeSetEvent( ppi->InputIdleEvent, IO_NO_INCREMENT, FALSE);
   }
}

VOID FASTCALL
IdlePong(VOID)
{
   PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();

   DPRINT("IdlePong ppi 0x%x\n",ppi);
   if ( ppi && ppi->InputIdleEvent )
   {
      KeClearEvent(ppi->InputIdleEvent);
   }
}

static VOID FASTCALL
IntCallWndProc( PWND Window, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    BOOL SameThread = FALSE;
    CWPSTRUCT CWP;

    if (Window->head.pti == ((PTHREADINFO)PsGetCurrentThreadWin32Thread()))
        SameThread = TRUE;

    CWP.hwnd    = hWnd;
    CWP.message = Msg;
    CWP.wParam  = wParam;
    CWP.lParam  = lParam;
    co_HOOK_CallHooks( WH_CALLWNDPROC, HC_ACTION, SameThread, (LPARAM)&CWP );
}

static VOID FASTCALL
IntCallWndProcRet ( PWND Window, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *uResult)
{
    BOOL SameThread = FALSE;
    CWPRETSTRUCT CWPR;

    if (Window->head.pti == ((PTHREADINFO)PsGetCurrentThreadWin32Thread()))
        SameThread = TRUE;

    CWPR.hwnd    = hWnd;
    CWPR.message = Msg;
    CWPR.wParam  = wParam;
    CWPR.lParam  = lParam;
    CWPR.lResult = *uResult;
    co_HOOK_CallHooks( WH_CALLWNDPROCRET, HC_ACTION, SameThread, (LPARAM)&CWPR );
}

LRESULT FASTCALL
IntDispatchMessage(PMSG pMsg)
{
    LARGE_INTEGER TickCount;
    LONG Time;
    LRESULT retval = 0;
    PMSGMEMORY MsgMemoryEntry;
    INT lParamBufferSize;
    PTHREADINFO pti;
    LPARAM lParamPacked;
    PWND Window = NULL;

    if (pMsg->hwnd)
    {
        Window = UserGetWindowObject(pMsg->hwnd);
        if (!Window) return 0;
    }

    pti = PsGetCurrentThreadWin32Thread();

    if (((pMsg->message == WM_SYSTIMER) ||
         (pMsg->message == WM_TIMER)) &&
         (pMsg->lParam) )
    {
        if (pMsg->message == WM_TIMER)
        {
            ObReferenceObject(pti->pEThread);
            if (ValidateTimerCallback(pti,pMsg->lParam))
            {
                KeQueryTickCount(&TickCount);
                Time = MsqCalculateMessageTime(&TickCount);
                retval = co_IntCallWindowProc((WNDPROC)pMsg->lParam,
                                              TRUE,
                                              pMsg->hwnd,
                                              WM_TIMER,
                                              pMsg->wParam,
                                              (LPARAM)Time,
                                              sizeof(LPARAM));
            }
            ObDereferenceObject(pti->pEThread);
            return retval;        
        }
        else
        {
            PTIMER pTimer = FindSystemTimer(pMsg);
            if (pTimer && pTimer->pfn)
            {
                KeQueryTickCount(&TickCount);
                Time = MsqCalculateMessageTime(&TickCount);
                pTimer->pfn(pMsg->hwnd, WM_SYSTIMER, (UINT)pMsg->wParam, Time);
            }
            return 0;
        }
    }
    // Need a window!
    if ( !Window ) return 0;

    /* See if this message type is present in the table */
    MsgMemoryEntry = FindMsgMemory(pMsg->message);
    if ( !MsgMemoryEntry )
    {
        lParamBufferSize = -1;
    }
    else
    {
        lParamBufferSize = MsgMemorySize(MsgMemoryEntry, pMsg->wParam, pMsg->lParam);
    }

    if (! NT_SUCCESS(PackParam(&lParamPacked, pMsg->message, pMsg->wParam, pMsg->lParam, FALSE)))
    {
        DPRINT1("Failed to pack message parameters\n");
        return 0;
    }
    ObReferenceObject(pti->pEThread);
    retval = co_IntCallWindowProc( Window->lpfnWndProc,
                                   !Window->Unicode,
                                   pMsg->hwnd,
                                   pMsg->message,
                                   pMsg->wParam,
                                   lParamPacked,
                                   lParamBufferSize);

    if (! NT_SUCCESS(UnpackParam(lParamPacked, pMsg->message, pMsg->wParam, pMsg->lParam, FALSE)))
    {
        DPRINT1("Failed to unpack message parameters\n");
    }

    if (pMsg->message == WM_PAINT)
    {
        /* send a WM_NCPAINT and WM_ERASEBKGND if the non-client area is still invalid */
        HRGN hrgn = IntSysCreateRectRgn( 0, 0, 0, 0 );
        co_UserGetUpdateRgn( Window, hrgn, TRUE );
        REGION_FreeRgnByHandle( hrgn );
    }
    ObDereferenceObject(pti->pEThread);
    return retval;
}

VOID FASTCALL
co_IntSendHitTestMessages(PUSER_MESSAGE_QUEUE ThreadQueue, LPMSG Msg)
{
    if(!Msg->hwnd || ThreadQueue->CaptureWindow)
    {
        return;
    }

    switch(Msg->message)
    {
    case WM_MOUSEMOVE:
        {
            co_IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(HTCLIENT, Msg->message));
            break;
        }
    case WM_NCMOUSEMOVE:
        {
            co_IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(Msg->wParam, Msg->message));
            break;
        }
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_XBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_XBUTTONDBLCLK:
        {
            WPARAM wParam;
            PSYSTEM_CURSORINFO CurInfo;
            CurInfo = IntGetSysCursorInfo();

            wParam = (WPARAM)(CurInfo->ButtonsDown);

            co_IntSendMessage(Msg->hwnd, WM_MOUSEMOVE, wParam, Msg->lParam);
            co_IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(HTCLIENT, Msg->message));
            break;
        }
    case WM_NCLBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
    case WM_NCXBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCXBUTTONDBLCLK:
        {
            co_IntSendMessage(Msg->hwnd, WM_NCMOUSEMOVE, (WPARAM)Msg->wParam, Msg->lParam);
            co_IntSendMessage(Msg->hwnd, WM_SETCURSOR, (WPARAM)Msg->hwnd, MAKELPARAM(Msg->wParam, Msg->message));
            break;
        }
    }
}

BOOL FASTCALL
co_IntActivateWindowMouse( PUSER_MESSAGE_QUEUE ThreadQueue,
                           LPMSG Msg,
                           PWND MsgWindow,
                           USHORT *HitTest)
{
    ULONG Result;
    PWND Parent;

    ASSERT_REFS_CO(MsgWindow);

    if(*HitTest == (USHORT)HTTRANSPARENT)
    {
        /* eat the message, search again! */
        return TRUE;
    }

    Parent = IntGetParent(MsgWindow);//fixme: deref retval?

    /* If no parent window, pass MsgWindows HWND as wParam. Fixes bug #3111 */
    Result = co_IntSendMessage(MsgWindow->head.h,
                               WM_MOUSEACTIVATE,
                              (WPARAM) (Parent ? Parent->head.h : MsgWindow->head.h),
                              (LPARAM)MAKELONG(*HitTest, Msg->message)
    );

    switch (Result)
    {
    case MA_NOACTIVATEANDEAT:
        return TRUE;
    case MA_NOACTIVATE:
        break;
    case MA_ACTIVATEANDEAT:
        co_IntMouseActivateWindow(MsgWindow);
        return TRUE;
    default:
        /* MA_ACTIVATE */
        co_IntMouseActivateWindow(MsgWindow);
        break;
    }

    return FALSE;
}

BOOL FASTCALL
co_IntTranslateMouseMessage( PUSER_MESSAGE_QUEUE ThreadQueue,
                             LPMSG Msg,
                             USHORT *HitTest,
                             BOOL Remove)
{
    PWND Window;
    USER_REFERENCE_ENTRY Ref, DesktopRef;

    if(!(Window = UserGetWindowObject(Msg->hwnd)))
    {
        /* let's just eat the message?! */
        return TRUE;
    }

    *HitTest = HTCLIENT;

    UserRefObjectCo(Window, &Ref);

    if ( ThreadQueue == Window->head.pti->MessageQueue &&
         ThreadQueue->CaptureWindow != Window->head.h)
    {
        /* only send WM_NCHITTEST messages if we're not capturing the window! */
        if (Remove ) 
        {
            *HitTest = co_IntSendMessage(Window->head.h, WM_NCHITTEST, 0,
                                         MAKELONG(Msg->pt.x, Msg->pt.y));
        } 
        /* else we are going to see this message again, but then with Remove == TRUE */

        if (*HitTest == (USHORT)HTTRANSPARENT)
        {
            PWND DesktopWindow;
            HWND hDesktop = IntGetDesktopWindow();

            if ((DesktopWindow = UserGetWindowObject(hDesktop)))
            {
                PWND Wnd;

                UserRefObjectCo(DesktopWindow, &DesktopRef);

                co_WinPosWindowFromPoint(DesktopWindow, Window->head.pti->MessageQueue, &Msg->pt, &Wnd);
                if (Wnd)
                {
                    if (Wnd != Window)
                    {
                        /* post the message to the other window */
                        Msg->hwnd = Wnd->head.h;
                        if(!(Wnd->state & WNDS_DESTROYED))
                        {
                            MsqPostMessage(Wnd->head.pti->MessageQueue, Msg, FALSE,
                                           Msg->message == WM_MOUSEMOVE ? QS_MOUSEMOVE :
                            QS_MOUSEBUTTON);
                        }

                        /* eat the message */
                        UserDereferenceObject(Wnd);
                        UserDerefObjectCo(DesktopWindow);
                        UserDerefObjectCo(Window);
                        return TRUE;
                    }
                    UserDereferenceObject(Wnd);
                }

                UserDerefObjectCo(DesktopWindow);
            }
        }
    }

    if ( gspv.bMouseClickLock &&
        ((Msg->message == WM_LBUTTONUP) ||
         (Msg->message == WM_LBUTTONDOWN) ) )
    {
        if (MsqIsClkLck(Msg, Remove))
        {
            // FIXME: drop the message, hack: use WM_NULL
            Msg->message = WM_NULL;
        }
    }

    if (IS_BTN_MESSAGE(Msg->message, DOWN))
    {
        /* generate double click messages, if necessary */
        if ((((*HitTest) != HTCLIENT) ||
            (Window->pcls->style & CS_DBLCLKS)) &&
            MsqIsDblClk(Msg, Remove))
        {
            Msg->message += WM_LBUTTONDBLCLK - WM_LBUTTONDOWN;
        }
    }

    if(Msg->message != WM_MOUSEWHEEL)
    {

        if ((*HitTest) != HTCLIENT)
        {
            Msg->message += WM_NCMOUSEMOVE - WM_MOUSEMOVE;
            if ( (Msg->message == WM_NCRBUTTONUP) &&
                 (((*HitTest) == HTCAPTION) || ((*HitTest) == HTSYSMENU)) )
            {
                Msg->message = WM_CONTEXTMENU;
                Msg->wParam = (WPARAM)Window->head.h;
            }
            else
            {
                Msg->wParam = *HitTest;
            }
            Msg->lParam = MAKELONG(Msg->pt.x, Msg->pt.y);
        }
        else if ( ThreadQueue->MoveSize == NULL &&
                  ThreadQueue->MenuOwner == NULL )
        {
            /* NOTE: Msg->pt should remain in screen coordinates. -- FiN */
            Msg->lParam = MAKELONG(
                     Msg->pt.x - (WORD)Window->rcClient.left,
                     Msg->pt.y - (WORD)Window->rcClient.top);
        }
    }

    UserDerefObjectCo(Window);
    return FALSE;
}

BOOL ProcessMouseMessage(MSG* Msg, BOOLEAN RemoveMessages)
{
    MOUSEHOOKSTRUCT MHook;
    EVENTMSG Event;
    PTHREADINFO pti;
    PUSER_MESSAGE_QUEUE ThreadQueue;
    USER_REFERENCE_ENTRY Ref;
    USHORT HitTest = HTNOWHERE;

    pti = PsGetCurrentThreadWin32Thread();
    ThreadQueue = pti->MessageQueue;

    if(RemoveMessages)
    {
        PWND MsgWindow = NULL;

        /* Mouse message process */

        if( Msg->hwnd &&
            ( MsgWindow = UserGetWindowObject(Msg->hwnd) ) &&
            Msg->message >= WM_MOUSEFIRST &&
            Msg->message <= WM_MOUSELAST )
        {
            USHORT HitTest;

            UserRefObjectCo(MsgWindow, &Ref);

            if ( co_IntTranslateMouseMessage( ThreadQueue,
                                              Msg,
                                             &HitTest,
                                              TRUE))
            /* FIXME - check message filter again, if the message doesn't match anymore,
                    search again */
            {
                UserDerefObjectCo(MsgWindow);
                /* eat the message, search again */
                return FALSE;
            }

            if(ThreadQueue->CaptureWindow == NULL)
            {
                co_IntSendHitTestMessages(ThreadQueue, Msg);

                if ( ( Msg->message != WM_MOUSEMOVE &&
                       Msg->message != WM_NCMOUSEMOVE ) &&
                       IS_BTN_MESSAGE(Msg->message, DOWN) &&
                       co_IntActivateWindowMouse(ThreadQueue, Msg, MsgWindow, &HitTest) )
                {
                    UserDerefObjectCo(MsgWindow);
                    /* eat the message, search again */
                    return FALSE;
                }
            }

            UserDerefObjectCo(MsgWindow);
        }
        else
        {
            co_IntSendHitTestMessages(ThreadQueue, Msg);
        }

        return TRUE;
    }

    if ( ( Msg->hwnd &&
           Msg->message >= WM_MOUSEFIRST &&
           Msg->message <= WM_MOUSELAST ) &&
           co_IntTranslateMouseMessage( ThreadQueue,
                                        Msg,
                                       &HitTest,
                                        FALSE) )
    /* FIXME - check message filter again, if the message doesn't match anymore,
            search again */
    {
        /* eat the message, search again */
        return FALSE;
    }

    pti->rpdesk->htEx = HitTest; /* Now set the capture hit. */

    Event.message = Msg->message;
    Event.time    = Msg->time;
    Event.hwnd    = Msg->hwnd;
    Event.paramL  = Msg->pt.x;
    Event.paramH  = Msg->pt.y;
    co_HOOK_CallHooks( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)&Event);


    MHook.pt           = Msg->pt;
    MHook.hwnd         = Msg->hwnd;
    MHook.wHitTestCode = HitTest;
    MHook.dwExtraInfo  = 0;
    if (co_HOOK_CallHooks( WH_MOUSE,
                           RemoveMessages ? HC_ACTION : HC_NOREMOVE,
                           Msg->message,
                           (LPARAM)&MHook ))
    {
        MHook.pt           = Msg->pt;
        MHook.hwnd         = Msg->hwnd;
        MHook.wHitTestCode = HitTest;
        MHook.dwExtraInfo  = 0;
        co_HOOK_CallHooks( WH_CBT,
                           HCBT_CLICKSKIPPED,
                           Msg->message,
                           (LPARAM)&MHook);
        DPRINT1("MouseMessage WH_CBT Call Hook return!\n");
        return FALSE;
    }

    return TRUE;
}

BOOL ProcessKeyboardMessage(MSG* Msg, BOOLEAN RemoveMessages)
{
    EVENTMSG Event;

    Event.message = Msg->message;
    Event.hwnd    = Msg->hwnd;
    Event.time    = Msg->time;
    Event.paramL  = (Msg->wParam & 0xFF) | (HIWORD(Msg->lParam) << 8);
    Event.paramH  = Msg->lParam & 0x7FFF;
    if (HIWORD(Msg->lParam) & 0x0100) Event.paramH |= 0x8000;
    co_HOOK_CallHooks( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)&Event);

    if (co_HOOK_CallHooks( WH_KEYBOARD,
                           RemoveMessages ? HC_ACTION : HC_NOREMOVE,
                           LOWORD(Msg->wParam),
                           Msg->lParam))
    {
        /* skip this message */
        co_HOOK_CallHooks( WH_CBT,
                           HCBT_KEYSKIPPED,
                           LOWORD(Msg->wParam),
                           Msg->lParam );
        DPRINT1("KeyboardMessage WH_CBT Call Hook return!\n");
        return FALSE;
    }
    return TRUE;
}

BOOL ProcessHardwareMessage(MSG* Msg, BOOLEAN RemoveMessages)
{
    if ( IS_MOUSE_MESSAGE(Msg->message))
    {
        if (!ProcessMouseMessage(Msg, RemoveMessages))
        {
            return FALSE;
        }
    }
    else if ( IS_KBD_MESSAGE(Msg->message))
    {
        if(!ProcessKeyboardMessage(Msg, RemoveMessages))
        {
            return FALSE;
        }
    }

    return TRUE;
}
/*
* Internal version of PeekMessage() doing all the work
*/
BOOL FASTCALL
co_IntPeekMessage( PMSG Msg,
                   PWND Window,
                   UINT MsgFilterMin,
                   UINT MsgFilterMax,
                   UINT RemoveMsg )
{
    PTHREADINFO pti;
    LARGE_INTEGER LargeTickCount;
    PUSER_MESSAGE_QUEUE ThreadQueue;
    BOOL RemoveMessages;

    pti = PsGetCurrentThreadWin32Thread();
    ThreadQueue = pti->MessageQueue;

    RemoveMessages = RemoveMsg & PM_REMOVE;

    IdlePong();

    do
    {
        KeQueryTickCount(&LargeTickCount);
        ThreadQueue->LastMsgRead = LargeTickCount.u.LowPart;

        /* Dispatch sent messages here. */
        while (co_MsqDispatchOneSentMessage(ThreadQueue)) ;

        /* Now look for a quit message. */

        if (ThreadQueue->QuitPosted)
        {
            /* According to the PSDK, WM_QUIT messages are always returned, regardless
        of the filter specified */
            Msg->hwnd = NULL;
            Msg->message = WM_QUIT;
            Msg->wParam = ThreadQueue->QuitExitCode;
            Msg->lParam = 0;
            if (RemoveMessages)
            {
                ThreadQueue->QuitPosted = FALSE;
            }

            return TRUE;
        }

        /* Now check for normal messages. */
        if (MsqPeekMessage( ThreadQueue,
                            RemoveMessages,
                            Window,
                            MsgFilterMin,
                            MsgFilterMax,
                            Msg ))
        {
            return TRUE;
        }

        /* Check for hardware events. */
        if(co_MsqPeekHardwareMessage( ThreadQueue,
                                     RemoveMessages, 
                                     Window, 
                                     MsgFilterMin, 
                                     MsgFilterMax, 
                                     Msg))
        {

            if(!ProcessHardwareMessage(Msg, RemoveMessages))
                continue;

            return TRUE;
        }

        /* Check for sent messages again. */
        while (co_MsqDispatchOneSentMessage(ThreadQueue))
        ;

        /* Check for paint messages. */
        if( IntGetPaintMessage( Window,
                                MsgFilterMin,
                                MsgFilterMax,
                                pti,
                                Msg,
                                RemoveMessages))
        {
            return TRUE;
        }

        if (PostTimerMessages(Window))
        {
            continue;
        }

        return FALSE;
    }
    while (TRUE);

    return TRUE;
}

static NTSTATUS FASTCALL
CopyMsgToKernelMem(MSG *KernelModeMsg, MSG *UserModeMsg, PMSGMEMORY MsgMemoryEntry)
{
    NTSTATUS Status;

    PVOID KernelMem;
    UINT Size;

    *KernelModeMsg = *UserModeMsg;

    /* See if this message type is present in the table */
    if (NULL == MsgMemoryEntry)
    {
        /* Not present, no copying needed */
        return STATUS_SUCCESS;
    }

    /* Determine required size */
    Size = MsgMemorySize(MsgMemoryEntry, UserModeMsg->wParam, UserModeMsg->lParam);

    if (0 != Size)
    {
        /* Allocate kernel mem */
        KernelMem = ExAllocatePoolWithTag(PagedPool, Size, TAG_MSG);
        if (NULL == KernelMem)
        {
            DPRINT1("Not enough memory to copy message to kernel mem\n");
            return STATUS_NO_MEMORY;
        }
        KernelModeMsg->lParam = (LPARAM) KernelMem;

        /* Copy data if required */
        if (0 != (MsgMemoryEntry->Flags & MMS_FLAG_READ))
        {
            Status = MmCopyFromCaller(KernelMem, (PVOID) UserModeMsg->lParam, Size);
            if (! NT_SUCCESS(Status))
            {
                DPRINT1("Failed to copy message to kernel: invalid usermode buffer\n");
                ExFreePoolWithTag(KernelMem, TAG_MSG);
                return Status;
            }
        }
        else
        {
            /* Make sure we don't pass any secrets to usermode */
            RtlZeroMemory(KernelMem, Size);
        }
    }
    else
    {
        KernelModeMsg->lParam = 0;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS FASTCALL
CopyMsgToUserMem(MSG *UserModeMsg, MSG *KernelModeMsg)
{
    NTSTATUS Status;
    PMSGMEMORY MsgMemoryEntry;
    UINT Size;

    /* See if this message type is present in the table */
    MsgMemoryEntry = FindMsgMemory(UserModeMsg->message);
    if (NULL == MsgMemoryEntry)
    {
        /* Not present, no copying needed */
        return STATUS_SUCCESS;
    }

    /* Determine required size */
    Size = MsgMemorySize(MsgMemoryEntry, UserModeMsg->wParam, UserModeMsg->lParam);

    if (0 != Size)
    {
        /* Copy data if required */
        if (0 != (MsgMemoryEntry->Flags & MMS_FLAG_WRITE))
        {
            Status = MmCopyToCaller((PVOID) UserModeMsg->lParam, (PVOID) KernelModeMsg->lParam, Size);
            if (! NT_SUCCESS(Status))
            {
                DPRINT1("Failed to copy message from kernel: invalid usermode buffer\n");
                ExFreePool((PVOID) KernelModeMsg->lParam);
                return Status;
            }
        }

        ExFreePool((PVOID) KernelModeMsg->lParam);
    }

    return STATUS_SUCCESS;
}

static BOOL FASTCALL
co_IntWaitMessage( PWND Window,
                   UINT MsgFilterMin,
                   UINT MsgFilterMax )
{
    PTHREADINFO pti;
    PUSER_MESSAGE_QUEUE ThreadQueue;
    NTSTATUS Status = STATUS_SUCCESS;
    MSG Msg;

    pti = PsGetCurrentThreadWin32Thread();
    ThreadQueue = pti->MessageQueue;

    do
    {
        if ( co_IntPeekMessage( &Msg,
                                Window,
                                MsgFilterMin,
                                MsgFilterMax,
                                PM_NOREMOVE))
        {
            return TRUE;
        }

        /* Nothing found. Wait for new messages. */
        Status = co_MsqWaitForNewMessages( ThreadQueue,
                                            Window,
                                            MsgFilterMin,
                                            MsgFilterMax);
    }
    while ( (STATUS_WAIT_0 <= Status && Status <= STATUS_WAIT_63) ||
            STATUS_TIMEOUT == Status );

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        DPRINT1("Exit co_IntWaitMessage on error!\n");
    }

    return FALSE;
}

BOOL FASTCALL
co_IntGetPeekMessage( PMSG pMsg,
                      HWND hWnd,
                      UINT MsgFilterMin,
                      UINT MsgFilterMax,
                      UINT RemoveMsg,
                      BOOL bGMSG )
{
    PWND Window;
    BOOL Present = FALSE;

    if ( hWnd == HWND_TOPMOST || hWnd == HWND_BROADCAST )
        hWnd = HWND_BOTTOM;

    /* Validate input */
    if (hWnd && hWnd != HWND_BOTTOM)
    {
        if (!(Window = UserGetWindowObject(hWnd)))
        {
            if (bGMSG)
                return -1;
            else
                return FALSE;
        }
    }
    else
    {
        Window = (PWND)hWnd;
    }

    if (MsgFilterMax < MsgFilterMin)
    {
        MsgFilterMin = 0;
        MsgFilterMax = 0;
    }

    do
    {
        Present = co_IntPeekMessage( pMsg,
                                     Window,
                                     MsgFilterMin,
                                     MsgFilterMax,
                                     RemoveMsg );
        if (Present)
        {
           // The WH_GETMESSAGE hook enables an application to monitor messages about to
           // be returned by the GetMessage or PeekMessage function.

           co_HOOK_CallHooks( WH_GETMESSAGE, HC_ACTION, RemoveMsg & PM_REMOVE, (LPARAM)pMsg);

           if ( bGMSG )
              return (WM_QUIT != pMsg->message);
        }

        if ( bGMSG )
        {
           if ( !co_IntWaitMessage(Window, MsgFilterMin, MsgFilterMax) )
              return -1;
        }
        else
        {
           if (!(RemoveMsg & PM_NOYIELD))
           {
              IdlePing();
              // Yield this thread!
              UserLeave();
              ZwYieldExecution();
              UserEnterExclusive();
              // Fall through to exit.
              IdlePong();
           }
           break;
        }
    }
    while( bGMSG && !Present );

    return Present;
}

BOOL FASTCALL
UserPostThreadMessage( DWORD idThread,
                       UINT Msg,
                       WPARAM wParam,
                       LPARAM lParam )
{
    MSG Message;
    PETHREAD peThread;
    PTHREADINFO pThread;
    LARGE_INTEGER LargeTickCount;
    NTSTATUS Status;

    DPRINT1("UserPostThreadMessage wParam 0x%x  lParam 0x%x\n", wParam,lParam);

    if (FindMsgMemory(Msg) != 0)
    {
        SetLastWin32Error(ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }

    Status = PsLookupThreadByThreadId((HANDLE)idThread,&peThread);

    if( Status == STATUS_SUCCESS )
    {
        pThread = (PTHREADINFO)peThread->Tcb.Win32Thread;
        if( !pThread ||
            !pThread->MessageQueue ||
            (pThread->TIF_flags & TIF_INCLEANUP))
        {
            ObDereferenceObject( peThread );
            return FALSE;
        }

        Message.hwnd = NULL;
        Message.message = Msg;
        Message.wParam = wParam;
        Message.lParam = lParam;
        Message.pt = gpsi->ptCursor;

        KeQueryTickCount(&LargeTickCount);
        pThread->timeLast = Message.time = MsqCalculateMessageTime(&LargeTickCount);
        MsqPostMessage(pThread->MessageQueue, &Message, FALSE, QS_POSTMESSAGE);
        ObDereferenceObject( peThread );
        return TRUE;
    }
    else
    {
        SetLastNtError( Status );
    }
    return FALSE;
}

BOOL FASTCALL
UserPostMessage( HWND Wnd,
                 UINT Msg,
                 WPARAM wParam,
                 LPARAM lParam )
{
    PTHREADINFO pti;
    MSG Message;
    LARGE_INTEGER LargeTickCount;

    if (FindMsgMemory(Msg) != 0)
    {
        SetLastWin32Error(ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }

    if (!Wnd)
    {
        return UserPostThreadMessage( PtrToInt(PsGetCurrentThreadId()),
                                      Msg,
                                      wParam,
                                      lParam);
    }
    if (Wnd == HWND_BROADCAST)
    {
        HWND *List;
        PWND DesktopWindow;
        ULONG i;

        DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
        List = IntWinListChildren(DesktopWindow);

        if (List != NULL)
        {
            UserPostMessage(DesktopWindow->head.h, Msg, wParam, lParam);
            for (i = 0; List[i]; i++)
            {
                UserPostMessage(List[i], Msg, wParam, lParam);
            }
            ExFreePool(List);
        }
    }
    else
    {
        PWND Window;

        Window = UserGetWindowObject(Wnd);
        if ( !Window )
        {
            return FALSE;
        }

        pti = Window->head.pti;
        if ( pti->TIF_flags & TIF_INCLEANUP )
        {
            DPRINT1("Attempted to post message to window 0x%x when the thread is in cleanup!\n", Wnd);
            return FALSE;
        }

        if ( Window->state & WNDS_DESTROYED )
        {
            DPRINT1("Attempted to post message to window 0x%x that is being destroyed!\n", Wnd);
            /* FIXME - last error code? */
            return FALSE;
        }

        if (WM_QUIT == Msg)
        {
            MsqPostQuitMessage(Window->head.pti->MessageQueue, wParam);
        }
        else
        {
            Message.hwnd = Wnd;
            Message.message = Msg;
            Message.wParam = wParam;
            Message.lParam = lParam;
            Message.pt = gpsi->ptCursor;
            KeQueryTickCount(&LargeTickCount);
            pti->timeLast = Message.time = MsqCalculateMessageTime(&LargeTickCount);
            MsqPostMessage(Window->head.pti->MessageQueue, &Message, FALSE, QS_POSTMESSAGE);
        }
    }
    return TRUE;
}


LRESULT FASTCALL
co_IntSendMessage( HWND hWnd,
                   UINT Msg,
                   WPARAM wParam,
                   LPARAM lParam )
{
    ULONG_PTR Result = 0;
    if(co_IntSendMessageTimeout(hWnd, Msg, wParam, lParam, SMTO_NORMAL, 0, &Result))
    {
        return (LRESULT)Result;
    }
    return 0;
}

static LRESULT FASTCALL
co_IntSendMessageTimeoutSingle( HWND hWnd,
                                UINT Msg,
                                WPARAM wParam,
                                LPARAM lParam,
                                UINT uFlags,
                                UINT uTimeout,
                                ULONG_PTR *uResult )
{
    NTSTATUS Status;
    PWND Window = NULL;
    PMSGMEMORY MsgMemoryEntry;
    INT lParamBufferSize;
    LPARAM lParamPacked;
    PTHREADINFO Win32Thread;
    ULONG_PTR Result = 0;
    DECLARE_RETURN(LRESULT);
    USER_REFERENCE_ENTRY Ref;

    if (!(Window = UserGetWindowObject(hWnd)))
    {
        RETURN( FALSE);
    }

    UserRefObjectCo(Window, &Ref);

    Win32Thread = PsGetCurrentThreadWin32Thread();

    IntCallWndProc( Window, hWnd, Msg, wParam, lParam);

    if ( NULL != Win32Thread &&
         Window->head.pti->MessageQueue == Win32Thread->MessageQueue)
    {
        if (Win32Thread->TIF_flags & TIF_INCLEANUP)
        {
            /* Never send messages to exiting threads */
            RETURN( FALSE);
        }

        /* See if this message type is present in the table */
        MsgMemoryEntry = FindMsgMemory(Msg);
        if (NULL == MsgMemoryEntry)
        {
            lParamBufferSize = -1;
        }
        else
        {
            lParamBufferSize = MsgMemorySize(MsgMemoryEntry, wParam, lParam);
        }

        if (! NT_SUCCESS(PackParam(&lParamPacked, Msg, wParam, lParam, FALSE)))
        {
            DPRINT1("Failed to pack message parameters\n");
            RETURN( FALSE);
        }

        ObReferenceObject(Win32Thread->pEThread);
        Result = (ULONG_PTR)co_IntCallWindowProc( Window->lpfnWndProc,
                                                  !Window->Unicode,
                                                  hWnd,
                                                  Msg,
                                                  wParam,
                                                  lParamPacked,
                                                  lParamBufferSize );
        if(uResult)
        {
            *uResult = Result;
        }

        ObDereferenceObject(Win32Thread->pEThread);

        IntCallWndProcRet( Window, hWnd, Msg, wParam, lParam, (LRESULT *)uResult);

        if (! NT_SUCCESS(UnpackParam(lParamPacked, Msg, wParam, lParam, FALSE)))
        {
            DPRINT1("Failed to unpack message parameters\n");
            RETURN( TRUE);
        }

        RETURN( TRUE);
    }

    if (uFlags & SMTO_ABORTIFHUNG && MsqIsHung(Window->head.pti->MessageQueue))
    {
        /* FIXME - Set a LastError? */
        RETURN( FALSE);
    }

    if (Window->state & WNDS_DESTROYED)
    {
        /* FIXME - last error? */
        DPRINT1("Attempted to send message to window 0x%x that is being destroyed!\n", hWnd);
        RETURN( FALSE);
    }

    do
    {
        Status = co_MsqSendMessage( Window->head.pti->MessageQueue,
                                    hWnd,
                                    Msg,
                                    wParam,
                                    lParam,
                                    uTimeout,
                                    (uFlags & SMTO_BLOCK),
                                    MSQ_NORMAL,
                                    uResult );
    }
    while ((STATUS_TIMEOUT == Status) &&
           (uFlags & SMTO_NOTIMEOUTIFNOTHUNG) &&
           !MsqIsHung(Window->head.pti->MessageQueue));

    IntCallWndProcRet( Window, hWnd, Msg, wParam, lParam, (LRESULT *)uResult);

    if (STATUS_TIMEOUT == Status)
    {
        /*
MSDN says:
    Microsoft Windows 2000: If GetLastError returns zero, then the function
    timed out.
    XP+ : If the function fails or times out, the return value is zero.
    To get extended error information, call GetLastError. If GetLastError
    returns ERROR_TIMEOUT, then the function timed out.
*/
        SetLastWin32Error(ERROR_TIMEOUT);
        RETURN( FALSE);
    }
    else if (! NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        RETURN( FALSE);
    }

    RETURN( TRUE);

CLEANUP:
    if (Window) UserDerefObjectCo(Window);
    END_CLEANUP;
}

LRESULT FASTCALL
co_IntSendMessageTimeout( HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam,
                          UINT uFlags,
                          UINT uTimeout,
                          ULONG_PTR *uResult )
{
    PWND DesktopWindow;
    HWND *Children;
    HWND *Child;

    if (HWND_BROADCAST != hWnd)
    {
        return co_IntSendMessageTimeoutSingle(hWnd, Msg, wParam, lParam, uFlags, uTimeout, uResult);
    }

    DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
    if (NULL == DesktopWindow)
    {
        SetLastWin32Error(ERROR_INTERNAL_ERROR);
        return 0;
    }

    /* Send message to the desktop window too! */
    co_IntSendMessageTimeoutSingle(DesktopWindow->head.h, Msg, wParam, lParam, uFlags, uTimeout, uResult);

    Children = IntWinListChildren(DesktopWindow);
    if (NULL == Children)
    {
        return 0;
    }

    for (Child = Children; NULL != *Child; Child++)
    {
        co_IntSendMessageTimeoutSingle(*Child, Msg, wParam, lParam, uFlags, uTimeout, uResult);
    }

    ExFreePool(Children);

    return (LRESULT) TRUE;
}

LRESULT FASTCALL 
co_IntSendMessageNoWait(HWND hWnd,
                        UINT Msg,
                        WPARAM wParam,
                        LPARAM lParam)
{
    ULONG_PTR Result = 0;
    co_IntSendMessageWithCallBack(hWnd,
                                  Msg,
                                  wParam,
                                  lParam,
                                  NULL,
                                  0,
                                  &Result);
    return Result;
}

LRESULT FASTCALL
co_IntSendMessageWithCallBack( HWND hWnd,
                              UINT Msg,
                              WPARAM wParam,
                              LPARAM lParam,
                              SENDASYNCPROC CompletionCallback,
                              ULONG_PTR CompletionCallbackContext,
                              ULONG_PTR *uResult)
{
    ULONG_PTR Result;
    PWND Window = NULL;
    PMSGMEMORY MsgMemoryEntry;
    INT lParamBufferSize;
    LPARAM lParamPacked;
    PTHREADINFO Win32Thread;
    DECLARE_RETURN(LRESULT);
    USER_REFERENCE_ENTRY Ref;
    PUSER_SENT_MESSAGE Message;

    if (!(Window = UserGetWindowObject(hWnd)))
    {
        RETURN(FALSE);
    }

    UserRefObjectCo(Window, &Ref);

    if (Window->state & WNDS_DESTROYED)
    {
        /* FIXME - last error? */
        DPRINT1("Attempted to send message to window 0x%x that is being destroyed!\n", hWnd);
        RETURN(FALSE);
    }

    Win32Thread = PsGetCurrentThreadWin32Thread();

    IntCallWndProc( Window, hWnd, Msg, wParam, lParam);

    if (Win32Thread == NULL)
    {
        ASSERT(FALSE);
        RETURN(FALSE);
    }

    if (Win32Thread->TIF_flags & TIF_INCLEANUP)
    {
        /* Never send messages to exiting threads */
        RETURN(FALSE);
    }

    /* See if this message type is present in the table */
    MsgMemoryEntry = FindMsgMemory(Msg);
    if (NULL == MsgMemoryEntry)
    {
        lParamBufferSize = -1;
    }
    else
    {
        lParamBufferSize = MsgMemorySize(MsgMemoryEntry, wParam, lParam);
    }

    if (! NT_SUCCESS(PackParam(&lParamPacked, Msg, wParam, lParam, Window->head.pti->MessageQueue != Win32Thread->MessageQueue)))
    {
        DPRINT1("Failed to pack message parameters\n");
        RETURN( FALSE);
    }

    /* If this is not a callback and it can be sent now, then send it. */
    if ((Window->head.pti->MessageQueue == Win32Thread->MessageQueue) && (CompletionCallback == NULL))
    {
        ObReferenceObject(Win32Thread->pEThread);
        Result = (ULONG_PTR)co_IntCallWindowProc( Window->lpfnWndProc,
                                                  !Window->Unicode,
                                                  hWnd,
                                                  Msg,
                                                  wParam,
                                                  lParamPacked,
                                                  lParamBufferSize );
        if(uResult)
        {
            *uResult = Result;
        }
        ObDereferenceObject(Win32Thread->pEThread);
    }

    IntCallWndProcRet( Window, hWnd, Msg, wParam, lParam, (LRESULT *)uResult);

    if ((Window->head.pti->MessageQueue == Win32Thread->MessageQueue) && (CompletionCallback == NULL))
    {
        if (! NT_SUCCESS(UnpackParam(lParamPacked, Msg, wParam, lParam, FALSE)))
        {
            DPRINT1("Failed to unpack message parameters\n");
        }
        RETURN(TRUE);
    }

    if(!(Message = ExAllocatePoolWithTag(NonPagedPool, sizeof(USER_SENT_MESSAGE), TAG_USRMSG)))
    {
        DPRINT1("MsqSendMessage(): Not enough memory to allocate a message");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Message->Msg.hwnd = hWnd;
    Message->Msg.message = Msg;
    Message->Msg.wParam = wParam;
    Message->Msg.lParam = lParamPacked;
    Message->CompletionEvent = NULL;
    Message->Result = 0;
    Message->SenderQueue = NULL; //Win32Thread->MessageQueue;

    IntReferenceMessageQueue(Window->head.pti->MessageQueue);
    Message->CompletionCallback = CompletionCallback;
    Message->CompletionCallbackContext = CompletionCallbackContext;
    Message->HookMessage = MSQ_NORMAL | MSQ_SENTNOWAIT;
    Message->HasPackedLParam = (lParamBufferSize > 0);

    InsertTailList(&Window->head.pti->MessageQueue->SentMessagesListHead, &Message->ListEntry);
    IntDereferenceMessageQueue(Window->head.pti->MessageQueue);

    RETURN(TRUE);

CLEANUP:
    if (Window) UserDerefObjectCo(Window);
    END_CLEANUP;
}

/* This function posts a message if the destination's message queue belongs to
another thread, otherwise it sends the message. It does not support broadcast
messages! */
LRESULT FASTCALL
co_IntPostOrSendMessage( HWND hWnd,
                         UINT Msg,
                         WPARAM wParam,
                         LPARAM lParam )
{
    ULONG_PTR Result;
    PTHREADINFO pti;
    PWND Window;

    if ( hWnd == HWND_BROADCAST )
    {
        return 0;
    }

    if(!(Window = UserGetWindowObject(hWnd)))
    {
        return 0;
    }

    pti = PsGetCurrentThreadWin32Thread();

    if ( Window->head.pti->MessageQueue != pti->MessageQueue &&
         FindMsgMemory(Msg) == 0 )
    {
        Result = UserPostMessage(hWnd, Msg, wParam, lParam);
    }
    else
    {
        if ( !co_IntSendMessageTimeoutSingle(hWnd, Msg, wParam, lParam, SMTO_NORMAL, 0, &Result) )
        {
            Result = 0;
        }
    }

    return (LRESULT)Result;
}

LRESULT FASTCALL
co_IntDoSendMessage( HWND hWnd,
                     UINT Msg,
                     WPARAM wParam,
                     LPARAM lParam,
                     PDOSENDMESSAGE dsm,
                     PNTUSERSENDMESSAGEINFO UnsafeInfo )
{
    PTHREADINFO pti;
    LRESULT Result = TRUE;
    NTSTATUS Status;
    PWND Window = NULL;
    NTUSERSENDMESSAGEINFO Info;
    MSG UserModeMsg;
    MSG KernelModeMsg;
    PMSGMEMORY MsgMemoryEntry;

    RtlZeroMemory(&Info, sizeof(NTUSERSENDMESSAGEINFO));

    /* FIXME: Call hooks. */
    if (HWND_BROADCAST != hWnd)
    {
        Window = UserGetWindowObject(hWnd);
        if ( !Window )
        {
            /* Tell usermode to not touch this one */
            Info.HandledByKernel = TRUE;
            MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERSENDMESSAGEINFO));
            return 0;
        }
    }

    /* Check for an exiting window. */
    if (Window && Window->state & WNDS_DESTROYED)
    {
        DPRINT1("co_IntDoSendMessage Window Exiting!\n");
    }

    /* See if the current thread can handle the message */
    pti = PsGetCurrentThreadWin32Thread();

    // This is checked in user mode!!!!!!!
    if ( HWND_BROADCAST != hWnd &&
         NULL != pti &&
         Window->head.pti->MessageQueue == pti->MessageQueue &&
         !ISITHOOKED(WH_CALLWNDPROC) &&
         !ISITHOOKED(WH_CALLWNDPROCRET) &&
         ( Msg < WM_DDE_FIRST || Msg > WM_DDE_LAST ) )
    {
        /* Gather the information usermode needs to call the window proc directly */
        Info.HandledByKernel = FALSE;

        Status = MmCopyFromCaller(&(Info.Ansi), &(UnsafeInfo->Ansi), sizeof(BOOL));
        if (! NT_SUCCESS(Status))
        {
            Info.Ansi = ! Window->Unicode;
        }

        Info.Ansi = !Window->Unicode;
        Info.Proc = Window->lpfnWndProc;
    }
    else
    {
        /* Must be handled by other thread */
        //      if (HWND_BROADCAST != hWnd)
        //      {
        //         UserDereferenceObject(Window);
        //      }
        Info.HandledByKernel = TRUE;
        UserModeMsg.hwnd = hWnd;
        UserModeMsg.message = Msg;
        UserModeMsg.wParam = wParam;
        UserModeMsg.lParam = lParam;
        MsgMemoryEntry = FindMsgMemory(UserModeMsg.message);

        Status = CopyMsgToKernelMem(&KernelModeMsg, &UserModeMsg, MsgMemoryEntry);
        if (! NT_SUCCESS(Status))
        {
            MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERSENDMESSAGEINFO));
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return (dsm ? 0 : -1);
        }

        if(!dsm)
        {
            Result = co_IntSendMessage( KernelModeMsg.hwnd,
                                        KernelModeMsg.message,
                                        KernelModeMsg.wParam,
                                        KernelModeMsg.lParam );
        }
        else
        {
            Result = co_IntSendMessageTimeout( KernelModeMsg.hwnd,
                                               KernelModeMsg.message,
                                               KernelModeMsg.wParam,
                                               KernelModeMsg.lParam,
                                               dsm->uFlags,
                                               dsm->uTimeout,
                                               &dsm->Result );
        }

        Status = CopyMsgToUserMem(&UserModeMsg, &KernelModeMsg);
        if (! NT_SUCCESS(Status))
        {
            MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERSENDMESSAGEINFO));
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return(dsm ? 0 : -1);
        }
    }

    Status = MmCopyToCaller(UnsafeInfo, &Info, sizeof(NTUSERSENDMESSAGEINFO));
    if (! NT_SUCCESS(Status))
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
    }

    return (LRESULT)Result;
}


BOOL FASTCALL
UserSendNotifyMessage( HWND hWnd,
                       UINT Msg,
                       WPARAM wParam,
                       LPARAM lParam )
{
    BOOL Result = TRUE;

    if (FindMsgMemory(Msg) != 0)
    {
        SetLastWin32Error(ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }

    // Basicly the same as IntPostOrSendMessage
    if (hWnd == HWND_BROADCAST) //Handle Broadcast
    {
        HWND *List;
        PWND DesktopWindow;
        ULONG i;

        DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
        List = IntWinListChildren(DesktopWindow);

        if (List != NULL)
        {
            UserSendNotifyMessage(DesktopWindow->head.h, Msg, wParam, lParam);
            for (i = 0; List[i]; i++)
            {
                UserSendNotifyMessage(List[i], Msg, wParam, lParam);
            }
            ExFreePool(List);
        }
    }
    else
    {
        ULONG_PTR PResult;
        PTHREADINFO pti;
        PWND Window;

        if ( !(Window = UserGetWindowObject(hWnd)) ) return FALSE;

        pti = PsGetCurrentThreadWin32Thread();

        if (Window->head.pti->MessageQueue != pti->MessageQueue)
        { // Send message w/o waiting for it.
            Result = UserPostMessage(hWnd, Msg, wParam, lParam);
        }
        else
        { // Handle message and callback.
            Result = co_IntSendMessageTimeoutSingle( hWnd,
                                                     Msg,
                                                     wParam,
                                                     lParam,
                                                     SMTO_NORMAL,
                                                     0,
                                                     &PResult );
        }
    }
    return Result;
}


DWORD APIENTRY
IntGetQueueStatus(BOOL ClearChanges)
{
    PTHREADINFO pti;
    PUSER_MESSAGE_QUEUE Queue;
    DWORD Result;

    pti = PsGetCurrentThreadWin32Thread();
    Queue = pti->MessageQueue;

    Result = MAKELONG(Queue->QueueBits, Queue->ChangedBits);
    if (ClearChanges)
    {
        Queue->ChangedBits = 0;
    }

    return Result;
}

BOOL APIENTRY
IntInitMessagePumpHook()
{
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    if (pti->pcti)
    {
        pti->pcti->dwcPumpHook++;
        return TRUE;
    }
    return FALSE;
}

BOOL APIENTRY
IntUninitMessagePumpHook()
{
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    if (pti->pcti)
    {
        if (pti->pcti->dwcPumpHook <= 0)
        {
            return FALSE;
        }
        pti->pcti->dwcPumpHook--;
        return TRUE;
    }
    return FALSE;
}

/** Functions ******************************************************************/

BOOL APIENTRY
NtUserPostMessage(HWND hWnd,
                  UINT Msg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    BOOL ret;

    UserEnterExclusive();

    ret = UserPostMessage(hWnd, Msg, wParam, lParam);

    UserLeave();

    return ret;
}

BOOL APIENTRY
NtUserPostThreadMessage(DWORD idThread,
                        UINT Msg,
                        WPARAM wParam,
                        LPARAM lParam)
{
    BOOL ret;

    UserEnterExclusive();

    ret = UserPostThreadMessage( idThread, Msg, wParam, lParam);

    UserLeave();
    
    return ret;
}

DWORD APIENTRY
NtUserQuerySendMessage(DWORD Unknown0)
{
    UNIMPLEMENTED;

    return 0;
}


////////// API on the way out!
LRESULT APIENTRY
NtUserSendMessageTimeout( HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam,
                          UINT uFlags,
                          UINT uTimeout,
                          ULONG_PTR *uResult,
                          PNTUSERSENDMESSAGEINFO UnsafeInfo )
{
    DOSENDMESSAGE dsm;
    LRESULT Result;

    DPRINT("Enter NtUserSendMessageTimeout\n");

    dsm.uFlags = uFlags;
    dsm.uTimeout = uTimeout;

    UserEnterExclusive();

    Result = co_IntDoSendMessage(hWnd, Msg, wParam, lParam, &dsm, UnsafeInfo);

    UserLeave();

    if(uResult != NULL && Result != 0)
    {
        _SEH2_TRY
        {
            ProbeForWrite(uResult, sizeof(ULONG_PTR), 1);
            RtlCopyMemory(uResult, &dsm.Result, sizeof(ULONG_PTR));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);;
            Result = FALSE;
        }
        _SEH2_END;
    }

    return Result;
}

LRESULT APIENTRY
NtUserSendMessage( HWND Wnd,
                   UINT Msg,
                   WPARAM wParam,
                   LPARAM lParam,
                   PNTUSERSENDMESSAGEINFO UnsafeInfo )
{
    BOOL ret;

    UserEnterExclusive();

    ret = co_IntDoSendMessage(Wnd, Msg, wParam, lParam, NULL, UnsafeInfo);

    UserLeave();

    return ret;
}
//////////

BOOL APIENTRY
NtUserWaitMessage(VOID)
{
    BOOL ret;

    UserEnterExclusive();

    ret = co_IntWaitMessage(NULL, 0, 0);

    UserLeave();
    
    return ret;
}


BOOL APIENTRY
NtUserGetMessage( PNTUSERGETMESSAGEINFO UnsafeInfo,
                  HWND hWnd,
                  UINT MsgFilterMin,
                  UINT MsgFilterMax )
/*
* FUNCTION: Get a message from the calling thread's message queue.
* ARGUMENTS:
*      UnsafeMsg - Pointer to the structure which receives the returned message.
*      Wnd - Window whose messages are to be retrieved.
*      MsgFilterMin - Integer value of the lowest message value to be
*                     retrieved.
*      MsgFilterMax - Integer value of the highest message value to be
*                     retrieved.
*/
{
    NTUSERGETMESSAGEINFO Info;
    NTSTATUS Status;
    /* FIXME: if initialization is removed, gcc complains that this may be used before initialization. Please review */
    PMSGMEMORY MsgMemoryEntry;
    PVOID UserMem;
    ULONG Size;
    MSG Msg;
    BOOL GotMessage;

    if ( (MsgFilterMin|MsgFilterMax) & ~WM_MAXIMUM )
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    UserEnterExclusive();

    RtlZeroMemory(&Msg, sizeof(MSG));

    GotMessage = co_IntGetPeekMessage(&Msg, hWnd, MsgFilterMin, MsgFilterMax, PM_REMOVE, TRUE);

    UserLeave();

    Info.Msg = Msg; //.Msg;
    /* See if this message type is present in the table */
    MsgMemoryEntry = FindMsgMemory(Info.Msg.message);

    _SEH2_TRY
    {
        ProbeForWrite(UnsafeInfo, sizeof(NTUSERGETMESSAGEINFO), 1);
        RtlCopyMemory(UnsafeInfo, &Info, sizeof(NTUSERGETMESSAGEINFO));

        if (NULL == MsgMemoryEntry)
        {
            /* Not present, no copying needed */
            UnsafeInfo->LParamSize = 0;
        }
        else
        {
            /* Determine required size */
            Size = MsgMemorySize(MsgMemoryEntry, Info.Msg.wParam, Info.Msg.lParam);

            /* Allocate required amount of user-mode memory */
            Status = ZwAllocateVirtualMemory(NtCurrentProcess(), 
                                             &UserMem, 
                                             0,
                                             &Size, 
                                             MEM_COMMIT, 
                                             PAGE_READWRITE);
            if (! NT_SUCCESS(Status))
            {
                SetLastNtError(Status);
                _SEH2_YIELD(return (BOOL) -1);
            }

            /* Transfer lParam data to user-mode mem */
            ProbeForWrite(UserMem, Size, 1);
            RtlCopyMemory(UserMem, (PVOID)Info.Msg.lParam, Size);

            UnsafeInfo->LParamSize = Size;
            UnsafeInfo->Msg.lParam = (LPARAM) UserMem;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());

        if(UserMem != NULL)
        {
            ZwFreeVirtualMemory(NtCurrentProcess(), &UserMem, &Size, MEM_RELEASE);
        }

        _SEH2_YIELD(return (BOOL) -1);
    }
    _SEH2_END;
    
    return GotMessage;
}


BOOL APIENTRY
NtUserGetMessageX(PMSG pMsg,
                  HWND hWnd,
                  UINT MsgFilterMin,
                  UINT MsgFilterMax)
{
    MSG Msg;
    BOOL Ret;

    if ( (MsgFilterMin|MsgFilterMax) & ~WM_MAXIMUM )
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    UserEnterExclusive();

    RtlZeroMemory(&Msg, sizeof(MSG));

    Ret = co_IntGetPeekMessage(&Msg, hWnd, MsgFilterMin, MsgFilterMax, PM_REMOVE, TRUE);

    UserLeave();

    if (Ret)
    {
        _SEH2_TRY
        {
            ProbeForWrite(pMsg, sizeof(MSG), 1);
            RtlCopyMemory(pMsg, &Msg, sizeof(MSG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
            Ret = FALSE;
        }
        _SEH2_END;
    }

    return Ret;
}

BOOL APIENTRY
NtUserPeekMessage(PNTUSERGETMESSAGEINFO UnsafeInfo,
                  HWND hWnd,
                  UINT MsgFilterMin,
                  UINT MsgFilterMax,
                  UINT RemoveMsg)
{
    NTSTATUS Status;
    NTUSERGETMESSAGEINFO Info;
    PMSGMEMORY MsgMemoryEntry;
    PVOID UserMem = NULL;
    ULONG Size;
    MSG Msg;
    BOOL Ret;

    if ( RemoveMsg & PM_BADMSGFLAGS )
    {
        SetLastWin32Error(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    UserEnterExclusive();

    RtlZeroMemory(&Msg, sizeof(MSG));

    Ret = co_IntGetPeekMessage(&Msg, hWnd, MsgFilterMin, MsgFilterMax, RemoveMsg, FALSE);

    UserLeave();

    if (Ret)
    {
        Info.Msg = Msg;
        /* See if this message type is present in the table */
        MsgMemoryEntry = FindMsgMemory(Info.Msg.message);

        _SEH2_TRY
        {
            ProbeForWrite(UnsafeInfo, sizeof(NTUSERGETMESSAGEINFO), 1);
            RtlCopyMemory(UnsafeInfo, &Info, sizeof(NTUSERGETMESSAGEINFO));

            if (NULL == MsgMemoryEntry)
            {
                /* Not present, no copying needed */
                UnsafeInfo->LParamSize = 0;
            }
            else
            {
                /* Determine required size */
                Size = MsgMemorySize(MsgMemoryEntry, Info.Msg.wParam, Info.Msg.lParam);

                /* Allocate required amount of user-mode memory */
                Status = ZwAllocateVirtualMemory(NtCurrentProcess(), 
                                                 &UserMem, 
                                                 0,
                                                 &Size, 
                                                 MEM_COMMIT, 
                                                 PAGE_READWRITE);
                if (! NT_SUCCESS(Status))
                {
                    SetLastNtError(Status);
                    _SEH2_YIELD(return (BOOL) -1);
                }

                /* Transfer lParam data to user-mode mem */
                ProbeForWrite(UserMem, Size, 1);
                RtlCopyMemory(UserMem, (PVOID)Info.Msg.lParam, Size);

                UnsafeInfo->LParamSize = Size;
                UnsafeInfo->Msg.lParam = (LPARAM) UserMem;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
            Ret = (BOOL) -1;

            if(UserMem != NULL)
            {
                ZwFreeVirtualMemory(NtCurrentProcess(), &UserMem, &Size, MEM_RELEASE);
            }
        }
        _SEH2_END;
    }

    return Ret;
}

BOOL APIENTRY
NtUserPeekMessageX( PMSG pMsg,
                    HWND hWnd,
                    UINT MsgFilterMin,
                    UINT MsgFilterMax,
                    UINT RemoveMsg)
{
    MSG Msg;
    BOOL Ret;

    if ( RemoveMsg & PM_BADMSGFLAGS )
    {
        SetLastWin32Error(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    UserEnterExclusive();

    RtlZeroMemory(&Msg, sizeof(MSG));

    Ret = co_IntGetPeekMessage(&Msg, hWnd, MsgFilterMin, MsgFilterMax, RemoveMsg, FALSE);

    UserLeave();

    if (Ret)
    {
        _SEH2_TRY
        {
            ProbeForWrite(pMsg, sizeof(MSG), 1);
            RtlCopyMemory(pMsg, &Msg, sizeof(MSG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
            Ret = FALSE;
        }
        _SEH2_END;
    }
    
    return Ret;
}

BOOL APIENTRY
NtUserCallMsgFilter( LPMSG lpmsg, INT code)
{
    BOOL Ret = FALSE;
    MSG Msg;

    _SEH2_TRY
    {
        ProbeForRead(lpmsg, sizeof(MSG), 1);
        RtlCopyMemory( &Msg, lpmsg, sizeof(MSG));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    UserEnterExclusive();

    if ( co_HOOK_CallHooks( WH_SYSMSGFILTER, code, 0, (LPARAM)&Msg))
    {
        Ret = TRUE;
    }
    else
    {
        Ret = co_HOOK_CallHooks( WH_MSGFILTER, code, 0, (LPARAM)&Msg);
    }

    UserLeave();

    _SEH2_TRY
    {
        ProbeForWrite(lpmsg, sizeof(MSG), 1);
        RtlCopyMemory(lpmsg, &Msg, sizeof(MSG));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = FALSE;
    }
    _SEH2_END;

    return Ret;
}

LRESULT APIENTRY
NtUserDispatchMessage(PMSG UnsafeMsgInfo)
{
    LRESULT Res = 0;
    MSG SafeMsg;

    _SEH2_TRY
    {
        ProbeForRead(UnsafeMsgInfo, sizeof(MSG), 1);
        RtlCopyMemory(&SafeMsg, UnsafeMsgInfo, sizeof(MSG));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    UserEnterExclusive();

    Res = IntDispatchMessage(&SafeMsg);

    UserLeave();
    return Res;
}


BOOL APIENTRY
NtUserTranslateMessage(LPMSG lpMsg, UINT flags)
{
    MSG SafeMsg;
    BOOL Ret;

    _SEH2_TRY
    {
        ProbeForRead(lpMsg, sizeof(MSG), 1);
        RtlCopyMemory(&SafeMsg, lpMsg, sizeof(MSG));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    UserEnterExclusive();

    Ret = IntTranslateKbdMessage(&SafeMsg, flags);

    UserLeave();

    return Ret;
}

BOOL APIENTRY
NtUserMessageCall( HWND hWnd,
                   UINT Msg, 
                   WPARAM wParam,
                   LPARAM lParam,
                   ULONG_PTR ResultInfo,
                   DWORD dwType, // fnID?
                   BOOL Ansi)
{
    LRESULT lResult = 0;
    BOOL Ret = FALSE;
    PWND Window = NULL;
    USER_REFERENCE_ENTRY Ref;

    UserEnterExclusive();

    /* Validate input */
    if (hWnd && (hWnd != INVALID_HANDLE_VALUE))
    {
        Window = UserGetWindowObject(hWnd);
        if (!Window)
        {
            UserLeave();
            return FALSE;
        }
    }

    switch(dwType)
    {
    case FNID_DEFWINDOWPROC:
        if (Window) UserRefObjectCo(Window, &Ref);
        lResult = IntDefWindowProc(Window, Msg, wParam, lParam, Ansi);
        Ret = TRUE;
        if (Window) UserDerefObjectCo(Window);
        break;
    case FNID_SENDNOTIFYMESSAGE:
        Ret = UserSendNotifyMessage(hWnd, Msg, wParam, lParam);
        break;
    case FNID_BROADCASTSYSTEMMESSAGE:
        {
            BROADCASTPARM parm;
            DWORD_PTR RetVal = 0;

            if (ResultInfo)
            {
                _SEH2_TRY
                {
                    ProbeForWrite((PVOID)ResultInfo, sizeof(BROADCASTPARM), 1);
                    RtlCopyMemory(&parm, (PVOID)ResultInfo, sizeof(BROADCASTPARM));
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Ret = FALSE;
                    _SEH2_YIELD(break);
                }
                _SEH2_END;
            }
            else
                break;

            if ( parm.recipients & BSM_ALLDESKTOPS ||
                    parm.recipients == BSM_ALLCOMPONENTS )
            {
            }
            else if (parm.recipients & BSM_APPLICATIONS)
            {
                if (parm.flags & BSF_QUERY)
                {
                    if (parm.flags & BSF_FORCEIFHUNG || parm.flags & BSF_NOHANG)
                    {
                        co_IntSendMessageTimeout( HWND_BROADCAST,
						                          Msg,
						                          wParam,
						                          lParam,
						                          SMTO_ABORTIFHUNG,
						                          2000,
						                          &RetVal);
                    }
                    else if (parm.flags & BSF_NOTIMEOUTIFNOTHUNG)
                    {
                        co_IntSendMessageTimeout( HWND_BROADCAST,
						                          Msg,
                                                  wParam,
						                          lParam,
						                          SMTO_NOTIMEOUTIFNOTHUNG,
						                          2000,
						                          &RetVal);
                    }
                    else
                    {
                        co_IntSendMessageTimeout( HWND_BROADCAST,
						                          Msg,
						                          wParam,
						                          lParam,
						                          SMTO_NORMAL,
						                          2000,
						                          &RetVal);
                    }
                    Ret = RetVal;
                }
                else if (parm.flags & BSF_POSTMESSAGE)
                {
                    Ret = UserPostMessage(HWND_BROADCAST, Msg, wParam, lParam);
                }
                else //Everything else,,,, if ( parm.flags & BSF_SENDNOTIFYMESSAGE)
                {
                    Ret = UserSendNotifyMessage(HWND_BROADCAST, Msg, wParam, lParam);
                }
            }
        }
        break;
    case FNID_SENDMESSAGECALLBACK:
        {
            PCALL_BACK_INFO CallBackInfo = (PCALL_BACK_INFO)ResultInfo;
            ULONG_PTR uResult;

            if (!CallBackInfo)
                break;

            if (!co_IntSendMessageWithCallBack(hWnd, Msg, wParam, lParam,
                        CallBackInfo->CallBack, CallBackInfo->Context, &uResult))
            {
                DPRINT1("Callback failure!\n");
            }
        }
        break;
        // CallNextHook bypass.
    case FNID_CALLWNDPROC:
    case FNID_CALLWNDPROCRET:
        {
            PTHREADINFO pti;
            PCLIENTINFO ClientInfo;
            PHOOK NextObj, Hook;

            pti = GetW32ThreadInfo();

            Hook = pti->sphkCurrent;

            if (!Hook) break;

            NextObj = Hook->phkNext;
            ClientInfo = pti->pClientInfo;
            _SEH2_TRY
            {
                ClientInfo->phkCurrent = NextObj;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                ClientInfo = NULL;
            }
            _SEH2_END;

            if (!ClientInfo || !NextObj) break;

            NextObj->phkNext = IntGetNextHook(NextObj);

            if ( Hook->HookId == WH_CALLWNDPROC)
            {
                CWPSTRUCT CWP;
                CWP.hwnd    = hWnd;
                CWP.message = Msg;
                CWP.wParam  = wParam;
                CWP.lParam  = lParam;
                DPRINT("WH_CALLWNDPROC: Hook %x NextHook %x\n", Hook, NextObj );

                lResult = co_IntCallHookProc( Hook->HookId,
                                              HC_ACTION,
                                              ((ClientInfo->CI_flags & CI_CURTHPRHOOK) ? 1 : 0),
                                              (LPARAM)&CWP,
                                              Hook->Proc,
                                              Hook->Ansi,
                                              &Hook->ModuleName);
            }
            else
            {
                CWPRETSTRUCT CWPR;
                CWPR.hwnd    = hWnd;
                CWPR.message = Msg;
                CWPR.wParam  = wParam;
                CWPR.lParam  = lParam;
                CWPR.lResult = ClientInfo->dwHookData;

                lResult = co_IntCallHookProc( Hook->HookId,
                                              HC_ACTION,
                                              ((ClientInfo->CI_flags & CI_CURTHPRHOOK) ? 1 : 0),
                                              (LPARAM)&CWPR,
                                              Hook->Proc,
                                              Hook->Ansi,
                                              &Hook->ModuleName);
            }
        }
        break;
    }

    switch(dwType)
    {
    case FNID_DEFWINDOWPROC:
    case FNID_CALLWNDPROC:
    case FNID_CALLWNDPROCRET:
        if (ResultInfo)
        {
            _SEH2_TRY
            {
                ProbeForWrite((PVOID)ResultInfo, sizeof(LRESULT), 1);
                RtlCopyMemory((PVOID)ResultInfo, &lResult, sizeof(LRESULT));
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Ret = FALSE;
            }
            _SEH2_END;
        }
        break;
    default:
        break;
    }

    UserLeave();

    return Ret;
}

#define INFINITE 0xFFFFFFFF
#define WAIT_FAILED ((DWORD)0xFFFFFFFF)

DWORD
APIENTRY
NtUserWaitForInputIdle( IN HANDLE hProcess,
                        IN DWORD dwMilliseconds,
                        IN BOOL Unknown2)
{
    PEPROCESS Process;
    PPROCESSINFO W32Process;
    PTHREADINFO pti;
    NTSTATUS Status;
    HANDLE Handles[3];
    LARGE_INTEGER Timeout;

    UserEnterExclusive();

    Status = ObReferenceObjectByHandle(hProcess,
                                       PROCESS_QUERY_INFORMATION,
                                       PsProcessType,
                                       UserMode,
                                       (PVOID*)&Process,
                                       NULL);

    if (!NT_SUCCESS(Status))
    {
        UserLeave();
        SetLastNtError(Status);
        return WAIT_FAILED;
    }

    pti = PsGetCurrentThreadWin32Thread();

    W32Process = (PPROCESSINFO)Process->Win32Process;

    if ( PsGetProcessExitProcessCalled(Process) ||
         !W32Process ||
         pti->ppi == W32Process)
    {
        ObDereferenceObject(Process);
        UserLeave();
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return WAIT_FAILED;
    }

    Handles[0] = Process;
    Handles[1] = W32Process->InputIdleEvent;
    Handles[2] = pti->MessageQueue->NewMessages; // pEventQueueServer; IntMsqSetWakeMask returns hEventQueueClient

    if (!Handles[1])
    {
        ObDereferenceObject(Process);
        UserLeave();
        return STATUS_SUCCESS;  /* no event to wait on */
    }

    if (dwMilliseconds != INFINITE)
       Timeout.QuadPart = (LONGLONG) dwMilliseconds * (LONGLONG) -10000;

    DPRINT("WFII: ppi 0x%x\n",W32Process);
    DPRINT("WFII: waiting for %p\n", Handles[1] );
    do
    {
        UserLeave();
        Status = KeWaitForMultipleObjects( 3,
                                           Handles,
                                           WaitAny,
                                           UserRequest,
                                           UserMode,
                                           FALSE,
                                           dwMilliseconds == INFINITE ? NULL : &Timeout,
                                           NULL);
        UserEnterExclusive();

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            Status = WAIT_FAILED;
            goto WaitExit;
        }

        switch (Status)
        {
        case STATUS_WAIT_0:
            goto WaitExit;

        case STATUS_WAIT_2:
            {
               MSG Msg;
               co_IntPeekMessage( &Msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE );
               DPRINT1("WFII: WAIT 2\n");
            }
            break;

        case STATUS_TIMEOUT:
            DPRINT1("WFII: timeout\n");
        case WAIT_FAILED:
            goto WaitExit;

        default:
            DPRINT1("WFII: finished\n");
            Status = STATUS_SUCCESS;
            goto WaitExit;
        }
    }
    while (TRUE);

WaitExit:
    ObDereferenceObject(Process);
    UserLeave();
    return Status;
}

/* EOF */
