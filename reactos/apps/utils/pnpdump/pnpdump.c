/*
 * pnpdump - PnP BIOS information dumper
 */

#include <windows.h>
//#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>

//#define DUMP_DATA
#define DUMP_SIZE_INFO


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


static char Hex[] = "0123456789ABCDEF";


#ifdef DUMP_DATA
void HexDump(char *buffer, ULONG size)
{
  ULONG offset = 0;
  unsigned char *ptr;

  while (offset < (size & ~15))
    {
      ptr = (unsigned char*)((ULONG)buffer + offset);
      printf("%08lx  %02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx-%02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx\n",
	     offset,
	     ptr[0],
	     ptr[1],
	     ptr[2],
	     ptr[3],
	     ptr[4],
	     ptr[5],
	     ptr[6],
	     ptr[7],
	     ptr[8],
	     ptr[9],
	     ptr[10],
	     ptr[11],
	     ptr[12],
	     ptr[13],
	     ptr[14],
	     ptr[15]);
      offset += 16;
    }

  ptr = (unsigned char*)((ULONG)buffer + offset);
  printf("%08lx ", offset);
  while (offset < size)
    {
      printf(" %02hx", *ptr);
      offset++;
      ptr++;
    }

  printf("\n\n\n");
}
#endif


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
PnpDecodeIrq(char *Ptr)
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
PnpDecodeDma(char *Ptr)
{
  char DmaChannel;
  char DmaStatus;
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
PnpDecodeIoPort(char *Ptr)
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
PnpDecodeFixedIoPort(char *Ptr)
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
PnpDecodeMemory16(char *Ptr)
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
PnpDecodeMemory32(char *Ptr)
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
PnpDecodeFixedMemory(char *Ptr)
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
  char PnpId[8];
  char *Ptr;
  unsigned int TagSize;
  unsigned int TagType;

  char Id[4];

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

  printf("  '%s'\n",
	 PnpId);

  if (DeviceNode->Size > sizeof(CM_PNP_BIOS_DEVICE_NODE))
    {
      Ptr = (char *)(DeviceNode + 1);
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
