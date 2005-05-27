/*
 * Common prototypes for Action handlers
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

typedef struct tagMSIFEATURE
{
    WCHAR Feature[96];
    WCHAR Feature_Parent[96];
    WCHAR Title[0x100];
    WCHAR Description[0x100];
    INT Display;
    INT Level;
    WCHAR Directory[96];
    INT Attributes;
    
    INSTALLSTATE Installed;
    INSTALLSTATE ActionRequest;
    INSTALLSTATE Action;

    INT ComponentCount;
    INT Components[1024]; /* yes hardcoded limit.... I am bad */
    INT Cost;
} MSIFEATURE;

typedef struct tagMSICOMPONENT
{
    WCHAR Component[96];
    WCHAR ComponentId[96];
    WCHAR Directory[96];
    INT Attributes;
    WCHAR Condition[0x100];
    WCHAR KeyPath[96];

    INSTALLSTATE Installed;
    INSTALLSTATE ActionRequest;
    INSTALLSTATE Action;

    BOOL Enabled;
    INT  Cost;
    INT  RefCount;

    LPWSTR FullKeypath;
} MSICOMPONENT;

typedef struct tagMSIFOLDER
{
    LPWSTR Directory;
    LPWSTR TargetDefault;
    LPWSTR SourceDefault;

    LPWSTR ResolvedTarget;
    LPWSTR ResolvedSource;
    LPWSTR Property;   /* initially set property */
    INT   ParentIndex;
    INT   State;
        /* 0 = uninitialized */
        /* 1 = existing */
        /* 2 = created remove if empty */
        /* 3 = created persist if empty */
    INT   Cost;
    INT   Space;
}MSIFOLDER;

typedef struct tagMSIFILE
{
    LPWSTR File;
    INT ComponentIndex;
    LPWSTR FileName;
    LPWSTR ShortName;
    INT FileSize;
    LPWSTR Version;
    LPWSTR Language;
    INT Attributes;
    INT Sequence;   

    INT State;
       /* 0 = uninitialize */
       /* 1 = not present */
       /* 2 = present but replace */
       /* 3 = present do not replace */
       /* 4 = Installed */
    LPWSTR  SourcePath;
    LPWSTR  TargetPath;
    BOOL    Temporary; 
}MSIFILE;


UINT ACTION_PerformAction(MSIPACKAGE *package, const WCHAR *action);
UINT ACTION_PerformUIAction(MSIPACKAGE *package, const WCHAR *action);
void ACTION_FinishCustomActions( MSIPACKAGE* package);
UINT ACTION_CustomAction(MSIPACKAGE *package,const WCHAR *action, BOOL execute);
void ACTION_UpdateComponentStates(MSIPACKAGE *package, LPCWSTR szFeature);
UINT ACTION_AppSearch(MSIPACKAGE *package);

DWORD deformat_string(MSIPACKAGE *package, LPCWSTR ptr, WCHAR** data );
WCHAR *load_dynamic_stringW(MSIRECORD *row, INT index);
LPWSTR load_dynamic_property(MSIPACKAGE *package, LPCWSTR prop, UINT* rc);
LPWSTR resolve_folder(MSIPACKAGE *package, LPCWSTR name, BOOL source, 
                      BOOL set_prop, MSIFOLDER **folder);
int get_loaded_component(MSIPACKAGE* package, LPCWSTR Component );
int get_loaded_feature(MSIPACKAGE* package, LPCWSTR Feature );
int get_loaded_file(MSIPACKAGE* package, LPCWSTR file);
int track_tempfile(MSIPACKAGE *package, LPCWSTR name, LPCWSTR path);
