/*
 * Locale functions
 *
 * Copyright 2004, 2019 Alexandre Julliard
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
#include <string.h>
#include <stdlib.h>

#ifdef __REACTOS__
#include <rtl_vista.h>
#define NDEBUG
#define wcsicmp _wcsicmp
#define GetCurrentProcess() NtCurrentProcess()
#else
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "ntdll_misc.h"
#endif
#include "locale_private.h"
#include "wine/debug.h"

#ifdef __REACTOS__
NTSTATUS WINAPI RtlLcidToLocaleName( LCID lcid, UNICODE_STRING *str, ULONG flags, BOOLEAN alloc );
NTSYSAPI NTSTATUS  WINAPI RtlQueryActivationContextApplicationSettingsLibSupp(DWORD,HANDLE,const WCHAR*,const WCHAR*,WCHAR*,SIZE_T,SIZE_T*);
NTSYSAPI NTSTATUS  WINAPI RtlNormalizeString(ULONG,const WCHAR*,INT,WCHAR*,INT*);
NTSYSAPI NTSTATUS  WINAPI RtlIdnToNameprepUnicode(DWORD,const WCHAR*,INT,WCHAR*,INT*);
NTSYSAPI NTSTATUS  WINAPI RtlGetLocaleFileMappingAddress(void**,LCID*,LARGE_INTEGER*);

/* Avoid linking to actctx.c to prevent linker errors */
#define RtlQueryActivationContextApplicationSettings RtlQueryActivationContextApplicationSettingsLibSupp

#undef TRACE
#undef FIXME
#undef ERR
#define TRACE(...)
#define FIXME(...)
#define ERR(...)
#endif
WINE_DEFAULT_DEBUG_CHANNEL(nls);

UINT NlsAnsiCodePage = 0;
BYTE NlsMbCodePageTag = 0;
BYTE NlsMbOemCodePageTag = 0;

static LCID user_resource_lcid;
static LCID user_resource_neutral_lcid;
static LCID system_lcid;
static NLSTABLEINFO nls_info = { { CP_UTF8 }, { CP_UTF8 } };
static struct norm_table *norm_tables[16];
static const NLS_LOCALE_HEADER *locale_table;
static const WCHAR *locale_strings;


static WCHAR casemap( USHORT *table, WCHAR ch )
{
    return ch + table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0x0f)];
}


static NTSTATUS load_norm_table( ULONG form, const struct norm_table **info )
{
    unsigned int i;
    USHORT *data, *tables;
    SIZE_T size;
    NTSTATUS status;

    if (!form) return STATUS_INVALID_PARAMETER;
    if (form >= ARRAY_SIZE(norm_tables)) return STATUS_OBJECT_NAME_NOT_FOUND;

    if (!norm_tables[form])
    {
        if ((status = NtGetNlsSectionPtr( NLS_SECTION_NORMALIZE, form, NULL, (void **)&data, &size )))
            return status;

        /* sanity checks */

        if (size <= 0x44) goto invalid;
        if (data[0x14] != form) goto invalid;
        tables = data + 0x1a;
        for (i = 0; i < 8; i++)
        {
            if (tables[i] > size / sizeof(USHORT)) goto invalid;
            if (i && tables[i] < tables[i-1]) goto invalid;
        }

        if (InterlockedCompareExchangePointer( (void **)&norm_tables[form], data, NULL ))
            NtUnmapViewOfSection( GetCurrentProcess(), data );
    }
    *info = norm_tables[form];
    return STATUS_SUCCESS;

invalid:
    NtUnmapViewOfSection( GetCurrentProcess(), data );
    return STATUS_INVALID_PARAMETER;
}

#ifndef __REACTOS__
static PEB64 *get_peb64( void )
{
    TEB64 *teb64 = NtCurrentTeb64();

    if (!teb64) return NULL;
    return (PEB64 *)(UINT_PTR)teb64->Peb;
}
#endif

void locale_init(void)
{
    const NLS_LOCALE_LCID_INDEX *entry;
    USHORT utf8[2] = { 0, CP_UTF8 };
    WCHAR locale[LOCALE_NAME_MAX_LENGTH];
    LARGE_INTEGER unused;
    SIZE_T size;
    UINT ansi_cp = 1252, oem_cp = 437;
    void *ansi_ptr = utf8, *oem_ptr = utf8, *case_ptr;
    NTSTATUS status;
    const struct locale_nls_header *header;
#ifndef __REACTOS__
   PEB64 *peb64 = get_peb64();
#endif

    status = RtlGetLocaleFileMappingAddress( (void **)&header, &system_lcid, &unused );
    if (status)
    {
        ERR( "locale init failed %lx\n", status );
        return;
    }
    locale_table = (const NLS_LOCALE_HEADER *)((char *)header + header->locales);
    locale_strings = (const WCHAR *)((char *)locale_table + locale_table->strings_offset);

    entry = find_lcid_entry( locale_table, system_lcid );
    ansi_cp = get_locale_data( locale_table, entry->idx )->idefaultansicodepage;
    oem_cp = get_locale_data( locale_table, entry->idx )->idefaultcodepage;

    NtQueryDefaultLocale( TRUE, &user_resource_lcid );
    user_resource_neutral_lcid = PRIMARYLANGID( user_resource_lcid );
#ifndef __REACTOS__
    if (user_resource_lcid == LOCALE_CUSTOM_UNSPECIFIED)
    {
        const NLS_LOCALE_LCNAME_INDEX *entry;
        const WCHAR *parent;
        WCHAR bufferW[LOCALE_NAME_MAX_LENGTH];
        SIZE_T len;

        if (!RtlQueryEnvironmentVariable( NULL, L"WINEUSERLOCALE", 14, bufferW, ARRAY_SIZE(bufferW), &len )
            && (entry = find_lcname_entry( locale_table, bufferW )))
        {
            user_resource_lcid = get_locale_data( locale_table, entry->idx )->unique_lcid;
            parent = locale_strings + get_locale_data( locale_table, entry->idx )->sparent;
            if (*parent && (entry = find_lcname_entry( locale_table, parent + 1 )))
                user_resource_neutral_lcid = get_locale_data( locale_table, entry->idx )->unique_lcid;
        }
    }
#endif

    TRACE( "resources: %04lx/%04lx/%04lx\n", user_resource_lcid, user_resource_neutral_lcid, system_lcid );

    if (!RtlQueryActivationContextApplicationSettings( 0, NULL, L"http://schemas.microsoft.com/SMI/2019/WindowsSettings",
                                                       L"activeCodePage", locale, ARRAY_SIZE(locale), NULL ))
    {
        const NLS_LOCALE_LCNAME_INDEX *entry;

        if (!wcsicmp( locale, L"utf-8" ))
        {
            ansi_cp = oem_cp = CP_UTF8;
        }
        else if (!wcsicmp( locale, L"legacy" ))
        {
            if (ansi_cp == CP_UTF8) ansi_cp = 1252;
            if (oem_cp == CP_UTF8) oem_cp = 437;
        }
        else if ((entry = find_lcname_entry( locale_table, locale )))
        {
            ansi_cp = get_locale_data( locale_table, entry->idx )->idefaultansicodepage;
            oem_cp = get_locale_data( locale_table, entry->idx )->idefaultcodepage;
        }
    }

    NtGetNlsSectionPtr( 10, 0, NULL, &case_ptr, &size );
#ifdef __REACTOS__
    NtCurrentPeb()->UnicodeCaseTableData = case_ptr;
    if (ansi_cp != CP_UTF8)
    {
        NtGetNlsSectionPtr( 11, ansi_cp, NULL, &ansi_ptr, &size );
        NtCurrentPeb()->AnsiCodePageData = ansi_ptr;
    }
     if (oem_cp != CP_UTF8)
    {
        NtGetNlsSectionPtr( 11, oem_cp, NULL, &oem_ptr, &size );
        NtCurrentPeb()->OemCodePageData = oem_ptr;
    }
    RtlInitNlsTables( ansi_ptr, oem_ptr, case_ptr, &nls_info );
    /* TODO: Is any Wow64 handling needed? */
#else
    NtCurrentTeb()->Peb->UnicodeCaseTableData = case_ptr;
    if (peb64) peb64->UnicodeCaseTableData = PtrToUlong( case_ptr );
    if (ansi_cp != CP_UTF8)
    {
        NtGetNlsSectionPtr( 11, ansi_cp, NULL, &ansi_ptr, &size );
        NtCurrentTeb()->Peb->AnsiCodePageData = ansi_ptr;
        if (peb64) peb64->AnsiCodePageData = PtrToUlong( ansi_ptr );
    }
    if (oem_cp != CP_UTF8)
    {
        NtGetNlsSectionPtr( 11, oem_cp, NULL, &oem_ptr, &size );
        NtCurrentTeb()->Peb->OemCodePageData = oem_ptr;
        if (peb64) peb64->OemCodePageData = PtrToUlong( oem_ptr );
    }
    RtlInitNlsTables( ansi_ptr, oem_ptr, case_ptr, &nls_info );
#endif
    NlsAnsiCodePage     = nls_info.AnsiTableInfo.CodePage;
    NlsMbCodePageTag    = nls_info.AnsiTableInfo.DBCSCodePage;
    NlsMbOemCodePageTag = nls_info.OemTableInfo.DBCSCodePage;
}


/* return LCIDs to use for resource lookup */
void get_resource_lcids( LANGID *user, LANGID *user_neutral, LANGID *system )
{
    *user = LANGIDFROMLCID( user_resource_lcid );
    *user_neutral = LANGIDFROMLCID( user_resource_neutral_lcid );
    *system = LANGIDFROMLCID( system_lcid );
}


static NTSTATUS get_dummy_preferred_ui_language( DWORD flags, LANGID lang, ULONG *count,
                                                 WCHAR *buffer, ULONG *size )
{
    WCHAR name[LOCALE_NAME_MAX_LENGTH + 2];
    NTSTATUS status;
    ULONG len;

    FIXME("(0x%lx %#x %p %p %p) returning a dummy value (current locale)\n", flags, lang, count, buffer, size);

    if (flags & MUI_LANGUAGE_ID) swprintf( name, ARRAY_SIZE(name), L"%04lX", lang );
    else
    {
        UNICODE_STRING str;

        if (lang == LOCALE_CUSTOM_UNSPECIFIED)
            NtQueryInstallUILanguage( &lang );

        str.Buffer = name;
        str.MaximumLength = sizeof(name);
        status = RtlLcidToLocaleName( lang, &str, 0, FALSE );
        if (status) return status;
    }

    len = wcslen( name ) + 2;
    name[len - 1] = 0;
    if (buffer)
    {
        if (len > *size)
        {
            *size = len;
            return STATUS_BUFFER_TOO_SMALL;
        }
        memcpy( buffer, name, len * sizeof(WCHAR) );
    }
    *size = len;
    *count = 1;
    TRACE("returned variable content: %ld, \"%s\", %ld\n", *count, debugstr_w(buffer), *size);
    return STATUS_SUCCESS;

}

/**************************************************************************
 *      RtlGetProcessPreferredUILanguages   (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetProcessPreferredUILanguages( DWORD flags, ULONG *count, WCHAR *buffer, ULONG *size )
{
    LANGID ui_language;

    FIXME( "%08lx, %p, %p %p\n", flags, count, buffer, size );

    NtQueryDefaultUILanguage( &ui_language );
    return get_dummy_preferred_ui_language( flags, ui_language, count, buffer, size );
}


/**************************************************************************
 *      RtlGetSystemPreferredUILanguages   (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetSystemPreferredUILanguages( DWORD flags, ULONG unknown, ULONG *count,
                                                  WCHAR *buffer, ULONG *size )
{
    LANGID ui_language;

    if (flags & ~(MUI_LANGUAGE_NAME | MUI_LANGUAGE_ID | MUI_MACHINE_LANGUAGE_SETTINGS)) return STATUS_INVALID_PARAMETER;
    if ((flags & MUI_LANGUAGE_NAME) && (flags & MUI_LANGUAGE_ID)) return STATUS_INVALID_PARAMETER;
    if (*size && !buffer) return STATUS_INVALID_PARAMETER;

    NtQueryInstallUILanguage( &ui_language );
    return get_dummy_preferred_ui_language( flags, ui_language, count, buffer, size );
}


/**************************************************************************
 *      RtlGetThreadPreferredUILanguages   (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetThreadPreferredUILanguages( DWORD flags, ULONG *count, WCHAR *buffer, ULONG *size )
{
    LANGID ui_language;

    FIXME( "%08lx, %p, %p %p\n", flags, count, buffer, size );

    NtQueryDefaultUILanguage( &ui_language );
    return get_dummy_preferred_ui_language( flags, ui_language, count, buffer, size );
}


/**************************************************************************
 *      RtlGetUserPreferredUILanguages   (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetUserPreferredUILanguages( DWORD flags, ULONG unknown, ULONG *count,
                                                WCHAR *buffer, ULONG *size )
{
    LANGID ui_language;

    if (flags & ~(MUI_LANGUAGE_NAME | MUI_LANGUAGE_ID)) return STATUS_INVALID_PARAMETER;
    if ((flags & MUI_LANGUAGE_NAME) && (flags & MUI_LANGUAGE_ID)) return STATUS_INVALID_PARAMETER;
    if (*size && !buffer) return STATUS_INVALID_PARAMETER;

    NtQueryDefaultUILanguage( &ui_language );
    return get_dummy_preferred_ui_language( flags, ui_language, count, buffer, size );
}


/**************************************************************************
 *      RtlSetProcessPreferredUILanguages   (NTDLL.@)
 */
NTSTATUS WINAPI RtlSetProcessPreferredUILanguages( DWORD flags, PCZZWSTR buffer, ULONG *count )
{
    FIXME( "%lu, %p, %p\n", flags, buffer, count );
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlSetThreadPreferredUILanguages   (NTDLL.@)
 */
NTSTATUS WINAPI RtlSetThreadPreferredUILanguages( DWORD flags, PCZZWSTR buffer, ULONG *count )
{
    FIXME( "%lu, %p, %p\n", flags, buffer, count );
    return STATUS_SUCCESS;
}


/******************************************************************
 *      RtlInitCodePageTable   (NTDLL.@)
 */
void WINAPI RtlInitCodePageTable( USHORT *ptr, CPTABLEINFO *info )
{
    static const CPTABLEINFO utf8_cpinfo = { CP_UTF8, 4, '?', 0xfffd, '?', '?' };

    if (ptr[1] == CP_UTF8) *info = utf8_cpinfo;
    else init_codepage_table( ptr, info );
}


/**************************************************************************
 *      RtlInitNlsTables   (NTDLL.@)
 */
void WINAPI RtlInitNlsTables( USHORT *ansi, USHORT *oem, USHORT *casetable, NLSTABLEINFO *info )
{
    RtlInitCodePageTable( ansi, &info->AnsiTableInfo );
    RtlInitCodePageTable( oem, &info->OemTableInfo );
    info->UpperCaseTable = casetable + 2;
    info->LowerCaseTable = casetable + casetable[1] + 2;
}


/**************************************************************************
 *      RtlResetRtlTranslations   (NTDLL.@)
 */
#ifndef __REACTOS__
void WINAPI RtlResetRtlTranslations( const NLSTABLEINFO *info )
#else
void WINAPI RtlResetRtlTranslations( NLSTABLEINFO *info )
#endif
{
    NlsAnsiCodePage     = info->AnsiTableInfo.CodePage;
    NlsMbCodePageTag    = info->AnsiTableInfo.DBCSCodePage;
    NlsMbOemCodePageTag = info->OemTableInfo.DBCSCodePage;
    nls_info = *info;
}


/**************************************************************************
 *      RtlGetLocaleFileMappingAddress   (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetLocaleFileMappingAddress( void **ptr, LCID *lcid, LARGE_INTEGER *size )
{
    static void *cached_ptr;
    static LCID cached_lcid;

    if (!cached_ptr)
    {
        void *addr;
#ifdef __REACTOS__
        NTSTATUS status = NtInitializeNlsFiles( &addr, &cached_lcid, size, NULL );
#else
        NTSTATUS status = NtInitializeNlsFiles( &addr, &cached_lcid, size );
#endif

        if (status) return status;
        if (InterlockedCompareExchangePointer( &cached_ptr, addr, NULL ))
            NtUnmapViewOfSection( GetCurrentProcess(), addr );
    }
    *ptr = cached_ptr;
    *lcid = cached_lcid;
    return STATUS_SUCCESS;
}

/**************************************************************************
 *      RtlAnsiCharToUnicodeChar   (NTDLL.@)
 */
#ifdef __REACTOS__
WCHAR NTAPI RtlAnsiCharToUnicodeChar(IN OUT PUCHAR *ansi)
#else
WCHAR WINAPI RtlAnsiCharToUnicodeChar( char **ansi )
#endif
{
    unsigned char ch = *(*ansi)++;

    if (nls_info.AnsiTableInfo.CodePage == CP_UTF8)
    {
        unsigned int res;

        if (ch < 0x80) return ch;
#ifdef __REACTOS__
        if ((res = decode_utf8_char( ch, (const char **)ansi, (const char*)(*ansi + 3) )) > 0x10ffff) res = 0xfffd;
#else
        if ((res = decode_utf8_char( ch, (const char **)ansi, *ansi + 3 )) > 0x10ffff) res = 0xfffd;
#endif
        return res;
    }
    if (nls_info.AnsiTableInfo.DBCSOffsets)
    {
        USHORT off = nls_info.AnsiTableInfo.DBCSOffsets[ch];
        if (off) return nls_info.AnsiTableInfo.DBCSOffsets[off + (unsigned char)*(*ansi)++];
    }
    return nls_info.AnsiTableInfo.MultiByteTable[ch];
}

/******************************************************************************
 *	RtlCompareUnicodeStrings   (NTDLL.@)
 */
LONG WINAPI RtlCompareUnicodeStrings( const WCHAR *s1, SIZE_T len1, const WCHAR *s2, SIZE_T len2,
                                      BOOLEAN case_insensitive )
{
    LONG ret = 0;
    SIZE_T len = min( len1, len2 );

    if (case_insensitive)
    {
        if (nls_info.UpperCaseTable)
        {
            while (!ret && len--) ret = casemap( nls_info.UpperCaseTable, *s1++ ) -
                                        casemap( nls_info.UpperCaseTable, *s2++ );
        }
        else  /* locale not setup yet */
        {
            while (!ret && len--) ret = casemap_ascii( *s1++ ) - casemap_ascii( *s2++ );
        }
    }
    else
    {
        while (!ret && len--) ret = *s1++ - *s2++;
    }
    if (!ret) ret = len1 - len2;
    return ret;
}

/**************************************************************************
 *	RtlPrefixUnicodeString   (NTDLL.@)
 */
BOOLEAN WINAPI RtlPrefixUnicodeString( const UNICODE_STRING *s1, const UNICODE_STRING *s2,
                                       BOOLEAN ignore_case )
{
    unsigned int i;

    if (s1->Length > s2->Length) return FALSE;
    if (ignore_case)
    {
        for (i = 0; i < s1->Length / sizeof(WCHAR); i++)
            if (casemap( nls_info.UpperCaseTable, s1->Buffer[i] ) !=
                casemap( nls_info.UpperCaseTable, s2->Buffer[i] )) return FALSE;
    }
    else
    {
        for (i = 0; i < s1->Length / sizeof(WCHAR); i++)
            if (s1->Buffer[i] != s2->Buffer[i]) return FALSE;
    }
    return TRUE;
}



/******************************************************************************
 *	RtlHashUnicodeString   (NTDLL.@)
 */
NTSTATUS WINAPI RtlHashUnicodeString( const UNICODE_STRING *string, BOOLEAN case_insensitive,
                                      ULONG alg, ULONG *hash )
{
    unsigned int i;

    if (!string || !hash) return STATUS_INVALID_PARAMETER;

    switch (alg)
    {
    case HASH_STRING_ALGORITHM_DEFAULT:
    case HASH_STRING_ALGORITHM_X65599:
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    *hash = 0;
    if (!case_insensitive)
        for (i = 0; i < string->Length / sizeof(WCHAR); i++)
            *hash = *hash * 65599 + string->Buffer[i];
    else if (nls_info.UpperCaseTable)
        for (i = 0; i < string->Length / sizeof(WCHAR); i++)
            *hash = *hash * 65599 + casemap( nls_info.UpperCaseTable, string->Buffer[i] );
    else  /* locale not setup yet */
        for (i = 0; i < string->Length / sizeof(WCHAR); i++)
            *hash = *hash * 65599 + casemap_ascii( string->Buffer[i] );
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlCustomCPToUnicodeN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlCustomCPToUnicodeN( CPTABLEINFO *info, WCHAR *dst, DWORD dstlen, DWORD *reslen,
                                       const char *src, DWORD srclen )
{
    unsigned int ret = cp_mbstowcs( info, dst, dstlen / sizeof(WCHAR), src, srclen );
    if (reslen) *reslen = ret * sizeof(WCHAR);
    return STATUS_SUCCESS;
}

/**************************************************************************
 *	RtlUnicodeToCustomCPN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUnicodeToCustomCPN( CPTABLEINFO *info, char *dst, DWORD dstlen, DWORD *reslen,
                                       const WCHAR *src, DWORD srclen )
{
    unsigned int ret = cp_wcstombs( info, dst, dstlen, src, srclen / sizeof(WCHAR) );
    if (reslen) *reslen = ret;
    return STATUS_SUCCESS;
}

/**************************************************************************
 *	RtlMultiByteToUnicodeN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlMultiByteToUnicodeN( WCHAR *dst, DWORD dstlen, DWORD *reslen,
                                        const char *src, DWORD srclen )
{
    unsigned int ret;

    if (nls_info.AnsiTableInfo.CodePage != CP_UTF8)
        ret = cp_mbstowcs( &nls_info.AnsiTableInfo, dst, dstlen / sizeof(WCHAR), src, srclen );
    else
        utf8_mbstowcs( dst, dstlen / sizeof(WCHAR), &ret, src, srclen );

    if (reslen) *reslen = ret * sizeof(WCHAR);
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlMultiByteToUnicodeSize   (NTDLL.@)
 */
NTSTATUS WINAPI RtlMultiByteToUnicodeSize( DWORD *size, const char *str, DWORD len )
{
    unsigned int ret;

    if (nls_info.AnsiTableInfo.CodePage != CP_UTF8)
        ret = cp_mbstowcs_size( &nls_info.AnsiTableInfo, str, len );
    else
        utf8_mbstowcs_size( str, len, &ret );

    *size = ret * sizeof(WCHAR);
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlOemToUnicodeN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlOemToUnicodeN( WCHAR *dst, DWORD dstlen, DWORD *reslen,
                                  const char *src, DWORD srclen )
{
    unsigned int ret;

    if (nls_info.OemTableInfo.CodePage != CP_UTF8)
        ret = cp_mbstowcs( &nls_info.OemTableInfo, dst, dstlen / sizeof(WCHAR), src, srclen );
    else
        utf8_mbstowcs( dst, dstlen / sizeof(WCHAR), &ret, src, srclen );

    if (reslen) *reslen = ret * sizeof(WCHAR);
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlOemStringToUnicodeSize   (NTDLL.@)
 *      RtlxOemStringToUnicodeSize  (NTDLL.@)
 */
#ifdef __REACTOS__
ULONG WINAPI RtlxOemStringToUnicodeSize(IN PCOEM_STRING str)
#else
DWORD WINAPI RtlOemStringToUnicodeSize( const STRING *str )
#endif
{
    unsigned int ret;

    if (nls_info.OemTableInfo.CodePage != CP_UTF8)
        ret = cp_mbstowcs_size( &nls_info.OemTableInfo, str->Buffer, str->Length );
    else
        utf8_mbstowcs_size( str->Buffer, str->Length, &ret );

    return (ret + 1) * sizeof(WCHAR);
}


/**************************************************************************
 *      RtlUnicodeStringToOemSize   (NTDLL.@)
 *      RtlxUnicodeStringToOemSize  (NTDLL.@)
 */
#ifdef __REACTOS__
ULONG NTAPI RtlxUnicodeStringToOemSize(IN PCUNICODE_STRING str)
#else
DWORD WINAPI RtlUnicodeStringToOemSize( const UNICODE_STRING *str )
#endif
{
    unsigned int ret;

    if (nls_info.OemTableInfo.CodePage != CP_UTF8)
        ret = cp_wcstombs_size( &nls_info.OemTableInfo, str->Buffer, str->Length / sizeof(WCHAR) );
    else
        utf8_wcstombs_size( str->Buffer, str->Length / sizeof(WCHAR), &ret );

    return ret + 1;
}



/**************************************************************************
 *	RtlUnicodeToMultiByteN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUnicodeToMultiByteN( char *dst, DWORD dstlen, DWORD *reslen,
                                        const WCHAR *src, DWORD srclen )
{
    unsigned int ret;

    if (nls_info.AnsiTableInfo.CodePage != CP_UTF8)
        ret = cp_wcstombs( &nls_info.AnsiTableInfo, dst, dstlen, src, srclen / sizeof(WCHAR) );
    else
        utf8_wcstombs( dst, dstlen, &ret, src, srclen / sizeof(WCHAR) );

    if (reslen) *reslen = ret;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlUnicodeToMultiByteSize   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUnicodeToMultiByteSize( DWORD *size, const WCHAR *str, DWORD len )
{
    unsigned int ret;

    if (nls_info.AnsiTableInfo.CodePage != CP_UTF8)
        ret = cp_wcstombs_size( &nls_info.AnsiTableInfo, str, len / sizeof(WCHAR) );
    else
        utf8_wcstombs_size( str, len / sizeof(WCHAR), &ret );

    *size = ret;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlUnicodeToOemN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUnicodeToOemN( char *dst, DWORD dstlen, DWORD *reslen,
                                  const WCHAR *src, DWORD srclen )
{
    unsigned int ret;

    if (nls_info.OemTableInfo.CodePage != CP_UTF8)
        ret = cp_wcstombs( &nls_info.OemTableInfo, dst, dstlen, src, srclen / sizeof(WCHAR) );
    else
        utf8_wcstombs( dst, dstlen, &ret, src, srclen / sizeof(WCHAR) );

    if (reslen) *reslen = ret;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlDowncaseUnicodeChar   (NTDLL.@)
 */
WCHAR WINAPI RtlDowncaseUnicodeChar( WCHAR wch )
{
    if (nls_info.LowerCaseTable) return casemap( nls_info.LowerCaseTable, wch );
    if (wch >= 'A' && wch <= 'Z') wch += 'a' - 'A';
    return wch;
}


/**************************************************************************
 *	RtlDowncaseUnicodeString   (NTDLL.@)
 */
NTSTATUS WINAPI RtlDowncaseUnicodeString( UNICODE_STRING *dest, const UNICODE_STRING *src,
                                          BOOLEAN alloc )
{
    DWORD i, len = src->Length;

    if (alloc)
    {
        dest->MaximumLength = len;
#ifdef __REACTOS__
        if (!(dest->Buffer = RtlpAllocateMemory(len, 'wNLS'))) return STATUS_NO_MEMORY;
#else
        if (!(dest->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, len ))) return STATUS_NO_MEMORY;
#endif
    }
    else if (len > dest->MaximumLength) return STATUS_BUFFER_OVERFLOW;

    for (i = 0; i < len / sizeof(WCHAR); i++)
        dest->Buffer[i] = casemap( nls_info.LowerCaseTable, src->Buffer[i] );
    dest->Length = len;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlUpcaseUnicodeChar   (NTDLL.@)
 */
WCHAR WINAPI RtlUpcaseUnicodeChar( WCHAR wch )
{
    return casemap( nls_info.UpperCaseTable, wch );
}

/**************************************************************************
 *	RtlUpcaseUnicodeString   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUpcaseUnicodeString( UNICODE_STRING *dest, const UNICODE_STRING *src,
                                        BOOLEAN alloc )
{
    DWORD i, len = src->Length;

    if (alloc)
    {
        dest->MaximumLength = len;
#ifdef __REACTOS__
        if (!(dest->Buffer = RtlpAllocateMemory(len, 'wNLS'))) return STATUS_NO_MEMORY;
#else
        if (!(dest->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, len ))) return STATUS_NO_MEMORY;
#endif
    }
    else if (len > dest->MaximumLength) return STATUS_BUFFER_OVERFLOW;

    for (i = 0; i < len / sizeof(WCHAR); i++)
        dest->Buffer[i] = casemap( nls_info.UpperCaseTable, src->Buffer[i] );
    dest->Length = len;
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlUpcaseUnicodeToCustomCPN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUpcaseUnicodeToCustomCPN( CPTABLEINFO *info, char *dst, DWORD dstlen, DWORD *reslen,
                                             const WCHAR *src, DWORD srclen )
{
    DWORD i, ret;

    srclen /= sizeof(WCHAR);
    if (info->DBCSCodePage)
    {
        WCHAR *uni2cp = info->WideCharTable;

        for (i = dstlen; srclen && i; i--, srclen--, src++)
        {
            WCHAR ch = casemap( nls_info.UpperCaseTable, *src );
            if (uni2cp[ch] & 0xff00)
            {
                if (i == 1) break;  /* do not output a partial char */
                i--;
                *dst++ = uni2cp[ch] >> 8;
            }
            *dst++ = (char)uni2cp[ch];
        }
        ret = dstlen - i;
    }
    else
    {
        char *uni2cp = info->WideCharTable;
        ret = min( srclen, dstlen );
        for (i = 0; i < ret; i++) dst[i] = uni2cp[casemap( nls_info.UpperCaseTable, src[i] )];
    }
    if (reslen) *reslen = ret;
    return STATUS_SUCCESS;
}


static NTSTATUS upcase_unicode_to_utf8( char *dst, DWORD dstlen, DWORD *reslen,
                                        const WCHAR *src, DWORD srclen )
{
    char *end;
    unsigned int val;
    NTSTATUS status = STATUS_SUCCESS;

    srclen /= sizeof(WCHAR);

    for (end = dst + dstlen; srclen; srclen--, src++)
    {
        WCHAR ch = casemap( nls_info.UpperCaseTable, *src );

        if (ch < 0x80)  /* 0x00-0x7f: 1 byte */
        {
            if (dst > end - 1) break;
            *dst++ = ch;
            continue;
        }
        if (ch < 0x800)  /* 0x80-0x7ff: 2 bytes */
        {
            if (dst > end - 2) break;
            dst[1] = 0x80 | (ch & 0x3f);
            ch >>= 6;
            dst[0] = 0xc0 | ch;
            dst += 2;
            continue;
        }
        if (!get_utf16( src, srclen, &val ))
        {
            val = 0xfffd;
            status = STATUS_SOME_NOT_MAPPED;
        }
        if (val < 0x10000)  /* 0x800-0xffff: 3 bytes */
        {
            if (dst > end - 3) break;
            dst[2] = 0x80 | (val & 0x3f);
            val >>= 6;
            dst[1] = 0x80 | (val & 0x3f);
            val >>= 6;
            dst[0] = 0xe0 | val;
            dst += 3;
        }
        else   /* 0x10000-0x10ffff: 4 bytes */
        {
            if (dst > end - 4) break;
            dst[3] = 0x80 | (val & 0x3f);
            val >>= 6;
            dst[2] = 0x80 | (val & 0x3f);
            val >>= 6;
            dst[1] = 0x80 | (val & 0x3f);
            val >>= 6;
            dst[0] = 0xf0 | val;
            dst += 4;
            src++;
            srclen--;
        }
    }
    if (srclen) status = STATUS_BUFFER_TOO_SMALL;
    if (reslen) *reslen = dstlen - (end - dst);
    return status;
}

/**************************************************************************
 *	RtlUpcaseUnicodeToMultiByteN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUpcaseUnicodeToMultiByteN( char *dst, DWORD dstlen, DWORD *reslen,
                                              const WCHAR *src, DWORD srclen )
{
    if (nls_info.AnsiTableInfo.CodePage == CP_UTF8)
        return upcase_unicode_to_utf8( dst, dstlen, reslen, src, srclen );
    return RtlUpcaseUnicodeToCustomCPN( &nls_info.AnsiTableInfo, dst, dstlen, reslen, src, srclen );
}


/**************************************************************************
 *	RtlUpcaseUnicodeToOemN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUpcaseUnicodeToOemN( char *dst, DWORD dstlen, DWORD *reslen,
                                        const WCHAR *src, DWORD srclen )
{
    if (nls_info.OemTableInfo.CodePage == CP_UTF8)
        return upcase_unicode_to_utf8( dst, dstlen, reslen, src, srclen );
    return RtlUpcaseUnicodeToCustomCPN( &nls_info.OemTableInfo, dst, dstlen, reslen, src, srclen );
}


#ifndef __REACTOS__ /* Conflicts with CRT */
/*********************************************************************
 *	towlower   (NTDLL.@)
 */
WCHAR __cdecl towlower( WCHAR ch )
{
    if (ch >= 0x100) return ch;
    return casemap( nls_info.LowerCaseTable, ch );
}
#endif

/*********************************************************************
 *           towupper    (NTDLL.@)
 */
WCHAR __cdecl towupper( WCHAR ch )
{
    if (nls_info.UpperCaseTable) return casemap( nls_info.UpperCaseTable, ch );
    return casemap_ascii( ch );
}


/******************************************************************
 *      RtlIsValidLocaleName   (NTDLL.@)
 */
BOOLEAN WINAPI RtlIsValidLocaleName( const WCHAR *name, ULONG flags )
{
    const NLS_LOCALE_LCNAME_INDEX *entry = find_lcname_entry( locale_table, name );

    if (!entry) return FALSE;
    /* reject neutral locale unless flag 2 is set */
    if (!(flags & 2) && !get_locale_data( locale_table, entry->idx )->inotneutral) return FALSE;
    return TRUE;
}


/******************************************************************
 *      RtlLcidToLocaleName   (NTDLL.@)
 */
NTSTATUS WINAPI RtlLcidToLocaleName( LCID lcid, UNICODE_STRING *str, ULONG flags, BOOLEAN alloc )
{
    const NLS_LOCALE_LCID_INDEX *entry;
    const WCHAR *name;
    ULONG len;

    if (!str) return STATUS_INVALID_PARAMETER_2;

    switch (lcid)
    {
    case LOCALE_USER_DEFAULT:
        NtQueryDefaultLocale( TRUE, &lcid );
        break;
    case LOCALE_SYSTEM_DEFAULT:
    case LOCALE_CUSTOM_DEFAULT:
        lcid = system_lcid;
        break;
    case LOCALE_CUSTOM_UI_DEFAULT:
        return STATUS_UNSUCCESSFUL;
    case LOCALE_CUSTOM_UNSPECIFIED:
        return STATUS_INVALID_PARAMETER_1;
    }

    if (!(entry = find_lcid_entry( locale_table, lcid ))) return STATUS_INVALID_PARAMETER_1;
    /* reject neutral locale unless flag 2 is set */
    if (!(flags & 2) && !get_locale_data( locale_table, entry->idx )->inotneutral)
        return STATUS_INVALID_PARAMETER_1;

    name = locale_strings + entry->name;
    len = *name++;

    if (alloc)
    {
#ifdef __REACTOS__
        if (!(str->Buffer = RtlpAllocateMemory((len + 1) * sizeof(WCHAR), 'wNLS')))
#else
        if (!(str->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
#endif
            return STATUS_NO_MEMORY;
        str->MaximumLength = (len + 1) * sizeof(WCHAR);
    }
    else if (str->MaximumLength < (len + 1) * sizeof(WCHAR)) return STATUS_BUFFER_TOO_SMALL;

    wcscpy( str->Buffer, name );
    str->Length = len * sizeof(WCHAR);
#ifndef __REACTOS__
    TRACE( "%04lx -> %s\n", lcid, debugstr_us(str) );
#endif
    return STATUS_SUCCESS;
}


/******************************************************************
 *      RtlLocaleNameToLcid   (NTDLL.@)
 */
NTSTATUS WINAPI RtlLocaleNameToLcid( const WCHAR *name, LCID *lcid, ULONG flags )
{
    const NLS_LOCALE_LCNAME_INDEX *entry = find_lcname_entry( locale_table, name );

    if (!entry) return STATUS_INVALID_PARAMETER_1;
    /* reject neutral locale unless flag 2 is set */
    if (!(flags & 2) && !get_locale_data( locale_table, entry->idx )->inotneutral)
        return STATUS_INVALID_PARAMETER_1;
    *lcid = entry->id;
    TRACE( "%s -> %04lx\n", debugstr_w(name), *lcid );
    return STATUS_SUCCESS;
}


/**************************************************************************
 *	RtlUTF8ToUnicodeN   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUTF8ToUnicodeN( WCHAR *dst, DWORD dstlen, DWORD *reslen, const char *src, DWORD srclen )
{
    unsigned int ret;
    NTSTATUS status;

    if (!src) return STATUS_INVALID_PARAMETER_4;
    if (!reslen) return STATUS_INVALID_PARAMETER;

    if (!dst)
        status = utf8_mbstowcs_size( src, srclen, &ret );
    else
        status = utf8_mbstowcs( dst, dstlen / sizeof(WCHAR), &ret, src, srclen );

    *reslen = ret * sizeof(WCHAR);
    return status;
}


/**************************************************************************
 *	RtlUnicodeToUTF8N   (NTDLL.@)
 */
NTSTATUS WINAPI RtlUnicodeToUTF8N( char *dst, DWORD dstlen, DWORD *reslen, const WCHAR *src, DWORD srclen )
{
    unsigned int ret;
    NTSTATUS status;

    if (!src) return STATUS_INVALID_PARAMETER_4;
    if (!reslen) return STATUS_INVALID_PARAMETER;
    if (dst && (srclen & 1)) return STATUS_INVALID_PARAMETER_5;

    if (!dst)
        status = utf8_wcstombs_size( src, srclen / sizeof(WCHAR), &ret );
    else
        status = utf8_wcstombs( dst, dstlen, &ret, src, srclen / sizeof(WCHAR) );

    *reslen = ret;
    return status;
}


/******************************************************************************
 *      RtlIsNormalizedString   (NTDLL.@)
 */
NTSTATUS WINAPI RtlIsNormalizedString( ULONG form, const WCHAR *str, INT len, BOOLEAN *res )
{
    const struct norm_table *info;
    NTSTATUS status;
    BYTE props, class, last_class = 0;
    unsigned int ch;
    int i, r, result = 1;

    if ((status = load_norm_table( form, &info ))) return status;

    if (len == -1) len = wcslen( str );

    for (i = 0; i < len && result; i += r)
    {
        if (!(r = get_utf16( str + i, len - i, &ch ))) return STATUS_NO_UNICODE_TRANSLATION;
        if (info->comp_size)
        {
            if ((ch >= HANGUL_VBASE && ch < HANGUL_VBASE + HANGUL_VCOUNT) ||
                (ch >= HANGUL_TBASE && ch < HANGUL_TBASE + HANGUL_TCOUNT))
            {
                result = -1;  /* QC=Maybe */
                continue;
            }
        }
        else if (ch >= HANGUL_SBASE && ch < HANGUL_SBASE + HANGUL_SCOUNT)
        {
            result = 0;  /* QC=No */
            break;
        }
        props = get_char_props( info, ch );
        class = props & 0x3f;
        if (class == 0x3f)
        {
            last_class = 0;
            if (props == 0xbf) result = 0;  /* QC=No */
            else if (props == 0xff)
            {
                /* ignore other chars in Hangul range */
                if (ch >= HANGUL_LBASE && ch < HANGUL_LBASE + 0x100) continue;
                if (ch >= HANGUL_SBASE && ch < HANGUL_SBASE + 0x2c00) continue;
                /* allow final null */
                if (!ch && i == len - 1) continue;
                return STATUS_NO_UNICODE_TRANSLATION;
            }
        }
        else if (props & 0x80)
        {
            if ((props & 0xc0) == 0xc0) result = -1;  /* QC=Maybe */
            if (class && class < last_class) result = 0;  /* QC=No */
            last_class = class;
        }
        else last_class = 0;
    }

    if (result == -1)
    {
        int dstlen = len * 4;
        NTSTATUS status;
#ifdef __REACTOS__
        WCHAR *buffer = RtlpAllocateMemory(dstlen * sizeof(WCHAR), 'wNLS');
#else
        WCHAR *buffer = RtlAllocateHeap( GetProcessHeap(), 0, dstlen * sizeof(WCHAR) );
#endif
        if (!buffer) return STATUS_NO_MEMORY;
        status = RtlNormalizeString( form, str, len, buffer, &dstlen );
        result = !status && (dstlen == len) && !wcsncmp( buffer, str, len );
#ifdef __REACTOS__
    RtlpFreeMemory(buffer, 'wNLS');
#else
    RtlFreeHeap( GetProcessHeap(), 0, buffer );
#endif
    }
    *res = result;
    return STATUS_SUCCESS;
}


/******************************************************************************
 *      RtlNormalizeString   (NTDLL.@)
 */
NTSTATUS WINAPI RtlNormalizeString( ULONG form, const WCHAR *src, INT src_len, WCHAR *dst, INT *dst_len )
{
    int buf_len;
    WCHAR *buf = NULL;
    const struct norm_table *info;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE( "%lx %s %d %p %d\n", form, debugstr_wn(src, src_len), src_len, dst, *dst_len );

    if ((status = load_norm_table( form, &info ))) return status;

    if (src_len == -1) src_len = wcslen(src) + 1;

    if (!*dst_len)
    {
        *dst_len = src_len * info->len_factor;
        if (*dst_len > 64) *dst_len = max( 64, src_len + src_len / 8 );
        return STATUS_SUCCESS;
    }
    if (!src_len)
    {
        *dst_len = 0;
        return STATUS_SUCCESS;
    }

    if (!info->comp_size) return decompose_string( info, src, src_len, dst, dst_len );

    buf_len = src_len * 4;
    for (;;)
    {
#ifdef __REACTOS__
        buf = RtlpAllocateMemory(buf_len * sizeof(WCHAR), 'wNLS');
#else
        buf = RtlAllocateHeap( GetProcessHeap(), 0, buf_len * sizeof(WCHAR) );
#endif
        if (!buf) return STATUS_NO_MEMORY;
        status = decompose_string( info, src, src_len, buf, &buf_len );
        if (status != STATUS_BUFFER_TOO_SMALL) break;
#ifdef __REACTOS__
    RtlpFreeMemory(buf, 'wNLS');
#else
    RtlFreeHeap( GetProcessHeap(), 0, buf );
#endif
    }
    if (!status)
    {
        buf_len = compose_string( info, buf, buf_len );
        if (*dst_len >= buf_len) memcpy( dst, buf, buf_len * sizeof(WCHAR) );
        else status = STATUS_BUFFER_TOO_SMALL;
    }
#ifdef __REACTOS__
    RtlpFreeMemory(buf, 'wNLS');
#else
    RtlFreeHeap( GetProcessHeap(), 0, buf );
#endif
    *dst_len = buf_len;
    return status;
}


/* Punycode parameters */
enum { BASE = 36, TMIN = 1, TMAX = 26, SKEW = 38, DAMP = 700 };

static BOOL check_invalid_chars( const struct norm_table *info, DWORD flags,
                                 const unsigned int *buffer, int len )
{
    int i;

    for (i = 0; i < len; i++)
    {
        switch (buffer[i])
        {
        case 0x200c:  /* zero-width non-joiner */
        case 0x200d:  /* zero-width joiner */
            if (!i || get_combining_class( info, buffer[i - 1] ) != 9) return TRUE;
            break;
        case 0x2260:  /* not equal to */
        case 0x226e:  /* not less than */
        case 0x226f:  /* not greater than */
            if (flags & IDN_USE_STD3_ASCII_RULES) return TRUE;
            break;
        }
        switch (get_char_props( info, buffer[i] ))
        {
        case 0xbf:
            return TRUE;
        case 0xff:
            if (buffer[i] >= HANGUL_SBASE && buffer[i] < HANGUL_SBASE + 0x2c00) break;
            return TRUE;
        case 0x7f:
            if (!(flags & IDN_ALLOW_UNASSIGNED)) return TRUE;
            break;
        }
    }

    if ((flags & IDN_USE_STD3_ASCII_RULES) && len && (buffer[0] == '-' || buffer[len - 1] == '-'))
        return TRUE;

    return FALSE;
}


/******************************************************************************
 *      RtlIdnToAscii   (NTDLL.@)
 */
NTSTATUS WINAPI RtlIdnToAscii( DWORD flags, const WCHAR *src, INT srclen, WCHAR *dst, INT *dstlen )
{
    static const WCHAR prefixW[] = {'x','n','-','-'};
    const struct norm_table *info;
    NTSTATUS status;
    WCHAR normstr[256], res[256];
    unsigned int ch, buffer[64];
    int i, len, start, end, out_label, out = 0, normlen = ARRAY_SIZE(normstr);

    TRACE( "%lx %s %p %d\n", flags, debugstr_wn(src, srclen), dst, *dstlen );

    if ((status = load_norm_table( 13, &info ))) return status;

    if ((status = RtlIdnToNameprepUnicode( flags, src, srclen, normstr, &normlen ))) return status;

    /* implementation of Punycode based on RFC 3492 */

    for (start = 0; start < normlen; start = end + 1)
    {
        int n = 0x80, bias = 72, delta = 0, b = 0, h, buflen = 0;

        out_label = out;
        for (i = start; i < normlen; i += len)
        {
            if (!(len = get_utf16( normstr + i, normlen - i, &ch ))) break;
            if (!ch || ch == '.') break;
            if (ch < 0x80) b++;
            buffer[buflen++] = ch;
        }
        end = i;

        if (b == end - start)
        {
            if (end < normlen) b++;
            if (out + b > ARRAY_SIZE(res)) return STATUS_INVALID_IDN_NORMALIZATION;
            memcpy( res + out, normstr + start, b * sizeof(WCHAR) );
            out += b;
            continue;
        }

        if (buflen >= 4 && buffer[2] == '-' && buffer[3] == '-') return STATUS_INVALID_IDN_NORMALIZATION;
        if (check_invalid_chars( info, flags, buffer, buflen )) return STATUS_INVALID_IDN_NORMALIZATION;

        if (out + 5 + b > ARRAY_SIZE(res)) return STATUS_INVALID_IDN_NORMALIZATION;
        memcpy( res + out, prefixW, sizeof(prefixW) );
        out += ARRAY_SIZE(prefixW);
        if (b)
        {
            for (i = start; i < end; i++) if (normstr[i] < 0x80) res[out++] = normstr[i];
            res[out++] = '-';
        }

        for (h = b; h < buflen; delta++, n++)
        {
            int m = 0x10ffff, q, k;

            for (i = 0; i < buflen; i++) if (buffer[i] >= n && m > buffer[i]) m = buffer[i];
            delta += (m - n) * (h + 1);
            n = m;

            for (i = 0; i < buflen; i++)
            {
                if (buffer[i] == n)
                {
                    for (q = delta, k = BASE; ; k += BASE)
                    {
                        int t = k <= bias ? TMIN : k >= bias + TMAX ? TMAX : k - bias;
                        int disp = q < t ? q : t + (q - t) % (BASE - t);
                        if (out + 1 > ARRAY_SIZE(res)) return STATUS_INVALID_IDN_NORMALIZATION;
                        res[out++] = disp <= 25 ? 'a' + disp : '0' + disp - 26;
                        if (q < t) break;
                        q = (q - t) / (BASE - t);
                    }
                    delta /= (h == b ? DAMP : 2);
                    delta += delta / (h + 1);
                    for (k = 0; delta > ((BASE - TMIN) * TMAX) / 2; k += BASE) delta /= BASE - TMIN;
                    bias = k + ((BASE - TMIN + 1) * delta) / (delta + SKEW);
                    delta = 0;
                    h++;
                }
                else if (buffer[i] < n) delta++;
            }
        }

        if (out - out_label > 63) return STATUS_INVALID_IDN_NORMALIZATION;

        if (end < normlen)
        {
            if (out + 1 > ARRAY_SIZE(res)) return STATUS_INVALID_IDN_NORMALIZATION;
            res[out++] = normstr[end];
        }
    }

    if (*dstlen)
    {
        if (out <= *dstlen) memcpy( dst, res, out * sizeof(WCHAR) );
        else status = STATUS_BUFFER_TOO_SMALL;
    }
    *dstlen = out;
    return status;
}


/******************************************************************************
 *      RtlIdnToNameprepUnicode   (NTDLL.@)
 */
NTSTATUS WINAPI RtlIdnToNameprepUnicode( DWORD flags, const WCHAR *src, INT srclen,
                                         WCHAR *dst, INT *dstlen )
{
    const struct norm_table *info;
    unsigned int ch;
    NTSTATUS status;
    WCHAR buf[256];
    int i, start, len, buflen = ARRAY_SIZE(buf);

    if (flags & ~(IDN_ALLOW_UNASSIGNED | IDN_USE_STD3_ASCII_RULES)) return STATUS_INVALID_PARAMETER;
    if (!src || srclen < -1) return STATUS_INVALID_PARAMETER;

    TRACE( "%lx %s %p %d\n", flags, debugstr_wn(src, srclen), dst, *dstlen );

    if ((status = load_norm_table( 13, &info ))) return status;

    if (srclen == -1) srclen = wcslen(src) + 1;

    for (i = 0; i < srclen; i++) if (src[i] < 0x20 || src[i] >= 0x7f) break;

    if (i == srclen || (i == srclen - 1 && !src[i]))  /* ascii only */
    {
        if (srclen > buflen) return STATUS_INVALID_IDN_NORMALIZATION;
        memcpy( buf, src, srclen * sizeof(WCHAR) );
        buflen = srclen;
    }
    else if ((status = RtlNormalizeString( 13, src, srclen, buf, &buflen )))
    {
        if (status == STATUS_NO_UNICODE_TRANSLATION) status = STATUS_INVALID_IDN_NORMALIZATION;
        return status;
    }

    for (i = start = 0; i < buflen; i += len)
    {
        if (!(len = get_utf16( buf + i, buflen - i, &ch ))) break;
        if (!ch) break;
        if (ch == '.')
        {
            if (start == i) return STATUS_INVALID_IDN_NORMALIZATION;
            /* maximal label length is 63 characters */
            if (i - start > 63) return STATUS_INVALID_IDN_NORMALIZATION;
            if ((flags & IDN_USE_STD3_ASCII_RULES) && (buf[start] == '-' || buf[i-1] == '-'))
                return STATUS_INVALID_IDN_NORMALIZATION;
            start = i + 1;
            continue;
        }
        if (flags & IDN_USE_STD3_ASCII_RULES)
        {
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') || ch == '-') continue;
            return STATUS_INVALID_IDN_NORMALIZATION;
        }
        if (!(flags & IDN_ALLOW_UNASSIGNED))
        {
            if (get_char_props( info, ch ) == 0x7f) return STATUS_INVALID_IDN_NORMALIZATION;
        }
    }
    if (!i || i - start > 63) return STATUS_INVALID_IDN_NORMALIZATION;
    if ((flags & IDN_USE_STD3_ASCII_RULES) && (buf[start] == '-' || buf[i-1] == '-'))
        return STATUS_INVALID_IDN_NORMALIZATION;

    if (*dstlen)
    {
        if (buflen <= *dstlen) memcpy( dst, buf, buflen * sizeof(WCHAR) );
        else status = STATUS_BUFFER_TOO_SMALL;
    }
    *dstlen = buflen;
    return status;
}


/******************************************************************************
 *      RtlIdnToUnicode   (NTDLL.@)
 */
NTSTATUS WINAPI RtlIdnToUnicode( DWORD flags, const WCHAR *src, INT srclen, WCHAR *dst, INT *dstlen )
{
    const struct norm_table *info;
    int i, buflen, start, end, out_label, out = 0;
    NTSTATUS status;
    UINT buffer[64];
    WCHAR ch = 0;

    if (!src || srclen < -1) return STATUS_INVALID_PARAMETER;
    if (srclen == -1) srclen = wcslen( src ) + 1;

    TRACE( "%lx %s %p %d\n", flags, debugstr_wn(src, srclen), dst, *dstlen );

    if ((status = load_norm_table( 13, &info ))) return status;

    for (start = 0; start < srclen; )
    {
        int n = 0x80, bias = 72, pos = 0, old_pos, w, k, t, delim = 0, digit, delta;

        out_label = out;
        for (i = start; i < srclen; i++)
        {
            ch = src[i];
            if (ch > 0x7f || (i != srclen - 1 && !ch)) return STATUS_INVALID_IDN_NORMALIZATION;
            if (!ch || ch == '.') break;
            if (ch == '-') delim = i;

            if (!(flags & IDN_USE_STD3_ASCII_RULES)) continue;
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') || ch == '-')
                continue;
            return STATUS_INVALID_IDN_NORMALIZATION;
        }
        end = i;

        /* last label may be empty */
        if (start == end && ch) return STATUS_INVALID_IDN_NORMALIZATION;

        if (end - start < 4 ||
            (src[start] != 'x' && src[start] != 'X') ||
            (src[start + 1] != 'n' && src[start + 1] != 'N') ||
            src[start + 2] != '-' || src[start + 3] != '-')
        {
            if (end - start > 63) return STATUS_INVALID_IDN_NORMALIZATION;

            if ((flags & IDN_USE_STD3_ASCII_RULES) && (src[start] == '-' || src[end - 1] == '-'))
                return STATUS_INVALID_IDN_NORMALIZATION;

            if (end < srclen) end++;
            if (*dstlen)
            {
                if (out + end - start <= *dstlen)
                    memcpy( dst + out, src + start, (end - start) * sizeof(WCHAR));
                else return STATUS_BUFFER_TOO_SMALL;
            }
            out += end - start;
            start = end;
            continue;
        }

        if (delim == start + 3) delim++;
        buflen = 0;
        for (i = start + 4; i < delim && buflen < ARRAY_SIZE(buffer); i++) buffer[buflen++] = src[i];
        if (buflen) i++;
        while (i < end)
        {
            old_pos = pos;
            w = 1;
            for (k = BASE; ; k += BASE)
            {
                if (i >= end) return STATUS_INVALID_IDN_NORMALIZATION;
                ch = src[i++];
                if (ch >= 'a' && ch <= 'z') digit = ch - 'a';
                else if (ch >= 'A' && ch <= 'Z') digit = ch - 'A';
                else if (ch >= '0' && ch <= '9') digit = ch - '0' + 26;
                else return STATUS_INVALID_IDN_NORMALIZATION;
                pos += digit * w;
                t = k <= bias ? TMIN : k >= bias + TMAX ? TMAX : k - bias;
                if (digit < t) break;
                w *= BASE - t;
            }

            delta = (pos - old_pos) / (!old_pos ? DAMP : 2);
            delta += delta / (buflen + 1);
            for (k = 0; delta > ((BASE - TMIN) * TMAX) / 2; k += BASE) delta /= BASE - TMIN;
            bias = k + ((BASE - TMIN + 1) * delta) / (delta + SKEW);
            n += pos / (buflen + 1);
            pos %= buflen + 1;

            if (buflen >= ARRAY_SIZE(buffer) - 1) return STATUS_INVALID_IDN_NORMALIZATION;
            memmove( buffer + pos + 1, buffer + pos, (buflen - pos) * sizeof(*buffer) );
            buffer[pos++] = n;
            buflen++;
        }

        if (check_invalid_chars( info, flags, buffer, buflen )) return STATUS_INVALID_IDN_NORMALIZATION;

        for (i = 0; i < buflen; i++)
        {
            int len = 1 + (buffer[i] >= 0x10000);
            if (*dstlen)
            {
                if (out + len <= *dstlen) put_utf16( dst + out, buffer[i] );
                else return STATUS_BUFFER_TOO_SMALL;
            }
            out += len;
        }

        if (out - out_label > 63) return STATUS_INVALID_IDN_NORMALIZATION;

        if (end < srclen)
        {
            if (*dstlen)
            {
                if (out + 1 <= *dstlen) dst[out] = src[end];
                else return STATUS_BUFFER_TOO_SMALL;
            }
            out++;
        }
        start = end + 1;
    }
    *dstlen = out;
    return STATUS_SUCCESS;
}
