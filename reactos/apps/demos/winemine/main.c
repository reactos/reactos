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
#include "main.h"
#include "dialog.h"
#include "resource.h"

/* Work around a Wine bug which defines handles as UINT rather than LPVOID */
#ifdef WINE_STRICT
#define NULL_HANDLE NULL
#else
#define NULL_HANDLE 0
#endif

#ifdef DUMB_DEBUG
#include <stdio.h>
#define DEBUG(x) fprintf(stderr,x)
#else
#define DEBUG(x)
#endif

int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdline, int cmdshow )
{
    MSG msg;
    WNDCLASS wc;
    HWND hWnd;
    HACCEL haccel;
    char appname[9];

    LoadString( hInst, IDS_APPNAME, appname, sizeof(appname));

    wc.style = 0;
    wc.lpfnWndProc = MainProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon( hInst, appname );
    wc.hCursor = LoadCursor( NULL_HANDLE, IDI_APPLICATION );
    wc.hbrBackground = (HBRUSH) GetStockObject( BLACK_BRUSH );
    wc.lpszMenuName = "MENU_WINEMINE";
    wc.lpszClassName = appname;

    if (!RegisterClass(&wc)) exit(1);
    hWnd = CreateWindow( appname, appname,
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL_HANDLE, NULL_HANDLE, hInst, NULL );

    if (!hWnd) exit(1);

    ShowWindow( hWnd, cmdshow );
    UpdateWindow( hWnd );

    haccel = LoadAccelerators( hInst, appname );
    SetTimer( hWnd, ID_TIMER, 1000, NULL );

    while( GetMessage(&msg, NULL_HANDLE, 0, 0) ) {
        if (!TranslateAccelerator( hWnd, haccel, &msg ))
            TranslateMessage( &msg );

        DispatchMessage( &msg );
    }
    return msg.wParam;
}

LRESULT WINAPI MainProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    HMENU hMenu;
    static BOARD board;

    switch( msg ) {
    case WM_CREATE:
        board.hInst = ((LPCREATESTRUCT) lParam)->hInstance;
        board.hWnd = hWnd;
        InitBoard( &board );
        CreateBoard( &board );
        return 0;

    case WM_PAINT:
      {
        HDC hMemDC;

        DEBUG("WM_PAINT\n");
        hdc = BeginPaint( hWnd, &ps );
        hMemDC = CreateCompatibleDC( hdc );

        DrawBoard( hdc, hMemDC, &ps, &board );

        DeleteDC( hMemDC );
        EndPaint( hWnd, &ps );

        return 0;
      }

    case WM_MOVE:
        DEBUG("WM_MOVE\n");
        board.pos.x = (unsigned) LOWORD(lParam);
        board.pos.y = (unsigned) HIWORD(lParam);
        return 0;

    case WM_DESTROY:
        SaveBoard( &board );
        DestroyBoard( &board );
        PostQuitMessage( 0 );
        return 0;

    case WM_TIMER:
        if( board.status == PLAYING ) {
            board.time++;
                  RedrawWindow( hWnd, &board.timer_rect, NULL_HANDLE,
                    RDW_INVALIDATE | RDW_UPDATENOW );
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
        if( wParam & MK_LBUTTON ) {
            board.press.x = 0;
            board.press.y = 0;
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
        if( (wParam & MK_LBUTTON) && (wParam & MK_RBUTTON) ) {
            msg = WM_MBUTTONDOWN;
        }
        else if( wParam & MK_LBUTTON ) {
            msg = WM_LBUTTONDOWN;
        }
        else {
            return 0;
        }

        TestBoard( hWnd, &board, LOWORD(lParam), HIWORD(lParam),  msg );

        return 0;
    }

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDM_NEW:
            CreateBoard( &board );
            return 0;

        case IDM_MARKQ:
            hMenu = GetMenu( hWnd );
            board.IsMarkQ = !board.IsMarkQ;
            if( board.IsMarkQ )
                CheckMenuItem( hMenu, IDM_MARKQ, MF_CHECKED );
            else
                CheckMenuItem( hMenu, IDM_MARKQ, MF_UNCHECKED );
            return 0;

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
            DialogBoxParam( board.hInst, "DLG_TIMES", hWnd,
                    TimesDlgProc, (LPARAM) &board);
            return 0;

        case IDM_ABOUT:
            DialogBox( board.hInst, "DLG_ABOUT", hWnd, AboutDlgProc );
            return 0;
        default:
            DEBUG("Unknown WM_COMMAND command message received\n");
            break;
        }
    }
    return( DefWindowProc( hWnd, msg, wParam, lParam ));
}

void InitBoard( BOARD *p_board )
{
    HMENU hMenu;

    p_board->hMinesBMP = LoadBitmap( p_board->hInst, (LPCSTR) IDB_MINES);
    p_board->hFacesBMP = LoadBitmap( p_board->hInst, (LPCSTR) IDB_FACES);
    p_board->hLedsBMP = LoadBitmap( p_board->hInst, (LPCSTR) IDB_LEDS);

    LoadBoard( p_board );

    if( p_board->pos.x < (unsigned) GetSystemMetrics( SM_CXFIXEDFRAME ))
        p_board->pos.x = GetSystemMetrics( SM_CXFIXEDFRAME );

    if( p_board->pos.x > (unsigned) (GetSystemMetrics( SM_CXSCREEN )
    - GetSystemMetrics( SM_CXFIXEDFRAME ))) {
        p_board->pos.x = GetSystemMetrics( SM_CXSCREEN )
        - GetSystemMetrics( SM_CXFIXEDFRAME );
    }

    if( p_board->pos.y < (unsigned) (GetSystemMetrics( SM_CYMENU )
    + GetSystemMetrics( SM_CYCAPTION )
    + GetSystemMetrics( SM_CYFIXEDFRAME ))) {
        p_board->pos.y = GetSystemMetrics( SM_CYMENU ) +
        GetSystemMetrics( SM_CYCAPTION ) +
        GetSystemMetrics( SM_CYFIXEDFRAME );
    }

    if( p_board->pos.y > (unsigned) (GetSystemMetrics( SM_CYSCREEN )
    - GetSystemMetrics( SM_CYFIXEDFRAME ))) {
        p_board->pos.y = GetSystemMetrics( SM_CYSCREEN )
        - GetSystemMetrics( SM_CYFIXEDFRAME );
    }

    hMenu = GetMenu( p_board->hWnd );
    CheckMenuItem( hMenu, IDM_BEGINNER + (unsigned) p_board->difficulty,
            MF_CHECKED );
    if( p_board->IsMarkQ )
        CheckMenuItem( hMenu, IDM_MARKQ, MF_CHECKED );
    else
        CheckMenuItem( hMenu, IDM_MARKQ, MF_UNCHECKED );
    CheckLevel( p_board );
}

void LoadBoard( BOARD *p_board )
{
    DWORD size;
    DWORD type;
    HKEY hkey;
    char data[16];
    char key_name[8];
    unsigned i;


    RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Wine\\WineMine",
            0, KEY_QUERY_VALUE, &hkey );

    size = sizeof( data );
    if( RegQueryValueEx( hkey, "Xpos", NULL, (LPDWORD) &type,
            (LPBYTE) data, (LPDWORD) &size ) == ERROR_SUCCESS ) {
        p_board->pos.x = atoi( data );
    }
    else
        p_board->pos.x = GetSystemMetrics( SM_CXFIXEDFRAME );

    size = sizeof( data );
    if( RegQueryValueEx( hkey, "Ypos", NULL, (LPDWORD) &type,
            (LPBYTE) data, (LPDWORD) &size ) == ERROR_SUCCESS )
        p_board->pos.y = atoi( data );
    else
        p_board->pos.y = GetSystemMetrics( SM_CYMENU )
        + GetSystemMetrics( SM_CYCAPTION )
        + GetSystemMetrics( SM_CYFIXEDFRAME );

    size = sizeof( data );
    if( RegQueryValueEx( hkey, "Rows", NULL, (LPDWORD) &type,
            (LPBYTE) data, (LPDWORD) &size ) == ERROR_SUCCESS )
        p_board->rows = atoi( data );
    else
        p_board->rows = BEGINNER_ROWS;

    size = sizeof( data );
    if( RegQueryValueEx( hkey, "Cols", NULL, (LPDWORD) &type,
            (LPBYTE) data, (LPDWORD) &size ) == ERROR_SUCCESS )
        p_board->cols = atoi( data );
    else
        p_board->cols = BEGINNER_COLS;

    size = sizeof( data );
    if( RegQueryValueEx( hkey, "Mines", NULL, (LPDWORD) &type,
            (LPBYTE) data, (LPDWORD) &size ) == ERROR_SUCCESS )
        p_board->mines = atoi( data );
    else
        p_board->rows = BEGINNER_ROWS;

    size = sizeof( data );
    if( RegQueryValueEx( hkey, "Difficulty", NULL, (LPDWORD) &type,
            (LPBYTE) data, (LPDWORD) &size ) == ERROR_SUCCESS )
        p_board->difficulty = (DIFFICULTY) atoi( data );
    else
        p_board->difficulty = BEGINNER;

    size = sizeof( data );
    if( RegQueryValueEx( hkey, "MarkQ", NULL, (LPDWORD) &type,
            (LPBYTE) data, (LPDWORD) &size ) == ERROR_SUCCESS )
        p_board->IsMarkQ = atoi( data );
    else
        p_board->IsMarkQ = TRUE;

    for( i = 0; i < 3; i++ ) {
        wsprintf( key_name, "Name%d", i );
        size = sizeof( data );
        if( RegQueryValueEx( hkey, key_name, NULL, (LPDWORD) &type,
                (LPBYTE) data,
                (LPDWORD) &size ) == ERROR_SUCCESS )
                    strncpy( p_board->best_name[i], data, sizeof( data ) );
        else
            wsprintf( p_board->best_name[i], "Nobody");
    }

    for( i = 0; i < 3; i++ ) {
        wsprintf( key_name, "Time%d", i );
        size = sizeof( data );
        if( RegQueryValueEx( hkey, key_name, NULL, (LPDWORD) &type,
                (LPBYTE) data,
                (LPDWORD) &size ) == ERROR_SUCCESS )
            p_board->best_time[i] = atoi( data );
        else
            p_board->best_time[i] = 999;
    }
    RegCloseKey( hkey );
}

void SaveBoard( BOARD *p_board )
{
    DWORD disp;
    HKEY hkey;
    SECURITY_ATTRIBUTES sa;
    unsigned i;
    char data[16];
    char key_name[8];

    if( RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                "Software\\Wine\\WineMine", 0, NULL,
                REG_OPTION_NON_VOLATILE, KEY_WRITE, &sa,
                &hkey, &disp ) != ERROR_SUCCESS)
        return;

    wsprintf( data, "%d", p_board->pos.x );
    RegSetValueEx( hkey, "Xpos", 0, REG_SZ, (LPBYTE) data, strlen(data)+1 );

    wsprintf( data, "%d", p_board->pos.x );
    RegSetValueEx( hkey, "Ypos", 0, REG_SZ, (LPBYTE) data, strlen(data)+1 );

    wsprintf( data, "%d", (int) p_board->difficulty );
    RegSetValueEx( hkey, "Difficulty", 0, REG_SZ, (LPBYTE) data, strlen(data)+1 );

    wsprintf( data, "%d", p_board->rows );
    RegSetValueEx( hkey, "Rows", 0, REG_SZ, (LPBYTE) data, strlen(data)+1 );

    wsprintf( data, "%d", p_board->cols );
    RegSetValueEx( hkey, "Cols", 0, REG_SZ, (LPBYTE) data, strlen(data)+1 );

    wsprintf( data, "%d", p_board->mines );
    RegSetValueEx( hkey, "Mines", 0, REG_SZ, (LPBYTE) data, strlen(data)+1 );

    wsprintf( data, "%d", (int) p_board->IsMarkQ );
    RegSetValueEx( hkey, "MarkQ", 0, REG_SZ, (LPBYTE) data, strlen(data)+1 );

    for( i = 0; i < 3; i++ ) {
        wsprintf( key_name, "Name%u", i );
        strncpy( data, p_board->best_name[i], sizeof( data ) );
        RegSetValueEx( hkey, key_name, 0, REG_SZ, (LPBYTE) data, strlen(data)+1 );
    }

    for( i = 0; i < 3; i++ ) {
        wsprintf( key_name, "Time%u", i );
        wsprintf( data, "%d", p_board->best_time[i] );
        RegSetValueEx( hkey, key_name, 0, REG_SZ, (LPBYTE) data, strlen(data)+1 );
    }
    RegCloseKey( hkey );
}

void DestroyBoard( BOARD *p_board )
{
    DeleteObject( p_board->hFacesBMP );
    DeleteObject( p_board->hLedsBMP );
    DeleteObject( p_board->hMinesBMP );
}

void SetDifficulty( BOARD *p_board, DIFFICULTY difficulty )
{
    HMENU hMenu = GetMenu( p_board->hWnd );

    CheckMenuItem( hMenu, IDM_BEGINNER + p_board->difficulty, MF_UNCHECKED );
    p_board->difficulty = difficulty;
    CheckMenuItem( hMenu, IDM_BEGINNER + difficulty, MF_CHECKED );

    switch( difficulty ) {
    case BEGINNER:
        p_board->cols = BEGINNER_COLS;
        p_board->rows = BEGINNER_ROWS;
        p_board->mines = BEGINNER_MINES;
        break;

    case ADVANCED:
        p_board->cols = ADVANCED_COLS;
        p_board->rows = ADVANCED_ROWS;
        p_board->mines = ADVANCED_MINES;
        break;

    case EXPERT:
        p_board->cols = EXPERT_COLS;
        p_board->rows = EXPERT_ROWS;
        p_board->mines = EXPERT_MINES;
        break;

    case CUSTOM:
        DialogBoxParam( p_board->hInst, "DLG_CUSTOM", p_board->hWnd,
                CustomDlgProc, (LPARAM) p_board);
        break;
    }
}

void CreateBoard( BOARD *p_board )
{
    int left, top, bottom, right, wnd_x, wnd_y, wnd_width, wnd_height;

    p_board->mb = MB_NONE;
    p_board->boxes_left = p_board->cols * p_board->rows - p_board->mines;
    p_board->num_flags = 0;

    CreateBoxes( p_board );

    p_board->width = p_board->cols * MINE_WIDTH + BOARD_WMARGIN * 2;

    p_board->height = p_board->rows * MINE_HEIGHT + LED_HEIGHT
        + BOARD_HMARGIN * 3;

    wnd_x = p_board->pos.x - GetSystemMetrics( SM_CXFIXEDFRAME );
    wnd_y = p_board->pos.y - GetSystemMetrics( SM_CYMENU )
        - GetSystemMetrics( SM_CYCAPTION )
        - GetSystemMetrics( SM_CYFIXEDFRAME );
    wnd_width = p_board->width
        + GetSystemMetrics( SM_CXFIXEDFRAME ) * 2;
    wnd_height = p_board->height
        + GetSystemMetrics( SM_CYMENU )
        + GetSystemMetrics( SM_CYCAPTION )
        + GetSystemMetrics( SM_CYFIXEDFRAME ) * 2;

    /* setting the mines rectangle boundary */
    left = BOARD_WMARGIN;
    top = BOARD_HMARGIN * 2 + LED_HEIGHT;
    right = left + p_board->cols * MINE_WIDTH;
    bottom = top + p_board->rows * MINE_HEIGHT;
    SetRect( &p_board->mines_rect, left, top, right, bottom );

    /* setting the face rectangle boundary */
    left = p_board->width / 2 - FACE_WIDTH / 2;
    top = BOARD_HMARGIN;
    right = left + FACE_WIDTH;
    bottom = top + FACE_HEIGHT;
    SetRect( &p_board->face_rect, left, top, right, bottom );

    /* setting the timer rectangle boundary */
    left = BOARD_WMARGIN;
    top = BOARD_HMARGIN;
    right = left + LED_WIDTH * 3;
    bottom = top + LED_HEIGHT;
    SetRect( &p_board->timer_rect, left, top, right, bottom );

    /* setting the counter rectangle boundary */
    left =  p_board->width - BOARD_WMARGIN - LED_WIDTH * 3;
    top = BOARD_HMARGIN;
    right = p_board->width - BOARD_WMARGIN;
    bottom = top + LED_HEIGHT;
    SetRect( &p_board->counter_rect, left, top, right, bottom );

    p_board->status = WAITING;
    p_board->face_bmp = SMILE_BMP;
    p_board->time = 0;

    MoveWindow( p_board->hWnd, wnd_x, wnd_y, wnd_width, wnd_height, TRUE );
    RedrawWindow( p_board->hWnd, NULL, NULL_HANDLE,
            RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE );
}


void CheckLevel( BOARD *p_board )
{
    if( p_board->rows < BEGINNER_ROWS )
        p_board->rows = BEGINNER_ROWS;

    if( p_board->rows > MAX_ROWS )
        p_board->rows = MAX_ROWS;

    if( p_board->cols < BEGINNER_COLS )
        p_board->cols = BEGINNER_COLS;

    if( p_board->cols > MAX_COLS )
        p_board->cols = MAX_COLS;

    if( p_board->mines < BEGINNER_MINES )
        p_board->mines = BEGINNER_MINES;

    if( p_board->mines > p_board->cols * p_board->rows - 1 )
        p_board->mines = p_board->cols * p_board->rows - 1;
}


void CreateBoxes( BOARD *p_board )
{
    int i, j;
    unsigned col, row;

    srand( (unsigned) time( NULL ) );

    /* Create the boxes...
     * We actually create them with an empty border,
     * so special care doesn't have to be taken on the edges
     */

    for( col = 0; col <= p_board->cols + 1; col++ )
      for( row = 0; row <= p_board->rows + 1; row++ ) {
        p_board->box[col][row].IsPressed = FALSE;
        p_board->box[col][row].IsMine = FALSE;
        p_board->box[col][row].FlagType = NORMAL;
        p_board->box[col][row].NumMines = 0;
      }

    /* create mines */
    i = 0;
     while( (unsigned) i < p_board->mines ) {
          col = (int) (p_board->cols * (float) rand() / RAND_MAX + 1);
          row = (int) (p_board->rows * (float) rand() / RAND_MAX + 1);

        if( !p_board->box[col][row].IsMine ) {
            i++;
            p_board->box[col][row].IsMine = TRUE;
        }
    }

    /*
     * Now we label the remaining boxes with the
     * number of mines surrounding them.
     */

    for( col = 1; col < p_board->cols + 1; col++ )
    for( row = 1; row < p_board->rows + 1; row++ ) {
        for( i = -1; i <= 1; i++ )
        for( j = -1; j <= 1; j++ ) {
            if( p_board->box[col + i][row + j].IsMine ) {
                p_board->box[col][row].NumMines++ ;
            }
        }
    }
}

void DrawMines ( HDC hdc, HDC hMemDC, BOARD *p_board )
{
    HGDIOBJ hOldObj;
    unsigned col, row;
    hOldObj = SelectObject (hMemDC, p_board->hMinesBMP);

    for( row = 1; row <= p_board->rows; row++ ) {
      for( col = 1; col <= p_board->cols; col++ ) {
        DrawMine( hdc, hMemDC, p_board, col, row, FALSE );
      }
    }
    SelectObject( hMemDC, hOldObj );
}

void DrawMine( HDC hdc, HDC hMemDC, BOARD *p_board, unsigned col, unsigned row, BOOL IsPressed )
{
    MINEBMP_OFFSET offset = BOX_BMP;

    if( col == 0 || col > p_board->cols || row == 0 || row > p_board->rows )
           return;

    if( p_board->status == GAMEOVER ) {
        if( p_board->box[col][row].IsMine ) {
            switch( p_board->box[col][row].FlagType ) {
            case FLAG:
                offset = FLAG_BMP;
                break;
            case COMPLETE:
                offset = EXPLODE_BMP;
                break;
            case NORMAL:
                offset = MINE_BMP;
            }
        } else {
            switch( p_board->box[col][row].FlagType ) {
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
    } else {    /* WAITING or PLAYING */
        switch( p_board->box[col][row].FlagType ) {
        case QUESTION:
            if( !IsPressed )
                offset = QUESTION_BMP;
            else
                offset = QPRESS_BMP;
            break;
        case FLAG:
            offset = FLAG_BMP;
            break;
        case NORMAL:
            if( !IsPressed )
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

    if( p_board->box[col][row].FlagType == COMPLETE
        && !p_board->box[col][row].IsMine )
          offset = (MINEBMP_OFFSET) p_board->box[col][row].NumMines;

    BitBlt( hdc,
            (col - 1) * MINE_WIDTH + p_board->mines_rect.left,
            (row - 1) * MINE_HEIGHT + p_board->mines_rect.top,
            MINE_WIDTH, MINE_HEIGHT,
            hMemDC, 0, offset * MINE_HEIGHT, SRCCOPY );
}

void DrawLeds( HDC hdc, HDC hMemDC, BOARD *p_board, int number, int x, int y )
{
    HGDIOBJ hOldObj;
    unsigned led[3], i;
    int count;

    count = number;
    if( count < 1000 ) {
        if( count >= 0 ) {
            led[0] = count / 100 ;
            count -= led[0] * 100;
        }
        else {
            led[0] = 10; /* negative sign */
            count = -count;
        }
        led[1] = count / 10;
        count -= led[1] * 10;
        led[2] = count;
    }
    else {
        for( i = 0; i < 3; i++ )
            led[i] = 10;
    }

    /* use unlit led if not playing */
    if( p_board->status == WAITING )
        for( i = 0; i < 3; i++ )
            led[i] = 11;

    hOldObj = SelectObject (hMemDC, p_board->hLedsBMP);

    for( i = 0; i < 3; i++ ) {
        BitBlt( hdc,
            i * LED_WIDTH + x,
            y,
            LED_WIDTH,
            LED_HEIGHT,
            hMemDC,
            0,
            led[i] * LED_HEIGHT,
            SRCCOPY);
    }

    SelectObject( hMemDC, hOldObj );
}


void DrawFace( HDC hdc, HDC hMemDC, BOARD *p_board )
{
    HGDIOBJ hOldObj;

    hOldObj = SelectObject (hMemDC, p_board->hFacesBMP);

    BitBlt( hdc,
        p_board->face_rect.left,
        p_board->face_rect.top,
        FACE_WIDTH,
        FACE_HEIGHT,
        hMemDC, 0, p_board->face_bmp * FACE_HEIGHT, SRCCOPY);

    SelectObject( hMemDC, hOldObj );
}


void DrawBoard( HDC hdc, HDC hMemDC, PAINTSTRUCT *ps, BOARD *p_board )
{
    RECT tmp_rect;

    if( IntersectRect( &tmp_rect, &ps->rcPaint, &p_board->counter_rect ) )
        DrawLeds( hdc, hMemDC, p_board, p_board->mines - p_board->num_flags,
                  p_board->counter_rect.left,
                  p_board->counter_rect.top );

    if( IntersectRect( &tmp_rect, &ps->rcPaint, &p_board->timer_rect ) )
        DrawLeds( hdc, hMemDC, p_board, p_board->time,
                  p_board->timer_rect.left,
                  p_board->timer_rect.top );

    if( IntersectRect( &tmp_rect, &ps->rcPaint, &p_board->face_rect ) )
        DrawFace( hdc, hMemDC, p_board );

    if( IntersectRect( &tmp_rect, &ps->rcPaint, &p_board->mines_rect ) )
        DrawMines( hdc, hMemDC, p_board );
}


void TestBoard( HWND hWnd, BOARD *p_board, unsigned x, unsigned y, int msg )
{
    POINT pt;

    pt.x = x;
    pt.y = y;

    if( PtInRect( &p_board->mines_rect, pt ) && p_board->status != GAMEOVER
    && p_board->status != WON )
        TestMines( p_board, pt, msg );
    else {
        UnpressBoxes( p_board,
            p_board->press.x,
            p_board->press.y );
        p_board->press.x = 0;
        p_board->press.y = 0;
    }

    if( p_board->boxes_left == 0 ) {
        p_board->status = WON;

        if( p_board->difficulty != CUSTOM &&
                    p_board->time < p_board->best_time[p_board->difficulty] ) {
            p_board->best_time[p_board->difficulty] = p_board->time;

            DialogBoxParam( p_board->hInst, "DLG_CONGRATS", hWnd,
                    CongratsDlgProc, (LPARAM) p_board);

            DialogBoxParam( p_board->hInst, "DLG_TIMES", hWnd,
                    TimesDlgProc, (LPARAM) p_board);
        }
    }
    TestFace( p_board, pt, msg );
}

void TestMines( BOARD *p_board, POINT pt, int msg )
{
    BOOL draw = TRUE;
    unsigned col, row;

    col = (pt.x - p_board->mines_rect.left) / MINE_WIDTH + 1;
    row = (pt.y - p_board->mines_rect.top ) / MINE_HEIGHT + 1;

    switch ( msg ) {
    case WM_LBUTTONDOWN:
        if( p_board->press.x != col || p_board->press.y != row ) {
            UnpressBox( p_board,
                    p_board->press.x, p_board->press.y );
            p_board->press.x = col;
            p_board->press.y = row;
            PressBox( p_board, col, row );
        }
        draw = FALSE;
        break;

    case WM_LBUTTONUP:
        if( p_board->press.x != col || p_board->press.y != row )
            UnpressBox( p_board,
                    p_board->press.x, p_board->press.y );
        p_board->press.x = 0;
        p_board->press.y = 0;
        if( p_board->box[col][row].FlagType != FLAG )
            p_board->status = PLAYING;
        CompleteBox( p_board, col, row );
        break;

    case WM_MBUTTONDOWN:
        PressBoxes( p_board, col, row );
        draw = FALSE;
        break;

    case WM_MBUTTONUP:
        if( p_board->press.x != col || p_board->press.y != row )
            UnpressBoxes( p_board,
                    p_board->press.x, p_board->press.y );
        p_board->press.x = 0;
        p_board->press.y = 0;
        CompleteBoxes( p_board, col, row );
        break;

    case WM_RBUTTONDOWN:
        AddFlag( p_board, col, row );
        p_board->status = PLAYING;
        break;
    default:
        DEBUG("Unknown message type received in TestMines\n");
        break;
    }

    if( draw )
    {
        RedrawWindow( p_board->hWnd, NULL, NULL_HANDLE,
            RDW_INVALIDATE | RDW_UPDATENOW );
    }
}


void TestFace( BOARD *p_board, POINT pt, int msg )
{
    if( p_board->status == PLAYING || p_board->status == WAITING ) {
        if( msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN )
            p_board->face_bmp = OOH_BMP;
        else p_board->face_bmp = SMILE_BMP;
    }
    else if( p_board->status == GAMEOVER )
        p_board->face_bmp = DEAD_BMP;
    else if( p_board->status == WON )
            p_board->face_bmp = COOL_BMP;

    if( PtInRect( &p_board->face_rect, pt ) ) {
        if( msg == WM_LBUTTONDOWN )
            p_board->face_bmp = SPRESS_BMP;

        if( msg == WM_LBUTTONUP )
            CreateBoard( p_board );
    }

    RedrawWindow( p_board->hWnd, &p_board->face_rect, NULL_HANDLE,
        RDW_INVALIDATE | RDW_UPDATENOW );
}


void CompleteBox( BOARD *p_board, unsigned col, unsigned row )
{
    int i, j;

    if( p_board->box[col][row].FlagType != COMPLETE &&
            p_board->box[col][row].FlagType != FLAG &&
            col > 0 && col < p_board->cols + 1 &&
            row > 0 && row < p_board->rows + 1 ) {
        p_board->box[col][row].FlagType = COMPLETE;

        if( p_board->box[col][row].IsMine ) {
            p_board->face_bmp = DEAD_BMP;
            p_board->status = GAMEOVER;
        }
        else if( p_board->status != GAMEOVER )
            p_board->boxes_left--;

        if( p_board->box[col][row].NumMines == 0 )
        {
            for( i = -1; i <= 1; i++ )
            for( j = -1; j <= 1; j++ )
                CompleteBox( p_board, col + i, row + j  );
        }
    }
}


void CompleteBoxes( BOARD *p_board, unsigned col, unsigned row )
{
    unsigned numFlags = 0;
    int i, j;

    if( p_board->box[col][row].FlagType == COMPLETE ) {
        for( i = -1; i <= 1; i++ )
          for( j = -1; j <= 1; j++ ) {
            if( p_board->box[col+i][row+j].FlagType == FLAG )
                numFlags++;
          }

        if( numFlags == p_board->box[col][row].NumMines ) {
            for( i = -1; i <= 1; i++ )
              for( j = -1; j <= 1; j++ ) {
                if( p_board->box[col+i][row+j].FlagType != FLAG )
                    CompleteBox( p_board, col+i, row+j );
              }
        }
    }
}


void AddFlag( BOARD *p_board, unsigned col, unsigned row )
{
    if( p_board->box[col][row].FlagType != COMPLETE ) {
        switch( p_board->box[col][row].FlagType ) {
        case FLAG:
            if( p_board->IsMarkQ )
                p_board->box[col][row].FlagType = QUESTION;
            else
                p_board->box[col][row].FlagType = NORMAL;
            p_board->num_flags--;
            break;

        case QUESTION:
            p_board->box[col][row].FlagType = NORMAL;
            break;

        default:
            p_board->box[col][row].FlagType = FLAG;
            p_board->num_flags++;
        }
    }
}


void PressBox( BOARD *p_board, unsigned col, unsigned row )
{
    HDC hdc;
    HGDIOBJ hOldObj;
    HDC hMemDC;

    hdc = GetDC( p_board->hWnd );
    hMemDC = CreateCompatibleDC( hdc );
    hOldObj = SelectObject (hMemDC, p_board->hMinesBMP);

    DrawMine( hdc, hMemDC, p_board, col, row, TRUE );

    SelectObject( hMemDC, hOldObj );
    DeleteDC( hMemDC );
    ReleaseDC( p_board->hWnd, hdc );
}


void PressBoxes( BOARD *p_board, unsigned col, unsigned row )
{
    int i, j;

    for( i = -1; i <= 1; i++ )
      for( j = -1; j <= 1; j++ ) {
        p_board->box[col + i][row + j].IsPressed = TRUE;
        PressBox( p_board, col + i, row + j );
    }

    for( i = -1; i <= 1; i++ )
      for( j = -1; j <= 1; j++ ) {
        if( !p_board->box[p_board->press.x + i][p_board->press.y + j].IsPressed )
            UnpressBox( p_board, p_board->press.x + i, p_board->press.y + j );
    }

    for( i = -1; i <= 1; i++ )
      for( j = -1; j <= 1; j++ ) {
        p_board->box[col + i][row + j].IsPressed = FALSE;
        PressBox( p_board, col + i, row + j );
    }

    p_board->press.x = col;
    p_board->press.y = row;
}


void UnpressBox( BOARD *p_board, unsigned col, unsigned row )
{
    HDC hdc;
    HGDIOBJ hOldObj;
    HDC hMemDC;

    hdc = GetDC( p_board->hWnd );
    hMemDC = CreateCompatibleDC( hdc );
    hOldObj = SelectObject( hMemDC, p_board->hMinesBMP );

    DrawMine( hdc, hMemDC, p_board, col, row, FALSE );

    SelectObject( hMemDC, hOldObj );
    DeleteDC( hMemDC );
    ReleaseDC( p_board->hWnd, hdc );
}


void UnpressBoxes( BOARD *p_board, unsigned col, unsigned row )
{
    int i, j;

    for( i = -1; i <= 1; i++ )
      for( j = -1; j <= 1; j++ ) {
        UnpressBox( p_board, col + i, row + j );
      }
}
