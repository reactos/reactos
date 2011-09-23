/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         helper functions
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>
#include "helper.h"

MSG_ENTRY message_cache[100];
static int message_cache_size = 0;

static char* get_msg_name(UINT msg)
{
    switch(msg)
    {
        case WM_NCACTIVATE: return "WM_NCACTIVATE";
        case WM_ACTIVATE: return "WM_ACTIVATE";
        case WM_ACTIVATEAPP: return "WM_ACTIVATEAPP";
        case WM_WINDOWPOSCHANGING: return "WM_WINDOWPOSCHANGING";
        case WM_WINDOWPOSCHANGED: return "WM_WINDOWPOSCHANGED";
        case WM_SETFOCUS: return "WM_SETFOCUS";
        case WM_KILLFOCUS: return "WM_KILLFOCUS";
        case WM_NCPAINT: return "WM_NCPAINT";
        case WM_PAINT: return "WM_PAINT";
        case WM_ERASEBKGND: return "WM_ERASEBKGND";
        case WM_SIZE: return "WM_SIZE";
        case WM_MOVE: return "WM_MOVE";
        case WM_SHOWWINDOW: return "WM_SHOWWINDOW";
        case WM_QUERYNEWPALETTE: return "WM_QUERYNEWPALETTE";
        case WM_MOUSELEAVE: return "WM_MOUSELEAVE";
        case WM_MOUSEHOVER: return "WM_MOUSEHOVER";
        case WM_NCMOUSELEAVE: return "WM_NCMOUSELEAVE";
        case WM_NCMOUSEHOVER: return "WM_NCMOUSEHOVER";
        case WM_NCHITTEST: return "WM_NCHITTEST";
        case WM_SETCURSOR: return "WM_SETCURSOR";
        case WM_MOUSEMOVE: return "WM_MOUSEMOVE";
        default: return NULL;
    }
}

void empty_message_cache()
{
    memset(message_cache, 0, sizeof(message_cache));
    message_cache_size = 0;
}

void trace_cache(const char* file, int line)
{
    int i;
    char *szMsgName;

    for (i=0; i < message_cache_size; i++)
    {
        if(!message_cache[i].hook)
        {
            if((szMsgName = get_msg_name(message_cache[i].msg)))
            {
                trace_(file,line)("hwnd%d, msg:%s\n",message_cache[i].iwnd, szMsgName );
            }
            else
            {
                trace_(file,line)("hwnd%d, msg:%d\n",message_cache[i].iwnd, message_cache[i].msg );
            }
        }
        else
        {
            trace_(file,line)("hwnd%d, hook:%d, param1:%d, param2:%d\n",message_cache[i].iwnd, 
                                                            message_cache[i].msg,
                                                            message_cache[i].param1,
                                                            message_cache[i].param2);
        }
    }
    trace("\n");
}

void compare_cache(const char* file, int line, MSG_ENTRY *msg_chain)
{
    int i = 0;
    BOOL got_error = FALSE;

    while(1)
    {
        char *szMsgExpected, *szMsgGot;
        BOOL same = !memcmp(&message_cache[i],msg_chain, sizeof(MSG_ENTRY));

        szMsgExpected = get_msg_name(msg_chain->msg);
        szMsgGot = get_msg_name(message_cache[i].msg);
        if(!msg_chain->hook)
        {
            if(szMsgExpected && szMsgGot)
            {
                ok_(file,line)(same,
                   "message %d: expected %s to hwnd%d and got %s to hwnd%d\n",
                    i, szMsgExpected, msg_chain->iwnd, szMsgGot, message_cache[i].iwnd);    
            }
            else
            {
                ok_(file,line)(same,
                   "message %d: expected msg %d to hwnd%d and got msg %d to hwnd%d\n",
                    i, msg_chain->msg, msg_chain->iwnd, message_cache[i].msg, message_cache[i].iwnd);
            }
        }
        else
        {
            ok_(file,line)(same,
               "message %d: expected hook %d, hwnd%d, param1 %d, param2 %d and got hook %d, hwnd%d, param1 %d, param2 %d\n",
                i, msg_chain->msg, msg_chain->iwnd,msg_chain->param1, msg_chain->param2,
               message_cache[i].msg, message_cache[i].iwnd, message_cache[i].param1, message_cache[i].param2);
        }

        if(!got_error && !same)
        {
            got_error = TRUE;
        }

        if(msg_chain->msg !=0 && msg_chain->iwnd != 0)
        {
            msg_chain++;
        }
        else
        {
            if(i>message_cache_size)
            {
                break;
            }
        }
        i++;
    }

    if(got_error )
    {
        trace_(file,line)("The complete list of messages got is:\n");
        trace_cache(file,line);
    }

    empty_message_cache();
}

void record_message(int iwnd, UINT message, BOOL hook, int param1,int param2)
{
    if(message_cache_size>=100)
    {
        return;
    }

    message_cache[message_cache_size].iwnd = iwnd;
    message_cache[message_cache_size].msg = message;
    message_cache[message_cache_size].hook = hook;
    message_cache[message_cache_size].param1 = param1;
    message_cache[message_cache_size].param2 = param2;
    message_cache_size++;
}

ATOM RegisterSimpleClass(WNDPROC lpfnWndProc, LPCWSTR lpszClassName)
{
    WNDCLASSEXW wcex;

    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc    = lpfnWndProc;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszClassName  = lpszClassName;
    return RegisterClassExW(&wcex);
}