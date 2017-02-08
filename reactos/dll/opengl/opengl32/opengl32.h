/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/opengl.h
 * PURPOSE:              OpenGL32 lib, general header
 */

#ifndef _OPENGL32_PCH_
#define _OPENGL32_PCH_

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winddi.h>
#include <GL/gl.h>

#include <wine/debug.h>

#include "icd.h"

/* *$%$£^§! headers inclusion */
static __inline
BOOLEAN
RemoveEntryList(
    _In_ PLIST_ENTRY Entry)
{
    PLIST_ENTRY OldFlink;
    PLIST_ENTRY OldBlink;

    OldFlink = Entry->Flink;
    OldBlink = Entry->Blink;
    OldFlink->Blink = OldBlink;
    OldBlink->Flink = OldFlink;
    return (OldFlink == OldBlink);
}

static __inline
VOID
InsertTailList(
    _In_ PLIST_ENTRY ListHead,
    _In_ PLIST_ENTRY Entry
)
{
    PLIST_ENTRY OldBlink;
    OldBlink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = OldBlink;
    OldBlink->Flink = Entry;
    ListHead->Blink = Entry;
}


static __inline
VOID
InitializeListHead(
    _Inout_ PLIST_ENTRY ListHead
)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

extern LIST_ENTRY ContextListHead;

struct wgl_context
{
    DWORD magic;
    volatile LONG lock;

    LIST_ENTRY ListEntry;

    DHGLRC dhglrc;
    struct ICD_Data* icd_data;
    INT pixelformat;
    volatile LONG thread_id;
};

#define WGL_DC_OBJ_DC 0x1
struct wgl_dc_data
{
    /* Header */
    union
    {
        HWND hwnd;
        HDC hdc;
        HANDLE u;
    } owner;
    ULONG flags;
    
    /* Pixel format */
    INT pixelformat;
    
    /* ICD */
    struct ICD_Data* icd_data;
    INT nb_icd_formats;
    
    /* Software implementation */
    INT nb_sw_formats;
    void* sw_data;
    
    /* Linked list */
    struct wgl_dc_data* next;
};

/* Clean up functions */
void IntDeleteAllContexts(void);
void IntDeleteAllICDs(void);

#ifdef OPENGL32_USE_TLS
extern DWORD OglTlsIndex;

struct Opengl32_ThreadData
{
    const GLDISPATCHTABLE* glDispatchTable;
    HGLRC hglrc;
    HDC hdc;
    struct wgl_dc_data* dc_data;
    PVOID* icdData;
};
C_ASSERT(FIELD_OFFSET(struct Opengl32_ThreadData, glDispatchTable) == 0);

static inline
void
IntMakeCurrent(HGLRC hglrc, HDC hdc, struct wgl_dc_data* dc_data)
{
    struct Opengl32_ThreadData* thread_data = TlsGetValue(OglTlsIndex);

    thread_data->hglrc = hglrc;
    thread_data->hdc = hdc;
    thread_data->dc_data = dc_data;
}

static inline
HGLRC
IntGetCurrentRC(void)
{
    struct Opengl32_ThreadData* data = TlsGetValue(OglTlsIndex);
    return data ? data->hglrc : NULL;
}

static inline
DHGLRC
IntGetCurrentDHGLRC(void)
{
    struct wgl_context* ctx = (struct wgl_context*)IntGetCurrentRC();
    if(!ctx) return NULL;
    return ctx->dhglrc;
}

static inline
HDC
IntGetCurrentDC(void)
{
    struct Opengl32_ThreadData* data = TlsGetValue(OglTlsIndex);
    return data->hdc;
}

static inline
struct wgl_dc_data*
IntGetCurrentDcData(void)
{
    struct Opengl32_ThreadData* data = TlsGetValue(OglTlsIndex);
    return data->dc_data;
}

static inline
const GLDISPATCHTABLE *
IntGetCurrentDispatchTable(void)
{
    struct Opengl32_ThreadData* data = TlsGetValue(OglTlsIndex);
    return data->glDispatchTable;
}

static inline
void
IntSetCurrentDispatchTable(const GLDISPATCHTABLE* table)
{
    struct Opengl32_ThreadData* data = TlsGetValue(OglTlsIndex);
    data->glDispatchTable = table;
}

static inline
void
IntSetCurrentICDPrivate(void* value)
{
    struct Opengl32_ThreadData* data = TlsGetValue(OglTlsIndex);
    data->icdData = value;
}

static inline
void*
IntGetCurrentICDPrivate(void)
{
    struct Opengl32_ThreadData* data = TlsGetValue(OglTlsIndex);
    return data->icdData;
}


#else
static inline
const GLDISPATCHTABLE*
IntGetCurrentDispatchTable(void)
{
    return (GLDISPATCHTABLE*)NtCurrentTeb()->glTable;
}
#endif // defined(OPENGL32_USE_TLS)

/* Software implementation functions */
INT sw_DescribePixelFormat(HDC hdc, INT format, UINT size, PIXELFORMATDESCRIPTOR* descr);
BOOL sw_SetPixelFormat(HDC hdc, struct wgl_dc_data*, INT format);
DHGLRC sw_CreateContext(struct wgl_dc_data*);
BOOL sw_DeleteContext(DHGLRC dhglrc);
BOOL sw_SetContext(struct wgl_dc_data* dc_data, DHGLRC dhglrc);
void sw_ReleaseContext(DHGLRC hglrc);
PROC sw_GetProcAddress(LPCSTR name);
BOOL sw_CopyContext(DHGLRC dhglrcSrc, DHGLRC dhglrcDst, UINT mask);
BOOL sw_ShareLists(DHGLRC dhglrcSrc, DHGLRC dhglrcDst);
BOOL sw_SwapBuffers(HDC hdc, struct wgl_dc_data* dc_data);

#endif /* _OPENGL32_PCH_ */
