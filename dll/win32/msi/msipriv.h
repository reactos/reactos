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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "fdi.h"
#include "msi.h"
#include "msiquery.h"
#include "msidefs.h"
#include "objbase.h"
#include "objidl.h"
#include "fusion.h"
#include "winnls.h"
#include "winver.h"
#include "wine/list.h"
#include "wine/debug.h"

#include "msiserver.h"
#include "winemsi_s.h"

static const BOOL is_64bit = sizeof(void *) > sizeof(int);
extern BOOL is_wow64;

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
#define MSI_INITIAL_MEDIA_TRANSFORM_DISKID 32000

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

/* integer versions of the MSIDBOPEN_* constants */
#define MSI_OPEN_READONLY 0
#define MSI_OPEN_TRANSACT 1
#define MSI_OPEN_DIRECT 2
#define MSI_OPEN_CREATE 3
#define MSI_OPEN_CREATEDIRECT 4
#define MSI_OPEN_PATCHFILE 32

typedef struct tagMSIDATABASE
{
    MSIOBJECTHDR hdr;
    IStorage *storage;
    string_table *strings;
    UINT bytes_per_strref;
    LPWSTR path;
    LPWSTR deletefile;
    LPWSTR tempfolder;
    UINT mode;
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
        LPWSTR szwVal;
        IStream *stream;
    } u;
    int len;
} MSIFIELD;

typedef struct tagMSIRECORD
{
    MSIOBJECTHDR hdr;
    UINT count;       /* as passed to MsiCreateRecord */
    UINT64 cookie;
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
    LPWSTR last_volume;
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
    DWORD uninstallable;
    BOOL delete_on_close;
    BOOL registered;
    UINT disk_id;
} MSIPATCHINFO;

typedef struct tagMSIBINARY
{
    struct list entry;
    WCHAR *source;
    WCHAR *tmpfile;
} MSIBINARY;

typedef struct _column_info
{
    LPCWSTR table;
    LPCWSTR column;
    INT   type;
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
     * set_int - set the integer value at {row, col}
     * This function has undefined behaviour if the column does not contain
     * integers.
     */
    UINT (*set_int)( struct tagMSIVIEW *view, UINT row, UINT col, int val );

    /*
     * set_string - set the string value at {row, col}
     * This function has undefined behaviour if the column does not contain
     * strings.
     */
    UINT (*set_string)( struct tagMSIVIEW *view, UINT row, UINT col, const WCHAR *val, int len );

    /*
     * set_stream - set the stream value at {row, col}
     * This function has undefined behaviour if the column does not contain
     * streams.
     */
    UINT (*set_stream)( struct tagMSIVIEW *view, UINT row, UINT col, IStream *stream );

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
    UINT (*add_column)( struct tagMSIVIEW *view, LPCWSTR column, INT type, BOOL hold );

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
    PLATFORM_UNRECOGNIZED,
    PLATFORM_INTEL,
    PLATFORM_INTEL64,
    PLATFORM_X64,
    PLATFORM_ARM,
    PLATFORM_ARM64
};

enum clr_version
{
    CLR_VERSION_V10,
    CLR_VERSION_V11,
    CLR_VERSION_V20,
    CLR_VERSION_V40,
    CLR_VERSION_MAX
};

enum script
{
    SCRIPT_NONE     = -1,
    SCRIPT_INSTALL  = 0,
    SCRIPT_COMMIT   = 1,
    SCRIPT_ROLLBACK = 2,
    SCRIPT_MAX      = 3
};

typedef struct tagMSIPACKAGE
{
    MSIOBJECTHDR hdr;
    MSIDATABASE *db;
    INT version;
    enum platform platform;
    UINT num_langids;
    LANGID *langids;
    void *cookie;
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
    LPWSTR LastActionTemplate;
    UINT   LastActionResult;
    UINT   action_progress_increment;
    HANDLE log_file;
    HMODULE hfusion10;
    HMODULE hfusion11;
    HMODULE hfusion20;
    HMODULE hfusion40;
    HMODULE hmscoree;
    HRESULT (WINAPI *pGetFileVersion)( const WCHAR *, WCHAR *, DWORD, DWORD * );
    HRESULT (WINAPI *pCreateAssemblyNameObject)( IAssemblyName **, const WCHAR *, DWORD, void * );
    HRESULT (WINAPI *pCreateAssemblyEnum)( IAssemblyEnum **, IUnknown *, IAssemblyName *, DWORD, void * );
    IAssemblyCache *cache_net[CLR_VERSION_MAX];
    IAssemblyCache *cache_sxs;

    struct list classes;
    struct list extensions;
    struct list progids;
    struct list mimes;
    struct list appids;

    enum script script;
    LPWSTR *script_actions[SCRIPT_MAX];
    int    script_actions_count[SCRIPT_MAX];
    LPWSTR *unique_actions;
    int    unique_actions_count;
    BOOL   ExecuteSequenceRun;
    UINT   InWhatSequence;

    struct list RunningActions;

    HANDLE custom_server_32_process;
    HANDLE custom_server_64_process;
    HANDLE custom_server_32_pipe;
    HANDLE custom_server_64_pipe;

    LPWSTR PackagePath;
    LPWSTR ProductCode;
    LPWSTR localfile;
    BOOL delete_on_close;

    INSTALLUILEVEL ui_level;
    msi_dialog *dialog;
    LPWSTR next_dialog;
    float center_x;
    float center_y;

    UINT WordCount;
    MSIINSTALLCONTEXT Context;

    struct list subscriptions;

    struct list sourcelist_info;
    struct list sourcelist_media;

    unsigned char scheduled_action_running : 1;
    unsigned char commit_action_running : 1;
    unsigned char rollback_action_running : 1;
    unsigned char need_reboot_at_end : 1;
    unsigned char need_reboot_now : 1;
    unsigned char need_rollback : 1;
    unsigned char rpc_server_started : 1;
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
    /* Cost is in 512-byte units, as returned from MsiEnumComponentCosts() et al. */
    int cost;
    INT  RefCount;
    LPWSTR FullKeypath;
    LPWSTR AdvertiseString;
    MSIASSEMBLY *assembly;
    int num_clients;

    unsigned int anyAbsent:1;
    unsigned int hasAdvertisedFeature:1;
    unsigned int hasLocalFeature:1;
    unsigned int hasSourceFeature:1;
    unsigned int added:1;
    unsigned int updated:1;
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

#define SEQUENCE_UI       0x1
#define SEQUENCE_EXEC     0x2

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
#define SQUASHED_GUID_SIZE 33

#define MSIHANDLE_MAGIC 0x4d434923

/* handle unicode/ansi output in the Msi* API functions */
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

UINT msi_strcpy_to_awstring(const WCHAR *, int, awstring *, DWORD *);

/* msi server interface */
extern MSIHANDLE msi_get_remote(MSIHANDLE handle);
extern LONG WINAPI rpc_filter(EXCEPTION_POINTERS *eptr);

/* handle functions */
extern void *msihandle2msiinfo(MSIHANDLE handle, UINT type);
extern MSIHANDLE alloc_msihandle( MSIOBJECTHDR * );
extern MSIHANDLE alloc_msi_remote_handle(MSIHANDLE remote);
extern void *alloc_msiobject(UINT type, UINT size, msihandledestructor destroy );
extern void msiobj_addref(MSIOBJECTHDR *);
extern int msiobj_release(MSIOBJECTHDR *);
extern void msiobj_lock(MSIOBJECTHDR *);
extern void msiobj_unlock(MSIOBJECTHDR *);
extern void msi_free_handle_table(void);

extern void free_cached_tables( MSIDATABASE *db );
extern UINT MSI_CommitTables( MSIDATABASE *db );
extern UINT msi_commit_streams( MSIDATABASE *db );


/* string table functions */
extern BOOL msi_add_string( string_table *st, const WCHAR *data, int len, BOOL persistent );
extern UINT msi_string2id( const string_table *st, const WCHAR *data, int len, UINT *id );
extern VOID msi_destroy_stringtable( string_table *st );
extern const WCHAR *msi_string_lookup( const string_table *st, UINT id, int *len );
extern HRESULT msi_init_string_table( IStorage *stg );
extern string_table *msi_load_string_table( IStorage *stg, UINT *bytes_per_strref );
extern UINT msi_save_string_table( const string_table *st, IStorage *storage, UINT *bytes_per_strref );
extern UINT msi_get_string_table_codepage( const string_table *st );
extern UINT msi_set_string_table_codepage( string_table *st, UINT codepage );
extern WCHAR *msi_strdupW( const WCHAR *value, int len );

extern BOOL TABLE_Exists( MSIDATABASE *db, LPCWSTR name );
extern MSICONDITION MSI_DatabaseIsTablePersistent( MSIDATABASE *db, LPCWSTR table );

extern UINT read_stream_data( IStorage *stg, LPCWSTR stname, BOOL table,
                              BYTE **pdata, UINT *psz );
extern UINT write_stream_data( IStorage *stg, LPCWSTR stname,
                               LPCVOID data, UINT sz, BOOL bTable );

/* transform functions */
extern UINT msi_table_apply_transform( MSIDATABASE *db, IStorage *stg, int err_cond );
extern UINT MSI_DatabaseApplyTransformW( MSIDATABASE *db,
                 LPCWSTR szTransformFile, int iErrorCond );
extern void append_storage_to_db( MSIDATABASE *db, IStorage *stg );
extern UINT msi_apply_transforms( MSIPACKAGE *package );

/* patch functions */
extern UINT msi_check_patch_applicable( MSIPACKAGE *package, MSISUMMARYINFO *si );
extern UINT msi_apply_patches( MSIPACKAGE *package );
extern UINT msi_apply_registered_patch( MSIPACKAGE *package, LPCWSTR patch_code );
extern void msi_free_patchinfo( MSIPATCHINFO *patch );
extern UINT msi_patch_assembly( MSIPACKAGE *, MSIASSEMBLY *, MSIFILEPATCH * );

/* action internals */
extern UINT MSI_InstallPackage( MSIPACKAGE *, LPCWSTR, LPCWSTR );
extern INT ACTION_ShowDialog( MSIPACKAGE*, LPCWSTR);
extern INT ACTION_DialogBox( MSIPACKAGE*, LPCWSTR);
extern UINT ACTION_ForceReboot(MSIPACKAGE *package);
extern UINT MSI_Sequence( MSIPACKAGE *package, LPCWSTR szTable );
extern UINT MSI_SetFeatureStates( MSIPACKAGE *package );
extern UINT msi_parse_command_line( MSIPACKAGE *package, LPCWSTR szCommandLine, BOOL preserve_case );
extern const WCHAR *msi_get_command_line_option( const WCHAR *cmd, const WCHAR *option, UINT *len );
extern UINT msi_schedule_action( MSIPACKAGE *package, UINT script, const WCHAR *action );
extern INSTALLSTATE msi_get_component_action( MSIPACKAGE *package, MSICOMPONENT *comp );
extern INSTALLSTATE msi_get_feature_action( MSIPACKAGE *package, MSIFEATURE *feature );
extern UINT msi_load_all_components( MSIPACKAGE *package );
extern UINT msi_load_all_features( MSIPACKAGE *package );
extern UINT msi_validate_product_id( MSIPACKAGE *package );

/* record internals */
extern void MSI_CloseRecord( MSIOBJECTHDR * );
extern UINT MSI_RecordSetIStream( MSIRECORD *, UINT, IStream *);
extern UINT MSI_RecordGetIStream( MSIRECORD *, UINT, IStream **);
extern const WCHAR *MSI_RecordGetString( const MSIRECORD *, UINT );
extern MSIRECORD *MSI_CreateRecord( UINT );
extern UINT MSI_RecordSetInteger( MSIRECORD *, UINT, int );
extern UINT MSI_RecordSetStringW( MSIRECORD *, UINT, LPCWSTR );
extern BOOL MSI_RecordIsNull( MSIRECORD *, UINT );
extern UINT MSI_RecordGetStringW( MSIRECORD * , UINT, LPWSTR, LPDWORD);
extern UINT MSI_RecordGetStringA( MSIRECORD *, UINT, LPSTR, LPDWORD);
extern int MSI_RecordGetInteger( MSIRECORD *, UINT );
extern UINT MSI_RecordReadStream( MSIRECORD *, UINT, char *, LPDWORD);
extern UINT MSI_RecordSetStream(MSIRECORD *, UINT, IStream *);
extern UINT MSI_RecordGetFieldCount( const MSIRECORD *rec );
extern UINT MSI_RecordStreamToFile( MSIRECORD *, UINT, LPCWSTR );
extern UINT MSI_RecordSetStreamFromFileW( MSIRECORD *, UINT, LPCWSTR );
extern UINT MSI_RecordCopyField( MSIRECORD *, UINT, MSIRECORD *, UINT );
extern MSIRECORD *MSI_CloneRecord( MSIRECORD * );
extern BOOL MSI_RecordsAreEqual( MSIRECORD *, MSIRECORD * );
extern BOOL MSI_RecordsAreFieldsEqual(MSIRECORD *a, MSIRECORD *b, UINT field);
extern UINT msi_record_set_string(MSIRECORD *, UINT, const WCHAR *, int);
extern const WCHAR *msi_record_get_string(const MSIRECORD *, UINT, int *);
extern void dump_record(MSIRECORD *);
extern UINT unmarshal_record(const struct wire_record *in, MSIHANDLE *out);
extern struct wire_record *marshal_record(MSIHANDLE handle);
extern void free_remote_record(struct wire_record *rec);
extern UINT copy_remote_record(const struct wire_record *rec, MSIHANDLE handle);

/* stream internals */
extern void enum_stream_names( IStorage *stg );
extern WCHAR *encode_streamname(BOOL is_table, const WCHAR *in);
extern BOOL decode_streamname(LPCWSTR in, LPWSTR out);

/* database internals */
extern UINT msi_get_stream( MSIDATABASE *, const WCHAR *, IStream ** );
extern UINT MSI_OpenDatabaseW( LPCWSTR, LPCWSTR, MSIDATABASE ** );
extern UINT MSI_DatabaseOpenViewW(MSIDATABASE *, LPCWSTR, MSIQUERY ** );
extern UINT WINAPIV MSI_OpenQuery( MSIDATABASE *, MSIQUERY **, LPCWSTR, ... );
typedef UINT (*record_func)( MSIRECORD *, LPVOID );
extern UINT MSI_IterateRecords( MSIQUERY *, LPDWORD, record_func, LPVOID );
extern MSIRECORD * WINAPIV MSI_QueryGetRecord( MSIDATABASE *db, LPCWSTR query, ... );
extern UINT MSI_DatabaseGetPrimaryKeys( MSIDATABASE *, LPCWSTR, MSIRECORD ** );

/* view internals */
extern UINT MSI_ViewExecute( MSIQUERY*, MSIRECORD * );
extern UINT MSI_ViewFetch( MSIQUERY*, MSIRECORD ** );
extern UINT MSI_ViewClose( MSIQUERY* );
extern UINT MSI_ViewGetColumnInfo(MSIQUERY *, MSICOLINFO, MSIRECORD **);
extern UINT MSI_ViewModify( MSIQUERY *, MSIMODIFY, MSIRECORD * );
extern UINT VIEW_find_column( MSIVIEW *, LPCWSTR, LPCWSTR, UINT * );
extern UINT msi_view_get_row(MSIDATABASE *, MSIVIEW *, UINT, MSIRECORD **);

/* install internals */
extern UINT MSI_SetInstallLevel( MSIPACKAGE *package, int iInstallLevel );

/* package internals */
#define WINE_OPENPACKAGEFLAGS_RECACHE 0x80000000
extern MSIPACKAGE *MSI_CreatePackage( MSIDATABASE * );
extern UINT MSI_OpenPackageW( LPCWSTR szPackage, DWORD dwOptions, MSIPACKAGE **pPackage );
extern UINT MSI_SetTargetPathW( MSIPACKAGE *, LPCWSTR, LPCWSTR );
extern INT MSI_ProcessMessageVerbatim( MSIPACKAGE *, INSTALLMESSAGE, MSIRECORD * );
extern INT MSI_ProcessMessage( MSIPACKAGE *, INSTALLMESSAGE, MSIRECORD * );
extern MSICONDITION MSI_EvaluateConditionW( MSIPACKAGE *, LPCWSTR );
extern UINT MSI_GetComponentStateW( MSIPACKAGE *, LPCWSTR, INSTALLSTATE *, INSTALLSTATE * );
extern UINT MSI_GetFeatureStateW( MSIPACKAGE *, LPCWSTR, INSTALLSTATE *, INSTALLSTATE * );
extern UINT MSI_SetFeatureStateW(MSIPACKAGE*, LPCWSTR, INSTALLSTATE );
extern UINT msi_download_file( LPCWSTR szUrl, LPWSTR filename );
extern UINT msi_package_add_info(MSIPACKAGE *, DWORD, DWORD, LPCWSTR, LPWSTR);
extern UINT msi_package_add_media_disk(MSIPACKAGE *, DWORD, DWORD, DWORD, LPWSTR, LPWSTR);
extern UINT msi_clone_properties(MSIDATABASE *);
extern UINT msi_set_context(MSIPACKAGE *);
extern void msi_adjust_privilege_properties(MSIPACKAGE *);
extern UINT MSI_GetFeatureCost(MSIPACKAGE *, MSIFEATURE *, MSICOSTTREE, INSTALLSTATE, LPINT);

/* for deformating */
extern UINT MSI_FormatRecordW( MSIPACKAGE *, MSIRECORD *, LPWSTR, LPDWORD );

/* registry data encoding/decoding functions */
extern BOOL unsquash_guid(LPCWSTR in, LPWSTR out);
extern BOOL squash_guid(LPCWSTR in, LPWSTR out);
extern BOOL encode_base85_guid(GUID *,LPWSTR);
extern BOOL decode_base85_guid(LPCWSTR,GUID*);
extern UINT MSIREG_OpenUninstallKey(const WCHAR *, enum platform, HKEY *, BOOL);
extern UINT MSIREG_DeleteUninstallKey(const WCHAR *, enum platform);
extern UINT MSIREG_OpenProductKey(LPCWSTR szProduct, LPCWSTR szUserSid,
                                  MSIINSTALLCONTEXT context, HKEY* key, BOOL create);
extern UINT MSIREG_OpenFeaturesKey(LPCWSTR szProduct, LPCWSTR szUserSid, MSIINSTALLCONTEXT context,
                                   HKEY *key, BOOL create);
extern UINT MSIREG_OpenUserPatchesKey(LPCWSTR szPatch, HKEY* key, BOOL create);
UINT MSIREG_OpenUserDataFeaturesKey(LPCWSTR szProduct, LPCWSTR szUserSid, MSIINSTALLCONTEXT context,
                                    HKEY *key, BOOL create);
extern UINT MSIREG_OpenUserComponentsKey(LPCWSTR szComponent, HKEY* key, BOOL create);
extern UINT MSIREG_OpenUserDataComponentKey(LPCWSTR szComponent, LPCWSTR szUserSid,
                                            HKEY *key, BOOL create);
extern UINT MSIREG_OpenPatchesKey(LPCWSTR szPatch, HKEY* key, BOOL create);
extern UINT MSIREG_OpenUserDataProductKey(LPCWSTR szProduct, MSIINSTALLCONTEXT dwContext,
                                          LPCWSTR szUserSid, HKEY *key, BOOL create);
extern UINT MSIREG_OpenUserDataPatchKey(LPCWSTR szPatch, MSIINSTALLCONTEXT dwContext,
                                        HKEY *key, BOOL create);
extern UINT MSIREG_OpenUserDataProductPatchesKey(LPCWSTR product, MSIINSTALLCONTEXT context,
                                                 HKEY *key, BOOL create);
extern UINT MSIREG_OpenInstallProps(LPCWSTR szProduct, MSIINSTALLCONTEXT dwContext,
                                    LPCWSTR szUserSid, HKEY *key, BOOL create);
extern UINT MSIREG_OpenUpgradeCodesKey(LPCWSTR szProduct, HKEY* key, BOOL create);
extern UINT MSIREG_OpenUserUpgradeCodesKey(LPCWSTR szProduct, HKEY* key, BOOL create);
extern UINT MSIREG_DeleteProductKey(LPCWSTR szProduct);
extern UINT MSIREG_DeleteUserProductKey(LPCWSTR szProduct);
extern UINT MSIREG_DeleteUserDataPatchKey(LPCWSTR patch, MSIINSTALLCONTEXT context);
extern UINT MSIREG_DeleteUserDataProductKey(LPCWSTR, MSIINSTALLCONTEXT);
extern UINT MSIREG_DeleteUserFeaturesKey(LPCWSTR szProduct);
extern UINT MSIREG_DeleteUserDataComponentKey(LPCWSTR szComponent, LPCWSTR szUserSid);
extern UINT MSIREG_DeleteUserUpgradeCodesKey(LPCWSTR szUpgradeCode);
extern UINT MSIREG_DeleteUpgradeCodesKey(const WCHAR *);
extern UINT MSIREG_DeleteClassesUpgradeCodesKey(LPCWSTR szUpgradeCode);
extern UINT MSIREG_OpenClassesUpgradeCodesKey(LPCWSTR szUpgradeCode, HKEY* key, BOOL create);
extern UINT MSIREG_DeleteLocalClassesProductKey(LPCWSTR szProductCode);
extern UINT MSIREG_DeleteLocalClassesFeaturesKey(LPCWSTR szProductCode);
extern UINT msi_locate_product(LPCWSTR szProduct, MSIINSTALLCONTEXT *context);
extern WCHAR *msi_reg_get_val_str( HKEY hkey, const WCHAR *name );
extern BOOL msi_reg_get_val_dword( HKEY hkey, LPCWSTR name, DWORD *val);

extern DWORD msi_version_str_to_dword(LPCWSTR p);
extern void msi_parse_version_string(LPCWSTR, PDWORD, PDWORD);
extern int msi_compare_file_versions(VS_FIXEDFILEINFO *, const WCHAR *);
extern int msi_compare_font_versions(const WCHAR *, const WCHAR *);

extern LONG msi_reg_set_val_str( HKEY hkey, LPCWSTR name, LPCWSTR value );
extern LONG msi_reg_set_val_multi_str( HKEY hkey, LPCWSTR name, LPCWSTR value );
extern LONG msi_reg_set_val_dword( HKEY hkey, LPCWSTR name, DWORD val );
extern LONG msi_reg_set_subkey_val( HKEY hkey, LPCWSTR path, LPCWSTR name, LPCWSTR val );

/* msi dialog interface */
extern void msi_dialog_check_messages( HANDLE );
extern void msi_dialog_destroy( msi_dialog* );
extern void msi_dialog_unregister_class( void );

/* summary information */
extern UINT msi_get_suminfo( IStorage *stg, UINT uiUpdateCount, MSISUMMARYINFO **si );
extern UINT msi_get_db_suminfo( MSIDATABASE *db, UINT uiUpdateCount, MSISUMMARYINFO **si );
extern WCHAR *msi_suminfo_dup_string( MSISUMMARYINFO *si,
                                      UINT property );
extern INT msi_suminfo_get_int32( MSISUMMARYINFO *si, UINT uiProperty );
extern WCHAR *msi_get_suminfo_product( IStorage *stg );
extern UINT msi_add_suminfo( MSIDATABASE *db, LPWSTR **records, int num_records, int num_columns );
extern UINT msi_export_suminfo( MSIDATABASE *db, HANDLE handle );
extern UINT msi_load_suminfo_properties( MSIPACKAGE *package );

/* undocumented functions */
UINT WINAPI MsiCreateAndVerifyInstallerDirectory( DWORD );
UINT WINAPI MsiDecomposeDescriptorW( LPCWSTR, LPWSTR, LPWSTR, LPWSTR, LPDWORD );
UINT WINAPI MsiDecomposeDescriptorA( LPCSTR, LPSTR, LPSTR, LPSTR, LPDWORD );
LANGID WINAPI MsiLoadStringW( MSIHANDLE, UINT, LPWSTR, int, LANGID );
LANGID WINAPI MsiLoadStringA( MSIHANDLE, UINT, LPSTR, int, LANGID );

/* UI globals */
extern INSTALLUILEVEL gUILevel;
extern HWND gUIhwnd;
extern INSTALLUI_HANDLERA gUIHandlerA;
extern INSTALLUI_HANDLERW gUIHandlerW;
extern INSTALLUI_HANDLER_RECORD gUIHandlerRecord;
extern DWORD gUIFilter;
extern DWORD gUIFilterRecord;
extern LPVOID gUIContext;
extern LPVOID gUIContextRecord;
extern WCHAR *gszLogFile;
extern HINSTANCE msi_hInstance;

/* action related functions */
extern UINT ACTION_PerformAction(MSIPACKAGE *package, const WCHAR *action);
extern void ACTION_FinishCustomActions( const MSIPACKAGE* package);
extern UINT ACTION_CustomAction(MSIPACKAGE *package, const WCHAR *action);
extern void custom_stop_server(HANDLE process, HANDLE pipe);

/* actions in other modules */
extern UINT ACTION_AppSearch(MSIPACKAGE *package);
extern UINT ACTION_CCPSearch(MSIPACKAGE *package);
extern UINT ACTION_FindRelatedProducts(MSIPACKAGE *package);
extern UINT ACTION_InstallFiles(MSIPACKAGE *package);
extern UINT ACTION_PatchFiles( MSIPACKAGE *package );
extern UINT ACTION_RemoveFiles(MSIPACKAGE *package);
extern UINT ACTION_MoveFiles(MSIPACKAGE *package);
extern UINT ACTION_DuplicateFiles(MSIPACKAGE *package);
extern UINT ACTION_RemoveDuplicateFiles(MSIPACKAGE *package);
extern UINT ACTION_RegisterClassInfo(MSIPACKAGE *package);
extern UINT ACTION_RegisterProgIdInfo(MSIPACKAGE *package);
extern UINT ACTION_RegisterExtensionInfo(MSIPACKAGE *package);
extern UINT ACTION_RegisterMIMEInfo(MSIPACKAGE *package);
extern UINT ACTION_RegisterFonts(MSIPACKAGE *package);
extern UINT ACTION_UnregisterClassInfo(MSIPACKAGE *package);
extern UINT ACTION_UnregisterExtensionInfo(MSIPACKAGE *package);
extern UINT ACTION_UnregisterFonts(MSIPACKAGE *package);
extern UINT ACTION_UnregisterMIMEInfo(MSIPACKAGE *package);
extern UINT ACTION_UnregisterProgIdInfo(MSIPACKAGE *package);
extern UINT ACTION_MsiPublishAssemblies(MSIPACKAGE *package);
extern UINT ACTION_MsiUnpublishAssemblies(MSIPACKAGE *package);

/* Helpers */
extern DWORD deformat_string(MSIPACKAGE *package, LPCWSTR ptr, WCHAR** data );
extern WCHAR *msi_dup_record_field( MSIRECORD *row, INT index );
extern WCHAR *msi_dup_property( MSIDATABASE *db, const WCHAR *prop );
extern UINT msi_set_property( MSIDATABASE *, const WCHAR *, const WCHAR *, int );
extern UINT msi_get_property( MSIDATABASE *, LPCWSTR, LPWSTR, LPDWORD );
extern int msi_get_property_int( MSIDATABASE *package, LPCWSTR prop, int def );
extern WCHAR *msi_resolve_source_folder(MSIPACKAGE *package, const WCHAR *name,
                                        MSIFOLDER **folder);
extern void msi_resolve_target_folder(MSIPACKAGE *package, const WCHAR *name, BOOL load_prop);
extern WCHAR *msi_normalize_path(const WCHAR *);
extern WCHAR *msi_resolve_file_source(MSIPACKAGE *package,
                                      MSIFILE *file);
extern const WCHAR *msi_get_target_folder(MSIPACKAGE *package, const WCHAR *name);
extern void msi_reset_source_folders( MSIPACKAGE *package );
extern MSICOMPONENT *msi_get_loaded_component(MSIPACKAGE *package, const WCHAR *Component);
extern MSIFEATURE *msi_get_loaded_feature(MSIPACKAGE *package, const WCHAR *Feature);
extern MSIFILE *msi_get_loaded_file(MSIPACKAGE *package, const WCHAR *file);
extern MSIFOLDER *msi_get_loaded_folder(MSIPACKAGE *package, const WCHAR *dir);
extern WCHAR *msi_create_temp_file(MSIDATABASE *db);
extern void msi_free_action_script(MSIPACKAGE *package, UINT script);
extern WCHAR *msi_build_icon_path(MSIPACKAGE *, const WCHAR *);
extern WCHAR * WINAPIV msi_build_directory_name(DWORD , ...);
extern void msi_reduce_to_long_filename(WCHAR *);
extern WCHAR *msi_create_component_advertise_string(MSIPACKAGE *, MSICOMPONENT *,
                                                    const WCHAR *);
extern void ACTION_UpdateComponentStates(MSIPACKAGE *package, MSIFEATURE *feature);
extern UINT msi_register_unique_action(MSIPACKAGE *, const WCHAR *);
extern BOOL msi_action_is_unique(const MSIPACKAGE *, const WCHAR *);
extern UINT msi_set_last_used_source(LPCWSTR product, LPCWSTR usersid,
                        MSIINSTALLCONTEXT context, DWORD options, LPCWSTR value);
extern UINT msi_create_empty_local_file(LPWSTR path, LPCWSTR suffix);
extern UINT msi_set_sourcedir_props(MSIPACKAGE *package, BOOL replace);
extern MSIASSEMBLY *msi_load_assembly(MSIPACKAGE *, MSICOMPONENT *);
extern UINT msi_install_assembly(MSIPACKAGE *, MSICOMPONENT *);
extern UINT msi_uninstall_assembly(MSIPACKAGE *, MSICOMPONENT *);
extern void msi_destroy_assembly_caches(MSIPACKAGE *);
extern BOOL msi_is_global_assembly(MSICOMPONENT *);
extern IAssemblyEnum *msi_create_assembly_enum(MSIPACKAGE *, const WCHAR *);
extern WCHAR *msi_get_assembly_path(MSIPACKAGE *, const WCHAR *);
extern WCHAR **msi_split_string(const WCHAR *, WCHAR);
extern UINT msi_set_original_database_property(MSIDATABASE *, const WCHAR *);
extern WCHAR *msi_get_error_message(MSIDATABASE *, int);
extern UINT msi_strncpyWtoA(const WCHAR *str, int len, char *buf, DWORD *sz, BOOL remote);
extern UINT msi_strncpyW(const WCHAR *str, int len, WCHAR *buf, DWORD *sz);
extern WCHAR *msi_get_package_code(MSIDATABASE *db);

/* wrappers for filesystem functions */
static inline void msi_disable_fs_redirection( MSIPACKAGE *package )
{
    if (is_wow64 && package->platform == PLATFORM_X64) Wow64DisableWow64FsRedirection( &package->cookie );
}
static inline void msi_revert_fs_redirection( MSIPACKAGE *package )
{
    if (is_wow64 && package->platform == PLATFORM_X64) Wow64RevertWow64FsRedirection( package->cookie );
}
extern BOOL msi_get_temp_file_name( MSIPACKAGE *, const WCHAR *, const WCHAR *, WCHAR * );
extern HANDLE msi_create_file( MSIPACKAGE *, const WCHAR *, DWORD, DWORD, DWORD, DWORD );
extern BOOL msi_delete_file( MSIPACKAGE *, const WCHAR * );
extern BOOL msi_remove_directory( MSIPACKAGE *, const WCHAR * );
extern DWORD msi_get_file_attributes( MSIPACKAGE *, const WCHAR * );
extern BOOL msi_set_file_attributes( MSIPACKAGE *, const WCHAR *, DWORD );
extern HANDLE msi_find_first_file( MSIPACKAGE *, const WCHAR *, WIN32_FIND_DATAW * );
extern BOOL msi_find_next_file( MSIPACKAGE *, HANDLE, WIN32_FIND_DATAW * );
extern BOOL msi_move_file( MSIPACKAGE *, const WCHAR *, const WCHAR *, DWORD );
extern DWORD msi_get_file_version_info( MSIPACKAGE *, const WCHAR *, DWORD, BYTE * );
extern BOOL msi_create_full_path( MSIPACKAGE *, const WCHAR * );
extern DWORD msi_get_disk_file_size( MSIPACKAGE *, const WCHAR * );
extern VS_FIXEDFILEINFO *msi_get_disk_file_version( MSIPACKAGE *, const WCHAR * );
extern UINT msi_get_filehash( MSIPACKAGE *, const WCHAR *, MSIFILEHASHINFO * );
extern WCHAR *msi_get_font_file_version( MSIPACKAGE *,
                                         const WCHAR * );

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

extern UINT ready_media(MSIPACKAGE *package, BOOL compressed, MSIMEDIAINFO *mi);
extern UINT msi_load_media_info(MSIPACKAGE *package, UINT Sequence, MSIMEDIAINFO *mi);
extern void msi_free_media_info(MSIMEDIAINFO *mi);
extern BOOL msi_cabextract(MSIPACKAGE* package, MSIMEDIAINFO *mi, LPVOID data);
extern UINT msi_add_cabinet_stream(MSIPACKAGE *, UINT, IStorage *, const WCHAR *);

/* control event stuff */
extern void msi_event_fire(MSIPACKAGE *, const WCHAR *, MSIRECORD *);
extern void msi_event_cleanup_all_subscriptions( MSIPACKAGE * );

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

extern HRESULT create_msiserver(IUnknown *pOuter, LPVOID *ppObj);
extern HRESULT create_session(MSIHANDLE msiHandle, IDispatch *pInstaller, IDispatch **pDispatch);
extern HRESULT get_typeinfo(tid_t tid, ITypeInfo **ti);
extern void release_typelib(void);

/* Scripting */
extern DWORD call_script(MSIHANDLE hPackage, INT type, LPCWSTR script, LPCWSTR function, LPCWSTR action);

/* User interface messages from the actions */
extern void msi_ui_progress(MSIPACKAGE *, int, int, int, int);

static inline char *strdupWtoA( LPCWSTR str )
{
    LPSTR ret = NULL;
    DWORD len;

    if (!str) return ret;
    len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    ret = malloc( len );
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
    ret = malloc( len * sizeof(WCHAR) );
    if (ret)
        MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    return ret;
}

static inline char *strdupWtoU( LPCWSTR str )
{
    LPSTR ret = NULL;
    DWORD len;

    if (!str) return ret;
    len = WideCharToMultiByte( CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    ret = malloc( len );
    if (ret)
        WideCharToMultiByte( CP_UTF8, 0, str, -1, ret, len, NULL, NULL );
    return ret;
}

static inline LPWSTR strdupUtoW( LPCSTR str )
{
    LPWSTR ret = NULL;
    DWORD len;

    if (!str) return ret;
    len = MultiByteToWideChar( CP_UTF8, 0, str, -1, NULL, 0 );
    ret = malloc( len * sizeof(WCHAR) );
    if (ret)
        MultiByteToWideChar( CP_UTF8, 0, str, -1, ret, len );
    return ret;
}

static inline int cost_from_size( int size )
{
    /* Cost is size rounded up to the nearest 4096 bytes,
     * expressed in units of 512 bytes. */
    return ((size + 4095) & ~4095) / 512;
}

#endif /* __WINE_MSI_PRIVATE__ */
