/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/lineinput.c
 * PURPOSE:         Console line input functions
 * PROGRAMMERS:     Jeffrey Morlan
 */

#pragma once

VOID HistoryDeleteBuffers(PCONSOLE Console);
VOID LineInputKeyDown(PCONSOLE Console, KEY_EVENT_RECORD *KeyEvent);
