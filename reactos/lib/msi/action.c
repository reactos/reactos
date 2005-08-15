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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Pages I need
 *
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/installexecutesequence_table.asp

http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/standard_actions_reference.asp
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/debug.h"
#include "msidefs.h"
#include "msipriv.h"
#include "winuser.h"
#include "shlobj.h"
#include "wine/unicode.h"
#include "winver.h"
#include "action.h"

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
 * action handlers
 */
typedef UINT (*STANDARDACTIONHANDLER)(MSIPACKAGE*);

static UINT ACTION_LaunchConditions(MSIPACKAGE *package);
static UINT ACTION_CostInitialize(MSIPACKAGE *package);
static UINT ACTION_CreateFolders(MSIPACKAGE *package);
static UINT ACTION_CostFinalize(MSIPACKAGE *package);
static UINT ACTION_FileCost(MSIPACKAGE *package);
static UINT ACTION_WriteRegistryValues(MSIPACKAGE *package);
static UINT ACTION_InstallInitialize(MSIPACKAGE *package);
static UINT ACTION_InstallValidate(MSIPACKAGE *package);
static UINT ACTION_ProcessComponents(MSIPACKAGE *package);
static UINT ACTION_RegisterTypeLibraries(MSIPACKAGE *package);
static UINT ACTION_RegisterUser(MSIPACKAGE *package);
static UINT ACTION_CreateShortcuts(MSIPACKAGE *package);
static UINT ACTION_PublishProduct(MSIPACKAGE *package);
static UINT ACTION_WriteIniValues(MSIPACKAGE *package);
static UINT ACTION_SelfRegModules(MSIPACKAGE *package);
static UINT ACTION_PublishFeatures(MSIPACKAGE *package);
static UINT ACTION_RegisterProduct(MSIPACKAGE *package);
static UINT ACTION_InstallExecute(MSIPACKAGE *package);
static UINT ACTION_InstallFinalize(MSIPACKAGE *package);
static UINT ACTION_ForceReboot(MSIPACKAGE *package);
static UINT ACTION_ResolveSource(MSIPACKAGE *package);
static UINT ACTION_ExecuteAction(MSIPACKAGE *package);
static UINT ACTION_RegisterFonts(MSIPACKAGE *package);
static UINT ACTION_PublishComponents(MSIPACKAGE *package);

/*
 * consts and values used
 */
static const WCHAR c_colon[] = {'C',':','\\',0};

const static WCHAR szCreateFolders[] =
    {'C','r','e','a','t','e','F','o','l','d','e','r','s',0};
const static WCHAR szCostFinalize[] =
    {'C','o','s','t','F','i','n','a','l','i','z','e',0};
const WCHAR szInstallFiles[] =
    {'I','n','s','t','a','l','l','F','i','l','e','s',0};
const WCHAR szDuplicateFiles[] =
    {'D','u','p','l','i','c','a','t','e','F','i','l','e','s',0};
const static WCHAR szWriteRegistryValues[] =
    {'W','r','i','t','e','R','e','g','i','s','t','r','y',
            'V','a','l','u','e','s',0};
const static WCHAR szCostInitialize[] =
    {'C','o','s','t','I','n','i','t','i','a','l','i','z','e',0};
const static WCHAR szFileCost[] = 
    {'F','i','l','e','C','o','s','t',0};
const static WCHAR szInstallInitialize[] = 
    {'I','n','s','t','a','l','l','I','n','i','t','i','a','l','i','z','e',0};
const static WCHAR szInstallValidate[] = 
    {'I','n','s','t','a','l','l','V','a','l','i','d','a','t','e',0};
const static WCHAR szLaunchConditions[] = 
    {'L','a','u','n','c','h','C','o','n','d','i','t','i','o','n','s',0};
const static WCHAR szProcessComponents[] = 
    {'P','r','o','c','e','s','s','C','o','m','p','o','n','e','n','t','s',0};
const static WCHAR szRegisterTypeLibraries[] = 
    {'R','e','g','i','s','t','e','r','T','y','p','e',
            'L','i','b','r','a','r','i','e','s',0};
const WCHAR szRegisterClassInfo[] = 
    {'R','e','g','i','s','t','e','r','C','l','a','s','s','I','n','f','o',0};
const WCHAR szRegisterProgIdInfo[] = 
    {'R','e','g','i','s','t','e','r','P','r','o','g','I','d','I','n','f','o',0};
const static WCHAR szCreateShortcuts[] = 
    {'C','r','e','a','t','e','S','h','o','r','t','c','u','t','s',0};
const static WCHAR szPublishProduct[] = 
    {'P','u','b','l','i','s','h','P','r','o','d','u','c','t',0};
const static WCHAR szWriteIniValues[] = 
    {'W','r','i','t','e','I','n','i','V','a','l','u','e','s',0};
const static WCHAR szSelfRegModules[] = 
    {'S','e','l','f','R','e','g','M','o','d','u','l','e','s',0};
const static WCHAR szPublishFeatures[] = 
    {'P','u','b','l','i','s','h','F','e','a','t','u','r','e','s',0};
const static WCHAR szRegisterProduct[] = 
    {'R','e','g','i','s','t','e','r','P','r','o','d','u','c','t',0};
const static WCHAR szInstallExecute[] = 
    {'I','n','s','t','a','l','l','E','x','e','c','u','t','e',0};
const static WCHAR szInstallExecuteAgain[] = 
    {'I','n','s','t','a','l','l','E','x','e','c','u','t','e',
            'A','g','a','i','n',0};
const static WCHAR szInstallFinalize[] = 
    {'I','n','s','t','a','l','l','F','i','n','a','l','i','z','e',0};
const static WCHAR szForceReboot[] = 
    {'F','o','r','c','e','R','e','b','o','o','t',0};
const static WCHAR szResolveSource[] =
    {'R','e','s','o','l','v','e','S','o','u','r','c','e',0};
const WCHAR szAppSearch[] = 
    {'A','p','p','S','e','a','r','c','h',0};
const static WCHAR szAllocateRegistrySpace[] = 
    {'A','l','l','o','c','a','t','e','R','e','g','i','s','t','r','y',
            'S','p','a','c','e',0};
const static WCHAR szBindImage[] = 
    {'B','i','n','d','I','m','a','g','e',0};
const static WCHAR szCCPSearch[] = 
    {'C','C','P','S','e','a','r','c','h',0};
const static WCHAR szDeleteServices[] = 
    {'D','e','l','e','t','e','S','e','r','v','i','c','e','s',0};
const static WCHAR szDisableRollback[] = 
    {'D','i','s','a','b','l','e','R','o','l','l','b','a','c','k',0};
const static WCHAR szExecuteAction[] = 
    {'E','x','e','c','u','t','e','A','c','t','i','o','n',0};
const WCHAR szFindRelatedProducts[] = 
    {'F','i','n','d','R','e','l','a','t','e','d',
            'P','r','o','d','u','c','t','s',0};
const static WCHAR szInstallAdminPackage[] = 
    {'I','n','s','t','a','l','l','A','d','m','i','n',
            'P','a','c','k','a','g','e',0};
const static WCHAR szInstallSFPCatalogFile[] = 
    {'I','n','s','t','a','l','l','S','F','P','C','a','t','a','l','o','g',
            'F','i','l','e',0};
const static WCHAR szIsolateComponents[] = 
    {'I','s','o','l','a','t','e','C','o','m','p','o','n','e','n','t','s',0};
const WCHAR szMigrateFeatureStates[] = 
    {'M','i','g','r','a','t','e','F','e','a','t','u','r','e',
            'S','t','a','t','e','s',0};
const WCHAR szMoveFiles[] = 
    {'M','o','v','e','F','i','l','e','s',0};
const static WCHAR szMsiPublishAssemblies[] = 
    {'M','s','i','P','u','b','l','i','s','h',
            'A','s','s','e','m','b','l','i','e','s',0};
const static WCHAR szMsiUnpublishAssemblies[] = 
    {'M','s','i','U','n','p','u','b','l','i','s','h',
            'A','s','s','e','m','b','l','i','e','s',0};
const static WCHAR szInstallODBC[] = 
    {'I','n','s','t','a','l','l','O','D','B','C',0};
const static WCHAR szInstallServices[] = 
    {'I','n','s','t','a','l','l','S','e','r','v','i','c','e','s',0};
const WCHAR szPatchFiles[] = 
    {'P','a','t','c','h','F','i','l','e','s',0};
const static WCHAR szPublishComponents[] = 
    {'P','u','b','l','i','s','h','C','o','m','p','o','n','e','n','t','s',0};
const static WCHAR szRegisterComPlus[] =
    {'R','e','g','i','s','t','e','r','C','o','m','P','l','u','s',0};
const WCHAR szRegisterExtensionInfo[] =
    {'R','e','g','i','s','t','e','r','E','x','t','e','n','s','i','o','n',
            'I','n','f','o',0};
const static WCHAR szRegisterFonts[] =
    {'R','e','g','i','s','t','e','r','F','o','n','t','s',0};
const WCHAR szRegisterMIMEInfo[] =
    {'R','e','g','i','s','t','e','r','M','I','M','E','I','n','f','o',0};
const static WCHAR szRegisterUser[] =
    {'R','e','g','i','s','t','e','r','U','s','e','r',0};
const WCHAR szRemoveDuplicateFiles[] =
    {'R','e','m','o','v','e','D','u','p','l','i','c','a','t','e',
            'F','i','l','e','s',0};
const static WCHAR szRemoveEnvironmentStrings[] =
    {'R','e','m','o','v','e','E','n','v','i','r','o','n','m','e','n','t',
            'S','t','r','i','n','g','s',0};
const WCHAR szRemoveExistingProducts[] =
    {'R','e','m','o','v','e','E','x','i','s','t','i','n','g',
            'P','r','o','d','u','c','t','s',0};
const WCHAR szRemoveFiles[] =
    {'R','e','m','o','v','e','F','i','l','e','s',0};
const static WCHAR szRemoveFolders[] =
    {'R','e','m','o','v','e','F','o','l','d','e','r','s',0};
const static WCHAR szRemoveIniValues[] =
    {'R','e','m','o','v','e','I','n','i','V','a','l','u','e','s',0};
const static WCHAR szRemoveODBC[] =
    {'R','e','m','o','v','e','O','D','B','C',0};
const static WCHAR szRemoveRegistryValues[] =
    {'R','e','m','o','v','e','R','e','g','i','s','t','r','y',
            'V','a','l','u','e','s',0};
const static WCHAR szRemoveShortcuts[] =
    {'R','e','m','o','v','e','S','h','o','r','t','c','u','t','s',0};
const static WCHAR szRMCCPSearch[] =
    {'R','M','C','C','P','S','e','a','r','c','h',0};
const static WCHAR szScheduleReboot[] =
    {'S','c','h','e','d','u','l','e','R','e','b','o','o','t',0};
const static WCHAR szSelfUnregModules[] =
    {'S','e','l','f','U','n','r','e','g','M','o','d','u','l','e','s',0};
const static WCHAR szSetODBCFolders[] =
    {'S','e','t','O','D','B','C','F','o','l','d','e','r','s',0};
const static WCHAR szStartServices[] =
    {'S','t','a','r','t','S','e','r','v','i','c','e','s',0};
const static WCHAR szStopServices[] =
    {'S','t','o','p','S','e','r','v','i','c','e','s',0};
const static WCHAR szUnpublishComponents[] =
    {'U','n','p','u','b','l','i','s','h',
            'C','o','m','p','o','n','e','n','t','s',0};
const static WCHAR szUnpublishFeatures[] =
    {'U','n','p','u','b','l','i','s','h','F','e','a','t','u','r','e','s',0};
const WCHAR szUnregisterClassInfo[] =
    {'U','n','r','e','g','i','s','t','e','r','C','l','a','s','s',
            'I','n','f','o',0};
const static WCHAR szUnregisterComPlus[] =
    {'U','n','r','e','g','i','s','t','e','r','C','o','m','P','l','u','s',0};
const WCHAR szUnregisterExtensionInfo[] =
    {'U','n','r','e','g','i','s','t','e','r',
            'E','x','t','e','n','s','i','o','n','I','n','f','o',0};
const static WCHAR szUnregisterFonts[] =
    {'U','n','r','e','g','i','s','t','e','r','F','o','n','t','s',0};
const WCHAR szUnregisterMIMEInfo[] =
    {'U','n','r','e','g','i','s','t','e','r','M','I','M','E','I','n','f','o',0};
const WCHAR szUnregisterProgIdInfo[] =
    {'U','n','r','e','g','i','s','t','e','r','P','r','o','g','I','d',
            'I','n','f','o',0};
const static WCHAR szUnregisterTypeLibraries[] =
    {'U','n','r','e','g','i','s','t','e','r','T','y','p','e',
            'L','i','b','r','a','r','i','e','s',0};
const static WCHAR szValidateProductID[] =
    {'V','a','l','i','d','a','t','e','P','r','o','d','u','c','t','I','D',0};
const static WCHAR szWriteEnvironmentStrings[] =
    {'W','r','i','t','e','E','n','v','i','r','o','n','m','e','n','t',
            'S','t','r','i','n','g','s',0};

struct _actions {
    LPCWSTR action;
    STANDARDACTIONHANDLER handler;
};

static struct _actions StandardActions[] = {
    { szAllocateRegistrySpace, NULL},
    { szAppSearch, ACTION_AppSearch },
    { szBindImage, NULL},
    { szCCPSearch, NULL},
    { szCostFinalize, ACTION_CostFinalize },
    { szCostInitialize, ACTION_CostInitialize },
    { szCreateFolders, ACTION_CreateFolders },
    { szCreateShortcuts, ACTION_CreateShortcuts },
    { szDeleteServices, NULL},
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
    { szIsolateComponents, NULL},
    { szLaunchConditions, ACTION_LaunchConditions },
    { szMigrateFeatureStates, NULL},
    { szMoveFiles, NULL},
    { szMsiPublishAssemblies, NULL},
    { szMsiUnpublishAssemblies, NULL},
    { szInstallODBC, NULL},
    { szInstallServices, NULL},
    { szPatchFiles, NULL},
    { szProcessComponents, ACTION_ProcessComponents },
    { szPublishComponents, ACTION_PublishComponents },
    { szPublishFeatures, ACTION_PublishFeatures },
    { szPublishProduct, ACTION_PublishProduct },
    { szRegisterClassInfo, ACTION_RegisterClassInfo },
    { szRegisterComPlus, NULL},
    { szRegisterExtensionInfo, ACTION_RegisterExtensionInfo },
    { szRegisterFonts, ACTION_RegisterFonts },
    { szRegisterMIMEInfo, ACTION_RegisterMIMEInfo },
    { szRegisterProduct, ACTION_RegisterProduct },
    { szRegisterProgIdInfo, ACTION_RegisterProgIdInfo },
    { szRegisterTypeLibraries, ACTION_RegisterTypeLibraries },
    { szRegisterUser, ACTION_RegisterUser},
    { szRemoveDuplicateFiles, NULL},
    { szRemoveEnvironmentStrings, NULL},
    { szRemoveExistingProducts, NULL},
    { szRemoveFiles, NULL},
    { szRemoveFolders, NULL},
    { szRemoveIniValues, NULL},
    { szRemoveODBC, NULL},
    { szRemoveRegistryValues, NULL},
    { szRemoveShortcuts, NULL},
    { szResolveSource, ACTION_ResolveSource},
    { szRMCCPSearch, NULL},
    { szScheduleReboot, NULL},
    { szSelfRegModules, ACTION_SelfRegModules },
    { szSelfUnregModules, NULL},
    { szSetODBCFolders, NULL},
    { szStartServices, NULL},
    { szStopServices, NULL},
    { szUnpublishComponents, NULL},
    { szUnpublishFeatures, NULL},
    { szUnregisterClassInfo, NULL},
    { szUnregisterComPlus, NULL},
    { szUnregisterExtensionInfo, NULL},
    { szUnregisterFonts, NULL},
    { szUnregisterMIMEInfo, NULL},
    { szUnregisterProgIdInfo, NULL},
    { szUnregisterTypeLibraries, NULL},
    { szValidateProductID, NULL},
    { szWriteEnvironmentStrings, NULL},
    { szWriteIniValues, ACTION_WriteIniValues },
    { szWriteRegistryValues, ACTION_WriteRegistryValues},
    { NULL, NULL},
};


/********************************************************
 * helper functions
 ********************************************************/

static void ce_actiontext(MSIPACKAGE* package, LPCWSTR action)
{
    static const WCHAR szActionText[] = 
        {'A','c','t','i','o','n','T','e','x','t',0};
    MSIRECORD *row;

    row = MSI_CreateRecord(1);
    MSI_RecordSetStringW(row,1,action);
    ControlEvent_FireSubscribedEvent(package,szActionText, row);
    msiobj_release(&row->hdr);
}

static void ui_actionstart(MSIPACKAGE *package, LPCWSTR action)
{
    static const WCHAR template_s[]=
        {'A','c','t','i','o','n',' ','%','s',':',' ','%','s','.',' ', '%','s',
         '.',0};
    static const WCHAR format[] = 
        {'H','H','\'',':','\'','m','m','\'',':','\'','s','s',0};
    static const WCHAR Query_t[] = 
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','A','c','t','i','o', 'n','T','e','x','t','`',' ',
         'W','H','E','R','E', ' ','`','A','c','t','i','o','n','`',' ','=', 
         ' ','\'','%','s','\'',0};
    WCHAR message[1024];
    WCHAR timet[0x100];
    MSIRECORD * row = 0;
    LPCWSTR ActionText;

    GetTimeFormatW(LOCALE_USER_DEFAULT, 0, NULL, format, timet, 0x100);

    row = MSI_QueryGetRecord( package->db, Query_t, action );
    if (!row)
        return;

    ActionText = MSI_RecordGetString(row,2);

    sprintfW(message,template_s,timet,action,ActionText);
    msiobj_release(&row->hdr);

    row = MSI_CreateRecord(1);
    MSI_RecordSetStringW(row,1,message);
 
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

/****************************************************
 * TOP level entry points 
 *****************************************************/

UINT ACTION_DoTopLevelINSTALL(MSIPACKAGE *package, LPCWSTR szPackagePath,
                              LPCWSTR szCommandLine, LPCWSTR msiFilePath)
{
    DWORD sz;
    WCHAR buffer[10];
    UINT rc;
    BOOL ui = FALSE;
    static const WCHAR szUILevel[] = {'U','I','L','e','v','e','l',0};
    static const WCHAR szAction[] = {'A','C','T','I','O','N',0};
    static const WCHAR szInstall[] = {'I','N','S','T','A','L','L',0};

    MSI_SetPropertyW(package, szAction, szInstall);

    package->script = HeapAlloc(GetProcessHeap(),0,sizeof(MSISCRIPT));
    memset(package->script,0,sizeof(MSISCRIPT));

    package->script->InWhatSequence = SEQUENCE_INSTALL;

    package->msiFilePath= strdupW(msiFilePath);

    if (szPackagePath)   
    {
        LPWSTR p, check, path;
 
        package->PackagePath = strdupW(szPackagePath);
        path = strdupW(szPackagePath);
        p = strrchrW(path,'\\');    
        if (p)
        {
            p++;
            *p=0;
        }
        else
        {
            HeapFree(GetProcessHeap(),0,path);
            path = HeapAlloc(GetProcessHeap(),0,MAX_PATH*sizeof(WCHAR));
            GetCurrentDirectoryW(MAX_PATH,path);
            strcatW(path,cszbs);
        }

        check = load_dynamic_property(package, cszSourceDir,NULL);
        if (!check)
            MSI_SetPropertyW(package, cszSourceDir, path);
        else
            HeapFree(GetProcessHeap(), 0, check);

        HeapFree(GetProcessHeap(), 0, path);
    }

    if (szCommandLine)
    {
        LPWSTR ptr,ptr2;
        ptr = (LPWSTR)szCommandLine;
       
        while (*ptr)
        {
            WCHAR *prop = NULL;
            WCHAR *val = NULL;

            TRACE("Looking at %s\n",debugstr_w(ptr));

            ptr2 = strchrW(ptr,'=');
            if (ptr2)
            {
                BOOL quote=FALSE;
                DWORD len = 0;

                while (*ptr == ' ') ptr++;
                len = ptr2-ptr;
                prop = HeapAlloc(GetProcessHeap(),0,(len+1)*sizeof(WCHAR));
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
                val = HeapAlloc(GetProcessHeap(),0,(len+1)*sizeof(WCHAR));
                memcpy(val,ptr2,len*sizeof(WCHAR));
                val[len] = 0;

                if (strlenW(prop) > 0)
                {
                    TRACE("Found commandline property (%s) = (%s)\n", 
                                       debugstr_w(prop), debugstr_w(val));
                    MSI_SetPropertyW(package,prop,val);
                }
                HeapFree(GetProcessHeap(),0,val);
                HeapFree(GetProcessHeap(),0,prop);
            }
            ptr++;
        }
    }
  
    sz = 10; 
    if (MSI_GetPropertyW(package,szUILevel,buffer,&sz) == ERROR_SUCCESS)
    {
        if (atoiW(buffer) >= INSTALLUILEVEL_REDUCED)
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
        if (cond)
        {
            /* this is a hack to skip errors in the condition code */
            if (MSI_EvaluateConditionW(package, cond) == MSICONDITION_FALSE)
                goto end;
        }

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
        return  ERROR_FUNCTION_FAILED;
    }

    /* check conditions */
    cond = MSI_RecordGetString(row,2);
    if (cond)
    {
        /* this is a hack to skip errors in the condition code */
        if (MSI_EvaluateConditionW(iap->package, cond) == MSICONDITION_FALSE)
        {
            TRACE("Skipping action: %s (condition is false)\n",
                            debugstr_w(action));
            return ERROR_SUCCESS;
        }
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
        ERR("Execution halted due to error (%i)\n",rc);

    return rc;
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
        TRACE("Running the actions \n"); 

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
            ce_actiontext(package, action);
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
                    FIXME("UNHANDLED Standard Action %s\n",debugstr_w(action));
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
        FIXME("UNHANDLED MSI ACTION %s\n",debugstr_w(action));
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
        FIXME("UNHANDLED MSI ACTION %s\n",debugstr_w(action));
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
        ERR("Unable to get folder id \n");
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

    HeapFree(GetProcessHeap(),0,full_path);
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

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_CreateFolders, package);
    msiobj_release(&view->hdr);
   
    return rc;
}

static int load_component(MSIPACKAGE* package, MSIRECORD * row)
{
    int index = package->loaded_components;
    DWORD sz;

    /* fill in the data */

    package->loaded_components++;
    if (package->loaded_components == 1)
        package->components = HeapAlloc(GetProcessHeap(),0,
                                        sizeof(MSICOMPONENT));
    else
        package->components = HeapReAlloc(GetProcessHeap(),0,
            package->components, package->loaded_components * 
            sizeof(MSICOMPONENT));

    memset(&package->components[index],0,sizeof(MSICOMPONENT));

    sz = IDENTIFIER_SIZE;       
    MSI_RecordGetStringW(row,1,package->components[index].Component,&sz);

    TRACE("Loading Component %s\n",
           debugstr_w(package->components[index].Component));

    sz = 0x100;
    if (!MSI_RecordIsNull(row,2))
        MSI_RecordGetStringW(row,2,package->components[index].ComponentId,&sz);
            
    sz = IDENTIFIER_SIZE;       
    MSI_RecordGetStringW(row,3,package->components[index].Directory,&sz);

    package->components[index].Attributes = MSI_RecordGetInteger(row,4);

    sz = 0x100;       
    MSI_RecordGetStringW(row,5,package->components[index].Condition,&sz);

    sz = IDENTIFIER_SIZE;       
    MSI_RecordGetStringW(row,6,package->components[index].KeyPath,&sz);

    package->components[index].Installed = INSTALLSTATE_ABSENT;
    package->components[index].Action = INSTALLSTATE_UNKNOWN;
    package->components[index].ActionRequest = INSTALLSTATE_UNKNOWN;

    package->components[index].Enabled = TRUE;

    return index;
}

typedef struct {
    MSIPACKAGE *package;
    INT index;
    INT cnt;
} _ilfs;

static UINT iterate_component_check(MSIRECORD *row, LPVOID param)
{
    _ilfs* ilfs= (_ilfs*)param;
    INT c_indx;

    c_indx = load_component(ilfs->package,row);

    ilfs->package->features[ilfs->index].Components[ilfs->cnt] = c_indx;
    ilfs->package->features[ilfs->index].ComponentCount ++;
    TRACE("Loaded new component to index %i\n",c_indx);

    return ERROR_SUCCESS;
}

static UINT iterate_load_featurecomponents(MSIRECORD *row, LPVOID param)
{
    _ilfs* ilfs= (_ilfs*)param;
    LPCWSTR component;
    DWORD rc;
    INT c_indx;
    INT cnt = ilfs->package->features[ilfs->index].ComponentCount;
    MSIQUERY * view;
    static const WCHAR Query[] = 
        {'S','E','L','E','C','T',' ','*',' ','F','R', 'O','M',' ', 
         '`','C','o','m','p','o','n','e','n','t','`',' ',
         'W','H','E','R','E',' ', 
         '`','C','o','m','p','o','n','e','n','t','`',' ',
         '=','\'','%','s','\'',0};

    component = MSI_RecordGetString(row,1);

    /* check to see if the component is already loaded */
    c_indx = get_loaded_component(ilfs->package,component);
    if (c_indx != -1)
    {
        TRACE("Component %s already loaded at %i\n", debugstr_w(component),
                        c_indx);
        ilfs->package->features[ilfs->index].Components[cnt] = c_indx;
        ilfs->package->features[ilfs->index].ComponentCount ++;
        return ERROR_SUCCESS;
    }

    rc = MSI_OpenQuery(ilfs->package->db, &view, Query, component);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    ilfs->cnt = cnt;
    rc = MSI_IterateRecords(view, NULL, iterate_component_check, ilfs);
    msiobj_release( &view->hdr );

    return ERROR_SUCCESS;
}

static UINT load_feature(MSIRECORD * row, LPVOID param)
{
    MSIPACKAGE* package = (MSIPACKAGE*)param;
    int index = package->loaded_features;
    DWORD sz;
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

    ilfs.package = package;
    ilfs.index = index;

    /* fill in the data */

    package->loaded_features ++;
    if (package->loaded_features == 1)
        package->features = HeapAlloc(GetProcessHeap(),0,sizeof(MSIFEATURE));
    else
        package->features = HeapReAlloc(GetProcessHeap(),0,package->features,
                                package->loaded_features * sizeof(MSIFEATURE));

    memset(&package->features[index],0,sizeof(MSIFEATURE));
    
    sz = IDENTIFIER_SIZE;       
    MSI_RecordGetStringW(row,1,package->features[index].Feature,&sz);

    TRACE("Loading feature %s\n",debugstr_w(package->features[index].Feature));

    sz = IDENTIFIER_SIZE;
    if (!MSI_RecordIsNull(row,2))
        MSI_RecordGetStringW(row,2,package->features[index].Feature_Parent,&sz);

    sz = 0x100;
     if (!MSI_RecordIsNull(row,3))
        MSI_RecordGetStringW(row,3,package->features[index].Title,&sz);

     sz = 0x100;
     if (!MSI_RecordIsNull(row,4))
        MSI_RecordGetStringW(row,4,package->features[index].Description,&sz);

    if (!MSI_RecordIsNull(row,5))
        package->features[index].Display = MSI_RecordGetInteger(row,5);
  
    package->features[index].Level= MSI_RecordGetInteger(row,6);

     sz = IDENTIFIER_SIZE;
     if (!MSI_RecordIsNull(row,7))
        MSI_RecordGetStringW(row,7,package->features[index].Directory,&sz);

    package->features[index].Attributes= MSI_RecordGetInteger(row,8);

    package->features[index].Installed = INSTALLSTATE_ABSENT;
    package->features[index].Action = INSTALLSTATE_UNKNOWN;
    package->features[index].ActionRequest = INSTALLSTATE_UNKNOWN;

    /* load feature components */

    rc = MSI_OpenQuery(package->db, &view, Query1, 
                    package->features[index].Feature);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    MSI_IterateRecords(view, NULL, iterate_load_featurecomponents , &ilfs);
    msiobj_release(&view->hdr);

    return ERROR_SUCCESS;
}

static UINT load_file(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE* package = (MSIPACKAGE*)param;
    DWORD index = package->loaded_files;
    LPCWSTR component;

    /* fill in the data */

    package->loaded_files++;
    if (package->loaded_files== 1)
        package->files = HeapAlloc(GetProcessHeap(),0,sizeof(MSIFILE));
    else
        package->files = HeapReAlloc(GetProcessHeap(),0,
            package->files , package->loaded_files * sizeof(MSIFILE));

    memset(&package->files[index],0,sizeof(MSIFILE));
 
    package->files[index].File = load_dynamic_stringW(row, 1);

    component = MSI_RecordGetString(row, 2);
    package->files[index].ComponentIndex = get_loaded_component(package,
                    component);

    if (package->files[index].ComponentIndex == -1)
        ERR("Unfound Component %s\n",debugstr_w(component));

    package->files[index].FileName = load_dynamic_stringW(row,3);
    reduce_to_longfilename(package->files[index].FileName);

    package->files[index].ShortName = load_dynamic_stringW(row,3);
    reduce_to_shortfilename(package->files[index].ShortName);
    
    package->files[index].FileSize = MSI_RecordGetInteger(row,4);
    package->files[index].Version = load_dynamic_stringW(row, 5);
    package->files[index].Language = load_dynamic_stringW(row, 6);
    package->files[index].Attributes= MSI_RecordGetInteger(row,7);
    package->files[index].Sequence= MSI_RecordGetInteger(row,8);

    package->files[index].Temporary = FALSE;
    package->files[index].State = 0;

    TRACE("File Loaded (%s)\n",debugstr_w(package->files[index].File));  
 
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

    if (!package)
        return ERROR_INVALID_HANDLE;

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
    MSIQUERY * view;
    UINT rc;
    static const WCHAR Query_all[] =
        {'S','E','L','E','C','T',' ','*',' ', 'F','R','O','M',' ',
         '`','F','e','a','t','u','r','e','`',0};
    static const WCHAR szCosting[] =
        {'C','o','s','t','i','n','g','C','o','m','p','l','e','t','e',0 };
    static const WCHAR szZero[] = { '0', 0 };
    WCHAR buffer[3];
    DWORD sz = 3;

    MSI_GetPropertyW(package, szCosting, buffer, &sz);
    if (buffer[0]=='1')
        return ERROR_SUCCESS;
    
    MSI_SetPropertyW(package, szCosting, szZero);
    MSI_SetPropertyW(package, cszRootDrive , c_colon);

    rc = MSI_DatabaseOpenViewW(package->db,Query_all,&view);
    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MSI_IterateRecords(view, NULL, load_feature, package);
    msiobj_release(&view->hdr);

    load_all_files(package);

    return ERROR_SUCCESS;
}

static UINT execute_script(MSIPACKAGE *package, UINT script )
{
    int i;
    UINT rc = ERROR_SUCCESS;

    TRACE("Executing Script %i\n",script);

    for (i = 0; i < package->script->ActionCount[script]; i++)
    {
        LPWSTR action;
        action = package->script->Actions[script][i];
        ui_actionstart(package, action);
        TRACE("Executing Action (%s)\n",debugstr_w(action));
        rc = ACTION_PerformAction(package, action, TRUE);
        HeapFree(GetProcessHeap(),0,package->script->Actions[script][i]);
        if (rc != ERROR_SUCCESS)
            break;
    }
    HeapFree(GetProcessHeap(),0,package->script->Actions[script]);

    package->script->ActionCount[script] = 0;
    package->script->Actions[script] = NULL;
    return rc;
}

static UINT ACTION_FileCost(MSIPACKAGE *package)
{
    return ERROR_SUCCESS;
}


static INT load_folder(MSIPACKAGE *package, const WCHAR* dir)
{
    static const WCHAR Query[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','D','i','r','e','c', 't','o','r','y','`',' ',
         'W','H','E','R','E',' ', '`', 'D','i','r','e','c','t', 'o','r','y','`',
         ' ','=',' ','\'','%','s','\'',
         0};
    LPWSTR ptargetdir, targetdir, srcdir;
    LPCWSTR parent;
    LPWSTR shortname = NULL;
    MSIRECORD * row = 0;
    INT index = -1;
    DWORD i;

    TRACE("Looking for dir %s\n",debugstr_w(dir));

    for (i = 0; i < package->loaded_folders; i++)
    {
        if (strcmpW(package->folders[i].Directory,dir)==0)
        {
            TRACE(" %s retuning on index %lu\n",debugstr_w(dir),i);
            return i;
        }
    }

    TRACE("Working to load %s\n",debugstr_w(dir));

    index = package->loaded_folders++;
    if (package->loaded_folders==1)
        package->folders = HeapAlloc(GetProcessHeap(),0,
                                        sizeof(MSIFOLDER));
    else
        package->folders= HeapReAlloc(GetProcessHeap(),0,
            package->folders, package->loaded_folders* 
            sizeof(MSIFOLDER));

    memset(&package->folders[index],0,sizeof(MSIFOLDER));

    package->folders[index].Directory = strdupW(dir);

    row = MSI_QueryGetRecord(package->db, Query, dir);
    if (!row)
        return -1;

    ptargetdir = targetdir = load_dynamic_stringW(row,3);

    /* split src and target dir */
    if (strchrW(targetdir,':'))
    {
        srcdir=strchrW(targetdir,':');
        *srcdir=0;
        srcdir ++;
    }
    else
        srcdir=NULL;

    /* for now only pick long filename versions */
    if (strchrW(targetdir,'|'))
    {
        shortname = targetdir;
        targetdir = strchrW(targetdir,'|'); 
        *targetdir = 0;
        targetdir ++;
    }
    /* for the sourcedir pick the short filename */
    if (srcdir && strchrW(srcdir,'|'))
    {
        LPWSTR p = strchrW(srcdir,'|'); 
        *p = 0;
    }

    /* now check for root dirs */
    if (targetdir[0] == '.' && targetdir[1] == 0)
        targetdir = NULL;
        
    if (targetdir)
    {
        TRACE("   TargetDefault = %s\n",debugstr_w(targetdir));
        HeapFree(GetProcessHeap(),0, package->folders[index].TargetDefault);
        package->folders[index].TargetDefault = strdupW(targetdir);
    }

    if (srcdir)
        package->folders[index].SourceDefault = strdupW(srcdir);
    else if (shortname)
        package->folders[index].SourceDefault = strdupW(shortname);
    else if (targetdir)
        package->folders[index].SourceDefault = strdupW(targetdir);
    HeapFree(GetProcessHeap(), 0, ptargetdir);
        TRACE("   SourceDefault = %s\n",debugstr_w(package->folders[index].SourceDefault));

    parent = MSI_RecordGetString(row,2);
    if (parent) 
    {
        i = load_folder(package,parent);
        package->folders[index].ParentIndex = i;
        TRACE("Parent is index %i... %s %s\n",
                    package->folders[index].ParentIndex,
        debugstr_w(package->folders[package->folders[index].ParentIndex].Directory),
                    debugstr_w(parent));
    }
    else
        package->folders[index].ParentIndex = -2;

    package->folders[index].Property = load_dynamic_property(package, dir,NULL);

    msiobj_release(&row->hdr);
    TRACE(" %s retuning on index %i\n",debugstr_w(dir),index);
    return index;
}

/* scan for and update current install states */
static void ACTION_UpdateInstallStates(MSIPACKAGE *package)
{
    int i;

    for (i = 0; i < package->loaded_components; i++)
    {
        INSTALLSTATE res;
        res = MsiGetComponentPathW(package->ProductCode, 
                        package->components[i].ComponentId , NULL, NULL);
        if (res < 0)
            res = INSTALLSTATE_ABSENT;
        package->components[i].Installed = res;
    }

    for (i = 0; i < package->loaded_features; i++)
    {
        INSTALLSTATE res = -10;
        int j;
        for (j = 0; j < package->features[i].ComponentCount; j++)
        {
            MSICOMPONENT* component = &package->components[package->features[i].
                                                           Components[j]];
            if (res == -10)
                res = component->Installed;
            else
            {
                if (res == component->Installed)
                    continue;

                if (res != component->Installed)
                        res = INSTALLSTATE_INCOMPLETE;
            }
        }
    }
}

static BOOL process_state_property (MSIPACKAGE* package, LPCWSTR property, 
                                    INSTALLSTATE state)
{
    static const WCHAR all[]={'A','L','L',0};
    LPWSTR override = NULL;
    INT i;
    BOOL rc = FALSE;

    override = load_dynamic_property(package, property, NULL);
    if (override)
    {
        rc = TRUE;
        for(i = 0; i < package->loaded_features; i++)
        {
            if (strcmpiW(override,all)==0)
            {
                package->features[i].ActionRequest= state;
                package->features[i].Action = state;
            }
            else
            {
                LPWSTR ptr = override;
                LPWSTR ptr2 = strchrW(override,',');

                while (ptr)
                {
                    if ((ptr2 && 
                        strncmpW(ptr,package->features[i].Feature, ptr2-ptr)==0)
                        || (!ptr2 &&
                        strcmpW(ptr,package->features[i].Feature)==0))
                    {
                        package->features[i].ActionRequest= state;
                        package->features[i].Action = state;
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
        HeapFree(GetProcessHeap(),0,override);
    } 

    return rc;
}

static UINT SetFeatureStates(MSIPACKAGE *package)
{
    LPWSTR level;
    INT install_level;
    DWORD i;
    INT j;
    static const WCHAR szlevel[] =
        {'I','N','S','T','A','L','L','L','E','V','E','L',0};
    static const WCHAR szAddLocal[] =
        {'A','D','D','L','O','C','A','L',0};
    static const WCHAR szRemove[] =
        {'R','E','M','O','V','E',0};
    BOOL override = FALSE;

    /* I do not know if this is where it should happen.. but */

    TRACE("Checking Install Level\n");

    level = load_dynamic_property(package,szlevel,NULL);
    if (level)
    {
        install_level = atoiW(level);
        HeapFree(GetProcessHeap(), 0, level);
    }
    else
        install_level = 1;

    /* ok hereis the _real_ rub
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

    if (!override)
    {
        for(i = 0; i < package->loaded_features; i++)
        {
            BOOL feature_state = ((package->features[i].Level > 0) &&
                             (package->features[i].Level <= install_level));

            if ((feature_state) && 
               (package->features[i].Action == INSTALLSTATE_UNKNOWN))
            {
                if (package->features[i].Attributes & 
                                msidbFeatureAttributesFavorSource)
                {
                    package->features[i].ActionRequest = INSTALLSTATE_SOURCE;
                    package->features[i].Action = INSTALLSTATE_SOURCE;
                }
                else if (package->features[i].Attributes &
                                msidbFeatureAttributesFavorAdvertise)
                {
                    package->features[i].ActionRequest =INSTALLSTATE_ADVERTISED;
                    package->features[i].Action =INSTALLSTATE_ADVERTISED;
                }
                else
                {
                    package->features[i].ActionRequest = INSTALLSTATE_LOCAL;
                    package->features[i].Action = INSTALLSTATE_LOCAL;
                }
            }
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

    for(i = 0; i < package->loaded_features; i++)
    {
        MSIFEATURE* feature = &package->features[i];
        TRACE("Examining Feature %s (Installed %i, Action %i, Request %i)\n",
            debugstr_w(feature->Feature), feature->Installed, feature->Action,
            feature->ActionRequest);

        for( j = 0; j < feature->ComponentCount; j++)
        {
            MSICOMPONENT* component = &package->components[
                                                    feature->Components[j]];

            if (!component->Enabled)
            {
                component->Action = INSTALLSTATE_UNKNOWN;
                component->ActionRequest = INSTALLSTATE_UNKNOWN;
            }
            else
            {
                if (feature->Action == INSTALLSTATE_LOCAL)
                {
                    component->Action = INSTALLSTATE_LOCAL;
                    component->ActionRequest = INSTALLSTATE_LOCAL;
                }
                else if (feature->ActionRequest == INSTALLSTATE_SOURCE)
                {
                    if ((component->Action == INSTALLSTATE_UNKNOWN) ||
                        (component->Action == INSTALLSTATE_ABSENT) ||
                        (component->Action == INSTALLSTATE_ADVERTISED))
                           
                    {
                        component->Action = INSTALLSTATE_SOURCE;
                        component->ActionRequest = INSTALLSTATE_SOURCE;
                    }
                }
                else if (feature->ActionRequest == INSTALLSTATE_ADVERTISED)
                {
                    if ((component->Action == INSTALLSTATE_UNKNOWN) ||
                        (component->Action == INSTALLSTATE_ABSENT))
                           
                    {
                        component->Action = INSTALLSTATE_ADVERTISED;
                        component->ActionRequest = INSTALLSTATE_ADVERTISED;
                    }
                }
                else if (feature->ActionRequest == INSTALLSTATE_ABSENT)
                {
                    if (component->Action == INSTALLSTATE_UNKNOWN)
                    {
                        component->Action = INSTALLSTATE_ABSENT;
                        component->ActionRequest = INSTALLSTATE_ABSENT;
                    }
                }
            }
        }
    } 

    for(i = 0; i < package->loaded_components; i++)
    {
        MSICOMPONENT* component= &package->components[i];

        TRACE("Result: Component %s (Installed %i, Action %i, Request %i)\n",
            debugstr_w(component->Component), component->Installed, 
            component->Action, component->ActionRequest);
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
    HeapFree( GetProcessHeap(), 0, path);

    return ERROR_SUCCESS;
}

static UINT ITERATE_CostFinalizeConditions(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    LPCWSTR Feature;
    int feature_index;

    Feature = MSI_RecordGetString(row,1);

    feature_index = get_loaded_feature(package,Feature);
    if (feature_index < 0)
        ERR("FAILED to find loaded feature %s\n",debugstr_w(Feature));
    else
    {
        LPCWSTR Condition;
        Condition = MSI_RecordGetString(row,3);

        if (MSI_EvaluateConditionW(package,Condition) == MSICONDITION_TRUE)
        {
            int level = MSI_RecordGetInteger(row,2);
            TRACE("Reseting feature %s to level %i\n", debugstr_w(Feature),
                            level);
            package->features[feature_index].Level = level;
        }
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
    UINT rc;
    MSIQUERY * view;
    DWORD i;
    LPWSTR level;
    DWORD sz = 3;
    WCHAR buffer[3];

    MSI_GetPropertyW(package, szCosting, buffer, &sz);
    if (buffer[0]=='1')
        return ERROR_SUCCESS;

    TRACE("Building Directory properties\n");

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords(view, NULL, ITERATE_CostFinalizeDirectories,
                        package);
        msiobj_release(&view->hdr);
    }

    TRACE("File calculations %i files\n",package->loaded_files);

    for (i = 0; i < package->loaded_files; i++)
    {
        MSICOMPONENT* comp = NULL;
        MSIFILE* file= NULL;

        file = &package->files[i];
        if (file->ComponentIndex >= 0)
            comp = &package->components[file->ComponentIndex];

        if (file->Temporary == TRUE)
            continue;

        if (comp)
        {
            LPWSTR p;

            /* calculate target */
            p = resolve_folder(package, comp->Directory, FALSE, FALSE, NULL);

            HeapFree(GetProcessHeap(),0,file->TargetPath);

            TRACE("file %s is named %s\n",
                   debugstr_w(file->File),debugstr_w(file->FileName));       

            file->TargetPath = build_directory_name(2, p, file->FileName);

            HeapFree(GetProcessHeap(),0,p);

            TRACE("file %s resolves to %s\n",
                   debugstr_w(file->File),debugstr_w(file->TargetPath));       

            if (GetFileAttributesW(file->TargetPath) == INVALID_FILE_ATTRIBUTES)
            {
                file->State = 1;
                comp->Cost += file->FileSize;
            }
            else
            {
                if (file->Version)
                {
                    DWORD handle;
                    DWORD versize;
                    UINT sz;
                    LPVOID version;
                    static const WCHAR name[] = 
                        {'\\',0};
                    static const WCHAR name_fmt[] = 
                        {'%','u','.','%','u','.','%','u','.','%','u',0};
                    WCHAR filever[0x100];
                    VS_FIXEDFILEINFO *lpVer;

                    TRACE("Version comparison.. \n");
                    versize = GetFileVersionInfoSizeW(file->TargetPath,&handle);
                    version = HeapAlloc(GetProcessHeap(),0,versize);
                    GetFileVersionInfoW(file->TargetPath, 0, versize, version);

                    VerQueryValueW(version, (LPWSTR)name, (LPVOID*)&lpVer, &sz);

                    sprintfW(filever,name_fmt,
                        HIWORD(lpVer->dwFileVersionMS),
                        LOWORD(lpVer->dwFileVersionMS),
                        HIWORD(lpVer->dwFileVersionLS),
                        LOWORD(lpVer->dwFileVersionLS));

                    TRACE("new %s old %s\n", debugstr_w(file->Version),
                          debugstr_w(filever));
                    if (strcmpiW(filever,file->Version)<0)
                    {
                        file->State = 2;
                        FIXME("cost should be diff in size\n");
                        comp->Cost += file->FileSize;
                    }
                    else
                        file->State = 3;
                    HeapFree(GetProcessHeap(),0,version);
                }
                else
                    file->State = 3;
            }
        } 
    }

    TRACE("Evaluating Condition Table\n");

    rc = MSI_DatabaseOpenViewW(package->db, ConditionQuery, &view);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_IterateRecords(view, NULL, ITERATE_CostFinalizeConditions,
                    package);
        msiobj_release(&view->hdr);
    }

    TRACE("Enabling or Disabling Components\n");
    for (i = 0; i < package->loaded_components; i++)
    {
        if (package->components[i].Condition[0])
        {
            if (MSI_EvaluateConditionW(package,
                package->components[i].Condition) == MSICONDITION_FALSE)
            {
                TRACE("Disabling component %s\n",
                      debugstr_w(package->components[i].Component));
                package->components[i].Enabled = FALSE;
            }
        }
    }

    MSI_SetPropertyW(package,szCosting,szOne);
    /* set default run level if not set */
    level = load_dynamic_property(package,szlevel,NULL);
    if (!level)
        MSI_SetPropertyW(package,szlevel, szOne);
    else
        HeapFree(GetProcessHeap(),0,level);

    ACTION_UpdateInstallStates(package);

    return SetFeatureStates(package);
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

            data = HeapAlloc(GetProcessHeap(),0,*size);

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
            HeapFree(GetProcessHeap(),0,deformated);

            TRACE("Data %li bytes(%i)\n",*size,count);
        }
        else
        {
            LPWSTR deformated;
            LPWSTR p;
            DWORD d = 0;
            deformat_string(package, &value[1], &deformated);

            *type=REG_DWORD; 
            *size = sizeof(DWORD);
            data = HeapAlloc(GetProcessHeap(),0,*size);
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
            TRACE("DWORD %li\n",*(LPDWORD)data);

            HeapFree(GetProcessHeap(),0,deformated);
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
    INT component_index;
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
    component_index = get_loaded_component(package,component);

    if (!ACTION_VerifyComponentForAction(package, component_index,
                            INSTALLSTATE_LOCAL))
    {
        TRACE("Skipping write due to disabled component %s\n",
                        debugstr_w(component));

        package->components[component_index].Action =
                package->components[component_index].Installed;

        return ERROR_SUCCESS;
    }

    package->components[component_index].Action = INSTALLSTATE_LOCAL;

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
                LPWSTR all_users = load_dynamic_property(package, szALLUSER, NULL);
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
                HeapFree(GetProcessHeap(),0,all_users);
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
    uikey = HeapAlloc(GetProcessHeap(), 0, size*sizeof(WCHAR));
    strcpyW(uikey,szRoot);
    strcatW(uikey,deformated);

    if (RegCreateKeyW( root_key, deformated, &hkey))
    {
        ERR("Could not create key %s\n",debugstr_w(deformated));
        HeapFree(GetProcessHeap(),0,deformated);
        HeapFree(GetProcessHeap(),0,uikey);
        return ERROR_SUCCESS;
    }
    HeapFree(GetProcessHeap(),0,deformated);

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

    HeapFree(GetProcessHeap(),0,value_data);
    HeapFree(GetProcessHeap(),0,deformated);
    HeapFree(GetProcessHeap(),0,uikey);

    return ERROR_SUCCESS;
}

static UINT ACTION_WriteRegistryValues(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','R','e','g','i','s','t','r','y','`',0 };

    if (!package)
        return ERROR_INVALID_HANDLE;

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
    DWORD progress = 0;
    DWORD total = 0;
    static const WCHAR q1[]=
        {'S','E','L','E','C','T',' ','*',' ', 'F','R','O','M',' ',
         '`','R','e','g','i','s','t','r','y','`',0};
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    int i;

    TRACE(" InstallValidate \n");

    rc = MSI_DatabaseOpenViewW(package->db, q1, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }
    while (1)
    {
        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }
        progress +=1;

        msiobj_release(&row->hdr);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);

    total = total + progress * REG_PROGRESS_VALUE;
    total = total + package->loaded_components * COMPONENT_PROGRESS_VALUE;
    for (i=0; i < package->loaded_files; i++)
        total += package->files[i].FileSize;
    ui_progress(package,0,total,0,0);

    for(i = 0; i < package->loaded_features; i++)
    {
        MSIFEATURE* feature = &package->features[i];
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
        HeapFree(GetProcessHeap(),0,deformated);
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

static LPWSTR resolve_keypath( MSIPACKAGE* package, INT
                            component_index)
{
    MSICOMPONENT* cmp = &package->components[component_index];

    if (cmp->KeyPath[0]==0)
    {
        LPWSTR p = resolve_folder(package,cmp->Directory,FALSE,FALSE,NULL);
        return p;
    }
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

        buffer = HeapAlloc(GetProcessHeap(),0, len *sizeof(WCHAR));

        if (deformated_name)
            sprintfW(buffer,fmt2,root,deformated,deformated_name);
        else
            sprintfW(buffer,fmt,root,deformated);

        HeapFree(GetProcessHeap(),0,deformated);
        HeapFree(GetProcessHeap(),0,deformated_name);
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
        int j;
        j = get_loaded_file(package,cmp->KeyPath);

        if (j>=0)
        {
            LPWSTR p = strdupW(package->files[j].TargetPath);
            return p;
        }
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
        RegSetValueExW(hkey,path,0,REG_DWORD,
                    (LPBYTE)&count,sizeof(count));
    else
        RegDeleteValueW(hkey,path);
    RegCloseKey(hkey);
    return count;
}

/*
 * Return TRUE if the count should be written out and FALSE if not
 */
static void ACTION_RefCountComponent( MSIPACKAGE* package, UINT index)
{
    INT count = 0;
    BOOL write = FALSE;
    INT j;

    /* only refcount DLLs */
    if (package->components[index].KeyPath[0]==0 || 
        package->components[index].Attributes & 
            msidbComponentAttributesRegistryKeyPath || 
        package->components[index].Attributes & 
            msidbComponentAttributesODBCDataSource)
        write = FALSE;
    else
    {
        count = ACTION_GetSharedDLLsCount(package->components[index].
                        FullKeypath);
        write = (count > 0);

        if (package->components[index].Attributes & 
                    msidbComponentAttributesSharedDllRefCount)
            write = TRUE;
    }

    /* increment counts */
    for (j = 0; j < package->loaded_features; j++)
    {
        int i;

        if (!ACTION_VerifyFeatureForAction(package,j,INSTALLSTATE_LOCAL))
            continue;

        for (i = 0; i < package->features[j].ComponentCount; i++)
        {
            if (package->features[j].Components[i] == index)
                count++;
        }
    }
    /* decrement counts */
    for (j = 0; j < package->loaded_features; j++)
    {
        int i;
        if (!ACTION_VerifyFeatureForAction(package,j,INSTALLSTATE_ABSENT))
            continue;

        for (i = 0; i < package->features[j].ComponentCount; i++)
        {
            if (package->features[j].Components[i] == index)
                count--;
        }
    }

    /* ref count all the files in the component */
    if (write)
        for (j = 0; j < package->loaded_files; j++)
        {
            if (package->files[j].Temporary)
                continue;
            if (package->files[j].ComponentIndex == index)
                ACTION_WriteSharedDLLsCount(package->files[j].TargetPath,count);
        }
    
    /* add a count for permenent */
    if (package->components[index].Attributes &
                                msidbComponentAttributesPermanent)
        count ++;
    
    package->components[index].RefCount = count;

    if (write)
        ACTION_WriteSharedDLLsCount(package->components[index].FullKeypath,
            package->components[index].RefCount);
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
    DWORD i;
    HKEY hkey=0,hkey2=0;

    if (!package)
        return ERROR_INVALID_HANDLE;

    /* writes the Component and Features values to the registry */

    rc = MSIREG_OpenComponents(&hkey);
    if (rc != ERROR_SUCCESS)
        goto end;
      
    squash_guid(package->ProductCode,squished_pc);
    ui_progress(package,1,COMPONENT_PROGRESS_VALUE,1,0);
    for (i = 0; i < package->loaded_components; i++)
    {
        ui_progress(package,2,0,0,0);
        if (package->components[i].ComponentId[0]!=0)
        {
            WCHAR *keypath = NULL;
            MSIRECORD * uirow;

            squash_guid(package->components[i].ComponentId,squished_cc);
           
            keypath = resolve_keypath(package,i);
            package->components[i].FullKeypath = keypath;

            /* do the refcounting */
            ACTION_RefCountComponent( package, i);

            TRACE("Component %s (%s), Keypath=%s, RefCount=%i\n", 
                            debugstr_w(package->components[i].Component),
                            debugstr_w(squished_cc),
                            debugstr_w(package->components[i].FullKeypath), 
                            package->components[i].RefCount);
            /*
            * Write the keypath out if the component is to be registered
            * and delete the key if the component is to be deregistered
            */
            if (ACTION_VerifyComponentForAction(package, i,
                                    INSTALLSTATE_LOCAL))
            {
                rc = RegCreateKeyW(hkey,squished_cc,&hkey2);
                if (rc != ERROR_SUCCESS)
                    continue;

                if (keypath)
                {
                    RegSetValueExW(hkey2,squished_pc,0,REG_SZ,(LPBYTE)keypath,
                                (strlenW(keypath)+1)*sizeof(WCHAR));

                    if (package->components[i].Attributes & 
                                msidbComponentAttributesPermanent)
                    {
                        static const WCHAR szPermKey[] =
                            { '0','0','0','0','0','0','0','0','0','0','0','0',
                              '0','0','0','0','0','0','0', '0','0','0','0','0',
                              '0','0','0','0','0','0','0','0',0};

                        RegSetValueExW(hkey2,szPermKey,0,REG_SZ,
                                        (LPBYTE)keypath,
                                        (strlenW(keypath)+1)*sizeof(WCHAR));
                    }
                    
                    RegCloseKey(hkey2);
        
                    /* UI stuff */
                    uirow = MSI_CreateRecord(3);
                    MSI_RecordSetStringW(uirow,1,package->ProductCode);
                    MSI_RecordSetStringW(uirow,2,package->components[i].
                                                            ComponentId);
                    MSI_RecordSetStringW(uirow,3,keypath);
                    ui_actiondata(package,szProcessComponents,uirow);
                    msiobj_release( &uirow->hdr );
               }
            }
            else if (ACTION_VerifyComponentForAction(package, i,
                                    INSTALLSTATE_ABSENT))
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
                MSI_RecordSetStringW(uirow,2,package->components[i].
                                ComponentId);
                ui_actiondata(package,szProcessComponents,uirow);
                msiobj_release( &uirow->hdr );
            }
        }
    } 
end:
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

    if ((INT)lpszName == 1)
        tl_struct->path = strdupW(tl_struct->source);
    else
    {
        tl_struct->path = HeapAlloc(GetProcessHeap(),0,sz);
        sprintfW(tl_struct->path,fmt,tl_struct->source, lpszName);
    }

    TRACE("trying %s\n", debugstr_w(tl_struct->path));
    res = LoadTypeLib(tl_struct->path,&tl_struct->ptLib);
    if (!SUCCEEDED(res))
    {
        HeapFree(GetProcessHeap(),0,tl_struct->path);
        tl_struct->path = NULL;

        return TRUE;
    }

    ITypeLib_GetLibAttr(tl_struct->ptLib, &attr);
    if (IsEqualGUID(&(tl_struct->clsid),&(attr->guid)))
    {
        ITypeLib_ReleaseTLibAttr(tl_struct->ptLib, attr);
        return FALSE;
    }

    HeapFree(GetProcessHeap(),0,tl_struct->path);
    tl_struct->path = NULL;

    ITypeLib_ReleaseTLibAttr(tl_struct->ptLib, attr);
    ITypeLib_Release(tl_struct->ptLib);

    return TRUE;
}

static UINT ITERATE_RegisterTypeLibraries(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE* package = (MSIPACKAGE*)param;
    LPCWSTR component;
    INT index;
    typelib_struct tl_struct;
    HMODULE module;
    static const WCHAR szTYPELIB[] = {'T','Y','P','E','L','I','B',0};

    component = MSI_RecordGetString(row,3);
    index = get_loaded_component(package,component);
    if (index < 0)
        return ERROR_SUCCESS;

    if (!ACTION_VerifyComponentForAction(package, index, INSTALLSTATE_LOCAL))
    {
        TRACE("Skipping typelib reg due to disabled component\n");

        package->components[index].Action =
            package->components[index].Installed;

        return ERROR_SUCCESS;
    }

    package->components[index].Action = INSTALLSTATE_LOCAL;

    index = get_loaded_file(package,package->components[index].KeyPath); 

    if (index < 0)
        return ERROR_SUCCESS;

    module = LoadLibraryExW(package->files[index].TargetPath, NULL,
                    LOAD_LIBRARY_AS_DATAFILE);
    if (module != NULL)
    {
        LPWSTR guid;
        guid = load_dynamic_stringW(row,1);
        CLSIDFromString(guid, &tl_struct.clsid);
        HeapFree(GetProcessHeap(),0,guid);
        tl_struct.source = strdupW(package->files[index].TargetPath);
        tl_struct.path = NULL;

        EnumResourceNamesW(module, szTYPELIB, Typelib_EnumResNameProc,
                        (LONG_PTR)&tl_struct);

        if (tl_struct.path != NULL)
        {
            LPWSTR help = NULL;
            LPCWSTR helpid;
            HRESULT res;

            helpid = MSI_RecordGetString(row,6);

            if (helpid)
                help = resolve_folder(package,helpid,FALSE,FALSE,NULL);
            res = RegisterTypeLib(tl_struct.ptLib,tl_struct.path,help);
            HeapFree(GetProcessHeap(),0,help);

            if (!SUCCEEDED(res))
                ERR("Failed to register type library %s\n",
                        debugstr_w(tl_struct.path));
            else
            {
                ui_actiondata(package,szRegisterTypeLibraries,row);

                TRACE("Registered %s\n", debugstr_w(tl_struct.path));
            }

            ITypeLib_Release(tl_struct.ptLib);
            HeapFree(GetProcessHeap(),0,tl_struct.path);
        }
        else
            ERR("Failed to load type library %s\n",
                    debugstr_w(tl_struct.source));

        FreeLibrary(module);
        HeapFree(GetProcessHeap(),0,tl_struct.source);
    }
    else
        ERR("Could not load file! %s\n",
                debugstr_w(package->files[index].TargetPath));

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

    if (!package)
        return ERROR_INVALID_HANDLE;

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
    LPWSTR target_file, target_folder;
    LPCWSTR buffer;
    WCHAR filename[0x100];
    DWORD sz;
    DWORD index;
    static const WCHAR szlnk[]={'.','l','n','k',0};
    IShellLinkW *sl;
    IPersistFile *pf;
    HRESULT res;

    buffer = MSI_RecordGetString(row,4);
    index = get_loaded_component(package,buffer);

    if (index < 0)
        return ERROR_SUCCESS;

    if (!ACTION_VerifyComponentForAction(package, index, INSTALLSTATE_LOCAL))
    {
        TRACE("Skipping shortcut creation due to disabled component\n");

        package->components[index].Action =
                package->components[index].Installed;

        return ERROR_SUCCESS;
    }

    package->components[index].Action = INSTALLSTATE_LOCAL;

    ui_actiondata(package,szCreateShortcuts,row);

    res = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IShellLinkW, (LPVOID *) &sl );

    if (FAILED(res))
    {
        ERR("Is IID_IShellLink\n");
        return ERROR_SUCCESS;
    }

    res = IShellLinkW_QueryInterface( sl, &IID_IPersistFile,(LPVOID*) &pf );
    if( FAILED( res ) )
    {
        ERR("Is IID_IPersistFile\n");
        return ERROR_SUCCESS;
    }

    buffer = MSI_RecordGetString(row,2);
    target_folder = resolve_folder(package, buffer,FALSE,FALSE,NULL);

    /* may be needed because of a bug somehwere else */
    create_full_pathW(target_folder);

    sz = 0x100;
    MSI_RecordGetStringW(row,3,filename,&sz);
    reduce_to_longfilename(filename);
    if (!strchrW(filename,'.') || strcmpiW(strchrW(filename,'.'),szlnk))
        strcatW(filename,szlnk);
    target_file = build_directory_name(2, target_folder, filename);
    HeapFree(GetProcessHeap(),0,target_folder);

    buffer = MSI_RecordGetString(row,5);
    if (strchrW(buffer,'['))
    {
        LPWSTR deformated;
        deformat_string(package,buffer,&deformated);
        IShellLinkW_SetPath(sl,deformated);
        HeapFree(GetProcessHeap(),0,deformated);
    }
    else
    {
        LPWSTR keypath;
        FIXME("poorly handled shortcut format, advertised shortcut\n");
        keypath = strdupW(package->components[index].FullKeypath);
        IShellLinkW_SetPath(sl,keypath);
        HeapFree(GetProcessHeap(),0,keypath);
    }

    if (!MSI_RecordIsNull(row,6))
    {
        LPWSTR deformated;
        buffer = MSI_RecordGetString(row,6);
        deformat_string(package,buffer,&deformated);
        IShellLinkW_SetArguments(sl,deformated);
        HeapFree(GetProcessHeap(),0,deformated);
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
        WCHAR *Path = NULL;
        INT index; 

        buffer = MSI_RecordGetString(row,9);

        build_icon_path(package,buffer,&Path);
        index = MSI_RecordGetInteger(row,10);

        IShellLinkW_SetIconLocation(sl,Path,index);
        HeapFree(GetProcessHeap(),0,Path);
    }

    if (!MSI_RecordIsNull(row,11))
        IShellLinkW_SetShowCmd(sl,MSI_RecordGetInteger(row,11));

    if (!MSI_RecordIsNull(row,12))
    {
        LPWSTR Path;
        buffer = MSI_RecordGetString(row,12);
        Path = resolve_folder(package, buffer, FALSE, FALSE, NULL);
        IShellLinkW_SetWorkingDirectory(sl,Path);
        HeapFree(GetProcessHeap(), 0, Path);
    }

    TRACE("Writing shortcut to %s\n",debugstr_w(target_file));
    IPersistFile_Save(pf,target_file,FALSE);

    HeapFree(GetProcessHeap(),0,target_file);    

    IPersistFile_Release( pf );
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

    if (!package)
        return ERROR_INVALID_HANDLE;

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
    LPWSTR FilePath=NULL;
    LPCWSTR FileName=NULL;
    CHAR buffer[1024];
    DWORD sz;
    UINT rc;

    FileName = MSI_RecordGetString(row,1);
    if (!FileName)
    {
        ERR("Unable to get FileName\n");
        return ERROR_SUCCESS;
    }

    build_icon_path(package,FileName,&FilePath);

    TRACE("Creating icon file at %s\n",debugstr_w(FilePath));

    the_file = CreateFileW(FilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);

    if (the_file == INVALID_HANDLE_VALUE)
    {
        ERR("Unable to create file %s\n",debugstr_w(FilePath));
        HeapFree(GetProcessHeap(),0,FilePath);
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

    HeapFree(GetProcessHeap(),0,FilePath);

    CloseHandle(the_file);
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

    if (!package)
        return ERROR_INVALID_HANDLE;

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


    buffer = load_dynamic_property(package,INSTALLPROPERTY_PRODUCTNAMEW,NULL);
    size = strlenW(buffer)*sizeof(WCHAR);
    RegSetValueExW(hukey,INSTALLPROPERTY_PRODUCTNAMEW,0,REG_SZ, 
            (LPBYTE)buffer,size);
    HeapFree(GetProcessHeap(),0,buffer);

    buffer = load_dynamic_property(package,szProductLanguage,NULL);
    size = sizeof(DWORD);
    langid = atoiW(buffer);
    RegSetValueExW(hukey,INSTALLPROPERTY_LANGUAGEW,0,REG_DWORD, 
            (LPBYTE)&langid,size);
    HeapFree(GetProcessHeap(),0,buffer);

    buffer = load_dynamic_property(package,szARPProductIcon,NULL);
    if (buffer)
    {
        LPWSTR path;
        build_icon_path(package,buffer,&path);
        size = strlenW(path) * sizeof(WCHAR);
        RegSetValueExW(hukey,INSTALLPROPERTY_PRODUCTICONW,0,REG_SZ,
                (LPBYTE)path,size);
    }
    HeapFree(GetProcessHeap(),0,buffer);

    buffer = load_dynamic_property(package,szProductVersion,NULL);
    if (buffer)
    {
        DWORD verdword = build_version_dword(buffer);
        size = sizeof(DWORD);
        RegSetValueExW(hukey,INSTALLPROPERTY_VERSIONW,0,REG_DWORD, (LPBYTE
                    )&verdword,size);
    }
    HeapFree(GetProcessHeap(),0,buffer);
    
    FIXME("Need to write more keys to the user registry\n");
  
    hDb= alloc_msihandle( &package->db->hdr );
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
            size = strlenW(squashed)*sizeof(WCHAR);
            RegSetValueExW(hukey,INSTALLPROPERTY_PACKAGECODEW,0,REG_SZ,
                    (LPBYTE)squashed, size);
        }
        else
        {
            ERR("Unable to query Revision_Number... \n");
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
    INT component_index,action;
    static const WCHAR szWindowsFolder[] =
          {'W','i','n','d','o','w','s','F','o','l','d','e','r',0};

    component = MSI_RecordGetString(row, 8);
    component_index = get_loaded_component(package,component);

    if (!ACTION_VerifyComponentForAction(package, component_index,
                            INSTALLSTATE_LOCAL))
    {
        TRACE("Skipping ini file due to disabled component %s\n",
                        debugstr_w(component));

        package->components[component_index].Action =
            package->components[component_index].Installed;

        return ERROR_SUCCESS;
    }

    package->components[component_index].Action = INSTALLSTATE_LOCAL;

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
            folder = load_dynamic_property(package,dirproperty,NULL);
    }
    else
        folder = load_dynamic_property(package, szWindowsFolder, NULL);

    if (!folder)
    {
        ERR("Unable to resolve folder! (%s)\n",debugstr_w(dirproperty));
        goto cleanup;
    }

    fullname = build_directory_name(3, folder, filename, NULL);

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
    HeapFree(GetProcessHeap(),0,fullname);
    HeapFree(GetProcessHeap(),0,folder);
    HeapFree(GetProcessHeap(),0,deformated_key);
    HeapFree(GetProcessHeap(),0,deformated_value);
    HeapFree(GetProcessHeap(),0,deformated_section);
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
    INT index;
    DWORD len;
    static const WCHAR ExeStr[] =
        {'r','e','g','s','v','r','3','2','.','e','x','e',' ','\"',0};
    static const WCHAR close[] =  {'\"',0};
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL brc;

    memset(&si,0,sizeof(STARTUPINFOW));

    filename = MSI_RecordGetString(row,1);
    index = get_loaded_file(package,filename);

    if (index < 0)
    {
        ERR("Unable to find file id %s\n",debugstr_w(filename));
        return ERROR_SUCCESS;
    }

    len = strlenW(ExeStr);
    len += strlenW(package->files[index].TargetPath);
    len +=2;

    FullName = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
    strcpyW(FullName,ExeStr);
    strcatW(FullName,package->files[index].TargetPath);
    strcatW(FullName,close);

    TRACE("Registering %s\n",debugstr_w(FullName));
    brc = CreateProcessW(NULL, FullName, NULL, NULL, FALSE, 0, NULL, c_colon,
                    &si, &info);

    if (brc)
        msi_dialog_check_messages(info.hProcess);

    HeapFree(GetProcessHeap(),0,FullName);
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
    UINT rc;
    DWORD i;
    HKEY hkey=0;
    HKEY hukey=0;
    
    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MSIREG_OpenFeaturesKey(package->ProductCode,&hkey,TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;

    rc = MSIREG_OpenUserFeaturesKey(package->ProductCode,&hukey,TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;

    /* here the guids are base 85 encoded */
    for (i = 0; i < package->loaded_features; i++)
    {
        LPWSTR data = NULL;
        GUID clsid;
        int j;
        INT size;
        BOOL absent = FALSE;

        if (!ACTION_VerifyFeatureForAction(package,i,INSTALLSTATE_LOCAL) &&
            !ACTION_VerifyFeatureForAction(package,i,INSTALLSTATE_SOURCE) &&
            !ACTION_VerifyFeatureForAction(package,i,INSTALLSTATE_ADVERTISED))
            absent = TRUE;

        size = package->features[i].ComponentCount*21;
        size +=1;
        if (package->features[i].Feature_Parent[0])
            size += strlenW(package->features[i].Feature_Parent)+2;

        data = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));

        data[0] = 0;
        for (j = 0; j < package->features[i].ComponentCount; j++)
        {
            WCHAR buf[21];
            memset(buf,0,sizeof(buf));
            if (package->components
                [package->features[i].Components[j]].ComponentId[0]!=0)
            {
                TRACE("From %s\n",debugstr_w(package->components
                            [package->features[i].Components[j]].ComponentId));
                CLSIDFromString(package->components
                            [package->features[i].Components[j]].ComponentId,
                            &clsid);
                encode_base85_guid(&clsid,buf);
                TRACE("to %s\n",debugstr_w(buf));
                strcatW(data,buf);
            }
        }
        if (package->features[i].Feature_Parent[0])
        {
            static const WCHAR sep[] = {'\2',0};
            strcatW(data,sep);
            strcatW(data,package->features[i].Feature_Parent);
        }

        size = (strlenW(data)+1)*sizeof(WCHAR);
        RegSetValueExW(hkey,package->features[i].Feature,0,REG_SZ,
                       (LPBYTE)data,size);
        HeapFree(GetProcessHeap(),0,data);

        if (!absent)
        {
            size = strlenW(package->features[i].Feature_Parent)*sizeof(WCHAR);
            RegSetValueExW(hukey,package->features[i].Feature,0,REG_SZ,
                       (LPBYTE)package->features[i].Feature_Parent,size);
        }
        else
        {
            size = (strlenW(package->features[i].Feature_Parent)+2)*
                    sizeof(WCHAR);
            data = HeapAlloc(GetProcessHeap(),0,size);
            data[0] = 0x6;
            strcpyW(&data[1],package->features[i].Feature_Parent);
            RegSetValueExW(hukey,package->features[i].Feature,0,REG_SZ,
                       (LPBYTE)data,size);
            HeapFree(GetProcessHeap(),0,data);
        }
    }

end:
    RegCloseKey(hkey);
    RegCloseKey(hukey);
    return rc;
}

static UINT ACTION_RegisterProduct(MSIPACKAGE *package)
{
    HKEY hkey=0;
    LPWSTR buffer = NULL;
    UINT rc,i;
    DWORD size;
    static WCHAR szNONE[] = {0};
    static const WCHAR szWindowsInstaler[] = 
    {'W','i','n','d','o','w','s','I','n','s','t','a','l','l','e','r',0};
    static const WCHAR szPropKeys[][80] = 
    {
{'A','R','P','A','U','T','H','O','R','I','Z','E','D','C','D','F','P','R','E','F','I','X',0},
{'A','R','P','C','O','N','T','A','C','T',0},
{'A','R','P','C','O','M','M','E','N','T','S',0},
{'P','r','o','d','u','c','t','N','a','m','e',0},
{'P','r','o','d','u','c','t','V','e','r','s','i','o','n',0},
{'A','R','P','H','E','L','P','L','I','N','K',0},
{'A','R','P','H','E','L','P','T','E','L','E','P','H','O','N','E',0},
{'A','R','P','I','N','S','T','A','L','L','L','O','C','A','T','I','O','N',0},
{'S','o','u','r','c','e','D','i','r',0},
{'M','a','n','u','f','a','c','t','u','r','e','r',0},
{'A','R','P','R','E','A','D','M','E',0},
{'A','R','P','S','I','Z','E',0},
{'A','R','P','U','R','L','I','N','F','O','A','B','O','U','T',0},
{'A','R','P','U','R','L','U','P','D','A','T','E','I','N','F','O',0},
{0},
    };

    static const WCHAR szRegKeys[][80] = 
    {
{'A','u','t','h','o','r','i','z','e','d','C','D','F','P','r','e','f','i','x',0},
{'C','o','n','t','a','c','t',0},
{'C','o','m','m','e','n','t','s',0},
{'D','i','s','p','l','a','y','N','a','m','e',0},
{'D','i','s','p','l','a','y','V','e','r','s','i','o','n',0},
{'H','e','l','p','L','i','n','k',0},
{'H','e','l','p','T','e','l','e','p','h','o','n','e',0},
{'I','n','s','t','a','l','l','L','o','c','a','t','i','o','n',0},
{'I','n','s','t','a','l','l','S','o','u','r','c','e',0},
{'P','u','b','l','i','s','h','e','r',0},
{'R','e','a','d','m','e',0},
{'S','i','z','e',0},
{'U','R','L','I','n','f','o','A','b','o','u','t',0},
{'U','R','L','U','p','d','a','t','e','I','n','f','o',0},
{0},
    };

    static const WCHAR installerPathFmt[] = {
    '%','s','\\',
    'I','n','s','t','a','l','l','e','r','\\',0};
    static const WCHAR fmt[] = {
    '%','s','\\',
    'I','n','s','t','a','l','l','e','r','\\',
    '%','x','.','m','s','i',0};
    static const WCHAR szUpgradeCode[] = 
        {'U','p','g','r','a','d','e','C','o','d','e',0};
    static const WCHAR modpath_fmt[] = 
        {'M','s','i','E','x','e','c','.','e','x','e',' ','/','I','[','P','r','o','d','u','c','t','C','o','d','e',']',0};
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
    WCHAR windir[MAX_PATH], path[MAX_PATH], packagefile[MAX_PATH];
    INT num,start;

    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MSIREG_OpenUninstallKey(package->ProductCode,&hkey,TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;

    /* dump all the info i can grab */
    FIXME("Flesh out more information \n");

    i = 0;
    while (szPropKeys[i][0]!=0)
    {
        buffer = load_dynamic_property(package,szPropKeys[i],&rc);
        if (rc != ERROR_SUCCESS)
            buffer = szNONE;
        size = strlenW(buffer)*sizeof(WCHAR);
        RegSetValueExW(hkey,szRegKeys[i],0,REG_SZ,(LPBYTE)buffer,size);
        HeapFree(GetProcessHeap(),0,buffer);
        i++;
    }

    rc = 0x1;
    size = sizeof(rc);
    RegSetValueExW(hkey,szWindowsInstaler,0,REG_DWORD,(LPBYTE)&rc,size);
    
    /* copy the package locally */
    num = GetTickCount() & 0xffff;
    if (!num) 
        num = 1;
    start = num;
    GetWindowsDirectoryW(windir, sizeof(windir) / sizeof(windir[0]));
    snprintfW(packagefile,sizeof(packagefile)/sizeof(packagefile[0]),fmt,
     windir,num);
    do 
    {
        HANDLE handle = CreateFileW(packagefile,GENERIC_WRITE, 0, NULL,
                                  CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0 );
        if (handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle);
            break;
        }
        if (GetLastError() != ERROR_FILE_EXISTS &&
            GetLastError() != ERROR_SHARING_VIOLATION)
            break;
        if (!(++num & 0xffff)) num = 1;
        sprintfW(packagefile,fmt,num);
    } while (num != start);

    snprintfW(path,sizeof(path)/sizeof(path[0]),installerPathFmt,windir);
    create_full_pathW(path);
    TRACE("Copying to local package %s\n",debugstr_w(packagefile));
    if (!CopyFileW(package->msiFilePath,packagefile,FALSE))
        ERR("Unable to copy package (%s -> %s) (error %ld)\n",
            debugstr_w(package->msiFilePath), debugstr_w(packagefile),
            GetLastError());
    size = strlenW(packagefile)*sizeof(WCHAR);
    RegSetValueExW(hkey,INSTALLPROPERTY_LOCALPACKAGEW,0,REG_SZ,
            (LPBYTE)packagefile,size);

    /* do ModifyPath and UninstallString */
    size = deformat_string(package,modpath_fmt,&buffer);
    RegSetValueExW(hkey,szModifyPath,0,REG_EXPAND_SZ,(LPBYTE)buffer,size);
    RegSetValueExW(hkey,szUninstallString,0,REG_EXPAND_SZ,(LPBYTE)buffer,size);
    HeapFree(GetProcessHeap(),0,buffer);

    FIXME("Write real Estimated Size when we have it\n");
    size = 0;
    RegSetValueExW(hkey,szEstimatedSize,0,REG_DWORD,(LPBYTE)&size,sizeof(DWORD));
   
    GetLocalTime(&systime);
    size = 9*sizeof(WCHAR);
    buffer= HeapAlloc(GetProcessHeap(),0,size);
    sprintfW(buffer,date_fmt,systime.wYear,systime.wMonth,systime.wDay);
    size = strlenW(buffer)*sizeof(WCHAR);
    RegSetValueExW(hkey,INSTALLPROPERTY_INSTALLDATEW,0,REG_SZ,
            (LPBYTE)buffer,size);
    HeapFree(GetProcessHeap(),0,buffer);
   
    buffer = load_dynamic_property(package,szProductLanguage,NULL);
    size = atoiW(buffer);
    RegSetValueExW(hkey,INSTALLPROPERTY_LANGUAGEW,0,REG_DWORD,
            (LPBYTE)&size,sizeof(DWORD));
    HeapFree(GetProcessHeap(),1,buffer);

    buffer = load_dynamic_property(package,szProductVersion,NULL);
    if (buffer)
    {
        DWORD verdword = build_version_dword(buffer);
        DWORD vermajor = verdword>>24;
        DWORD verminor = (verdword>>16)&0x00FF;
        size = sizeof(DWORD);
        RegSetValueExW(hkey,INSTALLPROPERTY_VERSIONW,0,REG_DWORD,
                (LPBYTE)&verdword,size);
        RegSetValueExW(hkey,INSTALLPROPERTY_VERSIONMAJORW,0,REG_DWORD,
                (LPBYTE)&vermajor,size);
        RegSetValueExW(hkey,INSTALLPROPERTY_VERSIONMINORW,0,REG_DWORD,
                (LPBYTE)&verminor,size);
    }
    HeapFree(GetProcessHeap(),0,buffer);
    
    /* Handle Upgrade Codes */
    upgrade_code = load_dynamic_property(package,szUpgradeCode, NULL);
    if (upgrade_code)
    {
        HKEY hkey2;
        WCHAR squashed[33];
        MSIREG_OpenUpgradeCodesKey(upgrade_code, &hkey2, TRUE);
        squash_guid(package->ProductCode,squashed);
        RegSetValueExW(hkey2, squashed, 0,REG_SZ,NULL,0);
        RegCloseKey(hkey2);
        MSIREG_OpenUserUpgradeCodesKey(upgrade_code, &hkey2, TRUE);
        squash_guid(package->ProductCode,squashed);
        RegSetValueExW(hkey2, squashed, 0,REG_SZ,NULL,0);
        RegCloseKey(hkey2);

        HeapFree(GetProcessHeap(),0,upgrade_code);
    }
    
end:
    RegCloseKey(hkey);

    return ERROR_SUCCESS;
}

static UINT ACTION_InstallExecute(MSIPACKAGE *package)
{
    UINT rc;

    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = execute_script(package,INSTALL_SCRIPT);

    return rc;
}

static UINT ACTION_InstallFinalize(MSIPACKAGE *package)
{
    UINT rc;

    if (!package)
        return ERROR_INVALID_HANDLE;

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
    WCHAR  squished_pc[100];
    DWORD size;

    if (!package)
        return ERROR_INVALID_HANDLE;

    squash_guid(package->ProductCode,squished_pc);

    GetSystemDirectoryW(sysdir, sizeof(sysdir)/sizeof(sysdir[0]));
    RegCreateKeyW(HKEY_LOCAL_MACHINE,RunOnce,&hkey);
    snprintfW(buffer,sizeof(buffer)/sizeof(buffer[0]),msiexec_fmt,sysdir,
     squished_pc);

    size = strlenW(buffer)*sizeof(WCHAR);
    RegSetValueExW(hkey,squished_pc,0,REG_SZ,(LPBYTE)buffer,size);
    RegCloseKey(hkey);

    TRACE("Reboot command %s\n",debugstr_w(buffer));

    RegCreateKeyW(HKEY_LOCAL_MACHINE,InstallRunOnce,&hkey);
    sprintfW(buffer,install_fmt,package->ProductCode,squished_pc);

    size = strlenW(buffer)*sizeof(WCHAR);
    RegSetValueExW(hkey,squished_pc,0,REG_SZ,(LPBYTE)buffer,size);
    RegCloseKey(hkey);

    return ERROR_INSTALL_SUSPEND;
}

UINT ACTION_ResolveSource(MSIPACKAGE* package)
{
    /*
     * we are currently doing what should be done here in the top level Install
     * however for Adminastrative and uninstalls this step will be needed
     */
    return ERROR_SUCCESS;
}

static UINT ACTION_RegisterUser(MSIPACKAGE *package)
{
    HKEY hkey=0;
    LPWSTR buffer;
    LPWSTR productid;
    UINT rc,i;
    DWORD size;

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

    if (!package)
        return ERROR_INVALID_HANDLE;

    productid = load_dynamic_property(package,INSTALLPROPERTY_PRODUCTIDW,
            &rc);
    if (!productid)
        return ERROR_SUCCESS;

    rc = MSIREG_OpenUninstallKey(package->ProductCode,&hkey,TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;

    i = 0;
    while (szPropKeys[i][0]!=0)
    {
        buffer = load_dynamic_property(package,szPropKeys[i],&rc);
        if (rc == ERROR_SUCCESS)
        {
            size = strlenW(buffer)*sizeof(WCHAR);
            RegSetValueExW(hkey,szRegKeys[i],0,REG_SZ,(LPBYTE)buffer,size);
        }
        else
            RegSetValueExW(hkey,szRegKeys[i],0,REG_SZ,NULL,0);
        i++;
    }

end:
    HeapFree(GetProcessHeap(),0,productid);
    RegCloseKey(hkey);

    return ERROR_SUCCESS;
}


static UINT ACTION_ExecuteAction(MSIPACKAGE *package)
{
    static const WCHAR szUILevel[] = {'U','I','L','e','v','e','l',0};
    static const WCHAR szTwo[] = {'2',0};
    UINT rc;
    LPWSTR level;
    level = load_dynamic_property(package,szUILevel,NULL);

    MSI_SetPropertyW(package,szUILevel,szTwo);
    package->script->InWhatSequence |= SEQUENCE_EXEC;
    rc = ACTION_ProcessExecSequence(package,FALSE);
    MSI_SetPropertyW(package,szUILevel,level);
    HeapFree(GetProcessHeap(),0,level);
    return rc;
}


/*
 * Code based off of code located here
 * http://www.codeproject.com/gdi/fontnamefromfile.asp
 *
 * Using string index 4 (full font name) instead of 1 (family name)
 */
static LPWSTR load_ttfname_from(LPCWSTR filename)
{
    HANDLE handle;
    LPWSTR ret = NULL;
    int i;

    typedef struct _tagTT_OFFSET_TABLE{
        USHORT uMajorVersion;
        USHORT uMinorVersion;
        USHORT uNumOfTables;
        USHORT uSearchRange;
        USHORT uEntrySelector;
        USHORT uRangeShift;
    }TT_OFFSET_TABLE;

    typedef struct _tagTT_TABLE_DIRECTORY{
        char szTag[4]; /* table name */
        ULONG uCheckSum; /* Check sum */
        ULONG uOffset; /* Offset from beginning of file */
        ULONG uLength; /* length of the table in bytes */
    }TT_TABLE_DIRECTORY;

    typedef struct _tagTT_NAME_TABLE_HEADER{
    USHORT uFSelector; /* format selector. Always 0 */
    USHORT uNRCount; /* Name Records count */
    USHORT uStorageOffset; /* Offset for strings storage, 
                            * from start of the table */
    }TT_NAME_TABLE_HEADER;
   
    typedef struct _tagTT_NAME_RECORD{
        USHORT uPlatformID;
        USHORT uEncodingID;
        USHORT uLanguageID;
        USHORT uNameID;
        USHORT uStringLength;
        USHORT uStringOffset; /* from start of storage area */
    }TT_NAME_RECORD;

#define SWAPWORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x) MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))

    handle = CreateFileW(filename ,GENERIC_READ, 0, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, 0 );
    if (handle != INVALID_HANDLE_VALUE)
    {
        TT_TABLE_DIRECTORY tblDir;
        BOOL bFound = FALSE;
        TT_OFFSET_TABLE ttOffsetTable;

        ReadFile(handle,&ttOffsetTable, sizeof(TT_OFFSET_TABLE),NULL,NULL);
        ttOffsetTable.uNumOfTables = SWAPWORD(ttOffsetTable.uNumOfTables);
        ttOffsetTable.uMajorVersion = SWAPWORD(ttOffsetTable.uMajorVersion);
        ttOffsetTable.uMinorVersion = SWAPWORD(ttOffsetTable.uMinorVersion);
        
        if (ttOffsetTable.uMajorVersion != 1 || 
                        ttOffsetTable.uMinorVersion != 0)
            return NULL;

        for (i=0; i< ttOffsetTable.uNumOfTables; i++)
        {
            ReadFile(handle,&tblDir, sizeof(TT_TABLE_DIRECTORY),NULL,NULL);
            if (strncmp(tblDir.szTag,"name",4)==0)
            {
                bFound = TRUE;
                tblDir.uLength = SWAPLONG(tblDir.uLength);
                tblDir.uOffset = SWAPLONG(tblDir.uOffset);
                break;
            }
        }

        if (bFound)
        {
            TT_NAME_TABLE_HEADER ttNTHeader;
            TT_NAME_RECORD ttRecord;

            SetFilePointer(handle, tblDir.uOffset, NULL, FILE_BEGIN);
            ReadFile(handle,&ttNTHeader, sizeof(TT_NAME_TABLE_HEADER),
                            NULL,NULL);

            ttNTHeader.uNRCount = SWAPWORD(ttNTHeader.uNRCount);
            ttNTHeader.uStorageOffset = SWAPWORD(ttNTHeader.uStorageOffset);
            bFound = FALSE;
            for(i=0; i<ttNTHeader.uNRCount; i++)
            {
                ReadFile(handle,&ttRecord, sizeof(TT_NAME_RECORD),NULL,NULL);
                ttRecord.uNameID = SWAPWORD(ttRecord.uNameID);
                /* 4 is the Full Font Name */
                if(ttRecord.uNameID == 4)
                {
                    int nPos;
                    LPSTR buf;
                    static LPCSTR tt = " (TrueType)";

                    ttRecord.uStringLength = SWAPWORD(ttRecord.uStringLength);
                    ttRecord.uStringOffset = SWAPWORD(ttRecord.uStringOffset);
                    nPos = SetFilePointer(handle, 0, NULL, FILE_CURRENT);
                    SetFilePointer(handle, tblDir.uOffset + 
                                    ttRecord.uStringOffset + 
                                    ttNTHeader.uStorageOffset,
                                    NULL, FILE_BEGIN);
                    buf = HeapAlloc(GetProcessHeap(), 0, 
                                    ttRecord.uStringLength + 1 + strlen(tt));
                    memset(buf, 0, ttRecord.uStringLength + 1 + strlen(tt));
                    ReadFile(handle, buf, ttRecord.uStringLength, NULL, NULL);
                    if (strlen(buf) > 0)
                    {
                        strcat(buf,tt);
                        ret = strdupAtoW(buf);
                        HeapFree(GetProcessHeap(),0,buf);
                        break;
                    }

                    HeapFree(GetProcessHeap(),0,buf);
                    SetFilePointer(handle,nPos, NULL, FILE_BEGIN);
                }
            }
        }
        CloseHandle(handle);
    }
    else
        ERR("Unable to open font file %s\n", debugstr_w(filename));

    TRACE("Returning fontname %s\n",debugstr_w(ret));
    return ret;
}

static UINT ITERATE_RegisterFonts(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    LPWSTR name;
    LPCWSTR file;
    UINT index;
    DWORD size;
    static const WCHAR regfont1[] =
        {'S','o','f','t','w','a','r','e','\\',
         'M','i','c','r','o','s','o','f','t','\\',
         'W','i','n','d','o','w','s',' ','N','T','\\',
         'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
         'F','o','n','t','s',0};
    static const WCHAR regfont2[] =
        {'S','o','f','t','w','a','r','e','\\',
         'M','i','c','r','o','s','o','f','t','\\',
         'W','i','n','d','o','w','s','\\',
         'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
         'F','o','n','t','s',0};
    HKEY hkey1;
    HKEY hkey2;

    file = MSI_RecordGetString(row,1);
    index = get_loaded_file(package,file);
    if (index < 0)
    {
        ERR("Unable to load file\n");
        return ERROR_SUCCESS;
    }

    /* check to make sure that component is installed */
    if (!ACTION_VerifyComponentForAction(package, 
                package->files[index].ComponentIndex, INSTALLSTATE_LOCAL))
    {
        TRACE("Skipping: Component not scheduled for install\n");
        return ERROR_SUCCESS;
    }

    RegCreateKeyW(HKEY_LOCAL_MACHINE,regfont1,&hkey1);
    RegCreateKeyW(HKEY_LOCAL_MACHINE,regfont2,&hkey2);

    if (MSI_RecordIsNull(row,2))
        name = load_ttfname_from(package->files[index].TargetPath);
    else
        name = load_dynamic_stringW(row,2);

    if (name)
    {
        size = strlenW(package->files[index].FileName) * sizeof(WCHAR);
        RegSetValueExW(hkey1,name,0,REG_SZ,
                    (LPBYTE)package->files[index].FileName,size);
        RegSetValueExW(hkey2,name,0,REG_SZ,
                    (LPBYTE)package->files[index].FileName,size);
    }

    HeapFree(GetProcessHeap(),0,name);
    RegCloseKey(hkey1);
    RegCloseKey(hkey2);
    return ERROR_SUCCESS;
}

static UINT ACTION_RegisterFonts(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','F','o','n','t','`',0};

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
    {
        TRACE("MSI_DatabaseOpenViewW failed: %d\n", rc);
        return ERROR_SUCCESS;
    }

    MSI_IterateRecords(view, NULL, ITERATE_RegisterFonts, package);
    msiobj_release(&view->hdr);

    return ERROR_SUCCESS;
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
    UINT index;
    DWORD sz = 0;

    component = MSI_RecordGetString(rec,3);
    index = get_loaded_component(package,component);

    if (!ACTION_VerifyComponentForAction(package, index,
                            INSTALLSTATE_LOCAL) && 
       !ACTION_VerifyComponentForAction(package, index,
                            INSTALLSTATE_SOURCE) &&
       !ACTION_VerifyComponentForAction(package, index,
                            INSTALLSTATE_ADVERTISED))
    {
        TRACE("Skipping: Component %s not scheduled for install\n",
                        debugstr_w(component));

        return ERROR_SUCCESS;
    }

    compgroupid = MSI_RecordGetString(rec,1);

    rc = MSIREG_OpenUserComponentsKey(compgroupid, &hkey, TRUE);
    if (rc != ERROR_SUCCESS)
        goto end;
    
    text = MSI_RecordGetString(rec,4);
    qualifier = MSI_RecordGetString(rec,2);
    feature = MSI_RecordGetString(rec,5);
  
    advertise = create_component_advertise_string(package, 
                    &package->components[index], feature);

    sz = strlenW(advertise);

    if (text)
        sz += lstrlenW(text);

    sz+=3;
    sz *= sizeof(WCHAR);
           
    output = HeapAlloc(GetProcessHeap(),0,sz);
    memset(output,0,sz);
    strcpyW(output,advertise);

    if (text)
        strcatW(output,text);

    sz = (lstrlenW(output)+2) * sizeof(WCHAR);
    RegSetValueExW(hkey, qualifier,0,REG_MULTI_SZ, (LPBYTE)output, sz);
    
end:
    RegCloseKey(hkey);
    HeapFree(GetProcessHeap(),0,output);
    
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
