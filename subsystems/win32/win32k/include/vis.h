/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Visibility computations interface definition
 * FILE:             include/win32k/vis.h
 * PROGRAMMER:       Ge van Geldorp (ge@gse.nl)
 *
 */

#pragma once

#include <include/window.h>

HRGN FASTCALL
VIS_ComputeVisibleRegion(PWINDOW_OBJECT Window, BOOLEAN ClientArea,
   BOOLEAN ClipChildren, BOOLEAN ClipSiblings);

VOID FASTCALL
co_VIS_WindowLayoutChanged(PWINDOW_OBJECT Window, HRGN UncoveredRgn);

/* EOF */
