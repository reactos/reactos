/*
 * PROJECT:     ReactOS Desktop Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/desk/preview.h
 * PURPOSE:     Definitions used by the preview control
 * COPYRIGHT:   Copyright 2006, 2007 Eric Kohl
 */

#pragma once

#define IDX_DESKTOP           0
#define IDX_INACTIVE_CAPTION  1
#define IDX_INACTIVE_BORDER   2
#define IDX_ACTIVE_CAPTION    3
#define IDX_ACTIVE_BORDER     4
#define IDX_MENU              5
#define IDX_SELECTION         6
#define IDX_WINDOW            7
#define IDX_SCROLLBAR         8
#define IDX_3D_OBJECTS        9
#define IDX_CAPTION_BUTTON   12
#define IDX_DIALOG           14
#define IDX_APPSPACE         16
#define IDX_QUICKINFO        20

#define PVM_SETSIZE          (WM_USER+1)
#define PVM_GETSIZE          (WM_USER+2)
#define PVM_SETFONT          (WM_USER+3)
#define PVM_GETFONT          (WM_USER+4)
#define PVM_SETCOLOR         (WM_USER+5)
#define PVM_GETCOLOR         (WM_USER+6)
#define PVM_SET_HDC_PREVIEW  (WM_USER+7)
#define PVM_UPDATETHEME      (WM_USER+8)

BOOL RegisterPreviewControl(IN HINSTANCE hInstance);
VOID UnregisterPreviewControl(IN HINSTANCE hInstance);
