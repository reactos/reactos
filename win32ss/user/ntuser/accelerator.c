/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window accelerator
 * FILE:             win32ss/user/ntuser/accelerator.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   Copyright 1993 Martin Ayotte
 *                   Copyright 1994 Alexandre Julliard
 *                   Copyright 1997 Morten Welinder
 *                   Copyright 2011 Rafal Harabien
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserAccel);

#define FVIRT_TBL_END 0x80
#define FVIRT_MASK 0x7F

/* FUNCTIONS *****************************************************************/

PACCELERATOR_TABLE FASTCALL UserGetAccelObject(HACCEL hAccel)
{
    PACCELERATOR_TABLE Accel;

    if (!hAccel)
    {
        EngSetLastError(ERROR_INVALID_ACCEL_HANDLE);
        return NULL;
    }

    Accel = UserGetObject(gHandleTable, hAccel, TYPE_ACCELTABLE);
    if (!Accel)
    {
        EngSetLastError(ERROR_INVALID_ACCEL_HANDLE);
        return NULL;
    }

    return Accel;
}


static
BOOLEAN FASTCALL
co_IntTranslateAccelerator(
    PWND Window,
    CONST MSG *pMsg,
    CONST ACCEL *pAccel)
{
    BOOL bFound = FALSE;
    UINT Mask = 0, nPos;
    HWND hWnd;
    HMENU hMenu, hSubMenu;
    PMENU MenuObject;

    ASSERT_REFS_CO(Window);

    hWnd = Window->head.h;

    TRACE("IntTranslateAccelerator(hwnd %p, message %x, wParam %x, lParam %x, fVirt 0x%x, key %x, cmd %x)\n",
          hWnd, pMsg->message, pMsg->wParam, pMsg->lParam, pAccel->fVirt, pAccel->key, pAccel->cmd);

    if (UserGetKeyState(VK_CONTROL) & 0x8000) Mask |= FCONTROL;
    if (UserGetKeyState(VK_MENU) & 0x8000) Mask |= FALT; // FIXME: VK_LMENU (msg winetest!)
    if (UserGetKeyState(VK_SHIFT) & 0x8000) Mask |= FSHIFT;
    TRACE("Mask 0x%x\n", Mask);

    if (pAccel->fVirt & FVIRTKEY)
    {
        /* This is a virtual key. Process WM_(SYS)KEYDOWN messages. */
        if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN)
        {
            /* Check virtual key and SHIFT, CTRL, LALT state */
            if (pMsg->wParam == pAccel->key && Mask == (pAccel->fVirt & (FSHIFT | FCONTROL | FALT)))
            {
                bFound = TRUE;
            }
        }
    }
    else
    {
        /* This is a char code. Process WM_(SYS)CHAR messages. */
        if (pMsg->message == WM_CHAR || pMsg->message == WM_SYSCHAR)
        {
            /* Check char code and LALT state only */
            if (pMsg->wParam == pAccel->key && (Mask & FALT) == (pAccel->fVirt & FALT))
            {
                bFound = TRUE;
            }
        }
    }

    if (!bFound)
    {
        /* Don't translate this msg */
        TRACE("IntTranslateAccelerator returns FALSE\n");
        return FALSE;
    }

    /* Check if accelerator is associated with menu command */
    hMenu = (Window->style & WS_CHILD) ? 0 : (HMENU)Window->IDMenu;
    hSubMenu = NULL;
    MenuObject = UserGetMenuObject(hMenu);
    nPos = pAccel->cmd;
    if (MenuObject)
    {
        if ((MENU_FindItem (&MenuObject, &nPos, MF_BYPOSITION)))
            hSubMenu = MenuObject->head.h;
        else
            hMenu = NULL;
    }
    if (!hMenu)
    {
        /* Check system menu now */
        hMenu = Window->SystemMenu;
        hSubMenu = hMenu; /* system menu is a popup menu */
        MenuObject = UserGetMenuObject(hMenu);
        nPos = pAccel->cmd;
        if (MenuObject)
        {
            if ((MENU_FindItem (&MenuObject, &nPos, MF_BYPOSITION)))
                hSubMenu = MenuObject->head.h;
            else
                hMenu = NULL;
        }
    }

    /* If this is a menu item, there is no capturing enabled and
       window is not disabled, send WM_INITMENU */
    if (hMenu && !IntGetCaptureWindow())
    {
        co_IntSendMessage(hWnd, WM_INITMENU, (WPARAM)hMenu, 0L);
        if (hSubMenu)
        {
            nPos = IntFindSubMenu(&hMenu, hSubMenu);
            TRACE("hSysMenu = %p, hSubMenu = %p, nPos = %u\n", hMenu, hSubMenu, nPos);
            co_IntSendMessage(hWnd, WM_INITMENUPOPUP, (WPARAM)hSubMenu, MAKELPARAM(nPos, TRUE));
        }
    }

    /* Don't send any message if:
       - window is disabled
       - menu item is disabled
       - this is window menu and window is minimized */
    if (!(Window->style & WS_DISABLED) &&
            !(hMenu && IntGetMenuState(hMenu, pAccel->cmd, MF_BYCOMMAND) & (MF_DISABLED | MF_GRAYED)) &&
            !(hMenu && hMenu == (HMENU)Window->IDMenu && (Window->style & WS_MINIMIZED)))
    {
        /* If this is system menu item, send WM_SYSCOMMAND, otherwise send WM_COMMAND */
        if (hMenu && hMenu == Window->SystemMenu)
        {
            TRACE("Sending WM_SYSCOMMAND, wParam=%0x\n", pAccel->cmd);
            co_IntSendMessage(hWnd, WM_SYSCOMMAND, pAccel->cmd, 0x00010000L);
        }
        else
        {
            TRACE("Sending WM_COMMAND, wParam=%0x\n", 0x10000 | pAccel->cmd);
            co_IntSendMessage(hWnd, WM_COMMAND, 0x10000 | pAccel->cmd, 0L);
        }
    }

    TRACE("IntTranslateAccelerator returns TRUE\n");
    return TRUE;
}


/* SYSCALLS *****************************************************************/


ULONG
APIENTRY
NtUserCopyAcceleratorTable(
    HACCEL hAccel,
    LPACCEL Entries,
    ULONG EntriesCount)
{
    PACCELERATOR_TABLE Accel;
    ULONG Ret;
    DECLARE_RETURN(int);

    TRACE("Enter NtUserCopyAcceleratorTable\n");
    UserEnterShared();

    Accel = UserGetAccelObject(hAccel);
    if (!Accel)
    {
        RETURN(0);
    }

    /* If Entries is NULL return table size */
    if (!Entries)
    {
        RETURN(Accel->Count);
    }

    /* Don't overrun */
    if (Accel->Count < EntriesCount)
        EntriesCount = Accel->Count;

    Ret = 0;

    _SEH2_TRY
    {
        ProbeForWrite(Entries, EntriesCount*sizeof(Entries[0]), 4);

        for (Ret = 0; Ret < EntriesCount; Ret++)
        {
            Entries[Ret].fVirt = Accel->Table[Ret].fVirt;
            Entries[Ret].key = Accel->Table[Ret].key;
            Entries[Ret].cmd = Accel->Table[Ret].cmd;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        Ret = 0;
    }
    _SEH2_END;

    RETURN(Ret);

CLEANUP:
    TRACE("Leave NtUserCopyAcceleratorTable, ret=%i\n", _ret_);
    UserLeave();
    END_CLEANUP;
}

HACCEL
APIENTRY
NtUserCreateAcceleratorTable(
    LPACCEL Entries,
    ULONG EntriesCount)
{
    PACCELERATOR_TABLE Accel;
    HACCEL hAccel;
    ULONG Index;
    NTSTATUS Status = STATUS_SUCCESS;
    DECLARE_RETURN(HACCEL);
    PTHREADINFO pti;

    TRACE("Enter NtUserCreateAcceleratorTable(Entries %p, EntriesCount %u)\n",
          Entries, EntriesCount);
    UserEnterExclusive();

    if (!Entries || EntriesCount <= 0)
    {
        SetLastNtError(STATUS_INVALID_PARAMETER);
        RETURN( (HACCEL) NULL );
    }

    pti = PsGetCurrentThreadWin32Thread();

    Accel = UserCreateObject(gHandleTable,
        pti->rpdesk,
        pti,
        (PHANDLE)&hAccel,
        TYPE_ACCELTABLE,
        sizeof(ACCELERATOR_TABLE));

    if (Accel == NULL)
    {
        SetLastNtError(STATUS_NO_MEMORY);
        RETURN( (HACCEL) NULL );
    }

    Accel->Count = EntriesCount;
    Accel->Table = ExAllocatePoolWithTag(PagedPool, EntriesCount * sizeof(ACCEL), USERTAG_ACCEL);
    if (Accel->Table == NULL)
    {
        UserDereferenceObject(Accel);
        UserDeleteObject(hAccel, TYPE_ACCELTABLE);
        SetLastNtError(STATUS_NO_MEMORY);
        RETURN( (HACCEL) NULL);
    }

    _SEH2_TRY
    {
        ProbeForRead(Entries, EntriesCount * sizeof(ACCEL), 4);

        for (Index = 0; Index < EntriesCount; Index++)
        {
            Accel->Table[Index].fVirt = Entries[Index].fVirt & FVIRT_MASK;
            if(Accel->Table[Index].fVirt & FVIRTKEY)
            {
                Accel->Table[Index].key = Entries[Index].key;
            }
            else
            {
                RtlMultiByteToUnicodeN(&Accel->Table[Index].key,
                sizeof(WCHAR),
                NULL,
                (PCSTR)&Entries[Index].key,
                sizeof(CHAR));
            }

            Accel->Table[Index].cmd = Entries[Index].cmd;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Accel->Table, USERTAG_ACCEL);
        UserDereferenceObject(Accel);
        UserDeleteObject(hAccel, TYPE_ACCELTABLE);
        SetLastNtError(Status);
        RETURN( (HACCEL) NULL);
    }

    /* FIXME: Save HandleTable in a list somewhere so we can clean it up again */

    /* Release the extra reference (UserCreateObject added 2 references) */
    UserDereferenceObject(Accel);

    RETURN(hAccel);

CLEANUP:
    TRACE("Leave NtUserCreateAcceleratorTable(Entries %p, EntriesCount %u) = %p\n",
          Entries, EntriesCount, _ret_);
    UserLeave();
    END_CLEANUP;
}

BOOLEAN
UserDestroyAccelTable(PVOID Object)
{
    PACCELERATOR_TABLE Accel = Object;

    if (Accel->Table != NULL)
    {
        ExFreePoolWithTag(Accel->Table, USERTAG_ACCEL);
        Accel->Table = NULL;
    }

    UserDeleteObject(Accel->head.h, TYPE_ACCELTABLE);
    return TRUE;
}

BOOLEAN
APIENTRY
NtUserDestroyAcceleratorTable(
    HACCEL hAccel)
{
    PACCELERATOR_TABLE Accel;
    DECLARE_RETURN(BOOLEAN);

    /* FIXME: If the handle table is from a call to LoadAcceleratorTable, decrement it's
       usage count (and return TRUE).
    FIXME: Destroy only tables created using CreateAcceleratorTable.
     */

    TRACE("NtUserDestroyAcceleratorTable(Table %p)\n", hAccel);
    UserEnterExclusive();

    if (!(Accel = UserGetAccelObject(hAccel)))
    {
        RETURN( FALSE);
    }

    UserDestroyAccelTable(Accel);

    RETURN( TRUE);

CLEANUP:
    TRACE("Leave NtUserDestroyAcceleratorTable(Table %p) = %u\n", hAccel, _ret_);
    UserLeave();
    END_CLEANUP;
}

int
APIENTRY
NtUserTranslateAccelerator(
    HWND hWnd,
    HACCEL hAccel,
    LPMSG pUnsafeMessage)
{
    PWND Window = NULL;
    PACCELERATOR_TABLE Accel = NULL;
    ULONG i;
    MSG Message;
    USER_REFERENCE_ENTRY AccelRef, WindowRef;
    DECLARE_RETURN(int);

    TRACE("NtUserTranslateAccelerator(hWnd %p, hAccel %p, Message %p)\n",
          hWnd, hAccel, pUnsafeMessage);
    UserEnterShared();

    if (hWnd == NULL)
    {
        RETURN( 0);
    }

    _SEH2_TRY
    {
        ProbeForRead(pUnsafeMessage, sizeof(MSG), 4);
        RtlCopyMemory(&Message, pUnsafeMessage, sizeof(MSG));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(RETURN( 0));
    }
    _SEH2_END;

    if ((Message.message != WM_KEYDOWN) &&
        (Message.message != WM_SYSKEYDOWN) &&
        (Message.message != WM_SYSCHAR) &&
        (Message.message != WM_CHAR))
    {
        RETURN( 0);
    }

    Accel = UserGetAccelObject(hAccel);
    if (!Accel)
    {
        RETURN( 0);
    }

    UserRefObjectCo(Accel, &AccelRef);

    Window = UserGetWindowObject(hWnd);
    if (!Window)
    {
        RETURN( 0);
    }

    UserRefObjectCo(Window, &WindowRef);

    /* FIXME: Associate AcceleratorTable with the current thread */

    for (i = 0; i < Accel->Count; i++)
    {
        if (co_IntTranslateAccelerator(Window, &Message, &Accel->Table[i]))
        {
            RETURN( 1);
        }

        /* Undocumented feature... */
        if (Accel->Table[i].fVirt & FVIRT_TBL_END)
            break;
    }

    RETURN( 0);

CLEANUP:
    if (Window) UserDerefObjectCo(Window);
    if (Accel) UserDerefObjectCo(Accel);

    TRACE("NtUserTranslateAccelerator returns %d\n", _ret_);
    UserLeave();
    END_CLEANUP;
}
