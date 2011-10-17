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
#include <undocuser.h>

MSG_ENTRY last_post_message;
MSG_ENTRY message_cache[100];
static int message_cache_size = 0;

MSG_ENTRY empty_chain[]= {{0,0}};

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
        case WM_SYSTIMER: return "WM_SYSTIMER";
        case WM_GETMINMAXINFO: return "WM_GETMINMAXINFO";
        case WM_NCCALCSIZE: return "WM_NCCALCSIZE";
        case WM_SETTINGCHANGE: return "WM_SETTINGCHANGE";
        case WM_GETICON: return "WM_GETICON";
        case WM_SETICON: return "WM_SETICON";
        default: return NULL;
    }
}

static char* get_hook_name(UINT id)
{
    switch (id)
    {
        case WH_KEYBOARD: return "WH_KEYBOARD";
        case WH_KEYBOARD_LL: return "WH_KEYBOARD_LL";
        case WH_MOUSE: return "WH_MOUSE";
        case WH_MOUSE_LL: return "WH_MOUSE_LL";
        default: return NULL;
    }
}

void empty_message_cache()
{
    memset(&last_post_message, 0, sizeof(last_post_message));
    memset(message_cache, 0, sizeof(message_cache));
    message_cache_size = 0;
}

void sprintf_msg_entry(char* buffer, MSG_ENTRY* msg)
{
    if(!msg->iwnd && !msg->msg)
    {
        sprintf(buffer, "nothing");
    }
    else 
    {
        char* msgName;
        char* msgType;

        switch (msg->type)
        {
        case POST:
            msgName = get_msg_name(msg->msg);
            msgType = "post msg";
            break;
        case SENT:
            msgName = get_msg_name(msg->msg);
            msgType = "sent msg";
            break;
        case HOOK:
            msgName = get_hook_name(msg->msg);
            msgType = "hook";
            break;
        case EVENT:
            msgName = NULL;
            msgType = "event";
            break;
        default:
            return;
        }

        if(msgName)
            sprintf(buffer, "hwnd%d %s %s %d %d", msg->iwnd, msgType, msgName, msg->param1, msg->param2); 
        else
            sprintf(buffer, "hwnd%d %s %d %d %d", msg->iwnd, msgType, msg->msg, msg->param1, msg->param2); 
    }
}

void trace_cache(const char* file, int line)
{
    int i;
    char buff[100];

    for (i=0; i < message_cache_size; i++)
    {
        sprintf_msg_entry(buff, &message_cache[i]);
        trace_(file,line)("%d: %s\n", i, buff);
    }
    trace_(file,line)("\n");
}

void compare_cache(const char* file, int line, MSG_ENTRY *msg_chain)
{
    int i = 0;
    char buffGot[100], buffExp[100];
    BOOL got_error = FALSE;

    while(1)
    {
        BOOL same = !memcmp(&message_cache[i],msg_chain, sizeof(MSG_ENTRY));

        sprintf_msg_entry(buffGot, &message_cache[i]);
        sprintf_msg_entry(buffExp, msg_chain);
        ok_(file,line)(same,"%d: got %s, expected %s\n",i, buffGot, buffExp);

        if(!got_error && !same)
            got_error = TRUE;

        if(msg_chain->msg !=0 || msg_chain->iwnd != 0)
            msg_chain++;
        else
        {
            if(i>message_cache_size)
                break;
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

void record_message(int iwnd, UINT message, MSG_TYPE type, int param1,int param2)
{
    if(message_cache_size>=100)
    {
        return;
    }

    /* do not report a post message a second time */
    if(type == SENT &&
       last_post_message.iwnd == iwnd && 
       last_post_message.msg == message && 
       last_post_message.param1 == param1 && 
       last_post_message.param2 == param2)
    {
        memset(&last_post_message, 0, sizeof(last_post_message));
        return;
    }

    message_cache[message_cache_size].iwnd = iwnd;
    message_cache[message_cache_size].msg = message;
    message_cache[message_cache_size].type = type;
    message_cache[message_cache_size].param1 = param1;
    message_cache[message_cache_size].param2 = param2;

    if(message_cache[message_cache_size].type == POST)
    {
        last_post_message = message_cache[message_cache_size];
    }
    else
    {
        memset(&last_post_message, 0, sizeof(last_post_message));
    }

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
