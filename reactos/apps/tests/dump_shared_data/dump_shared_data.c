#include <stdio.h>
#include <ntddk.h>

int main()
{
  int i;

  printf("TickCountLow: %x\n", 
	 SharedUserData->TickCountLow);
  printf("Drives: ");
  for (i = 0; i < 26; i++)
    {
      printf("%c", (SharedUserData->DosDeviceMap & (1 << i))?'1':'0');
    }
  printf("\n");
  for (i = 0; i < 26; i++)
    {
      if (SharedUserData->DosDeviceMap & (1 << i))
	{
	  printf("%c: ", 'A'+i);
	  switch(SharedUserData->DosDeviceDriveType[i])
	    {
	      case DOSDEVICE_DRIVE_UNKNOWN:
		printf("Unknown\n");
		break;
	      case DOSDEVICE_DRIVE_CALCULATE:
		printf("No root\n");
		break;
	      case DOSDEVICE_DRIVE_REMOVABLE:
		printf("Removable\n");
		break;
	      case DOSDEVICE_DRIVE_FIXED:
		printf("Fixed\n");
		break;
	      case DOSDEVICE_DRIVE_REMOTE:
		printf("Remote\n");
		break;
	      case DOSDEVICE_DRIVE_CDROM:
		printf("CD-ROM\n");
		break;
	      case DOSDEVICE_DRIVE_RAMDISK:
		printf("Ram disk\n");
		break;
	      default:
		printf("undefined type\n");
		break;
	    }
	}
    }
  printf("\n\n");
}
