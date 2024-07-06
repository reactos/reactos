/*
 * Copyright 2011 Piotr Caban for CodeWeavers
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
#include <limits.h>
#include <share.h>

#include "msvcp90.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

#define SECSPERDAY        86400
/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)
#define TICKSPERSEC       10000000
#define TICKS_1601_TO_1970 (SECS_1601_TO_1970 * TICKSPERSEC)

/* ?_Index@ios_base@std@@0HA */
int ios_base_Index = 0;
/* ?_Sync@ios_base@std@@0_NA */
bool ios_base_Sync = FALSE;

typedef struct {
    streamoff off;
    __int64 DECLSPEC_ALIGN(8) pos;
    _Mbstatet state;
} fpos_mbstatet;

static inline const char* debugstr_fpos_mbstatet(fpos_mbstatet *fpos)
{
    return wine_dbg_sprintf("fpos(%s %s %d)", wine_dbgstr_longlong(fpos->off),
            wine_dbgstr_longlong(fpos->pos), MBSTATET_TO_INT(&fpos->state));
}

typedef struct {
    void (__cdecl *pfunc)(ios_base*, streamsize);
    streamsize arg;
} manip_streamsize;

typedef struct {
    void (__cdecl *pfunc)(ios_base*, int);
    int arg;
} manip_int;

typedef enum {
    INITFL_new   = 0,
    INITFL_open  = 1,
    INITFL_close = 2
} basic_filebuf__Initfl;

typedef struct {
    basic_streambuf_char base;
    codecvt_char *cvt;
#if _MSVCP_VER < 80
    int state0;
    char putback;
    basic_string_char *conv;
#else
    char putback;
#endif
    bool wrotesome;
    _Mbstatet state;
    bool close;
#if _MSVCP_VER == 70
    locale loc;
#endif
    FILE *file;
} basic_filebuf_char;

typedef struct {
    basic_streambuf_wchar base;
    codecvt_wchar *cvt;
#if _MSVCP_VER < 80
    int state0;
    wchar_t putback;
    basic_string_char *conv;
#else
    wchar_t putback;
#endif
    bool wrotesome;
    _Mbstatet state;
    bool close;
#if _MSVCP_VER == 70
    locale loc;
#endif
    FILE *file;
} basic_filebuf_wchar;

typedef enum {
    STRINGBUF_allocated = 1,
    STRINGBUF_no_write = 2,
    STRINGBUF_no_read = 4,
    STRINGBUF_append = 8,
    STRINGBUF_at_end = 16
} basic_stringbuf_state;

typedef struct {
    basic_streambuf_char base;
    char *seekhigh;
    int state;
    char allocator; /* empty struct */
} basic_stringbuf_char;

typedef struct {
    basic_streambuf_wchar base;
    wchar_t *seekhigh;
    int state;
    char allocator; /* empty struct */
} basic_stringbuf_wchar;

typedef struct {
    ios_base base;
    basic_streambuf_char *strbuf;
    struct _basic_ostream_char *stream;
    char fillch;
} basic_ios_char;

typedef struct {
    ios_base base;
    basic_streambuf_wchar *strbuf;
    struct _basic_ostream_wchar *stream;
    wchar_t fillch;
} basic_ios_wchar;

typedef struct _basic_ostream_char {
    const int *vbtable;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_ostream_char;

typedef struct _basic_ostream_wchar {
    const int *vbtable;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_ostream_wchar;

typedef struct {
    const int *vbtable;
    streamsize count;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_istream_char;

typedef struct {
    const int *vbtable;
    streamsize count;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_istream_wchar;

typedef struct {
    basic_istream_char base1;
    basic_ostream_char base2;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_iostream_char;

typedef struct {
    basic_istream_wchar base1;
    basic_ostream_wchar base2;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_iostream_wchar;

typedef struct {
    basic_ostream_char base;
    basic_filebuf_char filebuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_ofstream_char;

typedef struct {
    basic_ostream_wchar base;
    basic_filebuf_wchar filebuf;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_ofstream_wchar;

typedef struct {
    basic_istream_char base;
    basic_filebuf_char filebuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_ifstream_char;

typedef struct {
    basic_istream_wchar base;
    basic_filebuf_wchar filebuf;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_ifstream_wchar;

typedef struct {
    basic_iostream_char base;
    basic_filebuf_char filebuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_fstream_char;

typedef struct {
    basic_iostream_wchar base;
    basic_filebuf_wchar filebuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_fstream_wchar;

typedef struct {
    basic_ostream_char base;
    basic_stringbuf_char strbuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_ostringstream_char;

typedef struct {
    basic_ostream_wchar base;
    basic_stringbuf_wchar strbuf;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_ostringstream_wchar;

typedef struct {
    basic_istream_char base;
    basic_stringbuf_char strbuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_istringstream_char;

typedef struct {
    basic_istream_wchar base;
    basic_stringbuf_wchar strbuf;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_istringstream_wchar;

typedef struct {
    basic_iostream_char base;
    basic_stringbuf_char strbuf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} basic_stringstream_char;

typedef struct {
    basic_iostream_wchar base;
    basic_stringbuf_wchar strbuf;
    /* virtual inheritance
     * basic_ios_wchar basic_ios;
     */
} basic_stringstream_wchar;

typedef enum {
    STRSTATE_Allocated = 1,
    STRSTATE_Constant = 2,
    STRSTATE_Dynamic = 4,
    STRSTATE_Frozen = 8
} strstreambuf__Strstate;

typedef struct {
    basic_streambuf_char base;
    streamsize minsize;
    char *endsave;
    char *seekhigh;
    int strmode;
    void* (__cdecl *palloc)(size_t);
    void (__cdecl *pfree)(void*);
} strstreambuf;

typedef struct {
    basic_ostream_char base;
    strstreambuf buf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} ostrstream;

typedef struct {
    basic_istream_char base;
    strstreambuf buf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} istrstream;

typedef struct {
    basic_iostream_char base;
    strstreambuf buf;
    /* virtual inheritance
     * basic_ios_char basic_ios;
     */
} strstream;

struct space_info {
    ULONGLONG capacity;
    ULONGLONG free;
    ULONGLONG available;
};

enum file_type {
#if _MSVCP_VER < 140
    status_unknown,
    file_not_found,
#else
    file_not_found = -1,
    none_file,
#endif
    regular_file,
    directory_file,
    symlink_file,
    block_file,
    character_file,
    fifo_file,
    socket_file,
#if _MSVCP_VER < 140
    type_unknown,
#else
    status_unknown
#endif
};

#if _MSVCP_VER >= 110
#define BASIC_IOS_VTORDISP 1
#define INIT_BASIC_IOS_VTORDISP(basic_ios) ((int*)basic_ios)[-1] = 0
#else
#define BASIC_IOS_VTORDISP 0
#define INIT_BASIC_IOS_VTORDISP(basic_ios)
#endif

#define VBTABLE_ENTRY(class, offset, vbase, vtordisp) ALIGNED_SIZE(sizeof(class)+vtordisp*sizeof(int), TYPE_ALIGNMENT(vbase))-offset
#define VBTABLE_BASIC_IOS_ENTRY(class, offset) VBTABLE_ENTRY(class, offset, basic_ios_char, BASIC_IOS_VTORDISP)

extern const vtable_ptr iosb_vtable;

/* ??_7ios_base@std@@6B@ */
extern const vtable_ptr ios_base_vtable;

/* ??_7?$basic_ios@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_ios_char_vtable;

/* ??_7?$basic_ios@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr basic_ios_wchar_vtable;

/* ??_7?$basic_ios@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_ios_short_vtable;

/* ??_7?$basic_streambuf@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_streambuf_char_vtable;

/* ??_7?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr basic_streambuf_wchar_vtable;

/* ??_7?$basic_streambuf@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_streambuf_short_vtable;

/* ??_7?$basic_filebuf@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_filebuf_char_vtable;

/* ??_7?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr basic_filebuf_wchar_vtable;

/* ??_7?$basic_filebuf@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_filebuf_short_vtable;

/* ??_7?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@6B@ */
extern const vtable_ptr basic_stringbuf_char_vtable;

/* ??_7?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@6B@ */
extern const vtable_ptr basic_stringbuf_wchar_vtable;

/* ??_7?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@6B@ */
extern const vtable_ptr basic_stringbuf_short_vtable;

/* ??_8?$basic_ostream@DU?$char_traits@D@std@@@std@@7B@ */
const int basic_ostream_char_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_ostream_char, 0)};
/* ??_7?$basic_ostream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_ostream_char_vtable;

/* ??_8?$basic_ostream@_WU?$char_traits@_W@std@@@std@@7B@ */
const int basic_ostream_wchar_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_ostream_wchar, 0)};
/* ??_7?$basic_ostream@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr basic_ostream_wchar_vtable;

/* ??_8?$basic_ostream@GU?$char_traits@G@std@@@std@@7B@ */
const int basic_ostream_short_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_ostream_wchar, 0)};
/* ??_7?$basic_ostream@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_ostream_short_vtable;

/* ??_8?$basic_istream@DU?$char_traits@D@std@@@std@@7B@ */
const int basic_istream_char_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_istream_char, 0)};
/* ??_7?$basic_istream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_istream_char_vtable;

/* ??_8?$basic_istream@_WU?$char_traits@_W@std@@@std@@7B@ */
const int basic_istream_wchar_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_istream_wchar, 0)};
/* ??_7?$basic_istream@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr basic_istream_wchar_vtable;

/* ??_8?$basic_istream@GU?$char_traits@G@std@@@std@@7B@ */
const int basic_istream_short_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_istream_wchar, 0)};
/* ??_7?$basic_istream@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_istream_short_vtable;

/* ??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@ */
const int basic_iostream_char_vbtable1[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_iostream_char, 0)};
/* ??_8?$basic_iostream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@ */
const int basic_iostream_char_vbtable2[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_iostream_char, FIELD_OFFSET(basic_iostream_char, base2))};
/* ??_7?$basic_iostream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_iostream_char_vtable;

/* ??_8?$basic_iostream@_WU?$char_traits@_W@std@@@std@@7B?$basic_istream@_WU?$char_traits@_W@std@@@1@@ */
/* ??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@ */
const int basic_iostream_wchar_vbtable1[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_iostream_wchar, 0)};
/* ??_8?$basic_iostream@_WU?$char_traits@_W@std@@@std@@7B?$basic_ostream@_WU?$char_traits@_W@std@@@1@@ */
/* ??_8?$basic_iostream@GU?$char_traits@G@std@@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@ */
const int basic_iostream_wchar_vbtable2[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_iostream_wchar, FIELD_OFFSET(basic_iostream_wchar, base2))};
/* ??_7?$basic_iostream@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr basic_iostream_wchar_vtable;
/* ??_7?$basic_iostream@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_iostream_short_vtable;

/* ??_8?$basic_ofstream@DU?$char_traits@D@std@@@std@@7B@ */
const int basic_ofstream_char_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_ofstream_char, 0)};
/* ??_7?$basic_ofstream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_ofstream_char_vtable;

/* ??_8?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@7B@ */
const int basic_ofstream_wchar_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_ofstream_wchar, 0)};
/* ??_7?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr basic_ofstream_wchar_vtable;

/* ??_8?$basic_ofstream@GU?$char_traits@G@std@@@std@@7B@ */
const int basic_ofstream_short_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_ofstream_wchar, 0)};
/* ??_7?$basic_ofstream@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_ofstream_short_vtable;

/* ??_8?$basic_ifstream@DU?$char_traits@D@std@@@std@@7B@ */
const int basic_ifstream_char_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_ifstream_char, 0)};
/* ??_7?$basic_ifstream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_ifstream_char_vtable;

/* ??_8?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@7B@ */
const int basic_ifstream_wchar_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_ifstream_wchar, 0)};
/* ??_7?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr basic_ifstream_wchar_vtable;

/* ??_8?$basic_ifstream@GU?$char_traits@G@std@@@std@@7B@ */
const int basic_ifstream_short_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_ifstream_wchar, 0)};
/* ??_7?$basic_ifstream@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_ifstream_short_vtable;

/* ??_8?$basic_fstream@DU?$char_traits@D@std@@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@ */
const int basic_fstream_char_vbtable1[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_fstream_char, 0)};
/* ??_8?$basic_fstream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@ */
const int basic_fstream_char_vbtable2[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_fstream_char, FIELD_OFFSET(basic_fstream_char, base.base2))};
/* ??_7?$basic_fstream@DU?$char_traits@D@std@@@std@@6B@ */
extern const vtable_ptr basic_fstream_char_vtable;

/* ??_8?$basic_fstream@_WU?$char_traits@_W@std@@@std@@7B?$basic_istream@_WU?$char_traits@_W@std@@@1@@ */
/* ??_8?$basic_fstream@GU?$char_traits@G@std@@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@ */
const int basic_fstream_wchar_vbtable1[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_fstream_wchar, 0)};
/* ??_8?$basic_fstream@_WU?$char_traits@_W@std@@@std@@7B?$basic_ostream@_WU?$char_traits@_W@std@@@1@@ */
/* ??_8?$basic_fstream@GU?$char_traits@G@std@@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@ */
const int basic_fstream_wchar_vbtable2[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_fstream_wchar, FIELD_OFFSET(basic_fstream_wchar, base.base2))};
/* ??_7?$basic_fstream@_WU?$char_traits@_W@std@@@std@@6B@ */
extern const vtable_ptr basic_fstream_wchar_vtable;
/* ??_7?$basic_fstream@GU?$char_traits@G@std@@@std@@6B@ */
extern const vtable_ptr basic_fstream_short_vtable;

/* ??_8?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B@ */
const int basic_ostringstream_char_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_ostringstream_char, 0)};
/* ??_7?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@6B@ */
extern const vtable_ptr basic_ostringstream_char_vtable;

/* ??_8?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@7B@ */
const int basic_ostringstream_wchar_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_ostringstream_wchar, 0)};
/* ??_7?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@6B@ */
extern const vtable_ptr basic_ostringstream_wchar_vtable;

/* ??_8?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B@ */
const int basic_ostringstream_short_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_ostringstream_wchar, 0)};
/* ??_7?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@6B@ */
extern const vtable_ptr basic_ostringstream_short_vtable;

/* ??_8?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B@ */
const int basic_istringstream_char_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_istringstream_char, 0)};
/* ??_7?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@6B@ */
extern const vtable_ptr basic_istringstream_char_vtable;

/* ??_8?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@7B@ */
const int basic_istringstream_wchar_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_istringstream_wchar, 0)};
/* ??_7?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@6B@ */
extern const vtable_ptr basic_istringstream_wchar_vtable;

/* ??_8?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B@ */
const int basic_istringstream_short_vbtable[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_istringstream_wchar, 0)};
/* ??_7?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@6B@ */
extern const vtable_ptr basic_istringstream_short_vtable;

/* ??_8?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B?$basic_istream@DU?$char_traits@D@std@@@1@@ */
const int basic_stringstream_char_vbtable1[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_stringstream_char, 0)};
/* ??_8?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@ */
const int basic_stringstream_char_vbtable2[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_stringstream_char, FIELD_OFFSET(basic_stringstream_char, base.base2))};
/* ??_7?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@6B@ */
extern const vtable_ptr basic_stringstream_char_vtable;

/* ??_8?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@7B?$basic_istream@_WU?$char_traits@_W@std@@@1@@ */
const int basic_stringstream_wchar_vbtable1[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_stringstream_wchar, 0)};
/* ??_8?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@7B?$basic_ostream@_WU?$char_traits@_W@std@@@1@@ */
const int basic_stringstream_wchar_vbtable2[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_stringstream_wchar, FIELD_OFFSET(basic_stringstream_wchar, base.base2))};
/* ??_7?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@6B@ */
extern const vtable_ptr basic_stringstream_wchar_vtable;

/* ??_8?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@ */
const int basic_stringstream_short_vbtable1[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_stringstream_wchar, 0)};
/* ??_8?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@ */
const int basic_stringstream_short_vbtable2[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(basic_stringstream_wchar, FIELD_OFFSET(basic_stringstream_wchar, base.base2))};
/* ??_7?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@6B@ */
extern const vtable_ptr basic_stringstream_short_vtable;

/* ??_7strstreambuf@std@@6B */
extern const vtable_ptr strstreambuf_vtable;

static const int ostrstream_vbtable[] = {0, VBTABLE_BASIC_IOS_ENTRY(ostrstream, 0)};
extern const vtable_ptr ostrstream_vtable;

static const int istrstream_vbtable[] = {0, VBTABLE_BASIC_IOS_ENTRY(istrstream, 0)};

static const int strstream_vbtable1[] = {0, VBTABLE_BASIC_IOS_ENTRY(strstream, 0)};
static const int strstream_vbtable2[] = {0,
    VBTABLE_BASIC_IOS_ENTRY(strstream, FIELD_OFFSET(strstream, base.base2))};
extern const vtable_ptr strstream_vtable;

DEFINE_RTTI_DATA0(iosb, 0, ".?AV?$_Iosb@H@std@@")
DEFINE_RTTI_DATA1(ios_base, 0, &iosb_rtti_base_descriptor, ".?AV?$_Iosb@H@std@@")
DEFINE_RTTI_DATA2(basic_ios_char, 0, &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ios@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA2(basic_ios_wchar, 0, &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ios@_WU?$char_traits@_W@std@@@std@@")
DEFINE_RTTI_DATA2(basic_ios_short, 0, &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ios@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA0(basic_streambuf_char, 0,
        ".?AV?$basic_streambuf@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA0(basic_streambuf_wchar, 0,
        ".?AV?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@")
DEFINE_RTTI_DATA0(basic_streambuf_short, 0,
        ".?AV?$basic_streambuf@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA1(basic_filebuf_char, 0, &basic_streambuf_char_rtti_base_descriptor,
        ".?AV?$basic_filebuf@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA1(basic_filebuf_wchar, 0, &basic_streambuf_wchar_rtti_base_descriptor,
        ".?AV?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@")
DEFINE_RTTI_DATA1(basic_filebuf_short, 0, &basic_streambuf_short_rtti_base_descriptor,
        ".?AV?$basic_filebuf@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA1(basic_stringbuf_char, 0, &basic_streambuf_char_rtti_base_descriptor,
        ".?AV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@")
DEFINE_RTTI_DATA1(basic_stringbuf_wchar, 0, &basic_streambuf_wchar_rtti_base_descriptor,
        ".?AV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@")
DEFINE_RTTI_DATA1(basic_stringbuf_short, 0, &basic_streambuf_short_rtti_base_descriptor,
        ".?AV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@")
DEFINE_RTTI_DATA3(basic_ostream_char, sizeof(basic_ostream_char), &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostream@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA3(basic_ostream_wchar, sizeof(basic_ostream_wchar), &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostream@_WU?$char_traits@_W@std@@@std@@")
DEFINE_RTTI_DATA3(basic_ostream_short, sizeof(basic_ostream_wchar), &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostream@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA3(basic_istream_char, sizeof(basic_istream_char), &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istream@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA3(basic_istream_wchar, sizeof(basic_istream_wchar), &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istream@_WU?$char_traits@_W@std@@@std@@")
DEFINE_RTTI_DATA3(basic_istream_short, sizeof(basic_istream_wchar), &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istream@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA8(basic_iostream_char, sizeof(basic_iostream_char),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_iostream@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA8(basic_iostream_wchar, sizeof(basic_iostream_wchar),
        &basic_istream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_iostream@_WU?$char_traits@_W@std@@@std@@")
DEFINE_RTTI_DATA8(basic_iostream_short, sizeof(basic_iostream_wchar),
        &basic_istream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_iostream@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA4(basic_ofstream_char, sizeof(basic_ofstream_char),
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ofstream@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA4(basic_ofstream_wchar, sizeof(basic_ofstream_wchar),
        &basic_ostream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@")
DEFINE_RTTI_DATA4(basic_ofstream_short, sizeof(basic_ofstream_wchar),
        &basic_ostream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ofstream@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA4(basic_ifstream_char, sizeof(basic_ifstream_char),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ifstream@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA4(basic_ifstream_wchar, sizeof(basic_ifstream_wchar),
        &basic_istream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@")
DEFINE_RTTI_DATA4(basic_ifstream_short, sizeof(basic_ifstream_wchar),
        &basic_istream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ifstream@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA8(basic_fstream_char, sizeof(basic_fstream_char),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_fstream@DU?$char_traits@D@std@@@std@@")
DEFINE_RTTI_DATA8(basic_fstream_wchar, sizeof(basic_fstream_wchar),
        &basic_istream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_fstream@_WU?$char_traits@_W@std@@@std@@")
DEFINE_RTTI_DATA8(basic_fstream_short, sizeof(basic_fstream_wchar),
        &basic_istream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_fstream@GU?$char_traits@G@std@@@std@@")
DEFINE_RTTI_DATA4(basic_ostringstream_char, sizeof(basic_ostringstream_char),
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@")
DEFINE_RTTI_DATA4(basic_ostringstream_wchar, sizeof(basic_ostringstream_wchar),
        &basic_ostream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@")
DEFINE_RTTI_DATA4(basic_ostringstream_short, sizeof(basic_ostringstream_wchar),
        &basic_ostream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@")
DEFINE_RTTI_DATA4(basic_istringstream_char, sizeof(basic_istringstream_char),
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@")
DEFINE_RTTI_DATA4(basic_istringstream_wchar, sizeof(basic_istringstream_wchar),
        &basic_ostream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@")
DEFINE_RTTI_DATA4(basic_istringstream_short, sizeof(basic_istringstream_wchar),
        &basic_ostream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@")
DEFINE_RTTI_DATA8(basic_stringstream_char, sizeof(basic_stringstream_char),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@")
DEFINE_RTTI_DATA8(basic_stringstream_wchar, sizeof(basic_stringstream_wchar),
        &basic_istream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_wchar_rtti_base_descriptor, &basic_ios_wchar_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@")
DEFINE_RTTI_DATA8(basic_stringstream_short, sizeof(basic_stringstream_wchar),
        &basic_istream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_short_rtti_base_descriptor, &basic_ios_short_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        ".?AV?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@")
DEFINE_RTTI_DATA1(strstreambuf, sizeof(strstreambuf),
        &basic_streambuf_char_rtti_base_descriptor, ".?AVstrstreambuf@std@@")
DEFINE_RTTI_DATA4(ostrstream, sizeof(ostrstream),
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        "?AVostrstream@std@@")
DEFINE_RTTI_DATA8(strstream, sizeof(strstream),
        &basic_istream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        &basic_ostream_char_rtti_base_descriptor, &basic_ios_char_rtti_base_descriptor,
        &ios_base_rtti_base_descriptor, &iosb_rtti_base_descriptor,
        "?AVstrstream@std@@")

__ASM_BLOCK_BEGIN(ios_vtables)
    __ASM_VTABLE(iosb,
            VTABLE_ADD_FUNC(iosb_vector_dtor));
    __ASM_VTABLE(ios_base,
            VTABLE_ADD_FUNC(ios_base_vector_dtor));
    __ASM_VTABLE(basic_ios_char,
            VTABLE_ADD_FUNC(basic_ios_char_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_ios_wchar,
            VTABLE_ADD_FUNC(basic_ios_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_ios_short,
            VTABLE_ADD_FUNC(basic_ios_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_streambuf_char,
            VTABLE_ADD_FUNC(basic_streambuf_char_vector_dtor)
#if _MSVCP_VER >= 100
            VTABLE_ADD_FUNC(basic_streambuf_char__Lock)
            VTABLE_ADD_FUNC(basic_streambuf_char__Unlock)
#endif
            VTABLE_ADD_FUNC(basic_streambuf_char_overflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_char_showmanyc)
            VTABLE_ADD_FUNC(basic_streambuf_char_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsgetn)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
            VTABLE_ADD_FUNC(basic_streambuf_char__Xsgetn_s)
#endif
            VTABLE_ADD_FUNC(basic_streambuf_char_xsputn)
            VTABLE_ADD_FUNC(basic_streambuf_char_seekoff)
            VTABLE_ADD_FUNC(basic_streambuf_char_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_char_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_char_sync)
            VTABLE_ADD_FUNC(basic_streambuf_char_imbue));
    __ASM_VTABLE(basic_streambuf_wchar,
            VTABLE_ADD_FUNC(basic_streambuf_wchar_vector_dtor)
#if _MSVCP_VER >= 100
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Lock)
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Unlock)
#endif
            VTABLE_ADD_FUNC(basic_streambuf_wchar_overflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_showmanyc)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsgetn)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Xsgetn_s)
#endif
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsputn)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_seekoff)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_sync)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_imbue));
    __ASM_VTABLE(basic_streambuf_short,
            VTABLE_ADD_FUNC(basic_streambuf_wchar_vector_dtor)
#if _MSVCP_VER >= 100
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Lock)
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Unlock)
#endif
            VTABLE_ADD_FUNC(basic_streambuf_wchar_overflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_showmanyc)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsgetn)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Xsgetn_s)
#endif
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsputn)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_seekoff)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_sync)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_imbue));
    __ASM_VTABLE(basic_filebuf_char,
            VTABLE_ADD_FUNC(basic_filebuf_char_vector_dtor)
#if _MSVCP_VER >= 100
            VTABLE_ADD_FUNC(basic_streambuf_char__Lock)
            VTABLE_ADD_FUNC(basic_streambuf_char__Unlock)
#endif
            VTABLE_ADD_FUNC(basic_filebuf_char_overflow)
            VTABLE_ADD_FUNC(basic_filebuf_char_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_char_showmanyc)
            VTABLE_ADD_FUNC(basic_filebuf_char_underflow)
            VTABLE_ADD_FUNC(basic_filebuf_char_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsgetn)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
            VTABLE_ADD_FUNC(basic_streambuf_char__Xsgetn_s)
#endif
            VTABLE_ADD_FUNC(basic_streambuf_char_xsputn)
            VTABLE_ADD_FUNC(basic_filebuf_char_seekoff)
            VTABLE_ADD_FUNC(basic_filebuf_char_seekpos)
            VTABLE_ADD_FUNC(basic_filebuf_char_setbuf)
            VTABLE_ADD_FUNC(basic_filebuf_char_sync)
            VTABLE_ADD_FUNC(basic_filebuf_char_imbue));
    __ASM_VTABLE(basic_filebuf_wchar,
            VTABLE_ADD_FUNC(basic_filebuf_wchar_vector_dtor)
#if _MSVCP_VER >= 100
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Lock)
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Unlock)
#endif
            VTABLE_ADD_FUNC(basic_filebuf_wchar_overflow)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_showmanyc)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_underflow)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsgetn)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Xsgetn_s)
#endif
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsputn)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_seekoff)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_seekpos)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_setbuf)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_sync)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_imbue));
    __ASM_VTABLE(basic_filebuf_short,
            VTABLE_ADD_FUNC(basic_filebuf_wchar_vector_dtor)
#if _MSVCP_VER >= 100
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Lock)
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Unlock)
#endif
            VTABLE_ADD_FUNC(basic_filebuf_wchar_overflow)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_showmanyc)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_underflow)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsgetn)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Xsgetn_s)
#endif
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsputn)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_seekoff)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_seekpos)
            VTABLE_ADD_FUNC(basic_filebuf_short_setbuf)
            VTABLE_ADD_FUNC(basic_filebuf_wchar_sync)
            VTABLE_ADD_FUNC(basic_filebuf_short_imbue));
    __ASM_VTABLE(basic_stringbuf_char,
            VTABLE_ADD_FUNC(basic_stringbuf_char_vector_dtor)
            VTABLE_ADD_FUNC(basic_stringbuf_char_overflow)
            VTABLE_ADD_FUNC(basic_stringbuf_char_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_char_showmanyc)
            VTABLE_ADD_FUNC(basic_stringbuf_char_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsgetn)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
            VTABLE_ADD_FUNC(basic_streambuf_char__Xsgetn_s)
#endif
            VTABLE_ADD_FUNC(basic_streambuf_char_xsputn)
            VTABLE_ADD_FUNC(basic_stringbuf_char_seekoff)
            VTABLE_ADD_FUNC(basic_stringbuf_char_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_char_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_char_sync)
            VTABLE_ADD_FUNC(basic_streambuf_char_imbue));
    __ASM_VTABLE(basic_stringbuf_wchar,
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_vector_dtor)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_overflow)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_showmanyc)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsgetn)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Xsgetn_s)
#endif
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsputn)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_seekoff)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_sync)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_imbue));
    __ASM_VTABLE(basic_stringbuf_short,
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_vector_dtor)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_overflow)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_showmanyc)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsgetn)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
            VTABLE_ADD_FUNC(basic_streambuf_wchar__Xsgetn_s)
#endif
            VTABLE_ADD_FUNC(basic_streambuf_wchar_xsputn)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_seekoff)
            VTABLE_ADD_FUNC(basic_stringbuf_wchar_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_sync)
            VTABLE_ADD_FUNC(basic_streambuf_wchar_imbue));
    __ASM_VTABLE(basic_ostream_char,
            VTABLE_ADD_FUNC(basic_ostream_char_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_ostream_wchar,
            VTABLE_ADD_FUNC(basic_ostream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_ostream_short,
            VTABLE_ADD_FUNC(basic_ostream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_istream_char,
            VTABLE_ADD_FUNC(basic_istream_char_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_istream_wchar,
            VTABLE_ADD_FUNC(basic_istream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_istream_short,
            VTABLE_ADD_FUNC(basic_istream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_iostream_char,
            VTABLE_ADD_FUNC(basic_iostream_char_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_iostream_wchar,
            VTABLE_ADD_FUNC(basic_iostream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_iostream_short,
            VTABLE_ADD_FUNC(basic_iostream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_ofstream_char,
            VTABLE_ADD_FUNC(basic_ofstream_char_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_ofstream_wchar,
            VTABLE_ADD_FUNC(basic_ofstream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_ofstream_short,
            VTABLE_ADD_FUNC(basic_ofstream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_ifstream_char,
            VTABLE_ADD_FUNC(basic_ifstream_char_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_ifstream_wchar,
            VTABLE_ADD_FUNC(basic_ifstream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_ifstream_short,
            VTABLE_ADD_FUNC(basic_ifstream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_fstream_char,
            VTABLE_ADD_FUNC(basic_fstream_char_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_fstream_wchar,
            VTABLE_ADD_FUNC(basic_fstream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_fstream_short,
            VTABLE_ADD_FUNC(basic_fstream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_ostringstream_char,
            VTABLE_ADD_FUNC(basic_ostringstream_char_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_ostringstream_wchar,
            VTABLE_ADD_FUNC(basic_ostringstream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_ostringstream_short,
            VTABLE_ADD_FUNC(basic_ostringstream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_istringstream_char,
            VTABLE_ADD_FUNC(basic_istringstream_char_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_istringstream_wchar,
            VTABLE_ADD_FUNC(basic_istringstream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_istringstream_short,
            VTABLE_ADD_FUNC(basic_istringstream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_stringstream_char,
            VTABLE_ADD_FUNC(basic_stringstream_char_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_stringstream_wchar,
            VTABLE_ADD_FUNC(basic_stringstream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(basic_stringstream_short,
            VTABLE_ADD_FUNC(basic_stringstream_wchar_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(strstreambuf,
            VTABLE_ADD_FUNC(strstreambuf_vector_dtor)
            VTABLE_ADD_FUNC(strstreambuf_overflow)
            VTABLE_ADD_FUNC(strstreambuf_pbackfail)
            VTABLE_ADD_FUNC(basic_streambuf_char_showmanyc)
            VTABLE_ADD_FUNC(strstreambuf_underflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_uflow)
            VTABLE_ADD_FUNC(basic_streambuf_char_xsgetn)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
            VTABLE_ADD_FUNC(basic_streambuf_char__Xsgetn_s)
#endif
            VTABLE_ADD_FUNC(basic_streambuf_char_xsputn)
            VTABLE_ADD_FUNC(strstreambuf_seekoff)
            VTABLE_ADD_FUNC(strstreambuf_seekpos)
            VTABLE_ADD_FUNC(basic_streambuf_char_setbuf)
            VTABLE_ADD_FUNC(basic_streambuf_char_sync)
            VTABLE_ADD_FUNC(basic_streambuf_char_imbue));
    __ASM_VTABLE(ostrstream,
            VTABLE_ADD_FUNC(ostrstream_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
    __ASM_VTABLE(strstream,
            VTABLE_ADD_FUNC(strstream_vector_dtor)
#if _MSVCP_VER == 110
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp1)
            VTABLE_ADD_FUNC(basic_ios__Add_vtordisp2)
#endif
            );
__ASM_BLOCK_END

/* ?setp@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXPAD00@Z */
/* ?setp@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXPEAD00@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_setp_next, 16)
void __thiscall basic_streambuf_char_setp_next(basic_streambuf_char *this, char *first, char *next, char *last)
{
    TRACE("(%p %p %p %p)\n", this, first, next, last);

    this->wbuf = first;
    this->wpos = next;
    this->wsize = last-next;
}

/* ?setp@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXPAD0@Z */
/* ?setp@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXPEAD0@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_setp, 12)
void __thiscall basic_streambuf_char_setp(basic_streambuf_char *this, char *first, char *last)
{
    basic_streambuf_char_setp_next(this, first, first, last);
}

/* ?setg@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXPAD00@Z */
/* ?setg@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXPEAD00@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_setg, 16)
void __thiscall basic_streambuf_char_setg(basic_streambuf_char *this, char *first, char *next, char *last)
{
    TRACE("(%p %p %p %p)\n", this, first, next, last);

    this->rbuf = first;
    this->rpos = next;
    this->rsize = last-next;
}

/* ?_Init@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXXZ */
/* ?_Init@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Init_empty, 4)
void __thiscall basic_streambuf_char__Init_empty(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);

    this->prbuf = &this->rbuf;
    this->pwbuf = &this->wbuf;
    this->prpos = &this->rpos;
    this->pwpos = &this->wpos;
    this->prsize = &this->rsize;
    this->pwsize = &this->wsize;

    basic_streambuf_char_setp(this, NULL, NULL);
    basic_streambuf_char_setg(this, NULL, NULL, NULL);
}

/* ??0?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_ctor_uninitialized, 8)
basic_streambuf_char* __thiscall basic_streambuf_char_ctor_uninitialized(basic_streambuf_char *this, int uninitialized)
{
    TRACE("(%p %d)\n", this, uninitialized);
    this->vtable = &basic_streambuf_char_vtable;
#if _MSVCP_VER >= 70 && _MSVCP_VER <= 100
    mutex_ctor(&this->lock);
#endif
    return this;
}

/* ??0?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAE@XZ */
/* ??0?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_ctor, 4)
basic_streambuf_char* __thiscall basic_streambuf_char_ctor(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);

    this->vtable = &basic_streambuf_char_vtable;
#if _MSVCP_VER >= 70 && _MSVCP_VER <= 100
    mutex_ctor(&this->lock);
#endif
#if _MSVCP_VER >= 70
    this->loc = operator_new(sizeof(locale));
#endif
    locale_ctor(IOS_LOCALE(this));
    basic_streambuf_char__Init_empty(this);

    return this;
}

/* ??1?$basic_streambuf@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_streambuf@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_dtor, 4)
void __thiscall basic_streambuf_char_dtor(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);

#if _MSVCP_VER >= 70 && _MSVCP_VER <= 100
    mutex_dtor(&this->lock);
#endif
    locale_dtor(IOS_LOCALE(this));
#if _MSVCP_VER >= 70
    operator_delete(this->loc);
#endif
}

DEFINE_THISCALL_WRAPPER(basic_streambuf_char_vector_dtor, 8)
basic_streambuf_char* __thiscall basic_streambuf_char_vector_dtor(basic_streambuf_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_streambuf_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_streambuf_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Gnavail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEHXZ */
/* ?_Gnavail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Gnavail, 4)
streamsize __thiscall basic_streambuf_char__Gnavail(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos ? *this->prsize : 0;
}

/* ?_Gndec@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEPADXZ */
/* ?_Gndec@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Gndec, 4)
char* __thiscall basic_streambuf_char__Gndec(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)++;
    (*this->prpos)--;
    return *this->prpos;
}

/* ?_Gninc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEPADXZ */
/* ?_Gninc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Gninc, 4)
char* __thiscall basic_streambuf_char__Gninc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)--;
    return (*this->prpos)++;
}

/* ?_Gnpreinc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEPADXZ */
/* ?_Gnpreinc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Gnpreinc, 4)
char* __thiscall basic_streambuf_char__Gnpreinc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)--;
    (*this->prpos)++;
    return *this->prpos;
}

/* ?_Init@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXPAPAD0PAH001@Z */
/* ?_Init@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXPEAPEAD0PEAH001@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Init, 28)
void __thiscall basic_streambuf_char__Init(basic_streambuf_char *this, char **gf, char **gn, int *gc, char **pf, char **pn, int *pc)
{
    TRACE("(%p %p %p %p %p %p %p)\n", this, gf, gn, gc, pf, pn, pc);

    this->prbuf = gf;
    this->pwbuf = pf;
    this->prpos = gn;
    this->pwpos = pn;
    this->prsize = gc;
    this->pwsize = pc;
}

/* ?_Lock@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?_Lock@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Lock, 4)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char__Lock(this) CALL_VTBL_FUNC(this, 4, void, (basic_streambuf_char*), (this))
#else
#define call_basic_streambuf_char__Lock(this) basic_streambuf_char__Lock(this)
#endif
void __thiscall basic_streambuf_char__Lock(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
#if _MSVCP_VER >= 70 && _MSVCP_VER <= 100
    mutex_lock(&this->lock);
#endif
}

/* ?_Pnavail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEHXZ */
/* ?_Pnavail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Pnavail, 4)
streamsize __thiscall basic_streambuf_char__Pnavail(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos ? *this->pwsize : 0;
}

/* ?_Pninc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEPADXZ */
/* ?_Pninc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Pninc, 4)
char* __thiscall basic_streambuf_char__Pninc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    (*this->pwsize)--;
    return (*this->pwpos)++;
}

/* ?underflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?underflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_underflow, 4)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char_underflow(this) CALL_VTBL_FUNC(this, 24, int, (basic_streambuf_char*), (this))
#else
#define call_basic_streambuf_char_underflow(this) CALL_VTBL_FUNC(this, 16, int, (basic_streambuf_char*), (this))
#endif
int __thiscall basic_streambuf_char_underflow(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return EOF;
}

/* ?uflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?uflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_uflow, 4)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char_uflow(this) CALL_VTBL_FUNC(this, 28, int, (basic_streambuf_char*), (this))
#else
#define call_basic_streambuf_char_uflow(this) CALL_VTBL_FUNC(this, 20, int, (basic_streambuf_char*), (this))
#endif
int __thiscall basic_streambuf_char_uflow(basic_streambuf_char *this)
{
    int ret;

    TRACE("(%p)\n", this);

    if(call_basic_streambuf_char_underflow(this)==EOF)
        return EOF;

    ret = (unsigned char)**this->prpos;
    (*this->prsize)--;
    (*this->prpos)++;
    return ret;
}

/* ?_Xsgetn_s@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHPADIH@Z */
/* ?_Xsgetn_s@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA_JPEAD_K_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Xsgetn_s, 20)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Xsgetn_s, 16)
#endif
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
#define call_basic_streambuf_char__Xsgetn_s(this, ptr, size, count) CALL_VTBL_FUNC(this, 28, \
        streamsize, (basic_streambuf_char*, char*, size_t, streamsize), (this, ptr, size, count))
#else
#define call_basic_streambuf_char__Xsgetn_s(this, ptr, size, count) basic_streambuf_char__Xsgetn_s(this, ptr, size, count)
#endif
streamsize __thiscall basic_streambuf_char__Xsgetn_s(basic_streambuf_char *this, char *ptr, size_t size, streamsize count)
{
    streamsize copied, chunk;
    int c;

    TRACE("(%p %p %Iu %s)\n", this, ptr, size, wine_dbgstr_longlong(count));

    for(copied=0; copied<count && size;) {
        chunk = basic_streambuf_char__Gnavail(this);
        if(chunk > count-copied)
            chunk = count-copied;

        if(chunk > 0) {
            memcpy_s(ptr+copied, size, *this->prpos, chunk);
            *this->prpos += chunk;
            *this->prsize -= chunk;
            copied += chunk;
            size -= chunk;
        }else if((c = call_basic_streambuf_char_uflow(this)) != EOF) {
            ptr[copied] = c;
            copied++;
            size--;
        }else {
            break;
        }
    }

    return copied;
}

/* ?_Sgetn_s@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHPADIH@Z */
/* ?_Sgetn_s@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA_JPEAD_K_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Sgetn_s, 20)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Sgetn_s, 16)
#endif
streamsize __thiscall basic_streambuf_char__Sgetn_s(basic_streambuf_char *this, char *ptr, size_t size, streamsize count)
{
    TRACE("(%p %p %Iu %s)\n", this, ptr, size, wine_dbgstr_longlong(count));
    return call_basic_streambuf_char__Xsgetn_s(this, ptr, size, count);
}

/* ?_Unlock@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?_Unlock@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char__Unlock, 4)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char__Unlock(this) CALL_VTBL_FUNC(this, 8, void, (basic_streambuf_char*), (this))
#else
#define call_basic_streambuf_char__Unlock(this) basic_streambuf_char__Unlock(this)
#endif
void __thiscall basic_streambuf_char__Unlock(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
#if _MSVCP_VER >= 70 && _MSVCP_VER <= 100
    mutex_unlock(&this->lock);
#endif
}

/* ?eback@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEPADXZ */
/* ?eback@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_eback, 4)
char* __thiscall basic_streambuf_char_eback(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->prbuf;
}

/* ?gptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEPADXZ */
/* ?gptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_gptr, 4)
char* __thiscall basic_streambuf_char_gptr(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos;
}

/* ?egptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEPADXZ */
/* ?egptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_egptr, 4)
char* __thiscall basic_streambuf_char_egptr(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos+*this->prsize;
}

/* ?epptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEPADXZ */
/* ?epptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_epptr, 4)
char* __thiscall basic_streambuf_char_epptr(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos+*this->pwsize;
}

/* ?gbump@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXH@Z */
/* ?gbump@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_gbump, 8)
void __thiscall basic_streambuf_char_gbump(basic_streambuf_char *this, int off)
{
    TRACE("(%p %d)\n", this, off);
    *this->prpos += off;
    *this->prsize -= off;
}

/* ?getloc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AVlocale@2@XZ */
/* ?getloc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QBE?AVlocale@2@XZ */
/* ?getloc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AVlocale@2@XZ */
/* ?getloc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEBA?AVlocale@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_getloc, 8)
locale* __thiscall basic_streambuf_char_getloc(const basic_streambuf_char *this, locale *ret)
{
    TRACE("(%p)\n", this);
    return locale_copy_ctor(ret, IOS_LOCALE(this));
}

/* ?imbue@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEXABVlocale@2@@Z */
/* ?imbue@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAXAEBVlocale@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_imbue, 8)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char_imbue(this, loc) CALL_VTBL_FUNC(this, 56, void, (basic_streambuf_char*, const locale*), (this, loc))
#elif _MSVCP_VER >= 80
#define call_basic_streambuf_char_imbue(this, loc) CALL_VTBL_FUNC(this, 52, void, (basic_streambuf_char*, const locale*), (this, loc))
#else
#define call_basic_streambuf_char_imbue(this, loc) CALL_VTBL_FUNC(this, 48, void, (basic_streambuf_char*, const locale*), (this, loc))
#endif
void __thiscall basic_streambuf_char_imbue(basic_streambuf_char *this, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
}

/* ?overflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHH@Z */
/* ?overflow@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_overflow, 8)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char_overflow(this, ch) CALL_VTBL_FUNC(this, 12, int, (basic_streambuf_char*, int), (this, ch))
#else
#define call_basic_streambuf_char_overflow(this, ch) CALL_VTBL_FUNC(this, 4, int, (basic_streambuf_char*, int), (this, ch))
#endif
int __thiscall basic_streambuf_char_overflow(basic_streambuf_char *this, int ch)
{
    TRACE("(%p %d)\n", this, ch);
    return EOF;
}

/* ?pbackfail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHH@Z */
/* ?pbackfail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pbackfail, 8)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char_pbackfail(this, ch) CALL_VTBL_FUNC(this, 16, int, (basic_streambuf_char*, int), (this, ch))
#else
#define call_basic_streambuf_char_pbackfail(this, ch) CALL_VTBL_FUNC(this, 8, int, (basic_streambuf_char*, int), (this, ch))
#endif
int __thiscall basic_streambuf_char_pbackfail(basic_streambuf_char *this, int ch)
{
    TRACE("(%p %d)\n", this, ch);
    return EOF;
}

/* ?pbase@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEPADXZ */
/* ?pbase@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pbase, 4)
char* __thiscall basic_streambuf_char_pbase(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->pwbuf;
}

/* ?pbump@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXH@Z */
/* ?pbump@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pbump, 8)
void __thiscall basic_streambuf_char_pbump(basic_streambuf_char *this, int off)
{
    TRACE("(%p %d)\n", this, off);
    *this->pwpos += off;
    *this->pwsize -= off;
}

/* ?pptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IBEPADXZ */
/* ?pptr@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEBAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pptr, 4)
char* __thiscall basic_streambuf_char_pptr(const basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos;
}

/* ?pubimbue@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?pubimbue@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubimbue, 12)
locale* __thiscall basic_streambuf_char_pubimbue(basic_streambuf_char *this, locale *ret, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
    memcpy(ret, IOS_LOCALE(this), sizeof(locale));
    call_basic_streambuf_char_imbue(this, loc);
    locale_copy_ctor(IOS_LOCALE(this), loc);
    return ret;
}

/* ?seekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_seekoff, 24)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_seekoff, 20)
#endif
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char_seekoff(this, ret, off, way, mode) \
        CALL_VTBL_FUNC(this, 40, fpos_mbstatet*, (basic_streambuf_char*, fpos_mbstatet*, streamoff, int, int), (this, ret, off, way, mode))
#elif _MSVCP_VER >= 80
#define call_basic_streambuf_char_seekoff(this, ret, off, way, mode) \
        CALL_VTBL_FUNC(this, 36, fpos_mbstatet*, (basic_streambuf_char*, fpos_mbstatet*, streamoff, int, int), (this, ret, off, way, mode))
#else
#define call_basic_streambuf_char_seekoff(this, ret, off, way, mode) \
        CALL_VTBL_FUNC(this, 32, fpos_mbstatet*, (basic_streambuf_char*, fpos_mbstatet*, streamoff, int, int), (this, ret, off, way, mode))
#endif
fpos_mbstatet* __thiscall basic_streambuf_char_seekoff(basic_streambuf_char *this,
        fpos_mbstatet *ret, streamoff off, int way, int mode)
{
    TRACE("(%p %s %d %d)\n", this, wine_dbgstr_longlong(off), way, mode);
    ret->off = -1;
    ret->pos = 0;
    memset(&ret->state, 0, sizeof(ret->state));
    return ret;
}

/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@JHH@Z */
/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@_JHH@Z */
/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@JFF@Z */
/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@_JFF@Z */
/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubseekoff, 24)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubseekoff, 20)
#endif
fpos_mbstatet* __thiscall basic_streambuf_char_pubseekoff(basic_streambuf_char *this,
        fpos_mbstatet *ret, streamoff off, int way, int mode)
{
    TRACE("(%p %s %d %d)\n", this, wine_dbgstr_longlong(off), way, mode);
    return call_basic_streambuf_char_seekoff(this, ret, off, way, mode);
}

/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@JII@Z */
/* ?pubseekoff@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@_JII@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubseekoff_old, 24)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubseekoff_old, 20)
#endif
fpos_mbstatet* __thiscall basic_streambuf_char_pubseekoff_old(basic_streambuf_char *this,
        fpos_mbstatet *ret, streamoff off, unsigned int way, unsigned int mode)
{
    TRACE("(%p %s %d %d)\n", this, wine_dbgstr_longlong(off), way, mode);
    return basic_streambuf_char_pubseekoff(this, ret, off, way, mode);
}

/* ?seekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_seekpos, 36)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char_seekpos(this, ret, pos, mode) \
        CALL_VTBL_FUNC(this, 44, fpos_mbstatet*, (basic_streambuf_char*, fpos_mbstatet*, fpos_mbstatet, int), (this, ret, pos, mode))
#elif _MSVCP_VER >= 80
#define call_basic_streambuf_char_seekpos(this, ret, pos, mode) \
        CALL_VTBL_FUNC(this, 40, fpos_mbstatet*, (basic_streambuf_char*, fpos_mbstatet*, fpos_mbstatet, int), (this, ret, pos, mode))
#else
#define call_basic_streambuf_char_seekpos(this, ret, pos, mode) \
        CALL_VTBL_FUNC(this, 36, fpos_mbstatet*, (basic_streambuf_char*, fpos_mbstatet*, fpos_mbstatet, int), (this, ret, pos, mode))
#endif
fpos_mbstatet* __thiscall basic_streambuf_char_seekpos(basic_streambuf_char *this,
        fpos_mbstatet *ret, fpos_mbstatet pos, int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_mbstatet(&pos), mode);
    ret->off = -1;
    ret->pos = 0;
    memset(&ret->state, 0, sizeof(ret->state));
    return ret;
}

/* ?pubseekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@V32@H@Z */
/* ?pubseekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubseekpos, 36)
fpos_mbstatet* __thiscall basic_streambuf_char_pubseekpos(basic_streambuf_char *this,
        fpos_mbstatet *ret, fpos_mbstatet pos, int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_mbstatet(&pos), mode);
    return call_basic_streambuf_char_seekpos(this, ret, pos, mode);
}

/* ?pubseekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@V32@I@Z */
/* ?pubseekpos@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@V32@I@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubseekpos_old, 36)
fpos_mbstatet* __thiscall basic_streambuf_char_pubseekpos_old(basic_streambuf_char *this,
        fpos_mbstatet *ret, fpos_mbstatet pos, unsigned int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_mbstatet(&pos), mode);
    return basic_streambuf_char_pubseekpos(this, ret, pos, mode);
}

/* ?setbuf@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEPAV12@PADH@Z */
/* ?setbuf@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAPEAV12@PEAD_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_setbuf, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_setbuf, 12)
#endif
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char_setbuf(this, buf, count) CALL_VTBL_FUNC(this, 48, basic_streambuf_char*, (basic_streambuf_char*, char*, streamsize), (this, buf, count))
#elif _MSVCP_VER >= 80
#define call_basic_streambuf_char_setbuf(this, buf, count) CALL_VTBL_FUNC(this, 44, basic_streambuf_char*, (basic_streambuf_char*, char*, streamsize), (this, buf, count))
#else
#define call_basic_streambuf_char_setbuf(this, buf, count) CALL_VTBL_FUNC(this, 40, basic_streambuf_char*, (basic_streambuf_char*, char*, streamsize), (this, buf, count))
#endif
basic_streambuf_char* __thiscall basic_streambuf_char_setbuf(basic_streambuf_char *this, char *buf, streamsize count)
{
    TRACE("(%p %p %s)\n", this, buf, wine_dbgstr_longlong(count));
    return this;
}

/* ?pubsetbuf@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PADH@Z */
/* ?pubsetbuf@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEAD_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubsetbuf, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubsetbuf, 12)
#endif
basic_streambuf_char* __thiscall basic_streambuf_char_pubsetbuf(basic_streambuf_char *this, char *buf, streamsize count)
{
    TRACE("(%p %p %s)\n", this, buf, wine_dbgstr_longlong(count));
    return call_basic_streambuf_char_setbuf(this, buf, count);
}

/* ?sync@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?sync@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sync, 4)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char_sync(this) CALL_VTBL_FUNC(this, 52, int, (basic_streambuf_char*), (this))
#elif _MSVCP_VER >= 80
#define call_basic_streambuf_char_sync(this) CALL_VTBL_FUNC(this, 48, int, (basic_streambuf_char*), (this))
#else
#define call_basic_streambuf_char_sync(this) CALL_VTBL_FUNC(this, 44, int, (basic_streambuf_char*), (this))
#endif
int __thiscall basic_streambuf_char_sync(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return 0;
}

/* ?pubsync@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?pubsync@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_pubsync, 4)
int __thiscall basic_streambuf_char_pubsync(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return call_basic_streambuf_char_sync(this);
}

/* ?sgetn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHPADH@Z */
/* ?sgetn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA_JPEAD_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sgetn, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sgetn, 12)
#endif
streamsize __thiscall basic_streambuf_char_sgetn(basic_streambuf_char *this, char *ptr, streamsize count)
{
    TRACE("(%p %p %s)\n", this, ptr, wine_dbgstr_longlong(count));
    return call_basic_streambuf_char__Xsgetn_s(this, ptr, -1, count);
}

/* ?showmanyc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?showmanyc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_showmanyc, 4)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char_showmanyc(this) CALL_VTBL_FUNC(this, 20, streamsize, (basic_streambuf_char*), (this))
#else
#define call_basic_streambuf_char_showmanyc(this) CALL_VTBL_FUNC(this, 12, streamsize, (basic_streambuf_char*), (this))
#endif
streamsize __thiscall basic_streambuf_char_showmanyc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return 0;
}

/* ?in_avail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?in_avail@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_in_avail, 4)
streamsize __thiscall basic_streambuf_char_in_avail(basic_streambuf_char *this)
{
    streamsize ret;

    TRACE("(%p)\n", this);

    ret = basic_streambuf_char__Gnavail(this);
    return ret ? ret : call_basic_streambuf_char_showmanyc(this);
}

/* ?sputbackc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHD@Z */
/* ?sputbackc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHD@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sputbackc, 8)
int __thiscall basic_streambuf_char_sputbackc(basic_streambuf_char *this, char ch)
{
    TRACE("(%p %d)\n", this, ch);
    if(*this->prpos && *this->prpos>*this->prbuf && (*this->prpos)[-1]==ch) {
        (*this->prsize)++;
        (*this->prpos)--;
        return (unsigned char)ch;
    }

    return call_basic_streambuf_char_pbackfail(this, (unsigned char)ch);
}

/* ?sputc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHD@Z */
/* ?sputc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHD@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sputc, 8)
int __thiscall basic_streambuf_char_sputc(basic_streambuf_char *this, char ch)
{
    TRACE("(%p %d)\n", this, ch);
    return basic_streambuf_char__Pnavail(this) ?
        (unsigned char)(*basic_streambuf_char__Pninc(this) = ch) :
        call_basic_streambuf_char_overflow(this, (unsigned char)ch);
}

/* ?sungetc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?sungetc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sungetc, 4)
int __thiscall basic_streambuf_char_sungetc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    if(*this->prpos && *this->prpos>*this->prbuf) {
        (*this->prsize)++;
        (*this->prpos)--;
        return (unsigned char)**this->prpos;
    }

    return call_basic_streambuf_char_pbackfail(this, EOF);
}

/* ?stossc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?stossc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_stossc, 4)
void __thiscall basic_streambuf_char_stossc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    if(basic_streambuf_char__Gnavail(this))
        basic_streambuf_char__Gninc(this);
    else
        call_basic_streambuf_char_uflow(this);
}

/* ?sbumpc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?sbumpc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sbumpc, 4)
int __thiscall basic_streambuf_char_sbumpc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return basic_streambuf_char__Gnavail(this) ?
        (int)(unsigned char)*basic_streambuf_char__Gninc(this) : call_basic_streambuf_char_uflow(this);
}

/* ?sgetc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?sgetc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sgetc, 4)
int __thiscall basic_streambuf_char_sgetc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);
    return basic_streambuf_char__Gnavail(this) ?
        (int)(unsigned char)*basic_streambuf_char_gptr(this) : call_basic_streambuf_char_underflow(this);
}

/* ?snextc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?snextc@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_snextc, 4)
int __thiscall basic_streambuf_char_snextc(basic_streambuf_char *this)
{
    TRACE("(%p)\n", this);

    if(basic_streambuf_char__Gnavail(this) > 1)
        return (unsigned char)*basic_streambuf_char__Gnpreinc(this);
    return basic_streambuf_char_sbumpc(this)==EOF ?
        EOF : basic_streambuf_char_sgetc(this);
}

/* ?xsgetn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHPADH@Z */
/* ?xsgetn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA_JPEAD_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_xsgetn, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_xsgetn, 12)
#endif
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char_xsgetn(this, ptr, count) CALL_VTBL_FUNC(this, 32, streamsize, (basic_streambuf_char*, char*, streamsize), (this, ptr, count))
#else
#define call_basic_streambuf_char_xsgetn(this, ptr, count) CALL_VTBL_FUNC(this, 24, streamsize, (basic_streambuf_char*, char*, streamsize), (this, ptr, count))
#endif
streamsize __thiscall basic_streambuf_char_xsgetn(basic_streambuf_char *this, char *ptr, streamsize count)
{
    TRACE("(%p %p %s)\n", this, ptr, wine_dbgstr_longlong(count));
    return call_basic_streambuf_char__Xsgetn_s(this, ptr, -1, count);
}

/* ?xsputn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MAEHPBDH@Z */
/* ?xsputn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@MEAA_JPEBD_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_xsputn, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_xsputn, 12)
#endif
#if _MSVCP_VER >= 100
#define call_basic_streambuf_char_xsputn(this, ptr, count) CALL_VTBL_FUNC(this, 36, streamsize, (basic_streambuf_char*, const char*, streamsize), (this, ptr, count))
#elif _MSVCP_VER >= 80
#define call_basic_streambuf_char_xsputn(this, ptr, count) CALL_VTBL_FUNC(this, 32, streamsize, (basic_streambuf_char*, const char*, streamsize), (this, ptr, count))
#else
#define call_basic_streambuf_char_xsputn(this, ptr, count) CALL_VTBL_FUNC(this, 28, streamsize, (basic_streambuf_char*, const char*, streamsize), (this, ptr, count))
#endif
streamsize __thiscall basic_streambuf_char_xsputn(basic_streambuf_char *this, const char *ptr, streamsize count)
{
    streamsize copied, chunk;

    TRACE("(%p %p %s)\n", this, ptr, wine_dbgstr_longlong(count));

    for(copied=0; copied<count;) {
        chunk = basic_streambuf_char__Pnavail(this);
        if(chunk > count-copied)
            chunk = count-copied;

        if(chunk > 0) {
            memcpy(*this->pwpos, ptr+copied, chunk);
            *this->pwpos += chunk;
            *this->pwsize -= chunk;
            copied += chunk;
        }else if(call_basic_streambuf_char_overflow(this, (unsigned char)ptr[copied]) != EOF) {
            copied++;
        }else {
            break;
        }
    }

    return copied;
}

/* ?sputn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QAEHPBDH@Z */
/* ?sputn@?$basic_streambuf@DU?$char_traits@D@std@@@std@@QEAA_JPEBD_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sputn, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_sputn, 12)
#endif
streamsize __thiscall basic_streambuf_char_sputn(basic_streambuf_char *this, const char *ptr, streamsize count)
{
    TRACE("(%p %p %s)\n", this, ptr, wine_dbgstr_longlong(count));
    return call_basic_streambuf_char_xsputn(this, ptr, count);
}

/* ?swap@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IAEXAAV12@@Z */
/* ?swap@?$basic_streambuf@DU?$char_traits@D@std@@@std@@IEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_char_swap, 8)
void __thiscall basic_streambuf_char_swap(basic_streambuf_char *this, basic_streambuf_char *r)
{
    char *wfirst, *wnext, *wlast, *rfirst, *rnext, *rlast;
#if _MSVCP_VER < 70
    locale loc;
#else
    locale *loc;
#endif

    TRACE("(%p %p)\n", this, r);

    if(this == r)
        return;

    wfirst = *this->pwbuf;
    wnext = *this->pwpos;
    wlast = *this->pwpos + *this->pwsize;
    rfirst = *this->prbuf;
    rnext = *this->prpos;
    rlast = *this->prpos + *this->prsize;
    loc = this->loc;

    basic_streambuf_char_setp_next(this, *r->pwbuf, *r->pwpos, *r->pwpos + *r->pwsize);
    basic_streambuf_char_setg(this, *r->prbuf, *r->prpos, *r->prpos + *r->prsize);
    this->loc = r->loc;

    basic_streambuf_char_setp_next(r, wfirst, wnext, wlast);
    basic_streambuf_char_setg(r, rfirst, rnext, rlast);
    r->loc = loc;
}

/* ?setp@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXPA_W00@Z */
/* ?setp@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXPEA_W00@Z */
/* ?setp@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXPAG00@Z */
/* ?setp@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXPEAG00@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_setp_next, 16)
void __thiscall basic_streambuf_wchar_setp_next(basic_streambuf_wchar *this, wchar_t *first, wchar_t *next, wchar_t *last)
{
    TRACE("(%p %p %p %p)\n", this, first, next, last);

    this->wbuf = first;
    this->wpos = next;
    this->wsize = last-next;
}

/* ?setp@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXPA_W0@Z */
/* ?setp@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXPEA_W0@Z */
/* ?setp@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXPAG0@Z */
/* ?setp@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXPEAG0@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_setp, 12)
void __thiscall basic_streambuf_wchar_setp(basic_streambuf_wchar *this, wchar_t *first, wchar_t *last)
{
    basic_streambuf_wchar_setp_next(this, first, first, last);
}

/* ?setg@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXPA_W00@Z */
/* ?setg@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXPEA_W00@Z */
/* ?setg@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXPAG00@Z */
/* ?setg@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXPEAG00@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_setg, 16)
void __thiscall basic_streambuf_wchar_setg(basic_streambuf_wchar *this, wchar_t *first, wchar_t *next, wchar_t *last)
{
    TRACE("(%p %p %p %p)\n", this, first, next, last);

    this->rbuf = first;
    this->rpos = next;
    this->rsize = last-next;
}

/* ?_Init@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXXZ */
/* ?_Init@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXXZ */
/* ?_Init@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXXZ */
/* ?_Init@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Init_empty, 4)
void __thiscall basic_streambuf_wchar__Init_empty(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);

    this->prbuf = &this->rbuf;
    this->pwbuf = &this->wbuf;
    this->prpos = &this->rpos;
    this->pwpos = &this->wpos;
    this->prsize = &this->rsize;
    this->pwsize = &this->wsize;

    basic_streambuf_wchar_setp(this, NULL, NULL);
    basic_streambuf_wchar_setg(this, NULL, NULL, NULL);
}

/* ??0?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_ctor_uninitialized, 8)
basic_streambuf_wchar* __thiscall basic_streambuf_wchar_ctor_uninitialized(basic_streambuf_wchar *this, int uninitialized)
{
    TRACE("(%p %d)\n", this, uninitialized);
    this->vtable = &basic_streambuf_wchar_vtable;
#if _MSVCP_VER >= 70 && _MSVCP_VER <= 100
    mutex_ctor(&this->lock);
#endif
    return this;
}

/* ??0?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_short_ctor_uninitialized, 8)
basic_streambuf_wchar* __thiscall basic_streambuf_short_ctor_uninitialized(basic_streambuf_wchar *this, int uninitialized)
{
    TRACE("(%p %d)\n", this, uninitialized);
    basic_streambuf_wchar_ctor_uninitialized(this, uninitialized);
    this->vtable = &basic_streambuf_short_vtable;
    return this;
}

/* ??0?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAE@XZ */
/* ??0?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_ctor, 4)
basic_streambuf_wchar* __thiscall basic_streambuf_wchar_ctor(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);

    this->vtable = &basic_streambuf_wchar_vtable;
#if _MSVCP_VER >= 70 && _MSVCP_VER <= 100
    mutex_ctor(&this->lock);
#endif
#if _MSVCP_VER >= 70
    this->loc = operator_new(sizeof(locale));
#endif
    locale_ctor(IOS_LOCALE(this));
    basic_streambuf_wchar__Init_empty(this);

    return this;
}

/* ??0?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAE@XZ */
/* ??0?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_short_ctor, 4)
basic_streambuf_wchar* __thiscall basic_streambuf_short_ctor(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    basic_streambuf_wchar_ctor(this);
    this->vtable = &basic_streambuf_short_vtable;
    return this;
}

/* ??1?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
/* ??1?$basic_streambuf@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_streambuf@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_dtor, 4)
void __thiscall basic_streambuf_wchar_dtor(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);

#if _MSVCP_VER >= 70 && _MSVCP_VER <= 100
    mutex_dtor(&this->lock);
#endif
    locale_dtor(IOS_LOCALE(this));
#if _MSVCP_VER >= 70
    operator_delete(this->loc);
#endif
}

DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_vector_dtor, 8)
basic_streambuf_wchar* __thiscall basic_streambuf_wchar_vector_dtor(basic_streambuf_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_streambuf_wchar_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_streambuf_wchar_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Gnavail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEHXZ */
/* ?_Gnavail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBA_JXZ */
/* ?_Gnavail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEHXZ */
/* ?_Gnavail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Gnavail, 4)
streamsize __thiscall basic_streambuf_wchar__Gnavail(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos ? *this->prsize : 0;
}

/* ?_Gndec@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEPA_WXZ */
/* ?_Gndec@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAPEA_WXZ */
/* ?_Gndec@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEPAGXZ */
/* ?_Gndec@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Gndec, 4)
wchar_t* __thiscall basic_streambuf_wchar__Gndec(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)++;
    (*this->prpos)--;
    return *this->prpos;
}

/* ?_Gninc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEPA_WXZ */
/* ?_Gninc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAPEA_WXZ */
/* ?_Gninc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEPAGXZ */
/* ?_Gninc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Gninc, 4)
wchar_t* __thiscall basic_streambuf_wchar__Gninc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)--;
    return (*this->prpos)++;
}

/* ?_Gnpreinc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEPA_WXZ */
/* ?_Gnpreinc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAPEA_WXZ */
/* ?_Gnpreinc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEPAGXZ */
/* ?_Gnpreinc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Gnpreinc, 4)
wchar_t* __thiscall basic_streambuf_wchar__Gnpreinc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    (*this->prsize)--;
    (*this->prpos)++;
    return *this->prpos;
}

/* ?_Init@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXPAPA_W0PAH001@Z */
/* ?_Init@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXPEAPEA_W0PEAH001@Z */
/* ?_Init@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXPAPAG0PAH001@Z */
/* ?_Init@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXPEAPEAG0PEAH001@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Init, 28)
void __thiscall basic_streambuf_wchar__Init(basic_streambuf_wchar *this, wchar_t **gf, wchar_t **gn, int *gc, wchar_t **pf, wchar_t **pn, int *pc)
{
    TRACE("(%p %p %p %p %p %p %p)\n", this, gf, gn, gc, pf, pn, pc);

    this->prbuf = gf;
    this->pwbuf = pf;
    this->prpos = gn;
    this->pwpos = pn;
    this->prsize = gc;
    this->pwsize = pc;
}

/* ?_Lock@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?_Lock@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ?_Lock@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?_Lock@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Lock, 4)
void __thiscall basic_streambuf_wchar__Lock(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
#if _MSVCP_VER >= 70 && _MSVCP_VER <= 100
    mutex_lock(&this->lock);
#endif
}

/* ?_Pnavail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEHXZ */
/* ?_Pnavail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBA_JXZ */
/* ?_Pnavail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEHXZ */
/* ?_Pnavail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Pnavail, 4)
streamsize __thiscall basic_streambuf_wchar__Pnavail(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos ? *this->pwsize : 0;
}

/* ?_Pninc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEPA_WXZ */
/* ?_Pninc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAPEA_WXZ */
/* ?_Pninc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEPAGXZ */
/* ?_Pninc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Pninc, 4)
wchar_t* __thiscall basic_streambuf_wchar__Pninc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    (*this->pwsize)--;
    return (*this->pwpos)++;
}

/* ?underflow@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEGXZ */
/* ?underflow@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAGXZ */
/* ?underflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEGXZ */
/* ?underflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_underflow, 4)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_wchar_underflow(this) CALL_VTBL_FUNC(this, 24, unsigned short, (basic_streambuf_wchar*), (this))
#else
#define call_basic_streambuf_wchar_underflow(this) CALL_VTBL_FUNC(this, 16, unsigned short, (basic_streambuf_wchar*), (this))
#endif
unsigned short __thiscall basic_streambuf_wchar_underflow(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return WEOF;
}

/* ?uflow@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEGXZ */
/* ?uflow@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAGXZ */
/* ?uflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEGXZ */
/* ?uflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_uflow, 4)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_wchar_uflow(this) CALL_VTBL_FUNC(this, 28, unsigned short, (basic_streambuf_wchar*), (this))
#else
#define call_basic_streambuf_wchar_uflow(this) CALL_VTBL_FUNC(this, 20, unsigned short, (basic_streambuf_wchar*), (this))
#endif
unsigned short __thiscall basic_streambuf_wchar_uflow(basic_streambuf_wchar *this)
{
    int ret;

    TRACE("(%p)\n", this);

    if(call_basic_streambuf_wchar_underflow(this)==WEOF)
        return WEOF;

    ret = **this->prpos;
    (*this->prsize)--;
    (*this->prpos)++;
    return ret;
}

/* ?_Xsgetn_s@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEHPA_WIH@Z */
/* ?_Xsgetn_s@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAA_JPEA_W_K_J@Z */
/* ?_Xsgetn_s@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHPAGIH@Z */
/* ?_Xsgetn_s@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA_JPEAG_K_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Xsgetn_s, 20)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Xsgetn_s, 16)
#endif
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
#define call_basic_streambuf_wchar__Xsgetn_s(this, ptr, size, count) CALL_VTBL_FUNC(this, 28, \
        streamsize, (basic_streambuf_wchar*, wchar_t*, size_t, streamsize), (this, ptr, size, count))
#else
#define call_basic_streambuf_wchar__Xsgetn_s(this, ptr, size, count) basic_streambuf_wchar__Xsgetn_s(this, ptr, size, count)
#endif
streamsize __thiscall basic_streambuf_wchar__Xsgetn_s(basic_streambuf_wchar *this, wchar_t *ptr, size_t size, streamsize count)
{
    streamsize copied, chunk;
    unsigned short c;

    TRACE("(%p %p %Iu %s)\n", this, ptr, size, wine_dbgstr_longlong(count));

    for(copied=0; copied<count && size;) {
        chunk = basic_streambuf_wchar__Gnavail(this);
        if(chunk > count-copied)
            chunk = count-copied;

        if(chunk > 0) {
            memcpy_s(ptr+copied, size, *this->prpos, chunk*sizeof(wchar_t));
            *this->prpos += chunk;
            *this->prsize -= chunk;
            copied += chunk;
            size -= chunk*sizeof(wchar_t);
        }else if((c = call_basic_streambuf_wchar_uflow(this)) != WEOF) {
            ptr[copied] = c;
            copied++;
            size--;
        }else {
            break;
        }
    }

    return copied;
}

/* ?_Sgetn_s@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEHPA_WIH@Z */
/* ?_Sgetn_s@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA_JPEA_W_K_J@Z */
/* ?_Sgetn_s@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEHPAGIH@Z */
/* ?_Sgetn_s@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA_JPEAG_K_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Sgetn_s, 20)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Sgetn_s, 16)
#endif
streamsize __thiscall basic_streambuf_wchar__Sgetn_s(basic_streambuf_wchar *this, wchar_t *ptr, size_t size, streamsize count)
{
    TRACE("(%p %p %Iu %s)\n", this, ptr, size, wine_dbgstr_longlong(count));
    return call_basic_streambuf_wchar__Xsgetn_s(this, ptr, size, count);
}

/* ?_Unlock@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?_Unlock@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ?_Unlock@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?_Unlock@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar__Unlock, 4)
void __thiscall basic_streambuf_wchar__Unlock(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
#if _MSVCP_VER >= 70 && _MSVCP_VER <= 100
    mutex_unlock(&this->lock);
#endif
}

/* ?eback@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEPA_WXZ */
/* ?eback@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBAPEA_WXZ */
/* ?eback@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?eback@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_eback, 4)
wchar_t* __thiscall basic_streambuf_wchar_eback(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->prbuf;
}

/* ?gptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEPA_WXZ */
/* ?gptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBAPEA_WXZ */
/* ?gptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?gptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_gptr, 4)
wchar_t* __thiscall basic_streambuf_wchar_gptr(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos;
}

/* ?egptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEPA_WXZ */
/* ?egptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBAPEA_WXZ */
/* ?egptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?egptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_egptr, 4)
wchar_t* __thiscall basic_streambuf_wchar_egptr(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->prpos+*this->prsize;
}

/* ?epptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEPA_WXZ */
/* ?epptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBAPEA_WXZ */
/* ?epptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?epptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_epptr, 4)
wchar_t* __thiscall basic_streambuf_wchar_epptr(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos+*this->pwsize;
}

/* ?gbump@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXH@Z */
/* ?gbump@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXH@Z */
/* ?gbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXH@Z */
/* ?gbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_gbump, 8)
void __thiscall basic_streambuf_wchar_gbump(basic_streambuf_wchar *this, int off)
{
    TRACE("(%p %d)\n", this, off);
    *this->prpos += off;
    *this->prsize -= off;
}

/* ?getloc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QBE?AVlocale@2@XZ */
/* ?getloc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEBA?AVlocale@2@XZ */
/* ?getloc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QBE?AVlocale@2@XZ */
/* ?getloc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEBA?AVlocale@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_getloc, 8)
locale* __thiscall basic_streambuf_wchar_getloc(const basic_streambuf_wchar *this, locale *ret)
{
    TRACE("(%p)\n", this);
    return locale_copy_ctor(ret, IOS_LOCALE(this));
}

/* ?imbue@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEXABVlocale@2@@Z */
/* ?imbue@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAXAEBVlocale@2@@Z */
/* ?imbue@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEXABVlocale@2@@Z */
/* ?imbue@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAXAEBVlocale@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_imbue, 8)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_wchar_imbue(this, loc) CALL_VTBL_FUNC(this, 56, void, (basic_streambuf_wchar*, const locale*), (this, loc))
#elif _MSVCP_VER >= 80
#define call_basic_streambuf_wchar_imbue(this, loc) CALL_VTBL_FUNC(this, 52, void, (basic_streambuf_wchar*, const locale*), (this, loc))
#else
#define call_basic_streambuf_wchar_imbue(this, loc) CALL_VTBL_FUNC(this, 48, void, (basic_streambuf_wchar*, const locale*), (this, loc))
#endif
void __thiscall basic_streambuf_wchar_imbue(basic_streambuf_wchar *this, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
}

/* ?overflow@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEGG@Z */
/* ?overflow@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAGG@Z */
/* ?overflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEGG@Z */
/* ?overflow@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_overflow, 8)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_wchar_overflow(this, ch) CALL_VTBL_FUNC(this, 12, unsigned short, (basic_streambuf_wchar*, unsigned short), (this, ch))
#else
#define call_basic_streambuf_wchar_overflow(this, ch) CALL_VTBL_FUNC(this, 4, unsigned short, (basic_streambuf_wchar*, unsigned short), (this, ch))
#endif
unsigned short __thiscall basic_streambuf_wchar_overflow(basic_streambuf_wchar *this, unsigned short ch)
{
    TRACE("(%p %d)\n", this, ch);
    return WEOF;
}

/* ?pbackfail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEGG@Z */
/* ?pbackfail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAGG@Z */
/* ?pbackfail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEGG@Z */
/* ?pbackfail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pbackfail, 8)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_wchar_pbackfail(this, ch) CALL_VTBL_FUNC(this, 16, unsigned short, (basic_streambuf_wchar*, unsigned short), (this, ch))
#else
#define call_basic_streambuf_wchar_pbackfail(this, ch) CALL_VTBL_FUNC(this, 8, unsigned short, (basic_streambuf_wchar*, unsigned short), (this, ch))
#endif
unsigned short __thiscall basic_streambuf_wchar_pbackfail(basic_streambuf_wchar *this, unsigned short ch)
{
    TRACE("(%p %d)\n", this, ch);
    return WEOF;
}

/* ?pbase@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEPA_WXZ */
/* ?pbase@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBAPEA_WXZ */
/* ?pbase@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?pbase@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pbase, 4)
wchar_t* __thiscall basic_streambuf_wchar_pbase(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->pwbuf;
}

/* ?pbump@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXH@Z */
/* ?pbump@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEAAXH@Z */
/* ?pbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXH@Z */
/* ?pbump@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXH@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pbump, 8)
void __thiscall basic_streambuf_wchar_pbump(basic_streambuf_wchar *this, int off)
{
    TRACE("(%p %d)\n", this, off);
    *this->pwpos += off;
    *this->pwsize -= off;
}

/* ?pptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IBEPA_WXZ */
/* ?pptr@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IEBAPEA_WXZ */
/* ?pptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IBEPAGXZ */
/* ?pptr@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEBAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pptr, 4)
wchar_t* __thiscall basic_streambuf_wchar_pptr(const basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return *this->pwpos;
}

/* ?pubimbue@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?pubimbue@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
/* ?pubimbue@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?pubimbue@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubimbue, 12)
locale* __thiscall basic_streambuf_wchar_pubimbue(basic_streambuf_wchar *this, locale *ret, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
    memcpy(ret, IOS_LOCALE(this), sizeof(locale));
    call_basic_streambuf_wchar_imbue(this, loc);
    locale_copy_ctor(IOS_LOCALE(this), loc);
    return ret;
}

/* ?seekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
/* ?seekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_seekoff, 24)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_seekoff, 20)
#endif
#if _MSVCP_VER >= 100
#define call_basic_streambuf_wchar_seekoff(this, ret, off, way, mode) \
        CALL_VTBL_FUNC(this, 40, fpos_mbstatet*, (basic_streambuf_wchar*, fpos_mbstatet*, streamoff, int, int), (this, ret, off, way, mode))
#elif _MSVCP_VER >= 80
#define call_basic_streambuf_wchar_seekoff(this, ret, off, way, mode) \
        CALL_VTBL_FUNC(this, 36, fpos_mbstatet*, (basic_streambuf_wchar*, fpos_mbstatet*, streamoff, int, int), (this, ret, off, way, mode))
#else
#define call_basic_streambuf_wchar_seekoff(this, ret, off, way, mode) \
        CALL_VTBL_FUNC(this, 32, fpos_mbstatet*, (basic_streambuf_wchar*, fpos_mbstatet*, streamoff, int, int), (this, ret, off, way, mode))
#endif
fpos_mbstatet* __thiscall basic_streambuf_wchar_seekoff(basic_streambuf_wchar *this,
        fpos_mbstatet *ret, streamoff off, int way, int mode)
{
    TRACE("(%p %s %d %d)\n", this, wine_dbgstr_longlong(off), way, mode);
    ret->off = -1;
    ret->pos = 0;
    memset(&ret->state, 0, sizeof(ret->state));
    return ret;
}

/* ?pubseekoff@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAE?AV?$fpos@H@2@JHH@Z */
/* ?pubseekoff@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA?AV?$fpos@H@2@_JHH@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@JHH@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@_JHH@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@JFF@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@_JFF@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@_JW4seekdir@ioos_base@2@H@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubseekoff, 24)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubseekoff, 20)
#endif
fpos_mbstatet* __thiscall basic_streambuf_wchar_pubseekoff(basic_streambuf_wchar *this,
        fpos_mbstatet *ret, streamoff off, int way, int mode)
{
    TRACE("(%p %s %d %d)\n", this, wine_dbgstr_longlong(off), way, mode);
    return call_basic_streambuf_wchar_seekoff(this, ret, off, way, mode);
}

/* ?pubseekoff@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAE?AV?$fpos@H@2@JII@Z */
/* ?pubseekoff@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA?AV?$fpos@H@2@_JII@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@JII@Z */
/* ?pubseekoff@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@_JII@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubseekoff_old, 24)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubseekoff_old, 20)
#endif
fpos_mbstatet* __thiscall basic_streambuf_wchar_pubseekoff_old(basic_streambuf_wchar *this,
        fpos_mbstatet *ret, streamoff off, unsigned int way, unsigned int mode)
{
    TRACE("(%p %s %d %d)\n", this, wine_dbgstr_longlong(off), way, mode);
    return basic_streambuf_wchar_pubseekoff(this, ret, off, way, mode);
}

/* ?seekpos@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_seekpos, 36)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_wchar_seekpos(this, ret, pos, mode) \
        CALL_VTBL_FUNC(this, 44, fpos_mbstatet*, (basic_streambuf_wchar*, fpos_mbstatet*, fpos_mbstatet, int), (this, ret, pos, mode))
#elif _MSVCP_VER >= 80
#define call_basic_streambuf_wchar_seekpos(this, ret, pos, mode) \
        CALL_VTBL_FUNC(this, 40, fpos_mbstatet*, (basic_streambuf_wchar*, fpos_mbstatet*, fpos_mbstatet, int), (this, ret, pos, mode))
#else
#define call_basic_streambuf_wchar_seekpos(this, ret, pos, mode) \
        CALL_VTBL_FUNC(this, 36, fpos_mbstatet*, (basic_streambuf_wchar*, fpos_mbstatet*, fpos_mbstatet, int), (this, ret, pos, mode))
#endif
fpos_mbstatet* __thiscall basic_streambuf_wchar_seekpos(basic_streambuf_wchar *this,
        fpos_mbstatet *ret, fpos_mbstatet pos, int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_mbstatet(&pos), mode);
    ret->off = -1;
    ret->pos = 0;
    memset(&ret->state, 0, sizeof(ret->state));
    return ret;
}

/* ?pubseekpos@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAE?AV?$fpos@H@2@V32@H@Z */
/* ?pubseekpos@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA?AV?$fpos@H@2@V32@H@Z */
/* ?pubseekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@V32@H@Z */
/* ?pubseekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubseekpos, 36)
fpos_mbstatet* __thiscall basic_streambuf_wchar_pubseekpos(basic_streambuf_wchar *this,
        fpos_mbstatet *ret, fpos_mbstatet pos, int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_mbstatet(&pos), mode);
    return call_basic_streambuf_wchar_seekpos(this, ret, pos, mode);
}

/* ?pubseekpos@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAE?AV?$fpos@H@2@V32@I@Z */
/* ?pubseekpos@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA?AV?$fpos@H@2@V32@I@Z */
/* ?pubseekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@V32@I@Z */
/* ?pubseekpos@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@V32@I@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubseekpos_old, 36)
fpos_mbstatet* __thiscall basic_streambuf_wchar_pubseekpos_old(basic_streambuf_wchar *this,
        fpos_mbstatet *ret, fpos_mbstatet pos, unsigned int mode)
{
    TRACE("(%p %s %d)\n", this, debugstr_fpos_mbstatet(&pos), mode);
    return basic_streambuf_wchar_pubseekpos(this, ret, pos, mode);
}

/* ?setbuf@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEPAV12@PA_WH@Z */
/* ?setbuf@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAPEAV12@PEA_W_J@Z */
/* ?setbuf@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEPAV12@PAGH@Z */
/* ?setbuf@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAPEAV12@PEAG_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_setbuf, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_setbuf, 12)
#endif
#if _MSVCP_VER >= 100
#define call_basic_streambuf_wchar_setbuf(this, buf, count) CALL_VTBL_FUNC(this, 48, basic_streambuf_wchar*, (basic_streambuf_wchar*, wchar_t*, streamsize), (this, buf, count))
#elif _MSVCP_VER >= 80
#define call_basic_streambuf_wchar_setbuf(this, buf, count) CALL_VTBL_FUNC(this, 44, basic_streambuf_wchar*, (basic_streambuf_wchar*, wchar_t*, streamsize), (this, buf, count))
#else
#define call_basic_streambuf_wchar_setbuf(this, buf, count) CALL_VTBL_FUNC(this, 40, basic_streambuf_wchar*, (basic_streambuf_wchar*, wchar_t*, streamsize), (this, buf, count))
#endif
basic_streambuf_wchar* __thiscall basic_streambuf_wchar_setbuf(basic_streambuf_wchar *this, wchar_t *buf, streamsize count)
{
    TRACE("(%p %p %s)\n", this, buf, wine_dbgstr_longlong(count));
    return this;
}

/* ?pubsetbuf@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEPAV12@PA_WH@Z */
/* ?pubsetbuf@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAPEAV12@PEA_W_J@Z */
/* ?pubsetbuf@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PAGH@Z */
/* ?pubsetbuf@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEAG_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubsetbuf, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubsetbuf, 12)
#endif
basic_streambuf_wchar* __thiscall basic_streambuf_wchar_pubsetbuf(basic_streambuf_wchar *this, wchar_t *buf, streamsize count)
{
    TRACE("(%p %p %s)\n", this, buf, wine_dbgstr_longlong(count));
    return call_basic_streambuf_wchar_setbuf(this, buf, count);
}

/* ?sync@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEHXZ */
/* ?sync@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAAHXZ */
/* ?sync@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHXZ */
/* ?sync@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sync, 4)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_wchar_sync(this) CALL_VTBL_FUNC(this, 52, int, (basic_streambuf_wchar*), (this))
#elif _MSVCP_VER >= 80
#define call_basic_streambuf_wchar_sync(this) CALL_VTBL_FUNC(this, 48, int, (basic_streambuf_wchar*), (this))
#else
#define call_basic_streambuf_wchar_sync(this) CALL_VTBL_FUNC(this, 44, int, (basic_streambuf_wchar*), (this))
#endif
int __thiscall basic_streambuf_wchar_sync(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return 0;
}

/* ?pubsync@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEHXZ */
/* ?pubsync@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAHXZ */
/* ?pubsync@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEHXZ */
/* ?pubsync@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_pubsync, 4)
int __thiscall basic_streambuf_wchar_pubsync(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return call_basic_streambuf_wchar_sync(this);
}

/* ?xsgetn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEHPA_WH@Z */
/* ?xsgetn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAA_JPEA_W_J@Z */
/* ?xsgetn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHPAGH@Z */
/* ?xsgetn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA_JPEAG_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_xsgetn, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_xsgetn, 12)
#endif
#if _MSVCP_VER >= 100
#define call_basic_streambuf_wchar_xsgetn(this, ptr, count) CALL_VTBL_FUNC(this, 32, streamsize, (basic_streambuf_wchar*, wchar_t*, streamsize), (this, ptr, count))
#else
#define call_basic_streambuf_wchar_xsgetn(this, ptr, count) CALL_VTBL_FUNC(this, 24, streamsize, (basic_streambuf_wchar*, wchar_t*, streamsize), (this, ptr, count))
#endif
streamsize __thiscall basic_streambuf_wchar_xsgetn(basic_streambuf_wchar *this, wchar_t *ptr, streamsize count)
{
    TRACE("(%p %p %s)\n", this, ptr, wine_dbgstr_longlong(count));
    return call_basic_streambuf_wchar__Xsgetn_s(this, ptr, -1, count);
}

/* ?sgetn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEHPA_WH@Z */
/* ?sgetn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA_JPEA_W_J@Z */
/* ?sgetn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEHPAGH@Z */
/* ?sgetn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA_JPEAG_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sgetn, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sgetn, 12)
#endif
streamsize __thiscall basic_streambuf_wchar_sgetn(basic_streambuf_wchar *this, wchar_t *ptr, streamsize count)
{
    TRACE("(%p %p %s)\n", this, ptr, wine_dbgstr_longlong(count));
    return call_basic_streambuf_wchar_xsgetn(this, ptr, count);
}

/* ?showmanyc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEHXZ */
/* ?showmanyc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAA_JXZ */
/* ?showmanyc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHXZ */
/* ?showmanyc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_showmanyc, 4)
#if _MSVCP_VER >= 100
#define call_basic_streambuf_wchar_showmanyc(this) CALL_VTBL_FUNC(this, 20, streamsize, (basic_streambuf_wchar*), (this))
#else
#define call_basic_streambuf_wchar_showmanyc(this) CALL_VTBL_FUNC(this, 12, streamsize, (basic_streambuf_wchar*), (this))
#endif
streamsize __thiscall basic_streambuf_wchar_showmanyc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return 0;
}

/* ?in_avail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEHXZ */
/* ?in_avail@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA_JXZ */
/* ?in_avail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEHXZ */
/* ?in_avail@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA_JXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_in_avail, 4)
streamsize __thiscall basic_streambuf_wchar_in_avail(basic_streambuf_wchar *this)
{
    streamsize ret;

    TRACE("(%p)\n", this);

    ret = basic_streambuf_wchar__Gnavail(this);
    return ret ? ret : call_basic_streambuf_wchar_showmanyc(this);
}

/* ?sputbackc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEG_W@Z */
/* ?sputbackc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAG_W@Z */
/* ?sputbackc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGG@Z */
/* ?sputbackc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sputbackc, 8)
unsigned short __thiscall basic_streambuf_wchar_sputbackc(basic_streambuf_wchar *this, wchar_t ch)
{
    TRACE("(%p %d)\n", this, ch);
    if(*this->prpos && *this->prpos>*this->prbuf && (*this->prpos)[-1]==ch) {
        (*this->prsize)++;
        (*this->prpos)--;
        return ch;
    }

    return call_basic_streambuf_wchar_pbackfail(this, ch);
}

/* ?sputc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEG_W@Z */
/* ?sputc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAG_W@Z */
/* ?sputc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGG@Z */
/* ?sputc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAHG@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sputc, 8)
unsigned short __thiscall basic_streambuf_wchar_sputc(basic_streambuf_wchar *this, wchar_t ch)
{
    TRACE("(%p %d)\n", this, ch);
    return basic_streambuf_wchar__Pnavail(this) ?
        (*basic_streambuf_wchar__Pninc(this) = ch) :
        call_basic_streambuf_wchar_overflow(this, ch);
}

/* ?sungetc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEGXZ */
/* ?sungetc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAGXZ */
/* ?sungetc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?sungetc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sungetc, 4)
unsigned short __thiscall basic_streambuf_wchar_sungetc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    if(*this->prpos && *this->prpos>*this->prbuf) {
        (*this->prsize)++;
        (*this->prpos)--;
        return **this->prpos;
    }

    return call_basic_streambuf_wchar_pbackfail(this, WEOF);
}

/* ?stossc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?stossc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ?stossc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?stossc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_stossc, 4)
void __thiscall basic_streambuf_wchar_stossc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    if(basic_streambuf_wchar__Gnavail(this))
        basic_streambuf_wchar__Gninc(this);
    else
        call_basic_streambuf_wchar_uflow(this);
}

/* ?sbumpc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEGXZ */
/* ?sbumpc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAGXZ */
/* ?sbumpc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?sbumpc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sbumpc, 4)
unsigned short __thiscall basic_streambuf_wchar_sbumpc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return basic_streambuf_wchar__Gnavail(this) ?
        *basic_streambuf_wchar__Gninc(this) : call_basic_streambuf_wchar_uflow(this);
}

/* ?sgetc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEGXZ */
/* ?sgetc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAGXZ */
/* ?sgetc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?sgetc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sgetc, 4)
unsigned short __thiscall basic_streambuf_wchar_sgetc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return basic_streambuf_wchar__Gnavail(this) ?
        *basic_streambuf_wchar_gptr(this) : call_basic_streambuf_wchar_underflow(this);
}

/* ?snextc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEGXZ */
/* ?snextc@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAAGXZ */
/* ?snextc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?snextc@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_snextc, 4)
unsigned short __thiscall basic_streambuf_wchar_snextc(basic_streambuf_wchar *this)
{
    TRACE("(%p)\n", this);

    if(basic_streambuf_wchar__Gnavail(this) > 1)
        return *basic_streambuf_wchar__Gnpreinc(this);
    return basic_streambuf_wchar_sbumpc(this)==WEOF ?
        WEOF : basic_streambuf_wchar_sgetc(this);
}

/* ?xsputn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MAEHPB_WH@Z */
/* ?xsputn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@MEAA_JPEB_W_J@Z */
/* ?xsputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MAEHPBGH@Z */
/* ?xsputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@MEAA_JPEBG_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_xsputn, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_xsputn, 12)
#endif
#if _MSVCP_VER >= 100
#define call_basic_streambuf_wchar_xsputn(this, ptr, count) CALL_VTBL_FUNC(this, 36, streamsize, (basic_streambuf_wchar*, const wchar_t*, streamsize), (this, ptr, count))
#elif _MSVCP_VER >= 80
#define call_basic_streambuf_wchar_xsputn(this, ptr, count) CALL_VTBL_FUNC(this, 32, streamsize, (basic_streambuf_wchar*, const wchar_t*, streamsize), (this, ptr, count))
#else
#define call_basic_streambuf_wchar_xsputn(this, ptr, count) CALL_VTBL_FUNC(this, 28, streamsize, (basic_streambuf_wchar*, const wchar_t*, streamsize), (this, ptr, count))
#endif
streamsize __thiscall basic_streambuf_wchar_xsputn(basic_streambuf_wchar *this, const wchar_t *ptr, streamsize count)
{
    streamsize copied, chunk;

    TRACE("(%p %p %s)\n", this, ptr, wine_dbgstr_longlong(count));

    for(copied=0; copied<count;) {
        chunk = basic_streambuf_wchar__Pnavail(this);
        if(chunk > count-copied)
            chunk = count-copied;

        if(chunk > 0) {
            memcpy(*this->pwpos, ptr+copied, chunk*sizeof(wchar_t));
            *this->pwpos += chunk;
            *this->pwsize -= chunk;
            copied += chunk;
        }else if(call_basic_streambuf_wchar_overflow(this, ptr[copied]) != WEOF) {
            copied++;
        }else {
            break;
        }
    }

    return copied;
}

/* ?sputn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QAEHPB_WH@Z */
/* ?sputn@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@QEAA_JPEB_W_J@Z */
/* ?sputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QAEHPBGH@Z */
/* ?sputn@?$basic_streambuf@GU?$char_traits@G@std@@@std@@QEAA_JPEBG_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sputn, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_sputn, 12)
#endif
streamsize __thiscall basic_streambuf_wchar_sputn(basic_streambuf_wchar *this, const wchar_t *ptr, streamsize count)
{
    TRACE("(%p %p %s)\n", this, ptr, wine_dbgstr_longlong(count));
    return call_basic_streambuf_wchar_xsputn(this, ptr, count);
}

/* ?swap@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IAEXAAV12@@Z */
/* ?swap@?$basic_streambuf@GU?$char_traits@G@std@@@std@@IEAAXAEAV12@@Z */
/* ?swap@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXAAV12@@Z */
/* ?swap@?$basic_streambuf@_WU?$char_traits@_W@std@@@std@@IAEXAAV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_streambuf_wchar_swap, 8)
void __thiscall basic_streambuf_wchar_swap(basic_streambuf_wchar *this, basic_streambuf_wchar *r)
{
    wchar_t *wfirst, *wnext, *wlast, *rfirst, *rnext, *rlast;
#if _MSVCP_VER < 70
    locale loc;
#else
    locale *loc;
#endif

    TRACE("(%p %p)\n", this, r);

    if(this == r)
        return;

    wfirst = *this->pwbuf;
    wnext = *this->pwpos;
    wlast = *this->pwpos + *this->pwsize;
    rfirst = *this->prbuf;
    rnext = *this->prpos;
    rlast = *this->prpos + *this->prsize;
    loc = this->loc;

    basic_streambuf_wchar_setp_next(this, *r->pwbuf, *r->pwpos, *r->pwpos + *r->pwsize);
    basic_streambuf_wchar_setg(this, *r->prbuf, *r->prpos, *r->prpos + *r->prsize);
    this->loc = r->loc;

    basic_streambuf_wchar_setp_next(r, wfirst, wnext, wlast);
    basic_streambuf_wchar_setg(r, rfirst, rnext, rlast);
    r->loc = loc;
}

/* ?_Stinit@?1??_Init@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IAEXPAU_iobuf@@W4_Initfl@23@@Z@4HA */
/* ?_Stinit@?1??_Init@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IEAAXPEAU_iobuf@@W4_Initfl@23@@Z@4HA */
#if _MSVCP_VER >= 140
_Mbstatet basic_filebuf_char__Init__Stinit = {0};
#else
_Mbstatet basic_filebuf_char__Init__Stinit = 0;
#endif

/* ?_Init@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IAEXPAU_iobuf@@W4_Initfl@12@@Z */
/* ?_Init@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IEAAXPEAU_iobuf@@W4_Initfl@12@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char__Init, 12)
void __thiscall basic_filebuf_char__Init(basic_filebuf_char *this, FILE *file, basic_filebuf__Initfl which)
{
    TRACE("(%p %p %d)\n", this, file, which);

    this->cvt = NULL;
    this->wrotesome = FALSE;
    this->state = basic_filebuf_char__Init__Stinit;
    this->close = (which == INITFL_open);
    this->file = file;

    basic_streambuf_char__Init_empty(&this->base);
    if(file)
    {
        char **base, **ptr;
        int *cnt;

#if _MSVCP_VER >= 140
        _get_stream_buffer_pointers(file, &base, &ptr, &cnt);
#else
        base = &file->_base;
        ptr = &file->_ptr;
        cnt = &file->_cnt;
#endif
        basic_streambuf_char__Init(&this->base, base, ptr, cnt, base, ptr, cnt);
    }
}

/* ?_Initcvt@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IAEXPAV?$codecvt@DDH@2@@Z */
/* ?_Initcvt@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IEAAXPEAV?$codecvt@DDH@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char__Initcvt_cvt, 8)
void __thiscall basic_filebuf_char__Initcvt_cvt(basic_filebuf_char *this, codecvt_char *cvt)
{
    TRACE("(%p %p)\n", this, cvt);

    if(codecvt_base_always_noconv(&cvt->base)) {
        this->cvt = NULL;
    }else {
        basic_streambuf_char__Init_empty(&this->base);
        this->cvt = cvt;
    }
}

/* ?_Initcvt@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IAEXXZ */
/* ?_Initcvt@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char__Initcvt, 4)
void __thiscall basic_filebuf_char__Initcvt(basic_filebuf_char *this)
{
    codecvt_char *cvt = codecvt_char_use_facet(IOS_LOCALE(&this->base));
    basic_filebuf_char__Initcvt_cvt( this, cvt );
}

/* ?_Endwrite@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IAE_NXZ */
/* ?_Endwrite@?$basic_filebuf@DU?$char_traits@D@std@@@std@@IEAA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char__Endwrite, 4)
bool __thiscall basic_filebuf_char__Endwrite(basic_filebuf_char *this)
{
    TRACE("(%p)\n", this);

    if(!this->wrotesome || !this->cvt)
        return TRUE;


    if(call_basic_streambuf_char_overflow(&this->base, EOF) == EOF)
        return FALSE;

    while(1) {
        /* TODO: check if we need a dynamic buffer here */
        char buf[128];
        char *next;
        int ret;

        ret = codecvt_char_unshift(this->cvt, &this->state, buf, buf+sizeof(buf), &next);
        switch(ret) {
        case CODECVT_ok:
            this->wrotesome = FALSE;
            /* fall through */
        case CODECVT_partial:
            if(!fwrite(buf, next-buf, 1, this->file))
                return FALSE;
            if(this->wrotesome)
                break;
            /* fall through */
        case CODECVT_noconv:
            if(call_basic_streambuf_char_overflow(&this->base, EOF) == EOF)
                return FALSE;
            return TRUE;
        default:
            return FALSE;
        }
    }
}

/* ?close@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@XZ */
/* ?close@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_close, 4)
basic_filebuf_char* __thiscall basic_filebuf_char_close(basic_filebuf_char *this)
{
    basic_filebuf_char *ret = this;

    TRACE("(%p)\n", this);

    if(!this->file)
        return NULL;

    /* TODO: handle exceptions */
    if(!basic_filebuf_char__Endwrite(this))
        ret = NULL;
    if(fclose(this->file))
        ret  = NULL;

    basic_filebuf_char__Init(this, NULL, INITFL_close);
    return ret;
}

/* ??0?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_ctor_file, 8)
basic_filebuf_char* __thiscall basic_filebuf_char_ctor_file(basic_filebuf_char *this, FILE *file)
{
    TRACE("(%p %p)\n", this, file);

    basic_streambuf_char_ctor(&this->base);
    this->base.vtable = &basic_filebuf_char_vtable;

    basic_filebuf_char__Init(this, file, INITFL_new);
    return this;
}

/* ??_F?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_F?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_ctor, 4)
basic_filebuf_char* __thiscall basic_filebuf_char_ctor(basic_filebuf_char *this)
{
    return basic_filebuf_char_ctor_file(this, NULL);
}

/* ??0?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_ctor_uninitialized, 8)
basic_filebuf_char* __thiscall basic_filebuf_char_ctor_uninitialized(basic_filebuf_char *this, int uninitialized)
{
    TRACE("(%p %d)\n", this, uninitialized);

    basic_streambuf_char_ctor(&this->base);
    this->base.vtable = &basic_filebuf_char_vtable;
    return this;
}

/* ??1?$basic_filebuf@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_filebuf@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_dtor, 4)
void __thiscall basic_filebuf_char_dtor(basic_filebuf_char *this)
{
    TRACE("(%p)\n", this);

    if(this->close)
        basic_filebuf_char_close(this);
    basic_streambuf_char_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(basic_filebuf_char_vector_dtor, 8)
basic_filebuf_char* __thiscall basic_filebuf_char_vector_dtor(basic_filebuf_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_filebuf_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_filebuf_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?is_open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_is_open, 4)
bool __thiscall basic_filebuf_char_is_open(const basic_filebuf_char *this)
{
    TRACE("(%p)\n", this);
    return this->file != NULL;
}

/* ?_Fiopen@std@@YAPAU_iobuf@@PB_WHH@Z */
/* ?_Fiopen@std@@YAPEAU_iobuf@@PEB_WHH@Z */
FILE* __cdecl _Fiopen_wchar(const wchar_t *name, int mode, int prot)
{
    static const struct {
        int mode;
        const wchar_t str[4];
        const wchar_t str_bin[4];
    } str_mode[] = {
        {OPENMODE_out,                              L"w",   L"wb"},
        {OPENMODE_out|OPENMODE_app,                 L"a",   L"ab"},
        {OPENMODE_app,                              L"a",   L"ab"},
        {OPENMODE_out|OPENMODE_trunc,               L"w",   L"wb"},
        {OPENMODE_in,                               L"r",   L"rb"},
        {OPENMODE_in|OPENMODE_out,                  L"r+",  L"r+b"},
        {OPENMODE_in|OPENMODE_out|OPENMODE_trunc,   L"w+",  L"w+b"},
        {OPENMODE_in|OPENMODE_out|OPENMODE_app,     L"a+",  L"a+b"},
        {OPENMODE_in|OPENMODE_app,                  L"a+",  L"a+b"}
    };

    int real_mode = mode & ~(OPENMODE_ate|OPENMODE__Nocreate|OPENMODE__Noreplace|OPENMODE_binary);
    size_t mode_idx;
    FILE *f = NULL;

    TRACE("(%s %d %d)\n", debugstr_w(name), mode, prot);

    for(mode_idx=0; mode_idx<ARRAY_SIZE(str_mode); mode_idx++)
        if(str_mode[mode_idx].mode == real_mode)
            break;
    if(mode_idx == ARRAY_SIZE(str_mode))
        return NULL;

    if((mode & OPENMODE__Nocreate) && !(f = _wfopen(name, L"r")))
        return NULL;
    else if(f)
        fclose(f);

    if((mode & OPENMODE__Noreplace) && (mode & (OPENMODE_out|OPENMODE_app))
            && (f = _wfopen(name, L"r"))) {
        fclose(f);
        return NULL;
    }

#if _MSVCP_VER < 80    /* msvcp60 - msvcp71 are ignoring prot argument */
    prot = SH_DENYNO;
#endif

    f = _wfsopen(name, (mode & OPENMODE_binary) ? str_mode[mode_idx].str_bin
            : str_mode[mode_idx].str, prot);
    if(!f)
        return NULL;

    if((mode & OPENMODE_ate) && fseek(f, 0, SEEK_END)) {
        fclose(f);
        return NULL;
    }

    return f;
}

/* ?_Fiopen@std@@YAPAU_iobuf@@PBDHH@Z */
/* ?_Fiopen@std@@YAPEAU_iobuf@@PEBDHH@Z */
FILE* __cdecl _Fiopen(const char *name, int mode, int prot)
{
    wchar_t nameW[FILENAME_MAX];

    TRACE("(%s %d %d)\n", name, mode, prot);

#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
    if(mbstowcs_s(NULL, nameW, FILENAME_MAX, name, FILENAME_MAX-1) != 0)
        return NULL;
#else
    if(!MultiByteToWideChar(CP_ACP, 0, name, -1, nameW, FILENAME_MAX-1))
        return NULL;
#endif
    return _Fiopen_wchar(nameW, mode, prot);
}

/* ?__Fiopen@std@@YAPAU_iobuf@@PBDH@Z */
/* ?__Fiopen@std@@YAPEAU_iobuf@@PEBDH@Z */
FILE* __cdecl ___Fiopen(const char *name, int mode)
{
    TRACE("(%p %d)\n", name, mode);
    return _Fiopen(name, mode, _SH_DENYNO);
}

/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PB_WHH@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEB_WHH@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PBGHH@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEBGHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_open_wchar, 16)
basic_filebuf_char* __thiscall basic_filebuf_char_open_wchar(basic_filebuf_char *this, const wchar_t *name, int mode, int prot)
{
    FILE *f = NULL;

    TRACE("(%p %s %d %d)\n", this, debugstr_w(name), mode, prot);

    if(basic_filebuf_char_is_open(this))
        return NULL;

    if(!(f = _Fiopen_wchar(name, mode, prot)))
        return NULL;

    basic_filebuf_char__Init(this, f, INITFL_open);
    basic_filebuf_char__Initcvt_cvt(this, codecvt_char_use_facet(IOS_LOCALE(&this->base)));
    return this;
}

/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PB_WI@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEB_WI@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PBGI@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEBGI@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_open_wchar_mode, 12)
basic_filebuf_char* __thiscall basic_filebuf_char_open_wchar_mode(basic_filebuf_char *this, const wchar_t *name, unsigned int mode)
{
    return basic_filebuf_char_open_wchar(this, name, mode, SH_DENYNO);
}

/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PBDHH@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_open, 16)
basic_filebuf_char* __thiscall basic_filebuf_char_open(basic_filebuf_char *this, const char *name, int mode, int prot)
{
    wchar_t nameW[FILENAME_MAX];

    TRACE("(%p %s %d %d)\n", this, name, mode, prot);

    if(mbstowcs_s(NULL, nameW, FILENAME_MAX, name, FILENAME_MAX-1) != 0)
        return NULL;
    return basic_filebuf_char_open_wchar(this, nameW, mode, prot);
}

/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PBDF@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEBDF@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_open_mode_old, 12)
basic_filebuf_char* __thiscall basic_filebuf_char_open_mode_old(basic_filebuf_char *this, const char *name, short mode)
{
    TRACE("(%p %p %d)\n", this, name, mode);
    return basic_filebuf_char_open(this, name, mode, SH_DENYNO);
}

/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PBDH@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEBDH@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAEPAV12@PBDI@Z */
/* ?open@?$basic_filebuf@DU?$char_traits@D@std@@@std@@QEAAPEAV12@PEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_open_mode, 12)
basic_filebuf_char* __thiscall basic_filebuf_char_open_mode(basic_filebuf_char *this, const char *name, unsigned int mode)
{
    return basic_filebuf_char_open(this, name, mode, SH_DENYNO);
}

/* ?overflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHH@Z */
/* ?overflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_overflow, 8)
int __thiscall basic_filebuf_char_overflow(basic_filebuf_char *this, int c)
{
    char buf[8], *dyn_buf;
    char ch = c, *to_next;
    const char *from_next;
    int ret, max_size;


    TRACE("(%p %d)\n", this, c);

    if(!basic_filebuf_char_is_open(this))
        return EOF;
    if(c == EOF)
        return !c;

    if(!this->cvt)
        return fputc(ch, this->file);

    from_next = &ch;
    do {
        ret = codecvt_char_out(this->cvt, &this->state, from_next, &ch+1,
                &from_next, buf, buf+sizeof(buf), &to_next);

        switch(ret) {
        case CODECVT_partial:
            if(to_next == buf)
                break;
            /* fall through */
        case CODECVT_ok:
            if(!fwrite(buf, to_next-buf, 1, this->file))
                return EOF;
            if(ret == CODECVT_partial)
                continue;
            return c;
        case CODECVT_noconv:
            return fwrite(&ch, sizeof(char), 1, this->file) ? c : EOF;
        default:
            return EOF;
        }

        break;
    } while(1);

    max_size = codecvt_base_max_length(&this->cvt->base);
    dyn_buf = malloc(max_size);
    if(!dyn_buf)
        return EOF;

    ret = codecvt_char_out(this->cvt, &this->state, from_next, &ch+1,
            &from_next, dyn_buf, dyn_buf+max_size, &to_next);

    switch(ret) {
    case CODECVT_ok:
        ret = fwrite(dyn_buf, to_next-dyn_buf, 1, this->file);
        free(dyn_buf);
        return ret ? c : EOF;
    case CODECVT_partial:
        ERR("buffer should be big enough to store all output\n");
        /* fall through */
    default:
        free(dyn_buf);
        return EOF;
    }
}

/* ?pbackfail@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHH@Z */
/* ?pbackfail@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_pbackfail, 8)
int __thiscall basic_filebuf_char_pbackfail(basic_filebuf_char *this, int c)
{
    TRACE("(%p %d)\n", this, c);

    if(!basic_filebuf_char_is_open(this))
        return EOF;

    if(basic_streambuf_char_gptr(&this->base)>basic_streambuf_char_eback(&this->base)
            && (c==EOF || (int)(unsigned char)basic_streambuf_char_gptr(&this->base)[-1]==c)) {
        basic_streambuf_char__Gndec(&this->base);
        return c==EOF ? !c : c;
    }else if(c!=EOF && !this->cvt) {
        return ungetc(c, this->file);
    }

    return EOF;
}

/* ?uflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?uflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_uflow, 4)
int __thiscall basic_filebuf_char_uflow(basic_filebuf_char *this)
{
    char ch, buf[128], *to_next;
    const char *buf_next;
    int c, j;
    size_t i;

    TRACE("(%p)\n", this);

    if(!basic_filebuf_char_is_open(this))
        return EOF;

    if(basic_streambuf_char_gptr(&this->base) < basic_streambuf_char_egptr(&this->base))
        return (unsigned char)*basic_streambuf_char__Gninc(&this->base);

    c = fgetc(this->file);
    if(!this->cvt || c==EOF)
        return c;

    buf_next = buf;
    for(i=0; i < ARRAY_SIZE(buf); i++) {
        buf[i] = c;

        switch(codecvt_char_in(this->cvt, &this->state, buf_next,
                    buf+i+1, &buf_next, &ch, &ch+1, &to_next)) {
        case CODECVT_partial:
        case CODECVT_ok:
            if(to_next == &ch) {
                c = fgetc(this->file);
                if(c == EOF)
                    return EOF;
                continue;
            }

            for(j = --i; j >= buf_next-buf; j--)
                ungetc(buf[j], this->file);
            return ch;
        case CODECVT_noconv:
            return (unsigned char)buf[0];
        default:
            return EOF;
        }
    }

    FIXME("buffer is too small\n");
    return EOF;
}

/* ?underflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?underflow@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_underflow, 4)
int __thiscall basic_filebuf_char_underflow(basic_filebuf_char *this)
{
    int ret;

    TRACE("(%p)\n", this);

    if(basic_streambuf_char_gptr(&this->base) < basic_streambuf_char_egptr(&this->base))
        return (unsigned char)*basic_streambuf_char_gptr(&this->base);

    ret = call_basic_streambuf_char_uflow(&this->base);
    if(ret != EOF)
        ret = call_basic_streambuf_char_pbackfail(&this->base, ret);
    return ret;
}

/* ?seekoff@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_seekoff, 24)
#else
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_seekoff, 20)
#endif
fpos_mbstatet* __thiscall basic_filebuf_char_seekoff(basic_filebuf_char *this,
        fpos_mbstatet *ret, streamoff off, int way, int mode)
{
    fpos_t pos;

    TRACE("(%p %p %s %d %d)\n", this, ret, wine_dbgstr_longlong(off), way, mode);

    if(!basic_filebuf_char_is_open(this) || !basic_filebuf_char__Endwrite(this)
            || fseek(this->file, off, way)) {
        ret->off = -1;
        ret->pos = 0;
        memset(&ret->state, 0, sizeof(ret->state));
        return ret;
    }

    fgetpos(this->file, &pos);
    ret->off = 0;
    ret->pos = pos;
    ret->state = this->state;
    return ret;
}

/* ?seekpos@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_seekpos, 36)
fpos_mbstatet* __thiscall basic_filebuf_char_seekpos(basic_filebuf_char *this,
        fpos_mbstatet *ret, fpos_mbstatet pos, int mode)
{
    fpos_t fpos;

    TRACE("(%p %p %s %d)\n", this, ret, debugstr_fpos_mbstatet(&pos), mode);

    if(!basic_filebuf_char_is_open(this) || !basic_filebuf_char__Endwrite(this)
            || fseek(this->file, (LONG)pos.pos, SEEK_SET)
            || (pos.off && fseek(this->file, pos.off, SEEK_CUR))) {
        ret->off = -1;
        ret->pos = 0;
        memset(&ret->state, 0, sizeof(ret->state));
        return ret;
    }

    fgetpos(this->file, &fpos);
    ret->off = 0;
    ret->pos = fpos;
    ret->state = this->state;
    return ret;
}

/* ?setbuf@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEPAV?$basic_streambuf@DU?$char_traits@D@std@@@2@PADH@Z */
/* ?setbuf@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAPEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@PEAD_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_setbuf, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_setbuf, 12)
#endif
basic_streambuf_char* __thiscall basic_filebuf_char_setbuf(basic_filebuf_char *this, char *buf, streamsize count)
{
    TRACE("(%p %p %s)\n", this, buf, wine_dbgstr_longlong(count));

    if(!basic_filebuf_char_is_open(this))
        return NULL;

    if(setvbuf(this->file, buf, (buf==NULL && count==0) ? _IONBF : _IOFBF, count))
        return NULL;

    basic_filebuf_char__Init(this, this->file, INITFL_open);
    return &this->base;
}

/* ?sync@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEHXZ */
/* ?sync@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_sync, 4)
int __thiscall basic_filebuf_char_sync(basic_filebuf_char *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_char_is_open(this))
        return 0;

    if(call_basic_streambuf_char_overflow(&this->base, EOF) == EOF)
        return 0;
    return fflush(this->file);
}

/* ?imbue@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAEXABVlocale@2@@Z */
/* ?imbue@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MEAAXAEBVlocale@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_char_imbue, 8)
void __thiscall basic_filebuf_char_imbue(basic_filebuf_char *this, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
    basic_filebuf_char__Initcvt_cvt(this, codecvt_char_use_facet(loc));
}

/* ?_Stinit@?1??_Init@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@IAEXPAU_iobuf@@W4_Initfl@23@@Z@4HA */
/* ?_Stinit@?1??_Init@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@IEAAXPEAU_iobuf@@W4_Initfl@23@@Z@4HA */
#if _MSVCP_VER >= 140
_Mbstatet basic_filebuf_wchar__Init__Stinit = {0};
#else
_Mbstatet basic_filebuf_wchar__Init__Stinit = 0;
#endif

/* ?_Stinit@?1??_Init@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IAEXPAU_iobuf@@W4_Initfl@23@@Z@4HA */
/* ?_Stinit@?1??_Init@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IEAAXPEAU_iobuf@@W4_Initfl@23@@Z@4HA */
#if _MSVCP_VER >= 140
_Mbstatet basic_filebuf_short__Init__Stinit = {0};
#else
_Mbstatet basic_filebuf_short__Init__Stinit = 0;
#endif

/* ?_Init@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@IAEXPAU_iobuf@@W4_Initfl@12@@Z */
/* ?_Init@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@IEAAXPEAU_iobuf@@W4_Initfl@12@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar__Init, 12)
void __thiscall basic_filebuf_wchar__Init(basic_filebuf_wchar *this, FILE *file, basic_filebuf__Initfl which)
{
    TRACE("(%p %p %d)\n", this, file, which);

    this->cvt = NULL;
    this->wrotesome = FALSE;
    this->state = basic_filebuf_wchar__Init__Stinit;
    this->close = (which == INITFL_open);
    this->file = file;

    basic_streambuf_wchar__Init_empty(&this->base);
}

/* ?_Init@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IAEXPAU_iobuf@@W4_Initfl@12@@Z */
/* ?_Init@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IEAAXPEAU_iobuf@@W4_Initfl@12@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short__Init, 12)
void __thiscall basic_filebuf_short__Init(basic_filebuf_wchar *this, FILE *file, basic_filebuf__Initfl which)
{
    TRACE("(%p %p %d)\n", this, file, which);

    this->cvt = NULL;
    this->wrotesome = FALSE;
    this->state = basic_filebuf_short__Init__Stinit;
    this->close = (which == INITFL_open);
    this->file = file;

    basic_streambuf_wchar__Init_empty(&this->base);
}

/* ?_Initcvt@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@IAEXPAV?$codecvt@_WDH@2@@Z */
/* ?_Initcvt@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@IEAAXPEAV?$codecvt@_WDH@2@@Z */
/* ?_Initcvt@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IAEXPAV?$codecvt@GDH@2@@Z */
/* ?_Initcvt@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IEAAXPEAV?$codecvt@GDH@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar__Initcvt_cvt, 8)
void __thiscall basic_filebuf_wchar__Initcvt_cvt(basic_filebuf_wchar *this, codecvt_wchar *cvt)
{
    TRACE("(%p %p)\n", this, cvt);

    if(codecvt_base_always_noconv(&cvt->base)) {
        this->cvt = NULL;
    }else {
        basic_streambuf_wchar__Init_empty(&this->base);
        this->cvt = cvt;
    }
}

/* ?_Initcvt@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IAEXXZ */
/* ?_Initcvt@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar__Initcvt, 4)
void __thiscall basic_filebuf_wchar__Initcvt(basic_filebuf_wchar *this)
{
    codecvt_wchar *cvt = codecvt_wchar_use_facet(IOS_LOCALE(&this->base));
    basic_filebuf_wchar__Initcvt_cvt( this, cvt );
}

/* ?_Endwrite@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@IAE_NXZ */
/* ?_Endwrite@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@IEAA_NXZ */
/* ?_Endwrite@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IAE_NXZ */
/* ?_Endwrite@?$basic_filebuf@GU?$char_traits@G@std@@@std@@IEAA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar__Endwrite, 4)
bool __thiscall basic_filebuf_wchar__Endwrite(basic_filebuf_wchar *this)
{
    TRACE("(%p)\n", this);

    if(!this->wrotesome || !this->cvt)
        return TRUE;

    if(call_basic_streambuf_wchar_overflow(&this->base, WEOF) == WEOF)
        return FALSE;

    while(1) {
        /* TODO: check if we need a dynamic buffer here */
        char buf[128];
        char *next;
        int ret;

        ret = codecvt_wchar_unshift(this->cvt, &this->state, buf, buf+sizeof(buf), &next);
        switch(ret) {
        case CODECVT_ok:
            this->wrotesome = FALSE;
            /* fall through */
        case CODECVT_partial:
            if(!fwrite(buf, next-buf, 1, this->file))
                return FALSE;
            if(this->wrotesome)
                break;
            /* fall through */
        case CODECVT_noconv:
            if(call_basic_streambuf_wchar_overflow(&this->base, WEOF) == WEOF)
                return FALSE;
            return TRUE;
        default:
            return FALSE;
        }
    }
}

/* ?close@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QAEPAV12@XZ */
/* ?close@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QEAAPEAV12@XZ */
/* ?close@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEPAV12@XZ */
/* ?close@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_close, 4)
basic_filebuf_wchar* __thiscall basic_filebuf_wchar_close(basic_filebuf_wchar *this)
{
    basic_filebuf_wchar *ret = this;

    TRACE("(%p)\n", this);

    if(!this->file)
        return NULL;

    /* TODO: handle exceptions */
    if(!basic_filebuf_wchar__Endwrite(this))
        ret = NULL;
    if(fclose(this->file))
        ret  = NULL;

    basic_filebuf_wchar__Init(this, NULL, INITFL_close);
    return ret;
}

/* ??0?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_ctor_file, 8)
basic_filebuf_wchar* __thiscall basic_filebuf_wchar_ctor_file(basic_filebuf_wchar *this, FILE *file)
{
    TRACE("(%p %p)\n", this, file);

    basic_streambuf_wchar_ctor(&this->base);
    this->base.vtable = &basic_filebuf_wchar_vtable;

    basic_filebuf_wchar__Init(this, file, INITFL_new);
    return this;
}

/* ??0?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_ctor_file, 8)
basic_filebuf_wchar* __thiscall basic_filebuf_short_ctor_file(basic_filebuf_wchar *this, FILE *file)
{
    TRACE("(%p %p)\n", this, file);

    basic_streambuf_short_ctor(&this->base);
    this->base.vtable = &basic_filebuf_short_vtable;

    basic_filebuf_short__Init(this, file, INITFL_new);
    return this;
}

/* ??_F?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ??_F?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_ctor, 4)
basic_filebuf_wchar* __thiscall basic_filebuf_wchar_ctor(basic_filebuf_wchar *this)
{
    return basic_filebuf_wchar_ctor_file(this, NULL);
}

/* ??_F?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_F?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_ctor, 4)
basic_filebuf_wchar* __thiscall basic_filebuf_short_ctor(basic_filebuf_wchar *this)
{
    return basic_filebuf_short_ctor_file(this, NULL);
}

/* ??0?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_ctor_uninitialized, 8)
basic_filebuf_wchar* __thiscall basic_filebuf_wchar_ctor_uninitialized(basic_filebuf_wchar *this, int uninitialized)
{
    TRACE("(%p %d)\n", this, uninitialized);

    basic_streambuf_wchar_ctor(&this->base);
    this->base.vtable = &basic_filebuf_wchar_vtable;
    return this;
}

/* ??0?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_ctor_uninitialized, 8)
basic_filebuf_wchar* __thiscall basic_filebuf_short_ctor_uninitialized(basic_filebuf_wchar *this, int uninitialized)
{
    TRACE("(%p %d)\n", this, uninitialized);

    basic_streambuf_short_ctor(&this->base);
    this->base.vtable = &basic_filebuf_short_vtable;
    return this;
}

/* ??1?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
/* ??1?$basic_filebuf@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_filebuf@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_dtor, 4)
void __thiscall basic_filebuf_wchar_dtor(basic_filebuf_wchar *this)
{
    TRACE("(%p)\n", this);

    if(this->close)
        basic_filebuf_wchar_close(this);
    basic_streambuf_wchar_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_vector_dtor, 8)
basic_filebuf_wchar* __thiscall basic_filebuf_wchar_vector_dtor(basic_filebuf_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_filebuf_wchar_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_filebuf_wchar_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?is_open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QEBA_NXZ */
/* ?is_open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_is_open, 4)
bool __thiscall basic_filebuf_wchar_is_open(const basic_filebuf_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->file != NULL;
}

/* ?open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QAEPAV12@PB_WHH@Z */
/* ?open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QEAAPEAV12@PEB_WHH@Z */
/* ?open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QAEPAV12@PBGHH@Z */
/* ?open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QEAAPEAV12@PEBGHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_open_wchar, 16)
basic_filebuf_wchar* __thiscall basic_filebuf_wchar_open_wchar(basic_filebuf_wchar *this, const wchar_t *name, int mode, int prot)
{
    FILE *f = NULL;

    TRACE("(%p %s %d %d)\n", this, debugstr_w(name), mode, prot);

    if(basic_filebuf_wchar_is_open(this))
        return NULL;

    if(!(f = _Fiopen_wchar(name, mode, prot)))
        return NULL;

    basic_filebuf_wchar__Init(this, f, INITFL_open);
    basic_filebuf_wchar__Initcvt_cvt(this, codecvt_wchar_use_facet(IOS_LOCALE(&this->base)));
    return this;
}

/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PB_WHH@Z */
/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEB_WHH@Z */
/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PBGHH@Z */
/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEBGHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_open_wchar, 16)
basic_filebuf_wchar* __thiscall basic_filebuf_short_open_wchar(basic_filebuf_wchar *this, const wchar_t *name, int mode, int prot)
{
    FILE *f = NULL;

    TRACE("(%p %s %d %d)\n", this, debugstr_w(name), mode, prot);

    if(basic_filebuf_wchar_is_open(this))
        return NULL;

    if(!(f = _Fiopen_wchar(name, mode, prot)))
        return NULL;

    basic_filebuf_short__Init(this, f, INITFL_open);
    basic_filebuf_wchar__Initcvt_cvt(this, codecvt_short_use_facet(IOS_LOCALE(&this->base)));
    return this;
}

/* ?open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QAEPAV12@PB_WI@Z */
/* ?open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QEAAPEAV12@PEB_WI@Z */
/* ?open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QAEPAV12@PBGI@Z */
/* ?open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QEAAPEAV12@PEBGI@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_open_wchar_mode, 12)
basic_filebuf_wchar* __thiscall basic_filebuf_wchar_open_wchar_mode(basic_filebuf_wchar *this, const wchar_t *name, unsigned int mode)
{
    return basic_filebuf_wchar_open_wchar(this, name, mode, SH_DENYNO);
}

/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PB_WI@Z */
/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEB_WI@Z */
/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PBGI@Z */
/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEBGI@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_open_wchar_mode, 12)
basic_filebuf_wchar* __thiscall basic_filebuf_short_open_wchar_mode(basic_filebuf_wchar *this, const wchar_t *name, unsigned int mode)
{
    return basic_filebuf_short_open_wchar(this, name, mode, SH_DENYNO);
}

/* ?open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QAEPAV12@PBDHH@Z */
/* ?open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QEAAPEAV12@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_open, 16)
basic_filebuf_wchar* __thiscall basic_filebuf_wchar_open(basic_filebuf_wchar *this, const char *name, int mode, int prot)
{
    wchar_t nameW[FILENAME_MAX];

    TRACE("(%p %s %d %d)\n", this, name, mode, prot);

    if(mbstowcs_s(NULL, nameW, FILENAME_MAX, name, FILENAME_MAX-1) != 0)
        return NULL;
    return basic_filebuf_wchar_open_wchar(this, nameW, mode, prot);
}

/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PBDHH@Z */
/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_open, 16)
basic_filebuf_wchar* __thiscall basic_filebuf_short_open(basic_filebuf_wchar *this, const char *name, int mode, int prot)
{
    wchar_t nameW[FILENAME_MAX];

    TRACE("(%p %s %d %d)\n", this, name, mode, prot);

    if(mbstowcs_s(NULL, nameW, FILENAME_MAX, name, FILENAME_MAX-1) != 0)
        return NULL;
    return basic_filebuf_short_open_wchar(this, nameW, mode, prot);
}

/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PBDF@Z */
/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEBDF@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_open_mode_old, 12)
basic_filebuf_wchar* __thiscall basic_filebuf_wchar_open_mode_old(basic_filebuf_wchar *this, const char *name, short mode)
{
    TRACE("(%p %p %d)\n", this, name, mode);
    return basic_filebuf_wchar_open(this, name, mode, SH_DENYNO);
}

/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PBDH@Z */
/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEBDH@Z */
/* ?open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QAEPAV12@PBDI@Z */
/* ?open@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@QEAAPEAV12@PEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_open_mode, 12)
basic_filebuf_wchar* __thiscall basic_filebuf_wchar_open_mode(basic_filebuf_wchar *this, const char *name, unsigned int mode)
{
    return basic_filebuf_wchar_open(this, name, mode, SH_DENYNO);
}

/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAEPAV12@PBDI@Z */
/* ?open@?$basic_filebuf@GU?$char_traits@G@std@@@std@@QEAAPEAV12@PEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_open_mode, 12)
basic_filebuf_wchar* __thiscall basic_filebuf_short_open_mode(basic_filebuf_wchar *this, const char *name, unsigned int mode)
{
    return basic_filebuf_short_open(this, name, mode, SH_DENYNO);
}

/* ?overflow@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MAEGG@Z */
/* ?overflow@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MEAAGG@Z */
/* ?overflow@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAEGG@Z */
/* ?overflow@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_overflow, 8)
unsigned short __thiscall basic_filebuf_wchar_overflow(basic_filebuf_wchar *this, unsigned short c)
{
    char buf[8], *dyn_buf, *to_next;
    wchar_t ch = c;
    const wchar_t *from_next;
    int max_size;
    unsigned short ret;


    TRACE("(%p %d)\n", this, c);

    if(!basic_filebuf_wchar_is_open(this))
        return WEOF;
    if(c == WEOF)
        return !c;

    if(!this->cvt)
        return fputwc(ch, this->file);

    from_next = &ch;
    do {
        ret = codecvt_wchar_out(this->cvt, &this->state, from_next, &ch+1,
                &from_next, buf, buf+sizeof(buf), &to_next);

        switch(ret) {
        case CODECVT_partial:
            if(to_next == buf)
                break;
            /* fall through */
        case CODECVT_ok:
            if(!fwrite(buf, to_next-buf, 1, this->file))
                return WEOF;
            if(ret == CODECVT_partial)
                continue;
            return c;
        case CODECVT_noconv:
            return fwrite(&ch, sizeof(wchar_t), 1, this->file) ? c : WEOF;
        default:
            return WEOF;
        }

        break;
    } while(1);

    max_size = codecvt_base_max_length(&this->cvt->base);
    dyn_buf = malloc(max_size);
    if(!dyn_buf)
        return WEOF;

    ret = codecvt_wchar_out(this->cvt, &this->state, from_next, &ch+1,
            &from_next, dyn_buf, dyn_buf+max_size, &to_next);

    switch(ret) {
    case CODECVT_ok:
        ret = fwrite(dyn_buf, to_next-dyn_buf, 1, this->file);
        free(dyn_buf);
        return ret ? c : WEOF;
    case CODECVT_partial:
        ERR("buffer should be big enough to store all output\n");
        /* fall through */
    default:
        free(dyn_buf);
        return WEOF;
    }
}

/* ?pbackfail@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MAEGG@Z */
/* ?pbackfail@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MEAAGG@Z */
/* ?pbackfail@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAEGG@Z */
/* ?pbackfail@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_pbackfail, 8)
unsigned short __thiscall basic_filebuf_wchar_pbackfail(basic_filebuf_wchar *this, unsigned short c)
{
    TRACE("(%p %d)\n", this, c);

    if(!basic_filebuf_wchar_is_open(this))
        return WEOF;

    if(basic_streambuf_wchar_gptr(&this->base)>basic_streambuf_wchar_eback(&this->base)
            && (c==WEOF || basic_streambuf_wchar_gptr(&this->base)[-1]==c)) {
        basic_streambuf_wchar__Gndec(&this->base);
        return c==WEOF ? !c : c;
    }else if(c!=WEOF && !this->cvt) {
        return ungetwc(c, this->file);
    }else if(c!=WEOF && basic_streambuf_wchar_gptr(&this->base)!=&this->putback) {
        this->putback = c;
        basic_streambuf_wchar_setg(&this->base, &this->putback, &this->putback, &this->putback+1);
        return c;
    }

    return WEOF;
}

/* ?uflow@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MAEGXZ */
/* ?uflow@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MEAAGXZ */
/* ?uflow@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAEGXZ */
/* ?uflow@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_uflow, 4)
unsigned short __thiscall basic_filebuf_wchar_uflow(basic_filebuf_wchar *this)
{
    wchar_t ch, *to_next;
    char buf[128];
    const char *buf_next;
    int c, j;
    size_t i;

    TRACE("(%p)\n", this);

    if(!basic_filebuf_wchar_is_open(this))
        return WEOF;

    if(basic_streambuf_wchar_gptr(&this->base) < basic_streambuf_wchar_egptr(&this->base))
        return *basic_streambuf_wchar__Gninc(&this->base);

    if(!this->cvt)
        return fgetwc(this->file);

    buf_next = buf;
    for(i=0; i < ARRAY_SIZE(buf); i++) {
        if((c = fgetc(this->file)) == EOF)
            return WEOF;
        buf[i] = c;

        switch(codecvt_wchar_in(this->cvt, &this->state, buf_next,
                    buf+i+1, &buf_next, &ch, &ch+1, &to_next)) {
        case CODECVT_partial:
        case CODECVT_ok:
            if(to_next == &ch)
                continue;

            for(j = --i; j >= buf_next-buf; j--)
                ungetc(buf[j], this->file);
            return ch;
        case CODECVT_noconv:
            if(i+1 < sizeof(wchar_t))
                continue;

            memcpy(&ch, buf, sizeof(wchar_t));
            return ch;
        default:
            return WEOF;
        }
    }

    FIXME("buffer is too small\n");
    return WEOF;
}

/* ?underflow@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MAEGXZ */
/* ?underflow@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MEAAGXZ */
/* ?underflow@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAEGXZ */
/* ?underflow@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_underflow, 4)
unsigned short __thiscall basic_filebuf_wchar_underflow(basic_filebuf_wchar *this)
{
    unsigned short ret;

    TRACE("(%p)\n", this);

    if(basic_streambuf_wchar_gptr(&this->base) < basic_streambuf_wchar_egptr(&this->base))
        return *basic_streambuf_wchar_gptr(&this->base);

    ret = call_basic_streambuf_wchar_uflow(&this->base);
    if(ret != WEOF)
        ret = call_basic_streambuf_wchar_pbackfail(&this->base, ret);
    return ret;
}

/* ?seekoff@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
/* ?seekoff@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_seekoff, 24)
#else
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_seekoff, 20)
#endif
fpos_mbstatet* __thiscall basic_filebuf_wchar_seekoff(basic_filebuf_wchar *this,
        fpos_mbstatet *ret, streamoff off, int way, int mode)
{
    fpos_t pos;

    TRACE("(%p %p %s %d %d)\n", this, ret, wine_dbgstr_longlong(off), way, mode);

    if(basic_streambuf_wchar_gptr(&this->base) == &this->putback) {
        if(way == SEEKDIR_cur)
            off -= sizeof(wchar_t);

        basic_streambuf_wchar_setg(&this->base, &this->putback, &this->putback+1, &this->putback+1);
    }

    if(!basic_filebuf_wchar_is_open(this) || !basic_filebuf_wchar__Endwrite(this)
            || fseek(this->file, off, way)) {
        ret->off = -1;
        ret->pos = 0;
        memset(&ret->state, 0, sizeof(ret->state));
        return ret;
    }

    fgetpos(this->file, &pos);
    ret->off = 0;
    ret->pos = pos;
    ret->state = this->state;
    return ret;
}

/* ?seekpos@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_seekpos, 36)
fpos_mbstatet* __thiscall basic_filebuf_wchar_seekpos(basic_filebuf_wchar *this,
        fpos_mbstatet *ret, fpos_mbstatet pos, int mode)
{
    fpos_t fpos;

    TRACE("(%p %p %s %d)\n", this, ret, debugstr_fpos_mbstatet(&pos), mode);

    if(!basic_filebuf_wchar_is_open(this) || !basic_filebuf_wchar__Endwrite(this)
            || fseek(this->file, (LONG)pos.pos, SEEK_SET)
            || (pos.off && fseek(this->file, pos.off, SEEK_CUR))) {
        ret->off = -1;
        ret->pos = 0;
        memset(&ret->state, 0, sizeof(ret->state));
        return ret;
    }

    if(basic_streambuf_wchar_gptr(&this->base) == &this->putback)
        basic_streambuf_wchar_setg(&this->base, &this->putback, &this->putback+1, &this->putback+1);

    fgetpos(this->file, &fpos);
    ret->off = 0;
    ret->pos = fpos;
    ret->state = this->state;
    return ret;
}

/* ?setbuf@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MAEPAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@PA_WH@Z */
/* ?setbuf@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MEAAPEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@PEA_W_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_setbuf, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_setbuf, 12)
#endif
basic_streambuf_wchar* __thiscall basic_filebuf_wchar_setbuf(basic_filebuf_wchar *this, wchar_t *buf, streamsize count)
{
    TRACE("(%p %p %s)\n", this, buf, wine_dbgstr_longlong(count));

    if(!basic_filebuf_wchar_is_open(this))
        return NULL;

    if(setvbuf(this->file, (char*)buf, (buf==NULL && count==0) ? _IONBF : _IOFBF, count*sizeof(wchar_t)))
        return NULL;

    basic_filebuf_wchar__Init(this, this->file, INITFL_open);
    return &this->base;
}

/* ?setbuf@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAEPAV?$basic_streambuf@GU?$char_traits@G@std@@@2@PAGH@Z */
/* ?setbuf@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAAPEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@PEAG_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_setbuf, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_setbuf, 12)
#endif
basic_streambuf_wchar* __thiscall basic_filebuf_short_setbuf(basic_filebuf_wchar *this, wchar_t *buf, streamsize count)
{
    TRACE("(%p %p %s)\n", this, buf, wine_dbgstr_longlong(count));

    if(!basic_filebuf_wchar_is_open(this))
        return NULL;

    if(setvbuf(this->file, (char*)buf, (buf==NULL && count==0) ? _IONBF : _IOFBF, count*sizeof(wchar_t)))
        return NULL;

    basic_filebuf_short__Init(this, this->file, INITFL_open);
    return &this->base;
}

/* ?sync@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MAEHXZ */
/* ?sync@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MEAAHXZ */
/* ?sync@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAEHXZ */
/* ?sync@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_sync, 4)
int __thiscall basic_filebuf_wchar_sync(basic_filebuf_wchar *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_wchar_is_open(this))
        return 0;

    if(call_basic_streambuf_wchar_overflow(&this->base, WEOF) == WEOF)
        return 0;
    return fflush(this->file);
}

/* ?imbue@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MAEXABVlocale@2@@Z */
/* ?imbue@?$basic_filebuf@_WU?$char_traits@_W@std@@@std@@MEAAXAEBVlocale@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_wchar_imbue, 8)
void __thiscall basic_filebuf_wchar_imbue(basic_filebuf_wchar *this, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
    basic_filebuf_wchar__Initcvt_cvt(this, codecvt_wchar_use_facet(loc));
}

/* ?imbue@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAEXABVlocale@2@@Z */
/* ?imbue@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MEAAXAEBVlocale@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_filebuf_short_imbue, 8)
void __thiscall basic_filebuf_short_imbue(basic_filebuf_wchar *this, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
    basic_filebuf_wchar__Initcvt_cvt(this, codecvt_short_use_facet(loc));
}

/* ?_Getstate@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEHH@Z */
/* ?_Getstate@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAAHH@Z */
/* ?_Mode@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEHH@Z */
/* ?_Mode@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char__Getstate, 8)
int __thiscall basic_stringbuf_char__Getstate(basic_stringbuf_char *this, IOSB_openmode mode)
{
    int state = 0;

    if(!(mode & OPENMODE_in))
        state |= STRINGBUF_no_read;

    if(!(mode & OPENMODE_out))
        state |= STRINGBUF_no_write;

    if(mode & OPENMODE_ate)
        state |= STRINGBUF_at_end;

    if(mode & OPENMODE_app)
        state |= STRINGBUF_append;

    return state;
}

/* ?_Init@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAEXPBDIH@Z */
/* ?_Init@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAAXPEBD_KH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char__Init, 16)
void __thiscall basic_stringbuf_char__Init(basic_stringbuf_char *this, const char *str, size_t count, int state)
{
    TRACE("(%p, %p, %Iu, %d)\n", this, str, count, state);

    basic_streambuf_char__Init_empty(&this->base);

    this->state = state;
    this->seekhigh = NULL;

    if(count && str) {
        char *buf = operator_new(count);

        memcpy(buf, str, count);
        this->seekhigh = buf + count;

        this->state |= STRINGBUF_allocated;

        if(!(state & STRINGBUF_no_read))
            basic_streambuf_char_setg(&this->base, buf, buf, buf + count);

        if(!(state & STRINGBUF_no_write)) {
            basic_streambuf_char_setp_next(&this->base, buf, (state & STRINGBUF_at_end) ? buf + count : buf, buf + count);

            if(!basic_streambuf_char_gptr(&this->base))
                basic_streambuf_char_setg(&this->base, buf, 0, buf);
        }
    }
}

/* ??0?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
/* ??0?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_ctor_str, 12)
basic_stringbuf_char* __thiscall basic_stringbuf_char_ctor_str(basic_stringbuf_char *this,
        const basic_string_char *str, IOSB_openmode mode)
{
    TRACE("(%p %p %d)\n", this, str, mode);

    basic_streambuf_char_ctor(&this->base);
    this->base.vtable = &basic_stringbuf_char_vtable;

    basic_stringbuf_char__Init(this, MSVCP_basic_string_char_c_str(str),
            str->size, basic_stringbuf_char__Getstate(this, mode));
    return this;
}

/* ??0?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z */
/* ??0?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_ctor_mode, 8)
basic_stringbuf_char* __thiscall basic_stringbuf_char_ctor_mode(
        basic_stringbuf_char *this, IOSB_openmode mode)
{
    TRACE("(%p %d)\n", this, mode);

    basic_streambuf_char_ctor(&this->base);
    this->base.vtable = &basic_stringbuf_char_vtable;

    basic_stringbuf_char__Init(this, NULL, 0, basic_stringbuf_char__Getstate(this, mode));
    return this;
}

/* ??_F?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_F?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_ctor, 4)
basic_stringbuf_char* __thiscall basic_stringbuf_char_ctor(basic_stringbuf_char *this)
{
    return basic_stringbuf_char_ctor_mode(this, OPENMODE_in|OPENMODE_out);
}

/* ?_Tidy@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAEXXZ */
/* ?_Tidy@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char__Tidy, 4)
void __thiscall basic_stringbuf_char__Tidy(basic_stringbuf_char *this)
{
    TRACE("(%p)\n", this);

    if(this->state & STRINGBUF_allocated) {
        operator_delete(basic_streambuf_char_eback(&this->base));
        this->seekhigh = NULL;
        this->state &= ~STRINGBUF_allocated;
    }

    basic_streambuf_char__Init_empty(&this->base);
}

/* ??1?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UAE@XZ */
/* ??1?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_dtor, 4)
void __thiscall basic_stringbuf_char_dtor(basic_stringbuf_char *this)
{
    TRACE("(%p)\n", this);

    basic_stringbuf_char__Tidy(this);
    basic_streambuf_char_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_vector_dtor, 8)
basic_stringbuf_char* __thiscall basic_stringbuf_char_vector_dtor(basic_stringbuf_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *) this - 1;

        for (i = *ptr - 1; i >= 0; i--)
            basic_stringbuf_char_dtor(this+i);

        operator_delete(ptr);
    }else {
        basic_stringbuf_char_dtor(this);

        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?overflow@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAEHH@Z */
/* ?overflow@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_overflow, 8)
int __thiscall basic_stringbuf_char_overflow(basic_stringbuf_char *this, int meta)
{
    size_t oldsize, size;
    char *ptr, *buf;

    TRACE("(%p %x)\n", this, meta);

    if(meta == EOF)
        return !EOF;
    if(this->state & STRINGBUF_no_write)
        return EOF;

    ptr = basic_streambuf_char_pptr(&this->base);
    if((this->state&STRINGBUF_append) && ptr<this->seekhigh)
        basic_streambuf_char_setp_next(&this->base, basic_streambuf_char_pbase(&this->base),
                this->seekhigh, basic_streambuf_char_epptr(&this->base));

    if(ptr && ptr<basic_streambuf_char_epptr(&this->base))
        return (unsigned char)(*basic_streambuf_char__Pninc(&this->base) = meta);

    oldsize = (ptr ? basic_streambuf_char_epptr(&this->base)-basic_streambuf_char_eback(&this->base): 0);
    size = oldsize|0xf;
    size += size/2;
    buf = operator_new(size);

    if(!oldsize) {
        this->seekhigh = buf;
        basic_streambuf_char_setp(&this->base, buf, buf+size);
        if(this->state & STRINGBUF_no_read)
            basic_streambuf_char_setg(&this->base, buf, NULL, buf);
        else
            basic_streambuf_char_setg(&this->base, buf, buf, buf+1);

        this->state |= STRINGBUF_allocated;
    }else {
        ptr = basic_streambuf_char_eback(&this->base);
        memcpy(buf, ptr, oldsize);

        this->seekhigh = buf+(this->seekhigh-ptr);
        basic_streambuf_char_setp_next(&this->base, buf,
                buf+(basic_streambuf_char_pptr(&this->base)-ptr), buf+size);
        if(this->state & STRINGBUF_no_read)
            basic_streambuf_char_setg(&this->base, buf, NULL, buf);
        else
            basic_streambuf_char_setg(&this->base, buf,
                    buf+(basic_streambuf_char_gptr(&this->base)-ptr),
                    basic_streambuf_char_pptr(&this->base)+1);

        operator_delete(ptr);
    }

    return (unsigned char)(*basic_streambuf_char__Pninc(&this->base) = meta);
}

/* ?pbackfail@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAEHH@Z */
/* ?pbackfail@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_pbackfail, 8)
int __thiscall basic_stringbuf_char_pbackfail(basic_stringbuf_char *this, int c)
{
    char *cur;

    TRACE("(%p %x)\n", this, c);

    cur = basic_streambuf_char_gptr(&this->base);
    if(!cur || cur==basic_streambuf_char_eback(&this->base)
            || (c!=EOF && c!=cur[-1] && this->state&STRINGBUF_no_write))
        return EOF;

    if(c != EOF)
        cur[-1] = c;
    basic_streambuf_char_gbump(&this->base, -1);
    return c==EOF ? !EOF : c;
}

/* ?underflow@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAEHXZ */
/* ?underflow@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_underflow, 4)
int __thiscall basic_stringbuf_char_underflow(basic_stringbuf_char *this)
{
    char *ptr, *cur;

    TRACE("(%p)\n", this);

    cur = basic_streambuf_char_gptr(&this->base);
    if(!cur || this->state&STRINGBUF_no_read)
        return EOF;

    ptr  = basic_streambuf_char_pptr(&this->base);
    if(this->seekhigh < ptr)
        this->seekhigh = ptr;

    ptr = basic_streambuf_char_egptr(&this->base);
    if(this->seekhigh > ptr)
        basic_streambuf_char_setg(&this->base, basic_streambuf_char_eback(&this->base), cur, this->seekhigh);

    if(cur < this->seekhigh)
        return (unsigned char)*cur;
    return EOF;
}

/* ?seekoff@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_seekoff, 24)
#else
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_seekoff, 20)
#endif
fpos_mbstatet* __thiscall basic_stringbuf_char_seekoff(basic_stringbuf_char *this,
        fpos_mbstatet *ret, streamoff off, int way, int mode)
{
    char *beg, *cur_r, *cur_w;

    TRACE("(%p %p %s %d %d)\n", this, ret, wine_dbgstr_longlong(off), way, mode);

    cur_w = basic_streambuf_char_pptr(&this->base);
    if(cur_w > this->seekhigh)
        this->seekhigh = cur_w;

    ret->off = 0;
    ret->pos = 0;
    memset(&ret->state, 0, sizeof(ret->state));

    beg = basic_streambuf_char_eback(&this->base);
    cur_r = basic_streambuf_char_gptr(&this->base);
    if((mode & OPENMODE_in) && cur_r) {
        if(way==SEEKDIR_cur && !(mode & OPENMODE_out))
            off += cur_r-beg;
        else if(way == SEEKDIR_end)
            off += this->seekhigh-beg;
        else if(way != SEEKDIR_beg)
            off = -1;

        if(off<0 || off>this->seekhigh-beg) {
            off = -1;
        }else {
            basic_streambuf_char_gbump(&this->base, beg-cur_r+off);
            if((mode & OPENMODE_out) && cur_w) {
                basic_streambuf_char_setp_next(&this->base, beg,
                        basic_streambuf_char_gptr(&this->base),
                        basic_streambuf_char_epptr(&this->base));
            }
        }
    }else if((mode & OPENMODE_out) && cur_w) {
        if(way == SEEKDIR_cur)
            off += cur_w-beg;
        else if(way == SEEKDIR_end)
            off += this->seekhigh-beg;
        else if(way != SEEKDIR_beg)
            off = -1;

        if(off<0 || off>this->seekhigh-beg)
            off = -1;
        else
            basic_streambuf_char_pbump(&this->base, beg-cur_w+off);
    }else {
        off = -1;
    }

    ret->off = off;
    return ret;
}

/* ?seekpos@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_seekpos, 36)
fpos_mbstatet* __thiscall basic_stringbuf_char_seekpos(basic_stringbuf_char *this,
        fpos_mbstatet *ret, fpos_mbstatet pos, int mode)
{
    TRACE("(%p %p %s %d)\n", this, ret, debugstr_fpos_mbstatet(&pos), mode);

    if(pos.off==-1 && pos.pos==0 && MBSTATET_TO_INT(&pos.state)==0) {
        *ret = pos;
        return ret;
    }

    return basic_stringbuf_char_seekoff(this, ret, pos.pos+pos.off, SEEKDIR_beg, mode);
}

/* ?str@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
/* ?str@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_str_set, 8)
void __thiscall basic_stringbuf_char_str_set(basic_stringbuf_char *this, const basic_string_char *str)
{
    TRACE("(%p %p)\n", this, str);

    basic_stringbuf_char__Tidy(this);
    basic_stringbuf_char__Init(this, MSVCP_basic_string_char_c_str(str), str->size, this->state);
}

/* ?str@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?str@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_char_str_get, 8)
basic_string_char* __thiscall basic_stringbuf_char_str_get(const basic_stringbuf_char *this, basic_string_char *ret)
{
    char *ptr;

    TRACE("(%p)\n", this);

    if(!(this->state & STRINGBUF_no_write) && basic_streambuf_char_pptr(&this->base)) {
        char *pptr;

        ptr = basic_streambuf_char_pbase(&this->base);
        pptr = basic_streambuf_char_pptr(&this->base);

        return MSVCP_basic_string_char_ctor_cstr_len(ret, ptr, (this->seekhigh < pptr ? pptr : this->seekhigh) - ptr);
    }

    if(!(this->state & STRINGBUF_no_read) && basic_streambuf_char_gptr(&this->base)) {
        ptr = basic_streambuf_char_eback(&this->base);
        return MSVCP_basic_string_char_ctor_cstr_len(ret, ptr, basic_streambuf_char_egptr(&this->base) - ptr);
    }

    return MSVCP_basic_string_char_ctor(ret);
}

/* ?_Getstate@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@AAEHH@Z */
/* ?_Getstate@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@AEAAHH@Z */
/* ?_Getstate@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AAEHH@Z */
/* ?_Getstate@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AEAAHH@Z */
/* ?_Mode@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AAEHH@Z */
/* ?_Mode@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@AEAAHH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar__Getstate, 8)
int __thiscall basic_stringbuf_wchar__Getstate(basic_stringbuf_wchar *this, IOSB_openmode mode)
{
    int state = 0;

    if(!(mode & OPENMODE_in))
        state |= STRINGBUF_no_read;

    if(!(mode & OPENMODE_out))
        state |= STRINGBUF_no_write;

    if(mode & OPENMODE_ate)
        state |= STRINGBUF_at_end;

    if(mode & OPENMODE_app)
        state |= STRINGBUF_append;

    return state;
}

/* ?_Init@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAEXPB_WIH@Z */
/* ?_Init@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAAXPEB_W_KH@Z */
/* ?_Init@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEXPBGIH@Z */
/* ?_Init@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAXPEBG_KH@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar__Init, 16)
void __thiscall basic_stringbuf_wchar__Init(basic_stringbuf_wchar *this, const wchar_t *str, size_t count, int state)
{
    TRACE("(%p, %p, %Iu, %d)\n", this, str, count, state);

    basic_streambuf_wchar__Init_empty(&this->base);

    this->state = state;
    this->seekhigh = NULL;

    if(count && str) {
        wchar_t *buf = operator_new(count*sizeof(wchar_t));

        memcpy(buf, str, count*sizeof(wchar_t));
        this->seekhigh = buf + count;

        this->state |= STRINGBUF_allocated;

        if(!(state & STRINGBUF_no_read))
            basic_streambuf_wchar_setg(&this->base, buf, buf, buf + count);

        if(!(state & STRINGBUF_no_write)) {
            basic_streambuf_wchar_setp_next(&this->base, buf, (state & STRINGBUF_at_end) ? buf + count : buf, buf + count);

            if(!basic_streambuf_wchar_gptr(&this->base))
                basic_streambuf_wchar_setg(&this->base, buf, 0, buf);
        }
    }
}

/* ??0?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@ABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
/* ??0?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_ctor_str, 12)
basic_stringbuf_wchar* __thiscall basic_stringbuf_wchar_ctor_str(basic_stringbuf_wchar *this,
        const basic_string_wchar *str, IOSB_openmode mode)
{
    TRACE("(%p %p %d)\n", this, str, mode);

    basic_streambuf_wchar_ctor(&this->base);
    this->base.vtable = &basic_stringbuf_wchar_vtable;

    basic_stringbuf_wchar__Init(this, MSVCP_basic_string_wchar_c_str(str),
            str->size, basic_stringbuf_wchar__Getstate(this, mode));
    return this;
}

/* ??0?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
/* ??0?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_ctor_str, 12)
basic_stringbuf_wchar* __thiscall basic_stringbuf_short_ctor_str(basic_stringbuf_wchar *this,
        const basic_string_wchar *str, IOSB_openmode mode)
{
    basic_stringbuf_wchar_ctor_str(this, str, mode);
    this->base.vtable = &basic_stringbuf_short_vtable;
    return this;
}

/* ??0?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@H@Z */
/* ??0?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_ctor_mode, 8)
basic_stringbuf_wchar* __thiscall basic_stringbuf_wchar_ctor_mode(
        basic_stringbuf_wchar *this, IOSB_openmode mode)
{
    TRACE("(%p %d)\n", this, mode);

    basic_streambuf_wchar_ctor(&this->base);
    this->base.vtable = &basic_stringbuf_wchar_vtable;

    basic_stringbuf_wchar__Init(this, NULL, 0, basic_stringbuf_wchar__Getstate(this, mode));
    return this;
}

/* ??0?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@H@Z */
/* ??0?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_ctor_mode, 8)
basic_stringbuf_wchar* __thiscall basic_stringbuf_short_ctor_mode(
        basic_stringbuf_wchar *this, IOSB_openmode mode)
{
    basic_stringbuf_wchar_ctor_mode(this, mode);
    this->base.vtable = &basic_stringbuf_short_vtable;
    return this;
}

/* ??_F?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_F?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_ctor, 4)
basic_stringbuf_wchar* __thiscall basic_stringbuf_wchar_ctor(basic_stringbuf_wchar *this)
{
    return basic_stringbuf_wchar_ctor_mode(this, OPENMODE_in|OPENMODE_out);
}

/* ??_F?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_F?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_short_ctor, 4)
basic_stringbuf_wchar* __thiscall basic_stringbuf_short_ctor(basic_stringbuf_wchar *this)
{
    return basic_stringbuf_short_ctor_mode(this, OPENMODE_in|OPENMODE_out);
}

/* ?_Tidy@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAEXXZ */
/* ?_Tidy@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAAXXZ */
/* ?_Tidy@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEXXZ */
/* ?_Tidy@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar__Tidy, 4)
void __thiscall basic_stringbuf_wchar__Tidy(basic_stringbuf_wchar *this)
{
    TRACE("(%p)\n", this);

    if(this->state & STRINGBUF_allocated) {
        operator_delete(basic_streambuf_wchar_eback(&this->base));
        this->seekhigh = NULL;
        this->state &= ~STRINGBUF_allocated;
    }

    basic_streambuf_wchar__Init_empty(&this->base);
}

/* ??1?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UAE@XZ */
/* ??1?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UEAA@XZ */
/* ??1?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UAE@XZ */
/* ??1?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_dtor, 4)
void __thiscall basic_stringbuf_wchar_dtor(basic_stringbuf_wchar *this)
{
    TRACE("(%p)\n", this);

    basic_stringbuf_wchar__Tidy(this);
    basic_streambuf_wchar_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_vector_dtor, 8)
basic_stringbuf_wchar* __thiscall basic_stringbuf_wchar_vector_dtor(basic_stringbuf_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *) this - 1;

        for (i = *ptr - 1; i >= 0; i--)
            basic_stringbuf_wchar_dtor(this+i);

        operator_delete(ptr);
    }else {
        basic_stringbuf_wchar_dtor(this);

        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?overflow@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MAEGG@Z */
/* ?overflow@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MEAAGG@Z */
/* ?overflow@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAEGG@Z */
/* ?overflow@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_overflow, 8)
unsigned short __thiscall basic_stringbuf_wchar_overflow(basic_stringbuf_wchar *this, unsigned short meta)
{
    size_t oldsize, size;
    wchar_t *ptr, *buf;

    TRACE("(%p %x)\n", this, meta);

    if(meta == WEOF)
        return !WEOF;
    if(this->state & STRINGBUF_no_write)
        return WEOF;

    ptr = basic_streambuf_wchar_pptr(&this->base);
    if((this->state&STRINGBUF_append) && ptr<this->seekhigh)
        basic_streambuf_wchar_setp_next(&this->base, basic_streambuf_wchar_pbase(&this->base),
                this->seekhigh, basic_streambuf_wchar_epptr(&this->base));

    if(ptr && ptr<basic_streambuf_wchar_epptr(&this->base))
        return (*basic_streambuf_wchar__Pninc(&this->base) = meta);

    oldsize = (ptr ? basic_streambuf_wchar_epptr(&this->base)-basic_streambuf_wchar_eback(&this->base): 0);
    size = oldsize|0xf;
    size += size/2;
    buf = operator_new(size*sizeof(wchar_t));

    if(!oldsize) {
        this->seekhigh = buf;
        basic_streambuf_wchar_setp(&this->base, buf, buf+size);
        if(this->state & STRINGBUF_no_read)
            basic_streambuf_wchar_setg(&this->base, buf, NULL, buf);
        else
            basic_streambuf_wchar_setg(&this->base, buf, buf, buf+1);

        this->state |= STRINGBUF_allocated;
    }else {
        ptr = basic_streambuf_wchar_eback(&this->base);
        memcpy(buf, ptr, oldsize*sizeof(wchar_t));

        this->seekhigh = buf+(this->seekhigh-ptr);
        basic_streambuf_wchar_setp_next(&this->base, buf,
                buf+(basic_streambuf_wchar_pptr(&this->base)-ptr), buf+size);
        if(this->state & STRINGBUF_no_read)
            basic_streambuf_wchar_setg(&this->base, buf, NULL, buf);
        else
            basic_streambuf_wchar_setg(&this->base, buf,
                    buf+(basic_streambuf_wchar_gptr(&this->base)-ptr),
                    basic_streambuf_wchar_pptr(&this->base)+1);

        operator_delete(ptr);
    }

    return (*basic_streambuf_wchar__Pninc(&this->base) = meta);
}

/* ?pbackfail@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MAEGG@Z */
/* ?pbackfail@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MEAAGG@Z */
/* ?pbackfail@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAEGG@Z */
/* ?pbackfail@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_pbackfail, 8)
unsigned short __thiscall basic_stringbuf_wchar_pbackfail(basic_stringbuf_wchar *this, unsigned short c)
{
    wchar_t *cur;

    TRACE("(%p %x)\n", this, c);

    cur = basic_streambuf_wchar_gptr(&this->base);
    if(!cur || cur==basic_streambuf_wchar_eback(&this->base)
            || (c!=WEOF && c!=cur[-1] && this->state&STRINGBUF_no_write))
        return WEOF;

    if(c != WEOF)
        cur[-1] = c;
    basic_streambuf_wchar_gbump(&this->base, -1);
    return c==WEOF ? !WEOF : c;
}

/* ?underflow@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MAEGXZ */
/* ?underflow@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MEAAGXZ */
/* ?underflow@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAEGXZ */
/* ?underflow@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_underflow, 4)
unsigned short __thiscall basic_stringbuf_wchar_underflow(basic_stringbuf_wchar *this)
{
    wchar_t *ptr, *cur;

    TRACE("(%p)\n", this);

    cur = basic_streambuf_wchar_gptr(&this->base);
    if(!cur || this->state&STRINGBUF_no_read)
        return WEOF;

    ptr  = basic_streambuf_wchar_pptr(&this->base);
    if(this->seekhigh < ptr)
        this->seekhigh = ptr;

    ptr = basic_streambuf_wchar_egptr(&this->base);
    if(this->seekhigh > ptr)
        basic_streambuf_wchar_setg(&this->base, basic_streambuf_wchar_eback(&this->base), cur, this->seekhigh);

    if(cur < this->seekhigh)
        return *cur;
    return WEOF;
}

/* ?seekoff@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
/* ?seekoff@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_seekoff, 24)
#else
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_seekoff, 20)
#endif
fpos_mbstatet* __thiscall basic_stringbuf_wchar_seekoff(basic_stringbuf_wchar *this,
        fpos_mbstatet *ret, streamoff off, int way, int mode)
{
    wchar_t *beg, *cur_r, *cur_w;

    TRACE("(%p %p %s %d %d)\n", this, ret, wine_dbgstr_longlong(off), way, mode);

    cur_w = basic_streambuf_wchar_pptr(&this->base);
    if(cur_w > this->seekhigh)
        this->seekhigh = cur_w;

    ret->off = 0;
    ret->pos = 0;
    memset(&ret->state, 0, sizeof(ret->state));

    beg = basic_streambuf_wchar_eback(&this->base);
    cur_r = basic_streambuf_wchar_gptr(&this->base);
    if((mode & OPENMODE_in) && cur_r) {
        if(way==SEEKDIR_cur && !(mode & OPENMODE_out))
            off += cur_r-beg;
        else if(way == SEEKDIR_end)
            off += this->seekhigh-beg;
        else if(way != SEEKDIR_beg)
            off = -1;

        if(off<0 || off>this->seekhigh-beg) {
            off = -1;
        }else {
            basic_streambuf_wchar_gbump(&this->base, beg-cur_r+off);
            if((mode & OPENMODE_out) && cur_w) {
                basic_streambuf_wchar_setp_next(&this->base, beg,
                        basic_streambuf_wchar_gptr(&this->base),
                        basic_streambuf_wchar_epptr(&this->base));
            }
        }
    }else if((mode & OPENMODE_out) && cur_w) {
        if(way == SEEKDIR_cur)
            off += cur_w-beg;
        else if(way == SEEKDIR_end)
            off += this->seekhigh-beg;
        else if(way != SEEKDIR_beg)
            off = -1;

        if(off<0 || off>this->seekhigh-beg)
            off = -1;
        else
            basic_streambuf_wchar_pbump(&this->base, beg-cur_w+off);
    }else {
        off = -1;
    }

    ret->off = off;
    return ret;
}

/* ?seekpos@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_seekpos, 36)
fpos_mbstatet* __thiscall basic_stringbuf_wchar_seekpos(basic_stringbuf_wchar *this,
        fpos_mbstatet *ret, fpos_mbstatet pos, int mode)
{
    TRACE("(%p %p %s %d)\n", this, ret, debugstr_fpos_mbstatet(&pos), mode);

    if(pos.off==-1 && pos.pos==0 && MBSTATET_TO_INT(&pos.state)==0) {
        *ret = pos;
        return ret;
    }

    return basic_stringbuf_wchar_seekoff(this, ret, pos.pos+pos.off, SEEKDIR_beg, mode);
}

/* ?str@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?str@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
/* ?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_str_set, 8)
void __thiscall basic_stringbuf_wchar_str_set(basic_stringbuf_wchar *this, const basic_string_wchar *str)
{
    TRACE("(%p %p)\n", this, str);

    basic_stringbuf_wchar__Tidy(this);
    basic_stringbuf_wchar__Init(this, MSVCP_basic_string_wchar_c_str(str), str->size, this->state);
}

/* ?str@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?str@?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringbuf_wchar_str_get, 8)
basic_string_wchar* __thiscall basic_stringbuf_wchar_str_get(const basic_stringbuf_wchar *this, basic_string_wchar *ret)
{
    wchar_t *ptr;

    TRACE("(%p)\n", this);

    if(!(this->state & STRINGBUF_no_write) && basic_streambuf_wchar_pptr(&this->base)) {
        wchar_t *pptr;

        ptr = basic_streambuf_wchar_pbase(&this->base);
        pptr = basic_streambuf_wchar_pptr(&this->base);

        return MSVCP_basic_string_wchar_ctor_cstr_len(ret, ptr, (this->seekhigh < pptr ? pptr : this->seekhigh) - ptr);
    }

    if(!(this->state & STRINGBUF_no_read) && basic_streambuf_wchar_gptr(&this->base)) {
        ptr = basic_streambuf_wchar_eback(&this->base);
        return MSVCP_basic_string_wchar_ctor_cstr_len(ret, ptr, basic_streambuf_wchar_egptr(&this->base) - ptr);
    }

    return MSVCP_basic_string_wchar_ctor(ret);
}

/* ??0ios_base@std@@IAE@XZ */
/* ??0ios_base@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_ctor, 4)
ios_base* __thiscall ios_base_ctor(ios_base *this)
{
    TRACE("(%p)\n", this);
    this->vtable = &ios_base_vtable;
    return this;
}

/* ??0ios_base@std@@QAE@ABV01@@Z */
/* ??0ios_base@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_copy_ctor, 8)
ios_base* __thiscall ios_base_copy_ctor(ios_base *this, const ios_base *copy)
{
    TRACE("(%p %p)\n", this, copy);
    *this = *copy;
    this->vtable = &ios_base_vtable;
    return this;
}

/* ?_Callfns@ios_base@std@@AAEXW4event@12@@Z */
/* ?_Callfns@ios_base@std@@AEAAXW4event@12@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_Callfns, 8)
void __thiscall ios_base_Callfns(ios_base *this, IOS_BASE_event event)
{
    IOS_BASE_fnarray *cur;

    TRACE("(%p %x)\n", this, event);

    for(cur=this->calls; cur; cur=cur->next)
        cur->event_handler(event, this, cur->index);
}

/* ?_Tidy@ios_base@std@@AAAXXZ */
/* ?_Tidy@ios_base@std@@AAEXXZ */
/* ?_Tidy@ios_base@std@@AEAAXXZ */
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
void __cdecl ios_base_Tidy(ios_base *this)
#else
DEFINE_THISCALL_WRAPPER(ios_base_Tidy, 4)
void __thiscall ios_base_Tidy(ios_base *this)
#endif
{
    IOS_BASE_iosarray *arr_cur, *arr_next;
    IOS_BASE_fnarray *event_cur, *event_next;

    TRACE("(%p)\n", this);

    ios_base_Callfns(this, EVENT_erase_event);

    for(arr_cur=this->arr; arr_cur; arr_cur=arr_next) {
        arr_next = arr_cur->next;
        operator_delete(arr_cur);
    }
    this->arr = NULL;

    for(event_cur=this->calls; event_cur; event_cur=event_next) {
        event_next = event_cur->next;
        operator_delete(event_cur);
    }
    this->calls = NULL;
}

/* ?_Ios_base_dtor@ios_base@std@@CAXPAV12@@Z */
/* ?_Ios_base_dtor@ios_base@std@@CAXPEAV12@@Z */
void __cdecl ios_base_Ios_base_dtor(ios_base *obj)
{
    TRACE("(%p)\n", obj);
    locale_dtor(IOS_LOCALE(obj));
#if _MSVCP_VER >= 70
    operator_delete(obj->loc);
#endif
    ios_base_Tidy(obj);
}

/* ??1ios_base@std@@UAE@XZ */
/* ??1ios_base@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_dtor, 4)
void __thiscall ios_base_dtor(ios_base *this)
{
    ios_base_Ios_base_dtor(this);
}

DEFINE_THISCALL_WRAPPER(ios_base_vector_dtor, 8)
ios_base* __thiscall ios_base_vector_dtor(ios_base *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            ios_base_dtor(this+i);
        operator_delete(ptr);
    } else {
        ios_base_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

DEFINE_THISCALL_WRAPPER(iosb_vector_dtor, 8)
void* __thiscall iosb_vector_dtor(void *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        INT_PTR *ptr = (INT_PTR *)this-1;
        operator_delete(ptr);
    } else {
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Findarr@ios_base@std@@AAEAAU_Iosarray@12@H@Z */
/* ?_Findarr@ios_base@std@@AEAAAEAU_Iosarray@12@H@Z */
DEFINE_THISCALL_WRAPPER(ios_base_Findarr, 8)
IOS_BASE_iosarray* __thiscall ios_base_Findarr(ios_base *this, int index)
{
    IOS_BASE_iosarray *p;

    TRACE("(%p %d)\n", this, index);

    for(p=this->arr; p; p=p->next) {
        if(p->index == index)
            return p;
    }

    for(p=this->arr; p; p=p->next) {
        if(!p->long_val && !p->ptr_val) {
            p->index = index;
            return p;
        }
    }

    p = operator_new(sizeof(IOS_BASE_iosarray));
    p->next = this->arr;
    p->index = index;
    p->long_val = 0;
    p->ptr_val = NULL;
    this->arr = p;
    return p;
}

/* ?iword@ios_base@std@@QAEAAJH@Z */
/* ?iword@ios_base@std@@QEAAAEAJH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_iword, 8)
LONG* __thiscall ios_base_iword(ios_base *this, int index)
{
    TRACE("(%p %d)\n", this, index);
    return &ios_base_Findarr(this, index)->long_val;
}

/* ?pword@ios_base@std@@QAEAAPAXH@Z */
/* ?pword@ios_base@std@@QEAAAEAPEAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_pword, 8)
void** __thiscall ios_base_pword(ios_base *this, int index)
{
    TRACE("(%p %d)\n", this, index);
    return &ios_base_Findarr(this, index)->ptr_val;
}

/* ?register_callback@ios_base@std@@QAEXP6AXW4event@12@AAV12@H@ZH@Z */
/* ?register_callback@ios_base@std@@QEAAXP6AXW4event@12@AEAV12@H@ZH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_register_callback, 12)
void __thiscall ios_base_register_callback(ios_base *this, IOS_BASE_event_callback callback, int index)
{
    IOS_BASE_fnarray *event;

    TRACE("(%p %p %d)\n", this, callback, index);

    event = operator_new(sizeof(IOS_BASE_fnarray));
    event->next = this->calls;
    event->index = index;
    event->event_handler = callback;
    this->calls = event;
}

/* ?clear@ios_base@std@@QAEXH_N@Z */
/* ?clear@ios_base@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(ios_base_clear_reraise, 12)
void __thiscall ios_base_clear_reraise(ios_base *this, IOSB_iostate state, bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);

    this->state = state & IOSTATE_mask;
    if(!(this->state & this->except))
        return;

    if(reraise)
        _CxxThrowException(NULL, NULL);
    else if(this->state & this->except & IOSTATE_eofbit)
        throw_failure("eofbit is set");
    else if(this->state & this->except & IOSTATE_failbit)
        throw_failure("failbit is set");
    else if(this->state & this->except & IOSTATE_badbit)
        throw_failure("badbit is set");
    else if(this->state & this->except & IOSTATE__Hardfail)
        throw_failure("_Hardfail is set");
}

/* ?clear@ios_base@std@@QAEXH@Z */
/* ?clear@ios_base@std@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_clear, 8)
void __thiscall ios_base_clear(ios_base *this, IOSB_iostate state)
{
    ios_base_clear_reraise(this, state, FALSE);
}

/* ?clear@ios_base@std@@QAEXI@Z */
/* ?clear@ios_base@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(ios_base_clear_unsigned, 8)
void __thiscall ios_base_clear_unsigned(ios_base *this, unsigned int state)
{
    ios_base_clear_reraise(this, (IOSB_iostate)state, FALSE);
}

/* ?exceptions@ios_base@std@@QAEXH@Z */
/* ?exceptions@ios_base@std@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_exceptions_set, 8)
void __thiscall ios_base_exceptions_set(ios_base *this, IOSB_iostate state)
{
    TRACE("(%p %x)\n", this, state);
    this->except = state & IOSTATE_mask;
    ios_base_clear(this, this->state);
}

/* ?exceptions@ios_base@std@@QAEXI@Z */
/* ?exceptions@ios_base@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(ios_base_exceptions_set_unsigned, 8)
void __thiscall ios_base_exceptions_set_unsigned(ios_base *this, unsigned int state)
{
    TRACE("(%p %x)\n", this, state);
    ios_base_exceptions_set(this, state);
}

/* ?exceptions@ios_base@std@@QBEHXZ */
/* ?exceptions@ios_base@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_base_exceptions_get, 4)
IOSB_iostate __thiscall ios_base_exceptions_get(ios_base *this)
{
    TRACE("(%p)\n", this);
    return this->except;
}

/* ?copyfmt@ios_base@std@@QAEAAV12@ABV12@@Z */
/* ?copyfmt@ios_base@std@@QEAAAEAV12@AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_copyfmt, 8)
ios_base* __thiscall ios_base_copyfmt(ios_base *this, const ios_base *rhs)
{
    TRACE("(%p %p)\n", this, rhs);

    if(this != rhs) {
        IOS_BASE_iosarray *arr_cur;
        IOS_BASE_fnarray *event_cur;

        ios_base_Tidy(this);

        for(arr_cur=rhs->arr; arr_cur; arr_cur=arr_cur->next) {
            if(arr_cur->long_val)
                *ios_base_iword(this, arr_cur->index) = arr_cur->long_val;
            if(arr_cur->ptr_val)
                *ios_base_pword(this, arr_cur->index) = arr_cur->ptr_val;
        }
        this->stdstr = rhs->stdstr;
        this->fmtfl = rhs->fmtfl;
        this->prec = rhs->prec;
        this->wide = rhs->wide;
        locale_operator_assign(IOS_LOCALE(this), IOS_LOCALE(rhs));

        for(event_cur=rhs->calls; event_cur; event_cur=event_cur->next)
            ios_base_register_callback(this, event_cur->event_handler, event_cur->index);

        ios_base_Callfns(this, EVENT_copyfmt_event);
        ios_base_exceptions_set(this, rhs->except);
    }

    return this;
}

/* ??4ios_base@std@@QAEAAV01@ABV01@@Z */
/* ??4ios_base@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_assign, 8)
ios_base* __thiscall ios_base_assign(ios_base *this, const ios_base *right)
{
    TRACE("(%p %p)\n", this, right);

    if(this != right) {
        this->state = right->state;
        ios_base_copyfmt(this, right);
    }

    return this;
}

/* ?fail@ios_base@std@@QBE_NXZ */
/* ?fail@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_fail, 4)
bool __thiscall ios_base_fail(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return (this->state & (IOSTATE_failbit|IOSTATE_badbit)) != 0;
}

/* ??7ios_base@std@@QBE_NXZ */
/* ??7ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_op_succ, 4)
bool __thiscall ios_base_op_succ(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return ios_base_fail(this);
}

/* ??Bios_base@std@@QBEPAXXZ */
/* ??Bios_base@std@@QEBAPEAXXZ */
DEFINE_THISCALL_WRAPPER(ios_base_op_fail, 4)
void* __thiscall ios_base_op_fail(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return ios_base_fail(this) ? NULL : (void*)this;
}

/* ??Bios_base@std@@QBA_NXZ */
/* ??Bios_base@std@@QBE_NXZ */
/* ??Bios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_operator_bool, 4)
bool __thiscall ios_base_operator_bool(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return (this->state & (IOSTATE_failbit|IOSTATE_badbit)) == 0;
}

/* ?_Addstd@ios_base@std@@SAXPAV12@@Z */
/* ?_Addstd@ios_base@std@@SAXPEAV12@@Z */
void __cdecl ios_base_Addstd(ios_base *add)
{
    FIXME("(%p) stub\n", add);
}

/* ?_Index_func@ios_base@std@@CAAAHXZ */
/* ?_Index_func@ios_base@std@@CAAEAHXZ */
int* __cdecl ios_base_Index_func(void)
{
    TRACE("\n");
    return &ios_base_Index;
}

/* ?_Init@ios_base@std@@IAEXXZ */
/* ?_Init@ios_base@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(ios_base__Init, 4)
void __thiscall ios_base__Init(ios_base *this)
{
    TRACE("(%p)\n", this);

    this->stdstr = 0;
    this->state = this->except = IOSTATE_goodbit;
    this->fmtfl = FMTFLAG_skipws | FMTFLAG_dec;
    this->prec = 6;
    this->wide = 0;
    this->arr = NULL;
    this->calls = NULL;
#if _MSVCP_VER >= 70
    this->loc = operator_new(sizeof(locale));
#endif
    locale_ctor(IOS_LOCALE(this));
}

/* ?_Sync_func@ios_base@std@@CAAA_NXZ */
/* ?_Sync_func@ios_base@std@@CAAEA_NXZ */
bool* __cdecl ios_base_Sync_func(void)
{
    TRACE("\n");
    return &ios_base_Sync;
}

/* ?bad@ios_base@std@@QBE_NXZ */
/* ?bad@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_bad, 4)
bool __thiscall ios_base_bad(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return (this->state & IOSTATE_badbit) != 0;
}

/* ?eof@ios_base@std@@QBE_NXZ */
/* ?eof@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_eof, 4)
bool __thiscall ios_base_eof(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return (this->state & IOSTATE_eofbit) != 0;
}

/* ?flags@ios_base@std@@QAEHH@Z */
/* ?flags@ios_base@std@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_flags_set, 8)
IOSB_fmtflags __thiscall ios_base_flags_set(ios_base *this, IOSB_fmtflags flags)
{
    IOSB_fmtflags ret = this->fmtfl;

    TRACE("(%p %x)\n", this, flags);

    this->fmtfl = flags & FMTFLAG_mask;
    return ret;
}

/* ?flags@ios_base@std@@QBEHXZ */
/* ?flags@ios_base@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_base_flags_get, 4)
IOSB_fmtflags __thiscall ios_base_flags_get(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return this->fmtfl;
}

/* ?getloc@ios_base@std@@QBE?AVlocale@2@XZ */
/* ?getloc@ios_base@std@@QEBA?AVlocale@2@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_getloc, 8)
locale* __thiscall ios_base_getloc(const ios_base *this, locale *ret)
{
    TRACE("(%p)\n", this);
    return locale_copy_ctor(ret, IOS_LOCALE(this));
}

/* ?good@ios_base@std@@QBE_NXZ */
/* ?good@ios_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(ios_base_good, 4)
bool __thiscall ios_base_good(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return this->state == IOSTATE_goodbit;
}

/* ?imbue@ios_base@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?imbue@ios_base@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_imbue, 12)
locale* __thiscall ios_base_imbue(ios_base *this, locale *ret, const locale *loc)
{
    TRACE("(%p %p)\n", this, loc);
    *ret = *IOS_LOCALE(this);
    locale_copy_ctor(IOS_LOCALE(this), loc);
    return ret;
}

/* ?precision@ios_base@std@@QAEHH@Z */
/* ?precision@ios_base@std@@QEAA_J_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(ios_base_precision_set, 12)
#else
DEFINE_THISCALL_WRAPPER(ios_base_precision_set, 8)
#endif
streamsize __thiscall ios_base_precision_set(ios_base *this, streamsize precision)
{
    streamsize ret = this->prec;

    TRACE("(%p %s)\n", this, wine_dbgstr_longlong(precision));

    this->prec = precision;
    return ret;
}

/* ?precision@ios_base@std@@QBEHXZ */
/* ?precision@ios_base@std@@QEBA_JXZ */
DEFINE_THISCALL_WRAPPER(ios_base_precision_get, 4)
streamsize __thiscall ios_base_precision_get(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return this->prec;
}

/* ?rdstate@ios_base@std@@QBEHXZ */
/* ?rdstate@ios_base@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(ios_base_rdstate, 4)
IOSB_iostate __thiscall ios_base_rdstate(const ios_base *this)
{
    TRACE("(%p)\n", this);
    return this->state;
}

/* ?setf@ios_base@std@@QAEHHH@Z */
/* ?setf@ios_base@std@@QEAAHHH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_setf_mask, 12)
IOSB_fmtflags __thiscall ios_base_setf_mask(ios_base *this, IOSB_fmtflags flags, IOSB_fmtflags mask)
{
    IOSB_fmtflags ret = this->fmtfl;

    TRACE("(%p %x %x)\n", this, flags, mask);

    this->fmtfl = (this->fmtfl & (~mask)) | (flags & mask & FMTFLAG_mask);
    return ret;
}

/* ?setf@ios_base@std@@QAEHH@Z */
/* ?setf@ios_base@std@@QEAAHH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_setf, 8)
IOSB_fmtflags __thiscall ios_base_setf(ios_base *this, IOSB_fmtflags flags)
{
    IOSB_fmtflags ret = this->fmtfl;

    TRACE("(%p %x)\n", this, flags);

    this->fmtfl |= flags & FMTFLAG_mask;
    return ret;
}

/* ?setstate@ios_base@std@@QAEXH_N@Z */
/* ?setstate@ios_base@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(ios_base_setstate_reraise, 12)
void __thiscall ios_base_setstate_reraise(ios_base *this, IOSB_iostate state, bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);

    if(state != IOSTATE_goodbit)
        ios_base_clear_reraise(this, this->state | state, reraise);
}

/* ?setstate@ios_base@std@@QAEXH@Z */
/* ?setstate@ios_base@std@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_setstate, 8)
void __thiscall ios_base_setstate(ios_base *this, IOSB_iostate state)
{
    ios_base_setstate_reraise(this, state, FALSE);
}

/* ?setstate@ios_base@std@@QAEXI@Z */
/* ?setstate@ios_base@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(ios_base_setstate_unsigned, 8)
void __thiscall ios_base_setstate_unsigned(ios_base *this, unsigned int state)
{
    ios_base_setstate_reraise(this, (IOSB_iostate)state, FALSE);
}

/* ?sync_with_stdio@ios_base@std@@SA_N_N@Z */
bool __cdecl ios_base_sync_with_stdio(bool sync)
{
    _Lockit lock;
    bool ret;

    TRACE("(%x)\n", sync);

    _Lockit_ctor_locktype(&lock, _LOCK_STREAM);
    ret = ios_base_Sync;
    ios_base_Sync = sync;
    _Lockit_dtor(&lock);
    return ret;
}

/* ?unsetf@ios_base@std@@QAEXH@Z */
/* ?unsetf@ios_base@std@@QEAAXH@Z */
DEFINE_THISCALL_WRAPPER(ios_base_unsetf, 8)
void __thiscall ios_base_unsetf(ios_base *this, IOSB_fmtflags flags)
{
    TRACE("(%p %x)\n", this, flags);
    this->fmtfl &= ~flags;
}

/* ?width@ios_base@std@@QAEHH@Z */
/* ?width@ios_base@std@@QEAA_J_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(ios_base_width_set, 12)
#else
DEFINE_THISCALL_WRAPPER(ios_base_width_set, 8)
#endif
streamsize __thiscall ios_base_width_set(ios_base *this, streamsize width)
{
    streamsize ret = this->wide;

    TRACE("(%p %s)\n", this, wine_dbgstr_longlong(width));

    this->wide = width;
    return ret;
}

/* ?width@ios_base@std@@QBEHXZ */
/* ?width@ios_base@std@@QEBA_JXZ */
DEFINE_THISCALL_WRAPPER(ios_base_width_get, 4)
streamsize __thiscall ios_base_width_get(ios_base *this)
{
    TRACE("(%p)\n", this);
    return this->wide;
}

/* ?xalloc@ios_base@std@@SAHXZ */
int __cdecl ios_base_xalloc(void)
{
    _Lockit lock;
    int ret;

    TRACE("\n");

    _Lockit_ctor_locktype(&lock, _LOCK_STREAM);
    ret = ios_base_Index++;
    _Lockit_dtor(&lock);
    return ret;
}

/* ?swap@ios_base@std@@QAEXAAV12@@Z */
/* ?swap@ios_base@std@@QEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_swap, 8)
void __thiscall ios_base_swap(ios_base *this, ios_base *r)
{
    ios_base tmp;

    TRACE("(%p %p)\n", this, r);

    if(this == r)
        return;

    tmp = *this;
    *this = *r;
    this->vtable = tmp.vtable;
    tmp.vtable = r->vtable;
    *r = tmp;
}

DEFINE_THISCALL_WRAPPER(basic_ios__Add_vtordisp1, 4)
void __thiscall basic_ios__Add_vtordisp1(void *this)
{
    WARN("should not be called (%p)\n", this);
}

DEFINE_THISCALL_WRAPPER(basic_ios__Add_vtordisp2, 4)
void __thiscall basic_ios__Add_vtordisp2(void *this)
{
    WARN("should not be called (%p)\n", this);
}

/* ??0?$basic_ios@DU?$char_traits@D@std@@@std@@IAE@XZ */
/* ??0?$basic_ios@DU?$char_traits@D@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_ctor, 4)
basic_ios_char* __thiscall basic_ios_char_ctor(basic_ios_char *this)
{
    TRACE("(%p)\n", this);

    ios_base_ctor(&this->base);
    this->base.vtable = &basic_ios_char_vtable;
    return this;
}

/* ?init@?$basic_ios@DU?$char_traits@D@std@@@std@@IAEXPAV?$basic_streambuf@DU?$char_traits@D@std@@@2@_N@Z */
/* ?init@?$basic_ios@DU?$char_traits@D@std@@@std@@IEAAXPEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_init, 12)
void __thiscall basic_ios_char_init(basic_ios_char *this, basic_streambuf_char *streambuf, bool isstd)
{
    TRACE("(%p %p %x)\n", this, streambuf, isstd);
    ios_base__Init(&this->base);
    this->strbuf = streambuf;
    this->stream = NULL;
    this->fillch = ' ';

    if(!streambuf)
        ios_base_setstate(&this->base, IOSTATE_badbit);

    if(isstd)
        FIXME("standard streams not handled yet\n");
}

/* ??0?$basic_ios@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
/* ??0?$basic_ios@DU?$char_traits@D@std@@@std@@QEAA@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_ctor_streambuf, 8)
basic_ios_char* __thiscall basic_ios_char_ctor_streambuf(basic_ios_char *this, basic_streambuf_char *strbuf)
{
    TRACE("(%p %p)\n", this, strbuf);

    basic_ios_char_ctor(this);
    basic_ios_char_init(this, strbuf, FALSE);
    return this;
}

/* ??1?$basic_ios@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_ios@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_dtor, 4)
void __thiscall basic_ios_char_dtor(basic_ios_char *this)
{
    TRACE("(%p)\n", this);
    ios_base_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(basic_ios_char_vector_dtor, 8)
basic_ios_char* __thiscall basic_ios_char_vector_dtor(basic_ios_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ios_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ios_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?clear@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEXH_N@Z */
/* ?clear@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_clear_reraise, 12)
void __thiscall basic_ios_char_clear_reraise(basic_ios_char *this, IOSB_iostate state, bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);
    ios_base_clear_reraise(&this->base, state | (this->strbuf ? IOSTATE_goodbit : IOSTATE_badbit), reraise);
}

/* ?clear@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEXI@Z */
/* ?clear@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_clear, 8)
void __thiscall basic_ios_char_clear(basic_ios_char *this, unsigned int state)
{
    basic_ios_char_clear_reraise(this, (IOSB_iostate)state, FALSE);
}

/* ?copyfmt@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEAAV12@ABV12@@Z */
/* ?copyfmt@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAAEAV12@AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_copyfmt, 8)
basic_ios_char* __thiscall basic_ios_char_copyfmt(basic_ios_char *this, basic_ios_char *copy)
{
    TRACE("(%p %p)\n", this, copy);
    if(this == copy)
        return this;

    this->stream = copy->stream;
    this->fillch = copy->fillch;
    ios_base_copyfmt(&this->base, &copy->base);
    return this;
}

/* ?fill@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEDD@Z */
/* ?fill@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAADD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_fill_set, 8)
char __thiscall basic_ios_char_fill_set(basic_ios_char *this, char fill)
{
    char ret = this->fillch;

    TRACE("(%p %c)\n", this, fill);

    this->fillch = fill;
    return ret;
}

/* ?fill@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEDXZ */
/* ?fill@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBADXZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_fill_get, 4)
char __thiscall basic_ios_char_fill_get(basic_ios_char *this)
{
    TRACE("(%p)\n", this);
    return this->fillch;
}

/* ?imbue@?$basic_ios@DU?$char_traits@D@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?imbue@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_imbue, 12)
locale *__thiscall basic_ios_char_imbue(basic_ios_char *this, locale *ret, const locale *loc)
{
    TRACE("(%p %p %p)\n", this, ret, loc);

    if(this->strbuf) {
        basic_streambuf_char_pubimbue(this->strbuf, ret, loc);
        locale_dtor(ret);
    }

    return ios_base_imbue(&this->base, ret, loc);
}

/* ?narrow@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEDDD@Z */
/* ?narrow@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBADDD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_narrow, 12)
char __thiscall basic_ios_char_narrow(basic_ios_char *this, char ch, char def)
{
    TRACE("(%p %c %c)\n", this, ch, def);
    return ctype_char_narrow_ch(ctype_char_use_facet(IOS_LOCALE(&this->base)), ch, def);
}

/* ?rdbuf@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEPAV?$basic_streambuf@DU?$char_traits@D@std@@@2@PAV32@@Z */
/* ?rdbuf@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAPEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@PEAV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_rdbuf_set, 8)
basic_streambuf_char* __thiscall basic_ios_char_rdbuf_set(basic_ios_char *this, basic_streambuf_char *streambuf)
{
    basic_streambuf_char *ret = this->strbuf;

    TRACE("(%p %p)\n", this, streambuf);

    this->strbuf = streambuf;
    basic_ios_char_clear(this, IOSTATE_goodbit);
    return ret;
}

/* ?rdbuf@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_streambuf@DU?$char_traits@D@std@@@2@XZ */
/* ?rdbuf@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_rdbuf_get, 4)
basic_streambuf_char* __thiscall basic_ios_char_rdbuf_get(const basic_ios_char *this)
{
    TRACE("(%p)\n", this);
    return this->strbuf;
}

/* ?setstate@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEXH_N@Z */
/* ?setstate@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_setstate_reraise, 12)
void __thiscall basic_ios_char_setstate_reraise(basic_ios_char *this, IOSB_iostate state, bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);

    if(state != IOSTATE_goodbit)
        basic_ios_char_clear_reraise(this, this->base.state | state, reraise);
}

/* ?setstate@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEXI@Z */
/* ?setstate@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_setstate, 8)
void __thiscall basic_ios_char_setstate(basic_ios_char *this, unsigned int state)
{
    basic_ios_char_setstate_reraise(this, (IOSB_iostate)state, FALSE);
}

/* ?tie@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEPAV?$basic_ostream@DU?$char_traits@D@std@@@2@PAV32@@Z */
/* ?tie@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAPEAV?$basic_ostream@DU?$char_traits@D@std@@@2@PEAV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_tie_set, 8)
basic_ostream_char* __thiscall basic_ios_char_tie_set(basic_ios_char *this, basic_ostream_char *ostream)
{
    basic_ostream_char *ret = this->stream;

    TRACE("(%p %p)\n", this, ostream);

    this->stream = ostream;
    return ret;
}

/* ?tie@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_ostream@DU?$char_traits@D@std@@@2@XZ */
/* ?tie@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_ostream@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_char_tie_get, 4)
basic_ostream_char* __thiscall basic_ios_char_tie_get(const basic_ios_char *this)
{
    TRACE("(%p)\n", this);
    return this->stream;
}

/* ?widen@?$basic_ios@DU?$char_traits@D@std@@@std@@QBEDD@Z */
/* ?widen@?$basic_ios@DU?$char_traits@D@std@@@std@@QEBADD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_widen, 8)
char __thiscall basic_ios_char_widen(basic_ios_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return ctype_char_widen_ch(ctype_char_use_facet(IOS_LOCALE(&this->base)), ch);
}

/* ?swap@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_ios@DU?$char_traits@D@std@@@std@@QEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_char_swap, 8)
void __thiscall basic_ios_char_swap(basic_ios_char *this, basic_ios_char *r)
{
    void *swap_ptr;

    TRACE("(%p %p)\n", this, r);

    if(this == r)
        return;

    ios_base_swap(&this->base, &r->base);
    swap_ptr = this->stream;
    this->stream = r->stream;
    r->stream = swap_ptr;
    this->fillch ^= r->fillch;
    r->fillch ^= this->fillch;
    this->fillch ^= r->fillch;
}

/* ??0?$basic_ios@_WU?$char_traits@_W@std@@@std@@IAE@XZ */
/* ??0?$basic_ios@_WU?$char_traits@_W@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_ctor, 4)
basic_ios_wchar* __thiscall basic_ios_wchar_ctor(basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);

    ios_base_ctor(&this->base);
    this->base.vtable = &basic_ios_wchar_vtable;
    return this;
}

/* ??0?$basic_ios@GU?$char_traits@G@std@@@std@@IAE@XZ */
/* ??0?$basic_ios@GU?$char_traits@G@std@@@std@@IEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_short_ctor, 4)
basic_ios_wchar* __thiscall basic_ios_short_ctor(basic_ios_wchar *this)
{
    basic_ios_wchar_ctor(this);
    this->base.vtable = &basic_ios_short_vtable;
    return this;
}

/* ?init@?$basic_ios@_WU?$char_traits@_W@std@@@std@@IAEXPAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@_N@Z */
/* ?init@?$basic_ios@_WU?$char_traits@_W@std@@@std@@IEAAXPEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@_N@Z */
/* ?init@?$basic_ios@GU?$char_traits@G@std@@@std@@IAEXPAV?$basic_streambuf@GU?$char_traits@G@std@@@2@_N@Z */
/* ?init@?$basic_ios@GU?$char_traits@G@std@@@std@@IEAAXPEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_init, 12)
void __thiscall basic_ios_wchar_init(basic_ios_wchar *this, basic_streambuf_wchar *streambuf, bool isstd)
{
    TRACE("(%p %p %x)\n", this, streambuf, isstd);
    ios_base__Init(&this->base);
    this->strbuf = streambuf;
    this->stream = NULL;
    this->fillch = ' ';

    if(!streambuf)
        ios_base_setstate(&this->base, IOSTATE_badbit);

    if(isstd)
        FIXME("standard streams not handled yet\n");
}

/* ??0?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAE@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
/* ??0?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAA@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_ctor_streambuf, 8)
basic_ios_wchar* __thiscall basic_ios_wchar_ctor_streambuf(basic_ios_wchar *this, basic_streambuf_wchar *strbuf)
{
    TRACE("(%p %p)\n", this, strbuf);

    basic_ios_wchar_ctor(this);
    basic_ios_wchar_init(this, strbuf, FALSE);
    return this;
}

/* ??0?$basic_ios@GU?$char_traits@G@std@@@std@@QAE@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
/* ??0?$basic_ios@GU?$char_traits@G@std@@@std@@QEAA@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_short_ctor_streambuf, 8)
basic_ios_wchar* __thiscall basic_ios_short_ctor_streambuf(basic_ios_wchar *this, basic_streambuf_wchar *strbuf)
{
    basic_ios_wchar_ctor_streambuf(this, strbuf);
    this->base.vtable = &basic_ios_short_vtable;
    return this;
}

/* ??1?$basic_ios@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_ios@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
/* ??1?$basic_ios@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_ios@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_dtor, 4)
void __thiscall basic_ios_wchar_dtor(basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);
    ios_base_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(basic_ios_wchar_vector_dtor, 8)
basic_ios_wchar* __thiscall basic_ios_wchar_vector_dtor(basic_ios_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ios_wchar_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ios_wchar_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?clear@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEXH_N@Z */
/* ?clear@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAXH_N@Z */
/* ?clear@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXH_N@Z */
/* ?clear@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_clear_reraise, 12)
void __thiscall basic_ios_wchar_clear_reraise(basic_ios_wchar *this, IOSB_iostate state, bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);
    ios_base_clear_reraise(&this->base, state | (this->strbuf ? IOSTATE_goodbit : IOSTATE_badbit), reraise);
}

/* ?clear@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEXI@Z */
/* ?clear@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAXI@Z */
/* ?clear@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXI@Z */
/* ?clear@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_clear, 8)
void __thiscall basic_ios_wchar_clear(basic_ios_wchar *this, unsigned int state)
{
    basic_ios_wchar_clear_reraise(this, (IOSB_iostate)state, FALSE);
}

/* ?copyfmt@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEAAV12@ABV12@@Z */
/* ?copyfmt@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@AEBV12@@Z */
/* ?copyfmt@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEAAV12@ABV12@@Z */
/* ?copyfmt@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAAEAV12@AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_copyfmt, 8)
basic_ios_wchar* __thiscall basic_ios_wchar_copyfmt(basic_ios_wchar *this, basic_ios_wchar *copy)
{
    TRACE("(%p %p)\n", this, copy);
    if(this == copy)
        return this;

    this->stream = copy->stream;
    this->fillch = copy->fillch;
    ios_base_copyfmt(&this->base, &copy->base);
    return this;
}

/* ?fill@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAE_W_W@Z */
/* ?fill@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAA_W_W@Z */
/* ?fill@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEGG@Z */
/* ?fill@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAGG@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_fill_set, 8)
wchar_t __thiscall basic_ios_wchar_fill_set(basic_ios_wchar *this, wchar_t fill)
{
    wchar_t ret = this->fillch;

    TRACE("(%p %c)\n", this, fill);

    this->fillch = fill;
    return ret;
}

/* ?fill@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QBE_WXZ */
/* ?fill@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEBA_WXZ */
/* ?fill@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEGXZ */
/* ?fill@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBAGXZ */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_fill_get, 4)
wchar_t __thiscall basic_ios_wchar_fill_get(basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->fillch;
}

/* ?imbue@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?imbue@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
/* ?imbue@?$basic_ios@GU?$char_traits@G@std@@@std@@QAE?AVlocale@2@ABV32@@Z */
/* ?imbue@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAA?AVlocale@2@AEBV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_imbue, 12)
locale *__thiscall basic_ios_wchar_imbue(basic_ios_wchar *this, locale *ret, const locale *loc)
{
    TRACE("(%p %p %p)\n", this, ret, loc);

    if(this->strbuf) {
        basic_streambuf_wchar_pubimbue(this->strbuf, ret, loc);
        locale_dtor(ret);
    }

    return ios_base_imbue(&this->base, ret, loc);
}

/* ?narrow@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QBED_WD@Z */
/* ?narrow@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEBAD_WD@Z */
/* ?narrow@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEDGD@Z */
/* ?narrow@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBADGD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_narrow, 12)
char __thiscall basic_ios_wchar_narrow(basic_ios_wchar *this, wchar_t ch, char def)
{
    TRACE("(%p %c %c)\n", this, ch, def);
    return ctype_wchar_narrow_ch(ctype_wchar_use_facet(IOS_LOCALE(&this->base)), ch, def);
}

/* ?rdbuf@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEPAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@PAV32@@Z */
/* ?rdbuf@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAPEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@PEAV32@@Z */
/* ?rdbuf@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEPAV?$basic_streambuf@GU?$char_traits@G@std@@@2@PAV32@@Z */
/* ?rdbuf@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAPEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@PEAV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_rdbuf_set, 8)
basic_streambuf_wchar* __thiscall basic_ios_wchar_rdbuf_set(basic_ios_wchar *this, basic_streambuf_wchar *streambuf)
{
    basic_streambuf_wchar *ret = this->strbuf;

    TRACE("(%p %p)\n", this, streambuf);

    this->strbuf = streambuf;
    basic_ios_wchar_clear(this, IOSTATE_goodbit);
    return ret;
}

/* ?rdbuf@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QBEPAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@XZ */
/* ?rdbuf@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEBAPEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@XZ */
/* ?rdbuf@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEPAV?$basic_streambuf@GU?$char_traits@G@std@@@2@XZ */
/* ?rdbuf@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBAPEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_rdbuf_get, 4)
basic_streambuf_wchar* __thiscall basic_ios_wchar_rdbuf_get(const basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->strbuf;
}

/* ?setstate@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEXH_N@Z */
/* ?setstate@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAXH_N@Z */
/* ?setstate@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXH_N@Z */
/* ?setstate@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAXH_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_setstate_reraise, 12)
void __thiscall basic_ios_wchar_setstate_reraise(basic_ios_wchar *this, IOSB_iostate state, bool reraise)
{
    TRACE("(%p %x %x)\n", this, state, reraise);

    if(state != IOSTATE_goodbit)
        basic_ios_wchar_clear_reraise(this, this->base.state | state, reraise);
}

/* ?setstate@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEXI@Z */
/* ?setstate@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAXI@Z */
/* ?setstate@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXI@Z */
/* ?setstate@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAXI@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_setstate, 8)
void __thiscall basic_ios_wchar_setstate(basic_ios_wchar *this, IOSB_iostate state)
{
    basic_ios_wchar_setstate_reraise(this, state, FALSE);
}

/* ?tie@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEPAV?$basic_ostream@_WU?$char_traits@_W@std@@@2@PAV32@@Z */
/* ?tie@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAPEAV?$basic_ostream@_WU?$char_traits@_W@std@@@2@PEAV32@@Z */
/* ?tie@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEPAV?$basic_ostream@GU?$char_traits@G@std@@@2@PAV32@@Z */
/* ?tie@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAPEAV?$basic_ostream@GU?$char_traits@G@std@@@2@PEAV32@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_tie_set, 8)
basic_ostream_wchar* __thiscall basic_ios_wchar_tie_set(basic_ios_wchar *this, basic_ostream_wchar *ostream)
{
    basic_ostream_wchar *ret = this->stream;

    TRACE("(%p %p)\n", this, ostream);

    this->stream = ostream;
    return ret;
}

/* ?tie@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QBEPAV?$basic_ostream@_WU?$char_traits@_W@std@@@2@XZ */
/* ?tie@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEBAPEAV?$basic_ostream@_WU?$char_traits@_W@std@@@2@XZ */
/* ?tie@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEPAV?$basic_ostream@GU?$char_traits@G@std@@@2@XZ */
/* ?tie@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBAPEAV?$basic_ostream@GU?$char_traits@G@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_tie_get, 4)
basic_ostream_wchar* __thiscall basic_ios_wchar_tie_get(const basic_ios_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->stream;
}

/* ?widen@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QBE_WD@Z */
/* ?widen@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEBA_WD@Z */
/* ?widen@?$basic_ios@GU?$char_traits@G@std@@@std@@QBEGD@Z */
/* ?widen@?$basic_ios@GU?$char_traits@G@std@@@std@@QEBAGD@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_widen, 8)
wchar_t __thiscall basic_ios_wchar_widen(basic_ios_wchar *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return ctype_wchar_widen_ch(ctype_wchar_use_facet(IOS_LOCALE(&this->base)), ch);
}

/* ?swap@?$basic_ios@GU?$char_traits@G@std@@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_ios@GU?$char_traits@G@std@@@std@@QEAAXAEAV12@@Z */
/* ?swap@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_ios@_WU?$char_traits@_W@std@@@std@@QEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_ios_wchar_swap, 8)
void __thiscall basic_ios_wchar_swap(basic_ios_wchar *this, basic_ios_wchar *r)
{
    void *swap_ptr;

    TRACE("(%p %p)\n", this, r);

    if(this == r)
        return;

    ios_base_swap(&this->base, &r->base);
    swap_ptr = this->stream;
    this->stream = r->stream;
    r->stream = swap_ptr;
    this->fillch ^= r->fillch;
    r->fillch ^= this->fillch;
    this->fillch ^= r->fillch;
}

/* Caution: basic_ostream uses virtual inheritance.
 * All constructors have additional parameter that says if base class should be initialized.
 * Base class needs to be accessed using vbtable.
 */
static inline basic_ios_char* basic_ostream_char_get_basic_ios(basic_ostream_char *this)
{
    return (basic_ios_char*)((char*)this+this->vbtable[1]);
}

static inline basic_ios_char* basic_ostream_char_to_basic_ios(basic_ostream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_ostream_char_vbtable[1]);
}

static inline basic_ostream_char* basic_ostream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_ostream_char*)((char*)ptr-basic_ostream_char_vbtable[1]);
}

/* ??0?$basic_ostream@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N@Z */
/* ??0?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAA@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_ctor, 16)
basic_ostream_char* __thiscall basic_ostream_char_ctor(basic_ostream_char *this,
        basic_streambuf_char *strbuf, bool isstd, bool virt_init)
{
    basic_ios_char *base;

    TRACE("(%p %p %d %d)\n", this, strbuf, isstd, virt_init);

    if(virt_init) {
        this->vbtable = basic_ostream_char_vbtable;
        base = basic_ostream_char_get_basic_ios(this);
        INIT_BASIC_IOS_VTORDISP(base);
        basic_ios_char_ctor(base);
    }else {
        base = basic_ostream_char_get_basic_ios(this);
    }

    base->base.vtable = &basic_ostream_char_vtable;
    basic_ios_char_init(base, strbuf, isstd);
    return this;
}

/* ??0?$basic_ostream@DU?$char_traits@D@std@@@std@@QAE@W4_Uninitialized@1@_N@Z */
/* ??0?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAA@W4_Uninitialized@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_ctor_uninitialized, 16)
basic_ostream_char* __thiscall basic_ostream_char_ctor_uninitialized(basic_ostream_char *this,
        int uninitialized, bool addstd, bool virt_init)
{
    basic_ios_char *base;

    TRACE("(%p %d %x)\n", this, uninitialized, addstd);

    if(virt_init) {
        this->vbtable = basic_ostream_char_vbtable;
        base = basic_ostream_char_get_basic_ios(this);
        INIT_BASIC_IOS_VTORDISP(base);
        basic_ios_char_ctor(base);
    }else {
        base = basic_ostream_char_get_basic_ios(this);
    }

    base->base.vtable = &basic_ostream_char_vtable;
    if(addstd)
        ios_base_Addstd(&base->base);
    return this;
}

/* ??1?$basic_ostream@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_ostream@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_dtor, 4)
void __thiscall basic_ostream_char_dtor(basic_ios_char *base)
{
    basic_ostream_char *this = basic_ostream_char_from_basic_ios(base);

    /* don't destroy virtual base here */
    TRACE("(%p)\n", this);
}

/* ??_D?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_D?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_vbase_dtor, 4)
void __thiscall basic_ostream_char_vbase_dtor(basic_ostream_char *this)
{
    basic_ios_char *base = basic_ostream_char_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_ostream_char_dtor(base);
    basic_ios_char_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_ostream_char_vector_dtor, 8)
basic_ostream_char* __thiscall basic_ostream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_ostream_char *this = basic_ostream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ostream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ostream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?flush@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV12@XZ */
/* ?flush@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_flush, 4)
basic_ostream_char* __thiscall basic_ostream_char_flush(basic_ostream_char *this)
{
    /* this function is not matching C++ specification */
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(basic_ios_char_rdbuf_get(base) && ios_base_good(&base->base)
            && basic_streambuf_char_pubsync(basic_ios_char_rdbuf_get(base))==-1)
        basic_ios_char_setstate(base, IOSTATE_badbit);
    return this;
}

/* ?flush@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@1@AAV21@@Z */
/* ?flush@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@1@AEAV21@@Z */
basic_ostream_char* __cdecl flush_ostream_char(basic_ostream_char *ostream)
{
    return basic_ostream_char_flush(ostream);
}

/* ?_Osfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?_Osfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char__Osfx, 4)
void __thiscall basic_ostream_char__Osfx(basic_ostream_char *this)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(base->base.fmtfl & FMTFLAG_unitbuf)
        basic_ostream_char_flush(this);
}

/* ?osfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?osfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_osfx, 4)
void __thiscall basic_ostream_char_osfx(basic_ostream_char *this)
{
    TRACE("(%p)\n", this);
    basic_ostream_char__Osfx(this);
}

static BOOL basic_ostream_char_sentry_create(basic_ostream_char *ostr)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(ostr);

    if(basic_ios_char_rdbuf_get(base))
        call_basic_streambuf_char__Lock(base->strbuf);

    if(ios_base_good(&base->base) && base->stream)
        basic_ostream_char_flush(base->stream);

    return ios_base_good(&base->base);
}

static void basic_ostream_char_sentry_destroy(basic_ostream_char *ostr)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(ostr);

    if(ios_base_good(&base->base) && !__uncaught_exception())
        basic_ostream_char_osfx(ostr);

    if(basic_ios_char_rdbuf_get(base))
        call_basic_streambuf_char__Unlock(base->strbuf);
}

/* ?opfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAE_NXZ */
/* ?opfx@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_opfx, 4)
bool __thiscall basic_ostream_char_opfx(basic_ostream_char *this)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(ios_base_good(&base->base) && base->stream)
        basic_ostream_char_flush(base->stream);
    return ios_base_good(&base->base);
}

/* ?put@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV12@D@Z */
/* ?put@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@D@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_put, 8)
basic_ostream_char* __thiscall basic_ostream_char_put(basic_ostream_char *this, char ch)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p %c)\n", this, ch);

    if(!basic_ostream_char_sentry_create(this)
            || basic_streambuf_char_sputc(base->strbuf, ch)==EOF) {
        basic_ostream_char_sentry_destroy(this);
        basic_ios_char_setstate(base, IOSTATE_badbit);
        return this;
    }

    basic_ostream_char_sentry_destroy(this);
    return this;
}

/* ?seekp@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV12@JH@Z */
/* ?seekp@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@_JH@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_ostream_char_seekp, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_ostream_char_seekp, 12)
#endif
basic_ostream_char* __thiscall basic_ostream_char_seekp(basic_ostream_char *this, streamoff off, int way)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p %s %d)\n", this, wine_dbgstr_longlong(off), way);

    if(!ios_base_fail(&base->base)) {
        fpos_mbstatet seek;

        basic_streambuf_char_pubseekoff(basic_ios_char_rdbuf_get(base),
                &seek, off, way, OPENMODE_out);
        if(seek.off==-1 && seek.pos==0 && MBSTATET_TO_INT(&seek.state)==0)
            basic_ios_char_setstate(base, IOSTATE_failbit);
    }
    return this;
}

/* ?seekp@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z */
/* ?seekp@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_seekp_fpos, 28)
basic_ostream_char* __thiscall basic_ostream_char_seekp_fpos(basic_ostream_char *this, fpos_mbstatet pos)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p %s)\n", this, debugstr_fpos_mbstatet(&pos));

    if(!ios_base_fail(&base->base)) {
        fpos_mbstatet seek;

        basic_streambuf_char_pubseekpos(basic_ios_char_rdbuf_get(base),
                &seek, pos, OPENMODE_out);
        if(seek.off==-1 && seek.pos==0 && MBSTATET_TO_INT(&seek.state)==0)
            basic_ios_char_setstate(base, IOSTATE_failbit);
    }
    return this;
}

/* ?tellp@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@XZ */
/* ?tellp@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_tellp, 8)
fpos_mbstatet* __thiscall basic_ostream_char_tellp(basic_ostream_char *this, fpos_mbstatet *ret)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(!ios_base_fail(&base->base)) {
        basic_streambuf_char_pubseekoff(basic_ios_char_rdbuf_get(base),
                ret, 0, SEEKDIR_cur, OPENMODE_out);
    }else {
        ret->off = -1;
        ret->pos = 0;
        memset(&ret->state, 0, sizeof(ret->state));
    }
    return ret;
}

/* ?write@?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV12@PBDH@Z */
/* ?write@?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEBD_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_write, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_ostream_char_write, 12)
#endif
basic_ostream_char* __thiscall basic_ostream_char_write(basic_ostream_char *this, const char *str, streamsize count)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);

    TRACE("(%p %s %s)\n", this, debugstr_a(str), wine_dbgstr_longlong(count));

    if(!basic_ostream_char_sentry_create(this)
            || basic_streambuf_char_sputn(base->strbuf, str, count)!=count) {
        basic_ostream_char_sentry_destroy(this);
        basic_ios_char_setstate(base, IOSTATE_badbit);
        return this;
    }

    basic_ostream_char_sentry_destroy(this);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@F@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@F@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_short, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_short(basic_ostream_char *this, short val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_char_put_long(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base),
                (ios_base_flags_get(&base->base) & FMTFLAG_basefield & (FMTFLAG_oct | FMTFLAG_hex))
                ? (LONG)((unsigned short)val) : (LONG)val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@G@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@G@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_ushort, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_ushort(basic_ostream_char *this, unsigned short val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %u)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf =  strbuf;
        num_put_char_put_ulong(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@H@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@H@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@J@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_int, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_int(basic_ostream_char *this, int val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf =  strbuf;
        num_put_char_put_long(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@I@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@I@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@K@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@K@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_uint, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_uint(basic_ostream_char *this, unsigned int val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %u)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf =  strbuf;
        num_put_char_put_ulong(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@M@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@M@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_float, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_float(basic_ostream_char *this, float val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %f)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_char_put_double(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@N@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_double, 12)
basic_ostream_char* __thiscall basic_ostream_char_print_double(basic_ostream_char *this, double val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %lf)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_char_put_double(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@O@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@O@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_ldouble, 12)
basic_ostream_char* __thiscall basic_ostream_char_print_ldouble(basic_ostream_char *this, double val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %lf)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_char_put_ldouble(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_streambuf, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_streambuf(basic_ostream_char *this, basic_streambuf_char *val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_badbit;
    int c = '\n';

    TRACE("(%p %p)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        for(c = basic_streambuf_char_sgetc(val); c!=EOF;
                c = basic_streambuf_char_snextc(val)) {
            state = IOSTATE_goodbit;

            if(basic_streambuf_char_sputc(base->strbuf, c) == EOF) {
                state = IOSTATE_badbit;
                break;
            }
        }
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_char_sentry_destroy(this);

    ios_base_width_set(&base->base, 0);
    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@PBX@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@PEBX@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_ptr, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_ptr(basic_ostream_char *this, const void *val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_char_put_ptr(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@_J@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@_J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_int64, 12)
basic_ostream_char* __thiscall basic_ostream_char_print_int64(basic_ostream_char *this, __int64 val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_char_put_int64(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@_K@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@_K@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_uint64, 12)
basic_ostream_char* __thiscall basic_ostream_char_print_uint64(basic_ostream_char *this, unsigned __int64 val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_char_put_uint64(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@_N@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_bool, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_bool(basic_ostream_char *this, bool val)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %x)\n", this, val);

    if(basic_ostream_char_sentry_create(this)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_put *numput = num_put_char_use_facet(IOS_LOCALE(strbuf));
        ostreambuf_iterator_char dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_char_put_bool(numput, &dest, dest, &base->base, basic_ios_char_fill_get(base), val);
    }
    basic_ostream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ?ends@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@1@AAV21@@Z */
/* ?ends@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@1@AEAV21@@Z */
basic_ostream_char* __cdecl basic_ostream_char_ends(basic_ostream_char *ostr)
{
    TRACE("(%p)\n", ostr);

    basic_ostream_char_put(ostr, 0);
    return ostr;
}

/* ?endl@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@1@AAV21@@Z */
/* ?endl@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@1@AEAV21@@Z */
basic_ostream_char* __cdecl basic_ostream_char_endl(basic_ostream_char *ostr)
{
    TRACE("(%p)\n", ostr);

    basic_ostream_char_put(ostr, '\n');
    basic_ostream_char_flush(ostr);
    return ostr;
}

/* ??$?6DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?6DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
basic_ostream_char* __cdecl basic_ostream_char_print_bstr(basic_ostream_char *ostr, const basic_string_char *str)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", ostr, str);

    if(basic_ostream_char_sentry_create(ostr)) {
        size_t len = MSVCP_basic_string_char_length(str);
        streamsize pad = (base->base.wide>len ? base->base.wide-len : 0);

        if((base->base.fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_char_sputc(base->strbuf, base->fillch) == EOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        if(state == IOSTATE_goodbit) {
            if(basic_streambuf_char_sputn(base->strbuf, MSVCP_basic_string_char_c_str(str), len) != len)
                    state = IOSTATE_badbit;
        }

        if(state == IOSTATE_goodbit) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_char_sputc(base->strbuf, base->fillch) == EOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        base->base.wide = 0;
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_char_sentry_destroy(ostr);

    basic_ios_char_setstate(base, state);
    return ostr;
}

/* ??$?6U?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@C@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@C@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@D@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@D@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@E@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@E@Z */
basic_ostream_char* __cdecl basic_ostream_char_print_ch(basic_ostream_char *ostr, char ch)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", ostr, ch);

    if(basic_ostream_char_sentry_create(ostr)) {
        streamsize pad = (base->base.wide>1 ? base->base.wide-1 : 0);

        if((base->base.fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_char_sputc(base->strbuf, base->fillch) == EOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        if(state == IOSTATE_goodbit) {
            if(basic_streambuf_char_sputc(base->strbuf, ch) == EOF)
                state = IOSTATE_badbit;
        }

        if(state == IOSTATE_goodbit) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_char_sputc(base->strbuf, base->fillch) == EOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        base->base.wide = 0;
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_char_sentry_destroy(ostr);

    basic_ios_char_setstate(base, state);
    return ostr;
}

/* ??$?6U?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@PBC@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@PEBC@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@PBD@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@PEBD@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@PBE@Z */
/* ??$?6U?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@PEBE@Z */
basic_ostream_char* __cdecl basic_ostream_char_print_str(basic_ostream_char *ostr, const char *str)
{
    basic_ios_char *base = basic_ostream_char_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %s)\n", ostr, str);

    if(basic_ostream_char_sentry_create(ostr)) {
        size_t len = strlen(str);
        streamsize pad = (base->base.wide>len ? base->base.wide-len : 0);

        if((base->base.fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_char_sputc(base->strbuf, base->fillch) == EOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        if(state == IOSTATE_goodbit) {
            if(basic_streambuf_char_sputn(base->strbuf, str, len) != len)
                state = IOSTATE_badbit;
        }

        if(state == IOSTATE_goodbit) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_char_sputc(base->strbuf, base->fillch) == EOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        base->base.wide = 0;
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_char_sentry_destroy(ostr);

    basic_ios_char_setstate(base, state);
    return ostr;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@P6AAAV01@AAV01@@Z@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@P6AAEAV01@AEAV01@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_func, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_func(basic_ostream_char *this,
        basic_ostream_char* (__cdecl *pfunc)(basic_ostream_char*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(this);
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@P6AAAV?$basic_ios@DU?$char_traits@D@std@@@1@AAV21@@Z@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@P6AAEAV?$basic_ios@DU?$char_traits@D@std@@@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_func_basic_ios, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_func_basic_ios(basic_ostream_char *this,
        basic_ios_char* (__cdecl *pfunc)(basic_ios_char*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(basic_ostream_char_get_basic_ios(this));
    return this;
}

/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z */
/* ??6?$basic_ostream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@P6AAEAVios_base@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_print_func_ios_base, 8)
basic_ostream_char* __thiscall basic_ostream_char_print_func_ios_base(
        basic_ostream_char *this, ios_base* (__cdecl *pfunc)(ios_base*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(&basic_ostream_char_get_basic_ios(this)->base);
    return this;
}

/* ?swap@?$basic_ostream@DU?$char_traits@D@std@@@std@@IAEXAAV12@@Z */
/* ?swap@?$basic_ostream@DU?$char_traits@D@std@@@std@@IEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_char_swap, 8)
void __thiscall basic_ostream_char_swap(basic_ostream_char *this, basic_ostream_char *r)
{
    TRACE("(%p %p)\n", this, r);

    if(this == r)
        return;

    basic_ios_char_swap(basic_ostream_char_get_basic_ios(this),
            basic_ostream_char_get_basic_ios(r));
}

/* Caution: basic_ostream uses virtual inheritance. */
static inline basic_ios_wchar* basic_ostream_wchar_get_basic_ios(basic_ostream_wchar *this)
{
    return (basic_ios_wchar*)((char*)this+this->vbtable[1]);
}

static inline basic_ios_wchar* basic_ostream_wchar_to_basic_ios(basic_ostream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_ostream_wchar_vbtable[1]);
}

static inline basic_ostream_wchar* basic_ostream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_ostream_wchar*)((char*)ptr-basic_ostream_wchar_vbtable[1]);
}

/* ??0?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAE@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@_N@Z */
/* ??0?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAA@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_ctor, 16)
basic_ostream_wchar* __thiscall basic_ostream_wchar_ctor(basic_ostream_wchar *this,
        basic_streambuf_wchar *strbuf, bool isstd, bool virt_init)
{
    basic_ios_wchar *base;

    TRACE("(%p %p %d %d)\n", this, strbuf, isstd, virt_init);

    if(virt_init) {
        this->vbtable = basic_ostream_wchar_vbtable;
        base = basic_ostream_wchar_get_basic_ios(this);
        INIT_BASIC_IOS_VTORDISP(base);
        basic_ios_wchar_ctor(base);
    }else {
        base = basic_ostream_wchar_get_basic_ios(this);
    }

    base->base.vtable = &basic_ostream_wchar_vtable;
    basic_ios_wchar_init(base, strbuf, isstd);
    return this;
}

/* ??0?$basic_ostream@GU?$char_traits@G@std@@@std@@QAE@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@_N@Z */
/* ??0?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAA@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_ctor, 16)
basic_ostream_wchar* __thiscall basic_ostream_short_ctor(basic_ostream_wchar *this,
        basic_streambuf_wchar *strbuf, bool isstd, bool virt_init)
{
    basic_ostream_wchar_ctor(this, strbuf, isstd, virt_init);
    basic_ostream_wchar_get_basic_ios(this)->base.vtable = &basic_ostream_short_vtable;
    return this;
}

/* ??0?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAE@W4_Uninitialized@1@_N@Z */
/* ??0?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAA@W4_Uninitialized@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_ctor_uninitialized, 16)
basic_ostream_wchar* __thiscall basic_ostream_wchar_ctor_uninitialized(basic_ostream_wchar *this,
        int uninitialized, bool addstd, bool virt_init)
{
    basic_ios_wchar *base;

    TRACE("(%p %d %x)\n", this, uninitialized, addstd);

    if(virt_init) {
        this->vbtable = basic_ostream_wchar_vbtable;
        base = basic_ostream_wchar_get_basic_ios(this);
        INIT_BASIC_IOS_VTORDISP(base);
        basic_ios_wchar_ctor(base);
    }else {
        base = basic_ostream_wchar_get_basic_ios(this);
    }

    base->base.vtable = &basic_ostream_wchar_vtable;
    if(addstd)
        ios_base_Addstd(&base->base);
    return this;
}

/* ??0?$basic_ostream@GU?$char_traits@G@std@@@std@@QAE@W4_Uninitialized@1@_N@Z */
/* ??0?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAA@W4_Uninitialized@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_ctor_uninitialized, 16)
basic_ostream_wchar* __thiscall basic_ostream_short_ctor_uninitialized(basic_ostream_wchar *this,
        int uninitialized, bool addstd, bool virt_init)
{
    basic_ostream_wchar_ctor_uninitialized(this, uninitialized, addstd, virt_init);
    basic_ostream_wchar_get_basic_ios(this)->base.vtable = &basic_ostream_short_vtable;
    return this;
}

/* ??1?$basic_ostream@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_ostream@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
/* ??1?$basic_ostream@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_ostream@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_dtor, 4)
void __thiscall basic_ostream_wchar_dtor(basic_ios_wchar *base)
{
    basic_ostream_wchar *this = basic_ostream_wchar_from_basic_ios(base);

    /* don't destroy virtual base here */
    TRACE("(%p)\n", this);
}

/* ??_D?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ??_D?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ??_D?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_D?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_vbase_dtor, 4)
void __thiscall basic_ostream_wchar_vbase_dtor(basic_ostream_wchar *this)
{
    basic_ios_wchar *base = basic_ostream_wchar_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_ostream_wchar_dtor(base);
    basic_ios_wchar_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_vector_dtor, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_ostream_wchar *this = basic_ostream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ostream_wchar_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ostream_wchar_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?flush@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@XZ */
/* ?flush@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@XZ */
/* ?flush@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV12@XZ */
/* ?flush@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_flush, 4)
basic_ostream_wchar* __thiscall basic_ostream_wchar_flush(basic_ostream_wchar *this)
{
    /* this function is not matching C++ specification */
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(basic_ios_wchar_rdbuf_get(base) && ios_base_good(&base->base)
            && basic_streambuf_wchar_pubsync(basic_ios_wchar_rdbuf_get(base))==-1)
        basic_ios_wchar_setstate(base, IOSTATE_badbit);
    return this;
}

/* ?flush@std@@YAAAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@AAV21@@Z */
/* ?flush@std@@YAAEAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@AEAV21@@Z */
/* ?flush@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@1@AAV21@@Z */
/* ?flush@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@1@AEAV21@@Z */
basic_ostream_wchar* __cdecl flush_ostream_wchar(basic_ostream_wchar *ostream)
{
    return basic_ostream_wchar_flush(ostream);
}

/* ?_Osfx@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?_Osfx@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ?_Osfx@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?_Osfx@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar__Osfx, 4)
void __thiscall basic_ostream_wchar__Osfx(basic_ostream_wchar *this)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(base->base.fmtfl & FMTFLAG_unitbuf)
        basic_ostream_wchar_flush(this);
}

/* ?osfx@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?osfx@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ?osfx@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?osfx@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_osfx, 4)
void __thiscall basic_ostream_wchar_osfx(basic_ostream_wchar *this)
{
    TRACE("(%p)\n", this);
    basic_ostream_wchar__Osfx(this);
}

static BOOL basic_ostream_wchar_sentry_create(basic_ostream_wchar *ostr)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(ostr);

    if(basic_ios_wchar_rdbuf_get(base))
        basic_streambuf_wchar__Lock(base->strbuf);

    if(ios_base_good(&base->base) && base->stream)
        basic_ostream_wchar_flush(base->stream);

    return ios_base_good(&base->base);
}

static void basic_ostream_wchar_sentry_destroy(basic_ostream_wchar *ostr)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(ostr);

    if(ios_base_good(&base->base) && !__uncaught_exception())
        basic_ostream_wchar_osfx(ostr);

    if(basic_ios_wchar_rdbuf_get(base))
        basic_streambuf_wchar__Unlock(base->strbuf);
}

/* ?opfx@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAE_NXZ */
/* ?opfx@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAA_NXZ */
/* ?opfx@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAE_NXZ */
/* ?opfx@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_opfx, 4)
bool __thiscall basic_ostream_wchar_opfx(basic_ostream_wchar *this)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(ios_base_good(&base->base) && base->stream)
        basic_ostream_wchar_flush(base->stream);
    return ios_base_good(&base->base);
}

/* ?put@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@_W@Z */
/* ?put@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@_W@Z */
/* ?put@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV12@G@Z */
/* ?put@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@G@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_put, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_put(basic_ostream_wchar *this, wchar_t ch)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p %c)\n", this, ch);

    if(!basic_ostream_wchar_sentry_create(this)
            || basic_streambuf_wchar_sputc(base->strbuf, ch)==WEOF) {
        basic_ostream_wchar_sentry_destroy(this);
        basic_ios_wchar_setstate(base, IOSTATE_badbit);
        return this;
    }

    basic_ostream_wchar_sentry_destroy(this);
    return this;
}

/* ?seekp@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@JH@Z */
/* ?seekp@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@_JH@Z */
/* ?seekp@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV12@JH@Z */
/* ?seekp@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@_JH@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_seekp, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_seekp, 12)
#endif
basic_ostream_wchar* __thiscall basic_ostream_wchar_seekp(basic_ostream_wchar *this, streamoff off, int way)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p %s %d)\n", this, wine_dbgstr_longlong(off), way);

    if(!ios_base_fail(&base->base)) {
        fpos_mbstatet seek;

        basic_streambuf_wchar_pubseekoff(basic_ios_wchar_rdbuf_get(base),
                &seek, off, way, OPENMODE_out);
        if(seek.off==-1 && seek.pos==0 && MBSTATET_TO_INT(&seek.state)==0)
            basic_ios_wchar_setstate(base, IOSTATE_failbit);
    }
    return this;
}

/* ?seekp@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z */
/* ?seekp@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z */
/* ?seekp@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z */
/* ?seekp@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_seekp_fpos, 28)
basic_ostream_wchar* __thiscall basic_ostream_wchar_seekp_fpos(basic_ostream_wchar *this, fpos_mbstatet pos)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p %s)\n", this, debugstr_fpos_mbstatet(&pos));

    if(!ios_base_fail(&base->base)) {
        fpos_mbstatet seek;

        basic_streambuf_wchar_pubseekpos(basic_ios_wchar_rdbuf_get(base),
                &seek, pos, OPENMODE_out);
        if(seek.off==-1 && seek.pos==0 && MBSTATET_TO_INT(&seek.state)==0)
            basic_ios_wchar_setstate(base, IOSTATE_failbit);
    }
    return this;
}

/* ?tellp@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAE?AV?$fpos@H@2@XZ */
/* ?tellp@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAA?AV?$fpos@H@2@XZ */
/* ?tellp@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@XZ */
/* ?tellp@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_tellp, 8)
fpos_mbstatet* __thiscall basic_ostream_wchar_tellp(basic_ostream_wchar *this, fpos_mbstatet *ret)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p)\n", this);

    if(!ios_base_fail(&base->base)) {
        basic_streambuf_wchar_pubseekoff(basic_ios_wchar_rdbuf_get(base),
                ret, 0, SEEKDIR_cur, OPENMODE_out);
    }else {
        ret->off = -1;
        ret->pos = 0;
        memset(&ret->state, 0, sizeof(ret->state));
    }
    return ret;
}

/* ?write@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PB_WH@Z */
/* ?write@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEB_W_J@Z */
/* ?write@?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV12@PBGH@Z */
/* ?write@?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@PEBG_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_write, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_write, 12)
#endif
basic_ostream_wchar* __thiscall basic_ostream_wchar_write(basic_ostream_wchar *this, const wchar_t *str, streamsize count)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);

    TRACE("(%p %s %s)\n", this, debugstr_w(str), wine_dbgstr_longlong(count));

    if(!basic_ostream_wchar_sentry_create(this)
            || basic_streambuf_wchar_sputn(base->strbuf, str, count)!=count) {
        basic_ostream_wchar_sentry_destroy(this);
        basic_ios_wchar_setstate(base, IOSTATE_badbit);
        return this;
    }

    basic_ostream_wchar_sentry_destroy(this);
    return this;
}

static basic_ostream_wchar* basic_ostream_print_short(basic_ostream_wchar *this, short val, const num_put *numput)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_long(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base),
                (ios_base_flags_get(&base->base) & FMTFLAG_basefield & (FMTFLAG_oct | FMTFLAG_hex))
                ? (LONG)((unsigned short)val) : (LONG)val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@F@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@F@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_short, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_short(basic_ostream_wchar *this, short val)
{
    return basic_ostream_print_short(this, val, num_put_wchar_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@F@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@F@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_short, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_short(basic_ostream_wchar *this, short val)
{
    return basic_ostream_print_short(this, val, num_put_short_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

static basic_ostream_wchar* basic_ostream_print_ushort(basic_ostream_wchar *this, unsigned short val, const num_put *numput)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_ulong(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@G@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@G@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_ushort, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_ushort(basic_ostream_wchar *this, unsigned short val)
{
    return basic_ostream_print_ushort(this, val, num_put_wchar_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@G@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@G@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_ushort, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_ushort(basic_ostream_wchar *this, unsigned short val)
{
    return basic_ostream_print_ushort(this, val, num_put_short_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ??6std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@0@AAV10@G@Z */
/* ??6std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@0@AEAV10@G@Z */
basic_ostream_wchar* __cdecl basic_ostream_short_print_ushort_global(basic_ostream_wchar *ostr, unsigned short val)
{
    return basic_ostream_print_ushort(ostr, val, num_put_short_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(ostr)))));
}

static basic_ostream_wchar* basic_ostream_print_int(basic_ostream_wchar *this, int val, const num_put *numput)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_long(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@H@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@H@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@J@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_int, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_int(basic_ostream_wchar *this, int val)
{
    return basic_ostream_print_int(this, val, num_put_wchar_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@H@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@H@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@J@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_int, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_int(basic_ostream_wchar *this, int val)
{
    return basic_ostream_print_int(this, val, num_put_short_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

static basic_ostream_wchar* basic_ostream_print_uint(basic_ostream_wchar *this, unsigned int val, const num_put *numput)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %u)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_ulong(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@I@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@I@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@K@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@K@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_uint, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_uint(basic_ostream_wchar *this, unsigned int val)
{
    return basic_ostream_print_uint(this, val, num_put_wchar_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@I@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@I@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@K@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@K@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_uint, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_uint(basic_ostream_wchar *this, unsigned int val)
{
    return basic_ostream_print_uint(this, val, num_put_short_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

static basic_ostream_wchar* basic_ostream_print_float(basic_ostream_wchar *this, float val, const num_put *numput)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %f)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_double(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@M@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@M@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_float, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_float(basic_ostream_wchar *this, float val)
{
    return basic_ostream_print_float(this, val, num_put_wchar_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@M@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@M@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_float, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_float(basic_ostream_wchar *this, float val)
{
    return basic_ostream_print_float(this, val, num_put_short_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

static basic_ostream_wchar* basic_ostream_print_double(basic_ostream_wchar *this, double val, const num_put *numput)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %lf)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_double(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@N@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_double, 12)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_double(basic_ostream_wchar *this, double val)
{
    return basic_ostream_print_double(this, val, num_put_wchar_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@N@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_double, 12)
basic_ostream_wchar* __thiscall basic_ostream_short_print_double(basic_ostream_wchar *this, double val)
{
    return basic_ostream_print_double(this, val, num_put_short_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

static basic_ostream_wchar* basic_ostream_print_ldouble(basic_ostream_wchar *this, double val, const num_put *numput)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %lf)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_ldouble(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@O@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@O@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_ldouble, 12)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_ldouble(basic_ostream_wchar *this, double val)
{
    return basic_ostream_print_ldouble(this, val, num_put_wchar_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@O@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@O@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_ldouble, 12)
basic_ostream_wchar* __thiscall basic_ostream_short_print_ldouble(basic_ostream_wchar *this, double val)
{
    return basic_ostream_print_ldouble(this, val, num_put_short_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_streambuf, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_streambuf(basic_ostream_wchar *this, basic_streambuf_wchar *val)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_badbit;
    unsigned short c = '\n';

    TRACE("(%p %p)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        for(c = basic_streambuf_wchar_sgetc(val); c!=WEOF;
                c = basic_streambuf_wchar_snextc(val)) {
            state = IOSTATE_goodbit;

            if(basic_streambuf_wchar_sputc(base->strbuf, c) == WEOF) {
                state = IOSTATE_badbit;
                break;
            }
        }
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_wchar_sentry_destroy(this);

    ios_base_width_set(&base->base, 0);
    basic_ios_wchar_setstate(base, state);
    return this;
}

static basic_ostream_wchar* basic_ostream_print_ptr(basic_ostream_wchar *this, const void *val, const num_put *numput)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_ptr(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@PBX@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@PEBX@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_ptr, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_ptr(basic_ostream_wchar *this, const void *val)
{
    return basic_ostream_print_ptr(this, val, num_put_wchar_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@PBX@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@PEBX@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_ptr, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_ptr(basic_ostream_wchar *this, const void *val)
{
    return basic_ostream_print_ptr(this, val, num_put_short_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

static basic_ostream_wchar* basic_ostream_print_int64(basic_ostream_wchar *this, __int64 val, const num_put *numput)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_int64(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@_J@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@_J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_int64, 12)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_int64(basic_ostream_wchar *this, __int64 val)
{
    return basic_ostream_print_int64(this, val, num_put_wchar_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@_J@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@_J@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_int64, 12)
basic_ostream_wchar* __thiscall basic_ostream_short_print_int64(basic_ostream_wchar *this, __int64 val)
{
    return basic_ostream_print_int64(this, val, num_put_short_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

static basic_ostream_wchar* basic_ostream_print_uint64(basic_ostream_wchar *this, unsigned __int64 val, const num_put *numput)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_uint64(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@_K@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@_K@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_uint64, 12)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_uint64(basic_ostream_wchar *this, unsigned __int64 val)
{
    return basic_ostream_print_uint64(this, val, num_put_wchar_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@_K@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@_K@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_uint64, 12)
basic_ostream_wchar* __thiscall basic_ostream_short_print_uint64(basic_ostream_wchar *this, unsigned __int64 val)
{
    return basic_ostream_print_uint64(this, val, num_put_short_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

static basic_ostream_wchar* basic_ostream_print_bool(basic_ostream_wchar *this, bool val, const num_put *numput)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %x)\n", this, val);

    if(basic_ostream_wchar_sentry_create(this)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        ostreambuf_iterator_wchar dest;

        memset(&dest, 0, sizeof(dest));
        dest.strbuf = strbuf;
        num_put_wchar_put_bool(numput, &dest, dest, &base->base, basic_ios_wchar_fill_get(base), val);
    }
    basic_ostream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@_N@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_bool, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_bool(basic_ostream_wchar *this, bool val)
{
    return basic_ostream_print_bool(this, val, num_put_wchar_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@_N@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_short_print_bool, 8)
basic_ostream_wchar* __thiscall basic_ostream_short_print_bool(basic_ostream_wchar *this, bool val)
{
    return basic_ostream_print_bool(this, val, num_put_short_use_facet(
                IOS_LOCALE(basic_ios_wchar_rdbuf_get(basic_ostream_wchar_get_basic_ios(this)))));
}

/* ?ends@std@@YAAAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@AAV21@@Z */
/* ?ends@std@@YAAEAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@AEAV21@@Z */
/* ?ends@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@1@AAV21@@Z */
/* ?ends@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@1@AEAV21@@Z */
basic_ostream_wchar* __cdecl basic_ostream_wchar_ends(basic_ostream_wchar *ostr)
{
    TRACE("(%p)\n", ostr);

    basic_ostream_wchar_put(ostr, 0);
    return ostr;
}

/* ?endl@std@@YAAAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@AAV21@@Z */
/* ?endl@std@@YAAEAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@AEAV21@@Z */
/* ?endl@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@1@AAV21@@Z */
/* ?endl@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@1@AEAV21@@Z */
basic_ostream_wchar* __cdecl basic_ostream_wchar_endl(basic_ostream_wchar *ostr)
{
    TRACE("(%p)\n", ostr);

    basic_ostream_wchar_put(ostr, '\n');
    basic_ostream_wchar_flush(ostr);
    return ostr;
}

/* ??$?6_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YAAAV?$basic_ostream@_WU?$char_traits@_W@std@@@0@AAV10@ABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?6_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YAAEAV?$basic_ostream@_WU?$char_traits@_W@std@@@0@AEAV10@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?6GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@0@AAV10@ABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?6GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@0@AEAV10@AEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
basic_ostream_wchar* __cdecl basic_ostream_wchar_print_bstr(basic_ostream_wchar *ostr, const basic_string_wchar *str)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", ostr, str);

    if(basic_ostream_wchar_sentry_create(ostr)) {
        size_t len = MSVCP_basic_string_wchar_length(str);
        streamsize pad = (base->base.wide>len ? base->base.wide-len : 0);

        if((base->base.fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_wchar_sputc(base->strbuf, base->fillch) == WEOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        if(state == IOSTATE_goodbit) {
            if(basic_streambuf_wchar_sputn(base->strbuf, MSVCP_basic_string_wchar_c_str(str), len) != len)
                state = IOSTATE_badbit;
        }

        if(state == IOSTATE_goodbit) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_wchar_sputc(base->strbuf, base->fillch) == WEOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        base->base.wide = 0;
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_wchar_sentry_destroy(ostr);

    basic_ios_wchar_setstate(base, state);
    return ostr;
}

/* ??$?6_WU?$char_traits@_W@std@@@std@@YAAAV?$basic_ostream@_WU?$char_traits@_W@std@@@0@AAV10@_W@Z */
/* ??$?6_WU?$char_traits@_W@std@@@std@@YAAEAV?$basic_ostream@_WU?$char_traits@_W@std@@@0@AEAV10@_W@Z */
/* ??$?6GU?$char_traits@G@std@@@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@0@AAV10@G@Z */
/* ??$?6GU?$char_traits@G@std@@@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@0@AEAV10@G@Z */
basic_ostream_wchar* __cdecl basic_ostream_wchar_print_ch(basic_ostream_wchar *ostr, wchar_t ch)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %d)\n", ostr, ch);

    if(basic_ostream_wchar_sentry_create(ostr)) {
        streamsize pad = (base->base.wide>1 ? base->base.wide-1 : 0);

        if((base->base.fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_wchar_sputc(base->strbuf, base->fillch) == WEOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        if(state == IOSTATE_goodbit) {
            if(basic_streambuf_wchar_sputc(base->strbuf, ch) == WEOF)
                state = IOSTATE_badbit;
        }

        if(state == IOSTATE_goodbit) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_wchar_sputc(base->strbuf, base->fillch) == WEOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        base->base.wide = 0;
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_wchar_sentry_destroy(ostr);

    basic_ios_wchar_setstate(base, state);
    return ostr;
}

/* ??$?6_WU?$char_traits@_W@std@@@std@@YAAAV?$basic_ostream@_WU?$char_traits@_W@std@@@0@AAV10@PB_W@Z */
/* ??$?6_WU?$char_traits@_W@std@@@std@@YAAEAV?$basic_ostream@_WU?$char_traits@_W@std@@@0@AEAV10@PEB_W@Z */
/* ??$?6GU?$char_traits@G@std@@@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@0@AAV10@PBG@Z */
/* ??$?6GU?$char_traits@G@std@@@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@0@AEAV10@PEBG@Z */
basic_ostream_wchar* __cdecl basic_ostream_wchar_print_str(basic_ostream_wchar *ostr, const wchar_t *str)
{
    basic_ios_wchar *base = basic_ostream_wchar_get_basic_ios(ostr);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %s)\n", ostr, debugstr_w(str));

    if(basic_ostream_wchar_sentry_create(ostr)) {
        size_t len = wcslen(str);
        streamsize pad = (base->base.wide>len ? base->base.wide-len : 0);

        if((base->base.fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_wchar_sputc(base->strbuf, base->fillch) == WEOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        if(state == IOSTATE_goodbit) {
            if(basic_streambuf_wchar_sputn(base->strbuf, str, len) != len)
                                        state = IOSTATE_badbit;
        }

        if(state == IOSTATE_goodbit) {
            for(; pad!=0; pad--) {
                if(basic_streambuf_wchar_sputc(base->strbuf, base->fillch) == WEOF) {
                    state = IOSTATE_badbit;
                    break;
                }
            }
        }

        base->base.wide = 0;
    }else {
        state = IOSTATE_badbit;
    }
    basic_ostream_wchar_sentry_destroy(ostr);

    basic_ios_wchar_setstate(base, state);
    return ostr;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@P6AAAV01@AAV01@@Z@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@P6AAEAV01@AEAV01@@Z@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAV01@AAV01@@Z@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@P6AAEAV01@AEAV01@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_func, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_func(basic_ostream_wchar *this,
        basic_ostream_wchar* (__cdecl *pfunc)(basic_ostream_wchar*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(this);
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@P6AAAV?$basic_ios@_WU?$char_traits@_W@std@@@1@AAV21@@Z@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@P6AAEAV?$basic_ios@_WU?$char_traits@_W@std@@@1@AEAV21@@Z@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAV?$basic_ios@GU?$char_traits@G@std@@@1@AAV21@@Z@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@P6AAEAV?$basic_ios@GU?$char_traits@G@std@@@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_func_basic_ios, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_func_basic_ios(basic_ostream_wchar *this,
        basic_ios_wchar* (__cdecl *pfunc)(basic_ios_wchar*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(basic_ostream_wchar_get_basic_ios(this));
    return this;
}

/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z */
/* ??6?$basic_ostream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@P6AAEAVios_base@1@AEAV21@@Z@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z */
/* ??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@P6AAEAVios_base@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_print_func_ios_base, 8)
basic_ostream_wchar* __thiscall basic_ostream_wchar_print_func_ios_base(
        basic_ostream_wchar *this, ios_base* (__cdecl *pfunc)(ios_base*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(&basic_ostream_wchar_get_basic_ios(this)->base);
    return this;
}

/* ?swap@?$basic_ostream@GU?$char_traits@G@std@@@std@@IAEXAAV12@@Z */
/* ?swap@?$basic_ostream@GU?$char_traits@G@std@@@std@@IEAAXAEAV12@@Z */
/* ?swap@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@IAEXAAV12@@Z */
/* ?swap@?$basic_ostream@_WU?$char_traits@_W@std@@@std@@IEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostream_wchar_swap, 8)
void __thiscall basic_ostream_wchar_swap(basic_ostream_wchar *this, basic_ostream_wchar *r)
{
    TRACE("(%p %p)\n", this, r);

    if(this == r)
        return;

    basic_ios_wchar_swap(basic_ostream_wchar_get_basic_ios(this),
            basic_ostream_wchar_get_basic_ios(r));
}

/* Caution: basic_istream uses virtual inheritance. */
static inline basic_ios_char* basic_istream_char_get_basic_ios(basic_istream_char *this)
{
    return (basic_ios_char*)((char*)this+this->vbtable[1]);
}

static inline basic_ios_char* basic_istream_char_to_basic_ios(basic_istream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_istream_char_vbtable[1]);
}

static inline basic_istream_char* basic_istream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_istream_char*)((char*)ptr-basic_istream_char_vbtable[1]);
}

/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N1@Z */
/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N1@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_ctor_init, 20)
basic_istream_char* __thiscall basic_istream_char_ctor_init(basic_istream_char *this,
        basic_streambuf_char *strbuf, bool isstd, bool noinit, bool virt_init)
{
    basic_ios_char *base;

    TRACE("(%p %p %d %d %d)\n", this, strbuf, isstd, noinit, virt_init);

    if(virt_init) {
        this->vbtable = basic_istream_char_vbtable;
        base = basic_istream_char_get_basic_ios(this);
        INIT_BASIC_IOS_VTORDISP(base);
        basic_ios_char_ctor(base);
    }else {
        base = basic_istream_char_get_basic_ios(this);
    }

    base->base.vtable = &basic_istream_char_vtable;
    this->count = 0;
    if(!noinit)
        basic_ios_char_init(base, strbuf, isstd);
    return this;
}

/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N@Z */
/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_ctor, 16)
basic_istream_char* __thiscall basic_istream_char_ctor(basic_istream_char *this,
        basic_streambuf_char *strbuf, bool isstd, bool virt_init)
{
    return basic_istream_char_ctor_init(this, strbuf, isstd, FALSE, virt_init);
}

/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_ctor_uninitialized, 12)
basic_istream_char* __thiscall basic_istream_char_ctor_uninitialized(basic_istream_char *this,
        int uninitialized, bool virt_init)
{
    basic_ios_char *base;

    TRACE("(%p %d %d)\n", this, uninitialized, virt_init);

    if(virt_init) {
        this->vbtable = basic_istream_char_vbtable;
        base = basic_istream_char_get_basic_ios(this);
        INIT_BASIC_IOS_VTORDISP(base);
        basic_ios_char_ctor(base);
    }else {
        base = basic_istream_char_get_basic_ios(this);
    }

    base->base.vtable = &basic_istream_char_vtable;
    ios_base_Addstd(&base->base);
    return this;
}

/* ??1?$basic_istream@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_istream@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_dtor, 4)
void __thiscall basic_istream_char_dtor(basic_ios_char *base)
{
    basic_istream_char *this = basic_istream_char_from_basic_ios(base);

    /* don't destroy virtual base here */
    TRACE("(%p)\n", this);
}

/* ??_D?$basic_istream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_D?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_vbase_dtor, 4)
void __thiscall basic_istream_char_vbase_dtor(basic_istream_char *this)
{
    basic_ios_char *base = basic_istream_char_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_istream_char_dtor(base);
    basic_ios_char_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_istream_char_vector_dtor, 8)
basic_istream_char* __thiscall basic_istream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_istream_char *this = basic_istream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_istream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_istream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Ipfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QAE_N_N@Z */
/* ?_Ipfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA_N_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char__Ipfx, 8)
bool __thiscall basic_istream_char__Ipfx(basic_istream_char *this, bool noskip)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);

    TRACE("(%p %d)\n", this, noskip);

    if(ios_base_good(&base->base)) {
        if(basic_ios_char_tie_get(base))
            basic_ostream_char_flush(basic_ios_char_tie_get(base));

        if(!noskip && (ios_base_flags_get(&base->base) & FMTFLAG_skipws)) {
            basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
            const ctype_char *ctype = ctype_char_use_facet(IOS_LOCALE(base->strbuf));
            int ch;

            for(ch = basic_streambuf_char_sgetc(strbuf); ;
                    ch = basic_streambuf_char_snextc(strbuf)) {
                if(ch == EOF) {
                    basic_ios_char_setstate(base, IOSTATE_eofbit);
                    break;
                }

                if(!ctype_char_is_ch(ctype, _SPACE|_BLANK, ch))
                    break;
            }
        }
    }

    if(!ios_base_good(&base->base)) {
        basic_ios_char_setstate(base, IOSTATE_failbit);
        return FALSE;
    }

    return TRUE;
}

/* ?ipfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QAE_N_N@Z */
/* ?ipfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA_N_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_ipfx, 8)
bool __thiscall basic_istream_char_ipfx(basic_istream_char *this, bool noskip)
{
    return basic_istream_char__Ipfx(this, noskip);
}

/* ?isfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?isfx@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_isfx, 4)
void __thiscall basic_istream_char_isfx(basic_istream_char *this)
{
    TRACE("(%p)\n", this);
}

static BOOL basic_istream_char_sentry_create(basic_istream_char *istr, bool noskip)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(istr);

    if(basic_ios_char_rdbuf_get(base))
        call_basic_streambuf_char__Lock(base->strbuf);

    return basic_istream_char_ipfx(istr, noskip);
}

static void basic_istream_char_sentry_destroy(basic_istream_char *istr)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(istr);

    if(basic_ios_char_rdbuf_get(base))
        call_basic_streambuf_char__Unlock(base->strbuf);
}

/* ?gcount@?$basic_istream@DU?$char_traits@D@std@@@std@@QBEHXZ */
/* ?gcount@?$basic_istream@DU?$char_traits@D@std@@@std@@QEBA_JXZ */
/* ?gcount@?$basic_istream@DU?$char_traits@D@std@@@std@@QBA_JXZ */
/* ?gcount@?$basic_istream@DU?$char_traits@D@std@@@std@@QBE_JXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_gcount, 4)
streamsize __thiscall basic_istream_char_gcount(const basic_istream_char *this)
{
    TRACE("(%p)\n", this);
    return this->count;
}

/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_get, 4)
int __thiscall basic_istream_char_get(basic_istream_char *this)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int ret;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(!basic_istream_char_sentry_create(this, TRUE)) {
        basic_istream_char_sentry_destroy(this);
        return EOF;
    }

    ret = basic_streambuf_char_sbumpc(basic_ios_char_rdbuf_get(base));
    basic_istream_char_sentry_destroy(this);
    if(ret == EOF)
        basic_ios_char_setstate(base, IOSTATE_eofbit|IOSTATE_failbit);
    else
        this->count++;

    return ret;
}

/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@AAD@Z */
/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@AEAD@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_get_ch, 8)
basic_istream_char* __thiscall basic_istream_char_get_ch(basic_istream_char *this, char *ch)
{
    int ret;

    TRACE("(%p %p)\n", this, ch);

    ret = basic_istream_char_get(this);
    if(ret != EOF)
        *ch = (char)ret;
    return this;
}

/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@PADHD@Z */
/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEAD_JD@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_char_get_str_delim, 20)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_char_get_str_delim, 16)
#endif
basic_istream_char* __thiscall basic_istream_char_get_str_delim(basic_istream_char *this, char *str, streamsize count, char delim)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int ch = delim;

    TRACE("(%p %p %s %s)\n", this, str, wine_dbgstr_longlong(count), debugstr_an(&delim, 1));

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        for(ch = basic_streambuf_char_sgetc(strbuf); count>1;
                ch = basic_streambuf_char_snextc(strbuf)) {
            if(ch==EOF || ch==delim)
                break;

            *str++ = ch;
            this->count++;
            count--;
        }
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, (!this->count ? IOSTATE_failbit : IOSTATE_goodbit) |
            (ch==EOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    if(count > 0)
        *str = 0;
    return this;
}

/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@PADH@Z */
/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEAD_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_char_get_str, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_char_get_str, 12)
#endif
basic_istream_char* __thiscall basic_istream_char_get_str(basic_istream_char *this, char *str, streamsize count)
{
    return basic_istream_char_get_str_delim(this, str, count, '\n');
}

/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@AAV?$basic_streambuf@DU?$char_traits@D@std@@@2@D@Z */
/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@AEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@D@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_get_streambuf_delim, 12)
basic_istream_char* __thiscall basic_istream_char_get_streambuf_delim(basic_istream_char *this, basic_streambuf_char *strbuf, char delim)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int ch = delim;

    TRACE("(%p %p %s)\n", this, strbuf, debugstr_an(&delim, 1));

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf_read = basic_ios_char_rdbuf_get(base);

        for(ch = basic_streambuf_char_sgetc(strbuf_read); ;
                ch = basic_streambuf_char_snextc(strbuf_read)) {
            if(ch==EOF || ch==delim)
                break;

            if(basic_streambuf_char_sputc(strbuf, ch) == EOF)
                break;
            this->count++;
        }
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, (!this->count ? IOSTATE_failbit : IOSTATE_goodbit) |
            (ch==EOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return this;
}

/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@AAV?$basic_streambuf@DU?$char_traits@D@std@@@2@@Z */
/* ?get@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@AEAV?$basic_streambuf@DU?$char_traits@D@std@@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_get_streambuf, 8)
basic_istream_char* __thiscall basic_istream_char_get_streambuf(basic_istream_char *this, basic_streambuf_char *strbuf)
{
    return basic_istream_char_get_streambuf_delim(this, strbuf, '\n');
}

/* ?getline@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@PADHD@Z */
/* ?getline@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEAD_JD@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_char_getline_delim, 20)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_char_getline_delim, 16)
#endif
basic_istream_char* __thiscall basic_istream_char_getline_delim(basic_istream_char *this, char *str, streamsize count, char delim)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int ch = (unsigned char)delim;

    TRACE("(%p %p %s %s)\n", this, str, wine_dbgstr_longlong(count), debugstr_an(&delim, 1));

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE) && count>0) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        while(count > 1) {
            ch = basic_streambuf_char_sbumpc(strbuf);

            if(ch==EOF || ch==(unsigned char)delim)
                break;

            *str++ = ch;
            this->count++;
            count--;
        }

        if(ch == (unsigned char)delim)
            this->count++;
        else if(ch != EOF) {
            ch = basic_streambuf_char_sgetc(strbuf);

            if(ch == (unsigned char)delim) {
                basic_streambuf_char__Gninc(strbuf);
                this->count++;
            }
        }
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, (ch==EOF ? IOSTATE_eofbit : IOSTATE_goodbit) |
            (!this->count || (ch!=(unsigned char)delim && ch!=EOF) ? IOSTATE_failbit : IOSTATE_goodbit));
    if(count > 0)
        *str = 0;
    return this;
}

/* ?getline@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@PADH@Z */
/* ?getline@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEAD_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_char_getline, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_char_getline, 12)
#endif
basic_istream_char* __thiscall basic_istream_char_getline(basic_istream_char *this, char *str, streamsize count)
{
    return basic_istream_char_getline_delim(this, str, count, '\n');
}

/* ?ignore@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@HH@Z */
/* ?ignore@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@_JH@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_char_ignore, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_char_ignore, 12)
#endif
basic_istream_char* __thiscall basic_istream_char_ignore(basic_istream_char *this, streamsize count, int delim)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int ch = (unsigned char)delim;
    unsigned int state;

    TRACE("(%p %s %d)\n", this, wine_dbgstr_longlong(count), delim);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        state = IOSTATE_goodbit;

        while(count > 0) {
            ch = basic_streambuf_char_sbumpc(strbuf);

            if(ch==EOF) {
                state = IOSTATE_eofbit;
                break;
            }

            if(ch==delim)
                break;

            this->count++;
            if(count != INT_MAX)
                count--;
        }
    }else
        state = IOSTATE_failbit;
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ?ws@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@1@AAV21@@Z */
/* ?ws@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@1@AEAV21@@Z */
basic_istream_char* __cdecl ws_basic_istream_char(basic_istream_char *istream)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(istream);
    int ch = '\n';

    TRACE("(%p)\n", istream);

    if(basic_istream_char_sentry_create(istream, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const ctype_char *ctype = ctype_char_use_facet(IOS_LOCALE(strbuf));

        for(ch = basic_streambuf_char_sgetc(strbuf); ctype_char_is_ch(ctype, _SPACE, ch);
                ch = basic_streambuf_char_snextc(strbuf)) {
            if(ch == EOF)
                break;
        }
    }
    basic_istream_char_sentry_destroy(istream);

    if(ch == EOF)
        basic_ios_char_setstate(base, IOSTATE_eofbit);
    return istream;
}

/* ?peek@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?peek@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_peek, 4)
int __thiscall basic_istream_char_peek(basic_istream_char *this)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int ret = EOF;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE))
        ret = basic_streambuf_char_sgetc(basic_ios_char_rdbuf_get(base));
    basic_istream_char_sentry_destroy(this);

    if (ret == EOF)
        basic_ios_char_setstate(base, IOSTATE_eofbit);

    return ret;
}

/* ?_Read_s@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@PADIH@Z */
/* ?_Read_s@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEAD_K_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_char__Read_s, 20)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_char__Read_s, 16)
#endif
basic_istream_char* __thiscall basic_istream_char__Read_s(basic_istream_char *this, char *str, size_t size, streamsize count)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p %Iu %s)\n", this, str, size, wine_dbgstr_longlong(count));

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        this->count = basic_streambuf_char__Sgetn_s(strbuf, str, size, count);
        if(this->count != count)
            state |= IOSTATE_failbit | IOSTATE_eofbit;
    }else {
        this->count = 0;
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ?read@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@PADH@Z */
/* ?read@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@PEAD_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_char_read, 12)
#endif
basic_istream_char* __thiscall basic_istream_char_read(basic_istream_char *this, char *str, streamsize count)
{
    return basic_istream_char__Read_s(this, str, -1, count);
}

/* ?_Readsome_s@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEHPADIH@Z */
/* ?_Readsome_s@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA_JPEAD_K_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_char__Readsome_s, 20)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_char__Readsome_s, 16)
#endif
streamsize __thiscall basic_istream_char__Readsome_s(basic_istream_char *this, char *str, size_t size, streamsize count)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p %Iu %s)\n", this, str, size, wine_dbgstr_longlong(count));

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        streamsize avail = basic_streambuf_char_in_avail(basic_ios_char_rdbuf_get(base));
        if(avail > count)
            avail = count;

        if(avail == -1)
            state |= IOSTATE_eofbit;
        else if(avail > 0)
            basic_istream_char__Read_s(this, str, size, avail);
    }else {
        state |= IOSTATE_failbit;
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this->count;
}

/* ?readsome@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEHPADH@Z */
/* ?readsome@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA_JPEAD_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_char_readsome, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_char_readsome, 12)
#endif
streamsize __thiscall basic_istream_char_readsome(basic_istream_char *this, char *str, streamsize count)
{
    return basic_istream_char__Readsome_s(this, str, count, count);
}

/* ?putback@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@D@Z */
/* ?putback@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@D@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_putback, 8)
basic_istream_char* __thiscall basic_istream_char_putback(basic_istream_char *this, char ch)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %c)\n", this, ch);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        if(!ios_base_good(&base->base))
            state |= IOSTATE_failbit;
        else if(!strbuf || basic_streambuf_char_sputbackc(strbuf, ch)==EOF)
            state |= IOSTATE_badbit;
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ?unget@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@XZ */
/* ?unget@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_unget, 4)
basic_istream_char* __thiscall basic_istream_char_unget(basic_istream_char *this)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

        if(!ios_base_good(&base->base))
            state |= IOSTATE_failbit;
        else if(!strbuf || basic_streambuf_char_sungetc(strbuf)==EOF)
            state |= IOSTATE_badbit;
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ?sync@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEHXZ */
/* ?sync@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_sync, 4)
int __thiscall basic_istream_char_sync(basic_istream_char *this)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);

    TRACE("(%p)\n", this);

    if(!strbuf)
        return -1;

    if(basic_istream_char_sentry_create(this, TRUE)) {
        if(basic_streambuf_char_pubsync(strbuf) != -1) {
            basic_istream_char_sentry_destroy(this);
            return 0;
        }
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, IOSTATE_badbit);
    return -1;
}

/* ?tellg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAE?AV?$fpos@H@2@XZ */
/* ?tellg@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAA?AV?$fpos@H@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_char_tellg, 8)
fpos_mbstatet* __thiscall basic_istream_char_tellg(basic_istream_char *this, fpos_mbstatet *ret)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);

    TRACE("(%p %p)\n", this, ret);

#if _MSVCP_VER >= 110
    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char_pubseekoff(basic_ios_char_rdbuf_get(base),
                ret, 0, SEEKDIR_cur, OPENMODE_in);
    }else {
        ret->off = -1;
        ret->pos = 0;
        memset(&ret->state, 0, sizeof(ret->state));
    }
    basic_istream_char_sentry_destroy(this);
#else
    if(ios_base_fail(&base->base)) {
        ret->off = -1;
        ret->pos = 0;
        memset(&ret->state, 0, sizeof(ret->state));
        return ret;
    }

    basic_streambuf_char_pubseekoff(basic_ios_char_rdbuf_get(base),
            ret, 0, SEEKDIR_cur, OPENMODE_in);
#endif

    return ret;
}

/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@JW4seekdir@ios_base@2@@Z */
/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@_JW4seekdir@ios_base@2@@Z */
/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@JH@Z */
/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@_JH@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_istream_char_seekg, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_char_seekg, 12)
#endif
basic_istream_char* __thiscall basic_istream_char_seekg(basic_istream_char *this, streamoff off, int dir)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
#if _MSVCP_VER >= 110
    IOSB_iostate state;

    TRACE("(%p %s %d)\n", this, wine_dbgstr_longlong(off), dir);

    state = ios_base_rdstate(&base->base);
    ios_base_clear(&base->base, state & ~IOSTATE_eofbit);

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        fpos_mbstatet ret;

        basic_streambuf_char_pubseekoff(strbuf, &ret, off, dir, OPENMODE_in);

        if(ret.off==-1 && ret.pos==0 && MBSTATET_TO_INT(&ret.state)==0)
            basic_ios_char_setstate(base, IOSTATE_failbit);
    }
    basic_istream_char_sentry_destroy(this);
#else
    TRACE("(%p %s %d)\n", this, wine_dbgstr_longlong(off), dir);

    if(!ios_base_fail(&base->base)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        fpos_mbstatet ret;

        basic_streambuf_char_pubseekoff(strbuf, &ret, off, dir, OPENMODE_in);

        if(ret.off==-1 && ret.pos==0 && MBSTATET_TO_INT(&ret.state)==0)
            basic_ios_char_setstate(base, IOSTATE_failbit);
        else
            basic_ios_char_clear(base, IOSTATE_goodbit);
        return this;
    }else
        basic_ios_char_clear(base, IOSTATE_goodbit);
#endif
    return this;
}

/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z */
/* ?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_seekg_fpos, 28)
basic_istream_char* __thiscall basic_istream_char_seekg_fpos(basic_istream_char *this, fpos_mbstatet pos)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
#if _MSVCP_VER >= 110
    IOSB_iostate state;

    TRACE("(%p %s)\n", this, debugstr_fpos_mbstatet(&pos));

    state = ios_base_rdstate(&base->base);
    ios_base_clear(&base->base, state & ~IOSTATE_eofbit);

    if(basic_istream_char_sentry_create(this, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        fpos_mbstatet ret;

        basic_streambuf_char_pubseekpos(strbuf, &ret, pos, OPENMODE_in);

        if(ret.off==-1 && ret.pos==0 && MBSTATET_TO_INT(&ret.state)==0)
            basic_ios_char_setstate(base, IOSTATE_failbit);
    }
    basic_istream_char_sentry_destroy(this);
#else
    TRACE("(%p %s)\n", this, debugstr_fpos_mbstatet(&pos));

    if(!ios_base_fail(&base->base)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        fpos_mbstatet ret;

        basic_streambuf_char_pubseekpos(strbuf, &ret, pos, OPENMODE_in);

        if(ret.off==-1 && ret.pos==0 && MBSTATET_TO_INT(&ret.state)==0)
            basic_ios_char_setstate(base, IOSTATE_failbit);
        else
            basic_ios_char_clear(base, IOSTATE_goodbit);
        return this;
    }else
        basic_ios_char_clear(base, IOSTATE_goodbit);
#endif
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAF@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAF@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_short, 8)
basic_istream_char* __thiscall basic_istream_char_read_short(basic_istream_char *this, short *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};
        LONG tmp;

        first.strbuf = strbuf;
        num_get_char_get_long(numget, &last, first, last, &base->base, &state, &tmp);

        if(!(state&IOSTATE_failbit) && tmp==(LONG)((short)tmp))
            *v = tmp;
        else
            state |= IOSTATE_failbit;
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAG@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAG@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_ushort, 8)
basic_istream_char* __thiscall basic_istream_char_read_ushort(basic_istream_char *this, unsigned short *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_ushort(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAH@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAH@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_int, 8)
basic_istream_char* __thiscall basic_istream_char_read_int(basic_istream_char *this, int *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_long(numget, &last, first, last, &base->base, &state, (LONG*)v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAI@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAI@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_uint, 8)
basic_istream_char* __thiscall basic_istream_char_read_uint(basic_istream_char *this, unsigned int *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_uint(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAJ@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAJ@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_long, 8)
basic_istream_char* __thiscall basic_istream_char_read_long(basic_istream_char *this, LONG *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_long(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAK@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAK@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_ulong, 8)
basic_istream_char* __thiscall basic_istream_char_read_ulong(basic_istream_char *this, ULONG *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_ulong(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAM@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAM@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_float, 8)
basic_istream_char* __thiscall basic_istream_char_read_float(basic_istream_char *this, float *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_float(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAN@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAN@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_double, 8)
basic_istream_char* __thiscall basic_istream_char_read_double(basic_istream_char *this, double *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_double(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAO@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAO@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_ldouble, 8)
basic_istream_char* __thiscall basic_istream_char_read_ldouble(basic_istream_char *this, double *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_ldouble(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AAPAX@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEAPEAX@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_ptr, 8)
basic_istream_char* __thiscall basic_istream_char_read_ptr(basic_istream_char *this, void **v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_void(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AA_J@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEA_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_int64, 8)
basic_istream_char* __thiscall basic_istream_char_read_int64(basic_istream_char *this, __int64 *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_int64(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AA_K@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEA_K@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_uint64, 8)
basic_istream_char* __thiscall basic_istream_char_read_uint64(basic_istream_char *this, unsigned __int64 *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_uint64(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@AA_N@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@AEA_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_bool, 8)
basic_istream_char* __thiscall basic_istream_char_read_bool(basic_istream_char *this, bool *v)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        const num_get *numget = num_get_char_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_char first={0}, last={0};

        first.strbuf = strbuf;
        num_get_char_get_bool(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state);
    return this;
}

/* ??$getline@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@D@Z */
/* ??$getline@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@D@Z */
basic_istream_char* __cdecl basic_istream_char_getline_bstr_delim(
        basic_istream_char *istream, basic_string_char *str, char delim)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(istream);
    IOSB_iostate state = IOSTATE_goodbit;
    int c = (unsigned char)delim;

    TRACE("(%p %p %s)\n", istream, str, debugstr_an(&delim, 1));

    if(basic_istream_char_sentry_create(istream, TRUE)) {
        basic_streambuf_char *strbuf = basic_ios_char_rdbuf_get(base);
        MSVCP_basic_string_char_clear(str);

        c = basic_streambuf_char_sgetc(strbuf);
        for(; c!=(unsigned char)delim && c!=EOF; c = basic_streambuf_char_snextc(strbuf))
            MSVCP_basic_string_char_append_ch(str, c);
        if(c==EOF) state |= IOSTATE_eofbit;
        else if(c==(unsigned char)delim) basic_streambuf_char_sbumpc(strbuf);

        if(!MSVCP_basic_string_char_length(str) && c!=(unsigned char)delim) state |= IOSTATE_failbit;
    }
    basic_istream_char_sentry_destroy(istream);

    basic_ios_char_setstate(basic_istream_char_get_basic_ios(istream), state);
    return istream;
}

/* ??$getline@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$getline@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
basic_istream_char* __cdecl basic_istream_char_getline_bstr(
        basic_istream_char *istream, basic_string_char *str)
{
    return basic_istream_char_getline_bstr_delim(istream, str, '\n');
}

/* ??$?5DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?5DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
basic_istream_char* __cdecl basic_istream_char_read_bstr(
        basic_istream_char *istream, basic_string_char *str)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(istream);
    IOSB_iostate state = IOSTATE_failbit;
    int c = '\n';

    TRACE("(%p %p)\n", istream, str);

    if(basic_istream_char_sentry_create(istream, FALSE)) {
        const ctype_char *ctype = ctype_char_use_facet(IOS_LOCALE(base->strbuf));
        size_t count = ios_base_width_get(&base->base);

        if(!count)
            count = -1;

        MSVCP_basic_string_char_clear(str);

        for(c = basic_streambuf_char_sgetc(basic_ios_char_rdbuf_get(base));
                c!=EOF && !ctype_char_is_ch(ctype, _SPACE|_BLANK, c) && count>0;
                c = basic_streambuf_char_snextc(basic_ios_char_rdbuf_get(base)), count--) {
            state = IOSTATE_goodbit;
            MSVCP_basic_string_char_append_ch(str, c);
        }
    }
    basic_istream_char_sentry_destroy(istream);

    ios_base_width_set(&base->base, 0);
    basic_ios_char_setstate(base, state | (c==EOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??$?5DU?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@PAD@Z */
/* ??$?5DU?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@PEAD@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@PAC@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@PEAC@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@PAE@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@PEAE@Z */
basic_istream_char* __cdecl basic_istream_char_read_str(basic_istream_char *istream, char *str)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(istream);
    IOSB_iostate state = IOSTATE_failbit;
    int c = '\n';

    TRACE("(%p %p)\n", istream, str);

    if(basic_istream_char_sentry_create(istream, FALSE)) {
        const ctype_char *ctype = ctype_char_use_facet(IOS_LOCALE(base->strbuf));
        size_t count = ios_base_width_get(&base->base)-1;

        for(c = basic_streambuf_char_sgetc(basic_ios_char_rdbuf_get(base));
                c!=EOF && !ctype_char_is_ch(ctype, _SPACE|_BLANK, c) && count>0;
                c = basic_streambuf_char_snextc(basic_ios_char_rdbuf_get(base)), count--) {
            state = IOSTATE_goodbit;
            *str++ = c;
        }
    }
    basic_istream_char_sentry_destroy(istream);

    *str = 0;
    ios_base_width_set(&base->base, 0);
    basic_ios_char_setstate(base, state | (c==EOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??$?5DU?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAD@Z */
/* ??$?5DU?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAD@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAC@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAC@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAE@Z */
/* ??$?5U?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAE@Z */
basic_istream_char* __cdecl basic_istream_char_read_ch(basic_istream_char *istream, char *ch)
{
    IOSB_iostate state = IOSTATE_failbit;
    int c = 0;

    TRACE("(%p %p)\n", istream, ch);

    if(basic_istream_char_sentry_create(istream, FALSE)) {
        c = basic_streambuf_char_sbumpc(basic_ios_char_rdbuf_get(
                    basic_istream_char_get_basic_ios(istream)));
        if(c != EOF) {
            state = IOSTATE_goodbit;
            *ch = c;
        }
    }
    basic_istream_char_sentry_destroy(istream);

    basic_ios_char_setstate(basic_istream_char_get_basic_ios(istream),
            state | (c==EOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_streambuf, 8)
basic_istream_char* __thiscall basic_istream_char_read_streambuf(
        basic_istream_char *this, basic_streambuf_char *streambuf)
{
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_failbit;
    int c = '\n';

    TRACE("(%p %p)\n", this, streambuf);

    if(basic_istream_char_sentry_create(this, FALSE)) {
        for(c = basic_streambuf_char_sgetc(basic_ios_char_rdbuf_get(base)); c!=EOF;
                c = basic_streambuf_char_snextc(basic_ios_char_rdbuf_get(base))) {
            state = IOSTATE_goodbit;
            if(basic_streambuf_char_sputc(streambuf, c) == EOF)
                break;
        }
    }
    basic_istream_char_sentry_destroy(this);

    basic_ios_char_setstate(base, state | (c==EOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@P6AAAV01@AAV01@@Z@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@P6AAEAV01@AEAV01@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_func, 8)
basic_istream_char* __thiscall basic_istream_char_read_func(basic_istream_char *this,
        basic_istream_char* (__cdecl *pfunc)(basic_istream_char*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(this);
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@P6AAAV?$basic_ios@DU?$char_traits@D@std@@@1@AAV21@@Z@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@P6AAEAV?$basic_ios@DU?$char_traits@D@std@@@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_func_basic_ios, 8)
basic_istream_char* __thiscall basic_istream_char_read_func_basic_ios(basic_istream_char *this,
        basic_ios_char* (__cdecl *pfunc)(basic_ios_char*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(basic_istream_char_get_basic_ios(this));
    return this;
}

/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z */
/* ??5?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAAEAV01@P6AAEAVios_base@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_read_func_ios_base, 8)
basic_istream_char* __thiscall basic_istream_char_read_func_ios_base(basic_istream_char *this,
        ios_base* (__cdecl *pfunc)(ios_base*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(&basic_istream_char_get_basic_ios(this)->base);
    return this;
}

/* ??$?5MDU?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAV?$complex@M@0@@Z */
/* ??$?5MDU?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAV?$complex@M@0@@Z */
basic_istream_char* __cdecl basic_istream_char_read_complex_float(basic_istream_char *this, complex_float *v)
{
    float r;
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);

    TRACE("(%p %p)\n", this, v);

    ws_basic_istream_char(this);
    if(basic_istream_char_peek(this) == '(') {
        char c;
        basic_istream_char_get(this);
        basic_istream_char_read_float(this, &r);

        if(ios_base_fail(&base->base))
            return this;

        ws_basic_istream_char(this);
        c = basic_istream_char_peek(this);
        if(c == ',') {
            float i;
            basic_istream_char_get(this);
            basic_istream_char_read_float(this, &i);

            if(ios_base_fail(&base->base))
                return this;

            ws_basic_istream_char(this);
            c = basic_istream_char_peek(this);
            if(c == ')') { /* supported format: (real, imag) */
                basic_istream_char_get(this);
                v->real = r;
                v->imag = i;
            }else {
                basic_ios_char_setstate(base, IOSTATE_failbit);
                return this;
            }
        }else if(c == ')') { /* supported format: (real) */
            basic_istream_char_get(this);
            v->real = r;
            v->imag = 0;
        }else {
            basic_ios_char_setstate(base, IOSTATE_failbit);
            return this;
        }
    }else { /* supported format: real */
        basic_istream_char_read_float(this, &r);

        if(ios_base_fail(&base->base))
            return this;

        v->real = r;
        v->imag = 0;
    }

    return this;
}

/* ??$?5NDU?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAV?$complex@N@0@@Z */
/* ??$?5DU?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAV?$complex@N@0@@Z */
basic_istream_char* __cdecl basic_istream_char_read_complex_double(basic_istream_char *this, complex_double *v)
{
    double r;
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);

    TRACE("(%p %p)\n", this, v);

    ws_basic_istream_char(this);
    if(basic_istream_char_peek(this) == '(') {
        char c;
        basic_istream_char_get(this);
        basic_istream_char_read_double(this, &r);

        if(ios_base_fail(&base->base))
            return this;

        ws_basic_istream_char(this);
        c = basic_istream_char_peek(this);
        if(c == ',') {
            double i;
            basic_istream_char_get(this);
            basic_istream_char_read_double(this, &i);

            if(ios_base_fail(&base->base))
                return this;

            ws_basic_istream_char(this);
            c = basic_istream_char_peek(this);
            if(c == ')') { /* supported format: (real, imag) */
                basic_istream_char_get(this);
                v->real = r;
                v->imag = i;
            }else {
                basic_ios_char_setstate(base, IOSTATE_failbit);
                return this;
            }
        }else if(c == ')') { /* supported format: (real) */
            basic_istream_char_get(this);
            v->real = r;
            v->imag = 0;
        }else {
            basic_ios_char_setstate(base, IOSTATE_failbit);
            return this;
        }
    }else { /* supported format: real */
        basic_istream_char_read_double(this, &r);

        if(ios_base_fail(&base->base))
            return this;

        v->real = r;
        v->imag = 0;
    }

    return this;
}

/* ??$?5ODU?$char_traits@D@std@@@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAV?$complex@O@0@@Z */
/* ??$?5ODU?$char_traits@D@std@@@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@0@AEAV10@AEAV?$complex@O@0@@Z */
basic_istream_char* __cdecl basic_istream_char_read_complex_ldouble(basic_istream_char *this, complex_double *v)
{
    double r;
    basic_ios_char *base = basic_istream_char_get_basic_ios(this);

    TRACE("(%p %p)\n", this, v);

    ws_basic_istream_char(this);
    if(basic_istream_char_peek(this) == '(') {
        char c;
        basic_istream_char_get(this);
        basic_istream_char_read_ldouble(this, &r);

        if(ios_base_fail(&base->base))
            return this;

        ws_basic_istream_char(this);
        c = basic_istream_char_peek(this);
        if(c == ',') {
            double i;
            basic_istream_char_get(this);
            basic_istream_char_read_ldouble(this, &i);

            if(ios_base_fail(&base->base))
                return this;

            ws_basic_istream_char(this);
            c = basic_istream_char_peek(this);
            if(c == ')') { /* supported format: (real, imag) */
                basic_istream_char_get(this);
                v->real = r;
                v->imag = i;
            }else {
                basic_ios_char_setstate(base, IOSTATE_failbit);
                return this;
            }
        }else if(c == ')') { /* supported format: (real) */
            basic_istream_char_get(this);
            v->real = r;
            v->imag = 0;
        }else {
            basic_ios_char_setstate(base, IOSTATE_failbit);
            return this;
        }
    }else { /* supported format: real */
        basic_istream_char_read_ldouble(this, &r);

        if(ios_base_fail(&base->base))
            return this;

        v->real = r;
        v->imag = 0;
    }

    return this;
}

/* ?swap@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_istream@DU?$char_traits@D@std@@@std@@QEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_char_swap, 8)
void __thiscall basic_istream_char_swap(basic_istream_char *this, basic_istream_char *r)
{
    TRACE("(%p %p)\n", this, r);

    if(this == r)
        return;

    basic_ios_char_swap(basic_istream_char_get_basic_ios(this),
            basic_istream_char_get_basic_ios(r));
    this->count ^= r->count;
    r->count ^= this->count;
    this->count ^= r->count;
}

/* Caution: basic_istream uses virtual inheritance. */
static inline basic_ios_wchar* basic_istream_wchar_get_basic_ios(basic_istream_wchar *this)
{
    return (basic_ios_wchar*)((char*)this+this->vbtable[1]);
}

static inline basic_ios_wchar* basic_istream_wchar_to_basic_ios(basic_istream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_istream_wchar_vbtable[1]);
}

static inline basic_istream_wchar* basic_istream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_istream_wchar*)((char*)ptr-basic_istream_wchar_vbtable[1]);
}

/* ??0?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAE@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@_N1@Z */
/* ??0?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@_N1@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_ctor_init, 20)
basic_istream_wchar* __thiscall basic_istream_wchar_ctor_init(basic_istream_wchar *this,
        basic_streambuf_wchar *strbuf, bool isstd, bool noinit, bool virt_init)
{
    basic_ios_wchar *base;

    TRACE("(%p %p %d %d %d)\n", this, strbuf, isstd, noinit, virt_init);

    if(virt_init) {
        this->vbtable = basic_istream_wchar_vbtable;
        base = basic_istream_wchar_get_basic_ios(this);
        INIT_BASIC_IOS_VTORDISP(base);
        basic_ios_wchar_ctor(base);
    }else {
        base = basic_istream_wchar_get_basic_ios(this);
    }

    base->base.vtable = &basic_istream_wchar_vtable;
    this->count = 0;
    if(!noinit)
        basic_ios_wchar_init(base, strbuf, isstd);
    return this;
}

/* ??0?$basic_istream@GU?$char_traits@G@std@@@std@@QAE@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@_N1@Z */
/* ??0?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@_N1@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_ctor_init, 20)
basic_istream_wchar* __thiscall basic_istream_short_ctor_init(basic_istream_wchar *this,
        basic_streambuf_wchar *strbuf, bool isstd, bool noinit, bool virt_init)
{
    basic_istream_wchar_ctor_init(this, strbuf, isstd, noinit, virt_init);
    basic_istream_wchar_get_basic_ios(this)->base.vtable = &basic_istream_short_vtable;
    return this;
}

/* ??0?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAE@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@_N@Z */
/* ??0?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_ctor, 16)
basic_istream_wchar* __thiscall basic_istream_wchar_ctor(basic_istream_wchar *this,
        basic_streambuf_wchar *strbuf, bool isstd, bool virt_init)
{
    return basic_istream_wchar_ctor_init(this, strbuf, isstd, FALSE, virt_init);
}

/* ??0?$basic_istream@GU?$char_traits@G@std@@@std@@QAE@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@_N@Z */
/* ??0?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_ctor, 16)
basic_istream_wchar* __thiscall basic_istream_short_ctor(basic_istream_wchar *this,
        basic_streambuf_wchar *strbuf, bool isstd, bool virt_init)
{
    return basic_istream_short_ctor_init(this, strbuf, isstd, FALSE, virt_init);
}

/* ??0?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_ctor_uninitialized, 12)
basic_istream_wchar* __thiscall basic_istream_wchar_ctor_uninitialized(
        basic_istream_wchar *this, int uninitialized, bool virt_init)
{
    basic_ios_wchar *base;

    TRACE("(%p %d %d)\n", this, uninitialized, virt_init);

    if(virt_init) {
        this->vbtable = basic_istream_wchar_vbtable;
        base = basic_istream_wchar_get_basic_ios(this);
        INIT_BASIC_IOS_VTORDISP(base);
        basic_ios_wchar_ctor(base);
    }else {
        base = basic_istream_wchar_get_basic_ios(this);
    }

    base->base.vtable = &basic_istream_wchar_vtable;
    ios_base_Addstd(&base->base);
    return this;
}

/* ??0?$basic_istream@GU?$char_traits@G@std@@@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_ctor_uninitialized, 12)
basic_istream_wchar* __thiscall basic_istream_short_ctor_uninitialized(
        basic_istream_wchar *this, int uninitialized, bool virt_init)
{
    basic_istream_wchar_ctor_uninitialized(this, uninitialized, virt_init);
    basic_istream_wchar_get_basic_ios(this)->base.vtable = &basic_istream_short_vtable;
    return this;
}

/* ??1?$basic_istream@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_istream@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
/* ??1?$basic_istream@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_istream@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_dtor, 4)
void __thiscall basic_istream_wchar_dtor(basic_ios_wchar *base)
{
    basic_istream_wchar *this = basic_istream_wchar_from_basic_ios(base);

    /* don't destroy virtual base here */
    TRACE("(%p)\n", this);
}

/* ??_D?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ??_D?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ??_D?$basic_istream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_D?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_vbase_dtor, 4)
void __thiscall basic_istream_wchar_vbase_dtor(basic_istream_wchar *this)
{
    basic_ios_wchar *base = basic_istream_wchar_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_istream_wchar_dtor(base);
    basic_ios_wchar_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_istream_wchar_vector_dtor, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_istream_wchar *this = basic_istream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_istream_wchar_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_istream_wchar_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Ipfx@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAE_N_N@Z */
/* ?_Ipfx@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA_N_N@Z */
/* ?_Ipfx@?$basic_istream@GU?$char_traits@G@std@@@std@@QAE_N_N@Z */
/* ?_Ipfx@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA_N_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar__Ipfx, 8)
bool __thiscall basic_istream_wchar__Ipfx(basic_istream_wchar *this, bool noskip)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);

    TRACE("(%p %d)\n", this, noskip);

    if(ios_base_good(&base->base)) {
        if(basic_ios_wchar_tie_get(base))
            basic_ostream_wchar_flush(basic_ios_wchar_tie_get(base));

        if(!noskip && (ios_base_flags_get(&base->base) & FMTFLAG_skipws)) {
            basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
            const ctype_wchar *ctype = ctype_wchar_use_facet(IOS_LOCALE(base->strbuf));
            int ch;

            for(ch = basic_streambuf_wchar_sgetc(strbuf); ;
                    ch = basic_streambuf_wchar_snextc(strbuf)) {
                if(ch == WEOF) {
                    basic_ios_wchar_setstate(base, IOSTATE_eofbit);
                    break;
                }

                if(!ctype_wchar_is_ch(ctype, _SPACE|_BLANK, ch))
                    break;
            }
        }
    }

    if(!ios_base_good(&base->base)) {
        basic_ios_wchar_setstate(base, IOSTATE_failbit);
        return FALSE;
    }
    return TRUE;
}

/* ?ipfx@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAE_N_N@Z */
/* ?ipfx@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA_N_N@Z */
/* ?ipfx@?$basic_istream@GU?$char_traits@G@std@@@std@@QAE_N_N@Z */
/* ?ipfx@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA_N_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_ipfx, 8)
bool __thiscall basic_istream_wchar_ipfx(basic_istream_wchar *this, bool noskip)
{
    return basic_istream_wchar__Ipfx(this, noskip);
}

/* ?isfx@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?isfx@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ?isfx@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?isfx@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_isfx, 4)
void __thiscall basic_istream_wchar_isfx(basic_istream_wchar *this)
{
    TRACE("(%p)\n", this);
}

static BOOL basic_istream_wchar_sentry_create(basic_istream_wchar *istr, bool noskip)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(istr);

    if(basic_ios_wchar_rdbuf_get(base))
        basic_streambuf_wchar__Lock(base->strbuf);

    return basic_istream_wchar_ipfx(istr, noskip);
}

static void basic_istream_wchar_sentry_destroy(basic_istream_wchar *istr)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(istr);

    if(basic_ios_wchar_rdbuf_get(base))
        basic_streambuf_wchar__Unlock(base->strbuf);
}

/* ?gcount@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QBEHXZ */
/* ?gcount@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEBA_JXZ */
/* ?gcount@?$basic_istream@GU?$char_traits@G@std@@@std@@QBEHXZ */
/* ?gcount@?$basic_istream@GU?$char_traits@G@std@@@std@@QEBA_JXZ */
/* ?gcount@?$basic_istream@GU?$char_traits@G@std@@@std@@QBA_JXZ */
/* ?gcount@?$basic_istream@GU?$char_traits@G@std@@@std@@QBE_JXZ */
/* ?gcount@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QBA_JXZ */
/* ?gcount@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QBE_JXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_gcount, 4)
streamsize __thiscall basic_istream_wchar_gcount(const basic_istream_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->count;
}

/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEGXZ */
/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAGXZ */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get, 4)
unsigned short __thiscall basic_istream_wchar_get(basic_istream_wchar *this)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int ret;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(!basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_istream_wchar_sentry_destroy(this);
        return WEOF;
    }

    ret = basic_streambuf_wchar_sbumpc(basic_ios_wchar_rdbuf_get(base));
    basic_istream_wchar_sentry_destroy(this);
    if(ret == WEOF)
        basic_ios_wchar_setstate(base, IOSTATE_eofbit|IOSTATE_failbit);
    else
        this->count++;

    return ret;
}

/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@AA_W@Z */
/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@AEA_W@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@AAG@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@AEAG@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get_ch, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_get_ch(basic_istream_wchar *this, wchar_t *ch)
{
    unsigned short ret;

    TRACE("(%p %p)\n", this, ch);

    ret = basic_istream_wchar_get(this);
    if(ret != WEOF)
        *ch = (wchar_t)ret;
    return this;
}

/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PA_WH_W@Z */
/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEA_W_J_W@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@PAGHG@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@PEAG_JG@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get_str_delim, 20)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get_str_delim, 16)
#endif
basic_istream_wchar* __thiscall basic_istream_wchar_get_str_delim(basic_istream_wchar *this, wchar_t *str, streamsize count, wchar_t delim)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    unsigned short ch = delim;

    TRACE("(%p %p %s %s)\n", this, str, wine_dbgstr_longlong(count), debugstr_wn(&delim, 1));

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);

        for(ch = basic_streambuf_wchar_sgetc(strbuf); count>1;
                ch = basic_streambuf_wchar_snextc(strbuf)) {
            if(ch==WEOF || ch==delim)
                break;

            *str++ = ch;
            this->count++;
            count--;
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, (!this->count ? IOSTATE_failbit : IOSTATE_goodbit) |
            (ch==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    if(count > 0)
        *str = 0;
    return this;
}

/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PA_WH@Z */
/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEA_W_J@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@PAGH@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@PEAG_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get_str, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get_str, 12)
#endif
basic_istream_wchar* __thiscall basic_istream_wchar_get_str(basic_istream_wchar *this, wchar_t *str, streamsize count)
{
    return basic_istream_wchar_get_str_delim(this, str, count, '\n');
}

/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@AAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@_W@Z */
/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@AEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@_W@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@AAV?$basic_streambuf@GU?$char_traits@G@std@@@2@G@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@AEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@G@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get_streambuf_delim, 12)
basic_istream_wchar* __thiscall basic_istream_wchar_get_streambuf_delim(basic_istream_wchar *this, basic_streambuf_wchar *strbuf, wchar_t delim)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    unsigned short ch = delim;

    TRACE("(%p %p %s)\n", this, strbuf, debugstr_wn(&delim, 1));

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf_read = basic_ios_wchar_rdbuf_get(base);

        for(ch = basic_streambuf_wchar_sgetc(strbuf_read); ;
                ch = basic_streambuf_wchar_snextc(strbuf_read)) {
            if(ch==WEOF || ch==delim)
                break;

            if(basic_streambuf_wchar_sputc(strbuf, ch) == WEOF)
                break;
            this->count++;
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, (!this->count ? IOSTATE_failbit : IOSTATE_goodbit) |
            (ch==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return this;
}

/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@AAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@@Z */
/* ?get@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@AEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@2@@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@AAV?$basic_streambuf@GU?$char_traits@G@std@@@2@@Z */
/* ?get@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@AEAV?$basic_streambuf@GU?$char_traits@G@std@@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_get_streambuf, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_get_streambuf(basic_istream_wchar *this, basic_streambuf_wchar *strbuf)
{
    return basic_istream_wchar_get_streambuf_delim(this, strbuf, '\n');
}

/* ?getline@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PA_WH_W@Z */
/* ?getline@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEA_W_J_W@Z */
/* ?getline@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@PAGHG@Z */
/* ?getline@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@PEAG_JG@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_getline_delim, 20)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_getline_delim, 16)
#endif
basic_istream_wchar* __thiscall basic_istream_wchar_getline_delim(basic_istream_wchar *this, wchar_t *str, streamsize count, wchar_t delim)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    unsigned short ch = delim;

    TRACE("(%p %p %s %s)\n", this, str, wine_dbgstr_longlong(count), debugstr_wn(&delim, 1));

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE) && count>0) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);

        while(count > 1) {
            ch = basic_streambuf_wchar_sbumpc(strbuf);

            if(ch==WEOF || ch==delim)
                break;

            *str++ = ch;
            this->count++;
            count--;
        }

        if(ch == delim)
            this->count++;
        else if(ch != WEOF) {
            ch = basic_streambuf_wchar_sgetc(strbuf);

            if(ch == delim) {
                basic_streambuf_wchar__Gninc(strbuf);
                this->count++;
            }
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, (ch==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit) |
            (!this->count || (ch!=delim && ch!=WEOF) ? IOSTATE_failbit : IOSTATE_goodbit));
    if(count > 0)
        *str = 0;
    return this;
}

/* ?getline@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PA_WH@Z */
/* ?getline@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEA_W_J@Z */
/* ?getline@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@PAGH@Z */
/* ?getline@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@PEAG_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_getline, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_getline, 12)
#endif
basic_istream_wchar* __thiscall basic_istream_wchar_getline(basic_istream_wchar *this, wchar_t *str, streamsize count)
{
    return basic_istream_wchar_getline_delim(this, str, count, '\n');
}

/* ?ignore@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@HG@Z */
/* ?ignore@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@_JG@Z */
/* ?ignore@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@HG@Z */
/* ?ignore@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@_JG@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_ignore, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_ignore, 12)
#endif
basic_istream_wchar* __thiscall basic_istream_wchar_ignore(basic_istream_wchar *this, streamsize count, unsigned short delim)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    unsigned short ch = delim;
    unsigned int state;

    TRACE("(%p %s %d)\n", this, wine_dbgstr_longlong(count), delim);

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        state = IOSTATE_goodbit;

        while(count > 0) {
            ch = basic_streambuf_wchar_sbumpc(strbuf);

            if(ch==WEOF) {
                state = IOSTATE_eofbit;
                break;
            }

            if(ch==delim)
                break;

            this->count++;
            if(count != INT_MAX)
                count--;
        }
    }else
        state = IOSTATE_failbit;
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ?ws@std@@YAAAV?$basic_istream@_WU?$char_traits@_W@std@@@1@AAV21@@Z */
/* ?ws@std@@YAAEAV?$basic_istream@_WU?$char_traits@_W@std@@@1@AEAV21@@Z */
/* ?ws@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@1@AAV21@@Z */
/* ?ws@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@1@AEAV21@@Z */
basic_istream_wchar* __cdecl ws_basic_istream_wchar(basic_istream_wchar *istream)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(istream);
    unsigned short ch = '\n';

    TRACE("(%p)\n", istream);

    if(basic_istream_wchar_sentry_create(istream, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const ctype_wchar *ctype = ctype_wchar_use_facet(IOS_LOCALE(strbuf));

        for(ch = basic_streambuf_wchar_sgetc(strbuf); ctype_wchar_is_ch(ctype, _SPACE, ch);
                ch = basic_streambuf_wchar_snextc(strbuf)) {
            if(ch == WEOF)
                break;
        }
    }
    basic_istream_wchar_sentry_destroy(istream);

    if(ch == WEOF)
        basic_ios_wchar_setstate(base, IOSTATE_eofbit);
    return istream;
}

/* ?peek@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEGXZ */
/* ?peek@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAGXZ */
/* ?peek@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEGXZ */
/* ?peek@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAGXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_peek, 4)
unsigned short __thiscall basic_istream_wchar_peek(basic_istream_wchar *this)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    unsigned short ret = WEOF;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE))
        ret = basic_streambuf_wchar_sgetc(basic_ios_wchar_rdbuf_get(base));
    basic_istream_wchar_sentry_destroy(this);

    if (ret == WEOF)
        basic_ios_wchar_setstate(base, IOSTATE_eofbit);

    return ret;
}

/* ?_Read_s@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PA_WIH@Z */
/* ?_Read_s@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEA_W_K_J@Z */
/* ?_Read_s@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@PAGIH@Z */
/* ?_Read_s@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@PEAG_K_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar__Read_s, 20)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_wchar__Read_s, 16)
#endif
basic_istream_wchar* __thiscall basic_istream_wchar__Read_s(basic_istream_wchar *this, wchar_t *str, size_t size, streamsize count)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p %Iu %s)\n", this, str, size, wine_dbgstr_longlong(count));

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);

        this->count = basic_streambuf_wchar__Sgetn_s(strbuf, str, size, count);
        if(this->count != count)
            state |= IOSTATE_failbit | IOSTATE_eofbit;
    }else {
        this->count = 0;
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ?read@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@PA_WH@Z */
/* ?read@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@PEA_W_J@Z */
/* ?read@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@PAGH@Z */
/* ?read@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@PEAG_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read, 12)
#endif
basic_istream_wchar* __thiscall basic_istream_wchar_read(basic_istream_wchar *this, wchar_t *str, streamsize count)
{
    return basic_istream_wchar__Read_s(this, str, -1, count);
}

/* ?_Readsome_s@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEHPA_WIH@Z */
/* ?_Readsome_s@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA_JPEA_W_K_J@Z */
/* ?_Readsome_s@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEHPAGIH@Z */
/* ?_Readsome_s@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA_JPEAG_K_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar__Readsome_s, 20)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_wchar__Readsome_s, 16)
#endif
streamsize __thiscall basic_istream_wchar__Readsome_s(basic_istream_wchar *this, wchar_t *str, size_t size, streamsize count)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %p %Iu %s)\n", this, str, size, wine_dbgstr_longlong(count));

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        streamsize avail = basic_streambuf_wchar_in_avail(basic_ios_wchar_rdbuf_get(base));
        if(avail > count)
            avail = count;

        if(avail == -1)
            state |= IOSTATE_eofbit;
        else if(avail > 0)
            basic_istream_wchar__Read_s(this, str, size, avail);
    }else {
        state |= IOSTATE_failbit;
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this->count;
}

/* ?readsome@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEHPA_WH@Z */
/* ?readsome@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA_JPEA_W_J@Z */
/* ?readsome@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEHPAGH@Z */
/* ?readsome@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA_JPEAG_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_readsome, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_readsome, 12)
#endif
streamsize __thiscall basic_istream_wchar_readsome(basic_istream_wchar *this, wchar_t *str, streamsize count)
{
    return basic_istream_wchar__Readsome_s(this, str, count, count);
}

/* ?putback@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@_W@Z */
/* ?putback@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@_W@Z */
/* ?putback@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@G@Z */
/* ?putback@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@G@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_putback, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_putback(basic_istream_wchar *this, wchar_t ch)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p %c)\n", this, ch);

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);

        if(!ios_base_good(&base->base))
            state |= IOSTATE_failbit;
        else if(!strbuf || basic_streambuf_wchar_sputbackc(strbuf, ch)==WEOF)
            state |= IOSTATE_badbit;
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ?unget@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@XZ */
/* ?unget@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@XZ */
/* ?unget@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@XZ */
/* ?unget@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_unget, 4)
basic_istream_wchar* __thiscall basic_istream_wchar_unget(basic_istream_wchar *this)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_goodbit;

    TRACE("(%p)\n", this);

    this->count = 0;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);

        if(!ios_base_good(&base->base))
            state |= IOSTATE_failbit;
        else if(!strbuf || basic_streambuf_wchar_sungetc(strbuf)==WEOF)
            state |= IOSTATE_badbit;
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ?sync@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEHXZ */
/* ?sync@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAHXZ */
/* ?sync@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEHXZ */
/* ?sync@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAHXZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_sync, 4)
int __thiscall basic_istream_wchar_sync(basic_istream_wchar *this)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);

    TRACE("(%p)\n", this);

    if(!strbuf)
        return -1;

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        if(basic_streambuf_wchar_pubsync(strbuf) != -1) {
            basic_istream_wchar_sentry_destroy(this);
            return 0;
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, IOSTATE_badbit);
    return -1;
}

/* ?tellg@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAE?AV?$fpos@H@2@XZ */
/* ?tellg@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAA?AV?$fpos@H@2@XZ */
/* ?tellg@?$basic_istream@GU?$char_traits@G@std@@@std@@QAE?AV?$fpos@H@2@XZ */
/* ?tellg@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAA?AV?$fpos@H@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_tellg, 8)
fpos_mbstatet* __thiscall basic_istream_wchar_tellg(basic_istream_wchar *this, fpos_mbstatet *ret)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);

    TRACE("(%p %p)\n", this, ret);

#if _MSVCP_VER >= 110
    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar_pubseekoff(basic_ios_wchar_rdbuf_get(base),
                ret, 0, SEEKDIR_cur, OPENMODE_in);
    }else {
        ret->off = -1;
        ret->pos = 0;
        memset(&ret->state, 0, sizeof(ret->state));
    }
    basic_istream_wchar_sentry_destroy(this);
#else
    if(ios_base_fail(&base->base)) {
        ret->off = -1;
        ret->pos = 0;
        memset(&ret->state, 0, sizeof(ret->state));
        return ret;
    }

    basic_streambuf_wchar_pubseekoff(basic_ios_wchar_rdbuf_get(base),
            ret, 0, SEEKDIR_cur, OPENMODE_in);
#endif
    return ret;
}

/* ?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@JW4seekdir@ios_base@2@@Z */
/* ?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@_JW4seekdir@ios_base@2@@Z */
/* ?seekg@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@JH@Z */
/* ?seekg@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@_JH@Z */
/* ?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@JH@Z */
/* ?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@_JH@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_seekg, 16)
#else
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_seekg, 12)
#endif
basic_istream_wchar* __thiscall basic_istream_wchar_seekg(basic_istream_wchar *this, streamoff off, int dir)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
#if _MSVCP_VER >= 110
    IOSB_iostate state;

    TRACE("(%p %s %d)\n", this, wine_dbgstr_longlong(off), dir);

    state = ios_base_rdstate(&base->base);
    ios_base_clear(&base->base, state & ~IOSTATE_eofbit);

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        fpos_mbstatet ret;

        basic_streambuf_wchar_pubseekoff(strbuf, &ret, off, dir, OPENMODE_in);

        if(ret.off==-1 && ret.pos==0 && MBSTATET_TO_INT(&ret.state)==0)
            basic_ios_wchar_setstate(base, IOSTATE_failbit);
    }
    basic_istream_wchar_sentry_destroy(this);
#else
    TRACE("(%p %s %d)\n", this, wine_dbgstr_longlong(off), dir);

    if(!ios_base_fail(&base->base)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        fpos_mbstatet ret;

        basic_streambuf_wchar_pubseekoff(strbuf, &ret, off, dir, OPENMODE_in);

        if(ret.off==-1 && ret.pos==0 && MBSTATET_TO_INT(&ret.state)==0)
            basic_ios_wchar_setstate(base, IOSTATE_failbit);
        else
            basic_ios_wchar_clear(base, IOSTATE_goodbit);
        return this;
    }else
        basic_ios_wchar_clear(base, IOSTATE_goodbit);
#endif
    return this;
}

/* ?seekg@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z */
/* ?seekg@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z */
/* ?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z */
/* ?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV12@V?$fpos@H@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_seekg_fpos, 28)
basic_istream_wchar* __thiscall basic_istream_wchar_seekg_fpos(basic_istream_wchar *this, fpos_mbstatet pos)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
#if _MSVCP_VER >= 110
    IOSB_iostate state;

    TRACE("(%p %s)\n", this, debugstr_fpos_mbstatet(&pos));

    state = ios_base_rdstate(&base->base);
    ios_base_clear(&base->base, state & ~IOSTATE_eofbit);

    if(basic_istream_wchar_sentry_create(this, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        fpos_mbstatet ret;

        basic_streambuf_wchar_pubseekpos(strbuf, &ret, pos, OPENMODE_in);

        if(ret.off==-1 && ret.pos==0 && MBSTATET_TO_INT(&ret.state)==0)
            basic_ios_wchar_setstate(base, IOSTATE_failbit);
    }
    basic_istream_wchar_sentry_destroy(this);
#else
    TRACE("(%p %s)\n", this, debugstr_fpos_mbstatet(&pos));

    if(!ios_base_fail(&base->base)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        fpos_mbstatet ret;

        basic_streambuf_wchar_pubseekpos(strbuf, &ret, pos, OPENMODE_in);

        if(ret.off==-1 && ret.pos==0 && MBSTATET_TO_INT(&ret.state)==0)
            basic_ios_wchar_setstate(base, IOSTATE_failbit);
        else
            basic_ios_wchar_clear(base, IOSTATE_goodbit);
        return this;
    }else
        basic_ios_wchar_clear(base, IOSTATE_goodbit);
#endif
    return this;
}

static basic_istream_wchar* basic_istream_read_short(basic_istream_wchar *this, short *v, const num_get *numget)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};
        LONG tmp;

        first.strbuf = strbuf;
        num_get_wchar_get_long(numget, &last, first, last, &base->base, &state, &tmp);

        if(!(state&IOSTATE_failbit) && tmp==(LONG)((short)tmp))
            *v = tmp;
        else
            state |= IOSTATE_failbit;
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAF@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAF@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_short, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_short(basic_istream_wchar *this, short *v)
{
    return basic_istream_read_short(this, v, num_get_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAF@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAF@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_short, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_short(basic_istream_wchar *this, short *v)
{
    return basic_istream_read_short(this, v, num_get_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAG@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAG@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_ushort, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_ushort(basic_istream_wchar *this, unsigned short *v)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        const num_get *numget = num_get_wchar_use_facet(IOS_LOCALE(strbuf));
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_ushort(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

static basic_istream_wchar* basic_istream_read_int(basic_istream_wchar *this, int *v, const num_get *numget)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_long(numget, &last, first, last, &base->base, &state, (LONG*)v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAH@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAH@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_int, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_int(basic_istream_wchar *this, int *v)
{
    return basic_istream_read_int(this, v, num_get_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAH@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAH@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_int, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_int(basic_istream_wchar *this, int *v)
{
    return basic_istream_read_int(this, v, num_get_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

static basic_istream_wchar* basic_istream_read_uint(basic_istream_wchar *this, unsigned int *v, const num_get *numget)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_uint(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAI@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAI@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_uint, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_uint(basic_istream_wchar *this, unsigned int *v)
{
    return basic_istream_read_uint(this, v, num_get_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAI@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAI@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_uint, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_uint(basic_istream_wchar *this, unsigned int *v)
{
    return basic_istream_read_uint(this, v, num_get_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

static basic_istream_wchar* basic_istream_read_long(basic_istream_wchar *this, LONG *v, const num_get *numget)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_long(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAJ@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAJ@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_long, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_long(basic_istream_wchar *this, LONG *v)
{
    return basic_istream_read_long(this, v, num_get_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAJ@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAJ@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_long, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_long(basic_istream_wchar *this, LONG *v)
{
    return basic_istream_read_long(this, v, num_get_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

static basic_istream_wchar* basic_istream_read_ulong(basic_istream_wchar *this, ULONG *v, const num_get *numget)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_ulong(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAK@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAK@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_ulong, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_ulong(basic_istream_wchar *this, ULONG *v)
{
    return basic_istream_read_ulong(this, v, num_get_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAK@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAK@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_ulong, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_ulong(basic_istream_wchar *this, ULONG *v)
{
    return basic_istream_read_ulong(this, v, num_get_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

static basic_istream_wchar* basic_istream_read_float(basic_istream_wchar *this, float *v, const num_get *numget)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_float(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAM@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAM@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_float, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_float(basic_istream_wchar *this, float *v)
{
    return basic_istream_read_float(this, v, num_get_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAM@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAM@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_float, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_float(basic_istream_wchar *this, float *v)
{
    return basic_istream_read_float(this, v, num_get_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

static basic_istream_wchar* basic_istream_read_double(basic_istream_wchar *this, double *v, const num_get *numget)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_double(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAN@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAN@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_double, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_double(basic_istream_wchar *this, double *v)
{
    return basic_istream_read_double(this, v, num_get_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAN@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAN@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_double, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_double(basic_istream_wchar *this, double *v)
{
    return basic_istream_read_double(this, v, num_get_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

static basic_istream_wchar* basic_istream_read_ldouble(basic_istream_wchar *this, double *v, const num_get *numget)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_ldouble(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAO@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAO@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_ldouble, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_ldouble(basic_istream_wchar *this, double *v)
{
    return basic_istream_read_ldouble(this, v, num_get_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAO@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAO@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_ldouble, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_ldouble(basic_istream_wchar *this, double *v)
{
    return basic_istream_read_ldouble(this, v, num_get_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

static basic_istream_wchar* basic_istream_read_ptr(basic_istream_wchar *this, void **v, const num_get *numget)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_void(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AAPAX@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEAPEAX@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_ptr, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_ptr(basic_istream_wchar *this, void **v)
{
    return basic_istream_read_ptr(this, v, num_get_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AAPAX@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEAPEAX@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_ptr, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_ptr(basic_istream_wchar *this, void **v)
{
    return basic_istream_read_ptr(this, v, num_get_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

static basic_istream_wchar* basic_istream_read_int64(basic_istream_wchar *this, __int64 *v, const num_get *numget)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_int64(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AA_J@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEA_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_int64, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_int64(basic_istream_wchar *this, __int64 *v)
{
    return basic_istream_read_int64(this, v, num_get_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AA_J@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEA_J@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_int64, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_int64(basic_istream_wchar *this, __int64 *v)
{
    return basic_istream_read_int64(this, v, num_get_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

static basic_istream_wchar* basic_istream_read_uint64(basic_istream_wchar *this, unsigned __int64 *v, const num_get *numget)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_uint64(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AA_K@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEA_K@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_uint64, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_uint64(basic_istream_wchar *this, unsigned __int64 *v)
{
    return basic_istream_read_uint64(this, v, num_get_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AA_K@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEA_K@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_uint64, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_uint64(basic_istream_wchar *this, unsigned __int64 *v)
{
    return basic_istream_read_uint64(this, v, num_get_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

static basic_istream_wchar* basic_istream_read_bool(basic_istream_wchar *this, bool *v, const num_get *numget)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    int state = IOSTATE_goodbit;

    TRACE("(%p %p)\n", this, v);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        istreambuf_iterator_wchar first={0}, last={0};

        first.strbuf = strbuf;
        num_get_wchar_get_bool(numget, &last, first, last, &base->base, &state, v);
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@AA_N@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@AEA_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_bool, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_bool(basic_istream_wchar *this, bool *v)
{
    return basic_istream_read_bool(this, v, num_get_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@AA_N@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@AEA_N@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_short_read_bool, 8)
basic_istream_wchar* __thiscall basic_istream_short_read_bool(basic_istream_wchar *this, bool *v)
{
    return basic_istream_read_bool(this, v, num_get_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(this)->strbuf)));
}

/* ??$getline@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@YAAAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AAV10@AAV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@_W@Z */
/* ??$getline@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@YAAEAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AEAV10@AEAV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@_W@Z */
/* ??$getline@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@0@AAV10@AAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@G@Z */
/* ??$getline@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@0@AEAV10@AEAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@G@Z */
basic_istream_wchar* __cdecl basic_istream_wchar_getline_bstr_delim(
        basic_istream_wchar *istream, basic_string_wchar *str, wchar_t delim)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(istream);
    IOSB_iostate state = IOSTATE_goodbit;
    int c = delim;

    TRACE("(%p %p %s)\n", istream, str, debugstr_wn(&delim, 1));

    if(basic_istream_wchar_sentry_create(istream, TRUE)) {
        basic_streambuf_wchar *strbuf = basic_ios_wchar_rdbuf_get(base);
        MSVCP_basic_string_wchar_clear(str);

        c = basic_streambuf_wchar_sgetc(strbuf);
        for(; c!=delim && c!=WEOF; c = basic_streambuf_wchar_snextc(strbuf))
            MSVCP_basic_string_wchar_append_ch(str, c);
        if(c==delim) basic_streambuf_wchar_sbumpc(strbuf);
        else if(c==WEOF) state |= IOSTATE_eofbit;

        if(!MSVCP_basic_string_wchar_length(str) && c!=delim) state |= IOSTATE_failbit;
    }
    basic_istream_wchar_sentry_destroy(istream);

    basic_ios_wchar_setstate(basic_istream_wchar_get_basic_ios(istream), state);
    return istream;
}

/* ??$getline@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@YAAAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AAV10@AAV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$getline@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@YAAEAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AEAV10@AEAV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$getline@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@0@AAV10@AAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$getline@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@0@AEAV10@AEAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
basic_istream_wchar* __cdecl basic_istream_wchar_getline_bstr(
        basic_istream_wchar *istream, basic_string_wchar *str)
{
    return basic_istream_wchar_getline_bstr_delim(istream, str, '\n');
}

static basic_istream_wchar* basic_istream_read_bstr(basic_istream_wchar *istream,
        basic_string_wchar *str, const ctype_wchar *ctype)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(istream);
    IOSB_iostate state = IOSTATE_failbit;
    int c = '\n';

    TRACE("(%p %p)\n", istream, str);

    if(basic_istream_wchar_sentry_create(istream, FALSE)) {
        size_t count = ios_base_width_get(&base->base);

        if(!count)
            count = -1;

        MSVCP_basic_string_wchar_clear(str);

        for(c = basic_streambuf_wchar_sgetc(basic_ios_wchar_rdbuf_get(base));
                c!=WEOF && !ctype_wchar_is_ch(ctype, _SPACE|_BLANK, c) && count>0;
                c = basic_streambuf_wchar_snextc(basic_ios_wchar_rdbuf_get(base)), count--) {
            state = IOSTATE_goodbit;
            MSVCP_basic_string_wchar_append_ch(str, c);
        }
    }
    basic_istream_wchar_sentry_destroy(istream);

    ios_base_width_set(&base->base, 0);
    basic_ios_wchar_setstate(base, state | (c==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??$?5_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YAAAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AAV10@AAV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?5_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YAAEAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AEAV10@AEAV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
basic_istream_wchar* __cdecl basic_istream_wchar_read_bstr(
        basic_istream_wchar *istream, basic_string_wchar *str)
{
    return basic_istream_read_bstr(istream, str, ctype_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(istream)->strbuf)));
}

/* ??$?5GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@0@AAV10@AAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?5GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@0@AEAV10@AEAV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
basic_istream_wchar* __cdecl basic_istream_short_read_bstr(
        basic_istream_wchar *istream, basic_string_wchar *str)
{
    return basic_istream_read_bstr(istream, str, ctype_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(istream)->strbuf)));
}

static basic_istream_wchar* basic_istream_read_str(basic_istream_wchar *istream, wchar_t *str, const ctype_wchar *ctype)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(istream);
    IOSB_iostate state = IOSTATE_failbit;
    unsigned short c = '\n';

    TRACE("(%p %p)\n", istream, str);

    if(basic_istream_wchar_sentry_create(istream, FALSE)) {
        size_t count = ios_base_width_get(&base->base)-1;

        for(c = basic_streambuf_wchar_sgetc(basic_ios_wchar_rdbuf_get(base));
                c!=WEOF && !ctype_wchar_is_ch(ctype, _SPACE|_BLANK, c) && count>0;
                c = basic_streambuf_wchar_snextc(basic_ios_wchar_rdbuf_get(base)), count--) {
            state = IOSTATE_goodbit;
            *str++ = c;
        }
    }
    basic_istream_wchar_sentry_destroy(istream);

    *str = 0;
    ios_base_width_set(&base->base, 0);
    basic_ios_wchar_setstate(base, state | (c==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??$?5_WU?$char_traits@_W@std@@@std@@YAAAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AAV10@PA_W@Z */
/* ??$?5_WU?$char_traits@_W@std@@@std@@YAAEAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AEAV10@PEA_W@Z */
basic_istream_wchar* __cdecl basic_istream_wchar_read_str(basic_istream_wchar *istream, wchar_t *str)
{
    return basic_istream_read_str(istream, str, ctype_wchar_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(istream)->strbuf)));
}

/* ??$?5GU?$char_traits@G@std@@@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@0@AAV10@PAG@Z */
/* ??$?5GU?$char_traits@G@std@@@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@0@AEAV10@PEAG@Z */
basic_istream_wchar* __cdecl basic_istream_short_read_str(basic_istream_wchar *istream, wchar_t *str)
{
    return basic_istream_read_str(istream, str, ctype_short_use_facet(
                IOS_LOCALE(basic_istream_wchar_get_basic_ios(istream)->strbuf)));
}

/* ??$?5_WU?$char_traits@_W@std@@@std@@YAAAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AAV10@AA_W@Z */
/* ??$?5_WU?$char_traits@_W@std@@@std@@YAAEAV?$basic_istream@_WU?$char_traits@_W@std@@@0@AEAV10@AEA_W@Z */
/* ??$?5GU?$char_traits@G@std@@@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@0@AAV10@AAG@Z */
/* ??$?5GU?$char_traits@G@std@@@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@0@AEAV10@AEAG@Z */
basic_istream_wchar* __cdecl basic_istream_wchar_read_ch(basic_istream_wchar *istream, wchar_t *ch)
{
    IOSB_iostate state = IOSTATE_failbit;
    unsigned short c = 0;

    TRACE("(%p %p)\n", istream, ch);

    if(basic_istream_wchar_sentry_create(istream, FALSE)) {
        c = basic_streambuf_wchar_sbumpc(basic_ios_wchar_rdbuf_get(
                    basic_istream_wchar_get_basic_ios(istream)));
        if(c != WEOF) {
            state = IOSTATE_goodbit;
            *ch = c;
        }
    }
    basic_istream_wchar_sentry_destroy(istream);

    basic_ios_wchar_setstate(basic_istream_wchar_get_basic_ios(istream),
            state | (c==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return istream;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_streambuf, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_streambuf(
        basic_istream_wchar *this, basic_streambuf_wchar *streambuf)
{
    basic_ios_wchar *base = basic_istream_wchar_get_basic_ios(this);
    IOSB_iostate state = IOSTATE_failbit;
    unsigned short c = '\n';

    TRACE("(%p %p)\n", this, streambuf);

    if(basic_istream_wchar_sentry_create(this, FALSE)) {
        for(c = basic_streambuf_wchar_sgetc(basic_ios_wchar_rdbuf_get(base)); c!=WEOF;
                c = basic_streambuf_wchar_snextc(basic_ios_wchar_rdbuf_get(base))) {
            state = IOSTATE_goodbit;
            if(basic_streambuf_wchar_sputc(streambuf, c) == WEOF)
                break;
        }
    }
    basic_istream_wchar_sentry_destroy(this);

    basic_ios_wchar_setstate(base, state | (c==WEOF ? IOSTATE_eofbit : IOSTATE_goodbit));
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@P6AAAV01@AAV01@@Z@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@P6AAEAV01@AEAV01@@Z@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAV01@AAV01@@Z@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@P6AAEAV01@AEAV01@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_func, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_func(basic_istream_wchar *this,
        basic_istream_wchar* (__cdecl *pfunc)(basic_istream_wchar*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(this);
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@P6AAAV?$basic_ios@_WU?$char_traits@_W@std@@@1@AAV21@@Z@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@P6AAEAV?$basic_ios@_WU?$char_traits@_W@std@@@1@AEAV21@@Z@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAV?$basic_ios@GU?$char_traits@G@std@@@1@AAV21@@Z@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@P6AAEAV?$basic_ios@GU?$char_traits@G@std@@@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_func_basic_ios, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_func_basic_ios(basic_istream_wchar *this,
        basic_ios_wchar* (__cdecl *pfunc)(basic_ios_wchar*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(basic_istream_wchar_get_basic_ios(this));
    return this;
}

/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z */
/* ??5?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAAEAV01@P6AAEAVios_base@1@AEAV21@@Z@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z */
/* ??5?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAAEAV01@P6AAEAVios_base@1@AEAV21@@Z@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_read_func_ios_base, 8)
basic_istream_wchar* __thiscall basic_istream_wchar_read_func_ios_base(
        basic_istream_wchar *this, ios_base* (__cdecl *pfunc)(ios_base*))
{
    TRACE("(%p %p)\n", this, pfunc);
    pfunc(&basic_istream_wchar_get_basic_ios(this)->base);
    return this;
}

/* ?swap@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_istream@GU?$char_traits@G@std@@@std@@QEAAXAEAV12@@Z */
/* ?swap@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_istream@_WU?$char_traits@_W@std@@@std@@QEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_istream_wchar_swap, 8)
void __thiscall basic_istream_wchar_swap(basic_istream_wchar *this, basic_istream_wchar *r)
{
    TRACE("(%p %p)\n", this, r);

    if(this == r)
        return;

    basic_ios_wchar_swap(basic_istream_wchar_get_basic_ios(this),
            basic_istream_wchar_get_basic_ios(r));
    this->count ^= r->count;
    r->count ^= this->count;
    this->count ^= r->count;
}

static inline basic_ios_char* basic_iostream_char_to_basic_ios(basic_iostream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_iostream_char_vbtable1[1]);
}

static inline basic_iostream_char* basic_iostream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_iostream_char*)((char*)ptr-basic_iostream_char_vbtable1[1]);
}

/* ??0?$basic_iostream@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
/* ??0?$basic_iostream@DU?$char_traits@D@std@@@std@@QEAA@PEAV?$basic_streambuf@DU?$char_traits@D@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_iostream_char_ctor, 12)
basic_iostream_char* __thiscall basic_iostream_char_ctor(basic_iostream_char *this, basic_streambuf_char *strbuf, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d)\n", this, strbuf, virt_init);

    if(virt_init) {
        this->base1.vbtable = basic_iostream_char_vbtable1;
        this->base2.vbtable = basic_iostream_char_vbtable2;
        basic_ios = basic_istream_char_get_basic_ios(&this->base1);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base1);
    }

    basic_istream_char_ctor(&this->base1, strbuf, FALSE, FALSE);
    basic_ostream_char_ctor_uninitialized(&this->base2, 0, FALSE, FALSE);
    basic_ios->base.vtable = &basic_iostream_char_vtable;
    return this;
}

/* ??1?$basic_iostream@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_iostream@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_iostream_char_dtor, 4)
void __thiscall basic_iostream_char_dtor(basic_ios_char *base)
{
    basic_iostream_char *this = basic_iostream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);
    basic_ostream_char_dtor(basic_ostream_char_to_basic_ios(&this->base2));
    basic_istream_char_dtor(basic_istream_char_to_basic_ios(&this->base1));
}

/* ??_D?$basic_iostream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_D?$basic_iostream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_iostream_char_vbase_dtor, 4)
void __thiscall basic_iostream_char_vbase_dtor(basic_iostream_char *this)
{
    basic_ios_char *base = basic_iostream_char_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_iostream_char_dtor(base);
    basic_ios_char_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_iostream_char_vector_dtor, 8)
basic_iostream_char* __thiscall basic_iostream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_iostream_char *this = basic_iostream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_iostream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_iostream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?swap@?$basic_iostream@DU?$char_traits@D@std@@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_iostream@DU?$char_traits@D@std@@@std@@QEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_iostream_char_swap, 8)
void __thiscall basic_iostream_char_swap(basic_iostream_char *this, basic_iostream_char *r)
{
    TRACE("(%p %p)\n", this, r);

    if(this == r)
        return;

    basic_ios_char_swap(basic_istream_char_get_basic_ios(&this->base1),
            basic_istream_char_get_basic_ios(&r->base1));
}

static inline basic_ios_wchar* basic_iostream_wchar_to_basic_ios(basic_iostream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_iostream_wchar_vbtable1[1]);
}

static inline basic_iostream_wchar* basic_iostream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_iostream_wchar*)((char*)ptr-basic_iostream_wchar_vbtable1[1]);
}

/* ??0?$basic_iostream@_WU?$char_traits@_W@std@@@std@@QAE@PAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
/* ??0?$basic_iostream@_WU?$char_traits@_W@std@@@std@@QEAA@PEAV?$basic_streambuf@_WU?$char_traits@_W@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_iostream_wchar_ctor, 12)
basic_iostream_wchar* __thiscall basic_iostream_wchar_ctor(basic_iostream_wchar *this,
        basic_streambuf_wchar *strbuf, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d)\n", this, strbuf, virt_init);

    if(virt_init) {
        this->base1.vbtable = basic_iostream_wchar_vbtable1;
        this->base2.vbtable = basic_iostream_wchar_vbtable2;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base1);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base1);
    }

    basic_istream_wchar_ctor(&this->base1, strbuf, FALSE, FALSE);
    basic_ostream_wchar_ctor_uninitialized(&this->base2, 0, FALSE, FALSE);

    basic_ios->base.vtable = &basic_iostream_wchar_vtable;
    return this;
}

/* ??0?$basic_iostream@GU?$char_traits@G@std@@@std@@QAE@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
/* ??0?$basic_iostream@GU?$char_traits@G@std@@@std@@QEAA@PEAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z */
DEFINE_THISCALL_WRAPPER(basic_iostream_short_ctor, 12)
basic_iostream_wchar* __thiscall basic_iostream_short_ctor(basic_iostream_wchar *this,
        basic_streambuf_wchar *strbuf, bool virt_init)
{
    basic_iostream_wchar_ctor(this, strbuf, virt_init);
    basic_istream_wchar_get_basic_ios(&this->base1)->base.vtable = &basic_iostream_short_vtable;
    return this;
}

/* ??1?$basic_iostream@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_iostream@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
/* ??1?$basic_iostream@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_iostream@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_iostream_wchar_dtor, 4)
void __thiscall basic_iostream_wchar_dtor(basic_ios_wchar *base)
{
    basic_iostream_wchar *this = basic_iostream_wchar_from_basic_ios(base);

    TRACE("(%p)\n", this);
    basic_ostream_wchar_dtor(basic_ostream_wchar_to_basic_ios(&this->base2));
    basic_istream_wchar_dtor(basic_istream_wchar_to_basic_ios(&this->base1));
}

/* ??_D?$basic_iostream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ??_D?$basic_iostream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ??_D?$basic_iostream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_D?$basic_iostream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_iostream_wchar_vbase_dtor, 4)
void __thiscall basic_iostream_wchar_vbase_dtor(basic_iostream_wchar *this)
{
    basic_ios_wchar *base = basic_iostream_wchar_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_iostream_wchar_dtor(base);
    basic_ios_wchar_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_iostream_wchar_vector_dtor, 8)
basic_iostream_wchar* __thiscall basic_iostream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_iostream_wchar *this = basic_iostream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_iostream_wchar_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_iostream_wchar_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?swap@?$basic_iostream@GU?$char_traits@G@std@@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_iostream@GU?$char_traits@G@std@@@std@@QEAAXAEAV12@@Z */
/* ?swap@?$basic_iostream@_WU?$char_traits@_W@std@@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_iostream@_WU?$char_traits@_W@std@@@std@@QEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_iostream_wchar_swap, 8)
void __thiscall basic_iostream_wchar_swap(basic_iostream_wchar *this, basic_iostream_wchar *r)
{
    TRACE("(%p %p)\n", this, r);

    if(this == r)
        return;

    basic_ios_wchar_swap(basic_istream_wchar_get_basic_ios(&this->base1),
            basic_istream_wchar_get_basic_ios(&r->base1));
}

static inline basic_ios_char* basic_ofstream_char_to_basic_ios(basic_ofstream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_ofstream_char_vbtable[1]);
}

static inline basic_ofstream_char* basic_ofstream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_ofstream_char*)((char*)ptr-basic_ofstream_char_vbtable[1]);
}

/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAE@XZ */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_ctor, 8)
basic_ofstream_char* __thiscall basic_ofstream_char_ctor(basic_ofstream_char *this, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %d)\n", this, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ofstream_char_vbtable;
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
    }

    basic_filebuf_char_ctor(&this->filebuf);
    basic_ostream_char_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ofstream_char_vtable;
    return this;
}

/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_ctor_file, 12)
basic_ofstream_char* __thiscall basic_ofstream_char_ctor_file(
        basic_ofstream_char *this, FILE *file, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d)\n", this, file, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ofstream_char_vbtable;
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
    }

    basic_filebuf_char_ctor_file(&this->filebuf, file);
    basic_ostream_char_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ofstream_char_vtable;
    return this;
}

/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAE@PBDHH@Z */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAA@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_ctor_name, 20)
basic_ofstream_char* __thiscall basic_ofstream_char_ctor_name(basic_ofstream_char *this,
        const char *name, int mode, int prot, bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, name, mode, prot, virt_init);

    basic_ofstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open(&this->filebuf, name, mode|OPENMODE_out, prot)) {
        basic_ios_char *basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAA@PEBGHH@Z */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAE@PB_WHH@Z */
/* ??0?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAA@PEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_ctor_name_wchar, 20)
basic_ofstream_char* __thiscall basic_ofstream_char_ctor_name_wchar(basic_ofstream_char *this,
        const wchar_t *name, int mode, int prot, bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, debugstr_w(name), mode, prot, virt_init);

    basic_ofstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open_wchar(&this->filebuf, name, mode|OPENMODE_out, prot)) {
        basic_ios_char *basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??1?$basic_ofstream@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_ofstream@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_dtor, 4)
void __thiscall basic_ofstream_char_dtor(basic_ios_char *base)
{
    basic_ofstream_char *this = basic_ofstream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_ostream_char_dtor(basic_ostream_char_to_basic_ios(&this->base));
    basic_filebuf_char_dtor(&this->filebuf);
}

/* ??_D?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_D?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_vbase_dtor, 4)
void __thiscall basic_ofstream_char_vbase_dtor(basic_ofstream_char *this)
{
    basic_ios_char *base = basic_ofstream_char_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_ofstream_char_dtor(base);
    basic_ios_char_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_ofstream_char_vector_dtor, 8)
basic_ofstream_char* __thiscall basic_ofstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_ofstream_char *this = basic_ofstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ofstream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ofstream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?close@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?close@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_close, 4)
void __thiscall basic_ofstream_char_close(basic_ofstream_char *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_char_close(&this->filebuf)) {
        basic_ios_char *basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?is_open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_is_open, 4)
bool __thiscall basic_ofstream_char_is_open(const basic_ofstream_char *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_char_is_open(&this->filebuf);
}

/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPBDHH@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_open, 16)
void __thiscall basic_ofstream_char_open(basic_ofstream_char *this,
        const char *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, prot);

    if(!basic_filebuf_char_open(&this->filebuf, name, mode|OPENMODE_out, prot)) {
        basic_ios_char *basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPBDI@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_open_old, 12)
void __thiscall basic_ofstream_char_open_old(basic_ofstream_char *this,
        const char *name, unsigned int mode)
{
    basic_ofstream_char_open(this, name, mode, _SH_DENYNO);
}

/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPBGHH@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEBGHH@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPB_WHH@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_open_wchar, 16)
void __thiscall basic_ofstream_char_open_wchar(basic_ofstream_char *this,
        const wchar_t *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, debugstr_w(name), mode, prot);

    if(!basic_filebuf_char_open_wchar(&this->filebuf, name, mode|OPENMODE_out, prot)) {
        basic_ios_char *basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPBGI@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEBGI@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QAEXPB_WI@Z */
/* ?open@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEAAXPEB_WI@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_open_wchar_old, 12)
void __thiscall basic_ofstream_char_open_wchar_old(basic_ofstream_char *this,
        const wchar_t *name, unsigned int mode)
{
    basic_ofstream_char_open_wchar(this, name, mode, _SH_DENYNO);
}

/* ?rdbuf@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
/* ?rdbuf@?$basic_ofstream@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_char_rdbuf, 4)
basic_filebuf_char* __thiscall basic_ofstream_char_rdbuf(const basic_ofstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_char*)&this->filebuf;
}

static inline basic_ios_wchar* basic_ofstream_wchar_to_basic_ios(basic_ofstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_ofstream_wchar_vbtable[1]);
}

static inline basic_ofstream_wchar* basic_ofstream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_ofstream_wchar*)((char*)ptr-basic_ofstream_wchar_vbtable[1]);
}

/* ??0?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QAE@XZ */
/* ??0?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_ctor, 8)
basic_ofstream_wchar* __thiscall basic_ofstream_wchar_ctor(basic_ofstream_wchar *this, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d)\n", this, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ofstream_wchar_vbtable;
        basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
    }

    basic_filebuf_wchar_ctor(&this->filebuf);
    basic_ostream_wchar_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ofstream_wchar_vtable;
    return this;
}

/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAE@XZ */
/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_ctor, 8)
basic_ofstream_wchar* __thiscall basic_ofstream_short_ctor(basic_ofstream_wchar *this, bool virt_init)
{
    basic_ofstream_wchar_ctor(this, virt_init);
    basic_ostream_wchar_get_basic_ios(&this->base)->base.vtable = &basic_ofstream_short_vtable;
    return this;
}

/* ??0?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_ctor_file, 12)
basic_ofstream_wchar* __thiscall basic_ofstream_wchar_ctor_file(
        basic_ofstream_wchar *this, FILE *file, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d)\n", this, file, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ofstream_wchar_vbtable;
        basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
    }

    basic_filebuf_wchar_ctor_file(&this->filebuf, file);
    basic_ostream_wchar_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ofstream_wchar_vtable;
    return this;
}

/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_ctor_file, 12)
basic_ofstream_wchar* __thiscall basic_ofstream_short_ctor_file(
        basic_ofstream_wchar *this, FILE *file, bool virt_init)
{
    basic_ofstream_wchar_ctor_file(this, file, virt_init);
    basic_ostream_wchar_get_basic_ios(&this->base)->base.vtable = &basic_ofstream_short_vtable;
    return this;
}

/* ??0?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QAE@PBDHH@Z */
/* ??0?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEAA@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_ctor_name, 20)
basic_ofstream_wchar* __thiscall basic_ofstream_wchar_ctor_name(basic_ofstream_wchar *this,
        const char *name, int mode, int prot, bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, name, mode, prot, virt_init);

    basic_ofstream_wchar_ctor(this, virt_init);

    if(!basic_filebuf_wchar_open(&this->filebuf, name, mode|OPENMODE_out, prot)) {
        basic_ios_wchar *basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAE@PBDHH@Z */
/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAA@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_ctor_name, 20)
basic_ofstream_wchar* __thiscall basic_ofstream_short_ctor_name(basic_ofstream_wchar *this,
        const char *name, int mode, int prot, bool virt_init)
{
    basic_ofstream_wchar_ctor_name(this, name, mode, prot, virt_init);
    basic_ostream_wchar_get_basic_ios(&this->base)->base.vtable = &basic_ofstream_short_vtable;
    return this;
}

/* ??0?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEAA@PEBGHH@Z */
/* ??0?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QAE@PB_WHH@Z */
/* ??0?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEAA@PEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_ctor_name_wchar, 20)
basic_ofstream_wchar* __thiscall basic_ofstream_wchar_ctor_name_wchar(basic_ofstream_wchar *this,
        const wchar_t *name, int mode, int prot, bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, debugstr_w(name), mode, prot, virt_init);

    basic_ofstream_wchar_ctor(this, virt_init);

    if(!basic_filebuf_wchar_open_wchar(&this->filebuf, name, mode|OPENMODE_out, prot)) {
        basic_ios_wchar *basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAA@PEBGHH@Z */
/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAE@PB_WHH@Z */
/* ??0?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAA@PEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_short_ctor_name_wchar, 20)
basic_ofstream_wchar* __thiscall basic_ofstream_short_ctor_name_wchar(basic_ofstream_wchar *this,
        const wchar_t *name, int mode, int prot, bool virt_init)
{
    basic_ofstream_wchar_ctor_name_wchar(this, name, mode, prot, virt_init);
    basic_ostream_wchar_get_basic_ios(&this->base)->base.vtable = &basic_ofstream_short_vtable;
    return this;
}

/* ??1?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
/* ??1?$basic_ofstream@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_ofstream@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_dtor, 4)
void __thiscall basic_ofstream_wchar_dtor(basic_ios_wchar *base)
{
    basic_ofstream_wchar *this = basic_ofstream_wchar_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_ostream_wchar_dtor(basic_ostream_wchar_to_basic_ios(&this->base));
    basic_filebuf_wchar_dtor(&this->filebuf);
}

/* ??_D?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ??_D?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ??_D?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_D?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_vbase_dtor, 4)
void __thiscall basic_ofstream_wchar_vbase_dtor(basic_ofstream_wchar *this)
{
    basic_ios_wchar *base = basic_ofstream_wchar_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_ofstream_wchar_dtor(base);
    basic_ios_wchar_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_vector_dtor, 8)
basic_ofstream_wchar* __thiscall basic_ofstream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_ofstream_wchar *this = basic_ofstream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ofstream_wchar_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ofstream_wchar_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?close@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?close@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ?close@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?close@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_close, 4)
void __thiscall basic_ofstream_wchar_close(basic_ofstream_wchar *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_wchar_close(&this->filebuf)) {
        basic_ios_wchar *basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?is_open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEBA_NXZ */
/* ?is_open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_is_open, 4)
bool __thiscall basic_ofstream_wchar_is_open(const basic_ofstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_wchar_is_open(&this->filebuf);
}

/* ?open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QAEXPBDHH@Z */
/* ?open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEBDHH@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAEXPBDHH@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAAXPEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_open, 16)
void __thiscall basic_ofstream_wchar_open(basic_ofstream_wchar *this,
        const char *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, prot);

    if(!basic_filebuf_wchar_open(&this->filebuf, name, mode|OPENMODE_out, prot)) {
        basic_ios_wchar *basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QAEXPBDI@Z */
/* ?open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEBDI@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAEXPBDI@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAAXPEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_open_old, 12)
void __thiscall basic_ofstream_wchar_open_old(basic_ofstream_wchar *this,
        const char *name, unsigned int mode)
{
    basic_ofstream_wchar_open(this, name, mode, _SH_DENYNO);
}

/* ?open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QAEXPBGHH@Z */
/* ?open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEBGHH@Z */
/* ?open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QAEXPB_WHH@Z */
/* ?open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEB_WHH@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAEXPBGHH@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAAXPEBGHH@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAEXPB_WHH@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAAXPEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_open_wchar, 16)
void __thiscall basic_ofstream_wchar_open_wchar(basic_ofstream_wchar *this,
        const wchar_t *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, debugstr_w(name), mode, prot);

    if(!basic_filebuf_wchar_open_wchar(&this->filebuf, name, mode|OPENMODE_out, prot)) {
        basic_ios_wchar *basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QAEXPBGI@Z */
/* ?open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEBGI@Z */
/* ?open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QAEXPB_WI@Z */
/* ?open@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEB_WI@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAEXPBGI@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAAXPEBGI@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QAEXPB_WI@Z */
/* ?open@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEAAXPEB_WI@Z */
DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_open_wchar_old, 12)
void __thiscall basic_ofstream_wchar_open_wchar_old(basic_ofstream_wchar *this,
        const wchar_t *name, unsigned int mode)
{
    basic_ofstream_wchar_open_wchar(this, name, mode, _SH_DENYNO);
}

/* ?rdbuf@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QBEPAV?$basic_filebuf@_WU?$char_traits@_W@std@@@2@XZ */
/* ?rdbuf@?$basic_ofstream@_WU?$char_traits@_W@std@@@std@@QEBAPEAV?$basic_filebuf@_WU?$char_traits@_W@std@@@2@XZ */
/* ?rdbuf@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QBEPAV?$basic_filebuf@GU?$char_traits@G@std@@@2@XZ */
/* ?rdbuf@?$basic_ofstream@GU?$char_traits@G@std@@@std@@QEBAPEAV?$basic_filebuf@GU?$char_traits@G@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ofstream_wchar_rdbuf, 4)
basic_filebuf_wchar* __thiscall basic_ofstream_wchar_rdbuf(const basic_ofstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_wchar*)&this->filebuf;
}

static inline basic_ios_char* basic_ifstream_char_to_basic_ios(basic_ifstream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_ifstream_char_vbtable[1]);
}

static inline basic_ifstream_char* basic_ifstream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_ifstream_char*)((char*)ptr-basic_ifstream_char_vbtable[1]);
}

/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAE@XZ */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_ctor, 8)
basic_ifstream_char* __thiscall basic_ifstream_char_ctor(basic_ifstream_char *this, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %d)\n", this, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ifstream_char_vbtable;
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
    }

    basic_filebuf_char_ctor(&this->filebuf);
    basic_istream_char_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ifstream_char_vtable;
    return this;
}

/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_ctor_file, 12)
basic_ifstream_char* __thiscall basic_ifstream_char_ctor_file(
        basic_ifstream_char *this, FILE *file, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d)\n", this, file, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ifstream_char_vbtable;
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
    }

    basic_filebuf_char_ctor_file(&this->filebuf, file);
    basic_istream_char_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ifstream_char_vtable;
    return this;
}

/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAE@PBDHH@Z */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAA@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_ctor_name, 20)
basic_ifstream_char* __thiscall basic_ifstream_char_ctor_name(basic_ifstream_char *this,
        const char *name, int mode, int prot, bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, name, mode, prot, virt_init);

    basic_ifstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open(&this->filebuf, name, mode|OPENMODE_in, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAE@PBDH@Z */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAA@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_ctor_name_old, 16)
basic_ifstream_char* __thiscall basic_ifstream_char_ctor_name_old(basic_ifstream_char *this,
        const char *name, int mode, bool virt_init)
{
    return basic_ifstream_char_ctor_name(this, name, mode, _SH_DENYNO, virt_init);
}

/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAA@PEBGHH@Z */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAE@PB_WHH@Z */
/* ??0?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAA@PEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_ctor_name_wchar, 20)
basic_ifstream_char* __thiscall basic_ifstream_char_ctor_name_wchar(basic_ifstream_char *this,
        const wchar_t *name, int mode, int prot, bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, debugstr_w(name), mode, prot, virt_init);

    basic_ifstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open_wchar(&this->filebuf, name, mode|OPENMODE_in, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??1?$basic_ifstream@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_ifstream@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_dtor, 4)
void __thiscall basic_ifstream_char_dtor(basic_ios_char *base)
{
    basic_ifstream_char *this = basic_ifstream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_istream_char_dtor(basic_istream_char_to_basic_ios(&this->base));
    basic_filebuf_char_dtor(&this->filebuf);
}

/* ??_D?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_D?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_vbase_dtor, 4)
void __thiscall basic_ifstream_char_vbase_dtor(basic_ifstream_char *this)
{
    basic_ios_char *base = basic_ifstream_char_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_ifstream_char_dtor(base);
    basic_ios_char_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_ifstream_char_vector_dtor, 8)
basic_ifstream_char* __thiscall basic_ifstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_ifstream_char *this = basic_ifstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ifstream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ifstream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?close@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?close@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_close, 4)
void __thiscall basic_ifstream_char_close(basic_ifstream_char *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_char_close(&this->filebuf)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?is_open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_is_open, 4)
bool __thiscall basic_ifstream_char_is_open(const basic_ifstream_char *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_char_is_open(&this->filebuf);
}

/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPBDHH@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_open, 16)
void __thiscall basic_ifstream_char_open(basic_ifstream_char *this,
        const char *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, prot);

    if(!basic_filebuf_char_open(&this->filebuf, name, mode|OPENMODE_in, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPBDI@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_open_old, 12)
void __thiscall basic_ifstream_char_open_old(basic_ifstream_char *this,
        const char *name, unsigned int mode)
{
    basic_ifstream_char_open(this, name, mode, _SH_DENYNO);
}

/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPBGHH@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEBGHH@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPB_WHH@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_open_wchar, 16)
void __thiscall basic_ifstream_char_open_wchar(basic_ifstream_char *this,
        const wchar_t *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, debugstr_w(name), mode, prot);

    if(!basic_filebuf_char_open_wchar(&this->filebuf, name, mode|OPENMODE_in, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPBGI@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEBGI@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QAEXPB_WI@Z */
/* ?open@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEAAXPEB_WI@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_open_wchar_old, 12)
void __thiscall basic_ifstream_char_open_wchar_old(basic_ifstream_char *this,
        const wchar_t *name, unsigned int mode)
{
    basic_ifstream_char_open_wchar(this, name, mode, _SH_DENYNO);
}

/* ?rdbuf@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
/* ?rdbuf@?$basic_ifstream@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_char_rdbuf, 4)
basic_filebuf_char* __thiscall basic_ifstream_char_rdbuf(const basic_ifstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_char*)&this->filebuf;
}

static inline basic_ios_wchar* basic_ifstream_wchar_to_basic_ios(basic_ifstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_ifstream_wchar_vbtable[1]);
}

static inline basic_ifstream_wchar* basic_ifstream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_ifstream_wchar*)((char*)ptr-basic_ifstream_wchar_vbtable[1]);
}

/* ??0?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAE@XZ */
/* ??0?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_ctor, 8)
basic_ifstream_wchar* __thiscall basic_ifstream_wchar_ctor(basic_ifstream_wchar *this, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d)\n", this, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ifstream_wchar_vbtable;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
    }

    basic_filebuf_wchar_ctor(&this->filebuf);
    basic_istream_wchar_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ifstream_wchar_vtable;
    return this;
}

/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAE@XZ */
/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_ctor, 8)
basic_ifstream_wchar* __thiscall basic_ifstream_short_ctor(basic_ifstream_wchar *this, bool virt_init)
{
    basic_ifstream_wchar_ctor(this, virt_init);
    basic_istream_wchar_get_basic_ios(&this->base)->base.vtable = &basic_ifstream_short_vtable;
    return this;
}

/* ??0?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_ctor_file, 12)
basic_ifstream_wchar* __thiscall basic_ifstream_wchar_ctor_file(
        basic_ifstream_wchar *this, FILE *file, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d)\n", this, file, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ifstream_wchar_vbtable;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
    }

    basic_filebuf_wchar_ctor_file(&this->filebuf, file);
    basic_istream_wchar_ctor(&this->base, &this->filebuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ifstream_wchar_vtable;
    return this;
}

/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_ctor_file, 12)
basic_ifstream_wchar* __thiscall basic_ifstream_short_ctor_file(
        basic_ifstream_wchar *this, FILE *file, bool virt_init)
{
    basic_ifstream_wchar_ctor_file(this, file, virt_init);
    basic_istream_wchar_get_basic_ios(&this->base)->base.vtable = &basic_ifstream_short_vtable;
    return this;
}

/* ??0?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAE@PBDHH@Z */
/* ??0?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAA@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_ctor_name, 20)
basic_ifstream_wchar* __thiscall basic_ifstream_wchar_ctor_name(basic_ifstream_wchar *this,
        const char *name, int mode, int prot, bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, name, mode, prot, virt_init);

    basic_ifstream_wchar_ctor(this, virt_init);

    if(!basic_filebuf_wchar_open(&this->filebuf, name, mode|OPENMODE_in, prot)) {
        basic_ios_wchar *basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAE@PBDHH@Z */
/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAA@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_ctor_name, 20)
basic_ifstream_wchar* __thiscall basic_ifstream_short_ctor_name(basic_ifstream_wchar *this,
        const char *name, int mode, int prot, bool virt_init)
{
    basic_ifstream_wchar_ctor_name(this, name, mode, prot, virt_init);
    basic_istream_wchar_get_basic_ios(&this->base)->base.vtable = &basic_ifstream_short_vtable;
    return this;
}

/* ??0?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAE@PBDH@Z */
/* ??0?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAA@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_ctor_name_old, 16)
basic_ifstream_wchar* __thiscall basic_ifstream_wchar_ctor_name_old(basic_ifstream_wchar *this,
        const char *name, int mode, bool virt_init)
{
    return basic_ifstream_wchar_ctor_name(this, name, mode, _SH_DENYNO, virt_init);
}

/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAE@PBDH@Z */
/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAA@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_ctor_name_old, 16)
basic_ifstream_wchar* __thiscall basic_ifstream_short_ctor_name_old(basic_ifstream_wchar *this,
        const char *name, int mode, bool virt_init)
{
    return basic_ifstream_short_ctor_name(this, name, mode, _SH_DENYNO, virt_init);
}

/* ??0?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAA@PEBGHH@Z */
/* ??0?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAE@PB_WHH@Z */
/* ??0?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAA@PEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_ctor_name_wchar, 20)
basic_ifstream_wchar* __thiscall basic_ifstream_wchar_ctor_name_wchar(basic_ifstream_wchar *this,
        const wchar_t *name, int mode, int prot, bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, debugstr_w(name), mode, prot, virt_init);

    basic_ifstream_wchar_ctor(this, virt_init);

    if(!basic_filebuf_wchar_open_wchar(&this->filebuf, name, mode|OPENMODE_in, prot)) {
        basic_ios_wchar *basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAA@PEBGHH@Z */
/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAE@PB_WHH@Z */
/* ??0?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAA@PEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_short_ctor_name_wchar, 20)
basic_ifstream_wchar* __thiscall basic_ifstream_short_ctor_name_wchar(basic_ifstream_wchar *this,
        const wchar_t *name, int mode, int prot, bool virt_init)
{
    basic_ifstream_wchar_ctor_name_wchar(this, name, mode, prot, virt_init);
    basic_istream_wchar_get_basic_ios(&this->base)->base.vtable = &basic_ifstream_short_vtable;
    return this;
}

/* ??1?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
/* ??1?$basic_ifstream@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_ifstream@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_dtor, 4)
void __thiscall basic_ifstream_wchar_dtor(basic_ios_wchar *base)
{
    basic_ifstream_wchar *this = basic_ifstream_wchar_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_istream_wchar_dtor(basic_istream_wchar_to_basic_ios(&this->base));
    basic_filebuf_wchar_dtor(&this->filebuf);
}

/* ??_D?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ??_D?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ??_D?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_D?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_vbase_dtor, 4)
void __thiscall basic_ifstream_wchar_vbase_dtor(basic_ifstream_wchar *this)
{
    basic_ios_wchar *base = basic_ifstream_wchar_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_ifstream_wchar_dtor(base);
    basic_ios_wchar_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_vector_dtor, 8)
basic_ifstream_wchar* __thiscall basic_ifstream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_ifstream_wchar *this = basic_ifstream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ifstream_wchar_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ifstream_wchar_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?close@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?close@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ?close@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?close@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_close, 4)
void __thiscall basic_ifstream_wchar_close(basic_ifstream_wchar *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_wchar_close(&this->filebuf)) {
        basic_ios_wchar *basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?is_open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEBA_NXZ */
/* ?is_open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_is_open, 4)
bool __thiscall basic_ifstream_wchar_is_open(const basic_ifstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_wchar_is_open(&this->filebuf);
}

/* ?open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAEXPBDHH@Z */
/* ?open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEBDHH@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAEXPBDHH@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAAXPEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_open, 16)
void __thiscall basic_ifstream_wchar_open(basic_ifstream_wchar *this,
        const char *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, prot);

    if(!basic_filebuf_wchar_open(&this->filebuf, name, mode|OPENMODE_in, prot)) {
        basic_ios_wchar *basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAEXPBDI@Z */
/* ?open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEBDI@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAEXPBDI@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAAXPEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_open_old, 12)
void __thiscall basic_ifstream_wchar_open_old(basic_ifstream_wchar *this,
        const char *name, unsigned int mode)
{
    basic_ifstream_wchar_open(this, name, mode, _SH_DENYNO);
}

/* ?open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAEXPBGHH@Z */
/* ?open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEBGHH@Z */
/* ?open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAEXPB_WHH@Z */
/* ?open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEB_WHH@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAEXPBGHH@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAAXPEBGHH@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAEXPB_WHH@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAAXPEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_open_wchar, 16)
void __thiscall basic_ifstream_wchar_open_wchar(basic_ifstream_wchar *this,
        const wchar_t *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, debugstr_w(name), mode, prot);

    if(!basic_filebuf_wchar_open_wchar(&this->filebuf, name, mode|OPENMODE_in, prot)) {
        basic_ios_wchar *basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAEXPBGI@Z */
/* ?open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEBGI@Z */
/* ?open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QAEXPB_WI@Z */
/* ?open@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEB_WI@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAEXPBGI@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAAXPEBGI@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QAEXPB_WI@Z */
/* ?open@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEAAXPEB_WI@Z */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_open_wchar_old, 12)
void __thiscall basic_ifstream_wchar_open_wchar_old(basic_ifstream_wchar *this,
        const wchar_t *name, unsigned int mode)
{
    basic_ifstream_wchar_open_wchar(this, name, mode, _SH_DENYNO);
}

/* ?rdbuf@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QBEPAV?$basic_filebuf@_WU?$char_traits@_W@std@@@2@XZ */
/* ?rdbuf@?$basic_ifstream@_WU?$char_traits@_W@std@@@std@@QEBAPEAV?$basic_filebuf@_WU?$char_traits@_W@std@@@2@XZ */
/* ?rdbuf@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QBEPAV?$basic_filebuf@GU?$char_traits@G@std@@@2@XZ */
/* ?rdbuf@?$basic_ifstream@GU?$char_traits@G@std@@@std@@QEBAPEAV?$basic_filebuf@GU?$char_traits@G@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ifstream_wchar_rdbuf, 4)
basic_filebuf_wchar* __thiscall basic_ifstream_wchar_rdbuf(const basic_ifstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_wchar*)&this->filebuf;
}

static inline basic_ios_char* basic_fstream_char_to_basic_ios(basic_fstream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_fstream_char_vbtable1[1]);
}

static inline basic_fstream_char* basic_fstream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_fstream_char*)((char*)ptr-basic_fstream_char_vbtable1[1]);
}

/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@XZ */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_ctor, 8)
basic_fstream_char* __thiscall basic_fstream_char_ctor(basic_fstream_char *this, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %d)\n", this, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_fstream_char_vbtable1;
        this->base.base2.vbtable = basic_fstream_char_vbtable2;
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
    }

    basic_filebuf_char_ctor(&this->filebuf);
    basic_iostream_char_ctor(&this->base, &this->filebuf.base, FALSE);
    basic_ios->base.vtable = &basic_fstream_char_vtable;
    return this;
}

/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_ctor_file, 12)
basic_fstream_char* __thiscall basic_fstream_char_ctor_file(basic_fstream_char *this,
        FILE *file, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d)\n", this, file, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_fstream_char_vbtable1;
        this->base.base2.vbtable = basic_fstream_char_vbtable2;
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
    }

    basic_filebuf_char_ctor_file(&this->filebuf, file);
    basic_iostream_char_ctor(&this->base, &this->filebuf.base, FALSE);
    basic_ios->base.vtable = &basic_fstream_char_vtable;
    return this;
}

/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@PBDHH@Z */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@PEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_ctor_name, 20)
basic_fstream_char* __thiscall basic_fstream_char_ctor_name(basic_fstream_char *this,
        const char *name, int mode, int prot, bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, name, mode, prot, virt_init);

    basic_fstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open(&this->filebuf, name, mode, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@PBDH@Z */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_ctor_name_noprot, 16)
basic_fstream_char* __thiscall basic_fstream_char_ctor_name_noprot(basic_fstream_char *this,
        const char *name, int mode, bool virt_init)
{
    return basic_fstream_char_ctor_name(this, name, mode, _SH_DENYNO, virt_init);
}

/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@PEBGHH@Z */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QAE@PB_WHH@Z */
/* ??0?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAA@PEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_ctor_name_wchar, 20)
basic_fstream_char* __thiscall basic_fstream_char_ctor_name_wchar(basic_fstream_char *this,
        const wchar_t *name, int mode, int prot, bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, debugstr_w(name), mode, prot, virt_init);

    basic_fstream_char_ctor(this, virt_init);

    if(!basic_filebuf_char_open_wchar(&this->filebuf, name, mode, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??1?$basic_fstream@DU?$char_traits@D@std@@@std@@UAE@XZ */
/* ??1?$basic_fstream@DU?$char_traits@D@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_dtor, 4)
void __thiscall basic_fstream_char_dtor(basic_ios_char *base)
{
    basic_fstream_char *this = basic_fstream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_iostream_char_dtor(basic_iostream_char_to_basic_ios(&this->base));
    basic_filebuf_char_dtor(&this->filebuf);
}

/* ??_D?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ??_D?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_vbase_dtor, 4)
void __thiscall basic_fstream_char_vbase_dtor(basic_fstream_char *this)
{
    basic_ios_char *base = basic_fstream_char_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_fstream_char_dtor(base);
    basic_ios_char_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_fstream_char_vector_dtor, 8)
basic_fstream_char* __thiscall basic_fstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_fstream_char *this = basic_fstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_fstream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_fstream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?close@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXXZ */
/* ?close@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_close, 4)
void __thiscall basic_fstream_char_close(basic_fstream_char *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_char_close(&this->filebuf)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?is_open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_is_open, 4)
bool __thiscall basic_fstream_char_is_open(const basic_fstream_char *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_char_is_open(&this->filebuf);
}

/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPBDHH@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_open, 16)
void __thiscall basic_fstream_char_open(basic_fstream_char *this,
        const char *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, prot);

    if(!basic_filebuf_char_open(&this->filebuf, name, mode, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPBDI@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_open_old, 12)
void __thiscall basic_fstream_char_open_old(basic_fstream_char *this,
        const char *name, unsigned int mode)
{
    basic_fstream_char_open(this, name, mode, _SH_DENYNO);
}

/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPBGHH@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEBGHH@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPB_WHH@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_open_wchar, 16)
void __thiscall basic_fstream_char_open_wchar(basic_fstream_char *this,
        const wchar_t *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, debugstr_w(name), mode, prot);

    if(!basic_filebuf_char_open_wchar(&this->filebuf, name, mode, prot)) {
        basic_ios_char *basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        basic_ios_char_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPBGI@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEBGI@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QAEXPB_WI@Z */
/* ?open@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEAAXPEB_WI@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_open_wchar_old, 12)
void __thiscall basic_fstream_char_open_wchar_old(basic_fstream_char *this,
        const wchar_t *name, unsigned int mode)
{
    basic_fstream_char_open_wchar(this, name, mode, _SH_DENYNO);
}

/* ?rdbuf@?$basic_fstream@DU?$char_traits@D@std@@@std@@QBEPAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
/* ?rdbuf@?$basic_fstream@DU?$char_traits@D@std@@@std@@QEBAPEAV?$basic_filebuf@DU?$char_traits@D@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_char_rdbuf, 4)
basic_filebuf_char* __thiscall basic_fstream_char_rdbuf(const basic_fstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_char*)&this->filebuf;
}

static inline basic_ios_wchar* basic_fstream_wchar_to_basic_ios(basic_fstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_fstream_wchar_vbtable1[1]);
}

static inline basic_fstream_wchar* basic_fstream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_fstream_wchar*)((char*)ptr-basic_fstream_wchar_vbtable1[1]);
}

/* ??0?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAE@XZ */
/* ??0?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_ctor, 8)
basic_fstream_wchar* __thiscall basic_fstream_wchar_ctor(basic_fstream_wchar *this, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d)\n", this, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_fstream_wchar_vbtable1;
        this->base.base2.vbtable = basic_fstream_wchar_vbtable2;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
    }

    basic_filebuf_wchar_ctor(&this->filebuf);
    basic_iostream_wchar_ctor(&this->base, &this->filebuf.base, FALSE);
    basic_ios->base.vtable = &basic_fstream_wchar_vtable;
    return this;
}

/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QAE@XZ */
/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_ctor, 8)
basic_fstream_wchar* __thiscall basic_fstream_short_ctor(basic_fstream_wchar *this, bool virt_init)
{
    basic_fstream_wchar_ctor(this, virt_init);
    basic_istream_wchar_get_basic_ios(&this->base.base1)->base.vtable = &basic_fstream_short_vtable;
    return this;
}

/* ??0?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_ctor_file, 12)
basic_fstream_wchar* __thiscall basic_fstream_wchar_ctor_file(basic_fstream_wchar *this,
        FILE *file, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d)\n", this, file, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_fstream_wchar_vbtable1;
        this->base.base2.vbtable = basic_fstream_wchar_vbtable2;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
    }

    basic_filebuf_wchar_ctor_file(&this->filebuf, file);
    basic_iostream_wchar_ctor(&this->base, &this->filebuf.base, FALSE);
    basic_ios->base.vtable = &basic_fstream_wchar_vtable;
    return this;
}

/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QAE@PAU_iobuf@@@Z */
/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAA@PEAU_iobuf@@@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_ctor_file, 12)
basic_fstream_wchar* __thiscall basic_fstream_short_ctor_file(basic_fstream_wchar *this,
        FILE *file, bool virt_init)
{
    basic_fstream_wchar_ctor_file(this, file, virt_init);
    basic_istream_wchar_get_basic_ios(&this->base.base1)->base.vtable = &basic_fstream_short_vtable;
    return this;
}

/* ??0?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAE@PB_WHH@Z */
/* ??0?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEAA@PEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_ctor_name, 20)
basic_fstream_wchar* __thiscall basic_fstream_wchar_ctor_name(basic_fstream_wchar *this,
        const char *name, int mode, int prot, bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, name, mode, prot, virt_init);

    basic_fstream_wchar_ctor(this, virt_init);

    if(!basic_filebuf_wchar_open(&this->filebuf, name, mode, prot)) {
        basic_ios_wchar *basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAA@PEBGHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_ctor_name, 20)
basic_fstream_wchar* __thiscall basic_fstream_short_ctor_name(basic_fstream_wchar *this,
        const char *name, int mode, int prot, bool virt_init)
{
    basic_fstream_wchar_ctor_name(this, name, mode, prot, virt_init);
    basic_istream_wchar_get_basic_ios(&this->base.base1)->base.vtable = &basic_fstream_short_vtable;
    return this;
}

/* ??0?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAE@PBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_ctor_name_noprot, 16)
basic_fstream_wchar* __thiscall basic_fstream_wchar_ctor_name_noprot(basic_fstream_wchar *this,
        const char *name, int mode, bool virt_init)
{
    return basic_fstream_wchar_ctor_name(this, name, mode, _SH_DENYNO, virt_init);
}

/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QAE@PBDH@Z */
/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAA@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_ctor_name_noprot, 16)
basic_fstream_wchar* __thiscall basic_fstream_short_ctor_name_noprot(basic_fstream_wchar *this,
        const char *name, int mode, bool virt_init)
{
    return basic_fstream_short_ctor_name(this, name, mode, _SH_DENYNO, virt_init);
}

/* ??0?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEAA@PEBGHH@Z */
/* ??0?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAE@PB_WHH@Z */
/* ??0?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEAA@PEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_ctor_name_wchar, 20)
basic_fstream_wchar* __thiscall basic_fstream_wchar_ctor_name_wchar(basic_fstream_wchar *this,
        const wchar_t *name, int mode, int prot, bool virt_init)
{
    TRACE("(%p %s %d %d %d)\n", this, debugstr_w(name), mode, prot, virt_init);

    basic_fstream_wchar_ctor(this, virt_init);

    if(!basic_filebuf_wchar_open_wchar(&this->filebuf, name, mode, prot)) {
        basic_ios_wchar *basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
    return this;
}

/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QAE@PBGHH@Z */
/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAA@PEBGHH@Z */
/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QAE@PB_WHH@Z */
/* ??0?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAA@PEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_short_ctor_name_wchar, 20)
basic_fstream_wchar* __thiscall basic_fstream_short_ctor_name_wchar(basic_fstream_wchar *this,
        const wchar_t *name, int mode, int prot, bool virt_init)
{
    basic_fstream_wchar_ctor_name_wchar(this, name, mode, prot, virt_init);
    basic_istream_wchar_get_basic_ios(&this->base.base1)->base.vtable = &basic_fstream_short_vtable;
    return this;
}

/* ??1?$basic_fstream@_WU?$char_traits@_W@std@@@std@@UAE@XZ */
/* ??1?$basic_fstream@_WU?$char_traits@_W@std@@@std@@UEAA@XZ */
/* ??1?$basic_fstream@GU?$char_traits@G@std@@@std@@UAE@XZ */
/* ??1?$basic_fstream@GU?$char_traits@G@std@@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_dtor, 4)
void __thiscall basic_fstream_wchar_dtor(basic_ios_wchar *base)
{
    basic_fstream_wchar *this = basic_fstream_wchar_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_iostream_wchar_dtor(basic_iostream_wchar_to_basic_ios(&this->base));
    basic_filebuf_wchar_dtor(&this->filebuf);
}

/* ??_D?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ??_D?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ??_D?$basic_fstream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ??_D?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_vbase_dtor, 4)
void __thiscall basic_fstream_wchar_vbase_dtor(basic_fstream_wchar *this)
{
    basic_ios_wchar *base = basic_fstream_wchar_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_fstream_wchar_dtor(base);
    basic_ios_wchar_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_vector_dtor, 8)
basic_fstream_wchar* __thiscall basic_fstream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_fstream_wchar *this = basic_fstream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_fstream_wchar_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_fstream_wchar_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?close@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAEXXZ */
/* ?close@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEAAXXZ */
/* ?close@?$basic_fstream@GU?$char_traits@G@std@@@std@@QAEXXZ */
/* ?close@?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_close, 4)
void __thiscall basic_fstream_wchar_close(basic_fstream_wchar *this)
{
    TRACE("(%p)\n", this);

    if(!basic_filebuf_wchar_close(&this->filebuf)) {
        basic_ios_wchar *basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?is_open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEBA_NXZ */
/* ?is_open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QBE_NXZ */
/* ?is_open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_is_open, 4)
bool __thiscall basic_fstream_wchar_is_open(const basic_fstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return basic_filebuf_wchar_is_open(&this->filebuf);
}

/* ?open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAEXPBDHH@Z */
/* ?open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEBDHH@Z */
/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QAEXPBDHH@Z */
/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAAXPEBDHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_open, 16)
void __thiscall basic_fstream_wchar_open(basic_fstream_wchar *this,
        const char *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, name, mode, prot);

    if(!basic_filebuf_wchar_open(&this->filebuf, name, mode, prot)) {
        basic_ios_wchar *basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAEXPBDI@Z */
/* ?open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEBDI@Z */
/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QAEXPBDI@Z */
/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAAXPEBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_open_old, 12)
void __thiscall basic_fstream_wchar_open_old(basic_fstream_wchar *this,
        const char *name, unsigned int mode)
{
    basic_fstream_wchar_open(this, name, mode, _SH_DENYNO);
}

/* ?open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAEXPBGHH@Z */
/* ?open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEBGHH@Z */
/* ?open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAEXPB_WHH@Z */
/* ?open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEB_WHH@Z */
/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QAEXPBGHH@Z */
/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAAXPEBGHH@Z */
/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QAEXPB_WHH@Z */
/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAAXPEB_WHH@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_open_wchar, 16)
void __thiscall basic_fstream_wchar_open_wchar(basic_fstream_wchar *this,
        const wchar_t *name, int mode, int prot)
{
    TRACE("(%p %s %d %d)\n", this, debugstr_w(name), mode, prot);

    if(!basic_filebuf_wchar_open_wchar(&this->filebuf, name, mode, prot)) {
        basic_ios_wchar *basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
        basic_ios_wchar_setstate(basic_ios, IOSTATE_failbit);
    }
}

/* ?open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAEXPBGI@Z */
/* ?open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEBGI@Z */
/* ?open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QAEXPB_WI@Z */
/* ?open@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEAAXPEB_WI@Z */
/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QAEXPBGI@Z */
/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAAXPEBGI@Z */
/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QAEXPB_WI@Z */
/* ?open@?$basic_fstream@GU?$char_traits@G@std@@@std@@QEAAXPEB_WI@Z */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_open_wchar_old, 12)
void __thiscall basic_fstream_wchar_open_wchar_old(basic_fstream_wchar *this,
        const wchar_t *name, unsigned int mode)
{
    basic_fstream_wchar_open_wchar(this, name, mode, _SH_DENYNO);
}

/* ?rdbuf@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QBEPAV?$basic_filebuf@_WU?$char_traits@_W@std@@@2@XZ */
/* ?rdbuf@?$basic_fstream@_WU?$char_traits@_W@std@@@std@@QEBAPEAV?$basic_filebuf@_WU?$char_traits@_W@std@@@2@XZ */
/* ?rdbuf@?$basic_fstream@GU?$char_traits@G@std@@@std@@QBEPAV?$basic_filebuf@GU?$char_traits@G@std@@@2@XZ */
/* ?rdbuf@?$basic_fstream@GU?$char_traits@G@std@@@std@@QEBAPEAV?$basic_filebuf@GU?$char_traits@G@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_fstream_wchar_rdbuf, 4)
basic_filebuf_wchar* __thiscall basic_fstream_wchar_rdbuf(const basic_fstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_filebuf_wchar*)&this->filebuf;
}

static inline basic_ios_char* basic_ostringstream_char_to_basic_ios(basic_ostringstream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_ostringstream_char_vbtable[1]);
}

static inline basic_ostringstream_char* basic_ostringstream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_ostringstream_char*)((char*)ptr-basic_ostringstream_char_vbtable[1]);
}

/* ??0?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
/* ??0?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_ctor_str, 16)
basic_ostringstream_char* __thiscall basic_ostringstream_char_ctor_str(basic_ostringstream_char *this,
        const basic_string_char *str, int mode, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ostringstream_char_vbtable;
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
    }

    basic_stringbuf_char_ctor_str(&this->strbuf, str, mode|OPENMODE_out);
    basic_ostream_char_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ostringstream_char_vtable;
    return this;
}

/* ??0?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z */
/* ??0?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_ctor_mode, 12)
basic_ostringstream_char* __thiscall basic_ostringstream_char_ctor_mode(
        basic_ostringstream_char *this, int mode, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ostringstream_char_vbtable;
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
    }

    basic_stringbuf_char_ctor_mode(&this->strbuf, mode|OPENMODE_out);
    basic_ostream_char_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ostringstream_char_vtable;
    return this;
}

/* ??_F?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_F?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_ctor, 4)
basic_ostringstream_char* __thiscall basic_ostringstream_char_ctor(
        basic_ostringstream_char *this)
{
    return basic_ostringstream_char_ctor_mode(this, 0, TRUE);
}

/* ??1?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UAE@XZ */
/* ??1?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_dtor, 4)
void __thiscall basic_ostringstream_char_dtor(basic_ios_char *base)
{
    basic_ostringstream_char *this = basic_ostringstream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_stringbuf_char_dtor(&this->strbuf);
    basic_ostream_char_dtor(basic_ostream_char_to_basic_ios(&this->base));
}

/* ??_D?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_D?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_vbase_dtor, 4)
void __thiscall basic_ostringstream_char_vbase_dtor(basic_ostringstream_char *this)
{
    basic_ios_char *base = basic_ostringstream_char_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_ostringstream_char_dtor(base);
    basic_ios_char_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_vector_dtor, 8)
basic_ostringstream_char* __thiscall basic_ostringstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_ostringstream_char *this = basic_ostringstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ostringstream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ostringstream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPAV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?rdbuf@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEAV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_rdbuf, 4)
basic_stringbuf_char* __thiscall basic_ostringstream_char_rdbuf(const basic_ostringstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_char*)&this->strbuf;
}

/* ?str@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
/* ?str@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_str_set, 8)
void __thiscall basic_ostringstream_char_str_set(basic_ostringstream_char *this, const basic_string_char *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_char_str_set(&this->strbuf, str);
}

/* ?str@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?str@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_char_str_get, 8)
basic_string_char* __thiscall basic_ostringstream_char_str_get(const basic_ostringstream_char *this, basic_string_char *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_char_str_get(&this->strbuf, ret);
}

static inline basic_ios_wchar* basic_ostringstream_wchar_to_basic_ios(basic_ostringstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_ostringstream_wchar_vbtable[1]);
}

static inline basic_ostringstream_wchar* basic_ostringstream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_ostringstream_wchar*)((char*)ptr-basic_ostringstream_wchar_vbtable[1]);
}

/* ??0?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@ABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
/* ??0?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_ctor_str, 16)
basic_ostringstream_wchar* __thiscall basic_ostringstream_wchar_ctor_str(basic_ostringstream_wchar *this,
        const basic_string_wchar *str, int mode, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ostringstream_wchar_vbtable;
        basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
    }

    basic_stringbuf_wchar_ctor_str(&this->strbuf, str, mode|OPENMODE_out);
    basic_ostream_wchar_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ostringstream_wchar_vtable;
    return this;
}

/* ??0?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
/* ??0?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_short_ctor_str, 16)
basic_ostringstream_wchar* __thiscall basic_ostringstream_short_ctor_str(basic_ostringstream_wchar *this,
        const basic_string_wchar *str, int mode, bool virt_init)
{
    basic_ostringstream_wchar_ctor_str(this, str, mode, virt_init);
    basic_ostream_wchar_get_basic_ios(&this->base)->base.vtable = &basic_ostringstream_short_vtable;
    return this;
}

/* ??0?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@H@Z */
/* ??0?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_ctor_mode, 12)
basic_ostringstream_wchar* __thiscall basic_ostringstream_wchar_ctor_mode(
        basic_ostringstream_wchar *this, int mode, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_ostringstream_wchar_vbtable;
        basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_wchar_get_basic_ios(&this->base);
    }

    basic_stringbuf_wchar_ctor_mode(&this->strbuf, mode|OPENMODE_out);
    basic_ostream_wchar_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_ostringstream_wchar_vtable;
    return this;
}

/* ??0?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@H@Z */
/* ??0?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_short_ctor_mode, 12)
basic_ostringstream_wchar* __thiscall basic_ostringstream_short_ctor_mode(
        basic_ostringstream_wchar *this, int mode, bool virt_init)
{
    basic_ostringstream_wchar_ctor_mode(this, mode, virt_init);
    basic_ostream_wchar_get_basic_ios(&this->base)->base.vtable = &basic_ostringstream_short_vtable;
    return this;
}

/* ??_F?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_F?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_ctor, 4)
basic_ostringstream_wchar* __thiscall basic_ostringstream_wchar_ctor(
        basic_ostringstream_wchar *this)
{
    return basic_ostringstream_wchar_ctor_mode(this, 0, TRUE);
}

/* ??_F?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_F?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_short_ctor, 4)
basic_ostringstream_wchar* __thiscall basic_ostringstream_short_ctor(
        basic_ostringstream_wchar *this)
{
    return basic_ostringstream_short_ctor_mode(this, 0, TRUE);
}

/* ??1?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UAE@XZ */
/* ??1?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UEAA@XZ */
/* ??1?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UAE@XZ */
/* ??1?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_dtor, 4)
void __thiscall basic_ostringstream_wchar_dtor(basic_ios_wchar *base)
{
    basic_ostringstream_wchar *this = basic_ostringstream_wchar_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_stringbuf_wchar_dtor(&this->strbuf);
    basic_ostream_wchar_dtor(basic_ostream_wchar_to_basic_ios(&this->base));
}

/* ??_D?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_D?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
/* ??_D?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_D?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_vbase_dtor, 4)
void __thiscall basic_ostringstream_wchar_vbase_dtor(basic_ostringstream_wchar *this)
{
    basic_ios_wchar *base = basic_ostringstream_wchar_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_ostringstream_wchar_dtor(base);
    basic_ios_wchar_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_vector_dtor, 8)
basic_ostringstream_wchar* __thiscall basic_ostringstream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_ostringstream_wchar *this = basic_ostringstream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_ostringstream_wchar_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_ostringstream_wchar_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEPAV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?rdbuf@?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAPEAV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?rdbuf@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPAV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?rdbuf@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEAV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_rdbuf, 4)
basic_stringbuf_wchar* __thiscall basic_ostringstream_wchar_rdbuf(const basic_ostringstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_wchar*)&this->strbuf;
}

/* ?str@?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?str@?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?str@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
/* ?str@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_str_set, 8)
void __thiscall basic_ostringstream_wchar_str_set(basic_ostringstream_wchar *this, const basic_string_wchar *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_wchar_str_set(&this->strbuf, str);
}

/* ?str@?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?str@?$basic_ostringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?str@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?str@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_ostringstream_wchar_str_get, 8)
basic_string_wchar* __thiscall basic_ostringstream_wchar_str_get(const basic_ostringstream_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_wchar_str_get(&this->strbuf, ret);
}

static inline basic_ios_char* basic_istringstream_char_to_basic_ios(basic_istringstream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_istringstream_char_vbtable[1]);
}

static inline basic_istringstream_char* basic_istringstream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_istringstream_char*)((char*)ptr-basic_istringstream_char_vbtable[1]);
}

/* ??0?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
/* ??0?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_ctor_str, 16)
basic_istringstream_char* __thiscall basic_istringstream_char_ctor_str(basic_istringstream_char *this,
        const basic_string_char *str, int mode, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_istringstream_char_vbtable;
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
    }

    basic_stringbuf_char_ctor_str(&this->strbuf, str, mode|OPENMODE_in);
    basic_istream_char_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_istringstream_char_vtable;
    return this;
}

/* ??0?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z */
/* ??0?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_ctor_mode, 12)
basic_istringstream_char* __thiscall basic_istringstream_char_ctor_mode(
        basic_istringstream_char *this, int mode, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_istringstream_char_vbtable;
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base);
    }

    basic_stringbuf_char_ctor_mode(&this->strbuf, mode|OPENMODE_in);
    basic_istream_char_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_istringstream_char_vtable;
    return this;
}

/* ??_F?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_F?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_ctor, 4)
basic_istringstream_char* __thiscall basic_istringstream_char_ctor(
        basic_istringstream_char *this)
{
    return basic_istringstream_char_ctor_mode(this, 0, TRUE);
}

/* ??1?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UAE@XZ */
/* ??1?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_dtor, 4)
void __thiscall basic_istringstream_char_dtor(basic_ios_char *base)
{
    basic_istringstream_char *this = basic_istringstream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_stringbuf_char_dtor(&this->strbuf);
    basic_istream_char_dtor(basic_istream_char_to_basic_ios(&this->base));
}

/* ??_D?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_D?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_vbase_dtor, 4)
void __thiscall basic_istringstream_char_vbase_dtor(basic_istringstream_char *this)
{
    basic_ios_char *base = basic_istringstream_char_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_istringstream_char_dtor(base);
    basic_ios_char_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_istringstream_char_vector_dtor, 8)
basic_istringstream_char* __thiscall basic_istringstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_istringstream_char *this = basic_istringstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_istringstream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_istringstream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPAV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?rdbuf@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEAV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_rdbuf, 4)
basic_stringbuf_char* __thiscall basic_istringstream_char_rdbuf(const basic_istringstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_char*)&this->strbuf;
}

/* ?str@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
/* ?str@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_str_set, 8)
void __thiscall basic_istringstream_char_str_set(basic_istringstream_char *this, const basic_string_char *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_char_str_set(&this->strbuf, str);
}

/* ?str@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?str@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_char_str_get, 8)
basic_string_char* __thiscall basic_istringstream_char_str_get(const basic_istringstream_char *this, basic_string_char *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_char_str_get(&this->strbuf, ret);
}

static inline basic_ios_wchar* basic_istringstream_wchar_to_basic_ios(basic_istringstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_istringstream_wchar_vbtable[1]);
}

static inline basic_istringstream_wchar* basic_istringstream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_istringstream_wchar*)((char*)ptr-basic_istringstream_wchar_vbtable[1]);
}

/* ??0?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@ABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
/* ??0?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_ctor_str, 16)
basic_istringstream_wchar* __thiscall basic_istringstream_wchar_ctor_str(basic_istringstream_wchar *this,
        const basic_string_wchar *str, int mode, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_istringstream_wchar_vbtable;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
    }

    basic_stringbuf_wchar_ctor_str(&this->strbuf, str, mode|OPENMODE_in);
    basic_istream_wchar_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_istringstream_wchar_vtable;
    return this;
}

/* ??0?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
/* ??0?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_short_ctor_str, 16)
basic_istringstream_wchar* __thiscall basic_istringstream_short_ctor_str(basic_istringstream_wchar *this,
        const basic_string_wchar *str, int mode, bool virt_init)
{
    basic_istringstream_wchar_ctor_str(this, str, mode, virt_init);
    basic_istream_wchar_get_basic_ios(&this->base)->base.vtable = &basic_istringstream_short_vtable;
    return this;
}

/* ??0?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@H@Z */
/* ??0?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_ctor_mode, 12)
basic_istringstream_wchar* __thiscall basic_istringstream_wchar_ctor_mode(
        basic_istringstream_wchar *this, int mode, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.vbtable = basic_istringstream_wchar_vbtable;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base);
    }

    basic_stringbuf_wchar_ctor_mode(&this->strbuf, mode|OPENMODE_in);
    basic_istream_wchar_ctor(&this->base, &this->strbuf.base, FALSE, FALSE);
    basic_ios->base.vtable = &basic_istringstream_wchar_vtable;
    return this;
}

/* ??0?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@H@Z */
/* ??0?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_short_ctor_mode, 12)
basic_istringstream_wchar* __thiscall basic_istringstream_short_ctor_mode(
        basic_istringstream_wchar *this, int mode, bool virt_init)
{
    basic_istringstream_wchar_ctor_mode(this, mode, virt_init);
    basic_istream_wchar_get_basic_ios(&this->base)->base.vtable = &basic_istringstream_short_vtable;
    return this;
}

/* ??_F?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_F?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_ctor, 4)
basic_istringstream_wchar* __thiscall basic_istringstream_wchar_ctor(
        basic_istringstream_wchar *this)
{
    return basic_istringstream_wchar_ctor_mode(this, 0, TRUE);
}

/* ??_F?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_F?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_short_ctor, 4)
basic_istringstream_wchar* __thiscall basic_istringstream_short_ctor(
        basic_istringstream_wchar *this)
{
    return basic_istringstream_short_ctor_mode(this, 0, TRUE);
}

/* ??1?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UAE@XZ */
/* ??1?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UEAA@XZ */
/* ??1?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UAE@XZ */
/* ??1?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_dtor, 4)
void __thiscall basic_istringstream_wchar_dtor(basic_ios_wchar *base)
{
    basic_istringstream_wchar *this = basic_istringstream_wchar_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_stringbuf_wchar_dtor(&this->strbuf);
    basic_istream_wchar_dtor(basic_istream_wchar_to_basic_ios(&this->base));
}

/* ??_D?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_D?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
/* ??_D?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_D?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_vbase_dtor, 4)
void __thiscall basic_istringstream_wchar_vbase_dtor(basic_istringstream_wchar *this)
{
    basic_ios_wchar *base = basic_istringstream_wchar_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_istringstream_wchar_dtor(base);
    basic_ios_wchar_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_vector_dtor, 8)
basic_istringstream_wchar* __thiscall basic_istringstream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_istringstream_wchar *this = basic_istringstream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_istringstream_wchar_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_istringstream_wchar_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEPAV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?rdbuf@?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAPEAV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?rdbuf@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPAV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?rdbuf@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEAV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_rdbuf, 4)
basic_stringbuf_wchar* __thiscall basic_istringstream_wchar_rdbuf(const basic_istringstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_wchar*)&this->strbuf;
}

/* ?str@?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?str@?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?str@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
/* ?str@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_str_set, 8)
void __thiscall basic_istringstream_wchar_str_set(basic_istringstream_wchar *this, const basic_string_wchar *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_wchar_str_set(&this->strbuf, str);
}

/* ?str@?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?str@?$basic_istringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?str@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?str@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_istringstream_wchar_str_get, 8)
basic_string_wchar* __thiscall basic_istringstream_wchar_str_get(const basic_istringstream_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_wchar_str_get(&this->strbuf, ret);
}

static inline basic_ios_char* basic_stringstream_char_to_basic_ios(basic_stringstream_char *ptr)
{
    return (basic_ios_char*)((char*)ptr+basic_stringstream_char_vbtable1[1]);
}

static inline basic_stringstream_char* basic_stringstream_char_from_basic_ios(basic_ios_char *ptr)
{
    return (basic_stringstream_char*)((char*)ptr-basic_stringstream_char_vbtable1[1]);
}

/* ??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
/* ??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_ctor_str, 16)
basic_stringstream_char* __thiscall basic_stringstream_char_ctor_str(basic_stringstream_char *this,
        const basic_string_char *str, int mode, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_stringstream_char_vbtable1;
        this->base.base2.vbtable = basic_stringstream_char_vbtable2;
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
    }

    basic_stringbuf_char_ctor_str(&this->strbuf, str, mode);
    basic_iostream_char_ctor(&this->base, &this->strbuf.base, FALSE);
    basic_ios->base.vtable = &basic_stringstream_char_vtable;
    return this;
}

/* ??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z */
/* ??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_ctor_mode, 12)
basic_stringstream_char* __thiscall basic_stringstream_char_ctor_mode(
        basic_stringstream_char *this, int mode, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_stringstream_char_vbtable1;
        this->base.base2.vbtable = basic_stringstream_char_vbtable2;
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
    }

    basic_stringbuf_char_ctor_mode(&this->strbuf, mode);
    basic_iostream_char_ctor(&this->base, &this->strbuf.base, FALSE);
    basic_ios->base.vtable = &basic_stringstream_char_vtable;
    return this;
}

/* ??_F?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_F?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_ctor, 4)
basic_stringstream_char* __thiscall basic_stringstream_char_ctor(
        basic_stringstream_char *this)
{
    return basic_stringstream_char_ctor_mode(
            this, OPENMODE_out|OPENMODE_in, TRUE);
}

/* ??1?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UAE@XZ */
/* ??1?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_dtor, 4)
void __thiscall basic_stringstream_char_dtor(basic_ios_char *base)
{
    basic_stringstream_char *this = basic_stringstream_char_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_iostream_char_dtor(basic_iostream_char_to_basic_ios(&this->base));
    basic_stringbuf_char_dtor(&this->strbuf);
}

/* ??_D?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ??_D?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_vbase_dtor, 4)
void __thiscall basic_stringstream_char_vbase_dtor(basic_stringstream_char *this)
{
    basic_ios_char *base = basic_stringstream_char_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_stringstream_char_dtor(base);
    basic_ios_char_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_stringstream_char_vector_dtor, 8)
basic_stringstream_char* __thiscall basic_stringstream_char_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    basic_stringstream_char *this = basic_stringstream_char_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_stringstream_char_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_stringstream_char_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPAV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?rdbuf@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEAV?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_rdbuf, 4)
basic_stringbuf_char* __thiscall basic_stringstream_char_rdbuf(const basic_stringstream_char *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_char*)&this->strbuf;
}

/* ?str@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
/* ?str@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_str_set, 8)
void __thiscall basic_stringstream_char_str_set(basic_stringstream_char *this, const basic_string_char *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_char_str_set(&this->strbuf, str);
}

/* ?str@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?str@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_char_str_get, 8)
basic_string_char* __thiscall basic_stringstream_char_str_get(const basic_stringstream_char *this, basic_string_char *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_char_str_get(&this->strbuf, ret);
}

static inline basic_ios_wchar* basic_stringstream_wchar_to_basic_ios(basic_stringstream_wchar *ptr)
{
    return (basic_ios_wchar*)((char*)ptr+basic_stringstream_wchar_vbtable1[1]);
}

static inline basic_stringstream_wchar* basic_stringstream_wchar_from_basic_ios(basic_ios_wchar *ptr)
{
    return (basic_stringstream_wchar*)((char*)ptr-basic_stringstream_wchar_vbtable1[1]);
}

/* ??0?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@ABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
/* ??0?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@AEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_ctor_str, 16)
basic_stringstream_wchar* __thiscall basic_stringstream_wchar_ctor_str(basic_stringstream_wchar *this,
        const basic_string_wchar *str, int mode, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %p %d %d)\n", this, str, mode, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_stringstream_wchar_vbtable1;
        this->base.base2.vbtable = basic_stringstream_wchar_vbtable2;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
    }

    basic_stringbuf_wchar_ctor_str(&this->strbuf, str, mode);
    basic_iostream_wchar_ctor(&this->base, &this->strbuf.base, FALSE);
    basic_ios->base.vtable = &basic_stringstream_wchar_vtable;
    return this;
}

/* ??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
/* ??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_short_ctor_str, 16)
basic_stringstream_wchar* __thiscall basic_stringstream_short_ctor_str(basic_stringstream_wchar *this,
        const basic_string_wchar *str, int mode, bool virt_init)
{
    basic_stringstream_wchar_ctor_str(this, str, mode, virt_init);
    basic_istream_wchar_get_basic_ios(&this->base.base1)->base.vtable = &basic_stringstream_short_vtable;
    return this;
}

/* ??0?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@H@Z */
/* ??0?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_ctor_mode, 12)
basic_stringstream_wchar* __thiscall basic_stringstream_wchar_ctor_mode(
        basic_stringstream_wchar *this, int mode, bool virt_init)
{
    basic_ios_wchar *basic_ios;

    TRACE("(%p %d %d)\n", this, mode, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = basic_stringstream_wchar_vbtable1;
        this->base.base2.vbtable = basic_stringstream_wchar_vbtable2;
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_wchar_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_wchar_get_basic_ios(&this->base.base1);
    }

    basic_stringbuf_wchar_ctor_mode(&this->strbuf, mode);
    basic_iostream_wchar_ctor(&this->base, &this->strbuf.base, FALSE);
    basic_ios->base.vtable = &basic_stringstream_wchar_vtable;
    return this;
}

/* ??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@H@Z */
/* ??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@H@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_short_ctor_mode, 12)
basic_stringstream_wchar* __thiscall basic_stringstream_short_ctor_mode(
        basic_stringstream_wchar *this, int mode, bool virt_init)
{
    basic_stringstream_wchar_ctor_mode(this, mode, virt_init);
    basic_istream_wchar_get_basic_ios(&this->base.base1)->base.vtable = &basic_stringstream_short_vtable;
    return this;
}

/* ??_F?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_F?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_ctor, 4)
basic_stringstream_wchar* __thiscall basic_stringstream_wchar_ctor(
        basic_stringstream_wchar *this)
{
    return basic_stringstream_wchar_ctor_mode(
            this, OPENMODE_out|OPENMODE_in, TRUE);
}

/* ??_F?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_F?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_short_ctor, 4)
basic_stringstream_wchar* __thiscall basic_stringstream_short_ctor(
        basic_stringstream_wchar *this)
{
    return basic_stringstream_short_ctor_mode(
            this, OPENMODE_out|OPENMODE_in, TRUE);
}

/* ??1?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UAE@XZ */
/* ??1?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@UEAA@XZ */
/* ??1?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UAE@XZ */
/* ??1?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_dtor, 4)
void __thiscall basic_stringstream_wchar_dtor(basic_ios_wchar *base)
{
    basic_stringstream_wchar *this = basic_stringstream_wchar_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_iostream_wchar_dtor(basic_iostream_wchar_to_basic_ios(&this->base));
    basic_stringbuf_wchar_dtor(&this->strbuf);
}

/* ??_D?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ??_D?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
/* ??_D?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ??_D?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_vbase_dtor, 4)
void __thiscall basic_stringstream_wchar_vbase_dtor(basic_stringstream_wchar *this)
{
    basic_ios_wchar *base = basic_stringstream_wchar_to_basic_ios(this);

    TRACE("(%p)\n", this);

    basic_stringstream_wchar_dtor(base);
    basic_ios_wchar_dtor(base);
}

DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_vector_dtor, 8)
basic_stringstream_wchar* __thiscall basic_stringstream_wchar_vector_dtor(basic_ios_wchar *base, unsigned int flags)
{
    basic_stringstream_wchar *this = basic_stringstream_wchar_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            basic_stringstream_wchar_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        basic_stringstream_wchar_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?rdbuf@?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEPAV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?rdbuf@?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAPEAV?$basic_stringbuf@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?rdbuf@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPAV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?rdbuf@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEAV?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_rdbuf, 4)
basic_stringbuf_wchar* __thiscall basic_stringstream_wchar_rdbuf(const basic_stringstream_wchar *this)
{
    TRACE("(%p)\n", this);
    return (basic_stringbuf_wchar*)&this->strbuf;
}

/* ?str@?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?str@?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?str@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
/* ?str@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_str_set, 8)
void __thiscall basic_stringstream_wchar_str_set(basic_stringstream_wchar *this, const basic_string_wchar *str)
{
    TRACE("(%p %p)\n", this, str);
    basic_stringbuf_wchar_str_set(&this->strbuf, str);
}

/* ?str@?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?str@?$basic_stringstream@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?str@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?str@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_stringstream_wchar_str_get, 8)
basic_string_wchar* __thiscall basic_stringstream_wchar_str_get(const basic_stringstream_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p %p)\n", this, ret);
    return basic_stringbuf_wchar_str_get(&this->strbuf, ret);
}

/* ?_Init@strstreambuf@std@@IAEXHPAD0H@Z */
/* ?_Init@strstreambuf@std@@IEAAX_JPEAD1H@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(strstreambuf__Init, 24)
#else
DEFINE_THISCALL_WRAPPER(strstreambuf__Init, 20)
#endif
void __thiscall strstreambuf__Init(strstreambuf *this, streamsize len, char *g, char *p, int mode)
{
    TRACE("(%p %s %p %p %d)\n", this, wine_dbgstr_longlong(len), g, p, mode);

    this->minsize = 32;
    this->endsave = NULL;
    this->strmode = mode;
    this->palloc = NULL;
    this->pfree = NULL;

    if(!g) {
        this->strmode |= STRSTATE_Dynamic;
        if(len > this->minsize)
            this->minsize = len;
        this->seekhigh = NULL;
        return;
    }

    if(len < 0)
        len = INT_MAX;
    else if(!len)
        len = strlen(g);

    this->seekhigh = g+len;
    basic_streambuf_char_setg(&this->base, g, g, p ? p : this->seekhigh);
    if(p)
        basic_streambuf_char_setp(&this->base, p, this->seekhigh);
}

/* ??0strstreambuf@std@@QAE@PACH0@Z */
/* ??0strstreambuf@std@@QEAA@PEAC_J0@Z */
/* ??0strstreambuf@std@@QAE@PADH0@Z */
/* ??0strstreambuf@std@@QEAA@PEAD_J0@Z */
/* ??0strstreambuf@std@@QAE@PAEH0@Z */
/* ??0strstreambuf@std@@QEAA@PEAE_J0@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(strstreambuf_ctor_get_put, 20)
#else
DEFINE_THISCALL_WRAPPER(strstreambuf_ctor_get_put, 16)
#endif
strstreambuf* __thiscall strstreambuf_ctor_get_put(strstreambuf *this, char *g, streamsize len, char *p)
{
    TRACE("(%p %p %s %p)\n", this, g, wine_dbgstr_longlong(len), p);

    basic_streambuf_char_ctor(&this->base);
    this->base.vtable = &strstreambuf_vtable;

    strstreambuf__Init(this, len, g, p, 0);
    return this;
}

/* ??0strstreambuf@std@@QAE@H@Z */
/* ??0strstreambuf@std@@QEAA@_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(strstreambuf_ctor_len, 12)
#else
DEFINE_THISCALL_WRAPPER(strstreambuf_ctor_len, 8)
#endif
strstreambuf* __thiscall strstreambuf_ctor_len(strstreambuf *this, streamsize len)
{
    return strstreambuf_ctor_get_put(this, NULL, len, NULL);
}

/* ??0strstreambuf@std@@QAE@P6APAXI@ZP6AXPAX@Z@Z */
/* ??0strstreambuf@std@@QEAA@P6APEAX_K@ZP6AXPEAX@Z@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_ctor_alloc, 12)
strstreambuf* __thiscall strstreambuf_ctor_alloc(strstreambuf *this,
        void* (__cdecl *palloc)(size_t), void (__cdecl *pfree)(void*))
{
    TRACE("(%p %p %p)\n", this, palloc, pfree);

    strstreambuf_ctor_get_put(this, NULL, 0, NULL);
    this->palloc = palloc;
    this->pfree = pfree;
    return this;
}

/* ??0strstreambuf@std@@QAE@PBCH@Z */
/* ??0strstreambuf@std@@QEAA@PEBC_J@Z */
/* ??0strstreambuf@std@@QAE@PBDH@Z */
/* ??0strstreambuf@std@@QEAA@PEBD_J@Z */
/* ??0strstreambuf@std@@QAE@PBEH@Z */
/* ??0strstreambuf@std@@QEAA@PEBE_J@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(strstreambuf_ctor_get, 16)
#else
DEFINE_THISCALL_WRAPPER(strstreambuf_ctor_get, 12)
#endif
strstreambuf* __thiscall strstreambuf_ctor_get(strstreambuf *this, const char *g, streamsize len)
{
    TRACE("(%p %p %s)\n", this, g, wine_dbgstr_longlong(len));

    strstreambuf_ctor_get_put(this, (char*)g, len, NULL);
    this->strmode |= STRSTATE_Constant;
    return this;
}

/* ??_Fstrstreambuf@std@@QAEXXZ */
/* ??_Fstrstreambuf@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(strstreambuf_ctor, 4)
strstreambuf* __thiscall strstreambuf_ctor(strstreambuf *this)
{
    return strstreambuf_ctor_get_put(this, NULL, 0, NULL);
}

/* ?_Tidy@strstreambuf@std@@IAEXXZ */
/* ?_Tidy@strstreambuf@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(strstreambuf__Tidy, 4)
void __thiscall strstreambuf__Tidy(strstreambuf *this)
{
    TRACE("(%p)\n", this);

    if((this->strmode & STRSTATE_Allocated) && !(this->strmode & STRSTATE_Frozen)) {
        if(this->pfree)
            this->pfree(basic_streambuf_char_eback(&this->base));
        else
            operator_delete(basic_streambuf_char_eback(&this->base));
    }

    this->endsave = NULL;
    this->seekhigh = NULL;
    this->strmode &= ~(STRSTATE_Allocated | STRSTATE_Frozen);
    basic_streambuf_char_setg(&this->base, NULL, NULL, NULL);
    basic_streambuf_char_setp(&this->base, NULL, NULL);
}

/* ??1strstreambuf@std@@UAE@XZ */
/* ??1strstreambuf@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(strstreambuf_dtor, 4)
void __thiscall strstreambuf_dtor(strstreambuf *this)
{
    TRACE("(%p)\n", this);

    strstreambuf__Tidy(this);
    basic_streambuf_char_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(strstreambuf_vector_dtor, 8)
strstreambuf* __thiscall strstreambuf_vector_dtor(strstreambuf *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            strstreambuf_dtor(this+i);
        operator_delete(ptr);
    } else {
        strstreambuf_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?freeze@strstreambuf@std@@QAEX_N@Z */
/* ?freeze@strstreambuf@std@@QEAAX_N@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_freeze, 8)
void __thiscall strstreambuf_freeze(strstreambuf *this, bool freeze)
{
    TRACE("(%p %d)\n", this, freeze);

    if(!freeze == !(this->strmode & STRSTATE_Frozen))
        return;

    if(freeze) {
        this->strmode |= STRSTATE_Frozen;
        this->endsave = basic_streambuf_char_epptr(&this->base);
        basic_streambuf_char_setp_next(&this->base, basic_streambuf_char_pbase(&this->base),
                basic_streambuf_char_pptr(&this->base), basic_streambuf_char_eback(&this->base));
    }else {
        this->strmode &= ~STRSTATE_Frozen;
        basic_streambuf_char_setp_next(&this->base, basic_streambuf_char_pbase(&this->base),
                basic_streambuf_char_pptr(&this->base), this->endsave);
    }
}

/* ?str@strstreambuf@std@@QAEPADXZ */
/* ?str@strstreambuf@std@@QEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(strstreambuf_str, 4)
char* __thiscall strstreambuf_str(strstreambuf *this)
{
    TRACE("(%p)\n", this);

    strstreambuf_freeze(this, TRUE);
    return basic_streambuf_char_gptr(&this->base);
}

/* ?pcount@strstreambuf@std@@QBEHXZ */
/* ?pcount@strstreambuf@std@@QEBA_JXZ */
DEFINE_THISCALL_WRAPPER(strstreambuf_pcount, 4)
streamsize __thiscall strstreambuf_pcount(const strstreambuf *this)
{
    char *ppos = basic_streambuf_char_pptr(&this->base);

    TRACE("(%p)\n", this);

    return ppos ? ppos-basic_streambuf_char_pbase(&this->base) : 0;
}

/* ?overflow@strstreambuf@std@@MAEHH@Z */
/* ?overflow@strstreambuf@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_overflow, 8)
int __thiscall strstreambuf_overflow(strstreambuf *this, int c)
{
    size_t old_size, size;
    char *ptr, *buf;

    TRACE("(%p %d)\n", this, c);

    if(c == EOF)
        return !EOF;

    if(this->strmode & STRSTATE_Frozen)
        return EOF;

    ptr = basic_streambuf_char_pptr(&this->base);
    if(ptr && ptr<basic_streambuf_char_epptr(&this->base))
        return (unsigned char)(*basic_streambuf_char__Pninc(&this->base) = c);

    if(!(this->strmode & STRSTATE_Dynamic) || (this->strmode & STRSTATE_Constant))
        return EOF;

    ptr = basic_streambuf_char_eback(&this->base);
    old_size = ptr ? basic_streambuf_char_epptr(&this->base) - ptr : 0;

    size = old_size + old_size/2;
    if(size < this->minsize)
        size = this->minsize;

    if(this->palloc)
        buf = this->palloc(size);
    else
        buf = operator_new(size);
    if(!buf)
        return EOF;

    memcpy(buf, ptr, old_size);
    if(this->strmode & STRSTATE_Allocated) {
        if(this->pfree)
            this->pfree(ptr);
        else
            operator_delete(ptr);
    }

    this->strmode |= STRSTATE_Allocated;
    if(!old_size) {
        this->seekhigh = buf;
        basic_streambuf_char_setp(&this->base, buf, buf+size);
        basic_streambuf_char_setg(&this->base, buf, buf, buf);
    }else {
        this->seekhigh = this->seekhigh-ptr+buf;
        basic_streambuf_char_setp_next(&this->base, basic_streambuf_char_pbase(&this->base)-ptr+buf,
                basic_streambuf_char_pptr(&this->base)-ptr+buf, buf+size);
        basic_streambuf_char_setg(&this->base, buf, basic_streambuf_char_gptr(&this->base)-ptr+buf,
                basic_streambuf_char_pptr(&this->base));
    }

    return (unsigned char)(*basic_streambuf_char__Pninc(&this->base) = c);
}

/* ?pbackfail@strstreambuf@std@@MAEHH@Z */
/* ?pbackfail@strstreambuf@std@@MEAAHH@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_pbackfail, 8)
int __thiscall strstreambuf_pbackfail(strstreambuf *this, int c)
{
    char *ptr = basic_streambuf_char_gptr(&this->base);

    TRACE("(%p %d)\n", this, c);

    if(ptr<=basic_streambuf_char_eback(&this->base)
            || ((this->strmode & STRSTATE_Constant) && c!=ptr[-1]))
        return EOF;

    basic_streambuf_char_gbump(&this->base, -1);
    if(c == EOF)
        return !EOF;
    if(this->strmode & STRSTATE_Constant)
        return (unsigned char)c;

    return (unsigned char)(ptr[0] = c);
}

/* ?seekoff@strstreambuf@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@strstreambuf@std@@MEAA?AV?$fpos@H@2@_JW4seekdir@ios_base@2@H@Z */
/* ?seekoff@strstreambuf@std@@MAE?AV?$fpos@H@2@JHH@Z */
/* ?seekoff@strstreambuf@std@@MEAA?AV?$fpos@H@2@_JHH@Z */
#if STREAMOFF_BITS == 64
DEFINE_THISCALL_WRAPPER(strstreambuf_seekoff, 24)
#else
DEFINE_THISCALL_WRAPPER(strstreambuf_seekoff, 20)
#endif
fpos_mbstatet* __thiscall strstreambuf_seekoff(strstreambuf *this, fpos_mbstatet *ret, streamoff off, int way, int mode)
{
    char *eback = basic_streambuf_char_eback(&this->base);
    char *pptr = basic_streambuf_char_pptr(&this->base);
    char *gptr = basic_streambuf_char_gptr(&this->base);

    TRACE("(%p %p %s %d %d)\n", this, ret, wine_dbgstr_longlong(off), way, mode);

    ret->off = 0;
    memset(&ret->state, 0, sizeof(ret->state));

    if(pptr > this->seekhigh)
        this->seekhigh = pptr;

    if((mode & OPENMODE_in) && gptr) {
        if(way==SEEKDIR_cur && !(mode & OPENMODE_out))
            off += gptr-eback;
        else if(way == SEEKDIR_end)
            off += this->seekhigh-eback;
        else if(way != SEEKDIR_beg)
            off = -1;

        if(off<0 || off>this->seekhigh-eback) {
            off = -1;
        }else {
            basic_streambuf_char_gbump(&this->base, eback-gptr+off);
            if((mode & OPENMODE_out) && pptr) {
                basic_streambuf_char_setp_next(&this->base, eback,
                        gptr, basic_streambuf_char_epptr(&this->base));
            }
        }
    }else if((mode & OPENMODE_out) && pptr) {
        if(way == SEEKDIR_cur)
            off += pptr-eback;
        else if(way == SEEKDIR_end)
            off += this->seekhigh-eback;
        else if(way != SEEKDIR_beg)
            off = -1;

         if(off<0 || off>this->seekhigh-eback)
             off = -1;
         else
             basic_streambuf_char_pbump(&this->base, eback-pptr+off);
    }else {
        off = -1;
    }

    ret->pos = off;
    return ret;
}

/* ?seekpos@strstreambuf@std@@MAE?AV?$fpos@H@2@V32@H@Z */
/* ?seekpos@strstreambuf@std@@MEAA?AV?$fpos@H@2@V32@H@Z */
DEFINE_THISCALL_WRAPPER(strstreambuf_seekpos, 36)
fpos_mbstatet* __thiscall strstreambuf_seekpos(strstreambuf *this, fpos_mbstatet *ret, fpos_mbstatet pos, int mode)
{
    TRACE("(%p %p %s %d)\n", this, ret, debugstr_fpos_mbstatet(&pos), mode);

    if(pos.off==-1 && pos.pos==0 && MBSTATET_TO_INT(&pos.state)==0) {
        *ret = pos;
        return ret;
    }

    return strstreambuf_seekoff(this, ret, pos.pos+pos.off, SEEKDIR_beg, mode);
}

/* ?underflow@strstreambuf@std@@MAEHXZ */
/* ?underflow@strstreambuf@std@@MEAAHXZ */
DEFINE_THISCALL_WRAPPER(strstreambuf_underflow, 4)
int __thiscall strstreambuf_underflow(strstreambuf *this)
{
    char *gptr = basic_streambuf_char_gptr(&this->base);
    char *pptr;

    TRACE("(%p)\n", this);

    if(!gptr)
        return EOF;

    if(gptr < basic_streambuf_char_egptr(&this->base))
        return (unsigned char)(*gptr);

    pptr = basic_streambuf_char_gptr(&this->base);
    if(pptr > this->seekhigh)
        this->seekhigh = pptr;

    if(this->seekhigh <= gptr)
        return EOF;

    basic_streambuf_char_setg(&this->base, basic_streambuf_char_eback(&this->base),
            gptr, this->seekhigh);
    return (unsigned char)(*gptr);
}

static inline basic_ios_char* ostrstream_to_basic_ios(ostrstream *ptr)
{
    return (basic_ios_char*)((char*)ptr+ostrstream_vbtable[1]);
}

static inline ostrstream* ostrstream_from_basic_ios(basic_ios_char *ptr)
{
    return (ostrstream*)((char*)ptr-ostrstream_vbtable[1]);
}

/* ??0ostrstream@std@@QAE@PADHH@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(ostrstream_ctor, 24)
#else
DEFINE_THISCALL_WRAPPER(ostrstream_ctor, 20)
#endif
ostrstream* __thiscall ostrstream_ctor(ostrstream *this, char *buf, streamsize size, int mode, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %s %d %d)\n", this, buf, wine_dbgstr_longlong(size), mode, virt_init);

    if(virt_init) {
        this->base.vbtable = ostrstream_vbtable;
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_ostream_char_get_basic_ios(&this->base);
    }

    strstreambuf_ctor_get_put(&this->buf, buf, size,
            buf && (mode & OPENMODE_app) ? buf+strlen(buf) : buf);
    basic_ostream_char_ctor(&this->base, &this->buf.base, FALSE, FALSE);
    basic_ios->base.vtable = &ostrstream_vtable;
    return this;
}

/* ??1ostrstream@std@@UAE@XZ */
/* ??1ostrstream@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(ostrstream_dtor, 4)
void __thiscall ostrstream_dtor(basic_ios_char *base)
{
    ostrstream *this = ostrstream_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_ostream_char_dtor(basic_ostream_char_to_basic_ios(&this->base));
    strstreambuf_dtor(&this->buf);
}

static void ostrstream_vbase_dtor(ostrstream *this)
{
    basic_ios_char *base = ostrstream_to_basic_ios(this);

    TRACE("(%p)\n", this);

    ostrstream_dtor(base);
    basic_ios_char_dtor(base);
}

DEFINE_THISCALL_WRAPPER(ostrstream_vector_dtor, 8)
ostrstream* __thiscall ostrstream_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    ostrstream *this = ostrstream_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            ostrstream_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        ostrstream_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

static inline istrstream* istrstream_from_basic_ios(basic_ios_char *ptr)
{
    return (istrstream*)((char*)ptr-istrstream_vbtable[1]);
}

/* ??1istrstream@std@@UAE@XZ */
/* ??1istrstream@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(istrstream_dtor, 4)
void __thiscall istrstream_dtor(basic_ios_char *base)
{
    istrstream *this = istrstream_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_istream_char_dtor(basic_istream_char_to_basic_ios(&this->base));
    strstreambuf_dtor(&this->buf);
}

static inline basic_ios_char* strstream_to_basic_ios(strstream *ptr)
{
    return (basic_ios_char*)((char*)ptr+strstream_vbtable1[1]);
}

static inline strstream* strstream_from_basic_ios(basic_ios_char *ptr)
{
    return (strstream*)((char*)ptr-strstream_vbtable1[1]);
}

/* ??$?6MDU?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@ABV?$complex@M@0@@Z  */
/* ??$?6MDU?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@AEBV?$complex@M@0@@Z  */
basic_ostream_char* __cdecl basic_ostream_char_print_complex_float(basic_ostream_char *ostr, const complex_float *val)
{
    struct {
        basic_ostringstream_char obj;
        basic_ios_char vbase;
    } oss;
    ios_base *ostringstream_ios_base, *ostream_ios_base;
    locale loc;
    basic_string_char str;
    basic_ostringstream_char_ctor(&oss.obj);
    ostringstream_ios_base = &oss.vbase.base;
    ostream_ios_base = &basic_ostream_char_get_basic_ios(ostr)->base;
    TRACE("(%p %p)\n", ostr, val);

    ios_base_imbue(ostringstream_ios_base, &loc, IOS_LOCALE(ostream_ios_base));
    locale_dtor(&loc);
    ios_base_precision_set(ostringstream_ios_base, ios_base_precision_get(ostream_ios_base));
    ios_base_flags_set(ostringstream_ios_base, ios_base_flags_get(ostream_ios_base));

    basic_ostream_char_print_ch(&oss.obj.base, '(');
    basic_ostream_char_print_float(&oss.obj.base, val->real);
    basic_ostream_char_print_ch(&oss.obj.base, ',');
    basic_ostream_char_print_float(&oss.obj.base, val->imag);
    basic_ostream_char_print_ch(&oss.obj.base, ')');

    basic_ostringstream_char_str_get(&oss.obj, &str);
    basic_ostringstream_char_dtor(&oss.vbase);
    basic_ostream_char_print_bstr(ostr, &str);
    MSVCP_basic_string_char_dtor(&str);
    return ostr;
}

/* ??$?6NDU?$char_traits@D@std@@@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@0@AAV10@ABV?$complex@N@0@@Z */
/* ??$?6NDU?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@AEBV?$complex@N@0@@Z */
basic_ostream_char* __cdecl basic_ostream_char_print_complex_double(basic_ostream_char *ostr, const complex_double *val)
{
    struct {
        basic_ostringstream_char obj;
        basic_ios_char vbase;
    } oss;
    ios_base *ostringstream_ios_base, *ostream_ios_base;
    locale loc;
    basic_string_char str;
    basic_ostringstream_char_ctor(&oss.obj);
    ostringstream_ios_base = &oss.vbase.base;
    ostream_ios_base = &basic_ostream_char_get_basic_ios(ostr)->base;
    TRACE("(%p %p)\n", ostr, val);

    ios_base_imbue(ostringstream_ios_base, &loc, IOS_LOCALE(ostream_ios_base));
    locale_dtor(&loc);
    ios_base_precision_set(ostringstream_ios_base, ios_base_precision_get(ostream_ios_base));
    ios_base_flags_set(ostringstream_ios_base, ios_base_flags_get(ostream_ios_base));

    basic_ostream_char_print_ch(&oss.obj.base, '(');
    basic_ostream_char_print_double(&oss.obj.base, val->real);
    basic_ostream_char_print_ch(&oss.obj.base, ',');
    basic_ostream_char_print_double(&oss.obj.base, val->imag);
    basic_ostream_char_print_ch(&oss.obj.base, ')');

    basic_ostringstream_char_str_get(&oss.obj, &str);
    basic_ostringstream_char_dtor(&oss.vbase);
    basic_ostream_char_print_bstr(ostr, &str);
    MSVCP_basic_string_char_dtor(&str);
    return ostr;
}

/* ??$?6odu?$char_traits@d@std@@@std@@yaaav?$basic_ostream@du?$char_traits@d@std@@@0@aav10@abv?$complex@o@0@@Z */
/* ??$?6ODU?$char_traits@D@std@@@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@AEBV?$complex@O@0@@Z */
basic_ostream_char* __cdecl basic_ostream_char_print_complex_ldouble(basic_ostream_char *ostr, const complex_double *val)
{
    struct {
        basic_ostringstream_char obj;
        basic_ios_char vbase;
    } oss;
    ios_base *ostringstream_ios_base, *ostream_ios_base;
    locale loc;
    basic_string_char str;
    basic_ostringstream_char_ctor(&oss.obj);
    ostringstream_ios_base = &oss.vbase.base;
    ostream_ios_base = &basic_ostream_char_get_basic_ios(ostr)->base;
    TRACE("(%p %p)\n", ostr, val);

    ios_base_imbue(ostringstream_ios_base, &loc, IOS_LOCALE(ostream_ios_base));
    locale_dtor(&loc);
    ios_base_precision_set(ostringstream_ios_base, ios_base_precision_get(ostream_ios_base));
    ios_base_flags_set(ostringstream_ios_base, ios_base_flags_get(ostream_ios_base));

    basic_ostream_char_print_ch(&oss.obj.base, '(');
    basic_ostream_char_print_ldouble(&oss.obj.base, val->real);
    basic_ostream_char_print_ch(&oss.obj.base, ',');
    basic_ostream_char_print_ldouble(&oss.obj.base, val->imag);
    basic_ostream_char_print_ch(&oss.obj.base, ')');

    basic_ostringstream_char_str_get(&oss.obj, &str);
    basic_ostringstream_char_dtor(&oss.vbase);
    basic_ostream_char_print_bstr(ostr, &str);
    MSVCP_basic_string_char_dtor(&str);
    return ostr;
}

/* ?_File_size@sys@tr2@std@@YA_KPBD@Z  */
/* ?_File_size@sys@tr2@std@@YA_KPEBD@Z */
ULONGLONG __cdecl tr2_sys__File_size(char const* path)
{
    WIN32_FILE_ATTRIBUTE_DATA fad;

    TRACE("(%s)\n", debugstr_a(path));
    if(!GetFileAttributesExA(path, GetFileExInfoStandard, &fad))
        return 0;

    return ((ULONGLONG)(fad.nFileSizeHigh) << 32) + fad.nFileSizeLow;
}

static int equivalent_handles(HANDLE h1, HANDLE h2)
{
    int ret;
    BY_HANDLE_FILE_INFORMATION info1, info2;

    if(h1 == INVALID_HANDLE_VALUE)
        return h2 == INVALID_HANDLE_VALUE ? -1 : 0;
    else if(h2 == INVALID_HANDLE_VALUE)
        return 0;

    ret = GetFileInformationByHandle(h1, &info1) && GetFileInformationByHandle(h2, &info2);
    if(!ret)
        return -1;
    return (info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber
            && info1.nFileIndexHigh == info2.nFileIndexHigh
            && info1.nFileIndexLow == info2.nFileIndexLow
            );
}

/* ?_Equivalent@sys@tr2@std@@YAHPBD0@Z  */
/* ?_Equivalent@sys@tr2@std@@YAHPEBD0@Z */
int __cdecl tr2_sys__Equivalent(char const* path1, char const* path2)
{
    HANDLE h1, h2;
    int ret;

    TRACE("(%s %s)\n", debugstr_a(path1), debugstr_a(path2));

    h1 = CreateFileA(path1, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, 0);
    h2 = CreateFileA(path2, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, 0);
    ret = equivalent_handles(h1, h2);
    CloseHandle(h1);
    CloseHandle(h2);
    return ret;
}

/* ?_Current_get@sys@tr2@std@@YAPADAAY0BAE@D@Z */
/* ?_Current_get@sys@tr2@std@@YAPEADAEAY0BAE@D@Z */
char* __cdecl tr2_sys__Current_get(char *current_path)
{
    TRACE("(%s)\n", debugstr_a(current_path));

    if(!GetCurrentDirectoryA(MAX_PATH, current_path))
        return NULL;
    return current_path;
}

/* ?_Current_set@sys@tr2@std@@YA_NPBD@Z */
/* ?_Current_set@sys@tr2@std@@YA_NPEBD@Z */
bool __cdecl tr2_sys__Current_set(char const* path)
{
    TRACE("(%s)\n", debugstr_a(path));
    return SetCurrentDirectoryA(path) != 0;
}

/* ?_Make_dir@sys@tr2@std@@YAHPBD@Z */
/* ?_Make_dir@sys@tr2@std@@YAHPEBD@Z */
int __cdecl tr2_sys__Make_dir(char const* path)
{
    TRACE("(%s)\n", debugstr_a(path));

    if(!CreateDirectoryA(path, NULL)) {
        if(GetLastError() == ERROR_ALREADY_EXISTS)
            return 0;
        else
            return -1;
    }

    return 1;
}

/* ?_Remove_dir@sys@tr2@std@@YA_NPBD@Z */
/* ?_Remove_dir@sys@tr2@std@@YA_NPEBD@Z */
bool __cdecl tr2_sys__Remove_dir(char const* path)
{
    TRACE("(%s)\n", debugstr_a(path));
    return RemoveDirectoryA(path) != 0;
}

/* ?_Copy_file@sys@tr2@std@@YAHPBD0_N@Z */
/* ?_Copy_file@sys@tr2@std@@YAHPEBD0_N@Z */
int __cdecl tr2_sys__Copy_file(char const* source, char const* dest, bool fail_if_exists)
{
    TRACE("(%s %s %x)\n", debugstr_a(source), debugstr_a(dest), fail_if_exists);

    if(!source || !dest)
        return ERROR_INVALID_PARAMETER;

    if(CopyFileA(source, dest, fail_if_exists))
        return ERROR_SUCCESS;
    return GetLastError();
}

/* ?_Rename@sys@tr2@std@@YAHPBD0@Z */
/* ?_Rename@sys@tr2@std@@YAHPEBD0@Z */
int __cdecl tr2_sys__Rename(char const* old_path, char const* new_path)
{
    TRACE("(%s %s)\n", debugstr_a(old_path), debugstr_a(new_path));

    if(!old_path || !new_path)
        return ERROR_INVALID_PARAMETER;

    if(MoveFileExA(old_path, new_path, MOVEFILE_COPY_ALLOWED))
        return ERROR_SUCCESS;
    return GetLastError();
}

/* ?_Statvfs@sys@tr2@std@@YA?AUspace_info@123@PBD@Z */
/* ?_Statvfs@sys@tr2@std@@YA?AUspace_info@123@PEBD@Z */
struct space_info* __cdecl tr2_sys__Statvfs(struct space_info *ret, const char* path)
{
    ULARGE_INTEGER available, total, free;

    TRACE("(%s)\n", debugstr_a(path));

    if(!path || !GetDiskFreeSpaceExA(path, &available, &total, &free)) {
        ret->capacity = ret->free = ret->available = 0;
    }else {
        ret->capacity = total.QuadPart;
        ret->free = free.QuadPart;
        ret->available = available.QuadPart;
    }
    return ret;
}

/* ?_Stat@sys@tr2@std@@YA?AW4file_type@123@PBDAAH@Z */
/* ?_Stat@sys@tr2@std@@YA?AW4file_type@123@PEBDAEAH@Z */
enum file_type __cdecl tr2_sys__Stat(char const* path, int* err_code)
{
    DWORD attr;
    TRACE("(%s %p)\n", debugstr_a(path), err_code);
    if(!path) {
        *err_code = ERROR_INVALID_PARAMETER;
        return status_unknown;
    }

    attr=GetFileAttributesA(path);
    if(attr == INVALID_FILE_ATTRIBUTES) {
        enum file_type ret;
        switch(GetLastError()) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_BAD_NETPATH:
            case ERROR_INVALID_NAME:
            case ERROR_BAD_PATHNAME:
            case ERROR_PATH_NOT_FOUND:
                ret = file_not_found;
                *err_code = ERROR_SUCCESS;
                break;
            default:
                ret = status_unknown;
                *err_code = GetLastError();
        }
        return ret;
    }

    *err_code = ERROR_SUCCESS;
    return (attr & FILE_ATTRIBUTE_DIRECTORY)?directory_file:regular_file;
}

/* ?_Lstat@sys@tr2@std@@YA?AW4file_type@123@PBDAAH@Z */
/* ?_Lstat@sys@tr2@std@@YA?AW4file_type@123@PEBDAEAH@Z */
enum file_type __cdecl tr2_sys__Lstat(char const* path, int* err_code)
{
    return tr2_sys__Stat(path, err_code);
}

static __int64 get_last_write_time(HANDLE h)
{
    FILETIME wt;
    __int64 ret;

    if(!GetFileTime(h, 0, 0, &wt))
        return -1;

    ret = (((__int64)wt.dwHighDateTime)<< 32) + wt.dwLowDateTime;
    ret -= TICKS_1601_TO_1970;
    return ret;
}

/* ?_Last_write_time@sys@tr2@std@@YA_JPBD@Z */
/* ?_Last_write_time@sys@tr2@std@@YA_JPEBD@Z */
__int64 __cdecl tr2_sys__Last_write_time(char const* path)
{
    HANDLE handle;
    __int64 ret;

    TRACE("(%s)\n", debugstr_a(path));

    handle = CreateFileA(path, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    if(handle == INVALID_HANDLE_VALUE)
        return 0;

    ret = get_last_write_time(handle);
    CloseHandle(handle);
    return ret / TICKSPERSEC;
}

/* _Last_write_time */
__int64 __cdecl _Last_write_time(const wchar_t *path)
{
    HANDLE handle;
    __int64 ret;

    TRACE("(%s)\n", debugstr_w(path));

    handle = CreateFileW(path, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    if(handle == INVALID_HANDLE_VALUE)
        return -1;

    ret = get_last_write_time(handle);
    CloseHandle(handle);
    return ret;
}

/* ?_Last_write_time@sys@tr2@std@@YA_JPB_W@Z */
/* ?_Last_write_time@sys@tr2@std@@YA_JPEB_W@Z */
__int64 __cdecl tr2_sys__Last_write_time_wchar(const wchar_t *path)
{
    TRACE("(%s)\n", debugstr_w(path));
    return _Last_write_time(path) / TICKSPERSEC;
}

static int set_last_write_time(HANDLE h, __int64 time)
{
    FILETIME wt;

    time += TICKS_1601_TO_1970;
    wt.dwLowDateTime = (DWORD)time;
    wt.dwHighDateTime = (DWORD)(time >> 32);
    return SetFileTime(h, 0, 0, &wt);
}

/* ?_Last_write_time@sys@tr2@std@@YAXPBD_J@Z */
/* ?_Last_write_time@sys@tr2@std@@YAXPEBD_J@Z */
void __cdecl tr2_sys__Last_write_time_set(char const* path, __int64 newtime)
{
    HANDLE handle;

    TRACE("(%s)\n", debugstr_a(path));

    handle = CreateFileA(path, FILE_WRITE_ATTRIBUTES,
            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    if(handle == INVALID_HANDLE_VALUE)
        return;

    /* This is the implementation based on the test of msvcp110.
     * According to the test of msvcp120,
     * msvcp120's implementation does nothing. Obviously, this is a bug of windows.
     */
    set_last_write_time(handle, newtime * TICKSPERSEC);
    CloseHandle(handle);
}

/* _Set_last_write_time */
int __cdecl _Set_last_write_time(const wchar_t *path, __int64 time)
{
    HANDLE handle;
    int ret;

    TRACE("(%s)\n", debugstr_w(path));

    handle = CreateFileW(path, FILE_WRITE_ATTRIBUTES,
            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    if(handle == INVALID_HANDLE_VALUE)
        return 0;

    ret = set_last_write_time(handle, time);
    CloseHandle(handle);
    return ret;
}

/* ?_Last_write_time@sys@tr2@std@@YAXPB_W_J@Z */
/* ?_Last_write_time@sys@tr2@std@@YAXPEB_W_J@Z */
void __cdecl tr2_sys__Last_write_time_set_wchar(const wchar_t *path, __int64 time)
{
    TRACE("(%s)\n", debugstr_w(path));
    _Set_last_write_time(path, time * TICKSPERSEC);
}

/* ??_Open_dir@sys@tr2@std@@YAPAXPA_WPB_WAAHAAW4file_type@123@@Z */
/* ??_Open_dir@sys@tr2@std@@YAPEAXPEA_WPEB_WAEAHAEAW4file_type@123@@Z */
void* __cdecl tr2_sys__Open_dir_wchar(wchar_t* target, wchar_t const* dest, int* err_code, enum file_type* type)
{
    HANDLE handle;
    WIN32_FIND_DATAW data;
    wchar_t temppath[MAX_PATH];

    TRACE("(%p %s %p %p)\n", target, debugstr_w(dest), err_code, type);
    if(wcslen(dest) > MAX_PATH - 3) {
        *err_code = ERROR_BAD_PATHNAME;
        *target = '\0';
        return NULL;
    }
    wcscpy(temppath, dest);
    wcscat(temppath, L"\\*");

    handle = FindFirstFileW(temppath, &data);
    if(handle == INVALID_HANDLE_VALUE) {
        *err_code = ERROR_BAD_PATHNAME;
        *target = '\0';
        return NULL;
    }
    while(!wcscmp(data.cFileName, L".") || !wcscmp(data.cFileName, L"..")) {
        if(!FindNextFileW(handle, &data)) {
            *err_code = ERROR_SUCCESS;
            *type = status_unknown;
            *target = '\0';
            FindClose(handle);
            return NULL;
        }
    }

    wcscpy(target, data.cFileName);
    *err_code = ERROR_SUCCESS;
    if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        *type = directory_file;
    else
        *type = regular_file;
    return handle;
}

/* ?_Open_dir@sys@tr2@std@@YAPAXAAY0BAE@DPBDAAHAAW4file_type@123@@Z */
/* ?_Open_dir@sys@tr2@std@@YAPEAXAEAY0BAE@DPEBDAEAHAEAW4file_type@123@@Z */
void* __cdecl tr2_sys__Open_dir(char* target, char const* dest, int* err_code, enum file_type* type)
{
    void *handle;
    wchar_t target_w[MAX_PATH];
    wchar_t dest_w[MAX_PATH];

    TRACE("(%p %s %p %p)\n", target, debugstr_a(dest), err_code, type);

    if (dest && !MultiByteToWideChar(CP_ACP, 0, dest, -1, dest_w, MAX_PATH))
    {
        WARN("Failed to convert input string.\n");
        *err_code = ERROR_BAD_PATHNAME;
        return NULL;
    }

    handle = tr2_sys__Open_dir_wchar(target_w, dest ? dest_w : NULL, err_code, type);

    WideCharToMultiByte(CP_ACP, 0, target_w, -1, target, MAX_PATH, NULL, NULL);

    return handle;
}

/* ??_Read_dir@sys@tr2@std@@YAPA_WPA_WPAXAAW4file_type@123@@Z */
/* ??_Read_dir@sys@tr2@std@@YAPEA_WPEA_WPEAXAEAW4file_type@123@@Z */
wchar_t* __cdecl tr2_sys__Read_dir_wchar(wchar_t* target, void* handle, enum file_type* type)
{
    WIN32_FIND_DATAW data;

    TRACE("(%p %p %p)\n", target, handle, type);

    do {
        if(!FindNextFileW(handle, &data)) {
            *type = status_unknown;
            *target = '\0';
            return target;
        }
    } while(!wcscmp(data.cFileName, L".") || !wcscmp(data.cFileName, L".."));

    wcscpy(target, data.cFileName);
    if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        *type = directory_file;
    else
        *type = regular_file;
    return target;
}

/* ?_Read_dir@sys@tr2@std@@YAPADAAY0BAE@DPAXAAW4file_type@123@@Z */
/* ?_Read_dir@sys@tr2@std@@YAPEADAEAY0BAE@DPEAXAEAW4file_type@123@@Z */
char* __cdecl tr2_sys__Read_dir(char* target, void* handle, enum file_type* type)
{
    wchar_t target_w[MAX_PATH];

    tr2_sys__Read_dir_wchar(target_w, handle, type);
    WideCharToMultiByte(CP_ACP, 0, target_w, -1, target, MAX_PATH, NULL, NULL);

    return target;
}

/* ?_Close_dir@sys@tr2@std@@YAXPAX@Z */
/* ?_Close_dir@sys@tr2@std@@YAXPEAX@Z */
void __cdecl tr2_sys__Close_dir(void* handle)
{
    TRACE("(%p)\n", handle);

    FindClose(handle);
}

/* ?_Link@sys@tr2@std@@YAHPBD0@Z */
/* ?_Link@sys@tr2@std@@YAHPEBD0@Z */
int __cdecl tr2_sys__Link(char const* existing_path, char const* new_path)
{
    TRACE("(%s %s)\n", debugstr_a(existing_path), debugstr_a(new_path));
    if(!existing_path || !new_path)
        return ERROR_INVALID_PARAMETER;

    if(CreateHardLinkA(new_path, existing_path, NULL))
        return ERROR_SUCCESS;
    return GetLastError();
}

/* ?_Link@sys@tr2@std@@YAHPB_W0@Z */
/* ?_Link@sys@tr2@std@@YAHPEB_W0@Z */
/* _Link */
int __cdecl tr2_sys__Link_wchar(WCHAR const* existing_path, WCHAR const* new_path)
{
    TRACE("(%s %s)\n", debugstr_w(existing_path), debugstr_w(new_path));
    if(!existing_path || !new_path)
        return ERROR_INVALID_PARAMETER;

    if(CreateHardLinkW(new_path, existing_path, NULL))
        return ERROR_SUCCESS;
    return GetLastError();
}

/* ?_Symlink@sys@tr2@std@@YAHPBD0@Z */
/* ?_Symlink@sys@tr2@std@@YAHPEBD0@Z */
int __cdecl tr2_sys__Symlink(char const* existing_file_name, char const* file_name)
{
    TRACE("(%s %s)\n", debugstr_a(existing_file_name), debugstr_a(file_name));
    if(!existing_file_name || !file_name)
        return ERROR_INVALID_PARAMETER;

#ifdef __REACTOS__
    return ERROR_CALL_NOT_IMPLEMENTED;
#else
    if(CreateSymbolicLinkA(file_name, existing_file_name, 0))
        return ERROR_SUCCESS;
    return GetLastError();
#endif
}

/* ?_Symlink@sys@tr2@std@@YAHPB_W0@Z */
/* ?_Symlink@sys@tr2@std@@YAHPEB_W0@Z */
/* _Symlink */
int __cdecl tr2_sys__Symlink_wchar(WCHAR const* existing_file_name, WCHAR const* file_name)
{
    TRACE("(%s %s)\n", debugstr_w(existing_file_name), debugstr_w(file_name));
    if(!existing_file_name || !file_name)
        return ERROR_INVALID_PARAMETER;

#ifdef __REACTOS__
    return ERROR_CALL_NOT_IMPLEMENTED;
#else
    if(CreateSymbolicLinkW(file_name, existing_file_name, 0))
        return ERROR_SUCCESS;
    return GetLastError();
#endif
}

/* ?_Unlink@sys@tr2@std@@YAHPBD@Z */
/* ?_Unlink@sys@tr2@std@@YAHPEBD@Z */
int __cdecl tr2_sys__Unlink(char const* path)
{
    TRACE("(%s)\n", debugstr_a(path));

    if(DeleteFileA(path))
        return ERROR_SUCCESS;
    return GetLastError();
}

/* ?_Unlink@sys@tr2@std@@YAHPB_W@Z */
/* ?_Unlink@sys@tr2@std@@YAHPEB_W@Z */
/* _Unlink */
int __cdecl tr2_sys__Unlink_wchar(WCHAR const* path)
{
    TRACE("(%s)\n", debugstr_w(path));

    if(DeleteFileW(path))
        return ERROR_SUCCESS;
    return GetLastError();
}

/* ??0strstream@std@@QAE@PADHH@Z */
/* ??0strstream@std@@QEAA@PEAD_JH@Z */
#if _MSVCP_VER >= 100 /* sizeof(streamsize) == 8 */
DEFINE_THISCALL_WRAPPER(strstream_ctor, 24)
#else
DEFINE_THISCALL_WRAPPER(strstream_ctor, 20)
#endif
strstream* __thiscall strstream_ctor(strstream *this, char *buf, streamsize size, int mode, bool virt_init)
{
    basic_ios_char *basic_ios;

    TRACE("(%p %p %s %d %d)\n", this, buf, wine_dbgstr_longlong(size), mode, virt_init);

    if(virt_init) {
        this->base.base1.vbtable = strstream_vbtable1;
        this->base.base2.vbtable = strstream_vbtable2;
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
        INIT_BASIC_IOS_VTORDISP(basic_ios);
        basic_ios_char_ctor(basic_ios);
    }else {
        basic_ios = basic_istream_char_get_basic_ios(&this->base.base1);
    }

    strstreambuf_ctor_get_put(&this->buf, buf, size,
            buf && (mode & OPENMODE_app) ? buf+strlen(buf) : buf);
    basic_iostream_char_ctor(&this->base, &this->buf.base, FALSE);
    basic_ios->base.vtable = &strstream_vtable;
    return this;
}

/* ??1strstream@std@@UAE@XZ */
/* ??1strstream@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(strstream_dtor, 4)
void __thiscall strstream_dtor(basic_ios_char *base)
{
    strstream *this = strstream_from_basic_ios(base);

    TRACE("(%p)\n", this);

    basic_iostream_char_dtor(basic_iostream_char_to_basic_ios(&this->base));
    strstreambuf_dtor(&this->buf);
}

static void strstream_vbase_dtor(strstream *this)
{
    basic_ios_char *base = strstream_to_basic_ios(this);

    TRACE("(%p)\n", this);

    strstream_dtor(base);
    basic_ios_char_dtor(base);
}

DEFINE_THISCALL_WRAPPER(strstream_vector_dtor, 8)
strstream* __thiscall strstream_vector_dtor(basic_ios_char *base, unsigned int flags)
{
    strstream *this = strstream_from_basic_ios(base);

    TRACE("(%p %x)\n", this, flags);

    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            strstream_vbase_dtor(this+i);
        operator_delete(ptr);
    } else {
        strstream_vbase_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

static void __cdecl setprecision_func(ios_base *base, streamsize prec)
{
    ios_base_precision_set(base, prec);
}

/* ?setprecision@std@@YA?AU?$_Smanip@H@1@H@Z */
/* ?setprecision@std@@YA?AU?$_Smanip@_J@1@_J@Z */
manip_streamsize* __cdecl setprecision(manip_streamsize *ret, streamsize prec)
{
    TRACE("(%p %s)\n", ret, wine_dbgstr_longlong(prec));

    ret->pfunc = setprecision_func;
    ret->arg = prec;
    return ret;
}

static void __cdecl setw_func(ios_base *base, streamsize width)
{
    ios_base_width_set(base, width);
}

/* ?setw@std@@YA?AU?$_Smanip@H@1@H@Z */
/* ?setw@std@@YA?AU?$_Smanip@_J@1@_J@Z */
manip_streamsize* __cdecl setw(manip_streamsize *ret, streamsize width)
{
    TRACE("(%p %s)\n", ret, wine_dbgstr_longlong(width));

    ret->pfunc = setw_func;
    ret->arg = width;
    return ret;
}

static void __cdecl resetioflags_func(ios_base *base, int mask)
{
    ios_base_setf_mask(base, 0, mask);
}

/* ?resetiosflags@std@@YA?AU?$_Smanip@H@1@H@Z */
manip_int* __cdecl resetiosflags(manip_int *ret, int mask)
{
    TRACE("(%p %d)\n", ret, mask);

    ret->pfunc = resetioflags_func;
    ret->arg = mask;
    return ret;
}

static void __cdecl setiosflags_func(ios_base *base, int mask)
{
    ios_base_setf_mask(base, FMTFLAG_mask, mask);
}

/* ?setiosflags@std@@YA?AU?$_Smanip@H@1@H@Z */
manip_int* __cdecl setiosflags(manip_int *ret, int mask)
{
    TRACE("(%p %d)\n", ret, mask);

    ret->pfunc = setiosflags_func;
    ret->arg = mask;
    return ret;
}

static void __cdecl setbase_func(ios_base *base, int set_base)
{
    if(set_base == 10)
        set_base = FMTFLAG_dec;
    else if(set_base == 8)
        set_base = FMTFLAG_oct;
    else if(set_base == 16)
        set_base = FMTFLAG_hex;
    else
        set_base = 0;

    ios_base_setf_mask(base, set_base, FMTFLAG_basefield);
}

/* ?setbase@std@@YA?AU?$_Smanip@H@1@H@Z */
manip_int* __cdecl setbase(manip_int *ret, int base)
{
    TRACE("(%p %d)\n", ret, base);

    ret->pfunc = setbase_func;
    ret->arg = base;
    return ret;
}

static basic_filebuf_char filebuf_char_stdin;
/* ?cin@std@@3V?$basic_istream@DU?$char_traits@D@std@@@1@A */
struct {
    basic_istream_char obj;
#if _MSVCP_VER >= 110
    int vtordisp;
#endif
    basic_ios_char vbase;
} cin = { { 0 } };
C_ASSERT(sizeof(cin) == VBTABLE_BASIC_IOS_ENTRY(basic_istream_char, 0)+sizeof(basic_ios_char));
/* ?_Ptr_cin@std@@3PAV?$basic_istream@DU?$char_traits@D@std@@@1@A */
/* ?_Ptr_cin@std@@3PEAV?$basic_istream@DU?$char_traits@D@std@@@1@EA */
basic_istream_char *_Ptr_cin = &cin.obj;

static basic_filebuf_wchar filebuf_short_stdin;
/* ?wcin@std@@3V?$basic_istream@GU?$char_traits@G@std@@@1@A */
struct {
    basic_istream_wchar obj;
#if _MSVCP_VER >= 110
    int vtordisp;
#endif
    basic_ios_wchar vbase;
} ucin = { { 0 } };
C_ASSERT(sizeof(ucin) == VBTABLE_BASIC_IOS_ENTRY(basic_istream_wchar, 0)+sizeof(basic_ios_wchar));
/* ?_Ptr_wcin@std@@3PAV?$basic_istream@GU?$char_traits@G@std@@@1@A */
/* ?_Ptr_wcin@std@@3PEAV?$basic_istream@GU?$char_traits@G@std@@@1@EA */
basic_istream_wchar *_Ptr_ucin = &ucin.obj;

static basic_filebuf_wchar filebuf_wchar_stdin;
/* ?wcin@std@@3V?$basic_istream@_WU?$char_traits@_W@std@@@1@A */
struct {
    basic_istream_wchar obj;
#if _MSVCP_VER >= 110
    int vtordisp;
#endif
    basic_ios_wchar vbase;
} wcin = { { 0 } };
C_ASSERT(sizeof(wcin) == VBTABLE_BASIC_IOS_ENTRY(basic_istream_wchar, 0)+sizeof(basic_ios_wchar));
/* ?_Ptr_wcin@std@@3PAV?$basic_istream@_WU?$char_traits@_W@std@@@1@A */
/* ?_Ptr_wcin@std@@3PEAV?$basic_istream@_WU?$char_traits@_W@std@@@1@EA */
basic_istream_wchar *_Ptr_wcin = &wcin.obj;

static basic_filebuf_char filebuf_char_stdout;
/* ?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A */
struct {
    basic_ostream_char obj;
#if _MSVCP_VER >= 110
    int vtordisp;
#endif
    basic_ios_char vbase;
} cout = { { 0 } };
C_ASSERT(sizeof(cout) == VBTABLE_BASIC_IOS_ENTRY(basic_ostream_char, 0)+sizeof(basic_ios_char));
/* ?_Ptr_cout@std@@3PAV?$basic_ostream@DU?$char_traits@D@std@@@1@A */
/* ?_Ptr_cout@std@@3PEAV?$basic_ostream@DU?$char_traits@D@std@@@1@EA */
basic_ostream_char *_Ptr_cout = &cout.obj;

static basic_filebuf_wchar filebuf_short_stdout;
/* ?wcout@std@@3V?$basic_ostream@GU?$char_traits@G@std@@@1@A */
struct {
    basic_ostream_wchar obj;
#if _MSVCP_VER >= 110
    int vtordisp;
#endif
    basic_ios_wchar vbase;
} ucout = { { 0 } };
C_ASSERT(sizeof(ucout) == VBTABLE_BASIC_IOS_ENTRY(basic_ostream_wchar, 0)+sizeof(basic_ios_wchar));
/* ?_Ptr_wcout@std@@3PAV?$basic_ostream@GU?$char_traits@G@std@@@1@A */
/* ?_Ptr_wcout@std@@3PEAV?$basic_ostream@GU?$char_traits@G@std@@@1@EA */
basic_ostream_wchar *_Ptr_ucout = &ucout.obj;

static basic_filebuf_wchar filebuf_wchar_stdout;
/* ?wcout@std@@3V?$basic_ostream@_WU?$char_traits@_W@std@@@1@A */
struct {
    basic_ostream_wchar obj;
#if _MSVCP_VER >= 110
    int vtordisp;
#endif
    basic_ios_wchar vbase;
} wcout = { { 0 } };
C_ASSERT(sizeof(wcout) == VBTABLE_BASIC_IOS_ENTRY(basic_ostream_wchar, 0)+sizeof(basic_ios_wchar));
/* ?_Ptr_wcout@std@@3PAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@A */
/* ?_Ptr_wcout@std@@3PEAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@EA */
basic_ostream_wchar *_Ptr_wcout = &wcout.obj;

static basic_filebuf_char filebuf_char_stderr;
/* ?cerr@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A */
struct {
    basic_ostream_char obj;
#if _MSVCP_VER >= 110
    int vtordisp;
#endif
    basic_ios_char vbase;
} cerr = { { 0 } };
C_ASSERT(sizeof(cerr) == VBTABLE_BASIC_IOS_ENTRY(basic_ostream_char, 0)+sizeof(basic_ios_char));
/* ?_Ptr_cerr@std@@3PAV?$basic_ostream@DU?$char_traits@D@std@@@1@A */
/* ?_Ptr_cerr@std@@3PEAV?$basic_ostream@DU?$char_traits@D@std@@@1@EA */
basic_ostream_char *_Ptr_cerr = &cerr.obj;

static basic_filebuf_wchar filebuf_short_stderr;
/* ?wcerr@std@@3V?$basic_ostream@GU?$char_traits@G@std@@@1@A */
struct {
    basic_ostream_wchar obj;
#if _MSVCP_VER >= 110
    int vtordisp;
#endif
    basic_ios_wchar vbase;
} ucerr = { { 0 } };
C_ASSERT(sizeof(ucerr) == VBTABLE_BASIC_IOS_ENTRY(basic_ostream_wchar, 0)+sizeof(basic_ios_wchar));
/* ?_Ptr_wcerr@std@@3PAV?$basic_ostream@GU?$char_traits@G@std@@@1@A */
/* ?_Ptr_wcerr@std@@3PEAV?$basic_ostream@GU?$char_traits@G@std@@@1@EA */
basic_ostream_wchar *_Ptr_ucerr = &ucerr.obj;

static basic_filebuf_wchar filebuf_wchar_stderr;
/* ?wcerr@std@@3V?$basic_ostream@_WU?$char_traits@_W@std@@@1@A */
struct {
    basic_ostream_wchar obj;
#if _MSVCP_VER >= 110
    int vtordisp;
#endif
    basic_ios_wchar vbase;
} wcerr = { { 0 } };
C_ASSERT(sizeof(wcerr) == VBTABLE_BASIC_IOS_ENTRY(basic_ostream_wchar, 0)+sizeof(basic_ios_wchar));
/* ?_Ptr_wcerr@std@@3PAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@A */
/* ?_Ptr_wcerr@std@@3PEAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@EA */
basic_ostream_wchar *_Ptr_wcerr = &wcerr.obj;

static basic_filebuf_char filebuf_char_log;
/* ?clog@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A */
struct {
    basic_ostream_char obj;
#if _MSVCP_VER >= 110
    int vtordisp;
#endif
    basic_ios_char vbase;
} MSVCP_clog = { { 0 } };
C_ASSERT(sizeof(MSVCP_clog) == VBTABLE_BASIC_IOS_ENTRY(basic_ostream_char, 0)+sizeof(basic_ios_char));
/* ?_Ptr_clog@std@@3PAV?$basic_ostream@DU?$char_traits@D@std@@@1@A */
/* ?_Ptr_clog@std@@3PEAV?$basic_ostream@DU?$char_traits@D@std@@@1@EA */
basic_ostream_char *_Ptr_clog = &MSVCP_clog.obj;

static basic_filebuf_wchar filebuf_short_log;
/* ?wclog@std@@3V?$basic_ostream@GU?$char_traits@G@std@@@1@A */
struct {
    basic_ostream_wchar obj;
#if _MSVCP_VER >= 110
    int vtordisp;
#endif
    basic_ios_wchar vbase;
} uclog = { { 0 } };
C_ASSERT(sizeof(uclog) == VBTABLE_BASIC_IOS_ENTRY(basic_ostream_wchar, 0)+sizeof(basic_ios_wchar));
/* ?_Ptr_wclog@std@@3PAV?$basic_ostream@GU?$char_traits@G@std@@@1@A */
/* ?_Ptr_wclog@std@@3PEAV?$basic_ostream@GU?$char_traits@G@std@@@1@EA */
basic_ostream_wchar *_Ptr_uclog = &uclog.obj;

static basic_filebuf_wchar filebuf_wchar_log;
/* ?wclog@std@@3V?$basic_ostream@_WU?$char_traits@_W@std@@@1@A */
struct {
    basic_ostream_wchar obj;
#if _MSVCP_VER >= 110
    int vtordisp;
#endif
    basic_ios_wchar vbase;
} wclog = { { 0 } };
C_ASSERT(sizeof(wclog) == VBTABLE_BASIC_IOS_ENTRY(basic_ostream_wchar, 0)+sizeof(basic_ios_wchar));
/* ?_Ptr_wclog@std@@3PAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@A */
/* ?_Ptr_wclog@std@@3PEAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@EA */
basic_ostream_wchar *_Ptr_wclog = &wclog.obj;

/* ?_Init_cnt@Init@ios_base@std@@0HA */
int ios_base_Init__Init_cnt = -1;

/* ?_Init_cnt_func@Init@ios_base@std@@CAAAHXZ */
/* ?_Init_cnt_func@Init@ios_base@std@@CAAEAHXZ */
int* __cdecl ios_base_Init__Init_cnt_func(void)
{
    return &ios_base_Init__Init_cnt;
}

/* ?_Init_ctor@Init@ios_base@std@@CAXPAV123@@Z */
/* ?_Init_ctor@Init@ios_base@std@@CAXPEAV123@@Z */
void __cdecl ios_base_Init__Init_ctor(void *this)
{
    TRACE("(%p)\n", this);

    if(ios_base_Init__Init_cnt < 0)
        ios_base_Init__Init_cnt = 1;
    else
        ios_base_Init__Init_cnt++;
}

/* ??0Init@ios_base@std@@QAE@XZ */
/* ??0Init@ios_base@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_Init_ctor, 4)
void* __thiscall ios_base_Init_ctor(void *this)
{
    ios_base_Init__Init_ctor(this);
    return this;
}

/* ?_Init_dtor@Init@ios_base@std@@CAXPAV123@@Z */
/* ?_Init_dtor@Init@ios_base@std@@CAXPEAV123@@Z */
void __cdecl ios_base_Init__Init_dtor(void *this)
{
    TRACE("(%p)\n", this);

    ios_base_Init__Init_cnt--;
    if(!ios_base_Init__Init_cnt) {
        basic_ostream_char_flush(&cout.obj);
        basic_ostream_char_flush(&cerr.obj);
        basic_ostream_char_flush(&MSVCP_clog.obj);
    }
}

/* ??1Init@ios_base@std@@QAE@XZ */
/* ??1Init@ios_base@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(ios_base_Init_dtor, 4)
void __thiscall ios_base_Init_dtor(void *this)
{
    ios_base_Init__Init_dtor(this);
}

/* ??4Init@ios_base@std@@QAEAAV012@ABV012@@Z */
/* ??4Init@ios_base@std@@QEAAAEAV012@AEBV012@@Z */
DEFINE_THISCALL_WRAPPER(ios_base_Init_op_assign, 8)
void* __thiscall ios_base_Init_op_assign(void *this, void *rhs)
{
    TRACE("(%p %p)\n", this, rhs);
    return this;
}

/* ?_Init_cnt@_Winit@std@@0HA */
int _Winit__Init_cnt = -1;

/* ??0_Winit@std@@QAE@XZ */
/* ??0_Winit@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Winit_ctor, 4)
void* __thiscall _Winit_ctor(void *this)
{
    TRACE("(%p)\n", this);

    if(_Winit__Init_cnt < 0)
        _Winit__Init_cnt = 1;
    else
        _Winit__Init_cnt++;

    return this;
}

/* ?_File_size@sys@tr2@std@@YA_KPB_W@Z */
/* ?_File_size@sys@tr2@std@@YA_KPEB_W@Z */
ULONGLONG __cdecl tr2_sys__File_size_wchar(WCHAR const* path)
{
    WIN32_FILE_ATTRIBUTE_DATA fad;

    TRACE("(%s)\n", debugstr_w(path));
    if(!GetFileAttributesExW(path, GetFileExInfoStandard, &fad))
        return 0;

    return ((ULONGLONG)(fad.nFileSizeHigh) << 32) + fad.nFileSizeLow;
}

/* _File_size, msvcp140 version. Different error handling. */
ULONGLONG __cdecl _File_size(WCHAR const* path)
{
    WIN32_FILE_ATTRIBUTE_DATA fad;

    TRACE("(%s)\n", debugstr_w(path));
    if(!GetFileAttributesExW(path, GetFileExInfoStandard, &fad))
        return ~(ULONGLONG)0;

    return ((ULONGLONG)(fad.nFileSizeHigh) << 32) + fad.nFileSizeLow;
}

int __cdecl _Resize(const WCHAR *path, UINT64 size)
{
    LARGE_INTEGER offset;
    HANDLE file;
    BOOL ret;

    TRACE("(%s %s)\n", debugstr_w(path), wine_dbgstr_longlong(size));

    file = CreateFileW(path, FILE_GENERIC_WRITE, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, 0);
    if(file == INVALID_HANDLE_VALUE)
        return GetLastError();

    offset.QuadPart = size;
    if((ret = SetFilePointerEx(file, offset, NULL, FILE_BEGIN)))
        ret = SetEndOfFile(file);
    CloseHandle(file);
    return ret ? 0 : GetLastError();
}

/* ?_Equivalent@sys@tr2@std@@YAHPB_W0@Z */
/* ?_Equivalent@sys@tr2@std@@YAHPEB_W0@Z */
int __cdecl tr2_sys__Equivalent_wchar(WCHAR const* path1, WCHAR const* path2)
{
    HANDLE h1, h2;
    int ret;

    TRACE("(%s %s)\n", debugstr_w(path1), debugstr_w(path2));

    h1 = CreateFileW(path1, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, 0);
    h2 = CreateFileW(path2, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, 0);
    ret = equivalent_handles(h1, h2);
    CloseHandle(h1);
    CloseHandle(h2);
    return ret;
}

/* _Equivalent, msvcp140 version */
int __cdecl _Equivalent(WCHAR const* path1, WCHAR const* path2)
{
    HANDLE h1, h2;
    int ret;

    TRACE("(%s %s)\n", debugstr_w(path1), debugstr_w(path2));

    h1 = CreateFileW(path1, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    h2 = CreateFileW(path2, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ret = equivalent_handles(h1, h2);
    CloseHandle(h1);
    CloseHandle(h2);
    return ret;
}

/* ?_Current_get@sys@tr2@std@@YAPA_WAAY0BAE@_W@Z */
/* ?_Current_get@sys@tr2@std@@YAPEA_WAEAY0BAE@_W@Z */
WCHAR* __cdecl tr2_sys__Current_get_wchar(WCHAR *current_path)
{
    TRACE("(%s)\n", debugstr_w(current_path));

    if(!GetCurrentDirectoryW(MAX_PATH, current_path))
        return NULL;
    return current_path;
}

/* _Current_get, msvcp140 version */
BOOL __cdecl _Current_get(WCHAR *current_path)
{
    TRACE("(%s)\n", debugstr_w(current_path));

    if(!GetCurrentDirectoryW(MAX_PATH, current_path))
        return FALSE;
    return TRUE;
}

/* ?_Current_set@sys@tr2@std@@YA_NPB_W@Z */
/* ?_Current_set@sys@tr2@std@@YA_NPEB_W@Z */
bool __cdecl tr2_sys__Current_set_wchar(WCHAR const* path)
{
    TRACE("(%s)\n", debugstr_w(path));
    return SetCurrentDirectoryW(path) != 0;
}

/* ?_Make_dir@sys@tr2@std@@YAHPB_W@Z */
/* ?_Make_dir@sys@tr2@std@@YAHPEB_W@Z */
int __cdecl tr2_sys__Make_dir_wchar(WCHAR const* path)
{
    TRACE("(%s)\n", debugstr_w(path));

    if(!CreateDirectoryW(path, NULL)) {
        if(GetLastError() == ERROR_ALREADY_EXISTS)
            return 0;
        else
            return -1;
    }

    return 1;
}

/* ?_Remove_dir@sys@tr2@std@@YA_NPB_W@Z */
/* ?_Remove_dir@sys@tr2@std@@YA_NPEB_W@Z */
bool __cdecl tr2_sys__Remove_dir_wchar(WCHAR const* path)
{
    TRACE("(%s)\n", debugstr_w(path));
    return RemoveDirectoryW(path) != 0;
}

/* ?_Copy_file@sys@tr2@std@@YAHPB_W0_N@Z */
/* ?_Copy_file@sys@tr2@std@@YAHPEB_W0_N@Z */
int __cdecl tr2_sys__Copy_file_wchar(WCHAR const* source, WCHAR const* dest, bool fail_if_exists)
{
    TRACE("(%s %s %x)\n", debugstr_w(source), debugstr_w(dest), fail_if_exists);

    if(CopyFileW(source, dest, fail_if_exists))
        return ERROR_SUCCESS;
    return GetLastError();
}

/* ?_Rename@sys@tr2@std@@YAHPB_W0@Z */
/* ?_Rename@sys@tr2@std@@YAHPEB_W0@Z */
int __cdecl tr2_sys__Rename_wchar(WCHAR const* old_path, WCHAR const* new_path)
{
    TRACE("(%s %s)\n", debugstr_w(old_path), debugstr_w(new_path));

    if(MoveFileExW(old_path, new_path, MOVEFILE_COPY_ALLOWED))
        return ERROR_SUCCESS;
    return GetLastError();
}

/* ?_Statvfs@sys@tr2@std@@YA?AUspace_info@123@PB_W@Z */
/* ?_Statvfs@sys@tr2@std@@YA?AUspace_info@123@PEB_W@Z */
struct space_info* __cdecl tr2_sys__Statvfs_wchar(struct space_info *ret, const WCHAR* path)
{
    ULARGE_INTEGER available, total, free;

    TRACE("(%s)\n", debugstr_w(path));

    if(!path || !GetDiskFreeSpaceExW(path, &available, &total, &free)) {
        ret->capacity = ret->free = ret->available = 0;
    }else {
        ret->capacity = total.QuadPart;
        ret->free = free.QuadPart;
        ret->available = available.QuadPart;
    }
    return ret;
}

/* ?_Stat@sys@tr2@std@@YA?AW4file_type@123@PB_WAAH@Z */
/* ?_Stat@sys@tr2@std@@YA?AW4file_type@123@PEB_WAEAH@Z */
enum file_type __cdecl tr2_sys__Stat_wchar(WCHAR const* path, int* err_code)
{
    DWORD attr;
    TRACE("(%s %p)\n", debugstr_w(path), err_code);
    if(!path) {
        *err_code = ERROR_INVALID_PARAMETER;
        return status_unknown;
    }

    attr=GetFileAttributesW(path);
    if(attr == INVALID_FILE_ATTRIBUTES) {
        enum file_type ret;
        switch(GetLastError()) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_BAD_NETPATH:
            case ERROR_INVALID_NAME:
            case ERROR_BAD_PATHNAME:
            case ERROR_PATH_NOT_FOUND:
                ret = file_not_found;
                *err_code = ERROR_SUCCESS;
                break;
            default:
                ret = status_unknown;
                *err_code = GetLastError();
        }
        return ret;
    }

    *err_code = ERROR_SUCCESS;
    return (attr & FILE_ATTRIBUTE_DIRECTORY)?directory_file:regular_file;
}

/* _Stat, msvcp140 version */
enum file_type __cdecl _Stat(WCHAR const* path, int* permissions)
{
    DWORD attr;
    TRACE("(%s %p)\n", debugstr_w(path), permissions);
    if(!path) {
        return file_not_found;
    }

    attr=GetFileAttributesW(path);
    if(attr == INVALID_FILE_ATTRIBUTES) {
        enum file_type ret;
        switch(GetLastError()) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_BAD_NETPATH:
            case ERROR_INVALID_NAME:
            case ERROR_BAD_PATHNAME:
            case ERROR_PATH_NOT_FOUND:
                ret = file_not_found;
                break;
            default:
                ret = status_unknown;
        }
        return ret;
    }

    if (permissions)
        *permissions = (attr & FILE_ATTRIBUTE_READONLY) ? 0555 : 0777;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) ? directory_file : regular_file;
}

/* _Copy_file, msvcp140 version */
DWORD __cdecl _Copy_file(WCHAR const* src, WCHAR const* dst)
{
    TRACE("src %s, dst %s.\n", debugstr_w(src), debugstr_w(dst));

    if (CopyFileW(src, dst, FALSE))
        return ERROR_SUCCESS;

    return GetLastError();
}

/* ?_Lstat@sys@tr2@std@@YA?AW4file_type@123@PB_WAAH@Z */
/* ?_Lstat@sys@tr2@std@@YA?AW4file_type@123@PEB_WAEAH@Z */
enum file_type __cdecl tr2_sys__Lstat_wchar(WCHAR const* path, int* err_code)
{
    return tr2_sys__Stat_wchar(path, err_code);
}

/* _Lstat, msvcp140 version */
enum file_type __cdecl _Lstat(WCHAR const* path, int* permissions)
{
    return _Stat(path, permissions);
}

WCHAR * __cdecl _Temp_get(WCHAR *dst)
{
    GetTempPathW(MAX_PATH, dst);
    return dst;
}

/* ??1_Winit@std@@QAE@XZ */
/* ??1_Winit@std@@QAE@XZ */
DEFINE_THISCALL_WRAPPER(_Winit_dtor, 4)
void __thiscall _Winit_dtor(void *this)
{
    TRACE("(%p)\n", this);

    _Winit__Init_cnt--;
    if(!_Winit__Init_cnt) {
        basic_ostream_wchar_flush(&wcout.obj);
        basic_ostream_wchar_flush(&wcerr.obj);
        basic_ostream_wchar_flush(&wclog.obj);
    }
}

/* ??4_Winit@std@@QAEAAV01@ABV01@@Z */
/* ??4_Winit@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(_Winit_op_assign, 8)
void* __thiscall _Winit_op_assign(void *this, void *rhs)
{
    TRACE("(%p %p)\n", this, rhs);
    return this;
}

void init_io(void *base)
{
#ifdef RTTI_USE_RVA
    init_iosb_rtti(base);
    init_ios_base_rtti(base);
    init_basic_ios_char_rtti(base);
    init_basic_ios_wchar_rtti(base);
    init_basic_ios_short_rtti(base);
    init_basic_streambuf_char_rtti(base);
    init_basic_streambuf_wchar_rtti(base);
    init_basic_streambuf_short_rtti(base);
    init_basic_filebuf_char_rtti(base);
    init_basic_filebuf_wchar_rtti(base);
    init_basic_filebuf_short_rtti(base);
    init_basic_stringbuf_char_rtti(base);
    init_basic_stringbuf_wchar_rtti(base);
    init_basic_stringbuf_short_rtti(base);
    init_basic_ostream_char_rtti(base);
    init_basic_ostream_wchar_rtti(base);
    init_basic_ostream_short_rtti(base);
    init_basic_istream_char_rtti(base);
    init_basic_istream_wchar_rtti(base);
    init_basic_istream_short_rtti(base);
    init_basic_iostream_char_rtti(base);
    init_basic_iostream_wchar_rtti(base);
    init_basic_iostream_short_rtti(base);
    init_basic_ofstream_char_rtti(base);
    init_basic_ofstream_wchar_rtti(base);
    init_basic_ofstream_short_rtti(base);
    init_basic_ifstream_char_rtti(base);
    init_basic_ifstream_wchar_rtti(base);
    init_basic_ifstream_short_rtti(base);
    init_basic_fstream_char_rtti(base);
    init_basic_fstream_wchar_rtti(base);
    init_basic_fstream_short_rtti(base);
    init_basic_ostringstream_char_rtti(base);
    init_basic_ostringstream_wchar_rtti(base);
    init_basic_ostringstream_short_rtti(base);
    init_basic_istringstream_char_rtti(base);
    init_basic_istringstream_wchar_rtti(base);
    init_basic_istringstream_short_rtti(base);
    init_basic_stringstream_char_rtti(base);
    init_basic_stringstream_wchar_rtti(base);
    init_basic_stringstream_short_rtti(base);
    init_strstreambuf_rtti(base);
    init_strstream_rtti(base);
    init_ostrstream_rtti(base);
#endif

    basic_filebuf_char_ctor_file(&filebuf_char_stdin, stdin);
    basic_istream_char_ctor(&cin.obj, &filebuf_char_stdin.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_short_ctor_file(&filebuf_short_stdin, stdin);
    basic_istream_short_ctor(&ucin.obj, &filebuf_short_stdin.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_wchar_ctor_file(&filebuf_wchar_stdin, stdin);
    basic_istream_wchar_ctor(&wcin.obj, &filebuf_wchar_stdin.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_char_ctor_file(&filebuf_char_stdout, stdout);
    basic_ostream_char_ctor(&cout.obj, &filebuf_char_stdout.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_short_ctor_file(&filebuf_short_stdout, stdout);
    basic_ostream_short_ctor(&ucout.obj, &filebuf_short_stdout.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_wchar_ctor_file(&filebuf_wchar_stdout, stdout);
    basic_ostream_wchar_ctor(&wcout.obj, &filebuf_wchar_stdout.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_char_ctor_file(&filebuf_char_stderr, stderr);
    basic_ostream_char_ctor(&cerr.obj, &filebuf_char_stderr.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_short_ctor_file(&filebuf_short_stderr, stderr);
    basic_ostream_short_ctor(&ucerr.obj, &filebuf_short_stderr.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_wchar_ctor_file(&filebuf_wchar_stderr, stderr);
    basic_ostream_wchar_ctor(&wcerr.obj, &filebuf_wchar_stderr.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_char_ctor_file(&filebuf_char_log, stderr);
    basic_ostream_char_ctor(&MSVCP_clog.obj, &filebuf_char_log.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_short_ctor_file(&filebuf_short_log, stderr);
    basic_ostream_short_ctor(&uclog.obj, &filebuf_short_log.base, FALSE/*FIXME*/, TRUE);

    basic_filebuf_wchar_ctor_file(&filebuf_wchar_log, stderr);
    basic_ostream_wchar_ctor(&wclog.obj, &filebuf_wchar_log.base, FALSE/*FIXME*/, TRUE);
}

void free_io(void)
{
    basic_istream_char_vbase_dtor(&cin.obj);
    basic_filebuf_char_dtor(&filebuf_char_stdin);

    basic_istream_wchar_vbase_dtor(&ucin.obj);
    basic_filebuf_wchar_dtor(&filebuf_short_stdin);

    basic_istream_wchar_vbase_dtor(&wcin.obj);
    basic_filebuf_wchar_dtor(&filebuf_wchar_stdin);

    basic_ostream_char_vbase_dtor(&cout.obj);
    basic_filebuf_char_dtor(&filebuf_char_stdout);

    basic_ostream_wchar_vbase_dtor(&ucout.obj);
    basic_filebuf_wchar_dtor(&filebuf_short_stdout);

    basic_ostream_wchar_vbase_dtor(&wcout.obj);
    basic_filebuf_wchar_dtor(&filebuf_wchar_stdout);

    basic_ostream_char_vbase_dtor(&cerr.obj);
    basic_filebuf_char_dtor(&filebuf_char_stderr);

    basic_ostream_wchar_vbase_dtor(&ucerr.obj);
    basic_filebuf_wchar_dtor(&filebuf_short_stderr);

    basic_ostream_wchar_vbase_dtor(&wcerr.obj);
    basic_filebuf_wchar_dtor(&filebuf_wchar_stderr);

    basic_ostream_char_vbase_dtor(&MSVCP_clog.obj);
    basic_filebuf_char_dtor(&filebuf_char_log);

    basic_ostream_wchar_vbase_dtor(&uclog.obj);
    basic_filebuf_wchar_dtor(&filebuf_short_log);

    basic_ostream_wchar_vbase_dtor(&wclog.obj);
    basic_filebuf_wchar_dtor(&filebuf_wchar_log);
}

/* ?_Cin_func@std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@1@XZ */
/* ?_Cin_func@std@@YAAEAV?$basic_istream@DU?$char_traits@D@std@@@1@XZ */
basic_istream_char* __cdecl _Cin_func(void)
{
    return &cin.obj;
}

/* ?_Wcin_func@std@@YAAAV?$basic_istream@GU?$char_traits@G@std@@@1@XZ */
/* ?_Wcin_func@std@@YAAEAV?$basic_istream@GU?$char_traits@G@std@@@1@XZ */
basic_istream_wchar* __cdecl _Wcin_func_short(void)
{
    return &ucin.obj;
}

/* ?_Wcin_func@std@@YAAAV?$basic_istream@_WU?$char_traits@_W@std@@@1@XZ */
/* ?_Wcin_func@std@@YAAEAV?$basic_istream@_WU?$char_traits@_W@std@@@1@XZ */
basic_istream_wchar* __cdecl _Wcin_func(void)
{
    return &wcin.obj;
}

/* ?_Cout_func@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@1@XZ */
/* ?_Cout_func@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@1@XZ */
basic_ostream_char* __cdecl _Cout_func(void)
{
    return &cout.obj;
}

/* ?_Wcout_func@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@1@XZ */
/* ?_Wcout_func@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@1@XZ */
basic_ostream_wchar* __cdecl _Wcout_func_short(void)
{
    return &ucout.obj;
}


/* ?_Wcout_func@std@@YAAAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@XZ */
/* ?_Wcout_func@std@@YAAEAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@XZ */
basic_ostream_wchar* __cdecl _Wcout_func(void)
{
    return &wcout.obj;
}

/* ?_Clog_func@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@1@XZ */
/* ?_Clog_func@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@1@XZ */
basic_ostream_char* __cdecl _Clog_func(void)
{
    return &MSVCP_clog.obj;
}

/* ?_Wclog_func@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@1@XZ */
/* ?_Wclog_func@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@1@XZ */
basic_ostream_wchar* __cdecl _Wclog_func_short(void)
{
    return &uclog.obj;
}

/* ?_Wclog_func@std@@YAAAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@XZ */
/* ?_Wclog_func@std@@YAAEAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@XZ */
basic_ostream_wchar* __cdecl _Wclog_func(void)
{
    return &wclog.obj;
}

/* ?_Cerr_func@std@@YAAAV?$basic_ostream@DU?$char_traits@D@std@@@1@XZ */
/* ?_Cerr_func@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@1@XZ */
basic_ostream_char* __cdecl _Cerr_func(void)
{
    return &cerr.obj;
}

/* ?_Wcerr_func@std@@YAAAV?$basic_ostream@GU?$char_traits@G@std@@@1@XZ */
/* ?_Wcerr_func@std@@YAAEAV?$basic_ostream@GU?$char_traits@G@std@@@1@XZ */
basic_ostream_wchar* __cdecl _Wcerr_func_short(void)
{
    return &ucerr.obj;
}

/* ?_Wcerr_func@std@@YAAAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@XZ */
/* ?_Wcerr_func@std@@YAAEAV?$basic_ostream@_WU?$char_traits@_W@std@@@1@XZ */
basic_ostream_wchar* __cdecl _Wcerr_func(void)
{
    return &wcerr.obj;
}
