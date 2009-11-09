/*
 * WineMine (main.c)
 *
 * Copyright 2000 Joshua Thielen <jt85296@ltu.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <tchar.h>
#include "main.h"
#include "dialog.h"
#include "resource.h"

#ifdef DUMB_DEBUG
#include <stdio.h>
#define DEBUG(x) fprintf(stderr,x)
#else
#define DEBUG(x)
#endif

static const TCHAR szAppName[] = TEXT("WineMine");


int WINAPI _tWinMain( HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR cmdline, int cmdshow )
{
    MSG msg;
    WNDCLASS wc;
    HWND hWnd;
    HACCEL haccel;

    wc.style = 0;
    wc.lpfnWndProc = MainProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon( hInst, MAKEINTRESOURCE(IDI_WINEMINE) );
    wc.hCursor = LoadCursor( NULL, (LPCTSTR)IDI_APPLICATION );
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszMenuName = MAKEINTRESOURCE(IDM_WINEMINE);
    wc.lpszClassName = szAppName;

    if ( !RegisterClass(&wc) )
        return 1;

    hWnd = CreateWindow( szAppName, szAppName,
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInst, NULL );

    if (!hWnd)
        return 1;

    ShowWindow( hWnd, cmdshow );
    UpdateWindow( hWnd );

    haccel = LoadAccelerators( hInst, MAKEINTRESOURCE(IDA_WINEMINE) );
    SetTimer( hWnd, ID_TIMER, 1000, NULL );

    while( GetMessage(&msg, NULL, 0, 0) )
    {
        if ( !TranslateAccelerator(hWnd, haccel, &msg) )
            TranslateMessage(&msg);

        DispatchMessage(&msg);
    }

    return msg.wParam;
}

LRESULT WINAPI MainProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static BOARD board;

    switch(msg)
    {
        case WM_CREATE:
            board.hInst = ((LPCREATESTRUCT) lParam)->hInstance;
            board.hWnd = hWnd;
            InitBoard( &board );
            CreateBoard( &board );
            return 0;

        case WM_PAINT:
        {
            HDC hDC;
            HDC hMemDC;
            PAINTSTRUCT ps;

            DEBUG("WM_PAINT\n");
            hDC = BeginPaint( hWnd, &ps );
            hMemDC = CreateCompatibleDC(hDC);

            DrawBoard( hDC, hMemDC, &ps, &board );

            DeleteDC( hMemDC );
            EndPaint( hWnd, &ps );

            return 0;
        }

        case WM_MOVE:
            DEBUG("WM_MOVE\n");
            board.Pos.x = (LONG) LOWORD(lParam);
            board.Pos.y = (LONG) HIWORD(lParam);
            return 0;

        case WM_DESTROY:
            SaveBoard( &board );
            DestroyBoard( &board );
            PostQuitMessage( 0 );
            return 0;

        case WM_TIMER:
            if( board.Status == PLAYING )
            {
                board.uTime++;
                RedrawWindow( hWnd, &board.TimerRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
            }
            return 0;

        case WM_LBUTTONDOWN:
            DEBUG("WM_LBUTTONDOWN\n");

            if( wParam & MK_RBUTTON )
                msg = WM_MBUTTONDOWN;

            TestBoard( hWnd, &board, LOWORD(lParam), HIWORD(lParam), msg );
            SetCapture( hWnd );
            return 0;

        case WM_LBUTTONUP:
            DEBUG("WM_LBUTTONUP\n");

            if( wParam & MK_RBUTTON )
                msg = WM_MBUTTONUP;

            TestBoard( hWnd, &board, LOWORD(lParam), HIWORD(lParam), msg );
            ReleaseCapture();
            return 0;

        case WM_RBUTTONDOWN:
            DEBUG("WM_RBUTTONDOWN\n");

            if( wParam & MK_LBUTTON )
            {
                board.Press.x = 0;
                board.Press.y = 0;
                msg = WM_MBUTTONDOWN;
            }

            TestBoard( hWnd, &board, LOWORD(lParam), HIWORD(lParam), msg );
            return 0;

        case WM_RBUTTONUP:
            DEBUG("WM_RBUTTONUP\n");
            if( wParam & MK_LBUTTON )
                msg = WM_MBUTTONUP;
            TestBoard( hWnd, &board, LOWORD(lParam), HIWORD(lParam), msg );
            return 0;

        case WM_MBUTTONDOWN:
            DEBUG("WM_MBUTTONDOWN\n");
            TestBoard( hWnd, &board, LOWORD(lParam), HIWORD(lParam), msg );
            return 0;

        case WM_MBUTTONUP:
            DEBUG("WM_MBUTTONUP\n");
            TestBoard( hWnd, &board, LOWORD(lParam), HIWORD(lParam), msg );
            return 0;

        case WM_MOUSEMOVE:
        {
            if( (wParam & MK_LBUTTON) && (wParam & MK_RBUTTON) )
                msg = WM_MBUTTONDOWN;
            else if( wParam & MK_LBUTTON )
                msg = WM_LBUTTONDOWN;
            else
                return 0;

            TestBoard( hWnd, &board, LOWORD(lParam), HIWORD(lParam),  msg );

            return 0;
        }

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDM_NEW:
                    CreateBoard( &board );
                    return 0;

                case IDM_MARKQ:
                {
                    HMENU hMenu;

                    hMenu = GetMenu( hWnd );
                    board.bMark = !board.bMark;

                    if( board.bMark )
                        CheckMenuItem( hMenu, IDM_MARKQ, MF_CHECKED );
                    else
                        CheckMenuItem( hMenu, IDM_MARKQ, MF_UNCHECKED );

                    return 0;
                }

                case IDM_BEGINNER:
                    SetDifficulty( &board, BEGINNER );
                    CreateBoard( &board );
                    return 0;

                case IDM_ADVANCED:
                    SetDifficulty( &board, ADVANCED );
                    CreateBoard( &board );
                    return 0;

                case IDM_EXPERT:
                    SetDifficulty( &board, EXPERT );
                    CreateBoard( &board );
                    return 0;

                case IDM_CUSTOM:
                    SetDifficulty( &board, CUSTOM );
                    CreateBoard( &board );
                    return 0;

                case IDM_EXIT:
                    SendMessage( hWnd, WM_CLOSE, 0, 0);
                    return 0;

                case IDM_TIMES:
                    DialogBoxParam( board.hInst, MAKEINTRESOURCE(IDD_TIMES), hWnd, TimesDlgProc, (LPARAM) &board);
                    return 0;

                case IDM_ABOUT:
                {
                    TCHAR szOtherStuff[255];

                    LoadString( board.hInst, IDS_ABOUT, szOtherStuff, sizeof(szOtherStuff) / sizeof(TCHAR) );

                    ShellAbout( hWnd, szAppName, szOtherStuff, (HICON)SendMessage(hWnd, WM_GETICON, ICON_BIG, 0) );
                    return 0;
                }

                default:
                    DEBUG("Unknown WM_COMMAND command message received\n");
                    break;
            }
    }

    return( DefWindowProc( hWnd, msg, wParam, lParam ));
}

void InitBoard( BOARD *pBoard )
{
    HMENU hMenu;

    pBoard->hMinesBMP = LoadBitmap( pBoard->hInst, (LPCTSTR) IDB_MINES);
    pBoard->hFacesBMP = LoadBitmap( pBoard->hInst, (LPCTSTR) IDB_FACES);
    pBoard->hLedsBMP = LoadBitmap( pBoard->hInst, (LPCTSTR) IDB_LEDS);

    LoadBoard( pBoard );

    if( pBoard->Pos.x < GetSystemMetrics( SM_CXFIXEDFRAME ) )
        pBoard->Pos.x = GetSystemMetrics( SM_CXFIXEDFRAME );

    if( pBoard->Pos.x > (GetSystemMetrics( SM_CXSCREEN )  - GetSystemMetrics( SM_CXFIXEDFRAME )))
    {
        pBoard->Pos.x = GetSystemMetrics( SM_CXSCREEN )
        - GetSystemMetrics( SM_CXFIXEDFRAME );
    }

    if( pBoard->Pos.y < (GetSystemMetrics( SM_CYMENU ) + GetSystemMetrics( SM_CYCAPTION ) + GetSystemMetrics( SM_CYFIXEDFRAME )))
    {
        pBoard->Pos.y = GetSystemMetrics( SM_CYMENU ) +
        GetSystemMetrics( SM_CYCAPTION ) +
        GetSystemMetrics( SM_CYFIXEDFRAME );
    }

    if( pBoard->Pos.y > (GetSystemMetrics( SM_CYSCREEN ) - GetSystemMetrics( SM_CYFIXEDFRAME )))
    {
        pBoard->Pos.y = GetSystemMetrics( SM_CYSCREEN )
        - GetSystemMetrics( SM_CYFIXEDFRAME );
    }

    hMenu = GetMenu( pBoard->hWnd );
    CheckMenuItem( hMenu, IDM_BEGINNER + pBoard->Difficulty, MF_CHECKED );

    if( pBoard->bMark )
        CheckMenuItem( hMenu, IDM_MARKQ, MF_CHECKED );
    else
        CheckMenuItem( hMenu, IDM_MARKQ, MF_UNCHECKED );
    CheckLevel( pBoard );
}

static DWORD LoadDWord(HKEY hKey, TCHAR *szKeyName, DWORD dwDefaultValue)
{
    DWORD dwSize;
    DWORD dwValue;

    dwSize = sizeof(DWORD);

    if( RegQueryValueEx( hKey, szKeyName, NULL, NULL, (LPBYTE) &dwValue, &dwSize ) == ERROR_SUCCESS )
        return dwValue;

    return dwDefaultValue;
}

void LoadBoard( BOARD *pBoard )
{
    DWORD dwSize;
    HKEY hKey;
    TCHAR szData[16];
    TCHAR szKeyName[8];
    TCHAR szNobody[15];
    UCHAR i;

    RegOpenKeyEx( HKEY_CURRENT_USER, szWineMineRegKey, 0, KEY_QUERY_VALUE, &hKey );

    pBoard->Pos.x =      (LONG)       LoadDWord( hKey, TEXT("Xpos"), GetSystemMetrics(SM_CXFIXEDFRAME) );
    pBoard->Pos.y =      (LONG)       LoadDWord( hKey, TEXT("Ypos"), GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFIXEDFRAME) );
    pBoard->uRows =      (ULONG)      LoadDWord( hKey, TEXT("Height"), BEGINNER_ROWS );
    pBoard->uCols =      (ULONG)      LoadDWord( hKey, TEXT("Width"), BEGINNER_COLS );
    pBoard->uMines =     (ULONG)      LoadDWord( hKey, TEXT("Mines"), BEGINNER_MINES );
    pBoard->Difficulty = (DIFFICULTY) LoadDWord( hKey, TEXT("Difficulty"), BEGINNER );
    pBoard->bMark =      (BOOL)       LoadDWord( hKey, TEXT("Mark"), TRUE );

    LoadString( pBoard->hInst, IDS_NOBODY, szNobody, sizeof(szNobody) / sizeof(TCHAR) );

    for( i = 0; i < 3; i++ )
    {
        // As we write to the same registry key as MS WinMine does, we have to start at 1 for the registry keys
        wsprintf( szKeyName, TEXT("Name%d"), i + 1 );
        dwSize = sizeof(szData);

        if( RegQueryValueEx( hKey, szKeyName, NULL, NULL, (LPBYTE)szData, (LPDWORD) &dwSize ) == ERROR_SUCCESS )
            _tcsncpy( pBoard->szBestName[i], szData, sizeof(szData) / sizeof(TCHAR) );
        else
            _tcscpy( pBoard->szBestName[i], szNobody);
    }

    for( i = 0; i < 3; i++ )
    {
        wsprintf( szKeyName, TEXT("Time%d"), i + 1 );
        pBoard->uBestTime[i] = LoadDWord( hKey, szKeyName, 999 );
    }

    RegCloseKey(hKey);
}

void SaveBoard( BOARD *pBoard )
{
    DWORD dwValue;
    HKEY hKey;
    UCHAR i;
    TCHAR szData[16];
    TCHAR szKeyName[8];

    if( RegCreateKeyEx( HKEY_CURRENT_USER, szWineMineRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL ) != ERROR_SUCCESS)
        return;

    RegSetValueEx( hKey, TEXT("Xpos"), 0, REG_DWORD, (LPBYTE) &pBoard->Pos.x, sizeof(DWORD) );
    RegSetValueEx( hKey, TEXT("Ypos"), 0, REG_DWORD, (LPBYTE) &pBoard->Pos.y, sizeof(DWORD) );
    RegSetValueEx( hKey, TEXT("Difficulty"), 0, REG_DWORD, (LPBYTE) &pBoard->Difficulty, sizeof(DWORD) );
    RegSetValueEx( hKey, TEXT("Height"), 0, REG_DWORD, (LPBYTE) &pBoard->uRows, sizeof(DWORD) );
    RegSetValueEx( hKey, TEXT("Width"), 0, REG_DWORD, (LPBYTE) &pBoard->uCols, sizeof(DWORD) );
    RegSetValueEx( hKey, TEXT("Mines"), 0, REG_DWORD, (LPBYTE) &pBoard->uMines, sizeof(DWORD) );
    RegSetValueEx( hKey, TEXT("Mark"), 0, REG_DWORD, (LPBYTE) &pBoard->bMark, sizeof(DWORD) );

    for( i = 0; i < 3; i++ )
    {
        // As we write to the same registry key as MS WinMine does, we have to start at 1 for the registry keys
        wsprintf( szKeyName, TEXT("Name%u"), i + 1);
        _tcsncpy( szData, pBoard->szBestName[i], sizeof(szData) / sizeof(TCHAR) );
        RegSetValueEx( hKey, szKeyName, 0, REG_SZ, (LPBYTE)szData, (_tcslen(szData) + 1) * sizeof(TCHAR) );
    }

    for( i = 0; i < 3; i++ )
    {
        wsprintf( szKeyName, TEXT("Time%u"), i + 1);
        dwValue = pBoard->uBestTime[i];
        RegSetValueEx( hKey, szKeyName, 0, REG_DWORD, (LPBYTE)(LPDWORD)&dwValue, sizeof(DWORD) );
    }

    RegCloseKey(hKey);
}

void DestroyBoard( BOARD *pBoard )
{
    DeleteObject( pBoard->hFacesBMP );
    DeleteObject( pBoard->hLedsBMP );
    DeleteObject( pBoard->hMinesBMP );
}

void SetDifficulty( BOARD *pBoard, DIFFICULTY Difficulty )
{
    HMENU hMenu;

    switch(Difficulty)
    {
        case BEGINNER:
            pBoard->uCols = BEGINNER_COLS;
            pBoard->uRows = BEGINNER_ROWS;
            pBoard->uMines = BEGINNER_MINES;
            break;

        case ADVANCED:
            pBoard->uCols = ADVANCED_COLS;
            pBoard->uRows = ADVANCED_ROWS;
            pBoard->uMines = ADVANCED_MINES;
            break;

        case EXPERT:
            pBoard->uCols = EXPERT_COLS;
            pBoard->uRows = EXPERT_ROWS;
            pBoard->uMines = EXPERT_MINES;
            break;

        case CUSTOM:
            if( DialogBoxParam( pBoard->hInst, MAKEINTRESOURCE(IDD_CUSTOM), pBoard->hWnd, CustomDlgProc, (LPARAM) pBoard) != IDOK )
                return;

            break;
    }

    hMenu = GetMenu(pBoard->hWnd);
    CheckMenuItem( hMenu, IDM_BEGINNER + pBoard->Difficulty, MF_UNCHECKED );
    pBoard->Difficulty = Difficulty;
    CheckMenuItem( hMenu, IDM_BEGINNER + Difficulty, MF_CHECKED );

}

void CreateBoard( BOARD *pBoard )
{
    ULONG uLeft, uTop, uBottom, uRight, uWndX, uWndY, uWndWidth, uWndHeight;

    pBoard->uBoxesLeft = pBoard->uCols * pBoard->uRows - pBoard->uMines;
    pBoard->uNumFlags = 0;

    CreateBoxes( pBoard );

    pBoard->uWidth = pBoard->uCols * MINE_WIDTH + BOARD_WMARGIN * 2;

    pBoard->uHeight = pBoard->uRows * MINE_HEIGHT + LED_HEIGHT
        + BOARD_HMARGIN * 3;

    uWndX = pBoard->Pos.x - GetSystemMetrics( SM_CXFIXEDFRAME );
    uWndY = pBoard->Pos.y - GetSystemMetrics( SM_CYMENU )
                          - GetSystemMetrics( SM_CYCAPTION )
                          - GetSystemMetrics( SM_CYFIXEDFRAME );
    uWndWidth = pBoard->uWidth + GetSystemMetrics( SM_CXFIXEDFRAME ) * 2;
    uWndHeight = pBoard->uHeight
               + GetSystemMetrics( SM_CYMENU )
               + GetSystemMetrics( SM_CYCAPTION )
               + GetSystemMetrics( SM_CYFIXEDFRAME ) * 2;

    /* setting the mines rectangle boundary */
    uLeft = BOARD_WMARGIN;
    uTop = BOARD_HMARGIN * 2 + LED_HEIGHT;
    uRight = uLeft + pBoard->uCols * MINE_WIDTH;
    uBottom = uTop + pBoard->uRows * MINE_HEIGHT;
    SetRect( &pBoard->MinesRect, uLeft, uTop, uRight, uBottom );

    /* setting the face rectangle boundary */
    uLeft = pBoard->uWidth / 2 - FACE_WIDTH / 2;
    uTop = BOARD_HMARGIN;
    uRight = uLeft + FACE_WIDTH;
    uBottom = uTop + FACE_HEIGHT;
    SetRect( &pBoard->FaceRect, uLeft, uTop, uRight, uBottom );

    /* setting the timer rectangle boundary */
    uLeft = BOARD_WMARGIN;
    uTop = BOARD_HMARGIN;
    uRight = uLeft + LED_WIDTH * 3;
    uBottom = uTop + LED_HEIGHT;
    SetRect( &pBoard->CounterRect, uLeft, uTop, uRight, uBottom );

    /* setting the counter rectangle boundary */
    uLeft =  pBoard->uWidth - BOARD_WMARGIN - LED_WIDTH * 3;
    uTop = BOARD_HMARGIN;
    uRight = pBoard->uWidth - BOARD_WMARGIN;
    uBottom = uTop + LED_HEIGHT;
    SetRect( &pBoard->TimerRect, uLeft, uTop, uRight, uBottom );

    pBoard->Status = WAITING;
    pBoard->FaceBmp = SMILE_BMP;
    pBoard->uTime = 0;

    MoveWindow( pBoard->hWnd, uWndX, uWndY, uWndWidth, uWndHeight, TRUE );
    RedrawWindow( pBoard->hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE );
}

void CheckLevel( BOARD *pBoard )
{
    if( pBoard->uRows < BEGINNER_ROWS )
        pBoard->uRows = BEGINNER_ROWS;

    if( pBoard->uRows > MAX_ROWS )
        pBoard->uRows = MAX_ROWS;

    if( pBoard->uCols < BEGINNER_COLS )
        pBoard->uCols = BEGINNER_COLS;

    if( pBoard->uCols > MAX_COLS )
        pBoard->uCols = MAX_COLS;

    if( pBoard->uMines < BEGINNER_MINES )
        pBoard->uMines = BEGINNER_MINES;

    if( pBoard->uMines > pBoard->uCols * pBoard->uRows - 1 )
        pBoard->uMines = pBoard->uCols * pBoard->uRows - 1;
}

void CreateBoxes( BOARD *pBoard )
{
    LONG i, j;
    ULONG uCol, uRow;

    srand( (unsigned int)time( NULL ) );

    /* Create the boxes...
     * We actually create them with an empty border,
     * so special care doesn't have to be taken on the edges
     */

    for( uCol = 0; uCol <= pBoard->uCols + 1; uCol++ )
    {
        for( uRow = 0; uRow <= pBoard->uRows + 1; uRow++ )
        {
            pBoard->Box[uCol][uRow].bIsPressed = FALSE;
            pBoard->Box[uCol][uRow].bIsMine = FALSE;
            pBoard->Box[uCol][uRow].uFlagType = NORMAL;
            pBoard->Box[uCol][uRow].uNumMines = 0;
        }
    }

    /* create mines */
    i = 0;
    while( (ULONG)i < pBoard->uMines )
    {
        uCol = (ULONG)(pBoard->uCols * (float)rand() / RAND_MAX + 1);
        uRow = (ULONG)(pBoard->uRows * (float)rand() / RAND_MAX + 1);

        if( !pBoard->Box[uCol][uRow].bIsMine )
        {
            i++;
            pBoard->Box[uCol][uRow].bIsMine = TRUE;
        }
    }

    /*
     * Now we label the remaining boxes with the
     * number of mines surrounding them.
     */
    for( uCol = 1; uCol < pBoard->uCols + 1; uCol++ )
    {
        for( uRow = 1; uRow < pBoard->uRows + 1; uRow++ )
        {
            for( i = -1; i <= 1; i++ )
            {
                for( j = -1; j <= 1; j++ )
                {
                    if( pBoard->Box[uCol + i][uRow + j].bIsMine )
                    {
                        pBoard->Box[uCol][uRow].uNumMines++;
                    }
                }
            }
        }
    }
}

void DrawMines ( HDC hdc, HDC hMemDC, BOARD *pBoard )
{
    HGDIOBJ hOldObj;
    ULONG uCol, uRow;
    hOldObj = SelectObject (hMemDC, pBoard->hMinesBMP);

    for( uRow = 1; uRow <= pBoard->uRows; uRow++ )
    {
        for( uCol = 1; uCol <= pBoard->uCols; uCol++ )
        {
            DrawMine( hdc, hMemDC, pBoard, uCol, uRow, FALSE );
        }
    }

    SelectObject( hMemDC, hOldObj );
}

void DrawMine( HDC hdc, HDC hMemDC, BOARD *pBoard, ULONG uCol, ULONG uRow, BOOL bIsPressed )
{
    MINEBMP_OFFSET offset = BOX_BMP;

    if( uCol == 0 || uCol > pBoard->uCols || uRow == 0 || uRow > pBoard->uRows )
        return;

    if( pBoard->Status == GAMEOVER )
    {
        if( pBoard->Box[uCol][uRow].bIsMine )
        {
            switch( pBoard->Box[uCol][uRow].uFlagType )
            {
                case FLAG:
                    offset = FLAG_BMP;
                    break;
                case COMPLETE:
                    offset = EXPLODE_BMP;
                    break;
                case QUESTION:
                    /* fall through */
                case NORMAL:
                    offset = MINE_BMP;
            }
        }
        else
        {
            switch( pBoard->Box[uCol][uRow].uFlagType )
            {
                case QUESTION:
                    offset = QUESTION_BMP;
                    break;
                case FLAG:
                    offset = WRONG_BMP;
                    break;
                case NORMAL:
                    offset = BOX_BMP;
                    break;
                case COMPLETE:
                    /* Do nothing */
                    break;
                default:
                    DEBUG("Unknown FlagType during game over in DrawMine\n");
                    break;
            }
        }
    }
    else
    {    /* WAITING or PLAYING */
        switch( pBoard->Box[uCol][uRow].uFlagType )
        {
            case QUESTION:
                if( !bIsPressed )
                    offset = QUESTION_BMP;
                else
                    offset = QPRESS_BMP;
                break;
            case FLAG:
                offset = FLAG_BMP;
                break;
            case NORMAL:
                if( !bIsPressed )
                    offset = BOX_BMP;
                else
                    offset = MPRESS_BMP;
                break;
            case COMPLETE:
                /* Do nothing */
                break;
            default:
                DEBUG("Unknown FlagType while playing in DrawMine\n");
                break;
        }
    }

    if( pBoard->Box[uCol][uRow].uFlagType == COMPLETE && !pBoard->Box[uCol][uRow].bIsMine )
          offset = (MINEBMP_OFFSET) pBoard->Box[uCol][uRow].uNumMines;

    BitBlt( hdc,
            (uCol - 1) * MINE_WIDTH + pBoard->MinesRect.left,
            (uRow - 1) * MINE_HEIGHT + pBoard->MinesRect.top,
            MINE_WIDTH, MINE_HEIGHT,
            hMemDC, 0, offset * MINE_HEIGHT, SRCCOPY );
}

void DrawLeds( HDC hDC, HDC hMemDC, BOARD *pBoard, LONG nNumber, LONG x, LONG y )
{
    HGDIOBJ hOldObj;
    UCHAR i;
    ULONG uLED[3];
    LONG nCount;

    nCount = nNumber;

    if( nCount < 1000 )
    {
        if( nCount >= 0 )
        {
            uLED[0] = nCount / 100 ;
            nCount -= uLED[0] * 100;
        }
        else
        {
            uLED[0] = 10; /* negative sign */
            nCount = -nCount;
        }

        uLED[1] = nCount / 10;
        nCount -= uLED[1] * 10;
        uLED[2] = nCount;
    }
    else
    {
        for( i = 0; i < 3; i++ )
            uLED[i] = 10;
    }

    /* use unlit led if not playing */
    /* if( pBoard->Status == WAITING )
        for( i = 0; i < 3; i++ )
            uLED[i] = 11;*/

    hOldObj = SelectObject (hMemDC, pBoard->hLedsBMP);

    for( i = 0; i < 3; i++ )
    {
        BitBlt( hDC,
            i * LED_WIDTH + x,
            y,
            LED_WIDTH,
            LED_HEIGHT,
            hMemDC,
            0,
            uLED[i] * LED_HEIGHT,
            SRCCOPY);
    }

    SelectObject( hMemDC, hOldObj );
}

void DrawFace( HDC hDC, HDC hMemDC, BOARD *pBoard )
{
    HGDIOBJ hOldObj;

    hOldObj = SelectObject (hMemDC, pBoard->hFacesBMP);

    BitBlt( hDC,
        pBoard->FaceRect.left,
        pBoard->FaceRect.top,
        FACE_WIDTH,
        FACE_HEIGHT,
        hMemDC, 0, pBoard->FaceBmp * FACE_HEIGHT, SRCCOPY);

    SelectObject( hMemDC, hOldObj );
}

void DrawBoard( HDC hDC, HDC hMemDC, PAINTSTRUCT *ps, BOARD *pBoard )
{
    RECT TempRect;

    if( IntersectRect( &TempRect, &ps->rcPaint, &pBoard->CounterRect) )
        DrawLeds( hDC, hMemDC, pBoard, pBoard->uMines - pBoard->uNumFlags,
                  pBoard->CounterRect.left,
                  pBoard->CounterRect.top );

    if( IntersectRect( &TempRect, &ps->rcPaint, &pBoard->TimerRect ) )
        DrawLeds( hDC, hMemDC, pBoard, pBoard->uTime,
                  pBoard->TimerRect.left,
                  pBoard->TimerRect.top);

    if( IntersectRect( &TempRect, &ps->rcPaint, &pBoard->FaceRect ) )
        DrawFace( hDC, hMemDC, pBoard );

    if( IntersectRect( &TempRect, &ps->rcPaint, &pBoard->MinesRect ) )
        DrawMines( hDC, hMemDC, pBoard );
}


void TestBoard( HWND hWnd, BOARD *pBoard, LONG x, LONG y, int msg )
{
    POINT pt;
    ULONG uCol, uRow;

    pt.x = x;
    pt.y = y;

    if( PtInRect( &pBoard->MinesRect, pt ) && pBoard->Status != GAMEOVER && pBoard->Status != WON )
        TestMines( pBoard, pt, msg );
    else
    {
        UnpressBoxes( pBoard, pBoard->Press.x, pBoard->Press.y );
        pBoard->Press.x = 0;
        pBoard->Press.y = 0;
    }

    if( pBoard->uBoxesLeft == 0 )
    {
        // MG - 2006-02-21
        // mimic MS minesweeper behaviour - when autocompleting a board, flag mines 
        pBoard->Status = WON;

        for( uCol = 0; uCol <= pBoard->uCols + 1; uCol++ )
        {
            for( uRow = 0; uRow <= pBoard->uRows + 1; uRow++ )
            {
                if(pBoard->Box[uCol][uRow].bIsMine)
                {
                    pBoard->Box[uCol][uRow].uFlagType = FLAG;
                }
            }
        }

        pBoard->uNumFlags = pBoard->uMines;
        RedrawWindow( pBoard->hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
        
        if( pBoard->Difficulty != CUSTOM && pBoard->uTime < pBoard->uBestTime[pBoard->Difficulty] )
        {
            pBoard->uBestTime[pBoard->Difficulty] = pBoard->uTime;

            DialogBoxParam( pBoard->hInst, MAKEINTRESOURCE(IDD_CONGRATS), hWnd, CongratsDlgProc, (LPARAM) pBoard);
            DialogBoxParam( pBoard->hInst, MAKEINTRESOURCE(IDD_TIMES), hWnd, TimesDlgProc, (LPARAM) pBoard);
        }
    }

    TestFace( pBoard, pt, msg );
}

void TestMines( BOARD *pBoard, POINT pt, int msg )
{
    BOOL bDraw = TRUE;
    ULONG uCol, uRow;

    uCol = (pt.x - pBoard->MinesRect.left) / MINE_WIDTH + 1;
    uRow = (pt.y - pBoard->MinesRect.top ) / MINE_HEIGHT + 1;

    switch (msg)
    {
        case WM_LBUTTONDOWN:
            if( pBoard->Press.x != uCol || pBoard->Press.y != uRow )
            {
                UnpressBox( pBoard, pBoard->Press.x, pBoard->Press.y );
                pBoard->Press.x = uCol;
                pBoard->Press.y = uRow;
                PressBox( pBoard, uCol, uRow );
            }

            bDraw = FALSE;
            break;

        case WM_LBUTTONUP:
            if( pBoard->Press.x != uCol || pBoard->Press.y != uRow )
                UnpressBox( pBoard, pBoard->Press.x, pBoard->Press.y );

            pBoard->Press.x = 0;
            pBoard->Press.y = 0;

            if( pBoard->Box[uCol][uRow].uFlagType != FLAG )
                pBoard->Status = PLAYING;

            CompleteBox( pBoard, uCol, uRow );
            break;

        case WM_MBUTTONDOWN:
            PressBoxes( pBoard, uCol, uRow );
            bDraw = FALSE;
            break;

        case WM_MBUTTONUP:
            if( pBoard->Press.x != uCol || pBoard->Press.y != uRow )
                UnpressBoxes( pBoard, pBoard->Press.x, pBoard->Press.y );

            pBoard->Press.x = 0;
            pBoard->Press.y = 0;
            CompleteBoxes( pBoard, uCol, uRow );
            break;

        case WM_RBUTTONDOWN:
            AddFlag( pBoard, uCol, uRow );
            pBoard->Status = PLAYING;
            break;

        default:
            DEBUG("Unknown message type received in TestMines\n");
            break;
    }

    if(bDraw)
        RedrawWindow( pBoard->hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
}

void TestFace( BOARD *pBoard, POINT pt, int msg )
{
    if( pBoard->Status == PLAYING || pBoard->Status == WAITING )
    {
        if( msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN )
            pBoard->FaceBmp = OOH_BMP;
        else pBoard->FaceBmp = SMILE_BMP;
    }
    else if( pBoard->Status == GAMEOVER )
        pBoard->FaceBmp = DEAD_BMP;
    else if( pBoard->Status == WON )
            pBoard->FaceBmp = COOL_BMP;

    if( PtInRect( &pBoard->FaceRect, pt ) )
    {
        if( msg == WM_LBUTTONDOWN )
            pBoard->FaceBmp = SPRESS_BMP;

        if( msg == WM_LBUTTONUP )
            CreateBoard( pBoard );
    }

    RedrawWindow( pBoard->hWnd, &pBoard->FaceRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
}

void CompleteBox( BOARD *pBoard, ULONG uCol, ULONG uRow )
{
    CHAR i, j;

    if( pBoard->Box[uCol][uRow].uFlagType != COMPLETE &&
            pBoard->Box[uCol][uRow].uFlagType != FLAG &&
            uCol > 0 && uCol < pBoard->uCols + 1 &&
            uRow > 0 && uRow < pBoard->uRows + 1 )
    {
        pBoard->Box[uCol][uRow].uFlagType = COMPLETE;

        if( pBoard->Box[uCol][uRow].bIsMine )
        {
            pBoard->FaceBmp = DEAD_BMP;
            pBoard->Status = GAMEOVER;
        }
        else if( pBoard->Status != GAMEOVER )
            pBoard->uBoxesLeft--;

        if( pBoard->Box[uCol][uRow].uNumMines == 0 )
        {
            for( i = -1; i <= 1; i++ )
                for( j = -1; j <= 1; j++ )
                    CompleteBox( pBoard, uCol + i, uRow + j );
        }
    }
}

void CompleteBoxes( BOARD *pBoard, ULONG uCol, ULONG uRow )
{
    CHAR i, j;
    ULONG uNumFlags = 0;

    if( pBoard->Box[uCol][uRow].uFlagType == COMPLETE )
    {
        for( i = -1; i <= 1; i++ )
        {
            for( j = -1; j <= 1; j++ )
            {
                if( pBoard->Box[uCol + i][uRow + j].uFlagType == FLAG )
                    uNumFlags++;
            }
        }

        if( uNumFlags == pBoard->Box[uCol][uRow].uNumMines )
        {
            for( i = -1; i <= 1; i++ )
            {
                for( j = -1; j <= 1; j++ )
                {
                    if( pBoard->Box[uCol + i][uRow + j].uFlagType != FLAG )
                        CompleteBox( pBoard, uCol + i, uRow + j );
                }
            }
        }
    }
}

void AddFlag( BOARD *pBoard, ULONG uCol, ULONG uRow )
{
    if( pBoard->Box[uCol][uRow].uFlagType != COMPLETE )
    {
        switch( pBoard->Box[uCol][uRow].uFlagType )
        {
            case FLAG:
                if( pBoard->bMark )
                    pBoard->Box[uCol][uRow].uFlagType = QUESTION;
                else
                    pBoard->Box[uCol][uRow].uFlagType = NORMAL;

                pBoard->uNumFlags--;
                break;

            case QUESTION:
                pBoard->Box[uCol][uRow].uFlagType = NORMAL;
                break;

            default:
                pBoard->Box[uCol][uRow].uFlagType = FLAG;
                pBoard->uNumFlags++;
        }
    }
}

void PressBox( BOARD *pBoard, ULONG uCol, ULONG uRow )
{
    HDC hDC;
    HGDIOBJ hOldObj;
    HDC hMemDC;

    hDC = GetDC( pBoard->hWnd );
    hMemDC = CreateCompatibleDC(hDC);
    hOldObj = SelectObject (hMemDC, pBoard->hMinesBMP);

    DrawMine( hDC, hMemDC, pBoard, uCol, uRow, TRUE );

    SelectObject( hMemDC, hOldObj );
    DeleteDC( hMemDC );
    ReleaseDC( pBoard->hWnd, hDC );
}

void PressBoxes( BOARD *pBoard, ULONG uCol, ULONG uRow )
{
    CHAR i, j;

    for( i = -1; i <= 1; i++ )
    {
        for( j = -1; j <= 1; j++ )
        {
            pBoard->Box[uCol + i][uRow + j].bIsPressed = TRUE;
            PressBox( pBoard, uCol + i, uRow + j );
        }
    }

    for( i = -1; i <= 1; i++ )
    {
        for( j = -1; j <= 1; j++ )
        {
            if( !pBoard->Box[pBoard->Press.x + i][pBoard->Press.y + j].bIsPressed )
                UnpressBox( pBoard, pBoard->Press.x + i, pBoard->Press.y + j );
        }
    }

    for( i = -1; i <= 1; i++ )
    {
        for( j = -1; j <= 1; j++ )
        {
            pBoard->Box[uCol + i][uRow + j].bIsPressed = FALSE;
            PressBox( pBoard, uCol + i, uRow + j );
        }
    }

    pBoard->Press.x = uCol;
    pBoard->Press.y = uRow;
}

void UnpressBox( BOARD *pBoard, ULONG uCol, ULONG uRow )
{
    HDC hDC;
    HGDIOBJ hOldObj;
    HDC hMemDC;

    hDC = GetDC( pBoard->hWnd );
    hMemDC = CreateCompatibleDC( hDC );
    hOldObj = SelectObject( hMemDC, pBoard->hMinesBMP );

    DrawMine( hDC, hMemDC, pBoard, uCol, uRow, FALSE );

    SelectObject( hMemDC, hOldObj );
    DeleteDC( hMemDC );
    ReleaseDC( pBoard->hWnd, hDC );
}

void UnpressBoxes( BOARD *pBoard, ULONG uCol, ULONG uRow )
{
    CHAR i, j;

    for( i = -1; i <= 1; i++ )
    {
        for( j = -1; j <= 1; j++ )
        {
            UnpressBox( pBoard, uCol + i, uRow + j );
        }
    }
}
