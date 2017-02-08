/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2005 Mike McCormack for CodeWeavers
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

#ifndef __WINE_MSI_PRIVATE__
#define __WINE_MSI_PRIVATE__

#include <wine/config.h>

#include <assert.h>
#include <stdarg.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wincon.h>
#include <winver.h>
#include <msiquery.h>
#include <objbase.h>
#include <msiserver.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <fusion.h>
#include <sddl.h>
#include <msidefs.h>

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/unicode.h>

static const BOOL is_64bit = sizeof(void *) > sizeof(int);
BOOL is_wow64 DECLSPEC_HIDDEN;

#define MSI_DATASIZEMASK 0x00ff
#define MSITYPE_VALID    0x0100
#define MSITYPE_LOCALIZABLE 0x200
#define MSITYPE_STRING   0x0800
#define MSITYPE_NULLABLE 0x1000
#define MSITYPE_KEY      0x2000
#define MSITYPE_TEMPORARY 0x4000
#define MSITYPE_UNKNOWN   0x8000

#define MAX_STREAM_NAME_LEN     62
#define LONG_STR_BYTES  3

/* Install UI level mask for AND operation to exclude flags */
#define INSTALLUILEVEL_MASK             0x0007

#define MSITYPE_IS_BINARY(type) (((type) & ~MSITYPE_NULLABLE) == (MSITYPE_STRING|MSITYPE_VALID))

struct tagMSITABLE;
typedef struct tagMSITABLE MSITABLE;

struct string_table;
typedef struct string_table string_table;

struct tagMSIOBJECTHDR;
typedef struct tagMSIOBJECTHDR MSIOBJECTHDR;

typedef VOID (*msihandledestructor)( MSIOBJECTHDR * );

struct tagMSIOBJECTHDR
{
    UINT magic;
    UINT type;
    LONG refcount;
    msihandledestructor destructor;
};

#define MSI_INITIAL_MEDIA_TRANSFORM_OFFSET 10000
#define MSI_INITIAL_MEDIA_TRANSFORM_DISKID 30000

typedef struct tagMSISTREAM
{
    UINT str_index;
    IStream *stream;
} MSISTREAM;

typedef struct tagMSITRANSFORM
{
    struct list entry;
    IStorage *stg;
} MSITRANSFORM;

typedef struct tagMSIDATABASE
{
    MSIOBJECTHDR hdr;
    IStorage *storage;
    string_table *strings;
    UINT bytes_per_strref;
    LPWSTR path;
    LPWSTR deletefile;
    LPWSTR tempfolder;
    LPCWSTR mode;
    UINT media_transform_offset;
    UINT media_transform_disk_id;
    struct list tables;
    struct list transforms;
    MSISTREAM *streams;
    UINT num_streams;
    UINT num_streams_allocated;
} MSIDATABASE;

typedef struct tagMSIVIEW MSIVIEW;

typedef struct tagMSIQUERY
{
    MSIOBJECTHDR hdr;
    MSIVIEW *view;
    UINT row;
    MSIDATABASE *db;
    struct list mem;
} MSIQUERY;

/* maybe we can use a Variant instead of doing it ourselves? */
typedef struct tagMSIFIELD
{
    UINT type;
    union
    {
        INT iVal;
        INT_PTR pVal;
        LPWSTR szwVal;
        IStream *stream;
    } u;
    int len;
} MSIFIELD;

typedef struct tagMSIRECORD
{
    MSIOBJECTHDR hdr;
    UINT count;       /* as passed to MsiCreateRecord */
    MSIFIELD fields[1]; /* nb. array size is count+1 */
} MSIRECORD;

typedef struct tagMSISOURCELISTINFO
{
    struct list entry;
    DWORD context;
    DWORD options;
    LPCWSTR property;
    LPWSTR value;
} MSISOURCELISTINFO;

typedef struct tagMSIMEDIADISK
{
    struct list entry;
    DWORD context;
    DWORD options;
    DWORD disk_id;
    LPWSTR volume_label;
    LPWSTR disk_prompt;
} MSIMEDIADISK;

typedef struct tagMSIMEDIAINFO
{
    UINT disk_id;
    UINT type;
    UINT last_sequence;
    LPWSTR disk_prompt;
    LPWSTR cabinet;
    LPWSTR volume_label;
    BOOL is_continuous;
    BOOL is_extracted;
    WCHAR sourcedir[MAX_PATH];
} MSIMEDIAINFO;

typedef struct tagMSICABINETSTREAM
{
    struct list entry;
    UINT disk_id;
    IStorage *storage;
    WCHAR *stream;
} MSICABINETSTREAM;

typedef struct tagMSIPATCHINFO
{
    struct list entry;
    LPWSTR patchcode;
    LPWSTR products;
    LPWSTR transforms;
    LPWSTR filename;
    LPWSTR localfile;
    MSIPATCHSTATE state;
    BOOL delete_on_close;
    BOOL registered;
    UINT disk_id;
} MSIPATCHINFO;

typedef struct tagMSIBINARY
{
    struct list entry;
    WCHAR *source;
    WCHAR *tmpfile;
    HMODULE module;
} MSIBINARY;

typedef struct _column_info
{
    LPCWSTR table;
    LPCWSTR column;
    INT   type;
    BOOL   temporary;
    struct expr *val;
    struct _column_info *next;
} column_info;

typedef const struct tagMSICOLUMNHASHENTRY *MSIITERHANDLE;

typedef struct tagMSIVIEWOPS
{
    /*
     * fetch_int - reads one integer from {row,col} in the table
     *
     *  This function should be called after the execute method.
     *  Data returned by the function should not change until
     *   close or delete is called.
     *  To get a string value, query the database's string table with
     *   the integer value returned from this function.
     */
    UINT (*fetch_int)( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val );

    /*
     * fetch_stream - gets a stream from {row,col} in the table
     *
     *  This function is similar to fetch_int, except fetches a
     *    stream instead of an integer.
     */
    UINT (*fetch_stream)( struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm );

    /*
     * get_row - gets values from a row
     *
     */
    UINT (*get_row)( struct tagMSIVIEW *view, UINT row, MSIRECORD **rec );

    /*
     * set_row - sets values in a row as specified by mask
     *
     *  Similar semantics to fetch_int
     */
    UINT (*set_row)( struct tagMSIVIEW *view, UINT row, MSIRECORD *rec, UINT mask );

    /*
     * Inserts a new row into the database from the records contents
     */
    UINT (*insert_row)( struct tagMSIVIEW *view, MSIRECORD *record, UINT row, BOOL temporary );

    /*
     * Deletes a row from the database
     */
    UINT (*delete_row)( struct tagMSIVIEW *view, UINT row );

    /*
     * execute - loads the underlying data into memory so it can be read
     */
    UINT (*execute)( struct tagMSIVIEW *view, MSIRECORD *record );

    /*
     * close - clears the data read by execute from memory
     */
    UINT (*close)( struct tagMSIVIEW *view );

    /*
     * get_dimensions - returns the number of rows or columns in a table.
     *
     *  The number of rows can only be queried after the execute method
     *   is called. The number of columns can be queried at any time.
     */
    UINT (*get_dimensions)( struct tagMSIVIEW *view, UINT *rows, UINT *cols );

    /*
     * get_column_info - returns the name and type of a specific column
     *
     *  The column information can be queried at any time.
     */
    UINT (*get_column_info)( struct tagMSIVIEW *view, UINT n, LPCWSTR *name, UINT *type,
                             BOOL *temporary, LPCWSTR *table_name );

    /*
     * modify - not yet implemented properly
     */
    UINT (*modify)( struct tagMSIVIEW *view, MSIMODIFY eModifyMode, MSIRECORD *record, UINT row );

    /*
     * delete - destroys the structure completely
     */
    UINT (*delete)( struct tagMSIVIEW * );

    /*
     * find_matching_rows - iterates through rows that match a value
     *
     * If the column type is a string then a string ID should be passed in.
     *  If the value to be looked up is an integer then no transformation of
     *  the input value is required, except if the column is a string, in which
     *  case a string ID should be passed in.
     * The handle is an input/output parameter that keeps track of the current
     *  position in the iteration. It must be initialised to zero before the
     *  first call and continued to be passed in to subsequent calls.
     */
    UINT (*find_matching_rows)( struct tagMSIVIEW *view, UINT col, UINT val, UINT *row, MSIITERHANDLE *handle );

    /*
     * add_ref - increases the reference count of the table
     */
    UINT (*add_ref)( struct tagMSIVIEW *view );

    /*
     * release - decreases the reference count of the table
     */
    UINT (*release)( struct tagMSIVIEW *view );

    /*
     * add_column - adds a column to the table
     */
    UINT (*add_column)( struct tagMSIVIEW *view, LPCWSTR table, UINT number, LPCWSTR column, UINT type, BOOL hold );

    /*
     * remove_column - removes the column represented by table name and column number from the table
     */
    UINT (*remove_column)( struct tagMSIVIEW *view, LPCWSTR table, UINT number );

    /*
     * sort - orders the table by columns
     */
    UINT (*sort)( struct tagMSIVIEW *view, column_info *columns );

    /*
     * drop - drops the table from the database
     */
    UINT (*drop)( struct tagMSIVIEW *view );
} MSIVIEWOPS;

struct tagMSIVIEW
{
    MSIOBJECTHDR hdr;
    const MSIVIEWOPS *ops;
    MSIDBERROR error;
    const WCHAR *error_column;
};

struct msi_dialog_tag;
typedef struct msi_dialog_tag msi_dialog;

enum platform
{
    PLATFORM_UNKNOWN,
    PLATFORM_INTEL,
    PLATFORM_INTEL64,
    PLATFORM_X64,
    PLATFORM_ARM
};

enum clr_version
{
    CLR_VERSION_V10,
    CLR_VERSION_V11,
    CLR_VERSION_V20,
    CLR_VERSION_V40,
    CLR_VERSION_MAX
};

typedef struct tagMSIPACKAGE
{
    MSIOBJECTHDR hdr;
    MSIDATABASE *db;
    INT version;
    enum platform platform;
    UINT num_langids;
    LANGID *langids;
    struct list patches;
    struct list components;
    struct list features;
    struct list files;
    struct list filepatches;
    struct list tempfiles;
    struct list folders;
    struct list binaries;
    struct list cabinet_streams;
    LPWSTR ActionFormat;
    LPWSTR LastAction;
    UINT   action_progress_increment;
    HANDLE log_file;
    IAssemblyCache *cache_net[CLR_VERSION_MAX];
    IAssemblyCache *cache_sxs;

    struct list classes;
    struct list extensions;
    struct list progids;
    struct list mimes;
    struct list appids;

    struct tagMSISCRIPT *script;

    struct list RunningActions;

    LPWSTR BaseURL;
    LPWSTR PackagePath;
    LPWSTR ProductCode;
    LPWSTR localfile;
    BOOL delete_on_close;

    INSTALLUILEVEL ui_level;
    UINT CurrentInstallState;
    msi_dialog *dialog;
    LPWSTR next_dialog;
    float center_x;
    float center_y;

    UINT WordCount;
    UINT Context;

    struct list subscriptions;

    struct list sourcelist_info;
    struct list sourcelist_media;

    unsigned char scheduled_action_running : 1;
    unsigned char commit_action_running : 1;
    unsigned char rollback_action_running : 1;
    unsigned char need_reboot_at_end : 1;
    unsigned char need_reboot_now : 1;
    unsigned char need_rollback : 1;
    unsigned char full_reinstall : 1;
} MSIPACKAGE;

typedef struct tagMSIPREVIEW
{
    MSIOBJECTHDR hdr;
    MSIPACKAGE *package;
    msi_dialog *dialog;
} MSIPREVIEW;

#define MSI_MAX_PROPS 20

typedef struct tagMSISUMMARYINFO
{
    MSIOBJECTHDR hdr;
    IStorage *storage;
    DWORD update_count;
    PROPVARIANT property[MSI_MAX_PROPS];
} MSISUMMARYINFO;

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
    struct list Children;
    struct list Components;
} MSIFEATURE;

typedef struct tagMSIASSEMBLY
{
    LPWSTR feature;
    LPWSTR manifest;
    LPWSTR application;
    DWORD attributes;
    LPWSTR display_name;
    LPWSTR tempdir;
    BOOL installed;
    BOOL clr_version[CLR_VERSION_MAX];
} MSIASSEMBLY;

typedef struct tagMSICOMPONENT
{
    struct list entry;
    LPWSTR Component;
    LPWSTR ComponentId;
    LPWSTR Directory;
    INT Attributes;
    LPWSTR Condition;
    LPWSTR KeyPath;
    INSTALLSTATE Installed;
    INSTALLSTATE ActionRequest;
    INSTALLSTATE Action;
    BOOL ForceLocalState;
    BOOL Enabled;
    INT  Cost;
    INT  RefCount;
    LPWSTR FullKeypath;
    LPWSTR AdvertiseString;
    MSIASSEMBLY *assembly;
    int num_clients;

    unsigned int anyAbsent:1;
    unsigned int hasAdvertisedFeature:1;
    unsigned int hasLocalFeature:1;
    unsigned int hasSourceFeature:1;
} MSICOMPONENT;

typedef struct tagComponentList
{
    struct list entry;
    MSICOMPONENT *component;
} ComponentList;

typedef struct tagFeatureList
{
    struct list entry;
    MSIFEATURE *feature;
} FeatureList;

enum folder_state
{
    FOLDER_STATE_UNINITIALIZED,
    FOLDER_STATE_EXISTS,
    FOLDER_STATE_CREATED,
    FOLDER_STATE_CREATED_PERSISTENT,
    FOLDER_STATE_REMOVED
};

typedef struct tagMSIFOLDER
{
    struct list entry;
    struct list children;
    LPWSTR Directory;
    LPWSTR Parent;
    LPWSTR TargetDefault;
    LPWSTR SourceLongPath;
    LPWSTR SourceShortPath;
    LPWSTR ResolvedTarget;
    LPWSTR ResolvedSource;
    enum folder_state State;
    BOOL persistent;
    INT Cost;
    INT Space;
} MSIFOLDER;

typedef struct tagFolderList
{
    struct list entry;
    MSIFOLDER *folder;
} FolderList;

typedef enum _msi_file_state {
    msifs_invalid,
    msifs_missing,
    msifs_overwrite,
    msifs_present,
    msifs_installed,
    msifs_skipped,
    msifs_hashmatch
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
    LPWSTR  TargetPath;
    BOOL IsCompressed;
    MSIFILEHASHINFO hash;
    UINT disk_id;
} MSIFILE;

typedef struct tagMSIFILEPATCH
{
    struct list entry;
    MSIFILE *File;
    INT Sequence;
    INT PatchSize;
    INT Attributes;
    BOOL extracted;
    UINT disk_id;
    WCHAR *path;
} MSIFILEPATCH;

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
    INSTALLSTATE action;
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
    INSTALLSTATE action;
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
    LPWSTR suffix;
    LPWSTR clsid;
    MSICLASS *Class;
};

enum SCRIPTS
{
    SCRIPT_NONE     = -1,
    SCRIPT_INSTALL  = 0,
    SCRIPT_COMMIT   = 1,
    SCRIPT_ROLLBACK = 2,
    SCRIPT_MAX      = 3
};

#define SEQUENCE_UI       0x1
#define SEQUENCE_EXEC     0x2
#define SEQUENCE_INSTALL  0x10

typedef struct tagMSISCRIPT
{
    LPWSTR  *Actions[SCRIPT_MAX];
    UINT    ActionCount[SCRIPT_MAX];
    BOOL    ExecuteSequenceRun;
    UINT    InWhatSequence;
    LPWSTR  *UniqueActions;
    UINT    UniqueActionsCount;
} MSISCRIPT;

#define MSIHANDLETYPE_ANY 0
#define MSIHANDLETYPE_DATABASE 1
#define MSIHANDLETYPE_SUMMARYINFO 2
#define MSIHANDLETYPE_VIEW 3
#define MSIHANDLETYPE_RECORD 4
#define MSIHANDLETYPE_PACKAGE 5
#define MSIHANDLETYPE_PREVIEW 6

#define MSI_MAJORVERSION 4
#define MSI_MINORVERSION 5
#define MSI_BUILDNUMBER 6001

#define GUID_SIZE 39
#define SQUISH_GUID_SIZE 33

#define MSIHANDLE_MAGIC 0x4d434923

/* handle unicode/ascii output in the Msi* API functions */
typedef struct {
    BOOL unicode;
    union {
       LPSTR a;
       LPWSTR w;
    } str;
} awstring;

typedef struct {
    BOOL unicode;
    union {
       LPCSTR a;
       LPCWSTR w;
    } str;
} awcstring;

UINT msi_strcpy_to_awstring(const WCHAR *, int, awstring *, DWORD *) DECLSPEC_HIDDEN;

/* msi server interface */
extern HRESULT create_msi_custom_remote( IUnknown *pOuter, LPVOID *ppObj ) DECLSPEC_HIDDEN;
extern HRESULT create_msi_remote_package( IUnknown *pOuter, LPVOID *ppObj ) DECLSPEC_HIDDEN;
extern HRESULT create_msi_remote_database( IUnknown *pOuter, LPVOID *ppObj ) DECLSPEC_HIDDEN;
extern IUnknown *msi_get_remote(MSIHANDLE handle) DECLSPEC_HIDDEN;

/* handle functions */
extern void *msihandle2msiinfo(MSIHANDLE handle, UINT type) DECLSPEC_HIDDEN;
extern MSIHANDLE alloc_msihandle( MSIOBJECTHDR * ) DECLSPEC_HIDDEN;
extern MSIHANDLE alloc_msi_remote_handle( IUnknown *unk ) DECLSPEC_HIDDEN;
extern void *alloc_msiobject(UINT type, UINT size, msihandledestructor destroy ) DECLSPEC_HIDDEN;
extern void msiobj_addref(MSIOBJECTHDR *) DECLSPEC_HIDDEN;
extern int msiobj_release(MSIOBJECTHDR *) DECLSPEC_HIDDEN;
extern void msiobj_lock(MSIOBJECTHDR *) DECLSPEC_HIDDEN;
extern void msiobj_unlock(MSIOBJECTHDR *) DECLSPEC_HIDDEN;
extern void msi_free_handle_table(void) DECLSPEC_HIDDEN;

extern void free_cached_tables( MSIDATABASE *db ) DECLSPEC_HIDDEN;
extern UINT MSI_CommitTables( MSIDATABASE *db ) DECLSPEC_HIDDEN;
extern UINT msi_commit_streams( MSIDATABASE *db ) DECLSPEC_HIDDEN;


/* string table functions */
enum StringPersistence
{
    StringPersistent = 0,
    StringNonPersistent = 1
};

extern BOOL msi_add_string( string_table *st, const WCHAR *data, int len, enum StringPersistence persistence ) DECLSPEC_HIDDEN;
extern UINT msi_string2id( const string_table *st, const WCHAR *data, int len, UINT *id ) DECLSPEC_HIDDEN;
extern VOID msi_destroy_stringtable( string_table *st ) DECLSPEC_HIDDEN;
extern const WCHAR *msi_string_lookup( const string_table *st, UINT id, int *len ) DECLSPEC_HIDDEN;
extern HRESULT msi_init_string_table( IStorage *stg ) DECLSPEC_HIDDEN;
extern string_table *msi_load_string_table( IStorage *stg, UINT *bytes_per_strref ) DECLSPEC_HIDDEN;
extern UINT msi_save_string_table( const string_table *st, IStorage *storage, UINT *bytes_per_strref ) DECLSPEC_HIDDEN;
extern UINT msi_get_string_table_codepage( const string_table *st ) DECLSPEC_HIDDEN;
extern UINT msi_set_string_table_codepage( string_table *st, UINT codepage ) DECLSPEC_HIDDEN;
extern WCHAR *msi_strdupW( const WCHAR *value, int len ) DECLSPEC_HIDDEN;

extern BOOL TABLE_Exists( MSIDATABASE *db, LPCWSTR name ) DECLSPEC_HIDDEN;
extern MSICONDITION MSI_DatabaseIsTablePersistent( MSIDATABASE *db, LPCWSTR table ) DECLSPEC_HIDDEN;

extern UINT read_stream_data( IStorage *stg, LPCWSTR stname, BOOL table,
                              BYTE **pdata, UINT *psz ) DECLSPEC_HIDDEN;
extern UINT write_stream_data( IStorage *stg, LPCWSTR stname,
                               LPCVOID data, UINT sz, BOOL bTable ) DECLSPEC_HIDDEN;

/* transform functions */
extern UINT msi_table_apply_transform( MSIDATABASE *db, IStorage *stg ) DECLSPEC_HIDDEN;
extern UINT MSI_DatabaseApplyTransformW( MSIDATABASE *db,
                 LPCWSTR szTransformFile, int iErrorCond ) DECLSPEC_HIDDEN;
extern void append_storage_to_db( MSIDATABASE *db, IStorage *stg ) DECLSPEC_HIDDEN;
extern UINT msi_apply_transforms( MSIPACKAGE *package ) DECLSPEC_HIDDEN;

/* patch functions */
extern UINT msi_check_patch_applicable( MSIPACKAGE *package, MSISUMMARYINFO *si ) DECLSPEC_HIDDEN;
extern UINT msi_apply_patches( MSIPACKAGE *package ) DECLSPEC_HIDDEN;
extern UINT msi_apply_registered_patch( MSIPACKAGE *package, LPCWSTR patch_code ) DECLSPEC_HIDDEN;
extern void msi_free_patchinfo( MSIPATCHINFO *patch ) DECLSPEC_HIDDEN;

/* action internals */
extern UINT MSI_InstallPackage( MSIPACKAGE *, LPCWSTR, LPCWSTR ) DECLSPEC_HIDDEN;
extern UINT ACTION_DialogBox( MSIPACKAGE*, LPCWSTR) DECLSPEC_HIDDEN;
extern UINT ACTION_ForceReboot(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT MSI_Sequence( MSIPACKAGE *package, LPCWSTR szTable ) DECLSPEC_HIDDEN;
extern UINT MSI_SetFeatureStates( MSIPACKAGE *package ) DECLSPEC_HIDDEN;
extern UINT msi_parse_command_line( MSIPACKAGE *package, LPCWSTR szCommandLine, BOOL preserve_case ) DECLSPEC_HIDDEN;
extern UINT msi_schedule_action( MSIPACKAGE *package, UINT script, const WCHAR *action ) DECLSPEC_HIDDEN;
extern INSTALLSTATE msi_get_component_action( MSIPACKAGE *package, MSICOMPONENT *comp ) DECLSPEC_HIDDEN;
extern INSTALLSTATE msi_get_feature_action( MSIPACKAGE *package, MSIFEATURE *feature ) DECLSPEC_HIDDEN;
extern UINT msi_load_all_components( MSIPACKAGE *package ) DECLSPEC_HIDDEN;
extern UINT msi_load_all_features( MSIPACKAGE *package ) DECLSPEC_HIDDEN;
extern UINT msi_validate_product_id( MSIPACKAGE *package ) DECLSPEC_HIDDEN;

/* record internals */
extern void MSI_CloseRecord( MSIOBJECTHDR * ) DECLSPEC_HIDDEN;
extern UINT MSI_RecordSetIStream( MSIRECORD *, UINT, IStream *) DECLSPEC_HIDDEN;
extern UINT MSI_RecordGetIStream( MSIRECORD *, UINT, IStream **) DECLSPEC_HIDDEN;
extern const WCHAR *MSI_RecordGetString( const MSIRECORD *, UINT ) DECLSPEC_HIDDEN;
extern MSIRECORD *MSI_CreateRecord( UINT ) DECLSPEC_HIDDEN;
extern UINT MSI_RecordSetInteger( MSIRECORD *, UINT, int ) DECLSPEC_HIDDEN;
extern UINT MSI_RecordSetIntPtr( MSIRECORD *, UINT, INT_PTR ) DECLSPEC_HIDDEN;
extern UINT MSI_RecordSetStringW( MSIRECORD *, UINT, LPCWSTR ) DECLSPEC_HIDDEN;
extern BOOL MSI_RecordIsNull( MSIRECORD *, UINT ) DECLSPEC_HIDDEN;
extern UINT MSI_RecordGetStringW( MSIRECORD * , UINT, LPWSTR, LPDWORD) DECLSPEC_HIDDEN;
extern UINT MSI_RecordGetStringA( MSIRECORD *, UINT, LPSTR, LPDWORD) DECLSPEC_HIDDEN;
extern int MSI_RecordGetInteger( MSIRECORD *, UINT ) DECLSPEC_HIDDEN;
extern INT_PTR MSI_RecordGetIntPtr( MSIRECORD *, UINT ) DECLSPEC_HIDDEN;
extern UINT MSI_RecordReadStream( MSIRECORD *, UINT, char *, LPDWORD) DECLSPEC_HIDDEN;
extern UINT MSI_RecordSetStream(MSIRECORD *, UINT, IStream *) DECLSPEC_HIDDEN;
extern UINT MSI_RecordGetFieldCount( const MSIRECORD *rec ) DECLSPEC_HIDDEN;
extern UINT MSI_RecordStreamToFile( MSIRECORD *, UINT, LPCWSTR ) DECLSPEC_HIDDEN;
extern UINT MSI_RecordSetStreamFromFileW( MSIRECORD *, UINT, LPCWSTR ) DECLSPEC_HIDDEN;
extern UINT MSI_RecordCopyField( MSIRECORD *, UINT, MSIRECORD *, UINT ) DECLSPEC_HIDDEN;
extern MSIRECORD *MSI_CloneRecord( MSIRECORD * ) DECLSPEC_HIDDEN;
extern BOOL MSI_RecordsAreEqual( MSIRECORD *, MSIRECORD * ) DECLSPEC_HIDDEN;
extern BOOL MSI_RecordsAreFieldsEqual(MSIRECORD *a, MSIRECORD *b, UINT field) DECLSPEC_HIDDEN;
extern UINT msi_record_set_string(MSIRECORD *, UINT, const WCHAR *, int) DECLSPEC_HIDDEN;
extern const WCHAR *msi_record_get_string(const MSIRECORD *, UINT, int *) DECLSPEC_HIDDEN;

/* stream internals */
extern void enum_stream_names( IStorage *stg ) DECLSPEC_HIDDEN;
extern LPWSTR encode_streamname(BOOL bTable, LPCWSTR in) DECLSPEC_HIDDEN;
extern BOOL decode_streamname(LPCWSTR in, LPWSTR out) DECLSPEC_HIDDEN;

/* database internals */
extern UINT msi_get_stream( MSIDATABASE *, const WCHAR *, IStream ** ) DECLSPEC_HIDDEN;
extern UINT MSI_OpenDatabaseW( LPCWSTR, LPCWSTR, MSIDATABASE ** ) DECLSPEC_HIDDEN;
extern UINT MSI_DatabaseOpenViewW(MSIDATABASE *, LPCWSTR, MSIQUERY ** ) DECLSPEC_HIDDEN;
extern UINT MSI_OpenQuery( MSIDATABASE *, MSIQUERY **, LPCWSTR, ... ) DECLSPEC_HIDDEN;
typedef UINT (*record_func)( MSIRECORD *, LPVOID );
extern UINT MSI_IterateRecords( MSIQUERY *, LPDWORD, record_func, LPVOID ) DECLSPEC_HIDDEN;
extern MSIRECORD *MSI_QueryGetRecord( MSIDATABASE *db, LPCWSTR query, ... ) DECLSPEC_HIDDEN;
extern UINT MSI_DatabaseGetPrimaryKeys( MSIDATABASE *, LPCWSTR, MSIRECORD ** ) DECLSPEC_HIDDEN;

/* view internals */
extern UINT MSI_ViewExecute( MSIQUERY*, MSIRECORD * ) DECLSPEC_HIDDEN;
extern UINT MSI_ViewFetch( MSIQUERY*, MSIRECORD ** ) DECLSPEC_HIDDEN;
extern UINT MSI_ViewClose( MSIQUERY* ) DECLSPEC_HIDDEN;
extern UINT MSI_ViewGetColumnInfo(MSIQUERY *, MSICOLINFO, MSIRECORD **) DECLSPEC_HIDDEN;
extern UINT MSI_ViewModify( MSIQUERY *, MSIMODIFY, MSIRECORD * ) DECLSPEC_HIDDEN;
extern UINT VIEW_find_column( MSIVIEW *, LPCWSTR, LPCWSTR, UINT * ) DECLSPEC_HIDDEN;
extern UINT msi_view_get_row(MSIDATABASE *, MSIVIEW *, UINT, MSIRECORD **) DECLSPEC_HIDDEN;

/* install internals */
extern UINT MSI_SetInstallLevel( MSIPACKAGE *package, int iInstallLevel ) DECLSPEC_HIDDEN;

/* package internals */
extern MSIPACKAGE *MSI_CreatePackage( MSIDATABASE *, LPCWSTR ) DECLSPEC_HIDDEN;
extern UINT MSI_OpenPackageW( LPCWSTR szPackage, MSIPACKAGE **pPackage ) DECLSPEC_HIDDEN;
extern UINT MSI_SetTargetPathW( MSIPACKAGE *, LPCWSTR, LPCWSTR ) DECLSPEC_HIDDEN;
extern INT MSI_ProcessMessage( MSIPACKAGE *, INSTALLMESSAGE, MSIRECORD * ) DECLSPEC_HIDDEN;
extern MSICONDITION MSI_EvaluateConditionW( MSIPACKAGE *, LPCWSTR ) DECLSPEC_HIDDEN;
extern UINT MSI_GetComponentStateW( MSIPACKAGE *, LPCWSTR, INSTALLSTATE *, INSTALLSTATE * ) DECLSPEC_HIDDEN;
extern UINT MSI_GetFeatureStateW( MSIPACKAGE *, LPCWSTR, INSTALLSTATE *, INSTALLSTATE * ) DECLSPEC_HIDDEN;
extern UINT MSI_SetFeatureStateW(MSIPACKAGE*, LPCWSTR, INSTALLSTATE ) DECLSPEC_HIDDEN;
extern UINT msi_download_file( LPCWSTR szUrl, LPWSTR filename ) DECLSPEC_HIDDEN;
extern UINT msi_package_add_info(MSIPACKAGE *, DWORD, DWORD, LPCWSTR, LPWSTR) DECLSPEC_HIDDEN;
extern UINT msi_package_add_media_disk(MSIPACKAGE *, DWORD, DWORD, DWORD, LPWSTR, LPWSTR) DECLSPEC_HIDDEN;
extern UINT msi_clone_properties(MSIDATABASE *) DECLSPEC_HIDDEN;
extern UINT msi_set_context(MSIPACKAGE *) DECLSPEC_HIDDEN;
extern void msi_adjust_privilege_properties(MSIPACKAGE *) DECLSPEC_HIDDEN;
extern UINT MSI_GetFeatureCost(MSIPACKAGE *, MSIFEATURE *, MSICOSTTREE, INSTALLSTATE, LPINT) DECLSPEC_HIDDEN;

/* for deformating */
extern UINT MSI_FormatRecordW( MSIPACKAGE *, MSIRECORD *, LPWSTR, LPDWORD ) DECLSPEC_HIDDEN;

/* registry data encoding/decoding functions */
extern BOOL unsquash_guid(LPCWSTR in, LPWSTR out) DECLSPEC_HIDDEN;
extern BOOL squash_guid(LPCWSTR in, LPWSTR out) DECLSPEC_HIDDEN;
extern BOOL encode_base85_guid(GUID *,LPWSTR) DECLSPEC_HIDDEN;
extern BOOL decode_base85_guid(LPCWSTR,GUID*) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenUninstallKey(const WCHAR *, enum platform, HKEY *, BOOL) DECLSPEC_HIDDEN;
extern UINT MSIREG_DeleteUninstallKey(const WCHAR *, enum platform) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenProductKey(LPCWSTR szProduct, LPCWSTR szUserSid,
                                  MSIINSTALLCONTEXT context, HKEY* key, BOOL create) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenFeaturesKey(LPCWSTR szProduct, LPCWSTR szUserSid, MSIINSTALLCONTEXT context,
                                   HKEY *key, BOOL create) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenUserPatchesKey(LPCWSTR szPatch, HKEY* key, BOOL create) DECLSPEC_HIDDEN;
UINT MSIREG_OpenUserDataFeaturesKey(LPCWSTR szProduct, LPCWSTR szUserSid, MSIINSTALLCONTEXT context,
                                    HKEY *key, BOOL create) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenUserComponentsKey(LPCWSTR szComponent, HKEY* key, BOOL create) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenUserDataComponentKey(LPCWSTR szComponent, LPCWSTR szUserSid,
                                            HKEY *key, BOOL create) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenPatchesKey(LPCWSTR szPatch, HKEY* key, BOOL create) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenUserDataProductKey(LPCWSTR szProduct, MSIINSTALLCONTEXT dwContext,
                                          LPCWSTR szUserSid, HKEY *key, BOOL create) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenUserDataPatchKey(LPCWSTR szPatch, MSIINSTALLCONTEXT dwContext,
                                        HKEY *key, BOOL create) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenUserDataProductPatchesKey(LPCWSTR product, MSIINSTALLCONTEXT context,
                                                 HKEY *key, BOOL create) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenInstallProps(LPCWSTR szProduct, MSIINSTALLCONTEXT dwContext,
                                    LPCWSTR szUserSid, HKEY *key, BOOL create) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenUpgradeCodesKey(LPCWSTR szProduct, HKEY* key, BOOL create) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenUserUpgradeCodesKey(LPCWSTR szProduct, HKEY* key, BOOL create) DECLSPEC_HIDDEN;
extern UINT MSIREG_DeleteProductKey(LPCWSTR szProduct) DECLSPEC_HIDDEN;
extern UINT MSIREG_DeleteUserProductKey(LPCWSTR szProduct) DECLSPEC_HIDDEN;
extern UINT MSIREG_DeleteUserDataPatchKey(LPCWSTR patch, MSIINSTALLCONTEXT context) DECLSPEC_HIDDEN;
extern UINT MSIREG_DeleteUserDataProductKey(LPCWSTR szProduct) DECLSPEC_HIDDEN;
extern UINT MSIREG_DeleteUserFeaturesKey(LPCWSTR szProduct) DECLSPEC_HIDDEN;
extern UINT MSIREG_DeleteUserDataComponentKey(LPCWSTR szComponent, LPCWSTR szUserSid) DECLSPEC_HIDDEN;
extern UINT MSIREG_DeleteUserUpgradeCodesKey(LPCWSTR szUpgradeCode) DECLSPEC_HIDDEN;
extern UINT MSIREG_DeleteClassesUpgradeCodesKey(LPCWSTR szUpgradeCode) DECLSPEC_HIDDEN;
extern UINT MSIREG_OpenClassesUpgradeCodesKey(LPCWSTR szUpgradeCode, HKEY* key, BOOL create) DECLSPEC_HIDDEN;
extern UINT MSIREG_DeleteLocalClassesProductKey(LPCWSTR szProductCode) DECLSPEC_HIDDEN;
extern UINT MSIREG_DeleteLocalClassesFeaturesKey(LPCWSTR szProductCode) DECLSPEC_HIDDEN;
extern UINT msi_locate_product(LPCWSTR szProduct, MSIINSTALLCONTEXT *context) DECLSPEC_HIDDEN;
extern LPWSTR msi_reg_get_val_str( HKEY hkey, LPCWSTR name ) DECLSPEC_HIDDEN;
extern BOOL msi_reg_get_val_dword( HKEY hkey, LPCWSTR name, DWORD *val) DECLSPEC_HIDDEN;

extern DWORD msi_version_str_to_dword(LPCWSTR p) DECLSPEC_HIDDEN;
extern void msi_parse_version_string(LPCWSTR, PDWORD, PDWORD) DECLSPEC_HIDDEN;
extern VS_FIXEDFILEINFO *msi_get_disk_file_version(LPCWSTR) DECLSPEC_HIDDEN;
extern int msi_compare_file_versions(VS_FIXEDFILEINFO *, const WCHAR *) DECLSPEC_HIDDEN;
extern int msi_compare_font_versions(const WCHAR *, const WCHAR *) DECLSPEC_HIDDEN;
extern DWORD msi_get_disk_file_size(LPCWSTR) DECLSPEC_HIDDEN;
extern BOOL msi_file_hash_matches(MSIFILE *) DECLSPEC_HIDDEN;
extern UINT msi_get_filehash(const WCHAR *, MSIFILEHASHINFO *) DECLSPEC_HIDDEN;

extern LONG msi_reg_set_val_str( HKEY hkey, LPCWSTR name, LPCWSTR value ) DECLSPEC_HIDDEN;
extern LONG msi_reg_set_val_multi_str( HKEY hkey, LPCWSTR name, LPCWSTR value ) DECLSPEC_HIDDEN;
extern LONG msi_reg_set_val_dword( HKEY hkey, LPCWSTR name, DWORD val ) DECLSPEC_HIDDEN;
extern LONG msi_reg_set_subkey_val( HKEY hkey, LPCWSTR path, LPCWSTR name, LPCWSTR val ) DECLSPEC_HIDDEN;

/* msi dialog interface */
extern void msi_dialog_check_messages( HANDLE ) DECLSPEC_HIDDEN;
extern void msi_dialog_destroy( msi_dialog* ) DECLSPEC_HIDDEN;
extern void msi_dialog_unregister_class( void ) DECLSPEC_HIDDEN;
extern UINT msi_spawn_error_dialog( MSIPACKAGE*, LPWSTR, LPWSTR ) DECLSPEC_HIDDEN;

/* summary information */
extern UINT msi_get_suminfo( IStorage *stg, UINT uiUpdateCount, MSISUMMARYINFO **si ) DECLSPEC_HIDDEN;
extern UINT msi_get_db_suminfo( MSIDATABASE *db, UINT uiUpdateCount, MSISUMMARYINFO **si ) DECLSPEC_HIDDEN;
extern LPWSTR msi_suminfo_dup_string( MSISUMMARYINFO *si, UINT uiProperty ) DECLSPEC_HIDDEN;
extern INT msi_suminfo_get_int32( MSISUMMARYINFO *si, UINT uiProperty ) DECLSPEC_HIDDEN;
extern LPWSTR msi_get_suminfo_product( IStorage *stg ) DECLSPEC_HIDDEN;
extern UINT msi_add_suminfo( MSIDATABASE *db, LPWSTR **records, int num_records, int num_columns ) DECLSPEC_HIDDEN;
extern UINT msi_export_suminfo( MSIDATABASE *db, HANDLE handle ) DECLSPEC_HIDDEN;
extern enum platform parse_platform( const WCHAR *str ) DECLSPEC_HIDDEN;
extern UINT msi_load_suminfo_properties( MSIPACKAGE *package ) DECLSPEC_HIDDEN;

/* undocumented functions */
UINT WINAPI MsiCreateAndVerifyInstallerDirectory( DWORD );
UINT WINAPI MsiDecomposeDescriptorW( LPCWSTR, LPWSTR, LPWSTR, LPWSTR, LPDWORD );
UINT WINAPI MsiDecomposeDescriptorA( LPCSTR, LPSTR, LPSTR, LPSTR, LPDWORD );
LANGID WINAPI MsiLoadStringW( MSIHANDLE, UINT, LPWSTR, int, LANGID );
LANGID WINAPI MsiLoadStringA( MSIHANDLE, UINT, LPSTR, int, LANGID );

/* UI globals */
extern INSTALLUILEVEL gUILevel DECLSPEC_HIDDEN;
extern HWND gUIhwnd DECLSPEC_HIDDEN;
extern INSTALLUI_HANDLERA gUIHandlerA DECLSPEC_HIDDEN;
extern INSTALLUI_HANDLERW gUIHandlerW DECLSPEC_HIDDEN;
extern INSTALLUI_HANDLER_RECORD gUIHandlerRecord DECLSPEC_HIDDEN;
extern DWORD gUIFilter DECLSPEC_HIDDEN;
extern LPVOID gUIContext DECLSPEC_HIDDEN;
extern WCHAR *gszLogFile DECLSPEC_HIDDEN;
extern HINSTANCE msi_hInstance DECLSPEC_HIDDEN;

/* action related functions */
extern UINT ACTION_PerformAction(MSIPACKAGE *package, const WCHAR *action, UINT script) DECLSPEC_HIDDEN;
extern UINT ACTION_PerformUIAction(MSIPACKAGE *package, const WCHAR *action, UINT script) DECLSPEC_HIDDEN;
extern void ACTION_FinishCustomActions( const MSIPACKAGE* package) DECLSPEC_HIDDEN;
extern UINT ACTION_CustomAction(MSIPACKAGE *, const WCHAR *, UINT) DECLSPEC_HIDDEN;

/* actions in other modules */
extern UINT ACTION_AppSearch(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_CCPSearch(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_FindRelatedProducts(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_InstallFiles(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_PatchFiles( MSIPACKAGE *package ) DECLSPEC_HIDDEN;
extern UINT ACTION_RemoveFiles(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_MoveFiles(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_DuplicateFiles(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_RemoveDuplicateFiles(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_RegisterClassInfo(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_RegisterProgIdInfo(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_RegisterExtensionInfo(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_RegisterMIMEInfo(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_RegisterFonts(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_UnregisterClassInfo(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_UnregisterExtensionInfo(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_UnregisterFonts(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_UnregisterMIMEInfo(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_UnregisterProgIdInfo(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_MsiPublishAssemblies(MSIPACKAGE *package) DECLSPEC_HIDDEN;
extern UINT ACTION_MsiUnpublishAssemblies(MSIPACKAGE *package) DECLSPEC_HIDDEN;

/* Helpers */
extern DWORD deformat_string(MSIPACKAGE *package, LPCWSTR ptr, WCHAR** data ) DECLSPEC_HIDDEN;
extern WCHAR *msi_dup_record_field(MSIRECORD *row, INT index) DECLSPEC_HIDDEN;
extern LPWSTR msi_dup_property( MSIDATABASE *db, LPCWSTR prop ) DECLSPEC_HIDDEN;
extern UINT msi_set_property( MSIDATABASE *, const WCHAR *, const WCHAR *, int ) DECLSPEC_HIDDEN;
extern UINT msi_get_property( MSIDATABASE *, LPCWSTR, LPWSTR, LPDWORD ) DECLSPEC_HIDDEN;
extern int msi_get_property_int( MSIDATABASE *package, LPCWSTR prop, int def ) DECLSPEC_HIDDEN;
extern WCHAR *msi_resolve_source_folder(MSIPACKAGE *package, const WCHAR *name, MSIFOLDER **folder) DECLSPEC_HIDDEN;
extern void msi_resolve_target_folder(MSIPACKAGE *package, const WCHAR *name, BOOL load_prop) DECLSPEC_HIDDEN;
extern WCHAR *msi_normalize_path(const WCHAR *) DECLSPEC_HIDDEN;
extern WCHAR *msi_resolve_file_source(MSIPACKAGE *package, MSIFILE *file) DECLSPEC_HIDDEN;
extern const WCHAR *msi_get_target_folder(MSIPACKAGE *package, const WCHAR *name) DECLSPEC_HIDDEN;
extern void msi_reset_folders( MSIPACKAGE *package, BOOL source ) DECLSPEC_HIDDEN;
extern MSICOMPONENT *msi_get_loaded_component(MSIPACKAGE *package, const WCHAR *Component) DECLSPEC_HIDDEN;
extern MSIFEATURE *msi_get_loaded_feature(MSIPACKAGE *package, const WCHAR *Feature) DECLSPEC_HIDDEN;
extern MSIFILE *msi_get_loaded_file(MSIPACKAGE *package, const WCHAR *file) DECLSPEC_HIDDEN;
extern MSIFOLDER *msi_get_loaded_folder(MSIPACKAGE *package, const WCHAR *dir) DECLSPEC_HIDDEN;
extern WCHAR *msi_create_temp_file(MSIDATABASE *db) DECLSPEC_HIDDEN;
extern void msi_free_action_script(MSIPACKAGE *package, UINT script) DECLSPEC_HIDDEN;
extern WCHAR *msi_build_icon_path(MSIPACKAGE *, const WCHAR *) DECLSPEC_HIDDEN;
extern WCHAR *msi_build_directory_name(DWORD , ...) DECLSPEC_HIDDEN;
extern BOOL msi_create_full_path(const WCHAR *path) DECLSPEC_HIDDEN;
extern void msi_reduce_to_long_filename(WCHAR *) DECLSPEC_HIDDEN;
extern WCHAR *msi_create_component_advertise_string(MSIPACKAGE *, MSICOMPONENT *, const WCHAR *) DECLSPEC_HIDDEN;
extern void ACTION_UpdateComponentStates(MSIPACKAGE *package, MSIFEATURE *feature) DECLSPEC_HIDDEN;
extern UINT msi_register_unique_action(MSIPACKAGE *, const WCHAR *) DECLSPEC_HIDDEN;
extern BOOL msi_action_is_unique(const MSIPACKAGE *, const WCHAR *) DECLSPEC_HIDDEN;
extern WCHAR *msi_build_error_string(MSIPACKAGE *, UINT, DWORD, ...) DECLSPEC_HIDDEN;
extern UINT msi_set_last_used_source(LPCWSTR product, LPCWSTR usersid,
                        MSIINSTALLCONTEXT context, DWORD options, LPCWSTR value) DECLSPEC_HIDDEN;
extern UINT msi_create_empty_local_file(LPWSTR path, LPCWSTR suffix) DECLSPEC_HIDDEN;
extern UINT msi_set_sourcedir_props(MSIPACKAGE *package, BOOL replace) DECLSPEC_HIDDEN;
extern MSIASSEMBLY *msi_load_assembly(MSIPACKAGE *, MSICOMPONENT *) DECLSPEC_HIDDEN;
extern UINT msi_install_assembly(MSIPACKAGE *, MSICOMPONENT *) DECLSPEC_HIDDEN;
extern UINT msi_uninstall_assembly(MSIPACKAGE *, MSICOMPONENT *) DECLSPEC_HIDDEN;
extern BOOL msi_init_assembly_caches(MSIPACKAGE *) DECLSPEC_HIDDEN;
extern void msi_destroy_assembly_caches(MSIPACKAGE *) DECLSPEC_HIDDEN;
extern BOOL msi_is_global_assembly(MSICOMPONENT *) DECLSPEC_HIDDEN;
extern IAssemblyEnum *msi_create_assembly_enum(MSIPACKAGE *, const WCHAR *) DECLSPEC_HIDDEN;
extern WCHAR *msi_get_assembly_path(MSIPACKAGE *, const WCHAR *) DECLSPEC_HIDDEN;
extern WCHAR *msi_font_version_from_file(const WCHAR *) DECLSPEC_HIDDEN;
extern WCHAR **msi_split_string(const WCHAR *, WCHAR) DECLSPEC_HIDDEN;
extern UINT msi_set_original_database_property(MSIDATABASE *, const WCHAR *) DECLSPEC_HIDDEN;

/* media */

typedef BOOL (*PMSICABEXTRACTCB)(MSIPACKAGE *, LPCWSTR, DWORD, LPWSTR *, DWORD *, PVOID);

#define MSICABEXTRACT_BEGINEXTRACT  0x01
#define MSICABEXTRACT_FILEEXTRACTED 0x02

typedef struct
{
    MSIPACKAGE* package;
    MSIMEDIAINFO *mi;
    PMSICABEXTRACTCB cb;
    LPWSTR curfile;
    PVOID user;
} MSICABDATA;

extern UINT ready_media(MSIPACKAGE *package, BOOL compressed, MSIMEDIAINFO *mi) DECLSPEC_HIDDEN;
extern UINT msi_load_media_info(MSIPACKAGE *package, UINT Sequence, MSIMEDIAINFO *mi) DECLSPEC_HIDDEN;
extern void msi_free_media_info(MSIMEDIAINFO *mi) DECLSPEC_HIDDEN;
extern BOOL msi_cabextract(MSIPACKAGE* package, MSIMEDIAINFO *mi, LPVOID data) DECLSPEC_HIDDEN;
extern UINT msi_add_cabinet_stream(MSIPACKAGE *, UINT, IStorage *, const WCHAR *) DECLSPEC_HIDDEN;

/* control event stuff */
extern void msi_event_fire(MSIPACKAGE *, const WCHAR *, MSIRECORD *) DECLSPEC_HIDDEN;
extern void msi_event_cleanup_all_subscriptions( MSIPACKAGE * ) DECLSPEC_HIDDEN;

/* OLE automation */
typedef enum tid_t {
    Database_tid,
    Installer_tid,
    Record_tid,
    Session_tid,
    StringList_tid,
    SummaryInfo_tid,
    View_tid,
    LAST_tid
} tid_t;

extern HRESULT create_msiserver(IUnknown *pOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
extern HRESULT create_session(MSIHANDLE msiHandle, IDispatch *pInstaller, IDispatch **pDispatch) DECLSPEC_HIDDEN;
extern HRESULT get_typeinfo(tid_t tid, ITypeInfo **ti) DECLSPEC_HIDDEN;
extern void release_typelib(void) DECLSPEC_HIDDEN;

/* Scripting */
extern DWORD call_script(MSIHANDLE hPackage, INT type, LPCWSTR script, LPCWSTR function, LPCWSTR action) DECLSPEC_HIDDEN;

/* User interface messages from the actions */
extern void msi_ui_progress(MSIPACKAGE *, int, int, int, int) DECLSPEC_HIDDEN;
extern void msi_ui_actiondata(MSIPACKAGE *, const WCHAR *, MSIRECORD *) DECLSPEC_HIDDEN;

/* common strings */
static const WCHAR szSourceDir[] = {'S','o','u','r','c','e','D','i','r',0};
static const WCHAR szSOURCEDIR[] = {'S','O','U','R','C','E','D','I','R',0};
static const WCHAR szRootDrive[] = {'R','O','O','T','D','R','I','V','E',0};
static const WCHAR szTargetDir[] = {'T','A','R','G','E','T','D','I','R',0};
static const WCHAR szLocalSid[] = {'S','-','1','-','5','-','1','8',0};
static const WCHAR szAllSid[] = {'S','-','1','-','1','-','0',0};
static const WCHAR szEmpty[] = {0};
static const WCHAR szAll[] = {'A','L','L',0};
static const WCHAR szOne[] = {'1',0};
static const WCHAR szZero[] = {'0',0};
static const WCHAR szSpace[] = {' ',0};
static const WCHAR szBackSlash[] = {'\\',0};
static const WCHAR szForwardSlash[] = {'/',0};
static const WCHAR szDot[] = {'.',0};
static const WCHAR szDotDot[] = {'.','.',0};
static const WCHAR szSemiColon[] = {';',0};
static const WCHAR szPreselected[] = {'P','r','e','s','e','l','e','c','t','e','d',0};
static const WCHAR szPatches[] = {'P','a','t','c','h','e','s',0};
static const WCHAR szState[] = {'S','t','a','t','e',0};
static const WCHAR szMsi[] = {'m','s','i',0};
static const WCHAR szPatch[] = {'P','A','T','C','H',0};
static const WCHAR szSourceList[] = {'S','o','u','r','c','e','L','i','s','t',0};
static const WCHAR szInstalled[] = {'I','n','s','t','a','l','l','e','d',0};
static const WCHAR szReinstall[] = {'R','E','I','N','S','T','A','L','L',0};
static const WCHAR szReinstallMode[] = {'R','E','I','N','S','T','A','L','L','M','O','D','E',0};
static const WCHAR szRemove[] = {'R','E','M','O','V','E',0};
static const WCHAR szUserSID[] = {'U','s','e','r','S','I','D',0};
static const WCHAR szProductCode[] = {'P','r','o','d','u','c','t','C','o','d','e',0};
static const WCHAR szRegisterClassInfo[] = {'R','e','g','i','s','t','e','r','C','l','a','s','s','I','n','f','o',0};
static const WCHAR szRegisterProgIdInfo[] = {'R','e','g','i','s','t','e','r','P','r','o','g','I','d','I','n','f','o',0};
static const WCHAR szRegisterExtensionInfo[] = {'R','e','g','i','s','t','e','r','E','x','t','e','n','s','i','o','n','I','n','f','o',0};
static const WCHAR szRegisterMIMEInfo[] = {'R','e','g','i','s','t','e','r','M','I','M','E','I','n','f','o',0};
static const WCHAR szDuplicateFiles[] = {'D','u','p','l','i','c','a','t','e','F','i','l','e','s',0};
static const WCHAR szRemoveDuplicateFiles[] = {'R','e','m','o','v','e','D','u','p','l','i','c','a','t','e','F','i','l','e','s',0};
static const WCHAR szInstallFiles[] = {'I','n','s','t','a','l','l','F','i','l','e','s',0};
static const WCHAR szPatchFiles[] = {'P','a','t','c','h','F','i','l','e','s',0};
static const WCHAR szRemoveFiles[] = {'R','e','m','o','v','e','F','i','l','e','s',0};
static const WCHAR szFindRelatedProducts[] = {'F','i','n','d','R','e','l','a','t','e','d','P','r','o','d','u','c','t','s',0};
static const WCHAR szAllUsers[] = {'A','L','L','U','S','E','R','S',0};
static const WCHAR szCustomActionData[] = {'C','u','s','t','o','m','A','c','t','i','o','n','D','a','t','a',0};
static const WCHAR szUILevel[] = {'U','I','L','e','v','e','l',0};
static const WCHAR szProductID[] = {'P','r','o','d','u','c','t','I','D',0};
static const WCHAR szPIDTemplate[] = {'P','I','D','T','e','m','p','l','a','t','e',0};
static const WCHAR szPIDKEY[] = {'P','I','D','K','E','Y',0};
static const WCHAR szTYPELIB[] = {'T','Y','P','E','L','I','B',0};
static const WCHAR szSumInfo[] = {5 ,'S','u','m','m','a','r','y','I','n','f','o','r','m','a','t','i','o','n',0};
static const WCHAR szHCR[] = {'H','K','E','Y','_','C','L','A','S','S','E','S','_','R','O','O','T','\\',0};
static const WCHAR szHCU[] = {'H','K','E','Y','_','C','U','R','R','E','N','T','_','U','S','E','R','\\',0};
static const WCHAR szHLM[] = {'H','K','E','Y','_','L','O','C','A','L','_','M','A','C','H','I','N','E','\\',0};
static const WCHAR szHU[] = {'H','K','E','Y','_','U','S','E','R','S','\\',0};
static const WCHAR szWindowsFolder[] = {'W','i','n','d','o','w','s','F','o','l','d','e','r',0};
static const WCHAR szAppSearch[] = {'A','p','p','S','e','a','r','c','h',0};
static const WCHAR szMoveFiles[] = {'M','o','v','e','F','i','l','e','s',0};
static const WCHAR szCCPSearch[] = {'C','C','P','S','e','a','r','c','h',0};
static const WCHAR szUnregisterClassInfo[] = {'U','n','r','e','g','i','s','t','e','r','C','l','a','s','s','I','n','f','o',0};
static const WCHAR szUnregisterExtensionInfo[] = {'U','n','r','e','g','i','s','t','e','r','E','x','t','e','n','s','i','o','n','I','n','f','o',0};
static const WCHAR szUnregisterMIMEInfo[] = {'U','n','r','e','g','i','s','t','e','r','M','I','M','E','I','n','f','o',0};
static const WCHAR szUnregisterProgIdInfo[] = {'U','n','r','e','g','i','s','t','e','r','P','r','o','g','I','d','I','n','f','o',0};
static const WCHAR szRegisterFonts[] = {'R','e','g','i','s','t','e','r','F','o','n','t','s',0};
static const WCHAR szUnregisterFonts[] = {'U','n','r','e','g','i','s','t','e','r','F','o','n','t','s',0};
static const WCHAR szCLSID[] = {'C','L','S','I','D',0};
static const WCHAR szProgID[] = {'P','r','o','g','I','D',0};
static const WCHAR szVIProgID[] = {'V','e','r','s','i','o','n','I','n','d','e','p','e','n','d','e','n','t','P','r','o','g','I','D',0};
static const WCHAR szAppID[] = {'A','p','p','I','D',0};
static const WCHAR szDefaultIcon[] = {'D','e','f','a','u','l','t','I','c','o','n',0};
static const WCHAR szInprocHandler[] = {'I','n','p','r','o','c','H','a','n','d','l','e','r',0};
static const WCHAR szInprocHandler32[] = {'I','n','p','r','o','c','H','a','n','d','l','e','r','3','2',0};
static const WCHAR szMIMEDatabase[] = {'M','I','M','E','\\','D','a','t','a','b','a','s','e','\\','C','o','n','t','e','n','t',' ','T','y','p','e','\\',0};
static const WCHAR szLocalPackage[] = {'L','o','c','a','l','P','a','c','k','a','g','e',0};
static const WCHAR szOriginalDatabase[] = {'O','r','i','g','i','n','a','l','D','a','t','a','b','a','s','e',0};
static const WCHAR szUpgradeCode[] = {'U','p','g','r','a','d','e','C','o','d','e',0};
static const WCHAR szAdminUser[] = {'A','d','m','i','n','U','s','e','r',0};
static const WCHAR szIntel[] = {'I','n','t','e','l',0};
static const WCHAR szIntel64[] = {'I','n','t','e','l','6','4',0};
static const WCHAR szX64[] = {'x','6','4',0};
static const WCHAR szAMD64[] = {'A','M','D','6','4',0};
static const WCHAR szARM[] = {'A','r','m',0};
static const WCHAR szWow6432NodeCLSID[] = {'W','o','w','6','4','3','2','N','o','d','e','\\','C','L','S','I','D',0};
static const WCHAR szStreams[] = {'_','S','t','r','e','a','m','s',0};
static const WCHAR szStorages[] = {'_','S','t','o','r','a','g','e','s',0};
static const WCHAR szMsiPublishAssemblies[] = {'M','s','i','P','u','b','l','i','s','h','A','s','s','e','m','b','l','i','e','s',0};
static const WCHAR szCostingComplete[] = {'C','o','s','t','i','n','g','C','o','m','p','l','e','t','e',0};
static const WCHAR szTempFolder[] = {'T','e','m','p','F','o','l','d','e','r',0};
static const WCHAR szDatabase[] = {'D','A','T','A','B','A','S','E',0};
static const WCHAR szCRoot[] = {'C',':','\\',0};
static const WCHAR szProductLanguage[] = {'P','r','o','d','u','c','t','L','a','n','g','u','a','g','e',0};
static const WCHAR szProductVersion[] = {'P','r','o','d','u','c','t','V','e','r','s','i','o','n',0};
static const WCHAR szWindowsInstaller[] = {'W','i','n','d','o','w','s','I','n','s','t','a','l','l','e','r',0};
static const WCHAR szStringData[] = {'_','S','t','r','i','n','g','D','a','t','a',0};
static const WCHAR szStringPool[] = {'_','S','t','r','i','n','g','P','o','o','l',0};
static const WCHAR szInstallLevel[] = {'I','N','S','T','A','L','L','L','E','V','E','L',0};
static const WCHAR szCostInitialize[] = {'C','o','s','t','I','n','i','t','i','a','l','i','z','e',0};
static const WCHAR szAppDataFolder[] = {'A','p','p','D','a','t','a','F','o','l','d','e','r',0};
static const WCHAR szRollbackDisabled[] = {'R','o','l','l','b','a','c','k','D','i','s','a','b','l','e','d',0};
static const WCHAR szName[] = {'N','a','m','e',0};
static const WCHAR szData[] = {'D','a','t','a',0};
static const WCHAR szLangResource[] = {'\\','V','a','r','F','i','l','e','I','n','f','o','\\','T','r','a','n','s','l','a','t','i','o','n',0};
static const WCHAR szInstallLocation[] = {'I','n','s','t','a','l','l','L','o','c','a','t','i','o','n',0};
static const WCHAR szProperty[] = {'P','r','o','p','e','r','t','y',0};

/* memory allocation macro functions */
static void *msi_alloc( size_t len ) __WINE_ALLOC_SIZE(1);
static inline void *msi_alloc( size_t len )
{
    return HeapAlloc( GetProcessHeap(), 0, len );
}

static void *msi_alloc_zero( size_t len ) __WINE_ALLOC_SIZE(1);
static inline void *msi_alloc_zero( size_t len )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len );
}

static void *msi_realloc( void *mem, size_t len ) __WINE_ALLOC_SIZE(2);
static inline void *msi_realloc( void *mem, size_t len )
{
    return HeapReAlloc( GetProcessHeap(), 0, mem, len );
}

static void *msi_realloc_zero( void *mem, size_t len ) __WINE_ALLOC_SIZE(2);
static inline void *msi_realloc_zero( void *mem, size_t len )
{
    return HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, mem, len );
}

static inline BOOL msi_free( void *mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}

static inline char *strdupWtoA( LPCWSTR str )
{
    LPSTR ret = NULL;
    DWORD len;

    if (!str) return ret;
    len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    ret = msi_alloc( len );
    if (ret)
        WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    return ret;
}

static inline LPWSTR strdupAtoW( LPCSTR str )
{
    LPWSTR ret = NULL;
    DWORD len;

    if (!str) return ret;
    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    ret = msi_alloc( len * sizeof(WCHAR) );
    if (ret)
        MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    return ret;
}

static inline LPWSTR strdupW( LPCWSTR src )
{
    LPWSTR dest;
    if (!src) return NULL;
    dest = msi_alloc( (lstrlenW(src)+1)*sizeof(WCHAR) );
    if (dest)
        lstrcpyW(dest, src);
    return dest;
}

#include "query.h"

#endif /* __WINE_MSI_PRIVATE__ */
