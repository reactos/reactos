/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeLoader
 * FILE:            boot/freeldr/freeldr/ui/noui.c
 * PURPOSE:         No Text UI interface
 * PROGRAMMERS:     Hervé Poussineau
 */
#ifndef _M_ARM
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

VOID NoUiDrawText(ULONG X, ULONG Y, PCSTR Text, UCHAR Attr)
{
}

VOID NoUiDrawText2(ULONG X, ULONG Y, ULONG MaxNumChars, PCSTR Text, UCHAR Attr)
{
}

VOID NoUiDrawCenteredText(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, PCSTR TextString, UCHAR Attr)
{
}

VOID NoUiDrawStatusText(PCSTR StatusText)
{
    printf("%s\n", StatusText);
}

VOID NoUiUpdateDateTime(VOID)
{
}

VOID NoUiMessageBox(PCSTR MessageText)
{
    // We have not yet displayed the user interface
    // We are probably still reading the .ini file
    // and have encountered an error. Just use printf()
    // and return.
    printf("%s\n", MessageText);
    printf("Press any key\n");
    MachConsGetCh();
}

VOID NoUiMessageBoxCritical(PCSTR MessageText)
{
    // We have not yet displayed the user interface
    // We are probably still reading the .ini file
    // and have encountered an error. Just use printf()
    // and return.
    printf("%s\n", MessageText);
    printf("Press any key\n");
    MachConsGetCh();
}

VOID NoUiDrawProgressBarCenter(ULONG Position, ULONG Range, PCHAR ProgressText)
{
}

VOID NoUiDrawProgressBar(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, ULONG Position, ULONG Range, PCHAR ProgressText)
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

BOOLEAN NoUiDisplayMenu(PCSTR MenuHeader, PCSTR MenuFooter, BOOLEAN ShowBootOptions, PCSTR MenuItemList[], ULONG MenuItemCount, ULONG DefaultMenuItem, LONG MenuTimeOut, ULONG* SelectedMenuItem, BOOLEAN CanEscape, UiMenuKeyPressFilterCallback KeyPressFilter)
{
    *SelectedMenuItem = DefaultMenuItem;
    return TRUE;
}

VOID NoUiDrawMenu(PUI_MENU_INFO MenuInfo)
{
}
#endif
