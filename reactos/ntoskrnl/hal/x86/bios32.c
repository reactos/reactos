/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/pci
 * PURPOSE:         Interfaces to BIOS32 interface
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *               05/06/98: Created
 */

/*
 * NOTES: Sections copied from the Linux pci support
 */

/* INCLUDES ***************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <string.h>
#include <internal/string.h>
#include <internal/mmhal.h>
#include <internal/i386/segment.h>

/* TYPES ******************************************************************/

typedef struct
{
   /*
    * "_32_" if present
    */
   unsigned int signature;
   
   /*
    * Entry point (physical address)
    */
   unsigned long int entry;
   
   /*
    * Revision level
    */
   unsigned char revision;
   
   /*
    * Length in paragraphs
    */
   unsigned char length;
   
   /*
    * Checksum (so all bytes add up to zero)
    */
   unsigned char checksum;
   
   unsigned char reserved[5];
} bios32;

BOOLEAN bios32_detected = FALSE;

static struct
{
   unsigned long address;
   unsigned short segment;
} bios32_indirect = {0,KERNEL_CS};

/* FUNCTIONS **************************************************************/

#define BIOS32_SIGNATURE (('_' << 0)+('3'<<8)+('2'<<16)+('_'<<24))

BOOL static checksum(bios32* service_entry)
/*
 * FUNCTION: Checks the checksum of a bios32 service entry
 * ARGUMENTS:
 *         service_entry = Pointer to the service entry
 * RETURNS: True if the sum of the bytes in the entry was zero
 *          False otherwise
 */
{
   unsigned char* p = (unsigned char *)service_entry;
   int i;
   unsigned char sum=0;
   
   for (i=0; i<(service_entry->length*16); i++)
     {
	sum=sum+p[i];
     }
//   printk("sum = %d\n",sum);
   if (sum==0)
     {
	return(TRUE);
     }
   return(FALSE);
}

BOOLEAN Hal_bios32_is_service_present(ULONG service)
{
   unsigned char return_code;
   unsigned int address;
   unsigned int length;
   unsigned int entry;
   
   __asm__("lcall (%%edi)"
	   : "=a" (return_code),
	     "=b" (address),
	     "=c" (length),
	     "=d" (entry)
	   : "0" (service),
	     "1" (0),
	     "D" (&bios32_indirect));
   if (return_code==0)
     {
	return(address+entry);
     }
   return(0);
}

VOID Hal_bios32_probe()
/*
 * FUNCTION: Probes for an BIOS32 extension
 * RETURNS: True if detected
 */
{
   int i;
   
   for (i=0xe0000;i<=0xffff0;i++)
     {
	bios32* service_entry = (bios32 *)physical_to_linear(i);
	if ( service_entry->signature != BIOS32_SIGNATURE )
	  {
	     continue;
	  }
//        printk("Signature detected at %x\n",i);
	if (!checksum(service_entry))
	  {
	     continue;
	  }
        DbgPrint("ReactOS: BIOS32 detected at %x\n",i);
	bios32_indirect.address = service_entry->entry;
	bios32_detected=TRUE;
     }
}
