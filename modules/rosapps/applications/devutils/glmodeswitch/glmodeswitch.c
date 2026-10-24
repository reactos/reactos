/*
 * PROJECT:     glmodeswitch
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     OpenGL gears with fullscreen and display mode switching, for stress testing drivers
 * COPYRIGHT:   Gear geometry based on glxgears by Brian Paul
 *              Copyright 2026 Mark Jansen (mark.jansen@reactos.org)
 */

#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_MODES 16
#define TIMER_SWITCH 1

typedef struct _MODE
{
    DWORD Width, Height, Bpp;
} MODE;

static HWND g_hwnd;
static HDC g_hdc;
static HGLRC g_hglrc;
static GLuint g_gear1, g_gear2, g_gear3;
static GLuint g_FontBase;
static int g_CharW = 8, g_CharH = 16;
static BOOL g_ShowOverlay = TRUE;
static char g_Status[128];
static char g_Renderer[128];
static char g_Fps[32];

static const char *g_HelpLines[] =
{
    "F1 help  H overlay  F fullscreen  M/Space next mode",
    "R restore mode  A auto switch  Esc quit",
};

static MODE g_Modes[MAX_MODES];
static int g_nModes;
static int g_CurMode = -1; /* -1 = registry mode */
static BOOL g_Fullscreen;
static BOOL g_Restore = TRUE;
static BOOL g_ExitWhenDone;
static UINT g_Interval = 2000;
static BOOL g_AutoSwitch;
static int g_SwitchesLeft = -1; /* -1 = unlimited */
static DWORD g_SavedStyle;
static RECT g_SavedRect;

/* Sends msg to the debugger and the title bar */
static void Report(const char *msg)
{
    char buf[256];

    _snprintf(buf, sizeof(buf) - 1, "glmodeswitch: %s\n", msg);
    buf[sizeof(buf) - 1] = '\0';
    OutputDebugStringA(buf);
    SetWindowTextA(g_hwnd, msg);
}

/* Like Report, and also shows the message on the overlay status line */
static void Log(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    _vsnprintf(g_Status, sizeof(g_Status) - 1, fmt, args);
    va_end(args);

    Report(g_Status);
}

static void gear(GLfloat inner, GLfloat outer, GLfloat width, GLint teeth, GLfloat depth)
{
    GLint i;
    GLfloat r0 = inner, r1 = outer - depth / 2, r2 = outer + depth / 2;
    GLfloat angle, da = 2 * M_PI / teeth / 4, u, v, len;

    glShadeModel(GL_FLAT);
    glNormal3f(0, 0, 1);

    /* front face */
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++)
    {
        angle = i * 2 * M_PI / teeth;
        glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
        glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
        if (i < teeth)
        {
            glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
            glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
        }
    }
    glEnd();

    /* front sides of teeth */
    glBegin(GL_QUADS);
    for (i = 0; i < teeth; i++)
    {
        angle = i * 2 * M_PI / teeth;
        glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
        glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
        glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5);
        glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
    }
    glEnd();

    glNormal3f(0, 0, -1);

    /* back face */
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++)
    {
        angle = i * 2 * M_PI / teeth;
        glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
        glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
        if (i < teeth)
        {
            glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
            glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
        }
    }
    glEnd();

    /* back sides of teeth */
    glBegin(GL_QUADS);
    for (i = 0; i < teeth; i++)
    {
        angle = i * 2 * M_PI / teeth;
        glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
        glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5);
        glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
        glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
    }
    glEnd();

    /* outward faces of teeth */
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i < teeth; i++)
    {
        angle = i * 2 * M_PI / teeth;

        glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
        glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
        u = r2 * cos(angle + da) - r1 * cos(angle);
        v = r2 * sin(angle + da) - r1 * sin(angle);
        len = sqrt(u * u + v * v);
        u /= len;
        v /= len;
        glNormal3f(v, -u, 0);
        glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
        glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
        glNormal3f(cos(angle), sin(angle), 0);
        glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5);
        glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5);
        u = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
        v = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
        glNormal3f(v, -u, 0);
        glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
        glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
        glNormal3f(cos(angle), sin(angle), 0);
    }
    glVertex3f(r1, 0, width * 0.5);
    glVertex3f(r1, 0, -width * 0.5);
    glEnd();

    glShadeModel(GL_SMOOTH);

    /* inside radius cylinder */
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++)
    {
        angle = i * 2 * M_PI / teeth;
        glNormal3f(-cos(angle), -sin(angle), 0);
        glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
        glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
    }
    glEnd();
}

static GLuint MakeGear(const GLfloat *color, GLfloat inner, GLfloat outer, GLfloat width, GLint teeth)
{
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
    gear(inner, outer, width, teeth, 0.7f);
    glEndList();
    return list;
}

static void InitGL(void)
{
    static const GLfloat pos[4] = { 5.0f, 5.0f, 10.0f, 0.0f };
    static const GLfloat red[4] = { 0.8f, 0.1f, 0.0f, 1.0f };
    static const GLfloat green[4] = { 0.0f, 0.8f, 0.2f, 1.0f };
    static const GLfloat blue[4] = { 0.2f, 0.2f, 1.0f, 1.0f };
    TEXTMETRICA tm;

    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    g_gear1 = MakeGear(red, 1.0f, 4.0f, 1.0f, 20);
    g_gear2 = MakeGear(green, 0.5f, 2.0f, 2.0f, 10);
    g_gear3 = MakeGear(blue, 1.3f, 2.0f, 0.5f, 10);

    SelectObject(g_hdc, GetStockObject(SYSTEM_FIXED_FONT));
    GetTextMetricsA(g_hdc, &tm);
    g_CharW = tm.tmAveCharWidth;
    g_CharH = tm.tmHeight;
    g_FontBase = glGenLists(96);
    wglUseFontBitmapsA(g_hdc, 32, 96, g_FontBase);
}

/* Draws text below *y, wrapping at spaces (or hard, for long words) to fit cols */
static void DrawWrapped(int *y, const char *text, int cols)
{
    size_t len = strlen(text);
    int n, brk;

    while (len)
    {
        n = (int)min(len, (size_t)cols);
        if ((size_t)n < len)
        {
            for (brk = n; brk > 0 && text[brk] != ' '; brk--)
                ;
            if (brk > 0)
                n = brk;
        }

        *y -= g_CharH;
        glRasterPos2i(4, *y);
        glCallLists(n, GL_UNSIGNED_BYTE, text);

        text += n;
        len -= n;
        while (len && *text == ' ')
        {
            text++;
            len--;
        }
    }
}

static void DrawOverlay(void)
{
    RECT rc;
    int i, y, cols;

    if (!g_ShowOverlay)
        return;

    GetClientRect(g_hwnd, &rc);

    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, rc.right, 0, rc.bottom, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);
    glListBase(g_FontBase - 32);
    cols = max(1, (rc.right - 8) / g_CharW);
    y = rc.bottom;
    DrawWrapped(&y, g_Status, cols);
    DrawWrapped(&y, g_Renderer, cols);
    DrawWrapped(&y, g_Fps, cols);
    for (i = 0; i < ARRAYSIZE(g_HelpLines); i++)
        DrawWrapped(&y, g_HelpLines[i], cols);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

static void Reshape(int width, int height)
{
    GLfloat h = width ? (GLfloat)height / width : 1.0f;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -h, h, 5.0, 60.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -40.0f);
}

/* Updates the FPS text once a second, in the overlay and the title bar */
static void CountFrame(void)
{
    static DWORD start, frames;
    DWORD now = GetTickCount();

    frames++;
    if (now - start < 1000)
        return;

    _snprintf(g_Fps, sizeof(g_Fps) - 1, "%lu fps", frames * 1000 / (now - start));
    SetWindowTextA(g_hwnd, g_Fps);
    start = now;
    frames = 0;
}

static void Draw(void)
{
    /* Frame based like glxgears, so the speed reflects the frame rate */
    static GLfloat angle;

    angle = fmodf(angle + 2.0f, 360.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glRotatef(20.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(30.0f, 0.0f, 1.0f, 0.0f);

    glPushMatrix();
    glTranslatef(-3.0f, -2.0f, 0.0f);
    glRotatef(angle, 0.0f, 0.0f, 1.0f);
    glCallList(g_gear1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(3.1f, -2.0f, 0.0f);
    glRotatef(-2.0f * angle - 9.0f, 0.0f, 0.0f, 1.0f);
    glCallList(g_gear2);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-3.1f, 4.2f, 0.0f);
    glRotatef(-2.0f * angle - 25.0f, 0.0f, 0.0f, 1.0f);
    glCallList(g_gear3);
    glPopMatrix();

    glPopMatrix();

    DrawOverlay();
    SwapBuffers(g_hdc);
    CountFrame();
}

static void FitToScreen(void)
{
    SetWindowPos(g_hwnd, HWND_TOPMOST, 0, 0,
                 GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

static void SetFullscreen(BOOL bFullscreen)
{
    if (bFullscreen == g_Fullscreen)
        return;

    if (bFullscreen)
    {
        g_SavedStyle = GetWindowLongW(g_hwnd, GWL_STYLE);
        GetWindowRect(g_hwnd, &g_SavedRect);
        SetWindowLongW(g_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        g_Fullscreen = TRUE;
        FitToScreen();
    }
    else
    {
        SetWindowLongW(g_hwnd, GWL_STYLE, g_SavedStyle);
        SetWindowPos(g_hwnd, HWND_NOTOPMOST, g_SavedRect.left, g_SavedRect.top,
                     g_SavedRect.right - g_SavedRect.left, g_SavedRect.bottom - g_SavedRect.top,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        g_Fullscreen = FALSE;
    }
}

static const char *DispChangeName(LONG ret)
{
    switch (ret)
    {
    case DISP_CHANGE_SUCCESSFUL: return "DISP_CHANGE_SUCCESSFUL";
    case DISP_CHANGE_RESTART: return "DISP_CHANGE_RESTART";
    case DISP_CHANGE_FAILED: return "DISP_CHANGE_FAILED";
    case DISP_CHANGE_BADMODE: return "DISP_CHANGE_BADMODE";
    case DISP_CHANGE_NOTUPDATED: return "DISP_CHANGE_NOTUPDATED";
    case DISP_CHANGE_BADFLAGS: return "DISP_CHANGE_BADFLAGS";
    case DISP_CHANGE_BADPARAM: return "DISP_CHANGE_BADPARAM";
    default: return "?";
    }
}

static void SwitchMode(int index)
{
    DEVMODEW dm;
    LONG ret;
    char mode[32];

    if (index < 0)
    {
        ret = ChangeDisplaySettingsW(NULL, 0);
        Log("Registry mode: %s (%ld)", DispChangeName(ret), ret);
    }
    else
    {
        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        dm.dmPelsWidth = g_Modes[index].Width;
        dm.dmPelsHeight = g_Modes[index].Height;
        if (g_Modes[index].Bpp)
        {
            dm.dmFields |= DM_BITSPERPEL;
            dm.dmBitsPerPel = g_Modes[index].Bpp;
        }
        ret = ChangeDisplaySettingsW(&dm, CDS_FULLSCREEN);

        if (dm.dmFields & DM_BITSPERPEL)
            _snprintf(mode, sizeof(mode) - 1, "%lux%lux%lu", dm.dmPelsWidth, dm.dmPelsHeight, dm.dmBitsPerPel);
        else
            _snprintf(mode, sizeof(mode) - 1, "%lux%lu", dm.dmPelsWidth, dm.dmPelsHeight);
        mode[sizeof(mode) - 1] = '\0';
        Log("%s: %s (%ld)", mode, DispChangeName(ret), ret);
    }
    g_CurMode = index;

    if (g_Fullscreen)
        FitToScreen();
}

static void NextMode(void)
{
    SwitchMode((g_CurMode + 1) % g_nModes);
}

static void Usage(void)
{
    MessageBoxW(g_hwnd,
                L"glmodeswitch [options] [WxH[xBPP] ...]\n\n"
                L"-f\t\tStart fullscreen\n"
                L"-i <ms>\t\tSwitch to the next mode every <ms>\n"
                L"-n <count>\tStop after <count> switches\n"
                L"-x\t\tExit after the last switch\n"
                L"-k\t\tKeep the last mode on exit\n\n"
                L"Default modes: 640x480 800x600 1024x768\n\n"
                L"Keys: F1 help, H toggle overlay, F fullscreen,\n"
                L"M/Space next mode, R restore mode,\n"
                L"A toggle auto switching, Esc quit",
                L"glmodeswitch", MB_OK | MB_TOPMOST);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        Reshape(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_TIMER:
        if (g_SwitchesLeft == 0)
        {
            KillTimer(hwnd, TIMER_SWITCH);
            if (g_ExitWhenDone)
                DestroyWindow(hwnd);
            return 0;
        }
        if (g_SwitchesLeft > 0)
            g_SwitchesLeft--;
        NextMode();
        return 0;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
            DestroyWindow(hwnd);
            break;
        case VK_F1:
            Usage();
            break;
        case 'H':
            g_ShowOverlay = !g_ShowOverlay;
            break;
        case 'F':
            SetFullscreen(!g_Fullscreen);
            break;
        case 'M':
        case VK_SPACE:
            NextMode();
            break;
        case 'R':
            SwitchMode(-1);
            break;
        case 'A':
            g_AutoSwitch = !g_AutoSwitch;
            if (g_AutoSwitch)
                SetTimer(hwnd, TIMER_SWITCH, g_Interval, NULL);
            else
                KillTimer(hwnd, TIMER_SWITCH);
            break;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static BOOL ParseArgs(void)
{
    int i;
    MODE m;

    for (i = 1; i < __argc; i++)
    {
        const char *arg = __argv[i];

        if (!strcmp(arg, "-f"))
            g_Fullscreen = TRUE;
        else if (!strcmp(arg, "-i") && i + 1 < __argc)
        {
            g_Interval = atoi(__argv[++i]);
            g_AutoSwitch = TRUE;
        }
        else if (!strcmp(arg, "-n") && i + 1 < __argc)
            g_SwitchesLeft = atoi(__argv[++i]);
        else if (!strcmp(arg, "-x"))
            g_ExitWhenDone = TRUE;
        else if (!strcmp(arg, "-k"))
            g_Restore = FALSE;
        else if (g_nModes < MAX_MODES &&
                 (m.Bpp = 0, sscanf(arg, "%lux%lux%lu", &m.Width, &m.Height, &m.Bpp) >= 2))
            g_Modes[g_nModes++] = m;
        else
            return FALSE;
    }

    if (!g_nModes)
    {
        static const MODE Defaults[] = { { 640, 480, 0 }, { 800, 600, 0 }, { 1024, 768, 0 } };
        memcpy(g_Modes, Defaults, sizeof(Defaults));
        g_nModes = ARRAYSIZE(Defaults);
    }
    return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSW wc = { 0 };
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    MSG msg;
    BOOL bStartFullscreen;

    if (!ParseArgs())
    {
        Usage();
        return 1;
    }

    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.lpszClassName = L"glmodeswitch";
    RegisterClassW(&wc);

    g_hwnd = CreateWindowW(wc.lpszClassName, L"glmodeswitch",
                           WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                           CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
                           NULL, NULL, hInstance, NULL);
    if (!g_hwnd)
        return 1;

    g_hdc = GetDC(g_hwnd);
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    if (!SetPixelFormat(g_hdc, ChoosePixelFormat(g_hdc, &pfd), &pfd))
        return 1;

    g_hglrc = wglCreateContext(g_hdc);
    if (!g_hglrc || !wglMakeCurrent(g_hdc, g_hglrc))
        return 1;

    _snprintf(g_Renderer, sizeof(g_Renderer) - 1, "GL_RENDERER %s  pixel format %d",
              (const char *)glGetString(GL_RENDERER), GetPixelFormat(g_hdc));
    Report(g_Renderer);
    InitGL();

    ShowWindow(g_hwnd, nCmdShow);

    bStartFullscreen = g_Fullscreen;
    g_Fullscreen = FALSE;
    SetFullscreen(bStartFullscreen);

    if (g_AutoSwitch)
        SetTimer(g_hwnd, TIMER_SWITCH, g_Interval, NULL);

    for (;;)
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                goto done;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Draw();
    }

done:
    if (g_Restore && g_CurMode >= 0)
        ChangeDisplaySettingsW(NULL, 0);

    /* The GL context is deliberately not deleted, so the driver cleans up at process exit */
    return (int)msg.wParam;
}
