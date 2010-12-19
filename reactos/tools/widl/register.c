/*
 * Generation of dll registration scripts
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
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <ctype.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typegen.h"

static int indent;

static const char *format_uuid( const UUID *uuid )
{
    static char buffer[40];
    sprintf( buffer, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
             uuid->Data1, uuid->Data2, uuid->Data3,
             uuid->Data4[0], uuid->Data4[1], uuid->Data4[2], uuid->Data4[3],
             uuid->Data4[4], uuid->Data4[5], uuid->Data4[6], uuid->Data4[7] );
    return buffer;
}

static const char *get_coclass_threading( const type_t *class )
{
    static const char * const models[] =
    {
        NULL,
        "Apartment", /* THREADING_APARTMENT */
        "Neutral",   /* THREADING_NEUTRAL */
        "Single",    /* THREADING_SINGLE */
        "Free",      /* THREADING_FREE */
        "Both",      /* THREADING_BOTH */
    };
    return models[get_attrv( class->attrs, ATTR_THREADING )];
}

static int write_interface( const type_t *iface )
{
    const UUID *uuid = get_attrp( iface->attrs, ATTR_UUID );

    if (!uuid) return 0;
    if (!is_object( iface )) return 0;
    if (!type_iface_get_inherit(iface)) /* special case for IUnknown */
    {
        put_str( indent, "ForceRemove '%s' = s '%s'\n", format_uuid( uuid ), iface->name );
        return 0;
    }
    if (is_local( iface->attrs )) return 0;
    put_str( indent, "ForceRemove '%s' = s '%s'\n", format_uuid( uuid ), iface->name );
    put_str( indent, "{\n" );
    indent++;
    put_str( indent, "NumMethods = s %u\n", count_methods( iface ));
    put_str( indent, "ProxyStubClsid32 = s '%%CLSID_PSFactoryBuffer%%'\n" );
    indent--;
    put_str( indent, "}\n" );
    return 1;
}

static int write_interfaces( const statement_list_t *stmts )
{
    const statement_t *stmt;
    int count = 0;

    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type == STMT_TYPE && type_get_type( stmt->u.type ) == TYPE_INTERFACE)
            count += write_interface( stmt->u.type );
    }
    return count;
}

static int write_coclass( const type_t *class, const typelib_t *typelib )
{
    const UUID *uuid = get_attrp( class->attrs, ATTR_UUID );
    const char *descr = get_attrp( class->attrs, ATTR_HELPSTRING );
    const char *progid = get_attrp( class->attrs, ATTR_PROGID );
    const char *vi_progid = get_attrp( class->attrs, ATTR_VIPROGID );
    const char *threading = get_coclass_threading( class );

    if (!uuid) return 0;
    if (typelib && !threading) return 0;
    if (!descr) descr = class->name;

    put_str( indent, "ForceRemove '%s' = s '%s'\n", format_uuid( uuid ), descr );
    put_str( indent++, "{\n" );
    if (threading) put_str( indent, "InprocServer32 = s '%%MODULE%%' { val ThreadingModel = s '%s' }\n",
                            threading );
    if (progid) put_str( indent, "ProgId = s '%s'\n", progid );
    if (typelib)
    {
        const UUID *typelib_uuid = get_attrp( typelib->attrs, ATTR_UUID );
        const unsigned int version = get_attrv( typelib->attrs, ATTR_VERSION );
        put_str( indent, "TypeLib = s '%s'\n", format_uuid( typelib_uuid ));
        put_str( indent, "Version = s '%u.%u'\n", MAJORVERSION(version), MINORVERSION(version) );
    }
    if (vi_progid) put_str( indent, "VersionIndependentProgId = s '%s'\n", vi_progid );
    put_str( --indent, "}\n" );
    return 1;
}

static void write_coclasses( const statement_list_t *stmts, const typelib_t *typelib )
{
    const statement_t *stmt;

    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type == STMT_TYPE)
        {
            const type_t *type = stmt->u.type;
            if (type_get_type(type) == TYPE_COCLASS) write_coclass( type, typelib );
        }
        else if (stmt->type == STMT_LIBRARY)
        {
            const typelib_t *lib = stmt->u.lib;
            write_coclasses( lib->stmts, lib );
        }
    }
}

static int write_progid( const type_t *class )
{
    const UUID *uuid = get_attrp( class->attrs, ATTR_UUID );
    const char *descr = get_attrp( class->attrs, ATTR_HELPSTRING );
    const char *progid = get_attrp( class->attrs, ATTR_PROGID );
    const char *vi_progid = get_attrp( class->attrs, ATTR_VIPROGID );

    if (!uuid) return 0;
    if (!descr) descr = class->name;

    if (progid)
    {
        put_str( indent, "'%s' = s '%s'\n", progid, descr );
        put_str( indent++, "{\n" );
        put_str( indent, "CLSID = s '%s'\n", format_uuid( uuid ) );
        put_str( --indent, "}\n" );
    }
    if (vi_progid)
    {
        put_str( indent, "'%s' = s '%s'\n", vi_progid, descr );
        put_str( indent++, "{\n" );
        put_str( indent, "CLSID = s '%s'\n", format_uuid( uuid ) );
        if (progid && strcmp( progid, vi_progid )) put_str( indent, "CurVer = s '%s'\n", progid );
        put_str( --indent, "}\n" );
    }
    return 1;
}

static void write_progids( const statement_list_t *stmts )
{
    const statement_t *stmt;

    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type == STMT_TYPE)
        {
            const type_t *type = stmt->u.type;
            if (type_get_type(type) == TYPE_COCLASS) write_progid( type );
        }
        else if (stmt->type == STMT_LIBRARY)
        {
            write_progids( stmt->u.lib->stmts );
        }
    }
}

/* put a string into the resource file */
static inline void put_string( const char *str )
{
    while (*str)
    {
        unsigned char ch = *str++;
        put_word( toupper(ch) );
    }
    put_word( 0 );
}

void write_regscript( const statement_list_t *stmts )
{
    int count;

    if (!do_regscript) return;
    if (do_everything && !need_proxy_file( stmts )) return;

    init_output_buffer();

    put_str( indent, "HKCR\n" );
    put_str( indent++, "{\n" );

    put_str( indent, "NoRemove Interface\n" );
    put_str( indent++, "{\n" );
    count = write_interfaces( stmts );
    put_str( --indent, "}\n" );

    put_str( indent, "NoRemove CLSID\n" );
    put_str( indent++, "{\n" );
    if (count)
    {
        put_str( indent, "ForceRemove '%%CLSID_PSFactoryBuffer%%' = s 'PSFactoryBuffer'\n" );
        put_str( indent++, "{\n" );
        put_str( indent, "InprocServer32 = s '%%MODULE%%' { val ThreadingModel = s 'Both' }\n" );
        put_str( --indent, "}\n" );
    }
    write_coclasses( stmts, NULL );
    put_str( --indent, "}\n" );

    write_progids( stmts );
    put_str( --indent, "}\n" );

    if (strendswith( regscript_name, ".res" ))  /* create a binary resource file */
    {
        unsigned char *data = output_buffer;
        size_t data_size = output_buffer_pos;
        size_t header_size = 5 * sizeof(unsigned int) + 2 * sizeof(unsigned short);

        header_size += (strlen(regscript_token) + strlen("WINE_REGISTRY") + 2) * sizeof(unsigned short);

        init_output_buffer();

        put_dword( 0 );      /* ResSize */
        put_dword( 32 );     /* HeaderSize */
        put_word( 0xffff );  /* ResType */
        put_word( 0x0000 );
        put_word( 0xffff );  /* ResName */
        put_word( 0x0000 );
        put_dword( 0 );      /* DataVersion */
        put_word( 0 );       /* Memory options */
        put_word( 0 );       /* Language */
        put_dword( 0 );      /* Version */
        put_dword( 0 );      /* Characteristics */

        put_dword( data_size );               /* ResSize */
        put_dword( (header_size + 3) & ~3 );  /* HeaderSize */
        put_string( "WINE_REGISTRY" );        /* ResType */
        put_string( regscript_token );        /* ResName */
        align_output( 4 );
        put_dword( 0 );      /* DataVersion */
        put_word( 0 );       /* Memory options */
        put_word( 0 );       /* Language */
        put_dword( 0 );      /* Version */
        put_dword( 0 );      /* Characteristics */

        put_data( data, data_size );
        free( data );
        align_output( 4 );
        flush_output_buffer( regscript_name );
    }
    else
    {
        FILE *f = fopen( regscript_name, "w" );
        if (!f) error( "Could not open %s for output\n", regscript_name );
        if (fwrite( output_buffer, output_buffer_pos, 1, f ) != output_buffer_pos)
            error( "Failed to write to %s\n", regscript_name );
        if (fclose( f ))
            error( "Failed to write to %s\n", regscript_name );
    }
}
