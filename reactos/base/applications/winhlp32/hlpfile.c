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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winhelp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhelp);

static inline unsigned short GET_USHORT(const BYTE* buffer, unsigned i)
{
    return (BYTE)buffer[i] + 0x100 * (BYTE)buffer[i + 1];
}

static inline short GET_SHORT(const BYTE* buffer, unsigned i)
{
    return (BYTE)buffer[i] + 0x100 * (signed char)buffer[i+1];
}

static inline unsigned GET_UINT(const BYTE* buffer, unsigned i)
{
    return GET_USHORT(buffer, i) + 0x10000 * GET_USHORT(buffer, i + 2);
}

static HLPFILE *first_hlpfile = 0;

static BOOL  HLPFILE_DoReadHlpFile(HLPFILE*, LPCSTR);
static BOOL  HLPFILE_ReadFileToBuffer(HLPFILE*, HFILE);
static BOOL  HLPFILE_FindSubFile(HLPFILE*, LPCSTR, BYTE**, BYTE**);
static BOOL  HLPFILE_SystemCommands(HLPFILE*);
static INT   HLPFILE_UncompressedLZ77_Size(const BYTE *ptr, const BYTE *end);
static BYTE* HLPFILE_UncompressLZ77(const BYTE *ptr, const BYTE *end, BYTE *newptr);
static BOOL  HLPFILE_UncompressLZ77_Phrases(HLPFILE*);
static BOOL  HLPFILE_Uncompress_Phrases40(HLPFILE*);
static BOOL  HLPFILE_Uncompress_Topic(HLPFILE*);
static BOOL  HLPFILE_GetContext(HLPFILE*);
static BOOL  HLPFILE_GetKeywords(HLPFILE*);
static BOOL  HLPFILE_GetMap(HLPFILE*);
static BOOL  HLPFILE_GetTOMap(HLPFILE*);
static BOOL  HLPFILE_AddPage(HLPFILE*, const BYTE*, const BYTE*, unsigned, unsigned);
static BOOL  HLPFILE_SkipParagraph(HLPFILE*, const BYTE*, const BYTE*, unsigned*);
static void  HLPFILE_Uncompress2(HLPFILE*, const BYTE*, const BYTE*, BYTE*, const BYTE*);
static BOOL  HLPFILE_Uncompress3(HLPFILE*, char*, const char*, const BYTE*, const BYTE*);
static void  HLPFILE_UncompressRLE(const BYTE* src, const BYTE* end, BYTE* dst, unsigned dstsz);
static BOOL  HLPFILE_ReadFont(HLPFILE* hlpfile);

/******************************************************************
 *		HLPFILE_PageByOffset
 *
 *
 */
HLPFILE_PAGE *HLPFILE_PageByOffset(HLPFILE* hlpfile, LONG offset, ULONG* relative)
{
    HLPFILE_PAGE*       page;
    HLPFILE_PAGE*       found;

    if (!hlpfile) return 0;

    WINE_TRACE("<%s>[%x]\n", hlpfile->lpszPath, offset);

    if (offset == 0xFFFFFFFF) return NULL;
    page = NULL;

    for (found = NULL, page = hlpfile->first_page; page; page = page->next)
    {
        if (page->offset <= offset && (!found || found->offset < page->offset))
        {
            *relative = offset - page->offset;
            found = page;
        }
    }
    if (!found)
        WINE_ERR("Page of offset %u not found in file %s\n",
                 offset, hlpfile->lpszPath);
    return found;
}

/**************************************************************************
 * comp_PageByHash
 *
 * HLPFILE_BPTreeCompare function for '|CONTEXT' B+ tree file
 *
 */
static int comp_PageByHash(void *p, const void *key,
                           int leaf, void** next)
{
    LONG lKey = (LONG_PTR)key;
    LONG lTest = (INT)GET_UINT(p, 0);

    *next = (char *)p+(leaf?8:6);
    WINE_TRACE("Comparing '%d' with '%d'\n", lKey, lTest);
    if (lTest < lKey) return -1;
    if (lTest > lKey) return 1;
    return 0;
}

/***********************************************************************
 *
 *           HLPFILE_PageByHash
 */
HLPFILE_PAGE *HLPFILE_PageByHash(HLPFILE* hlpfile, LONG lHash, ULONG* relative)
{
    BYTE *ptr;

    if (!hlpfile) return NULL;
    if (!lHash) return HLPFILE_Contents(hlpfile, relative);

    WINE_TRACE("<%s>[%x]\n", hlpfile->lpszPath, lHash);

    /* For win 3.0 files hash values are really page numbers */
    if (hlpfile->version <= 16)
    {
        if (lHash >= hlpfile->wTOMapLen) return NULL;
        return HLPFILE_PageByOffset(hlpfile, hlpfile->TOMap[lHash], relative);
    }

    ptr = HLPFILE_BPTreeSearch(hlpfile->Context, LongToPtr(lHash), comp_PageByHash);
    if (!ptr)
    {
        WINE_ERR("Page of hash %x not found in file %s\n", lHash, hlpfile->lpszPath);
        return NULL;
    }

    return HLPFILE_PageByOffset(hlpfile, GET_UINT(ptr, 4), relative);
}

/***********************************************************************
 *
 *           HLPFILE_PageByMap
 */
HLPFILE_PAGE *HLPFILE_PageByMap(HLPFILE* hlpfile, LONG lMap, ULONG* relative)
{
    unsigned int i;

    if (!hlpfile) return 0;

    WINE_TRACE("<%s>[%x]\n", hlpfile->lpszPath, lMap);

    for (i = 0; i < hlpfile->wMapLen; i++)
    {
        if (hlpfile->Map[i].lMap == lMap)
            return HLPFILE_PageByOffset(hlpfile, hlpfile->Map[i].offset, relative);
    }

    WINE_ERR("Page of Map %x not found in file %s\n", lMap, hlpfile->lpszPath);
    return NULL;
}

/***********************************************************************
 *
 *           HLPFILE_Contents
 */
HLPFILE_PAGE* HLPFILE_Contents(HLPFILE *hlpfile, ULONG* relative)
{
    HLPFILE_PAGE*       page = NULL;

    if (!hlpfile) return NULL;

    page = HLPFILE_PageByOffset(hlpfile, hlpfile->contents_start, relative);
    if (!page)
    {
        page = hlpfile->first_page;
        *relative = 0;
    }
    return page;
}

/***********************************************************************
 *
 *           HLPFILE_Hash
 */
LONG HLPFILE_Hash(LPCSTR lpszContext)
{
    LONG lHash = 0;
    CHAR c;

    while ((c = *lpszContext++))
    {
        CHAR x = 0;
        if (c >= 'A' && c <= 'Z') x = c - 'A' + 17;
        if (c >= 'a' && c <= 'z') x = c - 'a' + 17;
        if (c >= '1' && c <= '9') x = c - '0';
        if (c == '0') x = 10;
        if (c == '.') x = 12;
        if (c == '_') x = 13;
        if (x) lHash = lHash * 43 + x;
    }
    return lHash;
}

/***********************************************************************
 *
 *           HLPFILE_ReadHlpFile
 */
HLPFILE *HLPFILE_ReadHlpFile(LPCSTR lpszPath)
{
    HLPFILE*      hlpfile;

    for (hlpfile = first_hlpfile; hlpfile; hlpfile = hlpfile->next)
    {
        if (!strcmp(lpszPath, hlpfile->lpszPath))
        {
            hlpfile->wRefCount++;
            return hlpfile;
        }
    }

    hlpfile = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                        sizeof(HLPFILE) + lstrlen(lpszPath) + 1);
    if (!hlpfile) return 0;

    hlpfile->lpszPath           = (char*)hlpfile + sizeof(HLPFILE);
    hlpfile->contents_start     = 0xFFFFFFFF;
    hlpfile->next               = first_hlpfile;
    hlpfile->wRefCount          = 1;

    strcpy(hlpfile->lpszPath, lpszPath);

    first_hlpfile = hlpfile;
    if (hlpfile->next) hlpfile->next->prev = hlpfile;

    if (!HLPFILE_DoReadHlpFile(hlpfile, lpszPath))
    {
        HLPFILE_FreeHlpFile(hlpfile);
        hlpfile = 0;
    }

    return hlpfile;
}

/***********************************************************************
 *
 *           HLPFILE_DoReadHlpFile
 */
static BOOL HLPFILE_DoReadHlpFile(HLPFILE *hlpfile, LPCSTR lpszPath)
{
    BOOL        ret;
    HFILE       hFile;
    OFSTRUCT    ofs;
    BYTE*       buf;
    DWORD       ref = 0x0C;
    unsigned    index, old_index, offset, len, offs, topicoffset;

    hFile = OpenFile(lpszPath, &ofs, OF_READ);
    if (hFile == HFILE_ERROR) return FALSE;

    ret = HLPFILE_ReadFileToBuffer(hlpfile, hFile);
    _lclose(hFile);
    if (!ret) return FALSE;

    if (!HLPFILE_SystemCommands(hlpfile)) return FALSE;

    if (hlpfile->version <= 16 && !HLPFILE_GetTOMap(hlpfile)) return FALSE;

    /* load phrases support */
    if (!HLPFILE_UncompressLZ77_Phrases(hlpfile))
        HLPFILE_Uncompress_Phrases40(hlpfile);

    if (!HLPFILE_Uncompress_Topic(hlpfile)) return FALSE;
    if (!HLPFILE_ReadFont(hlpfile)) return FALSE;

    buf = hlpfile->topic_map[0];
    old_index = -1;
    offs = 0;
    do
    {
        BYTE*   end;

        if (hlpfile->version <= 16)
        {
            index  = (ref - 0x0C) / hlpfile->dsize;
            offset = (ref - 0x0C) % hlpfile->dsize;
        }
        else
        {
            index  = (ref - 0x0C) >> 14;
            offset = (ref - 0x0C) & 0x3FFF;
        }

        if (hlpfile->version <= 16 && index != old_index && old_index != -1)
        {
            /* we jumped to the next block, adjust pointers */
            ref -= 12;
            offset -= 12;
        }

        WINE_TRACE("ref=%08x => [%u/%u]\n", ref, index, offset);

        if (index >= hlpfile->topic_maplen) {WINE_WARN("maplen\n"); break;}
        buf = hlpfile->topic_map[index] + offset;
        if (buf + 0x15 >= hlpfile->topic_end) {WINE_WARN("extra\n"); break;}
        end = min(buf + GET_UINT(buf, 0), hlpfile->topic_end);
        if (index != old_index) {offs = 0; old_index = index;}

        switch (buf[0x14])
	{
	case 0x02:
            if (hlpfile->version <= 16)
                topicoffset = ref + index * 12;
            else
                topicoffset = index * 0x8000 + offs;
            if (!HLPFILE_AddPage(hlpfile, buf, end, ref, topicoffset)) return FALSE;
            break;

	case 0x01:
	case 0x20:
	case 0x23:
            if (!HLPFILE_SkipParagraph(hlpfile, buf, end, &len)) return FALSE;
            offs += len;
            break;

	default:
            WINE_ERR("buf[0x14] = %x\n", buf[0x14]);
	}

        if (hlpfile->version <= 16)
        {
            ref += GET_UINT(buf, 0xc);
            if (GET_UINT(buf, 0xc) == 0)
                break;
        }
        else
            ref = GET_UINT(buf, 0xc);
    } while (ref != 0xffffffff);

    HLPFILE_GetKeywords(hlpfile);
    HLPFILE_GetMap(hlpfile);
    if (hlpfile->version <= 16) return TRUE;
    return HLPFILE_GetContext(hlpfile);
}

/***********************************************************************
 *
 *           HLPFILE_AddPage
 */
static BOOL HLPFILE_AddPage(HLPFILE *hlpfile, const BYTE *buf, const BYTE *end, unsigned ref, unsigned offset)
{
    HLPFILE_PAGE* page;
    const BYTE*   title;
    UINT          titlesize, blocksize, datalen;
    char*         ptr;
    HLPFILE_MACRO*macro;

    blocksize = GET_UINT(buf, 0);
    datalen = GET_UINT(buf, 0x10);
    title = buf + datalen;
    if (title > end) {WINE_WARN("page2\n"); return FALSE;};

    titlesize = GET_UINT(buf, 4);
    page = HeapAlloc(GetProcessHeap(), 0, sizeof(HLPFILE_PAGE) + titlesize + 1);
    if (!page) return FALSE;
    page->lpszTitle = (char*)page + sizeof(HLPFILE_PAGE);

    if (titlesize > blocksize - datalen)
    {
        /* need to decompress */
        if (hlpfile->hasPhrases)
            HLPFILE_Uncompress2(hlpfile, title, end, (BYTE*)page->lpszTitle, (BYTE*)page->lpszTitle + titlesize);
        else if (hlpfile->hasPhrases40)
            HLPFILE_Uncompress3(hlpfile, page->lpszTitle, page->lpszTitle + titlesize, title, end);
        else
        {
            WINE_FIXME("Text size is too long, splitting\n");
            titlesize = blocksize - datalen;
            memcpy(page->lpszTitle, title, titlesize);
        }
    }
    else
        memcpy(page->lpszTitle, title, titlesize);

    page->lpszTitle[titlesize] = '\0';

    if (hlpfile->first_page)
    {
        hlpfile->last_page->next = page;
        page->prev = hlpfile->last_page;
        hlpfile->last_page = page;
    }
    else
    {
        hlpfile->first_page = page;
        hlpfile->last_page = page;
        page->prev = NULL;
    }

    page->file            = hlpfile;
    page->next            = NULL;
    page->first_macro     = NULL;
    page->first_link      = NULL;
    page->wNumber         = GET_UINT(buf, 0x21);
    page->offset          = offset;
    page->reference       = ref;

    page->browse_bwd = GET_UINT(buf, 0x19);
    page->browse_fwd = GET_UINT(buf, 0x1D);

    if (hlpfile->version <= 16)
    {
        if (page->browse_bwd == 0xFFFF || page->browse_bwd == 0xFFFFFFFF)
            page->browse_bwd = 0xFFFFFFFF;
        else
            page->browse_bwd = hlpfile->TOMap[page->browse_bwd];

        if (page->browse_fwd == 0xFFFF || page->browse_fwd == 0xFFFFFFFF)
            page->browse_fwd = 0xFFFFFFFF;
        else
            page->browse_fwd = hlpfile->TOMap[page->browse_fwd];
    }

    WINE_TRACE("Added page[%d]: title='%s' %08x << %08x >> %08x\n",
               page->wNumber, page->lpszTitle, 
               page->browse_bwd, page->offset, page->browse_fwd);

    /* now load macros */
    ptr = page->lpszTitle + strlen(page->lpszTitle) + 1;
    while (ptr < page->lpszTitle + titlesize)
    {
        unsigned len = strlen(ptr);
        char*    macro_str;

        WINE_TRACE("macro: %s\n", ptr);
        macro = HeapAlloc(GetProcessHeap(), 0, sizeof(HLPFILE_MACRO) + len + 1);
        macro->lpszMacro = macro_str = (char*)(macro + 1);
        memcpy(macro_str, ptr, len + 1);
        /* FIXME: shall we really link macro in reverse order ??
         * may produce strange results when played at page opening
         */
        macro->next = page->first_macro;
        page->first_macro = macro;
        ptr += len + 1;
    }

    return TRUE;
}

static long fetch_long(const BYTE** ptr)
{
    long        ret;

    if (*(*ptr) & 1)
    {
        ret = (*(const unsigned long*)(*ptr) - 0x80000000L) / 2;
        (*ptr) += 4;
    }
    else
    {
        ret = (*(const unsigned short*)(*ptr) - 0x8000) / 2;
        (*ptr) += 2;
    }

    return ret;
}

static unsigned long fetch_ulong(const BYTE** ptr)
{
    unsigned long        ret;

    if (*(*ptr) & 1)
    {
        ret = *(const unsigned long*)(*ptr) / 2;
        (*ptr) += 4;
    }
    else
    {
        ret = *(const unsigned short*)(*ptr) / 2;
        (*ptr) += 2;
    }
    return ret;
}    

static short fetch_short(const BYTE** ptr)
{
    short       ret;

    if (*(*ptr) & 1)
    {
        ret = (*(const unsigned short*)(*ptr) - 0x8000) / 2;
        (*ptr) += 2;
    }
    else
    {
        ret = (*(const unsigned char*)(*ptr) - 0x80) / 2;
        (*ptr)++;
    }
    return ret;
}

static unsigned short fetch_ushort(const BYTE** ptr)
{
    unsigned short ret;

    if (*(*ptr) & 1)
    {
        ret = *(const unsigned short*)(*ptr) / 2;
        (*ptr) += 2;
    }
    else
    {
        ret = *(const unsigned char*)(*ptr) / 2;
        (*ptr)++;
    }
    return ret;
}

/***********************************************************************
 *
 *           HLPFILE_SkipParagraph
 */
static BOOL HLPFILE_SkipParagraph(HLPFILE *hlpfile, const BYTE *buf, const BYTE *end, unsigned* len)
{
    const BYTE  *tmp;

    if (!hlpfile->first_page) {WINE_WARN("no page\n"); return FALSE;};
    if (buf + 0x19 > end) {WINE_WARN("header too small\n"); return FALSE;};

    tmp = buf + 0x15;
    if (buf[0x14] == 0x20 || buf[0x14] == 0x23)
    {
        fetch_long(&tmp);
        *len = fetch_ushort(&tmp);
    }
    else *len = end-buf-15;

    return TRUE;
}

/******************************************************************
 *		HLPFILE_DecompressGfx
 *
 * Decompress the data part of a bitmap or a metafile
 */
static const BYTE*      HLPFILE_DecompressGfx(const BYTE* src, unsigned csz, unsigned sz, BYTE packing,
                                              BYTE** alloc)
{
    const BYTE* dst;
    BYTE*       tmp;
    unsigned    sz77;

    WINE_TRACE("Unpacking (%d) from %u bytes to %u bytes\n", packing, csz, sz);

    switch (packing)
    {
    case 0: /* uncompressed */
        if (sz != csz)
            WINE_WARN("Bogus gfx sizes (uncompressed): %u / %u\n", sz, csz);
        dst = src;
        *alloc = NULL;
        break;
    case 1: /* RunLen */
        dst = *alloc = HeapAlloc(GetProcessHeap(), 0, sz);
        if (!dst) return NULL;
        HLPFILE_UncompressRLE(src, src + csz, *alloc, sz);
        break;
    case 2: /* LZ77 */
        sz77 = HLPFILE_UncompressedLZ77_Size(src, src + csz);
        dst = *alloc = HeapAlloc(GetProcessHeap(), 0, sz77);
        if (!dst) return NULL;
        HLPFILE_UncompressLZ77(src, src + csz, *alloc);
        if (sz77 != sz)
            WINE_WARN("Bogus gfx sizes (LZ77): %u / %u\n", sz77, sz);
        break;
    case 3: /* LZ77 then RLE */
        sz77 = HLPFILE_UncompressedLZ77_Size(src, src + csz);
        tmp = HeapAlloc(GetProcessHeap(), 0, sz77);
        if (!tmp) return FALSE;
        HLPFILE_UncompressLZ77(src, src + csz, tmp);
        dst = *alloc = HeapAlloc(GetProcessHeap(), 0, sz);
        if (!dst)
        {
            HeapFree(GetProcessHeap(), 0, tmp);
            return FALSE;
        }
        HLPFILE_UncompressRLE(tmp, tmp + sz77, *alloc, sz);
        HeapFree(GetProcessHeap(), 0, tmp);
        break;
    default:
        WINE_FIXME("Unsupported packing %u\n", packing);
        return NULL;
    }
    return dst;
}

static BOOL HLPFILE_RtfAddRawString(struct RtfData* rd, const char* str, size_t sz)
{
    if (rd->ptr + sz >= rd->data + rd->allocated)
    {
        char*   new = HeapReAlloc(GetProcessHeap(), 0, rd->data, rd->allocated *= 2);
        if (!new) return FALSE;
        rd->ptr = new + (rd->ptr - rd->data);
        rd->data = new;
    }
    memcpy(rd->ptr, str, sz);
    rd->ptr += sz;

    return TRUE;
}

static BOOL HLPFILE_RtfAddControl(struct RtfData* rd, const char* str)
{
    if (*str == '\\' || *str == '{') rd->in_text = FALSE;
    else if (*str == '}') rd->in_text = TRUE;
    return HLPFILE_RtfAddRawString(rd, str, strlen(str));
}

static BOOL HLPFILE_RtfAddText(struct RtfData* rd, const char* str)
{
    const char* p;
    const char* last;
    const char* replace;
    unsigned    rlen;

    if (!rd->in_text)
    {
        if (!HLPFILE_RtfAddRawString(rd, " ", 1)) return FALSE;
        rd->in_text = TRUE;
    }
    for (last = p = str; *p; p++)
    {
        if (*p < 0) /* escape non ASCII chars */
        {
            static char         xx[8];
            rlen = sprintf(xx, "\\'%x", *(const BYTE*)p);
            replace = xx;
        }
        else switch (*p)
        {
        case '{':  rlen = 2; replace = "\\{";  break;
        case '}':  rlen = 2; replace = "\\}";  break;
        case '\\': rlen = 2; replace = "\\\\"; break;
        default:   continue;
        }
        if ((p != last && !HLPFILE_RtfAddRawString(rd, last, p - last)) ||
            !HLPFILE_RtfAddRawString(rd, replace, rlen)) return FALSE;
        last = p + 1;
    }
    return HLPFILE_RtfAddRawString(rd, last, p - last);
}

/******************************************************************
 *		RtfAddHexBytes
 *
 */
static BOOL HLPFILE_RtfAddHexBytes(struct RtfData* rd, const void* _ptr, unsigned sz)
{
    char        tmp[512];
    unsigned    i, step;
    const BYTE* ptr = _ptr;
    static const char* _2hex = "0123456789abcdef";

    if (!rd->in_text)
    {
        if (!HLPFILE_RtfAddRawString(rd, " ", 1)) return FALSE;
        rd->in_text = TRUE;
    }
    for (; sz; sz -= step)
    {
        step = min(256, sz);
        for (i = 0; i < step; i++)
        {
            tmp[2 * i + 0] = _2hex[*ptr >> 4];
            tmp[2 * i + 1] = _2hex[*ptr++ & 0xF];
        }
        if (!HLPFILE_RtfAddRawString(rd, tmp, 2 * step)) return FALSE;
    }
    return TRUE;
}

/******************************************************************
 *             HLPFILE_RtfAddTransparentBitmap
 *
 * We'll transform a transparent bitmap into an metafile that
 * we then transform into RTF
 */
static BOOL HLPFILE_RtfAddTransparentBitmap(struct RtfData* rd, const BITMAPINFO* bi,
                                            const void* pict, unsigned nc)
{
    HDC                 hdc, hdcMask, hdcMem, hdcEMF;
    HBITMAP             hbm, hbmMask, hbmOldMask, hbmOldMem;
    HENHMETAFILE        hEMF;
    BOOL                ret = FALSE;
    void*               data;
    UINT                sz;

    hbm = CreateDIBitmap(hdc = GetDC(0), &bi->bmiHeader,
                         CBM_INIT, pict, bi, DIB_RGB_COLORS);

    hdcMem = CreateCompatibleDC(hdc);
    hbmOldMem = SelectObject(hdcMem, hbm);

    /* create the mask bitmap from the main bitmap */
    hdcMask = CreateCompatibleDC(hdc);
    hbmMask = CreateBitmap(bi->bmiHeader.biWidth, bi->bmiHeader.biHeight, 1, 1, NULL);
    hbmOldMask = SelectObject(hdcMask, hbmMask);
    SetBkColor(hdcMem,
               RGB(bi->bmiColors[nc - 1].rgbRed,
                   bi->bmiColors[nc - 1].rgbGreen,
                   bi->bmiColors[nc - 1].rgbBlue));
    BitBlt(hdcMask, 0, 0, bi->bmiHeader.biWidth, bi->bmiHeader.biHeight, hdcMem, 0, 0, SRCCOPY);

    /* sets to RGB(0,0,0) the transparent bits in main bitmap */
    SetBkColor(hdcMem, RGB(0,0,0));
    SetTextColor(hdcMem, RGB(255,255,255));
    BitBlt(hdcMem, 0, 0, bi->bmiHeader.biWidth, bi->bmiHeader.biHeight, hdcMask, 0, 0, SRCAND);

    SelectObject(hdcMask, hbmOldMask);
    DeleteDC(hdcMask);

    SelectObject(hdcMem, hbmOldMem);
    DeleteDC(hdcMem);

    /* we create the bitmap on the fly */
    hdcEMF = CreateEnhMetaFile(NULL, NULL, NULL, NULL);
    hdcMem = CreateCompatibleDC(hdcEMF);

    /* sets to RGB(0,0,0) the transparent bits in final bitmap */
    hbmOldMem = SelectObject(hdcMem, hbmMask);
    SetBkColor(hdcEMF, RGB(255, 255, 255));
    SetTextColor(hdcEMF, RGB(0, 0, 0));
    BitBlt(hdcEMF, 0, 0, bi->bmiHeader.biWidth, bi->bmiHeader.biHeight, hdcMem, 0, 0, SRCAND);

    /* and copy the remaining bits of main bitmap */
    SelectObject(hdcMem, hbm);
    BitBlt(hdcEMF, 0, 0, bi->bmiHeader.biWidth, bi->bmiHeader.biHeight, hdcMem, 0, 0, SRCPAINT);
    SelectObject(hdcMem, hbmOldMem);
    DeleteDC(hdcMem);

    /* do the cleanup */
    ReleaseDC(0, hdc);
    DeleteObject(hbmMask);
    DeleteObject(hbm);

    hEMF = CloseEnhMetaFile(hdcEMF);

    /* generate rtf stream */
    sz = GetEnhMetaFileBits(hEMF, 0, NULL);
    if (sz && (data = HeapAlloc(GetProcessHeap(), 0, sz)))
    {
        if (sz == GetEnhMetaFileBits(hEMF, sz, data))
        {
            ret = HLPFILE_RtfAddControl(rd, "{\\pict\\emfblip") &&
                HLPFILE_RtfAddHexBytes(rd, data, sz) &&
                HLPFILE_RtfAddControl(rd, "}");
        }
        HeapFree(GetProcessHeap(), 0, data);
    }
    DeleteEnhMetaFile(hEMF);

    return ret;
}

/******************************************************************
 *		HLPFILE_RtfAddBitmap
 *
 */
static BOOL HLPFILE_RtfAddBitmap(struct RtfData* rd, const BYTE* beg, BYTE type, BYTE pack)
{
    const BYTE*         ptr;
    const BYTE*         pict_beg;
    BYTE*               alloc = NULL;
    BITMAPINFO*         bi;
    unsigned long       off, csz;
    unsigned            nc = 0;
    BOOL                clrImportant = FALSE;
    BOOL                ret = FALSE;
    char                tmp[256];

    bi = HeapAlloc(GetProcessHeap(), 0, sizeof(*bi));
    if (!bi) return FALSE;

    ptr = beg + 2; /* for type and pack */

    bi->bmiHeader.biSize          = sizeof(bi->bmiHeader);
    bi->bmiHeader.biXPelsPerMeter = fetch_ulong(&ptr);
    bi->bmiHeader.biYPelsPerMeter = fetch_ulong(&ptr);
    bi->bmiHeader.biPlanes        = fetch_ushort(&ptr);
    bi->bmiHeader.biBitCount      = fetch_ushort(&ptr);
    bi->bmiHeader.biWidth         = fetch_ulong(&ptr);
    bi->bmiHeader.biHeight        = fetch_ulong(&ptr);
    bi->bmiHeader.biClrUsed       = fetch_ulong(&ptr);
    clrImportant  = fetch_ulong(&ptr);
    bi->bmiHeader.biClrImportant  = (clrImportant > 1) ? clrImportant : 0;
    bi->bmiHeader.biCompression   = BI_RGB;
    if (bi->bmiHeader.biBitCount > 32) WINE_FIXME("Unknown bit count %u\n", bi->bmiHeader.biBitCount);
    if (bi->bmiHeader.biPlanes != 1) WINE_FIXME("Unsupported planes %u\n", bi->bmiHeader.biPlanes);
    bi->bmiHeader.biSizeImage = (((bi->bmiHeader.biWidth * bi->bmiHeader.biBitCount + 31) & ~31) / 8) * bi->bmiHeader.biHeight;
    WINE_TRACE("planes=%d bc=%d size=(%d,%d)\n",
               bi->bmiHeader.biPlanes, bi->bmiHeader.biBitCount,
               bi->bmiHeader.biWidth, bi->bmiHeader.biHeight);

    csz = fetch_ulong(&ptr);
    fetch_ulong(&ptr); /* hotspot size */

    off = GET_UINT(ptr, 0);     ptr += 4;
    /* GET_UINT(ptr, 0); hotspot offset */ ptr += 4;

    /* now read palette info */
    if (type == 0x06)
    {
        unsigned i;

        nc = bi->bmiHeader.biClrUsed;
        /* not quite right, especially for bitfields type of compression */
        if (!nc && bi->bmiHeader.biBitCount <= 8)
            nc = 1 << bi->bmiHeader.biBitCount;

        bi = HeapReAlloc(GetProcessHeap(), 0, bi, sizeof(*bi) + nc * sizeof(RGBQUAD));
        if (!bi) return FALSE;
        for (i = 0; i < nc; i++)
        {
            bi->bmiColors[i].rgbBlue     = ptr[0];
            bi->bmiColors[i].rgbGreen    = ptr[1];
            bi->bmiColors[i].rgbRed      = ptr[2];
            bi->bmiColors[i].rgbReserved = 0;
            ptr += 4;
        }
    }
    pict_beg = HLPFILE_DecompressGfx(beg + off, csz, bi->bmiHeader.biSizeImage, pack, &alloc);

    if (clrImportant == 1 && nc > 0)
    {
        ret = HLPFILE_RtfAddTransparentBitmap(rd, bi, pict_beg, nc);
        goto done;
    }
    if (!HLPFILE_RtfAddControl(rd, "{\\pict")) goto done;
    if (type == 0x06)
    {
        sprintf(tmp, "\\dibitmap0\\picw%d\\pich%d",
                bi->bmiHeader.biWidth, bi->bmiHeader.biHeight);
        if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
        if (!HLPFILE_RtfAddHexBytes(rd, bi, sizeof(*bi) + nc * sizeof(RGBQUAD))) goto done;
    }
    else
    {
        sprintf(tmp, "\\wbitmap0\\wbmbitspixel%d\\wbmplanes%d\\picw%d\\pich%d",
                bi->bmiHeader.biBitCount, bi->bmiHeader.biPlanes,
                bi->bmiHeader.biWidth, bi->bmiHeader.biHeight);
        if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
    }
    if (!HLPFILE_RtfAddHexBytes(rd, pict_beg, bi->bmiHeader.biSizeImage)) goto done;
    if (!HLPFILE_RtfAddControl(rd, "}")) goto done;

    ret = TRUE;
done:
    HeapFree(GetProcessHeap(), 0, bi);
    HeapFree(GetProcessHeap(), 0, alloc);

    return ret;
}

/******************************************************************
 *		HLPFILE_RtfAddMetaFile
 *
 */
static BOOL     HLPFILE_RtfAddMetaFile(struct RtfData* rd, const BYTE* beg, BYTE pack)
{
    const BYTE*         ptr;
    unsigned long       size, csize;
    unsigned long       off, hsoff;
    const BYTE*         bits;
    BYTE*               alloc = NULL;
    char                tmp[256];
    unsigned            mm;
    BOOL                ret;

    WINE_TRACE("Loading metafile\n");

    ptr = beg + 2; /* for type and pack */

    mm = fetch_ushort(&ptr); /* mapping mode */
    sprintf(tmp, "{\\pict\\wmetafile%d\\picw%d\\pich%d",
            mm, GET_USHORT(ptr, 0), GET_USHORT(ptr, 2));
    if (!HLPFILE_RtfAddControl(rd, tmp)) return FALSE;
    ptr += 4;

    size = fetch_ulong(&ptr); /* decompressed size */
    csize = fetch_ulong(&ptr); /* compressed size */
    fetch_ulong(&ptr); /* hotspot size */
    off = GET_UINT(ptr, 0);
    hsoff = GET_UINT(ptr, 4);
    ptr += 8;

    WINE_TRACE("sz=%lu csz=%lu offs=%lu/%u,%lu\n",
               size, csize, off, ptr - beg, hsoff);

    bits = HLPFILE_DecompressGfx(beg + off, csize, size, pack, &alloc);
    if (!bits) return FALSE;

    ret = HLPFILE_RtfAddHexBytes(rd, bits, size) &&
        HLPFILE_RtfAddControl(rd, "}");

    HeapFree(GetProcessHeap(), 0, alloc);

    return ret;
}

/******************************************************************
 *		HLPFILE_RtfAddGfxByAddr
 *
 */
static  BOOL    HLPFILE_RtfAddGfxByAddr(struct RtfData* rd, HLPFILE *hlpfile,
                                        const BYTE* ref, unsigned long size)
{
    unsigned    i, numpict;

    numpict = GET_USHORT(ref, 2);
    WINE_TRACE("Got picture magic=%04x #=%d\n", GET_USHORT(ref, 0), numpict);

    for (i = 0; i < numpict; i++)
    {
        const BYTE*     beg;
        const BYTE*     ptr;
        BYTE            type, pack;

        WINE_TRACE("Offset[%d] = %x\n", i, GET_UINT(ref, (1 + i) * 4));
        beg = ptr = ref + GET_UINT(ref, (1 + i) * 4);

        type = *ptr++;
        pack = *ptr++;

        switch (type)
        {
        case 5: /* device dependent bmp */
        case 6: /* device independent bmp */
            HLPFILE_RtfAddBitmap(rd, beg, type, pack);
            break;
        case 8:
            HLPFILE_RtfAddMetaFile(rd, beg, pack);
            break;
        default: WINE_FIXME("Unknown type %u\n", type); return FALSE;
        }

        /* FIXME: hotspots */

        /* FIXME: implement support for multiple picture format */
        if (numpict != 1) WINE_FIXME("Supporting only one bitmap format per logical bitmap (for now). Using first format\n");
        break;
    }
    return TRUE;
}

/******************************************************************
 *		HLPFILE_RtfAddGfxByIndex
 *
 *
 */
static  BOOL    HLPFILE_RtfAddGfxByIndex(struct RtfData* rd, HLPFILE *hlpfile,
                                         unsigned index)
{
    char        tmp[16];
    BYTE        *ref, *end;

    WINE_TRACE("Loading picture #%d\n", index);

    sprintf(tmp, "|bm%u", index);

    if (!HLPFILE_FindSubFile(hlpfile, tmp, &ref, &end)) {WINE_WARN("no sub file\n"); return FALSE;}

    ref += 9;
    return HLPFILE_RtfAddGfxByAddr(rd, hlpfile, ref, end - ref);
}

/******************************************************************
 *		HLPFILE_AllocLink
 *
 *
 */
static HLPFILE_LINK*       HLPFILE_AllocLink(struct RtfData* rd, int cookie,
                                             const char* str, unsigned len, LONG hash,
                                             unsigned clrChange, unsigned wnd)
{
    HLPFILE_LINK*  link;
    char*          link_str;

    /* FIXME: should build a string table for the attributes.link.lpszPath
     * they are reallocated for each link
     */
    if (len == -1) len = strlen(str);
    link = HeapAlloc(GetProcessHeap(), 0, sizeof(HLPFILE_LINK) + len + 1);
    if (!link) return NULL;

    link->cookie     = cookie;
    link->string     = link_str = (char*)(link + 1);
    memcpy(link_str, str, len);
    link_str[len] = '\0';
    link->hash       = hash;
    link->bClrChange = clrChange ? 1 : 0;
    link->window     = wnd;
    link->next       = rd->first_link;
    rd->first_link   = link;
    link->cpMin      = rd->char_pos;
    link->cpMax      = 0;
    rd->force_color  = clrChange;
    if (rd->current_link) WINE_FIXME("Pending link\n");
    rd->current_link = link;

    WINE_TRACE("Link[%d] to %s@%08x:%d\n",
               link->cookie, link->string, link->hash, link->window);
    return link;
}

unsigned HLPFILE_HalfPointsToTwips(unsigned pts)
{
    static unsigned logPxY;
    if (!logPxY)
    {
        HDC hdc = GetDC(NULL);
        logPxY = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);
    }
    return MulDiv(pts, 72 * 10, logPxY);
}

/***********************************************************************
 *
 *           HLPFILE_BrowseParagraph
 */
static BOOL HLPFILE_BrowseParagraph(HLPFILE_PAGE* page, struct RtfData* rd,
                                    BYTE *buf, BYTE* end, unsigned* parlen)
{
    UINT               textsize;
    const BYTE        *format, *format_end;
    char              *text, *text_base, *text_end;
    long               size, blocksize, datalen;
    unsigned short     bits;
    unsigned           nc, ncol = 1;
    short              table_width;
    BOOL               in_table = FALSE;
    char               tmp[256];
    BOOL               ret = FALSE;

    if (buf + 0x19 > end) {WINE_WARN("header too small\n"); return FALSE;};

    *parlen = 0;
    blocksize = GET_UINT(buf, 0);
    size = GET_UINT(buf, 0x4);
    datalen = GET_UINT(buf, 0x10);
    text = text_base = HeapAlloc(GetProcessHeap(), 0, size);
    if (!text) return FALSE;
    if (size > blocksize - datalen)
    {
        /* need to decompress */
        if (page->file->hasPhrases)
            HLPFILE_Uncompress2(page->file, buf + datalen, end, (BYTE*)text, (BYTE*)text + size);
        else if (page->file->hasPhrases40)
            HLPFILE_Uncompress3(page->file, text, text + size, buf + datalen, end);
        else
        {
            WINE_FIXME("Text size is too long, splitting\n");
            size = blocksize - datalen;
            memcpy(text, buf + datalen, size);
        }
    }
    else
        memcpy(text, buf + datalen, size);

    text_end = text + size;

    format = buf + 0x15;
    format_end = buf + GET_UINT(buf, 0x10);

    if (buf[0x14] == 0x20 || buf[0x14] == 0x23)
    {
        fetch_long(&format);
        *parlen = fetch_ushort(&format);
    }

    if (buf[0x14] == 0x23)
    {
        char    type;

        in_table = TRUE;
        ncol = *format++;

        if (!HLPFILE_RtfAddControl(rd, "\\trowd")) goto done;
        type = *format++;
        if (type == 0 || type == 2)
        {
            table_width = GET_SHORT(format, 0);
            format += 2;
        }
        else
            table_width = 32767;
        WINE_TRACE("New table: cols=%d type=%x width=%d\n",
                   ncol, type, table_width);
        if (ncol > 1)
        {
            int     pos;
            sprintf(tmp, "\\trgaph%d\\trleft%d",
                    HLPFILE_HalfPointsToTwips(MulDiv(GET_SHORT(format, 6), table_width, 32767)),
                    HLPFILE_HalfPointsToTwips(MulDiv(GET_SHORT(format, 0), table_width, 32767)));
            if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
            pos = HLPFILE_HalfPointsToTwips(MulDiv(GET_SHORT(format, 6) / 2, table_width, 32767));
            for (nc = 0; nc < ncol; nc++)
            {
                WINE_TRACE("column(%d/%d) gap=%d width=%d\n",
                           nc, ncol, GET_SHORT(format, nc*4),
                           GET_SHORT(format, nc*4+2));
                pos += GET_SHORT(format, nc * 4) + GET_SHORT(format, nc * 4 + 2);
                sprintf(tmp, "\\cellx%d",
                        HLPFILE_HalfPointsToTwips(MulDiv(pos, table_width, 32767)));
                if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
            }
        }
        else
        {
            WINE_TRACE("column(0/%d) gap=%d width=%d\n",
                       ncol, GET_SHORT(format, 0), GET_SHORT(format, 2));
            sprintf(tmp, "\\trleft%d\\cellx%d ",
                    HLPFILE_HalfPointsToTwips(MulDiv(GET_SHORT(format, 0), table_width, 32767)),
                    HLPFILE_HalfPointsToTwips(MulDiv(GET_SHORT(format, 0) + GET_SHORT(format, 2),
                                      table_width, 32767)));
            if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
        }
        format += ncol * 4;
    }

    for (nc = 0; nc < ncol; /**/)
    {
        WINE_TRACE("looking for format at offset %lu in column %d\n", (SIZE_T)(format - (buf + 0x15)), nc);
        if (!HLPFILE_RtfAddControl(rd, "\\pard")) goto done;
        if (in_table)
        {
            nc = GET_SHORT(format, 0);
            if (nc == -1) break;
            format += 5;
            if (!HLPFILE_RtfAddControl(rd, "\\intbl")) goto done;
        }
        else nc++;
        if (buf[0x14] == 0x01)
            format += 6;
        else
            format += 4;
        bits = GET_USHORT(format, 0); format += 2;
        if (bits & 0x0001) fetch_long(&format);
        if (bits & 0x0002)
        {
            sprintf(tmp, "\\sb%d", HLPFILE_HalfPointsToTwips(fetch_short(&format)));
            if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
        }
        if (bits & 0x0004)
        {
            sprintf(tmp, "\\sa%d", HLPFILE_HalfPointsToTwips(fetch_short(&format)));
            if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
        }
        if (bits & 0x0008)
        {
            sprintf(tmp, "\\sl%d", HLPFILE_HalfPointsToTwips(fetch_short(&format)));
            if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
        }
        if (bits & 0x0010)
        {
            sprintf(tmp, "\\li%d", HLPFILE_HalfPointsToTwips(fetch_short(&format)));
            if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
        }
        if (bits & 0x0020)
        {
            sprintf(tmp, "\\ri%d", HLPFILE_HalfPointsToTwips(fetch_short(&format)));
            if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
        }
        if (bits & 0x0040)
        {
            sprintf(tmp, "\\fi%d", HLPFILE_HalfPointsToTwips(fetch_short(&format)));
            if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
        }
        if (bits & 0x0100)
        {
            BYTE        brdr = *format++;
            short       w;

            if (brdr & 0x01 && !HLPFILE_RtfAddControl(rd, "\\box")) goto done;
            if (brdr & 0x02 && !HLPFILE_RtfAddControl(rd, "\\brdrt")) goto done;
            if (brdr & 0x04 && !HLPFILE_RtfAddControl(rd, "\\brdrl")) goto done;
            if (brdr & 0x08 && !HLPFILE_RtfAddControl(rd, "\\brdrb")) goto done;
            if (brdr & 0x10 && !HLPFILE_RtfAddControl(rd, "\\brdrr")) goto done;
            if (brdr & 0x20 && !HLPFILE_RtfAddControl(rd, "\\brdrth")) goto done;
            if (!(brdr & 0x20) && !HLPFILE_RtfAddControl(rd, "\\brdrs")) goto done;
            if (brdr & 0x40 && !HLPFILE_RtfAddControl(rd, "\\brdrdb")) goto done;
            /* 0x80: unknown */

            w = GET_SHORT(format, 0); format += 2;
            if (w)
            {
                sprintf(tmp, "\\brdrw%d", HLPFILE_HalfPointsToTwips(w));
                if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
            }
        }
        if (bits & 0x0200)
        {
            int                 i, ntab = fetch_short(&format);
            unsigned            tab, ts;
            const char*         kind;

            for (i = 0; i < ntab; i++)
            {
                tab = fetch_ushort(&format);
                ts = (tab & 0x4000) ? fetch_ushort(&format) : 0 /* left */;
                switch (ts)
                {
                default: WINE_FIXME("Unknown tab style %x\n", ts);
                /* fall through */
                case 0: kind = ""; break;
                case 1: kind = "\\tqr"; break;
                case 2: kind = "\\tqc"; break;
                }
                /* FIXME: do kind */
                sprintf(tmp, "%s\\tx%d",
                        kind, HLPFILE_HalfPointsToTwips(tab & 0x3FFF));
                if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
            }
        }
        switch (bits & 0xc00)
        {
        default: WINE_FIXME("Unsupported alignment 0xC00\n"); break;
        case 0: if (!HLPFILE_RtfAddControl(rd, "\\ql")) goto done; break;
        case 0x400: if (!HLPFILE_RtfAddControl(rd, "\\qr")) goto done; break;
        case 0x800: if (!HLPFILE_RtfAddControl(rd, "\\qc")) goto done; break;
        }

        /* 0x1000 doesn't need space */
        if ((bits & 0x1000) && !HLPFILE_RtfAddControl(rd, "\\keep")) goto done;
        if ((bits & 0xE080) != 0) 
            WINE_FIXME("Unsupported bits %04x, potential trouble ahead\n", bits);

        while (text < text_end && format < format_end)
        {
            WINE_TRACE("Got text: %s (%p/%p - %p/%p)\n", wine_dbgstr_a(text), text, text_end, format, format_end);
            textsize = strlen(text);
            if (textsize)
            {
                if (rd->force_color)
                {
                    if ((rd->current_link->cookie == hlp_link_popup) ?
                        !HLPFILE_RtfAddControl(rd, "{\\uld\\cf1") :
                        !HLPFILE_RtfAddControl(rd, "{\\ul\\cf1")) goto done;
                }
                if (!HLPFILE_RtfAddText(rd, text)) goto done;
                if (rd->force_color && !HLPFILE_RtfAddControl(rd, "}")) goto done;
                rd->char_pos += textsize;
            }
            /* else: null text, keep on storing attributes */
            text += textsize + 1;

	    if (*format == 0xff)
            {
                format++;
                break;
            }

            WINE_TRACE("format=%02x\n", *format);
            switch (*format)
            {
            case 0x20:
                WINE_FIXME("NIY20\n");
                format += 5;
                break;

            case 0x21:
                WINE_FIXME("NIY21\n");
                format += 3;
                break;

	    case 0x80:
                {
                    unsigned    font = GET_USHORT(format, 1);
                    unsigned    fs;

                    WINE_TRACE("Changing font to %d\n", font);
                    format += 3;
                    /* Font size in hlpfile is given in the same units as
                       rtf control word \fs uses (half-points). */
                    switch (rd->font_scale)
                    {
                    case 0: fs = page->file->fonts[font].LogFont.lfHeight - 4; break;
                    default:
                    case 1: fs = page->file->fonts[font].LogFont.lfHeight; break;
                    case 2: fs = page->file->fonts[font].LogFont.lfHeight + 4; break;
                    }
                    /* FIXME: missing at least colors, also bold attribute looses information */

                    sprintf(tmp, "\\f%d\\cf%d\\fs%d%s%s%s%s",
                            font, font + 2, fs,
                            page->file->fonts[font].LogFont.lfWeight > 400 ? "\\b" : "\\b0",
                            page->file->fonts[font].LogFont.lfItalic ? "\\i" : "\\i0",
                            page->file->fonts[font].LogFont.lfUnderline ? "\\ul" : "\\ul0",
                            page->file->fonts[font].LogFont.lfStrikeOut ? "\\strike" : "\\strike0");
                    if (!HLPFILE_RtfAddControl(rd, tmp)) goto done;
                }
               break;

	    case 0x81:
                if (!HLPFILE_RtfAddControl(rd, "\\line")) goto done;
                format += 1;
                rd->char_pos++;
                break;

	    case 0x82:
                if (in_table)
                {
                    if (format[1] != 0xFF)
                    {
                        if (!HLPFILE_RtfAddControl(rd, "\\par\\intbl")) goto done;
                    }
                    else
                    {
                        if (!HLPFILE_RtfAddControl(rd, "\\cell\\pard\\intbl")) goto done;
                    }
                }
                else if (!HLPFILE_RtfAddControl(rd, "\\par")) goto done;
                format += 1;
                rd->char_pos++;
                break;

	    case 0x83:
                if (!HLPFILE_RtfAddControl(rd, "\\tab")) goto done;
                format += 1;
                rd->char_pos++;
                break;

#if 0
	    case 0x84:
                format += 3;
                break;
#endif

	    case 0x86:
	    case 0x87:
	    case 0x88:
                {
                    BYTE    type = format[1];
                    long    size;

                    /* FIXME: we don't use 'BYTE    pos = (*format - 0x86);' for the image position */
                    format += 2;
                    size = fetch_long(&format);

                    switch (type)
                    {
                    case 0x22:
                        fetch_ushort(&format); /* hot spot */
                        /* fall thru */
                    case 0x03:
                        switch (GET_SHORT(format, 0))
                        {
                        case 0:
                            HLPFILE_RtfAddGfxByIndex(rd, page->file, GET_SHORT(format, 2));
                            rd->char_pos++;
                            break;
                        case 1:
                            WINE_FIXME("does it work ??? %x<%lu>#%u\n",
                                       GET_SHORT(format, 0),
                                       size, GET_SHORT(format, 2));
                            HLPFILE_RtfAddGfxByAddr(rd, page->file, format + 2, size - 4);
                            rd->char_pos++;
                           break;
                        default:
                            WINE_FIXME("??? %u\n", GET_SHORT(format, 0));
                            break;
                        }
                        break;
                    case 0x05:
                        WINE_FIXME("Got an embedded element %s\n", format + 6);
                        break;
                    default:
                        WINE_FIXME("Got a type %d picture\n", type);
                        break;
                    }
                    format += size;
                }
                break;

	    case 0x89:
                format += 1;
                if (!rd->current_link)
                    WINE_FIXME("No existing link\n");
                rd->current_link->cpMax = rd->char_pos;
                rd->current_link = NULL;
                rd->force_color = FALSE;
                break;

            case 0x8B:
                if (!HLPFILE_RtfAddControl(rd, "\\~")) goto done;
                format += 1;
                rd->char_pos++;
                break;

            case 0x8C:
                if (!HLPFILE_RtfAddControl(rd, "\\_")) goto done;
                /* FIXME: it could be that hypen is also in input stream !! */
                format += 1;
                rd->char_pos++;
                break;

#if 0
	    case 0xA9:
                format += 2;
                break;
#endif

            case 0xC8:
            case 0xCC:
                WINE_TRACE("macro => %s\n", format + 3);
                HLPFILE_AllocLink(rd, hlp_link_macro, (const char*)format + 3,
                                  GET_USHORT(format, 1), 0, !(*format & 4), -1);
                format += 3 + GET_USHORT(format, 1);
                break;

            case 0xE0:
            case 0xE1:
                WINE_WARN("jump topic 1 => %u\n", GET_UINT(format, 1));
                HLPFILE_AllocLink(rd, (*format & 1) ? hlp_link_link : hlp_link_popup,
                                  page->file->lpszPath, -1, GET_UINT(format, 1), 1, -1);


                format += 5;
                break;

	    case 0xE2:
	    case 0xE3:
            case 0xE6:
            case 0xE7:
                HLPFILE_AllocLink(rd, (*format & 1) ? hlp_link_link : hlp_link_popup,
                                  page->file->lpszPath, -1, GET_UINT(format, 1),
                                  !(*format & 4), -1);
                format += 5;
                break;

	    case 0xEA:
            case 0xEB:
            case 0xEE:
            case 0xEF:
                {
                    char*       ptr = (char*) format + 8;
                    BYTE        type = format[3];
                    int         wnd = -1;

                    switch (type)
                    {
                    case 1:
                        wnd = *ptr;
                        /* fall through */
                    case 0:
                        ptr = page->file->lpszPath;
                        break;
                    case 6:
                        for (wnd = page->file->numWindows - 1; wnd >= 0; wnd--)
                        {
                            if (!strcmp(ptr, page->file->windows[wnd].name)) break;
                        }
                        if (wnd == -1)
                            WINE_WARN("Couldn't find window info for %s\n", ptr);
                        ptr += strlen(ptr) + 1;
                        /* fall through */
                    case 4:
                        break;
                    default:
                        WINE_WARN("Unknown link type %d\n", type);
                        break;
                    }
                    HLPFILE_AllocLink(rd, (*format & 1) ? hlp_link_link : hlp_link_popup,
                                      ptr, -1, GET_UINT(format, 4), !(*format & 4), wnd);
                }
                format += 3 + GET_USHORT(format, 1);
                break;

	    default:
                WINE_WARN("format %02x\n", *format);
                format++;
	    }
	}
    }
    if (in_table)
    {
        if (!HLPFILE_RtfAddControl(rd, "\\row\\par\\pard\\plain")) goto done;
        rd->char_pos += 2;
    }
    ret = TRUE;
done:

    HeapFree(GetProcessHeap(), 0, text_base);
    return ret;
}

/******************************************************************
 *		HLPFILE_BrowsePage
 *
 */
BOOL    HLPFILE_BrowsePage(HLPFILE_PAGE* page, struct RtfData* rd,
                           unsigned font_scale, unsigned relative)
{
    HLPFILE     *hlpfile = page->file;
    BYTE        *buf, *end;
    DWORD       ref = page->reference;
    unsigned    index, old_index = -1, offset, count = 0, offs = 0;
    unsigned    cpg, parlen;
    char        tmp[1024];
    const char* ck = NULL;

    rd->in_text = TRUE;
    rd->data = rd->ptr = HeapAlloc(GetProcessHeap(), 0, rd->allocated = 32768);
    rd->char_pos = 0;
    rd->first_link = rd->current_link = NULL;
    rd->force_color = FALSE;
    rd->font_scale = font_scale;
    rd->relative = relative;
    rd->char_pos_rel = 0;

    switch (hlpfile->charset)
    {
    case DEFAULT_CHARSET:
    case ANSI_CHARSET:          cpg = 1252; break;
    case SHIFTJIS_CHARSET:      cpg = 932; break;
    case HANGEUL_CHARSET:       cpg = 949; break;
    case GB2312_CHARSET:        cpg = 936; break;
    case CHINESEBIG5_CHARSET:   cpg = 950; break;
    case GREEK_CHARSET:         cpg = 1253; break;
    case TURKISH_CHARSET:       cpg = 1254; break;
    case HEBREW_CHARSET:        cpg = 1255; break;
    case ARABIC_CHARSET:        cpg = 1256; break;
    case BALTIC_CHARSET:        cpg = 1257; break;
    case VIETNAMESE_CHARSET:    cpg = 1258; break;
    case RUSSIAN_CHARSET:       cpg = 1251; break;
    case EE_CHARSET:            cpg = 1250; break;
    case THAI_CHARSET:          cpg = 874; break;
    case JOHAB_CHARSET:         cpg = 1361; break;
    case MAC_CHARSET:           ck = "mac"; break;
    default:
        WINE_FIXME("Unsupported charset %u\n", hlpfile->charset);
        cpg = 1252;
    }
    if (ck)
    {
        sprintf(tmp, "{\\rtf1\\%s\\deff0", ck);
        if (!HLPFILE_RtfAddControl(rd, tmp)) return FALSE;
    }
    else
    {
        sprintf(tmp, "{\\rtf1\\ansi\\ansicpg%d\\deff0", cpg);
        if (!HLPFILE_RtfAddControl(rd, tmp)) return FALSE;
    }

    /* generate font table */
    if (!HLPFILE_RtfAddControl(rd, "{\\fonttbl")) return FALSE;
    for (index = 0; index < hlpfile->numFonts; index++)
    {
        const char* family;
        switch (hlpfile->fonts[index].LogFont.lfPitchAndFamily & 0xF0)
        {
        case FF_MODERN:     family = "modern";  break;
        case FF_ROMAN:      family = "roman";   break;
        case FF_SWISS:      family = "swiss";   break;
        case FF_SCRIPT:     family = "script";  break;
        case FF_DECORATIVE: family = "decor";   break;
        default:            family = "nil";     break;
        }
        sprintf(tmp, "{\\f%d\\f%s\\fprq%d\\fcharset%d %s;}",
                index, family,
                hlpfile->fonts[index].LogFont.lfPitchAndFamily & 0x0F,
                hlpfile->fonts[index].LogFont.lfCharSet,
                hlpfile->fonts[index].LogFont.lfFaceName);
        if (!HLPFILE_RtfAddControl(rd, tmp)) return FALSE;
    }
    if (!HLPFILE_RtfAddControl(rd, "}")) return FALSE;
    /* generate color table */
    if (!HLPFILE_RtfAddControl(rd, "{\\colortbl ;\\red0\\green128\\blue0;")) return FALSE;
    for (index = 0; index < hlpfile->numFonts; index++)
    {
        const char* family;
        switch (hlpfile->fonts[index].LogFont.lfPitchAndFamily & 0xF0)
        {
        case FF_MODERN:     family = "modern";  break;
        case FF_ROMAN:      family = "roman";   break;
        case FF_SWISS:      family = "swiss";   break;
        case FF_SCRIPT:     family = "script";  break;
        case FF_DECORATIVE: family = "decor";   break;
        default:            family = "nil";     break;
        }
        sprintf(tmp, "\\red%d\\green%d\\blue%d;",
                GetRValue(hlpfile->fonts[index].color),
                GetGValue(hlpfile->fonts[index].color),
                GetBValue(hlpfile->fonts[index].color));
        if (!HLPFILE_RtfAddControl(rd, tmp)) return FALSE;
    }
    if (!HLPFILE_RtfAddControl(rd, "}")) return FALSE;

    do
    {
        if (hlpfile->version <= 16)
        {
            index  = (ref - 0x0C) / hlpfile->dsize;
            offset = (ref - 0x0C) % hlpfile->dsize;
        }
        else
        {
            index  = (ref - 0x0C) >> 14;
            offset = (ref - 0x0C) & 0x3FFF;
        }

        if (hlpfile->version <= 16 && index != old_index && old_index != -1)
        {
            /* we jumped to the next block, adjust pointers */
            ref -= 12;
            offset -= 12;
        }

        if (index >= hlpfile->topic_maplen) {WINE_WARN("maplen\n"); break;}
        buf = hlpfile->topic_map[index] + offset;
        if (buf + 0x15 >= hlpfile->topic_end) {WINE_WARN("extra\n"); break;}
        end = min(buf + GET_UINT(buf, 0), hlpfile->topic_end);
        if (index != old_index) {offs = 0; old_index = index;}

        switch (buf[0x14])
        {
        case 0x02:
            if (count++) goto done;
            break;
        case 0x01:
        case 0x20:
        case 0x23:
            if (!HLPFILE_BrowseParagraph(page, rd, buf, end, &parlen)) return FALSE;
            if (relative > index * 0x8000 + offs)
                rd->char_pos_rel = rd->char_pos;
            offs += parlen;
            break;
        default:
            WINE_ERR("buf[0x14] = %x\n", buf[0x14]);
        }
        if (hlpfile->version <= 16)
        {
            ref += GET_UINT(buf, 0xc);
            if (GET_UINT(buf, 0xc) == 0)
                break;
        }
        else
            ref = GET_UINT(buf, 0xc);
    } while (ref != 0xffffffff);
done:
    page->first_link = rd->first_link;
    return HLPFILE_RtfAddControl(rd, "}");
}

/******************************************************************
 *		HLPFILE_ReadFont
 *
 *
 */
static BOOL HLPFILE_ReadFont(HLPFILE* hlpfile)
{
    BYTE        *ref, *end;
    unsigned    i, len, idx;
    unsigned    face_num, dscr_num, face_offset, dscr_offset;
    BYTE        flag, family;

    if (!HLPFILE_FindSubFile(hlpfile, "|FONT", &ref, &end))
    {
        WINE_WARN("no subfile FONT\n");
        hlpfile->numFonts = 0;
        hlpfile->fonts = NULL;
        return FALSE;
    }

    ref += 9;

    face_num    = GET_USHORT(ref, 0);
    dscr_num    = GET_USHORT(ref, 2);
    face_offset = GET_USHORT(ref, 4);
    dscr_offset = GET_USHORT(ref, 6);

    WINE_TRACE("Got NumFacenames=%u@%u NumDesc=%u@%u\n",
               face_num, face_offset, dscr_num, dscr_offset);

    hlpfile->numFonts = dscr_num;
    hlpfile->fonts = HeapAlloc(GetProcessHeap(), 0, sizeof(HLPFILE_FONT) * dscr_num);

    len = (dscr_offset - face_offset) / face_num;
/* EPP     for (i = face_offset; i < dscr_offset; i += len) */
/* EPP         WINE_FIXME("[%d]: %*s\n", i / len, len, ref + i); */
    for (i = 0; i < dscr_num; i++)
    {
        flag = ref[dscr_offset + i * 11 + 0];
        family = ref[dscr_offset + i * 11 + 2];

        hlpfile->fonts[i].LogFont.lfHeight = ref[dscr_offset + i * 11 + 1];
        hlpfile->fonts[i].LogFont.lfWidth = 0;
        hlpfile->fonts[i].LogFont.lfEscapement = 0;
        hlpfile->fonts[i].LogFont.lfOrientation = 0;
        hlpfile->fonts[i].LogFont.lfWeight = (flag & 1) ? 700 : 400;
        hlpfile->fonts[i].LogFont.lfItalic = (flag & 2) ? TRUE : FALSE;
        hlpfile->fonts[i].LogFont.lfUnderline = (flag & 4) ? TRUE : FALSE;
        hlpfile->fonts[i].LogFont.lfStrikeOut = (flag & 8) ? TRUE : FALSE;
        hlpfile->fonts[i].LogFont.lfCharSet = hlpfile->charset;
        hlpfile->fonts[i].LogFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
        hlpfile->fonts[i].LogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        hlpfile->fonts[i].LogFont.lfQuality = DEFAULT_QUALITY;
        hlpfile->fonts[i].LogFont.lfPitchAndFamily = DEFAULT_PITCH;

        switch (family)
        {
        case 0x01: hlpfile->fonts[i].LogFont.lfPitchAndFamily |= FF_MODERN;     break;
        case 0x02: hlpfile->fonts[i].LogFont.lfPitchAndFamily |= FF_ROMAN;      break;
        case 0x03: hlpfile->fonts[i].LogFont.lfPitchAndFamily |= FF_SWISS;      break;
        case 0x04: hlpfile->fonts[i].LogFont.lfPitchAndFamily |= FF_SCRIPT;     break;
        case 0x05: hlpfile->fonts[i].LogFont.lfPitchAndFamily |= FF_DECORATIVE; break;
        default: WINE_FIXME("Unknown family %u\n", family);
        }
        idx = GET_USHORT(ref, dscr_offset + i * 11 + 3);

        if (idx < face_num)
        {
            memcpy(hlpfile->fonts[i].LogFont.lfFaceName, ref + face_offset + idx * len, min(len, LF_FACESIZE - 1));
            hlpfile->fonts[i].LogFont.lfFaceName[min(len, LF_FACESIZE - 1)] = '\0';
        }
        else
        {
            WINE_FIXME("Too high face ref (%u/%u)\n", idx, face_num);
            strcpy(hlpfile->fonts[i].LogFont.lfFaceName, "Helv");
        }
        hlpfile->fonts[i].hFont = 0;
        hlpfile->fonts[i].color = RGB(ref[dscr_offset + i * 11 + 5],
                                      ref[dscr_offset + i * 11 + 6],
                                      ref[dscr_offset + i * 11 + 7]);
#define X(b,s) ((flag & (1 << b)) ? "-"s: "")
        WINE_TRACE("Font[%d]: flags=%02x%s%s%s%s%s%s pSize=%u family=%u face=%s[%u] color=%08x\n",
                   i, flag,
                   X(0, "bold"),
                   X(1, "italic"),
                   X(2, "underline"),
                   X(3, "strikeOut"),
                   X(4, "dblUnderline"),
                   X(5, "smallCaps"),
                   ref[dscr_offset + i * 11 + 1],
                   family,
                   hlpfile->fonts[i].LogFont.lfFaceName, idx,
                   GET_UINT(ref, dscr_offset + i * 11 + 5) & 0x00FFFFFF);
    }
    return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_ReadFileToBuffer
 */
static BOOL HLPFILE_ReadFileToBuffer(HLPFILE* hlpfile, HFILE hFile)
{
    BYTE  header[16], dummy[1];

    if (_hread(hFile, header, 16) != 16) {WINE_WARN("header\n"); return FALSE;};

    /* sanity checks */
    if (GET_UINT(header, 0) != 0x00035F3F)
    {WINE_WARN("wrong header\n"); return FALSE;};

    hlpfile->file_buffer_size = GET_UINT(header, 12);
    hlpfile->file_buffer = HeapAlloc(GetProcessHeap(), 0, hlpfile->file_buffer_size + 1);
    if (!hlpfile->file_buffer) return FALSE;

    memcpy(hlpfile->file_buffer, header, 16);
    if (_hread(hFile, hlpfile->file_buffer + 16, hlpfile->file_buffer_size - 16) !=hlpfile->file_buffer_size - 16)
    {WINE_WARN("filesize1\n"); return FALSE;};

    if (_hread(hFile, dummy, 1) != 0) WINE_WARN("filesize2\n");

    hlpfile->file_buffer[hlpfile->file_buffer_size] = '\0'; /* FIXME: was '0', sounds backwards to me */

    return TRUE;
}

/**************************************************************************
 * comp_FindSubFile
 *
 * HLPFILE_BPTreeCompare function for HLPFILE directory.
 *
 */
static int comp_FindSubFile(void *p, const void *key,
                            int leaf, void** next)
{
    *next = (char *)p+strlen(p)+(leaf?5:3);
    WINE_TRACE("Comparing '%s' with '%s'\n", (char *)p, (char *)key);
    return strcmp(p, key);
}

/***********************************************************************
 *
 *           HLPFILE_FindSubFile
 */
static BOOL HLPFILE_FindSubFile(HLPFILE* hlpfile, LPCSTR name, BYTE **subbuf, BYTE **subend)
{
    BYTE *ptr;

    WINE_TRACE("looking for file '%s'\n", name);
    ptr = HLPFILE_BPTreeSearch(hlpfile->file_buffer + GET_UINT(hlpfile->file_buffer, 4),
                               name, comp_FindSubFile);
    if (!ptr) return FALSE;
    *subbuf = hlpfile->file_buffer + GET_UINT(ptr, strlen(name)+1);
    if (*subbuf >= hlpfile->file_buffer + hlpfile->file_buffer_size)
    {
        WINE_ERR("internal file %s does not fit\n", name);
        return FALSE;
    }
    *subend = *subbuf + GET_UINT(*subbuf, 0);
    if (*subend > hlpfile->file_buffer + hlpfile->file_buffer_size)
    {
        WINE_ERR("internal file %s does not fit\n", name);
        return FALSE;
    }
    if (GET_UINT(*subbuf, 0) < GET_UINT(*subbuf, 4) + 9)
    {
        WINE_ERR("invalid size provided for internal file %s\n", name);
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_SystemCommands
 */
static BOOL HLPFILE_SystemCommands(HLPFILE* hlpfile)
{
    BYTE *buf, *ptr, *end;
    HLPFILE_MACRO *macro, **m;
    LPSTR p;
    unsigned short magic, minor, major, flags;

    hlpfile->lpszTitle = NULL;

    if (!HLPFILE_FindSubFile(hlpfile, "|SYSTEM", &buf, &end)) return FALSE;

    magic = GET_USHORT(buf + 9, 0);
    minor = GET_USHORT(buf + 9, 2);
    major = GET_USHORT(buf + 9, 4);
    /* gen date on 4 bytes */
    flags = GET_USHORT(buf + 9, 10);
    WINE_TRACE("Got system header: magic=%04x version=%d.%d flags=%04x\n",
               magic, major, minor, flags);
    if (magic != 0x036C || major != 1)
    {WINE_WARN("Wrong system header\n"); return FALSE;}
    if (minor <= 16)
    {
        hlpfile->tbsize = 0x800;
        hlpfile->compressed = 0;
    }
    else if (flags == 0)
    {
        hlpfile->tbsize = 0x1000;
        hlpfile->compressed = 0;
    }
    else if (flags == 4)
    {
        hlpfile->tbsize = 0x1000;
        hlpfile->compressed = 1;
    }
    else
    {
        hlpfile->tbsize = 0x800;
        hlpfile->compressed = 1;
    }

    if (hlpfile->compressed)
        hlpfile->dsize = 0x4000;
    else
        hlpfile->dsize = hlpfile->tbsize - 0x0C;

    hlpfile->version = minor;
    hlpfile->flags = flags;
    hlpfile->charset = DEFAULT_CHARSET;

    if (hlpfile->version <= 16)
    {
        char *str = (char*)buf + 0x15;

        hlpfile->lpszTitle = HeapAlloc(GetProcessHeap(), 0, strlen(str) + 1);
        if (!hlpfile->lpszTitle) return FALSE;
        lstrcpy(hlpfile->lpszTitle, str);
        WINE_TRACE("Title: %s\n", hlpfile->lpszTitle);
        /* Nothing more to parse */
        return TRUE;
    }
    for (ptr = buf + 0x15; ptr + 4 <= end; ptr += GET_USHORT(ptr, 2) + 4)
    {
        char *str = (char*) ptr + 4;
        switch (GET_USHORT(ptr, 0))
	{
	case 1:
            if (hlpfile->lpszTitle) {WINE_WARN("title\n"); break;}
            hlpfile->lpszTitle = HeapAlloc(GetProcessHeap(), 0, strlen(str) + 1);
            if (!hlpfile->lpszTitle) return FALSE;
            lstrcpy(hlpfile->lpszTitle, str);
            WINE_TRACE("Title: %s\n", hlpfile->lpszTitle);
            break;

	case 2:
            if (hlpfile->lpszCopyright) {WINE_WARN("copyright\n"); break;}
            hlpfile->lpszCopyright = HeapAlloc(GetProcessHeap(), 0, strlen(str) + 1);
            if (!hlpfile->lpszCopyright) return FALSE;
            lstrcpy(hlpfile->lpszCopyright, str);
            WINE_TRACE("Copyright: %s\n", hlpfile->lpszCopyright);
            break;

	case 3:
            if (GET_USHORT(ptr, 2) != 4) {WINE_WARN("system3\n");break;}
            hlpfile->contents_start = GET_UINT(ptr, 4);
            WINE_TRACE("Setting contents start at %08lx\n", hlpfile->contents_start);
            break;

	case 4:
            macro = HeapAlloc(GetProcessHeap(), 0, sizeof(HLPFILE_MACRO) + lstrlen(str) + 1);
            if (!macro) break;
            p = (char*)macro + sizeof(HLPFILE_MACRO);
            lstrcpy(p, str);
            macro->lpszMacro = p;
            macro->next = 0;
            for (m = &hlpfile->first_macro; *m; m = &(*m)->next);
            *m = macro;
            break;

        case 5:
            if (GET_USHORT(ptr, 4 + 4) != 1)
                WINE_FIXME("More than one icon, picking up first\n");
            /* 0x16 is sizeof(CURSORICONDIR), see user32/user_private.h */
            hlpfile->hIcon = CreateIconFromResourceEx(ptr + 4 + 0x16,
                                                      GET_USHORT(ptr, 2) - 0x16, TRUE,
                                                      0x30000, 0, 0, 0);
            break;

        case 6:
            if (GET_USHORT(ptr, 2) != 90) {WINE_WARN("system6\n");break;}

	    if (hlpfile->windows) 
        	hlpfile->windows = HeapReAlloc(GetProcessHeap(), 0, hlpfile->windows, 
                                           sizeof(HLPFILE_WINDOWINFO) * ++hlpfile->numWindows);
	    else 
        	hlpfile->windows = HeapAlloc(GetProcessHeap(), 0, 
                                           sizeof(HLPFILE_WINDOWINFO) * ++hlpfile->numWindows);
	    
            if (hlpfile->windows)
            {
                unsigned flags = GET_USHORT(ptr, 4);
                HLPFILE_WINDOWINFO* wi = &hlpfile->windows[hlpfile->numWindows - 1];

                if (flags & 0x0001) strcpy(wi->type, &str[2]);
                else wi->type[0] = '\0';
                if (flags & 0x0002) strcpy(wi->name, &str[12]);
                else wi->name[0] = '\0';
                if (flags & 0x0004) strcpy(wi->caption, &str[21]);
                else lstrcpynA(wi->caption, hlpfile->lpszTitle, sizeof(wi->caption));
                wi->origin.x = (flags & 0x0008) ? GET_USHORT(ptr, 76) : CW_USEDEFAULT;
                wi->origin.y = (flags & 0x0010) ? GET_USHORT(ptr, 78) : CW_USEDEFAULT;
                wi->size.cx = (flags & 0x0020) ? GET_USHORT(ptr, 80) : CW_USEDEFAULT;
                wi->size.cy = (flags & 0x0040) ? GET_USHORT(ptr, 82) : CW_USEDEFAULT;
                wi->style = (flags & 0x0080) ? GET_USHORT(ptr, 84) : SW_SHOW;
                wi->win_style = WS_OVERLAPPEDWINDOW;
                wi->sr_color = (flags & 0x0100) ? GET_UINT(ptr, 86) : 0xFFFFFF;
                wi->nsr_color = (flags & 0x0200) ? GET_UINT(ptr, 90) : 0xFFFFFF;
                WINE_TRACE("System-Window: flags=%c%c%c%c%c%c%c%c type=%s name=%s caption=%s (%d,%d)x(%d,%d)\n",
                           flags & 0x0001 ? 'T' : 't',
                           flags & 0x0002 ? 'N' : 'n',
                           flags & 0x0004 ? 'C' : 'c',
                           flags & 0x0008 ? 'X' : 'x',
                           flags & 0x0010 ? 'Y' : 'y',
                           flags & 0x0020 ? 'W' : 'w',
                           flags & 0x0040 ? 'H' : 'h',
                           flags & 0x0080 ? 'S' : 's',
                           wi->type, wi->name, wi->caption, wi->origin.x, wi->origin.y,
                           wi->size.cx, wi->size.cy);
            }
            break;
        case 8:
            WINE_WARN("Citation: '%s'\n", ptr + 4);
            break;
        case 11:
            hlpfile->charset = ptr[4];
            WINE_TRACE("Charset: %d\n", hlpfile->charset);
            break;
	default:
            WINE_WARN("Unsupported SystemRecord[%d]\n", GET_USHORT(ptr, 0));
	}
    }
    if (!hlpfile->lpszTitle)
        hlpfile->lpszTitle = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1);
    return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_UncompressedLZ77_Size
 */
static INT HLPFILE_UncompressedLZ77_Size(const BYTE *ptr, const BYTE *end)
{
    int  i, newsize = 0;

    while (ptr < end)
    {
        int mask = *ptr++;
        for (i = 0; i < 8 && ptr < end; i++, mask >>= 1)
	{
            if (mask & 1)
	    {
                int code = GET_USHORT(ptr, 0);
                int len  = 3 + (code >> 12);
                newsize += len;
                ptr     += 2;
	    }
            else newsize++, ptr++;
	}
    }

    return newsize;
}

/***********************************************************************
 *
 *           HLPFILE_UncompressLZ77
 */
static BYTE *HLPFILE_UncompressLZ77(const BYTE *ptr, const BYTE *end, BYTE *newptr)
{
    int i;

    while (ptr < end)
    {
        int mask = *ptr++;
        for (i = 0; i < 8 && ptr < end; i++, mask >>= 1)
	{
            if (mask & 1)
	    {
                int code   = GET_USHORT(ptr, 0);
                int len    = 3 + (code >> 12);
                int offset = code & 0xfff;
                /*
                 * We must copy byte-by-byte here. We cannot use memcpy nor
                 * memmove here. Just example:
                 * a[]={1,2,3,4,5,6,7,8,9,10}
                 * newptr=a+2;
                 * offset=1;
                 * We expect:
                 * {1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 11, 12}
                 */
                for (; len>0; len--, newptr++) *newptr = *(newptr-offset-1);
                ptr    += 2;
	    }
            else *newptr++ = *ptr++;
	}
    }

    return newptr;
}

/***********************************************************************
 *
 *           HLPFILE_UncompressLZ77_Phrases
 */
static BOOL HLPFILE_UncompressLZ77_Phrases(HLPFILE* hlpfile)
{
    UINT i, num, dec_size, head_size;
    BYTE *buf, *end;

    if (!HLPFILE_FindSubFile(hlpfile, "|Phrases", &buf, &end)) return FALSE;

    if (hlpfile->version <= 16)
        head_size = 13;
    else
        head_size = 17;

    num = hlpfile->num_phrases = GET_USHORT(buf, 9);
    if (buf + 2 * num + 0x13 >= end) {WINE_WARN("1a\n"); return FALSE;};

    if (hlpfile->version <= 16)
        dec_size = end - buf - 15 - 2 * num;
    else
        dec_size = HLPFILE_UncompressedLZ77_Size(buf + 0x13 + 2 * num, end);

    hlpfile->phrases_offsets = HeapAlloc(GetProcessHeap(), 0, sizeof(unsigned) * (num + 1));
    hlpfile->phrases_buffer  = HeapAlloc(GetProcessHeap(), 0, dec_size);
    if (!hlpfile->phrases_offsets || !hlpfile->phrases_buffer)
    {
        HeapFree(GetProcessHeap(), 0, hlpfile->phrases_offsets);
        HeapFree(GetProcessHeap(), 0, hlpfile->phrases_buffer);
        return FALSE;
    }

    for (i = 0; i <= num; i++)
        hlpfile->phrases_offsets[i] = GET_USHORT(buf, head_size + 2 * i) - 2 * num - 2;

    if (hlpfile->version <= 16)
        memcpy(hlpfile->phrases_buffer, buf + 15 + 2*num, dec_size);
    else
        HLPFILE_UncompressLZ77(buf + 0x13 + 2 * num, end, (BYTE*)hlpfile->phrases_buffer);

    hlpfile->hasPhrases = TRUE;
    return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_Uncompress_Phrases40
 */
static BOOL HLPFILE_Uncompress_Phrases40(HLPFILE* hlpfile)
{
    UINT num;
    INT dec_size, cpr_size;
    BYTE *buf_idx, *end_idx;
    BYTE *buf_phs, *end_phs;
    long* ptr, mask = 0;
    unsigned int i;
    unsigned short bc, n;

    if (!HLPFILE_FindSubFile(hlpfile, "|PhrIndex", &buf_idx, &end_idx) ||
        !HLPFILE_FindSubFile(hlpfile, "|PhrImage", &buf_phs, &end_phs)) return FALSE;

    ptr = (long*)(buf_idx + 9 + 28);
    bc = GET_USHORT(buf_idx, 9 + 24) & 0x0F;
    num = hlpfile->num_phrases = GET_USHORT(buf_idx, 9 + 4);

    WINE_TRACE("Index: Magic=%08x #entries=%u CpsdSize=%u PhrImgSize=%u\n"
               "\tPhrImgCprsdSize=%u 0=%u bc=%x ukn=%x\n",
               GET_UINT(buf_idx, 9 + 0),
               GET_UINT(buf_idx, 9 + 4),
               GET_UINT(buf_idx, 9 + 8),
               GET_UINT(buf_idx, 9 + 12),
               GET_UINT(buf_idx, 9 + 16),
               GET_UINT(buf_idx, 9 + 20),
               GET_USHORT(buf_idx, 9 + 24),
               GET_USHORT(buf_idx, 9 + 26));

    dec_size = GET_UINT(buf_idx, 9 + 12);
    cpr_size = GET_UINT(buf_idx, 9 + 16);

    if (dec_size != cpr_size &&
        dec_size != HLPFILE_UncompressedLZ77_Size(buf_phs + 9, end_phs))
    {
        WINE_WARN("size mismatch %u %u\n",
                  dec_size, HLPFILE_UncompressedLZ77_Size(buf_phs + 9, end_phs));
        dec_size = max(dec_size, HLPFILE_UncompressedLZ77_Size(buf_phs + 9, end_phs));
    }

    hlpfile->phrases_offsets = HeapAlloc(GetProcessHeap(), 0, sizeof(unsigned) * (num + 1));
    hlpfile->phrases_buffer  = HeapAlloc(GetProcessHeap(), 0, dec_size);
    if (!hlpfile->phrases_offsets || !hlpfile->phrases_buffer)
    {
        HeapFree(GetProcessHeap(), 0, hlpfile->phrases_offsets);
        HeapFree(GetProcessHeap(), 0, hlpfile->phrases_buffer);
        return FALSE;
    }

#define getbit() (ptr += (mask < 0), mask = mask*2 + (mask<=0), (*ptr & mask) != 0)

    hlpfile->phrases_offsets[0] = 0;
    for (i = 0; i < num; i++)
    {
        for (n = 1; getbit(); n += 1 << bc);
        if (getbit()) n++;
        if (bc > 1 && getbit()) n += 2;
        if (bc > 2 && getbit()) n += 4;
        if (bc > 3 && getbit()) n += 8;
        if (bc > 4 && getbit()) n += 16;
        hlpfile->phrases_offsets[i + 1] = hlpfile->phrases_offsets[i] + n;
    }
#undef getbit

    if (dec_size == cpr_size)
        memcpy(hlpfile->phrases_buffer, buf_phs + 9, dec_size);
    else
        HLPFILE_UncompressLZ77(buf_phs + 9, end_phs, (BYTE*)hlpfile->phrases_buffer);

    hlpfile->hasPhrases40 = TRUE;
    return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_Uncompress_Topic
 */
static BOOL HLPFILE_Uncompress_Topic(HLPFILE* hlpfile)
{
    BYTE *buf, *ptr, *end, *newptr;
    unsigned int i, newsize = 0;
    unsigned int topic_size;

    if (!HLPFILE_FindSubFile(hlpfile, "|TOPIC", &buf, &end))
    {WINE_WARN("topic0\n"); return FALSE;}

    buf += 9; /* Skip file header */
    topic_size = end - buf;
    if (hlpfile->compressed)
    {
        hlpfile->topic_maplen = (topic_size - 1) / hlpfile->tbsize + 1;

        for (i = 0; i < hlpfile->topic_maplen; i++)
        {
            ptr = buf + i * hlpfile->tbsize;

            /* I don't know why, it's necessary for printman.hlp */
            if (ptr + 0x44 > end) ptr = end - 0x44;

            newsize += HLPFILE_UncompressedLZ77_Size(ptr + 0xc, min(end, ptr + hlpfile->tbsize));
        }

        hlpfile->topic_map = HeapAlloc(GetProcessHeap(), 0,
                                       hlpfile->topic_maplen * sizeof(hlpfile->topic_map[0]) + newsize);
        if (!hlpfile->topic_map) return FALSE;
        newptr = (BYTE*)(hlpfile->topic_map + hlpfile->topic_maplen);
        hlpfile->topic_end = newptr + newsize;

        for (i = 0; i < hlpfile->topic_maplen; i++)
        {
            ptr = buf + i * hlpfile->tbsize;
            if (ptr + 0x44 > end) ptr = end - 0x44;

            hlpfile->topic_map[i] = newptr;
            newptr = HLPFILE_UncompressLZ77(ptr + 0xc, min(end, ptr + hlpfile->tbsize), newptr);
        }
    }
    else
    {
        /* basically, we need to copy the TopicBlockSize byte pages
         * (removing the first 0x0C) in one single area in memory
         */
        hlpfile->topic_maplen = (topic_size - 1) / hlpfile->tbsize + 1;
        hlpfile->topic_map = HeapAlloc(GetProcessHeap(), 0,
                                       hlpfile->topic_maplen * (sizeof(hlpfile->topic_map[0]) + hlpfile->dsize));
        if (!hlpfile->topic_map) return FALSE;
        newptr = (BYTE*)(hlpfile->topic_map + hlpfile->topic_maplen);
        hlpfile->topic_end = newptr + topic_size;

        for (i = 0; i < hlpfile->topic_maplen; i++)
        {
            hlpfile->topic_map[i] = newptr + i * hlpfile->dsize;
            memcpy(hlpfile->topic_map[i], buf + i * hlpfile->tbsize + 0x0C, hlpfile->dsize);
        }
    }
    return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_Uncompress2
 */

static void HLPFILE_Uncompress2(HLPFILE* hlpfile, const BYTE *ptr, const BYTE *end, BYTE *newptr, const BYTE *newend)
{
    BYTE *phptr, *phend;
    UINT code;
    UINT index;

    while (ptr < end && newptr < newend)
    {
        if (!*ptr || *ptr >= 0x10)
            *newptr++ = *ptr++;
        else
	{
            code  = 0x100 * ptr[0] + ptr[1];
            index = (code - 0x100) / 2;

            phptr = (BYTE*)hlpfile->phrases_buffer + hlpfile->phrases_offsets[index];
            phend = (BYTE*)hlpfile->phrases_buffer + hlpfile->phrases_offsets[index + 1];

            if (newptr + (phend - phptr) > newend)
            {
                WINE_FIXME("buffer overflow %p > %p for %lu bytes\n",
                           newptr, newend, (SIZE_T)(phend - phptr));
                return;
            }
            memcpy(newptr, phptr, phend - phptr);
            newptr += phend - phptr;
            if (code & 1) *newptr++ = ' ';

            ptr += 2;
	}
    }
    if (newptr > newend) WINE_FIXME("buffer overflow %p > %p\n", newptr, newend);
}

/******************************************************************
 *		HLPFILE_Uncompress3
 *
 *
 */
static BOOL HLPFILE_Uncompress3(HLPFILE* hlpfile, char* dst, const char* dst_end,
                                const BYTE* src, const BYTE* src_end)
{
    unsigned int idx, len;

    for (; src < src_end; src++)
    {
        if ((*src & 1) == 0)
        {
            idx = *src / 2;
            if (idx > hlpfile->num_phrases)
            {
                WINE_ERR("index in phrases %d/%d\n", idx, hlpfile->num_phrases);
                len = 0;
            }
            else 
            {
                len = hlpfile->phrases_offsets[idx + 1] - hlpfile->phrases_offsets[idx];
                if (dst + len <= dst_end)
                    memcpy(dst, &hlpfile->phrases_buffer[hlpfile->phrases_offsets[idx]], len);
            }
        }
        else if ((*src & 0x03) == 0x01)
        {
            idx = (*src + 1) * 64;
            idx += *++src;
            if (idx > hlpfile->num_phrases)
            {
                WINE_ERR("index in phrases %d/%d\n", idx, hlpfile->num_phrases);
                len = 0;
            }
            else
            {
                len = hlpfile->phrases_offsets[idx + 1] - hlpfile->phrases_offsets[idx];
                if (dst + len <= dst_end)
                    memcpy(dst, &hlpfile->phrases_buffer[hlpfile->phrases_offsets[idx]], len);
            }
        }
        else if ((*src & 0x07) == 0x03)
        {
            len = (*src / 8) + 1;
            if (dst + len <= dst_end)
                memcpy(dst, src + 1, len);
            src += len;
        }
        else
        {
            len = (*src / 16) + 1;
            if (dst + len <= dst_end)
                memset(dst, ((*src & 0x0F) == 0x07) ? ' ' : 0, len);
        }
        dst += len;
    }

    if (dst > dst_end) WINE_ERR("buffer overflow (%p > %p)\n", dst, dst_end);
    return TRUE;
}

/******************************************************************
 *		HLPFILE_UncompressRLE
 *
 *
 */
static void HLPFILE_UncompressRLE(const BYTE* src, const BYTE* end, BYTE* dst, unsigned dstsz)
{
    BYTE        ch;
    BYTE*       sdst = dst + dstsz;

    while (src < end)
    {
        ch = *src++;
        if (ch & 0x80)
        {
            ch &= 0x7F;
            if (dst + ch <= sdst)
                memcpy(dst, src, ch);
            src += ch;
        }
        else
        {
            if (dst + ch <= sdst)
                memset(dst, (char)*src, ch);
            src++;
        }
        dst += ch;
    }
    if (dst != sdst)
        WINE_WARN("Buffer X-flow: d(%lu) instead of d(%u)\n",
                  (SIZE_T)(dst - (sdst - dstsz)), dstsz);
}

/**************************************************************************
 * HLPFILE_BPTreeSearch
 *
 * Searches for an element in B+ tree
 *
 * PARAMS
 *     buf        [I] pointer to the embedded file structured as a B+ tree
 *     key        [I] pointer to data to find
 *     comp       [I] compare function
 *
 * RETURNS
 *     Pointer to block identified by key, or NULL if failure.
 *
 */
void* HLPFILE_BPTreeSearch(BYTE* buf, const void* key,
                           HLPFILE_BPTreeCompare comp)
{
    unsigned magic;
    unsigned page_size;
    unsigned cur_page;
    unsigned level;
    BYTE *pages, *ptr, *newptr;
    int i, entries;
    int ret;

    magic = GET_USHORT(buf, 9);
    if (magic != 0x293B)
    {
        WINE_ERR("Invalid magic in B+ tree: 0x%x\n", magic);
        return NULL;
    }
    page_size = GET_USHORT(buf, 9+4);
    cur_page  = GET_USHORT(buf, 9+26);
    level     = GET_USHORT(buf, 9+32);
    pages     = buf + 9 + 38;
    while (--level > 0)
    {
        ptr = pages + cur_page*page_size;
        entries = GET_SHORT(ptr, 2);
        ptr += 6;
        for (i = 0; i < entries; i++)
        {
            if (comp(ptr, key, 0, (void **)&newptr) > 0) break;
            ptr = newptr;
        }
        cur_page = GET_USHORT(ptr-2, 0);
    }
    ptr = pages + cur_page*page_size;
    entries = GET_SHORT(ptr, 2);
    ptr += 8;
    for (i = 0; i < entries; i++)
    {
        ret = comp(ptr, key, 1, (void **)&newptr);
        if (ret == 0) return ptr;
        if (ret > 0) return NULL;
        ptr = newptr;
    }
    return NULL;
}

/**************************************************************************
 * HLPFILE_BPTreeEnum
 *
 * Enumerates elements in B+ tree.
 *
 * PARAMS
 *     buf        [I]  pointer to the embedded file structured as a B+ tree
 *     cb         [I]  compare function
 *     cookie     [IO] cookie for cb function
 */
void HLPFILE_BPTreeEnum(BYTE* buf, HLPFILE_BPTreeCallback cb, void* cookie)
{
    unsigned magic;
    unsigned page_size;
    unsigned cur_page;
    unsigned level;
    BYTE *pages, *ptr, *newptr;
    int i, entries;

    magic = GET_USHORT(buf, 9);
    if (magic != 0x293B)
    {
        WINE_ERR("Invalid magic in B+ tree: 0x%x\n", magic);
        return;
    }
    page_size = GET_USHORT(buf, 9+4);
    cur_page  = GET_USHORT(buf, 9+26);
    level     = GET_USHORT(buf, 9+32);
    pages     = buf + 9 + 38;
    while (--level > 0)
    {
        ptr = pages + cur_page*page_size;
        cur_page = GET_USHORT(ptr, 4);
    }
    while (cur_page != 0xFFFF)
    {
        ptr = pages + cur_page*page_size;
        entries = GET_SHORT(ptr, 2);
        ptr += 8;
        for (i = 0; i < entries; i++)
        {
            cb(ptr, (void **)&newptr, cookie);
            ptr = newptr;
        }
        cur_page = GET_USHORT(pages+cur_page*page_size, 6);
    }
}


/***********************************************************************
 *
 *           HLPFILE_GetContext
 */
static BOOL HLPFILE_GetContext(HLPFILE *hlpfile)
{
    BYTE                *cbuf, *cend;
    unsigned            clen;

    if (!HLPFILE_FindSubFile(hlpfile, "|CONTEXT",  &cbuf, &cend))
    {WINE_WARN("context0\n"); return FALSE;}

    clen = cend - cbuf;
    hlpfile->Context = HeapAlloc(GetProcessHeap(), 0, clen);
    if (!hlpfile->Context) return FALSE;
    memcpy(hlpfile->Context, cbuf, clen);

    return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_GetKeywords
 */
static BOOL HLPFILE_GetKeywords(HLPFILE *hlpfile)
{
    BYTE                *cbuf, *cend;
    unsigned            clen;

    if (!HLPFILE_FindSubFile(hlpfile, "|KWBTREE", &cbuf, &cend)) return FALSE;
    clen = cend - cbuf;
    hlpfile->kwbtree = HeapAlloc(GetProcessHeap(), 0, clen);
    if (!hlpfile->kwbtree) return FALSE;
    memcpy(hlpfile->kwbtree, cbuf, clen);

    if (!HLPFILE_FindSubFile(hlpfile, "|KWDATA", &cbuf, &cend))
    {
        WINE_ERR("corrupted help file: kwbtree present but kwdata absent\n");
        HeapFree(GetProcessHeap(), 0, hlpfile->kwbtree);
        return FALSE;
    }
    clen = cend - cbuf;
    hlpfile->kwdata = HeapAlloc(GetProcessHeap(), 0, clen);
    if (!hlpfile->kwdata)
    {
        HeapFree(GetProcessHeap(), 0, hlpfile->kwdata);
        return FALSE;
    }
    memcpy(hlpfile->kwdata, cbuf, clen);

    return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_GetMap
 */
static BOOL HLPFILE_GetMap(HLPFILE *hlpfile)
{
    BYTE                *cbuf, *cend;
    unsigned            entries, i;

    if (!HLPFILE_FindSubFile(hlpfile, "|CTXOMAP",  &cbuf, &cend))
    {WINE_WARN("no map section\n"); return FALSE;}

    entries = GET_USHORT(cbuf, 9);
    hlpfile->Map = HeapAlloc(GetProcessHeap(), 0, entries * sizeof(HLPFILE_MAP));
    if (!hlpfile->Map) return FALSE;
    hlpfile->wMapLen = entries;
    for (i = 0; i < entries; i++)
    {
        hlpfile->Map[i].lMap = GET_UINT(cbuf+11,i*8);
        hlpfile->Map[i].offset = GET_UINT(cbuf+11,i*8+4);
    }
    return TRUE;
}

/***********************************************************************
 *
 *           HLPFILE_GetTOMap
 */
static BOOL HLPFILE_GetTOMap(HLPFILE *hlpfile)
{
    BYTE                *cbuf, *cend;
    unsigned            clen;

    if (!HLPFILE_FindSubFile(hlpfile, "|TOMAP",  &cbuf, &cend))
    {WINE_WARN("no tomap section\n"); return FALSE;}

    clen = cend - cbuf - 9;
    hlpfile->TOMap = HeapAlloc(GetProcessHeap(), 0, clen);
    if (!hlpfile->TOMap) return FALSE;
    memcpy(hlpfile->TOMap, cbuf+9, clen);
    hlpfile->wTOMapLen = clen/4;
    return TRUE;
}

/***********************************************************************
 *
 *           DeleteMacro
 */
static void HLPFILE_DeleteMacro(HLPFILE_MACRO* macro)
{
    HLPFILE_MACRO*      next;

    while (macro)
    {
        next = macro->next;
        HeapFree(GetProcessHeap(), 0, macro);
        macro = next;
    }
}

/***********************************************************************
 *
 *           DeletePage
 */
static void HLPFILE_DeletePage(HLPFILE_PAGE* page)
{
    HLPFILE_PAGE* next;

    while (page)
    {
        next = page->next;
        HLPFILE_DeleteMacro(page->first_macro);
        HeapFree(GetProcessHeap(), 0, page);
        page = next;
    }
}

/***********************************************************************
 *
 *           HLPFILE_FreeHlpFile
 */
void HLPFILE_FreeHlpFile(HLPFILE* hlpfile)
{
    unsigned i;

    if (!hlpfile || --hlpfile->wRefCount > 0) return;

    if (hlpfile->next) hlpfile->next->prev = hlpfile->prev;
    if (hlpfile->prev) hlpfile->prev->next = hlpfile->next;
    else first_hlpfile = hlpfile->next;

    if (hlpfile->numFonts)
    {
        for (i = 0; i < hlpfile->numFonts; i++)
        {
            DeleteObject(hlpfile->fonts[i].hFont);
        }
        HeapFree(GetProcessHeap(), 0, hlpfile->fonts);
    }

    if (hlpfile->numBmps)
    {
        for (i = 0; i < hlpfile->numBmps; i++)
        {
            DeleteObject(hlpfile->bmps[i]);
        }
        HeapFree(GetProcessHeap(), 0, hlpfile->bmps);
    }

    HLPFILE_DeletePage(hlpfile->first_page);
    HLPFILE_DeleteMacro(hlpfile->first_macro);

    DestroyIcon(hlpfile->hIcon);
    if (hlpfile->numWindows)    HeapFree(GetProcessHeap(), 0, hlpfile->windows);
    HeapFree(GetProcessHeap(), 0, hlpfile->Context);
    HeapFree(GetProcessHeap(), 0, hlpfile->Map);
    HeapFree(GetProcessHeap(), 0, hlpfile->lpszTitle);
    HeapFree(GetProcessHeap(), 0, hlpfile->lpszCopyright);
    HeapFree(GetProcessHeap(), 0, hlpfile->file_buffer);
    HeapFree(GetProcessHeap(), 0, hlpfile->phrases_offsets);
    HeapFree(GetProcessHeap(), 0, hlpfile->phrases_buffer);
    HeapFree(GetProcessHeap(), 0, hlpfile->topic_map);
    HeapFree(GetProcessHeap(), 0, hlpfile->help_on_file);
    HeapFree(GetProcessHeap(), 0, hlpfile);
}
