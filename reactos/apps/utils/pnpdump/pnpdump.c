/*
 * pnpdump - PnP BIOS information dumper
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct _CM_PNP_BIOS_DEVICE_NODE
{
  USHORT Size;
  UCHAR Node;
  ULONG ProductId;
  UCHAR DeviceType[3];
  USHORT DeviceAttributes;
} PACKED CM_PNP_BIOS_DEVICE_NODE,*PCM_PNP_BIOS_DEVICE_NODE;

typedef struct _CM_PNP_BIOS_INSTALLATION_CHECK
{
  UCHAR Signature[4];             // $PnP (ascii)
  UCHAR Revision;
  UCHAR Length;
  USHORT ControlField;
  UCHAR Checksum;
  ULONG EventFlagAddress;         // Physical address
  USHORT RealModeEntryOffset;
  USHORT RealModeEntrySegment;
  USHORT ProtectedModeEntryOffset;
  ULONG ProtectedModeCodeBaseAddress;
  ULONG OemDeviceId;
  USHORT RealModeDataBaseAddress;
  ULONG ProtectedModeDataBaseAddress;
} PACKED CM_PNP_BIOS_INSTALLATION_CHECK, *PCM_PNP_BIOS_INSTALLATION_CHECK;

typedef struct _PNP_ID_NAME_
{
  char *PnpId;
  char *DeviceName;
} PNP_ID_NAME, *PPNP_ID_NAME;


static char Hex[] = "0123456789ABCDEF";

static PNP_ID_NAME PnpName[] =
{
  /* Interrupt Controllers */
  {"PNP0000", "AT Interrupt Controller"},
  {"PNP0001", "EISA Interrupt Controller"},
  {"PNP0002", "MCA Interrupt Controller"},
  {"PNP0003", "APIC"},
  {"PNP0004", "Cyrix SLiC MP Interrupt Controller"},

  /* Timers */
  {"PNP0100", "AT Timer"},
  {"PNP0101", "EISA Timer"},
  {"PNP0102", "MCA Timer"},

  /* DMA Controllers */
  {"PNP0200", "AT DMA Controller"},
  {"PNP0201", "EISA DMA Controller"},
  {"PNP0202", "MCA DMA Controller"},

  /* Keyboards */
  {"PNP0300", "IBM PC/XT Keyboard (83 keys)"},
  {"PNP0301", "IBM PC/AT Keyboard (86 keys)"},
  {"PNP0302", "IBM PC/XT Keyboard (84 keys)"},
  {"PNP0303", "IBM Enhanced (101/102 keys)"},
  {"PNP0304", "Olivetti Keyboard (83 keys)"},
  {"PNP0305", "Olivetti Keyboard (102 keys)"},
  {"PNP0306", "Olivetti Keyboard (86 keys)"},
  {"PNP0307", "Microsoft Windows(R) Keyboard"},
  {"PNP0308", "General Input Device Emulation Interface (GIDEI) legacy"},
  {"PNP0309", "Olivetti Keyboard (A101/102 key)"},
  {"PNP030A", "AT&T 302 keyboard"},
  {"PNP030B", "Reserved by Microsoft"},
  {"PNP0320", "Japanese 101-key keyboard"},
  {"PNP0321", "Japanese AX keyboard"},
  {"PNP0322", "Japanese 106-key keyboard A01"},
  {"PNP0323", "Japanese 106-key keyboard 002/003"},
  {"PNP0324", "Japanese 106-key keyboard 001"},
  {"PNP0325", "Japanese Toshiba Desktop keyboard"},
  {"PNP0326", "Japanese Toshiba Laptop keyboard"},
  {"PNP0327", "Japanese Toshiba Notebook keyboard"},
  {"PNP0340", "Korean 84-key keyboard"},
  {"PNP0341", "Korean 86-key keyboard"},
  {"PNP0342", "Korean Enhanced keyboard"},
  {"PNP0343", "Korean Enhanced keyboard 101b"},
  {"PNP0343", "Korean Enhanced keyboard 101c"},
  {"PNP0344", "Korean Enhanced keyboard 103"},

  /* Parallel Ports */
  {"PNP0400", "Standard LPT printer port"},
  {"PNP0401", "ECP printer port"},

  /* Serial Ports */
  {"PNP0500", "Standard PC COM port"},
  {"PNP0501", "16550A-compatible COM port"},
  {"PNP0510", "Generic IRDA-compatible port"},

  /* Harddisk Controllers */
  {"PNP0600", "Generic ESDI/ATA/IDE harddisk controller"},
  {"PNP0601", "Plus Hardcard II"},
  {"PNP0602", "Plus Hardcard IIXL/EZ"},
  {"PNP0603", "Generic IDE supporting Microsoft Device Bay Specification"},

  /* Floppy Controllers */
  {"PNP0700", "PC standard floppy disk controller"},
  {"PNP0701", "Standard floppy controller supporting MS Device Bay Specification"},

  /* obsolete devices */
  {"PNP0800", "Microsoft Sound System compatible device"},

  /* Display Adapters */
  {"PNP0900", "VGA Compatible"},
  {"PNP0901", "Video Seven VRAM/VRAM II/1024i"},
  {"PNP0902", "8514/A Compatible"},
  {"PNP0903", "Trident VGA"},
  {"PNP0904", "Cirrus Logic Laptop VGA"},
  {"PNP0905", "Cirrus Logic VGA"},
  {"PNP0906", "Tseng ET4000"},
  {"PNP0907", "Western Digital VGA"},
  {"PNP0908", "Western Digital Laptop VGA"},
  {"PNP0909", "S3 Inc. 911/924"},
  {"PNP090A", "ATI Ultra Pro/Plus (Mach 32)"},
  {"PNP090B", "ATI Ultra (Mach 8)"},
  {"PNP090C", "XGA Compatible"},
  {"PNP090D", "ATI VGA Wonder"},
  {"PNP090E", "Weitek P9000 Graphics Adapter"},
  {"PNP090F", "Oak Technology VGA"},
  {"PNP0910", "Compaq QVision"},
  {"PNP0911", "XGA/2"},
  {"PNP0912", "Tseng Labs W32/W32i/W32p"},
  {"PNP0913", "S3 Inc. 801/928/964"},
  {"PNP0914", "Cirrus Logic 5429/5434 (memory mapped)"},
  {"PNP0915", "Compaq Advanced VGA (AVGA)"},
  {"PNP0916", "ATI Ultra Pro Turbo (Mach64)"},
  {"PNP0917", "Reserved by Microsoft"},
  {"PNP0918", "Matrox MGA"},
  {"PNP0919", "Compaq QVision 2000"},
  {"PNP091A", "Tseng W128"},
  {"PNP0930", "Chips & Technologies Super VGA"},
  {"PNP0931", "Chips & Technologies Accelerator"},
  {"PNP0940", "NCR 77c22e Super VGA"},
  {"PNP0941", "NCR 77c32blt"},
  {"PNP09FF", "Plug and Play Monitors (VESA DDC)"},

  /* Peripheral Buses */
  {"PNP0A00", "ISA Bus"},
  {"PNP0A01", "EISA Bus"},
  {"PNP0A02", "MCA Bus"},
  {"PNP0A03", "PCI Bus"},
  {"PNP0A04", "VESA/VL Bus"},
  {"PNP0A05", "Generic ACPI Bus"},
  {"PNP0A06", "Generic ACPI Extended-IO Bus (EIO bus)"},

  /* System devices */
  {"PNP0800", "AT-style speaker sound"},
  {"PNP0B00", "AT Real-Time Clock"},
  {"PNP0C00", "Plug and Play BIOS (only created by the root enumerator)"},
  {"PNP0C01", "System Board"},
  {"PNP0C02", "General Plug and Play motherboard registers."},
  {"PNP0C03", "Plug and Play BIOS Event Notification Interrupt"},
  {"PNP0C04", "Math Coprocessor"},
  {"PNP0C05", "APM BIOS (Version independent)"},
  {"PNP0C06", "Reserved for identification of early Plug and Play BIOS implementation"},
  {"PNP0C07", "Reserved for identification of early Plug and Play BIOS implementation"},
  {"PNP0C08", "ACPI system board hardware"},
  {"PNP0C09", "ACPI Embedded Controller"},
  {"PNP0C0A", "ACPI Control Method Battery"},
  {"PNP0C0B", "ACPI Fan"},
  {"PNP0C0C", "ACPI power button device"},
  {"PNP0C0D", "ACPI lid device"},
  {"PNP0C0E", "ACPI sleep button device"},
  {"PNP0C0F", "PCI interrupt link device"},
  {"PNP0C10", "ACPI system indicator device"},
  {"PNP0C11", "ACPI thermal zone"},
  {"PNP0C12", "Device Bay Controller"},

  /* PCMCIA Controllers */
  {"PNP0E00", "Intel 82365-Compatible PCMCIA Controller"},
  {"PNP0E01", "Cirrus Logic CL-PD6720 PCMCIA Controller"},
  {"PNP0E02", "VLSI VL82C146 PCMCIA Controller"},
  {"PNP0E03", "Intel 82365-compatible CardBus controller"},

  /* Mice */
  {"PNP0F00", "Microsoft Bus Mouse"},
  {"PNP0F01", "Microsoft Serial Mouse"},
  {"PNP0F02", "Microsoft InPort Mouse"},
  {"PNP0F03", "Microsoft PS/2-style Mouse"},
  {"PNP0F04", "Mouse Systems Mouse"},
  {"PNP0F05", "Mouse Systems 3-Button Mouse (COM2)"},
  {"PNP0F06", "Genius Mouse (COM1)"},
  {"PNP0F07", "Genius Mouse (COM2)"},
  {"PNP0F08", "Logitech Serial Mouse"},
  {"PNP0F09", "Microsoft BallPoint Serial Mouse"},
  {"PNP0F0A", "Microsoft Plug and Play Mouse"},
  {"PNP0F0B", "Microsoft Plug and Play BallPoint Mouse"},
  {"PNP0F0C", "Microsoft-compatible Serial Mouse"},
  {"PNP0F0D", "Microsoft-compatible InPort-compatible Mouse"},
  {"PNP0F0E", "Microsoft-compatible PS/2-style Mouse"},
  {"PNP0F0F", "Microsoft-compatible Serial BallPoint-compatible Mouse"},
  {"PNP0F10", "Texas Instruments QuickPort Mouse"},
  {"PNP0F11", "Microsoft-compatible Bus Mouse"},
  {"PNP0F12", "Logitech PS/2-style Mouse"},
  {"PNP0F13", "PS/2 Port for PS/2-style Mice"},
  {"PNP0F14", "Microsoft Kids Mouse"},
  {"PNP0F15", "Logitech bus mouse"},
  {"PNP0F16", "Logitech SWIFT device"},
  {"PNP0F17", "Logitech-compatible serial mouse"},
  {"PNP0F18", "Logitech-compatible bus mouse"},
  {"PNP0F19", "Logitech-compatible PS/2-style Mouse"},
  {"PNP0F1A", "Logitech-compatible SWIFT Device"},
  {"PNP0F1B", "HP Omnibook Mouse"},
  {"PNP0F1C", "Compaq LTE Trackball PS/2-style Mouse"},
  {"PNP0F1D", "Compaq LTE Trackball Serial Mouse"},
  {"PNP0F1E", "Microsoft Kids Trackball Mouse"},
  {"PNP0F1F", "Reserved by Microsoft Input Device Group"},
  {"PNP0F20", "Reserved by Microsoft Input Device Group"},
  {"PNP0F21", "Reserved by Microsoft Input Device Group"},
  {"PNP0F22", "Reserved by Microsoft Input Device Group"},
  {"PNP0F23", "Reserved by Microsoft Input Device Group"},
  {"PNP0FFF", "Reserved by Microsoft Systems"},

  /* List Terminator */
  {NULL, NULL}
};


/* FUNCTIONS ****************************************************************/

static char *
GetDeviceName(char *PnpId)
{
  PPNP_ID_NAME IdName;

  IdName = PnpName;
  while (IdName->PnpId != NULL)
    {
      if (!strcmp(IdName->PnpId, PnpId))
	return IdName->DeviceName;

      IdName++;
    }

  return "Unknown Device";
}


LONG
GetPnpKey(PHKEY PnpKey)
{
  LONG lError;
  char szBuffer[80];
  HKEY hAdapterKey;
  HKEY hBusKey;
  DWORD dwBus;
  DWORD dwType;
  DWORD dwSize;

  *PnpKey = 0;

  lError = RegOpenKey(HKEY_LOCAL_MACHINE,
		      "HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter",
		      &hAdapterKey);
  if (lError != ERROR_SUCCESS)
    return 0;

  /* Enumerate buses */
  for (dwBus = 0; ; dwBus++)
    {
      sprintf(szBuffer, "%lu", dwBus);

      lError = RegOpenKey(hAdapterKey,
			  szBuffer,
			  &hBusKey);
      if (lError != ERROR_SUCCESS)
	{
	  RegCloseKey(hAdapterKey);
	  return lError;
	}

      dwSize = 80;
      lError = RegQueryValueEx(hBusKey,
			       "Identifier",
			       NULL,
			       &dwType,
			       szBuffer,
			       &dwSize);
      if (lError != ERROR_SUCCESS)
	{
	  RegCloseKey(hBusKey);
	  RegCloseKey(hAdapterKey);
	  return lError;
	}

      if (dwType == REG_SZ && stricmp(szBuffer, "pnp bios") == 0)
	{
	  *PnpKey = hBusKey;
	  RegCloseKey(hAdapterKey);
	  return ERROR_SUCCESS;
	}

      RegCloseKey(hBusKey);
    }

  return 1;
}


static VOID
PnpDecodeIrq(unsigned char *Ptr)
{
  USHORT IrqMask;
  int i;

  IrqMask = *Ptr;
  Ptr++;
  IrqMask |= (*Ptr << 8);

  printf("      IRQs:");

  for (i = 0; i < 16; i++)
    {
      if (IrqMask & (1 << i))
	{
	  printf(" %u", i);
	}
    }

  printf("\n");
}


static VOID
PnpDecodeDma(unsigned char *Ptr)
{
  unsigned char DmaChannel;
  unsigned char DmaStatus;
  int i;

  DmaChannel = *Ptr;
  Ptr++;
  DmaStatus = *Ptr;

  printf("      DMAs:");

  for (i = 0; i < 8; i++)
    {
      if (DmaChannel & (1 << i))
	{
	  printf(" %u", i);
	}
    }

  printf("\n");
}


static VOID
PnpDecodeIoPort(unsigned char *Ptr)
{
  USHORT MinBase;
  USHORT MaxBase;
  UCHAR Align;
  UCHAR Length;

  // Info = *Ptr;
  Ptr++;
  MinBase = *Ptr;
  Ptr++;
  MinBase += (*Ptr << 8);
  Ptr++;
  MaxBase = *Ptr;
  Ptr++;
  MaxBase += (*Ptr << 8);
  Ptr++;
  Align = *Ptr;
  Ptr++;
  Length = *Ptr;

  printf("  I/O Port descriptor\n");
  printf("    MinBase 0x%x  MaxBase 0x%x  Align %u  Length %u\n",
	 MinBase, MaxBase, Align, Length);
}


static VOID
PnpDecodeFixedIoPort(unsigned char *Ptr)
{
  USHORT IoPort;
  UCHAR Length;

  IoPort = *Ptr;
  Ptr++;
  IoPort += (*Ptr << 8);
  Ptr++;
  Length = *Ptr;

  printf("  Fixed I/O Port descriptor\n");
  printf("    PortBase 0x%hx  Length 0x%x\n",
	 IoPort, Length);

#if 0
  if (Length == 1)
    {
      printf("  Fixed location I/O Port descriptor: 0x%x\n",
	     IoPort);
    }
  else
    {
      printf("  Fixed location I/O Port descriptor: 0x%x - 0x%x\n",
	     IoPort,
	     IoPort + Length - 1);
    }
#endif
}


static VOID
PnpDecodeMemory16(unsigned char *Ptr)
{
  USHORT DescLength;
  UCHAR Info;
  USHORT MinBase;
  USHORT MaxBase;
  USHORT Align;
  USHORT Length;

  Info = *Ptr;
  Ptr++;

  MinBase = *Ptr;
  Ptr++;
  MinBase += (*Ptr << 8);
  Ptr++;

  MaxBase = *Ptr;
  Ptr++;
  MaxBase += (*Ptr << 8);
  Ptr++;

  Align = *Ptr;
  Ptr++;
  Align += (*Ptr << 8);
  Ptr++;

  Length = *Ptr;
  Ptr++;
  Length += (*Ptr << 8);

  printf("  16-Bit memory range descriptor\n");
  printf("    MinBase 0x%hx  MaxBase 0x%hx  Align 0x%hx  Length 0x%hx  Flags 0x%02x\n",
	 MinBase, MaxBase, Align,Length, Info);
}


static VOID
PnpDecodeMemory32(unsigned char *Ptr)
{
  USHORT DescLength;
  UCHAR Info;
  ULONG MinBase;
  ULONG MaxBase;
  ULONG Align;
  ULONG Length;

  Info = *Ptr;
  Ptr++;

  MinBase = *Ptr;
  Ptr++;
  MinBase += (*Ptr << 8);
  Ptr++;
  MinBase += (*Ptr << 16);
  Ptr++;
  MinBase += (*Ptr << 24);
  Ptr++;

  MaxBase = *Ptr;
  Ptr++;
  MaxBase += (*Ptr << 8);
  Ptr++;
  MaxBase += (*Ptr << 16);
  Ptr++;
  MaxBase += (*Ptr << 24);
  Ptr++;

  Align = *Ptr;
  Ptr++;
  Align += (*Ptr << 8);
  Ptr++;
  Align += (*Ptr << 16);
  Ptr++;
  Align += (*Ptr << 24);
  Ptr++;

  Length = *Ptr;
  Ptr++;
  Length += (*Ptr << 8);
  Ptr++;
  Length += (*Ptr << 16);
  Ptr++;
  Length += (*Ptr << 24);

  printf("  32-Bit memory range descriptor\n");
  printf("    MinBase 0x%lx  MaxBase 0x%lx  Align 0x%lx  Length 0x%lx  Flags 0x%02x\n",
	 MinBase, MaxBase, Align,Length, Info);
}


static VOID
PnpDecodeFixedMemory(unsigned char *Ptr)
{
  USHORT DescLength;
  UCHAR Info;
  ULONG Base;
  ULONG Length;

  Info = *Ptr;
  Ptr++;

  Base = *Ptr;
  Ptr++;
  Base += (*Ptr << 8);
  Ptr++;
  Base += (*Ptr << 16);
  Ptr++;
  Base += (*Ptr << 24);
  Ptr++;

  Length = *Ptr;
  Ptr++;
  Length += (*Ptr << 8);
  Ptr++;
  Length += (*Ptr << 16);
  Ptr++;
  Length += (*Ptr << 24);

  printf("  32-Bit fixed location memory range descriptor\n");
  printf("    Base 0x%lx  Length 0x%lx  Flags 0x%02x\n",
	 Base, Length, Info);
}


void PrintDeviceData (PCM_PNP_BIOS_DEVICE_NODE DeviceNode)
{
  unsigned char PnpId[8];
  unsigned char *Ptr;
  unsigned int TagSize;
  unsigned int TagType;

  unsigned char Id[4];

  printf ("Node: %x  Size %hu (0x%hx)\n",
	  DeviceNode->Node,
	  DeviceNode->Size,
	  DeviceNode->Size);

  memcpy(Id, &DeviceNode->ProductId, 4);

  PnpId[0] = ((Id[0] >> 2) & 0x1F) + 0x40;
  PnpId[1] = ((Id[0] << 3) & 0x18) +
	     ((Id[1] >> 5) & 0x07) + 0x40;
  PnpId[2] = (Id[1] & 0x1F) + 0x40;

  PnpId[3] = Hex[(Id[2] >> 4) & 0xF];
  PnpId[4] = Hex[Id[2] & 0x0F];

  PnpId[5] = Hex[(Id[3] >> 4) & 0x0F];
  PnpId[6] = Hex[Id[3] & 0x0F];
  PnpId[7] = 0;

  printf("  '%s' (%s)\n",
	 PnpId, GetDeviceName(PnpId));

  if (DeviceNode->Size > sizeof(CM_PNP_BIOS_DEVICE_NODE))
    {
      Ptr = (unsigned char *)(DeviceNode + 1);
      while (TRUE)
	{
	  if (*Ptr & 0x80)
	    {
	      TagType = *Ptr & 0x7F;
	      Ptr++;
	      TagSize = *Ptr;
	      Ptr++;
	      TagSize += (*Ptr << 16);
	      Ptr++;


	      switch (TagType)
		{
		  case 1:
		    PnpDecodeMemory16(Ptr);
		    break;

		  case 5:
		    PnpDecodeMemory32(Ptr);
		    break;

		  case 6:
		    PnpDecodeFixedMemory(Ptr);
		    break;

		  default:
		    printf("      Large tag: type %u  size %u\n",
			   TagType,
			   TagSize);
		    break;
		}
	    }
	  else
	    {
	      TagType = (*Ptr >> 3) & 0x0F;
	      TagSize = *Ptr & 0x07;
	      Ptr++;

	      switch (TagType)
		{
		  case 2:
		    printf("      Logical device ID\n");
		    break;

		  case 3:
		    printf("      Compatible device ID\n");
		    break;

		  case 4:
		    PnpDecodeIrq(Ptr);
		    break;

		  case 5:
		    PnpDecodeDma(Ptr);
		    break;

		  case 8:
		    PnpDecodeIoPort(Ptr);
		    break;

		  case 9:
		    PnpDecodeFixedIoPort(Ptr);
		    break;

		  case 0x0F: /* end tag */
		    break;

		  default:
		    printf("      Small tag: type %u  size %u\n",
			   TagType,
			   TagSize);
		    break;
		}

	      /* end tag */
	      if (TagType == 0x0F)
		break;
	    }

	  Ptr = Ptr + TagSize;
	}
    }
}


int main (int argc, char *argv[])
{
  LONG lError;
  HKEY hPnpKey;
  DWORD dwType;
  DWORD dwSize;
  DWORD i;
  PCM_FULL_RESOURCE_DESCRIPTOR lpBuffer;
  PCM_PNP_BIOS_INSTALLATION_CHECK lpPnpInst;
  PCM_PNP_BIOS_DEVICE_NODE lpDevNode;

  DWORD dwDataSize, dwResourceSize;

  hPnpKey = 0;

  lError = GetPnpKey(&hPnpKey);
  if (lError != ERROR_SUCCESS)
    {
      printf("Failed to get PnP-BIOS key\n");
      return 0;
    }

  if (hPnpKey != 0)
    {
      printf("Found PnP-BIOS key\n");
    }

  /* Allocate buffer */
  dwSize = 1024;
  lpBuffer = malloc(dwSize);

  lError = RegQueryValueEx(hPnpKey,
			   "Configuration Data",
			   NULL,
			   &dwType,
			   (LPSTR)lpBuffer,
			   &dwSize);
  if (lError != ERROR_SUCCESS)
    {
      if (lError == ERROR_MORE_DATA)
	{
	  printf("Need to resize buffer to %lu\n", dwSize);

	}

      printf("Failed to read 'Configuration Data' value\n");
      free (lpBuffer);
      RegCloseKey(hPnpKey);
      return 0;
    }

//  printf ("Data size: %lu\n", dwSize);

  RegCloseKey(hPnpKey);

//  printf("Resource count %lu\n", lpBuffer->PartialResourceList.Count);

  if (lpBuffer->PartialResourceList.Count == 0)
    {
      printf("Invalid resource count!\n");
      free (lpBuffer);
      return 0;
    }

//  printf("lpBuffer %p\n", lpBuffer);

  dwResourceSize = lpBuffer->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize;
//  printf("ResourceSize: %lu\n", dwResourceSize);

  lpPnpInst = (PCM_PNP_BIOS_INSTALLATION_CHECK)
	((DWORD)(&lpBuffer->PartialResourceList.PartialDescriptors[0]) +
	  sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

//  printf("lpPnpInst %p\n", lpPnpInst);

  printf("Signature '%.4s'\n", lpPnpInst->Signature);
  if (strncmp(lpPnpInst->Signature, "$PnP", 4))
    {
      printf("Error: Invalid PnP signature\n");
      free(lpBuffer);
      return 0;
    }

//  printf("InstCheck length: %lu\n", lpPnpInst->Length);

  dwDataSize = sizeof(CM_PNP_BIOS_INSTALLATION_CHECK);
  lpDevNode = (PCM_PNP_BIOS_DEVICE_NODE)((DWORD)lpPnpInst + sizeof(CM_PNP_BIOS_INSTALLATION_CHECK));

  if (lpDevNode->Size == 0)
    {
      printf("Error: Device node size is zero!\n");
      return 0;
    }

#if 0
      printf ("Node: %x  Size %hu (0x%hx)\n",
	      lpDevNode->Node,
	      lpDevNode->Size,
	      lpDevNode->Size);

  printf("Done.\n");
return 0;
#endif


  while (dwDataSize < dwResourceSize)
    {
      if (lpDevNode->Size == 0)
	break;

      printf ("Node: %x  Size %hu (0x%hx)\n",
	      lpDevNode->Node,
	      lpDevNode->Size,
	      lpDevNode->Size);

      dwDataSize += lpDevNode->Size;
      lpDevNode = (PCM_PNP_BIOS_DEVICE_NODE)((DWORD)lpDevNode + lpDevNode->Size);
    }

  printf ("\n Press any key...\n");
  getch();

  dwDataSize = sizeof(CM_PNP_BIOS_INSTALLATION_CHECK);
  lpDevNode = (PCM_PNP_BIOS_DEVICE_NODE)((DWORD)lpPnpInst + sizeof(CM_PNP_BIOS_INSTALLATION_CHECK));

  while (dwDataSize < dwResourceSize)
    {
      PrintDeviceData (lpDevNode);

      printf ("\n Press any key...\n");
      getch();

      dwDataSize += lpDevNode->Size;
      lpDevNode = (PCM_PNP_BIOS_DEVICE_NODE)((DWORD)lpDevNode + lpDevNode->Size);
    }

  free (lpBuffer);

  return 0;
}

/* EOF */
