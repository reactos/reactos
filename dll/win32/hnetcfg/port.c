/*
 * Copyright 2009 Hans Leidekker for CodeWeavers
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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "string.h"
#include "initguid.h"
#include "assert.h"
#include "winsock2.h"
#include "winhttp.h"
#include "shlwapi.h"
#include "xmllite.h"
#include "ole2.h"
#include "netfw.h"
#include "natupnp.h"

#include "wine/debug.h"
#include "hnetcfg_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(hnetcfg);

struct port_mapping
{
    BSTR external_ip;
    LONG external;
    BSTR protocol;
    LONG internal;
    BSTR client;
    VARIANT_BOOL enabled;
    BSTR descr;
};

struct xml_value_desc
{
    const WCHAR *name;
    BSTR value;
};

static struct
{
    LONG refs;
    BOOL winsock_initialized;
    WCHAR locationW[256];
    HINTERNET session, connection;
    WCHAR desc_urlpath[128];
    WCHAR control_url[256];
    unsigned int version;
    struct port_mapping *mappings;
    unsigned int mapping_count;
}
upnp_gateway_connection;

static SRWLOCK upnp_gateway_connection_lock = SRWLOCK_INIT;

static void free_port_mapping( struct port_mapping *mapping )
{
    SysFreeString( mapping->external_ip );
    SysFreeString( mapping->protocol );
    SysFreeString( mapping->client );
    SysFreeString( mapping->descr );
}

static void free_mappings(void)
{
    unsigned int i;

    for (i = 0; i < upnp_gateway_connection.mapping_count; ++i)
        free_port_mapping( &upnp_gateway_connection.mappings[i] );
    free( upnp_gateway_connection.mappings );
    upnp_gateway_connection.mappings = NULL;
    upnp_gateway_connection.mapping_count = 0;
}

static BOOL copy_port_mapping( struct port_mapping *dst, const struct port_mapping *src )
{
    memset( dst, 0, sizeof(*dst) );

#define COPY_BSTR_CHECK(name) if (src->name && !(dst->name = SysAllocString( src->name ))) \
    { \
        free_port_mapping( dst ); \
        return FALSE; \
    }

    COPY_BSTR_CHECK( external_ip );
    COPY_BSTR_CHECK( protocol );
    COPY_BSTR_CHECK( client );
    COPY_BSTR_CHECK( descr );
#undef COPY_BSTR_CHECK

    dst->external = src->external;
    dst->internal = src->internal;
    dst->enabled = src->enabled;
    return TRUE;
}

static BOOL parse_search_response( char *response, WCHAR *locationW, unsigned int location_size )
{
    char *saveptr = NULL, *tok, *tok2;
    unsigned int status;

    tok = strtok_s( response, "\n", &saveptr );
    if (!tok) return FALSE;

    /* HTTP/1.1 200 OK */
    tok2 = strtok( tok, " " );
    if (!tok2) return FALSE;
    tok2 = strtok( NULL, " " );
    if (!tok2) return FALSE;
    status = atoi( tok2 );
    if (status != HTTP_STATUS_OK)
    {
        WARN( "status %u.\n", status );
        return FALSE;
    }
    while ((tok = strtok_s( NULL, "\n", &saveptr )))
    {
        tok2 = strtok( tok, " " );
        if (!tok2) continue;
        if (!stricmp( tok2, "LOCATION:" ))
        {
            tok2 = strtok( NULL, " \r" );
            if (!tok2)
            {
                WARN( "Error parsing location.\n" );
                return FALSE;
            }
            return !!MultiByteToWideChar( CP_UTF8, 0, tok2, -1, locationW,  location_size / 2 );
        }
    }
    return FALSE;
}

static BOOL parse_desc_xml( const char *desc_xml )
{
    static const WCHAR urn_wanipconnection[] = L"urn:schemas-upnp-org:service:WANIPConnection:";
    WCHAR control_url[ARRAY_SIZE(upnp_gateway_connection.control_url)];
    BOOL service_type_matches, control_url_found, found = FALSE;
    unsigned int version = 0;
    XmlNodeType node_type;
    IXmlReader *reader;
    const WCHAR *value;
    BOOL ret = FALSE;
    IStream *stream;
    HRESULT hr;

    if (!(stream = SHCreateMemStream( (BYTE *)desc_xml, strlen( desc_xml ) + 1 ))) return FALSE;
    if (FAILED(hr = CreateXmlReader( &IID_IXmlReader, (void **)&reader, NULL )))
    {
        IStream_Release( stream );
        return FALSE;
    }
    if (FAILED(hr = IXmlReader_SetInput( reader, (IUnknown*)stream ))) goto done;

    while (SUCCEEDED(IXmlReader_Read( reader, &node_type )) && node_type != XmlNodeType_None)
    {
        if (node_type != XmlNodeType_Element) continue;

        if (FAILED(IXmlReader_GetLocalName( reader, &value, NULL ))) goto done;
        if (wcsicmp( value, L"service" )) continue;
        control_url_found = service_type_matches = FALSE;
        while (SUCCEEDED(IXmlReader_Read( reader, &node_type )))
        {
            if (node_type != XmlNodeType_Element && node_type != XmlNodeType_EndElement) continue;
            if (FAILED(IXmlReader_GetLocalName( reader, &value, NULL )))
            {
                WARN( "IXmlReader_GetLocalName failed.\n" );
                goto done;
            }
            if (node_type == XmlNodeType_EndElement)
            {
                if (!wcsicmp( value, L"service" )) break;
                continue;
            }
            if (!wcsicmp( value, L"serviceType" ))
            {
                if (FAILED(IXmlReader_Read(reader, &node_type ))) goto done;
                if (node_type != XmlNodeType_Text) goto done;
                if (FAILED(IXmlReader_GetValue( reader, &value, NULL ))) goto done;
                if (wcsnicmp( value, urn_wanipconnection, ARRAY_SIZE(urn_wanipconnection) - 1 )) break;
                version = _wtoi( value + ARRAY_SIZE(urn_wanipconnection) - 1 );
                service_type_matches = version >= 1;
            }
            else if (!wcsicmp( value, L"controlURL" ))
            {
                if (FAILED(IXmlReader_Read(reader, &node_type ))) goto done;
                if (node_type != XmlNodeType_Text) goto done;
                if (FAILED(IXmlReader_GetValue( reader, &value, NULL ))) goto done;
                if (wcslen( value ) + 1 > ARRAY_SIZE(control_url)) goto done;
                wcscpy( control_url, value );
                control_url_found = TRUE;
            }
        }
        if (service_type_matches && control_url_found)
        {
            if (found)
            {
                FIXME( "Found another WANIPConnection service, ignoring.\n" );
                continue;
            }
            found = TRUE;
            wcscpy( upnp_gateway_connection.control_url, control_url );
            upnp_gateway_connection.version = version;
        }
    }

    ret = found;
done:
    IXmlReader_Release( reader );
    IStream_Release( stream );
    return ret;
}

static BOOL get_xml_elements( const char *desc_xml, struct xml_value_desc *values, unsigned int value_count )
{
    XmlNodeType node_type;
    IXmlReader *reader;
    const WCHAR *value;
    BOOL ret = FALSE;
    IStream *stream;
    unsigned int i;
    HRESULT hr;

    for (i = 0; i < value_count; ++i) assert( !values[i].value );

    if (!(stream = SHCreateMemStream( (BYTE *)desc_xml, strlen( desc_xml ) + 1 ))) return FALSE;
    if (FAILED(hr = CreateXmlReader( &IID_IXmlReader, (void **)&reader, NULL )))
    {
        IStream_Release( stream );
        return FALSE;
    }
    if (FAILED(hr = IXmlReader_SetInput( reader, (IUnknown*)stream ))) goto done;

    while (SUCCEEDED(IXmlReader_Read( reader, &node_type )) && node_type != XmlNodeType_None)
    {
        if (node_type != XmlNodeType_Element) continue;

        if (FAILED(IXmlReader_GetQualifiedName( reader, &value, NULL ))) goto done;
        for (i = 0; i < value_count; ++i)
            if (!wcsicmp( value, values[i].name )) break;
        if (i == value_count) continue;
        if (FAILED(IXmlReader_Read(reader, &node_type ))) goto done;
        if (node_type != XmlNodeType_Text)
        {
            if (node_type == XmlNodeType_EndElement) value = L"";
            else                                     goto done;
        }
        else
        {
            if (FAILED(IXmlReader_GetValue( reader, &value, NULL ))) goto done;
        }
        if (values[i].value)
        {
            WARN( "Duplicate value %s.\n", debugstr_w(values[i].name) );
            goto done;
        }
        if (!(values[i].value = SysAllocString( value ))) goto done;
    }
    ret = TRUE;

done:
    if (!ret)
    {
        for (i = 0; i < value_count; ++i)
        {
            SysFreeString( values[i].value );
            values[i].value = NULL;
        }
    }
    IXmlReader_Release( reader );
    IStream_Release( stream );
    return ret;
}

static BOOL open_gateway_connection(void)
{
    static const int timeout = 3000;

    WCHAR hostname[64];
    URL_COMPONENTS url;

    memset( &url, 0, sizeof(url) );
    url.dwStructSize = sizeof(url);
    url.lpszHostName = hostname;
    url.dwHostNameLength = ARRAY_SIZE(hostname);
    url.lpszUrlPath = upnp_gateway_connection.desc_urlpath;
    url.dwUrlPathLength = ARRAY_SIZE(upnp_gateway_connection.desc_urlpath);

    if (!WinHttpCrackUrl( upnp_gateway_connection.locationW, 0, 0, &url )) return FALSE;

    upnp_gateway_connection.session = WinHttpOpen( L"hnetcfg", WINHTTP_ACCESS_TYPE_NO_PROXY,
                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
    if (!upnp_gateway_connection.session) return FALSE;
    if (!WinHttpSetTimeouts( upnp_gateway_connection.session, timeout, timeout, timeout, timeout ))
        return FALSE;

    TRACE( "hostname %s, urlpath %s, port %u.\n",
           debugstr_w(hostname), debugstr_w(upnp_gateway_connection.desc_urlpath), url.nPort );
    upnp_gateway_connection.connection = WinHttpConnect ( upnp_gateway_connection.session, hostname, url.nPort, 0 );
    if (!upnp_gateway_connection.connection)
    {
        WARN( "WinHttpConnect error %lu.\n", GetLastError() );
        return FALSE;
    }
    return TRUE;
}

static BOOL get_control_url(void)
{
    static const WCHAR *accept_types[] =
    {
        L"text/xml",
        NULL
    };
    unsigned int desc_xml_size, offset;
    DWORD size, status = 0;
    HINTERNET request;
    char *desc_xml;
    BOOL ret;

    request = WinHttpOpenRequest( upnp_gateway_connection.connection, NULL, upnp_gateway_connection.desc_urlpath, NULL,
                                  WINHTTP_NO_REFERER, accept_types, 0 );
    if (!request) return FALSE;

    if (!WinHttpSendRequest( request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0 ))
    {
        WARN( "Error sending request %lu.\n", GetLastError() );
        WinHttpCloseHandle( request );
        return FALSE;
    }
    if (!WinHttpReceiveResponse(request, NULL))
    {
        WARN( "Error receiving response %lu.\n", GetLastError() );
        WinHttpCloseHandle( request );
        return FALSE;
    }
    size = sizeof(status);
    if (!WinHttpQueryHeaders( request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                              NULL, &status, &size, NULL) || status != HTTP_STATUS_OK )
    {
        WARN( "Error response from server, error %lu, http status %lu.\n", GetLastError(), status );
        WinHttpCloseHandle( request );
        return FALSE;
    }
    desc_xml_size = 1024;
    desc_xml = malloc( desc_xml_size );
    offset = 0;
    while (WinHttpReadData( request, desc_xml + offset, desc_xml_size - offset - 1, &size ) && size)
    {
        offset += size;
        if (offset + 1 == desc_xml_size)
        {
            char *new;

            desc_xml_size *= 2;
            if (!(new = realloc( desc_xml, desc_xml_size )))
            {
                ERR( "No memory.\n" );
                break;
            }
            desc_xml = new;
        }
    }
    desc_xml[offset] = 0;
    WinHttpCloseHandle( request );
    ret = parse_desc_xml( desc_xml );
    free( desc_xml );
    return ret;
}

static void gateway_connection_cleanup(void)
{
    TRACE( ".\n" );
    free_mappings();
    WinHttpCloseHandle( upnp_gateway_connection.connection );
    WinHttpCloseHandle( upnp_gateway_connection.session );
    if (upnp_gateway_connection.winsock_initialized) WSACleanup();
    memset( &upnp_gateway_connection, 0, sizeof(upnp_gateway_connection) );
}

static BOOL request_service( const WCHAR *function, const struct xml_value_desc *request_param,
                             unsigned int request_param_count, struct xml_value_desc *result,
                             unsigned int result_count, DWORD *http_status, BSTR *server_error_code_str )
{
    static const char request_template_header[] =
        "<?xml version=\"1.0\"?>\r\n"
        "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                                "SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <SOAP-ENV:Body>\r\n"
        "    <m:%S xmlns:m=\"urn:schemas-upnp-org:service:WANIPConnection:%u\">\r\n";
    static const char request_template_footer[] =
        "    </m:%S>\r\n"
        "  </SOAP-ENV:Body>\r\n"
        "</SOAP-ENV:Envelope>\r\n";

    unsigned int request_data_size, request_len, offset, i, reply_buffer_size;
    char *request_data, *reply_buffer = NULL, *ptr;
    struct xml_value_desc error_value_desc;
    WCHAR request_headers[1024];
    HINTERNET request = NULL;
    BOOL ret = FALSE;
    DWORD size;

    *server_error_code_str = NULL;
    request_data_size = strlen(request_template_header) + strlen(request_template_footer) + 2 * wcslen( function )
                        + 9 /* version + zero terminator */;
    for (i = 0; i < request_param_count; ++i)
    {
        request_data_size += 13 + 2 * wcslen( request_param[i].name ) + wcslen( request_param[i].value );
    }
    if (!(request_data = malloc( request_data_size ))) return FALSE;

    request = WinHttpOpenRequest( upnp_gateway_connection.connection, L"POST", upnp_gateway_connection.control_url,
                                  NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0 );
    if (!request) goto done;

    ptr = request_data;
    snprintf( ptr, request_data_size, request_template_header, function, upnp_gateway_connection.version );
    offset = strlen( ptr );
    ptr += offset;
    request_data_size -= offset;
    for (i = 0; i < request_param_count; ++i)
    {
        snprintf( ptr, request_data_size, "      <%S>%S</%S>\r\n",
                  request_param[i].name, request_param[i].value, request_param[i].name);
        offset = strlen( ptr );
        ptr += offset;
        request_data_size -= offset;
    }
    snprintf( ptr, request_data_size, request_template_footer, function );

    request_len = strlen( request_data );
    swprintf( request_headers, ARRAY_SIZE(request_headers),
            L"SOAPAction: \"urn:schemas-upnp-org:service:WANIPConnection:%u#%s\"\r\n"
            L"Content-Type: text/xml",
            upnp_gateway_connection.version, function );
    if (!WinHttpSendRequest( request, request_headers, -1, request_data, request_len, request_len, 0 ))
    {
        WARN( "Error sending request %lu.\n", GetLastError() );
        goto done;
    }
    if (!WinHttpReceiveResponse(request, NULL))
    {
        WARN( "Error receiving response %lu.\n", GetLastError() );
        goto done;
    }
    size = sizeof(*http_status);
    if (!WinHttpQueryHeaders( request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                              NULL, http_status, &size, NULL) || *http_status != HTTP_STATUS_OK )
    {
        if (*http_status != HTTP_STATUS_SERVER_ERROR)
        {
            ret = TRUE;
            goto done;
        }
    }

    offset = 0;
    reply_buffer_size = 1024;
    if (!(reply_buffer = malloc( reply_buffer_size ))) goto done;
    while ((ret = WinHttpReadData( request, reply_buffer + offset, reply_buffer_size - offset - 1, &size )) && size)
    {
        offset += size;
        if (offset + 1 == reply_buffer_size)
        {
            char *new;

            reply_buffer_size *= 2;
            if (!(new = realloc( reply_buffer, reply_buffer_size ))) goto done;
            reply_buffer = new;
        }
    }
    reply_buffer[offset] = 0;

    if (*http_status == HTTP_STATUS_OK) ret = get_xml_elements( reply_buffer, result, result_count );
    else
    {
        error_value_desc.name = L"errorCode";
        error_value_desc.value = NULL;
        if ((ret = get_xml_elements( reply_buffer, &error_value_desc, 1 )))
            *server_error_code_str = error_value_desc.value;
    }

done:
    free( reply_buffer );
    free( request_data );
    WinHttpCloseHandle( request );
    return ret;
}

enum port_mapping_parameter
{
    PM_EXTERNAL_IP,
    PM_EXTERNAL,
    PM_PROTOCOL,
    PM_INTERNAL,
    PM_CLIENT,
    PM_ENABLED,
    PM_DESC,
    PM_LEASE_DURATION,
    PM_LAST,
    PM_REMOVE_PORT_LAST = PM_INTERNAL,
};

static struct xml_value_desc port_mapping_template[] =
{
    { L"NewRemoteHost" },
    { L"NewExternalPort" },
    { L"NewProtocol" },
    { L"NewInternalPort" },
    { L"NewInternalClient" },
    { L"NewEnabled" },
    { L"NewPortMappingDescription" },
    { L"NewLeaseDuration" },
};

static LONG long_from_bstr( BSTR s )
{
    if (!s) return 0;
    return _wtoi( s );
}

static BSTR mapping_move_bstr( BSTR *s )
{
    BSTR ret;

    if (*s)
    {
        ret = *s;
        *s = NULL;
    }
    else if (!(ret = SysAllocString( L"" )))
    {
        ERR( "No memory.\n" );
    }
    return ret;
}

static void update_mapping_list(void)
{
    struct xml_value_desc mapping_desc[ARRAY_SIZE(port_mapping_template)];
    struct xml_value_desc index_param;
    struct port_mapping *new_mappings;
    unsigned int i, index;
    WCHAR index_str[9];
    BSTR error_str;
    DWORD status;
    BOOL ret;

    free_mappings();

    index_param.name = L"NewPortMappingIndex";

    index = 0;
    while (1)
    {
        new_mappings = realloc( upnp_gateway_connection.mappings, (index + 1) * sizeof(*new_mappings) );
        if (!new_mappings) break;
        upnp_gateway_connection.mappings = new_mappings;

        memcpy( mapping_desc, port_mapping_template, sizeof(mapping_desc) );
        swprintf( index_str, ARRAY_SIZE(index_str), L"%u", index );
        index_param.value = SysAllocString( index_str );
        ret = request_service( L"GetGenericPortMappingEntry", &index_param, 1,
                               mapping_desc, ARRAY_SIZE(mapping_desc), &status, &error_str );
        SysFreeString( index_param.value );
        if (!ret) break;
        if (status != HTTP_STATUS_OK)
        {
            if (error_str)
            {
                if (long_from_bstr( error_str ) != 713)
                    WARN( "Server returned error %s.\n", debugstr_w(error_str) );
                SysFreeString( error_str );
            }
            break;
        }
        new_mappings[index].external_ip = mapping_move_bstr( &mapping_desc[PM_EXTERNAL_IP].value );
        new_mappings[index].external = long_from_bstr( mapping_desc[PM_EXTERNAL].value );
        new_mappings[index].protocol = mapping_move_bstr( &mapping_desc[PM_PROTOCOL].value );
        new_mappings[index].internal = long_from_bstr( mapping_desc[PM_INTERNAL].value );
        new_mappings[index].client = mapping_move_bstr( &mapping_desc[PM_CLIENT].value );
        if (mapping_desc[PM_ENABLED].value && (!wcsicmp( mapping_desc[PM_ENABLED].value, L"true" )
                                               || long_from_bstr( mapping_desc[PM_ENABLED].value )))
            new_mappings[index].enabled = VARIANT_TRUE;
        else
            new_mappings[index].enabled = VARIANT_FALSE;
        new_mappings[index].descr = mapping_move_bstr( &mapping_desc[PM_DESC].value );

        TRACE( "%s %s %s:%lu -> %s:%lu, enabled %d.\n", debugstr_w(new_mappings[index].descr),
               debugstr_w(new_mappings[index].protocol), debugstr_w(new_mappings[index].external_ip),
               new_mappings[index].external, debugstr_w(new_mappings[index].client),
               new_mappings[index].internal, new_mappings[index].enabled );

        for (i = 0; i < ARRAY_SIZE(mapping_desc); ++i)
            SysFreeString( mapping_desc[i].value );
        upnp_gateway_connection.mappings = new_mappings;
        upnp_gateway_connection.mapping_count = ++index;
    }
}

static BOOL remove_port_mapping( LONG port, BSTR protocol )
{
    struct xml_value_desc mapping_desc[PM_REMOVE_PORT_LAST];
    DWORD status = 0;
    BSTR error_str;
    WCHAR portW[6];
    BOOL ret;

    AcquireSRWLockExclusive( &upnp_gateway_connection_lock );
    memcpy( mapping_desc, port_mapping_template, sizeof(mapping_desc) );
    swprintf( portW, ARRAY_SIZE(portW), L"%u", port );
    mapping_desc[PM_EXTERNAL_IP].value = SysAllocString( L"" );
    mapping_desc[PM_EXTERNAL].value = SysAllocString( portW );
    mapping_desc[PM_PROTOCOL].value = protocol;

    ret = request_service( L"DeletePortMapping", mapping_desc, PM_REMOVE_PORT_LAST,
                           NULL, 0, &status, &error_str );
    if (ret && status != HTTP_STATUS_OK)
    {
        WARN( "status %lu, server returned error %s.\n", status, debugstr_w(error_str) );
        SysFreeString( error_str );
        ret = FALSE;
    }
    else if (!ret)
    {
        WARN( "Request failed.\n" );
    }
    update_mapping_list();
    ReleaseSRWLockExclusive( &upnp_gateway_connection_lock );

    SysFreeString( mapping_desc[PM_EXTERNAL_IP].value );
    SysFreeString( mapping_desc[PM_EXTERNAL].value );
    return ret;
}

static BOOL add_port_mapping( LONG external, BSTR protocol, LONG internal, BSTR client,
                              VARIANT_BOOL enabled, BSTR description )
{
    struct xml_value_desc mapping_desc[PM_LAST];
    WCHAR externalW[6], internalW[6];
    DWORD status = 0;
    BSTR error_str;
    BOOL ret;

    AcquireSRWLockExclusive( &upnp_gateway_connection_lock );
    memcpy( mapping_desc, port_mapping_template, sizeof(mapping_desc) );
    swprintf( externalW, ARRAY_SIZE(externalW), L"%u", external );
    swprintf( internalW, ARRAY_SIZE(internalW), L"%u", internal );
    mapping_desc[PM_EXTERNAL_IP].value = SysAllocString( L"" );
    mapping_desc[PM_EXTERNAL].value = SysAllocString( externalW );
    mapping_desc[PM_PROTOCOL].value = protocol;
    mapping_desc[PM_INTERNAL].value = SysAllocString( internalW );
    mapping_desc[PM_CLIENT].value = client;
    mapping_desc[PM_ENABLED].value = SysAllocString( enabled ? L"1" : L"0" );
    mapping_desc[PM_DESC].value = description;
    mapping_desc[PM_LEASE_DURATION].value = SysAllocString( L"0" );

    ret = request_service( L"AddPortMapping", mapping_desc, PM_LAST,
                           NULL, 0, &status, &error_str );
    if (ret && status != HTTP_STATUS_OK)
    {
        WARN( "status %lu, server returned error %s.\n", status, debugstr_w(error_str) );
        SysFreeString( error_str );
        ret = FALSE;
    }
    else if (!ret)
    {
        WARN( "Request failed.\n" );
    }
    update_mapping_list();
    ReleaseSRWLockExclusive( &upnp_gateway_connection_lock );

    SysFreeString( mapping_desc[PM_EXTERNAL_IP].value );
    SysFreeString( mapping_desc[PM_EXTERNAL].value );
    SysFreeString( mapping_desc[PM_INTERNAL].value );
    SysFreeString( mapping_desc[PM_ENABLED].value );
    SysFreeString( mapping_desc[PM_LEASE_DURATION].value );
    return ret;
}

static BOOL init_gateway_connection(void)
{
    static const char upnp_search_request[] =
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST:239.255.255.250:1900\r\n"
        "ST:upnp:rootdevice\r\n"
        "MX:2\r\n"
        "MAN:\"ssdp:discover\"\r\n"
        "\r\n";

    const DWORD timeout = 1000;
    int len, address_len;
    char buffer[2048];
    SOCKADDR_IN addr;
    WSADATA wsa_data;
    unsigned int i;
    SOCKET s;

    upnp_gateway_connection.winsock_initialized = WSAStartup( MAKEWORD( 2, 2 ), &wsa_data );
    if ((s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP )) == -1)
    {
        ERR( "Failed to create socket, error %u.\n", WSAGetLastError() );
        return FALSE;
    }
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) == SOCKET_ERROR)
    {
        ERR( "setsockopt(SO_RCVTIME0) failed, error %u.\n", WSAGetLastError() );
        closesocket( s );
        return FALSE;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons( 1900 );
    addr.sin_addr.S_un.S_addr = inet_addr( "239.255.255.250" );
    if (sendto( s, upnp_search_request, strlen( upnp_search_request ), 0, (SOCKADDR *)&addr, sizeof(addr) )
        == SOCKET_ERROR)
    {
        ERR( "sendto failed, error %u\n", WSAGetLastError() );
        closesocket( s );
        return FALSE;
    }
    /* Windows has a dedicated SSDP discovery service which maintains gateway device info and does
     * not usually delay in get_StaticPortMappingCollection(). Although it may still delay depending
     * on network connection state and always delays in IUPnPNAT_get_NATEventManager(). */
    FIXME( "Waiting for reply from router.\n" );
    for (i = 0; i < 2; ++i)
    {
        address_len = sizeof(addr);
        len = recvfrom( s, buffer, sizeof(buffer) - 1, 0, (SOCKADDR *)&addr, &address_len );
        if (len == -1)
        {
            if (WSAGetLastError() != WSAETIMEDOUT)
            {
                WARN( "recvfrom error %u.\n", WSAGetLastError() );
                closesocket( s );
                return FALSE;
            }
        }
        else break;
    }
    closesocket( s );
    if (i == 2)
    {
        TRACE( "No reply from router.\n" );
        return FALSE;
    }
    TRACE( "Received reply from gateway, len %d.\n", len );
    buffer[len] = 0;
    if (!parse_search_response( buffer, upnp_gateway_connection.locationW, sizeof(upnp_gateway_connection.locationW) ))
    {
        WARN( "Error parsing response.\n" );
        return FALSE;
    }
    TRACE( "Gateway description location %s.\n", debugstr_w(upnp_gateway_connection.locationW) );
    if (!open_gateway_connection())
    {
        WARN( "Error opening gateway connection.\n" );
        return FALSE;
    }
    TRACE( "Opened gateway connection.\n" );
    if (!get_control_url())
    {
        WARN( "Could not get_control URL.\n" );
        gateway_connection_cleanup();
        return FALSE;
    }
    TRACE( "control_url %s, version %u.\n", debugstr_w(upnp_gateway_connection.control_url),
            upnp_gateway_connection.version );

    update_mapping_list();
    return TRUE;
}

static BOOL grab_gateway_connection(void)
{
    AcquireSRWLockExclusive( &upnp_gateway_connection_lock );
    if (!upnp_gateway_connection.refs && !init_gateway_connection())
    {
        gateway_connection_cleanup();
        ReleaseSRWLockExclusive( &upnp_gateway_connection_lock );
        return FALSE;
    }
    ++upnp_gateway_connection.refs;
    ReleaseSRWLockExclusive( &upnp_gateway_connection_lock );
    return TRUE;
}

static void release_gateway_connection(void)
{
    AcquireSRWLockExclusive( &upnp_gateway_connection_lock );
    assert( upnp_gateway_connection.refs );
    if (!--upnp_gateway_connection.refs)
        gateway_connection_cleanup();
    ReleaseSRWLockExclusive( &upnp_gateway_connection_lock );
}

static BOOL find_port_mapping( LONG port, BSTR protocol, struct port_mapping *ret )
{
    unsigned int i;
    BOOL found;

    AcquireSRWLockExclusive( &upnp_gateway_connection_lock );
    for (i = 0; i < upnp_gateway_connection.mapping_count; ++i)
    {
        if (upnp_gateway_connection.mappings[i].external == port
            && !wcscmp( upnp_gateway_connection.mappings[i].protocol, protocol ))
            break;
    }
    found = i < upnp_gateway_connection.mapping_count;
    if (found) copy_port_mapping( ret, &upnp_gateway_connection.mappings[i] );
    ReleaseSRWLockExclusive( &upnp_gateway_connection_lock );
    return found;
}

static unsigned int get_port_mapping_range( unsigned int index, unsigned int count, struct port_mapping *ret )
{
    unsigned int i;

    AcquireSRWLockExclusive( &upnp_gateway_connection_lock );
    for (i = 0; i < count && index + i < upnp_gateway_connection.mapping_count; ++i)
        if (!copy_port_mapping( &ret[i], &upnp_gateway_connection.mappings[index + i] ))
        {
            ERR( "No memory.\n" );
            break;
        }
    ReleaseSRWLockExclusive( &upnp_gateway_connection_lock );

    return i;
}

static unsigned int get_port_mapping_count(void)
{
    unsigned int ret;

    AcquireSRWLockExclusive( &upnp_gateway_connection_lock );
    ret = upnp_gateway_connection.mapping_count;
    ReleaseSRWLockExclusive( &upnp_gateway_connection_lock );
    return ret;
}

static BOOL is_valid_protocol( BSTR protocol )
{
    if (!protocol) return FALSE;
    return !wcscmp( protocol, L"UDP" ) || !wcscmp( protocol, L"TCP" );
}

struct static_port_mapping
{
    IStaticPortMapping IStaticPortMapping_iface;
    LONG refs;
    struct port_mapping data;
};

static inline struct static_port_mapping *impl_from_IStaticPortMapping( IStaticPortMapping *iface )
{
    return CONTAINING_RECORD(iface, struct static_port_mapping, IStaticPortMapping_iface);
}

static ULONG WINAPI static_port_mapping_AddRef(
    IStaticPortMapping *iface )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );
    return InterlockedIncrement( &mapping->refs );
}

static ULONG WINAPI static_port_mapping_Release(
    IStaticPortMapping *iface )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );
    LONG refs = InterlockedDecrement( &mapping->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", mapping);
        free_port_mapping( &mapping->data );
        free( mapping );
    }
    return refs;
}

static HRESULT WINAPI static_port_mapping_QueryInterface(
    IStaticPortMapping *iface,
    REFIID riid,
    void **ppvObject )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );

    TRACE("%p %s %p\n", mapping, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IStaticPortMapping ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IStaticPortMapping_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI static_port_mapping_GetTypeInfoCount(
    IStaticPortMapping *iface,
    UINT *pctinfo )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );

    TRACE("%p %p\n", mapping, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI static_port_mapping_GetTypeInfo(
    IStaticPortMapping *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );

    TRACE("%p %u %lu %p\n", mapping, iTInfo, lcid, ppTInfo);
    return get_typeinfo( IStaticPortMapping_tid, ppTInfo );
}

static HRESULT WINAPI static_port_mapping_GetIDsOfNames(
    IStaticPortMapping *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %lu %p\n", mapping, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( IStaticPortMapping_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI static_port_mapping_Invoke(
    IStaticPortMapping *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %ld %s %ld %d %p %p %p %p\n", mapping, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( IStaticPortMapping_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &mapping->IStaticPortMapping_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI static_port_mapping_get_ExternalIPAddress(
    IStaticPortMapping *iface,
    BSTR *value )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    if (!value) return E_POINTER;
    *value = SysAllocString( mapping->data.external_ip );
    if (mapping->data.external_ip && !*value) return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI static_port_mapping_get_ExternalPort(
    IStaticPortMapping *iface,
    LONG *value )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    if (!value) return E_POINTER;
    *value = mapping->data.external;
    return S_OK;
}

static HRESULT WINAPI static_port_mapping_get_InternalPort(
    IStaticPortMapping *iface,
    LONG *value )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    if (!value) return E_POINTER;
    *value = mapping->data.internal;
    return S_OK;
}

static HRESULT WINAPI static_port_mapping_get_Protocol(
    IStaticPortMapping *iface,
    BSTR *value )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    if (!value) return E_POINTER;
    *value = SysAllocString( mapping->data.protocol );
    if (mapping->data.protocol && !*value) return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI static_port_mapping_get_InternalClient(
    IStaticPortMapping *iface,
    BSTR *value )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    if (!value) return E_POINTER;
    *value = SysAllocString( mapping->data.client );
    if (mapping->data.client && !*value) return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI static_port_mapping_get_Enabled(
    IStaticPortMapping *iface,
    VARIANT_BOOL *value )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    if (!value) return E_POINTER;
    *value = mapping->data.enabled;
    return S_OK;
}

static HRESULT WINAPI static_port_mapping_get_Description(
    IStaticPortMapping *iface,
    BSTR *value )
{
    struct static_port_mapping *mapping = impl_from_IStaticPortMapping( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    if (!value) return E_POINTER;
    *value = SysAllocString( mapping->data.descr );
    if (mapping->data.descr && !*value) return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI static_port_mapping_EditInternalClient(
    IStaticPortMapping *iface,
    BSTR value )
{
    FIXME( "iface %p, value %s stub.\n", iface, debugstr_w(value) );

    return E_NOTIMPL;
}

static HRESULT WINAPI static_port_mapping_Enable(
    IStaticPortMapping *iface,
    VARIANT_BOOL value )
{
    FIXME( "iface %p, value %d stub.\n", iface, value );

    return E_NOTIMPL;
}

static HRESULT WINAPI static_port_mapping_EditDescription(
    IStaticPortMapping *iface,
    BSTR value )
{
    FIXME( "iface %p, value %s stub.\n", iface, debugstr_w(value) );

    return E_NOTIMPL;
}

static HRESULT WINAPI static_port_mapping_EditInternalPort(
    IStaticPortMapping *iface,
    LONG value )
{
    FIXME( "iface %p, value %ld stub.\n", iface, value );

    return E_NOTIMPL;
}

static const IStaticPortMappingVtbl static_port_mapping_vtbl =
{
    static_port_mapping_QueryInterface,
    static_port_mapping_AddRef,
    static_port_mapping_Release,
    static_port_mapping_GetTypeInfoCount,
    static_port_mapping_GetTypeInfo,
    static_port_mapping_GetIDsOfNames,
    static_port_mapping_Invoke,
    static_port_mapping_get_ExternalIPAddress,
    static_port_mapping_get_ExternalPort,
    static_port_mapping_get_InternalPort,
    static_port_mapping_get_Protocol,
    static_port_mapping_get_InternalClient,
    static_port_mapping_get_Enabled,
    static_port_mapping_get_Description,
    static_port_mapping_EditInternalClient,
    static_port_mapping_Enable,
    static_port_mapping_EditDescription,
    static_port_mapping_EditInternalPort,
};

static HRESULT static_port_mapping_create( const struct port_mapping *mapping_data, IStaticPortMapping **ret )
{
    struct static_port_mapping *mapping;

    if (!(mapping = calloc( 1, sizeof(*mapping) ))) return E_OUTOFMEMORY;

    mapping->refs = 1;
    mapping->IStaticPortMapping_iface.lpVtbl = &static_port_mapping_vtbl;
    mapping->data = *mapping_data;
    *ret = &mapping->IStaticPortMapping_iface;
    return S_OK;
}

struct port_mapping_enum
{
    IEnumVARIANT IEnumVARIANT_iface;
    LONG refs;
    unsigned int index;
};

static inline struct port_mapping_enum *impl_from_IEnumVARIANT( IEnumVARIANT *iface )
{
    return CONTAINING_RECORD(iface, struct port_mapping_enum, IEnumVARIANT_iface);
}

static ULONG WINAPI port_mapping_enum_AddRef(
    IEnumVARIANT *iface )
{
    struct port_mapping_enum *mapping_enum = impl_from_IEnumVARIANT( iface );
    return InterlockedIncrement( &mapping_enum->refs );
}

static ULONG WINAPI port_mapping_enum_Release(
    IEnumVARIANT *iface )
{
    struct port_mapping_enum *mapping_enum = impl_from_IEnumVARIANT( iface );
    LONG refs = InterlockedDecrement( &mapping_enum->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", mapping_enum);
        free( mapping_enum );
        release_gateway_connection();
    }
    return refs;
}

static HRESULT WINAPI port_mapping_enum_QueryInterface(
    IEnumVARIANT *iface,
    REFIID riid,
    void **ppvObject )
{
    struct port_mapping_enum *mapping_enum = impl_from_IEnumVARIANT( iface );

    TRACE("%p %s %p\n", mapping_enum, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IEnumVARIANT ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IEnumVARIANT_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI port_mapping_enum_Next( IEnumVARIANT *iface, ULONG celt, VARIANT *var, ULONG *fetched )
{
    struct port_mapping_enum *mapping_enum = impl_from_IEnumVARIANT( iface );
    struct port_mapping *data;
    IStaticPortMapping *pm;
    unsigned int i, count;
    HRESULT ret;

    TRACE( "iface %p, celt %lu, var %p, fetched %p.\n", iface, celt, var, fetched );

    if (fetched) *fetched = 0;
    if (!celt) return S_OK;
    if (!var) return E_POINTER;

    if (!(data = calloc( 1, celt * sizeof(*data) ))) return E_OUTOFMEMORY;
    count = get_port_mapping_range( mapping_enum->index, celt, data );
    TRACE( "count %u.\n", count );
    for (i = 0; i < count; ++i)
    {
        if (FAILED(static_port_mapping_create( &data[i], &pm ))) break;

        V_VT(&var[i]) = VT_DISPATCH;
        V_DISPATCH(&var[i]) = (IDispatch *)pm;
    }
    mapping_enum->index += i;
    if (fetched) *fetched = i;
    ret = (i < celt) ? S_FALSE : S_OK;
    for (     ; i < count; ++i)
    {
        free_port_mapping( &data[i] );
        VariantInit( &var[i] );
    }
    for (     ; i < celt; ++i)
        VariantInit( &var[i] );

    free( data );
    return ret;
}

static HRESULT WINAPI port_mapping_enum_Skip( IEnumVARIANT *iface, ULONG celt )
{
    struct port_mapping_enum *mapping_enum = impl_from_IEnumVARIANT( iface );
    unsigned int count = get_port_mapping_count();

    TRACE( "iface %p, celt %lu.\n", iface, celt );

    mapping_enum->index += celt;
    return mapping_enum->index <= count ? S_OK : S_FALSE;
}

static HRESULT WINAPI port_mapping_enum_Reset( IEnumVARIANT *iface )
{
    struct port_mapping_enum *mapping_enum = impl_from_IEnumVARIANT( iface );

    TRACE( "iface %p.\n", iface );

    mapping_enum->index = 0;
    return S_OK;
}

static HRESULT create_port_mapping_enum( IUnknown **ret );

static HRESULT WINAPI port_mapping_enum_Clone( IEnumVARIANT *iface, IEnumVARIANT **ret )
{
    struct port_mapping_enum *mapping_enum = impl_from_IEnumVARIANT( iface );
    HRESULT hr;

    TRACE( "iface %p, ret %p.\n", iface, ret );

    if (!ret) return E_POINTER;
    *ret = NULL;
    if (FAILED(hr = create_port_mapping_enum( (IUnknown **)ret ))) return hr;
    impl_from_IEnumVARIANT( *ret )->index = mapping_enum->index;
    return S_OK;
}

static const IEnumVARIANTVtbl port_mapping_enum_vtbl =
{
    port_mapping_enum_QueryInterface,
    port_mapping_enum_AddRef,
    port_mapping_enum_Release,
    port_mapping_enum_Next,
    port_mapping_enum_Skip,
    port_mapping_enum_Reset,
    port_mapping_enum_Clone,
};

static HRESULT create_port_mapping_enum( IUnknown **ret )
{
    struct port_mapping_enum *mapping_enum;

    if (!(mapping_enum = calloc( 1, sizeof(*mapping_enum) ))) return E_OUTOFMEMORY;

    grab_gateway_connection();

    mapping_enum->refs = 1;
    mapping_enum->IEnumVARIANT_iface.lpVtbl = &port_mapping_enum_vtbl;
    mapping_enum->index = 0;
    *ret = (IUnknown *)&mapping_enum->IEnumVARIANT_iface;
    return S_OK;
}

struct static_port_mapping_collection
{
    IStaticPortMappingCollection IStaticPortMappingCollection_iface;
    LONG refs;
};

static inline struct static_port_mapping_collection *impl_from_IStaticPortMappingCollection
                                                     ( IStaticPortMappingCollection *iface )
{
    return CONTAINING_RECORD(iface, struct static_port_mapping_collection, IStaticPortMappingCollection_iface);
}

static ULONG WINAPI static_ports_AddRef(
    IStaticPortMappingCollection *iface )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );
    return InterlockedIncrement( &ports->refs );
}

static ULONG WINAPI static_ports_Release(
    IStaticPortMappingCollection *iface )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );
    LONG refs = InterlockedDecrement( &ports->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", ports);
        release_gateway_connection();
        free( ports );
    }
    return refs;
}

static HRESULT WINAPI static_ports_QueryInterface(
    IStaticPortMappingCollection *iface,
    REFIID riid,
    void **ppvObject )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );

    TRACE("%p %s %p\n", ports, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IStaticPortMappingCollection ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IStaticPortMappingCollection_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI static_ports_GetTypeInfoCount(
    IStaticPortMappingCollection *iface,
    UINT *pctinfo )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );

    TRACE("%p %p\n", ports, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI static_ports_GetTypeInfo(
    IStaticPortMappingCollection *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );

    TRACE("%p %u %lu %p\n", ports, iTInfo, lcid, ppTInfo);
    return get_typeinfo( IStaticPortMappingCollection_tid, ppTInfo );
}

static HRESULT WINAPI static_ports_GetIDsOfNames(
    IStaticPortMappingCollection *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %lu %p\n", ports, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( IStaticPortMappingCollection_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI static_ports_Invoke(
    IStaticPortMappingCollection *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %ld %s %ld %d %p %p %p %p\n", ports, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( IStaticPortMappingCollection_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &ports->IStaticPortMappingCollection_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI static_ports__NewEnum(
    IStaticPortMappingCollection *iface,
    IUnknown **ret )
{
    TRACE( "iface %p, ret %p.\n", iface, ret );

    if (!ret) return E_POINTER;

    *ret = NULL;
    return create_port_mapping_enum( ret );
}

static HRESULT WINAPI static_ports_get_Item(
    IStaticPortMappingCollection *iface,
    LONG port,
    BSTR protocol,
    IStaticPortMapping **mapping )
{
    struct port_mapping mapping_data;
    HRESULT ret;

    TRACE( "iface %p, port %ld, protocol %s.\n", iface, port, debugstr_w(protocol) );

    if (!mapping) return E_POINTER;
    *mapping = NULL;
    if (!is_valid_protocol( protocol )) return E_INVALIDARG;
    if (port < 0 || port > 65535) return E_INVALIDARG;

    if (!find_port_mapping( port, protocol, &mapping_data )) return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    if (FAILED(ret = static_port_mapping_create( &mapping_data, mapping ))) free_port_mapping( &mapping_data );
    return ret;
}

static HRESULT WINAPI static_ports_get_Count(
    IStaticPortMappingCollection *iface,
    LONG *count )
{
    TRACE( "iface %p, count %p.\n", iface, count );

    if (!count) return E_POINTER;
    *count = get_port_mapping_count();
    return S_OK;
}

static HRESULT WINAPI static_ports_Remove(
    IStaticPortMappingCollection *iface,
    LONG port,
    BSTR protocol )
{
    TRACE( "iface %p, port %ld, protocol %s.\n", iface, port, debugstr_w(protocol) );

    if (!is_valid_protocol( protocol )) return E_INVALIDARG;
    if (port < 0 || port > 65535) return E_INVALIDARG;

    if (!remove_port_mapping( port, protocol )) return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI static_ports_Add(
    IStaticPortMappingCollection *iface,
    LONG external,
    BSTR protocol,
    LONG internal,
    BSTR client,
    VARIANT_BOOL enabled,
    BSTR description,
    IStaticPortMapping **mapping )
{
    struct port_mapping mapping_data;
    HRESULT ret;

    TRACE( "iface %p, external %ld, protocol %s, internal %ld, client %s, enabled %d, description %s, mapping %p.\n",
           iface, external, debugstr_w(protocol), internal, debugstr_w(client), enabled, debugstr_w(description),
           mapping );

    if (!mapping) return E_POINTER;
    *mapping = NULL;

    if (!is_valid_protocol( protocol )) return E_INVALIDARG;
    if (external < 0 || external > 65535) return E_INVALIDARG;
    if (internal < 0 || internal > 65535) return E_INVALIDARG;
    if (!client || !description) return E_INVALIDARG;

    if (!add_port_mapping( external, protocol, internal, client, enabled, description )) return E_FAIL;

    mapping_data.external_ip = NULL;
    mapping_data.external = external;
    mapping_data.protocol = SysAllocString( protocol );
    mapping_data.internal = internal;
    mapping_data.client = SysAllocString( client );
    mapping_data.enabled = enabled;
    mapping_data.descr = SysAllocString( description );
    if (!mapping_data.protocol || !mapping_data.client || !mapping_data.descr)
    {
        free_port_mapping( &mapping_data );
        return E_OUTOFMEMORY;
    }
    if (FAILED(ret = static_port_mapping_create( &mapping_data, mapping ))) free_port_mapping( &mapping_data );
    return ret;
}

static const IStaticPortMappingCollectionVtbl static_ports_vtbl =
{
    static_ports_QueryInterface,
    static_ports_AddRef,
    static_ports_Release,
    static_ports_GetTypeInfoCount,
    static_ports_GetTypeInfo,
    static_ports_GetIDsOfNames,
    static_ports_Invoke,
    static_ports__NewEnum,
    static_ports_get_Item,
    static_ports_get_Count,
    static_ports_Remove,
    static_ports_Add,
};

static HRESULT static_port_mapping_collection_create(IStaticPortMappingCollection **object)
{
    struct static_port_mapping_collection *ports;

    if (!object) return E_POINTER;
    if (!grab_gateway_connection())
    {
        *object = NULL;
        return S_OK;
    }
    if (!(ports = calloc( 1, sizeof(*ports) )))
    {
        release_gateway_connection();
        return E_OUTOFMEMORY;
    }
    ports->refs = 1;
    ports->IStaticPortMappingCollection_iface.lpVtbl = &static_ports_vtbl;
    *object = &ports->IStaticPortMappingCollection_iface;
    return S_OK;
}

typedef struct fw_port
{
    INetFwOpenPort INetFwOpenPort_iface;
    LONG refs;
    BSTR name;
    NET_FW_IP_PROTOCOL protocol;
    LONG port;
} fw_port;

static inline fw_port *impl_from_INetFwOpenPort( INetFwOpenPort *iface )
{
    return CONTAINING_RECORD(iface, fw_port, INetFwOpenPort_iface);
}

static ULONG WINAPI fw_port_AddRef(
    INetFwOpenPort *iface )
{
    fw_port *fw_port = impl_from_INetFwOpenPort( iface );
    return InterlockedIncrement( &fw_port->refs );
}

static ULONG WINAPI fw_port_Release(
    INetFwOpenPort *iface )
{
    fw_port *fw_port = impl_from_INetFwOpenPort( iface );
    LONG refs = InterlockedDecrement( &fw_port->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_port);
        SysFreeString( fw_port->name );
        free( fw_port );
    }
    return refs;
}

static HRESULT WINAPI fw_port_QueryInterface(
    INetFwOpenPort *iface,
    REFIID riid,
    void **ppvObject )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_INetFwOpenPort ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    INetFwOpenPort_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fw_port_GetTypeInfoCount(
    INetFwOpenPort *iface,
    UINT *pctinfo )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %p\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI fw_port_GetTypeInfo(
    INetFwOpenPort *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %u %lu %p\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo( INetFwOpenPort_tid, ppTInfo );
}

static HRESULT WINAPI fw_port_GetIDsOfNames(
    INetFwOpenPort *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %lu %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( INetFwOpenPort_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_port_Invoke(
    INetFwOpenPort *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %ld %s %ld %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( INetFwOpenPort_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &This->INetFwOpenPort_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_port_get_Name(
    INetFwOpenPort *iface,
    BSTR *name)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_Name(
    INetFwOpenPort *iface,
    BSTR name)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %s\n", This, debugstr_w(name));

    if (!(name = SysAllocString( name )))
        return E_OUTOFMEMORY;

    SysFreeString( This->name );
    This->name = name;
    return S_OK;
}

static HRESULT WINAPI fw_port_get_IpVersion(
    INetFwOpenPort *iface,
    NET_FW_IP_VERSION *ipVersion)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, ipVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_IpVersion(
    INetFwOpenPort *iface,
    NET_FW_IP_VERSION ipVersion)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %u\n", This, ipVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_get_Protocol(
    INetFwOpenPort *iface,
    NET_FW_IP_PROTOCOL *ipProtocol)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, ipProtocol);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_Protocol(
    INetFwOpenPort *iface,
    NET_FW_IP_PROTOCOL ipProtocol)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %u\n", This, ipProtocol);

    if (ipProtocol != NET_FW_IP_PROTOCOL_TCP && ipProtocol != NET_FW_IP_PROTOCOL_UDP)
        return E_INVALIDARG;

    This->protocol = ipProtocol;
    return S_OK;
}

static HRESULT WINAPI fw_port_get_Port(
    INetFwOpenPort *iface,
    LONG *portNumber)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, portNumber);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_Port(
    INetFwOpenPort *iface,
    LONG portNumber)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %ld\n", This, portNumber);
    This->port = portNumber;
    return S_OK;
}

static HRESULT WINAPI fw_port_get_Scope(
    INetFwOpenPort *iface,
    NET_FW_SCOPE *scope)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, scope);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_Scope(
    INetFwOpenPort *iface,
    NET_FW_SCOPE scope)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %u\n", This, scope);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_get_RemoteAddresses(
    INetFwOpenPort *iface,
    BSTR *remoteAddrs)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, remoteAddrs);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_RemoteAddresses(
    INetFwOpenPort *iface,
    BSTR remoteAddrs)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %s\n", This, debugstr_w(remoteAddrs));
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_get_Enabled(
    INetFwOpenPort *iface,
    VARIANT_BOOL *enabled)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, enabled);

    *enabled = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI fw_port_put_Enabled(
    INetFwOpenPort *iface,
    VARIANT_BOOL enabled)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %d\n", This, enabled);
    return S_OK;
}

static HRESULT WINAPI fw_port_get_BuiltIn(
    INetFwOpenPort *iface,
    VARIANT_BOOL *builtIn)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, builtIn);
    return E_NOTIMPL;
}

static const struct INetFwOpenPortVtbl fw_port_vtbl =
{
    fw_port_QueryInterface,
    fw_port_AddRef,
    fw_port_Release,
    fw_port_GetTypeInfoCount,
    fw_port_GetTypeInfo,
    fw_port_GetIDsOfNames,
    fw_port_Invoke,
    fw_port_get_Name,
    fw_port_put_Name,
    fw_port_get_IpVersion,
    fw_port_put_IpVersion,
    fw_port_get_Protocol,
    fw_port_put_Protocol,
    fw_port_get_Port,
    fw_port_put_Port,
    fw_port_get_Scope,
    fw_port_put_Scope,
    fw_port_get_RemoteAddresses,
    fw_port_put_RemoteAddresses,
    fw_port_get_Enabled,
    fw_port_put_Enabled,
    fw_port_get_BuiltIn
};

HRESULT NetFwOpenPort_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    fw_port *fp;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fp = malloc( sizeof(*fp) );
    if (!fp) return E_OUTOFMEMORY;

    fp->INetFwOpenPort_iface.lpVtbl = &fw_port_vtbl;
    fp->refs = 1;
    fp->name = NULL;
    fp->protocol = NET_FW_IP_PROTOCOL_TCP;
    fp->port = 0;

    *ppObj = &fp->INetFwOpenPort_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

typedef struct fw_ports
{
    INetFwOpenPorts INetFwOpenPorts_iface;
    LONG refs;
} fw_ports;

static inline fw_ports *impl_from_INetFwOpenPorts( INetFwOpenPorts *iface )
{
    return CONTAINING_RECORD(iface, fw_ports, INetFwOpenPorts_iface);
}

static ULONG WINAPI fw_ports_AddRef(
    INetFwOpenPorts *iface )
{
    fw_ports *fw_ports = impl_from_INetFwOpenPorts( iface );
    return InterlockedIncrement( &fw_ports->refs );
}

static ULONG WINAPI fw_ports_Release(
    INetFwOpenPorts *iface )
{
    fw_ports *fw_ports = impl_from_INetFwOpenPorts( iface );
    LONG refs = InterlockedDecrement( &fw_ports->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_ports);
        free( fw_ports );
    }
    return refs;
}

static HRESULT WINAPI fw_ports_QueryInterface(
    INetFwOpenPorts *iface,
    REFIID riid,
    void **ppvObject )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_INetFwOpenPorts ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    INetFwOpenPorts_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fw_ports_GetTypeInfoCount(
    INetFwOpenPorts *iface,
    UINT *pctinfo )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    TRACE("%p %p\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI fw_ports_GetTypeInfo(
    INetFwOpenPorts *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    TRACE("%p %u %lu %p\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo( INetFwOpenPorts_tid, ppTInfo );
}

static HRESULT WINAPI fw_ports_GetIDsOfNames(
    INetFwOpenPorts *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %lu %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( INetFwOpenPorts_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_ports_Invoke(
    INetFwOpenPorts *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %ld %s %ld %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( INetFwOpenPorts_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &This->INetFwOpenPorts_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_ports_get_Count(
    INetFwOpenPorts *iface,
    LONG *count)
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %p\n", This, count);

    *count = 0;
    return S_OK;
}

static HRESULT WINAPI fw_ports_Add(
    INetFwOpenPorts *iface,
    INetFwOpenPort *port)
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %p\n", This, port);
    return S_OK;
}

static HRESULT WINAPI fw_ports_Remove(
    INetFwOpenPorts *iface,
    LONG portNumber,
    NET_FW_IP_PROTOCOL ipProtocol)
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %ld, %u\n", This, portNumber, ipProtocol);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_ports_Item(
    INetFwOpenPorts *iface,
    LONG portNumber,
    NET_FW_IP_PROTOCOL ipProtocol,
    INetFwOpenPort **openPort)
{
    HRESULT hr;
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %ld, %u, %p\n", This, portNumber, ipProtocol, openPort);

    hr = NetFwOpenPort_create( NULL, (void **)openPort );
    if (SUCCEEDED(hr))
    {
        INetFwOpenPort_put_Protocol( *openPort, ipProtocol );
        INetFwOpenPort_put_Port( *openPort, portNumber );
    }
    return hr;
}

static HRESULT WINAPI fw_ports_get__NewEnum(
    INetFwOpenPorts *iface,
    IUnknown **newEnum)
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %p\n", This, newEnum);
    return E_NOTIMPL;
}

static const struct INetFwOpenPortsVtbl fw_ports_vtbl =
{
    fw_ports_QueryInterface,
    fw_ports_AddRef,
    fw_ports_Release,
    fw_ports_GetTypeInfoCount,
    fw_ports_GetTypeInfo,
    fw_ports_GetIDsOfNames,
    fw_ports_Invoke,
    fw_ports_get_Count,
    fw_ports_Add,
    fw_ports_Remove,
    fw_ports_Item,
    fw_ports_get__NewEnum
};

HRESULT NetFwOpenPorts_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    fw_ports *fp;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fp = malloc( sizeof(*fp) );
    if (!fp) return E_OUTOFMEMORY;

    fp->INetFwOpenPorts_iface.lpVtbl = &fw_ports_vtbl;
    fp->refs = 1;

    *ppObj = &fp->INetFwOpenPorts_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

typedef struct _upnpnat
{
    IUPnPNAT IUPnPNAT_iface;
    LONG ref;
} upnpnat;

static inline upnpnat *impl_from_IUPnPNAT( IUPnPNAT *iface )
{
    return CONTAINING_RECORD(iface, upnpnat, IUPnPNAT_iface);
}

static HRESULT WINAPI upnpnat_QueryInterface(IUPnPNAT *iface, REFIID riid, void **object)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), object );

    if ( IsEqualGUID( riid, &IID_IUPnPNAT ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *object = iface;
    }
    else if(IsEqualGUID( riid, &IID_IProvideClassInfo))
    {
        TRACE("IProvideClassInfo not supported.\n");
        return E_NOINTERFACE;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IUPnPNAT_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI upnpnat_AddRef(IUPnPNAT *iface)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI upnpnat_Release(IUPnPNAT *iface)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );
    LONG refs = InterlockedDecrement( &This->ref );
    if (!refs)
    {
        free( This );
    }
    return refs;
}

static HRESULT WINAPI upnpnat_GetTypeInfoCount(IUPnPNAT *iface, UINT *pctinfo)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );

    TRACE("%p %p\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI upnpnat_GetTypeInfo(IUPnPNAT *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );

    TRACE("%p %u %lu %p\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo( IUPnPNAT_tid, ppTInfo );
}

static HRESULT WINAPI upnpnat_GetIDsOfNames(IUPnPNAT *iface, REFIID riid, LPOLESTR *rgszNames,
                UINT cNames, LCID lcid, DISPID *rgDispId)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %lu %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( IUPnPNAT_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI upnpnat_Invoke(IUPnPNAT *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
                WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                UINT *puArgErr)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %ld %s %ld %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( IUPnPNAT_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &This->IUPnPNAT_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI upnpnat_get_StaticPortMappingCollection(IUPnPNAT *iface, IStaticPortMappingCollection **collection)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );

    TRACE("%p, %p\n", This, collection);

    return static_port_mapping_collection_create( collection );
}

static HRESULT WINAPI upnpnat_get_DynamicPortMappingCollection(IUPnPNAT *iface, IDynamicPortMappingCollection **collection)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );
    FIXME("%p, %p\n", This, collection);
    if(collection)
        *collection = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI upnpnat_get_NATEventManager(IUPnPNAT *iface, INATEventManager **manager)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );
    FIXME("%p, %p\n", This, manager);
    if(manager)
        *manager = NULL;
    return E_NOTIMPL;
}

const static IUPnPNATVtbl upnpnat_vtbl =
{
    upnpnat_QueryInterface,
    upnpnat_AddRef,
    upnpnat_Release,
    upnpnat_GetTypeInfoCount,
    upnpnat_GetTypeInfo,
    upnpnat_GetIDsOfNames,
    upnpnat_Invoke,
    upnpnat_get_StaticPortMappingCollection,
    upnpnat_get_DynamicPortMappingCollection,
    upnpnat_get_NATEventManager
};


HRESULT IUPnPNAT_create(IUnknown *outer, void **object)
{
    upnpnat *nat;

    TRACE("(%p,%p)\n", outer, object);

    nat = malloc( sizeof(*nat) );
    if (!nat) return E_OUTOFMEMORY;

    nat->IUPnPNAT_iface.lpVtbl = &upnpnat_vtbl;
    nat->ref = 1;

    *object = &nat->IUPnPNAT_iface;

    TRACE("returning iface %p\n", *object);
    return S_OK;
}
