/*
*
*
*  Facility:
*
*    SNMP Extension Agent
*
*  Abstract:
*  
*    This module contains support functions for the creation and
*    maintenance of the in-memory cache for the HostMIB Subagent.
*
*
*  Author:
*
*    D. D. Burns @ WebEnable, Inc.
*
*
*  Revision History:
*
*    V1.00 - 04/17/97  D. D. Burns     Original Creation
*
*
*/


/*
Host-MIB Cache Overview
-----------------------

This module contains functions that create, maintain and allow for searching
data structures that are used to implement a cache of HOST-MIB information.

Typically, a cache is created on a "per-table" basis and is formed as a
linked-list of CACHEROW structures (all cache structures are defined in
"HMCACHE.H"), one CACHEROW structure for each logical "row" in the table.

The list-head of each cache is a CACHEHEAD structure and is instantiated in
the source module for the functions that service the attributes in that
table (so the CACHEHEAD structure for the cache for the "hrStorage" table is
in module "HRSTOENT.C").

Caches are created at start-up time by special cache-creation functions (coded
to the specs for each table) in each of the "table" source modules.  Those
cache-creation functions (plus the associated "get" and "set" functions) use
the general cache manipulation functions in this module.

For example, a typical cache looks like this:

      HrStorage Table Cache
        "hrStorage_cache"
      (statically allocated
         in "HRSTOENT.C")...

       *============*
       |  CACHEHEAD |
       |  "list"....|--*          ..(Malloced as a single instance in function
       *============*  |          .  "CreateTableRow")
                       V          .
                      *================*                ..(Malloced as an
                      |    CACHEROW    |                . array in function
                   *--|...."next"      |                . "CreateTableRow()")
                   |  |    "index".....|--> "1"         .
                   |  | "attrib_list"..|--> *===============*
                   |  *================*    |    ATTRIB     |
                   |                        | "attrib_type".|-->CA_NUMBER
                   |                        | "u.unumber"...|-->"4"
                   |                        +---------------+
                   |                        |    ATTRIB     |
                   |                        | "attrib_type".|-->CA_STRING
                   |                        | "u.string"....|-->"<string>"
                   |                        +---------------+
                   |                        |    ATTRIB     |
                   |                        | "attrib_type".|-->CA_CACHE
                   |                        |   "u.cache"...|------*
                   |                        +---------------+      |
                   |                                .              |
                   |  *=================*           .              |
                   *->|    CACHEROW     |           .              |
                   *--|...."next"       |                   *============*
                   |  |    "index"......|-->"2"             |  CACHEHEAD |
                   |  |  "attrib_list"..|-->                |  "list"....|--*
                   |  *=================*                   *============*  |
                   V                                         (For doubly    |
                                                              indexed       V
                                                              tables)

The general cache manipulation functions in this module include:

Name                    Purpose
----                    -------
CreateTableRow          Creates an instance of a CACHEROW with a given
                        attribute count.  (This function does not link the
                        instance into any list, it merely mallocs storage).

AddTableRow             Given an index value, a CACHEROW instance (created by
                        "CreateTableRow()" above) and a CACHEHEAD, this
                        function links the CACHEROW instance into the list
                        described by CACHEHEAD in the proper place given the
                        index value.

         These two functions above are used to populate the cache 
         (typically at startup time).

Name                    Purpose
----                    -------
FindTableRow            Given an index value and a CACHEHEAD, this function
                        returns a pointer to the CACHEROW instance in the
                        CACHEHEAD cache that has the given index.  This 
                        function is used to find a given cache entry (ie table
                        "row") in service for a "get" or "set" routine.

FindNextTableRow        Given an index value and a CACHEHEAD, this function
                        returns a pointer to the CACHEROW instance in the
                        CACHEHEAD cache that IMMEDIATELY FOLLOWS the given
                        index.  This function is used to find a given cache
                        entry (ie table "row") in service for "get-next"
                        situations.

GetNextTableRow         Given a CACHEROW row (obtained using either of the
                        routines above), this gets the next entry regardless
                        of index (or NULL if the given row is the last row).
                        (Implemented as a macro in "HMCACHE.H").

RemoveTableRow          Given an index value and a CACHEHEAD, this function
                        unlinks the CACHEROW instance from the cache list
                        described by CACHEHEAD. (TBD in support of PNP)

DestroyTable            Given a pointer to a CACHEHEAD, this function releases
                        every row-instance in the cache (through calls to
                        "DestroyTableRow()" below).  This function presumes
                        that the CACHEHEAD itself is statically allocated.

DestroyTableRow         Given an instance of an (unlinked) CACHEROW, this
                        function releases the storage associated with it.


For Debugging:
Name                    Purpose
----                    -------
PrintCache              Prints a dump on an output file in ASCII of the 
                        contents of a specified cache.  Only works for caches
                        for which a "print-row" function is defined and
                        referenced in the CACHEHEAD structure for the cache.
*/



/*
| INCLUDES:
*/
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>      /* for debug printf */
#include <time.h>       /* for debug support */
#include <malloc.h>

#include "hmcache.h"

/*
|==============================================================================
| Debug File Channel
|
*/
#if defined(CACHE_DUMP) || defined(PROC_CACHE)
FILE *Ofile;
#endif




/* CreateTableRow - Create a CACHEROW structure for attributes in a table */
/* CreateTableRow - Create a CACHEROW structure for attributes in a table */
/* CreateTableRow - Create a CACHEROW structure for attributes in a table */

CACHEROW *
CreateTableRow(
               ULONG attribute_count
              )

/*
|  EXPLICIT INPUTS:
|
|       "attribute_count" indicates how much storage to allocate for the
|       array of attributes for this row.
|
|  IMPLICIT INPUTS:
|
|       None.
|
|  OUTPUTS:
|
|     On Success:
|       Function returns a pointer to allocated storage containing an
|       image of a CACHEROW structure.  Enough storage for each attribute
|       in the row has been allocated within array "attrib_list[]" and the
|       count of these elements has been stored in the CACHEROW structure.
|
|     On any Failure:
|       Function returns NULL (indicating "not enough storage").
|
|  THE BIG PICTURE:
|
|       At subagent startup time, the caches for each table in the MIB is
|       populated with rows for each row in the table.  This function is
|       invoked by the start-up code for each table to create one CACHEROW
|       structure for each row needed in the table.
|
|       With the advent of PNP, this function can be called after startup
|       time to add rows to an existing table.
|
|       (Note: The actual insertion of an instance returned by this function
|              into a cache table list is done by function "AddTableRow()" ).
|
|  OTHER THINGS TO KNOW:
|
|       "DestroyTableRow()" deallocates storage associated with any instance
|       of a CACHEROW structure created by this function.
|
*/
{
CACHEROW        *new=NULL;      /* New CACHEROW instance to be created */
ULONG           i;              /* handy-dandy Index                   */


/* Create the main CACHEROW structure to be returned . . . */
if ( (new = (CACHEROW *) malloc(sizeof(CACHEROW))) == NULL) {

    /* "Not Enough Storage" */
    return (NULL);
    }

/*
| Now try to allocate enough storage for the array of attributes in this row
*/
if ( (new->attrib_list = (ATTRIB *) malloc(sizeof(ATTRIB) * attribute_count))
    == NULL) {

    /* "Not Enough Storage" */
    free( new ) ;       /* Blow off the CACHEROW we won't be returning */
    return (NULL);
    }

/* Indicate how big this array is so DestroyTableRow() can do the right thing*/
new->attrib_count = attribute_count;

/* Zap each array entry so things are clean */
for (i = 0; i < attribute_count; i += 1) {
    new->attrib_list[i].attrib_type = CA_UNKNOWN;
    new->attrib_list[i].u.string_value = NULL;
    }


new->index = 0;         /* No legal index yet        */
new->next = NULL;       /* Not in the cache list yet */


/* Return the newly allocated CACHEROW structure for further population */
return ( new ) ;
}

/* DestroyTable - Destroy all rows in CACHEHEAD structure (Release Storage) */
/* DestroyTable - Destroy all rows in CACHEHEAD structure (Release Storage) */
/* DestroyTable - Destroy all rows in CACHEHEAD structure (Release Storage) */

void
DestroyTable(
             CACHEHEAD *cache   /* Cache whose rows are to be Released */
             )

/*
|  EXPLICIT INPUTS:
|
|       "cache" is the CACHEHEAD instance of a table for which all rows are
|       to be released.
|
|  IMPLICIT INPUTS:
|
|       None.
|
|  OUTPUTS:
|
|     On Success/Failure:
|       Function returns, the CACHEHEAD is set to reflect an empty cache.
|
|  THE BIG PICTURE:
|
|       At subagent startup time, the caches for each table in the MIB is
|       populated with rows for each row in the table.  "CreateTableRow" is
|       invoked by the start-up code for each table to create one CACHEROW
|       structure for each row needed in the table.
|
|       With the advent of PNP, this function can be called after startup
|       time to delete all storage associated with such a cache.
|
|
|  OTHER THINGS TO KNOW:
|
|       This function may be recursively invoked through the call to
|       "DestroyTable()" inside "DestroyTableRow()".
|
|       This function may be safely invoked on an "empty" cache-head.
|
|       This function doesn't attempt to release storage associated with
|       the CACHEHEAD structure itself.
|
|  DOUBLE NOTE:
|       This function simply releases storage.  You can't go calling this
|       function willy-nilly on just any cache without taking into
|       consideration the semantics of what maybe being blown away. For
|       instance, some tables in Host MIB contain attributes whose values
|       are indices into other tables.  If a table is destroyed and rebuilt,
|       clearly the references to the rebuilt table must be refreshed in
|       some manner.
*/
{

/*
| If an old copy of the cache exists, blow it away now
*/
while (cache->list != NULL) {

    CACHEROW    *row_to_go;

    /* Pick up the row to blow away */
    row_to_go = cache->list;

    /* Change the cache-head to point to the next row (if any) */
    cache->list = GetNextTableRow(row_to_go);

    DestroyTableRow(row_to_go);
    }

/* Show no entries in the cache */
cache->list_count = 0;
}

/* DestroyTableRow - Destroy a CACHEROW structure (Release Storage) */
/* DestroyTableRow - Destroy a CACHEROW structure (Release Storage) */
/* DestroyTableRow - Destroy a CACHEROW structure (Release Storage) */

void
DestroyTableRow(
                CACHEROW *row   /* Row to be Released */
                )

/*
|  EXPLICIT INPUTS:
|
|       "row" is the instance of a row to be released.
|
|  IMPLICIT INPUTS:
|
|       None.
|
|  OUTPUTS:
|
|     On Success/Failure:
|       Function returns.
|
|  THE BIG PICTURE:
|
|       At subagent startup time, the caches for each table in the MIB is
|       populated with rows for each row in the table.  "CreateTableRow" is
|       invoked by the start-up code for each table to create one CACHEROW
|       structure for each row needed in the table.
|
|       With the advent of PNP, this function can be called after startup
|       time to delete storage associated with rows being replaced in an
|       existing table.
|
|       (Note: The actual deletion of an row instance from a cache table list
|              must be done before this function is called).
|
|  OTHER THINGS TO KNOW:
|
|       "CreateTableRow()" creates an instance of what this function
|       "destroys".
|
|       This function may be recursively invoked through the call to
|       "DestroyTable()" in the event we are release a row that contains
|       an attribute "value" that is really another table (in the case
|       of a multiply-indexed attribute).
|
|  DOUBLE NOTE:
|       This function simply releases storage.  You can't go calling this
|       function willy-nilly on just any cache without taking into
|       consideration the semantics of what maybe being blown away. For
|       instance, some tables in Host MIB contain attributes whose values
|       are indices into other tables.  If a row is destroyed, clearly the
|       references to the row must be refreshed in some manner.
*/
{
CACHEROW        *new=NULL;      /* New CACHEROW instance to be created */
ULONG           i;              /* handy-dandy Index                   */


/* Zap storage associated each attribute entry (if any) */
for (i = 0; i < row->attrib_count; i += 1) {

    /* Blow off storage for attribute values that have malloc-ed storage */
    switch (row->attrib_list[i].attrib_type) {

        case CA_STRING:
            free( row->attrib_list[i].u.string_value );
            break;


        case CA_CACHE:
            /* Release the contents of the entire cache */
            DestroyTable( row->attrib_list[i].u.cache );

            /* Free the storage containing the cache
            free ( row->attrib_list[i].u.cache );
            break;

        
        case CA_NUMBER:
        case CA_COMPUTED:
        case CA_UNKNOWN:
            /* No malloced storage associated with these types */
        default:
           break;
        }
    }


/* Free the storage associated with the array of attributes */
free( row->attrib_list);

/* Free the storage for the row itself */
free( row );
}

/* AddTableRow - Adds a specific "row" into a cached "table" */
/* AddTableRow - Adds a specific "row" into a cached "table" */
/* AddTableRow - Adds a specific "row" into a cached "table" */

BOOL
AddTableRow(
             ULONG      index,          /* Index for row desired */
             CACHEROW   *row,           /* Row to be added to .. */
             CACHEHEAD  *cache          /* this cache            */
              )

/*
|  EXPLICIT INPUTS:
|
|       "index" is index inserted into "row" before the
|       "row" is added to "cache".
|
|  IMPLICIT INPUTS:
|
|       None.
|
|  OUTPUTS:
|
|     On Success:
|       Function returns TRUE indicating that the row was successfully
|       added to the cache for the table.
|
|     On Failure:
|       Function returns FALSE, indicating that the row already
|       existed.
|
|  THE BIG PICTURE:
|
|       At startup time the subagent is busy populating the cache for
|       each table.  The rows in any cached table are inserted into
|       the cache by this function.
|
|  OTHER THINGS TO KNOW:
|
|       Code in this function presumes that the list (cache) is in sorted index
|       order.
|
|       Any change of organization of the linked list that constitutes the
|       cache will impact this function and "Find(Next)TableRow()".
|
*/
{
CACHEROW       **index_row;     /* Used for searching cache              */
                                /* NOTE: It always points at a cell that */
                                /*       points to the next list element */
                                /*       (if any is on the list).        */


/* Whip down the list until there is no "next" or "next" is "bigger" . . . */
for ( index_row = &cache->list;
      *index_row != NULL;
      index_row = &((*index_row)->next)
     ) {

    /* If this row MATCHES the to-be-inserted row: Error! */
    if ((*index_row)->index == index) {
        return ( FALSE );
        }

    /*
    | If next cache entry is "Greater Than" new index, then
    | "index_row" points to the cell that should be changed to insert
    | the new entry.
    */
    if ((*index_row)->index > index) {
        break;
        }

    /* Otherwise we should try for a "next" entry in the list */
    }


/*
| When we fall thru here "index_row" contains the address of the cell to
| change to add the new row into the cache (might be in the list-head, 
| might be in a list-entry)
*/
row->next = *index_row;   /* Put cache-list "next" into new row element */
*index_row = row;         /* Insert new row into the list               */

row->index = index;       /* Stick the index into the row entry itself  */

cache->list_count += 1;   /* Count another entry on the cache list      */


/* Successful insertion */
return (TRUE);
}

/* FindTableRow - Finds a specific "row" in a cached "table" */
/* FindTableRow - Finds a specific "row" in a cached "table" */
/* FindTableRow - Finds a specific "row" in a cached "table" */

CACHEROW *
FindTableRow(
             ULONG      index,          /* Index for row desired */
             CACHEHEAD  *cache          /* Table cache to search */
              )

/*
|  EXPLICIT INPUTS:
|
|       "index" indicates which table row entry is desired
|       "cache" indicates the cache list to search for the desired row.
|
|  IMPLICIT INPUTS:
|
|       None.
|
|  OUTPUTS:
|
|     On Success:
|       Function returns a pointer to the instance of a CACHEROW structure
|       for the desired row.
|
|     On any Failure:
|       Function returns NULL (indicating "no such entry" or "cache empty").
|
|  THE BIG PICTURE:
|
|       As the subagent runs, the "get" functions for the attributes that do
|       not "compute" their values dynamically must lookup cached values.
|
|       This function can be used by any "get" function that knows the
|       CACHEHEAD for it's table to find a specific row containing an
|       attribute value to be returned.
|
|  OTHER THINGS TO KNOW:
|
|       Code in this function presumes that the list is in sorted index
|       order (hence it gives up once encountering an entry whose index
|       is "too big").
|
|       Any change of organization of the linked list that constitutes the
|       cache will impact this function and "AddTableRow()".
|
*/
{
CACHEROW        *row=NULL;   /* Row instance to be returned, initially none */


/* Whip down the list until there is no next . . */
for ( row = cache->list; row != NULL; row = row->next ) {

    /* If this is "It": Return it */
    if (row->index == index) {
        return ( row );
        }

    /* If this is "Greater Than IT", it's not in the list: Return NULL */
    if (row->index > index) {
        return ( NULL );
        }

    /* Otherwise we should try for a "next" entry in the list */
    }


/*
| If we fall thru here we didn't find the desired entry because the cache
| list is either empty or devoid of the desired row.
*/
return (NULL);

}

/* FindNextTableRow - Finds Next row after a given "index" in a cache */
/* FindNextTableRow - Finds Next row after a given "index" in a cache */
/* FindNextTableRow - Finds Next row after a given "index" in a cache */

CACHEROW *
FindNextTableRow(
                 ULONG      index,          /* Index for row desired */
                 CACHEHEAD  *cache          /* Table cache to search */
                 )

/*
|  EXPLICIT INPUTS:
|
|       "index" indicates which table row entry AFTER WHICH the NEXT is
|               desired.  The "index" row need not exist (could be before
|               the first row or a missing row "in the middle", but
|               it may not specify a row that would be after the
|               last in the table.
|
|       "cache" indicates the cache list to search for the desired row.
|
|  IMPLICIT INPUTS:
|
|       None.
|
|  OUTPUTS:
|
|     On Success:
|       Function returns a pointer to the instance of a CACHEROW structure
|       for the desired NEXT row.
|
|     On any Failure:
|       Function returns NULL (indicating "no such entry", "cache empty" or
|                              "end-of-cache" reached).
|
|  THE BIG PICTURE:
|
|       As the subagent runs, the "get-next" functions for the attributes 
|       that do not "compute" their values dynamically must lookup cached 
|       values.
|
|       This function can be used by any "FindNextInstance" function that
|       knows the CACHEHEAD for it's table to find the NEXT row following a
|       specific row containing an attribute value to be returned.
|
|  OTHER THINGS TO KNOW:
|
|       To get the first entry in a table, supply an ("illegal") index of 0.
|
|       Any change of organization of the linked list that constitutes the
|       cache will impact this function, "FindTableRow()" and "AddTableRow()".
|
*/
{
CACHEROW        *row=NULL;   /* Row instance to be returned, initially none */


/*
| If there is a non-empty cache and the input "index" is less than
| the first entry in the cache, simply return the first entry.
*/
if (   cache->list != NULL         /* If there is a non-empty cache . . . */
    && index < cache->list->index  /* AND index is LESS THAN first entry  */
    ) {

    /* Return the first entry in the table */
    return (cache->list);
    }

/* Whip down the list until there is no next . . */
for ( row = cache->list; row != NULL; row = row->next ) {

    /* If "index" specifies THIS ROW . . . */
    if (row->index == index) {
        return ( row->next );   /* Return NEXT (or NULL if no "next") */
        }

    /* If this is "Greater Than IT", "index" is not in the list */
    if (row->index > index) {
        return ( row  );        /* Return CURRENT, it is Greater than "index"*/
        }

    /* Otherwise we should try for a "next" entry in the list */
    }

/*
| If we fall thru here then the cache is empty or the "index" specifies
| a row after the last legal row.
*/
return (NULL);
}


#if defined(CACHE_DUMP)

/* PrintCache - Dumps for debugging the contents of a cache */
/* PrintCache - Dumps for debugging the contents of a cache */
/* PrintCache - Dumps for debugging the contents of a cache */

void
PrintCache(
           CACHEHEAD  *cache          /* Table cache to dump */
           )

/*
|  EXPLICIT INPUTS:
|
|       "cache" indicates the cache whose contents is to be dumped.
|
|  IMPLICIT INPUTS:
|
|       None.
|
|  OUTPUTS:
|
|     On Success:
|       Function returns.  It may be called recursively by a "Print-Row"
|       function.
|
|
|  THE BIG PICTURE:
|
|     For debugging only.
|
|  OTHER THINGS TO KNOW:
|
|     Define "CACHE_DUMP" at the top of "HMCACHE.H" to enable this debug
|     support.  You can change the file into which output goes by modifying
|     "DUMP_FILE", also in "HMCACHE.H".
|
*/

#define DO_CLOSE  \
   { if ((open_count -= 1) == 0) { fclose(OFILE); } }

{
CACHEROW        *row;                   /* Row instance to dump. */
UINT            i;                      /* Element counter       */
time_t          ltime;                  /* For debug message     */
static
UINT            open_count=0;         /* We can be called recursively */

/* Avoid a recursive open */
if (open_count == 0) {

    /* Open the debug log file */
    if ((Ofile=fopen(DUMP_FILE, "a+")) == NULL) {
        return;
        }

    /*
    | Put a time stamp into the debug file because we're opening for append.
    */
    time( &ltime);
    fprintf(OFILE, "=============== Open for appending: %s\n", ctime( &ltime ));
    }

open_count += 1;

if (cache == NULL) {
    fprintf(OFILE, "Call to PrintCache with NULL CACHEHEAD pointer.\n");

    DO_CLOSE;
    return;
    }

if (cache->print_row == NULL) {
    fprintf(OFILE,
            "Call to PrintCache with NULL CACHEHEAD Print-Routine pointer.\n");

    DO_CLOSE;
    return;
    }

/* Print a Title */
cache->print_row(NULL);

fprintf(OFILE, "Element Count: %d\n", cache->list_count);

/* For every row in the cache . . . */
for (row = cache->list, i = 0; row != NULL; row = row->next, i += 1) {

    fprintf(OFILE, "\nElement #%d, Internal Index %d,  at 0x%x:\n",
            i, row->index, row);

    cache->print_row(row);
    }

fprintf(OFILE, "======== End of Cache ========\n\n");

DO_CLOSE;

}

#endif  // defined(CACHE_DUMP)
