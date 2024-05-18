/*
 *  Copyright 2003 J Brown
 *  Copyright 2006 Eric Kohl
 *  Copyright 2007 Marc Piulachs (marc.piulachs@codexchange.net)
 *  Copyright 2015 Daniel Reimer
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <windows.h>
#include <tchar.h>
#include <scrnsave.h>
#include "resource.h"

#define RANDOM( min, max ) ((rand() % (int)(((max)+1) - (min))) + (min))

#define MAX_LOADSTRING 100
#define MAX_STARS 1000

#define APPNAME            _T("Starfield")
#define APP_TIMER          1
#define APP_TIMER_INTERVAL 20

#define MAX_STARS 1000

// Details of each individual star
typedef struct star
{
    int m_nXPos, m_nYPos, m_nZPos;
    int m_nOldX, m_nOldY;
} STAR;

STAR *stars;

int m_nTotStars;
int m_nCenterX, m_nCenterY;

void DrawStarField (HDC pDC)
{
    int nX, nY;
    int i;
    for (i = 0; i < m_nTotStars; i++)
    {
        // Clear last position of this star
        SetPixel (
            pDC,
            stars[i].m_nOldX,
            stars[i].m_nOldY,
            RGB (0, 0, 0));

        nX = (int)((((long)stars[i].m_nXPos << 7) / (long)stars[i].m_nZPos) + m_nCenterX);
        nY = (int)((((long)stars[i].m_nYPos << 7) / (long)stars[i].m_nZPos) + m_nCenterY);

        // Draw star
        SetPixel (
            pDC,
            nX,
            nY,
            RGB (255, 255, 255));

        // Remember current position for clearing later
        stars[i].m_nOldX = nX;
        stars[i].m_nOldY = nY;
    }
}

BOOL SetUpStars (int nNumStars)
{
    int i;
    if (nNumStars > MAX_STARS)
    {
        MessageBox (0,
            _T("Too many stars! Aborting!"),
            _T("Error"),
            MB_OK | MB_ICONWARNING);
        return FALSE;
    }

    if (stars)
        free (stars);

    m_nTotStars = nNumStars;

    stars = (STAR*)malloc(nNumStars * sizeof(STAR));

    if (!stars)
    {
        MessageBox (0,
            _T("Unable to allocate memory! Aborting!"),
            _T("Error"),
            MB_OK | MB_ICONWARNING);
        return FALSE;
    }

    for (i = 0; i < m_nTotStars; i++)
    {
        do
        {
            stars[i].m_nXPos = RANDOM (-320, 320);
            stars[i].m_nYPos = RANDOM (-200, 200);
            stars[i].m_nZPos = i+1;
            stars[i].m_nOldX = -1;
            stars[i].m_nOldY = -1;
        } while ((stars[i].m_nXPos == 0) || (stars[i].m_nYPos == 0));
    }
    return TRUE;
}

void MoveStarField (int nXofs, int nYofs, int nZofs)
{
    int i;
    for (i = 0; i < m_nTotStars; i++)
    {
        stars[i].m_nXPos += nXofs;
        stars[i].m_nYPos += nYofs;
        stars[i].m_nZPos += nZofs;

        if (stars[i].m_nZPos > m_nTotStars)
            stars[i].m_nZPos -= m_nTotStars;
        if (stars[i].m_nZPos < 1)
            stars[i].m_nZPos += m_nTotStars;
    }
}

void SetDimensions (int nWidth, int nHeight)
{
    m_nCenterX = nWidth / 2;
    m_nCenterY = nHeight / 2;
}

LRESULT WINAPI ScreenSaverProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HDC pDC;

    switch (msg)
    {
        case WM_CREATE:
        {
            SetTimer (
                hwnd,
                APP_TIMER,
                APP_TIMER_INTERVAL,
                NULL);
        }
        break;
        case WM_PAINT:
        {
            PAINTSTRUCT    PtStr;
            HDC pDC = BeginPaint (hwnd, &PtStr);
            DrawStarField (pDC);
            EndPaint (hwnd, &PtStr);
            SetUpStars(250);
            return (0);
        }
        break;
        case WM_TIMER:
        {
            if (wParam == APP_TIMER)
            {
                MoveStarField (0, 0, -3);
                pDC = GetDC(hwnd);
                DrawStarField (pDC);
                ReleaseDC(hwnd, pDC);
            }
        }
        break;
        case WM_SIZE:
        {
            // Change the center point of the starfield
            SetDimensions (
                LOWORD(lParam),
                HIWORD(lParam));
        }
        break;
        case WM_DESTROY:
        {
            KillTimer (hwnd, APP_TIMER);
            free(stars);
            ShowCursor(TRUE);
            PostQuitMessage (0);
            return 0;
        }
        break;
        default:
            return DefScreenSaverProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hmodule)
{
    TCHAR szTitle[256];
    TCHAR szText[256];

    LoadString(hmodule,
            IDS_TITLE,
            szTitle,
            256);

    LoadString(hmodule,
            IDS_TEXT,
            szText,
            256);

    MessageBox(0,
            szText,
            szTitle,
            MB_OK | MB_ICONWARNING);
    return FALSE;
}
