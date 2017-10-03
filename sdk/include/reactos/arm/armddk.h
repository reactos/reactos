#ifndef _ARMDDK_
#define _ARMDDK_

#define USPCR                   0x7FFF0000
#define USERPCR                 ((volatile KPCR * const)USPCR)


#ifndef _WINNT_


//
// Address space layout
//
extern PVOID MmHighestUserAddress;
extern PVOID MmSystemRangeStart;
extern ULONG_PTR MmUserProbeAddress;
#define MM_HIGHEST_USER_ADDRESS           MmHighestUserAddress
#define MM_SYSTEM_RANGE_START             MmSystemRangeStart
#define MM_USER_PROBE_ADDRESS             MmUserProbeAddress
#define MM_LOWEST_USER_ADDRESS            (PVOID)0x10000
#define MM_LOWEST_SYSTEM_ADDRESS          (PVOID)0xC0800000



//
// Used to contain PFNs and PFN counts
//
//typedef ULONG PFN_COUNT;
//typedef ULONG PFN_NUMBER, *PPFN_NUMBER;
//typedef LONG SPFN_NUMBER, *PSPFN_NUMBER;



#endif

//
// Processor Control Region
//
#ifdef _WINNT_
#define KIRQL ULONG
#endif

#define ASSERT_BREAKPOINT BREAKPOINT_COMMAND_STRING + 1

#define RESULT_ZERO     0
#define RESULT_NEGATIVE 1
#define RESULT_POSITIVE 2

//
// Intrinsics
//
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedExchange  _InterlockedExchange
#endif
