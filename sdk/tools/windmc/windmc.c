/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS windmc
 * FILE:            tools/windmc/windmc.c
 * PURPOSE:         Binutils-compatible Windows Message Compiler host tool
 * PROGRAMMER:      ReactOS contributors
 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#include <locale.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define strdup _strdup
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include <strings.h>
#include <iconv.h>
#endif

#define MAX_LANGS 32
#define MAX_SEVERITIES 16
#define MAX_FACILITIES 32
#define MAX_LINE 4096

/* ========== Language info table ========== */

typedef struct {
    uint16_t lang_id, primary, sub;
    int code_page;
    const char *country, *lang_name;
} LangInfo;

static const LangInfo known_langs[] = {
    { 0x0401, 0x01, 0x1, 1256, "Saudi Arabia",    "Arabic" },
    { 0x0402, 0x02, 0x1, 1251, "Bulgaria",        "Bulgarian" },
    { 0x0403, 0x03, 0x1, 1252, "Spain",           "Catalan" },
    { 0x0404, 0x04, 0x1,  950, "Taiwan",          "Chinese" },
    { 0x0405, 0x05, 0x1, 1250, "Czech Republic",  "Czech" },
    { 0x0406, 0x06, 0x1, 1252, "Denmark",         "Danish" },
    { 0x0407, 0x07, 0x1, 1252, "Germany",         "German" },
    { 0x0408, 0x08, 0x1, 1253, "Greece",          "Greek" },
    { 0x0409, 0x09, 0x1, 1252, "United States",   "English" },
    { 0x040A, 0x0A, 0x1, 1252, "Spain",           "Spanish - Traditional Sort" },
    { 0x040B, 0x0B, 0x1, 1252, "Finland",         "Finnish" },
    { 0x040C, 0x0C, 0x1, 1252, "France",          "French" },
    { 0x040D, 0x0D, 0x1, 1255, "Israel",          "Hebrew" },
    { 0x040E, 0x0E, 0x1, 1250, "Hungary",         "Hungarian" },
    { 0x040F, 0x0F, 0x1, 1252, "Iceland",         "Icelandic" },
    { 0x0410, 0x10, 0x1, 1252, "Italy",           "Italian" },
    { 0x0411, 0x11, 0x1,  932, "Japan",           "Japanese" },
    { 0x0412, 0x12, 0x1,  949, "Korea",           "Korean" },
    { 0x0413, 0x13, 0x1, 1252, "Netherlands",     "Dutch" },
    { 0x0414, 0x14, 0x1, 1252, "Norway",          "Norwegian (Bokmal)" },
    { 0x0415, 0x15, 0x1, 1250, "Poland",          "Polish" },
    { 0x0416, 0x16, 0x1, 1252, "Brazil",          "Portuguese (Brazil)" },
    { 0x0418, 0x18, 0x1, 1250, "Romania",         "Romanian" },
    { 0x0419, 0x19, 0x1, 1251, "Russia",          "Russian" },
    { 0x041A, 0x1A, 0x1, 1250, "Croatia",         "Croatian" },
    { 0x041B, 0x1B, 0x1, 1250, "Slovakia",        "Slovak" },
    { 0x041C, 0x1C, 0x1, 1250, "Albania",         "Albanian" },
    { 0x041D, 0x1D, 0x1, 1252, "Sweden",          "Swedish" },
    { 0x041E, 0x1E, 0x1,  874, "Thailand",        "Thai" },
    { 0x041F, 0x1F, 0x1, 1254, "Turkey",          "Turkish" },
    { 0x0420, 0x20, 0x1, 1256, "Pakistan",        "Urdu" },
    { 0x0421, 0x21, 0x1, 1252, "Indonesia",       "Indonesian" },
    { 0x0422, 0x22, 0x1, 1251, "Ukraine",         "Ukrainian" },
    { 0x0424, 0x24, 0x1, 1250, "Slovenia",        "Slovenian" },
    { 0x0425, 0x25, 0x1, 1257, "Estonia",         "Estonian" },
    { 0x0426, 0x26, 0x1, 1257, "Latvia",          "Latvian" },
    { 0x0427, 0x27, 0x1, 1257, "Lithuania",       "Lithuanian" },
    { 0x0804, 0x04, 0x2,  936, "People's republic of China", "Chinese (People's republic of China)" },
    { 0x0816, 0x16, 0x2, 1252, "Portugal",        "Portuguese" },
    { 0x0C0A, 0x0A, 0x3, 1252, "Spain",           "Spanish - Modern Sort" },
    { 0,      0,    0,   0,    NULL,               NULL }
};

static const LangInfo *find_lang_info(uint16_t lang_id) {
    for (int i = 0; known_langs[i].lang_name; i++)
        if (known_langs[i].lang_id == lang_id) return &known_langs[i];
    return NULL;
}

/* ========== Data structures ========== */

typedef struct { char name[64]; uint32_t value; char define_name[128]; } SeverityEntry;
typedef struct { char name[64]; uint32_t value; char define_name[128]; } FacilityEntry;
typedef struct { char name[64]; uint16_t lang_id; char filename[256]; } LangEntry;
typedef struct { int lang_index; char *text; } MsgLangText;
typedef struct { char *text; char newline[3]; size_t newline_len; } InputLine;
typedef struct { uint32_t value; const char *text; } BinMsg;

typedef struct {
    uint32_t id, severity, facility, value;
    char symbolic_name[256];
    char type_def[64];
    MsgLangText lang_texts[MAX_LANGS];
    int num_lang_texts;
} Message;

typedef struct { int is_comment; int index; } HeaderItem;

/* ========== Globals ========== */

static SeverityEntry severities[MAX_SEVERITIES];
static int num_severities = 0;
static int custom_facilities = 0;

static FacilityEntry facilities[MAX_FACILITIES];
static int num_facilities = 0;

static LangEntry langs[MAX_LANGS];
static int num_langs = 0;

static Message *messages = NULL;
static int num_messages = 0, cap_messages = 0;

static char **comment_strs = NULL;
static int *comment_block_ids = NULL;
static int num_comments = 0, cap_comments = 0;

static HeaderItem *header_items = NULL;
static int num_header_items = 0, cap_header_items = 0;

static char current_typedef[64] = "";

/* Lines read from file */
static InputLine *lines = NULL;
static int num_lines = 0, cap_lines = 0;
static int input_codepage = 1252;
static int input_is_utf8 = 0;
static int output_unicode = 1;
static int use_bin_prefix = 0;
static char output_base_name[256] = "";

/* ========== Memory helpers ========== */

static void *xrealloc(void *ptr, size_t size) {
    void *result = realloc(ptr, size);
    if (!result) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return result;
}

static char *xstrdup(const char *s) {
    char *result = strdup(s);
    if (!result) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return result;
}

static char *xmalloc(size_t size) {
    char *result = malloc(size);
    if (!result) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return result;
}

static char *make_output_path(const char *dir, const char *base, const char *lang_file, const char *suffix) {
    size_t len = strlen(dir) + 1 + strlen(base) + strlen(suffix) + 1;
    if (lang_file)
        len += 1 + strlen(lang_file);

    char *path = xmalloc(len);
    if (lang_file)
        snprintf(path, len, "%s/%s_%s%s", dir, base, lang_file, suffix);
    else
        snprintf(path, len, "%s/%s%s", dir, base, suffix);
    return path;
}

static void make_dir(const char *path) {
#ifdef _WIN32
    if (_mkdir(path) != 0 && errno != EEXIST) {
#else
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
#endif
        fprintf(stderr, "Cannot create directory %s: %s\n", path, strerror(errno));
        exit(1);
    }
}

/* ========== Dynamic array helpers ========== */

static void ensure_messages(void) {
    if (num_messages >= cap_messages) {
        cap_messages = cap_messages ? cap_messages * 2 : 64;
        messages = xrealloc(messages, sizeof(Message) * cap_messages);
    }
}

static void ensure_comments(void) {
    if (num_comments >= cap_comments) {
        cap_comments = cap_comments ? cap_comments * 2 : 64;
        comment_strs = xrealloc(comment_strs, sizeof(char*) * cap_comments);
        comment_block_ids = xrealloc(comment_block_ids, sizeof(int) * cap_comments);
    }
}

static void ensure_header_items(void) {
    if (num_header_items >= cap_header_items) {
        cap_header_items = cap_header_items ? cap_header_items * 2 : 128;
        header_items = xrealloc(header_items, sizeof(HeaderItem) * cap_header_items);
    }
}

static void add_comment(const char *text, int block_id) {
    ensure_comments();
    comment_strs[num_comments] = xstrdup(text);
    comment_block_ids[num_comments] = block_id;
    num_comments++;
}

static void add_header_comment(int ci) {
    ensure_header_items();
    header_items[num_header_items].is_comment = 1;
    header_items[num_header_items].index = ci;
    num_header_items++;
}

static void add_header_message(int mi) {
    ensure_header_items();
    header_items[num_header_items].is_comment = 0;
    header_items[num_header_items].index = mi;
    num_header_items++;
}

static void add_line(const char *s, const char *newline, size_t newline_len) {
    if (num_lines >= cap_lines) {
        cap_lines = cap_lines ? cap_lines * 2 : 256;
        lines = xrealloc(lines, sizeof(InputLine) * cap_lines);
    }
    lines[num_lines].text = xstrdup(s);
    lines[num_lines].newline[0] = '\0';
    lines[num_lines].newline_len = newline_len;
    if (newline_len > 0) {
        memcpy(lines[num_lines].newline, newline, newline_len);
        lines[num_lines].newline[newline_len] = '\0';
    }
    num_lines++;
}

/* ========== Utilities ========== */

static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end >= s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

static void copy_trunc(char *dst, size_t dst_size, const char *src) {
    if (dst_size == 0) return;
    snprintf(dst, dst_size, "%s", src ? src : "");
}

static void append_utf8_codepoint(char **buf, size_t *len, size_t *cap, uint32_t cp) {
    unsigned char tmp[4];
    size_t need;

    if (cp <= 0x7F) {
        tmp[0] = (unsigned char)cp;
        need = 1;
    } else if (cp <= 0x7FF) {
        tmp[0] = (unsigned char)(0xC0 | (cp >> 6));
        tmp[1] = (unsigned char)(0x80 | (cp & 0x3F));
        need = 2;
    } else if (cp <= 0xFFFF) {
        tmp[0] = (unsigned char)(0xE0 | (cp >> 12));
        tmp[1] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
        tmp[2] = (unsigned char)(0x80 | (cp & 0x3F));
        need = 3;
    } else {
        tmp[0] = (unsigned char)(0xF0 | (cp >> 18));
        tmp[1] = (unsigned char)(0x80 | ((cp >> 12) & 0x3F));
        tmp[2] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
        tmp[3] = (unsigned char)(0x80 | (cp & 0x3F));
        need = 4;
    }

    if (*len + need + 1 > *cap) {
        while (*len + need + 1 > *cap) *cap *= 2;
        *buf = xrealloc(*buf, *cap);
    }

    memcpy(*buf + *len, tmp, need);
    *len += need;
    (*buf)[*len] = '\0';
}

static uint32_t parse_value(const char *s) {
    char buf[64];
    strncpy(buf, s, sizeof(buf)-1); buf[sizeof(buf)-1] = '\0';
    char *t = trim(buf);
    if (t[0] == '0' && (t[1] == 'x' || t[1] == 'X'))
        return (uint32_t)strtoul(t, NULL, 16);
    return (uint32_t)strtoul(t, NULL, 10);
}

/* Return a trimmed copy of lines[idx]. The caller must use the result before calling again. */
static char trim_buf[MAX_LINE];
static char *get_trimmed(int idx) {
    strncpy(trim_buf, lines[idx].text, sizeof(trim_buf)-1);
    trim_buf[sizeof(trim_buf)-1] = '\0';
    return trim(trim_buf);
}

static const char *match_assignment(const char *line, const char *key) {
    size_t key_len = strlen(key);
    if (strncasecmp(line, key, key_len) != 0) return NULL;
    line += key_len;
    while (*line && isspace((unsigned char)*line)) line++;
    if (*line != '=') return NULL;
    line++;
    while (*line && isspace((unsigned char)*line)) line++;
    return line;
}

/* ========== Init ========== */

static void init_defaults(void) {
    strcpy(severities[0].name, "Success"); severities[0].value = 0x0; severities[0].define_name[0] = '\0';
    strcpy(severities[1].name, "Informational"); severities[1].value = 0x1; severities[1].define_name[0] = '\0';
    strcpy(severities[2].name, "Warning"); severities[2].value = 0x2; severities[2].define_name[0] = '\0';
    strcpy(severities[3].name, "Error"); severities[3].value = 0x3; severities[3].define_name[0] = '\0';
    num_severities = 4;

    strcpy(facilities[0].name, "System"); facilities[0].value = 0x0; facilities[0].define_name[0] = '\0';
    strcpy(facilities[1].name, "Application"); facilities[1].value = 0xFFF; facilities[1].define_name[0] = '\0';
    num_facilities = 2;
}

static uint32_t find_severity(const char *name) {
    for (int i = 0; i < num_severities; i++)
        if (strcasecmp(severities[i].name, name) == 0) return severities[i].value;
    fprintf(stderr, "Unknown severity: %s\n", name); exit(1);
}

static uint32_t find_facility(const char *name) {
    for (int i = 0; i < num_facilities; i++)
        if (strcasecmp(facilities[i].name, name) == 0) return facilities[i].value;
    if (strcasecmp(name, "System") == 0) return 0x0;
    if (strcasecmp(name, "Application") == 0) return 0xFFF;
    fprintf(stderr, "Unknown facility: %s\n", name); exit(1);
}

static int find_lang_index(const char *name) {
    for (int i = 0; i < num_langs; i++)
        if (strcasecmp(langs[i].name, name) == 0) return i;
    fprintf(stderr, "Unknown language: %s\n", name); exit(1);
}

/* ========== Name=Value:Define list parsing ========== */

/* Parse from lines[*pos] onwards. The first line contains '(' and possibly ')'.
   Reads lines until ')' is found. Updates *pos to past the last consumed line. */
static void parse_name_value_list_from_lines(int *pos,
    void (*callback)(const char *name, uint32_t value, const char *define_name))
{
    size_t buf_cap = 256;
    size_t buf_len = 0;
    char *buf = xrealloc(NULL, buf_cap);
    buf[0] = '\0';

    /* Find '(' in lines[*pos] */
    char *p = strchr(lines[*pos].text, '(');
    if (!p) { (*pos)++; free(buf); return; }
    size_t chunk_len = strlen(p + 1);
    if (chunk_len + 1 > buf_cap) {
        buf_cap = chunk_len + 1;
        buf = xrealloc(buf, buf_cap);
    }
    memcpy(buf, p + 1, chunk_len + 1);
    buf_len = chunk_len;
    (*pos)++;

    while (!strchr(buf, ')') && *pos < num_lines) {
        chunk_len = strlen(lines[*pos].text);
        if (buf_len + 1 + chunk_len + 1 > buf_cap) {
            while (buf_len + 1 + chunk_len + 1 > buf_cap) buf_cap *= 2;
            buf = xrealloc(buf, buf_cap);
        }
        buf[buf_len++] = ' ';
        memcpy(buf + buf_len, lines[*pos].text, chunk_len + 1);
        buf_len += chunk_len;
        (*pos)++;
    }

    p = strchr(buf, ')');
    if (p) *p = '\0';

    char *token = buf;
    while (*token) {
        while (*token && isspace((unsigned char)*token)) token++;
        if (!*token) break;
        char *eq = strchr(token, '=');
        if (!eq) break;

        char name[64];
        int len = (int)(eq - token);
        if (len >= (int)sizeof(name)) len = (int)sizeof(name) - 1;
        strncpy(name, token, len); name[len] = '\0'; trim(name);
        token = eq + 1;
        while (*token && isspace((unsigned char)*token)) token++;

        char val_str[64]; int vi = 0;
        while (*token && *token != ':' && !isspace((unsigned char)*token) && *token != ')')
            if (vi < (int)sizeof(val_str)-1) val_str[vi++] = *token++;
            else token++;
        val_str[vi] = '\0';
        uint32_t value = parse_value(val_str);

        char define_name[128] = "";
        if (*token == ':') {
            token++; int di = 0;
            while (*token && !isspace((unsigned char)*token) && *token != ')')
                if (di < (int)sizeof(define_name)-1) define_name[di++] = *token++;
                else token++;
            define_name[di] = '\0';
        }
        callback(name, value, define_name);
    }

    free(buf);
}

static void add_severity_cb(const char *name, uint32_t value, const char *dn) {
    for (int i = 0; i < num_severities; i++) {
        if (strcasecmp(severities[i].name, name) == 0) {
            severities[i].value = value;
            strcpy(severities[i].define_name, dn ? dn : "");
            return;
        }
    }
    if (num_severities >= MAX_SEVERITIES) return;
    strcpy(severities[num_severities].name, name);
    severities[num_severities].value = value;
    strcpy(severities[num_severities].define_name, dn ? dn : "");
    num_severities++;
}

static void add_facility_cb(const char *name, uint32_t value, const char *dn) {
    if (num_facilities >= MAX_FACILITIES) return;
    strcpy(facilities[num_facilities].name, name);
    facilities[num_facilities].value = value;
    strcpy(facilities[num_facilities].define_name, dn ? dn : "");
    num_facilities++;
}

static void add_language_cb(const char *name, uint32_t value, const char *dn) {
    if (num_langs >= MAX_LANGS) return;
    strcpy(langs[num_langs].name, name);
    langs[num_langs].lang_id = (uint16_t)value;
    if (dn && dn[0])
        strcpy(langs[num_langs].filename, dn);
    else
        snprintf(langs[num_langs].filename, sizeof(langs[num_langs].filename), "MSG%05X", value);
    num_langs++;
}

/* ========== Byte-to-UTF16 conversion (Windows-1252 mapping) ========== */

/* Windows-1252 mapping for bytes 0x80-0x9F to Unicode codepoints */
static const uint16_t cp1252_80_9f[32] = {
    0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
    0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178
};

/* Convert raw bytes to UTF-16LE using Windows-1252 mapping.
   Each byte maps to one uint16_t. Returns number of uint16_t written including null. */
static int cp1252_to_utf16le(const char *src, uint16_t *dst, int max_units) {
    int di = 0;
    const unsigned char *s = (const unsigned char *)src;
    while (*s && di < max_units - 1) {
        unsigned char c = *s++;
        if (c < 0x80) {
            dst[di++] = c;
        } else if (c >= 0x80 && c <= 0x9F) {
            dst[di++] = cp1252_80_9f[c - 0x80];
        } else {
            /* 0xA0-0xFF: direct mapping to Unicode (Latin-1 Supplement) */
            dst[di++] = c;
        }
    }
    dst[di++] = 0;
    return di;
}

static int unicode_to_cp1252(uint32_t cp, unsigned char *out) {
    if (cp < 0x80) {
        *out = (unsigned char)cp;
        return 1;
    }
    if (cp >= 0xA0 && cp <= 0xFF) {
        *out = (unsigned char)cp;
        return 1;
    }
    for (int i = 0; i < 32; i++) {
        if (cp1252_80_9f[i] == cp) {
            *out = (unsigned char)(0x80 + i);
            return 1;
        }
    }
    return 0;
}

static int utf8_to_utf16le(const char *src, uint16_t *dst, int max_units) {
    int di = 0;
    const unsigned char *s = (const unsigned char *)src;

    while (*s && di < max_units - 1) {
        uint32_t cp;
        unsigned char c = *s++;

        if (c < 0x80) {
            cp = c;
        } else if ((c & 0xE0) == 0xC0 && s[0]) {
            cp = ((uint32_t)(c & 0x1F) << 6) |
                 (uint32_t)(s[0] & 0x3F);
            s += 1;
        } else if ((c & 0xF0) == 0xE0 && s[0] && s[1]) {
            cp = ((uint32_t)(c & 0x0F) << 12) |
                 ((uint32_t)(s[0] & 0x3F) << 6) |
                 (uint32_t)(s[1] & 0x3F);
            s += 2;
        } else if ((c & 0xF8) == 0xF0 && s[0] && s[1] && s[2]) {
            cp = ((uint32_t)(c & 0x07) << 18) |
                 ((uint32_t)(s[0] & 0x3F) << 12) |
                 ((uint32_t)(s[1] & 0x3F) << 6) |
                 (uint32_t)(s[2] & 0x3F);
            s += 3;
        } else {
            cp = c;
        }

        if (cp <= 0xFFFF) {
            dst[di++] = (uint16_t)cp;
        } else if (di < max_units - 2) {
            cp -= 0x10000;
            dst[di++] = (uint16_t)(0xD800 + (cp >> 10));
            dst[di++] = (uint16_t)(0xDC00 + (cp & 0x3FF));
        } else {
            break;
        }
    }

    dst[di++] = 0;
    return di;
}

static int text_to_utf16le(const char *src, uint16_t *dst, int max_units) {
    if (input_is_utf8)
        return utf8_to_utf16le(src, dst, max_units);
    return cp1252_to_utf16le(src, dst, max_units);
}

static const char *codepage_to_encoding_name(int codepage) {
    switch (codepage) {
    case 874: return "CP874";
    case 932: return "CP932";
    case 936: return "CP936";
    case 949: return "CP949";
    case 950: return "CP950";
    case 1250: return "CP1250";
    case 1251: return "CP1251";
    case 1252: return "CP1252";
    case 1253: return "CP1253";
    case 1254: return "CP1254";
    case 1255: return "CP1255";
    case 1256: return "CP1256";
    case 1257: return "CP1257";
    default: return NULL;
    }
}

static unsigned char *convert_utf8_to_codepage(const char *src, int codepage, size_t *out_len) {
#ifdef _WIN32
    int wide_len;
    int byte_len;
    WCHAR *wide_buf;
    unsigned char *out_buf;

    wide_len = MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
    if (wide_len <= 0) {
        fprintf(stderr, "Cannot convert UTF-8 input to UTF-16\n");
        exit(1);
    }

    wide_buf = xrealloc(NULL, sizeof(WCHAR) * (size_t)wide_len);
    if (MultiByteToWideChar(CP_UTF8, 0, src, -1, wide_buf, wide_len) <= 0) {
        fprintf(stderr, "Cannot convert UTF-8 input to UTF-16\n");
        free(wide_buf);
        exit(1);
    }

    byte_len = WideCharToMultiByte((UINT)codepage, 0, wide_buf, -1, NULL, 0, NULL, NULL);
    if (byte_len <= 0) {
        fprintf(stderr, "Cannot convert UTF-16 to codepage %d\n", codepage);
        free(wide_buf);
        exit(1);
    }

    out_buf = xrealloc(NULL, (size_t)byte_len);
    if (WideCharToMultiByte((UINT)codepage, 0, wide_buf, -1, (char *)out_buf, byte_len, NULL, NULL) <= 0) {
        fprintf(stderr, "Cannot convert UTF-16 to codepage %d\n", codepage);
        free(wide_buf);
        free(out_buf);
        exit(1);
    }

    free(wide_buf);
    *out_len = (size_t)byte_len;
    return out_buf;
#else
    const char *encoding = codepage_to_encoding_name(codepage);
    iconv_t cd;
    size_t in_left;
    size_t out_cap;
    unsigned char *out_buf;
    char *in_ptr = (char *)src;
    char *out_ptr;
    size_t out_left;

    if (!encoding) {
        fprintf(stderr, "Unsupported output codepage: %d\n", codepage);
        exit(1);
    }

    cd = iconv_open(encoding, "UTF-8");
    if (cd == (iconv_t)-1) {
        fprintf(stderr, "iconv cannot convert UTF-8 to %s\n", encoding);
        exit(1);
    }

    in_left = strlen(src);
    out_cap = in_left * 8 + 16;
    out_buf = xrealloc(NULL, out_cap);
    out_ptr = (char *)out_buf;
    out_left = out_cap;

    while (1) {
        size_t rc = iconv(cd, &in_ptr, &in_left, &out_ptr, &out_left);
        if (rc != (size_t)-1) break;
        if (errno != E2BIG) {
            fprintf(stderr, "iconv conversion to %s failed\n", encoding);
            iconv_close(cd);
            free(out_buf);
            exit(1);
        }

        {
            size_t used = (size_t)(out_ptr - (char *)out_buf);
            out_cap *= 2;
            out_buf = xrealloc(out_buf, out_cap);
            out_ptr = (char *)out_buf + used;
            out_left = out_cap - used;
        }
    }

    *out_ptr++ = '\0';
    *out_len = (size_t)(out_ptr - (char *)out_buf);
    iconv_close(cd);
    return out_buf;
#endif
}

/* ========== Binary output helpers ========== */

static void write_u16le(FILE *f, uint16_t v) {
    unsigned char b[2] = { v & 0xFF, (v >> 8) & 0xFF };
    fwrite(b, 1, 2, f);
}

static void write_u32le(FILE *f, uint32_t v) {
    unsigned char b[4] = { v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF, (v >> 24) & 0xFF };
    fwrite(b, 1, 4, f);
}

static int compare_binmsg(const void *lhs, const void *rhs) {
    const BinMsg *left = lhs;
    const BinMsg *right = rhs;
    if (left->value < right->value) return -1;
    if (left->value > right->value) return 1;
    return 0;
}

static int compare_lang_index(const void *lhs, const void *rhs) {
    const int left = *(const int *)lhs;
    const int right = *(const int *)rhs;
    if (langs[left].lang_id < langs[right].lang_id) return -1;
    if (langs[left].lang_id > langs[right].lang_id) return 1;
    return 0;
}

static int write_header_comment(FILE *fp, const char *comment);
static int comment_is_blank(const char *comment);

/* ========== Write .bin file ========== */

static void write_bin_file(const char *dir, int lang_idx) {
    typedef struct { uint32_t low_id, high_id; int start_idx, count; } Block;
    const LangInfo *lang_info = find_lang_info(langs[lang_idx].lang_id);
    const int lang_codepage = lang_info ? lang_info->code_page : 1252;

    BinMsg *bm = xrealloc(NULL, sizeof(BinMsg) * (num_messages + 1));
    int bc = 0;

    for (int i = 0; i < num_messages; i++)
        for (int j = 0; j < messages[i].num_lang_texts; j++)
            if (messages[i].lang_texts[j].lang_index == lang_idx) {
                bm[bc].value = messages[i].value;
                bm[bc].text = messages[i].lang_texts[j].text;
                bc++; break;
            }

    if (bc == 0) { free(bm); return; }

    /* Sort */
    qsort(bm, bc, sizeof(BinMsg), compare_binmsg);

    /* Build blocks */
    Block *blk = xrealloc(NULL, sizeof(Block) * (bc + 1));
    int nb = 1;
    blk[0] = (Block){ bm[0].value, bm[0].value, 0, 1 };
    for (int i = 1; i < bc; i++) {
        if (bm[i].value == blk[nb-1].high_id + 1) { blk[nb-1].high_id = bm[i].value; blk[nb-1].count++; }
        else { blk[nb] = (Block){ bm[i].value, bm[i].value, i, 1 }; nb++; }
    }

    /* Compute entry sizes */
    int *esz = xrealloc(NULL, sizeof(int) * (bc ? bc : 1));
    unsigned char **payload = xrealloc(NULL, sizeof(unsigned char *) * (bc ? bc : 1));
    size_t *payload_len = xrealloc(NULL, sizeof(size_t) * (bc ? bc : 1));

    for (int i = 0; i < bc; i++) {
        const char *text = bm[i].text;
        if (output_unicode) {
            size_t tlen = strlen(text);
            uint16_t *u16 = xrealloc(NULL, sizeof(uint16_t) * (tlen * 2 + 4));
            int u16len = text_to_utf16le(text, u16, (int)(tlen * 2 + 4));
            payload[i] = (unsigned char *)u16;
            payload_len[i] = (size_t)u16len * 2;
        } else {
            payload[i] = convert_utf8_to_codepage(text, lang_codepage, &payload_len[i]);
        }
        int total = 4 + (int)payload_len[i];
        esz[i] = (total + 3) & ~3;
    }

    /* Compute offsets */
    uint32_t hsz = 4 + (uint32_t)nb * 12;
    uint32_t *boff = xrealloc(NULL, sizeof(uint32_t) * nb);
    uint32_t off = hsz;
    for (int b = 0; b < nb; b++) {
        boff[b] = off;
        for (int i = blk[b].start_idx; i < blk[b].start_idx + blk[b].count; i++) off += esz[i];
    }

    char *path = use_bin_prefix
        ? make_output_path(dir, output_base_name, langs[lang_idx].filename, ".bin")
        : make_output_path(dir, langs[lang_idx].filename, NULL, ".bin");
    FILE *fp = fopen(path, "wb");
    if (!fp) { fprintf(stderr, "Cannot create %s: %s\n", path, strerror(errno)); exit(1); }

    write_u32le(fp, (uint32_t)nb);
    for (int b = 0; b < nb; b++) { write_u32le(fp, blk[b].low_id); write_u32le(fp, blk[b].high_id); write_u32le(fp, boff[b]); }

    for (int i = 0; i < bc; i++) {
        write_u16le(fp, (uint16_t)esz[i]);
        write_u16le(fp, output_unicode ? 0x0001 : 0x0000);
        fwrite(payload[i], 1, payload_len[i], fp);
        int pad = esz[i] - 4 - (int)payload_len[i];
        for (int p = 0; p < pad; p++) fputc(0, fp);
    }

    fclose(fp);
    free(path);
    for (int i = 0; i < bc; i++) free(payload[i]);
    free(payload); free(payload_len); free(esz); free(boff); free(blk); free(bm);
}

/* ========== Write .h file ========== */

static void write_header(const char *dir, const char *base) {
    char *path = make_output_path(dir, base, NULL, ".h");
    FILE *fp = fopen(path, "w");
    if (!fp) { fprintf(stderr, "Cannot create %s: %s\n", path, strerror(errno)); exit(1); }

    fprintf(fp, "/* Do not edit this file manually.\n");
    fprintf(fp, "   This file is autogenerated by windmc.  */\n\n");
    fprintf(fp, "//\n");
    fprintf(fp, "//  The values are 32 bit layed out as follows:\n");
    fprintf(fp, "//\n");
    fprintf(fp, "//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1\n");
    fprintf(fp, "//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0\n");
    fprintf(fp, "//  +---+-+-+-----------------------+-------------------------------+\n");
    fprintf(fp, "//  |Sev|C|R|     Facility          |               Code            |\n");
    fprintf(fp, "//  +---+-+-+-----------------------+-------------------------------+\n");
    fprintf(fp, "//\n");
    fprintf(fp, "//  where\n");
    fprintf(fp, "//\n");
    fprintf(fp, "//      C    - is the Customer code flag\n");
    fprintf(fp, "//\n");
    fprintf(fp, "//      R    - is a reserved bit\n");
    fprintf(fp, "//\n");
    fprintf(fp, "//      Code - is the facility's status code\n");
    fprintf(fp, "//\n");
    fprintf(fp, "//      Sev  - is the severity code\n");
    fprintf(fp, "//\n");

    for (int i = 0; i < num_severities; i++) {
        fprintf(fp, "//           %s - %02x\n", severities[i].name, severities[i].value);
        if (severities[i].define_name[0])
            fprintf(fp, "#define %s 0x%x\n", severities[i].define_name, severities[i].value);
    }

    fprintf(fp, "//\n");
    fprintf(fp, "//      Facility - is the facility code\n");
    fprintf(fp, "//\n");

    if (custom_facilities) {
        int has_system = 0;
        int has_application = 0;
        for (int i = 0; i < num_facilities; i++) {
            fprintf(fp, "//           %s - %04x\n", facilities[i].name, facilities[i].value);
            if (facilities[i].define_name[0])
                fprintf(fp, "#define %s 0x%x\n", facilities[i].define_name, facilities[i].value);
            if (strcasecmp(facilities[i].name, "System") == 0) has_system = 1;
            if (strcasecmp(facilities[i].name, "Application") == 0) has_application = 1;
        }
        if (!has_system)
            fprintf(fp, "//           System - 00ff\n");
        if (!has_application)
            fprintf(fp, "//           Application - 0fff\n");
    } else {
        /* Default facilities use fixed display strings */
        fprintf(fp, "//           System - 00ff\n");
        fprintf(fp, "//           Application - 0fff\n");
    }

    fprintf(fp, "//\n\n");

    int suppress_next_message_prefix = 0;
    for (int i = 0; i < num_header_items; i++) {
        if (header_items[i].is_comment) {
            int block_id = comment_block_ids[header_items[i].index];
            const char *comment = comment_strs[header_items[i].index];
            int status = write_header_comment(fp, comment);
            if (status) {
                while (i + 1 < num_header_items &&
                       header_items[i + 1].is_comment &&
                       comment_block_ids[header_items[i + 1].index] == block_id)
                    i++;
                while (i + 1 < num_header_items &&
                       header_items[i + 1].is_comment &&
                       comment_is_blank(comment_strs[header_items[i + 1].index]))
                    i++;
                suppress_next_message_prefix = (status == 2);
            } else if (!comment_is_blank(comment)) {
                suppress_next_message_prefix = 0;
            }
        } else {
            Message *m = &messages[header_items[i].index];
            if (!suppress_next_message_prefix)
                fprintf(fp, "//\n");
            fprintf(fp, "// MessageId: %s\n//\n", m->symbolic_name);
            if (m->type_def[0])
                fprintf(fp, "#define %s (%s) 0x%x\n", m->symbolic_name, m->type_def, m->value);
            else
                fprintf(fp, "#define %s  0x%x\n", m->symbolic_name, m->value);
            fprintf(fp, "\n");
            suppress_next_message_prefix = 0;
        }
    }

    fclose(fp);
    free(path);
}

/* ========== Write .rc file ========== */

static void write_rc(const char *dir, const char *base) {
    char *path = make_output_path(dir, base, NULL, ".rc");
    FILE *fp = fopen(path, "w");
    if (!fp) { fprintf(stderr, "Cannot create %s: %s\n", path, strerror(errno)); exit(1); }

    fprintf(fp, "/* Do not edit this file manually.\n");
    fprintf(fp, "   This file is autogenerated by windmc.  */\n\n");

    int *order = xrealloc(NULL, sizeof(int) * (num_langs ? num_langs : 1));
    for (int i = 0; i < num_langs; i++) order[i] = i;
    qsort(order, num_langs, sizeof(int), compare_lang_index);

    for (int idx = 0; idx < num_langs; idx++) {
        int i = order[idx];
        const LangInfo *li = find_lang_info(langs[i].lang_id);
        if (!li) { fprintf(stderr, "Warning: unknown lang 0x%x\n", langs[i].lang_id); continue; }
        fprintf(fp, "\n// Country: %s\n", li->country);
        fprintf(fp, "// Language: %s\n", li->lang_name);
        fprintf(fp, "#pragma code_page(%d)\n", li->code_page);
        fprintf(fp, "LANGUAGE 0x%x, 0x%x\n", li->primary, li->sub);
        if (use_bin_prefix)
            fprintf(fp, "1 11 \"%s_%s.bin\"\n", output_base_name, langs[i].filename);
        else
            fprintf(fp, "1 11 \"%s.bin\"\n", langs[i].filename);
    }

    free(order);
    fclose(fp);
    free(path);
}

/* ========== Basename extraction ========== */

static void get_basename(const char *path, char *out, int sz) {
    const char *p = strrchr(path, '/');
    if (p) p++; else p = path;
    const char *dot = strrchr(p, '.');
    if (dot && dot > p) {
        int len = (int)(dot - p);
        if (len >= sz) len = sz - 1;
        strncpy(out, p, len); out[len] = '\0';
    } else {
        strncpy(out, p, sz - 1); out[sz - 1] = '\0';
    }
}

static int comment_is_blank(const char *comment) {
    while (*comment) {
        if (*comment != '\r' && !isspace((unsigned char)*comment))
            return 0;
        comment++;
    }
    return 1;
}

static void read_ansi_or_utf8_lines(FILE *fp) {
    unsigned char *data = NULL;
    size_t cap = 0;
    size_t len = 0;
    size_t line_start = 0;

    for (;;) {
        unsigned char chunk[4096];
        size_t got = fread(chunk, 1, sizeof(chunk), fp);
        if (got == 0) break;
        if (len + got > cap) {
            cap = cap ? cap * 2 : 4096;
            while (len + got > cap) cap *= 2;
            data = xrealloc(data, cap);
        }
        memcpy(data + len, chunk, got);
        len += got;
    }

    if (ferror(fp)) {
        fprintf(stderr, "Failed to read input\n");
        free(data);
        exit(1);
    }

    if (input_codepage == 65001 && len >= 3 &&
        data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        line_start = 3;
    }

    for (size_t i = line_start; i < len; i++) {
        if (data[i] != '\n') continue;

        if (i > line_start && data[i - 1] == '\r') {
            char *line = xrealloc(NULL, i - line_start);
            memcpy(line, data + line_start, i - line_start - 1);
            line[i - line_start - 1] = '\0';
            add_line(line, "\r\n", 2);
            free(line);
        } else {
            char *line = xrealloc(NULL, i - line_start + 1);
            memcpy(line, data + line_start, i - line_start);
            line[i - line_start] = '\0';
            add_line(line, "\n", 1);
            free(line);
        }

        line_start = i + 1;
    }

    if (line_start < len || len == 0) {
        char *line = xrealloc(NULL, len - line_start + 1);
        memcpy(line, data + line_start, len - line_start);
        line[len - line_start] = '\0';
        add_line(line, "", 0);
        free(line);
    }

    free(data);
}

static void flush_utf16le_line(char **line_buf, size_t *line_len, const char *newline, size_t newline_len) {
    (*line_buf)[*line_len] = '\0';
    add_line(*line_buf, newline, newline_len);
    *line_len = 0;
    (*line_buf)[0] = '\0';
}

static void read_utf16le_lines(FILE *fp) {
    unsigned char *data = NULL;
    size_t cap = 0;
    size_t len = 0;
    char *line_buf = xrealloc(NULL, 256);
    size_t line_cap = 256;
    size_t line_len = 0;

    for (;;) {
        unsigned char chunk[4096];
        size_t got = fread(chunk, 1, sizeof(chunk), fp);
        if (got == 0) break;
        if (len + got > cap) {
            cap = cap ? cap * 2 : 4096;
            while (len + got > cap) cap *= 2;
            data = xrealloc(data, cap);
        }
        memcpy(data + len, chunk, got);
        len += got;
    }

    if (ferror(fp)) {
        fprintf(stderr, "Failed to read UTF-16 input\n");
        free(line_buf);
        free(data);
        exit(1);
    }

    if (len >= 2 && data[0] == 0xFF && data[1] == 0xFE)
        memmove(data, data + 2, len -= 2);

    if ((len & 1) != 0) {
        fprintf(stderr, "UTF-16 input length must be even\n");
        free(line_buf);
        free(data);
        exit(1);
    }

    for (size_t i = 0; i < len; i += 2) {
        uint16_t unit = (uint16_t)(data[i] | (data[i + 1] << 8));

        if (unit == 0x000D) {
            if (i + 3 < len) {
                uint16_t next = (uint16_t)(data[i + 2] | (data[i + 3] << 8));
                if (next == 0x000A) {
                    flush_utf16le_line(&line_buf, &line_len, "\r\n", 2);
                    i += 2;
                    continue;
                }
            }
            flush_utf16le_line(&line_buf, &line_len, "\r", 1);
            continue;
        }

        if (unit == 0x000A) {
            flush_utf16le_line(&line_buf, &line_len, "\n", 1);
            continue;
        }

        if (unit >= 0xD800 && unit <= 0xDBFF && i + 3 < len) {
            uint16_t low = (uint16_t)(data[i + 2] | (data[i + 3] << 8));
            if (low >= 0xDC00 && low <= 0xDFFF) {
                uint32_t cp = 0x10000 + (((uint32_t)(unit - 0xD800) << 10) | (uint32_t)(low - 0xDC00));
                append_utf8_codepoint(&line_buf, &line_len, &line_cap, cp);
                i += 2;
                continue;
            }
        }

        append_utf8_codepoint(&line_buf, &line_len, &line_cap, unit);
    }

    if (line_len > 0 || len == 0)
        flush_utf16le_line(&line_buf, &line_len, "", 0);

    free(line_buf);
    free(data);
}

static int write_header_comment(FILE *fp, const char *comment) {
    const unsigned char *s = (const unsigned char *)comment;
    size_t len = strlen(comment);
    int had_cr = len > 0 && comment[len - 1] == '\r';
    int had_cpp_prefix = strncmp(comment, "//", 2) == 0;

    if (!input_is_utf8) {
        fputs(comment, fp);
        fputc('\n', fp);
        return 0;
    }

    while (*s) {
        uint32_t cp;
        unsigned char byte;

        if (*s < 0x80) {
            cp = *s++;
        } else if ((*s & 0xE0) == 0xC0 && s[1]) {
            cp = ((uint32_t)(s[0] & 0x1F) << 6) |
                 (uint32_t)(s[1] & 0x3F);
            s += 2;
        } else if ((*s & 0xF0) == 0xE0 && s[1] && s[2]) {
            cp = ((uint32_t)(s[0] & 0x0F) << 12) |
                 ((uint32_t)(s[1] & 0x3F) << 6) |
                 (uint32_t)(s[2] & 0x3F);
            s += 3;
        } else if ((*s & 0xF8) == 0xF0 && s[1] && s[2] && s[3]) {
            cp = ((uint32_t)(s[0] & 0x07) << 18) |
                 ((uint32_t)(s[1] & 0x3F) << 12) |
                 ((uint32_t)(s[2] & 0x3F) << 6) |
                 (uint32_t)(s[3] & 0x3F);
            s += 4;
        } else {
            break;
        }

        if (cp == '\r') {
            fputc('\r', fp);
            continue;
        }

        if (!unicode_to_cp1252(cp, &byte)) {
            if (had_cpp_prefix)
                fputs("//", fp);
            if (had_cr && !had_cpp_prefix)
                fputc('\r', fp);
            fputc('\n', fp);
            return had_cpp_prefix ? 2 : 1;
        }
        fputc(byte, fp);
    }

    fputc('\n', fp);
    return 0;
}

/* ========== Main ========== */

int main(int argc, char **argv) {
    char *header_dir = ".";
    char *resource_dir = ".";
    char *input_file = NULL;
    int unicode_input = 0;

    setlocale(LC_ALL, "");

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-u") == 0) unicode_input = 1;
        else if (strcmp(argv[i], "-a") == 0) input_codepage = 1252;
        else if (strcmp(argv[i], "-A") == 0) output_unicode = 0;
        else if (strcmp(argv[i], "-b") == 0) use_bin_prefix = 1;
        else if (strcmp(argv[i], "-C") == 0) {
            if (++i >= argc) { fprintf(stderr, "Missing -C arg\n"); return 1; }
            input_codepage = (int)parse_value(argv[i]);
        }
        else if (strcmp(argv[i], "-U") == 0) output_unicode = 1;
        else if (strcmp(argv[i], "-h") == 0) { if (++i >= argc) { fprintf(stderr, "Missing -h arg\n"); return 1; } header_dir = argv[i]; }
        else if (strcmp(argv[i], "-r") == 0) { if (++i >= argc) { fprintf(stderr, "Missing -r arg\n"); return 1; } resource_dir = argv[i]; }
        else if (strncmp(argv[i], "--codepage_in=", 14) == 0) input_codepage = (int)parse_value(argv[i] + 14);
        else if (strcmp(argv[i], "--ascii_out") == 0) output_unicode = 0;
        else if (strcmp(argv[i], "--binprefix") == 0) use_bin_prefix = 1;
        else if (strncmp(argv[i], "--headerdir=", 12) == 0) header_dir = argv[i] + 12;
        else if (strncmp(argv[i], "--rcdir=", 8) == 0) resource_dir = argv[i] + 8;
        else if (strcmp(argv[i], "--ascii_in") == 0) input_codepage = 1252;
        else if (strcmp(argv[i], "--unicode_in") == 0) unicode_input = 1;
        else if (strcmp(argv[i], "--unicode_out") == 0) output_unicode = 1;
        else if (argv[i][0] == '-') {
            fprintf(stderr, "Unsupported option: %s\n", argv[i]);
            return 1;
        }
        else input_file = argv[i];
    }

    if (!input_file) { fprintf(stderr, "Usage: windmc [-u] [-b] [-h hdr_dir] [-r rc_dir] input.mc\n"); return 1; }
    input_is_utf8 = unicode_input || input_codepage == 65001;
    make_dir(header_dir);
    make_dir(resource_dir);
    init_defaults();

    /* Read entire file into lines[] */
    FILE *fp = fopen(input_file, "rb");
    if (!fp) { fprintf(stderr, "Cannot open %s: %s\n", input_file, strerror(errno)); return 1; }
    if (unicode_input) read_utf16le_lines(fp);
    else read_ansi_or_utf8_lines(fp);
    fclose(fp);

    /* Parse */
    int pos = 0;
    uint32_t last_msg_id = 0;
    int have_prev_msg = 0;
    int pending_comment_start = 0;
    int comment_block_id = 0;
    int previous_was_comment = 0;

    while (pos < num_lines) {
        char *trimmed = get_trimmed(pos);
        if (trimmed[0] == '\0') {
            previous_was_comment = 0;
            pos++;
            continue;
        }

        /* Comment */
        if (lines[pos].text[0] == ';') {
            const char *ctext = lines[pos].text + 1;
            char comment_buf[MAX_LINE];
            if (!previous_was_comment)
                comment_block_id++;
            snprintf(comment_buf, sizeof(comment_buf), "%s%s",
                     ctext, lines[pos].newline_len == 2 ? "\r" : "");
            add_comment(comment_buf, comment_block_id);
            previous_was_comment = 1;
            pos++;
            continue;
        }

        previous_was_comment = 0;

        {
            const char *value = match_assignment(trimmed, "MessageIdTypedef");
            if (value) {
                copy_trunc(current_typedef, sizeof(current_typedef), value);
                pos++;
                continue;
            }
        }

        if (match_assignment(trimmed, "SeverityNames")) {
            parse_name_value_list_from_lines(&pos, add_severity_cb);
            continue;
        }

        if (match_assignment(trimmed, "FacilityNames")) {
            num_facilities = 0;
            custom_facilities = 1;
            parse_name_value_list_from_lines(&pos, add_facility_cb);
            continue;
        }

        if (match_assignment(trimmed, "LanguageNames")) {
            num_langs = 0;
            parse_name_value_list_from_lines(&pos, add_language_cb);
            continue;
        }

        if (match_assignment(trimmed, "MessageId")) {
            /* Flush pending comments */
            for (int c = pending_comment_start; c < num_comments; c++)
                add_header_comment(c);
            pending_comment_start = num_comments;

            ensure_messages();
            Message *m = &messages[num_messages];
            memset(m, 0, sizeof(Message));

            {
                const char *val = match_assignment(trimmed, "MessageId");
                if (val && val[0] == '\0')
                    m->id = have_prev_msg ? last_msg_id + 1 : 1;
                else if (val)
                    m->id = parse_value(val);
            }
            last_msg_id = m->id;
            have_prev_msg = 1;
            m->severity = 0;
            m->facility = 0;
            copy_trunc(m->type_def, sizeof(m->type_def), current_typedef);
            pos++;

            /* Read sub-directives */
            while (pos < num_lines) {
                trimmed = get_trimmed(pos);
                if (trimmed[0] == '\0') { pos++; continue; }

                {
                    const char *value = match_assignment(trimmed, "Severity");
                    if (value) {
                        m->severity = find_severity(value);
                        pos++;
                        continue;
                    }
                }
                {
                    const char *value = match_assignment(trimmed, "Facility");
                    if (value) {
                        m->facility = find_facility(value);
                        pos++;
                        continue;
                    }
                }
                {
                    const char *value = match_assignment(trimmed, "SymbolicName");
                    if (value) {
                        copy_trunc(m->symbolic_name, sizeof(m->symbolic_name), value);
                        pos++;
                        continue;
                    }
                }
                {
                    const char *value = match_assignment(trimmed, "Language");
                    if (value) {
                        char lang_name[64];
                        copy_trunc(lang_name, sizeof(lang_name), value);
                        int li;
                        if (num_langs == 0 && strcasecmp(lang_name, "English") == 0) {
                            add_language_cb("English", 0x0409, "MSG00001");
                            li = num_langs - 1;
                        } else {
                            li = find_lang_index(lang_name);
                        }
                        pos++;

                        /* Read text until "." */
                        int text_cap = 4096;
                        char *text = xrealloc(NULL, text_cap);
                        int text_len = 0;

                        while (pos < num_lines) {
                            char *rawline = lines[pos].text;
                            if (strcmp(rawline, ".") == 0) { pos++; break; }

                            int clen = (int)strlen(rawline);
                            int need = text_len + clen + (int)lines[pos].newline_len + 1;
                            if (need >= text_cap) {
                                while (need >= text_cap) text_cap *= 2;
                                text = xrealloc(text, text_cap);
                            }

                            memcpy(text + text_len, rawline, clen);
                            text_len += clen;
                            if (lines[pos].newline_len > 0) {
                                memcpy(text + text_len, lines[pos].newline, lines[pos].newline_len);
                                text_len += (int)lines[pos].newline_len;
                            }
                            pos++;
                        }
                        text[text_len] = '\0';

                        MsgLangText *mlt = &m->lang_texts[m->num_lang_texts];
                        mlt->lang_index = li;
                        mlt->text = text;
                        m->num_lang_texts++;
                        continue;
                    }
                }
                {
                    /* End of this message definition */
                    break;
                }
            }

            m->value = (m->severity << 30) | (m->facility << 16) | m->id;
            if (m->symbolic_name[0] != '\0')
                add_header_message(num_messages);
            num_messages++;
            pending_comment_start = num_comments;
            continue;
        }

        /* Unknown line, skip */
        pos++;
    }

    for (int c = pending_comment_start; c < num_comments; c++)
        add_header_comment(c);

    char base[256];
    get_basename(input_file, base, sizeof(base));
    copy_trunc(output_base_name, sizeof(output_base_name), base);
    write_header(header_dir, base);
    write_rc(resource_dir, base);
    for (int i = 0; i < num_langs; i++)
        write_bin_file(resource_dir, i);

    return 0;
}
