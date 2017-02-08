/*
 * Help Viewer
 *
 * Copyright    1996 Ulrich Schmid
 *              2002, 2008 Eric Pouech
 *              2007 Kirill K. Smirnov
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

typedef struct tagHlpFileLink
{
    enum {hlp_link_link, hlp_link_popup, hlp_link_macro} cookie;
    LPCSTR      string;         /* name of the file to for the link (NULL if same file) */
    LONG        hash;           /* topic index */
    unsigned    bClrChange : 1; /* true if the link is green & underlined */
    unsigned    bHotSpot : 1;   /* true if the link is an hotspot (actually HLPFILE_HOTSPOTLINK) */
    unsigned    window;         /* window number for displaying the link (-1 is current) */
    DWORD       cpMin;
    DWORD       cpMax;
    struct tagHlpFileLink* next;
} HLPFILE_LINK;

typedef struct tagHlpFileHotSpotLink
{
    HLPFILE_LINK link;
    unsigned    x;
    unsigned    y;
    unsigned    width;
    unsigned    height;
} HLPFILE_HOTSPOTLINK;

typedef struct tagHlpFileMacro
{
    LPCSTR                      lpszMacro;
    struct tagHlpFileMacro*     next;
} HLPFILE_MACRO;

typedef struct tagHlpFilePage
{
    LPSTR                       lpszTitle;
    HLPFILE_MACRO*              first_macro;

    HLPFILE_LINK*               first_link;

    unsigned                    wNumber;
    unsigned                    offset;
    DWORD                       reference;
    struct tagHlpFilePage*      next;
    struct tagHlpFilePage*      prev;

    DWORD                       browse_bwd;
    DWORD                       browse_fwd;

    struct tagHlpFileFile*      file;
} HLPFILE_PAGE;

typedef struct
{
    LONG                        lMap;
    unsigned long               offset;
} HLPFILE_MAP;

typedef struct
{
    LOGFONTA                    LogFont;
    HFONT                       hFont;
    COLORREF                    color;
} HLPFILE_FONT;

typedef struct tagHlpFileFile
{
    BYTE*                       file_buffer;
    UINT                        file_buffer_size;
    LPSTR                       lpszPath;
    LPSTR                       lpszTitle;
    LPSTR                       lpszCopyright;
    HLPFILE_PAGE*               first_page;
    HLPFILE_PAGE*               last_page;
    HLPFILE_MACRO*              first_macro;
    BYTE*                       Context;
    BYTE*                       kwbtree;
    BYTE*                       kwdata;
    unsigned                    wMapLen;
    HLPFILE_MAP*                Map;
    unsigned                    wTOMapLen;
    unsigned*                   TOMap;
    unsigned long               contents_start;

    struct tagHlpFileFile*      prev;
    struct tagHlpFileFile*      next;

    unsigned                    wRefCount;

    unsigned short              version;
    unsigned short              flags;
    unsigned short              charset;
    unsigned short              tbsize;     /* topic block size */
    unsigned short              dsize;      /* decompress size */
    BOOL                        compressed;
    BOOL                        hasPhrases;   /* file has |Phrases */
    BOOL                        hasPhrases40; /* file has |PhrIndex/|PhrImage */
    UINT                        num_phrases;
    unsigned*                   phrases_offsets;
    char*                       phrases_buffer;

    BYTE**                      topic_map;
    BYTE*                       topic_end;
    UINT                        topic_maplen;

    unsigned                    numBmps;
    HBITMAP*                    bmps;

    unsigned                    numFonts;
    HLPFILE_FONT*               fonts;

    unsigned                    numWindows;
    HLPFILE_WINDOWINFO*         windows;
    HICON                       hIcon;

    BOOL                        has_popup_color;
    COLORREF                    popup_color;

    LPSTR                       help_on_file;

    int                         scale;
    int                         rounderr;
} HLPFILE;

/*
 * Compare function type for HLPFILE_BPTreeSearch function.
 *
 * PARAMS
 *     p       [I] pointer to testing block (key + data)
 *     key     [I] pointer to key value to look for
 *     leaf    [I] whether this function called for index of leaf page
 *     next    [O] pointer to pointer to next block
 */
typedef int (*HLPFILE_BPTreeCompare)(void *p, const void *key,
                                     int leaf, void **next);

/*
 * Callback function type for HLPFILE_BPTreeEnum function.
 *
 * PARAMS
 *     p       [I]  pointer to data block
 *     next    [O]  pointer to pointer to next block
 *     cookie  [IO] cookie data
 */
typedef void (*HLPFILE_BPTreeCallback)(void *p, void **next, void *cookie);

HLPFILE*      HLPFILE_ReadHlpFile(LPCSTR lpszPath);
HLPFILE_PAGE* HLPFILE_PageByHash(HLPFILE* hlpfile, LONG lHash, ULONG* relative);
HLPFILE_PAGE* HLPFILE_PageByMap(HLPFILE* hlpfile, LONG lMap, ULONG* relative);
HLPFILE_PAGE* HLPFILE_PageByOffset(HLPFILE* hlpfile, LONG offset, ULONG* relative);
LONG          HLPFILE_Hash(LPCSTR lpszContext);
void          HLPFILE_FreeHlpFile(HLPFILE*);

void  HLPFILE_BPTreeEnum(BYTE*, HLPFILE_BPTreeCallback cb, void *cookie);

struct RtfData {
    BOOL        in_text;
    char*       data;           /* RTF stream start */
    char*       ptr;            /* current position in stream */
    unsigned    allocated;      /* overall allocated size */
    unsigned    char_pos;       /* current char position (in richedit) */
    char*       where;          /* pointer to feed back richedit */
    unsigned    font_scale;     /* how to scale fonts */
    HLPFILE_LINK*first_link;
    HLPFILE_LINK*current_link;
    BOOL        force_color;
    unsigned    relative;       /* offset within page to lookup for */
    unsigned    char_pos_rel;   /* char_pos correspondinf to relative */
};

BOOL          HLPFILE_BrowsePage(HLPFILE_PAGE*, struct RtfData* rd,
                                 unsigned font_scale, unsigned relative);

#define HLP_DISPLAY30 0x01     /* version 3.0 displayable information */
#define HLP_TOPICHDR  0x02     /* topic header information */
#define HLP_DISPLAY   0x20     /* version 3.1 displayable information */
#define HLP_TABLE     0x23     /* version 3.1 table */
