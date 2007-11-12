/* $Id$
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
#endif /* __cplusplus */

#define NDEBUG

#ifndef PFD_GENERIC_ACCELERATED
# define PFD_GENERIC_ACCELERATED 0x00001000
#endif

#define OPENGL_DRIVERS_SUBKEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers"

/* gl function list */
#include "glfuncs.h"

/* ICD index list/types */
#include "icdtable.h"

/* debug flags */
#if !defined(NDEBUG)
# define DEBUG_OPENGL32
/* enable breakpoints */
/*# define DEBUG_OPENGL32_BRKPTS*/
/* dumps the list of (un)supported glXXX functions when an ICD is loaded. */
# define DEBUG_OPENGL32_ICD_EXPORTS
/* prints much information about whats going on */
# define DEBUG_OPENGL32_TRACE
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
# endif
#endif

#ifndef DBGPRINT
# define DBGPRINT( ... ) do {} while (0)
#endif

#ifdef DEBUG_OPENGL32_BRKPTS
# if defined(__GNUC__)
#  define DBGBREAK() __asm__( "int $3" );
# elif defined(_MSC_VER)
#  define DBGBREAK() __asm { int 3 }
# else
#  error Unsupported compiler!
# endif
#else
# define DBGBREAK() do {} while (0)
#endif

#ifdef DEBUG_OPENGL32_TRACE
# define DBGTRACE( args... ) DBGPRINT( args )
#else
# define DBGTRACE( ... ) do {} while (0)
#endif

/* function/data attributes */
#define EXPORT __declspec(dllexport)
#ifdef _MSC_VER
#  define NAKED __declspec(naked)
#  define SHARED
#  ifndef STDCALL
#    define STDCALL __stdcall
#  endif /* STDCALL */
#else /* GCC */
#  define NAKED __attribute__((naked))
#  define SHARED __attribute__((section("shared"), shared))
#endif

#ifdef APIENTRY
#undef APIENTRY
#endif /* APIENTRY */
#define APIENTRY EXPORT __stdcall

/* gl function list */
#include "glfuncs.h"

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
typedef DWORD WINAPI (*SetContextCallBack)( const ICDTable * );

/* OpenGL ICD data */
typedef struct tagGLDRIVERDATA
{
	HMODULE handle;                 /*!< DLL handle */
	UINT    refcount;               /*!< Number of references to this ICD */
	WCHAR   driver_name[256];       /*!< Name of ICD driver */

	WCHAR   dll[256];               /*!< Dll filename from registry */
	DWORD   version;                /*!< Version value from registry */
	DWORD   driver_version;         /*!< DriverVersion value from registry */
	DWORD   flags;                  /*!< Flags value from registry */

	BOOL      WINAPI (*DrvCopyContext)( HGLRC, HGLRC, UINT );
	HGLRC     WINAPI (*DrvCreateContext)( HDC );
	HGLRC     WINAPI (*DrvCreateLayerContext)( HDC, int );
	BOOL      WINAPI (*DrvDeleteContext)( HGLRC );
	BOOL      WINAPI (*DrvDescribeLayerPlane)( HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR );
	int       WINAPI (*DrvDescribePixelFormat)( IN HDC, IN int, IN UINT, OUT LPPIXELFORMATDESCRIPTOR );
	int       WINAPI (*DrvGetLayerPaletteEntries)( HDC, int, int, int, COLORREF * );
	PROC      WINAPI (*DrvGetProcAddress)( LPCSTR lpProcName );
	void      WINAPI (*DrvReleaseContext)( HGLRC hglrc ); /* maybe returns BOOL? */
	BOOL      WINAPI (*DrvRealizeLayerPalette)( HDC, int, BOOL );
	PICDTable WINAPI (*DrvSetContext)( HDC hdc, HGLRC hglrc, SetContextCallBack callback );
	int       WINAPI (*DrvSetLayerPaletteEntries)( HDC, int, int, int, CONST COLORREF * );
	BOOL      WINAPI (*DrvSetPixelFormat)( IN HDC, IN int, const PIXELFORMATDESCRIPTOR * );
	BOOL      WINAPI (*DrvShareLists)( HGLRC, HGLRC );
	BOOL      WINAPI (*DrvSwapBuffers)( HDC );
	BOOL      WINAPI (*DrvSwapLayerBuffers)( HDC, UINT );
	BOOL      WINAPI (*DrvValidateVersion)( DWORD );

	struct tagGLDRIVERDATA *next;   /* next ICD -- linked list */
} GLDRIVERDATA;

/* Our private OpenGL context (stored in TLS) */
typedef struct tagGLRC
{
	GLDRIVERDATA *icd;  /*!< driver used for this context */
	HDC     hdc;        /*!< DC handle */
	BOOL    is_current; /*!< Wether this context is current for some DC */
	DWORD   thread_id;  /*!< Thread holding this context */

	HGLRC   hglrc;      /*!< GLRC from DrvCreateContext (ICD internal) */

	struct tagGLRC *next; /* linked list */
} GLRC;

/* OpenGL private device context data */
typedef struct tagGLDCDATA
{
	HDC hdc;           /*!< Device context handle for which this data is */
	GLDRIVERDATA *icd; /*!< Driver used for this DC */
	int pixel_format;  /*!< Selected pixel format */

	struct tagGLDCDATA *next; /* linked list */
} GLDCDATA;


/* Process data */
typedef struct tagGLPROCESSDATA
{
	GLDRIVERDATA *driver_list;  /*!< List of loaded drivers */
	HANDLE        driver_mutex; /*!< Mutex to protect driver list */
	GLRC         *glrc_list;    /*!< List of GL rendering contexts */
	HANDLE        glrc_mutex;   /*!< Mutex to protect glrc list */
	GLDCDATA     *dcdata_list;  /*!< List of GL private DC data */
	HANDLE        dcdata_mutex; /*!< Mutex to protect glrc list */
} GLPROCESSDATA;

/* TLS data */
typedef struct tagGLTHREADDATA
{
	GLRC   *glrc;      /*!< current GL rendering context */
} GLTHREADDATA;

extern DWORD OPENGL32_tls;
extern GLPROCESSDATA OPENGL32_processdata;
#define OPENGL32_threaddata ((GLTHREADDATA *)TlsGetValue( OPENGL32_tls ))

/* function prototypes */
GLDRIVERDATA *OPENGL32_LoadICD( LPCWSTR driver );
BOOL OPENGL32_UnloadICD( GLDRIVERDATA *icd );
BOOL APIENTRY rosglMakeCurrent( HDC hdc, HGLRC hglrc );

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

#ifdef OPENGL32_GL_FUNC_PROTOTYPES

#define X(func,ret,typeargs,args,icdidx,tebidx,stack) EXPORT ret STDCALL func typeargs;
GLFUNCS_MACRO
#undef X

#endif /* OPENGL32_GL_FUNC_PROTOTYPES */

#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */

#endif /* OPENGL32_PRIVATE_H */

/* EOF */
