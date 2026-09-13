/*
 * Console driver ioctls
 *
 * Copyright 2020 Jacek Caban for CodeWeavers
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

#ifndef _INC_CONDRV
#define _INC_CONDRV

#include "winioctl.h"
#include "wincon.h"

/* common console input and output ioctls */
#define IOCTL_CONDRV_GET_MODE              CTL_CODE(FILE_DEVICE_CONSOLE,  0, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CONDRV_SET_MODE              CTL_CODE(FILE_DEVICE_CONSOLE,  1, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CONDRV_IS_UNIX               CTL_CODE(FILE_DEVICE_CONSOLE,  2, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* console input ioctls */
#define IOCTL_CONDRV_READ_CONSOLE          CTL_CODE(FILE_DEVICE_CONSOLE, 10, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CONDRV_READ_FILE             CTL_CODE(FILE_DEVICE_CONSOLE, 11, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CONDRV_READ_INPUT            CTL_CODE(FILE_DEVICE_CONSOLE, 12, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CONDRV_WRITE_INPUT           CTL_CODE(FILE_DEVICE_CONSOLE, 13, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CONDRV_PEEK                  CTL_CODE(FILE_DEVICE_CONSOLE, 14, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CONDRV_GET_INPUT_INFO        CTL_CODE(FILE_DEVICE_CONSOLE, 15, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CONDRV_SET_INPUT_INFO        CTL_CODE(FILE_DEVICE_CONSOLE, 16, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CONDRV_GET_TITLE             CTL_CODE(FILE_DEVICE_CONSOLE, 17, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CONDRV_SET_TITLE             CTL_CODE(FILE_DEVICE_CONSOLE, 18, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CONDRV_CTRL_EVENT            CTL_CODE(FILE_DEVICE_CONSOLE, 19, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONDRV_BEEP                  CTL_CODE(FILE_DEVICE_CONSOLE, 20, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONDRV_FLUSH                 CTL_CODE(FILE_DEVICE_CONSOLE, 21, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONDRV_GET_WINDOW            CTL_CODE(FILE_DEVICE_CONSOLE, 22, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONDRV_GET_PROCESS_LIST      CTL_CODE(FILE_DEVICE_CONSOLE, 23, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONDRV_READ_CONSOLE_CONTROL  CTL_CODE(FILE_DEVICE_CONSOLE, 24, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CONDRV_GET_INPUT_COUNT       CTL_CODE(FILE_DEVICE_CONSOLE, 25, METHOD_BUFFERED, FILE_READ_ACCESS)

/* console output ioctls */
#define IOCTL_CONDRV_WRITE_CONSOLE         CTL_CODE(FILE_DEVICE_CONSOLE, 30, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CONDRV_WRITE_FILE            CTL_CODE(FILE_DEVICE_CONSOLE, 31, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CONDRV_READ_OUTPUT           CTL_CODE(FILE_DEVICE_CONSOLE, 32, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CONDRV_WRITE_OUTPUT          CTL_CODE(FILE_DEVICE_CONSOLE, 33, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CONDRV_GET_OUTPUT_INFO       CTL_CODE(FILE_DEVICE_CONSOLE, 34, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CONDRV_SET_OUTPUT_INFO       CTL_CODE(FILE_DEVICE_CONSOLE, 35, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CONDRV_ACTIVATE              CTL_CODE(FILE_DEVICE_CONSOLE, 36, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CONDRV_FILL_OUTPUT           CTL_CODE(FILE_DEVICE_CONSOLE, 37, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_CONDRV_SCROLL                CTL_CODE(FILE_DEVICE_CONSOLE, 38, METHOD_BUFFERED, FILE_WRITE_ACCESS)

/* console connection ioctls */
#define IOCTL_CONDRV_BIND_PID              CTL_CODE(FILE_DEVICE_CONSOLE, 51, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* console server ioctls */
#define IOCTL_CONDRV_SETUP_INPUT           CTL_CODE(FILE_DEVICE_CONSOLE, 60, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* ioctls used for communication between driver and host */
#define IOCTL_CONDRV_INIT_OUTPUT           CTL_CODE(FILE_DEVICE_CONSOLE, 90, METHOD_BUFFERED, 0)
#define IOCTL_CONDRV_CLOSE_OUTPUT          CTL_CODE(FILE_DEVICE_CONSOLE, 91, METHOD_BUFFERED, 0)

/* console handle type */
typedef unsigned int condrv_handle_t;

/* convert an object handle to a server handle */
static inline condrv_handle_t condrv_handle( HANDLE handle )
{
    if ((int)(INT_PTR)handle != (INT_PTR)handle) return 0xfffffff0;  /* some invalid handle */
    return (INT_PTR)handle;
}

/* structure for console char/attribute info */
typedef struct
{
    WCHAR          ch;
    unsigned short attr;
} char_info_t;

/* IOCTL_CONDRV_GET_INPUT_INFO result */
struct condrv_input_info
{
    unsigned int  input_cp;       /* console input codepage */
    unsigned int  output_cp;      /* console output codepage */
};

/* IOCTL_CONDRV_SET_INPUT_INFO params */
struct condrv_input_info_params
{
    unsigned int  mask;               /* setting mask */
    struct condrv_input_info info;    /* input_info */
};

#define SET_CONSOLE_INPUT_INFO_INPUT_CODEPAGE   0x01
#define SET_CONSOLE_INPUT_INFO_OUTPUT_CODEPAGE  0x02

/* IOCTL_CONDRV_WRITE_OUTPUT and IOCTL_CONDRV_READ_OUTPUT params */
struct condrv_output_params
{
    unsigned int  x;                  /* destination position */
    unsigned int  y;
    unsigned int  mode;               /* char info mode */
    unsigned int  width;              /* width of output rectangle, 0 for wrapped mode */
    /* followed by an array of data with type depending on mode */
};

enum char_info_mode
{
    CHAR_INFO_MODE_TEXT,              /* characters only */
    CHAR_INFO_MODE_ATTR,              /* attributes only */
    CHAR_INFO_MODE_TEXTATTR,          /* both characters and attributes */
};

/* IOCTL_CONDRV_GET_OUTPUT_INFO result */
struct condrv_output_info
{
    short int     cursor_size;        /* size of cursor (percentage filled) */
    short int     cursor_visible;     /* cursor visibility flag */
    short int     cursor_x;           /* position of cursor (x, y) */
    short int     cursor_y;
    short int     width;              /* width of the screen buffer */
    short int     height;             /* height of the screen buffer */
    short int     attr;               /* default fill attributes (screen colors) */
    short int     popup_attr;         /* pop-up color attributes */
    short int     win_left;           /* window actually displayed by renderer */
    short int     win_top;            /* the rect area is expressed within the */
    short int     win_right;          /* boundaries of the screen buffer */
    short int     win_bottom;
    short int     max_width;          /* maximum size (width x height) for the window */
    short int     max_height;
    short int     font_width;         /* font size (width x height) */
    short int     font_height;
    short int     font_weight;        /* font weight */
    short int     font_pitch_family;  /* font pitch & family */
    unsigned int  color_map[16];      /* color table */
};

/* IOCTL_CONDRV_SET_OUTPUT_INFO params */
struct condrv_output_info_params
{
    unsigned int  mask;               /* setting mask */
    struct condrv_output_info info;   /* output info */
};

#define SET_CONSOLE_OUTPUT_INFO_CURSOR_GEOM     0x0001
#define SET_CONSOLE_OUTPUT_INFO_CURSOR_POS      0x0002
#define SET_CONSOLE_OUTPUT_INFO_SIZE            0x0004
#define SET_CONSOLE_OUTPUT_INFO_ATTR            0x0008
#define SET_CONSOLE_OUTPUT_INFO_DISPLAY_WINDOW  0x0010
#define SET_CONSOLE_OUTPUT_INFO_MAX_SIZE        0x0020
#define SET_CONSOLE_OUTPUT_INFO_POPUP_ATTR      0x0040
#define SET_CONSOLE_OUTPUT_INFO_FONT            0x0080

/* IOCTL_CONDRV_GET_TITLE params */
struct condrv_title_params
{
    unsigned int title_len;
    WCHAR buffer[1];
};

/* IOCTL_CONDRV_FILL_OUTPUT params */
struct condrv_fill_output_params
{
    int            x;                 /* position where to start writing */
    int            y;
    int            mode;              /* char info mode */
    int            count;             /* number to write */
    int            wrap;              /* wrap around at end of line? */
    WCHAR          ch;                /* character to write */
    unsigned short attr;              /* attribute to write */
};

/* IOCTL_CONDRV_SCROLL params */
struct condrv_scroll_params
{
    SMALL_RECT   scroll;              /* source rectangle */
    COORD        origin;              /* destination coordinates */
    SMALL_RECT   clip;                /* clipping rectangle */
    char_info_t  fill;                /* empty character info */
};

/* IOCTL_CONDRV_CTRL_EVENT params */
struct condrv_ctrl_event
{
    int          event;         /* the event to send */
    unsigned int group_id;      /* the group to send the event to */
};

/* Wine specific values for console inheritance (params->ConsoleHandle) */
#define CONSOLE_HANDLE_ALLOC            LongToHandle(-1)
#define CONSOLE_HANDLE_ALLOC_NO_WINDOW  LongToHandle(-2)
#define CONSOLE_HANDLE_SHELL            LongToHandle(-3)
#define CONSOLE_HANDLE_SHELL_NO_WINDOW  LongToHandle(-4)

#endif /* _INC_CONDRV */
