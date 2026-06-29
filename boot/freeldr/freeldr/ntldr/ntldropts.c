/*
 * PROJECT:     FreeLoader
 * LICENSE:     Dual-licensed:
 *              GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     NT Kernel Load Options Support Functions
 * COPYRIGHT:   Copyright 2020-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>
#include "ntldropts.h"

/* FUNCTIONS *****************************************************************/

PCSTR
NtLdrGetNextOption(
    _Inout_ PCSTR* Options,
    _Out_opt_ PULONG OptionLength)
{
    PCSTR NextOption;
    PCSTR Option = NULL;
    ULONG Length = 0;

    if (OptionLength)
        *OptionLength = 0;

    if (!Options || !*Options)
        return NULL;

    /* Loop over each option */
    NextOption = *Options;
    while (*NextOption)
    {
        /* Skip possible initial whitespace */
        NextOption += strspn(NextOption, " \t");

        /* Stop now if we have already found an option.
         * NextOption points to the next following option. */
        if (Option)
            break;

        /* Check whether a new option starts. Options are delimited
         * with an option separator '/' or with whitespace. */
        if (*NextOption == '/')
            ++NextOption;

        /* Get the actual length of the option until
         * the next whitespace or option separator. */
        Length = (ULONG)strcspn(NextOption, " \t/");

        /* Retrieve the option if present and go to the beginning of the next one */
        if (Length != 0)
            Option = NextOption;

        /* Restart after the end of the option */
        NextOption += Length;
    }

    *Options = NextOption;
    if (Option && OptionLength)
        *OptionLength = Length;
    return Option;
}

/*
 * OptionName specifies the option name, without any leading
 * option separator '/', to search for within the Options.
 * The search is made case-insensitive.
 */
PCSTR
NtLdrGetOptionExN(
    _In_ PCSTR Options,
    _In_reads_(OptNameLength) PCCH OptionName,
    _In_ ULONG OptNameLength,
    _Out_opt_ PULONG OptionLength)
{
    PCSTR NextOptions;
    PCSTR Option = NULL;
    ULONG OptLength = 0;

    if (OptionLength)
        *OptionLength = 0;

    if (!Options || !*Options)
        return NULL;
    if (!OptionName || (OptNameLength == 0) || !*OptionName)
        return NULL;

    NextOptions = Options;
    while ((Option = NtLdrGetNextOption(&NextOptions, &OptLength)))
    {
        /*
         * Check whether the option to find exactly matches the current
         * load option, or is a prefix thereof if this is an option with
         * appended data.
         */
        if ((OptLength >= OptNameLength) &&
            (_strnicmp(Option, OptionName, OptNameLength) == 0))
        {
            if ((OptLength == OptNameLength) ||
                (OptionName[OptNameLength-1] == '=') ||
                (OptionName[OptNameLength-1] == ':'))
            {
                break;
            }
        }
    }

    if (Option && OptionLength)
        *OptionLength = OptLength;
    return Option;
}

PCSTR
NtLdrGetOptionEx(
    _In_ PCSTR Options,
    _In_ PCSTR OptionName,
    _Out_opt_ PULONG OptionLength)
{
    return NtLdrGetOptionExN(Options, OptionName,
                             (ULONG)strlen(OptionName),
                             OptionLength);
}

PCSTR
NtLdrGetOption(
    _In_ PCSTR Options,
    _In_ PCSTR OptionName)
{
    return NtLdrGetOptionEx(Options, OptionName, NULL);
}

/**
 * @brief
 * Appends or prepends new options to the ones originally contained
 * in the buffer pointed by Options, of maximum size BufferSize.
 **/
VOID
NtLdrAddOptions(
    _Inout_updates_z_(BufferSize) PSTR Options,
    _In_ ULONG BufferSize,
    _In_ BOOLEAN Append,
    _In_opt_ PCSTR NewOptions)
{
    ULONG OptionsLength;
    ULONG NewOptsLength;
    BOOLEAN AddSeparator;

    if (!Options || (BufferSize == 0))
        return;
    // ASSERT(strlen(Options) + 1 <= BufferSize);

    if (!NewOptions || !*NewOptions)
        return;

    if (Append)
    {
        OptionsLength = (ULONG)strlen(Options);
        OptionsLength = min(OptionsLength, BufferSize-1);

        /* Add a whitespace separator if needed */
        if (OptionsLength != 0 &&
            (Options[OptionsLength-1] != ' ') &&
            (Options[OptionsLength-1] != '\t') &&
            (*NewOptions != '\0') &&
            (*NewOptions != ' ') &&
            (*NewOptions != '\t'))
        {
            RtlStringCbCatA(Options, BufferSize * sizeof(CHAR), " ");
        }

        /* Append the options */
        RtlStringCbCatA(Options, BufferSize * sizeof(CHAR), NewOptions);
    }
    else
    {
        NewOptsLength = (ULONG)strlen(NewOptions);
        NewOptsLength = min(NewOptsLength, BufferSize-1);

        /* Add a whitespace separator if needed */
        AddSeparator = FALSE;
        if (NewOptsLength != 0 &&
            (NewOptions[NewOptsLength-1] != ' ') &&
            (NewOptions[NewOptsLength-1] != '\t') &&
            (*Options != '\0') &&
            (*Options != ' ') &&
            (*Options != '\t'))
        {
            AddSeparator = TRUE;
            ++NewOptsLength;
        }

        /*
         * Move the original load options forward (possibly truncating them
         * at the end if the buffer is not large enough) to make place for
         * the options to prepend.
         */
        OptionsLength = (ULONG)strlen(Options) + 1;
        OptionsLength = min(OptionsLength, BufferSize - NewOptsLength);
        RtlMoveMemory(Options + NewOptsLength,
                      Options,
                      OptionsLength * sizeof(CHAR));
        /* NULL-terminate */
        (Options + NewOptsLength)[OptionsLength-1] = '\0';

        /* Restore the new options length back to its original value */
        if (AddSeparator)
            --NewOptsLength;
        /* Prepend the options and add the whitespace separator if needed */
        strncpy(Options, NewOptions, NewOptsLength);
        if (AddSeparator)
            Options[NewOptsLength] = ' ';
    }
}

/**
 * @brief
 * Updates the options in the buffer pointed by LoadOptions, of maximum size
 * BufferSize, by first removing any specified options, and then adding any
 * other ones.
 *
 * OptionsToAdd is a NULL-terminated array of string buffer pointers that
 *    specify the options to be added into LoadOptions. Whether they are
 *    prepended or appended to LoadOptions is controlled via the Append
 *    parameter. The options are added in the order specified by the array.
 *
 * OptionsToRemove is a NULL-terminated array of string buffer pointers that
 *    specify the options to remove from LoadOptions. Specifying also there
 *    any options to add, has the effect of removing from LoadOptions any
 *    duplicates of the options to be added, before adding them later into
 *    LoadOptions. The options are removed in the order specified by the array.
 *
 * The options string buffers in the OptionsToRemove array have the format:
 *    "/option1 /option2[=] ..."
 *
 * An option in the OptionsToRemove list with a trailing '=' or ':' designates
 * an option in LoadOptions with user-specific data appended after the sign.
 * When such an option is being removed from LoadOptions, all the appended
 * data is also removed until the next option.
 **/
VOID
NtLdrUpdateOptions(
    _Inout_updates_z_(BufferSize) PSTR LoadOptions,
    _In_ ULONG BufferSize,
    _In_ BOOLEAN Append,
    _In_opt_ PCSTR OptionsToAdd[],
    _In_opt_ PCSTR OptionsToRemove[])
{
    PCSTR NextOptions, NextOpt;
    PSTR Options, Option;
    ULONG NextOptLength;
    ULONG OptionLength;

    if (!LoadOptions || (BufferSize == 0))
        return;
    // ASSERT(strlen(LoadOptions) + 1 <= BufferSize);

    /* Loop over the options to remove */
    for (; OptionsToRemove && *OptionsToRemove; ++OptionsToRemove)
    {
        NextOptions = *OptionsToRemove;
        while ((NextOpt = NtLdrGetNextOption(&NextOptions, &NextOptLength)))
        {
            /* Scan the load options */
            Options = LoadOptions;
            while ((Option = (PSTR)NtLdrGetNextOption((PCSTR*)&Options, &OptionLength)))
            {
                /*
                 * Check whether the option to find exactly matches the current
                 * load option, or is a prefix thereof if this is an option with
                 * appended data.
                 */
                if ((OptionLength >= NextOptLength) &&
                    (_strnicmp(Option, NextOpt, NextOptLength) == 0))
                {
                    if ((OptionLength == NextOptLength) ||
                        (NextOpt[NextOptLength-1] == '=') ||
                        (NextOpt[NextOptLength-1] == ':'))
                    {
                        /* Eat any skipped option or whitespace separators */
                        while ((Option > LoadOptions) &&
                               (Option[-1] == '/' ||
                                Option[-1] == ' ' ||
                                Option[-1] == '\t'))
                        {
                            --Option;
                        }

                        /* If the option was not preceded by a whitespace
                         * separator, insert one and advance the pointer. */
                        if ((Option > LoadOptions) &&
                            (Option[-1] != ' ') &&
                            (Option[-1] != '\t') &&
                            (*Options != '\0') /* &&
                            ** Not necessary since NtLdrGetNextOption() **
                            ** stripped any leading separators.         **
                            (*Options != ' ') &&
                            (*Options != '\t') */)
                        {
                            *Option++ = ' ';
                        }

                        /* Move the remaining options back, erasing the current one */
                        ASSERT(Option <= Options);
                        RtlMoveMemory(Option,
                                      Options,
                                      (strlen(Options) + 1) * sizeof(CHAR));

                        /* Reset the iterator */
                        Options = Option;
                    }
                }
            }
        }
    }

    /* Now loop over the options to add */
    for (; OptionsToAdd && *OptionsToAdd; ++OptionsToAdd)
    {
        NtLdrAddOptions(LoadOptions,
                        BufferSize,
                        Append,
                        *OptionsToAdd);
    }
}
