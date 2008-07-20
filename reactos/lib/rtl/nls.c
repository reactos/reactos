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

PUSHORT NlsUnicodeUpcaseTable = NULL;
PUSHORT NlsUnicodeLowercaseTable = NULL;

USHORT NlsAnsiCodePage = 0; /* exported */
BOOLEAN NlsMbCodePageTag = FALSE; /* exported */
PWCHAR NlsAnsiToUnicodeTable = NULL;
PCHAR NlsUnicodeToAnsiTable = NULL;
PWCHAR NlsDbcsUnicodeToAnsiTable = NULL;
PUSHORT NlsLeadByteInfo = NULL; /* exported */


USHORT NlsOemCodePage = 0;
BOOLEAN NlsMbOemCodePageTag = FALSE; /* exported */
PWCHAR NlsOemToUnicodeTable = NULL;
PCHAR NlsUnicodeToOemTable =NULL;
PWCHAR NlsDbcsUnicodeToOemTable = NULL;
PUSHORT _NlsOemLeadByteInfo = NULL; /* exported */

#define NlsOemLeadByteInfo              _NlsOemLeadByteInfo
#define INIT_FUNCTION

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlCustomCPToUnicodeN(IN PCPTABLEINFO CustomCP,
                      PWCHAR UnicodeString,
                      ULONG UnicodeSize,
                      PULONG ResultSize,
                      PCHAR CustomString,
                      ULONG CustomSize)
{
   ULONG Size = 0;
   ULONG i;

   if (CustomCP->DBCSCodePage == 0)
   {
      /* single-byte code page */
      if (CustomSize > (UnicodeSize / sizeof(WCHAR)))
         Size = UnicodeSize / sizeof(WCHAR);
      else
         Size = CustomSize;

      if (ResultSize != NULL)
         *ResultSize = Size * sizeof(WCHAR);

      for (i = 0; i < Size; i++)
      {
         *UnicodeString = CustomCP->MultiByteTable[(UCHAR)*CustomString];
         UnicodeString++;
         CustomString++;
      }
   }
   else
   {
      /* multi-byte code page */
      /* FIXME */
      ASSERT(FALSE);
   }

   return(STATUS_SUCCESS);
}



WCHAR NTAPI
RtlDowncaseUnicodeChar (IN WCHAR Source)
{
   USHORT Offset;

   if (Source < L'A')
      return Source;

   if (Source <= L'Z')
      return Source + (L'a' - L'A');

   if (Source < 0x80)
      return Source;

   Offset = ((USHORT)Source >> 8);
   DPRINT("Offset: %hx\n", Offset);

   Offset = NlsUnicodeLowercaseTable[Offset];
   DPRINT("Offset: %hx\n", Offset);

   Offset += (((USHORT)Source & 0x00F0) >> 4);
   DPRINT("Offset: %hx\n", Offset);

   Offset = NlsUnicodeLowercaseTable[Offset];
   DPRINT("Offset: %hx\n", Offset);

   Offset += ((USHORT)Source & 0x000F);
   DPRINT("Offset: %hx\n", Offset);

   Offset = NlsUnicodeLowercaseTable[Offset];
   DPRINT("Offset: %hx\n", Offset);

   DPRINT("Result: %hx\n", Source + (SHORT)Offset);

   return Source + (SHORT)Offset;
}




/*
 * @implemented
 */
VOID NTAPI
RtlGetDefaultCodePage(OUT PUSHORT AnsiCodePage,
                      OUT PUSHORT OemCodePage)
{
   *AnsiCodePage = NlsAnsiCodePage;
   *OemCodePage = NlsOemCodePage;
}




/*
 * @implemented
 */
VOID NTAPI
RtlInitCodePageTable(IN PUSHORT TableBase,
                     OUT PCPTABLEINFO CodePageTable)
{
   PNLS_FILE_HEADER NlsFileHeader;
   PUSHORT Ptr;
   USHORT Offset;

   DPRINT("RtlInitCodePageTable() called\n");

   NlsFileHeader = (PNLS_FILE_HEADER)TableBase;

   CodePageTable->CodePage = NlsFileHeader->CodePage;
   CodePageTable->MaximumCharacterSize = NlsFileHeader->MaximumCharacterSize;
   CodePageTable->DefaultChar = NlsFileHeader->DefaultChar;
   CodePageTable->UniDefaultChar = NlsFileHeader->UniDefaultChar;
   CodePageTable->TransDefaultChar = NlsFileHeader->TransDefaultChar;
   CodePageTable->TransUniDefaultChar = NlsFileHeader->TransUniDefaultChar;

   RtlCopyMemory(&CodePageTable->LeadByte,
                 &NlsFileHeader->LeadByte,
                 MAXIMUM_LEADBYTES);

   /* Set Pointer to start of multi byte table */
   Ptr = (PUSHORT)((ULONG_PTR)TableBase + 2 * NlsFileHeader->HeaderSize);

   /* Get offset to the wide char table */
   Offset = (USHORT)(*Ptr++) + NlsFileHeader->HeaderSize + 1;

   /* Set pointer to the multi byte table */
   CodePageTable->MultiByteTable = Ptr;

   /* Skip ANSI and OEM table */
   Ptr += 256;
   if (*Ptr++)
      Ptr += 256;

   /* Set pointer to DBCS ranges */
   CodePageTable->DBCSRanges = (PUSHORT)Ptr;

   if (*Ptr > 0)
   {
      CodePageTable->DBCSCodePage = 1;
      CodePageTable->DBCSOffsets = (PUSHORT)++Ptr;
   }
   else
   {
      CodePageTable->DBCSCodePage = 0;
      CodePageTable->DBCSOffsets = 0;
   }

   CodePageTable->WideCharTable = (PVOID)((ULONG_PTR)TableBase + 2 * Offset);
}




/*
 * @implemented
 */
VOID NTAPI
RtlInitNlsTables(IN PUSHORT AnsiTableBase,
                 IN PUSHORT OemTableBase,
                 IN PUSHORT CaseTableBase,
                 OUT PNLSTABLEINFO NlsTable)
{
   DPRINT("RtlInitNlsTables()called\n");

   if (AnsiTableBase == NULL ||
         OemTableBase == NULL ||
         CaseTableBase == NULL)
      return;

   RtlInitCodePageTable (AnsiTableBase,
                         &NlsTable->AnsiTableInfo);

   RtlInitCodePageTable (OemTableBase,
                         &NlsTable->OemTableInfo);

   NlsTable->UpperCaseTable = (PUSHORT)CaseTableBase + 2;
   NlsTable->LowerCaseTable = (PUSHORT)CaseTableBase + *((PUSHORT)CaseTableBase + 1) + 2;
}


/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlMultiByteToUnicodeN(
   IN PWCHAR UnicodeString,
   IN ULONG UnicodeSize,
   IN PULONG ResultSize,
   IN PCSTR MbString,
   IN ULONG MbSize)
{
   ULONG Size = 0;
   ULONG i;

   if (NlsMbCodePageTag == FALSE)
   {
      /* single-byte code page */
      if (MbSize > (UnicodeSize / sizeof(WCHAR)))
         Size = UnicodeSize / sizeof(WCHAR);
      else
         Size = MbSize;

      if (ResultSize != NULL)
         *ResultSize = Size * sizeof(WCHAR);

      for (i = 0; i < Size; i++)
         UnicodeString[i] = NlsAnsiToUnicodeTable[(UCHAR)MbString[i]];
   }
   else
   {
      /* multi-byte code page */
      /* FIXME */

      UCHAR Char;
      USHORT LeadByteInfo;
      PCSTR MbEnd = MbString + MbSize;

      for (i = 0; i < UnicodeSize / sizeof(WCHAR) && MbString < MbEnd; i++)
      {
         Char = *(PUCHAR)MbString++;

         if (Char < 0x80)
         {
            *UnicodeString++ = Char;
            continue;
         }

         LeadByteInfo = NlsLeadByteInfo[Char];

         if (!LeadByteInfo)
         {
            *UnicodeString++ = NlsAnsiToUnicodeTable[Char];
            continue;
         }

         if (MbString < MbEnd)
            *UnicodeString++ = NlsLeadByteInfo[LeadByteInfo + *(PUCHAR)MbString++];
      }

      if (ResultSize != NULL)
         *ResultSize = i * sizeof(WCHAR);
   }

   return(STATUS_SUCCESS);
}



/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlMultiByteToUnicodeSize(PULONG UnicodeSize,
                          PCSTR MbString,
                          ULONG MbSize)
{
    ULONG Length = 0;

    if (!NlsMbCodePageTag)
    {
        /* single-byte code page */
        *UnicodeSize = MbSize * sizeof (WCHAR);
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        while (MbSize--)
        {
            UCHAR Char = *(PUCHAR)MbString++;

            if (Char >= 0x80 && NlsLeadByteInfo[Char])
            {
                if (MbSize)
                {
                    /* Move on */
                    MbSize--;
                    MbString++;
                }
            }

            /* Increase returned size */
            Length++;
        }

        /* Return final size */
        *UnicodeSize = Length * sizeof(WCHAR);
    }

    /* Success */
    return STATUS_SUCCESS;
}



/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlOemToUnicodeN (PWCHAR UnicodeString,
                  ULONG UnicodeSize,
                  PULONG ResultSize,
                  PCHAR OemString,
                  ULONG OemSize)
{
   ULONG Size = 0;
   ULONG i;

   if (NlsMbOemCodePageTag == FALSE)
   {
      /* single-byte code page */
      if (OemSize > (UnicodeSize / sizeof(WCHAR)))
         Size = UnicodeSize / sizeof(WCHAR);
      else
         Size = OemSize;

      if (ResultSize != NULL)
         *ResultSize = Size * sizeof(WCHAR);

      for (i = 0; i < Size; i++)
      {
         *UnicodeString = NlsOemToUnicodeTable[(INT)*OemString];
         UnicodeString++;
         OemString++;
      }
   }
   else
   {
      /* multi-byte code page */
      /* FIXME */

      UCHAR Char;
      USHORT OemLeadByteInfo;
      PCHAR OemEnd = OemString + OemSize;

      for (i = 0; i < UnicodeSize / sizeof(WCHAR) && OemString < OemEnd; i++)
      {
         Char = *(PUCHAR)OemString++;

         if (Char < 0x80)
         {
            *UnicodeString++ = Char;
            continue;
         }

         OemLeadByteInfo = NlsOemLeadByteInfo[Char];

         if (!OemLeadByteInfo)
         {
            *UnicodeString++ = NlsOemToUnicodeTable[Char];
            continue;
         }

         if (OemString < OemEnd)
            *UnicodeString++ =
               NlsOemLeadByteInfo[OemLeadByteInfo + *(PUCHAR)OemString++];
      }

      if (ResultSize != NULL)
         *ResultSize = i * sizeof(WCHAR);
   }

   return STATUS_SUCCESS;
}



/*
 * @implemented
 */
VOID NTAPI
RtlResetRtlTranslations(IN PNLSTABLEINFO NlsTable)
{
   DPRINT("RtlResetRtlTranslations() called\n");

   /* Set ANSI data */
   NlsAnsiToUnicodeTable = (PWCHAR)NlsTable->AnsiTableInfo.MultiByteTable; /* Real type is PUSHORT */
   NlsUnicodeToAnsiTable = NlsTable->AnsiTableInfo.WideCharTable;
   NlsDbcsUnicodeToAnsiTable = (PWCHAR)NlsTable->AnsiTableInfo.WideCharTable;
   NlsMbCodePageTag = (NlsTable->AnsiTableInfo.DBCSCodePage != 0);
   NlsLeadByteInfo = NlsTable->AnsiTableInfo.DBCSOffsets;
   NlsAnsiCodePage = NlsTable->AnsiTableInfo.CodePage;
   DPRINT("Ansi codepage %hu\n", NlsAnsiCodePage);

   /* Set OEM data */
   NlsOemToUnicodeTable = (PWCHAR)NlsTable->OemTableInfo.MultiByteTable; /* Real type is PUSHORT */
   NlsUnicodeToOemTable = NlsTable->OemTableInfo.WideCharTable;
   NlsDbcsUnicodeToOemTable = (PWCHAR)NlsTable->OemTableInfo.WideCharTable;
   NlsMbOemCodePageTag = (NlsTable->OemTableInfo.DBCSCodePage != 0);
   NlsOemLeadByteInfo = NlsTable->OemTableInfo.DBCSOffsets;
   NlsOemCodePage = NlsTable->OemTableInfo.CodePage;
   DPRINT("Oem codepage %hu\n", NlsOemCodePage);

   /* Set Unicode case map data */
   NlsUnicodeUpcaseTable = NlsTable->UpperCaseTable;
   NlsUnicodeLowercaseTable = NlsTable->LowerCaseTable;
}



/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUnicodeToCustomCPN(IN PCPTABLEINFO CustomCP,
                      PCHAR CustomString,
                      ULONG CustomSize,
                      PULONG ResultSize,
                      PWCHAR UnicodeString,
                      ULONG UnicodeSize)
{
   ULONG Size = 0;
   ULONG i;

   if (CustomCP->DBCSCodePage == 0)
   {
      /* single-byte code page */
      if (UnicodeSize > (CustomSize * sizeof(WCHAR)))
         Size = CustomSize;
      else
         Size = UnicodeSize / sizeof(WCHAR);

      if (ResultSize != NULL)
         *ResultSize = Size;

      for (i = 0; i < Size; i++)
      {
         *CustomString = ((PCHAR)CustomCP->WideCharTable)[*UnicodeString];
         CustomString++;
         UnicodeString++;
      }
   }
   else
   {
      /* multi-byte code page */
      /* FIXME */
      ASSERT(FALSE);
   }

   return STATUS_SUCCESS;
}



/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUnicodeToMultiByteN (PCHAR MbString,
                        ULONG MbSize,
                        PULONG ResultSize,
                        PWCHAR UnicodeString,
                        ULONG UnicodeSize)
{
   ULONG Size = 0;
   ULONG i;

   if (NlsMbCodePageTag == FALSE)
   {
      /* single-byte code page */
      Size =  (UnicodeSize > (MbSize * sizeof (WCHAR)))
                 ? MbSize
	         : (UnicodeSize / sizeof (WCHAR));

      if (ResultSize != NULL)
      {
         *ResultSize = Size;
      }

      for (i = 0; i < Size; i++)
      {
         *MbString++ = NlsUnicodeToAnsiTable[*UnicodeString++];
      }
   }
   else
   {
      /* multi-byte code page */
      /* FIXME */

      USHORT WideChar;
      USHORT MbChar;

      for (i = MbSize, Size = UnicodeSize / sizeof(WCHAR); i && Size; i--, Size--)
      {
         WideChar = *UnicodeString++;

         if (WideChar < 0x80)
         {
            *MbString++ = LOBYTE(WideChar);
            continue;
         }

         MbChar = NlsDbcsUnicodeToAnsiTable[WideChar];

         if (!HIBYTE(MbChar))
         {
            *MbString++ = LOBYTE(MbChar);
            continue;
         }

         if (i >= 2)
         {
            *MbString++ = HIBYTE(MbChar);
            *MbString++ = LOBYTE(MbChar);
            i--;
         }
         else break;
      }

      if (ResultSize != NULL)
         *ResultSize = MbSize - i;
   }

   return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlUnicodeToMultiByteSize(PULONG MbSize,
                          PWCHAR UnicodeString,
                          ULONG UnicodeSize)
{
    ULONG UnicodeLength = UnicodeSize / sizeof(WCHAR);
    ULONG MbLength = 0;

    if (!NlsMbCodePageTag)
    {
        /* single-byte code page */
        *MbSize = UnicodeLength;
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        while (UnicodeLength--)
        {
            USHORT WideChar = *UnicodeString++;

            if (WideChar >= 0x80 && HIBYTE(NlsDbcsUnicodeToAnsiTable[WideChar]))
            {
                MbLength += sizeof(WCHAR);
            }
            else
            {
                MbLength++;
            }
        }

        *MbSize = MbLength;
    }

    /* Success */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUnicodeToOemN (PCHAR OemString,
                  ULONG OemSize,
                  PULONG ResultSize,
                  PWCHAR UnicodeString,
                  ULONG UnicodeSize)
{
   ULONG Size = 0;
   ULONG i;

   if (NlsMbOemCodePageTag == FALSE)
   {
      /* single-byte code page */
      if (UnicodeSize > (OemSize * sizeof(WCHAR)))
         Size = OemSize;
      else
         Size = UnicodeSize / sizeof(WCHAR);

      if (ResultSize != NULL)
         *ResultSize = Size;

      for (i = 0; i < Size; i++)
      {
         *OemString = NlsUnicodeToOemTable[*UnicodeString];
         OemString++;
         UnicodeString++;
      }
   }
   else
   {
      /* multi-byte code page */
      /* FIXME */

      USHORT WideChar;
      USHORT OemChar;

      for (i = OemSize, Size = UnicodeSize / sizeof(WCHAR); i && Size; i--, Size--)
      {
         WideChar = *UnicodeString++;

         if (WideChar < 0x80)
         {
            *OemString++ = LOBYTE(WideChar);
            continue;
         }

         OemChar = NlsDbcsUnicodeToOemTable[WideChar];

         if (!HIBYTE(OemChar))
         {
            *OemString++ = LOBYTE(OemChar);
            continue;
         }

         if (i >= 2)
         {
            *OemString++ = HIBYTE(OemChar);
            *OemString++ = LOBYTE(OemChar);
            i--;
         }
         else break;
      }

      if (ResultSize != NULL)
         *ResultSize = OemSize - i;
   }

   return STATUS_SUCCESS;
}




/*
 * @implemented
 */
WCHAR NTAPI
RtlUpcaseUnicodeChar(IN WCHAR Source)
{
   USHORT Offset;

   if (Source < 'a')
      return Source;

   if (Source <= 'z')
      return (Source - ('a' - 'A'));

   Offset = ((USHORT)Source >> 8) & 0xFF;
   Offset = NlsUnicodeUpcaseTable[Offset];

   Offset += ((USHORT)Source >> 4) & 0xF;
   Offset = NlsUnicodeUpcaseTable[Offset];

   Offset += ((USHORT)Source & 0xF);
   Offset = NlsUnicodeUpcaseTable[Offset];

   return Source + (SHORT)Offset;
}



/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUpcaseUnicodeToCustomCPN (IN PCPTABLEINFO CustomCP,
                             PCHAR CustomString,
                             ULONG CustomSize,
                             PULONG ResultSize,
                             PWCHAR UnicodeString,
                             ULONG UnicodeSize)
{
   WCHAR UpcaseChar;
   ULONG Size = 0;
   ULONG i;

   if (CustomCP->DBCSCodePage == 0)
   {
      /* single-byte code page */
      if (UnicodeSize > (CustomSize * sizeof(WCHAR)))
         Size = CustomSize;
      else
         Size = UnicodeSize / sizeof(WCHAR);

      if (ResultSize != NULL)
         *ResultSize = Size;

      for (i = 0; i < Size; i++)
      {
         UpcaseChar = RtlUpcaseUnicodeChar(*UnicodeString);
         *CustomString = ((PCHAR)CustomCP->WideCharTable)[UpcaseChar];
         CustomString++;
         UnicodeString++;
      }
   }
   else
   {
      /* multi-byte code page */
      /* FIXME */
      ASSERT(FALSE);
   }

   return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUpcaseUnicodeToMultiByteN (PCHAR MbString,
                              ULONG MbSize,
                              PULONG ResultSize,
                              PWCHAR UnicodeString,
                              ULONG UnicodeSize)
{
   WCHAR UpcaseChar;
   ULONG Size = 0;
   ULONG i;

   if (NlsMbCodePageTag == FALSE)
   {
      /* single-byte code page */
      if (UnicodeSize > (MbSize * sizeof(WCHAR)))
         Size = MbSize;
      else
         Size = UnicodeSize / sizeof(WCHAR);

      if (ResultSize != NULL)
         *ResultSize = Size;

      for (i = 0; i < Size; i++)
      {
         UpcaseChar = RtlUpcaseUnicodeChar(*UnicodeString);
         *MbString = NlsUnicodeToAnsiTable[UpcaseChar];
         MbString++;
         UnicodeString++;
      }
   }
   else
   {
      /* multi-byte code page */
      /* FIXME */
      ASSERT(FALSE);
   }

   return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUpcaseUnicodeToOemN (PCHAR OemString,
                        ULONG OemSize,
                        PULONG ResultSize,
                        PWCHAR UnicodeString,
                        ULONG UnicodeSize)
{
   WCHAR UpcaseChar;
   ULONG Size = 0;
   ULONG i;

   ASSERT(NlsUnicodeToOemTable != NULL);

   if (NlsMbOemCodePageTag == FALSE)
   {
      /* single-byte code page */
      if (UnicodeSize > (OemSize * sizeof(WCHAR)))
         Size = OemSize;
      else
         Size = UnicodeSize / sizeof(WCHAR);

      if (ResultSize != NULL)
         *ResultSize = Size;

      for (i = 0; i < Size; i++)
      {
         UpcaseChar = RtlUpcaseUnicodeChar(*UnicodeString);
         *OemString = NlsUnicodeToOemTable[UpcaseChar];
         OemString++;
         UnicodeString++;
      }
   }
   else
   {
      /* multi-byte code page */
      /* FIXME */

      USHORT WideChar;
      USHORT OemChar;

      for (i = OemSize, Size = UnicodeSize / sizeof(WCHAR); i && Size; i--, Size--)
      {
         WideChar = RtlUpcaseUnicodeChar(*UnicodeString++);

         if (WideChar < 0x80)
         {
            *OemString++ = LOBYTE(WideChar);
            continue;
         }

         OemChar = NlsDbcsUnicodeToOemTable[WideChar];

         if (!HIBYTE(OemChar))
         {
            *OemString++ = LOBYTE(OemChar);
            continue;
         }

         if (i >= 2)
         {
            *OemString++ = HIBYTE(OemChar);
            *OemString++ = LOBYTE(OemChar);
            i--;
         }
         else break;
      }

      if (ResultSize != NULL)
         *ResultSize = OemSize - i;
   }

   return STATUS_SUCCESS;
}



/*
 * @unimplemented
 */
CHAR NTAPI
RtlUpperChar (IN CHAR Source)
{
   WCHAR Unicode;
   CHAR Destination;

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
        if (NlsMbCodePageTag == FALSE)
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
