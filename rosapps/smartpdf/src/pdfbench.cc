/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
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

// this sucks but I don't know any other way
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "str_util.h"
#include "base_util.h"
#include "file_util.h"

#include "PdfEngine.h"

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
#include "pdiff.h"

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

extern void PreviewBitmapInit(void);
extern void PreviewBitmapDestroy(void);
extern void PreviewBitmapSplashFitz(RenderedBitmap *bmpSplash, RenderedBitmap *bmpFitz);
extern "C" char *GetPasswordForFile(WindowInfo *win, const char *fileName);

char *GetPasswordForFile(WindowInfo *win, const char *fileName)
{
    return strdup("");
}

static void PreviewBitmapFitz(RenderedBitmap *bmpFitz)
{
    PreviewBitmapSplashFitz(NULL, bmpFitz);
}

static void PreviewBitmapSplash(RenderedBitmap *bmpSplash)
{
    PreviewBitmapSplashFitz(bmpSplash, NULL);
}

class RGBAImageFitz : public RGBAImage
{
public:
    RGBAImageFitz(fz_pixmap *bmp) { 
        m_bmp = bmp; 
#ifdef FITZ_HEAD
        samples = bmp->p;
#else
        samples = bmp->samples;
#endif
        w = bmp->w;
        h = bmp->h;
    }
    virtual ~RGBAImageFitz() {}
    virtual int Get_Width(void) const { return m_bmp->w; }
    virtual int Get_Height(void) const  { return m_bmp->h; }
    virtual unsigned char Get_Red(unsigned int i) { return samples[i*4]; }
    virtual unsigned char Get_Green(unsigned int i) { return samples[i*4+1]; }
    virtual unsigned char Get_Blue(unsigned int i) { return samples[i*4+2]; }
//    virtual unsigned char Get_Alpha(unsigned int i) { return samples[i*4+3]; }
    virtual unsigned char Get_Alpha(unsigned int i) { return 0; }
    virtual void Set(unsigned char r, unsigned char g, unsigned char b, unsigned char a, unsigned int i) { /* no-op */ }
    virtual unsigned int Get(int i) const { 
        unsigned int *data = (unsigned int *)samples;
        return samples[i];
    }
private:
    fz_pixmap * m_bmp;
    fz_sample * samples;
    int         w;
    int         h;
};

class RGBAImageSplash : public RGBAImage
{
public:
    RGBAImageSplash(SplashBitmap *bmp) { 
        m_bmp = bmp; 
        assert(splashModeBGR8 == m_bmp->getMode());
        rowSize = m_bmp->getRowSize();
        assert(rowSize > 0);
    }
    virtual ~RGBAImageSplash() {}
    virtual int Get_Width(void) const { return m_bmp->getWidth(); }
    virtual int Get_Height(void) const { return m_bmp->getHeight(); }
    virtual unsigned char Get_Red(unsigned int i) { 
        SplashColor p;
        int x = i % rowSize;
        int y = i / rowSize;
        m_bmp->getPixel(x, y, p);
        return splashBGR8R(p);
    }
    virtual unsigned char Get_Green(unsigned int i) { 
        SplashColor p;
        int x = i % rowSize;
        int y = i / rowSize;
        m_bmp->getPixel(x, y, p);
        return splashBGR8G(p);
    }
    virtual unsigned char Get_Blue(unsigned int i) {
        SplashColor p;
        int x = i % rowSize;
        int y = i / rowSize;
        m_bmp->getPixel(x, y, p);
        return splashBGR8B(p);
    }
    virtual unsigned char Get_Alpha(unsigned int i) { return 0; }
    virtual void Set(unsigned char r, unsigned char g, unsigned char b, unsigned char a, unsigned int i) { /* no-op */ }
    virtual unsigned int Get(int i) const { 
        SplashColor p;
        int x = i % rowSize;
        int y = i / rowSize;
        m_bmp->getPixel(x, y, p);
        return p[0] + ((unsigned int)p[1] >> 8) + ((unsigned int)p[2] >> 16);
    }
private:
    SplashBitmap *  m_bmp; 
    int             rowSize;
};

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

char *makepath(char *buf, int buf_size, const char *path,
               const char *filename)
{
    strcpy_s(buf, buf_size, path);
    int len = strlen(path);
    if (len > 0 && path[len - 1] != DIR_SEP_CHAR && len + 1 < buf_size) {
        buf[len++] = DIR_SEP_CHAR;
        buf[len] = '\0';
    }
    strcat_s(buf, buf_size, filename);
    return buf;
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
#define DUMP_LINKS_ARG      "-links"
#define TEXT_ARG            "-text"
#define FITZ_ARG            "-fitz"
#define BOTH_ARG            "-both"
#define PDIFF_ARG           "-pdiff"

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

/* If true and rendering both, checks images for visual differences */
static BOOL gfPDiff = FALSE;

/* 1 second (1000 milliseconds) */
#define SLOW_PREVIEW_TIME 1000

/* If true, preview rendered image in a slow mode i.e. delay displaying for
   SLOW_PREVIEW_TIME. This is so that a human has enough time to see if the
   PDF renders ok. In release mode on fast processor pages take only ~100-200 ms
   to render and they go away too quickly to be inspected by a human. */
static int gfSlowPreview = FALSE;

/* If true, we only dump the text, not render */
static int gfTextOnly = FALSE;

/* If true, using fitz (instead of poppler) for rendering */
static int gfFitzRendering = FALSE;

#define PAGE_NO_NOT_GIVEN -1

/* If equals PAGE_NO_NOT_GIVEN, we're in default mode where we render all pages.
   If different, will only render this page */
static int  gPageNo = PAGE_NO_NOT_GIVEN;
/* If true, will only load the file, not render any pages. Mostly for
   profiling load time */
static BOOL gfLoadOnly = FALSE;

/* if TRUE, will dump information about links */
static BOOL gfLinks = FALSE;

/* if TRUE, will timer, render and preview both fitz and poppler backends */
static BOOL gfBoth = FALSE;

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
    txtDup = str_dup(txt);
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

static void printUsageAndExit(int argc, char **argv)
{
    printf("Usage: pdftest [-preview] [-slowpreview] [-timings] [-text] [-resolution NxM] [-recursive] [-page N] [-out out.txt] pdf-files-to-process\n");
    for (int i=0; i < argc; i++) {
        printf("i=%d, '%s'\n", i, argv[i]);
    }
    exit(0);
}

void *StandardSecurityHandler::getAuthData()
{
    return NULL;
}

static int ShowPreview(void)
{
    if (gfPreview || gfSlowPreview)
        return TRUE;
    return FALSE;
}

static int DoPDiff(void)
{
    return gfPDiff;
}

static void DumpLinks(int pageNo, PdfEngine *engine)
{
    assert(engine);
    if (!engine) return;
    int linkCount = engine->linkCount(pageNo);
    for (int linkNo = 0; linkNo < linkCount; ++linkNo) {
        const char *linkType = engine->linkType(pageNo, linkNo);
        LogInfo("Link: page %d, type=%s\n", pageNo, linkType);
    }
}

static void renderPdfAsText(const char *fileName)
{
    GooString *         fileNameStr = NULL;
    PDFDoc *            pdfDoc = NULL;
    GooString *         txt = NULL;

    assert(fileName);
    if (!fileName)
        return;

    LogInfo("started: %s\n", fileName);

    TextOutputDev * textOut = new TextOutputDev(NULL, gTrue, gFalse, gFalse);
    if (!textOut->isOk()) {
        delete textOut;
        return;
    }

    MsTimer msTimer;
    /* note: don't delete fileNameStr since PDFDoc takes ownership and deletes them itself */
    fileNameStr = new GooString(fileName);
    if (!fileNameStr)
        goto Exit;

    pdfDoc = new PDFDoc(fileNameStr, NULL, NULL, NULL);
    if (!pdfDoc->isOk()) {
        error(-1, "renderPdfFile(): failed to open PDF file %s\n", fileName);
        goto Exit;
    }

    msTimer.stop();
    double timeInMs = msTimer.timeInMs();
    LogInfo("load: %.2f ms\n", timeInMs);

    int pageCount = pdfDoc->getNumPages();
    LogInfo("page count: %d\n", pageCount);

    for (int curPage = 1; curPage <= pageCount; curPage++) {
        if ((gPageNo != PAGE_NO_NOT_GIVEN) && (gPageNo != curPage))
            continue;

        msTimer.start();
        int rotate = 0;
        GBool useMediaBox = gFalse;
        GBool crop = gTrue;
        GBool doLinks = gFalse;
        pdfDoc->displayPage(textOut, curPage, 72, 72, rotate, useMediaBox, crop, doLinks);
        txt = textOut->getText(0.0, 0.0, 10000.0, 10000.0);
        msTimer.stop();
        timeInMs = msTimer.timeInMs();
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

extern "C" fz_error *initfontlibs_ms(void);

#define FITZ_TMP_NAME "c:\\fitz_tmp.pdf"
#define POPPLER_TMP_NAME "c:\\poppler_tmp.pdf"

#if 0
void SplashRender::RenderPage(int pageNo)
{
    pageDx = (int)pdfEngine->pdfDoc()->getPageCropWidth(pageNo);
    pageDy = (int)pdfEngine->pdfDoc()->getPageCropHeight(pageNo);

    renderDx = pageDx;
    renderDy = pageDy;
    if (gfForceResolution) {
        renderDx = gResolutionX;
        renderDy = gResolutionY;
    }
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
    pdfEngine->pdfDoc()->displayPage(outputDevice, pageNo, hDPI, vDPI, rotate, useMediaBox, crop, doLinks);
    FreePage();
}
#endif

enum RenderType {
    renderFitz,
    renderSplash,
    renderBoth
};

static void renderPdf(const char *fileName, RenderType renderType)
{
    const char *fileNameFitz = NULL;
    PdfEngine * engineFitz = NULL;
    int         pageCountFitz;

    const char *fileNameSplash = NULL;
    PdfEngine * engineSplash = NULL;
    int         pageCountSplash;

    switch (renderType) {
        case renderBoth:            
            // TODO: fails if file already exists and has read-only attribute
            CopyFile(fileName, FITZ_TMP_NAME, FALSE);
            CopyFile(fileName, POPPLER_TMP_NAME, FALSE);
            fileNameSplash = POPPLER_TMP_NAME;
            fileNameFitz = FITZ_TMP_NAME;
            break;
       case renderFitz:
            fileNameFitz = fileName;
            break;
       case renderSplash:
            fileNameSplash = fileName;
            break;
    }

    LogInfo("started: %s\n", fileName);
    initfontlibs_ms();

    if (fileNameFitz) {
        engineFitz = new PdfEngineFitz();

        MsTimer msTimer;
        if (!engineFitz->load(fileNameFitz, NULL)) {
            LogInfo("failed to load fitz\n");
            goto Error;
        }
        msTimer.stop();
        double timeInMs = msTimer.timeInMs();
        LogInfo("load fitz  : %.2f ms\n", timeInMs);
        pageCountFitz = engineFitz->pageCount();
    }

    if (fileNameSplash) {
        engineSplash = new PdfEnginePoppler();

        MsTimer msTimer;
        if (!engineSplash->load(fileNameSplash, NULL)) {
            LogInfo("failed to load splash\n");
            goto Error;
        }
        msTimer.stop();
        double timeInMs = msTimer.timeInMs();
        LogInfo("load splash: %.2f ms\n", timeInMs);
        pageCountSplash = engineSplash->pageCount();
    }

    int pageCount;
    switch (renderType) {
        case renderBoth:            
            pageCount = pageCountFitz;
            if (pageCountSplash < pageCount)
                pageCount = pageCountSplash;
            break;
       case renderFitz:
            pageCount = pageCountFitz;
            break;
       case renderSplash:
            pageCount = pageCountSplash;
            break;
    }
    LogInfo("page count: %d\n", pageCount);

    for (int curPage = 1; curPage <= pageCount; curPage++) {
        if ((gPageNo != PAGE_NO_NOT_GIVEN) && (gPageNo != curPage))
            continue;

        RenderedBitmap *bmpFitz = NULL;
        RenderedBitmap *bmpSplash = NULL;

        if (fileNameFitz) {
            MsTimer msTimer;
            bmpFitz = engineFitz->renderBitmap(curPage, 100.0, 0, NULL, NULL);
            msTimer.stop();
            double timeInMs = msTimer.timeInMs();

            if (gfTimings)
                if (!bmpFitz)
                    LogInfo("page fitz   %d: failed to render\n", curPage);
                else
                    LogInfo("page fitz   %d (%dx%d): %.2f ms\n", curPage, bmpFitz->dx(), bmpFitz->dy(), timeInMs);
            if (gfLinks)
                DumpLinks(curPage, engineFitz);
        }

        if (fileNameSplash) {
            MsTimer msTimer;
            bmpSplash = engineSplash->renderBitmap(curPage, 100.0, 0, NULL, NULL);
            msTimer.stop();
            double timeInMs = msTimer.timeInMs();
            if (gfTimings)
                if (!bmpSplash)
                    LogInfo("page splash %d: failed to render\n", curPage);
                else
                    LogInfo("page splash %d (%dx%d): %.2f ms\n", curPage, bmpSplash->dx(), bmpSplash->dy(), timeInMs);
            if (gfLinks)
                DumpLinks(curPage, engineSplash);
        }

        if (ShowPreview()) {
            PreviewBitmapSplashFitz(bmpSplash, bmpFitz);
            if (gfSlowPreview)
                sleep_milliseconds(SLOW_PREVIEW_TIME);
        }

#if 0
        if (DoPDiff()) {
            CompareArgs compareArgs;
            compareArgs.ImgA = new RGBAImageFitz(renderFitz->image);
            compareArgs.ImgB = new RGBAImageSplash(splashBmp);
            compareArgs.ImgDiff = NULL;
            unsigned long pixelDiffCount = Yee_Compare(compareArgs);
            LogInfo("pixels different: %d\n", (int)pixelDiffCount);
        }
#endif
    }

Error:
    delete engineFitz;
    delete engineSplash;
    LogInfo("finished: %s\n", fileName);
}

static void renderFile(const char *fileName)
{
    if (gfTextOnly) {
        /* TODO: right not rendering as text is only supported with poppler, not fitz */
        renderPdfAsText(fileName);
        return;
    }

    RenderType renderType;
    if (gfBoth)
        renderType = renderBoth;
    else {
        renderType = renderSplash;
        if (gfFitzRendering)
            renderType = renderFitz;
    }
    renderPdf(fileName, renderType);
}

static int ParseInteger(const char *start, const char *end, int *intOut)
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
static int ParseResolutionString(const char *resolutionString, int *resolutionXOut, int *resolutionYOut)
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
static void u_ParseResolutionString(void)
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

static void runAllUnitTests(void)
{
#ifdef DEBUG
    u_ParseResolutionString();
#endif
}

static void parseCommandLine(int argc, char **argv)
{
    char *      arg;

    if (argc < 2)
        printUsageAndExit(argc, argv);

    for (int i=1; i < argc; i++) {
        arg = argv[i];
        assert(arg);
        if ('-' == arg[0]) {
            if (str_ieq(arg, TIMINGS_ARG)) {
                gfTimings = TRUE;
            } else if (str_ieq(arg, RESOLUTION_ARG)) {
                ++i;
                if (i == argc)
                    printUsageAndExit(argc, argv); /* expect a file name after that */
                if (!ParseResolutionString(argv[i], &gResolutionX, &gResolutionY))
                    printUsageAndExit(argc, argv);
                gfForceResolution = TRUE;
            } else if (str_ieq(arg, RECURSIVE_ARG)) {
                gfRecursive = TRUE;
            } else if (str_ieq(arg, OUT_ARG)) {
                /* expect a file name after that */
                ++i;
                if (i == argc)
                    printUsageAndExit(argc, argv);
                gOutFileName = str_dup(argv[i]);
            } else if (str_ieq(arg, PREVIEW_ARG)) {
                gfPreview = TRUE;
            } else if (str_ieq(arg, TEXT_ARG)) {
                gfTextOnly = TRUE;
            } else if (str_ieq(arg, SLOW_PREVIEW_ARG)) {
                gfSlowPreview = TRUE;
            } else if (str_ieq(arg, LOAD_ONLY_ARG)) {
                gfLoadOnly = TRUE;
            } else if (str_ieq(arg, FITZ_ARG)) {
                gfFitzRendering = TRUE;
            } else if (str_ieq(arg, BOTH_ARG)) {
                gfBoth = TRUE;
            } else if (str_ieq(arg, PDIFF_ARG)) {
                gfPDiff = TRUE;
            } else if (str_ieq(arg, PAGE_ARG)) {
                /* expect an integer after that */
                ++i;
                if (i == argc)
                    printUsageAndExit(argc, argv);
                gPageNo = atoi(argv[i]);
                if (gPageNo < 1)
                    printUsageAndExit(argc, argv);
            } else if (str_ieq(arg, DUMP_LINKS_ARG)) {
                gfLinks = TRUE;
            } else {
                /* unknown option */
                printUsageAndExit(argc, argv);
            }
        } else {
            /* we assume that this is not an option hence it must be
               a name of PDF/directory/file with PDF names */
            StrList_Insert(&gArgsListRoot, arg);
        }
    }
}

void renderFileList(char *pdfFileList)
{
    char *data = NULL;
    char *dataNormalized = NULL;
    char *pdfFileName;
    size_t fileSize;

    assert(pdfFileList);
    if (!pdfFileList)
        return;
    data = file_read_all(pdfFileList, &fileSize);
    if (!data) {
        error(-1, "couldn't load file '%s'", pdfFileList);
        return;
    }
    dataNormalized = str_normalize_newline(data, UNIX_NEWLINE);
    if (!dataNormalized) {
        error(-1, "couldn't normalize data of file '%s'", pdfFileList);
        goto Exit;
    }
    for (;;) {
        pdfFileName = str_split_iter(&dataNormalized, UNIX_NEWLINE_C);
        if (!pdfFileName)
            break;
        str_strip_ws_both(pdfFileName);
        if (str_empty(pdfFileName)) {
            free((void*)pdfFileName);
            continue;
        }
        renderFile(pdfFileName);
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
    if (str_endswith(path, ".pdf"))
        return TRUE;
    return FALSE;
}

void renderDirectory(char *path)
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
                    renderFile(filename);
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
static void renderCmdLineArg(char *cmdLineArg)
{
    assert(cmdLineArg);
    if (!cmdLineArg)
        return;
    if (IsDirectoryName(cmdLineArg)) {
        renderDirectory(cmdLineArg);
    } else if (IsFileName(cmdLineArg)) {
        if (IsPdfFileName(cmdLineArg))
            renderFile(cmdLineArg);
        else
            renderFileList(cmdLineArg);
    } else {
        error(-1, "unexpected argument '%s'", cmdLineArg);
    }
}

extern "C" void deinitfontlibs_ms(void);

extern "C" void dump_type_stats(void);

int main(int argc, char **argv)
{
    runAllUnitTests();

    parseCommandLine(argc, argv);
    if (0 == StrList_Len(&gArgsListRoot))
        printUsageAndExit(argc, argv);
    assert(gArgsListRoot);

    void *leak = malloc(20);

    SplashColorsInit();
    globalParams = new GlobalParams("");
    if (!globalParams)
        return 1;
    globalParams->setErrQuiet(gFalse);

    FILE * outFile = NULL;
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

    PreviewBitmapInit();

    StrList * curr = gArgsListRoot;
    while (curr) {
        renderCmdLineArg(curr->str);
        curr = curr->next;
    }
    if (outFile)
        fclose(outFile);
    PreviewBitmapDestroy();
    deinitfontlibs_ms();
    StrList_Destroy(&gArgsListRoot);
    dump_type_stats();
    delete globalParams;
    return 0;
}
