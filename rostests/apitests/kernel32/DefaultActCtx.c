/*
 * Test for the default activation context that is active in every process.
 *
 * Copyright 2017 Giannis Adamopoulos
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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

START_TEST(DefaultActCtx)
{
    DWORD buffer[256];
    BOOL res;
    PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION details = (PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION)buffer;
    PACTIVATION_CONTEXT_DETAILED_INFORMATION info = (PACTIVATION_CONTEXT_DETAILED_INFORMATION)buffer;
    HANDLE h;
    DWORD i;
    ACTCTX_SECTION_KEYED_DATA KeyedData = { 0 };
    
    res = QueryActCtxW(QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX, 
                       NULL, 
                       NULL, 
                       ActivationContextDetailedInformation,  
                       &buffer, 
                       sizeof(buffer), 
                       NULL);
    ok(res == TRUE, "\n");
    ok(info->lpRootManifestPath == NULL, "Expected null lpRootManifestPath, got %S\n", info->lpRootManifestPath);
    ok(info->lpRootConfigurationPath == NULL, "Expected null lpRootConfigurationPath, got %S\n", info->lpRootConfigurationPath);
    ok(info->lpAppDirPath == NULL, "Expected null lpAppDirPath, got %S\n", info->lpAppDirPath);
    ok(info->ulAssemblyCount == 0, "\n");
    
    i = 0;
    res = QueryActCtxW(QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX, 
                       NULL, 
                       &i, 
                       AssemblyDetailedInformationInActivationContext,  
                       &buffer, 
                       sizeof(buffer), 
                       NULL);
    ok(res == TRUE, "\n");
    ok(details->lpAssemblyEncodedAssemblyIdentity == NULL, "Expected null lpAssemblyEncodedAssemblyIdentity, got %S\n", details->lpAssemblyEncodedAssemblyIdentity);
    ok(details->lpAssemblyManifestPath == NULL, "Expected null lpAssemblyManifestPath, got %S\n", details->lpAssemblyManifestPath);
    ok(details->lpAssemblyPolicyPath == NULL, "Expected null lpAssemblyPolicyPath, got %S\n", details->lpAssemblyPolicyPath);
    ok(details->lpAssemblyDirectoryName == NULL, "Expected null lpAssemblyDirectoryName, got %S\n", details->lpAssemblyDirectoryName);
    
    i = 1;
    res = QueryActCtxW(QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX, 
                       NULL, 
                       &i, 
                       AssemblyDetailedInformationInActivationContext,  
                       &buffer, 
                       sizeof(buffer), 
                       NULL);
    ok(res == TRUE, "\n"); /* This is FALSE in win10 */
    ok(details->lpAssemblyEncodedAssemblyIdentity == NULL, "Expected null lpAssemblyEncodedAssemblyIdentity, got %S\n", details->lpAssemblyEncodedAssemblyIdentity);
    ok(details->lpAssemblyManifestPath == NULL, "Expected null lpAssemblyManifestPath, got %S\n", details->lpAssemblyManifestPath);
    ok(details->lpAssemblyPolicyPath == NULL, "Expected null lpAssemblyPolicyPath, got %S\n", details->lpAssemblyPolicyPath);
    ok(details->lpAssemblyDirectoryName == NULL, "Expected null lpAssemblyDirectoryName, got %S\n", details->lpAssemblyDirectoryName);

    res = GetCurrentActCtx (&h);
    ok(res == TRUE, "\n");
    ok(h == NULL, "\n");

    KeyedData.cbSize = sizeof(KeyedData);
    res = FindActCtxSectionStringW(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX,
                                   NULL,
                                   ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION,
                                   L"Microsoft.Windows.SysyemCompatible",
                                   &KeyedData);
    ok(res == FALSE, "\n");

    KeyedData.cbSize = sizeof(KeyedData);
    res = FindActCtxSectionStringW(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX,
                                   NULL,
                                   ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION,
                                   L"System Default Context",
                                   &KeyedData);
    ok(res == FALSE, "\n");
    
    KeyedData.cbSize = sizeof(KeyedData);
    res = FindActCtxSectionStringW(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX,
                                   NULL,
                                   ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION,
                                   L"Microsoft.Windows.Common-Controls",
                                   &KeyedData);
    ok(res == TRUE, "\n");
    ok(KeyedData.hActCtx == NULL, "Expected null handle for common control context\n");
    ok(KeyedData.ulAssemblyRosterIndex != 0, "%lu\n", KeyedData.ulAssemblyRosterIndex);
    //ok(wcsstr(details-> , L"SystemCompative"
    
}
