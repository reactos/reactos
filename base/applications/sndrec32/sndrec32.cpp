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
#include <winnls.h>
#include <uxtheme.h>
#include <vssym32.h>

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
HTHEME buttontheme[5];
LPCTSTR buttxts[5];
HFONT hfButtonCaption;
WNDPROC buttons_std_proc;

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

    switch (GetUserDefaultUILanguage())
    {
        case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
            SetProcessDefaultLayout(LAYOUT_RTL);
            break;

        default:
            break;
    }

    win_first = wout_first = FALSE;

    text_rect.left = REFRESHA_X;
    text_rect.top = REFRESHA_Y;
    text_rect.right = REFRESHA_CX;
    text_rect.bottom = REFRESHA_CY;

    text2_rect.left = REFRESHB_X;
    text2_rect.top = REFRESHB_Y;
    text2_rect.right = REFRESHB_CX;
    text2_rect.bottom = REFRESHB_CY;

    /* Retrieving default system font, and others system informations */
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
                         sizeof(NONCLIENTMETRICS),
                         &s_info,
                         0);

    /* Set font size */
    s_info.lfMenuFont.lfHeight = 14;

    /* get the language layout */
    DWORD layout;
    GetProcessDefaultLayout(&layout);

    /* Init button texts, exchange rewind and forward in case we use an RTL layout*/
    if (layout == LAYOUT_RTL)
    {
        buttxts[BUTSTART_ID] = TEXT("44");  // rewind
        buttxts[BUTEND_ID] = TEXT("33");    // forward
        buttxts[BUTPLAY_ID] = TEXT("3");        // play
    }
    else
    {
        buttxts[BUTSTART_ID] = TEXT("33");  // rewind
        buttxts[BUTEND_ID] = TEXT("44");    // forward
        buttxts[BUTPLAY_ID] = TEXT("4");        // play
    }
    buttxts[BUTSTOP_ID] = TEXT("g");        // stop
    buttxts[BUTREC_ID] = TEXT("n");         // record

    /* Create a Marlett Font */
    hfButtonCaption = CreateFont(0, 0, 0, 0,
                                 FW_BOLD,
                                 FALSE,
                                 FALSE,
                                 FALSE,
                                 SYMBOL_CHARSET,
                                 OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS,
                                 DEFAULT_QUALITY,
                                 DEFAULT_PITCH | FF_DECORATIVE,
                                 TEXT("Marlett"));

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
        if (!TranslateAccelerator(main_win, hAccelTable, &msg))
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

    hWnd = CreateWindowEx(WS_EX_STATICEDGE,
                          TEXT("sndrec32_wave"),
                          TEXT(""),
                          WS_VISIBLE | WS_CHILD,
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
    WCHAR szAppName[100];
    HICON hIcon;

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
                MessageBox(0, TEXT("InitInstance_wave() Error!"), TEXT("ERROR"), MB_ICONERROR);
                return FALSE;
            }

            /* Create the audio control buttons, associate a theme handle and use the Marlett font on them */
            for (UINT i = 0; i < _countof(buttons); ++i)
            {
                buttons[i] = CreateWindow(TEXT("button"),
                                          buttxts[i],
                                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_CENTER | BS_TEXT | BS_OWNERDRAW,
                                          BUTTONS_CX + (i * (BUTTONS_W + ((i == 0) ? 0 : BUTTONS_SPACE))),
                                          BUTTONS_CY,
                                          BUTTONS_W,
                                          BUTTONS_H,
                                          hWnd,
                                          (HMENU)UlongToPtr(i),
                                          hInst,
                                          0);
                if (!buttons[i])
                {
                    MessageBox(0, 0, TEXT("CreateWindow() Error!"), 0);
                    return FALSE;
                }

                buttontheme[i] = OpenThemeData(buttons[i], L"Button");

                /* Use Marlett font on this button */
                SendMessage(buttons[i], WM_SETFONT, (WPARAM)hfButtonCaption, (LPARAM)TRUE);
                UpdateWindow(buttons[i]);
                EnableWindow(GetDlgItem(hWnd, i), FALSE);
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
            EnableWindow(buttons[BUTREC_ID], TRUE);
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

                        EnableWindow(buttons[BUTREC_ID], TRUE);
                        EnableWindow(buttons[BUTSTART_ID], FALSE);
                        EnableWindow(buttons[BUTEND_ID], FALSE);
                        EnableWindow(buttons[BUTSTOP_ID], FALSE);
                        EnableWindow(buttons[BUTPLAY_ID], FALSE);

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
                    LoadStringW(hInst, IDS_APP_TITLE, szAppName, _countof(szAppName));
                    hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_REACTOS_SNDREC32));
                    ShellAboutW(hWnd, szAppName, NULL, hIcon);
                    DestroyIcon(hIcon);
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

                    EnableWindow(buttons[BUTSTART_ID], FALSE);
                    EnableWindow(buttons[BUTEND_ID], FALSE);
                    EnableWindow(buttons[BUTREC_ID], FALSE);
                    EnableWindow(buttons[BUTPLAY_ID], FALSE);

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

                        EnableWindow(buttons[BUTSTART_ID], TRUE);
                        EnableWindow(buttons[BUTEND_ID], TRUE);
                        EnableWindow(buttons[BUTREC_ID], TRUE);
                        EnableWindow(buttons[BUTPLAY_ID], TRUE);

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

                        EnableWindow(buttons[BUTSTART_ID], TRUE);
                        EnableWindow(buttons[BUTEND_ID], TRUE);
                        EnableWindow(buttons[BUTREC_ID], TRUE);
                        EnableWindow(buttons[BUTPLAY_ID], TRUE);

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

                    EnableWindow(buttons[BUTSTOP_ID], TRUE);
                    EnableWindow(buttons[BUTSTART_ID], FALSE);
                    EnableWindow(buttons[BUTEND_ID], FALSE);
                    EnableWindow(buttons[BUTREC_ID], FALSE);
                    EnableWindow(buttons[BUTPLAY_ID], FALSE);

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
                        EnableWindow(buttons[BUTPLAY_ID], TRUE);
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

        case WM_THEMECHANGED:
            switch ((UINT)wParam)
            {
                case BUTSTART_ID:
                case BUTEND_ID:
                case BUTPLAY_ID:
                case BUTSTOP_ID:
                case BUTREC_ID:
                {
                    CloseThemeData (buttontheme[(UINT)wParam]);
                    buttontheme[(UINT)wParam] = OpenThemeData (hWnd, L"Button");
                    return TRUE;
                }
                break;
            }
            break;

        case WM_DRAWITEM:
            switch ((UINT)wParam)
            {
                case BUTSTART_ID:
                case BUTEND_ID:
                case BUTPLAY_ID:
                case BUTSTOP_ID:
                case BUTREC_ID:
                {
                    DrawButton((UINT)wParam, lParam);
                    return TRUE;
                }
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
                       0,
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
                       0,
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
                       0,
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
                       0,
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
                       0,
                       0,
                       str_tmp,
                       _tcslen(str_tmp),
                       0);

            SelectObject(hdc, oldfont);
            DeleteObject(font);
            EndPaint(hWnd, &ps);
            break;

        case WM_DESTROY:
            if (hfButtonCaption)
            {
                DeleteObject(hfButtonCaption);
            }

            for (UINT i = 0; i < _countof(buttontheme); ++i)
            {
                if (buttontheme[i])
                {
                    CloseThemeData(buttontheme[i]);
                }
            }
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

void DrawButton(UINT ButtonID, LPARAM lParam)
{
    LPDRAWITEMSTRUCT lpdis = (DRAWITEMSTRUCT*)lParam;
    SIZE size;
    LRESULT ButtonStatus;
    UINT ButtonStyle;
    int lpdx[] = {8,8}, captionspacer = 0;
    COLORREF OriginalCol;

    /* Add a defined spacing of 8px for the rewind and forward text */
    if (ButtonID < BUTPLAY_ID)
    {
        captionspacer=8;
    }

    /* Make the color of the record button red and all others black */
    if (ButtonID == BUTREC_ID)
    {
        OriginalCol = SetTextColor(lpdis->hDC, RGB(255, 0, 0));
    }
    else
    {
        OriginalCol = SetTextColor(lpdis->hDC, RGB(0, 0, 0));
    }

    if ((ButtonID >= BUTSTART_ID) && (ButtonID <= BUTREC_ID))
    {
        ButtonStatus = SendMessage(buttons[ButtonID], BM_GETSTATE, NULL, NULL);
    }

    if(buttontheme[ButtonID])
    {
        ButtonStyle = PBS_NORMAL;
        if (ButtonStatus & BST_PUSHED)
        {
            ButtonStyle = PBS_PRESSED;
        }
        else if (ButtonStatus & BST_HOT)
        {
            ButtonStyle = PBS_HOT;
        }
        else if (!IsWindowEnabled(buttons[ButtonID]))
        {
            ButtonStyle = PBS_DISABLED;
            SetTextColor(lpdis->hDC, GetSysColor(COLOR_GRAYTEXT));
        }
        DrawThemeBackground(buttontheme[ButtonID], lpdis->hDC, BP_PUSHBUTTON, ButtonStyle, &lpdis->rcItem, 0);
    }
    else
    {
        ButtonStyle = DFCS_BUTTONPUSH;
        if (ButtonStatus & BST_PUSHED)
        {
            ButtonStyle |= DFCS_PUSHED;
        }
        else if (ButtonStatus & BST_HOT)
        {
            ButtonStyle |= DFCS_HOT;
        }
        else if (!IsWindowEnabled(buttons[ButtonID]))
        {
            ButtonStyle |= DFCS_INACTIVE;
            SetTextColor(lpdis->hDC, GetSysColor(COLOR_GRAYTEXT));
        }
        DrawFrameControl(lpdis->hDC, &lpdis->rcItem, DFC_BUTTON, ButtonStyle);
    }

    SetBkMode(lpdis->hDC, TRANSPARENT);
    GetTextExtentPoint32(lpdis->hDC, buttxts[ButtonID], _tcslen(buttxts[ButtonID]), &size);
    ExtTextOut(lpdis->hDC,
              ((lpdis->rcItem.right - lpdis->rcItem.left) - size.cx + captionspacer) / 2,
              ((lpdis->rcItem.bottom - lpdis->rcItem.top) - size.cy) / 2,
              ETO_CLIPPED,
              &lpdis->rcItem,
              buttxts[ButtonID],
              _tcslen(buttxts[ButtonID]),
              lpdx);


    if ((ButtonStatus & BST_FOCUS))
    {
        InflateRect(&lpdis->rcItem, -3, -3);
        DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
    }

    /* restore the original text color */
    SetTextColor(lpdis->hDC, OriginalCol);
}

void l_play_finished()
{
    stopped_flag = true;

    EnableWindow(buttons[BUTSTART_ID], TRUE);
    EnableWindow(buttons[BUTEND_ID], TRUE);
    EnableWindow(buttons[BUTREC_ID], TRUE);
    EnableWindow(buttons[BUTPLAY_ID], TRUE);

    InvalidateRect(wave_win, 0, TRUE);
}

void l_audio_arrival(unsigned int samples_arrival)
{
    slider_pos += (DWORD)((slider_max * samples_arrival) / samples_max);
}

void l_buffer_resized(unsigned int new_size)
{
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

    EnableWindow(buttons[BUTPLAY_ID], TRUE);
    EnableWindow(buttons[BUTSTOP_ID], TRUE);
    EnableWindow(buttons[BUTSTART_ID], TRUE);
    EnableWindow(buttons[BUTEND_ID], TRUE);
    EnableWindow(buttons[BUTREC_ID], TRUE);

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
