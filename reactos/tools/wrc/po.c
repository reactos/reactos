/*
 * Support for po files
 *
 * Copyright 2010 Alexandre Julliard
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#ifdef HAVE_LIBGETTEXTPO
#include <gettext-po.h>
#endif

#include "wrc.h"
#include "genres.h"
#include "newstruc.h"
#include "utils.h"
#include "wine/list.h"
#include "wine/unicode.h"

static resource_t *new_top, *new_tail;

static int is_english( const language_t *lan )
{
    return lan->id == LANG_ENGLISH && lan->sub == SUBLANG_DEFAULT;
}

static version_t *get_dup_version( language_t *lang )
{
    /* English "translations" take precedence over the original rc contents */
    return new_version( is_english( lang ) ? 1 : -1 );
}

static name_id_t *dup_name_id( name_id_t *id )
{
    name_id_t *new;

    if (!id || id->type != name_str) return id;
    new = new_name_id();
    *new = *id;
    new->name.s_name = convert_string( id->name.s_name, str_unicode, 1252 );
    return new;
}

static char *convert_msgid_ascii( const string_t *str, int error_on_invalid_char )
{
    int i;
    string_t *newstr = convert_string( str, str_unicode, 1252 );
    char *buffer = xmalloc( newstr->size + 1 );

    for (i = 0; i < newstr->size; i++)
    {
        buffer[i] =  newstr->str.wstr[i];
        if (newstr->str.wstr[i] >= 32 && newstr->str.wstr[i] <= 127) continue;
        if (newstr->str.wstr[i] == '\t' || newstr->str.wstr[i] == '\n') continue;
        if (error_on_invalid_char)
        {
            print_location( &newstr->loc );
            error( "Invalid character %04x in source string\n", newstr->str.wstr[i] );
        }
        free( buffer);
        free_string( newstr );
        return NULL;
    }
    buffer[i] = 0;
    free_string( newstr );
    return buffer;
}

static char *get_message_context( char **msgid )
{
    static const char magic[] = "#msgctxt#";
    char *id, *context;

    if (strncmp( *msgid, magic, sizeof(magic) - 1 )) return NULL;
    context = *msgid + sizeof(magic) - 1;
    if (!(id = strchr( context, '#' ))) return NULL;
    *id = 0;
    *msgid = id + 1;
    return context;
}

static int control_has_title( const control_t *ctrl )
{
    if (!ctrl->title) return 0;
    if (ctrl->title->type != name_str) return 0;
    /* check for text static control */
    if (ctrl->ctlclass && ctrl->ctlclass->type == name_ord && ctrl->ctlclass->name.i_name == CT_STATIC)
    {
        switch (ctrl->style->or_mask & SS_TYPEMASK)
        {
        case SS_LEFT:
        case SS_CENTER:
        case SS_RIGHT:
            return 1;
        default:
            return 0;
        }
    }
    return 1;
}

static resource_t *dup_resource( resource_t *res, language_t *lang )
{
    resource_t *new = xmalloc( sizeof(*new) );

    *new = *res;
    new->lan = lang;
    new->next = new->prev = NULL;
    new->name = dup_name_id( res->name );

    switch (res->type)
    {
    case res_dlg:
        new->res.dlg = xmalloc( sizeof(*(new)->res.dlg) );
        *new->res.dlg = *res->res.dlg;
        new->res.dlg->lvc.language = lang;
        new->res.dlg->lvc.version = get_dup_version( lang );
        break;
    case res_men:
        new->res.men = xmalloc( sizeof(*(new)->res.men) );
        *new->res.men = *res->res.men;
        new->res.men->lvc.language = lang;
        new->res.men->lvc.version = get_dup_version( lang );
        break;
    case res_stt:
        new->res.stt = xmalloc( sizeof(*(new)->res.stt) );
        *new->res.stt = *res->res.stt;
        new->res.stt->lvc.language = lang;
        new->res.stt->lvc.version = get_dup_version( lang );
        break;
    default:
        assert(0);
    }
    return new;
}

static const struct
{
    unsigned int id, sub;
    const char *name;
} languages[] =
{
    { LANG_ARABIC,         SUBLANG_NEUTRAL,                     "ar" },
    { LANG_ARABIC,         SUBLANG_ARABIC_SAUDI_ARABIA,         "ar_SA" },
    { LANG_ARABIC,         SUBLANG_ARABIC_IRAQ,                 "ar_IQ" },
    { LANG_ARABIC,         SUBLANG_ARABIC_EGYPT,                "ar_EG" },
    { LANG_ARABIC,         SUBLANG_ARABIC_LIBYA,                "ar_LY" },
    { LANG_ARABIC,         SUBLANG_ARABIC_ALGERIA,              "ar_DZ" },
    { LANG_ARABIC,         SUBLANG_ARABIC_MOROCCO,              "ar_MA" },
    { LANG_ARABIC,         SUBLANG_ARABIC_TUNISIA,              "ar_TN" },
    { LANG_ARABIC,         SUBLANG_ARABIC_OMAN,                 "ar_OM" },
    { LANG_ARABIC,         SUBLANG_ARABIC_YEMEN,                "ar_YE" },
    { LANG_ARABIC,         SUBLANG_ARABIC_SYRIA,                "ar_SY" },
    { LANG_ARABIC,         SUBLANG_ARABIC_JORDAN,               "ar_JO" },
    { LANG_ARABIC,         SUBLANG_ARABIC_LEBANON,              "ar_LB" },
    { LANG_ARABIC,         SUBLANG_ARABIC_KUWAIT,               "ar_KW" },
    { LANG_ARABIC,         SUBLANG_ARABIC_UAE,                  "ar_AE" },
    { LANG_ARABIC,         SUBLANG_ARABIC_BAHRAIN,              "ar_BH" },
    { LANG_ARABIC,         SUBLANG_ARABIC_QATAR,                "ar_QA" },
    { LANG_BULGARIAN,      SUBLANG_NEUTRAL,                     "bg" },
    { LANG_BULGARIAN,      SUBLANG_BULGARIAN_BULGARIA,          "bg_BG" },
    { LANG_CATALAN,        SUBLANG_NEUTRAL,                     "ca" },
    { LANG_CATALAN,        SUBLANG_CATALAN_CATALAN,             "ca_ES" },
    { LANG_CHINESE,        SUBLANG_NEUTRAL,                     "zh" },
    { LANG_CHINESE,        SUBLANG_CHINESE_TRADITIONAL,         "zh_TW" },
    { LANG_CHINESE,        SUBLANG_CHINESE_SIMPLIFIED,          "zh_CN" },
    { LANG_CHINESE,        SUBLANG_CHINESE_HONGKONG,            "zh_HK" },
    { LANG_CHINESE,        SUBLANG_CHINESE_SINGAPORE,           "zh_SG" },
    { LANG_CHINESE,        SUBLANG_CHINESE_MACAU,               "zh_MO" },
    { LANG_CZECH,          SUBLANG_NEUTRAL,                     "cs" },
    { LANG_CZECH,          SUBLANG_CZECH_CZECH_REPUBLIC,        "cs_CZ" },
    { LANG_DANISH,         SUBLANG_NEUTRAL,                     "da" },
    { LANG_DANISH,         SUBLANG_DANISH_DENMARK,              "da_DK" },
    { LANG_GERMAN,         SUBLANG_NEUTRAL,                     "de" },
    { LANG_GERMAN,         SUBLANG_GERMAN,                      "de_DE" },
    { LANG_GERMAN,         SUBLANG_GERMAN_SWISS,                "de_CH" },
    { LANG_GERMAN,         SUBLANG_GERMAN_AUSTRIAN,             "de_AT" },
    { LANG_GERMAN,         SUBLANG_GERMAN_LUXEMBOURG,           "de_LU" },
    { LANG_GERMAN,         SUBLANG_GERMAN_LIECHTENSTEIN,        "de_LI" },
    { LANG_GREEK,          SUBLANG_NEUTRAL,                     "el" },
    { LANG_GREEK,          SUBLANG_GREEK_GREECE,                "el_GR" },
    { LANG_ENGLISH,        SUBLANG_NEUTRAL,                     "en" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_US,                  "en_US" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_UK,                  "en_GB" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_AUS,                 "en_AU" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_CAN,                 "en_CA" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_NZ,                  "en_NZ" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_EIRE,                "en_IE" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_SOUTH_AFRICA,        "en_ZA" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_JAMAICA,             "en_JM" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_CARIBBEAN,           "en_CB" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_BELIZE,              "en_BZ" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_TRINIDAD,            "en_TT" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_ZIMBABWE,            "en_ZW" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_PHILIPPINES,         "en_PH" },
    { LANG_SPANISH,        SUBLANG_NEUTRAL,                     "es" },
    { LANG_SPANISH,        SUBLANG_SPANISH,                     "es_ES" },
    { LANG_SPANISH,        SUBLANG_SPANISH_MEXICAN,             "es_MX" },
    { LANG_SPANISH,        SUBLANG_SPANISH_MODERN,              "es_ES_modern" },
    { LANG_SPANISH,        SUBLANG_SPANISH_GUATEMALA,           "es_GT" },
    { LANG_SPANISH,        SUBLANG_SPANISH_COSTA_RICA,          "es_CR" },
    { LANG_SPANISH,        SUBLANG_SPANISH_PANAMA,              "es_PA" },
    { LANG_SPANISH,        SUBLANG_SPANISH_DOMINICAN_REPUBLIC,  "es_DO" },
    { LANG_SPANISH,        SUBLANG_SPANISH_VENEZUELA,           "es_VE" },
    { LANG_SPANISH,        SUBLANG_SPANISH_COLOMBIA,            "es_CO" },
    { LANG_SPANISH,        SUBLANG_SPANISH_PERU,                "es_PE" },
    { LANG_SPANISH,        SUBLANG_SPANISH_ARGENTINA,           "es_AR" },
    { LANG_SPANISH,        SUBLANG_SPANISH_ECUADOR,             "es_EC" },
    { LANG_SPANISH,        SUBLANG_SPANISH_CHILE,               "es_CL" },
    { LANG_SPANISH,        SUBLANG_SPANISH_URUGUAY,             "es_UY" },
    { LANG_SPANISH,        SUBLANG_SPANISH_PARAGUAY,            "es_PY" },
    { LANG_SPANISH,        SUBLANG_SPANISH_BOLIVIA,             "es_BO" },
    { LANG_SPANISH,        SUBLANG_SPANISH_EL_SALVADOR,         "es_SV" },
    { LANG_SPANISH,        SUBLANG_SPANISH_HONDURAS,            "es_HN" },
    { LANG_SPANISH,        SUBLANG_SPANISH_NICARAGUA,           "es_NI" },
    { LANG_SPANISH,        SUBLANG_SPANISH_PUERTO_RICO,         "es_PR" },
    { LANG_FINNISH,        SUBLANG_NEUTRAL,                     "fi" },
    { LANG_FINNISH,        SUBLANG_FINNISH_FINLAND,             "fi_FI" },
    { LANG_FRENCH,         SUBLANG_NEUTRAL,                     "fr" },
    { LANG_FRENCH,         SUBLANG_FRENCH,                      "fr_FR" },
    { LANG_FRENCH,         SUBLANG_FRENCH_BELGIAN,              "fr_BE" },
    { LANG_FRENCH,         SUBLANG_FRENCH_CANADIAN,             "fr_CA" },
    { LANG_FRENCH,         SUBLANG_FRENCH_SWISS,                "fr_CH" },
    { LANG_FRENCH,         SUBLANG_FRENCH_LUXEMBOURG,           "fr_LU" },
    { LANG_FRENCH,         SUBLANG_FRENCH_MONACO,               "fr_MC" },
    { LANG_HEBREW,         SUBLANG_NEUTRAL,                     "he" },
    { LANG_HEBREW,         SUBLANG_HEBREW_ISRAEL,               "he_IL" },
    { LANG_HUNGARIAN,      SUBLANG_NEUTRAL,                     "hu" },
    { LANG_HUNGARIAN,      SUBLANG_HUNGARIAN_HUNGARY,           "hu_HU" },
    { LANG_ICELANDIC,      SUBLANG_NEUTRAL,                     "is" },
    { LANG_ICELANDIC,      SUBLANG_ICELANDIC_ICELAND,           "is_IS" },
    { LANG_ITALIAN,        SUBLANG_NEUTRAL,                     "it" },
    { LANG_ITALIAN,        SUBLANG_ITALIAN,                     "it_IT" },
    { LANG_ITALIAN,        SUBLANG_ITALIAN_SWISS,               "it_CH" },
    { LANG_JAPANESE,       SUBLANG_NEUTRAL,                     "ja" },
    { LANG_JAPANESE,       SUBLANG_JAPANESE_JAPAN,              "ja_JP" },
    { LANG_KOREAN,         SUBLANG_NEUTRAL,                     "ko" },
    { LANG_KOREAN,         SUBLANG_KOREAN,                      "ko_KR" },
    { LANG_DUTCH,          SUBLANG_NEUTRAL,                     "nl" },
    { LANG_DUTCH,          SUBLANG_DUTCH,                       "nl_NL" },
    { LANG_DUTCH,          SUBLANG_DUTCH_BELGIAN,               "nl_BE" },
    { LANG_DUTCH,          SUBLANG_DUTCH_SURINAM,               "nl_SR" },
    { LANG_NORWEGIAN,      SUBLANG_NORWEGIAN_BOKMAL,            "nb_NO" },
    { LANG_NORWEGIAN,      SUBLANG_NORWEGIAN_NYNORSK,           "nn_NO" },
    { LANG_POLISH,         SUBLANG_NEUTRAL,                     "pl" },
    { LANG_POLISH,         SUBLANG_POLISH_POLAND,               "pl_PL" },
    { LANG_PORTUGUESE,     SUBLANG_NEUTRAL,                     "pt" },
    { LANG_PORTUGUESE,     SUBLANG_PORTUGUESE_BRAZILIAN,        "pt_BR" },
    { LANG_PORTUGUESE,     SUBLANG_PORTUGUESE_PORTUGAL,         "pt_PT" },
    { LANG_ROMANSH,        SUBLANG_NEUTRAL,                     "rm" },
    { LANG_ROMANSH,        SUBLANG_ROMANSH_SWITZERLAND,         "rm_CH" },
    { LANG_ROMANIAN,       SUBLANG_NEUTRAL,                     "ro" },
    { LANG_ROMANIAN,       SUBLANG_ROMANIAN_ROMANIA,            "ro_RO" },
    { LANG_RUSSIAN,        SUBLANG_NEUTRAL,                     "ru" },
    { LANG_RUSSIAN,        SUBLANG_RUSSIAN_RUSSIA,              "ru_RU" },
    { LANG_SERBIAN,        SUBLANG_NEUTRAL,                     "hr" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_CROATIA,             "hr_HR" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_LATIN,               "sr_RS@latin" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_CYRILLIC,            "sr_RS@cyrillic" },
    { LANG_SLOVAK,         SUBLANG_NEUTRAL,                     "sk" },
    { LANG_SLOVAK,         SUBLANG_SLOVAK_SLOVAKIA,             "sk_SK" },
    { LANG_ALBANIAN,       SUBLANG_NEUTRAL,                     "sq" },
    { LANG_ALBANIAN,       SUBLANG_ALBANIAN_ALBANIA,            "sq_AL" },
    { LANG_SWEDISH,        SUBLANG_NEUTRAL,                     "sv" },
    { LANG_SWEDISH,        SUBLANG_SWEDISH_SWEDEN,              "sv_SE" },
    { LANG_SWEDISH,        SUBLANG_SWEDISH_FINLAND,             "sv_FI" },
    { LANG_THAI,           SUBLANG_NEUTRAL,                     "th" },
    { LANG_THAI,           SUBLANG_THAI_THAILAND,               "th_TH" },
    { LANG_TURKISH,        SUBLANG_NEUTRAL,                     "tr" },
    { LANG_TURKISH,        SUBLANG_TURKISH_TURKEY,              "tr_TR" },
    { LANG_URDU,           SUBLANG_NEUTRAL,                     "ur" },
    { LANG_URDU,           SUBLANG_URDU_PAKISTAN,               "ur_PK" },
    { LANG_INDONESIAN,     SUBLANG_NEUTRAL,                     "id" },
    { LANG_INDONESIAN,     SUBLANG_INDONESIAN_INDONESIA,        "id_ID" },
    { LANG_UKRAINIAN,      SUBLANG_NEUTRAL,                     "uk" },
    { LANG_UKRAINIAN,      SUBLANG_UKRAINIAN_UKRAINE,           "uk_UA" },
    { LANG_BELARUSIAN,     SUBLANG_NEUTRAL,                     "be" },
    { LANG_BELARUSIAN,     SUBLANG_BELARUSIAN_BELARUS,          "be_BY" },
    { LANG_SLOVENIAN,      SUBLANG_NEUTRAL,                     "sl" },
    { LANG_SLOVENIAN,      SUBLANG_SLOVENIAN_SLOVENIA,          "sl_SI" },
    { LANG_ESTONIAN,       SUBLANG_NEUTRAL,                     "et" },
    { LANG_ESTONIAN,       SUBLANG_ESTONIAN_ESTONIA,            "et_EE" },
    { LANG_LATVIAN,        SUBLANG_NEUTRAL,                     "lv" },
    { LANG_LATVIAN,        SUBLANG_LATVIAN_LATVIA,              "lv_LV" },
    { LANG_LITHUANIAN,     SUBLANG_NEUTRAL,                     "lt" },
    { LANG_LITHUANIAN,     SUBLANG_LITHUANIAN_LITHUANIA,        "lt_LT" },
    { LANG_PERSIAN,        SUBLANG_NEUTRAL,                     "fa" },
    { LANG_PERSIAN,        SUBLANG_PERSIAN_IRAN,                "fa_IR" },
    { LANG_ARMENIAN,       SUBLANG_NEUTRAL,                     "hy" },
    { LANG_ARMENIAN,       SUBLANG_ARMENIAN_ARMENIA,            "hy_AM" },
    { LANG_AZERI,          SUBLANG_NEUTRAL,                     "az" },
    { LANG_AZERI,          SUBLANG_AZERI_LATIN,                 "az_AZ@latin" },
    { LANG_AZERI,          SUBLANG_AZERI_CYRILLIC,              "az_AZ@cyrillic" },
    { LANG_BASQUE,         SUBLANG_NEUTRAL,                     "eu" },
    { LANG_BASQUE,         SUBLANG_BASQUE_BASQUE,               "eu_ES" },
    { LANG_MACEDONIAN,     SUBLANG_NEUTRAL,                     "mk" },
    { LANG_MACEDONIAN,     SUBLANG_MACEDONIAN_MACEDONIA,        "mk_MK" },
    { LANG_AFRIKAANS,      SUBLANG_NEUTRAL,                     "af" },
    { LANG_AFRIKAANS,      SUBLANG_AFRIKAANS_SOUTH_AFRICA,      "af_ZA" },
    { LANG_GEORGIAN,       SUBLANG_NEUTRAL,                     "ka" },
    { LANG_GEORGIAN,       SUBLANG_GEORGIAN_GEORGIA,            "ka_GE" },
    { LANG_FAEROESE,       SUBLANG_NEUTRAL,                     "fo" },
    { LANG_FAEROESE,       SUBLANG_FAEROESE_FAROE_ISLANDS,      "fo_FO" },
    { LANG_HINDI,          SUBLANG_NEUTRAL,                     "hi" },
    { LANG_HINDI,          SUBLANG_HINDI_INDIA,                 "hi_IN" },
    { LANG_MALAY,          SUBLANG_NEUTRAL,                     "ms" },
    { LANG_MALAY,          SUBLANG_MALAY_MALAYSIA,              "ms_MY" },
    { LANG_MALAY,          SUBLANG_MALAY_BRUNEI_DARUSSALAM,     "ms_BN" },
    { LANG_KAZAK,          SUBLANG_NEUTRAL,                     "kk" },
    { LANG_KAZAK,          SUBLANG_KAZAK_KAZAKHSTAN,            "kk_KZ" },
    { LANG_KYRGYZ,         SUBLANG_NEUTRAL,                     "ky" },
    { LANG_KYRGYZ,         SUBLANG_KYRGYZ_KYRGYZSTAN,           "ky_KG" },
    { LANG_SWAHILI,        SUBLANG_NEUTRAL,                     "sw" },
    { LANG_SWAHILI,        SUBLANG_SWAHILI_KENYA,               "sw_KE" },
    { LANG_UZBEK,          SUBLANG_NEUTRAL,                     "uz" },
    { LANG_UZBEK,          SUBLANG_UZBEK_LATIN,                 "uz_UZ@latin" },
    { LANG_UZBEK,          SUBLANG_UZBEK_CYRILLIC,              "uz_UZ@cyrillic" },
    { LANG_TATAR,          SUBLANG_NEUTRAL,                     "tt" },
    { LANG_TATAR,          SUBLANG_TATAR_RUSSIA,                "tt_TA" },
    { LANG_PUNJABI,        SUBLANG_NEUTRAL,                     "pa" },
    { LANG_PUNJABI,        SUBLANG_PUNJABI_INDIA,               "pa_IN" },
    { LANG_GUJARATI,       SUBLANG_NEUTRAL,                     "gu" },
    { LANG_GUJARATI,       SUBLANG_GUJARATI_INDIA,              "gu_IN" },
    { LANG_ORIYA,          SUBLANG_NEUTRAL,                     "or" },
    { LANG_ORIYA,          SUBLANG_ORIYA_INDIA,                 "or_IN" },
    { LANG_TAMIL,          SUBLANG_NEUTRAL,                     "ta" },
    { LANG_TAMIL,          SUBLANG_TAMIL_INDIA,                 "ta_IN" },
    { LANG_TELUGU,         SUBLANG_NEUTRAL,                     "te" },
    { LANG_TELUGU,         SUBLANG_TELUGU_INDIA,                "te_IN" },
    { LANG_KANNADA,        SUBLANG_NEUTRAL,                     "kn" },
    { LANG_KANNADA,        SUBLANG_KANNADA_INDIA,               "kn_IN" },
    { LANG_MALAYALAM,      SUBLANG_NEUTRAL,                     "ml" },
    { LANG_MALAYALAM,      SUBLANG_MALAYALAM_INDIA,             "ml_IN" },
    { LANG_MARATHI,        SUBLANG_NEUTRAL,                     "mr" },
    { LANG_MARATHI,        SUBLANG_MARATHI_INDIA,               "mr_IN" },
    { LANG_SANSKRIT,       SUBLANG_NEUTRAL,                     "sa" },
    { LANG_SANSKRIT,       SUBLANG_SANSKRIT_INDIA,              "sa_IN" },
    { LANG_MONGOLIAN,      SUBLANG_NEUTRAL,                     "mn" },
    { LANG_MONGOLIAN,      SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA, "mn_MN" },
    { LANG_WELSH,          SUBLANG_NEUTRAL,                     "cy" },
    { LANG_WELSH,          SUBLANG_WELSH_UNITED_KINGDOM,        "cy_GB" },
    { LANG_GALICIAN,       SUBLANG_NEUTRAL,                     "gl" },
    { LANG_GALICIAN,       SUBLANG_GALICIAN_GALICIAN,           "gl_ES" },
    { LANG_KONKANI,        SUBLANG_NEUTRAL,                     "kok" },
    { LANG_KONKANI,        SUBLANG_KONKANI_INDIA,               "kok_IN" },
    { LANG_DIVEHI,         SUBLANG_NEUTRAL,                     "dv" },
    { LANG_DIVEHI,         SUBLANG_DIVEHI_MALDIVES,             "dv_MV" },
    { LANG_BRETON,         SUBLANG_NEUTRAL,                     "br" },
    { LANG_BRETON,         SUBLANG_BRETON_FRANCE,               "br_FR" },

#ifdef LANG_ESPERANTO
    { LANG_ESPERANTO,      SUBLANG_DEFAULT,                     "eo" },
#endif
#ifdef LANG_WALON
    { LANG_WALON,          SUBLANG_NEUTRAL,                     "wa" },
    { LANG_WALON,          SUBLANG_DEFAULT,                     "wa_BE" },
#endif
#ifdef LANG_CORNISH
    { LANG_CORNISH,        SUBLANG_NEUTRAL,                     "kw" },
    { LANG_CORNISH,        SUBLANG_DEFAULT,                     "kw_GB" },
#endif
#ifdef LANG_GAELIC
    { LANG_GAELIC,         SUBLANG_NEUTRAL,                     "ga" },
    { LANG_GAELIC,         SUBLANG_GAELIC,                      "ga_IE" },
    { LANG_GAELIC,         SUBLANG_GAELIC_SCOTTISH,             "gd_GB" },
    { LANG_GAELIC,         SUBLANG_GAELIC_MANX,                 "gv_GB" },
#endif
};

#ifndef HAVE_LIBGETTEXTPO

typedef void *po_file_t;

static const char *get_msgstr( po_file_t po, const char *msgid, const char *context, int *found )
{
    if (context) (*found)++;
    return msgid;
}

static po_file_t read_po_file( const char *name )
{
    return NULL;
}

static void po_file_free ( po_file_t po )
{
}

void write_pot_file( const char *outname )
{
    error( "PO files not supported in this wrc build\n" );
}

void write_po_files( const char *outname )
{
    error( "PO files not supported in this wrc build\n" );
}

#else  /* HAVE_LIBGETTEXTPO */

static void po_xerror( int severity, po_message_t message,
                       const char *filename, size_t lineno, size_t column,
                       int multiline_p, const char *message_text )
{
    fprintf( stderr, "%s:%u:%u: %s\n",
             filename, (unsigned int)lineno, (unsigned int)column, message_text );
    if (severity) exit(1);
}

static void po_xerror2( int severity, po_message_t message1,
                        const char *filename1, size_t lineno1, size_t column1,
                        int multiline_p1, const char *message_text1,
                        po_message_t message2,
                        const char *filename2, size_t lineno2, size_t column2,
                        int multiline_p2, const char *message_text2 )
{
    fprintf( stderr, "%s:%u:%u: %s\n",
             filename1, (unsigned int)lineno1, (unsigned int)column1, message_text1 );
    fprintf( stderr, "%s:%u:%u: %s\n",
             filename2, (unsigned int)lineno2, (unsigned int)column2, message_text2 );
    if (severity) exit(1);
}

static const struct po_xerror_handler po_xerror_handler = { po_xerror, po_xerror2 };

static char *convert_string_utf8( const string_t *str, int codepage )
{
    string_t *newstr = convert_string( str, str_unicode, codepage );
    char *buffer = xmalloc( newstr->size * 4 + 1 );
    int len = wine_utf8_wcstombs( 0, newstr->str.wstr, newstr->size, buffer, newstr->size * 4 );
    buffer[len] = 0;
    free_string( newstr );
    return buffer;
}

static po_message_t find_message( po_file_t po, const char *msgid, const char *msgctxt,
                                  po_message_iterator_t *iterator )
{
    po_message_t msg;
    const char *context;

    *iterator = po_message_iterator( po, NULL );
    while ((msg = po_next_message( *iterator )))
    {
        if (strcmp( po_message_msgid( msg ), msgid )) continue;
        if (!msgctxt) break;
        if (!(context = po_message_msgctxt( msg ))) continue;
        if (!strcmp( context, msgctxt )) break;
    }
    return msg;
}

static const char *get_msgstr( po_file_t po, const char *msgid, const char *context, int *found )
{
    const char *ret = msgid;
    po_message_t msg;
    po_message_iterator_t iterator;

    msg = find_message( po, msgid, context, &iterator );
    if (msg && !po_message_is_fuzzy( msg ))
    {
        ret = po_message_msgstr( msg );
        if (!ret[0]) ret = msgid;  /* ignore empty strings */
        else (*found)++;
    }
    po_message_iterator_free( iterator );
    return ret;
}

static po_file_t read_po_file( const char *name )
{
    po_file_t po;

    if (!(po = po_file_read( name, &po_xerror_handler )))
        error( "cannot load po file '%s'\n", name );
    return po;
}

static void add_po_string( po_file_t po, const string_t *msgid, const string_t *msgstr,
                           const language_t *lang )
{
    po_message_t msg;
    po_message_iterator_t iterator;
    int codepage;
    char *id, *id_buffer, *context, *str = NULL, *str_buffer = NULL;

    if (!msgid->size) return;

    id_buffer = id = convert_msgid_ascii( msgid, 1 );
    context = get_message_context( &id );

    if (msgstr)
    {
        if (lang) codepage = get_language_codepage( lang->id, lang->sub );
        else codepage = get_language_codepage( 0, 0 );
        assert( codepage != -1 );
        str_buffer = str = convert_string_utf8( msgstr, codepage );
        if (is_english( lang )) get_message_context( &str );
    }
    if (!(msg = find_message( po, id, context, &iterator )))
    {
        msg = po_message_create();
        po_message_set_msgid( msg, id );
        po_message_set_msgstr( msg, str ? str : "" );
        if (context) po_message_set_msgctxt( msg, context );
        po_message_insert( iterator, msg );
    }
    if (msgid->loc.file) po_message_add_filepos( msg, msgid->loc.file, msgid->loc.line );
    po_message_iterator_free( iterator );
    free( id_buffer );
    free( str_buffer );
}

struct po_file_lang
{
    struct list entry;
    language_t  lang;
    po_file_t   po;
};

static struct list po_file_langs = LIST_INIT( po_file_langs );

static po_file_t create_po_file(void)
{
    po_file_t po;
    po_message_t msg;
    po_message_iterator_t iterator;

    po = po_file_create();
    iterator = po_message_iterator( po, NULL );
    msg = po_message_create();
    po_message_set_msgid( msg, "" );
    po_message_set_msgstr( msg,
                           "Project-Id-Version: Wine\n"
                           "Report-Msgid-Bugs-To: http://bugs.winehq.org\n"
                           "POT-Creation-Date: N/A\n"
                           "PO-Revision-Date: N/A\n"
                           "Last-Translator: Automatically generated\n"
                           "Language-Team: none\n"
                           "MIME-Version: 1.0\n"
                           "Content-Type: text/plain; charset=UTF-8\n"
                           "Content-Transfer-Encoding: 8bit\n" );
    po_message_insert( iterator, msg );
    po_message_iterator_free( iterator );
    return po;
}

static po_file_t get_po_file( const language_t *lang )
{
    struct po_file_lang *po_file;

    LIST_FOR_EACH_ENTRY( po_file, &po_file_langs, struct po_file_lang, entry )
        if (po_file->lang.id == lang->id && po_file->lang.sub == lang->sub) return po_file->po;

    /* create a new one */
    po_file = xmalloc( sizeof(*po_file) );
    po_file->lang = *lang;
    po_file->po = create_po_file();
    list_add_tail( &po_file_langs, &po_file->entry );
    return po_file->po;
}

static char *get_po_file_name( const language_t *lang )
{
    unsigned int i;
    char name[40];

    sprintf( name, "%02x-%02x", lang->id, lang->sub );
    for (i = 0; i < sizeof(languages)/sizeof(languages[0]); i++)
    {
        if (languages[i].id == lang->id && languages[i].sub == lang->sub)
        {
            strcpy( name, languages[i].name );
            break;
        }
    }
    strcat( name, ".po" );
    return xstrdup( name );
}

static unsigned int flush_po_files( const char *output_name )
{
    struct po_file_lang *po_file, *next;
    unsigned int count = 0;

    LIST_FOR_EACH_ENTRY_SAFE( po_file, next, &po_file_langs, struct po_file_lang, entry )
    {
        char *name = get_po_file_name( &po_file->lang );
        if (output_name)
        {
            const char *p = strrchr( output_name, '/' );
            if (p) p++;
            else p = output_name;
            if (!strcmp( p, name ))
            {
                po_file_write( po_file->po, name, &po_xerror_handler );
                count++;
            }
        }
        else  /* no specified output name, output a file for every language found */
        {
            po_file_write( po_file->po, name, &po_xerror_handler );
            count++;
            fprintf( stderr, "created %s\n", name );
        }
        po_file_free( po_file->po );
        list_remove( &po_file->entry );
        free( po_file );
        free( name );
    }
    return count;
}

static void add_pot_stringtable( po_file_t po, const resource_t *res )
{
    const stringtable_t *stt = res->res.stt;
    int i;

    while (stt)
    {
        for (i = 0; i < stt->nentries; i++)
            if (stt->entries[i].str) add_po_string( po, stt->entries[i].str, NULL, NULL );
        stt = stt->next;
    }
}

static void add_po_stringtable( const resource_t *english, const resource_t *res )
{
    const stringtable_t *english_stt = english->res.stt;
    const stringtable_t *stt = res->res.stt;
    po_file_t po = get_po_file( stt->lvc.language );
    int i;

    while (english_stt && stt)
    {
        for (i = 0; i < stt->nentries; i++)
            if (english_stt->entries[i].str && stt->entries[i].str)
                add_po_string( po, english_stt->entries[i].str, stt->entries[i].str, stt->lvc.language );
        stt = stt->next;
        english_stt = english_stt->next;
    }
}

static void add_pot_dialog_controls( po_file_t po, const control_t *ctrl )
{
    while (ctrl)
    {
        if (control_has_title( ctrl )) add_po_string( po, ctrl->title->name.s_name, NULL, NULL );
        ctrl = ctrl->next;
    }
}

static void add_pot_dialog( po_file_t po, const resource_t *res )
{
    const dialog_t *dlg = res->res.dlg;

    if (dlg->title) add_po_string( po, dlg->title, NULL, NULL );
    if (dlg->font) add_po_string( po, dlg->font->name, NULL, NULL );
    add_pot_dialog_controls( po, dlg->controls );
}

static void add_po_dialog_controls( po_file_t po, const control_t *english_ctrl,
                                    const control_t *ctrl, const language_t *lang )
{
    while (english_ctrl && ctrl)
    {
        if (control_has_title( english_ctrl ) && control_has_title( ctrl ))
            add_po_string( po, english_ctrl->title->name.s_name, ctrl->title->name.s_name, lang );

        ctrl = ctrl->next;
        english_ctrl = english_ctrl->next;
    }
}

static void add_po_dialog( const resource_t *english, const resource_t *res )
{
    const dialog_t *english_dlg = english->res.dlg;
    const dialog_t *dlg = res->res.dlg;
    po_file_t po = get_po_file( dlg->lvc.language );

    if (english_dlg->title && dlg->title)
        add_po_string( po, english_dlg->title, dlg->title, dlg->lvc.language );
    if (english_dlg->font && dlg->font)
        add_po_string( po, english_dlg->font->name, dlg->font->name, dlg->lvc.language );
    add_po_dialog_controls( po, english_dlg->controls, dlg->controls, dlg->lvc.language );
}

static void add_pot_menu_items( po_file_t po, const menu_item_t *item )
{
    while (item)
    {
        if (item->name) add_po_string( po, item->name, NULL, NULL );
        if (item->popup) add_pot_menu_items( po, item->popup );
        item = item->next;
    }
}

static void add_pot_menu( po_file_t po, const resource_t *res )
{
    add_pot_menu_items( po, res->res.men->items );
}

static void add_po_menu_items( po_file_t po, const menu_item_t *english_item,
                               const menu_item_t *item, const language_t *lang )
{
    while (english_item && item)
    {
        if (english_item->name && item->name)
            add_po_string( po, english_item->name, item->name, lang );
        if (english_item->popup && item->popup)
            add_po_menu_items( po, english_item->popup, item->popup, lang );
        item = item->next;
        english_item = english_item->next;
    }
}

static void add_po_menu( const resource_t *english, const resource_t *res )
{
    const menu_item_t *english_items = english->res.men->items;
    const menu_item_t *items = res->res.men->items;
    po_file_t po = get_po_file( res->res.men->lvc.language );

    add_po_menu_items( po, english_items, items, res->res.men->lvc.language );
}

static resource_t *find_english_resource( resource_t *res )
{
    resource_t *ptr;

    for (ptr = resource_top; ptr; ptr = ptr->next)
    {
        if (ptr->type != res->type) continue;
        if (!ptr->lan) continue;
        if (!is_english( ptr->lan )) continue;
        if (compare_name_id( ptr->name, res->name )) continue;
        return ptr;
    }
    return NULL;
}

void write_pot_file( const char *outname )
{
    resource_t *res;
    po_file_t po = create_po_file();

    for (res = resource_top; res; res = res->next)
    {
        if (!is_english( res->lan )) continue;

        switch (res->type)
        {
        case res_acc: break;  /* FIXME */
        case res_dlg: add_pot_dialog( po, res ); break;
        case res_men: add_pot_menu( po, res ); break;
        case res_stt: add_pot_stringtable( po, res ); break;
        case res_msg: break;  /* FIXME */
        default: break;
        }
    }
    po_file_write( po, outname, &po_xerror_handler );
    po_file_free( po );
}

void write_po_files( const char *outname )
{
    resource_t *res, *english;

    for (res = resource_top; res; res = res->next)
    {
        if (!(english = find_english_resource( res ))) continue;
        switch (res->type)
        {
        case res_acc: break;  /* FIXME */
        case res_dlg: add_po_dialog( english, res ); break;
        case res_men: add_po_menu( english, res ); break;
        case res_stt: add_po_stringtable( english, res ); break;
        case res_msg: break;  /* FIXME */
        default: break;
        }
    }
    if (!flush_po_files( outname ))
    {
        if (outname) error( "No translations found for %s\n", outname );
        else error( "No translations found\n" );
    }
}

#endif  /* HAVE_LIBGETTEXTPO */

static string_t *translate_string( po_file_t po, string_t *str, int *found )
{
    string_t *new;
    const char *transl;
    int res;
    char *buffer, *msgid, *context;

    if (!str->size || !(buffer = convert_msgid_ascii( str, 0 )))
        return convert_string( str, str_unicode, 1252 );

    msgid = buffer;
    context = get_message_context( &msgid );
    transl = get_msgstr( po, msgid, context, found );

    new = xmalloc( sizeof(*new) );
    new->type = str_unicode;
    new->size = wine_utf8_mbstowcs( 0, transl, strlen(transl), NULL, 0 );
    new->str.wstr = xmalloc( (new->size+1) * sizeof(WCHAR) );
    res = wine_utf8_mbstowcs( MB_ERR_INVALID_CHARS, transl, strlen(transl), new->str.wstr, new->size );
    if (res == -2)
        error( "Invalid utf-8 character in string '%s'\n", transl );
    new->str.wstr[new->size] = 0;
    free( buffer );
    return new;
}

static control_t *translate_controls( po_file_t po, control_t *ctrl, int *found )
{
    control_t *new, *head = NULL, *tail = NULL;

    while (ctrl)
    {
        new = xmalloc( sizeof(*new) );
        *new = *ctrl;
        if (control_has_title( ctrl ))
        {
            new->title = new_name_id();
            *new->title = *ctrl->title;
            new->title->name.s_name = translate_string( po, ctrl->title->name.s_name, found );
        }
        else new->title = dup_name_id( ctrl->title );
        new->ctlclass = dup_name_id( ctrl->ctlclass );
        if (tail) tail->next = new;
        else head = new;
        new->next = NULL;
        new->prev = tail;
        tail = new;
        ctrl = ctrl->next;
    }
    return head;
}

static menu_item_t *translate_items( po_file_t po, menu_item_t *item, int *found )
{
    menu_item_t *new, *head = NULL, *tail = NULL;

    while (item)
    {
        new = xmalloc( sizeof(*new) );
        *new = *item;
        if (item->name) new->name = translate_string( po, item->name, found );
        if (item->popup) new->popup = translate_items( po, item->popup, found );
        if (tail) tail->next = new;
        else head = new;
        new->next = NULL;
        new->prev = tail;
        tail = new;
        item = item->next;
    }
    return head;
}

static stringtable_t *translate_stringtable( po_file_t po, stringtable_t *stt,
                                             language_t *lang, int *found )
{
    stringtable_t *new, *head = NULL, *tail = NULL;
    int i;

    while (stt)
    {
        new = xmalloc( sizeof(*new) );
        *new = *stt;
        new->lvc.language = lang;
        new->lvc.version = get_dup_version( lang );
        new->entries = xmalloc( new->nentries * sizeof(*new->entries) );
        memcpy( new->entries, stt->entries, new->nentries * sizeof(*new->entries) );
        for (i = 0; i < stt->nentries; i++)
            if (stt->entries[i].str)
                new->entries[i].str = translate_string( po, stt->entries[i].str, found );

        if (tail) tail->next = new;
        else head = new;
        new->next = NULL;
        new->prev = tail;
        tail = new;
        stt = stt->next;
    }
    return head;
}

static void translate_dialog( po_file_t po, dialog_t *dlg, dialog_t *new, int *found )
{
    if (dlg->title) new->title = translate_string( po, dlg->title, found );
    if (dlg->font)
    {
        new->font = xmalloc( sizeof(*dlg->font) );
        new->font = dlg->font;
        new->font->name = translate_string( po, dlg->font->name, found );
    }
    new->controls = translate_controls( po, dlg->controls, found );
}

static void translate_resources( po_file_t po, language_t *lang )
{
    resource_t *res;

    for (res = resource_top; res; res = res->next)
    {
        resource_t *new = NULL;
        int found = 0;

        if (!is_english( res->lan )) continue;

        switch (res->type)
        {
        case res_acc:
            /* FIXME */
            break;
        case res_dlg:
            new = dup_resource( res, lang );
            translate_dialog( po, res->res.dlg, new->res.dlg, &found );
            break;
        case res_men:
            new = dup_resource( res, lang );
            new->res.men->items = translate_items( po, res->res.men->items, &found );
            break;
        case res_stt:
            new = dup_resource( res, lang );
            new->res.stt = translate_stringtable( po, res->res.stt, lang, &found );
            break;
        case res_msg:
            /* FIXME */
            break;
        default:
            break;
        }

        if (new && found)
        {
            if (new_tail) new_tail->next = new;
            else new_top = new;
            new->prev = new_tail;
            new_tail = new;
        }
    }
}

void add_translations( const char *po_dir )
{
    resource_t *res;
    po_file_t po;
    char buffer[256];
    char *p, *tok, *name;
    unsigned int i;
    FILE *f;

    /* first check if we have English resources to translate */
    for (res = resource_top; res; res = res->next) if (is_english( res->lan )) break;
    if (!res) return;

    new_top = new_tail = NULL;

    name = strmake( "%s/LINGUAS", po_dir );
    if (!(f = fopen( name, "r" )))
    {
        free( name );
        return;
    }
    free( name );
    while (fgets( buffer, sizeof(buffer), f ))
    {
        if ((p = strchr( buffer, '#' ))) *p = 0;
        for (tok = strtok( buffer, " \t\r\n" ); tok; tok = strtok( NULL, " \t\r\n" ))
        {
            for (i = 0; i < sizeof(languages)/sizeof(languages[0]); i++)
                if (!strcmp( tok, languages[i].name )) break;

            if (i == sizeof(languages)/sizeof(languages[0]))
                error( "unknown language '%s'\n", tok );

            name = strmake( "%s/%s.po", po_dir, tok );
            po = read_po_file( name );
            translate_resources( po, new_language(languages[i].id, languages[i].sub) );
            po_file_free( po );
            free( name );
        }
    }
    fclose( f );

    /* prepend the translated resources to the global list */
    if (new_tail)
    {
        new_tail->next = resource_top;
        resource_top->prev = new_tail;
        resource_top = new_top;
    }
}
