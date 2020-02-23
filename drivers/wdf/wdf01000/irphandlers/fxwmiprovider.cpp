#include "common/fxwmiprovider.h"
#include "common/fxwmiinstance.h"


_Must_inspect_result_
FxWmiInstance*
FxWmiProvider::GetInstanceReferencedLocked(
    __in ULONG Index,
    __in PVOID Tag
    )
{
    FxWmiInstance* pFound;
    PLIST_ENTRY ple;
    ULONG i;

    pFound = NULL;

    for (i = 0, ple = m_InstanceListHead.Flink;
         i < m_NumInstances;
         ple = ple->Flink, i++)
    {
        if (i == Index)
        {
            pFound = CONTAINING_RECORD(ple, FxWmiInstance, m_ListEntry);
            pFound->ADDREF(Tag);
            break;
        }
    }

    return pFound;
}