/*----------------------------------------------------------------------------
** wclickat.c
**  Utilty to send clicks to Wine Windows
**
** See usage() for usage instructions.
**
**---------------------------------------------------------------------------
**  Copyright 2004 Jozef Stefanka for CodeWeavers, Inc.
**  Copyright 2005 Dmitry Timoshkov for CodeWeavers, Inc.
**  Copyright 2005 Francois Gouget for CodeWeavers, Inc.
**
**     This program is free software; you can redistribute it and/or modify
**     it under the terms of the GNU General Public License as published by
**     the Free Software Foundation; either version 2 of the License, or
**     (at your option) any later version.
**
**     This program is distributed in the hope that it will be useful,
**     but WITHOUT ANY WARRANTY; without even the implied warranty of
**     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**     GNU General Public License for more details.
**
**     You should have received a copy of the GNU General Public License
**     along with this program; if not, write to the Free Software
**     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
**--------------------------------------------------------------------------*/


#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <ctype.h>


#define APP_NAME              "wclickat"
#define DEFAULT_DELAY         500
#define DEFAULT_REPEAT        1000

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof((array)[0]))

static const WCHAR STATIC_CLASS[]={'s','t','a','t','i','c','\0'};

/*----------------------------------------------------------------------------
**  Global variables
**--------------------------------------------------------------------------*/

#define RC_RUNNING            -1
#define RC_SUCCESS             0
#define RC_INVALID_ARGUMENTS   1
#define RC_NODISPLAY           2
#define RC_TIMEOUT             3
static int     status;

typedef enum
{
    ACTION_INVALID,
    ACTION_FIND,
    ACTION_LCLICK,
    ACTION_MCLICK,
    ACTION_RCLICK
} action_type;
static action_type g_action = ACTION_INVALID;

static WCHAR*  g_window_class = NULL;
static WCHAR*  g_window_title = NULL;
static long    g_control_id = 0;
static WCHAR*  g_control_class = NULL;
static WCHAR*  g_control_caption = NULL;
static long    g_x = -1;
static long    g_y = -1;
static long    g_dragto_x = -1;
static long    g_dragto_y = -1;
static long    g_disabled = 0;

static long    g_delay = DEFAULT_DELAY;
static long    g_timeout = 0;
static long    g_repeat = 0;
static long    g_untildeath = 0;
static UINT    timer_id;


/*
 * Provide some basic debugging support.
 */
#ifdef __GNUC__
#define __PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))
#else
#define __PRINTF_ATTR(fmt,args)
#endif
static int debug_on=0;
static int init_debug()
{
    char* str=getenv("CXTEST_DEBUG");
    if (str && strstr(str, "+wclickat"))
        debug_on=1;
    return debug_on;
}

static void cxlog(const char* format, ...) __PRINTF_ATTR(1,2);
static void cxlog(const char* format, ...)
{
    va_list valist;

    if (debug_on)
    {
        va_start(valist, format);
        vfprintf(stderr, format, valist);
        va_end(valist);
    }
}

/*----------------------------------------------------------------------------
** usage
**--------------------------------------------------------------------------*/
static void usage(void)
{
    fprintf(stderr, "%s - Utility to send clicks to Wine Windows.\n", APP_NAME);
    fprintf(stderr, "----------------------------------------------\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "    %s action --winclass class --wintitle title [--timeout ms]\n",APP_NAME);
    fprintf(stderr, "    %*.*s     [--ctrlclas class] [--ctrlcaption caption] [--ctrlid id]\n", strlen(APP_NAME) + 3, strlen(APP_NAME) + 3, "");
    fprintf(stderr, "    %*.*s     [--position XxY] [--delay ms] [--untildeath] [--repeat ms]\n", strlen(APP_NAME) + 3, strlen(APP_NAME) + 3, "");
    fprintf(stderr, "Where action can be one of:\n");
    fprintf(stderr, "  find              Find the specified window or control\n");
    fprintf(stderr, "  button<n>         Send a click with the given X button number\n");
    fprintf(stderr, "  click|lclick      Synonym for button1 (left click)\n");
    fprintf(stderr, "  mclick            Synonym for button2 (middle click)\n");
    fprintf(stderr, "  rclick            Synonym for button3 (right click)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "The options are as follows:\n");
    fprintf(stderr, "  --timeout ms      How long to wait before failing with a code of %d\n", RC_TIMEOUT);
    fprintf(stderr, "  --winclass class  Class name of the top-level window of interest\n");
    fprintf(stderr, "  --wintitle title  Title of the top-level window of interest\n");
    fprintf(stderr, "  --ctrlclass name  Class name of the control of interest, if any\n");
    fprintf(stderr, "  --ctrlcaption cap A substring of the control's caption\n");
    fprintf(stderr, "  --ctrlid id       Id of the control\n");
    fprintf(stderr, "  --position XxY    Coordinates for the click, relative to the window / control\n");
    fprintf(stderr, "  --dragto          If given, then position specifies start click, and\n");
    fprintf(stderr, "                    dragto specifies release coords.\n");
    fprintf(stderr, "  --allow-disabled  Match the window or control even hidden or disabled\n");
    fprintf(stderr, "  --delay ms        Wait ms milliseconds before clicking. The default is %d\n", DEFAULT_DELAY);
    fprintf(stderr, "  --untildeath      Wait until the window disappears\n");
    fprintf(stderr, "  --repeat ms       Click every ms milliseconds. The default is %d\n", DEFAULT_REPEAT);
    fprintf(stderr, "\n");
    fprintf(stderr, "%s returns %d on success\n", APP_NAME, RC_SUCCESS);
    fprintf(stderr, "\n");
    fprintf(stderr, "Environment variable overrides:\n");
    fprintf(stderr, "  CXTEST_TIME_MULTIPLE  Specifies a floating multiplier applied to any\n");
    fprintf(stderr, "                        delay and timeout parameters.\n");
}

static const WCHAR* my_strstriW(const WCHAR* haystack, const WCHAR* needle)
{
    const WCHAR *h,*n;
    WCHAR first;

    if (!*needle)
        return haystack;

    /* Special case the first character because
     * we will be doing a lot of comparisons with it.
     */
    first=towlower(*needle);
    needle++;
    while (*haystack)
    {
        while (towlower(*haystack)!=first && *haystack)
            haystack++;

        h=haystack+1;
        n=needle;
        while (towlower(*h)==towlower(*n) && *h)
        {
            h++;
            n++;
        }
        if (!*n)
            return haystack;
        haystack++;
    }
    return NULL;
}

static BOOL CALLBACK find_control(HWND hwnd, LPARAM lParam)
{
    WCHAR str[1024];
    HWND* pcontrol;

    if (!GetClassNameW(hwnd, str, ARRAY_LENGTH(str)) ||
        lstrcmpiW(str, g_control_class))
        return TRUE;

    if (g_control_caption)
    {
        if (!GetWindowTextW(hwnd, str, ARRAY_LENGTH(str)) ||
            !my_strstriW(str, g_control_caption))
            return TRUE;
    }
    if (g_control_id && g_control_id != GetWindowLong(hwnd, GWL_ID))
        return TRUE;

    /* Check that the control is visible and active */
    if (!g_disabled)
    {
        DWORD style = GetWindowStyle(hwnd);
        if (!(style & WS_VISIBLE) || (style &  WS_DISABLED))
            return TRUE;
    }

    pcontrol = (HWND*)lParam;
    *pcontrol = hwnd;
    return FALSE;
}

static BOOL CALLBACK find_top_window(HWND hwnd, LPARAM lParam)
{
    WCHAR str[1024];
    HWND* pwindow;

    if (!GetClassNameW(hwnd, str, ARRAY_LENGTH(str)) ||
        lstrcmpiW(str, g_window_class))
        return TRUE;

    if (!GetWindowTextW(hwnd, str, ARRAY_LENGTH(str)) ||
        lstrcmpiW(str, g_window_title))
        return TRUE;

    /* Check that the window is visible and active */
    if (!g_disabled)
    {
        DWORD style = GetWindowStyle(hwnd);
        if (!(style & WS_VISIBLE) || (style &  WS_DISABLED))
            return TRUE;
    }

    /* See if we find the control we want */
    if (g_control_class)
    {
        HWND control = NULL;
        EnumChildWindows(hwnd, find_control, (LPARAM)&control);
        if (!control)
            return TRUE;
        hwnd=control;
    }

    pwindow = (HWND*)lParam;
    *pwindow = hwnd;
    return FALSE;
}

static HWND find_window()
{
    HWND hwnd;

    hwnd=NULL;
    EnumWindows(find_top_window, (LPARAM)&hwnd);
    return hwnd;
}

static void do_click(HWND window, DWORD down, DWORD up)
{
    WINDOWINFO window_info;
    long x, y;

    SetForegroundWindow(GetParent(window));
    window_info.cbSize=sizeof(window_info);
    GetWindowInfo(window, &window_info);

    /* The calculations below convert the coordinates so they are absolute
     * screen coordinates in 'Mickeys' as required by mouse_event.
     * In mickeys the screen size is always 65535x65535.
     */
    x=window_info.rcWindow.left+g_x;
    if (x<window_info.rcWindow.left || x>=window_info.rcWindow.right)
        x=(window_info.rcWindow.right+window_info.rcWindow.left)/2;
    x=(x << 16)/GetSystemMetrics(SM_CXSCREEN);

    y=window_info.rcWindow.top+g_y;
    if (y<window_info.rcWindow.top || y>=window_info.rcWindow.bottom)
        y=(window_info.rcWindow.bottom+window_info.rcWindow.top)/2;
    y=(y << 16)/GetSystemMetrics(SM_CYSCREEN);

    mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, x, y, 0, 0);
    if (down) {
        mouse_event(MOUSEEVENTF_ABSOLUTE | down, x, y, 0, 0);
        if ((g_dragto_x > 0) && (g_dragto_y > 0)) {
            int i;
            long dx, dy;
            long step_per_x, step_per_y;
            long dragto_x, dragto_y;

            dragto_x=window_info.rcWindow.left+g_dragto_x;
            if (dragto_x<window_info.rcWindow.left || dragto_x>=window_info.rcWindow.right)
                dragto_x=(window_info.rcWindow.right+window_info.rcWindow.left)/2;
            dragto_x=(dragto_x << 16)/GetSystemMetrics(SM_CXSCREEN);

            dragto_y=window_info.rcWindow.top+g_dragto_y;
            if (dragto_y<window_info.rcWindow.top || dragto_y>=window_info.rcWindow.bottom)
                dragto_y=(window_info.rcWindow.bottom+window_info.rcWindow.top)/2;
            dragto_y=(dragto_y << 16)/GetSystemMetrics(SM_CYSCREEN);

            dx = g_dragto_x - g_x;
            dy = g_dragto_y - g_y;
            step_per_x = dx / 4;
            step_per_y = dy / 4;
            for (i = 0; i < 4; i++) {
                mouse_event(MOUSEEVENTF_MOVE, step_per_x, step_per_y, 0, 0);
            }
            x=dragto_x;
            y=dragto_y;
        }
    }
    if (up)
       mouse_event(MOUSEEVENTF_ABSOLUTE | up, x, y, 0, 0);
}

static void CALLBACK ClickProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    HWND window = find_window();

    if (!window)
    {
        if (g_untildeath)
        {
            /* FIXME: The window / control might just be disabled and if
             * that's the case we should not exit yet. But I don't expect
             * --untildeath to be used at all anyway so fixing this can
             * wait until it becomes necessary.
             */
            status=RC_SUCCESS;
        }
        else
            cxlog("The window has disappeared!\n");
        return;
    }

    switch (g_action)
    {
    case ACTION_FIND:
        /* Nothing to do */
        break;
    case ACTION_LCLICK:
        cxlog("Sending left click\n");
        do_click(window, MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP);
        break;
    case ACTION_MCLICK:
        cxlog("Sending middle click\n");
        do_click(window, MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP);
        break;
    case ACTION_RCLICK:
        cxlog("Sending right click\n");
        do_click(window, MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP);
    default:
        fprintf(stderr, "error: unknown action %d\n", g_action);
        break;
    }
    if (!g_repeat)
        status=RC_SUCCESS;
}

static void CALLBACK DelayProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    KillTimer(NULL, timer_id);
    timer_id=0;
    if (g_repeat)
    {
        cxlog("Setting up a timer for --repeat\n");
        timer_id=SetTimer(NULL, 0, g_repeat, ClickProc);
    }

    ClickProc(NULL, 0, 0, 0);
}

static void CALLBACK FindWindowProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    HWND window = find_window();
    if (!window)
        return;

    cxlog("Found the window\n");
    if (g_delay)
    {
        cxlog("Waiting for a bit\n");
        KillTimer(NULL, timer_id);
        timer_id=SetTimer(NULL, 0, g_delay, DelayProc);
        do_click(window, 0,0);
    }
    else
    {
        DelayProc(NULL, 0, 0, 0);
    }
}

static void CALLBACK TimeoutProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    status = RC_TIMEOUT;
}

/*----------------------------------------------------------------------------
** parse_arguments
**--------------------------------------------------------------------------*/
static int arg_get_long(const char** *argv, const char* name, long* value)
{
    if (!**argv)
    {
        fprintf(stderr, "error: missing argument for '%s'\n", name);
        return 1;
    }

    *value=atol(**argv);
    if (*value < 0)
    {
        fprintf(stderr, "error: invalid argument '%s' for '%s'\n",
                **argv, name);
        (*argv)++;
        return 1;
    }
    (*argv)++;
    return 0;
}

static int arg_get_utf8(const char** *argv, const char* name, WCHAR* *value)
{
    int len;

    if (!**argv)
    {
        fprintf(stderr, "error: missing argument for '%s'\n", name);
        return 1;
    }

    len = MultiByteToWideChar(CP_UTF8, 0, **argv, -1, NULL, 0);
    *value = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
    if (!*value)
    {
        fprintf(stderr, "error: memory allocation error\n");
        (*argv)++;
        return 1;
    }
    MultiByteToWideChar(CP_UTF8, 0, **argv, -1, *value, len);
    (*argv)++;
    return 0;
}

static int parse_arguments(int argc, const char** argv)
{
    int rc;
    const char* arg;
    char* p;

    rc=0;
    argv++;
    while (*argv)
    {
        arg=*argv++;
        if (*arg!='-')
        {
            if (g_action != ACTION_INVALID)
            {
                fprintf(stderr, "error: '%s' an action has already been specified\n", arg);
                rc=1;
            }
            else if (strcmp(arg, "click") == 0 || strcmp(arg, "lclick") == 0)
            {
                g_action = ACTION_LCLICK;
            }
            else if (strcmp(arg, "mclick") == 0)
            {
                g_action = ACTION_MCLICK;
            }
            else if (strcmp(arg, "rclick") == 0)
            {
                g_action = ACTION_RCLICK;
            }
            else if (strncmp(arg, "button", 6) == 0)
            {
                int button;
                char extra='\0';
                int r=sscanf(arg, "button%d%c", &button, &extra);
                /* We should always get r==1 but due to a bug in Wine's
                 * msvcrt.dll implementation (at least up to 20050127)
                 * we may also get r==2 and extra=='\0'.
                 */
                if (r!=1 && (r!=2 || extra!='\0'))
                {
                    fprintf(stderr, "error: invalid argument '%s' for '%s'\n",
                            *argv, arg);
                    rc=1;
                }
                else if (button<1 || button>3)
                {
                    fprintf(stderr, "error: unknown button '%s'\n", arg);
                    rc=1;
                }
                else
                {
                    /* Just to remain compatible with the enum */
                    g_action=button+ACTION_LCLICK-1;
                }
             }
            else if (strcmp(arg, "find") == 0)
            {
                g_action = ACTION_FIND;
            }
            else
            {
                fprintf(stderr, "error: unknown action '%s'\n", arg);
                rc=1;
            }
        }
        else if (strcmp(arg, "--winclass") == 0)
        {
            rc|=arg_get_utf8(&argv, arg, &g_window_class);
        }
        else if (strcmp(arg, "--wintitle") == 0)
        {
            rc|=arg_get_utf8(&argv,arg, &g_window_title);
        }
        else if (strcmp(arg, "--ctrlclass") == 0)
        {
            rc|=arg_get_utf8(&argv, arg, &g_control_class);
        }
        else if (strcmp(arg, "--ctrlid") == 0)
        {
            rc|=arg_get_long(&argv, arg, &g_control_id);
        }
        else if (strcmp(arg, "--ctrlcaption") == 0)
        {
            rc|=arg_get_utf8(&argv, arg, &g_control_caption);
        }
        else if (strcmp(arg, "--position") == 0)
        {
            if (!*argv)
            {
                fprintf(stderr, "error: missing argument for '%s'\n", arg);
                rc=1;
            }
            else
            {
                char extra='\0';
                int r=sscanf(*argv, "%ldx%ld%c", &g_x, &g_y, &extra);
                /* We should always get r==2 but due to a bug in Wine's
                 * msvcrt.dll implementation (at least up to 20050127)
                 * we may also get r==3 and extra=='\0'.
                 */
                if (r!=2 && (r!=3 || extra!='\0'))
                {
                    fprintf(stderr, "error: invalid argument '%s' for '%s'\n",
                            *argv, arg);
                    rc=1;
                }
                argv++;
            }
        }
        else if (strcmp(arg, "--dragto") == 0)
        {
            if (!*argv)
            {
                fprintf(stderr, "error: missing argument for '%s'\n", arg);
                rc=1;
            }
            else
            {
                char extra='\0';
                int r=sscanf(*argv, "%ldx%ld%c", &g_dragto_x, &g_dragto_y, &extra);
                /* We should always get r==2 but due to a bug in Wine's
                 *                  * msvcrt.dll implementation (at least up to 20050127)
                 *                                   * we may also get r==3 and extra=='\0'.
                 *                                                    */
                if (r!=2 && (r!=3 || extra!='\0'))
                {
                    fprintf(stderr, "error: invalid argument '%s' for '%s'\n",
                    *argv, arg);
                    rc=1;
                }
                argv++;
            }
        }
        else if (strcmp(arg, "--allow-disabled") == 0)
        {
            g_disabled = 1;
        }
        else if (strcmp(arg, "--delay") == 0)
        {
            rc|=arg_get_long(&argv, arg, &g_delay);
        }
        else if (strcmp(arg, "--timeout") == 0)
        {
            rc|=arg_get_long(&argv, arg, &g_timeout);
        }
        else if (strcmp(arg, "--repeat") == 0)
        {
            rc|=arg_get_long(&argv, arg, &g_repeat);
        }
        else if (strcmp(arg, "--untildeath") == 0)
        {
            g_untildeath=1;
        }
        else if (strcmp(arg, "--help") == 0)
        {
            rc=2;
        }
    }

    if (g_action == ACTION_INVALID)
    {
        fprintf(stderr, "error: you must specify an action type\n");
        rc=1;
    }
    else
    {
        /* Adjust the default delay and repeat parameters depending on
         * the operating mode so less needs to be specified on the command
         * line, and so we can assume them to be set right.
         */
        if (g_action == ACTION_FIND)
            g_delay=0;
        if (!g_untildeath)
            g_repeat=0;
        else if (!g_repeat)
            g_repeat=DEFAULT_REPEAT;
    }

    if (!g_window_class)
    {
        fprintf(stderr, "error: you must specify a --winclass parameter\n");
        rc=1;
    }
    if (!g_window_title)
    {
        fprintf(stderr, "error: you must specify a --wintitle parameter\n");
        rc=1;
    }
    if (g_control_class)
    {
        if (!g_control_id && !g_control_caption)
        {
            fprintf(stderr, "error: you must specify either the control id or its caption\n");
            rc=1;
        }
    }

    /*------------------------------------------------------------------------
    ** Process environment variables
    **----------------------------------------------------------------------*/
    p = getenv("CXTEST_TIME_MULTIPLE");
    if (p)
    {
        float g_multiple = atof(p);
        g_delay   = (long) (((float) g_delay) * g_multiple);
        g_timeout = (long) (((float) g_timeout) * g_multiple);
    }

    return rc;
}

int main(int argc, const char** argv)
{
    MSG msg;

    init_debug();

    status = parse_arguments(argc, argv);
    if (status)
    {
       if (status == 2)
          usage();
       else
           fprintf(stderr, "Issue %s --help for usage.\n", *argv);
       return RC_INVALID_ARGUMENTS;
    }
    cxlog("Entering message loop. action=%d\n", g_action);

    if (g_timeout>0)
        SetTimer(NULL, 0, g_timeout, TimeoutProc);
    timer_id=SetTimer(NULL, 0, 100, FindWindowProc);

    status=RC_RUNNING;
    while (status==RC_RUNNING && GetMessage(&msg, NULL, 0, 0)!=0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return status;
}
