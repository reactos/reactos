/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeLoader
 * FILE:            boot/freeldr/freeldr/ui/noui.c
 * PURPOSE:         No Text UI interface
 * PROGRAMMERS:     Hervé Poussineau
 */

#include <freeldr.h>

BOOLEAN NoUiInitialize(VOID)
{
    return TRUE;
}

VOID NoUiUnInitialize(VOID)
{
}

VOID NoUiDrawBackdrop(VOID)
{
}

VOID NoUiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, CHAR FillChar, UCHAR Attr)
{
}

VOID NoUiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom)
{
}

VOID NoUiDrawBox(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOLEAN Fill, BOOLEAN Shadow, UCHAR Attr)
{
}

VOID
NoUiDrawText(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ PCSTR Text,
    _In_ UCHAR Attr)
{
    printf("%s\n", Text);
}

VOID
NoUiDrawText2(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_opt_ ULONG MaxNumChars,
    _In_reads_or_z_(MaxNumChars) PCSTR Text,
    _In_ UCHAR Attr)
{
    if (MaxNumChars == 0)
        MaxNumChars = (ULONG)strlen(Text);
    printf("%*s\n", MaxNumChars, Text);
}

VOID
NoUiDrawCenteredText(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR TextString,
    _In_ UCHAR Attr)
{
    printf("%s\n", TextString);
}

VOID NoUiDrawStatusText(PCSTR StatusText)
{
    printf("%s\n", StatusText);
}

VOID NoUiUpdateDateTime(VOID)
{
}

VOID
NoUiMessageBox(
    _In_ PCSTR MessageText)
{
    NoUiMessageBoxCritical(MessageText);
}

VOID
NoUiMessageBoxCritical(
    _In_ PCSTR MessageText)
{
    /*
     * We have not yet displayed the user interface
     * We are probably still reading the .ini file
     * and have encountered an error. Just use printf()
     * and return.
     */
    printf("%s\n", MessageText);
    printf("Press any key\n");
    MachConsGetCh();
}

/* Loading Progress-Bar Functions ********************************************/

VOID
NoUiSetProgressBarText(
    _In_ PCSTR ProgressText)
{
}

VOID
NoUiTickProgressBar(
    _In_ ULONG SubPercentTimes100)
{
}

VOID
NoUiDrawProgressBarCenter(
    _In_ PCSTR ProgressText)
{
}

VOID
NoUiDrawProgressBar(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR ProgressText)
{
}


BOOLEAN NoUiEditBox(PCSTR MessageText, PCHAR EditTextBuffer, ULONG Length)
{
    return FALSE;
}

UCHAR NoUiTextToColor(PCSTR ColorText)
{
    return 0;
}

UCHAR NoUiTextToFillStyle(PCSTR FillStyleText)
{
    return 0;
}

VOID NoUiFadeInBackdrop(VOID)
{
}

VOID NoUiFadeOut(VOID)
{
}

///////////////////////////////////////////////////////////////////////////////////////
//
// Menu Functions
//
///////////////////////////////////////////////////////////////////////////////////////

BOOLEAN
NoUiDisplayMenu(
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
    *SelectedMenuItem = DefaultMenuItem;
    return TRUE;
}

VOID
NoUiDrawMenu(
    _In_ PUI_MENU_INFO MenuInfo)
{
}
