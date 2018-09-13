/*++

Copyright (C) 1993 - 1996 Microsoft Corporation

Module Name:

    winperf.h

Abstract:

    Header file for the Performance Monitor data.

    This file contains the definitions of the data structures returned
    by the Configuration Registry in response to a request for
    performance data.  This file is used by both the Configuration
    Registry and the Performance Monitor to define their interface.
    The complete interface is described here, except for the name
    of the node to query in the registry.  It is

                   HKEY_PERFORMANCE_DATA.

    By querying that node with a subkey of "Global" the caller will
    retrieve the structures described here.

    There is no need to RegOpenKey() the reserved handle HKEY_PERFORMANCE_DATA,
    but the caller should RegCloseKey() the handle so that network transports
    and drivers can be removed or installed (which cannot happen while
    they are open for monitoring.)  Remote requests must first
    RegConnectRegistry().

--*/

#ifndef _WINPERF_
#define _WINPERF_

//  Data structure definitions.

//  In order for data to be returned through the Configuration Registry
//  in a system-independent fashion, it must be self-describing.

//  In the following, all offsets are in bytes.

//
//  Data is returned through the Configuration Registry in a
//  a data block which begins with a _PERF_DATA_BLOCK structure.
//

#define PERF_DATA_VERSION   1
#define PERF_DATA_REVISION  1


typedef struct _PERF_DATA_BLOCK {
    WCHAR           Signature[4];       // Signature: Unicode "PERF"
    DWORD           LittleEndian;       // 0 = Big Endian, 1 = Little Endian
    DWORD           Version;            // Version of these data structures
                                        // starting at 1
    DWORD           Revision;           // Revision of these data structures
                                        // starting at 0 for each Version
    DWORD           TotalByteLength;    // Total length of data block
    DWORD           HeaderLength;       // Length of this structure
    DWORD           NumObjectTypes;     // Number of types of objects
                                        // being reported
    LONG            DefaultObject;      // Object Title Index of default
                                        // object to display when data from
                                        // this system is retrieved (-1 =
                                        // none, but this is not expected to
                                        // be used)
    SYSTEMTIME      SystemTime;         // Time at the system under
                                        // measurement
    LARGE_INTEGER   PerfTime;           // Performance counter value
                                        // at the system under measurement
    LARGE_INTEGER   PerfFreq;           // Performance counter frequency
                                        // at the system under measurement
    LARGE_INTEGER   PerfTime100nSec;    // Performance counter time in 100 nsec
                                        // units at the system under measurement
    DWORD           SystemNameLength;   // Length of the system name
    DWORD           SystemNameOffset;   // Offset, from beginning of this
                                        // structure, to name of system
                                        // being measured
} PERF_DATA_BLOCK, *PPERF_DATA_BLOCK;


//
//  The _PERF_DATA_BLOCK structure is followed by NumObjectTypes of
//  data sections, one for each type of object measured.  Each object
//  type section begins with a _PERF_OBJECT_TYPE structure.
//


typedef struct _PERF_OBJECT_TYPE {
    DWORD           TotalByteLength;    // Length of this object definition
                                        // including this structure, the
                                        // counter definitions, and the
                                        // instance definitions and the
                                        // counter blocks for each instance:
                                        // This is the offset from this
                                        // structure to the next object, if
                                        // any
    DWORD           DefinitionLength;   // Length of object definition,
                                        // which includes this structure
                                        // and the counter definition
                                        // structures for this object: this
                                        // is the offset of the first
                                        // instance or of the counters
                                        // for this object if there is
                                        // no instance
    DWORD           HeaderLength;       // Length of this structure: this
                                        // is the offset to the first
                                        // counter definition for this
                                        // object
    DWORD           ObjectNameTitleIndex;
                                        // Index to name in Title Database
    LPWSTR          ObjectNameTitle;    // Initially NULL, for use by
                                        // analysis program to point to
                                        // retrieved title string
    DWORD           ObjectHelpTitleIndex;
                                        // Index to Help in Title Database
    LPWSTR          ObjectHelpTitle;    // Initially NULL, for use by
                                        // analysis program to point to
                                        // retrieved title string
    DWORD           DetailLevel;        // Object level of detail (for
                                        // controlling display complexity);
                                        // will be min of detail levels
                                        // for all this object's counters
    DWORD           NumCounters;        // Number of counters in each
                                        // counter block (one counter
                                        // block per instance)
    LONG            DefaultCounter;     // Default counter to display when
                                        // this object is selected, index
                                        // starting at 0 (-1 = none, but
                                        // this is not expected to be used)
    LONG            NumInstances;       // Number of object instances
                                        // for which counters are being
                                        // returned from the system under
                                        // measurement. If the object defined
                                        // will never have any instance data
                                        // structures (PERF_INSTANCE_DEFINITION)
                                        // then this value should be -1, if the
                                        // object can have 0 or more instances,
                                        // but has none present, then this
                                        // should be 0, otherwise this field
                                        // contains the number of instances of
                                        // this counter.
    DWORD           CodePage;           // 0 if instance strings are in
                                        // UNICODE, else the Code Page of
                                        // the instance names
    LARGE_INTEGER   PerfTime;           // Sample Time in "Object" units
                                        //
    LARGE_INTEGER   PerfFreq;           // Frequency of "Object" units in
                                        // counts per second.
} PERF_OBJECT_TYPE, *PPERF_OBJECT_TYPE;

#define PERF_NO_INSTANCES           -1  // no instances (see NumInstances above)
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
//  PERF_COUNTER_DEFINITION.CounterType field values
//
//
//        Counter ID Field Definition:
//
//   3      2        2    2    2        1        1    1
//   1      8        4    2    0        6        2    0    8                0
//  +--------+--------+----+----+--------+--------+----+----+----------------+
//  |Display |Calculation  |Time|Counter |        |Ctr |Size|                |
//  |Flags   |Modifiers    |Base|SubType |Reserved|Type|Fld |   Reserved     |
//  +--------+--------+----+----+--------+--------+----+----+----------------+
//
//
//  The counter type is the "or" of the following values as described below
//
//  select one of the following to indicate the counter's data size
//
#define PERF_SIZE_DWORD         0x00000000
#define PERF_SIZE_LARGE         0x00000100
#define PERF_SIZE_ZERO          0x00000200  // for Zero Length fields
#define PERF_SIZE_VARIABLE_LEN  0x00000300  // length is in CounterLength field
                                            //  of Counter Definition struct
//
//  select one of the following values to indicate the counter field usage
//
#define PERF_TYPE_NUMBER        0x00000000  // a number (not a counter)
#define PERF_TYPE_COUNTER       0x00000400  // an increasing numeric value
#define PERF_TYPE_TEXT          0x00000800  // a text field
#define PERF_TYPE_ZERO          0x00000C00  // displays a zero
//
//  If the PERF_TYPE_NUMBER field was selected, then select one of the
//  following to describe the Number
//
#define PERF_NUMBER_HEX         0x00000000  // display as HEX value
#define PERF_NUMBER_DECIMAL     0x00010000  // display as a decimal integer
#define PERF_NUMBER_DEC_1000    0x00020000  // display as a decimal/1000
//
//  If the PERF_TYPE_COUNTER value was selected then select one of the
//  following to indicate the type of counter
//
#define PERF_COUNTER_VALUE      0x00000000  // display counter value
#define PERF_COUNTER_RATE       0x00010000  // divide ctr / delta time
#define PERF_COUNTER_FRACTION   0x00020000  // divide ctr / base
#define PERF_COUNTER_BASE       0x00030000  // base value used in fractions
#define PERF_COUNTER_ELAPSED    0x00040000  // subtract counter from current time
#define PERF_COUNTER_QUEUELEN   0x00050000  // Use Queuelen processing func.
#define PERF_COUNTER_HISTOGRAM  0x00060000  // Counter begins or ends a histogram
//
//  If the PERF_TYPE_TEXT value was selected, then select one of the
//  following to indicate the type of TEXT data.
//
#define PERF_TEXT_UNICODE       0x00000000  // type of text in text field
#define PERF_TEXT_ASCII         0x00010000  // ASCII using the CodePage field
//
//  Timer SubTypes
//
#define PERF_TIMER_TICK         0x00000000  // use system perf. freq for base
#define PERF_TIMER_100NS        0x00100000  // use 100 NS timer time base units
#define PERF_OBJECT_TIMER       0x00200000  // use the object timer freq
//
//  Any types that have calculations performed can use one or more of
//  the following calculation modification flags listed here
//
#define PERF_DELTA_COUNTER      0x00400000  // compute difference first
#define PERF_DELTA_BASE         0x00800000  // compute base diff as well
#define PERF_INVERSE_COUNTER    0x01000000  // show as 1.00-value (assumes:
#define PERF_MULTI_COUNTER      0x02000000  // sum of multiple instances
//
//  Select one of the following values to indicate the display suffix (if any)
//
#define PERF_DISPLAY_NO_SUFFIX  0x00000000  // no suffix
#define PERF_DISPLAY_PER_SEC    0x10000000  // "/sec"
#define PERF_DISPLAY_PERCENT    0x20000000  // "%"
#define PERF_DISPLAY_SECONDS    0x30000000  // "secs"
#define PERF_DISPLAY_NOSHOW     0x40000000  // value is not displayed
//
//  Predefined counter types
//

// 32-bit Counter.  Divide delta by delta time.  Display suffix: "/sec"
#define PERF_COUNTER_COUNTER        \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_PER_SEC)


// 64-bit Timer.  Divide delta by delta time.  Display suffix: "%"
#define PERF_COUNTER_TIMER          \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_PERCENT)

// Queue Length Space-Time Product. Divide delta by delta time. No Display Suffix.
#define PERF_COUNTER_QUEUELEN_TYPE  \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_QUEUELEN |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

// Queue Length Space-Time Product. Divide delta by delta time. No Display Suffix.
#define PERF_COUNTER_LARGE_QUEUELEN_TYPE  \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_QUEUELEN |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

// 64-bit Counter.  Divide delta by delta time. Display Suffix: "/sec"
#define PERF_COUNTER_BULK_COUNT     \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_PER_SEC)

// Indicates the counter is not a  counter but rather Unicode text Display as text.
#define PERF_COUNTER_TEXT           \
            (PERF_SIZE_VARIABLE_LEN | PERF_TYPE_TEXT | PERF_TEXT_UNICODE |\
            PERF_DISPLAY_NO_SUFFIX)

// Indicates the data is a counter  which should not be
// time averaged on display (such as an error counter on a serial line)
// Display as is.  No Display Suffix.
#define PERF_COUNTER_RAWCOUNT       \
            (PERF_SIZE_DWORD | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL |\
            PERF_DISPLAY_NO_SUFFIX)

// Same as PERF_COUNTER_RAWCOUNT except its size is a large integer
#define PERF_COUNTER_LARGE_RAWCOUNT       \
            (PERF_SIZE_LARGE | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL |\
            PERF_DISPLAY_NO_SUFFIX)

// Special case for RAWCOUNT that want to be displayed in hex
// Indicates the data is a counter  which should not be
// time averaged on display (such as an error counter on a serial line)
// Display as is.  No Display Suffix.
#define PERF_COUNTER_RAWCOUNT_HEX       \
            (PERF_SIZE_DWORD | PERF_TYPE_NUMBER | PERF_NUMBER_HEX |\
            PERF_DISPLAY_NO_SUFFIX)

// Same as PERF_COUNTER_RAWCOUNT_HEX except its size is a large integer
#define PERF_COUNTER_LARGE_RAWCOUNT_HEX       \
            (PERF_SIZE_LARGE | PERF_TYPE_NUMBER | PERF_NUMBER_HEX |\
            PERF_DISPLAY_NO_SUFFIX)


// A count which is either 1 or 0 on each sampling interrupt (% busy)
// Divide delta by delta base. Display Suffix: "%"
#define PERF_SAMPLE_FRACTION        \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |\
            PERF_DELTA_COUNTER | PERF_DELTA_BASE | PERF_DISPLAY_PERCENT)

// A count which is sampled on each sampling interrupt (queue length)
// Divide delta by delta time. No Display Suffix.
#define PERF_SAMPLE_COUNTER         \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

// A label: no data is associated with this counter (it has 0 length)
// Do not display.
#define PERF_COUNTER_NODATA         \
            (PERF_SIZE_ZERO | PERF_DISPLAY_NOSHOW)

// 64-bit Timer inverse (e.g., idle is measured, but display busy %)
// Display 100 - delta divided by delta time.  Display suffix: "%"
#define PERF_COUNTER_TIMER_INV      \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_INVERSE_COUNTER | \
            PERF_DISPLAY_PERCENT)

// The divisor for a sample, used with the previous counter to form a
// sampled %.  You must check for >0 before dividing by this!  This
// counter will directly follow the  numerator counter.  It should not
// be displayed to the user.
#define PERF_SAMPLE_BASE            \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
            PERF_DISPLAY_NOSHOW |\
            0x00000001)  // for compatibility with pre-beta versions

// A timer which, when divided by an average base, produces a time
// in seconds which is the average time of some operation.  This
// timer times total operations, and  the base is the number of opera-
// tions.  Display Suffix: "sec"
#define PERF_AVERAGE_TIMER          \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |\
            PERF_DISPLAY_SECONDS)

// Used as the denominator in the computation of time or count
// averages.  Must directly follow the numerator counter.  Not dis-
// played to the user.
#define PERF_AVERAGE_BASE           \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
            PERF_DISPLAY_NOSHOW |\
            0x00000002)  // for compatibility with pre-beta versions


// A bulk count which, when divided (typically) by the number of
// operations, gives (typically) the number of bytes per operation.
// No Display Suffix.
#define PERF_AVERAGE_BULK           \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION  |\
            PERF_DISPLAY_NOSHOW)

// 64-bit Timer in 100 nsec units. Display delta divided by
// delta time.  Display suffix: "%"
#define PERF_100NSEC_TIMER          \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_100NS | PERF_DELTA_COUNTER | PERF_DISPLAY_PERCENT)

// 64-bit Timer inverse (e.g., idle is measured, but display busy %)
// Display 100 - delta divided by delta time.  Display suffix: "%"
#define PERF_100NSEC_TIMER_INV      \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_TIMER_100NS | PERF_DELTA_COUNTER | PERF_INVERSE_COUNTER  |\
            PERF_DISPLAY_PERCENT)

// 64-bit Timer.  Divide delta by delta time.  Display suffix: "%"
// Timer for multiple instances, so result can exceed 100%.
#define PERF_COUNTER_MULTI_TIMER    \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_DELTA_COUNTER | PERF_TIMER_TICK | PERF_MULTI_COUNTER |\
            PERF_DISPLAY_PERCENT)

// 64-bit Timer inverse (e.g., idle is measured, but display busy %)
// Display 100 * _MULTI_BASE - delta divided by delta time.
// Display suffix: "%" Timer for multiple instances, so result
// can exceed 100%.  Followed by a counter of type _MULTI_BASE.
#define PERF_COUNTER_MULTI_TIMER_INV \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
            PERF_DELTA_COUNTER | PERF_MULTI_COUNTER | PERF_TIMER_TICK |\
            PERF_INVERSE_COUNTER | PERF_DISPLAY_PERCENT)

// Number of instances to which the preceding _MULTI_..._INV counter
// applies.  Used as a factor to get the percentage.
#define PERF_COUNTER_MULTI_BASE     \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
            PERF_MULTI_COUNTER | PERF_DISPLAY_NOSHOW)

// 64-bit Timer in 100 nsec units. Display delta divided by delta time.
// Display suffix: "%" Timer for multiple instances, so result can exceed 100%.
#define PERF_100NSEC_MULTI_TIMER   \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_DELTA_COUNTER  |\
            PERF_COUNTER_RATE | PERF_TIMER_100NS | PERF_MULTI_COUNTER |\
            PERF_DISPLAY_PERCENT)

// 64-bit Timer inverse (e.g., idle is measured, but display busy %)
// Display 100 * _MULTI_BASE - delta divided by delta time.
// Display suffix: "%" Timer for multiple instances, so result
// can exceed 100%.  Followed by a counter of type _MULTI_BASE.
#define PERF_100NSEC_MULTI_TIMER_INV \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_DELTA_COUNTER  |\
            PERF_COUNTER_RATE | PERF_TIMER_100NS | PERF_MULTI_COUNTER |\
            PERF_INVERSE_COUNTER | PERF_DISPLAY_PERCENT)

// Indicates the data is a fraction of the following counter  which
// should not be time averaged on display (such as free space over
// total space.) Display as is.  Display the quotient as "%".
#define PERF_RAW_FRACTION           \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |\
            PERF_DISPLAY_PERCENT)

// Indicates the data is a base for the preceding counter which should
// not be time averaged on display (such as free space over total space.)
#define PERF_RAW_BASE               \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
            PERF_DISPLAY_NOSHOW |\
            0x00000003)  // for compatibility with pre-beta versions

// The data collected in this counter is actually the start time of the
// item being measured. For display, this data is subtracted from the
// sample time to yield the elapsed time as the difference between the two.
// In the definition below, the PerfTime field of the Object contains
// the sample time as indicated by the PERF_OBJECT_TIMER bit and the
// difference is scaled by the PerfFreq of the Object to convert the time
// units into seconds.
#define PERF_ELAPSED_TIME           \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_ELAPSED |\
            PERF_OBJECT_TIMER | PERF_DISPLAY_SECONDS)
//
//  The following counter type can be used with the preceding types to
//  define a range of values to be displayed in a histogram.
//

#define PERF_COUNTER_HISTOGRAM_TYPE   0x80000000
                                        // Counter begins or ends a histogram
//
//  This counter is used to display the difference from one sample
//  to the next. The counter value is a constantly increasing number
//  and the value displayed is the difference between the current
//  value and the previous value. Negative numbers are not allowed
//  which shouldn't be a problem as long as the counter value is 
//  increasing or unchanged.
//
#define PERF_COUNTER_DELTA      \
            (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_VALUE |\
            PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

#define PERF_COUNTER_LARGE_DELTA      \
            (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_VALUE |\
            PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)
//
//  The following are used to determine the level of detail associated
//  with the counter.  The user will be setting the level of detail
//  that should be displayed at any given time.
//

#define PERF_DETAIL_NOVICE          100 // The uninformed can understand it
#define PERF_DETAIL_ADVANCED        200 // For the advanced user
#define PERF_DETAIL_EXPERT          300 // For the expert user
#define PERF_DETAIL_WIZARD          400 // For the system designer

//
//  There is one of the following for each of the
//  PERF_OBJECT_TYPE.NumCounters.  The Unicode names in this structure MUST
//  come from a message file.
//

typedef struct _PERF_COUNTER_DEFINITION {
    DWORD           ByteLength;         // Length in bytes of this structure
    DWORD           CounterNameTitleIndex;
                                        // Index of Counter name into
                                        // Title Database
    LPWSTR          CounterNameTitle;   // Initially NULL, for use by
                                        // analysis program to point to
                                        // retrieved title string
    DWORD           CounterHelpTitleIndex;
                                        // Index of Counter Help into
                                        // Title Database
    LPWSTR          CounterHelpTitle;   // Initially NULL, for use by
                                        // analysis program to point to
                                        // retrieved title string
    LONG            DefaultScale;       // Power of 10 by which to scale
                                        // chart line if vertical axis is 100
                                        // 0 ==> 1, 1 ==> 10, -1 ==>1/10, etc.
    DWORD           DetailLevel;        // Counter level of detail (for
                                        // controlling display complexity)
    DWORD           CounterType;        // Type of counter
    DWORD           CounterSize;        // Size of counter in bytes
    DWORD           CounterOffset;      // Offset from the start of the
                                        // PERF_COUNTER_BLOCK to the first
                                        // byte of this counter
} PERF_COUNTER_DEFINITION, *PPERF_COUNTER_DEFINITION;

//
//  If (PERF_DATA_BLOCK.NumInstances >= 0) then there will be
//  PERF_DATA_BLOCK.NumInstances of a (PERF_INSTANCE_DEFINITION
//  followed by a PERF_COUNTER_BLOCK followed by the counter data fields)
//  for each instance.
//
//  If (PERF_DATA_BLOCK.NumInstances < 0) then the counter definition
//  strucutre above will be followed by only a PERF_COUNTER_BLOCK and the
//  counter data for that COUNTER.
//

#define PERF_NO_UNIQUE_ID -1

typedef struct _PERF_INSTANCE_DEFINITION {
    DWORD           ByteLength;         // Length in bytes of this structure,
                                        // including the subsequent name
    DWORD           ParentObjectTitleIndex;
                                        // Title Index to name of "parent"
                                        // object (e.g., if thread, then
                                        // process is parent object type);
                                        // if logical drive, the physical
                                        // drive is parent object type
    DWORD           ParentObjectInstance;
                                        // Index to instance of parent object
                                        // type which is the parent of this
                                        // instance.
    LONG            UniqueID;           // A unique ID used instead of
                                        // matching the name to identify
                                        // this instance, -1 = none
    DWORD           NameOffset;         // Offset from beginning of
                                        // this struct to the Unicode name
                                        // of this instance
    DWORD           NameLength;         // Length in bytes of name; 0 = none
                                        // this length includes the characters
                                        // in the string plus the size of the 
                                        // terminating NULL char. It does not
                                        // include any additional pad bytes to
                                        // correct structure alignment
} PERF_INSTANCE_DEFINITION, *PPERF_INSTANCE_DEFINITION;
//
//  If .ParentObjectName is 0, there
//  is no parent-child hierarchy for this object type.  Otherwise,
//   the .ParentObjectInstance is an index, starting at 0, into the
//  instances reported for the parent object type.  It is only
//  meaningful if .ParentObjectName is not 0.  The purpose of all this
//  is to permit reporting/summation of object instances like threads
//  within processes, and logical drives within physical drives.
//
//
//  The PERF_INSTANCE_DEFINITION will be followed by a PERF_COUNTER_BLOCK.
//

typedef struct _PERF_COUNTER_BLOCK {
    DWORD           ByteLength;         // Length in bytes of this structure,
                                        // including the following counters
} PERF_COUNTER_BLOCK, *PPERF_COUNTER_BLOCK;

//
//  The PERF_COUNTER_BLOCK is followed by PERF_OBJECT_TYPE.NumCounters
//  number of counters.
//

//
//  function typedefs for extensible counter function prototypes
//
typedef DWORD (APIENTRY PM_OPEN_PROC) (LPWSTR);
typedef DWORD (APIENTRY PM_COLLECT_PROC) (LPWSTR, LPVOID *, LPDWORD, LPDWORD);
typedef DWORD (APIENTRY PM_CLOSE_PROC) (void);

#endif // _WINPERF_


