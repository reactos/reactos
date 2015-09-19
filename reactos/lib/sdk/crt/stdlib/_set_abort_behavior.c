/*
 * PROJECT:         ReactOS C runtime library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            lib/sdk/crt/stdlib/_set_abort_behavior.c
 * PURPOSE:         _set_abort_behavior implementation
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

extern unsigned int __abort_behavior;

/*!
 * \brief Specifies the behavior of the abort() function.
 *
 * \param flags - Value of the new flags.
 * \param mask - Mask that specifies which flags to update.
 * \return The old flags value.
 */
unsigned int
_cdecl
_set_abort_behavior(
    unsigned int flags,
    unsigned int mask)
{
    unsigned int old_flags;

    /* Save the old flags */
    old_flags = __abort_behavior;

    /* Reset all flags that are not in the mask */
    flags &= mask;

    /* Update the flags in the mask to the new flags value */
    __abort_behavior &= ~mask;
    __abort_behavior |= flags;

    /* Return the old flags */
    return old_flags;
}

