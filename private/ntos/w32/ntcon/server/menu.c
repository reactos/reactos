/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    menu.c

Abstract:

        This file implements the system menu management.

Author:

    Therese Stowell (thereses) Jan-24-1992 (swiped from Win3.1)

--*/

#include "precomp.h"
#pragma hdrstop


VOID
MyModifyMenuItem(
    IN PCONSOLE_INFORMATION Console,
    IN UINT ItemId
    )
/*++

   This routine edits the indicated control to one word. This is used to
        trim the Accelerator key text off of the end of the standard menu
        items because we don't support the accelerators.

--*/

{
    WCHAR ItemString[30];
    int ItemLength;
    MENUITEMINFO mii;

    ItemLength = LoadString(ghInstance,ItemId,ItemString,NELEM(ItemString));
    if (ItemLength == 0) {
        //DbgPrint("LoadString in MyModifyMenu failed %d\n",GetLastError());
        return;
    }

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING;
    mii.dwTypeData = ItemString;

    if (ItemId == SC_CLOSE) {
        mii.fMask |= MIIM_BITMAP;
        mii.hbmpItem = HBMMENU_POPUP_CLOSE;
    }

    SetMenuItemInfo(Console->hMenu, ItemId, FALSE, &mii);

}

VOID
InitSystemMenu(
    IN PCONSOLE_INFORMATION Console
    )
{
    WCHAR ItemString[30];
    int ItemLength;

    //
    // load the clipboard menu.
    //

    Console->hHeirMenu = LoadMenu(ghInstance, MAKEINTRESOURCE(ID_WOMENU));
    if (Console->hHeirMenu) {
        ItemLength = LoadString(ghInstance,cmEdit,ItemString,NELEM(ItemString));
        if (ItemLength == 0)
            KdPrint(("LoadString 1 failed %d\n",GetLastError()));
    }
    else
        KdPrint(("LoadMenu 1 failed %d\n",GetLastError()));

    //
    // edit the accelerators off of the standard items.
    //

    MyModifyMenuItem(Console,SC_CLOSE);

    //
    // Append the clipboard menu to system menu
    //

    if (!AppendMenu(Console->hMenu,
               MF_POPUP | MF_STRING,
               (ULONG_PTR)Console->hHeirMenu,
               //"Edit"
               ItemString
              )) {
        KdPrint(("AppendMenu 1 failed %d\n",GetLastError()));
    }

    //
    // Add other items to system menu
    //

    ItemLength = LoadString(ghInstance,cmDefaults,ItemString,NELEM(ItemString));
    if (ItemLength == 0)
        KdPrint(("LoadString %d failed %d\n",cmDefaults,GetLastError()));
    if (ItemLength) {
        if (!AppendMenu(Console->hMenu, MF_STRING, cmDefaults, ItemString)) {
            KdPrint(("AppendMenu %d failed %d\n",cmDefaults,GetLastError()));
        }
    }
    ItemLength = LoadString(ghInstance,cmControl,ItemString,NELEM(ItemString));
    if (ItemLength == 0)
        KdPrint(("LoadString %d failed %d\n",cmControl,GetLastError()));
    if (ItemLength) {
        if (!AppendMenu(Console->hMenu, MF_STRING, cmControl, ItemString)) {
            KdPrint(("AppendMenu %d failed %d\n",cmControl,GetLastError()));
        }
    }
}


VOID
InitializeMenu(
    IN PCONSOLE_INFORMATION Console
    )
/*++

    this initializes the system menu when a WM_INITMENU message
    is read.

--*/

{
    HMENU hMenu = Console->hMenu;
    HMENU hHeirMenu = Console->hHeirMenu;

    //
    // if we're in graphics mode, disable size menu
    //

    if (!(Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER)) {
        EnableMenuItem(hMenu,SC_SIZE,MF_GRAYED);
    }

    //
    // if the console is iconic, disable Mark and Scroll.
    //

    if (Console->Flags & CONSOLE_IS_ICONIC) {
        EnableMenuItem(hHeirMenu,cmMark,MF_GRAYED);
        EnableMenuItem(hHeirMenu,cmScroll,MF_GRAYED);
    } else {

        //
        // if the console is not iconic
        //   if there are no scroll bars
        //       or we're in mark mode
        //       disable scroll
        //   else
        //       enable scroll
        //
        //   if we're in scroll mode
        //       disable mark
        //   else
        //       enable mark

        if ((Console->CurrentScreenBuffer->WindowMaximizedX &&
             Console->CurrentScreenBuffer->WindowMaximizedY) ||
             Console->Flags & CONSOLE_SELECTING) {
            EnableMenuItem(hHeirMenu,cmScroll,MF_GRAYED);
        } else {
            EnableMenuItem(hHeirMenu,cmScroll,MF_ENABLED);
        }
        if (Console->Flags & CONSOLE_SCROLLING) {
            EnableMenuItem(hHeirMenu,cmMark,MF_GRAYED);
        } else {
            EnableMenuItem(hHeirMenu,cmMark,MF_ENABLED);
        }
    }

    //
    // if we're selecting or scrolling, disable Paste.
    // otherwise enable it.
    //

    if (Console->Flags & (CONSOLE_SELECTING | CONSOLE_SCROLLING)) {
        EnableMenuItem(hHeirMenu,cmPaste,MF_GRAYED);
    } else {
        EnableMenuItem(hHeirMenu,cmPaste,MF_ENABLED);
    }

    //
    // if app has active selection, enable copy; else disabled
    //

    if (Console->Flags & CONSOLE_SELECTING &&
        Console->SelectionFlags & CONSOLE_SELECTION_NOT_EMPTY) {
        EnableMenuItem(hHeirMenu,cmCopy,MF_ENABLED);
    } else {
        EnableMenuItem(hHeirMenu,cmCopy,MF_GRAYED);
    }

    //
    // disable close
    //

    if (Console->Flags & CONSOLE_DISABLE_CLOSE)
        EnableMenuItem(hMenu,SC_CLOSE,MF_GRAYED);
    else
        EnableMenuItem(hMenu,SC_CLOSE,MF_ENABLED);

    //
    // enable Move if not iconic
    //

    if (Console->Flags & CONSOLE_IS_ICONIC) {
        EnableMenuItem(hMenu,SC_MOVE,MF_GRAYED);
    } else {
        EnableMenuItem(hMenu,SC_MOVE,MF_ENABLED);
    }

    //
    // enable Settings if not already doing it
    //

    if (Console->hWndProperties && IsWindow(Console->hWndProperties)) {
        EnableMenuItem(hMenu,cmControl,MF_GRAYED);
    } else {
        EnableMenuItem(hMenu,cmControl,MF_ENABLED);
        Console->hWndProperties = NULL;
    }
}

VOID
SetWinText(
    IN PCONSOLE_INFORMATION Console,
    IN UINT wID,
    IN BOOL Add
    )

/*++

    This routine adds or removes the name to or from the
    beginning of the window title.  The possible names
    are "Scroll", "Mark", "Paste", and "Copy".

--*/

{
    WCHAR TextBuf[256];
    PWCHAR TextBufPtr;
    int TextLength;
    int NameLength;
    WCHAR NameString[20];

    NameLength = LoadString(ghInstance,wID,NameString,
                                  sizeof(NameString)/sizeof(WCHAR));
    if (Add) {
        RtlCopyMemory(TextBuf,NameString,NameLength*sizeof(WCHAR));
        TextBuf[NameLength] = ' ';
        TextBufPtr = TextBuf + NameLength + 1;
    } else {
        TextBufPtr = TextBuf;
    }
    TextLength = GetWindowText(Console->hWnd,
                                  TextBufPtr,
                                  sizeof(TextBuf)/sizeof(WCHAR)-NameLength-1);
    if (TextLength == 0)
        return;
    if (Add) {
        TextBufPtr = TextBuf;
    } else {
        /*
         * The window title might have already been reset, so make sure
         * the name is there before trying to remove it.
         */
        if (wcsncmp(NameString, TextBufPtr, NameLength) != 0)
            return;
        TextBufPtr = TextBuf + NameLength + 1;
    }
    SetWindowText(Console->hWnd,TextBufPtr);
}


VOID
PropertiesDlgShow(
    IN PCONSOLE_INFORMATION Console,
    IN BOOL fCurrent
    )

/*++

    Displays the properties dialog and updates the window state,
    if necessary.

--*/

{
    HANDLE hSection = NULL;
    HANDLE hClientSection = NULL;
    HANDLE hThread;
    SIZE_T ulViewSize;
    LARGE_INTEGER li;
    NTSTATUS Status;
    PCONSOLE_STATE_INFO pStateInfo;
    PCONSOLE_PROCESS_HANDLE ProcessHandleRecord;
    PSCREEN_INFORMATION ScreenInfo;
    LPTHREAD_START_ROUTINE MyPropRoutine;

    /*
     * Map the shared memory block handle into the client side process's
     * address space.
     */
    ProcessHandleRecord = CONTAINING_RECORD(Console->ProcessHandleList.Blink,
                                            CONSOLE_PROCESS_HANDLE,
                                            ListLink);
    /*
     * For global properties pass in hWnd for the hClientSection
     */
    if (!fCurrent) {
        hClientSection = Console->hWnd;
        goto PropCallback;
    }

    /*
     * Create a shared memory block.
     */
    li.QuadPart = sizeof(CONSOLE_STATE_INFO) + Console->OriginalTitleLength;
    Status = NtCreateSection(&hSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &li,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("CONSRV: error %x creating file mapping\n", Status));
        return;
    }

    /*
     * Get a pointer to the shared memory block.
     */
    pStateInfo = NULL;
    ulViewSize = 0;
    Status = NtMapViewOfSection(hSection,
                                NtCurrentProcess(),
                                &pStateInfo,
                                0,
                                0,
                                NULL,
                                &ulViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READWRITE);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("CONSRV: error %x mapping view of file\n", Status));
        NtClose(hSection);
        return;
    }

    /*
     * Fill in the shared memory block with the current values.
     */
    ScreenInfo = Console->CurrentScreenBuffer;
    pStateInfo->Length = li.LowPart;
    pStateInfo->ScreenBufferSize = ScreenInfo->ScreenBufferSize;
    pStateInfo->WindowSize.X = CONSOLE_WINDOW_SIZE_X(ScreenInfo);
    pStateInfo->WindowSize.Y = CONSOLE_WINDOW_SIZE_Y(ScreenInfo);
    pStateInfo->WindowPosX = Console->WindowRect.left;
    pStateInfo->WindowPosY = Console->WindowRect.top;
    if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
        pStateInfo->FontSize = SCR_FONTSIZE(ScreenInfo);
        pStateInfo->FontFamily = SCR_FAMILY(ScreenInfo);
        pStateInfo->FontWeight = SCR_FONTWEIGHT(ScreenInfo);
        wcscpy(pStateInfo->FaceName, SCR_FACENAME(ScreenInfo));
#if defined(FE_SB)
// if TT font has external leading, the Size.Y <> SizeWant.Y
// if we still pass actual Size.Y to console.cpl to query font,
// it will be incorrect. Jun-26-1996

        if (CONSOLE_IS_DBCS_ENABLED() &&
            TM_IS_TT_FONT(SCR_FAMILY(ScreenInfo)))
        {
            if ((SCR_FONTNUMBER(ScreenInfo) >= 0 ) &&
                (SCR_FONTNUMBER(ScreenInfo) < NumberOfFonts)) {

                pStateInfo->FontSize = FontInfo[SCR_FONTNUMBER(ScreenInfo)].SizeWant;
            }
        }
#endif
        pStateInfo->CursorSize = ScreenInfo->BufferInfo.TextInfo.CursorSize;
    }
    pStateInfo->FullScreen = Console->FullScreenFlags & CONSOLE_FULLSCREEN;
    pStateInfo->QuickEdit = Console->Flags & CONSOLE_QUICK_EDIT_MODE;
    pStateInfo->AutoPosition = Console->Flags & CONSOLE_AUTO_POSITION;
    pStateInfo->InsertMode = Console->InsertMode;
    pStateInfo->ScreenAttributes = ScreenInfo->Attributes;
    pStateInfo->PopupAttributes = ScreenInfo->PopupAttributes;
    pStateInfo->HistoryBufferSize = Console->CommandHistorySize;
    pStateInfo->NumberOfHistoryBuffers = Console->MaxCommandHistories;
    pStateInfo->HistoryNoDup = Console->Flags & CONSOLE_HISTORY_NODUP;
    RtlCopyMemory(pStateInfo->ColorTable,
                  Console->ColorTable,
                  sizeof(Console->ColorTable));
    pStateInfo->hWnd = Console->hWnd;
    wcscpy(pStateInfo->ConsoleTitle, Console->OriginalTitle);
#if defined(FE_SB)
    pStateInfo->CodePage = Console->OutputCP;
#endif
    NtUnmapViewOfSection(NtCurrentProcess(), pStateInfo);

    Status = NtDuplicateObject(NtCurrentProcess(),
                               hSection,
                               ProcessHandleRecord->ProcessHandle,
                               &hClientSection,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("CONSRV: error %x mapping handle to client\n", Status));
        NtClose(hSection);
        return;
    }

PropCallback:
    /*
     * Get a pointer to the client-side properties routine.
     */
    MyPropRoutine = ProcessHandleRecord->PropRoutine;
    ASSERT(MyPropRoutine);

    /*
     * Call back into the client process to spawn the properties dialog.
     */
    UnlockConsole(Console);
    hThread = InternalCreateCallbackThread(ProcessHandleRecord->ProcessHandle,
                                           (ULONG_PTR)MyPropRoutine,
                                           (ULONG_PTR)hClientSection);
    if (!hThread) {
        KdPrint(("CONSRV: CreateRemoteThread failed %d\n", GetLastError()));
    }
    LockConsole(Console);

    /*
     * Close any open handles and free allocated memory.
     */
    if (hThread)
        NtClose(hThread);
    if (hSection)
        NtClose(hSection);

    return;
}


VOID
PropertiesUpdate(
    IN PCONSOLE_INFORMATION Console,
    IN HANDLE hClientSection
    )

/*++

    Updates the console state from information sent by the properties
    dialog box.

--*/

{
    HANDLE hSection;
    SIZE_T ulViewSize;
    NTSTATUS Status;
    PCONSOLE_STATE_INFO pStateInfo;
    PCONSOLE_PROCESS_HANDLE ProcessHandleRecord;
    PSCREEN_INFORMATION ScreenInfo;
    ULONG FontIndex;
    WINDOWPLACEMENT wp;
    COORD NewSize;
    WINDOW_LIMITS WindowLimits;

    /*
     * Map the shared memory block handle into our address space.
     */
    ProcessHandleRecord = CONTAINING_RECORD(Console->ProcessHandleList.Blink,
                                            CONSOLE_PROCESS_HANDLE,
                                            ListLink);
    Status = NtDuplicateObject(ProcessHandleRecord->ProcessHandle,
                               hClientSection,
                               NtCurrentProcess(),
                               &hSection,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("CONSRV: error %x mapping client handle\n", Status));
        return;
    }

    /*
     * Get a pointer to the shared memory block.
     */
    pStateInfo = NULL;
    ulViewSize = 0;
    Status = NtMapViewOfSection(hSection,
                                NtCurrentProcess(),
                                &pStateInfo,
                                0,
                                0,
                                NULL,
                                &ulViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READONLY);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("CONSRV: error %x mapping view of file\n", Status));
        NtClose(hSection);
        return;
    }

    /*
     * Verify the size of the shared memory block.
     */
    if (ulViewSize < sizeof(CONSOLE_STATE_INFO)) {
        KdPrint(("CONSRV: sizeof(hSection) < sizeof(CONSOLE_STATE_INFO)\n"));
        NtUnmapViewOfSection(NtCurrentProcess(), pStateInfo);
        NtClose(hSection);
        return;
    }

    ScreenInfo = Console->CurrentScreenBuffer;
#if defined(FE_SB)
    if (Console->OutputCP != pStateInfo->CodePage)
    {
        UINT CodePage = Console->OutputCP;

        Console->OutputCP = pStateInfo->CodePage;
        if (CONSOLE_IS_DBCS_ENABLED())
            Console->fIsDBCSOutputCP = !!IsAvailableFarEastCodePage(Console->OutputCP);
        else
            Console->fIsDBCSOutputCP = FALSE;
        SetConsoleCPInfo(Console,TRUE);
#if defined(FE_IME)
        SetImeOutputCodePage(Console, ScreenInfo, CodePage);
#endif // FE_IME
    }
    if (Console->CP != pStateInfo->CodePage)
    {
        UINT CodePage = Console->CP;

        Console->CP = pStateInfo->CodePage;
        if (CONSOLE_IS_DBCS_ENABLED())
            Console->fIsDBCSCP = !!IsAvailableFarEastCodePage(Console->CP);
        else
            Console->fIsDBCSCP = FALSE;
        SetConsoleCPInfo(Console,FALSE);
#if defined(FE_IME)
        SetImeCodePage(Console);
#endif // FE_IME
    }
#endif // FE_SB

    /*
     * Update the console state from the supplied values.
     */
    if (!(Console->Flags & CONSOLE_VDM_REGISTERED) &&
        (pStateInfo->ScreenBufferSize.X != ScreenInfo->ScreenBufferSize.X ||
         pStateInfo->ScreenBufferSize.Y != ScreenInfo->ScreenBufferSize.Y)) {

        PCOOKED_READ_DATA CookedReadData = Console->lpCookedReadData;

        if (CookedReadData && CookedReadData->NumberOfVisibleChars) {
            DeleteCommandLine(CookedReadData, FALSE);
        }
        ResizeScreenBuffer(ScreenInfo,
                           pStateInfo->ScreenBufferSize,
                           TRUE);
        if (CookedReadData && CookedReadData->NumberOfVisibleChars) {
            RedrawCommandLine(CookedReadData);
        }
    }
#if !defined(FE_SB)
    FontIndex = FindCreateFont(pStateInfo->FontFamily,
                               pStateInfo->FaceName,
                               pStateInfo->FontSize,
                               pStateInfo->FontWeight);
#else
    FontIndex = FindCreateFont(pStateInfo->FontFamily,
                               pStateInfo->FaceName,
                               pStateInfo->FontSize,
                               pStateInfo->FontWeight,
                               pStateInfo->CodePage);
#endif

#if defined(FE_SB)
#if defined(i386)
    if (! (Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
        SetScreenBufferFont(ScreenInfo, FontIndex, pStateInfo->CodePage);
    }
    else {
        ChangeDispSettings(Console, Console->hWnd, 0);
        SetScreenBufferFont(ScreenInfo, FontIndex, pStateInfo->CodePage);
        ConvertToFullScreen(Console);
        ChangeDispSettings(Console, Console->hWnd, CDS_FULLSCREEN);
    }
#else // i386
    SetScreenBufferFont(ScreenInfo, FontIndex, pStateInfo->CodePage);
#endif
#else // FE_SB
    SetScreenBufferFont(ScreenInfo, FontIndex);
#endif // FE_SB
    SetCursorInformation(ScreenInfo,
                         pStateInfo->CursorSize,
                         ScreenInfo->BufferInfo.TextInfo.CursorVisible);

    GetWindowLimits(ScreenInfo, &WindowLimits);
    NewSize.X = min(pStateInfo->WindowSize.X, WindowLimits.MaximumWindowSize.X);
    NewSize.Y = min(pStateInfo->WindowSize.Y, WindowLimits.MaximumWindowSize.Y);
    if (NewSize.X != CONSOLE_WINDOW_SIZE_X(ScreenInfo) ||
        NewSize.Y != CONSOLE_WINDOW_SIZE_Y(ScreenInfo)) {
        wp.length = sizeof(wp);
        GetWindowPlacement(Console->hWnd, &wp);
        wp.rcNormalPosition.right += (NewSize.X - CONSOLE_WINDOW_SIZE_X(ScreenInfo)) *
                            SCR_FONTSIZE(ScreenInfo).X;
        wp.rcNormalPosition.bottom += (NewSize.Y - CONSOLE_WINDOW_SIZE_Y(ScreenInfo)) *
                             SCR_FONTSIZE(ScreenInfo).Y;
        SetWindowPlacement(Console->hWnd, &wp);
    }

#ifdef i386
    if (FullScreenInitialized) {
        if (pStateInfo->FullScreen == FALSE) {
            if (Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
                ConvertToWindowed(Console);
#if defined(FE_SB)
                /*
                 * Should not sets 0 always.
                 * because exist CONSOLE_FULLSCREEN_HARDWARE bit by avobe
                 *   else {
                 *       ChangeDispSettings(Console, Console->hWnd, 0);
                 *       SetScreenBufferFont(ScreenInfo, FontIndex, pStateInfo->CodePage);
                 *       ConvertToFullScreen(Console);
                 *       ChangeDispSettings(Console, Console->hWnd, CDS_FULLSCREEN);
                 *   }
                 * block.
                 *
                 * This block enable as follows:
                 *   1. console window is full screen
                 *   2. open property by ALT+SPACE
                 *   3. changes window mode by settings.
                 */
                Console->FullScreenFlags &= ~CONSOLE_FULLSCREEN;
#else
                ASSERT(!(Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE));
                Console->FullScreenFlags = 0;
#endif

                ChangeDispSettings(Console, Console->hWnd, 0);
            }
        } else {
            if (Console->FullScreenFlags == 0) {
                ConvertToFullScreen(Console);
                Console->FullScreenFlags |= CONSOLE_FULLSCREEN;

                ChangeDispSettings(Console, Console->hWnd, CDS_FULLSCREEN);
            }
        }
    }
#endif
    if (pStateInfo->QuickEdit) {
        Console->Flags |= CONSOLE_QUICK_EDIT_MODE;
    } else {
        Console->Flags &= ~CONSOLE_QUICK_EDIT_MODE;
    }
    if (pStateInfo->AutoPosition) {
        Console->Flags |= CONSOLE_AUTO_POSITION;
    } else {
        Console->Flags &= ~CONSOLE_AUTO_POSITION;
        SetWindowPos(Console->hWnd, NULL,
                        pStateInfo->WindowPosX,
                        pStateInfo->WindowPosY,
                        0, 0, SWP_NOZORDER | SWP_NOSIZE);
    }
    if (Console->InsertMode != pStateInfo->InsertMode) {
        SetCursorMode(ScreenInfo, FALSE);
        Console->InsertMode = (pStateInfo->InsertMode != FALSE);
#ifdef FE_SB
        if (Console->lpCookedReadData) {
            ((PCOOKED_READ_DATA)Console->lpCookedReadData)->InsertMode = Console->InsertMode;
        }
#endif
    }

    RtlCopyMemory(Console->ColorTable,
                  pStateInfo->ColorTable,
                  sizeof(Console->ColorTable));
    SetScreenColors(ScreenInfo,
                    pStateInfo->ScreenAttributes,
                    pStateInfo->PopupAttributes,
                    TRUE);

    ResizeCommandHistoryBuffers(Console, pStateInfo->HistoryBufferSize);
    Console->MaxCommandHistories = (SHORT)pStateInfo->NumberOfHistoryBuffers;
    if (pStateInfo->HistoryNoDup) {
        Console->Flags |= CONSOLE_HISTORY_NODUP;
    } else {
        Console->Flags &= ~CONSOLE_HISTORY_NODUP;
    }

#if defined(FE_IME)
    SetUndetermineAttribute(Console) ;
#endif

    NtUnmapViewOfSection(NtCurrentProcess(), pStateInfo);
    NtClose(hSection);

    return;
}
