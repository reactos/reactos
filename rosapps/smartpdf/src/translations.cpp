#include "base_util.h"
#include "translations.h"
#include "translations_txt.h"
#include "str_util.h"

/*
This code relies on the following variables that must be defined in a 
separate file (translations_txt.h and translations_txt.c).
The idea is that those files are automatically generated 
by a script from translations file.

// number of languages we support
int g_transLangsCount;

// array of language names so that g_transLangs[i] is a name of
// language. i is 0..g_transLangsCount-1
const char **g_transLangs;

// total number of translated strings
int g_transTranslationsCount;

// array of translated strings. 
// it has g_transLangsCount * g_translationsCount elements
// (for simplicity). Translation i for language n is at position
// (n * g_transTranslationsCount) + i
const char **g_transTranslations;
*/

// numeric index of the current language. 0 ... g_transLangsCount-1
static int currLangIdx = 0;

/* 'data'/'data_len' is a text describing all texts we translate.
   It builds data structures need for quick lookup of translations
   as well as a list of available languages.
   It returns a list of available languages.
   The list is not valid after a call to Translations_FreeData.
   The function must be called before any other function in this module.
   It can be called multiple times. This is to make debugging of translations
   easier by allowing re-loading translation file at runtime. 
   */
bool Translations_FromData(const char* langs, const char* data, size_t data_len)
{
    assert(0); // TODO: implement me
    return false;
}

bool Translations_SetCurrentLanguage(const char* lang)
{
    for (int i=0; i < g_transLangsCount; i++) {
        if (str_eq(lang, g_transLangs[i])) {
            currLangIdx = i;
            return true;
        }
    }
    return false;
}

const char* Translatations_GetTranslation(const char* txt)
{
    // perf shortcut: don't bother translating if we use default lanuage
    if (0 == currLangIdx)
        return txt;
    for (int i=0; i < g_transTranslationsCount; i++) {
        // TODO: translations are sorted so can use binary search
        const char *tmp =  g_transTranslations[i];
        int cmp_res = strcmp(txt, tmp);
        if (0 == cmp_res) {
            tmp = g_transTranslations[(currLangIdx * g_transTranslationsCount) + i];
            if (NULL == tmp)
                return txt;
            return tmp;
        } else if (cmp_res < 0) {
            assert(0);  // bad - didn't find a translation
            return txt;
        }
    }
    assert(0); // bad - didn't find a translation
    return txt;
}

static WCHAR* lastTxtCached = NULL;

#define UTF8_END   -1
#define UTF8_ERROR -2

typedef struct json_utf8_decode
{
    int the_index;
    const char *the_input;
    int the_length;
    int the_char;
    int the_byte;
} json_utf8_decode;


void
utf8_decode_init(json_utf8_decode *utf8, const char *p, int length)
{
    utf8->the_index = 0;
    utf8->the_input = p;
    utf8->the_length = length;
    utf8->the_char = 0;
    utf8->the_byte = 0;
}

static int
get(json_utf8_decode *utf8)
{
    int c;
    if (utf8->the_index >= utf8->the_length) {
        return UTF8_END;
    }
    c = utf8->the_input[utf8->the_index] & 0xFF;
    utf8->the_index += 1;
    return c;
}

static int
cont(json_utf8_decode *utf8)
{
    int c = get(utf8);
    return ((c & 0xC0) == 0x80) ? (c & 0x3F) : UTF8_ERROR;
}

int
utf8_decode_next(json_utf8_decode *utf8)
{
    int c;  /* the first byte of the character */
    int r;  /* the result */

    if (utf8->the_index >= utf8->the_length) {
        return utf8->the_index == utf8->the_length ? UTF8_END : UTF8_ERROR;
    }
    utf8->the_byte = utf8->the_index;
    utf8->the_char += 1;
    c = get(utf8);
/*
    Zero continuation (0 to 127)
*/
    if ((c & 0x80) == 0) {
        return c;
    }
/*
    One contination (128 to 2047)
*/
    if ((c & 0xE0) == 0xC0) {
        int c1 = cont(utf8);
        if (c1 < 0) {
            return UTF8_ERROR;
        }
        r = ((c & 0x1F) << 6) | c1;
        return r >= 128 ? r : UTF8_ERROR;
    }
/*
    Two continuation (2048 to 55295 and 57344 to 65535)
*/
    if ((c & 0xF0) == 0xE0) {
        int c1 = cont(utf8);
        int c2 = cont(utf8);
        if (c1 < 0 || c2 < 0) {
            return UTF8_ERROR;
        }
        r = ((c & 0x0F) << 12) | (c1 << 6) | c2;
        return r >= 2048 && (r < 55296 || r > 57343) ? r : UTF8_ERROR;
    }
/*
    Three continuation (65536 to 1114111)
*/
    if ((c & 0xF1) == 0xF0) {
        int c1 = cont(utf8);
        int c2 = cont(utf8);
        int c3 = cont(utf8);
        if (c1 < 0 || c2 < 0 || c3 < 0) {
            return UTF8_ERROR;
        }
        r = ((c & 0x0F) << 18) | (c1 << 12) | (c2 << 6) | c3;
        return r >= 65536 && r <= 1114111 ? r : UTF8_ERROR;
    }
    return UTF8_ERROR;
}

static int json_utf8_to_utf16(unsigned short *w, const char *p, int length)
{
    int c;
    int the_index = 0;
    json_utf8_decode utf8;

    utf8_decode_init(&utf8, p, length);
    for (;;) {
        c = utf8_decode_next(&utf8);
        if (c < 0) {
            return UTF8_END ? the_index : UTF8_ERROR;
        }
        if (c < 0x10000) {
            w[the_index] = (unsigned short)c;
            the_index += 1;
        } else {
            c &= 0xFFFF;
            w[the_index] = (unsigned short)(0xD800 | (c >> 10));
            the_index += 1;
            w[the_index] = (unsigned short)(0xDC00 | (c & 0x3FF));
            the_index += 1;
        }
    }
}

WCHAR* utf8_to_utf16(const char *txt)
{
    // TODO: this is not correct, need real conversion
    size_t len = strlen(txt);
    WCHAR* wstr = (WCHAR*)zmalloc((len+1) * sizeof(WCHAR));
    if (!wstr) return NULL;
    int res = json_utf8_to_utf16((unsigned short*)wstr, txt, len);
    assert(UTF8_ERROR != res);
    return wstr;
}

// TODO: this is not thread-safe. lastTxtCached should be per-thread
const WCHAR* Translatations_GetTranslationW(const char* txt)
{
    txt = Translatations_GetTranslation(txt);
    if (!txt) return NULL;
    if (lastTxtCached)
        free(lastTxtCached);
    lastTxtCached = utf8_to_utf16(txt);
    return (const WCHAR*)lastTxtCached;
}

void Translations_FreeData()
{
    // TODO: will be more when we implement Translations_FromData
    free(lastTxtCached);
    lastTxtCached = NULL;
}

