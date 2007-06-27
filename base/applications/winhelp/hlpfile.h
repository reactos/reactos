/*
 * Help Viewer
 *
 * Copyright    1996 Ulrich Schmid
 *              2002 Eric Pouech
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

struct tagHelpFile;

typedef struct 
{
    char        type[10];
    char        name[9];
    char        caption[51];
    POINT       origin;
    SIZE        size;
    int         style;
    DWORD       win_style;
    COLORREF    sr_color;       /* color for scrollable region */
    COLORREF    nsr_color;      /* color for non scrollable region */
} HLPFILE_WINDOWINFO;

typedef struct
{
    enum {hlp_link_link, hlp_link_popup, hlp_link_macro} cookie;
    LPCSTR      lpszString;     /* name of the file to for the link (NULL if same file) */
    LONG        lHash;          /* topic index */
    unsigned    bClrChange : 1, /* true if the link is green & underlined */
                wRefCount;      /* number of internal references to this object */
    unsigned    window;         /* window number for displaying the link (-1 is current) */
} HLPFILE_LINK;

enum para_type {para_normal_text, para_debug_text, para_bitmap, para_metafile};

typedef struct tagHlpFileParagraph
{
    enum para_type              cookie;

    union
    {
        struct
        {
            LPSTR                       lpszText;
            unsigned                    wFont;
            unsigned                    wIndent;
            unsigned                    wHSpace;
            unsigned                    wVSpace;
        } text;
        struct
        {
            unsigned                    pos;    /* 0: center, 1: left, 2: right */
            union 
            {
                struct 
                {
                    HBITMAP             hBitmap;
                } bmp;
                METAFILEPICT            mfp;
            } u;
        } gfx; /* for bitmaps and metafiles */
    } u;

    HLPFILE_LINK*               link;

    struct tagHlpFileParagraph* next;
} HLPFILE_PARAGRAPH;

typedef struct tagHlpFileMacro
{
    LPCSTR                      lpszMacro;
    struct tagHlpFileMacro*     next;
} HLPFILE_MACRO;

typedef struct tagHlpFilePage
{
    LPSTR                       lpszTitle;
    HLPFILE_PARAGRAPH*          first_paragraph;
    HLPFILE_MACRO*              first_macro;

    unsigned                    wNumber;
    unsigned                    offset;
    struct tagHlpFilePage*      next;
    struct tagHlpFilePage*      prev;

    DWORD                       browse_bwd;
    DWORD                       browse_fwd;

    struct tagHlpFileFile*      file;
} HLPFILE_PAGE;

typedef struct
{
    LONG                        lHash;
    unsigned long               offset;
} HLPFILE_CONTEXT;

typedef struct
{
    LONG                        lMap;
    unsigned long               offset;
} HLPFILE_MAP;

typedef struct
{
    LOGFONT                     LogFont;
    HFONT                       hFont;
    COLORREF                    color;
} HLPFILE_FONT;

typedef struct tagHlpFileFile
{
    LPSTR                       lpszPath;
    LPSTR                       lpszTitle;
    LPSTR                       lpszCopyright;
    HLPFILE_PAGE*               first_page;
    HLPFILE_MACRO*              first_macro;
    unsigned                    wContextLen;
    HLPFILE_CONTEXT*            Context;
    unsigned                    wMapLen;
    HLPFILE_MAP*                Map;
    unsigned long               contents_start;

    struct tagHlpFileFile*      prev;
    struct tagHlpFileFile*      next;

    unsigned                    wRefCount;

    unsigned short              version;
    unsigned short              flags;
    unsigned                    hasPhrases; /* Phrases or PhrIndex/PhrImage */

    unsigned                    numBmps;
    HBITMAP*                    bmps;

    unsigned                    numFonts;
    HLPFILE_FONT*               fonts;

    unsigned                    numWindows;
    HLPFILE_WINDOWINFO*         windows;
} HLPFILE;

HLPFILE*      HLPFILE_ReadHlpFile(LPCSTR lpszPath);
HLPFILE_PAGE* HLPFILE_Contents(HLPFILE* hlpfile);
HLPFILE_PAGE* HLPFILE_PageByHash(HLPFILE* hlpfile, LONG lHash);
HLPFILE_PAGE* HLPFILE_PageByMap(HLPFILE* hlpfile, LONG lMap);
HLPFILE_PAGE* HLPFILE_PageByOffset(HLPFILE* hlpfile, LONG offset);
LONG          HLPFILE_Hash(LPCSTR lpszContext);
void          HLPFILE_FreeLink(HLPFILE_LINK* link);
void          HLPFILE_FreeHlpFile(HLPFILE*);
