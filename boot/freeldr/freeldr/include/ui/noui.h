/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeLoader
 * FILE:            boot/freeldr/freeldr/include/ui/noui.h
 * PURPOSE:         No UI interface header
 * PROGRAMMERS:     Hervé Poussineau
 */

#pragma once

/* No User Interface Functions ***********************************************/

BOOLEAN NoUiInitialize(VOID);
VOID NoUiUnInitialize(VOID);

VOID NoUiDrawBackdrop(VOID);
VOID NoUiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, CHAR FillChar, UCHAR Attr);
VOID NoUiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom);
VOID NoUiDrawBox(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOLEAN Fill, BOOLEAN Shadow, UCHAR Attr);

/* Draws text at coordinates specified */
VOID
NoUiDrawText(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ PCSTR Text,
    _In_ UCHAR Attr);

/* Draws text at coordinates specified */
VOID
NoUiDrawText2(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_opt_ ULONG MaxNumChars,
    _In_reads_or_z_(MaxNumChars) PCSTR Text,
    _In_ UCHAR Attr);

/* Draws centered text at the coordinates specified and clips the edges */
VOID
NoUiDrawCenteredText(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR TextString,
    _In_ UCHAR Attr);

VOID NoUiDrawStatusText(PCSTR StatusText);
VOID NoUiUpdateDateTime(VOID);
VOID NoUiMessageBox(PCSTR MessageText);
VOID NoUiMessageBoxCritical(PCSTR MessageText);

/* Draws the progress bar showing nPos percent filled */
VOID
NoUiDrawProgressBarCenter(
    _In_ ULONG Position,
    _In_ ULONG Range,
    _Inout_z_ PSTR ProgressText);

/* Draws the progress bar showing nPos percent filled */
VOID
NoUiDrawProgressBar(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ ULONG Position,
    _In_ ULONG Range,
    _Inout_z_ PSTR ProgressText);

BOOLEAN NoUiEditBox(PCSTR MessageText, PCHAR EditTextBuffer, ULONG Length);
UCHAR NoUiTextToColor(PCSTR ColorText);
UCHAR NoUiTextToFillStyle(PCSTR FillStyleText);
VOID NoUiFadeInBackdrop(VOID);
VOID NoUiFadeOut(VOID);

/* Menu Functions ************************************************************/

BOOLEAN
NoUiDisplayMenu(
    IN PCSTR MenuHeader,
    IN PCSTR MenuFooter OPTIONAL,
    IN BOOLEAN ShowBootOptions,
    IN PCSTR MenuItemList[],
    IN ULONG MenuItemCount,
    IN ULONG DefaultMenuItem,
    IN LONG MenuTimeOut,
    OUT PULONG SelectedMenuItem,
    IN BOOLEAN CanEscape,
    IN UiMenuKeyPressFilterCallback KeyPressFilter OPTIONAL,
    IN PVOID Context OPTIONAL);

VOID
NoUiDrawMenu(
    _In_ PUI_MENU_INFO MenuInfo);
