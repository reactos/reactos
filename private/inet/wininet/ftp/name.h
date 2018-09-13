/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Name.h

Abstract:

    This module defines all of the name.c routines

Author:

    Gary Kimura     [GaryKi]    30-Jul-1990

Revision History:

    Heath Hunnicutt [T-HeathH]  13-Jul-1994 - Ported this file to ftphelp
        project.

--*/

#ifndef _NAME_H_INCLUDED_
#define _NAME_H_INCLUDED_

#if defined(__cplusplus)
extern "C" {
#endif

//
//  The following enumerated type is used to denote the result of name
//  comparisons
//

typedef enum _FSRTL_COMPARISON_RESULT {
    LessThan = -1,
    EqualTo = 0,
    GreaterThan = 1
} FSRTL_COMPARISON_RESULT;
//
//  These following bit values are set in the FsRtlLegalDbcsCharacterArray
//

#ifndef _NTIFS_
extern PUCHAR FsRtlLegalAnsiCharacterArray;
extern PUSHORT NlsOemLeadByteInfo;  // Lead byte info. for ACP
#else
extern PUCHAR *FsRtlLegalAnsiCharacterArray;
extern PUSHORT *NlsOemLeadByteInfo;  // Lead byte info. for ACP
#endif

#define FSRTL_FAT_LEGAL         0x01
#define FSRTL_HPFS_LEGAL        0x02
#define FSRTL_NTFS_LEGAL        0x04
#define FSRTL_WILD_CHARACTER    0x08

#ifndef _NTIFS_

//
//  The following macro is used to determine if an Ansi character is wild.
//

#define FsRtlIsAnsiCharacterWild(C) (                                 \
        ((SCHAR)(C) < 0) ? FALSE :                                    \
                           FlagOn( FsRtlLegalAnsiCharacterArray[(C)], \
                                   FSRTL_WILD_CHARACTER )             \
)

//
//  The following macro is used to determine if an Ansi character is Fat legal.
//

#define FsRtlIsAnsiCharacterLegalFat(C,WILD_OK) (                           \
        ((SCHAR)(C) < 0) ? TRUE :                                           \
                           FlagOn( FsRtlLegalAnsiCharacterArray[(C)],       \
                                   FSRTL_FAT_LEGAL |                        \
                                   ((WILD_OK) ? FSRTL_WILD_CHARACTER : 0) ) \
)

//
//  The following macro is used to determine if an Ansi character is Hpfs legal.
//

#define FsRtlIsAnsiCharacterLegalHpfs(C,WILD_OK) (                          \
        ((SCHAR)(C) < 0) ? TRUE :                                           \
                           FlagOn( FsRtlLegalAnsiCharacterArray[(C)],       \
                                   FSRTL_HPFS_LEGAL |                       \
                                   ((WILD_OK) ? FSRTL_WILD_CHARACTER : 0) ) \
)

//
//  The following macro is used to determine if an Ansi character is Ntfs legal.
//

#define FsRtlIsAnsiCharacterLegalNtfs(C,WILD_OK) (                          \
        ((SCHAR)(C) < 0) ? TRUE :                                           \
                           FlagOn( FsRtlLegalAnsiCharacterArray[(C)],       \
                                   FSRTL_NTFS_LEGAL |                       \
                                   ((WILD_OK) ? FSRTL_WILD_CHARACTER : 0) ) \
)

#else

//
//  The following macro is used to determine if an Ansi character is wild.
//

#define FsRtlIsAnsiCharacterWild(C) (                                    \
        ((SCHAR)(C) < 0) ? FALSE :                                       \
                           FlagOn( (*FsRtlLegalAnsiCharacterArray)[(C)], \
                                   FSRTL_WILD_CHARACTER )                \
)

//
//  The following macro is used to determine if an Ansi character is Fat legal.
//

#define FsRtlIsAnsiCharacterLegalFat(C,WILD_OK) (                           \
        ((SCHAR)(C) < 0) ? TRUE :                                           \
                           FlagOn( (*FsRtlLegalAnsiCharacterArray)[(C)],    \
                                   FSRTL_FAT_LEGAL |                        \
                                   ((WILD_OK) ? FSRTL_WILD_CHARACTER : 0) ) \
)

//
//  The following macro is used to determine if an Ansi character is Hpfs legal.
//

#define FsRtlIsAnsiCharacterLegalHpfs(C,WILD_OK) (                          \
        ((SCHAR)(C) < 0) ? TRUE :                                           \
                           FlagOn( (*FsRtlLegalAnsiCharacterArray)[(C)],    \
                                   FSRTL_HPFS_LEGAL |                       \
                                   ((WILD_OK) ? FSRTL_WILD_CHARACTER : 0) ) \
)

//
//  The following macro is used to determine if an Ansi character is Ntfs legal.
//

#define FsRtlIsAnsiCharacterLegalNtfs(C,WILD_OK) (                          \
        ((SCHAR)(C) < 0) ? TRUE :                                           \
                           FlagOn( (*FsRtlLegalAnsiCharacterArray)[(C)],    \
                                   FSRTL_NTFS_LEGAL |                       \
                                   ((WILD_OK) ? FSRTL_WILD_CHARACTER : 0) ) \
)

#endif
//
//  Unicode Name support routines, implemented in Name.c
//
//  The routines here are used to manipulate unicode names
//

//
//  The following macro is used to determine if a character is wild.
//

#ifndef _NTIFS_

#define FsRtlIsUnicodeCharacterWild(C) (                                  \
      (((C) >= 0x40) ? FALSE : FlagOn( FsRtlLegalAnsiCharacterArray[(C)], \
                                       FSRTL_WILD_CHARACTER ) )           \
)

#else

#define FsRtlIsUnicodeCharacterWild(C) (                                     \
      (((C) >= 0x40) ? FALSE : FlagOn( (*FsRtlLegalAnsiCharacterArray)[(C)], \
                                       FSRTL_WILD_CHARACTER ) )              \
)

#endif

BOOLEAN
FsRtlDoesNameContainWildCards (
    IN LPCSTR pszName
    );

BOOLEAN
FsRtlIsNameInExpression
(
    IN LPCSTR pszExpression,
    IN LPCSTR pszName,
    IN BOOLEAN IgnoreCase
);

#if defined(__cplusplus)
}
#endif

#endif // _NAME_H_INCLUDED_
