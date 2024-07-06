/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include "assert.h"
#include "locale.h"
#include "errno.h"
#include "limits.h"
#include "math.h"
#include "mbctype.h"
#include "wchar.h"
#include "wctype.h"
#include "uchar.h"
#include "time.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "msvcp90.h"
#include "wine/heap.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

typedef enum {
    DATEORDER_no_order,
    DATEORDER_dmy,
    DATEORDER_mdy,
    DATEORDER_ymd,
    DATEORDER_ydm
} dateorder;

char* __cdecl _Getdays(void);
wchar_t* __cdecl _W_Getdays(void);
char* __cdecl _Getmonths(void);
wchar_t* __cdecl _W_Getmonths(void);
void* __cdecl _Gettnames(void);
#ifndef __REACTOS__
unsigned int __cdecl ___lc_codepage_func(void);
int __cdecl ___lc_collate_cp_func(void);
const unsigned short* __cdecl __pctype_func(void);
#endif
const locale_facet* __thiscall locale__Getfacet(const locale*, size_t);
const locale* __cdecl locale_classic(void);

#if _MSVCP_VER >= 110
wchar_t ** __cdecl ___lc_locale_name_func(void);
#else
LCID* __cdecl ___lc_handle_func(void);
#endif

#if _MSVCP_VER < 100
#define locale_string basic_string_char
#define locale_string_char_ctor(this)           MSVCP_basic_string_char_ctor(this)
#define locale_string_char_ctor_cstr(this,str)  MSVCP_basic_string_char_ctor_cstr(this,str)
#define locale_string_char_copy_ctor(this,copy) MSVCP_basic_string_char_copy_ctor(this,copy)
#define locale_string_char_dtor(this)           MSVCP_basic_string_char_dtor(this)
#define locale_string_char_c_str(this)          MSVCP_basic_string_char_c_str(this)
#define locale_string_char_assign(this,assign)  MSVCP_basic_string_char_assign(this,assign)
#else
#define locale_string _Yarn_char
#define locale_string_char_ctor(this)           _Yarn_char_ctor(this)
#define locale_string_char_ctor_cstr(this,str)  _Yarn_char_ctor_cstr(this,str)
#define locale_string_char_copy_ctor(this,copy) _Yarn_char_copy_ctor(this,copy)
#define locale_string_char_dtor(this)           _Yarn_char_dtor(this)
#define locale_string_char_c_str(this)          _Yarn_char_c_str(this)
#define locale_string_char_assign(this,assign)  _Yarn_char_op_assign(this,assign)

#define locale_string_wchar _Yarn_wchar
#define locale_string_wchar_ctor(this)           _Yarn_wchar_ctor(this)
#define locale_string_wchar_dtor(this)           _Yarn_wchar_dtor(this)
#define locale_string_wchar_c_str(this)          _Yarn_wchar__C_str(this)
#define locale_string_wchar_assign(this,str)     _Yarn_wchar_op_assign_cstr(this,str)
#endif

typedef int category;

typedef struct {
    size_t id;
} locale_id;

typedef struct _locale__Locimp {
    locale_facet facet;
    locale_facet **facetvec;
    size_t facet_cnt;
    category catmask;
    bool transparent;
    locale_string name;
} locale__Locimp;

typedef struct {
    void *timeptr;
} _Timevec;

typedef struct {
    _Lockit lock;
    locale_string days;
    locale_string months;
#if _MSVCP_VER >= 110
    locale_string_wchar wdays;
    locale_string_wchar wmonths;
#endif
    locale_string oldlocname;
    locale_string newlocname;
} _Locinfo;

typedef struct {
#if _MSVCP_VER < 110
    LCID handle;
#endif
    unsigned page;
#if _MSVCP_VER >= 110
    wchar_t *lc_name;
#endif
} _Collvec;

typedef struct {
    locale_facet facet;
    _Collvec coll;
} collate;

typedef struct {
    locale_facet facet;
    const char *grouping;
    char dp;
    char sep;
    const char *false_name;
    const char *true_name;
} numpunct_char;

typedef struct {
    locale_facet facet;
    const char *grouping;
    wchar_t dp;
    wchar_t sep;
    const wchar_t *false_name;
    const wchar_t *true_name;
} numpunct_wchar;

typedef struct {
    locale_facet facet;
    _Timevec time;
#if _MSVCP_VER <= 100
    _Cvtvec cvt;
#endif
} time_put;

typedef struct {
    locale_facet facet;
    const char *days;
    const char *months;
    dateorder dateorder;
    _Cvtvec cvt;
} time_get_char;

typedef struct {
    locale_facet facet;
    const wchar_t *days;
    const wchar_t *months;
    dateorder dateorder;
    _Cvtvec cvt;
} time_get_wchar;

/* ?_Id_cnt@id@locale@std@@0HA */
int locale_id__Id_cnt = 0;

static locale classic_locale;

/* ?_Global@_Locimp@locale@std@@0PAV123@A */
/* ?_Global@_Locimp@locale@std@@0PEAV123@EA */
locale__Locimp *global_locale = NULL;

/* ?_Clocptr@_Locimp@locale@std@@0PAV123@A */
/* ?_Clocptr@_Locimp@locale@std@@0PEAV123@EA */
locale__Locimp *locale__Locimp__Clocptr = NULL;

static char istreambuf_iterator_char_val(istreambuf_iterator_char *this)
{
    if(this->strbuf && !this->got) {
        int c = basic_streambuf_char_sgetc(this->strbuf);
        if(c == EOF)
            this->strbuf = NULL;
        else
            this->val = c;
    }

    this->got = TRUE;
    return this->val;
}

static wchar_t istreambuf_iterator_wchar_val(istreambuf_iterator_wchar *this)
{
    if(this->strbuf && !this->got) {
        unsigned short c = basic_streambuf_wchar_sgetc(this->strbuf);
        if(c == WEOF)
            this->strbuf = NULL;
        else
            this->val = c;
    }

    this->got = TRUE;
    return this->val;
}

static void istreambuf_iterator_char_inc(istreambuf_iterator_char *this)
{
    if(!this->strbuf || basic_streambuf_char_sbumpc(this->strbuf)==EOF) {
        this->strbuf = NULL;
        this->got = TRUE;
    }else {
        this->got = FALSE;
        istreambuf_iterator_char_val(this);
    }
}

static void istreambuf_iterator_wchar_inc(istreambuf_iterator_wchar *this)
{
    if(!this->strbuf || basic_streambuf_wchar_sbumpc(this->strbuf)==WEOF) {
        this->strbuf = NULL;
        this->got = TRUE;
    }else {
        this->got = FALSE;
        istreambuf_iterator_wchar_val(this);
    }
}

static void ostreambuf_iterator_char_put(ostreambuf_iterator_char *this, char ch)
{
    if(this->failed || basic_streambuf_char_sputc(this->strbuf, ch)==EOF)
        this->failed = TRUE;
}

static void ostreambuf_iterator_wchar_put(ostreambuf_iterator_wchar *this, wchar_t ch)
{
    if(this->failed || basic_streambuf_wchar_sputc(this->strbuf, ch)==WEOF)
        this->failed = TRUE;
}

/* ??1facet@locale@std@@UAE@XZ */
/* ??1facet@locale@std@@UEAA@XZ */
/* ??1facet@locale@std@@MAA@XZ */
/* ??1facet@locale@std@@MAE@XZ */
/* ??1facet@locale@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(locale_facet_dtor, 4)
void __thiscall locale_facet_dtor(locale_facet *this)
{
    TRACE("(%p)\n", this);
}

DEFINE_THISCALL_WRAPPER(locale_facet_vector_dtor, 8)
#define call_locale_facet_vector_dtor(this, flags) CALL_VTBL_FUNC(this, 0, \
        locale_facet*, (locale_facet*, unsigned int), (this, flags))
locale_facet* __thiscall locale_facet_vector_dtor(locale_facet *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            locale_facet_dtor(this+i);
        operator_delete(ptr);
    } else {
        locale_facet_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

typedef struct
{
    locale_facet *fac;
    struct list entry;
} facets_elem;
static struct list lazy_facets = LIST_INIT(lazy_facets);

/* Not exported from msvcp90 */
/* ?facet_Register@facet@locale@std@@CAXPAV123@@Z */
/* ?facet_Register@facet@locale@std@@CAXPEAV123@@Z */
void __cdecl locale_facet_register(locale_facet *add)
{
    facets_elem *head = operator_new(sizeof(*head));
    head->fac = add;
    list_add_head(&lazy_facets, &head->entry);
}

/* Not exported from msvcp90 */
/* ?_Register@facet@locale@std@@QAEXXZ */
/* ?_Register@facet@locale@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(locale_facet__Register, 4)
void __thiscall locale_facet__Register(locale_facet *this)
{
    TRACE("(%p)\n", this);
    locale_facet_register(this);
}

/* Not exported from msvcp90 */
/* ??_7facet@locale@std@@6B@ */
extern const vtable_ptr locale_facet_vtable;

/* ??0id@locale@std@@QAE@I@Z */
/* ??0id@locale@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(locale_id_ctor_id, 8)
locale_id* __thiscall locale_id_ctor_id(locale_id *this, size_t id)
{
    TRACE("(%p %Iu)\n", this, id);

    this->id = id;
    return this;
}

/* ??_Fid@locale@std@@QAEXXZ */
/* ??_Fid@locale@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(locale_id_ctor, 4)
locale_id* __thiscall locale_id_ctor(locale_id *this)
{
    TRACE("(%p)\n", this);

    this->id = 0;
    return this;
}

/* ??Bid@locale@std@@QAEIXZ */
/* ??Bid@locale@std@@QEAA_KXZ */
DEFINE_THISCALL_WRAPPER(locale_id_operator_size_t, 4)
size_t __thiscall locale_id_operator_size_t(locale_id *this)
{
    _Lockit lock;

    TRACE("(%p)\n", this);

    if(!this->id) {
        _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
        this->id = ++locale_id__Id_cnt;
        _Lockit_dtor(&lock);
    }

    return this->id;
}

/* ?_Id_cnt_func@id@locale@std@@CAAAHXZ */
/* ?_Id_cnt_func@id@locale@std@@CAAEAHXZ */
int* __cdecl locale_id__Id_cnt_func(void)
{
    TRACE("\n");
    return &locale_id__Id_cnt;
}

/* ??_Ffacet@locale@std@@QAEXXZ */
/* ??_Ffacet@locale@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(locale_facet_ctor, 4)
locale_facet* __thiscall locale_facet_ctor(locale_facet *this)
{
    TRACE("(%p)\n", this);
    this->vtable = &locale_facet_vtable;
    this->refs = 0;
    return this;
}

/* ??0facet@locale@std@@IAE@I@Z */
/* ??0facet@locale@std@@IEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(locale_facet_ctor_refs, 8)
locale_facet* __thiscall locale_facet_ctor_refs(locale_facet *this, size_t refs)
{
    TRACE("(%p %Iu)\n", this, refs);
    this->vtable = &locale_facet_vtable;
    this->refs = refs;
    return this;
}

/* ?_Incref@facet@locale@std@@QAEXXZ */
/* ?_Incref@facet@locale@std@@QEAAXXZ */
/* ?_Incref@facet@locale@std@@UAEXXZ */
/* ?_Incref@facet@locale@std@@UEAAXXZ */
#if _MSVCP_VER >= 110
#define call_locale_facet__Incref(this) CALL_VTBL_FUNC(this, 4, void, (locale_facet*), (this))
#else
#define call_locale_facet__Incref locale_facet__Incref
#endif
DEFINE_THISCALL_WRAPPER(locale_facet__Incref, 4)
void __thiscall locale_facet__Incref(locale_facet *this)
{
    _Lockit lock;

    TRACE("(%p)\n", this);

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    this->refs++;
    _Lockit_dtor(&lock);
}

/* ?_Decref@facet@locale@std@@QAEPAV123@XZ */
/* ?_Decref@facet@locale@std@@QEAAPEAV123@XZ */
/* ?_Decref@facet@locale@std@@UAEPAV_Facet_base@3@XZ */
/* ?_Decref@facet@locale@std@@UEAAPEAV_Facet_base@3@XZ */
#if _MSVCP_VER >= 110
#define call_locale_facet__Decref(this) CALL_VTBL_FUNC(this, 8, \
        locale_facet*, (locale_facet*), (this))
#else
#define call_locale_facet__Decref locale_facet__Decref
#endif
DEFINE_THISCALL_WRAPPER(locale_facet__Decref, 4)
locale_facet* __thiscall locale_facet__Decref(locale_facet *this)
{
    _Lockit lock;
    locale_facet *ret;

    TRACE("(%p)\n", this);

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    if(this->refs)
        this->refs--;

    ret = this->refs ? NULL : this;
    _Lockit_dtor(&lock);

    return ret;
}

/* ?_Getcat@facet@locale@std@@SAIPAPBV123@PBV23@@Z */
/* ?_Getcat@facet@locale@std@@SA_KPEAPEBV123@PEBV23@@Z */
size_t __cdecl locale_facet__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);
    return -1;
}

/* ?_Getcat@facet@locale@std@@SAIPAPBV123@@Z */
/* ?_Getcat@facet@locale@std@@SA_KPEAPEBV123@@Z */
size_t __cdecl locale_facet__Getcat_old(const locale_facet **facet)
{
    TRACE("(%p)\n", facet);
    return -1;
}

/* ??0_Timevec@std@@QAE@ABV01@@Z */
/* ??0_Timevec@std@@QEAA@AEBV01@@Z */
/* This copy constructor modifies copied object */
DEFINE_THISCALL_WRAPPER(_Timevec_copy_ctor, 8)
_Timevec* __thiscall _Timevec_copy_ctor(_Timevec *this, _Timevec *copy)
{
    TRACE("(%p %p)\n", this, copy);
    this->timeptr = copy->timeptr;
    copy->timeptr = NULL;
    return this;
}

/* ??0_Timevec@std@@QAE@PAX@Z */
/* ??0_Timevec@std@@QEAA@PEAX@Z */
DEFINE_THISCALL_WRAPPER(_Timevec_ctor_timeptr, 8)
_Timevec* __thiscall _Timevec_ctor_timeptr(_Timevec *this, void *timeptr)
{
    TRACE("(%p %p)\n", this, timeptr);
    this->timeptr = timeptr;
    return this;
}

/* ??_F_Timevec@std@@QAEXXZ */
/* ??_F_Timevec@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Timevec_ctor, 4)
_Timevec* __thiscall _Timevec_ctor(_Timevec *this)
{
    TRACE("(%p)\n", this);
    this->timeptr = NULL;
    return this;
}

/* ??1_Timevec@std@@QAE@XZ */
/* ??1_Timevec@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Timevec_dtor, 4)
void __thiscall _Timevec_dtor(_Timevec *this)
{
    TRACE("(%p)\n", this);
    free(this->timeptr);
}

/* ??4_Timevec@std@@QAEAAV01@ABV01@@Z */
/* ??4_Timevec@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(_Timevec_op_assign, 8)
_Timevec* __thiscall _Timevec_op_assign(_Timevec *this, _Timevec *right)
{
    TRACE("(%p %p)\n", this, right);
    this->timeptr = right->timeptr;
    right->timeptr = NULL;
    return this;
}

/* ?_Getptr@_Timevec@std@@QBEPAXXZ */
/* ?_Getptr@_Timevec@std@@QEBAPEAXXZ */
DEFINE_THISCALL_WRAPPER(_Timevec__Getptr, 4)
void* __thiscall _Timevec__Getptr(_Timevec *this)
{
    TRACE("(%p)\n", this);
    return this->timeptr;
}

/* ?_Locinfo_ctor@_Locinfo@std@@SAXPAV12@HPBD@Z */
/* ?_Locinfo_ctor@_Locinfo@std@@SAXPEAV12@HPEBD@Z */
_Locinfo* __cdecl _Locinfo__Locinfo_ctor_cat_cstr(_Locinfo *locinfo, int category, const char *locstr)
{
    const char *locale = NULL;

    TRACE("(%p %d %s)\n", locinfo, category, locstr);

    if(!locstr)
        _Xruntime_error("bad locale name");

    _Lockit_ctor_locktype(&locinfo->lock, _LOCK_LOCALE);
    locale_string_char_ctor(&locinfo->days);
    locale_string_char_ctor(&locinfo->months);
#if _MSVCP_VER >= 110
    locale_string_wchar_ctor(&locinfo->wdays);
    locale_string_wchar_ctor(&locinfo->wmonths);
#endif
    locale_string_char_ctor_cstr(&locinfo->oldlocname, setlocale(LC_ALL, NULL));

    if(category)
        locale = setlocale(LC_ALL, locstr);
    else
        locale = setlocale(LC_ALL, NULL);

    if(locale)
        locale_string_char_ctor_cstr(&locinfo->newlocname, locale);
    else
        locale_string_char_ctor_cstr(&locinfo->newlocname, "*");

    return locinfo;
}

/* ??0_Locinfo@std@@QAE@HPBD@Z */
/* ??0_Locinfo@std@@QEAA@HPEBD@Z */
DEFINE_THISCALL_WRAPPER(_Locinfo_ctor_cat_cstr, 12)
_Locinfo* __thiscall _Locinfo_ctor_cat_cstr(_Locinfo *this, int category, const char *locstr)
{
    return _Locinfo__Locinfo_ctor_cat_cstr(this, category, locstr);
}

/* ?_Locinfo_ctor@_Locinfo@std@@SAXPAV12@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
/* ?_Locinfo_ctor@_Locinfo@std@@SAXPEAV12@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
_Locinfo* __cdecl _Locinfo__Locinfo_ctor_bstr(_Locinfo *locinfo, const basic_string_char *locstr)
{
    return _Locinfo__Locinfo_ctor_cat_cstr(locinfo, 1/*FIXME*/, MSVCP_basic_string_char_c_str(locstr));
}

/* ??0_Locinfo@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
/* ??0_Locinfo@std@@QEAA@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@@Z */
DEFINE_THISCALL_WRAPPER(_Locinfo_ctor_bstr, 8)
_Locinfo* __thiscall _Locinfo_ctor_bstr(_Locinfo *this, const basic_string_char *locstr)
{
    return _Locinfo__Locinfo_ctor_cat_cstr(this, 1/*FIXME*/, MSVCP_basic_string_char_c_str(locstr));
}

/* ?_Locinfo_ctor@_Locinfo@std@@SAXPAV12@PBD@Z */
/* ?_Locinfo_ctor@_Locinfo@std@@SAXPEAV12@PEBD@Z */
_Locinfo* __cdecl _Locinfo__Locinfo_ctor_cstr(_Locinfo *locinfo, const char *locstr)
{
    return _Locinfo__Locinfo_ctor_cat_cstr(locinfo, 1/*FIXME*/, locstr);
}

/* ??0_Locinfo@std@@QAE@PBD@Z */
/* ??0_Locinfo@std@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(_Locinfo_ctor_cstr, 8)
_Locinfo* __thiscall _Locinfo_ctor_cstr(_Locinfo *this, const char *locstr)
{
    return _Locinfo__Locinfo_ctor_cat_cstr(this, 1/*FIXME*/, locstr);
}

/* ?_Locinfo_dtor@_Locinfo@std@@SAXPAV12@@Z */
/* ?_Locinfo_dtor@_Locinfo@std@@SAXPEAV12@@Z */
void __cdecl _Locinfo__Locinfo_dtor(_Locinfo *locinfo)
{
    TRACE("(%p)\n", locinfo);

    setlocale(LC_ALL, locale_string_char_c_str(&locinfo->oldlocname));
    locale_string_char_dtor(&locinfo->days);
    locale_string_char_dtor(&locinfo->months);
#if _MSVCP_VER >= 110
    locale_string_wchar_dtor(&locinfo->wdays);
    locale_string_wchar_dtor(&locinfo->wmonths);
#endif
    locale_string_char_dtor(&locinfo->oldlocname);
    locale_string_char_dtor(&locinfo->newlocname);
    _Lockit_dtor(&locinfo->lock);
}

/* ??_F_Locinfo@std@@QAEXXZ */
/* ??_F_Locinfo@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo_ctor, 4)
_Locinfo* __thiscall _Locinfo_ctor(_Locinfo *this)
{
    return _Locinfo__Locinfo_ctor_cat_cstr(this, 1/*FIXME*/, "C");
}

/* ??1_Locinfo@std@@QAE@XZ */
/* ??1_Locinfo@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo_dtor, 4)
void __thiscall _Locinfo_dtor(_Locinfo *this)
{
    _Locinfo__Locinfo_dtor(this);
}

/* ?_Locinfo_Addcats@_Locinfo@std@@SAAAV12@PAV12@HPBD@Z */
/* ?_Locinfo_Addcats@_Locinfo@std@@SAAEAV12@PEAV12@HPEBD@Z */
_Locinfo* __cdecl _Locinfo__Locinfo_Addcats(_Locinfo *locinfo, int category, const char *locstr)
{
    const char *locale = NULL;

    TRACE("(%p %d %s)\n", locinfo, category, locstr);
    if(!locstr)
        _Xruntime_error("bad locale name");

    locale_string_char_dtor(&locinfo->newlocname);

    if(category)
        locale = setlocale(LC_ALL, locstr);
    else
        locale = setlocale(LC_ALL, NULL);

    if(locale)
        locale_string_char_ctor_cstr(&locinfo->newlocname, locale);
    else
        locale_string_char_ctor_cstr(&locinfo->newlocname, "*");

    return locinfo;
}

/* ?_Addcats@_Locinfo@std@@QAEAAV12@HPBD@Z */
/* ?_Addcats@_Locinfo@std@@QEAAAEAV12@HPEBD@Z */
DEFINE_THISCALL_WRAPPER(_Locinfo__Addcats, 12)
_Locinfo* __thiscall _Locinfo__Addcats(_Locinfo *this, int category, const char *locstr)
{
    return _Locinfo__Locinfo_Addcats(this, category, locstr);
}

static _Collvec* getcoll(_Collvec *ret)
{
    TRACE("\n");

    ret->page = ___lc_collate_cp_func();
#if _MSVCP_VER < 110
    ret->handle = ___lc_handle_func()[LC_COLLATE];
#else
    ret->lc_name = ___lc_locale_name_func()[LC_COLLATE];
#endif
    return ret;
}

/* _Getcoll */
#if defined(__i386__)
/* Work around a gcc bug */
ULONGLONG __cdecl _Getcoll(void)
{
    C_ASSERT(sizeof(_Collvec) == sizeof(ULONGLONG));
    ULONGLONG ret;

    getcoll((_Collvec*)&ret);
    return ret;
}
#else
_Collvec __cdecl _Getcoll(void)
{
    _Collvec ret;

    getcoll(&ret);
    return ret;
}
#endif

/* ?_Getcoll@_Locinfo@std@@QBE?AU_Collvec@@XZ */
/* ?_Getcoll@_Locinfo@std@@QEBA?AU_Collvec@@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getcoll, 8)
_Collvec* __thiscall _Locinfo__Getcoll(const _Locinfo *this, _Collvec *ret)
{
    return getcoll(ret);
}

/* _Getctype */
_Ctypevec __cdecl _Getctype(void)
{
    _Ctypevec ret;
    short *table;
#if _MSVCP_VER >= 110
    wchar_t *name;
    size_t size;
#endif

    TRACE("\n");

    ret.page = ___lc_codepage_func();
#if _MSVCP_VER < 110
    ret.handle = ___lc_handle_func()[LC_COLLATE];
#else
    if((name = ___lc_locale_name_func()[LC_COLLATE])) {
        size = wcslen(name)+1;
        ret.name = operator_new(size*sizeof(*name));
        memcpy(ret.name, name, size*sizeof(*name));
    } else {
        ret.name = NULL;
    }
#endif
    ret.delfl = TRUE;
    table = operator_new(sizeof(short[256]));
    memcpy(table, __pctype_func(), sizeof(short[256]));
    ret.table = table;
    return ret;
}

/* ?_Getctype@_Locinfo@std@@QBE?AU_Ctypevec@@XZ */
/* ?_Getctype@_Locinfo@std@@QEBA?AU_Ctypevec@@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getctype, 8)
_Ctypevec* __thiscall _Locinfo__Getctype(const _Locinfo *this, _Ctypevec *ret)
{
    *ret = _Getctype();
    return ret;
}

/* _Getcvt */
#if _MSVCP_VER < 110 && defined(__i386__)
/* Work around a gcc bug */
ULONGLONG __cdecl _Getcvt(void)
{
    C_ASSERT(sizeof(_Cvtvec) == sizeof(ULONGLONG));
    union {
        _Cvtvec cvtvec;
        ULONGLONG ull;
    } ret;

    TRACE("\n");

    ret.cvtvec.page = ___lc_codepage_func();
    ret.cvtvec.handle = ___lc_handle_func()[LC_CTYPE];
    return ret.ull;
}
#elif _MSVCP_VER < 110
_Cvtvec __cdecl _Getcvt(void)
{
    _Cvtvec ret;

    TRACE("\n");

    ret.page = ___lc_codepage_func();
    ret.handle = ___lc_handle_func()[LC_CTYPE];
    return ret;
}
#else
_Cvtvec __cdecl _Getcvt(void)
{
    _Cvtvec ret;
    int i;

    TRACE("\n");

    memset(&ret, 0, sizeof(ret));
    ret.page = ___lc_codepage_func();
    ret.mb_max = ___mb_cur_max_func();

    if(ret.mb_max > 1) {
        for(i=0; i<256; i++)
            if(_ismbblead(i)) ret.isleadbyte[i/8] |= 1 << (i&7);
    }
    return ret;
}
#endif

/* ?_Getcvt@_Locinfo@std@@QBE?AU_Cvtvec@@XZ */
/* ?_Getcvt@_Locinfo@std@@QEBA?AU_Cvtvec@@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getcvt, 8)
_Cvtvec* __thiscall _Locinfo__Getcvt(const _Locinfo *this, _Cvtvec *ret)
{
#if _MSVCP_VER < 110 && defined(__i386__)
    ULONGLONG cvtvec;
#else
    _Cvtvec cvtvec;
#endif

    cvtvec = _Getcvt();
    memcpy(ret, &cvtvec, sizeof(cvtvec));
    return ret;
}

int __cdecl _Getdateorder(void)
{
    WCHAR date_fmt[2];

#if _MSVCP_VER < 110
    if(!GetLocaleInfoW(___lc_handle_func()[LC_TIME], LOCALE_ILDATE,
                date_fmt, ARRAY_SIZE(date_fmt)))
        return DATEORDER_no_order;
#else
    if(!GetLocaleInfoEx(___lc_locale_name_func()[LC_TIME], LOCALE_ILDATE,
                date_fmt, ARRAY_SIZE(date_fmt)))
        return DATEORDER_no_order;
#endif

    if(*date_fmt == '0') return DATEORDER_mdy;
    if(*date_fmt == '1') return DATEORDER_dmy;
    if(*date_fmt == '2') return DATEORDER_ymd;
    return DATEORDER_no_order;
}

/* ?_Getdateorder@_Locinfo@std@@QBEHXZ */
/* ?_Getdateorder@_Locinfo@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getdateorder, 4)
int __thiscall _Locinfo__Getdateorder(const _Locinfo *this)
{
    TRACE("(%p)\n", this);
    return _Getdateorder();
}

/* ?_Getdays@_Locinfo@std@@QBEPBDXZ */
/* ?_Getdays@_Locinfo@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getdays, 4)
const char* __thiscall _Locinfo__Getdays(const _Locinfo *this)
{
    char *days = _Getdays();
    const char *ret;

    TRACE("(%p)\n", this);

    if(days) {
        locale_string_char_dtor((locale_string *)&this->days);
        locale_string_char_ctor_cstr((locale_string *)&this->days, days);
        free(days);
    }

    ret = locale_string_char_c_str(&this->days);
    if (!ret[0]) ret = ":Sun:Sunday:Mon:Monday:Tue:Tuesday:Wed:Wednesday:Thu:Thursday:Fri:Friday:Sat:Saturday";
    return ret;
}

#if _MSVCP_VER >= 110
/* ?_W_Getdays@_Locinfo@std@@QBEPBGXZ */
/* ?_W_Getdays@_Locinfo@std@@QEBAPEBGXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__W_Getdays, 4)
const wchar_t* __thiscall _Locinfo__W_Getdays(const _Locinfo *this)
{
    static const wchar_t defdays[] =
    {
        ':','S','u','n',':','S','u','n','d','a','y',
        ':','M','o','n',':','M','o','n','d','a','y',
        ':','T','u','e',':','T','u','e','s','d','a','y',
        ':','W','e','d',':','W','e','d','n','e','s','d','a','y',
        ':','T','h','u',':','T','h','u','r','s','d','a','y',
        ':','F','r','i',':','F','r','i','d','a','y',
        ':','S','a','t',':','S','a','t','u','r','d','a','y'
    };
    wchar_t *wdays = _W_Getdays();
    const wchar_t *ret;

    TRACE("(%p)\n", this);

    if(wdays) {
        locale_string_wchar_assign((locale_string_wchar *)&this->wdays, wdays);
        free(wdays);
    }

    ret = locale_string_wchar_c_str(&this->wdays);
    if (!ret[0]) ret = defdays;
    return ret;
}

/* ?_W_Getmonths@_Locinfo@std@@QBEPBGXZ */
/* ?_W_Getmonths@_Locinfo@std@@QEBAPEBGXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__W_Getmonths, 4)
const wchar_t* __thiscall _Locinfo__W_Getmonths(const _Locinfo *this)
{
    static const wchar_t defmonths[] =
    {
        ':','J','a','n',':','J','a','n','u','a','r','y',
        ':','F','e','b',':','F','e','b','r','u','a','r','y',
        ':','M','a','r',':','M','a','r','c','h',
        ':','A','p','r',':','A','p','r','i','l',
        ':','M','a','y',':','M','a','y',
        ':','J','u','n',':','J','u','n','e',
        ':','J','u','l',':','J','u','l','y',
        ':','A','u','g',':','A','u','g','u','s','t',
        ':','S','e','p',':','S','e','p','t','e','m','b','e','r',
        ':','O','c','t',':','O','c','t','o','b','e','r',
        ':','N','o','v',':','N','o','v','e','m','b','e','r',
        ':','D','e','c',':','D','e','c','e','m','b','e','r'
    };
    wchar_t *wmonths = _W_Getmonths();
    const wchar_t *ret;

    TRACE("(%p)\n", this);

    if(wmonths) {
        locale_string_wchar_assign((locale_string_wchar *)&this->wmonths, wmonths);
        free(wmonths);
    }

    ret = locale_string_wchar_c_str(&this->wmonths);
    if (!ret[0]) ret = defmonths;
    return ret;
}
#endif

/* ?_Getmonths@_Locinfo@std@@QBEPBDXZ */
/* ?_Getmonths@_Locinfo@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getmonths, 4)
const char* __thiscall _Locinfo__Getmonths(const _Locinfo *this)
{
    char *months = _Getmonths();
    const char *ret;

    TRACE("(%p)\n", this);

    if(months) {
        locale_string_char_dtor((locale_string *)&this->months);
        locale_string_char_ctor_cstr((locale_string *)&this->months, months);
        free(months);
    }

    ret = locale_string_char_c_str(&this->months);
    if (!ret[0]) ret = ":Jan:January:Feb:February:Mar:March:Apr:April:May:May:Jun:June:Jul:July"
                       ":Aug:August:Sep:September:Oct:October:Nov:November:Dec:December";
    return ret;
}

/* ?_Getfalse@_Locinfo@std@@QBEPBDXZ */
/* ?_Getfalse@_Locinfo@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getfalse, 4)
const char* __thiscall _Locinfo__Getfalse(const _Locinfo *this)
{
    TRACE("(%p)\n", this);
    return "false";
}

/* ?_Gettrue@_Locinfo@std@@QBEPBDXZ */
/* ?_Gettrue@_Locinfo@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Gettrue, 4)
const char* __thiscall _Locinfo__Gettrue(const _Locinfo *this)
{
    TRACE("(%p)\n", this);
    return "true";
}

/* ?_Getlconv@_Locinfo@std@@QBEPBUlconv@@XZ */
/* ?_Getlconv@_Locinfo@std@@QEBAPEBUlconv@@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getlconv, 4)
const struct lconv* __thiscall _Locinfo__Getlconv(const _Locinfo *this)
{
    TRACE("(%p)\n", this);
    return localeconv();
}

/* ?_Gettnames@_Locinfo@std@@QBE?AV_Timevec@2@XZ */
/* ?_Gettnames@_Locinfo@std@@QEBA?AV_Timevec@2@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Gettnames, 8)
_Timevec*__thiscall _Locinfo__Gettnames(const _Locinfo *this, _Timevec *ret)
{
    TRACE("(%p)\n", this);

    _Timevec_ctor_timeptr(ret, _Gettnames());
    return ret;
}

/* ?id@?$collate@D@std@@2V0locale@2@A */
locale_id collate_char_id = {0};

/* ??_7?$collate@D@std@@6B@ */
extern const vtable_ptr collate_char_vtable;

/* ?_Init@?$collate@D@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$collate@D@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(collate_char__Init, 8)
void __thiscall collate_char__Init(collate *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Getcoll(locinfo, &this->coll);
}

/* ??0?$collate@D@std@@IAE@PBDI@Z */
/* ??0?$collate@D@std@@IEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(collate_char_ctor_name, 12)
collate* __thiscall collate_char_ctor_name(collate *this, const char *name, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %s %Iu)\n", this, name, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &collate_char_vtable;

    _Locinfo_ctor_cstr(&locinfo, name);
    collate_char__Init(this, &locinfo);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$collate@D@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$collate@D@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_char_ctor_locinfo, 12)
collate* __thiscall collate_char_ctor_locinfo(collate *this, const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &collate_char_vtable;
    collate_char__Init(this, locinfo);
    return this;
}

/* ??0?$collate@D@std@@QAE@I@Z */
/* ??0?$collate@D@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_char_ctor_refs, 8)
collate* __thiscall collate_char_ctor_refs(collate *this, size_t refs)
{
    return collate_char_ctor_name(this, "C", refs);
}

/* ??1?$collate@D@std@@UAE@XZ */
/* ??1?$collate@D@std@@UEAA@XZ */
/* ??1?$collate@D@std@@MAE@XZ */
/* ??1?$collate@D@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(collate_char_dtor, 4)
void __thiscall collate_char_dtor(collate *this)
{
    TRACE("(%p)\n", this);
}

DEFINE_THISCALL_WRAPPER(collate_char_vector_dtor, 8)
collate* __thiscall collate_char_vector_dtor(collate *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            collate_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        collate_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ??_F?$collate@D@std@@QAEXXZ */
/* ??_F?$collate@D@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(collate_char_ctor, 4)
collate* __thiscall collate_char_ctor(collate *this)
{
    return collate_char_ctor_name(this, "C", 0);
}

/* ?_Getcat@?$collate@D@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$collate@D@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl collate_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = operator_new(sizeof(collate));
        collate_char_ctor_name((collate*)*facet,
                locale_string_char_c_str(&loc->ptr->name), 0);
    }

    return LC_COLLATE;
}

/* ?_Getcat@?$collate@D@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$collate@D@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl collate_char__Getcat_old(const locale_facet **facet)
{
    return collate_char__Getcat(facet, locale_classic());
}

static collate* collate_char_use_facet(const locale *loc)
{
    static collate *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&collate_char_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (collate*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    collate_char__Getcat(&fac, loc);
    obj = (collate*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* _Strcoll */
int __cdecl _Strcoll(const char *first1, const char *last1, const char *first2,
        const char *last2, const _Collvec *coll)
{
    LCID lcid;

    TRACE("(%s %s)\n", debugstr_an(first1, last1-first1), debugstr_an(first2, last2-first2));

#if _MSVCP_VER < 110
    lcid = (coll ? coll->handle : ___lc_handle_func()[LC_COLLATE]);
#else
    lcid = LocaleNameToLCID(coll ? coll->lc_name : ___lc_locale_name_func()[LC_COLLATE], 0);
#endif
    return CompareStringA(lcid, 0, first1, last1-first1, first2, last2-first2)-CSTR_EQUAL;
}

/* ?do_compare@?$collate@D@std@@MBEHPBD000@Z */
/* ?do_compare@?$collate@D@std@@MEBAHPEBD000@Z */
DEFINE_THISCALL_WRAPPER(collate_char_do_compare, 20)
#if _MSVCP_VER <= 100
#define call_collate_char_do_compare(this, first1, last1, first2, last2) CALL_VTBL_FUNC(this, 4, int, \
        (const collate*, const char*, const char*, const char*, const char*), \
        (this, first1, last1, first2, last2))
#else
#define call_collate_char_do_compare(this, first1, last1, first2, last2) CALL_VTBL_FUNC(this, 12, int, \
        (const collate*, const char*, const char*, const char*, const char*), \
        (this, first1, last1, first2, last2))
#endif
int __thiscall collate_char_do_compare(const collate *this, const char *first1,
        const char *last1, const char *first2, const char *last2)
{
    TRACE("(%p %p %p %p %p)\n", this, first1, last1, first2, last2);
    return _Strcoll(first1, last1, first2, last2, &this->coll);
}

/* ?compare@?$collate@D@std@@QBEHPBD000@Z */
/* ?compare@?$collate@D@std@@QEBAHPEBD000@Z */
DEFINE_THISCALL_WRAPPER(collate_char_compare, 20)
int __thiscall collate_char_compare(const collate *this, const char *first1,
        const char *last1, const char *first2, const char *last2)
{
    TRACE("(%p %p %p %p %p)\n", this, first1, last1, first2, last2);
    return call_collate_char_do_compare(this, first1, last1, first2, last2);
}

/* ?do_hash@?$collate@D@std@@MBEJPBD0@Z */
/* ?do_hash@?$collate@D@std@@MEBAJPEBD0@Z */
DEFINE_THISCALL_WRAPPER(collate_char_do_hash, 12)
#if _MSVCP_VER <= 100
#define call_collate_char_do_hash(this, first, last) CALL_VTBL_FUNC(this, 12, LONG, \
        (const collate*, const char*, const char*), (this, first, last))
#else
#define call_collate_char_do_hash(this, first, last) CALL_VTBL_FUNC(this, 20, LONG, \
        (const collate*, const char*, const char*), (this, first, last))
#endif
LONG __thiscall collate_char_do_hash(const collate *this,
        const char *first, const char *last)
{
    ULONG ret = 0;

    TRACE("(%p %p %p)\n", this, first, last);

    for(; first<last; first++)
        ret = (ret<<8 | ret>>24) + *first;
    return ret;
}

/* ?hash@?$collate@D@std@@QBEJPBD0@Z */
/* ?hash@?$collate@D@std@@QEBAJPEBD0@Z */
DEFINE_THISCALL_WRAPPER(collate_char_hash, 12)
LONG __thiscall collate_char_hash(const collate *this,
        const char *first, const char *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    return call_collate_char_do_hash(this, first, last);
}

/* ?do_transform@?$collate@D@std@@MBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PBD0@Z */
/* ?do_transform@?$collate@D@std@@MEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PEBD0@Z */
DEFINE_THISCALL_WRAPPER(collate_char_do_transform, 16)
basic_string_char* __thiscall collate_char_do_transform(const collate *this,
        basic_string_char *ret, const char *first, const char *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return ret;
}

/* ?transform@?$collate@D@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PBD0@Z */
/* ?transform@?$collate@D@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PEBD0@Z */
DEFINE_THISCALL_WRAPPER(collate_char_transform, 16)
basic_string_char* __thiscall collate_char_transform(const collate *this,
        basic_string_char *ret, const char *first, const char *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return ret;
}

/* ?id@?$collate@_W@std@@2V0locale@2@A */
locale_id collate_wchar_id = {0};
/* ?id@?$collate@G@std@@2V0locale@2@A */
locale_id collate_short_id = {0};

/* ??_7?$collate@_W@std@@6B@ */
extern const vtable_ptr collate_wchar_vtable;
/* ??_7?$collate@G@std@@6B@ */
extern const vtable_ptr collate_short_vtable;

/* ?_Init@?$collate@_W@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$collate@_W@std@@IEAAXAEBV_Locinfo@2@@Z */
/* ?_Init@?$collate@G@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$collate@G@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar__Init, 8)
void __thiscall collate_wchar__Init(collate *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Getcoll(locinfo, &this->coll);
}

/* ??0?$collate@_W@std@@IAE@PBDI@Z */
/* ??0?$collate@_W@std@@IEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_ctor_name, 12)
collate* __thiscall collate_wchar_ctor_name(collate *this, const char *name, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %s %Iu)\n", this, name, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &collate_wchar_vtable;

    _Locinfo_ctor_cstr(&locinfo, name);
    collate_wchar__Init(this, &locinfo);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$collate@G@std@@IAE@PBDI@Z */
/* ??0?$collate@G@std@@IEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(collate_short_ctor_name, 12)
collate* __thiscall collate_short_ctor_name(collate *this, const char *name, size_t refs)
{
    collate *ret = collate_wchar_ctor_name(this, name, refs);
    ret->facet.vtable = &collate_short_vtable;
    return ret;
}

/* ??0?$collate@_W@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$collate@_W@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_ctor_locinfo, 12)
collate* __thiscall collate_wchar_ctor_locinfo(collate *this, const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &collate_wchar_vtable;
    collate_wchar__Init(this, locinfo);
    return this;
}

/* ??0?$collate@G@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$collate@G@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_short_ctor_locinfo, 12)
collate* __thiscall collate_short_ctor_locinfo(collate *this, const _Locinfo *locinfo, size_t refs)
{
    collate *ret = collate_wchar_ctor_locinfo(this, locinfo, refs);
    ret->facet.vtable = &collate_short_vtable;
    return ret;
}

/* ??0?$collate@_W@std@@QAE@I@Z */
/* ??0?$collate@_W@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_ctor_refs, 8)
collate* __thiscall collate_wchar_ctor_refs(collate *this, size_t refs)
{
    return collate_wchar_ctor_name(this, "C", refs);
}

/* ??0?$collate@G@std@@QAE@I@Z */
/* ??0?$collate@G@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(collate_short_ctor_refs, 8)
collate* __thiscall collate_short_ctor_refs(collate *this, size_t refs)
{
    collate *ret = collate_wchar_ctor_refs(this, refs);
    ret->facet.vtable = &collate_short_vtable;
    return ret;
}

/* ??1?$collate@G@std@@UAE@XZ */
/* ??1?$collate@G@std@@UEAA@XZ */
/* ??1?$collate@_W@std@@MAE@XZ */
/* ??1?$collate@_W@std@@MEAA@XZ */
/* ??1?$collate@G@std@@MAE@XZ */
/* ??1?$collate@G@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(collate_wchar_dtor, 4)
void __thiscall collate_wchar_dtor(collate *this)
{
    TRACE("(%p)\n", this);
}

DEFINE_THISCALL_WRAPPER(collate_wchar_vector_dtor, 8)
collate* __thiscall collate_wchar_vector_dtor(collate *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            collate_wchar_dtor(this+i);
        operator_delete(ptr);
    } else {
        collate_wchar_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ??_F?$collate@_W@std@@QAEXXZ */
/* ??_F?$collate@_W@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(collate_wchar_ctor, 4)
collate* __thiscall collate_wchar_ctor(collate *this)
{
    return collate_wchar_ctor_name(this, "C", 0);
}

/* ??_F?$collate@G@std@@QAEXXZ */
/* ??_F?$collate@G@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(collate_short_ctor, 4)
collate* __thiscall collate_short_ctor(collate *this)
{
    collate *ret = collate_wchar_ctor(this);
    ret->facet.vtable = &collate_short_vtable;
    return ret;
}

/* ?_Getcat@?$collate@_W@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$collate@_W@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl collate_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = operator_new(sizeof(collate));
        collate_wchar_ctor_name((collate*)*facet,
                locale_string_char_c_str(&loc->ptr->name), 0);
    }

    return LC_COLLATE;
}

/* ?_Getcat@?$collate@_W@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$collate@_W@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl collate_wchar__Getcat_old(const locale_facet **facet)
{
    return collate_wchar__Getcat(facet, locale_classic());
}

static collate* collate_wchar_use_facet(const locale *loc)
{
    static collate *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&collate_wchar_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (collate*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    collate_wchar__Getcat(&fac, loc);
    obj = (collate*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Getcat@?$collate@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$collate@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl collate_short__Getcat(const locale_facet **facet, const locale *loc)
{
    if(facet && !*facet) {
        collate_wchar__Getcat(facet, loc);
        (*(locale_facet**)facet)->vtable = &collate_short_vtable;
    }

    return LC_COLLATE;
}

/* ?_Getcat@?$collate@G@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$collate@G@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl collate_short__Getcat_old(const locale_facet **facet)
{
    return collate_short__Getcat(facet, locale_classic());
}

static collate* collate_short_use_facet(const locale *loc)
{
    static collate *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&collate_short_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (collate*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    collate_short__Getcat(&fac, loc);
    obj = (collate*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* _Wcscoll */
int __cdecl _Wcscoll(const wchar_t *first1, const wchar_t *last1, const wchar_t *first2,
        const wchar_t *last2, const _Collvec *coll)
{
    TRACE("(%s %s)\n", debugstr_wn(first1, last1-first1), debugstr_wn(first2, last2-first2));

#if _MSVCP_VER < 110
    return CompareStringW(coll ? coll->handle : ___lc_handle_func()[LC_COLLATE],
            0, first1, last1-first1, first2, last2-first2)-CSTR_EQUAL;
#else
    return CompareStringEx(coll ? coll->lc_name : ___lc_locale_name_func()[LC_COLLATE],
            0, first1, last1-first1, first2, last2-first2, NULL, NULL, 0)-CSTR_EQUAL;
#endif
}

/* ?do_compare@?$collate@_W@std@@MBEHPB_W000@Z */
/* ?do_compare@?$collate@_W@std@@MEBAHPEB_W000@Z */
/* ?do_compare@?$collate@G@std@@MBEHPBG000@Z */
/* ?do_compare@?$collate@G@std@@MEBAHPEBG000@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_do_compare, 20)
#if _MSVCP_VER <= 100
#define call_collate_wchar_do_compare(this, first1, last1, first2, last2) CALL_VTBL_FUNC(this, 4, int, \
        (const collate*, const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*), \
        (this, first1, last1, first2, last2))
#else
#define call_collate_wchar_do_compare(this, first1, last1, first2, last2) CALL_VTBL_FUNC(this, 12, int, \
        (const collate*, const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*), \
        (this, first1, last1, first2, last2))
#endif
int __thiscall collate_wchar_do_compare(const collate *this, const wchar_t *first1,
        const wchar_t *last1, const wchar_t *first2, const wchar_t *last2)
{
    TRACE("(%p %p %p %p %p)\n", this, first1, last1, first2, last2);
    return _Wcscoll(first1, last1, first2, last2, &this->coll);
}

/* ?compare@?$collate@_W@std@@QBEHPB_W000@Z */
/* ?compare@?$collate@_W@std@@QEBAHPEB_W000@Z */
/* ?compare@?$collate@G@std@@QBEHPBG000@Z */
/* ?compare@?$collate@G@std@@QEBAHPEBG000@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_compare, 20)
int __thiscall collate_wchar_compare(const collate *this, const wchar_t *first1,
        const wchar_t *last1, const wchar_t *first2, const wchar_t *last2)
{
    TRACE("(%p %p %p %p %p)\n", this, first1, last1, first2, last2);
    return call_collate_wchar_do_compare(this, first1, last1, first2, last2);
}

/* ?do_hash@?$collate@_W@std@@MBEJPB_W0@Z */
/* ?do_hash@?$collate@_W@std@@MEBAJPEB_W0@Z */
/* ?do_hash@?$collate@G@std@@MBEJPBG0@Z */
/* ?do_hash@?$collate@G@std@@MEBAJPEBG0@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_do_hash, 12)
#if _MSVCP_VER <= 100
#define call_collate_wchar_do_hash(this, first, last) CALL_VTBL_FUNC(this, 12, LONG, \
        (const collate*, const wchar_t*, const wchar_t*), (this, first, last))
#else
#define call_collate_wchar_do_hash(this, first, last) CALL_VTBL_FUNC(this, 20, LONG, \
        (const collate*, const wchar_t*, const wchar_t*), (this, first, last))
#endif
LONG __thiscall collate_wchar_do_hash(const collate *this,
        const wchar_t *first, const wchar_t *last)
{
    ULONG ret = 0;

    TRACE("(%p %p %p)\n", this, first, last);

    for(; first<last; first++)
        ret = (ret<<8 | ret>>24) + *first;
    return ret;
}

/* ?hash@?$collate@_W@std@@QBEJPB_W0@Z */
/* ?hash@?$collate@_W@std@@QEBAJPEB_W0@Z */
/* ?hash@?$collate@G@std@@QBEJPBG0@Z */
/* ?hash@?$collate@G@std@@QEBAJPEBG0@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_hash, 12)
LONG __thiscall collate_wchar_hash(const collate *this,
        const wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    return call_collate_wchar_do_hash(this, first, last);
}

/* ?do_transform@?$collate@_W@std@@MBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PB_W0@Z */
/* ?do_transform@?$collate@_W@std@@MEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PEB_W0@Z */
/* ?do_transform@?$collate@G@std@@MBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PBG0@Z */
/* ?do_transform@?$collate@G@std@@MEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PEBG0@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_do_transform, 16)
basic_string_wchar* __thiscall collate_wchar_do_transform(const collate *this,
        basic_string_wchar *ret, const wchar_t *first, const wchar_t *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return ret;
}

/* ?transform@?$collate@_W@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PB_W0@Z */
/* ?transform@?$collate@_W@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PEB_W0@Z */
/* ?transform@?$collate@G@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PBG0@Z */
/* ?transform@?$collate@G@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PEBG0@Z */
DEFINE_THISCALL_WRAPPER(collate_wchar_transform, 16)
basic_string_wchar* __thiscall collate_wchar_transform(const collate *this,
        basic_string_wchar *ret, const wchar_t *first, const wchar_t *last)
{
    FIXME("(%p %p %p) stub\n", this, first, last);
    return ret;
}

/* ??_7ctype_base@std@@6B@ */
extern const vtable_ptr ctype_base_vtable;

/* ??0ctype_base@std@@QAE@I@Z */
/* ??0ctype_base@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_base_ctor_refs, 8)
ctype_base* __thiscall ctype_base_ctor_refs(ctype_base *this, size_t refs)
{
    TRACE("(%p %Iu)\n", this, refs);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &ctype_base_vtable;
    return this;
}

/* ??_Fctype_base@std@@QAEXXZ */
/* ??_Fctype_base@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(ctype_base_ctor, 4)
ctype_base* __thiscall ctype_base_ctor(ctype_base *this)
{
    TRACE("(%p)\n", this);
    locale_facet_ctor_refs(&this->facet, 0);
    this->facet.vtable = &ctype_base_vtable;
    return this;
}

/* ??1ctype_base@std@@UAE@XZ */
/* ??1ctype_base@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(ctype_base_dtor, 4)
void __thiscall ctype_base_dtor(ctype_base *this)
{
    TRACE("(%p)\n", this);
}

DEFINE_THISCALL_WRAPPER(ctype_base_vector_dtor, 8)
ctype_base* __thiscall ctype_base_vector_dtor(ctype_base *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            ctype_base_dtor(this+i);
        operator_delete(ptr);
    } else {
        ctype_base_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Xran@ctype_base@std@@KAXXZ */
void __cdecl ctype_base__Xran(void)
{
    _Xout_of_range("out of range in ctype<T>");
}

/* ?id@?$ctype@D@std@@2V0locale@2@A */
locale_id ctype_char_id = {0};
/* ?table_size@?$ctype@D@std@@2IB */
/* ?table_size@?$ctype@D@std@@2_KB */
size_t ctype_char_table_size = 256;

/* ??_7?$ctype@D@std@@6B@ */
extern const vtable_ptr ctype_char_vtable;

/* ?_Id_func@?$ctype@D@std@@SAAAVid@locale@2@XZ */
/* ?_Id_func@?$ctype@D@std@@SAAEAVid@locale@2@XZ */
locale_id* __cdecl ctype_char__Id_func(void)
{
    TRACE("()\n");
    return &ctype_char_id;
}

/* ?_Init@?$ctype@D@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$ctype@D@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(ctype_char__Init, 8)
void __thiscall ctype_char__Init(ctype_char *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Getctype(locinfo, &this->ctype);
}

/* ?_Tidy@?$ctype@D@std@@IAEXXZ */
/* ?_Tidy@?$ctype@D@std@@IEAAXXZ */
DEFINE_THISCALL_WRAPPER(ctype_char__Tidy, 4)
void __thiscall ctype_char__Tidy(ctype_char *this)
{
    TRACE("(%p)\n", this);

    if(this->ctype.delfl)
        free((short*)this->ctype.table);
#if _MSVCP_VER >= 110
    free(this->ctype.name);
#endif
}

/* ?classic_table@?$ctype@D@std@@KAPBFXZ */
/* ?classic_table@?$ctype@D@std@@KAPEBFXZ */
const short* __cdecl ctype_char_classic_table(void)
{
    ctype_char *ctype;

    TRACE("()\n");
    ctype = ctype_char_use_facet( locale_classic() );
    return ctype->ctype.table;
}

/* ??0?$ctype@D@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$ctype@D@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_ctor_locinfo, 12)
ctype_char* __thiscall ctype_char_ctor_locinfo(ctype_char *this,
        const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);
    ctype_base_ctor_refs(&this->base, refs);
    this->base.facet.vtable = &ctype_char_vtable;
    ctype_char__Init(this, locinfo);
    return this;
}

/* ??0?$ctype@D@std@@QAE@PBF_NI@Z */
/* ??0?$ctype@D@std@@QEAA@PEBF_N_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_ctor_table, 16)
ctype_char* __thiscall ctype_char_ctor_table(ctype_char *this,
        const short *table, bool delete, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %p %d %Iu)\n", this, table, delete, refs);

    ctype_base_ctor_refs(&this->base, refs);
    this->base.facet.vtable = &ctype_char_vtable;

    _Locinfo_ctor(&locinfo);
    ctype_char__Init(this, &locinfo);
    _Locinfo_dtor(&locinfo);

    if(table) {
        ctype_char__Tidy(this);
        this->ctype.table = table;
        this->ctype.delfl = delete;
    }
    return this;
}

/* ??_F?$ctype@D@std@@QAEXXZ */
/* ??_F?$ctype@D@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(ctype_char_ctor, 4)
ctype_char* __thiscall ctype_char_ctor(ctype_char *this)
{
    return ctype_char_ctor_table(this, NULL, FALSE, 0);
}

/* ??1?$ctype@D@std@@UAE@XZ */
/* ??1?$ctype@D@std@@UEAA@XZ */
/* ??1?$ctype@D@std@@MAE@XZ */
/* ??1?$ctype@D@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(ctype_char_dtor, 4)
void __thiscall ctype_char_dtor(ctype_char *this)
{
    TRACE("(%p)\n", this);
    ctype_char__Tidy(this);
}

DEFINE_THISCALL_WRAPPER(ctype_char_vector_dtor, 8)
ctype_char* __thiscall ctype_char_vector_dtor(ctype_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            ctype_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        ctype_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?do_narrow@?$ctype@D@std@@MBEDDD@Z */
/* ?do_narrow@?$ctype@D@std@@MEBADDD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_narrow_ch, 12)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
#define call_ctype_char_do_narrow_ch(this, ch, unused) CALL_VTBL_FUNC(this, 36, \
        char, (const ctype_char*, char, char), (this, ch, unused))
#elif _MSVCP_VER <= 100
#define call_ctype_char_do_narrow_ch(this, ch, unused) CALL_VTBL_FUNC(this, 32, \
        char, (const ctype_char*, char, char), (this, ch, unused))
#else
#define call_ctype_char_do_narrow_ch(this, ch, unused) CALL_VTBL_FUNC(this, 40, \
        char, (const ctype_char*, char, char), (this, ch, unused))
#endif
char __thiscall ctype_char_do_narrow_ch(const ctype_char *this, char ch, char unused)
{
    TRACE("(%p %c %c)\n", this, ch, unused);
    return ch;
}

/* ?do_narrow@?$ctype@D@std@@MBEPBDPBD0DPAD@Z */
/* ?do_narrow@?$ctype@D@std@@MEBAPEBDPEBD0DPEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_narrow, 20)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
#define call_ctype_char_do_narrow(this, first, last, unused, dest) CALL_VTBL_FUNC(this, 32, \
        const char*, (const ctype_char*, const char*, const char*, char, char*), \
        (this, first, last, unused, dest))
#elif _MSVCP_VER <= 100
#define call_ctype_char_do_narrow(this, first, last, unused, dest) CALL_VTBL_FUNC(this, 28, \
        const char*, (const ctype_char*, const char*, const char*, char, char*), \
        (this, first, last, unused, dest))
#else
#define call_ctype_char_do_narrow(this, first, last, unused, dest) CALL_VTBL_FUNC(this, 36, \
        const char*, (const ctype_char*, const char*, const char*, char, char*), \
        (this, first, last, unused, dest))
#endif
const char* __thiscall ctype_char_do_narrow(const ctype_char *this,
        const char *first, const char *last, char unused, char *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    memcpy(dest, first, last-first);
    return last;
}

/* ?_Do_narrow_s@?$ctype@D@std@@MBEPBDPBD0DPADI@Z */
/* ?_Do_narrow_s@?$ctype@D@std@@MEBAPEBDPEBD0DPEAD_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char__Do_narrow_s, 24)
#define call_ctype_char__Do_narrow_s(this, first, last, unused, dest, size) CALL_VTBL_FUNC(this, 40, \
        const char*, (const ctype_char*, const char*, const char*, char, char*, size_t), \
        (this, first, last, unused, dest, size))
const char* __thiscall ctype_char__Do_narrow_s(const ctype_char *this, const char *first,
        const char *last, char unused, char *dest, size_t size)
{
    TRACE("(%p %p %p %p %Iu)\n", this, first, last, dest, size);
    memcpy_s(dest, size, first, last-first);
    return last;
}

/* ?narrow@?$ctype@D@std@@QBEDDD@Z */
/* ?narrow@?$ctype@D@std@@QEBADDD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_narrow_ch, 12)
char __thiscall ctype_char_narrow_ch(const ctype_char *this, char ch, char dflt)
{
    TRACE("(%p %c %c)\n", this, ch, dflt);
    return call_ctype_char_do_narrow_ch(this, ch, dflt);
}

/* ?narrow@?$ctype@D@std@@QBEPBDPBD0DPAD@Z */
/* ?narrow@?$ctype@D@std@@QEBAPEBDPEBD0DPEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_narrow, 20)
const char* __thiscall ctype_char_narrow(const ctype_char *this,
        const char *first, const char *last, char dflt, char *dest)
{
    TRACE("(%p %p %p %c %p)\n", this, first, last, dflt, dest);
    return call_ctype_char_do_narrow(this, first, last, dflt, dest);
}

/* ?_Narrow_s@?$ctype@D@std@@QBEPBDPBD0DPADI@Z */
/* ?_Narrow_s@?$ctype@D@std@@QEBAPEBDPEBD0DPEAD_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char__Narrow_s, 24)
const char* __thiscall ctype_char__Narrow_s(const ctype_char *this, const char *first,
        const char *last, char dflt, char *dest, size_t size)
{
    TRACE("(%p %p %p %p %Iu)\n", this, first, last, dest, size);
    return call_ctype_char__Do_narrow_s(this, first, last, dflt, dest, size);
}

/* ?do_widen@?$ctype@D@std@@MBEDD@Z */
/* ?do_widen@?$ctype@D@std@@MEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_widen_ch, 8)
#if _MSVCP_VER <= 100
#define call_ctype_char_do_widen_ch(this, ch) CALL_VTBL_FUNC(this, 24, \
        char, (const ctype_char*, char), (this, ch))
#else
#define call_ctype_char_do_widen_ch(this, ch) CALL_VTBL_FUNC(this, 32, \
        char, (const ctype_char*, char), (this, ch))
#endif
char __thiscall ctype_char_do_widen_ch(const ctype_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return ch;
}

/* ?do_widen@?$ctype@D@std@@MBEPBDPBD0PAD@Z */
/* ?do_widen@?$ctype@D@std@@MEBAPEBDPEBD0PEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_do_widen, 16)
#if _MSVCP_VER <= 100
#define call_ctype_char_do_widen(this, first, last, dest) CALL_VTBL_FUNC(this, 20, \
        const char*, (const ctype_char*, const char*, const char*, char*), \
        (this, first, last, dest))
#else
#define call_ctype_char_do_widen(this, first, last, dest) CALL_VTBL_FUNC(this, 28, \
        const char*, (const ctype_char*, const char*, const char*, char*), \
        (this, first, last, dest))
#endif
const char* __thiscall ctype_char_do_widen(const ctype_char *this,
        const char *first, const char *last, char *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    memcpy(dest, first, last-first);
    return last;
}

/* ?_Do_widen_s@?$ctype@D@std@@MBEPBDPBD0PADI@Z */
/* ?_Do_widen_s@?$ctype@D@std@@MEBAPEBDPEBD0PEAD_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char__Do_widen_s, 20)
#define call_ctype_char__Do_widen_s(this, first, last, dest, size) CALL_VTBL_FUNC(this, 28, \
        const char*, (const ctype_char*, const char*, const char*, char*, size_t), \
        (this, first, last, dest, size))
const char* __thiscall ctype_char__Do_widen_s(const ctype_char *this,
        const char *first, const char *last, char *dest, size_t size)
{
    TRACE("(%p %p %p %p %Iu)\n", this, first, last, dest, size);
    memcpy_s(dest, size, first, last-first);
    return last;
}

/* ?widen@?$ctype@D@std@@QBEDD@Z */
/* ?widen@?$ctype@D@std@@QEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_widen_ch, 8)
char __thiscall ctype_char_widen_ch(const ctype_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return call_ctype_char_do_widen_ch(this, ch);
}

/* ?widen@?$ctype@D@std@@QBEPBDPBD0PAD@Z */
/* ?widen@?$ctype@D@std@@QEBAPEBDPEBD0PEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_widen, 16)
const char* __thiscall ctype_char_widen(const ctype_char *this,
        const char *first, const char *last, char *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    return call_ctype_char_do_widen(this, first, last, dest);
}

/* ?_Widen_s@?$ctype@D@std@@QBEPBDPBD0PADI@Z */
/* ?_Widen_s@?$ctype@D@std@@QEBAPEBDPEBD0PEAD_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_char__Widen_s, 20)
const char* __thiscall ctype_char__Widen_s(const ctype_char *this,
        const char *first, const char *last, char *dest, size_t size)
{
    TRACE("(%p %p %p %p %Iu)\n", this, first, last, dest, size);
    return call_ctype_char__Do_widen_s(this, first, last, dest, size);
}

/* ?_Getcat@?$ctype@D@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$ctype@D@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl ctype_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = operator_new(sizeof(ctype_char));
        _Locinfo_ctor_cstr(&locinfo, locale_string_char_c_str(&loc->ptr->name));
        ctype_char_ctor_locinfo((ctype_char*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_CTYPE;
}

/* ?_Getcat@?$ctype@D@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$ctype@D@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl ctype_char__Getcat_old(const locale_facet **facet)
{
    return ctype_char__Getcat(facet, locale_classic());
}

ctype_char* ctype_char_use_facet(const locale *loc)
{
    static ctype_char *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&ctype_char_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (ctype_char*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    ctype_char__Getcat(&fac, loc);
    obj = (ctype_char*)fac;
    call_locale_facet__Incref(&obj->base.facet);
    locale_facet_register(&obj->base.facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* _Tolower */
int __cdecl _Tolower(int ch, const _Ctypevec *ctype)
{
    unsigned int cp;

    TRACE("%d %p\n", ch, ctype);

    if(ctype)
        cp = ctype->page;
    else
        cp = ___lc_codepage_func();

    /* Don't convert to unicode in case of C locale */
    if(!cp) {
        if(ch>='A' && ch<='Z')
            ch = ch-'A'+'a';
        return ch;
    } else {
        WCHAR wide, lower;
        char str[2];
        int size;

        if(ch > 255) {
            str[0] = (ch>>8) & 255;
            str[1] = ch & 255;
            size = 2;
        } else {
            str[0] = ch & 255;
            size = 1;
        }

        if(!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, str, size, &wide, 1))
            return ch;

        lower = towlower(wide);
        if(lower == wide)
            return ch;

        WideCharToMultiByte(cp, 0, &lower, 1, str, 2, NULL, NULL);

        return str[0] + (str[1]<<8);
    }
}

/* ?do_tolower@?$ctype@D@std@@MBEDD@Z */
/* ?do_tolower@?$ctype@D@std@@MEBADD@Z */
#if _MSVCP_VER <= 100
#define call_ctype_char_do_tolower_ch(this, ch) CALL_VTBL_FUNC(this, 8, \
        char, (const ctype_char*, char), (this, ch))
#else
#define call_ctype_char_do_tolower_ch(this, ch) CALL_VTBL_FUNC(this, 16, \
        char, (const ctype_char*, char), (this, ch))
#endif
DEFINE_THISCALL_WRAPPER(ctype_char_do_tolower_ch, 8)
char __thiscall ctype_char_do_tolower_ch(const ctype_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return _Tolower(ch, &this->ctype);
}

/* ?do_tolower@?$ctype@D@std@@MBEPBDPADPBD@Z */
/* ?do_tolower@?$ctype@D@std@@MEBAPEBDPEADPEBD@Z */
#if _MSVCP_VER <= 100
#define call_ctype_char_do_tolower(this, first, last) CALL_VTBL_FUNC(this, 4, \
        const char*, (const ctype_char*, char*, const char*), (this, first, last))
#else
#define call_ctype_char_do_tolower(this, first, last) CALL_VTBL_FUNC(this, 12, \
        const char*, (const ctype_char*, char*, const char*), (this, first, last))
#endif
DEFINE_THISCALL_WRAPPER(ctype_char_do_tolower, 12)
const char* __thiscall ctype_char_do_tolower(const ctype_char *this, char *first, const char *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    for(; first<last; first++)
        *first = _Tolower(*first, &this->ctype);
    return last;
}

/* ?tolower@?$ctype@D@std@@QBEDD@Z */
/* ?tolower@?$ctype@D@std@@QEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_tolower_ch, 8)
char __thiscall ctype_char_tolower_ch(const ctype_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return call_ctype_char_do_tolower_ch(this, ch);
}

/* ?tolower@?$ctype@D@std@@QBEPBDPADPBD@Z */
/* ?tolower@?$ctype@D@std@@QEBAPEBDPEADPEBD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_tolower, 12)
const char* __thiscall ctype_char_tolower(const ctype_char *this, char *first, const char *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    return call_ctype_char_do_tolower(this, first, last);
}

/* _Toupper */
int __cdecl _Toupper(int ch, const _Ctypevec *ctype)
{
    unsigned int cp;

    TRACE("%d %p\n", ch, ctype);

    if(ctype)
        cp = ctype->page;
    else
        cp = ___lc_codepage_func();

    /* Don't convert to unicode in case of C locale */
    if(!cp) {
        if(ch>='a' && ch<='z')
            ch = ch-'a'+'A';
        return ch;
    } else {
        WCHAR wide, upper;
        char str[2];
        int size;

        if(ch > 255) {
            str[0] = (ch>>8) & 255;
            str[1] = ch & 255;
            size = 2;
        } else {
            str[0] = ch & 255;
            size = 1;
        }

        if(!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, str, size, &wide, 1))
            return ch;

        upper = towupper(wide);
        if(upper == wide)
            return ch;

        WideCharToMultiByte(cp, 0, &upper, 1, str, 2, NULL, NULL);

        return str[0] + (str[1]<<8);
    }
}

/* ?do_toupper@?$ctype@D@std@@MBEDD@Z */
/* ?do_toupper@?$ctype@D@std@@MEBADD@Z */
#if _MSVCP_VER <= 100
#define call_ctype_char_do_toupper_ch(this, ch) CALL_VTBL_FUNC(this, 16, \
        char, (const ctype_char*, char), (this, ch))
#else
#define call_ctype_char_do_toupper_ch(this, ch) CALL_VTBL_FUNC(this, 24, \
        char, (const ctype_char*, char), (this, ch))
#endif
DEFINE_THISCALL_WRAPPER(ctype_char_do_toupper_ch, 8)
char __thiscall ctype_char_do_toupper_ch(const ctype_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return _Toupper(ch, &this->ctype);
}

/* ?do_toupper@?$ctype@D@std@@MBEPBDPADPBD@Z */
/* ?do_toupper@?$ctype@D@std@@MEBAPEBDPEADPEBD@Z */
#if _MSVCP_VER <= 100
#define call_ctype_char_do_toupper(this, first, last) CALL_VTBL_FUNC(this, 12, \
        const char*, (const ctype_char*, char*, const char*), (this, first, last))
#else
#define call_ctype_char_do_toupper(this, first, last) CALL_VTBL_FUNC(this, 20, \
        const char*, (const ctype_char*, char*, const char*), (this, first, last))
#endif
DEFINE_THISCALL_WRAPPER(ctype_char_do_toupper, 12)
const char* __thiscall ctype_char_do_toupper(const ctype_char *this,
        char *first, const char *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    for(; first<last; first++)
        *first = _Toupper(*first, &this->ctype);
    return last;
}

/* ?toupper@?$ctype@D@std@@QBEDD@Z */
/* ?toupper@?$ctype@D@std@@QEBADD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_toupper_ch, 8)
char __thiscall ctype_char_toupper_ch(const ctype_char *this, char ch)
{
    TRACE("(%p %c)\n", this, ch);
    return call_ctype_char_do_toupper_ch(this, ch);
}

/* ?toupper@?$ctype@D@std@@QBEPBDPADPBD@Z */
/* ?toupper@?$ctype@D@std@@QEBAPEBDPEADPEBD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_toupper, 12)
const char* __thiscall ctype_char_toupper(const ctype_char *this, char *first, const char *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    return call_ctype_char_do_toupper(this, first, last);
}

/* ?is@?$ctype@D@std@@QBE_NFD@Z */
/* ?is@?$ctype@D@std@@QEBA_NFD@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_is_ch, 12)
bool __thiscall ctype_char_is_ch(const ctype_char *this, short mask, char ch)
{
    TRACE("(%p %x %c)\n", this, mask, ch);
    return (this->ctype.table[(unsigned char)ch] & mask) != 0;
}

/* ?is@?$ctype@D@std@@QBEPBDPBD0PAF@Z */
/* ?is@?$ctype@D@std@@QEBAPEBDPEBD0PEAF@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_is, 16)
const char* __thiscall ctype_char_is(const ctype_char *this, const char *first, const char *last, short *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    for(; first<last; first++)
        *dest++ = this->ctype.table[(unsigned char)*first];
    return last;
}

/* ?scan_is@?$ctype@D@std@@QBEPBDFPBD0@Z */
/* ?scan_is@?$ctype@D@std@@QEBAPEBDFPEBD0@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_scan_is, 16)
const char* __thiscall ctype_char_scan_is(const ctype_char *this, short mask, const char *first, const char *last)
{
    TRACE("(%p %x %p %p)\n", this, mask, first, last);
    for(; first<last; first++)
        if(!ctype_char_is_ch(this, mask, *first))
            break;
    return first;
}

/* ?scan_not@?$ctype@D@std@@QBEPBDFPBD0@Z */
/* ?scan_not@?$ctype@D@std@@QEBAPEBDFPEBD0@Z */
DEFINE_THISCALL_WRAPPER(ctype_char_scan_not, 16)
const char* __thiscall ctype_char_scan_not(const ctype_char *this, short mask, const char *first, const char *last)
{
    TRACE("(%p %x %p %p)\n", this, mask, first, last);
    for(; first<last; first++)
        if(ctype_char_is_ch(this, mask, *first))
            break;
    return first;
}

/* ?table@?$ctype@D@std@@IBEPBFXZ */
/* ?table@?$ctype@D@std@@IEBAPEBFXZ */
DEFINE_THISCALL_WRAPPER(ctype_char_table, 4)
const short* __thiscall ctype_char_table(const ctype_char *this)
{
    TRACE("(%p)\n", this);
    return this->ctype.table;
}

/* ?id@?$ctype@_W@std@@2V0locale@2@A */
locale_id ctype_wchar_id = {0};
/* ?id@?$ctype@G@std@@2V0locale@2@A */
locale_id ctype_short_id = {0};

/* ??_7?$ctype@_W@std@@6B@ */
extern const vtable_ptr ctype_wchar_vtable;
/* ??_7?$ctype@G@std@@6B@ */
extern const vtable_ptr ctype_short_vtable;

/* ?_Id_func@?$ctype@_W@std@@SAAAVid@locale@2@XZ */
/* ?_Id_func@?$ctype@_W@std@@SAAEAVid@locale@2@XZ */
locale_id* __cdecl ctype_wchar__Id_func(void)
{
    TRACE("()\n");
    return &ctype_wchar_id;
}

/* ?_Id_func@?$ctype@G@std@@SAAAVid@locale@2@XZ */
/* ?_Id_func@?$ctype@G@std@@SAAEAVid@locale@2@XZ */
locale_id* __cdecl ctype_short__Id_func(void)
{
    TRACE("()\n");
    return &ctype_short_id;
}

/* ?_Init@?$ctype@_W@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$ctype@_W@std@@IEAAXAEBV_Locinfo@2@@Z */
/* ?_Init@?$ctype@G@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$ctype@G@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Init, 8)
void __thiscall ctype_wchar__Init(ctype_wchar *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Getctype(locinfo, &this->ctype);
    _Locinfo__Getcvt(locinfo, &this->cvt);
}

/* ??0?$ctype@_W@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$ctype@_W@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_ctor_locinfo, 12)
ctype_wchar* __thiscall ctype_wchar_ctor_locinfo(ctype_wchar *this,
        const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);
    ctype_base_ctor_refs(&this->base, refs);
    this->base.facet.vtable = &ctype_wchar_vtable;
    ctype_wchar__Init(this, locinfo);
    return this;
}

/* ??0?$ctype@G@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$ctype@G@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_short_ctor_locinfo, 12)
ctype_wchar* __thiscall ctype_short_ctor_locinfo(ctype_wchar *this,
        const _Locinfo *locinfo, size_t refs)
{
    ctype_wchar *ret = ctype_wchar_ctor_locinfo(this, locinfo, refs);
    this->base.facet.vtable = &ctype_short_vtable;
    return ret;
}

/* ??0?$ctype@_W@std@@QAE@I@Z */
/* ??0?$ctype@_W@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_ctor_refs, 8)
ctype_wchar* __thiscall ctype_wchar_ctor_refs(ctype_wchar *this, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %Iu)\n", this, refs);

    ctype_base_ctor_refs(&this->base, refs);
    this->base.facet.vtable = &ctype_wchar_vtable;

    _Locinfo_ctor(&locinfo);
    ctype_wchar__Init(this, &locinfo);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$ctype@G@std@@QAE@I@Z */
/* ??0?$ctype@G@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_short_ctor_refs, 8)
ctype_wchar* __thiscall ctype_short_ctor_refs(ctype_wchar *this, size_t refs)
{
    ctype_wchar *ret = ctype_wchar_ctor_refs(this, refs);
    this->base.facet.vtable = &ctype_short_vtable;
    return ret;
}

/* ??0?$ctype@G@std@@IAE@PBDI@Z */
/* ??0?$ctype@G@std@@IEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_short_ctor_name, 12)
ctype_wchar* __thiscall ctype_short_ctor_name(ctype_wchar *this,
    const char *name, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %s %Iu)\n", this, debugstr_a(name), refs);

    ctype_base_ctor_refs(&this->base, refs);
    this->base.facet.vtable = &ctype_short_vtable;

    _Locinfo_ctor_cstr(&locinfo, name);
    ctype_wchar__Init(this, &locinfo);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??_F?$ctype@_W@std@@QAEXXZ */
/* ??_F?$ctype@_W@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(ctype_wchar_ctor, 4)
ctype_wchar* __thiscall ctype_wchar_ctor(ctype_wchar *this)
{
    TRACE("(%p)\n", this);
    return ctype_short_ctor_refs(this, 0);
}

/* ??_F?$ctype@G@std@@QAEXXZ */
/* ??_F?$ctype@G@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(ctype_short_ctor, 4)
ctype_wchar* __thiscall ctype_short_ctor(ctype_wchar *this)
{
    ctype_wchar *ret = ctype_wchar_ctor(this);
    this->base.facet.vtable = &ctype_short_vtable;
    return ret;
}

/* ??1?$ctype@G@std@@UAE@XZ */
/* ??1?$ctype@G@std@@UEAA@XZ */
/* ??1?$ctype@_W@std@@MAE@XZ */
/* ??1?$ctype@_W@std@@MEAA@XZ */
/* ??1?$ctype@G@std@@MAE@XZ */
/* ??1?$ctype@G@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(ctype_wchar_dtor, 4)
void __thiscall ctype_wchar_dtor(ctype_wchar *this)
{
    TRACE("(%p)\n", this);
    if(this->ctype.delfl)
        free((void*)this->ctype.table);
#if _MSVCP_VER >= 110
    free(this->ctype.name);
#endif
}

DEFINE_THISCALL_WRAPPER(ctype_wchar_vector_dtor, 8)
ctype_wchar* __thiscall ctype_wchar_vector_dtor(ctype_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            ctype_wchar_dtor(this+i);
        operator_delete(ptr);
    } else {
        ctype_wchar_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* _Wcrtomb */
int __cdecl _Wcrtomb(char *s, wchar_t wch, _Mbstatet *state, const _Cvtvec *cvt)
{
    int cp, size;
    BOOL def;

    TRACE("%p %d %p %p\n", s, wch, state, cvt);

    if(cvt)
        cp = cvt->page;
    else
        cp = ___lc_codepage_func();

    if(!cp) {
        if(wch > 255) {
           *_errno() = EILSEQ;
           return -1;
        }

        *s = wch & 255;
        return 1;
    }

    size = WideCharToMultiByte(cp, 0, &wch, 1, s, MB_LEN_MAX, NULL, &def);
    if(!size || def) {
        *_errno() = EILSEQ;
        return -1;
    }

    return size;
}

/* ?_Donarrow@?$ctype@_W@std@@IBED_WD@Z */
/* ?_Donarrow@?$ctype@_W@std@@IEBAD_WD@Z */
/* ?_Donarrow@?$ctype@G@std@@IBEDGD@Z */
/* ?_Donarrow@?$ctype@G@std@@IEBADGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Donarrow, 12)
char __thiscall ctype_wchar__Donarrow(const ctype_wchar *this, wchar_t ch, char dflt)
{
    char buf[MB_LEN_MAX];

    TRACE("(%p %d %d)\n", this, ch, dflt);

    return _Wcrtomb(buf, ch, NULL, &this->cvt)==1 ? buf[0] : dflt;
}

/* ?do_narrow@?$ctype@_W@std@@MBED_WD@Z */
/* ?do_narrow@?$ctype@_W@std@@MEBAD_WD@Z */
/* ?do_narrow@?$ctype@G@std@@MBEDGD@Z */
/* ?do_narrow@?$ctype@G@std@@MEBADGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_narrow_ch, 12)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
#define call_ctype_wchar_do_narrow_ch(this, ch, dflt) CALL_VTBL_FUNC(this, 52, \
        char, (const ctype_wchar*, wchar_t, char), (this, ch, dflt))
#elif _MSVCP_VER <= 100
#define call_ctype_wchar_do_narrow_ch(this, ch, dflt) CALL_VTBL_FUNC(this, 48, \
        char, (const ctype_wchar*, wchar_t, char), (this, ch, dflt))
#else
#define call_ctype_wchar_do_narrow_ch(this, ch, dflt) CALL_VTBL_FUNC(this, 56, \
        char, (const ctype_wchar*, wchar_t, char), (this, ch, dflt))
#endif
char __thiscall ctype_wchar_do_narrow_ch(const ctype_wchar *this, wchar_t ch, char dflt)
{
    return ctype_wchar__Donarrow(this, ch, dflt);
}

/* ?do_narrow@?$ctype@_W@std@@MBEPB_WPB_W0DPAD@Z */
/* ?do_narrow@?$ctype@_W@std@@MEBAPEB_WPEB_W0DPEAD@Z */
/* ?do_narrow@?$ctype@G@std@@MBEPBGPBG0DPAD@Z */
/* ?do_narrow@?$ctype@G@std@@MEBAPEBGPEBG0DPEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_narrow, 20)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
#define call_ctype_wchar_do_narrow(this, first, last, dflt, dest) CALL_VTBL_FUNC(this, 48, \
        const wchar_t*, (const ctype_wchar*, const wchar_t*, const wchar_t*, char, char*), \
        (this, first, last, dflt, dest))
#elif _MSVCP_VER <= 100
#define call_ctype_wchar_do_narrow(this, first, last, dflt, dest) CALL_VTBL_FUNC(this, 44, \
        const wchar_t*, (const ctype_wchar*, const wchar_t*, const wchar_t*, char, char*), \
        (this, first, last, dflt, dest))
#else
#define call_ctype_wchar_do_narrow(this, first, last, dflt, dest) CALL_VTBL_FUNC(this, 52, \
        const wchar_t*, (const ctype_wchar*, const wchar_t*, const wchar_t*, char, char*), \
        (this, first, last, dflt, dest))
#endif
const wchar_t* __thiscall ctype_wchar_do_narrow(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, char dflt, char *dest)
{
    TRACE("(%p %p %p %d %p)\n", this, first, last, dflt, dest);
    for(; first<last; first++)
        *dest++ = ctype_wchar__Donarrow(this, *first, dflt);
    return last;
}

/* ?_Do_narrow_s@?$ctype@_W@std@@MBEPB_WPB_W0DPADI@Z */
/* ?_Do_narrow_s@?$ctype@_W@std@@MEBAPEB_WPEB_W0DPEAD_K@Z */
/* ?_Do_narrow_s@?$ctype@G@std@@MBEPBGPBG0DPADI@Z */
/* ?_Do_narrow_s@?$ctype@G@std@@MEBAPEBGPEBG0DPEAD_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Do_narrow_s, 24)
#define call_ctype_wchar__Do_narrow_s(this, first, last, dflt, dest, size) CALL_VTBL_FUNC(this, 56, \
        const wchar_t*, (const ctype_wchar*, const wchar_t*, const wchar_t*, char, char*, size_t), \
        (this, first, last, dflt, dest, size))
const wchar_t* __thiscall ctype_wchar__Do_narrow_s(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, char dflt, char *dest, size_t size)
{
    TRACE("(%p %p %p %d %p %Iu)\n", this, first, last, dflt, dest, size);
    /* This function converts all multi-byte characters to dflt,
     * thanks to it result size is known before converting */
    if(last-first > size)
        ctype_base__Xran();
    return ctype_wchar_do_narrow(this, first, last, dflt, dest);
}

/* ?narrow@?$ctype@_W@std@@QBED_WD@Z */
/* ?narrow@?$ctype@_W@std@@QEBAD_WD@Z */
/* ?narrow@?$ctype@G@std@@QBEDGD@Z */
/* ?narrow@?$ctype@G@std@@QEBADGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_narrow_ch, 12)
char __thiscall ctype_wchar_narrow_ch(const ctype_wchar *this, wchar_t ch, char dflt)
{
    TRACE("(%p %d %d)\n", this, ch, dflt);
    return call_ctype_wchar_do_narrow_ch(this, ch, dflt);
}

/* ?narrow@?$ctype@_W@std@@QBEPB_WPB_W0DPAD@Z */
/* ?narrow@?$ctype@_W@std@@QEBAPEB_WPEB_W0DPEAD@Z */
/* ?narrow@?$ctype@G@std@@QBEPBGPBG0DPAD@Z */
/* ?narrow@?$ctype@G@std@@QEBAPEBGPEBG0DPEAD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_narrow, 20)
const wchar_t* __thiscall ctype_wchar_narrow(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, char dflt, char *dest)
{
    TRACE("(%p %p %p %d %p)\n", this, first, last, dflt, dest);
    return call_ctype_wchar_do_narrow(this, first, last, dflt, dest);
}

/* ?_Narrow_s@?$ctype@_W@std@@QBEPB_WPB_W0DPADI@Z */
/* ?_Narrow_s@?$ctype@_W@std@@QEBAPEB_WPEB_W0DPEAD_K@Z */
/* ?_Narrow_s@?$ctype@G@std@@QBEPBGPBG0DPADI@Z */
/* ?_Narrow_s@?$ctype@G@std@@QEBAPEBGPEBG0DPEAD_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Narrow_s, 24)
const wchar_t* __thiscall ctype_wchar__Narrow_s(const ctype_wchar *this, const wchar_t *first,
        const wchar_t *last, char dflt, char *dest, size_t size)
{
    TRACE("(%p %p %p %d %p %Iu)\n", this, first, last, dflt, dest, size);
    return call_ctype_wchar__Do_narrow_s(this, first, last, dflt, dest, size);
}

/* _Mbrtowc */
int __cdecl _Mbrtowc(wchar_t *out, const char *in, size_t len, _Mbstatet *state, const _Cvtvec *cvt)
{
    int i, cp;
    CPINFO cp_info;
    BOOL is_lead;

    TRACE("(%p %p %Iu %p %p)\n", out, in, len, state, cvt);

    if(!len)
        return 0;

    if(cvt)
        cp = cvt->page;
    else
        cp = ___lc_codepage_func();

    if(!cp) {
        if(out)
            *out = (unsigned char)*in;

        memset(state, 0, sizeof(*state));
        return *in ? 1 : 0;
    }

    if(MBSTATET_TO_INT(state)) {
        ((char*)&MBSTATET_TO_INT(state))[1] = *in;

        if(!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, (char*)&MBSTATET_TO_INT(state), 2, out, out ? 1 : 0)) {
            memset(state, 0, sizeof(*state));
            *_errno() = EILSEQ;
            return -1;
        }

        memset(state, 0, sizeof(*state));
        return 2;
    }

    GetCPInfo(cp, &cp_info);
    is_lead = FALSE;
    for(i=0; i<MAX_LEADBYTES; i+=2) {
        if(!cp_info.LeadByte[i+1])
            break;
        if((unsigned char)*in>=cp_info.LeadByte[i] && (unsigned char)*in<=cp_info.LeadByte[i+1]) {
            is_lead = TRUE;
            break;
        }
    }

    if(is_lead) {
        if(len == 1) {
            MBSTATET_TO_INT(state) = (unsigned char)*in;
            return -2;
        }

        if(!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, in, 2, out, out ? 1 : 0)) {
            *_errno() = EILSEQ;
            return -1;
        }
        return 2;
    }

    if(!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, in, 1, out, out ? 1 : 0)) {
        *_errno() = EILSEQ;
        return -1;
    }
    return 1;
}

static inline wchar_t mb_to_wc(char ch, const _Cvtvec *cvt)
{
    _Mbstatet state;
    wchar_t ret;

    memset(&state, 0, sizeof(state));
    return _Mbrtowc(&ret, &ch, 1, &state, cvt) == 1 ? ret : 0;
}

/* ?_Dowiden@?$ctype@_W@std@@IBE_WD@Z */
/* ?_Dowiden@?$ctype@_W@std@@IEBA_WD@Z */
/* ?_Dowiden@?$ctype@G@std@@IBEGD@Z */
/* ?_Dowiden@?$ctype@G@std@@IEBAGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Dowiden, 8)
wchar_t __thiscall ctype_wchar__Dowiden(const ctype_wchar *this, char ch)
{
    _Mbstatet state;
    wchar_t ret;

    TRACE("(%p %d)\n", this, ch);

    memset(&state, 0, sizeof(state));
    return _Mbrtowc(&ret, &ch, 1, &state, &this->cvt)<0 ? WEOF : ret;
}

/* ?do_widen@?$ctype@_W@std@@MBE_WD@Z */
/* ?do_widen@?$ctype@_W@std@@MEBA_WD@Z */
/* ?do_widen@?$ctype@G@std@@MBEGD@Z */
/* ?do_widen@?$ctype@G@std@@MEBAGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_widen_ch, 8)
#if _MSVCP_VER <= 100
#define call_ctype_wchar_do_widen_ch(this, ch) CALL_VTBL_FUNC(this, 40, \
        wchar_t, (const ctype_wchar*, char), (this, ch))
#else
#define call_ctype_wchar_do_widen_ch(this, ch) CALL_VTBL_FUNC(this, 48, \
        wchar_t, (const ctype_wchar*, char), (this, ch))
#endif
wchar_t __thiscall ctype_wchar_do_widen_ch(const ctype_wchar *this, char ch)
{
    return ctype_wchar__Dowiden(this, ch);
}

/* ?do_widen@?$ctype@_W@std@@MBEPBDPBD0PA_W@Z */
/* ?do_widen@?$ctype@_W@std@@MEBAPEBDPEBD0PEA_W@Z */
/* ?do_widen@?$ctype@G@std@@MBEPBDPBD0PAG@Z */
/* ?do_widen@?$ctype@G@std@@MEBAPEBDPEBD0PEAG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_widen, 16)
#if _MSVCP_VER <= 100
#define call_ctype_wchar_do_widen(this, first, last, dest) CALL_VTBL_FUNC(this, 36, \
        const char*, (const ctype_wchar*, const char*, const char*, wchar_t*), \
        (this, first, last, dest))
#else
#define call_ctype_wchar_do_widen(this, first, last, dest) CALL_VTBL_FUNC(this, 44, \
        const char*, (const ctype_wchar*, const char*, const char*, wchar_t*), \
        (this, first, last, dest))
#endif
const char* __thiscall ctype_wchar_do_widen(const ctype_wchar *this,
        const char *first, const char *last, wchar_t *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    for(; first<last; first++)
        *dest++ = ctype_wchar__Dowiden(this, *first);
    return last;
}

/* ?_Do_widen_s@?$ctype@_W@std@@MBEPBDPBD0PA_WI@Z */
/* ?_Do_widen_s@?$ctype@_W@std@@MEBAPEBDPEBD0PEA_W_K@Z */
/* ?_Do_widen_s@?$ctype@G@std@@MBEPBDPBD0PAGI@Z */
/* ?_Do_widen_s@?$ctype@G@std@@MEBAPEBDPEBD0PEAG_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Do_widen_s, 20)
#define call_ctype_wchar__Do_widen_s(this, first, last, dest, size) CALL_VTBL_FUNC(this, 44, \
        const char*, (const ctype_wchar*, const char*, const char*, wchar_t*, size_t), \
        (this, first, last, dest, size))
const char* __thiscall ctype_wchar__Do_widen_s(const ctype_wchar *this,
        const char *first, const char *last, wchar_t *dest, size_t size)
{
    TRACE("(%p %p %p %p %Iu)\n", this, first, last, dest, size);
    /* This function converts all multi-byte characters to WEOF,
     * thanks to it result size is known before converting */
    if(size < last-first)
        ctype_base__Xran();
    return ctype_wchar_do_widen(this, first, last, dest);
}

/* ?widen@?$ctype@_W@std@@QBE_WD@Z */
/* ?widen@?$ctype@_W@std@@QEBA_WD@Z */
/* ?widen@?$ctype@G@std@@QBEGD@Z */
/* ?widen@?$ctype@G@std@@QEBAGD@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_widen_ch, 8)
wchar_t __thiscall ctype_wchar_widen_ch(const ctype_wchar *this, char ch)
{
    TRACE("(%p %d)\n", this, ch);
    return call_ctype_wchar_do_widen_ch(this, ch);
}

/* ?widen@?$ctype@_W@std@@QBEPBDPBD0PA_W@Z */
/* ?widen@?$ctype@_W@std@@QEBAPEBDPEBD0PEA_W@Z */
/* ?widen@?$ctype@G@std@@QBEPBDPBD0PAG@Z */
/* ?widen@?$ctype@G@std@@QEBAPEBDPEBD0PEAG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_widen, 16)
const char* __thiscall ctype_wchar_widen(const ctype_wchar *this,
        const char *first, const char *last, wchar_t *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    return call_ctype_wchar_do_widen(this, first, last, dest);
}

/* ?_Widen_s@?$ctype@_W@std@@QBEPBDPBD0PA_WI@Z */
/* ?_Widen_s@?$ctype@_W@std@@QEBAPEBDPEBD0PEA_W_K@Z */
/* ?_Widen_s@?$ctype@G@std@@QBEPBDPBD0PAGI@Z */
/* ?_Widen_s@?$ctype@G@std@@QEBAPEBDPEBD0PEAG_K@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar__Widen_s, 20)
const char* __thiscall ctype_wchar__Widen_s(const ctype_wchar *this,
        const char *first, const char *last, wchar_t *dest, size_t size)
{
    TRACE("(%p %p %p %p %Iu)\n", this, first, last, dest, size);
    return call_ctype_wchar__Do_widen_s(this, first, last, dest, size);
}

/* ?_Getcat@?$ctype@_W@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$ctype@_W@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl ctype_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = operator_new(sizeof(ctype_wchar));
        _Locinfo_ctor_cstr(&locinfo, locale_string_char_c_str(&loc->ptr->name));
        ctype_wchar_ctor_locinfo((ctype_wchar*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_CTYPE;
}

/* ?_Getcat@?$ctype@_W@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$ctype@_W@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl ctype_wchar__Getcat_old(const locale_facet **facet)
{
    return ctype_wchar__Getcat(facet, locale_classic());
}

/* ?_Getcat@?$ctype@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$ctype@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl ctype_short__Getcat(const locale_facet **facet, const locale *loc)
{
    if(facet && !*facet) {
        ctype_wchar__Getcat(facet, loc);
        (*(locale_facet**)facet)->vtable = &ctype_short_vtable;
    }

    return LC_CTYPE;
}

/* ?_Getcat@?$ctype@G@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$ctype@G@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl ctype_short__Getcat_old(const locale_facet **facet)
{
    return ctype_short__Getcat(facet, locale_classic());
}

/* _Towlower */
wchar_t __cdecl _Towlower(wchar_t ch, const _Ctypevec *ctype)
{
    TRACE("(%d %p)\n", ch, ctype);
    return towlower(ch);
}

ctype_wchar* ctype_wchar_use_facet(const locale *loc)
{
    static ctype_wchar *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&ctype_wchar_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (ctype_wchar*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    ctype_wchar__Getcat(&fac, loc);
    obj = (ctype_wchar*)fac;
    call_locale_facet__Incref(&obj->base.facet);
    locale_facet_register(&obj->base.facet);
    _Lockit_dtor(&lock);

    return obj;
}

ctype_wchar* ctype_short_use_facet(const locale *loc)
{
    static ctype_wchar *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&ctype_short_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (ctype_wchar*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    ctype_short__Getcat(&fac, loc);
    obj = (ctype_wchar*)fac;
    call_locale_facet__Incref(&obj->base.facet);
    locale_facet_register(&obj->base.facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?do_tolower@?$ctype@_W@std@@MBE_W_W@Z */
/* ?do_tolower@?$ctype@_W@std@@MEBA_W_W@Z */
/* ?do_tolower@?$ctype@G@std@@MBEGG@Z */
/* ?do_tolower@?$ctype@G@std@@MEBAGG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_tolower_ch, 8)
#if _MSVCP_VER <= 100
#define call_ctype_wchar_do_tolower_ch(this, ch) CALL_VTBL_FUNC(this, 24, \
        wchar_t, (const ctype_wchar*, wchar_t), (this, ch))
#else
#define call_ctype_wchar_do_tolower_ch(this, ch) CALL_VTBL_FUNC(this, 32, \
        wchar_t, (const ctype_wchar*, wchar_t), (this, ch))
#endif
wchar_t __thiscall ctype_wchar_do_tolower_ch(const ctype_wchar *this, wchar_t ch)
{
    return _Towlower(ch, &this->ctype);
}

/* ?do_tolower@?$ctype@_W@std@@MBEPB_WPA_WPB_W@Z */
/* ?do_tolower@?$ctype@_W@std@@MEBAPEB_WPEA_WPEB_W@Z */
/* ?do_tolower@?$ctype@G@std@@MBEPBGPAGPBG@Z */
/* ?do_tolower@?$ctype@G@std@@MEBAPEBGPEAGPEBG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_tolower, 12)
#if _MSVCP_VER <= 100
#define call_ctype_wchar_do_tolower(this, first, last) CALL_VTBL_FUNC(this, 20, \
        const wchar_t*, (const ctype_wchar*, wchar_t*, const wchar_t*), \
        (this, first, last))
#else
#define call_ctype_wchar_do_tolower(this, first, last) CALL_VTBL_FUNC(this, 28, \
        const wchar_t*, (const ctype_wchar*, wchar_t*, const wchar_t*), \
        (this, first, last))
#endif
const wchar_t* __thiscall ctype_wchar_do_tolower(const ctype_wchar *this,
        wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    for(; first<last; first++)
        *first = _Towlower(*first, &this->ctype);
    return last;
}

/* ?tolower@?$ctype@_W@std@@QBE_W_W@Z */
/* ?tolower@?$ctype@_W@std@@QEBA_W_W@Z */
/* ?tolower@?$ctype@G@std@@QBEGG@Z */
/* ?tolower@?$ctype@G@std@@QEBAGG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_tolower_ch, 8)
wchar_t __thiscall ctype_wchar_tolower_ch(const ctype_wchar *this, wchar_t ch)
{
    TRACE("(%p %d)\n", this, ch);
    return call_ctype_wchar_do_tolower_ch(this, ch);
}

/* ?tolower@?$ctype@_W@std@@QBEPB_WPA_WPB_W@Z */
/* ?tolower@?$ctype@_W@std@@QEBAPEB_WPEA_WPEB_W@Z */
/* ?tolower@?$ctype@G@std@@QBEPBGPAGPBG@Z */
/* ?tolower@?$ctype@G@std@@QEBAPEBGPEAGPEBG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_tolower, 12)
const wchar_t* __thiscall ctype_wchar_tolower(const ctype_wchar *this,
        wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    return call_ctype_wchar_do_tolower(this, first, last);
}

/* _Towupper */
wchar_t __cdecl _Towupper(wchar_t ch, const _Ctypevec *ctype)
{
    TRACE("(%d %p)\n", ch, ctype);
    return towupper(ch);
}

/* ?do_toupper@?$ctype@_W@std@@MBE_W_W@Z */
/* ?do_toupper@?$ctype@_W@std@@MEBA_W_W@Z */
/* ?do_toupper@?$ctype@G@std@@MBEGG@Z */
/* ?do_toupper@?$ctype@G@std@@MEBAGG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_toupper_ch, 8)
#if _MSVCP_VER <= 100
#define call_ctype_wchar_do_toupper_ch(this, ch) CALL_VTBL_FUNC(this, 32, \
        wchar_t, (const ctype_wchar*, wchar_t), (this, ch))
#else
#define call_ctype_wchar_do_toupper_ch(this, ch) CALL_VTBL_FUNC(this, 40, \
        wchar_t, (const ctype_wchar*, wchar_t), (this, ch))
#endif
wchar_t __thiscall ctype_wchar_do_toupper_ch(const ctype_wchar *this, wchar_t ch)
{
    return _Towupper(ch, &this->ctype);
}

/* ?do_toupper@?$ctype@_W@std@@MBEPB_WPA_WPB_W@Z */
/* ?do_toupper@?$ctype@_W@std@@MEBAPEB_WPEA_WPEB_W@Z */
/* ?do_toupper@?$ctype@G@std@@MBEPBGPAGPBG@Z */
/* ?do_toupper@?$ctype@G@std@@MEBAPEBGPEAGPEBG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_toupper, 12)
#if _MSVCP_VER <= 100
#define call_ctype_wchar_do_toupper(this, first, last) CALL_VTBL_FUNC(this, 28, \
        const wchar_t*, (const ctype_wchar*, wchar_t*, const wchar_t*), \
        (this, first, last))
#else
#define call_ctype_wchar_do_toupper(this, first, last) CALL_VTBL_FUNC(this, 36, \
        const wchar_t*, (const ctype_wchar*, wchar_t*, const wchar_t*), \
        (this, first, last))
#endif
const wchar_t* __thiscall ctype_wchar_do_toupper(const ctype_wchar *this,
        wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    for(; first<last; first++)
        *first = _Towupper(*first, &this->ctype);
    return last;
}

/* ?toupper@?$ctype@_W@std@@QBE_W_W@Z */
/* ?toupper@?$ctype@_W@std@@QEBA_W_W@Z */
/* ?toupper@?$ctype@G@std@@QBEGG@Z */
/* ?toupper@?$ctype@G@std@@QEBAGG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_toupper_ch, 8)
wchar_t __thiscall ctype_wchar_toupper_ch(const ctype_wchar *this, wchar_t ch)
{
    TRACE("(%p %d)\n", this, ch);
    return call_ctype_wchar_do_toupper_ch(this, ch);
}

/* ?toupper@?$ctype@_W@std@@QBEPB_WPA_WPB_W@Z */
/* ?toupper@?$ctype@_W@std@@QEBAPEB_WPEA_WPEB_W@Z */
/* ?toupper@?$ctype@G@std@@QBEPBGPAGPBG@Z */
/* ?toupper@?$ctype@G@std@@QEBAPEBGPEAGPEBG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_toupper, 12)
const wchar_t* __thiscall ctype_wchar_toupper(const ctype_wchar *this,
        wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %p %p)\n", this, first, last);
    return call_ctype_wchar_do_toupper(this, first, last);
}

/* _Getwctypes */
const wchar_t* __cdecl _Getwctypes(const wchar_t *first, const wchar_t *last,
        short *mask, const _Ctypevec *ctype)
{
    TRACE("(%p %p %p %p)\n", first, last, mask, ctype);
    GetStringTypeW(CT_CTYPE1, first, last-first, (WORD*)mask);
    return last;
}

/* _Getwctype */
short __cdecl _Getwctype(wchar_t ch, const _Ctypevec *ctype)
{
    short mask = 0;
    _Getwctypes(&ch, &ch+1, &mask, ctype);
    return mask;
}

/* ?do_is@?$ctype@_W@std@@MBE_NF_W@Z */
/* ?do_is@?$ctype@_W@std@@MEBA_NF_W@Z */
/* ?do_is@?$ctype@G@std@@MBE_NFG@Z */
/* ?do_is@?$ctype@G@std@@MEBA_NFG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_is_ch, 12)
#if _MSVCP_VER <= 100
#define call_ctype_wchar_do_is_ch(this, mask, ch) CALL_VTBL_FUNC(this, 8, \
        bool, (const ctype_wchar*, short, wchar_t), (this, mask, ch))
#else
#define call_ctype_wchar_do_is_ch(this, mask, ch) CALL_VTBL_FUNC(this, 16, \
        bool, (const ctype_wchar*, short, wchar_t), (this, mask, ch))
#endif
bool __thiscall ctype_wchar_do_is_ch(const ctype_wchar *this, short mask, wchar_t ch)
{
    TRACE("(%p %x %d)\n", this, mask, ch);
    return (_Getwctype(ch, &this->ctype) & mask) != 0;
}

/* ?do_is@?$ctype@_W@std@@MBEPB_WPB_W0PAF@Z */
/* ?do_is@?$ctype@_W@std@@MEBAPEB_WPEB_W0PEAF@Z */
/* ?do_is@?$ctype@G@std@@MBEPBGPBG0PAF@Z */
/* ?do_is@?$ctype@G@std@@MEBAPEBGPEBG0PEAF@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_is, 16)
#if _MSVCP_VER <= 100
#define call_ctype_wchar_do_is(this, first, last, dest) CALL_VTBL_FUNC(this, 4, \
        const wchar_t*, (const ctype_wchar*, const wchar_t*, const wchar_t*, short*), \
        (this, first, last, dest))
#else
#define call_ctype_wchar_do_is(this, first, last, dest) CALL_VTBL_FUNC(this, 12, \
        const wchar_t*, (const ctype_wchar*, const wchar_t*, const wchar_t*, short*), \
        (this, first, last, dest))
#endif
const wchar_t* __thiscall ctype_wchar_do_is(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, short *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    return _Getwctypes(first, last, dest, &this->ctype);
}

/* ?is@?$ctype@_W@std@@QBE_NF_W@Z */
/* ?is@?$ctype@_W@std@@QEBA_NF_W@Z */
/* ?is@?$ctype@G@std@@QBE_NFG@Z */
/* ?is@?$ctype@G@std@@QEBA_NFG@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_is_ch, 12)
bool __thiscall ctype_wchar_is_ch(const ctype_wchar *this, short mask, wchar_t ch)
{
    TRACE("(%p %x %d)\n", this, mask, ch);
    return call_ctype_wchar_do_is_ch(this, mask, ch);
}

/* ?is@?$ctype@_W@std@@QBEPB_WPB_W0PAF@Z */
/* ?is@?$ctype@_W@std@@QEBAPEB_WPEB_W0PEAF@Z */
/* ?is@?$ctype@G@std@@QBEPBGPBG0PAF@Z */
/* ?is@?$ctype@G@std@@QEBAPEBGPEBG0PEAF@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_is, 16)
const wchar_t* __thiscall ctype_wchar_is(const ctype_wchar *this,
        const wchar_t *first, const wchar_t *last, short *dest)
{
    TRACE("(%p %p %p %p)\n", this, first, last, dest);
    return call_ctype_wchar_do_is(this, first, last, dest);
}

/* ?do_scan_is@?$ctype@_W@std@@MBEPB_WFPB_W0@Z */
/* ?do_scan_is@?$ctype@_W@std@@MEBAPEB_WFPEB_W0@Z */
/* ?do_scan_is@?$ctype@G@std@@MBEPBGFPBG0@Z */
/* ?do_scan_is@?$ctype@G@std@@MEBAPEBGFPEBG0@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_scan_is, 16)
#if _MSVCP_VER <= 100
#define call_ctype_wchar_do_scan_is(this, mask, first, last) CALL_VTBL_FUNC(this, 12, \
        const wchar_t*, (const ctype_wchar*, short, const wchar_t*, const wchar_t*), \
        (this, mask, first, last))
#else
#define call_ctype_wchar_do_scan_is(this, mask, first, last) CALL_VTBL_FUNC(this, 20, \
        const wchar_t*, (const ctype_wchar*, short, const wchar_t*, const wchar_t*), \
        (this, mask, first, last))
#endif
const wchar_t* __thiscall ctype_wchar_do_scan_is(const ctype_wchar *this,
        short mask, const wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %d %p %p)\n", this, mask, first, last);
    for(; first<last; first++)
        if(!ctype_wchar_is_ch(this, mask, *first))
            break;
    return first;
}

/* ?scan_is@?$ctype@_W@std@@QBEPB_WFPB_W0@Z */
/* ?scan_is@?$ctype@_W@std@@QEBAPEB_WFPEB_W0@Z */
/* ?scan_is@?$ctype@G@std@@QBEPBGFPBG0@Z */
/* ?scan_is@?$ctype@G@std@@QEBAPEBGFPEBG0@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_scan_is, 16)
const wchar_t* __thiscall ctype_wchar_scan_is(const ctype_wchar *this,
        short mask, const wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %x %p %p)\n", this, mask, first, last);
    return call_ctype_wchar_do_scan_is(this, mask, first, last);
}

/* ?do_scan_not@?$ctype@_W@std@@MBEPB_WFPB_W0@Z */
/* ?do_scan_not@?$ctype@_W@std@@MEBAPEB_WFPEB_W0@Z */
/* ?do_scan_not@?$ctype@G@std@@MBEPBGFPBG0@Z */
/* ?do_scan_not@?$ctype@G@std@@MEBAPEBGFPEBG0@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_do_scan_not, 16)
#if _MSVCP_VER <= 100
#define call_ctype_wchar_do_scan_not(this, mask, first, last) CALL_VTBL_FUNC(this, 16, \
        const wchar_t*, (const ctype_wchar*, short, const wchar_t*, const wchar_t*), \
        (this, mask, first, last))
#else
#define call_ctype_wchar_do_scan_not(this, mask, first, last) CALL_VTBL_FUNC(this, 24, \
        const wchar_t*, (const ctype_wchar*, short, const wchar_t*, const wchar_t*), \
        (this, mask, first, last))
#endif
const wchar_t* __thiscall ctype_wchar_do_scan_not(const ctype_wchar *this,
        short mask, const wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %x %p %p)\n", this, mask, first, last);
    for(; first<last; first++)
        if(ctype_wchar_is_ch(this, mask, *first))
            break;
    return first;
}

/* ?scan_not@?$ctype@_W@std@@QBEPB_WFPB_W0@Z */
/* ?scan_not@?$ctype@_W@std@@QEBAPEB_WFPEB_W0@Z */
/* ?scan_not@?$ctype@G@std@@QBEPBGFPBG0@Z */
/* ?scan_not@?$ctype@G@std@@QEBAPEBGFPEBG0@Z */
DEFINE_THISCALL_WRAPPER(ctype_wchar_scan_not, 16)
const wchar_t* __thiscall ctype_wchar_scan_not(const ctype_wchar *this,
        short mask, const wchar_t *first, const wchar_t *last)
{
    TRACE("(%p %x %p %p)\n", this, mask, first, last);
    return call_ctype_wchar_do_scan_not(this, mask, first, last);
}

/* ??_7codecvt_base@std@@6B@ */
extern const vtable_ptr codecvt_base_vtable;

/* ??0codecvt_base@std@@QAE@I@Z */
/* ??0codecvt_base@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_base_ctor_refs, 8)
codecvt_base* __thiscall codecvt_base_ctor_refs(codecvt_base *this, size_t refs)
{
    TRACE("(%p %Iu)\n", this, refs);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &codecvt_base_vtable;
    return this;
}

/* ??_Fcodecvt_base@std@@QAEXXZ */
/* ??_Fcodecvt_base@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(codecvt_base_ctor, 4)
codecvt_base* __thiscall codecvt_base_ctor(codecvt_base *this)
{
    return codecvt_base_ctor_refs(this, 0);
}

/* ??1codecvt_base@std@@UAE@XZ */
/* ??1codecvt_base@std@@UEAA@XZ */
DEFINE_THISCALL_WRAPPER(codecvt_base_dtor, 4)
void __thiscall codecvt_base_dtor(codecvt_base *this)
{
    TRACE("(%p)\n", this);
    locale_facet_dtor(&this->facet);
}

DEFINE_THISCALL_WRAPPER(codecvt_base_vector_dtor, 8)
codecvt_base* __thiscall codecvt_base_vector_dtor(codecvt_base *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            codecvt_base_dtor(this+i);
        operator_delete(ptr);
    } else {
        codecvt_base_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?do_always_noconv@codecvt_base@std@@MBE_NXZ */
/* ?do_always_noconv@codecvt_base@std@@MEBA_NXZ */
#if _MSVCP_VER <= 100
#define call_codecvt_base_do_always_noconv(this) CALL_VTBL_FUNC(this, 4, \
        bool, (const codecvt_base*), (this))
#else
#define call_codecvt_base_do_always_noconv(this) CALL_VTBL_FUNC(this, 12, \
        bool, (const codecvt_base*), (this))
#endif
DEFINE_THISCALL_WRAPPER(codecvt_base_do_always_noconv, 4)
bool __thiscall codecvt_base_do_always_noconv(const codecvt_base *this)
{
    TRACE("(%p)\n", this);
    return _MSVCP_VER <= 100;
}

/* ?always_noconv@codecvt_base@std@@QBE_NXZ */
/* ?always_noconv@codecvt_base@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(codecvt_base_always_noconv, 4)
bool __thiscall codecvt_base_always_noconv(const codecvt_base *this)
{
    TRACE("(%p)\n", this);
    return call_codecvt_base_do_always_noconv(this);
}

/* ?do_max_length@codecvt_base@std@@MBEHXZ */
/* ?do_max_length@codecvt_base@std@@MEBAHXZ */
#if _MSVCP_VER <= 100
#define call_codecvt_base_do_max_length(this) CALL_VTBL_FUNC(this, 8, \
        int, (const codecvt_base*), (this))
#else
#define call_codecvt_base_do_max_length(this) CALL_VTBL_FUNC(this, 16, \
        int, (const codecvt_base*), (this))
#endif
DEFINE_THISCALL_WRAPPER(codecvt_base_do_max_length, 4)
int __thiscall codecvt_base_do_max_length(const codecvt_base *this)
{
    TRACE("(%p)\n", this);
    return 1;
}

/* ?max_length@codecvt_base@std@@QBEHXZ */
/* ?max_length@codecvt_base@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(codecvt_base_max_length, 4)
int __thiscall codecvt_base_max_length(const codecvt_base *this)
{
    TRACE("(%p)\n", this);
    return call_codecvt_base_do_max_length(this);
}

/* ?do_encoding@codecvt_base@std@@MBEHXZ */
/* ?do_encoding@codecvt_base@std@@MEBAHXZ */
#if _MSVCP_VER <= 100
#define call_codecvt_base_do_encoding(this) CALL_VTBL_FUNC(this, 12, \
        int, (const codecvt_base*), (this))
#else
#define call_codecvt_base_do_encoding(this) CALL_VTBL_FUNC(this, 20, \
        int, (const codecvt_base*), (this))
#endif
DEFINE_THISCALL_WRAPPER(codecvt_base_do_encoding, 4)
int __thiscall codecvt_base_do_encoding(const codecvt_base *this)
{
    TRACE("(%p)\n", this);
    return 1;
}

/* ?encoding@codecvt_base@std@@QBEHXZ */
/* ?encoding@codecvt_base@std@@QEBAHXZ */
DEFINE_THISCALL_WRAPPER(codecvt_base_encoding, 4)
int __thiscall codecvt_base_encoding(const codecvt_base *this)
{
    TRACE("(%p)\n", this);
    return call_codecvt_base_do_encoding(this);
}

/* ?id@?$codecvt@DDH@std@@2V0locale@2@A */
locale_id codecvt_char_id = {0};

/* ??_7?$codecvt@DDH@std@@6B@ */
extern const vtable_ptr codecvt_char_vtable;

/* ?_Init@?$codecvt@DDH@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$codecvt@DDH@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char__Init, 8)
void __thiscall codecvt_char__Init(codecvt_char *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
}

/* ??0?$codecvt@DDH@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$codecvt@DDH@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char_ctor_locinfo, 12)
codecvt_char* __thiscall codecvt_char_ctor_locinfo(codecvt_char *this, const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);
    codecvt_base_ctor_refs(&this->base, refs);
    this->base.facet.vtable = &codecvt_char_vtable;
    return this;
}

/* ??0?$codecvt@DDH@std@@QAE@I@Z */
/* ??0?$codecvt@DDH@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char_ctor_refs, 8)
codecvt_char* __thiscall codecvt_char_ctor_refs(codecvt_char *this, size_t refs)
{
    return codecvt_char_ctor_locinfo(this, NULL, refs);
}

/* ??_F?$codecvt@DDH@std@@QAEXXZ */
/* ??_F?$codecvt@DDH@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(codecvt_char_ctor, 4)
codecvt_char* __thiscall codecvt_char_ctor(codecvt_char *this)
{
    return codecvt_char_ctor_locinfo(this, NULL, 0);
}

/* ??1?$codecvt@DDH@std@@UAE@XZ */
/* ??1?$codecvt@DDH@std@@UEAA@XZ */
/* ??1?$codecvt@DDH@std@@MAE@XZ */
/* ??1?$codecvt@DDH@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(codecvt_char_dtor, 4)
void __thiscall codecvt_char_dtor(codecvt_char *this)
{
    TRACE("(%p)\n", this);
    codecvt_base_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(codecvt_char_vector_dtor, 8)
codecvt_char* __thiscall codecvt_char_vector_dtor(codecvt_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            codecvt_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        codecvt_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$codecvt@DDH@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$codecvt@DDH@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl codecvt_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = operator_new(sizeof(codecvt_char));
        codecvt_char_ctor((codecvt_char*)*facet);
    }

    return LC_CTYPE;
}

/* ?_Getcat@?$codecvt@DDH@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$codecvt@DDH@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl codecvt_char__Getcat_old(const locale_facet **facet)
{
    return codecvt_char__Getcat(facet, locale_classic());
}

codecvt_char* codecvt_char_use_facet(const locale *loc)
{
    static codecvt_char *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&codecvt_char_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (codecvt_char*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    codecvt_char__Getcat(&fac, loc);
    obj = (codecvt_char*)fac;
    call_locale_facet__Incref(&obj->base.facet);
    locale_facet_register(&obj->base.facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?do_always_noconv@?$codecvt@DDH@std@@MBE_NXZ */
/* ?do_always_noconv@?$codecvt@DDH@std@@MEBA_NXZ */
DEFINE_THISCALL_WRAPPER(codecvt_char_do_always_noconv, 4)
bool __thiscall codecvt_char_do_always_noconv(const codecvt_char *this)
{
    TRACE("(%p)\n", this);
    return TRUE;
}

/* ?do_in@?$codecvt@DDH@std@@MBEHAAHPBD1AAPBDPAD3AAPAD@Z */
/* ?do_in@?$codecvt@DDH@std@@MEBAHAEAHPEBD1AEAPEBDPEAD3AEAPEAD@Z */
#if _MSVCP_VER <= 100
#define call_codecvt_char_do_in(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 16, int, \
            (const codecvt_char*, _Mbstatet*, const char*, const char*, const char**, char*, char*, char**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
#else
#define call_codecvt_char_do_in(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 24, int, \
            (const codecvt_char*, _Mbstatet*, const char*, const char*, const char**, char*, char*, char**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
#endif
DEFINE_THISCALL_WRAPPER(codecvt_char_do_in, 32)
int __thiscall codecvt_char_do_in(const codecvt_char *this, _Mbstatet *state,
        const char *from, const char *from_end, const char **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from, from_end,
            from_next, to, to_end, to_next);
    *from_next = from;
    *to_next = to;
    return CODECVT_noconv;
}

/* ?in@?$codecvt@DDH@std@@QBEHAAHPBD1AAPBDPAD3AAPAD@Z */
/* ?in@?$codecvt@DDH@std@@QEBAHAEAHPEBD1AEAPEBDPEAD3AEAPEAD@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char_in, 32)
int __thiscall codecvt_char_in(const codecvt_char *this, _Mbstatet *state,
        const char *from, const char *from_end, const char **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from, from_end,
            from_next, to, to_end, to_next);
    return call_codecvt_char_do_in(this, state, from, from_end, from_next,
            to, to_end, to_next);
}

/* ?do_out@?$codecvt@DDH@std@@MBEHAAHPBD1AAPBDPAD3AAPAD@Z */
/* ?do_out@?$codecvt@DDH@std@@MEBAHAEAHPEBD1AEAPEBDPEAD3AEAPEAD@Z */
#if _MSVCP_VER <= 100
#define call_codecvt_char_do_out(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 20, int, \
            (const codecvt_char*, _Mbstatet*, const char*, const char*, const char**, char*, char*, char**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
#else
#define call_codecvt_char_do_out(this, state, from, from_end, from_next, to, to_end, to_next) \
        CALL_VTBL_FUNC(this, 28, int, \
                (const codecvt_char*, _Mbstatet*, const char*, const char*, const char**, char*, char*, char**), \
                (this, state, from, from_end, from_next, to, to_end, to_next))
#endif
DEFINE_THISCALL_WRAPPER(codecvt_char_do_out, 32)
int __thiscall codecvt_char_do_out(const codecvt_char *this, _Mbstatet *state,
        const char *from, const char *from_end, const char **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from,
            from_end, from_next, to, to_end, to_next);
    *from_next = from;
    *to_next = to;
    return CODECVT_noconv;
}

/* ?out@?$codecvt@DDH@std@@QBEHAAHPBD1AAPBDPAD3AAPAD@Z */
/* ?out@?$codecvt@DDH@std@@QEBAHAEAHPEBD1AEAPEBDPEAD3AEAPEAD@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char_out, 32)
int __thiscall codecvt_char_out(const codecvt_char *this, _Mbstatet *state,
        const char *from, const char *from_end, const char **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from, from_end,
            from_next, to, to_end, to_next);
    return call_codecvt_char_do_out(this, state, from, from_end, from_next,
            to, to_end, to_next);
}

/* ?do_unshift@?$codecvt@DDH@std@@MBEHAAHPAD1AAPAD@Z */
/* ?do_unshift@?$codecvt@DDH@std@@MEBAHAEAHPEAD1AEAPEAD@Z */
#if _MSVCP_VER <= 100
#define call_codecvt_char_do_unshift(this, state, to, to_end, to_next) CALL_VTBL_FUNC(this, 24, \
        int, (const codecvt_char*, _Mbstatet*, char*, char*, char**), (this, state, to, to_end, to_next))
#else
#define call_codecvt_char_do_unshift(this, state, to, to_end, to_next) CALL_VTBL_FUNC(this, 32, \
        int, (const codecvt_char*, _Mbstatet*, char*, char*, char**), (this, state, to, to_end, to_next))
#endif
DEFINE_THISCALL_WRAPPER(codecvt_char_do_unshift, 20)
int __thiscall codecvt_char_do_unshift(const codecvt_char *this,
        _Mbstatet *state, char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p)\n", this, state, to, to_end, to_next);
    *to_next = to;
    return CODECVT_noconv;
}

/* ?unshift@?$codecvt@DDH@std@@QBEHAAHPAD1AAPAD@Z */
/* ?unshift@?$codecvt@DDH@std@@QEBAHAEAHPEAD1AEAPEAD@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char_unshift, 20)
int __thiscall codecvt_char_unshift(const codecvt_char *this,
        _Mbstatet *state, char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p)\n", this, state, to, to_end, to_next);
    return call_codecvt_char_do_unshift(this, state, to, to_end, to_next);
}

/* ?do_length@?$codecvt@DDH@std@@MBEHABHPBD1I@Z */
/* ?do_length@?$codecvt@DDH@std@@MEBAHAEBHPEBD1_K@Z */
#if _MSVCP_VER <= 100
#define call_codecvt_char_do_length(this, state, from, from_end, max) CALL_VTBL_FUNC(this, 28, \
        int, (const codecvt_char*, const _Mbstatet*, const char*, const char*, size_t), \
        (this, state, from, from_end, max))
#else
#define call_codecvt_char_do_length(this, state, from, from_end, max) CALL_VTBL_FUNC(this, 36, \
        int, (const codecvt_char*, const _Mbstatet*, const char*, const char*, size_t), \
        (this, state, from, from_end, max))
#endif
DEFINE_THISCALL_WRAPPER(codecvt_char_do_length, 20)
int __thiscall codecvt_char_do_length(const codecvt_char *this, const _Mbstatet *state,
        const char *from, const char *from_end, size_t max)
{
    TRACE("(%p %p %p %p %Iu)\n", this, state, from, from_end, max);
    return (from_end-from > max ? max : from_end-from);
}

/* ?length@?$codecvt@DDH@std@@QBEHABHPBD1I@Z */
/* ?length@?$codecvt@DDH@std@@QEBAHAEBHPEBD1_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char_length, 20)
int __thiscall codecvt_char_length(const codecvt_char *this, const _Mbstatet *state,
        const char *from, const char *from_end, size_t max)
{
    TRACE("(%p %p %p %p %Iu)\n", this, state, from, from_end, max);
    return call_codecvt_char_do_length(this, state, from, from_end, max);
}

/* ?id@?$codecvt@_WDH@std@@2V0locale@2@A */
locale_id codecvt_wchar_id = {0};
/* ?id@?$codecvt@GDH@std@@2V0locale@2@A */
locale_id codecvt_short_id = {0};

/* ??_7?$codecvt@_WDH@std@@6B@ */
extern const vtable_ptr codecvt_wchar_vtable;
/* ??_7?$codecvt@GDH@std@@6B@ */
extern const vtable_ptr codecvt_short_vtable;

/* ?_Init@?$codecvt@GDH@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$codecvt@GDH@std@@IEAAXAEBV_Locinfo@2@@Z */
/* ?_Init@?$codecvt@_WDH@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$codecvt@_WDH@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(codecvt_wchar__Init, 8)
void __thiscall codecvt_wchar__Init(codecvt_wchar *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Getcvt(locinfo, &this->cvt);
}

/* ??0?$codecvt@_WDH@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$codecvt@_WDH@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_ctor_locinfo, 12)
codecvt_wchar* __thiscall codecvt_wchar_ctor_locinfo(codecvt_wchar *this, const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);

    codecvt_base_ctor_refs(&this->base, refs);
    this->base.facet.vtable = &codecvt_wchar_vtable;

    codecvt_wchar__Init(this, locinfo);
    return this;
}

/* ??0?$codecvt@GDH@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$codecvt@GDH@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_short_ctor_locinfo, 12)
codecvt_wchar* __thiscall codecvt_short_ctor_locinfo(codecvt_wchar *this, const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);

    codecvt_wchar_ctor_locinfo(this, locinfo, refs);
    this->base.facet.vtable = &codecvt_short_vtable;
    return this;
}

/* ??0?$codecvt@_WDH@std@@QAE@I@Z */
/* ??0?$codecvt@_WDH@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_ctor_refs, 8)
codecvt_wchar* __thiscall codecvt_wchar_ctor_refs(codecvt_wchar *this, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %Iu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    codecvt_wchar_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$codecvt@GDH@std@@QAE@I@Z */
/* ??0?$codecvt@GDH@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_short_ctor_refs, 8)
codecvt_wchar* __thiscall codecvt_short_ctor_refs(codecvt_wchar *this, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %Iu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    codecvt_short_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$codecvt@GDH@std@@IAE@PBDI@Z */
/* ??0?$codecvt@GDH@std@@IEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_short_ctor_name, 12)
codecvt_wchar* __thiscall codecvt_short_ctor_name(codecvt_wchar *this, const char *name, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %s %Iu)\n", this, name, refs);

    _Locinfo_ctor_cstr(&locinfo, name);
    codecvt_short_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??_F?$codecvt@_WDH@std@@QAEXXZ */
/* ??_F?$codecvt@_WDH@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_ctor, 4)
codecvt_wchar* __thiscall codecvt_wchar_ctor(codecvt_wchar *this)
{
    return codecvt_wchar_ctor_refs(this, 0);
}

/* ??_F?$codecvt@GDH@std@@QAEXXZ */
/* ??_F?$codecvt@GDH@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(codecvt_short_ctor, 4)
codecvt_wchar* __thiscall codecvt_short_ctor(codecvt_wchar *this)
{
    return codecvt_short_ctor_refs(this, 0);
}

/* ??1?$codecvt@GDH@std@@UAE@XZ */
/* ??1?$codecvt@GDH@std@@UEAA@XZ */
/* ??1?$codecvt@GDH@std@@MAE@XZ */
/* ??1?$codecvt@GDH@std@@MEAA@XZ */
/* ??1?$codecvt@_WDH@std@@MAE@XZ */
/* ??1?$codecvt@_WDH@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_dtor, 4)
void __thiscall codecvt_wchar_dtor(codecvt_wchar *this)
{
    TRACE("(%p)\n", this);
    codecvt_base_dtor(&this->base);
}

DEFINE_THISCALL_WRAPPER(codecvt_wchar_vector_dtor, 8)
codecvt_wchar* __thiscall codecvt_wchar_vector_dtor(codecvt_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            codecvt_wchar_dtor(this+i);
        operator_delete(ptr);
    } else {
        codecvt_wchar_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$codecvt@_WDH@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$codecvt@_WDH@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl codecvt_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = operator_new(sizeof(codecvt_wchar));
        _Locinfo_ctor_cstr(&locinfo, locale_string_char_c_str(&loc->ptr->name));
        codecvt_wchar_ctor_locinfo((codecvt_wchar*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_CTYPE;
}

/* ?_Getcat@?$codecvt@_WDH@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$codecvt@_WDH@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl codecvt_wchar__Getcat_old(const locale_facet **facet)
{
    return codecvt_wchar__Getcat(facet, locale_classic());
}

codecvt_wchar* codecvt_wchar_use_facet(const locale *loc)
{
    static codecvt_wchar *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&codecvt_wchar_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (codecvt_wchar*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    codecvt_wchar__Getcat(&fac, loc);
    obj = (codecvt_wchar*)fac;
    call_locale_facet__Incref(&obj->base.facet);
    locale_facet_register(&obj->base.facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Getcat@?$codecvt@GDH@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$codecvt@GDH@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl codecvt_short__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = operator_new(sizeof(codecvt_wchar));
        _Locinfo_ctor_cstr(&locinfo, locale_string_char_c_str(&loc->ptr->name));
        codecvt_short_ctor((codecvt_wchar*)*facet);
        _Locinfo_dtor(&locinfo);
    }

    return LC_CTYPE;
}

/* ?_Getcat@?$codecvt@GDH@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$codecvt@GDH@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl codecvt_short__Getcat_old(const locale_facet **facet)
{
    return codecvt_short__Getcat(facet, locale_classic());
}

codecvt_wchar* codecvt_short_use_facet(const locale *loc)
{
    static codecvt_wchar *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&codecvt_short_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (codecvt_wchar*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    codecvt_short__Getcat(&fac, loc);
    obj = (codecvt_wchar*)fac;
    call_locale_facet__Incref(&obj->base.facet);
    locale_facet_register(&obj->base.facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Id_func@?$codecvt@_WDH@std@@SAAAVid@locale@2@XZ */
/* ?_Id_func@?$codecvt@_WDH@std@@SAAEAVid@locale@2@XZ */
locale_id* __cdecl codecvt_wchar__Id_func(void)
{
    TRACE("()\n");
    return &codecvt_wchar_id;
}

/* ?_Id_func@?$codecvt@GDH@std@@SAAAVid@locale@2@XZ */
/* ?_Id_func@?$codecvt@GDH@std@@SAAEAVid@locale@2@XZ */
locale_id* __cdecl codecvt_short__Id_func(void)
{
    TRACE("()\n");
    return &codecvt_short_id;
}

/* ?do_always_noconv@?$codecvt@GDH@std@@MBE_NXZ */
/* ?do_always_noconv@?$codecvt@GDH@std@@MEBA_NXZ */
/* ?do_always_noconv@?$codecvt@_WDH@std@@MBE_NXZ */
/* ?do_always_noconv@?$codecvt@_WDH@std@@MEBA_NXZ */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_do_always_noconv, 4)
bool __thiscall codecvt_wchar_do_always_noconv(const codecvt_wchar *this)
{
    TRACE("(%p)\n", this);
    return FALSE;
}

/* ?do_max_length@?$codecvt@GDH@std@@MBEHXZ */
/* ?do_max_length@?$codecvt@GDH@std@@MEBAHXZ */
/* ?do_max_length@?$codecvt@_WDH@std@@MBEHXZ */
/* ?do_max_length@?$codecvt@_WDH@std@@MEBAHXZ */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_do_max_length, 4)
int __thiscall codecvt_wchar_do_max_length(const codecvt_wchar *this)
{
    TRACE("(%p)\n", this);
    return MB_LEN_MAX;
}

/* ?do_encoding@?$codecvt@GDH@std@@MBEHXZ */
/* ?do_encoding@?$codecvt@GDH@std@@MEBAHXZ */
/* ?do_encoding@?$codecvt@_WDH@std@@MBEHXZ */
/* ?do_encoding@?$codecvt@_WDH@std@@MEBAHXZ */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_do_encoding, 4)
int __thiscall codecvt_wchar_do_encoding(const codecvt_wchar *this)
{
    TRACE("(%p)\n", this);
    return 0;
}

/* ?do_in@?$codecvt@GDH@std@@MBEHAAHPBD1AAPBDPAG3AAPAG@Z */
/* ?do_in@?$codecvt@GDH@std@@MEBAHAEAHPEBD1AEAPEBDPEAG3AEAPEAG@Z */
/* ?do_in@?$codecvt@_WDH@std@@MBEHAAHPBD1AAPBDPA_W3AAPA_W@Z */
/* ?do_in@?$codecvt@_WDH@std@@MEBAHAEAHPEBD1AEAPEBDPEA_W3AEAPEA_W@Z */
#if _MSVCP_VER <= 100
#define call_codecvt_wchar_do_in(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 16, int, \
            (const codecvt_wchar*, _Mbstatet*, const char*, const char*, const char**, wchar_t*, wchar_t*, wchar_t**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
#else
#define call_codecvt_wchar_do_in(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 24, int, \
            (const codecvt_wchar*, _Mbstatet*, const char*, const char*, const char**, wchar_t*, wchar_t*, wchar_t**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
#endif
DEFINE_THISCALL_WRAPPER(codecvt_wchar_do_in, 32)
int __thiscall codecvt_wchar_do_in(const codecvt_wchar *this, _Mbstatet *state,
        const char *from, const char *from_end, const char **from_next,
        wchar_t *to, wchar_t *to_end, wchar_t **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from,
            from_end, from_next, to, to_end, to_next);

    *from_next = from;
    *to_next = to;

    while(*from_next!=from_end && *to_next!=to_end) {
        switch(_Mbrtowc(*to_next, *from_next, from_end-*from_next, state, &this->cvt)) {
        case -2:
            *from_next = from_end;
            return CODECVT_partial;
        case -1:
            return CODECVT_error;
        case 2:
            (*from_next)++;
            /* fall through */
        case 0:
        case 1:
            (*from_next)++;
            (*to_next)++;
        }
    }

    return CODECVT_ok;
}

/* ?in@?$codecvt@GDH@std@@QBEHAAHPBD1AAPBDPAG3AAPAG@Z */
/* ?in@?$codecvt@GDH@std@@QEBAHAEAHPEBD1AEAPEBDPEAG3AEAPEAG@Z */
/* ?in@?$codecvt@_WDH@std@@QBEHAAHPBD1AAPBDPA_W3AAPA_W@Z */
/* ?in@?$codecvt@_WDH@std@@QEBAHAEAHPEBD1AEAPEBDPEA_W3AEAPEA_W@Z */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_in, 32)
int __thiscall codecvt_wchar_in(const codecvt_wchar *this, _Mbstatet *state,
        const char *from, const char *from_end, const char **from_next,
        wchar_t *to, wchar_t *to_end, wchar_t **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from,
            from_end, from_next, to, to_end, to_next);
    return call_codecvt_wchar_do_in(this, state, from,
            from_end, from_next, to, to_end, to_next);
}

/* ?do_out@?$codecvt@GDH@std@@MBEHAAHPBG1AAPBGPAD3AAPAD@Z */
/* ?do_out@?$codecvt@GDH@std@@MEBAHAEAHPEBG1AEAPEBGPEAD3AEAPEAD@Z */
/* ?do_out@?$codecvt@_WDH@std@@MBEHAAHPB_W1AAPB_WPAD3AAPAD@Z */
/* ?do_out@?$codecvt@_WDH@std@@MEBAHAEAHPEB_W1AEAPEB_WPEAD3AEAPEAD@Z */
#if _MSVCP_VER <= 100
#define call_codecvt_wchar_do_out(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 20, int, \
            (const codecvt_wchar*, _Mbstatet*, const wchar_t*, const wchar_t*, const wchar_t**, char*, char*, char**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
#else
#define call_codecvt_wchar_do_out(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 28, int, \
            (const codecvt_wchar*, _Mbstatet*, const wchar_t*, const wchar_t*, const wchar_t**, char*, char*, char**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
#endif
DEFINE_THISCALL_WRAPPER(codecvt_wchar_do_out, 32)
int __thiscall codecvt_wchar_do_out(const codecvt_wchar *this, _Mbstatet *state,
        const wchar_t *from, const wchar_t *from_end, const wchar_t **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from,
            from_end, from_next, to, to_end, to_next);

    *from_next = from;
    *to_next = to;

    while(*from_next!=from_end && *to_next!=to_end) {
        _Mbstatet old_state = *state;
        int size;
        char buf[MB_LEN_MAX];

        switch((size = _Wcrtomb(buf, **from_next, state, &this->cvt))) {
        case -1:
            return CODECVT_error;
        default:
            if(size > from_end-*from_next) {
                *state = old_state;
                return CODECVT_partial;
            }

            (*from_next)++;
            memcpy_s(*to_next, to_end-*to_next, buf, size);
            (*to_next) += size;
        }
    }

    return CODECVT_ok;
}

/* ?out@?$codecvt@GDH@std@@QBEHAAHPBG1AAPBGPAD3AAPAD@Z */
/* ?out@?$codecvt@GDH@std@@QEBAHAEAHPEBG1AEAPEBGPEAD3AEAPEAD@Z */
/* ?out@?$codecvt@_WDH@std@@QBEHAAHPB_W1AAPB_WPAD3AAPAD@Z */
/* ?out@?$codecvt@_WDH@std@@QEBAHAEAHPEB_W1AEAPEB_WPEAD3AEAPEAD@Z */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_out, 32)
int __thiscall codecvt_wchar_out(const codecvt_wchar *this, _Mbstatet *state,
        const wchar_t *from, const wchar_t *from_end, const wchar_t **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from,
            from_end, from_next, to, to_end, to_next);
    return call_codecvt_wchar_do_out(this, state, from,
            from_end, from_next, to, to_end, to_next);
}

/* ?do_unshift@?$codecvt@GDH@std@@MBEHAAHPAD1AAPAD@Z */
/* ?do_unshift@?$codecvt@GDH@std@@MEBAHAEAHPEAD1AEAPEAD@Z */
/* ?do_unshift@?$codecvt@_WDH@std@@MBEHAAHPAD1AAPAD@Z */
/* ?do_unshift@?$codecvt@_WDH@std@@MEBAHAEAHPEAD1AEAPEAD@Z */
#if _MSVCP_VER <= 100
#define call_codecvt_wchar_do_unshift(this, state, to, to_end, to_next) CALL_VTBL_FUNC(this, 24, \
        int, (const codecvt_wchar*, _Mbstatet*, char*, char*, char**), (this, state, to, to_end, to_next))
#else
#define call_codecvt_wchar_do_unshift(this, state, to, to_end, to_next) CALL_VTBL_FUNC(this, 32, \
        int, (const codecvt_wchar*, _Mbstatet*, char*, char*, char**), (this, state, to, to_end, to_next))
#endif
DEFINE_THISCALL_WRAPPER(codecvt_wchar_do_unshift, 20)
int __thiscall codecvt_wchar_do_unshift(const codecvt_wchar *this,
        _Mbstatet *state, char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p)\n", this, state, to, to_end, to_next);
    if(MBSTATET_TO_INT(state))
        WARN("unexpected state: %x\n", MBSTATET_TO_INT(state));

    *to_next = to;
    return CODECVT_ok;
}

/* ?unshift@?$codecvt@GDH@std@@QBEHAAHPAD1AAPAD@Z */
/* ?unshift@?$codecvt@GDH@std@@QEBAHAEAHPEAD1AEAPEAD@Z */
/* ?unshift@?$codecvt@_WDH@std@@QBEHAAHPAD1AAPAD@Z */
/* ?unshift@?$codecvt@_WDH@std@@QEBAHAEAHPEAD1AEAPEAD@Z */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_unshift, 20)
int __thiscall codecvt_wchar_unshift(const codecvt_wchar *this,
        _Mbstatet *state, char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p)\n", this, state, to, to_end, to_next);
    return call_codecvt_wchar_do_unshift(this, state, to, to_end, to_next);
}

/* ?do_length@?$codecvt@GDH@std@@MBEHABHPBD1I@Z */
/* ?do_length@?$codecvt@GDH@std@@MEBAHAEBHPEBD1_K@Z */
/* ?do_length@?$codecvt@_WDH@std@@MBEHABHPBD1I@Z */
/* ?do_length@?$codecvt@_WDH@std@@MEBAHAEBHPEBD1_K@Z */
#if _MSVCP_VER <= 100
#define call_codecvt_wchar_do_length(this, state, from, from_end, max) CALL_VTBL_FUNC(this, 28, \
        int, (const codecvt_wchar*, const _Mbstatet*, const char*, const char*, size_t), \
        (this, state, from, from_end, max))
#else
#define call_codecvt_wchar_do_length(this, state, from, from_end, max) CALL_VTBL_FUNC(this, 36, \
        int, (const codecvt_wchar*, const _Mbstatet*, const char*, const char*, size_t), \
        (this, state, from, from_end, max))
#endif
DEFINE_THISCALL_WRAPPER(codecvt_wchar_do_length, 20)
int __thiscall codecvt_wchar_do_length(const codecvt_wchar *this, const _Mbstatet *state,
        const char *from, const char *from_end, size_t max)
{
    _Mbstatet tmp_state = *state;
    int ret=0;

    TRACE("(%p %p %p %p %Iu)\n", this, state, from, from_end, max);

    while(ret<max && from!=from_end) {
        switch(_Mbrtowc(NULL, from, from_end-from, &tmp_state, &this->cvt)) {
        case -2:
        case -1:
            return ret;
        case 2:
            from++;
            /* fall through */
        case 0:
        case 1:
            from++;
            ret++;
        }
    }

    return ret;
}

/* ?length@?$codecvt@GDH@std@@QBEHABHPBD1I@Z */
/* ?length@?$codecvt@GDH@std@@QEBAHAEBHPEBD1_K@Z */
/* ?length@?$codecvt@_WDH@std@@QBEHABHPBD1I@Z */
/* ?length@?$codecvt@_WDH@std@@QEBAHAEBHPEBD1_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_wchar_length, 20)
int __thiscall codecvt_wchar_length(const codecvt_wchar *this, const _Mbstatet *state,
        const char *from, const char *from_end, size_t max)
{
    TRACE("(%p %p %p %p %Iu)\n", this, state, from, from_end, max);
    return call_codecvt_wchar_do_length(this, state, from, from_end, max);
}

#if _MSVCP_VER >= 140

/* ??_7?$codecvt@_SDU_Mbstatet@@@std@@6B@ */
extern const vtable_ptr codecvt_char16_vtable;

/* ?_Init@?$codecvt@_SDU_Mbstatet@@@std@@IAAXABV_Locinfo@2@@Z */
/* ?_Init@?$codecvt@_SDU_Mbstatet@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$codecvt@_SDU_Mbstatet@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char16__Init, 8)
void __thiscall codecvt_char16__Init(codecvt_char16 *this, const _Locinfo *locinfo)
{
    FIXME("(%p %p) stub\n", this, locinfo);
}

/* ??0?$codecvt@_SDU_Mbstatet@@@std@@QAA@ABV_Locinfo@1@I@Z */
/* ??0?$codecvt@_SDU_Mbstatet@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$codecvt@_SDU_Mbstatet@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char16_ctor_locinfo, 12)
codecvt_char16* __thiscall codecvt_char16_ctor_locinfo(codecvt_char16 *this,
        const _Locinfo *locinfo, size_t refs)
{
    FIXME("(%p %p %Iu) stub\n", this, locinfo, refs);
    return NULL;
}

/* ??0?$codecvt@_SDU_Mbstatet@@@std@@QAA@I@Z */
/* ??0?$codecvt@_SDU_Mbstatet@@@std@@QAE@I@Z */
/* ??0?$codecvt@_SDU_Mbstatet@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char16_ctor_refs, 8)
codecvt_char* __thiscall codecvt_char16_ctor_refs(codecvt_char16 *this, size_t refs)
{
    FIXME("(%p %Iu) stub\n", this, refs);
    return NULL;
}

/* ??0?$codecvt@_SDU_Mbstatet@@@std@@QAA@ABV_Locinfo@1@KW4_Codecvt_mode@1@I@Z */
/* ??0?$codecvt@_SDU_Mbstatet@@@std@@QAE@ABV_Locinfo@1@KW4_Codecvt_mode@1@I@Z */
/* ??0?$codecvt@_SDU_Mbstatet@@@std@@QEAA@AEBV_Locinfo@1@KW4_Codecvt_mode@1@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char16_ctor_mode, 16)
codecvt_char16* __thiscall codecvt_char16_ctor_mode(codecvt_char16 *this,
        ULONG max_code, codecvt_convert_mode mode, size_t refs)
{
    FIXME("(%p %ld %d %Iu) stub\n", this, max_code, mode, refs);
    return NULL;
}

/* ??_F?$codecvt@_SDU_Mbstatet@@@std@@QAAXXZ */
/* ??_F?$codecvt@_SDU_Mbstatet@@@std@@QAEXXZ */
/* ??_F?$codecvt@_SDU_Mbstatet@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(codecvt_char16_ctor, 4)
codecvt_char* __thiscall codecvt_char16_ctor(codecvt_char16 *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ??1?$codecvt@_SDU_Mbstatet@@@std@@MAA@XZ */
/* ??1?$codecvt@_SDU_Mbstatet@@@std@@MAE@XZ */
/* ??1?$codecvt@_SDU_Mbstatet@@@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(codecvt_char16_dtor, 4)
void __thiscall codecvt_char16_dtor(codecvt_char16 *this)
{
    FIXME("(%p) stub\n", this);
}

DEFINE_THISCALL_WRAPPER(codecvt_char16_vector_dtor, 8)
codecvt_char16* __thiscall codecvt_char16_vector_dtor(
        codecvt_char16 *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            codecvt_char16_dtor(this+i);
        operator_delete(ptr);
    } else {
        codecvt_char16_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$codecvt@_SDU_Mbstatet@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$codecvt@_SDU_Mbstatet@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl codecvt_char16__Getcat(const locale_facet **facet, const locale *loc)
{
    FIXME("(%p %p) stub\n", facet, loc);
    return 0;
}

/* ?do_always_noconv@?$codecvt@_SDU_Mbstatet@@@std@@MBA_NXZ */
/* ?do_always_noconv@?$codecvt@_SDU_Mbstatet@@@std@@MBE_NXZ */
/* ?do_always_noconv@?$codecvt@_SDU_Mbstatet@@@std@@MEBA_NXZ */
DEFINE_THISCALL_WRAPPER(codecvt_char16_do_always_noconv, 4)
bool __thiscall codecvt_char16_do_always_noconv(const codecvt_char16 *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?do_encoding@?$codecvt@_SDU_Mbstatet@@@std@@MBAHXZ */
/* ?do_encoding@?$codecvt@_SDU_Mbstatet@@@std@@MBEHXZ */
/* ?do_encoding@?$codecvt@_SDU_Mbstatet@@@std@@MEBAHXZ */
DEFINE_THISCALL_WRAPPER(codecvt_char16_do_encoding, 4)
int __thiscall codecvt_char16_do_encoding(const codecvt_char16 *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?do_in@?$codecvt@_SDU_Mbstatet@@@std@@MBAHAAU_Mbstatet@@PBD1AAPBDPA_S3AAPA_S@Z */
/* ?do_in@?$codecvt@_SDU_Mbstatet@@@std@@MBEHAAU_Mbstatet@@PBD1AAPBDPA_S3AAPA_S@Z */
/* ?do_in@?$codecvt@_SDU_Mbstatet@@@std@@MEBAHAEAU_Mbstatet@@PEBD1AEAPEBDPEA_S3AEAPEA_S@Z */
#define call_codecvt_char16_do_in(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 24, int, \
            (const codecvt_char16*, _Mbstatet*, const char*, const char*, const char**, \
                    char16_t*, char16_t*, char16_t**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
DEFINE_THISCALL_WRAPPER(codecvt_char16_do_in, 32)
int __thiscall codecvt_char16_do_in(const codecvt_char16 *this, _Mbstatet *state,
        const char *from, const char *from_end, const char **from_next,
        char16_t *to, char16_t *to_end, char16_t **to_next)
{
    FIXME("(%p %p %p %p %p %p %p %p) stub\n", this, state, from,
            from_end, from_next, to, to_end, to_next);
    return 0;
}

/* ?in@?$codecvt@_SDU_Mbstatet@@@std@@QBAHAAU_Mbstatet@@PBD1AAPBDPA_S3AAPA_S@Z */
/* ?in@?$codecvt@_SDU_Mbstatet@@@std@@QBEHAAU_Mbstatet@@PBD1AAPBDPA_S3AAPA_S@Z */
/* ?in@?$codecvt@_SDU_Mbstatet@@@std@@QEBAHAEAU_Mbstatet@@PEBD1AEAPEBDPEA_S3AEAPEA_S@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char16_in, 32)
int __thiscall codecvt_char16_in(const codecvt_char16 *this, _Mbstatet *state,
        const char *from, const char *from_end, const char **from_next,
        char16_t *to, char16_t *to_end, char16_t **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from, from_end,
            from_next, to, to_end, to_next);
    return call_codecvt_char16_do_in(this, state, from, from_end, from_next,
            to, to_end, to_next);
}

/* ?do_length@?$codecvt@_SDU_Mbstatet@@@std@@MBAHAAU_Mbstatet@@PBD1I@Z */
/* ?do_length@?$codecvt@_SDU_Mbstatet@@@std@@MBEHAAU_Mbstatet@@PBD1I@Z */
/* ?do_length@?$codecvt@_SDU_Mbstatet@@@std@@MEBAHAEAU_Mbstatet@@PEBD1_K@Z */
#define call_codecvt_char16_do_length(this, state, from, from_end, max) CALL_VTBL_FUNC(this, 36, \
        int, (const codecvt_char16*, const _Mbstatet*, const char*, const char*, size_t), \
        (this, state, from, from_end, max))
DEFINE_THISCALL_WRAPPER(codecvt_char16_do_length, 20)
int __thiscall codecvt_char16_do_length(const codecvt_char16 *this, const _Mbstatet *state,
        const char *from, const char *from_end, size_t max)
{
    FIXME("(%p %p %p %p %Iu) stub\n", this, state, from, from_end, max);
    return 0;
}


/* ?length@?$codecvt@_SDU_Mbstatet@@@std@@QBAHAAU_Mbstatet@@PBD1I@Z */
/* ?length@?$codecvt@_SDU_Mbstatet@@@std@@QBEHAAU_Mbstatet@@PBD1I@Z */
/* ?length@?$codecvt@_SDU_Mbstatet@@@std@@QEBAHAEAU_Mbstatet@@PEBD1_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char16_length, 20)
int __thiscall codecvt_char16_length(const codecvt_char16 *this, const _Mbstatet *state,
        const char *from, const char *from_end, size_t max)
{
    TRACE("(%p %p %p %p %Iu)\n", this, state, from, from_end, max);
    return call_codecvt_char16_do_length(this, state, from, from_end, max);
}

/* ?do_max_length@?$codecvt@_SDU_Mbstatet@@@std@@MBAHXZ */
/* ?do_max_length@?$codecvt@_SDU_Mbstatet@@@std@@MBEHXZ */
/* ?do_max_length@?$codecvt@_SDU_Mbstatet@@@std@@MEBAHXZ */
DEFINE_THISCALL_WRAPPER(codecvt_char16_do_max_length, 4)
int __thiscall codecvt_char16_do_max_length(const codecvt_char16 *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?do_out@?$codecvt@_SDU_Mbstatet@@@std@@MBAHAAU_Mbstatet@@PB_S1AAPB_SPAD3AAPAD@Z */
/* ?do_out@?$codecvt@_SDU_Mbstatet@@@std@@MBEHAAU_Mbstatet@@PB_S1AAPB_SPAD3AAPAD@Z */
/* ?do_out@?$codecvt@_SDU_Mbstatet@@@std@@MEBAHAEAU_Mbstatet@@PEB_S1AEAPEB_SPEAD3AEAPEAD@Z */
#define call_codecvt_char16_do_out(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 28, int, \
            (const codecvt_char16*, _Mbstatet*, const char16_t*, const char16_t*, const char16_t**, \
                    char*, char*, char**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
DEFINE_THISCALL_WRAPPER(codecvt_char16_do_out, 32)
int __thiscall codecvt_char16_do_out(const codecvt_char16 *this, _Mbstatet *state,
        const char16_t *from, const char16_t *from_end, const char16_t **from_next,
        char *to, char *to_end, char **to_next)
{
    FIXME("(%p %p %p %p %p %p %p %p) stub\n", this, state, from,
            from_end, from_next, to, to_end, to_next);
    return 0;
}

/* ?out@?$codecvt@_SDU_Mbstatet@@@std@@QBAHAAU_Mbstatet@@PB_S1AAPB_SPAD3AAPAD@Z */
/* ?out@?$codecvt@_SDU_Mbstatet@@@std@@QBEHAAU_Mbstatet@@PB_S1AAPB_SPAD3AAPAD@Z */
/* ?out@?$codecvt@_SDU_Mbstatet@@@std@@QEBAHAEAU_Mbstatet@@PEB_S1AEAPEB_SPEAD3AEAPEAD@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char16_out, 32)
int __thiscall codecvt_char16_out(const codecvt_char16 *this, _Mbstatet *state,
        const char16_t *from, const char16_t *from_end, const char16_t **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from,
            from_end, from_next, to, to_end, to_next);
    return call_codecvt_char16_do_out(this, state, from,
            from_end, from_next, to, to_end, to_next);
}

/* ?do_unshift@?$codecvt@_SDU_Mbstatet@@@std@@MBAHAAU_Mbstatet@@PAD1AAPAD@Z */
/* ?do_unshift@?$codecvt@_SDU_Mbstatet@@@std@@MBEHAAU_Mbstatet@@PAD1AAPAD@Z */
/* ?do_unshift@?$codecvt@_SDU_Mbstatet@@@std@@MEBAHAEAU_Mbstatet@@PEAD1AEAPEAD@Z */
#define call_codecvt_char16_do_unshift(this, state, to, to_end, to_next) CALL_VTBL_FUNC(this, 32, \
        int, (const codecvt_char16*, _Mbstatet*, char*, char*, char**), (this, state, to, to_end, to_next))
DEFINE_THISCALL_WRAPPER(codecvt_char16_do_unshift, 20)
int __thiscall codecvt_char16_do_unshift(const codecvt_char16 *this,
        _Mbstatet *state, char *to, char *to_end, char **to_next)
{
    FIXME("(%p %p %p %p %p) stub\n", this, state, to, to_end, to_next);
    return 0;
}

/* ?unshift@?$codecvt@_SDU_Mbstatet@@@std@@QBAHAAU_Mbstatet@@PAD1AAPAD@Z */
/* ?unshift@?$codecvt@_SDU_Mbstatet@@@std@@QBEHAAU_Mbstatet@@PAD1AAPAD@Z */
/* ?unshift@?$codecvt@_SDU_Mbstatet@@@std@@QEBAHAEAU_Mbstatet@@PEAD1AEAPEAD@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char16_unshift, 20)
int __thiscall codecvt_char16_unshift(const codecvt_char16 *this,
        _Mbstatet *state, char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p)\n", this, state, to, to_end, to_next);
    return call_codecvt_char16_do_unshift(this, state, to, to_end, to_next);
}

/* ??_7?$codecvt@_UDU_Mbstatet@@@std@@6B@ */
extern const vtable_ptr codecvt_char32_vtable;

/* ?_Init@?$codecvt@_UDU_Mbstatet@@@std@@IAAXABV_Locinfo@2@@Z */
/* ?_Init@?$codecvt@_UDU_Mbstatet@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$codecvt@_UDU_Mbstatet@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char32__Init, 8)
void __thiscall codecvt_char32__Init(codecvt_char32 *this, const _Locinfo *locinfo)
{
    FIXME("(%p %p) stub\n", this, locinfo);
}

/* ??0?$codecvt@_UDU_Mbstatet@@@std@@QAA@ABV_Locinfo@1@I@Z */
/* ??0?$codecvt@_UDU_Mbstatet@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$codecvt@_UDU_Mbstatet@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char32_ctor_locinfo, 12)
codecvt_char32* __thiscall codecvt_char32_ctor_locinfo(codecvt_char32 *this, const _Locinfo *locinfo, size_t refs)
{
    FIXME("(%p %p %Iu) stub\n", this, locinfo, refs);
    return NULL;
}

/* ??0?$codecvt@_UDU_Mbstatet@@@std@@QAA@I@Z */
/* ??0?$codecvt@_UDU_Mbstatet@@@std@@QAE@I@Z */
/* ??0?$codecvt@_UDU_Mbstatet@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char32_ctor_refs, 8)
codecvt_char* __thiscall codecvt_char32_ctor_refs(codecvt_char32 *this, size_t refs)
{
    FIXME("(%p %Iu) stub\n", this, refs);
    return NULL;
}

/* ??0?$codecvt@_UDU_Mbstatet@@@std@@QAA@ABV_Locinfo@1@KW4_Codecvt_mode@1@I@Z */
/* ??0?$codecvt@_UDU_Mbstatet@@@std@@QAE@ABV_Locinfo@1@KW4_Codecvt_mode@1@I@Z */
/* ??0?$codecvt@_UDU_Mbstatet@@@std@@QEAA@AEBV_Locinfo@1@KW4_Codecvt_mode@1@_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char32_ctor_mode, 16)
codecvt_char32* __thiscall codecvt_char32_ctor_mode(codecvt_char32 *this, ULONG max_code,
        codecvt_convert_mode mode, size_t refs)
{
    FIXME("(%p %ld %d %Iu) stub\n", this, max_code, mode, refs);
    return NULL;
}

/* ??_F?$codecvt@_UDU_Mbstatet@@@std@@QAAXXZ */
/* ??_F?$codecvt@_UDU_Mbstatet@@@std@@QAEXXZ */
/* ??_F?$codecvt@_UDU_Mbstatet@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(codecvt_char32_ctor, 4)
codecvt_char* __thiscall codecvt_char32_ctor(codecvt_char32 *this)
{
    FIXME("(%p) stub\n", this);
    return NULL;
}

/* ??1?$codecvt@_UDU_Mbstatet@@@std@@MAA@XZ */
/* ??1?$codecvt@_UDU_Mbstatet@@@std@@MAE@XZ */
/* ??1?$codecvt@_UDU_Mbstatet@@@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(codecvt_char32_dtor, 4)
void __thiscall codecvt_char32_dtor(codecvt_char32 *this)
{
    FIXME("(%p) stub\n", this);
}

DEFINE_THISCALL_WRAPPER(codecvt_char32_vector_dtor, 8)
codecvt_char32* __thiscall codecvt_char32_vector_dtor(codecvt_char32 *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            codecvt_char32_dtor(this+i);
        operator_delete(ptr);
    } else {
        codecvt_char32_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$codecvt@_UDU_Mbstatet@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$codecvt@_UDU_Mbstatet@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl codecvt_char32__Getcat(const locale_facet **facet, const locale *loc)
{
    FIXME("(%p %p) stub\n", facet, loc);
    return 0;
}

/* ?do_always_noconv@?$codecvt@_UDU_Mbstatet@@@std@@MBA_NXZ */
/* ?do_always_noconv@?$codecvt@_UDU_Mbstatet@@@std@@MBE_NXZ */
/* ?do_always_noconv@?$codecvt@_UDU_Mbstatet@@@std@@MEBA_NXZ */
DEFINE_THISCALL_WRAPPER(codecvt_char32_do_always_noconv, 4)
bool __thiscall codecvt_char32_do_always_noconv(const codecvt_char32 *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?do_encoding@?$codecvt@_UDU_Mbstatet@@@std@@MBAHXZ */
/* ?do_encoding@?$codecvt@_UDU_Mbstatet@@@std@@MBEHXZ */
/* ?do_encoding@?$codecvt@_UDU_Mbstatet@@@std@@MEBAHXZ */
DEFINE_THISCALL_WRAPPER(codecvt_char32_do_encoding, 4)
int __thiscall codecvt_char32_do_encoding(const codecvt_char32 *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?do_in@?$codecvt@_UDU_Mbstatet@@@std@@MBEHAAU_Mbstatet@@PBD1AAPBDPA_U3AAPA_U@Z */
/* ?do_in@?$codecvt@_UDU_Mbstatet@@@std@@MBEHAAU_Mbstatet@@PBD1AAPBDPA_U3AAPA_U@Z */
/* ?do_in@?$codecvt@_UDU_Mbstatet@@@std@@MEBAHAEAU_Mbstatet@@PEBD1AEAPEBDPEA_U3AEAPEA_U@Z */
#define call_codecvt_char32_do_in(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 24, int, \
            (const codecvt_char32*, _Mbstatet*, const char*, const char*, const char**, \
                    char32_t*, char32_t*, char32_t**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
DEFINE_THISCALL_WRAPPER(codecvt_char32_do_in, 32)
int __thiscall codecvt_char32_do_in(const codecvt_char32 *this, _Mbstatet *state,
        const char *from, const char *from_end, const char **from_next,
        char32_t *to, char32_t *to_end, char32_t **to_next)
{
    FIXME("(%p %p %p %p %p %p %p %p) stub\n", this, state, from,
            from_end, from_next, to, to_end, to_next);
    return 0;
}

/* ?in@?$codecvt@_UDU_Mbstatet@@@std@@QBAHAAU_Mbstatet@@PBD1AAPBDPA_U3AAPA_U@Z */
/* ?in@?$codecvt@_UDU_Mbstatet@@@std@@QBEHAAU_Mbstatet@@PBD1AAPBDPA_U3AAPA_U@Z */
/* ?in@?$codecvt@_UDU_Mbstatet@@@std@@QEBAHAEAU_Mbstatet@@PEBD1AEAPEBDPEA_U3AEAPEA_U@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char32_in, 32)
int __thiscall codecvt_char32_in(const codecvt_char32 *this, _Mbstatet *state,
        const char *from, const char *from_end, const char **from_next,
        char32_t *to, char32_t *to_end, char32_t **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from, from_end,
            from_next, to, to_end, to_next);
    return call_codecvt_char32_do_in(this, state, from, from_end, from_next,
            to, to_end, to_next);
}

/* ?do_length@?$codecvt@_UDU_Mbstatet@@@std@@MBAHAAU_Mbstatet@@PBD1I@Z */
/* ?do_length@?$codecvt@_UDU_Mbstatet@@@std@@MBEHAAU_Mbstatet@@PBD1I@Z */
/* ?do_length@?$codecvt@_UDU_Mbstatet@@@std@@MEBAHAEAU_Mbstatet@@PEBD1_K@Z */
#define call_codecvt_char32_do_length(this, state, from, from_end, max) CALL_VTBL_FUNC(this, 36, \
        int, (const codecvt_char32*, const _Mbstatet*, const char*, const char*, size_t), \
        (this, state, from, from_end, max))
DEFINE_THISCALL_WRAPPER(codecvt_char32_do_length, 20)
int __thiscall codecvt_char32_do_length(const codecvt_char32 *this, const _Mbstatet *state,
        const char *from, const char *from_end, size_t max)
{
    FIXME("(%p %p %p %p %Iu) stub\n", this, state, from, from_end, max);
    return 0;
}


/* ?length@?$codecvt@_UDU_Mbstatet@@@std@@QBAHAAU_Mbstatet@@PBD1I@Z */
/* ?length@?$codecvt@_UDU_Mbstatet@@@std@@QBEHAAU_Mbstatet@@PBD1I@Z */
/* ?length@?$codecvt@_UDU_Mbstatet@@@std@@QEBAHAEAU_Mbstatet@@PEBD1_K@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char32_length, 20)
int __thiscall codecvt_char32_length(const codecvt_char32 *this, const _Mbstatet *state,
        const char *from, const char *from_end, size_t max)
{
    TRACE("(%p %p %p %p %Iu)\n", this, state, from, from_end, max);
    return call_codecvt_char32_do_length(this, state, from, from_end, max);
}

/* ?do_max_length@?$codecvt@_UDU_Mbstatet@@@std@@MBAHXZ */
/* ?do_max_length@?$codecvt@_UDU_Mbstatet@@@std@@MBEHXZ */
/* ?do_max_length@?$codecvt@_UDU_Mbstatet@@@std@@MEBAHXZ */
DEFINE_THISCALL_WRAPPER(codecvt_char32_do_max_length, 4)
int __thiscall codecvt_char32_do_max_length(const codecvt_char32 *this)
{
    FIXME("(%p) stub\n", this);
    return 0;
}

/* ?do_out@?$codecvt@_UDU_Mbstatet@@@std@@MBAHAAU_Mbstatet@@PB_U1AAPB_UPAD3AAPAD@Z */
/* ?do_out@?$codecvt@_UDU_Mbstatet@@@std@@MBEHAAU_Mbstatet@@PB_U1AAPB_UPAD3AAPAD@Z */
/* ?do_out@?$codecvt@_UDU_Mbstatet@@@std@@MEBAHAEAU_Mbstatet@@PEB_U1AEAPEB_UPEAD3AEAPEAD@Z */
#define call_codecvt_char32_do_out(this, state, from, from_end, from_next, to, to_end, to_next) \
    CALL_VTBL_FUNC(this, 28, int, \
            (const codecvt_char32*, _Mbstatet*, const char32_t*, const char32_t*, const char32_t**, \
                    char*, char*, char**), \
            (this, state, from, from_end, from_next, to, to_end, to_next))
DEFINE_THISCALL_WRAPPER(codecvt_char32_do_out, 32)
int __thiscall codecvt_char32_do_out(const codecvt_char32 *this, _Mbstatet *state,
        const char32_t *from, const char32_t *from_end, const char32_t **from_next,
        char *to, char *to_end, char **to_next)
{
    FIXME("(%p %p %p %p %p %p %p %p) stub\n", this, state, from,
            from_end, from_next, to, to_end, to_next);
    return 0;
}

/* ?out@?$codecvt@_UDU_Mbstatet@@@std@@QBAHAAU_Mbstatet@@PB_U1AAPB_UPAD3AAPAD@Z */
/* ?out@?$codecvt@_UDU_Mbstatet@@@std@@QBEHAAU_Mbstatet@@PB_U1AAPB_UPAD3AAPAD@Z */
/* ?out@?$codecvt@_UDU_Mbstatet@@@std@@QEBAHAEAU_Mbstatet@@PEB_U1AEAPEB_UPEAD3AEAPEAD@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char32_out, 32)
int __thiscall codecvt_char32_out(const codecvt_char32 *this, _Mbstatet *state,
        const char32_t *from, const char32_t *from_end, const char32_t **from_next,
        char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p %p %p %p)\n", this, state, from,
            from_end, from_next, to, to_end, to_next);
    return call_codecvt_char32_do_out(this, state, from,
            from_end, from_next, to, to_end, to_next);
}

/* ?do_unshift@?$codecvt@_UDU_Mbstatet@@@std@@MBAHAAU_Mbstatet@@PAD1AAPAD@Z */
/* ?do_unshift@?$codecvt@_UDU_Mbstatet@@@std@@MBEHAAU_Mbstatet@@PAD1AAPAD@Z */
/* ?do_unshift@?$codecvt@_UDU_Mbstatet@@@std@@MEBAHAEAU_Mbstatet@@PEAD1AEAPEAD@Z */
#define call_codecvt_char32_do_unshift(this, state, to, to_end, to_next) CALL_VTBL_FUNC(this, 32, \
        int, (const codecvt_char32*, _Mbstatet*, char*, char*, char**), (this, state, to, to_end, to_next))
DEFINE_THISCALL_WRAPPER(codecvt_char32_do_unshift, 20)
int __thiscall codecvt_char32_do_unshift(const codecvt_char32 *this,
        _Mbstatet *state, char *to, char *to_end, char **to_next)
{
    FIXME("(%p %p %p %p %p) stub\n", this, state, to, to_end, to_next);
    return 0;
}

/* ?unshift@?$codecvt@_UDU_Mbstatet@@@std@@QBAHAAU_Mbstatet@@PAD1AAPAD@Z */
/* ?unshift@?$codecvt@_UDU_Mbstatet@@@std@@QBEHAAU_Mbstatet@@PAD1AAPAD@Z */
/* ?unshift@?$codecvt@_UDU_Mbstatet@@@std@@QEBAHAEAU_Mbstatet@@PEAD1AEAPEAD@Z */
DEFINE_THISCALL_WRAPPER(codecvt_char32_unshift, 20)
int __thiscall codecvt_char32_unshift(const codecvt_char32 *this,
        _Mbstatet *state, char *to, char *to_end, char **to_next)
{
    TRACE("(%p %p %p %p %p)\n", this, state, to, to_end, to_next);
    return call_codecvt_char32_do_unshift(this, state, to, to_end, to_next);
}
#endif

/* ?id@?$numpunct@D@std@@2V0locale@2@A */
locale_id numpunct_char_id = {0};

/* ??_7?$numpunct@D@std@@6B@ */
extern const vtable_ptr numpunct_char_vtable;

/* ?_Init@?$numpunct@D@std@@IAEXABV_Locinfo@2@_N@Z */
/* ?_Init@?$numpunct@D@std@@IEAAXAEBV_Locinfo@2@_N@Z */
DEFINE_THISCALL_WRAPPER(numpunct_char__Init, 12)
void __thiscall numpunct_char__Init(numpunct_char *this, const _Locinfo *locinfo, bool isdef)
{
    int len;

    TRACE("(%p %p %d)\n", this, locinfo, isdef);

    len = strlen(_Locinfo__Getfalse(locinfo))+1;
    this->false_name = operator_new(len);
    memcpy((char*)this->false_name, _Locinfo__Getfalse(locinfo), len);

    len = strlen(_Locinfo__Gettrue(locinfo))+1;
    this->true_name = operator_new(len);
    memcpy((char*)this->true_name, _Locinfo__Gettrue(locinfo), len);

    if(isdef) {
        this->grouping = operator_new(1);
        *(char*)this->grouping = 0;

        this->dp = '.';
        this->sep = ',';
    } else {
        const struct lconv *lc = _Locinfo__Getlconv(locinfo);

        len = strlen(lc->grouping)+1;
        this->grouping = operator_new(len);
        memcpy((char*)this->grouping, lc->grouping, len);

        this->dp = lc->decimal_point[0];
        this->sep = lc->thousands_sep[0];
    }
}

/* ?_Tidy@?$numpunct@D@std@@AAEXXZ */
/* ?_Tidy@?$numpunct@D@std@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(numpunct_char__Tidy, 4)
void __thiscall numpunct_char__Tidy(numpunct_char *this)
{
    TRACE("(%p)\n", this);

    operator_delete((char*)this->grouping);
    operator_delete((char*)this->false_name);
    operator_delete((char*)this->true_name);
}

/* ??0?$numpunct@D@std@@QAE@ABV_Locinfo@1@I_N@Z */
/* ??0?$numpunct@D@std@@QEAA@AEBV_Locinfo@1@_K_N@Z */
DEFINE_THISCALL_WRAPPER(numpunct_char_ctor_locinfo, 16)
numpunct_char* __thiscall numpunct_char_ctor_locinfo(numpunct_char *this,
        const _Locinfo *locinfo, size_t refs, bool usedef)
{
    TRACE("(%p %p %Iu %d)\n", this, locinfo, refs, usedef);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &numpunct_char_vtable;
    numpunct_char__Init(this, locinfo, usedef);
    return this;
}

/* ??0?$numpunct@D@std@@IAE@PBDI_N@Z */
/* ??0?$numpunct@D@std@@IEAA@PEBD_K_N@Z */
DEFINE_THISCALL_WRAPPER(numpunct_char_ctor_name, 16)
numpunct_char* __thiscall numpunct_char_ctor_name(numpunct_char *this,
        const char *name, size_t refs, bool usedef)
{
    _Locinfo locinfo;

    TRACE("(%p %s %Iu %d)\n", this, debugstr_a(name), refs, usedef);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &numpunct_char_vtable;

    _Locinfo_ctor_cstr(&locinfo, name);
    numpunct_char__Init(this, &locinfo, usedef);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$numpunct@D@std@@QAE@I@Z */
/* ??0?$numpunct@D@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(numpunct_char_ctor_refs, 8)
numpunct_char* __thiscall numpunct_char_ctor_refs(numpunct_char *this, size_t refs)
{
    TRACE("(%p %Iu)\n", this, refs);
    return numpunct_char_ctor_name(this, "C", refs, FALSE);
}

/* ??_F?$numpunct@D@std@@QAEXXZ */
/* ??_F?$numpunct@D@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_ctor, 4)
numpunct_char* __thiscall numpunct_char_ctor(numpunct_char *this)
{
    return numpunct_char_ctor_refs(this, 0);
}

/* ??1?$numpunct@D@std@@UAE@XZ */
/* ??1?$numpunct@D@std@@UEAA@XZ */
/* ??1?$numpunct@D@std@@MAE@XZ */
/* ??1?$numpunct@D@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_dtor, 4)
void __thiscall numpunct_char_dtor(numpunct_char *this)
{
    TRACE("(%p)\n", this);
    numpunct_char__Tidy(this);
}

DEFINE_THISCALL_WRAPPER(numpunct_char_vector_dtor, 8)
numpunct_char* __thiscall numpunct_char_vector_dtor(numpunct_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            numpunct_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        numpunct_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$numpunct@D@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$numpunct@D@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl numpunct_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = operator_new(sizeof(numpunct_char));
        numpunct_char_ctor_name((numpunct_char*)*facet,
                locale_string_char_c_str(&loc->ptr->name), 0, TRUE);
    }

    return LC_NUMERIC;
}

/* ?_Getcat@?$numpunct@D@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$numpunct@D@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl numpunct_char__Getcat_old(const locale_facet **facet)
{
    return numpunct_char__Getcat(facet, locale_classic());
}

static numpunct_char* numpunct_char_use_facet(const locale *loc)
{
    static numpunct_char *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&numpunct_char_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (numpunct_char*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    numpunct_char__Getcat(&fac, loc);
    obj = (numpunct_char*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?do_decimal_point@?$numpunct@D@std@@MBEDXZ */
/* ?do_decimal_point@?$numpunct@D@std@@MEBADXZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_do_decimal_point, 4)
#if _MSVCP_VER <= 100
#define call_numpunct_char_do_decimal_point(this) CALL_VTBL_FUNC(this, 4, \
        char, (const numpunct_char *this), (this))
#else
#define call_numpunct_char_do_decimal_point(this) CALL_VTBL_FUNC(this, 12, \
        char, (const numpunct_char *this), (this))
#endif
char __thiscall numpunct_char_do_decimal_point(const numpunct_char *this)
{
    TRACE("(%p)\n", this);
    return this->dp;
}

/* ?decimal_point@?$numpunct@D@std@@QBEDXZ */
/* ?decimal_point@?$numpunct@D@std@@QEBADXZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_decimal_point, 4)
char __thiscall numpunct_char_decimal_point(const numpunct_char *this)
{
    TRACE("(%p)\n", this);
    return call_numpunct_char_do_decimal_point(this);
}

/* ?do_thousands_sep@?$numpunct@D@std@@MBEDXZ */
/* ?do_thousands_sep@?$numpunct@D@std@@MEBADXZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_do_thousands_sep, 4)
#if _MSVCP_VER <= 100
#define call_numpunct_char_do_thousands_sep(this) CALL_VTBL_FUNC(this, 8, \
        char, (const numpunct_char*), (this))
#else
#define call_numpunct_char_do_thousands_sep(this) CALL_VTBL_FUNC(this, 16, \
        char, (const numpunct_char*), (this))
#endif
char __thiscall numpunct_char_do_thousands_sep(const numpunct_char *this)
{
    TRACE("(%p)\n", this);
    return this->sep;
}

/* ?thousands_sep@?$numpunct@D@std@@QBEDXZ */
/* ?thousands_sep@?$numpunct@D@std@@QEBADXZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_thousands_sep, 4)
char __thiscall numpunct_char_thousands_sep(const numpunct_char *this)
{
    TRACE("(%p)\n", this);
    return call_numpunct_char_do_thousands_sep(this);
}

/* ?do_grouping@?$numpunct@D@std@@MBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?do_grouping@?$numpunct@D@std@@MEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_do_grouping, 8)
#if _MSVCP_VER <= 100
#define call_numpunct_char_do_grouping(this, ret) CALL_VTBL_FUNC(this, 12, \
        basic_string_char*, (const numpunct_char*, basic_string_char*), (this, ret))
#else
#define call_numpunct_char_do_grouping(this, ret) CALL_VTBL_FUNC(this, 20, \
        basic_string_char*, (const numpunct_char*, basic_string_char*), (this, ret))
#endif
basic_string_char* __thiscall numpunct_char_do_grouping(
        const numpunct_char *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return MSVCP_basic_string_char_ctor_cstr(ret, this->grouping);
}

/* ?grouping@?$numpunct@D@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?grouping@?$numpunct@D@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_grouping, 8)
basic_string_char* __thiscall numpunct_char_grouping(const numpunct_char *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return call_numpunct_char_do_grouping(this, ret);
}

/* ?do_falsename@?$numpunct@D@std@@MBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?do_falsename@?$numpunct@D@std@@MEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_do_falsename, 8)
#if _MSVCP_VER <= 100
#define call_numpunct_char_do_falsename(this, ret) CALL_VTBL_FUNC(this, 16, \
        basic_string_char*, (const numpunct_char*, basic_string_char*), (this, ret))
#else
#define call_numpunct_char_do_falsename(this, ret) CALL_VTBL_FUNC(this, 24, \
        basic_string_char*, (const numpunct_char*, basic_string_char*), (this, ret))
#endif
basic_string_char* __thiscall numpunct_char_do_falsename(
        const numpunct_char *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return MSVCP_basic_string_char_ctor_cstr(ret, this->false_name);
}

/* ?falsename@?$numpunct@D@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?falsename@?$numpunct@D@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_falsename, 8)
basic_string_char* __thiscall numpunct_char_falsename(const numpunct_char *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return call_numpunct_char_do_falsename(this, ret);
}

/* ?do_truename@?$numpunct@D@std@@MBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?do_truename@?$numpunct@D@std@@MEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_do_truename, 8)
#if _MSVCP_VER <= 100
#define call_numpunct_char_do_truename(this, ret) CALL_VTBL_FUNC(this, 20, \
        basic_string_char*, (const numpunct_char*, basic_string_char*), (this, ret))
#else
#define call_numpunct_char_do_truename(this, ret) CALL_VTBL_FUNC(this, 28, \
        basic_string_char*, (const numpunct_char*, basic_string_char*), (this, ret))
#endif
basic_string_char* __thiscall numpunct_char_do_truename(
        const numpunct_char *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return MSVCP_basic_string_char_ctor_cstr(ret, this->true_name);
}

/* ?truename@?$numpunct@D@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?truename@?$numpunct@D@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_char_truename, 8)
basic_string_char* __thiscall numpunct_char_truename(const numpunct_char *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return call_numpunct_char_do_truename(this, ret);
}

/* ?id@?$numpunct@_W@std@@2V0locale@2@A */
locale_id numpunct_wchar_id = {0};
/* ?id@?$numpunct@G@std@@2V0locale@2@A */
locale_id numpunct_short_id = {0};

/* ??_7?$numpunct@_W@std@@6B@ */
extern const vtable_ptr numpunct_wchar_vtable;
/* ??_7?$numpunct@G@std@@6B@ */
extern const vtable_ptr numpunct_short_vtable;

/* ?_Init@?$numpunct@_W@std@@IAEXABV_Locinfo@2@_N@Z */
/* ?_Init@?$numpunct@_W@std@@IEAAXAEBV_Locinfo@2@_N@Z */
/* ?_Init@?$numpunct@G@std@@IAEXABV_Locinfo@2@_N@Z */
/* ?_Init@?$numpunct@G@std@@IEAAXAEBV_Locinfo@2@_N@Z */
DEFINE_THISCALL_WRAPPER(numpunct_wchar__Init, 12)
void __thiscall numpunct_wchar__Init(numpunct_wchar *this,
        const _Locinfo *locinfo, bool isdef)
{
    const char *to_convert;
    _Cvtvec cvt;
    int len;

    TRACE("(%p %p %d)\n", this, locinfo, isdef);

    _Locinfo__Getcvt(locinfo, &cvt);

    to_convert = _Locinfo__Getfalse(locinfo);
    len = MultiByteToWideChar(cvt.page, 0, to_convert, -1, NULL, 0);
    this->false_name = operator_new(len*sizeof(WCHAR));
    MultiByteToWideChar(cvt.page, 0, to_convert, -1,
            (wchar_t*)this->false_name, len);

    to_convert = _Locinfo__Gettrue(locinfo);
    len = MultiByteToWideChar(cvt.page, 0, to_convert, -1, NULL, 0);
    this->true_name = operator_new(len*sizeof(WCHAR));
    MultiByteToWideChar(cvt.page, 0, to_convert, -1,
            (wchar_t*)this->true_name, len);

    if(isdef) {
        this->grouping = operator_new(1);
        *(char*)this->grouping = 0;

        this->dp = '.';
        this->sep = ',';
    } else {
        const struct lconv *lc = _Locinfo__Getlconv(locinfo);

        len = strlen(lc->grouping)+1;
        this->grouping = operator_new(len);
        memcpy((char*)this->grouping, lc->grouping, len);

        this->dp = mb_to_wc(lc->decimal_point[0], &cvt);
        this->sep = mb_to_wc(lc->thousands_sep[0], &cvt);
    }
}

/* ?_Tidy@?$numpunct@_W@std@@AAEXXZ */
/* ?_Tidy@?$numpunct@_W@std@@AEAAXXZ */
/* ?_Tidy@?$numpunct@G@std@@AAEXXZ */
/* ?_Tidy@?$numpunct@G@std@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar__Tidy, 4)
void __thiscall numpunct_wchar__Tidy(numpunct_wchar *this)
{
    TRACE("(%p)\n", this);

    operator_delete((char*)this->grouping);
    operator_delete((wchar_t*)this->false_name);
    operator_delete((wchar_t*)this->true_name);
}

/* ??0?$numpunct@_W@std@@QAE@ABV_Locinfo@1@I_N@Z */
/* ??0?$numpunct@_W@std@@QEAA@AEBV_Locinfo@1@_K_N@Z */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_ctor_locinfo, 16)
numpunct_wchar* __thiscall numpunct_wchar_ctor_locinfo(numpunct_wchar *this,
        const _Locinfo *locinfo, size_t refs, bool usedef)
{
    TRACE("(%p %p %Iu %d)\n", this, locinfo, refs, usedef);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &numpunct_wchar_vtable;
    numpunct_wchar__Init(this, locinfo, usedef);
    return this;
}

/* ??0?$numpunct@G@std@@QAE@ABV_Locinfo@1@I_N@Z */
/* ??0?$numpunct@G@std@@QEAA@AEBV_Locinfo@1@_K_N@Z */
DEFINE_THISCALL_WRAPPER(numpunct_short_ctor_locinfo, 16)
numpunct_wchar* __thiscall numpunct_short_ctor_locinfo(numpunct_wchar *this,
        const _Locinfo *locinfo, size_t refs, bool usedef)
{
    numpunct_wchar_ctor_locinfo(this, locinfo, refs, usedef);
    this->facet.vtable = &numpunct_short_vtable;
    return this;
}

/* ??0?$numpunct@_W@std@@IAE@PBDI_N@Z */
/* ??0?$numpunct@_W@std@@IEAA@PEBD_K_N@Z */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_ctor_name, 16)
numpunct_wchar* __thiscall numpunct_wchar_ctor_name(numpunct_wchar *this,
        const char *name, size_t refs, bool usedef)
{
    _Locinfo locinfo;

    TRACE("(%p %s %Iu %d)\n", this, debugstr_a(name), refs, usedef);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &numpunct_wchar_vtable;

    _Locinfo_ctor_cstr(&locinfo, name);
    numpunct_wchar__Init(this, &locinfo, usedef);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$numpunct@G@std@@IAE@PBDI_N@Z */
/* ??0?$numpunct@G@std@@IEAA@PEBD_K_N@Z */
    DEFINE_THISCALL_WRAPPER(numpunct_short_ctor_name, 16)
numpunct_wchar* __thiscall numpunct_short_ctor_name(numpunct_wchar *this,
        const char *name, size_t refs, bool usedef)
{
    numpunct_wchar_ctor_name(this, name, refs, usedef);
    this->facet.vtable = &numpunct_short_vtable;
    return this;
}

/* ??0?$numpunct@_W@std@@QAE@I@Z */
/* ??0?$numpunct@_W@std@@QEAA@_K@Z */
    DEFINE_THISCALL_WRAPPER(numpunct_wchar_ctor_refs, 8)
numpunct_wchar* __thiscall numpunct_wchar_ctor_refs(numpunct_wchar *this, size_t refs)
{
    TRACE("(%p %Iu)\n", this, refs);
    return numpunct_wchar_ctor_name(this, "C", refs, FALSE);
}

/* ??0?$numpunct@G@std@@QAE@I@Z */
/* ??0?$numpunct@G@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(numpunct_short_ctor_refs, 8)
numpunct_wchar* __thiscall numpunct_short_ctor_refs(numpunct_wchar *this, size_t refs)
{
    numpunct_wchar_ctor_refs(this, refs);
    this->facet.vtable = &numpunct_short_vtable;
    return this;
}

/* ??_F?$numpunct@_W@std@@QAEXXZ */
/* ??_F?$numpunct@_W@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_ctor, 4)
numpunct_wchar* __thiscall numpunct_wchar_ctor(numpunct_wchar *this)
{
    return numpunct_wchar_ctor_refs(this, 0);
}

/* ??_F?$numpunct@G@std@@QAEXXZ */
/* ??_F?$numpunct@G@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(numpunct_short_ctor, 4)
numpunct_wchar* __thiscall numpunct_short_ctor(numpunct_wchar *this)
{
    return numpunct_short_ctor_refs(this, 0);
}

/* ??1?$numpunct@G@std@@UAE@XZ */
/* ??1?$numpunct@G@std@@UEAA@XZ */
/* ??1?$numpunct@_W@std@@MAE@XZ */
/* ??1?$numpunct@_W@std@@MEAA@XZ */
/* ??1?$numpunct@G@std@@MAE@XZ */
/* ??1?$numpunct@G@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_dtor, 4)
void __thiscall numpunct_wchar_dtor(numpunct_wchar *this)
{
    TRACE("(%p)\n", this);
    numpunct_wchar__Tidy(this);
}

DEFINE_THISCALL_WRAPPER(numpunct_wchar_vector_dtor, 8)
numpunct_wchar* __thiscall numpunct_wchar_vector_dtor(numpunct_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            numpunct_wchar_dtor(this+i);
        operator_delete(ptr);
    } else {
        numpunct_wchar_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$numpunct@_W@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$numpunct@_W@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl numpunct_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = operator_new(sizeof(numpunct_wchar));
        numpunct_wchar_ctor_name((numpunct_wchar*)*facet,
                locale_string_char_c_str(&loc->ptr->name), 0, TRUE);
    }

    return LC_NUMERIC;
}

/* ?_Getcat@?$numpunct@_W@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$numpunct@_W@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl numpunct_wchar__Getcat_old(const locale_facet **facet)
{
    return numpunct_wchar__Getcat(facet, locale_classic());
}

static numpunct_wchar* numpunct_wchar_use_facet(const locale *loc)
{
    static numpunct_wchar *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&numpunct_wchar_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (numpunct_wchar*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    numpunct_wchar__Getcat(&fac, loc);
    obj = (numpunct_wchar*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Getcat@?$numpunct@G@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$numpunct@G@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl numpunct_short__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = operator_new(sizeof(numpunct_wchar));
        numpunct_short_ctor_name((numpunct_wchar*)*facet,
                locale_string_char_c_str(&loc->ptr->name), 0, TRUE);
    }

    return LC_NUMERIC;
}

/* ?_Getcat@?$numpunct@G@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$numpunct@G@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl numpunct_short__Getcat_old(const locale_facet **facet)
{
    return numpunct_short__Getcat(facet, locale_classic());
}

static numpunct_wchar* numpunct_short_use_facet(const locale *loc)
{
    static numpunct_wchar *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&numpunct_short_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (numpunct_wchar*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    numpunct_short__Getcat(&fac, loc);
    obj = (numpunct_wchar*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?do_decimal_point@?$numpunct@_W@std@@MBE_WXZ */
/* ?do_decimal_point@?$numpunct@_W@std@@MEBA_WXZ */
/* ?do_decimal_point@?$numpunct@G@std@@MBEGXZ */
/* ?do_decimal_point@?$numpunct@G@std@@MEBAGXZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_do_decimal_point, 4)
#if _MSVCP_VER <= 100
#define call_numpunct_wchar_do_decimal_point(this) CALL_VTBL_FUNC(this, 4, \
        wchar_t, (const numpunct_wchar *this), (this))
#else
#define call_numpunct_wchar_do_decimal_point(this) CALL_VTBL_FUNC(this, 12, \
        wchar_t, (const numpunct_wchar *this), (this))
#endif
wchar_t __thiscall numpunct_wchar_do_decimal_point(const numpunct_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->dp;
}

/* ?decimal_point@?$numpunct@_W@std@@QBE_WXZ */
/* ?decimal_point@?$numpunct@_W@std@@QEBA_WXZ */
/* ?decimal_point@?$numpunct@G@std@@QBEGXZ */
/* ?decimal_point@?$numpunct@G@std@@QEBAGXZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_decimal_point, 4)
wchar_t __thiscall numpunct_wchar_decimal_point(const numpunct_wchar *this)
{
    TRACE("(%p)\n", this);
    return call_numpunct_wchar_do_decimal_point(this);
}

/* ?do_thousands_sep@?$numpunct@_W@std@@MBE_WXZ */
/* ?do_thousands_sep@?$numpunct@_W@std@@MEBA_WXZ */
/* ?do_thousands_sep@?$numpunct@G@std@@MBEGXZ */
/* ?do_thousands_sep@?$numpunct@G@std@@MEBAGXZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_do_thousands_sep, 4)
#if _MSVCP_VER <= 100
#define call_numpunct_wchar_do_thousands_sep(this) CALL_VTBL_FUNC(this, 8, \
        wchar_t, (const numpunct_wchar *this), (this))
#else
#define call_numpunct_wchar_do_thousands_sep(this) CALL_VTBL_FUNC(this, 16, \
        wchar_t, (const numpunct_wchar *this), (this))
#endif
wchar_t __thiscall numpunct_wchar_do_thousands_sep(const numpunct_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->sep;
}

/* ?thousands_sep@?$numpunct@_W@std@@QBE_WXZ */
/* ?thousands_sep@?$numpunct@_W@std@@QEBA_WXZ */
/* ?thousands_sep@?$numpunct@G@std@@QBEGXZ */
/* ?thousands_sep@?$numpunct@G@std@@QEBAGXZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_thousands_sep, 4)
wchar_t __thiscall numpunct_wchar_thousands_sep(const numpunct_wchar *this)
{
    TRACE("(%p)\n", this);
    return call_numpunct_wchar_do_thousands_sep(this);
}

/* ?do_grouping@?$numpunct@_W@std@@MBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?do_grouping@?$numpunct@_W@std@@MEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?do_grouping@?$numpunct@G@std@@MBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?do_grouping@?$numpunct@G@std@@MEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_do_grouping, 8)
#if _MSVCP_VER <= 100
#define call_numpunct_wchar_do_grouping(this, ret) CALL_VTBL_FUNC(this, 12, \
        basic_string_char*, (const numpunct_wchar*, basic_string_char*), (this, ret))
#else
#define call_numpunct_wchar_do_grouping(this, ret) CALL_VTBL_FUNC(this, 20, \
        basic_string_char*, (const numpunct_wchar*, basic_string_char*), (this, ret))
#endif
basic_string_char* __thiscall numpunct_wchar_do_grouping(const numpunct_wchar *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return MSVCP_basic_string_char_ctor_cstr(ret, this->grouping);
}

/* ?grouping@?$numpunct@_W@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?grouping@?$numpunct@_W@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?grouping@?$numpunct@G@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?grouping@?$numpunct@G@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_grouping, 8)
basic_string_char* __thiscall numpunct_wchar_grouping(const numpunct_wchar *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);
    return call_numpunct_wchar_do_grouping(this, ret);
}

/* ?do_falsename@?$numpunct@_W@std@@MBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?do_falsename@?$numpunct@_W@std@@MEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?do_falsename@?$numpunct@G@std@@MBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?do_falsename@?$numpunct@G@std@@MEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_do_falsename, 8)
#if _MSVCP_VER <= 100
#define call_numpunct_wchar_do_falsename(this, ret) CALL_VTBL_FUNC(this, 16, \
        basic_string_wchar*, (const numpunct_wchar*, basic_string_wchar*), (this, ret))
#else
#define call_numpunct_wchar_do_falsename(this, ret) CALL_VTBL_FUNC(this, 24, \
        basic_string_wchar*, (const numpunct_wchar*, basic_string_wchar*), (this, ret))
#endif
basic_string_wchar* __thiscall numpunct_wchar_do_falsename(const numpunct_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p)\n", this);
    return MSVCP_basic_string_wchar_ctor_cstr(ret, this->false_name);
}

/* ?falsename@?$numpunct@_W@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?falsename@?$numpunct@_W@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?falsename@?$numpunct@G@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?falsename@?$numpunct@G@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_falsename, 8)
basic_string_wchar* __thiscall numpunct_wchar_falsename(const numpunct_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p)\n", this);
    return call_numpunct_wchar_do_falsename(this, ret);
}

/* ?do_truename@?$numpunct@_W@std@@MBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?do_truename@?$numpunct@_W@std@@MEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?do_truename@?$numpunct@G@std@@MBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?do_truename@?$numpunct@G@std@@MEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_do_truename, 8)
#if _MSVCP_VER <= 100
#define call_numpunct_wchar_do_truename(this, ret) CALL_VTBL_FUNC(this, 20, \
        basic_string_wchar*, (const numpunct_wchar*, basic_string_wchar*), (this, ret))
#else
#define call_numpunct_wchar_do_truename(this, ret) CALL_VTBL_FUNC(this, 28, \
        basic_string_wchar*, (const numpunct_wchar*, basic_string_wchar*), (this, ret))
#endif
basic_string_wchar* __thiscall numpunct_wchar_do_truename(const numpunct_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p)\n", this);
    return MSVCP_basic_string_wchar_ctor_cstr(ret, this->true_name);
}

/* ?truename@?$numpunct@_W@std@@QBE?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?truename@?$numpunct@_W@std@@QEBA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?truename@?$numpunct@G@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?truename@?$numpunct@G@std@@QEBA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(numpunct_wchar_truename, 8)
basic_string_wchar* __thiscall numpunct_wchar_truename(const numpunct_wchar *this, basic_string_wchar *ret)
{
    TRACE("(%p)\n", this);
    return call_numpunct_wchar_do_truename(this, ret);
}

double __cdecl _Stod(const char *buf, char **buf_end, LONG exp)
{
    double ret = strtod(buf, buf_end);

    if(exp)
        ret *= pow(10, exp);
    return ret;
}

double __cdecl _Stodx(const char *buf, char **buf_end, LONG exp, int *err)
{
    double ret;

    *err = *_errno();
    *_errno() = 0;
    ret = _Stod(buf, buf_end, exp);
    if(*_errno()) {
        *err = *_errno();
    }else {
        *_errno() = *err;
        *err = 0;
    }
    return ret;
}

float __cdecl _Stof(const char *buf, char **buf_end, LONG exp)
{
    return _Stod(buf, buf_end, exp);
}

float __cdecl _Stofx(const char *buf, char **buf_end, LONG exp, int *err)
{
    return _Stodx(buf, buf_end, exp, err);
}

__int64 __cdecl _Stoll(const char *buf, char **buf_end, int base)
{
    return _strtoi64(buf, buf_end, base);
}

__int64 __cdecl _Stollx(const char *buf, char **buf_end, int base, int *err)
{
    __int64 ret;

    *err = *_errno();
    *_errno() = 0;
    ret = _strtoi64(buf, buf_end, base);
    if(*_errno()) {
        *err = *_errno();
    }else {
        *_errno() = *err;
        *err = 0;
    }
    return ret;
}

LONG __cdecl _Stolx(const char *buf, char **buf_end, int base, int *err)
{
    __int64 i = _Stollx(buf, buf_end, base, err);
    if(!*err && i!=(__int64)((LONG)i))
        *err = ERANGE;
    return i;
}

unsigned __int64 __cdecl _Stoull(const char *buf, char **buf_end, int base)
{
    return _strtoui64(buf, buf_end, base);
}

unsigned __int64 __cdecl _Stoullx(const char *buf, char **buf_end, int base, int *err)
{
    unsigned __int64 ret;

    *err = *_errno();
    *_errno() = 0;
    ret = _strtoui64(buf, buf_end, base);
    if(*_errno()) {
        *err = *_errno();
    }else {
        *_errno() = *err;
        *err = 0;
    }
    return ret;
}

ULONG __cdecl _Stoul(const char *buf, char **buf_end, int base)
{
    int err;
    unsigned __int64 i = _Stoullx(buf[0]=='-' ? buf+1 : buf, buf_end, base, &err);
    if(!err && i!=(unsigned __int64)((ULONG)i))
        *_errno() = ERANGE;
    return buf[0]=='-' ? -i : i;
}

ULONG __cdecl _Stoulx(const char *buf, char **buf_end, int base, int *err)
{
    unsigned __int64 i = _Stoullx(buf[0]=='-' ? buf+1 : buf, buf_end, base, err);
    if(!*err && i!=(unsigned __int64)((ULONG)i))
        *err = ERANGE;
    return buf[0]=='-' ? -i : i;
}

/* ?id@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@2V0locale@2@A */
locale_id num_get_wchar_id = {0};
/* ?id@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@2V0locale@2@A */
locale_id num_get_short_id = {0};

/* ??_7?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@6B@ */
extern const vtable_ptr num_get_wchar_vtable;
/* ??_7?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@6B@ */
extern const vtable_ptr num_get_short_vtable;

/* ?_Init@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
/* ?_Init@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar__Init, 8)
void __thiscall num_get_wchar__Init(num_get *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
#if _MSVCP_VER <= 100
    _Locinfo__Getcvt(locinfo, &this->cvt);
#endif
}

/* ??0?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_ctor_locinfo, 12)
num_get* __thiscall num_get_wchar_ctor_locinfo(num_get *this,
        const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &num_get_wchar_vtable;

    num_get_wchar__Init(this, locinfo);
    return this;
}

/* ??0?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_ctor_locinfo, 12)
num_get* __thiscall num_get_short_ctor_locinfo(num_get *this,
        const _Locinfo *locinfo, size_t refs)
{
    num_get_wchar_ctor_locinfo(this, locinfo, refs);
    this->facet.vtable = &num_get_short_vtable;
    return this;
}

/* ??0?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@I@Z */
/* ??0?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_ctor_refs, 8)
num_get* __thiscall num_get_wchar_ctor_refs(num_get *this, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %Iu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    num_get_wchar_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@I@Z */
/* ??0?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_ctor_refs, 8)
num_get* __thiscall num_get_short_ctor_refs(num_get *this, size_t refs)
{
    num_get_wchar_ctor_refs(this, refs);
    this->facet.vtable = &num_get_short_vtable;
    return this;
}

/* ??_F?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAEXXZ */
/* ??_F?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(num_get_wchar_ctor, 4)
num_get* __thiscall num_get_wchar_ctor(num_get *this)
{
    return num_get_wchar_ctor_refs(this, 0);
}

/* ??_F?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAEXXZ */
/* ??_F?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(num_get_short_ctor, 4)
num_get* __thiscall num_get_short_ctor(num_get *this)
{
    return num_get_short_ctor_refs(this, 0);
}

/* ??1?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@UAE@XZ */
/* ??1?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@UEAA@XZ */
/* ??1?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MAE@XZ */
/* ??1?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEAA@XZ */
/* ??1?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MAE@XZ */
/* ??1?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(num_get_wchar_dtor, 4)
void __thiscall num_get_wchar_dtor(num_get *this)
{
    TRACE("(%p)\n", this);
    locale_facet_dtor(&this->facet);
}

DEFINE_THISCALL_WRAPPER(num_get_wchar_vector_dtor, 8)
num_get* __thiscall num_get_wchar_vector_dtor(num_get *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            num_get_wchar_dtor(this+i);
        operator_delete(ptr);
    } else {
        num_get_wchar_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl num_get_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = operator_new(sizeof(num_get));
        _Locinfo_ctor_cstr(&locinfo, locale_string_char_c_str(&loc->ptr->name));
        num_get_wchar_ctor_locinfo((num_get*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_NUMERIC;
}

/* ?_Getcat@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl num_get_wchar__Getcat_old(const locale_facet **facet)
{
    return num_get_wchar__Getcat(facet, locale_classic());
}

num_get* num_get_wchar_use_facet(const locale *loc)
{
        static num_get *obj = NULL;

        _Lockit lock;
        const locale_facet *fac;

        _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
        fac = locale__Getfacet(loc, locale_id_operator_size_t(&num_get_wchar_id));
        if(fac) {
            _Lockit_dtor(&lock);
            return (num_get*)fac;
        }

        if(obj) {
            _Lockit_dtor(&lock);
            return obj;
        }

        num_get_wchar__Getcat(&fac, loc);
        obj = (num_get*)fac;
        call_locale_facet__Incref(&obj->facet);
        locale_facet_register(&obj->facet);
        _Lockit_dtor(&lock);

        return obj;
}

/* ?_Getcat@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl num_get_short__Getcat(const locale_facet **facet, const locale *loc)
{
    if(facet && !*facet) {
        num_get_wchar__Getcat(facet, loc);
        (*(locale_facet**)facet)->vtable = &num_get_short_vtable;
    }

    return LC_NUMERIC;
}

/* ?_Getcat@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl num_get_short__Getcat_old(const locale_facet **facet)
{
    return num_get_short__Getcat(facet, locale_classic());
}

num_get* num_get_short_use_facet(const locale *loc)
{
    static num_get *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&num_get_short_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (num_get*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    num_get_short__Getcat(&fac, loc);
    obj = (num_get*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

static int num_get__Getffld(const num_get *this, char *dest, istreambuf_iterator_wchar *first,
        istreambuf_iterator_wchar *last, const locale *loc, numpunct_wchar *numpunct)
{
    basic_string_char grouping_bstr;
    basic_string_char groups_found;
    int i, groups_no = 0, cur_group = 0, exp = 0;
    char *dest_beg = dest, *num_end = dest+25, *exp_end = dest+31;
    wchar_t sep = 0, digits[11], *digits_pos;
    const char *grouping, *groups;
    BOOL error = FALSE, got_digit = FALSE, got_nonzero = FALSE;
    const _Cvtvec *cvt;

    TRACE("(%p %p %p %p)\n", dest, first, last, loc);

#if _MSVCP_VER <= 100
    cvt = &this->cvt;
#else
    cvt = &ctype_wchar_use_facet(loc)->cvt;
#endif

    for(i=0; i<10; i++)
        digits[i] = mb_to_wc('0'+i, cvt);
    digits[10] = 0;

    numpunct_wchar_grouping(numpunct, &grouping_bstr);
    grouping = MSVCP_basic_string_char_c_str(&grouping_bstr);
#if _MSVCP_VER >= 70
    if (grouping[0]) sep = numpunct_wchar_thousands_sep(numpunct);
#endif

    if(sep)
        MSVCP_basic_string_char_ctor(&groups_found);

    istreambuf_iterator_wchar_val(first);
    /* get sign */
    if(first->strbuf && first->val==mb_to_wc('-', cvt)) {
        *dest++ = '-';
        istreambuf_iterator_wchar_inc(first);
    }else if(first->strbuf && first->val==mb_to_wc('+', cvt)) {
        *dest++ = '+';
        istreambuf_iterator_wchar_inc(first);
    }

    /* read possibly grouped numbers before decimal */
    for(; first->strbuf; istreambuf_iterator_wchar_inc(first)) {
        if(!(digits_pos = wcschr(digits, first->val))) {
            if(sep && first->val==sep) {
                if(!groups_no) break; /* empty group - stop parsing */
                MSVCP_basic_string_char_append_ch(&groups_found, groups_no);
                groups_no = 0;
                ++cur_group;
            }else {
                break;
            }
        }else {
            got_digit = TRUE; /* found a digit, zero or non-zero */
            /* write digit to dest if not a leading zero (to not waste dest buffer) */
            if(!got_nonzero && first->val == digits[0])
            {
                ++groups_no;
                continue;
            }
            got_nonzero = TRUE;
            if(dest < num_end)
                *dest++ = '0'+digits_pos-digits;
            else
                exp++; /* too many digits, just multiply by 10 */
            if(sep && groups_no<CHAR_MAX)
                ++groups_no;
        }
    }

    /* if all leading zeroes, we didn't write anything so put a zero we check for a decimal */
    if(got_digit && !got_nonzero)
        *dest++ = '0';

    /* get decimal, if any */
    if(first->strbuf && first->val==numpunct_wchar_decimal_point(numpunct)) {
        if(dest < num_end)
            *dest++ = *localeconv()->decimal_point;
        istreambuf_iterator_wchar_inc(first);
    }

    /* read non-grouped after decimal */
    for(; first->strbuf; istreambuf_iterator_wchar_inc(first)) {
        if(!(digits_pos = wcschr(digits, first->val)))
            break;
        else if(dest<num_end) {
            got_digit = TRUE;
            *dest++ = '0'+digits_pos-digits;
        }
    }

    /* read exponent, if any */
    if(first->strbuf && (first->val==mb_to_wc('e', cvt) || first->val==mb_to_wc('E', cvt))) {
        *dest++ = 'e';
        istreambuf_iterator_wchar_inc(first);

        if(first->strbuf && first->val==mb_to_wc('-', cvt)) {
            *dest++ = '-';
            istreambuf_iterator_wchar_inc(first);
        }else if(first->strbuf && first->val==mb_to_wc('+', cvt)) {
            *dest++ = '+';
            istreambuf_iterator_wchar_inc(first);
        }

        got_digit = got_nonzero = FALSE;
        error = TRUE;
        /* skip any leading zeroes */
        for(; first->strbuf && first->val==digits[0]; istreambuf_iterator_wchar_inc(first))
            error = FALSE;

        for(; first->strbuf && (digits_pos = wcschr(digits, first->val)); istreambuf_iterator_wchar_inc(first)) {
            got_digit = got_nonzero = TRUE; /* leading zeroes would have been skipped, so first digit is non-zero */
            error = FALSE;
            if(dest<exp_end)
                *dest++ = '0'+digits_pos-digits;
        }

        /* if just found zeroes for exponent, use that */
        if(got_digit && !got_nonzero)
        {
            error = FALSE;
            if(dest<exp_end)
                *dest++ = '0';
        }
    }

    if(sep && groups_no)
        MSVCP_basic_string_char_append_ch(&groups_found, groups_no);

    if(!cur_group) /* no groups, skip loop */
        cur_group--;
    else if(!(groups = MSVCP_basic_string_char_c_str(&groups_found))[cur_group])
        error = TRUE; /* trailing empty */

    for(; cur_group>=0 && !error; cur_group--) {
        if(*grouping == CHAR_MAX) {
            if(cur_group)
                error = TRUE;
            break;
        }else if((cur_group && *grouping!=groups[cur_group])
                || (!cur_group && *grouping<groups[cur_group])) {
            error = TRUE;
            break;
        }else if(grouping[1]) {
            grouping++;
        }
    }
    MSVCP_basic_string_char_dtor(&grouping_bstr);
    if(sep)
        MSVCP_basic_string_char_dtor(&groups_found);

    if(error) {
        *dest_beg = '\0';
        return 0;
    }
    *dest++ = '\0';
    return exp;
}

/* ?_Getffld@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABAHPADAAV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@1ABVlocale@2@@Z */
/* ?_Getffld@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBAHPEADAEAV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@1AEBVlocale@2@@Z */
int __cdecl num_get_wchar__Getffld(const num_get *this, char *dest, istreambuf_iterator_wchar *first,
    istreambuf_iterator_wchar *last, const locale *loc)
{
    return num_get__Getffld(this, dest, first, last, loc, numpunct_wchar_use_facet(loc));
}

/* ?_Getffld@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABAHPADAAV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@1ABVlocale@2@@Z */
/* ?_Getffld@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBAHPEADAEAV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@1AEBVlocale@2@@Z */
int __cdecl num_get_short__Getffld(const num_get *this, char *dest, istreambuf_iterator_wchar *first,
    istreambuf_iterator_wchar *last, const locale *loc)
{
    return num_get__Getffld(this, dest, first, last, loc, numpunct_short_use_facet(loc));
}

/* ?_Getffldx@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABAHPADAAV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@1AAVios_base@2@PAH@Z */
/* ?_Getffldx@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBAHPEADAEAV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@1AEAVios_base@2@PEAH@Z */
/* ?_Getffldx@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABAHPADAAV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@1AAVios_base@2@PAH@Z */
/* ?_Getffldx@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBAHPEADAEAV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@1AEAVios_base@2@PEAH@Z */
int __cdecl num_get_wchar__Getffldx(num_get *this, char *dest, istreambuf_iterator_wchar *first,
    istreambuf_iterator_wchar *last, ios_base *ios, int *phexexp)
{
    FIXME("(%p %p %p %p %p) stub\n", dest, first, last, ios, phexexp);
    return -1;
}

static int num_get__Getifld(const num_get *this, char *dest, istreambuf_iterator_wchar *first,
    istreambuf_iterator_wchar *last, int fmtflags, const locale *loc, numpunct_wchar *numpunct)
{
    wchar_t digits[23], *digits_pos, sep = 0;
    basic_string_char grouping_bstr;
    basic_string_char groups_found;
    int i, basefield, base, groups_no = 0, cur_group = 0;
    char *dest_beg = dest, *dest_end = dest+24;
    const char *grouping, *groups;
    BOOL error = TRUE, dest_empty = TRUE, found_zero = FALSE;
    const _Cvtvec *cvt;

    TRACE("(%p %p %p %04x %p)\n", dest, first, last, fmtflags, loc);

#if _MSVCP_VER <= 100
    cvt = &this->cvt;
#else
    cvt = &ctype_wchar_use_facet(loc)->cvt;
#endif

    for(i=0; i<10; i++)
        digits[i] = mb_to_wc('0'+i, cvt);
    for(i=0; i<6; i++) {
        digits[10+i] = mb_to_wc('a'+i, cvt);
        digits[16+i] = mb_to_wc('A'+i, cvt);
    }

    numpunct_wchar_grouping(numpunct, &grouping_bstr);
    grouping = MSVCP_basic_string_char_c_str(&grouping_bstr);
#if _MSVCP_VER >= 70
    if (grouping[0]) sep = numpunct_wchar_thousands_sep(numpunct);
#endif

    basefield = fmtflags & FMTFLAG_basefield;
    if(basefield == FMTFLAG_oct)
        base = 8;
    else if(basefield == FMTFLAG_hex)
        base = 22; /* equal to the size of digits buffer */
    else if(!basefield)
        base = 0;
    else
        base = 10;

    istreambuf_iterator_wchar_val(first);
    if(first->strbuf && first->val==mb_to_wc('-', cvt)) {
        *dest++ = '-';
        istreambuf_iterator_wchar_inc(first);
    }else if(first->strbuf && first->val==mb_to_wc('+', cvt)) {
        *dest++ = '+';
        istreambuf_iterator_wchar_inc(first);
    }

    if(first->strbuf && first->val==digits[0]) {
        found_zero = TRUE;
        istreambuf_iterator_wchar_inc(first);
        if(first->strbuf && (first->val==mb_to_wc('x', cvt) || first->val==mb_to_wc('X', cvt))) {
            if(!base || base == 22) {
                found_zero = FALSE;
                istreambuf_iterator_wchar_inc(first);
                base = 22;
            }else {
                base = 10;
            }
        }else {
            error = FALSE;
            if(!base) base = 8;
        }
    }else {
        if(!base) base = 10;
    }
    digits[base] = 0;

    if(sep) {
        MSVCP_basic_string_char_ctor(&groups_found);
        if(found_zero) ++groups_no;
    }

    for(; first->strbuf; istreambuf_iterator_wchar_inc(first)) {
        if(!(digits_pos = wcschr(digits, first->val))) {
            if(sep && first->val==sep) {
                if(!groups_no) break; /* empty group - stop parsing */
                MSVCP_basic_string_char_append_ch(&groups_found, groups_no);
                groups_no = 0;
                ++cur_group;
            }else {
                break;
            }
        }else {
            error = FALSE;
            if(dest_empty && first->val == digits[0]) {
                found_zero = TRUE;
                ++groups_no;
                continue;
            }
            dest_empty = FALSE;
            /* skip digits that can't be copied to dest buffer, other
             * functions are responsible for detecting overflows */
            if(dest < dest_end)
                *dest++ = (digits_pos-digits<10 ? '0'+digits_pos-digits :
                        (digits_pos-digits<16 ? 'a'+digits_pos-digits-10 :
                         'A'+digits_pos-digits-16));
            if(sep && groups_no<CHAR_MAX)
                ++groups_no;
        }
    }

    if(sep && groups_no)
        MSVCP_basic_string_char_append_ch(&groups_found, groups_no);

    if(!cur_group) { /* no groups, skip loop */
        cur_group--;
    }else if(!(groups = MSVCP_basic_string_char_c_str(&groups_found))[cur_group]) {
        error = TRUE; /* trailing empty */
        found_zero = FALSE;
    }

    for(; cur_group>=0 && !error; cur_group--) {
        if(*grouping == CHAR_MAX) {
            if(cur_group)
                error = TRUE;
            break;
        }else if((cur_group && *grouping!=groups[cur_group])
                || (!cur_group && *grouping<groups[cur_group])) {
            error = TRUE;
            break;
        }else if(grouping[1]) {
            grouping++;
        }
    }

    MSVCP_basic_string_char_dtor(&grouping_bstr);
    if(sep)
        MSVCP_basic_string_char_dtor(&groups_found);

    if(error) {
        if (found_zero)
            *dest++ = '0';
        else
            dest = dest_beg;
    }else if(dest_empty)
        *dest++ = '0';
    *dest = '\0';

    return (base==22 ? 16 : base);
}

/* ?_Getifld@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABAHPADAAV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@1HABVlocale@2@@Z */
/* ?_Getifld@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBAHPEADAEAV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@1HAEBVlocale@2@@Z */
int __cdecl num_get_wchar__Getifld(const num_get *this, char *dest, istreambuf_iterator_wchar *first,
    istreambuf_iterator_wchar *last, int fmtflags, const locale *loc)
{
    return num_get__Getifld(this, dest, first, last,
            fmtflags, loc, numpunct_wchar_use_facet(loc));
}

/* ?_Getifld@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABAHPADAAV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@1HABVlocale@2@@Z */
/* ?_Getifld@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBAHPEADAEAV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@1HAEBVlocale@2@@Z */
int __cdecl num_get_short__Getifld(const num_get *this, char *dest, istreambuf_iterator_wchar *first,
    istreambuf_iterator_wchar *last, int fmtflags, const locale *loc)
{
    return num_get__Getifld(this, dest, first, last,
            fmtflags, loc, numpunct_short_use_facet(loc));
}

/* ?_Hexdig@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABEH_W000@Z */
/* ?_Hexdig@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBAH_W000@Z */
/* ?_Hexdig@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABEHGGGG@Z */
/* ?_Hexdig@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBAHGGGG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_num_get_wchar__Hexdig, 20)
int __thiscall MSVCP_num_get_wchar__Hexdig(num_get *this, wchar_t dig, wchar_t e0, wchar_t al, wchar_t au)
{
    FIXME("(%p %c %c %c %c) stub\n", this, dig, e0, al, au);
    return -1;
}

static istreambuf_iterator_wchar* num_get_do_get_void(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        void **pval, numpunct_wchar *numpunct)
{
    unsigned __int64 v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stoullx(tmp, &end, num_get__Getifld(this, tmp, &first,
                &last, FMTFLAG_hex, IOS_LOCALE(base), numpunct), &err);
    if(v!=(unsigned __int64)((INT_PTR)v))
        *state |= IOSTATE_failbit;
    else if(end!=tmp && !err)
        *pval = (void*)((INT_PTR)v);
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAPAX@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAPEAX@Z */
#if _MSVCP_VER <= 100
#define call_num_get_wchar_do_get_void(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 4, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, void**), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_wchar_do_get_void(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 12, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, void**), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_void,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_void(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, void **pval)
{
    return num_get_do_get_void(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(IOS_LOCALE(base)));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAPAX@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAPEAX@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_void,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_void(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, void **pval)
{
    return num_get_do_get_void(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(IOS_LOCALE(base)));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAPAX@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAPEAX@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAPAX@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAPEAX@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_void,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_void(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, void **pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_void(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_double(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        double *pval, numpunct_wchar *numpunct)
{
    double v;
    char tmp[32], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stodx(tmp, &end, num_get__Getffld(this, tmp, &first, &last, IOS_LOCALE(base), numpunct), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAO@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAO@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAN@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAN@Z */
#if _MSVCP_VER <= 100
#define call_num_get_wchar_do_get_ldouble(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 8, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, double*), \
        (this, ret, first, last, base, state, pval))
#define call_num_get_wchar_do_get_double(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 12, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, double*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_wchar_do_get_ldouble(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 16, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, double*), \
        (this, ret, first, last, base, state, pval))
#define call_num_get_wchar_do_get_double(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 20, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, double*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_double,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_double(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, double *pval)
{
    return num_get_do_get_double(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(IOS_LOCALE(base)));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAO@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAO@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAN@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAN@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_double,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_double(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, double *pval)
{
    return num_get_do_get_double(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(IOS_LOCALE(base)));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAO@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAO@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAO@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAO@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_ldouble,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_ldouble(const num_get *this, istreambuf_iterator_wchar *ret,
        istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, double *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_ldouble(this, ret, first, last, base, state, pval);
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAN@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAN@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAN@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAN@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_double,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_double(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, double *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_double(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_float(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        float *pval, numpunct_wchar *numpunct)
{
    float v;
    char tmp[32], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stofx(tmp, &end, num_get__Getffld(this, tmp, &first,
                &last, IOS_LOCALE(base), numpunct), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAM@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAM@Z */
#if _MSVCP_VER <= 100
#define call_num_get_wchar_do_get_float(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 16, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, float*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_wchar_do_get_float(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 24, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, float*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_float,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_float(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, float *pval)
{
    return num_get_do_get_float(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(IOS_LOCALE(base)));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAM@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAM@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_float,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_float(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, float *pval)
{
    return num_get_do_get_float(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(IOS_LOCALE(base)));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAM@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAM@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAM@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAM@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_float,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_float(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, float *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_float(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_uint64(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        unsigned __int64 *pval, numpunct_wchar *numpunct)
{
    unsigned __int64 v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stoullx(tmp, &end, num_get__Getifld(this, tmp, &first,
                &last, base->fmtfl, IOS_LOCALE(base), numpunct), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAA_K@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEA_K@Z */
#if _MSVCP_VER <= 100
#define call_num_get_wchar_do_get_uint64(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 20, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, unsigned __int64*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_wchar_do_get_uint64(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 28, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, unsigned __int64*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_uint64,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_uint64(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned __int64 *pval)
{
    return num_get_do_get_uint64(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(IOS_LOCALE(base)));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAA_K@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEA_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_uint64,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_uint64(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned __int64 *pval)
{
    return num_get_do_get_uint64(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(IOS_LOCALE(base)));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAA_K@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEA_K@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAA_K@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEA_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_uint64,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_uint64(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned __int64 *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_uint64(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_int64(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        __int64 *pval, numpunct_wchar *numpunct)
{
    __int64 v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stollx(tmp, &end, num_get__Getifld(this, tmp, &first,
                &last, base->fmtfl, IOS_LOCALE(base), numpunct), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAA_J@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEA_J@Z */
#if _MSVCP_VER <= 100
#define call_num_get_wchar_do_get_int64(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 24, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, __int64*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_wchar_do_get_int64(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 32, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, __int64*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_int64,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_int64(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, __int64 *pval)
{
    return num_get_do_get_int64(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(IOS_LOCALE(base)));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAA_J@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEA_J@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_int64,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_int64(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, __int64 *pval)
{
    return num_get_do_get_int64(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(IOS_LOCALE(base)));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAA_J@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEA_J@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAA_J@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEA_J@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_int64,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_int64(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, __int64 *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_int64(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_ulong(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        ULONG *pval, numpunct_wchar *numpunct)
{
    ULONG v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stoulx(tmp, &end, num_get__Getifld(this, tmp, &first,
                &last, base->fmtfl, IOS_LOCALE(base), numpunct), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAK@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAK@Z */
#if _MSVCP_VER <= 100
#define call_num_get_wchar_do_get_ulong(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 28, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, ULONG*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_wchar_do_get_ulong(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 36, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, ULONG*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_ulong,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_ulong(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, ULONG *pval)
{
    return num_get_do_get_ulong(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(IOS_LOCALE(base)));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAK@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAK@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_ulong,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_ulong(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, ULONG *pval)
{
    return num_get_do_get_ulong(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(IOS_LOCALE(base)));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAK@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAK@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAK@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAK@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_ulong,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_ulong(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, ULONG *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_ulong(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_long(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        LONG *pval, numpunct_wchar *numpunct)
{
    LONG v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stolx(tmp, &end, num_get__Getifld(this, tmp, &first,
                &last, base->fmtfl, IOS_LOCALE(base), numpunct), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAJ@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAJ@Z */
#if _MSVCP_VER <= 100
#define call_num_get_wchar_do_get_long(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 32, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, LONG*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_wchar_do_get_long(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 40, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, LONG*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_long,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_long(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, LONG *pval)
{
    return num_get_do_get_long(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(IOS_LOCALE(base)));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAJ@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAJ@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_long,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_long(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, LONG *pval)
{
    return num_get_do_get_long(this, ret, first, last, base,
        state, pval, numpunct_short_use_facet(IOS_LOCALE(base)));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAJ@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAJ@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAJ@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAJ@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_long,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_long(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, LONG *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_long(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAI@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAI@Z */
#if _MSVCP_VER <= 100
#define call_num_get_wchar_do_get_uint(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 36, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, unsigned int*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_wchar_do_get_uint(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 44, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, unsigned int*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_uint,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_uint(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned int *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return num_get_wchar_do_get_ulong(this, ret, first, last, base, state, (ULONG*)pval);
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAI@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAI@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_uint,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_uint(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned int *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return num_get_short_do_get_ulong(this, ret, first, last, base, state, (ULONG*)pval);
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAI@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAI@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAI@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAI@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_uint,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_uint(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned int *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_uint(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAG@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAG@Z */
#if _MSVCP_VER <= 100
#define call_num_get_wchar_do_get_ushort(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 40, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, unsigned short*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_wchar_do_get_ushort(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 48, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, unsigned short*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_ushort,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_ushort(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned short *pval)
{
    ULONG v;
    char tmp[25], *beg, *end;
    int err, b;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    b = num_get_wchar__Getifld(this, tmp,
            &first, &last, base->fmtfl, IOS_LOCALE(base));
    beg = tmp + (tmp[0]=='-' ? 1 : 0);
    v = _Stoulx(beg, &end, b, &err);

    if(v != (ULONG)((unsigned short)v))
        *state |= IOSTATE_failbit;
    else if(end!=beg && !err)
        *pval = (tmp[0]=='-' ? -((unsigned short)v) : v);
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAG@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAG@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_ushort,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_ushort(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned short *pval)
{
    FIXME("(%p %p %p %p %p) stub\n", this, ret, base, state, pval);
    return ret;
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAAG@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEAG@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAAG@ */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEAG@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_ushort,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_ushort(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, unsigned short *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_ushort(this, ret, first, last, base, state, pval);
}

static istreambuf_iterator_wchar* num_get_do_get_bool(const num_get *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar first,
        istreambuf_iterator_wchar last, ios_base *base, int *state,
        bool *pval, numpunct_wchar *numpunct)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    if(base->fmtfl & FMTFLAG_boolalpha) {
        basic_string_wchar false_bstr, true_bstr;
        const wchar_t *pfalse, *ptrue;

        numpunct_wchar_falsename(numpunct, &false_bstr);
        numpunct_wchar_truename(numpunct, &true_bstr);
        pfalse = MSVCP_basic_string_wchar_c_str(&false_bstr);
        ptrue = MSVCP_basic_string_wchar_c_str(&true_bstr);

        for(istreambuf_iterator_wchar_val(&first); first.strbuf;) {
            if(pfalse && *pfalse && first.val!=*pfalse)
                pfalse = NULL;
            if(ptrue && *ptrue && first.val!=*ptrue)
                ptrue = NULL;

            if(pfalse && *pfalse && ptrue && !*ptrue)
                ptrue = NULL;
            if(ptrue && *ptrue && pfalse && !*pfalse)
                pfalse = NULL;

            if(pfalse)
                pfalse++;
            if(ptrue)
                ptrue++;

            if(pfalse || ptrue)
                istreambuf_iterator_wchar_inc(&first);

            if((!pfalse || !*pfalse) && (!ptrue || !*ptrue))
                break;
        }

        if(ptrue)
            *pval = TRUE;
        else if(pfalse)
            *pval = FALSE;
        else
            *state |= IOSTATE_failbit;

        MSVCP_basic_string_wchar_dtor(&false_bstr);
        MSVCP_basic_string_wchar_dtor(&true_bstr);
    }else {
        char tmp[25], *end;
        int err;
        LONG v = _Stolx(tmp, &end, num_get__Getifld(this, tmp, &first,
                    &last, base->fmtfl, IOS_LOCALE(base), numpunct), &err);

        if(end!=tmp && err==0 && (v==0 || v==1))
            *pval = v;
        else
            *state |= IOSTATE_failbit;
    }

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;
    memcpy(ret, &first, sizeof(first));
    return ret;
}

/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAA_N@Z */
/* ?do_get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEA_N@Z */
#if _MSVCP_VER <= 100
#define call_num_get_wchar_do_get_bool(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 44, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, bool*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_wchar_do_get_bool(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 52, istreambuf_iterator_wchar*, \
        (const num_get*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, bool*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_wchar_do_get_bool,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_do_get_bool(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, bool *pval)
{
    return num_get_do_get_bool(this, ret, first, last, base,
            state, pval, numpunct_wchar_use_facet(IOS_LOCALE(base)));
}

/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAA_N@Z */
/* ?do_get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEA_N@Z */
DEFINE_THISCALL_WRAPPER(num_get_short_do_get_bool,36)
istreambuf_iterator_wchar *__thiscall num_get_short_do_get_bool(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, bool *pval)
{
    return num_get_do_get_bool(this, ret, first, last, base,
            state, pval, numpunct_short_use_facet(IOS_LOCALE(base)));
}

/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHAA_N@Z */
/* ?get@?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHAEA_N@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AAVios_base@2@AAHAA_N@Z */
/* ?get@?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@GU?$char_traits@G@std@@@2@V32@0AEAVios_base@2@AEAHAEA_N@Z */
DEFINE_THISCALL_WRAPPER(num_get_wchar_get_bool,36)
istreambuf_iterator_wchar *__thiscall num_get_wchar_get_bool(const num_get *this, istreambuf_iterator_wchar *ret,
    istreambuf_iterator_wchar first, istreambuf_iterator_wchar last, ios_base *base, int *state, bool *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_wchar_do_get_bool(this, ret, first, last, base, state, pval);
}

/* ?id@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@2V0locale@2@A */
locale_id num_get_char_id = {0};

/* ??_7?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@6B@ */
extern const vtable_ptr num_get_char_vtable;

/* ?_Init@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(num_get_char__Init, 8)
void __thiscall num_get_char__Init(num_get *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
#if _MSVCP_VER <= 100
    _Locinfo__Getcvt(locinfo, &this->cvt);
#endif
}

/* ??0?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_ctor_locinfo, 12)
num_get* __thiscall num_get_char_ctor_locinfo(num_get *this,
        const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &num_get_char_vtable;

    num_get_char__Init(this, locinfo);
    return this;
}

/* ??0?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@I@Z */
/* ??0?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_ctor_refs, 8)
num_get* __thiscall num_get_char_ctor_refs(num_get *this, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %Iu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    num_get_char_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??_F?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAEXXZ */
/* ??_F?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(num_get_char_ctor, 4)
num_get* __thiscall num_get_char_ctor(num_get *this)
{
    return num_get_char_ctor_refs(this, 0);
}

/* ??1?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@UAE@XZ */
/* ??1?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@UEAA@XZ */
/* ??1?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MAE@XZ */
/* ??1?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(num_get_char_dtor, 4)
void __thiscall num_get_char_dtor(num_get *this)
{
    TRACE("(%p)\n", this);
    locale_facet_dtor(&this->facet);
}

DEFINE_THISCALL_WRAPPER(num_get_char_vector_dtor, 8)
num_get* __thiscall num_get_char_vector_dtor(num_get *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            num_get_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        num_get_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl num_get_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = operator_new(sizeof(num_get));
        _Locinfo_ctor_cstr(&locinfo, locale_string_char_c_str(&loc->ptr->name));
        num_get_char_ctor_locinfo((num_get*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_NUMERIC;
}

/* ?_Getcat@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl num_get_char__Getcat_old(const locale_facet **facet)
{
    return num_get_char__Getcat(facet, locale_classic());
}

num_get* num_get_char_use_facet(const locale *loc)
{
    static num_get *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&num_get_char_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (num_get*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    num_get_char__Getcat(&fac, loc);
    obj = (num_get*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Getffld@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABAHPADAAV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@1ABVlocale@2@@Z */
/* ?_Getffld@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBAHPEADAEAV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@1AEBVlocale@2@@Z */
/* Copies number to dest buffer, validates grouping and skips separators.
 * Updates first so it points past the number, all digits are skipped.
 * Returns how exponent needs to changed.
 * Size of dest buffer is not specified, assuming it's not smaller than 32:
 * strlen(+0.e+) + 22(digits) + 4(exponent) + 1(nullbyte)
 */
int __cdecl num_get_char__Getffld(const num_get *this, char *dest, istreambuf_iterator_char *first,
        istreambuf_iterator_char *last, const locale *loc)
{
    numpunct_char *numpunct = numpunct_char_use_facet(loc);
    basic_string_char grouping_bstr;
    basic_string_char groups_found;
    int groups_no = 0, cur_group = 0, exp = 0;
    char *dest_beg = dest, *num_end = dest+25, *exp_end = dest+31, sep = 0;
    const char *grouping, *groups;
    BOOL error = FALSE, got_digit = FALSE, got_nonzero = FALSE;

    TRACE("(%p %p %p %p)\n", dest, first, last, loc);

    numpunct_char_grouping(numpunct, &grouping_bstr);
    grouping = MSVCP_basic_string_char_c_str(&grouping_bstr);
#if _MSVCP_VER >= 70
    if (grouping[0]) sep = numpunct_char_thousands_sep(numpunct);
#endif

    if(sep)
        MSVCP_basic_string_char_ctor(&groups_found);

    istreambuf_iterator_char_val(first);
    /* get sign */
    if(first->strbuf && (first->val=='-' || first->val=='+')) {
        *dest++ = first->val;
        istreambuf_iterator_char_inc(first);
    }

    /* read possibly grouped numbers before decimal */
    for(; first->strbuf; istreambuf_iterator_char_inc(first)) {
        if(first->val<'0' || first->val>'9') {
            if(sep && first->val==sep) {
                if(!groups_no) break; /* empty group - stop parsing */
                MSVCP_basic_string_char_append_ch(&groups_found, groups_no);
                groups_no = 0;
                ++cur_group;
            }else {
                break;
            }
        }else {
            got_digit = TRUE; /* found a digit, zero or non-zero */
            /* write digit to dest if not a leading zero (to not waste dest buffer) */
            if(!got_nonzero && first->val == '0')
            {
                ++groups_no;
                continue;
            }
            got_nonzero = TRUE;
            if(dest < num_end)
                *dest++ = first->val;
            else
                exp++; /* too many digits, just multiply by 10 */
            if(sep && groups_no<CHAR_MAX)
                ++groups_no;
        }
    }

    /* if all leading zeroes, we didn't write anything so put a zero we check for a decimal */
    if(got_digit && !got_nonzero)
        *dest++ = '0';

    /* get decimal, if any */
    if(first->strbuf && first->val==numpunct_char_decimal_point(numpunct)) {
        if(dest < num_end)
            *dest++ = *localeconv()->decimal_point;
        istreambuf_iterator_char_inc(first);
    }

    /* read non-grouped after decimal */
    for(; first->strbuf; istreambuf_iterator_char_inc(first)) {
        if(first->val<'0' || first->val>'9')
            break;
        else if(dest<num_end) {
            got_digit = TRUE;
            *dest++ = first->val;
        }
    }

    /* read exponent, if any */
    if(first->strbuf && (first->val=='e' || first->val=='E')) {
        *dest++ = first->val;
        istreambuf_iterator_char_inc(first);

        if(first->strbuf && (first->val=='-' || first->val=='+')) {
            *dest++ = first->val;
            istreambuf_iterator_char_inc(first);
        }

        got_digit = got_nonzero = FALSE;
        error = TRUE;
        /* skip any leading zeroes */
        for(; first->strbuf && first->val=='0'; istreambuf_iterator_char_inc(first))
            got_digit = TRUE;

        for(; first->strbuf && first->val>='0' && first->val<='9'; istreambuf_iterator_char_inc(first)) {
            got_digit = got_nonzero = TRUE; /* leading zeroes would have been skipped, so first digit is non-zero */
            error = FALSE;
            if(dest<exp_end)
                *dest++ = first->val;
        }

        /* if just found zeroes for exponent, use that */
        if(got_digit && !got_nonzero)
        {
            error = FALSE;
            if(dest<exp_end)
                *dest++ = '0';
        }
    }

    if(sep && groups_no)
        MSVCP_basic_string_char_append_ch(&groups_found, groups_no);

    if(!cur_group) /* no groups, skip loop */
        cur_group--;
    else if(!(groups = MSVCP_basic_string_char_c_str(&groups_found))[cur_group])
        error = TRUE; /* trailing empty */

    for(; cur_group>=0 && !error; cur_group--) {
        if(*grouping == CHAR_MAX) {
            if(cur_group)
                error = TRUE;
            break;
        }else if((cur_group && *grouping!=groups[cur_group])
                || (!cur_group && *grouping<groups[cur_group])) {
            error = TRUE;
            break;
        }else if(grouping[1]) {
            grouping++;
        }
    }
    MSVCP_basic_string_char_dtor(&grouping_bstr);
    if(sep)
        MSVCP_basic_string_char_dtor(&groups_found);

    if(error) {
        *dest_beg = '\0';
        return 0;
    }
    *dest++ = '\0';
    return exp;
}

/* ?_Getffldx@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABAHPADAAV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@1AAVios_base@2@PAH@Z */
/* ?_Getffldx@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBAHPEADAEAV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@1AEAVios_base@2@PEAH@Z */
int __cdecl num_get_char__Getffldx(const num_get *this, char *dest, istreambuf_iterator_char *first,
    istreambuf_iterator_char *last, ios_base *ios, int *phexexp)
{
    FIXME("(%p %p %p %p %p) stub\n", dest, first, last, ios, phexexp);
    return -1;
}

/* ?_Getifld@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABAHPADAAV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@1HABVlocale@2@@Z */
/* ?_Getifld@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBAHPEADAEAV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@1HAEBVlocale@2@@Z */
/* Copies number to dest buffer, validates grouping and skips separators.
 * Updates first so it points past the number, all digits are skipped.
 * Returns number base (8, 10 or 16).
 * Size of dest buffer is not specified, assuming it's not smaller than 25:
 * 22(8^22>2^64)+1(detect overflows)+1(sign)+1(nullbyte) = 25
 */
int __cdecl num_get_char__Getifld(const num_get *this, char *dest, istreambuf_iterator_char *first,
        istreambuf_iterator_char *last, int fmtflags, const locale *loc)
{
    static const char digits[] = "0123456789abcdefABCDEF";

    numpunct_char *numpunct = numpunct_char_use_facet(loc);
    basic_string_char grouping_bstr;
    basic_string_char groups_found;
    int basefield, base, groups_no = 0, cur_group = 0;
    char *dest_beg = dest, *dest_end = dest+24, sep = 0;
    const char *grouping, *groups;
    BOOL error = TRUE, dest_empty = TRUE, found_zero = FALSE;

    TRACE("(%p %p %p %04x %p)\n", dest, first, last, fmtflags, loc);

    numpunct_char_grouping(numpunct, &grouping_bstr);
    grouping = MSVCP_basic_string_char_c_str(&grouping_bstr);
#if _MSVCP_VER >= 70
    if (grouping[0]) sep = numpunct_char_thousands_sep(numpunct);
#endif

    basefield = fmtflags & FMTFLAG_basefield;
    if(basefield == FMTFLAG_oct)
        base = 8;
    else if(basefield == FMTFLAG_hex)
        base = 22; /* equal to the size of digits buffer */
    else if(!basefield)
        base = 0;
    else
        base = 10;

    istreambuf_iterator_char_val(first);
    if(first->strbuf && (first->val=='-' || first->val=='+')) {
        *dest++ = first->val;
        istreambuf_iterator_char_inc(first);
    }

    if(first->strbuf && first->val=='0') {
        found_zero = TRUE;
        istreambuf_iterator_char_inc(first);
        if(first->strbuf && (first->val=='x' || first->val=='X')) {
            if(!base || base == 22) {
                found_zero = FALSE;
                istreambuf_iterator_char_inc(first);
                base = 22;
            }else {
                base = 10;
            }
        }else {
            error = FALSE;
            if(!base) base = 8;
        }
    }else {
        if (!base) base = 10;
    }

    if(sep)
    {
        MSVCP_basic_string_char_ctor(&groups_found);
        if(found_zero) ++groups_no;
    }

    for(; first->strbuf; istreambuf_iterator_char_inc(first)) {
        if(!memchr(digits, first->val, base)) {
            if(sep && first->val==sep) {
                if(!groups_no) break; /* empty group - stop parsing */
                MSVCP_basic_string_char_append_ch(&groups_found, groups_no);
                groups_no = 0;
                ++cur_group;
            }else {
                break;
            }
        }else {
            error = FALSE;
            if(dest_empty && first->val == '0')
            {
                found_zero = TRUE;
                ++groups_no;
                continue;
            }
            dest_empty = FALSE;
            /* skip digits that can't be copied to dest buffer, other
             * functions are responsible for detecting overflows */
            if(dest < dest_end)
                *dest++ = first->val;
            if(sep && groups_no<CHAR_MAX)
                ++groups_no;
        }
    }

    if(sep && groups_no)
        MSVCP_basic_string_char_append_ch(&groups_found, groups_no);

    if(!cur_group) { /* no groups, skip loop */
        cur_group--;
    }else if(!(groups = MSVCP_basic_string_char_c_str(&groups_found))[cur_group]) {
        error = TRUE; /* trailing empty */
        found_zero = FALSE;
    }

    for(; cur_group>=0 && !error; cur_group--) {
        if(*grouping == CHAR_MAX) {
            if(cur_group)
                error = TRUE;
            break;
        }else if((cur_group && *grouping!=groups[cur_group])
                || (!cur_group && *grouping<groups[cur_group])) {
            error = TRUE;
            break;
        }else if(grouping[1]) {
            grouping++;
        }
    }

    MSVCP_basic_string_char_dtor(&grouping_bstr);
    if(sep)
        MSVCP_basic_string_char_dtor(&groups_found);

    if(error) {
        if (found_zero)
            *dest++ = '0';
        else
            dest = dest_beg;
    }else if(dest_empty)
        *dest++ = '0';
    *dest = '\0';

    return (base==22 ? 16 : base);
}

/* ?_Hexdig@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABEHD000@Z */
/* ?_Hexdig@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBAHD000@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_num_get_char__Hexdig, 20)
int __thiscall MSVCP_num_get_char__Hexdig(num_get *this, char dig, char e0, char al, char au)
{
    FIXME("(%p %c %c %c %c) stub\n", this, dig, e0, al, au);
    return -1;
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAPAX@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAPEAX@Z */
#if _MSVCP_VER <= 100
#define call_num_get_char_do_get_void(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 4, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, void**), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_char_do_get_void(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 12, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, void**), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_void,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_void(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, void **pval)
{
    unsigned __int64 v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stoullx(tmp, &end, num_get_char__Getifld(this, tmp,
                &first, &last, FMTFLAG_hex, IOS_LOCALE(base)), &err);
    if(v!=(unsigned __int64)((INT_PTR)v))
        *state |= IOSTATE_failbit;
    else if(end!=tmp && !err)
        *pval = (void*)((INT_PTR)v);
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAPAX@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAPEAX@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_void,36)
istreambuf_iterator_char *__thiscall num_get_char_get_void(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, void **pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_void(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAO@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAO@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAN@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAN@Z */
#if _MSVCP_VER <= 100
#define call_num_get_char_do_get_ldouble(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 8, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, double*), \
        (this, ret, first, last, base, state, pval))
#define call_num_get_char_do_get_double(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 12, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, double*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_char_do_get_ldouble(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 16, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, double*), \
        (this, ret, first, last, base, state, pval))
#define call_num_get_char_do_get_double(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 20, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, double*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_double,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_double(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, double *pval)
{
    double v;
    char tmp[32], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stodx(tmp, &end, num_get_char__Getffld(this, tmp, &first, &last, IOS_LOCALE(base)), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAO@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAO@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_ldouble,36)
istreambuf_iterator_char *__thiscall num_get_char_get_ldouble(const num_get *this, istreambuf_iterator_char *ret,
        istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, double *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_ldouble(this, ret, first, last, base, state, pval);
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAN@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAN@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_double,36)
istreambuf_iterator_char *__thiscall num_get_char_get_double(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, double *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_double(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAM@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAM@Z */
#if _MSVCP_VER <= 100
#define call_num_get_char_do_get_float(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 16, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, float*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_char_do_get_float(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 24, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, float*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_float,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_float(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, float *pval)
{
    float v;
    char tmp[32], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stofx(tmp, &end, num_get_char__Getffld(this, tmp, &first, &last, IOS_LOCALE(base)), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAM@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAM@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_float,36)
istreambuf_iterator_char *__thiscall num_get_char_get_float(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, float *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_float(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAA_K@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEA_K@Z */
#if _MSVCP_VER <= 100
#define call_num_get_char_do_get_uint64(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 20, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, unsigned __int64*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_char_do_get_uint64(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 28, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, unsigned __int64*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_uint64,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_uint64(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, unsigned __int64 *pval)
{
    unsigned __int64 v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stoullx(tmp, &end, num_get_char__Getifld(this, tmp,
                &first, &last, base->fmtfl, IOS_LOCALE(base)), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAA_K@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEA_K@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_uint64,36)
istreambuf_iterator_char *__thiscall num_get_char_get_uint64(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, unsigned __int64 *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_uint64(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAA_J@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEA_J@Z */
#if _MSVCP_VER <= 100
#define call_num_get_char_do_get_int64(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 24, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, __int64*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_char_do_get_int64(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 32, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, __int64*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_int64,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_int64(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, __int64 *pval)
{
    __int64 v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stollx(tmp, &end, num_get_char__Getifld(this, tmp,
                &first, &last, base->fmtfl, IOS_LOCALE(base)), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAA_J@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEA_J@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_int64,36)
istreambuf_iterator_char *__thiscall num_get_char_get_int64(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, __int64 *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_int64(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAK@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAK@Z */
#if _MSVCP_VER <= 100
#define call_num_get_char_do_get_ulong(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 28, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, ULONG*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_char_do_get_ulong(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 36, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, ULONG*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_ulong,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_ulong(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, ULONG *pval)
{
    ULONG v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stoulx(tmp, &end, num_get_char__Getifld(this, tmp,
                &first, &last, base->fmtfl, IOS_LOCALE(base)), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAK@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAK@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_ulong,36)
istreambuf_iterator_char *__thiscall num_get_char_get_ulong(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, ULONG *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_ulong(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAJ@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAJ@Z */
#if _MSVCP_VER <= 100
#define call_num_get_char_do_get_long(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 32, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, LONG*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_char_do_get_long(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 40, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, LONG*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_long,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_long(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, LONG *pval)
{
    LONG v;
    char tmp[25], *end;
    int err;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    v = _Stolx(tmp, &end, num_get_char__Getifld(this, tmp,
                &first, &last, base->fmtfl, IOS_LOCALE(base)), &err);
    if(end!=tmp && !err)
        *pval = v;
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAJ@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAJ@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_long,36)
istreambuf_iterator_char *__thiscall num_get_char_get_long(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, LONG *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_long(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAI@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAI@Z */
#if _MSVCP_VER <= 100
#define call_num_get_char_do_get_uint(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 36, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, unsigned int*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_char_do_get_uint(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 44, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, unsigned int*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_uint,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_uint(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, unsigned int *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return num_get_char_do_get_ulong(this, ret, first, last, base, state, (ULONG*)pval);
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAI@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAI@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_uint,36)
istreambuf_iterator_char *__thiscall num_get_char_get_uint(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, unsigned int *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_uint(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAG@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAG@Z */
#if _MSVCP_VER <= 100
#define call_num_get_char_do_get_ushort(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 40, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, unsigned short*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_char_do_get_ushort(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 48, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, unsigned short*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_ushort,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_ushort(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, unsigned short *pval)
{
    ULONG v;
    char tmp[25], *beg, *end;
    int err, b;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    b = num_get_char__Getifld(this, tmp,
            &first, &last, base->fmtfl, IOS_LOCALE(base));
    beg = tmp + (tmp[0]=='-' ? 1 : 0);
    v = _Stoulx(beg, &end, b, &err);

    if(v != (ULONG)((unsigned short)v))
        *state |= IOSTATE_failbit;
    else if(end!=beg && !err)
        *pval = (tmp[0]=='-' ? -((unsigned short)v) : v);
    else
        *state |= IOSTATE_failbit;

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;

    *ret = first;
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAG@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEAG@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_ushort,36)
istreambuf_iterator_char *__thiscall num_get_char_get_ushort(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, unsigned short *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_ushort(this, ret, first, last, base, state, pval);
}

/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAA_N@Z */
/* ?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEA_N@Z */
#if _MSVCP_VER <= 100
#define call_num_get_char_do_get_bool(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 44, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, bool*), \
        (this, ret, first, last, base, state, pval))
#else
#define call_num_get_char_do_get_bool(this, ret, first, last, base, state, pval) CALL_VTBL_FUNC(this, 52, istreambuf_iterator_char*, \
        (const num_get*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, bool*), \
        (this, ret, first, last, base, state, pval))
#endif
DEFINE_THISCALL_WRAPPER(num_get_char_do_get_bool,36)
istreambuf_iterator_char *__thiscall num_get_char_do_get_bool(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, bool *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);

    if(base->fmtfl & FMTFLAG_boolalpha) {
        numpunct_char *numpunct = numpunct_char_use_facet(IOS_LOCALE(base));
        basic_string_char false_bstr, true_bstr;
        const char *pfalse, *ptrue;

        numpunct_char_falsename(numpunct, &false_bstr);
        numpunct_char_truename(numpunct, &true_bstr);
        pfalse = MSVCP_basic_string_char_c_str(&false_bstr);
        ptrue = MSVCP_basic_string_char_c_str(&true_bstr);

        for(istreambuf_iterator_char_val(&first); first.strbuf;) {
            if(pfalse && *pfalse && first.val!=*pfalse)
                pfalse = NULL;
            if(ptrue && *ptrue && first.val!=*ptrue)
                ptrue = NULL;

            if(pfalse && *pfalse && ptrue && !*ptrue)
                ptrue = NULL;
            if(ptrue && *ptrue && pfalse && !*pfalse)
                pfalse = NULL;

            if(pfalse)
                pfalse++;
            if(ptrue)
                ptrue++;

            if(pfalse || ptrue)
                istreambuf_iterator_char_inc(&first);

            if((!pfalse || !*pfalse) && (!ptrue || !*ptrue))
                break;
        }

        if(ptrue)
            *pval = TRUE;
        else if(pfalse)
            *pval = FALSE;
        else
            *state |= IOSTATE_failbit;

        MSVCP_basic_string_char_dtor(&false_bstr);
        MSVCP_basic_string_char_dtor(&true_bstr);
    }else {
        char tmp[25], *end;
        int err;
        LONG v = _Stolx(tmp, &end, num_get_char__Getifld(this, tmp,
                    &first, &last, base->fmtfl, IOS_LOCALE(base)), &err);

        if(end!=tmp && err==0 && (v==0 || v==1))
            *pval = v;
        else
            *state |= IOSTATE_failbit;
    }

    if(!first.strbuf)
        *state |= IOSTATE_eofbit;
    memcpy(ret, &first, sizeof(first));
    return ret;
}

/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAA_N@Z */
/* ?get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHAEA_N@Z */
DEFINE_THISCALL_WRAPPER(num_get_char_get_bool,36)
istreambuf_iterator_char *__thiscall num_get_char_get_bool(const num_get *this, istreambuf_iterator_char *ret,
    istreambuf_iterator_char first, istreambuf_iterator_char last, ios_base *base, int *state, bool *pval)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, state, pval);
    return call_num_get_char_do_get_bool(this, ret, first, last, base, state, pval);
}

/* ?id@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@2V0locale@2@A */
locale_id num_put_char_id = {0};

/* num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@6B@ */
extern const vtable_ptr num_put_char_vtable;

/* ?_Init@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(num_put_char__Init, 8)
void __thiscall num_put_char__Init(num_put *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
#if _MSVCP_VER < 110
    _Locinfo__Getcvt(locinfo, &this->cvt);
#endif
}

/* ??0?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(num_put_char_ctor_locinfo, 12)
num_put* __thiscall num_put_char_ctor_locinfo(num_put *this, const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &num_put_char_vtable;

    num_put_char__Init(this, locinfo);
    return this;
}

/* ??0?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@I@Z */
/* ??0?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(num_put_char_ctor_refs, 8)
num_put* __thiscall num_put_char_ctor_refs(num_put *this, size_t refs)
{
     _Locinfo locinfo;

     TRACE("(%p %Iu)\n", this, refs);

     _Locinfo_ctor(&locinfo);
     num_put_char_ctor_locinfo(this, &locinfo, refs);
     _Locinfo_dtor(&locinfo);
     return this;
}

/* ??_F?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAEXXZ */
/* ??_F?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(num_put_char_ctor, 4)
num_put* __thiscall num_put_char_ctor(num_put *this)
{
    return num_put_char_ctor_refs(this, 0);
}

/* ??1?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@UAE@XZ */
/* ??1?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@UEAA@XZ */
/* ??1?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MAE@XZ */
/* ??1?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(num_put_char_dtor, 4)
void __thiscall num_put_char_dtor(num_put *this)
{
    TRACE("(%p)\n", this);
    locale_facet_dtor(&this->facet);
}

DEFINE_THISCALL_WRAPPER(num_put_char_vector_dtor, 8)
num_put* __thiscall num_put_char_vector_dtor(num_put *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            num_put_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        num_put_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl num_put_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = operator_new(sizeof(num_put));
        _Locinfo_ctor_cstr(&locinfo, locale_string_char_c_str(&loc->ptr->name));
        num_put_char_ctor_locinfo((num_put*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_NUMERIC;
}

/* ?_Getcat@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl num_put_char__Getcat_old(const locale_facet **facet)
{
    return num_put_char__Getcat(facet, locale_classic());
}

num_put* num_put_char_use_facet(const locale *loc)
{
    static num_put *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&num_put_char_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (num_put*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    num_put_char__Getcat(&fac, loc);
    obj = (num_put*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@PBDI@Z */
/* ?_Put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@PEBD_K@Z */
ostreambuf_iterator_char* __cdecl num_put_char__Put(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, const char *ptr, size_t count)
{
    TRACE("(%p %p %p %Iu)\n", this, ret, ptr, count);

    for(; count>0; count--)
        ostreambuf_iterator_char_put(&dest, *ptr++);

    *ret = dest;
    return ret;
}

/* ?_Putc@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@PBDI@Z */
/* ?_Putc@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@PEBD_K@Z */
ostreambuf_iterator_char* __cdecl num_put_char__Putc(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, const char *ptr, size_t count)
{
    TRACE("(%p %p %p %Iu)\n", this, ret, ptr, count);

    for(; count>0; count--)
        ostreambuf_iterator_char_put(&dest, *ptr++);

    *ret = dest;
    return ret;
}

/* ?_Putgrouped@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@PBDID@Z */
/* ?_Putgrouped@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@PEBD_KD@Z */
ostreambuf_iterator_char* __cdecl num_put_char__Putgrouped(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, const char *ptr, size_t count, char delim)
{
    FIXME("(%p %p %p %Iu %d) stub\n", this, ret, ptr, count, delim);
    return NULL;
}

/* ?_Rep@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@DI@Z */
/* ?_Rep@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@D_K@Z */
ostreambuf_iterator_char* __cdecl num_put_char__Rep(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, char c, size_t count)
{
    TRACE("(%p %p %d %Iu)\n", this, ret, c, count);

    for(; count>0; count--)
        ostreambuf_iterator_char_put(&dest, c);

    *ret = dest;
    return ret;
}

/* ?_Ffmt@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABAPADPADDH@Z */
/* ?_Ffmt@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBAPEADPEADDH@Z */
char* __cdecl num_put_char__Ffmt(const num_put *this, char *fmt, char spec, int fmtfl)
{
    int type = fmtfl & FMTFLAG_floatfield;
    char *p = fmt;

    TRACE("(%p %p %d %d)\n", this, fmt, spec, fmtfl);

    *p++ = '%';
    if(fmtfl & FMTFLAG_showpos)
        *p++ = '+';
    if(fmtfl & FMTFLAG_showpoint)
        *p++ = '#';
    *p++ = '.';
    *p++ = '*';
    if(spec)
        *p++ = spec;

    if(type == FMTFLAG_fixed)
        *p++ = 'f';
    else if(type == FMTFLAG_scientific)
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'E' : 'e';
    else if(type == (FMTFLAG_fixed|FMTFLAG_scientific))
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'A' : 'a';
    else
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'G' : 'g';

    *p++ = '\0';
    return fmt;
}

/* ?_Fput@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DPBDIIII@Z */
/* ?_Fput@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DPEBD_K333@Z */
ostreambuf_iterator_char* __cdecl num_put_char__Fput(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, const char *buf, size_t bef_point,
        size_t aft_point, size_t trailing, size_t count)
{
    FIXME("(%p %p %p %d %p %Iu %Iu %Iu %Iu) stub\n", this, ret, base,
            fill, buf, bef_point, aft_point, trailing, count);
    return NULL;
}

/* TODO: This function should be removed when num_put_char__Fput is implemented */
static ostreambuf_iterator_char* num_put_char_fput(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, char *buf, size_t count)
{
    numpunct_char *numpunct = numpunct_char_use_facet(IOS_LOCALE(base));
    basic_string_char grouping_bstr;
    const char *grouping;
    char *p, sep = 0, dec_point = *localeconv()->decimal_point;
    int cur_group = 0, group_size = 0;
    int adjustfield = base->fmtfl & FMTFLAG_adjustfield;
    size_t pad;

    TRACE("(%p %p %p %d %s %Iu)\n", this, ret, base, fill, buf, count);

    /* Change decimal point */
    for(p=buf; p<buf+count; p++) {
        if(*p == dec_point) {
            *p = numpunct_char_decimal_point(numpunct);
            break;
        }
    }
    p--;

    /* Add separators to number */
    numpunct_char_grouping(numpunct, &grouping_bstr);
    grouping = MSVCP_basic_string_char_c_str(&grouping_bstr);
#if _MSVCP_VER >= 70
    if (grouping[0]) sep = numpunct_char_thousands_sep(numpunct);
#endif

    for(; p>buf && sep && grouping[cur_group]!=CHAR_MAX; p--) {
        group_size++;
        if(group_size == grouping[cur_group]) {
            group_size = 0;
            if(grouping[cur_group+1])
                cur_group++;

            memmove(p+1, p, buf+count-p);
            *p = sep;
            count++;
        }
    }
    MSVCP_basic_string_char_dtor(&grouping_bstr);

    /* Display number with padding */
    if(count >= base->wide)
        pad = 0;
    else
        pad = base->wide-count;
    base->wide = 0;

    if((adjustfield & FMTFLAG_internal) && (buf[0]=='-' || buf[0]=='+')) {
        num_put_char__Putc(this, &dest, dest, buf, 1);
        buf++;
    }
    if(adjustfield != FMTFLAG_left) {
        num_put_char__Rep(this, ret, dest, fill, pad);
        pad = 0;
    }
    num_put_char__Putc(this, &dest, dest, buf, count);
    return num_put_char__Rep(this, ret, dest, fill, pad);
}

/* ?_Ifmt@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABAPADPADPBDH@Z */
/* ?_Ifmt@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBAPEADPEADPEBDH@Z */
char* __cdecl num_put_char__Ifmt(const num_put *this, char *fmt, const char *spec, int fmtfl)
{
    int base = fmtfl & FMTFLAG_basefield;
    char *p = fmt;

    TRACE("(%p %p %p %d)\n", this, fmt, spec, fmtfl);

    *p++ = '%';
    if(fmtfl & FMTFLAG_showpos)
        *p++ = '+';
    if(fmtfl & FMTFLAG_showbase)
        *p++ = '#';

    *p++ = *spec++;
    if(*spec == 'l')
        *p++ = *spec++;

    if(base == FMTFLAG_oct)
        *p++ = 'o';
    else if(base == FMTFLAG_hex)
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'X' : 'x';
    else
        *p++ = *spec;

    *p++ = '\0';
    return fmt;
}

/* ?_Iput@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DPADI@Z */
/* ?_Iput@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DPEAD_K@Z */
ostreambuf_iterator_char* __cdecl num_put_char__Iput(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, char *buf, size_t count)
{
    numpunct_char *numpunct = numpunct_char_use_facet(IOS_LOCALE(base));
    basic_string_char grouping_bstr;
    const char *grouping;
    char *p, sep = 0;
    int cur_group = 0, group_size = 0;
    int adjustfield = base->fmtfl & FMTFLAG_adjustfield;
    size_t pad;

    TRACE("(%p %p %p %d %s %Iu)\n", this, ret, base, fill, buf, count);

    /* Add separators to number */
    numpunct_char_grouping(numpunct, &grouping_bstr);
    grouping = MSVCP_basic_string_char_c_str(&grouping_bstr);
#if _MSVCP_VER >= 70
    if (grouping[0]) sep = numpunct_char_thousands_sep(numpunct);
#endif

    for(p=buf+count-1; p>buf && sep && grouping[cur_group]!=CHAR_MAX; p--) {
        group_size++;
        if(group_size == grouping[cur_group]) {
            group_size = 0;
            if(grouping[cur_group+1])
                cur_group++;

            memmove(p+1, p, buf+count-p);
            *p = sep;
            count++;
        }
    }
    MSVCP_basic_string_char_dtor(&grouping_bstr);

    /* Display number with padding */
    if(count >= base->wide)
        pad = 0;
    else
        pad = base->wide-count;
    base->wide = 0;

    if((adjustfield & FMTFLAG_internal) && (buf[0]=='-' || buf[0]=='+')) {
        num_put_char__Putc(this, &dest, dest, buf, 1);
        buf++;
    }else if((adjustfield & FMTFLAG_internal) && (buf[1]=='x' || buf[1]=='X')) {
        num_put_char__Putc(this, &dest, dest, buf, 2);
        buf += 2;
    }
    if(adjustfield != FMTFLAG_left) {
        num_put_char__Rep(this, ret, dest, fill, pad);
        pad = 0;
    }
    num_put_char__Putc(this, &dest, dest, buf, count);
    return num_put_char__Rep(this, ret, dest, fill, pad);
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DJ@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DJ@Z */
#if _MSVCP_VER <= 100
#define call_num_put_char_do_put_long(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 28, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, LONG), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_char_do_put_long(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 36, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, LONG), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_long, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_long, 32)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_do_put_long(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, LONG v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators between every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d %ld)\n", this, ret, base, fill, v);

    return num_put_char__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_char__Ifmt(this, fmt, "ld", base->fmtfl), v));
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DJ@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DJ@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_put_long, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_put_long, 32)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_put_long(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, LONG v)
{
    TRACE("(%p %p %p %d %ld)\n", this, ret, base, fill, v);
    return call_num_put_char_do_put_long(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DK@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DK@Z */
#if _MSVCP_VER <= 100
#define call_num_put_char_do_put_ulong(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 24, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, ULONG), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_char_do_put_ulong(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 32, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, ULONG), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_ulong, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_ulong, 32)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_do_put_ulong(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, ULONG v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators between every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d %ld)\n", this, ret, base, fill, v);

    return num_put_char__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_char__Ifmt(this, fmt, "lu", base->fmtfl), v));
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DK@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DK@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_put_ulong, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_put_ulong, 32)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_put_ulong(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, ULONG v)
{
    TRACE("(%p %p %p %d %ld)\n", this, ret, base, fill, v);
    return call_num_put_char_do_put_ulong(this, ret, dest, base, fill, v);
}

static inline unsigned get_precision(const ios_base *base)
{
    streamsize ret = base->prec <= 0 && !(base->fmtfl & FMTFLAG_fixed) ? 6 : base->prec;
    if(ret > UINT_MAX)
        ret = UINT_MAX;
    return ret;
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DN@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DN@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DO@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DO@Z */
#if _MSVCP_VER <= 100
#define call_num_put_char_do_put_double(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 12, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, double), \
        (this, ret, dest, base, fill, v))
#define call_num_put_char_do_put_ldouble(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 8, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, double), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_char_do_put_double(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 20, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, double), \
        (this, ret, dest, base, fill, v))
#define call_num_put_char_do_put_ldouble(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 16, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, double), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_double, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_double, 36)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_do_put_double(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, double v)
{
    char *tmp;
    char fmt[8]; /* strlen("%+#.*lg")+1 */
    int size;
    unsigned prec;

    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);

    num_put_char__Ffmt(this, fmt, '\0', base->fmtfl);
    prec = get_precision(base);
    size = _scprintf(fmt, prec, v);

    /* TODO: don't use dynamic allocation */
    tmp = operator_new(size*2);
    num_put_char_fput(this, ret, dest, base, fill, tmp, sprintf(tmp, fmt, prec, v));
    operator_delete(tmp);
    return ret;
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DN@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DN@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_put_double, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_put_double, 36)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_put_double(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, double v)
{
    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);
    return call_num_put_char_do_put_double(this, ret, dest, base, fill, v);
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DO@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DO@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_put_ldouble, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_put_ldouble, 36)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_put_ldouble(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, double v)
{
    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);
    return call_num_put_char_do_put_ldouble(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DPBX@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DPEBX@Z */
#if _MSVCP_VER <= 100
#define call_num_put_char_do_put_ptr(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 4, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, const void*), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_char_do_put_ptr(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 12, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, const void*), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_ptr, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_ptr, 32)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_do_put_ptr(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, const void *v)
{
    char tmp[17]; /* 8(16^8==2^64)*2(separators between every digit) + 1 */

    TRACE("(%p %p %p %d %p)\n", this, ret, base, fill, v);

    return num_put_char__Iput(this, ret, dest, base, fill, tmp, sprintf(tmp, "%p", v));
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DPBX@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DPEBX@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_put_ptr, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_put_ptr, 32)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_put_ptr(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, const void *v)
{
    TRACE("(%p %p %p %d %p)\n", this, ret, base, fill, v);
    return call_num_put_char_do_put_ptr(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@D_J@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@D_J@Z */
#if _MSVCP_VER <= 100
#define call_num_put_char_do_put_int64(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 20, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, __int64), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_char_do_put_int64(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 28, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, __int64), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_int64, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_int64, 36)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_do_put_int64(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, __int64 v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators between every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d)\n", this, ret, base, fill);

    return num_put_char__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_char__Ifmt(this, fmt, "lld", base->fmtfl), v));
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@D_J@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@D_J@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_put_int64, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_put_int64, 36)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_put_int64(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, __int64 v)
{
    TRACE("(%p %p %p %d)\n", this, ret, base, fill);
    return call_num_put_char_do_put_int64(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@D_K@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@D_K@Z */
#if _MSVCP_VER <= 100
#define call_num_put_char_do_put_uint64(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 16, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, unsigned __int64), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_char_do_put_uint64(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 24, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, unsigned __int64), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_uint64, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_uint64, 36)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_do_put_uint64(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, unsigned __int64 v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators between every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d)\n", this, ret, base, fill);

    return num_put_char__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_char__Ifmt(this, fmt, "llu", base->fmtfl), v));
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@D_K@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@D_K@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_put_uint64, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_put_uint64, 36)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_put_uint64(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, unsigned __int64 v)
{
    TRACE("(%p %p %p %d)\n", this, ret, base, fill);
    return call_num_put_char_do_put_uint64(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@D_N@Z */
/* ?do_put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@D_N@Z */
#if _MSVCP_VER <= 100
#define call_num_put_char_do_put_bool(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 32, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, bool), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_char_do_put_bool(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 40, ostreambuf_iterator_char*, \
        (const num_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, bool), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_bool, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_do_put_bool, 32)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_do_put_bool(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, bool v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);

    if(base->fmtfl & FMTFLAG_boolalpha) {
        numpunct_char *numpunct = numpunct_char_use_facet(IOS_LOCALE(base));
        basic_string_char str;
        size_t pad, len;

        if(v)
            numpunct_char_truename(numpunct, &str);
        else
            numpunct_char_falsename(numpunct, &str);

        len = MSVCP_basic_string_char_length(&str);
        pad = (len>base->wide ? 0 : base->wide-len);
        base->wide = 0;

        if((base->fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            num_put_char__Rep(this, &dest, dest, fill, pad);
            pad = 0;
        }
        num_put_char__Putc(this, &dest, dest, MSVCP_basic_string_char_c_str(&str), len);
        MSVCP_basic_string_char_dtor(&str);
        return num_put_char__Rep(this, ret, dest, fill, pad);
    }

    return num_put_char_put_long(this, ret, dest, base, fill, v);
}

/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@D_N@Z */
/* ?put@?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@D_N@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_char_put_bool, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_char_put_bool, 32)
#endif
ostreambuf_iterator_char* __thiscall num_put_char_put_bool(const num_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, bool v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);
    return call_num_put_char_do_put_bool(this, ret, dest, base, fill, v);
}

/* ?id@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@2V0locale@2@A */
locale_id num_put_wchar_id = {0};
/* ?id@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@2V0locale@2@A */
locale_id num_put_short_id = {0};

/* num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@6B@ */
extern const vtable_ptr num_put_wchar_vtable;
/* num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@6B@ */
extern const vtable_ptr num_put_short_vtable;

/* ?_Init@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(num_put_wchar__Init, 8)
void __thiscall num_put_wchar__Init(num_put *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
#if _MSVCP_VER < 110
    _Locinfo__Getcvt(locinfo, &this->cvt);
#endif
}

/* ??0?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(num_put_wchar_ctor_locinfo, 12)
num_put* __thiscall num_put_wchar_ctor_locinfo(num_put *this, const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);

    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &num_put_wchar_vtable;

    num_put_wchar__Init(this, locinfo);
    return this;
}

/* ??0?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(num_put_short_ctor_locinfo, 12)
num_put* __thiscall num_put_short_ctor_locinfo(num_put *this, const _Locinfo *locinfo, size_t refs)
{
    num_put_wchar_ctor_locinfo(this, locinfo, refs);
    this->facet.vtable = &num_put_short_vtable;
    return this;
}

/* ??0?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@I@Z */
/* ??0?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(num_put_wchar_ctor_refs, 8)
num_put* __thiscall num_put_wchar_ctor_refs(num_put *this, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %Iu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    num_put_wchar_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@I@Z */
/* ??0?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(num_put_short_ctor_refs, 8)
num_put* __thiscall num_put_short_ctor_refs(num_put *this, size_t refs)
{
    num_put_wchar_ctor_refs(this, refs);
    this->facet.vtable = &num_put_short_vtable;
    return this;
}

/* ??_F?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAEXXZ */
/* ??_F?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(num_put_wchar_ctor, 4)
num_put* __thiscall num_put_wchar_ctor(num_put *this)
{
    return num_put_wchar_ctor_refs(this, 0);
}

/* ??_F?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAEXXZ */
/* ??_F?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(num_put_short_ctor, 4)
num_put* __thiscall num_put_short_ctor(num_put *this)
{
    return num_put_short_ctor_refs(this, 0);
}

/* ??1?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@UAE@XZ */
/* ??1?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@UEAA@XZ */
/* ??1?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MAE@XZ */
/* ??1?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEAA@XZ */
/* ??1?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MAE@XZ */
/* ??1?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(num_put_wchar_dtor, 4)
void __thiscall num_put_wchar_dtor(num_put *this)
{
    TRACE("(%p)\n", this);
    locale_facet_dtor(&this->facet);
}

DEFINE_THISCALL_WRAPPER(num_put_wchar_vector_dtor, 8)
num_put* __thiscall num_put_wchar_vector_dtor(num_put *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            num_put_wchar_dtor(this+i);
        operator_delete(ptr);
    } else {
        num_put_wchar_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl num_put_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = operator_new(sizeof(num_put));
        _Locinfo_ctor_cstr(&locinfo, locale_string_char_c_str(&loc->ptr->name));
        num_put_wchar_ctor_locinfo((num_put*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_NUMERIC;
}

/* ?_Getcat@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl num_put_wchar__Getcat_old(const locale_facet **facet)
{
    return num_put_wchar__Getcat(facet, locale_classic());
}

/* ?_Getcat@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl num_put_short__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = operator_new(sizeof(num_put));
        _Locinfo_ctor_cstr(&locinfo, locale_string_char_c_str(&loc->ptr->name));
        num_put_short_ctor_locinfo((num_put*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_NUMERIC;
}

/* ?_Getcat@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl num_put_short__Getcat_old(const locale_facet **facet)
{
    return num_put_short__Getcat(facet, locale_classic());
}

num_put* num_put_wchar_use_facet(const locale *loc)
{
    static num_put *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&num_put_wchar_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (num_put*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    num_put_wchar__Getcat(&fac, loc);
    obj = (num_put*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

num_put* num_put_short_use_facet(const locale *loc)
{
    static num_put *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&num_put_short_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (num_put*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    num_put_short__Getcat(&fac, loc);
    obj = (num_put*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@PB_WI@Z */
/* ?_Put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@PEB_W_K@Z */
/* ?_Put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@PBGI@Z */
/* ?_Put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@PEBG_K@Z */
ostreambuf_iterator_wchar* __cdecl num_put_wchar__Put(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, const wchar_t *ptr, size_t count)
{
    TRACE("(%p %p %s %Iu)\n", this, ret, debugstr_wn(ptr, count), count);

    for(; count>0; count--)
        ostreambuf_iterator_wchar_put(&dest, *ptr++);

    *ret = dest;
    return ret;
}

#if _MSVCP_VER < 110
/* ?_Putc@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@PBDI@Z */
/* ?_Putc@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@PEBD_K@Z */
/* ?_Putc@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@PBDI@Z */
/* ?_Putc@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@PEBD_K@Z */
ostreambuf_iterator_wchar* __cdecl num_put_wchar__Putc(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, const char *ptr, size_t count)
{
    _Mbstatet state;
    wchar_t ch;

    TRACE("(%p %p %s %Iu)\n", this, ret, debugstr_an(ptr, count), count);

    memset(&state, 0, sizeof(state));
    for(; count>0; count--) {
        if(_Mbrtowc(&ch, ptr++, 1, &state, &this->cvt) == 1)
            ostreambuf_iterator_wchar_put(&dest, ch);
    }

    *ret = dest;
    return ret;
}
#endif

/* ?_Putgrouped@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@PBDI_W@Z */
/* ?_Putgrouped@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@PEBD_K_W@Z */
/* ?_Putgrouped@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@PBDIG@Z */
/* ?_Putgrouped@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@PEBD_KG@Z */
ostreambuf_iterator_wchar* __cdecl num_put_wchar__Putgrouped(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, const char *ptr, size_t count, wchar_t delim)
{
    FIXME("(%p %p %p %Iu %d) stub\n", this, ret, ptr, count, delim);
    return NULL;
}

/* ?_Rep@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@_WI@Z */
/* ?_Rep@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@_W_K@Z */
/* ?_Rep@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@GI@Z */
/* ?_Rep@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@G_K@Z */
ostreambuf_iterator_wchar* __cdecl num_put_wchar__Rep(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, wchar_t c, size_t count)
{
    TRACE("(%p %p %d %Iu)\n", this, ret, c, count);

    for(; count>0; count--)
        ostreambuf_iterator_wchar_put(&dest, c);

    *ret = dest;
    return ret;
}

/* ?_Ffmt@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABAPADPADDH@Z */
/* ?_Ffmt@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBAPEADPEADDH@Z */
/* ?_Ffmt@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABAPADPADDH@Z */
/* ?_Ffmt@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBAPEADPEADDH@Z */
char* __cdecl num_put_wchar__Ffmt(const num_put *this, char *fmt, char spec, int fmtfl)
{
    int type = fmtfl & FMTFLAG_floatfield;
    char *p = fmt;

    TRACE("(%p %p %d %d)\n", this, fmt, spec, fmtfl);

    *p++ = '%';
    if(fmtfl & FMTFLAG_showpos)
        *p++ = '+';
    if(fmtfl & FMTFLAG_showbase)
        *p++ = '#';
    *p++ = '.';
    *p++ = '*';
    if(spec)
        *p++ = spec;

    if(type == FMTFLAG_fixed)
        *p++ = 'f';
    else if(type == FMTFLAG_scientific)
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'E' : 'e';
    else if(type == (FMTFLAG_fixed|FMTFLAG_scientific))
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'A' : 'a';
    else
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'G' : 'g';

    *p++ = '\0';
    return fmt;
}

/* ?_Fput@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WPBDIIII@Z */
/* ?_Fput@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WPEBD_K444@Z */
ostreambuf_iterator_wchar* __cdecl num_put_wchar__Fput(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, const char *buf, size_t bef_point,
        size_t aft_point, size_t trailing, size_t count)
{
    FIXME("(%p %p %p %d %p %Iu %Iu %Iu %Iu) stub\n", this, ret, base,
            fill, buf, bef_point, aft_point, trailing, count);
    return NULL;
}

/* ?_Fput@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GPBDIIII@Z */
/* ?_Fput@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GPEBD_K333@Z */
ostreambuf_iterator_wchar* __cdecl num_put_short__Fput(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, const char *buf, size_t bef_point,
        size_t aft_point, size_t trailing, size_t count)
{
    FIXME("(%p %p %p %d %p %Iu %Iu %Iu %Iu) stub\n", this, ret, base,
            fill, buf, bef_point, aft_point, trailing, count);
    return NULL;
}

#if _MSVCP_VER < 110
static void num_put_wchar_wide_put(const num_put *this,
        ostreambuf_iterator_wchar *dest, ios_base *base,
        const char *buf, size_t count)
{
    num_put_wchar__Putc(this, dest, *dest, buf, count);
}
#else
static void num_put_wchar_wide_put(const num_put *this,
        ostreambuf_iterator_wchar *dest, ios_base *base,
                const char *buf, size_t count)
{
    ctype_wchar *ctype;
    size_t i;

    ctype = ctype_wchar_use_facet(IOS_LOCALE(base));
    for(i=0; i<count; i++)
        ostreambuf_iterator_wchar_put(dest, ctype_wchar_widen_ch(ctype, buf[i]));
}
#endif

/* TODO: This function should be removed when num_put_wchar__Fput is implemented */
static ostreambuf_iterator_wchar* num_put__fput(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, char *buf,
        size_t count, numpunct_wchar *numpunct)
{
    basic_string_char grouping_bstr;
    const char *grouping;
    char *p, dec_point = *localeconv()->decimal_point;
    wchar_t sep = 0;
    int cur_group = 0, group_size = 0;
    int adjustfield = base->fmtfl & FMTFLAG_adjustfield;
    size_t i, pad;

    TRACE("(%p %p %p %d %s %Iu)\n", this, ret, base, fill, buf, count);

    for(p=buf; p<buf+count; p++) {
        if(*p == dec_point)
            break;
    }
    p--;

    /* Add separators to number */
    numpunct_wchar_grouping(numpunct, &grouping_bstr);
    grouping = MSVCP_basic_string_char_c_str(&grouping_bstr);
#if _MSVCP_VER >= 70
    if (grouping[0]) sep = numpunct_wchar_thousands_sep(numpunct);
#endif

    for(; p>buf && sep && grouping[cur_group]!=CHAR_MAX; p--) {
        group_size++;
        if(group_size == grouping[cur_group]) {
            group_size = 0;
            if(grouping[cur_group+1])
                cur_group++;

            memmove(p+1, p, buf+count-p);
            *p = '\0'; /* mark thousands separator positions */
            count++;
        }
    }
    MSVCP_basic_string_char_dtor(&grouping_bstr);

    /* Display number with padding */
    if(count >= base->wide)
        pad = 0;
    else
        pad = base->wide-count;
    base->wide = 0;

    if((adjustfield & FMTFLAG_internal) && (buf[0]=='-' || buf[0]=='+')) {
        num_put_wchar_wide_put(this, &dest, base, buf, 1);
        buf++;
    }
    if(adjustfield != FMTFLAG_left) {
        num_put_wchar__Rep(this, ret, dest, fill, pad);
        pad = 0;
    }

    for(i=0; i<count; i++) {
        if(buf[i] == dec_point)
            num_put_wchar__Rep(this, &dest, dest, numpunct_wchar_decimal_point(numpunct), 1);
        else if(!buf[i])
            num_put_wchar__Rep(this, &dest, dest, sep, 1);
        else
            num_put_wchar_wide_put(this, &dest, base, buf+i, 1);
    }

    return num_put_wchar__Rep(this, ret, dest, fill, pad);
}

/* ?_Ifmt@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABAPADPADPBDH@Z */
/* ?_Ifmt@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBAPEADPEADPEBDH@Z */
/* ?_Ifmt@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABAPADPADPBDH@Z */
/* ?_Ifmt@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBAPEADPEADPEBDH@Z */
char* __cdecl num_put_wchar__Ifmt(const num_put *this, char *fmt, const char *spec, int fmtfl)
{
    int base = fmtfl & FMTFLAG_basefield;
    char *p = fmt;

    TRACE("(%p %p %p %d)\n", this, fmt, spec, fmtfl);

    *p++ = '%';
    if(fmtfl & FMTFLAG_showpos)
        *p++ = '+';
    if(fmtfl & FMTFLAG_showbase)
        *p++ = '#';

    *p++ = *spec++;
    if(*spec == 'l')
        *p++ = *spec++;

    if(base == FMTFLAG_oct)
        *p++ = 'o';
    else if(base == FMTFLAG_hex)
        *p++ = (fmtfl & FMTFLAG_uppercase) ? 'X' : 'x';
    else
        *p++ = *spec;

    *p++ = '\0';
    return fmt;
}

static ostreambuf_iterator_wchar* num_put__Iput(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, char *buf,
        size_t count, numpunct_wchar *numpunct)
{
    basic_string_char grouping_bstr;
    const char *grouping;
    char *p;
    wchar_t sep;
    int cur_group = 0, group_size = 0;
    int adjustfield = base->fmtfl & FMTFLAG_adjustfield;
    size_t i, pad;

    TRACE("(%p %p %p %d %s %Iu)\n", this, ret, base, fill, buf, count);

    /* Add separators to number */
    numpunct_wchar_grouping(numpunct, &grouping_bstr);
    grouping = MSVCP_basic_string_char_c_str(&grouping_bstr);
    sep = grouping[0] ? numpunct_wchar_thousands_sep(numpunct) : '\0';

    for(p=buf+count-1; p>buf && sep && grouping[cur_group]!=CHAR_MAX; p--) {
        group_size++;
        if(group_size == grouping[cur_group]) {
            group_size = 0;
            if(grouping[cur_group+1])
                cur_group++;

            memmove(p+1, p, buf+count-p);
            *p = '\0'; /* mark thousands separator positions */
            count++;
        }
    }
    MSVCP_basic_string_char_dtor(&grouping_bstr);

    /* Display number with padding */
    if(count >= base->wide)
        pad = 0;
    else
        pad = base->wide-count;
    base->wide = 0;

    if((adjustfield & FMTFLAG_internal) && (buf[0]=='-' || buf[0]=='+')) {
        num_put_wchar_wide_put(this, &dest, base, buf, 1);
        buf++;
    }else if((adjustfield & FMTFLAG_internal) && (buf[1]=='x' || buf[1]=='X')) {
        num_put_wchar_wide_put(this, &dest, base, buf, 2);
        buf += 2;
    }
    if(adjustfield != FMTFLAG_left) {
        num_put_wchar__Rep(this, ret, dest, fill, pad);
        pad = 0;
    }

    for(i=0; i<count; i++) {
        if(!buf[i])
            num_put_wchar__Rep(this, &dest, dest, sep, 1);
        else
            num_put_wchar_wide_put(this, &dest, base, buf+i, 1);
    }

    return num_put_wchar__Rep(this, ret, dest, fill, pad);
}

/* ?_Iput@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WPADI@Z */
/* ?_Iput@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WPEAD_K@Z */
ostreambuf_iterator_wchar* __cdecl num_put_wchar__Iput(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, char *buf, size_t count)
{
    return num_put__Iput(this, ret, dest, base, fill, buf, count, numpunct_wchar_use_facet(IOS_LOCALE(base)));
}

/* ?_Iput@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@ABA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GPADI@Z */
/* ?_Iput@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@AEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GPEAD_K@Z */
ostreambuf_iterator_wchar* __cdecl num_put_short__Iput(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, char *buf, size_t count)
{
    return num_put__Iput(this, ret, dest, base, fill, buf, count, numpunct_short_use_facet(IOS_LOCALE(base)));
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WJ@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WJ@Z */
#if _MSVCP_VER <= 100
#define call_num_put_wchar_do_put_long(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 28, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, LONG), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_wchar_do_put_long(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 36, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, LONG), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_long, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_long, 32)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_long(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, LONG v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators between every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d %ld)\n", this, ret, base, fill, v);

    return num_put_wchar__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "ld", base->fmtfl), v));
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GJ@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GJ@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_long, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_long, 32)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_long(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, LONG v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators between every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d %ld)\n", this, ret, base, fill, v);

    return num_put_short__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "ld", base->fmtfl), v));
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WJ@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WJ@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GJ@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GJ@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_long, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_long, 32)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_long(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, LONG v)
{
    TRACE("(%p %p %p %d %ld)\n", this, ret, base, fill, v);
    return call_num_put_wchar_do_put_long(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WK@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WK@Z */
#if _MSVCP_VER <= 100
#define call_num_put_wchar_do_put_ulong(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 24, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, ULONG), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_wchar_do_put_ulong(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 32, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, ULONG), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_ulong, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_ulong, 32)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_ulong(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, ULONG v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators between every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d %ld)\n", this, ret, base, fill, v);

    return num_put_wchar__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "lu", base->fmtfl), v));
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GK@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GK@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_ulong, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_ulong, 32)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_ulong(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, ULONG v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators between every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d %ld)\n", this, ret, base, fill, v);

    return num_put_short__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "lu", base->fmtfl), v));
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WK@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WK@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GK@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GK@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_ulong, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_ulong, 32)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_ulong(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, ULONG v)
{
    TRACE("(%p %p %p %d %ld)\n", this, ret, base, fill, v);
    return call_num_put_wchar_do_put_ulong(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WN@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WN@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WO@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WO@Z */
#if _MSVCP_VER <= 100
#define call_num_put_wchar_do_put_double(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 12, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, double), \
        (this, ret, dest, base, fill, v))
#define call_num_put_wchar_do_put_ldouble(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 8, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, double), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_wchar_do_put_double(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 20, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, double), \
        (this, ret, dest, base, fill, v))
#define call_num_put_wchar_do_put_ldouble(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 16, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, double), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_double, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_double, 36)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_double(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, double v)
{
    char *tmp;
    char fmt[8]; /* strlen("%+#.*lg")+1 */
    int size;
    unsigned prec;

    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);

    num_put_wchar__Ffmt(this, fmt, '\0', base->fmtfl);
    prec = get_precision(base);
    size = _scprintf(fmt, prec, v);

    /* TODO: don't use dynamic allocation */
    tmp = operator_new(size*2);
    num_put__fput(this, ret, dest, base, fill, tmp, sprintf(tmp, fmt, prec, v),
            numpunct_wchar_use_facet(IOS_LOCALE(base)));
    operator_delete(tmp);
    return ret;
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GN@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GN@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GO@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GO@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_double, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_double, 36)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_double(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, double v)
{
    char *tmp;
    char fmt[8]; /* strlen("%+#.*lg")+1 */
    int size;
    unsigned prec;

    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);

    num_put_wchar__Ffmt(this, fmt, '\0', base->fmtfl);
    prec = get_precision(base);
    size = _scprintf(fmt, prec, v);

    /* TODO: don't use dynamic allocation */
    tmp = operator_new(size*2);
    num_put__fput(this, ret, dest, base, fill, tmp, sprintf(tmp, fmt, prec, v),
            numpunct_short_use_facet(IOS_LOCALE(base)));
    operator_delete(tmp);
    return ret;
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WN@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WN@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GN@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GN@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_double, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_double, 36)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_double(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, double v)
{
    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);
    return call_num_put_wchar_do_put_double(this, ret, dest, base, fill, v);
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WO@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WO@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GO@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GO@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_ldouble, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_ldouble, 36)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_ldouble(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, double v)
{
    TRACE("(%p %p %p %d %lf)\n", this, ret, base, fill, v);
    return call_num_put_wchar_do_put_ldouble(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WPBX@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WPEBX@Z */
#if _MSVCP_VER <= 100
#define call_num_put_wchar_do_put_ptr(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 4, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, const void*), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_wchar_do_put_ptr(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 12, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, const void*), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_ptr, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_ptr, 32)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_ptr(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, const void *v)
{
    char tmp[17]; /* 8(16^8==2^64)*2(separators between every digit) + 1 */

    TRACE("(%p %p %p %d %p)\n", this, ret, base, fill, v);

    return num_put_wchar__Iput(this, ret, dest, base, fill, tmp, sprintf(tmp, "%p", v));
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GPBX@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GPEBX@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_ptr, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_ptr, 32)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_ptr(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, const void *v)
{
    char tmp[17]; /* 8(16^8==2^64)*2(separators between every digit) + 1 */

    TRACE("(%p %p %p %d %p)\n", this, ret, base, fill, v);

    return num_put_short__Iput(this, ret, dest, base, fill, tmp, sprintf(tmp, "%p", v));
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WPBX@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WPEBX@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GPBX@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GPEBX@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_ptr, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_ptr, 32)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_ptr(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, const void *v)
{
    TRACE("(%p %p %p %d %p)\n", this, ret, base, fill, v);
    return call_num_put_wchar_do_put_ptr(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_W_J@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_W_J@Z */
#if _MSVCP_VER <= 100
#define call_num_put_wchar_do_put_int64(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 20, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, __int64), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_wchar_do_put_int64(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 28, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, __int64), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_int64, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_int64, 36)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_int64(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, __int64 v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators between every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d)\n", this, ret, base, fill);

    return num_put_wchar__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "lld", base->fmtfl), v));
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@G_J@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@G_J@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_int64, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_int64, 36)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_int64(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, __int64 v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators between every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d)\n", this, ret, base, fill);

    return num_put_short__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "lld", base->fmtfl), v));
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_W_J@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_W_J@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@G_J@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@G_J@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_int64, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_int64, 36)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_int64(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, __int64 v)
{
    TRACE("(%p %p %p %d)\n", this, ret, base, fill);
    return call_num_put_wchar_do_put_int64(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_W_K@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_W_K@Z */
#if _MSVCP_VER <= 100
#define call_num_put_wchar_do_put_uint64(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 16, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, unsigned __int64), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_wchar_do_put_uint64(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 24, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, unsigned __int64), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_uint64, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_uint64, 36)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_uint64(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, unsigned __int64 v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators between every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d)\n", this, ret, base, fill);

    return num_put_wchar__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "llu", base->fmtfl), v));
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@G_K@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@G_K@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_uint64, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_uint64, 36)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_uint64(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, unsigned __int64 v)
{
    char tmp[48]; /* 22(8^22>2^64)*2(separators between every digit) + 3(strlen("+0x"))+1 */
    char fmt[7]; /* strlen("%+#lld")+1 */

    TRACE("(%p %p %p %d)\n", this, ret, base, fill);

    return num_put_short__Iput(this, ret, dest, base, fill, tmp,
            sprintf(tmp, num_put_wchar__Ifmt(this, fmt, "llu", base->fmtfl), v));
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_W_K@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_W_K@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@G_K@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@G_K@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_uint64, 32)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_uint64, 36)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_uint64(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, unsigned __int64 v)
{
    TRACE("(%p %p %p %d)\n", this, ret, base, fill);
    return call_num_put_wchar_do_put_uint64(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_W_N@Z */
/* ?do_put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_W_N@Z */
#if _MSVCP_VER <= 100
#define call_num_put_wchar_do_put_bool(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 32, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, bool), \
        (this, ret, dest, base, fill, v))
#else
#define call_num_put_wchar_do_put_bool(this, ret, dest, base, fill, v) CALL_VTBL_FUNC(this, 40, ostreambuf_iterator_wchar*, \
        (const num_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, bool), \
        (this, ret, dest, base, fill, v))
#endif
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_bool, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_do_put_bool, 32)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_do_put_bool(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, bool v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);

    if(base->fmtfl & FMTFLAG_boolalpha) {
        numpunct_wchar *numpunct = numpunct_wchar_use_facet(IOS_LOCALE(base));
        basic_string_wchar str;
        size_t pad, len;

        if(v)
            numpunct_wchar_truename(numpunct, &str);
        else
            numpunct_wchar_falsename(numpunct, &str);

        len = MSVCP_basic_string_wchar_length(&str);
        pad = (len>base->wide ? 0 : base->wide-len);
        base->wide = 0;

        if((base->fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            num_put_wchar__Rep(this, &dest, dest, fill, pad);
            pad = 0;
        }
        num_put_wchar__Put(this, &dest, dest, MSVCP_basic_string_wchar_c_str(&str), len);
        MSVCP_basic_string_wchar_dtor(&str);
        return num_put_wchar__Rep(this, ret, dest, fill, pad);
    }

    return num_put_wchar_put_long(this, ret, dest, base, fill, v);
}

/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@G_N@Z */
/* ?do_put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@G_N@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_bool, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_short_do_put_bool, 32)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_short_do_put_bool(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, bool v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);

    if(base->fmtfl & FMTFLAG_boolalpha) {
        numpunct_wchar *numpunct = numpunct_short_use_facet(IOS_LOCALE(base));
        basic_string_wchar str;
        size_t pad, len;

        if(v)
            numpunct_wchar_truename(numpunct, &str);
        else
            numpunct_wchar_falsename(numpunct, &str);

        len = MSVCP_basic_string_wchar_length(&str);
        pad = (len>base->wide ? 0 : base->wide-len);
        base->wide = 0;

        if((base->fmtfl & FMTFLAG_adjustfield) != FMTFLAG_left) {
            num_put_wchar__Rep(this, &dest, dest, fill, pad);
            pad = 0;
        }
        num_put_wchar__Put(this, &dest, dest, MSVCP_basic_string_wchar_c_str(&str), len);
        MSVCP_basic_string_wchar_dtor(&str);
        return num_put_wchar__Rep(this, ret, dest, fill, pad);
    }

    return num_put_wchar_put_long(this, ret, dest, base, fill, v);
}

/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_W_N@Z */
/* ?put@?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_W_N@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@G_N@Z */
/* ?put@?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@G_N@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_bool, 28)
#else
DEFINE_THISCALL_WRAPPER(num_put_wchar_put_bool, 32)
#endif
ostreambuf_iterator_wchar* __thiscall num_put_wchar_put_bool(const num_put *this, ostreambuf_iterator_wchar *ret,
        ostreambuf_iterator_wchar dest, ios_base *base, wchar_t fill, bool v)
{
    TRACE("(%p %p %p %d %d)\n", this, ret, base, fill, v);
    return call_num_put_wchar_do_put_bool(this, ret, dest, base, fill, v);
}

/* ?id@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@2V0locale@2@A */
locale_id time_put_char_id = {0};

/* ??_7?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@6B@ */
extern const vtable_ptr time_put_char_vtable;

/* ?_Init@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(time_put_char__Init, 8)
void __thiscall time_put_char__Init(time_put *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Gettnames(locinfo, &this->time);
#if _MSVCP_VER <= 100
    _Locinfo__Getcvt(locinfo, &this->cvt);
#endif
}

/* ??0?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(time_put_char_ctor_locinfo, 12)
time_put* __thiscall time_put_char_ctor_locinfo(time_put *this, const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &time_put_char_vtable;
    time_put_char__Init(this, locinfo);
    return this;
}

/* ??0?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@I@Z */
/* ??0?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(time_put_char_ctor_refs, 8)
time_put* __thiscall time_put_char_ctor_refs(time_put *this, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %Iu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    time_put_char_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??_F?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAEXXZ */
/* ??_F?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(time_put_char_ctor, 4)
time_put* __thiscall time_put_char_ctor(time_put *this)
{
    return time_put_char_ctor_refs(this, 0);
}

/* ??1?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MAE@XZ */
/* ??1?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(time_put_char_dtor, 4)
void __thiscall time_put_char_dtor(time_put *this)
{
    TRACE("(%p)\n", this);
    _Timevec_dtor(&this->time);
}

DEFINE_THISCALL_WRAPPER(time_put_char_vector_dtor, 8)
time_put* __thiscall time_put_char_vector_dtor(time_put *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            time_put_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        time_put_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl time_put_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = operator_new(sizeof(time_put));
        _Locinfo_ctor_cstr(&locinfo, locale_string_char_c_str(&loc->ptr->name));
        time_put_char_ctor_locinfo((time_put*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_TIME;
}

/* ?_Getcat@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl time_put_char__Getcat_old(const locale_facet **facet)
{
    return time_put_char__Getcat(facet, locale_classic());
}

static time_put* time_put_char_use_facet(const locale *loc)
{
    static time_put *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&time_put_char_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (time_put*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    time_put_char__Getcat(&fac, loc);
    obj = (time_put*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

#if _MSVCP_VER >= 70

/* ?do_put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DPBUtm@@DD@Z */
/* ?do_put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DPEBUtm@@DD@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(time_put_char_do_put, 36)
#else
DEFINE_THISCALL_WRAPPER(time_put_char_do_put, 40)
#endif
#if _MSVCP_VER <= 100
#define call_time_put_char_do_put(this, ret, dest, base, fill, t, spec, mod) CALL_VTBL_FUNC(this, 4, ostreambuf_iterator_char*, \
        (const time_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, const struct tm*, char, char), \
        (this, ret, dest, base, fill, t, spec, mod))
#else
#define call_time_put_char_do_put(this, ret, dest, base, fill, t, spec, mod) CALL_VTBL_FUNC(this, 12, ostreambuf_iterator_char*, \
        (const time_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, char, const struct tm*, char, char), \
        (this, ret, dest, base, fill, t, spec, mod))
#endif
ostreambuf_iterator_char* __thiscall time_put_char_do_put(const time_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, const struct tm *t, char spec, char mod)
{
    char buf[64], fmt[4], *p = fmt;
    size_t i, len;

    TRACE("(%p %p %p %c %p %c %c)\n", this, ret, base, fill, t, spec, mod);

    *p++ = '%';
    if(mod)
        *p++ = mod;
    *p++ = spec;
    *p++ = 0;

    len = _Strftime(buf, sizeof(buf), fmt, t, this->time.timeptr);
    for(i=0; i<len; i++)
        ostreambuf_iterator_char_put(&dest, buf[i]);

    *ret = dest;
    return ret;
}

/* ?put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DPBUtm@@DD@Z */
/* ?put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DPEBUtm@@DD@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(time_put_char_put, 36)
#else
DEFINE_THISCALL_WRAPPER(time_put_char_put, 40)
#endif
ostreambuf_iterator_char* __thiscall time_put_char_put(const time_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, const struct tm *t, char spec, char mod)
{
    TRACE("(%p %p %p %c %p %c %c)\n", this, ret, base, fill, t, spec, mod);
    return call_time_put_char_do_put(this, ret, dest, base, fill, t, spec, mod);
}

/* ?put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@DPBUtm@@PBD3@Z */
/* ?put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@DPEBUtm@@PEBD3@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(time_put_char_put_format, 36)
#else
DEFINE_THISCALL_WRAPPER(time_put_char_put_format, 40)
#endif
ostreambuf_iterator_char* __thiscall time_put_char_put_format(const time_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, char fill, const struct tm *t, const char *pat, const char *pat_end)
{
    TRACE("(%p %p %p %c %p %s)\n", this, ret, base, fill, t, debugstr_an(pat, pat_end-pat));

    while(pat < pat_end) {
        if(*pat != '%') {
            ostreambuf_iterator_char_put(&dest, *pat++);
        }else if(++pat == pat_end) {
            ostreambuf_iterator_char_put(&dest, '%');
        }else if(*pat=='#' && pat+1==pat_end) {
            ostreambuf_iterator_char_put(&dest, '%');
            ostreambuf_iterator_char_put(&dest, *pat++);
        }else {
            char mod;

            if(*pat == '#') {
                mod = '#';
                pat++;
            }else {
                mod = 0;
            }

            time_put_char_put(this, &dest, dest, base, fill, t, *pat++, mod);
        }
    }

    *ret = dest;
    return ret;
}

#else  /* _MSVCP_VER < 70 doesn't have the 'fill' parameter */

/* ?do_put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@PBUtm@@DD@Z */
/* ?do_put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@PEBUtm@@DD@Z */
DEFINE_THISCALL_WRAPPER(time_put_char_do_put, 32)
#define call_time_put_char_do_put(this, ret, dest, base, t, spec, mod) CALL_VTBL_FUNC(this, 4, ostreambuf_iterator_char*, \
        (const time_put*, ostreambuf_iterator_char*, ostreambuf_iterator_char, ios_base*, const struct tm*, char, char), \
        (this, ret, dest, base, t, spec, mod))
ostreambuf_iterator_char* __thiscall time_put_char_do_put(const time_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, const struct tm *t, char spec, char mod)
{
    char buf[64], fmt[4], *p = fmt;
    size_t i, len;

    TRACE("(%p %p %p %p %c %c)\n", this, ret, base, t, spec, mod);

    *p++ = '%';
    if(mod)
        *p++ = mod;
    *p++ = spec;
    *p++ = 0;

    len = _Strftime(buf, sizeof(buf), fmt, t, this->time.timeptr);
    for(i=0; i<len; i++)
        ostreambuf_iterator_char_put(&dest, buf[i]);

    *ret = dest;
    return ret;
}

/* ?put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@PBUtm@@DD@Z */
/* ?put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@PEBUtm@@DD@Z */
DEFINE_THISCALL_WRAPPER(time_put_char_put, 32)
ostreambuf_iterator_char* __thiscall time_put_char_put(const time_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, const struct tm *t, char spec, char mod)
{
    TRACE("(%p %p %p %p %c %c)\n", this, ret, base, t, spec, mod);
    return call_time_put_char_do_put(this, ret, dest, base, t, spec, mod);
}

/* ?put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AAVios_base@2@PBUtm@@PBD3@Z */
/* ?put@?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@DU?$char_traits@D@std@@@2@V32@AEAVios_base@2@PEBUtm@@PEBD3@Z */
DEFINE_THISCALL_WRAPPER(time_put_char_put_format, 32)
ostreambuf_iterator_char* __thiscall time_put_char_put_format(const time_put *this, ostreambuf_iterator_char *ret,
        ostreambuf_iterator_char dest, ios_base *base, const struct tm *t, const char *pat, const char *pat_end)
{
    TRACE("(%p %p %p %p %s)\n", this, ret, base, t, debugstr_an(pat, pat_end-pat));

    while(pat < pat_end) {
        if(*pat != '%') {
            ostreambuf_iterator_char_put(&dest, *pat++);
        }else if(++pat == pat_end) {
            ostreambuf_iterator_char_put(&dest, '%');
        }else if(*pat=='#' && pat+1==pat_end) {
            ostreambuf_iterator_char_put(&dest, '%');
            ostreambuf_iterator_char_put(&dest, *pat++);
        }else {
            char mod;

            if(*pat == '#') {
                mod = '#';
                pat++;
            }else {
                mod = 0;
            }

            time_put_char_put(this, &dest, dest, base, t, *pat++, mod);
        }
    }

    *ret = dest;
    return ret;
}

#endif  /* MSVCP_VER >= 70 */

/* ?id@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@2V0locale@2@A */
locale_id time_put_wchar_id = {0};
/* ?id@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@2V0locale@2@A */
locale_id time_put_short_id = {0};

/* ??_7?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@6B@ */
extern const vtable_ptr time_put_wchar_vtable;
/* ??_7?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@6B@ */
extern const vtable_ptr time_put_short_vtable;

/* ?_Init@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
/* ?_Init@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(time_put_wchar__Init, 8)
void __thiscall time_put_wchar__Init(time_put *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);
    _Locinfo__Gettnames(locinfo, &this->time);
#if _MSVCP_VER <= 100
    _Locinfo__Getcvt(locinfo, &this->cvt);
#endif
}

/* ??0?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(time_put_wchar_ctor_locinfo, 12)
time_put* __thiscall time_put_wchar_ctor_locinfo(time_put *this, const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &time_put_wchar_vtable;
    time_put_wchar__Init(this, locinfo);
    return this;
}

/* ??0?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(time_put_short_ctor_locinfo, 12)
time_put* __thiscall time_put_short_ctor_locinfo(time_put *this, const _Locinfo *locinfo, size_t refs)
{
    time_put_wchar_ctor_locinfo(this, locinfo, refs);
    this->facet.vtable = &time_put_short_vtable;
    return this;
}

/* ??0?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IAE@PBDI@Z */
/* ??0?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(time_put_wchar_ctor_name, 12)
time_put* __thiscall time_put_wchar_ctor_name(time_put *this, const char *name, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %s %Iu)\n", this, debugstr_a(name), refs);

    _Locinfo_ctor_cstr(&locinfo, name);
    time_put_wchar_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@IAE@PBDI@Z */
/* ??0?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@IEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(time_put_short_ctor_name, 12)
time_put* __thiscall time_put_short_ctor_name(time_put *this, const char *name, size_t refs)
{
    time_put_wchar_ctor_name(this, name, refs);
    this->facet.vtable = &time_put_short_vtable;
    return this;
}

/* ??0?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@I@Z */
/* ??0?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(time_put_wchar_ctor_refs, 8)
time_put* __thiscall time_put_wchar_ctor_refs(time_put *this, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %Iu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    time_put_wchar_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@I@Z */
/* ??0?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(time_put_short_ctor_refs, 8)
time_put* __thiscall time_put_short_ctor_refs(time_put *this, size_t refs)
{
    time_put_wchar_ctor_refs(this, refs);
    this->facet.vtable = &time_put_short_vtable;
    return this;
}

/* ??_F?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAEXXZ */
/* ??_F?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(time_put_wchar_ctor, 4)
time_put* __thiscall time_put_wchar_ctor(time_put *this)
{
    return time_put_wchar_ctor_refs(this, 0);
}

/* ??_F?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAEXXZ */
/* ??_F?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(time_put_short_ctor, 4)
time_put* __thiscall time_put_short_ctor(time_put *this)
{
    time_put_wchar_ctor(this);
    this->facet.vtable = &time_put_short_vtable;
    return this;
}

/* ??1?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MAE@XZ */
/* ??1?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEAA@XZ */
/* ??1?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MAE@XZ */
/* ??1?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(time_put_wchar_dtor, 4)
void __thiscall time_put_wchar_dtor(time_put *this)
{
    TRACE("(%p)\n", this);
    _Timevec_dtor(&this->time);
}

DEFINE_THISCALL_WRAPPER(time_put_wchar_vector_dtor, 8)
time_put* __thiscall time_put_wchar_vector_dtor(time_put *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            time_put_wchar_dtor(this+i);
        operator_delete(ptr);
    } else {
        time_put_wchar_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl time_put_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = operator_new(sizeof(time_put));
        time_put_wchar_ctor_name((time_put*)*facet,
                locale_string_char_c_str(&loc->ptr->name), 0);
    }

    return LC_TIME;
}

/* ?_Getcat@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl time_put_wchar__Getcat_old(const locale_facet **facet)
{
    return time_put_wchar__Getcat(facet, locale_classic());
}

static time_put* time_put_wchar_use_facet(const locale *loc)
{
    static time_put *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&time_put_wchar_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (time_put*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    time_put_wchar__Getcat(&fac, loc);
    obj = (time_put*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Getcat@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
size_t __cdecl time_put_short__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        *facet = operator_new(sizeof(time_put));
        time_put_short_ctor_name((time_put*)*facet,
                locale_string_char_c_str(&loc->ptr->name), 0);
    }

    return LC_TIME;
}

/* ?_Getcat@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SAIPAPBVfacet@locale@2@@Z */
/* ?_Getcat@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@@Z */
size_t __cdecl time_put_short__Getcat_old(const locale_facet **facet)
{
    return time_put_short__Getcat(facet, locale_classic());
}

static time_put* time_put_short_use_facet(const locale *loc)
{
    static time_put *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&time_put_short_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (time_put*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    time_put_short__Getcat(&fac, loc);
    obj = (time_put*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

#if _MSVCP_VER >= 70

/* ?do_put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GPBUtm@@DD@Z */
/* ?do_put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GPEBUtm@@DD@Z */
/* ?do_put@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WPBUtm@@DD@Z */
/* ?do_put@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WPEBUtm@@DD@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(time_put_wchar_do_put, 36)
#else
DEFINE_THISCALL_WRAPPER(time_put_wchar_do_put, 40)
#endif
#if _MSVCP_VER <= 100
#define call_time_put_wchar_do_put(this, ret, dest, base, fill, t, spec, mod) CALL_VTBL_FUNC(this, 4, ostreambuf_iterator_wchar*, \
        (const time_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, const struct tm*, char, char), \
        (this, ret, dest, base, fill, t, spec, mod))
#else
#define call_time_put_wchar_do_put(this, ret, dest, base, fill, t, spec, mod) CALL_VTBL_FUNC(this, 12, ostreambuf_iterator_wchar*, \
        (const time_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, wchar_t, const struct tm*, char, char), \
        (this, ret, dest, base, fill, t, spec, mod))
#endif
ostreambuf_iterator_wchar* __thiscall time_put_wchar_do_put(const time_put *this,
        ostreambuf_iterator_wchar *ret, ostreambuf_iterator_wchar dest, ios_base *base,
        wchar_t fill, const struct tm *t, char spec, char mod)
{
    char buf[64], fmt[4], *p = fmt;
    size_t i, len;
    const _Cvtvec *cvt;
    wchar_t c;

    TRACE("(%p %p %p %c %p %c %c)\n", this, ret, base, fill, t, spec, mod);

    *p++ = '%';
    if(mod)
        *p++ = mod;
    *p++ = spec;
    *p++ = 0;

#if _MSVCP_VER <= 100
    cvt = &this->cvt;
#else
    cvt = &ctype_wchar_use_facet(base->loc)->cvt;
#endif

    len = _Strftime(buf, sizeof(buf), fmt, t, this->time.timeptr);
    for(i=0; i<len; i++) {
        c = mb_to_wc(buf[i], cvt);
        ostreambuf_iterator_wchar_put(&dest, c);
    }

    *ret = dest;
    return ret;
}

/* ?put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GPBUtm@@DD@Z */
/* ?put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GPEBUtm@@DD@Z */
/* ?put@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WPBUtm@@DD@Z */
/* ?put@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WPEBUtm@@DD@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(time_put_wchar_put, 36)
#else
DEFINE_THISCALL_WRAPPER(time_put_wchar_put, 40)
#endif
ostreambuf_iterator_wchar* __thiscall time_put_wchar_put(const time_put *this,
        ostreambuf_iterator_wchar *ret, ostreambuf_iterator_wchar dest, ios_base *base,
        wchar_t fill, const struct tm *t, char spec, char mod)
{
    TRACE("(%p %p %p %c %p %c %c)\n", this, ret, base, fill, t, spec, mod);
    return call_time_put_wchar_do_put(this, ret, dest, base, fill, t, spec, mod);
}

/* ?put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@GPBUtm@@PBG3@Z */
/* ?put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@GPEBUtm@@PEBG3@Z */
/* ?put@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AAVios_base@2@_WPBUtm@@PB_W4@Z */
/* ?put@?$time_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@AEAVios_base@2@_WPEBUtm@@PEB_W4@Z */
#if _MSVCP_VER != 80
DEFINE_THISCALL_WRAPPER(time_put_wchar_put_format, 36)
#else
DEFINE_THISCALL_WRAPPER(time_put_wchar_put_format, 40)
#endif
ostreambuf_iterator_wchar* __thiscall time_put_wchar_put_format(const time_put *this,
        ostreambuf_iterator_wchar *ret, ostreambuf_iterator_wchar dest, ios_base *base,
        wchar_t fill, const struct tm *t, const wchar_t *pat, const wchar_t *pat_end)
{
    wchar_t percent;
    char c[MB_LEN_MAX];
    const _Cvtvec *cvt;

    TRACE("(%p %p %p %c %p %s)\n", this, ret, base, fill, t, debugstr_wn(pat, pat_end-pat));

#if _MSVCP_VER <= 100
    cvt = &this->cvt;
#else
    cvt = &ctype_wchar_use_facet(base->loc)->cvt;
#endif

    percent = mb_to_wc('%', cvt);
    while(pat < pat_end) {
        if(*pat != percent) {
            ostreambuf_iterator_wchar_put(&dest, *pat++);
        }else if(++pat == pat_end) {
            ostreambuf_iterator_wchar_put(&dest, percent);
        }else if(_Wcrtomb(c, *pat, NULL, cvt)!=1 || (*c=='#' && pat+1==pat_end)) {
            ostreambuf_iterator_wchar_put(&dest, percent);
            ostreambuf_iterator_wchar_put(&dest, *pat++);
        }else {
            pat++;
            if(*c == '#') {
                if(_Wcrtomb(c, *pat++, NULL, cvt) != 1) {
                    ostreambuf_iterator_wchar_put(&dest, percent);
                    ostreambuf_iterator_wchar_put(&dest, *(pat-2));
                    ostreambuf_iterator_wchar_put(&dest, *(pat-1));
                }else {
                    time_put_wchar_put(this, &dest, dest, base, fill, t, *c, '#');
                }
            }else {
                time_put_wchar_put(this, &dest, dest, base, fill, t, *c, 0);
            }
        }
    }

    *ret = dest;
    return ret;
}

#else  /* _MSVCP_VER < 70 doesn't have the 'fill' parameter */

/* ?do_put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@PBUtm@@DD@Z */
/* ?do_put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@MEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@PEBUtm@@DD@Z */
DEFINE_THISCALL_WRAPPER(time_put_wchar_do_put, 32)
#define call_time_put_wchar_do_put(this, ret, dest, base, t, spec, mod) CALL_VTBL_FUNC(this, 4, ostreambuf_iterator_wchar*, \
        (const time_put*, ostreambuf_iterator_wchar*, ostreambuf_iterator_wchar, ios_base*, const struct tm*, char, char), \
        (this, ret, dest, base, t, spec, mod))
ostreambuf_iterator_wchar* __thiscall time_put_wchar_do_put(const time_put *this,
        ostreambuf_iterator_wchar *ret, ostreambuf_iterator_wchar dest, ios_base *base,
        const struct tm *t, char spec, char mod)
{
    char buf[64], fmt[4], *p = fmt;
    size_t i, len;
    wchar_t c;

    TRACE("(%p %p %p %p %c %c)\n", this, ret, base, t, spec, mod);

    *p++ = '%';
    if(mod)
        *p++ = mod;
    *p++ = spec;
    *p++ = 0;

    len = _Strftime(buf, sizeof(buf), fmt, t, this->time.timeptr);
    for(i=0; i<len; i++) {
        c = mb_to_wc(buf[i], &this->cvt);
        ostreambuf_iterator_wchar_put(&dest, c);
    }

    *ret = dest;
    return ret;
}

/* ?put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@PBUtm@@DD@Z */
/* ?put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@PEBUtm@@DD@Z */
DEFINE_THISCALL_WRAPPER(time_put_wchar_put, 32)
ostreambuf_iterator_wchar* __thiscall time_put_wchar_put(const time_put *this,
        ostreambuf_iterator_wchar *ret, ostreambuf_iterator_wchar dest, ios_base *base,
        const struct tm *t, char spec, char mod)
{
    TRACE("(%p %p %p %p %c %c)\n", this, ret, base, t, spec, mod);
    return call_time_put_wchar_do_put(this, ret, dest, base, t, spec, mod);
}

/* ?put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QBE?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AAVios_base@2@PBUtm@@PBG3@Z */
/* ?put@?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QEBA?AV?$ostreambuf_iterator@GU?$char_traits@G@std@@@2@V32@AEAVios_base@2@PEBUtm@@PEBG3@Z */
DEFINE_THISCALL_WRAPPER(time_put_wchar_put_format, 32)
ostreambuf_iterator_wchar* __thiscall time_put_wchar_put_format(const time_put *this,
        ostreambuf_iterator_wchar *ret, ostreambuf_iterator_wchar dest, ios_base *base,
        const struct tm *t, const wchar_t *pat, const wchar_t *pat_end)
{
    wchar_t percent = mb_to_wc('%', &this->cvt);
    char c[MB_LEN_MAX];

    TRACE("(%p %p %p %p %s)\n", this, ret, base, t, debugstr_wn(pat, pat_end-pat));

    while(pat < pat_end) {
        if(*pat != percent) {
            ostreambuf_iterator_wchar_put(&dest, *pat++);
        }else if(++pat == pat_end) {
            ostreambuf_iterator_wchar_put(&dest, percent);
        }else if(_Wcrtomb(c, *pat, NULL, &this->cvt)!=1 || (*c=='#' && pat+1==pat_end)) {
            ostreambuf_iterator_wchar_put(&dest, percent);
            ostreambuf_iterator_wchar_put(&dest, *pat++);
        }else {
            pat++;
            if(*c == '#') {
                if(_Wcrtomb(c, *pat++, NULL, &this->cvt) != 1) {
                    ostreambuf_iterator_wchar_put(&dest, percent);
                    ostreambuf_iterator_wchar_put(&dest, *(pat-2));
                    ostreambuf_iterator_wchar_put(&dest, *(pat-1));
                }else {
                    time_put_wchar_put(this, &dest, dest, base, t, *c, '#');
                }
            }else {
                time_put_wchar_put(this, &dest, dest, base, t, *c, 0);
            }
        }
    }

    *ret = dest;
    return ret;
}

#endif  /* _MSVCP_VER >= 70 */

/* ?id@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@2V0locale@2@A */
locale_id time_get_char_id = {0};

/* ??_7?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@6B@ */
extern const vtable_ptr time_get_char_vtable;

/* ?_Init@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(time_get_char__Init, 8)
void __thiscall time_get_char__Init(time_get_char *this, const _Locinfo *locinfo)
{
    const char *months;
    const char *days;
    int len;

    TRACE("(%p %p)\n", this, locinfo);

    days = _Locinfo__Getdays(locinfo);
    len = strlen(days)+1;
    this->days = operator_new(len);
    memcpy((char*)this->days, days, len);

    months = _Locinfo__Getmonths(locinfo);
    len = strlen(months)+1;
    this->months = operator_new(len);
    memcpy((char*)this->months, months, len);

    this->dateorder = _Locinfo__Getdateorder(locinfo);
    _Locinfo__Getcvt(locinfo, &this->cvt);
}

/* ??0?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_ctor_locinfo, 12)
time_get_char* __thiscall time_get_char_ctor_locinfo(time_get_char *this,
        const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &time_get_char_vtable;
    time_get_char__Init(this, locinfo);
    return this;
}

/* ??0?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IAE@PBDI@Z */
/* ??0?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_ctor_name, 12)
time_get_char* __thiscall time_get_char_ctor_name(time_get_char *this, const char *name, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %s %Iu)\n", this, name, refs);

    _Locinfo_ctor_cstr(&locinfo, name);
    time_get_char_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@I@Z */
/* ??0?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_ctor_refs, 8)
time_get_char* __thiscall time_get_char_ctor_refs(time_get_char *this, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %Iu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    time_get_char_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??_F?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAEXXZ */
/* ??_F?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(time_get_char_ctor, 4)
time_get_char* __thiscall time_get_char_ctor(time_get_char *this)
{
    return time_get_char_ctor_refs(this, 0);
}

/* ?_Tidy@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AAEXXZ */
/* ?_Tidy@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(time_get_char__Tidy, 4)
void __thiscall time_get_char__Tidy(time_get_char *this)
{
    TRACE("(%p)\n", this);

    operator_delete((char*)this->days);
    operator_delete((char*)this->months);
}

/* ??1?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MAE@XZ */
/* ??1?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(time_get_char_dtor, 4) /* virtual */
void __thiscall time_get_char_dtor(time_get_char *this)
{
    TRACE("(%p)\n", this);

    time_get_char__Tidy(this);
}

DEFINE_THISCALL_WRAPPER(time_get_char_vector_dtor, 8)
time_get_char* __thiscall time_get_char_vector_dtor(time_get_char *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            time_get_char_dtor(this+i);
        operator_delete(ptr);
    } else {
        time_get_char_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
unsigned int __cdecl time_get_char__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = operator_new(sizeof(time_get_char));
        _Locinfo_ctor_cstr(&locinfo, locale_string_char_c_str(&loc->ptr->name));
        time_get_char_ctor_locinfo((time_get_char*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_TIME;
}

static time_get_char* time_get_char_use_facet(const locale *loc)
{
    static time_get_char *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&time_get_char_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (time_get_char*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    time_get_char__Getcat(&fac, loc);
    obj = (time_get_char*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Getint@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@ABAHAAV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@0HHAAH@Z */
/* ?_Getint@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@AEBAHAEAV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@0HHAEAH@Z */
int __cdecl time_get_char__Getint(const time_get_char *this,
        istreambuf_iterator_char *b, istreambuf_iterator_char *e,
        int min_val, int max_val, int *val)
{
    BOOL got_digit = FALSE;
    int len = 0, ret = 0;
    char buf[16];

    TRACE("(%p %p %p %d %d %p)\n", this, b, e, min_val, max_val, val);

    istreambuf_iterator_char_val(b);
    if(b->strbuf && (b->val == '-' || b->val == '+'))
    {
        buf[len++] = b->val;
        istreambuf_iterator_char_inc(b);
    }

    if (b->strbuf && b->val == '0')
    {
        got_digit = TRUE;
        buf[len++] = '0';
        istreambuf_iterator_char_inc(b);
    }
    while (b->strbuf && b->val == '0')
        istreambuf_iterator_char_inc(b);

    for (; b->strbuf && b->val >= '0' && b->val <= '9';
            istreambuf_iterator_char_inc(b))
    {
        if(len < ARRAY_SIZE(buf)-1)
            buf[len] = b->val;
        got_digit = TRUE;
        len++;
    }

    if (!b->strbuf)
        ret |= IOSTATE_eofbit;
    if (got_digit && len < ARRAY_SIZE(buf)-1)
    {
        int v, err;

        buf[len] = 0;
        v = _Stolx(buf, NULL, 10, &err);
        if(err || v < min_val || v > max_val)
            ret |= IOSTATE_failbit;
        else
            *val = v;
    }
    else
        ret |= IOSTATE_failbit;
    return ret;
}

/* ?do_date_order@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AW4dateorder@time_base@2@XZ */
/* ?do_date_order@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AW4dateorder@time_base@2@XZ */
DEFINE_THISCALL_WRAPPER(time_get_char_do_date_order, 4) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_char_do_date_order(this) CALL_VTBL_FUNC(this, 4, dateorder, (const time_get_char*), (this))
#else
#define call_time_get_char_do_date_order(this) CALL_VTBL_FUNC(this, 12, dateorder, (const time_get_char*), (this))
#endif
dateorder __thiscall time_get_char_do_date_order(const time_get_char *this)
{
    TRACE("(%p)\n", this);
    return this->dateorder;
}

/* ?date_order@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AW4dateorder@time_base@2@XZ */
/* ?date_order@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AW4dateorder@time_base@2@XZ */
DEFINE_THISCALL_WRAPPER(time_get_char_date_order, 4)
dateorder __thiscall time_get_char_date_order(const time_get_char *this)
{
    return call_time_get_char_do_date_order(this);
}

static int find_longest_match_char(istreambuf_iterator_char *iter, const char *str)
{
    int i, len = 0, last_match = -1, match = -1;
    const char *p, *end;
    char buf[64];

    for(istreambuf_iterator_char_val(iter); iter->strbuf && len<ARRAY_SIZE(buf);
            istreambuf_iterator_char_inc(iter))
    {
        BOOL got_prefix = FALSE;

        buf[len++] = iter->val;
        last_match = match;
        match = -1;
        for(p=str+1, i=0; *p; p = (*end ? end+1 : end), i++)
        {
            end = strchr(p, ':');
            if (!end)
                end = p + strlen(p);

            if (end-p >= len && !memcmp(p, buf, len))
            {
                if (end-p == len)
                    match = i;
                else
                    got_prefix = TRUE;
            }
        }

        if (!got_prefix)
        {
            if (match != -1)
            {
                istreambuf_iterator_char_inc(iter);
                return match;
            }
            break;
        }
    }
    if (len == ARRAY_SIZE(buf))
        FIXME("temporary buffer is too small\n");
    if (!iter->strbuf)
        return match;
    return last_match;
}

/* ?do_get_monthname@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?do_get_monthname@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_do_get_monthname, 36) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_char_do_get_monthname(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 20, istreambuf_iterator_char*, \
        (const time_get_char*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#else
#define call_time_get_char_do_get_monthname(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 28, istreambuf_iterator_char*, \
        (const time_get_char*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#endif
istreambuf_iterator_char* __thiscall time_get_char_do_get_monthname(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t)
{
    int match;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, err, t);

    if ((match = find_longest_match_char(&s, this->months)) != -1)
        t->tm_mon = match / 2;
    else
        *err |= IOSTATE_failbit;

    *ret = s;
    return ret;
}

/* ?get_monthname@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?get_monthname@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_get_monthname, 36)
istreambuf_iterator_char* __thiscall time_get_char_get_monthname(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t)
{
    return call_time_get_char_do_get_monthname(this, ret, s, e, base, err, t);
}

/* ?do_get_time@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?do_get_time@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_do_get_time, 36) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_char_do_get_time(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 8, istreambuf_iterator_char*, \
        (const time_get_char*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#else
#define call_time_get_char_do_get_time(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 16, istreambuf_iterator_char*, \
        (const time_get_char*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#endif
istreambuf_iterator_char* __thiscall time_get_char_do_get_time(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, err, t);

    *err |= time_get_char__Getint(this, &s, &e, 0, 23, &t->tm_hour);
    if (*err || istreambuf_iterator_char_val(&s)!=':')
        *err |= IOSTATE_failbit;

    if (!*err)
    {
        istreambuf_iterator_char_inc(&s);
        *err |= time_get_char__Getint(this, &s, &e, 0, 59, &t->tm_min);
    }
    if (*err || istreambuf_iterator_char_val(&s)!=':')
        *err |= IOSTATE_failbit;

    if (!*err)
    {
        istreambuf_iterator_char_inc(&s);
        *err |= time_get_char__Getint(this, &s, &e, 0, 59, &t->tm_sec);
    }

    *ret = s;
    return ret;
}

/* ?get_time@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?get_time@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_get_time, 36)
istreambuf_iterator_char* __thiscall time_get_char_get_time(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t)
{
    return call_time_get_char_do_get_time(this, ret, s, e, base, err, t);
}

/* ?do_get_weekday@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?do_get_weekday@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_do_get_weekday, 36) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_char_do_get_weekday(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 16, istreambuf_iterator_char*, \
        (const time_get_char*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#else
#define call_time_get_char_do_get_weekday(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 24, istreambuf_iterator_char*, \
        (const time_get_char*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#endif
istreambuf_iterator_char* __thiscall time_get_char_do_get_weekday(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t)
{
    int match;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, err, t);

    if ((match = find_longest_match_char(&s, this->days)) != -1)
        t->tm_wday = match / 2;
    else
        *err |= IOSTATE_failbit;

    *ret = s;
    return ret;
}

/* ?get_weekday@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?get_weekday@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_get_weekday, 36)
istreambuf_iterator_char* __thiscall time_get_char_get_weekday(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t)
{
    return call_time_get_char_do_get_weekday(this, ret, s, e, base, err, t);
}

/* ?do_get_year@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?do_get_year@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_do_get_year, 36) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_char_do_get_year(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 24, istreambuf_iterator_char*, \
        (const time_get_char*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#else
#define call_time_get_char_do_get_year(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 32, istreambuf_iterator_char*, \
        (const time_get_char*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#endif
istreambuf_iterator_char* __thiscall time_get_char_do_get_year(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t)
{
    int year;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, err, t);

    /* The function supports only dates from [1900-2035] range */
    *err |= time_get_char__Getint(this, &s, &e, 0, 2035, &year);
    if (!(*err & IOSTATE_failbit))
    {
        if (year >= 1900)
            year -= 1900;
        if (year > 135)
            *err |= IOSTATE_failbit;
        else
            t->tm_year = year;
    }

    *ret = s;
    return ret;
}

/* ?get_year@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?get_year@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_get_year, 36)
istreambuf_iterator_char* __thiscall time_get_char_get_year(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t)
{
    return call_time_get_char_do_get_year(this, ret, s, e, base, err, t);
}

static void skip_ws_char(ctype_char *ctype, istreambuf_iterator_char *iter)
{
    istreambuf_iterator_char_val(iter);
    while(iter->strbuf && ctype_char_is_ch(ctype, _SPACE, iter->val))
        istreambuf_iterator_char_inc(iter);
}

static void skip_date_delim_char(ctype_char *ctype, istreambuf_iterator_char *iter)
{
    skip_ws_char(ctype, iter);
    if(iter->strbuf && (iter->val == '/' || iter->val == ':'))
        istreambuf_iterator_char_inc(iter);
    skip_ws_char(ctype, iter);
}

/* ?do_get_date@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?do_get_date@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_do_get_date, 36) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_char_do_get_date(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 12, istreambuf_iterator_char*, \
        (const time_get_char*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#else
#define call_time_get_char_do_get_date(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 20, istreambuf_iterator_char*, \
        (const time_get_char*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#endif
istreambuf_iterator_char* __thiscall time_get_char_do_get_date(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t)
{
    ctype_char *ctype;
    dateorder order;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, err, t);

    ctype = ctype_char_use_facet(IOS_LOCALE(base));

    order = time_get_char_date_order(this);
    if(order == DATEORDER_no_order)
        order = DATEORDER_mdy;

    switch(order) {
    case DATEORDER_dmy:
        *err |= time_get_char__Getint(this, &s, &e, 1, 31, &t->tm_mday);
        skip_date_delim_char(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        if(s.strbuf && ctype_char_is_ch(ctype, _DIGIT, s.val)) {
            *err |= time_get_char__Getint(this, &s, &e, 1, 12, &t->tm_mon);
            t->tm_mon--;
        } else {
            time_get_char_get_monthname(this, &s, s, e, base, err, t);
        }
        skip_date_delim_char(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        time_get_char_get_year(this, &s, s, e, base, err, t);
        break;
    case DATEORDER_mdy:
        istreambuf_iterator_char_val(&s);
        if(s.strbuf && ctype_char_is_ch(ctype, _DIGIT, s.val)) {
            *err |= time_get_char__Getint(this, &s, &e, 1, 12, &t->tm_mon);
            t->tm_mon--;
        } else {
            time_get_char_get_monthname(this, &s, s, e, base, err, t);
        }
        skip_date_delim_char(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        *err |= time_get_char__Getint(this, &s, &e, 1, 31, &t->tm_mday);
        skip_date_delim_char(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        time_get_char_get_year(this, &s, s, e, base, err, t);
        break;
    case DATEORDER_ymd:
        time_get_char_get_year(this, &s, s, e, base, err, t);
        skip_date_delim_char(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        if(ctype_char_is_ch(ctype, _DIGIT, s.val)) {
            *err |= time_get_char__Getint(this, &s, &e, 1, 12, &t->tm_mon);
            t->tm_mon--;
        } else {
            time_get_char_get_monthname(this, &s, s, e, base, err, t);
        }
        skip_date_delim_char(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        *err |= time_get_char__Getint(this, &s, &e, 1, 31, &t->tm_mday);
        break;
    case DATEORDER_ydm:
        time_get_char_get_year(this, &s, s, e, base, err, t);
        skip_date_delim_char(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        *err |= time_get_char__Getint(this, &s, &e, 1, 31, &t->tm_mday);
        skip_date_delim_char(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        if(ctype_char_is_ch(ctype, _DIGIT, s.val)) {
            *err |= time_get_char__Getint(this, &s, &e, 1, 12, &t->tm_mon);
            t->tm_mon--;
        } else {
            time_get_char_get_monthname(this, &s, s, e, base, err, t);
        }
        break;
    default:
        ERR("incorrect order value: %d\n", order);
        break;
    }

    if(!s.strbuf)
        *err |= IOSTATE_eofbit;
    *ret = s;
    return ret;
}

/* ?get_date@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?get_date@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_get_date, 36)
istreambuf_iterator_char* __thiscall time_get_char_get_date(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t)
{
    return call_time_get_char_do_get_date(this, ret, s, e, base, err, t);
}

istreambuf_iterator_char* __thiscall time_get_char_get(const time_get_char*,
        istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char,
        ios_base*, int*, struct tm*, char, char);

/* ?_Getfmt@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@PBD@Z */
/* ?_Getfmt@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@IEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@PEBD@Z */
DEFINE_THISCALL_WRAPPER(time_get_char__Getfmt, 40)
istreambuf_iterator_char* __thiscall time_get_char__Getfmt(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t, const char *fmt)
{
    ctype_char *ctype;

    TRACE("(%p %p %p %p %p %s)\n", this, ret, base, err, t, fmt);

    ctype = ctype_char_use_facet(IOS_LOCALE(base));
    istreambuf_iterator_char_val(&s);

    while(*fmt) {
        if(ctype_char_is_ch(ctype, _SPACE, *fmt)) {
            skip_ws_char(ctype, &s);
            fmt++;
            continue;
        }

        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }

        if(*fmt == '%') {
            fmt++;
            time_get_char_get(this, &s, s, e, base, err, t, *fmt, 0);
        } else {
            if(s.val != *fmt)
                *err |= IOSTATE_failbit;
            else
                istreambuf_iterator_char_inc(&s);
        }

        if(*err & IOSTATE_failbit)
            break;
        fmt++;
    }

    if(!s.strbuf)
        *err |= IOSTATE_eofbit;
    *ret = s;
    return ret;
}

/* ?do_get@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@DD@Z */
/* ?do_get@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@DD@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_do_get, 44) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_char_do_get(this, ret, s, e, base, err, t, fmt, mod) CALL_VTBL_FUNC(this, 28, istreambuf_iterator_char*, \
        (const time_get_char*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, struct tm*, char, char), \
        (this, ret, s, e, base, err, t, fmt, mod))
#else
#define call_time_get_char_do_get(this, ret, s, e, base, err, t, fmt, mod) CALL_VTBL_FUNC(this, 36, istreambuf_iterator_char*, \
        (const time_get_char*, istreambuf_iterator_char*, istreambuf_iterator_char, istreambuf_iterator_char, ios_base*, int*, struct tm*, char, char), \
        (this, ret, s, e, base, err, t, fmt, mod))
#endif
istreambuf_iterator_char* __thiscall time_get_char_do_get(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t, char fmt, char mod)
{
    ctype_char *ctype;

    TRACE("(%p %p %p %p %p %c %c)\n", this, ret, base, err, t, fmt, mod);

    ctype = ctype_char_use_facet(IOS_LOCALE(base));

    switch(fmt) {
    case 'a':
    case 'A':
        time_get_char_get_weekday(this, &s, s, e, base, err, t);
        break;
    case 'b':
    case 'B':
    case 'h':
        time_get_char_get_monthname(this, &s, s, e, base, err, t);
        break;
    case 'c':
        time_get_char__Getfmt(this, &s, s, e, base, err, t, "%b %d %H:%M:%S %Y");
        break;
    case 'C':
        *err |= time_get_char__Getint(this, &s, &e, 0, 99, &t->tm_year);
        if(!(*err & IOSTATE_failbit))
            t->tm_year = t->tm_year * 100 - 1900;
        break;
    case 'd':
    case 'e':
        if(fmt == 'e') skip_ws_char(ctype, &s);
        *err |= time_get_char__Getint(this, &s, &e, 1, 31, &t->tm_mday);
        break;
    case 'D':
        time_get_char__Getfmt(this, &s, s, e, base, err, t, "%m/%d/%y");
        break;
    case 'F':
        time_get_char__Getfmt(this, &s, s, e, base, err, t, "%Y-%m-%d");
        break;
    case 'H':
        *err |= time_get_char__Getint(this, &s, &e, 0, 23, &t->tm_hour);
        break;
    case 'I':
        *err |= time_get_char__Getint(this, &s, &e, 0, 11, &t->tm_hour);
        break;
    case 'j':
        *err |= time_get_char__Getint(this, &s, &e, 1, 366, &t->tm_yday);
        break;
    case 'm':
        *err |= time_get_char__Getint(this, &s, &e, 1, 12, &t->tm_mon);
        if(!(*err & IOSTATE_failbit))
            t->tm_mon--;
        break;
    case 'M':
        *err = time_get_char__Getint(this, &s, &e, 0, 59, &t->tm_min);
        break;
    case 'n':
    case 't':
        skip_ws_char(ctype, &s);
        break;
    case 'p': {
        BOOL pm = FALSE;

        istreambuf_iterator_char_val(&s);
        if(s.strbuf && (s.val=='P' || s.val=='p'))
            pm = TRUE;
        else if (!s.strbuf || (s.val!='A' && s.val!='a')) {
            *err |= IOSTATE_failbit;
            break;
        }
        istreambuf_iterator_char_inc(&s);
        if(!s.strbuf || (s.val!='M' && s.val!='m')) {
            *err |= IOSTATE_failbit;
            break;
        }
        istreambuf_iterator_char_inc(&s);

        if(pm)
            t->tm_hour += 12;
        break;
    }
    case 'r':
        time_get_char__Getfmt(this, &s, s, e, base, err, t, "%I:%M:%S %p");
        break;
    case 'R':
        time_get_char__Getfmt(this, &s, s, e, base, err, t, "%H:%M");
        break;
    case 'S':
        *err |= time_get_char__Getint(this, &s, &e, 0, 59, &t->tm_sec);
        break;
    case 'T':
    case 'X':
        time_get_char__Getfmt(this, &s, s, e, base, err, t, "%H:%M:%S");
        break;
    case 'u':
        *err |= time_get_char__Getint(this, &s, &e, 1, 7, &t->tm_wday);
        if(!(*err & IOSTATE_failbit) && t->tm_wday==7)
            t->tm_wday = 0;
        break;
    case 'w':
        *err |= time_get_char__Getint(this, &s, &e, 0, 6, &t->tm_wday);
        break;
    case 'x':
        time_get_char_get_date(this, &s, s, e, base, err, t);
        break;
    case 'y':
        *err |= time_get_char__Getint(this, &s, &e, 0, 99, &t->tm_year);
        if(!(*err & IOSTATE_failbit) && t->tm_year<69)
            t->tm_year += 100;
        break;
    case 'Y':
        time_get_char_get_year(this, &s, s, e, base, err, t);
        break;
    default:
        FIXME("unrecognized format: %c\n", fmt);
        *err |= IOSTATE_failbit;
    }

    if(!s.strbuf)
        *err |= IOSTATE_eofbit;
    *ret = s;
    return ret;
}

/* ?get@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@DD@Z */
/* ?get@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@DD@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_get, 44)
istreambuf_iterator_char* __thiscall time_get_char_get(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t, char fmt, char mod)
{
    return call_time_get_char_do_get(this, ret, s, e, base, err, t, fmt, mod);
}

/* ?get@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@PBD4@Z */
/* ?get@?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@PEBD4@Z */
DEFINE_THISCALL_WRAPPER(time_get_char_get_fmt, 44)
istreambuf_iterator_char* __thiscall time_get_char_get_fmt(const time_get_char *this,
        istreambuf_iterator_char *ret, istreambuf_iterator_char s, istreambuf_iterator_char e,
        ios_base *base, int *err, struct tm *t, const char *fmtstart, const char *fmtend)
{
    ctype_char *ctype;

    TRACE("(%p %p %p %p %p %s)\n", this, ret, base, err, t, wine_dbgstr_an(fmtstart, fmtend-fmtstart));

    ctype = ctype_char_use_facet(IOS_LOCALE(base));
    istreambuf_iterator_char_val(&s);

    while(fmtstart < fmtend) {
        if(ctype_char_is_ch(ctype, _SPACE, *fmtstart)) {
            skip_ws_char(ctype, &s);
            fmtstart++;
            continue;
        }

        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }

        if(*fmtstart != '%' || fmtstart+1 >= fmtend || fmtstart[1] == '%') {
            if(s.val != *fmtstart)
                *err |= IOSTATE_failbit;
            else
                istreambuf_iterator_char_inc(&s);
            if(*fmtstart == '%')
                fmtstart++;
        } else {
            fmtstart++;
            time_get_char_get(this, &s, s, e, base, err, t, *fmtstart, 0);
        }

        if(*err & IOSTATE_failbit)
            break;
        fmtstart++;
    }

    if(!s.strbuf)
        *err |= IOSTATE_eofbit;
    *ret = s;
    return ret;
}

/* ?id@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@2V0locale@2@A */
locale_id time_get_wchar_id = {0};

/* ??_7?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@6B@ */
extern const vtable_ptr time_get_wchar_vtable;

#if _MSVCP_VER >=110
static wchar_t* create_time_get_str(const wchar_t *str)
{
    wchar_t *ret;
    int len;

    len = lstrlenW(str)+1;
    ret = operator_new(len * sizeof(wchar_t));
    memcpy(ret, str, len*sizeof(wchar_t));
    return ret;
}
#else
static wchar_t* create_time_get_str(const char *str, const _Locinfo *locinfo)
{
    wchar_t *ret;
    _Cvtvec cvt;
    int len;

    _Locinfo__Getcvt(locinfo, &cvt);
    len = MultiByteToWideChar(cvt.page, 0, str, -1, NULL, 0);
    ret = operator_new(len*sizeof(WCHAR));
    MultiByteToWideChar(cvt.page, 0, str, -1, ret, len);
    return ret;
}
#endif

/* ?_Init@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IAEXABV_Locinfo@2@@Z */
/* ?_Init@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IEAAXAEBV_Locinfo@2@@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar__Init, 8)
void __thiscall time_get_wchar__Init(time_get_wchar *this, const _Locinfo *locinfo)
{
    TRACE("(%p %p)\n", this, locinfo);

#if _MSVCP_VER >=110
    this->days = create_time_get_str(_Locinfo__W_Getdays(locinfo));
#else
    this->days = create_time_get_str(_Locinfo__Getdays(locinfo), locinfo);
#endif

#if _MSVCP_VER >=110
    this->months = create_time_get_str(_Locinfo__W_Getmonths(locinfo));
#else
    this->months = create_time_get_str(_Locinfo__Getmonths(locinfo), locinfo);
#endif

    this->dateorder = _Locinfo__Getdateorder(locinfo);
    _Locinfo__Getcvt(locinfo, &this->cvt);
}

/* ??0?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z */
/* ??0?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@AEBV_Locinfo@1@_K@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_ctor_locinfo, 12)
time_get_wchar* __thiscall time_get_wchar_ctor_locinfo(time_get_wchar *this,
        const _Locinfo *locinfo, size_t refs)
{
    TRACE("(%p %p %Iu)\n", this, locinfo, refs);
    locale_facet_ctor_refs(&this->facet, refs);
    this->facet.vtable = &time_get_wchar_vtable;
    time_get_wchar__Init(this, locinfo);
    return this;
}

/* ??0?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IAE@PBDI@Z */
/* ??0?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_ctor_name, 12)
time_get_wchar* __thiscall time_get_wchar_ctor_name(time_get_wchar *this, const char *name, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %s %Iu)\n", this, name, refs);

    _Locinfo_ctor_cstr(&locinfo, name);
    time_get_wchar_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAE@I@Z */
/* ??0?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAA@_K@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_ctor_refs, 8)
time_get_wchar* __thiscall time_get_wchar_ctor_refs(time_get_wchar *this, size_t refs)
{
    _Locinfo locinfo;

    TRACE("(%p %Iu)\n", this, refs);

    _Locinfo_ctor(&locinfo);
    time_get_wchar_ctor_locinfo(this, &locinfo, refs);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??_F?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QAEXXZ */
/* ??_F?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(time_get_wchar_ctor, 4)
time_get_wchar* __thiscall time_get_wchar_ctor(time_get_wchar *this)
{
    return time_get_wchar_ctor_refs(this, 0);
}

/* ?_Tidy@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AAEXXZ */
/* ?_Tidy@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(time_get_wchar__Tidy, 4)
void __thiscall time_get_wchar__Tidy(time_get_wchar *this)
{
    TRACE("(%p)\n", this);

    operator_delete((wchar_t*)this->days);
    operator_delete((wchar_t*)this->months);
}

/* ??1?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MAE@XZ */
/* ??1?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(time_get_wchar_dtor, 4) /* virtual */
void __thiscall time_get_wchar_dtor(time_get_wchar *this)
{
    TRACE("(%p)\n", this);

    time_get_wchar__Tidy(this);
}

DEFINE_THISCALL_WRAPPER(time_get_wchar_vector_dtor, 8)
time_get_wchar* __thiscall time_get_wchar_vector_dtor(time_get_wchar *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            time_get_wchar_dtor(this+i);
        operator_delete(ptr);
    } else {
        time_get_wchar_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_Getcat@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SAIPAPBVfacet@locale@2@PBV42@@Z */
/* ?_Getcat@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@SA_KPEAPEBVfacet@locale@2@PEBV42@@Z */
unsigned int __cdecl time_get_wchar__Getcat(const locale_facet **facet, const locale *loc)
{
    TRACE("(%p %p)\n", facet, loc);

    if(facet && !*facet) {
        _Locinfo locinfo;

        *facet = operator_new(sizeof(time_get_wchar));
        _Locinfo_ctor_cstr(&locinfo, locale_string_char_c_str(&loc->ptr->name));
        time_get_wchar_ctor_locinfo((time_get_wchar*)*facet, &locinfo, 0);
        _Locinfo_dtor(&locinfo);
    }

    return LC_TIME;
}

static time_get_wchar* time_get_wchar_use_facet(const locale *loc)
{
    static time_get_wchar *obj = NULL;

    _Lockit lock;
    const locale_facet *fac;

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    fac = locale__Getfacet(loc, locale_id_operator_size_t(&time_get_wchar_id));
    if(fac) {
        _Lockit_dtor(&lock);
        return (time_get_wchar*)fac;
    }

    if(obj) {
        _Lockit_dtor(&lock);
        return obj;
    }

    time_get_wchar__Getcat(&fac, loc);
    obj = (time_get_wchar*)fac;
    call_locale_facet__Incref(&obj->facet);
    locale_facet_register(&obj->facet);
    _Lockit_dtor(&lock);

    return obj;
}

/* ?_Getint@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@ABAHAAV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@0HHAAH@Z */
/* ?_Getint@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@AEBAHAEAV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@0HHAEAH@Z */
int __cdecl time_get_wchar__Getint(const time_get_wchar *this,
        istreambuf_iterator_wchar *b, istreambuf_iterator_wchar *e,
        int min_val, int max_val, int *val)
{
    BOOL got_digit = FALSE;
    int len = 0, ret = 0;
    char buf[16];

    TRACE("(%p %p %p %d %d %p)\n", this, b, e, min_val, max_val, val);

    istreambuf_iterator_wchar_val(b);
    if(b->strbuf && (b->val == '-' || b->val == '+'))
    {
        buf[len++] = b->val;
        istreambuf_iterator_wchar_inc(b);
    }

    if (b->strbuf && b->val == '0')
    {
        got_digit = TRUE;
        buf[len++] = '0';
        istreambuf_iterator_wchar_inc(b);
    }
    while (b->strbuf && b->val == '0')
        istreambuf_iterator_wchar_inc(b);

    for (; b->strbuf && b->val >= '0' && b->val <= '9';
            istreambuf_iterator_wchar_inc(b))
    {
        if(len < ARRAY_SIZE(buf)-1)
            buf[len] = b->val;
        got_digit = TRUE;
        len++;
    }

    if (!b->strbuf)
        ret |= IOSTATE_eofbit;
    if (got_digit && len < ARRAY_SIZE(buf)-1)
    {
        int v, err;

        buf[len] = 0;
        v = _Stolx(buf, NULL, 10, &err);
        if(err || v < min_val || v > max_val)
            ret |= IOSTATE_failbit;
        else
            *val = v;
    }
    else
        ret |= IOSTATE_failbit;
    return ret;
}

/* ?do_date_order@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AW4dateorder@time_base@2@XZ */
/* ?do_date_order@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AW4dateorder@time_base@2@XZ */
DEFINE_THISCALL_WRAPPER(time_get_wchar_do_date_order, 4) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_wchar_do_date_order(this) CALL_VTBL_FUNC(this, 4, dateorder, (const time_get_wchar*), (this))
#else
#define call_time_get_wchar_do_date_order(this) CALL_VTBL_FUNC(this, 12, dateorder, (const time_get_wchar*), (this))
#endif
dateorder __thiscall time_get_wchar_do_date_order(const time_get_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->dateorder;
}

/* ?date_order@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AW4dateorder@time_base@2@XZ */
/* ?date_order@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AW4dateorder@time_base@2@XZ */
DEFINE_THISCALL_WRAPPER(time_get_wchar_date_order, 4)
dateorder __thiscall time_get_wchar_date_order(const time_get_wchar *this)
{
    return call_time_get_wchar_do_date_order(this);
}

static int find_longest_match_wchar(istreambuf_iterator_wchar *iter, const wchar_t *str)
{
    int i, len = 0, last_match = -1, match = -1;
    const wchar_t *p, *end;
    wchar_t buf[64];

    for(istreambuf_iterator_wchar_val(iter); iter->strbuf && len<ARRAY_SIZE(buf);
            istreambuf_iterator_wchar_inc(iter))
    {
        BOOL got_prefix = FALSE;

        buf[len++] = iter->val;
        last_match = match;
        match = -1;
        for(p=str+1, i=0; *p; p = (*end ? end+1 : end), i++)
        {
            end = wcschr(p, ':');
            if (!end)
                end = p + lstrlenW(p);

            if (end-p >= len && !memcmp(p, buf, len*sizeof(wchar_t)))
            {
                if (end-p == len)
                    match = i;
                else
                    got_prefix = TRUE;
            }
        }

        if (!got_prefix)
        {
            if (match != -1)
            {
                istreambuf_iterator_wchar_inc(iter);
                return match;
            }
            break;
        }
    }
    if (len == ARRAY_SIZE(buf))
        FIXME("temporary buffer is too small\n");
    if (!iter->strbuf)
        return match;
    return last_match;
}

/* ?do_get_monthname@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?do_get_monthname@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_do_get_monthname, 36) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_wchar_do_get_monthname(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 20, istreambuf_iterator_wchar*, \
        (const time_get_wchar*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#else
#define call_time_get_wchar_do_get_monthname(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 28, istreambuf_iterator_wchar*, \
        (const time_get_wchar*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#endif
istreambuf_iterator_wchar* __thiscall time_get_wchar_do_get_monthname(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t)
{
    int match;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, err, t);

    if ((match = find_longest_match_wchar(&s, this->months)) != -1)
        t->tm_mon = match / 2;
    else
        *err |= IOSTATE_failbit;

    *ret = s;
    return ret;
}

/* ?get_monthname@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?get_monthname@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_get_monthname, 36)
istreambuf_iterator_wchar* __thiscall time_get_wchar_get_monthname(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t)
{
    return call_time_get_wchar_do_get_monthname(this, ret, s, e, base, err, t);
}

/* ?do_get_time@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?do_get_time@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_do_get_time, 36) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_wchar_do_get_time(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 8, istreambuf_iterator_wchar*, \
        (const time_get_wchar*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#else
#define call_time_get_wchar_do_get_time(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 16, istreambuf_iterator_wchar*, \
        (const time_get_wchar*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#endif
istreambuf_iterator_wchar* __thiscall time_get_wchar_do_get_time(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t)
{
    TRACE("(%p %p %p %p %p)\n", this, ret, base, err, t);

    *err |= time_get_wchar__Getint(this, &s, &e, 0, 23, &t->tm_hour);
    if (*err || istreambuf_iterator_wchar_val(&s)!=':')
        *err |= IOSTATE_failbit;

    if (!*err)
    {
        istreambuf_iterator_wchar_inc(&s);
        *err |= time_get_wchar__Getint(this, &s, &e, 0, 59, &t->tm_min);
    }
    if (*err || istreambuf_iterator_wchar_val(&s)!=':')
        *err |= IOSTATE_failbit;

    if (!*err)
    {
        istreambuf_iterator_wchar_inc(&s);
        *err |= time_get_wchar__Getint(this, &s, &e, 0, 59, &t->tm_sec);
    }

    *ret = s;
    return ret;
}

/* ?get_time@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?get_time@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_get_time, 36)
istreambuf_iterator_wchar* __thiscall time_get_wchar_get_time(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t)
{
    return call_time_get_wchar_do_get_time(this, ret, s, e, base, err, t);
}

/* ?do_get_weekday@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?do_get_weekday@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_do_get_weekday, 36) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_wchar_do_get_weekday(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 16, istreambuf_iterator_wchar*, \
        (const time_get_wchar*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#else
#define call_time_get_wchar_do_get_weekday(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 24, istreambuf_iterator_wchar*, \
        (const time_get_wchar*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#endif
istreambuf_iterator_wchar* __thiscall time_get_wchar_do_get_weekday(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t)
{
    int match;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, err, t);

    if ((match = find_longest_match_wchar(&s, this->days)) != -1)
        t->tm_wday = match / 2;
    else
        *err |= IOSTATE_failbit;

    *ret = s;
    return ret;
}

/* ?get_weekday@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?get_weekday@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_get_weekday, 36)
istreambuf_iterator_wchar* __thiscall time_get_wchar_get_weekday(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t)
{
    return call_time_get_wchar_do_get_weekday(this, ret, s, e, base, err, t);
}

/* ?do_get_year@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?do_get_year@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_do_get_year, 36) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_wchar_do_get_year(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 24, istreambuf_iterator_wchar*, \
        (const time_get_wchar*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#else
#define call_time_get_wchar_do_get_year(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 32, istreambuf_iterator_wchar*, \
        (const time_get_wchar*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#endif
istreambuf_iterator_wchar* __thiscall time_get_wchar_do_get_year(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t)
{
    int year;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, err, t);

    /* The function supports only dates from [1900-2035] range */
    *err |= time_get_wchar__Getint(this, &s, &e, 0, 2035, &year);
    if (!(*err & IOSTATE_failbit))
    {
        if (year >= 1900)
            year -= 1900;
        if (year > 135)
            *err |= IOSTATE_failbit;
        else
            t->tm_year = year;
    }

    *ret = s;
    return ret;
}

/* ?get_year@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?get_year@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_get_year, 36)
istreambuf_iterator_wchar* __thiscall time_get_wchar_get_year(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t)
{
    return call_time_get_wchar_do_get_year(this, ret, s, e, base, err, t);
}

static void skip_ws_wchar(ctype_wchar *ctype, istreambuf_iterator_wchar *iter)
{
    istreambuf_iterator_wchar_val(iter);
    while(iter->strbuf && ctype_wchar_is_ch(ctype, _SPACE, iter->val))
        istreambuf_iterator_wchar_inc(iter);
}

static void skip_date_delim_wchar(ctype_wchar *ctype, istreambuf_iterator_wchar *iter)
{
    skip_ws_wchar(ctype, iter);
    if(iter->strbuf && (iter->val == '/' || iter->val == ':'))
        istreambuf_iterator_wchar_inc(iter);
    skip_ws_wchar(ctype, iter);
}

/* ?do_get_date@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?do_get_date@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_do_get_date, 36) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_wchar_do_get_date(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 12, istreambuf_iterator_wchar*, \
        (const time_get_wchar*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#else
#define call_time_get_wchar_do_get_date(this, ret, s, e, base, err, t) CALL_VTBL_FUNC(this, 20, istreambuf_iterator_wchar*, \
        (const time_get_wchar*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, struct tm*), \
        (this, ret, s, e, base, err, t))
#endif
istreambuf_iterator_wchar* __thiscall time_get_wchar_do_get_date(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t)
{
    ctype_wchar *ctype;
    dateorder order;

    TRACE("(%p %p %p %p %p)\n", this, ret, base, err, t);

    ctype = ctype_wchar_use_facet(IOS_LOCALE(base));

    order = time_get_wchar_date_order(this);
    if(order == DATEORDER_no_order)
        order = DATEORDER_mdy;

    switch(order) {
    case DATEORDER_dmy:
        *err |= time_get_wchar__Getint(this, &s, &e, 1, 31, &t->tm_mday);
        skip_date_delim_wchar(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        if(s.strbuf && ctype_wchar_is_ch(ctype, _DIGIT, s.val)) {
            *err |= time_get_wchar__Getint(this, &s, &e, 1, 12, &t->tm_mon);
            t->tm_mon--;
        } else {
            time_get_wchar_get_monthname(this, &s, s, e, base, err, t);
        }
        skip_date_delim_wchar(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        time_get_wchar_get_year(this, &s, s, e, base, err, t);
        break;
    case DATEORDER_mdy:
        istreambuf_iterator_wchar_val(&s);
        if(s.strbuf && ctype_wchar_is_ch(ctype, _DIGIT, s.val)) {
            *err |= time_get_wchar__Getint(this, &s, &e, 1, 12, &t->tm_mon);
            t->tm_mon--;
        } else {
            time_get_wchar_get_monthname(this, &s, s, e, base, err, t);
        }
        skip_date_delim_wchar(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        *err |= time_get_wchar__Getint(this, &s, &e, 1, 31, &t->tm_mday);
        skip_date_delim_wchar(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        time_get_wchar_get_year(this, &s, s, e, base, err, t);
        break;
    case DATEORDER_ymd:
        time_get_wchar_get_year(this, &s, s, e, base, err, t);
        skip_date_delim_wchar(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        if(ctype_wchar_is_ch(ctype, _DIGIT, s.val)) {
            *err |= time_get_wchar__Getint(this, &s, &e, 1, 12, &t->tm_mon);
            t->tm_mon--;
        } else {
            time_get_wchar_get_monthname(this, &s, s, e, base, err, t);
        }
        skip_date_delim_wchar(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        *err |= time_get_wchar__Getint(this, &s, &e, 1, 31, &t->tm_mday);
        break;
    case DATEORDER_ydm:
        time_get_wchar_get_year(this, &s, s, e, base, err, t);
        skip_date_delim_wchar(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        *err |= time_get_wchar__Getint(this, &s, &e, 1, 31, &t->tm_mday);
        skip_date_delim_wchar(ctype, &s);
        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }
        if(ctype_wchar_is_ch(ctype, _DIGIT, s.val)) {
            *err |= time_get_wchar__Getint(this, &s, &e, 1, 12, &t->tm_mon);
            t->tm_mon--;
        } else {
            time_get_wchar_get_monthname(this, &s, s, e, base, err, t);
        }
        break;
    default:
        ERR("incorrect order value: %d\n", order);
        break;
    }

    if(!s.strbuf)
        *err |= IOSTATE_eofbit;
    *ret = s;
    return ret;
}

/* ?get_date@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@@Z */
/* ?get_date@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_get_date, 36)
istreambuf_iterator_wchar* __thiscall time_get_wchar_get_date(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t)
{
    return call_time_get_wchar_do_get_date(this, ret, s, e, base, err, t);
}

istreambuf_iterator_wchar* __thiscall time_get_wchar_get(const time_get_wchar*,
        istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar,
        ios_base*, int*, struct tm*, char, char);

/* ?_Getfmt@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@PBD@Z */
/* ?_Getfmt@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@IEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@PEBD@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar__Getfmt, 40)
istreambuf_iterator_wchar* __thiscall time_get_wchar__Getfmt(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t, const char *fmt)
{
    ctype_wchar *ctype;

    TRACE("(%p %p %p %p %p %s)\n", this, ret, base, err, t, fmt);

    ctype = ctype_wchar_use_facet(IOS_LOCALE(base));
    istreambuf_iterator_wchar_val(&s);

    while(*fmt) {
        if(*fmt == ' ') {
            skip_ws_wchar(ctype, &s);
            fmt++;
            continue;
        }

        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }

        if(*fmt == '%') {
            fmt++;
            time_get_wchar_get(this, &s, s, e, base, err, t, *fmt, 0);
        } else {
            if(s.val != *fmt)
                *err |= IOSTATE_failbit;
            else
                istreambuf_iterator_wchar_inc(&s);
        }

        if(*err & IOSTATE_failbit)
            break;
        fmt++;
    }

    if(!s.strbuf)
        *err |= IOSTATE_eofbit;
    *ret = s;
    return ret;
}

/* ?do_get@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@DD@Z */
/* ?do_get@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@MEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@DD@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_do_get, 44) /* virtual */
#if _MSVCP_VER <= 100
#define call_time_get_wchar_do_get(this, ret, s, e, base, err, t, fmt, mod) CALL_VTBL_FUNC(this, 28, istreambuf_iterator_wchar*, \
        (const time_get_wchar*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, struct tm*, char, char), \
        (this, ret, s, e, base, err, t, fmt, mod))
#else
#define call_time_get_wchar_do_get(this, ret, s, e, base, err, t, fmt, mod) CALL_VTBL_FUNC(this, 36, istreambuf_iterator_wchar*, \
        (const time_get_wchar*, istreambuf_iterator_wchar*, istreambuf_iterator_wchar, istreambuf_iterator_wchar, ios_base*, int*, struct tm*, char, char), \
        (this, ret, s, e, base, err, t, fmt, mod))
#endif
istreambuf_iterator_wchar* __thiscall time_get_wchar_do_get(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t, char fmt, char mod)
{
    ctype_wchar *ctype;

    TRACE("(%p %p %p %p %p %c %c)\n", this, ret, base, err, t, fmt, mod);

    ctype = ctype_wchar_use_facet(IOS_LOCALE(base));

    switch(fmt) {
    case 'a':
    case 'A':
        time_get_wchar_get_weekday(this, &s, s, e, base, err, t);
        break;
    case 'b':
    case 'B':
    case 'h':
        time_get_wchar_get_monthname(this, &s, s, e, base, err, t);
        break;
    case 'c':
        time_get_wchar__Getfmt(this, &s, s, e, base, err, t, "%b %d %H:%M:%S %Y");
        break;
    case 'C':
        *err |= time_get_wchar__Getint(this, &s, &e, 0, 99, &t->tm_year);
        if(!(*err & IOSTATE_failbit))
            t->tm_year = t->tm_year * 100 - 1900;
        break;
    case 'd':
    case 'e':
        if(fmt == 'e') skip_ws_wchar(ctype, &s);
        *err |= time_get_wchar__Getint(this, &s, &e, 1, 31, &t->tm_mday);
        break;
    case 'D':
        time_get_wchar__Getfmt(this, &s, s, e, base, err, t, "%m/%d/%y");
        break;
    case 'F':
        time_get_wchar__Getfmt(this, &s, s, e, base, err, t, "%Y-%m-%d");
        break;
    case 'H':
        *err |= time_get_wchar__Getint(this, &s, &e, 0, 23, &t->tm_hour);
        break;
    case 'I':
        *err |= time_get_wchar__Getint(this, &s, &e, 0, 11, &t->tm_hour);
        break;
    case 'j':
        *err |= time_get_wchar__Getint(this, &s, &e, 1, 366, &t->tm_yday);
        break;
    case 'm':
        *err |= time_get_wchar__Getint(this, &s, &e, 1, 12, &t->tm_mon);
        if(!(*err & IOSTATE_failbit))
            t->tm_mon--;
        break;
    case 'M':
        *err = time_get_wchar__Getint(this, &s, &e, 0, 59, &t->tm_min);
        break;
    case 'n':
    case 't':
        skip_ws_wchar(ctype, &s);
        break;
    case 'p': {
        BOOL pm = FALSE;

        istreambuf_iterator_wchar_val(&s);
        if(s.strbuf && (s.val=='P' || s.val=='p'))
            pm = TRUE;
        else if (!s.strbuf || (s.val!='A' && s.val!='a')) {
            *err |= IOSTATE_failbit;
            break;
        }
        istreambuf_iterator_wchar_inc(&s);
        if(!s.strbuf || (s.val!='M' && s.val!='m')) {
            *err |= IOSTATE_failbit;
            break;
        }
        istreambuf_iterator_wchar_inc(&s);

        if(pm)
            t->tm_hour += 12;
        break;
    }
    case 'r':
        time_get_wchar__Getfmt(this, &s, s, e, base, err, t, "%I:%M:%S %p");
        break;
    case 'R':
        time_get_wchar__Getfmt(this, &s, s, e, base, err, t, "%H:%M");
        break;
    case 'S':
        *err |= time_get_wchar__Getint(this, &s, &e, 0, 59, &t->tm_sec);
        break;
    case 'T':
    case 'X':
        time_get_wchar__Getfmt(this, &s, s, e, base, err, t, "%H:%M:%S");
        break;
    case 'u':
        *err |= time_get_wchar__Getint(this, &s, &e, 1, 7, &t->tm_wday);
        if(!(*err & IOSTATE_failbit) && t->tm_wday==7)
            t->tm_wday = 0;
        break;
    case 'w':
        *err |= time_get_wchar__Getint(this, &s, &e, 0, 6, &t->tm_wday);
        break;
    case 'x':
        time_get_wchar_get_date(this, &s, s, e, base, err, t);
        break;
    case 'y':
        *err |= time_get_wchar__Getint(this, &s, &e, 0, 99, &t->tm_year);
        if(!(*err & IOSTATE_failbit) && t->tm_year<69)
            t->tm_year += 100;
        break;
    case 'Y':
        time_get_wchar_get_year(this, &s, s, e, base, err, t);
        break;
    default:
        FIXME("unrecognized format: %c\n", fmt);
        *err |= IOSTATE_failbit;
    }

    if(!s.strbuf)
        *err |= IOSTATE_eofbit;
    *ret = s;
    return ret;
}

/* ?get@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@DD@Z */
/* ?get@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@DD@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_get, 44)
istreambuf_iterator_wchar* __thiscall time_get_wchar_get(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t, char fmt, char mod)
{
    return call_time_get_wchar_do_get(this, ret, s, e, base, err, t, fmt, mod);
}

/* ?get@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QBE?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AAVios_base@2@AAHPAUtm@@PB_W4@Z */
/* ?get@?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@QEBA?AV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@2@V32@0AEAVios_base@2@AEAHPEAUtm@@PEB_W4@Z */
DEFINE_THISCALL_WRAPPER(time_get_wchar_get_fmt, 44)
istreambuf_iterator_wchar* __thiscall time_get_wchar_get_fmt(const time_get_wchar *this,
        istreambuf_iterator_wchar *ret, istreambuf_iterator_wchar s, istreambuf_iterator_wchar e,
        ios_base *base, int *err, struct tm *t, const wchar_t *fmtstart, const wchar_t *fmtend)
{
    ctype_wchar *ctype;

    TRACE("(%p %p %p %p %p %s)\n", this, ret, base, err, t, wine_dbgstr_wn(fmtstart, fmtend-fmtstart));

    ctype = ctype_wchar_use_facet(IOS_LOCALE(base));
    istreambuf_iterator_wchar_val(&s);

    while(fmtstart < fmtend) {
        if(ctype_wchar_is_ch(ctype, _SPACE, *fmtstart)) {
            skip_ws_wchar(ctype, &s);
            fmtstart++;
            continue;
        }

        if(!s.strbuf) {
            *err |= IOSTATE_failbit;
            break;
        }

        if(*fmtstart != '%' || fmtstart+1 >= fmtend || fmtstart[1] == '%') {
            if(s.val != *fmtstart)
                *err |= IOSTATE_failbit;
            else
                istreambuf_iterator_wchar_inc(&s);
            if(*fmtstart == '%')
                fmtstart++;
        } else {
            fmtstart++;
            time_get_wchar_get(this, &s, s, e, base, err, t, *fmtstart, 0);
        }

        if(*err & IOSTATE_failbit)
            break;
        fmtstart++;
    }

    if(!s.strbuf)
        *err |= IOSTATE_eofbit;
    *ret = s;
    return ret;
}

/* ??_7_Locimp@locale@std@@6B@ */
extern const vtable_ptr locale__Locimp_vtable;

/* ??0_Locimp@locale@std@@AAE@_N@Z */
/* ??0_Locimp@locale@std@@AEAA@_N@Z */
DEFINE_THISCALL_WRAPPER(locale__Locimp_ctor_transparent, 8)
locale__Locimp* __thiscall locale__Locimp_ctor_transparent(locale__Locimp *this, bool transparent)
{
    TRACE("(%p %d)\n", this, transparent);

    memset(this, 0, sizeof(locale__Locimp));
    locale_facet_ctor_refs(&this->facet, 1);
    this->facet.vtable = &locale__Locimp_vtable;
    this->transparent = transparent;
    locale_string_char_ctor_cstr(&this->name, "*");
    return this;
}

/* ??_F_Locimp@locale@std@@QAEXXZ */
/* ??_F_Locimp@locale@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(locale__Locimp_ctor, 4)
locale__Locimp* __thiscall locale__Locimp_ctor(locale__Locimp *this)
{
    return locale__Locimp_ctor_transparent(this, FALSE);
}

/* ??0_Locimp@locale@std@@AAE@ABV012@@Z */
/* ??0_Locimp@locale@std@@AEAA@AEBV012@@Z */
DEFINE_THISCALL_WRAPPER(locale__Locimp_copy_ctor, 8)
locale__Locimp* __thiscall locale__Locimp_copy_ctor(locale__Locimp *this, const locale__Locimp *copy)
{
    _Lockit lock;
    size_t i;

    TRACE("(%p %p)\n", this, copy);

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    memcpy(this, copy, sizeof(locale__Locimp));
    locale_facet_ctor_refs(&this->facet, 1);
    this->facet.vtable = &locale__Locimp_vtable;
    if(copy->facetvec) {
        this->facetvec = operator_new(copy->facet_cnt*sizeof(locale_facet*));
        for(i=0; i<this->facet_cnt; i++)
        {
            this->facetvec[i] = copy->facetvec[i];
            if(this->facetvec[i])
                call_locale_facet__Incref(this->facetvec[i]);
        }
    }
    locale_string_char_copy_ctor(&this->name, &copy->name);
    _Lockit_dtor(&lock);
    return this;
}

/* ?_Locimp_ctor@_Locimp@locale@std@@CAXPAV123@ABV123@@Z */
/* ?_Locimp_ctor@_Locimp@locale@std@@CAXPEAV123@AEBV123@@Z */
locale__Locimp* __cdecl locale__Locimp__Locimp_ctor(locale__Locimp *this, const locale__Locimp *copy)
{
    return locale__Locimp_copy_ctor(this, copy);
}

/* ??1_Locimp@locale@std@@MAE@XZ */
/* ??1_Locimp@locale@std@@MEAA@XZ */
DEFINE_THISCALL_WRAPPER(locale__Locimp_dtor, 4)
void __thiscall locale__Locimp_dtor(locale__Locimp *this)
{
    size_t i;

    TRACE("(%p)\n", this);

    locale_facet_dtor(&this->facet);
    for(i=0; i<this->facet_cnt; i++)
        if(this->facetvec[i] && call_locale_facet__Decref(this->facetvec[i]))
            call_locale_facet_vector_dtor(this->facetvec[i], 1);

    operator_delete(this->facetvec);
    locale_string_char_dtor(&this->name);
}

/* ?_Locimp_dtor@_Locimp@locale@std@@CAXPAV123@@Z */
/* ?_Locimp_dtor@_Locimp@locale@std@@CAXPEAV123@@Z */
void __cdecl locale__Locimp__Locimp_dtor(locale__Locimp *this)
{
    locale__Locimp_dtor(this);
}

DEFINE_THISCALL_WRAPPER(locale__Locimp_vector_dtor, 8)
locale__Locimp* __thiscall locale__Locimp_vector_dtor(locale__Locimp *this, unsigned int flags)
{
    TRACE("(%p %x)\n", this, flags);
    if(flags & 2) {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)this-1;

        for(i=*ptr-1; i>=0; i--)
            locale__Locimp_dtor(this+i);
        operator_delete(ptr);
    } else {
        locale__Locimp_dtor(this);
        if(flags & 1)
            operator_delete(this);
    }

    return this;
}

/* ?_New_Locimp@_Locimp@locale@std@@CAPAV123@ABV123@@Z */
/* ?_New_Locimp@_Locimp@locale@std@@CAPEAV123@AEBV123@@Z */
locale__Locimp* __cdecl locale__Locimp__New_Locimp(const locale__Locimp *copy)
{
    locale__Locimp *ret;

    TRACE("(%p)\n", copy);

    ret = operator_new(sizeof(locale__Locimp));
    return locale__Locimp_copy_ctor(ret, copy);
}

/* ?_New_Locimp@_Locimp@locale@std@@CAPAV123@_N@Z */
/* ?_New_Locimp@_Locimp@locale@std@@CAPEAV123@_N@Z */
locale__Locimp* __cdecl locale__Locimp__New_Locimp_transparent(bool transparent)
{
    locale__Locimp *ret;

    TRACE("(%x)\n", transparent);

    ret = operator_new(sizeof(locale__Locimp));
    return locale__Locimp_ctor_transparent(ret, transparent);
}

/* ?_Locimp_Addfac@_Locimp@locale@std@@CAXPAV123@PAVfacet@23@I@Z */
/* ?_Locimp_Addfac@_Locimp@locale@std@@CAXPEAV123@PEAVfacet@23@_K@Z */
void __cdecl locale__Locimp__Locimp_Addfac(locale__Locimp *locimp, locale_facet *facet, size_t id)
{
    _Lockit lock;

    TRACE("(%p %p %Iu)\n", locimp, facet, id);

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    if(id >= locimp->facet_cnt) {
        size_t new_size = id+1;
        locale_facet **new_facetvec;

        if(new_size < 40)
            new_size = 40;

        new_facetvec = operator_new(sizeof(locale_facet*)*new_size);
        memset(new_facetvec, 0, sizeof(locale_facet*)*new_size);
        memcpy(new_facetvec, locimp->facetvec, sizeof(locale_facet*)*locimp->facet_cnt);
        operator_delete(locimp->facetvec);
        locimp->facetvec = new_facetvec;
        locimp->facet_cnt = new_size;
    }

    if(locimp->facetvec[id] && call_locale_facet__Decref(locimp->facetvec[id]))
        call_locale_facet_vector_dtor(locimp->facetvec[id], 1);

    locimp->facetvec[id] = facet;
    if(facet)
        call_locale_facet__Incref(facet);
    _Lockit_dtor(&lock);
}

/* ?_Addfac@_Locimp@locale@std@@AAEXPAVfacet@23@I@Z */
/* ?_Addfac@_Locimp@locale@std@@AEAAXPEAVfacet@23@_K@Z */
DEFINE_THISCALL_WRAPPER(locale__Locimp__Addfac, 12)
void __thiscall locale__Locimp__Addfac(locale__Locimp *this, locale_facet *facet, size_t id)
{
    locale__Locimp__Locimp_Addfac(this, facet, id);
}

/* ?_Clocptr_func@_Locimp@locale@std@@CAAAPAV123@XZ */
/* ?_Clocptr_func@_Locimp@locale@std@@CAAEAPEAV123@XZ */
locale__Locimp** __cdecl locale__Locimp__Clocptr_func(void)
{
    FIXME("stub\n");
    return NULL;
}

/* ?_Makeushloc@_Locimp@locale@std@@CAXABV_Locinfo@3@HPAV123@PBV23@@Z */
/* ?_Makeushloc@_Locimp@locale@std@@CAXAEBV_Locinfo@3@HPEAV123@PEBV23@@Z */
/* List of missing facets:
 * messages, money_get, money_put, moneypunct, moneypunct, time_get
 */
void __cdecl locale__Locimp__Makeushloc(const _Locinfo *locinfo, category cat, locale__Locimp *locimp, const locale *loc)
{
    FIXME("(%p %d %p %p) semi-stub\n", locinfo, cat, locimp, loc);

    if(cat & (1<<(ctype_short__Getcat(NULL, NULL)-1))) {
        ctype_wchar *ctype;

        if(loc) {
            ctype = ctype_short_use_facet(loc);
        }else {
            ctype = operator_new(sizeof(ctype_wchar));
            ctype_short_ctor_locinfo(ctype, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &ctype->base.facet, locale_id_operator_size_t(&ctype_short_id));
    }

    if(cat & (1<<(num_get_short__Getcat(NULL, NULL)-1))) {
        num_get *numget;

        if(loc) {
            numget = num_get_short_use_facet(loc);
        }else {
            numget = operator_new(sizeof(num_get));
            num_get_short_ctor_locinfo(numget, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &numget->facet, locale_id_operator_size_t(&num_get_short_id));
    }

    if(cat & (1<<(num_put_short__Getcat(NULL, NULL)-1))) {
        num_put *numput;

        if(loc) {
            numput = num_put_short_use_facet(loc);
        }else {
            numput = operator_new(sizeof(num_put));
            num_put_short_ctor_locinfo(numput, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &numput->facet, locale_id_operator_size_t(&num_put_short_id));
    }

    if(cat & (1<<(numpunct_short__Getcat(NULL, NULL)-1))) {
        numpunct_wchar *numpunct;

        if(loc) {
            numpunct = numpunct_short_use_facet(loc);
        }else {
            numpunct = operator_new(sizeof(numpunct_wchar));
            numpunct_short_ctor_locinfo(numpunct, locinfo, 0, FALSE);
        }
        locale__Locimp__Addfac(locimp, &numpunct->facet, locale_id_operator_size_t(&numpunct_short_id));
    }

    if(cat & (1<<(collate_short__Getcat(NULL, NULL)-1))) {
        collate *c;

        if(loc) {
            c = collate_short_use_facet(loc);
        }else {
            c = operator_new(sizeof(collate));
            collate_short_ctor_locinfo(c, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &c->facet, locale_id_operator_size_t(&collate_short_id));
    }

     if(cat & (1<<(time_put_short__Getcat(NULL, NULL)-1))) {
         time_put *t;

         if(loc) {
             t = time_put_short_use_facet(loc);
         }else {
             t = operator_new(sizeof(time_put));
             time_put_short_ctor_locinfo(t, locinfo, 0);
         }
         locale__Locimp__Addfac(locimp, &t->facet, locale_id_operator_size_t(&time_put_short_id));
     }

    if(cat & (1<<(codecvt_short__Getcat(NULL, NULL)-1))) {
        codecvt_wchar *codecvt;

        if(loc) {
            codecvt = codecvt_short_use_facet(loc);
        }else {
            codecvt = operator_new(sizeof(codecvt_wchar));
            codecvt_short_ctor_locinfo(codecvt, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &codecvt->base.facet, locale_id_operator_size_t(&codecvt_short_id));
    }
}

/* ?_Makewloc@_Locimp@locale@std@@CAXABV_Locinfo@3@HPAV123@PBV23@@Z */
/* ?_Makewloc@_Locimp@locale@std@@CAXAEBV_Locinfo@3@HPEAV123@PEBV23@@Z */
/* List of missing facets:
 * messages, money_get, money_put, moneypunct, moneypunct
 */
void __cdecl locale__Locimp__Makewloc(const _Locinfo *locinfo, category cat, locale__Locimp *locimp, const locale *loc)
{
    FIXME("(%p %d %p %p) semi-stub\n", locinfo, cat, locimp, loc);

    if(cat & (1<<(ctype_wchar__Getcat(NULL, NULL)-1))) {
        ctype_wchar *ctype;

        if(loc) {
            ctype = ctype_wchar_use_facet(loc);
        }else {
            ctype = operator_new(sizeof(ctype_wchar));
            ctype_wchar_ctor_locinfo(ctype, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &ctype->base.facet, locale_id_operator_size_t(&ctype_wchar_id));
    }

    if(cat & (1<<(num_get_wchar__Getcat(NULL, NULL)-1))) {
        num_get *numget;

        if(loc) {
            numget = num_get_wchar_use_facet(loc);
        }else {
            numget = operator_new(sizeof(num_get));
            num_get_wchar_ctor_locinfo(numget, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &numget->facet, locale_id_operator_size_t(&num_get_wchar_id));
    }

    if(cat & (1<<(num_put_wchar__Getcat(NULL, NULL)-1))) {
        num_put *numput;

        if(loc) {
            numput = num_put_wchar_use_facet(loc);
        }else {
            numput = operator_new(sizeof(num_put));
            num_put_wchar_ctor_locinfo(numput, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &numput->facet, locale_id_operator_size_t(&num_put_wchar_id));
    }

    if(cat & (1<<(numpunct_wchar__Getcat(NULL, NULL)-1))) {
        numpunct_wchar *numpunct;

        if(loc) {
            numpunct = numpunct_wchar_use_facet(loc);
        }else {
            numpunct = operator_new(sizeof(numpunct_wchar));
            numpunct_wchar_ctor_locinfo(numpunct, locinfo, 0, FALSE);
        }
        locale__Locimp__Addfac(locimp, &numpunct->facet, locale_id_operator_size_t(&numpunct_wchar_id));
    }

    if(cat & (1<<(collate_wchar__Getcat(NULL, NULL)-1))) {
        collate *c;

        if(loc) {
            c = collate_wchar_use_facet(loc);
        }else {
            c = operator_new(sizeof(collate));
            collate_wchar_ctor_locinfo(c, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &c->facet, locale_id_operator_size_t(&collate_wchar_id));
    }

    if(cat & (1<<(time_get_wchar__Getcat(NULL, NULL)-1))) {
        time_get_wchar *t;

        if(loc) {
            t = time_get_wchar_use_facet(loc);
        }else {
            t = operator_new(sizeof(time_get_wchar));
            time_get_wchar_ctor_locinfo(t, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &t->facet, locale_id_operator_size_t(&time_get_wchar_id));
    }

    if(cat & (1<<(time_put_wchar__Getcat(NULL, NULL)-1))) {
        time_put *t;

        if(loc) {
            t = time_put_wchar_use_facet(loc);
        }else {
            t = operator_new(sizeof(time_put));
            time_put_wchar_ctor_locinfo(t, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &t->facet, locale_id_operator_size_t(&time_put_wchar_id));
    }

    if(cat & (1<<(codecvt_wchar__Getcat(NULL, NULL)-1))) {
        codecvt_wchar *codecvt;

        if(loc) {
            codecvt = codecvt_wchar_use_facet(loc);
        }else {
            codecvt = operator_new(sizeof(codecvt_wchar));
            codecvt_wchar_ctor_locinfo(codecvt, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &codecvt->base.facet, locale_id_operator_size_t(&codecvt_wchar_id));
    }
}

/* ?_Makexloc@_Locimp@locale@std@@CAXABV_Locinfo@3@HPAV123@PBV23@@Z */
/* ?_Makexloc@_Locimp@locale@std@@CAXAEBV_Locinfo@3@HPEAV123@PEBV23@@Z */
/* List of missing facets:
 * messages, money_get, money_put, moneypunct, moneypunct
 */
void __cdecl locale__Locimp__Makexloc(const _Locinfo *locinfo, category cat, locale__Locimp *locimp, const locale *loc)
{
    FIXME("(%p %d %p %p) semi-stub\n", locinfo, cat, locimp, loc);

    if(cat & (1<<(ctype_char__Getcat(NULL, NULL)-1))) {
        ctype_char *ctype;

        if(loc) {
            ctype = ctype_char_use_facet(loc);
        }else {
            ctype = operator_new(sizeof(ctype_char));
            ctype_char_ctor_locinfo(ctype, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &ctype->base.facet, locale_id_operator_size_t(&ctype_char_id));
    }

    if(cat & (1<<(num_get_char__Getcat(NULL, NULL)-1))) {
        num_get *numget;

        if(loc) {
            numget = num_get_char_use_facet(loc);
        }else {
            numget = operator_new(sizeof(num_get));
            num_get_char_ctor_locinfo(numget, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &numget->facet, locale_id_operator_size_t(&num_get_char_id));
    }

    if(cat & (1<<(num_put_char__Getcat(NULL, NULL)-1))) {
        num_put *numput;

        if(loc) {
            numput = num_put_char_use_facet(loc);
        }else {
            numput = operator_new(sizeof(num_put));
            num_put_char_ctor_locinfo(numput, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &numput->facet, locale_id_operator_size_t(&num_put_char_id));
    }

    if(cat & (1<<(numpunct_char__Getcat(NULL, NULL)-1))) {
        numpunct_char *numpunct;

        if(loc) {
            numpunct = numpunct_char_use_facet(loc);
        }else {
            numpunct = operator_new(sizeof(numpunct_char));
            numpunct_char_ctor_locinfo(numpunct, locinfo, 0, FALSE);
        }
        locale__Locimp__Addfac(locimp, &numpunct->facet, locale_id_operator_size_t(&numpunct_char_id));
    }

    if(cat & (1<<(collate_char__Getcat(NULL, NULL)-1))) {
        collate *c;

        if(loc) {
            c = collate_char_use_facet(loc);
        }else {
            c = operator_new(sizeof(collate));
            collate_char_ctor_locinfo(c, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &c->facet, locale_id_operator_size_t(&collate_char_id));
    }

    if(cat & (1<<(time_get_char__Getcat(NULL, NULL)-1))) {
        time_get_char *t;

        if(loc) {
            t = time_get_char_use_facet(loc);
        }else {
            t = operator_new(sizeof(time_get_char));
            time_get_char_ctor_locinfo(t, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &t->facet, locale_id_operator_size_t(&time_get_char_id));
    }

    if(cat & (1<<(time_put_char__Getcat(NULL, NULL)-1))) {
        time_put *t;

        if(loc) {
            t = time_put_char_use_facet(loc);
        }else {
            t = operator_new(sizeof(time_put));
            time_put_char_ctor_locinfo(t, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &t->facet, locale_id_operator_size_t(&time_put_char_id));
    }

    if(cat & (1<<(codecvt_char__Getcat(NULL, NULL)-1))) {
        codecvt_char *codecvt;

        if(loc) {
            codecvt = codecvt_char_use_facet(loc);
        }else {
            codecvt = operator_new(sizeof(codecvt_char));
            codecvt_char_ctor_locinfo(codecvt, locinfo, 0);
        }
        locale__Locimp__Addfac(locimp, &codecvt->base.facet, locale_id_operator_size_t(&codecvt_char_id));
    }
}

/* ?_Makeloc@_Locimp@locale@std@@CAPAV123@ABV_Locinfo@3@HPAV123@PBV23@@Z */
/* ?_Makeloc@_Locimp@locale@std@@CAPEAV123@AEBV_Locinfo@3@HPEAV123@PEBV23@@Z */
locale__Locimp* __cdecl locale__Locimp__Makeloc(const _Locinfo *locinfo, category cat, locale__Locimp *locimp, const locale *loc)
{
    TRACE("(%p %d %p %p)\n", locinfo, cat, locimp, loc);

    locale__Locimp__Makexloc(locinfo, cat, locimp, loc);
    locale__Locimp__Makewloc(locinfo, cat, locimp, loc);
    locale__Locimp__Makeushloc(locinfo, cat, locimp, loc);

    locimp->catmask |= cat;
    locale_string_char_assign(&locimp->name, &locinfo->newlocname);
    return locimp;
}

/* ??0locale@std@@AAE@PAV_Locimp@01@@Z */
/* ??0locale@std@@AEAA@PEAV_Locimp@01@@Z */
DEFINE_THISCALL_WRAPPER(locale_ctor_locimp, 8)
locale* __thiscall locale_ctor_locimp(locale *this, locale__Locimp *locimp)
{
    TRACE("(%p %p)\n", this, locimp);
    /* Don't change locimp reference counter */
    this->ptr = locimp;
    return this;
}

/* ?_Init@locale@std@@CAPAV_Locimp@12@XZ */
/* ?_Init@locale@std@@CAPEAV_Locimp@12@XZ */
locale__Locimp* __cdecl locale__Init(void)
{
    _Lockit lock;

    TRACE("\n");

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    if(global_locale) {
        _Lockit_dtor(&lock);
        return global_locale;
    }

    global_locale = operator_new(sizeof(locale__Locimp));
    locale__Locimp_ctor(global_locale);
    global_locale->catmask = (1<<(LC_MAX+1))-1;
    locale_string_char_dtor(&global_locale->name);
    locale_string_char_ctor_cstr(&global_locale->name, "C");

    locale__Locimp__Clocptr = global_locale;
    global_locale->facet.refs++;
    locale_ctor_locimp(&classic_locale, locale__Locimp__Clocptr);
    _Lockit_dtor(&lock);

    return global_locale;
}

/* ?_Init@locale@std@@CAPAV_Locimp@12@_N@Z */
/* ?_Init@locale@std@@CAPEAV_Locimp@12@_N@Z */
locale__Locimp* __cdecl locale__Init_ref(bool inc_ref)
{
    locale__Locimp *ret;
    _Lockit lock;

    TRACE("(%x)\n", inc_ref);

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    if(inc_ref && global_locale) {
        call_locale_facet__Incref(&global_locale->facet);
        _Lockit_dtor(&lock);
        return global_locale;
    }

    ret = locale__Init();
    _Lockit_dtor(&lock);
    return ret;
}

/* ?_Iscloc@locale@std@@QBE_NXZ */
/* ?_Iscloc@locale@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(locale__Iscloc, 4)
bool __thiscall locale__Iscloc(const locale *this)
{
    TRACE("(%p)\n", this);
    return this->ptr == locale__Locimp__Clocptr;
}

/* ??0locale@std@@QAE@ABV01@0H@Z */
/* ??0locale@std@@QEAA@AEBV01@0H@Z */
DEFINE_THISCALL_WRAPPER(locale_ctor_locale_locale, 16)
locale* __thiscall locale_ctor_locale_locale(locale *this, const locale *loc, const locale *other, category cat)
{
    _Locinfo locinfo;

    TRACE("(%p %p %p %d)\n", this, loc, other, cat);

    this->ptr = operator_new(sizeof(locale__Locimp));
    locale__Locimp_copy_ctor(this->ptr, loc->ptr);

    _Locinfo_ctor_cat_cstr(&locinfo, loc->ptr->catmask, locale_string_char_c_str(&loc->ptr->name));
    _Locinfo__Addcats(&locinfo, cat & other->ptr->catmask, locale_string_char_c_str(&other->ptr->name));
    locale__Locimp__Makeloc(&locinfo, cat, this->ptr, other);
    _Locinfo_dtor(&locinfo);

    return this;
}

/* ??0locale@std@@QAE@ABV01@@Z */
/* ??0locale@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(locale_copy_ctor, 8)
locale* __thiscall locale_copy_ctor(locale *this, const locale *copy)
{
    TRACE("(%p %p)\n", this, copy);
    this->ptr = copy->ptr;
    call_locale_facet__Incref(&this->ptr->facet);
    return this;
}

/* ??0locale@std@@QAE@ABV01@PBDH@Z */
/* ??0locale@std@@QEAA@AEBV01@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(locale_ctor_locale_cstr, 16)
locale* __thiscall locale_ctor_locale_cstr(locale *this, const locale *loc, const char *locname, category cat)
{
    _Locinfo locinfo;

    TRACE("(%p %p %s %d)\n", this, loc, locname, cat);

    _Locinfo_ctor_cat_cstr(&locinfo, cat, locname);
    if(!memcmp(locale_string_char_c_str(&locinfo.newlocname), "*", 2)) {
        _Locinfo_dtor(&locinfo);
        operator_delete(this->ptr);
        _Xruntime_error("bad locale name");
    }

    this->ptr = operator_new(sizeof(locale__Locimp));
    locale__Locimp_copy_ctor(this->ptr, loc->ptr);

    locale__Locimp__Makeloc(&locinfo, cat, this->ptr, NULL);
    _Locinfo_dtor(&locinfo);
    return this;
}

/* ??0locale@std@@QAE@PBDH@Z */
/* ??0locale@std@@QEAA@PEBDH@Z */
DEFINE_THISCALL_WRAPPER(locale_ctor_cstr, 12)
locale* __thiscall locale_ctor_cstr(locale *this, const char *locname, category cat)
{
    _Locinfo locinfo;

    TRACE("(%p %s %d)\n", this, locname, cat);

    this->ptr = operator_new(sizeof(locale__Locimp));
    locale__Locimp_ctor(this->ptr);

    locale__Init();

    _Locinfo_ctor_cat_cstr(&locinfo, cat, locname);
    if(!memcmp(locale_string_char_c_str(&locinfo.newlocname), "*", 2)) {
        _Locinfo_dtor(&locinfo);
        operator_delete(this->ptr);
        _Xruntime_error("bad locale name");
    }

    locale__Locimp__Makeloc(&locinfo, cat, this->ptr, NULL);
    _Locinfo_dtor(&locinfo);

    return this;
}

/* ??0locale@std@@QAE@W4_Uninitialized@1@@Z */
/* ??0locale@std@@QEAA@W4_Uninitialized@1@@Z */
DEFINE_THISCALL_WRAPPER(locale_ctor_uninitialized, 8)
locale* __thiscall locale_ctor_uninitialized(locale *this, int uninitialized)
{
    TRACE("(%p)\n", this);
    this->ptr = NULL;
    return this;
}

/* ??0locale@std@@QAE@XZ */
/* ??0locale@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(locale_ctor, 4)
locale* __thiscall locale_ctor(locale *this)
{
    TRACE("(%p)\n", this);
    this->ptr = locale__Init();
    call_locale_facet__Incref(&this->ptr->facet);
    return this;
}

/* ??1locale@std@@QAE@XZ */
/* ??1locale@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(locale_dtor, 4)
void __thiscall locale_dtor(locale *this)
{
    TRACE("(%p)\n", this);
    if(this->ptr && call_locale_facet__Decref(&this->ptr->facet))
    {
        locale__Locimp_dtor(this->ptr);
        operator_delete(this->ptr);
    }
}

/* ??4locale@std@@QAEAAV01@ABV01@@Z */
/* ??4locale@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(locale_operator_assign, 8)
locale* __thiscall locale_operator_assign(locale *this, const locale *loc)
{
    FIXME("(%p %p) stub\n", this, loc);
    return NULL;
}

/* ??8locale@std@@QBE_NABV01@@Z */
/* ??8locale@std@@QEBA_NAEBV01@@Z */
DEFINE_THISCALL_WRAPPER(locale_operator_equal, 8)
bool __thiscall locale_operator_equal(const locale *this, const locale *loc)
{
    FIXME("(%p %p) stub\n", this, loc);
    return 0;
}

/* ??9locale@std@@QBE_NABV01@@Z */
/* ??9locale@std@@QEBA_NAEBV01@@Z */
DEFINE_THISCALL_WRAPPER(locale_operator_not_equal, 8)
bool __thiscall locale_operator_not_equal(const locale *this, locale const *loc)
{
    FIXME("(%p %p) stub\n", this, loc);
    return 0;
}

/* ?_Addfac@locale@std@@QAEAAV12@PAVfacet@12@II@Z */
/* ?_Addfac@locale@std@@QEAAAEAV12@PEAVfacet@12@_K1@Z */
DEFINE_THISCALL_WRAPPER(locale__Addfac, 16)
locale* __thiscall locale__Addfac(locale *this, locale_facet *facet, size_t id, size_t catmask)
{
    TRACE("(%p %p %Iu %Iu)\n", this, facet, id, catmask);

    if(this->ptr->facet.refs > 1) {
        locale__Locimp *new_ptr = operator_new(sizeof(locale__Locimp));
        locale__Locimp_copy_ctor(new_ptr, this->ptr);
        call_locale_facet__Decref(&this->ptr->facet);
        this->ptr = new_ptr;
    }

    locale__Locimp__Addfac(this->ptr, facet, id);

    if(catmask) {
        locale_string_char_dtor(&this->ptr->name);
        locale_string_char_ctor_cstr(&this->ptr->name, "*");
    }
    return this;
}

/* ?_Getfacet@locale@std@@QBEPBVfacet@12@I_N@Z */
/* ?_Getfacet@locale@std@@QEBAPEBVfacet@12@_K_N@Z */
DEFINE_THISCALL_WRAPPER(locale__Getfacet_bool, 12)
const locale_facet* __thiscall locale__Getfacet_bool(const locale *this,
        size_t id, bool allow_transparent)
{
    locale_facet *fac;

    TRACE("(%p %Iu)\n", this, id);

    fac = id < this->ptr->facet_cnt ? this->ptr->facetvec[id] : NULL;
    if(fac || !this->ptr->transparent || !allow_transparent)
        return fac;

    return id < global_locale->facet_cnt ? global_locale->facetvec[id] : NULL;
}

/* ?_Getfacet@locale@std@@QBEPBVfacet@12@I@Z */
/* ?_Getfacet@locale@std@@QEBAPEBVfacet@12@_K@Z */
DEFINE_THISCALL_WRAPPER(locale__Getfacet, 8)
const locale_facet* __thiscall locale__Getfacet(const locale *this, size_t id)
{
    return locale__Getfacet_bool( this, id, TRUE );
}

/* ?_Getgloballocale@locale@std@@CAPAV_Locimp@12@XZ */
/* ?_Getgloballocale@locale@std@@CAPEAV_Locimp@12@XZ */
locale__Locimp* __cdecl locale__Getgloballocale(void)
{
    TRACE("\n");
    return global_locale;
}

/* ?_Setgloballocale@locale@std@@CAXPAX@Z */
/* ?_Setgloballocale@locale@std@@CAXPEAX@Z */
void __cdecl locale__Setgloballocale(void *locimp)
{
    TRACE("(%p)\n", locimp);
    global_locale = locimp;
}

/* ?classic@locale@std@@SAABV12@XZ */
/* ?classic@locale@std@@SAAEBV12@XZ */
const locale* __cdecl locale_classic(void)
{
    TRACE("\n");
    locale__Init();
    return &classic_locale;
}

/* ?empty@locale@std@@SA?AV12@XZ */
locale* __cdecl locale_empty(locale *ret)
{
    TRACE("\n");

    locale__Init();

    ret->ptr = operator_new(sizeof(locale__Locimp));
    locale__Locimp_ctor_transparent(ret->ptr, TRUE);
    return ret;
}

/* ?global@locale@std@@SA?AV12@ABV12@@Z */
/* ?global@locale@std@@SA?AV12@AEBV12@@Z */
locale* __cdecl locale_global(locale *ret, const locale *loc)
{
    _Lockit lock;
    int i;

    TRACE("(%p %p)\n", loc, ret);

    _Lockit_ctor_locktype(&lock, _LOCK_LOCALE);
    locale_ctor(ret);

    if(loc->ptr != global_locale) {
        call_locale_facet__Decref(&global_locale->facet);
        global_locale = loc->ptr;
        call_locale_facet__Incref(&global_locale->facet);

        for(i=LC_ALL+1; i<=LC_MAX; i++) {
            if((global_locale->catmask & (1<<(i-1))) == 0)
                continue;
            setlocale(i, locale_string_char_c_str(&global_locale->name));
        }
    }
    _Lockit_dtor(&lock);
    return ret;
}

#if _MSVCP_VER < 100

/* ?_Getname@_Locinfo@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?_Getname@_Locinfo@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getname, 8)
basic_string_char* __thiscall _Locinfo__Getname(const _Locinfo *this, basic_string_char *ret)
{
    TRACE("(%p)\n", this);

    MSVCP_basic_string_char_copy_ctor(ret, &this->newlocname);
    return ret;
}

/* ?name@locale@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?name@locale@std@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(locale_name, 8)
basic_string_char* __thiscall locale_name(const locale *this, basic_string_char *ret)
{
    TRACE( "(%p)\n", this);
    MSVCP_basic_string_char_copy_ctor(ret, &this->ptr->name);
    return ret;
}

#else

/* ?_Getname@_Locinfo@std@@QBEPBDXZ */
/* ?_Getname@_Locinfo@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Locinfo__Getname, 4)
const char * __thiscall _Locinfo__Getname( const _Locinfo *this )
{
    return locale_string_char_c_str( &this->newlocname );
}

#endif  /* _MSVCP_VER < 100 */

/* wctrans */
wctrans_t __cdecl wctrans(const char *property)
{
    static const char str_tolower[] = "tolower";
    static const char str_toupper[] = "toupper";

    if(!strcmp(property, str_tolower))
        return 2;
    if(!strcmp(property, str_toupper))
        return 1;
    return 0;
}

/* towctrans */
wint_t __cdecl towctrans(wint_t c, wctrans_t category)
{
    if(category == 1)
        return towupper(c);
    return towlower(c);
}

#if _MSVCP_VER <= 71
/* btowc */
wint_t __cdecl btowc(int c)
{
    wchar_t ret;
    int state = 0;
    char ch = c;

    if (c == EOF || _Mbrtowc( &ret, &ch, 1, &state, NULL ) != 1) return WEOF;
    return ret;
}

/* mbrlen */
size_t __cdecl mbrlen(const char *str, size_t n, mbstate_t *state)
{
    static int local_state;

    if (!state) state = &local_state;
    return _Mbrtowc( NULL, str, n, state, NULL );
}

/* mbrtowc */
size_t __cdecl mbrtowc(wchar_t *dst, const char *str, size_t n, mbstate_t *state)
{
    static int local_state;

    if (!state) state = &local_state;
    return _Mbrtowc( dst, str, n, state, NULL );
}

/* mbsrtowcs */
size_t __cdecl mbsrtowcs(wchar_t *dst, const char **pstr, size_t n, mbstate_t *state)
{
    static int local_state;
    size_t ret = 0;
    wchar_t wc;
    const char *src;

    src = *pstr;
    if (!state) state = &local_state;

    while (!dst || n > ret)
    {
        int len = _Mbrtowc( &wc, src, 2, state, NULL );
        if (len < 0) return -1;
        if (!len) break;
        if (dst) dst[ret] = wc;
        ret++;
        if (!wc) break;
        src += len;
    }
    return ret;
}

/* wctob */
int __cdecl wctob(wint_t wc)
{
    char ret[MB_LEN_MAX];

    if (wc == WEOF || _Wcrtomb( ret, wc, NULL, NULL ) != 1) return EOF;
    return ret[0];
}

/* wcrtomb */
size_t __cdecl wcrtomb(char *dst, wchar_t wc, mbstate_t *state)
{
    return _Wcrtomb( dst, wc, state, NULL );
}

/* wcsrtombs */
size_t __cdecl wcsrtombs(char *dst, const wchar_t **pstr, size_t n, mbstate_t *state)
{
    const wchar_t *src;
    char buffer[MB_LEN_MAX];
    size_t ret = 0;

    src = *pstr;

    while (!dst || n > ret)
    {
        int len = _Wcrtomb( buffer, *src, state, NULL );
        if (len <= 0) return -1;
        if (n < ret + len) break;
        memcpy( dst + ret, buffer, len );
        ret += len;
        if (!buffer[0]) break;
        src++;
    }
    return ret;
}
#endif

int __cdecl _To_byte(const wchar_t *src, char *dst)
{
    TRACE("(%s %p)\n", debugstr_w(src), dst);
    return WideCharToMultiByte(CP_ACP, 0, src, -1, dst, MAX_PATH, NULL, NULL);
}

int __cdecl _To_wide(const char *src, wchar_t *dst)
{
    TRACE("(%s %p)\n", debugstr_a(src), dst);
    return MultiByteToWideChar(CP_ACP, 0, src, -1, dst, MAX_PATH);
}

size_t __cdecl _Strxfrm(char *dest, char *dest_end,
        const char *src, const char *src_end, _Collvec *coll)
{
    size_t dest_len = dest_end - dest;
    size_t src_len = src_end - src;
    _Collvec cv;
    WCHAR *buf;
    LCID lcid;
    int len;

    TRACE("(%p %p %p %p %p)\n", dest, dest_end, src, src_end, coll);

    if (coll) cv = *coll;
    else getcoll(&cv);

#if _MSVCP_VER < 110
    lcid = cv.handle;
#else
    lcid = LocaleNameToLCID(cv.lc_name, 0);
#endif

    if (!lcid && !cv.page)
    {
        if (src_len > dest_len) return src_len;
        memcpy(dest, src, src_len);
        return src_len;
    }

    len = MultiByteToWideChar(cv.page, MB_ERR_INVALID_CHARS, src, src_len, NULL, 0);
    if (!len) return INT_MAX;
    buf = heap_alloc(len * sizeof(WCHAR));
    if (!buf) return INT_MAX;
    MultiByteToWideChar(cv.page, MB_ERR_INVALID_CHARS, src, src_len, buf, len);

    len = LCMapStringW(lcid, LCMAP_SORTKEY, buf, len, NULL, 0);
    if (len <= dest_len)
        LCMapStringW(lcid, LCMAP_SORTKEY, buf, len, (WCHAR*)dest, dest_len);
    heap_free(buf);
    return len;
}

size_t __cdecl _Wcsxfrm(wchar_t *dest, wchar_t *dest_end,
        const wchar_t *src, const wchar_t *src_end, _Collvec *coll)
{
    size_t dest_len = dest_end - dest;
    size_t src_len = src_end - src;
    _Collvec cv;
    LCID lcid;
    int i, len;

    TRACE("(%p %p %p %p %p)\n", dest, dest_end, src, src_end, coll);

    if (coll) cv = *coll;
    else getcoll(&cv);

#if _MSVCP_VER < 110
    lcid = cv.handle;
#else
    lcid = LocaleNameToLCID(cv.lc_name, 0);
#endif

    if (!lcid)
    {
        if (src_len > dest_len) return src_len;
        memcpy(dest, src, src_len * sizeof(wchar_t));
        return src_len;
    }

    len = LCMapStringW(lcid, LCMAP_SORTKEY, src, src_len, NULL, 0);
    if (!len) return INT_MAX;
    if (len > dest_len) return len;

    LCMapStringW(lcid, LCMAP_SORTKEY, src, src_len, dest, dest_len);
    for (i = len - 1; i >= 0; i--)
        dest[i] = ((BYTE*)dest)[i];
    return len;
}

DEFINE_RTTI_DATA0(_Facet_base, 0, ".?AV_Facet_base@std@@")
DEFINE_RTTI_DATA0(locale_facet, 0, ".?AVfacet@locale@std@@")
DEFINE_RTTI_DATA1(locale__Locimp, 0, &locale_facet_rtti_base_descriptor, ".?AV_Locimp@locale@std@@")
DEFINE_RTTI_DATA1(collate_char, 0, &locale_facet_rtti_base_descriptor, ".?AV?$collate@D@std@@")
DEFINE_RTTI_DATA1(collate_wchar, 0, &locale_facet_rtti_base_descriptor, ".?AV?$collate@_W@std@@")
DEFINE_RTTI_DATA1(collate_short, 0, &locale_facet_rtti_base_descriptor, ".?AV?$collate@G@std@@")
DEFINE_RTTI_DATA1(ctype_base, 0, &locale_facet_rtti_base_descriptor, ".?AUctype_base@std@@")
DEFINE_RTTI_DATA2(ctype_char, 0, &ctype_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$ctype@D@std@@")
DEFINE_RTTI_DATA2(ctype_wchar, 0, &ctype_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$ctype@_W@std@@")
DEFINE_RTTI_DATA2(ctype_short, 0, &ctype_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$ctype@G@std@@")
DEFINE_RTTI_DATA1(codecvt_base, 0, &locale_facet_rtti_base_descriptor, ".?AVcodecvt_base@std@@")
#if _MSVCP_VER >= 140
DEFINE_RTTI_DATA2(codecvt_char, 0, &codecvt_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$codecvt@DDU_Mbstatet@@@std@@")
DEFINE_RTTI_DATA2(codecvt_char16, 0, &codecvt_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$codecvt@_SDU@std@@")
DEFINE_RTTI_DATA2(codecvt_char32, 0, &codecvt_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$codecvt@_UDU@std@@")
DEFINE_RTTI_DATA2(codecvt_wchar, 0, &codecvt_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$codecvt@_WDU_Mbstatet@@@std@@")
DEFINE_RTTI_DATA2(codecvt_short, 0, &codecvt_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$codecvt@GDU_Mbstatet@@@std@@")
#else
DEFINE_RTTI_DATA2(codecvt_char, 0, &codecvt_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$codecvt@DDH@std@@")
DEFINE_RTTI_DATA2(codecvt_wchar, 0, &codecvt_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$codecvt@_WDH@std@@")
DEFINE_RTTI_DATA2(codecvt_short, 0, &codecvt_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$codecvt@GDH@std@@")
#endif
DEFINE_RTTI_DATA1(numpunct_char, 0, &locale_facet_rtti_base_descriptor, ".?AV?$numpunct@D@std@@")
DEFINE_RTTI_DATA1(numpunct_wchar, 0, &locale_facet_rtti_base_descriptor, ".?AV?$numpunct@_W@std@@")
DEFINE_RTTI_DATA1(numpunct_short, 0, &locale_facet_rtti_base_descriptor, ".?AV?$numpunct@G@std@@")
DEFINE_RTTI_DATA1(num_get_char, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(num_get_wchar, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(num_get_short, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(num_put_char, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(num_put_wchar, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(num_put_short, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(time_put_char, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(time_put_wchar, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_put@_WV?$ostreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(time_put_short, 0, &locale_facet_rtti_base_descriptor, ".?AV?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@")
DEFINE_RTTI_DATA1(time_base, 0, &locale_facet_rtti_base_descriptor, ".?AUtime_base@std@@")
DEFINE_RTTI_DATA2(time_get_char, 0, &time_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@")
DEFINE_RTTI_DATA2(time_get_wchar, 0, &time_base_rtti_base_descriptor, &locale_facet_rtti_base_descriptor, ".?AV?$time_get@_WV?$istreambuf_iterator@_WU?$char_traits@_W@std@@@std@@@std@@")

__ASM_BLOCK_BEGIN(locale_vtables)
    __ASM_VTABLE(_Facet_base,
            VTABLE_ADD_FUNC(locale_facet_vector_dtor)
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Incref)
            );
    __ASM_VTABLE(locale_facet,
            VTABLE_ADD_FUNC(locale_facet_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            );
    __ASM_VTABLE(locale__Locimp,
            VTABLE_ADD_FUNC(locale__Locimp_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            );
    __ASM_VTABLE(collate_char,
            VTABLE_ADD_FUNC(collate_char_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(collate_char_do_compare)
            VTABLE_ADD_FUNC(collate_char_do_transform)
            VTABLE_ADD_FUNC(collate_char_do_hash));
    __ASM_VTABLE(collate_wchar,
            VTABLE_ADD_FUNC(collate_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(collate_wchar_do_compare)
            VTABLE_ADD_FUNC(collate_wchar_do_transform)
            VTABLE_ADD_FUNC(collate_wchar_do_hash));
    __ASM_VTABLE(collate_short,
            VTABLE_ADD_FUNC(collate_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(collate_wchar_do_compare)
            VTABLE_ADD_FUNC(collate_wchar_do_transform)
            VTABLE_ADD_FUNC(collate_wchar_do_hash));
    __ASM_VTABLE(ctype_base,
            VTABLE_ADD_FUNC(ctype_base_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            );
    __ASM_VTABLE(ctype_char,
            VTABLE_ADD_FUNC(ctype_char_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(ctype_char_do_tolower)
            VTABLE_ADD_FUNC(ctype_char_do_tolower_ch)
            VTABLE_ADD_FUNC(ctype_char_do_toupper)
            VTABLE_ADD_FUNC(ctype_char_do_toupper_ch)
            VTABLE_ADD_FUNC(ctype_char_do_widen)
            VTABLE_ADD_FUNC(ctype_char_do_widen_ch)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
            VTABLE_ADD_FUNC(ctype_char__Do_widen_s)
            VTABLE_ADD_FUNC(ctype_char_do_narrow)
            VTABLE_ADD_FUNC(ctype_char_do_narrow_ch)
            VTABLE_ADD_FUNC(ctype_char__Do_narrow_s)
#else
            VTABLE_ADD_FUNC(ctype_char_do_narrow)
            VTABLE_ADD_FUNC(ctype_char_do_narrow_ch)
#endif
            );
    __ASM_VTABLE(ctype_wchar,
            VTABLE_ADD_FUNC(ctype_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(ctype_wchar_do_is)
            VTABLE_ADD_FUNC(ctype_wchar_do_is_ch)
            VTABLE_ADD_FUNC(ctype_wchar_do_scan_is)
            VTABLE_ADD_FUNC(ctype_wchar_do_scan_not)
            VTABLE_ADD_FUNC(ctype_wchar_do_tolower)
            VTABLE_ADD_FUNC(ctype_wchar_do_tolower_ch)
            VTABLE_ADD_FUNC(ctype_wchar_do_toupper)
            VTABLE_ADD_FUNC(ctype_wchar_do_toupper_ch)
            VTABLE_ADD_FUNC(ctype_wchar_do_widen)
            VTABLE_ADD_FUNC(ctype_wchar_do_widen_ch)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
            VTABLE_ADD_FUNC(ctype_wchar__Do_widen_s)
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow)
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow_ch)
            VTABLE_ADD_FUNC(ctype_wchar__Do_narrow_s)
#else
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow)
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow_ch)
#endif
            );
    __ASM_VTABLE(ctype_short,
            VTABLE_ADD_FUNC(ctype_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(ctype_wchar_do_is)
            VTABLE_ADD_FUNC(ctype_wchar_do_is_ch)
            VTABLE_ADD_FUNC(ctype_wchar_do_scan_is)
            VTABLE_ADD_FUNC(ctype_wchar_do_scan_not)
            VTABLE_ADD_FUNC(ctype_wchar_do_tolower)
            VTABLE_ADD_FUNC(ctype_wchar_do_tolower_ch)
            VTABLE_ADD_FUNC(ctype_wchar_do_toupper)
            VTABLE_ADD_FUNC(ctype_wchar_do_toupper_ch)
            VTABLE_ADD_FUNC(ctype_wchar_do_widen)
            VTABLE_ADD_FUNC(ctype_wchar_do_widen_ch)
#if _MSVCP_VER >= 80 && _MSVCP_VER <= 90
            VTABLE_ADD_FUNC(ctype_wchar__Do_widen_s)
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow)
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow_ch)
            VTABLE_ADD_FUNC(ctype_wchar__Do_narrow_s)
#else
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow)
            VTABLE_ADD_FUNC(ctype_wchar_do_narrow_ch)
#endif
            );
    __ASM_VTABLE(codecvt_base,
            VTABLE_ADD_FUNC(codecvt_base_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(codecvt_base_do_always_noconv)
            VTABLE_ADD_FUNC(codecvt_base_do_max_length)
            VTABLE_ADD_FUNC(codecvt_base_do_encoding));
    __ASM_VTABLE(codecvt_char,
            VTABLE_ADD_FUNC(codecvt_char_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
            VTABLE_ADD_FUNC(codecvt_char_do_always_noconv)
#else
            VTABLE_ADD_FUNC(codecvt_base_do_always_noconv)
#endif
            VTABLE_ADD_FUNC(codecvt_base_do_max_length)
            VTABLE_ADD_FUNC(codecvt_base_do_encoding)
            VTABLE_ADD_FUNC(codecvt_char_do_in)
            VTABLE_ADD_FUNC(codecvt_char_do_out)
            VTABLE_ADD_FUNC(codecvt_char_do_unshift)
            VTABLE_ADD_FUNC(codecvt_char_do_length));
#if _MSVCP_VER >= 140
    __ASM_VTABLE(codecvt_char16,
            VTABLE_ADD_FUNC(codecvt_char16_vector_dtor)
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
            VTABLE_ADD_FUNC(codecvt_char16_do_always_noconv)
            VTABLE_ADD_FUNC(codecvt_char16_do_max_length)
            VTABLE_ADD_FUNC(codecvt_char16_do_encoding)
            VTABLE_ADD_FUNC(codecvt_char16_do_in)
            VTABLE_ADD_FUNC(codecvt_char16_do_out)
            VTABLE_ADD_FUNC(codecvt_char16_do_unshift)
            VTABLE_ADD_FUNC(codecvt_char16_do_length));
    __ASM_VTABLE(codecvt_char32,
            VTABLE_ADD_FUNC(codecvt_char32_vector_dtor)
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
            VTABLE_ADD_FUNC(codecvt_char32_do_always_noconv)
            VTABLE_ADD_FUNC(codecvt_char32_do_max_length)
            VTABLE_ADD_FUNC(codecvt_char32_do_encoding)
            VTABLE_ADD_FUNC(codecvt_char32_do_in)
            VTABLE_ADD_FUNC(codecvt_char32_do_out)
            VTABLE_ADD_FUNC(codecvt_char32_do_unshift)
            VTABLE_ADD_FUNC(codecvt_char32_do_length));
#endif
    __ASM_VTABLE(codecvt_wchar,
            VTABLE_ADD_FUNC(codecvt_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(codecvt_wchar_do_always_noconv)
            VTABLE_ADD_FUNC(codecvt_wchar_do_max_length)
#if _MSVCP_VER >= 90 && _MSVCP_VER <= 100
            VTABLE_ADD_FUNC(codecvt_base_do_encoding)
#else
            VTABLE_ADD_FUNC(codecvt_wchar_do_encoding)
#endif
            VTABLE_ADD_FUNC(codecvt_wchar_do_in)
            VTABLE_ADD_FUNC(codecvt_wchar_do_out)
            VTABLE_ADD_FUNC(codecvt_wchar_do_unshift)
            VTABLE_ADD_FUNC(codecvt_wchar_do_length));
    __ASM_VTABLE(codecvt_short,
            VTABLE_ADD_FUNC(codecvt_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(codecvt_wchar_do_always_noconv)
            VTABLE_ADD_FUNC(codecvt_wchar_do_max_length)
#if _MSVCP_VER >= 90 && _MSVCP_VER <= 100
            VTABLE_ADD_FUNC(codecvt_base_do_encoding)
#else
            VTABLE_ADD_FUNC(codecvt_wchar_do_encoding)
#endif
            VTABLE_ADD_FUNC(codecvt_wchar_do_in)
            VTABLE_ADD_FUNC(codecvt_wchar_do_out)
            VTABLE_ADD_FUNC(codecvt_wchar_do_unshift)
            VTABLE_ADD_FUNC(codecvt_wchar_do_length));
    __ASM_VTABLE(numpunct_char,
            VTABLE_ADD_FUNC(numpunct_char_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(numpunct_char_do_decimal_point)
            VTABLE_ADD_FUNC(numpunct_char_do_thousands_sep)
            VTABLE_ADD_FUNC(numpunct_char_do_grouping)
            VTABLE_ADD_FUNC(numpunct_char_do_falsename)
            VTABLE_ADD_FUNC(numpunct_char_do_truename));
    __ASM_VTABLE(numpunct_wchar,
            VTABLE_ADD_FUNC(numpunct_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(numpunct_wchar_do_decimal_point)
            VTABLE_ADD_FUNC(numpunct_wchar_do_thousands_sep)
            VTABLE_ADD_FUNC(numpunct_wchar_do_grouping)
            VTABLE_ADD_FUNC(numpunct_wchar_do_falsename)
            VTABLE_ADD_FUNC(numpunct_wchar_do_truename));
    __ASM_VTABLE(numpunct_short,
            VTABLE_ADD_FUNC(numpunct_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(numpunct_wchar_do_decimal_point)
            VTABLE_ADD_FUNC(numpunct_wchar_do_thousands_sep)
            VTABLE_ADD_FUNC(numpunct_wchar_do_grouping)
            VTABLE_ADD_FUNC(numpunct_wchar_do_falsename)
            VTABLE_ADD_FUNC(numpunct_wchar_do_truename));
    __ASM_VTABLE(num_get_char,
            VTABLE_ADD_FUNC(num_get_char_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(num_get_char_do_get_void)
            VTABLE_ADD_FUNC(num_get_char_do_get_double)
            VTABLE_ADD_FUNC(num_get_char_do_get_double)
            VTABLE_ADD_FUNC(num_get_char_do_get_float)
            VTABLE_ADD_FUNC(num_get_char_do_get_uint64)
            VTABLE_ADD_FUNC(num_get_char_do_get_int64)
            VTABLE_ADD_FUNC(num_get_char_do_get_ulong)
            VTABLE_ADD_FUNC(num_get_char_do_get_long)
            VTABLE_ADD_FUNC(num_get_char_do_get_uint)
            VTABLE_ADD_FUNC(num_get_char_do_get_ushort)
            VTABLE_ADD_FUNC(num_get_char_do_get_bool));
    __ASM_VTABLE(num_get_short,
            VTABLE_ADD_FUNC(num_get_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(num_get_short_do_get_void)
            VTABLE_ADD_FUNC(num_get_short_do_get_double)
            VTABLE_ADD_FUNC(num_get_short_do_get_double)
            VTABLE_ADD_FUNC(num_get_short_do_get_float)
            VTABLE_ADD_FUNC(num_get_short_do_get_uint64)
            VTABLE_ADD_FUNC(num_get_short_do_get_int64)
            VTABLE_ADD_FUNC(num_get_short_do_get_ulong)
            VTABLE_ADD_FUNC(num_get_short_do_get_long)
            VTABLE_ADD_FUNC(num_get_short_do_get_uint)
            VTABLE_ADD_FUNC(num_get_short_do_get_ushort)
            VTABLE_ADD_FUNC(num_get_short_do_get_bool));
    __ASM_VTABLE(num_get_wchar,
            VTABLE_ADD_FUNC(num_get_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(num_get_wchar_do_get_void)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_double)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_double)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_float)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_uint64)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_int64)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_ulong)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_long)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_uint)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_ushort)
            VTABLE_ADD_FUNC(num_get_wchar_do_get_bool));
    __ASM_VTABLE(num_put_char,
            VTABLE_ADD_FUNC(num_put_char_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(num_put_char_do_put_ptr)
            VTABLE_ADD_FUNC(num_put_char_do_put_double)
            VTABLE_ADD_FUNC(num_put_char_do_put_double)
            VTABLE_ADD_FUNC(num_put_char_do_put_uint64)
            VTABLE_ADD_FUNC(num_put_char_do_put_int64)
            VTABLE_ADD_FUNC(num_put_char_do_put_ulong)
            VTABLE_ADD_FUNC(num_put_char_do_put_long)
            VTABLE_ADD_FUNC(num_put_char_do_put_bool));
    __ASM_VTABLE(num_put_wchar,
            VTABLE_ADD_FUNC(num_put_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(num_put_wchar_do_put_ptr)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_double)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_double)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_uint64)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_int64)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_ulong)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_long)
            VTABLE_ADD_FUNC(num_put_wchar_do_put_bool));
    __ASM_VTABLE(num_put_short,
            VTABLE_ADD_FUNC(num_put_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(num_put_short_do_put_ptr)
            VTABLE_ADD_FUNC(num_put_short_do_put_double)
            VTABLE_ADD_FUNC(num_put_short_do_put_double)
            VTABLE_ADD_FUNC(num_put_short_do_put_uint64)
            VTABLE_ADD_FUNC(num_put_short_do_put_int64)
            VTABLE_ADD_FUNC(num_put_short_do_put_ulong)
            VTABLE_ADD_FUNC(num_put_short_do_put_long)
            VTABLE_ADD_FUNC(num_put_short_do_put_bool));
    __ASM_VTABLE(time_put_char,
            VTABLE_ADD_FUNC(time_put_char_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(time_put_char_do_put));
    __ASM_VTABLE(time_put_wchar,
            VTABLE_ADD_FUNC(time_put_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(time_put_wchar_do_put));
    __ASM_VTABLE(time_put_short,
            VTABLE_ADD_FUNC(time_put_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(time_put_wchar_do_put));
    __ASM_VTABLE(time_get_char,
            VTABLE_ADD_FUNC(time_get_char_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(time_get_char_do_date_order)
            VTABLE_ADD_FUNC(time_get_char_do_get_time)
            VTABLE_ADD_FUNC(time_get_char_do_get_date)
            VTABLE_ADD_FUNC(time_get_char_do_get_weekday)
            VTABLE_ADD_FUNC(time_get_char_do_get_monthname)
            VTABLE_ADD_FUNC(time_get_char_do_get_year)
#if _MSVCP_VER >= 100
            VTABLE_ADD_FUNC(time_get_char_do_get)
#endif
            );
    __ASM_VTABLE(time_get_wchar,
            VTABLE_ADD_FUNC(time_get_wchar_vector_dtor)
#if _MSVCP_VER >= 110
            VTABLE_ADD_FUNC(locale_facet__Incref)
            VTABLE_ADD_FUNC(locale_facet__Decref)
#endif
            VTABLE_ADD_FUNC(time_get_wchar_do_date_order)
            VTABLE_ADD_FUNC(time_get_wchar_do_get_time)
            VTABLE_ADD_FUNC(time_get_wchar_do_get_date)
            VTABLE_ADD_FUNC(time_get_wchar_do_get_weekday)
            VTABLE_ADD_FUNC(time_get_wchar_do_get_monthname)
            VTABLE_ADD_FUNC(time_get_wchar_do_get_year)
#if _MSVCP_VER >= 100
            VTABLE_ADD_FUNC(time_get_wchar_do_get)
#endif
            );
__ASM_BLOCK_END

void init_locale(void *base)
{
#ifdef RTTI_USE_RVA
    init__Facet_base_rtti(base);
    init_locale_facet_rtti(base);
    init_locale__Locimp_rtti(base);
    init_collate_char_rtti(base);
    init_collate_wchar_rtti(base);
    init_collate_short_rtti(base);
    init_ctype_base_rtti(base);
    init_ctype_char_rtti(base);
    init_ctype_wchar_rtti(base);
    init_ctype_short_rtti(base);
    init_codecvt_base_rtti(base);
    init_codecvt_char_rtti(base);
#if _MSVCP_VER >= 140
    init_codecvt_char16_rtti(base);
    init_codecvt_char32_rtti(base);
#endif
    init_codecvt_wchar_rtti(base);
    init_codecvt_short_rtti(base);
    init_numpunct_char_rtti(base);
    init_numpunct_wchar_rtti(base);
    init_numpunct_short_rtti(base);
    init_num_get_char_rtti(base);
    init_num_get_wchar_rtti(base);
    init_num_get_short_rtti(base);
    init_num_put_char_rtti(base);
    init_num_put_wchar_rtti(base);
    init_num_put_short_rtti(base);
    init_time_put_char_rtti(base);
    init_time_put_wchar_rtti(base);
    init_time_put_short_rtti(base);
    init_time_base_rtti(base);
    init_time_get_char_rtti(base);
    init_time_get_wchar_rtti(base);
#endif
}

void free_locale(void)
{
    facets_elem *iter, *safe;

    if(global_locale) {
        locale_dtor(&classic_locale);
        locale__Locimp_dtor(global_locale);
        operator_delete(global_locale);
    }

    LIST_FOR_EACH_ENTRY_SAFE(iter, safe, &lazy_facets, facets_elem, entry) {
        list_remove(&iter->entry);
        if(call_locale_facet__Decref(iter->fac))
            call_locale_facet_vector_dtor(iter->fac, 1);
        operator_delete(iter);
    }
}
