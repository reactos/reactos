/*
  A tool to stress-test poppler rendering and measure rendering times for
  very simplistic performance measuring.

  TODO:
   * print more info about document like e.g. enumarate images,
     streams, compression, encryption, password-protection. Each should have
     a command-line arguments to turn it on/off
   * never over-write file given as -out argument (optionally, provide -force
     option to force writing the -out file). It's way too easy too lose results
     of a previous run.
*/

#include <assert.h>
#include <config.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifdef WIN32
#include <windows.h>
#else
#define stricmp strcasecmp
#endif

#include "ErrorCodes.h"
#include "GooString.h"
#include "GooList.h"
#include "GlobalParams.h"
#include "SplashBitmap.h"
#include "Object.h" /* must be included before SplashOutputDev.h because of sloppiness in SplashOutputDev.h */
#include "SplashOutputDev.h"
#include "TextOutputDev.h"
#include "PDFDoc.h"
#include "SecurityHandler.h"
#include "Link.h"

#include "BaseUtils.h"

extern void PreviewBitmap_Init(void);
extern void PreviewBitmap(SplashBitmap *);
extern void PreviewBitmap_Destroy(void);

#define PDF_FILE_DPI 72

#define MAX_FILENAME_SIZE 1024

struct FindFileState {
    char path[MAX_FILENAME_SIZE];
    char dirpath[MAX_FILENAME_SIZE]; /* current dir path */
    char pattern[MAX_FILENAME_SIZE]; /* search pattern */
    const char *bufptr;
#ifdef WIN32
    WIN32_FIND_DATA fileinfo;
    HANDLE dir;
#else
    DIR *dir;
#endif
};

#ifdef WIN32
#include <windows.h>
#include <sys/timeb.h>
#include <direct.h>

__inline char *getcwd(char *buffer, int maxlen)
{
    return _getcwd(buffer, maxlen);
}

int fnmatch(const char *pattern, const char *string, int flags)
{
    int prefix_len;
    const char *star_pos = strchr(pattern, '*');
    if (!star_pos)
        return strcmp(pattern, string) != 0;

    prefix_len = (int)(star_pos-pattern);
    if (0 == prefix_len)
        return 0;

    if (0 == _strnicmp(pattern, string, prefix_len))
        return 0;

    return 1;
}

#else
#include <fnmatch.h>
#endif

#ifdef WIN32
/* on windows to query dirs we need foo\* to get files in this directory.
    foo\ always fails and foo will return just info about foo directory,
    not files in this directory */
static void win_correct_path_for_FindFirstFile(char *path, int path_max_len)
{
    int path_len = strlen(path);
    if (path_len >= path_max_len-4)
        return;
    if (DIR_SEP_CHAR != path[path_len])
        path[path_len++] = DIR_SEP_CHAR;
    path[path_len++] = '*';
    path[path_len] = 0;
}
#endif

FindFileState *find_file_open(const char *path, const char *pattern)
{
    FindFileState *s;

    s = (FindFileState*)malloc(sizeof(FindFileState));
    if (!s)
        return NULL;
    strcpy_s(s->path, sizeof(s->path), path);
    strcpy_s(s->dirpath, sizeof(s->path), path);
#ifdef WIN32
    win_correct_path_for_FindFirstFile(s->path, sizeof(s->path));
#endif
    strcpy_s(s->pattern, sizeof(s->pattern), pattern);
    s->bufptr = s->path;
#ifdef WIN32
    s->dir = INVALID_HANDLE_VALUE;
#else
    s->dir = NULL;
#endif
    return s;
}

#if 0 /* re-enable if we #define USE_OWN_GET_AUTH_DATA */
void *StandardSecurityHandler::getAuthData()
{
    return NULL;
}
#endif

/* strcat and truncate. */
char *pstrcat(char *buf, int buf_size, const char *s)
{
    int len;
    len = strlen(buf);
    if (len < buf_size)
        strcpy_s(buf + len, buf_size - len, s);
    return buf;
}

char *makepath(char *buf, int buf_size, const char *path,
               const char *filename)
{
    int len;

    strcpy_s(buf, buf_size, path);
    len = strlen(path);
    if (len > 0 && path[len - 1] != DIR_SEP_CHAR && len + 1 < buf_size) {
        buf[len++] = DIR_SEP_CHAR;
        buf[len] = '\0';
    }
    return pstrcat(buf, buf_size, filename);
}

#ifdef WIN32
static int skip_matching_file(const char *filename)
{
    if (0 == strcmp(".", filename))
        return 1;
    if (0 == strcmp("..", filename))
        return 1;
    return 0;
}
#endif

int find_file_next(FindFileState *s, char *filename, int filename_size_max)
{
#ifdef WIN32
    int    fFound;
    if (INVALID_HANDLE_VALUE == s->dir) {
        s->dir = FindFirstFile(s->path, &(s->fileinfo));
        if (INVALID_HANDLE_VALUE == s->dir)
            return -1;
        goto CheckFile;
    }

    while (1) {
        fFound = FindNextFile(s->dir, &(s->fileinfo));
        if (!fFound)
            return -1;
CheckFile:
        if (skip_matching_file(s->fileinfo.cFileName))
            continue;
        if (0 == fnmatch(s->pattern, s->fileinfo.cFileName, 0) ) {
            makepath(filename, filename_size_max, s->dirpath, s->fileinfo.cFileName);
            return 0;
        }
    }
#else
    struct dirent *dirent;
    const char *p;
    char *q;

    if (s->dir == NULL)
        goto redo;

    for (;;) {
        dirent = readdir(s->dir);
        if (dirent == NULL) {
        redo:
            if (s->dir) {
                closedir(s->dir);
                s->dir = NULL;
            }
            p = s->bufptr;
            if (*p == '\0')
                return -1;
            /* CG: get_str(&p, s->dirpath, sizeof(s->dirpath), ":") */
            q = s->dirpath;
            while (*p != ':' && *p != '\0') {
                if ((q - s->dirpath) < (int)sizeof(s->dirpath) - 1)
                    *q++ = *p;
                p++;
            }
            *q = '\0';
            if (*p == ':')
                p++;
            s->bufptr = p;
            s->dir = opendir(s->dirpath);
            if (!s->dir)
                goto redo;
        } else {
            if (fnmatch(s->pattern, dirent->d_name, 0) == 0) {
                makepath(filename, filename_size_max,
                         s->dirpath, dirent->d_name);
                return 0;
            }
        }
    }
#endif
}

void find_file_close(FindFileState *s)
{
#ifdef WIN32
    if (INVALID_HANDLE_VALUE != s->dir)
       FindClose(s->dir);
#else
    if (s->dir)
        closedir(s->dir);
#endif
    free(s);
}

typedef struct StrList {
    struct StrList *next;
    char *          str;
} StrList;

/* List of all command-line arguments that are not switches.
   We assume those are:
     - names of PDF files
     - names of a file with a list of PDF files
     - names of directories with PDF files
*/
static StrList *gArgsListRoot = NULL;

/* Names of all command-line switches we recognize */
#define TIMINGS_ARG         "-timings"
#define RESOLUTION_ARG      "-resolution"
#define RECURSIVE_ARG       "-recursive"
#define OUT_ARG             "-out"
#define PREVIEW_ARG         "-preview"
#define SLOW_PREVIEW_ARG    "-slowpreview"
#define LOAD_ONLY_ARG       "-loadonly"
#define PAGE_ARG            "-page"
#define DUMP_LINKS_ARG      "-dump-links"
#define TEXT_ARG            "-text"

/* Should we record timings? True if -timings command-line argument was given. */
static BOOL gfTimings = FALSE;

/* If true, we use render each page at resolution 'gResolutionX'/'gResolutionY'.
   If false, we render each page at its native resolution.
   True if -resolution NxM command-line argument was given. */
static BOOL gfForceResolution = FALSE;
static int  gResolutionX = 0;
static int  gResolutionY = 0;
/* If NULL, we output the log info to stdout. If not NULL, should be a name
   of the file to which we output log info.
   Controled by -out command-line argument. */
static char *   gOutFileName = NULL;
/* FILE * correspondig to gOutFileName or stdout if gOutFileName is NULL or
   was invalid name */
static FILE *   gOutFile = NULL;
/* FILE * correspondig to gOutFileName or stderr if gOutFileName is NULL or
   was invalid name */
static FILE *   gErrFile = NULL;

/* If True and a directory is given as a command-line argument, we'll process
   pdf files in sub-directories as well.
   Controlled by -recursive command-line argument */
static BOOL gfRecursive = FALSE;

/* If true, preview rendered image. To make sure that they're being rendered correctly. */
static BOOL gfPreview = FALSE;

/* 1 second (1000 milliseconds) */
#define SLOW_PREVIEW_TIME 1000

/* If true, preview rendered image in a slow mode i.e. delay displaying for
   SLOW_PREVIEW_TIME. This is so that a human has enough time to see if the
   PDF renders ok. In release mode on fast processor pages take only ~100-200 ms
   to render and they go away too quickly to be inspected by a human. */
static int gfSlowPreview = FALSE;

/* If true, we only dump the text, not render */
static int gfTextOnly = FALSE;

#define PAGE_NO_NOT_GIVEN -1

/* If equals PAGE_NO_NOT_GIVEN, we're in default mode where we render all pages.
   If different, will only render this page */
static int  gPageNo = PAGE_NO_NOT_GIVEN;
/* If true, will only load the file, not render any pages. Mostly for
   profiling load time */
static BOOL gfLoadOnly = FALSE;

/* if TRUE, will dump information about links */
static BOOL gfDumpLinks = FALSE;

static SplashColor splashColRed;
static SplashColor splashColGreen;
static SplashColor splashColBlue;
static SplashColor splashColWhite;
static SplashColor splashColBlack;

#define SPLASH_COL_RED_PTR (SplashColorPtr)&(splashColRed[0])
#define SPLASH_COL_GREEN_PTR (SplashColorPtr)&(splashColGreen[0])
#define SPLASH_COL_BLUE_PTR (SplashColorPtr)&(splashColBlue[0])
#define SPLASH_COL_WHITE_PTR (SplashColorPtr)&(splashColWhite[0])
#define SPLASH_COL_BLACK_PTR (SplashColorPtr)&(splashColBlack[0])

static SplashColorPtr  gBgColor = SPLASH_COL_WHITE_PTR;
static SplashColorMode gSplashColorMode = splashModeBGR8;

int StrList_Len(StrList **root)
{
    int         len = 0;
    StrList *   cur;
    assert(root);
    if (!root)
        return 0;
    cur = *root;
    while (cur) {
        ++len;
        cur = cur->next;
    }
    return len;
}

int StrList_InsertAndOwn(StrList **root, char *txt)
{
    StrList *   el;
    assert(root && txt);
    if (!root || !txt)
        return FALSE;

    el = (StrList*)malloc(sizeof(StrList));
    if (!el)
        return FALSE;
    el->str = txt;
    el->next = *root;
    *root = el;
    return TRUE;
}

int StrList_Insert(StrList **root, char *txt)
{
    char *txtDup;

    assert(root && txt);
    if (!root || !txt)
        return FALSE;
    txtDup = Str_Dup(txt);
    if (!txtDup)
        return FALSE;

    if (!StrList_InsertAndOwn(root, txtDup)) {
        free((void*)txtDup);
        return FALSE;
    }
    return TRUE;
}

StrList* StrList_RemoveHead(StrList **root)
{
    StrList *tmp;
    assert(root);
    if (!root)
        return NULL;

    if (!*root)
        return NULL;
    tmp = *root;
    *root = tmp->next;
    tmp->next = NULL;
    return tmp;
}

void StrList_FreeElement(StrList *el)
{
    if (!el)
        return;
    free((void*)el->str);
    free((void*)el);
}

void StrList_Destroy(StrList **root)
{
    StrList *   cur;
    StrList *   next;

    if (!root)
        return;
    cur = *root;
    while (cur) {
        next = cur->next;
        StrList_FreeElement(cur);
        cur = next;
    }
    *root = NULL;
}

static void SplashColorSet(SplashColorPtr col, Guchar red, Guchar green, Guchar blue, Guchar alpha)
{
    switch (gSplashColorMode)
    {
        case splashModeBGR8:
            col[0] = blue;
            col[1] = green;
            col[2] = red;
            break;
        case splashModeRGB8:
            col[0] = red;
            col[1] = green;
            col[2] = blue;
            break;
        default:
            assert(0);
            break;
    }
}

static void ColorsInit(void)
{
    /* splash colors */
    SplashColorSet(SPLASH_COL_RED_PTR, 0xff, 0, 0, 0);
    SplashColorSet(SPLASH_COL_GREEN_PTR, 0, 0xff, 0, 0);
    SplashColorSet(SPLASH_COL_BLUE_PTR, 0, 0, 0xff, 0);
    SplashColorSet(SPLASH_COL_BLACK_PTR, 0, 0, 0, 0);
    SplashColorSet(SPLASH_COL_WHITE_PTR, 0xff, 0xff, 0xff, 0);
}

#ifndef WIN32
void OutputDebugString(const char *txt)
{
    /* do nothing */
}
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#endif

void CDECL error(int pos, char *msg, ...) {
    va_list args;
    char        buf[4096], *p = buf;

    // NB: this can be called before the globalParams object is created
    if (globalParams && globalParams->getErrQuiet()) {
        return;
    }

    if (pos >= 0) {
        p += _snprintf(p, sizeof(buf)-1, "Error (%d): ", pos);
        *p   = '\0';
        OutputDebugString(p);
    } else {
        OutputDebugString("Error: ");
    }

    p = buf;
    va_start(args, msg);
    p += _vsnprintf(p, sizeof(buf) - 1, msg, args);
    while ( p > buf  &&  isspace(p[-1]) )
            *--p = '\0';
    *p++ = '\r';
    *p++ = '\n';
    *p   = '\0';
    OutputDebugString(buf);
    va_end(args);

    if (pos >= 0) {
        p += _snprintf(p, sizeof(buf)-1, "Error (%d): ", pos);
        *p   = '\0';
        OutputDebugString(buf);
        if (gErrFile)
            fprintf(gErrFile, buf);
    } else {
        OutputDebugString("Error: ");
        if (gErrFile)
            fprintf(gErrFile, "Error: ");
    }

    p = buf;
    va_start(args, msg);
    p += _vsnprintf(p, sizeof(buf) - 3, msg, args);
    while ( p > buf  &&  isspace(p[-1]) )
            *--p = '\0';
    *p++ = '\r';
    *p++ = '\n';
    *p   = '\0';
    OutputDebugString(buf);
    if (gErrFile)
        fprintf(gErrFile, buf);
    va_end(args);
}

void LogInfo(char *fmt, ...)
{
    va_list args;
    char        buf[4096], *p = buf;

    p = buf;
    va_start(args, fmt);
    p += _vsnprintf(p, sizeof(buf) - 1, fmt, args);
    *p   = '\0';
    fprintf(gOutFile, buf);
    va_end(args);
    fflush(gOutFile);
}

static void PrintUsageAndExit(int argc, char **argv)
{
    printf("Usage: pdftest [-preview] [-slowpreview] [-timings] [-text] [-resolution NxM] [-recursive] [-page N] [-out out.txt] pdf-files-to-process\n");
    for (int i=0; i < argc; i++) {
        printf("i=%d, '%s'\n", i, argv[i]);
    }
    exit(0);
}

/* milli-second timer */
#ifdef WIN32
typedef struct MsTimer {
    LARGE_INTEGER   start;
    LARGE_INTEGER   end;
} MsTimer;

void MsTimer_Start(MsTimer *timer)
{
    assert(timer);
    if (!timer)
        return;
    QueryPerformanceCounter(&timer->start);
}
void MsTimer_End(MsTimer *timer)
{
    assert(timer);
    if (!timer)
        return;
    QueryPerformanceCounter(&timer->end);
}

double MsTimer_GetTimeInMs(MsTimer *timer)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    double durationInSecs = (double)(timer->end.QuadPart-timer->start.QuadPart)/(double)freq.QuadPart;
    return durationInSecs * 1000.0;
}

#else
#include <sys/time.h>

typedef struct MsTimer {
    struct timeval    start;
    struct timeval    end;
} MsTimer;

void MsTimer_Start(MsTimer *timer)
{
    assert(timer);
    if (!timer)
        return;
    gettimeofday(&timer->start, NULL);
}

void MsTimer_End(MsTimer *timer)
{
    assert(timer);
    if (!timer)
        return;
    gettimeofday(&timer->end, NULL);
}

double MsTimer_GetTimeInMs(MsTimer *timer)
{
    double timeInMs;
    time_t seconds;
    int    usecs;

    assert(timer);
    if (!timer)
        return 0.0;
    /* TODO: this logic needs to be verified */
    seconds = timer->end.tv_sec - timer->start.tv_sec;
    usecs = timer->end.tv_usec - timer->start.tv_usec;
    if (usecs < 0) {
        --seconds;
        usecs += 1000000;
    }
    timeInMs = (double)seconds*(double)1000.0 + (double)usecs/(double)1000.0;
    return timeInMs;
}
#endif

void *StandardSecurityHandler::getAuthData()
{
    return NULL;
}

int ShowPreview(void)
{
    if (gfPreview || gfSlowPreview)
        return TRUE;
    return FALSE;
}

static Links *GetLinksForPage(PDFDoc *doc, int pageNo)
{
    Object obj;
    Catalog *catalog = doc->getCatalog();
    Page *page = catalog->getPage(pageNo);
    Links *links = new Links(page->getAnnots(&obj), catalog->getBaseURI());
    obj.free();
    return links;
}

static const char * GetLinkActionKindName(LinkActionKind kind) {
    switch (kind) {
        case (actionGoTo):
            return "actionGoTo";
        case actionGoToR:
            return "actionGoToR";
        case actionLaunch:
            return "actionLaunch";
        case actionURI:
            return "actionURI";
        case actionNamed:
            return "actionNamed";
        case actionMovie:
            return "actionMovie";
        case actionUnknown:
            return "actionUnknown";
        default:
            assert(0);
            return "unknown action";
    }
}

static void DumpLinks(PDFDoc *doc)
{
    Links *     links = NULL;
    int         pagesCount, linkCount;
    Link *      link;

    if (!doc)
        return;

    pagesCount = doc->getNumPages();
    for (int pageNo = 1; pageNo < pagesCount; ++pageNo) {
        delete links;
        links = GetLinksForPage(doc, pageNo);
        if (!links)
            continue;
        linkCount = links->getNumLinks();
        for (int i=0; i<linkCount; i++) {
            link = links->getLink(i);
            LinkAction *    action = link->getAction();
            LinkActionKind  actionKind = action->getKind();
            LogInfo("Link: page %d, type=%d (%s)\n", pageNo, actionKind, GetLinkActionKindName(actionKind));
#if 0 /* TODO: maybe print the uri (and more) ? */
            if (actionURI == actionKind) {
                LinkURI *   linkUri;
                linkUri = (LinkURI*)action;
                DBG_OUT("   uri=%s\n", linkUri->getURI()->getCString());
            }
#endif
        }
    }
    delete links;
}

static void RenderPdfFileAsText(const char *fileName)
{
    MsTimer             msTimer;
    double              timeInMs;
    int                 pageCount;
    int                 rotate;
    GBool               useMediaBox;
    GBool               crop;
    GBool               doLinks;
    GooString *         fileNameStr = NULL;
    PDFDoc *            pdfDoc = NULL;
    TextOutputDev *     textOut = NULL;
    GooString *         txt = NULL;

    assert(fileName);
    if (!fileName)
        return;

    LogInfo("started: %s\n", fileName);

    textOut = new TextOutputDev(NULL, gTrue, gFalse, gFalse);
    if (!textOut->isOk()) {
        goto Exit;
    }

    MsTimer_Start(&msTimer);
    /* note: don't delete fileNameStr since PDFDoc takes ownership and deletes them itself */
    fileNameStr = new GooString(fileName);
    if (!fileNameStr)
        goto Exit;

    pdfDoc = new PDFDoc(fileNameStr, NULL, NULL, NULL);
    if (!pdfDoc->isOk()) {
        error(-1, "renderPdfFile(): failed to open PDF file %s\n", fileName);
        goto Exit;
    }

    MsTimer_End(&msTimer);
    timeInMs = MsTimer_GetTimeInMs(&msTimer);
    LogInfo("load: %.2f ms\n", timeInMs);

    pageCount = pdfDoc->getNumPages();
    LogInfo("page count: %d\n", pageCount);

    for (int curPage = 1; curPage <= pageCount; curPage++) {
        if ((gPageNo != PAGE_NO_NOT_GIVEN) && (gPageNo != curPage))
            continue;

        MsTimer_Start(&msTimer);
        rotate = 0;
        useMediaBox = gFalse;
        crop = gTrue;
        doLinks = gFalse;
        pdfDoc->displayPage(textOut, curPage, 72, 72, rotate, useMediaBox, crop, doLinks);
        txt = textOut->getText(0.0, 0.0, 10000.0, 10000.0);
        MsTimer_End(&msTimer);
        timeInMs = MsTimer_GetTimeInMs(&msTimer);
        if (gfTimings)
            LogInfo("page %d: %.2f ms\n", curPage, timeInMs);
        printf("%s\n", txt->getCString());
        delete txt;
        txt = NULL;
    }

Exit:
    LogInfo("finished: %s\n", fileName);
    delete textOut;
    delete pdfDoc;
}

/* Render one pdf file with a given 'fileName'. Log apropriate info. */
static void RenderPdfFileAsGfx(const char *fileName)
{
    MsTimer             msTimer;
    double              timeInMs;
    int                 pageCount;
    int                 pageDx, pageDy;
    int                 renderDx, renderDy;
    double              scaleX, scaleY;
    double              hDPI, vDPI;
    int                 rotate;
    GBool               useMediaBox;
    GBool               crop;
    GBool               doLinks;
    GooString *         fileNameStr = NULL;
    PDFDoc *            pdfDoc = NULL;
    SplashOutputDev *   outputDevice = NULL;
    SplashBitmap *      bitmap = NULL;

    assert(fileName);
    if (!fileName)
        return;

    LogInfo("started: %s\n", fileName);

    outputDevice = new SplashOutputDev(gSplashColorMode, 4, gFalse, gBgColor);
    if (!outputDevice) {
        error(-1, "renderPdfFile(): failed to create outputDev\n");
        goto Error;
    }

    MsTimer_Start(&msTimer);
    /* note: don't delete fileNameStr since PDFDoc takes ownership and deletes them itself */
    fileNameStr = new GooString(fileName);
    if (!fileNameStr)
        goto Error;

    pdfDoc = new PDFDoc(fileNameStr, NULL, NULL, NULL);
    if (!pdfDoc->isOk()) {
        error(-1, "renderPdfFile(): failed to open PDF file %s\n", fileName);
        goto Error;
    }
    outputDevice->startDoc(pdfDoc->getXRef());

    MsTimer_End(&msTimer);
    timeInMs = MsTimer_GetTimeInMs(&msTimer);
    LogInfo("load: %.2f ms\n", timeInMs);

    pageCount = pdfDoc->getNumPages();
    LogInfo("page count: %d\n", pageCount);

    if (gfLoadOnly)
        goto DumpLinks;

    for (int curPage = 1; curPage <= pageCount; curPage++) {
        if ((gPageNo != PAGE_NO_NOT_GIVEN) && (gPageNo != curPage))
            continue;

        pageDx = (int)pdfDoc->getPageCropWidth(curPage);
        pageDy = (int)pdfDoc->getPageCropHeight(curPage);

        renderDx = pageDx;
        renderDy = pageDy;
        if (gfForceResolution) {
            renderDx = gResolutionX;
            renderDy = gResolutionY;
        }
        MsTimer_Start(&msTimer);
        rotate = 0;
        useMediaBox = gFalse;
        crop = gTrue;
        doLinks = gTrue;
        scaleX = 1.0;
        scaleY = 1.0;
        if (pageDx != renderDx)
            scaleX = (double)renderDx / (double)pageDx;
        if (pageDy != renderDy)
            scaleY = (double)renderDy / (double)pageDy;
        hDPI = (double)PDF_FILE_DPI * scaleX;
        vDPI = (double)PDF_FILE_DPI * scaleY;
        pdfDoc->displayPage(outputDevice, curPage, hDPI, vDPI, rotate, useMediaBox, crop, doLinks);
        MsTimer_End(&msTimer);
        timeInMs = MsTimer_GetTimeInMs(&msTimer);
        if (gfTimings)
            LogInfo("page %d: %.2f ms\n", curPage, timeInMs);
        if (ShowPreview()) {
            delete bitmap;
            bitmap = outputDevice->takeBitmap();
            PreviewBitmap(bitmap);
            if (gfSlowPreview && (int)timeInMs < SLOW_PREVIEW_TIME)
                SleepMilliseconds(SLOW_PREVIEW_TIME - (int)timeInMs);
        }
    }
DumpLinks:
    if (gfDumpLinks)
        DumpLinks(pdfDoc);
Error:
    LogInfo("finished: %s\n", fileName);
    delete bitmap;
    delete outputDevice;
    delete pdfDoc;
}

static void RenderPdfFile(const char *fileName)
{
    if (gfTextOnly)
        RenderPdfFileAsText(fileName);
    else
        RenderPdfFileAsGfx(fileName);
}

int ParseInteger(const char *start, const char *end, int *intOut)
{
    char            numBuf[16];
    int             digitsCount;
    const char *    tmp;

    assert(start && end && intOut);
    assert(end >= start);
    if (!start || !end || !intOut || (start > end))
        return FALSE;

    digitsCount = 0;
    tmp = start;
    while (tmp <= end) {
        if (isspace(*tmp)) {
            /* do nothing, we allow whitespace */
        } else if (!isdigit(*tmp))
            return FALSE;
        numBuf[digitsCount] = *tmp;
        ++digitsCount;
        if (digitsCount == dimof(numBuf)-3) /* -3 to be safe */
            return FALSE;
        ++tmp;
    }
    if (0 == digitsCount)
        return FALSE;
    numBuf[digitsCount] = 0;
    *intOut = atoi(numBuf);
    return TRUE;
}

/* Given 'resolutionString' in format NxM (e.g. "100x200"), parse the string and put N
   into 'resolutionXOut' and M into 'resolutionYOut'.
   Return FALSE if there was an error (e.g. string is not in the right format */
int ParseResolutionString(const char *resolutionString, int *resolutionXOut, int *resolutionYOut)
{
    const char *    posOfX;

    assert(resolutionString);
    assert(resolutionXOut);
    assert(resolutionYOut);
    if (!resolutionString || !resolutionXOut || !resolutionYOut)
        return FALSE;
    *resolutionXOut = 0;
    *resolutionYOut = 0;
    posOfX = strchr(resolutionString, 'X');
    if (!posOfX)
        posOfX = strchr(resolutionString, 'x');
    if (!posOfX)
        return FALSE;
    if (posOfX == resolutionString)
        return FALSE;
    if (!ParseInteger(resolutionString, posOfX-1, resolutionXOut))
        return FALSE;
    if (!ParseInteger(posOfX+1, resolutionString+strlen(resolutionString)-1, resolutionYOut))
        return FALSE;
    return TRUE;
}

#ifdef DEBUG
void u_ParseResolutionString(void)
{
    int i;
    int result, resX, resY;
    const char *str;
    struct TestData {
        const char *    str;
        int             result;
        int             resX;
        int             resY;
    } testData[] = {
        { "", FALSE, 0, 0 },
        { "abc", FALSE, 0, 0},
        { "34", FALSE, 0, 0},
        { "0x0", TRUE, 0, 0},
        { "0x1", TRUE, 0, 1},
        { "0xab", FALSE, 0, 0},
        { "1x0", TRUE, 1, 0},
        { "100x200", TRUE, 100, 200},
        { "58x58", TRUE, 58, 58},
        { "  58x58", TRUE, 58, 58},
        { "58x  58", TRUE, 58, 58},
        { "58x58  ", TRUE, 58, 58},
        { "     58  x  58  ", TRUE, 58, 58},
        { "34x1234a", FALSE, 0, 0},
        { NULL, FALSE, 0, 0}
    };
    for (i=0; NULL != testData[i].str; i++) {
        str = testData[i].str;
        result = ParseResolutionString(str, &resX, &resY);
        assert(result == testData[i].result);
        if (result) {
            assert(resX == testData[i].resX);
            assert(resY == testData[i].resY);
        }
    }
}
#endif

void RunAllUnitTests(void)
{
#ifdef DEBUG
    u_ParseResolutionString();
#endif
}

void ParseCommandLine(int argc, char **argv)
{
    char *      arg;

    if (argc < 2)
        PrintUsageAndExit(argc, argv);

    for (int i=1; i < argc; i++) {
        arg = argv[i];
        assert(arg);
        if ('-' == arg[0]) {
            if (Str_EqNoCase(arg, TIMINGS_ARG)) {
                gfTimings = TRUE;
            } else if (Str_EqNoCase(arg, RESOLUTION_ARG)) {
                ++i;
                if (i == argc)
                    PrintUsageAndExit(argc, argv); /* expect a file name after that */
                if (!ParseResolutionString(argv[i], &gResolutionX, &gResolutionY))
                    PrintUsageAndExit(argc, argv);
                gfForceResolution = TRUE;
            } else if (Str_EqNoCase(arg, RECURSIVE_ARG)) {
                gfRecursive = TRUE;
            } else if (Str_EqNoCase(arg, OUT_ARG)) {
                /* expect a file name after that */
                ++i;
                if (i == argc)
                    PrintUsageAndExit(argc, argv);
                gOutFileName = Str_Dup(argv[i]);
            } else if (Str_EqNoCase(arg, PREVIEW_ARG)) {
                gfPreview = TRUE;
            } else if (Str_EqNoCase(arg, TEXT_ARG)) {
                gfTextOnly = TRUE;
            } else if (Str_EqNoCase(arg, SLOW_PREVIEW_ARG)) {
                gfSlowPreview = TRUE;
            } else if (Str_EqNoCase(arg, LOAD_ONLY_ARG)) {
                gfLoadOnly = TRUE;
            } else if (Str_EqNoCase(arg, PAGE_ARG)) {
                /* expect an integer after that */
                ++i;
                if (i == argc)
                    PrintUsageAndExit(argc, argv);
                gPageNo = atoi(argv[i]);
                if (gPageNo < 1)
                    PrintUsageAndExit(argc, argv);
            } else if (Str_EqNoCase(arg, DUMP_LINKS_ARG)) {
                gfDumpLinks = TRUE;
            } else {
                /* unknown option */
                PrintUsageAndExit(argc, argv);
            }
        } else {
            /* we assume that this is not an option hence it must be
               a name of PDF/directory/file with PDF names */
            StrList_Insert(&gArgsListRoot, arg);
        }
    }
}

#define UNIX_NEWLINE "\x0a"
#define UNIX_NEWLINE_C 0xa

void RenderPdfFileList(char *pdfFileList)
{
    char *data = NULL;
    char *dataNormalized = NULL;
    char *pdfFileName;
    unsigned long fileSize;

    assert(pdfFileList);
    if (!pdfFileList)
        return;
    data = File_Slurp(pdfFileList, &fileSize);
    if (!data) {
        error(-1, "couldn't load file '%s'", pdfFileList);
        return;
    }
    dataNormalized = Str_NormalizeNewline(data, UNIX_NEWLINE);
    if (!dataNormalized) {
        error(-1, "couldn't normalize data of file '%s'", pdfFileList);
        goto Exit;
    }
    for (;;) {
        pdfFileName = Str_SplitIter(&dataNormalized, UNIX_NEWLINE_C);
        if (!pdfFileName)
            break;
        Str_StripWsBoth(pdfFileName);
        if (Str_Empty(pdfFileName)) {
            free((void*)pdfFileName);
            continue;
        }
        RenderPdfFile(pdfFileName);
        free((void*)pdfFileName);
    }
Exit:
    free((void*)dataNormalized);
    free((void*)data);
}

#ifdef WIN32
#include <sys/types.h>
#include <sys/stat.h>

int IsDirectoryName(char *path)
{
    struct _stat    buf;
    int             result;

    result = _stat(path, &buf );
    if (0 != result)
        return FALSE;

    if (buf.st_mode & _S_IFDIR)
        return TRUE;

    return FALSE;
}

int IsFileName(char *path)
{
    struct _stat    buf;
    int             result;

    result = _stat(path, &buf );
    if (0 != result)
        return FALSE;

    if (buf.st_mode & _S_IFREG)
        return TRUE;

    return FALSE;
}
#else
int IsDirectoryName(char *path)
{
    /* TODO: implement me */
    return FALSE;
}

int IsFileName(char *path)
{
    /* TODO: implement me */
    return TRUE;
}
#endif

int IsPdfFileName(char *path)
{
    if (Str_EndsWithNoCase(path, ".pdf"))
        return TRUE;
    return FALSE;
}

void RenderDirectory(char *path)
{
    FindFileState * ffs;
    char            filename[MAX_FILENAME_SIZE];
    StrList *       dirList = NULL;
    StrList *       el;

    StrList_Insert(&dirList, path);

    while (0 != StrList_Len(&dirList)) {
        el = StrList_RemoveHead(&dirList);
        ffs = find_file_open(el->str, "*");
        while (!find_file_next(ffs, filename, sizeof(filename))) {
            if (IsDirectoryName(filename)) {
                if (gfRecursive) {
                    StrList_Insert(&dirList, filename);
                }
            } else if (IsFileName(filename)) {
                if (IsPdfFileName(filename)) {
                    RenderPdfFile(filename);
                }
            }
        }
        find_file_close(ffs);
        StrList_FreeElement(el);
    }
    StrList_Destroy(&dirList);
}

/* Render 'cmdLineArg', which can be:
   - directory name
   - name of PDF file
   - name of text file with names of PDF files
*/
void RenderCmdLineArg(char *cmdLineArg)
{
    assert(cmdLineArg);
    if (!cmdLineArg)
        return;
    if (IsDirectoryName(cmdLineArg)) {
        RenderDirectory(cmdLineArg);
    } else if (IsFileName(cmdLineArg)) {
        if (IsPdfFileName(cmdLineArg))
            RenderPdfFile(cmdLineArg);
        else
            RenderPdfFileList(cmdLineArg);
    } else {
        error(-1, "unexpected argument '%s'", cmdLineArg);
    }
}

int main(int argc, char **argv)
{
    StrList *       curr;
    FILE *          outFile = NULL;

    RunAllUnitTests();

    ParseCommandLine(argc, argv);
    if (0 == StrList_Len(&gArgsListRoot))
        PrintUsageAndExit(argc, argv);
    assert(gArgsListRoot);

    ColorsInit();
    globalParams = new GlobalParams("");
    if (!globalParams)
        return 1;
    globalParams->setErrQuiet(gFalse);

    if (gOutFileName) {
        outFile = fopen(gOutFileName, "wb");
        if (!outFile) {
            printf("failed to open -out file %s\n", gOutFileName);
            return 1;
        }
        gOutFile = outFile;
    }
    else
        gOutFile = stdout;

    if (gOutFileName)
        gErrFile = outFile;
    else
        gErrFile = stderr;

    PreviewBitmap_Init();

    curr = gArgsListRoot;
    while (curr) {
        RenderCmdLineArg(curr->str);
        curr = curr->next;
    }
    if (outFile)
        fclose(outFile);
    PreviewBitmap_Destroy();
    StrList_Destroy(&gArgsListRoot);
    delete globalParams;
    return 0;
}
