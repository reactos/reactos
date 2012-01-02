/*
 * Copyright (C) 2007 Google (Evan Stade)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _GDIPLUSINIT_H
#define _GDIPLUSINIT_H

enum DebugEventLevel
{
    DebugEventLevelFatal,
    DebugEventLevelWarning
};

typedef VOID (WINAPI *DebugEventProc)(enum DebugEventLevel, CHAR *);
typedef Status (WINAPI *NotificationHookProc)(ULONG_PTR *);
typedef void (WINAPI *NotificationUnhookProc)(ULONG_PTR);

struct GdiplusStartupInput
{
    UINT32 GdiplusVersion;
    DebugEventProc DebugEventCallback;
    BOOL SuppressBackgroundThread;
    BOOL SuppressExternalCodecs;

#ifdef __cplusplus
    GdiplusStartupInput(DebugEventProc debugEventCallback = NULL,
        BOOL suppressBackgroundThread = FALSE,
        BOOL suppressExternalCodecs = FALSE)
    {
        GdiplusVersion = 1;
        DebugEventCallback = debugEventCallback;
        SuppressBackgroundThread = suppressBackgroundThread;
        SuppressExternalCodecs = suppressExternalCodecs;
    }
#endif
};

struct GdiplusStartupOutput
{
    NotificationHookProc NotificationHook;
    NotificationUnhookProc NotificationUnhook;
};

#ifdef __cplusplus
extern "C" {
#endif

Status WINAPI GdiplusStartup(ULONG_PTR *, const struct GdiplusStartupInput *, struct GdiplusStartupOutput *);
void WINAPI GdiplusShutdown(ULONG_PTR);

#ifdef __cplusplus
}
#endif

#endif
