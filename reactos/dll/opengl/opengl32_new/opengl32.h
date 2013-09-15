/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/opengl.h
 * PURPOSE:              OpenGL32 lib, general header
 */

#pragma once

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winddi.h>
#include <GL/gl.h>

#include "icd.h"

struct wgl_context
{
    DWORD magic;
    volatile LONG lock;
    
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

#ifdef OPENGL32_USE_TLS
extern DWORD OglTlsIndex;

struct Opengl32_ThreadData
{
    const GLCLTPROCTABLE* ProcTable;
    HGLRC hglrc;
    HDC hdc;
    struct wgl_dc_data* dc_data;
};

static inline
HGLRC
IntGetCurrentRC(void)
{
    struct Opengl32_ThreadData* data = TlsGetValue(OglTlsIndex);
    return data->hglrc;
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
    return &data->ProcTable->glDispatchTable;
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
BOOL sw_SetPixelFormat(struct wgl_dc_data*, INT format);
DHGLRC sw_CreateContext(struct wgl_dc_data*);
BOOL sw_DeleteContext(DHGLRC dhglrc);
const GLCLTPROCTABLE* sw_SetContext(struct wgl_dc_data* dc_data, DHGLRC dhglrc);
void sw_ReleaseContext(DHGLRC hglrc);
PROC sw_GetProcAddress(LPCSTR name);
BOOL sw_CopyContext(DHGLRC dhglrcSrc, DHGLRC dhglrcDst, UINT mask);
BOOL sw_ShareLists(DHGLRC dhglrcSrc, DHGLRC dhglrcDst);
BOOL sw_SwapBuffers(HDC hdc, struct wgl_dc_data* dc_data);
