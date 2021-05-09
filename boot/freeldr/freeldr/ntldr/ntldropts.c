/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     NT Kernel Load Options Support Functions
 * COPYRIGHT:   Copyright 2020 Hermes Belusca-Maito
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>
#include "ntldropts.h"

/* FUNCTIONS *****************************************************************/

PCSTR
NtLdrGetNextOption(
    IN OUT PCSTR* Options,
    OUT PULONG OptionLength OPTIONAL)
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

        /*
         * Check whether a new option starts. Options are delimited
         * with an option separator '/' or with whitespace.
         */
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
    IN PCSTR Options,
    IN PCCH OptionName,
    IN ULONG OptNameLength,
    OUT PULONG OptionLength OPTIONAL)
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
    IN PCSTR Options,
    IN PCSTR OptionName,
    OUT PULONG OptionLength OPTIONAL)
{
    return NtLdrGetOptionExN(Options, OptionName,
                             (ULONG)strlen(OptionName),
                             OptionLength);
}

PCSTR
NtLdrGetOption(
    IN PCSTR Options,
    IN PCSTR OptionName)
{
    return NtLdrGetOptionEx(Options, OptionName, NULL);
}

/*
 * Appends or prepends new options to the ones originally contained
 * in the buffer pointed by LoadOptions, of maximum size BufferSize.
 */
VOID
NtLdrAddOptions(
    IN OUT PSTR LoadOptions,
    IN ULONG BufferSize,
    IN BOOLEAN Append,
    IN PCSTR NewOptions OPTIONAL)
{
    ULONG OptionsLength;
    ULONG NewOptsLength;
    BOOLEAN AddSeparator;

    if (!LoadOptions || (BufferSize == 0))
        return;
    // ASSERT(strlen(LoadOptions) + 1 <= BufferSize);

    if (!NewOptions || !*NewOptions)
        return;

    if (Append)
    {
        OptionsLength = (ULONG)strlen(LoadOptions);
        OptionsLength = min(OptionsLength, BufferSize-1);

        /* Add a whitespace separator if needed */
        if (OptionsLength != 0 &&
            (LoadOptions[OptionsLength-1] != ' ') &&
            (LoadOptions[OptionsLength-1] != '\t') &&
            (*NewOptions != '\0') &&
            (*NewOptions != ' ') &&
            (*NewOptions != '\t'))
        {
            RtlStringCbCatA(LoadOptions, BufferSize * sizeof(CHAR), " ");
        }

        /* Append the options */
        RtlStringCbCatA(LoadOptions, BufferSize * sizeof(CHAR), NewOptions);
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
            (*LoadOptions != '\0') &&
            (*LoadOptions != ' ') &&
            (*LoadOptions != '\t'))
        {
            AddSeparator = TRUE;
            ++NewOptsLength;
        }

        /*
         * Move the original load options forward (possibly truncating them
         * at the end if the buffer is not large enough) to make place for
         * the options to prepend.
         */
        OptionsLength = (ULONG)strlen(LoadOptions) + 1;
        OptionsLength = min(OptionsLength, BufferSize - NewOptsLength);
        RtlMoveMemory(LoadOptions + NewOptsLength,
                      LoadOptions,
                      OptionsLength * sizeof(CHAR));
        /* NULL-terminate */
        (LoadOptions + NewOptsLength)[OptionsLength-1] = '\0';

        /* Restore the new options length back to its original value */
        if (AddSeparator) --NewOptsLength;
        /* Prepend the options and add the whitespace separator if needed */
        strncpy(LoadOptions, NewOptions, NewOptsLength);
        if (AddSeparator) LoadOptions[NewOptsLength] = ' ';
    }
}
