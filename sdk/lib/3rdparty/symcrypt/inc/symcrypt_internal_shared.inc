//
// symcrypt_internal_shared.inc
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// This is the file that contains the SymCrypt version information and defines SYMCRYPT_DEBUG.
// It is included in both C and ASM such that the values are the same on both sides.
// We use the C preprocessor to set ASM constants, as we already need to use the C preprocessor for
// symcryptasm processing (see scripts/symcryptasm_processor.py).
//
// In previous releases we had a numbering system with  major/minor version number.
// This worked well with the sequential servicing imposed by SourceDepot.
// With the switch to Git this no longer works due to having multiple branches.
// We move to having the version here only specify the API and minor version number
// These will NOT be changed for every build. The API version only changes when there are
// breaking changes to the API in symcrypt.h. (Note: symcrypt_low_level.h is not stable and can change
// at any time.) The minor version is changed at regular intervals, but not necessarily at
// every build of the library.
//
// Separate from these numbers the build system includes information about the branch,
// last commit, build time, etc.
//
// The API numbering starts at 100 to avoid number conflicts with the old system.
//

#define SYMCRYPT_CODE_VERSION_API       103
#define SYMCRYPT_CODE_VERSION_MINOR     11
#define SYMCRYPT_CODE_VERSION_PATCH     0

#if defined(DBG)
#define SYMCRYPT_DEBUG 1
#else
#define SYMCRYPT_DEBUG 0
#endif
