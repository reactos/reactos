/*
 * Thread definitions
 *
 * Copyright 1996 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_THREAD_H
#define __WINE_THREAD_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#define WINE_NO_TEB
#include <winternl.h>
#include <wine/windef16.h>

struct _SECURITY_ATTRIBUTES;
struct tagSYSLEVEL;
struct server_buffer_info;
struct fiber_data;

struct debug_info
{
    char *str_pos;       /* current position in strings buffer */
    char *out_pos;       /* current position in output buffer */
    char  strings[1024]; /* buffer for temporary strings */
    char  output[1024];  /* current output line */
};

/* Thread exception block

  flags in the comment:
  1-- win95 field
  d-- win95 debug version
  -2- nt field
  --3 wine special
  --n wine unused
  !-- or -!- likely or observed  collision
  more problems (collected from mailing list):
  psapi.dll 0x10/0x30 (expects nt fields)
  ie4       0x40
  PESHiELD  0x23/0x30 (win95)
*/
#ifndef WINE_TEB_DEFINED
#define WINE_TEB_DEFINED
typedef struct _TEB
{
    NT_TIB       Tib;            /* 12-  00 Thread information block */
    PVOID        EnvironmentPointer; /* 12-  1c EnvironmentPointer (win95: tib flags + win16 mutex count) */
    CLIENT_ID    ClientId;       /* -2-  20 Process and thread id (win95: debug context) */
    HQUEUE16     queue;          /* 1!-  28 Message queue (NT: DWORD ActiveRpcHandle)*/
    WORD         pad1;           /* --n  2a */
    PVOID        ThreadLocalStoragePointer; /* 1--  2c Pointer to TLS array */
    PEB         *Peb;            /* 12-  30 owning process PEB */
    DWORD        LastErrorValue; /* -2-  34 Last error code */
    DWORD        exit_code;      /* 1--  38 Termination status */
    WORD         teb_sel;        /* 1--  3c Selector to TEB */
    WORD         emu_sel;        /* 1-n  3e 80387 emulator selector */
    DWORD        unknown1;       /* --n  40 */
    DWORD        unknown2;       /* --n  44 */
    DWORD        unknown3;       /* --n  48 */
    int          thread_errno;   /* --3  4c Per-thread errno (was: ring0_thread) */
    int          thread_h_errno; /* --3  50 Per-thread h_errno (was: ptr to tdbx structure) */
    void        *stack_base;     /* 1-n  54 Stack base (unused) */
    void        *exit_stack;     /* 1-n  58 Exit stack */
    void        *emu_data;       /* --n  5c Related to 80387 emulation */
    DWORD        last_error;     /* 1--  60 Last error code */
    HANDLE       debug_cb;       /* 1-n  64 Debugger context block */
    DWORD        debug_thread;   /* 1-n  68 Thread debugging this one (?) */
    void        *pcontext;       /* 1-n  6c Thread register context */
    DWORD        cur_stack;      /* --3  70 Current stack (was: unknown) */
    DWORD        ThunkConnect;   /* 1-n  74 */
    DWORD        NegStackBase;   /* 1-n  78 */
    WORD         current_ss;     /* 1-n  7c Another 16-bit stack selector */
    WORD         pad2;           /* --n  7e */
    void        *ss_table;       /* --n  80 Pointer to info about 16-bit stack */
    WORD         stack_sel;      /* --3  84 16-bit stack selector */
    HTASK16      htask16;        /* --3  86 Win16 task handle */
    DWORD        pad4[15];       /* --n  88 */
    ULONG        CurrentLocale;  /* -2-  C4 */
    DWORD        pad5[48];       /* --n  C8 */
    DWORD        delta_priority; /* 1-n 188 Priority delta */
    DWORD        unknown4[7];    /* d-n 18c Unknown */
    void        *create_data;    /* d-n 1a8 Pointer to creation structure */
    DWORD        suspend_count;  /* d-n 1ac SuspendThread() counter */
    DWORD        unknown5[6];    /* --n 1b0 Unknown */
    DWORD        sys_count[4];   /* --3 1c8 Syslevel mutex entry counters */
    struct tagSYSLEVEL *sys_mutex[4];   /* --3 1d8 Syslevel mutex pointers */
    DWORD        unknown6[5];    /* --n 1e8 Unknown */

    /* The following are Wine-specific fields (NT: GDI stuff) */
    UINT         code_page;      /* --3 1fc Thread code page */
    DWORD        unused[2];      /* --3 200 Was server buffer */
    DWORD        gs_sel;         /* --3 208 %gs selector for this thread */
    int          request_fd;     /* --3 20c fd for sending server requests */
    int          reply_fd;       /* --3 210 fd for receiving server replies */
    int          wait_fd[2];     /* --3 214 fd for sleeping server requests */
    struct debug_info *debug_info;        /* --3 21c Info for debugstr functions */
    void        *pthread_data;   /* --3 220 Data for pthread emulation */
    struct async_private *pending_list;   /* --3 224 list of pending async operations */
    void        *driver_data;    /* --3 228 Graphics driver private data */
    DWORD        dpmi_vif;       /* --3 22c Protected mode virtual interrupt flag */
    DWORD        vm86_pending;   /* --3 230 Data for vm86 mode */
    void        *vm86_ptr;       /* --3 234 Data for vm86 mode */
    /* here is plenty space for wine specific fields (don't forget to change pad6!!) */

    /* the following are nt specific fields */
    DWORD        pad6[624];                  /* --n 238 */
    UNICODE_STRING StaticUnicodeString;      /* -2- bf8 used by advapi32 */
    WCHAR        StaticUnicodeBuffer[261];   /* -2- c00 used by advapi32 */
    PVOID        DeallocationStack;          /* -2- e0c Base of the stack */
    LPVOID       TlsSlots[64];               /* -2- e10 Thread local storage */
    LIST_ENTRY   TlsLinks;                   /* -2- f10 */
    DWORD        pad8[1];                    /* --n f18 */
    PVOID        ReservedForNtRpc;           /* -2- f1c used by rpcrt4 */
    DWORD        pad9[24];                   /* --n f20 */
    PVOID        ReservedForOle;             /* -2- f80 used by ole32 (IErrorInfo*) */
    PVOID        pad10[4];                   /* --n f84 */
    PVOID        TlsExpansionSlots;          /* -2- f94 */
} TEB;
#endif /* WINE_TEB_DEFINED */


/* The thread information for 16-bit threads */
/* NtCurrentTeb()->SubSystemTib points to this */
typedef struct
{
    void           *unknown;    /* 00 unknown */
    UNICODE_STRING *exe_name;   /* 04 exe module name */

    /* the following fields do not exist under Windows */
    UNICODE_STRING  exe_str;    /* exe name string pointed to by exe_name */
    CURDIR          curdir;     /* current directory */
    WCHAR           curdir_buffer[MAX_PATH];
} WIN16_SUBSYSTEM_TIB;

/* scheduler/thread.c */
extern TEB *THREAD_InitStack( TEB *teb, DWORD stack_size );

#endif  /* __WINE_THREAD_H */
