//
// Parameters for trial division mechanism
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
// GENERATED FILE, DO NOT EDIT.
//


//
// The primes are put into groups of consecutive primes (skipping 2, 3, 5, and 17).
// Each group has a product less than SYMCRYPT_MAX_SMALL_PRIME_GROUP_PRODUCT which is
// chosen to avoid overflows in the modular reduction computation.
//

typedef struct _SYMCRYPT_SMALL_PRIME_GROUPS_SPEC {
    UINT16 nGroups;    // # groups of this size
    UINT8  nPrimes;    // # primes in the group
    UINT32 maxPrime;   // largest prime in the last group
} SYMCRYPT_SMALL_PRIME_GROUPS_SPEC;

#define SYMCRYPT_MAX_SMALL_PRIME_GROUP_PRODUCT    (0x1c71c71cU)

const SYMCRYPT_SMALL_PRIME_GROUPS_SPEC g_SymCryptSmallPrimeGroupsSpec[] = {
    {     1,  7, 31 },
    {     1,  5, 53 },
    {     5,  4, 151 },
    {    34,  3, 787 },
    {  1156,  2, 21841 },
    {     0,  1, 0xffffffff },
};
