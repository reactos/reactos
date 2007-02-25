/*
 * PROJECT:     ReactOS Desktop Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/cpl/desk/preview.h
 * PURPOSE:     Definitions used by the preview control
 * COPYRIGHT:   Copyright 2006, 2007 Eric Kohl
 */

#define IDX_3D_OBJECTS        0
#define IDX_SCROLLBAR         1
#define IDX_DESKTOP           2
#define IDX_DIALOG            3
#define IDX_WINDOW            4
#define IDX_APPSPACE          5
#define IDX_SELECTION         6
#define IDX_MENU              7

#define IDX_QUICKINFO         9
#define IDX_INACTIVE_BORDER  10
#define IDX_ACTIVE_BORDER    11

#define IDX_INACTIVE_CAPTION 15
#define IDX_ACTIVE_CAPTION   16
#define IDX_CAPTION_BUTTON   17


#define PVM_GETCYCAPTION     (WM_USER+1)
#define PVM_SETCYCAPTION     (WM_USER+2)

#define PVM_GETCYMENU        (WM_USER+3)
#define PVM_SETCYMENU        (WM_USER+4)

#define PVM_GETCXSCROLLBAR   (WM_USER+5)
#define PVM_SETCXSCROLLBAR   (WM_USER+6)

#define PVM_GETCYSIZEFRAME   (WM_USER+7)
#define PVM_SETCYSIZEFRAME   (WM_USER+8)

BOOL RegisterPreviewControl(IN HINSTANCE hInstance);
VOID UnregisterPreviewControl(IN HINSTANCE hInstance);
