/*      BREAK <Current directory list structure>
 *
 *
 *       Microsoft Confidential
 *       Copyright (C) Microsoft Corporation 1991
 *       All Rights Reserved.
 *
 *
 *     CDS - Current Directory Structure
 *
 * CDS items are used bu the internal routines to store cluster numbers and
 * network identifiers for each logical name.  The ID field is used dually,
 * both as net ID and for a cluster number for local devices.  In the case
 * of local devices, the cluster number will be -1 if there is a potential
 * of the disk being changed or if the path must be recracked.
 *
 *       Some pathnames have special preambles, such as
 *
 *               \\machane\sharename\...
 *       For these pathnames we can't allow ".." processing to back us
 *       up into the special front part of the name.  The CURDIR_END field
 *       holds the address of the seperator character which marks
 *       the split between the special preamble and the regular
 *       path list; ".." processing isn't allowed to back us up past
 *       (i.e., before) CURDIR_END
 *       For the root, it points at the leading /.  For net
 *       assignments it points at the end (nul) of the initial assignment:
 *       A:/     \\foo\bar           \\foo\bar\blech\bozo
 *         ^              ^                   ^
 */


#define DIRSTRLEN   64+3        // Max length in bytes of directory strings
#define TEMPLEN     DIRSTRLEN*2

/* XLATOFF */
#pragma pack(1)
/* XLATON */

typedef struct CURDIR_LIST {
    CHAR    CurDir_Text[DIRSTRLEN];         // text of assignment and curdir
    USHORT  CurDir_Flags;                   // various flags
    USHORT  CurDir_End;                     // index to ".." backup limit -
} CDS;                                      // see above

typedef CDS UNALIGNED *PCDS;

#define curdirLen   sizeof(CURDIR_LIST)     // Needed for screwed up
                                            // ASM87 which doesn't allow
                                            // Size directive as a macro
                                            // argument

typedef struct CURDIR_LIST_JPN {
    CHAR    CurDirJPN_Text[DIRSTRLEN];      // text of assignment and curdir
    USHORT  CurDirJPN_Flags;                // various flags
    USHORT  CurDirJPN_End;                  // index to ".." backup limit -
    CHAR    CurDirJPN_Reserve[17];          // Reserved for application compatibility.
                                            // Ichitaro ver5 checks drive type by this structure size.
} CDS_JPN;                                  // see above

typedef CDS_JPN UNALIGNED *PCDS_JPN;

#define curdirLen_Jpn   sizeof(CURDIR_LIST_JPN)     // Needed for screwed up
                                                    // ASM87 which doesn't allow
                                                    // Size directive as a macro
                                                    // argument

// Flag values for CURDIR_FLAGS

#define CURDIR_ISNET    0x8000
#define CURDIR_ISIFS    0x8000
#define CURDIR_INUSE    0x4000
#define CURDIR_SPLICE   0x2000
#define CURDIR_LOCAL    0x1000

#define CURDIR_TOSYNC   0x0800              // Directory path to be sync added
#define CURDIR_NT_FIX   0x0400              // fixed disk (includes NETWORK
                                            // drives. Used in $Current_dir perf
                                            // work.



/* XLATOFF */
typedef CDS     UNALIGNED *PCDS;
typedef CDS_JPN UNALIGNED *PCDS_JPN;
/* XLATON */

/* XLATOFF */
#pragma pack()
/* XLATON */
