/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/coninput.h
 * PURPOSE:         Console Input functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

VOID FASTCALL PurgeInputBuffer(PCONSOLE Console);

VOID NTAPI
ConDrvProcessKey(IN PCONSOLE Console,
                 IN BOOLEAN Down,
                 IN UINT VirtualKeyCode,
                 IN UINT VirtualScanCode,
                 IN WCHAR UnicodeChar,
                 IN ULONG ShiftState,
                 IN BYTE KeyStateCtrl);

/* EOF */
