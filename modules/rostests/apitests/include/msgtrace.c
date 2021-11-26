/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         helper functions
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <apitest.h>

#include <stdio.h>
#include <winuser.h>
#include <msgtrace.h>
#include <undocuser.h>

MSG_CACHE default_cache = {
#ifdef _MSC_VER
    0
#endif
};
MSG_ENTRY empty_chain[]= {{0,0}};

static char* get_msg_name(UINT msg)
{
    switch(msg)
    {
        case WM_CREATE: return "WM_CREATE";
        case WM_NCCREATE: return "WM_NCCREATE";
        case WM_PARENTNOTIFY: return "WM_PARENTNOTIFY";
        case WM_DESTROY: return "WM_DESTROY";
        case WM_NCDESTROY: return "WM_NCDESTROY";
        case WM_CHILDACTIVATE: return "WM_CHILDACTIVATE";
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
        case WM_KEYDOWN: return "WM_KEYDOWN";
        case WM_KEYUP: return "WM_KEYUP";
        case WM_NOTIFY: return "WM_NOTIFY";
        case WM_COMMAND: return "WM_COMMAND";
        case WM_PRINTCLIENT: return "WM_PRINTCLIENT";
        case WM_CTLCOLORSTATIC: return "WM_CTLCOLORSTATIC";
        case WM_STYLECHANGING: return "WM_STYLECHANGING";
        case WM_STYLECHANGED: return "WM_STYLECHANGED";
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

void empty_message_cache(MSG_CACHE* cache)
{
    memset(cache, 0, sizeof(MSG_CACHE));
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
        case MARKER:
            msgName = get_msg_name(msg->msg);
            msgType = msg->type == POST ? "post msg" : "marker";
            break;
        case SENT:
        case SENT_RET:
            msgName = get_msg_name(msg->msg);
            msgType = msg->type == SENT ? "sent msg" : "sent_ret msg";
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

void trace_cache(MSG_CACHE* cache, const char* file, int line)
{
    int i;
    char buff[100];

    for (i=0; i < cache->count; i++)
    {
        sprintf_msg_entry(buff, &cache->message_cache[i]);
        trace_(file,line)("%d: %s\n", i, buff);
    }
    trace_(file,line)("\n");
}

void compare_cache(MSG_CACHE* cache, const char* file, int line, MSG_ENTRY *msg_chain)
{
    int i = 0;
    char buffGot[100], buffExp[100];
    BOOL got_error = FALSE;

    while(1)
    {
        BOOL same = !memcmp(&cache->message_cache[i],msg_chain, sizeof(MSG_ENTRY));

        sprintf_msg_entry(buffGot, &cache->message_cache[i]);
        sprintf_msg_entry(buffExp, msg_chain);
        ok_(file,line)(same,"%d: got %s, expected %s\n",i, buffGot, buffExp);

        if(!got_error && !same)
            got_error = TRUE;

        if(msg_chain->msg !=0 || msg_chain->iwnd != 0)
            msg_chain++;
        else
        {
            if(i > cache->count)
                break;
        }
        i++;
    }

    if(got_error )
    {
        trace_(file,line)("The complete list of messages got is:\n");
        trace_cache(cache, file,line);
    }

    empty_message_cache(cache);
}

void record_message(MSG_CACHE* cache, int iwnd, UINT message, MSG_TYPE type, int param1,int param2)
{
    if(cache->count >= 100)
    {
        return;
    }

    /* do not report a post message a second time */
    if(type == SENT &&
       cache->last_post_message.iwnd == iwnd &&
       cache->last_post_message.msg == message &&
       cache->last_post_message.param1 == param1 &&
       cache->last_post_message.param2 == param2)
    {
        memset(&cache->last_post_message, 0, sizeof(MSG_ENTRY));
        return;
    }

    cache->message_cache[cache->count].iwnd = iwnd;
    cache->message_cache[cache->count].msg = message;
    cache->message_cache[cache->count].type = type;
    cache->message_cache[cache->count].param1 = param1;
    cache->message_cache[cache->count].param2 = param2;

    if(cache->message_cache[cache->count].type == POST)
    {
        cache->last_post_message = cache->message_cache[cache->count];
    }
    else
    {
        memset(&cache->last_post_message, 0, sizeof(MSG_ENTRY));
    }

    cache->count++;
}
