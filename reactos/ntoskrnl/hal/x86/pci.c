/* $Id: pci.c,v 1.5 2000/08/17 17:42:53 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/pci.c
 * PURPOSE:         Interfaces to the PCI bus
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 *                  Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  05/06/1998: Created
 *                  17/08/2000: Added preliminary pci bus scanner
 */

/*
 * NOTES: Sections copied from the Linux pci support
 */

/* INCLUDES ***************************************************************/

#include <ddk/ntddk.h>
#include <internal/i386/hal.h>

#define NDEBUG
#include <internal/debug.h>


/* MACROS *****************************************************************/

/* access type 1 macros */
#define PCI_FUNC(devfn)		((devfn) & 0x07)
#define CONFIG_CMD(bus, device_fn, where)	\
	(0x80000000 | (bus << 16) | (device_fn << 8) | (where & ~3))

#define PCIBIOS_SUCCESSFUL		0x00
#define PCIBIOS_DEVICE_NOT_FOUND	0x86
#define PCIBIOS_BAD_REGISTER_NUMBER	0x87

// access type 2 macros
#define IOADDR(devfn, where)   ((0xC000 | ((devfn & 0x78) << 5)) + where)
#define FUNC(devfn)            (((devfn & 7) << 1) | 0xf0)


/* FUNCTIONS **************************************************************/

/*
 * Bus access type 1 (recommended)
 */
static int
ReadPciConfigUcharType1(UCHAR Bus,
                        UCHAR device_fn,
                        UCHAR where,
                        PUCHAR Value)
{
   WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus,device_fn,where));
   *Value = READ_PORT_UCHAR((PUCHAR)0xCFC + (where&3));
   return PCIBIOS_SUCCESSFUL;
}

#if 0
static int
ReadPciConfigUshortType1(UCHAR Bus,
                         UCHAR device_fn,
                         UCHAR where,
                         PUSHORT Value)
{
   if ((where & 1) != 0)
     {
	return PCIBIOS_BAD_REGISTER_NUMBER;
     }

   WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus,device_fn,where));
   *value = READ_PORT_USHORT((PUSHORT)0xCFC + (where&1));
   return PCIBIOS_SUCCESSFUL;
}
#endif

static int
ReadPciConfigUlongType1(UCHAR Bus,
                        UCHAR device_fn,
                        UCHAR where,
                        PULONG Value)
{
   if ((where & 3) != 0)
     {
	return PCIBIOS_BAD_REGISTER_NUMBER;
     }

   WRITE_PORT_ULONG((PULONG)0xCF8, CONFIG_CMD(Bus,device_fn,where));
   *Value = READ_PORT_ULONG((PULONG)0xCFC);
   return PCIBIOS_SUCCESSFUL;
}

static void
ScanPciBusType1(ULONG Bus)
{
   USHORT dev_fn;
   UCHAR hdr_type;
   ULONG foo;

   for (dev_fn = 0; dev_fn < 256; dev_fn++)
     {
	if (PCI_FUNC(dev_fn) == 0)
	  {
	     /* 0E=PCI_HEADER_TYPE */
	     ReadPciConfigUcharType1(Bus, dev_fn, 0x0E, &hdr_type);
	  }
	else if ((hdr_type & 0x80) == 0)
	  {
	     /* not a multi-function device */
	     continue;
	  }

	/* 00=PCI_VENDOR_ID */
	ReadPciConfigUlongType1(Bus, dev_fn, 0x00, &foo);
	/* some broken boards return 0 if a slot is empty: */
	if (foo == 0xFFFFFFFF || foo == 0)
	  {
	     hdr_type = 0;
	     continue;
	  }

	DbgPrint("dev_fn=%3u, Vendor 0x%04lX, Device 0x%04lX\n",
		 dev_fn, foo & 0xFFFF, (foo >> 16) & 0xFFFF);
     }
}

static NTSTATUS
ScanPciBussesType1(VOID)
{
   ULONG i;
   ULONG temp;

   DPRINT("ScanPciBussesType1()\n");

   DPRINT("Checking if configuration type 1 works\n");
   WRITE_PORT_UCHAR((PUCHAR)0xCFB, 0x01);
   temp = READ_PORT_ULONG((PULONG)0xCF8);
   WRITE_PORT_ULONG((PULONG)0xCF8, 0x80000000);
   if (READ_PORT_ULONG((PULONG)0xCF8) != 0x80000000)
     {
	WRITE_PORT_ULONG((PULONG)0xCF8, temp);
	DPRINT("No pci configuration type 1\n");
	return STATUS_UNSUCCESSFUL;
     }

   DPRINT("Using configuration type 1\n");
   WRITE_PORT_ULONG((PULONG)0xCF8, temp);
   for (i = 0; i < 8; i++)
     {
	DPRINT("Scanning PCI bus %u...\n", i);
	ScanPciBusType1(i);
     }
   WRITE_PORT_ULONG((PULONG)0xCF8, temp);

   return STATUS_SUCCESS;
}


/*
 * Bus access type 2 (should not be used any longer)
 */
static int
ReadPciConfigUcharType2 (UCHAR Bus,
                         UCHAR device_fn,
                         UCHAR where,
                         PUCHAR Value)
{
   if ((device_fn & 0x80) != 0)
     {
	return PCIBIOS_DEVICE_NOT_FOUND;
     }

   WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(device_fn));
   WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
   *Value = READ_PORT_UCHAR((PUCHAR)(IOADDR(device_fn,where)));
   WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);

   return PCIBIOS_SUCCESSFUL;
}

#if 0
static int
ReadPciConfigUshortType2(UCHAR Bus,
                         UCHAR device_fn,
                         UCHAR where,
                         PUSHORT Value)
{
   if ((device_fn & 0x80) != 0)
     {
	return PCIBIOS_DEVICE_NOT_FOUND;
     }

   WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(device_fn));
   WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
   *Value = READ_PORT_USHORT((PUSHORT)(IOADDR(device_fn,where)));
   WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);

   return PCIBIOS_SUCCESSFUL;
}
#endif

static int
ReadPciConfigUlongType2(UCHAR Bus,
                        UCHAR device_fn,
                        UCHAR where,
                        PULONG Value)
{
   if ((device_fn & 0x80) != 0)
     {
	return PCIBIOS_DEVICE_NOT_FOUND;
     }

   WRITE_PORT_UCHAR((PUCHAR)0xCF8, FUNC(device_fn));
   WRITE_PORT_UCHAR((PUCHAR)0xCFA, Bus);
   *Value = READ_PORT_ULONG((PULONG)(IOADDR(device_fn,where)));
   WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0);

   return PCIBIOS_SUCCESSFUL;
}

static void
ScanPciBusType2(ULONG Bus)
{
   USHORT dev_fn;
   UCHAR hdr_type;
   ULONG foo;

   for (dev_fn = 0; dev_fn < 256; dev_fn++)
     {
	if (PCI_FUNC(dev_fn) == 0)
	  {
	     /* 0E=PCI_HEADER_TYPE */
	     ReadPciConfigUcharType2(Bus, dev_fn, 0x0E, &hdr_type);
	  }
	else if ((hdr_type & 0x80) == 0)
	  {
	     /* not a multi-function device */
	     continue;
	}

	/* 00=PCI_VENDOR_ID */
	ReadPciConfigUlongType2(Bus, dev_fn, 0x00, &foo);
	/* some broken boards return 0 if a slot is empty: */
	if (foo == 0xFFFFFFFF || foo == 0)
	  {
	     hdr_type = 0;
	     continue;
	  }

	DbgPrint("dev_fn=%3u, Vendor 0x%04lX, Device 0x%04lX\n",
		 dev_fn, foo & 0xFFFF, (foo >> 16) & 0xFFFF);
    }
}

static NTSTATUS
ScanPciBussesType2(VOID)
{
   ULONG i;

   DPRINT("Checking if configuration type 2 works\n");
   WRITE_PORT_UCHAR((PUCHAR)0xCFB, 0x00);
   WRITE_PORT_UCHAR((PUCHAR)0xCF8, 0x00);
   WRITE_PORT_UCHAR((PUCHAR)0xCFA, 0x00);
   if (READ_PORT_UCHAR((PUCHAR)0xCF8) != 0x00 ||
       READ_PORT_UCHAR((PUCHAR)0xCFB) != 0x00)
     {
	DPRINT("No pci configuration type 2\n");
	return STATUS_UNSUCCESSFUL;
     }

   DPRINT("Using configuration type 2\n");
   for (i = 0; i < 8; i++)
     {
	DPRINT("Scanning PCI bus %u...\n", i);
	ScanPciBusType2(i);
     }

   return STATUS_UNSUCCESSFUL;
}


VOID HalpInitPciBus (VOID)
{
   NTSTATUS Status;

   DPRINT("HalpInitPciBus()\n");

   Status = ScanPciBussesType1();
   if (NT_SUCCESS(Status))
     {
	return;
     }

   Status = ScanPciBussesType2();
   if (NT_SUCCESS(Status))
     {
	return;
     }

   DbgPrint("No pci bus found\n");
}

/* EOF */
