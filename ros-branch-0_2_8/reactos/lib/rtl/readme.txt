RTL restrictions:

ExAllocatePool (and friends) must be used exclusively. RtlAllocateHeap (and friends) must NOT be used! ExAllocatePool (and friends) translate to RtlAllocateHeap (and friends) in ntdll\rtl\libsupp.c.

RtlEnterCriticalSection (and friends) must be used exclusively. ExAcquireFastMutex (and friends) must NOT be used! RtlEnterCriticalSection (and friends) translate to ExAcquireFastMutex (and friends) in ntoskrnl\rtl\libsupp.c. This means that RtlEnterCriticalSection (and friends) can NOT be used recursively in RTL. The reason for choosing RtlEnterCriticalSection (and friends) over ExAcquireFastMutex (and friends) is that the FAST_MUTEX struct is smaller than the RTL_CRITICAL_SECTION struct.