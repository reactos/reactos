/* $Id: opengl32.h,v 1.11 2004/02/06 18:17:18 royce Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/opengl32.h
 * PURPOSE:              OpenGL32 lib
 * PROGRAMMER:           Royce Mitchell III, Anich Gregor (blight)
 * UPDATE HISTORY:
 *                       Feb 1, 2004: Created
 */

#ifndef OPENGL32_PRIVATE_H
#define OPENGL32_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/* gl function list */
#include "glfuncs.h"

/* ICD index list/types */
#include "icdtable.h"

/* debug flags */
#if !defined(NDEBUG)
# define DEBUG_OPENGL32
# define DEBUG_OPENGL32_BRKPTS		/* enable breakpoints */
# define DEBUG_OPENGL32_ICD_EXPORTS	/* dumps the list of (un)supported glXXX
                                       functions when an ICD is loaded. */
#endif /* !NDEBUG */

/* debug macros */
#ifdef _MSC_VER
inline void DBGPRINT ( ... ) {}
#else
# ifdef DEBUG_OPENGL32
ULONG DbgPrint(PCH Format,...);
#  include <debug.h>
#  define DBGPRINT( fmt, args... ) \
          DPRINT( "OpenGL32.DLL: %s: "fmt"\n", __FUNCTION__, ##args )
# else
#  define DBGPRINT( ... ) do {} while (0)
# endif
#endif

#ifdef DEBUG_OPENGL32_BRKPTS
# if defined(__GNUC__)
#  define DBGBREAK() __asm__( "int $3" );
# elif defined(_MSC_VER)
#  define DBGBREAK() __asm { int 3 }
# else
#  error Unsupported compiler!
# endif
#endif

/* function/data attributes */
#define EXPORT __declspec(dllexport)
#ifdef _MSC_VER
#  define NAKED __declspec(naked)
#  define SHARED
#  ifndef STDCALL
#    define STDCALL __stdcall
#  endif/*STDCALL*/
#else /* GCC */
#  define NAKED __attribute__((naked))
#  define SHARED __attribute__((section("shared"), shared))
#endif

#ifdef APIENTRY
#undef APIENTRY
#endif//APIENTRY
#define APIENTRY EXPORT __stdcall

/* gl function list */
#include "glfuncs.h"

/* table indices for funcnames and function pointers */
#if 0
enum glfunc_indices
{
	GLIDX_INVALID = -1,
#define X(func, ret, typeargs, args) GLIDX_##func,
	GLFUNCS_MACRO
#undef X
	GLIDX_COUNT
};

/* function name table */
extern const char* OPENGL32_funcnames[GLIDX_COUNT];
#endif//0

/* GL data types - x86 typedefs */
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef unsigned short GLhalf;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;

/* Called by the driver to set the dispatch table */
typedef DWORD (CALLBACK * SetContextCallBack)( const ICDTable * );

/* OpenGL ICD data */
typedef struct tagGLDRIVERDATA
{
	HMODULE handle;                 /* DLL handle */
	UINT    refcount;               /* number of references to this ICD */
	WCHAR   driver_name[256];       /* name of display driver */

	WCHAR   dll[256];               /* Dll value from registry */
	DWORD   version;                /* Version value from registry */
	DWORD   driver_version;         /* DriverVersion value from registry */
	DWORD   flags;                  /* Flags value from registry */

	BOOL    (*DrvCopyContext)( HGLRC, HGLRC, UINT );
	HGLRC   (*DrvCreateContext)( HDC );
	HGLRC   (*DrvCreateLayerContext)( HDC, int );
	BOOL    (*DrvDeleteContext)( HGLRC );
	BOOL    (*DrvDescribeLayerPlane)( HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR );
	int     (*DrvDescribePixelFormat)( IN HDC, IN int, IN UINT, OUT LPPIXELFORMATDESCRIPTOR );
	int     (*DrvGetLayerPaletteEntries)( HDC, int, int, int, COLORREF * );
	FARPROC (*DrvGetProcAddress)( LPCSTR lpProcName );
	void    (*DrvReleaseContext)();
	BOOL    (*DrvRealizeLayerPalette)( HDC, int, BOOL );
	int     (*DrvSetContext)( HDC hdc, HGLRC hglrc, SetContextCallBack callback );
	int     (*DrvSetLayerPaletteEntries)( HDC, int, int, int, CONST COLORREF * );
	BOOL    (*DrvSetPixelFormat)( IN HDC, IN int, IN CONST PIXELFORMATDESCRIPTOR * );
	BOOL    (*DrvShareLists)( HGLRC, HGLRC );
	BOOL    (*DrvSwapBuffers)( HDC );
	BOOL    (*DrvSwapLayerBuffers)( HDC, UINT );
	BOOL    (*DrvValidateVersion)( DWORD );

	struct tagGLDRIVERDATA *next;   /* next ICD -- linked list */
} GLDRIVERDATA;

/* Out private OpenGL context (saved in TLS) */
typedef struct tagGLRC
{
	GLDRIVERDATA *icd;  /* driver used for this context */
	INT     iFormat;    /* current pixel format index - ? */
	HDC     hdc;        /* DC handle */
	BOOL    is_current; /* wether this context is current for some DC */
	DWORD   thread_id;  /* thread holding this context */

	HGLRC   hglrc;      /* GLRC from DrvCreateContext */

	struct tagGLRC *next; /* linked list */
} GLRC;

/* Process data */
typedef struct tagGLPROCESSDATA
{
	GLDRIVERDATA *driver_list;  /* list of loaded drivers */
	HANDLE        driver_mutex; /* mutex to protect driver list */
	GLRC  *glrc_list;           /* list of GL rendering contexts */
	HANDLE glrc_mutex;          /* mutex to protect glrc list */
} GLPROCESSDATA;

/* TLS data */
typedef struct tagGLTHREADDATA
{
	GLRC   *glrc;      /* current GL rendering context */
} GLTHREADDATA;

extern DWORD OPENGL32_tls;
extern GLPROCESSDATA OPENGL32_processdata;
#define OPENGL32_threaddata ((GLTHREADDATA *)TlsGetValue( OPENGL32_tls ))

/* function prototypes */
GLDRIVERDATA *OPENGL32_LoadICDForHDC( HDC hdc );
GLDRIVERDATA *OPENGL32_LoadICD( LPCWSTR driver );
BOOL OPENGL32_UnloadICD( GLDRIVERDATA *icd );
DWORD OPENGL32_RegEnumDrivers( DWORD idx, LPWSTR name, LPDWORD cName );
DWORD OPENGL32_RegGetDriverInfo( LPCWSTR driver, GLDRIVERDATA *icd );

/* empty gl functions from gl.c */
int STDCALL glEmptyFunc0();
int STDCALL glEmptyFunc4( long );
int STDCALL glEmptyFunc8( long, long );
int STDCALL glEmptyFunc12( long, long, long );
int STDCALL glEmptyFunc16( long, long, long, long );
int STDCALL glEmptyFunc20( long, long, long, long, long );
int STDCALL glEmptyFunc24( long, long, long, long, long, long );
int STDCALL glEmptyFunc28( long, long, long, long, long, long, long );
int STDCALL glEmptyFunc32( long, long, long, long, long, long, long, long );
int STDCALL glEmptyFunc36( long, long, long, long, long, long, long, long,
                           long );
int STDCALL glEmptyFunc40( long, long, long, long, long, long, long, long,
                           long, long );
int STDCALL glEmptyFunc44( long, long, long, long, long, long, long, long,
                           long, long, long );
int STDCALL glEmptyFunc48( long, long, long, long, long, long, long, long,
                           long, long, long, long );
int STDCALL glEmptyFunc52( long, long, long, long, long, long, long, long,
                           long, long, long, long, long );
int STDCALL glEmptyFunc56( long, long, long, long, long, long, long, long,
                           long, long, long, long, long, long );

#define X(func,ret,typeargs,args,icdidx,tebidx,stack) EXPORT ret STDCALL func typeargs;
GLFUNCS_MACRO
#undef X

#ifdef __cplusplus
}; // extern "C"
#endif//__cplusplus

#endif//OPENGL32_PRIVATE_H

/* EOF */
