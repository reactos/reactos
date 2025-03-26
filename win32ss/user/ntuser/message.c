/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Messages
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include <win32k.h>

#include <dde.h>

DBG_DEFAULT_CHANNEL(UserMsg);

#define PM_BADMSGFLAGS ~((QS_RAWINPUT << 16)|PM_QS_SENDMESSAGE|PM_QS_PAINT|PM_QS_POSTMESSAGE|PM_QS_INPUT|PM_NOYIELD|PM_REMOVE)

/* Strings that are OK to pass between user and kernel mode.
 * There may be other strings needed that can easily be added here. */
WCHAR StrUserKernel[3][20] = {{L"intl"}, {L"Environment"}, {L"Policy"}};

/* FUNCTIONS *****************************************************************/

/* PosInArray checks for strings that can pass between user and kernel mode.
 * See: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-settingchange
 * It mentions 'Environment', 'intl', and 'Policy'.
 * These strings are enumerated in the StrUserKernel definition.
 * Returns: A positive integer indicating its position in the array if the
 * string is found, or returns a minus one (-1) if the string is not found. */
static INT PosInArray(_In_ PCWSTR String)
{
    INT i;

    for (i = 0; i < ARRAYSIZE(StrUserKernel); ++i)
    {
        if (wcsncmp(String, StrUserKernel[i], _countof(StrUserKernel[0])) == 0)
            return i;
    }
    return -1;
}

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

static MSGMEMORY g_MsgMemory[] =
{
    { WM_CREATE, MMS_SIZE_SPECIAL, MMS_FLAG_READWRITE },
    { WM_GETMINMAXINFO, sizeof(MINMAXINFO), MMS_FLAG_READWRITE },
    { WM_GETTEXT, MMS_SIZE_WPARAMWCHAR, MMS_FLAG_WRITE },
    { WM_NCCALCSIZE, MMS_SIZE_SPECIAL, MMS_FLAG_READWRITE },
    { WM_NCCREATE, MMS_SIZE_SPECIAL, MMS_FLAG_READWRITE },
    { WM_SETTEXT, MMS_SIZE_LPARAMSZ, MMS_FLAG_READ },
    { WM_STYLECHANGED, sizeof(STYLESTRUCT), MMS_FLAG_READ },
    { WM_STYLECHANGING, sizeof(STYLESTRUCT), MMS_FLAG_READWRITE },
    { WM_SETTINGCHANGE, MMS_SIZE_LPARAMSZ, MMS_FLAG_READ },
    { WM_COPYDATA, MMS_SIZE_SPECIAL, MMS_FLAG_READ },
    { WM_COPYGLOBALDATA, MMS_SIZE_WPARAM, MMS_FLAG_READ },
    { WM_WINDOWPOSCHANGED, sizeof(WINDOWPOS), MMS_FLAG_READWRITE },
    { WM_WINDOWPOSCHANGING, sizeof(WINDOWPOS), MMS_FLAG_READWRITE },
    { WM_SIZING, sizeof(RECT), MMS_FLAG_READWRITE },
    { WM_MOVING, sizeof(RECT), MMS_FLAG_READWRITE },
    { WM_MEASUREITEM, sizeof(MEASUREITEMSTRUCT), MMS_FLAG_READWRITE },
    { WM_DRAWITEM, sizeof(DRAWITEMSTRUCT), MMS_FLAG_READWRITE },
    { WM_HELP, sizeof(HELPINFO), MMS_FLAG_READWRITE },
    { WM_NEXTMENU, sizeof(MDINEXTMENU), MMS_FLAG_READWRITE },
    { WM_DEVICECHANGE, MMS_SIZE_SPECIAL, MMS_FLAG_READ },
};

static PMSGMEMORY FASTCALL
FindMsgMemory(UINT Msg)
{
    PMSGMEMORY MsgMemoryEntry;

    /* See if this message type is present in the table */
    for (MsgMemoryEntry = g_MsgMemory;
    MsgMemoryEntry < g_MsgMemory + sizeof(g_MsgMemory) / sizeof(MSGMEMORY);
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
    PLARGE_STRING WindowName;
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
            // WM_SETTEXT and WM_SETTINGCHANGE can be null!
            if (!lParam)
            {
               TRACE("lParam is NULL!\n");
               Size = 0;
            }
            else
               Size = (UINT) ((wcslen((PWSTR) lParam) + 1) * sizeof(WCHAR));
        }
        else if (MMS_SIZE_SPECIAL == MsgMemoryEntry->Size)
        {
            switch(MsgMemoryEntry->Message)
            {
            case WM_CREATE:
            case WM_NCCREATE:
                Cs = (CREATESTRUCTW *) lParam;
                WindowName = (PLARGE_STRING) Cs->lpszName;
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
                {
                COPYDATASTRUCT *cds = (COPYDATASTRUCT *)lParam;
                Size = sizeof(COPYDATASTRUCT) + cds->cbData;
                }
                break;

            case WM_DEVICECHANGE:
                {
                    if ( lParam && (wParam & 0x8000) )
                    {
                        DEV_BROADCAST_HDR *header = (DEV_BROADCAST_HDR *)lParam;
                        Size = header->dbch_size;
                    }
                }
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
        ERR("Exception caught in MsgMemorySize()! Status: 0x%x\n", _SEH2_GetExceptionCode());
        Size = 0;
    }
    _SEH2_END;
    return Size;
}

UINT lParamMemorySize(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    PMSGMEMORY MsgMemoryEntry = FindMsgMemory(Msg);
    if(MsgMemoryEntry == NULL) return 0;
    return MsgMemorySize(MsgMemoryEntry, wParam, lParam);
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
            ERR("Not enough memory to pack lParam\n");
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
            ERR("Not enough memory to pack lParam\n");
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
            NT_ASSERT(ClassName->Buffer != NULL);
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
        SIZE_T size;

        MsgMemoryEntry = FindMsgMemory(Msg);

        if (!MsgMemoryEntry)
        {
            /* Keep previous behavior */
            return STATUS_SUCCESS;
        }
        size = MsgMemorySize(MsgMemoryEntry, wParam, lParam);
        if (!size)
        {
           ERR("No size for lParamPacked\n");
           return STATUS_SUCCESS;
        }
        PackedData = ExAllocatePoolWithTag(NonPagedPool, size, TAG_MSG);
        if (PackedData == NULL)
        {
            ERR("Not enough memory to pack lParam\n");
            return STATUS_NO_MEMORY;
        }
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
        ASSERT(MsgMemoryEntry);

        if (MsgMemoryEntry->Flags == MMS_FLAG_READWRITE)
        {
            //RtlCopyMemory((PVOID)lParam, (PVOID)lParamPacked, MsgMemoryEntry->Size);
        }
        ExFreePool((PVOID) lParamPacked);
        return STATUS_SUCCESS;
    }

    ASSERT(FALSE);

    return STATUS_INVALID_PARAMETER;
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
            ERR("Not enough memory to copy message to kernel mem\n");
            return STATUS_NO_MEMORY;
        }
        KernelModeMsg->lParam = (LPARAM) KernelMem;

        /* Copy data if required */
        if (0 != (MsgMemoryEntry->Flags & MMS_FLAG_READ))
        {
            TRACE("Copy Message %u from usermode buffer\n", KernelModeMsg->message);
            /* Don't do extra testing for 1 word messages. For examples see
             * https://wiki.winehq.org/List_Of_Windows_Messages and
             * we are just handling WM_WININICHANGE here. */
            if (Size > 1 && UserModeMsg->lParam &&
                KernelModeMsg->message == WM_WININICHANGE)
            {
                WCHAR lParamMsg[_countof(StrUserKernel[0]) + 1] = { 0 };
                _SEH2_TRY
                {
                    RtlCopyMemory(lParamMsg, (WCHAR*)UserModeMsg->lParam, sizeof(lParamMsg));
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    _SEH2_YIELD(return STATUS_ACCESS_VIOLATION);
                }
                _SEH2_END;

                /* Make sure that we have a UNICODE_NULL within lParamMsg */
                lParamMsg[ARRAYSIZE(lParamMsg) - 1] = UNICODE_NULL;

                if (!UserModeMsg->wParam && PosInArray(lParamMsg) >= 0)
                {
                    TRACE("Copy String '%S' from usermode buffer\n", lParamMsg);
                    wcscpy(KernelMem, lParamMsg);
                    return STATUS_SUCCESS;
                }
            }

            Status = MmCopyFromCaller(KernelMem, (PVOID)UserModeMsg->lParam, Size);
            if (!NT_SUCCESS(Status))
            {
                ERR("Failed to copy message to kernel: invalid usermode lParam buffer\n");
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
                ERR("Failed to copy message from kernel: invalid usermode lParam buffer\n");
                ExFreePool((PVOID) KernelModeMsg->lParam);
                return Status;
            }
        }
        ExFreePool((PVOID) KernelModeMsg->lParam);
    }

    return STATUS_SUCCESS;
}

//
// Wakeup any thread/process waiting on idle input.
//
VOID FASTCALL
IdlePing(VOID)
{
   PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();
   PTHREADINFO pti;

   pti = PsGetCurrentThreadWin32Thread();

   if ( pti )
   {
      pti->pClientInfo->cSpins = 0; // Reset spins.

      if ( pti->pDeskInfo && pti == gptiForeground )
      {
         if ( pti->fsHooks & HOOKID_TO_FLAG(WH_FOREGROUNDIDLE) ||
              pti->pDeskInfo->fsHooks & HOOKID_TO_FLAG(WH_FOREGROUNDIDLE) )
         {
            co_HOOK_CallHooks(WH_FOREGROUNDIDLE,HC_ACTION,0,0);
         }
      }
   }

   TRACE("IdlePing ppi %p\n", ppi);
   if ( ppi && ppi->InputIdleEvent )
   {
      TRACE("InputIdleEvent\n");
      KeSetEvent( ppi->InputIdleEvent, IO_NO_INCREMENT, FALSE);
   }
}

VOID FASTCALL
IdlePong(VOID)
{
   PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();

   TRACE("IdlePong ppi %p\n", ppi);
   if ( ppi && ppi->InputIdleEvent )
   {
      KeClearEvent(ppi->InputIdleEvent);
   }
}

static BOOL is_message_broadcastable(UINT msg)
{
    return msg < WM_USER || msg >= 0xc000;
}

UINT FASTCALL
GetWakeMask(UINT first, UINT last )
{
    UINT mask = QS_POSTMESSAGE | QS_SENDMESSAGE;  /* Always selected */

    if (first || last)
    {
        if ((first <= WM_KEYLAST) && (last >= WM_KEYFIRST)) mask |= QS_KEY;
        if ( ((first <= WM_MOUSELAST) && (last >= WM_MOUSEFIRST)) ||
             ((first <= WM_NCMOUSELAST) && (last >= WM_NCMOUSEFIRST)) ) mask |= QS_MOUSE;
        if ((first <= WM_TIMER) && (last >= WM_TIMER)) mask |= QS_TIMER;
        if ((first <= WM_SYSTIMER) && (last >= WM_SYSTIMER)) mask |= QS_TIMER;
        if ((first <= WM_PAINT) && (last >= WM_PAINT)) mask |= QS_PAINT;
    }
    else mask = QS_ALLINPUT;

    return mask;
}

//
// Pass Strings to User Heap Space for Message Hook Callbacks.
//
BOOL
FASTCALL
IntMsgCreateStructW(
    PWND Window,
    CREATESTRUCTW *pCsw,
    CREATESTRUCTW *Cs,
    PVOID *ppszClass,
    PVOID *ppszName )
{
    PLARGE_STRING WindowName;
    PUNICODE_STRING ClassName;
    PVOID pszClass = NULL, pszName = NULL;

    /* Fill the new CREATESTRUCTW */
    RtlCopyMemory(pCsw, Cs, sizeof(CREATESTRUCTW));
    pCsw->style = Window->style; /* HCBT_CREATEWND needs the real window style */

    WindowName = (PLARGE_STRING)   Cs->lpszName;
    ClassName  = (PUNICODE_STRING) Cs->lpszClass;

    // Based on the assumption this is from "unicode source" user32, ReactOS, answer is yes.
    if (!IS_ATOM(ClassName->Buffer))
    {
        if (ClassName->Length)
        {
            if (Window->state & WNDS_ANSICREATOR)
            {
                ANSI_STRING AnsiString;
                AnsiString.MaximumLength = (USHORT)RtlUnicodeStringToAnsiSize(ClassName)+sizeof(CHAR);
                pszClass = UserHeapAlloc(AnsiString.MaximumLength);
                if (!pszClass)
                {
                    ERR("UserHeapAlloc() failed!\n");
                    return FALSE;
                }
                RtlZeroMemory(pszClass, AnsiString.MaximumLength);
                AnsiString.Buffer = (PCHAR)pszClass;
                RtlUnicodeStringToAnsiString(&AnsiString, ClassName, FALSE);
            }
            else
            {
                UNICODE_STRING UnicodeString;
                UnicodeString.MaximumLength = ClassName->Length + sizeof(UNICODE_NULL);
                pszClass = UserHeapAlloc(UnicodeString.MaximumLength);
                if (!pszClass)
                {
                    ERR("UserHeapAlloc() failed!\n");
                    return FALSE;
                }
                RtlZeroMemory(pszClass, UnicodeString.MaximumLength);
                UnicodeString.Buffer = (PWSTR)pszClass;
                RtlCopyUnicodeString(&UnicodeString, ClassName);
            }
            *ppszClass = pszClass;
            pCsw->lpszClass = UserHeapAddressToUser(pszClass);
        }
        else
        {
            pCsw->lpszClass = NULL;
        }
    }
    else
    {
        pCsw->lpszClass = ClassName->Buffer;
    }
    if (WindowName->Length)
    {
        UNICODE_STRING Name;
        Name.Buffer = WindowName->Buffer;
        Name.Length = (USHORT)min(WindowName->Length, MAXUSHORT); // FIXME: LARGE_STRING truncated
        Name.MaximumLength = (USHORT)min(WindowName->MaximumLength, MAXUSHORT);

        if (Window->state & WNDS_ANSICREATOR)
        {
            ANSI_STRING AnsiString;
            AnsiString.MaximumLength = (USHORT)RtlUnicodeStringToAnsiSize(&Name) + sizeof(CHAR);
            pszName = UserHeapAlloc(AnsiString.MaximumLength);
            if (!pszName)
            {
               ERR("UserHeapAlloc() failed!\n");
               return FALSE;
            }
            RtlZeroMemory(pszName, AnsiString.MaximumLength);
            AnsiString.Buffer = (PCHAR)pszName;
            RtlUnicodeStringToAnsiString(&AnsiString, &Name, FALSE);
        }
        else
        {
            UNICODE_STRING UnicodeString;
            UnicodeString.MaximumLength = Name.Length + sizeof(UNICODE_NULL);
            pszName = UserHeapAlloc(UnicodeString.MaximumLength);
            if (!pszName)
            {
               ERR("UserHeapAlloc() failed!\n");
               return FALSE;
            }
            RtlZeroMemory(pszName, UnicodeString.MaximumLength);
            UnicodeString.Buffer = (PWSTR)pszName;
            RtlCopyUnicodeString(&UnicodeString, &Name);
        }
        *ppszName = pszName;
        pCsw->lpszName = UserHeapAddressToUser(pszName);
    }
    else
    {
        pCsw->lpszName = NULL;
    }

    return TRUE;
}

static VOID FASTCALL
IntCallWndProc( PWND Window, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam )
{
    BOOL SameThread = FALSE;
    CWPSTRUCT CWP;
    PVOID pszClass = NULL, pszName = NULL;
    CREATESTRUCTW Csw;

    //// Check for a hook to eliminate overhead. ////
    if ( !ISITHOOKED(WH_CALLWNDPROC) && !(Window->head.rpdesk->pDeskInfo->fsHooks & HOOKID_TO_FLAG(WH_CALLWNDPROC)) )
        return;

    if (Window->head.pti == ((PTHREADINFO)PsGetCurrentThreadWin32Thread()))
        SameThread = TRUE;

    if ( Msg == WM_CREATE || Msg == WM_NCCREATE )
    {   //
        // String pointers are in user heap space, like WH_CBT HCBT_CREATEWND.
        //
        if (!IntMsgCreateStructW( Window, &Csw, (CREATESTRUCTW *)lParam, &pszClass, &pszName ))
            return;
        lParam = (LPARAM)&Csw;
    }

    CWP.hwnd    = hWnd;
    CWP.message = Msg;
    CWP.wParam  = wParam;
    CWP.lParam  = lParam;
    co_HOOK_CallHooks( WH_CALLWNDPROC, HC_ACTION, SameThread, (LPARAM)&CWP );

    if (pszName)  UserHeapFree(pszName);
    if (pszClass) UserHeapFree(pszClass);
}

static VOID FASTCALL
IntCallWndProcRet( PWND Window, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *uResult )
{
    BOOL SameThread = FALSE;
    CWPRETSTRUCT CWPR;
    PVOID pszClass = NULL, pszName = NULL;
    CREATESTRUCTW Csw;

    if ( !ISITHOOKED(WH_CALLWNDPROCRET) && !(Window->head.rpdesk->pDeskInfo->fsHooks & HOOKID_TO_FLAG(WH_CALLWNDPROCRET)) )
        return;

    if (Window->head.pti == ((PTHREADINFO)PsGetCurrentThreadWin32Thread()))
        SameThread = TRUE;

    if ( Msg == WM_CREATE || Msg == WM_NCCREATE )
    {
        if (!IntMsgCreateStructW( Window, &Csw, (CREATESTRUCTW *)lParam, &pszClass, &pszName ))
            return;
        lParam = (LPARAM)&Csw;
    }

    CWPR.hwnd    = hWnd;
    CWPR.message = Msg;
    CWPR.wParam  = wParam;
    CWPR.lParam  = lParam;
    CWPR.lResult = uResult ? (*uResult) : 0;
    co_HOOK_CallHooks( WH_CALLWNDPROCRET, HC_ACTION, SameThread, (LPARAM)&CWPR );

    if (pszName)  UserHeapFree(pszName);
    if (pszClass) UserHeapFree(pszClass);
}

static LRESULT handle_internal_message( PWND pWnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    LRESULT lRes;
//    USER_REFERENCE_ENTRY Ref;
//    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    if (!pWnd || UserIsDesktopWindow(pWnd) || UserIsMessageWindow(pWnd))
       return 0;

    TRACE("Internal Event Msg 0x%x hWnd 0x%p\n", msg, UserHMGetHandle(pWnd));

    switch(msg)
    {
       case WM_ASYNC_SHOWWINDOW:
          return co_WinPosShowWindow( pWnd, wparam );
       case WM_ASYNC_SETWINDOWPOS:
       {
          PWINDOWPOS winpos = (PWINDOWPOS)lparam;
          if (!winpos) return 0;
          lRes = co_WinPosSetWindowPos( pWnd,
                                        winpos->hwndInsertAfter,
                                        winpos->x,
                                        winpos->y,
                                        winpos->cx,
                                        winpos->cy,
                                        winpos->flags);
          ExFreePoolWithTag(winpos, USERTAG_SWP);
          return lRes;
       }
       case WM_ASYNC_DESTROYWINDOW:
       {
          TRACE("WM_ASYNC_DESTROYWINDOW\n");
          if (pWnd->style & WS_CHILD)
             return co_UserFreeWindow(pWnd, PsGetCurrentProcessWin32Process(), PsGetCurrentThreadWin32Thread(), TRUE);
          else
             co_UserDestroyWindow(pWnd);
       }
    }
    return 0;
}

static LRESULT handle_internal_events( PTHREADINFO pti, PWND pWnd, DWORD dwQEvent, LONG_PTR ExtraInfo, PMSG pMsg)
{
    LRESULT Result = 0;

    switch(dwQEvent)
    {
       case POSTEVENT_NWE:
       {
          co_EVENT_CallEvents( pMsg->message, pMsg->hwnd, pMsg->wParam, ExtraInfo);
       }
       break;
       case POSTEVENT_SAW:
       {
          //ERR("HIE : SAW : pti 0x%p hWnd 0x%p\n",pti,pMsg->hwnd);
          IntActivateWindow((PWND)pMsg->wParam, pti, (HANDLE)pMsg->lParam, (DWORD)ExtraInfo);
       }
       break;
       case POSTEVENT_DAW:
       {
          //ERR("HIE : DAW : pti 0x%p tid 0x%p hWndPrev 0x%p\n",pti,ExtraInfo,pMsg->hwnd);
          IntDeactivateWindow(pti, (HANDLE)ExtraInfo);
       }
       break;
    }
    return Result;
}

LRESULT FASTCALL
IntDispatchMessage(PMSG pMsg)
{
    LONG Time;
    LRESULT retval = 0;
    PTHREADINFO pti;
    PWND Window = NULL;
    BOOL DoCallBack = TRUE;

    if (pMsg->hwnd)
    {
        Window = UserGetWindowObject(pMsg->hwnd);
        if (!Window) return 0;
    }

    pti = PsGetCurrentThreadWin32Thread();

    if ( Window && Window->head.pti != pti)
    {
       EngSetLastError( ERROR_MESSAGE_SYNC_ONLY );
       return 0;
    }

    if (((pMsg->message == WM_SYSTIMER) ||
         (pMsg->message == WM_TIMER)) &&
         (pMsg->lParam) )
    {
        if (pMsg->message == WM_TIMER)
        {
            if (ValidateTimerCallback(pti,pMsg->lParam))
            {
                Time = EngGetTickCount32();
                retval = co_IntCallWindowProc((WNDPROC)pMsg->lParam,
                                              TRUE,
                                              pMsg->hwnd,
                                              WM_TIMER,
                                              pMsg->wParam,
                                              (LPARAM)Time,
                                              -1);
            }
            return retval;
        }
        else
        {
            PTIMER pTimer = FindSystemTimer(pMsg);
            if (pTimer && pTimer->pfn)
            {
                Time = EngGetTickCount32();
                pTimer->pfn(pMsg->hwnd, WM_SYSTIMER, (UINT)pMsg->wParam, Time);
            }
            return 0;
        }
    }
    // Need a window!
    if ( !Window ) return 0;

    if (pMsg->message == WM_PAINT) Window->state |= WNDS_PAINTNOTPROCESSED;

    if ( Window->state & WNDS_SERVERSIDEWINDOWPROC )
    {
       TRACE("Dispatch: Server Side Window Procedure\n");
       switch(Window->fnid)
       {
          case FNID_DESKTOP:
            DoCallBack = !DesktopWindowProc( Window,
                                             pMsg->message,
                                             pMsg->wParam,
                                             pMsg->lParam,
                                            &retval);
            break;
          case FNID_MESSAGEWND:
            DoCallBack = !UserMessageWindowProc( Window,
                                                 pMsg->message,
                                                 pMsg->wParam,
                                                 pMsg->lParam,
                                                &retval);
            break;
          case FNID_MENU:
            DoCallBack = !PopupMenuWndProc( Window,
                                            pMsg->message,
                                            pMsg->wParam,
                                            pMsg->lParam,
                                           &retval);
            break;
       }
    }

    /* Since we are doing a callback on the same thread right away, there is
       no need to copy the lparam to kernel mode and then back to usermode.
       We just pretend it isn't a pointer */

    if (DoCallBack)
    retval = co_IntCallWindowProc( Window->lpfnWndProc,
                                   !Window->Unicode,
                                   pMsg->hwnd,
                                   pMsg->message,
                                   pMsg->wParam,
                                   pMsg->lParam,
                                   -1);

    if ( pMsg->message == WM_PAINT &&
         VerifyWnd(Window) &&
         Window->state & WNDS_PAINTNOTPROCESSED ) // <--- Cleared, paint was already processed!
    {
        Window->state2 &= ~WNDS2_WMPAINTSENT;
        /* send a WM_ERASEBKGND if the non-client area is still invalid */
        ERR("Message WM_PAINT count %d Internal Paint Set? %s\n",Window->head.pti->cPaintsReady, Window->state & WNDS_INTERNALPAINT ? "TRUE" : "FALSE");
        IntPaintWindow( Window );
    }

    return retval;
}

/*
 * Internal version of PeekMessage() doing all the work
 *
 * MSDN:
 *   Sent messages
 *   Posted messages
 *   Input (hardware) messages and system internal events
 *   Sent messages (again)
 *   WM_PAINT messages
 *   WM_TIMER messages
 */
BOOL APIENTRY
co_IntPeekMessage( PMSG Msg,
                   PWND Window,
                   UINT MsgFilterMin,
                   UINT MsgFilterMax,
                   UINT RemoveMsg,
                   LONG_PTR *ExtraInfo,
                   BOOL bGMSG )
{
    PTHREADINFO pti;
    BOOL RemoveMessages;
    UINT ProcessMask;
    BOOL Hit = FALSE;

    pti = PsGetCurrentThreadWin32Thread();

    RemoveMessages = RemoveMsg & PM_REMOVE;
    ProcessMask = HIWORD(RemoveMsg);

 /* Hint, "If wMsgFilterMin and wMsgFilterMax are both zero, PeekMessage returns
    all available messages (that is, no range filtering is performed)".        */
    if (!ProcessMask) ProcessMask = (QS_ALLPOSTMESSAGE|QS_ALLINPUT);

    IdlePong();

    do
    {
        /* Update the last message-queue access time */
        pti->pcti->timeLastRead = EngGetTickCount32();

        // Post mouse moves while looping through peek messages.
        if (pti->MessageQueue->QF_flags & QF_MOUSEMOVED)
        {
           IntCoalesceMouseMove(pti);
        }

        /* Dispatch sent messages here. */
        while ( co_MsqDispatchOneSentMessage(pti) )
        {
           /* if some PM_QS* flags were specified, only handle sent messages from now on */
           if (HIWORD(RemoveMsg) && !bGMSG) Hit = TRUE; // wine does this; ProcessMask = QS_SENDMESSAGE;
        }
        if (Hit) return FALSE;

        /* Clear changed bits so we can wait on them if we don't find a message */
        if (ProcessMask & QS_POSTMESSAGE)
        {
           pti->pcti->fsChangeBits &= ~(QS_POSTMESSAGE | QS_HOTKEY | QS_TIMER);
           if (MsgFilterMin == 0 && MsgFilterMax == 0) // Wine hack does this; ~0U)
           {
              pti->pcti->fsChangeBits &= ~QS_ALLPOSTMESSAGE;
           }
        }

        if (ProcessMask & QS_INPUT)
        {
           pti->pcti->fsChangeBits &= ~QS_INPUT;
        }

        /* Now check for normal messages. */
        if (( (ProcessMask & QS_POSTMESSAGE) ||
              (ProcessMask & QS_HOTKEY) ) &&
            MsqPeekMessage( pti,
                            RemoveMessages,
                            Window,
                            MsgFilterMin,
                            MsgFilterMax,
                            ProcessMask,
                            ExtraInfo,
                            0,
                            Msg ))
        {
            goto GotMessage;
        }

        /* Only check for quit messages if not posted messages pending. */
        if (ProcessMask & QS_POSTMESSAGE && pti->QuitPosted)
        {
            /* According to the PSDK, WM_QUIT messages are always returned, regardless
               of the filter specified */
            Msg->hwnd = NULL;
            Msg->message = WM_QUIT;
            Msg->wParam = pti->exitCode;
            Msg->lParam = 0;
            if (RemoveMessages)
            {
                pti->QuitPosted = FALSE;
                ClearMsgBitsMask(pti, QS_POSTMESSAGE);
                pti->pcti->fsWakeBits &= ~QS_ALLPOSTMESSAGE;
                pti->pcti->fsChangeBits &= ~QS_ALLPOSTMESSAGE;
            }
            goto GotMessage;
        }

        /* Check for hardware events. */
        if ((ProcessMask & QS_INPUT) &&
            co_MsqPeekHardwareMessage( pti,
                                       RemoveMessages,
                                       Window,
                                       MsgFilterMin,
                                       MsgFilterMax,
                                       ProcessMask,
                                       Msg))
        {
            goto GotMessage;
        }

        /* Now check for System Event messages. */
        {
           LONG_PTR eExtraInfo;
           MSG eMsg;
           DWORD dwQEvent;
           if (MsqPeekMessage( pti,
                               TRUE,
                               Window,
                               0,
                               0,
                               QS_EVENT,
                              &eExtraInfo,
                              &dwQEvent,
                              &eMsg ))
           {
              handle_internal_events( pti, Window, dwQEvent, eExtraInfo, &eMsg);
              continue;
           }
        }

        /* Check for sent messages again. */
        while ( co_MsqDispatchOneSentMessage(pti) )
        {
           if (HIWORD(RemoveMsg) && !bGMSG) Hit = TRUE;
        }
        if (Hit) return FALSE;

        /* Check for paint messages. */
        if ((ProcessMask & QS_PAINT) &&
            pti->cPaintsReady &&
            IntGetPaintMessage( Window,
                                MsgFilterMin,
                                MsgFilterMax,
                                pti,
                                Msg,
                                RemoveMessages))
        {
            goto GotMessage;
        }

       /* This is correct, check for the current threads timers waiting to be
          posted to this threads message queue. If any we loop again.
        */
        if ((ProcessMask & QS_TIMER) &&
            PostTimerMessages(Window))
        {
            continue;
        }

        return FALSE;
    }
    while (TRUE);

GotMessage:
    /* Update the last message-queue access time */
    pti->pcti->timeLastRead = EngGetTickCount32();
    return TRUE;
}

BOOL FASTCALL
co_IntWaitMessage( PWND Window,
                   UINT MsgFilterMin,
                   UINT MsgFilterMax )
{
    PTHREADINFO pti;
    NTSTATUS Status = STATUS_SUCCESS;
    MSG Msg;
    LONG_PTR ExtraInfo = 0;

    pti = PsGetCurrentThreadWin32Thread();

    do
    {
        if ( co_IntPeekMessage( &Msg,       // Dont reenter!
                                 Window,
                                 MsgFilterMin,
                                 MsgFilterMax,
                                 MAKELONG( PM_NOREMOVE, GetWakeMask( MsgFilterMin, MsgFilterMax)),
                                 &ExtraInfo,
                                 TRUE ) )   // act like GetMessage.
        {
            return TRUE;
        }

        /* Nothing found. Wait for new messages. */
        Status = co_MsqWaitForNewMessages( pti,
                                           Window,
                                           MsgFilterMin,
                                           MsgFilterMax);
        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            ERR("Exit co_IntWaitMessage on error!\n");
            return FALSE;
        }
        if (Status == STATUS_USER_APC || Status == STATUS_TIMEOUT)
        {
           return FALSE;
        }
    }
    while ( TRUE );

    return FALSE;
}

BOOL APIENTRY
co_IntGetPeekMessage( PMSG pMsg,
                      HWND hWnd,
                      UINT MsgFilterMin,
                      UINT MsgFilterMax,
                      UINT RemoveMsg,
                      BOOL bGMSG )
{
    PWND Window;
    PTHREADINFO pti;
    BOOL Present = FALSE;
    NTSTATUS Status;
    LONG_PTR ExtraInfo = 0;

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

    if (bGMSG)
    {
       RemoveMsg |= ((GetWakeMask( MsgFilterMin, MsgFilterMax ))<< 16);
    }

    pti = PsGetCurrentThreadWin32Thread();
    pti->pClientInfo->cSpins++; // Bump up the spin count.

    do
    {
        Present = co_IntPeekMessage( pMsg,
                                     Window,
                                     MsgFilterMin,
                                     MsgFilterMax,
                                     RemoveMsg,
                                     &ExtraInfo,
                                     bGMSG );
        if (Present)
        {
           if ( pMsg->message != WM_DEVICECHANGE || (pMsg->wParam & 0x8000) )
           {
               /* GetMessage or PostMessage must never get messages that contain pointers */
               ASSERT(FindMsgMemory(pMsg->message) == NULL);
           }

           if ( pMsg->message >= WM_DDE_FIRST && pMsg->message <= WM_DDE_LAST )
           {
              if (!IntDdeGetMessageHook(pMsg, ExtraInfo))
              {
                 TRACE("DDE Get return ERROR\n");
                 continue;
              }
           }

           if (pMsg->message != WM_PAINT && pMsg->message != WM_QUIT)
           {
              if (!RtlEqualMemory(&pti->ptLast, &pMsg->pt, sizeof(POINT)))
              {
                 pti->TIF_flags |= TIF_MSGPOSCHANGED;
              }
              pti->timeLast = pMsg->time;
              pti->ptLast   = pMsg->pt;
           }

           // The WH_GETMESSAGE hook enables an application to monitor messages about to
           // be returned by the GetMessage or PeekMessage function.

           co_HOOK_CallHooks( WH_GETMESSAGE, HC_ACTION, RemoveMsg & PM_REMOVE, (LPARAM)pMsg);

           if ( bGMSG || pMsg->message == WM_PAINT) break;
        }

        if ( bGMSG )
        {
            Status = co_MsqWaitForNewMessages( pti,
                                               Window,
                                               MsgFilterMin,
                                               MsgFilterMax);
           if ( !NT_SUCCESS(Status) ||
                Status == STATUS_USER_APC ||
                Status == STATUS_TIMEOUT )
           {
              Present = -1;
              break;
           }
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

    // Been spinning, time to swap vinyl...
    if (pti->pClientInfo->cSpins >= 100)
    {
       // Clear the spin cycle to fix the mix.
       pti->pClientInfo->cSpins = 0;
       //if (!(pti->TIF_flags & TIF_SPINNING)) // FIXME: Need to swap vinyl...
    }
    return Present;
}

BOOL FASTCALL
UserPostThreadMessage( PTHREADINFO pti,
                       UINT Msg,
                       WPARAM wParam,
                       LPARAM lParam )
{
    MSG Message;

    if (is_pointer_message(Msg, wParam))
    {
        EngSetLastError(ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }
    Message.hwnd = NULL;
    Message.message = Msg;
    Message.wParam = wParam;
    Message.lParam = lParam;
    Message.pt = gpsi->ptCursor;
    Message.time = EngGetTickCount32();
    MsqPostMessage(pti, &Message, FALSE, QS_POSTMESSAGE, 0, 0);
    return TRUE;
}

PTHREADINFO FASTCALL
IntSendTo(PWND Window, PTHREADINFO ptiCur, UINT Msg)
{
   if ( ptiCur )
   {
      if (!Window ||
           Window->head.pti == ptiCur )
      {
         return NULL;
      }
   }
   return Window ? Window->head.pti : NULL;
}

BOOL FASTCALL
UserPostMessage( HWND Wnd,
                 UINT Msg,
                 WPARAM wParam,
                 LPARAM lParam )
{
    PTHREADINFO pti;
    MSG Message;
    LONG_PTR ExtraInfo = 0;

    Message.hwnd = Wnd;
    Message.message = Msg;
    Message.wParam = wParam;
    Message.lParam = lParam;
    Message.pt = gpsi->ptCursor;
    Message.time = EngGetTickCount32();

    if (is_pointer_message(Message.message, Message.wParam))
    {
        EngSetLastError(ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }

    if (Wnd == HWND_BROADCAST || Wnd == HWND_TOPMOST)
    {
        HWND *List;
        PWND DesktopWindow;
        ULONG i;

        if (!is_message_broadcastable(Msg)) return TRUE;

        DesktopWindow = UserGetDesktopWindow();
        List = IntWinListChildren(DesktopWindow);

        if (List != NULL)
        {
            UserPostMessage(UserHMGetHandle(DesktopWindow), Msg, wParam, lParam);
            for (i = 0; List[i]; i++)
            {
                PWND pwnd = UserGetWindowObject(List[i]);
                if (!pwnd) continue;

                if ( pwnd->fnid == FNID_MENU || // Also need pwnd->pcls->atomClassName == gaOleMainThreadWndClass
                     pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_SWITCH] )
                   continue;

                UserPostMessage(List[i], Msg, wParam, lParam);
            }
            ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
        }
    }
    else
    {
        PWND Window;

        if (!Wnd)
        {
           return UserPostThreadMessage( gptiCurrent,
                                         Msg,
                                         wParam,
                                         lParam);
        }

        Window = UserGetWindowObject(Wnd);
        if ( !Window )
        {
            ERR("UserPostMessage: Invalid handle 0x%p Msg 0x%x!\n", Wnd, Msg);
            return FALSE;
        }

        pti = Window->head.pti;

        if ( pti->TIF_flags & TIF_INCLEANUP )
        {
            ERR("Attempted to post message to window %p when the thread is in cleanup!\n", Wnd);
            return FALSE;
        }

        if ( Window->state & WNDS_DESTROYED )
        {
            ERR("Attempted to post message to window %p that is being destroyed!\n", Wnd);
            EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
            return FALSE;
        }

        if ( Msg >= WM_DDE_FIRST && Msg <= WM_DDE_LAST )
        {
           if (!IntDdePostMessageHook(Window, Msg, wParam, &lParam, &ExtraInfo))
           {
              TRACE("Posting Exit DDE 0x%x\n",Msg);
              return FALSE;
           }
           Message.lParam = lParam;
        }

        MsqPostMessage(pti, &Message, FALSE, QS_POSTMESSAGE, 0, ExtraInfo);
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

    if (co_IntSendMessageTimeout(hWnd, Msg, wParam, lParam, SMTO_NORMAL, 0, &Result))
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
    NTSTATUS Status = STATUS_SUCCESS;
    PWND Window;
    PMSGMEMORY MsgMemoryEntry;
    INT lParamBufferSize;
    LPARAM lParamPacked;
    PTHREADINFO Win32Thread, ptiSendTo = NULL;
    ULONG_PTR Result = 0;
    LRESULT Ret = FALSE;
    USER_REFERENCE_ENTRY Ref;
    BOOL DoCallBack = TRUE;

    if (!(Window = UserGetWindowObject(hWnd)))
    {
        TRACE("SendMessageTimeoutSingle: Invalid handle 0x%p!\n",hWnd);
        return FALSE;
    }

    UserRefObjectCo(Window, &Ref);

    Win32Thread = PsGetCurrentThreadWin32Thread();

    ptiSendTo = IntSendTo(Window, Win32Thread, Msg);

    if ( Msg >= WM_DDE_FIRST && Msg <= WM_DDE_LAST )
    {
       if (!IntDdeSendMessageHook(Window, Msg, wParam, lParam))
       {
          ERR("Sending Exit DDE 0x%x\n",Msg);
          goto Cleanup; // Return FALSE
       }
    }

    if ( !ptiSendTo )
    {
        if (Win32Thread->TIF_flags & TIF_INCLEANUP)
        {
            /* Never send messages to exiting threads */
            goto Cleanup; // Return FALSE
        }

        if (Msg & 0x80000000)
        {
           TRACE("SMTS: Internal Message!\n");
           Result = (ULONG_PTR)handle_internal_message( Window, Msg, wParam, lParam );
           if (uResult) *uResult = Result;
           Ret = TRUE;
           goto Cleanup;
        }

        // Only happens when calling the client!
        IntCallWndProc( Window, hWnd, Msg, wParam, lParam);

        if ( Window->state & WNDS_SERVERSIDEWINDOWPROC )
        {
           TRACE("SMT: Server Side Window Procedure\n");
           // Handle it here. Safeguard against excessive recursions.
           if (IoGetRemainingStackSize() < PAGE_SIZE)
           {
              ERR("Server Callback Exceeded Stack!\n");
              goto Cleanup; // Return FALSE
           }
           /* Return after server side call, IntCallWndProcRet will not be called. */
           switch(Window->fnid)
           {
              case FNID_DESKTOP:
                DoCallBack = !DesktopWindowProc(Window, Msg, wParam, lParam,(LRESULT*)&Result);
                break;
              case FNID_MESSAGEWND:
                DoCallBack = !UserMessageWindowProc(Window, Msg, wParam, lParam,(LRESULT*)&Result);
                break;
              case FNID_MENU:
                DoCallBack = !PopupMenuWndProc( Window, Msg, wParam, lParam,(LRESULT*)&Result);
                break;
           }
           if (!DoCallBack)
           {
              if (uResult) *uResult = Result;
              Ret = TRUE;
              goto Cleanup;
           }
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
           // If zero, do not allow callback on client side to allocate a buffer!!!!! See CORE-7695.
           if (!lParamBufferSize) lParamBufferSize = -1;
        }

        if (! NT_SUCCESS(PackParam(&lParamPacked, Msg, wParam, lParam, FALSE)))
        {
           ERR("Failed to pack message parameters\n");
           goto Cleanup; // Return FALSE
        }

        Result = (ULONG_PTR)co_IntCallWindowProc( Window->lpfnWndProc,
                                                  !Window->Unicode,
                                                  hWnd,
                                                  Msg,
                                                  wParam,
                                                  lParamPacked,
                                                  lParamBufferSize );
        if (uResult)
        {
            *uResult = Result;
        }

        if (! NT_SUCCESS(UnpackParam(lParamPacked, Msg, wParam, lParam, FALSE)))
        {
            ERR("Failed to unpack message parameters\n");
            Ret = TRUE;
            goto Cleanup;
        }

        // Only happens when calling the client!
        IntCallWndProcRet( Window, hWnd, Msg, wParam, lParam, (LRESULT *)uResult);

        Ret = TRUE;
        goto Cleanup;
    }

    if (Window->state & WNDS_DESTROYED)
    {
        /* FIXME: Last error? */
        ERR("Attempted to send message to window %p that is being destroyed!\n", hWnd);
        goto Cleanup; // Return FALSE
    }

    if ((uFlags & SMTO_ABORTIFHUNG) && MsqIsHung(ptiSendTo, 4 * MSQ_HUNG))
    {
        // FIXME: Set window hung and add to a list.
        /* FIXME: Set a LastError? */
        ERR("Window %p (%p) (pti %p) is hung!\n", hWnd, Window, ptiSendTo);
        goto Cleanup; // Return FALSE
    }

    do
    {
        Status = co_MsqSendMessage( ptiSendTo,
                                    hWnd,
                                    Msg,
                                    wParam,
                                    lParam,
                                    uTimeout,
                                    (uFlags & SMTO_BLOCK),
                                    MSQ_NORMAL,
                                    uResult );
    }
    while ((Status == STATUS_TIMEOUT) &&
           (uFlags & SMTO_NOTIMEOUTIFNOTHUNG) &&
           !MsqIsHung(ptiSendTo, MSQ_HUNG)); // FIXME: Set window hung and add to a list.

    if (Status == STATUS_TIMEOUT)
    {
        if (0 && MsqIsHung(ptiSendTo, MSQ_HUNG))
        {
            TRACE("Let's go Ghost!\n");
            IntMakeHungWindowGhosted(hWnd);
        }
/*
 *  MSDN says:
 *  Microsoft Windows 2000: If GetLastError returns zero, then the function
 *  timed out.
 *  XP+ : If the function fails or times out, the return value is zero.
 *  To get extended error information, call GetLastError. If GetLastError
 *  returns ERROR_TIMEOUT, then the function timed out.
 */
        EngSetLastError(ERROR_TIMEOUT);
        goto Cleanup; // Return FALSE
    }
    else if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        goto Cleanup; // Return FALSE
    }

    Ret = TRUE;

Cleanup:
    UserDerefObjectCo(Window);
    return Ret;
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

    if (hWnd != HWND_BROADCAST && hWnd != HWND_TOPMOST)
    {
        return co_IntSendMessageTimeoutSingle(hWnd, Msg, wParam, lParam, uFlags, uTimeout, uResult);
    }

    if (!is_message_broadcastable(Msg)) return TRUE;

    DesktopWindow = UserGetDesktopWindow();
    if (NULL == DesktopWindow)
    {
       EngSetLastError(ERROR_INTERNAL_ERROR);
       return 0;
    }

    if (hWnd != HWND_TOPMOST)
    {
       /* Send message to the desktop window too! */
       co_IntSendMessageTimeoutSingle(UserHMGetHandle(DesktopWindow), Msg, wParam, lParam, uFlags, uTimeout, uResult);
    }

    Children = IntWinListChildren(DesktopWindow);
    if (NULL == Children)
    {
        return 0;
    }

    for (Child = Children; NULL != *Child; Child++)
    {
        PWND pwnd = UserGetWindowObject(*Child);
        if (!pwnd) continue;

        if ( pwnd->fnid == FNID_MENU ||
             pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_SWITCH] )
           continue;

        co_IntSendMessageTimeoutSingle(*Child, Msg, wParam, lParam, uFlags, uTimeout, uResult);
    }

    ExFreePoolWithTag(Children, USERTAG_WINDOWLIST);

    return (LRESULT) TRUE;
}

LRESULT FASTCALL
co_IntSendMessageNoWait(HWND hWnd,
                        UINT Msg,
                        WPARAM wParam,
                        LPARAM lParam)
{
    ULONG_PTR Result = 0;
    return co_IntSendMessageWithCallBack(hWnd,
                                         Msg,
                                         wParam,
                                         lParam,
                                         NULL,
                                         0,
                                         &Result);
}
/* MSDN:
   If you send a message in the range below WM_USER to the asynchronous message
   functions (PostMessage, SendNotifyMessage, and SendMessageCallback), its
   message parameters cannot include pointers. Otherwise, the operation will fail.
   The functions will return before the receiving thread has had a chance to
   process the message and the sender will free the memory before it is used.
*/
LRESULT FASTCALL
co_IntSendMessageWithCallBack(HWND hWnd,
                              UINT Msg,
                              WPARAM wParam,
                              LPARAM lParam,
                              SENDASYNCPROC CompletionCallback,
                              ULONG_PTR CompletionCallbackContext,
                              ULONG_PTR *uResult)
{
    ULONG_PTR Result;
    PWND Window;
    PMSGMEMORY MsgMemoryEntry;
    INT lParamBufferSize;
    LPARAM lParamPacked;
    PTHREADINFO Win32Thread, ptiSendTo = NULL;
    LRESULT Ret = FALSE;
    USER_REFERENCE_ENTRY Ref;
    PUSER_SENT_MESSAGE Message;
    BOOL DoCallBack = TRUE;

    if (!(Window = UserGetWindowObject(hWnd)))
    {
        TRACE("SendMessageWithCallBack: Invalid handle 0x%p\n",hWnd);
        return FALSE;
    }

    UserRefObjectCo(Window, &Ref);

    if (Window->state & WNDS_DESTROYED)
    {
        /* FIXME: last error? */
        ERR("Attempted to send message to window %p that is being destroyed\n", hWnd);
        goto Cleanup; // Return FALSE
    }

    Win32Thread = PsGetCurrentThreadWin32Thread();

    if (Win32Thread == NULL || Win32Thread->TIF_flags & TIF_INCLEANUP)
        goto Cleanup; // Return FALSE

    ptiSendTo = IntSendTo(Window, Win32Thread, Msg);

    if (Msg & 0x80000000 &&
        !ptiSendTo)
    {
        if (Win32Thread->TIF_flags & TIF_INCLEANUP)
            goto Cleanup; // Return FALSE

        TRACE("SMWCB: Internal Message\n");
        Result = (ULONG_PTR)handle_internal_message(Window, Msg, wParam, lParam);
        if (uResult) *uResult = Result;
        Ret = TRUE;
        goto Cleanup;
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
        if (!lParamBufferSize) lParamBufferSize = -1;
    }

    if (!NT_SUCCESS(PackParam(&lParamPacked, Msg, wParam, lParam, !!ptiSendTo)))
    {
        ERR("Failed to pack message parameters\n");
        goto Cleanup; // Return FALSE
    }

    /* If it can be sent now, then send it. */
    if (!ptiSendTo)
    {
        if (Win32Thread->TIF_flags & TIF_INCLEANUP)
        {
            UnpackParam(lParamPacked, Msg, wParam, lParam, FALSE);
            /* Never send messages to exiting threads */
            goto Cleanup; // Return FALSE
        }

        IntCallWndProc(Window, hWnd, Msg, wParam, lParam);

        if (Window->state & WNDS_SERVERSIDEWINDOWPROC)
        {
           TRACE("SMWCB: Server Side Window Procedure\n");
           switch(Window->fnid)
           {
              case FNID_DESKTOP:
                DoCallBack = !DesktopWindowProc(Window, Msg, wParam, lParamPacked, (LRESULT*)&Result);
                break;
              case FNID_MESSAGEWND:
                DoCallBack = !UserMessageWindowProc(Window, Msg, wParam, lParam, (LRESULT*)&Result);
                break;
              case FNID_MENU:
                DoCallBack = !PopupMenuWndProc(Window, Msg, wParam, lParam, (LRESULT*)&Result);
                break;
           }
        }

        if (DoCallBack)
        Result = (ULONG_PTR)co_IntCallWindowProc(Window->lpfnWndProc,
                                                 !Window->Unicode,
                                                 hWnd,
                                                 Msg,
                                                 wParam,
                                                 lParamPacked,
                                                 lParamBufferSize);
        if(uResult)
        {
            *uResult = Result;
        }

        IntCallWndProcRet(Window, hWnd, Msg, wParam, lParam, (LRESULT *)uResult);

        if (CompletionCallback)
        {
            co_IntCallSentMessageCallback(CompletionCallback,
                                          hWnd,
                                          Msg,
                                          CompletionCallbackContext,
                                          Result);
        }
    }

    if (!ptiSendTo)
    {
        if (!NT_SUCCESS(UnpackParam(lParamPacked, Msg, wParam, lParam, FALSE)))
        {
            ERR("Failed to unpack message parameters\n");
        }
        Ret = TRUE;
        goto Cleanup;
    }

    if(!(Message = AllocateUserMessage(FALSE)))
    {
        ERR("Failed to allocate message\n");
        goto Cleanup; // Return FALSE
    }

    Message->Msg.hwnd = hWnd;
    Message->Msg.message = Msg;
    Message->Msg.wParam = wParam;
    Message->Msg.lParam = lParamPacked;
    Message->pkCompletionEvent = NULL; // No event needed.
    Message->lResult = 0;
    Message->QS_Flags = 0;
    Message->ptiReceiver = ptiSendTo;
    Message->ptiSender = NULL;
    Message->ptiCallBackSender = Win32Thread;
    Message->CompletionCallback = CompletionCallback;
    Message->CompletionCallbackContext = CompletionCallbackContext;
    Message->HookMessage = MSQ_NORMAL;
    Message->HasPackedLParam = (lParamBufferSize > 0);
    Message->QS_Flags = QS_SENDMESSAGE;
    Message->flags = SMF_RECEIVERFREE;

    if (Msg & 0x80000000) // Higher priority event message!
        InsertHeadList(&ptiSendTo->SentMessagesListHead, &Message->ListEntry);
    else
        InsertTailList(&ptiSendTo->SentMessagesListHead, &Message->ListEntry);
    MsqWakeQueue(ptiSendTo, QS_SENDMESSAGE, TRUE);

    Ret = TRUE;

Cleanup:
    UserDerefObjectCo(Window);
    return Ret;
}

#if 0
/*
  This HACK function posts a message if the destination's message queue belongs to
  another thread, otherwise it sends the message. It does not support broadcast
  messages!
*/
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
        TRACE("PostOrSendMessage: Invalid handle 0x%p!\n",hWnd);
        return 0;
    }

    pti = PsGetCurrentThreadWin32Thread();

    if ( IntSendTo(Window, pti, Msg) &&
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
#endif

static LRESULT FASTCALL
co_IntDoSendMessage( HWND hWnd,
                     UINT Msg,
                     WPARAM wParam,
                     LPARAM lParam,
                     PDOSENDMESSAGE dsm)
{
    LRESULT Result = TRUE;
    NTSTATUS Status;
    PWND Window = NULL;
    MSG UserModeMsg, KernelModeMsg;
    PMSGMEMORY MsgMemoryEntry;
    PTHREADINFO ptiSendTo;

    if (hWnd != HWND_BROADCAST && hWnd != HWND_TOPMOST)
    {
        Window = UserGetWindowObject(hWnd);
        if ( !Window )
        {
            return 0;
        }
    }

    /* Check for an exiting window. */
    if (Window && Window->state & WNDS_DESTROYED)
    {
        ERR("co_IntDoSendMessage Window Exiting!\n");
    }

    /* See if the current thread can handle this message */
    ptiSendTo = IntSendTo(Window, gptiCurrent, Msg);

    // If broadcasting or sending to another thread, save the users data.
    if (!Window || ptiSendTo )
    {
       UserModeMsg.hwnd    = hWnd;
       UserModeMsg.message = Msg;
       UserModeMsg.wParam  = wParam;
       UserModeMsg.lParam  = lParam;
       MsgMemoryEntry = FindMsgMemory(UserModeMsg.message);
       Status = CopyMsgToKernelMem(&KernelModeMsg, &UserModeMsg, MsgMemoryEntry);
       if (!NT_SUCCESS(Status))
       {
          EngSetLastError(ERROR_INVALID_PARAMETER);
          return (dsm ? 0 : -1);
       }
    }
    else
    {
       KernelModeMsg.hwnd    = hWnd;
       KernelModeMsg.message = Msg;
       KernelModeMsg.wParam  = wParam;
       KernelModeMsg.lParam  = lParam;
    }

    if (!dsm)
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

    if (!Window || ptiSendTo )
    {
       Status = CopyMsgToUserMem(&UserModeMsg, &KernelModeMsg);
       if (!NT_SUCCESS(Status))
       {
          EngSetLastError(ERROR_INVALID_PARAMETER);
          return(dsm ? 0 : -1);
       }
    }

    return (LRESULT)Result;
}

BOOL FASTCALL
UserSendNotifyMessage( HWND hWnd,
                       UINT Msg,
                       WPARAM wParam,
                       LPARAM lParam )
{
    BOOL Ret = TRUE;

    if (is_pointer_message(Msg, wParam))
    {
        EngSetLastError(ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }

    // Basicly the same as IntPostOrSendMessage
    if (hWnd == HWND_BROADCAST) // Handle Broadcast
    {
        HWND *List;
        PWND DesktopWindow;
        ULONG i;

        DesktopWindow = UserGetDesktopWindow();
        List = IntWinListChildren(DesktopWindow);

        if (List != NULL)
        {
            UserSendNotifyMessage(UserHMGetHandle(DesktopWindow), Msg, wParam, lParam);
            for (i = 0; List[i]; i++)
            {
               PWND pwnd = UserGetWindowObject(List[i]);
               if (!pwnd) continue;

               if ( pwnd->fnid == FNID_MENU ||
                    pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_SWITCH] )
                  continue;

               Ret = UserSendNotifyMessage(List[i], Msg, wParam, lParam);
            }
            ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
        }
    }
    else
    {
        Ret = co_IntSendMessageNoWait( hWnd, Msg, wParam, lParam);
    }
    return Ret;
}


DWORD APIENTRY
IntGetQueueStatus(DWORD Changes)
{
    PTHREADINFO pti;
    DWORD Result;

    pti = PsGetCurrentThreadWin32Thread();
// wine:
    Changes &= (QS_ALLINPUT|QS_ALLPOSTMESSAGE|QS_SMRESULT);

    /* High word, types of messages currently in the queue.
       Low  word, types of messages that have been added to the queue and that
                  are still in the queue
     */
    Result = MAKELONG(pti->pcti->fsChangeBits & Changes, pti->pcti->fsWakeBits & Changes);

    pti->pcti->fsChangeBits &= ~Changes;

    return Result;
}

BOOL APIENTRY
IntInitMessagePumpHook(VOID)
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
IntUninitMessagePumpHook(VOID)
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

BOOL FASTCALL
IntCallMsgFilter( LPMSG lpmsg, INT code)
{
    BOOL Ret = FALSE;

    if ( co_HOOK_CallHooks( WH_SYSMSGFILTER, code, 0, (LPARAM)lpmsg))
    {
        Ret = TRUE;
    }
    else
    {
        Ret = co_HOOK_CallHooks( WH_MSGFILTER, code, 0, (LPARAM)lpmsg);
    }
    return Ret;
}

/** Functions ******************************************************************/

BOOL
APIENTRY
NtUserDragDetect(
   HWND hWnd,
   POINT pt) // Just like the User call.
{
    MSG msg;
    RECT rect;
    ULONG wDragWidth, wDragHeight;
    BOOL Ret = FALSE;

    TRACE("Enter NtUserDragDetect(%p)\n", hWnd);
    UserEnterExclusive();

    wDragWidth = UserGetSystemMetrics(SM_CXDRAG);
    wDragHeight= UserGetSystemMetrics(SM_CYDRAG);

    rect.left = pt.x - wDragWidth;
    rect.right = pt.x + wDragWidth;

    rect.top = pt.y - wDragHeight;
    rect.bottom = pt.y + wDragHeight;

    co_UserSetCapture(hWnd);

    for (;;)
    {
        while (co_IntGetPeekMessage( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE, FALSE ) ||
               co_IntGetPeekMessage( &msg, 0, WM_QUEUESYNC,  WM_QUEUESYNC, PM_REMOVE, FALSE ) ||
               co_IntGetPeekMessage( &msg, 0, WM_KEYFIRST,   WM_KEYLAST,   PM_REMOVE, FALSE ) )
        {
            if ( msg.message == WM_LBUTTONUP )
            {
                co_UserSetCapture(NULL);
                goto Exit; // Return FALSE
            }
            if ( msg.message == WM_MOUSEMOVE )
            {
                POINT tmp;
                tmp.x = (short)LOWORD(msg.lParam);
                tmp.y = (short)HIWORD(msg.lParam);
                if( !RECTL_bPointInRect( &rect, tmp.x, tmp.y ) )
                {
                    co_UserSetCapture(NULL);
                    Ret = TRUE;
                    goto Exit;
                }
            }
            if ( msg.message == WM_KEYDOWN )
            {
                if ( msg.wParam == VK_ESCAPE )
                {
                   co_UserSetCapture(NULL);
                   Ret = TRUE;
                   goto Exit;
                }
            }
            if ( msg.message == WM_QUEUESYNC )
            {
                co_HOOK_CallHooks( WH_CBT, HCBT_QS, 0, 0 );
            }
        }
        co_IntWaitMessage(NULL, 0, 0);
    }

Exit:
   TRACE("Leave NtUserDragDetect, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

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
    BOOL ret = FALSE;
    PETHREAD peThread;
    PTHREADINFO pThread;
    NTSTATUS Status;

    UserEnterExclusive();

    Status = PsLookupThreadByThreadId(UlongToHandle(idThread), &peThread);

    if ( Status == STATUS_SUCCESS )
    {
        pThread = (PTHREADINFO)peThread->Tcb.Win32Thread;
        if( !pThread ||
            !pThread->MessageQueue ||
            (pThread->TIF_flags & TIF_INCLEANUP))
        {
            ObDereferenceObject( peThread );
            goto exit;
        }
        ret = UserPostThreadMessage( pThread, Msg, wParam, lParam);
        ObDereferenceObject( peThread );
    }
    else
    {
        SetLastNtError( Status );
    }
exit:
    UserLeave();
    return ret;
}

BOOL APIENTRY
NtUserWaitMessage(VOID)
{
    BOOL ret;

    UserEnterExclusive();
    TRACE("NtUserWaitMessage Enter\n");
    ret = co_IntWaitMessage(NULL, 0, 0);
    TRACE("NtUserWaitMessage Leave\n");
    UserLeave();

    return ret;
}

BOOL APIENTRY
NtUserGetMessage(PMSG pMsg,
                  HWND hWnd,
                  UINT MsgFilterMin,
                  UINT MsgFilterMax )
{
    MSG Msg;
    BOOL Ret;

    if ( (MsgFilterMin|MsgFilterMax) & ~WM_MAXIMUM )
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
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

    if ((INT)Ret != -1)
       Ret = Ret ? (WM_QUIT != pMsg->message) : FALSE;

    return Ret;
}

BOOL APIENTRY
NtUserPeekMessage( PMSG pMsg,
                  HWND hWnd,
                  UINT MsgFilterMin,
                  UINT MsgFilterMax,
                  UINT RemoveMsg)
{
    MSG Msg;
    BOOL Ret;

    if ( RemoveMsg & PM_BADMSGFLAGS )
    {
        EngSetLastError(ERROR_INVALID_FLAGS);
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
    PWND pWnd;

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
    pWnd = UserGetWindowObject(SafeMsg.hwnd);
    if (pWnd) // Must have a window!
    {
       Ret = IntTranslateKbdMessage(&SafeMsg, flags);
    }
    else
    {
        TRACE("No Window for Translate. hwnd 0x%p Msg %u\n", SafeMsg.hwnd, SafeMsg.message);
        Ret = FALSE;
    }
    UserLeave();

    return Ret;
}

LRESULT APIENTRY ScrollBarWndProc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam);

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

    switch(dwType)
    {
    case FNID_SCROLLBAR:
        {
           lResult = ScrollBarWndProc(hWnd, Msg, wParam, lParam);
           break;
        }
    case FNID_DESKTOP:
        {
           Window = UserGetWindowObject(hWnd);
           if (Window)
           {
              //ERR("FNID_DESKTOP IN\n");
              Ret = DesktopWindowProc(Window, Msg, wParam, lParam, &lResult);
              //ERR("FNID_DESKTOP OUT\n");
           }
           break;
        }
   case FNID_MENU:
       {
          Window = UserGetWindowObject(hWnd);
          if (Window)
          {
              Ret = PopupMenuWndProc( Window, Msg, wParam, lParam, &lResult);
          }
          break;
       }
   case FNID_MESSAGEWND:
       {
           Window = UserGetWindowObject(hWnd);
           if (Window)
           {
                Ret = !UserMessageWindowProc(Window, Msg, wParam, lParam, &lResult);
           }
           break;
       }
    case FNID_DEFWINDOWPROC:
        /* Validate input */
        if (hWnd)
        {
           Window = UserGetWindowObject(hWnd);
           if (!Window)
           {
               UserLeave();
               return FALSE;
           }
           UserRefObjectCo(Window, &Ref);
        }
        lResult = IntDefWindowProc(Window, Msg, wParam, lParam, Ansi);
        Ret = TRUE;
        if (hWnd)
            UserDerefObjectCo(Window);
        break;
    case FNID_SENDNOTIFYMESSAGE:
        Ret = UserSendNotifyMessage(hWnd, Msg, wParam, lParam);
        break;
    case FNID_BROADCASTSYSTEMMESSAGE:
        {
            BROADCASTPARM parm, *retparam;
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
                    _SEH2_YIELD(break);
                }
                _SEH2_END;
            }
            else
                break;

            if ( parm.recipients & BSM_ALLDESKTOPS ||
                 parm.recipients == BSM_ALLCOMPONENTS )
            {
               PLIST_ENTRY DesktopEntry;
               PDESKTOP rpdesk;
               HWND *List, hwndDenied = NULL;
               HDESK hDesk = NULL;
               PWND pwnd, pwndDesk;
               ULONG i;
               UINT fuFlags;

               for (DesktopEntry = InputWindowStation->DesktopListHead.Flink;
                    DesktopEntry != &InputWindowStation->DesktopListHead;
                    DesktopEntry = DesktopEntry->Flink)
               {
                  rpdesk = CONTAINING_RECORD(DesktopEntry, DESKTOP, ListEntry);
                  pwndDesk = rpdesk->pDeskInfo->spwnd;
                  List = IntWinListChildren(pwndDesk);

                  if (parm.flags & BSF_QUERY)
                  {
                     if (List != NULL)
                     {
                        if (parm.flags & BSF_FORCEIFHUNG || parm.flags & BSF_NOHANG)
                        {
                           fuFlags = SMTO_ABORTIFHUNG;
                        }
                        else if (parm.flags & BSF_NOTIMEOUTIFNOTHUNG)
                        {
                           fuFlags = SMTO_NOTIMEOUTIFNOTHUNG;
                        }
                        else
                        {
                           fuFlags = SMTO_NORMAL;
                        }
                        co_IntSendMessageTimeout( UserHMGetHandle(pwndDesk),
                                                  Msg,
                                                  wParam,
                                                  lParam,
                                                  fuFlags,
                                                  2000,
                                                 &RetVal);
                        Ret = TRUE;
                        for (i = 0; List[i]; i++)
                        {
                           pwnd = UserGetWindowObject(List[i]);
                           if (!pwnd) continue;

                           if ( pwnd->fnid == FNID_MENU ||
                                pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_SWITCH] )
                              continue;

                           if ( parm.flags & BSF_IGNORECURRENTTASK )
                           {
                              if ( pwnd->head.pti == gptiCurrent )
                                 continue;
                           }
                           co_IntSendMessageTimeout( List[i],
                                                     Msg,
                                                     wParam,
                                                     lParam,
                                                     fuFlags,
                                                     2000,
                                                    &RetVal);

                           if (!RetVal && EngGetLastError() == ERROR_TIMEOUT)
                           {
                              if (!(parm.flags & BSF_FORCEIFHUNG))
                                 Ret = FALSE;
                           }
                           if (RetVal == BROADCAST_QUERY_DENY)
                           {
                              hwndDenied = List[i];
                              hDesk = UserHMGetHandle(pwndDesk);
                              Ret = FALSE;
                           }
                        }
                        ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
                        _SEH2_TRY
                        {
                           retparam = (PBROADCASTPARM) ResultInfo;
                           retparam->hDesk = hDesk;
                           retparam->hWnd = hwndDenied;
                        }
                        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                        {
                           _SEH2_YIELD(break);
                        }
                        _SEH2_END;
                        if (!Ret) break; // Have a hit! Let everyone know!
                     }
                  }
                  else if (parm.flags & BSF_POSTMESSAGE)
                  {
                     if (List != NULL)
                     {
                        UserPostMessage(UserHMGetHandle(pwndDesk), Msg, wParam, lParam);

                        for (i = 0; List[i]; i++)
                        {
                           pwnd = UserGetWindowObject(List[i]);
                           if (!pwnd) continue;

                           if ( pwnd->fnid == FNID_MENU ||
                                pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_SWITCH] )
                              continue;

                           if ( parm.flags & BSF_IGNORECURRENTTASK )
                           {
                              if ( pwnd->head.pti == gptiCurrent )
                                 continue;
                           }
                           UserPostMessage(List[i], Msg, wParam, lParam);
                        }
                        ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
                     }
                     Ret = TRUE;
                  }
                  else
                  {
                     if (List != NULL)
                     {
                        UserSendNotifyMessage(UserHMGetHandle(pwndDesk), Msg, wParam, lParam);

                        for (i = 0; List[i]; i++)
                        {
                           pwnd = UserGetWindowObject(List[i]);
                           if (!pwnd) continue;

                           if ( pwnd->fnid == FNID_MENU ||
                                pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_SWITCH] )
                              continue;

                           if ( parm.flags & BSF_IGNORECURRENTTASK )
                           {
                              if ( pwnd->head.pti == gptiCurrent )
                                 continue;
                           }
                           UserSendNotifyMessage(List[i], Msg, wParam, lParam);
                        }
                        ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
                     }
                     if (lParam && !wParam && _wcsicmp((WCHAR*)lParam, L"Environment") == 0)
                     {
                         /* Handle Broadcast of WM_SETTINGCHAGE for Environment */
                         co_IntDoSendMessage(HWND_BROADCAST, WM_SETTINGCHANGE,
                                             0, (LPARAM)L"Environment", 0);
                     }
                     Ret = TRUE;
                  }
               }
            }
            else if (parm.recipients & BSM_APPLICATIONS)
            {
               HWND *List, hwndDenied = NULL;
               HDESK hDesk = NULL;
               PWND pwnd, pwndDesk;
               ULONG i;
               UINT fuFlags;

               pwndDesk = UserGetDesktopWindow();
               List = IntWinListChildren(pwndDesk);

               if (parm.flags & BSF_QUERY)
               {
                  if (List != NULL)
                  {
                     if (parm.flags & BSF_FORCEIFHUNG || parm.flags & BSF_NOHANG)
                     {
                        fuFlags = SMTO_ABORTIFHUNG;
                     }
                     else if (parm.flags & BSF_NOTIMEOUTIFNOTHUNG)
                     {
                        fuFlags = SMTO_NOTIMEOUTIFNOTHUNG;
                     }
                     else
                     {
                        fuFlags = SMTO_NORMAL;
                     }
                     co_IntSendMessageTimeout( UserHMGetHandle(pwndDesk),
                                               Msg,
                                               wParam,
                                               lParam,
                                               fuFlags,
                                               2000,
                                              &RetVal);
                     Ret = TRUE;
                     for (i = 0; List[i]; i++)
                     {
                        pwnd = UserGetWindowObject(List[i]);
                        if (!pwnd) continue;

                        if ( pwnd->fnid == FNID_MENU ||
                             pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_SWITCH] )
                           continue;

                        if ( parm.flags & BSF_IGNORECURRENTTASK )
                        {
                           if ( pwnd->head.pti == gptiCurrent )
                              continue;
                        }
                        co_IntSendMessageTimeout( List[i],
                                                  Msg,
                                                  wParam,
                                                  lParam,
                                                  fuFlags,
                                                  2000,
                                                 &RetVal);

                        if (!RetVal && EngGetLastError() == ERROR_TIMEOUT)
                        {
                           if (!(parm.flags & BSF_FORCEIFHUNG))
                              Ret = FALSE;
                        }
                        if (RetVal == BROADCAST_QUERY_DENY)
                        {
                           hwndDenied = List[i];
                           hDesk = UserHMGetHandle(pwndDesk);
                           Ret = FALSE;
                        }
                     }
                     ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
                     _SEH2_TRY
                     {
                        retparam = (PBROADCASTPARM) ResultInfo;
                        retparam->hDesk = hDesk;
                        retparam->hWnd = hwndDenied;
                     }
                     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                     {
                        _SEH2_YIELD(break);
                     }
                     _SEH2_END;
                  }
               }
               else if (parm.flags & BSF_POSTMESSAGE)
               {
                  if (List != NULL)
                  {
                     UserPostMessage(UserHMGetHandle(pwndDesk), Msg, wParam, lParam);

                     for (i = 0; List[i]; i++)
                     {
                        pwnd = UserGetWindowObject(List[i]);
                        if (!pwnd) continue;

                        if ( pwnd->fnid == FNID_MENU ||
                             pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_SWITCH] )
                           continue;

                        if ( parm.flags & BSF_IGNORECURRENTTASK )
                        {
                           if ( pwnd->head.pti == gptiCurrent )
                              continue;
                        }
                        UserPostMessage(List[i], Msg, wParam, lParam);
                     }
                     ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
                  }
                  Ret = TRUE;
               }
               else
               {
                  if (List != NULL)
                  {
                     UserSendNotifyMessage(UserHMGetHandle(pwndDesk), Msg, wParam, lParam);

                     for (i = 0; List[i]; i++)
                     {
                        pwnd = UserGetWindowObject(List[i]);
                        if (!pwnd) continue;

                        if ( pwnd->fnid == FNID_MENU ||
                             pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_SWITCH] )
                           continue;

                        if ( parm.flags & BSF_IGNORECURRENTTASK )
                        {
                           if ( pwnd->head.pti == gptiCurrent )
                              continue;
                        }
                        UserSendNotifyMessage(List[i], Msg, wParam, lParam);
                     }
                     ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
                  }
                  Ret = TRUE;
               }
            }
        }
        break;
    case FNID_SENDMESSAGECALLBACK:
        {
            CALL_BACK_INFO CallBackInfo;
            ULONG_PTR uResult;

            _SEH2_TRY
            {
                ProbeForRead((PVOID)ResultInfo, sizeof(CALL_BACK_INFO), 1);
                RtlCopyMemory(&CallBackInfo, (PVOID)ResultInfo, sizeof(CALL_BACK_INFO));
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            if (is_pointer_message(Msg, wParam))
            {
               EngSetLastError(ERROR_MESSAGE_SYNC_ONLY );
               break;
            }

            if (!(Ret = co_IntSendMessageWithCallBack(hWnd, Msg, wParam, lParam,
                        CallBackInfo.CallBack, CallBackInfo.Context, &uResult)))
            {
                ERR("Callback failure!\n");
            }
        }
        break;
    case FNID_SENDMESSAGE:
        {
            lResult = co_IntDoSendMessage(hWnd, Msg, wParam, lParam, 0);
            Ret = TRUE;

            if (ResultInfo)
            {
                _SEH2_TRY
                {
                    ProbeForWrite((PVOID)ResultInfo, sizeof(ULONG_PTR), 1);
                    RtlCopyMemory((PVOID)ResultInfo, &lResult, sizeof(ULONG_PTR));
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Ret = FALSE;
                    _SEH2_YIELD(break);
                }
                _SEH2_END;
            }
            break;
        }
    case FNID_SENDMESSAGEFF:
    case FNID_SENDMESSAGEWTOOPTION:
        {
            DOSENDMESSAGE dsm, *pdsm = (PDOSENDMESSAGE)ResultInfo;
            if (ResultInfo)
            {
                _SEH2_TRY
                {
                    ProbeForRead(pdsm, sizeof(DOSENDMESSAGE), 1);
                    RtlCopyMemory(&dsm, pdsm, sizeof(DOSENDMESSAGE));
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    _SEH2_YIELD(break);
                }
                _SEH2_END;
            }

            Ret = co_IntDoSendMessage( hWnd, Msg, wParam, lParam, pdsm ? &dsm : NULL );

            if (pdsm)
            {
                _SEH2_TRY
                {
                    ProbeForWrite(pdsm, sizeof(DOSENDMESSAGE), 1);
                    RtlCopyMemory(pdsm, &dsm, sizeof(DOSENDMESSAGE));
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Ret = FALSE;
                    _SEH2_YIELD(break);
                }
                _SEH2_END;
            }
            break;
        }
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
                TRACE("WH_CALLWNDPROC: Hook %p NextHook %p\n", Hook, NextObj);

                lResult = co_IntCallHookProc( Hook->HookId,
                                              HC_ACTION,
                                              ((ClientInfo->CI_flags & CI_CURTHPRHOOK) ? 1 : 0),
                                              (LPARAM)&CWP,
                                              Hook->Proc,
                                              Hook->ihmod,
                                              Hook->offPfn,
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
                                              Hook->ihmod,
                                              Hook->offPfn,
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
    case FNID_SCROLLBAR:
    case FNID_DESKTOP:
    case FNID_MENU:
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
                        IN BOOL bSharedWow)
{
    PEPROCESS Process;
    PPROCESSINFO W32Process;
    PTHREADINFO pti;
    NTSTATUS Status;
    HANDLE Handles[3];
    LARGE_INTEGER Timeout;
    KAPC_STATE ApcState;

    UserEnterExclusive();

    Status = ObReferenceObjectByHandle(hProcess,
                                       PROCESS_QUERY_INFORMATION,
                                       *PsProcessType,
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
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return WAIT_FAILED;
    }

    Handles[0] = Process;
    Handles[1] = W32Process->InputIdleEvent;
    Handles[2] = pti->pEventQueueServer; // IntMsqSetWakeMask returns hEventQueueClient

    if (!Handles[1])
    {
        ObDereferenceObject(Process);
        UserLeave();
        return STATUS_SUCCESS;  /* no event to wait on */
    }

    if (dwMilliseconds != INFINITE)
       Timeout.QuadPart = (LONGLONG) dwMilliseconds * (LONGLONG) -10000;

    KeStackAttachProcess(&Process->Pcb, &ApcState);
    W32Process->W32PF_flags |= W32PF_WAITFORINPUTIDLE;
    for (pti = W32Process->ptiList; pti; pti = pti->ptiSibling)
    {
       pti->TIF_flags |= TIF_WAITFORINPUTIDLE;
       pti->pClientInfo->dwTIFlags = pti->TIF_flags;
    }
    KeUnstackDetachProcess(&ApcState);

    TRACE("WFII: ppi %p\n", W32Process);
    TRACE("WFII: waiting for %p\n", Handles[1] );

    /*
     * We must add a refcount to our current PROCESSINFO,
     * because anything could happen (including process death) we're leaving win32k
     */
    IntReferenceProcessInfo(W32Process);

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
               co_IntGetPeekMessage( &Msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE, FALSE);
               ERR("WFII: WAIT 2\n");
            }
            break;

        case STATUS_TIMEOUT:
            ERR("WFII: timeout\n");
        case WAIT_FAILED:
            goto WaitExit;

        default:
            ERR("WFII: finished\n");
            Status = STATUS_SUCCESS;
            goto WaitExit;
        }
    }
    while (TRUE);

WaitExit:
    KeStackAttachProcess(&Process->Pcb, &ApcState);
    for (pti = W32Process->ptiList; pti; pti = pti->ptiSibling)
    {
       pti->TIF_flags &= ~TIF_WAITFORINPUTIDLE;
       pti->pClientInfo->dwTIFlags = pti->TIF_flags;
    }
    W32Process->W32PF_flags &= ~W32PF_WAITFORINPUTIDLE;
    KeUnstackDetachProcess(&ApcState);

    IntDereferenceProcessInfo(W32Process);
    ObDereferenceObject(Process);
    UserLeave();
    return Status;
}

/* EOF */
