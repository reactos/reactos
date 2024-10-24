/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Boot Theme & Animation header
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

//
// Positions of areas and images
//

#if SOS_UI == SOS_UI_NONE
#define VID_SCROLL_AREA_LEFT        0
#define VID_SCROLL_AREA_TOP         0
#define VID_SCROLL_AREA_RIGHT     SCREEN_WIDTH - 1
#define VID_SCROLL_AREA_BOTTOM    SCREEN_HEIGHT
#else
#define VID_SCROLL_AREA_LEFT       32
#define VID_SCROLL_AREA_TOP        80
#define VID_SCROLL_AREA_RIGHT     631
#define VID_SCROLL_AREA_BOTTOM    400
#endif

#define VID_PROGRESS_BAR_LEFT     259
#define VID_PROGRESS_BAR_TOP      352
#define VID_PROGRESS_BAR_WIDTH    121
#define VID_PROGRESS_BAR_HEIGHT   12

/* 16px space between shutdown logo and message */
#define VID_SHUTDOWN_LOGO_LEFT    225
#define VID_SHUTDOWN_LOGO_TOP     114
#define VID_SHUTDOWN_MSG_LEFT     213
#define VID_SHUTDOWN_MSG_TOP      354

#define VID_SKU_AREA_LEFT         418
#define VID_SKU_AREA_TOP          230
#define VID_SKU_AREA_RIGHT        454
#define VID_SKU_AREA_BOTTOM       256

#define VID_SKU_SAVE_AREA_LEFT    413
#define VID_SKU_SAVE_AREA_TOP     237

#define VID_SKU_TEXT_LEFT         180
#define VID_SKU_TEXT_TOP          121

#define VID_FOOTER_BG_TOP        (SCREEN_HEIGHT - 59)


//
// Boot Splash-Screen Functions
//

CODE_SEG("INIT")
BOOLEAN
NTAPI
BootAnimInitialize(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ ULONG Count);

VOID
NTAPI
BootAnimTickProgressBar(
    _In_ ULONG SubPercentTimes100);

CODE_SEG("INIT")
VOID
NTAPI
InbvRotBarInit(VOID);

CODE_SEG("INIT")
VOID
NTAPI
DisplayBootBitmap(
    _In_ BOOLEAN TextMode);

CODE_SEG("INIT")
VOID
NTAPI
FinalizeBootLogo(VOID);

VOID
NTAPI
DisplayShutdownBitmap(VOID);

VOID
NTAPI
DisplayShutdownText(VOID);
