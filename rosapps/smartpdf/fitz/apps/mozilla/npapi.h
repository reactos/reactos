/* -*- Mode: C; tab-width: 4; -*- */
/*
 *  npapi.h $Revision: 1.88 $
 *  Netscape client plug-in API spec
 */

#ifndef _NPAPI_H_
#define _NPAPI_H_

#include "jri.h"                /* Java Runtime Interface */

#ifdef _WINDOWS
# ifndef XP_WIN
#   define XP_WIN 1
# endif /* XP_WIN */
#endif /* _WINDOWS */

#ifdef __MWERKS__
# define _declspec __declspec
# ifdef macintosh
#   ifndef XP_MAC
#     define XP_MAC 1
#   endif /* XP_MAC */
# endif /* macintosh */
# ifdef __INTEL__
#   undef NULL
#   ifndef XP_WIN
#     define XP_WIN 1
#   endif /* __INTEL__ */
# endif /* XP_PC */
#endif /* __MWERKS__ */

#ifdef XP_MAC
  #include <Quickdraw.h>
  #include <Events.h>
#endif
              
#ifdef XP_UNIX
  #include <X11/Xlib.h>
  #include <X11/Xutil.h>
#endif          


/*----------------------------------------------------------------------*/
/*         Plugin Version Constants       */
/*----------------------------------------------------------------------*/

#define NP_VERSION_MAJOR 0
#define NP_VERSION_MINOR 11



/*----------------------------------------------------------------------*/
/*         Definition of Basic Types        */
/*----------------------------------------------------------------------*/

#ifndef _UINT16
typedef unsigned short uint16;
#endif
#ifndef _UINT32
#if defined(__alpha)
typedef unsigned int uint32;
#else /* __alpha */
typedef unsigned long uint32;
#endif /* __alpha */
#endif
#ifndef _INT16
typedef short int16;
#endif
#ifndef _INT32
#if defined(__alpha)
typedef int int32;
#else /* __alpha */
typedef long int32;
#endif /* __alpha */
#endif

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef NULL
#define NULL (0L)
#endif

typedef unsigned char NPBool;
typedef int16     NPError;
typedef int16     NPReason;
typedef char*     NPMIMEType;



/*----------------------------------------------------------------------*/
/*         Structures and definitions       */
/*----------------------------------------------------------------------*/

#ifdef XP_MAC
#pragma options align=mac68k
#endif

/*
 *  NPP is a plug-in's opaque instance handle
 */
typedef struct _NPP
{
    void* pdata;      /* plug-in private data */
    void* ndata;      /* netscape private data */
} NPP_t;

typedef NPP_t*  NPP;


typedef struct _NPStream
{
    void*   pdata;    /* plug-in private data */
    void*   ndata;    /* netscape private data */
    const char*   url;
    uint32    end;
    uint32    lastmodified;
    void*   notifyData;
} NPStream;


typedef struct _NPByteRange
{
    int32 offset;     /* negative offset means from the end */
    uint32  length;
    struct _NPByteRange* next;
} NPByteRange;


typedef struct _NPSavedData
{
    int32 len;
    void* buf;
} NPSavedData;


typedef struct _NPRect
{
    uint16  top;
    uint16  left;
    uint16  bottom;
    uint16  right;
} NPRect;


#ifdef XP_UNIX
/*
 * Unix specific structures and definitions
 */

/*
 * Callback Structures.
 *
 * These are used to pass additional platform specific information.
 */
enum {
  NP_SETWINDOW = 1,
  NP_PRINT
};

typedef struct
{
    int32   type;
} NPAnyCallbackStruct;

typedef struct
{
    int32     type;
    Display*    display;
    Visual*     visual;
    Colormap    colormap;
    unsigned int  depth;
} NPSetWindowCallbackStruct;

typedef struct
{
    int32     type;
    FILE*     fp;
} NPPrintCallbackStruct;

#endif /* XP_UNIX */

/*
 * List of variable names for which NPP_GetValue shall be implemented
 */
typedef enum {
  NPPVpluginNameString = 1,
  NPPVpluginDescriptionString,
  NPPVpluginWindowBool,
  NPPVpluginTransparentBool
} NPPVariable;

/*
 * List of variable names for which NPN_GetValue is implemented by Mozilla
 */
typedef enum {
  NPNVxDisplay = 1,
  NPNVxtAppContext,
    NPNVnetscapeWindow,
  NPNVjavascriptEnabledBool,
  NPNVasdEnabledBool,
  NPNVisOfflineBool
} NPNVariable;

/*
 * The type of a NPWindow - it specifies the type of the data structure
 * returned in the window field.
 */
typedef enum {
    NPWindowTypeWindow = 1,
    NPWindowTypeDrawable
} NPWindowType;

typedef struct _NPWindow
{
    void* window;   /* Platform specific window handle */
    int32 x;      /* Position of top left corner relative */
    int32 y;      /*  to a netscape page.         */
    uint32  width;    /* Maximum window size */
    uint32  height;
    NPRect  clipRect; /* Clipping rectangle in port coordinates */
            /* Used by MAC only.        */
#ifdef XP_UNIX
    void *  ws_info;  /* Platform-dependent additonal data */
#endif /* XP_UNIX */
    NPWindowType type;  /* Is this a window or a drawable? */
} NPWindow;


typedef struct _NPFullPrint
{
    NPBool  pluginPrinted;  /* Set TRUE if plugin handled fullscreen */
              /*  printing               */
    NPBool  printOne;   /* TRUE if plugin should print one copy  */
              /*  to default printer           */
    void* platformPrint;  /* Platform-specific printing info */
} NPFullPrint;

typedef struct _NPEmbedPrint
{
    NPWindow  window;
    void* platformPrint;  /* Platform-specific printing info */
} NPEmbedPrint;

typedef struct _NPPrint
{
    uint16  mode;           /* NP_FULL or NP_EMBED */
    union
    {
    NPFullPrint   fullPrint;    /* if mode is NP_FULL */
    NPEmbedPrint  embedPrint;   /* if mode is NP_EMBED */
    } print;
} NPPrint;

#ifdef XP_MAC
typedef EventRecord NPEvent;
#elif defined(XP_WIN)
typedef struct _NPEvent
{
    uint16   event;
    uint32   wParam;
    uint32   lParam;
} NPEvent;
#elif defined (XP_UNIX)
typedef XEvent NPEvent;
#else
typedef void*     NPEvent;
#endif /* XP_MAC */

#ifdef XP_MAC
typedef RgnHandle NPRegion;
#elif defined(XP_WIN)
typedef HRGN NPRegion;
#elif defined(XP_UNIX)
typedef Region NPRegion;
#else
typedef void *NPRegion;
#endif /* XP_MAC */

#ifdef XP_MAC
/*
 *  Mac-specific structures and definitions.
 */

typedef struct NP_Port
{
    CGrafPtr  port;   /* Grafport */
    int32   portx;    /* position inside the topmost window */
    int32   porty;
} NP_Port;

/*
 *  Non-standard event types that can be passed to HandleEvent
 */
#define getFocusEvent     (osEvt + 16)
#define loseFocusEvent      (osEvt + 17)
#define adjustCursorEvent   (osEvt + 18)

#endif /* XP_MAC */


/*
 * Values for mode passed to NPP_New:
 */
#define NP_EMBED    1
#define NP_FULL     2

/*
 * Values for stream type passed to NPP_NewStream:
 */
#define NP_NORMAL   1
#define NP_SEEK     2
#define NP_ASFILE   3
#define NP_ASFILEONLY   4

#define NP_MAXREADY (((unsigned)(~0)<<1)>>1)

#ifdef XP_MAC
#pragma options align=reset
#endif


/*----------------------------------------------------------------------*/
/*         Error and Reason Code definitions      */
/*----------------------------------------------------------------------*/

/*
 *  Values of type NPError:
 */
#define NPERR_BASE              0
#define NPERR_NO_ERROR            (NPERR_BASE + 0)
#define NPERR_GENERIC_ERROR         (NPERR_BASE + 1)
#define NPERR_INVALID_INSTANCE_ERROR    (NPERR_BASE + 2)
#define NPERR_INVALID_FUNCTABLE_ERROR   (NPERR_BASE + 3)
#define NPERR_MODULE_LOAD_FAILED_ERROR    (NPERR_BASE + 4)
#define NPERR_OUT_OF_MEMORY_ERROR     (NPERR_BASE + 5)
#define NPERR_INVALID_PLUGIN_ERROR      (NPERR_BASE + 6)
#define NPERR_INVALID_PLUGIN_DIR_ERROR    (NPERR_BASE + 7)
#define NPERR_INCOMPATIBLE_VERSION_ERROR  (NPERR_BASE + 8)
#define NPERR_INVALID_PARAM       (NPERR_BASE + 9)
#define NPERR_INVALID_URL         (NPERR_BASE + 10)
#define NPERR_FILE_NOT_FOUND        (NPERR_BASE + 11)
#define NPERR_NO_DATA           (NPERR_BASE + 12)
#define NPERR_STREAM_NOT_SEEKABLE     (NPERR_BASE + 13)

/*
 *  Values of type NPReason:
 */
#define NPRES_BASE        0
#define NPRES_DONE          (NPRES_BASE + 0)
#define NPRES_NETWORK_ERR     (NPRES_BASE + 1)
#define NPRES_USER_BREAK      (NPRES_BASE + 2)

/*
 *      Don't use these obsolete error codes any more.
 */
#define NP_NOERR  NP_NOERR_is_obsolete_use_NPERR_NO_ERROR
#define NP_EINVAL NP_EINVAL_is_obsolete_use_NPERR_GENERIC_ERROR
#define NP_EABORT NP_EABORT_is_obsolete_use_NPRES_USER_BREAK

/*
 * Version feature information
 */
#define NPVERS_HAS_STREAMOUTPUT   8
#define NPVERS_HAS_NOTIFICATION   9
#define NPVERS_HAS_LIVECONNECT    9
#define NPVERS_WIN16_HAS_LIVECONNECT  9
#define NPVERS_68K_HAS_LIVECONNECT  11
#define NPVERS_HAS_WINDOWLESS       11


/*----------------------------------------------------------------------*/
/*         Function Prototypes        */
/*----------------------------------------------------------------------*/

#if defined(_WINDOWS) && !defined(WIN32)
#define NP_LOADDS  _loadds
#else
#define NP_LOADDS
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * NPP_* functions are provided by the plugin and called by the navigator.
 */

#ifdef XP_UNIX
char*         NPP_GetMIMEDescription(void);
#endif /* XP_UNIX */

NPError    NPP_Initialize(void);
void       NPP_Shutdown(void);
NPError     NP_LOADDS NPP_New(NPMIMEType pluginType, NPP instance,
                uint16 mode, int16 argc, char* argn[],
                char* argv[], NPSavedData* saved);
NPError     NP_LOADDS NPP_Destroy(NPP instance, NPSavedData** save);
NPError     NP_LOADDS NPP_SetWindow(NPP instance, NPWindow* window);
NPError     NP_LOADDS NPP_NewStream(NPP instance, NPMIMEType type,
                    NPStream* stream, NPBool seekable,
                    uint16* stype);
NPError     NP_LOADDS NPP_DestroyStream(NPP instance, NPStream* stream,
                      NPReason reason);
int32     NP_LOADDS NPP_WriteReady(NPP instance, NPStream* stream);
int32     NP_LOADDS NPP_Write(NPP instance, NPStream* stream, int32 offset,
                  int32 len, void* buffer);
void      NP_LOADDS NPP_StreamAsFile(NPP instance, NPStream* stream,
                     const char* fname);
void      NP_LOADDS NPP_Print(NPP instance, NPPrint* platformPrint);
int16     NPP_HandleEvent(NPP instance, void* event);
void      NP_LOADDS NPP_URLNotify(NPP instance, const char* url,
                    NPReason reason, void* notifyData);
jref      NP_LOADDS     NPP_GetJavaClass(void);
NPError         NPP_GetValue(void *instance, NPPVariable variable,
                   void *value);
NPError         NPP_SetValue(void *instance, NPNVariable variable,
                   void *value);

/*
 * NPN_* functions are provided by the navigator and called by the plugin.
 */

void    NPN_Version(int* plugin_major, int* plugin_minor,
              int* netscape_major, int* netscape_minor);
NPError   NPN_GetURLNotify(NPP instance, const char* url,
                 const char* target, void* notifyData);
NPError   NPN_GetURL(NPP instance, const char* url,
               const char* target);
NPError   NPN_PostURLNotify(NPP instance, const char* url,
                  const char* target, uint32 len,
                  const char* buf, NPBool file,
                  void* notifyData);
NPError   NPN_PostURL(NPP instance, const char* url,
              const char* target, uint32 len,
              const char* buf, NPBool file);
NPError   NPN_RequestRead(NPStream* stream, NPByteRange* rangeList);
NPError   NPN_NewStream(NPP instance, NPMIMEType type,
                const char* target, NPStream** stream);
int32   NPN_Write(NPP instance, NPStream* stream, int32 len,
              void* buffer);
NPError     NPN_DestroyStream(NPP instance, NPStream* stream,
                  NPReason reason);
void    NPN_Status(NPP instance, const char* message);
const char* NPN_UserAgent(NPP instance);
void*   NPN_MemAlloc(uint32 size);
void    NPN_MemFree(void* ptr);
uint32    NPN_MemFlush(uint32 size);
void      NPN_ReloadPlugins(NPBool reloadPages);
JRIEnv*     NPN_GetJavaEnv(void);
jref      NPN_GetJavaPeer(NPP instance);
NPError     NPN_GetValue(NPP instance, NPNVariable variable,
               void *value);
NPError     NPN_SetValue(NPP instance, NPPVariable variable,
               void *value);
void    NPN_InvalidateRect(NPP instance, NPRect *invalidRect);
void    NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion);
void    NPN_ForceRedraw(NPP instance);

#ifdef __cplusplus
}  /* end extern "C" */
#endif

#endif /* _NPAPI_H_ */
