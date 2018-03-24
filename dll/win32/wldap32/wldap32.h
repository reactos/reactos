/*
 * WLDAP32 - LDAP support for Wine
 *
 * Copyright 2005 Hans Leidekker
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

#pragma once

#include "wine/heap.h"
#include "wine/unicode.h"

extern HINSTANCE hwldap32 DECLSPEC_HIDDEN;

ULONG map_error( int ) DECLSPEC_HIDDEN;

/* A set of helper functions to convert LDAP data structures
 * to and from ansi (A), wide character (W) and utf8 (U) encodings.
 */

static inline char *strdupU( const char *src )
{
    char *dst;
    if (!src) return NULL;
    if ((dst = heap_alloc( (strlen( src ) + 1) * sizeof(char) ))) strcpy( dst, src );
    return dst;
}

static inline WCHAR *strdupW( const WCHAR *src )
{
    WCHAR *dst;
    if (!src) return NULL;
    if ((dst = heap_alloc( (strlenW( src ) + 1) * sizeof(WCHAR) ))) strcpyW( dst, src );
    return dst;
}

static inline LPWSTR strAtoW( LPCSTR str )
{
    LPWSTR ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = heap_alloc( len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

static inline LPSTR strWtoA( LPCWSTR str )
{
    LPSTR ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = heap_alloc( len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline char *strWtoU( LPCWSTR str )
{
    LPSTR ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = heap_alloc( len )))
            WideCharToMultiByte( CP_UTF8, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline LPWSTR strUtoW( char *str )
{
    LPWSTR ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_UTF8, 0, str, -1, NULL, 0 );
        if ((ret = heap_alloc( len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_UTF8, 0, str, -1, ret, len );
    }
    return ret;
}

static inline void strfreeA( LPSTR str )
{
    heap_free( str );
}

static inline void strfreeW( LPWSTR str )
{
    heap_free( str );
}

static inline void strfreeU( char *str )
{
    heap_free( str );
}

static inline DWORD strarraylenA( LPSTR *strarray )
{
    LPSTR *p = strarray;
    while (*p) p++;
    return p - strarray;
}

static inline DWORD strarraylenW( LPWSTR *strarray )
{
    LPWSTR *p = strarray;
    while (*p) p++;
    return p - strarray;
}

static inline DWORD strarraylenU( char **strarray )
{
    char **p = strarray;
    while (*p) p++;
    return p - strarray;
}

static inline LPWSTR *strarrayAtoW( LPSTR *strarray )
{
    LPWSTR *strarrayW = NULL;
    DWORD size;

    if (strarray)
    {
        size  = sizeof(WCHAR*) * (strarraylenA( strarray ) + 1);
        if ((strarrayW = heap_alloc( size )))
        {
            LPSTR *p = strarray;
            LPWSTR *q = strarrayW;

            while (*p) *q++ = strAtoW( *p++ );
            *q = NULL;
        }
    }
    return strarrayW;
}

static inline LPSTR *strarrayWtoA( LPWSTR *strarray )
{
    LPSTR *strarrayA = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(LPSTR) * (strarraylenW( strarray ) + 1);
        if ((strarrayA = heap_alloc( size )))
        {
            LPWSTR *p = strarray;
            LPSTR *q = strarrayA;

            while (*p) *q++ = strWtoA( *p++ );
            *q = NULL;
        }
    }
    return strarrayA;
}

static inline char **strarrayWtoU( LPWSTR *strarray )
{
    char **strarrayU = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(char*) * (strarraylenW( strarray ) + 1);
        if ((strarrayU = heap_alloc( size )))
        {
            LPWSTR *p = strarray;
            char **q = strarrayU;

            while (*p) *q++ = strWtoU( *p++ );
            *q = NULL;
        }
    }
    return strarrayU;
}

static inline LPWSTR *strarrayUtoW( char **strarray )
{
    LPWSTR *strarrayW = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(WCHAR*) * (strarraylenU( strarray ) + 1);
        if ((strarrayW = heap_alloc( size )))
        {
            char **p = strarray;
            LPWSTR *q = strarrayW;

            while (*p) *q++ = strUtoW( *p++ );
            *q = NULL;
        }
    }
    return strarrayW;
}

static inline void strarrayfreeA( LPSTR *strarray )
{
    if (strarray)
    {
        LPSTR *p = strarray;
        while (*p) strfreeA( *p++ );
        heap_free( strarray );
    }
}

static inline void strarrayfreeW( LPWSTR *strarray )
{
    if (strarray)
    {
        LPWSTR *p = strarray;
        while (*p) strfreeW( *p++ );
        heap_free( strarray );
    }
}

static inline void strarrayfreeU( char **strarray )
{
    if (strarray)
    {
        char **p = strarray;
        while (*p) strfreeU( *p++ );
        heap_free( strarray );
    }
}

#ifdef HAVE_LDAP

static inline struct berval *bvdup( struct berval *bv )
{
    struct berval *berval;
    DWORD size = sizeof(struct berval) + bv->bv_len;

    if ((berval = heap_alloc( size )))
    {
        char *val = (char *)berval + sizeof(struct berval);

        berval->bv_len = bv->bv_len;
        berval->bv_val = val;
        memcpy( val, bv->bv_val, bv->bv_len );
    }
    return berval;
}

static inline DWORD bvarraylen( struct berval **bv )
{
    struct berval **p = bv;
    while (*p) p++;
    return p - bv;
}

static inline struct berval **bvarraydup( struct berval **bv )
{
    struct berval **berval = NULL;
    DWORD size;

    if (bv)
    {
        size = sizeof(struct berval *) * (bvarraylen( bv ) + 1);
        if ((berval = heap_alloc( size )))
        {
            struct berval **p = bv;
            struct berval **q = berval;

            while (*p) *q++ = bvdup( *p++ );
            *q = NULL;
        }
    }
    return berval;
}

static inline void bvarrayfree( struct berval **bv )
{
    struct berval **p = bv;
    while (*p) heap_free( *p++ );
    heap_free( bv );
}

static inline LDAPModW *modAtoW( LDAPModA *mod )
{
    LDAPModW *modW;

    if ((modW = heap_alloc( sizeof(LDAPModW) )))
    {
        modW->mod_op = mod->mod_op;
        modW->mod_type = strAtoW( mod->mod_type );

        if (mod->mod_op & LDAP_MOD_BVALUES)
            modW->mod_vals.modv_bvals = bvarraydup( mod->mod_vals.modv_bvals );
        else
            modW->mod_vals.modv_strvals = strarrayAtoW( mod->mod_vals.modv_strvals );
    }
    return modW;
}

static inline LDAPMod *modWtoU( LDAPModW *mod )
{
    LDAPMod *modU;

    if ((modU = heap_alloc( sizeof(LDAPMod) )))
    {
        modU->mod_op = mod->mod_op;
        modU->mod_type = strWtoU( mod->mod_type );

        if (mod->mod_op & LDAP_MOD_BVALUES)
            modU->mod_vals.modv_bvals = bvarraydup( mod->mod_vals.modv_bvals );
        else
            modU->mod_vals.modv_strvals = strarrayWtoU( mod->mod_vals.modv_strvals );
    }
    return modU;
}

static inline void modfreeW( LDAPModW *mod )
{
    if (mod->mod_op & LDAP_MOD_BVALUES)
        bvarrayfree( mod->mod_vals.modv_bvals );
    else
        strarrayfreeW( mod->mod_vals.modv_strvals );
    heap_free( mod );
}

static inline void modfreeU( LDAPMod *mod )
{
    if (mod->mod_op & LDAP_MOD_BVALUES)
        bvarrayfree( mod->mod_vals.modv_bvals );
    else
        strarrayfreeU( mod->mod_vals.modv_strvals );
    heap_free( mod );
}

static inline DWORD modarraylenA( LDAPModA **modarray )
{
    LDAPModA **p = modarray;
    while (*p) p++;
    return p - modarray;
}

static inline DWORD modarraylenW( LDAPModW **modarray )
{
    LDAPModW **p = modarray;
    while (*p) p++;
    return p - modarray;
}

static inline LDAPModW **modarrayAtoW( LDAPModA **modarray )
{
    LDAPModW **modarrayW = NULL;
    DWORD size;

    if (modarray)
    {
        size = sizeof(LDAPModW*) * (modarraylenA( modarray ) + 1);
        if ((modarrayW = heap_alloc( size )))
        {
            LDAPModA **p = modarray;
            LDAPModW **q = modarrayW;

            while (*p) *q++ = modAtoW( *p++ );
            *q = NULL;
        }
    }
    return modarrayW;
}

static inline LDAPMod **modarrayWtoU( LDAPModW **modarray )
{
    LDAPMod **modarrayU = NULL;
    DWORD size;

    if (modarray)
    {
        size = sizeof(LDAPMod*) * (modarraylenW( modarray ) + 1);
        if ((modarrayU = heap_alloc( size )))
        {
            LDAPModW **p = modarray;
            LDAPMod **q = modarrayU;

            while (*p) *q++ = modWtoU( *p++ );
            *q = NULL;
        }
    }
    return modarrayU;
}

static inline void modarrayfreeW( LDAPModW **modarray )
{
    if (modarray)
    {
        LDAPModW **p = modarray;
        while (*p) modfreeW( *p++ );
        heap_free( modarray );
    }
}

static inline void modarrayfreeU( LDAPMod **modarray )
{
    if (modarray)
    {
        LDAPMod **p = modarray;
        while (*p) modfreeU( *p++ );
        heap_free( modarray );
    }
}

static inline LDAPControlW *controlAtoW( LDAPControlA *control )
{
    LDAPControlW *controlW;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = heap_alloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(controlW = heap_alloc( sizeof(LDAPControlW) )))
    {
        heap_free( val );
        return NULL;
    }

    controlW->ldctl_oid = strAtoW( control->ldctl_oid );
    controlW->ldctl_value.bv_len = len; 
    controlW->ldctl_value.bv_val = val; 
    controlW->ldctl_iscritical = control->ldctl_iscritical;

    return controlW;
}

static inline LDAPControlA *controlWtoA( LDAPControlW *control )
{
    LDAPControlA *controlA;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = heap_alloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(controlA = heap_alloc( sizeof(LDAPControlA) )))
    {
        heap_free( val );
        return NULL;
    }

    controlA->ldctl_oid = strWtoA( control->ldctl_oid );
    controlA->ldctl_value.bv_len = len; 
    controlA->ldctl_value.bv_val = val;
    controlA->ldctl_iscritical = control->ldctl_iscritical;

    return controlA;
}

static inline LDAPControl *controlWtoU( LDAPControlW *control )
{
    LDAPControl *controlU;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = heap_alloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(controlU = heap_alloc( sizeof(LDAPControl) )))
    {
        heap_free( val );
        return NULL;
    }

    controlU->ldctl_oid = strWtoU( control->ldctl_oid );
    controlU->ldctl_value.bv_len = len; 
    controlU->ldctl_value.bv_val = val; 
    controlU->ldctl_iscritical = control->ldctl_iscritical;

    return controlU;
}

static inline LDAPControlW *controlUtoW( LDAPControl *control )
{
    LDAPControlW *controlW;
    DWORD len = control->ldctl_value.bv_len;
    char *val = NULL;

    if (control->ldctl_value.bv_val)
    {
        if (!(val = heap_alloc( len ))) return NULL;
        memcpy( val, control->ldctl_value.bv_val, len );
    }

    if (!(controlW = heap_alloc( sizeof(LDAPControlW) )))
    {
        heap_free( val );
        return NULL;
    }

    controlW->ldctl_oid = strUtoW( control->ldctl_oid );
    controlW->ldctl_value.bv_len = len; 
    controlW->ldctl_value.bv_val = val; 
    controlW->ldctl_iscritical = control->ldctl_iscritical;

    return controlW;
}

static inline void controlfreeA( LDAPControlA *control )
{
    if (control)
    {
        strfreeA( control->ldctl_oid );
        heap_free( control->ldctl_value.bv_val );
        heap_free( control );
    }
}

static inline void controlfreeW( LDAPControlW *control )
{
    if (control)
    {
        strfreeW( control->ldctl_oid );
        heap_free( control->ldctl_value.bv_val );
        heap_free( control );
    }
}

static inline void controlfreeU( LDAPControl *control )
{
    if (control)
    {
        strfreeU( control->ldctl_oid );
        heap_free( control->ldctl_value.bv_val );
        heap_free( control );
    }
}

static inline DWORD controlarraylenA( LDAPControlA **controlarray )
{
    LDAPControlA **p = controlarray;
    while (*p) p++;
    return p - controlarray;
}

static inline DWORD controlarraylenW( LDAPControlW **controlarray )
{
    LDAPControlW **p = controlarray;
    while (*p) p++;
    return p - controlarray;
}

static inline DWORD controlarraylenU( LDAPControl **controlarray )
{
    LDAPControl **p = controlarray;
    while (*p) p++;
    return p - controlarray;
}

static inline LDAPControlW **controlarrayAtoW( LDAPControlA **controlarray )
{
    LDAPControlW **controlarrayW = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(LDAPControlW*) * (controlarraylenA( controlarray ) + 1);
        if ((controlarrayW = heap_alloc( size )))
        {
            LDAPControlA **p = controlarray;
            LDAPControlW **q = controlarrayW;

            while (*p) *q++ = controlAtoW( *p++ );
            *q = NULL;
        }
    }
    return controlarrayW;
}

static inline LDAPControlA **controlarrayWtoA( LDAPControlW **controlarray )
{
    LDAPControlA **controlarrayA = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(LDAPControl*) * (controlarraylenW( controlarray ) + 1);
        if ((controlarrayA = heap_alloc( size )))
        {
            LDAPControlW **p = controlarray;
            LDAPControlA **q = controlarrayA;

            while (*p) *q++ = controlWtoA( *p++ );
            *q = NULL;
        }
    }
    return controlarrayA;
}

static inline LDAPControl **controlarrayWtoU( LDAPControlW **controlarray )
{
    LDAPControl **controlarrayU = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(LDAPControl*) * (controlarraylenW( controlarray ) + 1);
        if ((controlarrayU = heap_alloc( size )))
        {
            LDAPControlW **p = controlarray;
            LDAPControl **q = controlarrayU;

            while (*p) *q++ = controlWtoU( *p++ );
            *q = NULL;
        }
    }
    return controlarrayU;
}

static inline LDAPControlW **controlarrayUtoW( LDAPControl **controlarray )
{
    LDAPControlW **controlarrayW = NULL;
    DWORD size;

    if (controlarray)
    {
        size = sizeof(LDAPControlW*) * (controlarraylenU( controlarray ) + 1);
        if ((controlarrayW = heap_alloc( size )))
        {
            LDAPControl **p = controlarray;
            LDAPControlW **q = controlarrayW;

            while (*p) *q++ = controlUtoW( *p++ );
            *q = NULL;
        }
    }
    return controlarrayW;
}

static inline void controlarrayfreeA( LDAPControlA **controlarray )
{
    if (controlarray)
    {
        LDAPControlA **p = controlarray;
        while (*p) controlfreeA( *p++ );
        heap_free( controlarray );
    }
}

static inline void controlarrayfreeW( LDAPControlW **controlarray )
{
    if (controlarray)
    {
        LDAPControlW **p = controlarray;
        while (*p) controlfreeW( *p++ );
        heap_free( controlarray );
    }
}

static inline void controlarrayfreeU( LDAPControl **controlarray )
{
    if (controlarray)
    {
        LDAPControl **p = controlarray;
        while (*p) controlfreeU( *p++ );
        heap_free( controlarray );
    }
}

static inline LDAPSortKeyW *sortkeyAtoW( LDAPSortKeyA *sortkey )
{
    LDAPSortKeyW *sortkeyW;

    if ((sortkeyW = heap_alloc( sizeof(LDAPSortKeyW) )))
    {
        sortkeyW->sk_attrtype = strAtoW( sortkey->sk_attrtype );
        sortkeyW->sk_matchruleoid = strAtoW( sortkey->sk_matchruleoid );
        sortkeyW->sk_reverseorder = sortkey->sk_reverseorder;
    }
    return sortkeyW;
}

static inline LDAPSortKeyA *sortkeyWtoA( LDAPSortKeyW *sortkey )
{
    LDAPSortKeyA *sortkeyA;

    if ((sortkeyA = heap_alloc( sizeof(LDAPSortKeyA) )))
    {
        sortkeyA->sk_attrtype = strWtoA( sortkey->sk_attrtype );
        sortkeyA->sk_matchruleoid = strWtoA( sortkey->sk_matchruleoid );
        sortkeyA->sk_reverseorder = sortkey->sk_reverseorder;
    }
    return sortkeyA;
}

static inline LDAPSortKey *sortkeyWtoU( LDAPSortKeyW *sortkey )
{
    LDAPSortKey *sortkeyU;

    if ((sortkeyU = heap_alloc( sizeof(LDAPSortKey) )))
    {
        sortkeyU->attributeType = strWtoU( sortkey->sk_attrtype );
        sortkeyU->orderingRule = strWtoU( sortkey->sk_matchruleoid );
        sortkeyU->reverseOrder = sortkey->sk_reverseorder;
    }
    return sortkeyU;
}

static inline void sortkeyfreeA( LDAPSortKeyA *sortkey )
{
    if (sortkey)
    {
        strfreeA( sortkey->sk_attrtype );
        strfreeA( sortkey->sk_matchruleoid );
        heap_free( sortkey );
    }
}

static inline void sortkeyfreeW( LDAPSortKeyW *sortkey )
{
    if (sortkey)
    {
        strfreeW( sortkey->sk_attrtype );
        strfreeW( sortkey->sk_matchruleoid );
        heap_free( sortkey );
    }
}

static inline void sortkeyfreeU( LDAPSortKey *sortkey )
{
    if (sortkey)
    {
        strfreeU( sortkey->attributeType );
        strfreeU( sortkey->orderingRule );
        heap_free( sortkey );
    }
}

static inline DWORD sortkeyarraylenA( LDAPSortKeyA **sortkeyarray )
{
    LDAPSortKeyA **p = sortkeyarray;
    while (*p) p++;
    return p - sortkeyarray;
}

static inline DWORD sortkeyarraylenW( LDAPSortKeyW **sortkeyarray )
{
    LDAPSortKeyW **p = sortkeyarray;
    while (*p) p++;
    return p - sortkeyarray;
}

static inline LDAPSortKeyW **sortkeyarrayAtoW( LDAPSortKeyA **sortkeyarray )
{
    LDAPSortKeyW **sortkeyarrayW = NULL;
    DWORD size;

    if (sortkeyarray)
    {
        size = sizeof(LDAPSortKeyW*) * (sortkeyarraylenA( sortkeyarray ) + 1);
        if ((sortkeyarrayW = heap_alloc( size )))
        {
            LDAPSortKeyA **p = sortkeyarray;
            LDAPSortKeyW **q = sortkeyarrayW;

            while (*p) *q++ = sortkeyAtoW( *p++ );
            *q = NULL;
        }
    }
    return sortkeyarrayW;
}

static inline LDAPSortKey **sortkeyarrayWtoU( LDAPSortKeyW **sortkeyarray )
{
    LDAPSortKey **sortkeyarrayU = NULL;
    DWORD size;

    if (sortkeyarray)
    {
        size = sizeof(LDAPSortKey*) * (sortkeyarraylenW( sortkeyarray ) + 1);
        if ((sortkeyarrayU = heap_alloc( size )))
        {
            LDAPSortKeyW **p = sortkeyarray;
            LDAPSortKey **q = sortkeyarrayU;

            while (*p) *q++ = sortkeyWtoU( *p++ );
            *q = NULL;
        }
    }
    return sortkeyarrayU;
}

static inline void sortkeyarrayfreeW( LDAPSortKeyW **sortkeyarray )
{
    if (sortkeyarray)
    {
        LDAPSortKeyW **p = sortkeyarray;
        while (*p) sortkeyfreeW( *p++ );
        heap_free( sortkeyarray );
    }
}

static inline void sortkeyarrayfreeU( LDAPSortKey **sortkeyarray )
{
    if (sortkeyarray)
    {
        LDAPSortKey **p = sortkeyarray;
        while (*p) sortkeyfreeU( *p++ );
        heap_free( sortkeyarray );
    }
}
#endif /* HAVE_LDAP */
