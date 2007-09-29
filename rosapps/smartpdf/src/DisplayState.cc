/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
#include "DisplayState.h"
#include "str_util.h"
#include "dstring.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void normalizeRotation(int *rotation)
{
    assert(rotation);
    if (!rotation) return;
    while (*rotation < 0)
        *rotation += 360;
    while (*rotation >= 360)
        *rotation -= 360;
}

BOOL validRotation(int rotation)
{
    normalizeRotation(&rotation);
    if ((0 == rotation) || (90 == rotation) ||
        (180 == rotation) || (270 == rotation))
        return TRUE;
    return FALSE;
}

BOOL ValidZoomVirtual(double zoomVirtual)
{
    if ((ZOOM_FIT_PAGE == zoomVirtual) || (ZOOM_FIT_WIDTH == zoomVirtual))
        return TRUE;
    if ((zoomVirtual < ZOOM_MIN) || (zoomVirtual > ZOOM_MAX)) {
        DBG_OUT("ValidZoomVirtual() invalid zoom: %.4f\n", zoomVirtual);
        return FALSE;
    }
    return TRUE;
}

#define STR_FROM_ENUM(val) \
    if (val == var) \
        return val##_STR;

const char *DisplayModeNameFromEnum(DisplayMode var)
{
    STR_FROM_ENUM(DM_SINGLE_PAGE)
    STR_FROM_ENUM(DM_FACING)
    STR_FROM_ENUM(DM_CONTINUOUS)
    STR_FROM_ENUM(DM_CONTINUOUS_FACING)
    return NULL;
}

#define IS_STR_ENUM(enumName) \
    if (str_eq(txt, enumName##_STR)) { \
        *resOut = enumName; \
        return TRUE; \
    }

BOOL DisplayModeEnumFromName(const char *txt, DisplayMode *resOut)
{
    IS_STR_ENUM(DM_SINGLE_PAGE)
    IS_STR_ENUM(DM_FACING)
    IS_STR_ENUM(DM_CONTINUOUS)
    IS_STR_ENUM(DM_CONTINUOUS_FACING)
    assert(0);
    return FALSE;
}

void DisplayState_Init(DisplayState *ds)
{
    memzero(ds, sizeof(DisplayState));
    ds->displayMode = DM_SINGLE_PAGE;
    ds->visible = FALSE;
    ds->fullScreen = FALSE;
    ds->pageNo = 1;
    ds->zoomVirtual = 100.0;
    ds->rotation = 0;
}

void DisplayState_Free(DisplayState *ds)
{
    free((void*)ds->filePath);
    DisplayState_Init(ds);
}

BOOL DisplayState_Serialize(DisplayState *ds, DString *strOut)
{
    const char *        displayModeName = NULL;

    DStringSprintf(strOut, "  %s: %s\n", FILE_STR, ds->filePath);

    displayModeName = DisplayModeNameFromEnum(ds->displayMode);
    if (displayModeName)
        DStringSprintf(strOut, "  %s: %s\n", DISPLAY_MODE_STR, displayModeName);
    else
        DStringSprintf(strOut, "  %s: %s\n", DISPLAY_MODE_STR, DisplayModeNameFromEnum(DM_SINGLE_PAGE));

    DStringSprintf(strOut, "  %s: %d\n",   VISIBLE_STR, ds->visible);
    DStringSprintf(strOut, "  %s: %d\n",   PAGE_NO_STR, ds->pageNo);
    DStringSprintf(strOut, "  %s: %.4f\n", ZOOM_VIRTUAL_STR, ds->zoomVirtual);
    DStringSprintf(strOut, "  %s: %d\n",   ROTATION_STR, ds->rotation);
    DStringSprintf(strOut, "  %s: %d\n",   FULLSCREEN_STR, (int)ds->fullScreen);

    DStringSprintf(strOut, "  %s: %d\n",   SCROLL_X_STR, ds->scrollX);
    DStringSprintf(strOut, "  %s: %d\n",   SCROLL_Y_STR, ds->scrollY);

    DStringSprintf(strOut, "  %s: %d\n",   WINDOW_X_STR, ds->windowX);
    DStringSprintf(strOut, "  %s: %d\n",   WINDOW_Y_STR, ds->windowY);
    DStringSprintf(strOut, "  %s: %d\n",   WINDOW_DX_STR, ds->windowDx);
    DStringSprintf(strOut, "  %s: %d\n",   WINDOW_DY_STR, ds->windowDy);
    return TRUE;
}

