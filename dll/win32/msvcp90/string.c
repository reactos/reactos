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

#include "msvcp90.h"

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

#if _MSVCP_VER <= 90
#define STRING_ALLOCATOR(this) ((this)->allocator)
#elif _MSVCP_VER == 100
#define STRING_ALLOCATOR(this) (&(this)->allocator)
#else
#define STRING_ALLOCATOR(this) NULL
#endif

/* size_t_noverify structure */
typedef struct {
    size_t val;
} size_t_noverify;

/* allocator class */
typedef struct {
    char empty_struct;
} allocator;

/* char_traits<char> */
/* ?assign@?$char_traits@D@std@@SAXAADABD@Z */
/* ?assign@?$char_traits@D@std@@SAXAEADAEBD@Z */
void CDECL MSVCP_char_traits_char_assign(char *ch, const char *assign)
{
    *ch = *assign;
}

/* ?eq@?$char_traits@D@std@@SA_NABD0@Z */
/* ?eq@?$char_traits@D@std@@SA_NAEBD0@Z */
bool CDECL MSVCP_char_traits_char_eq(const char *ch1, const char *ch2)
{
    return *ch1 == *ch2;
}

/* ?lt@?$char_traits@D@std@@SA_NABD0@Z */
/* ?lt@?$char_traits@D@std@@SA_NAEBD0@Z */
bool CDECL MSVCP_char_traits_lt(const char *ch1, const char *ch2)
{
    return *ch1 < *ch2;
}

/* ?compare@?$char_traits@D@std@@SAHPBD0I@Z */
/* ?compare@?$char_traits@D@std@@SAHPEBD0_K@Z */
int CDECL MSVCP_char_traits_char_compare(
        const char *s1, const char *s2, size_t count)
{
    int ret = memcmp(s1, s2, count);
    return (ret>0 ? 1 : (ret<0 ? -1 : 0));
}

/* ?length@?$char_traits@D@std@@SAIPBD@Z */
/* ?length@?$char_traits@D@std@@SA_KPEBD@Z */
size_t CDECL MSVCP_char_traits_char_length(const char *str)
{
    return strlen(str);
}

/* ?_Copy_s@?$char_traits@D@std@@SAPADPADIPBDI@Z */
/* ?_Copy_s@?$char_traits@D@std@@SAPEADPEAD_KPEBD1@Z */
char* CDECL MSVCP_char_traits_char__Copy_s(char *dest,
        size_t size, const char *src, size_t count)
{
    memcpy_s(dest, size, src, count);
    return dest;
}

/* ?copy@?$char_traits@D@std@@SAPADPADPBDI@Z */
/* ?copy@?$char_traits@D@std@@SAPEADPEADPEBD_K@Z */
char* CDECL MSVCP_char_traits_char_copy(
        char *dest, const char *src, size_t count)
{
    return MSVCP_char_traits_char__Copy_s(dest, count, src, count);
}

/* ?find@?$char_traits@D@std@@SAPBDPBDIABD@Z */
/* ?find@?$char_traits@D@std@@SAPEBDPEBD_KAEBD@Z */
const char * CDECL MSVCP_char_traits_char_find(
        const char *str, size_t range, const char *c)
{
    return memchr(str, *c, range);
}

/* ?_Move_s@?$char_traits@D@std@@SAPADPADIPBDI@Z */
/* ?_Move_s@?$char_traits@D@std@@SAPEADPEAD_KPEBD1@Z */
char* CDECL MSVCP_char_traits_char__Move_s(char *dest,
        size_t size, const char *src, size_t count)
{
    memmove_s(dest, size, src, count);
    return dest;
}

/* ?move@?$char_traits@D@std@@SAPADPADPBDI@Z */
/* ?move@?$char_traits@D@std@@SAPEADPEADPEBD_K@Z */
char* CDECL MSVCP_char_traits_char_move(
        char *dest, const char *src, size_t count)
{
    return MSVCP_char_traits_char__Move_s(dest, count, src, count);
}

/* ?assign@?$char_traits@D@std@@SAPADPADID@Z */
/* ?assign@?$char_traits@D@std@@SAPEADPEAD_KD@Z */
char* CDECL MSVCP_char_traits_char_assignn(char *str, size_t num, char c)
{
    return memset(str, c, num);
}

/* ?to_char_type@?$char_traits@D@std@@SADABH@Z */
/* ?to_char_type@?$char_traits@D@std@@SADAEBH@Z */
char CDECL MSVCP_char_traits_char_to_char_type(const int *i)
{
    return (char)*i;
}

/* ?to_int_type@?$char_traits@D@std@@SAHABD@Z */
/* ?to_int_type@?$char_traits@D@std@@SAHAEBD@Z */
int CDECL MSVCP_char_traits_char_to_int_type(const char *ch)
{
    return (unsigned char)*ch;
}

/* ?eq_int_type@?$char_traits@D@std@@SA_NABH0@Z */
/* ?eq_int_type@?$char_traits@D@std@@SA_NAEBH0@Z */
bool CDECL MSVCP_char_traits_char_eq_int_type(const int *i1, const int *i2)
{
    return *i1 == *i2;
}

/* ?eof@?$char_traits@D@std@@SAHXZ */
int CDECL MSVCP_char_traits_char_eof(void)
{
    return EOF;
}

/* ?not_eof@?$char_traits@D@std@@SAHABH@Z */
/* ?not_eof@?$char_traits@D@std@@SAHAEBH@Z */
int CDECL MSVCP_char_traits_char_not_eof(int *in)
{
    return (*in==EOF ? !EOF : *in);
}


/* char_traits<wchar_t> */
/* ?assign@?$char_traits@_W@std@@SAXAA_WAB_W@Z */
/* ?assign@?$char_traits@_W@std@@SAXAEA_WAEB_W@Z */
void CDECL MSVCP_char_traits_wchar_assign(wchar_t *ch,
        const wchar_t *assign)
{
    *ch = *assign;
}

/* ?eq@?$char_traits@_W@std@@SA_NAB_W0@Z */
/* ?eq@?$char_traits@_W@std@@SA_NAEB_W0@Z */
bool CDECL MSVCP_char_traits_wchar_eq(wchar_t *ch1, wchar_t *ch2)
{
    return *ch1 == *ch2;
}

/* ?lt@?$char_traits@_W@std@@SA_NAB_W0@Z */
/* ?lt@?$char_traits@_W@std@@SA_NAEB_W0@Z */
bool CDECL MSVCP_char_traits_wchar_lt(const wchar_t *ch1,
        const wchar_t *ch2)
{
    return *ch1 < *ch2;
}

/* ?compare@?$char_traits@_W@std@@SAHPB_W0I@Z */
/* ?compare@?$char_traits@_W@std@@SAHPEB_W0_K@Z */
int CDECL MSVCP_char_traits_wchar_compare(const wchar_t *s1,
        const wchar_t *s2, size_t count)
{
    int ret = memcmp(s1, s2, count * sizeof(wchar_t));
    return (ret>0 ? 1 : (ret<0 ? -1 : 0));
}

/* ?length@?$char_traits@_W@std@@SAIPB_W@Z */
/* ?length@?$char_traits@_W@std@@SA_KPEB_W@Z */
size_t CDECL MSVCP_char_traits_wchar_length(const wchar_t *str)
{
    return wcslen((WCHAR*)str);
}

/* ?_Copy_s@?$char_traits@_W@std@@SAPA_WPA_WIPB_WI@Z */
/* ?_Copy_s@?$char_traits@_W@std@@SAPEA_WPEA_W_KPEB_W1@Z */
wchar_t* CDECL MSVCP_char_traits_wchar__Copy_s(wchar_t *dest,
        size_t size, const wchar_t *src, size_t count)
{
    memcpy_s(dest, size * sizeof(wchar_t), src, count * sizeof(wchar_t));
    return dest;
}

/* ?copy@?$char_traits@_W@std@@SAPA_WPA_WPB_WI@Z */
/* ?copy@?$char_traits@_W@std@@SAPEA_WPEA_WPEB_W_K@Z */
wchar_t* CDECL MSVCP_char_traits_wchar_copy(wchar_t *dest,
        const wchar_t *src, size_t count)
{
    return MSVCP_char_traits_wchar__Copy_s(dest, count, src, count);
}

/* ?find@?$char_traits@_W@std@@SAPB_WPB_WIAB_W@Z */
/* ?find@?$char_traits@_W@std@@SAPEB_WPEB_W_KAEB_W@Z */
const wchar_t* CDECL MSVCP_char_traits_wchar_find(
        const wchar_t *str, size_t range, const wchar_t *c)
{
    size_t i=0;

    for(i=0; i<range; i++)
        if(str[i] == *c)
            return str+i;

    return NULL;
}

/* ?_Move_s@?$char_traits@_W@std@@SAPA_WPA_WIPB_WI@Z */
/* ?_Move_s@?$char_traits@_W@std@@SAPEA_WPEA_W_KPEB_W1@Z */
wchar_t* CDECL MSVCP_char_traits_wchar__Move_s(wchar_t *dest,
        size_t size, const wchar_t *src, size_t count)
{
    memmove_s(dest, size * sizeof(wchar_t), src, count * sizeof(wchar_t));
    return dest;
}

/* ?move@?$char_traits@_W@std@@SAPA_WPA_WPB_WI@Z */
/* ?move@?$char_traits@_W@std@@SAPEA_WPEA_WPEB_W_K@Z */
wchar_t* CDECL MSVCP_char_traits_wchar_move(wchar_t *dest,
        const wchar_t *src, size_t count)
{
    return MSVCP_char_traits_wchar__Move_s(dest, count, src, count);
}

/* ?assign@?$char_traits@_W@std@@SAPA_WPA_WI_W@Z */
/* ?assign@?$char_traits@_W@std@@SAPEA_WPEA_W_K_W@Z */
wchar_t* CDECL MSVCP_char_traits_wchar_assignn(wchar_t *str,
        size_t num, wchar_t c)
{
    size_t i;

    for(i=0; i<num; i++)
        str[i] = c;

    return str;
}

/* ?to_char_type@?$char_traits@_W@std@@SA_WABG@Z */
/* ?to_char_type@?$char_traits@_W@std@@SA_WAEBG@Z */
wchar_t CDECL MSVCP_char_traits_wchar_to_char_type(const unsigned short *i)
{
    return *i;
}

/* ?to_int_type@?$char_traits@_W@std@@SAGAB_W@Z */
/* ?to_int_type@?$char_traits@_W@std@@SAGAEB_W@Z */
unsigned short CDECL MSVCP_char_traits_wchar_to_int_type(const wchar_t *ch)
{
    return *ch;
}

/* ?eq_int_type@?$char_traits@_W@std@@SA_NABG0@Z */
/* ?eq_int_type@?$char_traits@_W@std@@SA_NAEBG0@Z */
bool CDECL MSVCP_char_traits_wchar_eq_int_tpe(const unsigned short *i1,
        const unsigned short *i2)
{
    return *i1 == *i2;
}

/* ?eof@?$char_traits@_W@std@@SAGXZ */
unsigned short CDECL MSVCP_char_traits_wchar_eof(void)
{
    return WEOF;
}

/* ?not_eof@?$char_traits@_W@std@@SAGABG@Z */
/* ?not_eof@?$char_traits@_W@std@@SAGAEBG@Z */
unsigned short CDECL MSVCP_char_traits_wchar_not_eof(const unsigned short *in)
{
    return (*in==WEOF ? !WEOF : *in);
}


/* char_traits<unsigned short> */
/* ?assign@?$char_traits@G@std@@SAXAAGABG@Z */
/* ?assign@?$char_traits@G@std@@SAXAEAGAEBG@Z */
void CDECL MSVCP_char_traits_short_assign(unsigned short *ch,
        const unsigned short *assign)
{
    *ch = *assign;
}

/* ?eq@?$char_traits@G@std@@SA_NABG0@Z */
/* ?eq@?$char_traits@G@std@@SA_NAEBG0@Z */
bool CDECL MSVCP_char_traits_short_eq(const unsigned short *ch1,
        const unsigned short *ch2)
{
    return *ch1 == *ch2;
}

/* ?lt@?$char_traits@G@std@@SA_NABG0@Z */
/* ?lt@?$char_traits@G@std@@SA_NAEBG0@Z */
bool CDECL MSVCP_char_traits_short_lt(const unsigned short *ch1,
        const unsigned short *ch2)
{
    return *ch1 < *ch2;
}

/* ?compare@?$char_traits@G@std@@SAHPBG0I@Z */
/* ?compare@?$char_traits@G@std@@SAHPEBG0_K@Z */
int CDECL MSVCP_char_traits_short_compare(const unsigned short *s1,
        const unsigned short *s2, size_t count)
{
    size_t i;

    for(i=0; i<count; i++)
        if(s1[i] != s2[i])
            return (s1[i] < s2[i] ? -1 : 1);

    return 0;
}

/* ?length@?$char_traits@G@std@@SAIPBG@Z */
/* ?length@?$char_traits@G@std@@SA_KPEBG@Z */
size_t CDECL MSVCP_char_traits_short_length(const unsigned short *str)
{
    size_t len;

    for(len=0; str[len]; len++);

    return len;
}

/* ?_Copy_s@?$char_traits@G@std@@SAPAGPAGIPBGI@Z */
/* ?_Copy_s@?$char_traits@G@std@@SAPEAGPEAG_KPEBG1@Z */
unsigned short * CDECL MSVCP_char_traits_short__Copy_s(unsigned short *dest,
        size_t size, const unsigned short *src, size_t count)
{
    memcpy_s(dest, size * sizeof(unsigned short), src, count * sizeof(unsigned short));
    return dest;
}

/* ?copy@?$char_traits@G@std@@SAPAGPAGPBGI@Z */
/* ?copy@?$char_traits@G@std@@SAPEAGPEAGPEBG_K@Z */
unsigned short* CDECL MSVCP_char_traits_short_copy(unsigned short *dest,
        const unsigned short *src, size_t count)
{
    return MSVCP_char_traits_short__Copy_s(dest, count, src, count);
}

/* ?find@?$char_traits@G@std@@SAPBGPBGIABG@Z */
/* ?find@?$char_traits@G@std@@SAPEBGPEBG_KAEBG@Z */
const unsigned short* CDECL MSVCP_char_traits_short_find(
        const unsigned short *str, size_t range, const unsigned short *c)
{
    size_t i;

    for(i=0; i<range; i++)
        if(str[i] == *c)
            return str+i;

    return NULL;
}

/* ?_Move_s@?$char_traits@G@std@@SAPAGPAGIPBGI@Z */
/* ?_Move_s@?$char_traits@G@std@@SAPEAGPEAG_KPEBG1@Z */
unsigned short* CDECL MSVCP_char_traits_short__Move_s(unsigned short *dest,
        size_t size, const unsigned short *src, size_t count)
{
    memmove_s(dest, size * sizeof(unsigned short), src, count * sizeof(unsigned short));
    return dest;
}

/* ?move@?$char_traits@G@std@@SAPAGPAGPBGI@Z */
/* ?move@?$char_traits@G@std@@SAPEAGPEAGPEBG_K@Z */
unsigned short* CDECL MSVCP_char_traits_short_move(unsigned short *dest,
        const unsigned short *src, size_t count)
{
    return MSVCP_char_traits_short__Move_s(dest, count, src, count);
}

/* ?assign@?$char_traits@G@std@@SAPAGPAGIG@Z */
/* ?assign@?$char_traits@G@std@@SAPEAGPEAG_KG@Z */
unsigned short* CDECL MSVCP_char_traits_short_assignn(unsigned short *str,
        size_t num, unsigned short c)
{
    size_t i;

    for(i=0; i<num; i++)
        str[i] = c;

    return str;
}

/* ?to_char_type@?$char_traits@G@std@@SAGABG@Z */
/* ?to_char_type@?$char_traits@G@std@@SAGAEBG@Z */
unsigned short CDECL MSVCP_char_traits_short_to_char_type(const unsigned short *i)
{
    return *i;
}

/* ?to_int_type@?$char_traits@G@std@@SAGABG@Z */
/* ?to_int_type@?$char_traits@G@std@@SAGAEBG@Z */
unsigned short CDECL MSVCP_char_traits_short_to_int_type(const unsigned short *ch)
{
    return *ch;
}

/* ?eq_int_type@?$char_traits@G@std@@SA_NABG0@Z */
/* ?eq_int_type@?$char_traits@G@std@@SA_NAEBG0@Z */
bool CDECL MSVCP_char_traits_short_eq_int_type(unsigned short *i1,
        unsigned short *i2)
{
    return *i1 == *i2;
}

/* ?eof@?$char_traits@G@std@@SAGXZ */
unsigned short CDECL MSVCP_char_traits_short_eof(void)
{
    return -1;
}

/* ?not_eof@?$char_traits@G@std@@SAGABG@Z */
/* ?not_eof@?$char_traits@G@std@@SAGAEBG@Z */
unsigned short CDECL MSVCP_char_traits_short_not_eof(const unsigned short *in)
{
    return (*in==(unsigned short)-1 ? 0 : *in);
}


/* _String_base */
/* ?_Xlen@_String_base@std@@SAXXZ */
void  CDECL MSVCP__String_base_Xlen(void)
{
    TRACE("\n");
    _Xlength_error("string too long");
}

/* ?_Xlen@_String_base@std@@QBEXXZ */
DEFINE_THISCALL_WRAPPER(_String_base__Xlen, 4)
void __thiscall _String_base__Xlen(const void/*_String_base*/ *this)
{
    MSVCP__String_base_Xlen();
}

/* ?_Xran@_String_base@std@@SAXXZ */
void CDECL MSVCP__String_base_Xran(void)
{
    TRACE("\n");
    _Xout_of_range("invalid string position");
}

/* ?_Xran@_String_base@std@@QBEXXZ */
DEFINE_THISCALL_WRAPPER(_String_base__Xran, 4)
void __thiscall _String_base__Xran(const void/*_String_base*/ *this)
{
    MSVCP__String_base_Xran();
}

/* ?_Xinvarg@_String_base@std@@SAXXZ */
void CDECL MSVCP__String_base_Xinvarg(void)
{
    TRACE("\n");
    _Xinvalid_argument("invalid string argument");
}


/* basic_string<char, char_traits<char>, allocator<char>> */
/* ?npos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@2IB */
/* ?npos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@2_KB */
const size_t MSVCP_basic_string_char_npos = -1;

/* ?_Myptr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAEPADXZ */
/* ?_Myptr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAAPEADXZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_ptr, 4)
char* __thiscall basic_string_char_ptr(basic_string_char *this)
{
#ifndef __REACTOS__
    if(this->res < BUF_SIZE_CHAR)
        return this->data.buf;
#endif
    return this->data.ptr;
}

/* ?_Myptr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IBEPBDXZ */
/* ?_Myptr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_const_ptr, 4)
const char* __thiscall basic_string_char_const_ptr(const basic_string_char *this)
{
#ifndef __REACTOS__
    if(this->res < BUF_SIZE_CHAR)
        return this->data.buf;
#endif
    return this->data.ptr;
}

/* ?_Eos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAEXI@Z */
/* ?_Eos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_eos, 8)
void __thiscall basic_string_char_eos(basic_string_char *this, size_t len)
{
    static const char nullbyte = '\0';

    this->size = len;
    MSVCP_char_traits_char_assign(basic_string_char_ptr(this)+len, &nullbyte);
}

/* ?_Inside@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAE_NPBD@Z */
/* ?_Inside@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAA_NPEBD@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_inside, 8)
bool __thiscall basic_string_char_inside(
        basic_string_char *this, const char *ptr)
{
    char *cstr = basic_string_char_ptr(this);

    return ptr>=cstr && ptr<cstr+this->size;
}

/* ?_Tidy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAEX_NI@Z */
/* ?_Tidy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAAX_N_K@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_tidy, 12)
void __thiscall basic_string_char_tidy(basic_string_char *this,
        bool built, size_t new_size)
{
    if(built && BUF_SIZE_CHAR<=this->res) {
        char *ptr = this->data.ptr;

        if(new_size > 0)
            MSVCP_char_traits_char__Copy_s(this->data.buf, BUF_SIZE_CHAR, ptr, new_size);
        MSVCP_allocator_char_deallocate(STRING_ALLOCATOR(this), ptr, this->res+1);
    }

    this->res = BUF_SIZE_CHAR-1;
    basic_string_char_eos(this, new_size);
}

/* Exported only from msvcp60/70 */
/* ?_Tidy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEX_N@Z */
/* ?_Tidy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAAX_N@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_tidy_built, 8)
void __thiscall basic_string_char_tidy_built(basic_string_char *this, bool built)
{
    basic_string_char_tidy(this, built, 0);
}

/* ?_Grow@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAE_NI_N@Z */
/* ?_Grow@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAA_N_K_N@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_grow, 12)
bool __thiscall basic_string_char_grow(
        basic_string_char *this, size_t new_size, bool trim)
{
    if(this->res < new_size) {
        size_t new_res = new_size, len = this->size;
        char *ptr;

        new_res |= 0xf;

        if(new_res/3 < this->res/2)
            new_res = this->res + this->res/2;

        ptr = MSVCP_allocator_char_allocate(STRING_ALLOCATOR(this), new_res+1);
        if(!ptr)
            ptr = MSVCP_allocator_char_allocate(STRING_ALLOCATOR(this), new_size+1);
        else
            new_size = new_res;
        if(!ptr) {
            ERR("Out of memory\n");
            basic_string_char_tidy(this, TRUE, 0);
            return FALSE;
        }

        MSVCP_char_traits_char__Copy_s(ptr, new_size,
                basic_string_char_ptr(this), this->size);
        basic_string_char_tidy(this, TRUE, 0);
        this->data.ptr = ptr;
        this->res = new_size;
        basic_string_char_eos(this, len);
    } else if(trim && new_size < BUF_SIZE_CHAR)
        basic_string_char_tidy(this, TRUE,
                new_size<this->size ? new_size : this->size);
    else if(new_size == 0)
        basic_string_char_eos(this, 0);

    return (new_size>0);
}

/* ?_Copy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAEXII@Z */
/* ?_Copy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAAX_K0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char__Copy, 12)
void __thiscall basic_string_char__Copy(basic_string_char *this,
        size_t new_size, size_t copy_len)
{
    TRACE("%p %Iu %Iu\n", this, new_size, copy_len);

    if(!basic_string_char_grow(this, new_size, FALSE))
        return;
    basic_string_char_eos(this, copy_len);
}

/* ?get_allocator@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$allocator@D@2@XZ */
/* ?get_allocator@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$allocator@D@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_get_allocator, 8)
allocator* __thiscall basic_string_char_get_allocator(const basic_string_char *this, allocator *ret)
{
    TRACE("%p\n", this);
    return ret;
}

/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@II@Z */
/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_erase, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_erase(
        basic_string_char *this, size_t pos, size_t len)
{
    TRACE("%p %Iu %Iu\n", this, pos, len);

    if(pos > this->size)
        MSVCP__String_base_Xran();

    if(len > this->size-pos)
        len = this->size-pos;

    if(len) {
        MSVCP_char_traits_char__Move_s(basic_string_char_ptr(this)+pos,
                this->res-pos, basic_string_char_ptr(this)+pos+len,
                this->size-pos-len);
        basic_string_char_eos(this, this->size-len);
    }

    return this;
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ABV12@II@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@AEBV12@_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assign_substr, 16)
basic_string_char* __thiscall MSVCP_basic_string_char_assign_substr(
        basic_string_char *this, const basic_string_char *assign,
        size_t pos, size_t len)
{
    TRACE("%p %p %Iu %Iu\n", this, assign, pos, len);

    if(assign->size < pos)
        MSVCP__String_base_Xran();

    if(len > assign->size-pos)
        len = assign->size-pos;

    if(this == assign) {
        MSVCP_basic_string_char_erase(this, pos+len, MSVCP_basic_string_char_npos);
        MSVCP_basic_string_char_erase(this, 0, pos);
    } else if(basic_string_char_grow(this, len, FALSE)) {
        MSVCP_char_traits_char__Copy_s(basic_string_char_ptr(this),
                this->res, basic_string_char_const_ptr(assign)+pos, len);
        basic_string_char_eos(this, len);
    }

    return this;
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ABV12@@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@AEBV12@@Z */
/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@ABV01@@Z */
/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assign, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_assign(
        basic_string_char *this, const basic_string_char *assign)
{
    return MSVCP_basic_string_char_assign_substr(this, assign,
            0, MSVCP_basic_string_char_npos);
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBDI@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assign_cstr_len, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_assign_cstr_len(
        basic_string_char *this, const char *str, size_t len)
{
    TRACE("%p %s %Iu\n", this, debugstr_an(str, len), len);

    if(basic_string_char_inside(this, str))
        return MSVCP_basic_string_char_assign_substr(this, this,
                str-basic_string_char_ptr(this), len);
    else if(basic_string_char_grow(this, len, FALSE)) {
        MSVCP_char_traits_char__Copy_s(basic_string_char_ptr(this),
                this->res, str, len);
        basic_string_char_eos(this, len);
    }

    return this;
}

/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@D@Z */
/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@D@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assign_ch, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_assign_ch(
        basic_string_char *this, char ch)
{
    return MSVCP_basic_string_char_assign_cstr_len(this, &ch, 1);
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBD@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD@Z */
/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@PBD@Z */
/* ??4?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@PEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assign_cstr, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_assign_cstr(
        basic_string_char *this, const char *str)
{
    return MSVCP_basic_string_char_assign_cstr_len(this, str,
            MSVCP_char_traits_char_length(str));
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ID@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_KD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assignn, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_assignn(
        basic_string_char *this, size_t count, char ch)
{
    TRACE("%p %Iu %c\n", this, count, ch);

    basic_string_char_grow(this, count, FALSE);
    MSVCP_char_traits_char_assignn(basic_string_char_ptr(this), count, ch);
    basic_string_char_eos(this, count);
    return this;
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBD0@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assign_ptr_ptr, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_assign_ptr_ptr(
        basic_string_char *this, const char *first, const char *last)
{
    return MSVCP_basic_string_char_assign_cstr_len(this, first, last-first);
}

/* ?_Chassign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IAEXIID@Z */
/* ?_Chassign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IEAAX_K0D@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_Chassign, 16)
void __thiscall MSVCP_basic_string_char_Chassign(basic_string_char *this,
        size_t off, size_t count, char ch)
{
    TRACE("%p %Iu %Iu %c\n", this, off, count, ch);
    MSVCP_char_traits_char_assignn(basic_string_char_ptr(this)+off, count, ch);
}

/* ?_Copy_s@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPADIII@Z */
/* ?_Copy_s@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEAD_K11@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_Copy_s, 20)
size_t __thiscall MSVCP_basic_string_char_Copy_s(const basic_string_char *this,
        char *dest, size_t size, size_t count, size_t off)
{
    TRACE("%p %p %Iu %Iu %Iu\n", this, dest, size, count, off);

    if(this->size < off)
        MSVCP__String_base_Xran();

    if(count > this->size-off)
        count = this->size-off;

    MSVCP_char_traits_char__Copy_s(dest, size,
            basic_string_char_const_ptr(this)+off, count);
    return count;
}

/* ?copy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPADII@Z */
/* ?copy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEAD_K1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_copy, 16)
size_t __thiscall basic_string_char_copy(const basic_string_char *this,
        char *dest, size_t count, size_t off)
{
    return MSVCP_basic_string_char_Copy_s(this, dest, count, count, off);
}

/* ?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ */
/* ?c_str@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ */
/* ?data@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEPBDXZ */
/* ?data@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_c_str, 4)
const char* __thiscall MSVCP_basic_string_char_c_str(const basic_string_char *this)
{
    TRACE("%p\n", this);
    return basic_string_char_const_ptr(this);
}

/* ?capacity@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIXZ */
/* ?capacity@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_capacity, 4)
size_t __thiscall MSVCP_basic_string_char_capacity(basic_string_char *this)
{
    TRACE("%p\n", this);
    return this->res;
}

/* ?reserve@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXI@Z */
/* ?reserve@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_reserve, 8)
void __thiscall MSVCP_basic_string_char_reserve(basic_string_char *this, size_t size)
{
    size_t len;

    TRACE("%p %Iu\n", this, size);

    len = this->size;
    if(len > size)
        return;

    if(basic_string_char_grow(this, size, TRUE))
        basic_string_char_eos(this, len);
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor, 4)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor(basic_string_char *this)
{
    TRACE("%p\n", this);

    basic_string_char_tidy(this, FALSE, 0);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV01@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_copy_ctor, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_copy_ctor(
    basic_string_char *this, const basic_string_char *copy)
{
    TRACE("%p %p\n", this, copy);

    basic_string_char_tidy(this, FALSE, 0);
    MSVCP_basic_string_char_assign(this, copy);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBD@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_cstr, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_cstr(
        basic_string_char *this, const char *str)
{
    TRACE("%p %s\n", this, debugstr_a(str));

    basic_string_char_tidy(this, FALSE, 0);
    MSVCP_basic_string_char_assign_cstr(this, str);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBDABV?$allocator@D@1@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBDAEBV?$allocator@D@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_cstr_alloc, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_cstr_alloc(
        basic_string_char *this, const char *str, const void *alloc)
{
    return MSVCP_basic_string_char_ctor_cstr(this, str);
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBDI@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_cstr_len, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_cstr_len(
        basic_string_char *this, const char *str, size_t len)
{
    TRACE("%p %s %Iu\n", this, debugstr_an(str, len), len);

    basic_string_char_tidy(this, FALSE, 0);
    MSVCP_basic_string_char_assign_cstr_len(this, str, len);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBDIABV?$allocator@D@1@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBD_KAEBV?$allocator@D@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_cstr_len_alloc, 16)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_cstr_len_alloc(
        basic_string_char *this, const char *str, size_t len, const void *alloc)
{
    return MSVCP_basic_string_char_ctor_cstr_len(this, str, len);
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV01@II@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV01@_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_substr, 16)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_substr(
        basic_string_char *this, const basic_string_char *assign,
        size_t pos, size_t len)
{
    TRACE("%p %p %Iu %Iu\n", this, assign, pos, len);

    basic_string_char_tidy(this, FALSE, 0);
    MSVCP_basic_string_char_assign_substr(this, assign, pos, len);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV01@IIABV?$allocator@D@1@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV01@_K1AEBV?$allocator@D@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_substr_alloc, 20)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_substr_alloc(
        basic_string_char *this, const basic_string_char *assign,
        size_t pos, size_t len, const void *alloc)
{
    return MSVCP_basic_string_char_ctor_substr(this, assign, pos, len);
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV?$allocator@D@1@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@AEBV?$allocator@D@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_alloc, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_alloc(
        basic_string_char *this, const void *alloc)
{
    TRACE("%p %p\n", this, alloc);

    basic_string_char_tidy(this, FALSE, 0);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ID@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@_KD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_ch, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_ch(basic_string_char *this,
        size_t count, char ch)
{
    TRACE("%p %Iu %c\n", this, count, ch);

    basic_string_char_tidy(this, FALSE, 0);
    MSVCP_basic_string_char_assignn(this, count, ch);
    return this;
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@IDABV?$allocator@D@1@@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@_KDAEBV?$allocator@D@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_ch_alloc, 16)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_ch_alloc(basic_string_char *this,
        size_t count, char ch, const void *alloc)
{
    return MSVCP_basic_string_char_ctor_ch(this, count, ch);
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBD0@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@PEBD0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_ptr_ptr, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_ptr_ptr(basic_string_char *this,
        const char *first, const char *last)
{
    return MSVCP_basic_string_char_ctor_cstr_len(this, first, last-first);
}

/* ??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ */
/* ??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_dtor, 4)
void* __thiscall MSVCP_basic_string_char_dtor(basic_string_char *this)
{
    TRACE("%p\n", this);
    basic_string_char_tidy(this, TRUE, 0);
    return NULL;  /* FEAR 1 installer expects EAX set to 0 */
}

/* ?size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIXZ */
/* ?size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KXZ */
/* ?length@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIXZ */
/* ?length@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_length, 4)
size_t __thiscall MSVCP_basic_string_char_length(const basic_string_char *this)
{
    TRACE("%p\n", this);
    return this->size;
}

/* ?max_size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIXZ */
/* ?max_size@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_max_size, 4)
size_t __thiscall basic_string_char_max_size(const basic_string_char *this)
{
    TRACE("%p\n", this);
    return MSVCP_allocator_char_max_size(STRING_ALLOCATOR(this))-1;
}

/* ?empty@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE_NXZ */
/* ?empty@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_empty, 4)
bool __thiscall MSVCP_basic_string_char_empty(basic_string_char *this)
{
    TRACE("%p\n", this);
    return this->size == 0;
}

/* ?swap@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_swap, 8)
void __thiscall MSVCP_basic_string_char_swap(basic_string_char *this, basic_string_char *str)
{
    if(this != str) {
        char tmp[sizeof(this->data)];
        const size_t size = this->size;
        const size_t res = this->res;

        memcpy(tmp, this->data.buf, sizeof(this->data));
        memcpy(this->data.buf, str->data.buf, sizeof(this->data));
        memcpy(str->data.buf, tmp, sizeof(this->data));

        this->size = str->size;
        this->res = str->res;

        str->size = size;
        str->res = res;
    }
}

/* ?substr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV12@II@Z */
/* ?substr@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV12@_K0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_substr, 16)
basic_string_char* __thiscall MSVCP_basic_string_char_substr(basic_string_char *this,
        basic_string_char *ret, size_t off, size_t len)
{
    TRACE("%p %Iu %Iu\n", this, off, len);

    MSVCP_basic_string_char_ctor_substr(ret, this, off, len);
    return ret;
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ABV12@II@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@AEBV12@_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append_substr, 16)
basic_string_char* __thiscall MSVCP_basic_string_char_append_substr(basic_string_char *this,
        const basic_string_char *append, size_t offset, size_t count)
{
    TRACE("%p %p %Iu %Iu\n", this, append, offset, count);

    if(append->size < offset)
        MSVCP__String_base_Xran();

    if(count > append->size-offset)
        count = append->size-offset;

    if(MSVCP_basic_string_char_npos-this->size<=count || this->size+count<this->size)
        MSVCP__String_base_Xlen();

    if(basic_string_char_grow(this, this->size+count, FALSE)) {
        MSVCP_char_traits_char__Copy_s(basic_string_char_ptr(this)+this->size,
                this->res-this->size, basic_string_char_const_ptr(append)+offset, count);
        basic_string_char_eos(this, this->size+count);
    }

    return this;
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ABV12@@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@AEBV12@@Z */
/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@ABV01@@Z */
/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_append(
        basic_string_char *this, const basic_string_char *append)
{
    return MSVCP_basic_string_char_append_substr(this, append,
            0, MSVCP_basic_string_char_npos);
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBDI@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append_cstr_len, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_append_cstr_len(
        basic_string_char *this, const char *append, size_t count)
{
    TRACE("%p %s %Iu\n", this, debugstr_an(append, count), count);

    if(basic_string_char_inside(this, append))
        return MSVCP_basic_string_char_append_substr(this, this,
                append-basic_string_char_ptr(this), count);

    if(MSVCP_basic_string_char_npos-this->size<=count || this->size+count<this->size)
        MSVCP__String_base_Xlen();

    if(basic_string_char_grow(this, this->size+count, FALSE)) {
        MSVCP_char_traits_char__Copy_s(basic_string_char_ptr(this)+this->size,
                this->res-this->size, append, count);
        basic_string_char_eos(this, this->size+count);
    }

    return this;
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBD@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD@Z */
/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@PBD@Z */
/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@PEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append_cstr, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_append_cstr(
        basic_string_char *this, const char *append)
{
    return MSVCP_basic_string_char_append_cstr_len(this, append,
            MSVCP_char_traits_char_length(append));
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@PBD0@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@PEBD0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append_beg_end, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_append_beg_end(
        basic_string_char *this, const char *beg, const char *end)
{
    return MSVCP_basic_string_char_append_cstr_len(this, beg, end-beg);
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ID@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_KD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append_len_ch, 12)
basic_string_char* __thiscall MSVCP_basic_string_char_append_len_ch(
        basic_string_char *this, size_t count, char ch)
{
    TRACE("%p %Iu %c\n", this, count, ch);

    if(MSVCP_basic_string_char_npos-this->size <= count)
        MSVCP__String_base_Xlen();

    if(basic_string_char_grow(this, this->size+count, FALSE)) {
        MSVCP_char_traits_char_assignn(basic_string_char_ptr(this)+this->size, count, ch);
        basic_string_char_eos(this, this->size+count);
    }

    return this;
}

/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV01@D@Z */
/* ??Y?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV01@D@Z */
/* ?push_back@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXD@Z */
/* ?push_back@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append_ch, 8)
basic_string_char* __thiscall MSVCP_basic_string_char_append_ch(
        basic_string_char *this, char ch)
{
    return MSVCP_basic_string_char_append_len_ch(this, 1, ch);
}

/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@PBD@Z */
/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@PEBD@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@PBD@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@PEBD@Z */
basic_string_char* __cdecl MSVCP_basic_string_char_concatenate_bstr_cstr(basic_string_char *ret,
        const basic_string_char *left, const char *right)
{
    TRACE("%p %s\n", left, debugstr_a(right));

    MSVCP_basic_string_char_copy_ctor(ret, left);
    MSVCP_basic_string_char_append_cstr(ret, right);
    return ret;
}

/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBDABV10@@Z */
/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBDAEBV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBDABV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBDAEBV10@@Z */
basic_string_char* __cdecl MSVCP_basic_string_char_concatenate_cstr_bstr(basic_string_char *ret,
        const char *left, const basic_string_char *right)
{
    TRACE("%s %p\n", debugstr_a(left), right);

    MSVCP_basic_string_char_ctor_cstr(ret, left);
    MSVCP_basic_string_char_append(ret, right);
    return ret;
}

/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@0@Z */
/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@0@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@0@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@0@Z */
basic_string_char* __cdecl MSVCP_basic_string_char_concatenate(basic_string_char *ret,
        const basic_string_char *left, const basic_string_char *right)
{
    TRACE("%p %p\n", left, right);

    MSVCP_basic_string_char_copy_ctor(ret, left);
    MSVCP_basic_string_char_append(ret, right);
    return ret;
}

/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@D@Z */
/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@D@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@ABV10@D@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@AEBV10@D@Z */
basic_string_char* __cdecl MSVCP_basic_string_char_concatenate_bstr_ch(basic_string_char *ret,
        const basic_string_char *left, char right)
{
    TRACE("%p %c\n", left, right);

    MSVCP_basic_string_char_copy_ctor(ret, left);
    MSVCP_basic_string_char_append_ch(ret, right);
    return ret;
}

/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@DABV10@@Z */
/* ??$?HDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@DAEBV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@DABV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@DAEBV10@@Z */
basic_string_char* __cdecl MSVCP_basic_string_char_concatenate_ch_bstr(basic_string_char *ret,
        char left, const basic_string_char *right)
{
    TRACE("%c %p\n", left, right);

    MSVCP_basic_string_char_ctor_cstr_len(ret, &left, 1);
    MSVCP_basic_string_char_append(ret, right);
    return ret;
}

/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHIIPBDI@Z */
/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAH_K0PEBD0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_compare_substr_cstr_len, 20)
int __thiscall MSVCP_basic_string_char_compare_substr_cstr_len(
            const basic_string_char *this, size_t pos, size_t num,
            const char *str, size_t count)
{
    int ans;

    TRACE("%p %Iu %Iu %s %Iu\n", this, pos, num, debugstr_an(str, count), count);

    if(this->size < pos)
        MSVCP__String_base_Xran();

    if(num > this->size-pos)
        num = this->size-pos;

    ans = MSVCP_char_traits_char_compare(basic_string_char_const_ptr(this)+pos,
            str, num>count ? count : num);
    if(ans)
        return ans;

    if(num > count)
        ans = 1;
    else if(num < count)
        ans = -1;
    return ans;
}

/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHIIPBD@Z */
/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAH_K0PEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_compare_substr_cstr, 16)
int __thiscall MSVCP_basic_string_char_compare_substr_cstr(const basic_string_char *this,
        size_t pos, size_t num, const char *str)
{
    return MSVCP_basic_string_char_compare_substr_cstr_len(this, pos, num,
            str, MSVCP_char_traits_char_length(str));
}

/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHPBD@Z */
/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAHPEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_compare_cstr, 8)
int __thiscall MSVCP_basic_string_char_compare_cstr(
        const basic_string_char *this, const char *str)
{
    return MSVCP_basic_string_char_compare_substr_cstr_len(this, 0, this->size,
            str, MSVCP_char_traits_char_length(str));
}

/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHIIABV12@II@Z */
/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAH_K0AEBV12@00@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_compare_substr_substr, 24)
int __thiscall MSVCP_basic_string_char_compare_substr_substr(
        const basic_string_char *this, size_t pos, size_t num,
        const basic_string_char *compare, size_t off, size_t count)
{
    TRACE("%p %Iu %Iu %p %Iu %Iu\n", this, pos, num, compare, off, count);

    if(compare->size < off)
        MSVCP__String_base_Xran();

    if(count > compare->size-off)
        count = compare->size-off;

    return MSVCP_basic_string_char_compare_substr_cstr_len(this, pos, num,
            basic_string_char_const_ptr(compare)+off, count);
}

/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHIIABV12@@Z */
/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAH_K0AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_compare_substr, 16)
int __thiscall MSVCP_basic_string_char_compare_substr(
        const basic_string_char *this, size_t pos, size_t num,
        const basic_string_char *compare)
{
    return MSVCP_basic_string_char_compare_substr_cstr_len(this, pos, num,
            basic_string_char_const_ptr(compare), compare->size);
}

/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEHABV12@@Z */
/* ?compare@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAHAEBV12@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_compare, 8)
int __thiscall MSVCP_basic_string_char_compare(
        const basic_string_char *this, const basic_string_char *compare)
{
    return MSVCP_basic_string_char_compare_substr_cstr_len(this, 0, this->size,
            basic_string_char_const_ptr(compare), compare->size);
}

/* ??$?8DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??$?8DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??8std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??8std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_char_equal(
        const basic_string_char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare(left, right) == 0;
}

/* ??$?8DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??$?8DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
/* ??8std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??8std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
bool __cdecl MSVCP_basic_string_char_equal_str_cstr(
        const basic_string_char *left, const char *right)
{
    return MSVCP_basic_string_char_compare_cstr(left, right) == 0;
}

/* ??$?8DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?8DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??8std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
bool __cdecl MSVCP_basic_string_char_equal_cstr_str(
        const char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare_cstr(right, left) == 0;
}

/* ??$?9DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??$?9DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??9std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??9std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_char_not_equal(
        const basic_string_char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare(left, right) != 0;
}

/* ??$?9DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??$?9DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
/* ??9std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??9std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
bool __cdecl MSVCP_basic_string_char_not_equal_str_cstr(
        const basic_string_char *left, const char *right)
{
    return MSVCP_basic_string_char_compare_cstr(left, right) != 0;
}

/* ??$?9DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?9DU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??9std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??9std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
bool __cdecl MSVCP_basic_string_char_not_equal_cstr_str(
        const char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare_cstr(right, left) != 0;
}

/* ??$?MDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??$?MDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_char_lower(
        const basic_string_char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare(left, right) < 0;
}

/* ??$?MDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??$?MDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
bool __cdecl MSVCP_basic_string_char_lower_bstr_cstr(
        const basic_string_char *left, const char *right)
{
    return MSVCP_basic_string_char_compare_cstr(left, right) < 0;
}

/* ??$?MDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?MDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
bool __cdecl MSVCP_basic_string_char_lower_cstr_bstr(
        const char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare_cstr(right, left) > 0;
}

/* ??$?NDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??$?NDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_char_leq(
        const basic_string_char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare(left, right) <= 0;
}

/* ??$?NDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??$?NDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
bool __cdecl MSVCP_basic_string_char_leq_bstr_cstr(
        const basic_string_char *left, const char *right)
{
    return MSVCP_basic_string_char_compare_cstr(left, right) <= 0;
}

/* ??$?NDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?NDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
bool __cdecl MSVCP_basic_string_char_leq_cstr_bstr(
        const char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare_cstr(right, left) >= 0;
}

/* ??$?ODU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??$?ODU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_char_greater(
        const basic_string_char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare(left, right) > 0;
}

/* ??$?ODU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??$?ODU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
bool __cdecl MSVCP_basic_string_char_greater_bstr_cstr(
        const basic_string_char *left, const char *right)
{
    return MSVCP_basic_string_char_compare_cstr(left, right) > 0;
}

/* ??$?ODU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?ODU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
bool __cdecl MSVCP_basic_string_char_greater_cstr_bstr(
        const char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare_cstr(right, left) < 0;
}

/* ??$?PDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
/* ??$?PDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_char_geq(
        const basic_string_char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare(left, right) >= 0;
}

/* ??$?PDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PBD@Z */
/* ??$?PDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@PEBD@Z */
bool __cdecl MSVCP_basic_string_char_geq_bstr_cstr(
        const basic_string_char *left, const char *right)
{
    return MSVCP_basic_string_char_compare_cstr(left, right) >= 0;
}

/* ??$?PDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
/* ??$?PDU?$char_traits@D@std@@V?$allocator@D@1@@std@@YA_NPEBDAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z */
bool __cdecl MSVCP_basic_string_char_geq_cstr_bstr(
        const char *left, const basic_string_char *right)
{
    return MSVCP_basic_string_char_compare_cstr(right, left) <= 0;
}

/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z */
/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_char_find_cstr_substr(
        const basic_string_char *this, const char *find, size_t pos, size_t len)
{
    const char *p, *end;

    TRACE("%p %s %Iu %Iu\n", this, debugstr_an(find, len), pos, len);

    if(len==0 && pos<=this->size)
        return pos;
    if(pos>=this->size || len>this->size)
        return MSVCP_basic_string_char_npos;

    end = basic_string_char_const_ptr(this)+this->size-len+1;
    for(p=basic_string_char_const_ptr(this)+pos; p<end; p++) {
        p = MSVCP_char_traits_char_find(p, end-p, find);
        if(!p)
            break;

        if(!MSVCP_char_traits_char_compare(p, find, len))
            return p-basic_string_char_const_ptr(this);
    }

    return MSVCP_basic_string_char_npos;
}

/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDI@Z */
/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_cstr_off, 12)
size_t __thiscall MSVCP_basic_string_char_find_cstr_off(
        const basic_string_char *this, const char *find, size_t pos)
{
    return MSVCP_basic_string_char_find_cstr_substr(this, find, pos,
            MSVCP_char_traits_char_length(find));
}

/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIABV12@I@Z */
/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_off, 12)
size_t __thiscall MSVCP_basic_string_char_find_off(
        const basic_string_char *this, const basic_string_char *find, size_t off)
{
    return MSVCP_basic_string_char_find_cstr_substr(this,
            basic_string_char_const_ptr(find), off, find->size);
}

/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIDI@Z */
/* ?find@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_ch, 12)
size_t __thiscall MSVCP_basic_string_char_find_ch(
        const basic_string_char *this, char ch, size_t pos)
{
    return MSVCP_basic_string_char_find_cstr_substr(this, &ch, pos, 1);
}

/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z */
/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_rfind_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_char_rfind_cstr_substr(
        const basic_string_char *this, const char *find, size_t pos, size_t len)
{
    const char *p, *end;

    TRACE("%p %s %Iu %Iu\n", this, debugstr_an(find, len), pos, len);

    if(len==0)
        return pos<this->size ? pos : this->size;

    if(len > this->size)
        return MSVCP_basic_string_char_npos;

    if(pos > this->size-len)
        pos = this->size-len;
    end = basic_string_char_const_ptr(this);
    for(p=end+pos; p>=end; p--) {
        if(*p==*find && !MSVCP_char_traits_char_compare(p, find, len))
            return p-basic_string_char_const_ptr(this);
    }

    return MSVCP_basic_string_char_npos;
}

/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDI@Z */
/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_rfind_cstr_off, 12)
size_t __thiscall MSVCP_basic_string_char_rfind_cstr_off(
        const basic_string_char *this, const char *find, size_t pos)
{
    return MSVCP_basic_string_char_rfind_cstr_substr(this, find, pos,
            MSVCP_char_traits_char_length(find));
}

/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIABV12@I@Z */
/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_rfind_off, 12)
size_t __thiscall MSVCP_basic_string_char_rfind_off(
        const basic_string_char *this, const basic_string_char *find, size_t off)
{
    return MSVCP_basic_string_char_rfind_cstr_substr(this,
            basic_string_char_const_ptr(find), off, find->size);
}

/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIDI@Z */
/* ?rfind@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_rfind_ch, 12)
size_t __thiscall MSVCP_basic_string_char_rfind_ch(
        const basic_string_char *this, char ch, size_t pos)
{
    return MSVCP_basic_string_char_rfind_cstr_substr(this, &ch, pos, 1);
}

/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z */
/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_char_find_first_of_cstr_substr(
        const basic_string_char *this, const char *find, size_t off, size_t len)
{
    const char *p, *end;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(len>0 && off<this->size) {
        end = basic_string_char_const_ptr(this)+this->size;
        for(p=basic_string_char_const_ptr(this)+off; p<end; p++)
            if(MSVCP_char_traits_char_find(find, len, p))
                return p-basic_string_char_const_ptr(this);
    }

    return MSVCP_basic_string_char_npos;
}

/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIABV12@I@Z */
/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_of, 12)
size_t __thiscall MSVCP_basic_string_char_find_first_of(
        const basic_string_char *this, const basic_string_char *find, size_t off)
{
    return MSVCP_basic_string_char_find_first_of_cstr_substr(this,
            basic_string_char_const_ptr(find), off, find->size);
}

/* ??0?$_Yarn@D@std@@QAE@XZ */
/* ??0?$_Yarn@D@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Yarn_char_ctor, 4)
_Yarn_char* __thiscall _Yarn_char_ctor(_Yarn_char *this)
{
    TRACE("(%p)\n", this);

    this->str = NULL;
    this->null_str = '\0';
    return this;
}

/* ?_Tidy@?$_Yarn@D@std@@AAEXXZ */
/* ?_Tidy@?$_Yarn@D@std@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Yarn_char__Tidy, 4)
void __thiscall _Yarn_char__Tidy(_Yarn_char *this)
{
    TRACE("(%p)\n", this);

    if(this->str)
        operator_delete(this->str);
    this->str = NULL;
}

/* ??4?$_Yarn@D@std@@QAEAAV01@PBD@Z */
/* ??4?$_Yarn@D@std@@QEAAAEAV01@PEBD@Z */
DEFINE_THISCALL_WRAPPER(_Yarn_char_op_assign_cstr, 8)
_Yarn_char* __thiscall _Yarn_char_op_assign_cstr(_Yarn_char *this, const char *str)
{
    TRACE("(%p %p)\n", this, str);

    if(str != this->str) {
        _Yarn_char__Tidy(this);

        if(str) {
            size_t len = strlen(str);

            this->str = operator_new((len+1)*sizeof(char));
            memcpy(this->str, str, (len+1)*sizeof(char));
        }
    }
    return this;
}

/* ??0?$_Yarn@D@std@@QAE@PBD@Z */
/* ??0?$_Yarn@D@std@@QEAA@PEBD@Z */
DEFINE_THISCALL_WRAPPER(_Yarn_char_ctor_cstr, 8)
_Yarn_char* __thiscall _Yarn_char_ctor_cstr(_Yarn_char *this, const char *str)
{
    TRACE("(%p %p)\n", this, str);

    _Yarn_char_ctor(this);
    return _Yarn_char_op_assign_cstr(this, str);
}

/* ??4?$_Yarn@D@std@@QAEAAV01@ABV01@@Z */
/* ??4?$_Yarn@D@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(_Yarn_char_op_assign, 8)
_Yarn_char* __thiscall _Yarn_char_op_assign(_Yarn_char *this, const _Yarn_char *rhs)
{
    TRACE("(%p %p)\n", this, rhs);

    return _Yarn_char_op_assign_cstr(this, rhs->str);
}

/* ??0?$_Yarn@D@std@@QAE@ABV01@@Z */
/* ??0?$_Yarn@D@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(_Yarn_char_copy_ctor, 8)
_Yarn_char* __thiscall _Yarn_char_copy_ctor(_Yarn_char *this, const _Yarn_char *copy)
{
    TRACE("(%p %p)\n", this, copy);

    _Yarn_char_ctor(this);
    return _Yarn_char_op_assign(this, copy);
}

/* ??1?$_Yarn@D@std@@QAE@XZ */
/* ??1?$_Yarn@D@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Yarn_char_dtor, 4)
void __thiscall _Yarn_char_dtor(_Yarn_char *this)
{
    TRACE("(%p)\n", this);
    _Yarn_char__Tidy(this);
}

/* ?_C_str@?$_Yarn@D@std@@QBEPBDXZ */
/* ?_C_str@?$_Yarn@D@std@@QEBAPEBDXZ */
/* ?c_str@?$_Yarn@D@std@@QBEPBDXZ */
/* ?c_str@?$_Yarn@D@std@@QEBAPEBDXZ */
DEFINE_THISCALL_WRAPPER(_Yarn_char_c_str, 4)
const char* __thiscall _Yarn_char_c_str(const _Yarn_char *this)
{
    TRACE("(%p)\n", this);
    return this->str ? this->str : &this->null_str;
}

/* ?_Empty@?$_Yarn@D@std@@QBE_NXZ */
/* ?_Empty@?$_Yarn@D@std@@QEBA_NXZ */
/* ?empty@?$_Yarn@D@std@@QBE_NXZ */
/* ?empty@?$_Yarn@D@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(_Yarn_char_empty, 4)
bool __thiscall _Yarn_char_empty(const _Yarn_char *this)
{
    TRACE("(%p)\n", this);
    return !this->str;
}

/* ??0?$_Yarn@_W@std@@QAE@XZ */
/* ??0?$_Yarn@_W@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Yarn_wchar_ctor, 4)
_Yarn_wchar* __thiscall _Yarn_wchar_ctor(_Yarn_wchar *this)
{
    TRACE("(%p)\n", this);

    this->str = NULL;
    this->null_str = '\0';
    return this;
}

/* ?_Tidy@?$_Yarn@_W@std@@AAEXXZ */
/* ?_Tidy@?$_Yarn@_W@std@@AEAAXXZ */
DEFINE_THISCALL_WRAPPER(_Yarn_wchar__Tidy, 4)
void __thiscall _Yarn_wchar__Tidy(_Yarn_wchar *this)
{
    TRACE("(%p)\n", this);

    if(this->str)
        operator_delete(this->str);
    this->str = NULL;
}

/* ??1?$_Yarn@_W@std@@QAE@XZ */
/* ??1?$_Yarn@_W@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(_Yarn_wchar_dtor, 4)
void __thiscall _Yarn_wchar_dtor(_Yarn_wchar *this)
{
    TRACE("(%p)\n", this);
    _Yarn_wchar__Tidy(this);
}

/* ??4?$_Yarn@_W@std@@QAEAAV01@PB_W@Z */
/* ??4?$_Yarn@_W@std@@QEAAAEAV01@PEB_W@Z */
DEFINE_THISCALL_WRAPPER(_Yarn_wchar_op_assign_cstr, 8)
_Yarn_wchar* __thiscall _Yarn_wchar_op_assign_cstr(_Yarn_wchar *this, const wchar_t *str)
{
    TRACE("(%p %p)\n", this, str);

    if(str != this->str) {
        _Yarn_wchar__Tidy(this);

        if(str) {
            size_t len = wcslen(str);

            this->str = operator_new((len+1)*sizeof(wchar_t));
            memcpy(this->str, str, (len+1)*sizeof(wchar_t));
        }
    }
    return this;
}

/* ?_C_str@?$_Yarn@_W@std@@QBEPB_WXZ */
/* ?_C_str@?$_Yarn@_W@std@@QEBAPEB_WXZ */
DEFINE_THISCALL_WRAPPER(_Yarn_wchar__C_str, 4)
const wchar_t* __thiscall _Yarn_wchar__C_str(const _Yarn_wchar *this)
{
    TRACE("(%p)\n", this);
    return this->str ? this->str : &this->null_str;
}

/* ?_Empty@?$_Yarn@_W@std@@QBE_NXZ */
/* ?_Empty@?$_Yarn@_W@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(_Yarn_wchar__Empty, 4)
bool __thiscall _Yarn_wchar__Empty(const _Yarn_wchar *this)
{
    TRACE("(%p)\n", this);
    return !this->str;
}

/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDI@Z */
/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_char_find_first_of_cstr(
        const basic_string_char *this, const char *find, size_t off)
{
    return MSVCP_basic_string_char_find_first_of_cstr_substr(
            this, find, off, MSVCP_char_traits_char_length(find));
}

/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIDI@Z */
/* ?find_first_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_of_ch, 12)
size_t __thiscall MSVCP_basic_string_char_find_first_of_ch(
        const basic_string_char *this, char ch, size_t off)
{
    return MSVCP_basic_string_char_find_first_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z */
/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_not_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_char_find_first_not_of_cstr_substr(
        const basic_string_char *this, const char *find, size_t off, size_t len)
{
    const char *p, *end;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(off<this->size) {
        end = basic_string_char_const_ptr(this)+this->size;
        for(p=basic_string_char_const_ptr(this)+off; p<end; p++)
            if(!MSVCP_char_traits_char_find(find, len, p))
                return p-basic_string_char_const_ptr(this);
    }

    return MSVCP_basic_string_char_npos;
}

/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIABV12@I@Z */
/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_not_of, 12)
size_t __thiscall MSVCP_basic_string_char_find_first_not_of(
        const basic_string_char *this, const basic_string_char *find, size_t off)
{
    return MSVCP_basic_string_char_find_first_not_of_cstr_substr(this,
            basic_string_char_const_ptr(find), off, find->size);
}

/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDI@Z */
/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_not_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_char_find_first_not_of_cstr(
        const basic_string_char *this, const char *find, size_t off)
{
    return MSVCP_basic_string_char_find_first_not_of_cstr_substr(
            this, find, off, MSVCP_char_traits_char_length(find));
}

/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIDI@Z */
/* ?find_first_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_first_not_of_ch, 12)
size_t __thiscall MSVCP_basic_string_char_find_first_not_of_ch(
        const basic_string_char *this, char ch, size_t off)
{
    return MSVCP_basic_string_char_find_first_not_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z */
/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_char_find_last_of_cstr_substr(
        const basic_string_char *this, const char *find, size_t off, size_t len)
{
    const char *p, *beg;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(len>0 && this->size>0) {
        if(off >= this->size)
            off = this->size-1;

        beg = basic_string_char_const_ptr(this);
        for(p=beg+off; p>=beg; p--)
            if(MSVCP_char_traits_char_find(find, len, p))
                return p-beg;
    }

    return MSVCP_basic_string_char_npos;
}

/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIABV12@I@Z */
/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_of, 12)
size_t __thiscall MSVCP_basic_string_char_find_last_of(
        const basic_string_char *this, const basic_string_char *find, size_t off)
{
    return MSVCP_basic_string_char_find_last_of_cstr_substr(this,
            basic_string_char_const_ptr(find), off, find->size);
}

/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDI@Z */
/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_char_find_last_of_cstr(
        const basic_string_char *this, const char *find, size_t off)
{
    return MSVCP_basic_string_char_find_last_of_cstr_substr(
            this, find, off, MSVCP_char_traits_char_length(find));
}

/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIDI@Z */
/* ?find_last_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_of_ch, 12)
size_t __thiscall MSVCP_basic_string_char_find_last_of_ch(
        const basic_string_char *this, char ch, size_t off)
{
    return MSVCP_basic_string_char_find_last_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDII@Z */
/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_not_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_char_find_last_not_of_cstr_substr(
        const basic_string_char *this, const char *find, size_t off, size_t len)
{
    const char *p, *beg;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(this->size>0) {
        if(off >= this->size)
            off = this->size-1;

        beg = basic_string_char_const_ptr(this);
        for(p=beg+off; p>=beg; p--)
            if(!MSVCP_char_traits_char_find(find, len, p))
                return p-beg;
    }

    return MSVCP_basic_string_char_npos;
}

/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIABV12@I@Z */
/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_not_of, 12)
size_t __thiscall MSVCP_basic_string_char_find_last_not_of(
        const basic_string_char *this, const basic_string_char *find, size_t off)
{
    return MSVCP_basic_string_char_find_last_not_of_cstr_substr(this,
            basic_string_char_const_ptr(find), off, find->size);
}

/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIPBDI@Z */
/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KPEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_not_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_char_find_last_not_of_cstr(
        const basic_string_char *this, const char *find, size_t off)
{
    return MSVCP_basic_string_char_find_last_not_of_cstr_substr(
            this, find, off, MSVCP_char_traits_char_length(find));
}

/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEIDI@Z */
/* ?find_last_not_of@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA_KD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_find_last_not_of_ch, 12)
size_t __thiscall MSVCP_basic_string_char_find_last_not_of_ch(
        const basic_string_char *this, char ch, size_t off)
{
    return MSVCP_basic_string_char_find_last_not_of_cstr_substr(this, &ch, off, 1);
}

/* ??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAADI@Z */
/* ??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_operator_at, 8)
char* __thiscall MSVCP_basic_string_char_operator_at(
        basic_string_char *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

#if _MSVCP_VER >= 80
    if (this->size < pos)
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
#endif

    return basic_string_char_ptr(this)+pos;
}

/* ??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEABDI@Z */
/* ??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAAEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_const_operator_at, 8)
const char* __thiscall MSVCP_basic_string_char_const_operator_at(
        const basic_string_char *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

#if _MSVCP_VER >= 80
    if (this->size < pos)
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
#endif

    return basic_string_char_const_ptr(this)+pos;
}

/* ??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAADU_Size_type_nosscl@01@@Z */
/* ??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEADU_Size_type_nosscl@01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_operator_at_noverify, 8)
char* __thiscall MSVCP_basic_string_char_operator_at_noverify(
        basic_string_char *this, size_t_noverify pos)
{
    TRACE("%p %Iu\n", this, pos.val);
    return basic_string_char_ptr(this)+pos.val;
}

/* ??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEABDU_Size_type_nosscl@01@@Z */
/* ??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAAEBDU_Size_type_nosscl@01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_operator_const_at_noverify, 8)
const char* __thiscall MSVCP_basic_string_char_operator_const_at_noverify(
        const basic_string_char *this, size_t_noverify pos)
{
    TRACE("%p %Iu\n", this, pos.val);
    return basic_string_char_const_ptr(this)+pos.val;
}

/* ?at@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAADI@Z */
/* ?at@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_at, 8)
char* __thiscall MSVCP_basic_string_char_at(
        basic_string_char *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

    if(this->size <= pos)
        MSVCP__String_base_Xran();

    return basic_string_char_ptr(this)+pos;
}

/* ?at@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEABDI@Z */
/* ?at@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBAAEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_const_at, 8)
const char* __thiscall MSVCP_basic_string_char_const_at(
        const basic_string_char *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

    if(this->size <= pos)
        MSVCP__String_base_Xran();

    return basic_string_char_const_ptr(this)+pos;
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IIPBDI@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0PEBD0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_cstr_len, 20)
basic_string_char* __thiscall basic_string_char_replace_cstr_len(basic_string_char *this,
        size_t off, size_t len, const char *str, size_t str_len)
{
    size_t inside_pos = -1;
    char *ptr = basic_string_char_ptr(this);

    TRACE("%p %Iu %Iu %p %Iu\n", this, off, len, str, str_len);

    if(this->size < off)
        MSVCP__String_base_Xran();

    if(len > this->size-off)
        len = this->size-off;

    if(MSVCP_basic_string_char_npos-str_len <= this->size-len)
        MSVCP__String_base_Xlen();

    if(basic_string_char_inside(this, str))
        inside_pos = str-ptr;

    if(len < str_len) {
        basic_string_char_grow(this, this->size-len+str_len, FALSE);
        ptr = basic_string_char_ptr(this);
    }

    if(inside_pos == -1) {
        memmove(ptr+off+str_len, ptr+off+len, (this->size-off-len)*sizeof(char));
        memcpy(ptr+off, str, str_len*sizeof(char));
    } else if(len >= str_len) {
        memmove(ptr+off, ptr+inside_pos, str_len*sizeof(char));
        memmove(ptr+off+str_len, ptr+off+len, (this->size-off-len)*sizeof(char));
    } else {
        size_t size;

        memmove(ptr+off+str_len, ptr+off+len, (this->size-off-len)*sizeof(char));

        if(inside_pos < off+len) {
            size = off+len-inside_pos;
            if(size > str_len)
                size = str_len;
            memmove(ptr+off, ptr+inside_pos, size*sizeof(char));
        } else {
            size = 0;
        }

        if(str_len > size)
            memmove(ptr+off+size, ptr+off+str_len, (str_len-size)*sizeof(char));
    }

    basic_string_char_eos(this, this->size-len+str_len);
    return this;
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IIPBD@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0PEBD@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_cstr, 16)
basic_string_char* __thiscall basic_string_char_replace_cstr(basic_string_char *this,
        size_t off, size_t len, const char *str)
{
    return basic_string_char_replace_cstr_len(this, off, len, str,
            MSVCP_char_traits_char_length(str));
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IIABV12@II@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0AEBV12@00@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_substr, 24)
basic_string_char* __thiscall basic_string_char_replace_substr(basic_string_char *this, size_t off,
        size_t len, const basic_string_char *str, size_t str_off, size_t str_len)
{
    if(str->size < str_off)
        MSVCP__String_base_Xran();

    if(str_len > str->size-str_off)
        str_len = str->size-str_off;

    return basic_string_char_replace_cstr_len(this, off, len,
            basic_string_char_const_ptr(str)+str_off, str_len);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IIABV12@@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace, 16)
basic_string_char* __thiscall basic_string_char_replace(basic_string_char *this,
        size_t off, size_t len, const basic_string_char *str)
{
    return basic_string_char_replace_cstr_len(this, off, len,
            basic_string_char_const_ptr(str), str->size);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IIID@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K00D@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_ch, 20)
basic_string_char* __thiscall basic_string_char_replace_ch(basic_string_char *this,
        size_t off, size_t len, size_t count, char ch)
{
    char *ptr = basic_string_char_ptr(this);

    TRACE("%p %Iu %Iu %Iu %c\n", this, off, len, count, ch);

    if(this->size < off)
        MSVCP__String_base_Xran();

    if(len > this->size-off)
        len = this->size-off;

    if(MSVCP_basic_string_char_npos-count <= this->size-len)
        MSVCP__String_base_Xlen();

    if(len < count) {
        basic_string_char_grow(this, this->size-len+count, FALSE);
        ptr = basic_string_char_ptr(this);
    }

    memmove(ptr+off+count, ptr+off+len, (this->size-off-len)*sizeof(char));
    MSVCP_char_traits_char_assignn(ptr+off, count, ch);
    basic_string_char_eos(this, this->size-len+count);

    return this;
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IABV12@@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_KAEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert, 12)
basic_string_char* __thiscall basic_string_char_insert(basic_string_char *this,
        size_t off, const basic_string_char *str)
{
    return basic_string_char_replace(this, off, 0, str);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IABV12@II@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_KAEBV12@00@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_substr, 20)
basic_string_char* __thiscall basic_string_char_insert_substr(
        basic_string_char *this, size_t off, const basic_string_char *str,
        size_t str_off, size_t str_count)
{
    return basic_string_char_replace_substr(this, off, 0, str, str_off, str_count);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IPBD@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_KPEBD@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_cstr, 12)
basic_string_char* __thiscall basic_string_char_insert_cstr(
        basic_string_char *this, size_t off, const char *str)
{
    return basic_string_char_replace_cstr(this, off, 0, str);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IPBDI@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_KPEBD0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_cstr_len, 16)
basic_string_char* __thiscall basic_string_char_insert_cstr_len(basic_string_char *this,
        size_t off, const char *str, size_t str_len)
{
    return basic_string_char_replace_cstr_len(this, off, 0, str, str_len);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@IID@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@_K0D@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_chn, 16)
basic_string_char* __thiscall basic_string_char_insert_chn(basic_string_char *this,
        size_t off, size_t count, char ch)
{
    return basic_string_char_replace_ch(this, off, 0, count, ch);
}

/* ?resize@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXID@Z */
/* ?resize@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAX_KD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_resize_ch, 12)
void __thiscall MSVCP_basic_string_char_resize_ch(
        basic_string_char *this, size_t size, char ch)
{
    TRACE("%p %Iu %c\n", this, size, ch);

    if(size <= this->size)
        MSVCP_basic_string_char_erase(this, size, this->size);
    else
        MSVCP_basic_string_char_append_len_ch(this, size-this->size, ch);
}

/* ?resize@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXI@Z */
/* ?resize@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_resize, 8)
void __thiscall MSVCP_basic_string_char_resize(
        basic_string_char *this, size_t size)
{
    MSVCP_basic_string_char_resize_ch(this, size, '\0');
}

/* ?clear@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXXZ */
/* ?clear@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_clear, 4)
void __thiscall MSVCP_basic_string_char_clear(basic_string_char *this)
{
    basic_string_char_eos(this, 0);
}

/* basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t>> */
/* basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short>> */
/* ?npos@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@2IB */
/* ?npos@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@2_KB */
/* ?npos@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@2IB */
/* ?npos@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@2_KB */
const size_t MSVCP_basic_string_wchar_npos = -1;

/* ?_Myptr@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAEPA_WXZ */
/* ?_Myptr@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAAPEA_WXZ */
/* ?_Myptr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEPAGXZ */
/* ?_Myptr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAPEAGXZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_ptr, 4)
wchar_t* __thiscall basic_string_wchar_ptr(basic_string_wchar *this)
{
    if(this->res < BUF_SIZE_WCHAR)
        return this->data.buf;
    return this->data.ptr;
}

/* ?_Myptr@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IBEPB_WXZ */
/* ?_Myptr@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEBAPEB_WXZ */
/* ?_Myptr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IBEPBGXZ */
/* ?_Myptr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEBAPEBGXZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_const_ptr, 4)
const wchar_t* __thiscall basic_string_wchar_const_ptr(const basic_string_wchar *this)
{
    if(this->res < BUF_SIZE_WCHAR)
        return this->data.buf;
    return this->data.ptr;
}

/* ?_Eos@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAEXI@Z */
/* ?_Eos@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAAX_K@Z */
/* ?_Eos@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEXI@Z */
/* ?_Eos@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_eos, 8)
void __thiscall basic_string_wchar_eos(basic_string_wchar *this, size_t len)
{
    this->size = len;
    MSVCP_char_traits_wchar_assign(basic_string_wchar_ptr(this)+len, L"");
}

/* ?_Inside@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAE_NPB_W@Z */
/* ?_Inside@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAA_NPEB_W@Z */
/* ?_Inside@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAE_NPBG@Z */
/* ?_Inside@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAA_NPEBG@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_inside, 8)
bool __thiscall basic_string_wchar_inside(
        basic_string_wchar *this, const wchar_t *ptr)
{
    wchar_t *cstr = basic_string_wchar_ptr(this);

    return ptr>=cstr && ptr<cstr+this->size;
}

/* ?_Tidy@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAEX_NI@Z */
/* ?_Tidy@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAAX_N_K@Z */
/* ?_Tidy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEX_NI@Z */
/* ?_Tidy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAX_N_K@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_tidy, 12)
void __thiscall basic_string_wchar_tidy(basic_string_wchar *this,
        bool built, size_t new_size)
{
    if(built && BUF_SIZE_WCHAR<=this->res) {
        wchar_t *ptr = this->data.ptr;

        if(new_size > 0)
            MSVCP_char_traits_wchar__Copy_s(this->data.buf, BUF_SIZE_WCHAR, ptr, new_size);
        MSVCP_allocator_wchar_deallocate(STRING_ALLOCATOR(this), ptr, this->res+1);
    }

    this->res = BUF_SIZE_WCHAR-1;
    basic_string_wchar_eos(this, new_size);
}

/* ?_Grow@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAE_NI_N@Z */
/* ?_Grow@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAA_N_K_N@Z */
/* ?_Grow@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAE_NI_N@Z */
/* ?_Grow@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAA_N_K_N@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_grow, 12)
bool __thiscall basic_string_wchar_grow(
        basic_string_wchar *this, size_t new_size, bool trim)
{
    if(this->res < new_size) {
        size_t new_res = new_size, len = this->size;
        wchar_t *ptr;

        new_res |= 0xf;

        if(new_res/3 < this->res/2)
            new_res = this->res + this->res/2;

        ptr = MSVCP_allocator_wchar_allocate(STRING_ALLOCATOR(this), new_res+1);
        if(!ptr)
            ptr = MSVCP_allocator_wchar_allocate(STRING_ALLOCATOR(this), new_size+1);
        else
            new_size = new_res;
        if(!ptr) {
            ERR("Out of memory\n");
            basic_string_wchar_tidy(this, TRUE, 0);
            return FALSE;
        }

        MSVCP_char_traits_wchar__Copy_s(ptr, new_size,
                basic_string_wchar_ptr(this), this->size);
        basic_string_wchar_tidy(this, TRUE, 0);
        this->data.ptr = ptr;
        this->res = new_size;
        basic_string_wchar_eos(this, len);
    } else if(trim && new_size < BUF_SIZE_WCHAR)
        basic_string_wchar_tidy(this, TRUE,
                new_size<this->size ? new_size : this->size);
    else if(new_size == 0)
        basic_string_wchar_eos(this, 0);

    return (new_size>0);
}

/* ?_Copy@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAEXII@Z */
/* ?_Copy@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAAX_K0@Z */
/* ?_Copy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEXII@Z */
/* ?_Copy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAX_K0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar__Copy, 12)
void __thiscall basic_string_wchar__Copy(basic_string_wchar *this,
        size_t new_size, size_t copy_len)
{
    TRACE("%p %Iu %Iu\n", this, new_size, copy_len);

    if(!basic_string_wchar_grow(this, new_size, FALSE))
        return;
    basic_string_wchar_eos(this, copy_len);
}

/* ?get_allocator@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$allocator@_W@2@XZ */
/* ?get_allocator@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$allocator@_W@2@XZ */
/* ?get_allocator@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$allocator@G@2@XZ */
/* ?get_allocator@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$allocator@G@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_get_allocator, 8)
allocator* __thiscall basic_string_wchar_get_allocator(const basic_string_wchar *this, allocator *ret)
{
    TRACE("%p\n", this);
    return ret;
}

/* ?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@II@Z */
/* ?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_K0@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@II@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_erase, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_erase(
            basic_string_wchar *this, size_t pos, size_t len)
{
    TRACE("%p %Iu %Iu\n", this, pos, len);

    if(pos > this->size)
        MSVCP__String_base_Xran();

    if(len > this->size-pos)
        len = this->size-pos;

    if(len) {
        MSVCP_char_traits_wchar__Move_s(basic_string_wchar_ptr(this)+pos,
                this->res-pos, basic_string_wchar_ptr(this)+pos+len,
                this->size-pos-len);
        basic_string_wchar_eos(this, this->size-len);
    }

    return this;
}

/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@ABV12@II@Z */
/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@AEBV12@_K1@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@ABV12@II@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@AEBV12@_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assign_substr, 16)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assign_substr(
            basic_string_wchar *this, const basic_string_wchar *assign,
            size_t pos, size_t len)
{
    TRACE("%p %p %Iu %Iu\n", this, assign, pos, len);

    if(assign->size < pos)
        MSVCP__String_base_Xran();

    if(len > assign->size-pos)
        len = assign->size-pos;

    if(this == assign) {
        MSVCP_basic_string_wchar_erase(this, pos+len, MSVCP_basic_string_wchar_npos);
        MSVCP_basic_string_wchar_erase(this, 0, pos);
    } else if(basic_string_wchar_grow(this, len, FALSE)) {
        MSVCP_char_traits_wchar__Copy_s(basic_string_wchar_ptr(this),
                this->res, basic_string_wchar_const_ptr(assign)+pos, len);
        basic_string_wchar_eos(this, len);
    }

    return this;
}

/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@ABV12@@Z */
/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@AEBV12@@Z */
/* ??4?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV01@ABV01@@Z */
/* ??4?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV01@AEBV01@@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@ABV12@@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@AEBV12@@Z */
/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV01@ABV01@@Z */
/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assign, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assign(
            basic_string_wchar *this, const basic_string_wchar *assign)
{
    return MSVCP_basic_string_wchar_assign_substr(this, assign,
            0, MSVCP_basic_string_wchar_npos);
}

/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@PB_WI@Z */
/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@PEB_W_K@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBGI@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assign_cstr_len, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assign_cstr_len(
            basic_string_wchar *this, const wchar_t *str, size_t len)
{
    TRACE("%p %s %Iu\n", this, debugstr_wn(str, len), len);

    if(basic_string_wchar_inside(this, str))
        return MSVCP_basic_string_wchar_assign_substr(this, this,
                str-basic_string_wchar_ptr(this), len);
    else if(basic_string_wchar_grow(this, len, FALSE)) {
        MSVCP_char_traits_wchar__Copy_s(basic_string_wchar_ptr(this),
                this->res, str, len);
        basic_string_wchar_eos(this, len);
    }

    return this;
}

/* ??4?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV01@_W@Z */
/* ??4?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV01@_W@Z */
/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV01@G@Z */
/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV01@G@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assign_ch, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assign_ch(
        basic_string_wchar *this, wchar_t ch)
{
    return MSVCP_basic_string_wchar_assign_cstr_len(this, &ch, 1);
}

/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@PB_W@Z */
/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@PEB_W@Z */
/* ??4?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV01@PB_W@Z */
/* ??4?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV01@PEB_W@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBG@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG@Z */
/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV01@PBG@Z */
/* ??4?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV01@PEBG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assign_cstr, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assign_cstr(
            basic_string_wchar *this, const wchar_t *str)
{
    return MSVCP_basic_string_wchar_assign_cstr_len(this, str,
            MSVCP_char_traits_wchar_length(str));
}

/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@I_W@Z */
/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_K_W@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IG@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_KG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assignn, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assignn(
        basic_string_wchar *this, size_t count, wchar_t ch)
{
    TRACE("%p %Iu %c\n", this, count, ch);

    basic_string_wchar_grow(this, count, FALSE);
    MSVCP_char_traits_wchar_assignn(basic_string_wchar_ptr(this), count, ch);
    basic_string_wchar_eos(this, count);
    return this;
}

/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@PB_W0@Z */
/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@PEB_W0@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBG0@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assign_ptr_ptr, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assign_ptr_ptr(
        basic_string_wchar *this, const wchar_t *first, const wchar_t *last)
{
    return MSVCP_basic_string_wchar_assign_cstr_len(this, first, last-first);
}

/* ?_Chassign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IAEXII_W@Z */
/* ?_Chassign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@IEAAX_K0_W@Z */
/* ?_Chassign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IAEXIIG@Z */
/* ?_Chassign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@IEAAX_K0G@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_Chassign, 16)
void __thiscall MSVCP_basic_string_wchar_Chassign(basic_string_wchar *this,
        size_t off, size_t count, wchar_t ch)
{
    TRACE("%p %Iu %Iu %c\n", this, off, count, ch);
    MSVCP_char_traits_wchar_assignn(basic_string_wchar_ptr(this)+off, count, ch);
}

/* ?_Copy_s@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPA_WIII@Z */
/* ?_Copy_s@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEA_W_K11@Z */
/* ?_Copy_s@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPAGIII@Z */
/* ?_Copy_s@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEAG_K11@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_Copy_s, 20)
size_t __thiscall MSVCP_basic_string_wchar_Copy_s(const basic_string_wchar *this,
        wchar_t *dest, size_t size, size_t count, size_t off)
{
    TRACE("%p %p %Iu %Iu %Iu\n", this, dest, size, count, off);

    if(this->size < off)
        MSVCP__String_base_Xran();

    if(count > this->size-off)
        count = this->size-off;

    MSVCP_char_traits_wchar__Copy_s(dest, size,
            basic_string_wchar_const_ptr(this)+off, count);
    return count;
}

/* ?copy@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPA_WII@Z */
/* ?copy@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEA_W_K1@Z */
/* ?copy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPAGII@Z */
/* ?copy@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEAG_K1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_copy, 16)
size_t __thiscall basic_string_wchar_copy(const basic_string_wchar *this,
        wchar_t *dest, size_t count, size_t off)
{
    return MSVCP_basic_string_wchar_Copy_s(this, dest, count, count, off);
}

/* ?c_str@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEPB_WXZ */
/* ?c_str@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAPEB_WXZ */
/* ?data@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEPB_WXZ */
/* ?data@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAPEB_WXZ */
/* ?c_str@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPBGXZ */
/* ?c_str@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEBGXZ */
/* ?data@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEPBGXZ */
/* ?data@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAPEBGXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_c_str, 4)
const wchar_t* __thiscall MSVCP_basic_string_wchar_c_str(const basic_string_wchar *this)
{
    TRACE("%p\n", this);
    return basic_string_wchar_const_ptr(this);
}

/* ?capacity@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIXZ */
/* ?capacity@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KXZ */
/* ?capacity@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIXZ */
/* ?capacity@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_capacity, 4)
size_t __thiscall MSVCP_basic_string_wchar_capacity(basic_string_wchar *this)
{
    TRACE("%p\n", this);
    return this->res;
}

/* ?reserve@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXI@Z */
/* ?reserve@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAX_K@Z */
/* ?reserve@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXI@Z */
/* ?reserve@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_reserve, 8)
void __thiscall MSVCP_basic_string_wchar_reserve(basic_string_wchar *this, size_t size)
{
    size_t len;

    TRACE("%p %Iu\n", this, size);

    len = this->size;
    if(len > size)
        return;

    if(basic_string_wchar_grow(this, size, TRUE))
        basic_string_wchar_eos(this, len);
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@XZ */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@XZ */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@XZ */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor, 4)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor(basic_string_wchar *this)
{
    TRACE("%p\n", this);

    basic_string_wchar_tidy(this, FALSE, 0);
    return this;
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@ABV01@@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@AEBV01@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV01@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_copy_ctor, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_copy_ctor(
            basic_string_wchar *this, const basic_string_wchar *copy)
{
    TRACE("%p %p\n", this, copy);

    basic_string_wchar_tidy(this, FALSE, 0);
    MSVCP_basic_string_wchar_assign(this, copy);
    return this;
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@PB_W@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@PEB_W@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@PBG@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@PEBG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_cstr, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_cstr(
            basic_string_wchar *this, const wchar_t *str)
{
    TRACE("%p %s\n", this, debugstr_w(str));

    basic_string_wchar_tidy(this, FALSE, 0);
    MSVCP_basic_string_wchar_assign_cstr(this, str);
    return this;
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@PB_WABV?$allocator@_W@1@@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@PEB_WAEBV?$allocator@_W@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@PBGABV?$allocator@G@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@PEBGAEBV?$allocator@G@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_cstr_alloc, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_cstr_alloc(
        basic_string_wchar *this, const wchar_t *str, const void *alloc)
{
    return MSVCP_basic_string_wchar_ctor_cstr(this, str);
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@PB_WI@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@PEB_W_K@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@PBGI@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@PEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_cstr_len, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_cstr_len(
        basic_string_wchar *this, const wchar_t *str, size_t len)
{
    TRACE("%p %s %Iu\n", this, debugstr_wn(str, len), len);

    basic_string_wchar_tidy(this, FALSE, 0);
    MSVCP_basic_string_wchar_assign_cstr_len(this, str, len);
    return this;
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@PB_WIABV?$allocator@_W@1@@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@PEB_W_KAEBV?$allocator@_W@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@PBGIABV?$allocator@G@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@PEBG_KAEBV?$allocator@G@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_cstr_len_alloc, 16)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_cstr_len_alloc(
        basic_string_wchar *this, const wchar_t *str, size_t len, const void *alloc)
{
    return MSVCP_basic_string_wchar_ctor_cstr_len(this, str, len);
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@ABV01@II@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@AEBV01@_K1@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV01@II@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV01@_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_substr, 16)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_substr(
        basic_string_wchar *this, const basic_string_wchar *assign,
        size_t pos, size_t len)
{
    TRACE("%p %p %Iu %Iu\n", this, assign, pos, len);

    basic_string_wchar_tidy(this, FALSE, 0);
    MSVCP_basic_string_wchar_assign_substr(this, assign, pos, len);
    return this;
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@ABV01@IIABV?$allocator@_W@1@@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@AEBV01@_K1AEBV?$allocator@_W@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV01@IIABV?$allocator@G@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV01@_K1AEBV?$allocator@G@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_substr_alloc, 20)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_substr_alloc(
        basic_string_wchar *this, const basic_string_wchar *assign,
        size_t pos, size_t len, const void *alloc)
{
    return MSVCP_basic_string_wchar_ctor_substr(this, assign, pos, len);
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@ABV?$allocator@_W@1@@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@AEBV?$allocator@_W@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV?$allocator@G@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@AEBV?$allocator@G@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_alloc, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_alloc(
        basic_string_wchar *this, const void *alloc)
{
    TRACE("%p %p\n", this, alloc);

    basic_string_wchar_tidy(this, FALSE, 0);
    return this;
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@I_W@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@_K_W@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@IG@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@_KG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_ch, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_ch(basic_string_wchar *this,
        size_t count, wchar_t ch)
{
    TRACE("%p %Iu %c\n", this, count, ch);

    basic_string_wchar_tidy(this, FALSE, 0);
    MSVCP_basic_string_wchar_assignn(this, count, ch);
    return this;
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@I_WABV?$allocator@_W@1@@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@_K_WAEBV?$allocator@_W@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@IGABV?$allocator@G@1@@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@_KGAEBV?$allocator@G@1@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_ch_alloc, 16)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_ch_alloc(basic_string_wchar *this,
        size_t count, wchar_t ch, const void *alloc)
{
    return MSVCP_basic_string_wchar_ctor_ch(this, count, ch);
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@PB_W0@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@PEB_W0@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@PBG0@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@PEBG0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_ptr_ptr, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_ptr_ptr(basic_string_wchar *this,
        const wchar_t *first, const wchar_t *last)
{
    return MSVCP_basic_string_wchar_ctor_cstr_len(this, first, last-first);
}

/* ??1?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@XZ */
/* ??1?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@XZ */
/* ??1?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@XZ */
/* ??1?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_dtor, 4)
void* __thiscall MSVCP_basic_string_wchar_dtor(basic_string_wchar *this)
{
    TRACE("%p\n", this);
    basic_string_wchar_tidy(this, TRUE, 0);
    return NULL;  /* FEAR 1 installer expects EAX set to 0 */
}

/* ?size@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIXZ */
/* ?size@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KXZ */
/* ?length@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIXZ */
/* ?length@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KXZ */
/* ?size@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIXZ */
/* ?size@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KXZ */
/* ?length@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIXZ */
/* ?length@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_length, 4)
size_t __thiscall MSVCP_basic_string_wchar_length(const basic_string_wchar *this)
{
    TRACE("%p\n", this);
    return this->size;
}

/* ?max_size@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIXZ */
/* ?max_size@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KXZ */
/* ?max_size@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIXZ */
/* ?max_size@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KXZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_max_size, 4)
size_t __thiscall basic_string_wchar_max_size(const basic_string_wchar *this)
{
    TRACE("%p\n", this);
    return MSVCP_allocator_wchar_max_size(STRING_ALLOCATOR(this))-1;
}

/* ?empty@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE_NXZ */
/* ?empty@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_NXZ */
/* ?empty@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE_NXZ */
/* ?empty@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_NXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_empty, 4)
bool __thiscall MSVCP_basic_string_wchar_empty(basic_string_wchar *this)
{
    TRACE("%p\n", this);
    return this->size == 0;
}

/* ?swap@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXAEAV12@@Z */
/* ?swap@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXAAV12@@Z */
/* ?swap@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXAEAV12@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_swap, 8)
void __thiscall MSVCP_basic_string_wchar_swap(basic_string_wchar *this, basic_string_wchar *str)
{
    if(this != str) {
        char tmp[sizeof(this->data)];
        const size_t size = this->size;
        const size_t res = this->res;

        memcpy(tmp, this->data.buf, sizeof(this->data));
        memcpy(this->data.buf, str->data.buf, sizeof(this->data));
        memcpy(str->data.buf, tmp, sizeof(this->data));

        this->size = str->size;
        this->res = str->res;

        str->size = size;
        str->res = res;
    }
}

/* ?substr@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV12@II@Z */
/* ?substr@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV12@_K0@Z */
/* ?substr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV12@II@Z */
/* ?substr@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV12@_K0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_substr, 16)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_substr(basic_string_wchar *this,
        basic_string_wchar *ret, size_t off, size_t len)
{
    TRACE("%p %Iu %Iu\n", this, off, len);

    MSVCP_basic_string_wchar_ctor_substr(ret, this, off, len);
    return ret;
}

/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@ABV12@II@Z */
/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@AEBV12@_K1@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@ABV12@II@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@AEBV12@_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append_substr, 16)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_substr(basic_string_wchar *this,
        const basic_string_wchar *append, size_t offset, size_t count)
{
    TRACE("%p %p %Iu %Iu\n", this, append, offset, count);

    if(append->size < offset)
        MSVCP__String_base_Xran();

    if(count > append->size-offset)
        count = append->size-offset;

    if(MSVCP_basic_string_wchar_npos-this->size<=count || this->size+count<this->size)
        MSVCP__String_base_Xlen();

    if(basic_string_wchar_grow(this, this->size+count, FALSE)) {
        MSVCP_char_traits_wchar__Copy_s(basic_string_wchar_ptr(this)+this->size,
                this->res-this->size, basic_string_wchar_const_ptr(append)+offset, count);
        basic_string_wchar_eos(this, this->size+count);
    }

    return this;
}

/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@ABV12@@Z */
/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@AEBV12@@Z */
/* ??Y?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV01@ABV01@@Z */
/* ??Y?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV01@AEBV01@@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@ABV12@@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@AEBV12@@Z */
/* ??Y?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV01@ABV01@@Z */
/* ??Y?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV01@AEBV01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append(
            basic_string_wchar *this, const basic_string_wchar *append)
{
    return MSVCP_basic_string_wchar_append_substr(this, append,
            0, MSVCP_basic_string_wchar_npos);
}

/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@PB_WI@Z */
/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@PEB_W_K@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBGI@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append_cstr_len, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_cstr_len(
        basic_string_wchar *this, const wchar_t *append, size_t count)
{
    TRACE("%p %s %Iu\n", this, debugstr_wn(append, count), count);

    if(basic_string_wchar_inside(this, append))
        return MSVCP_basic_string_wchar_append_substr(this, this,
                append-basic_string_wchar_ptr(this), count);

    if(MSVCP_basic_string_wchar_npos-this->size<=count || this->size+count<this->size)
        MSVCP__String_base_Xlen();

    if(basic_string_wchar_grow(this, this->size+count, FALSE)) {
        MSVCP_char_traits_wchar__Copy_s(basic_string_wchar_ptr(this)+this->size,
                this->res-this->size, append, count);
        basic_string_wchar_eos(this, this->size+count);
    }

    return this;
}

/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@PB_W@Z */
/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@PEB_W@Z */
/* ??Y?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV01@PB_W@Z */
/* ??Y?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV01@PEB_W@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBG@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG@Z */
/* ??Y?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV01@PBG@Z */
/* ??Y?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV01@PEBG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append_cstr, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_cstr(
        basic_string_wchar *this, const wchar_t *append)
{
    return MSVCP_basic_string_wchar_append_cstr_len(this, append,
            MSVCP_char_traits_wchar_length(append));
}

/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@PB_W0@Z */
/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@PEB_W0@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@PBG0@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@PEBG0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append_beg_end, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_beg_end(
        basic_string_wchar *this, const wchar_t *beg, const wchar_t *end)
{
    return MSVCP_basic_string_wchar_append_cstr_len(this, beg, end-beg);
}

/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@I_W@Z */
/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_K_W@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IG@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_KG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append_len_ch, 12)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_len_ch(
        basic_string_wchar *this, size_t count, wchar_t ch)
{
    TRACE("%p %Iu %c\n", this, count, ch);

    if(MSVCP_basic_string_wchar_npos-this->size <= count)
        MSVCP__String_base_Xlen();

    if(basic_string_wchar_grow(this, this->size+count, FALSE)) {
        MSVCP_char_traits_wchar_assignn(basic_string_wchar_ptr(this)+this->size, count, ch);
        basic_string_wchar_eos(this, this->size+count);
    }

    return this;
}

/* ??Y?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV01@_W@Z */
/* ??Y?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV01@_W@Z */
/* ?push_back@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEX_W@Z */
/* ?push_back@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAX_W@Z */
/* ??Y?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV01@G@Z */
/* ??Y?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV01@G@Z */
/* ?push_back@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXG@Z */
/* ?push_back@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append_ch, 8)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_ch(
        basic_string_wchar *this, wchar_t ch)
{
    return MSVCP_basic_string_wchar_append_len_ch(this, 1, ch);
}

/* ??$?H_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@ABV10@PB_W@Z */
/* ??$?H_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@AEBV10@PEB_W@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@ABV10@PBG@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@AEBV10@PEBG@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@ABV10@PBG@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@AEBV10@PEBG@Z */
/* ??Hstd@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@ABV10@PB_W@Z */
basic_string_wchar* __cdecl MSVCP_basic_string_wchar_concatenate_bstr_cstr(basic_string_wchar *ret,
        const basic_string_wchar *left, const wchar_t *right)
{
    TRACE("%p %s\n", left, debugstr_w(right));

    MSVCP_basic_string_wchar_copy_ctor(ret, left);
    MSVCP_basic_string_wchar_append_cstr(ret, right);
    return ret;
}

/* ??$?H_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PB_WABV10@@Z */
/* ??$?H_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PEB_WAEBV10@@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBGABV10@@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBGAEBV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBGABV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBGAEBV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PB_WABV10@@Z */
basic_string_wchar* __cdecl MSVCP_basic_string_wchar_concatenate_cstr_bstr(basic_string_wchar *ret,
        const wchar_t *left, const basic_string_wchar *right)
{
    TRACE("%s %p\n", debugstr_w(left), right);

    MSVCP_basic_string_wchar_ctor_cstr(ret, left);
    MSVCP_basic_string_wchar_append(ret, right);
    return ret;
}

/* ??$?H_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@ABV10@0@Z */
/* ??$?H_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@AEBV10@0@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@ABV10@0@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@AEBV10@0@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@ABV10@0@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@AEBV10@0@Z */
/* ??Hstd@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@ABV10@0@Z */
basic_string_wchar* __cdecl MSVCP_basic_string_wchar_concatenate(basic_string_wchar *ret,
        const basic_string_wchar *left, const basic_string_wchar *right)
{
    TRACE("%p %p\n", left, right);

    MSVCP_basic_string_wchar_copy_ctor(ret, left);
    MSVCP_basic_string_wchar_append(ret, right);
    return ret;
}

/* ??$?H_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@ABV10@_W@Z */
/* ??$?H_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@AEBV10@_W@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@ABV10@G@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@AEBV10@G@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@ABV10@G@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@AEBV10@G@Z */
/* ??Hstd@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@ABV10@_W@Z */
basic_string_wchar* __cdecl MSVCP_basic_string_wchar_concatenate_bstr_ch(
        basic_string_wchar *ret, const basic_string_wchar *left, wchar_t right)
{
    TRACE("%p %c\n", left, right);

    MSVCP_basic_string_wchar_copy_ctor(ret, left);
    MSVCP_basic_string_wchar_append_ch(ret, right);
    return ret;
}

/* ??$?H_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@_WABV10@@Z */
/* ??$?H_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@_WAEBV10@@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@GABV10@@Z */
/* ??$?HGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@GAEBV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@GABV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@GAEBV10@@Z */
/* ??Hstd@@YA?AV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@_WABV10@@Z */
basic_string_wchar* __cdecl MSVCP_basic_string_wchar_concatenate_ch_bstr(
        basic_string_wchar* ret, wchar_t left, const basic_string_wchar *right)
{
    TRACE("%c %p\n", left, right);

    MSVCP_basic_string_wchar_ctor_cstr_len(ret, &left, 1);
    MSVCP_basic_string_wchar_append(ret, right);
    return ret;
}

/* ?compare@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEHIIPB_WI@Z */
/* ?compare@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAH_K0PEB_W0@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEHIIPBGI@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAH_K0PEBG0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_compare_substr_cstr_len, 20)
int __thiscall MSVCP_basic_string_wchar_compare_substr_cstr_len(
        const basic_string_wchar *this, size_t pos, size_t num,
        const wchar_t *str, size_t count)
{
    int ans;

    TRACE("%p %Iu %Iu %s %Iu\n", this, pos, num, debugstr_wn(str, count), count);

    if(this->size < pos)
        MSVCP__String_base_Xran();

    if(num > this->size-pos)
        num = this->size-pos;

    ans = MSVCP_char_traits_wchar_compare(basic_string_wchar_const_ptr(this)+pos,
            str, num>count ? count : num);
    if(ans)
        return ans;

    if(num > count)
        ans = 1;
    else if(num < count)
        ans = -1;
    return ans;
}

/* ?compare@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEHIIPB_W@Z */
/* ?compare@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAH_K0PEB_W@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEHIIPBG@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAH_K0PEBG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_compare_substr_cstr, 16)
int __thiscall MSVCP_basic_string_wchar_compare_substr_cstr(const basic_string_wchar *this,
        size_t pos, size_t num, const wchar_t *str)
{
    return MSVCP_basic_string_wchar_compare_substr_cstr_len(this, pos, num,
            str, MSVCP_char_traits_wchar_length(str));
}

/* ?compare@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEHPB_W@Z */
/* ?compare@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAHPEB_W@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEHPBG@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAHPEBG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_compare_cstr, 8)
int __thiscall MSVCP_basic_string_wchar_compare_cstr(
        const basic_string_wchar *this, const wchar_t *str)
{
    return MSVCP_basic_string_wchar_compare_substr_cstr_len(this, 0, this->size,
            str, MSVCP_char_traits_wchar_length(str));
}

/* ?compare@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEHIIABV12@II@Z */
/* ?compare@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAH_K0AEBV12@00@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEHIIABV12@II@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAH_K0AEBV12@00@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_compare_substr_substr, 24)
int __thiscall MSVCP_basic_string_wchar_compare_substr_substr(
        const basic_string_wchar *this, size_t pos, size_t num,
        const basic_string_wchar *compare, size_t off, size_t count)
{
    TRACE("%p %Iu %Iu %p %Iu %Iu\n", this, pos, num, compare, off, count);

    if(compare->size < off)
        MSVCP__String_base_Xran();

    if(count > compare->size-off)
        count = compare->size-off;

    return MSVCP_basic_string_wchar_compare_substr_cstr_len(this, pos, num,
            basic_string_wchar_const_ptr(compare)+off, count);
}

/* ?compare@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEHIIABV12@@Z */
/* ?compare@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAH_K0AEBV12@@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEHIIABV12@@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAH_K0AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_compare_substr, 16)
int __thiscall MSVCP_basic_string_wchar_compare_substr(
        const basic_string_wchar *this, size_t pos, size_t num,
        const basic_string_wchar *compare)
{
    return MSVCP_basic_string_wchar_compare_substr_cstr_len(this, pos, num,
            basic_string_wchar_const_ptr(compare), compare->size);
}

/* ?compare@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEHABV12@@Z */
/* ?compare@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAHAEBV12@@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEHABV12@@Z */
/* ?compare@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAHAEBV12@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_compare, 8)
int __thiscall MSVCP_basic_string_wchar_compare(
        const basic_string_wchar *this, const basic_string_wchar *compare)
{
    return MSVCP_basic_string_wchar_compare_substr_cstr_len(this, 0, this->size,
            basic_string_wchar_const_ptr(compare), compare->size);
}

/* ??$?8_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
/* ??$?8_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
/* ??$?8GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??$?8GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??8std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??8std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??8std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_wchar_equal(
        const basic_string_wchar *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare(left, right) == 0;
}

/* ??$?8_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PB_W@Z */
/* ??$?8_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PEB_W@Z */
/* ??$?8GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??$?8GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
/* ??8std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??8std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
/* ??8std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PB_W@Z */
bool __cdecl MSVCP_basic_string_wchar_equal_str_cstr(
        const basic_string_wchar *left, const wchar_t *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(left, right) == 0;
}

/* ??$?8_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NPB_WABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?8_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NPEB_WAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?8GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?8GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??8std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??8std@@YA_NPB_WABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
bool __cdecl MSVCP_basic_string_wchar_equal_cstr_str(
        const wchar_t *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(right, left) == 0;
}

/* ??$?9_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
/* ??$?9_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
/* ??$?9GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??$?9GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??9std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??9std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??9std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_wchar_not_equal(
        const basic_string_wchar *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare(left, right) != 0;
}

/* ??$?9_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PB_W@Z */
/* ??$?9_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PEB_W@Z */
/* ??$?9GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??$?9GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
/* ??9std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??9std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
/* ??9std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PB_W@Z */
bool __cdecl MSVCP_basic_string_wchar_not_equal_str_cstr(
        const basic_string_wchar *left, const wchar_t *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(left, right) != 0;
}

/* ??$?9_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NPB_WABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?9_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NPEB_WAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?9GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?9GU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??9std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??9std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??9std@@YA_NPB_WABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
bool __cdecl MSVCP_basic_string_wchar_not_equal_cstr_str(
        const wchar_t *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(right, left) != 0;
}

/* ??$?M_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
/* ??$?M_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
/* ??$?MGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??$?MGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_wchar_lower(
        const basic_string_wchar *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare(left, right) < 0;
}

/* ??$?M_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PB_W@Z */
/* ??$?M_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PEB_W@Z */
/* ??$?MGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??$?MGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
bool __cdecl MSVCP_basic_string_wchar_lower_bstr_cstr(
        const basic_string_wchar *left, const wchar_t *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(left, right) < 0;
}

/* ??$?M_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NPB_WABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?M_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NPEB_WAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?MGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?MGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
bool __cdecl MSVCP_basic_string_wchar_lower_cstr_bstr(
        const wchar_t *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(right, left) > 0;
}

/* ??$?N_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
/* ??$?N_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
/* ??$?NGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??$?NGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_wchar_leq(
        const basic_string_wchar *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare(left, right) <= 0;
}

/* ??$?N_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PB_W@Z */
/* ??$?N_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PEB_W@Z */
/* ??$?NGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??$?NGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
bool __cdecl MSVCP_basic_string_wchar_leq_bstr_cstr(
        const basic_string_wchar *left, const wchar_t *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(left, right) <= 0;
}

/* ??$?N_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NPB_WABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?N_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NPEB_WAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?NGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?NGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
bool __cdecl MSVCP_basic_string_wchar_leq_cstr_bstr(
        const wchar_t *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(right, left) >= 0;
}

/* ??$?O_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
/* ??$?O_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
/* ??$?OGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??$?OGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_wchar_greater(
        const basic_string_wchar *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare(left, right) > 0;
}

/* ??$?O_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PB_W@Z */
/* ??$?O_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PEB_W@Z */
/* ??$?OGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??$?OGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
bool __cdecl MSVCP_basic_string_wchar_greater_bstr_cstr(
        const basic_string_wchar *left, const wchar_t *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(left, right) > 0;
}

/* ??$?O_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NPB_WABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?O_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NPEB_WAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?OGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?OGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
bool __cdecl MSVCP_basic_string_wchar_greater_cstr_bstr(
        const wchar_t *left, const basic_string_wchar *right)
{
    return MSVCP_basic_string_wchar_compare_cstr(right, left) < 0;
}

/* ??$?P_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
/* ??$?P_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@0@Z */
/* ??$?PGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
/* ??$?PGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@0@Z */
bool __cdecl MSVCP_basic_string_wchar_geq(
        const basic_string_wchar *left, const basic_string_wchar *right)
{
        return MSVCP_basic_string_wchar_compare(left, right) >= 0;
}

/* ??$?P_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PB_W@Z */
/* ??$?P_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@PEB_W@Z */
/* ??$?PGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PBG@Z */
/* ??$?PGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@PEBG@Z */
bool __cdecl MSVCP_basic_string_wchar_geq_bstr_cstr(
        const basic_string_wchar *left, const wchar_t *right)
{
        return MSVCP_basic_string_wchar_compare_cstr(left, right) >= 0;
}

/* ??$?P_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NPB_WABV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?P_WU?$char_traits@_W@std@@V?$allocator@_W@1@@std@@YA_NPEB_WAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@0@@Z */
/* ??$?PGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
/* ??$?PGU?$char_traits@G@std@@V?$allocator@G@1@@std@@YA_NPEBGAEBV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z */
bool __cdecl MSVCP_basic_string_wchar_geq_cstr_bstr(
        const wchar_t *left, const basic_string_wchar *right)
{
        return MSVCP_basic_string_wchar_compare_cstr(right, left) <= 0;
}

/* ?find@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPB_WII@Z */
/* ?find@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEB_W_K1@Z */
/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGII@Z */
/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_wchar_find_cstr_substr(
        const basic_string_wchar *this, const wchar_t *find, size_t pos, size_t len)
{
    const wchar_t *p, *end;

    TRACE("%p %s %Iu %Iu\n", this, debugstr_wn(find, len), pos, len);

    if(len==0 && pos<=this->size)
        return pos;
    if(pos>=this->size || len>this->size)
        return MSVCP_basic_string_wchar_npos;

    end = basic_string_wchar_const_ptr(this)+this->size-len+1;
    for(p=basic_string_wchar_const_ptr(this)+pos; p<end; p++) {
        p = MSVCP_char_traits_wchar_find(p, end-p, find);
        if(!p)
            break;

        if(!MSVCP_char_traits_wchar_compare(p, find, len))
            return p-basic_string_wchar_const_ptr(this);
    }

    return MSVCP_basic_string_wchar_npos;
}

/* ?find@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPB_WI@Z */
/* ?find@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEB_W_K@Z */
/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGI@Z */
/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_cstr_off, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_cstr_off(
        const basic_string_wchar *this, const wchar_t *find, size_t pos)
{
    return MSVCP_basic_string_wchar_find_cstr_substr(this, find, pos,
            MSVCP_char_traits_wchar_length(find));
}

/* ?find@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIABV12@I@Z */
/* ?find@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KAEBV12@_K@Z */
/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIABV12@I@Z */
/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_off, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_off(
        const basic_string_wchar *this, const basic_string_wchar *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_cstr_substr(this,
            basic_string_wchar_const_ptr(find), off, find->size);
}

/* ?find@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEI_WI@Z */
/* ?find@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_K_W_K@Z */
/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIGI@Z */
/* ?find@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_ch, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_ch(
        const basic_string_wchar *this, wchar_t ch, size_t pos)
{
    return MSVCP_basic_string_wchar_find_cstr_substr(this, &ch, pos, 1);
}

/* ?rfind@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPB_WII@Z */
/* ?rfind@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEB_W_K1@Z */
/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGII@Z */
/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_rfind_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_wchar_rfind_cstr_substr(
        const basic_string_wchar *this, const wchar_t *find, size_t pos, size_t len)
{
    const wchar_t *p, *end;

    TRACE("%p %s %Iu %Iu\n", this, debugstr_wn(find, len), pos, len);

    if(len==0)
        return pos<this->size ? pos : this->size;

    if(len > this->size)
        return MSVCP_basic_string_wchar_npos;

    if(pos > this->size-len)
        pos = this->size-len;
    end = basic_string_wchar_const_ptr(this);
    for(p=end+pos; p>=end; p--) {
        if(*p==*find && !MSVCP_char_traits_wchar_compare(p, find, len))
            return p-basic_string_wchar_const_ptr(this);
    }

    return MSVCP_basic_string_wchar_npos;
}

/* ?rfind@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPB_WI@Z */
/* ?rfind@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEB_W_K@Z */
/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGI@Z */
/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_rfind_cstr_off, 12)
size_t __thiscall MSVCP_basic_string_wchar_rfind_cstr_off(
        const basic_string_wchar *this, const wchar_t *find, size_t pos)
{
    return MSVCP_basic_string_wchar_rfind_cstr_substr(this, find, pos,
            MSVCP_char_traits_wchar_length(find));
}

/* ?rfind@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIABV12@I@Z */
/* ?rfind@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KAEBV12@_K@Z */
/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIABV12@I@Z */
/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_rfind_off, 12)
size_t __thiscall MSVCP_basic_string_wchar_rfind_off(
        const basic_string_wchar *this, const basic_string_wchar *find, size_t off)
{
    return MSVCP_basic_string_wchar_rfind_cstr_substr(this,
            basic_string_wchar_const_ptr(find), off, find->size);
}

/* ?rfind@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEI_WI@Z */
/* ?rfind@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_K_W_K@Z */
/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIGI@Z */
/* ?rfind@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_rfind_ch, 12)
size_t __thiscall MSVCP_basic_string_wchar_rfind_ch(
        const basic_string_wchar *this, wchar_t ch, size_t pos)
{
    return MSVCP_basic_string_wchar_rfind_cstr_substr(this, &ch, pos, 1);
}

/* ?find_first_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPB_WII@Z */
/* ?find_first_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEB_W_K1@Z */
/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGII@Z */
/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_wchar_find_first_of_cstr_substr(
        const basic_string_wchar *this, const wchar_t *find, size_t off, size_t len)
{
    const wchar_t *p, *end;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(len>0 && off<this->size) {
        end = basic_string_wchar_const_ptr(this)+this->size;
        for(p=basic_string_wchar_const_ptr(this)+off; p<end; p++)
            if(MSVCP_char_traits_wchar_find(find, len, p))
                return p-basic_string_wchar_const_ptr(this);
    }

    return MSVCP_basic_string_wchar_npos;
}

/* ?find_first_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIABV12@I@Z */
/* ?find_first_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KAEBV12@_K@Z */
/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIABV12@I@Z */
/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_of, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_first_of(
        const basic_string_wchar *this, const basic_string_wchar *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_first_of_cstr_substr(this,
            basic_string_wchar_const_ptr(find), off, find->size);
}

/* ?find_first_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPB_WI@Z */
/* ?find_first_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEB_W_K@Z */
/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGI@Z */
/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_first_of_cstr(
        const basic_string_wchar *this, const wchar_t *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_first_of_cstr_substr(
            this, find, off, MSVCP_char_traits_wchar_length(find));
}

/* ?find_first_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEI_WI@Z */
/* ?find_first_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_K_W_K@Z */
/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIGI@Z */
/* ?find_first_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_of_ch, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_first_of_ch(
        const basic_string_wchar *this, wchar_t ch, size_t off)
{
    return MSVCP_basic_string_wchar_find_first_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_first_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPB_WII@Z */
/* ?find_first_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEB_W_K1@Z */
/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGII@Z */
/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_not_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_wchar_find_first_not_of_cstr_substr(
        const basic_string_wchar *this, const wchar_t *find, size_t off, size_t len)
{
    const wchar_t *p, *end;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(off<this->size) {
        end = basic_string_wchar_const_ptr(this)+this->size;
        for(p=basic_string_wchar_const_ptr(this)+off; p<end; p++)
            if(!MSVCP_char_traits_wchar_find(find, len, p))
                return p-basic_string_wchar_const_ptr(this);
    }

    return MSVCP_basic_string_wchar_npos;
}

/* ?find_first_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIABV12@I@Z */
/* ?find_first_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KAEBV12@_K@Z */
/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIABV12@I@Z */
/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_not_of, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_first_not_of(
        const basic_string_wchar *this, const basic_string_wchar *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_first_not_of_cstr_substr(this,
            basic_string_wchar_const_ptr(find), off, find->size);
}

/* ?find_first_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPB_WI@Z */
/* ?find_first_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEB_W_K@Z */
/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGI@Z */
/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_not_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_first_not_of_cstr(
        const basic_string_wchar *this, const wchar_t *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_first_not_of_cstr_substr(
            this, find, off, MSVCP_char_traits_wchar_length(find));
}

/* ?find_first_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEI_WI@Z */
/* ?find_first_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_K_W_K@Z */
/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIGI@Z */
/* ?find_first_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_first_not_of_ch, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_first_not_of_ch(
        const basic_string_wchar *this, wchar_t ch, size_t off)
{
    return MSVCP_basic_string_wchar_find_first_not_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_last_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPB_WII@Z */
/* ?find_last_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEB_W_K1@Z */
/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGII@Z */
/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_wchar_find_last_of_cstr_substr(
        const basic_string_wchar *this, const wchar_t *find, size_t off, size_t len)
{
    const wchar_t *p, *beg;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);


    if(len>0 && this->size>0) {
        if(off >= this->size)
            off = this->size-1;

        beg = basic_string_wchar_const_ptr(this);
        for(p=beg+off; p>=beg; p--)
            if(MSVCP_char_traits_wchar_find(find, len, p))
                return p-beg;
    }

    return MSVCP_basic_string_wchar_npos;
}

/* ?find_last_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIABV12@I@Z */
/* ?find_last_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KAEBV12@_K@Z */
/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIABV12@I@Z */
/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_of, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_last_of(
        const basic_string_wchar *this, const basic_string_wchar *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_last_of_cstr_substr(this,
            basic_string_wchar_const_ptr(find), off, find->size);
}

/* ?find_last_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPB_WI@Z */
/* ?find_last_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEB_W_K@Z */
/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGI@Z */
/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_last_of_cstr(
        const basic_string_wchar *this, const wchar_t *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_last_of_cstr_substr(
            this, find, off, MSVCP_char_traits_wchar_length(find));
}

/* ?find_last_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEI_WI@Z */
/* ?find_last_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_K_W_K@Z */
/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIGI@Z */
/* ?find_last_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_of_ch, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_last_of_ch(
        const basic_string_wchar *this, wchar_t ch, size_t off)
{
    return MSVCP_basic_string_wchar_find_last_of_cstr_substr(this, &ch, off, 1);
}

/* ?find_last_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPB_WII@Z */
/* ?find_last_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEB_W_K1@Z */
/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGII@Z */
/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_not_of_cstr_substr, 16)
size_t __thiscall MSVCP_basic_string_wchar_find_last_not_of_cstr_substr(
        const basic_string_wchar *this, const wchar_t *find, size_t off, size_t len)
{
    const wchar_t *p, *beg;

    TRACE("%p %p %Iu %Iu\n", this, find, off, len);

    if(this->size>0) {
        if(off >= this->size)
            off = this->size-1;

        beg = basic_string_wchar_const_ptr(this);
        for(p=beg+off; p>=beg; p--)
            if(!MSVCP_char_traits_wchar_find(find, len, p))
                return p-beg;
    }

    return MSVCP_basic_string_wchar_npos;
}

/* ?find_last_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIABV12@I@Z */
/* ?find_last_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KAEBV12@_K@Z */
/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIABV12@I@Z */
/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KAEBV12@_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_not_of, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_last_not_of(
        const basic_string_wchar *this, const basic_string_wchar *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_last_not_of_cstr_substr(this,
            basic_string_wchar_const_ptr(find), off, find->size);
}

/* ?find_last_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEIPB_WI@Z */
/* ?find_last_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_KPEB_W_K@Z */
/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIPBGI@Z */
/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KPEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_not_of_cstr, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_last_not_of_cstr(
        const basic_string_wchar *this, const wchar_t *find, size_t off)
{
    return MSVCP_basic_string_wchar_find_last_not_of_cstr_substr(
            this, find, off, MSVCP_char_traits_wchar_length(find));
}

/* ?find_last_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEI_WI@Z */
/* ?find_last_not_of@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA_K_W_K@Z */
/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEIGI@Z */
/* ?find_last_not_of@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA_KG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_find_last_not_of_ch, 12)
size_t __thiscall MSVCP_basic_string_wchar_find_last_not_of_ch(
        const basic_string_wchar *this, wchar_t ch, size_t off)
{
    return MSVCP_basic_string_wchar_find_last_not_of_cstr_substr(this, &ch, off, 1);
}

/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@IIPB_WI@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_K0PEB_W0@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IIPBGI@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0PEBG0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_cstr_len, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_cstr_len(basic_string_wchar *this,
        size_t off, size_t len, const wchar_t *str, size_t str_len)
{
    size_t inside_pos = -1;
    wchar_t *ptr = basic_string_wchar_ptr(this);

    TRACE("%p %Iu %Iu %p %Iu\n", this, off, len, str, str_len);

    if(this->size < off)
        MSVCP__String_base_Xran();

    if(len > this->size-off)
        len = this->size-off;

    if(MSVCP_basic_string_wchar_npos-str_len <= this->size-len)
        MSVCP__String_base_Xlen();

    if(basic_string_wchar_inside(this, str))
        inside_pos = str-ptr;

    if(len < str_len) {
        basic_string_wchar_grow(this, this->size-len+str_len, FALSE);
        ptr = basic_string_wchar_ptr(this);
    }

    if(inside_pos == -1) {
        memmove(ptr+off+str_len, ptr+off+len, (this->size-off-len)*sizeof(wchar_t));
        memcpy(ptr+off, str, str_len*sizeof(wchar_t));
    } else if(len >= str_len) {
        memmove(ptr+off, ptr+inside_pos, str_len*sizeof(wchar_t));
        memmove(ptr+off+str_len, ptr+off+len, (this->size-off-len)*sizeof(wchar_t));
    } else {
        size_t size;

        memmove(ptr+off+str_len, ptr+off+len, (this->size-off-len)*sizeof(wchar_t));

        if(inside_pos < off+len) {
            size = off+len-inside_pos;
            if(size > str_len)
                size = str_len;
            memmove(ptr+off, ptr+inside_pos, size*sizeof(wchar_t));
        } else {
            size = 0;
        }

        if(str_len > size)
            memmove(ptr+off+size, ptr+off+str_len, (str_len-size)*sizeof(wchar_t));
    }

    basic_string_wchar_eos(this, this->size-len+str_len);
    return this;
}

/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@IIPB_W@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_K0PEB_W@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IIPBG@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0PEBG@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_cstr, 16)
basic_string_wchar* __thiscall basic_string_wchar_replace_cstr(basic_string_wchar *this,
        size_t off, size_t len, const wchar_t *str)
{
    return basic_string_wchar_replace_cstr_len(this, off, len, str,
            MSVCP_char_traits_wchar_length(str));
}

/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@IIABV12@II@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_K0AEBV12@00@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IIABV12@II@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0AEBV12@00@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_substr, 24)
basic_string_wchar* __thiscall basic_string_wchar_replace_substr(basic_string_wchar *this, size_t off,
        size_t len, const basic_string_wchar *str, size_t str_off, size_t str_len)
{
    if(str->size < str_off)
        MSVCP__String_base_Xran();

    if(str_len > str->size-str_off)
        str_len = str->size-str_off;

    return basic_string_wchar_replace_cstr_len(this, off, len,
            basic_string_wchar_const_ptr(str)+str_off, str_len);
}

/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@IIABV12@@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_K0AEBV12@@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IIABV12@@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace, 16)
basic_string_wchar* __thiscall basic_string_wchar_replace(basic_string_wchar *this,
        size_t off, size_t len, const basic_string_wchar *str)
{
    return basic_string_wchar_replace_cstr_len(this, off, len,
            basic_string_wchar_const_ptr(str), str->size);
}

/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@III_W@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_K00_W@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IIIG@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K00G@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_ch, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_ch(basic_string_wchar *this,
        size_t off, size_t len, size_t count, wchar_t ch)
{
    wchar_t *ptr = basic_string_wchar_ptr(this);

    TRACE("%p %Iu %Iu %Iu %c\n", this, off, len, count, ch);

    if(this->size < off)
        MSVCP__String_base_Xran();

    if(len > this->size-off)
        len = this->size-off;

    if(MSVCP_basic_string_wchar_npos-count <= this->size-len)
        MSVCP__String_base_Xlen();

    if(len < count) {
        basic_string_wchar_grow(this, this->size-len+count, FALSE);
        ptr = basic_string_wchar_ptr(this);
    }

    memmove(ptr+off+count, ptr+off+len, (this->size-off-len)*sizeof(wchar_t));
    MSVCP_char_traits_wchar_assignn(ptr+off, count, ch);
    basic_string_wchar_eos(this, this->size-len+count);

    return this;
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@IABV12@@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_KAEBV12@@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IABV12@@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_KAEBV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert, 12)
basic_string_wchar* __thiscall basic_string_wchar_insert(basic_string_wchar *this,
        size_t off, const basic_string_wchar *str)
{
    return basic_string_wchar_replace(this, off, 0, str);
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@IABV12@II@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_KAEBV12@00@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IABV12@II@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_KAEBV12@00@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_substr, 20)
basic_string_wchar* __thiscall basic_string_wchar_insert_substr(
        basic_string_wchar *this, size_t off, const basic_string_wchar *str,
        size_t str_off, size_t str_count)
{
    return basic_string_wchar_replace_substr(this, off, 0, str, str_off, str_count);
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@IPB_W@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_KPEB_W@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IPBG@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_KPEBG@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_cstr, 12)
basic_string_wchar* __thiscall basic_string_wchar_insert_cstr(
        basic_string_wchar *this, size_t off, const wchar_t *str)
{
    return basic_string_wchar_replace_cstr(this, off, 0, str);
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@IPB_WI@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_KPEB_W0@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IPBGI@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_KPEBG0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_cstr_len, 16)
basic_string_wchar* __thiscall basic_string_wchar_insert_cstr_len(basic_string_wchar *this,
        size_t off, const wchar_t *str, size_t str_len)
{
    return basic_string_wchar_replace_cstr_len(this, off, 0, str, str_len);
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@II_W@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@_K0_W@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@IIG@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@_K0G@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_chn, 16)
basic_string_wchar* __thiscall basic_string_wchar_insert_chn(basic_string_wchar *this,
        size_t off, size_t count, wchar_t ch)
{
    return basic_string_wchar_replace_ch(this, off, 0, count, ch);
}

/* ??A?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAA_WI@Z */
/* ??A?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEA_W_K@Z */
/* ??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAGI@Z */
/* ??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_operator_at, 8)
wchar_t* __thiscall MSVCP_basic_string_wchar_operator_at(
        basic_string_wchar *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

#if _MSVCP_VER >= 80
    if (this->size < pos)
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
#endif

    return basic_string_wchar_ptr(this)+pos;
}

/* ??A?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEAB_WI@Z */
/* ??A?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAAEB_W_K@Z */
/* ??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEABGI@Z */
/* ??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAAEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_const_operator_at, 8)
const wchar_t* __thiscall MSVCP_basic_string_wchar_const_operator_at(
        const basic_string_wchar *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

#if _MSVCP_VER >= 80
    if (this->size < pos)
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
#endif

    return basic_string_wchar_const_ptr(this)+pos;
}

/* ??A?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAA_WU_Size_type_nosscl@01@@Z */
/* ??A?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEA_WU_Size_type_nosscl@01@@Z */
/* ??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAGU_Size_type_nosscl@01@@Z */
/* ??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAGU_Size_type_nosscl@01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_operator_at_noverify, 8)
wchar_t* __thiscall MSVCP_basic_string_wchar_operator_at_noverify(
        basic_string_wchar *this, size_t_noverify pos)
{
    TRACE("%p %Iu\n", this, pos.val);
    return basic_string_wchar_ptr(this)+pos.val;
}

/* ??A?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEAB_WU_Size_type_nosscl@01@@Z */
/* ??A?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAAEB_WU_Size_type_nosscl@01@@Z */
/* ??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEABGU_Size_type_nosscl@01@@Z */
/* ??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAAEBGU_Size_type_nosscl@01@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_operator_const_at_noverify, 8)
const wchar_t* __thiscall MSVCP_basic_string_wchar_operator_const_at_noverify(
        const basic_string_wchar *this, size_t_noverify pos)
{
    TRACE("%p %Iu\n", this, pos.val);
    return basic_string_wchar_const_ptr(this)+pos.val;
}

/* ?at@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAA_WI@Z */
/* ?at@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEA_W_K@Z */
/* ?at@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAGI@Z */
/* ?at@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_at, 8)
wchar_t* __thiscall MSVCP_basic_string_wchar_at(
        basic_string_wchar *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

    if(this->size <= pos)
        MSVCP__String_base_Xran();

    return basic_string_wchar_ptr(this)+pos;
}

/* ?at@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBEAB_WI@Z */
/* ?at@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBAAEB_W_K@Z */
/* ?at@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEABGI@Z */
/* ?at@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBAAEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_const_at, 8)
const wchar_t* __thiscall MSVCP_basic_string_wchar_const_at(
        const basic_string_wchar *this, size_t pos)
{
    TRACE("%p %Iu\n", this, pos);

    if(this->size <= pos)
        MSVCP__String_base_Xran();

    return basic_string_wchar_const_ptr(this)+pos;
}

/* ?resize@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXI_W@Z */
/* ?resize@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAX_K_W@Z */
/* ?resize@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXIG@Z */
/* ?resize@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAX_KG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_resize_ch, 12)
void __thiscall MSVCP_basic_string_wchar_resize_ch(
        basic_string_wchar *this, size_t size, wchar_t ch)
{
    TRACE("%p %Iu %c\n", this, size, ch);

    if(size <= this->size)
        MSVCP_basic_string_wchar_erase(this, size, this->size);
    else
        MSVCP_basic_string_wchar_append_len_ch(this, size-this->size, ch);
}

/* ?resize@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXI@Z */
/* ?resize@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAX_K@Z */
/* ?resize@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXI@Z */
/* ?resize@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAX_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_resize, 8)
void __thiscall MSVCP_basic_string_wchar_resize(
        basic_string_wchar *this, size_t size)
{
    MSVCP_basic_string_wchar_resize_ch(this, size, '\0');
}

/* ?clear@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXXZ */
/* ?clear@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXXZ */
/* ?clear@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXXZ */
/* ?clear@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_clear, 4)
void __thiscall MSVCP_basic_string_wchar_clear(basic_string_wchar *this)
{
    basic_string_wchar_eos(this, 0);
}

/* _String_val class */
/* ??_F?$_String_val@DV?$allocator@D@std@@@std@@QAEXXZ */
/* ??_F?$_String_val@DV?$allocator@D@std@@@std@@QEAAXXZ */
/* ??_F?$_String_val@GV?$allocator@G@std@@@std@@QAEXXZ */
/* ??_F?$_String_val@GV?$allocator@G@std@@@std@@QEAAXXZ */
/* ??_F?$_String_val@_WV?$allocator@_W@std@@@std@@QAEXXZ */
/* ??_F?$_String_val@_WV?$allocator@_W@std@@@std@@QEAAXXZ */
DEFINE_THISCALL_WRAPPER(_String_val_ctor, 4)
void* __thiscall _String_val_ctor(void *this)
{
    TRACE("%p\n", this);
    return this;
}

/* ??0?$_String_val@DV?$allocator@D@std@@@std@@IAE@V?$allocator@D@1@@Z */
/* ??0?$_String_val@DV?$allocator@D@std@@@std@@IEAA@V?$allocator@D@1@@Z */
/* ??0?$_String_val@GV?$allocator@G@std@@@std@@IAE@V?$allocator@G@1@@Z */
/* ??0?$_String_val@GV?$allocator@G@std@@@std@@IEAA@V?$allocator@G@1@@Z */
/* ??0?$_String_val@_WV?$allocator@_W@std@@@std@@IAE@V?$allocator@_W@1@@Z */
/* ??0?$_String_val@_WV?$allocator@_W@std@@@std@@IEAA@V?$allocator@_W@1@@Z */
/* ??0?$_String_val@DV?$allocator@D@std@@@std@@QAE@ABV01@@Z */
/* ??0?$_String_val@DV?$allocator@D@std@@@std@@QEAA@AEBV01@@Z */
/* ??0?$_String_val@GV?$allocator@G@std@@@std@@QAE@ABV01@@Z */
/* ??0?$_String_val@GV?$allocator@G@std@@@std@@QEAA@AEBV01@@Z */
/* ??0?$_String_val@_WV?$allocator@_W@std@@@std@@QAE@ABV01@@Z */
/* ??0?$_String_val@_WV?$allocator@_W@std@@@std@@QEAA@AEBV01@@Z */
/* ??4?$_String_val@DV?$allocator@D@std@@@std@@QAEAAV01@ABV01@@Z */
/* ??4?$_String_val@DV?$allocator@D@std@@@std@@QEAAAEAV01@AEBV01@@Z */
/* ??4?$_String_val@GV?$allocator@G@std@@@std@@QAEAAV01@ABV01@@Z */
/* ??4?$_String_val@GV?$allocator@G@std@@@std@@QEAAAEAV01@AEBV01@@Z */
/* ??4?$_String_val@_WV?$allocator@_W@std@@@std@@QAEAAV01@ABV01@@Z */
/* ??4?$_String_val@_WV?$allocator@_W@std@@@std@@QEAAAEAV01@AEBV01@@Z */
/* ??4?$_String_val@DV?$allocator@D@std@@@std@@QAEAAV01@ABV01@@Z */
DEFINE_THISCALL_WRAPPER(_String_val_null_ctor, 8)
void* __thiscall _String_val_null_ctor(void *this, const void *misc)
{
    TRACE("%p %p\n", this, misc);
    return this;
}

#if _MSVCP_VER < 80  /* old iterator functions */

typedef struct {
    const char *pos;
} basic_string_char_iterator;

typedef struct {
    const wchar_t *pos;
} basic_string_wchar_iterator;

/* ?_Pdif@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@KAIVconst_iterator@12@0@Z */
/* ?_Pdif@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@CAIV?$_Ptrit@DHPBDABDPADAAD@2@0@Z */
size_t __cdecl basic_string_char__Pdif(basic_string_char_iterator i1, basic_string_char_iterator i2)
{
    TRACE("(%p %p)\n", i1.pos, i2.pos);
    return !i1.pos ? 0 : i1.pos-i2.pos;
}

/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AViterator@12@V312@0@Z */
/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$_Ptrit@DHPADAADPADAAD@2@V32@0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_erase_iter_range, 16)
basic_string_char_iterator* __thiscall basic_string_char_erase_iter_range(basic_string_char *this,
        basic_string_char_iterator *ret, basic_string_char_iterator beg, basic_string_char_iterator end)
{
    size_t off;

    ret->pos = basic_string_char_ptr(this);
    off = basic_string_char__Pdif(beg, *ret);

    MSVCP_basic_string_char_erase(this, off, basic_string_char__Pdif(end, beg));

    ret->pos = basic_string_char_ptr(this)+off;
    return ret;
}

/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AViterator@12@V312@@Z */
/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$_Ptrit@DHPADAADPADAAD@2@V32@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_erase_iter, 12)
basic_string_char_iterator* __thiscall basic_string_char_erase_iter(basic_string_char *this,
        basic_string_char_iterator *ret, basic_string_char_iterator pos)
{
    size_t off;

    ret->pos = basic_string_char_ptr(this);
    off = basic_string_char__Pdif(pos, *ret);

    MSVCP_basic_string_char_erase(this, off, 1);

    ret->pos = basic_string_char_ptr(this)+off;
    return ret;
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Vconst_iterator@12@0@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_Ptrit@DHPBDABDPADAAD@2@0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_assign_iter, 12)
basic_string_char* __thiscall basic_string_char_assign_iter(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end)
{
    return MSVCP_basic_string_char_assign_ptr_ptr(this, beg.pos, end.pos);
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@Vconst_iterator@01@0@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@V?$_Ptrit@DHPBDABDPADAAD@1@0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_ctor_iter, 12)
basic_string_char* __thiscall basic_string_char_ctor_iter(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end)
{
    return MSVCP_basic_string_char_ctor_cstr_len(this, beg.pos, end.pos-beg.pos);
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Vconst_iterator@12@0@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_Ptrit@DHPBDABDPADAAD@2@0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_append_iter, 12)
basic_string_char* __thiscall basic_string_char_append_iter(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end)
{
    return basic_string_char_replace_cstr_len(this, this->size, 0, beg.pos, end.pos-beg.pos);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Viterator@12@0Vconst_iterator@12@1@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_Ptrit@DHPADAADPADAAD@2@0V?$_Ptrit@DHPBDABDPADAAD@2@1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_iter, 20)
basic_string_char* __thiscall basic_string_char_replace_iter_iter(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end,
                basic_string_char_iterator rbeg, basic_string_char_iterator rend)
{
    return basic_string_char_replace_cstr_len(this, beg.pos-basic_string_char_ptr(this),
            end.pos-beg.pos, rbeg.pos, rend.pos-rbeg.pos);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Viterator@12@0ABV12@@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_Ptrit@DHPADAADPADAAD@2@0ABV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_bstr, 16)
basic_string_char* __thiscall basic_string_char_replace_iter_bstr(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end, const basic_string_char *str)
{
    return basic_string_char_replace_cstr_len(this, beg.pos-basic_string_char_ptr(this),
            end.pos-beg.pos, basic_string_char_const_ptr(str), str->size);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Viterator@12@0ID@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_Ptrit@DHPADAADPADAAD@2@0ID@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_ch, 20)
basic_string_char* __thiscall basic_string_char_replace_iter_ch(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end, size_t count, char ch)
{
    /* TODO: add more efficient implementation */
    size_t off = beg.pos-basic_string_char_ptr(this);

    basic_string_char_replace_cstr_len(this, off, end.pos-beg.pos, NULL, 0);
    while(count--)
        basic_string_char_replace_cstr_len(this, off, 0, &ch, 1);
    return this;
}

static basic_string_char* basic_string_char_replace_iter_ptr_ptr(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end,
        const char *res_beg, const char *res_end)
{
    basic_string_char_iterator begin = { basic_string_char_ptr(this) };
    return basic_string_char_replace_cstr_len(this, basic_string_char__Pdif(beg, begin),
            basic_string_char__Pdif(end, beg), res_beg, res_end-res_beg);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Viterator@12@0PBD1@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_Ptrit@DHPADAADPADAAD@2@0PBD1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_beg_end, 20)
basic_string_char* __thiscall basic_string_char_replace_iter_beg_end(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end, const char *rbeg, const char *rend)
{
    return basic_string_char_replace_cstr_len(this, beg.pos-basic_string_char_ptr(this),
            end.pos-beg.pos, rbeg, rend-rbeg);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Viterator@12@0PBD@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_Ptrit@DHPADAADPADAAD@2@0PBD@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_cstr, 16)
basic_string_char* __thiscall basic_string_char_replace_iter_cstr(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end, const char *str)
{
    return basic_string_char_replace_cstr_len(this, beg.pos-basic_string_char_ptr(this),
            end.pos-beg.pos, str, strlen(str));
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@Viterator@12@0PBDI@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_Ptrit@DHPADAADPADAAD@2@0PBDI@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_replace_iter_cstr_len, 20)
basic_string_char* __thiscall basic_string_char_replace_iter_cstr_len(basic_string_char *this,
        basic_string_char_iterator beg, basic_string_char_iterator end, const char *str, size_t len)
{
    return basic_string_char_replace_cstr_len(this, beg.pos-basic_string_char_ptr(this),
            end.pos-beg.pos, str, len);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXViterator@12@ID@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXV?$_Ptrit@DHPADAADPADAAD@2@ID@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_iter_chn, 16)
void __thiscall basic_string_char_insert_iter_chn(basic_string_char *this,
        basic_string_char_iterator where, size_t count, char ch)
{
    basic_string_char_iterator iter = { basic_string_char_ptr(this) };
    size_t off = basic_string_char__Pdif(where, iter);

    basic_string_char_insert_chn(this, off, count, ch);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AViterator@12@V312@D@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$_Ptrit@DHPADAADPADAAD@2@V32@D@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_iter_ch, 16)
basic_string_char_iterator* __thiscall basic_string_char_insert_iter_ch(basic_string_char *this,
        basic_string_char_iterator *ret, basic_string_char_iterator where, char ch)
{
    size_t off;

    ret->pos = basic_string_char_ptr(this);
    off = basic_string_char__Pdif(where, *ret);

    basic_string_char_insert_chn(this, off, 1, ch);
    ret->pos = basic_string_char_ptr(this)+off;
    return ret;
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AViterator@12@V312@@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$_Ptrit@DHPADAADPADAAD@2@V32@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_iter_null, 12)
basic_string_char_iterator* __thiscall basic_string_char_insert_iter_null(basic_string_char *this,
        basic_string_char_iterator *ret, basic_string_char_iterator where)
{
    return basic_string_char_insert_iter_ch(this, ret, where, 0);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXViterator@12@PBD1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_iter_ptr_ptr, 16)
void __thiscall basic_string_char_insert_iter_ptr_ptr(basic_string_char *this,
        basic_string_char_iterator where, const char *beg, const char *end)
{
    basic_string_char_replace_iter_ptr_ptr(this, where, where, beg, end);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXViterator@12@PBD1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_iter_beg_end, 16)
void __thiscall basic_string_char_insert_iter_beg_end(basic_string_char *this,
        basic_string_char_iterator where, basic_string_char_iterator beg, basic_string_char_iterator end)
{
    basic_string_char_replace_iter_ptr_ptr(this, where, where, beg.pos, end.pos);
}

/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AViterator@12@XZ */
/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AVconst_iterator@12@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$reverse_iterator@Viterator@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$reverse_iterator@Vconst_iterator@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$_Ptrit@DHPADAADPADAAD@2@XZ */
/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$_Ptrit@DHPBDABDPADAAD@2@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$reverse_iterator@V?$_Ptrit@DHPADAADPADAAD@std@@@2@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$reverse_iterator@V?$_Ptrit@DHPBDABDPADAAD@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_begin, 8)
basic_string_char_iterator* __thiscall basic_string_char_begin(
        basic_string_char *this, basic_string_char_iterator *ret)
{
    ret->pos = basic_string_char_ptr(this);
    return ret;
}

/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AViterator@12@XZ */
/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AVconst_iterator@12@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$reverse_iterator@Viterator@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$reverse_iterator@Vconst_iterator@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$_Ptrit@DHPADAADPADAAD@2@XZ */
/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$_Ptrit@DHPBDABDPADAAD@2@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$reverse_iterator@V?$_Ptrit@DHPADAADPADAAD@std@@@2@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$reverse_iterator@V?$_Ptrit@DHPBDABDPADAAD@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_char_end, 8)
basic_string_char_iterator* __thiscall basic_string_char_end(
        basic_string_char *this, basic_string_char_iterator *ret)
{
    ret->pos = basic_string_char_ptr(this)+this->size;
    return ret;
}

/* ?_Pdif@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@KAIVconst_iterator@12@0@Z */
/* ?_Pdif@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@KAIVconst_iterator@12@0@Z */
/* ?_Pdif@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@CAIV?$_Ptrit@_WHPB_WAB_WPA_WAA_W@2@0@Z */
/* ?_Pdif@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@CAIV?$_Ptrit@GHPBGABGPAGAAG@2@0@Z */
size_t __cdecl basic_string_wchar__Pdif(basic_string_wchar_iterator i1, basic_string_wchar_iterator i2)
{
    TRACE("(%p %p)\n", i1.pos, i2.pos);
    return !i1.pos ? 0 : i1.pos-i2.pos;
}

/* ?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AViterator@12@V312@0@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AViterator@12@V312@0@Z */
/* ?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@V32@0@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$_Ptrit@GHPAGAAGPAGAAG@2@V32@0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_erase_iter_range, 16)
basic_string_wchar_iterator* __thiscall basic_string_wchar_erase_iter_range(basic_string_wchar *this,
        basic_string_wchar_iterator *ret, basic_string_wchar_iterator beg, basic_string_wchar_iterator end)
{
    size_t off;

    ret->pos = basic_string_wchar_ptr(this);
    off = basic_string_wchar__Pdif(beg, *ret);

    MSVCP_basic_string_wchar_erase(this, off, basic_string_wchar__Pdif(end, beg));

    ret->pos = basic_string_wchar_ptr(this)+off;
    return ret;
}

/* ?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AViterator@12@V312@@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AViterator@12@V312@@Z */
/* ?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@V32@@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$_Ptrit@GHPAGAAGPAGAAG@2@V32@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_erase_iter, 12)
basic_string_wchar_iterator* __thiscall basic_string_wchar_erase_iter(basic_string_wchar *this,
        basic_string_wchar_iterator *ret, basic_string_wchar_iterator pos)
{
    size_t off;

    ret->pos = basic_string_wchar_ptr(this);
    off = basic_string_wchar__Pdif(pos, *ret);

    MSVCP_basic_string_wchar_erase(this, off, 1);

    ret->pos = basic_string_wchar_ptr(this)+off;
    return ret;
}

/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Vconst_iterator@12@0@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Vconst_iterator@12@0@Z */
/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_Ptrit@_WHPB_WAB_WPA_WAA_W@2@0@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_Ptrit@GHPBGABGPAGAAG@2@0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_assign_iter, 12)
basic_string_wchar* __thiscall basic_string_wchar_assign_iter(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end)
{
        return MSVCP_basic_string_wchar_assign_ptr_ptr(this, beg.pos, end.pos);
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@Vconst_iterator@01@0@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@Vconst_iterator@01@0@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@V?$_Ptrit@_WHPB_WAB_WPA_WAA_W@1@0@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@V?$_Ptrit@GHPBGABGPAGAAG@1@0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_ctor_iter, 12)
basic_string_wchar* __thiscall basic_string_wchar_ctor_iter(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end)
{
    return MSVCP_basic_string_wchar_ctor_cstr_len(this, beg.pos, end.pos-beg.pos);
}

/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Vconst_iterator@12@0@Z */
/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Vconst_iterator@12@0@Z */
/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_Ptrit@_WHPB_WAB_WPA_WAA_W@2@0@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_Ptrit@GHPBGABGPAGAAG@2@0@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_append_iter, 12)
basic_string_wchar* __thiscall basic_string_wchar_append_iter(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end)
{
    return basic_string_wchar_replace_cstr_len(this, this->size, 0, beg.pos, end.pos-beg.pos);
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Viterator@12@0Vconst_iterator@12@1@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Viterator@12@0Vconst_iterator@12@1@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_Ptrit@GHPAGAAGPAGAAG@2@0V?$_Ptrit@GHPBGABGPAGAAG@2@1@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@0V?$_Ptrit@_WHPB_WAB_WPA_WAA_W@2@1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_iter, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_iter(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end,
        basic_string_wchar_iterator rbeg, basic_string_wchar_iterator rend)
{
    return basic_string_wchar_replace_cstr_len(this, beg.pos-basic_string_wchar_ptr(this),
            end.pos-beg.pos, rbeg.pos, rend.pos-rbeg.pos);
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Viterator@12@0ABV12@@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Viterator@12@0ABV12@@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_Ptrit@GHPAGAAGPAGAAG@2@0ABV12@@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@0ABV12@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_bstr, 16)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_bstr(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end, basic_string_wchar *str)
{
    return basic_string_wchar_replace_cstr_len(this, beg.pos-basic_string_wchar_ptr(this),
            end.pos-beg.pos, basic_string_wchar_ptr(str), str->size);
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Viterator@12@0IG@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Viterator@12@0I_W@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_Ptrit@GHPAGAAGPAGAAG@2@0IG@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@0I_W@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_ch, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_ch(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end, size_t count, wchar_t ch)
{
    /* TODO: add more efficient implementation */
    size_t off = beg.pos-basic_string_wchar_ptr(this);

    basic_string_wchar_replace_cstr_len(this, off, end.pos-beg.pos, NULL, 0);
    while(count--)
        basic_string_wchar_replace_cstr_len(this, off, 0, &ch, 1);
    return this;
}

static basic_string_wchar* basic_string_wchar_replace_iter_ptr_ptr(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end,
        const wchar_t *res_beg, const wchar_t *res_end)
{
    basic_string_wchar_iterator begin = { basic_string_wchar_ptr(this) };
    return basic_string_wchar_replace_cstr_len(this, basic_string_wchar__Pdif(beg, begin),
            basic_string_wchar__Pdif(end, beg), res_beg, res_end-res_beg);
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Viterator@12@0PBG1@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Viterator@12@0PB_W1@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_Ptrit@GHPAGAAGPAGAAG@2@0PBG1@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@0PB_W1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_beg_end, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_beg_end(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end,
        const wchar_t *rbeg, const wchar_t *rend)
{
    return basic_string_wchar_replace_cstr_len(this, beg.pos-basic_string_wchar_ptr(this),
            end.pos-beg.pos, rbeg, rend-rbeg);
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Viterator@12@0PBG@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Viterator@12@0PB_W@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_Ptrit@GHPAGAAGPAGAAG@2@0PBG@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@0PB_W@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_cstr, 16)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_cstr(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end, const wchar_t *str)
{
    return basic_string_wchar_replace_cstr_len(this, beg.pos-basic_string_wchar_ptr(this),
            end.pos-beg.pos, str, wcslen(str));
}

/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@Viterator@12@0PBGI@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@Viterator@12@0PB_WI@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_Ptrit@GHPAGAAGPAGAAG@2@0PBGI@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@0PB_WI@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_replace_iter_cstr_len, 20)
basic_string_wchar* __thiscall basic_string_wchar_replace_iter_cstr_len(basic_string_wchar *this,
        basic_string_wchar_iterator beg, basic_string_wchar_iterator end,
        const wchar_t *str, size_t len)
{
    return basic_string_wchar_replace_cstr_len(this, beg.pos-basic_string_wchar_ptr(this),
            end.pos-beg.pos, str, len);
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXViterator@12@I_W@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXViterator@12@IG@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXV?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@I_W@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXV?$_Ptrit@GHPAGAAGPAGAAG@2@IG@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_iter_chn, 16)
void __thiscall basic_string_wchar_insert_iter_chn(basic_string_wchar *this,
        basic_string_wchar_iterator where, size_t count, wchar_t ch)
{
    basic_string_wchar_iterator iter = { basic_string_wchar_ptr(this) };
    size_t off = basic_string_wchar__Pdif(where, iter);

    basic_string_wchar_insert_chn(this, off, count, ch);
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AViterator@12@V312@_W@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AViterator@12@V312@G@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@V32@_W@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$_Ptrit@GHPAGAAGPAGAAG@2@V32@G@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_iter_ch, 16)
basic_string_wchar_iterator* __thiscall basic_string_wchar_insert_iter_ch(basic_string_wchar *this,
        basic_string_wchar_iterator *ret, basic_string_wchar_iterator where, wchar_t ch)
{
    size_t off;

    ret->pos = basic_string_wchar_ptr(this);
    off = basic_string_wchar__Pdif(where, *ret);

    basic_string_wchar_insert_chn(this, off, 1, ch);
    ret->pos = basic_string_wchar_ptr(this)+off;
    return ret;
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AViterator@12@V312@@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AViterator@12@V312@@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@V32@@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$_Ptrit@GHPAGAAGPAGAAG@2@V32@@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_iter_null, 12)
basic_string_wchar_iterator* __thiscall basic_string_wchar_insert_iter_null(basic_string_wchar *this,
        basic_string_wchar_iterator *ret, basic_string_wchar_iterator where)
{
    return basic_string_wchar_insert_iter_ch(this, ret, where, 0);
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXViterator@12@PB_W1@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXViterator@12@PBG1@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXV?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@PB_W1@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXV?$_Ptrit@GHPAGAAGPAGAAG@2@PBG1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_iter_ptr_ptr, 16)
void __thiscall basic_string_wchar_insert_iter_ptr_ptr(basic_string_wchar *this,
        basic_string_wchar_iterator where, const wchar_t *beg, const wchar_t *end)
{
    basic_string_wchar_replace_iter_ptr_ptr(this, where, where, beg, end);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXViterator@12@Vconst_iterator@12@1@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXViterator@12@Vconst_iterator@12@1@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXViterator@12@Vconst_iterator@12@1@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXV?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@V?$_Ptrit@_WHPB_WAB_WPA_WAA_W@2@1@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXV?$_Ptrit@GHPAGAAGPAGAAG@2@V?$_Ptrit@GHPBGABGPAGAAG@2@1@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_iter_beg_end, 16)
void __thiscall basic_string_wchar_insert_iter_beg_end(basic_string_wchar *this,
        basic_string_wchar_iterator where, basic_string_wchar_iterator beg, basic_string_wchar_iterator end)
{
    basic_string_wchar_replace_iter_ptr_ptr(this, where, where, beg.pos, end.pos);
}

/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AViterator@12@XZ */
/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AVconst_iterator@12@XZ */
/* ?begin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AViterator@12@XZ */
/* ?begin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AVconst_iterator@12@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@Viterator@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$reverse_iterator@Vconst_iterator@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$reverse_iterator@Viterator@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$reverse_iterator@Vconst_iterator@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$_Ptrit@GHPAGAAGPAGAAG@2@XZ */
/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$_Ptrit@GHPBGABGPAGAAG@2@XZ */
/* ?begin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@XZ */
/* ?begin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$_Ptrit@_WHPB_WAB_WPA_WAA_W@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@V?$_Ptrit@GHPAGAAGPAGAAG@std@@@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$reverse_iterator@V?$_Ptrit@GHPBGABGPAGAAG@std@@@2@XZ */
/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$reverse_iterator@V?$_Ptrit@_WHPA_WAA_WPA_WAA_W@std@@@2@XZ */
/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$reverse_iterator@V?$_Ptrit@_WHPB_WAB_WPA_WAA_W@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_begin, 8)
basic_string_wchar_iterator* __thiscall basic_string_wchar_begin(
        basic_string_wchar *this, basic_string_wchar_iterator *ret)
{
    ret->pos = basic_string_wchar_ptr(this);
    return ret;
}

/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AViterator@12@XZ */
/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AVconst_iterator@12@XZ */
/* ?end@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AViterator@12@XZ */
/* ?end@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AVconst_iterator@12@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@Viterator@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$reverse_iterator@Vconst_iterator@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$reverse_iterator@Viterator@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$reverse_iterator@Vconst_iterator@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$_Ptrit@GHPAGAAGPAGAAG@2@XZ */
/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$_Ptrit@GHPBGABGPAGAAG@2@XZ */
/* ?end@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$_Ptrit@_WHPA_WAA_WPA_WAA_W@2@XZ */
/* ?end@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$_Ptrit@_WHPB_WAB_WPA_WAA_W@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@V?$_Ptrit@GHPAGAAGPAGAAG@std@@@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$reverse_iterator@V?$_Ptrit@GHPBGABGPAGAAG@std@@@2@XZ */
/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$reverse_iterator@V?$_Ptrit@_WHPA_WAA_WPA_WAA_W@std@@@2@XZ */
/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$reverse_iterator@V?$_Ptrit@_WHPB_WAB_WPA_WAA_W@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_end, 8)
basic_string_wchar_iterator* __thiscall basic_string_wchar_end(
        basic_string_wchar *this, basic_string_wchar_iterator *ret)
{
    ret->pos = basic_string_wchar_ptr(this)+this->size;
    return ret;
}

#else /* _MSVCP_VER >= 80, new iterator functions */

/* _String_iterator<char> and _String_const_iterator<char> class */
typedef struct {
    basic_string_char *bstr;
    const char *pos;
} String_iterator_char;

typedef struct {
#if _MSVCP_VER == 80
    void *cont;
#endif
    const basic_string_char *bstr;
    const char *pos;
} String_reverse_iterator_char;

typedef struct {
    basic_string_wchar *bstr;
    const wchar_t *pos;
} String_iterator_wchar;

typedef struct {
#if _MSVCP_VER == 80
    void *cont;
#endif
    const basic_string_wchar *bstr;
    const wchar_t *pos;
}  String_reverse_iterator_wchar;

/* ?_Pdif@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@KAIV?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0@Z */
/* ?_Pdif@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@KA_KV?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0@Z */
size_t __cdecl MSVCP_basic_string_char_Pdif(String_iterator_char i1, String_iterator_char i2)
{
    TRACE("(%p %p) (%p %p)\n", i1.bstr, i1.pos, i2.bstr, i2.pos);

    if((!i1.bstr && i1.pos) || i1.bstr!=i2.bstr) {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return 0;
    }

    return !i1.pos ? 0 : i1.pos-i2.pos;
}

/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0@Z */
/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA?AV?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_erase_iter_range, 24)
String_iterator_char* __thiscall MSVCP_basic_string_char_erase_iter_range(basic_string_char *this,
        String_iterator_char *ret, String_iterator_char beg, String_iterator_char end)
{
    size_t off;

    ret->bstr = this;
    ret->pos = basic_string_char_ptr(this);
    off = MSVCP_basic_string_char_Pdif(beg, *ret);

    MSVCP_basic_string_char_erase(this, off, MSVCP_basic_string_char_Pdif(end, beg));

    ret->bstr = this;
    ret->pos = basic_string_char_ptr(this)+off;
    return ret;
}

/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
/* ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA?AV?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_erase_iter, 16)
String_iterator_char* __thiscall MSVCP_basic_string_char_erase_iter(basic_string_char *this,
        String_iterator_char *ret, String_iterator_char pos)
{
    size_t off;

    ret->bstr = this;
    ret->pos = basic_string_char_ptr(this);
    off = MSVCP_basic_string_char_Pdif(pos, *ret);

    MSVCP_basic_string_char_erase(this, off, 1);

    ret->bstr = this;
    ret->pos = basic_string_char_ptr(this)+off;
    return ret;
}

/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0@Z */
/* ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_assign_iter, 20)
basic_string_char* __thiscall MSVCP_basic_string_char_assign_iter(basic_string_char *this,
        String_iterator_char beg, String_iterator_char end)
{
    return MSVCP_basic_string_char_assign_ptr_ptr(this, beg.pos, end.pos);
}

/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@1@0@Z */
/* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@1@0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_ctor_iter, 20)
basic_string_char* __thiscall MSVCP_basic_string_char_ctor_iter(basic_string_char *this,
        String_iterator_char beg, String_iterator_char end)
{
    return MSVCP_basic_string_char_ctor_cstr_len(this, beg.pos, end.pos-beg.pos);
}

/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0@Z */
/* ?append@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_append_iter, 20)
basic_string_char* __thiscall MSVCP_basic_string_char_append_iter(
        basic_string_char *this, String_iterator_char beg, String_iterator_char end)
{
    return MSVCP_basic_string_char_append_cstr_len(this, beg.pos, end.pos-beg.pos);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@000@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@000@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_replace_iter_iter, 36)
basic_string_char* __thiscall MSVCP_basic_string_char_replace_iter_iter(basic_string_char *this,
        String_iterator_char beg, String_iterator_char end,
        String_iterator_char res_beg, String_iterator_char res_end)
{
    String_iterator_char begin = { this, basic_string_char_ptr(this) };
    return basic_string_char_replace_cstr_len(this,
            MSVCP_basic_string_char_Pdif(beg, begin), MSVCP_basic_string_char_Pdif(end, beg),
            res_beg.pos, MSVCP_basic_string_char_Pdif(res_end, res_beg));
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0ABV12@@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_replace_iter_bstr, 24)
basic_string_char* __thiscall MSVCP_basic_string_char_replace_iter_bstr(basic_string_char *this,
        String_iterator_char beg, String_iterator_char end, const basic_string_char *str)
{
    String_iterator_char begin = { this, basic_string_char_ptr(this) };
    return basic_string_char_replace(this, MSVCP_basic_string_char_Pdif(beg, begin),
            MSVCP_basic_string_char_Pdif(end, beg), str);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0ID@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0_KD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_replace_iter_ch, 28)
basic_string_char* __thiscall MSVCP_basic_string_char_replace_iter_ch(basic_string_char *this,
        String_iterator_char beg, String_iterator_char end, size_t count, char ch)
{
    String_iterator_char begin = { this, basic_string_char_ptr(this) };
    return basic_string_char_replace_ch(this, MSVCP_basic_string_char_Pdif(beg, begin),
            MSVCP_basic_string_char_Pdif(end, beg), count, ch);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0PBD1@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0PEBD1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_replace_iter_ptr_ptr, 28)
basic_string_char* __thiscall MSVCP_basic_string_char_replace_iter_ptr_ptr(basic_string_char *this,
        String_iterator_char beg, String_iterator_char end,
        const char *res_beg, const char *res_end)
{
    String_iterator_char begin = { this, basic_string_char_ptr(this) };
    return basic_string_char_replace_cstr_len(this, MSVCP_basic_string_char_Pdif(beg, begin),
            MSVCP_basic_string_char_Pdif(end, beg), res_beg, res_end-res_beg);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0PBD@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0PEBD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_replace_iter_cstr, 24)
basic_string_char* __thiscall MSVCP_basic_string_char_replace_iter_cstr(basic_string_char *this,
        String_iterator_char beg, String_iterator_char end, const char *str)
{
    String_iterator_char begin = { this, basic_string_char_ptr(this) };
    return basic_string_char_replace_cstr(this, MSVCP_basic_string_char_Pdif(beg, begin),
            MSVCP_basic_string_char_Pdif(end, beg), str);
}

/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0PBDI@Z */
/* ?replace@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAAEAV12@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@0PEBD_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_replace_iter_cstr_len, 28)
basic_string_char* __thiscall MSVCP_basic_string_char_replace_iter_cstr_len(basic_string_char *this,
        String_iterator_char beg, String_iterator_char end, const char *str, size_t len)
{
    String_iterator_char begin = { this, basic_string_char_ptr(this) };
    return basic_string_char_replace_cstr_len(this, MSVCP_basic_string_char_Pdif(beg, begin),
            MSVCP_basic_string_char_Pdif(end, beg), str, len);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXV?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@ID@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXV?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@_KD@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_insert_iter_chn, 20)
void __thiscall MSVCP_basic_string_char_insert_iter_chn(basic_string_char *this,
        String_iterator_char where, size_t count, char ch)
{
    String_iterator_char iter = { this, basic_string_char_ptr(this) };
    size_t off = MSVCP_basic_string_char_Pdif(where, iter);

    basic_string_char_insert_chn(this, off, count, ch);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@D@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA?AV?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@D@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_insert_iter_ch, 20)
String_iterator_char* __thiscall MSVCP_basic_string_char_insert_iter_ch(basic_string_char *this,
        String_iterator_char *ret, String_iterator_char where, char ch)
{
    size_t off;

    ret->bstr = this;
    ret->pos = basic_string_char_ptr(this);
    off = MSVCP_basic_string_char_Pdif(where, *ret);

    basic_string_char_insert_chn(this, off, 1, ch);
    ret->pos = basic_string_char_ptr(this)+off;
    return ret;
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA?AV?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_insert_iter_null, 16)
String_iterator_char* __thiscall MSVCP_basic_string_char_insert_iter_null(basic_string_char *this,
        String_iterator_char *ret, String_iterator_char where)
{
    return MSVCP_basic_string_char_insert_iter_ch(this, ret, where, 0);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXV?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@00@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXV?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@00@Z */
DEFINE_THISCALL_WRAPPER(basic_string_char_insert_iter_beg_end, 28)
void __thiscall basic_string_char_insert_iter_beg_end(basic_string_char *this,
        String_iterator_char where, String_iterator_char beg, String_iterator_char end)
{
    MSVCP_basic_string_char_replace_iter_iter(this, where, where, beg, end);
}

/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXV?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PBD1@Z */
/* ?insert@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAAXV?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@PEBD1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_insert_iter_ptr_ptr, 20)
void __thiscall MSVCP_basic_string_char_insert_iter_ptr_ptr(basic_string_char *this,
        String_iterator_char where, const char *beg, const char *end)
{
    MSVCP_basic_string_char_replace_iter_ptr_ptr(this, where, where, beg, end);
}

/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA?AV?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?begin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_begin, 8)
String_iterator_char* __thiscall MSVCP_basic_string_char_begin(
        basic_string_char *this, String_iterator_char *ret)
{
    TRACE("%p\n", this);

    ret->bstr = this;
    ret->pos = basic_string_char_const_ptr(this);
    return ret;
}

/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA?AV?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
/* ?end@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_end, 8)
String_iterator_char* __thiscall MSVCP_basic_string_char_end(
        basic_string_char *this, String_iterator_char *ret)
{
    TRACE("%p\n", this);

    ret->bstr = this;
    ret->pos = basic_string_char_const_ptr(this)+this->size;
    return ret;
}

/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$reverse_iterator@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$reverse_iterator@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_rbegin, 8)
String_reverse_iterator_char* __thiscall MSVCP_basic_string_char_rbegin(
        basic_string_char *this, String_reverse_iterator_char *ret)
{
    TRACE("%p\n", this);

#if _MSVCP_VER == 80
    ret->cont = NULL;
#endif
    ret->bstr = this;
    ret->pos = basic_string_char_const_ptr(this)+this->size;
    return ret;
}

/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$reverse_iterator@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
/* ?rend@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QEBA?AV?$reverse_iterator@V?$_String_const_iterator@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_char_rend, 8)
String_reverse_iterator_char* __thiscall MSVCP_basic_string_char_rend(
        basic_string_char *this, String_reverse_iterator_char *ret)
{
    TRACE("%p\n", this);

#if _MSVCP_VER == 80
    ret->cont = NULL;
#endif
    ret->bstr = this;
    ret->pos = basic_string_char_const_ptr(this);
    return ret;
}

/* ?_Pdif@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@KAIV?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0@Z */
/* ?_Pdif@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@KA_KV?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0@Z */
/* ?_Pdif@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@KAIV?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0@Z */
/* ?_Pdif@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@KA_KV?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0@Z */
size_t __cdecl MSVCP_basic_string_wchar_Pdif(String_iterator_wchar i1, String_iterator_wchar i2)
{
    TRACE("(%p %p) (%p %p)\n", i1.bstr, i1.pos, i2.bstr, i2.pos);

    if((!i1.bstr && i1.pos) || i1.bstr!=i2.bstr) {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return 0;
    }

    return !i1.pos ? 0 : i1.pos-i2.pos;
}

/* ?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0@Z */
/* ?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA?AV?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_erase_iter_range, 24)
String_iterator_wchar* __thiscall MSVCP_basic_string_wchar_erase_iter_range(basic_string_wchar *this,
        String_iterator_wchar *ret, String_iterator_wchar beg, String_iterator_wchar end)
{
    size_t off;

    ret->bstr = this;
    ret->pos = basic_string_wchar_ptr(this);
    off = MSVCP_basic_string_wchar_Pdif(beg, *ret);

    MSVCP_basic_string_wchar_erase(this, off, MSVCP_basic_string_wchar_Pdif(end, beg));

    ret->bstr = this;
    ret->pos = basic_string_wchar_ptr(this)+off;
    return ret;
}

/* ?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?erase@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA?AV?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
/* ?erase@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_erase_iter, 16)
String_iterator_wchar* __thiscall MSVCP_basic_string_wchar_erase_iter(basic_string_wchar *this,
        String_iterator_wchar *ret, String_iterator_wchar pos)
{
    size_t off;

    ret->bstr = this;
    ret->pos = basic_string_wchar_ptr(this);
    off = MSVCP_basic_string_wchar_Pdif(pos, *ret);

    MSVCP_basic_string_wchar_erase(this, off, 1);

    ret->bstr = this;
    ret->pos = basic_string_wchar_ptr(this)+off;
    return ret;
}

/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0@Z */
/* ?assign@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0@Z */
/* ?assign@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_assign_iter, 20)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_assign_iter(basic_string_wchar *this,
        String_iterator_wchar beg, String_iterator_wchar end)
{
    return MSVCP_basic_string_wchar_assign_ptr_ptr(this, beg.pos, end.pos);
}

/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@0@Z */
/* ??0?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@1@0@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@1@0@Z */
/* ??0?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@1@0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_ctor_iter, 20)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_ctor_iter(basic_string_wchar *this,
        String_iterator_wchar beg, String_iterator_wchar end)
{
    return MSVCP_basic_string_wchar_ctor_cstr_len(this, beg.pos, end.pos-beg.pos);
}

/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0@Z */
/* ?append@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0@Z */
/* ?append@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_append_iter, 20)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_append_iter(
        basic_string_wchar *this, String_iterator_wchar beg, String_iterator_wchar end)
{
    return MSVCP_basic_string_wchar_append_cstr_len(this, beg.pos, end.pos-beg.pos);
}

/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@000@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@000@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@000@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@000@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_replace_iter_iter, 36)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_replace_iter_iter(basic_string_wchar *this,
        String_iterator_wchar beg, String_iterator_wchar end,
                String_iterator_wchar res_beg, String_iterator_wchar res_end)
{
    String_iterator_wchar begin = { this, basic_string_wchar_ptr(this) };
    return basic_string_wchar_replace_cstr_len(this,
            MSVCP_basic_string_wchar_Pdif(beg, begin), MSVCP_basic_string_wchar_Pdif(end, beg),
            res_beg.pos, MSVCP_basic_string_wchar_Pdif(res_end, res_beg));
}

/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0ABV12@@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0AEBV12@@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0ABV12@@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0AEBV12@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_replace_iter_bstr, 24)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_replace_iter_bstr(basic_string_wchar *this,
        String_iterator_wchar beg, String_iterator_wchar end, const basic_string_wchar *str)
{
    String_iterator_wchar begin = { this, basic_string_wchar_ptr(this) };
    return basic_string_wchar_replace(this, MSVCP_basic_string_wchar_Pdif(beg, begin),
            MSVCP_basic_string_wchar_Pdif(end, beg), str);
}

/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0I_W@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0_K_W@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0IG@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0_KG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_replace_iter_ch, 28)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_replace_iter_ch(basic_string_wchar *this,
        String_iterator_wchar beg, String_iterator_wchar end, size_t count, wchar_t ch)
{
    String_iterator_wchar begin = { this, basic_string_wchar_ptr(this) };
    return basic_string_wchar_replace_ch(this, MSVCP_basic_string_wchar_Pdif(beg, begin),
            MSVCP_basic_string_wchar_Pdif(end, beg), count, ch);
}

/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0PB_W1@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0PEB_W1@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0PBG1@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0PEBG1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_replace_iter_ptr_ptr, 28)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_replace_iter_ptr_ptr(basic_string_wchar *this,
        String_iterator_wchar beg, String_iterator_wchar end,
        const wchar_t *res_beg, const wchar_t *res_end)
{
    String_iterator_wchar begin = { this, basic_string_wchar_ptr(this) };
    return basic_string_wchar_replace_cstr_len(this, MSVCP_basic_string_wchar_Pdif(beg, begin),
            MSVCP_basic_string_wchar_Pdif(end, beg), res_beg, res_end-res_beg);
}

/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0PB_W@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0PEB_W@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0PBG@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0PEBG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_replace_iter_cstr, 24)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_replace_iter_cstr(basic_string_wchar *this,
        String_iterator_wchar beg, String_iterator_wchar end, const wchar_t *str)
{
    String_iterator_wchar begin = { this, basic_string_wchar_ptr(this) };
    return basic_string_wchar_replace_cstr(this, MSVCP_basic_string_wchar_Pdif(beg, begin),
            MSVCP_basic_string_wchar_Pdif(end, beg), str);
}

/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEAAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0PB_WI@Z */
/* ?replace@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAAEAV12@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@0PEB_W_K@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0PBGI@Z */
/* ?replace@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAAEAV12@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@0PEBG_K@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_replace_iter_cstr_len, 28)
basic_string_wchar* __thiscall MSVCP_basic_string_wchar_replace_iter_cstr_len(basic_string_wchar *this,
        String_iterator_wchar beg, String_iterator_wchar end, const wchar_t *str, size_t len)
{
    String_iterator_wchar begin = { this, basic_string_wchar_ptr(this) };
    return basic_string_wchar_replace_cstr_len(this, MSVCP_basic_string_wchar_Pdif(beg, begin),
            MSVCP_basic_string_wchar_Pdif(end, beg), str, len);
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXV?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@I_W@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXV?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@_K_W@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXV?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@IG@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXV?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@_KG@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_insert_iter_chn, 20)
void __thiscall MSVCP_basic_string_wchar_insert_iter_chn(basic_string_wchar *this,
        String_iterator_wchar where, size_t count, wchar_t ch)
{
    String_iterator_wchar iter = { this, basic_string_wchar_ptr(this) };
    size_t off = MSVCP_basic_string_wchar_Pdif(where, iter);

    basic_string_wchar_insert_chn(this, off, count, ch);
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@_W@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA?AV?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@_W@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@G@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@G@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_insert_iter_ch, 20)
String_iterator_wchar* __thiscall MSVCP_basic_string_wchar_insert_iter_ch(basic_string_wchar *this,
        String_iterator_wchar *ret, String_iterator_wchar where, wchar_t ch)
{
    size_t off;

    ret->bstr = this;
    ret->pos = basic_string_wchar_ptr(this);
    off = MSVCP_basic_string_wchar_Pdif(where, *ret);

    basic_string_wchar_insert_chn(this, off, 1, ch);
    ret->pos = basic_string_wchar_ptr(this)+off;
    return ret;
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA?AV?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_insert_iter_null, 16)
String_iterator_wchar* __thiscall MSVCP_basic_string_wchar_insert_iter_null(basic_string_wchar *this,
        String_iterator_wchar *ret, String_iterator_wchar where)
{
    return MSVCP_basic_string_wchar_insert_iter_ch(this, ret, where, 0);
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXV?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@00@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXV?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@00@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXV?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@00@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXV?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@00@Z */
DEFINE_THISCALL_WRAPPER(basic_string_wchar_insert_iter_beg_end, 28)
void __thiscall basic_string_wchar_insert_iter_beg_end(basic_string_wchar *this,
        String_iterator_wchar where, String_iterator_wchar beg, String_iterator_wchar end)
{
    MSVCP_basic_string_wchar_replace_iter_iter(this, where, where, beg, end);
}

/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAEXV?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PB_W1@Z */
/* ?insert@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAAXV?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@PEB_W1@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXV?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PBG1@Z */
/* ?insert@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAAXV?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@PEBG1@Z */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_insert_iter_ptr_ptr, 20)
void __thiscall MSVCP_basic_string_wchar_insert_iter_ptr_ptr(basic_string_wchar *this,
        String_iterator_wchar where, const wchar_t *beg, const wchar_t *end)
{
    MSVCP_basic_string_wchar_replace_iter_ptr_ptr(this, where, where, beg, end);
}

/* ?begin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?begin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA?AV?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?begin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?begin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?begin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_begin, 8)
String_iterator_wchar* __thiscall MSVCP_basic_string_wchar_begin(
        basic_string_wchar *this, String_iterator_wchar *ret)
{
    TRACE("%p\n", this);

    ret->bstr = this;
    ret->pos = basic_string_wchar_const_ptr(this);
    return ret;
}

/* ?end@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?end@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA?AV?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?end@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?end@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@2@XZ */
/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
/* ?end@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_end, 8)
String_iterator_wchar* __thiscall MSVCP_basic_string_wchar_end(
        basic_string_wchar *this, String_iterator_wchar *ret)
{
    TRACE("%p\n", this);

    ret->bstr = this;
    ret->pos = basic_string_wchar_const_ptr(this)+this->size;
    return ret;
}

/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$reverse_iterator@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$reverse_iterator@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$reverse_iterator@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rbegin@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$reverse_iterator@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_rbegin, 8)
String_reverse_iterator_wchar* __thiscall MSVCP_basic_string_wchar_rbegin(
        basic_string_wchar *this, String_reverse_iterator_wchar *ret)
{
    TRACE("%p\n", this);

#if _MSVCP_VER == 80
    ret->cont = NULL;
#endif
    ret->bstr = this;
    ret->pos = basic_string_wchar_const_ptr(this)+this->size;
    return ret;
}

/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QBE?AV?$reverse_iterator@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rend@?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@QEBA?AV?$reverse_iterator@V?$_String_const_iterator@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE?AV?$reverse_iterator@V?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEAA?AV?$reverse_iterator@V?$_String_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$reverse_iterator@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
/* ?rend@?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QEBA?AV?$reverse_iterator@V?$_String_const_iterator@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@@2@XZ */
DEFINE_THISCALL_WRAPPER(MSVCP_basic_string_wchar_rend, 8)
String_reverse_iterator_wchar* __thiscall MSVCP_basic_string_wchar_rend(
        basic_string_wchar *this, String_reverse_iterator_wchar *ret)
{
    TRACE("%p\n", this);

#if _MSVCP_VER == 80
    ret->cont = NULL;
#endif
    ret->bstr = this;
    ret->pos = basic_string_wchar_const_ptr(this);
    return ret;
}

#endif  /* _MSVCP_VER < 80 */
