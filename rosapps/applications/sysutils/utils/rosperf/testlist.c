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

#include <windows.h>
#include "rosperf.h"

static TEST TestList[] =
  {
    { L"fill", L"Fill", NullInit, FillProc, NullCleanup, NullCleanup },
    { L"smallfill", L"Small Fill", NullInit, FillSmallProc, NullCleanup, NullCleanup },
    { L"hlines", L"Horizontal Lines", NullInit, LinesHorizontalProc, NullCleanup, NullCleanup },
    { L"vlines", L"Vertical Lines", NullInit, LinesVerticalProc, NullCleanup, NullCleanup },
    { L"lines", L"Lines", NullInit, LinesProc, NullCleanup, NullCleanup },
    { L"scroll", L"Scroll", NullInit, ScrollProc, NullCleanup, NullCleanup },
    { L"text", L"Text", NullInit, TextProc, NullCleanup, NullCleanup },
    { L"alpha", L"Alpha Blend", AlphaBlendInit, AlphaBlendProc, NullCleanup, AlphaBlendCleanup },
    { L"hgradient", L"Horizontal Gradient", NullInit, GradientHorizontalProc, NullCleanup, NullCleanup },
    { L"vgradient", L"Vertical Gradient", NullInit, GradientVerticalProc, NullCleanup, NullCleanup },
    { L"gradient", L"Gradient", NullInit, GradientProc, NullCleanup, NullCleanup }
  };


void
GetTests(unsigned *TestCount, PTEST *Tests)
  {
    *TestCount = sizeof(TestList) / sizeof(TEST);
    *Tests = TestList;
  }

/* EOF */
