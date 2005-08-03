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

#define IDENTIFIER_SIZE 96

typedef struct tagMSIFEATURE
{
    WCHAR Feature[IDENTIFIER_SIZE];
    WCHAR Feature_Parent[IDENTIFIER_SIZE];
    WCHAR Title[0x100];
    WCHAR Description[0x100];
    INT Display;
    INT Level;
    WCHAR Directory[IDENTIFIER_SIZE];
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
    WCHAR Component[IDENTIFIER_SIZE];
    WCHAR ComponentId[IDENTIFIER_SIZE];
    WCHAR Directory[IDENTIFIER_SIZE];
    INT Attributes;
    WCHAR Condition[0x100];
    WCHAR KeyPath[IDENTIFIER_SIZE];

    INSTALLSTATE Installed;
    INSTALLSTATE ActionRequest;
    INSTALLSTATE Action;

    BOOL Enabled;
    INT  Cost;
    INT  RefCount;

    LPWSTR FullKeypath;
    LPWSTR AdvertiseString;
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

typedef struct tagMSICLASS
{
    WCHAR CLSID[IDENTIFIER_SIZE];     /* Primary Key */
    WCHAR Context[IDENTIFIER_SIZE];   /* Primary Key */
    INT ComponentIndex;               /* Primary Key */
    INT ProgIDIndex;
    LPWSTR ProgIDText;
    LPWSTR Description;
    INT AppIDIndex;
    LPWSTR FileTypeMask;
    LPWSTR IconPath;
    LPWSTR DefInprocHandler;
    LPWSTR DefInprocHandler32;
    LPWSTR Argument;
    INT FeatureIndex;
    INT Attributes;
    /* not in the table, set during installation */
    BOOL Installed;
} MSICLASS;

typedef struct tagMSIEXTENSION
{
    WCHAR Extension[256];  /* Primary Key */
    INT ComponentIndex;    /* Primary Key */
    INT ProgIDIndex;
    LPWSTR ProgIDText;
    INT MIMEIndex;
    INT FeatureIndex;
    /* not in the table, set during installation */
    BOOL Installed;
    INT VerbCount;
    INT Verbs[100]; /* yes hard coded limit, but realistically 100 verbs??? */
} MSIEXTENSION;

typedef struct tagMSIPROGID
{
    LPWSTR ProgID;  /* Primary Key */
    INT ParentIndex;
    INT ClassIndex;
    LPWSTR Description;
    LPWSTR IconPath;
    /* not in the table, set during installation */
    BOOL InstallMe;
    INT CurVerIndex;
    INT VersionIndIndex;
} MSIPROGID;

typedef struct tagMSIVERB
{
    INT ExtensionIndex;
    LPWSTR Verb;
    INT Sequence;
    LPWSTR Command;
    LPWSTR Argument;
} MSIVERB;

typedef struct tagMSIMIME
{
    LPWSTR ContentType;  /* Primary Key */
    INT ExtensionIndex;
    WCHAR CLSID[IDENTIFIER_SIZE];
    INT ClassIndex;
    /* not in the table, set during installation */
    BOOL InstallMe;
} MSIMIME;

typedef struct tagMSIAPPID
{
    WCHAR AppID[IDENTIFIER_SIZE]; /* Primary key */
    LPWSTR RemoteServerName;
    LPWSTR LocalServer;
    LPWSTR ServiceParameters;
    LPWSTR DllSurrogate;
    BOOL ActivateAtStorage;
    BOOL RunAsInteractiveUser;
} MSIAPPID;

enum SCRIPTS {
        INSTALL_SCRIPT = 0,
        COMMIT_SCRIPT = 1,
        ROLLBACK_SCRIPT = 2,
        TOTAL_SCRIPTS = 3
};

typedef struct tagMSISCRIPT
{
    LPWSTR  *Actions[TOTAL_SCRIPTS];
    UINT    ActionCount[TOTAL_SCRIPTS];
    BOOL    ExecuteSequenceRun;
    BOOL    FindRelatedProductsRun;
    BOOL    CurrentlyScripting;
}MSISCRIPT;


UINT ACTION_PerformAction(MSIPACKAGE *package, const WCHAR *action, BOOL force);
UINT ACTION_PerformUIAction(MSIPACKAGE *package, const WCHAR *action);
void ACTION_FinishCustomActions( MSIPACKAGE* package);
UINT ACTION_CustomAction(MSIPACKAGE *package,const WCHAR *action, BOOL execute);

/* actions in other modules */
UINT ACTION_AppSearch(MSIPACKAGE *package);
UINT ACTION_FindRelatedProducts(MSIPACKAGE *package);
UINT ACTION_InstallFiles(MSIPACKAGE *package);
UINT ACTION_DuplicateFiles(MSIPACKAGE *package);
UINT ACTION_RegisterClassInfo(MSIPACKAGE *package);
UINT ACTION_RegisterProgIdInfo(MSIPACKAGE *package);
UINT ACTION_RegisterExtensionInfo(MSIPACKAGE *package);
UINT ACTION_RegisterMIMEInfo(MSIPACKAGE *package);


/* Helpers */
DWORD deformat_string(MSIPACKAGE *package, LPCWSTR ptr, WCHAR** data );
WCHAR *load_dynamic_stringW(MSIRECORD *row, INT index);
LPWSTR load_dynamic_property(MSIPACKAGE *package, LPCWSTR prop, UINT* rc);
LPWSTR resolve_folder(MSIPACKAGE *package, LPCWSTR name, BOOL source, 
                      BOOL set_prop, MSIFOLDER **folder);
int get_loaded_component(MSIPACKAGE* package, LPCWSTR Component );
int get_loaded_feature(MSIPACKAGE* package, LPCWSTR Feature );
int get_loaded_file(MSIPACKAGE* package, LPCWSTR file);
int track_tempfile(MSIPACKAGE *package, LPCWSTR name, LPCWSTR path);
UINT schedule_action(MSIPACKAGE *package, UINT script, LPCWSTR action);
UINT build_icon_path(MSIPACKAGE *, LPCWSTR, LPWSTR *);
DWORD build_version_dword(LPCWSTR);
LPWSTR build_directory_name(DWORD , ...);
BOOL create_full_pathW(const WCHAR *path);
BOOL ACTION_VerifyComponentForAction(MSIPACKAGE*, INT, INSTALLSTATE);
BOOL ACTION_VerifyFeatureForAction(MSIPACKAGE*, INT, INSTALLSTATE);
void reduce_to_longfilename(WCHAR*);
void reduce_to_shortfilename(WCHAR*);
LPWSTR create_component_advertise_string(MSIPACKAGE*, MSICOMPONENT*, LPCWSTR);
void ACTION_UpdateComponentStates(MSIPACKAGE *package, LPCWSTR szFeature);


/* control event stuff */
VOID ControlEvent_FireSubscribedEvent(MSIPACKAGE *package, LPCWSTR event,
                                      MSIRECORD *data);
VOID ControlEvent_CleanupSubscriptions(MSIPACKAGE *package);
VOID ControlEvent_SubscribeToEvent(MSIPACKAGE *package, LPCWSTR event,
                                   LPCWSTR control, LPCWSTR attribute);
VOID ControlEvent_UnSubscribeToEvent( MSIPACKAGE *package, LPCWSTR event,
                                      LPCWSTR control, LPCWSTR attribute );

/* User Interface messages from the actions */
void ui_progress(MSIPACKAGE *, int, int, int, int);
void ui_actiondata(MSIPACKAGE *, LPCWSTR, MSIRECORD *);


/* string consts use a number of places  and defined in helpers.c*/
extern const WCHAR cszSourceDir[];
extern const WCHAR szProductCode[];
extern const WCHAR cszRootDrive[];
extern const WCHAR cszbs[];
