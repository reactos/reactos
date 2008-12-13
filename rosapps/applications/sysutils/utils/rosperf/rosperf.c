/*
 *  ReactOS RosPerf - ReactOS GUI performance test program
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Ideas copied from x11perf:
 *
 * Copyright 1988, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.
 *
 *                         All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Digital not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <reactos/buildno.h>

#include "rosperf.h"

#define MAINWND_WIDTH   400
#define MAINWND_HEIGHT  400

static HWND LabelWnd;

unsigned
NullInit(void **Context, PPERF_INFO PerfInfo, unsigned Reps)
{
  *Context = NULL;

  return Reps;
}

void
NullCleanup(void *Context, PPERF_INFO PerfInfo)
{
}

static void
ProcessMessages(void)
{
  MSG Msg;

  while (PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE))
    {
      if (WM_QUIT == Msg.message)
        {
          exit(Msg.wParam);
        }
      TranslateMessage(&Msg);
      DispatchMessageW(&Msg);
    }
}

static void
ClearWindow(PPERF_INFO PerfInfo)
{
  InvalidateRect(PerfInfo->Wnd, NULL, TRUE);
  UpdateWindow(PerfInfo->Wnd);
}

static unsigned
CalibrateTest(PTEST Test, PPERF_INFO PerfInfo)
{
#define GOAL    2500   /* Try to get up to 2.5 seconds                 */
#define ENOUGH  2000   /* But settle for 2.0 seconds                   */
#define TICK      10   /* Assume clock not faster than .01 seconds     */

  unsigned Reps, DidReps;        /* Reps desired, reps performed                 */
  unsigned Exponent;
  void *Context;
  DWORD StartTick;
  DWORD Duration;

  /* Attempt to get an idea how long each rep lasts by getting enough
     reps to last more than ENOUGH.  Then scale that up to the number of
     seconds desired.

     If init call to test ever fails, return False and test will be skipped.
  */

  Reps = 1;
  for (;;)
    {
      ClearWindow(PerfInfo);
      DidReps = (*Test->Init)(&Context, PerfInfo, Reps);
      ProcessMessages();
      if (0 == DidReps)
        {
          return 0;
        }
      StartTick = GetTickCount();
      (*Test->Proc)(Context, PerfInfo, Reps);
      Duration = GetTickCount() - StartTick;
      (*Test->PassCleanup) (Context, PerfInfo);
      (*Test->Cleanup)(Context, PerfInfo);
      ProcessMessages();

      if (DidReps != Reps)
        {
          /* The test can't do the number of reps as we asked for.
             Give up */
          return DidReps;
        }
      /* Did we go long enough? */
      if (ENOUGH <= Duration)
        {
          break;
        }

      /* Don't let too short a clock make new reps wildly high */
      if (Duration <= TICK)
        {
          Reps *= 10;
        }
      else
        {
          /* Try to get up to GOAL seconds. */
          Reps = (int)(GOAL * (double) Reps / (double) Duration) + 1;
        }
    }

  Reps = (int) ((double) PerfInfo->Seconds * 1000.0 * (double) Reps / (double) Duration) + 1;

  /* Now round reps up to 1 digit accuracy, so we don't get stupid-looking
     numbers of repetitions. */
  Reps--;
  Exponent = 1;
  while (9 < Reps)
    {
      Reps /= 10;
      Exponent *= 10;
    }
  Reps = (Reps + 1) * Exponent;

  return Reps;
}

static void
DisplayStatus(HWND Label, LPCWSTR Message, LPCWSTR Test, int Try)
{
  WCHAR Status[128];

  _snwprintf(Status, sizeof(Status) / sizeof(Status[0]), L"%d %s %s", Try, Message, Test);
  SetWindowTextW(Label, Status);
  InvalidateRect(Label, NULL, TRUE);
  UpdateWindow(Label);
}

static double
RoundTo3Digits(double d)
{
  /* It's kind of silly to print out things like ``193658.4/sec'' so just
     junk all but 3 most significant digits. */

  double exponent, sign;

  exponent = 1.0;
  /* the code below won't work if d should happen to be non-positive. */
  if (d < 0.0)
    {
      d = -d;
      sign = -1.0;
    }
  else
    {
      sign = 1.0;
    }

  if (1000.0 <= d)
    {
      do
        {
          exponent *= 10.0;
        }
      while (1000.0 <= d / exponent);
      d = (double)((int)(d / exponent + 0.5));
      d *= exponent;
    }
  else
    {
      if (0.0 != d)
        {
          while (d * exponent < 100.0)
            {
              exponent *= 10.0;
            }
        }
      d = (double)((int)(d * exponent + 0.5));
      d /= exponent;
    }

  return d * sign;
}

static void
ReportTimes(DWORD Time, int Reps, LPCWSTR Label, BOOL Average)
{
  double MSecsPerObj, ObjsPerSec;

  if (0 != Time)
    {
      MSecsPerObj = (double) Time / (double) Reps;
      ObjsPerSec = (double) Reps * 1000.0 / (double) Time;

      /* Round obj/sec to 3 significant digits.  Leave msec untouched, to
         allow averaging results from several repetitions. */
      ObjsPerSec =  RoundTo3Digits(ObjsPerSec);

      wprintf(L"%7d %s @ %8.4f msec (%8.1f/sec): %s\n",
              Reps, Average ? L"trep" : L"reps", MSecsPerObj, ObjsPerSec, Label);
    }
  else
    {
      wprintf(L"%6d %sreps @ 0.0 msec (unmeasurably fast): %s\n",
              Reps, Average ? L"t" : L"", Label);
    }

}

static void
ProcessTest(PTEST Test, PPERF_INFO PerfInfo)
{
  unsigned Reps;
  unsigned Repeat;
  void *Context;
  DWORD StartTick;
  DWORD Time, TotalTime;

  DisplayStatus(LabelWnd, L"Calibrating", Test->Label, 0);
  Reps = CalibrateTest(Test, PerfInfo);
  if (0 == Reps)
    {
      return;
    }

  Reps = Test->Init(&Context, PerfInfo, Reps);
  if (0 == Reps)
    {
      return;
    }
  TotalTime = 0;
  for (Repeat = 0; Repeat < PerfInfo->Repeats; Repeat++)
    {
      DisplayStatus(LabelWnd, L"Testing", Test->Label, Repeat + 1);
      ClearWindow(PerfInfo);
      StartTick = GetTickCount();
      (*Test->Proc)(Context, PerfInfo, Reps);
      Time = GetTickCount() - StartTick;
      ProcessMessages();
      TotalTime += Time;
      ReportTimes(Time, Reps, Test->Label, FALSE);
      (*Test->PassCleanup)(Context, PerfInfo);
      ProcessMessages();
    }
  (*Test->Cleanup)(Context, PerfInfo);
  ReportTimes(TotalTime, Repeat * Reps, Test->Label, TRUE);
  ProcessMessages();
}

static void
PrintOSVersion(void)
{
#define BUFSIZE 160
  OSVERSIONINFOEXW VersionInfo;
  BOOL OsVersionInfoEx;
  HKEY hKey;
  WCHAR ProductType[BUFSIZE];
  DWORD BufLen;
  LONG Ret;
  unsigned RosVersionLen;
  LPWSTR RosVersion;

  /* Try calling GetVersionEx using the OSVERSIONINFOEX structure.
   * If that fails, try using the OSVERSIONINFO structure. */

  ZeroMemory(&VersionInfo, sizeof(OSVERSIONINFOEXW));
  VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

  OsVersionInfoEx = GetVersionExW((OSVERSIONINFOW *) &VersionInfo);
  if (! OsVersionInfoEx)
    {
      VersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
      if (! GetVersionExW((OSVERSIONINFOW *) &VersionInfo))
        {
          return;
        }
    }

  RosVersion = VersionInfo.szCSDVersion + wcslen(VersionInfo.szCSDVersion) + 1;
  RosVersionLen = sizeof(VersionInfo.szCSDVersion) / sizeof(VersionInfo.szCSDVersion[0]) -
                  (RosVersion - VersionInfo.szCSDVersion);
  if (7 <= RosVersionLen && 0 == _wcsnicmp(RosVersion, L"ReactOS", 7))
    {
      wprintf(L"Running on %s\n", RosVersion);
      return;
    }

  switch (VersionInfo.dwPlatformId)
    {
      /* Test for the Windows NT product family. */
      case VER_PLATFORM_WIN32_NT:

        /* Test for the specific product. */
        if (5 == VersionInfo.dwMajorVersion && 2 == VersionInfo.dwMinorVersion)
          {
            wprintf(L"Running on Microsoft Windows Server 2003, ");
          }
        else if (5 == VersionInfo.dwMajorVersion && 1 == VersionInfo.dwMinorVersion)
          {
            wprintf(L"Running on Microsoft Windows XP ");
          }
        else if (5 == VersionInfo.dwMajorVersion && 0 == VersionInfo.dwMinorVersion)
          {
            wprintf(L"Running on Microsoft Windows 2000 ");
          }
        else if (VersionInfo.dwMajorVersion <= 4 )
          {
            wprintf(L"Running on Microsoft Windows NT ");
          }

        /* Test for specific product on Windows NT 4.0 SP6 and later. */
        if (OsVersionInfoEx)
          {
            /* Test for the workstation type. */
            if (VER_NT_WORKSTATION == VersionInfo.wProductType)
              {
                if (4 == VersionInfo.dwMajorVersion)
                  {
                    wprintf(L"Workstation 4.0 ");
                  }
                else if (0 != (VersionInfo.wSuiteMask & VER_SUITE_PERSONAL))
                  {
                    wprintf(L"Home Edition ");
                  }
                else
                  {
                    wprintf(L"Professional ");
                  }
              }

            /* Test for the server type. */
            else if (VER_NT_SERVER == VersionInfo.wProductType  ||
                     VER_NT_DOMAIN_CONTROLLER == VersionInfo.wProductType)
              {
                if (5 == VersionInfo.dwMajorVersion && 2 == VersionInfo.dwMinorVersion)
                  {
                    if (0 != (VersionInfo.wSuiteMask & VER_SUITE_DATACENTER))
                      {
                        wprintf(L"Datacenter Edition ");
                      }
                    else if (0 != (VersionInfo.wSuiteMask & VER_SUITE_ENTERPRISE))
                      {
                        wprintf(L"Enterprise Edition ");
                      }
                    else if (VER_SUITE_BLADE == VersionInfo.wSuiteMask)
                      {
                        wprintf(L"Web Edition ");
                      }
                    else
                      {
                        wprintf(L"Standard Edition ");
                      }
                  }

                else if (5 == VersionInfo.dwMajorVersion && 0 == VersionInfo.dwMinorVersion)
                  {
                    if (0 != (VersionInfo.wSuiteMask & VER_SUITE_DATACENTER))
                      {
                        wprintf(L"Datacenter Server ");
                      }
                    else if (0 != (VersionInfo.wSuiteMask & VER_SUITE_ENTERPRISE))
                      {
                        wprintf(L"Advanced Server " );
                      }
                    else
                      {
                        wprintf(L"Server " );
                      }
                  }

                else  /* Windows NT 4.0 */
                  {
                    if (0 != (VersionInfo.wSuiteMask & VER_SUITE_ENTERPRISE))
                      {
                        wprintf(L"Server 4.0, Enterprise Edition ");
                      }
                    else
                      {
                        wprintf(L"Server 4.0 ");
                      }
                  }
              }
          }
        else  /* Test for specific product on Windows NT 4.0 SP5 and earlier */
          {
            BufLen = BUFSIZE;

            Ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
                                0, KEY_QUERY_VALUE, &hKey);
            if (ERROR_SUCCESS != Ret)
              {
                return;
              }

            Ret = RegQueryValueExW(hKey, L"ProductType", NULL, NULL,
                                   (LPBYTE) ProductType, &BufLen);
            if (ERROR_SUCCESS != Ret || BUFSIZE < BufLen)
              {
                return;
              }

            RegCloseKey(hKey);

            if (0 == lstrcmpiW(L"WINNT", ProductType))
              {
                wprintf(L"Workstation ");
              }
            else if (0 == lstrcmpiW(L"LANMANNT", ProductType))
              {
                wprintf(L"Server ");
              }
            else if (0 == lstrcmpiW(L"SERVERNT", ProductType))
              {
                wprintf(L"Advanced Server ");
              }

            wprintf(L"%d.%d ", VersionInfo.dwMajorVersion, VersionInfo.dwMinorVersion);
          }

        /* Display service pack (if any) and build number. */

        if (4 == VersionInfo.dwMajorVersion &&
            0 == lstrcmpiW(VersionInfo.szCSDVersion, L"Service Pack 6"))
          {
            /* Test for SP6 versus SP6a. */
            Ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009",
                                0, KEY_QUERY_VALUE, &hKey);
            if (ERROR_SUCCESS == Ret)
              {
                wprintf(L"Service Pack 6a (Build %d)\n", VersionInfo.dwBuildNumber & 0xFFFF);
              }
            else /* Windows NT 4.0 prior to SP6a */
              {
                wprintf(L"%s (Build %d)\n",
                        VersionInfo.szCSDVersion,
                        VersionInfo.dwBuildNumber & 0xFFFF);
              }

            RegCloseKey(hKey);
          }
        else /* not Windows NT 4.0 */
          {
            wprintf(L"%s (Build %d)\n",
                    VersionInfo.szCSDVersion,
                    VersionInfo.dwBuildNumber & 0xFFFF);
          }


        break;

      /* Test for the Windows Me/98/95. A bit silly since we're using Unicode... */
      case VER_PLATFORM_WIN32_WINDOWS:

        if (4 == VersionInfo.dwMajorVersion && 0 == VersionInfo.dwMinorVersion)
          {
            wprintf(L"Running on Microsoft Windows 95 ");
            if (L'C' == VersionInfo.szCSDVersion[1] || L'B' == VersionInfo.szCSDVersion[1])
              {
                wprintf(L"OSR2");
              }
          }

        else if (4 == VersionInfo.dwMajorVersion && 10 == VersionInfo.dwMinorVersion)
          {
            wprintf(L"Running on Microsoft Windows 98 ");
            if (L'A' == VersionInfo.szCSDVersion[1])
              {
                wprintf(L"SE");
              }
          }

        else if (4 == VersionInfo.dwMajorVersion && 90 == VersionInfo.dwMinorVersion)
          {
            wprintf(L"Running on Microsoft Windows Millennium Edition");
          }
        wprintf(L"\n");
        break;

      case VER_PLATFORM_WIN32s: /* Even silier... */

        wprintf(L"Running on Microsoft Win32s\n");
        break;
    }
}

static void
PrintAppVersion(void)
{
  wprintf(L"RosPerf %S (Build %S)\n", KERNEL_VERSION_STR, KERNEL_VERSION_BUILD_STR);
}

static void
PrintDisplayInfo(void)
{
  HDC Dc;

  Dc = GetDC(NULL);
  if (NULL == Dc)
    {
      return;
    }

  wprintf(L"Display settings %d * %d * %d\n", GetDeviceCaps(Dc, HORZRES),
          GetDeviceCaps(Dc, VERTRES), GetDeviceCaps(Dc, BITSPIXEL) * GetDeviceCaps(Dc, PLANES));

  ReleaseDC(NULL, Dc);
}

static void
PrintStartupInfo(void)
{
  PrintAppVersion();
  PrintOSVersion();
  PrintDisplayInfo();
}

static LRESULT CALLBACK
MainWndProc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT Ps;
  HDC Dc;
  LRESULT Result;

  switch (Msg)
    {
      case WM_DESTROY:
        PostQuitMessage(0);
        Result = 0;
        break;

      case WM_PAINT:
        Dc = BeginPaint(Wnd, &Ps);
        EndPaint (Wnd, &Ps);
	Result = 0;
	break;

      default:
        Result = DefWindowProcW(Wnd, Msg, wParam, lParam);
        break;
    }

  return Result;
}

static LRESULT CALLBACK
LabelWndProc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT Ps;
  HDC Dc;
  RECT ClientRect, WindowRect;
  TEXTMETRICW Tm;
  LRESULT Result;
  WCHAR Title[80];

  switch (Msg)
    {
      case WM_CREATE:
        /* Make text fit */
        Dc = GetDC(Wnd);
        if (NULL != Dc && GetClientRect(Wnd, &ClientRect) && GetWindowRect(Wnd, &WindowRect)
            && GetTextMetricsW(Dc, &Tm))
          {
            if (Tm.tmHeight != ClientRect.bottom)
              {
                SetWindowPos(Wnd, NULL, 0, 0, WindowRect.right - WindowRect.left,
                             (WindowRect.bottom - WindowRect.top) + (Tm.tmHeight - ClientRect.bottom),
                             SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
              }
          }
        if (NULL != Dc)
          {
            ReleaseDC(Wnd, Dc);
          }
        Result = DefWindowProcW(Wnd, Msg, wParam, lParam);
        break;

      case WM_PAINT:
        Dc = BeginPaint(Wnd, &Ps);
        GetWindowTextW(Wnd, Title, sizeof(Title) / sizeof(Title[0]));
        TextOutW(Dc, 0, 0, Title, wcslen(Title));
        EndPaint (Wnd, &Ps);
	Result = 0;
	break;

      default:
        Result = DefWindowProcW(Wnd, Msg, wParam, lParam);
        break;
    }

  return Result;
}

static HWND
CreatePerfWindows(HINSTANCE hInstance, PPERF_INFO PerfInfo)
{
  WNDCLASSW wc;
  HWND MainWnd;

  wc.lpszClassName = L"RosPerfMain";
  wc.lpfnWndProc = MainWndProc;
  wc.style = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIconW(NULL, (LPCWSTR) IDI_APPLICATION);
  wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
  wc.hbrBackground = CreateSolidBrush(PerfInfo->BackgroundColor);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClassW(&wc) == 0)
    {
      fwprintf(stderr, L"Failed to register RosPerfMain (last error %d)\n",
	       GetLastError());
      return NULL;
    }

  wc.lpszClassName = L"RosPerfLabel";
  wc.lpfnWndProc = LabelWndProc;
  wc.style = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIconW(NULL, (LPCWSTR) IDI_APPLICATION);
  wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClassW(&wc) == 0)
    {
      fwprintf(stderr, L"Failed to register RosPerfLabel (last error %d)\n",
	       GetLastError());
      return NULL;
    }

  MainWnd = CreateWindowW(L"RosPerfMain",
                          L"ReactOS performance test",
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          0,
                          0,
                          MAINWND_WIDTH,
                          MAINWND_HEIGHT,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);
  if (NULL == MainWnd)
    {
      fwprintf(stderr, L"Failed to create main window (last error %d)\n",
	       GetLastError());
      return NULL;
    }

  LabelWnd = CreateWindowW(L"RosPerfLabel",
                           L"",
                           WS_POPUP | WS_THICKFRAME | WS_VISIBLE,
                           0,
                           MAINWND_HEIGHT + 10,
                           MAINWND_WIDTH,
                           20,
                           MainWnd,
                           NULL,
                           hInstance,
                           NULL);
  if (NULL == LabelWnd)
    {
      fwprintf(stderr, L"Failed to create label window (last error 0x%lX)\n",
	      GetLastError());
      return NULL;
    }

  SetActiveWindow(MainWnd);

  return MainWnd;
}

static BOOL
ProcessCommandLine(PPERF_INFO PerfInfo, unsigned *TestCount, PTEST *Tests)
{
  int ArgC, Arg;
  LPWSTR *ArgV;
  LPWSTR EndPtr;
  PTEST AllTests;
  BOOL *DoTest;
  BOOL DoAll;
  unsigned AllTestCount, i, j;

  ArgV = CommandLineToArgvW(GetCommandLineW(), &ArgC);
  if (NULL == ArgV)
    {
      fwprintf(stderr, L"CommandLineToArgvW failed\n");
      return FALSE;
    }

  GetTests(&AllTestCount, &AllTests);
  DoTest = malloc(AllTestCount * sizeof(BOOL));
  if (NULL == DoTest)
    {
      fwprintf(stderr, L"Out of memory\n");
      return FALSE;
    }
  DoAll = TRUE;

  for (Arg = 1; Arg < ArgC; Arg++)
    {
      if (L'/' == ArgV[Arg][0] || L'-' == ArgV[Arg][0])
        {
          if (0 == _wcsicmp(ArgV[Arg] + 1, L"repeat"))
            {
              if (ArgC <= Arg + 1)
                {
                  fwprintf(stderr, L"%s needs a repeat count\n", ArgV[Arg]);
                  free(DoTest);
                  GlobalFree(ArgV);
                  return FALSE;
                }
              Arg++;
              PerfInfo->Repeats = wcstoul(ArgV[Arg], &EndPtr, 0);
              if (L'\0' != *EndPtr || (long) PerfInfo->Repeats <= 0 || ULONG_MAX == PerfInfo->Repeats)
                {
                  fwprintf(stderr, L"Invalid repeat count %s\n", ArgV[Arg]);
                  free(DoTest);
                  GlobalFree(ArgV);
                  return FALSE;
                }
            }
          else if (0 == _wcsicmp(ArgV[Arg] + 1, L"seconds"))
            {
              if (ArgC <= Arg + 1)
                {
                  fwprintf(stderr, L"%s needs a number of seconds\n", ArgV[Arg]);
                  free(DoTest);
                  GlobalFree(ArgV);
                  return FALSE;
                }
              Arg++;
              PerfInfo->Seconds = wcstoul(ArgV[Arg], &EndPtr, 0);
              if (L'\0' != *EndPtr || (long) PerfInfo->Seconds < 0 || ULONG_MAX == PerfInfo->Seconds)
                {
                  fwprintf(stderr, L"Invalid duration %s\n", ArgV[Arg]);
                  free(DoTest);
                  GlobalFree(ArgV);
                  return FALSE;
                }
            }
          else
            {
              fwprintf(stderr, L"Unrecognized option %s\n", ArgV[Arg]);
              free(DoTest);
              GlobalFree(ArgV);
              return FALSE;
            }
        }
      else
        {
          if (DoAll)
            {
              for (i = 0; i < AllTestCount; i++)
                {
                  DoTest[i] = FALSE;
                }
              DoAll = FALSE;
            }
          for (i = 0; i < AllTestCount; i++)
            {
              if (0 == _wcsicmp(ArgV[Arg], AllTests[i].Option))
                {
                  DoTest[i] = TRUE;
                  break;
                }
            }
          if (AllTestCount <= i)
            {
              fwprintf(stderr, L"Unrecognized test %s\n", ArgV[Arg]);
              free(DoTest);
              GlobalFree(ArgV);
              return FALSE;
            }
        }
    }

  GlobalFree(ArgV);

  if (DoAll)
    {
      for (i = 0; i < AllTestCount; i++)
        {
          DoTest[i] = TRUE;
        }
    }

  *TestCount = 0;
  for (i = 0; i < AllTestCount; i++)
    {
      if (DoTest[i])
        {
          (*TestCount)++;
        }
    }
  *Tests = malloc(*TestCount * sizeof(TEST));
  if (NULL == *Tests)
    {
      fwprintf(stderr, L"Out of memory\n");
      free(DoTest);
      return FALSE;
    }
  j = 0;
  for (i = 0; i < AllTestCount; i++)
    {
      if (DoTest[i])
        {
          (*Tests)[j] = AllTests[i];
          j++;
        }
    }
  free(DoTest);

  return TRUE;
}

int WINAPI
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  PTEST Tests;
  unsigned TestCount;
  unsigned CurrentTest;
  RECT Rect;
  PERF_INFO PerfInfo;

  PrintStartupInfo();

  PerfInfo.Seconds = 15;
  PerfInfo.Repeats = 4;
  PerfInfo.ForegroundColor = RGB(0, 0, 0);
  PerfInfo.BackgroundColor = RGB(255, 255, 255);

  if (! ProcessCommandLine(&PerfInfo, &TestCount, &Tests))
    {
      exit(1);
    }

  PerfInfo.Wnd = CreatePerfWindows(hInstance, &PerfInfo);
  if (NULL == PerfInfo.Wnd)
    {
      exit(1);
    }

  GetClientRect(PerfInfo.Wnd, &Rect);
  PerfInfo.WndWidth = Rect.right - Rect.left;
  PerfInfo.WndHeight = Rect.bottom - Rect.top;
  PerfInfo.ForegroundDc = GetDC(PerfInfo.Wnd);
  PerfInfo.BackgroundDc = GetDC(PerfInfo.Wnd);
  if (NULL == PerfInfo.ForegroundDc || NULL == PerfInfo.BackgroundDc)
    {
      fwprintf(stderr, L"Failed to create device contexts (last error %d)\n",
               GetLastError());
      exit(1);
    }
  SelectObject(PerfInfo.ForegroundDc, CreateSolidBrush(PerfInfo.ForegroundColor));
  SelectObject(PerfInfo.ForegroundDc, CreatePen(PS_SOLID, 0, PerfInfo.ForegroundColor));
  SelectObject(PerfInfo.BackgroundDc, CreateSolidBrush(PerfInfo.BackgroundColor));
  SelectObject(PerfInfo.BackgroundDc, CreatePen(PS_SOLID, 0, PerfInfo.BackgroundColor));

  ProcessMessages();

  /* Move cursor out of the way */
  GetWindowRect(LabelWnd, &Rect);
  SetCursorPos(Rect.right, Rect.bottom);

  for (CurrentTest = 0; CurrentTest < TestCount; CurrentTest++)
    {
      wprintf(L"\n");
      ProcessTest(Tests + CurrentTest, &PerfInfo);
    }

  GlobalFree(Tests);

  return 0;
}

/* EOF */
