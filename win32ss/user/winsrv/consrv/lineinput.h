/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/lineinput.c
 * PURPOSE:         Console line input functions
 * PROGRAMMERS:     Jeffrey Morlan
 */

#pragma once

VOID HistoryDeleteBuffers(PCONSRV_CONSOLE Console);

VOID
LineInputKeyDown(PCONSRV_CONSOLE Console,
                 PUNICODE_STRING ExeName,
                 KEY_EVENT_RECORD *KeyEvent);
