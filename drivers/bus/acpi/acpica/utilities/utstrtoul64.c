/*******************************************************************************
 *
 * Module Name: utstrtoul64 - String-to-integer conversion support for both
 *                            64-bit and 32-bit integers
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2021, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include "acpi.h"
#include "accommon.h"

#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utstrtoul64")


/*******************************************************************************
 *
 * This module contains the top-level string to 64/32-bit unsigned integer
 * conversion functions:
 *
 *  1) A standard strtoul() function that supports 64-bit integers, base
 *     8/10/16, with integer overflow support. This is used mainly by the
 *     iASL compiler, which implements tighter constraints on integer
 *     constants than the runtime (interpreter) integer-to-string conversions.
 *  2) Runtime "Explicit conversion" as defined in the ACPI specification.
 *  3) Runtime "Implicit conversion" as defined in the ACPI specification.
 *
 * Current users of this module:
 *
 *  iASL        - Preprocessor (constants and math expressions)
 *  iASL        - Main parser, conversion of constants to integers
 *  iASL        - Data Table Compiler parser (constants and math expressions)
 *  Interpreter - Implicit and explicit conversions, GPE method names
 *  Interpreter - Repair code for return values from predefined names
 *  Debugger    - Command line input string conversion
 *  AcpiDump    - ACPI table physical addresses
 *  AcpiExec    - Support for namespace overrides
 *
 * Notes concerning users of these interfaces:
 *
 * AcpiGbl_IntegerByteWidth is used to set the 32/64 bit limit for explicit
 * and implicit conversions. This global must be set to the proper width.
 * For the core ACPICA code, the width depends on the DSDT version. For the
 * AcpiUtStrtoul64 interface, all conversions are 64 bits. This interface is
 * used primarily for iASL, where the default width is 64 bits for all parsers,
 * but error checking is performed later to flag cases where a 64-bit constant
 * is wrongly defined in a 32-bit DSDT/SSDT.
 *
 * In ACPI, the only place where octal numbers are supported is within
 * the ASL language itself. This is implemented via the main AcpiUtStrtoul64
 * interface. According the ACPI specification, there is no ACPI runtime
 * support (explicit/implicit) for octal string conversions.
 *
 ******************************************************************************/


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtStrtoul64
 *
 * PARAMETERS:  String                  - Null terminated input string,
 *                                        must be a valid pointer
 *              ReturnValue             - Where the converted integer is
 *                                        returned. Must be a valid pointer
 *
 * RETURN:      Status and converted integer. Returns an exception on a
 *              64-bit numeric overflow
 *
 * DESCRIPTION: Convert a string into an unsigned integer. Always performs a
 *              full 64-bit conversion, regardless of the current global
 *              integer width. Supports Decimal, Hex, and Octal strings.
 *
 * Current users of this function:
 *
 *  iASL        - Preprocessor (constants and math expressions)
 *  iASL        - Main ASL parser, conversion of ASL constants to integers
 *  iASL        - Data Table Compiler parser (constants and math expressions)
 *  Interpreter - Repair code for return values from predefined names
 *  AcpiDump    - ACPI table physical addresses
 *  AcpiExec    - Support for namespace overrides
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtStrtoul64 (
    char                    *String,
    UINT64                  *ReturnValue)
{
    ACPI_STATUS             Status = AE_OK;
    UINT8                   OriginalBitWidth;
    UINT32                  Base = 10;          /* Default is decimal */


    ACPI_FUNCTION_TRACE_STR (UtStrtoul64, String);


    *ReturnValue = 0;

    /* A NULL return string returns a value of zero */

    if (*String == 0)
    {
        return_ACPI_STATUS (AE_OK);
    }

    if (!AcpiUtRemoveWhitespace (&String))
    {
        return_ACPI_STATUS (AE_OK);
    }

    /*
     * 1) Check for a hex constant. A "0x" prefix indicates base 16.
     */
    if (AcpiUtDetectHexPrefix (&String))
    {
        Base = 16;
    }

    /*
     * 2) Check for an octal constant, defined to be a leading zero
     * followed by sequence of octal digits (0-7)
     */
    else if (AcpiUtDetectOctalPrefix (&String))
    {
        Base = 8;
    }

    if (!AcpiUtRemoveLeadingZeros (&String))
    {
        return_ACPI_STATUS (AE_OK);     /* Return value 0 */
    }

    /*
     * Force a full 64-bit conversion. The caller (usually iASL) must
     * check for a 32-bit overflow later as necessary (If current mode
     * is 32-bit, meaning a 32-bit DSDT).
     */
    OriginalBitWidth = AcpiGbl_IntegerBitWidth;
    AcpiGbl_IntegerBitWidth = 64;

    /*
     * Perform the base 8, 10, or 16 conversion. A 64-bit numeric overflow
     * will return an exception (to allow iASL to flag the statement).
     */
    switch (Base)
    {
    case 8:
        Status = AcpiUtConvertOctalString (String, ReturnValue);
        break;

    case 10:
        Status = AcpiUtConvertDecimalString (String, ReturnValue);
        break;

    case 16:
    default:
        Status = AcpiUtConvertHexString (String, ReturnValue);
        break;
    }

    /* Only possible exception from above is a 64-bit overflow */

    AcpiGbl_IntegerBitWidth = OriginalBitWidth;
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtImplicitStrtoul64
 *
 * PARAMETERS:  String                  - Null terminated input string,
 *                                        must be a valid pointer
 *
 * RETURN:      Converted integer
 *
 * DESCRIPTION: Perform a 64-bit conversion with restrictions placed upon
 *              an "implicit conversion" by the ACPI specification. Used by
 *              many ASL operators that require an integer operand, and support
 *              an automatic (implicit) conversion from a string operand
 *              to the final integer operand. The major restriction is that
 *              only hex strings are supported.
 *
 * -----------------------------------------------------------------------------
 *
 * Base is always 16, either with or without the 0x prefix. Decimal and
 * Octal strings are not supported, as per the ACPI specification.
 *
 * Examples (both are hex values):
 *      Add ("BA98", Arg0, Local0)
 *      Subtract ("0x12345678", Arg1, Local1)
 *
 * Conversion rules as extracted from the ACPI specification:
 *
 *  The converted integer is initialized to the value zero.
 *  The ASCII string is always interpreted as a hexadecimal constant.
 *
 *  1)  According to the ACPI specification, a "0x" prefix is not allowed.
 *      However, ACPICA allows this as an ACPI extension on general
 *      principle. (NO ERROR)
 *
 *  2)  The conversion terminates when the size of an integer is reached
 *      (32 or 64 bits). There are no numeric overflow conditions. (NO ERROR)
 *
 *  3)  The first non-hex character terminates the conversion and returns
 *      the current accumulated value of the converted integer (NO ERROR).
 *
 *  4)  Conversion of a null (zero-length) string to an integer is
 *      technically not allowed. However, ACPICA allows this as an ACPI
 *      extension. The conversion returns the value 0. (NO ERROR)
 *
 * NOTE: There are no error conditions returned by this function. At
 * the minimum, a value of zero is returned.
 *
 * Current users of this function:
 *
 *  Interpreter - All runtime implicit conversions, as per ACPI specification
 *  iASL        - Data Table Compiler parser (constants and math expressions)
 *
 ******************************************************************************/

UINT64
AcpiUtImplicitStrtoul64 (
    char                    *String)
{
    UINT64                  ConvertedInteger = 0;


    ACPI_FUNCTION_TRACE_STR (UtImplicitStrtoul64, String);


    if (!AcpiUtRemoveWhitespace (&String))
    {
        return_VALUE (0);
    }

    /*
     * Per the ACPI specification, only hexadecimal is supported for
     * implicit conversions, and the "0x" prefix is "not allowed".
     * However, allow a "0x" prefix as an ACPI extension.
     */
    AcpiUtRemoveHexPrefix (&String);

    if (!AcpiUtRemoveLeadingZeros (&String))
    {
        return_VALUE (0);
    }

    /*
     * Ignore overflow as per the ACPI specification. This is implemented by
     * ignoring the return status from the conversion function called below.
     * On overflow, the input string is simply truncated.
     */
    AcpiUtConvertHexString (String, &ConvertedInteger);
    return_VALUE (ConvertedInteger);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtExplicitStrtoul64
 *
 * PARAMETERS:  String                  - Null terminated input string,
 *                                        must be a valid pointer
 *
 * RETURN:      Converted integer
 *
 * DESCRIPTION: Perform a 64-bit conversion with the restrictions placed upon
 *              an "explicit conversion" by the ACPI specification. The
 *              main restriction is that only hex and decimal are supported.
 *
 * -----------------------------------------------------------------------------
 *
 * Base is either 10 (default) or 16 (with 0x prefix). Octal (base 8) strings
 * are not supported, as per the ACPI specification.
 *
 * Examples:
 *      ToInteger ("1000")      Decimal
 *      ToInteger ("0xABCD")    Hex
 *
 * Conversion rules as extracted from the ACPI specification:
 *
 *  1)  The input string is either a decimal or hexadecimal numeric string.
 *      A hex value must be prefixed by "0x" or it is interpreted as decimal.
 *
 *  2)  The value must not exceed the maximum of an integer value
 *      (32 or 64 bits). The ACPI specification states the behavior is
 *      "unpredictable", so ACPICA matches the behavior of the implicit
 *      conversion case. There are no numeric overflow conditions. (NO ERROR)
 *
 *  3)  Behavior on the first non-hex character is not defined by the ACPI
 *      specification (for the ToInteger operator), so ACPICA matches the
 *      behavior of the implicit conversion case. It terminates the
 *      conversion and returns the current accumulated value of the converted
 *      integer. (NO ERROR)
 *
 *  4)  Conversion of a null (zero-length) string to an integer is
 *      technically not allowed. However, ACPICA allows this as an ACPI
 *      extension. The conversion returns the value 0. (NO ERROR)
 *
 * NOTE: There are no error conditions returned by this function. At the
 * minimum, a value of zero is returned.
 *
 * Current users of this function:
 *
 *  Interpreter - Runtime ASL ToInteger operator, as per the ACPI specification
 *
 ******************************************************************************/

UINT64
AcpiUtExplicitStrtoul64 (
    char                    *String)
{
    UINT64                  ConvertedInteger = 0;
    UINT32                  Base = 10;          /* Default is decimal */


    ACPI_FUNCTION_TRACE_STR (UtExplicitStrtoul64, String);


    if (!AcpiUtRemoveWhitespace (&String))
    {
        return_VALUE (0);
    }

    /*
     * Only Hex and Decimal are supported, as per the ACPI specification.
     * A "0x" prefix indicates hex; otherwise decimal is assumed.
     */
    if (AcpiUtDetectHexPrefix (&String))
    {
        Base = 16;
    }

    if (!AcpiUtRemoveLeadingZeros (&String))
    {
        return_VALUE (0);
    }

    /*
     * Ignore overflow as per the ACPI specification. This is implemented by
     * ignoring the return status from the conversion functions called below.
     * On overflow, the input string is simply truncated.
     */
    switch (Base)
    {
    case 10:
    default:
        AcpiUtConvertDecimalString (String, &ConvertedInteger);
        break;

    case 16:
        AcpiUtConvertHexString (String, &ConvertedInteger);
        break;
    }

    return_VALUE (ConvertedInteger);
}
