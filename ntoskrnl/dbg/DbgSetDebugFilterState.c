extern LUID SeDebugPrivilege;

NTSYSAPI NTSTATUS NTAPI DbgSetDebugFilterState(ULONG ComponentId, ULONG Level, BOOLEAN State)
{
    PULONG Mask;

    /* Modifying debug filters requires the debug privilege */
    if (!SeSinglePrivilegeCheck(SeDebugPrivilege,
                                KeGetPreviousMode()))
    {
        /* Fail */
        return STATUS_ACCESS_DENIED;
    }

    /* Check if the ID fits in the component table */
    if (ComponentId < KdComponentTableSize)
    {
        /* It does, so get the mask from there */
        Mask = KdComponentTable[ComponentId];
    }
    else if (ComponentId == MAXULONG)
    {
        /*
         * This is the internal ID used for DbgPrint messages without ID and
         * Level. Use the system-wide mask for those.
         */
        Mask = &Kd_WIN2000_Mask;
    }
    else
    {
        /* Invalid ID, fail */
        return STATUS_INVALID_PARAMETER_1;
    }

    /* Convert Level to bit field if required */
    if (Level < 32) Level = 1 << Level;

    /* Check what kind of operation this is */
    if (State)
    {
        /* Set the Level */
        *Mask |= Level;
    }
    else
    {
        /* Remove the Level */
        *Mask &= ~Level;
    }

    /* Success */
    return STATUS_SUCCESS;
}
