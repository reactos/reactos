/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2004,2005 Aric Stewart for CodeWeavers
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

/*
 * Pages I need
 *
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/installexecutesequence_table.asp

http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/standard_actions_reference.asp
 */

#include <stdarg.h>

#define COBJMACROS

#include "stdio.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winsvc.h"
#include "wine/debug.h"
#include "msidefs.h"
#include "msipriv.h"
#include "winuser.h"
#include "shlobj.h"
#include "wine/unicode.h"
#include "winver.h"

#define REG_PROGRESS_VALUE 13200
#define COMPONENT_PROGRESS_VALUE 24000

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/*
 * Prototypes
 */
static UINT ACTION_ProcessExecSequence(MSIPACKAGE *package, BOOL UIran);
static UINT ACTION_ProcessUISequence(MSIPACKAGE *package);
static UINT ACTION_PerformActionSequence(MSIPACKAGE *package, UINT seq, BOOL UI);

/*
 * consts and values used
 */
static const WCHAR c_colon[] = {'C',':','\\',0};

static const WCHAR szCreateFolders[] =
    {'C','r','e','a','t','e','F','o','l','d','e','r','s',0};
static const WCHAR szCostFinalize[] =
    {'C','o','s','t','F','i','n','a','l','i','z','e',0};
const WCHAR szInstallFiles[] =
    {'I','n','s','t','a','l','l','F','i','l','e','s',0};
const WCHAR szDuplicateFiles[] =
    {'D','u','p','l','i','c','a','t','e','F','i','l','e','s',0};
static const WCHAR szWriteRegistryValues[] =
    {'W','r','i','t','e','R','e','g','i','s','t','r','y',
            'V','a','l','u','e','s',0};
static const WCHAR szCostInitialize[] =
    {'C','o','s','t','I','n','i','t','i','a','l','i','z','e',0};
static const WCHAR szFileCost[] = 
    {'F','i','l','e','C','o','s','t',0};
static const WCHAR szInstallInitialize[] = 
    {'I','n','s','t','a','l','l','I','n','i','t','i','a','l','i','z','e',0};
static const WCHAR szInstallValidate[] = 
    {'I','n','s','t','a','l','l','V','a','l','i','d','a','t','e',0};
static const WCHAR szLaunchConditions[] = 
    {'L','a','u','n','c','h','C','o','n','d','i','t','i','o','n','s',0};
static const WCHAR szProcessComponents[] = 
    {'P','r','o','c','e','s','s','C','o','m','p','o','n','e','n','t','s',0};
static const WCHAR szRegisterTypeLibraries[] = 
    {'R','e','g','i','s','t','e','r','T','y','p','e',
            'L','i','b','r','a','r','i','e','s',0};
const WCHAR szRegisterClassInfo[] = 
    {'R','e','g','i','s','t','e','r','C','l','a','s','s','I','n','f','o',0};
const WCHAR szRegisterProgIdInfo[] = 
    {'R','e','g','i','s','t','e','r','P','r','o','g','I','d','I','n','f','o',0};
static const WCHAR szCreateShortcuts[] = 
    {'C','r','e','a','t','e','S','h','o','r','t','c','u','t','s',0};
static const WCHAR szPublishProduct[] = 
    {'P','u','b','l','i','s','h','P','r','o','d','u','c','t',0};
static const WCHAR szWriteIniValues[] = 
    {'W','r','i','t','e','I','n','i','V','a','l','u','e','s',0};
static const WCHAR szSelfRegModules[] = 
    {'S','e','l','f','R','e','g','M','o','d','u','l','e','s',0};
static const WCHAR szPublishFeatures[] = 
    {'P','u','b','l','i','s','h','F','e','a','t','u','r','e','s',0};
static const WCHAR szRegisterProduct[] = 
    {'R','e','g','i','s','t','e','r','P','r','o','d','u','c','t',0};
static const WCHAR szInstallExecute[] = 
    {'I','n','s','t','a','l','l','E','x','e','c','u','t','e',0};
static const WCHAR szInstallExecuteAgain[] = 
    {'I','n','s','t','a','l','l','E','x','e','c','u','t','e',
            'A','g','a','i','n',0};
static const WCHAR szInstallFinalize[] = 
    {'I','n','s','t','a','l','l','F','i','n','a','l','i','z','e',0};
static const WCHAR szForceReboot[] = 
    {'F','o','r','c','e','R','e','b','o','o','t',0};
static const WCHAR szResolveSource[] =
    {'R','e','s','o','l','v','e','S','o','u','r','c','e',0};
static const WCHAR szAppSearch[] = 
    {'A','p','p','S','e','a','r','c','h',0};
static const WCHAR szAllocateRegistrySpace[] = 
    {'A','l','l','o','c','a','t','e','R','e','g','i','s','t','r','y',
            'S','p','a','c','e',0};
static const WCHAR szBindImage[] = 
    {'B','i','n','d','I','m','a','g','e',0};
static const WCHAR szCCPSearch[] = 
    {'C','C','P','S','e','a','r','c','h',0};
static const WCHAR szDeleteServices[] = 
    {'D','e','l','e','t','e','S','e','r','v','i','c','e','s',0};
static const WCHAR szDisableRollback[] = 
    {'D','i','s','a','b','l','e','R','o','l','l','b','a','c','k',0};
static const WCHAR szExecuteAction[] = 
    {'E','x','e','c','u','t','e','A','c','t','i','o','n',0};
const WCHAR szFindRelatedProducts[] = 
    {'F','i','n','d','R','e','l','a','t','e','d',
            'P','r','o','d','u','c','t','s',0};
static const WCHAR szInstallAdminPackage[] = 
    {'I','n','s','t','a','l','l','A','d','m','i','n',
            'P','a','c','k','a','g','e',0};
static const WCHAR szInstallSFPCatalogFile[] = 
    {'I','n','s','t','a','l','l','S','F','P','C','a','t','a','l','o','g',
            'F','i','l','e',0};
static const WCHAR szIsolateComponents[] = 
    {'I','s','o','l','a','t','e','C','o','m','p','o','n','e','n','t','s',0};
const WCHAR szMigrateFeatureStates[] = 
    {'M','i','g','r','a','t','e','F','e','a','t','u','r','e',
            'S','t','a','t','e','s',0};
const WCHAR szMoveFiles[] = 
    {'M','o','v','e','F','i','l','e','s',0};
static const WCHAR szMsiPublishAssemblies[] = 
    {'M','s','i','P','u','b','l','i','s','h',
            'A','s','s','e','m','b','l','i','e','s',0};
static const WCHAR szMsiUnpublishAssemblies[] = 
    {'M','s','i','U','n','p','u','b','l','i','s','h',
            'A','s','s','e','m','b','l','i','e','s',0};
static const WCHAR szInstallODBC[] = 
    {'I','n','s','t','a','l','l','O','D','B','C',0};
static const WCHAR szInstallServices[] = 
    {'I','n','s','t','a','l','l','S','e','r','v','i','c','e','s',0};
const WCHAR szPatchFiles[] = 
    {'P','a','t','c','h','F','i','l','e','s',0};
static const WCHAR szPublishComponents[] = 
    {'P','u','b','l','i','s','h','C','o','m','p','o','n','e','n','t','s',0};
static const WCHAR szRegisterComPlus[] =
    {'R','e','g','i','s','t','e','r','C','o','m','P','l','u','s',0};
const WCHAR szRegisterExtensionInfo[] =
    {'R','e','g','i','s','t','e','r','E','x','t','e','n','s','i','o','n',
            'I','n','f','o',0};
static const WCHAR szRegisterFonts[] =
    {'R','e','g','i','s','t','e','r','F','o','n','t','s',0};
const WCHAR szRegisterMIMEInfo[] =
    {'R','e','g','i','s','t','e','r','M','I','M','E','I','n','f','o',0};
static const WCHAR szRegisterUser[] =
    {'R','e','g','i','s','t','e','r','U','s','e','r',0};
const WCHAR szRemoveDuplicateFiles[] =
    {'R','e','m','o','v','e','D','u','p','l','i','c','a','t','e',
            'F','i','l','e','s',0};
static const WCHAR szRemoveEnvironmentStrings[] =
    {'R','e','m','o','v','e','E','n','v','i','r','o','n','m','e','n','t',
            'S','t','r','i','n','g','s',0};
const WCHAR szRemoveExistingProducts[] =
    {'R','e','m','o','v','e','E','x','i','s','t','i','n','g',
            'P','r','o','d','u','c','t','s',0};
const WCHAR szRemoveFiles[] =
    {'R','e','m','o','v','e','F','i','l','e','s',0};
static const WCHAR szRemoveFolders[] =
    {'R','e','m','o','v','e','F','o','l','d','e','r','s',0};
static const WCHAR szRemoveIniValues[] =
    {'R','e','m','o','v','e','I','n','i','V','a','l','u','e','s',0};
static const WCHAR szRemoveODBC[] =
    {'R','e','m','o','v','e','O','D','B','C',0};
static const WCHAR szRemoveRegistryValues[] =
    {'R','e','m','o','v','e','R','e','g','i','s','t','r','y',
            'V','a','l','u','e','s',0};
static const WCHAR szRemoveShortcuts[] =
    {'R','e','m','o','v','e','S','h','o','r','t','c','u','t','s',0};
static const WCHAR szRMCCPSearch[] =
    {'R','M','C','C','P','S','e','a','r','c','h',0};
static const WCHAR szScheduleReboot[] =
    {'S','c','h','e','d','u','l','e','R','e','b','o','o','t',0};
static const WCHAR szSelfUnregModules[] =
    {'S','e','l','f','U','n','r','e','g','M','o','d','u','l','e','s',0};
static const WCHAR szSetODBCFolders[] =
    {'S','e','t','O','D','B','C','F','o','l','d','e','r','s',0};
static const WCHAR szStartServices[] =
    {'S','t','a','r','t','S','e','r','v','i','c','e','s',0};
static const WCHAR szStopServices[] =
    {'S','t','o','p','S','e','r','v','i','c','e','s',0};
static const WCHAR szUnpublishComponents[] =
    {'U','n','p','u','b','l','i','s','h',
            'C','o','m','p','o','n','e','n','t','s',0};
static const WCHAR szUnpublishFeatures[] =
    {'U','n','p','u','b','l','i','s','h','F','e','a','t','u','r','e','s',0};
const WCHAR szUnregisterClassInfo[] =
    {'U','n','r','e','g','i','s','t','e','r','C','l','a','s','s',
            'I','n','f','o',0};
static const WCHAR szUnregisterComPlus[] =
    {'U','n','r','e','g','i','s','t','e','r','C','o','m','P','l','u','s',0};
const WCHAR szUnregisterExtensionInfo[] =
    {'U','n','r','e','g','i','s','t','e','r',
            'E','x','t','e','n','s','i','o','n','I','n','f','o',0};
static const WCHAR szUnregisterFonts[] =
    {'U','n','r','e','g','i','s','t','e','r','F','o','n','t','s',0};
const WCHAR szUnregisterMIMEInfo[] =
    {'U','n','r','e','g','i','s','t','e','r','M','I','M','E','I','n','f','o',0};
const WCHAR szUnregisterProgIdInfo[] =
    {'U','n','r','e','g','i','s','t','e','r','P','r','o','g','I','d',
            'I','n','f','o',0};
static const WCHAR szUnregisterTypeLibraries[] =
    {'U','n','r','e','g','i','s','t','e','r','T','y','p','e',
            'L','i','b','r','a','r','i','e','s',0};
static const WCHAR szValidateProductID[] =
    {'V','a','l','i','d','a','t','e','P','r','o','d','u','c','t','I','D',0};
static const WCHAR szWriteEnvironmentStrings[] =
    {'W','r','i','t','e','E','n','v','i','r','o','n','m','e','n','t',
            'S','t','r','i','n','g','s',0};

/* action handlers */
typedef UINT (*STANDARDACTIONHANDLER)(MSIPACKAGE*);

struct _actions {
    LPCWSTR action;
    STANDARDACTIONHANDLER handler;
};

static struct _actions StandardActions[];


/********************************************************
 * helper functions
 ********************************************************/

static void ui_actionstart(MSIPACKAGE *package, LPCWSTR action)
{
    static const WCHAR Query_t[] = 
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','A','c','t','i','o', 'n','T','e','x','t','`',' ',
         'W','H','E','R','E', ' ','`','A','c','t','i','o','n','`',' ','=', 
         ' ','\'','%','s','\'',0};
    MSIRECORD * row;

    row = MSI_QueryGetRecord( package->db, Query_t, action );
    if (!row)
        return;
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONSTART, row);
    msiobj_release(&row->hdr);
}

static void ui_actioninfo(MSIPACKAGE *package, LPCWSTR action, BOOL start, 
                          UINT rc)
{
    MSIRECORD * row;
    static const WCHAR template_s[]=
        {'A','c','t','i','o','n',' ','s','t','a','r','t',' ','%','s',':',' ',
         '%','s', '.',0};
    static const WCHAR template_e[]=
        {'A','c','t','i','o','n',' ','e','n','d','e','d',' ','%','s',':',' ',
         '%','s', '.',' ','R','e','t','u','r','n',' ','v','a','l','u','e',' ',
         '%','i','.',0};
    static const WCHAR format[] = 
        {'H','H','\'',':','\'','m','m','\'',':','\'','s','s',0};
    WCHAR message[1024];
    WCHAR timet[0x100];

    GetTimeFormatW(LOCALE_USER_DEFAULT, 0, NULL, format, timet, 0x100);
    if (start)
        sprintfW(message,template_s,timet,action);
    else
        sprintfW(message,template_e,timet,action,rc);
    
    row = MSI_CreateRecord(1);
    MSI_RecordSetStringW(row,1,message);
 
    MSI_ProcessMessage(package, INSTALLMESSAGE_INFO, row);
    msiobj_release(&row->hdr);
}

static UINT msi_parse_command_line( MSIPACKAGE *package, LPCWSTR szCommandLine )
{
    LPCWSTR ptr,ptr2;
    BOOL quote;
    DWORD len;
    LPWSTR prop = NULL, val = NULL;

    if (!szCommandLine)
        return ERROR_SUCCESS;

    ptr = szCommandLine;
       
    while (*ptr)
    {
        if (*ptr==' ')
        {
            ptr++;
            continue;
        }

        TRACE("Looking at %s\n",debugstr_w(ptr));

        ptr2 = strchrW(ptr,'=');
        if (!ptr2)
        {
            ERR("command line contains unknown string : %s\n", debugstr_w(ptr));
            break;
        }
 
        quote = FALSE;

        len = ptr2-ptr;
        prop = msi_alloc((len+1)*sizeof(WCHAR));
        memcpy(prop,ptr,len*sizeof(WCHAR));
        prop[len]=0;
        ptr2++;
       
        len = 0; 
        ptr = ptr2; 
        while (*ptr && (quote || (!quote && *ptr!=' ')))
        {
            if (*ptr == '"')
                quote = !quote;
            ptr++;
            len++;
        }
       
        if (*ptr2=='"')
        {
            ptr2++;
            len -= 2;
        }
        val = msi_alloc((len+1)*sizeof(WCHAR));
        memcpy(val,ptr2,len*sizeof(WCHAR));
        val[len] = 0;

        if (lstrlenW(prop) > 0)
        {
            TRACE("Found commandline property (%s) = (%s)\n", 
                   debugstr_w(prop), debugstr_w(val));
            MSI_SetPropertyW(package,prop,val);
        }
        msi_free(val);
        msi_free(prop);
    }

    return ERROR_SUCCESS;
}


static LPWSTR* msi_split_string( LPCWSTR str, WCHAR sep )
{
    LPCWSTR pc;
    LPWSTR p, *ret = NULL;
    UINT count = 0;

    if (!str)
        return ret;

    /* count the number of substrings */
    for ( pc = str, count = 0; pc; count++ )
    {
        pc = strchrW( pc, sep );
        if (pc)
            pc++;
    }

    /* allocate space for an array of substring pointers and the substrings */
    ret = msi_alloc( (count+1) * sizeof (LPWSTR) +
                     (lstrlenW(str)+1) * sizeof(WCHAR) );
    if (!ret)
        return ret;

    /* copy the string and set the pointers */
    p = (LPWSTR) &ret[count+1];
    lstrcpyW( p, str );
    for( count = 0; (ret[count] = p); count++ )
    {
        p = strchrW( p, sep );
        if (p)
            *p++ = 0;
    }

    return ret;
}

static UINT msi_check_transform_applicable( MSIPACKAGE *package, IStorage *patch )
{
    WCHAR szProductCode[] = { 'P','r','o','d','u','c','t','C','o','d','e',0 };
    LPWSTR prod_code, patch_product;
    UINT ret;

    prod_code = msi_dup_property( package, szProductCode );
    patch_product = msi_get_suminfo_product( patch );

    TRACE("db = %s patch = %s\n", debugstr_w(prod_code), debugstr_w(patch_product));

    if ( strstrW( patch_product, prod_code ) )
        ret = ERROR_SUCCESS;
    else
        ret = ERROR_FUNCTION_FAILED;

    msi_free( patch_product );
    msi_free( prod_code );

    return ret;
}

static UINT msi_apply_substorage_transform( MSIPACKAGE *package,
                                 MSIDATABASE *patch_db, LPCWSTR name )
{
    UINT ret = ERROR_FUNCTION_FAILED;
    IStorage *stg = NULL;
    HRESULT r;

    TRACE("%p %s\n", package, debugstr_w(name) );

    if (*name++ != ':')
    {
        ERR("expected a colon in %s\n", debugstr_w(name));
        return ERROR_FUNCTION_FAILED;
    }

    r = IStorage_OpenStorage( patch_db->storage, name, NULL, STGM_SHARE_EXCLUSIVE, NULL, 0, &stg );
    if (SUCCEEDED(r))
    {
        ret = msi_check_transform_applicable( package, stg );
        if (ret == ERROR_SUCCESS)
            msi_table_apply_transform( package->db, stg );
        else
            TRACE("substorage transform %s wasn't applicable\n", debugstr_w(name));
        IStorage_Release( stg );
    }
    else
        ERR("failed to open substorage %s\n", debugstr_w(name));

    return ERROR_SUCCESS;
}

static UINT msi_check_patch_applicable( MSIPACKAGE *package, MSISUMMARYINFO *si )
{
    static const WCHAR szProdID[] = { 'P','r','o','d','u','c','t','I','D',0 };
    LPWSTR guid_list, *guids, product_id;
    UINT i, ret = ERROR_FUNCTION_FAILED;

    product_id = msi_dup_property( package, szProdID );
    if (!product_id)
    {
        /* FIXME: the property ProductID should be written into the DB somewhere */
        ERR("no product ID to check\n");
        return ERROR_SUCCESS;
    }

    guid_list = msi_suminfo_dup_string( si, PID_TEMPLATE );
    guids = msi_split_string( guid_list, ';' );
    for ( i = 0; guids[i] && ret != ERROR_SUCCESS; i++ )
    {
        if (!lstrcmpW( guids[i], product_id ))
            ret = ERROR_SUCCESS;
    }
    msi_free( guids );
    msi_free( guid_list );
    msi_free( product_id );

    return ret;
}

static UINT msi_parse_patch_summary( MSIPACKAGE *package, MSIDATABASE *patch_db )
{
    MSISUMMARYINFO *si;
    LPWSTR str, *substorage;
    UINT i, r = ERROR_SUCCESS;

    si = MSI_GetSummaryInformationW( patch_db->storage, 0 );
    if (!si)
        return ERROR_FUNCTION_FAILED;

    msi_check_patch_applicable( package, si );

    /* enumerate the substorage */
    str = msi_suminfo_dup_string( si, PID_LASTAUTHOR );
    substorage = msi_split_string( str, ';' );
    for ( i = 0; substorage && substorage[i] && r == ERROR_SUCCESS; i++ )
        r = msi_apply_substorage_transform( package, patch_db, substorage[i] );
    msi_free( substorage );
    msi_free( str );

    /* FIXME: parse the sources in PID_REVNUMBER and do something with them... */

    msiobj_release( &si->hdr );

    return r;
}

static UINT msi_apply_patch_package( MSIPACKAGE *package, LPCWSTR file )
{
    MSIDATABASE *patch_db = NULL;
    UINT r;

    TRACE("%p %s\n", package, debugstr_w( file ) );

    /* FIXME:
     *  We probably want to make sure we only open a patch collection here.
     *  Patch collections (.msp) and databases (.msi) have different GUIDs
     *  but currently MSI_OpenDatabaseW will accept both.
     */
    r = MSI_OpenDatabaseW( file, MSIDBOPEN_READONLY, &patch_db );
    if ( r != ERROR_SUCCESS )
    {
        ERR("failed to open patch collection %s\n", debugstr_w( file ) );
        return r;
    }

    msi_parse_patch_summary( package, patch_db );

    /*
     * There might be a CAB file in the patch package,
     * so append it to the list of storage to search for streams.
     */
    append_storage_to_db( package->db, patch_db->storage );

    msiobj_release( &patch_db->hdr );

    return ERROR_SUCCESS;
}

/* get the PATCH property, and apply all the patches it specifies */
static UINT msi_apply_patches( MSIPACKAGE *package )
{
    static const WCHAR szPatch[] = { 'P','A','T','C','H',0 };
    LPWSTR patch_list, *patches;
    UINT i, r = ERROR_SUCCESS;

    patch_list = msi_dup_property( package, szPatch );

    TRACE("patches to be applied: %s\n", debugstr_w( patch_list ) );

    patches = msi_split_string( patch_list, ';' );
    for( i=0; patches && patches[i] && r == ERROR_SUCCESS; i++ )
        r = msi_apply_patch_package( package, patches[i] );

    msi_free( patches );
    msi_free( patch_list );

    return r;
}

static UINT msi_apply_transforms( MSIPACKAGE *package )
{
    static const WCHAR szTransforms[] = {
        'T','R','A','N','S','F','O','R','M','S',0 };
    LPWSTR xform_list, *xforms;
    UINT i, r = ERROR_SUCCESS;

    xform_list = msi_dup_property( package, szTransforms );
    xforms = msi_split_string( xform_list, ';' );

    for( i=0; xforms && xforms[i] && r == ERROR_SUCCESS; i++ )
    {
        if (xforms[i][0] == ':')
            r = msi_apply_substorage_transform( package, package->db, &xforms[i][1] );
        else
            r = MSI_DatabaseApplyTransformW( package->db, xforms[i], 0 );
    }

    msi_free( xforms );
    msi_free( xform_list );

    return r;
}

/****************************************************
 * TOP level entry points 
 *****************************************************/

UINT MSI_InstallPackage( MSIPACKAGE *package, LPCWSTR szPackagePath,
                         LPCWSTR szCommandLine )
{
    UINT rc;
    BOOL ui = FALSE;
    static const WCHAR szUILevel[] = {'U','I','L','e','v','e','l',0};
    static const WCHAR szAction[] = {'A','C','T','I','O','N',0};
    static const WCHAR szInstall[] = {'I','N','S','T','A','L','L',0};

    MSI_SetPropertyW(package, szAction, szInstall);

    package->script = msi_alloc_zero(sizeof(MSISCRIPT));

    package->script->InWhatSequence = SEQUENCE_INSTALL;

    if (szPackagePath)   
    {
        LPWSTR p, check, path;
 
        path = strdupW(szPackagePath);
        p = strrchrW(path,'\\');    
        if (p)
        {
            p++;
            *p=0;
        }
        else
        {
            msi_free(path);
            path = msi_alloc(MAX_PATH*sizeof(WCHAR));
            GetCurrentDirectoryW(MAX_PATH,path);
            strcatW(path,cszbs);
        }

        check = msi_dup_property( package, cszSourceDir );
        if (!check)
            MSI_SetPropertyW(package, cszSourceDir, path);
        msi_free(check);

        check = msi_dup_property( package, cszSOURCEDIR );
        if (!check)
            MSI_SetPropertyW(package, cszSOURCEDIR, path);

        msi_free( package->PackagePath );
        package->PackagePath = path;

        msi_free(check);
    }

    msi_parse_command_line( package, szCommandLine );

    msi_apply_transforms( package );
    msi_apply_patches( package );

    if ( msi_get_property_int(package, szUILevel, 0) >= INSTALLUILEVEL_REDUCED )
    {
        package->script->InWhatSequence |= SEQUENCE_UI;
        rc = ACTION_ProcessUISequence(package);
        ui = TRUE;
        if (rc == ERROR_SUCCESS)
        {
            package->script->InWhatSequence |= SEQUENCE_EXEC;
            rc = ACTION_ProcessExecSequence(package,TRUE);
        }
    }
    else
        rc = ACTION_ProcessExecSequence(package,FALSE);
    
    if (rc == -1)
    {
        /* install was halted but should be considered a success */
        rc = ERROR_SUCCESS;
    }

    package->script->CurrentlyScripting= FALSE;

    /* process the ending type action */
    if (rc == ERROR_SUCCESS)
        ACTION_PerformActionSequence(package,-1,ui);
    else if (rc == ERROR_INSTALL_USEREXIT) 
        ACTION_PerformActionSequence(package,-2,ui);
    else if (rc == ERROR_INSTALL_SUSPEND) 
        ACTION_PerformActionSequence(package,-4,ui);
    else  /* failed */
        ACTION_PerformActionSequence(package,-3,ui);

    /* finish up running custom actions */
    ACTION_FinishCustomActions(package);
    
    return rc;
}

static UINT ACTION_PerformActionSequence(MSIPACKAGE *package, UINT seq, BOOL UI)
{
    UINT rc = ERROR_SUCCESS;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','I','n','s','t','a','l','l','E','x','e','c','u','t','e',
         'S','e','q','u','e','n','c','e','`',' ', 'W','H','E','R','E',' ',
         '`','S','e','q','u','e','n','c','e','`',' ', '=',' ','%','i',0};

    static const WCHAR UISeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
     '`','I','n','s','t','a','l','l','U','I','S','e','q','u','e','n','c','e',
     '`', ' ', 'W','H','E','R','E',' ','`','S','e','q','u','e','n','c','e','`',
	 ' ', '=',' ','%','i',0};

    if (UI)
        row = MSI_QueryGetRecord(package->db, UISeqQuery, seq);
    else
        row = MSI_QueryGetRecord(package->db, ExecSeqQuery, seq);

    if (row)
    {
        LPCWSTR action, cond;

        TRACE("Running the actions\n"); 

        /* check conditions */
        cond = MSI_RecordGetString(row,2);

        /* this is a hack to skip errors in the condition code */
        if (MSI_EvaluateConditionW(package, cond) == MSICONDITION_FALSE)
            goto end;

        action = MSI_RecordGetString(row,1);
        if (!action)
        {
            ERR("failed to fetch action\n");
            rc = ERROR_FUNCTION_FAILED;
            goto end;
        }

        if (UI)
            rc = ACTION_PerformUIAction(package,action);
        else
            rc = ACTION_PerformAction(package,action,FALSE);
end:
        msiobj_release(&row->hdr);
    }
    else
        rc = ERROR_SUCCESS;

    return rc;
}

typedef struct {
    MSIPACKAGE* package;
    BOOL UI;
} iterate_action_param;

static UINT ITERATE_Actions(MSIRECORD *row, LPVOID param)
{
    iterate_action_param *iap= (iterate_action_param*)param;
    UINT rc;
    LPCWSTR cond, action;

    action = MSI_RecordGetString(row,1);
    if (!action)
    {
        ERR("Error is retrieving action name\n");
        return ERROR_FUNCTION_FAILED;
    }

    /* check conditions */
    cond = MSI_RecordGetString(row,2);

    /* this is a hack to skip errors in the condition code */
    if (MSI_EvaluateConditionW(iap->package, cond) == MSICONDITION_FALSE)
    {
        TRACE("Skipping action: %s (condition is false)\n", debugstr_w(action));
        return ERROR_SUCCESS;
    }

    if (iap->UI)
        rc = ACTION_PerformUIAction(iap->package,action);
    else
        rc = ACTION_PerformAction(iap->package,action,FALSE);

    msi_dialog_check_messages( NULL );

    if (iap->package->CurrentInstallState != ERROR_SUCCESS )
        rc = iap->package->CurrentInstallState;

    if (rc == ERROR_FUNCTION_NOT_CALLED)
        rc = ERROR_SUCCESS;

    if (rc != ERROR_SUCCESS)
        ERR("Execution halted, action %s returned %i\n", debugstr_w(action), rc);

    return rc;
}

UINT MSI_Sequence( MSIPACKAGE *package, LPCWSTR szTable, INT iSequenceMode )
{
    MSIQUERY * view;
    UINT r;
    static const WCHAR query[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','%','s','`',
         ' ','W','H','E','R','E',' ', 
         '`','S','e','q','u','e','n','c','e','`',' ',
         '>',' ','0',' ','O','R','D','E','R',' ','B','Y',' ',
         '`','S','e','q','u','e','n','c','e','`',0};
    iterate_action_param iap;

    /*
     * FIXME: probably should be checking UILevel in the
     *       ACTION_PerformUIAction/ACTION_PerformAction
     *       rather than saving the UI level here. Those
     *       two functions can be merged too.
     */
    iap.package = package;
    iap.UI = TRUE;

    TRACE("%p %s %i\n", package, debugstr_w(szTable), iSequenceMode );

    r = MSI_OpenQuery( package->db, &view, query, szTable );
    if (r == ERROR_SUCCESS)
    {
        r = MSI_IterateRecords( view, NULL, ITERATE_Actions, &iap );
        msiobj_release(&view->hdr);
    }

    return r;
}

static UINT ACTION_ProcessExecSequence(MSIPACKAGE *package, BOOL UIran)
{
    MSIQUERY * view;
    UINT rc;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ', 'F','R','O','M',' ',
         '`','I','n','s','t','a','l','l','E','x','e','c','u','t','e',
         'S','e','q','u','e','n','c','e','`',' ', 'W','H','E','R','E',' ',
         '`','S','e','q','u','e','n','c','e','`',' ', '>',' ','%','i',' ',
         'O','R','D','E','R',' ', 'B','Y',' ',
         '`','S','e','q','u','e','n','c','e','`',0 };
    MSIRECORD * row = 0;
    static const WCHAR IVQuery[] =
        {'S','E','L','E','C','T',' ','`','S','e','q','u','e','n','c','e','`',
         ' ', 'F','R','O','M',' ','`','I','n','s','t','a','l','l',
         'E','x','e','c','u','t','e','S','e','q','u','e','n','c','e','`',' ',
         'W','H','E','R','E',' ','`','A','c','t','i','o','n','`',' ','=',
         ' ','\'', 'I','n','s','t','a','l','l',
         'V','a','l','i','d','a','t','e','\'', 0};
    INT seq = 0;
    iterate_action_param iap;

    iap.package = package;
    iap.UI = FALSE;

    if (package->script->ExecuteSequenceRun)
    {
        TRACE("Execute Sequence already Run\n");
        return ERROR_SUCCESS;
    }

    package->script->ExecuteSequenceRun = TRUE;

    /* get the sequence number */
    if (UIran)
    {
        row = MSI_QueryGetRecord(package->db, IVQuery);
        if( !row )
            return ERROR_FUNCTION_FAILED;
        seq = MSI_RecordGetInteger(row,1);
        msiobj_release(&row->hdr);
    }

    rc = MSI_OpenQuery(package->db, &view, ExecSeqQuery, seq);
    if (rc == ERROR_SUCCESS)
    {
        TRACE("Running the actions\n");

        rc = MSI_IterateRecords(view, NULL, ITERATE_Actions, &iap);
        msiobj_release(&view->hdr);
    }

    return rc;
}

static UINT ACTION_ProcessUISequence(MSIPACKAGE *package)
{
    MSIQUERY * view;
    UINT rc;
    static const WCHAR ExecSeqQuery [] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','I','n','s','t','a','l','l',
         'U','I','S','e','q','u','e','n','c','e','`',
         ' ','W','H','E','R','E',' ', 
         '`','S','e','q','u','e','n','c','e','`',' ',
         '>',' ','0',' ','O','R','D','E','R',' ','B','Y',' ',
         '`','S','e','q','u','e','n','c','e','`',0};
    iterate_action_param iap;

    iap.package = package;
    iap.UI = TRUE;

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    
    if (rc == ERROR_SUCCESS)
    {
        TRACE("Running the actions\n"); 

        rc = MSI_IterateRecords(view, NULL, ITERATE_Actions, &iap);
        msiobj_release(&view->hdr);
    }

    return rc;
}

/********************************************************
 * ACTION helper functions and functions that perform the actions
 *******************************************************/
static BOOL ACTION_HandleStandardAction(MSIPACKAGE *package, LPCWSTR action, 
                                        UINT* rc, BOOL force )
{
    BOOL ret = FALSE; 
    BOOL run = force;
    int i;

    if (!run && !package->script->CurrentlyScripting)
        run = TRUE;
   
    if (!run)
    {
        if (strcmpW(action,szInstallFinalize) == 0 ||
            strcmpW(action,szInstallExecute) == 0 ||
            strcmpW(action,szInstallExecuteAgain) == 0) 
                run = TRUE;
    }
    
    i = 0;
    while (StandardActions[i].action != NULL)
    {
        if (strcmpW(StandardActions[i].action, action)==0)
        {
            if (!run)
            {
                ui_actioninfo(package, action, TRUE, 0);
                *rc = schedule_action(package,INSTALL_SCRIPT,action);
                ui_actioninfo(package, action, FALSE, *rc);
            }
            else
            {
                ui_actionstart(package, action);
                if (StandardActions[i].handler)
                {
                    *rc = StandardActions[i].handler(package);
                }
                else
                {
                    FIXME("unhandled standard action %s\n",debugstr_w(action));
                    *rc = ERROR_SUCCESS;
                }
            }
            ret = TRUE;
            break;
        }
        i++;
    }
    return ret;
}

static BOOL ACTION_HandleCustomAction( MSIPACKAGE* package, LPCWSTR action,
                                       UINT* rc, BOOL force )
{
    BOOL ret=FALSE;
    UINT arc;

    arc = ACTION_CustomAction(package,action, force);

    if (arc != ERROR_CALL_NOT_IMPLEMENTED)
    {
        *rc = arc;
        ret = TRUE;
    }
    return ret;
}

/* 
 * A lot of actions are really important even if they don't do anything
 * explicit... Lots of properties are set at the beginning of the installation
 * CostFinalize does a bunch of work to translate the directories and such
 * 
 * But until I get write access to the database that is hard, so I am going to
 * hack it to see if I can get something to run.
 */
UINT ACTION_PerformAction(MSIPACKAGE *package, const WCHAR *action, BOOL force)
{
    UINT rc = ERROR_SUCCESS; 
    BOOL handled;

    TRACE("Performing action (%s)\n",debugstr_w(action));

    handled = ACTION_HandleStandardAction(package, action, &rc, force);

    if (!handled)
        handled = ACTION_HandleCustomAction(package, action, &rc, force);

    if (!handled)
    {
        FIXME("unhandled msi action %s\n",debugstr_w(action));
        rc = ERROR_FUNCTION_NOT_CALLED;
    }

    return rc;
}

UINT ACTION_PerformUIAction(MSIPACKAGE *package, const WCHAR *action)
{
    UINT rc = ERROR_SUCCESS;
    BOOL handled = FALSE;

    TRACE("Performing action (%s)\n",debugstr_w(action));

    handled = ACTION_HandleStandardAction(package, action, &rc,TRUE);

    if (!handled)
        handled = ACTION_HandleCustomAction(package, action, &rc, FALSE);

    if( !handled && ACTION_DialogBox(package,action) == ERROR_SUCCESS )
        handled = TRUE;

    if (!handled)
    {
        FIXME("unhandled msi action %s\n",debugstr_w(action));
        rc = ERROR_FUNCTION_NOT_CALLED;
    }

    return rc;
}


/*
 * Actual Action Handlers
 */

static UINT ITERATE_CreateFolders(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    LPCWSTR dir;
    LPWSTR full_path;
    MSIRECORD *uirow;
    MSIFOLDER *folder;

    dir = MSI_RecordGetString(row,1);
    if (!dir)
    {
        ERR("Unable to get folder id\n");
        return ERROR_SUCCESS;
    }

    full_path = resolve_folder(package,dir,FALSE,FALSE,&folder);
    if (!full_path)
    {
        ERR("Unable to resolve folder id %s\n",debugstr_w(dir));
        return ERROR_SUCCESS;
    }

    TRACE("Folder is %s\n",debugstr_w(full_path));

    /* UI stuff */
    uirow = MSI_CreateRecord(1);
    MSI_RecordSetStringW(uirow,1,full_path);
    ui_actiondata(package,szCreateFolders,uirow);
    msiobj_release( &uirow->hdr );

    if (folder->State == 0)
        create_full_pathW(full_path);

    folder->State = 3;

    msi_free(full_path);
    return ERROR_SUCCESS;
}

/* FIXME: probably should merge this with the above function */
static UINT msi_create_directory( MSIPACKAGE* package, LPCWSTR dir )
{
    UINT rc = ERROR_SUCCESS;
    MSIFOLDER *folder;
    LPWSTR install_path;

    install_path = resolve_folder(package, dir, FALSE, FALSE, &folder);
    if (!install_path)
        return ERROR_FUNCTION_FAILED; 

    /* create the path */
    if (folder->State == 0)
    {
        create_full_pathW(install_path);
        folder->State = 2;
    }
    msi_free(install_path);

    return rc;
}

UINT msi_create_component_directories( MSIPACKAGE *package )
{
    MSICOMPONENT *comp;

    /* create all the folders required by the components are going to install */
    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
    {
        if (!ACTION_VerifyComponentForAction( comp, INSTALLSTATE_LOCAL))
            continue;
        msi_create_directory( package, comp->Directory );
    }

    return ERROR_SUCCESS;
}

/*
 * Also we cannot enable/disable components either, so for now I am just going 
 * to do all the directories for all the components.
 */
static UINT ACTION_CreateFolders(MSIPACKAGE *package)
{
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ',
         '`','D','i','r','e','c','t','o','r','y','_','`',
         ' ','F','R','O','M',' ',
         '`','C','r','e','a','t','e','F','o','l','d','e','r','`',0 };
    UINT rc;
    MSIQUERY *view;

    /* create all the empty folders specified in the CreateFolder table */
    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_CreateFolders, package);
    msiobj_release(&view->hdr);

    msi_create_component_directories( package );

    return rc;
}

static UINT load_component( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    MSICOMPONENT *comp;

    comp = msi_alloc_zero( sizeof(MSICOMPONENT) );
    if (!comp)
        return ERROR_FUNCTION_FAILED;

    list_add_tail( &package->components, &comp->entry );

    /* fill in the data */
    comp->Component = msi_dup_record_field( row, 1 );

    TRACE("Loading Component %s\n", debugstr_w(comp->Component));

    comp->ComponentId = msi_dup_record_field( row, 2 );
    comp->Directory = msi_dup_record_field( row, 3 );
    comp->Attributes = MSI_RecordGetInteger(row,4);
    comp->Condition = msi_dup_record_field( row, 5 );
    comp->KeyPath = msi_dup_record_field( row, 6 );

    comp->Installed = INSTALLSTATE_UNKNOWN;
    msi_component_set_state( comp, INSTALLSTATE_UNKNOWN );

    return ERROR_SUCCESS;
}

static UINT load_all_components( MSIPACKAGE *package )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R', 'O','M',' ', 
         '`','C','o','m','p','o','n','e','n','t','`',0 };
    MSIQUERY *view;
    UINT r;

    if (!list_empty(&package->components))
        return ERROR_SUCCESS;

    r = MSI_DatabaseOpenViewW( package->db, query, &view );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_IterateRecords(view, NULL, load_component, package);
    msiobj_release(&view->hdr);
    return r;
}

typedef struct {
    MSIPACKAGE *package;
    MSIFEATURE *feature;
} _ilfs;

static UINT add_feature_component( MSIFEATURE *feature, MSICOMPONENT *comp )
{
    ComponentList *cl;

    cl = msi_alloc( sizeof (*cl) );
    if ( !cl )
        return ERROR_NOT_ENOUGH_MEMORY;
    cl->component = comp;
    list_add_tail( &feature->Components, &cl->entry );

    return ERROR_SUCCESS;
}

static UINT add_feature_child( MSIFEATURE *parent, MSIFEATURE *child )
{
    FeatureList *fl;

    fl = msi_alloc( sizeof(*fl) );
    if ( !fl )
        return ERROR_NOT_ENOUGH_MEMORY;
    fl->feature = child;
    list_add_tail( &parent->Children, &fl->entry );

    return ERROR_SUCCESS;
}

static UINT iterate_load_featurecomponents(MSIRECORD *row, LPVOID param)
{
    _ilfs* ilfs= (_ilfs*)param;
    LPCWSTR component;
    MSICOMPONENT *comp;

    component = MSI_RecordGetString(row,1);

    /* check to see if the component is already loaded */
    comp = get_loaded_component( ilfs->package, component );
    if (!comp)
    {
        ERR("unknown component %s\n", debugstr_w(component));
        return ERROR_FUNCTION_FAILED;
    }

    add_feature_component( ilfs->feature, comp );
    comp->Enabled = TRUE;

    return ERROR_SUCCESS;
}

static MSIFEATURE *find_feature_by_name( MSIPACKAGE *package, LPCWSTR name )
{
    MSIFEATURE *feature;

    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        if ( !lstrcmpW( feature->Feature, name ) )
            return feature;
    }

    return NULL;
}

static UINT load_feature(MSIRECORD * row, LPVOID param)
{
    MSIPACKAGE* package = (MSIPACKAGE*)param;
    MSIFEATURE* feature;
    static const WCHAR Query1[] = 
        {'S','E','L','E','C','T',' ',
         '`','C','o','m','p','o','n','e','n','t','_','`',
         ' ','F','R','O','M',' ','`','F','e','a','t','u','r','e',
         'C','o','m','p','o','n','e','n','t','s','`',' ',
         'W','H','E','R','E',' ',
         '`','F','e', 'a','t','u','r','e','_','`',' ','=','\'','%','s','\'',0};
    MSIQUERY * view;
    UINT    rc;
    _ilfs ilfs;

    /* fill in the data */

    feature = msi_alloc_zero( sizeof (MSIFEATURE) );
    if (!feature)
        return ERROR_NOT_ENOUGH_MEMORY;

    list_init( &feature->Children );
    list_init( &feature->Components );
    
    feature->Feature = msi_dup_record_field( row, 1 );

    TRACE("Loading feature %s\n",debugstr_w(feature->Feature));

    feature->Feature_Parent = msi_dup_record_field( row, 2 );
    feature->Title = msi_dup_record_field( row, 3 );
    feature->Description = msi_dup_record_field( row, 4 );

    if (!MSI_RecordIsNull(row,5))
        feature->Display = MSI_RecordGetInteger(row,5);
  
    feature->Level= MSI_RecordGetInteger(row,6);
    feature->Directory = msi_dup_record_field( row, 7 );
    feature->Attributes = MSI_RecordGetInteger(row,8);

    feature->Installed = INSTALLSTATE_UNKNOWN;
    msi_feature_set_state( feature, INSTALLSTATE_UNKNOWN );

    list_add_tail( &package->features, &feature->entry );

    /* load feature components */

    rc = MSI_OpenQuery( package->db, &view, Query1, feature->Feature );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    ilfs.package = package;
    ilfs.feature = feature;

    MSI_IterateRecords(view, NULL, iterate_load_featurecomponents , &ilfs);
    msiobj_release(&view->hdr);

    return ERROR_SUCCESS;
}

static UINT find_feature_children(MSIRECORD * row, LPVOID param)
{
    MSIPACKAGE* package = (MSIPACKAGE*)param;
    MSIFEATURE *parent, *child;

    child = find_feature_by_name( package, MSI_RecordGetString( row, 1 ) );
    if (!child)
        return ERROR_FUNCTION_FAILED;

    if (!child->Feature_Parent)
        return ERROR_SUCCESS;

    parent = find_feature_by_name( package, child->Feature_Parent );
    if (!parent)
        return ERROR_FUNCTION_FAILED;

    add_feature_child( parent, child );
    return ERROR_SUCCESS;
}

static UINT load_all_features( MSIPACKAGE *package )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ', 'F','R','O','M',' ',
        '`','F','e','a','t','u','r','e','`',' ','O','R','D','E','R',
        ' ','B','Y',' ','`','D','i','s','p','l','a','y','`',0};
    MSIQUERY *view;
    UINT r;

    if (!list_empty(&package->features))
        return ERROR_SUCCESS;
 
    r = MSI_DatabaseOpenViewW( package->db, query, &view );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_IterateRecords( view, NULL, load_feature, package );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_IterateRecords( view, NULL, find_feature_children, package );
    msiobj_release( &view->hdr );

    return r;
}

static LPWSTR folder_split_path(LPWSTR p, WCHAR ch)
{
    if (!p)
        return p;
    p = strchrW(p, ch);
    if (!p)
        return p;
    *p = 0;
    return p+1;
}

static UINT load_file(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE* package = (MSIPACKAGE*)param;
    LPCWSTR component;
    MSIFILE *file;

    /* fill in the data */

    file = msi_alloc_zero( sizeof (MSIFILE) );
    if (!file)
        return ERROR_NOT_ENOUGH_MEMORY;
 
    file->File = msi_dup_record_field( row, 1 );

    component = MSI_RecordGetString( row, 2 );
    file->Component = get_loaded_component( package, component );

    if (!file->Component)
        ERR("Unfound Component %s\n",debugstr_w(component));

    file->FileName = msi_dup_record_field( row, 3 );
    reduce_to_longfilename( file->FileName );

    file->ShortName = msi_dup_record_field( row, 3 );
    file->LongName = strdupW( folder_split_path(file->ShortName, '|'));
    
    file->FileSize = MSI_RecordGetInteger( row, 4 );
    file->Version = msi_dup_record_field( row, 5 );
    file->Language = msi_dup_record_field( row, 6 );
    file->Attributes = MSI_RecordGetInteger( row, 7 );
    file->Sequence = MSI_RecordGetInteger( row, 8 );

    file->state = msifs_invalid;

    /* if the compressed bits are not set in the file attributes,
     * then read the information from the package word count property
     */
    if (file->Attributes & msidbFileAttributesCompressed)
    {
        file->IsCompressed = TRUE;
    }
    else if (file->Attributes & msidbFileAttributesNoncompressed)
    {
        file->IsCompressed = FALSE;
    }
    else
    {
        file->IsCompressed = package->WordCount & MSIWORDCOUNT_COMPRESSED;
    }

    TRACE("File Loaded (%s)\n",debugstr_w(file->File));  

    list_add_tail( &package->files, &file->entry );
 
    return ERROR_SUCCESS;
}

static UINT load_all_files(MSIPACKAGE *package)
{
    MSIQUERY * view;
    UINT rc;
    static const WCHAR Query[] =
        {'S','E','L','E','C','T',' ','*',' ', 'F','R','O','M',' ',
         '`','F','i','l','e','`',' ', 'O','R','D','E','R',' ','B','Y',' ',
         '`','S','e','q','u','e','n','c','e','`', 0};

    if (!list_empty(&package->files))
        return ERROR_SUCCESS;

    rc = MSI_DatabaseOpenViewW(package->db, Query, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, load_file, package);
    msiobj_release(&view->hdr);

    return ERROR_SUCCESS;
}


/*
 * I am not doing any of the costing functionality yet.
 * Mostly looking at doing the Component and Feature loading
 *
 * The native MSI does A LOT of modification to tables here. Mostly adding
 * a lot of temporary columns to the Feature and Component tables.
 *
 *    note: Native msi also tracks the short filename. But I am only going to
 *          track the long ones.  Also looking at this directory table
 *          it appears that the directory table does not get the parents
 *          resolved base on property only based on their entries in the
 *          directory table.
 */
static UINT ACTION_CostInitialize(MSIPACKAGE *package)
{
    static const WCHAR szCosting[] =
        {'C','o','s','t','i','n','g','C','o','m','p','l','e','t','e',0 };
    static const WCHAR szZero[] = { '0', 0 };

    if ( 1 == msi_get_property_int( package, szCosting, 0 ) )
        return ERROR_SUCCESS;

    MSI_SetPropertyW(package, szCosting, szZero);
    MSI_SetPropertyW(package, cszRootDrive, c_colon);

    load_all_components( package );
    load_all_features( package );
    load_all_files( package );

    return ERROR_SUCCESS;
}

static UINT execute_script(MSIPACKAGE *package, UINT script )
{
    int i;
    UINT rc = ERROR_SUCCESS;

    TRACE("Executing Script %i\n",script);

    if (!package->script)
    {
        ERR("no script!\n");
        return ERROR_FUNCTION_FAILED;
    }

    for (i = 0; i < package->script->ActionCount[script]; i++)
    {
        LPWSTR action;
        action = package->script->Actions[script][i];
        ui_actionstart(package, action);
        TRACE("Executing Action (%s)\n",debugstr_w(action));
        rc = ACTION_PerformAction(package, action, TRUE);
        if (rc != ERROR_SUCCESS)
            break;
    }
    msi_free_action_script(package, script);
    return rc;
}

static UINT ACTION_FileCost(MSIPACKAGE *package)
{
    return ERROR_SUCCESS;
}

static MSIFOLDER *load_folder( MSIPACKAGE *package, LPCWSTR dir )
{
    static const WCHAR Query[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','D','i','r','e','c', 't','o','r','y','`',' ',
         'W','H','E','R','E',' ', '`', 'D','i','r','e','c','t', 'o','r','y','`',
         ' ','=',' ','\'','%','s','\'',
         0};
    static const WCHAR szDot[] = { '.',0 };
    static WCHAR szEmpty[] = { 0 };
    LPWSTR p, tgt_short, tgt_long, src_short, src_long;
    LPCWSTR parent;
    MSIRECORD *row;
    MSIFOLDER *folder;

    TRACE("Looking for dir %s\n",debugstr_w(dir));

    folder = get_loaded_folder( package, dir );
    if (folder)
        return folder;

    TRACE("Working to load %s\n",debugstr_w(dir));

    folder = msi_alloc_zero( sizeof (MSIFOLDER) );
    if (!folder)
        return NULL;

    folder->Directory = strdupW(dir);

    row = MSI_QueryGetRecord(package->db, Query, dir);
    if (!row)
        return NULL;

    p = msi_dup_record_field(row, 3);

    /* split src and target dir */
    tgt_short = p;
    src_short = folder_split_path( p, ':' );

    /* split the long and short paths */
    tgt_long = folder_split_path( tgt_short, '|' );
    src_long = folder_split_path( src_short, '|' );

    /* check for no-op dirs */
    if (!lstrcmpW(szDot, tgt_short))
        tgt_short = szEmpty;
    if (!lstrcmpW(szDot, src_short))
        src_short = szEmpty;

    if (!tgt_long)
        tgt_long = tgt_short;
	
    if (!src_short) {
        src_short = tgt_short;
        src_long = tgt_long;
    }
    
    if (!src_long)
        src_long = src_short;

    /* FIXME: use the target short path too */
    folder->TargetDefault = strdupW(tgt_long);
    folder->SourceShortPath = strdupW(src_short);
    folder->SourceLongPath = strdupW(src_long);
    msi_free(p);

    TRACE("TargetDefault = %s\n",debugstr_w( folder->TargetDefault ));
    TRACE("SourceLong = %s\n", debugstr_w( folder->SourceLongPath ));
    TRACE("SourceShort = %s\n", debugstr_w( folder->SourceShortPath ));

    parent = MSI_RecordGetString(row, 2);
    if (parent) 
    {
        folder->Parent = load_folder( package, parent );
        if ( folder->Parent )
            TRACE("loaded parent %p %s\n", folder->Parent,
                  debugstr_w(folder->Parent->Directory));
        else
            ERR("failed to load parent folder %s\n", debugstr_w(parent));
    }

    folder->Property = msi_dup_property( package, dir );

    msiobj_release(&row->hdr);

    list_add_tail( &package->folders, &folder->entry );

    TRACE("%s returning %p\n",debugstr_w(dir),folder);

    return folder;
}

static void ACTION_GetComponentInstallStates(MSIPACKAGE *package)
{
    MSICOMPONENT *comp;

    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
    {
        INSTALLSTATE res;

        if (!comp->ComponentId)
            continue;

        res = MsiGetComponentPathW( package->ProductCode,
                                    comp->ComponentId, NULL, NULL);
        if (res < 0)
            res = INSTALLSTATE_ABSENT;
        comp->Installed = res;
    }
}

/* scan for and update current install states */
static void ACTION_UpdateFeatureInstallStates(MSIPACKAGE *package)
{
    MSICOMPONENT *comp;
    MSIFEATURE *feature;

    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        ComponentList *cl;
        INSTALLSTATE res = INSTALLSTATE_ABSENT;

        LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
        {
            comp= cl->component;

            if (!comp->ComponentId)
            {
                res = INSTALLSTATE_ABSENT;
                break;
            }

            if (res == INSTALLSTATE_ABSENT)
                res = comp->Installed;
            else
            {
                if (res == comp->Installed)
                    continue;

                if (res != INSTALLSTATE_DEFAULT || res != INSTALLSTATE_LOCAL ||
                    res != INSTALLSTATE_SOURCE)
                {
                    res = INSTALLSTATE_INCOMPLETE;
                }
            }
        }
        feature->Installed = res;
    }
}

static BOOL process_state_property (MSIPACKAGE* package, LPCWSTR property, 
                                    INSTALLSTATE state)
{
    static const WCHAR all[]={'A','L','L',0};
    LPWSTR override;
    MSIFEATURE *feature;

    override = msi_dup_property( package, property );
    if (!override)
        return FALSE;

    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        if (strcmpiW(override,all)==0)
            msi_feature_set_state( feature, state );
        else
        {
            LPWSTR ptr = override;
            LPWSTR ptr2 = strchrW(override,',');

            while (ptr)
            {
                if ((ptr2 && strncmpW(ptr,feature->Feature, ptr2-ptr)==0)
                    || (!ptr2 && strcmpW(ptr,feature->Feature)==0))
                {
                    msi_feature_set_state( feature, state );
                    break;
                }
                if (ptr2)
                {
                    ptr=ptr2+1;
                    ptr2 = strchrW(ptr,',');
                }
                else
                    break;
            }
        }
    }
    msi_free(override);

    return TRUE;
}

UINT MSI_SetFeatureStates(MSIPACKAGE *package)
{
    int install_level;
    static const WCHAR szlevel[] =
        {'I','N','S','T','A','L','L','L','E','V','E','L',0};
    static const WCHAR szAddLocal[] =
        {'A','D','D','L','O','C','A','L',0};
    static const WCHAR szRemove[] =
        {'R','E','M','O','V','E',0};
    static const WCHAR szReinstall[] =
        {'R','E','I','N','S','T','A','L','L',0};
    BOOL override = FALSE;
    MSICOMPONENT* component;
    MSIFEATURE *feature;


    /* I do not know if this is where it should happen.. but */

    TRACE("Checking Install Level\n");

    install_level = msi_get_property_int( package, szlevel, 1 );

    /* ok here is the _real_ rub
     * all these activation/deactivation things happen in order and things
     * later on the list override things earlier on the list.
     * 1) INSTALLLEVEL processing
     * 2) ADDLOCAL
     * 3) REMOVE
     * 4) ADDSOURCE
     * 5) ADDDEFAULT
     * 6) REINSTALL
     * 7) COMPADDLOCAL
     * 8) COMPADDSOURCE
     * 9) FILEADDLOCAL
     * 10) FILEADDSOURCE
     * 11) FILEADDDEFAULT
     * I have confirmed that if ADDLOCAL is stated then the INSTALLLEVEL is
     * ignored for all the features. seems strange, especially since it is not
     * documented anywhere, but it is how it works.
     *
     * I am still ignoring a lot of these. But that is ok for now, ADDLOCAL and
     * REMOVE are the big ones, since we don't handle administrative installs
     * yet anyway.
     */
    override |= process_state_property(package,szAddLocal,INSTALLSTATE_LOCAL);
    override |= process_state_property(package,szRemove,INSTALLSTATE_ABSENT);
    override |= process_state_property(package,szReinstall,INSTALLSTATE_LOCAL);

    if (!override)
    {
        LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
        {
            BOOL feature_state = ((feature->Level > 0) &&
                             (feature->Level <= install_level));

            if ((feature_state) && (feature->Action == INSTALLSTATE_UNKNOWN))
            {
                if (feature->Attributes & msidbFeatureAttributesFavorSource)
                    msi_feature_set_state( feature, INSTALLSTATE_SOURCE );
                else if (feature->Attributes & msidbFeatureAttributesFavorAdvertise)
                    msi_feature_set_state( feature, INSTALLSTATE_ADVERTISED );
                else
                    msi_feature_set_state( feature, INSTALLSTATE_LOCAL );
            }
        }

        /* disable child features of unselected parent features */
        LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
        {
            FeatureList *fl;

            if (feature->Level > 0 && feature->Level <= install_level)
                continue;

            LIST_FOR_EACH_ENTRY( fl, &feature->Children, FeatureList, entry )
                msi_feature_set_state( fl->feature, INSTALLSTATE_UNKNOWN );
        }
    }
    else
    {
        /* set the Preselected Property */
        static const WCHAR szPreselected[] = {'P','r','e','s','e','l','e','c','t','e','d',0};
        static const WCHAR szOne[] = { '1', 0 };

        MSI_SetPropertyW(package,szPreselected,szOne);
    }

    /*
     * now we want to enable or disable components base on feature
     */

    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        ComponentList *cl;

        TRACE("Examining Feature %s (Installed %i, Action %i)\n",
            debugstr_w(feature->Feature), feature->Installed, feature->Action);

        LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
        {
            component = cl->component;

            if (!component->Enabled)
                continue;

            if (component->Attributes & msidbComponentAttributesOptional)
                msi_component_set_state( component, INSTALLSTATE_DEFAULT );
            else
            {
                if (component->Attributes & msidbComponentAttributesSourceOnly)
                    msi_component_set_state( component, INSTALLSTATE_SOURCE );
                else
                    msi_component_set_state( component, INSTALLSTATE_LOCAL );
            }

            if (component->ForceLocalState)
                msi_component_set_state( component, INSTALLSTATE_LOCAL );

            if (feature->Attributes == msidbFeatureAttributesFavorLocal)
            {
                if (!(component->Attributes & msidbComponentAttributesSourceOnly))
                    msi_component_set_state( component, INSTALLSTATE_LOCAL );
            }
            else if (feature->Attributes == msidbFeatureAttributesFavorSource)
            {
                if ((component->Action == INSTALLSTATE_UNKNOWN) ||
                    (component->Action == INSTALLSTATE_ABSENT) ||
                    (component->Action == INSTALLSTATE_ADVERTISED) ||
                    (component->Action == INSTALLSTATE_DEFAULT))
                    msi_component_set_state( component, INSTALLSTATE_SOURCE );
            }
            else if (feature->ActionRequest == INSTALLSTATE_ADVERTISED)
            {
                if ((component->Action == INSTALLSTATE_UNKNOWN) ||
                    (component->Action == INSTALLSTATE_ABSENT))
                    msi_component_set_state( component, INSTALLSTATE_ADVERTISED );
            }
            else if (feature->ActionRequest == INSTALLSTATE_ABSENT)
            {
                if (component->Action == INSTALLSTATE_UNKNOWN)
                    msi_component_set_state( component, INSTALLSTATE_ABSENT );
            }
            else if (feature->ActionRequest == INSTALLSTATE_UNKNOWN)
                msi_component_set_state( component, INSTALLSTATE_UNKNOWN );

            if (component->ForceLocalState && feature->Action == INSTALLSTATE_SOURCE)
                msi_feature_set_state( feature, INSTALLSTATE_LOCAL );
        }
    }

    LIST_FOR_EACH_ENTRY( component, &package->components, MSICOMPONENT, entry )
    {
        if (component->Action == INSTALLSTATE_DEFAULT)
        {
            TRACE("%s was default, setting to local\n", debugstr_w(component->Component));
            msi_component_set_state( component, INSTALLSTATE_LOCAL );
        }

        TRACE("Result: Component %s (Installed %i, Action %i)\n",
            debugstr_w(component->Component), component->Installed, component->Action);
    }


    return ERROR_SUCCESS;
}

static UINT ITERATE_CostFinalizeDirectories(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    LPCWSTR name;
    LPWSTR path;

    name = MSI_RecordGetString(row,1);

    /* This helper function now does ALL the work */
    TRACE("Dir %s ...\n",debugstr_w(name));
    load_folder(package,name);
    path = resolve_folder(package,name,FALSE,TRUE,NULL);
    TRACE("resolves to %s\n",debugstr_w(path));
    msi_free(path);

    return ERROR_SUCCESS;
}

static UINT ITERATE_CostFinalizeConditions(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    LPCWSTR name;
    MSIFEATURE *feature;

    name = MSI_RecordGetString( row, 1 );

    feature = get_loaded_feature( package, name );
    if (!feature)
        ERR("FAILED to find loaded feature %s\n",debugstr_w(name));
    else
    {
        LPCWSTR Condition;
        Condition = MSI_RecordGetString(row,3);

        if (MSI_EvaluateConditionW(package,Condition) == MSICONDITION_TRUE)
        {
            int level = MSI_RecordGetInteger(row,2);
            TRACE("Reseting feature %s to level %i\n", debugstr_w(name), level);
            feature->Level = level;
        }
    }
    return ERROR_SUCCESS;
}

LPWSTR msi_get_disk_file_version( LPCWSTR filename )
{
    static const WCHAR name_fmt[] =
        {'%','u','.','%','u','.','%','u','.','%','u',0};
    static WCHAR name[] = {'\\',0};
    VS_FIXEDFILEINFO *lpVer;
    WCHAR filever[0x100];
    LPVOID version;
    DWORD versize;
    DWORD handle;
    UINT sz;

    TRACE("%s\n", debugstr_w(filename));

    versize = GetFileVersionInfoSizeW( filename, &handle );
    if (!versize)
        return NULL;

    version = msi_alloc( versize );
    GetFileVersionInfoW( filename, 0, versize, version );

    VerQueryValueW( version, name, (LPVOID*)&lpVer, &sz );
    msi_free( version );

    sprintfW( filever, name_fmt,
        HIWORD(lpVer->dwFileVersionMS),
        LOWORD(lpVer->dwFileVersionMS),
        HIWORD(lpVer->dwFileVersionLS),
        LOWORD(lpVer->dwFileVersionLS));

    return strdupW( filever );
}

static UINT msi_check_file_install_states( MSIPACKAGE *package )
{
    LPWSTR file_version;
    MSIFILE *file;

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        MSICOMPONENT* comp = file->Component;
        LPWSTR p;

        if (!comp)
            continue;

        if (file->IsCompressed)
            comp->ForceLocalState = TRUE;

        /* calculate target */
        p = resolve_folder(package, comp->Directory, FALSE, FALSE, NULL);

        msi_free(file->TargetPath);

        TRACE("file %s is named %s\n",
               debugstr_w(file->File), debugstr_w(file->FileName));

        file->TargetPath = build_directory_name(2, p, file->FileName);

        msi_free(p);

        TRACE("file %s resolves to %s\n",
               debugstr_w(file->File), debugstr_w(file->TargetPath));

        /* don't check files of components that aren't installed */
        if (comp->Installed == INSTALLSTATE_UNKNOWN ||
            comp->Installed == INSTALLSTATE_ABSENT)
        {
            file->state = msifs_missing;  /* assume files are missing */
            continue;
        }

        if (GetFileAttributesW(file->TargetPath) == INVALID_FILE_ATTRIBUTES)
        {
            file->state = msifs_missing;
            comp->Cost += file->FileSize;
            comp->Installed = INSTALLSTATE_INCOMPLETE;
            continue;
        }

        if (file->Version &&
            (file_version = msi_get_disk_file_version( file->TargetPath )))
        {
            TRACE("new %s old %s\n", debugstr_w(file->Version),
                  debugstr_w(file_version));
            /* FIXME: seems like a bad way to compare version numbers */
            if (lstrcmpiW(file_version, file->Version)<0)
            {
                file->state = msifs_overwrite;
                comp->Cost += file->FileSize;
                comp->Installed = INSTALLSTATE_INCOMPLETE;
            }
            else
                file->state = msifs_present;
            msi_free( file_version );
        }
        else
            file->state = msifs_present;
    }

    return ERROR_SUCCESS;
}

/*
 * A lot is done in this function aside from just the costing.
 * The costing needs to be implemented at some point but for now I am going
 * to focus on the directory building
 *
 */
static UINT ACTION_CostFinalize(MSIPACKAGE *package)
{
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','D','i','r','e','c','t','o','r','y','`',0};
    static const WCHAR ConditionQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','C','o','n','d','i','t','i','o','n','`',0};
    static const WCHAR szCosting[] =
        {'C','o','s','t','i','n','g','C','o','m','p','l','e','t','e',0 };
    static const WCHAR szlevel[] =
        {'I','N','S','T','A','L','L','L','E','V','E','L',0};
    static const WCHAR szOne[] = { '1', 0 };
    MSICOMPONENT *comp;
    UINT rc;
    MSIQUERY * view;
    LPWSTR level;

    if ( 1 == msi_get_property_int( package, szCosting, 0 ) )
        return ERROR_SUCCESS;

    TRACE("Building Directory properties\n");

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords(view, NULL, ITERATE_CostFinalizeDirectories,
                        package);
        msiobj_release(&view->hdr);
    }

    /* read components states from the registry */
    ACTION_GetComponentInstallStates(package);

    TRACE("File calculations\n");
    msi_check_file_install_states( package );

    TRACE("Evaluating Condition Table\n");

    rc = MSI_DatabaseOpenViewW(package->db, ConditionQuery, &view);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords(view, NULL, ITERATE_CostFinalizeConditions,
                    package);
        msiobj_release(&view->hdr);
    }

    TRACE("Enabling or Disabling Components\n");
    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
    {
        if (MSI_EvaluateConditionW(package, comp->Condition) == MSICONDITION_FALSE)
        {
            TRACE("Disabling component %s\n", debugstr_w(comp->Component));
            comp->Enabled = FALSE;
        }
    }

    MSI_SetPropertyW(package,szCosting,szOne);
    /* set default run level if not set */
    level = msi_dup_property( package, szlevel );
    if (!level)
        MSI_SetPropertyW(package,szlevel, szOne);
    msi_free(level);

    ACTION_UpdateFeatureInstallStates(package);

    return MSI_SetFeatureStates(package);
}

/* OK this value is "interpreted" and then formatted based on the 
   first few characters */
static LPSTR parse_value(MSIPACKAGE *package, LPCWSTR value, DWORD *type, 
                         DWORD *size)
{
    LPSTR data = NULL;
    if (value[0]=='#' && value[1]!='#' && value[1]!='%')
    {
        if (value[1]=='x')
        {
            LPWSTR ptr;
            CHAR byte[5];
            LPWSTR deformated = NULL;
            int count;

            deformat_string(package, &value[2], &deformated);

            /* binary value type */
            ptr = deformated;
            *type = REG_BINARY;
            if (strlenW(ptr)%2)
                *size = (strlenW(ptr)/2)+1;
            else
                *size = strlenW(ptr)/2;

            data = msi_alloc(*size);

            byte[0] = '0'; 
            byte[1] = 'x'; 
            byte[4] = 0; 
            count = 0;
            /* if uneven pad with a zero in front */
            if (strlenW(ptr)%2)
            {
                byte[2]= '0';
                byte[3]= *ptr;
                ptr++;
                data[count] = (BYTE)strtol(byte,NULL,0);
                count ++;
                TRACE("Uneven byte count\n");
            }
            while (*ptr)
            {
                byte[2]= *ptr;
                ptr++;
                byte[3]= *ptr;
                ptr++;
                data[count] = (BYTE)strtol(byte,NULL,0);
                count ++;
            }
            msi_free(deformated);

            TRACE("Data %i bytes(%i)\n",*size,count);
        }
        else
        {
            LPWSTR deformated;
            LPWSTR p;
            DWORD d = 0;
            deformat_string(package, &value[1], &deformated);

            *type=REG_DWORD; 
            *size = sizeof(DWORD);
            data = msi_alloc(*size);
            p = deformated;
            if (*p == '-')
                p++;
            while (*p)
            {
                if ( (*p < '0') || (*p > '9') )
                    break;
                d *= 10;
                d += (*p - '0');
                p++;
            }
            if (deformated[0] == '-')
                d = -d;
            *(LPDWORD)data = d;
            TRACE("DWORD %i\n",*(LPDWORD)data);

            msi_free(deformated);
        }
    }
    else
    {
        static const WCHAR szMulti[] = {'[','~',']',0};
        LPCWSTR ptr;
        *type=REG_SZ;

        if (value[0]=='#')
        {
            if (value[1]=='%')
            {
                ptr = &value[2];
                *type=REG_EXPAND_SZ;
            }
            else
                ptr = &value[1];
         }
         else
            ptr=value;

        if (strstrW(value,szMulti))
            *type = REG_MULTI_SZ;

        *size = deformat_string(package, ptr,(LPWSTR*)&data);
    }
    return data;
}

static UINT ITERATE_WriteRegistryValues(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    static const WCHAR szHCR[] = 
        {'H','K','E','Y','_','C','L','A','S','S','E','S','_',
         'R','O','O','T','\\',0};
    static const WCHAR szHCU[] =
        {'H','K','E','Y','_','C','U','R','R','E','N','T','_',
         'U','S','E','R','\\',0};
    static const WCHAR szHLM[] =
        {'H','K','E','Y','_','L','O','C','A','L','_',
         'M','A','C','H','I','N','E','\\',0};
    static const WCHAR szHU[] =
        {'H','K','E','Y','_','U','S','E','R','S','\\',0};

    LPSTR value_data = NULL;
    HKEY  root_key, hkey;
    DWORD type,size;
    LPWSTR  deformated;
    LPCWSTR szRoot, component, name, key, value;
    MSICOMPONENT *comp;
    MSIRECORD * uirow;
    LPWSTR uikey;
    INT   root;
    BOOL check_first = FALSE;
    UINT rc;

    ui_progress(package,2,0,0,0);

    value = NULL;
    key = NULL;
    uikey = NULL;
    name = NULL;

    component = MSI_RecordGetString(row, 6);
    comp = get_loaded_component(package,component);
    if (!comp)
        return ERROR_SUCCESS;

    if (!ACTION_VerifyComponentForAction( comp, INSTALLSTATE_LOCAL))
    {
        TRACE("Skipping write due to disabled component %s\n",
                        debugstr_w(component));

        comp->Action = comp->Installed;

        return ERROR_SUCCESS;
    }

    comp->Action = INSTALLSTATE_LOCAL;

    name = MSI_RecordGetString(row, 4);
    if( MSI_RecordIsNull(row,5) && name )
    {
        /* null values can have special meanings */
        if (name[0]=='-' && name[1] == 0)
                return ERROR_SUCCESS;
        else if ((name[0]=='+' && name[1] == 0) || 
                 (name[0] == '*' && name[1] == 0))
                name = NULL;
        check_first = TRUE;
    }

    root = MSI_RecordGetInteger(row,2);
    key = MSI_RecordGetString(row, 3);

    /* get the root key */
    switch (root)
    {
        case -1: 
            {
                static const WCHAR szALLUSER[] = {'A','L','L','U','S','E','R','S',0};
                LPWSTR all_users = msi_dup_property( package, szALLUSER );
                if (all_users && all_users[0] == '1')
                {
                    root_key = HKEY_LOCAL_MACHINE;
                    szRoot = szHLM;
                }
                else
                {
                    root_key = HKEY_CURRENT_USER;
                    szRoot = szHCU;
                }
                msi_free(all_users);
            }
                 break;
        case 0:  root_key = HKEY_CLASSES_ROOT; 
                 szRoot = szHCR;
                 break;
        case 1:  root_key = HKEY_CURRENT_USER;
                 szRoot = szHCU;
                 break;
        case 2:  root_key = HKEY_LOCAL_MACHINE;
                 szRoot = szHLM;
                 break;
        case 3:  root_key = HKEY_USERS; 
                 szRoot = szHU;
                 break;
        default:
                 ERR("Unknown root %i\n",root);
                 root_key=NULL;
                 szRoot = NULL;
                 break;
    }
    if (!root_key)
        return ERROR_SUCCESS;

    deformat_string(package, key , &deformated);
    size = strlenW(deformated) + strlenW(szRoot) + 1;
    uikey = msi_alloc(size*sizeof(WCHAR));
    strcpyW(uikey,szRoot);
    strcatW(uikey,deformated);

    if (RegCreateKeyW( root_key, deformated, &hkey))
    {
        ERR("Could not create key %s\n",debugstr_w(deformated));
        msi_free(deformated);
        msi_free(uikey);
        return ERROR_SUCCESS;
    }
    msi_free(deformated);

    value = MSI_RecordGetString(row,5);
    if (value)
        value_data = parse_value(package, value, &type, &size); 
    else
    {
        static const WCHAR szEmpty[] = {0};
        value_data = (LPSTR)strdupW(szEmpty);
        size = 0;
        type = REG_SZ;
    }

    deformat_string(package, name, &deformated);

    /* get the double nulls to terminate SZ_MULTI */
    if (type == REG_MULTI_SZ)
        size +=sizeof(WCHAR);

    if (!check_first)
    {
        TRACE("Setting value %s of %s\n",debugstr_w(deformated),
                        debugstr_w(uikey));
        RegSetValueExW(hkey, deformated, 0, type, (LPBYTE)value_data, size);
    }
    else
    {
        DWORD sz = 0;
        rc = RegQueryValueExW(hkey, deformated, NULL, NULL, NULL, &sz);
        if (rc == ERROR_SUCCESS || rc == ERROR_MORE_DATA)
        {
            TRACE("value %s of %s checked already exists\n",
                            debugstr_w(deformated), debugstr_w(uikey));
        }
        else
        {
            TRACE("Checked and setting value %s of %s\n",
                            debugstr_w(deformated), debugstr_w(uikey));
            if (deformated || size)
                RegSetValueExW(hkey, deformated, 0, type, (LPBYTE) value_data, size);
        }
    }
    RegCloseKey(hkey);

    uirow = MSI_CreateRecord(3);
    MSI_RecordSetStringW(uirow,2,deformated);
    MSI_RecordSetStringW(uirow,1,uikey);

    if (type == REG_SZ)
        MSI_RecordSetStringW(uirow,3,(LPWSTR)value_data);
    else
        MSI_RecordSetStringW(uirow,3,value);

    ui_actiondata(package,szWriteRegistryValues,uirow);
    msiobj_release( &uirow->hdr );

    msi_free(value_data);
    msi_free(deformated);
    msi_free(uikey);

    return ERROR_SUCCESS;
}

static UINT ACTION_WriteRegistryValues(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','R','e','g','i','s','t','r','y','`',0 };

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    /* increment progress bar each time action data is sent */
    ui_progress(package,1,REG_PROGRESS_VALUE,1,0);

    rc = MSI_IterateRecords(view, NULL, ITERATE_WriteRegistryValues, package);

    msiobj_release(&view->hdr);
    return rc;
}

static UINT ACTION_InstallInitialize(MSIPACKAGE *package)
{
    package->script->CurrentlyScripting = TRUE;

    return ERROR_SUCCESS;
}


static UINT ACTION_InstallValidate(MSIPACKAGE *package)
{
    MSICOMPONENT *comp;
    DWORD progress = 0;
    DWORD total = 0;
    static const WCHAR q1[]=
        {'S','E','L','E','C','T',' ','*',' ', 'F','R','O','M',' ',
         '`','R','e','g','i','s','t','r','y','`',0};
    UINT rc;
    MSIQUERY * view;
    MSIFEATURE *feature;
    MSIFILE *file;

    TRACE("InstallValidate\n");

    rc = MSI_DatabaseOpenViewW(package->db, q1, &view);
    if (rc == ERROR_SUCCESS)
    {
        MSI_IterateRecords( view, &progress, NULL, package );
        msiobj_release( &view->hdr );
        total += progress * REG_PROGRESS_VALUE;
    }

    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
        total += COMPONENT_PROGRESS_VALUE;

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
        total += file->FileSize;

    ui_progress(package,0,total,0,0);

    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        TRACE("Feature: %s; Installed: %i; Action %i; Request %i\n",
            debugstr_w(feature->Feature), feature->Installed, feature->Action,
            feature->ActionRequest);
    }
    
    return ERROR_SUCCESS;
}

static UINT ITERATE_LaunchConditions(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE* package = (MSIPACKAGE*)param;
    LPCWSTR cond = NULL; 
    LPCWSTR message = NULL;
    static const WCHAR title[]=
        {'I','n','s','t','a','l','l',' ','F','a', 'i','l','e','d',0};

    cond = MSI_RecordGetString(row,1);

    if (MSI_EvaluateConditionW(package,cond) != MSICONDITION_TRUE)
    {
        LPWSTR deformated;
        message = MSI_RecordGetString(row,2);
        deformat_string(package,message,&deformated); 
        MessageBoxW(NULL,deformated,title,MB_OK);
        msi_free(deformated);
        return ERROR_FUNCTION_FAILED;
    }

    return ERROR_SUCCESS;
}

static UINT ACTION_LaunchConditions(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view = NULL;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','L','a','u','n','c','h','C','o','n','d','i','t','i','o','n','`',0};

    TRACE("Checking launch conditions\n");

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_LaunchConditions, package);
    msiobj_release(&view->hdr);

    return rc;
}

static LPWSTR resolve_keypath( MSIPACKAGE* package, MSICOMPONENT *cmp )
{

    if (!cmp->KeyPath)
        return resolve_folder(package,cmp->Directory,FALSE,FALSE,NULL);

    if (cmp->Attributes & msidbComponentAttributesRegistryKeyPath)
    {
        MSIRECORD * row = 0;
        UINT root,len;
        LPWSTR deformated,buffer,deformated_name;
        LPCWSTR key,name;
        static const WCHAR ExecSeqQuery[] =
            {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
             '`','R','e','g','i','s','t','r','y','`',' ',
             'W','H','E','R','E',' ', '`','R','e','g','i','s','t','r','y','`',
             ' ','=',' ' ,'\'','%','s','\'',0 };
        static const WCHAR fmt[]={'%','0','2','i',':','\\','%','s','\\',0};
        static const WCHAR fmt2[]=
            {'%','0','2','i',':','\\','%','s','\\','%','s',0};

        row = MSI_QueryGetRecord(package->db, ExecSeqQuery,cmp->KeyPath);
        if (!row)
            return NULL;

        root = MSI_RecordGetInteger(row,2);
        key = MSI_RecordGetString(row, 3);
        name = MSI_RecordGetString(row, 4);
        deformat_string(package, key , &deformated);
        deformat_string(package, name, &deformated_name);

        len = strlenW(deformated) + 6;
        if (deformated_name)
            len+=strlenW(deformated_name);

        buffer = msi_alloc( len *sizeof(WCHAR));

        if (deformated_name)
            sprintfW(buffer,fmt2,root,deformated,deformated_name);
        else
            sprintfW(buffer,fmt,root,deformated);

        msi_free(deformated);
        msi_free(deformated_name);
        msiobj_release(&row->hdr);

        return buffer;
    }
    else if (cmp->Attributes & msidbComponentAttributesODBCDataSource)
    {
        FIXME("UNIMPLEMENTED keypath as ODBC Source\n");
        return NULL;
    }
    else
    {
        MSIFILE *file = get_loaded_file( package, cmp->KeyPath );

        if (file)
            return strdupW( file->TargetPath );
    }
    return NULL;
}

static HKEY openSharedDLLsKey(void)
{
    HKEY hkey=0;
    static const WCHAR path[] =
        {'S','o','f','t','w','a','r','e','\\',
         'M','i','c','r','o','s','o','f','t','\\',
         'W','i','n','d','o','w','s','\\',
         'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
         'S','h','a','r','e','d','D','L','L','s',0};

    RegCreateKeyW(HKEY_LOCAL_MACHINE,path,&hkey);
    return hkey;
}

static UINT ACTION_GetSharedDLLsCount(LPCWSTR dll)
{
    HKEY hkey;
    DWORD count=0;
    DWORD type;
    DWORD sz = sizeof(count);
    DWORD rc;
    
    hkey = openSharedDLLsKey();
    rc = RegQueryValueExW(hkey, dll, NULL, &type, (LPBYTE)&count, &sz);
    if (rc != ERROR_SUCCESS)
        count = 0;
    RegCloseKey(hkey);
    return count;
}

static UINT ACTION_WriteSharedDLLsCount(LPCWSTR path, UINT count)
{
    HKEY hkey;

    hkey = openSharedDLLsKey();
    if (count > 0)
        msi_reg_set_val_dword( hkey, path, count );
    else
        RegDeleteValueW(hkey,path);
    RegCloseKey(hkey);
    return count;
}

/*
 * Return TRUE if the count should be written out and FALSE if not
 */
static void ACTION_RefCountComponent( MSIPACKAGE* package, MSICOMPONENT *comp )
{
    MSIFEATURE *feature;
    INT count = 0;
    BOOL write = FALSE;

    /* only refcount DLLs */
    if (comp->KeyPath == NULL || 
        comp->Attributes & msidbComponentAttributesRegistryKeyPath || 
        comp->Attributes & msidbComponentAttributesODBCDataSource)
        write = FALSE;
    else
    {
        count = ACTION_GetSharedDLLsCount( comp->FullKeypath);
        write = (count > 0);

        if (comp->Attributes & msidbComponentAttributesSharedDllRefCount)
            write = TRUE;
    }

    /* increment counts */
    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        ComponentList *cl;

        if (!ACTION_VerifyFeatureForAction( feature, INSTALLSTATE_LOCAL ))
            continue;

        LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
        {
            if ( cl->component == comp )
                count++;
        }
    }

    /* decrement counts */
    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        ComponentList *cl;

        if (!ACTION_VerifyFeatureForAction( feature, INSTALLSTATE_ABSENT ))
            continue;

        LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
        {
            if ( cl->component == comp )
                count--;
        }
    }

    /* ref count all the files in the component */
    if (write)
    {
        MSIFILE *file;

        LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
        {
            if (file->Component == comp)
                ACTION_WriteSharedDLLsCount( file->TargetPath, count );
        }
    }
    
    /* add a count for permenent */
    if (comp->Attributes & msidbComponentAttributesPermanent)
        count ++;
    
    comp->RefCount = count;

    if (write)
        ACTION_WriteSharedDLLsCount( comp->FullKeypath, comp->RefCount );
}

/*
 * Ok further analysis makes me think that this work is
 * actually done in the PublishComponents and PublishFeatures
 * step, and not here.  It appears like the keypath and all that is
 * resolved in this step, however actually written in the Publish steps.
 * But we will leave it here for now because it is unclear
 */
static UINT ACTION_ProcessComponents(MSIPACKAGE *package)
{
    WCHAR squished_pc[GUID_SIZE];
    WCHAR squished_cc[GUID_SIZE];
    UINT rc;
    MSICOMPONENT *comp;
    HKEY hkey=0,hkey2=0;

    /* writes the Component and Features values to the registry */

    rc = MSIREG_OpenComponents(&hkey);
    if (rc != ERROR_SUCCESS)
        return rc;

    squash_guid(package->ProductCode,squished_pc);
    ui_progress(package,1,COMPONENT_PROGRESS_VALUE,1,0);

    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
    {
        MSIRECORD * uirow;

        ui_progress(package,2,0,0,0);
        if (!comp->ComponentId)
            continue;

        squash_guid(comp->ComponentId,squished_cc);
           
        msi_free(comp->FullKeypath);
        comp->FullKeypath = resolve_keypath( package, comp );

        /* do the refcounting */
        ACTION_RefCountComponent( package, comp );

        TRACE("Component %s (%s), Keypath=%s, RefCount=%i\n",
                            debugstr_w(comp->Component),
                            debugstr_w(squished_cc),
                            debugstr_w(comp->FullKeypath),
                            comp->RefCount);
        /*
         * Write the keypath out if the component is to be registered
         * and delete the key if the component is to be deregistered
         */
        if (ACTION_VerifyComponentForAction( comp, INSTALLSTATE_LOCAL))
        {
            rc = RegCreateKeyW(hkey,squished_cc,&hkey2);
            if (rc != ERROR_SUCCESS)
                continue;

            if (!comp->FullKeypath)
                continue;

            msi_reg_set_val_str( hkey2, squished_pc, comp->FullKeypath );

            if (comp->Attributes & msidbComponentAttributesPermanent)
            {
                static const WCHAR szPermKey[] =
                    { '0','0','0','0','0','0','0','0','0','0','0','0',
                      '0','0','0','0','0','0','0','0','0','0','0','0',
                      '0','0','0','0','0','0','0','0',0 };

                msi_reg_set_val_str( hkey2, szPermKey, comp->FullKeypath );
            }

            RegCloseKey(hkey2);

            /* UI stuff */
            uirow = MSI_CreateRecord(3);
            MSI_RecordSetStringW(uirow,1,package->ProductCode);
            MSI_RecordSetStringW(uirow,2,comp->ComponentId);
            MSI_RecordSetStringW(uirow,3,comp->FullKeypath);
            ui_actiondata(package,szProcessComponents,uirow);
            msiobj_release( &uirow->hdr );
        }
        else if (ACTION_VerifyComponentForAction( comp, INSTALLSTATE_ABSENT))
        {
            DWORD res;

            rc = RegOpenKeyW(hkey,squished_cc,&hkey2);
            if (rc != ERROR_SUCCESS)
                continue;

            RegDeleteValueW(hkey2,squished_pc);

            /* if the key is empty delete it */
            res = RegEnumKeyExW(hkey2,0,NULL,0,0,NULL,0,NULL);
            RegCloseKey(hkey2);
            if (res == ERROR_NO_MORE_ITEMS)
                RegDeleteKeyW(hkey,squished_cc);

            /* UI stuff */
            uirow = MSI_CreateRecord(2);
            MSI_RecordSetStringW(uirow,1,package->ProductCode);
            MSI_RecordSetStringW(uirow,2,comp->ComponentId);
            ui_actiondata(package,szProcessComponents,uirow);
            msiobj_release( &uirow->hdr );
        }
    } 
    RegCloseKey(hkey);
    return rc;
}

typedef struct {
    CLSID       clsid;
    LPWSTR      source;

    LPWSTR      path;
    ITypeLib    *ptLib;
} typelib_struct;

static BOOL CALLBACK Typelib_EnumResNameProc( HMODULE hModule, LPCWSTR lpszType, 
                                       LPWSTR lpszName, LONG_PTR lParam)
{
    TLIBATTR *attr;
    typelib_struct *tl_struct = (typelib_struct*) lParam;
    static const WCHAR fmt[] = {'%','s','\\','%','i',0};
    int sz; 
    HRESULT res;

    if (!IS_INTRESOURCE(lpszName))
    {
        ERR("Not Int Resource Name %s\n",debugstr_w(lpszName));
        return TRUE;
    }

    sz = strlenW(tl_struct->source)+4;
    sz *= sizeof(WCHAR);

    if ((INT_PTR)lpszName == 1)
        tl_struct->path = strdupW(tl_struct->source);
    else
    {
        tl_struct->path = msi_alloc(sz);
        sprintfW(tl_struct->path,fmt,tl_struct->source, lpszName);
    }

    TRACE("trying %s\n", debugstr_w(tl_struct->path));
    res = LoadTypeLib(tl_struct->path,&tl_struct->ptLib);
    if (!SUCCEEDED(res))
    {
        msi_free(tl_struct->path);
        tl_struct->path = NULL;

        return TRUE;
    }

    ITypeLib_GetLibAttr(tl_struct->ptLib, &attr);
    if (IsEqualGUID(&(tl_struct->clsid),&(attr->guid)))
    {
        ITypeLib_ReleaseTLibAttr(tl_struct->ptLib, attr);
        return FALSE;
    }

    msi_free(tl_struct->path);
    tl_struct->path = NULL;

    ITypeLib_ReleaseTLibAttr(tl_struct->ptLib, attr);
    ITypeLib_Release(tl_struct->ptLib);

    return TRUE;
}

static UINT ITERATE_RegisterTypeLibraries(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE* package = (MSIPACKAGE*)param;
    LPCWSTR component;
    MSICOMPONENT *comp;
    MSIFILE *file;
    typelib_struct tl_struct;
    HMODULE module;
    static const WCHAR szTYPELIB[] = {'T','Y','P','E','L','I','B',0};

    component = MSI_RecordGetString(row,3);
    comp = get_loaded_component(package,component);
    if (!comp)
        return ERROR_SUCCESS;

    if (!ACTION_VerifyComponentForAction( comp, INSTALLSTATE_LOCAL))
    {
        TRACE("Skipping typelib reg due to disabled component\n");

        comp->Action = comp->Installed;

        return ERROR_SUCCESS;
    }

    comp->Action = INSTALLSTATE_LOCAL;

    file = get_loaded_file( package, comp->KeyPath ); 
    if (!file)
        return ERROR_SUCCESS;

    module = LoadLibraryExW( file->TargetPath, NULL, LOAD_LIBRARY_AS_DATAFILE );
    if (module)
    {
        LPCWSTR guid;
        guid = MSI_RecordGetString(row,1);
        CLSIDFromString((LPWSTR)guid, &tl_struct.clsid);
        tl_struct.source = strdupW( file->TargetPath );
        tl_struct.path = NULL;

        EnumResourceNamesW(module, szTYPELIB, Typelib_EnumResNameProc,
                        (LONG_PTR)&tl_struct);

        if (tl_struct.path)
        {
            LPWSTR help = NULL;
            LPCWSTR helpid;
            HRESULT res;

            helpid = MSI_RecordGetString(row,6);

            if (helpid)
                help = resolve_folder(package,helpid,FALSE,FALSE,NULL);
            res = RegisterTypeLib(tl_struct.ptLib,tl_struct.path,help);
            msi_free(help);

            if (!SUCCEEDED(res))
                ERR("Failed to register type library %s\n",
                        debugstr_w(tl_struct.path));
            else
            {
                ui_actiondata(package,szRegisterTypeLibraries,row);

                TRACE("Registered %s\n", debugstr_w(tl_struct.path));
            }

            ITypeLib_Release(tl_struct.ptLib);
            msi_free(tl_struct.path);
        }
        else
            ERR("Failed to load type library %s\n",
                    debugstr_w(tl_struct.source));

        FreeLibrary(module);
        msi_free(tl_struct.source);
    }
    else
        ERR("Could not load file! %s\n", debugstr_w(file->TargetPath));

    return ERROR_SUCCESS;
}

static UINT ACTION_RegisterTypeLibraries(MSIPACKAGE *package)
{
    /* 
     * OK this is a bit confusing.. I am given a _Component key and I believe
     * that the file that is being registered as a type library is the "key file
     * of that component" which I interpret to mean "The file in the KeyPath of
     * that component".
     */
    UINT rc;
    MSIQUERY * view;
    static const WCHAR Query[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','T','y','p','e','L','i','b','`',0};

    rc = MSI_DatabaseOpenViewW(package->db, Query, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_RegisterTypeLibraries, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT ITERATE_CreateShortcuts(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    LPWSTR target_file, target_folder, filename;
    LPCWSTR buffer, extension;
    MSICOMPONENT *comp;
    static const WCHAR szlnk[]={'.','l','n','k',0};
    IShellLinkW *sl = NULL;
    IPersistFile *pf = NULL;
    HRESULT res;

    buffer = MSI_RecordGetString(row,4);
    comp = get_loaded_component(package,buffer);
    if (!comp)
        return ERROR_SUCCESS;

    if (!ACTION_VerifyComponentForAction( comp, INSTALLSTATE_LOCAL ))
    {
        TRACE("Skipping shortcut creation due to disabled component\n");

        comp->Action = comp->Installed;

        return ERROR_SUCCESS;
    }

    comp->Action = INSTALLSTATE_LOCAL;

    ui_actiondata(package,szCreateShortcuts,row);

    res = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IShellLinkW, (LPVOID *) &sl );

    if (FAILED( res ))
    {
        ERR("CLSID_ShellLink not available\n");
        goto err;
    }

    res = IShellLinkW_QueryInterface( sl, &IID_IPersistFile,(LPVOID*) &pf );
    if (FAILED( res ))
    {
        ERR("QueryInterface(IID_IPersistFile) failed\n");
        goto err;
    }

    buffer = MSI_RecordGetString(row,2);
    target_folder = resolve_folder(package, buffer,FALSE,FALSE,NULL);

    /* may be needed because of a bug somehwere else */
    create_full_pathW(target_folder);

    filename = msi_dup_record_field( row, 3 );
    reduce_to_longfilename(filename);

    extension = strchrW(filename,'.');
    if (!extension || strcmpiW(extension,szlnk))
    {
        int len = strlenW(filename);
        filename = msi_realloc(filename, len * sizeof(WCHAR) + sizeof(szlnk));
        memcpy(filename + len, szlnk, sizeof(szlnk));
    }
    target_file = build_directory_name(2, target_folder, filename);
    msi_free(target_folder);
    msi_free(filename);

    buffer = MSI_RecordGetString(row,5);
    if (strchrW(buffer,'['))
    {
        LPWSTR deformated;
        deformat_string(package,buffer,&deformated);
        IShellLinkW_SetPath(sl,deformated);
        msi_free(deformated);
    }
    else
    {
        FIXME("poorly handled shortcut format, advertised shortcut\n");
        IShellLinkW_SetPath(sl,comp->FullKeypath);
    }

    if (!MSI_RecordIsNull(row,6))
    {
        LPWSTR deformated;
        buffer = MSI_RecordGetString(row,6);
        deformat_string(package,buffer,&deformated);
        IShellLinkW_SetArguments(sl,deformated);
        msi_free(deformated);
    }

    if (!MSI_RecordIsNull(row,7))
    {
        buffer = MSI_RecordGetString(row,7);
        IShellLinkW_SetDescription(sl,buffer);
    }

    if (!MSI_RecordIsNull(row,8))
        IShellLinkW_SetHotkey(sl,MSI_RecordGetInteger(row,8));

    if (!MSI_RecordIsNull(row,9))
    {
        LPWSTR Path;
        INT index; 

        buffer = MSI_RecordGetString(row,9);

        Path = build_icon_path(package,buffer);
        index = MSI_RecordGetInteger(row,10);

        /* no value means 0 */
        if (index == MSI_NULL_INTEGER)
            index = 0;

        IShellLinkW_SetIconLocation(sl,Path,index);
        msi_free(Path);
    }

    if (!MSI_RecordIsNull(row,11))
        IShellLinkW_SetShowCmd(sl,MSI_RecordGetInteger(row,11));

    if (!MSI_RecordIsNull(row,12))
    {
        LPWSTR Path;
        buffer = MSI_RecordGetString(row,12);
        Path = resolve_folder(package, buffer, FALSE, FALSE, NULL);
        if (Path)
            IShellLinkW_SetWorkingDirectory(sl,Path);
        msi_free(Path);
    }

    TRACE("Writing shortcut to %s\n",debugstr_w(target_file));
    IPersistFile_Save(pf,target_file,FALSE);

    msi_free(target_file);    

err:
    if (pf)
        IPersistFile_Release( pf );
    if (sl)
        IShellLinkW_Release( sl );

    return ERROR_SUCCESS;
}

static UINT ACTION_CreateShortcuts(MSIPACKAGE *package)
{
    UINT rc;
    HRESULT res;
    MSIQUERY * view;
    static const WCHAR Query[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','S','h','o','r','t','c','u','t','`',0};

    rc = MSI_DatabaseOpenViewW(package->db, Query, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    res = CoInitialize( NULL );
    if (FAILED (res))
    {
        ERR("CoInitialize failed\n");
        return ERROR_FUNCTION_FAILED;
    }

    rc = MSI_IterateRecords(view, NULL, ITERATE_CreateShortcuts, package);
    msiobj_release(&view->hdr);

    CoUninitialize();

    return rc;
}

static UINT ITERATE_PublishProduct(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE* package = (MSIPACKAGE*)param;
    HANDLE the_file;
    LPWSTR FilePath;
    LPCWSTR FileName;
    CHAR buffer[1024];
    DWORD sz;
    UINT rc;
    MSIRECORD *uirow;

    FileName = MSI_RecordGetString(row,1);
    if (!FileName)
    {
        ERR("Unable to get FileName\n");
        return ERROR_SUCCESS;
    }

    FilePath = build_icon_path(package,FileName);

    TRACE("Creating icon file at %s\n",debugstr_w(FilePath));

    the_file = CreateFileW(FilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);

    if (the_file == INVALID_HANDLE_VALUE)
    {
        ERR("Unable to create file %s\n",debugstr_w(FilePath));
        msi_free(FilePath);
        return ERROR_SUCCESS;
    }

    do 
    {
        DWORD write;
        sz = 1024;
        rc = MSI_RecordReadStream(row,2,buffer,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Failed to get stream\n");
            CloseHandle(the_file);  
            DeleteFileW(FilePath);
            break;
        }
        WriteFile(the_file,buffer,sz,&write,NULL);
    } while (sz == 1024);

    msi_free(FilePath);

    CloseHandle(the_file);

    uirow = MSI_CreateRecord(1);
    MSI_RecordSetStringW(uirow,1,FileName);
    ui_actiondata(package,szPublishProduct,uirow);
    msiobj_release( &uirow->hdr );

    return ERROR_SUCCESS;
}

/*
 * 99% of the work done here is only done for 
 * advertised installs. However this is where the
 * Icon table is processed and written out
 * so that is what I am going to do here.
 */
static UINT ACTION_PublishProduct(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    static const WCHAR Query[]=
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','I','c','o','n','`',0};
    /* for registry stuff */
    HKEY hkey=0;
    HKEY hukey=0;
    static const WCHAR szProductLanguage[] =
        {'P','r','o','d','u','c','t','L','a','n','g','u','a','g','e',0};
    static const WCHAR szARPProductIcon[] =
        {'A','R','P','P','R','O','D','U','C','T','I','C','O','N',0};
    static const WCHAR szProductVersion[] =
        {'P','r','o','d','u','c','t','V','e','r','s','i','o','n',0};
    DWORD langid;
    LPWSTR buffer;
    DWORD size;
    MSIHANDLE hDb, hSumInfo;

    /* write out icon files */

    rc = MSI_DatabaseOpenViewW(package->db, Query, &view);
    if (rc == ERROR_SUCCESS)
    {
        MSI_IterateRecords(view, NULL, ITERATE_PublishProduct, package);
        msiobj_release(&view->hdr);
    }

    /* ok there is a lot more done here but i need to figure out what */

    rc = MSIREG_OpenProductsKey(package->ProductCode,&hkey,TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;

    rc = MSIREG_OpenUserProductsKey(package->ProductCode,&hukey,TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;


    buffer = msi_dup_property( package, INSTALLPROPERTY_PRODUCTNAMEW );
    msi_reg_set_val_str( hukey, INSTALLPROPERTY_PRODUCTNAMEW, buffer );
    msi_free(buffer);

    langid = msi_get_property_int( package, szProductLanguage, 0 );
    msi_reg_set_val_dword( hkey, INSTALLPROPERTY_LANGUAGEW, langid );

    buffer = msi_dup_property( package, szARPProductIcon );
    if (buffer)
    {
        LPWSTR path = build_icon_path(package,buffer);
        msi_reg_set_val_str( hukey, INSTALLPROPERTY_PRODUCTICONW, path );
        msi_free( path );
    }
    msi_free(buffer);

    buffer = msi_dup_property( package, szProductVersion );
    if (buffer)
    {
        DWORD verdword = msi_version_str_to_dword(buffer);
        msi_reg_set_val_dword( hkey, INSTALLPROPERTY_VERSIONW, verdword );
    }
    msi_free(buffer);
    
    /* FIXME: Need to write more keys to the user registry */
  
    hDb= alloc_msihandle( &package->db->hdr );
    if (!hDb) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto end;
    }
    rc = MsiGetSummaryInformationW(hDb, NULL, 0, &hSumInfo); 
    MsiCloseHandle(hDb);
    if (rc == ERROR_SUCCESS)
    {
        WCHAR guidbuffer[0x200];
        size = 0x200;
        rc = MsiSummaryInfoGetPropertyW(hSumInfo, 9, NULL, NULL, NULL,
                                        guidbuffer, &size);
        if (rc == ERROR_SUCCESS)
        {
            WCHAR squashed[GUID_SIZE];
            /* for now we only care about the first guid */
            LPWSTR ptr = strchrW(guidbuffer,';');
            if (ptr) *ptr = 0;
            squash_guid(guidbuffer,squashed);
            msi_reg_set_val_str( hukey, INSTALLPROPERTY_PACKAGECODEW, squashed );
        }
        else
        {
            ERR("Unable to query Revision_Number...\n");
            rc = ERROR_SUCCESS;
        }
        MsiCloseHandle(hSumInfo);
    }
    else
    {
        ERR("Unable to open Summary Information\n");
        rc = ERROR_SUCCESS;
    }

end:

    RegCloseKey(hkey);
    RegCloseKey(hukey);

    return rc;
}

static UINT ITERATE_WriteIniValues(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    LPCWSTR component,section,key,value,identifier,filename,dirproperty;
    LPWSTR deformated_section, deformated_key, deformated_value;
    LPWSTR folder, fullname = NULL;
    MSIRECORD * uirow;
    INT action;
    MSICOMPONENT *comp;
    static const WCHAR szWindowsFolder[] =
          {'W','i','n','d','o','w','s','F','o','l','d','e','r',0};

    component = MSI_RecordGetString(row, 8);
    comp = get_loaded_component(package,component);

    if (!ACTION_VerifyComponentForAction( comp, INSTALLSTATE_LOCAL))
    {
        TRACE("Skipping ini file due to disabled component %s\n",
                        debugstr_w(component));

        comp->Action = comp->Installed;

        return ERROR_SUCCESS;
    }

    comp->Action = INSTALLSTATE_LOCAL;

    identifier = MSI_RecordGetString(row,1); 
    filename = MSI_RecordGetString(row,2);
    dirproperty = MSI_RecordGetString(row,3);
    section = MSI_RecordGetString(row,4);
    key = MSI_RecordGetString(row,5);
    value = MSI_RecordGetString(row,6);
    action = MSI_RecordGetInteger(row,7);

    deformat_string(package,section,&deformated_section);
    deformat_string(package,key,&deformated_key);
    deformat_string(package,value,&deformated_value);

    if (dirproperty)
    {
        folder = resolve_folder(package, dirproperty, FALSE, FALSE, NULL);
        if (!folder)
            folder = msi_dup_property( package, dirproperty );
    }
    else
        folder = msi_dup_property( package, szWindowsFolder );

    if (!folder)
    {
        ERR("Unable to resolve folder! (%s)\n",debugstr_w(dirproperty));
        goto cleanup;
    }

    fullname = build_directory_name(2, folder, filename);

    if (action == 0)
    {
        TRACE("Adding value %s to section %s in %s\n",
                debugstr_w(deformated_key), debugstr_w(deformated_section),
                debugstr_w(fullname));
        WritePrivateProfileStringW(deformated_section, deformated_key,
                                   deformated_value, fullname);
    }
    else if (action == 1)
    {
        WCHAR returned[10];
        GetPrivateProfileStringW(deformated_section, deformated_key, NULL,
                                 returned, 10, fullname);
        if (returned[0] == 0)
        {
            TRACE("Adding value %s to section %s in %s\n",
                    debugstr_w(deformated_key), debugstr_w(deformated_section),
                    debugstr_w(fullname));

            WritePrivateProfileStringW(deformated_section, deformated_key,
                                       deformated_value, fullname);
        }
    }
    else if (action == 3)
        FIXME("Append to existing section not yet implemented\n");

    uirow = MSI_CreateRecord(4);
    MSI_RecordSetStringW(uirow,1,identifier);
    MSI_RecordSetStringW(uirow,2,deformated_section);
    MSI_RecordSetStringW(uirow,3,deformated_key);
    MSI_RecordSetStringW(uirow,4,deformated_value);
    ui_actiondata(package,szWriteIniValues,uirow);
    msiobj_release( &uirow->hdr );
cleanup:
    msi_free(fullname);
    msi_free(folder);
    msi_free(deformated_key);
    msi_free(deformated_value);
    msi_free(deformated_section);
    return ERROR_SUCCESS;
}

static UINT ACTION_WriteIniValues(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    static const WCHAR ExecSeqQuery[] = 
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','I','n','i','F','i','l','e','`',0};

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
    {
        TRACE("no IniFile table\n");
        return ERROR_SUCCESS;
    }

    rc = MSI_IterateRecords(view, NULL, ITERATE_WriteIniValues, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT ITERATE_SelfRegModules(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    LPCWSTR filename;
    LPWSTR FullName;
    MSIFILE *file;
    DWORD len;
    static const WCHAR ExeStr[] =
        {'r','e','g','s','v','r','3','2','.','e','x','e',' ','\"',0};
    static const WCHAR close[] =  {'\"',0};
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL brc;
    MSIRECORD *uirow;
    LPWSTR uipath, p;

    memset(&si,0,sizeof(STARTUPINFOW));

    filename = MSI_RecordGetString(row,1);
    file = get_loaded_file( package, filename );

    if (!file)
    {
        ERR("Unable to find file id %s\n",debugstr_w(filename));
        return ERROR_SUCCESS;
    }

    len = strlenW(ExeStr) + strlenW( file->TargetPath ) + 2;

    FullName = msi_alloc(len*sizeof(WCHAR));
    strcpyW(FullName,ExeStr);
    strcatW( FullName, file->TargetPath );
    strcatW(FullName,close);

    TRACE("Registering %s\n",debugstr_w(FullName));
    brc = CreateProcessW(NULL, FullName, NULL, NULL, FALSE, 0, NULL, c_colon,
                    &si, &info);

    if (brc)
        msi_dialog_check_messages(info.hProcess);

    msi_free(FullName);

    /* the UI chunk */
    uirow = MSI_CreateRecord( 2 );
    uipath = strdupW( file->TargetPath );
    p = strrchrW(uipath,'\\');
    if (p)
        p[1]=0;
    MSI_RecordSetStringW( uirow, 1, &p[2] );
    MSI_RecordSetStringW( uirow, 2, uipath);
    ui_actiondata( package, szSelfRegModules, uirow);
    msiobj_release( &uirow->hdr );
    msi_free( uipath );
    /* FIXME: call ui_progress? */

    return ERROR_SUCCESS;
}

static UINT ACTION_SelfRegModules(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    static const WCHAR ExecSeqQuery[] = 
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','S','e','l','f','R','e','g','`',0};

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
    {
        TRACE("no SelfReg table\n");
        return ERROR_SUCCESS;
    }

    MSI_IterateRecords(view, NULL, ITERATE_SelfRegModules, package);
    msiobj_release(&view->hdr);

    return ERROR_SUCCESS;
}

static UINT ACTION_PublishFeatures(MSIPACKAGE *package)
{
    MSIFEATURE *feature;
    UINT rc;
    HKEY hkey=0;
    HKEY hukey=0;
    
    rc = MSIREG_OpenFeaturesKey(package->ProductCode,&hkey,TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;

    rc = MSIREG_OpenUserFeaturesKey(package->ProductCode,&hukey,TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;

    /* here the guids are base 85 encoded */
    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        ComponentList *cl;
        LPWSTR data = NULL;
        GUID clsid;
        INT size;
        BOOL absent = FALSE;
        MSIRECORD *uirow;

        if (!ACTION_VerifyFeatureForAction( feature, INSTALLSTATE_LOCAL ) &&
            !ACTION_VerifyFeatureForAction( feature, INSTALLSTATE_SOURCE ) &&
            !ACTION_VerifyFeatureForAction( feature, INSTALLSTATE_ADVERTISED ))
            absent = TRUE;

        size = 1;
        LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
        {
            size += 21;
        }
        if (feature->Feature_Parent)
            size += strlenW( feature->Feature_Parent )+2;

        data = msi_alloc(size * sizeof(WCHAR));

        data[0] = 0;
        LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
        {
            MSICOMPONENT* component = cl->component;
            WCHAR buf[21];

            buf[0] = 0;
            if (component->ComponentId)
            {
                TRACE("From %s\n",debugstr_w(component->ComponentId));
                CLSIDFromString(component->ComponentId, &clsid);
                encode_base85_guid(&clsid,buf);
                TRACE("to %s\n",debugstr_w(buf));
                strcatW(data,buf);
            }
        }
        if (feature->Feature_Parent)
        {
            static const WCHAR sep[] = {'\2',0};
            strcatW(data,sep);
            strcatW(data,feature->Feature_Parent);
        }

        msi_reg_set_val_str( hkey, feature->Feature, data );
        msi_free(data);

        size = 0;
        if (feature->Feature_Parent)
            size = strlenW(feature->Feature_Parent)*sizeof(WCHAR);
        if (!absent)
        {
            RegSetValueExW(hukey,feature->Feature,0,REG_SZ,
                       (LPBYTE)feature->Feature_Parent,size);
        }
        else
        {
            size += 2*sizeof(WCHAR);
            data = msi_alloc(size);
            data[0] = 0x6;
            data[1] = 0;
            if (feature->Feature_Parent)
                strcpyW( &data[1], feature->Feature_Parent );
            RegSetValueExW(hukey,feature->Feature,0,REG_SZ,
                       (LPBYTE)data,size);
            msi_free(data);
        }

        /* the UI chunk */
        uirow = MSI_CreateRecord( 1 );
        MSI_RecordSetStringW( uirow, 1, feature->Feature );
        ui_actiondata( package, szPublishFeatures, uirow);
        msiobj_release( &uirow->hdr );
        /* FIXME: call ui_progress? */
    }

end:
    RegCloseKey(hkey);
    RegCloseKey(hukey);
    return rc;
}

static UINT msi_get_local_package_name( LPWSTR path )
{
    static const WCHAR szInstaller[] = {
        '\\','I','n','s','t','a','l','l','e','r','\\',0};
    static const WCHAR fmt[] = { '%','x','.','m','s','i',0};
    DWORD time, len, i;
    HANDLE handle;

    time = GetTickCount();
    GetWindowsDirectoryW( path, MAX_PATH );
    lstrcatW( path, szInstaller );
    CreateDirectoryW( path, NULL );

    len = lstrlenW(path);
    for (i=0; i<0x10000; i++)
    {
        snprintfW( &path[len], MAX_PATH - len, fmt, (time+i)&0xffff );
        handle = CreateFileW( path, GENERIC_WRITE, 0, NULL,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0 );
        if (handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle);
            break;
        }
        if (GetLastError() != ERROR_FILE_EXISTS &&
            GetLastError() != ERROR_SHARING_VIOLATION)
            return ERROR_FUNCTION_FAILED;
    }

    return ERROR_SUCCESS;
}

static UINT msi_make_package_local( MSIPACKAGE *package, HKEY hkey )
{
    static const WCHAR szOriginalDatabase[] =
        {'O','r','i','g','i','n','a','l','D','a','t','a','b','a','s','e',0};
    WCHAR packagefile[MAX_PATH];
    LPWSTR msiFilePath;
    UINT r;

    r = msi_get_local_package_name( packagefile );
    if (r != ERROR_SUCCESS)
        return r;

    TRACE("Copying to local package %s\n",debugstr_w(packagefile));

    msiFilePath = msi_dup_property( package, szOriginalDatabase );
    r = CopyFileW( msiFilePath, packagefile, FALSE);
    msi_free( msiFilePath );

    if (!r)
    {
        ERR("Unable to copy package (%s -> %s) (error %d)\n",
            debugstr_w(msiFilePath), debugstr_w(packagefile), GetLastError());
        return ERROR_FUNCTION_FAILED;
    }

    /* FIXME: maybe set this key in ACTION_RegisterProduct instead */
    msi_reg_set_val_str( hkey, INSTALLPROPERTY_LOCALPACKAGEW, packagefile );
    return ERROR_SUCCESS;
}

static UINT msi_write_uninstall_property_vals( MSIPACKAGE *package, HKEY hkey )
{
    LPWSTR prop, val, key;
    static const LPCSTR propval[] = {
        "ARPAUTHORIZEDCDFPREFIX", "AuthorizedCDFPrefix",
        "ARPCONTACT",             "Contact",
        "ARPCOMMENTS",            "Comments",
        "ProductName",            "DisplayName",
        "ProductVersion",         "DisplayVersion",
        "ARPHELPLINK",            "HelpLink",
        "ARPHELPTELEPHONE",       "HelpTelephone",
        "ARPINSTALLLOCATION",     "InstallLocation",
        "SourceDir",              "InstallSource",
        "Manufacturer",           "Publisher",
        "ARPREADME",              "Readme",
        "ARPSIZE",                "Size",
        "ARPURLINFOABOUT",        "URLInfoAbout",
        "ARPURLUPDATEINFO",       "URLUpdateInfo",
        NULL,
    };
    const LPCSTR *p = propval;

    while( *p )
    {
        prop = strdupAtoW( *p++ );
        key = strdupAtoW( *p++ );
        val = msi_dup_property( package, prop );
        msi_reg_set_val_str( hkey, key, val );
        msi_free(val);
        msi_free(key);
        msi_free(prop);
    }
    return ERROR_SUCCESS;
}

static UINT ACTION_RegisterProduct(MSIPACKAGE *package)
{
    HKEY hkey=0;
    LPWSTR buffer = NULL;
    UINT rc;
    DWORD size, langid;
    static const WCHAR szWindowsInstaller[] = 
        {'W','i','n','d','o','w','s','I','n','s','t','a','l','l','e','r',0};
    static const WCHAR szUpgradeCode[] = 
        {'U','p','g','r','a','d','e','C','o','d','e',0};
    static const WCHAR modpath_fmt[] = 
        {'M','s','i','E','x','e','c','.','e','x','e',' ',
         '/','I','[','P','r','o','d','u','c','t','C','o','d','e',']',0};
    static const WCHAR szModifyPath[] = 
        {'M','o','d','i','f','y','P','a','t','h',0};
    static const WCHAR szUninstallString[] = 
        {'U','n','i','n','s','t','a','l','l','S','t','r','i','n','g',0};
    static const WCHAR szEstimatedSize[] = 
        {'E','s','t','i','m','a','t','e','d','S','i','z','e',0};
    static const WCHAR szProductLanguage[] =
        {'P','r','o','d','u','c','t','L','a','n','g','u','a','g','e',0};
    static const WCHAR szProductVersion[] =
        {'P','r','o','d','u','c','t','V','e','r','s','i','o','n',0};

    SYSTEMTIME systime;
    static const WCHAR date_fmt[] = {'%','i','%','i','%','i',0};
    LPWSTR upgrade_code;
    WCHAR szDate[9]; 

    rc = MSIREG_OpenUninstallKey(package->ProductCode,&hkey,TRUE);
    if (rc != ERROR_SUCCESS)
        return rc;

    /* dump all the info i can grab */
    /* FIXME: Flesh out more information */

    msi_write_uninstall_property_vals( package, hkey );

    msi_reg_set_val_dword( hkey, szWindowsInstaller, 1 );
    
    msi_make_package_local( package, hkey );

    /* do ModifyPath and UninstallString */
    size = deformat_string(package,modpath_fmt,&buffer);
    RegSetValueExW(hkey,szModifyPath,0,REG_EXPAND_SZ,(LPBYTE)buffer,size);
    RegSetValueExW(hkey,szUninstallString,0,REG_EXPAND_SZ,(LPBYTE)buffer,size);
    msi_free(buffer);

    /* FIXME: Write real Estimated Size when we have it */
    msi_reg_set_val_dword( hkey, szEstimatedSize, 0 );
   
    GetLocalTime(&systime);
    sprintfW(szDate,date_fmt,systime.wYear,systime.wMonth,systime.wDay);
    msi_reg_set_val_str( hkey, INSTALLPROPERTY_INSTALLDATEW, szDate );
   
    langid = msi_get_property_int( package, szProductLanguage, 0 );
    msi_reg_set_val_dword( hkey, INSTALLPROPERTY_LANGUAGEW, langid );

    buffer = msi_dup_property( package, szProductVersion );
    if (buffer)
    {
        DWORD verdword = msi_version_str_to_dword(buffer);

        msi_reg_set_val_dword( hkey, INSTALLPROPERTY_VERSIONW, verdword );
        msi_reg_set_val_dword( hkey, INSTALLPROPERTY_VERSIONMAJORW, verdword>>24 );
        msi_reg_set_val_dword( hkey, INSTALLPROPERTY_VERSIONMINORW, (verdword>>16)&0x00FF );
    }
    msi_free(buffer);
    
    /* Handle Upgrade Codes */
    upgrade_code = msi_dup_property( package, szUpgradeCode );
    if (upgrade_code)
    {
        HKEY hkey2;
        WCHAR squashed[33];
        MSIREG_OpenUpgradeCodesKey(upgrade_code, &hkey2, TRUE);
        squash_guid(package->ProductCode,squashed);
        msi_reg_set_val_str( hkey2, squashed, NULL );
        RegCloseKey(hkey2);
        MSIREG_OpenUserUpgradeCodesKey(upgrade_code, &hkey2, TRUE);
        squash_guid(package->ProductCode,squashed);
        msi_reg_set_val_str( hkey2, squashed, NULL );
        RegCloseKey(hkey2);

        msi_free(upgrade_code);
    }
    
    RegCloseKey(hkey);

    /* FIXME: call ui_actiondata */

    return ERROR_SUCCESS;
}

static UINT ACTION_InstallExecute(MSIPACKAGE *package)
{
    return execute_script(package,INSTALL_SCRIPT);
}

static UINT ACTION_InstallFinalize(MSIPACKAGE *package)
{
    UINT rc;

    /* turn off scheduleing */
    package->script->CurrentlyScripting= FALSE;

    /* first do the same as an InstallExecute */
    rc = ACTION_InstallExecute(package);
    if (rc != ERROR_SUCCESS)
        return rc;

    /* then handle Commit Actions */
    rc = execute_script(package,COMMIT_SCRIPT);

    return rc;
}

static UINT ACTION_ForceReboot(MSIPACKAGE *package)
{
    static const WCHAR RunOnce[] = {
    'S','o','f','t','w','a','r','e','\\',
    'M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\',
    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'R','u','n','O','n','c','e',0};
    static const WCHAR InstallRunOnce[] = {
    'S','o','f','t','w','a','r','e','\\',
    'M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\',
    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','s','t','a','l','l','e','r','\\',
    'R','u','n','O','n','c','e','E','n','t','r','i','e','s',0};

    static const WCHAR msiexec_fmt[] = {
    '%','s',
    '\\','M','s','i','E','x','e','c','.','e','x','e',' ','/','@',' ',
    '\"','%','s','\"',0};
    static const WCHAR install_fmt[] = {
    '/','I',' ','\"','%','s','\"',' ',
    'A','F','T','E','R','R','E','B','O','O','T','=','1',' ',
    'R','U','N','O','N','C','E','E','N','T','R','Y','=','\"','%','s','\"',0};
    WCHAR buffer[256], sysdir[MAX_PATH];
    HKEY hkey;
    WCHAR squished_pc[100];

    squash_guid(package->ProductCode,squished_pc);

    GetSystemDirectoryW(sysdir, sizeof(sysdir)/sizeof(sysdir[0]));
    RegCreateKeyW(HKEY_LOCAL_MACHINE,RunOnce,&hkey);
    snprintfW(buffer,sizeof(buffer)/sizeof(buffer[0]),msiexec_fmt,sysdir,
     squished_pc);

    msi_reg_set_val_str( hkey, squished_pc, buffer );
    RegCloseKey(hkey);

    TRACE("Reboot command %s\n",debugstr_w(buffer));

    RegCreateKeyW(HKEY_LOCAL_MACHINE,InstallRunOnce,&hkey);
    sprintfW(buffer,install_fmt,package->ProductCode,squished_pc);

    msi_reg_set_val_str( hkey, squished_pc, buffer );
    RegCloseKey(hkey);

    return ERROR_INSTALL_SUSPEND;
}

static UINT ACTION_ResolveSource(MSIPACKAGE* package)
{
    DWORD attrib, len;
    LPWSTR ptr, source;
    UINT rc;
    
    /*
     * we are currently doing what should be done here in the top level Install
     * however for Adminastrative and uninstalls this step will be needed
     */
    if (!package->PackagePath)
        return ERROR_SUCCESS;

    ptr = strrchrW(package->PackagePath, '\\');
    if (!ptr)
        return ERROR_SUCCESS;

    len = ptr - package->PackagePath + 2;
    source = msi_alloc(len * sizeof(WCHAR));
    lstrcpynW(source,  package->PackagePath, len);

    MSI_SetPropertyW(package, cszSourceDir, source);
    MSI_SetPropertyW(package, cszSOURCEDIR, source);

    msi_free(source);

    attrib = GetFileAttributesW(package->PackagePath);
    if (attrib == INVALID_FILE_ATTRIBUTES)
    {
        LPWSTR prompt;
        LPWSTR msg;
        DWORD size = 0;

        rc = MsiSourceListGetInfoW(package->ProductCode, NULL, 
                MSIINSTALLCONTEXT_USERMANAGED, MSICODE_PRODUCT,
                INSTALLPROPERTY_DISKPROMPTW,NULL,&size);
        if (rc == ERROR_MORE_DATA)
        {
            prompt = msi_alloc(size * sizeof(WCHAR));
            MsiSourceListGetInfoW(package->ProductCode, NULL, 
                    MSIINSTALLCONTEXT_USERMANAGED, MSICODE_PRODUCT,
                    INSTALLPROPERTY_DISKPROMPTW,prompt,&size);
        }
        else
            prompt = strdupW(package->PackagePath);

        msg = generate_error_string(package,1302,1,prompt);
        while(attrib == INVALID_FILE_ATTRIBUTES)
        {
            rc = MessageBoxW(NULL,msg,NULL,MB_OKCANCEL);
            if (rc == IDCANCEL)
            {
                rc = ERROR_INSTALL_USEREXIT;
                break;
            }
            attrib = GetFileAttributesW(package->PackagePath);
        }
        msi_free(prompt);
        rc = ERROR_SUCCESS;
    }
    else
        return ERROR_SUCCESS;

    return rc;
}

static UINT ACTION_RegisterUser(MSIPACKAGE *package)
{
    HKEY hkey=0;
    LPWSTR buffer;
    LPWSTR productid;
    UINT rc,i;

    static const WCHAR szPropKeys[][80] = 
    {
        {'P','r','o','d','u','c','t','I','D',0},
        {'U','S','E','R','N','A','M','E',0},
        {'C','O','M','P','A','N','Y','N','A','M','E',0},
        {0},
    };

    static const WCHAR szRegKeys[][80] = 
    {
        {'P','r','o','d','u','c','t','I','D',0},
        {'R','e','g','O','w','n','e','r',0},
        {'R','e','g','C','o','m','p','a','n','y',0},
        {0},
    };

    productid = msi_dup_property( package, INSTALLPROPERTY_PRODUCTIDW );
    if (!productid)
        return ERROR_SUCCESS;

    rc = MSIREG_OpenUninstallKey(package->ProductCode,&hkey,TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;

    for( i = 0; szPropKeys[i][0]; i++ )
    {
        buffer = msi_dup_property( package, szPropKeys[i] );
        msi_reg_set_val_str( hkey, szRegKeys[i], buffer );
        msi_free( buffer );
    }

end:
    msi_free(productid);
    RegCloseKey(hkey);

    /* FIXME: call ui_actiondata */

    return ERROR_SUCCESS;
}


static UINT ACTION_ExecuteAction(MSIPACKAGE *package)
{
    UINT rc;

    package->script->InWhatSequence |= SEQUENCE_EXEC;
    rc = ACTION_ProcessExecSequence(package,FALSE);
    return rc;
}


static UINT ITERATE_PublishComponent(MSIRECORD *rec, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    LPCWSTR compgroupid=NULL;
    LPCWSTR feature=NULL;
    LPCWSTR text = NULL;
    LPCWSTR qualifier = NULL;
    LPCWSTR component = NULL;
    LPWSTR advertise = NULL;
    LPWSTR output = NULL;
    HKEY hkey;
    UINT rc = ERROR_SUCCESS;
    MSICOMPONENT *comp;
    DWORD sz = 0;
    MSIRECORD *uirow;

    component = MSI_RecordGetString(rec,3);
    comp = get_loaded_component(package,component);

    if (!ACTION_VerifyComponentForAction( comp, INSTALLSTATE_LOCAL ) && 
       !ACTION_VerifyComponentForAction( comp, INSTALLSTATE_SOURCE ) &&
       !ACTION_VerifyComponentForAction( comp, INSTALLSTATE_ADVERTISED ))
    {
        TRACE("Skipping: Component %s not scheduled for install\n",
                        debugstr_w(component));

        return ERROR_SUCCESS;
    }

    compgroupid = MSI_RecordGetString(rec,1);
    qualifier = MSI_RecordGetString(rec,2);

    rc = MSIREG_OpenUserComponentsKey(compgroupid, &hkey, TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;
    
    text = MSI_RecordGetString(rec,4);
    feature = MSI_RecordGetString(rec,5);
  
    advertise = create_component_advertise_string(package, comp, feature);

    sz = strlenW(advertise);

    if (text)
        sz += lstrlenW(text);

    sz+=3;
    sz *= sizeof(WCHAR);
           
    output = msi_alloc_zero(sz);
    strcpyW(output,advertise);
    msi_free(advertise);

    if (text)
        strcatW(output,text);

    msi_reg_set_val_multi_str( hkey, qualifier, output );
    
end:
    RegCloseKey(hkey);
    msi_free(output);

    /* the UI chunk */
    uirow = MSI_CreateRecord( 2 );
    MSI_RecordSetStringW( uirow, 1, compgroupid );
    MSI_RecordSetStringW( uirow, 2, qualifier);
    ui_actiondata( package, szPublishComponents, uirow);
    msiobj_release( &uirow->hdr );
    /* FIXME: call ui_progress? */

    return rc;
}

/*
 * At present I am ignorning the advertised components part of this and only
 * focusing on the qualified component sets
 */
static UINT ACTION_PublishComponents(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','P','u','b','l','i','s','h',
         'C','o','m','p','o','n','e','n','t','`',0};
    
    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_PublishComponent, package);
    msiobj_release(&view->hdr);

    return rc;
}

static UINT ITERATE_InstallService(MSIRECORD *rec, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    MSIRECORD *row;
    MSIFILE *file;
    SC_HANDLE hscm, service = NULL;
    LPCWSTR name, disp, comp, depends, pass;
    LPCWSTR load_order, serv_name, key;
    DWORD serv_type, start_type;
    DWORD err_control;

    static const WCHAR query[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R', 'O','M',' ',
         '`','C','o','m','p','o','n','e','n','t','`',' ',
         'W','H','E','R','E',' ',
         '`','C','o','m','p','o','n','e','n','t','`',' ',
         '=','\'','%','s','\'',0};

    hscm = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASEW, GENERIC_WRITE);
    if (!hscm)
    {
        ERR("Failed to open the SC Manager!\n");
        goto done;
    }

    start_type = MSI_RecordGetInteger(rec, 5);
    if (start_type == SERVICE_BOOT_START || start_type == SERVICE_SYSTEM_START)
        goto done;

    depends = MSI_RecordGetString(rec, 8);
    if (depends && *depends)
        FIXME("Dependency list unhandled!\n");

    name = MSI_RecordGetString(rec, 2);
    disp = MSI_RecordGetString(rec, 3);
    serv_type = MSI_RecordGetInteger(rec, 4);
    err_control = MSI_RecordGetInteger(rec, 6);
    load_order = MSI_RecordGetString(rec, 7);
    serv_name = MSI_RecordGetString(rec, 9);
    pass = MSI_RecordGetString(rec, 10);
    comp = MSI_RecordGetString(rec, 12);

    /* fetch the service path */
    row = MSI_QueryGetRecord(package->db, query, comp);
    if (!row)
    {
        ERR("Control query failed!\n");
        goto done;
    }

    key = MSI_RecordGetString(row, 6);
    msiobj_release(&row->hdr);

    file = get_loaded_file(package, key);
    if (!file)
    {
        ERR("Failed to load the service file\n");
        goto done;
    }

    service = CreateServiceW(hscm, name, disp, GENERIC_ALL, serv_type,
                             start_type, err_control, file->TargetPath,
                             load_order, NULL, NULL, serv_name, pass);
    if (!service)
    {
        if (GetLastError() != ERROR_SERVICE_EXISTS)
            ERR("Failed to create service %s: %d\n", debugstr_w(name), GetLastError());
    }

done:
    CloseServiceHandle(service);
    CloseServiceHandle(hscm);

    return ERROR_SUCCESS;
}

static UINT ACTION_InstallServices( MSIPACKAGE *package )
{
    UINT rc;
    MSIQUERY * view;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         'S','e','r','v','i','c','e','I','n','s','t','a','l','l',0};
    
    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_InstallService, package);
    msiobj_release(&view->hdr);

    return rc;
}

static UINT msi_unimplemented_action_stub( MSIPACKAGE *package,
                                           LPCSTR action, LPCWSTR table )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','`','%','s','`',0 };
    MSIQUERY *view = NULL;
    DWORD count = 0;
    UINT r;
    
    r = MSI_OpenQuery( package->db, &view, query, table );
    if (r == ERROR_SUCCESS)
    {
        r = MSI_IterateRecords(view, &count, NULL, package);
        msiobj_release(&view->hdr);
    }

    if (count)
        FIXME("%s -> %u ignored %s table values\n",
              action, count, debugstr_w(table));

    return ERROR_SUCCESS;
}

static UINT ACTION_AllocateRegistrySpace( MSIPACKAGE *package )
{
    TRACE("%p\n", package);
    return ERROR_SUCCESS;
}

static UINT ACTION_RemoveIniValues( MSIPACKAGE *package )
{
    static const WCHAR table[] =
         {'R','e','m','o','v','e','I','n','i','F','i','l','e',0 };
    return msi_unimplemented_action_stub( package, "RemoveIniValues", table );
}

static UINT ACTION_MoveFiles( MSIPACKAGE *package )
{
    static const WCHAR table[] = { 'M','o','v','e','F','i','l','e',0 };
    return msi_unimplemented_action_stub( package, "MoveFiles", table );
}

static UINT ACTION_PatchFiles( MSIPACKAGE *package )
{
    static const WCHAR table[] = { 'P','a','t','c','h',0 };
    return msi_unimplemented_action_stub( package, "PatchFiles", table );
}

static UINT ACTION_BindImage( MSIPACKAGE *package )
{
    static const WCHAR table[] = { 'B','i','n','d','I','m','a','g','e',0 };
    return msi_unimplemented_action_stub( package, "BindImage", table );
}

static UINT ACTION_IsolateComponents( MSIPACKAGE *package )
{
    static const WCHAR table[] = {
        'I','s','o','l','a','t','e','C','o','m','p','o','n','e','n','t',0 };
    return msi_unimplemented_action_stub( package, "IsolateComponents", table );
}

static UINT ACTION_MigrateFeatureStates( MSIPACKAGE *package )
{
    static const WCHAR table[] = { 'U','p','g','r','a','d','e',0 };
    return msi_unimplemented_action_stub( package, "MigrateFeatureStates", table );
}

static UINT ACTION_SelfUnregModules( MSIPACKAGE *package )
{
    static const WCHAR table[] = { 'S','e','l','f','R','e','g',0 };
    return msi_unimplemented_action_stub( package, "SelfUnregModules", table );
}

static UINT ACTION_StartServices( MSIPACKAGE *package )
{
    static const WCHAR table[] = {
        'S','e','r','v','i','c','e','C','o','n','t','r','o','l',0 };
    return msi_unimplemented_action_stub( package, "StartServices", table );
}

static UINT ACTION_StopServices( MSIPACKAGE *package )
{
    static const WCHAR table[] = {
        'S','e','r','v','i','c','e','C','o','n','t','r','o','l',0 };
    return msi_unimplemented_action_stub( package, "StopServices", table );
}

static UINT ACTION_DeleteServices( MSIPACKAGE *package )
{
    static const WCHAR table[] = {
        'S','e','r','v','i','c','e','C','o','n','t','r','o','l',0 };
    return msi_unimplemented_action_stub( package, "DeleteServices", table );
}

static UINT ACTION_WriteEnvironmentStrings( MSIPACKAGE *package )
{
    static const WCHAR table[] = {
        'E','n','v','i','r','o','n','m','e','n','t',0 };
    return msi_unimplemented_action_stub( package, "WriteEnvironmentStrings", table );
}

static UINT ACTION_RemoveEnvironmentStrings( MSIPACKAGE *package )
{
    static const WCHAR table[] = {
        'E','n','v','i','r','o','n','m','e','n','t',0 };
    return msi_unimplemented_action_stub( package, "RemoveEnvironmentStrings", table );
}

static UINT ACTION_MsiPublishAssemblies( MSIPACKAGE *package )
{
    static const WCHAR table[] = {
        'M','s','i','A','s','s','e','m','b','l','y',0 };
    return msi_unimplemented_action_stub( package, "MsiPublishAssemblies", table );
}

static UINT ACTION_MsiUnpublishAssemblies( MSIPACKAGE *package )
{
    static const WCHAR table[] = {
        'M','s','i','A','s','s','e','m','b','l','y',0 };
    return msi_unimplemented_action_stub( package, "MsiUnpublishAssemblies", table );
}

static UINT ACTION_UnregisterFonts( MSIPACKAGE *package )
{
    static const WCHAR table[] = { 'F','o','n','t',0 };
    return msi_unimplemented_action_stub( package, "UnregisterFonts", table );
}

static UINT ACTION_CCPSearch( MSIPACKAGE *package )
{
    static const WCHAR table[] = { 'C','C','P','S','e','a','r','c','h',0 };
    return msi_unimplemented_action_stub( package, "CCPSearch", table );
}

static UINT ACTION_RMCCPSearch( MSIPACKAGE *package )
{
    static const WCHAR table[] = { 'C','C','P','S','e','a','r','c','h',0 };
    return msi_unimplemented_action_stub( package, "RMCCPSearch", table );
}

static UINT ACTION_RegisterComPlus( MSIPACKAGE *package )
{
    static const WCHAR table[] = { 'C','o','m','p','l','u','s',0 };
    return msi_unimplemented_action_stub( package, "RegisterComPlus", table );
}

static UINT ACTION_UnregisterComPlus( MSIPACKAGE *package )
{
    static const WCHAR table[] = { 'C','o','m','p','l','u','s',0 };
    return msi_unimplemented_action_stub( package, "UnregisterComPlus", table );
}

static struct _actions StandardActions[] = {
    { szAllocateRegistrySpace, ACTION_AllocateRegistrySpace },
    { szAppSearch, ACTION_AppSearch },
    { szBindImage, ACTION_BindImage },
    { szCCPSearch, ACTION_CCPSearch},
    { szCostFinalize, ACTION_CostFinalize },
    { szCostInitialize, ACTION_CostInitialize },
    { szCreateFolders, ACTION_CreateFolders },
    { szCreateShortcuts, ACTION_CreateShortcuts },
    { szDeleteServices, ACTION_DeleteServices },
    { szDisableRollback, NULL},
    { szDuplicateFiles, ACTION_DuplicateFiles },
    { szExecuteAction, ACTION_ExecuteAction },
    { szFileCost, ACTION_FileCost },
    { szFindRelatedProducts, ACTION_FindRelatedProducts },
    { szForceReboot, ACTION_ForceReboot },
    { szInstallAdminPackage, NULL},
    { szInstallExecute, ACTION_InstallExecute },
    { szInstallExecuteAgain, ACTION_InstallExecute },
    { szInstallFiles, ACTION_InstallFiles},
    { szInstallFinalize, ACTION_InstallFinalize },
    { szInstallInitialize, ACTION_InstallInitialize },
    { szInstallSFPCatalogFile, NULL},
    { szInstallValidate, ACTION_InstallValidate },
    { szIsolateComponents, ACTION_IsolateComponents },
    { szLaunchConditions, ACTION_LaunchConditions },
    { szMigrateFeatureStates, ACTION_MigrateFeatureStates },
    { szMoveFiles, ACTION_MoveFiles },
    { szMsiPublishAssemblies, ACTION_MsiPublishAssemblies },
    { szMsiUnpublishAssemblies, ACTION_MsiUnpublishAssemblies },
    { szInstallODBC, NULL},
    { szInstallServices, ACTION_InstallServices },
    { szPatchFiles, ACTION_PatchFiles },
    { szProcessComponents, ACTION_ProcessComponents },
    { szPublishComponents, ACTION_PublishComponents },
    { szPublishFeatures, ACTION_PublishFeatures },
    { szPublishProduct, ACTION_PublishProduct },
    { szRegisterClassInfo, ACTION_RegisterClassInfo },
    { szRegisterComPlus, ACTION_RegisterComPlus},
    { szRegisterExtensionInfo, ACTION_RegisterExtensionInfo },
    { szRegisterFonts, ACTION_RegisterFonts },
    { szRegisterMIMEInfo, ACTION_RegisterMIMEInfo },
    { szRegisterProduct, ACTION_RegisterProduct },
    { szRegisterProgIdInfo, ACTION_RegisterProgIdInfo },
    { szRegisterTypeLibraries, ACTION_RegisterTypeLibraries },
    { szRegisterUser, ACTION_RegisterUser},
    { szRemoveDuplicateFiles, NULL},
    { szRemoveEnvironmentStrings, ACTION_RemoveEnvironmentStrings },
    { szRemoveExistingProducts, NULL},
    { szRemoveFiles, ACTION_RemoveFiles},
    { szRemoveFolders, NULL},
    { szRemoveIniValues, ACTION_RemoveIniValues },
    { szRemoveODBC, NULL},
    { szRemoveRegistryValues, NULL},
    { szRemoveShortcuts, NULL},
    { szResolveSource, ACTION_ResolveSource},
    { szRMCCPSearch, ACTION_RMCCPSearch},
    { szScheduleReboot, NULL},
    { szSelfRegModules, ACTION_SelfRegModules },
    { szSelfUnregModules, ACTION_SelfUnregModules },
    { szSetODBCFolders, NULL},
    { szStartServices, ACTION_StartServices },
    { szStopServices, ACTION_StopServices },
    { szUnpublishComponents, NULL},
    { szUnpublishFeatures, NULL},
    { szUnregisterClassInfo, NULL},
    { szUnregisterComPlus, ACTION_UnregisterComPlus},
    { szUnregisterExtensionInfo, NULL},
    { szUnregisterFonts, ACTION_UnregisterFonts },
    { szUnregisterMIMEInfo, NULL},
    { szUnregisterProgIdInfo, NULL},
    { szUnregisterTypeLibraries, NULL},
    { szValidateProductID, NULL},
    { szWriteEnvironmentStrings, ACTION_WriteEnvironmentStrings },
    { szWriteIniValues, ACTION_WriteIniValues },
    { szWriteRegistryValues, ACTION_WriteRegistryValues},
    { NULL, NULL},
};
