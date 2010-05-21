/*
 * XDND handler code
 *
 * Copyright 2003 Ulrich Czekalla
 * Copyright 2007 Damjan Jovanovic
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

#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "x11drv.h"
#include "shlobj.h"  /* DROPFILES */

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xdnd);

/* Maximum wait time for selection notify */
#define SELECTION_RETRIES 500  /* wait for .1 seconds */
#define SELECTION_WAIT    1000 /* us */

typedef struct tagXDNDDATA
{
    int cf_win;
    Atom cf_xdnd;
    void *data;
    unsigned int size;
    struct tagXDNDDATA *next;
} XDNDDATA, *LPXDNDDATA;

static LPXDNDDATA XDNDData = NULL;
static POINT XDNDxy = { 0, 0 };

static void X11DRV_XDND_InsertXDNDData(int property, int format, void* data, unsigned int len);
static int X11DRV_XDND_DeconstructTextURIList(int property, void* data, int len);
static int X11DRV_XDND_DeconstructTextPlain(int property, void* data, int len);
static int X11DRV_XDND_DeconstructTextHTML(int property, void* data, int len);
static int X11DRV_XDND_MapFormat(unsigned int property, unsigned char *data, int len);
static void X11DRV_XDND_ResolveProperty(Display *display, Window xwin, Time tm,
    Atom *types, unsigned long *count);
static void X11DRV_XDND_SendDropFiles(HWND hwnd);
static void X11DRV_XDND_FreeDragDropOp(void);
static unsigned int X11DRV_XDND_UnixToDos(char** lpdest, char* lpsrc, int len);
static WCHAR* X11DRV_XDND_URIToDOS(char *encodedURI);

static CRITICAL_SECTION xdnd_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &xdnd_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": xdnd_cs") }
};
static CRITICAL_SECTION xdnd_cs = { &critsect_debug, -1, 0, 0, 0, 0 };


/**************************************************************************
 * X11DRV_XDND_EnterEvent
 *
 * Handle an XdndEnter event.
 */
void X11DRV_XDND_EnterEvent( HWND hWnd, XClientMessageEvent *event )
{
    int version;
    Atom *xdndtypes;
    unsigned long count = 0;

    version = (event->data.l[1] & 0xFF000000) >> 24;
    TRACE("ver(%d) check-XdndTypeList(%ld) data=%ld,%ld,%ld,%ld,%ld\n",
          version, (event->data.l[1] & 1),
          event->data.l[0], event->data.l[1], event->data.l[2],
          event->data.l[3], event->data.l[4]);

    if (version > WINE_XDND_VERSION)
    {
        TRACE("Ignores unsupported version\n");
        return;
    }

    /* If the source supports more than 3 data types we retrieve
     * the entire list. */
    if (event->data.l[1] & 1)
    {
        Atom acttype;
        int actfmt;
        unsigned long bytesret;

        /* Request supported formats from source window */
        wine_tsx11_lock();
        XGetWindowProperty(event->display, event->data.l[0], x11drv_atom(XdndTypeList),
                           0, 65535, FALSE, AnyPropertyType, &acttype, &actfmt, &count,
                           &bytesret, (unsigned char**)&xdndtypes);
        wine_tsx11_unlock();
    }
    else
    {
        count = 3;
        xdndtypes = (Atom*) &event->data.l[2];
    }

    if (TRACE_ON(xdnd))
    {
        unsigned int i = 0;

        wine_tsx11_lock();
        for (; i < count; i++)
        {
            if (xdndtypes[i] != 0)
            {
                char * pn = XGetAtomName(event->display, xdndtypes[i]);
                TRACE("XDNDEnterAtom %ld: %s\n", xdndtypes[i], pn);
                XFree(pn);
            }
        }
        wine_tsx11_unlock();
    }

    /* Do a one-time data read and cache results */
    X11DRV_XDND_ResolveProperty(event->display, event->window,
                                event->data.l[1], xdndtypes, &count);

    if (event->data.l[1] & 1)
        XFree(xdndtypes);
}

/**************************************************************************
 * X11DRV_XDND_PositionEvent
 *
 * Handle an XdndPosition event.
 */
void X11DRV_XDND_PositionEvent( HWND hWnd, XClientMessageEvent *event )
{
    XClientMessageEvent e;
    int accept = 0; /* Assume we're not accepting */

    XDNDxy.x = event->data.l[2] >> 16;
    XDNDxy.y = event->data.l[2] & 0xFFFF;

    /* FIXME: Notify OLE of DragEnter. Result determines if we accept */

    if (GetWindowLongW( hWnd, GWL_EXSTYLE ) & WS_EX_ACCEPTFILES)
        accept = 1;

    TRACE("action req: %ld accept(%d) at x(%d),y(%d)\n",
          event->data.l[4], accept, XDNDxy.x, XDNDxy.y);

    /*
     * Let source know if we're accepting the drop by
     * sending a status message.
     */
    e.type = ClientMessage;
    e.display = event->display;
    e.window = event->data.l[0];
    e.message_type = x11drv_atom(XdndStatus);
    e.format = 32;
    e.data.l[0] = event->window;
    e.data.l[1] = accept;
    e.data.l[2] = 0; /* Empty Rect */
    e.data.l[3] = 0; /* Empty Rect */
    if (accept)
        e.data.l[4] = event->data.l[4];
    else
        e.data.l[4] = None;
    wine_tsx11_lock();
    XSendEvent(event->display, event->data.l[0], False, NoEventMask, (XEvent*)&e);
    wine_tsx11_unlock();

    /* FIXME: if drag accepted notify OLE of DragOver */
}

/**************************************************************************
 * X11DRV_XDND_DropEvent
 *
 * Handle an XdndDrop event.
 */
void X11DRV_XDND_DropEvent( HWND hWnd, XClientMessageEvent *event )
{
    XClientMessageEvent e;

    TRACE("\n");

    /* If we have a HDROP type we send a WM_ACCEPTFILES.*/
    if (GetWindowLongW( hWnd, GWL_EXSTYLE ) & WS_EX_ACCEPTFILES)
        X11DRV_XDND_SendDropFiles( hWnd );

    /* FIXME: Notify OLE of Drop */
    X11DRV_XDND_FreeDragDropOp();

    /* Tell the target we are finished. */
    memset(&e, 0, sizeof(e));
    e.type = ClientMessage;
    e.display = event->display;
    e.window = event->data.l[0];
    e.message_type = x11drv_atom(XdndFinished);
    e.format = 32;
    e.data.l[0] = event->window;
    wine_tsx11_lock();
    XSendEvent(event->display, event->data.l[0], False, NoEventMask, (XEvent*)&e);
    wine_tsx11_unlock();
}

/**************************************************************************
 * X11DRV_XDND_LeaveEvent
 *
 * Handle an XdndLeave event.
 */
void X11DRV_XDND_LeaveEvent( HWND hWnd, XClientMessageEvent *event )
{
    TRACE("DND Operation canceled\n");

    X11DRV_XDND_FreeDragDropOp();

    /* FIXME: Notify OLE of DragLeave */
}


/**************************************************************************
 * X11DRV_XDND_ResolveProperty
 *
 * Resolve all MIME types to windows clipboard formats. All data is cached.
 */
static void X11DRV_XDND_ResolveProperty(Display *display, Window xwin, Time tm,
                                        Atom *types, unsigned long *count)
{
    unsigned int i, j;
    BOOL res;
    XEvent xe;
    Atom acttype;
    int actfmt;
    unsigned long bytesret, icount;
    int entries = 0;
    unsigned char* data = NULL;

    TRACE("count(%ld)\n", *count);

    X11DRV_XDND_FreeDragDropOp(); /* Clear previously cached data */

    for (i = 0; i < *count; i++)
    {
        TRACE("requesting atom %ld from xwin %ld\n", types[i], xwin);

        if (types[i] == 0)
            continue;

        wine_tsx11_lock();
        XConvertSelection(display, x11drv_atom(XdndSelection), types[i],
                          x11drv_atom(XdndTarget), xwin, /*tm*/CurrentTime);
        wine_tsx11_unlock();

        /*
         * Wait for SelectionNotify
         */
        for (j = 0; j < SELECTION_RETRIES; j++)
        {
            wine_tsx11_lock();
            res = XCheckTypedWindowEvent(display, xwin, SelectionNotify, &xe);
            wine_tsx11_unlock();
            if (res && xe.xselection.selection == x11drv_atom(XdndSelection)) break;

            usleep(SELECTION_WAIT);
        }

        if (xe.xselection.property == None)
            continue;

        wine_tsx11_lock();
        XGetWindowProperty(display, xwin, x11drv_atom(XdndTarget), 0, 65535, FALSE,
            AnyPropertyType, &acttype, &actfmt, &icount, &bytesret, &data);
        wine_tsx11_unlock();

        entries += X11DRV_XDND_MapFormat(types[i], data, get_property_size( actfmt, icount ));
        wine_tsx11_lock();
        XFree(data);
        wine_tsx11_unlock();
    }

    *count = entries;
}


/**************************************************************************
 * X11DRV_XDND_InsertXDNDData
 *
 * Cache available XDND property
 */
static void X11DRV_XDND_InsertXDNDData(int property, int format, void* data, unsigned int len)
{
    LPXDNDDATA current = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(XDNDDATA));

    if (current)
    {
        EnterCriticalSection(&xdnd_cs);
        current->next = XDNDData;
        current->cf_xdnd = property;
        current->cf_win = format;
        current->data = data;
        current->size = len;
        XDNDData = current;
        LeaveCriticalSection(&xdnd_cs);
    }
}


/**************************************************************************
 * X11DRV_XDND_MapFormat
 *
 * Map XDND MIME format to windows clipboard format.
 */
static int X11DRV_XDND_MapFormat(unsigned int property, unsigned char *data, int len)
{
    void* xdata;
    int count = 0;

    TRACE("%d: %s\n", property, data);

    /* Always include the raw type */
    xdata = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    memcpy(xdata, data, len);
    X11DRV_XDND_InsertXDNDData(property, property, xdata, len);
    count++;

    if (property == x11drv_atom(text_uri_list))
        count += X11DRV_XDND_DeconstructTextURIList(property, data, len);
    else if (property == x11drv_atom(text_plain))
        count += X11DRV_XDND_DeconstructTextPlain(property, data, len);
    else if (property == x11drv_atom(text_html))
        count += X11DRV_XDND_DeconstructTextHTML(property, data, len);

    return count;
}


/**************************************************************************
 * X11DRV_XDND_DeconstructTextURIList
 *
 * Interpret text/uri-list data and add records to <dndfmt> linked list
 */
static int X11DRV_XDND_DeconstructTextURIList(int property, void* data, int len)
{
    char *uriList = data;
    char *uri;
    WCHAR *path;

    WCHAR *out = NULL;
    int size = 0;
    int capacity = 4096;

    int count = 0;
    int start = 0;
    int end = 0;

    out = HeapAlloc(GetProcessHeap(), 0, capacity * sizeof(WCHAR));
    if (out == NULL)
        return 0;

    while (end < len)
    {
        while (end < len && uriList[end] != '\r')
            ++end;
        if (end < (len - 1) && uriList[end+1] != '\n')
        {
            WARN("URI list line doesn't end in \\r\\n\n");
            break;
        }

        uri = HeapAlloc(GetProcessHeap(), 0, end - start + 1);
        if (uri == NULL)
            break;
        lstrcpynA(uri, &uriList[start], end - start + 1);
        path = X11DRV_XDND_URIToDOS(uri);
        TRACE("converted URI %s to DOS path %s\n", debugstr_a(uri), debugstr_w(path));
        HeapFree(GetProcessHeap(), 0, uri);

        if (path)
        {
            int pathSize = strlenW(path) + 1;
            if (pathSize > capacity-size)
            {
                capacity = 2*capacity + pathSize;
                out = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, out, (capacity + 1)*sizeof(WCHAR));
                if (out == NULL)
                    goto done;
            }
            memcpy(&out[size], path, pathSize * sizeof(WCHAR));
            size += pathSize;
        done:
            HeapFree(GetProcessHeap(), 0, path);
            if (out == NULL)
                break;
        }

        start = end + 2;
        end = start;
    }
    if (out && end >= len)
    {
        DROPFILES *dropFiles;
        dropFiles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DROPFILES) + (size + 1)*sizeof(WCHAR));
        if (dropFiles)
        {
            dropFiles->pFiles = sizeof(DROPFILES);
            dropFiles->pt.x = XDNDxy.x;
            dropFiles->pt.y = XDNDxy.y;
            dropFiles->fNC = 0;
            dropFiles->fWide = TRUE;
            out[size] = '\0';
            memcpy(((char*)dropFiles) + dropFiles->pFiles, out, (size + 1)*sizeof(WCHAR));
            X11DRV_XDND_InsertXDNDData(property, CF_HDROP, dropFiles, sizeof(DROPFILES) + (size + 1)*sizeof(WCHAR));
            count = 1;
        }
    }
    HeapFree(GetProcessHeap(), 0, out);
    return count;
}


/**************************************************************************
 * X11DRV_XDND_DeconstructTextPlain
 *
 * Interpret text/plain Data and add records to <dndfmt> linked list
 */
static int X11DRV_XDND_DeconstructTextPlain(int property, void* data, int len)
{
    char* dostext;

    /* Always supply plain text */
    X11DRV_XDND_UnixToDos(&dostext, data, len);
    X11DRV_XDND_InsertXDNDData(property, CF_TEXT, dostext, strlen(dostext));

    TRACE("CF_TEXT (%d): %s\n", CF_TEXT, dostext);

    return 1;
}


/**************************************************************************
 * X11DRV_XDND_DeconstructTextHTML
 *
 * Interpret text/html data and add records to <dndfmt> linked list
 */
static int X11DRV_XDND_DeconstructTextHTML(int property, void* data, int len)
{
    char* dostext;

    X11DRV_XDND_UnixToDos(&dostext, data, len);

    X11DRV_XDND_InsertXDNDData(property,
        RegisterClipboardFormatA("UniformResourceLocator"), dostext, strlen(dostext));

    TRACE("UniformResourceLocator: %s\n", dostext);

    return 1;
}


/**************************************************************************
 * X11DRV_XDND_SendDropFiles
 */
static void X11DRV_XDND_SendDropFiles(HWND hwnd)
{
    LPXDNDDATA current;

    EnterCriticalSection(&xdnd_cs);

    current = XDNDData;

    /* Find CF_HDROP type if any */
    while (current != NULL)
    {
        if (current->cf_win == CF_HDROP)
            break;
        current = current->next;
    }

    if (current != NULL)
    {
        DROPFILES *lpDrop = current->data;

        if (lpDrop)
        {
            lpDrop->pt.x = XDNDxy.x;
            lpDrop->pt.y = XDNDxy.y;

            TRACE("Sending WM_DROPFILES: hWnd(0x%p) %p(%s)\n", hwnd,
                ((char*)lpDrop) + lpDrop->pFiles, debugstr_w((WCHAR*)(((char*)lpDrop) + lpDrop->pFiles)));

            PostMessageW(hwnd, WM_DROPFILES, (WPARAM)lpDrop, 0L);
        }
    }

    LeaveCriticalSection(&xdnd_cs);
}


/**************************************************************************
 * X11DRV_XDND_FreeDragDropOp
 */
static void X11DRV_XDND_FreeDragDropOp(void)
{
    LPXDNDDATA next;
    LPXDNDDATA current;

    TRACE("\n");

    EnterCriticalSection(&xdnd_cs);

    current = XDNDData;

    /** Free data cache */
    while (current != NULL)
    {
        next = current->next;
        HeapFree(GetProcessHeap(), 0, current);
        current = next;
    }

    XDNDData = NULL;
    XDNDxy.x = XDNDxy.y = 0;

    LeaveCriticalSection(&xdnd_cs);
}



/**************************************************************************
 * X11DRV_XDND_UnixToDos
 */
static unsigned int X11DRV_XDND_UnixToDos(char** lpdest, char* lpsrc, int len)
{
    int i;
    unsigned int destlen, lines;

    for (i = 0, lines = 0; i <= len; i++)
    {
        if (lpsrc[i] == '\n')
            lines++;
    }

    destlen = len + lines + 1;

    if (lpdest)
    {
        char* lpstr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, destlen);
        for (i = 0, lines = 0; i <= len; i++)
        {
            if (lpsrc[i] == '\n')
                lpstr[++lines + i] = '\r';
            lpstr[lines + i] = lpsrc[i];
        }

        *lpdest = lpstr;
    }

    return lines;
}


/**************************************************************************
 * X11DRV_XDND_URIToDOS
 */
static WCHAR* X11DRV_XDND_URIToDOS(char *encodedURI)
{
    WCHAR *ret = NULL;
    int i;
    int j = 0;
    char *uri = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, strlen(encodedURI) + 1);
    if (uri == NULL)
        return NULL;
    for (i = 0; encodedURI[i]; ++i)
    {
        if (encodedURI[i] == '%')
        { 
            if (encodedURI[i+1] && encodedURI[i+2])
            {
                char buffer[3];
                int number;
                buffer[0] = encodedURI[i+1];
                buffer[1] = encodedURI[i+2];
                buffer[2] = '\0';
                sscanf(buffer, "%x", &number);
                uri[j++] = number;
                i += 2;
            }
            else
            {
                WARN("invalid URI encoding in %s\n", debugstr_a(encodedURI));
                HeapFree(GetProcessHeap(), 0, uri);
                return NULL;
            }
        }
        else
            uri[j++] = encodedURI[i];
    }

    /* Read http://www.freedesktop.org/wiki/Draganddropwarts and cry... */
    if (strncmp(uri, "file:/", 6) == 0)
    {
        if (uri[6] == '/')
        {
            if (uri[7] == '/')
            {
                /* file:///path/to/file (nautilus, thunar) */
                ret = wine_get_dos_file_name(&uri[7]);
            }
            else if (uri[7])
            {
                /* file://hostname/path/to/file (X file drag spec) */
                char hostname[256];
                char *path = strchr(&uri[7], '/');
                if (path)
                {
                    *path = '\0';
                    if (strcmp(&uri[7], "localhost") == 0)
                    {
                        *path = '/';
                        ret = wine_get_dos_file_name(path);
                    }
                    else if (gethostname(hostname, sizeof(hostname)) == 0)
                    {
                        if (strcmp(hostname, &uri[7]) == 0)
                        {
                            *path = '/';
                            ret = wine_get_dos_file_name(path);
                        }
                    }
                }
            }
        }
        else if (uri[6])
        {
            /* file:/path/to/file (konqueror) */
            ret = wine_get_dos_file_name(&uri[5]);
        }
    }
    HeapFree(GetProcessHeap(), 0, uri);
    return ret;
}
