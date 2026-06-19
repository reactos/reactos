/*
 * Copyright 2010 Jacek Caban for CodeWeavers
 * Copyright 2010 Thomas Mullaly
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

#include <limits.h>
#include <wchar.h>

#include "urlmon_main.h"
#include "wine/debug.h"

#define NO_SHLWAPI_REG
#include "shlwapi.h"

#include "strsafe.h"
#include "winternl.h"
#include "inaddr.h"
#include "in6addr.h"
#include "ip2string.h"

#define URI_DISPLAY_NO_ABSOLUTE_URI         0x1
#define URI_DISPLAY_NO_DEFAULT_PORT_AUTH    0x2

#define ALLOW_NULL_TERM_SCHEME          0x01
#define ALLOW_NULL_TERM_USER_NAME       0x02
#define ALLOW_NULL_TERM_PASSWORD        0x04
#define ALLOW_BRACKETLESS_IP_LITERAL    0x08
#define SKIP_IP_FUTURE_CHECK            0x10
#define IGNORE_PORT_DELIMITER           0x20

#define RAW_URI_FORCE_PORT_DISP     0x1
#define RAW_URI_CONVERT_TO_DOS_PATH 0x2

#define COMBINE_URI_FORCE_FLAG_USE  0x1

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

static const IID IID_IUriObj = {0x4b364760,0x9f51,0x11df,{0x98,0x1c,0x08,0x00,0x20,0x0c,0x9a,0x66}};

typedef struct {
    IUri                IUri_iface;
    IUriBuilderFactory  IUriBuilderFactory_iface;
    IPersistStream      IPersistStream_iface;
    IMarshal            IMarshal_iface;

    LONG ref;

    BSTR            raw_uri;

    /* Information about the canonicalized URI's buffer. */
    WCHAR           *canon_uri;
    DWORD           canon_size;
    DWORD           canon_len;
    BOOL            display_modifiers;
    DWORD           create_flags;

    INT             scheme_start;
    DWORD           scheme_len;
    URL_SCHEME      scheme_type;

    INT             userinfo_start;
    DWORD           userinfo_len;
    INT             userinfo_split;

    INT             host_start;
    DWORD           host_len;
    Uri_HOST_TYPE   host_type;

    INT             port_offset;
    DWORD           port;
    BOOL            has_port;

    INT             authority_start;
    DWORD           authority_len;

    INT             domain_offset;

    INT             path_start;
    DWORD           path_len;
    INT             extension_offset;

    INT             query_start;
    DWORD           query_len;

    INT             fragment_start;
    DWORD           fragment_len;
} Uri;

typedef struct {
    IUriBuilder IUriBuilder_iface;
    LONG ref;

    Uri *uri;
    DWORD modified_props;

    WCHAR   *fragment;
    DWORD   fragment_len;

    WCHAR   *host;
    DWORD   host_len;

    WCHAR   *password;
    DWORD   password_len;

    WCHAR   *path;
    DWORD   path_len;

    BOOL    has_port;
    DWORD   port;

    WCHAR   *query;
    DWORD   query_len;

    WCHAR   *scheme;
    DWORD   scheme_len;

    WCHAR   *username;
    DWORD   username_len;
} UriBuilder;

typedef struct {
    BSTR            uri;

    BOOL            is_relative;
    BOOL            is_opaque;
    BOOL            has_implicit_scheme;
    BOOL            has_implicit_ip;
    UINT            implicit_ipv4;
    BOOL            must_have_path;

    const WCHAR     *scheme;
    DWORD           scheme_len;
    URL_SCHEME      scheme_type;

    const WCHAR     *username;
    DWORD           username_len;

    const WCHAR     *password;
    DWORD           password_len;

    const WCHAR     *host;
    DWORD           host_len;
    Uri_HOST_TYPE   host_type;

    IN6_ADDR        ipv6_address;

    BOOL            has_port;
    const WCHAR     *port;
    DWORD           port_len;
    DWORD           port_value;

    const WCHAR     *path;
    DWORD           path_len;

    const WCHAR     *query;
    DWORD           query_len;

    const WCHAR     *fragment;
    DWORD           fragment_len;
} parse_data;

static const CHAR hexDigits[] = "0123456789ABCDEF";

/* List of scheme types/scheme names that are recognized by the IUri interface as of IE 7. */
static const struct {
    URL_SCHEME  scheme;
    WCHAR       scheme_name[16];
} recognized_schemes[] = {
    {URL_SCHEME_FTP,            L"ftp"},
    {URL_SCHEME_HTTP,           L"http"},
    {URL_SCHEME_GOPHER,         L"gopher"},
    {URL_SCHEME_MAILTO,         L"mailto"},
    {URL_SCHEME_NEWS,           L"news"},
    {URL_SCHEME_NNTP,           L"nntp"},
    {URL_SCHEME_TELNET,         L"telnet"},
    {URL_SCHEME_WAIS,           L"wais"},
    {URL_SCHEME_FILE,           L"file"},
    {URL_SCHEME_MK,             L"mk"},
    {URL_SCHEME_HTTPS,          L"https"},
    {URL_SCHEME_SHELL,          L"shell"},
    {URL_SCHEME_SNEWS,          L"snews"},
    {URL_SCHEME_LOCAL,          L"local"},
    {URL_SCHEME_JAVASCRIPT,     L"javascript"},
    {URL_SCHEME_VBSCRIPT,       L"vbscript"},
    {URL_SCHEME_ABOUT,          L"about"},
    {URL_SCHEME_RES,            L"res"},
    {URL_SCHEME_MSSHELLROOTED,  L"ms-shell-rooted"},
    {URL_SCHEME_MSSHELLIDLIST,  L"ms-shell-idlist"},
    {URL_SCHEME_MSHELP,         L"hcp"},
    {URL_SCHEME_WILDCARD,       L"*"}
};

/* List of default ports Windows recognizes. */
static const struct {
    URL_SCHEME  scheme;
    USHORT      port;
} default_ports[] = {
    {URL_SCHEME_FTP,    21},
    {URL_SCHEME_HTTP,   80},
    {URL_SCHEME_GOPHER, 70},
    {URL_SCHEME_NNTP,   119},
    {URL_SCHEME_TELNET, 23},
    {URL_SCHEME_WAIS,   210},
    {URL_SCHEME_HTTPS,  443},
};

/* List of 3-character top level domain names Windows seems to recognize.
 * There might be more, but, these are the only ones I've found so far.
 */
static const struct {
    WCHAR tld_name[4];
} recognized_tlds[] = {
    {L"com"},
    {L"edu"},
    {L"gov"},
    {L"int"},
    {L"mil"},
    {L"net"},
    {L"org"}
};

static Uri *get_uri_obj(IUri *uri)
{
    Uri *ret;
    HRESULT hres;

    hres = IUri_QueryInterface(uri, &IID_IUriObj, (void**)&ret);
    return SUCCEEDED(hres) ? ret : NULL;
}

static inline BOOL is_alpha(WCHAR val) {
    return ((val >= 'a' && val <= 'z') || (val >= 'A' && val <= 'Z'));
}

static inline BOOL is_num(WCHAR val) {
    return (val >= '0' && val <= '9');
}

static inline BOOL is_drive_path(const WCHAR *str) {
    return (is_alpha(str[0]) && (str[1] == ':' || str[1] == '|'));
}

static inline BOOL is_unc_path(const WCHAR *str) {
    return (str[0] == '\\' && str[1] == '\\');
}

static inline BOOL is_forbidden_dos_path_char(WCHAR val) {
    return (val == '>' || val == '<' || val == '\"');
}

/* A URI is implicitly a file path if it begins with
 * a drive letter (e.g. X:) or starts with "\\" (UNC path).
 */
static inline BOOL is_implicit_file_path(const WCHAR *str) {
    return (is_unc_path(str) || (is_alpha(str[0]) && str[1] == ':'));
}

/* Checks if the URI is a hierarchical URI. A hierarchical
 * URI is one that has "//" after the scheme.
 */
static BOOL check_hierarchical(const WCHAR **ptr) {
    const WCHAR *start = *ptr;

    if(**ptr != '/')
        return FALSE;

    ++(*ptr);
    if(**ptr != '/') {
        *ptr = start;
        return FALSE;
    }

    ++(*ptr);
    return TRUE;
}

/* unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~" */
static inline BOOL is_unreserved(WCHAR val) {
    return (is_alpha(val) || is_num(val) || val == '-' || val == '.' ||
            val == '_' || val == '~');
}

/* sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
 *               / "*" / "+" / "," / ";" / "="
 */
static inline BOOL is_subdelim(WCHAR val) {
    return (val == '!' || val == '$' || val == '&' ||
            val == '\'' || val == '(' || val == ')' ||
            val == '*' || val == '+' || val == ',' ||
            val == ';' || val == '=');
}

/* gen-delims  = ":" / "/" / "?" / "#" / "[" / "]" / "@" */
static inline BOOL is_gendelim(WCHAR val) {
    return (val == ':' || val == '/' || val == '?' ||
            val == '#' || val == '[' || val == ']' ||
            val == '@');
}

/* Characters that delimit the end of the authority
 * section of a URI. Sometimes a '\\' is considered
 * an authority delimiter.
 */
static inline BOOL is_auth_delim(WCHAR val, BOOL acceptSlash) {
    return (val == '#' || val == '/' || val == '?' ||
            val == '\0' || (acceptSlash && val == '\\'));
}

/* reserved = gen-delims / sub-delims */
static inline BOOL is_reserved(WCHAR val) {
    return (is_subdelim(val) || is_gendelim(val));
}

static inline BOOL is_hexdigit(WCHAR val) {
    return ((val >= 'a' && val <= 'f') ||
            (val >= 'A' && val <= 'F') ||
            (val >= '0' && val <= '9'));
}

static inline BOOL is_path_delim(URL_SCHEME scheme, WCHAR val) {
    return (!val || (val == '#' && scheme != URL_SCHEME_FILE) || val == '?');
}

static inline BOOL is_slash(WCHAR c)
{
    return c == '/' || c == '\\';
}

static inline BOOL is_ascii(WCHAR c)
{
    return c < 0x80;
}

static BOOL is_default_port(URL_SCHEME scheme, DWORD port) {
    DWORD i;

    for(i = 0; i < ARRAY_SIZE(default_ports); ++i) {
        if(default_ports[i].scheme == scheme && default_ports[i].port)
            return TRUE;
    }

    return FALSE;
}

/* List of schemes types Windows seems to expect to be hierarchical. */
static inline BOOL is_hierarchical_scheme(URL_SCHEME type) {
    return(type == URL_SCHEME_HTTP || type == URL_SCHEME_FTP ||
           type == URL_SCHEME_GOPHER || type == URL_SCHEME_NNTP ||
           type == URL_SCHEME_TELNET || type == URL_SCHEME_WAIS ||
           type == URL_SCHEME_FILE || type == URL_SCHEME_HTTPS ||
           type == URL_SCHEME_RES);
}

/* Checks if 'flags' contains an invalid combination of Uri_CREATE flags. */
static inline BOOL has_invalid_flag_combination(DWORD flags) {
    return((flags & Uri_CREATE_DECODE_EXTRA_INFO && flags & Uri_CREATE_NO_DECODE_EXTRA_INFO) ||
           (flags & Uri_CREATE_CANONICALIZE && flags & Uri_CREATE_NO_CANONICALIZE) ||
           (flags & Uri_CREATE_CRACK_UNKNOWN_SCHEMES && flags & Uri_CREATE_NO_CRACK_UNKNOWN_SCHEMES) ||
           (flags & Uri_CREATE_PRE_PROCESS_HTML_URI && flags & Uri_CREATE_NO_PRE_PROCESS_HTML_URI) ||
           (flags & Uri_CREATE_IE_SETTINGS && flags & Uri_CREATE_NO_IE_SETTINGS));
}

/* Applies each default Uri_CREATE flags to 'flags' if it
 * doesn't cause a flag conflict.
 */
static void apply_default_flags(DWORD *flags) {
    if(!(*flags & Uri_CREATE_NO_CANONICALIZE))
        *flags |= Uri_CREATE_CANONICALIZE;
    if(!(*flags & Uri_CREATE_NO_DECODE_EXTRA_INFO))
        *flags |= Uri_CREATE_DECODE_EXTRA_INFO;
    if(!(*flags & Uri_CREATE_NO_CRACK_UNKNOWN_SCHEMES))
        *flags |= Uri_CREATE_CRACK_UNKNOWN_SCHEMES;
    if(!(*flags & Uri_CREATE_NO_PRE_PROCESS_HTML_URI))
        *flags |= Uri_CREATE_PRE_PROCESS_HTML_URI;
    if(!(*flags & Uri_CREATE_IE_SETTINGS))
        *flags |= Uri_CREATE_NO_IE_SETTINGS;
}

/* Determines if the URI is hierarchical using the information already parsed into
 * data and using the current location of parsing in the URI string.
 *
 * Windows considers a URI hierarchical if one of the following is true:
 *  A.) It's a wildcard scheme.
 *  B.) It's an implicit file scheme.
 *  C.) It's a known hierarchical scheme and it has two '\\' after the scheme name.
 *      (the '\\' will be converted into "//" during canonicalization).
 *  D.) "//" appears after the scheme name (or at the beginning if no scheme is given).
 */
static inline BOOL is_hierarchical_uri(const WCHAR **ptr, const parse_data *data) {
    const WCHAR *start = *ptr;

    if(data->scheme_type == URL_SCHEME_WILDCARD)
        return TRUE;
    else if(data->scheme_type == URL_SCHEME_FILE && data->has_implicit_scheme)
        return TRUE;
    else if(is_hierarchical_scheme(data->scheme_type) && (*ptr)[0] == '\\' && (*ptr)[1] == '\\') {
        *ptr += 2;
        return TRUE;
    } else if(data->scheme_type != URL_SCHEME_MAILTO && check_hierarchical(ptr))
        return TRUE;

    *ptr = start;
    return FALSE;
}

/* Taken from dlls/jscript/lex.c */
static int hex_to_int(WCHAR val) {
    if(val >= '0' && val <= '9')
        return val - '0';
    else if(val >= 'a' && val <= 'f')
        return val - 'a' + 10;
    else if(val >= 'A' && val <= 'F')
        return val - 'A' + 10;

    return -1;
}

/* Helper function for converting a percent encoded string
 * representation of a WCHAR value into its actual WCHAR value. If
 * the two characters following the '%' aren't valid hex values then
 * this function returns the NULL character.
 *
 * E.g.
 *  "%2E" will result in '.' being returned by this function.
 */
static WCHAR decode_pct_val(const WCHAR *ptr) {
    WCHAR ret = '\0';

    if(*ptr == '%' && is_hexdigit(*(ptr + 1)) && is_hexdigit(*(ptr + 2))) {
        INT a = hex_to_int(*(ptr + 1));
        INT b = hex_to_int(*(ptr + 2));

        ret = a << 4;
        ret += b;
    }

    return ret;
}

/* Helper function for percent encoding a given character
 * and storing the encoded value into a given buffer (dest).
 *
 * It's up to the calling function to ensure that there is
 * at least enough space in 'dest' for the percent encoded
 * value to be stored (so dest + 3 spaces available).
 */
static inline void pct_encode_val(WCHAR val, WCHAR *dest) {
    dest[0] = '%';
    dest[1] = hexDigits[(val >> 4) & 0xf];
    dest[2] = hexDigits[val & 0xf];
}

/* Attempts to parse the domain name from the host.
 *
 * This function also includes the Top-level Domain (TLD) name
 * of the host when it tries to find the domain name. If it finds
 * a valid domain name it will assign 'domain_start' the offset
 * into 'host' where the domain name starts.
 *
 * It's implied that if there is a domain name its range is:
 * [host+domain_start, host+host_len).
 */
void find_domain_name(const WCHAR *host, DWORD host_len,
                             INT *domain_start) {
    const WCHAR *last_tld, *sec_last_tld, *end, *p;

    end = host+host_len-1;

    *domain_start = -1;

    /* There has to be at least enough room for a '.' followed by a
     * 3-character TLD for a domain to even exist in the host name.
     */
    if(host_len < 4)
        return;

    for (last_tld = sec_last_tld = NULL, p = host; p <= end; p++)
    {
        if (*p == '.')
        {
            sec_last_tld = last_tld;
            last_tld = p;
        }
    }
    if(!last_tld)
        /* http://hostname -> has no domain name. */
        return;

    if(!sec_last_tld) {
        /* If the '.' is at the beginning of the host there
         * has to be at least 3 characters in the TLD for it
         * to be valid.
         *  Ex: .com -> .com as the domain name.
         *      .co  -> has no domain name.
         */
        if(last_tld-host == 0) {
            if(end-(last_tld-1) < 3)
                return;
        } else if(last_tld-host == 3) {
            DWORD i;

            /* If there are three characters in front of last_tld and
             * they are on the list of recognized TLDs, then this
             * host doesn't have a domain (since the host only contains
             * a TLD name.
             *  Ex: edu.uk -> has no domain name.
             *      foo.uk -> foo.uk as the domain name.
             */
            for(i = 0; i < ARRAY_SIZE(recognized_tlds); ++i) {
                if(!StrCmpNIW(host, recognized_tlds[i].tld_name, 3))
                    return;
            }
        } else if(last_tld-host < 3)
        {
            /* Anything less than 3 ASCII characters is considered part
             * of the TLD name.
             *  Ex: ak.uk -> Has no domain name.
             */
            for(p = host; p < last_tld; p++) {
                if(!is_ascii(*p))
                    break;
            }

            if(p == last_tld)
                return;
        }

        /* Otherwise the domain name is the whole host name. */
        *domain_start = 0;
    } else if(end+1-last_tld > 3) {
        /* If the last_tld has more than 3 characters, then it's automatically
         * considered the TLD of the domain name.
         *  Ex: www.winehq.org.uk.test -> uk.test as the domain name.
         */
        *domain_start = (sec_last_tld+1)-host;
    } else if(last_tld - (sec_last_tld+1) < 4) {
        DWORD i;
        /* If the sec_last_tld is 3 characters long it HAS to be on the list of
         * recognized to still be considered part of the TLD name, otherwise
         * it's considered the domain name.
         *  Ex: www.google.com.uk -> google.com.uk as the domain name.
         *      www.google.foo.uk -> foo.uk as the domain name.
         */
        if(last_tld - (sec_last_tld+1) == 3) {
            for(i = 0; i < ARRAY_SIZE(recognized_tlds); ++i) {
                if(!StrCmpNIW(sec_last_tld+1, recognized_tlds[i].tld_name, 3)) {
                    for (p = sec_last_tld; p > host; p--) if (p[-1] == '.') break;
                    *domain_start = p - host;
                    TRACE("Found domain name %s\n", debugstr_wn(host+*domain_start,
                                                        (host+host_len)-(host+*domain_start)));
                    return;
                }
            }

            *domain_start = (sec_last_tld+1)-host;
        } else {
            /* Since the sec_last_tld is less than 3 characters it's considered
             * part of the TLD.
             *  Ex: www.google.fo.uk -> google.fo.uk as the domain name.
             */
            for (p = sec_last_tld; p > host; p--) if (p[-1] == '.') break;
            *domain_start = p - host;
        }
    } else {
        /* The second to last TLD has more than 3 characters making it
         * the domain name.
         *  Ex: www.google.test.us -> test.us as the domain name.
         */
        *domain_start = (sec_last_tld+1)-host;
    }

    TRACE("Found domain name %s\n", debugstr_wn(host+*domain_start,
                                        (host+host_len)-(host+*domain_start)));
}

/* Removes the dot segments from a hierarchical URIs path component. This
 * function performs the removal in place.
 *
 * This function returns the new length of the path string.
 */
static DWORD remove_dot_segments(WCHAR *path, DWORD path_len) {
    WCHAR *out = path;
    const WCHAR *in = out;
    const WCHAR *end = out + path_len;
    DWORD len;

    while(in < end) {
        /* Move the first path segment in the input buffer to the end of
         * the output buffer, and any subsequent characters up to, including
         * the next "/" character (if any) or the end of the input buffer.
         */
        while(in < end && !is_slash(*in))
            *out++ = *in++;
        if(in == end)
            break;
        *out++ = *in++;

        while(in < end) {
            if(*in != '.')
                break;

            /* Handle ending "/." */
            if(in + 1 == end) {
                ++in;
                break;
            }

            /* Handle "/./" */
            if(is_slash(in[1])) {
                in += 2;
                continue;
            }

            /* If we don't have "/../" or ending "/.." */
            if(in[1] != '.' || (in + 2 != end && !is_slash(in[2])))
                break;

            /* Find the slash preceding out pointer and move out pointer to it */
            if(out > path+1 && is_slash(*--out))
                --out;
            while(out > path && !is_slash(*(--out)));
            if(is_slash(*out))
                ++out;
            in += 2;
            if(in != end)
                ++in;
        }
    }

    len = out - path;
    TRACE("(%p %ld): Path after dot segments removed %s len=%ld\n", path, path_len,
        debugstr_wn(path, len), len);
    return len;
}

/* Attempts to find the file extension in a given path. */
static INT find_file_extension(const WCHAR *path, DWORD path_len) {
    const WCHAR *end;

    for(end = path+path_len-1; end >= path && *end != '/' && *end != '\\'; --end) {
        if(*end == '.')
            return end-path;
    }

    return -1;
}

/* Removes all the leading and trailing white spaces or
 * control characters from the URI and removes all control
 * characters inside of the URI string.
 */
static BSTR pre_process_uri(LPCWSTR uri) {
    const WCHAR *start, *end, *ptr;
    WCHAR *ptr2;
    DWORD len;
    BSTR ret;

    start = uri;
    /* Skip leading controls and whitespace. */
    while(*start && (iswcntrl(*start) || iswspace(*start))) ++start;

    /* URI consisted only of control/whitespace. */
    if(!*start)
        return SysAllocStringLen(NULL, 0);

    end = start + lstrlenW(start);
    while(--end > start && (iswcntrl(*end) || iswspace(*end)));

    len = ++end - start;
    for(ptr = start; ptr < end; ptr++) {
        if(iswcntrl(*ptr))
            len--;
    }

    ret = SysAllocStringLen(NULL, len);
    if(!ret)
        return NULL;

    for(ptr = start, ptr2=ret; ptr < end; ptr++) {
        if(!iswcntrl(*ptr))
            *ptr2++ = *ptr;
    }

    return ret;
}

/* Converts an IPv4 address in numerical form into its fully qualified
 * string form. This function returns the number of characters written
 * to 'dest'. If 'dest' is NULL this function will return the number of
 * characters that would have been written.
 *
 * It's up to the caller to ensure there's enough space in 'dest' for the
 * address.
 */
static DWORD ui2ipv4(WCHAR *dest, UINT address) {
    DWORD ret = 0;
    UCHAR digits[4];

    digits[0] = (address >> 24) & 0xff;
    digits[1] = (address >> 16) & 0xff;
    digits[2] = (address >> 8) & 0xff;
    digits[3] = address & 0xff;

    if(!dest) {
        WCHAR tmp[16];
        ret = swprintf(tmp, ARRAY_SIZE(tmp), L"%u.%u.%u.%u", digits[0], digits[1], digits[2], digits[3]);
    } else
        ret = swprintf(dest, 16, L"%u.%u.%u.%u", digits[0], digits[1], digits[2], digits[3]);

    return ret;
}

static DWORD ui2str(WCHAR *dest, UINT value) {
    DWORD ret = 0;

    if(!dest) {
        WCHAR tmp[11];
        ret = swprintf(tmp, ARRAY_SIZE(tmp), L"%u", value);
    } else
        ret = swprintf(dest, 11, L"%u", value);

    return ret;
}

/* Checks if the characters pointed to by 'ptr' are
 * a percent encoded data octet.
 *
 * pct-encoded = "%" HEXDIG HEXDIG
 */
static BOOL check_pct_encoded(const WCHAR **ptr) {
    const WCHAR *start = *ptr;

    if(**ptr != '%')
        return FALSE;

    ++(*ptr);
    if(!is_hexdigit(**ptr)) {
        *ptr = start;
        return FALSE;
    }

    ++(*ptr);
    if(!is_hexdigit(**ptr)) {
        *ptr = start;
        return FALSE;
    }

    ++(*ptr);
    return TRUE;
}

/* dec-octet   = DIGIT                 ; 0-9
 *             / %x31-39 DIGIT         ; 10-99
 *             / "1" 2DIGIT            ; 100-199
 *             / "2" %x30-34 DIGIT     ; 200-249
 *             / "25" %x30-35          ; 250-255
 */
static BOOL check_dec_octet(const WCHAR **ptr) {
    const WCHAR *c1, *c2, *c3;

    c1 = *ptr;
    /* A dec-octet must be at least 1 digit long. */
    if(*c1 < '0' || *c1 > '9')
        return FALSE;

    ++(*ptr);

    c2 = *ptr;
    /* Since the 1-digit requirement was met, it doesn't
     * matter if this is a DIGIT value, it's considered a
     * dec-octet.
     */
    if(*c2 < '0' || *c2 > '9')
        return TRUE;

    ++(*ptr);

    c3 = *ptr;
    /* Same explanation as above. */
    if(*c3 < '0' || *c3 > '9')
        return TRUE;

    /* Anything > 255 isn't a valid IP dec-octet. */
    if(*c1 >= '2' && *c2 >= '5' && *c3 >= '5') {
        *ptr = c1;
        return FALSE;
    }

    ++(*ptr);
    return TRUE;
}

/* Checks if there is an implicit IPv4 address in the host component of the URI.
 * The max value of an implicit IPv4 address is UINT_MAX.
 *
 *  Ex:
 *      "234567" would be considered an implicit IPv4 address.
 */
static BOOL check_implicit_ipv4(const WCHAR **ptr, UINT *val) {
    const WCHAR *start = *ptr;
    ULONGLONG ret = 0;
    *val = 0;

    while(is_num(**ptr)) {
        ret = ret*10 + (**ptr - '0');

        if(ret > UINT_MAX) {
            *ptr = start;
            return FALSE;
        }
        ++(*ptr);
    }

    if(*ptr == start)
        return FALSE;

    *val = ret;
    return TRUE;
}

/* Checks if the string contains an IPv4 address.
 *
 * This function has a strict mode or a non-strict mode of operation
 * When 'strict' is set to FALSE this function will return TRUE if
 * the string contains at least 'dec-octet "." dec-octet' since partial
 * IPv4 addresses will be normalized out into full IPv4 addresses. When
 * 'strict' is set this function expects there to be a full IPv4 address.
 *
 * IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
 */
static BOOL check_ipv4address(const WCHAR **ptr, BOOL strict) {
    const WCHAR *start = *ptr;

    if(!check_dec_octet(ptr)) {
        *ptr = start;
        return FALSE;
    }

    if(**ptr != '.') {
        *ptr = start;
        return FALSE;
    }

    ++(*ptr);
    if(!check_dec_octet(ptr)) {
        *ptr = start;
        return FALSE;
    }

    if(**ptr != '.') {
        if(strict) {
            *ptr = start;
            return FALSE;
        } else
            return TRUE;
    }

    ++(*ptr);
    if(!check_dec_octet(ptr)) {
        *ptr = start;
        return FALSE;
    }

    if(**ptr != '.') {
        if(strict) {
            *ptr = start;
            return FALSE;
        } else
            return TRUE;
    }

    ++(*ptr);
    if(!check_dec_octet(ptr)) {
        *ptr = start;
        return FALSE;
    }

    /* Found a four digit ip address. */
    return TRUE;
}
/* Tries to parse the scheme name of the URI.
 *
 * scheme = ALPHA *(ALPHA | NUM | '+' | '-' | '.') as defined by RFC 3896.
 * NOTE: Windows accepts a number as the first character of a scheme.
 */
static BOOL parse_scheme_name(const WCHAR **ptr, parse_data *data, DWORD extras) {
    const WCHAR *start = *ptr;

    data->scheme = NULL;
    data->scheme_len = 0;

    while(**ptr) {
        if(**ptr == '*' && *ptr == start) {
            /* Might have found a wildcard scheme. If it is the next
             * char has to be a ':' for it to be a valid URI
             */
            ++(*ptr);
            break;
        } else if(!is_num(**ptr) && !is_alpha(**ptr) && **ptr != '+' &&
           **ptr != '-' && **ptr != '.')
            break;

        (*ptr)++;
    }

    if(*ptr == start)
        return FALSE;

    /* Schemes must end with a ':' */
    if(**ptr != ':' && !((extras & ALLOW_NULL_TERM_SCHEME) && !**ptr)) {
        *ptr = start;
        return FALSE;
    }

    data->scheme = start;
    data->scheme_len = *ptr - start;

    ++(*ptr);
    return TRUE;
}

/* Tries to deduce the corresponding URL_SCHEME for the given URI. Stores
 * the deduced URL_SCHEME in data->scheme_type.
 */
static BOOL parse_scheme_type(parse_data *data) {
    /* If there's scheme data then see if it's a recognized scheme. */
    if(data->scheme && data->scheme_len) {
        DWORD i;

        for(i = 0; i < ARRAY_SIZE(recognized_schemes); ++i) {
            if(lstrlenW(recognized_schemes[i].scheme_name) == data->scheme_len) {
                /* Has to be a case insensitive compare. */
                if(!StrCmpNIW(recognized_schemes[i].scheme_name, data->scheme, data->scheme_len)) {
                    data->scheme_type = recognized_schemes[i].scheme;
                    return TRUE;
                }
            }
        }

        /* If we get here it means it's not a recognized scheme. */
        data->scheme_type = URL_SCHEME_UNKNOWN;
        return TRUE;
    } else if(data->is_relative) {
        /* Relative URI's have no scheme. */
        data->scheme_type = URL_SCHEME_UNKNOWN;
        return TRUE;
    } else {
        /* Should never reach here! what happened... */
        FIXME("(%p): Unable to determine scheme type for URI %s\n", data, debugstr_w(data->uri));
        return FALSE;
    }
}

/* Tries to parse (or deduce) the scheme_name of a URI. If it can't
 * parse a scheme from the URI it will try to deduce the scheme_name and scheme_type
 * using the flags specified in 'flags' (if any). Flags that affect how this function
 * operates are the Uri_CREATE_ALLOW_* flags.
 *
 * All parsed/deduced information will be stored in 'data' when the function returns.
 *
 * Returns TRUE if it was able to successfully parse the information.
 */
static BOOL parse_scheme(const WCHAR **ptr, parse_data *data, DWORD flags, DWORD extras) {
    /* First check to see if the uri could implicitly be a file path. */
    if(is_implicit_file_path(*ptr)) {
        if(flags & Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME) {
            data->scheme = L"file";
            data->scheme_len = lstrlenW(L"file");
            data->has_implicit_scheme = TRUE;

            TRACE("(%p %p %lx): URI is an implicit file path.\n", ptr, data, flags);
        } else {
            /* Windows does not consider anything that can implicitly be a file
             * path to be a valid URI if the ALLOW_IMPLICIT_FILE_SCHEME flag is not set...
             */
            TRACE("(%p %p %lx): URI is implicitly a file path, but, the ALLOW_IMPLICIT_FILE_SCHEME flag wasn't set.\n",
                    ptr, data, flags);
            return FALSE;
        }
    } else if(!parse_scheme_name(ptr, data, extras)) {
        /* No scheme was found, this means it could be:
         *      a) an implicit Wildcard scheme
         *      b) a relative URI
         *      c) an invalid URI.
         */
        if(flags & Uri_CREATE_ALLOW_IMPLICIT_WILDCARD_SCHEME) {
            data->scheme = L"*";
            data->scheme_len = lstrlenW(L"*");
            data->has_implicit_scheme = TRUE;

            TRACE("(%p %p %lx): URI is an implicit wildcard scheme.\n", ptr, data, flags);
        } else if (flags & Uri_CREATE_ALLOW_RELATIVE) {
            data->is_relative = TRUE;
            TRACE("(%p %p %lx): URI is relative.\n", ptr, data, flags);
        } else {
            TRACE("(%p %p %lx): Malformed URI found. Unable to deduce scheme name.\n", ptr, data, flags);
            return FALSE;
        }
    }

    if(!data->is_relative)
        TRACE("(%p %p %lx): Found scheme=%s scheme_len=%ld\n", ptr, data, flags,
                debugstr_wn(data->scheme, data->scheme_len), data->scheme_len);

    if(!parse_scheme_type(data))
        return FALSE;

    TRACE("(%p %p %lx): Assigned %d as the URL_SCHEME.\n", ptr, data, flags, data->scheme_type);
    return TRUE;
}

static BOOL parse_username(const WCHAR **ptr, parse_data *data, DWORD flags, DWORD extras) {
    data->username = *ptr;

    while(**ptr != ':' && **ptr != '@') {
        if(**ptr == '%') {
            if(!check_pct_encoded(ptr)) {
                if(data->scheme_type != URL_SCHEME_UNKNOWN) {
                    *ptr = data->username;
                    data->username = NULL;
                    return FALSE;
                }
            } else
                continue;
        } else if(extras & ALLOW_NULL_TERM_USER_NAME && !**ptr)
            break;
        else if(is_auth_delim(**ptr, data->scheme_type != URL_SCHEME_UNKNOWN)) {
            *ptr = data->username;
            data->username = NULL;
            return FALSE;
        }

        ++(*ptr);
    }

    data->username_len = *ptr - data->username;
    return TRUE;
}

static BOOL parse_password(const WCHAR **ptr, parse_data *data, DWORD flags, DWORD extras) {
    data->password = *ptr;

    while(**ptr != '@') {
        if(**ptr == '%') {
            if(!check_pct_encoded(ptr)) {
                if(data->scheme_type != URL_SCHEME_UNKNOWN) {
                    *ptr = data->password;
                    data->password = NULL;
                    return FALSE;
                }
            } else
                continue;
        } else if(extras & ALLOW_NULL_TERM_PASSWORD && !**ptr)
            break;
        else if(is_auth_delim(**ptr, data->scheme_type != URL_SCHEME_UNKNOWN)) {
            *ptr = data->password;
            data->password = NULL;
            return FALSE;
        }

        ++(*ptr);
    }

    data->password_len = *ptr - data->password;
    return TRUE;
}

/* Parses the userinfo part of the URI (if it exists). The userinfo field of
 * a URI can consist of "username:password@", or just "username@".
 *
 * RFC def:
 * userinfo    = *( unreserved / pct-encoded / sub-delims / ":" )
 *
 * NOTES:
 *  1)  If there is more than one ':' in the userinfo part of the URI Windows
 *      uses the first occurrence of ':' to delimit the username and password
 *      components.
 *
 *      ex:
 *          ftp://user:pass:word@winehq.org
 *
 *      would yield "user" as the username and "pass:word" as the password.
 *
 *  2)  Windows allows any character to appear in the "userinfo" part of
 *      a URI, as long as it's not an authority delimiter character set.
 */
static void parse_userinfo(const WCHAR **ptr, parse_data *data, DWORD flags) {
    const WCHAR *start = *ptr;

    if(!parse_username(ptr, data, flags, 0)) {
        TRACE("(%p %p %lx): URI contained no userinfo.\n", ptr, data, flags);
        return;
    }

    if(**ptr == ':') {
        ++(*ptr);
        if(!parse_password(ptr, data, flags, 0)) {
            *ptr = start;
            data->username = NULL;
            data->username_len = 0;
            TRACE("(%p %p %lx): URI contained no userinfo.\n", ptr, data, flags);
            return;
        }
    }

    if(**ptr != '@') {
        *ptr = start;
        data->username = NULL;
        data->username_len = 0;
        data->password = NULL;
        data->password_len = 0;

        TRACE("(%p %p %lx): URI contained no userinfo.\n", ptr, data, flags);
        return;
    }

    if(data->username)
        TRACE("(%p %p %lx): Found username %s len=%ld.\n", ptr, data, flags,
            debugstr_wn(data->username, data->username_len), data->username_len);

    if(data->password)
        TRACE("(%p %p %lx): Found password %s len=%ld.\n", ptr, data, flags,
            debugstr_wn(data->password, data->password_len), data->password_len);

    ++(*ptr);
}

/* Attempts to parse a port from the URI.
 *
 * NOTES:
 *  Windows seems to have a cap on what the maximum value
 *  for a port can be. The max value is USHORT_MAX.
 *
 * port = *DIGIT
 */
static BOOL parse_port(const WCHAR **ptr, parse_data *data) {
    UINT port = 0;
    data->port = *ptr;

    while(!is_auth_delim(**ptr, data->scheme_type != URL_SCHEME_UNKNOWN)) {
        if(!is_num(**ptr)) {
            *ptr = data->port;
            data->port = NULL;
            return FALSE;
        }

        port = port*10 + (**ptr-'0');

        if(port > USHRT_MAX) {
            *ptr = data->port;
            data->port = NULL;
            return FALSE;
        }

        ++(*ptr);
    }

    data->has_port = TRUE;
    data->port_value = port;
    data->port_len = *ptr - data->port;

    TRACE("(%p %p): Found port %s len=%ld value=%lu\n", ptr, data,
        debugstr_wn(data->port, data->port_len), data->port_len, data->port_value);
    return TRUE;
}

/* Attempts to parse a IPv4 address from the URI.
 *
 * NOTES:
 *  Windows normalizes IPv4 addresses, This means there are three
 *  possibilities for the URI to contain an IPv4 address.
 *      1)  A well formed address (ex. 192.2.2.2).
 *      2)  A partially formed address. For example "192.0" would
 *          normalize to "192.0.0.0" during canonicalization.
 *      3)  An implicit IPv4 address. For example "256" would
 *          normalize to "0.0.1.0" during canonicalization. Also
 *          note that the maximum value for an implicit IP address
 *          is UINT_MAX, if the value in the URI exceeds this then
 *          it is not considered an IPv4 address.
 */
static BOOL parse_ipv4address(const WCHAR **ptr, parse_data *data) {
    const BOOL is_unknown = data->scheme_type == URL_SCHEME_UNKNOWN;
    data->host = *ptr;

    if(!check_ipv4address(ptr, FALSE)) {
        if(!check_implicit_ipv4(ptr, &data->implicit_ipv4)) {
            TRACE("(%p %p): URI didn't contain anything looking like an IPv4 address.\n", ptr, data);
            *ptr = data->host;
            data->host = NULL;
            return FALSE;
        } else
            data->has_implicit_ip = TRUE;
    }

    data->host_len = *ptr - data->host;
    data->host_type = Uri_HOST_IPV4;

    /* Check if what we found is the only part of the host name (if it isn't
     * we don't have an IPv4 address).
     */
    if(**ptr == ':') {
        ++(*ptr);
        if(!parse_port(ptr, data)) {
            *ptr = data->host;
            data->host = NULL;
            return FALSE;
        }
    } else if(!is_auth_delim(**ptr, !is_unknown)) {
        /* Found more data which belongs to the host, so this isn't an IPv4. */
        *ptr = data->host;
        data->host = NULL;
        data->has_implicit_ip = FALSE;
        return FALSE;
    }

    TRACE("(%p %p): IPv4 address found. host=%s host_len=%ld host_type=%d\n",
        ptr, data, debugstr_wn(data->host, data->host_len),
        data->host_len, data->host_type);
    return TRUE;
}

/* Attempts to parse the reg-name from the URI.
 *
 * Because of the way Windows handles ':' this function also
 * handles parsing the port.
 *
 * reg-name = *( unreserved / pct-encoded / sub-delims )
 *
 * NOTE:
 *  Windows allows everything, but, the characters in "auth_delims" and ':'
 *  to appear in a reg-name, unless it's an unknown scheme type then ':' is
 *  allowed to appear (even if a valid port isn't after it).
 *
 *  Windows doesn't like host names which start with '[' and end with ']'
 *  and don't contain a valid IP literal address in between them.
 *
 *  On Windows if a '[' is encountered in the host name the ':' no longer
 *  counts as a delimiter until you reach the next ']' or an "authority delimiter".
 *
 *  A reg-name CAN be empty.
 */
static BOOL parse_reg_name(const WCHAR **ptr, parse_data *data, DWORD extras) {
    const BOOL has_start_bracket = **ptr == '[';
    const BOOL known_scheme = data->scheme_type != URL_SCHEME_UNKNOWN;
    const BOOL is_res = data->scheme_type == URL_SCHEME_RES;
    BOOL inside_brackets = has_start_bracket;

    /* res URIs don't have ports. */
    BOOL ignore_col = (extras & IGNORE_PORT_DELIMITER) || is_res;

    /* We have to be careful with file schemes. */
    if(data->scheme_type == URL_SCHEME_FILE) {
        /* This is because an implicit file scheme could be "C:\\test" and it
         * would trick this function into thinking the host is "C", when after
         * canonicalization the host would end up being an empty string. A drive
         * path can also have a '|' instead of a ':' after the drive letter.
         */
        if(is_drive_path(*ptr)) {
            /* Regular old drive paths have no host type (or host name). */
            data->host_type = Uri_HOST_UNKNOWN;
            data->host = *ptr;
            data->host_len = 0;
            return TRUE;
        } else if(is_unc_path(*ptr))
            /* Skip past the "\\" of a UNC path. */
            *ptr += 2;
    }

    data->host = *ptr;

    /* For res URIs, everything before the first '/' is
     * considered the host.
     */
    while((!is_res && !is_auth_delim(**ptr, known_scheme)) ||
          (is_res && **ptr && **ptr != '/')) {
        if(**ptr == ':' && !ignore_col) {
            /* We can ignore ':' if we are inside brackets.*/
            if(!inside_brackets) {
                const WCHAR *tmp = (*ptr)++;

                /* Attempt to parse the port. */
                if(!parse_port(ptr, data)) {
                    /* Windows expects there to be a valid port for known scheme types. */
                    if(data->scheme_type != URL_SCHEME_UNKNOWN) {
                        *ptr = data->host;
                        data->host = NULL;
                        TRACE("(%p %p %lx): Expected valid port\n", ptr, data, extras);
                        return FALSE;
                    } else
                        /* Windows gives up on trying to parse a port when it
                         * encounters an invalid port.
                         */
                        ignore_col = TRUE;
                } else {
                    data->host_len = tmp - data->host;
                    break;
                }
            }
        } else if(**ptr == '%' && (known_scheme && !is_res)) {
            /* Has to be a legit % encoded value. */
            if(!check_pct_encoded(ptr)) {
                *ptr = data->host;
                data->host = NULL;
                return FALSE;
            } else
                continue;
        } else if(is_res && is_forbidden_dos_path_char(**ptr)) {
            *ptr = data->host;
            data->host = NULL;
            return FALSE;
        } else if(**ptr == ']')
            inside_brackets = FALSE;
        else if(**ptr == '[')
            inside_brackets = TRUE;

        ++(*ptr);
    }

    if(has_start_bracket) {
        /* Make sure the last character of the host wasn't a ']'. */
        if(*(*ptr-1) == ']') {
            TRACE("(%p %p %lx): Expected an IP literal inside of the host\n", ptr, data, extras);
            *ptr = data->host;
            data->host = NULL;
            return FALSE;
        }
    }

    /* Don't overwrite our length if we found a port earlier. */
    if(!data->port)
        data->host_len = *ptr - data->host;

    /* If the host is empty, then it's an unknown host type. */
    if(data->host_len == 0 || is_res)
        data->host_type = Uri_HOST_UNKNOWN;
    else {
        unsigned int i;

        data->host_type = Uri_HOST_DNS;

        for(i = 0; i < data->host_len; i++) {
            if(!is_ascii(data->host[i])) {
                data->host_type = Uri_HOST_IDN;
                break;
            }
        }
    }

    TRACE("(%p %p %lx): Parsed reg-name. host=%s len=%ld type=%d\n", ptr, data, extras,
          debugstr_wn(data->host, data->host_len), data->host_len, data->host_type);
    return TRUE;
}

/* Attempts to parse an IPv6 address out of the URI.
 *
 * IPv6address =                               6( h16 ":" ) ls32
 *                /                       "::" 5( h16 ":" ) ls32
 *                / [               h16 ] "::" 4( h16 ":" ) ls32
 *                / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
 *                / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
 *                / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
 *                / [ *4( h16 ":" ) h16 ] "::"              ls32
 *                / [ *5( h16 ":" ) h16 ] "::"              h16
 *                / [ *6( h16 ":" ) h16 ] "::"
 *
 * ls32        = ( h16 ":" h16 ) / IPv4address
 *             ; least-significant 32 bits of address.
 *
 * h16         = 1*4HEXDIG
 *             ; 16 bits of address represented in hexadecimal.
 */
static BOOL parse_ipv6address(const WCHAR **ptr, parse_data *data) {
    const WCHAR *terminator;

    if(RtlIpv6StringToAddressW(*ptr, &terminator, &data->ipv6_address))
        return FALSE;
    if(*terminator != ']' && !is_auth_delim(*terminator, data->scheme_type != URL_SCHEME_UNKNOWN))
        return FALSE;

    *ptr = terminator;
    data->host_type = Uri_HOST_IPV6;
    return TRUE;
}

/*  IPvFuture  = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" ) */
static BOOL parse_ipvfuture(const WCHAR **ptr, parse_data *data) {
    const WCHAR *start = *ptr;

    /* IPvFuture has to start with a 'v' or 'V'. */
    if(**ptr != 'v' && **ptr != 'V')
        return FALSE;

    /* Following the v there must be at least 1 hex digit. */
    ++(*ptr);
    if(!is_hexdigit(**ptr)) {
        *ptr = start;
        return FALSE;
    }

    ++(*ptr);
    while(is_hexdigit(**ptr))
        ++(*ptr);

    /* End of the hexdigit sequence must be a '.' */
    if(**ptr != '.') {
        *ptr = start;
        return FALSE;
    }

    ++(*ptr);
    if(!is_unreserved(**ptr) && !is_subdelim(**ptr) && **ptr != ':') {
        *ptr = start;
        return FALSE;
    }

    ++(*ptr);
    while(is_unreserved(**ptr) || is_subdelim(**ptr) || **ptr == ':')
        ++(*ptr);

    data->host_type = Uri_HOST_UNKNOWN;

    TRACE("(%p %p): Parsed IPvFuture address %s len=%d\n", ptr, data,
          debugstr_wn(start, *ptr-start), (int)(*ptr-start));

    return TRUE;
}

/* IP-literal = "[" ( IPv6address / IPvFuture  ) "]" */
static BOOL parse_ip_literal(const WCHAR **ptr, parse_data *data, DWORD extras) {
    data->host = *ptr;

    if(**ptr != '[' && !(extras & ALLOW_BRACKETLESS_IP_LITERAL)) {
        data->host = NULL;
        return FALSE;
    } else if(**ptr == '[')
        ++(*ptr);

    if(!parse_ipv6address(ptr, data)) {
        if(extras & SKIP_IP_FUTURE_CHECK || !parse_ipvfuture(ptr, data)) {
            *ptr = data->host;
            data->host = NULL;
            return FALSE;
        }
    }

    if(**ptr != ']' && !(extras & ALLOW_BRACKETLESS_IP_LITERAL)) {
        *ptr = data->host;
        data->host = NULL;
        return FALSE;
    } else if(!**ptr && extras & ALLOW_BRACKETLESS_IP_LITERAL) {
        /* The IP literal didn't contain brackets and was followed by
         * a NULL terminator, so no reason to even check the port.
         */
        data->host_len = *ptr - data->host;
        return TRUE;
    }

    ++(*ptr);
    if(**ptr == ':') {
        ++(*ptr);
        /* If a valid port is not found, then let it trickle down to
         * parse_reg_name.
         */
        if(!parse_port(ptr, data)) {
            *ptr = data->host;
            data->host = NULL;
            return FALSE;
        }
    } else
        data->host_len = *ptr - data->host;

    return TRUE;
}

/* Parses the host information from the URI.
 *
 * host = IP-literal / IPv4address / reg-name
 */
static BOOL parse_host(const WCHAR **ptr, parse_data *data, DWORD extras) {
    if(!parse_ip_literal(ptr, data, extras)) {
        if(!parse_ipv4address(ptr, data)) {
            if(!parse_reg_name(ptr, data, extras)) {
                TRACE("(%p %p %lx): Malformed URI, Unknown host type.\n",  ptr, data, extras);
                return FALSE;
            }
        }
    }

    return TRUE;
}

/* Parses the authority information from the URI.
 *
 * authority   = [ userinfo "@" ] host [ ":" port ]
 */
static BOOL parse_authority(const WCHAR **ptr, parse_data *data, DWORD flags) {
    parse_userinfo(ptr, data, flags);

    /* Parsing the port will happen during one of the host parsing
     * routines (if the URI has a port).
     */
    if(!parse_host(ptr, data, 0))
        return FALSE;

    return TRUE;
}

/* Attempts to parse the path information of a hierarchical URI. */
static BOOL parse_path_hierarchical(const WCHAR **ptr, parse_data *data, DWORD flags) {
    const WCHAR *start = *ptr;
    const BOOL is_file = data->scheme_type == URL_SCHEME_FILE;

    if(is_path_delim(data->scheme_type, **ptr)) {
        if(data->scheme_type == URL_SCHEME_WILDCARD && !data->must_have_path) {
            data->path = NULL;
            data->path_len = 0;
        } else if(!(flags & Uri_CREATE_NO_CANONICALIZE)) {
            /* If the path component is empty, then a '/' is added. */
            data->path = L"/";
            data->path_len = 1;
        }
    } else {
        while(!is_path_delim(data->scheme_type, **ptr)) {
            if(**ptr == '%' && data->scheme_type != URL_SCHEME_UNKNOWN && !is_file) {
                if(!check_pct_encoded(ptr)) {
                    *ptr = start;
                    return FALSE;
                } else
                    continue;
            } else if(is_forbidden_dos_path_char(**ptr) && is_file &&
                      (flags & Uri_CREATE_FILE_USE_DOS_PATH)) {
                /* File schemes with USE_DOS_PATH set aren't allowed to have
                 * a '<' or '>' or '\"' appear in them.
                 */
                *ptr = start;
                return FALSE;
            } else if(**ptr == '\\') {
                /* Not allowed to have a backslash if NO_CANONICALIZE is set
                 * and the scheme is known type (but not a file scheme).
                 */
                if(flags & Uri_CREATE_NO_CANONICALIZE) {
                    if(data->scheme_type != URL_SCHEME_FILE &&
                       data->scheme_type != URL_SCHEME_UNKNOWN) {
                        *ptr = start;
                        return FALSE;
                    }
                }
            }

            ++(*ptr);
        }

        /* The only time a URI doesn't have a path is when
         * the NO_CANONICALIZE flag is set and the raw URI
         * didn't contain one.
         */
        if(*ptr == start) {
            data->path = NULL;
            data->path_len = 0;
        } else {
            data->path = start;
            data->path_len = *ptr - start;
        }
    }

    if(data->path)
        TRACE("(%p %p %lx): Parsed path %s len=%ld\n", ptr, data, flags,
            debugstr_wn(data->path, data->path_len), data->path_len);
    else
        TRACE("(%p %p %lx): The URI contained no path\n", ptr, data, flags);

    return TRUE;
}

/* Parses the path of an opaque URI (much less strict than the parser
 * for a hierarchical URI).
 *
 * NOTE:
 *  Windows allows invalid % encoded data to appear in opaque URI paths
 *  for unknown scheme types.
 *
 *  File schemes with USE_DOS_PATH set aren't allowed to have '<', '>', or '\"'
 *  appear in them.
 */
static BOOL parse_path_opaque(const WCHAR **ptr, parse_data *data, DWORD flags) {
    const BOOL known_scheme = data->scheme_type != URL_SCHEME_UNKNOWN;
    const BOOL is_file = data->scheme_type == URL_SCHEME_FILE;
    const BOOL is_mailto = data->scheme_type == URL_SCHEME_MAILTO;

    if (is_mailto && (*ptr)[0] == '/' && (*ptr)[1] == '/')
    {
        if ((*ptr)[2]) data->path = *ptr + 2;
        else data->path = NULL;
    }
    else
        data->path = *ptr;

    while(!is_path_delim(data->scheme_type, **ptr)) {
        if(**ptr == '%' && known_scheme) {
            if(!check_pct_encoded(ptr)) {
                *ptr = data->path;
                data->path = NULL;
                return FALSE;
            } else
                continue;
        } else if(is_forbidden_dos_path_char(**ptr) && is_file &&
                  (flags & Uri_CREATE_FILE_USE_DOS_PATH)) {
            *ptr = data->path;
            data->path = NULL;
            return FALSE;
        }

        ++(*ptr);
    }

    if (data->path) data->path_len = *ptr - data->path;
    TRACE("(%p %p %lx): Parsed opaque URI path %s len=%ld\n", ptr, data, flags,
        debugstr_wn(data->path, data->path_len), data->path_len);
    return TRUE;
}

/* Determines how the URI should be parsed after the scheme information.
 *
 * If the scheme is followed by "//", then it is treated as a hierarchical URI
 * which then the authority and path information will be parsed out. Otherwise, the
 * URI will be treated as an opaque URI which the authority information is not parsed
 * out.
 *
 * RFC 3896 definition of hier-part:
 *
 * hier-part   = "//" authority path-abempty
 *                 / path-absolute
 *                 / path-rootless
 *                 / path-empty
 *
 * MSDN opaque URI definition:
 *  scheme ":" path [ "#" fragment ]
 *
 * NOTES:
 *  If the URI is of an unknown scheme type and has a "//" following the scheme then it
 *  is treated as a hierarchical URI, but, if the CREATE_NO_CRACK_UNKNOWN_SCHEMES flag is
 *  set then it is considered an opaque URI regardless of what follows the scheme information
 *  (per MSDN documentation).
 */
static BOOL parse_hierpart(const WCHAR **ptr, parse_data *data, DWORD flags) {
    const WCHAR *start = *ptr;

    data->must_have_path = FALSE;

    /* For javascript: URIs, simply set everything as a path */
    if(data->scheme_type == URL_SCHEME_JAVASCRIPT) {
        data->path = *ptr;
        data->path_len = lstrlenW(*ptr);
        data->is_opaque = TRUE;
        *ptr += data->path_len;
        return TRUE;
    }

    /* Checks if the authority information needs to be parsed. */
    if(is_hierarchical_uri(ptr, data)) {
        /* Only treat it as a hierarchical URI if the scheme_type is known or
         * the Uri_CREATE_NO_CRACK_UNKNOWN_SCHEMES flag is not set.
         */
        if(data->scheme_type != URL_SCHEME_UNKNOWN ||
           !(flags & Uri_CREATE_NO_CRACK_UNKNOWN_SCHEMES)) {
            TRACE("(%p %p %lx): Treating URI as an hierarchical URI.\n", ptr, data, flags);
            data->is_opaque = FALSE;

            if(data->scheme_type == URL_SCHEME_WILDCARD && !data->has_implicit_scheme) {
                if(**ptr == '/' && *(*ptr+1) == '/') {
                    data->must_have_path = TRUE;
                    *ptr += 2;
                }
            }

            /* TODO: Handle hierarchical URI's, parse authority then parse the path. */
            if(!parse_authority(ptr, data, flags))
                return FALSE;

            return parse_path_hierarchical(ptr, data, flags);
        } else
            /* Reset ptr to its starting position so opaque path parsing
             * begins at the correct location.
             */
            *ptr = start;
    }

    /* If it reaches here, then the URI will be treated as an opaque
     * URI.
     */

    TRACE("(%p %p %lx): Treating URI as an opaque URI.\n", ptr, data, flags);

    data->is_opaque = TRUE;
    if(!parse_path_opaque(ptr, data, flags))
        return FALSE;

    return TRUE;
}

/* Attempts to parse the query string from the URI.
 *
 * NOTES:
 *  If NO_DECODE_EXTRA_INFO flag is set, then invalid percent encoded
 *  data is allowed to appear in the query string. For unknown scheme types
 *  invalid percent encoded data is allowed to appear regardless.
 */
static BOOL parse_query(const WCHAR **ptr, parse_data *data, DWORD flags) {
    const BOOL known_scheme = data->scheme_type != URL_SCHEME_UNKNOWN;

    if(**ptr != '?') {
        TRACE("(%p %p %lx): URI didn't contain a query string.\n", ptr, data, flags);
        return TRUE;
    }

    data->query = *ptr;

    ++(*ptr);
    while(**ptr && **ptr != '#') {
        if(**ptr == '%' && known_scheme &&
           !(flags & Uri_CREATE_NO_DECODE_EXTRA_INFO)) {
            if(!check_pct_encoded(ptr)) {
                *ptr = data->query;
                data->query = NULL;
                return FALSE;
            } else
                continue;
        }

        ++(*ptr);
    }

    data->query_len = *ptr - data->query;

    TRACE("(%p %p %lx): Parsed query string %s len=%ld\n", ptr, data, flags,
        debugstr_wn(data->query, data->query_len), data->query_len);
    return TRUE;
}

/* Attempts to parse the fragment from the URI.
 *
 * NOTES:
 *  If NO_DECODE_EXTRA_INFO flag is set, then invalid percent encoded
 *  data is allowed to appear in the query string. For unknown scheme types
 *  invalid percent encoded data is allowed to appear regardless.
 */
static BOOL parse_fragment(const WCHAR **ptr, parse_data *data, DWORD flags) {
    const BOOL known_scheme = data->scheme_type != URL_SCHEME_UNKNOWN;

    if(**ptr != '#') {
        TRACE("(%p %p %lx): URI didn't contain a fragment.\n", ptr, data, flags);
        return TRUE;
    }

    data->fragment = *ptr;

    ++(*ptr);
    while(**ptr) {
        if(**ptr == '%' && known_scheme &&
           !(flags & Uri_CREATE_NO_DECODE_EXTRA_INFO)) {
            if(!check_pct_encoded(ptr)) {
                *ptr = data->fragment;
                data->fragment = NULL;
                return FALSE;
            } else
                continue;
        }

        ++(*ptr);
    }

    data->fragment_len = *ptr - data->fragment;

    TRACE("(%p %p %lx): Parsed fragment %s len=%ld\n", ptr, data, flags,
        debugstr_wn(data->fragment, data->fragment_len), data->fragment_len);
    return TRUE;
}

/* Parses and validates the components of the specified by data->uri
 * and stores the information it parses into 'data'.
 *
 * Returns TRUE if it successfully parsed the URI. False otherwise.
 */
static BOOL parse_uri(parse_data *data, DWORD flags) {
    const WCHAR *ptr;
    const WCHAR **pptr;

    ptr = data->uri;
    pptr = &ptr;

    TRACE("(%p %lx): BEGINNING TO PARSE URI %s.\n", data, flags, debugstr_w(data->uri));

    if(!parse_scheme(pptr, data, flags, 0))
        return FALSE;

    if(!parse_hierpart(pptr, data, flags))
        return FALSE;

    if(!parse_query(pptr, data, flags))
        return FALSE;

    if(!parse_fragment(pptr, data, flags))
        return FALSE;

    TRACE("(%p %lx): FINISHED PARSING URI.\n", data, flags);
    return TRUE;
}

static BOOL canonicalize_username(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    const WCHAR *ptr;

    if(!data->username) {
        uri->userinfo_start = -1;
        return TRUE;
    }

    uri->userinfo_start = uri->canon_len;
    for(ptr = data->username; ptr < data->username+data->username_len; ++ptr) {
        if(*ptr == '%') {
            /* Only decode % encoded values for known scheme types. */
            if(data->scheme_type != URL_SCHEME_UNKNOWN) {
                /* See if the value really needs decoding. */
                WCHAR val = decode_pct_val(ptr);
                if(is_unreserved(val)) {
                    if(!computeOnly)
                        uri->canon_uri[uri->canon_len] = val;

                    ++uri->canon_len;

                    /* Move pass the hex characters. */
                    ptr += 2;
                    continue;
                }
            }
        } else if(is_ascii(*ptr) && !is_reserved(*ptr) && !is_unreserved(*ptr) && *ptr != '\\') {
            /* Only percent encode forbidden characters if the NO_ENCODE_FORBIDDEN_CHARACTERS flag
             * is NOT set.
             */
            if(!(flags & Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS)) {
                if(!computeOnly)
                    pct_encode_val(*ptr, uri->canon_uri + uri->canon_len);

                uri->canon_len += 3;
                continue;
            }
        }

        if(!computeOnly)
            /* Nothing special, so just copy the character over. */
            uri->canon_uri[uri->canon_len] = *ptr;
        ++uri->canon_len;
    }

    return TRUE;
}

static BOOL canonicalize_password(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    const WCHAR *ptr;

    if(!data->password) {
        uri->userinfo_split = -1;
        return TRUE;
    }

    if(uri->userinfo_start == -1)
        /* Has a password, but, doesn't have a username. */
        uri->userinfo_start = uri->canon_len;

    uri->userinfo_split = uri->canon_len - uri->userinfo_start;

    /* Add the ':' to the userinfo component. */
    if(!computeOnly)
        uri->canon_uri[uri->canon_len] = ':';
    ++uri->canon_len;

    for(ptr = data->password; ptr < data->password+data->password_len; ++ptr) {
        if(*ptr == '%') {
            /* Only decode % encoded values for known scheme types. */
            if(data->scheme_type != URL_SCHEME_UNKNOWN) {
                /* See if the value really needs decoding. */
                WCHAR val = decode_pct_val(ptr);
                if(is_unreserved(val)) {
                    if(!computeOnly)
                        uri->canon_uri[uri->canon_len] = val;

                    ++uri->canon_len;

                    /* Move pass the hex characters. */
                    ptr += 2;
                    continue;
                }
            }
        } else if(is_ascii(*ptr) && !is_reserved(*ptr) && !is_unreserved(*ptr) && *ptr != '\\') {
            /* Only percent encode forbidden characters if the NO_ENCODE_FORBIDDEN_CHARACTERS flag
             * is NOT set.
             */
            if(!(flags & Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS)) {
                if(!computeOnly)
                    pct_encode_val(*ptr, uri->canon_uri + uri->canon_len);

                uri->canon_len += 3;
                continue;
            }
        }

        if(!computeOnly)
            /* Nothing special, so just copy the character over. */
            uri->canon_uri[uri->canon_len] = *ptr;
        ++uri->canon_len;
    }

    return TRUE;
}

/* Canonicalizes the userinfo of the URI represented by the parse_data.
 *
 * Canonicalization of the userinfo is a simple process. If there are any percent
 * encoded characters that fall in the "unreserved" character set, they are decoded
 * to their actual value. If a character is not in the "unreserved" or "reserved" sets
 * then it is percent encoded. Other than that the characters are copied over without
 * change.
 */
static BOOL canonicalize_userinfo(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    uri->userinfo_start = uri->userinfo_split = -1;
    uri->userinfo_len = 0;

    if(!data->username && !data->password)
        /* URI doesn't have userinfo, so nothing to do here. */
        return TRUE;

    if(!canonicalize_username(data, uri, flags, computeOnly))
        return FALSE;

    if(!canonicalize_password(data, uri, flags, computeOnly))
        return FALSE;

    uri->userinfo_len = uri->canon_len - uri->userinfo_start;
    if(!computeOnly)
        TRACE("(%p %p %lx %d): Canonicalized userinfo, userinfo_start=%d, userinfo=%s, userinfo_split=%d userinfo_len=%ld.\n",
                data, uri, flags, computeOnly, uri->userinfo_start, debugstr_wn(uri->canon_uri + uri->userinfo_start, uri->userinfo_len),
                uri->userinfo_split, uri->userinfo_len);

    /* Now insert the '@' after the userinfo. */
    if(!computeOnly)
        uri->canon_uri[uri->canon_len] = '@';
    ++uri->canon_len;

    return TRUE;
}

/* Attempts to canonicalize a reg_name.
 *
 * Things that happen:
 *  1)  If Uri_CREATE_NO_CANONICALIZE flag is not set, then the reg_name is
 *      lower cased. Unless it's an unknown scheme type, which case it's
 *      no lower cased regardless.
 *
 *  2)  Unreserved % encoded characters are decoded for known
 *      scheme types.
 *
 *  3)  Forbidden characters are % encoded as long as
 *      Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS flag is not set and
 *      it isn't an unknown scheme type.
 *
 *  4)  If it's a file scheme and the host is "localhost" it's removed.
 *
 *  5)  If it's a file scheme and Uri_CREATE_FILE_USE_DOS_PATH is set,
 *      then the UNC path characters are added before the host name.
 */
static BOOL canonicalize_reg_name(const parse_data *data, Uri *uri,
                                  DWORD flags, BOOL computeOnly) {
    const WCHAR *ptr;
    const BOOL known_scheme = data->scheme_type != URL_SCHEME_UNKNOWN;

    if(data->scheme_type == URL_SCHEME_FILE &&
       data->host_len == lstrlenW(L"localhost")) {
        if(!StrCmpNIW(data->host, L"localhost", data->host_len)) {
            uri->host_start = -1;
            uri->host_len = 0;
            uri->host_type = Uri_HOST_UNKNOWN;
            return TRUE;
        }
    }

    if(data->scheme_type == URL_SCHEME_FILE && flags & Uri_CREATE_FILE_USE_DOS_PATH) {
        if(!computeOnly) {
            uri->canon_uri[uri->canon_len] = '\\';
            uri->canon_uri[uri->canon_len+1] = '\\';
        }
        uri->canon_len += 2;
        uri->authority_start = uri->canon_len;
    }

    uri->host_start = uri->canon_len;

    for(ptr = data->host; ptr < data->host+data->host_len; ++ptr) {
        if(*ptr == '%' && known_scheme) {
            WCHAR val = decode_pct_val(ptr);
            if(is_unreserved(val)) {
                /* If NO_CANONICALIZE is not set, then windows lower cases the
                 * decoded value.
                 */
                if(!(flags & Uri_CREATE_NO_CANONICALIZE) && iswupper(val)) {
                    if(!computeOnly)
                        uri->canon_uri[uri->canon_len] = towlower(val);
                } else {
                    if(!computeOnly)
                        uri->canon_uri[uri->canon_len] = val;
                }
                ++uri->canon_len;

                /* Skip past the % encoded character. */
                ptr += 2;
                continue;
            } else {
                /* Just copy the % over. */
                if(!computeOnly)
                    uri->canon_uri[uri->canon_len] = *ptr;
                ++uri->canon_len;
            }
        } else if(*ptr == '\\') {
            /* Only unknown scheme types could have made it here with a '\\' in the host name. */
            if(!computeOnly)
                uri->canon_uri[uri->canon_len] = *ptr;
            ++uri->canon_len;
        } else if(!(flags & Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS) && is_ascii(*ptr) &&
                  !is_unreserved(*ptr) && !is_reserved(*ptr) && known_scheme) {
            if(!computeOnly) {
                pct_encode_val(*ptr, uri->canon_uri+uri->canon_len);

                /* The percent encoded value gets lower cased also. */
                if(!(flags & Uri_CREATE_NO_CANONICALIZE)) {
                    uri->canon_uri[uri->canon_len+1] = towlower(uri->canon_uri[uri->canon_len+1]);
                    uri->canon_uri[uri->canon_len+2] = towlower(uri->canon_uri[uri->canon_len+2]);
                }
            }

            uri->canon_len += 3;
        } else {
            if(!computeOnly) {
                if(!(flags & Uri_CREATE_NO_CANONICALIZE) && known_scheme)
                    uri->canon_uri[uri->canon_len] = towlower(*ptr);
                else
                    uri->canon_uri[uri->canon_len] = *ptr;
            }

            ++uri->canon_len;
        }
    }

    uri->host_len = uri->canon_len - uri->host_start;

    if(!computeOnly)
        TRACE("(%p %p %lx %d): Canonicalize reg_name=%s len=%ld\n", data, uri, flags,
            computeOnly, debugstr_wn(uri->canon_uri+uri->host_start, uri->host_len),
            uri->host_len);

    if(!computeOnly)
        find_domain_name(uri->canon_uri+uri->host_start, uri->host_len,
            &(uri->domain_offset));

    return TRUE;
}

/* Attempts to canonicalize an implicit IPv4 address. */
static BOOL canonicalize_implicit_ipv4address(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    uri->host_start = uri->canon_len;

    TRACE("%u\n", data->implicit_ipv4);
    /* For unknown scheme types Windows doesn't convert
     * the value into an IP address, but it still considers
     * it an IPv4 address.
     */
    if(data->scheme_type == URL_SCHEME_UNKNOWN) {
        if(!computeOnly)
            memcpy(uri->canon_uri+uri->canon_len, data->host, data->host_len*sizeof(WCHAR));
        uri->canon_len += data->host_len;
    } else {
        if(!computeOnly)
            uri->canon_len += ui2ipv4(uri->canon_uri+uri->canon_len, data->implicit_ipv4);
        else
            uri->canon_len += ui2ipv4(NULL, data->implicit_ipv4);
    }

    uri->host_len = uri->canon_len - uri->host_start;
    uri->host_type = Uri_HOST_IPV4;

    if(!computeOnly)
        TRACE("%p %p %lx %d): Canonicalized implicit IP address=%s len=%ld\n",
            data, uri, flags, computeOnly,
            debugstr_wn(uri->canon_uri+uri->host_start, uri->host_len),
            uri->host_len);

    return TRUE;
}

/* Attempts to canonicalize an IPv4 address.
 *
 * If the parse_data represents a URI that has an implicit IPv4 address
 * (ex. http://256/, this function will convert 256 into 0.0.1.0). If
 * the implicit IP address exceeds the value of UINT_MAX (maximum value
 * for an IPv4 address) it's canonicalized as if it were a reg-name.
 *
 * If the parse_data contains a partial or full IPv4 address it normalizes it.
 * A partial IPv4 address is something like "192.0" and would be normalized to
 * "192.0.0.0". With a full (or partial) IPv4 address like "192.002.01.003" would
 * be normalized to "192.2.1.3".
 *
 * NOTES:
 *  Windows ONLY normalizes IPv4 address for known scheme types (one that isn't
 *  URL_SCHEME_UNKNOWN). For unknown scheme types, it simply copies the data from
 *  the original URI into the canonicalized URI, but, it still recognizes URI's
 *  host type as HOST_IPV4.
 */
static BOOL canonicalize_ipv4address(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    if(data->has_implicit_ip)
        return canonicalize_implicit_ipv4address(data, uri, flags, computeOnly);
    else {
        uri->host_start = uri->canon_len;

        /* Windows only normalizes for known scheme types. */
        if(data->scheme_type != URL_SCHEME_UNKNOWN) {
            /* parse_data contains a partial or full IPv4 address, so normalize it. */
            DWORD i, octetDigitCount = 0, octetCount = 0;
            BOOL octetHasDigit = FALSE;

            for(i = 0; i < data->host_len; ++i) {
                if(data->host[i] == '0' && !octetHasDigit) {
                    /* Can ignore leading zeros if:
                     *  1) It isn't the last digit of the octet.
                     *  2) i+1 != data->host_len
                     *  3) i+1 != '.'
                     */
                    if(octetDigitCount == 2 ||
                       i+1 == data->host_len ||
                       data->host[i+1] == '.') {
                        if(!computeOnly)
                            uri->canon_uri[uri->canon_len] = data->host[i];
                        ++uri->canon_len;
                        TRACE("Adding zero\n");
                    }
                } else if(data->host[i] == '.') {
                    if(!computeOnly)
                        uri->canon_uri[uri->canon_len] = data->host[i];
                    ++uri->canon_len;

                    octetDigitCount = 0;
                    octetHasDigit = FALSE;
                    ++octetCount;
                } else {
                    if(!computeOnly)
                        uri->canon_uri[uri->canon_len] = data->host[i];
                    ++uri->canon_len;

                    ++octetDigitCount;
                    octetHasDigit = TRUE;
                }
            }

            /* Make sure the canonicalized IP address has 4 dec-octets.
             * If doesn't add "0" ones until there is 4;
             */
            for( ; octetCount < 3; ++octetCount) {
                if(!computeOnly) {
                    uri->canon_uri[uri->canon_len] = '.';
                    uri->canon_uri[uri->canon_len+1] = '0';
                }

                uri->canon_len += 2;
            }
        } else {
            /* Windows doesn't normalize addresses in unknown schemes. */
            if(!computeOnly)
                memcpy(uri->canon_uri+uri->canon_len, data->host, data->host_len*sizeof(WCHAR));
            uri->canon_len += data->host_len;
        }

        uri->host_len = uri->canon_len - uri->host_start;
        if(!computeOnly)
            TRACE("(%p %p %lx %d): Canonicalized IPv4 address, ip=%s len=%ld\n",
                data, uri, flags, computeOnly,
                debugstr_wn(uri->canon_uri+uri->host_start, uri->host_len),
                uri->host_len);
    }

    return TRUE;
}

/* Attempts to canonicalize the IPv6 address of the URI.
 *
 * Multiple things happen during the canonicalization of an IPv6 address:
 *  1)  Any leading zero's in a h16 component are removed.
 *      Ex: [0001:0022::] -> [1:22::]
 *
 *  2)  The longest sequence of zero h16 components are compressed
 *      into a "::" (elision). If there's a tie, the first is chosen.
 *
 *      Ex: [0:0:0:0:1:6:7:8]   -> [::1:6:7:8]
 *          [0:0:0:0:1:2::]     -> [::1:2:0:0]
 *          [0:0:1:2:0:0:7:8]   -> [::1:2:0:0:7:8]
 *
 *  3)  If an IPv4 address is attached to the IPv6 address, it's
 *      also normalized.
 *      Ex: [::001.002.022.000] -> [::1.2.22.0]
 *
 *  4)  If an elision is present, but, only represents one h16 component
 *      it's expanded.
 *
 *      Ex: [1::2:3:4:5:6:7] -> [1:0:2:3:4:5:6:7]
 *
 *  5)  If the IPv6 address contains an IPv4 address and there exists
 *      at least 1 non-zero h16 component the IPv4 address is converted
 *      into two h16 components, otherwise it's normalized and kept as is.
 *
 *      Ex: [::192.200.003.4]       -> [::192.200.3.4]
 *          [ffff::192.200.003.4]   -> [ffff::c0c8:3041]
 *
 * NOTE:
 *  For unknown scheme types Windows simply copies the address over without any
 *  changes.
 *
 *  IPv4 address can be included in an elision if all its components are 0's.
 */
static BOOL canonicalize_ipv6address(const parse_data *data, Uri *uri,
                                     DWORD flags, BOOL computeOnly) {
    uri->host_start = uri->canon_len;

    if(data->scheme_type == URL_SCHEME_UNKNOWN) {
        if(!computeOnly)
            memcpy(uri->canon_uri+uri->canon_len, data->host, data->host_len*sizeof(WCHAR));
        uri->canon_len += data->host_len;
    } else {
        WCHAR buffer[46];
        ULONG size = ARRAY_SIZE(buffer);

        if(computeOnly) {
            RtlIpv6AddressToStringExW(&data->ipv6_address, 0, 0, buffer, &size);
            uri->canon_len += size + 1;
        } else {
            uri->canon_uri[uri->canon_len++] = '[';
            RtlIpv6AddressToStringExW(&data->ipv6_address, 0, 0, uri->canon_uri + uri->canon_len, &size);
            uri->canon_len += size - 1;
            uri->canon_uri[uri->canon_len++] = ']';
        }
    }

    uri->host_len = uri->canon_len - uri->host_start;

    if(!computeOnly)
        TRACE("(%p %p %lx %d): Canonicalized IPv6 address %s, len=%ld\n", data, uri, flags,
            computeOnly, debugstr_wn(uri->canon_uri+uri->host_start, uri->host_len),
            uri->host_len);

    return TRUE;
}

/* Attempts to canonicalize the host of the URI (if any). */
static BOOL canonicalize_host(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    uri->host_start = -1;
    uri->host_len = 0;
    uri->domain_offset = -1;

    if(data->host) {
        switch(data->host_type) {
        case Uri_HOST_DNS:
            uri->host_type = Uri_HOST_DNS;
            if(!canonicalize_reg_name(data, uri, flags, computeOnly))
                return FALSE;

            break;
        case Uri_HOST_IPV4:
            uri->host_type = Uri_HOST_IPV4;
            if(!canonicalize_ipv4address(data, uri, flags, computeOnly))
                return FALSE;

            break;
        case Uri_HOST_IPV6:
            if(!canonicalize_ipv6address(data, uri, flags, computeOnly))
                return FALSE;

            uri->host_type = Uri_HOST_IPV6;
            break;

        case Uri_HOST_IDN:
            uri->host_type = Uri_HOST_IDN;
            if(!canonicalize_reg_name(data, uri, flags, computeOnly))
                return FALSE;

            break;
        case Uri_HOST_UNKNOWN:
            if(data->host_len > 0 || data->scheme_type != URL_SCHEME_FILE) {
                uri->host_start = uri->canon_len;

                /* Nothing happens to unknown host types. */
                if(!computeOnly)
                    memcpy(uri->canon_uri+uri->canon_len, data->host, data->host_len*sizeof(WCHAR));
                uri->canon_len += data->host_len;
                uri->host_len = data->host_len;
            }

            uri->host_type = Uri_HOST_UNKNOWN;
            break;
        default:
            FIXME("(%p %p %lx %d): Canonicalization for host type %d not supported.\n", data,
                    uri, flags, computeOnly, data->host_type);
            return FALSE;
       }
   }

   return TRUE;
}

static BOOL canonicalize_port(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    BOOL has_default_port = FALSE;
    USHORT default_port = 0;
    DWORD i;

    uri->port_offset = -1;

    /* Check if the scheme has a default port. */
    for(i = 0; i < ARRAY_SIZE(default_ports); ++i) {
        if(default_ports[i].scheme == data->scheme_type) {
            has_default_port = TRUE;
            default_port = default_ports[i].port;
            break;
        }
    }

    uri->has_port = data->has_port || has_default_port;

    /* Possible cases:
     *  1)  Has a port which is the default port.
     *  2)  Has a port (not the default).
     *  3)  Doesn't have a port, but, scheme has a default port.
     *  4)  No port.
     */
    if(has_default_port && data->has_port && data->port_value == default_port) {
        /* If it's the default port and this flag isn't set, don't do anything. */
        if(flags & Uri_CREATE_NO_CANONICALIZE) {
            uri->port_offset = uri->canon_len-uri->authority_start;
            if(!computeOnly)
                uri->canon_uri[uri->canon_len] = ':';
            ++uri->canon_len;

            if(data->port) {
                /* Copy the original port over. */
                if(!computeOnly)
                    memcpy(uri->canon_uri+uri->canon_len, data->port, data->port_len*sizeof(WCHAR));
                uri->canon_len += data->port_len;
            } else {
                if(!computeOnly)
                    uri->canon_len += ui2str(uri->canon_uri+uri->canon_len, data->port_value);
                else
                    uri->canon_len += ui2str(NULL, data->port_value);
            }
        }

        uri->port = default_port;
    } else if(data->has_port) {
        uri->port_offset = uri->canon_len-uri->authority_start;
        if(!computeOnly)
            uri->canon_uri[uri->canon_len] = ':';
        ++uri->canon_len;

        if(flags & Uri_CREATE_NO_CANONICALIZE && data->port) {
            /* Copy the original over without changes. */
            if(!computeOnly)
                memcpy(uri->canon_uri+uri->canon_len, data->port, data->port_len*sizeof(WCHAR));
            uri->canon_len += data->port_len;
        } else {
            if(!computeOnly)
                uri->canon_len += ui2str(uri->canon_uri+uri->canon_len, data->port_value);
            else
                uri->canon_len += ui2str(NULL, data->port_value);
        }

        uri->port = data->port_value;
    } else if(has_default_port)
        uri->port = default_port;

    return TRUE;
}

/* Canonicalizes the authority of the URI represented by the parse_data. */
static BOOL canonicalize_authority(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    uri->authority_start = uri->canon_len;
    uri->authority_len = 0;

    if(!canonicalize_userinfo(data, uri, flags, computeOnly))
        return FALSE;

    if(!canonicalize_host(data, uri, flags, computeOnly))
        return FALSE;

    if(!canonicalize_port(data, uri, flags, computeOnly))
        return FALSE;

    if(uri->host_start != -1 || (data->is_relative && (data->password || data->username)))
        uri->authority_len = uri->canon_len - uri->authority_start;
    else
        uri->authority_start = -1;

    return TRUE;
}

/* Attempts to canonicalize the path of a hierarchical URI.
 *
 * Things that happen:
 *  1). Forbidden characters are percent encoded, unless the NO_ENCODE_FORBIDDEN
 *      flag is set or it's a file URI. Forbidden characters are always encoded
 *      for file schemes regardless and forbidden characters are never encoded
 *      for unknown scheme types.
 *
 *  2). For known scheme types '\\' are changed to '/'.
 *
 *  3). Percent encoded, unreserved characters are decoded to their actual values.
 *      Unless the scheme type is unknown. For file schemes any percent encoded
 *      character in the unreserved or reserved set is decoded.
 *
 *  4). For File schemes if the path is starts with a drive letter and doesn't
 *      start with a '/' then one is appended.
 *      Ex: file://c:/test.mp3 -> file:///c:/test.mp3
 *
 *  5). Dot segments are removed from the path for all scheme types
 *      unless NO_CANONICALIZE flag is set. Dot segments aren't removed
 *      for wildcard scheme types.
 *
 * NOTES:
 *      file://c:/test%20test   -> file:///c:/test%2520test
 *      file://c:/test%3Etest   -> file:///c:/test%253Etest
 * if Uri_CREATE_FILE_USE_DOS_PATH is not set:
 *      file:///c:/test%20test  -> file:///c:/test%20test
 *      file:///c:/test%test    -> file:///c:/test%25test
 */
static DWORD canonicalize_path_hierarchical(const WCHAR *path, DWORD path_len, URL_SCHEME scheme_type, BOOL has_host, DWORD flags,
        BOOL is_implicit_scheme, WCHAR *ret_path) {
    const BOOL known_scheme = scheme_type != URL_SCHEME_UNKNOWN;
    const BOOL is_file = scheme_type == URL_SCHEME_FILE;
    const BOOL is_res = scheme_type == URL_SCHEME_RES;
    const WCHAR *ptr;
    BOOL escape_pct = FALSE;
    DWORD len = 0;

    if(!path)
        return 0;

    ptr = path;

    if(is_file && !has_host) {
        /* Check if a '/' needs to be appended for the file scheme. */
        if(path_len > 1 && is_drive_path(ptr) && !(flags & Uri_CREATE_FILE_USE_DOS_PATH)) {
            if(ret_path)
                ret_path[len] = '/';
            len++;
            escape_pct = TRUE;
        } else if(*ptr == '/') {
            if(!(flags & Uri_CREATE_FILE_USE_DOS_PATH)) {
                /* Copy the extra '/' over. */
                if(ret_path)
                    ret_path[len] = '/';
                len++;
            }
            ++ptr;
        }

        if(is_drive_path(ptr)) {
            if(ret_path) {
                ret_path[len] = *ptr;
                /* If there's a '|' after the drive letter, convert it to a ':'. */
                ret_path[len+1] = ':';
            }
            ptr += 2;
            len += 2;
        }
    }

    if(!is_file && *path && *path != '/') {
        /* Prepend a '/' to the path if it doesn't have one. */
        if(ret_path)
            ret_path[len] = '/';
        len++;
    }

    for(; ptr < path+path_len; ++ptr) {
        BOOL do_default_action = TRUE;

        if(*ptr == '%' && !is_res) {
            const WCHAR *tmp = ptr;
            WCHAR val;

            /* Check if the % represents a valid encoded char, or if it needs encoding. */
            BOOL force_encode = !check_pct_encoded(&tmp) && is_file && !(flags&Uri_CREATE_FILE_USE_DOS_PATH);
            val = decode_pct_val(ptr);

            if(force_encode || escape_pct) {
                /* Escape the percent sign in the file URI. */
                if(ret_path)
                    pct_encode_val(*ptr, ret_path+len);
                len += 3;
                do_default_action = FALSE;
            } else if((is_unreserved(val) && known_scheme) ||
                      (is_file && !is_implicit_scheme && (is_unreserved(val) || is_reserved(val) ||
                      (val && flags&Uri_CREATE_FILE_USE_DOS_PATH && !is_forbidden_dos_path_char(val))))) {
                if(ret_path)
                    ret_path[len] = val;
                len++;

                ptr += 2;
                continue;
            }
        } else if(*ptr == '/' && is_file && (flags & Uri_CREATE_FILE_USE_DOS_PATH)) {
            /* Convert the '/' back to a '\\'. */
            if(ret_path)
                ret_path[len] = '\\';
            len++;
            do_default_action = FALSE;
        } else if(*ptr == '\\' && known_scheme) {
            if(!(is_file && (flags & Uri_CREATE_FILE_USE_DOS_PATH))) {
                /* Convert '\\' into a '/'. */
                if(ret_path)
                    ret_path[len] = '/';
                len++;
                do_default_action = FALSE;
            }
        } else if(known_scheme && !is_res && is_ascii(*ptr) && !is_unreserved(*ptr) && !is_reserved(*ptr) &&
                  (!(flags & Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS) || is_file)) {
            if(!is_file || !(flags & Uri_CREATE_FILE_USE_DOS_PATH)) {
                /* Escape the forbidden character. */
                if(ret_path)
                    pct_encode_val(*ptr, ret_path+len);
                len += 3;
                do_default_action = FALSE;
            }
        }

        if(do_default_action) {
            if(ret_path)
                ret_path[len] = *ptr;
            len++;
        }
    }

    /* Removing the dot segments only happens when it's not in
     * computeOnly mode and it's not a wildcard scheme. File schemes
     * with USE_DOS_PATH set don't get dot segments removed.
     */
    if(!(is_file && (flags & Uri_CREATE_FILE_USE_DOS_PATH)) &&
       scheme_type != URL_SCHEME_WILDCARD) {
        if(!(flags & Uri_CREATE_NO_CANONICALIZE) && ret_path) {
            /* Remove the dot segments (if any) and reset everything to the new
             * correct length.
             */
            len = remove_dot_segments(ret_path, len);
        }
    }

    if(ret_path)
        TRACE("Canonicalized path %s len=%ld\n", debugstr_wn(ret_path, len), len);
    return len;
}

/* Attempts to canonicalize the path for an opaque URI.
 *
 * For known scheme types:
 *  1)  forbidden characters are percent encoded if
 *      NO_ENCODE_FORBIDDEN_CHARACTERS isn't set.
 *
 *  2)  Percent encoded, unreserved characters are decoded
 *      to their actual values, for known scheme types.
 *
 *  3)  '\\' are changed to '/' for known scheme types
 *      except for mailto schemes.
 *
 *  4)  For file schemes, if USE_DOS_PATH is set all '/'
 *      are converted to backslashes.
 *
 *  5)  For file schemes, if USE_DOS_PATH isn't set all '\'
 *      are converted to forward slashes.
 */
static BOOL canonicalize_path_opaque(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    const WCHAR *ptr;
    const BOOL known_scheme = data->scheme_type != URL_SCHEME_UNKNOWN;
    const BOOL is_file = data->scheme_type == URL_SCHEME_FILE;
    const BOOL is_mk = data->scheme_type == URL_SCHEME_MK;

    if(!data->path) {
        uri->path_start = -1;
        uri->path_len = 0;
        return TRUE;
    }

    uri->path_start = uri->canon_len;

    if(is_mk){
        /* hijack this flag for SCHEME_MK to tell the function when to start
         * converting slashes */
        flags |= Uri_CREATE_FILE_USE_DOS_PATH;
    }

    /* For javascript: URIs, simply copy path part without any canonicalization */
    if(data->scheme_type == URL_SCHEME_JAVASCRIPT) {
        if(!computeOnly)
            memcpy(uri->canon_uri+uri->canon_len, data->path, data->path_len*sizeof(WCHAR));
        uri->path_len = data->path_len;
        uri->canon_len += data->path_len;
        return TRUE;
    }

    /* Windows doesn't allow a "//" to appear after the scheme
     * of a URI, if it's an opaque URI.
     */
    if(data->scheme && *(data->path) == '/' && *(data->path+1) == '/') {
        /* So it inserts a "/." before the "//" if it exists. */
        if(!computeOnly) {
            uri->canon_uri[uri->canon_len] = '/';
            uri->canon_uri[uri->canon_len+1] = '.';
        }

        uri->canon_len += 2;
    }

    for(ptr = data->path; ptr < data->path+data->path_len; ++ptr) {
        BOOL do_default_action = TRUE;

        if(*ptr == '%' && known_scheme) {
            WCHAR val = decode_pct_val(ptr);

            if(is_unreserved(val)) {
                if(!computeOnly)
                    uri->canon_uri[uri->canon_len] = val;
                ++uri->canon_len;

                ptr += 2;
                continue;
            }
        } else if(*ptr == '/' && is_file && (flags & Uri_CREATE_FILE_USE_DOS_PATH)) {
            if(!computeOnly)
                uri->canon_uri[uri->canon_len] = '\\';
            ++uri->canon_len;
            do_default_action = FALSE;
        } else if(*ptr == '\\') {
            if((data->is_relative || is_mk || is_file) && !(flags & Uri_CREATE_FILE_USE_DOS_PATH)) {
                /* Convert to a '/'. */
                if(!computeOnly)
                    uri->canon_uri[uri->canon_len] = '/';
                ++uri->canon_len;
                do_default_action = FALSE;
            }
        } else if(is_mk && *ptr == ':' && ptr + 1 < data->path + data->path_len && *(ptr + 1) == ':') {
            flags &= ~Uri_CREATE_FILE_USE_DOS_PATH;
        } else if(known_scheme && is_ascii(*ptr) && !is_unreserved(*ptr) && !is_reserved(*ptr) &&
                  !(flags & Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS)) {
            if(!(is_file && (flags & Uri_CREATE_FILE_USE_DOS_PATH))) {
                if(!computeOnly)
                    pct_encode_val(*ptr, uri->canon_uri+uri->canon_len);
                uri->canon_len += 3;
                do_default_action = FALSE;
            }
        }

        if(do_default_action) {
            if(!computeOnly)
                uri->canon_uri[uri->canon_len] = *ptr;
            ++uri->canon_len;
        }
    }

    if(is_mk && !computeOnly && !(flags & Uri_CREATE_NO_CANONICALIZE)) {
        DWORD new_len = remove_dot_segments(uri->canon_uri + uri->path_start,
                                            uri->canon_len - uri->path_start);
        uri->canon_len = uri->path_start + new_len;
    }

    uri->path_len = uri->canon_len - uri->path_start;

    if(!computeOnly)
        TRACE("(%p %p %lx %d): Canonicalized opaque URI path %s len=%ld\n", data, uri, flags, computeOnly,
            debugstr_wn(uri->canon_uri+uri->path_start, uri->path_len), uri->path_len);
    return TRUE;
}

/* Determines how the URI represented by the parse_data should be canonicalized.
 *
 * Essentially, if the parse_data represents an hierarchical URI then it calls
 * canonicalize_authority and the canonicalization functions for the path. If the
 * URI is opaque it canonicalizes the path of the URI.
 */
static BOOL canonicalize_hierpart(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    if(!data->is_opaque || (data->is_relative && (data->password || data->username))) {
        /* "//" is only added for non-wildcard scheme types.
         *
         * A "//" is only added to a relative URI if it has a
         * host or port component (this only happens if a IUriBuilder
         * is generating an IUri).
         */
        if((data->is_relative && (data->host || data->has_port)) ||
           (!data->is_relative && data->scheme_type != URL_SCHEME_WILDCARD)) {
            if(data->scheme_type == URL_SCHEME_WILDCARD)
                FIXME("Here\n");

            if(!computeOnly) {
                INT pos = uri->canon_len;

                uri->canon_uri[pos] = '/';
                uri->canon_uri[pos+1] = '/';
           }
           uri->canon_len += 2;
        }

        if(!canonicalize_authority(data, uri, flags, computeOnly))
            return FALSE;

        if(data->is_relative && (data->password || data->username)) {
            if(!canonicalize_path_opaque(data, uri, flags, computeOnly))
                return FALSE;
        } else {
            if(!computeOnly)
                uri->path_start = uri->canon_len;
            uri->path_len = canonicalize_path_hierarchical(data->path, data->path_len, data->scheme_type, data->host_len != 0,
                    flags, data->has_implicit_scheme, computeOnly ? NULL : uri->canon_uri+uri->canon_len);
            uri->canon_len += uri->path_len;
            if(!computeOnly && !uri->path_len)
                uri->path_start = -1;
        }
    } else {
        /* Opaque URI's don't have an authority. */
        uri->userinfo_start = uri->userinfo_split = -1;
        uri->userinfo_len = 0;
        uri->host_start = -1;
        uri->host_len = 0;
        uri->host_type = Uri_HOST_UNKNOWN;
        uri->has_port = FALSE;
        uri->authority_start = -1;
        uri->authority_len = 0;
        uri->domain_offset = -1;
        uri->port_offset = -1;

        if(is_hierarchical_scheme(data->scheme_type)) {
            DWORD i;

            /* Absolute URIs aren't displayed for known scheme types
             * which should be hierarchical URIs.
             */
            uri->display_modifiers |= URI_DISPLAY_NO_ABSOLUTE_URI;

            /* Windows also sets the port for these (if they have one). */
            for(i = 0; i < ARRAY_SIZE(default_ports); ++i) {
                if(data->scheme_type == default_ports[i].scheme) {
                    uri->has_port = TRUE;
                    uri->port = default_ports[i].port;
                    break;
                }
            }
        }

        if(!canonicalize_path_opaque(data, uri, flags, computeOnly))
            return FALSE;
    }

    if(uri->path_start > -1 && !computeOnly)
        /* Finding file extensions happens for both types of URIs. */
        uri->extension_offset = find_file_extension(uri->canon_uri+uri->path_start, uri->path_len);
    else
        uri->extension_offset = -1;

    return TRUE;
}

/* Attempts to canonicalize the query string of the URI.
 *
 * Things that happen:
 *  1)  For known scheme types forbidden characters
 *      are percent encoded, unless the NO_DECODE_EXTRA_INFO flag is set
 *      or NO_ENCODE_FORBIDDEN_CHARACTERS is set.
 *
 *  2)  For known scheme types, percent encoded, unreserved characters
 *      are decoded as long as the NO_DECODE_EXTRA_INFO flag isn't set.
 */
static BOOL canonicalize_query(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    const WCHAR *ptr, *end;
    const BOOL known_scheme = data->scheme_type != URL_SCHEME_UNKNOWN;

    if(!data->query) {
        uri->query_start = -1;
        uri->query_len = 0;
        return TRUE;
    }

    uri->query_start = uri->canon_len;

    end = data->query+data->query_len;
    for(ptr = data->query; ptr < end; ++ptr) {
        if(*ptr == '%') {
            if(known_scheme && !(flags & Uri_CREATE_NO_DECODE_EXTRA_INFO)) {
                WCHAR val = decode_pct_val(ptr);
                if(is_unreserved(val)) {
                    if(!computeOnly)
                        uri->canon_uri[uri->canon_len] = val;
                    ++uri->canon_len;

                    ptr += 2;
                    continue;
                }
            }
        } else if(known_scheme && is_ascii(*ptr) && !is_unreserved(*ptr) && !is_reserved(*ptr)) {
            if(!(flags & Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS) &&
               !(flags & Uri_CREATE_NO_DECODE_EXTRA_INFO)) {
                if(!computeOnly)
                    pct_encode_val(*ptr, uri->canon_uri+uri->canon_len);
                uri->canon_len += 3;
                continue;
            }
        }

        if(!computeOnly)
            uri->canon_uri[uri->canon_len] = *ptr;
        ++uri->canon_len;
    }

    uri->query_len = uri->canon_len - uri->query_start;

    if(!computeOnly)
        TRACE("(%p %p %lx %d): Canonicalized query string %s len=%ld\n", data, uri, flags,
            computeOnly, debugstr_wn(uri->canon_uri+uri->query_start, uri->query_len),
            uri->query_len);
    return TRUE;
}

static BOOL canonicalize_fragment(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    const WCHAR *ptr, *end;
    const BOOL known_scheme = data->scheme_type != URL_SCHEME_UNKNOWN;

    if(!data->fragment) {
        uri->fragment_start = -1;
        uri->fragment_len = 0;
        return TRUE;
    }

    uri->fragment_start = uri->canon_len;

    end = data->fragment + data->fragment_len;
    for(ptr = data->fragment; ptr < end; ++ptr) {
        if(*ptr == '%') {
            if(known_scheme && !(flags & Uri_CREATE_NO_DECODE_EXTRA_INFO)) {
                WCHAR val = decode_pct_val(ptr);
                if(is_unreserved(val)) {
                    if(!computeOnly)
                        uri->canon_uri[uri->canon_len] = val;
                    ++uri->canon_len;

                    ptr += 2;
                    continue;
                }
            }
        } else if(known_scheme && is_ascii(*ptr) && !is_unreserved(*ptr) && !is_reserved(*ptr)) {
            if(!(flags & Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS) &&
               !(flags & Uri_CREATE_NO_DECODE_EXTRA_INFO)) {
                if(!computeOnly)
                    pct_encode_val(*ptr, uri->canon_uri+uri->canon_len);
                uri->canon_len += 3;
                continue;
            }
        }

        if(!computeOnly)
            uri->canon_uri[uri->canon_len] = *ptr;
        ++uri->canon_len;
    }

    uri->fragment_len = uri->canon_len - uri->fragment_start;

    if(!computeOnly)
        TRACE("(%p %p %lx %d): Canonicalized fragment %s len=%ld\n", data, uri, flags,
            computeOnly, debugstr_wn(uri->canon_uri+uri->fragment_start, uri->fragment_len),
            uri->fragment_len);
    return TRUE;
}

/* Canonicalizes the scheme information specified in the parse_data using the specified flags. */
static BOOL canonicalize_scheme(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    uri->scheme_start = -1;
    uri->scheme_len = 0;

    if(!data->scheme) {
        /* The only type of URI that doesn't have to have a scheme is a relative
         * URI.
         */
        if(!data->is_relative) {
            FIXME("(%p %p %lx): Unable to determine the scheme type of %s.\n", data,
                    uri, flags, debugstr_w(data->uri));
            return FALSE;
        }
    } else {
        if(!computeOnly) {
            DWORD i;
            INT pos = uri->canon_len;

            for(i = 0; i < data->scheme_len; ++i) {
                /* Scheme name must be lower case after canonicalization. */
                uri->canon_uri[i + pos] = towlower(data->scheme[i]);
            }

            uri->canon_uri[i + pos] = ':';
            uri->scheme_start = pos;

            TRACE("(%p %p %lx): Canonicalized scheme=%s, len=%ld.\n", data, uri, flags,
                    debugstr_wn(uri->canon_uri+uri->scheme_start,  data->scheme_len), data->scheme_len);
        }

        /* This happens in both computation modes. */
        uri->canon_len += data->scheme_len + 1;
        uri->scheme_len = data->scheme_len;
    }
    return TRUE;
}

/* Computes what the length of the URI specified by the parse_data will be
 * after canonicalization occurs using the specified flags.
 *
 * This function will return a non-zero value indicating the length of the canonicalized
 * URI, or -1 on error.
 */
static int compute_canonicalized_length(const parse_data *data, DWORD flags) {
    Uri uri;

    memset(&uri, 0, sizeof(Uri));

    TRACE("(%p %lx): Beginning to compute canonicalized length for URI %s\n", data, flags,
            debugstr_w(data->uri));

    if(!canonicalize_scheme(data, &uri, flags, TRUE)) {
        ERR("(%p %lx): Failed to compute URI scheme length.\n", data, flags);
        return -1;
    }

    if(!canonicalize_hierpart(data, &uri, flags, TRUE)) {
        ERR("(%p %lx): Failed to compute URI hierpart length.\n", data, flags);
        return -1;
    }

    if(!canonicalize_query(data, &uri, flags, TRUE)) {
        ERR("(%p %lx): Failed to compute query string length.\n", data, flags);
        return -1;
    }

    if(!canonicalize_fragment(data, &uri, flags, TRUE)) {
        ERR("(%p %lx): Failed to compute fragment length.\n", data, flags);
        return -1;
    }

    TRACE("(%p %lx): Finished computing canonicalized URI length. length=%ld\n", data, flags, uri.canon_len);

    return uri.canon_len;
}

/* Canonicalizes the URI data specified in the parse_data, using the given flags. If the
 * canonicalization succeeds it will store all the canonicalization information
 * in the pointer to the Uri.
 *
 * To canonicalize a URI this function first computes what the length of the URI
 * specified by the parse_data will be. Once this is done it will then perform the actual
 * canonicalization of the URI.
 */
static HRESULT canonicalize_uri(const parse_data *data, Uri *uri, DWORD flags) {
    INT len;

    uri->canon_uri = NULL;
    uri->canon_size = uri->canon_len = 0;

    TRACE("(%p %p %lx): beginning to canonicalize URI %s.\n", data, uri, flags, debugstr_w(data->uri));

    /* First try to compute the length of the URI. */
    len = compute_canonicalized_length(data, flags);
    if(len == -1) {
        ERR("(%p %p %lx): Could not compute the canonicalized length of %s.\n", data, uri, flags,
                debugstr_w(data->uri));
        return E_INVALIDARG;
    }

    uri->canon_uri = malloc((len + 1) * sizeof(WCHAR));
    if(!uri->canon_uri)
        return E_OUTOFMEMORY;

    uri->canon_size = len;
    if(!canonicalize_scheme(data, uri, flags, FALSE)) {
        ERR("(%p %p %lx): Unable to canonicalize the scheme of the URI.\n", data, uri, flags);
        return E_INVALIDARG;
    }
    uri->scheme_type = data->scheme_type;

    if(!canonicalize_hierpart(data, uri, flags, FALSE)) {
        ERR("(%p %p %lx): Unable to canonicalize the hierpart of the URI\n", data, uri, flags);
        return E_INVALIDARG;
    }

    if(!canonicalize_query(data, uri, flags, FALSE)) {
        ERR("(%p %p %lx): Unable to canonicalize query string of the URI.\n",
            data, uri, flags);
        return E_INVALIDARG;
    }

    if(!canonicalize_fragment(data, uri, flags, FALSE)) {
        ERR("(%p %p %lx): Unable to canonicalize fragment of the URI.\n",
            data, uri, flags);
        return E_INVALIDARG;
    }

    /* There's a possibility we didn't use all the space we allocated
     * earlier.
     */
    if(uri->canon_len < uri->canon_size) {
        /* This happens if the URI is hierarchical and dot
         * segments were removed from its path.
         */
        WCHAR *tmp = realloc(uri->canon_uri, (uri->canon_len + 1) * sizeof(WCHAR));
        if(!tmp)
            return E_OUTOFMEMORY;

        uri->canon_uri = tmp;
        uri->canon_size = uri->canon_len;
    }

    uri->canon_uri[uri->canon_len] = '\0';
    TRACE("(%p %p %lx): finished canonicalizing the URI. uri=%s\n", data, uri, flags, debugstr_w(uri->canon_uri));

    return S_OK;
}

static HRESULT get_builder_component(LPWSTR *component, DWORD *component_len,
                                     LPCWSTR source, DWORD source_len,
                                     LPCWSTR *output, DWORD *output_len)
{
    if(!output_len) {
        if(output)
            *output = NULL;
        return E_POINTER;
    }

    if(!output) {
        *output_len = 0;
        return E_POINTER;
    }

    if(!(*component) && source) {
        /* Allocate 'component', and copy the contents from 'source'
         * into the new allocation.
         */
        *component = malloc((source_len + 1) * sizeof(WCHAR));
        if(!(*component))
            return E_OUTOFMEMORY;

        memcpy(*component, source, source_len*sizeof(WCHAR));
        (*component)[source_len] = '\0';
        *component_len = source_len;
    }

    *output = *component;
    *output_len = *component_len;
    return *output ? S_OK : S_FALSE;
}

/* Allocates 'component' and copies the string from 'new_value' into 'component'.
 * If 'prefix' is set and 'new_value' isn't NULL, then it checks if 'new_value'
 * starts with 'prefix'. If it doesn't then 'prefix' is prepended to 'component'.
 *
 * If everything is successful, then will set 'success_flag' in 'flags'.
 */
static HRESULT set_builder_component(LPWSTR *component, DWORD *component_len, LPCWSTR new_value,
                                     WCHAR prefix, DWORD *flags, DWORD success_flag)
{
    free(*component);

    if(!new_value) {
        *component = NULL;
        *component_len = 0;
    } else {
        BOOL add_prefix = FALSE;
        DWORD len = lstrlenW(new_value);
        DWORD pos = 0;

        if(prefix && *new_value != prefix) {
            add_prefix = TRUE;
            *component = malloc((len + 2) * sizeof(WCHAR));
        } else
            *component = malloc((len + 1) * sizeof(WCHAR));

        if(!(*component))
            return E_OUTOFMEMORY;

        if(add_prefix)
            (*component)[pos++] = prefix;

        memcpy(*component+pos, new_value, (len+1)*sizeof(WCHAR));
        *component_len = len+pos;
    }

    *flags |= success_flag;
    return S_OK;
}

static void reset_builder(UriBuilder *builder) {
    if(builder->uri)
        IUri_Release(&builder->uri->IUri_iface);
    builder->uri = NULL;

    free(builder->fragment);
    builder->fragment = NULL;
    builder->fragment_len = 0;

    free(builder->host);
    builder->host = NULL;
    builder->host_len = 0;

    free(builder->password);
    builder->password = NULL;
    builder->password_len = 0;

    free(builder->path);
    builder->path = NULL;
    builder->path_len = 0;

    free(builder->query);
    builder->query = NULL;
    builder->query_len = 0;

    free(builder->scheme);
    builder->scheme = NULL;
    builder->scheme_len = 0;

    free(builder->username);
    builder->username = NULL;
    builder->username_len = 0;

    builder->has_port = FALSE;
    builder->port = 0;
    builder->modified_props = 0;
}

static HRESULT validate_scheme_name(const UriBuilder *builder, parse_data *data, DWORD flags) {
    const WCHAR *component;
    const WCHAR *ptr;
    const WCHAR **pptr;
    DWORD expected_len;

    if(builder->scheme) {
        ptr = builder->scheme;
        expected_len = builder->scheme_len;
    } else if(builder->uri && builder->uri->scheme_start > -1) {
        ptr = builder->uri->canon_uri+builder->uri->scheme_start;
        expected_len = builder->uri->scheme_len;
    } else {
        ptr = L"";
        expected_len = 0;
    }

    component = ptr;
    pptr = &ptr;
    if(parse_scheme(pptr, data, flags, ALLOW_NULL_TERM_SCHEME) &&
       data->scheme_len == expected_len) {
        if(data->scheme)
            TRACE("(%p %p %lx): Found valid scheme component %s len=%ld.\n", builder, data, flags,
               debugstr_wn(data->scheme, data->scheme_len), data->scheme_len);
    } else {
        TRACE("(%p %p %lx): Invalid scheme component found %s.\n", builder, data, flags,
            debugstr_wn(component, expected_len));
        return INET_E_INVALID_URL;
   }

    return S_OK;
}

static HRESULT validate_username(const UriBuilder *builder, parse_data *data, DWORD flags) {
    const WCHAR *ptr;
    const WCHAR **pptr;
    DWORD expected_len;

    if(builder->username) {
        ptr = builder->username;
        expected_len = builder->username_len;
    } else if(!(builder->modified_props & Uri_HAS_USER_NAME) && builder->uri &&
              builder->uri->userinfo_start > -1 && builder->uri->userinfo_split != 0) {
        /* Just use the username from the base Uri. */
        data->username = builder->uri->canon_uri+builder->uri->userinfo_start;
        data->username_len = (builder->uri->userinfo_split > -1) ?
                                        builder->uri->userinfo_split : builder->uri->userinfo_len;
        ptr = NULL;
    } else {
        ptr = NULL;
        expected_len = 0;
    }

    if(ptr) {
        const WCHAR *component = ptr;
        pptr = &ptr;
        if(parse_username(pptr, data, flags, ALLOW_NULL_TERM_USER_NAME) &&
           data->username_len == expected_len)
            TRACE("(%p %p %lx): Found valid username component %s len=%ld.\n", builder, data, flags,
                debugstr_wn(data->username, data->username_len), data->username_len);
        else {
            TRACE("(%p %p %lx): Invalid username component found %s.\n", builder, data, flags,
                debugstr_wn(component, expected_len));
            return INET_E_INVALID_URL;
        }
    }

    return S_OK;
}

static HRESULT validate_password(const UriBuilder *builder, parse_data *data, DWORD flags) {
    const WCHAR *ptr;
    const WCHAR **pptr;
    DWORD expected_len;

    if(builder->password) {
        ptr = builder->password;
        expected_len = builder->password_len;
    } else if(!(builder->modified_props & Uri_HAS_PASSWORD) && builder->uri &&
              builder->uri->userinfo_split > -1) {
        data->password = builder->uri->canon_uri+builder->uri->userinfo_start+builder->uri->userinfo_split+1;
        data->password_len = builder->uri->userinfo_len-builder->uri->userinfo_split-1;
        ptr = NULL;
    } else {
        ptr = NULL;
        expected_len = 0;
    }

    if(ptr) {
        const WCHAR *component = ptr;
        pptr = &ptr;
        if(parse_password(pptr, data, flags, ALLOW_NULL_TERM_PASSWORD) &&
           data->password_len == expected_len)
            TRACE("(%p %p %lx): Found valid password component %s len=%ld.\n", builder, data, flags,
                debugstr_wn(data->password, data->password_len), data->password_len);
        else {
            TRACE("(%p %p %lx): Invalid password component found %s.\n", builder, data, flags,
                debugstr_wn(component, expected_len));
            return INET_E_INVALID_URL;
        }
    }

    return S_OK;
}

static HRESULT validate_userinfo(const UriBuilder *builder, parse_data *data, DWORD flags) {
    HRESULT hr;

    hr = validate_username(builder, data, flags);
    if(FAILED(hr))
        return hr;

    hr = validate_password(builder, data, flags);
    if(FAILED(hr))
        return hr;

    return S_OK;
}

static HRESULT validate_host(const UriBuilder *builder, parse_data *data) {
    const WCHAR *ptr;
    const WCHAR **pptr;
    DWORD expected_len;

    if(builder->host) {
        ptr = builder->host;
        expected_len = builder->host_len;
    } else if(!(builder->modified_props & Uri_HAS_HOST) && builder->uri && builder->uri->host_start > -1) {
        ptr = builder->uri->canon_uri + builder->uri->host_start;
        expected_len = builder->uri->host_len;
    } else
        ptr = NULL;

    if(ptr) {
        const WCHAR *component = ptr;
        DWORD extras = ALLOW_BRACKETLESS_IP_LITERAL|IGNORE_PORT_DELIMITER|SKIP_IP_FUTURE_CHECK;
        pptr = &ptr;

        if(parse_host(pptr, data, extras) && data->host_len == expected_len)
            TRACE("(%p %p): Found valid host name %s len=%ld type=%d.\n", builder, data,
                debugstr_wn(data->host, data->host_len), data->host_len, data->host_type);
        else {
            TRACE("(%p %p): Invalid host name found %s.\n", builder, data,
                debugstr_wn(component, expected_len));
            return INET_E_INVALID_URL;
        }
    }

    return S_OK;
}

static void setup_port(const UriBuilder *builder, parse_data *data, DWORD flags) {
    if(builder->modified_props & Uri_HAS_PORT) {
        if(builder->has_port) {
            data->has_port = TRUE;
            data->port_value = builder->port;
        }
    } else if(builder->uri && builder->uri->has_port) {
        data->has_port = TRUE;
        data->port_value = builder->uri->port;
    }

    if(data->has_port)
        TRACE("(%p %p %lx): Using %lu as port for IUri.\n", builder, data, flags, data->port_value);
}

static HRESULT validate_path(const UriBuilder *builder, parse_data *data, DWORD flags) {
    const WCHAR *ptr = NULL;
    const WCHAR *component;
    const WCHAR **pptr;
    DWORD expected_len;
    BOOL check_len = TRUE;
    BOOL valid = FALSE;

    if(builder->path) {
        ptr = builder->path;
        expected_len = builder->path_len;
    } else if(!(builder->modified_props & Uri_HAS_PATH) &&
              builder->uri && builder->uri->path_start > -1) {
        ptr = builder->uri->canon_uri+builder->uri->path_start;
        expected_len = builder->uri->path_len;
    } else {
        ptr = L"";
        check_len = FALSE;
        expected_len = -1;
    }

    component = ptr;
    pptr = &ptr;

    /* How the path is validated depends on what type of
     * URI it is.
     */
    valid = data->is_opaque ?
        parse_path_opaque(pptr, data, flags) : parse_path_hierarchical(pptr, data, flags);

    if(!valid || (check_len && expected_len != data->path_len)) {
        TRACE("(%p %p %lx): Invalid path component %s.\n", builder, data, flags,
            debugstr_wn(component, expected_len) );
        return INET_E_INVALID_URL;
    }

    TRACE("(%p %p %lx): Valid path component %s len=%ld.\n", builder, data, flags,
        debugstr_wn(data->path, data->path_len), data->path_len);

    return S_OK;
}

static HRESULT validate_query(const UriBuilder *builder, parse_data *data, DWORD flags) {
    const WCHAR *ptr = NULL;
    const WCHAR **pptr;
    DWORD expected_len;

    if(builder->query) {
        ptr = builder->query;
        expected_len = builder->query_len;
    } else if(!(builder->modified_props & Uri_HAS_QUERY) && builder->uri &&
              builder->uri->query_start > -1) {
        ptr = builder->uri->canon_uri+builder->uri->query_start;
        expected_len = builder->uri->query_len;
    }

    if(ptr) {
        const WCHAR *component = ptr;
        pptr = &ptr;

        if(parse_query(pptr, data, flags) && expected_len == data->query_len)
            TRACE("(%p %p %lx): Valid query component %s len=%ld.\n", builder, data, flags,
                debugstr_wn(data->query, data->query_len), data->query_len);
        else {
            TRACE("(%p %p %lx): Invalid query component %s.\n", builder, data, flags,
                debugstr_wn(component, expected_len));
            return INET_E_INVALID_URL;
        }
    }

    return S_OK;
}

static HRESULT validate_fragment(const UriBuilder *builder, parse_data *data, DWORD flags) {
    const WCHAR *ptr = NULL;
    const WCHAR **pptr;
    DWORD expected_len;

    if(builder->fragment) {
        ptr = builder->fragment;
        expected_len = builder->fragment_len;
    } else if(!(builder->modified_props & Uri_HAS_FRAGMENT) && builder->uri &&
              builder->uri->fragment_start > -1) {
        ptr = builder->uri->canon_uri+builder->uri->fragment_start;
        expected_len = builder->uri->fragment_len;
    }

    if(ptr) {
        const WCHAR *component = ptr;
        pptr = &ptr;

        if(parse_fragment(pptr, data, flags) && expected_len == data->fragment_len)
            TRACE("(%p %p %lx): Valid fragment component %s len=%ld.\n", builder, data, flags,
                debugstr_wn(data->fragment, data->fragment_len), data->fragment_len);
        else {
            TRACE("(%p %p %lx): Invalid fragment component %s.\n", builder, data, flags,
                debugstr_wn(component, expected_len));
            return INET_E_INVALID_URL;
        }
    }

    return S_OK;
}

static HRESULT validate_components(const UriBuilder *builder, parse_data *data, DWORD flags) {
    HRESULT hr;

    memset(data, 0, sizeof(parse_data));

    TRACE("(%p %p %lx): Beginning to validate builder components.\n", builder, data, flags);

    hr = validate_scheme_name(builder, data, flags);
    if(FAILED(hr))
        return hr;

    /* Extra validation for file schemes. */
    if(data->scheme_type == URL_SCHEME_FILE) {
        if((builder->password || (builder->uri && builder->uri->userinfo_split > -1)) ||
           (builder->username || (builder->uri && builder->uri->userinfo_start > -1))) {
            TRACE("(%p %p %lx): File schemes can't contain a username or password.\n",
                builder, data, flags);
            return INET_E_INVALID_URL;
        }
    }

    hr = validate_userinfo(builder, data, flags);
    if(FAILED(hr))
        return hr;

    hr = validate_host(builder, data);
    if(FAILED(hr))
        return hr;

    setup_port(builder, data, flags);

    /* The URI is opaque if it doesn't have an authority component. */
    if(!data->is_relative)
        data->is_opaque = !data->username && !data->password && !data->host && !data->has_port
            && data->scheme_type != URL_SCHEME_FILE;
    else
        data->is_opaque = !data->host && !data->has_port;

    hr = validate_path(builder, data, flags);
    if(FAILED(hr))
        return hr;

    hr = validate_query(builder, data, flags);
    if(FAILED(hr))
        return hr;

    hr = validate_fragment(builder, data, flags);
    if(FAILED(hr))
        return hr;

    TRACE("(%p %p %lx): Finished validating builder components.\n", builder, data, flags);

    return S_OK;
}

static HRESULT compare_file_paths(const Uri *a, const Uri *b, BOOL *ret)
{
    WCHAR *canon_path_a, *canon_path_b;
    DWORD len_a, len_b;

    if(!a->path_len) {
        *ret = !b->path_len;
        return S_OK;
    }

    if(!b->path_len) {
        *ret = FALSE;
        return S_OK;
    }

    /* Fast path */
    if(a->path_len == b->path_len && !wcsnicmp(a->canon_uri+a->path_start, b->canon_uri+b->path_start, a->path_len)) {
        *ret = TRUE;
        return S_OK;
    }

    len_a = canonicalize_path_hierarchical(a->canon_uri+a->path_start, a->path_len, a->scheme_type, FALSE, 0, FALSE, NULL);
    len_b = canonicalize_path_hierarchical(b->canon_uri+b->path_start, b->path_len, b->scheme_type, FALSE, 0, FALSE, NULL);

    canon_path_a = malloc(len_a * sizeof(WCHAR));
    if(!canon_path_a)
        return E_OUTOFMEMORY;
    canon_path_b = malloc(len_b * sizeof(WCHAR));
    if(!canon_path_b) {
        free(canon_path_a);
        return E_OUTOFMEMORY;
    }

    len_a = canonicalize_path_hierarchical(a->canon_uri+a->path_start, a->path_len, a->scheme_type, FALSE, 0, FALSE, canon_path_a);
    len_b = canonicalize_path_hierarchical(b->canon_uri+b->path_start, b->path_len, b->scheme_type, FALSE, 0, FALSE, canon_path_b);

    *ret = len_a == len_b && !wcsnicmp(canon_path_a, canon_path_b, len_a);

    free(canon_path_a);
    free(canon_path_b);
    return S_OK;
}

/* Checks if the two Uri's are logically equivalent. It's a simple
 * comparison, since they are both of type Uri, and it can access
 * the properties of each Uri directly without the need to go
 * through the "IUri_Get*" interface calls.
 */
static HRESULT compare_uris(const Uri *a, const Uri *b, BOOL *ret) {
    const BOOL known_scheme = a->scheme_type != URL_SCHEME_UNKNOWN;
    const BOOL are_hierarchical = a->authority_start > -1 && b->authority_start > -1;
    HRESULT hres;

    *ret = FALSE;

    if(a->scheme_type != b->scheme_type)
        return S_OK;

    /* Only compare the scheme names (if any) if their unknown scheme types. */
    if(!known_scheme) {
        if((a->scheme_start > -1 && b->scheme_start > -1) &&
           (a->scheme_len == b->scheme_len)) {
            /* Make sure the schemes are the same. */
            if(StrCmpNW(a->canon_uri+a->scheme_start, b->canon_uri+b->scheme_start, a->scheme_len))
                return S_OK;
        } else if(a->scheme_len != b->scheme_len)
            /* One of the Uri's has a scheme name, while the other doesn't. */
            return S_OK;
    }

    /* If they have a userinfo component, perform case sensitive compare. */
    if((a->userinfo_start > -1 && b->userinfo_start > -1) &&
       (a->userinfo_len == b->userinfo_len)) {
        if(StrCmpNW(a->canon_uri+a->userinfo_start, b->canon_uri+b->userinfo_start, a->userinfo_len))
            return S_OK;
    } else if(a->userinfo_len != b->userinfo_len)
        /* One of the Uri's had a userinfo, while the other one doesn't. */
        return S_OK;

    /* Check if they have a host name. */
    if((a->host_start > -1 && b->host_start > -1) &&
       (a->host_len == b->host_len)) {
        /* Perform a case insensitive compare if they are a known scheme type. */
        if(known_scheme) {
            if(StrCmpNIW(a->canon_uri+a->host_start, b->canon_uri+b->host_start, a->host_len))
                return S_OK;
        } else if(StrCmpNW(a->canon_uri+a->host_start, b->canon_uri+b->host_start, a->host_len))
            return S_OK;
    } else if(a->host_len != b->host_len)
        /* One of the Uri's had a host, while the other one didn't. */
        return S_OK;

    if(a->has_port && b->has_port) {
        if(a->port != b->port)
            return S_OK;
    } else if(a->has_port || b->has_port)
        /* One had a port, while the other one didn't. */
        return S_OK;

    /* Windows is weird with how it handles paths. For example
     * One URI could be "http://google.com" (after canonicalization)
     * and one could be "http://google.com/" and the IsEqual function
     * would still evaluate to TRUE, but, only if they are both hierarchical
     * URIs.
     */
    if(a->scheme_type == URL_SCHEME_FILE) {
        BOOL cmp;

        hres = compare_file_paths(a, b, &cmp);
        if(FAILED(hres) || !cmp)
            return hres;
    } else if((a->path_start > -1 && b->path_start > -1) &&
       (a->path_len == b->path_len)) {
        if(StrCmpNW(a->canon_uri+a->path_start, b->canon_uri+b->path_start, a->path_len))
            return S_OK;
    } else if(are_hierarchical && a->path_len == -1 && b->path_len == 0) {
        if(*(a->canon_uri+a->path_start) != '/')
            return S_OK;
    } else if(are_hierarchical && b->path_len == 1 && a->path_len == 0) {
        if(*(b->canon_uri+b->path_start) != '/')
            return S_OK;
    } else if(a->path_len != b->path_len)
        return S_OK;

    /* Compare the query strings of the two URIs. */
    if((a->query_start > -1 && b->query_start > -1) &&
       (a->query_len == b->query_len)) {
        if(StrCmpNW(a->canon_uri+a->query_start, b->canon_uri+b->query_start, a->query_len))
            return S_OK;
    } else if(a->query_len != b->query_len)
        return S_OK;

    if((a->fragment_start > -1 && b->fragment_start > -1) &&
       (a->fragment_len == b->fragment_len)) {
        if(StrCmpNW(a->canon_uri+a->fragment_start, b->canon_uri+b->fragment_start, a->fragment_len))
            return S_OK;
    } else if(a->fragment_len != b->fragment_len)
        return S_OK;

    /* If we get here, the two URIs are equivalent. */
    *ret = TRUE;
    return S_OK;
}

static void convert_to_dos_path(const WCHAR *path, DWORD path_len,
                                WCHAR *output, DWORD *output_len)
{
    const WCHAR *ptr = path;

    if(path_len > 3 && *ptr == '/' && is_drive_path(path+1))
        /* Skip over the leading / before the drive path. */
        ++ptr;

    for(; ptr < path+path_len; ++ptr) {
        if(*ptr == '/') {
            if(output)
                *output++ = '\\';
            (*output_len)++;
        } else {
            if(output)
                *output++ = *ptr;
            (*output_len)++;
        }
    }
}

/* Generates a raw uri string using the parse_data. */
static DWORD generate_raw_uri(const parse_data *data, BSTR uri, DWORD flags) {
    DWORD length = 0;

    if(data->scheme) {
        if(uri) {
            memcpy(uri, data->scheme, data->scheme_len*sizeof(WCHAR));
            uri[data->scheme_len] = ':';
        }
        length += data->scheme_len+1;
    }

    if(!data->is_opaque) {
        /* For the "//" which appears before the authority component. */
        if(uri) {
            uri[length] = '/';
            uri[length+1] = '/';
        }
        length += 2;

        /* Check if we need to add the "\\" before the host name
         * of a UNC server name in a DOS path.
         */
        if(flags & RAW_URI_CONVERT_TO_DOS_PATH &&
           data->scheme_type == URL_SCHEME_FILE && data->host) {
            if(uri) {
                uri[length] = '\\';
                uri[length+1] = '\\';
            }
            length += 2;
        }
    }

    if(data->username) {
        if(uri)
            memcpy(uri+length, data->username, data->username_len*sizeof(WCHAR));
        length += data->username_len;
    }

    if(data->password) {
        if(uri) {
            uri[length] = ':';
            memcpy(uri+length+1, data->password, data->password_len*sizeof(WCHAR));
        }
        length += data->password_len+1;
    }

    if(data->password || data->username) {
        if(uri)
            uri[length] = '@';
        ++length;
    }

    if(data->host) {
        /* IPv6 addresses get the brackets added around them if they don't already
         * have them.
         */
        const BOOL add_brackets = data->host_type == Uri_HOST_IPV6 && *(data->host) != '[';
        if(add_brackets) {
            if(uri)
                uri[length] = '[';
            ++length;
        }

        if(uri)
            memcpy(uri+length, data->host, data->host_len*sizeof(WCHAR));
        length += data->host_len;

        if(add_brackets) {
            if(uri)
                uri[length] = ']';
            length++;
        }
    }

    if(data->has_port) {
        /* The port isn't included in the raw uri if it's the default
         * port for the scheme type.
         */
        DWORD i;
        BOOL is_default = FALSE;

        for(i = 0; i < ARRAY_SIZE(default_ports); ++i) {
            if(data->scheme_type == default_ports[i].scheme &&
               data->port_value == default_ports[i].port)
                is_default = TRUE;
        }

        if(!is_default || flags & RAW_URI_FORCE_PORT_DISP) {
            if(uri)
                uri[length] = ':';
            ++length;

            if(uri)
                length += ui2str(uri+length, data->port_value);
            else
                length += ui2str(NULL, data->port_value);
        }
    }

    /* Check if a '/' should be added before the path for hierarchical URIs. */
    if(!data->is_opaque && data->path && *(data->path) != '/') {
        if(uri)
            uri[length] = '/';
        ++length;
    }

    if(data->path) {
        if(!data->is_opaque && data->scheme_type == URL_SCHEME_FILE &&
           flags & RAW_URI_CONVERT_TO_DOS_PATH) {
            DWORD len = 0;

            if(uri)
                convert_to_dos_path(data->path, data->path_len, uri+length, &len);
            else
                convert_to_dos_path(data->path, data->path_len, NULL, &len);

            length += len;
        } else {
            if(uri)
                memcpy(uri+length, data->path, data->path_len*sizeof(WCHAR));
            length += data->path_len;
        }
    }

    if(data->query) {
        if(uri)
            memcpy(uri+length, data->query, data->query_len*sizeof(WCHAR));
        length += data->query_len;
    }

    if(data->fragment) {
        if(uri)
            memcpy(uri+length, data->fragment, data->fragment_len*sizeof(WCHAR));
        length += data->fragment_len;
    }

    if(uri)
        TRACE("(%p %p): Generated raw uri=%s len=%ld\n", data, uri, debugstr_wn(uri, length), length);
    else
        TRACE("(%p %p): Computed raw uri len=%ld\n", data, uri, length);

    return length;
}

static HRESULT generate_uri(const UriBuilder *builder, const parse_data *data, Uri *uri, DWORD flags) {
    HRESULT hr;
    DWORD length = generate_raw_uri(data, NULL, 0);
    uri->raw_uri = SysAllocStringLen(NULL, length);
    if(!uri->raw_uri)
        return E_OUTOFMEMORY;

    generate_raw_uri(data, uri->raw_uri, 0);

    hr = canonicalize_uri(data, uri, flags);
    if(FAILED(hr)) {
        if(hr == E_INVALIDARG)
            return INET_E_INVALID_URL;
        return hr;
    }

    uri->create_flags = flags;
    return S_OK;
}

static inline Uri* impl_from_IUri(IUri *iface)
{
    return CONTAINING_RECORD(iface, Uri, IUri_iface);
}

static inline void destroy_uri_obj(Uri *This)
{
    SysFreeString(This->raw_uri);
    free(This->canon_uri);
    free(This);
}

static HRESULT WINAPI Uri_QueryInterface(IUri *iface, REFIID riid, void **ppv)
{
    Uri *This = impl_from_IUri(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IUri_iface;
    }else if(IsEqualGUID(&IID_IUri, riid)) {
        TRACE("(%p)->(IID_IUri %p)\n", This, ppv);
        *ppv = &This->IUri_iface;
    }else if(IsEqualGUID(&IID_IUriBuilderFactory, riid)) {
        TRACE("(%p)->(IID_IUriBuilderFactory %p)\n", This, ppv);
        *ppv = &This->IUriBuilderFactory_iface;
    }else if(IsEqualGUID(&IID_IPersistStream, riid)) {
        TRACE("(%p)->(IID_IPersistStream %p)\n", This, ppv);
        *ppv = &This->IPersistStream_iface;
    }else if(IsEqualGUID(&IID_IMarshal, riid)) {
        TRACE("(%p)->(IID_IMarshal %p)\n", This, ppv);
        *ppv = &This->IMarshal_iface;
    }else if(IsEqualGUID(&IID_IUriObj, riid)) {
        TRACE("(%p)->(IID_IUriObj %p)\n", This, ppv);
        *ppv = This;
        return S_OK;
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Uri_AddRef(IUri *iface)
{
    Uri *This = impl_from_IUri(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI Uri_Release(IUri *iface)
{
    Uri *This = impl_from_IUri(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        destroy_uri_obj(This);

    return ref;
}

static HRESULT WINAPI Uri_GetPropertyBSTR(IUri *iface, Uri_PROPERTY uriProp, BSTR *pbstrProperty, DWORD dwFlags)
{
    Uri *This = impl_from_IUri(iface);
    HRESULT hres;
    TRACE("(%p %s)->(%d %p %lx)\n", This, debugstr_w(This->canon_uri), uriProp, pbstrProperty, dwFlags);

    if(!This->create_flags)
        return E_UNEXPECTED;
    if(!pbstrProperty)
        return E_POINTER;

    if(uriProp > Uri_PROPERTY_STRING_LAST) {
        /* It only returns S_FALSE for the ZONE property... */
        if(uriProp == Uri_PROPERTY_ZONE) {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            if(!(*pbstrProperty))
                return E_OUTOFMEMORY;
            return S_FALSE;
        }

        *pbstrProperty = NULL;
        return E_INVALIDARG;
    }

    if(dwFlags != 0 && dwFlags != Uri_DISPLAY_NO_FRAGMENT && dwFlags != Uri_PUNYCODE_IDN_HOST
       && dwFlags != Uri_DISPLAY_IDN_HOST)
        return E_INVALIDARG;

    if((dwFlags == Uri_DISPLAY_NO_FRAGMENT && uriProp != Uri_PROPERTY_DISPLAY_URI)
       || (dwFlags == Uri_PUNYCODE_IDN_HOST && uriProp != Uri_PROPERTY_ABSOLUTE_URI
           && uriProp != Uri_PROPERTY_DOMAIN && uriProp != Uri_PROPERTY_HOST)
       || (dwFlags == Uri_DISPLAY_IDN_HOST && uriProp != Uri_PROPERTY_ABSOLUTE_URI
           && uriProp != Uri_PROPERTY_DOMAIN && uriProp != Uri_PROPERTY_HOST))
        return E_INVALIDARG;

    switch(uriProp) {
    case Uri_PROPERTY_ABSOLUTE_URI:
        if(This->display_modifiers & URI_DISPLAY_NO_ABSOLUTE_URI) {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            hres = S_FALSE;
        }
        /* Uri_PUNYCODE_IDN_HOST doesn't remove user info containing only "@" and ":@" */
        else if (dwFlags == Uri_PUNYCODE_IDN_HOST && This->host_type == Uri_HOST_IDN && This->host_start > -1) {
            unsigned int punycode_host_len;

            punycode_host_len = IdnToAscii(0, This->canon_uri+This->host_start, This->host_len, NULL, 0);
            *pbstrProperty = SysAllocStringLen(NULL, This->canon_len-This->host_len+punycode_host_len);
            hres = S_OK;
            if(*pbstrProperty) {
                memcpy(*pbstrProperty, This->canon_uri, This->host_start*sizeof(WCHAR));
                IdnToAscii(0, This->canon_uri+This->host_start, This->host_len, *pbstrProperty+This->host_start, punycode_host_len);
                memcpy(*pbstrProperty+This->host_start+punycode_host_len,
                       This->canon_uri+This->host_start+This->host_len,
                       (This->canon_len-This->host_start-This->host_len)*sizeof(WCHAR));
            }
        } else {
            if(This->scheme_type != URL_SCHEME_UNKNOWN && This->userinfo_start > -1) {
                if(This->userinfo_len == 0) {
                    /* Don't include the '@' after the userinfo component. */
                    *pbstrProperty = SysAllocStringLen(NULL, This->canon_len-1);
                    hres = S_OK;
                    if(*pbstrProperty) {
                        /* Copy everything before it. */
                        memcpy(*pbstrProperty, This->canon_uri, This->userinfo_start*sizeof(WCHAR));

                        /* And everything after it. */
                        memcpy(*pbstrProperty+This->userinfo_start, This->canon_uri+This->userinfo_start+1,
                               (This->canon_len-This->userinfo_start-1)*sizeof(WCHAR));
                    }
                } else if(This->userinfo_split == 0 && This->userinfo_len == 1) {
                    /* Don't include the ":@" */
                    *pbstrProperty = SysAllocStringLen(NULL, This->canon_len-2);
                    hres = S_OK;
                    if(*pbstrProperty) {
                        memcpy(*pbstrProperty, This->canon_uri, This->userinfo_start*sizeof(WCHAR));
                        memcpy(*pbstrProperty+This->userinfo_start, This->canon_uri+This->userinfo_start+2,
                               (This->canon_len-This->userinfo_start-2)*sizeof(WCHAR));
                    }
                } else {
                    *pbstrProperty = SysAllocString(This->canon_uri);
                    hres = S_OK;
                }
            } else {
                *pbstrProperty = SysAllocString(This->canon_uri);
                hres = S_OK;
            }
        }

        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;

        break;
    case Uri_PROPERTY_AUTHORITY:
        if(This->authority_start > -1) {
            if(This->port_offset > -1 && is_default_port(This->scheme_type, This->port) &&
               This->display_modifiers & URI_DISPLAY_NO_DEFAULT_PORT_AUTH)
                /* Don't include the port in the authority component. */
                *pbstrProperty = SysAllocStringLen(This->canon_uri+This->authority_start, This->port_offset);
            else
                *pbstrProperty = SysAllocStringLen(This->canon_uri+This->authority_start, This->authority_len);
            hres = S_OK;
        } else {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            hres = S_FALSE;
        }

        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;

        break;
    case Uri_PROPERTY_DISPLAY_URI:
        /* The Display URI contains everything except for the userinfo for known
         * scheme types.
         */
        if(This->scheme_type != URL_SCHEME_UNKNOWN && This->userinfo_start > -1) {
            unsigned int length = This->canon_len-This->userinfo_len;

            /* Skip fragment if Uri_DISPLAY_NO_FRAGMENT is specified */
            if(dwFlags == Uri_DISPLAY_NO_FRAGMENT && This->fragment_start > -1)
                length -= This->fragment_len;

            *pbstrProperty = SysAllocStringLen(NULL, length);

            if(*pbstrProperty) {
                /* Copy everything before the userinfo over. */
                memcpy(*pbstrProperty, This->canon_uri, This->userinfo_start*sizeof(WCHAR));

                /* Copy everything after the userinfo over. */
                length -= This->userinfo_start+1;
                memcpy(*pbstrProperty+This->userinfo_start,
                   This->canon_uri+This->userinfo_start+This->userinfo_len+1, length*sizeof(WCHAR));
            }
        } else {
            unsigned int length = This->canon_len;

            /* Skip fragment if Uri_DISPLAY_NO_FRAGMENT is specified */
            if(dwFlags == Uri_DISPLAY_NO_FRAGMENT && This->fragment_start > -1)
                length -= This->fragment_len;

            *pbstrProperty = SysAllocStringLen(This->canon_uri, length);
        }

        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;
        else
            hres = S_OK;

        break;
    case Uri_PROPERTY_DOMAIN:
        if(This->domain_offset > -1) {
            if(dwFlags == Uri_PUNYCODE_IDN_HOST && This->host_type == Uri_HOST_IDN) {
                unsigned int punycode_length;

                punycode_length = IdnToAscii(0, This->canon_uri+This->host_start+This->domain_offset,
                                             This->host_len-This->domain_offset, NULL, 0);
                *pbstrProperty = SysAllocStringLen(NULL, punycode_length);
                if (*pbstrProperty)
                    IdnToAscii(0, This->canon_uri+This->host_start+This->domain_offset,
                               This->host_len-This->domain_offset, *pbstrProperty, punycode_length);
            } else {
                *pbstrProperty = SysAllocStringLen(This->canon_uri+This->host_start+This->domain_offset,
                                                   This->host_len-This->domain_offset);
            }

            hres = S_OK;
        } else {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            hres = S_FALSE;
        }

        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;

        break;
    case Uri_PROPERTY_EXTENSION:
        if(This->extension_offset > -1) {
            *pbstrProperty = SysAllocStringLen(This->canon_uri+This->path_start+This->extension_offset,
                                               This->path_len-This->extension_offset);
            hres = S_OK;
        } else {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            hres = S_FALSE;
        }

        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;

        break;
    case Uri_PROPERTY_FRAGMENT:
        if(This->fragment_start > -1) {
            *pbstrProperty = SysAllocStringLen(This->canon_uri+This->fragment_start, This->fragment_len);
            hres = S_OK;
        } else {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            hres = S_FALSE;
        }

        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;

        break;
    case Uri_PROPERTY_HOST:
        if(This->host_start > -1) {
            /* The '[' and ']' aren't included for IPv6 addresses. */
            if(This->host_type == Uri_HOST_IPV6)
                *pbstrProperty = SysAllocStringLen(This->canon_uri+This->host_start+1, This->host_len-2);
            else if(dwFlags == Uri_PUNYCODE_IDN_HOST && This->host_type == Uri_HOST_IDN) {
                unsigned int punycode_length;

                punycode_length = IdnToAscii(0, This->canon_uri+This->host_start, This->host_len, NULL, 0);
                *pbstrProperty = SysAllocStringLen(NULL, punycode_length);
                if (*pbstrProperty)
                    IdnToAscii(0, This->canon_uri+This->host_start, This->host_len, *pbstrProperty, punycode_length);
            }
            else
                *pbstrProperty = SysAllocStringLen(This->canon_uri+This->host_start, This->host_len);

            hres = S_OK;
        } else {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            hres = S_FALSE;
        }

        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;

        break;
    case Uri_PROPERTY_PASSWORD:
        if(This->userinfo_split > -1) {
            *pbstrProperty = SysAllocStringLen(
                This->canon_uri+This->userinfo_start+This->userinfo_split+1,
                This->userinfo_len-This->userinfo_split-1);
            hres = S_OK;
        } else {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            hres = S_FALSE;
        }

        if(!(*pbstrProperty))
            return E_OUTOFMEMORY;

        break;
    case Uri_PROPERTY_PATH:
        if(This->path_start > -1) {
            *pbstrProperty = SysAllocStringLen(This->canon_uri+This->path_start, This->path_len);
            hres = S_OK;
        } else {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            hres = S_FALSE;
        }

        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;

        break;
    case Uri_PROPERTY_PATH_AND_QUERY:
        if(This->path_start > -1) {
            *pbstrProperty = SysAllocStringLen(This->canon_uri+This->path_start, This->path_len+This->query_len);
            hres = S_OK;
        } else if(This->query_start > -1) {
            *pbstrProperty = SysAllocStringLen(This->canon_uri+This->query_start, This->query_len);
            hres = S_OK;
        } else {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            hres = S_FALSE;
        }

        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;

        break;
    case Uri_PROPERTY_QUERY:
        if(This->query_start > -1) {
            *pbstrProperty = SysAllocStringLen(This->canon_uri+This->query_start, This->query_len);
            hres = S_OK;
        } else {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            hres = S_FALSE;
        }

        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;

        break;
    case Uri_PROPERTY_RAW_URI:
        *pbstrProperty = SysAllocString(This->raw_uri);
        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;
        else
            hres = S_OK;
        break;
    case Uri_PROPERTY_SCHEME_NAME:
        if(This->scheme_start > -1) {
            *pbstrProperty = SysAllocStringLen(This->canon_uri + This->scheme_start, This->scheme_len);
            hres = S_OK;
        } else {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            hres = S_FALSE;
        }

        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;

        break;
    case Uri_PROPERTY_USER_INFO:
        if(This->userinfo_start > -1) {
            *pbstrProperty = SysAllocStringLen(This->canon_uri+This->userinfo_start, This->userinfo_len);
            hres = S_OK;
        } else {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            hres = S_FALSE;
        }

        if(!(*pbstrProperty))
            hres = E_OUTOFMEMORY;

        break;
    case Uri_PROPERTY_USER_NAME:
        if(This->userinfo_start > -1 && This->userinfo_split != 0) {
            /* If userinfo_split is set, that means a password exists
             * so the username is only from userinfo_start to userinfo_split.
             */
            if(This->userinfo_split > -1) {
                *pbstrProperty = SysAllocStringLen(This->canon_uri + This->userinfo_start, This->userinfo_split);
                hres = S_OK;
            } else {
                *pbstrProperty = SysAllocStringLen(This->canon_uri + This->userinfo_start, This->userinfo_len);
                hres = S_OK;
            }
        } else {
            *pbstrProperty = SysAllocStringLen(NULL, 0);
            hres = S_FALSE;
        }

        if(!(*pbstrProperty))
            return E_OUTOFMEMORY;

        break;
    default:
        FIXME("(%p)->(%d %p %lx)\n", This, uriProp, pbstrProperty, dwFlags);
        hres = E_NOTIMPL;
    }

    return hres;
}

static HRESULT WINAPI Uri_GetPropertyLength(IUri *iface, Uri_PROPERTY uriProp, DWORD *pcchProperty, DWORD dwFlags)
{
    Uri *This = impl_from_IUri(iface);
    HRESULT hres;
    TRACE("(%p %s)->(%d %p %lx)\n", This, debugstr_w(This->canon_uri), uriProp, pcchProperty, dwFlags);

    if(!This->create_flags)
        return E_UNEXPECTED;
    if(!pcchProperty)
        return E_INVALIDARG;

    /* Can only return a length for a property if it's a string. */
    if(uriProp > Uri_PROPERTY_STRING_LAST)
        return E_INVALIDARG;

    if(dwFlags != 0 && dwFlags != Uri_DISPLAY_NO_FRAGMENT && dwFlags != Uri_PUNYCODE_IDN_HOST
       && dwFlags != Uri_DISPLAY_IDN_HOST) {
        *pcchProperty = 0;
        return E_INVALIDARG;
    }

    switch(uriProp) {
    case Uri_PROPERTY_ABSOLUTE_URI:
        if(This->display_modifiers & URI_DISPLAY_NO_ABSOLUTE_URI) {
            *pcchProperty = 0;
            hres = S_FALSE;
        }
        /* Uri_PUNYCODE_IDN_HOST doesn't remove user info containing only "@" and ":@" */
        else if(dwFlags == Uri_PUNYCODE_IDN_HOST && This->host_type == Uri_HOST_IDN && This->host_start > -1) {
            unsigned int punycode_host_len = IdnToAscii(0, This->canon_uri+This->host_start, This->host_len, NULL, 0);
            *pcchProperty = This->canon_len - This->host_len + punycode_host_len;
            hres = S_OK;
        } else {
            if(This->scheme_type != URL_SCHEME_UNKNOWN) {
                if(This->userinfo_start > -1 && This->userinfo_len == 0)
                    /* Don't include the '@' in the length. */
                    *pcchProperty = This->canon_len-1;
                else if(This->userinfo_start > -1 && This->userinfo_len == 1 &&
                        This->userinfo_split == 0)
                    /* Don't include the ":@" in the length. */
                    *pcchProperty = This->canon_len-2;
                else
                    *pcchProperty = This->canon_len;
            } else
                *pcchProperty = This->canon_len;

            hres = S_OK;
        }

        break;
    case Uri_PROPERTY_AUTHORITY:
        if(This->port_offset > -1 &&
           This->display_modifiers & URI_DISPLAY_NO_DEFAULT_PORT_AUTH &&
           is_default_port(This->scheme_type, This->port))
            /* Only count up until the port in the authority. */
            *pcchProperty = This->port_offset;
        else
            *pcchProperty = This->authority_len;
        hres = (This->authority_start > -1) ? S_OK : S_FALSE;
        break;
    case Uri_PROPERTY_DISPLAY_URI:
        if(This->scheme_type != URL_SCHEME_UNKNOWN && This->userinfo_start > -1)
            *pcchProperty = This->canon_len-This->userinfo_len-1;
        else
            *pcchProperty = This->canon_len;

        if(dwFlags == Uri_DISPLAY_NO_FRAGMENT && This->fragment_start > -1)
            *pcchProperty -= This->fragment_len;

        hres = S_OK;
        break;
    case Uri_PROPERTY_DOMAIN:
        if(This->domain_offset > -1) {
            if(dwFlags == Uri_PUNYCODE_IDN_HOST && This->host_type == Uri_HOST_IDN)
                *pcchProperty = IdnToAscii(0, This->canon_uri+This->host_start+This->domain_offset, This->host_len-This->domain_offset, NULL, 0);
            else
                *pcchProperty = This->host_len - This->domain_offset;
        }
        else
            *pcchProperty = 0;

        hres = (This->domain_offset > -1) ? S_OK : S_FALSE;
        break;
    case Uri_PROPERTY_EXTENSION:
        if(This->extension_offset > -1) {
            *pcchProperty = This->path_len - This->extension_offset;
            hres = S_OK;
        } else {
            *pcchProperty = 0;
            hres = S_FALSE;
        }

        break;
    case Uri_PROPERTY_FRAGMENT:
        *pcchProperty = This->fragment_len;
        hres = (This->fragment_start > -1) ? S_OK : S_FALSE;
        break;
    case Uri_PROPERTY_HOST:
        *pcchProperty = This->host_len;

        /* '[' and ']' aren't included in the length. */
        if(This->host_type == Uri_HOST_IPV6)
            *pcchProperty -= 2;
        else if(dwFlags == Uri_PUNYCODE_IDN_HOST && This->host_type == Uri_HOST_IDN && This->host_start > -1)
            *pcchProperty = IdnToAscii(0, This->canon_uri+This->host_start, This->host_len, NULL, 0);

        hres = (This->host_start > -1) ? S_OK : S_FALSE;
        break;
    case Uri_PROPERTY_PASSWORD:
        *pcchProperty = (This->userinfo_split > -1) ? This->userinfo_len-This->userinfo_split-1 : 0;
        hres = (This->userinfo_split > -1) ? S_OK : S_FALSE;
        break;
    case Uri_PROPERTY_PATH:
        *pcchProperty = This->path_len;
        hres = (This->path_start > -1) ? S_OK : S_FALSE;
        break;
    case Uri_PROPERTY_PATH_AND_QUERY:
        *pcchProperty = This->path_len+This->query_len;
        hres = (This->path_start > -1 || This->query_start > -1) ? S_OK : S_FALSE;
        break;
    case Uri_PROPERTY_QUERY:
        *pcchProperty = This->query_len;
        hres = (This->query_start > -1) ? S_OK : S_FALSE;
        break;
    case Uri_PROPERTY_RAW_URI:
        *pcchProperty = SysStringLen(This->raw_uri);
        hres = S_OK;
        break;
    case Uri_PROPERTY_SCHEME_NAME:
        *pcchProperty = This->scheme_len;
        hres = (This->scheme_start > -1) ? S_OK : S_FALSE;
        break;
    case Uri_PROPERTY_USER_INFO:
        *pcchProperty = This->userinfo_len;
        hres = (This->userinfo_start > -1) ? S_OK : S_FALSE;
        break;
    case Uri_PROPERTY_USER_NAME:
        *pcchProperty = (This->userinfo_split > -1) ? This->userinfo_split : This->userinfo_len;
        if(This->userinfo_split == 0)
            hres = S_FALSE;
        else
            hres = (This->userinfo_start > -1) ? S_OK : S_FALSE;
        break;
    default:
        FIXME("(%p)->(%d %p %lx)\n", This, uriProp, pcchProperty, dwFlags);
        hres = E_NOTIMPL;
    }

    if(hres == S_OK
       && ((dwFlags == Uri_DISPLAY_NO_FRAGMENT && uriProp != Uri_PROPERTY_DISPLAY_URI)
            || (dwFlags == Uri_PUNYCODE_IDN_HOST && uriProp != Uri_PROPERTY_ABSOLUTE_URI
                && uriProp != Uri_PROPERTY_DOMAIN && uriProp != Uri_PROPERTY_HOST)
            || (dwFlags == Uri_DISPLAY_IDN_HOST && uriProp != Uri_PROPERTY_ABSOLUTE_URI
                && uriProp != Uri_PROPERTY_DOMAIN && uriProp != Uri_PROPERTY_HOST))) {
        *pcchProperty = 0;
        hres = E_INVALIDARG;
    }

    return hres;
}

static HRESULT WINAPI Uri_GetPropertyDWORD(IUri *iface, Uri_PROPERTY uriProp, DWORD *pcchProperty, DWORD dwFlags)
{
    Uri *This = impl_from_IUri(iface);
    HRESULT hres;

    TRACE("(%p %s)->(%d %p %lx)\n", This, debugstr_w(This->canon_uri), uriProp, pcchProperty, dwFlags);

    if(!This->create_flags)
        return E_UNEXPECTED;
    if(!pcchProperty)
        return E_INVALIDARG;

    /* Microsoft's implementation for the ZONE property of a URI seems to be lacking...
     * From what I can tell, instead of checking which URLZONE the URI belongs to it
     * simply assigns URLZONE_INVALID and returns E_NOTIMPL. This also applies to the GetZone
     * function.
     */
    if(uriProp == Uri_PROPERTY_ZONE) {
        *pcchProperty = URLZONE_INVALID;
        return E_NOTIMPL;
    }

    if(uriProp < Uri_PROPERTY_DWORD_START) {
        *pcchProperty = 0;
        return E_INVALIDARG;
    }

    switch(uriProp) {
    case Uri_PROPERTY_HOST_TYPE:
        *pcchProperty = This->host_type;
        hres = S_OK;
        break;
    case Uri_PROPERTY_PORT:
        if(!This->has_port) {
            *pcchProperty = 0;
            hres = S_FALSE;
        } else {
            *pcchProperty = This->port;
            hres = S_OK;
        }

        break;
    case Uri_PROPERTY_SCHEME:
        *pcchProperty = This->scheme_type;
        hres = S_OK;
        break;
    default:
        FIXME("(%p)->(%d %p %lx)\n", This, uriProp, pcchProperty, dwFlags);
        hres = E_NOTIMPL;
    }

    return hres;
}

static HRESULT WINAPI Uri_HasProperty(IUri *iface, Uri_PROPERTY uriProp, BOOL *pfHasProperty)
{
    Uri *This = impl_from_IUri(iface);

    TRACE("(%p %s)->(%d %p)\n", This, debugstr_w(This->canon_uri), uriProp, pfHasProperty);

    if(!pfHasProperty)
        return E_INVALIDARG;

    switch(uriProp) {
    case Uri_PROPERTY_ABSOLUTE_URI:
        *pfHasProperty = !(This->display_modifiers & URI_DISPLAY_NO_ABSOLUTE_URI);
        break;
    case Uri_PROPERTY_AUTHORITY:
        *pfHasProperty = This->authority_start > -1;
        break;
    case Uri_PROPERTY_DISPLAY_URI:
        *pfHasProperty = TRUE;
        break;
    case Uri_PROPERTY_DOMAIN:
        *pfHasProperty = This->domain_offset > -1;
        break;
    case Uri_PROPERTY_EXTENSION:
        *pfHasProperty = This->extension_offset > -1;
        break;
    case Uri_PROPERTY_FRAGMENT:
        *pfHasProperty = This->fragment_start > -1;
        break;
    case Uri_PROPERTY_HOST:
        *pfHasProperty = This->host_start > -1;
        break;
    case Uri_PROPERTY_PASSWORD:
        *pfHasProperty = This->userinfo_split > -1;
        break;
    case Uri_PROPERTY_PATH:
        *pfHasProperty = This->path_start > -1;
        break;
    case Uri_PROPERTY_PATH_AND_QUERY:
        *pfHasProperty = (This->path_start > -1 || This->query_start > -1);
        break;
    case Uri_PROPERTY_QUERY:
        *pfHasProperty = This->query_start > -1;
        break;
    case Uri_PROPERTY_RAW_URI:
        *pfHasProperty = TRUE;
        break;
    case Uri_PROPERTY_SCHEME_NAME:
        *pfHasProperty = This->scheme_start > -1;
        break;
    case Uri_PROPERTY_USER_INFO:
        *pfHasProperty = This->userinfo_start > -1;
        break;
    case Uri_PROPERTY_USER_NAME:
        if(This->userinfo_split == 0)
            *pfHasProperty = FALSE;
        else
            *pfHasProperty = This->userinfo_start > -1;
        break;
    case Uri_PROPERTY_HOST_TYPE:
        *pfHasProperty = TRUE;
        break;
    case Uri_PROPERTY_PORT:
        *pfHasProperty = This->has_port;
        break;
    case Uri_PROPERTY_SCHEME:
        *pfHasProperty = TRUE;
        break;
    case Uri_PROPERTY_ZONE:
        *pfHasProperty = FALSE;
        break;
    default:
        FIXME("(%p)->(%d %p): Unsupported property type.\n", This, uriProp, pfHasProperty);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI Uri_GetAbsoluteUri(IUri *iface, BSTR *pstrAbsoluteUri)
{
    TRACE("(%p)->(%p)\n", iface, pstrAbsoluteUri);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_ABSOLUTE_URI, pstrAbsoluteUri, 0);
}

static HRESULT WINAPI Uri_GetAuthority(IUri *iface, BSTR *pstrAuthority)
{
    TRACE("(%p)->(%p)\n", iface, pstrAuthority);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_AUTHORITY, pstrAuthority, 0);
}

static HRESULT WINAPI Uri_GetDisplayUri(IUri *iface, BSTR *pstrDisplayUri)
{
    TRACE("(%p)->(%p)\n", iface, pstrDisplayUri);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_DISPLAY_URI, pstrDisplayUri, 0);
}

static HRESULT WINAPI Uri_GetDomain(IUri *iface, BSTR *pstrDomain)
{
    TRACE("(%p)->(%p)\n", iface, pstrDomain);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_DOMAIN, pstrDomain, 0);
}

static HRESULT WINAPI Uri_GetExtension(IUri *iface, BSTR *pstrExtension)
{
    TRACE("(%p)->(%p)\n", iface, pstrExtension);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_EXTENSION, pstrExtension, 0);
}

static HRESULT WINAPI Uri_GetFragment(IUri *iface, BSTR *pstrFragment)
{
    TRACE("(%p)->(%p)\n", iface, pstrFragment);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_FRAGMENT, pstrFragment, 0);
}

static HRESULT WINAPI Uri_GetHost(IUri *iface, BSTR *pstrHost)
{
    TRACE("(%p)->(%p)\n", iface, pstrHost);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_HOST, pstrHost, 0);
}

static HRESULT WINAPI Uri_GetPassword(IUri *iface, BSTR *pstrPassword)
{
    TRACE("(%p)->(%p)\n", iface, pstrPassword);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_PASSWORD, pstrPassword, 0);
}

static HRESULT WINAPI Uri_GetPath(IUri *iface, BSTR *pstrPath)
{
    TRACE("(%p)->(%p)\n", iface, pstrPath);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_PATH, pstrPath, 0);
}

static HRESULT WINAPI Uri_GetPathAndQuery(IUri *iface, BSTR *pstrPathAndQuery)
{
    TRACE("(%p)->(%p)\n", iface, pstrPathAndQuery);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_PATH_AND_QUERY, pstrPathAndQuery, 0);
}

static HRESULT WINAPI Uri_GetQuery(IUri *iface, BSTR *pstrQuery)
{
    TRACE("(%p)->(%p)\n", iface, pstrQuery);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_QUERY, pstrQuery, 0);
}

static HRESULT WINAPI Uri_GetRawUri(IUri *iface, BSTR *pstrRawUri)
{
    TRACE("(%p)->(%p)\n", iface, pstrRawUri);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_RAW_URI, pstrRawUri, 0);
}

static HRESULT WINAPI Uri_GetSchemeName(IUri *iface, BSTR *pstrSchemeName)
{
    TRACE("(%p)->(%p)\n", iface, pstrSchemeName);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_SCHEME_NAME, pstrSchemeName, 0);
}

static HRESULT WINAPI Uri_GetUserInfo(IUri *iface, BSTR *pstrUserInfo)
{
    TRACE("(%p)->(%p)\n", iface, pstrUserInfo);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_USER_INFO, pstrUserInfo, 0);
}

static HRESULT WINAPI Uri_GetUserName(IUri *iface, BSTR *pstrUserName)
{
    TRACE("(%p)->(%p)\n", iface, pstrUserName);
    return IUri_GetPropertyBSTR(iface, Uri_PROPERTY_USER_NAME, pstrUserName, 0);
}

static HRESULT WINAPI Uri_GetHostType(IUri *iface, DWORD *pdwHostType)
{
    TRACE("(%p)->(%p)\n", iface, pdwHostType);
    return IUri_GetPropertyDWORD(iface, Uri_PROPERTY_HOST_TYPE, pdwHostType, 0);
}

static HRESULT WINAPI Uri_GetPort(IUri *iface, DWORD *pdwPort)
{
    TRACE("(%p)->(%p)\n", iface, pdwPort);
    return IUri_GetPropertyDWORD(iface, Uri_PROPERTY_PORT, pdwPort, 0);
}

static HRESULT WINAPI Uri_GetScheme(IUri *iface, DWORD *pdwScheme)
{
    TRACE("(%p)->(%p)\n", iface, pdwScheme);
    return IUri_GetPropertyDWORD(iface, Uri_PROPERTY_SCHEME, pdwScheme, 0);
}

static HRESULT WINAPI Uri_GetZone(IUri *iface, DWORD *pdwZone)
{
    TRACE("(%p)->(%p)\n", iface, pdwZone);
    return IUri_GetPropertyDWORD(iface, Uri_PROPERTY_ZONE,pdwZone, 0);
}

static HRESULT WINAPI Uri_GetProperties(IUri *iface, DWORD *pdwProperties)
{
    Uri *This = impl_from_IUri(iface);
    TRACE("(%p %s)->(%p)\n", This, debugstr_w(This->canon_uri), pdwProperties);

    if(!This->create_flags)
        return E_UNEXPECTED;
    if(!pdwProperties)
        return E_INVALIDARG;

    /* All URIs have these. */
    *pdwProperties = Uri_HAS_DISPLAY_URI|Uri_HAS_RAW_URI|Uri_HAS_SCHEME|Uri_HAS_HOST_TYPE;

    if(!(This->display_modifiers & URI_DISPLAY_NO_ABSOLUTE_URI))
        *pdwProperties |= Uri_HAS_ABSOLUTE_URI;

    if(This->scheme_start > -1)
        *pdwProperties |= Uri_HAS_SCHEME_NAME;

    if(This->authority_start > -1) {
        *pdwProperties |= Uri_HAS_AUTHORITY;
        if(This->userinfo_start > -1) {
            *pdwProperties |= Uri_HAS_USER_INFO;
            if(This->userinfo_split != 0)
                *pdwProperties |= Uri_HAS_USER_NAME;
        }
        if(This->userinfo_split > -1)
            *pdwProperties |= Uri_HAS_PASSWORD;
        if(This->host_start > -1)
            *pdwProperties |= Uri_HAS_HOST;
        if(This->domain_offset > -1)
            *pdwProperties |= Uri_HAS_DOMAIN;
    }

    if(This->has_port)
        *pdwProperties |= Uri_HAS_PORT;
    if(This->path_start > -1)
        *pdwProperties |= Uri_HAS_PATH|Uri_HAS_PATH_AND_QUERY;
    if(This->query_start > -1)
        *pdwProperties |= Uri_HAS_QUERY|Uri_HAS_PATH_AND_QUERY;

    if(This->extension_offset > -1)
        *pdwProperties |= Uri_HAS_EXTENSION;

    if(This->fragment_start > -1)
        *pdwProperties |= Uri_HAS_FRAGMENT;

    return S_OK;
}

static HRESULT WINAPI Uri_IsEqual(IUri *iface, IUri *pUri, BOOL *pfEqual)
{
    Uri *This = impl_from_IUri(iface);
    Uri *other;

    TRACE("(%p %s)->(%p %p)\n", This, debugstr_w(This->canon_uri), pUri, pfEqual);

    if(!This->create_flags)
        return E_UNEXPECTED;
    if(!pfEqual)
        return E_POINTER;

    if(!pUri) {
        *pfEqual = FALSE;

        /* For some reason Windows returns S_OK here... */
        return S_OK;
    }

    /* Try to convert it to a Uri (allows for a more simple comparison). */
    if(!(other = get_uri_obj(pUri))) {
        FIXME("(%p)->(%p %p) No support for unknown IUri's yet.\n", iface, pUri, pfEqual);
        return E_NOTIMPL;
    }

    TRACE("comparing to %s\n", debugstr_w(other->canon_uri));
    return compare_uris(This, other, pfEqual);
}

static const IUriVtbl UriVtbl = {
    Uri_QueryInterface,
    Uri_AddRef,
    Uri_Release,
    Uri_GetPropertyBSTR,
    Uri_GetPropertyLength,
    Uri_GetPropertyDWORD,
    Uri_HasProperty,
    Uri_GetAbsoluteUri,
    Uri_GetAuthority,
    Uri_GetDisplayUri,
    Uri_GetDomain,
    Uri_GetExtension,
    Uri_GetFragment,
    Uri_GetHost,
    Uri_GetPassword,
    Uri_GetPath,
    Uri_GetPathAndQuery,
    Uri_GetQuery,
    Uri_GetRawUri,
    Uri_GetSchemeName,
    Uri_GetUserInfo,
    Uri_GetUserName,
    Uri_GetHostType,
    Uri_GetPort,
    Uri_GetScheme,
    Uri_GetZone,
    Uri_GetProperties,
    Uri_IsEqual
};

static inline Uri* impl_from_IUriBuilderFactory(IUriBuilderFactory *iface)
{
    return CONTAINING_RECORD(iface, Uri, IUriBuilderFactory_iface);
}

static HRESULT WINAPI UriBuilderFactory_QueryInterface(IUriBuilderFactory *iface, REFIID riid, void **ppv)
{
    Uri *This = impl_from_IUriBuilderFactory(iface);
    return IUri_QueryInterface(&This->IUri_iface, riid, ppv);
}

static ULONG WINAPI UriBuilderFactory_AddRef(IUriBuilderFactory *iface)
{
    Uri *This = impl_from_IUriBuilderFactory(iface);
    return IUri_AddRef(&This->IUri_iface);
}

static ULONG WINAPI UriBuilderFactory_Release(IUriBuilderFactory *iface)
{
    Uri *This = impl_from_IUriBuilderFactory(iface);
    return IUri_Release(&This->IUri_iface);
}

static HRESULT WINAPI UriBuilderFactory_CreateIUriBuilder(IUriBuilderFactory *iface,
                                                          DWORD dwFlags,
                                                          DWORD_PTR dwReserved,
                                                          IUriBuilder **ppIUriBuilder)
{
    Uri *This = impl_from_IUriBuilderFactory(iface);
    TRACE("(%p)->(%08lx %08Ix %p)\n", This, dwFlags, dwReserved, ppIUriBuilder);

    if(!ppIUriBuilder)
        return E_POINTER;

    if(dwFlags || dwReserved) {
        *ppIUriBuilder = NULL;
        return E_INVALIDARG;
    }

    return CreateIUriBuilder(NULL, 0, 0, ppIUriBuilder);
}

static HRESULT WINAPI UriBuilderFactory_CreateInitializedIUriBuilder(IUriBuilderFactory *iface,
                                                                     DWORD dwFlags,
                                                                     DWORD_PTR dwReserved,
                                                                     IUriBuilder **ppIUriBuilder)
{
    Uri *This = impl_from_IUriBuilderFactory(iface);
    TRACE("(%p)->(%08lx %08Ix %p)\n", This, dwFlags, dwReserved, ppIUriBuilder);

    if(!ppIUriBuilder)
        return E_POINTER;

    if(dwFlags || dwReserved) {
        *ppIUriBuilder = NULL;
        return E_INVALIDARG;
    }

    return CreateIUriBuilder(&This->IUri_iface, 0, 0, ppIUriBuilder);
}

static const IUriBuilderFactoryVtbl UriBuilderFactoryVtbl = {
    UriBuilderFactory_QueryInterface,
    UriBuilderFactory_AddRef,
    UriBuilderFactory_Release,
    UriBuilderFactory_CreateIUriBuilder,
    UriBuilderFactory_CreateInitializedIUriBuilder
};

static inline Uri* impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, Uri, IPersistStream_iface);
}

static HRESULT WINAPI PersistStream_QueryInterface(IPersistStream *iface, REFIID riid, void **ppvObject)
{
    Uri *This = impl_from_IPersistStream(iface);
    return IUri_QueryInterface(&This->IUri_iface, riid, ppvObject);
}

static ULONG WINAPI PersistStream_AddRef(IPersistStream *iface)
{
    Uri *This = impl_from_IPersistStream(iface);
    return IUri_AddRef(&This->IUri_iface);
}

static ULONG WINAPI PersistStream_Release(IPersistStream *iface)
{
    Uri *This = impl_from_IPersistStream(iface);
    return IUri_Release(&This->IUri_iface);
}

static HRESULT WINAPI PersistStream_GetClassID(IPersistStream *iface, CLSID *pClassID)
{
    Uri *This = impl_from_IPersistStream(iface);
    TRACE("(%p)->(%p)\n", This, pClassID);

    if(!pClassID)
        return E_INVALIDARG;

    *pClassID = CLSID_CUri;
    return S_OK;
}

static HRESULT WINAPI PersistStream_IsDirty(IPersistStream *iface)
{
    Uri *This = impl_from_IPersistStream(iface);
    TRACE("(%p)\n", This);
    return S_FALSE;
}

struct persist_uri {
    DWORD size;
    DWORD unk1[2];
    DWORD create_flags;
    DWORD unk2[3];
    DWORD fields_no;
    BYTE data[1];
};

static HRESULT WINAPI PersistStream_Load(IPersistStream *iface, IStream *pStm)
{
    Uri *This = impl_from_IPersistStream(iface);
    struct persist_uri *data;
    parse_data parse;
    DWORD size;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, pStm);

    if(This->create_flags)
        return E_UNEXPECTED;
    if(!pStm)
        return E_INVALIDARG;

    hr = IStream_Read(pStm, &size, sizeof(DWORD), NULL);
    if(FAILED(hr))
        return hr;
    data = malloc(size);
    if(!data)
        return E_OUTOFMEMORY;
    hr = IStream_Read(pStm, data->unk1, size-sizeof(DWORD)-2, NULL);
    if(FAILED(hr)) {
        free(data);
        return hr;
    }

    if(size < sizeof(struct persist_uri)) {
        free(data);
        return S_OK;
    }

    if(*(DWORD*)data->data != Uri_PROPERTY_RAW_URI) {
        free(data);
        ERR("Can't find raw_uri\n");
        return E_UNEXPECTED;
    }

    This->raw_uri = SysAllocString((WCHAR*)(data->data+sizeof(DWORD)*2));
    if(!This->raw_uri) {
        free(data);
        return E_OUTOFMEMORY;
    }
    This->create_flags = data->create_flags;
    free(data);
    TRACE("%lx %s\n", This->create_flags, debugstr_w(This->raw_uri));

    memset(&parse, 0, sizeof(parse_data));
    parse.uri = This->raw_uri;
    if(!parse_uri(&parse, This->create_flags)) {
        SysFreeString(This->raw_uri);
        This->create_flags = 0;
        return E_UNEXPECTED;
    }

    hr = canonicalize_uri(&parse, This, This->create_flags);
    if(FAILED(hr)) {
        SysFreeString(This->raw_uri);
        This->create_flags = 0;
        return hr;
    }

    return S_OK;
}

static inline BYTE* persist_stream_add_strprop(Uri *This, BYTE *p, DWORD type, DWORD len, WCHAR *data)
{
    len *= sizeof(WCHAR);
    *(DWORD*)p = type;
    p += sizeof(DWORD);
    *(DWORD*)p = len+sizeof(WCHAR);
    p += sizeof(DWORD);
    memcpy(p, data, len);
    p += len;
    *(WCHAR*)p = 0;
    return p+sizeof(WCHAR);
}

static inline void persist_stream_save(Uri *This, IStream *pStm, BOOL marshal, struct persist_uri *data)
{
    BYTE *p = NULL;

    data->create_flags = This->create_flags;

    if(This->create_flags) {
        data->fields_no = 1;
        p = persist_stream_add_strprop(This, data->data, Uri_PROPERTY_RAW_URI,
                SysStringLen(This->raw_uri), This->raw_uri);
    }
    if(This->scheme_type!=URL_SCHEME_HTTP && This->scheme_type!=URL_SCHEME_HTTPS
            && This->scheme_type!=URL_SCHEME_FTP)
        return;

    if(This->fragment_len) {
        data->fields_no++;
        p = persist_stream_add_strprop(This, p, Uri_PROPERTY_FRAGMENT,
                This->fragment_len, This->canon_uri+This->fragment_start);
    }

    if(This->host_len) {
        data->fields_no++;
        if(This->host_type == Uri_HOST_IPV6)
            p = persist_stream_add_strprop(This, p, Uri_PROPERTY_HOST,
                    This->host_len-2, This->canon_uri+This->host_start+1);
        else
            p = persist_stream_add_strprop(This, p, Uri_PROPERTY_HOST,
                    This->host_len, This->canon_uri+This->host_start);
    }

    if(This->userinfo_split > -1) {
        data->fields_no++;
        p = persist_stream_add_strprop(This, p, Uri_PROPERTY_PASSWORD,
                This->userinfo_len-This->userinfo_split-1,
                This->canon_uri+This->userinfo_start+This->userinfo_split+1);
    }

    if(This->path_len) {
        data->fields_no++;
        p = persist_stream_add_strprop(This, p, Uri_PROPERTY_PATH,
                This->path_len, This->canon_uri+This->path_start);
    } else if(marshal) {
        WCHAR no_path = '/';
        data->fields_no++;
        p = persist_stream_add_strprop(This, p, Uri_PROPERTY_PATH, 1, &no_path);
    }

    if(This->has_port) {
        data->fields_no++;
        *(DWORD*)p = Uri_PROPERTY_PORT;
        p += sizeof(DWORD);
        *(DWORD*)p = sizeof(DWORD);
        p += sizeof(DWORD);
        *(DWORD*)p = This->port;
        p += sizeof(DWORD);
    }

    if(This->query_len) {
        data->fields_no++;
        p = persist_stream_add_strprop(This, p, Uri_PROPERTY_QUERY,
                This->query_len, This->canon_uri+This->query_start);
    }

    if(This->scheme_len) {
        data->fields_no++;
        p = persist_stream_add_strprop(This, p, Uri_PROPERTY_SCHEME_NAME,
                This->scheme_len, This->canon_uri+This->scheme_start);
    }

    if(This->userinfo_start>-1 && This->userinfo_split!=0) {
        data->fields_no++;
        if(This->userinfo_split > -1)
            p = persist_stream_add_strprop(This, p, Uri_PROPERTY_USER_NAME,
                    This->userinfo_split, This->canon_uri+This->userinfo_start);
        else
            p = persist_stream_add_strprop(This, p, Uri_PROPERTY_USER_NAME,
                    This->userinfo_len, This->canon_uri+This->userinfo_start);
    }
}

static HRESULT WINAPI PersistStream_Save(IPersistStream *iface, IStream *pStm, BOOL fClearDirty)
{
    Uri *This = impl_from_IPersistStream(iface);
    struct persist_uri *data;
    ULARGE_INTEGER size;
    HRESULT hres;

    TRACE("(%p)->(%p %x)\n", This, pStm, fClearDirty);

    if(!pStm)
        return E_INVALIDARG;

    hres = IPersistStream_GetSizeMax(&This->IPersistStream_iface, &size);
    if(FAILED(hres))
        return hres;

    data = calloc(1, size.u.LowPart);
    if(!data)
        return E_OUTOFMEMORY;
    data->size = size.u.LowPart;
    persist_stream_save(This, pStm, FALSE, data);

    hres = IStream_Write(pStm, data, data->size-2, NULL);
    free(data);
    return hres;
}

static HRESULT WINAPI PersistStream_GetSizeMax(IPersistStream *iface, ULARGE_INTEGER *pcbSize)
{
    Uri *This = impl_from_IPersistStream(iface);
    TRACE("(%p)->(%p)\n", This, pcbSize);

    if(!pcbSize)
        return E_INVALIDARG;

    pcbSize->u.LowPart = 2+sizeof(struct persist_uri);
    pcbSize->u.HighPart = 0;
    if(This->create_flags)
        pcbSize->u.LowPart += (SysStringLen(This->raw_uri)+1)*sizeof(WCHAR) + 2*sizeof(DWORD);
    else /* there's no place for fields no */
        pcbSize->u.LowPart -= sizeof(DWORD);
    if(This->scheme_type!=URL_SCHEME_HTTP && This->scheme_type!=URL_SCHEME_HTTPS
            && This->scheme_type!=URL_SCHEME_FTP)
        return S_OK;

    if(This->fragment_len)
        pcbSize->u.LowPart += (This->fragment_len+1)*sizeof(WCHAR) + 2*sizeof(DWORD);
    if(This->host_len) {
        if(This->host_type == Uri_HOST_IPV6)
            pcbSize->u.LowPart += (This->host_len-1)*sizeof(WCHAR) + 2*sizeof(DWORD);
        else
            pcbSize->u.LowPart += (This->host_len+1)*sizeof(WCHAR) + 2*sizeof(DWORD);
    }
    if(This->userinfo_split > -1)
        pcbSize->u.LowPart += (This->userinfo_len-This->userinfo_split)*sizeof(WCHAR) + 2*sizeof(DWORD);
    if(This->path_len)
        pcbSize->u.LowPart += (This->path_len+1)*sizeof(WCHAR) + 2*sizeof(DWORD);
    if(This->has_port)
        pcbSize->u.LowPart += 3*sizeof(DWORD);
    if(This->query_len)
        pcbSize->u.LowPart += (This->query_len+1)*sizeof(WCHAR) + 2*sizeof(DWORD);
    if(This->scheme_len)
        pcbSize->u.LowPart += (This->scheme_len+1)*sizeof(WCHAR) + 2*sizeof(DWORD);
    if(This->userinfo_start>-1 && This->userinfo_split!=0) {
        if(This->userinfo_split > -1)
            pcbSize->u.LowPart += (This->userinfo_split+1)*sizeof(WCHAR) + 2*sizeof(DWORD);
        else
            pcbSize->u.LowPart += (This->userinfo_len+1)*sizeof(WCHAR) + 2*sizeof(DWORD);
    }
    return S_OK;
}

static const IPersistStreamVtbl PersistStreamVtbl = {
    PersistStream_QueryInterface,
    PersistStream_AddRef,
    PersistStream_Release,
    PersistStream_GetClassID,
    PersistStream_IsDirty,
    PersistStream_Load,
    PersistStream_Save,
    PersistStream_GetSizeMax
};

static inline Uri* impl_from_IMarshal(IMarshal *iface)
{
    return CONTAINING_RECORD(iface, Uri, IMarshal_iface);
}

static HRESULT WINAPI Marshal_QueryInterface(IMarshal *iface, REFIID riid, void **ppvObject)
{
    Uri *This = impl_from_IMarshal(iface);
    return IUri_QueryInterface(&This->IUri_iface, riid, ppvObject);
}

static ULONG WINAPI Marshal_AddRef(IMarshal *iface)
{
    Uri *This = impl_from_IMarshal(iface);
    return IUri_AddRef(&This->IUri_iface);
}

static ULONG WINAPI Marshal_Release(IMarshal *iface)
{
    Uri *This = impl_from_IMarshal(iface);
    return IUri_Release(&This->IUri_iface);
}

static HRESULT WINAPI Marshal_GetUnmarshalClass(IMarshal *iface, REFIID riid, void *pv,
        DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, CLSID *pCid)
{
    Uri *This = impl_from_IMarshal(iface);
    TRACE("(%p)->(%s %p %lx %p %lx %p)\n", This, debugstr_guid(riid), pv,
            dwDestContext, pvDestContext, mshlflags, pCid);

    if(!pCid || (dwDestContext!=MSHCTX_LOCAL && dwDestContext!=MSHCTX_NOSHAREDMEM
                && dwDestContext!=MSHCTX_INPROC))
        return E_INVALIDARG;

    *pCid = CLSID_CUri;
    return S_OK;
}

struct inproc_marshal_uri {
    DWORD size;
    DWORD mshlflags;
    DWORD unk[4]; /* process identifier? */
    Uri *uri;
};

static HRESULT WINAPI Marshal_GetMarshalSizeMax(IMarshal *iface, REFIID riid, void *pv,
        DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, DWORD *pSize)
{
    Uri *This = impl_from_IMarshal(iface);
    ULARGE_INTEGER size;
    HRESULT hres;
    TRACE("(%p)->(%s %p %lx %p %lx %p)\n", This, debugstr_guid(riid), pv,
            dwDestContext, pvDestContext, mshlflags, pSize);

    if(!pSize || (dwDestContext!=MSHCTX_LOCAL && dwDestContext!=MSHCTX_NOSHAREDMEM
                && dwDestContext!=MSHCTX_INPROC))
        return E_INVALIDARG;

    if(dwDestContext == MSHCTX_INPROC) {
        *pSize = sizeof(struct inproc_marshal_uri);
        return S_OK;
    }

    hres = IPersistStream_GetSizeMax(&This->IPersistStream_iface, &size);
    if(FAILED(hres))
        return hres;
    if(!This->path_len && (This->scheme_type==URL_SCHEME_HTTP
                || This->scheme_type==URL_SCHEME_HTTPS
                || This->scheme_type==URL_SCHEME_FTP))
        size.u.LowPart += 3*sizeof(DWORD);
    *pSize = size.u.LowPart+2*sizeof(DWORD);
    return S_OK;
}

static HRESULT WINAPI Marshal_MarshalInterface(IMarshal *iface, IStream *pStm, REFIID riid,
        void *pv, DWORD dwDestContext, void *pvDestContext, DWORD mshlflags)
{
    Uri *This = impl_from_IMarshal(iface);
    DWORD *data;
    DWORD size;
    HRESULT hres;

    TRACE("(%p)->(%p %s %p %lx %p %lx)\n", This, pStm, debugstr_guid(riid), pv,
            dwDestContext, pvDestContext, mshlflags);

    if(!pStm || mshlflags!=MSHLFLAGS_NORMAL || (dwDestContext!=MSHCTX_LOCAL
                && dwDestContext!=MSHCTX_NOSHAREDMEM && dwDestContext!=MSHCTX_INPROC))
        return E_INVALIDARG;

    if(dwDestContext == MSHCTX_INPROC) {
        struct inproc_marshal_uri data;

        data.size = sizeof(data);
        data.mshlflags = MSHCTX_INPROC;
        data.unk[0] = 0;
        data.unk[1] = 0;
        data.unk[2] = 0;
        data.unk[3] = 0;
        data.uri = This;

        hres = IStream_Write(pStm, &data, data.size, NULL);
        if(FAILED(hres))
            return hres;

        IUri_AddRef(&This->IUri_iface);
        return S_OK;
    }

    hres = IMarshal_GetMarshalSizeMax(iface, riid, pv, dwDestContext,
            pvDestContext, mshlflags, &size);
    if(FAILED(hres))
        return hres;

    data = calloc(1, size);
    if(!data)
        return E_OUTOFMEMORY;

    data[0] = size;
    data[1] = dwDestContext;
    data[2] = size-2*sizeof(DWORD);
    persist_stream_save(This, pStm, TRUE, (struct persist_uri*)(data+2));

    hres = IStream_Write(pStm, data, data[0]-2, NULL);
    free(data);
    return hres;
}

static HRESULT WINAPI Marshal_UnmarshalInterface(IMarshal *iface,
        IStream *pStm, REFIID riid, void **ppv)
{
    Uri *This = impl_from_IMarshal(iface);
    DWORD header[2];
    HRESULT hres;

    TRACE("(%p)->(%p %s %p)\n", This, pStm, debugstr_guid(riid), ppv);

    if(This->create_flags)
        return E_UNEXPECTED;
    if(!pStm || !riid || !ppv)
        return E_INVALIDARG;

    hres = IStream_Read(pStm, header, sizeof(header), NULL);
    if(FAILED(hres))
        return hres;

    if(header[1]!=MSHCTX_LOCAL && header[1]!=MSHCTX_NOSHAREDMEM
            && header[1]!=MSHCTX_INPROC)
        return E_UNEXPECTED;

    if(header[1] == MSHCTX_INPROC) {
        struct inproc_marshal_uri data;
        parse_data parse;

        hres = IStream_Read(pStm, data.unk, sizeof(data)-2*sizeof(DWORD), NULL);
        if(FAILED(hres))
            return hres;

        This->raw_uri = SysAllocString(data.uri->raw_uri);
        if(!This->raw_uri) {
            return E_OUTOFMEMORY;
        }

        memset(&parse, 0, sizeof(parse_data));
        parse.uri = This->raw_uri;

        if(!parse_uri(&parse, data.uri->create_flags))
            return E_INVALIDARG;

        hres = canonicalize_uri(&parse, This, data.uri->create_flags);
        if(FAILED(hres))
            return hres;

        This->create_flags = data.uri->create_flags;
        IUri_Release(&data.uri->IUri_iface);

        return IUri_QueryInterface(&This->IUri_iface, riid, ppv);
    }

    hres = IPersistStream_Load(&This->IPersistStream_iface, pStm);
    if(FAILED(hres))
        return hres;

    return IUri_QueryInterface(&This->IUri_iface, riid, ppv);
}

static HRESULT WINAPI Marshal_ReleaseMarshalData(IMarshal *iface, IStream *pStm)
{
    Uri *This = impl_from_IMarshal(iface);
    LARGE_INTEGER off;
    DWORD header[2];
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pStm);

    if(!pStm)
        return E_INVALIDARG;

    hres = IStream_Read(pStm, header, 2*sizeof(DWORD), NULL);
    if(FAILED(hres))
        return hres;

    if(header[1] == MSHCTX_INPROC) {
        struct inproc_marshal_uri data;

        hres = IStream_Read(pStm, data.unk, sizeof(data)-2*sizeof(DWORD), NULL);
        if(FAILED(hres))
            return hres;

        IUri_Release(&data.uri->IUri_iface);
        return S_OK;
    }

    off.u.LowPart = header[0]-sizeof(header)-2;
    off.u.HighPart = 0;
    return IStream_Seek(pStm, off, STREAM_SEEK_CUR, NULL);
}

static HRESULT WINAPI Marshal_DisconnectObject(IMarshal *iface, DWORD dwReserved)
{
    Uri *This = impl_from_IMarshal(iface);
    TRACE("(%p)->(%lx)\n", This, dwReserved);
    return S_OK;
}

static const IMarshalVtbl MarshalVtbl = {
    Marshal_QueryInterface,
    Marshal_AddRef,
    Marshal_Release,
    Marshal_GetUnmarshalClass,
    Marshal_GetMarshalSizeMax,
    Marshal_MarshalInterface,
    Marshal_UnmarshalInterface,
    Marshal_ReleaseMarshalData,
    Marshal_DisconnectObject
};

HRESULT Uri_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    Uri *ret = calloc(1, sizeof(Uri));

    TRACE("(%p %p)\n", pUnkOuter, ppobj);

    *ppobj = ret;
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IUri_iface.lpVtbl = &UriVtbl;
    ret->IUriBuilderFactory_iface.lpVtbl = &UriBuilderFactoryVtbl;
    ret->IPersistStream_iface.lpVtbl = &PersistStreamVtbl;
    ret->IMarshal_iface.lpVtbl = &MarshalVtbl;
    ret->ref = 1;

    *ppobj = &ret->IUri_iface;
    return S_OK;
}

/***********************************************************************
 *           CreateUri (urlmon.@)
 *
 * Creates a new IUri object using the URI represented by pwzURI. This function
 * parses and validates the components of pwzURI and then canonicalizes the
 * parsed components.
 *
 * PARAMS
 *  pwzURI      [I] The URI to parse, validate, and canonicalize.
 *  dwFlags     [I] Flags which can affect how the parsing/canonicalization is performed.
 *  dwReserved  [I] Reserved (not used).
 *  ppURI       [O] The resulting IUri after parsing/canonicalization occurs.
 *
 * RETURNS
 *  Success: Returns S_OK. ppURI contains the pointer to the newly allocated IUri.
 *  Failure: E_INVALIDARG if there are invalid flag combinations in dwFlags, or an
 *           invalid parameter, or pwzURI doesn't represent a valid URI.
 *           E_OUTOFMEMORY if any memory allocation fails.
 *
 * NOTES
 *  Default flags:
 *      Uri_CREATE_CANONICALIZE, Uri_CREATE_DECODE_EXTRA_INFO, Uri_CREATE_CRACK_UNKNOWN_SCHEMES,
 *      Uri_CREATE_PRE_PROCESS_HTML_URI, Uri_CREATE_NO_IE_SETTINGS.
 */
HRESULT WINAPI CreateUri(LPCWSTR pwzURI, DWORD dwFlags, DWORD_PTR dwReserved, IUri **ppURI)
{
    const DWORD supported_flags = Uri_CREATE_ALLOW_RELATIVE|Uri_CREATE_ALLOW_IMPLICIT_WILDCARD_SCHEME|
        Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME|Uri_CREATE_NO_CANONICALIZE|Uri_CREATE_CANONICALIZE|
        Uri_CREATE_DECODE_EXTRA_INFO|Uri_CREATE_NO_DECODE_EXTRA_INFO|Uri_CREATE_CRACK_UNKNOWN_SCHEMES|
        Uri_CREATE_NO_CRACK_UNKNOWN_SCHEMES|Uri_CREATE_PRE_PROCESS_HTML_URI|Uri_CREATE_NO_PRE_PROCESS_HTML_URI|
        Uri_CREATE_NO_IE_SETTINGS|Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS|Uri_CREATE_FILE_USE_DOS_PATH;
    Uri *ret;
    HRESULT hr;
    parse_data data;

    TRACE("(%s %lx %Ix %p)\n", debugstr_w(pwzURI), dwFlags, dwReserved, ppURI);

    if(!ppURI)
        return E_INVALIDARG;

    if(!pwzURI) {
        *ppURI = NULL;
        return E_INVALIDARG;
    }

    /* Check for invalid flags. */
    if(has_invalid_flag_combination(dwFlags)) {
        *ppURI = NULL;
        return E_INVALIDARG;
    }

    /* Currently unsupported. */
    if(dwFlags & ~supported_flags)
        FIXME("Ignoring unsupported flag(s) %lx\n", dwFlags & ~supported_flags);

    hr = Uri_Construct(NULL, (void**)&ret);
    if(FAILED(hr)) {
        *ppURI = NULL;
        return hr;
    }

    /* Explicitly set the default flags if it doesn't cause a flag conflict. */
    apply_default_flags(&dwFlags);

    /* Pre process the URI, unless told otherwise. */
    if(!(dwFlags & Uri_CREATE_NO_PRE_PROCESS_HTML_URI))
        ret->raw_uri = pre_process_uri(pwzURI);
    else
        ret->raw_uri = SysAllocString(pwzURI);

    if(!ret->raw_uri) {
        free(ret);
        return E_OUTOFMEMORY;
    }

    memset(&data, 0, sizeof(parse_data));
    data.uri = ret->raw_uri;

    /* Validate and parse the URI into its components. */
    if(!parse_uri(&data, dwFlags)) {
        /* Encountered an unsupported or invalid URI */
        IUri_Release(&ret->IUri_iface);
        *ppURI = NULL;
        return E_INVALIDARG;
    }

    /* Canonicalize the URI. */
    hr = canonicalize_uri(&data, ret, dwFlags);
    if(FAILED(hr)) {
        IUri_Release(&ret->IUri_iface);
        *ppURI = NULL;
        return hr;
    }

    ret->create_flags = dwFlags;

    *ppURI = &ret->IUri_iface;
    return S_OK;
}

/***********************************************************************
 *           CreateUriWithFragment (urlmon.@)
 *
 * Creates a new IUri object. This is almost the same as CreateUri, expect that
 * it allows you to explicitly specify a fragment (pwzFragment) for pwzURI.
 *
 * PARAMS
 *  pwzURI      [I] The URI to parse and perform canonicalization on.
 *  pwzFragment [I] The explicit fragment string which should be added to pwzURI.
 *  dwFlags     [I] The flags which will be passed to CreateUri.
 *  dwReserved  [I] Reserved (not used).
 *  ppURI       [O] The resulting IUri after parsing/canonicalization.
 *
 * RETURNS
 *  Success: S_OK. ppURI contains the pointer to the newly allocated IUri.
 *  Failure: E_INVALIDARG if pwzURI already contains a fragment and pwzFragment
 *           isn't NULL. Will also return E_INVALIDARG for the same reasons as
 *           CreateUri will. E_OUTOFMEMORY if any allocation fails.
 */
HRESULT WINAPI CreateUriWithFragment(LPCWSTR pwzURI, LPCWSTR pwzFragment, DWORD dwFlags,
                                     DWORD_PTR dwReserved, IUri **ppURI)
{
    HRESULT hres;
    TRACE("(%s %s %lx %Ix %p)\n", debugstr_w(pwzURI), debugstr_w(pwzFragment), dwFlags, dwReserved, ppURI);

    if(!ppURI)
        return E_INVALIDARG;

    if(!pwzURI) {
        *ppURI = NULL;
        return E_INVALIDARG;
    }

    /* Check if a fragment should be appended to the URI string. */
    if(pwzFragment) {
        WCHAR *uriW;
        DWORD uri_len, frag_len;
        BOOL add_pound;

        /* Check if the original URI already has a fragment component. */
        if(StrChrW(pwzURI, '#')) {
            *ppURI = NULL;
            return E_INVALIDARG;
        }

        uri_len = lstrlenW(pwzURI);
        frag_len = lstrlenW(pwzFragment);

        /* If the fragment doesn't start with a '#', one will be added. */
        add_pound = *pwzFragment != '#';

        if(add_pound)
            uriW = malloc((uri_len + frag_len + 2) * sizeof(WCHAR));
        else
            uriW = malloc((uri_len + frag_len + 1) * sizeof(WCHAR));

        if(!uriW)
            return E_OUTOFMEMORY;

        memcpy(uriW, pwzURI, uri_len*sizeof(WCHAR));
        if(add_pound)
            uriW[uri_len++] = '#';
        memcpy(uriW+uri_len, pwzFragment, (frag_len+1)*sizeof(WCHAR));

        hres = CreateUri(uriW, dwFlags, 0, ppURI);

        free(uriW);
    } else
        /* A fragment string wasn't specified, so just forward the call. */
        hres = CreateUri(pwzURI, dwFlags, 0, ppURI);

    return hres;
}

static HRESULT build_uri(const UriBuilder *builder, IUri **uri, DWORD create_flags,
                         DWORD use_orig_flags, DWORD encoding_mask)
{
    HRESULT hr;
    parse_data data;
    Uri *ret;

    if(!uri)
        return E_POINTER;

    if(encoding_mask && (!builder->uri || builder->modified_props)) {
        *uri = NULL;
        return E_NOTIMPL;
    }

    /* Decide what flags should be used when creating the Uri. */
    if((use_orig_flags & UriBuilder_USE_ORIGINAL_FLAGS) && builder->uri)
        create_flags = builder->uri->create_flags;
    else {
        if(has_invalid_flag_combination(create_flags)) {
            *uri = NULL;
            return E_INVALIDARG;
        }

        /* Set the default flags if they don't cause a conflict. */
        apply_default_flags(&create_flags);
    }

    /* Return the base IUri if no changes have been made and the create_flags match. */
    if(builder->uri && !builder->modified_props && builder->uri->create_flags == create_flags) {
        *uri = &builder->uri->IUri_iface;
        IUri_AddRef(*uri);
        return S_OK;
    }

    hr = validate_components(builder, &data, create_flags);
    if(FAILED(hr)) {
        *uri = NULL;
        return hr;
    }

    hr = Uri_Construct(NULL, (void**)&ret);
    if(FAILED(hr)) {
        *uri = NULL;
        return hr;
    }

    hr = generate_uri(builder, &data, ret, create_flags);
    if(FAILED(hr)) {
        IUri_Release(&ret->IUri_iface);
        *uri = NULL;
        return hr;
    }

    *uri = &ret->IUri_iface;
    return S_OK;
}

static inline UriBuilder* impl_from_IUriBuilder(IUriBuilder *iface)
{
    return CONTAINING_RECORD(iface, UriBuilder, IUriBuilder_iface);
}

static HRESULT WINAPI UriBuilder_QueryInterface(IUriBuilder *iface, REFIID riid, void **ppv)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IUriBuilder_iface;
    }else if(IsEqualGUID(&IID_IUriBuilder, riid)) {
        TRACE("(%p)->(IID_IUriBuilder %p)\n", This, ppv);
        *ppv = &This->IUriBuilder_iface;
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI UriBuilder_AddRef(IUriBuilder *iface)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI UriBuilder_Release(IUriBuilder *iface)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->uri) IUri_Release(&This->uri->IUri_iface);
        free(This->fragment);
        free(This->host);
        free(This->password);
        free(This->path);
        free(This->query);
        free(This->scheme);
        free(This->username);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI UriBuilder_CreateUriSimple(IUriBuilder *iface,
                                                 DWORD        dwAllowEncodingPropertyMask,
                                                 DWORD_PTR    dwReserved,
                                                 IUri       **ppIUri)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    HRESULT hr;
    TRACE("(%p)->(%ld %Id %p)\n", This, dwAllowEncodingPropertyMask, dwReserved, ppIUri);

    hr = build_uri(This, ppIUri, 0, UriBuilder_USE_ORIGINAL_FLAGS, dwAllowEncodingPropertyMask);
    if(hr == E_NOTIMPL)
        FIXME("(%p)->(%ld %Id %p)\n", This, dwAllowEncodingPropertyMask, dwReserved, ppIUri);
    return hr;
}

static HRESULT WINAPI UriBuilder_CreateUri(IUriBuilder *iface,
                                           DWORD        dwCreateFlags,
                                           DWORD        dwAllowEncodingPropertyMask,
                                           DWORD_PTR    dwReserved,
                                           IUri       **ppIUri)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    HRESULT hr;
    TRACE("(%p)->(0x%08lx %ld %Id %p)\n", This, dwCreateFlags, dwAllowEncodingPropertyMask, dwReserved, ppIUri);

    if(dwCreateFlags == -1)
        hr = build_uri(This, ppIUri, 0, UriBuilder_USE_ORIGINAL_FLAGS, dwAllowEncodingPropertyMask);
    else
        hr = build_uri(This, ppIUri, dwCreateFlags, 0, dwAllowEncodingPropertyMask);

    if(hr == E_NOTIMPL)
        FIXME("(%p)->(0x%08lx %ld %Id %p)\n", This, dwCreateFlags, dwAllowEncodingPropertyMask, dwReserved, ppIUri);
    return hr;
}

static HRESULT WINAPI UriBuilder_CreateUriWithFlags(IUriBuilder *iface,
                                         DWORD        dwCreateFlags,
                                         DWORD        dwUriBuilderFlags,
                                         DWORD        dwAllowEncodingPropertyMask,
                                         DWORD_PTR    dwReserved,
                                         IUri       **ppIUri)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    HRESULT hr;
    TRACE("(%p)->(0x%08lx 0x%08lx %ld %Id %p)\n", This, dwCreateFlags, dwUriBuilderFlags,
        dwAllowEncodingPropertyMask, dwReserved, ppIUri);

    hr = build_uri(This, ppIUri, dwCreateFlags, dwUriBuilderFlags, dwAllowEncodingPropertyMask);
    if(hr == E_NOTIMPL)
        FIXME("(%p)->(0x%08lx 0x%08lx %ld %Id %p)\n", This, dwCreateFlags, dwUriBuilderFlags,
            dwAllowEncodingPropertyMask, dwReserved, ppIUri);
    return hr;
}

static HRESULT WINAPI  UriBuilder_GetIUri(IUriBuilder *iface, IUri **ppIUri)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%p)\n", This, ppIUri);

    if(!ppIUri)
        return E_POINTER;

    if(This->uri) {
        IUri *uri = &This->uri->IUri_iface;
        IUri_AddRef(uri);
        *ppIUri = uri;
    } else
        *ppIUri = NULL;

    return S_OK;
}

static HRESULT WINAPI UriBuilder_SetIUri(IUriBuilder *iface, IUri *pIUri)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%p)\n", This, pIUri);

    if(pIUri) {
        Uri *uri;

        if((uri = get_uri_obj(pIUri))) {
            /* Only reset the builder if its Uri isn't the same as
             * the Uri passed to the function.
             */
            if(This->uri != uri) {
                reset_builder(This);

                This->uri = uri;
                if(uri->has_port)
                    This->port = uri->port;

                IUri_AddRef(pIUri);
            }
        } else {
            FIXME("(%p)->(%p) Unknown IUri types not supported yet.\n", This, pIUri);
            return E_NOTIMPL;
        }
    } else if(This->uri)
        /* Only reset the builder if its Uri isn't NULL. */
        reset_builder(This);

    return S_OK;
}

static HRESULT WINAPI UriBuilder_GetFragment(IUriBuilder *iface, DWORD *pcchFragment, LPCWSTR *ppwzFragment)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%p %p)\n", This, pcchFragment, ppwzFragment);

    if(!This->uri || This->uri->fragment_start == -1 || This->modified_props & Uri_HAS_FRAGMENT)
        return get_builder_component(&This->fragment, &This->fragment_len, NULL, 0, ppwzFragment, pcchFragment);
    else
        return get_builder_component(&This->fragment, &This->fragment_len, This->uri->canon_uri+This->uri->fragment_start,
                                     This->uri->fragment_len, ppwzFragment, pcchFragment);
}

static HRESULT WINAPI UriBuilder_GetHost(IUriBuilder *iface, DWORD *pcchHost, LPCWSTR *ppwzHost)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%p %p)\n", This, pcchHost, ppwzHost);

    if(!This->uri || This->uri->host_start == -1 || This->modified_props & Uri_HAS_HOST)
        return get_builder_component(&This->host, &This->host_len, NULL, 0, ppwzHost, pcchHost);
    else {
        if(This->uri->host_type == Uri_HOST_IPV6)
            /* Don't include the '[' and ']' around the address. */
            return get_builder_component(&This->host, &This->host_len, This->uri->canon_uri+This->uri->host_start+1,
                                         This->uri->host_len-2, ppwzHost, pcchHost);
        else
            return get_builder_component(&This->host, &This->host_len, This->uri->canon_uri+This->uri->host_start,
                                         This->uri->host_len, ppwzHost, pcchHost);
    }
}

static HRESULT WINAPI UriBuilder_GetPassword(IUriBuilder *iface, DWORD *pcchPassword, LPCWSTR *ppwzPassword)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%p %p)\n", This, pcchPassword, ppwzPassword);

    if(!This->uri || This->uri->userinfo_split == -1 || This->modified_props & Uri_HAS_PASSWORD)
        return get_builder_component(&This->password, &This->password_len, NULL, 0, ppwzPassword, pcchPassword);
    else {
        const WCHAR *start = This->uri->canon_uri+This->uri->userinfo_start+This->uri->userinfo_split+1;
        DWORD len = This->uri->userinfo_len-This->uri->userinfo_split-1;
        return get_builder_component(&This->password, &This->password_len, start, len, ppwzPassword, pcchPassword);
    }
}

static HRESULT WINAPI UriBuilder_GetPath(IUriBuilder *iface, DWORD *pcchPath, LPCWSTR *ppwzPath)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%p %p)\n", This, pcchPath, ppwzPath);

    if(!This->uri || This->uri->path_start == -1 || This->modified_props & Uri_HAS_PATH)
        return get_builder_component(&This->path, &This->path_len, NULL, 0, ppwzPath, pcchPath);
    else
        return get_builder_component(&This->path, &This->path_len, This->uri->canon_uri+This->uri->path_start,
                                     This->uri->path_len, ppwzPath, pcchPath);
}

static HRESULT WINAPI UriBuilder_GetPort(IUriBuilder *iface, BOOL *pfHasPort, DWORD *pdwPort)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%p %p)\n", This, pfHasPort, pdwPort);

    if(!pfHasPort) {
        if(pdwPort)
            *pdwPort = 0;
        return E_POINTER;
    }

    if(!pdwPort) {
        *pfHasPort = FALSE;
        return E_POINTER;
    }

    *pfHasPort = This->has_port;
    *pdwPort = This->port;
    return S_OK;
}

static HRESULT WINAPI UriBuilder_GetQuery(IUriBuilder *iface, DWORD *pcchQuery, LPCWSTR *ppwzQuery)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%p %p)\n", This, pcchQuery, ppwzQuery);

    if(!This->uri || This->uri->query_start == -1 || This->modified_props & Uri_HAS_QUERY)
        return get_builder_component(&This->query, &This->query_len, NULL, 0, ppwzQuery, pcchQuery);
    else
        return get_builder_component(&This->query, &This->query_len, This->uri->canon_uri+This->uri->query_start,
                                     This->uri->query_len, ppwzQuery, pcchQuery);
}

static HRESULT WINAPI UriBuilder_GetSchemeName(IUriBuilder *iface, DWORD *pcchSchemeName, LPCWSTR *ppwzSchemeName)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%p %p)\n", This, pcchSchemeName, ppwzSchemeName);

    if(!This->uri || This->uri->scheme_start == -1 || This->modified_props & Uri_HAS_SCHEME_NAME)
        return get_builder_component(&This->scheme, &This->scheme_len, NULL, 0, ppwzSchemeName, pcchSchemeName);
    else
        return get_builder_component(&This->scheme, &This->scheme_len, This->uri->canon_uri+This->uri->scheme_start,
                                     This->uri->scheme_len, ppwzSchemeName, pcchSchemeName);
}

static HRESULT WINAPI UriBuilder_GetUserName(IUriBuilder *iface, DWORD *pcchUserName, LPCWSTR *ppwzUserName)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%p %p)\n", This, pcchUserName, ppwzUserName);

    if(!This->uri || This->uri->userinfo_start == -1 || This->uri->userinfo_split == 0 ||
       This->modified_props & Uri_HAS_USER_NAME)
        return get_builder_component(&This->username, &This->username_len, NULL, 0, ppwzUserName, pcchUserName);
    else {
        const WCHAR *start = This->uri->canon_uri+This->uri->userinfo_start;

        /* Check if there's a password in the userinfo section. */
        if(This->uri->userinfo_split > -1)
            /* Don't include the password. */
            return get_builder_component(&This->username, &This->username_len, start,
                                         This->uri->userinfo_split, ppwzUserName, pcchUserName);
        else
            return get_builder_component(&This->username, &This->username_len, start,
                                         This->uri->userinfo_len, ppwzUserName, pcchUserName);
    }
}

static HRESULT WINAPI UriBuilder_SetFragment(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return set_builder_component(&This->fragment, &This->fragment_len, pwzNewValue, '#',
                                 &This->modified_props, Uri_HAS_FRAGMENT);
}

static HRESULT WINAPI UriBuilder_SetHost(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));

    /* Host name can't be set to NULL. */
    if(!pwzNewValue)
        return E_INVALIDARG;

    return set_builder_component(&This->host, &This->host_len, pwzNewValue, 0,
                                 &This->modified_props, Uri_HAS_HOST);
}

static HRESULT WINAPI UriBuilder_SetPassword(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return set_builder_component(&This->password, &This->password_len, pwzNewValue, 0,
                                 &This->modified_props, Uri_HAS_PASSWORD);
}

static HRESULT WINAPI UriBuilder_SetPath(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return set_builder_component(&This->path, &This->path_len, pwzNewValue, 0,
                                 &This->modified_props, Uri_HAS_PATH);
}

static HRESULT WINAPI UriBuilder_SetPort(IUriBuilder *iface, BOOL fHasPort, DWORD dwNewValue)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%d %ld)\n", This, fHasPort, dwNewValue);

    This->has_port = fHasPort;
    This->port = dwNewValue;
    This->modified_props |= Uri_HAS_PORT;
    return S_OK;
}

static HRESULT WINAPI UriBuilder_SetQuery(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return set_builder_component(&This->query, &This->query_len, pwzNewValue, '?',
                                 &This->modified_props, Uri_HAS_QUERY);
}

static HRESULT WINAPI UriBuilder_SetSchemeName(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));

    /* Only set the scheme name if it's not NULL or empty. */
    if(!pwzNewValue || !*pwzNewValue)
        return E_INVALIDARG;

    return set_builder_component(&This->scheme, &This->scheme_len, pwzNewValue, 0,
                                 &This->modified_props, Uri_HAS_SCHEME_NAME);
}

static HRESULT WINAPI UriBuilder_SetUserName(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return set_builder_component(&This->username, &This->username_len, pwzNewValue, 0,
                                 &This->modified_props, Uri_HAS_USER_NAME);
}

static HRESULT WINAPI UriBuilder_RemoveProperties(IUriBuilder *iface, DWORD dwPropertyMask)
{
    const DWORD accepted_flags = Uri_HAS_AUTHORITY|Uri_HAS_DOMAIN|Uri_HAS_EXTENSION|Uri_HAS_FRAGMENT|Uri_HAS_HOST|
                                 Uri_HAS_PASSWORD|Uri_HAS_PATH|Uri_HAS_PATH_AND_QUERY|Uri_HAS_QUERY|
                                 Uri_HAS_USER_INFO|Uri_HAS_USER_NAME;

    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(0x%08lx)\n", This, dwPropertyMask);

    if(dwPropertyMask & ~accepted_flags)
        return E_INVALIDARG;

    if(dwPropertyMask & Uri_HAS_FRAGMENT)
        UriBuilder_SetFragment(iface, NULL);

    /* Even though you can't set the host name to NULL or an
     * empty string, you can still remove it... for some reason.
     */
    if(dwPropertyMask & Uri_HAS_HOST)
        set_builder_component(&This->host, &This->host_len, NULL, 0,
                              &This->modified_props, Uri_HAS_HOST);

    if(dwPropertyMask & Uri_HAS_PASSWORD)
        UriBuilder_SetPassword(iface, NULL);

    if(dwPropertyMask & Uri_HAS_PATH)
        UriBuilder_SetPath(iface, NULL);

    if(dwPropertyMask & Uri_HAS_PORT)
        UriBuilder_SetPort(iface, FALSE, 0);

    if(dwPropertyMask & Uri_HAS_QUERY)
        UriBuilder_SetQuery(iface, NULL);

    if(dwPropertyMask & Uri_HAS_USER_NAME)
        UriBuilder_SetUserName(iface, NULL);

    return S_OK;
}

static HRESULT WINAPI UriBuilder_HasBeenModified(IUriBuilder *iface, BOOL *pfModified)
{
    UriBuilder *This = impl_from_IUriBuilder(iface);
    TRACE("(%p)->(%p)\n", This, pfModified);

    if(!pfModified)
        return E_POINTER;

    *pfModified = This->modified_props > 0;
    return S_OK;
}

static const IUriBuilderVtbl UriBuilderVtbl = {
    UriBuilder_QueryInterface,
    UriBuilder_AddRef,
    UriBuilder_Release,
    UriBuilder_CreateUriSimple,
    UriBuilder_CreateUri,
    UriBuilder_CreateUriWithFlags,
    UriBuilder_GetIUri,
    UriBuilder_SetIUri,
    UriBuilder_GetFragment,
    UriBuilder_GetHost,
    UriBuilder_GetPassword,
    UriBuilder_GetPath,
    UriBuilder_GetPort,
    UriBuilder_GetQuery,
    UriBuilder_GetSchemeName,
    UriBuilder_GetUserName,
    UriBuilder_SetFragment,
    UriBuilder_SetHost,
    UriBuilder_SetPassword,
    UriBuilder_SetPath,
    UriBuilder_SetPort,
    UriBuilder_SetQuery,
    UriBuilder_SetSchemeName,
    UriBuilder_SetUserName,
    UriBuilder_RemoveProperties,
    UriBuilder_HasBeenModified,
};

/***********************************************************************
 *           CreateIUriBuilder (urlmon.@)
 */
HRESULT WINAPI CreateIUriBuilder(IUri *pIUri, DWORD dwFlags, DWORD_PTR dwReserved, IUriBuilder **ppIUriBuilder)
{
    UriBuilder *ret;

    TRACE("(%p %lx %Ix %p)\n", pIUri, dwFlags, dwReserved, ppIUriBuilder);

    if(!ppIUriBuilder)
        return E_POINTER;

    ret = calloc(1, sizeof(UriBuilder));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IUriBuilder_iface.lpVtbl = &UriBuilderVtbl;
    ret->ref = 1;

    if(pIUri) {
        Uri *uri;

        if((uri = get_uri_obj(pIUri))) {
            if(!uri->create_flags) {
                free(ret);
                return E_UNEXPECTED;
            }
            IUri_AddRef(pIUri);
            ret->uri = uri;

            if(uri->has_port)
                /* Windows doesn't set 'has_port' to TRUE in this case. */
                ret->port = uri->port;

        } else {
            free(ret);
            *ppIUriBuilder = NULL;
            FIXME("(%p %lx %Ix %p): Unknown IUri types not supported yet.\n", pIUri, dwFlags,
                  dwReserved, ppIUriBuilder);
            return E_NOTIMPL;
        }
    }

    *ppIUriBuilder = &ret->IUriBuilder_iface;
    return S_OK;
}

/* Merges the base path with the relative path and stores the resulting path
 * and path len in 'result' and 'result_len'.
 */
static HRESULT merge_paths(parse_data *data, const WCHAR *base, DWORD base_len, const WCHAR *relative,
                           DWORD relative_len, WCHAR **result, DWORD *result_len, DWORD flags)
{
    const WCHAR *end = NULL;
    DWORD base_copy_len = 0;
    WCHAR *ptr;

    if(base_len) {
        if(data->scheme_type == URL_SCHEME_MK && *relative == '/') {
            /* Find '::' segment */
            for(end = base; end < base+base_len-1; end++) {
                if(end[0] == ':' && end[1] == ':') {
                    end++;
                    break;
                }
            }

            /* If not found, try finding the end of @xxx: */
            if(end == base+base_len-1)
                end = *base == '@' ? wmemchr(base, ':', base_len) : NULL;
        }else {
            /* Find the characters that will be copied over from the base path. */
            for (end = base + base_len - 1; end >= base; end--) if (*end == '/') break;
            if(end < base && data->scheme_type == URL_SCHEME_FILE)
                /* Try looking for a '\\'. */
                for (end = base + base_len - 1; end >= base; end--) if (*end == '\\') break;
        }
    }

    if (end) base_copy_len = (end+1)-base;
    *result = malloc((base_copy_len + relative_len + 1) * sizeof(WCHAR));

    if(!(*result)) {
        *result_len = 0;
        return E_OUTOFMEMORY;
    }

    ptr = *result;
    memcpy(ptr, base, base_copy_len*sizeof(WCHAR));
    ptr += base_copy_len;

    memcpy(ptr, relative, relative_len*sizeof(WCHAR));
    ptr += relative_len;
    *ptr = '\0';

    *result_len = (ptr-*result);
    TRACE("ret %s\n", debugstr_wn(*result, *result_len));
    return S_OK;
}

static HRESULT combine_uri(Uri *base, Uri *relative, DWORD flags, IUri **result, DWORD extras) {
    Uri *ret;
    HRESULT hr;
    parse_data data;
    Uri *proc_uri = base;
    DWORD create_flags = 0, len = 0;

    memset(&data, 0, sizeof(parse_data));

    /* Base case is when the relative Uri has a scheme name,
     * if it does, then 'result' will contain the same data
     * as the relative Uri.
     */
    if(relative->scheme_start > -1) {
        data.uri = SysAllocString(relative->raw_uri);
        if(!data.uri) {
            *result = NULL;
            return E_OUTOFMEMORY;
        }

        parse_uri(&data, Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME);

        hr = Uri_Construct(NULL, (void**)&ret);
        if(FAILED(hr)) {
            *result = NULL;
            return hr;
        }

        if(extras & COMBINE_URI_FORCE_FLAG_USE) {
            if(flags & URL_DONT_SIMPLIFY)
                create_flags |= Uri_CREATE_NO_CANONICALIZE;
            if(flags & URL_DONT_UNESCAPE_EXTRA_INFO)
                create_flags |= Uri_CREATE_NO_DECODE_EXTRA_INFO;
        }

        ret->raw_uri = data.uri;
        hr = canonicalize_uri(&data, ret, create_flags);
        if(FAILED(hr)) {
            IUri_Release(&ret->IUri_iface);
            *result = NULL;
            return hr;
        }

        apply_default_flags(&create_flags);
        ret->create_flags = create_flags;

        *result = &ret->IUri_iface;
    } else {
        WCHAR *path = NULL;
        DWORD raw_flags = 0;

        if(base->scheme_start > -1) {
            data.scheme = base->canon_uri+base->scheme_start;
            data.scheme_len = base->scheme_len;
            data.scheme_type = base->scheme_type;
        } else {
            data.is_relative = TRUE;
            data.scheme_type = URL_SCHEME_UNKNOWN;
            create_flags |= Uri_CREATE_ALLOW_RELATIVE;
        }

        if(relative->authority_start > -1)
            proc_uri = relative;

        if(proc_uri->authority_start > -1) {
            if(proc_uri->userinfo_start > -1 && proc_uri->userinfo_split != 0) {
                data.username = proc_uri->canon_uri+proc_uri->userinfo_start;
                data.username_len = (proc_uri->userinfo_split > -1) ? proc_uri->userinfo_split : proc_uri->userinfo_len;
            }

            if(proc_uri->userinfo_split > -1) {
                data.password = proc_uri->canon_uri+proc_uri->userinfo_start+proc_uri->userinfo_split+1;
                data.password_len = proc_uri->userinfo_len-proc_uri->userinfo_split-1;
            }

            if(proc_uri->host_start > -1) {
                const WCHAR *host = proc_uri->canon_uri+proc_uri->host_start;
                parse_host(&host, &data, 0);
            }

            if(proc_uri->has_port) {
                data.has_port = TRUE;
                data.port_value = proc_uri->port;
            }
        } else if(base->scheme_type != URL_SCHEME_FILE)
            data.is_opaque = TRUE;

        if(proc_uri == relative || relative->path_start == -1 || !relative->path_len) {
            if(proc_uri->path_start > -1) {
                data.path = proc_uri->canon_uri+proc_uri->path_start;
                data.path_len = proc_uri->path_len;
            } else if(!data.is_opaque) {
                /* Just set the path as a '/' if the base didn't have
                 * one and if it's a hierarchical URI.
                 */
                data.path = L"/";
                data.path_len = 1;
            }

            if(relative->query_start > -1)
                proc_uri = relative;

            if(proc_uri->query_start > -1) {
                data.query = proc_uri->canon_uri+proc_uri->query_start;
                data.query_len = proc_uri->query_len;
            }
        } else {
            const WCHAR *ptr, **pptr;
            DWORD path_offset = 0, path_len = 0;

            /* There's two possibilities on what will happen to the path component
             * of the result IUri. First, if the relative path begins with a '/'
             * then the resulting path will just be the relative path. Second, if
             * relative path doesn't begin with a '/' then the base path and relative
             * path are merged together.
             */
            if(relative->path_len && *(relative->canon_uri+relative->path_start) == '/' && data.scheme_type != URL_SCHEME_MK) {
                WCHAR *tmp = NULL;
                BOOL copy_drive_path = FALSE;

                /* If the relative IUri's path starts with a '/', then we
                 * don't use the base IUri's path. Unless the base IUri
                 * is a file URI, in which case it uses the drive path of
                 * the base IUri (if it has any) in the new path.
                 */
                if(base->scheme_type == URL_SCHEME_FILE) {
                    if(base->path_len > 3 && *(base->canon_uri+base->path_start) == '/' &&
                       is_drive_path(base->canon_uri+base->path_start+1)) {
                        path_len += 3;
                        copy_drive_path = TRUE;
                    }
                }

                path_len += relative->path_len;

                path = malloc((path_len + 1) * sizeof(WCHAR));
                if(!path) {
                    *result = NULL;
                    return E_OUTOFMEMORY;
                }

                tmp = path;

                /* Copy the base paths, drive path over. */
                if(copy_drive_path) {
                    memcpy(tmp, base->canon_uri+base->path_start, 3*sizeof(WCHAR));
                    tmp += 3;
                }

                memcpy(tmp, relative->canon_uri+relative->path_start, relative->path_len*sizeof(WCHAR));
                path[path_len] = '\0';
            } else {
                /* Merge the base path with the relative path. */
                hr = merge_paths(&data, base->canon_uri+base->path_start, base->path_len,
                                 relative->canon_uri+relative->path_start, relative->path_len,
                                 &path, &path_len, flags);
                if(FAILED(hr)) {
                    *result = NULL;
                    return hr;
                }

                /* If the resulting IUri is a file URI, the drive path isn't
                 * reduced out when the dot segments are removed.
                 */
                if(path_len >= 3 && data.scheme_type == URL_SCHEME_FILE && !data.host) {
                    if(*path == '/' && is_drive_path(path+1))
                        path_offset = 2;
                    else if(is_drive_path(path))
                        path_offset = 1;
                }
            }

            /* Check if the dot segments need to be removed from the path. */
            if(!(flags & URL_DONT_SIMPLIFY) && !data.is_opaque) {
                DWORD offset = (path_offset > 0) ? path_offset+1 : 0;
                DWORD new_len = remove_dot_segments(path+offset,path_len-offset);

                if(new_len != path_len) {
                    WCHAR *tmp = realloc(path, (offset + new_len + 1) * sizeof(WCHAR));
                    if(!tmp) {
                        free(path);
                        *result = NULL;
                        return E_OUTOFMEMORY;
                    }

                    tmp[new_len+offset] = '\0';
                    path = tmp;
                    path_len = new_len+offset;
                }
            }

            if(relative->query_start > -1) {
                data.query = relative->canon_uri+relative->query_start;
                data.query_len = relative->query_len;
            }

            /* Make sure the path component is valid. */
            ptr = path;
            pptr = &ptr;
            if((data.is_opaque && !parse_path_opaque(pptr, &data, 0)) ||
               (!data.is_opaque && !parse_path_hierarchical(pptr, &data, 0))) {
                free(path);
                *result = NULL;
                return E_INVALIDARG;
            }
        }

        if(relative->fragment_start > -1) {
            data.fragment = relative->canon_uri+relative->fragment_start;
            data.fragment_len = relative->fragment_len;
        }

        if(flags & URL_DONT_SIMPLIFY)
            raw_flags |= RAW_URI_FORCE_PORT_DISP;
        if(flags & URL_FILE_USE_PATHURL)
            raw_flags |= RAW_URI_CONVERT_TO_DOS_PATH;

        len = generate_raw_uri(&data, data.uri, raw_flags);
        data.uri = SysAllocStringLen(NULL, len);
        if(!data.uri) {
            free(path);
            *result = NULL;
            return E_OUTOFMEMORY;
        }

        generate_raw_uri(&data, data.uri, raw_flags);

        hr = Uri_Construct(NULL, (void**)&ret);
        if(FAILED(hr)) {
            SysFreeString(data.uri);
            free(path);
            *result = NULL;
            return hr;
        }

        if(flags & URL_DONT_SIMPLIFY)
            create_flags |= Uri_CREATE_NO_CANONICALIZE;
        if(flags & URL_FILE_USE_PATHURL)
            create_flags |= Uri_CREATE_FILE_USE_DOS_PATH;

        ret->raw_uri = data.uri;
        hr = canonicalize_uri(&data, ret, create_flags);
        if(FAILED(hr)) {
            IUri_Release(&ret->IUri_iface);
            *result = NULL;
            return hr;
        }

        if(flags & URL_DONT_SIMPLIFY)
            ret->display_modifiers |= URI_DISPLAY_NO_DEFAULT_PORT_AUTH;

        apply_default_flags(&create_flags);
        ret->create_flags = create_flags;
        *result = &ret->IUri_iface;

        free(path);
    }

    return S_OK;
}

/***********************************************************************
 *           CoInternetCombineIUri (urlmon.@)
 */
HRESULT WINAPI CoInternetCombineIUri(IUri *pBaseUri, IUri *pRelativeUri, DWORD dwCombineFlags,
                                     IUri **ppCombinedUri, DWORD_PTR dwReserved)
{
    HRESULT hr;
    IInternetProtocolInfo *info;
    Uri *relative, *base;
    TRACE("(%p %p %lx %p %Ix)\n", pBaseUri, pRelativeUri, dwCombineFlags, ppCombinedUri, dwReserved);

    if(!ppCombinedUri)
        return E_INVALIDARG;

    if(!pBaseUri || !pRelativeUri) {
        *ppCombinedUri = NULL;
        return E_INVALIDARG;
    }

    relative = get_uri_obj(pRelativeUri);
    base = get_uri_obj(pBaseUri);
    if(!relative || !base) {
        *ppCombinedUri = NULL;
        FIXME("(%p %p %lx %p %Ix) Unknown IUri types not supported yet.\n",
            pBaseUri, pRelativeUri, dwCombineFlags, ppCombinedUri, dwReserved);
        return E_NOTIMPL;
    }

    info = get_protocol_info(base->canon_uri);
    if(info) {
        WCHAR result[INTERNET_MAX_URL_LENGTH+1];
        DWORD result_len = 0;

        hr = IInternetProtocolInfo_CombineUrl(info, base->canon_uri, relative->canon_uri, dwCombineFlags,
                                              result, INTERNET_MAX_URL_LENGTH+1, &result_len, 0);
        IInternetProtocolInfo_Release(info);
        if(SUCCEEDED(hr)) {
            hr = CreateUri(result, Uri_CREATE_ALLOW_RELATIVE, 0, ppCombinedUri);
            if(SUCCEEDED(hr))
                return hr;
        }
    }

    return combine_uri(base, relative, dwCombineFlags, ppCombinedUri, 0);
}

/***********************************************************************
 *           CoInternetCombineUrlEx (urlmon.@)
 */
HRESULT WINAPI CoInternetCombineUrlEx(IUri *pBaseUri, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags,
                                      IUri **ppCombinedUri, DWORD_PTR dwReserved)
{
    IUri *relative;
    Uri *base;
    HRESULT hr;
    IInternetProtocolInfo *info;

    TRACE("(%p %s %lx %p %Ix)\n", pBaseUri, debugstr_w(pwzRelativeUrl), dwCombineFlags,
        ppCombinedUri, dwReserved);

    if(!ppCombinedUri)
        return E_POINTER;

    if(!pwzRelativeUrl) {
        *ppCombinedUri = NULL;
        return E_UNEXPECTED;
    }

    if(!pBaseUri) {
        *ppCombinedUri = NULL;
        return E_INVALIDARG;
    }

    base = get_uri_obj(pBaseUri);
    if(!base) {
        *ppCombinedUri = NULL;
        FIXME("(%p %s %lx %p %Ix) Unknown IUri's not supported yet.\n", pBaseUri, debugstr_w(pwzRelativeUrl),
            dwCombineFlags, ppCombinedUri, dwReserved);
        return E_NOTIMPL;
    }

    info = get_protocol_info(base->canon_uri);
    if(info) {
        WCHAR result[INTERNET_MAX_URL_LENGTH+1];
        DWORD result_len = 0;

        hr = IInternetProtocolInfo_CombineUrl(info, base->canon_uri, pwzRelativeUrl, dwCombineFlags,
                                              result, INTERNET_MAX_URL_LENGTH+1, &result_len, 0);
        IInternetProtocolInfo_Release(info);
        if(SUCCEEDED(hr)) {
            hr = CreateUri(result, Uri_CREATE_ALLOW_RELATIVE, 0, ppCombinedUri);
            if(SUCCEEDED(hr))
                return hr;
        }
    }

    hr = CreateUri(pwzRelativeUrl, Uri_CREATE_ALLOW_RELATIVE|Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, &relative);
    if(FAILED(hr)) {
        *ppCombinedUri = NULL;
        return hr;
    }

    hr = combine_uri(base, get_uri_obj(relative), dwCombineFlags, ppCombinedUri, COMBINE_URI_FORCE_FLAG_USE);

    IUri_Release(relative);
    return hr;
}

static HRESULT parse_canonicalize(const Uri *uri, DWORD flags, LPWSTR output,
                                  DWORD output_len, DWORD *result_len)
{
    const WCHAR *ptr = NULL;
    WCHAR *path = NULL;
    const WCHAR **pptr;
    DWORD len = 0;
    BOOL reduce_path;

    /* URL_UNESCAPE only has effect if none of the URL_ESCAPE flags are set. */
    const BOOL allow_unescape = !(flags & URL_ESCAPE_UNSAFE) &&
                                !(flags & URL_ESCAPE_SPACES_ONLY) &&
                                !(flags & URL_ESCAPE_PERCENT);


    /* Check if the dot segments need to be removed from the
     * path component.
     */
    if(uri->scheme_start > -1 && uri->path_start > -1) {
        ptr = uri->canon_uri+uri->scheme_start+uri->scheme_len+1;
        pptr = &ptr;
    }
    reduce_path = !(flags & URL_DONT_SIMPLIFY) &&
                  ptr && check_hierarchical(pptr);

    for(ptr = uri->canon_uri; ptr < uri->canon_uri+uri->canon_len; ++ptr) {
        BOOL do_default_action = TRUE;

        /* Keep track of the path if we need to remove dot segments from
         * it later.
         */
        if(reduce_path && !path && ptr == uri->canon_uri+uri->path_start)
            path = output+len;

        /* Check if it's time to reduce the path. */
        if(reduce_path && ptr == uri->canon_uri+uri->path_start+uri->path_len) {
            DWORD current_path_len = (output+len) - path;
            DWORD new_path_len = remove_dot_segments(path, current_path_len);

            /* Update the current length. */
            len -= (current_path_len-new_path_len);
            reduce_path = FALSE;
        }

        if(*ptr == '%') {
            const WCHAR decoded = decode_pct_val(ptr);
            if(decoded) {
                if(allow_unescape && (flags & URL_UNESCAPE)) {
                    if(len < output_len)
                        output[len] = decoded;
                    len++;
                    ptr += 2;
                    do_default_action = FALSE;
                }
            }

            /* See if %'s needed to encoded. */
            if(do_default_action && (flags & URL_ESCAPE_PERCENT)) {
                if(len + 3 < output_len)
                    pct_encode_val(*ptr, output+len);
                len += 3;
                do_default_action = FALSE;
            }
        } else if(*ptr == ' ') {
            if((flags & URL_ESCAPE_SPACES_ONLY) &&
               !(flags & URL_ESCAPE_UNSAFE)) {
                if(len + 3 < output_len)
                    pct_encode_val(*ptr, output+len);
                len += 3;
                do_default_action = FALSE;
            }
        } else if(is_ascii(*ptr) && !is_reserved(*ptr) && !is_unreserved(*ptr)) {
            if(flags & URL_ESCAPE_UNSAFE) {
                if(len + 3 < output_len)
                    pct_encode_val(*ptr, output+len);
                len += 3;
                do_default_action = FALSE;
            }
        }

        if(do_default_action) {
            if(len < output_len)
                output[len] = *ptr;
            len++;
        }
    }

    /* Sometimes the path is the very last component of the IUri, so
     * see if the dot segments need to be reduced now.
     */
    if(reduce_path && path) {
        DWORD current_path_len = (output+len) - path;
        DWORD new_path_len = remove_dot_segments(path, current_path_len);

        /* Update the current length. */
        len -= (current_path_len-new_path_len);
    }

    if(len < output_len)
        output[len] = 0;
    else
        output[output_len-1] = 0;

    /* The null terminator isn't included in the length. */
    *result_len = len;
    if(len >= output_len)
        return STRSAFE_E_INSUFFICIENT_BUFFER;

    return S_OK;
}

static HRESULT parse_friendly(IUri *uri, LPWSTR output, DWORD output_len,
                              DWORD *result_len)
{
    HRESULT hr;
    DWORD display_len;
    BSTR display;

    hr = IUri_GetPropertyLength(uri, Uri_PROPERTY_DISPLAY_URI, &display_len, 0);
    if(FAILED(hr)) {
        *result_len = 0;
        return hr;
    }

    *result_len = display_len;
    if(display_len+1 > output_len)
        return STRSAFE_E_INSUFFICIENT_BUFFER;

    hr = IUri_GetDisplayUri(uri, &display);
    if(FAILED(hr)) {
        *result_len = 0;
        return hr;
    }

    memcpy(output, display, (display_len+1)*sizeof(WCHAR));
    SysFreeString(display);
    return S_OK;
}

static HRESULT parse_rootdocument(const Uri *uri, LPWSTR output, DWORD output_len,
                                  DWORD *result_len)
{
    static const WCHAR colon_slashesW[] = {':','/','/'};

    WCHAR *ptr;
    DWORD len = 0;

    /* Windows only returns the root document if the URI has an authority
     * and it's not an unknown scheme type or a file scheme type.
     */
    if(uri->authority_start == -1 ||
       uri->scheme_type == URL_SCHEME_UNKNOWN ||
       uri->scheme_type == URL_SCHEME_FILE) {
        *result_len = 0;
        if(!output_len)
            return STRSAFE_E_INSUFFICIENT_BUFFER;

        output[0] = 0;
        return S_OK;
    }

    len = uri->scheme_len+uri->authority_len;
    /* For the "://" and '/' which will be added. */
    len += 4;

    if(len+1 > output_len) {
        *result_len = len;
        return STRSAFE_E_INSUFFICIENT_BUFFER;
    }

    ptr = output;
    memcpy(ptr, uri->canon_uri+uri->scheme_start, uri->scheme_len*sizeof(WCHAR));

    /* Add the "://". */
    ptr += uri->scheme_len;
    memcpy(ptr, colon_slashesW, sizeof(colon_slashesW));

    /* Add the authority. */
    ptr += ARRAY_SIZE(colon_slashesW);
    memcpy(ptr, uri->canon_uri+uri->authority_start, uri->authority_len*sizeof(WCHAR));

    /* Add the '/' after the authority. */
    ptr += uri->authority_len;
    *ptr = '/';
    ptr[1] = 0;

    *result_len = len;
    return S_OK;
}

static HRESULT parse_document(const Uri *uri, LPWSTR output, DWORD output_len,
                              DWORD *result_len)
{
    DWORD len = 0;

    /* It has to be a known scheme type, but, it can't be a file
     * scheme. It also has to hierarchical.
     */
    if(uri->scheme_type == URL_SCHEME_UNKNOWN ||
       uri->scheme_type == URL_SCHEME_FILE ||
       uri->authority_start == -1) {
        *result_len = 0;
        if(output_len < 1)
            return STRSAFE_E_INSUFFICIENT_BUFFER;

        output[0] = 0;
        return S_OK;
    }

    if(uri->fragment_start > -1)
        len = uri->fragment_start;
    else
        len = uri->canon_len;

    *result_len = len;
    if(len+1 > output_len)
        return STRSAFE_E_INSUFFICIENT_BUFFER;

    memcpy(output, uri->canon_uri, len*sizeof(WCHAR));
    output[len] = 0;
    return S_OK;
}

static HRESULT parse_path_from_url(const Uri *uri, LPWSTR output, DWORD output_len,
                                   DWORD *result_len)
{
    const WCHAR *path_ptr;
    WCHAR buffer[INTERNET_MAX_URL_LENGTH+1];
    WCHAR *ptr;

    if(uri->scheme_type != URL_SCHEME_FILE) {
        *result_len = 0;
        if(output_len > 0)
            output[0] = 0;
        return E_INVALIDARG;
    }

    ptr = buffer;
    if(uri->host_start > -1) {
        static const WCHAR slash_slashW[] = {'\\','\\'};

        memcpy(ptr, slash_slashW, sizeof(slash_slashW));
        ptr += ARRAY_SIZE(slash_slashW);
        memcpy(ptr, uri->canon_uri+uri->host_start, uri->host_len*sizeof(WCHAR));
        ptr += uri->host_len;
    }

    path_ptr = uri->canon_uri+uri->path_start;
    if(uri->path_len > 3 && *path_ptr == '/' && is_drive_path(path_ptr+1))
        /* Skip past the '/' in front of the drive path. */
        ++path_ptr;

    for(; path_ptr < uri->canon_uri+uri->path_start+uri->path_len; ++path_ptr, ++ptr) {
        BOOL do_default_action = TRUE;

        if(*path_ptr == '%') {
            const WCHAR decoded = decode_pct_val(path_ptr);
            if(decoded) {
                *ptr = decoded;
                path_ptr += 2;
                do_default_action = FALSE;
            }
        } else if(*path_ptr == '/') {
            *ptr = '\\';
            do_default_action = FALSE;
        }

        if(do_default_action)
            *ptr = *path_ptr;
    }

    *ptr = 0;

    *result_len = ptr-buffer;
    if(*result_len+1 > output_len)
        return STRSAFE_E_INSUFFICIENT_BUFFER;

    memcpy(output, buffer, (*result_len+1)*sizeof(WCHAR));
    return S_OK;
}

static HRESULT parse_url_from_path(IUri *uri, LPWSTR output, DWORD output_len,
                                   DWORD *result_len)
{
    HRESULT hr;
    BSTR received;
    DWORD len = 0;

    hr = IUri_GetPropertyLength(uri, Uri_PROPERTY_ABSOLUTE_URI, &len, 0);
    if(FAILED(hr)) {
        *result_len = 0;
        return hr;
    }

    *result_len = len;
    if(len+1 > output_len)
        return STRSAFE_E_INSUFFICIENT_BUFFER;

    hr = IUri_GetAbsoluteUri(uri, &received);
    if(FAILED(hr)) {
        *result_len = 0;
        return hr;
    }

    memcpy(output, received, (len+1)*sizeof(WCHAR));
    SysFreeString(received);

    return S_OK;
}

static HRESULT parse_schema(IUri *uri, LPWSTR output, DWORD output_len,
                            DWORD *result_len)
{
    HRESULT hr;
    DWORD len;
    BSTR received;

    hr = IUri_GetPropertyLength(uri, Uri_PROPERTY_SCHEME_NAME, &len, 0);
    if(FAILED(hr)) {
        *result_len = 0;
        return hr;
    }

    *result_len = len;
    if(len+1 > output_len)
        return STRSAFE_E_INSUFFICIENT_BUFFER;

    hr = IUri_GetSchemeName(uri, &received);
    if(FAILED(hr)) {
        *result_len = 0;
        return hr;
    }

    memcpy(output, received, (len+1)*sizeof(WCHAR));
    SysFreeString(received);

    return S_OK;
}

static HRESULT parse_site(IUri *uri, LPWSTR output, DWORD output_len, DWORD *result_len)
{
    HRESULT hr;
    DWORD len;
    BSTR received;

    hr = IUri_GetPropertyLength(uri, Uri_PROPERTY_HOST, &len, 0);
    if(FAILED(hr)) {
        *result_len = 0;
        return hr;
    }

    *result_len = len;
    if(len+1 > output_len)
        return STRSAFE_E_INSUFFICIENT_BUFFER;

    hr = IUri_GetHost(uri, &received);
    if(FAILED(hr)) {
        *result_len = 0;
        return hr;
    }

    memcpy(output, received, (len+1)*sizeof(WCHAR));
    SysFreeString(received);

    return S_OK;
}

static HRESULT parse_domain(IUri *uri, LPWSTR output, DWORD output_len, DWORD *result_len)
{
    HRESULT hr;
    DWORD len;
    BSTR received;

    hr = IUri_GetPropertyLength(uri, Uri_PROPERTY_DOMAIN, &len, 0);
    if(FAILED(hr)) {
        *result_len = 0;
        return hr;
    }

    *result_len = len;
    if(len+1 > output_len)
        return STRSAFE_E_INSUFFICIENT_BUFFER;

    hr = IUri_GetDomain(uri, &received);
    if(FAILED(hr)) {
        *result_len = 0;
        return hr;
    }

    memcpy(output, received, (len+1)*sizeof(WCHAR));
    SysFreeString(received);

    return S_OK;
}

static HRESULT parse_anchor(IUri *uri, LPWSTR output, DWORD output_len, DWORD *result_len)
{
    HRESULT hr;
    DWORD len;
    BSTR received;

    hr = IUri_GetPropertyLength(uri, Uri_PROPERTY_FRAGMENT, &len, 0);
    if(FAILED(hr)) {
        *result_len = 0;
        return hr;
    }

    *result_len = len;
    if(len+1 > output_len)
        return STRSAFE_E_INSUFFICIENT_BUFFER;

    hr = IUri_GetFragment(uri, &received);
    if(FAILED(hr)) {
        *result_len = 0;
        return hr;
    }

    memcpy(output, received, (len+1)*sizeof(WCHAR));
    SysFreeString(received);

    return S_OK;
}

/***********************************************************************
 *           CoInternetParseIUri (urlmon.@)
 */
HRESULT WINAPI CoInternetParseIUri(IUri *pIUri, PARSEACTION ParseAction, DWORD dwFlags,
                                   LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult,
                                   DWORD_PTR dwReserved)
{
    HRESULT hr;
    Uri *uri;
    IInternetProtocolInfo *info;

    TRACE("(%p %d %lx %p %ld %p %Ix)\n", pIUri, ParseAction, dwFlags, pwzResult,
        cchResult, pcchResult, dwReserved);

    if(!pcchResult)
        return E_POINTER;

    if(!pwzResult || !pIUri) {
        *pcchResult = 0;
        return E_INVALIDARG;
    }

    if(!(uri = get_uri_obj(pIUri))) {
        *pcchResult = 0;
        FIXME("(%p %d %lx %p %ld %p %Ix) Unknown IUri's not supported for this action.\n",
            pIUri, ParseAction, dwFlags, pwzResult, cchResult, pcchResult, dwReserved);
        return E_NOTIMPL;
    }

    info = get_protocol_info(uri->canon_uri);
    if(info) {
        hr = IInternetProtocolInfo_ParseUrl(info, uri->canon_uri, ParseAction, dwFlags,
                                            pwzResult, cchResult, pcchResult, 0);
        IInternetProtocolInfo_Release(info);
        if(SUCCEEDED(hr)) return hr;
    }

    switch(ParseAction) {
    case PARSE_CANONICALIZE:
        hr = parse_canonicalize(uri, dwFlags, pwzResult, cchResult, pcchResult);
        break;
    case PARSE_FRIENDLY:
        hr = parse_friendly(pIUri, pwzResult, cchResult, pcchResult);
        break;
    case PARSE_ROOTDOCUMENT:
        hr = parse_rootdocument(uri, pwzResult, cchResult, pcchResult);
        break;
    case PARSE_DOCUMENT:
        hr = parse_document(uri, pwzResult, cchResult, pcchResult);
        break;
    case PARSE_PATH_FROM_URL:
        hr = parse_path_from_url(uri, pwzResult, cchResult, pcchResult);
        break;
    case PARSE_URL_FROM_PATH:
        hr = parse_url_from_path(pIUri, pwzResult, cchResult, pcchResult);
        break;
    case PARSE_SCHEMA:
        hr = parse_schema(pIUri, pwzResult, cchResult, pcchResult);
        break;
    case PARSE_SITE:
        hr = parse_site(pIUri, pwzResult, cchResult, pcchResult);
        break;
    case PARSE_DOMAIN:
        hr = parse_domain(pIUri, pwzResult, cchResult, pcchResult);
        break;
    case PARSE_LOCATION:
    case PARSE_ANCHOR:
        hr = parse_anchor(pIUri, pwzResult, cchResult, pcchResult);
        break;
    case PARSE_SECURITY_URL:
    case PARSE_MIME:
    case PARSE_SERVER:
    case PARSE_SECURITY_DOMAIN:
        *pcchResult = 0;
        hr = E_FAIL;
        break;
    default:
        *pcchResult = 0;
        hr = E_NOTIMPL;
        FIXME("(%p %d %lx %p %ld %p %Ix) Partial stub.\n", pIUri, ParseAction, dwFlags,
            pwzResult, cchResult, pcchResult, dwReserved);
    }

    return hr;
}
