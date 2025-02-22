/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         MenuOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/lineinput.h
 * PURPOSE:         Console line input functions
 * PROGRAMMERS:     Jeffrey Morlan
 */

#pragma once

VOID
LineInputKeyDown(PCONSRV_CONSOLE Console,
                 PUNICODE_STRING ExeName,
                 KEY_EVENT_RECORD *KeyEvent);
