/*
 * PROJECT:     ReactOS Picture and Fax Viewer
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
 * PURPOSE:     Animation of shimgvw.dll
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shimgvw.h"

#define ANIME_TIMER_ID  9999

void Anime_FreeInfo(PANIME pAnime)
{
    if (pAnime->m_pDelayItem)
    {
        free(pAnime->m_pDelayItem);
        pAnime->m_pDelayItem = NULL;
    }
    pAnime->m_nFrameIndex = 0;
    pAnime->m_nFrameCount = 0;
    pAnime->m_nLoopIndex = 0;
    pAnime->m_nLoopCount = (UINT)-1;
}

void Anime_SetTimerWnd(PANIME pAnime, HWND hwndTimer)
{
    pAnime->m_hwndTimer = hwndTimer;
}

void Anime_Pause(PANIME pAnime)
{
    KillTimer(pAnime->m_hwndTimer, ANIME_TIMER_ID);
}

void Anime_Start(PANIME pAnime, DWORD dwDelay)
{
    if (pAnime->m_pDelayItem)
        SetTimer(pAnime->m_hwndTimer, ANIME_TIMER_ID, dwDelay, NULL);
}

BOOL Anime_OnTimer(PANIME pAnime, WPARAM wParam)
{
    DWORD dwDelay;

    if (wParam != ANIME_TIMER_ID)
        return FALSE;

    Anime_Pause(pAnime);
    if (Anime_Step(pAnime, &dwDelay))
        Anime_Start(pAnime, dwDelay);
    return TRUE;
}

BOOL Anime_LoadInfo(PANIME pAnime)
{
    UINT nDimCount = 0;
    UINT cbItem;
    UINT result;

    Anime_Pause(pAnime);
    Anime_FreeInfo(pAnime);

    if (!g_pImage)
        return FALSE;

    GdipImageGetFrameDimensionsCount(g_pImage, &nDimCount);
    if (nDimCount)
    {
        GUID *dims = (GUID *)calloc(nDimCount, sizeof(GUID));
        if (dims)
        {
            GdipImageGetFrameDimensionsList(g_pImage, dims, nDimCount);
            GdipImageGetFrameCount(g_pImage, dims, &result);
            pAnime->m_nFrameCount = result;
            free(dims);
        }
    }

    result = 0;
    GdipGetPropertyItemSize(g_pImage, PropertyTagFrameDelay, &result);
    cbItem = result;
    if (cbItem)
    {
        pAnime->m_pDelayItem = (PropertyItem *)malloc(cbItem);
        GdipGetPropertyItem(g_pImage, PropertyTagFrameDelay, cbItem, pAnime->m_pDelayItem);
    }

    result = 0;
    GdipGetPropertyItemSize(g_pImage, PropertyTagLoopCount, &result);
    cbItem = result;
    if (cbItem)
    {
        PropertyItem *pItem = (PropertyItem *)malloc(cbItem);
        if (pItem)
        {
            if (GdipGetPropertyItem(g_pImage, PropertyTagLoopCount, cbItem, pItem) == Ok)
            {
                pAnime->m_nLoopCount = *(WORD *)pItem->value;
            }
            free(pItem);
        }
    }

    Anime_Start(pAnime, 0);

    return pAnime->m_pDelayItem != NULL;
}

void Anime_SetFrameIndex(PANIME pAnime, UINT nFrameIndex)
{
    if (nFrameIndex < pAnime->m_nFrameCount)
    {
        GUID guid = FrameDimensionTime;
        if (Ok != GdipImageSelectActiveFrame(g_pImage, &guid, nFrameIndex))
        {
            guid = FrameDimensionPage;
            GdipImageSelectActiveFrame(g_pImage, &guid, nFrameIndex);
        }
    }
    pAnime->m_nFrameIndex = nFrameIndex;
}

DWORD Anime_GetFrameDelay(PANIME pAnime, UINT nFrameIndex)
{
    if (nFrameIndex < pAnime->m_nFrameCount && pAnime->m_pDelayItem)
    {
        return ((DWORD *)pAnime->m_pDelayItem->value)[pAnime->m_nFrameIndex] * 10;
    }
    return 0;
}

BOOL Anime_Step(PANIME pAnime, DWORD *pdwDelay)
{
    *pdwDelay = INFINITE;
    if (pAnime->m_nLoopCount == (UINT)-1)
        return FALSE;

    if (pAnime->m_nFrameIndex + 1 < pAnime->m_nFrameCount)
    {
        *pdwDelay = Anime_GetFrameDelay(pAnime, pAnime->m_nFrameIndex);
        Anime_SetFrameIndex(pAnime, pAnime->m_nFrameIndex);
        ++pAnime->m_nFrameIndex;
        return TRUE;
    }

    if (pAnime->m_nLoopCount == 0 || pAnime->m_nLoopIndex < pAnime->m_nLoopCount)
    {
        *pdwDelay = Anime_GetFrameDelay(pAnime, pAnime->m_nFrameIndex);
        Anime_SetFrameIndex(pAnime, pAnime->m_nFrameIndex);
        pAnime->m_nFrameIndex = 0;
        ++pAnime->m_nLoopIndex;
        return TRUE;
    }

    return FALSE;
}
