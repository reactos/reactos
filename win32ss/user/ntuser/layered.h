/*
 * PROJECT:          ReactOS Win32k subsystem
 * LICENSE:          GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:          Layered window support
 * COPYRIGHT:        Copyright 2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#pragma once

BOOL
FASTCALL
IntLayeredGetState(
   _In_ PWND pWnd);

BOOL
FASTCALL
IntLayeredSetState(
   _In_ PWND pWnd,
   _In_ BOOL Set);

BOOL
FASTCALL
IntIsLayeredWindow(
   _In_ PWND pWnd);

VOID
FASTCALL
IntLayeredFreeWindow(
   _In_ PWND pWnd);

VOID
FASTCALL
IntLayeredPreUpdate(
   _In_ PWND pWnd);

VOID
FASTCALL
IntLayeredPostUpdate(
   _In_ PWND pWnd);

VOID
FASTCALL
IntLayeredRefresh(
   _In_ PWND pWnd,
   _In_opt_ const RECTL *prcDirty);

BOOL
FASTCALL
IntUpdateLayeredWindowI(
   _In_ PWND pWnd,
   _Inout_ UPDATELAYEREDWINDOWINFO *info);
