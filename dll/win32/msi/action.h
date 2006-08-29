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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __MSI_ACTION_H__
#define __MSI_ACTION_H__

#include "wine/list.h"

typedef struct tagMSIFEATURE
{
    struct list entry;
    LPWSTR Feature;
    LPWSTR Feature_Parent;
    LPWSTR Title;
    LPWSTR Description;
    INT Display;
    INT Level;
    LPWSTR Directory;
    INT Attributes;
    
    INSTALLSTATE Installed;
    INSTALLSTATE ActionRequest;
    INSTALLSTATE Action;

    struct list Components;
    
    INT Cost;
} MSIFEATURE;

typedef struct tagMSICOMPONENT
{
    struct list entry;
    DWORD magic;
    LPWSTR Component;
    LPWSTR ComponentId;
    LPWSTR Directory;
    INT Attributes;
    LPWSTR Condition;
    LPWSTR KeyPath;

    INSTALLSTATE Installed;
    INSTALLSTATE ActionRequest;
    INSTALLSTATE Action;

    BOOL Enabled;
    INT  Cost;
    INT  RefCount;

    LPWSTR FullKeypath;
    LPWSTR AdvertiseString;
} MSICOMPONENT;

typedef struct tagComponentList
{
    struct list entry;
    MSICOMPONENT *component;
} ComponentList;

typedef struct tagMSIFOLDER
{
    struct list entry;
    LPWSTR Directory;
    LPWSTR TargetDefault;
    LPWSTR SourceLongPath;
    LPWSTR SourceShortPath;

    LPWSTR ResolvedTarget;
    LPWSTR ResolvedSource;
    LPWSTR Property;   /* initially set property */
    struct tagMSIFOLDER *Parent;
    INT   State;
        /* 0 = uninitialized */
        /* 1 = existing */
        /* 2 = created remove if empty */
        /* 3 = created persist if empty */
    INT   Cost;
    INT   Space;
} MSIFOLDER;

typedef enum _msi_file_state {
    msifs_invalid,
    msifs_missing,
    msifs_overwrite,
    msifs_present,
    msifs_installed,
    msifs_skipped,
} msi_file_state;

typedef struct tagMSIFILE
{
    struct list entry;
    LPWSTR File;
    MSICOMPONENT *Component;
    LPWSTR FileName;
    LPWSTR ShortName;
    LPWSTR LongName;
    INT FileSize;
    LPWSTR Version;
    LPWSTR Language;
    INT Attributes;
    INT Sequence;   
    msi_file_state state;
    LPWSTR  SourcePath;
    LPWSTR  TargetPath;
} MSIFILE;

typedef struct tagMSITEMPFILE
{
    struct list entry;
    LPWSTR File;
    LPWSTR Path;
} MSITEMPFILE;

typedef struct tagMSIAPPID
{
    struct list entry;
    LPWSTR AppID; /* Primary key */
    LPWSTR RemoteServerName;
    LPWSTR LocalServer;
    LPWSTR ServiceParameters;
    LPWSTR DllSurrogate;
    BOOL ActivateAtStorage;
    BOOL RunAsInteractiveUser;
} MSIAPPID;

typedef struct tagMSIPROGID MSIPROGID;

typedef struct tagMSICLASS
{
    struct list entry;
    LPWSTR clsid;     /* Primary Key */
    LPWSTR Context;   /* Primary Key */
    MSICOMPONENT *Component;
    MSIPROGID *ProgID;
    LPWSTR ProgIDText;
    LPWSTR Description;
    MSIAPPID *AppID;
    LPWSTR FileTypeMask;
    LPWSTR IconPath;
    LPWSTR DefInprocHandler;
    LPWSTR DefInprocHandler32;
    LPWSTR Argument;
    MSIFEATURE *Feature;
    INT Attributes;
    /* not in the table, set during installation */
    BOOL Installed;
} MSICLASS;

typedef struct tagMSIMIME MSIMIME;

typedef struct tagMSIEXTENSION
{
    struct list entry;
    LPWSTR Extension;  /* Primary Key */
    MSICOMPONENT *Component;
    MSIPROGID *ProgID;
    LPWSTR ProgIDText;
    MSIMIME *Mime;
    MSIFEATURE *Feature;
    /* not in the table, set during installation */
    BOOL Installed;
    struct list verbs;
} MSIEXTENSION;

struct tagMSIPROGID
{
    struct list entry;
    LPWSTR ProgID;  /* Primary Key */
    MSIPROGID *Parent;
    MSICLASS *Class;
    LPWSTR Description;
    LPWSTR IconPath;
    /* not in the table, set during installation */
    BOOL InstallMe;
    MSIPROGID *CurVer;
    MSIPROGID *VersionInd;
};

typedef struct tagMSIVERB
{
    struct list entry;
    LPWSTR Verb;
    INT Sequence;
    LPWSTR Command;
    LPWSTR Argument;
} MSIVERB;

struct tagMSIMIME
{
    struct list entry;
    LPWSTR ContentType;  /* Primary Key */
    MSIEXTENSION *Extension;
    LPWSTR clsid;
    MSICLASS *Class;
    /* not in the table, set during installation */
    BOOL InstallMe;
};

enum SCRIPTS {
        INSTALL_SCRIPT = 0,
        COMMIT_SCRIPT = 1,
        ROLLBACK_SCRIPT = 2,
        TOTAL_SCRIPTS = 3
};

#define SEQUENCE_UI       0x1
#define SEQUENCE_EXEC     0x2
#define SEQUENCE_INSTALL  0x10

typedef struct tagMSISCRIPT
{
    LPWSTR  *Actions[TOTAL_SCRIPTS];
    UINT    ActionCount[TOTAL_SCRIPTS];
    BOOL    ExecuteSequenceRun;
    BOOL    CurrentlyScripting;
    UINT    InWhatSequence;
    LPWSTR  *UniqueActions;
    UINT    UniqueActionsCount;
} MSISCRIPT;


extern UINT ACTION_PerformAction(MSIPACKAGE *package, const WCHAR *action, BOOL force);
extern UINT ACTION_PerformUIAction(MSIPACKAGE *package, const WCHAR *action);
extern void ACTION_FinishCustomActions( MSIPACKAGE* package);
extern UINT ACTION_CustomAction(MSIPACKAGE *package,const WCHAR *action, BOOL execute);

/* actions in other modules */
extern UINT ACTION_AppSearch(MSIPACKAGE *package);
extern UINT ACTION_FindRelatedProducts(MSIPACKAGE *package);
extern UINT ACTION_InstallFiles(MSIPACKAGE *package);
extern UINT ACTION_RemoveFiles(MSIPACKAGE *package);
extern UINT ACTION_DuplicateFiles(MSIPACKAGE *package);
extern UINT ACTION_RegisterClassInfo(MSIPACKAGE *package);
extern UINT ACTION_RegisterProgIdInfo(MSIPACKAGE *package);
extern UINT ACTION_RegisterExtensionInfo(MSIPACKAGE *package);
extern UINT ACTION_RegisterMIMEInfo(MSIPACKAGE *package);


/* Helpers */
extern DWORD deformat_string(MSIPACKAGE *package, LPCWSTR ptr, WCHAR** data );
extern LPWSTR msi_dup_record_field(MSIRECORD *row, INT index);
extern LPWSTR msi_dup_property(MSIPACKAGE *package, LPCWSTR prop);
extern int msi_get_property_int( MSIPACKAGE *package, LPCWSTR prop, int def );
extern LPWSTR resolve_folder(MSIPACKAGE *package, LPCWSTR name, BOOL source, 
                      BOOL set_prop, MSIFOLDER **folder);
extern MSICOMPONENT *get_loaded_component( MSIPACKAGE* package, LPCWSTR Component );
extern MSIFEATURE *get_loaded_feature( MSIPACKAGE* package, LPCWSTR Feature );
extern MSIFILE *get_loaded_file( MSIPACKAGE* package, LPCWSTR file );
extern MSIFOLDER *get_loaded_folder( MSIPACKAGE *package, LPCWSTR dir );
extern int track_tempfile(MSIPACKAGE *package, LPCWSTR name, LPCWSTR path);
extern UINT schedule_action(MSIPACKAGE *package, UINT script, LPCWSTR action);
extern LPWSTR build_icon_path(MSIPACKAGE *, LPCWSTR);
extern LPWSTR build_directory_name(DWORD , ...);
extern BOOL create_full_pathW(const WCHAR *path);
extern BOOL ACTION_VerifyComponentForAction(MSICOMPONENT*, INSTALLSTATE);
extern BOOL ACTION_VerifyFeatureForAction(MSIFEATURE*, INSTALLSTATE);
extern void reduce_to_longfilename(WCHAR*);
extern void reduce_to_shortfilename(WCHAR*);
extern LPWSTR create_component_advertise_string(MSIPACKAGE*, MSICOMPONENT*, LPCWSTR);
extern void ACTION_UpdateComponentStates(MSIPACKAGE *package, LPCWSTR szFeature);
extern UINT register_unique_action(MSIPACKAGE *, LPCWSTR);
extern BOOL check_unique_action(MSIPACKAGE *, LPCWSTR);
extern WCHAR* generate_error_string(MSIPACKAGE *, UINT, DWORD, ... );
extern UINT msi_create_component_directories( MSIPACKAGE *package );


/* control event stuff */
extern VOID ControlEvent_FireSubscribedEvent(MSIPACKAGE *package, LPCWSTR event,
                                      MSIRECORD *data);
extern VOID ControlEvent_CleanupSubscriptions(MSIPACKAGE *package);
extern VOID ControlEvent_SubscribeToEvent(MSIPACKAGE *package, LPCWSTR event,
                                   LPCWSTR control, LPCWSTR attribute);
extern VOID ControlEvent_UnSubscribeToEvent( MSIPACKAGE *package, LPCWSTR event,
                                      LPCWSTR control, LPCWSTR attribute );

/* User Interface messages from the actions */
extern void ui_progress(MSIPACKAGE *, int, int, int, int);
extern void ui_actiondata(MSIPACKAGE *, LPCWSTR, MSIRECORD *);


/* string consts use a number of places  and defined in helpers.c*/
extern const WCHAR cszSourceDir[];
extern const WCHAR szProductCode[];
extern const WCHAR cszRootDrive[];
extern const WCHAR cszbs[];

#endif /* __MSI_ACTION_H__ */
