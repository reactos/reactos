/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeLoader
 * FILE:            boot/freeldr/freeldr/include/ui/noui.h
 * PURPOSE:         No UI interface header
 * PROGRAMMERS:     Hervé Poussineau
 */

#pragma once

///////////////////////////////////////////////////////////////////////////////////////
//
// No User Interface Functions
//
///////////////////////////////////////////////////////////////////////////////////////

BOOLEAN NoUiInitialize(VOID);
VOID NoUiUnInitialize(VOID);

VOID NoUiDrawBackdrop(VOID);
VOID NoUiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, CHAR FillChar, UCHAR Attr);
VOID NoUiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom);
VOID NoUiDrawBox(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOLEAN Fill, BOOLEAN Shadow, UCHAR Attr);
VOID NoUiDrawText(ULONG X, ULONG Y, PCSTR Text, UCHAR Attr);
VOID NoUiDrawText2(ULONG X, ULONG Y, ULONG MaxNumChars, PCSTR Text, UCHAR Attr);
VOID NoUiDrawCenteredText(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, PCSTR TextString, UCHAR Attr);
VOID NoUiDrawStatusText(PCSTR StatusText);
VOID NoUiUpdateDateTime(VOID);
VOID NoUiMessageBox(PCSTR MessageText);
VOID NoUiMessageBoxCritical(PCSTR MessageText);
VOID NoUiDrawProgressBarCenter(ULONG Position, ULONG Range, PCHAR ProgressText);
VOID NoUiDrawProgressBar(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, ULONG Position, ULONG Range, PCHAR ProgressText);
BOOLEAN NoUiEditBox(PCSTR MessageText, PCHAR EditTextBuffer, ULONG Length);
UCHAR NoUiTextToColor(PCSTR ColorText);
UCHAR NoUiTextToFillStyle(PCSTR FillStyleText);
VOID NoUiFadeInBackdrop(VOID);
VOID NoUiFadeOut(VOID);

///////////////////////////////////////////////////////////////////////////////////////
//
// Menu Functions
//
///////////////////////////////////////////////////////////////////////////////////////

BOOLEAN NoUiDisplayMenu(PCSTR MenuHeader, PCSTR MenuFooter, BOOLEAN ShowBootOptions, PCSTR MenuItemList[], ULONG MenuItemCount, ULONG DefaultMenuItem, LONG MenuTimeOut, ULONG* SelectedMenuItem, BOOLEAN CanEscape, UiMenuKeyPressFilterCallback KeyPressFilter);
VOID NoUiDrawMenu(PUI_MENU_INFO MenuInfo);
