/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/device.c
 * PURPOSE:         Kernel Device/Settings Functions
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/*
 * @implemented
 */
PCONFIGURATION_COMPONENT_DATA
STDCALL
INIT_FUNCTION
KeFindConfigurationEntry(IN PCONFIGURATION_COMPONENT_DATA Child,
                         IN CONFIGURATION_CLASS Class,
                         IN CONFIGURATION_TYPE Type,
                         IN PULONG ComponentKey OPTIONAL)
{
    /* Start Search at Root */
    return KeFindConfigurationNextEntry(Child, 
                                        Class, 
                                        Type, 
                                        ComponentKey, 
                                        NULL);
}

/*
 * @implemented
 */
PCONFIGURATION_COMPONENT_DATA
STDCALL
INIT_FUNCTION
KeFindConfigurationNextEntry(IN PCONFIGURATION_COMPONENT_DATA Child,
                             IN CONFIGURATION_CLASS Class,
                             IN CONFIGURATION_TYPE Type,
                             IN PULONG ComponentKey OPTIONAL,
                             IN PCONFIGURATION_COMPONENT_DATA *NextLink)
{
    ULONG Key = 0;    
    ULONG Mask = 0;
    PCONFIGURATION_COMPONENT_DATA Sibling;
    PCONFIGURATION_COMPONENT_DATA ReturnEntry;
    
    /* If we did get a key, then use it instead */
    if (ComponentKey)
    {
        Key = *ComponentKey;
        Mask = -1;
    }
    
    /* Loop the Components until we find a a match */
    while (Child)
    {
        /* Check if we are starting somewhere already */
        if (*NextLink)
        {
            /* If we've found the place where we started, clear and continue */
            if (Child == *NextLink) *NextLink = NULL;
        } 
        else
        {
            /* Try to get a match */
            if (Child->Component.Class == Class &&
                Child->Component.Type == Type &&
                (Child->Component.Key & Mask) == Key)
            {
                /* Match found */
                return Child;
            }
        }
        
        /* Now we've also got to lookup the siblings */
        Sibling = Child->Sibling;
        while (Sibling)
        {
            /* Check if we are starting somewhere already */
            if (*NextLink)
            {
                /* If we've found the place where we started, clear and continue */
                if (Sibling == *NextLink) *NextLink = NULL;
            } 
            else
            {
                /* Try to get a match */
                if (Sibling->Component.Class == Class &&
                    Sibling->Component.Type == Type &&
                    (Sibling->Component.Key & Mask) == Key)
                {
                    /* Match found */
                    return Sibling;
                }
            }
            
            /* We've got to check if the Sibling has a Child as well */
            if (Sibling->Child)
            {
                /* We're just going to call ourselves again */
                if ((ReturnEntry = KeFindConfigurationNextEntry(Sibling->Child,
                                                                Class,
                                                                Type,
                                                                ComponentKey,
                                                                NextLink)))
                {
                    return ReturnEntry;
                }
            }
            
            /* Next Sibling */
            Sibling = Sibling->Sibling;
        }
        
        /* Next Child */
        Child = Child->Child;
    }
    
    /* If we got here, nothign was found */
    return NULL;
}

/*
 * @implemented
 */
VOID
STDCALL
KeFlushEntireTb(
    IN BOOLEAN Unknown,
    IN BOOLEAN CurrentCpuOnly
)
{
	KIRQL OldIrql;
	PKPROCESS Process = NULL;
	PKPRCB Prcb = NULL;

	/* Raise the IRQL for the TB Flush */
	OldIrql = KeRaiseIrqlToSynchLevel();

	/* All CPUs need to have the TB flushed. */
	if (CurrentCpuOnly == FALSE) {
		Prcb = KeGetCurrentPrcb();

		/* How many CPUs is our caller using? */
		Process = Prcb->CurrentThread->ApcState.Process;

		/* More then one, so send an IPI */
		if (Process->ActiveProcessors > 1) {
			/* Send IPI Packet */
		}
	}

	/* Flush the TB for the Current CPU */
	KeFlushCurrentTb();

	/* Clean up */
	if (CurrentCpuOnly == FALSE) {
		/* Did we send an IPI? If so, wait for completion */
		if (Process->ActiveProcessors > 1) {
			do {
			} while (Prcb->TargetSet != 0);
		}
	}

	/* FIXME: According to MSKB, we should increment a counter? */

	/* Return to Original IRQL */
	KeLowerIrql(OldIrql);
}


/*
 * @implemented
 */
VOID
STDCALL
KeSetDmaIoCoherency(
    IN ULONG Coherency
)
{
	KiDmaIoCoherency = Coherency;
}

/*
 * @implemented
 */
KAFFINITY
STDCALL
KeQueryActiveProcessors (
    VOID
    )
{
	return KeActiveProcessors;
}


/*
 * @unimplemented
 */
VOID
__cdecl
KeSaveStateForHibernate(
    IN PVOID State
)
{
	UNIMPLEMENTED;
}
