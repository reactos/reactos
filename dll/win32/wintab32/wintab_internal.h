/*
 * Tablet header
 *
 * Copyright 2003 CodeWeavers (Aric Stewart)
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

#ifndef __WINE_WINTAB_INTERNAL_H
#define __WINE_WINTAB_INTERNAL_H

typedef struct tagWTI_INTERFACE_INFO {
    CHAR  WINTABID[1024];
        /* a copy of the null-terminated tablet hardware identification string
         * in the user buffer. This string should include make, model, and
         * revision information in user-readable format.
         */
    WORD    SPECVERSION;
        /* the specification version number. The high-order byte contains the
         * major version number; the low-order byte contains the minor version
         * number.
         */
    WORD    IMPLVERSION;
        /* the implementation version number. The high-order byte contains the
         * major version number; the low-order byte contains the minor version
         * number.
         */
    UINT    NDEVICES;
        /* the number of devices supported. */
    UINT    NCURSORS;
        /* the total number of cursor types supported. */
    UINT    NCONTEXTS;
        /* the number of contexts supported. */
    UINT    CTXOPTIONS;
        /* flags indicating which context options are supported */
    UINT    CTXSAVESIZE;
        /* the size of the save information returned from WTSave.*/
    UINT    NEXTENSIONS;
        /* the number of extension data items supported.*/
    UINT    NMANAGERS;
        /* the number of manager handles supported.*/
    }WTI_INTERFACE_INFO, *LPWTI_INTERFACE_INFO;

typedef struct tagWTI_STATUS_INFO{
    UINT    CONTEXTS;
        /* the number of contexts currently open.*/
    UINT    SYSCTXS;
        /* the number of system contexts currently open.*/
    UINT    PKTRATE;
        /* the maximum packet report rate currently being received by any
         * context, in Hertz.
         */
    WTPKT   PKTDATA;
        /* a mask indicating which packet data items are requested by at
         * least one context.
         */
    UINT    MANAGERS;
        /* the number of manager handles currently open.*/
    BOOL    SYSTEM;
        /* a non-zero value if system pointing is available to the whole
         * screen; zero otherwise.
         */
    DWORD   BUTTONUSE;
        /* a button mask indicating the logical buttons whose events are
         * requested by at least one context.
         */
    DWORD   SYSBTNUSE;
        /* a button mask indicating which logical buttons are assigned a system
         * button function by the current cursor's system button map.
         */
} WTI_STATUS_INFO, *LPWTI_STATUS_INFO;

typedef struct tagWTI_EXTENSIONS_INFO
{
    CHAR   NAME[256];
        /* a unique, null-terminated string describing the extension.*/
    UINT    TAG;
        /* a unique identifier for the extension. */
    WTPKT   MASK;
        /* a mask that can be bitwise OR'ed with WTPKT-type variables to select
         * the extension.
         */
    UINT    SIZE[2];
        /* an array of two UINTs specifying the extension's size within a packet
         * (in bytes). The first is for absolute mode; the second is for
         * relative mode.
         */
    AXIS    *AXES;
        /* an array of axis descriptions, as needed for the extension. */
    BYTE    *DEFAULT;
        /* the current global default data, as needed for the extension.  This
         * data is modified via the WTMgrExt function.
         */
    BYTE    *DEFCONTEXT;
    BYTE    *DEFSYSCTX;
        /* the current default context-specific data, as needed for the
         * extension. The indices identify the digitizing- and system-context
         * defaults, respectively.
         */
    BYTE    *CURSORS;
        /* Is the first of one or more consecutive indices, one per cursor type.
         * Each returns the current default cursor-specific data, as need for
         * the extension. This data is modified via the WTMgrCsrExt function.
         */
} WTI_EXTENSIONS_INFO, *LPWTI_EXTENSIONS_INFO;

typedef struct tagWTPACKET {
        HCTX pkContext;
        UINT pkStatus;
        LONG pkTime;
        WTPKT pkChanged;
        UINT pkSerialNumber;
        UINT pkCursor;
        DWORD pkButtons;
        DWORD pkX;
        DWORD pkY;
        DWORD pkZ;
        UINT pkNormalPressure;
        UINT pkTangentPressure;
        ORIENTATION pkOrientation;
        ROTATION pkRotation; /* 1.1 */
} WTPACKET, *LPWTPACKET;

typedef struct tagOPENCONTEXT
{
    HCTX        handle;
    LOGCONTEXTW context;
    HWND        hwndOwner;
    BOOL        enabled;
    INT         ActiveCursor;
    INT         QueueSize;
    INT         PacketsQueued;
    LPWTPACKET  PacketQueue;
    struct tagOPENCONTEXT *next;
} OPENCONTEXT, *LPOPENCONTEXT;

int TABLET_PostTabletMessage(LPOPENCONTEXT newcontext, UINT msg, WPARAM wParam,
                             LPARAM lParam, BOOL send_always);
LPOPENCONTEXT AddPacketToContextQueue(LPWTPACKET packet, HWND hwnd);

/* X11drv functions */
extern int  (CDECL *pLoadTabletInfo)(HWND hwnddefault);
extern int  (CDECL *pGetCurrentPacket)(LPWTPACKET packet);
extern int  (CDECL *pAttachEventQueueToTablet)(HWND hOwner);
extern UINT (CDECL *pWTInfoW)(UINT wCategory, UINT nIndex, LPVOID lpOutput);

extern HWND hwndDefault;
extern CRITICAL_SECTION csTablet;

#endif /* __WINE_WINTAB_INTERNAL_H */
