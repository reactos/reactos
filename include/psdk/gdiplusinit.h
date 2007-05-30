/*
 * GdiPlusInit.h
 *
 * Windows GDI+
 *
 * This file is part of the w32api package.
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _GDIPLUSINIT_H
#define _GDIPLUSINIT_H

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

enum DebugEventLevel {
  DebugEventLevelFatal,
  DebugEventLevelWarning
};

typedef VOID (WINAPI *DebugEventProc)(DebugEventLevel level, CHAR *message);

struct GdiplusStartupInput
{
  UINT32 GdiplusVersion;
  DebugEventProc DebugEventCallback;
  BOOL SuppressBackgroundThread;
  BOOL SuppressExternalCodecs;

  GdiplusStartupInput(
    DebugEventProc debugEventCallback = NULL,
    BOOL suppressBackgroundThread = FALSE,
    BOOL suppressExternalCodecs = FALSE)
  {
    GdiplusVersion = 1;
    DebugEventCallback = debugEventCallback;
    SuppressBackgroundThread = suppressBackgroundThread;
    SuppressExternalCodecs = suppressExternalCodecs;
  }
};

typedef Status (WINAPI *NotificationHookProc)(OUT ULONG_PTR *token);
typedef VOID (WINAPI *NotificationUnhookProc)(ULONG_PTR token);

struct GdiplusStartupOutput {
  NotificationHookProc NotificationHook;
  NotificationUnhookProc NotificationUnhook;
};

extern "C" WINAPI Status GdiplusStartup(ULONG_PTR *token, const GdiplusStartupInput *input, GdiplusStartupOutput *output);
extern "C" WINAPI VOID GdiplusShutdown(ULONG_PTR token);

#endif /* _GDIPLUSINIT_H */
