/*
 *  File HMCACHE.H
 *
 *  Facility:
 *
 *    Windows NT SNMP Extension Agent
 *
 *  Abstract:
 *
 *    This module is contains definitions pertaining to the HostMIB
 *    cacheing mechanism.
 *
 *  Author:
 *
 *    D. D. Burns @ WebEnable, Inc.
 *
 *
 *  Revision History:
 *
 *    V1.0 - 04/17/97  D. D. Burns     Original Creation
 */

#ifndef hmcache_h
#define hmcache_h


/*
|==============================================================================
| Debug Cache Dump Support
|
|    Define "CACHE_DUMP" to get function "PrintCache" defined to enable
|    debug-dumping of any cache for which a debug_print function is defined
|    (in a CACHEHEAD_INSTANCE() macro that defines the list-head of the cache).
|
|    Define "DUMP_FILE" to specify where the dump file is generated.  All
|    opens on this file are for "append", so you must explicitly delete the
|    file if you want to start fresh.  All entries are time-stamped, so any
|    confusion is your own, though.
|
|    NOTE 1: After defining "CACHE_DUMP", grep the sources to see what caches
|            "PrintCache()" may be being invoked on and where.  Typically
|            all caches are dumped immediately after they are built.  The
|            cache for hrSWRun(Perf) table is re-built if it is older than
|            "CACHE_MAX_AGE" (defined in "HRSWRUNE.C") when a request for
|            something in the tables served by the cache comes in.  So it
|            may also be dumped after each (re-)build.
|
|    NOTE 2: Define "PROC_CACHE" to get a periodic dump to "PROC_FILE" of
|            the hrProcessorLoad-specific cache.  This dump occurs on a
|            1-minute timer and will rapidly use up disk space if left
|            running for any long period.  This cache and dump is special
|            to the "hrProcessorLoad" variable in hrProcessor sub-table.
|            (Opens on this file are also for "append").
*/
//#define CACHE_DUMP 1
#define DUMP_FILE \
    "c:\\nt\\private\\net\\snmp\\subagent\\hostmib\\HostMib_Cache.dmp"

//#define PROC_CACHE 1
#define PROC_FILE \
    "c:\\nt\\private\\net\\snmp\\subagent\\hostmib\\Processor_Cache.dmp"


#if defined(CACHE_DUMP) || defined(PROC_CACHE)
#include <stdio.h>
#include <time.h>
#endif

/*
|==============================================================================
| hrStorage Attribute Defines
|
|    Each attribute defined for hrStorage table is associated with one of the
|    #defines below.  These symbols are used as C indices into the list of
|    attributes within a cached-row.
|
|    These symbols are globally accessible so that logic that builds hrFSTable
|    can "peek" at values stored in the hrStorageTable cache.
*/
#define HRST_INDEX 0    // hrStorageIndex
#define HRST_TYPE  1    // hrStorageType
#define HRST_DESCR 2    // hrStorageDescr
#define HRST_ALLOC 3    // hrStorageAllocationUnits
#define HRST_SIZE  4    // hrStorageSize
#define HRST_USED  5    // hrStorageUsed
#define HRST_FAILS 6    // hrStorageAllocationFailures
                   //-->Add more here, change count below!
#define HRST_ATTRIB_COUNT 7


/*
|==============================================================================
| hrFSTable Attribute Defines
|
|    Each attribute defined for hrFSTable is associated with one of the
|    #defines below.  These symbols are used as C indices into the array of
|    attributes within a cached-row.
|
|    These symbols are globally accessible so that logic that builds 
|    hrPartition can "peek" at values stored in the hrFSEntry Table cache.
*/
#define HRFS_INDEX    0    // hrFSIndex
#define HRFS_MOUNTPT  1    // hrFSMountPoint
#define HRFS_RMOUNTPT 2    // hrFSRemoteMountPoint
#define HRFS_TYPE     3    // hrFSType
#define HRFS_ACCESS   4    // hrFSAccess
#define HRFS_BOOTABLE 5    // hrFSBootable
#define HRFS_STORINDX 6    // hrFSStorageIndex
#define HRFS_LASTFULL 7    // hrFSLastFullBackupDate
#define HRFS_LASTPART 8    // hrFSLastPartialBackupDate
                      //-->Add more here, change count below!
#define HRFS_ATTRIB_COUNT 9


/*
|==============================================================================
| hrSWRun(Perf) Table Attribute Defines
|
|    Each attribute defined for hrSWRun Table and hrSWRunPerf Table is 
|    associated with one of the #defines below.  These symbols are used as 
|    C indices into the array of attributes within a cached-row.
|
|    These symbols are globally accessible so that logic for hrSWRunPerf table
|    (in "HRSWPREN.C") can reference these as well as logic for hrSWRun table
|    (in "HRSWRUNE.C") since both tables share the same cache.
|
|    Note that "HrSWRunID" is not cached.
*/
#define HRSR_INDEX    0    // HrSWRunIndex
#define HRSR_NAME     1    // HrSWRunName
#define HRSR_PATH     2    // HrSWRunPath
#define HRSR_PARAM    3    // HRSWRunParameters
#define HRSR_TYPE     4    // HrSWRunType
#define HRSR_STATUS   5    // HrSWRunStatus
#define HRSP_CPU      6    // HrSWRunPerfCPU - Performance
#define HRSP_MEM      7    // HrSWRunPerfMem - Performance
                      //-->Add more here, change count below!
#define HRSR_ATTRIB_COUNT 8


/*
|==============================================================================
| These structures are used in the implementation of an in-memory cache
| for the Host Resources MIB subagent.  For a broad overview of the cache
| scheme, see the documentation at the front of "HMCACHE.C".
|==============================================================================
*/


/*
|==============================================================================
| ATTRIB_TYPE
|
|     This enumerated type lists the data-type of a value of an attribute
|     stored in an instance of a ATTRIB structure (one of typically many
|     in a CACHEROW structure for a given table row).
|
|     "CA_STRING"
|       The value is a null-terminated string sitting in 'malloc'ed storage
|       within an ATTRIB structure.
|
|     "CA_NUMBER"
|       The value is a binary numeric value stored directly in the ATTRIB
|       structure.  No additional malloc storage is associated with this
|       type.
|
|     "CA_COMPUTED"
|       The value is not stored in the ATTRIB structure at all, but is
|       dynamic and is computed and returned by the support subagent
|       "get" function.
|
|     "CA_CACHE"
|       The value is a pointer to a CACHEHEAD structure that describes
|       another cache.  The CACHEHEAD structure is in 'malloc'ed storage.
|       This is used for "multiply-indexed" tables.
|
| Note that the instance of this enumerated type (in ATTRIB below) is mainly
| of use in debugging and memory management (when we get to the point where 
| cached-rows may be freed).  Generally the "Get" function that is going to
| reach into the cache is already going to be coded according to what is
| there, and may not even look to see what "type" the value is.
|==============================================================================
*/
typedef
    enum {
        CA_UNKNOWN,     /* Not yet set               */
        CA_STRING,      /* ('malloc'ed storage)      */
        CA_NUMBER,      /* (no 'malloc'ed storage)   */
        CA_COMPUTED,    /* (no 'malloc'ed storage)   */
        CA_CACHE        /* ('malloc'ed storage)      */
        } ATTRIB_TYPE;



/*
|==============================================================================
| ATTRIB
|
|     An array of these structures is logically allocated inside each 
|     instance of a CACHEROW structure.
|
|     An instance of this structure describes the value of one attribute
|     in the cache (in general; in the "CA_COMPUTED" case there is no value
|     present, the GET function "knows" what to do).
|==============================================================================
*/
typedef
    struct {

        ATTRIB_TYPE     attrib_type;    /* STRING, NUMBER, (COMPUTED) */

        union {
            LPSTR       string_value;   /* CA_STRING (malloc)   */
            ULONG       unumber_value;  /* CA_NUMBER (unsigned) */
            LONG        number_value;   /* CA_NUMBER (signed)   */
            void       *cache;          /* CA_CACHE  (malloc)   */
            } u;
            
        } ATTRIB;



/*
|==============================================================================
| CACHEROW
|
|     An instance of this structure occurs for each row in a table.  Instances
|     are strung on a list maintained by a CACHEHEAD structure (below), ordered
|     by the value of "index".
|
|     The "attrib_list[]" array storage is malloc'ed to the appropriate size
|     at the time an instance of this structure is created.  The indices into
|     this array are #DEFINED symbols, all according to the table definition
|     of the attributes in the table.  Typically the #defines are placed in
|     the source module that implements the table.
|
|     The internal arrangement of this structure (and underlying structures)
|     is meant to be such that function "DestroyTableRow()" can release all
|     storage for an instance of this structure without "knowing" the #DEFINE
|     index symbols above (or anything else).
|==============================================================================
*/
typedef
    struct rc_tag{

        ULONG           index;          /* SNMP index of table              */
        struct rc_tag   *next;          /* Next in the cache list           */

        /*
        | Contents of this table row:
        */
        ULONG           attrib_count;   /* # of elements in "attrib_list"[] */
        ATTRIB         *attrib_list;    /* --> array of attributes          */

        } CACHEROW;



/*
|==============================================================================
| CACHEHEAD
|
|     An instance of this structure (created by the macro CACHEHEAD_INSTANCE)
|     occurs for each SNMP "table" cached.
|
|     All CACHEROW elements of the list are ordered by their index values
|     as they are inserted by general function "AddTableRow()".
|
|     See documentation in "HMCACHE.C".
|
|NOTE: If you modify this structure or the macro that initializes static
|      instances of it, be sure to add/change code in "HRPARTIT.C" where
|      instances in dynamic (malloc) memory are created.
|==============================================================================
*/
typedef
    struct {

        ULONG           list_count;     /* (Mainly for ease of debugging) */
        CACHEROW        *list;          /* The row list itself            */
        void            (*print_row)(); /* Debug Print-A-Row function     */
        } CACHEHEAD;

#if defined(CACHE_DUMP)
#define CACHEHEAD_INSTANCE(name,debug_print)       \
        CACHEHEAD  name={ 0, NULL, debug_print };
#else
#define CACHEHEAD_INSTANCE(name,debug_print)       \
        CACHEHEAD  name={ 0, NULL, NULL };
#endif


/*
|==============================================================================
| HMCACHE.C - Function Prototypes
*/

/* CreateTableRow - Create a CACHEROW structure for attributes in a table */
CACHEROW *
CreateTableRow(
               ULONG attribute_count
              );

/* DestroyTable - Destroy all rows in CACHEHEAD structure (Release Storage) */
void
DestroyTable(
             CACHEHEAD *cache   /* Cache whose rows are to be Released */
             );

/* DestroyTableRow - Destroy a CACHEROW structure (Release Storage) */
void
DestroyTableRow(
                CACHEROW *row   /* Row to be Released */
                );

/* AddTableRow - Adds a specific "row" into a cached "table" */
BOOL
AddTableRow(
             ULONG      index,          /* Index for row desired */
             CACHEROW   *row,           /* Row to be added to .. */
             CACHEHEAD  *cache          /* this cache            */
              );

/* FindTableRow - Finds a specific "row" in a cached "table" */
CACHEROW *
FindTableRow(
             ULONG      index,          /* Index for row desired */
             CACHEHEAD  *cache          /* Table cache to search */
              );

/* FindNextTableRow - Finds Next row after a specific "row" in a cache */
CACHEROW *
FindNextTableRow(
                 ULONG      index,          /* Index for row desired */
                 CACHEHEAD  *cache          /* Table cache to search */
                 );

/* GetNextTableRow - Gets Next row after a specific "row" or NULL if none */
#define GetNextTableRow(row) row->next


/* ======  DEBUG DUMP SUPPORT ====== */
#if defined(CACHE_DUMP)
/* PrintCache - Dumps for debugging the contents of a cache */
void
PrintCache(
           CACHEHEAD  *cache          /* Table cache to dump */
           );

/* Debug Print Output channel used by PrintCache &  "Print-A-Row" functions*/
#define OFILE Ofile
extern FILE *Ofile;
#endif


/*
|==============================================================================
| Function Prototypes for cache-related function found in other modules:
*/

/* Gen_Hrstorage_Cache - Generate a initial cache for HrStorage Table */
BOOL Gen_Hrstorage_Cache( void );       /* "HRSTOENT.C"                      */
extern CACHEHEAD hrStorage_cache;       /* This cache is globally accessible */

/* Gen_HrFSTable_Cache - Generate a initial cache for HrFSTable */
BOOL Gen_HrFSTable_Cache( void );       /* "HRFSENTR.C"                      */
extern CACHEHEAD hrFSTable_cache;       /* This cache is globally accessible */

/* Gen_HrDevice_Cache - Generate a initial cache for HrDevice Table */
BOOL Gen_HrDevice_Cache( void );        /* "HRDEVENT.C"                      */
extern CACHEHEAD hrDevice_cache;        /* This cache is globally accessible */
extern ULONG InitLoadDev_index;         /* From hrDevice for hrSystem        */

/* Gen_HrSWInstalled_Cache - Generate a cache for HrSWInstalled Table        */
BOOL Gen_HrSWInstalled_Cache( void );   /* "HESWINEN.C"                      */

/* Gen_HrSWRun_Cache - Generate a initial cache for HrSWRun(Perf) Table      */
BOOL Gen_HrSWRun_Cache( void );         /* "HRSWRUNE.C"                      */
extern CACHEHEAD hrSWRunTable_cache;    /* Globally accessible for ref from  */
                                        /* "HRSWPREN.C"                      */
extern ULONG SWOSIndex;                 /* From "HRSWRUNE.C" for "HRSWRUN.C" */

/* hrSWRunCache_Refresh - hrSWRun(Perf) Cache Refresh-Check Routine */
BOOL hrSWRunCache_Refresh( void );

#endif /* hmcache_h */
