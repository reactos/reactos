/*
 * Window messaging support
 *
 * Copyright 2001 Alexandre Julliard
 * Copyright 2008 Maarten Lankhorst
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winnls.h"
#include "dbt.h"
#include "dde.h"
#include "imm.h"
#include "ddk/imm.h"
#include "wine/unicode.h"
#include "wine/winternl.h"
#include "wine/server.h"
#include "user_private.h"
#include "win.h"
#include "controls.h"
#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(msg);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(key);

#define WM_NCMOUSEFIRST WM_NCMOUSEMOVE
#define WM_NCMOUSELAST  (WM_NCMOUSEFIRST+(WM_MOUSELAST-WM_MOUSEFIRST))

#define MAX_PACK_COUNT 4

#define SYS_TIMER_RATE  55   /* min. timer rate in ms (actually 54.925)*/

/* the various structures that can be sent in messages, in platform-independent layout */
struct packed_CREATESTRUCTW
{
    ULONGLONG     lpCreateParams;
    ULONGLONG     hInstance;
    user_handle_t hMenu;
    DWORD         __pad1;
    user_handle_t hwndParent;
    DWORD         __pad2;
    INT           cy;
    INT           cx;
    INT           y;
    INT           x;
    LONG          style;
    ULONGLONG     lpszName;
    ULONGLONG     lpszClass;
    DWORD         dwExStyle;
    DWORD         __pad3;
};

struct packed_DRAWITEMSTRUCT
{
    UINT          CtlType;
    UINT          CtlID;
    UINT          itemID;
    UINT          itemAction;
    UINT          itemState;
    user_handle_t hwndItem;
    DWORD         __pad1;
    user_handle_t hDC;
    DWORD         __pad2;
    RECT          rcItem;
    ULONGLONG     itemData;
};

struct packed_MEASUREITEMSTRUCT
{
    UINT          CtlType;
    UINT          CtlID;
    UINT          itemID;
    UINT          itemWidth;
    UINT          itemHeight;
    ULONGLONG     itemData;
};

struct packed_DELETEITEMSTRUCT
{
    UINT          CtlType;
    UINT          CtlID;
    UINT          itemID;
    user_handle_t hwndItem;
    DWORD         __pad;
    ULONGLONG     itemData;
};

struct packed_COMPAREITEMSTRUCT
{
    UINT          CtlType;
    UINT          CtlID;
    user_handle_t hwndItem;
    DWORD         __pad1;
    UINT          itemID1;
    ULONGLONG     itemData1;
    UINT          itemID2;
    ULONGLONG     itemData2;
    DWORD         dwLocaleId;
    DWORD         __pad2;
};

struct packed_WINDOWPOS
{
    user_handle_t hwnd;
    DWORD         __pad1;
    user_handle_t hwndInsertAfter;
    DWORD         __pad2;
    INT           x;
    INT           y;
    INT           cx;
    INT           cy;
    UINT          flags;
    DWORD         __pad3;
};

struct packed_COPYDATASTRUCT
{
    ULONGLONG dwData;
    DWORD     cbData;
    ULONGLONG lpData;
};

struct packed_HELPINFO
{
    UINT          cbSize;
    INT           iContextType;
    INT           iCtrlId;
    user_handle_t hItemHandle;
    DWORD         __pad;
    ULONGLONG     dwContextId;
    POINT         MousePos;
};

struct packed_NCCALCSIZE_PARAMS
{
    RECT          rgrc[3];
    ULONGLONG     __pad1;
    user_handle_t hwnd;
    DWORD         __pad2;
    user_handle_t hwndInsertAfter;
    DWORD         __pad3;
    INT           x;
    INT           y;
    INT           cx;
    INT           cy;
    UINT          flags;
    DWORD         __pad4;
};

struct packed_MSG
{
    user_handle_t hwnd;
    DWORD         __pad1;
    UINT          message;
    ULONGLONG     wParam;
    ULONGLONG     lParam;
    DWORD         time;
    POINT         pt;
    DWORD         __pad2;
};

struct packed_MDINEXTMENU
{
    user_handle_t hmenuIn;
    DWORD         __pad1;
    user_handle_t hmenuNext;
    DWORD         __pad2;
    user_handle_t hwndNext;
    DWORD         __pad3;
};

struct packed_MDICREATESTRUCTW
{
    ULONGLONG szClass;
    ULONGLONG szTitle;
    ULONGLONG hOwner;
    INT       x;
    INT       y;
    INT       cx;
    INT       cy;
    DWORD     style;
    ULONGLONG lParam;
};

struct packed_hook_extra_info
{
    user_handle_t handle;
    DWORD         __pad;
    ULONGLONG     lparam;
};

/* the structures are unpacked on top of the packed ones, so make sure they fit */
C_ASSERT( sizeof(struct packed_CREATESTRUCTW) >= sizeof(CREATESTRUCTW) );
C_ASSERT( sizeof(struct packed_DRAWITEMSTRUCT) >= sizeof(DRAWITEMSTRUCT) );
C_ASSERT( sizeof(struct packed_MEASUREITEMSTRUCT) >= sizeof(MEASUREITEMSTRUCT) );
C_ASSERT( sizeof(struct packed_DELETEITEMSTRUCT) >= sizeof(DELETEITEMSTRUCT) );
C_ASSERT( sizeof(struct packed_COMPAREITEMSTRUCT) >= sizeof(COMPAREITEMSTRUCT) );
C_ASSERT( sizeof(struct packed_WINDOWPOS) >= sizeof(WINDOWPOS) );
C_ASSERT( sizeof(struct packed_COPYDATASTRUCT) >= sizeof(COPYDATASTRUCT) );
C_ASSERT( sizeof(struct packed_HELPINFO) >= sizeof(HELPINFO) );
C_ASSERT( sizeof(struct packed_NCCALCSIZE_PARAMS) >= sizeof(NCCALCSIZE_PARAMS) + sizeof(WINDOWPOS) );
C_ASSERT( sizeof(struct packed_MSG) >= sizeof(MSG) );
C_ASSERT( sizeof(struct packed_MDINEXTMENU) >= sizeof(MDINEXTMENU) );
C_ASSERT( sizeof(struct packed_MDICREATESTRUCTW) >= sizeof(MDICREATESTRUCTW) );
C_ASSERT( sizeof(struct packed_hook_extra_info) >= sizeof(struct hook_extra_info) );

union packed_structs
{
    struct packed_CREATESTRUCTW cs;
    struct packed_DRAWITEMSTRUCT dis;
    struct packed_MEASUREITEMSTRUCT mis;
    struct packed_DELETEITEMSTRUCT dls;
    struct packed_COMPAREITEMSTRUCT cis;
    struct packed_WINDOWPOS wp;
    struct packed_COPYDATASTRUCT cds;
    struct packed_HELPINFO hi;
    struct packed_NCCALCSIZE_PARAMS ncp;
    struct packed_MSG msg;
    struct packed_MDINEXTMENU mnm;
    struct packed_MDICREATESTRUCTW mcs;
    struct packed_hook_extra_info hook;
};

/* description of the data fields that need to be packed along with a sent message */
struct packed_message
{
    union packed_structs ps;
    int                  count;
    const void          *data[MAX_PACK_COUNT];
    size_t               size[MAX_PACK_COUNT];
};

/* info about the message currently being received by the current thread */
struct received_message_info
{
    enum message_type type;
    MSG               msg;
    UINT              flags;  /* InSendMessageEx return flags */
};

/* structure to group all parameters for sent messages of the various kinds */
struct send_message_info
{
    enum message_type type;
    DWORD             dest_tid;
    HWND              hwnd;
    UINT              msg;
    WPARAM            wparam;
    LPARAM            lparam;
    UINT              flags;      /* flags for SendMessageTimeout */
    UINT              timeout;    /* timeout for SendMessageTimeout */
    SENDASYNCPROC     callback;   /* callback function for SendMessageCallback */
    ULONG_PTR         data;       /* callback data */
    enum wm_char_mapping wm_char;
};


/* Message class descriptor */
static const WCHAR messageW[] = {'M','e','s','s','a','g','e',0};

const struct builtin_class_descr MESSAGE_builtin_class =
{
    messageW,             /* name */
    0,                    /* style */
    WINPROC_MESSAGE,      /* proc */
    0,                    /* extra */
    IDC_ARROW,            /* cursor */
    0                     /* brush */
};



/* flag for messages that contain pointers */
/* 32 messages per entry, messages 0..31 map to bits 0..31 */

#define SET(msg) (1 << ((msg) & 31))

static const unsigned int message_pointer_flags[] =
{
    /* 0x00 - 0x1f */
    SET(WM_CREATE) | SET(WM_SETTEXT) | SET(WM_GETTEXT) |
    SET(WM_WININICHANGE) | SET(WM_DEVMODECHANGE),
    /* 0x20 - 0x3f */
    SET(WM_GETMINMAXINFO) | SET(WM_DRAWITEM) | SET(WM_MEASUREITEM) | SET(WM_DELETEITEM) |
    SET(WM_COMPAREITEM),
    /* 0x40 - 0x5f */
    SET(WM_WINDOWPOSCHANGING) | SET(WM_WINDOWPOSCHANGED) | SET(WM_COPYDATA) |
    SET(WM_NOTIFY) | SET(WM_HELP),
    /* 0x60 - 0x7f */
    SET(WM_STYLECHANGING) | SET(WM_STYLECHANGED),
    /* 0x80 - 0x9f */
    SET(WM_NCCREATE) | SET(WM_NCCALCSIZE) | SET(WM_GETDLGCODE),
    /* 0xa0 - 0xbf */
    SET(EM_GETSEL) | SET(EM_GETRECT) | SET(EM_SETRECT) | SET(EM_SETRECTNP),
    /* 0xc0 - 0xdf */
    SET(EM_REPLACESEL) | SET(EM_GETLINE) | SET(EM_SETTABSTOPS),
    /* 0xe0 - 0xff */
    SET(SBM_GETRANGE) | SET(SBM_SETSCROLLINFO) | SET(SBM_GETSCROLLINFO) | SET(SBM_GETSCROLLBARINFO),
    /* 0x100 - 0x11f */
    0,
    /* 0x120 - 0x13f */
    0,
    /* 0x140 - 0x15f */
    SET(CB_GETEDITSEL) | SET(CB_ADDSTRING) | SET(CB_DIR) | SET(CB_GETLBTEXT) |
    SET(CB_INSERTSTRING) | SET(CB_FINDSTRING) | SET(CB_SELECTSTRING) |
    SET(CB_GETDROPPEDCONTROLRECT) | SET(CB_FINDSTRINGEXACT),
    /* 0x160 - 0x17f */
    0,
    /* 0x180 - 0x19f */
    SET(LB_ADDSTRING) | SET(LB_INSERTSTRING) | SET(LB_GETTEXT) | SET(LB_SELECTSTRING) |
    SET(LB_DIR) | SET(LB_FINDSTRING) |
    SET(LB_GETSELITEMS) | SET(LB_SETTABSTOPS) | SET(LB_ADDFILE) | SET(LB_GETITEMRECT),
    /* 0x1a0 - 0x1bf */
    SET(LB_FINDSTRINGEXACT),
    /* 0x1c0 - 0x1df */
    0,
    /* 0x1e0 - 0x1ff */
    0,
    /* 0x200 - 0x21f */
    SET(WM_NEXTMENU) | SET(WM_SIZING) | SET(WM_MOVING) | SET(WM_DEVICECHANGE),
    /* 0x220 - 0x23f */
    SET(WM_MDICREATE) | SET(WM_MDIGETACTIVE) | SET(WM_DROPOBJECT) |
    SET(WM_QUERYDROPOBJECT) | SET(WM_DRAGLOOP) | SET(WM_DRAGSELECT) | SET(WM_DRAGMOVE),
    /* 0x240 - 0x25f */
    0,
    /* 0x260 - 0x27f */
    0,
    /* 0x280 - 0x29f */
    0,
    /* 0x2a0 - 0x2bf */
    0,
    /* 0x2c0 - 0x2df */
    0,
    /* 0x2e0 - 0x2ff */
    0,
    /* 0x300 - 0x31f */
    SET(WM_ASKCBFORMATNAME)
};

/* flags for messages that contain Unicode strings */
static const unsigned int message_unicode_flags[] =
{
    /* 0x00 - 0x1f */
    SET(WM_CREATE) | SET(WM_SETTEXT) | SET(WM_GETTEXT) | SET(WM_GETTEXTLENGTH) |
    SET(WM_WININICHANGE) | SET(WM_DEVMODECHANGE),
    /* 0x20 - 0x3f */
    SET(WM_CHARTOITEM),
    /* 0x40 - 0x5f */
    0,
    /* 0x60 - 0x7f */
    0,
    /* 0x80 - 0x9f */
    SET(WM_NCCREATE),
    /* 0xa0 - 0xbf */
    0,
    /* 0xc0 - 0xdf */
    SET(EM_REPLACESEL) | SET(EM_GETLINE) | SET(EM_SETPASSWORDCHAR),
    /* 0xe0 - 0xff */
    0,
    /* 0x100 - 0x11f */
    SET(WM_CHAR) | SET(WM_DEADCHAR) | SET(WM_SYSCHAR) | SET(WM_SYSDEADCHAR),
    /* 0x120 - 0x13f */
    SET(WM_MENUCHAR),
    /* 0x140 - 0x15f */
    SET(CB_ADDSTRING) | SET(CB_DIR) | SET(CB_GETLBTEXT) | SET(CB_GETLBTEXTLEN) |
    SET(CB_INSERTSTRING) | SET(CB_FINDSTRING) | SET(CB_SELECTSTRING) | SET(CB_FINDSTRINGEXACT),
    /* 0x160 - 0x17f */
    0,
    /* 0x180 - 0x19f */
    SET(LB_ADDSTRING) | SET(LB_INSERTSTRING) | SET(LB_GETTEXT) | SET(LB_GETTEXTLEN) |
    SET(LB_SELECTSTRING) | SET(LB_DIR) | SET(LB_FINDSTRING) | SET(LB_ADDFILE),
    /* 0x1a0 - 0x1bf */
    SET(LB_FINDSTRINGEXACT),
    /* 0x1c0 - 0x1df */
    0,
    /* 0x1e0 - 0x1ff */
    0,
    /* 0x200 - 0x21f */
    0,
    /* 0x220 - 0x23f */
    SET(WM_MDICREATE),
    /* 0x240 - 0x25f */
    0,
    /* 0x260 - 0x27f */
    0,
    /* 0x280 - 0x29f */
    SET(WM_IME_CHAR),
    /* 0x2a0 - 0x2bf */
    0,
    /* 0x2c0 - 0x2df */
    0,
    /* 0x2e0 - 0x2ff */
    0,
    /* 0x300 - 0x31f */
    SET(WM_PAINTCLIPBOARD) | SET(WM_SIZECLIPBOARD) | SET(WM_ASKCBFORMATNAME)
};

/* check whether a given message type includes pointers */
static inline int is_pointer_message( UINT message )
{
    if (message >= 8*sizeof(message_pointer_flags)) return FALSE;
    return (message_pointer_flags[message / 32] & SET(message)) != 0;
}

/* check whether a given message type contains Unicode (or ASCII) chars */
static inline int is_unicode_message( UINT message )
{
    if (message >= 8*sizeof(message_unicode_flags)) return FALSE;
    return (message_unicode_flags[message / 32] & SET(message)) != 0;
}

#undef SET

/* add a data field to a packed message */
static inline void push_data( struct packed_message *data, const void *ptr, size_t size )
{
    data->data[data->count] = ptr;
    data->size[data->count] = size;
    data->count++;
}

/* add a string to a packed message */
static inline void push_string( struct packed_message *data, LPCWSTR str )
{
    push_data( data, str, (strlenW(str) + 1) * sizeof(WCHAR) );
}

/* make sure that the buffer contains a valid null-terminated Unicode string */
static inline BOOL check_string( LPCWSTR str, size_t size )
{
    for (size /= sizeof(WCHAR); size; size--, str++)
        if (!*str) return TRUE;
    return FALSE;
}

/* pack a pointer into a 32/64 portable format */
static inline ULONGLONG pack_ptr( const void *ptr )
{
    return (ULONG_PTR)ptr;
}

/* unpack a potentially 64-bit pointer, returning 0 when truncated */
static inline void *unpack_ptr( ULONGLONG ptr64 )
{
    if ((ULONG_PTR)ptr64 != ptr64) return 0;
    return (void *)(ULONG_PTR)ptr64;
}

/* make sure that there is space for 'size' bytes in buffer, growing it if needed */
static inline void *get_buffer_space( void **buffer, size_t size )
{
    void *ret;

    if (*buffer)
    {
        if (!(ret = HeapReAlloc( GetProcessHeap(), 0, *buffer, size )))
            HeapFree( GetProcessHeap(), 0, *buffer );
    }
    else ret = HeapAlloc( GetProcessHeap(), 0, size );

    *buffer = ret;
    return ret;
}

/* check whether a combobox expects strings or ids in CB_ADDSTRING/CB_INSERTSTRING */
static inline BOOL combobox_has_strings( HWND hwnd )
{
    DWORD style = GetWindowLongA( hwnd, GWL_STYLE );
    return (!(style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) || (style & CBS_HASSTRINGS));
}

/* check whether a listbox expects strings or ids in LB_ADDSTRING/LB_INSERTSTRING */
static inline BOOL listbox_has_strings( HWND hwnd )
{
    DWORD style = GetWindowLongA( hwnd, GWL_STYLE );
    return (!(style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) || (style & LBS_HASSTRINGS));
}

/* check whether message is in the range of keyboard messages */
static inline BOOL is_keyboard_message( UINT message )
{
    return (message >= WM_KEYFIRST && message <= WM_KEYLAST);
}

/* check whether message is in the range of mouse messages */
static inline BOOL is_mouse_message( UINT message )
{
    return ((message >= WM_NCMOUSEFIRST && message <= WM_NCMOUSELAST) ||
            (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST));
}

/* check whether message matches the specified hwnd filter */
static inline BOOL check_hwnd_filter( const MSG *msg, HWND hwnd_filter )
{
    if (!hwnd_filter || hwnd_filter == GetDesktopWindow()) return TRUE;
    return (msg->hwnd == hwnd_filter || IsChild( hwnd_filter, msg->hwnd ));
}

/* check for pending WM_CHAR message with DBCS trailing byte */
static inline BOOL get_pending_wmchar( MSG *msg, UINT first, UINT last, BOOL remove )
{
    struct wm_char_mapping_data *data = get_user_thread_info()->wmchar_data;

    if (!data || !data->get_msg.message) return FALSE;
    if ((first || last) && (first > WM_CHAR || last < WM_CHAR)) return FALSE;
    if (!msg) return FALSE;
    *msg = data->get_msg;
    if (remove) data->get_msg.message = 0;
    return TRUE;
}


/***********************************************************************
 *           MessageWndProc
 *
 * Window procedure for "Message" windows (HWND_MESSAGE parent).
 */
LRESULT WINAPI MessageWndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if (message == WM_NCCREATE) return TRUE;
    return 0;  /* all other messages are ignored */
}


/***********************************************************************
 *		broadcast_message_callback
 *
 * Helper callback for broadcasting messages.
 */
static BOOL CALLBACK broadcast_message_callback( HWND hwnd, LPARAM lparam )
{
    struct send_message_info *info = (struct send_message_info *)lparam;
    if (!(GetWindowLongW( hwnd, GWL_STYLE ) & (WS_POPUP|WS_CAPTION))) return TRUE;
    switch(info->type)
    {
    case MSG_UNICODE:
        SendMessageTimeoutW( hwnd, info->msg, info->wparam, info->lparam,
                             info->flags, info->timeout, NULL );
        break;
    case MSG_ASCII:
        SendMessageTimeoutA( hwnd, info->msg, info->wparam, info->lparam,
                             info->flags, info->timeout, NULL );
        break;
    case MSG_NOTIFY:
        SendNotifyMessageW( hwnd, info->msg, info->wparam, info->lparam );
        break;
    case MSG_CALLBACK:
        SendMessageCallbackW( hwnd, info->msg, info->wparam, info->lparam,
                              info->callback, info->data );
        break;
    case MSG_POSTED:
        PostMessageW( hwnd, info->msg, info->wparam, info->lparam );
        break;
    default:
        ERR( "bad type %d\n", info->type );
        break;
    }
    return TRUE;
}


/***********************************************************************
 *		map_wparam_AtoW
 *
 * Convert the wparam of an ASCII message to Unicode.
 */
BOOL map_wparam_AtoW( UINT message, WPARAM *wparam, enum wm_char_mapping mapping )
{
    char ch[2];
    WCHAR wch[2];

    wch[0] = wch[1] = 0;
    switch(message)
    {
    case WM_CHAR:
        /* WM_CHAR is magic: a DBCS char can be sent/posted as two consecutive WM_CHAR
         * messages, in which case the first char is stored, and the conversion
         * to Unicode only takes place once the second char is sent/posted.
         */
        if (mapping != WMCHAR_MAP_NOMAPPING)
        {
            struct wm_char_mapping_data *data = get_user_thread_info()->wmchar_data;
            BYTE low = LOBYTE(*wparam);

            if (HIBYTE(*wparam))
            {
                ch[0] = low;
                ch[1] = HIBYTE(*wparam);
                RtlMultiByteToUnicodeN( wch, sizeof(wch), NULL, ch, 2 );
                TRACE( "map %02x,%02x -> %04x mapping %u\n", (BYTE)ch[0], (BYTE)ch[1], wch[0], mapping );
                if (data) data->lead_byte[mapping] = 0;
            }
            else if (data && data->lead_byte[mapping])
            {
                ch[0] = data->lead_byte[mapping];
                ch[1] = low;
                RtlMultiByteToUnicodeN( wch, sizeof(wch), NULL, ch, 2 );
                TRACE( "map stored %02x,%02x -> %04x mapping %u\n", (BYTE)ch[0], (BYTE)ch[1], wch[0], mapping );
                data->lead_byte[mapping] = 0;
            }
            else if (!IsDBCSLeadByte( low ))
            {
                ch[0] = low;
                RtlMultiByteToUnicodeN( wch, sizeof(wch), NULL, ch, 1 );
                TRACE( "map %02x -> %04x\n", (BYTE)ch[0], wch[0] );
                if (data) data->lead_byte[mapping] = 0;
            }
            else  /* store it and wait for trail byte */
            {
                if (!data)
                {
                    if (!(data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data) )))
                        return FALSE;
                    get_user_thread_info()->wmchar_data = data;
                }
                TRACE( "storing lead byte %02x mapping %u\n", low, mapping );
                data->lead_byte[mapping] = low;
                return FALSE;
            }
            *wparam = MAKEWPARAM(wch[0], wch[1]);
            break;
        }
        /* else fall through */
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:
        ch[0] = LOBYTE(*wparam);
        ch[1] = HIBYTE(*wparam);
        RtlMultiByteToUnicodeN( wch, sizeof(wch), NULL, ch, 2 );
        *wparam = MAKEWPARAM(wch[0], wch[1]);
        break;
    case WM_IME_CHAR:
        ch[0] = HIBYTE(*wparam);
        ch[1] = LOBYTE(*wparam);
        if (ch[0]) RtlMultiByteToUnicodeN( wch, sizeof(wch[0]), NULL, ch, 2 );
        else RtlMultiByteToUnicodeN( wch, sizeof(wch[0]), NULL, ch + 1, 1 );
        *wparam = MAKEWPARAM(wch[0], HIWORD(*wparam));
        break;
    }
    return TRUE;
}


/***********************************************************************
 *		map_wparam_WtoA
 *
 * Convert the wparam of a Unicode message to ASCII.
 */
static void map_wparam_WtoA( MSG *msg, BOOL remove )
{
    BYTE ch[2];
    WCHAR wch[2];
    DWORD len;

    switch(msg->message)
    {
    case WM_CHAR:
        if (!HIWORD(msg->wParam))
        {
            wch[0] = LOWORD(msg->wParam);
            ch[0] = ch[1] = 0;
            RtlUnicodeToMultiByteN( (LPSTR)ch, 2, &len, wch, sizeof(wch[0]) );
            if (len == 2)  /* DBCS char */
            {
                struct wm_char_mapping_data *data = get_user_thread_info()->wmchar_data;
                if (!data)
                {
                    if (!(data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data) ))) return;
                    get_user_thread_info()->wmchar_data = data;
                }
                if (remove)
                {
                    data->get_msg = *msg;
                    data->get_msg.wParam = ch[1];
                }
                msg->wParam = ch[0];
                return;
            }
        }
        /* else fall through */
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:
        wch[0] = LOWORD(msg->wParam);
        wch[1] = HIWORD(msg->wParam);
        ch[0] = ch[1] = 0;
        RtlUnicodeToMultiByteN( (LPSTR)ch, 2, NULL, wch, sizeof(wch) );
        msg->wParam = MAKEWPARAM( ch[0] | (ch[1] << 8), 0 );
        break;
    case WM_IME_CHAR:
        wch[0] = LOWORD(msg->wParam);
        ch[0] = ch[1] = 0;
        RtlUnicodeToMultiByteN( (LPSTR)ch, 2, &len, wch, sizeof(wch[0]) );
        if (len == 2)
            msg->wParam = MAKEWPARAM( (ch[0] << 8) | ch[1], HIWORD(msg->wParam) );
        else
            msg->wParam = MAKEWPARAM( ch[0], HIWORD(msg->wParam) );
        break;
    }
}


/***********************************************************************
 *		pack_message
 *
 * Pack a message for sending to another process.
 * Return the size of the data we expect in the message reply.
 * Set data->count to -1 if there is an error.
 */
static size_t pack_message( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                            struct packed_message *data )
{
    data->count = 0;
    switch(message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
    {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
        data->ps.cs.lpCreateParams = pack_ptr( cs->lpCreateParams );
        data->ps.cs.hInstance      = pack_ptr( cs->hInstance );
        data->ps.cs.hMenu          = wine_server_user_handle( cs->hMenu );
        data->ps.cs.hwndParent     = wine_server_user_handle( cs->hwndParent );
        data->ps.cs.cy             = cs->cy;
        data->ps.cs.cx             = cs->cx;
        data->ps.cs.y              = cs->y;
        data->ps.cs.x              = cs->x;
        data->ps.cs.style          = cs->style;
        data->ps.cs.dwExStyle      = cs->dwExStyle;
        data->ps.cs.lpszName       = pack_ptr( cs->lpszName );
        data->ps.cs.lpszClass      = pack_ptr( cs->lpszClass );
        push_data( data, &data->ps.cs, sizeof(data->ps.cs) );
        if (!IS_INTRESOURCE(cs->lpszName)) push_string( data, cs->lpszName );
        if (!IS_INTRESOURCE(cs->lpszClass)) push_string( data, cs->lpszClass );
        return sizeof(data->ps.cs);
    }
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        return wparam * sizeof(WCHAR);
    case WM_WININICHANGE:
        if (lparam) push_string(data, (LPWSTR)lparam );
        return 0;
    case WM_SETTEXT:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        push_string( data, (LPWSTR)lparam );
        return 0;
    case WM_GETMINMAXINFO:
        push_data( data, (MINMAXINFO *)lparam, sizeof(MINMAXINFO) );
        return sizeof(MINMAXINFO);
    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lparam;
        data->ps.dis.CtlType    = dis->CtlType;
        data->ps.dis.CtlID      = dis->CtlID;
        data->ps.dis.itemID     = dis->itemID;
        data->ps.dis.itemAction = dis->itemAction;
        data->ps.dis.itemState  = dis->itemState;
        data->ps.dis.hwndItem   = wine_server_user_handle( dis->hwndItem );
        data->ps.dis.hDC        = wine_server_user_handle( dis->hDC );  /* FIXME */
        data->ps.dis.rcItem     = dis->rcItem;
        data->ps.dis.itemData   = dis->itemData;
        push_data( data, &data->ps.dis, sizeof(data->ps.dis) );
        return 0;
    }
    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lparam;
        data->ps.mis.CtlType    = mis->CtlType;
        data->ps.mis.CtlID      = mis->CtlID;
        data->ps.mis.itemID     = mis->itemID;
        data->ps.mis.itemWidth  = mis->itemWidth;
        data->ps.mis.itemHeight = mis->itemHeight;
        data->ps.mis.itemData   = mis->itemData;
        push_data( data, &data->ps.mis, sizeof(data->ps.mis) );
        return sizeof(data->ps.mis);
    }
    case WM_DELETEITEM:
    {
        DELETEITEMSTRUCT *dls = (DELETEITEMSTRUCT *)lparam;
        data->ps.dls.CtlType    = dls->CtlType;
        data->ps.dls.CtlID      = dls->CtlID;
        data->ps.dls.itemID     = dls->itemID;
        data->ps.dls.hwndItem   = wine_server_user_handle( dls->hwndItem );
        data->ps.dls.itemData   = dls->itemData;
        push_data( data, &data->ps.dls, sizeof(data->ps.dls) );
        return 0;
    }
    case WM_COMPAREITEM:
    {
        COMPAREITEMSTRUCT *cis = (COMPAREITEMSTRUCT *)lparam;
        data->ps.cis.CtlType    = cis->CtlType;
        data->ps.cis.CtlID      = cis->CtlID;
        data->ps.cis.hwndItem   = wine_server_user_handle( cis->hwndItem );
        data->ps.cis.itemID1    = cis->itemID1;
        data->ps.cis.itemData1  = cis->itemData1;
        data->ps.cis.itemID2    = cis->itemID2;
        data->ps.cis.itemData2  = cis->itemData2;
        data->ps.cis.dwLocaleId = cis->dwLocaleId;
        push_data( data, &data->ps.cis, sizeof(data->ps.cis) );
        return 0;
    }
    case WM_WINE_SETWINDOWPOS:
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
        WINDOWPOS *wp = (WINDOWPOS *)lparam;
        data->ps.wp.hwnd            = wine_server_user_handle( wp->hwnd );
        data->ps.wp.hwndInsertAfter = wine_server_user_handle( wp->hwndInsertAfter );
        data->ps.wp.x               = wp->x;
        data->ps.wp.y               = wp->y;
        data->ps.wp.cx              = wp->cx;
        data->ps.wp.cy              = wp->cy;
        data->ps.wp.flags           = wp->flags;
        push_data( data, &data->ps.wp, sizeof(data->ps.wp) );
        return sizeof(data->ps.wp);
    }
    case WM_COPYDATA:
    {
        COPYDATASTRUCT *cds = (COPYDATASTRUCT *)lparam;
        data->ps.cds.cbData = cds->cbData;
        data->ps.cds.dwData = cds->dwData;
        data->ps.cds.lpData = pack_ptr( cds->lpData );
        push_data( data, &data->ps.cds, sizeof(data->ps.cds) );
        if (cds->lpData) push_data( data, cds->lpData, cds->cbData );
        return 0;
    }
    case WM_NOTIFY:
        /* WM_NOTIFY cannot be sent across processes (MSDN) */
        data->count = -1;
        return 0;
    case WM_HELP:
    {
        HELPINFO *hi = (HELPINFO *)lparam;
        data->ps.hi.iContextType = hi->iContextType;
        data->ps.hi.iCtrlId      = hi->iCtrlId;
        data->ps.hi.hItemHandle  = wine_server_user_handle( hi->hItemHandle );
        data->ps.hi.dwContextId  = hi->dwContextId;
        data->ps.hi.MousePos     = hi->MousePos;
        push_data( data, &data->ps.hi, sizeof(data->ps.hi) );
        return 0;
    }
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
        push_data( data, (STYLESTRUCT *)lparam, sizeof(STYLESTRUCT) );
        return 0;
    case WM_NCCALCSIZE:
        if (!wparam)
        {
            push_data( data, (RECT *)lparam, sizeof(RECT) );
            return sizeof(RECT);
        }
        else
        {
            NCCALCSIZE_PARAMS *ncp = (NCCALCSIZE_PARAMS *)lparam;
            data->ps.ncp.rgrc[0]         = ncp->rgrc[0];
            data->ps.ncp.rgrc[1]         = ncp->rgrc[1];
            data->ps.ncp.rgrc[2]         = ncp->rgrc[2];
            data->ps.ncp.hwnd            = wine_server_user_handle( ncp->lppos->hwnd );
            data->ps.ncp.hwndInsertAfter = wine_server_user_handle( ncp->lppos->hwndInsertAfter );
            data->ps.ncp.x               = ncp->lppos->x;
            data->ps.ncp.y               = ncp->lppos->y;
            data->ps.ncp.cx              = ncp->lppos->cx;
            data->ps.ncp.cy              = ncp->lppos->cy;
            data->ps.ncp.flags           = ncp->lppos->flags;
            push_data( data, &data->ps.ncp, sizeof(data->ps.ncp) );
            return sizeof(data->ps.ncp);
        }
    case WM_GETDLGCODE:
        if (lparam)
        {
            MSG *msg = (MSG *)lparam;
            data->ps.msg.hwnd    = wine_server_user_handle( msg->hwnd );
            data->ps.msg.message = msg->message;
            data->ps.msg.wParam  = msg->wParam;
            data->ps.msg.lParam  = msg->lParam;
            data->ps.msg.time    = msg->time;
            data->ps.msg.pt      = msg->pt;
            push_data( data, &data->ps.msg, sizeof(data->ps.msg) );
            return sizeof(data->ps.msg);
        }
        return 0;
    case SBM_SETSCROLLINFO:
        push_data( data, (SCROLLINFO *)lparam, sizeof(SCROLLINFO) );
        return 0;
    case SBM_GETSCROLLINFO:
        push_data( data, (SCROLLINFO *)lparam, sizeof(SCROLLINFO) );
        return sizeof(SCROLLINFO);
    case SBM_GETSCROLLBARINFO:
    {
        const SCROLLBARINFO *info = (const SCROLLBARINFO *)lparam;
        size_t size = min( info->cbSize, sizeof(SCROLLBARINFO) );
        push_data( data, info, size );
        return size;
    }
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
    {
        size_t size = 0;
        if (wparam) size += sizeof(DWORD);
        if (lparam) size += sizeof(DWORD);
        return size;
    }
    case EM_GETRECT:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
        return sizeof(RECT);
    case EM_SETRECT:
    case EM_SETRECTNP:
        push_data( data, (RECT *)lparam, sizeof(RECT) );
        return 0;
    case EM_GETLINE:
    {
        WORD *pw = (WORD *)lparam;
        push_data( data, pw, sizeof(*pw) );
        return *pw * sizeof(WCHAR);
    }
    case EM_SETTABSTOPS:
    case LB_SETTABSTOPS:
        if (wparam) push_data( data, (UINT *)lparam, sizeof(UINT) * wparam );
        return 0;
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
        if (combobox_has_strings( hwnd )) push_string( data, (LPWSTR)lparam );
        return 0;
    case CB_GETLBTEXT:
        if (!combobox_has_strings( hwnd )) return sizeof(ULONG_PTR);
        return (SendMessageW( hwnd, CB_GETLBTEXTLEN, wparam, 0 ) + 1) * sizeof(WCHAR);
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
        if (listbox_has_strings( hwnd )) push_string( data, (LPWSTR)lparam );
        return 0;
    case LB_GETTEXT:
        if (!listbox_has_strings( hwnd )) return sizeof(ULONG_PTR);
        return (SendMessageW( hwnd, LB_GETTEXTLEN, wparam, 0 ) + 1) * sizeof(WCHAR);
    case LB_GETSELITEMS:
        return wparam * sizeof(UINT);
    case WM_NEXTMENU:
    {
        MDINEXTMENU *mnm = (MDINEXTMENU *)lparam;
        data->ps.mnm.hmenuIn   = wine_server_user_handle( mnm->hmenuIn );
        data->ps.mnm.hmenuNext = wine_server_user_handle( mnm->hmenuNext );
        data->ps.mnm.hwndNext  = wine_server_user_handle( mnm->hwndNext );
        push_data( data, &data->ps.mnm, sizeof(data->ps.mnm) );
        return sizeof(data->ps.mnm);
    }
    case WM_SIZING:
    case WM_MOVING:
        push_data( data, (RECT *)lparam, sizeof(RECT) );
        return sizeof(RECT);
    case WM_MDICREATE:
    {
        MDICREATESTRUCTW *mcs = (MDICREATESTRUCTW *)lparam;
        data->ps.mcs.szClass = pack_ptr( mcs->szClass );
        data->ps.mcs.szTitle = pack_ptr( mcs->szTitle );
        data->ps.mcs.hOwner  = pack_ptr( mcs->hOwner );
        data->ps.mcs.x       = mcs->x;
        data->ps.mcs.y       = mcs->y;
        data->ps.mcs.cx      = mcs->cx;
        data->ps.mcs.cy      = mcs->cy;
        data->ps.mcs.style   = mcs->style;
        data->ps.mcs.lParam  = mcs->lParam;
        push_data( data, &data->ps.mcs, sizeof(data->ps.mcs) );
        if (!IS_INTRESOURCE(mcs->szClass)) push_string( data, mcs->szClass );
        if (!IS_INTRESOURCE(mcs->szTitle)) push_string( data, mcs->szTitle );
        return sizeof(data->ps.mcs);
    }
    case WM_MDIGETACTIVE:
        if (lparam) return sizeof(BOOL);
        return 0;
    case WM_DEVICECHANGE:
    {
        DEV_BROADCAST_HDR *header = (DEV_BROADCAST_HDR *)lparam;
        push_data( data, header, header->dbch_size );
        return 0;
    }
    case WM_WINE_KEYBOARD_LL_HOOK:
    {
        struct hook_extra_info *h_extra = (struct hook_extra_info *)lparam;
        data->ps.hook.handle = wine_server_user_handle( h_extra->handle );
        push_data( data, &data->ps.hook, sizeof(data->ps.hook) );
        push_data( data, (LPVOID)h_extra->lparam, sizeof(KBDLLHOOKSTRUCT) );
        return 0;
    }
    case WM_WINE_MOUSE_LL_HOOK:
    {
        struct hook_extra_info *h_extra = (struct hook_extra_info *)lparam;
        data->ps.hook.handle = wine_server_user_handle( h_extra->handle );
        push_data( data, &data->ps.hook, sizeof(data->ps.hook) );
        push_data( data, (LPVOID)h_extra->lparam, sizeof(MSLLHOOKSTRUCT) );
        return 0;
    }
    case WM_NCPAINT:
        if (wparam <= 1) return 0;
        FIXME( "WM_NCPAINT hdc packing not supported yet\n" );
        data->count = -1;
        return 0;
    case WM_PAINT:
        if (!wparam) return 0;
        /* fall through */

    /* these contain an HFONT */
    case WM_SETFONT:
    case WM_GETFONT:
    /* these contain an HDC */
    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
    case WM_PRINT:
    case WM_PRINTCLIENT:
    /* these contain an HGLOBAL */
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    /* these contain HICON */
    case WM_GETICON:
    case WM_SETICON:
    case WM_QUERYDRAGICON:
    case WM_QUERYPARKICON:
    /* these contain pointers */
    case WM_DROPOBJECT:
    case WM_QUERYDROPOBJECT:
    case WM_DRAGLOOP:
    case WM_DRAGSELECT:
    case WM_DRAGMOVE:
        FIXME( "msg %x (%s) not supported yet\n", message, SPY_GetMsgName(message, hwnd) );
        data->count = -1;
        return 0;
    }
    return 0;
}


/***********************************************************************
 *		unpack_message
 *
 * Unpack a message received from another process.
 */
static BOOL unpack_message( HWND hwnd, UINT message, WPARAM *wparam, LPARAM *lparam,
                            void **buffer, size_t size )
{
    size_t minsize = 0;
    union packed_structs *ps = *buffer;

    switch(message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
    {
        CREATESTRUCTW cs;
        WCHAR *str = (WCHAR *)(&ps->cs + 1);
        if (size < sizeof(ps->cs)) return FALSE;
        size -= sizeof(ps->cs);
        cs.lpCreateParams = unpack_ptr( ps->cs.lpCreateParams );
        cs.hInstance      = unpack_ptr( ps->cs.hInstance );
        cs.hMenu          = wine_server_ptr_handle( ps->cs.hMenu );
        cs.hwndParent     = wine_server_ptr_handle( ps->cs.hwndParent );
        cs.cy             = ps->cs.cy;
        cs.cx             = ps->cs.cx;
        cs.y              = ps->cs.y;
        cs.x              = ps->cs.x;
        cs.style          = ps->cs.style;
        cs.dwExStyle      = ps->cs.dwExStyle;
        cs.lpszName       = unpack_ptr( ps->cs.lpszName );
        cs.lpszClass      = unpack_ptr( ps->cs.lpszClass );
        if (ps->cs.lpszName >> 16)
        {
            if (!check_string( str, size )) return FALSE;
            cs.lpszName = str;
            size -= (strlenW(str) + 1) * sizeof(WCHAR);
            str += strlenW(str) + 1;
        }
        if (ps->cs.lpszClass >> 16)
        {
            if (!check_string( str, size )) return FALSE;
            cs.lpszClass = str;
        }
        memcpy( &ps->cs, &cs, sizeof(cs) );
        break;
    }
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        if (!get_buffer_space( buffer, (*wparam * sizeof(WCHAR)) )) return FALSE;
        break;
    case WM_WININICHANGE:
        if (!*lparam) return TRUE;
        /* fall through */
    case WM_SETTEXT:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        if (!check_string( *buffer, size )) return FALSE;
        break;
    case WM_GETMINMAXINFO:
        minsize = sizeof(MINMAXINFO);
        break;
    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT dis;
        if (size < sizeof(ps->dis)) return FALSE;
        dis.CtlType    = ps->dis.CtlType;
        dis.CtlID      = ps->dis.CtlID;
        dis.itemID     = ps->dis.itemID;
        dis.itemAction = ps->dis.itemAction;
        dis.itemState  = ps->dis.itemState;
        dis.hwndItem   = wine_server_ptr_handle( ps->dis.hwndItem );
        dis.hDC        = wine_server_ptr_handle( ps->dis.hDC );
        dis.rcItem     = ps->dis.rcItem;
        dis.itemData   = (ULONG_PTR)unpack_ptr( ps->dis.itemData );
        memcpy( &ps->dis, &dis, sizeof(dis) );
        break;
    }
    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT mis;
        if (size < sizeof(ps->mis)) return FALSE;
        mis.CtlType    = ps->mis.CtlType;
        mis.CtlID      = ps->mis.CtlID;
        mis.itemID     = ps->mis.itemID;
        mis.itemWidth  = ps->mis.itemWidth;
        mis.itemHeight = ps->mis.itemHeight;
        mis.itemData   = (ULONG_PTR)unpack_ptr( ps->mis.itemData );
        memcpy( &ps->mis, &mis, sizeof(mis) );
        break;
    }
    case WM_DELETEITEM:
    {
        DELETEITEMSTRUCT dls;
        if (size < sizeof(ps->dls)) return FALSE;
        dls.CtlType    = ps->dls.CtlType;
        dls.CtlID      = ps->dls.CtlID;
        dls.itemID     = ps->dls.itemID;
        dls.hwndItem   = wine_server_ptr_handle( ps->dls.hwndItem );
        dls.itemData   = (ULONG_PTR)unpack_ptr( ps->dls.itemData );
        memcpy( &ps->dls, &dls, sizeof(dls) );
        break;
    }
    case WM_COMPAREITEM:
    {
        COMPAREITEMSTRUCT cis;
        if (size < sizeof(ps->cis)) return FALSE;
        cis.CtlType    = ps->cis.CtlType;
        cis.CtlID      = ps->cis.CtlID;
        cis.hwndItem   = wine_server_ptr_handle( ps->cis.hwndItem );
        cis.itemID1    = ps->cis.itemID1;
        cis.itemData1  = (ULONG_PTR)unpack_ptr( ps->cis.itemData1 );
        cis.itemID2    = ps->cis.itemID2;
        cis.itemData2  = (ULONG_PTR)unpack_ptr( ps->cis.itemData2 );
        cis.dwLocaleId = ps->cis.dwLocaleId;
        memcpy( &ps->cis, &cis, sizeof(cis) );
        break;
    }
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    case WM_WINE_SETWINDOWPOS:
    {
        WINDOWPOS wp;
        if (size < sizeof(ps->wp)) return FALSE;
        wp.hwnd            = wine_server_ptr_handle( ps->wp.hwnd );
        wp.hwndInsertAfter = wine_server_ptr_handle( ps->wp.hwndInsertAfter );
        wp.x               = ps->wp.x;
        wp.y               = ps->wp.y;
        wp.cx              = ps->wp.cx;
        wp.cy              = ps->wp.cy;
        wp.flags           = ps->wp.flags;
        memcpy( &ps->wp, &wp, sizeof(wp) );
        break;
    }
    case WM_COPYDATA:
    {
        COPYDATASTRUCT cds;
        if (size < sizeof(ps->cds)) return FALSE;
        cds.dwData = (ULONG_PTR)unpack_ptr( ps->cds.dwData );
        if (ps->cds.lpData)
        {
            cds.cbData = ps->cds.cbData;
            cds.lpData = &ps->cds + 1;
            minsize = sizeof(ps->cds) + cds.cbData;
        }
        else
        {
            cds.cbData = 0;
            cds.lpData = 0;
        }
        memcpy( &ps->cds, &cds, sizeof(cds) );
        break;
    }
    case WM_NOTIFY:
        /* WM_NOTIFY cannot be sent across processes (MSDN) */
        return FALSE;
    case WM_HELP:
    {
        HELPINFO hi;
        if (size < sizeof(ps->hi)) return FALSE;
        hi.cbSize       = sizeof(hi);
        hi.iContextType = ps->hi.iContextType;
        hi.iCtrlId      = ps->hi.iCtrlId;
        hi.hItemHandle  = wine_server_ptr_handle( ps->hi.hItemHandle );
        hi.dwContextId  = (ULONG_PTR)unpack_ptr( ps->hi.dwContextId );
        hi.MousePos     = ps->hi.MousePos;
        memcpy( &ps->hi, &hi, sizeof(hi) );
        break;
    }
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
        minsize = sizeof(STYLESTRUCT);
        break;
    case WM_NCCALCSIZE:
        if (!*wparam) minsize = sizeof(RECT);
        else
        {
            NCCALCSIZE_PARAMS ncp;
            WINDOWPOS wp;
            if (size < sizeof(ps->ncp)) return FALSE;
            ncp.rgrc[0]        = ps->ncp.rgrc[0];
            ncp.rgrc[1]        = ps->ncp.rgrc[1];
            ncp.rgrc[2]        = ps->ncp.rgrc[2];
            wp.hwnd            = wine_server_ptr_handle( ps->ncp.hwnd );
            wp.hwndInsertAfter = wine_server_ptr_handle( ps->ncp.hwndInsertAfter );
            wp.x               = ps->ncp.x;
            wp.y               = ps->ncp.y;
            wp.cx              = ps->ncp.cx;
            wp.cy              = ps->ncp.cy;
            wp.flags           = ps->ncp.flags;
            ncp.lppos = (WINDOWPOS *)((NCCALCSIZE_PARAMS *)&ps->ncp + 1);
            memcpy( &ps->ncp, &ncp, sizeof(ncp) );
            *ncp.lppos = wp;
        }
        break;
    case WM_GETDLGCODE:
        if (*lparam)
        {
            MSG msg;
            if (size < sizeof(ps->msg)) return FALSE;
            msg.hwnd    = wine_server_ptr_handle( ps->msg.hwnd );
            msg.message = ps->msg.message;
            msg.wParam  = (ULONG_PTR)unpack_ptr( ps->msg.wParam );
            msg.lParam  = (ULONG_PTR)unpack_ptr( ps->msg.lParam );
            msg.time    = ps->msg.time;
            msg.pt      = ps->msg.pt;
            memcpy( &ps->msg, &msg, sizeof(msg) );
            break;
        }
        return TRUE;
    case SBM_SETSCROLLINFO:
        minsize = sizeof(SCROLLINFO);
        break;
    case SBM_GETSCROLLINFO:
        if (!get_buffer_space( buffer, sizeof(SCROLLINFO ))) return FALSE;
        break;
    case SBM_GETSCROLLBARINFO:
        if (!get_buffer_space( buffer, sizeof(SCROLLBARINFO ))) return FALSE;
        break;
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        if (*wparam || *lparam)
        {
            if (!get_buffer_space( buffer, 2*sizeof(DWORD) )) return FALSE;
            if (*wparam) *wparam = (WPARAM)*buffer;
            if (*lparam) *lparam = (LPARAM)((DWORD *)*buffer + 1);
        }
        return TRUE;
    case EM_GETRECT:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
        if (!get_buffer_space( buffer, sizeof(RECT) )) return FALSE;
        break;
    case EM_SETRECT:
    case EM_SETRECTNP:
        minsize = sizeof(RECT);
        break;
    case EM_GETLINE:
    {
        WORD len;
        if (size < sizeof(WORD)) return FALSE;
        len = *(WORD *)*buffer;
        if (!get_buffer_space( buffer, (len + 1) * sizeof(WCHAR) )) return FALSE;
        *lparam = (LPARAM)*buffer + sizeof(WORD);  /* don't erase WORD at start of buffer */
        return TRUE;
    }
    case EM_SETTABSTOPS:
    case LB_SETTABSTOPS:
        if (!*wparam) return TRUE;
        minsize = *wparam * sizeof(UINT);
        break;
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
    case LB_ADDSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
        if (!*buffer) return TRUE;
        if (!check_string( *buffer, size )) return FALSE;
        break;
    case CB_GETLBTEXT:
    {
        size = sizeof(ULONG_PTR);
        if (combobox_has_strings( hwnd ))
            size = (SendMessageW( hwnd, CB_GETLBTEXTLEN, *wparam, 0 ) + 1) * sizeof(WCHAR);
        if (!get_buffer_space( buffer, size )) return FALSE;
        break;
    }
    case LB_GETTEXT:
    {
        size = sizeof(ULONG_PTR);
        if (listbox_has_strings( hwnd ))
            size = (SendMessageW( hwnd, LB_GETTEXTLEN, *wparam, 0 ) + 1) * sizeof(WCHAR);
        if (!get_buffer_space( buffer, size )) return FALSE;
        break;
    }
    case LB_GETSELITEMS:
        if (!get_buffer_space( buffer, *wparam * sizeof(UINT) )) return FALSE;
        break;
    case WM_NEXTMENU:
    {
        MDINEXTMENU mnm;
        if (size < sizeof(ps->mnm)) return FALSE;
        mnm.hmenuIn   = wine_server_ptr_handle( ps->mnm.hmenuIn );
        mnm.hmenuNext = wine_server_ptr_handle( ps->mnm.hmenuNext );
        mnm.hwndNext  = wine_server_ptr_handle( ps->mnm.hwndNext );
        memcpy( &ps->mnm, &mnm, sizeof(mnm) );
        break;
    }
    case WM_SIZING:
    case WM_MOVING:
        minsize = sizeof(RECT);
        if (!get_buffer_space( buffer, sizeof(RECT) )) return FALSE;
        break;
    case WM_MDICREATE:
    {
        MDICREATESTRUCTW mcs;
        WCHAR *str = (WCHAR *)(&ps->mcs + 1);
        if (size < sizeof(ps->mcs)) return FALSE;
        size -= sizeof(ps->mcs);

        mcs.szClass = unpack_ptr( ps->mcs.szClass );
        mcs.szTitle = unpack_ptr( ps->mcs.szTitle );
        mcs.hOwner  = unpack_ptr( ps->mcs.hOwner );
        mcs.x       = ps->mcs.x;
        mcs.y       = ps->mcs.y;
        mcs.cx      = ps->mcs.cx;
        mcs.cy      = ps->mcs.cy;
        mcs.style   = ps->mcs.style;
        mcs.lParam  = (LPARAM)unpack_ptr( ps->mcs.lParam );
        if (ps->mcs.szClass >> 16)
        {
            if (!check_string( str, size )) return FALSE;
            mcs.szClass = str;
            size -= (strlenW(str) + 1) * sizeof(WCHAR);
            str += strlenW(str) + 1;
        }
        if (ps->mcs.szTitle >> 16)
        {
            if (!check_string( str, size )) return FALSE;
            mcs.szTitle = str;
        }
        memcpy( &ps->mcs, &mcs, sizeof(mcs) );
        break;
    }
    case WM_MDIGETACTIVE:
        if (!*lparam) return TRUE;
        if (!get_buffer_space( buffer, sizeof(BOOL) )) return FALSE;
        break;
    case WM_DEVICECHANGE:
        minsize = sizeof(DEV_BROADCAST_HDR);
        break;
    case WM_WINE_KEYBOARD_LL_HOOK:
    case WM_WINE_MOUSE_LL_HOOK:
    {
        struct hook_extra_info h_extra;
        minsize = sizeof(ps->hook) +
                  (message == WM_WINE_KEYBOARD_LL_HOOK ? sizeof(KBDLLHOOKSTRUCT)
                                                       : sizeof(MSLLHOOKSTRUCT));
        if (size < minsize) return FALSE;
        h_extra.handle = wine_server_ptr_handle( ps->hook.handle );
        h_extra.lparam = (LPARAM)(&ps->hook + 1);
        memcpy( &ps->hook, &h_extra, sizeof(h_extra) );
        break;
    }
    case WM_NCPAINT:
        if (*wparam <= 1) return TRUE;
        FIXME( "WM_NCPAINT hdc unpacking not supported\n" );
        return FALSE;
    case WM_PAINT:
        if (!*wparam) return TRUE;
        /* fall through */

    /* these contain an HFONT */
    case WM_SETFONT:
    case WM_GETFONT:
    /* these contain an HDC */
    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
    case WM_PRINT:
    case WM_PRINTCLIENT:
    /* these contain an HGLOBAL */
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    /* these contain HICON */
    case WM_GETICON:
    case WM_SETICON:
    case WM_QUERYDRAGICON:
    case WM_QUERYPARKICON:
    /* these contain pointers */
    case WM_DROPOBJECT:
    case WM_QUERYDROPOBJECT:
    case WM_DRAGLOOP:
    case WM_DRAGSELECT:
    case WM_DRAGMOVE:
        FIXME( "msg %x (%s) not supported yet\n", message, SPY_GetMsgName(message, hwnd) );
        return FALSE;

    default:
        return TRUE; /* message doesn't need any unpacking */
    }

    /* default exit for most messages: check minsize and store buffer in lparam */
    if (size < minsize) return FALSE;
    *lparam = (LPARAM)*buffer;
    return TRUE;
}


/***********************************************************************
 *		pack_reply
 *
 * Pack a reply to a message for sending to another process.
 */
static void pack_reply( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                        LRESULT res, struct packed_message *data )
{
    data->count = 0;
    switch(message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
    {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
        data->ps.cs.lpCreateParams = (ULONG_PTR)cs->lpCreateParams;
        data->ps.cs.hInstance      = (ULONG_PTR)cs->hInstance;
        data->ps.cs.hMenu          = wine_server_user_handle( cs->hMenu );
        data->ps.cs.hwndParent     = wine_server_user_handle( cs->hwndParent );
        data->ps.cs.cy             = cs->cy;
        data->ps.cs.cx             = cs->cx;
        data->ps.cs.y              = cs->y;
        data->ps.cs.x              = cs->x;
        data->ps.cs.style          = cs->style;
        data->ps.cs.dwExStyle      = cs->dwExStyle;
        data->ps.cs.lpszName       = (ULONG_PTR)cs->lpszName;
        data->ps.cs.lpszClass      = (ULONG_PTR)cs->lpszClass;
        push_data( data, &data->ps.cs, sizeof(data->ps.cs) );
        break;
    }
    case WM_GETTEXT:
    case CB_GETLBTEXT:
    case LB_GETTEXT:
        push_data( data, (WCHAR *)lparam, (res + 1) * sizeof(WCHAR) );
        break;
    case WM_GETMINMAXINFO:
        push_data( data, (MINMAXINFO *)lparam, sizeof(MINMAXINFO) );
        break;
    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lparam;
        data->ps.mis.CtlType    = mis->CtlType;
        data->ps.mis.CtlID      = mis->CtlID;
        data->ps.mis.itemID     = mis->itemID;
        data->ps.mis.itemWidth  = mis->itemWidth;
        data->ps.mis.itemHeight = mis->itemHeight;
        data->ps.mis.itemData   = mis->itemData;
        push_data( data, &data->ps.mis, sizeof(data->ps.mis) );
        break;
    }
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
        WINDOWPOS *wp = (WINDOWPOS *)lparam;
        data->ps.wp.hwnd            = wine_server_user_handle( wp->hwnd );
        data->ps.wp.hwndInsertAfter = wine_server_user_handle( wp->hwndInsertAfter );
        data->ps.wp.x               = wp->x;
        data->ps.wp.y               = wp->y;
        data->ps.wp.cx              = wp->cx;
        data->ps.wp.cy              = wp->cy;
        data->ps.wp.flags           = wp->flags;
        push_data( data, &data->ps.wp, sizeof(data->ps.wp) );
        break;
    }
    case WM_GETDLGCODE:
        if (lparam)
        {
            MSG *msg = (MSG *)lparam;
            data->ps.msg.hwnd    = wine_server_user_handle( msg->hwnd );
            data->ps.msg.message = msg->message;
            data->ps.msg.wParam  = msg->wParam;
            data->ps.msg.lParam  = msg->lParam;
            data->ps.msg.time    = msg->time;
            data->ps.msg.pt      = msg->pt;
            push_data( data, &data->ps.msg, sizeof(data->ps.msg) );
        }
        break;
    case SBM_GETSCROLLINFO:
        push_data( data, (SCROLLINFO *)lparam, sizeof(SCROLLINFO) );
        break;
    case EM_GETRECT:
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
    case WM_SIZING:
    case WM_MOVING:
        push_data( data, (RECT *)lparam, sizeof(RECT) );
        break;
    case EM_GETLINE:
    {
        WORD *ptr = (WORD *)lparam;
        push_data( data, ptr, ptr[-1] * sizeof(WCHAR) );
        break;
    }
    case LB_GETSELITEMS:
        push_data( data, (UINT *)lparam, wparam * sizeof(UINT) );
        break;
    case WM_MDIGETACTIVE:
        if (lparam) push_data( data, (BOOL *)lparam, sizeof(BOOL) );
        break;
    case WM_NCCALCSIZE:
        if (!wparam)
            push_data( data, (RECT *)lparam, sizeof(RECT) );
        else
        {
            NCCALCSIZE_PARAMS *ncp = (NCCALCSIZE_PARAMS *)lparam;
            data->ps.ncp.rgrc[0]         = ncp->rgrc[0];
            data->ps.ncp.rgrc[1]         = ncp->rgrc[1];
            data->ps.ncp.rgrc[2]         = ncp->rgrc[2];
            data->ps.ncp.hwnd            = wine_server_user_handle( ncp->lppos->hwnd );
            data->ps.ncp.hwndInsertAfter = wine_server_user_handle( ncp->lppos->hwndInsertAfter );
            data->ps.ncp.x               = ncp->lppos->x;
            data->ps.ncp.y               = ncp->lppos->y;
            data->ps.ncp.cx              = ncp->lppos->cx;
            data->ps.ncp.cy              = ncp->lppos->cy;
            data->ps.ncp.flags           = ncp->lppos->flags;
            push_data( data, &data->ps.ncp, sizeof(data->ps.ncp) );
        }
        break;
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        if (wparam) push_data( data, (DWORD *)wparam, sizeof(DWORD) );
        if (lparam) push_data( data, (DWORD *)lparam, sizeof(DWORD) );
        break;
    case WM_NEXTMENU:
    {
        MDINEXTMENU *mnm = (MDINEXTMENU *)lparam;
        data->ps.mnm.hmenuIn   = wine_server_user_handle( mnm->hmenuIn );
        data->ps.mnm.hmenuNext = wine_server_user_handle( mnm->hmenuNext );
        data->ps.mnm.hwndNext  = wine_server_user_handle( mnm->hwndNext );
        push_data( data, &data->ps.mnm, sizeof(data->ps.mnm) );
        break;
    }
    case WM_MDICREATE:
    {
        MDICREATESTRUCTW *mcs = (MDICREATESTRUCTW *)lparam;
        data->ps.mcs.szClass = pack_ptr( mcs->szClass );
        data->ps.mcs.szTitle = pack_ptr( mcs->szTitle );
        data->ps.mcs.hOwner  = pack_ptr( mcs->hOwner );
        data->ps.mcs.x       = mcs->x;
        data->ps.mcs.y       = mcs->y;
        data->ps.mcs.cx      = mcs->cx;
        data->ps.mcs.cy      = mcs->cy;
        data->ps.mcs.style   = mcs->style;
        data->ps.mcs.lParam  = mcs->lParam;
        push_data( data, &data->ps.mcs, sizeof(data->ps.mcs) );
        break;
    }
    case WM_ASKCBFORMATNAME:
        push_data( data, (WCHAR *)lparam, (strlenW((WCHAR *)lparam) + 1) * sizeof(WCHAR) );
        break;
    }
}


/***********************************************************************
 *		unpack_reply
 *
 * Unpack a message reply received from another process.
 */
static void unpack_reply( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                          void *buffer, size_t size )
{
    union packed_structs *ps = buffer;

    switch(message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        if (size >= sizeof(ps->cs))
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
            cs->lpCreateParams = unpack_ptr( ps->cs.lpCreateParams );
            cs->hInstance      = unpack_ptr( ps->cs.hInstance );
            cs->hMenu          = wine_server_ptr_handle( ps->cs.hMenu );
            cs->hwndParent     = wine_server_ptr_handle( ps->cs.hwndParent );
            cs->cy             = ps->cs.cy;
            cs->cx             = ps->cs.cx;
            cs->y              = ps->cs.y;
            cs->x              = ps->cs.x;
            cs->style          = ps->cs.style;
            cs->dwExStyle      = ps->cs.dwExStyle;
            /* don't allow changing name and class pointers */
        }
        break;
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        memcpy( (WCHAR *)lparam, buffer, min( wparam*sizeof(WCHAR), size ));
        break;
    case WM_GETMINMAXINFO:
        memcpy( (MINMAXINFO *)lparam, buffer, min( sizeof(MINMAXINFO), size ));
        break;
    case WM_MEASUREITEM:
        if (size >= sizeof(ps->mis))
        {
            MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lparam;
            mis->CtlType    = ps->mis.CtlType;
            mis->CtlID      = ps->mis.CtlID;
            mis->itemID     = ps->mis.itemID;
            mis->itemWidth  = ps->mis.itemWidth;
            mis->itemHeight = ps->mis.itemHeight;
            mis->itemData   = (ULONG_PTR)unpack_ptr( ps->mis.itemData );
        }
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        if (size >= sizeof(ps->wp))
        {
            WINDOWPOS *wp = (WINDOWPOS *)lparam;
            wp->hwnd            = wine_server_ptr_handle( ps->wp.hwnd );
            wp->hwndInsertAfter = wine_server_ptr_handle( ps->wp.hwndInsertAfter );
            wp->x               = ps->wp.x;
            wp->y               = ps->wp.y;
            wp->cx              = ps->wp.cx;
            wp->cy              = ps->wp.cy;
            wp->flags           = ps->wp.flags;
        }
        break;
    case WM_GETDLGCODE:
        if (lparam && size >= sizeof(ps->msg))
        {
            MSG *msg = (MSG *)lparam;
            msg->hwnd    = wine_server_ptr_handle( ps->msg.hwnd );
            msg->message = ps->msg.message;
            msg->wParam  = (ULONG_PTR)unpack_ptr( ps->msg.wParam );
            msg->lParam  = (ULONG_PTR)unpack_ptr( ps->msg.lParam );
            msg->time    = ps->msg.time;
            msg->pt      = ps->msg.pt;
        }
        break;
    case SBM_GETSCROLLINFO:
        memcpy( (SCROLLINFO *)lparam, buffer, min( sizeof(SCROLLINFO), size ));
        break;
    case SBM_GETSCROLLBARINFO:
        memcpy( (SCROLLBARINFO *)lparam, buffer, min( sizeof(SCROLLBARINFO), size ));
        break;
    case EM_GETRECT:
    case CB_GETDROPPEDCONTROLRECT:
    case LB_GETITEMRECT:
    case WM_SIZING:
    case WM_MOVING:
        memcpy( (RECT *)lparam, buffer, min( sizeof(RECT), size ));
        break;
    case EM_GETLINE:
        size = min( size, (size_t)*(WORD *)lparam );
        memcpy( (WCHAR *)lparam, buffer, size );
        break;
    case LB_GETSELITEMS:
        memcpy( (UINT *)lparam, buffer, min( wparam*sizeof(UINT), size ));
        break;
    case LB_GETTEXT:
    case CB_GETLBTEXT:
        memcpy( (WCHAR *)lparam, buffer, size );
        break;
    case WM_NEXTMENU:
        if (size >= sizeof(ps->mnm))
        {
            MDINEXTMENU *mnm = (MDINEXTMENU *)lparam;
            mnm->hmenuIn   = wine_server_ptr_handle( ps->mnm.hmenuIn );
            mnm->hmenuNext = wine_server_ptr_handle( ps->mnm.hmenuNext );
            mnm->hwndNext  = wine_server_ptr_handle( ps->mnm.hwndNext );
        }
        break;
    case WM_MDIGETACTIVE:
        if (lparam) memcpy( (BOOL *)lparam, buffer, min( sizeof(BOOL), size ));
        break;
    case WM_NCCALCSIZE:
        if (!wparam)
            memcpy( (RECT *)lparam, buffer, min( sizeof(RECT), size ));
        else if (size >= sizeof(ps->ncp))
        {
            NCCALCSIZE_PARAMS *ncp = (NCCALCSIZE_PARAMS *)lparam;
            ncp->rgrc[0]                = ps->ncp.rgrc[0];
            ncp->rgrc[1]                = ps->ncp.rgrc[1];
            ncp->rgrc[2]                = ps->ncp.rgrc[2];
            ncp->lppos->hwnd            = wine_server_ptr_handle( ps->ncp.hwnd );
            ncp->lppos->hwndInsertAfter = wine_server_ptr_handle( ps->ncp.hwndInsertAfter );
            ncp->lppos->x               = ps->ncp.x;
            ncp->lppos->y               = ps->ncp.y;
            ncp->lppos->cx              = ps->ncp.cx;
            ncp->lppos->cy              = ps->ncp.cy;
            ncp->lppos->flags           = ps->ncp.flags;
        }
        break;
    case EM_GETSEL:
    case SBM_GETRANGE:
    case CB_GETEDITSEL:
        if (wparam)
        {
            memcpy( (DWORD *)wparam, buffer, min( sizeof(DWORD), size ));
            if (size <= sizeof(DWORD)) break;
            size -= sizeof(DWORD);
            buffer = (DWORD *)buffer + 1;
        }
        if (lparam) memcpy( (DWORD *)lparam, buffer, min( sizeof(DWORD), size ));
        break;
    case WM_MDICREATE:
        if (size >= sizeof(ps->mcs))
        {
            MDICREATESTRUCTW *mcs = (MDICREATESTRUCTW *)lparam;
            mcs->hOwner  = unpack_ptr( ps->mcs.hOwner );
            mcs->x       = ps->mcs.x;
            mcs->y       = ps->mcs.y;
            mcs->cx      = ps->mcs.cx;
            mcs->cy      = ps->mcs.cy;
            mcs->style   = ps->mcs.style;
            mcs->lParam  = (LPARAM)unpack_ptr( ps->mcs.lParam );
            /* don't allow changing class and title pointers */
        }
        break;
    default:
        ERR( "should not happen: unexpected message %x\n", message );
        break;
    }
}


/***********************************************************************
 *           reply_message
 *
 * Send a reply to a sent message.
 */
static void reply_message( struct received_message_info *info, LRESULT result, BOOL remove )
{
    struct packed_message data;
    int i, replied = info->flags & ISMEX_REPLIED;

    if (info->flags & ISMEX_NOTIFY) return;  /* notify messages don't get replies */
    if (!remove && replied) return;  /* replied already */

    memset( &data, 0, sizeof(data) );
    info->flags |= ISMEX_REPLIED;

    if (info->type == MSG_OTHER_PROCESS && !replied)
    {
        pack_reply( info->msg.hwnd, info->msg.message, info->msg.wParam,
                    info->msg.lParam, result, &data );
    }

    SERVER_START_REQ( reply_message )
    {
        req->result = result;
        req->remove = remove;
        for (i = 0; i < data.count; i++) wine_server_add_data( req, data.data[i], data.size[i] );
        wine_server_call( req );
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           handle_internal_message
 *
 * Handle an internal Wine message instead of calling the window proc.
 */
static LRESULT handle_internal_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch(msg)
    {
    case WM_WINE_DESTROYWINDOW:
        return WIN_DestroyWindow( hwnd );
    case WM_WINE_SETWINDOWPOS:
        if (is_desktop_window( hwnd )) return 0;
        return USER_SetWindowPos( (WINDOWPOS *)lparam );
    case WM_WINE_SHOWWINDOW:
        if (is_desktop_window( hwnd )) return 0;
        return ShowWindow( hwnd, wparam );
    case WM_WINE_SETPARENT:
        if (is_desktop_window( hwnd )) return 0;
        return (LRESULT)SetParent( hwnd, (HWND)wparam );
    case WM_WINE_SETWINDOWLONG:
        return WIN_SetWindowLong( hwnd, (short)LOWORD(wparam), HIWORD(wparam), lparam, TRUE );
    case WM_WINE_ENABLEWINDOW:
        if (is_desktop_window( hwnd )) return 0;
        return EnableWindow( hwnd, wparam );
    case WM_WINE_SETACTIVEWINDOW:
        if (is_desktop_window( hwnd )) return 0;
        return (LRESULT)SetActiveWindow( (HWND)wparam );
    case WM_WINE_KEYBOARD_LL_HOOK:
    case WM_WINE_MOUSE_LL_HOOK:
    {
        struct hook_extra_info *h_extra = (struct hook_extra_info *)lparam;

        return call_current_hook( h_extra->handle, HC_ACTION, wparam, h_extra->lparam );
    }
    default:
        if (msg >= WM_WINE_FIRST_DRIVER_MSG && msg <= WM_WINE_LAST_DRIVER_MSG)
            return USER_Driver->pWindowMessage( hwnd, msg, wparam, lparam );
        FIXME( "unknown internal message %x\n", msg );
        return 0;
    }
}

/* since the WM_DDE_ACK response to a WM_DDE_EXECUTE message should contain the handle
 * to the memory handle, we keep track (in the server side) of all pairs of handle
 * used (the client passes its value and the content of the memory handle), and
 * the server stored both values (the client, and the local one, created after the
 * content). When a ACK message is generated, the list of pair is searched for a
 * matching pair, so that the client memory handle can be returned.
 */
struct DDE_pair {
    HGLOBAL     client_hMem;
    HGLOBAL     server_hMem;
};

static      struct DDE_pair*    dde_pairs;
static      int                 dde_num_alloc;
static      int                 dde_num_used;

static CRITICAL_SECTION dde_crst;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &dde_crst,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dde_crst") }
};
static CRITICAL_SECTION dde_crst = { &critsect_debug, -1, 0, 0, 0, 0 };

static BOOL dde_add_pair(HGLOBAL chm, HGLOBAL shm)
{
    int  i;
#define GROWBY  4

    EnterCriticalSection(&dde_crst);

    /* now remember the pair of hMem on both sides */
    if (dde_num_used == dde_num_alloc)
    {
        struct DDE_pair* tmp;
	if (dde_pairs)
	    tmp  = HeapReAlloc( GetProcessHeap(), 0, dde_pairs,
                                            (dde_num_alloc + GROWBY) * sizeof(struct DDE_pair));
	else
	    tmp  = HeapAlloc( GetProcessHeap(), 0, 
                                            (dde_num_alloc + GROWBY) * sizeof(struct DDE_pair));

        if (!tmp)
        {
            LeaveCriticalSection(&dde_crst);
            return FALSE;
        }
        dde_pairs = tmp;
        /* zero out newly allocated part */
        memset(&dde_pairs[dde_num_alloc], 0, GROWBY * sizeof(struct DDE_pair));
        dde_num_alloc += GROWBY;
    }
#undef GROWBY
    for (i = 0; i < dde_num_alloc; i++)
    {
        if (dde_pairs[i].server_hMem == 0)
        {
            dde_pairs[i].client_hMem = chm;
            dde_pairs[i].server_hMem = shm;
            dde_num_used++;
            break;
        }
    }
    LeaveCriticalSection(&dde_crst);
    return TRUE;
}

static HGLOBAL dde_get_pair(HGLOBAL shm)
{
    int  i;
    HGLOBAL     ret = 0;

    EnterCriticalSection(&dde_crst);
    for (i = 0; i < dde_num_alloc; i++)
    {
        if (dde_pairs[i].server_hMem == shm)
        {
            /* free this pair */
            dde_pairs[i].server_hMem = 0;
            dde_num_used--;
            ret = dde_pairs[i].client_hMem;
            break;
        }
    }
    LeaveCriticalSection(&dde_crst);
    return ret;
}

/***********************************************************************
 *		post_dde_message
 *
 * Post a DDE message
 */
static BOOL post_dde_message( struct packed_message *data, const struct send_message_info *info )
{
    void*       ptr = NULL;
    int         size = 0;
    UINT_PTR    uiLo, uiHi;
    LPARAM      lp = 0;
    HGLOBAL     hunlock = 0;
    int         i;
    DWORD       res;

    if (!UnpackDDElParam( info->msg, info->lparam, &uiLo, &uiHi ))
        return FALSE;

    lp = info->lparam;
    switch (info->msg)
    {
        /* DDE messages which don't require packing are:
         * WM_DDE_INITIATE
         * WM_DDE_TERMINATE
         * WM_DDE_REQUEST
         * WM_DDE_UNADVISE
         */
    case WM_DDE_ACK:
        if (HIWORD(uiHi))
        {
            /* uiHi should contain a hMem from WM_DDE_EXECUTE */
            HGLOBAL h = dde_get_pair( (HANDLE)uiHi );
            if (h)
            {
                ULONGLONG hpack = pack_ptr( h );
                /* send back the value of h on the other side */
                push_data( data, &hpack, sizeof(hpack) );
                lp = uiLo;
                TRACE( "send dde-ack %lx %08lx => %p\n", uiLo, uiHi, h );
            }
        }
        else
        {
            /* uiHi should contain either an atom or 0 */
            TRACE( "send dde-ack %lx atom=%lx\n", uiLo, uiHi );
            lp = MAKELONG( uiLo, uiHi );
        }
        break;
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        size = 0;
        if (uiLo)
        {
            size = GlobalSize( (HGLOBAL)uiLo ) ;
            if ((info->msg == WM_DDE_ADVISE && size < sizeof(DDEADVISE)) ||
                (info->msg == WM_DDE_DATA   && size < FIELD_OFFSET(DDEDATA, Value)) ||
                (info->msg == WM_DDE_POKE   && size < FIELD_OFFSET(DDEPOKE, Value))
                )
            return FALSE;
        }
        else if (info->msg != WM_DDE_DATA) return FALSE;

        lp = uiHi;
        if (uiLo)
        {
            if ((ptr = GlobalLock( (HGLOBAL)uiLo) ))
            {
                DDEDATA *dde_data = ptr;
                TRACE("unused %d, fResponse %d, fRelease %d, fDeferUpd %d, fAckReq %d, cfFormat %d\n",
                       dde_data->unused, dde_data->fResponse, dde_data->fRelease,
                       dde_data->reserved, dde_data->fAckReq, dde_data->cfFormat);
                push_data( data, ptr, size );
                hunlock = (HGLOBAL)uiLo;
            }
        }
        TRACE( "send ddepack %u %lx\n", size, uiHi );
        break;
    case WM_DDE_EXECUTE:
        if (info->lparam)
        {
            if ((ptr = GlobalLock( (HGLOBAL)info->lparam) ))
            {
                push_data(data, ptr, GlobalSize( (HGLOBAL)info->lparam ));
                /* so that the other side can send it back on ACK */
                lp = info->lparam;
                hunlock = (HGLOBAL)info->lparam;
            }
        }
        break;
    }
    SERVER_START_REQ( send_message )
    {
        req->id      = info->dest_tid;
        req->type    = info->type;
        req->flags   = 0;
        req->win     = wine_server_user_handle( info->hwnd );
        req->msg     = info->msg;
        req->wparam  = info->wparam;
        req->lparam  = lp;
        req->timeout = TIMEOUT_INFINITE;
        for (i = 0; i < data->count; i++)
            wine_server_add_data( req, data->data[i], data->size[i] );
        if ((res = wine_server_call( req )))
        {
            if (res == STATUS_INVALID_PARAMETER)
                /* FIXME: find a STATUS_ value for this one */
                SetLastError( ERROR_INVALID_THREAD_ID );
            else
                SetLastError( RtlNtStatusToDosError(res) );
        }
        else
            FreeDDElParam(info->msg, info->lparam);
    }
    SERVER_END_REQ;
    if (hunlock) GlobalUnlock(hunlock);

    return !res;
}

/***********************************************************************
 *		unpack_dde_message
 *
 * Unpack a posted DDE message received from another process.
 */
static BOOL unpack_dde_message( HWND hwnd, UINT message, WPARAM *wparam, LPARAM *lparam,
                                void **buffer, size_t size )
{
    UINT_PTR	uiLo, uiHi;
    HGLOBAL	hMem = 0;
    void*	ptr;

    switch (message)
    {
    case WM_DDE_ACK:
        if (size)
        {
            ULONGLONG hpack;
            /* hMem is being passed */
            if (size != sizeof(hpack)) return FALSE;
            if (!buffer || !*buffer) return FALSE;
            uiLo = *lparam;
            memcpy( &hpack, *buffer, size );
            hMem = unpack_ptr( hpack );
            uiHi = (UINT_PTR)hMem;
            TRACE("recv dde-ack %lx mem=%lx[%lx]\n", uiLo, uiHi, GlobalSize( hMem ));
        }
        else
        {
            uiLo = LOWORD( *lparam );
            uiHi = HIWORD( *lparam );
            TRACE("recv dde-ack %lx atom=%lx\n", uiLo, uiHi);
        }
	*lparam = PackDDElParam( WM_DDE_ACK, uiLo, uiHi );
	break;
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
	if ((!buffer || !*buffer) && message != WM_DDE_DATA) return FALSE;
	uiHi = *lparam;
        if (size)
        {
            if (!(hMem = GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, size )))
                return FALSE;
            if ((ptr = GlobalLock( hMem )))
            {
                memcpy( ptr, *buffer, size );
                GlobalUnlock( hMem );
            }
            else
            {
                GlobalFree( hMem );
                return FALSE;
            }
        }
        uiLo = (UINT_PTR)hMem;

	*lparam = PackDDElParam( message, uiLo, uiHi );
	break;
    case WM_DDE_EXECUTE:
	if (size)
	{
	    if (!buffer || !*buffer) return FALSE;
            if (!(hMem = GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, size ))) return FALSE;
            if ((ptr = GlobalLock( hMem )))
	    {
		memcpy( ptr, *buffer, size );
		GlobalUnlock( hMem );
                TRACE( "exec: pairing c=%08lx s=%p\n", *lparam, hMem );
                if (!dde_add_pair( (HGLOBAL)*lparam, hMem ))
                {
                    GlobalFree( hMem );
                    return FALSE;
                }
            }
            else
            {
                GlobalFree( hMem );
                return FALSE;
            }
	} else return FALSE;
        *lparam = (LPARAM)hMem;
        break;
    }
    return TRUE;
}

/***********************************************************************
 *           call_window_proc
 *
 * Call a window procedure and the corresponding hooks.
 */
static LRESULT call_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                 BOOL unicode, BOOL same_thread, enum wm_char_mapping mapping )
{
    LRESULT result = 0;
    CWPSTRUCT cwp;
    CWPRETSTRUCT cwpret;

    if (msg & 0x80000000)
    {
        result = handle_internal_message( hwnd, msg, wparam, lparam );
        goto done;
    }

    /* first the WH_CALLWNDPROC hook */
    hwnd = WIN_GetFullHandle( hwnd );
    cwp.lParam  = lparam;
    cwp.wParam  = wparam;
    cwp.message = msg;
    cwp.hwnd    = hwnd;
    HOOK_CallHooks( WH_CALLWNDPROC, HC_ACTION, same_thread, (LPARAM)&cwp, unicode );

    /* now call the window procedure */
    if (!WINPROC_call_window( hwnd, msg, wparam, lparam, &result, unicode, mapping )) goto done;

    /* and finally the WH_CALLWNDPROCRET hook */
    cwpret.lResult = result;
    cwpret.lParam  = lparam;
    cwpret.wParam  = wparam;
    cwpret.message = msg;
    cwpret.hwnd    = hwnd;
    HOOK_CallHooks( WH_CALLWNDPROCRET, HC_ACTION, same_thread, (LPARAM)&cwpret, unicode );
 done:
    return result;
}


/***********************************************************************
 *           send_parent_notify
 *
 * Send a WM_PARENTNOTIFY to all ancestors of the given window, unless
 * the window has the WS_EX_NOPARENTNOTIFY style.
 */
static void send_parent_notify( HWND hwnd, WORD event, WORD idChild, POINT pt )
{
    /* pt has to be in the client coordinates of the parent window */
    MapWindowPoints( 0, hwnd, &pt, 1 );
    for (;;)
    {
        HWND parent;

        if (!(GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD)) break;
        if (GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_NOPARENTNOTIFY) break;
        if (!(parent = GetParent(hwnd))) break;
        if (parent == GetDesktopWindow()) break;
        MapWindowPoints( hwnd, parent, &pt, 1 );
        hwnd = parent;
        SendMessageW( hwnd, WM_PARENTNOTIFY,
                      MAKEWPARAM( event, idChild ), MAKELPARAM( pt.x, pt.y ) );
    }
}


/***********************************************************************
 *          accept_hardware_message
 *
 * Tell the server we have passed the message to the app
 * (even though we may end up dropping it later on)
 */
static void accept_hardware_message( UINT hw_id, BOOL remove, HWND new_hwnd )
{
    SERVER_START_REQ( accept_hardware_message )
    {
        req->hw_id   = hw_id;
        req->remove  = remove;
        req->new_win = wine_server_user_handle( new_hwnd );
        if (wine_server_call( req ))
            FIXME("Failed to reply to MSG_HARDWARE message. Message may not be removed from queue.\n");
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *          process_keyboard_message
 *
 * returns TRUE if the contents of 'msg' should be passed to the application
 */
static BOOL process_keyboard_message( MSG *msg, UINT hw_id, HWND hwnd_filter,
                                      UINT first, UINT last, BOOL remove )
{
    EVENTMSG event;

    if (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN ||
        msg->message == WM_KEYUP || msg->message == WM_SYSKEYUP)
        switch (msg->wParam)
        {
            case VK_LSHIFT: case VK_RSHIFT:
                msg->wParam = VK_SHIFT;
                break;
            case VK_LCONTROL: case VK_RCONTROL:
                msg->wParam = VK_CONTROL;
                break;
            case VK_LMENU: case VK_RMENU:
                msg->wParam = VK_MENU;
                break;
        }

    /* FIXME: is this really the right place for this hook? */
    event.message = msg->message;
    event.hwnd    = msg->hwnd;
    event.time    = msg->time;
    event.paramL  = (msg->wParam & 0xFF) | (HIWORD(msg->lParam) << 8);
    event.paramH  = msg->lParam & 0x7FFF;
    if (HIWORD(msg->lParam) & 0x0100) event.paramH |= 0x8000; /* special_key - bit */
    HOOK_CallHooks( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)&event, TRUE );

    /* check message filters */
    if (msg->message < first || msg->message > last) return FALSE;
    if (!check_hwnd_filter( msg, hwnd_filter )) return FALSE;

    if (remove)
    {
        if((msg->message == WM_KEYDOWN) &&
           (msg->hwnd != GetDesktopWindow()))
        {
            /* Handle F1 key by sending out WM_HELP message */
            if (msg->wParam == VK_F1)
            {
                PostMessageW( msg->hwnd, WM_KEYF1, 0, 0 );
            }
            else if(msg->wParam >= VK_BROWSER_BACK &&
                    msg->wParam <= VK_LAUNCH_APP2)
            {
                /* FIXME: Process keystate */
                SendMessageW(msg->hwnd, WM_APPCOMMAND, (WPARAM)msg->hwnd, MAKELPARAM(0, (FAPPCOMMAND_KEY | (msg->wParam - VK_BROWSER_BACK + 1))));
            }
        }
        else if (msg->message == WM_KEYUP)
        {
            /* Handle VK_APPS key by posting a WM_CONTEXTMENU message */
            if (msg->wParam == VK_APPS && !MENU_IsMenuActive())
                PostMessageW(msg->hwnd, WM_CONTEXTMENU, (WPARAM)msg->hwnd, -1);
        }
    }

    if (HOOK_CallHooks( WH_KEYBOARD, remove ? HC_ACTION : HC_NOREMOVE,
                        LOWORD(msg->wParam), msg->lParam, TRUE ))
    {
        /* skip this message */
        HOOK_CallHooks( WH_CBT, HCBT_KEYSKIPPED, LOWORD(msg->wParam), msg->lParam, TRUE );
        accept_hardware_message( hw_id, TRUE, 0 );
        return FALSE;
    }
    accept_hardware_message( hw_id, remove, 0 );
#if 0
    if ( msg->message == WM_KEYDOWN || msg->message == WM_KEYUP )
        if ( ImmProcessKey(msg->hwnd, GetKeyboardLayout(0), msg->wParam, msg->lParam, 0) )
            msg->wParam = VK_PROCESSKEY;
#endif
    return TRUE;
}


/***********************************************************************
 *          process_mouse_message
 *
 * returns TRUE if the contents of 'msg' should be passed to the application
 */
static BOOL process_mouse_message( MSG *msg, UINT hw_id, ULONG_PTR extra_info, HWND hwnd_filter,
                                   UINT first, UINT last, BOOL remove )
{
    static MSG clk_msg;

    POINT pt;
    UINT message;
    INT hittest;
    EVENTMSG event;
    GUITHREADINFO info;
    MOUSEHOOKSTRUCT hook;
    BOOL eatMsg;

    /* find the window to dispatch this mouse message to */

    info.cbSize = sizeof(info);
    GetGUIThreadInfo( GetCurrentThreadId(), &info );
    if (info.hwndCapture)
    {
        hittest = HTCLIENT;
        msg->hwnd = info.hwndCapture;
    }
    else
    {
        msg->hwnd = WINPOS_WindowFromPoint( msg->hwnd, msg->pt, &hittest );
    }

    if (!msg->hwnd || !WIN_IsCurrentThread( msg->hwnd ))
    {
        accept_hardware_message( hw_id, TRUE, msg->hwnd );
        return FALSE;
    }

    /* FIXME: is this really the right place for this hook? */
    event.message = msg->message;
    event.time    = msg->time;
    event.hwnd    = msg->hwnd;
    event.paramL  = msg->pt.x;
    event.paramH  = msg->pt.y;
    HOOK_CallHooks( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)&event, TRUE );

    if (!check_hwnd_filter( msg, hwnd_filter )) return FALSE;

    pt = msg->pt;
    message = msg->message;
    /* Note: windows has no concept of a non-client wheel message */
    if (message != WM_MOUSEWHEEL)
    {
        if (hittest != HTCLIENT)
        {
            message += WM_NCMOUSEMOVE - WM_MOUSEMOVE;
            msg->wParam = hittest;
        }
        else
        {
            /* coordinates don't get translated while tracking a menu */
            /* FIXME: should differentiate popups and top-level menus */
            if (!(info.flags & GUI_INMENUMODE))
                ScreenToClient( msg->hwnd, &pt );
        }
    }
    msg->lParam = MAKELONG( pt.x, pt.y );

    /* translate double clicks */

    if ((msg->message == WM_LBUTTONDOWN) ||
        (msg->message == WM_RBUTTONDOWN) ||
        (msg->message == WM_MBUTTONDOWN) ||
        (msg->message == WM_XBUTTONDOWN))
    {
        BOOL update = remove;

        /* translate double clicks -
	 * note that ...MOUSEMOVEs can slip in between
	 * ...BUTTONDOWN and ...BUTTONDBLCLK messages */

        if ((info.flags & (GUI_INMENUMODE|GUI_INMOVESIZE)) ||
            hittest != HTCLIENT ||
            (GetClassLongA( msg->hwnd, GCL_STYLE ) & CS_DBLCLKS))
        {
           if ((msg->message == clk_msg.message) &&
               (msg->hwnd == clk_msg.hwnd) &&
               (msg->wParam == clk_msg.wParam) &&
               (msg->time - clk_msg.time < GetDoubleClickTime()) &&
               (abs(msg->pt.x - clk_msg.pt.x) < GetSystemMetrics(SM_CXDOUBLECLK)/2) &&
               (abs(msg->pt.y - clk_msg.pt.y) < GetSystemMetrics(SM_CYDOUBLECLK)/2))
           {
               message += (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
               if (update)
               {
                   clk_msg.message = 0;  /* clear the double click conditions */
                   update = FALSE;
               }
           }
        }
        if (message < first || message > last) return FALSE;
        /* update static double click conditions */
        if (update) clk_msg = *msg;
    }
    else
    {
        if (message < first || message > last) return FALSE;
    }

    /* message is accepted now (but may still get dropped) */

    hook.pt           = msg->pt;
    hook.hwnd         = msg->hwnd;
    hook.wHitTestCode = hittest;
    hook.dwExtraInfo  = extra_info;
    if (HOOK_CallHooks( WH_MOUSE, remove ? HC_ACTION : HC_NOREMOVE,
                        message, (LPARAM)&hook, TRUE ))
    {
        hook.pt           = msg->pt;
        hook.hwnd         = msg->hwnd;
        hook.wHitTestCode = hittest;
        hook.dwExtraInfo  = extra_info;
        HOOK_CallHooks( WH_CBT, HCBT_CLICKSKIPPED, message, (LPARAM)&hook, TRUE );
        accept_hardware_message( hw_id, TRUE, 0 );
        return FALSE;
    }

    if ((hittest == HTERROR) || (hittest == HTNOWHERE))
    {
        SendMessageW( msg->hwnd, WM_SETCURSOR, (WPARAM)msg->hwnd,
                      MAKELONG( hittest, msg->message ));
        accept_hardware_message( hw_id, TRUE, 0 );
        return FALSE;
    }

    accept_hardware_message( hw_id, remove, 0 );

    if (!remove || info.hwndCapture)
    {
        msg->message = message;
        return TRUE;
    }

    eatMsg = FALSE;

    if ((msg->message == WM_LBUTTONDOWN) ||
        (msg->message == WM_RBUTTONDOWN) ||
        (msg->message == WM_MBUTTONDOWN) ||
        (msg->message == WM_XBUTTONDOWN))
    {
        /* Send the WM_PARENTNOTIFY,
         * note that even for double/nonclient clicks
         * notification message is still WM_L/M/RBUTTONDOWN.
         */
        send_parent_notify( msg->hwnd, msg->message, 0, msg->pt );

        /* Activate the window if needed */

        if (msg->hwnd != info.hwndActive)
        {
            HWND hwndTop = msg->hwnd;
            while (hwndTop)
            {
                if ((GetWindowLongW( hwndTop, GWL_STYLE ) & (WS_POPUP|WS_CHILD)) != WS_CHILD) break;
                hwndTop = GetParent( hwndTop );
            }

            if (hwndTop && hwndTop != GetDesktopWindow())
            {
                LONG ret = SendMessageW( msg->hwnd, WM_MOUSEACTIVATE, (WPARAM)hwndTop,
                                         MAKELONG( hittest, msg->message ) );
                switch(ret)
                {
                case MA_NOACTIVATEANDEAT:
                    eatMsg = TRUE;
                    /* fall through */
                case MA_NOACTIVATE:
                    break;
                case MA_ACTIVATEANDEAT:
                    eatMsg = TRUE;
                    /* fall through */
                case MA_ACTIVATE:
                case 0:
                    if (!FOCUS_MouseActivate( hwndTop )) eatMsg = TRUE;
                    break;
                default:
                    WARN( "unknown WM_MOUSEACTIVATE code %d\n", ret );
                    break;
                }
            }
        }
    }

    /* send the WM_SETCURSOR message */

    /* Windows sends the normal mouse message as the message parameter
       in the WM_SETCURSOR message even if it's non-client mouse message */
    SendMessageW( msg->hwnd, WM_SETCURSOR, (WPARAM)msg->hwnd, MAKELONG( hittest, msg->message ));

    msg->message = message;
    return !eatMsg;
}


/***********************************************************************
 *           process_hardware_message
 *
 * Process a hardware message; return TRUE if message should be passed on to the app
 */
static BOOL process_hardware_message( MSG *msg, UINT hw_id, ULONG_PTR extra_info, HWND hwnd_filter,
                                      UINT first, UINT last, BOOL remove )
{
    if (is_keyboard_message( msg->message ))
        return process_keyboard_message( msg, hw_id, hwnd_filter, first, last, remove );

    if (is_mouse_message( msg->message ))
        return process_mouse_message( msg, hw_id, extra_info, hwnd_filter, first, last, remove );

    ERR( "unknown message type %x\n", msg->message );
    return FALSE;
}


/***********************************************************************
 *           call_sendmsg_callback
 *
 * Call the callback function of SendMessageCallback.
 */
static inline void call_sendmsg_callback( SENDASYNCPROC callback, HWND hwnd, UINT msg,
                                          ULONG_PTR data, LRESULT result )
{
    if (!callback) return;

    if (TRACE_ON(relay))
        DPRINTF( "%04x:Call message callback %p (hwnd=%p,msg=%s,data=%08lx,result=%08lx)\n",
                 GetCurrentThreadId(), callback, hwnd, SPY_GetMsgName( msg, hwnd ),
                 data, result );
    callback( hwnd, msg, data, result );
    if (TRACE_ON(relay))
        DPRINTF( "%04x:Ret  message callback %p (hwnd=%p,msg=%s,data=%08lx,result=%08lx)\n",
                 GetCurrentThreadId(), callback, hwnd, SPY_GetMsgName( msg, hwnd ),
                 data, result );
}


/***********************************************************************
 *           peek_message
 *
 * Peek for a message matching the given parameters. Return FALSE if none available.
 * All pending sent messages are processed before returning.
 */
static BOOL peek_message( MSG *msg, HWND hwnd, UINT first, UINT last, UINT flags, UINT changed_mask )
{
    LRESULT result;
    struct user_thread_info *thread_info = get_user_thread_info();
    struct received_message_info info, *old_info;
    unsigned int hw_id = 0;  /* id of previous hardware message */
    void *buffer;
    size_t buffer_size = 256;

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, buffer_size ))) return FALSE;

    if (!first && !last) last = ~0;
    if (hwnd == HWND_BROADCAST) hwnd = HWND_TOPMOST;

    for (;;)
    {
        NTSTATUS res;
        size_t size = 0;
        const message_data_t *msg_data = buffer;

        SERVER_START_REQ( get_message )
        {
            req->flags     = flags;
            req->get_win   = wine_server_user_handle( hwnd );
            req->get_first = first;
            req->get_last  = last;
            req->hw_id     = hw_id;
            req->wake_mask = changed_mask & (QS_SENDMESSAGE | QS_SMRESULT);
            req->changed_mask = changed_mask;
            wine_server_set_reply( req, buffer, buffer_size );
            if (!(res = wine_server_call( req )))
            {
                size = wine_server_reply_size( reply );
                info.type        = reply->type;
                info.msg.hwnd    = wine_server_ptr_handle( reply->win );
                info.msg.message = reply->msg;
                info.msg.wParam  = reply->wparam;
                info.msg.lParam  = reply->lparam;
                info.msg.time    = reply->time;
                info.msg.pt.x    = 0;
                info.msg.pt.y    = 0;
                hw_id            = 0;
                thread_info->active_hooks = reply->active_hooks;
            }
            else buffer_size = reply->total;
        }
        SERVER_END_REQ;

        if (res)
        {
            HeapFree( GetProcessHeap(), 0, buffer );
            if (res != STATUS_BUFFER_OVERFLOW) return FALSE;
            if (!(buffer = HeapAlloc( GetProcessHeap(), 0, buffer_size ))) return FALSE;
            continue;
        }

        TRACE( "got type %d msg %x (%s) hwnd %p wp %lx lp %lx\n",
               info.type, info.msg.message,
               (info.type == MSG_WINEVENT) ? "MSG_WINEVENT" : SPY_GetMsgName(info.msg.message, info.msg.hwnd),
               info.msg.hwnd, info.msg.wParam, info.msg.lParam );

        switch(info.type)
        {
        case MSG_ASCII:
        case MSG_UNICODE:
            info.flags = ISMEX_SEND;
            break;
        case MSG_NOTIFY:
            info.flags = ISMEX_NOTIFY;
            break;
        case MSG_CALLBACK:
            info.flags = ISMEX_CALLBACK;
            break;
        case MSG_CALLBACK_RESULT:
            if (size >= sizeof(msg_data->callback))
                call_sendmsg_callback( wine_server_get_ptr(msg_data->callback.callback),
                                       info.msg.hwnd, info.msg.message,
                                       msg_data->callback.data, msg_data->callback.result );
            continue;
        case MSG_WINEVENT:
            if (size >= sizeof(msg_data->winevent))
            {
                WINEVENTPROC hook_proc;

                hook_proc = wine_server_get_ptr( msg_data->winevent.hook_proc );
                size -= sizeof(msg_data->winevent);
                if (size)
                {
                    WCHAR module[MAX_PATH];

                    size = min( size, (MAX_PATH - 1) * sizeof(WCHAR) );
                    memcpy( module, &msg_data->winevent + 1, size );
                    module[size / sizeof(WCHAR)] = 0;
                    if (!(hook_proc = get_hook_proc( hook_proc, module )))
                    {
                        ERR( "invalid winevent hook module name %s\n", debugstr_w(module) );
                        continue;
                    }
                }

                if (TRACE_ON(relay))
                    DPRINTF( "%04x:Call winevent proc %p (hook=%04x,event=%x,hwnd=%p,object_id=%lx,child_id=%lx,tid=%04x,time=%x)\n",
                             GetCurrentThreadId(), hook_proc,
                             msg_data->winevent.hook, info.msg.message, info.msg.hwnd, info.msg.wParam,
                             info.msg.lParam, msg_data->winevent.tid, info.msg.time);

                hook_proc( wine_server_ptr_handle( msg_data->winevent.hook ), info.msg.message,
                           info.msg.hwnd, info.msg.wParam, info.msg.lParam,
                           msg_data->winevent.tid, info.msg.time );

                if (TRACE_ON(relay))
                    DPRINTF( "%04x:Ret  winevent proc %p (hook=%04x,event=%x,hwnd=%p,object_id=%lx,child_id=%lx,tid=%04x,time=%x)\n",
                             GetCurrentThreadId(), hook_proc,
                             msg_data->winevent.hook, info.msg.message, info.msg.hwnd, info.msg.wParam,
                             info.msg.lParam, msg_data->winevent.tid, info.msg.time);
            }
            continue;
        case MSG_OTHER_PROCESS:
            info.flags = ISMEX_SEND;
            if (!unpack_message( info.msg.hwnd, info.msg.message, &info.msg.wParam,
                                 &info.msg.lParam, &buffer, size ))
            {
                /* ignore it */
                reply_message( &info, 0, TRUE );
                continue;
            }
            break;
        case MSG_HARDWARE:
            if (size >= sizeof(msg_data->hardware))
            {
                info.msg.pt.x = msg_data->hardware.x;
                info.msg.pt.y = msg_data->hardware.y;
                hw_id         = msg_data->hardware.hw_id;
                if (!process_hardware_message( &info.msg, hw_id, msg_data->hardware.info,
                                               hwnd, first, last, flags & PM_REMOVE ))
                {
                    TRACE("dropping msg %x\n", info.msg.message );
                    continue;  /* ignore it */
                }
                *msg = info.msg;
                thread_info->GetMessagePosVal = MAKELONG( info.msg.pt.x, info.msg.pt.y );
                thread_info->GetMessageTimeVal = info.msg.time;
                thread_info->GetMessageExtraInfoVal = msg_data->hardware.info;
                HeapFree( GetProcessHeap(), 0, buffer );
                HOOK_CallHooks( WH_GETMESSAGE, HC_ACTION, flags & PM_REMOVE, (LPARAM)msg, TRUE );
                return TRUE;
            }
            continue;
        case MSG_POSTED:
            if (info.msg.message & 0x80000000)  /* internal message */
            {
                if (flags & PM_REMOVE)
                {
                    handle_internal_message( info.msg.hwnd, info.msg.message,
                                             info.msg.wParam, info.msg.lParam );
                    /* if this is a nested call return right away */
                    if (first == info.msg.message && last == info.msg.message) return FALSE;
                }
                else
                    peek_message( msg, info.msg.hwnd, info.msg.message,
                                  info.msg.message, flags | PM_REMOVE, changed_mask );
                continue;
            }
	    if (info.msg.message >= WM_DDE_FIRST && info.msg.message <= WM_DDE_LAST)
	    {
		if (!unpack_dde_message( info.msg.hwnd, info.msg.message, &info.msg.wParam,
                                         &info.msg.lParam, &buffer, size ))
                    continue;  /* ignore it */
	    }
            *msg = info.msg;
            msg->pt.x = (short)LOWORD( thread_info->GetMessagePosVal );
            msg->pt.y = (short)HIWORD( thread_info->GetMessagePosVal );
            thread_info->GetMessageTimeVal = info.msg.time;
            thread_info->GetMessageExtraInfoVal = 0;
            HeapFree( GetProcessHeap(), 0, buffer );
            HOOK_CallHooks( WH_GETMESSAGE, HC_ACTION, flags & PM_REMOVE, (LPARAM)msg, TRUE );
            return TRUE;
        }

        /* if we get here, we have a sent message; call the window procedure */
        old_info = thread_info->receive_info;
        thread_info->receive_info = &info;
        result = call_window_proc( info.msg.hwnd, info.msg.message, info.msg.wParam,
                                   info.msg.lParam, (info.type != MSG_ASCII), FALSE,
                                   WMCHAR_MAP_RECVMESSAGE );
        reply_message( &info, result, TRUE );
        thread_info->receive_info = old_info;

        /* if some PM_QS* flags were specified, only handle sent messages from now on */
        if (HIWORD(flags) && !changed_mask) flags = PM_QS_SENDMESSAGE | LOWORD(flags);
    }
}


/***********************************************************************
 *           process_sent_messages
 *
 * Process all pending sent messages.
 */
static inline void process_sent_messages(void)
{
    MSG msg;
    peek_message( &msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE, 0 );
}


/***********************************************************************
 *           get_server_queue_handle
 *
 * Get a handle to the server message queue for the current thread.
 */
static HANDLE get_server_queue_handle(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();
    HANDLE ret;

    if (!(ret = thread_info->server_queue))
    {
        SERVER_START_REQ( get_msg_queue )
        {
            wine_server_call( req );
            ret = wine_server_ptr_handle( reply->handle );
        }
        SERVER_END_REQ;
        thread_info->server_queue = ret;
        if (!ret) ERR( "Cannot get server thread queue\n" );
    }
    return ret;
}


/***********************************************************************
 *           wait_message_reply
 *
 * Wait until a sent message gets replied to.
 */
static void wait_message_reply( UINT flags )
{
    HANDLE server_queue = get_server_queue_handle();

    for (;;)
    {
        unsigned int wake_bits = 0;

        SERVER_START_REQ( set_queue_mask )
        {
            req->wake_mask    = QS_SMRESULT | ((flags & SMTO_BLOCK) ? 0 : QS_SENDMESSAGE);
            req->changed_mask = req->wake_mask;
            req->skip_wait    = 1;
            if (!wine_server_call( req ))
                wake_bits = reply->wake_bits;
        }
        SERVER_END_REQ;

        if (wake_bits & QS_SMRESULT) return;  /* got a result */
        if (wake_bits & QS_SENDMESSAGE)
        {
            /* Process the sent message immediately */
            process_sent_messages();
            continue;
        }

        wow_handlers.wait_message( 1, &server_queue, INFINITE, QS_SENDMESSAGE, 0 );
    }
}

/***********************************************************************
 *		put_message_in_queue
 *
 * Put a sent message into the destination queue.
 * For inter-process message, reply_size is set to expected size of reply data.
 */
static BOOL put_message_in_queue( const struct send_message_info *info, size_t *reply_size )
{
    struct packed_message data;
    message_data_t msg_data;
    unsigned int res;
    int i;
    timeout_t timeout = TIMEOUT_INFINITE;

    /* Check for INFINITE timeout for compatibility with Win9x,
     * although Windows >= NT does not do so
     */
    if (info->type != MSG_NOTIFY &&
        info->type != MSG_CALLBACK &&
        info->type != MSG_POSTED &&
        info->timeout &&
        info->timeout != INFINITE)
    {
        /* timeout is signed despite the prototype */
        timeout = (timeout_t)max( 0, (int)info->timeout ) * -10000;
    }

    memset( &data, 0, sizeof(data) );
    if (info->type == MSG_OTHER_PROCESS)
    {
        *reply_size = pack_message( info->hwnd, info->msg, info->wparam, info->lparam, &data );
        if (data.count == -1)
        {
            WARN( "cannot pack message %x\n", info->msg );
            return FALSE;
        }
    }
    else if (info->type == MSG_CALLBACK)
    {
        msg_data.callback.callback = wine_server_client_ptr( info->callback );
        msg_data.callback.data     = info->data;
        msg_data.callback.result   = 0;
        data.data[0] = &msg_data;
        data.size[0] = sizeof(msg_data.callback);
        data.count = 1;
    }
    else if (info->type == MSG_POSTED && info->msg >= WM_DDE_FIRST && info->msg <= WM_DDE_LAST)
    {
        return post_dde_message( &data, info );
    }

    SERVER_START_REQ( send_message )
    {
        req->id      = info->dest_tid;
        req->type    = info->type;
        req->flags   = 0;
        req->win     = wine_server_user_handle( info->hwnd );
        req->msg     = info->msg;
        req->wparam  = info->wparam;
        req->lparam  = info->lparam;
        req->timeout = timeout;

        if (info->flags & SMTO_ABORTIFHUNG) req->flags |= SEND_MSG_ABORT_IF_HUNG;
        for (i = 0; i < data.count; i++) wine_server_add_data( req, data.data[i], data.size[i] );
        if ((res = wine_server_call( req )))
        {
            if (res == STATUS_INVALID_PARAMETER)
                /* FIXME: find a STATUS_ value for this one */
                SetLastError( ERROR_INVALID_THREAD_ID );
            else
                SetLastError( RtlNtStatusToDosError(res) );
        }
    }
    SERVER_END_REQ;
    return !res;
}


/***********************************************************************
 *		retrieve_reply
 *
 * Retrieve a message reply from the server.
 */
static LRESULT retrieve_reply( const struct send_message_info *info,
                               size_t reply_size, LRESULT *result )
{
    NTSTATUS status;
    void *reply_data = NULL;

    if (reply_size)
    {
        if (!(reply_data = HeapAlloc( GetProcessHeap(), 0, reply_size )))
        {
            WARN( "no memory for reply, will be truncated\n" );
            reply_size = 0;
        }
    }
    SERVER_START_REQ( get_message_reply )
    {
        req->cancel = 1;
        if (reply_size) wine_server_set_reply( req, reply_data, reply_size );
        if (!(status = wine_server_call( req ))) *result = reply->result;
        reply_size = wine_server_reply_size( reply );
    }
    SERVER_END_REQ;
    if (!status && reply_size)
        unpack_reply( info->hwnd, info->msg, info->wparam, info->lparam, reply_data, reply_size );

    HeapFree( GetProcessHeap(), 0, reply_data );

    TRACE( "hwnd %p msg %x (%s) wp %lx lp %lx got reply %lx (err=%d)\n",
           info->hwnd, info->msg, SPY_GetMsgName(info->msg, info->hwnd), info->wparam,
           info->lparam, *result, status );

    /* MSDN states that last error is 0 on timeout, but at least NT4 returns ERROR_TIMEOUT */
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 *		send_inter_thread_message
 */
static LRESULT send_inter_thread_message( const struct send_message_info *info, LRESULT *res_ptr )
{
    size_t reply_size = 0;

    TRACE( "hwnd %p msg %x (%s) wp %lx lp %lx\n",
           info->hwnd, info->msg, SPY_GetMsgName(info->msg, info->hwnd), info->wparam, info->lparam );

    USER_CheckNotLock();

    if (!put_message_in_queue( info, &reply_size )) return 0;

    /* there's no reply to wait for on notify/callback messages */
    if (info->type == MSG_NOTIFY || info->type == MSG_CALLBACK) return 1;

    wait_message_reply( info->flags );
    return retrieve_reply( info, reply_size, res_ptr );
}


/***********************************************************************
 *		send_inter_thread_callback
 */
static LRESULT send_inter_thread_callback( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                           LRESULT *result, void *arg )
{
    struct send_message_info *info = arg;
    info->hwnd   = hwnd;
    info->msg    = msg;
    info->wparam = wp;
    info->lparam = lp;
    return send_inter_thread_message( info, result );
}


/***********************************************************************
 *		send_message
 *
 * Backend implementation of the various SendMessage functions.
 */
static BOOL send_message( struct send_message_info *info, DWORD_PTR *res_ptr, BOOL unicode )
{
    DWORD dest_pid;
    BOOL ret;
    LRESULT result;

    if (is_broadcast(info->hwnd))
    {
        EnumWindows( broadcast_message_callback, (LPARAM)info );
        if (res_ptr) *res_ptr = 1;
        return TRUE;
    }

    if (!(info->dest_tid = GetWindowThreadProcessId( info->hwnd, &dest_pid ))) return FALSE;

    if (USER_IsExitingThread( info->dest_tid )) return FALSE;

    SPY_EnterMessage( SPY_SENDMESSAGE, info->hwnd, info->msg, info->wparam, info->lparam );

    if (info->dest_tid == GetCurrentThreadId())
    {
        result = call_window_proc( info->hwnd, info->msg, info->wparam, info->lparam,
                                   unicode, TRUE, info->wm_char );
        if (info->type == MSG_CALLBACK)
            call_sendmsg_callback( info->callback, info->hwnd, info->msg, info->data, result );
        ret = TRUE;
    }
    else
    {
        if (dest_pid != GetCurrentProcessId() && (info->type == MSG_ASCII || info->type == MSG_UNICODE))
            info->type = MSG_OTHER_PROCESS;

        /* MSG_ASCII can be sent unconverted except for WM_CHAR; everything else needs to be Unicode */
        if (!unicode && is_unicode_message( info->msg ) &&
            (info->type != MSG_ASCII || info->msg == WM_CHAR))
            ret = WINPROC_CallProcAtoW( send_inter_thread_callback, info->hwnd, info->msg,
                                        info->wparam, info->lparam, &result, info, info->wm_char );
        else
            ret = send_inter_thread_message( info, &result );
    }

    SPY_ExitMessage( SPY_RESULT_OK, info->hwnd, info->msg, result, info->wparam, info->lparam );
    if (ret && res_ptr) *res_ptr = result;
    return ret;
}


/***********************************************************************
 *		MSG_SendInternalMessageTimeout
 *
 * Same as SendMessageTimeoutW but sends the message to a specific thread
 * without requiring a window handle. Only works for internal Wine messages.
 */
LRESULT MSG_SendInternalMessageTimeout( DWORD dest_pid, DWORD dest_tid,
                                        UINT msg, WPARAM wparam, LPARAM lparam,
                                        UINT flags, UINT timeout, PDWORD_PTR res_ptr )
{
    struct send_message_info info;
    LRESULT ret, result;

    assert( msg & 0x80000000 );  /* must be an internal Wine message */

    info.type     = MSG_UNICODE;
    info.dest_tid = dest_tid;
    info.hwnd     = 0;
    info.msg      = msg;
    info.wparam   = wparam;
    info.lparam   = lparam;
    info.flags    = flags;
    info.timeout  = timeout;

    if (USER_IsExitingThread( dest_tid )) return 0;

    if (dest_tid == GetCurrentThreadId())
    {
        result = handle_internal_message( 0, msg, wparam, lparam );
        ret = 1;
    }
    else
    {
        if (dest_pid != GetCurrentProcessId()) info.type = MSG_OTHER_PROCESS;
        ret = send_inter_thread_message( &info, &result );
    }
    if (ret && res_ptr) *res_ptr = result;
    return ret;
}


/***********************************************************************
 *		SendMessageTimeoutW  (USER32.@)
 */
LRESULT WINAPI SendMessageTimeoutW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                    UINT flags, UINT timeout, PDWORD_PTR res_ptr )
{
    struct send_message_info info;

    info.type    = MSG_UNICODE;
    info.hwnd    = hwnd;
    info.msg     = msg;
    info.wparam  = wparam;
    info.lparam  = lparam;
    info.flags   = flags;
    info.timeout = timeout;

    return send_message( &info, res_ptr, TRUE );
}

/***********************************************************************
 *		SendMessageTimeoutA  (USER32.@)
 */
LRESULT WINAPI SendMessageTimeoutA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                    UINT flags, UINT timeout, PDWORD_PTR res_ptr )
{
    struct send_message_info info;

    info.type    = MSG_ASCII;
    info.hwnd    = hwnd;
    info.msg     = msg;
    info.wparam  = wparam;
    info.lparam  = lparam;
    info.flags   = flags;
    info.timeout = timeout;
    info.wm_char  = WMCHAR_MAP_SENDMESSAGETIMEOUT;

    return send_message( &info, res_ptr, FALSE );
}


/***********************************************************************
 *		SendMessageW  (USER32.@)
 */
LRESULT WINAPI SendMessageW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    DWORD_PTR res = 0;
    struct send_message_info info;

    info.type    = MSG_UNICODE;
    info.hwnd    = hwnd;
    info.msg     = msg;
    info.wparam  = wparam;
    info.lparam  = lparam;
    info.flags   = SMTO_NORMAL;
    info.timeout = 0;

    send_message( &info, &res, TRUE );
    return res;
}


/***********************************************************************
 *		SendMessageA  (USER32.@)
 */
LRESULT WINAPI SendMessageA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    DWORD_PTR res = 0;
    struct send_message_info info;

    info.type    = MSG_ASCII;
    info.hwnd    = hwnd;
    info.msg     = msg;
    info.wparam  = wparam;
    info.lparam  = lparam;
    info.flags   = SMTO_NORMAL;
    info.timeout = 0;
    info.wm_char  = WMCHAR_MAP_SENDMESSAGE;

    send_message( &info, &res, FALSE );
    return res;
}


/***********************************************************************
 *		SendNotifyMessageA  (USER32.@)
 */
BOOL WINAPI SendNotifyMessageA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct send_message_info info;

    if (is_pointer_message(msg))
    {
        SetLastError( ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }

    info.type    = MSG_NOTIFY;
    info.hwnd    = hwnd;
    info.msg     = msg;
    info.wparam  = wparam;
    info.lparam  = lparam;
    info.flags   = 0;
    info.wm_char = WMCHAR_MAP_SENDMESSAGETIMEOUT;

    return send_message( &info, NULL, FALSE );
}


/***********************************************************************
 *		SendNotifyMessageW  (USER32.@)
 */
BOOL WINAPI SendNotifyMessageW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct send_message_info info;

    if (is_pointer_message(msg))
    {
        SetLastError( ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }

    info.type    = MSG_NOTIFY;
    info.hwnd    = hwnd;
    info.msg     = msg;
    info.wparam  = wparam;
    info.lparam  = lparam;
    info.flags   = 0;

    return send_message( &info, NULL, TRUE );
}


/***********************************************************************
 *		SendMessageCallbackA  (USER32.@)
 */
BOOL WINAPI SendMessageCallbackA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                  SENDASYNCPROC callback, ULONG_PTR data )
{
    struct send_message_info info;

    if (is_pointer_message(msg))
    {
        SetLastError( ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }

    info.type     = MSG_CALLBACK;
    info.hwnd     = hwnd;
    info.msg      = msg;
    info.wparam   = wparam;
    info.lparam   = lparam;
    info.callback = callback;
    info.data     = data;
    info.flags    = 0;
    info.wm_char  = WMCHAR_MAP_SENDMESSAGETIMEOUT;

    return send_message( &info, NULL, FALSE );
}


/***********************************************************************
 *		SendMessageCallbackW  (USER32.@)
 */
BOOL WINAPI SendMessageCallbackW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                  SENDASYNCPROC callback, ULONG_PTR data )
{
    struct send_message_info info;

    if (is_pointer_message(msg))
    {
        SetLastError( ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }

    info.type     = MSG_CALLBACK;
    info.hwnd     = hwnd;
    info.msg      = msg;
    info.wparam   = wparam;
    info.lparam   = lparam;
    info.callback = callback;
    info.data     = data;
    info.flags    = 0;

    return send_message( &info, NULL, TRUE );
}


/***********************************************************************
 *		ReplyMessage  (USER32.@)
 */
BOOL WINAPI ReplyMessage( LRESULT result )
{
    struct received_message_info *info = get_user_thread_info()->receive_info;

    if (!info) return FALSE;
    reply_message( info, result, FALSE );
    return TRUE;
}


/***********************************************************************
 *		InSendMessage  (USER32.@)
 */
BOOL WINAPI InSendMessage(void)
{
    return (InSendMessageEx(NULL) & (ISMEX_SEND|ISMEX_REPLIED)) == ISMEX_SEND;
}


/***********************************************************************
 *		InSendMessageEx  (USER32.@)
 */
DWORD WINAPI InSendMessageEx( LPVOID reserved )
{
    struct received_message_info *info = get_user_thread_info()->receive_info;

    if (info) return info->flags;
    return ISMEX_NOSEND;
}


/***********************************************************************
 *		PostMessageA  (USER32.@)
 */
BOOL WINAPI PostMessageA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (!map_wparam_AtoW( msg, &wparam, WMCHAR_MAP_POSTMESSAGE )) return TRUE;
    return PostMessageW( hwnd, msg, wparam, lparam );
}


/***********************************************************************
 *		PostMessageW  (USER32.@)
 */
BOOL WINAPI PostMessageW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct send_message_info info;

    if (is_pointer_message( msg ))
    {
        SetLastError( ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }

    TRACE( "hwnd %p msg %x (%s) wp %lx lp %lx\n",
           hwnd, msg, SPY_GetMsgName(msg, hwnd), wparam, lparam );

    info.type   = MSG_POSTED;
    info.hwnd   = hwnd;
    info.msg    = msg;
    info.wparam = wparam;
    info.lparam = lparam;
    info.flags  = 0;

    if (is_broadcast(hwnd))
    {
        EnumWindows( broadcast_message_callback, (LPARAM)&info );
        return TRUE;
    }

    if (!hwnd) return PostThreadMessageW( GetCurrentThreadId(), msg, wparam, lparam );

    if (!(info.dest_tid = GetWindowThreadProcessId( hwnd, NULL ))) return FALSE;

    if (USER_IsExitingThread( info.dest_tid )) return TRUE;

    return put_message_in_queue( &info, NULL );
}


/**********************************************************************
 *		PostThreadMessageA  (USER32.@)
 */
BOOL WINAPI PostThreadMessageA( DWORD thread, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (!map_wparam_AtoW( msg, &wparam, WMCHAR_MAP_POSTMESSAGE )) return TRUE;
    return PostThreadMessageW( thread, msg, wparam, lparam );
}


/**********************************************************************
 *		PostThreadMessageW  (USER32.@)
 */
BOOL WINAPI PostThreadMessageW( DWORD thread, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct send_message_info info;

    if (is_pointer_message( msg ))
    {
        SetLastError( ERROR_MESSAGE_SYNC_ONLY );
        return FALSE;
    }
    if (USER_IsExitingThread( thread )) return TRUE;

    info.type     = MSG_POSTED;
    info.dest_tid = thread;
    info.hwnd     = 0;
    info.msg      = msg;
    info.wparam   = wparam;
    info.lparam   = lparam;
    info.flags    = 0;
    return put_message_in_queue( &info, NULL );
}


/***********************************************************************
 *		PostQuitMessage  (USER32.@)
 *
 * Posts a quit message to the current thread's message queue.
 *
 * PARAMS
 *  exit_code [I] Exit code to return from message loop.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  This function is not the same as calling:
 *|PostThreadMessage(GetCurrentThreadId(), WM_QUIT, exit_code, 0);
 *  It instead sets a flag in the message queue that signals it to generate
 *  a WM_QUIT message when there are no other pending sent or posted messages
 *  in the queue.
 */
void WINAPI PostQuitMessage( INT exit_code )
{
    SERVER_START_REQ( post_quit_message )
    {
        req->exit_code = exit_code;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *		PeekMessageW  (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PeekMessageW( MSG *msg_out, HWND hwnd, UINT first, UINT last, UINT flags )
{
    MSG msg;

    USER_CheckNotLock();

    /* check for graphics events */
    USER_Driver->pMsgWaitForMultipleObjectsEx( 0, NULL, 0, QS_ALLINPUT, 0 );

    if (!peek_message( &msg, hwnd, first, last, flags, 0 ))
    {
        if (!(flags & PM_NOYIELD)) wow_handlers.wait_message( 0, NULL, 0, 0, 0 );
        return FALSE;
    }

    /* copy back our internal safe copy of message data to msg_out.
     * msg_out is a variable from the *program*, so it can't be used
     * internally as it can get "corrupted" by our use of SendMessage()
     * (back to the program) inside the message handling itself. */
    if (!msg_out)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }
    *msg_out = msg;
    return TRUE;
}


/***********************************************************************
 *		PeekMessageA  (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PeekMessageA( MSG *msg, HWND hwnd, UINT first, UINT last, UINT flags )
{
    if (get_pending_wmchar( msg, first, last, (flags & PM_REMOVE) )) return TRUE;
    if (!PeekMessageW( msg, hwnd, first, last, flags )) return FALSE;
    map_wparam_WtoA( msg, (flags & PM_REMOVE) );
    return TRUE;
}


/***********************************************************************
 *		GetMessageW  (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetMessageW( MSG *msg, HWND hwnd, UINT first, UINT last )
{
    HANDLE server_queue = get_server_queue_handle();
    unsigned int mask = QS_POSTMESSAGE | QS_SENDMESSAGE;  /* Always selected */

    USER_CheckNotLock();

    /* check for graphics events */
    USER_Driver->pMsgWaitForMultipleObjectsEx( 0, NULL, 0, QS_ALLINPUT, 0 );

    if (first || last)
    {
        if ((first <= WM_KEYLAST) && (last >= WM_KEYFIRST)) mask |= QS_KEY;
        if ( ((first <= WM_MOUSELAST) && (last >= WM_MOUSEFIRST)) ||
             ((first <= WM_NCMOUSELAST) && (last >= WM_NCMOUSEFIRST)) ) mask |= QS_MOUSE;
        if ((first <= WM_TIMER) && (last >= WM_TIMER)) mask |= QS_TIMER;
        if ((first <= WM_SYSTIMER) && (last >= WM_SYSTIMER)) mask |= QS_TIMER;
        if ((first <= WM_PAINT) && (last >= WM_PAINT)) mask |= QS_PAINT;
    }
    else mask = QS_ALLINPUT;

    while (!peek_message( msg, hwnd, first, last, PM_REMOVE | (mask << 16), mask ))
    {
        wow_handlers.wait_message( 1, &server_queue, INFINITE, mask, 0 );
    }

    return (msg->message != WM_QUIT);
}


/***********************************************************************
 *		GetMessageA  (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetMessageA( MSG *msg, HWND hwnd, UINT first, UINT last )
{
    if (get_pending_wmchar( msg, first, last, TRUE )) return TRUE;
    GetMessageW( msg, hwnd, first, last );
    map_wparam_WtoA( msg, TRUE );
    return (msg->message != WM_QUIT);
}


/***********************************************************************
 *		IsDialogMessageA (USER32.@)
 *		IsDialogMessage  (USER32.@)
 */
BOOL WINAPI IsDialogMessageA( HWND hwndDlg, LPMSG pmsg )
{
    MSG msg = *pmsg;
    map_wparam_AtoW( msg.message, &msg.wParam, WMCHAR_MAP_NOMAPPING );
    return IsDialogMessageW( hwndDlg, &msg );
}


/***********************************************************************
 *		TranslateMessage (USER32.@)
 *
 * Implementation of TranslateMessage.
 *
 * TranslateMessage translates virtual-key messages into character-messages,
 * as follows :
 * WM_KEYDOWN/WM_KEYUP combinations produce a WM_CHAR or WM_DEADCHAR message.
 * ditto replacing WM_* with WM_SYS*
 * This produces WM_CHAR messages only for keys mapped to ASCII characters
 * by the keyboard driver.
 *
 * If the message is WM_KEYDOWN, WM_KEYUP, WM_SYSKEYDOWN, or WM_SYSKEYUP, the
 * return value is nonzero, regardless of the translation.
 *
 */
BOOL WINAPI TranslateMessage( const MSG *msg )
{
    UINT message;
    WCHAR wp[2];
    BYTE state[256];

    if (msg->message < WM_KEYFIRST || msg->message > WM_KEYLAST) return FALSE;
    if (msg->message != WM_KEYDOWN && msg->message != WM_SYSKEYDOWN) return TRUE;

    TRACE_(key)("Translating key %s (%04lX), scancode %04x\n",
                SPY_GetVKeyName(msg->wParam), msg->wParam, HIWORD(msg->lParam));

    switch (msg->wParam)
    {
    case VK_PACKET:
        message = (msg->message == WM_KEYDOWN) ? WM_CHAR : WM_SYSCHAR;
        TRACE_(key)("PostMessageW(%p,%s,%04x,%08x)\n",
                    msg->hwnd, SPY_GetMsgName(message, msg->hwnd), HIWORD(msg->lParam), LOWORD(msg->lParam));
        PostMessageW( msg->hwnd, message, HIWORD(msg->lParam), LOWORD(msg->lParam));
        return TRUE;
#if 0
    case VK_PROCESSKEY:
        return ImmTranslateMessage(msg->hwnd, msg->message, msg->wParam, msg->lParam);
#endif
    }

    GetKeyboardState( state );
    /* FIXME : should handle ToUnicode yielding 2 */
    switch (ToUnicode(msg->wParam, HIWORD(msg->lParam), state, wp, 2, 0))
    {
    case 1:
        message = (msg->message == WM_KEYDOWN) ? WM_CHAR : WM_SYSCHAR;
        TRACE_(key)("1 -> PostMessageW(%p,%s,%04x,%08lx)\n",
            msg->hwnd, SPY_GetMsgName(message, msg->hwnd), wp[0], msg->lParam);
        PostMessageW( msg->hwnd, message, wp[0], msg->lParam );
        break;

    case -1:
        message = (msg->message == WM_KEYDOWN) ? WM_DEADCHAR : WM_SYSDEADCHAR;
        TRACE_(key)("-1 -> PostMessageW(%p,%s,%04x,%08lx)\n",
            msg->hwnd, SPY_GetMsgName(message, msg->hwnd), wp[0], msg->lParam);
        PostMessageW( msg->hwnd, message, wp[0], msg->lParam );
        break;
    }
    return TRUE;
}


/***********************************************************************
 *		DispatchMessageA (USER32.@)
 *
 * See DispatchMessageW.
 */
LRESULT WINAPI DECLSPEC_HOTPATCH DispatchMessageA( const MSG* msg )
{
    LRESULT retval;

      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
        if (msg->lParam)
        {
            __TRY
            {
                retval = CallWindowProcA( (WNDPROC)msg->lParam, msg->hwnd,
                                          msg->message, msg->wParam, GetTickCount() );
            }
            __EXCEPT_PAGE_FAULT
            {
                retval = 0;
            }
            __ENDTRY
            return retval;
        }
    }
    if (!msg->hwnd) return 0;

    SPY_EnterMessage( SPY_DISPATCHMESSAGE, msg->hwnd, msg->message,
                      msg->wParam, msg->lParam );

    if (!WINPROC_call_window( msg->hwnd, msg->message, msg->wParam, msg->lParam,
                              &retval, FALSE, WMCHAR_MAP_DISPATCHMESSAGE ))
    {
        if (!IsWindow( msg->hwnd )) SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        else SetLastError( ERROR_MESSAGE_SYNC_ONLY );
        retval = 0;
    }

    SPY_ExitMessage( SPY_RESULT_OK, msg->hwnd, msg->message, retval,
                     msg->wParam, msg->lParam );

    if (msg->message == WM_PAINT)
    {
        /* send a WM_NCPAINT and WM_ERASEBKGND if the non-client area is still invalid */
        HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
        GetUpdateRgn( msg->hwnd, hrgn, TRUE );
        DeleteObject( hrgn );
    }
    return retval;
}


/***********************************************************************
 *		DispatchMessageW (USER32.@) Process a message
 *
 * Process the message specified in the structure *_msg_.
 *
 * If the lpMsg parameter points to a WM_TIMER message and the
 * parameter of the WM_TIMER message is not NULL, the lParam parameter
 * points to the function that is called instead of the window
 * procedure. The function stored in lParam (timer callback) is protected
 * from causing page-faults.
 *
 * The message must be valid.
 *
 * RETURNS
 *
 *   DispatchMessage() returns the result of the window procedure invoked.
 *
 * CONFORMANCE
 *
 *   ECMA-234, Win32
 *
 */
LRESULT WINAPI DECLSPEC_HOTPATCH DispatchMessageW( const MSG* msg )
{
    LRESULT retval;

      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
        if (msg->lParam)
        {
            __TRY
            {
                retval = CallWindowProcW( (WNDPROC)msg->lParam, msg->hwnd,
                                          msg->message, msg->wParam, GetTickCount() );
            }
            __EXCEPT_PAGE_FAULT
            {
                retval = 0;
            }
            __ENDTRY
            return retval;
        }
    }
    if (!msg->hwnd) return 0;

    SPY_EnterMessage( SPY_DISPATCHMESSAGE, msg->hwnd, msg->message,
                      msg->wParam, msg->lParam );

    if (!WINPROC_call_window( msg->hwnd, msg->message, msg->wParam, msg->lParam,
                              &retval, TRUE, WMCHAR_MAP_DISPATCHMESSAGE ))
    {
        if (!IsWindow( msg->hwnd )) SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        else SetLastError( ERROR_MESSAGE_SYNC_ONLY );
        retval = 0;
    }

    SPY_ExitMessage( SPY_RESULT_OK, msg->hwnd, msg->message, retval,
                     msg->wParam, msg->lParam );

    if (msg->message == WM_PAINT)
    {
        /* send a WM_NCPAINT and WM_ERASEBKGND if the non-client area is still invalid */
        HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
        GetUpdateRgn( msg->hwnd, hrgn, TRUE );
        DeleteObject( hrgn );
    }
    return retval;
}


/***********************************************************************
 *		GetMessagePos (USER.119)
 *		GetMessagePos (USER32.@)
 *
 * The GetMessagePos() function returns a long value representing a
 * cursor position, in screen coordinates, when the last message
 * retrieved by the GetMessage() function occurs. The x-coordinate is
 * in the low-order word of the return value, the y-coordinate is in
 * the high-order word. The application can use the MAKEPOINT()
 * macro to obtain a POINT structure from the return value.
 *
 * For the current cursor position, use GetCursorPos().
 *
 * RETURNS
 *
 * Cursor position of last message on success, zero on failure.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win32
 *
 */
DWORD WINAPI GetMessagePos(void)
{
    return get_user_thread_info()->GetMessagePosVal;
}


/***********************************************************************
 *		GetMessageTime (USER.120)
 *		GetMessageTime (USER32.@)
 *
 * GetMessageTime() returns the message time for the last message
 * retrieved by the function. The time is measured in milliseconds with
 * the same offset as GetTickCount().
 *
 * Since the tick count wraps, this is only useful for moderately short
 * relative time comparisons.
 *
 * RETURNS
 *
 * Time of last message on success, zero on failure.
 */
LONG WINAPI GetMessageTime(void)
{
    return get_user_thread_info()->GetMessageTimeVal;
}


/***********************************************************************
 *		GetMessageExtraInfo (USER.288)
 *		GetMessageExtraInfo (USER32.@)
 */
LPARAM WINAPI GetMessageExtraInfo(void)
{
    return get_user_thread_info()->GetMessageExtraInfoVal;
}


/***********************************************************************
 *		SetMessageExtraInfo (USER32.@)
 */
LPARAM WINAPI SetMessageExtraInfo(LPARAM lParam)
{
    struct user_thread_info *thread_info = get_user_thread_info();
    LONG old_value = thread_info->GetMessageExtraInfoVal;
    thread_info->GetMessageExtraInfoVal = lParam;
    return old_value;
}


/***********************************************************************
 *		WaitMessage (USER.112) Suspend thread pending messages
 *		WaitMessage (USER32.@) Suspend thread pending messages
 *
 * WaitMessage() suspends a thread until events appear in the thread's
 * queue.
 */
BOOL WINAPI WaitMessage(void)
{
    return (MsgWaitForMultipleObjectsEx( 0, NULL, INFINITE, QS_ALLINPUT, 0 ) != WAIT_FAILED);
}


/***********************************************************************
 *		MsgWaitForMultipleObjectsEx   (USER32.@)
 */
DWORD WINAPI MsgWaitForMultipleObjectsEx( DWORD count, CONST HANDLE *pHandles,
                                          DWORD timeout, DWORD mask, DWORD flags )
{
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    DWORD i;

    if (count > MAXIMUM_WAIT_OBJECTS-1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }

    /* set the queue mask */
    SERVER_START_REQ( set_queue_mask )
    {
        req->wake_mask    = (flags & MWMO_INPUTAVAILABLE) ? mask : 0;
        req->changed_mask = mask;
        req->skip_wait    = 0;
        wine_server_call( req );
    }
    SERVER_END_REQ;

    /* add the queue to the handle list */
    for (i = 0; i < count; i++) handles[i] = pHandles[i];
    handles[count] = get_server_queue_handle();

    return wow_handlers.wait_message( count+1, handles, timeout, mask, flags );
}


/***********************************************************************
 *		MsgWaitForMultipleObjects (USER32.@)
 */
DWORD WINAPI MsgWaitForMultipleObjects( DWORD count, CONST HANDLE *handles,
                                        BOOL wait_all, DWORD timeout, DWORD mask )
{
    return MsgWaitForMultipleObjectsEx( count, handles, timeout, mask,
                                        wait_all ? MWMO_WAITALL : 0 );
}


/***********************************************************************
 *		WaitForInputIdle (USER32.@)
 */
DWORD WINAPI WaitForInputIdle( HANDLE hProcess, DWORD dwTimeOut )
{
    DWORD start_time, elapsed, ret;
    HANDLE handles[2];

    handles[0] = hProcess;
    SERVER_START_REQ( get_process_idle_event )
    {
        req->handle = wine_server_obj_handle( hProcess );
        wine_server_call_err( req );
        handles[1] = wine_server_ptr_handle( reply->event );
    }
    SERVER_END_REQ;
    if (!handles[1]) return WAIT_FAILED;  /* no event to wait on */

    start_time = GetTickCount();
    elapsed = 0;

    TRACE("waiting for %p\n", handles[1] );
    do
    {
        ret = MsgWaitForMultipleObjects ( 2, handles, FALSE, dwTimeOut - elapsed, QS_SENDMESSAGE );
        switch (ret)
        {
        case WAIT_OBJECT_0:
            return 0;
        case WAIT_OBJECT_0+2:
            process_sent_messages();
            break;
        case WAIT_TIMEOUT:
        case WAIT_FAILED:
            TRACE("timeout or error\n");
            return ret;
        default:
            TRACE("finished\n");
            return 0;
        }
        if (dwTimeOut != INFINITE)
        {
            elapsed = GetTickCount() - start_time;
            if (elapsed > dwTimeOut)
                break;
        }
    }
    while (1);

    return WAIT_TIMEOUT;
}


/***********************************************************************
 *		RegisterWindowMessageA (USER32.@)
 *		RegisterWindowMessage (USER.118)
 */
UINT WINAPI RegisterWindowMessageA( LPCSTR str )
{
    UINT ret = UserAddAtomA(str);
    TRACE("%s, ret=%x\n", str, ret);
    return ret;
}


/***********************************************************************
 *		RegisterWindowMessageW (USER32.@)
 */
UINT WINAPI RegisterWindowMessageW( LPCWSTR str )
{
    UINT ret = UserAddAtomW(str);
    TRACE("%s ret=%x\n", debugstr_w(str), ret);
    return ret;
}

typedef struct BroadcastParm
{
    DWORD flags;
    LPDWORD recipients;
    UINT msg;
    WPARAM wp;
    LPARAM lp;
    DWORD success;
    HWINSTA winsta;
} BroadcastParm;

static BOOL CALLBACK bcast_childwindow( HWND hw, LPARAM lp )
{
    BroadcastParm *parm = (BroadcastParm*)lp;
    DWORD_PTR retval = 0;
    LRESULT lresult;

    if (parm->flags & BSF_IGNORECURRENTTASK && WIN_IsCurrentProcess(hw))
    {
        TRACE("Not telling myself %p\n", hw);
        return TRUE;
    }

    /* I don't know 100% for sure if this is what Windows does, but it fits the tests */
    if (parm->flags & BSF_QUERY)
    {
        TRACE("Telling window %p using SendMessageTimeout\n", hw);

        /* Not tested for conflicting flags */
        if (parm->flags & BSF_FORCEIFHUNG || parm->flags & BSF_NOHANG)
            lresult = SendMessageTimeoutW( hw, parm->msg, parm->wp, parm->lp, SMTO_ABORTIFHUNG, 2000, &retval );
        else if (parm->flags & BSF_NOTIMEOUTIFNOTHUNG)
            lresult = SendMessageTimeoutW( hw, parm->msg, parm->wp, parm->lp, SMTO_NOTIMEOUTIFNOTHUNG, 2000, &retval );
        else
            lresult = SendMessageTimeoutW( hw, parm->msg, parm->wp, parm->lp, SMTO_NORMAL, 2000, &retval );

        if (!lresult && GetLastError() == ERROR_TIMEOUT)
        {
            WARN("Timed out!\n");
            if (!(parm->flags & BSF_FORCEIFHUNG))
                goto fail;
        }
        if (retval == BROADCAST_QUERY_DENY)
            goto fail;

        return TRUE;

fail:
        parm->success = 0;
        return FALSE;
    }
    else if (parm->flags & BSF_POSTMESSAGE)
    {
        TRACE("Telling window %p using PostMessage\n", hw);
        PostMessageW( hw, parm->msg, parm->wp, parm->lp );
    }
    else
    {
        TRACE("Telling window %p using SendNotifyMessage\n", hw);
        SendNotifyMessageW( hw, parm->msg, parm->wp, parm->lp );
    }

    return TRUE;
}

static BOOL CALLBACK bcast_desktop( LPWSTR desktop, LPARAM lp )
{
    BOOL ret;
    HDESK hdesktop;
    BroadcastParm *parm = (BroadcastParm*)lp;

    TRACE("desktop: %s\n", debugstr_w( desktop ));

    hdesktop = open_winstation_desktop( parm->winsta, desktop, 0, FALSE, DESKTOP_ENUMERATE|DESKTOP_WRITEOBJECTS|STANDARD_RIGHTS_WRITE );
    if (!hdesktop)
    {
        FIXME("Could not open desktop %s\n", debugstr_w(desktop));
        return TRUE;
    }

    ret = EnumDesktopWindows( hdesktop, bcast_childwindow, lp );
    CloseDesktop(hdesktop);
    TRACE("-->%d\n", ret);
    return parm->success;
}

static BOOL CALLBACK bcast_winsta( LPWSTR winsta, LPARAM lp )
{
    BOOL ret;
    HWINSTA hwinsta = OpenWindowStationW( winsta, FALSE, WINSTA_ENUMDESKTOPS );
    TRACE("hwinsta: %p/%s/%08x\n", hwinsta, debugstr_w( winsta ), GetLastError());
    if (!hwinsta)
        return TRUE;
    ((BroadcastParm *)lp)->winsta = hwinsta;
    ret = EnumDesktopsW( hwinsta, bcast_desktop, lp );
    CloseWindowStation( hwinsta );
    TRACE("-->%d\n", ret);
    return ret;
}

/***********************************************************************
 *		BroadcastSystemMessageA (USER32.@)
 *		BroadcastSystemMessage  (USER32.@)
 */
LONG WINAPI BroadcastSystemMessageA( DWORD flags, LPDWORD recipients, UINT msg, WPARAM wp, LPARAM lp )
{
    return BroadcastSystemMessageExA( flags, recipients, msg, wp, lp, NULL );
}


/***********************************************************************
 *		BroadcastSystemMessageW (USER32.@)
 */
LONG WINAPI BroadcastSystemMessageW( DWORD flags, LPDWORD recipients, UINT msg, WPARAM wp, LPARAM lp )
{
    return BroadcastSystemMessageExW( flags, recipients, msg, wp, lp, NULL );
}

/***********************************************************************
 *              BroadcastSystemMessageExA (USER32.@)
 */
LONG WINAPI BroadcastSystemMessageExA( DWORD flags, LPDWORD recipients, UINT msg, WPARAM wp, LPARAM lp, PBSMINFO pinfo )
{
    map_wparam_AtoW( msg, &wp, WMCHAR_MAP_NOMAPPING );
    return BroadcastSystemMessageExW( flags, recipients, msg, wp, lp, NULL );
}


/***********************************************************************
 *              BroadcastSystemMessageExW (USER32.@)
 */
LONG WINAPI BroadcastSystemMessageExW( DWORD flags, LPDWORD recipients, UINT msg, WPARAM wp, LPARAM lp, PBSMINFO pinfo )
{
    BroadcastParm parm;
    DWORD recips = BSM_ALLCOMPONENTS;
    BOOL ret = TRUE;
    static const DWORD all_flags = ( BSF_QUERY | BSF_IGNORECURRENTTASK | BSF_FLUSHDISK | BSF_NOHANG
                                   | BSF_POSTMESSAGE | BSF_FORCEIFHUNG | BSF_NOTIMEOUTIFNOTHUNG
                                   | BSF_ALLOWSFW | BSF_SENDNOTIFYMESSAGE | BSF_RETURNHDESK | BSF_LUID );

    TRACE("Flags: %08x, recipients: %p(0x%x), msg: %04x, wparam: %08lx, lparam: %08lx\n", flags, recipients,
         (recipients ? *recipients : recips), msg, wp, lp);

    if (flags & ~all_flags)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!recipients)
        recipients = &recips;

    if ( pinfo && flags & BSF_QUERY )
        FIXME("Not returning PBSMINFO information yet\n");

    parm.flags = flags;
    parm.recipients = recipients;
    parm.msg = msg;
    parm.wp = wp;
    parm.lp = lp;
    parm.success = TRUE;

    if (*recipients & BSM_ALLDESKTOPS || *recipients == BSM_ALLCOMPONENTS)
        ret = EnumWindowStationsW(bcast_winsta, (LONG_PTR)&parm);
    else if (*recipients & BSM_APPLICATIONS)
    {
        EnumWindows(bcast_childwindow, (LONG_PTR)&parm);
        ret = parm.success;
    }
    else
        FIXME("Recipients %08x not supported!\n", *recipients);

    return ret;
}

/***********************************************************************
 *		SetMessageQueue (USER32.@)
 */
BOOL WINAPI SetMessageQueue( INT size )
{
    /* now obsolete the message queue will be expanded dynamically as necessary */
    return TRUE;
}


/***********************************************************************
 *		MessageBeep (USER32.@)
 */
BOOL WINAPI MessageBeep( UINT i )
{
    BOOL active = TRUE;
    SystemParametersInfoA( SPI_GETBEEP, 0, &active, FALSE );
    if (active) USER_Driver->pBeep();
    return TRUE;
}


/***********************************************************************
 *		SetTimer (USER32.@)
 */
UINT_PTR WINAPI SetTimer( HWND hwnd, UINT_PTR id, UINT timeout, TIMERPROC proc )
{
    UINT_PTR ret;
    WNDPROC winproc = 0;

    if (proc) winproc = WINPROC_AllocProc( (WNDPROC)proc, FALSE );

    SERVER_START_REQ( set_win_timer )
    {
        req->win    = wine_server_user_handle( hwnd );
        req->msg    = WM_TIMER;
        req->id     = id;
        req->rate   = max( timeout, SYS_TIMER_RATE );
        req->lparam = (ULONG_PTR)winproc;
        if (!wine_server_call_err( req ))
        {
            ret = reply->id;
            if (!ret) ret = TRUE;
        }
        else ret = 0;
    }
    SERVER_END_REQ;

    TRACE("Added %p %lx %p timeout %d\n", hwnd, id, winproc, timeout );
    return ret;
}


/***********************************************************************
 *		SetSystemTimer (USER32.@)
 */
UINT_PTR WINAPI SetSystemTimer( HWND hwnd, UINT_PTR id, UINT timeout, TIMERPROC proc )
{
    UINT_PTR ret;
    WNDPROC winproc = 0;

    if (proc) winproc = WINPROC_AllocProc( (WNDPROC)proc, FALSE );

    SERVER_START_REQ( set_win_timer )
    {
        req->win    = wine_server_user_handle( hwnd );
        req->msg    = WM_SYSTIMER;
        req->id     = id;
        req->rate   = max( timeout, SYS_TIMER_RATE );
        req->lparam = (ULONG_PTR)winproc;
        if (!wine_server_call_err( req ))
        {
            ret = reply->id;
            if (!ret) ret = TRUE;
        }
        else ret = 0;
    }
    SERVER_END_REQ;

    TRACE("Added %p %lx %p timeout %d\n", hwnd, id, winproc, timeout );
    return ret;
}


/***********************************************************************
 *		KillTimer (USER32.@)
 */
BOOL WINAPI KillTimer( HWND hwnd, UINT_PTR id )
{
    BOOL ret;

    SERVER_START_REQ( kill_win_timer )
    {
        req->win = wine_server_user_handle( hwnd );
        req->msg = WM_TIMER;
        req->id  = id;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *		KillSystemTimer (USER32.@)
 */
BOOL WINAPI KillSystemTimer( HWND hwnd, UINT_PTR id )
{
    BOOL ret;

    SERVER_START_REQ( kill_win_timer )
    {
        req->win = wine_server_user_handle( hwnd );
        req->msg = WM_SYSTIMER;
        req->id  = id;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 *		IsGUIThread  (USER32.@)
 */
BOOL WINAPI IsGUIThread( BOOL convert )
{
    FIXME( "%u: stub\n", convert );
    return TRUE;
}


/**********************************************************************
 *		GetGUIThreadInfo  (USER32.@)
 */
BOOL WINAPI GetGUIThreadInfo( DWORD id, GUITHREADINFO *info )
{
    BOOL ret;

    if (info->cbSize != sizeof(*info))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    SERVER_START_REQ( get_thread_input )
    {
        req->tid = id;
        if ((ret = !wine_server_call_err( req )))
        {
            info->flags          = 0;
            info->hwndActive     = wine_server_ptr_handle( reply->active );
            info->hwndFocus      = wine_server_ptr_handle( reply->focus );
            info->hwndCapture    = wine_server_ptr_handle( reply->capture );
            info->hwndMenuOwner  = wine_server_ptr_handle( reply->menu_owner );
            info->hwndMoveSize   = wine_server_ptr_handle( reply->move_size );
            info->hwndCaret      = wine_server_ptr_handle( reply->caret );
            info->rcCaret.left   = reply->rect.left;
            info->rcCaret.top    = reply->rect.top;
            info->rcCaret.right  = reply->rect.right;
            info->rcCaret.bottom = reply->rect.bottom;
            if (reply->menu_owner) info->flags |= GUI_INMENUMODE;
            if (reply->move_size) info->flags |= GUI_INMOVESIZE;
            if (reply->caret) info->flags |= GUI_CARETBLINKING;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************
 *		IsHungAppWindow (USER32.@)
 *
 */
BOOL WINAPI IsHungAppWindow( HWND hWnd )
{
    BOOL ret;

    SERVER_START_REQ( is_window_hung )
    {
        req->win = wine_server_user_handle( hWnd );
        ret = !wine_server_call_err( req ) && reply->is_hung;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************
 *      ChangeWindowMessageFilter (USER32.@)
 */
BOOL WINAPI ChangeWindowMessageFilter( UINT message, DWORD flag )
{
    FIXME( "%x %08x\n", message, flag );
    return TRUE;
}
