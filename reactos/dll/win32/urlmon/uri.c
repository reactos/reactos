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

#include "urlmon_main.h"
#include "wine/debug.h"

#define NO_SHLWAPI_REG
#include "shlwapi.h"

#define UINT_MAX 0xffffffff

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct {
    const IUriVtbl  *lpIUriVtbl;
    LONG ref;

    BSTR            raw_uri;

    /* Information about the canonicalized URI's buffer. */
    WCHAR           *canon_uri;
    DWORD           canon_size;
    DWORD           canon_len;

    INT             scheme_start;
    DWORD           scheme_len;
    URL_SCHEME      scheme_type;

    INT             userinfo_start;
    DWORD           userinfo_len;
    INT             userinfo_split;

    INT             host_start;
    DWORD           host_len;
    Uri_HOST_TYPE   host_type;
} Uri;

typedef struct {
    const IUriBuilderVtbl  *lpIUriBuilderVtbl;
    LONG ref;
} UriBuilder;

typedef struct {
    BSTR            uri;

    BOOL            is_relative;
    BOOL            is_opaque;
    BOOL            has_implicit_scheme;
    BOOL            has_implicit_ip;
    UINT            implicit_ipv4;

    const WCHAR     *scheme;
    DWORD           scheme_len;
    URL_SCHEME      scheme_type;

    const WCHAR     *userinfo;
    DWORD           userinfo_len;
    INT             userinfo_split;

    const WCHAR     *host;
    DWORD           host_len;
    Uri_HOST_TYPE   host_type;
} parse_data;

static const CHAR hexDigits[] = "0123456789ABCDEF";

/* List of scheme types/scheme names that are recognized by the IUri interface as of IE 7. */
static const struct {
    URL_SCHEME  scheme;
    WCHAR       scheme_name[16];
} recognized_schemes[] = {
    {URL_SCHEME_FTP,            {'f','t','p',0}},
    {URL_SCHEME_HTTP,           {'h','t','t','p',0}},
    {URL_SCHEME_GOPHER,         {'g','o','p','h','e','r',0}},
    {URL_SCHEME_MAILTO,         {'m','a','i','l','t','o',0}},
    {URL_SCHEME_NEWS,           {'n','e','w','s',0}},
    {URL_SCHEME_NNTP,           {'n','n','t','p',0}},
    {URL_SCHEME_TELNET,         {'t','e','l','n','e','t',0}},
    {URL_SCHEME_WAIS,           {'w','a','i','s',0}},
    {URL_SCHEME_FILE,           {'f','i','l','e',0}},
    {URL_SCHEME_MK,             {'m','k',0}},
    {URL_SCHEME_HTTPS,          {'h','t','t','p','s',0}},
    {URL_SCHEME_SHELL,          {'s','h','e','l','l',0}},
    {URL_SCHEME_SNEWS,          {'s','n','e','w','s',0}},
    {URL_SCHEME_LOCAL,          {'l','o','c','a','l',0}},
    {URL_SCHEME_JAVASCRIPT,     {'j','a','v','a','s','c','r','i','p','t',0}},
    {URL_SCHEME_VBSCRIPT,       {'v','b','s','c','r','i','p','t',0}},
    {URL_SCHEME_ABOUT,          {'a','b','o','u','t',0}},
    {URL_SCHEME_RES,            {'r','e','s',0}},
    {URL_SCHEME_MSSHELLROOTED,  {'m','s','-','s','h','e','l','l','-','r','o','o','t','e','d',0}},
    {URL_SCHEME_MSSHELLIDLIST,  {'m','s','-','s','h','e','l','l','-','i','d','l','i','s','t',0}},
    {URL_SCHEME_MSHELP,         {'h','c','p',0}},
    {URL_SCHEME_WILDCARD,       {'*',0}}
};

static inline BOOL is_alpha(WCHAR val) {
	return ((val >= 'a' && val <= 'z') || (val >= 'A' && val <= 'Z'));
}

static inline BOOL is_num(WCHAR val) {
	return (val >= '0' && val <= '9');
}

/* A URI is implicitly a file path if it begins with
 * a drive letter (eg X:) or starts with "\\" (UNC path).
 */
static inline BOOL is_implicit_file_path(const WCHAR *str) {
    if(is_alpha(str[0]) && str[1] == ':')
        return TRUE;
    else if(str[0] == '\\' && str[1] == '\\')
        return TRUE;

    return FALSE;
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
 * an authority delimeter.
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
 * Eg.
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

/* Converts an IPv4 address in numerical form into it's fully qualified
 * string form. This function returns the number of characters written
 * to 'dest'. If 'dest' is NULL this function will return the number of
 * characters that would have been written.
 *
 * It's up to the caller to ensure there's enough space in 'dest' for the
 * address.
 */
static DWORD ui2ipv4(WCHAR *dest, UINT address) {
    static const WCHAR formatW[] =
        {'%','u','.','%','u','.','%','u','.','%','u',0};
    DWORD ret = 0;
    UCHAR digits[4];

    digits[0] = (address >> 24) & 0xff;
    digits[1] = (address >> 16) & 0xff;
    digits[2] = (address >> 8) & 0xff;
    digits[3] = address & 0xff;

    if(!dest) {
        WCHAR tmp[16];
        ret = sprintfW(tmp, formatW, digits[0], digits[1], digits[2], digits[3]);
    } else
        ret = sprintfW(dest, formatW, digits[0], digits[1], digits[2], digits[3]);

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
    /* Since the 1 digit requirment was meet, it doesn't
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
static BOOL parse_scheme_name(const WCHAR **ptr, parse_data *data) {
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
    if(**ptr != ':') {
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

        for(i = 0; i < sizeof(recognized_schemes)/sizeof(recognized_schemes[0]); ++i) {
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
static BOOL parse_scheme(const WCHAR **ptr, parse_data *data, DWORD flags) {
    static const WCHAR fileW[] = {'f','i','l','e',0};
    static const WCHAR wildcardW[] = {'*',0};

    /* First check to see if the uri could implicitly be a file path. */
    if(is_implicit_file_path(*ptr)) {
        if(flags & Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME) {
            data->scheme = fileW;
            data->scheme_len = lstrlenW(fileW);
            data->has_implicit_scheme = TRUE;

            TRACE("(%p %p %x): URI is an implicit file path.\n", ptr, data, flags);
        } else {
            /* Window's does not consider anything that can implicitly be a file
             * path to be a valid URI if the ALLOW_IMPLICIT_FILE_SCHEME flag is not set...
             */
            TRACE("(%p %p %x): URI is implicitly a file path, but, the ALLOW_IMPLICIT_FILE_SCHEME flag wasn't set.\n",
                    ptr, data, flags);
            return FALSE;
        }
    } else if(!parse_scheme_name(ptr, data)) {
        /* No Scheme was found, this means it could be:
         *      a) an implicit Wildcard scheme
         *      b) a relative URI
         *      c) a invalid URI.
         */
        if(flags & Uri_CREATE_ALLOW_IMPLICIT_WILDCARD_SCHEME) {
            data->scheme = wildcardW;
            data->scheme_len = lstrlenW(wildcardW);
            data->has_implicit_scheme = TRUE;

            TRACE("(%p %p %x): URI is an implicit wildcard scheme.\n", ptr, data, flags);
        } else if (flags & Uri_CREATE_ALLOW_RELATIVE) {
            data->is_relative = TRUE;
            TRACE("(%p %p %x): URI is relative.\n", ptr, data, flags);
        } else {
            TRACE("(%p %p %x): Malformed URI found. Unable to deduce scheme name.\n", ptr, data, flags);
            return FALSE;
        }
    }

    if(!data->is_relative)
        TRACE("(%p %p %x): Found scheme=%s scheme_len=%d\n", ptr, data, flags,
                debugstr_wn(data->scheme, data->scheme_len), data->scheme_len);

    if(!parse_scheme_type(data))
        return FALSE;

    TRACE("(%p %p %x): Assigned %d as the URL_SCHEME.\n", ptr, data, flags, data->scheme_type);
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
 *      uses the first occurence of ':' to delimit the username and password
 *      components.
 *
 *      ex:
 *          ftp://user:pass:word@winehq.org
 *
 *      Would yield, "user" as the username and "pass:word" as the password.
 *
 *  2)  Windows allows any character to appear in the "userinfo" part of
 *      a URI, as long as it's not an authority delimeter character set.
 */
static void parse_userinfo(const WCHAR **ptr, parse_data *data, DWORD flags) {
    data->userinfo = *ptr;
    data->userinfo_split = -1;

    while(**ptr != '@') {
        if(**ptr == ':' && data->userinfo_split == -1)
            data->userinfo_split = *ptr - data->userinfo;
        else if(**ptr == '%') {
            /* If it's a known scheme type, it has to be a valid percent
             * encoded value.
             */
            if(!check_pct_encoded(ptr)) {
                if(data->scheme_type != URL_SCHEME_UNKNOWN) {
                    *ptr = data->userinfo;
                    data->userinfo = NULL;
                    data->userinfo_split = -1;

                    TRACE("(%p %p %x): URI contained no userinfo.\n", ptr, data, flags);
                    return;
                }
            } else
                continue;
        } else if(is_auth_delim(**ptr, data->scheme_type != URL_SCHEME_UNKNOWN))
            break;

        ++(*ptr);
    }

    if(**ptr != '@') {
        *ptr = data->userinfo;
        data->userinfo = NULL;
        data->userinfo_split = -1;

        TRACE("(%p %p %x): URI contained no userinfo.\n", ptr, data, flags);
        return;
    }

    data->userinfo_len = *ptr - data->userinfo;
    TRACE("(%p %p %x): Found userinfo=%s userinfo_len=%d split=%d.\n", ptr, data, flags,
            debugstr_wn(data->userinfo, data->userinfo_len), data->userinfo_len, data->userinfo_split);
    ++(*ptr);
}

/* Attempts to parse a IPv4 address from the URI.
 *
 * NOTES:
 *  Window's normalizes IPv4 addresses, This means there's three
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
static BOOL parse_ipv4address(const WCHAR **ptr, parse_data *data, DWORD flags) {
    const BOOL is_unknown = data->scheme_type == URL_SCHEME_UNKNOWN;
    data->host = *ptr;

    if(!check_ipv4address(ptr, FALSE)) {
        if(!check_implicit_ipv4(ptr, &data->implicit_ipv4)) {
            TRACE("(%p %p %x): URI didn't contain anything looking like an IPv4 address.\n",
                ptr, data, flags);
            *ptr = data->host;
            data->host = NULL;
            return FALSE;
        } else
            data->has_implicit_ip = TRUE;
    }

    /* Check if what we found is the only part of the host name (if it isn't
     * we don't have an IPv4 address).
     */
    if(!is_auth_delim(**ptr, !is_unknown) && **ptr != ':') {
        *ptr = data->host;
        data->host = NULL;
        data->has_implicit_ip = FALSE;
        return FALSE;
    }

    data->host_len = *ptr - data->host;
    data->host_type = Uri_HOST_IPV4;
    TRACE("(%p %p %x): IPv4 address found. host=%s host_len=%d host_type=%d\n",
        ptr, data, flags, debugstr_wn(data->host, data->host_len),
        data->host_len, data->host_type);
    return TRUE;
}

/* Parses the host information from the URI.
 *
 * host = IP-literal / IPv4address / reg-name
 */
static BOOL parse_host(const WCHAR **ptr, parse_data *data, DWORD flags) {
    if(!parse_ipv4address(ptr, data, flags)) {
        WARN("(%p %p %x): Only IPv4 parsing is implemented so far.\n", ptr, data, flags);
        data->host_type = Uri_HOST_UNKNOWN;
    }

    /* TODO parse IP-Literal / reg-name. */

    return TRUE;
}

/* Parses the authority information from the URI.
 *
 * authority   = [ userinfo "@" ] host [ ":" port ]
 */
static BOOL parse_authority(const WCHAR **ptr, parse_data *data, DWORD flags) {
    parse_userinfo(ptr, data, flags);

    if(!parse_host(ptr, data, flags))
        return FALSE;

    return TRUE;
}

/* Determines how the URI should be parsed after the scheme information.
 *
 * If the scheme is followed, by "//" then, it is treated as an hierarchical URI
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
 *  set then it is considered an opaque URI reguardless of what follows the scheme information
 *  (per MSDN documentation).
 */
static BOOL parse_hierpart(const WCHAR **ptr, parse_data *data, DWORD flags) {
    /* Checks if the authority information needs to be parsed.
     *
     * Relative URI's aren't hierarchical URI's, but, they could trick
     * "check_hierarchical" into thinking it is, so we need to explicitly
     * make sure it's not relative. Also, if the URI is an implicit file
     * scheme it might not contain a "//", but, it's considered hierarchical
     * anyways. Wildcard Schemes are always considered hierarchical
     */
    if(data->scheme_type == URL_SCHEME_WILDCARD ||
       data->scheme_type == URL_SCHEME_FILE ||
       (!data->is_relative && check_hierarchical(ptr))) {
        /* Only treat it as a hierarchical URI if the scheme_type is known or
         * the Uri_CREATE_NO_CRACK_UNKNOWN_SCHEMES flag is not set.
         */
        if(data->scheme_type != URL_SCHEME_UNKNOWN ||
           !(flags & Uri_CREATE_NO_CRACK_UNKNOWN_SCHEMES)) {
            TRACE("(%p %p %x): Treating URI as an hierarchical URI.\n", ptr, data, flags);
            data->is_opaque = FALSE;

            /* TODO: Handle hierarchical URI's, parse authority then parse the path. */
            if(!parse_authority(ptr, data, flags))
                return FALSE;

            return TRUE;
        }
    }

    /* If it reaches here, then the URI will be treated as an opaque
     * URI.
     */

    TRACE("(%p %p %x): Treating URI as an opaque URI.\n", ptr, data, flags);

    data->is_opaque = TRUE;
    /* TODO: Handle opaque URI's, parse path. */
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

    TRACE("(%p %x): BEGINNING TO PARSE URI %s.\n", data, flags, debugstr_w(data->uri));

    if(!parse_scheme(pptr, data, flags))
        return FALSE;

    if(!parse_hierpart(pptr, data, flags))
        return FALSE;

    TRACE("(%p %x): FINISHED PARSING URI.\n", data, flags);
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
    DWORD i = 0;

    uri->userinfo_start = uri->userinfo_split = -1;
    uri->userinfo_len = 0;

    if(!data->userinfo)
        /* URI doesn't have userinfo, so nothing to do here. */
        return TRUE;

    uri->userinfo_start = uri->canon_len;

    while(i < data->userinfo_len) {
        if(data->userinfo[i] == ':' && uri->userinfo_split == -1)
            /* Windows only considers the first ':' as the delimiter. */
            uri->userinfo_split = uri->canon_len - uri->userinfo_start;
        else if(data->userinfo[i] == '%') {
            /* Only decode % encoded values for known scheme types. */
            if(data->scheme_type != URL_SCHEME_UNKNOWN) {
                /* See if the value really needs decoded. */
                WCHAR val = decode_pct_val(data->userinfo + i);
                if(is_unreserved(val)) {
                    if(!computeOnly)
                        uri->canon_uri[uri->canon_len] = val;

                    ++uri->canon_len;

                    /* Move pass the hex characters. */
                    i += 3;
                    continue;
                }
            }
        } else if(!is_reserved(data->userinfo[i]) && !is_unreserved(data->userinfo[i]) &&
                  data->userinfo[i] != '\\') {
            /* Only percent encode forbidden characters if the NO_ENCODE_FORBIDDEN_CHARACTERS flag
             * is NOT set.
             */
            if(!(flags & Uri_CREATE_NO_ENCODE_FORBIDDEN_CHARACTERS)) {
                if(!computeOnly)
                    pct_encode_val(data->userinfo[i], uri->canon_uri + uri->canon_len);

                uri->canon_len += 3;
                ++i;
                continue;
            }
        }

        if(!computeOnly)
            /* Nothing special, so just copy the character over. */
            uri->canon_uri[uri->canon_len] = data->userinfo[i];

        ++uri->canon_len;
        ++i;
    }

    uri->userinfo_len = uri->canon_len - uri->userinfo_start;
    if(!computeOnly)
        TRACE("(%p %p %x %d): Canonicalized userinfo, userinfo_start=%d, userinfo=%s, userinfo_split=%d userinfo_len=%d.\n",
                data, uri, flags, computeOnly, uri->userinfo_start, debugstr_wn(uri->canon_uri + uri->userinfo_start, uri->userinfo_len),
                uri->userinfo_split, uri->userinfo_len);

    /* Now insert the '@' after the userinfo. */
    if(!computeOnly)
        uri->canon_uri[uri->canon_len] = '@';

    ++uri->canon_len;
    return TRUE;
}

/* Attempts to canonicalize an implicit IPv4 address. */
static BOOL canonicalize_implicit_ipv4address(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    uri->host_start = uri->canon_len;

    TRACE("%u\n", data->implicit_ipv4);
    /* For unknown scheme types Window's doesn't convert
     * the value into an IP address, but, it still considers
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
        TRACE("%p %p %x %d): Canonicalized implicit IP address=%s len=%d\n",
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
 * for an IPv4 address) it's canonicalized as if were a reg-name.
 *
 * If the parse_data contains a partial or full IPv4 address it normalizes it.
 * A partial IPv4 address is something like "192.0" and would be normalized to
 * "192.0.0.0". With a full (or partial) IPv4 address like "192.002.01.003" would
 * be normalized to "192.2.1.3".
 *
 * NOTES:
 *  Window's ONLY normalizes IPv4 address for known scheme types (one that isn't
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
            TRACE("(%p %p %x %d): Canonicalized IPv4 address, ip=%s len=%d\n",
                data, uri, flags, computeOnly,
                debugstr_wn(uri->canon_uri+uri->host_start, uri->host_len),
                uri->host_len);
    }

    return TRUE;
}

static BOOL canonicalize_host(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    uri->host_start = -1;
    uri->host_len = 0;

    if(data->host) {
        switch(data->host_type) {
        case Uri_HOST_IPV4:
            uri->host_type = Uri_HOST_IPV4;
            if(!canonicalize_ipv4address(data, uri, flags, computeOnly))
                return FALSE;

            break;
        default:
            WARN("(%p %p %x %d): Canonicalization not supported yet\n", data,
                    uri, flags, computeOnly);
       }
   }

   return TRUE;
}

/* Canonicalizes the authority of the URI represented by the parse_data. */
static BOOL canonicalize_authority(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    if(!canonicalize_userinfo(data, uri, flags, computeOnly))
        return FALSE;

    if(!canonicalize_host(data, uri, flags, computeOnly))
        return FALSE;

    /* TODO Canonicalize port information. */

    return TRUE;
}

/* Determines how the URI represented by the parse_data should be canonicalized.
 *
 * Essentially, if the parse_data represents an hierarchical URI then it calls
 * canonicalize_authority and the canonicalization functions for the path. If the
 * URI is opaque it canonicalizes the path of the URI.
 */
static BOOL canonicalize_hierpart(const parse_data *data, Uri *uri, DWORD flags, BOOL computeOnly) {
    if(!data->is_opaque) {
        /* "//" is only added for non-wildcard scheme types. */
        if(data->scheme_type != URL_SCHEME_WILDCARD) {
            if(!computeOnly) {
                INT pos = uri->canon_len;

                uri->canon_uri[pos] = '/';
                uri->canon_uri[pos+1] = '/';
           }
           uri->canon_len += 2;
        }

        if(!canonicalize_authority(data, uri, flags, computeOnly))
            return FALSE;

       /* TODO: Canonicalize the path of the URI. */

    } else {
        /* Opaque URI's don't have userinfo. */
        uri->userinfo_start = uri->userinfo_split = -1;
        uri->userinfo_len = 0;
    }

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
            FIXME("(%p %p %x): Unable to determine the scheme type of %s.\n", data,
                    uri, flags, debugstr_w(data->uri));
            return FALSE;
        }
    } else {
        if(!computeOnly) {
            DWORD i;
            INT pos = uri->canon_len;

            for(i = 0; i < data->scheme_len; ++i) {
                /* Scheme name must be lower case after canonicalization. */
                uri->canon_uri[i + pos] = tolowerW(data->scheme[i]);
            }

            uri->canon_uri[i + pos] = ':';
            uri->scheme_start = pos;

            TRACE("(%p %p %x): Canonicalized scheme=%s, len=%d.\n", data, uri, flags,
                    debugstr_wn(uri->canon_uri,  uri->scheme_len), data->scheme_len);
        }

        /* This happens in both computation modes. */
        uri->canon_len += data->scheme_len + 1;
        uri->scheme_len = data->scheme_len;
    }
    return TRUE;
}

/* Compute's what the length of the URI specified by the parse_data will be
 * after canonicalization occurs using the specified flags.
 *
 * This function will return a non-zero value indicating the length of the canonicalized
 * URI, or -1 on error.
 */
static int compute_canonicalized_length(const parse_data *data, DWORD flags) {
    Uri uri;

    memset(&uri, 0, sizeof(Uri));

    TRACE("(%p %x): Beginning to compute canonicalized length for URI %s\n", data, flags,
            debugstr_w(data->uri));

    if(!canonicalize_scheme(data, &uri, flags, TRUE)) {
        ERR("(%p %x): Failed to compute URI scheme length.\n", data, flags);
        return -1;
    }

    if(!canonicalize_hierpart(data, &uri, flags, TRUE)) {
        ERR("(%p %x): Failed to compute URI hierpart length.\n", data, flags);
        return -1;
    }

    TRACE("(%p %x): Finished computing canonicalized URI length. length=%d\n", data, flags, uri.canon_len);

    return uri.canon_len;
}

/* Canonicalizes the URI data specified in the parse_data, using the given flags. If the
 * canonicalization succeededs it will store all the canonicalization information
 * in the pointer to the Uri.
 *
 * To canonicalize a URI this function first computes what the length of the URI
 * specified by the parse_data will be. Once this is done it will then perfom the actual
 * canonicalization of the URI.
 */
static HRESULT canonicalize_uri(const parse_data *data, Uri *uri, DWORD flags) {
    INT len;

    uri->canon_uri = NULL;
    len = uri->canon_size = uri->canon_len = 0;

    TRACE("(%p %p %x): beginning to canonicalize URI %s.\n", data, uri, flags, debugstr_w(data->uri));

    /* First try to compute the length of the URI. */
    len = compute_canonicalized_length(data, flags);
    if(len == -1) {
        ERR("(%p %p %x): Could not compute the canonicalized length of %s.\n", data, uri, flags,
                debugstr_w(data->uri));
        return E_INVALIDARG;
    }

    uri->canon_uri = heap_alloc((len+1)*sizeof(WCHAR));
    if(!uri->canon_uri)
        return E_OUTOFMEMORY;

    if(!canonicalize_scheme(data, uri, flags, FALSE)) {
        ERR("(%p %p %x): Unable to canonicalize the scheme of the URI.\n", data, uri, flags);
        heap_free(uri->canon_uri);
        return E_INVALIDARG;
    }
    uri->scheme_type = data->scheme_type;

    if(!canonicalize_hierpart(data, uri, flags, FALSE)) {
        ERR("(%p %p %x): Unable to canonicalize the heirpart of the URI\n", data, uri, flags);
        heap_free(uri->canon_uri);
        return E_INVALIDARG;
    }

    uri->canon_uri[uri->canon_len] = '\0';
    TRACE("(%p %p %x): finished canonicalizing the URI.\n", data, uri, flags);

    return S_OK;
}

#define URI(x)         ((IUri*)  &(x)->lpIUriVtbl)
#define URIBUILDER(x)  ((IUriBuilder*)  &(x)->lpIUriBuilderVtbl)

#define URI_THIS(iface) DEFINE_THIS(Uri, IUri, iface)

static HRESULT WINAPI Uri_QueryInterface(IUri *iface, REFIID riid, void **ppv)
{
    Uri *This = URI_THIS(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = URI(This);
    }else if(IsEqualGUID(&IID_IUri, riid)) {
        TRACE("(%p)->(IID_IUri %p)\n", This, ppv);
        *ppv = URI(This);
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
    Uri *This = URI_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI Uri_Release(IUri *iface)
{
    Uri *This = URI_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        SysFreeString(This->raw_uri);
        heap_free(This->canon_uri);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI Uri_GetPropertyBSTR(IUri *iface, Uri_PROPERTY uriProp, BSTR *pbstrProperty, DWORD dwFlags)
{
    Uri *This = URI_THIS(iface);
    HRESULT hres;
    TRACE("(%p)->(%d %p %x)\n", This, uriProp, pbstrProperty, dwFlags);

    if(!pbstrProperty)
        return E_POINTER;

    if(uriProp > Uri_PROPERTY_STRING_LAST) {
        /* Windows allocates an empty BSTR for invalid Uri_PROPERTY's. */
        *pbstrProperty = SysAllocStringLen(NULL, 0);
        if(!(*pbstrProperty))
            return E_OUTOFMEMORY;

        /* It only returns S_FALSE for the ZONE property... */
        if(uriProp == Uri_PROPERTY_ZONE)
            return S_FALSE;
        else
            return S_OK;
    }

    /* Don't have support for flags yet. */
    if(dwFlags) {
        FIXME("(%p)->(%d %p %x)\n", This, uriProp, pbstrProperty, dwFlags);
        return E_NOTIMPL;
    }

    switch(uriProp) {
    case Uri_PROPERTY_HOST:
        if(This->host_start > -1) {
            *pbstrProperty = SysAllocStringLen(This->canon_uri+This->host_start, This->host_len);
            hres = S_OK;
        } else {
            /* Canonicalizing/parsing the host of a URI is only partially
             * implemented, so return E_NOTIMPL for now.
             */
            FIXME("(%p)->(%d %p %x) Partially implemented\n", This, uriProp, pbstrProperty, dwFlags);
            return E_NOTIMPL;
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
        if(This->userinfo_start > -1) {
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
        FIXME("(%p)->(%d %p %x)\n", This, uriProp, pbstrProperty, dwFlags);
        hres = E_NOTIMPL;
    }

    return hres;
}

static HRESULT WINAPI Uri_GetPropertyLength(IUri *iface, Uri_PROPERTY uriProp, DWORD *pcchProperty, DWORD dwFlags)
{
    Uri *This = URI_THIS(iface);
    HRESULT hres;
    TRACE("(%p)->(%d %p %x)\n", This, uriProp, pcchProperty, dwFlags);

    if(!pcchProperty)
        return E_INVALIDARG;

    /* Can only return a length for a property if it's a string. */
    if(uriProp > Uri_PROPERTY_STRING_LAST)
        return E_INVALIDARG;

    /* Don't have support for flags yet. */
    if(dwFlags) {
        FIXME("(%p)->(%d %p %x)\n", This, uriProp, pcchProperty, dwFlags);
        return E_NOTIMPL;
    }

    switch(uriProp) {
    case Uri_PROPERTY_HOST:
        if(This->host_start == -1) {
            /* Canonicalizing/parsing the host of a URI is only partially
             * implemented, so return E_NOTIMPL for now.
             */
            FIXME("(%p)->(%d %p %x) Partially implemented\n", This, uriProp, pcchProperty, dwFlags);
            return E_NOTIMPL;
        }

        *pcchProperty = This->host_len;
        hres = (This->host_start > -1) ? S_OK : S_FALSE;
        break;
    case Uri_PROPERTY_PASSWORD:
        *pcchProperty = (This->userinfo_split > -1) ? This->userinfo_len-This->userinfo_split-1 : 0;
        hres = (This->userinfo_split > -1) ? S_OK : S_FALSE;
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
        hres = (This->userinfo_start > -1) ? S_OK : S_FALSE;
        break;
    default:
        FIXME("(%p)->(%d %p %x)\n", This, uriProp, pcchProperty, dwFlags);
        hres = E_NOTIMPL;
    }

    return hres;
}

static HRESULT WINAPI Uri_GetPropertyDWORD(IUri *iface, Uri_PROPERTY uriProp, DWORD *pcchProperty, DWORD dwFlags)
{
    Uri *This = URI_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%d %p %x)\n", This, uriProp, pcchProperty, dwFlags);

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
    case Uri_PROPERTY_SCHEME:
        *pcchProperty = This->scheme_type;
        hres = S_OK;
        break;
    default:
        FIXME("(%p)->(%d %p %x)\n", This, uriProp, pcchProperty, dwFlags);
        hres = E_NOTIMPL;
    }

    return hres;
}

static HRESULT WINAPI Uri_HasProperty(IUri *iface, Uri_PROPERTY uriProp, BOOL *pfHasProperty)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%d %p)\n", This, uriProp, pfHasProperty);

    if(!pfHasProperty)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetAbsoluteUri(IUri *iface, BSTR *pstrAbsoluteUri)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrAbsoluteUri);

    if(!pstrAbsoluteUri)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetAuthority(IUri *iface, BSTR *pstrAuthority)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrAuthority);

    if(!pstrAuthority)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetDisplayUri(IUri *iface, BSTR *pstrDisplayUri)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrDisplayUri);

    if(!pstrDisplayUri)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetDomain(IUri *iface, BSTR *pstrDomain)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrDomain);

    if(!pstrDomain)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetExtension(IUri *iface, BSTR *pstrExtension)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrExtension);

    if(!pstrExtension)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetFragment(IUri *iface, BSTR *pstrFragment)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrFragment);

    if(!pstrFragment)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetHost(IUri *iface, BSTR *pstrHost)
{
    TRACE("(%p)->(%p)\n", iface, pstrHost);
    return Uri_GetPropertyBSTR(iface, Uri_PROPERTY_HOST, pstrHost, 0);
}

static HRESULT WINAPI Uri_GetPassword(IUri *iface, BSTR *pstrPassword)
{
    TRACE("(%p)->(%p)\n", iface, pstrPassword);
    return Uri_GetPropertyBSTR(iface, Uri_PROPERTY_PASSWORD, pstrPassword, 0);
}

static HRESULT WINAPI Uri_GetPath(IUri *iface, BSTR *pstrPath)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrPath);

    if(!pstrPath)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPathAndQuery(IUri *iface, BSTR *pstrPathAndQuery)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrPathAndQuery);

    if(!pstrPathAndQuery)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetQuery(IUri *iface, BSTR *pstrQuery)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstrQuery);

    if(!pstrQuery)
        return E_POINTER;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetRawUri(IUri *iface, BSTR *pstrRawUri)
{
    Uri *This = URI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, pstrRawUri);

    /* Just forward the call to GetPropertyBSTR. */
    return Uri_GetPropertyBSTR(iface, Uri_PROPERTY_RAW_URI, pstrRawUri, 0);
}

static HRESULT WINAPI Uri_GetSchemeName(IUri *iface, BSTR *pstrSchemeName)
{
    Uri *This = URI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, pstrSchemeName);
    return Uri_GetPropertyBSTR(iface, Uri_PROPERTY_SCHEME_NAME, pstrSchemeName, 0);
}

static HRESULT WINAPI Uri_GetUserInfo(IUri *iface, BSTR *pstrUserInfo)
{
    TRACE("(%p)->(%p)\n", iface, pstrUserInfo);
    return Uri_GetPropertyBSTR(iface, Uri_PROPERTY_USER_INFO, pstrUserInfo, 0);
}

static HRESULT WINAPI Uri_GetUserName(IUri *iface, BSTR *pstrUserName)
{
    TRACE("(%p)->(%p)\n", iface, pstrUserName);
    return Uri_GetPropertyBSTR(iface, Uri_PROPERTY_USER_NAME, pstrUserName, 0);
}

static HRESULT WINAPI Uri_GetHostType(IUri *iface, DWORD *pdwHostType)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwHostType);

    if(!pdwHostType)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetPort(IUri *iface, DWORD *pdwPort)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwPort);

    if(!pdwPort)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetScheme(IUri *iface, DWORD *pdwScheme)
{
    Uri *This = URI_THIS(iface);
    TRACE("(%p)->(%p)\n", This, pdwScheme);
    return Uri_GetPropertyDWORD(iface, Uri_PROPERTY_SCHEME, pdwScheme, 0);
}

static HRESULT WINAPI Uri_GetZone(IUri *iface, DWORD *pdwZone)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwZone);

    if(!pdwZone)
        return E_INVALIDARG;

    /* Microsoft doesn't seem to have this implemented yet... See
     * the comment in Uri_GetPropertyDWORD for more about this.
     */
    *pdwZone = URLZONE_INVALID;
    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_GetProperties(IUri *iface, DWORD *pdwProperties)
{
    Uri *This = URI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwProperties);

    if(!pdwProperties)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI Uri_IsEqual(IUri *iface, IUri *pUri, BOOL *pfEqual)
{
    Uri *This = URI_THIS(iface);
    TRACE("(%p)->(%p %p)\n", This, pUri, pfEqual);

    if(!pfEqual)
        return E_POINTER;

    if(!pUri) {
        *pfEqual = FALSE;

        /* For some reason Windows returns S_OK here... */
        return S_OK;
    }

    FIXME("(%p)->(%p %p)\n", This, pUri, pfEqual);
    return E_NOTIMPL;
}

#undef URI_THIS

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

/***********************************************************************
 *           CreateUri (urlmon.@)
 */
HRESULT WINAPI CreateUri(LPCWSTR pwzURI, DWORD dwFlags, DWORD_PTR dwReserved, IUri **ppURI)
{
    Uri *ret;
    HRESULT hr;
    parse_data data;

    TRACE("(%s %x %x %p)\n", debugstr_w(pwzURI), dwFlags, (DWORD)dwReserved, ppURI);

    if(!ppURI)
        return E_INVALIDARG;

    if(!pwzURI) {
        *ppURI = NULL;
        return E_INVALIDARG;
    }

    ret = heap_alloc(sizeof(Uri));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->lpIUriVtbl = &UriVtbl;
    ret->ref = 1;

    /* Create a copy of pwzURI and store it as the raw_uri. */
    ret->raw_uri = SysAllocString(pwzURI);
    if(!ret->raw_uri) {
        heap_free(ret);
        return E_OUTOFMEMORY;
    }

    memset(&data, 0, sizeof(parse_data));
    data.uri = ret->raw_uri;

    /* Validate and parse the URI into it's components. */
    if(!parse_uri(&data, dwFlags)) {
        /* Encountered an unsupported or invalid URI */
        SysFreeString(ret->raw_uri);
        heap_free(ret);
        *ppURI = NULL;
        return E_INVALIDARG;
    }

    /* Canonicalize the URI. */
    hr = canonicalize_uri(&data, ret, dwFlags);
    if(FAILED(hr)) {
        SysFreeString(ret->raw_uri);
        heap_free(ret);
        *ppURI = NULL;
        return hr;
    }

    *ppURI = URI(ret);
    return S_OK;
}

#define URIBUILDER_THIS(iface) DEFINE_THIS(UriBuilder, IUriBuilder, iface)

static HRESULT WINAPI UriBuilder_QueryInterface(IUriBuilder *iface, REFIID riid, void **ppv)
{
    UriBuilder *This = URIBUILDER_THIS(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = URIBUILDER(This);
    }else if(IsEqualGUID(&IID_IUriBuilder, riid)) {
        TRACE("(%p)->(IID_IUri %p)\n", This, ppv);
        *ppv = URIBUILDER(This);
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
    UriBuilder *This = URIBUILDER_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI UriBuilder_Release(IUriBuilder *iface)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI UriBuilder_CreateUriSimple(IUriBuilder *iface,
                                                 DWORD        dwAllowEncodingPropertyMask,
                                                 DWORD_PTR    dwReserved,
                                                 IUri       **ppIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dwAllowEncodingPropertyMask, (DWORD)dwReserved, ppIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_CreateUri(IUriBuilder *iface,
                                           DWORD        dwCreateFlags,
                                           DWORD        dwAllowEncodingPropertyMask,
                                           DWORD_PTR    dwReserved,
                                           IUri       **ppIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(0x%08x %d %d %p)\n", This, dwCreateFlags, dwAllowEncodingPropertyMask, (DWORD)dwReserved, ppIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_CreateUriWithFlags(IUriBuilder *iface,
                                         DWORD        dwCreateFlags,
                                         DWORD        dwUriBuilderFlags,
                                         DWORD        dwAllowEncodingPropertyMask,
                                         DWORD_PTR    dwReserved,
                                         IUri       **ppIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(0x%08x 0x%08x %d %d %p)\n", This, dwCreateFlags, dwUriBuilderFlags,
        dwAllowEncodingPropertyMask, (DWORD)dwReserved, ppIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI  UriBuilder_GetIUri(IUriBuilder *iface, IUri **ppIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetIUri(IUriBuilder *iface, IUri *pIUri)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pIUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetFragment(IUriBuilder *iface, DWORD *pcchFragment, LPCWSTR *ppwzFragment)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchFragment, ppwzFragment);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetHost(IUriBuilder *iface, DWORD *pcchHost, LPCWSTR *ppwzHost)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchHost, ppwzHost);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetPassword(IUriBuilder *iface, DWORD *pcchPassword, LPCWSTR *ppwzPassword)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchPassword, ppwzPassword);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetPath(IUriBuilder *iface, DWORD *pcchPath, LPCWSTR *ppwzPath)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchPath, ppwzPath);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetPort(IUriBuilder *iface, BOOL *pfHasPort, DWORD *pdwPort)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pfHasPort, pdwPort);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetQuery(IUriBuilder *iface, DWORD *pcchQuery, LPCWSTR *ppwzQuery)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchQuery, ppwzQuery);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetSchemeName(IUriBuilder *iface, DWORD *pcchSchemeName, LPCWSTR *ppwzSchemeName)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchSchemeName, ppwzSchemeName);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_GetUserName(IUriBuilder *iface, DWORD *pcchUserName, LPCWSTR *ppwzUserName)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcchUserName, ppwzUserName);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetFragment(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetHost(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetPassword(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetPath(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetPort(IUriBuilder *iface, BOOL fHasPort, DWORD dwNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%d %d)\n", This, fHasPort, dwNewValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetQuery(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetSchemeName(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_SetUserName(IUriBuilder *iface, LPCWSTR pwzNewValue)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzNewValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_RemoveProperties(IUriBuilder *iface, DWORD dwPropertyMask)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(0x%08x)\n", This, dwPropertyMask);
    return E_NOTIMPL;
}

static HRESULT WINAPI UriBuilder_HasBeenModified(IUriBuilder *iface, BOOL *pfModified)
{
    UriBuilder *This = URIBUILDER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pfModified);
    return E_NOTIMPL;
}

#undef URIBUILDER_THIS

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

    TRACE("(%p %x %x %p)\n", pIUri, dwFlags, (DWORD)dwReserved, ppIUriBuilder);

    ret = heap_alloc(sizeof(UriBuilder));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->lpIUriBuilderVtbl = &UriBuilderVtbl;
    ret->ref = 1;

    *ppIUriBuilder = URIBUILDER(ret);
    return S_OK;
}
