/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
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

/* Msi top level apis directly related to installs */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "msi.h"
#include "msidefs.h"
#include "msipriv.h"
#include "winuser.h"
#include "wine/unicode.h"
#include "action.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/***********************************************************************
 * MsiDoActionA       (MSI.@)
 */
UINT WINAPI MsiDoActionA( MSIHANDLE hInstall, LPCSTR szAction )
{
    LPWSTR szwAction;
    UINT rc;

    TRACE(" exteral attempt at action %s\n",szAction);

    if (!szAction)
        return ERROR_FUNCTION_FAILED;
    if (hInstall == 0)
        return ERROR_FUNCTION_FAILED;

    szwAction = strdupAtoW(szAction);

    if (!szwAction)
        return ERROR_FUNCTION_FAILED; 


    rc = MsiDoActionW(hInstall, szwAction);
    HeapFree(GetProcessHeap(),0,szwAction);
    return rc;
}

/***********************************************************************
 * MsiDoActionW       (MSI.@)
 */
UINT WINAPI MsiDoActionW( MSIHANDLE hInstall, LPCWSTR szAction )
{
    MSIPACKAGE *package;
    UINT ret = ERROR_INVALID_HANDLE;

    TRACE(" external attempt at action %s \n",debugstr_w(szAction));

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if( package )
    {
        ret = ACTION_PerformUIAction(package,szAction);
        msiobj_release( &package->hdr );
    }
    return ret;
}

/***********************************************************************
 * MsiGetTargetPathA        (MSI.@)
 */
UINT WINAPI MsiGetTargetPathA( MSIHANDLE hInstall, LPCSTR szFolder, 
                               LPSTR szPathBuf, DWORD* pcchPathBuf) 
{
    LPWSTR szwFolder;
    LPWSTR szwPathBuf;
    UINT rc;

    TRACE("getting folder %s %p %li\n",szFolder,szPathBuf, *pcchPathBuf);

    if (!szFolder)
        return ERROR_FUNCTION_FAILED;
    if (hInstall == 0)
        return ERROR_FUNCTION_FAILED;

    szwFolder = strdupAtoW(szFolder);

    if (!szwFolder)
        return ERROR_FUNCTION_FAILED; 

    szwPathBuf = HeapAlloc( GetProcessHeap(), 0 , *pcchPathBuf * sizeof(WCHAR));

    rc = MsiGetTargetPathW(hInstall, szwFolder, szwPathBuf,pcchPathBuf);

    WideCharToMultiByte( CP_ACP, 0, szwPathBuf, *pcchPathBuf, szPathBuf,
                         *pcchPathBuf, NULL, NULL );

    HeapFree(GetProcessHeap(),0,szwFolder);
    HeapFree(GetProcessHeap(),0,szwPathBuf);

    return rc;
}

/***********************************************************************
* MsiGetTargetPathW        (MSI.@)
*/
UINT WINAPI MsiGetTargetPathW( MSIHANDLE hInstall, LPCWSTR szFolder, LPWSTR
                                szPathBuf, DWORD* pcchPathBuf) 
{
    LPWSTR path;
    UINT rc = ERROR_FUNCTION_FAILED;
    MSIPACKAGE *package;

    TRACE("(%s %p %li)\n",debugstr_w(szFolder),szPathBuf,*pcchPathBuf);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;
    path = resolve_folder(package, szFolder, FALSE, FALSE, NULL);
    msiobj_release( &package->hdr );

    if (path && (strlenW(path) > *pcchPathBuf))
    {
        *pcchPathBuf = strlenW(path)+1;
        rc = ERROR_MORE_DATA;
    }
    else if (path)
    {
        *pcchPathBuf = strlenW(path)+1;
        strcpyW(szPathBuf,path);
        TRACE("Returning Path %s\n",debugstr_w(path));
        rc = ERROR_SUCCESS;
    }
    HeapFree(GetProcessHeap(),0,path);
    
    return rc;
}


/***********************************************************************
* MsiGetSourcePathA     (MSI.@)
*/
UINT WINAPI MsiGetSourcePathA( MSIHANDLE hInstall, LPCSTR szFolder, 
                               LPSTR szPathBuf, DWORD* pcchPathBuf) 
{
    LPWSTR szwFolder;
    LPWSTR szwPathBuf;
    UINT rc;

    TRACE("getting source %s %p %li\n",szFolder,szPathBuf, *pcchPathBuf);

    if (!szFolder)
        return ERROR_FUNCTION_FAILED;
    if (hInstall == 0)
        return ERROR_FUNCTION_FAILED;

    szwFolder = strdupAtoW(szFolder);
    if (!szwFolder)
        return ERROR_FUNCTION_FAILED; 

    szwPathBuf = HeapAlloc( GetProcessHeap(), 0 , *pcchPathBuf * sizeof(WCHAR));

    rc = MsiGetSourcePathW(hInstall, szwFolder, szwPathBuf,pcchPathBuf);

    WideCharToMultiByte( CP_ACP, 0, szwPathBuf, *pcchPathBuf, szPathBuf,
                         *pcchPathBuf, NULL, NULL );

    HeapFree(GetProcessHeap(),0,szwFolder);
    HeapFree(GetProcessHeap(),0,szwPathBuf);

    return rc;
}

/***********************************************************************
* MsiGetSourcePathW     (MSI.@)
*/
UINT WINAPI MsiGetSourcePathW( MSIHANDLE hInstall, LPCWSTR szFolder, LPWSTR
                                szPathBuf, DWORD* pcchPathBuf) 
{
    LPWSTR path;
    UINT rc = ERROR_FUNCTION_FAILED;
    MSIPACKAGE *package;

    TRACE("(%s %p %li)\n",debugstr_w(szFolder),szPathBuf,*pcchPathBuf);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if( !package )
        return ERROR_INVALID_HANDLE;
    path = resolve_folder(package, szFolder, TRUE, FALSE, NULL);
    msiobj_release( &package->hdr );

    if (path && strlenW(path) > *pcchPathBuf)
    {
        *pcchPathBuf = strlenW(path)+1;
        rc = ERROR_MORE_DATA;
    }
    else if (path)
    {
        *pcchPathBuf = strlenW(path)+1;
        strcpyW(szPathBuf,path);
        TRACE("Returning Path %s\n",debugstr_w(path));
        rc = ERROR_SUCCESS;
    }
    HeapFree(GetProcessHeap(),0,path);
    
    return rc;
}


/***********************************************************************
 * MsiSetTargetPathA  (MSI.@)
 */
UINT WINAPI MsiSetTargetPathA(MSIHANDLE hInstall, LPCSTR szFolder, 
                             LPCSTR szFolderPath)
{
    LPWSTR szwFolder;
    LPWSTR szwFolderPath;
    UINT rc;

    if (!szFolder)
        return ERROR_FUNCTION_FAILED;
    if (hInstall == 0)
        return ERROR_FUNCTION_FAILED;

    szwFolder = strdupAtoW(szFolder);
    if (!szwFolder)
        return ERROR_FUNCTION_FAILED; 

    szwFolderPath = strdupAtoW(szFolderPath);
    if (!szwFolderPath)
    {
        HeapFree(GetProcessHeap(),0,szwFolder);
        return ERROR_FUNCTION_FAILED; 
    }

    rc = MsiSetTargetPathW(hInstall, szwFolder, szwFolderPath);

    HeapFree(GetProcessHeap(),0,szwFolder);
    HeapFree(GetProcessHeap(),0,szwFolderPath);

    return rc;
}

/*
 * Ok my original interpretation of this was wrong. And it looks like msdn has
 * changed a bit also. The given folder path does not have to actually already
 * exist, it just cannot be read only and must be a legal folder path.
 */
UINT MSI_SetTargetPathW(MSIPACKAGE *package, LPCWSTR szFolder, 
                             LPCWSTR szFolderPath)
{
    DWORD i;
    DWORD attrib;
    LPWSTR path = NULL;
    LPWSTR path2 = NULL;
    MSIFOLDER *folder;

    TRACE("(%p %s %s)\n",package, debugstr_w(szFolder),debugstr_w(szFolderPath));

    if (package==NULL)
        return ERROR_INVALID_HANDLE;

    if (szFolderPath[0]==0)
        return ERROR_FUNCTION_FAILED;

    attrib = GetFileAttributesW(szFolderPath);
    if ( attrib != INVALID_FILE_ATTRIBUTES &&
          (!(attrib & FILE_ATTRIBUTE_DIRECTORY) ||
           attrib & FILE_ATTRIBUTE_OFFLINE ||
           attrib & FILE_ATTRIBUTE_READONLY))
        return ERROR_FUNCTION_FAILED;

    path = resolve_folder(package,szFolder,FALSE,FALSE,&folder);

    if (!path)
        return ERROR_INVALID_PARAMETER;

    if (attrib == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryW(szFolderPath,NULL))
            return ERROR_FUNCTION_FAILED;
        RemoveDirectoryW(szFolderPath);
    }

    HeapFree(GetProcessHeap(),0,folder->Property);
    folder->Property = build_directory_name(2, szFolderPath, NULL);

    if (lstrcmpiW(path, folder->Property) == 0)
    {
        /*
         *  Resolved Target has not really changed, so just 
         *  set this folder and do not recalculate everything.
         */
        HeapFree(GetProcessHeap(),0,folder->ResolvedTarget);
        folder->ResolvedTarget = NULL;
        path2 = resolve_folder(package,szFolder,FALSE,TRUE,NULL);
        HeapFree(GetProcessHeap(),0,path2);
    }
    else
    {
        for (i = 0; i < package->loaded_folders; i++)
        {
            HeapFree(GetProcessHeap(),0,package->folders[i].ResolvedTarget);
            package->folders[i].ResolvedTarget=NULL;
        }

        for (i = 0; i < package->loaded_folders; i++)
        {
            path2=resolve_folder(package, package->folders[i].Directory, FALSE,
                       TRUE, NULL);
            HeapFree(GetProcessHeap(),0,path2);
        }
    }
    HeapFree(GetProcessHeap(),0,path);

    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiSetTargetPathW  (MSI.@)
 */
UINT WINAPI MsiSetTargetPathW(MSIHANDLE hInstall, LPCWSTR szFolder, 
                             LPCWSTR szFolderPath)
{
    MSIPACKAGE *package;
    UINT ret;

    TRACE("(%s %s)\n",debugstr_w(szFolder),debugstr_w(szFolderPath));

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    ret = MSI_SetTargetPathW( package, szFolder, szFolderPath );
    msiobj_release( &package->hdr );
    return ret;
}

/***********************************************************************
 *           MsiGetMode    (MSI.@)
 *
 * Returns an internal installer state (if it is running in a mode iRunMode)
 *
 * PARAMS
 *   hInstall    [I]  Handle to the installation
 *   hRunMode    [I]  Checking run mode
 *        MSIRUNMODE_ADMIN             Administrative mode
 *        MSIRUNMODE_ADVERTISE         Advertisement mode
 *        MSIRUNMODE_MAINTENANCE       Maintenance mode
 *        MSIRUNMODE_ROLLBACKENABLED   Rollback is enabled
 *        MSIRUNMODE_LOGENABLED        Log file is writing
 *        MSIRUNMODE_OPERATIONS        Operations in progress??
 *        MSIRUNMODE_REBOOTATEND       We need to reboot after installation completed
 *        MSIRUNMODE_REBOOTNOW         We need to reboot to continue the installation
 *        MSIRUNMODE_CABINET           Files from cabinet are installed
 *        MSIRUNMODE_SOURCESHORTNAMES  Long names in source files is suppressed
 *        MSIRUNMODE_TARGETSHORTNAMES  Long names in destination files is suppressed
 *        MSIRUNMODE_RESERVED11        Reserved
 *        MSIRUNMODE_WINDOWS9X         Running under Windows95/98
 *        MSIRUNMODE_ZAWENABLED        Demand installation is supported
 *        MSIRUNMODE_RESERVED14        Reserved
 *        MSIRUNMODE_RESERVED15        Reserved
 *        MSIRUNMODE_SCHEDULED         called from install script
 *        MSIRUNMODE_ROLLBACK          called from rollback script
 *        MSIRUNMODE_COMMIT            called from commit script
 *
 * RETURNS
 *    In the state: TRUE
 *    Not in the state: FALSE
 *
 */

BOOL WINAPI MsiGetMode(MSIHANDLE hInstall, MSIRUNMODE iRunMode)
{
    FIXME("STUB (iRunMode=%i)\n",iRunMode);
    return TRUE;
}

/***********************************************************************
 * MsiSetFeatureStateA (MSI.@)
 *
 * According to the docs, when this is called it immediately recalculates
 * all the component states as well
 */
UINT WINAPI MsiSetFeatureStateA(MSIHANDLE hInstall, LPCSTR szFeature,
                                INSTALLSTATE iState)
{
    LPWSTR szwFeature = NULL;
    UINT rc;

    szwFeature = strdupAtoW(szFeature);

    if (!szwFeature)
        return ERROR_FUNCTION_FAILED;
   
    rc = MsiSetFeatureStateW(hInstall,szwFeature, iState); 

    HeapFree(GetProcessHeap(),0,szwFeature);

    return rc;
}



UINT WINAPI MSI_SetFeatureStateW(MSIPACKAGE* package, LPCWSTR szFeature,
                                INSTALLSTATE iState)
{
    INT index, i;
    UINT rc = ERROR_SUCCESS;

    TRACE(" %s to %i\n",debugstr_w(szFeature), iState);

    index = get_loaded_feature(package,szFeature);
    if (index < 0)
        return ERROR_UNKNOWN_FEATURE;

    if (iState == INSTALLSTATE_ADVERTISED && 
        package->features[index].Attributes & 
            msidbFeatureAttributesDisallowAdvertise)
        return ERROR_FUNCTION_FAILED;

    package->features[index].ActionRequest= iState;
    package->features[index].Action= iState;

    ACTION_UpdateComponentStates(package,szFeature);

    /* update all the features that are children of this feature */
    for (i = 0; i < package->loaded_features; i++)
    {
        if (strcmpW(szFeature, package->features[i].Feature_Parent) == 0)
            MSI_SetFeatureStateW(package, package->features[i].Feature, iState);
    }
    
    return rc;
}

/***********************************************************************
 * MsiSetFeatureStateW (MSI.@)
 */
UINT WINAPI MsiSetFeatureStateW(MSIHANDLE hInstall, LPCWSTR szFeature,
                                INSTALLSTATE iState)
{
    MSIPACKAGE* package;
    UINT rc = ERROR_SUCCESS;

    TRACE(" %s to %i\n",debugstr_w(szFeature), iState);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MSI_SetFeatureStateW(package,szFeature,iState);

    msiobj_release( &package->hdr );
    return rc;
}

/***********************************************************************
* MsiGetFeatureStateA   (MSI.@)
*/
UINT WINAPI MsiGetFeatureStateA(MSIHANDLE hInstall, LPSTR szFeature,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    LPWSTR szwFeature = NULL;
    UINT rc;
    
    szwFeature = strdupAtoW(szFeature);

    rc = MsiGetFeatureStateW(hInstall,szwFeature,piInstalled, piAction);

    HeapFree( GetProcessHeap(), 0 , szwFeature);

    return rc;
}

UINT MSI_GetFeatureStateW(MSIPACKAGE *package, LPWSTR szFeature,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    INT index;

    index = get_loaded_feature(package,szFeature);
    if (index < 0)
        return ERROR_UNKNOWN_FEATURE;

    if (piInstalled)
        *piInstalled = package->features[index].Installed;

    if (piAction)
        *piAction = package->features[index].Action;

    TRACE("returning %i %i\n",*piInstalled,*piAction);

    return ERROR_SUCCESS;
}

/***********************************************************************
* MsiGetFeatureStateW   (MSI.@)
*/
UINT WINAPI MsiGetFeatureStateW(MSIHANDLE hInstall, LPWSTR szFeature,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    MSIPACKAGE* package;
    UINT ret;

    TRACE("%ld %s %p %p\n", hInstall, debugstr_w(szFeature), piInstalled,
piAction);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;
    ret = MSI_GetFeatureStateW(package, szFeature, piInstalled, piAction);
    msiobj_release( &package->hdr );
    return ret;
}

/***********************************************************************
 * MsiGetComponentStateA (MSI.@)
 */
UINT WINAPI MsiGetComponentStateA(MSIHANDLE hInstall, LPSTR szComponent,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    LPWSTR szwComponent= NULL;
    UINT rc;
    
    szwComponent= strdupAtoW(szComponent);

    rc = MsiGetComponentStateW(hInstall,szwComponent,piInstalled, piAction);

    HeapFree( GetProcessHeap(), 0 , szwComponent);

    return rc;
}

UINT MSI_GetComponentStateW(MSIPACKAGE *package, LPWSTR szComponent,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    INT index;

    TRACE("%p %s %p %p\n", package, debugstr_w(szComponent), piInstalled,
piAction);

    index = get_loaded_component(package,szComponent);
    if (index < 0)
        return ERROR_UNKNOWN_COMPONENT;

    if (piInstalled)
        *piInstalled = package->components[index].Installed;

    if (piAction)
        *piAction = package->components[index].Action;

    TRACE("states (%i, %i)\n",
(piInstalled)?*piInstalled:-1,(piAction)?*piAction:-1);

    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiGetComponentStateW (MSI.@)
 */
UINT WINAPI MsiGetComponentStateW(MSIHANDLE hInstall, LPWSTR szComponent,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    MSIPACKAGE* package;
    UINT ret;

    TRACE("%ld %s %p %p\n", hInstall, debugstr_w(szComponent),
           piInstalled, piAction);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;
    ret = MSI_GetComponentStateW( package, szComponent, piInstalled, piAction);
    msiobj_release( &package->hdr );
    return ret;
}
