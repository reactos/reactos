#ifndef _MSV1_0_AVL_H_
#define _MSV1_0_AVL_H_

BOOL
NtlmAvlAdd(
    _Inout_ PSTRING AvData,
    _In_ MSV1_0_AVID AvId,
    _In_ PVOID Data,
    _In_ ULONG DataLen);

#endif
