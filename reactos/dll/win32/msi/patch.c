/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2004,2005 Aric Stewart for CodeWeavers
 * Copyright 2011 Hans Leidekker for CodeWeavers
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

#include "msipriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static BOOL match_language( MSIPACKAGE *package, LANGID langid )
{
    UINT i;

    if (!package->num_langids || !langid) return TRUE;
    for (i = 0; i < package->num_langids; i++)
    {
        if (package->langids[i] == langid) return TRUE;
    }
    return FALSE;
}

struct transform_desc
{
    WCHAR *product_code_from;
    WCHAR *product_code_to;
    WCHAR *version_from;
    WCHAR *version_to;
    WCHAR *upgrade_code;
};

static void free_transform_desc( struct transform_desc *desc )
{
    msi_free( desc->product_code_from );
    msi_free( desc->product_code_to );
    msi_free( desc->version_from );
    msi_free( desc->version_to );
    msi_free( desc->upgrade_code );
    msi_free( desc );
}

static struct transform_desc *parse_transform_desc( const WCHAR *str )
{
    struct transform_desc *ret;
    const WCHAR *p = str, *q;
    UINT len;

    if (!(ret = msi_alloc_zero( sizeof(*ret) ))) return NULL;

    q = strchrW( p, '}' );
    if (*p != '{' || !q) goto error;

    len = q - p + 1;
    if (!(ret->product_code_from = msi_alloc( (len + 1) * sizeof(WCHAR) ))) goto error;
    memcpy( ret->product_code_from, p, len * sizeof(WCHAR) );
    ret->product_code_from[len] = 0;

    p = q + 1;
    if (!(q = strchrW( p, ';' ))) goto error;
    len = q - p;
    if (!(ret->version_from = msi_alloc( (len + 1) * sizeof(WCHAR) ))) goto error;
    memcpy( ret->version_from, p, len * sizeof(WCHAR) );
    ret->version_from[len] = 0;

    p = q + 1;
    q = strchrW( p, '}' );
    if (*p != '{' || !q) goto error;

    len = q - p + 1;
    if (!(ret->product_code_to = msi_alloc( (len + 1) * sizeof(WCHAR) ))) goto error;
    memcpy( ret->product_code_to, p, len * sizeof(WCHAR) );
    ret->product_code_to[len] = 0;

    p = q + 1;
    if (!(q = strchrW( p, ';' ))) goto error;
    len = q - p;
    if (!(ret->version_to = msi_alloc( (len + 1) * sizeof(WCHAR) ))) goto error;
    memcpy( ret->version_to, p, len * sizeof(WCHAR) );
    ret->version_to[len] = 0;

    p = q + 1;
    q = strchrW( p, '}' );
    if (*p != '{' || !q) goto error;

    len = q - p + 1;
    if (!(ret->upgrade_code = msi_alloc( (len + 1) * sizeof(WCHAR) ))) goto error;
    memcpy( ret->upgrade_code, p, len * sizeof(WCHAR) );
    ret->upgrade_code[len] = 0;

    return ret;

error:
    free_transform_desc( ret );
    return NULL;
}

static UINT check_transform_applicable( MSIPACKAGE *package, IStorage *transform )
{
    static const UINT supported_flags =
        MSITRANSFORM_VALIDATE_PRODUCT  | MSITRANSFORM_VALIDATE_LANGUAGE |
        MSITRANSFORM_VALIDATE_PLATFORM | MSITRANSFORM_VALIDATE_MAJORVERSION |
        MSITRANSFORM_VALIDATE_MINORVERSION | MSITRANSFORM_VALIDATE_UPGRADECODE;
    MSISUMMARYINFO *si;
    UINT r, valid_flags = 0, wanted_flags = 0;
    WCHAR *template, *product, *p;
    struct transform_desc *desc;

    r = msi_get_suminfo( transform, 0, &si );
    if (r != ERROR_SUCCESS)
    {
        WARN("no summary information!\n");
        return r;
    }
    wanted_flags = msi_suminfo_get_int32( si, PID_CHARCOUNT );
    wanted_flags &= 0xffff; /* mask off error condition flags */
    TRACE("validation flags 0x%04x\n", wanted_flags);

    if (wanted_flags & ~supported_flags)
    {
        FIXME("unsupported validation flags 0x%04x\n", wanted_flags);
        msiobj_release( &si->hdr );
        return ERROR_FUNCTION_FAILED;
    }
    if (!(template = msi_suminfo_dup_string( si, PID_TEMPLATE )))
    {
        WARN("no template property!\n");
        msiobj_release( &si->hdr );
        return ERROR_FUNCTION_FAILED;
    }
    TRACE("template property: %s\n", debugstr_w(template));
    if (!(product = msi_get_suminfo_product( transform )))
    {
        WARN("no product property!\n");
        msi_free( template );
        msiobj_release( &si->hdr );
        return ERROR_FUNCTION_FAILED;
    }
    TRACE("product property: %s\n", debugstr_w(product));
    if (!(desc = parse_transform_desc( product )))
    {
        msi_free( template );
        msiobj_release( &si->hdr );
        return ERROR_FUNCTION_FAILED;
    }
    msi_free( product );

    if (wanted_flags & MSITRANSFORM_VALIDATE_LANGUAGE)
    {
        if (!template[0] || ((p = strchrW( template, ';' )) && match_language( package, atoiW( p + 1 ) )))
        {
            valid_flags |= MSITRANSFORM_VALIDATE_LANGUAGE;
        }
    }
    if (wanted_flags & MSITRANSFORM_VALIDATE_PRODUCT)
    {
        WCHAR *product_code_installed = msi_dup_property( package->db, szProductCode );

        if (!product_code_installed)
        {
            msi_free( template );
            free_transform_desc( desc );
            msiobj_release( &si->hdr );
            return ERROR_INSTALL_PACKAGE_INVALID;
        }
        if (!strcmpW( desc->product_code_from, product_code_installed ))
        {
            valid_flags |= MSITRANSFORM_VALIDATE_PRODUCT;
        }
        msi_free( product_code_installed );
    }
    if (wanted_flags & MSITRANSFORM_VALIDATE_PLATFORM)
    {
        if ((p = strchrW( template, ';' )))
        {
            *p = 0;
            if (package->platform == parse_platform( template ))
                valid_flags |= MSITRANSFORM_VALIDATE_PLATFORM;
        }
    }
    msi_free( template );
    if (wanted_flags & MSITRANSFORM_VALIDATE_MAJORVERSION)
    {
        WCHAR *product_version_installed = msi_dup_property( package->db, szProductVersion );
        DWORD major_installed, minor_installed, major, minor;

        if (!product_version_installed)
        {
            free_transform_desc( desc );
            msiobj_release( &si->hdr );
            return ERROR_INSTALL_PACKAGE_INVALID;
        }
        msi_parse_version_string( product_version_installed, &major_installed, &minor_installed );
        msi_parse_version_string( desc->version_from, &major, &minor );

        if (major_installed == major)
        {
            valid_flags |= MSITRANSFORM_VALIDATE_MAJORVERSION;
            wanted_flags &= ~MSITRANSFORM_VALIDATE_MINORVERSION;
        }
        msi_free( product_version_installed );
    }
    else if (wanted_flags & MSITRANSFORM_VALIDATE_MINORVERSION)
    {
        WCHAR *product_version_installed = msi_dup_property( package->db, szProductVersion );
        DWORD major_installed, minor_installed, major, minor;

        if (!product_version_installed)
        {
            free_transform_desc( desc );
            msiobj_release( &si->hdr );
            return ERROR_INSTALL_PACKAGE_INVALID;
        }
        msi_parse_version_string( product_version_installed, &major_installed, &minor_installed );
        msi_parse_version_string( desc->version_from, &major, &minor );

        if (major_installed == major && minor_installed == minor)
            valid_flags |= MSITRANSFORM_VALIDATE_MINORVERSION;
        msi_free( product_version_installed );
    }
    if (wanted_flags & MSITRANSFORM_VALIDATE_UPGRADECODE)
    {
        WCHAR *upgrade_code_installed = msi_dup_property( package->db, szUpgradeCode );

        if (!upgrade_code_installed)
        {
            free_transform_desc( desc );
            msiobj_release( &si->hdr );
            return ERROR_INSTALL_PACKAGE_INVALID;
        }
        if (!strcmpW( desc->upgrade_code, upgrade_code_installed ))
            valid_flags |= MSITRANSFORM_VALIDATE_UPGRADECODE;
        msi_free( upgrade_code_installed );
    }

    free_transform_desc( desc );
    msiobj_release( &si->hdr );
    if ((valid_flags & wanted_flags) != wanted_flags) return ERROR_FUNCTION_FAILED;
    TRACE("applicable transform\n");
    return ERROR_SUCCESS;
}

static UINT apply_substorage_transform( MSIPACKAGE *package, MSIDATABASE *patch_db, LPCWSTR name )
{
    UINT ret = ERROR_FUNCTION_FAILED;
    IStorage *stg = NULL;
    HRESULT r;

    TRACE("%p %s\n", package, debugstr_w(name));

    if (*name++ != ':')
    {
        ERR("expected a colon in %s\n", debugstr_w(name));
        return ERROR_FUNCTION_FAILED;
    }
    r = IStorage_OpenStorage( patch_db->storage, name, NULL, STGM_SHARE_EXCLUSIVE, NULL, 0, &stg );
    if (SUCCEEDED(r))
    {
        ret = check_transform_applicable( package, stg );
        if (ret == ERROR_SUCCESS)
            msi_table_apply_transform( package->db, stg );
        else
            TRACE("substorage transform %s wasn't applicable\n", debugstr_w(name));
        IStorage_Release( stg );
    }
    else
    {
        ERR("failed to open substorage %s\n", debugstr_w(name));
    }
    return ERROR_SUCCESS;
}

UINT msi_check_patch_applicable( MSIPACKAGE *package, MSISUMMARYINFO *si )
{
    LPWSTR guid_list, *guids, product_code;
    UINT i, ret = ERROR_FUNCTION_FAILED;

    product_code = msi_dup_property( package->db, szProductCode );
    if (!product_code)
    {
        /* FIXME: the property ProductCode should be written into the DB somewhere */
        ERR("no product code to check\n");
        return ERROR_SUCCESS;
    }
    guid_list = msi_suminfo_dup_string( si, PID_TEMPLATE );
    guids = msi_split_string( guid_list, ';' );
    for (i = 0; guids[i] && ret != ERROR_SUCCESS; i++)
    {
        if (!strcmpW( guids[i], product_code )) ret = ERROR_SUCCESS;
    }
    msi_free( guids );
    msi_free( guid_list );
    msi_free( product_code );
    return ret;
}

static UINT msi_parse_patch_summary( MSISUMMARYINFO *si, MSIPATCHINFO **patch )
{
    MSIPATCHINFO *pi;
    UINT r = ERROR_SUCCESS;
    WCHAR *p;

    if (!(pi = msi_alloc_zero( sizeof(MSIPATCHINFO) )))
    {
        return ERROR_OUTOFMEMORY;
    }
    if (!(pi->patchcode = msi_suminfo_dup_string( si, PID_REVNUMBER )))
    {
        msi_free( pi );
        return ERROR_OUTOFMEMORY;
    }
    p = pi->patchcode;
    if (*p != '{')
    {
        msi_free( pi->patchcode );
        msi_free( pi );
        return ERROR_PATCH_PACKAGE_INVALID;
    }
    if (!(p = strchrW( p + 1, '}' )))
    {
        msi_free( pi->patchcode );
        msi_free( pi );
        return ERROR_PATCH_PACKAGE_INVALID;
    }
    if (p[1])
    {
        FIXME("patch obsoletes %s\n", debugstr_w(p + 1));
        p[1] = 0;
    }
    TRACE("patch code %s\n", debugstr_w(pi->patchcode));
    if (!(pi->products = msi_suminfo_dup_string( si, PID_TEMPLATE )))
    {
        msi_free( pi->patchcode );
        msi_free( pi );
        return ERROR_OUTOFMEMORY;
    }
    if (!(pi->transforms = msi_suminfo_dup_string( si, PID_LASTAUTHOR )))
    {
        msi_free( pi->patchcode );
        msi_free( pi->products );
        msi_free( pi );
        return ERROR_OUTOFMEMORY;
    }
    *patch = pi;
    return r;
}

static UINT patch_set_media_source_prop( MSIPACKAGE *package )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','`','S','o','u','r','c','e','`',' ','F','R','O','M',' ',
        '`','M','e','d','i','a','`',' ','W','H','E','R','E',' ','`','S','o','u','r','c','e','`',' ',
        'I','S',' ','N','O','T',' ','N','U','L','L',0};
    MSIQUERY *view;
    MSIRECORD *rec;
    const WCHAR *property;
    WCHAR *patch;
    UINT r;

    r = MSI_DatabaseOpenViewW( package->db, query, &view );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_ViewExecute( view, 0 );
    if (r != ERROR_SUCCESS)
        goto done;

    if (MSI_ViewFetch( view, &rec ) == ERROR_SUCCESS)
    {
        property = MSI_RecordGetString( rec, 1 );
        patch = msi_dup_property( package->db, szPatch );
        msi_set_property( package->db, property, patch, -1 );
        msi_free( patch );
        msiobj_release( &rec->hdr );
    }

done:
    msiobj_release( &view->hdr );
    return r;
}

struct patch_offset
{
    struct list entry;
    WCHAR *name;
    UINT sequence;
};

struct patch_offset_list
{
    struct list files;
    struct list patches;
    UINT count, min, max;
    UINT offset_to_apply;
};

static struct patch_offset_list *patch_offset_list_create( void )
{
    struct patch_offset_list *pos = msi_alloc( sizeof(struct patch_offset_list) );
    list_init( &pos->files );
    list_init( &pos->patches );
    pos->count = pos->max = 0;
    pos->min = 999999;
    return pos;
}

static void patch_offset_list_free( struct patch_offset_list *pos )
{
    struct patch_offset *po, *po2;

    LIST_FOR_EACH_ENTRY_SAFE( po, po2, &pos->files, struct patch_offset, entry )
    {
        msi_free( po->name );
        msi_free( po );
    }
    LIST_FOR_EACH_ENTRY_SAFE( po, po2, &pos->patches, struct patch_offset, entry )
    {
        msi_free( po->name );
        msi_free( po );
    }
    msi_free( pos );
}

static void patch_offset_get_filepatches( MSIDATABASE *db, UINT last_sequence, struct patch_offset_list *pos )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','P','a','t','c','h',' ',
        'W','H','E','R','E',' ','S','e','q','u','e','n','c','e',' ','<','=',' ','?',' ',
        'O','R','D','E','R',' ','B','Y',' ','S','e','q','u','e','n','c','e',0};
    MSIQUERY *view;
    MSIRECORD *rec;
    UINT r;

    r = MSI_DatabaseOpenViewW( db, query, &view );
    if (r != ERROR_SUCCESS)
        return;

    rec = MSI_CreateRecord( 1 );
    MSI_RecordSetInteger( rec, 1, last_sequence );

    r = MSI_ViewExecute( view, rec );
    msiobj_release( &rec->hdr );
    if (r != ERROR_SUCCESS)
        return;

    while (MSI_ViewFetch( view, &rec ) == ERROR_SUCCESS)
    {
        struct patch_offset *po = msi_alloc( sizeof(struct patch_offset) );

        po->name     = msi_dup_record_field( rec, 1 );
        po->sequence = MSI_RecordGetInteger( rec, 2 );
        pos->min = min( pos->min, po->sequence );
        pos->max = max( pos->max, po->sequence );
        list_add_tail( &pos->patches, &po->entry );
        pos->count++;

        msiobj_release( &rec->hdr );
    }
    msiobj_release( &view->hdr );
}

static void patch_offset_get_files( MSIDATABASE *db, UINT last_sequence, struct patch_offset_list *pos )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','F','i','l','e',' ',
        'W','H','E','R','E',' ','S','e','q','u','e','n','c','e',' ','<','=',' ','?',' ',
        'O','R','D','E','R',' ','B','Y',' ','S','e','q','u','e','n','c','e',0};
    MSIQUERY *view;
    MSIRECORD *rec;
    UINT r;

    r = MSI_DatabaseOpenViewW( db, query, &view );
    if (r != ERROR_SUCCESS)
        return;

    rec = MSI_CreateRecord( 1 );
    MSI_RecordSetInteger( rec, 1, last_sequence );

    r = MSI_ViewExecute( view, rec );
    msiobj_release( &rec->hdr );
    if (r != ERROR_SUCCESS)
        return;

    while (MSI_ViewFetch( view, &rec ) == ERROR_SUCCESS)
    {
        UINT attributes = MSI_RecordGetInteger( rec, 7 );
        if (attributes & msidbFileAttributesPatchAdded)
        {
            struct patch_offset *po = msi_alloc( sizeof(struct patch_offset) );

            po->name     = msi_dup_record_field( rec, 1 );
            po->sequence = MSI_RecordGetInteger( rec, 8 );
            pos->min     = min( pos->min, po->sequence );
            pos->max     = max( pos->max, po->sequence );
            list_add_tail( &pos->files, &po->entry );
            pos->count++;
        }
        msiobj_release( &rec->hdr );
    }
    msiobj_release( &view->hdr );
}

static UINT patch_update_file_sequence( MSIDATABASE *db, const struct patch_offset_list *pos,
                                        MSIQUERY *view, MSIRECORD *rec )
{
    struct patch_offset *po;
    const WCHAR *file = MSI_RecordGetString( rec, 1 );
    UINT r = ERROR_SUCCESS, seq = MSI_RecordGetInteger( rec, 8 );

    LIST_FOR_EACH_ENTRY( po, &pos->files, struct patch_offset, entry )
    {
        if (!strcmpiW( file, po->name ))
        {
            MSI_RecordSetInteger( rec, 8, seq + pos->offset_to_apply );
            r = MSI_ViewModify( view, MSIMODIFY_UPDATE, rec );
            if (r != ERROR_SUCCESS)
                ERR("Failed to update offset for file %s (%u)\n", debugstr_w(file), r);
            break;
        }
    }
    return r;
}

static UINT patch_update_filepatch_sequence( MSIDATABASE *db, const struct patch_offset_list *pos,
                                             MSIQUERY *view, MSIRECORD *rec )
{
    static const WCHAR delete_query[] = {
        'D','E','L','E','T','E',' ','F','R','O','M',' ','`','P','a','t','c','h','`',' ',
        'W','H','E','R','E',' ','`','F','i','l','e','_','`',' ','=',' ','?',' ',
        'A','N','D',' ','`','S','e','q','u','e','n','c','e','`',' ','=',' ','?',0};
    static const WCHAR insert_query[] = {
        'I','N','S','E','R','T',' ','I','N','T','O',' ','`','P','a','t','c','h','`',' ',
        '(','`','F','i','l','e','_','`',',','`','S','e','q','u','e','n','c','e','`',',',
        '`','P','a','t','c','h','S','i','z','e','`',',','`','A','t','t','r','i','b','u','t','e','s','`',',',
        '`','H','e','a','d','e','r','`',',','`','S','t','r','e','a','m','R','e','f','_','`',')',' ',
        'V','A','L','U','E','S',' ','(','?',',','?',',','?',',','?',',','?',',','?',')',0};
    struct patch_offset *po;
    const WCHAR *file = MSI_RecordGetString( rec, 1 );
    UINT r = ERROR_SUCCESS, seq = MSI_RecordGetInteger( rec, 2 );

    LIST_FOR_EACH_ENTRY( po, &pos->patches, struct patch_offset, entry )
    {
        if (seq == po->sequence && !strcmpiW( file, po->name ))
        {
            MSIQUERY *delete_view, *insert_view;
            MSIRECORD *rec2;

            r = MSI_DatabaseOpenViewW( db, delete_query, &delete_view );
            if (r != ERROR_SUCCESS) return r;

            rec2 = MSI_CreateRecord( 2 );
            MSI_RecordSetStringW( rec2, 1, po->name );
            MSI_RecordSetInteger( rec2, 2, po->sequence );
            r = MSI_ViewExecute( delete_view, rec2 );
            msiobj_release( &delete_view->hdr );
            msiobj_release( &rec2->hdr );
            if (r != ERROR_SUCCESS) return r;

            r = MSI_DatabaseOpenViewW( db, insert_query, &insert_view );
            if (r != ERROR_SUCCESS) return r;

            MSI_RecordSetInteger( rec, 2, po->sequence + pos->offset_to_apply );

            r = MSI_ViewExecute( insert_view, rec );
            msiobj_release( &insert_view->hdr );
            if (r != ERROR_SUCCESS)
                ERR("Failed to update offset for filepatch %s (%u)\n", debugstr_w(file), r);
            break;
        }
    }
    return r;
}

static UINT patch_offset_modify_db( MSIDATABASE *db, struct patch_offset_list *pos )
{
    static const WCHAR file_query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','`','F','i','l','e','`',' ',
        'W','H','E','R','E',' ','`','S','e','q','u','e','n','c','e','`',' ','>','=',' ','?',' ',
        'A','N','D',' ','`','S','e','q','u','e','n','c','e','`',' ','<','=',' ','?',' ',
        'O','R','D','E','R',' ','B','Y',' ','`','S','e','q','u','e','n','c','e','`',0};
    static const WCHAR patch_query[] = {
        'S','E','L','E','C','T',' ','*','F','R','O','M',' ','`','P','a','t','c','h','`',' ',
        'W','H','E','R','E',' ','`','S','e','q','u','e','n','c','e','`',' ','>','=',' ','?',' ',
        'A','N','D',' ','`','S','e','q','u','e','n','c','e','`',' ','<','=',' ','?',' ',
        'O','R','D','E','R',' ','B','Y',' ','`','S','e','q','u','e','n','c','e','`',0};
    MSIRECORD *rec;
    MSIQUERY *view;
    UINT r, min = pos->min, max = pos->max, r_fetch;

    r = MSI_DatabaseOpenViewW( db, file_query, &view );
    if (r != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rec = MSI_CreateRecord( 2 );
    MSI_RecordSetInteger( rec, 1, min );
    MSI_RecordSetInteger( rec, 2, max );

    r = MSI_ViewExecute( view, rec );
    msiobj_release( &rec->hdr );
    if (r != ERROR_SUCCESS)
        goto done;

    while ((r_fetch = MSI_ViewFetch( view, &rec )) == ERROR_SUCCESS)
    {
        r = patch_update_file_sequence( db, pos, view, rec );
        msiobj_release( &rec->hdr );
        if (r != ERROR_SUCCESS) goto done;
    }
    msiobj_release( &view->hdr );

    r = MSI_DatabaseOpenViewW( db, patch_query, &view );
    if (r != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rec = MSI_CreateRecord( 2 );
    MSI_RecordSetInteger( rec, 1, min );
    MSI_RecordSetInteger( rec, 2, max );

    r = MSI_ViewExecute( view, rec );
    msiobj_release( &rec->hdr );
    if (r != ERROR_SUCCESS)
        goto done;

    while ((r_fetch = MSI_ViewFetch( view, &rec )) == ERROR_SUCCESS)
    {
        r = patch_update_filepatch_sequence( db, pos, view, rec );
        msiobj_release( &rec->hdr );
        if (r != ERROR_SUCCESS) goto done;
    }

done:
    msiobj_release( &view->hdr );
    return r;
}

static const WCHAR patch_media_query[] = {
    'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','`','M','e','d','i','a','`',' ',
    'W','H','E','R','E',' ','`','S','o','u','r','c','e','`',' ','I','S',' ','N','O','T',' ','N','U','L','L',' ',
    'A','N','D',' ','`','C','a','b','i','n','e','t','`',' ','I','S',' ','N','O','T',' ','N','U','L','L',' ',
    'O','R','D','E','R',' ','B','Y',' ','`','D','i','s','k','I','d','`',0};

struct patch_media
{
    struct list entry;
    UINT    disk_id;
    UINT    last_sequence;
    WCHAR  *prompt;
    WCHAR  *cabinet;
    WCHAR  *volume;
    WCHAR  *source;
};

static UINT patch_add_media( MSIPACKAGE *package, IStorage *storage, MSIPATCHINFO *patch )
{
    static const WCHAR delete_query[] = {
        'D','E','L','E','T','E',' ','F','R','O','M',' ','`','M','e','d','i','a','`',' ',
        'W','H','E','R','E',' ','`','D','i','s','k','I','d','`','=','?',0};
    static const WCHAR insert_query[] = {
        'I','N','S','E','R','T',' ','I','N','T','O',' ','`','M','e','d','i','a','`',' ',
        '(','`','D','i','s','k','I','d','`',',','`','L','a','s','t','S','e','q','u','e','n','c','e','`',',',
        '`','D','i','s','k','P','r','o','m','p','t','`',',','`','C','a','b','i','n','e','t','`',',',
        '`','V','o','l','u','m','e','L','a','b','e','l','`',',','`','S','o','u','r','c','e','`',')',' ',
        'V','A','L','U','E','S',' ','(','?',',','?',',','?',',','?',',','?',',','?',')',0};
    MSIQUERY *view;
    MSIRECORD *rec;
    UINT r, disk_id;
    struct list media_list;
    struct patch_media *media, *next;

    r = MSI_DatabaseOpenViewW( package->db, patch_media_query, &view );
    if (r != ERROR_SUCCESS) return r;

    r = MSI_ViewExecute( view, 0 );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &view->hdr );
        TRACE("query failed %u\n", r);
        return r;
    }
    list_init( &media_list );
    while (MSI_ViewFetch( view, &rec ) == ERROR_SUCCESS)
    {
        disk_id = MSI_RecordGetInteger( rec, 1 );
        TRACE("disk_id %u\n", disk_id);
        if (disk_id >= MSI_INITIAL_MEDIA_TRANSFORM_DISKID)
        {
            msiobj_release( &rec->hdr );
            continue;
        }
        if (!(media = msi_alloc( sizeof( *media )))) {
            msiobj_release( &rec->hdr );
            goto done;
	}
        media->disk_id = disk_id;
        media->last_sequence = MSI_RecordGetInteger( rec, 2 );
        media->prompt  = msi_dup_record_field( rec, 3 );
        media->cabinet = msi_dup_record_field( rec, 4 );
        media->volume  = msi_dup_record_field( rec, 5 );
        media->source  = msi_dup_record_field( rec, 6 );

        list_add_tail( &media_list, &media->entry );
        msiobj_release( &rec->hdr );
    }
    LIST_FOR_EACH_ENTRY( media, &media_list, struct patch_media, entry )
    {
        MSIQUERY *delete_view, *insert_view;

        r = MSI_DatabaseOpenViewW( package->db, delete_query, &delete_view );
        if (r != ERROR_SUCCESS) goto done;

        rec = MSI_CreateRecord( 1 );
        MSI_RecordSetInteger( rec, 1, media->disk_id );

        r = MSI_ViewExecute( delete_view, rec );
        msiobj_release( &delete_view->hdr );
        msiobj_release( &rec->hdr );
        if (r != ERROR_SUCCESS) goto done;

        r = MSI_DatabaseOpenViewW( package->db, insert_query, &insert_view );
        if (r != ERROR_SUCCESS) goto done;

        disk_id = package->db->media_transform_disk_id;
        TRACE("disk id       %u\n", disk_id);
        TRACE("last sequence %u\n", media->last_sequence);
        TRACE("prompt        %s\n", debugstr_w(media->prompt));
        TRACE("cabinet       %s\n", debugstr_w(media->cabinet));
        TRACE("volume        %s\n", debugstr_w(media->volume));
        TRACE("source        %s\n", debugstr_w(media->source));

        rec = MSI_CreateRecord( 6 );
        MSI_RecordSetInteger( rec, 1, disk_id );
        MSI_RecordSetInteger( rec, 2, media->last_sequence );
        MSI_RecordSetStringW( rec, 3, media->prompt );
        MSI_RecordSetStringW( rec, 4, media->cabinet );
        MSI_RecordSetStringW( rec, 5, media->volume );
        MSI_RecordSetStringW( rec, 6, media->source );

        r = MSI_ViewExecute( insert_view, rec );
        msiobj_release( &insert_view->hdr );
        msiobj_release( &rec->hdr );
        if (r != ERROR_SUCCESS) goto done;

        r = msi_add_cabinet_stream( package, disk_id, storage, media->cabinet );
        if (r != ERROR_SUCCESS) ERR("failed to add cabinet stream %u\n", r);
        else
        {
            patch->disk_id = disk_id;
            package->db->media_transform_disk_id++;
        }
    }

done:
    msiobj_release( &view->hdr );
    LIST_FOR_EACH_ENTRY_SAFE( media, next, &media_list, struct patch_media, entry )
    {
        list_remove( &media->entry );
        msi_free( media->prompt );
        msi_free( media->cabinet );
        msi_free( media->volume );
        msi_free( media->source );
        msi_free( media );
    }
    return r;
}

static UINT patch_set_offsets( MSIDATABASE *db, MSIPATCHINFO *patch )
{
    MSIQUERY *view;
    MSIRECORD *rec;
    UINT r;

    r = MSI_DatabaseOpenViewW( db, patch_media_query, &view );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_ViewExecute( view, 0 );
    if (r != ERROR_SUCCESS)
        goto done;

    while (MSI_ViewFetch( view, &rec ) == ERROR_SUCCESS)
    {
        UINT offset, last_sequence = MSI_RecordGetInteger( rec, 2 );
        struct patch_offset_list *pos;

        /* FIXME: set/check Source field instead? */
        if (last_sequence >= MSI_INITIAL_MEDIA_TRANSFORM_OFFSET)
        {
            msiobj_release( &rec->hdr );
            continue;
        }
        pos = patch_offset_list_create();
        patch_offset_get_files( db, last_sequence, pos );
        patch_offset_get_filepatches( db, last_sequence, pos );

        offset = db->media_transform_offset - pos->min;
        last_sequence = offset + pos->max;

        last_sequence += pos->min;
        pos->offset_to_apply = offset;
        if (pos->count)
        {
            r = patch_offset_modify_db( db, pos );
            if (r != ERROR_SUCCESS)
                ERR("Failed to set offsets, expect breakage (%u)\n", r);
        }
        MSI_RecordSetInteger( rec, 2, last_sequence );
        r = MSI_ViewModify( view, MSIMODIFY_UPDATE, rec );
        if (r != ERROR_SUCCESS)
            ERR("Failed to update Media table entry, expect breakage (%u)\n", r);

        db->media_transform_offset = last_sequence + 1;

        patch_offset_list_free( pos );
        msiobj_release( &rec->hdr );
    }

done:
    msiobj_release( &view->hdr );
    return r;
}

static UINT msi_apply_patch_db( MSIPACKAGE *package, MSIDATABASE *patch_db, MSIPATCHINFO *patch )
{
    UINT i, r = ERROR_SUCCESS;
    WCHAR **substorage;

    /* apply substorage transforms */
    substorage = msi_split_string( patch->transforms, ';' );
    for (i = 0; substorage && substorage[i] && r == ERROR_SUCCESS; i++)
    {
        r = apply_substorage_transform( package, patch_db, substorage[i] );
        if (r == ERROR_SUCCESS)
        {
            r = patch_set_offsets( package->db, patch );
            if (r == ERROR_SUCCESS)
                r = patch_add_media( package, patch_db->storage, patch );
        }
    }
    msi_free( substorage );
    if (r != ERROR_SUCCESS)
        return r;

    r = patch_set_media_source_prop( package );
    if (r != ERROR_SUCCESS)
        return r;

    patch->state = MSIPATCHSTATE_APPLIED;
    list_add_tail( &package->patches, &patch->entry );
    return ERROR_SUCCESS;
}

void msi_free_patchinfo( MSIPATCHINFO *patch )
{
    msi_free( patch->patchcode );
    msi_free( patch->products );
    msi_free( patch->transforms );
    msi_free( patch->filename );
    msi_free( patch->localfile );
    msi_free( patch );
}

static UINT msi_apply_patch_package( MSIPACKAGE *package, const WCHAR *file )
{
    static const WCHAR dotmsp[] = {'.','m','s','p',0};
    MSIDATABASE *patch_db = NULL;
    WCHAR localfile[MAX_PATH];
    MSISUMMARYINFO *si;
    MSIPATCHINFO *patch = NULL;
    UINT r;

    TRACE("%p, %s\n", package, debugstr_w(file));

    r = MSI_OpenDatabaseW( file, MSIDBOPEN_READONLY + MSIDBOPEN_PATCHFILE, &patch_db );
    if (r != ERROR_SUCCESS)
    {
        ERR("failed to open patch collection %s\n", debugstr_w( file ) );
        return r;
    }
    r = msi_get_suminfo( patch_db->storage, 0, &si );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &patch_db->hdr );
        return r;
    }
    r = msi_check_patch_applicable( package, si );
    if (r != ERROR_SUCCESS)
    {
        TRACE("patch not applicable\n");
        r = ERROR_SUCCESS;
        goto done;
    }
    r = msi_parse_patch_summary( si, &patch );
    if ( r != ERROR_SUCCESS )
        goto done;

    r = msi_create_empty_local_file( localfile, dotmsp );
    if ( r != ERROR_SUCCESS )
        goto done;

    r = ERROR_OUTOFMEMORY;
    patch->registered = FALSE;
    if (!(patch->filename = strdupW( file ))) goto done;
    if (!(patch->localfile = strdupW( localfile ))) goto done;

    r = msi_apply_patch_db( package, patch_db, patch );
    if (r != ERROR_SUCCESS) WARN("patch failed to apply %u\n", r);

done:
    msiobj_release( &si->hdr );
    msiobj_release( &patch_db->hdr );
    if (patch && r != ERROR_SUCCESS)
    {
        DeleteFileW( patch->localfile );
        msi_free_patchinfo( patch );
    }
    return r;
}

/* get the PATCH property, and apply all the patches it specifies */
UINT msi_apply_patches( MSIPACKAGE *package )
{
    LPWSTR patch_list, *patches;
    UINT i, r = ERROR_SUCCESS;

    patch_list = msi_dup_property( package->db, szPatch );

    TRACE("patches to be applied: %s\n", debugstr_w(patch_list));

    patches = msi_split_string( patch_list, ';' );
    for (i = 0; patches && patches[i] && r == ERROR_SUCCESS; i++)
        r = msi_apply_patch_package( package, patches[i] );

    msi_free( patches );
    msi_free( patch_list );
    return r;
}

UINT msi_apply_transforms( MSIPACKAGE *package )
{
    static const WCHAR szTransforms[] = {'T','R','A','N','S','F','O','R','M','S',0};
    LPWSTR xform_list, *xforms;
    UINT i, r = ERROR_SUCCESS;

    xform_list = msi_dup_property( package->db, szTransforms );
    xforms = msi_split_string( xform_list, ';' );

    for (i = 0; xforms && xforms[i] && r == ERROR_SUCCESS; i++)
    {
        if (xforms[i][0] == ':')
            r = apply_substorage_transform( package, package->db, xforms[i] );
        else
        {
            WCHAR *transform;

            if (!PathIsRelativeW( xforms[i] )) transform = xforms[i];
            else
            {
                WCHAR *p = strrchrW( package->PackagePath, '\\' );
                DWORD len = p - package->PackagePath + 1;

                if (!(transform = msi_alloc( (len + strlenW( xforms[i] ) + 1) * sizeof(WCHAR)) ))
                {
                    msi_free( xforms );
                    msi_free( xform_list );
                    return ERROR_OUTOFMEMORY;
                }
                memcpy( transform, package->PackagePath, len * sizeof(WCHAR) );
                memcpy( transform + len, xforms[i], (strlenW( xforms[i] ) + 1) * sizeof(WCHAR) );
            }
            r = MSI_DatabaseApplyTransformW( package->db, transform, 0 );
            if (transform != xforms[i]) msi_free( transform );
        }
    }
    msi_free( xforms );
    msi_free( xform_list );
    return r;
}

UINT msi_apply_registered_patch( MSIPACKAGE *package, LPCWSTR patch_code )
{
    UINT r;
    DWORD len;
    WCHAR patch_file[MAX_PATH];
    MSIDATABASE *patch_db;
    MSIPATCHINFO *patch_info;
    MSISUMMARYINFO *si;

    TRACE("%p, %s\n", package, debugstr_w(patch_code));

    len = sizeof(patch_file) / sizeof(WCHAR);
    r = MsiGetPatchInfoExW( patch_code, package->ProductCode, NULL, package->Context,
                            INSTALLPROPERTY_LOCALPACKAGEW, patch_file, &len );
    if (r != ERROR_SUCCESS)
    {
        ERR("failed to get patch filename %u\n", r);
        return r;
    }
    r = MSI_OpenDatabaseW( patch_file, MSIDBOPEN_READONLY + MSIDBOPEN_PATCHFILE, &patch_db );
    if (r != ERROR_SUCCESS)
    {
        ERR("failed to open patch database %s\n", debugstr_w( patch_file ));
        return r;
    }
    r = msi_get_suminfo( patch_db->storage, 0, &si );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &patch_db->hdr );
        return r;
    }
    r = msi_parse_patch_summary( si, &patch_info );
    msiobj_release( &si->hdr );
    if (r != ERROR_SUCCESS)
    {
        ERR("failed to parse patch summary %u\n", r);
        msiobj_release( &patch_db->hdr );
        return r;
    }
    patch_info->registered = TRUE;
    patch_info->localfile = strdupW( patch_file );
    if (!patch_info->localfile)
    {
        msiobj_release( &patch_db->hdr );
        msi_free_patchinfo( patch_info );
        return ERROR_OUTOFMEMORY;
    }
    r = msi_apply_patch_db( package, patch_db, patch_info );
    msiobj_release( &patch_db->hdr );
    if (r != ERROR_SUCCESS)
    {
        ERR("failed to apply patch %u\n", r);
        msi_free_patchinfo( patch_info );
    }
    return r;
}
