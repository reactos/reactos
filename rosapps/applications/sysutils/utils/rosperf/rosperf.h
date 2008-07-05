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

#ifndef ROSPERF_H_INCLUDED
#define ROSPERF_H_INCLUDED

typedef struct tagPERF_INFO
{
  HWND Wnd;
  unsigned Seconds;
  unsigned Repeats;
  COLORREF ForegroundColor;
  COLORREF BackgroundColor;
  HDC ForegroundDc;
  HDC BackgroundDc;
  INT WndWidth;
  INT WndHeight;
} PERF_INFO, *PPERF_INFO;

typedef unsigned (*INITTESTPROC)(void **Context, PPERF_INFO PerfInfo, unsigned Reps);
typedef void (*TESTPROC)(void *Context, PPERF_INFO PerfInfo, unsigned Reps);
typedef void (*CLEANUPTESTPROC)(void *Context, PPERF_INFO PerfInfo);

typedef struct tagTEST
{
  LPCWSTR Option;
  LPCWSTR Label;
  INITTESTPROC Init;
  TESTPROC Proc;
  CLEANUPTESTPROC PassCleanup;
  CLEANUPTESTPROC Cleanup;
} TEST, *PTEST;

void GetTests(unsigned *TestCount, PTEST *Tests);

/* Tests */
unsigned NullInit(void **Context, PPERF_INFO PerfInfo, unsigned Reps);
void NullCleanup(void *Context, PPERF_INFO PerfInfo);

void FillProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps);
void FillSmallProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps);

void LinesHorizontalProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps);
void LinesVerticalProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps);
void LinesProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps);

void TextProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps);

unsigned AlphaBlendInit(void **Context, PPERF_INFO PerfInfo, unsigned Reps);
void AlphaBlendCleanup(void *Context, PPERF_INFO PerfInfo);
void AlphaBlendProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps);

void GradientHorizontalProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps);
void GradientVerticalProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps);
void GradientProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps);

#endif /* ROSPERF_H_INCLUDED */

/* EOF */
