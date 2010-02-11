
NTSYSAPI NTSTATUS NTAPI DbgQueryDebugFilterState(ULONG ComponentId, ULONG Level)
{
    PULONG Mask;

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

    /* Convert Level to bit field if necessary */
    if (Level < 32) Level = 1 << Level;

    /* Determine if this Level is filtered out */
    if ((Kd_WIN2000_Mask & Level) ||
        (*Mask & Level))
    {
        /* This mask will get through to the debugger */
        return (NTSTATUS)TRUE;
    }
    else
    {
        /* This mask is filtered out */
        return (NTSTATUS)FALSE;
    }
}

