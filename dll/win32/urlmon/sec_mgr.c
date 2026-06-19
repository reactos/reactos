/*
 * Internet Security and Zone Manager
 *
 * Copyright (c) 2004 Huw D M Davies
 * Copyright 2004 Jacek Caban
 * Copyright 2009 Detlef Riekenberg
 * Copyright 2011 Thomas Mullaly for CodeWeavers
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

#include <stdio.h>

#include "urlmon_main.h"
#include "winreg.h"
#include "wininet.h"

#define NO_SHLWAPI_REG
#include "shlwapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);
static const WCHAR wszZonesKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones\\";
static const WCHAR zone_map_keyW[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap";
static const WCHAR wszZoneMapDomainsKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\Domains";

static inline BOOL is_drive_path(const WCHAR *path)
{
    return iswalpha(*path) && *(path+1) == ':';
}

/* List of schemes types Windows seems to expect to be hierarchical. */
static inline BOOL is_hierarchical_scheme(URL_SCHEME type) {
    return(type == URL_SCHEME_HTTP || type == URL_SCHEME_FTP ||
           type == URL_SCHEME_GOPHER || type == URL_SCHEME_NNTP ||
           type == URL_SCHEME_TELNET || type == URL_SCHEME_WAIS ||
           type == URL_SCHEME_FILE || type == URL_SCHEME_HTTPS ||
           type == URL_SCHEME_RES);
}

/********************************************************************
 * get_string_from_reg [internal]
 *
 * helper to get a string from the reg.
 *
 */
static void get_string_from_reg(HKEY hcu, HKEY hklm, LPCWSTR name, LPWSTR out, DWORD maxlen)
{
    DWORD type = REG_SZ;
    DWORD len = maxlen * sizeof(WCHAR);
    DWORD res;

    res = RegQueryValueExW(hcu, name, NULL, &type, (LPBYTE) out, &len);

    if (res && hklm) {
        len = maxlen * sizeof(WCHAR);
        type = REG_SZ;
        res = RegQueryValueExW(hklm, name, NULL, &type, (LPBYTE) out, &len);
    }

    if (res) {
        TRACE("%s failed: %ld\n", debugstr_w(name), res);
        *out = '\0';
    }
}

/********************************************************************
 * get_dword_from_reg [internal]
 *
 * helper to get a dword from the reg.
 *
 */
static void get_dword_from_reg(HKEY hcu, HKEY hklm, LPCWSTR name, LPDWORD out)
{
    DWORD type = REG_DWORD;
    DWORD len = sizeof(DWORD);
    DWORD res;

    res = RegQueryValueExW(hcu, name, NULL, &type, (LPBYTE) out, &len);

    if (res && hklm) {
        len = sizeof(DWORD);
        type = REG_DWORD;
        res = RegQueryValueExW(hklm, name, NULL, &type, (LPBYTE) out, &len);
    }

    if (res) {
        TRACE("%s failed: %ld\n", debugstr_w(name), res);
        *out = 0;
    }
}

static HRESULT get_zone_from_reg(LPCWSTR schema, DWORD *zone)
{
    DWORD res, size;
    HKEY hkey;

    static const WCHAR wszZoneMapProtocolKey[] =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\ProtocolDefaults";

    res = RegOpenKeyW(HKEY_CURRENT_USER, wszZoneMapProtocolKey, &hkey);
    if(res != ERROR_SUCCESS) {
        ERR("Could not open key %s\n", debugstr_w(wszZoneMapProtocolKey));
        return E_UNEXPECTED;
    }

    size = sizeof(DWORD);
    res = RegQueryValueExW(hkey, schema, NULL, NULL, (PBYTE)zone, &size);
    RegCloseKey(hkey);
    if(res == ERROR_SUCCESS)
        return S_OK;

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, wszZoneMapProtocolKey, &hkey);
    if(res != ERROR_SUCCESS) {
        ERR("Could not open key %s\n", debugstr_w(wszZoneMapProtocolKey));
        return E_UNEXPECTED;
    }

    size = sizeof(DWORD);
    res = RegQueryValueExW(hkey, schema, NULL, NULL, (PBYTE)zone, &size);
    RegCloseKey(hkey);
    if(res == ERROR_SUCCESS)
        return S_OK;

    *zone = 3;
    return S_OK;
}

/********************************************************************
 * matches_domain_pattern [internal]
 *
 * Checks if the given string matches the specified domain pattern.
 *
 * This function looks for explicit wildcard domain components iff
 * they appear at the very beginning of the 'pattern' string
 *
 *  pattern = "*.google.com"
 */
static BOOL matches_domain_pattern(LPCWSTR pattern, LPCWSTR str, BOOL implicit_wildcard, LPCWSTR *matched)
{
    BOOL matches = FALSE;
    DWORD pattern_len = lstrlenW(pattern);
    DWORD str_len = lstrlenW(str);

    TRACE("(%d) Checking if %s matches %s\n", implicit_wildcard, debugstr_w(str), debugstr_w(pattern));

    *matched = NULL;
    if(str_len >= pattern_len) {
        /* Check if there's an explicit wildcard in the pattern. */
        if(pattern[0] == '*' && pattern[1] == '.') {
            /* Make sure that 'str' matches the wildcard pattern.
             *
             * Example:
             *  pattern = "*.google.com"
             *
             * So in this case 'str' would have to end with ".google.com" in order
             * to map to this pattern.
             */
            if(str_len >= pattern_len+1 && !wcsicmp(str+(str_len-pattern_len+1), pattern+1)) {
                /* Check if there's another '.' inside of the "unmatched" portion
                 * of 'str'.
                 *
                 * Example:
                 *  pattern = "*.google.com"
                 *  str     = "test.testing.google.com"
                 *
                 * The currently matched portion is ".google.com" in 'str', we need
                 * see if there's a '.' inside of the unmatched portion ("test.testing"), because
                 * if there is and 'implicit_wildcard' isn't set, then this isn't
                 * a match.
                 */
                const WCHAR *ptr;
                for (ptr = str + str_len - pattern_len; ptr > str; ptr--) if (ptr[-1] == '.') break;
                if (ptr == str || implicit_wildcard) {
                    matches = TRUE;
                    *matched = ptr;
                }
            }
        } else if(implicit_wildcard && str_len > pattern_len) {
            /* When the pattern has an implicit wildcard component, it means
             * that anything goes in 'str' as long as it ends with the pattern
             * and that the beginning of the match has a '.' before it.
             *
             * Example:
             *  pattern = "google.com"
             *  str     = "www.google.com"
             *
             * Implicitly matches the pattern, where as:
             *
             *  pattern = "google.com"
             *  str     = "wwwgoogle.com"
             *
             * Doesn't match the pattern.
             */
            if(str[str_len-pattern_len-1] == '.' && !wcsicmp(str+(str_len-pattern_len), pattern)) {
                matches = TRUE;
                *matched = str+(str_len-pattern_len);
            }
        } else {
            /* The pattern doesn't have an implicit wildcard, or an explicit wildcard,
             * so 'str' has to be an exact match to the 'pattern'.
             */
            if(!wcsicmp(str, pattern)) {
                matches = TRUE;
                *matched = str;
            }
        }
    }

    if(matches)
        TRACE("Found a match: matched=%s\n", debugstr_w(*matched));
    else
        TRACE("No match found\n");

    return matches;
}

static BOOL get_zone_for_scheme(HKEY key, LPCWSTR schema, DWORD *zone)
{
    DWORD res;
    DWORD size = sizeof(DWORD);
    DWORD type;

    /* See if the key contains a value for the scheme first. */
    res = RegQueryValueExW(key, schema, NULL, &type, (BYTE*)zone, &size);
    if(res == ERROR_SUCCESS) {
        if(type == REG_DWORD)
            return TRUE;
        WARN("Unexpected value type %ld for value %s, expected REG_DWORD\n", type, debugstr_w(schema));
    }

    /* Try to get the zone for the wildcard scheme. */
    size = sizeof(DWORD);
    res = RegQueryValueExW(key, L"*", NULL, &type, (BYTE*)zone, &size);
    if(res != ERROR_SUCCESS)
        return FALSE;

    if(type != REG_DWORD) {
        WARN("Unexpected value type %ld for value %s, expected REG_DWORD\n", type, debugstr_w(L"*"));
        return FALSE;
    }

    return TRUE;
}

/********************************************************************
 * search_domain_for_zone [internal]
 *
 * Searches the specified 'domain' registry key to see if 'host' maps into it, or any
 * of its subdomain registry keys.
 *
 * Returns S_OK if a match is found, S_FALSE if no matches were found, or an error code.
 */
static HRESULT search_domain_for_zone(HKEY domains, LPCWSTR domain, DWORD domain_len, LPCWSTR schema,
                                      LPCWSTR host, DWORD host_len, DWORD *zone)
{
    BOOL found = FALSE;
    HKEY domain_key;
    DWORD res;
    LPCWSTR matched;

    if(host_len >= domain_len && matches_domain_pattern(domain, host, TRUE, &matched)) {
        res = RegOpenKeyW(domains, domain, &domain_key);
        if(res != ERROR_SUCCESS) {
            ERR("Failed to open domain key %s: %ld\n", debugstr_w(domain), res);
            return E_UNEXPECTED;
        }

        if(matched == host)
            found = get_zone_for_scheme(domain_key, schema, zone);
        else {
            INT domain_offset;
            DWORD subdomain_count, subdomain_len;
            BOOL check_domain = TRUE;

            find_domain_name(domain, domain_len, &domain_offset);

            res = RegQueryInfoKeyW(domain_key, NULL, NULL, NULL, &subdomain_count, &subdomain_len,
                                   NULL, NULL, NULL, NULL, NULL, NULL);
            if(res != ERROR_SUCCESS) {
                ERR("Unable to query info for key %s: %ld\n", debugstr_w(domain), res);
                RegCloseKey(domain_key);
                return E_UNEXPECTED;
            }

            if(subdomain_count) {
                WCHAR *subdomain;
                WCHAR *component;
                DWORD i;

                subdomain = malloc((subdomain_len + 1) * sizeof(WCHAR));
                if(!subdomain) {
                    RegCloseKey(domain_key);
                    return E_OUTOFMEMORY;
                }

                component = strndupW(host, matched-host - 1);
                if(!component) {
                    free(subdomain);
                    RegCloseKey(domain_key);
                    return E_OUTOFMEMORY;
                }

                for(i = 0; i < subdomain_count; ++i) {
                    DWORD len = subdomain_len+1;
                    const WCHAR *sub_matched;

                    res = RegEnumKeyExW(domain_key, i, subdomain, &len, NULL, NULL, NULL, NULL);
                    if(res != ERROR_SUCCESS) {
                        free(component);
                        free(subdomain);
                        RegCloseKey(domain_key);
                        return E_UNEXPECTED;
                    }

                    if(matches_domain_pattern(subdomain, component, FALSE, &sub_matched)) {
                        HKEY subdomain_key;

                        res = RegOpenKeyW(domain_key, subdomain, &subdomain_key);
                        if(res != ERROR_SUCCESS) {
                            ERR("Unable to open subdomain key %s of %s: %ld\n", debugstr_w(subdomain),
                                debugstr_w(domain), res);
                            free(component);
                            free(subdomain);
                            RegCloseKey(domain_key);
                            return E_UNEXPECTED;
                        }

                        found = get_zone_for_scheme(subdomain_key, schema, zone);
                        check_domain = FALSE;
                        RegCloseKey(subdomain_key);
                        break;
                    }
                }
                free(subdomain);
                free(component);
            }

            /* There's a chance that 'host' implicitly mapped into 'domain', in
             * which case we check to see if 'domain' contains zone information.
             *
             * This can only happen if 'domain' is its own domain name.
             *  Example:
             *      "google.com" (domain name = "google.com")
             *
             *  So if:
             *      host = "www.google.com"
             *
             *  Then host would map directly into the "google.com" domain key.
             *
             * If 'domain' has more than just its domain name, or it does not
             * have a domain name, then we don't perform the check. The reason
             * for this is that these domains don't allow implicit mappings.
             *  Example:
             *      domain = "org" (has no domain name)
             *      host   = "www.org"
             *
             *  The mapping would only happen if the "org" key had an explicit subkey
             *  called "www".
             */
            if(check_domain && !domain_offset && !wcschr(host, matched-host-1))
                found = get_zone_for_scheme(domain_key, schema, zone);
        }
        RegCloseKey(domain_key);
    }

    return found ? S_OK : S_FALSE;
}

static HRESULT search_for_domain_mapping(HKEY domains, LPCWSTR schema, LPCWSTR host, DWORD host_len, DWORD *zone)
{
    WCHAR *domain;
    DWORD domain_count, domain_len, i;
    DWORD res;
    HRESULT hres = S_FALSE;

    res = RegQueryInfoKeyW(domains, NULL, NULL, NULL, &domain_count, &domain_len,
                           NULL, NULL, NULL, NULL, NULL, NULL);
    if(res != ERROR_SUCCESS) {
        WARN("Failed to retrieve information about key\n");
        return E_UNEXPECTED;
    }

    if(!domain_count)
        return S_FALSE;

    domain = malloc((domain_len + 1) * sizeof(WCHAR));
    if(!domain)
        return E_OUTOFMEMORY;

    for(i = 0; i < domain_count; ++i) {
        DWORD len = domain_len+1;

        res = RegEnumKeyExW(domains, i, domain, &len, NULL, NULL, NULL, NULL);
        if(res != ERROR_SUCCESS) {
            free(domain);
            return E_UNEXPECTED;
        }

        hres = search_domain_for_zone(domains, domain, len, schema, host, host_len, zone);
        if(FAILED(hres) || hres == S_OK)
            break;
    }

    free(domain);
    return hres;
}

static HRESULT get_zone_from_domains(IUri *uri, DWORD *zone)
{
    HRESULT hres;
    BSTR host, scheme;
    DWORD res;
    HKEY domains;
    DWORD scheme_type;

    hres = IUri_GetScheme(uri, &scheme_type);
    if(FAILED(hres))
        return hres;

    /* Windows doesn't play nice with unknown scheme types when it tries
     * to check if a host name maps into any domains.
     */
    if(scheme_type == URL_SCHEME_UNKNOWN)
        return S_FALSE;

    hres = IUri_GetHost(uri, &host);
    if(FAILED(hres))
        return hres;

    /* Known hierarchical scheme types must have a host. If they don't Windows
     * assigns URLZONE_INVALID to the zone.
     */
    if((scheme_type != URL_SCHEME_UNKNOWN && scheme_type != URL_SCHEME_FILE)
        && is_hierarchical_scheme(scheme_type) && !*host) {
        *zone = URLZONE_INVALID;

        SysFreeString(host);

        /* The MapUrlToZone functions return S_OK when this condition occurs. */
        return S_OK;
    }

    hres = IUri_GetSchemeName(uri, &scheme);
    if(FAILED(hres)) {
        SysFreeString(host);
        return hres;
    }

    /* First try CURRENT_USER. */
    res = RegOpenKeyW(HKEY_CURRENT_USER, wszZoneMapDomainsKey, &domains);
    if(res == ERROR_SUCCESS) {
        hres = search_for_domain_mapping(domains, scheme, host, SysStringLen(host), zone);
        RegCloseKey(domains);
    } else
        WARN("Failed to open HKCU's %s key\n", debugstr_w(wszZoneMapDomainsKey));

    /* If that doesn't work try LOCAL_MACHINE. */
    if(hres == S_FALSE) {
        res = RegOpenKeyW(HKEY_LOCAL_MACHINE, wszZoneMapDomainsKey, &domains);
        if(res == ERROR_SUCCESS) {
            hres = search_for_domain_mapping(domains, scheme, host, SysStringLen(host), zone);
            RegCloseKey(domains);
        } else
            WARN("Failed to open HKLM's %s key\n", debugstr_w(wszZoneMapDomainsKey));
    }

    SysFreeString(host);
    SysFreeString(scheme);
    return hres;
}

static HRESULT map_security_uri_to_zone(IUri *uri, DWORD *zone)
{
    HRESULT hres;
    BSTR scheme;

    *zone = URLZONE_INVALID;

    hres = IUri_GetSchemeName(uri, &scheme);
    if(FAILED(hres))
        return hres;

    if(!wcsicmp(scheme, L"file")) {
        BSTR path;
        WCHAR *ptr, *path_start, root[20];

        hres = IUri_GetPath(uri, &path);
        if(FAILED(hres)) {
            SysFreeString(scheme);
            return hres;
        }

        if(*path == '/' && is_drive_path(path+1))
            path_start = path+1;
        else
            path_start = path;

        if((ptr = wcschr(path_start, ':')) && ptr-path_start+1 < ARRAY_SIZE(root)) {
            UINT type;

            memcpy(root, path_start, (ptr-path_start+1)*sizeof(WCHAR));
            root[ptr-path_start+1] = 0;

            type = GetDriveTypeW(root);

            switch(type) {
            case DRIVE_UNKNOWN:
            case DRIVE_NO_ROOT_DIR:
                break;
            case DRIVE_REMOVABLE:
            case DRIVE_FIXED:
            case DRIVE_CDROM:
            case DRIVE_RAMDISK:
                *zone = URLZONE_LOCAL_MACHINE;
                hres = S_OK;
                break;
            case DRIVE_REMOTE:
                *zone = URLZONE_INTERNET;
                hres = S_OK;
                break;
            default:
                FIXME("unsupported drive type %d\n", type);
            }
        }
        SysFreeString(path);
    }

    if(*zone == URLZONE_INVALID) {
        hres = get_zone_from_domains(uri, zone);
        if(hres == S_FALSE)
            hres = get_zone_from_reg(scheme, zone);
    }

    SysFreeString(scheme);
    return hres;
}

static HRESULT map_url_to_zone(LPCWSTR url, DWORD *zone, LPWSTR *ret_url)
{
    IUri *secur_uri;
    LPWSTR secur_url;
    HRESULT hres;

    *zone = URLZONE_INVALID;

    hres = CoInternetGetSecurityUrl(url, &secur_url, PSU_SECURITY_URL_ONLY, 0);
    if(hres != S_OK) {
        DWORD size = lstrlenW(url)*sizeof(WCHAR);

        secur_url = CoTaskMemAlloc(size);
        if(!secur_url)
            return E_OUTOFMEMORY;

        memcpy(secur_url, url, size);
    }

    hres = CreateUri(secur_url, Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, &secur_uri);
    if(FAILED(hres)) {
        CoTaskMemFree(secur_url);
        return hres;
    }

    hres = map_security_uri_to_zone(secur_uri, zone);
    IUri_Release(secur_uri);

    if(FAILED(hres) || !ret_url)
        CoTaskMemFree(secur_url);
    else
        *ret_url = secur_url;

    return hres;
}

static HRESULT map_uri_to_zone(IUri *uri, DWORD *zone, IUri **ret_uri)
{
    HRESULT hres;
    IUri *secur_uri;

    hres = CoInternetGetSecurityUrlEx(uri, &secur_uri, PSU_SECURITY_URL_ONLY, 0);
    if(FAILED(hres))
        return hres;

    hres = map_security_uri_to_zone(secur_uri, zone);
    if(FAILED(hres) || !ret_uri)
        IUri_Release(secur_uri);
    else
        *ret_uri = secur_uri;

    return hres;
}

static HRESULT open_zone_key(HKEY parent_key, DWORD zone, HKEY *hkey)
{
    WCHAR key_name[ARRAY_SIZE(wszZonesKey) + 12];
    DWORD res;

    wsprintfW(key_name, L"%s%u", wszZonesKey, zone);

    res = RegOpenKeyW(parent_key, key_name, hkey);

    if(res != ERROR_SUCCESS) {
        WARN("RegOpenKey failed\n");
        return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT get_action_policy(DWORD zone, DWORD action, BYTE *policy, DWORD size, URLZONEREG zone_reg)
{
    HKEY parent_key;
    HKEY hkey;
    LONG res;
    HRESULT hres;

    switch(action) {
    case URLACTION_SCRIPT_OVERRIDE_SAFETY:
    case URLACTION_ACTIVEX_OVERRIDE_SCRIPT_SAFETY:
        *(DWORD*)policy = URLPOLICY_DISALLOW;
        return S_OK;
    }

    switch(zone_reg) {
    case URLZONEREG_DEFAULT:
    case URLZONEREG_HKCU:
        parent_key = HKEY_CURRENT_USER;
        break;
    case URLZONEREG_HKLM:
        parent_key = HKEY_LOCAL_MACHINE;
        break;
    default:
        WARN("Unknown URLZONEREG: %d\n", zone_reg);
        return E_FAIL;
    };

    hres = open_zone_key(parent_key, zone, &hkey);
    if(SUCCEEDED(hres)) {
        WCHAR action_str[16];
        DWORD len = size;

        wsprintfW(action_str, L"%X", action);

        res = RegQueryValueExW(hkey, action_str, NULL, NULL, policy, &len);
        if(res == ERROR_MORE_DATA) {
            hres = E_INVALIDARG;
        }else if(res == ERROR_FILE_NOT_FOUND) {
            hres = E_FAIL;
        }else if(res != ERROR_SUCCESS) {
            ERR("RegQueryValue failed: %ld\n", res);
            hres = E_UNEXPECTED;
        }

        RegCloseKey(hkey);
    }

    if(FAILED(hres) && zone_reg == URLZONEREG_DEFAULT)
        return get_action_policy(zone, action, policy, size, URLZONEREG_HKLM);

    return hres;
}

static HRESULT generate_security_id(IUri *uri, BYTE *secid, DWORD *secid_len, DWORD zone)
{
    DWORD len;
    HRESULT hres;
    DWORD scheme_type;

    if(zone == URLZONE_INVALID)
        return E_INVALIDARG;

    hres = IUri_GetScheme(uri, &scheme_type);
    if(FAILED(hres))
        return hres;

    /* Windows handles opaque URLs differently then hierarchical ones. */
    if(!is_hierarchical_scheme(scheme_type) && scheme_type != URL_SCHEME_WILDCARD) {
        BSTR display_uri;

        hres = IUri_GetDisplayUri(uri, &display_uri);
        if(FAILED(hres))
            return hres;

        len = WideCharToMultiByte(CP_ACP, 0, display_uri, -1, NULL, 0, NULL, NULL)-1;

        if(len+sizeof(DWORD) > *secid_len) {
            SysFreeString(display_uri);
            return E_NOT_SUFFICIENT_BUFFER;
        }

        WideCharToMultiByte(CP_ACP, 0, display_uri, -1, (LPSTR)secid, len, NULL, NULL);
        SysFreeString(display_uri);

        *(DWORD*)(secid+len) = zone;
    } else {
        BSTR host, scheme;
        DWORD host_len, scheme_len;
        BYTE *ptr;

        hres = IUri_GetHost(uri, &host);
        if(FAILED(hres))
            return hres;

        /* The host can't be empty for Wildcard URIs. */
        if(scheme_type == URL_SCHEME_WILDCARD && !*host) {
            SysFreeString(host);
            return E_INVALIDARG;
        }

        hres = IUri_GetSchemeName(uri, &scheme);
        if(FAILED(hres)) {
            SysFreeString(host);
            return hres;
        }

        host_len = WideCharToMultiByte(CP_ACP, 0, host, -1, NULL, 0, NULL, NULL)-1;
        scheme_len = WideCharToMultiByte(CP_ACP, 0, scheme, -1, NULL, 0, NULL, NULL)-1;

        len = host_len+scheme_len+sizeof(BYTE);

        if(len+sizeof(DWORD) > *secid_len) {
            SysFreeString(host);
            SysFreeString(scheme);
            return E_NOT_SUFFICIENT_BUFFER;
        }

        WideCharToMultiByte(CP_ACP, 0, scheme, -1, (LPSTR)secid, len, NULL, NULL);
        SysFreeString(scheme);

        ptr = secid+scheme_len;
        *ptr++ = ':';

        WideCharToMultiByte(CP_ACP, 0, host, -1, (LPSTR)ptr, host_len, NULL, NULL);
        SysFreeString(host);

        ptr += host_len;

        *(DWORD*)ptr = zone;
    }

    *secid_len = len+sizeof(DWORD);

    return S_OK;
}

static HRESULT get_security_id_for_url(LPCWSTR url, BYTE *secid, DWORD *secid_len)
{
    HRESULT hres;
    DWORD zone = URLZONE_INVALID;
    LPWSTR secur_url = NULL;
    IUri *uri;

    hres = map_url_to_zone(url, &zone, &secur_url);
    if(FAILED(hres))
        return hres == 0x80041001 ? E_INVALIDARG : hres;

    hres = CreateUri(secur_url, Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, &uri);
    CoTaskMemFree(secur_url);
    if(FAILED(hres))
        return hres;

    hres = generate_security_id(uri, secid, secid_len, zone);
    IUri_Release(uri);

    return hres;
}

static HRESULT get_security_id_for_uri(IUri *uri, BYTE *secid, DWORD *secid_len)
{
    HRESULT hres;
    IUri *secur_uri;
    DWORD zone = URLZONE_INVALID;

    hres = map_uri_to_zone(uri, &zone, &secur_uri);
    if(FAILED(hres))
        return hres;

    hres = generate_security_id(secur_uri, secid, secid_len, zone);
    IUri_Release(secur_uri);

    return hres;
}

/***********************************************************************
 *           InternetSecurityManager implementation
 *
 */
typedef struct {
    IInternetSecurityManagerEx2 IInternetSecurityManagerEx2_iface;

    LONG ref;

    IInternetSecurityMgrSite *mgrsite;
    IInternetSecurityManager *custom_manager;
} SecManagerImpl;

static inline SecManagerImpl *impl_from_IInternetSecurityManagerEx2(IInternetSecurityManagerEx2 *iface)
{
    return CONTAINING_RECORD(iface, SecManagerImpl, IInternetSecurityManagerEx2_iface);
}

static HRESULT WINAPI SecManagerImpl_QueryInterface(IInternetSecurityManagerEx2* iface,REFIID riid,void** ppvObject)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);

    TRACE("(%p)->(%s %p)\n",This,debugstr_guid(riid),ppvObject);

    if(!ppvObject)
	return E_INVALIDARG;

    if(IsEqualIID(&IID_IUnknown, riid) ||
       IsEqualIID(&IID_IInternetSecurityManager, riid) ||
       IsEqualIID(&IID_IInternetSecurityManagerEx, riid) ||
       IsEqualIID(&IID_IInternetSecurityManagerEx2, riid)) {
        *ppvObject = iface;
    } else {
        WARN("not supported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IInternetSecurityManagerEx2_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI SecManagerImpl_AddRef(IInternetSecurityManagerEx2* iface)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, refCount);

    return refCount;
}

static ULONG WINAPI SecManagerImpl_Release(IInternetSecurityManagerEx2* iface)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, refCount);

    /* destroy the object if there are no more references on it */
    if (!refCount){
        if(This->mgrsite)
            IInternetSecurityMgrSite_Release(This->mgrsite);
        if(This->custom_manager)
            IInternetSecurityManager_Release(This->custom_manager);

        free(This);

        URLMON_UnlockModule();
    }

    return refCount;
}

static HRESULT WINAPI SecManagerImpl_SetSecuritySite(IInternetSecurityManagerEx2 *iface,
                                                     IInternetSecurityMgrSite *pSite)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);

    TRACE("(%p)->(%p)\n", This, pSite);

    if(This->mgrsite)
        IInternetSecurityMgrSite_Release(This->mgrsite);

    if(This->custom_manager) {
        IInternetSecurityManager_Release(This->custom_manager);
        This->custom_manager = NULL;
    }

    This->mgrsite = pSite;

    if(pSite) {
        IServiceProvider *servprov;
        HRESULT hres;

        IInternetSecurityMgrSite_AddRef(pSite);

        hres = IInternetSecurityMgrSite_QueryInterface(pSite, &IID_IServiceProvider,
                (void**)&servprov);
        if(SUCCEEDED(hres)) {
            IServiceProvider_QueryService(servprov, &SID_SInternetSecurityManager,
                    &IID_IInternetSecurityManager, (void**)&This->custom_manager);
            IServiceProvider_Release(servprov);
        }
    }

    return S_OK;
}

static HRESULT WINAPI SecManagerImpl_GetSecuritySite(IInternetSecurityManagerEx2 *iface,
                                                     IInternetSecurityMgrSite **ppSite)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);

    TRACE("(%p)->(%p)\n", This, ppSite);

    if(!ppSite)
        return E_INVALIDARG;

    if(This->mgrsite)
        IInternetSecurityMgrSite_AddRef(This->mgrsite);

    *ppSite = This->mgrsite;
    return S_OK;
}

static HRESULT WINAPI SecManagerImpl_MapUrlToZone(IInternetSecurityManagerEx2 *iface,
                                                  LPCWSTR pwszUrl, DWORD *pdwZone,
                                                  DWORD dwFlags)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);
    HRESULT hres;

    TRACE("(%p)->(%s %p %08lx)\n", iface, debugstr_w(pwszUrl), pdwZone, dwFlags);

    if(This->custom_manager) {
        hres = IInternetSecurityManager_MapUrlToZone(This->custom_manager,
                pwszUrl, pdwZone, dwFlags);
        if(hres != INET_E_DEFAULT_ACTION)
            return hres;
    }

    if(!pwszUrl) {
        *pdwZone = URLZONE_INVALID;
        return E_INVALIDARG;
    }

    if(dwFlags)
        FIXME("not supported flags: %08lx\n", dwFlags);

    return map_url_to_zone(pwszUrl, pdwZone, NULL);
}

static HRESULT WINAPI SecManagerImpl_GetSecurityId(IInternetSecurityManagerEx2 *iface,
        LPCWSTR pwszUrl, BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);

    TRACE("(%p)->(%s %p %p %08Ix)\n", iface, debugstr_w(pwszUrl), pbSecurityId,
          pcbSecurityId, dwReserved);

    if(This->custom_manager) {
        HRESULT hres;

        hres = IInternetSecurityManager_GetSecurityId(This->custom_manager,
                pwszUrl, pbSecurityId, pcbSecurityId, dwReserved);
        if(hres != INET_E_DEFAULT_ACTION)
            return hres;
    }

    if(!pwszUrl || !pbSecurityId || !pcbSecurityId)
        return E_INVALIDARG;

    if(dwReserved)
        FIXME("dwReserved is not supported\n");

    return get_security_id_for_url(pwszUrl, pbSecurityId, pcbSecurityId);
}


static HRESULT WINAPI SecManagerImpl_ProcessUrlAction(IInternetSecurityManagerEx2 *iface,
                                                      LPCWSTR pwszUrl, DWORD dwAction,
                                                      BYTE *pPolicy, DWORD cbPolicy,
                                                      BYTE *pContext, DWORD cbContext,
                                                      DWORD dwFlags, DWORD dwReserved)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);
    DWORD zone, policy;
    HRESULT hres;

    TRACE("(%p)->(%s %08lx %p %08lx %p %08lx %08lx %08lx)\n", iface, debugstr_w(pwszUrl), dwAction,
          pPolicy, cbPolicy, pContext, cbContext, dwFlags, dwReserved);

    if(This->custom_manager) {
        hres = IInternetSecurityManager_ProcessUrlAction(This->custom_manager, pwszUrl, dwAction,
                pPolicy, cbPolicy, pContext, cbContext, dwFlags, dwReserved);
        if(hres != INET_E_DEFAULT_ACTION)
            return hres;
    }

    if(dwFlags || dwReserved)
        FIXME("Unsupported arguments\n");

    if(!pwszUrl)
        return E_INVALIDARG;

    hres = map_url_to_zone(pwszUrl, &zone, NULL);
    if(FAILED(hres))
        return hres;

    hres = get_action_policy(zone, dwAction, (BYTE*)&policy, sizeof(policy), URLZONEREG_DEFAULT);
    if(FAILED(hres))
        return hres;

    TRACE("policy %lx\n", policy);
    if(cbPolicy >= sizeof(DWORD))
        *(DWORD*)pPolicy = policy;

    switch(GetUrlPolicyPermissions(policy)) {
    case URLPOLICY_ALLOW:
    case URLPOLICY_CHANNEL_SOFTDIST_PRECACHE:
        return S_OK;
    case URLPOLICY_DISALLOW:
        return S_FALSE;
    case URLPOLICY_QUERY:
        FIXME("URLPOLICY_QUERY not implemented\n");
        return E_FAIL;
    default:
        FIXME("Not implemented policy %lx\n", policy);
    }

    return E_FAIL;
}
                                               

static HRESULT WINAPI SecManagerImpl_QueryCustomPolicy(IInternetSecurityManagerEx2 *iface,
                                                       LPCWSTR pwszUrl, REFGUID guidKey,
                                                       BYTE **ppPolicy, DWORD *pcbPolicy,
                                                       BYTE *pContext, DWORD cbContext,
                                                       DWORD dwReserved)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);
    HRESULT hres;

    TRACE("(%p)->(%s %s %p %p %p %08lx %08lx )\n", iface, debugstr_w(pwszUrl), debugstr_guid(guidKey),
          ppPolicy, pcbPolicy, pContext, cbContext, dwReserved);

    if(This->custom_manager) {
        hres = IInternetSecurityManager_QueryCustomPolicy(This->custom_manager, pwszUrl, guidKey,
                ppPolicy, pcbPolicy, pContext, cbContext, dwReserved);
        if(hres != INET_E_DEFAULT_ACTION)
            return hres;
    }

    WARN("Unknown guidKey %s\n", debugstr_guid(guidKey));
    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

static HRESULT WINAPI SecManagerImpl_SetZoneMapping(IInternetSecurityManagerEx2 *iface,
                                                    DWORD dwZone, LPCWSTR pwszPattern, DWORD dwFlags)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);
    HRESULT hres;

    TRACE("(%p)->(%08lx %s %08lx)\n", iface, dwZone, debugstr_w(pwszPattern),dwFlags);

    if(This->custom_manager) {
        hres = IInternetSecurityManager_SetZoneMapping(This->custom_manager, dwZone,
                pwszPattern, dwFlags);
        if(hres != INET_E_DEFAULT_ACTION)
            return hres;
    }

    FIXME("Default action is not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SecManagerImpl_GetZoneMappings(IInternetSecurityManagerEx2 *iface,
        DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);
    HRESULT hres;

    TRACE("(%p)->(%08lx %p %08lx)\n", iface, dwZone, ppenumString,dwFlags);

    if(This->custom_manager) {
        hres = IInternetSecurityManager_GetZoneMappings(This->custom_manager, dwZone,
                ppenumString, dwFlags);
        if(hres != INET_E_DEFAULT_ACTION)
            return hres;
    }

    FIXME("Default action is not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI SecManagerImpl_ProcessUrlActionEx(IInternetSecurityManagerEx2 *iface,
        LPCWSTR pwszUrl, DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext,
        DWORD dwFlags, DWORD dwReserved, DWORD *pdwOutFlags)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);
    FIXME("(%p)->(%s %08lx %p %ld %p %ld %08lx %08lx %p) stub\n", This, debugstr_w(pwszUrl), dwAction, pPolicy, cbPolicy,
          pContext, cbContext, dwFlags, dwReserved, pdwOutFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI SecManagerImpl_MapUrlToZoneEx2(IInternetSecurityManagerEx2 *iface,
        IUri *pUri, DWORD *pdwZone, DWORD dwFlags, LPWSTR *ppwszMappedUrl, DWORD *pdwOutFlags)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);

    TRACE("(%p)->(%p %p %08lx %p %p)\n", This, pUri, pdwZone, dwFlags, ppwszMappedUrl, pdwOutFlags);

    if(This->custom_manager) {
        HRESULT hres;
        IInternetSecurityManagerEx2 *sec_mgr2;

        hres = IInternetSecurityManager_QueryInterface(This->custom_manager, &IID_IInternetSecurityManagerEx2,
                (void**)&sec_mgr2);
        if(SUCCEEDED(hres)) {
            hres = IInternetSecurityManagerEx2_MapUrlToZoneEx2(sec_mgr2, pUri, pdwZone, dwFlags, ppwszMappedUrl, pdwOutFlags);
            IInternetSecurityManagerEx2_Release(sec_mgr2);
        } else {
            BSTR url;

            hres = IUri_GetDisplayUri(pUri, &url);
            if(FAILED(hres))
                return hres;

            hres = IInternetSecurityManager_MapUrlToZone(This->custom_manager, url, pdwZone, dwFlags);
            SysFreeString(url);
        }

        if(hres != INET_E_DEFAULT_ACTION)
            return hres;
    }

    if(!pdwZone)
        return E_INVALIDARG;

    if(!pUri) {
        *pdwZone = URLZONE_INVALID;
        return E_INVALIDARG;
    }

    if(dwFlags)
        FIXME("Unsupported flags: %08lx\n", dwFlags);

    return map_uri_to_zone(pUri, pdwZone, NULL);
}

static HRESULT WINAPI SecManagerImpl_ProcessUrlActionEx2(IInternetSecurityManagerEx2 *iface,
        IUri *pUri, DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext,
        DWORD dwFlags, DWORD_PTR dwReserved, DWORD *pdwOutFlags)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);
    FIXME("(%p)->(%p %08lx %p %ld %p %ld %08lx %08Ix %p) stub\n", This, pUri, dwAction, pPolicy,
          cbPolicy, pContext, cbContext, dwFlags, dwReserved, pdwOutFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI SecManagerImpl_GetSecurityIdEx2(IInternetSecurityManagerEx2 *iface,
        IUri *pUri, BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);
    TRACE("(%p)->(%p %p %p %08Ix) stub\n", This, pUri, pbSecurityId, pcbSecurityId, dwReserved);

    if(dwReserved)
        FIXME("dwReserved is not supported yet\n");

    if(!pUri || !pcbSecurityId || !pbSecurityId)
        return E_INVALIDARG;

    return get_security_id_for_uri(pUri, pbSecurityId, pcbSecurityId);
}

static HRESULT WINAPI SecManagerImpl_QueryCustomPolicyEx2(IInternetSecurityManagerEx2 *iface,
        IUri *pUri, REFGUID guidKey, BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext,
        DWORD cbContext, DWORD_PTR dwReserved)
{
    SecManagerImpl *This = impl_from_IInternetSecurityManagerEx2(iface);
    FIXME("(%p)->(%p %s %p %p %p %ld %08Ix) stub\n", This, pUri, debugstr_guid(guidKey), ppPolicy, pcbPolicy,
          pContext, cbContext, dwReserved);
    return E_NOTIMPL;
}

static const IInternetSecurityManagerEx2Vtbl VT_SecManagerImpl =
{
    SecManagerImpl_QueryInterface,
    SecManagerImpl_AddRef,
    SecManagerImpl_Release,
    SecManagerImpl_SetSecuritySite,
    SecManagerImpl_GetSecuritySite,
    SecManagerImpl_MapUrlToZone,
    SecManagerImpl_GetSecurityId,
    SecManagerImpl_ProcessUrlAction,
    SecManagerImpl_QueryCustomPolicy,
    SecManagerImpl_SetZoneMapping,
    SecManagerImpl_GetZoneMappings,
    SecManagerImpl_ProcessUrlActionEx,
    SecManagerImpl_MapUrlToZoneEx2,
    SecManagerImpl_ProcessUrlActionEx2,
    SecManagerImpl_GetSecurityIdEx2,
    SecManagerImpl_QueryCustomPolicyEx2
};

HRESULT SecManagerImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    SecManagerImpl *This;

    TRACE("(%p,%p)\n",pUnkOuter,ppobj);
    This = malloc(sizeof(*This));

    /* Initialize the virtual function table. */
    This->IInternetSecurityManagerEx2_iface.lpVtbl = &VT_SecManagerImpl;

    This->ref = 1;
    This->mgrsite = NULL;
    This->custom_manager = NULL;

    *ppobj = This;

    URLMON_LockModule();

    return S_OK;
}

/***********************************************************************
 *           InternetZoneManager implementation
 *
 */
typedef struct {
    IInternetZoneManagerEx2 IInternetZoneManagerEx2_iface;
    LONG ref;
    LPDWORD *zonemaps;
    DWORD zonemap_count;
} ZoneMgrImpl;

static inline ZoneMgrImpl *impl_from_IInternetZoneManagerEx2(IInternetZoneManagerEx2 *iface)
{
    return CONTAINING_RECORD(iface, ZoneMgrImpl, IInternetZoneManagerEx2_iface);
}


/***********************************************************************
 * build_zonemap_from_reg [internal]
 *
 * Enumerate the Zones in the Registry and return the Zones in a DWORD-array
 * The number of the Zones is returned in data[0]
 */
static LPDWORD build_zonemap_from_reg(void)
{
    WCHAR name[32];
    HKEY hkey;
    LPDWORD data = NULL;
    DWORD allocated = 6; /* space for the zonecount and Zone "0" up to Zone "4" */
    DWORD used = 0;
    DWORD res;
    DWORD len;


    res = RegOpenKeyW(HKEY_CURRENT_USER, wszZonesKey, &hkey);
    if (res)
        return NULL;

    data = malloc(allocated * sizeof(DWORD));
    if (!data)
        goto cleanup;

    while (!res) {
        name[0] = '\0';
        len = ARRAY_SIZE(name);
        res = RegEnumKeyExW(hkey, used, name, &len, NULL, NULL, NULL, NULL);

        if (!res) {
            used++;
            if (used == allocated) {
                LPDWORD new_data;

                allocated *= 2;
                new_data = realloc(data, allocated * sizeof(DWORD));
                if (!new_data)
                    goto cleanup;

                data = new_data;
            }
            data[used] = wcstol(name, NULL, 10);
        }
    }
    if (used) {
        RegCloseKey(hkey);
        data[0] = used;
        return data;
    }

cleanup:
    /* something failed */
    RegCloseKey(hkey);
    free(data);
    return NULL;
}

/********************************************************************
 *      IInternetZoneManager_QueryInterface
 */
static HRESULT WINAPI ZoneMgrImpl_QueryInterface(IInternetZoneManagerEx2* iface, REFIID riid, void** ppvObject)
{
    ZoneMgrImpl* This = impl_from_IInternetZoneManagerEx2(iface);

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppvObject);

    if(!This || !ppvObject)
        return E_INVALIDARG;

    if(IsEqualIID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppvObject);
    }else if(IsEqualIID(&IID_IInternetZoneManager, riid)) {
        TRACE("(%p)->(IID_InternetZoneManager %p)\n", This, ppvObject);
    }else if(IsEqualIID(&IID_IInternetZoneManagerEx, riid)) {
        TRACE("(%p)->(IID_InternetZoneManagerEx %p)\n", This, ppvObject);
    }else if(IsEqualIID(&IID_IInternetZoneManagerEx2, riid)) {
        TRACE("(%p)->(IID_InternetZoneManagerEx2 %p)\n", This, ppvObject);
    }
    else
    {
        FIXME("Unknown interface: %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    *ppvObject = iface;
    IInternetZoneManagerEx2_AddRef(iface);
    return S_OK;
}

/********************************************************************
 *      IInternetZoneManager_AddRef
 */
static ULONG WINAPI ZoneMgrImpl_AddRef(IInternetZoneManagerEx2* iface)
{
    ZoneMgrImpl* This = impl_from_IInternetZoneManagerEx2(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%lu)\n",This, refCount - 1);

    return refCount;
}

/********************************************************************
 *      IInternetZoneManager_Release
 */
static ULONG WINAPI ZoneMgrImpl_Release(IInternetZoneManagerEx2* iface)
{
    ZoneMgrImpl* This = impl_from_IInternetZoneManagerEx2(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%lu)\n",This, refCount + 1);

    if(!refCount) {
        while (This->zonemap_count) free(This->zonemaps[--This->zonemap_count]);
        free(This->zonemaps);
        free(This);
        URLMON_UnlockModule();
    }
    
    return refCount;
}

/********************************************************************
 *      IInternetZoneManager_GetZoneAttributes
 */
static HRESULT WINAPI ZoneMgrImpl_GetZoneAttributes(IInternetZoneManagerEx2* iface,
                                                    DWORD dwZone,
                                                    ZONEATTRIBUTES* pZoneAttributes)
{
    ZoneMgrImpl* This = impl_from_IInternetZoneManagerEx2(iface);
    HRESULT hr;
    HKEY hcu;
    HKEY hklm = NULL;

    TRACE("(%p)->(%ld %p)\n", This, dwZone, pZoneAttributes);

    if (!pZoneAttributes)
        return E_INVALIDARG;

    hr = open_zone_key(HKEY_CURRENT_USER, dwZone, &hcu);
    if (FAILED(hr))
        return S_OK;  /* IE6 and older returned E_FAIL here */

    hr = open_zone_key(HKEY_LOCAL_MACHINE, dwZone, &hklm);
    if (FAILED(hr))
        TRACE("Zone %ld not in HKLM\n", dwZone);

    get_string_from_reg(hcu, hklm, L"DisplayName", pZoneAttributes->szDisplayName, MAX_ZONE_PATH);
    get_string_from_reg(hcu, hklm, L"Description", pZoneAttributes->szDescription, MAX_ZONE_DESCRIPTION);
    get_string_from_reg(hcu, hklm, L"Icon", pZoneAttributes->szIconPath, MAX_ZONE_PATH);
    get_dword_from_reg(hcu, hklm, L"MinLevel", &pZoneAttributes->dwTemplateMinLevel);
    get_dword_from_reg(hcu, hklm, L"CurrentLevel", &pZoneAttributes->dwTemplateCurrentLevel);
    get_dword_from_reg(hcu, hklm, L"RecommendedLevel", &pZoneAttributes->dwTemplateRecommended);
    get_dword_from_reg(hcu, hklm, L"Flags", &pZoneAttributes->dwFlags);

    RegCloseKey(hklm);
    RegCloseKey(hcu);
    return S_OK;
}

/********************************************************************
 *      IInternetZoneManager_SetZoneAttributes
 */
static HRESULT WINAPI ZoneMgrImpl_SetZoneAttributes(IInternetZoneManagerEx2* iface,
                                                    DWORD dwZone,
                                                    ZONEATTRIBUTES* pZoneAttributes)
{
    ZoneMgrImpl* This = impl_from_IInternetZoneManagerEx2(iface);
    HRESULT hr;
    HKEY hcu;

    TRACE("(%p)->(%ld %p)\n", This, dwZone, pZoneAttributes);

    if (!pZoneAttributes)
        return E_INVALIDARG;

    hr = open_zone_key(HKEY_CURRENT_USER, dwZone, &hcu);
    if (FAILED(hr))
        return S_OK;  /* IE6 returned E_FAIL here */

    /* cbSize is ignored */
    RegSetValueExW(hcu, L"DisplayName", 0, REG_SZ, (BYTE*)pZoneAttributes->szDisplayName,
                    (lstrlenW(pZoneAttributes->szDisplayName)+1)* sizeof(WCHAR));

    RegSetValueExW(hcu, L"Description", 0, REG_SZ, (BYTE*)pZoneAttributes->szDescription,
                    (lstrlenW(pZoneAttributes->szDescription)+1)* sizeof(WCHAR));

    RegSetValueExW(hcu, L"Icon", 0, REG_SZ, (BYTE*)pZoneAttributes->szIconPath,
                    (lstrlenW(pZoneAttributes->szIconPath)+1)* sizeof(WCHAR));

    RegSetValueExW(hcu, L"MinLevel", 0, REG_DWORD,
                    (const BYTE*) &pZoneAttributes->dwTemplateMinLevel, sizeof(DWORD));

    RegSetValueExW(hcu, L"CurrentLevel", 0, REG_DWORD,
                    (const BYTE*) &pZoneAttributes->dwTemplateCurrentLevel, sizeof(DWORD));

    RegSetValueExW(hcu, L"RecommendedLevel", 0, REG_DWORD,
                    (const BYTE*) &pZoneAttributes->dwTemplateRecommended, sizeof(DWORD));

    RegSetValueExW(hcu, L"Flags", 0, REG_DWORD, (const BYTE*) &pZoneAttributes->dwFlags, sizeof(DWORD));
    RegCloseKey(hcu);
    return S_OK;

}

/********************************************************************
 *      IInternetZoneManager_GetZoneCustomPolicy
 */
static HRESULT WINAPI ZoneMgrImpl_GetZoneCustomPolicy(IInternetZoneManagerEx2* iface,
                                                      DWORD dwZone,
                                                      REFGUID guidKey,
                                                      BYTE** ppPolicy,
                                                      DWORD* pcbPolicy,
                                                      URLZONEREG ulrZoneReg)
{
    FIXME("(%p)->(%08lx %s %p %p %08x) stub\n", iface, dwZone, debugstr_guid(guidKey),
                                                    ppPolicy, pcbPolicy, ulrZoneReg);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_SetZoneCustomPolicy
 */
static HRESULT WINAPI ZoneMgrImpl_SetZoneCustomPolicy(IInternetZoneManagerEx2* iface,
                                                      DWORD dwZone,
                                                      REFGUID guidKey,
                                                      BYTE* ppPolicy,
                                                      DWORD cbPolicy,
                                                      URLZONEREG ulrZoneReg)
{
    FIXME("(%p)->(%08lx %s %p %08lx %08x) stub\n", iface, dwZone, debugstr_guid(guidKey),
                                                    ppPolicy, cbPolicy, ulrZoneReg);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_GetZoneActionPolicy
 */
static HRESULT WINAPI ZoneMgrImpl_GetZoneActionPolicy(IInternetZoneManagerEx2* iface,
        DWORD dwZone, DWORD dwAction, BYTE* pPolicy, DWORD cbPolicy, URLZONEREG urlZoneReg)
{
    TRACE("(%p)->(%ld %08lx %p %ld %d)\n", iface, dwZone, dwAction, pPolicy,
            cbPolicy, urlZoneReg);

    if(!pPolicy)
        return E_INVALIDARG;

    return get_action_policy(dwZone, dwAction, pPolicy, cbPolicy, urlZoneReg);
}

/********************************************************************
 *      IInternetZoneManager_SetZoneActionPolicy
 */
static HRESULT WINAPI ZoneMgrImpl_SetZoneActionPolicy(IInternetZoneManagerEx2* iface,
                                                      DWORD dwZone,
                                                      DWORD dwAction,
                                                      BYTE* pPolicy,
                                                      DWORD cbPolicy,
                                                      URLZONEREG urlZoneReg)
{
    FIXME("(%p)->(%08lx %08lx %p %08lx %08x) stub\n", iface, dwZone, dwAction, pPolicy,
                                                       cbPolicy, urlZoneReg);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_PromptAction
 */
static HRESULT WINAPI ZoneMgrImpl_PromptAction(IInternetZoneManagerEx2* iface,
                                               DWORD dwAction,
                                               HWND hwndParent,
                                               LPCWSTR pwszUrl,
                                               LPCWSTR pwszText,
                                               DWORD dwPromptFlags)
{
    FIXME("%p %08lx %p %s %s %08lx\n", iface, dwAction, hwndParent,
          debugstr_w(pwszUrl), debugstr_w(pwszText), dwPromptFlags );
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_LogAction
 */
static HRESULT WINAPI ZoneMgrImpl_LogAction(IInternetZoneManagerEx2* iface,
                                            DWORD dwAction,
                                            LPCWSTR pwszUrl,
                                            LPCWSTR pwszText,
                                            DWORD dwLogFlags)
{
    FIXME("(%p)->(%08lx %s %s %08lx) stub\n", iface, dwAction, debugstr_w(pwszUrl),
                                              debugstr_w(pwszText), dwLogFlags);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManager_CreateZoneEnumerator
 */
static HRESULT WINAPI ZoneMgrImpl_CreateZoneEnumerator(IInternetZoneManagerEx2* iface,
                                                       DWORD* pdwEnum,
                                                       DWORD* pdwCount,
                                                       DWORD dwFlags)
{
    ZoneMgrImpl* This = impl_from_IInternetZoneManagerEx2(iface);
    LPDWORD * new_maps;
    LPDWORD data;
    DWORD new_map_count;
    DWORD i;

    TRACE("(%p)->(%p, %p, 0x%08lx)\n", This, pdwEnum, pdwCount, dwFlags);
    if (!pdwEnum || !pdwCount || (dwFlags != 0))
        return E_INVALIDARG;

    data = build_zonemap_from_reg();
    TRACE("found %ld zones\n", data ? data[0] : -1);

    if (!data)
        return E_FAIL;

    for (i = 0; i < This->zonemap_count; i++) {
        if (This->zonemaps && !This->zonemaps[i]) {
            This->zonemaps[i] = data;
            *pdwEnum = i;
            *pdwCount = data[0];
            return S_OK;
        }
    }

    /* try to double the number of pointers in the array */
    new_map_count = This->zonemaps ? This->zonemap_count * 2 : 2;
    new_maps = _recalloc(This->zonemaps, new_map_count, sizeof(DWORD*));
    if (!new_maps) {
        free(data);
        return E_FAIL;
    }
    This->zonemaps = new_maps;
    This->zonemap_count = new_map_count;
    This->zonemaps[i] = data;
    *pdwEnum = i;
    *pdwCount = data[0];
    return S_OK;
}

/********************************************************************
 *      IInternetZoneManager_GetZoneAt
 */
static HRESULT WINAPI ZoneMgrImpl_GetZoneAt(IInternetZoneManagerEx2* iface,
                                            DWORD dwEnum,
                                            DWORD dwIndex,
                                            DWORD* pdwZone)
{
    ZoneMgrImpl* This = impl_from_IInternetZoneManagerEx2(iface);
    LPDWORD data;

    TRACE("(%p)->(0x%08lx, %ld, %p)\n", This, dwEnum, dwIndex, pdwZone);

    /* make sure, that dwEnum and dwIndex are in the valid range */
    if (dwEnum < This->zonemap_count) {
        if ((data = This->zonemaps[dwEnum])) {
            if (dwIndex < data[0]) {
                *pdwZone = data[dwIndex + 1];
                return S_OK;
            }
        }
    }
    return E_INVALIDARG;
}

/********************************************************************
 *      IInternetZoneManager_DestroyZoneEnumerator
 */
static HRESULT WINAPI ZoneMgrImpl_DestroyZoneEnumerator(IInternetZoneManagerEx2* iface,
                                                        DWORD dwEnum)
{
    ZoneMgrImpl* This = impl_from_IInternetZoneManagerEx2(iface);
    LPDWORD data;

    TRACE("(%p)->(0x%08lx)\n", This, dwEnum);
    /* make sure, that dwEnum is valid */
    if (dwEnum < This->zonemap_count) {
        if ((data = This->zonemaps[dwEnum])) {
            This->zonemaps[dwEnum] = NULL;
            free(data);
            return S_OK;
        }
    }
    return E_INVALIDARG;
}

/********************************************************************
 *      IInternetZoneManager_CopyTemplatePoliciesToZone
 */
static HRESULT WINAPI ZoneMgrImpl_CopyTemplatePoliciesToZone(IInternetZoneManagerEx2* iface,
                                                             DWORD dwTemplate,
                                                             DWORD dwZone,
                                                             DWORD dwReserved)
{
    FIXME("(%p)->(%08lx %08lx %08lx) stub\n", iface, dwTemplate, dwZone, dwReserved);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManagerEx_GetZoneActionPolicyEx
 */
static HRESULT WINAPI ZoneMgrImpl_GetZoneActionPolicyEx(IInternetZoneManagerEx2* iface,
                                                        DWORD dwZone,
                                                        DWORD dwAction,
                                                        BYTE* pPolicy,
                                                        DWORD cbPolicy,
                                                        URLZONEREG urlZoneReg,
                                                        DWORD dwFlags)
{
    TRACE("(%p)->(%ld, 0x%lx, %p, %ld, %d, 0x%lx)\n", iface, dwZone,
            dwAction, pPolicy, cbPolicy, urlZoneReg, dwFlags);

    if(!pPolicy)
        return E_INVALIDARG;

    if (dwFlags)
        FIXME("dwFlags 0x%lx ignored\n", dwFlags);

    return get_action_policy(dwZone, dwAction, pPolicy, cbPolicy, urlZoneReg);
}

/********************************************************************
 *      IInternetZoneManagerEx_SetZoneActionPolicyEx
 */
static HRESULT WINAPI ZoneMgrImpl_SetZoneActionPolicyEx(IInternetZoneManagerEx2* iface,
                                                        DWORD dwZone,
                                                        DWORD dwAction,
                                                        BYTE* pPolicy,
                                                        DWORD cbPolicy,
                                                        URLZONEREG urlZoneReg,
                                                        DWORD dwFlags)
{
    FIXME("(%p)->(%ld, 0x%lx, %p, %ld, %d, 0x%lx) stub\n", iface, dwZone, dwAction, pPolicy,
                                                       cbPolicy, urlZoneReg, dwFlags);
    return E_NOTIMPL;
}

/********************************************************************
 *      IInternetZoneManagerEx2_GetZoneAttributesEx
 */
static HRESULT WINAPI ZoneMgrImpl_GetZoneAttributesEx(IInternetZoneManagerEx2* iface,
                                                      DWORD dwZone,
                                                      ZONEATTRIBUTES* pZoneAttributes,
                                                      DWORD dwFlags)
{
    TRACE("(%p)->(%ld, %p, 0x%lx)\n", iface, dwZone, pZoneAttributes, dwFlags);

    if (dwFlags)
        FIXME("dwFlags 0x%lx ignored\n", dwFlags);

    return IInternetZoneManagerEx2_GetZoneAttributes(iface, dwZone, pZoneAttributes);
}


/********************************************************************
 *      IInternetZoneManagerEx2_GetZoneSecurityState
 */
static HRESULT WINAPI ZoneMgrImpl_GetZoneSecurityState(IInternetZoneManagerEx2* iface,
                                                       DWORD dwZoneIndex,
                                                       BOOL fRespectPolicy,
                                                       LPDWORD pdwState,
                                                       BOOL *pfPolicyEncountered)
{
    FIXME("(%p)->(%ld, %d, %p, %p) stub\n", iface, dwZoneIndex, fRespectPolicy,
                                           pdwState, pfPolicyEncountered);

    *pdwState = SECURITY_IE_STATE_GREEN;

    if (pfPolicyEncountered)
        *pfPolicyEncountered = FALSE;

    return S_OK;
}

/********************************************************************
 *      IInternetZoneManagerEx2_GetIESecurityState
 */
static HRESULT WINAPI ZoneMgrImpl_GetIESecurityState(IInternetZoneManagerEx2* iface,
                                                     BOOL fRespectPolicy,
                                                     LPDWORD pdwState,
                                                     BOOL *pfPolicyEncountered,
                                                     BOOL fNoCache)
{
    FIXME("(%p)->(%d, %p, %p, %d) stub\n", iface, fRespectPolicy, pdwState,
                                           pfPolicyEncountered, fNoCache);

    *pdwState = SECURITY_IE_STATE_GREEN;

    if (pfPolicyEncountered)
        *pfPolicyEncountered = FALSE;

    return S_OK;
}

/********************************************************************
 *      IInternetZoneManagerEx2_FixInsecureSettings
 */
static HRESULT WINAPI ZoneMgrImpl_FixInsecureSettings(IInternetZoneManagerEx2* iface)
{
    FIXME("(%p) stub\n", iface);
    return S_OK;
}

/********************************************************************
 *      IInternetZoneManager_Construct
 */
static const IInternetZoneManagerEx2Vtbl ZoneMgrImplVtbl = {
    ZoneMgrImpl_QueryInterface,
    ZoneMgrImpl_AddRef,
    ZoneMgrImpl_Release,
    /* IInternetZoneManager */
    ZoneMgrImpl_GetZoneAttributes,
    ZoneMgrImpl_SetZoneAttributes,
    ZoneMgrImpl_GetZoneCustomPolicy,
    ZoneMgrImpl_SetZoneCustomPolicy,
    ZoneMgrImpl_GetZoneActionPolicy,
    ZoneMgrImpl_SetZoneActionPolicy,
    ZoneMgrImpl_PromptAction,
    ZoneMgrImpl_LogAction,
    ZoneMgrImpl_CreateZoneEnumerator,
    ZoneMgrImpl_GetZoneAt,
    ZoneMgrImpl_DestroyZoneEnumerator,
    ZoneMgrImpl_CopyTemplatePoliciesToZone,
    /* IInternetZoneManagerEx */
    ZoneMgrImpl_GetZoneActionPolicyEx,
    ZoneMgrImpl_SetZoneActionPolicyEx,
    /* IInternetZoneManagerEx2 */
    ZoneMgrImpl_GetZoneAttributesEx,
    ZoneMgrImpl_GetZoneSecurityState,
    ZoneMgrImpl_GetIESecurityState,
    ZoneMgrImpl_FixInsecureSettings,
};

HRESULT ZoneMgrImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    ZoneMgrImpl *ret = calloc(1, sizeof(ZoneMgrImpl));

    TRACE("(%p %p)\n", pUnkOuter, ppobj);
    ret->IInternetZoneManagerEx2_iface.lpVtbl = &ZoneMgrImplVtbl;
    ret->ref = 1;
    *ppobj = &ret->IInternetZoneManagerEx2_iface;

    URLMON_LockModule();

    return S_OK;
}

/***********************************************************************
 *           CoInternetCreateSecurityManager (URLMON.@)
 *
 */
HRESULT WINAPI CoInternetCreateSecurityManager( IServiceProvider *pSP,
    IInternetSecurityManager **ppSM, DWORD dwReserved )
{
    TRACE("%p %p %ld\n", pSP, ppSM, dwReserved );

    if(pSP)
        FIXME("pSP not supported\n");

    return SecManagerImpl_Construct(NULL, (void**) ppSM);
}

/********************************************************************
 *      CoInternetCreateZoneManager (URLMON.@)
 */
HRESULT WINAPI CoInternetCreateZoneManager(IServiceProvider* pSP, IInternetZoneManager** ppZM, DWORD dwReserved)
{
    TRACE("(%p %p %lx)\n", pSP, ppZM, dwReserved);
    return ZoneMgrImpl_Construct(NULL, (void**)ppZM);
}

static HRESULT parse_security_url(const WCHAR *url, PSUACTION action, WCHAR **result) {
    IInternetProtocolInfo *protocol_info;
    WCHAR *tmp, *new_url = NULL, *alloc_url = NULL;
    DWORD size, new_size;
    HRESULT hres = S_OK, parse_hres;

    while(1) {
        TRACE("parsing %s\n", debugstr_w(url));

        protocol_info = get_protocol_info(url);
        if(!protocol_info)
            break;

        size = lstrlenW(url)+1;
        new_url = CoTaskMemAlloc(size*sizeof(WCHAR));
        if(!new_url) {
            hres = E_OUTOFMEMORY;
            break;
        }

        new_size = 0;
        parse_hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, PARSE_SECURITY_URL, 0, new_url, size, &new_size, 0);
        if(parse_hres == S_FALSE) {
            if(!new_size) {
                hres = E_UNEXPECTED;
                break;
            }

            tmp = CoTaskMemRealloc(new_url, new_size*sizeof(WCHAR));
            if(!tmp) {
                hres = E_OUTOFMEMORY;
                break;
            }
            new_url = tmp;
            parse_hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, PARSE_SECURITY_URL, 0, new_url,
                    new_size, &new_size, 0);
            if(parse_hres == S_FALSE) {
                hres = E_FAIL;
                break;
            }
        }

        if(parse_hres != S_OK || !wcscmp(url, new_url))
            break;

        CoTaskMemFree(alloc_url);
        url = alloc_url = new_url;
        new_url = NULL;
    }

    CoTaskMemFree(new_url);

    if(hres != S_OK) {
        WARN("failed: %08lx\n", hres);
        CoTaskMemFree(alloc_url);
        return hres;
    }

    if(action == PSU_DEFAULT && (protocol_info = get_protocol_info(url))) {
        size = lstrlenW(url)+1;
        new_url = CoTaskMemAlloc(size * sizeof(WCHAR));
        if(new_url) {
            new_size = 0;
            parse_hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, PARSE_SECURITY_DOMAIN, 0,
                    new_url, size, &new_size, 0);
            if(parse_hres == S_FALSE) {
                if(new_size) {
                    tmp = CoTaskMemRealloc(new_url, new_size*sizeof(WCHAR));
                    if(tmp) {
                        new_url = tmp;
                        parse_hres = IInternetProtocolInfo_ParseUrl(protocol_info, url, PARSE_SECURITY_DOMAIN, 0, new_url,
                                new_size, &new_size, 0);
                        if(parse_hres == S_FALSE)
                            hres = E_FAIL;
                    }else {
                        hres = E_OUTOFMEMORY;
                    }
                }else {
                    hres = E_UNEXPECTED;
                }
            }

            if(hres == S_OK && parse_hres == S_OK) {
                CoTaskMemFree(alloc_url);
                url = alloc_url = new_url;
                new_url = NULL;
            }

            CoTaskMemFree(new_url);
        }else {
            hres = E_OUTOFMEMORY;
        }
        IInternetProtocolInfo_Release(protocol_info);
    }

    if(FAILED(hres)) {
        WARN("failed %08lx\n", hres);
        CoTaskMemFree(alloc_url);
        return hres;
    }

    if(!alloc_url) {
        size = lstrlenW(url)+1;
        alloc_url = CoTaskMemAlloc(size * sizeof(WCHAR));
        if(!alloc_url)
            return E_OUTOFMEMORY;
        memcpy(alloc_url, url, size * sizeof(WCHAR));
    }

    *result = alloc_url;
    return S_OK;
}

/********************************************************************
 *      CoInternetGetSecurityUrl (URLMON.@)
 */
HRESULT WINAPI CoInternetGetSecurityUrl(LPCWSTR pwzUrl, LPWSTR *ppwzSecUrl, PSUACTION psuAction, DWORD dwReserved)
{
    WCHAR *secure_url;
    HRESULT hres;

    TRACE("(%p,%p,%u,%lu)\n", pwzUrl, ppwzSecUrl, psuAction, dwReserved);

    hres = parse_security_url(pwzUrl, psuAction, &secure_url);
    if(FAILED(hres))
        return hres;

    if(psuAction != PSU_SECURITY_URL_ONLY) {
        PARSEDURLW parsed_url = { sizeof(parsed_url) };
        DWORD size;

        /* FIXME: Use helpers from uri.c */
        if(SUCCEEDED(ParseURLW(secure_url, &parsed_url))) {
            WCHAR *new_url;

            switch(parsed_url.nScheme) {
            case URL_SCHEME_FTP:
            case URL_SCHEME_HTTP:
            case URL_SCHEME_HTTPS:
                size = lstrlenW(secure_url)+1;
                new_url = CoTaskMemAlloc(size * sizeof(WCHAR));
                if(new_url)
                    hres = UrlGetPartW(secure_url, new_url, &size, URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME);
                else
                    hres = E_OUTOFMEMORY;
                CoTaskMemFree(secure_url);
                if(hres != S_OK) {
                    WARN("UrlGetPart failed: %08lx\n", hres);
                    CoTaskMemFree(new_url);
                    return FAILED(hres) ? hres : E_FAIL;
                }
                secure_url = new_url;
            }
        }
    }

    *ppwzSecUrl = secure_url;
    return S_OK;
}

/********************************************************************
 *      CoInternetGetSecurityUrlEx (URLMON.@)
 */
HRESULT WINAPI CoInternetGetSecurityUrlEx(IUri *pUri, IUri **ppSecUri, PSUACTION psuAction, DWORD_PTR dwReserved)
{
    URL_SCHEME scheme_type;
    BSTR secure_uri;
    WCHAR *ret_url;
    HRESULT hres;

    TRACE("(%p,%p,%u,%Iu)\n", pUri, ppSecUri, psuAction, dwReserved);

    if(!pUri || !ppSecUri)
        return E_INVALIDARG;

    hres = IUri_GetDisplayUri(pUri, &secure_uri);
    if(FAILED(hres))
        return hres;

    hres = parse_security_url(secure_uri, psuAction, &ret_url);
    SysFreeString(secure_uri);
    if(FAILED(hres))
        return hres;

    /* File URIs have to hierarchical. */
    hres = IUri_GetScheme(pUri, (DWORD*)&scheme_type);
    if(SUCCEEDED(hres) && scheme_type == URL_SCHEME_FILE) {
        const WCHAR *tmp = ret_url;

        /* Check and see if a "//" is after the scheme name. */
        tmp += ARRAY_SIZE(L"file");
        if(*tmp != '/' || *(tmp+1) != '/')
            hres = E_INVALIDARG;
    }

    if(SUCCEEDED(hres))
        hres = CreateUri(ret_url, Uri_CREATE_ALLOW_IMPLICIT_WILDCARD_SCHEME, 0, ppSecUri);
    CoTaskMemFree(ret_url);
    return hres;
}

/********************************************************************
 *      CompareSecurityIds (URLMON.@)
 */
HRESULT WINAPI CompareSecurityIds(BYTE *secid1, DWORD size1, BYTE *secid2, DWORD size2, DWORD reserved)
{
    FIXME("(%p %ld %p %ld %lx)\n", secid1, size1, secid2, size2, reserved);
    return E_NOTIMPL;
}

/********************************************************************
 *      IsInternetESCEnabledLocal (URLMON.108)
 *
 * Undocumented, returns TRUE if IE is running in Enhanced Security Configuration.
 */
BOOL WINAPI IsInternetESCEnabledLocal(void)
{
    static BOOL esc_initialized, esc_enabled;

    TRACE("()\n");

    if(!esc_initialized) {
        DWORD type, size, val;
        HKEY zone_map;

        if(RegOpenKeyExW(HKEY_CURRENT_USER, zone_map_keyW, 0, KEY_QUERY_VALUE, &zone_map) == ERROR_SUCCESS) {
            size = sizeof(DWORD);
            if(RegQueryValueExW(zone_map, L"IEHarden", NULL, &type, (BYTE*)&val, &size) == ERROR_SUCCESS)
                esc_enabled = type == REG_DWORD && val != 0;
            RegCloseKey(zone_map);
        }
        esc_initialized = TRUE;
    }

    return esc_enabled;
}
