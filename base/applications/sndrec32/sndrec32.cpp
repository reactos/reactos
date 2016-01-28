/* PROJECT:         ReactOS sndrec32
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/sndrec32.cpp
 * PURPOSE:         Sound recording
 * PROGRAMMERS:     Marco Pagliaricci (irc: rendar)
 *                  Robert Naumann (gonzoMD)
 */

#include "stdafx.h"

#include <commctrl.h>
#include <commdlg.h>

#include "sndrec32.h"
#include "shellapi.h"

#ifndef _UNICODE
#define gprintf _snprintf
#else
#define gprintf _snwprintf
#endif

HINSTANCE hInst;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];

ATOM MyRegisterClass(HINSTANCE hInstance);
ATOM MyRegisterClass_wave(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
BOOL InitInstance_wave(HWND, HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc_wave(HWND, UINT, WPARAM, LPARAM);

BOOL win_first, wout_first;

HWND main_win;
HWND wave_win;
HWND slider;
HWND buttons[5];
HBITMAP butbmps[5];
HBITMAP butbmps_dis[5];
WNDPROC buttons_std_proc;

BOOL butdisabled[5];
BOOL stopped_flag;
BOOL isnew;
BOOL display_dur;

DWORD slider_pos;
WORD slider_min;
WORD slider_max;

DWORD samples_max;

OPENFILENAME ofn;
TCHAR file_path[MAX_PATH];
TCHAR str_pos[MAX_LOADSTRING];
TCHAR str_dur[MAX_LOADSTRING];
TCHAR str_buf[MAX_LOADSTRING];
TCHAR str_fmt[MAX_LOADSTRING];
TCHAR str_chan[MAX_LOADSTRING];

TCHAR str_mono[10];
TCHAR str_stereo[10];

BOOL path_set;

snd::audio_membuffer *AUD_BUF;
snd::audio_waveout *AUD_OUT;
snd::audio_wavein *AUD_IN;

BOOL s_recording;

NONCLIENTMETRICS s_info;

RECT text_rect;
RECT text2_rect;
RECT cli;

INT_PTR
CALLBACK
AboutDlgProc(HWND hWnd,
             UINT msg,
             WPARAM wp,
             LPARAM lp)
{
    switch (msg)
    {
        case WM_COMMAND:
            switch (LOWORD(wp))
            {
                case IDOK:
                    EndDialog(hWnd, 0);
                    return TRUE;
            }
            break;
        case WM_CLOSE:
            EndDialog(hWnd, 0);
            return TRUE;
    }
    return FALSE;
}

int
APIENTRY
_tWinMain(HINSTANCE hInstance,
          HINSTANCE hPrevInstance,
          LPTSTR lpCmdLine,
          int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MSG msg;
    HACCEL hAccelTable;

    s_info.cbSize = sizeof( NONCLIENTMETRICS );

    InitCommonControls();

    win_first = wout_first = FALSE;

    text_rect.left = REFRESHA_X;
    text_rect.top = REFRESHA_Y;
    text_rect.right = REFRESHA_CX;
    text_rect.bottom = REFRESHA_CY;

    text2_rect.left = REFRESHB_X;
    text2_rect.top = REFRESHB_Y;
    text2_rect.right = REFRESHB_CX;
    text2_rect.bottom = REFRESHB_CY;

    /* Retrieving defaul system font, and others system informations */
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
                         sizeof(NONCLIENTMETRICS),
                         &s_info,
                         0);

    /* Set font size */
    s_info.lfMenuFont.lfHeight = 14;

    /* Inits buttons bitmaps */

    butbmps[0] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP2_START));
    butbmps[1] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP2_END));
    butbmps[2] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP2_PLAY));
    butbmps[3] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP2_STOP));
    butbmps[4] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP2_REC));

    butbmps_dis[0] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP2_START_DIS));
    butbmps_dis[1] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP2_END_DIS));
    butbmps_dis[2] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP2_PLAY_DIS));
    butbmps_dis[3] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP2_STOP_DIS));
    butbmps_dis[4] = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP2_REC_DIS));

    /* Inits audio devices and buffers */

    snd::audio_membuffer AUD_buffer(snd::A44100_16BIT_STEREO);
    snd::audio_waveout AUD_waveout(snd::A44100_16BIT_STEREO, AUD_buffer);
    snd::audio_wavein AUD_wavein(snd::A44100_16BIT_STEREO, AUD_buffer);

    AUD_buffer.play_finished = l_play_finished;
    AUD_buffer.audio_arrival = l_audio_arrival;
    AUD_buffer.buffer_resized = l_buffer_resized;

    AUD_buffer.alloc_seconds(INITIAL_BUFREC_SECONDS);

    AUD_IN = &AUD_wavein;
    AUD_OUT = &AUD_waveout;
    AUD_BUF = &AUD_buffer;

    /* Inits slider default parameters */

    slider_pos = 0;
    slider_min = 0;
    slider_max = SLIDER_W;

    stopped_flag = FALSE;
    path_set = FALSE;
    isnew = TRUE;
    display_dur = TRUE;

    samples_max = AUD_buffer.total_samples();

    s_recording = false;

    /* Inits strings */
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_REACTOS_SNDREC32, szWindowClass, MAX_LOADSTRING);
    LoadString(hInstance, IDS_STRPOS, str_pos, MAX_LOADSTRING);
    LoadString(hInstance, IDS_STRDUR, str_dur, MAX_LOADSTRING);
    LoadString(hInstance, IDS_STRBUF, str_buf, MAX_LOADSTRING);
    LoadString(hInstance, IDS_STRFMT, str_fmt, MAX_LOADSTRING);
    LoadString(hInstance, IDS_STRCHAN, str_chan, MAX_LOADSTRING);
    LoadString(hInstance, IDS_STRMONO, str_mono, 10);
    LoadString(hInstance, IDS_STRSTEREO, str_stereo, 10);

    /* Registers sndrec32 window class */
    MyRegisterClass(hInstance);
    MyRegisterClass_wave(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        MessageBox(0, TEXT("CreateWindow() Error!"), TEXT("ERROR"), MB_ICONERROR);
        return FALSE;
    }

    /* Loads key accelerators */
    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_REACTOS_SNDREC32));

    /* Starts main loop */
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (wout_first)
    {
        AUD_waveout.close();
    }

    if (win_first)
    {
        AUD_wavein.close();
    }

    AUD_buffer.clear();

    return (int)msg.wParam;
}

ATOM
MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SNDREC32));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SNDREC32));

    return RegisterClassEx(&wcex);
}

BOOL
InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

    hInst = hInstance;

    hWnd = CreateWindow(szWindowClass,
                        szTitle,
                        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        MAINWINDOW_W,
                        MAINWINDOW_H,
                        NULL,
                        NULL,
                        hInstance,
                        NULL);
    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    main_win = hWnd;

    return TRUE;
}

ATOM
MyRegisterClass_wave(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc_wave;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = 0;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = 0;
    wcex.lpszClassName = TEXT("sndrec32_wave");
    wcex.hIconSm = 0;

    return RegisterClassEx(&wcex);
}

BOOL
InitInstance_wave(HWND f,
                  HINSTANCE hInstance,
                  int nCmdShow)
{
    HWND hWnd;

    hInst = hInstance;

    hWnd = CreateWindow(TEXT("sndrec32_wave"),
                        TEXT(""),
                        WS_DLGFRAME | WS_VISIBLE | WS_CHILD,
                        WAVEBAR_X,
                        WAVEBAR_Y,
                        WAVEBAR_CX,
                        WAVEBAR_CY,
                        f,
                        (HMENU)8,
                        hInstance,
                        0);

    if (!hWnd )
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    wave_win = hWnd;

    return TRUE;
}

LRESULT
CALLBACK
WndProc_wave(HWND hWnd,
             UINT message,
             WPARAM wParam,
             LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    HPEN pen;
    HPEN oldpen;

    unsigned int max_h = (cli.bottom / 2);
    unsigned int samples;
    unsigned int x, line_h;

    switch (message)
    {
        case WM_CREATE:
            GetClientRect(hWnd, &cli);
            break;
        case WM_PAINT:
            /* Initialize hdc objects */
            hdc = BeginPaint(hWnd, &ps);
            pen = (HPEN)CreatePen(PS_SOLID, 1, WAVEBAR_COLOR);
            oldpen = (HPEN) SelectObject(hdc, (HBRUSH)pen);
            if (AUD_OUT->current_status() == snd::WAVEOUT_PLAYING)
            {
                samples = AUD_OUT->tot_samples_buf();
                for (unsigned int i = 0; i < WAVEBAR_CX; ++i)
                {
                    x = (i * samples) / WAVEBAR_CX;
                    line_h = (AUD_OUT->nsample(x) * max_h) / AUD_OUT->samplevalue_max();
                    if (line_h)
                    {
                        MoveToEx(hdc, i, max_h, 0);
                        LineTo(hdc, i, max_h - (line_h * 2));
                        LineTo(hdc, i, max_h + (line_h * 2));
                    }
                    else
                    {
                        SetPixel(hdc, i, max_h, WAVEBAR_COLOR);
                    }
                }
            }
            else if (AUD_IN->current_status() == snd::WAVEIN_RECORDING)
            {
                samples = AUD_IN->tot_samples_buf();
                for (unsigned int i = 0; i < WAVEBAR_CX; ++i)
                {
                    x = (i * samples) / WAVEBAR_CX;
                    line_h = (AUD_IN->nsample(x) * max_h) / AUD_IN->samplevalue_max();
                    if (line_h)
                    {
                        MoveToEx(hdc, i, max_h, 0);
                        LineTo(hdc, i, max_h - (line_h * 2));
                        LineTo(hdc, i, max_h + (line_h * 2));
                    }
                    else
                    {
                        SetPixel( hdc, i, max_h, WAVEBAR_COLOR );
                    }
                }
            }
            else
            {
                /* In standby mode draw a simple line */
                MoveToEx(hdc, 0, cli.bottom / 2, 0);
                LineTo(hdc, WAVEBAR_CX, cli.bottom  / 2);
            }

            SelectObject(hdc, oldpen);
            DeleteObject( pen );
            EndPaint( hWnd, &ps );
            break;

        case WM_USER:
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

LRESULT
CALLBACK
WndProc(HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
{
    int wmId;
    TCHAR str_tmp[MAX_LOADSTRING];
    PAINTSTRUCT ps;
    HDC hdc;
    HFONT font;
    HFONT oldfont;
    long long slid_samp = 0;

    /* Checking for global pointers to buffer and io audio devices */
    if ((!AUD_IN) || (!AUD_OUT) || (!AUD_BUF))
    {
        MessageBox(0, TEXT("Buffer Error"), 0, 0);
        return 1;
    }

    switch (message)
    {
        case WM_CREATE:
            /* Creating the wave bar */
            if (!InitInstance_wave(hWnd, hInst, SW_SHOWNORMAL))
            {
                MessageBox(0, TEXT("CreateWindow() Error!"), TEXT("ERROR"), MB_ICONERROR);
                return FALSE;
            }

            /* Creating ALL the buttons */
            for (int i = 0; i < 5; ++i)
            {
                buttons[i] = CreateWindow(TEXT("button"),
                                          TEXT(""),
                                          WS_CHILD | WS_VISIBLE | BS_BITMAP,
                                          BUTTONS_CX + (i * (BUTTONS_W + ((i == 0) ? 0 : BUTTONS_SPACE))),
                                          BUTTONS_CY,
                                          BUTTONS_W,
                                          BUTTONS_H,
                                          hWnd,
                                          (HMENU)i,
                                          hInst,
                                          0);
                if (!buttons[i])
                {
                    MessageBox(0, 0, TEXT("CreateWindow() Error!"), 0);
                    return FALSE;
                }

                /* Realize the button bmp image */
                SendMessage(buttons[i], BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)butbmps[i]);
                UpdateWindow(buttons[i]);
                disable_but(i);
            }

            /* Creating the SLIDER window */
            slider = CreateWindow(TRACKBAR_CLASS,
                                  TEXT(""),
                                  WS_CHILD | WS_VISIBLE | TBS_NOTICKS | TBS_HORZ | TBS_ENABLESELRANGE,
                                  SLIDER_CX,
                                  SLIDER_CY,
                                  SLIDER_W,
                                  SLIDER_H,
                                  hWnd,
                                  (HMENU)SLIDER_ID,
                                  hInst,
                                  0);
            if (!slider)
            {
                MessageBox(0, 0, TEXT( "CreateWindow() Error!" ), 0);
                return FALSE;
            }

            /* Sets slider limits */
            SendMessage(slider,
                        TBM_SETRANGE,
                        (WPARAM)TRUE,
                        (LPARAM)MAKELONG(slider_min, slider_max));

            UpdateWindow(slider);
            enable_but(BUTREC_ID);
            EnableWindow(slider, FALSE);
            break;

        /* Implements slider logic */
        case WM_HSCROLL:
        {
            switch (LOWORD(wParam))
            {
                case SB_ENDSCROLL:
                    break;
                case SB_PAGERIGHT:
                case SB_PAGELEFT:
                case TB_THUMBTRACK:
                    /* If the user touch the slider bar, set the
                       audio start position properly */
                    slider_pos = SendMessage(slider, TBM_GETPOS, 0, 0);
                    slid_samp = (__int64)slider_pos * (__int64)samples_max;
                    AUD_BUF->set_position(AUD_BUF->audinfo().bytes_in_samples((unsigned int)(slid_samp / (__int64)slider_max)));
                    InvalidateRect(hWnd, &text_rect, TRUE);
                    break;
            }
            break;
        }

        case WM_COMMAND:
            wmId = LOWORD(wParam);
            if ((wmId >= 0) && (wmId < 5) && (butdisabled[wmId] == TRUE))
                break;

            switch (wmId)
            {
                case ID_FILE_NEW:
                    if (!isnew)
                    {
                        if (AUD_IN->current_status() == snd::WAVEIN_RECORDING)
                            AUD_IN->stop_recording();

                        if ((AUD_OUT->current_status() == snd::WAVEOUT_PLAYING) ||
                            (AUD_OUT->current_status() == snd::WAVEOUT_PAUSED))
                            AUD_OUT->stop();

                        AUD_BUF->reset();

                        enable_but(BUTREC_ID);
                        disable_but(BUTSTART_ID);
                        disable_but(BUTEND_ID);
                        disable_but(BUTSTOP_ID);
                        disable_but(BUTPLAY_ID);

                        samples_max = AUD_BUF->total_samples();
                        slider_pos = 0;

                        SendMessage(slider, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) slider_pos);

                        EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVEAS, MF_GRAYED);
                        EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVE, MF_GRAYED);

                        isnew = TRUE;
                        display_dur = TRUE;

                        ZeroMemory(file_path, MAX_PATH * sizeof(TCHAR));

                        EnableWindow(slider, FALSE);

                        InvalidateRect(hWnd, &text_rect, TRUE);
                        InvalidateRect(hWnd, &text2_rect, TRUE);
                    }
                    break;

                case ID_FILE_OPEN:
                    ZeroMemory(&ofn, sizeof(ofn));

                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFilter = TEXT("Audio Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0");
                    ofn.lpstrFile = file_path;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                    ofn.lpstrDefExt = TEXT("wav");

                    if (GetOpenFileName(&ofn))
                    {
                        open_wav(file_path);
                        EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVE, MF_ENABLED);
                        EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVEAS, MF_ENABLED);

                        EnableWindow(slider, TRUE);
                    }

                    InvalidateRect(hWnd, &text_rect, TRUE);
                    InvalidateRect(hWnd, &text2_rect, TRUE);
                    break;

                case ID_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutDlgProc);
                    return TRUE;
                    break;

                case ID_FILE_SAVEAS:
                    ZeroMemory(&ofn, sizeof(ofn));

                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hWnd ;
                    ofn.Flags = OFN_OVERWRITEPROMPT;
                    ofn.lpstrFilter = TEXT("Audio Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0");
                    ofn.lpstrFile = file_path;
                    ofn.nMaxFile = MAX_PATH;

                    ofn.lpstrDefExt = TEXT("wav");

                    if (GetSaveFileName (&ofn))
                    {
                        write_wav(file_path);
                        EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVE, MF_ENABLED);
                    }
                    break;


                case ID_EDIT_AUDIOPROPS:
                    ShellExecute(NULL, NULL, _T("rundll32.exe"), _T("shell32.dll,Control_RunDLL mmsys.cpl,ShowAudioPropertySheet"), NULL, SW_SHOWNORMAL);
                    break;

                case ID_FILE_EXIT:
                    DestroyWindow(hWnd);
                    break;

                /* Sndrec32 buttons routines */
                case BUTSTART_ID:
                    AUD_BUF->set_position_start();
                    slider_pos = 0;
                    SendMessage(slider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)slider_pos);
                    break;

                case BUTEND_ID:
                    DestroyWindow(hWnd);
                    break;

                case BUTPLAY_ID:
                    if (wout_first == false)
                    {
                        AUD_OUT->open();
                        wout_first = true;
                    }

                    AUD_OUT->play();

                    disable_but(BUTSTART_ID);
                    disable_but(BUTEND_ID);
                    disable_but(BUTREC_ID);
                    disable_but(BUTPLAY_ID);

                    SetTimer(hWnd, 1, 250, 0);
                    SetTimer(hWnd, WAVEBAR_TIMERID, WAVEBAR_TIMERTIME, 0);

                    break;

                case BUTSTOP_ID:
                    if (s_recording)
                    {
                        s_recording = FALSE;

                        AUD_IN->stop_recording();

                        /* Resetting slider position */
                        slider_pos = 0;
                        SendMessage(slider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)slider_pos);

                        samples_max = AUD_BUF->samples_received();

                        EnableMenuItem((HMENU)IDR_MENU1, ID_FILE_SAVEAS, MF_ENABLED);

                        enable_but(BUTSTART_ID);
                        enable_but(BUTEND_ID);
                        enable_but(BUTREC_ID);
                        enable_but(BUTPLAY_ID);

                        EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVEAS, MF_ENABLED);
                        EnableWindow(slider, TRUE);

                        display_dur = FALSE;

                        AUD_BUF->truncate();

                        InvalidateRect(hWnd, &text_rect, TRUE);
                        InvalidateRect(wave_win, 0, TRUE);

                    }
                    else
                    {
                        AUD_OUT->pause();

                        enable_but(BUTSTART_ID);
                        enable_but(BUTEND_ID);
                        enable_but(BUTREC_ID);
                        enable_but(BUTPLAY_ID);

                    }

                    KillTimer(hWnd, 1);
                    KillTimer(hWnd, WAVEBAR_TIMERID);

                    InvalidateRect(hWnd, &text_rect, TRUE);

                    break;

                case BUTREC_ID:
                    if (win_first == false)
                    {
                        AUD_IN->open();
                        win_first = true;
                    }

                    s_recording = TRUE;

                    samples_max = AUD_BUF->total_samples();

                    AUD_IN->start_recording();

                    enable_but(BUTSTOP_ID);

                    disable_but(BUTSTART_ID);
                    disable_but(BUTEND_ID);
                    disable_but(BUTREC_ID);
                    disable_but(BUTPLAY_ID);

                    isnew = FALSE;

                    EnableWindow(slider, FALSE);

                    SetTimer(hWnd, 1, 150, 0);
                    SetTimer(hWnd, WAVEBAR_TIMERID, WAVEBAR_TIMERTIME, 0);

                    break;

                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;

        case WM_TIMER:
            switch (wParam)
            {
                case 1:
                    if (stopped_flag)
                    {
                        KillTimer(hWnd, 1);
                        KillTimer(hWnd, WAVEBAR_TIMERID);
                        slider_pos = 0;
                        enable_but(BUTPLAY_ID);
                        stopped_flag = FALSE;
                    }

                    SendMessage(slider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)slider_pos);
                    InvalidateRect(hWnd, &text_rect, TRUE);
                    break;

                case WAVEBAR_TIMERID:
                    InvalidateRect(wave_win, 0, TRUE);
                    SendMessage(wave_win, WM_USER, 0, 0);
                    break;
            }
            break;

        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            font = CreateFontIndirect(&s_info.lfMenuFont);
            oldfont = (HFONT) SelectObject(hdc, font);
            SetBkMode(hdc, TRANSPARENT);

            if (AUD_IN->current_status() == snd::WAVEIN_RECORDING)
            {
                gprintf(str_tmp,
                        MAX_LOADSTRING,
                        str_pos,
                        (float)((float)AUD_BUF->bytes_recorded() / (float)AUD_BUF->audinfo().byte_rate()));

            }
            else if (AUD_OUT->current_status() == snd::WAVEOUT_PLAYING)
            {
                gprintf(str_tmp,
                        MAX_LOADSTRING,
                        str_pos,
                        (float)((float)AUD_BUF->bytes_played() / (float)AUD_BUF->audinfo().byte_rate()));
            }
            else
            {
                gprintf(str_tmp,
                        MAX_LOADSTRING,
                        str_pos,
                        (float)((((float)slider_pos * (float)samples_max) / (float)slider_max) / (float)AUD_BUF->audinfo().sample_rate()));
            }

            ExtTextOut(hdc,
                       STRPOS_X,
                       STRPOS_Y,
                       ETO_OPAQUE,
                       0,
                       str_tmp,
                       _tcslen(str_tmp),
                       0);

            if (display_dur)
            {
                gprintf(str_tmp,
                        MAX_LOADSTRING,
                        str_dur,
                        AUD_BUF->fseconds_total());
            }
            else
            {
                gprintf(str_tmp,
                        MAX_LOADSTRING,
                        str_dur,
                        AUD_BUF->fseconds_recorded());
            }

            ExtTextOut(hdc,
                       STRDUR_X,
                       STRDUR_Y,
                       ETO_OPAQUE,
                       0,
                       str_tmp,
                       _tcslen(str_tmp),
                       0);

            gprintf(str_tmp,
                    MAX_LOADSTRING,
                    str_buf,
                    (float)((float)AUD_BUF->mem_size() / 1024.0f));

            ExtTextOut(hdc,
                       STRBUF_X,
                       STRBUF_Y,
                       ETO_OPAQUE,
                       0,
                       str_tmp,
                       _tcslen(str_tmp),
                       0);

            gprintf(str_tmp,
                    MAX_LOADSTRING,
                    str_fmt,
                    (float)((float)AUD_BUF->audinfo().sample_rate() / 1000.0f),
                    AUD_BUF->audinfo().bits(),
                    AUD_BUF->audinfo().channels() == 2 ? str_mono : str_stereo);

            ExtTextOut(hdc,
                       STRFMT_X,
                       STRFMT_Y,
                       ETO_OPAQUE,
                       0,
                       str_tmp,
                       _tcslen(str_tmp),
                       0);

            gprintf(str_tmp,
                    MAX_LOADSTRING,
                    str_chan,
                    AUD_BUF->audinfo().channels() == 2 ? str_stereo : str_mono);

            ExtTextOut(hdc,
                       STRCHAN_X,
                       STRCHAN_Y,
                       ETO_OPAQUE,
                       0,
                       str_tmp,
                       _tcslen(str_tmp),
                       0);

            SelectObject(hdc, oldfont);
            DeleteObject(font);
            EndPaint(hWnd, &ps);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

void l_play_finished(void)
{
    stopped_flag = true;

    enable_but(BUTSTART_ID);
    enable_but(BUTEND_ID);
    enable_but(BUTREC_ID);
    enable_but(BUTPLAY_ID);

    InvalidateRect(wave_win, 0, TRUE);
}

void l_audio_arrival(unsigned int samples_arrival)
{
    slider_pos += (DWORD)((slider_max * samples_arrival) / samples_max);
}

void l_buffer_resized(unsigned int new_size)
{
}

VOID enable_but(DWORD id)
{
    butdisabled[id] = FALSE;
    SendMessage(buttons[id], BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)butbmps[id]);
}

VOID disable_but(DWORD id)
{
    butdisabled[id] = TRUE;
    SendMessage(buttons[id], BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)butbmps_dis[id]);
}

BOOL open_wav(TCHAR *f)
{
    HANDLE file;

    riff_hdr r;
    wave_hdr w;
    data_chunk d;

    BOOL b;

    DWORD bytes_recorded_in_wav = 0;
    DWORD is_read = 0;

    file = CreateFile(f,
                      GENERIC_READ,
                      0,
                      0,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      0);
    if (!file)
    {
        MessageBox(main_win,
                   TEXT("Cannot open file. CreateFile() error."),
                   TEXT("ERROR"),
                   MB_OK | MB_ICONERROR);

        return FALSE;
    }

    b = ReadFile(file, (LPVOID)&r, sizeof(r), &is_read, 0);
    if (!b)
    {
        MessageBox(main_win,
                   TEXT("Cannot read RIFF header."),
                   TEXT("ERROR"),
                   MB_OK | MB_ICONERROR);

        CloseHandle(file);
        return FALSE;
    }

    b = ReadFile(file, (LPVOID)&w, sizeof(w), &is_read, 0);
    if (!b)
    {
        MessageBox(main_win,
                   TEXT("Cannot read WAVE header."),
                   TEXT("ERROR"),
                   MB_OK | MB_ICONERROR);

        CloseHandle(file);
        return FALSE;
    }

    b = ReadFile(file, (LPVOID)&d, sizeof(d), &is_read, 0);
    if (!b)
    {
        MessageBox(main_win,
                   TEXT("Cannot read WAVE subchunk."),
                   TEXT("ERROR"),
                   MB_OK | MB_ICONERROR);

        CloseHandle(file);
        return FALSE;
    }

    bytes_recorded_in_wav = r.chunksize - 36;
    if (bytes_recorded_in_wav == 0)
    {
        MessageBox(main_win,
                   TEXT("Cannot read file. No audio data."),
                   TEXT("ERROR"),
                   MB_OK | MB_ICONERROR);

        CloseHandle(file);
        return FALSE;
    }

    snd::audio_format openfmt(w.SampleRate, w.BitsPerSample, w.NumChannels);

    AUD_BUF->clear();
    AUD_BUF->alloc_bytes(bytes_recorded_in_wav);

    b = ReadFile(file,
                 (LPVOID)AUD_BUF->audio_buffer(),
                 bytes_recorded_in_wav,
                 &is_read,
                 0);

    AUD_BUF->set_b_received(bytes_recorded_in_wav);

    if ((!b) || (is_read != bytes_recorded_in_wav))
    {
        MessageBox(main_win,
                   TEXT("Cannot read file. Error reading audio data."),
                   TEXT("ERROR"),
                   MB_OK | MB_ICONERROR);

        CloseHandle(file);

        AUD_BUF->reset();
        return FALSE;
    }

    CloseHandle(file);

    enable_but(BUTPLAY_ID);
    enable_but(BUTSTOP_ID);
    enable_but(BUTSTART_ID);
    enable_but(BUTEND_ID);
    enable_but(BUTREC_ID);

    samples_max = AUD_BUF->samples_received();

    isnew = FALSE;

    return TRUE;
}

BOOL
write_wav(TCHAR *f)
{
    HANDLE file;
    DWORD written;
    BOOL is_writ;
    int i;
    riff_hdr r;
    wave_hdr w;
    data_chunk d;

    file = CreateFile(f, GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
    if (!file)
    {
        i = MessageBox(main_win,
                       TEXT("File already exist. Overwrite it?"),
                       TEXT("Warning"),
                       MB_YESNO | MB_ICONQUESTION);

        if (i == IDYES)
        {
            file = CreateFile(f, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
            if (!file)
            {
                MessageBox(main_win,
                           TEXT("File Error, CreateFile() failed."),
                           TEXT("ERROR"),
                           MB_OK | MB_ICONERROR);

                return FALSE;
            }

        } else
            return FALSE;
    }

    r.magic = 0x46464952;
    r.format = 0x45564157;
    r.chunksize = 36 + AUD_BUF->bytes_recorded();

    w.Subchunkid = 0x20746d66;
    w.Subchunk1Size = 16;
    w.AudioFormat = 1;
    w.NumChannels = AUD_BUF->audinfo().channels();
    w.SampleRate = AUD_BUF->audinfo().sample_rate();
    w.ByteRate = AUD_BUF->audinfo().byte_rate();
    w.BlockAlign = AUD_BUF->audinfo().block_align();
    w.BitsPerSample = AUD_BUF->audinfo().bits();

    d.subc = 0x61746164;
    d.subc_size = AUD_BUF->bytes_recorded();

    /* Writing headers */
    is_writ = WriteFile(file, (LPCVOID)&r, sizeof(r), &written, 0);
    if (!is_writ)
    {
        MessageBox(main_win,
                   TEXT("File Error, WriteFile() failed."),
                   TEXT("ERROR"),
                   MB_OK | MB_ICONERROR);

        CloseHandle(file);
        return FALSE;
    }

    is_writ = WriteFile(file, (LPCVOID)&w, sizeof(w), &written, 0);
    if (!is_writ)
    {
        MessageBox(main_win,
                   TEXT("File Error, WriteFile() failed."),
                   TEXT("ERROR"),
                   MB_OK | MB_ICONERROR);

        CloseHandle(file);
        return FALSE;
    }

    is_writ = WriteFile(file, (LPCVOID)&d, sizeof(d), &written, 0);
    if (!is_writ)
    {
        MessageBox(main_win,
                   TEXT("File Error, WriteFile() failed."),
                   TEXT("ERROR"),
                   MB_OK | MB_ICONERROR);

        CloseHandle(file);
        return FALSE;
    }

    is_writ = WriteFile(file,
                        (LPCVOID)AUD_BUF->audio_buffer(),
                        AUD_BUF->bytes_recorded(),
                        &written,
                        0);
    if (!is_writ)
    {
        MessageBox(main_win,
                   TEXT("File Error, WriteFile() failed."),
                   TEXT("ERROR"),
                   MB_OK | MB_ICONERROR);

        CloseHandle(file);
        return FALSE;
    }

    CloseHandle(file);
    return TRUE;
}
