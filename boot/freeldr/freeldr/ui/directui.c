/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/freeldr/ui/directui.c
 * PURPOSE:         FreeLDR UI Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#if 0
#include <freeldr.h>

/* GLOBALS ********************************************************************/

ULONG UiScreenWidth;
ULONG UiScreenHeight;
UCHAR UiMenuFgColor = COLOR_GRAY;
UCHAR UiMenuBgColor = COLOR_BLACK;
UCHAR UiTextColor = COLOR_GRAY;
UCHAR UiSelectedTextColor = COLOR_BLACK;
UCHAR UiSelectedTextBgColor = COLOR_GRAY;
CHAR  UiTimeText[260] = "Seconds until highlighted choice will be started automatically:";

/* FUNCTIONS ******************************************************************/

BOOLEAN
UiInitialize(IN BOOLEAN ShowUi)
{
    ULONG Depth;

    /* Nothing to do */
    if (!ShowUi) return TRUE;

    /* Set mode and query size */
    MachVideoSetDisplayMode(NULL, TRUE);
    MachVideoGetDisplaySize(&UiScreenWidth, &UiScreenHeight, &Depth);

    /* Clear the screen */
    UiDrawBackdrop();
    return TRUE;
}

VOID
UiUnInitialize(IN PCSTR BootText)
{
    /* Nothing to do */
    return;
}

VOID
UiDrawBackdrop(VOID)
{
    /* Clear the screen */
    MachVideoClearScreen(ATTR(COLOR_WHITE, COLOR_BLACK));
}

VOID
UiDrawText(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ PCSTR Text,
    _In_ UCHAR Attr)
{
    TuiDrawText2(X, Y, 0 /*(ULONG)strlen(Text)*/, Text, Attr);
}

VOID
UiDrawText2(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_opt_ ULONG MaxNumChars,
    _In_reads_or_z_(MaxNumChars) PCSTR Text,
    _In_ UCHAR Attr)
{
    TuiDrawText2(X, Y, MaxNumChars, Text, Attr);
}

VOID
UiDrawCenteredText(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR TextString,
    _In_ UCHAR Attr)
{
    TuiDrawCenteredText(Left, Top, Right, Bottom, TextString, Attr);
}

VOID
UiDrawStatusText(IN PCSTR StatusText)
{
    return;
}

VOID
UiInfoBox(
    _In_ PCSTR MessageText)
{
    TuiPrintf(MessageText);
}

VOID
UiMessageBox(
    _In_ PCSTR MessageText)
{
    TuiPrintf(MessageText);
}

VOID
UiMessageBoxCritical(
    _In_ PCSTR MessageText)
{
    TuiPrintf(MessageText);
}

VOID
UiShowMessageBoxesInSection(
    IN ULONG_PTR SectionId)
{
    return;
}

VOID
UiShowMessageBoxesInArgv(
    IN ULONG Argc,
    IN PCHAR Argv[])
{
    return;
}

VOID
UiDrawMenu(
    _In_ PUI_MENU_INFO MenuInfo)
{
    MiniTuiDrawMenu(MenuInfo);
}

BOOLEAN
UiDisplayMenu(
    IN PCSTR MenuHeader,
    IN PCSTR MenuFooter OPTIONAL,
    IN PCSTR MenuItemList[],
    IN ULONG MenuItemCount,
    IN ULONG DefaultMenuItem,
    IN LONG MenuTimeOut,
    OUT PULONG SelectedMenuItem,
    IN BOOLEAN CanEscape,
    IN UiMenuKeyPressFilterCallback KeyPressFilter OPTIONAL,
    IN PVOID Context OPTIONAL)
{
    return TuiDisplayMenu(MenuHeader,
                          MenuFooter,
                          ShowBootOptions,
                          MenuItemList,
                          MenuItemCount,
                          DefaultMenuItem,
                          MenuTimeOut,
                          SelectedMenuItem,
                          CanEscape,
                          KeyPressFilter,
                          Context);
}

#endif // _M_ARM
