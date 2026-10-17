/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * FILE:              lib/rtl/nls.c
 * PURPOSE:           National Language Support (NLS) functions
 * PROGRAMMERS:       Emanuele Aliberti
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern UINT NlsAnsiCodePage;
extern PUSHORT NlsAnsiToUnicodeTable;
extern PCHAR NlsUnicodeToAnsiTable;
extern PUSHORT NlsLeadByteInfo;

extern USHORT NlsOemCodePage;
extern PUSHORT NlsOemToUnicodeTable;
extern PCHAR NlsUnicodeToOemTable;
extern PUSHORT NlsUnicodeToMbOemTable;
extern PUSHORT NlsOemLeadByteInfo;

extern USHORT NlsOemDefaultChar;
extern USHORT NlsUnicodeDefaultChar;


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID NTAPI
RtlGetDefaultCodePage(OUT PUSHORT AnsiCodePage,
                      OUT PUSHORT OemCodePage)
{
    PAGED_CODE_RTL();

    *AnsiCodePage = NlsAnsiCodePage;
    *OemCodePage = NlsOemCodePage;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlConsoleMultiByteToUnicodeN(OUT PWCHAR UnicodeString,
                              IN ULONG UnicodeSize,
                              OUT PULONG ResultSize,
                              IN PCSTR MbString,
                              IN ULONG MbSize,
                              OUT PULONG Unknown)
{
    PAGED_CODE_RTL();

    UNIMPLEMENTED;
    DPRINT1("RtlConsoleMultiByteToUnicodeN calling RtlMultiByteToUnicodeN\n");
    *Unknown = 1;
    return RtlMultiByteToUnicodeN(UnicodeString,
                                  UnicodeSize,
                                  ResultSize,
                                  MbString,
                                  MbSize);
}

/*
 * @unimplemented
 */
CHAR NTAPI
RtlUpperChar(IN CHAR Source)
{
    WCHAR Unicode;
    CHAR Destination;

    PAGED_CODE_RTL();

    /* Check for simple ANSI case */
    if (Source <= 'z')
    {
        /* Check for simple downcase a-z case */
        if (Source >= 'a')
        {
            /* Just XOR with the difference */
            return Source ^ ('a' - 'A');
        }
        else
        {
            /* Otherwise return the same char, it's already upcase */
            return Source;
        }
    }
    else
    {
        if (!NlsMbCodePageTag)
        {
            /* single-byte code page */

            /* ansi->unicode */
            Unicode = NlsAnsiToUnicodeTable[(UCHAR)Source];

            /* upcase conversion */
            Unicode = RtlUpcaseUnicodeChar (Unicode);

            /* unicode -> ansi */
            Destination = NlsUnicodeToAnsiTable[(USHORT)Unicode];
        }
        else
        {
            /* multi-byte code page */
            /* FIXME */
            Destination = Source;
        }
    }

    return Destination;
}

/* EOF */
