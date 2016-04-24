/* 
 * Adapted from code at http://support.fccps.cz/download/adv/frr/win32_ddk_mingw/win32_ddk_mingw.html - thanks!
 * 
   File created by Frank Rysanek <rysanek@fccps.cz>
   Source code taken almost verbatim from "Driver Development, Part 1"
   published by Toby Opferman at CodeProject.com 
*/

#include <stdio.h>
#include <windows.h>
/*#include <string.h>*/
#include <unistd.h>     /* getcwd() */

#define MY_DRIVER_NAME "btrfs"
#define MY_DEVICE_NAME     "\\Btrfs"
#define MY_DOSDEVICE_NAME  "\\DosDevices\\"  MY_DRIVER_NAME  /* AKA symlink name */
/* for the loader and app */
#define MY_SERVICE_NAME_LONG  "Driver Test2"
#define MY_SERVICE_NAME_SHORT MY_DRIVER_NAME
#define MY_DRIVER_FILENAME    MY_DRIVER_NAME ".sys"

#define MAX_CWD_LEN 1024
static char cwd[MAX_CWD_LEN+3];  /* the XXXXX.sys filename will get appended to this as well */

/* geterrstr() taken verbatim from some code snippet at www.mingw.org by Dan Osborne. */
/* Apparently, it's a way to get a classic null-terminated string containing "last error". */
static char errbuffer[256];

static const char *geterrstr(DWORD errcode)
{
 size_t skip = 0;
 DWORD chars;

   chars = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                         NULL, errcode, 0, errbuffer, sizeof(errbuffer)-1, 0);
   errbuffer[sizeof(errbuffer)-1] = 0;

   if (chars) 
   {
      while (errbuffer[chars-1] == '\r' || errbuffer[chars-1] == '\n') 
      {
         errbuffer[--chars] = 0;
      }
   }

   if (chars && errbuffer[chars-1] == '.') errbuffer[--chars] = 0;

   if (chars >= 2 && errbuffer[0] == '%' 
       && errbuffer[1] >= '0' && errbuffer[1] <= '9')
   {
      skip = 2;

      while (chars > skip && errbuffer[skip] == ' ') ++skip;

      if (chars >= skip+2 && errbuffer[skip] == 'i' && errbuffer[skip+1] == 's')
      {
         skip += 2;
         while (chars > skip && errbuffer[skip] == ' ') ++skip;
      }
   }

   if (chars > skip && errbuffer[skip] >= 'A' && errbuffer[skip] <= 'Z') 
   {
      errbuffer[skip] += 'a' - 'A';
   }

   return errbuffer+skip;
}

void process_error(void)
{
   DWORD err = GetLastError();
   printf("Error: %lu = \"%s\"\n", (unsigned long)err, geterrstr(err));
}

int main(void)
{
 HANDLE hSCManager;
 HANDLE hService;
 SERVICE_STATUS ss;
 int retval = 0;
 BOOL amd64;

   /* First of all, maybe concatenate the current working directory
      with the desired driver file name - before we start messing with
      the service manager etc. */
   if (getcwd(cwd, MAX_CWD_LEN) == NULL)  /* error */
   {
      printf("Failed to learn the current working directory!\n");
      retval = -8;
      goto err_out1;
   } /* else got CWD just fine */

   if (strlen(cwd) + strlen(MY_DRIVER_FILENAME) + 1 > MAX_CWD_LEN)
   {
      printf("Current working dir + driver filename > longer than %d ?!?\n", MAX_CWD_LEN);
      retval = -9;
      goto err_out1;
   } /* else our buffer is long enough :-) */

   strcat(cwd, "\\");
      
   IsWow64Process(GetCurrentProcess(),&amd64);
   strcat(cwd, amd64 ? "x64" : "x86");
   
   strcat(cwd, "\\");
   strcat(cwd, MY_DRIVER_FILENAME); 
   printf("Driver path+name: %s\n", cwd);


   printf("Going to open the service manager... ");

   hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
   if (! hSCManager)
   {
      printf("Uh oh:\n");
      process_error();
      retval = -1;
      goto err_out1;
   }

   printf("okay.\n");
   printf("Going to create the service... ");

   hService = CreateService(hSCManager, MY_SERVICE_NAME_SHORT, 
                            MY_SERVICE_NAME_LONG, 
                            SERVICE_START | DELETE | SERVICE_STOP, 
                            SERVICE_KERNEL_DRIVER,
                            SERVICE_DEMAND_START, 
                            SERVICE_ERROR_IGNORE, 
                            cwd, 
                            NULL, NULL, NULL, NULL, NULL);

   if(!hService)
   {
      process_error();
      printf("\n already exists? Trying to open it... ");
      hService = OpenService(hSCManager, MY_SERVICE_NAME_SHORT, 
                             SERVICE_START | DELETE | SERVICE_STOP);
   }

   if(!hService)
   {
      printf("FAILED!\n");
      process_error();
      retval = -2;
      goto err_out2;
   }

   printf("okay.\n");
   printf("Going to start the service... ");

   if (StartService(hService, 0, NULL) == 0)  /* error */
   {
      printf("Uh oh:\n");
      process_error();
      retval = -3;
      goto err_out3;
   }

   printf("okay.\n");
   
//    TCHAR VolumeName[] = _T("Z:");
// TCHAR DeviceName[] = _T("\\Device\\VDisk1");
 
//    printf("Mounting volume... ");
//     if (!DefineDosDeviceA(DDD_RAW_TARGET_PATH, "T:", "\\Device\\HarddiskVolume3"))
//     {
//         printf("Uh oh:\n");
//         process_error();
//     } else {
//         printf("okay.\n");
//     }
   
//     if (!SetVolumeMountPointA("T:\\", "\\\\?\\Volume{9bd714c3-4379-11e5-b26c-806e6f6e6963}\\")) {
//         printf("Uh oh:\n");
//         process_error();
//     } else {
//         printf("okay.\n");
//     }
   
   printf("\n >>> Press Enter to unload the driver! <<<\n");
   getchar();
   
//    printf("Unmounting volume... ");
//     if (!DefineDosDeviceA(DDD_REMOVE_DEFINITION, "T:", NULL))
//     {
//         printf("Uh oh:\n");
//         process_error();
//     } else {
//         printf("okay.\n");
//     }

   printf("Going to stop the service... ");
   if (ControlService(hService, SERVICE_CONTROL_STOP, &ss) == 0) /* error */
   {
      printf("Uh oh:\n");
      process_error();
      retval = -4;
   }
   else printf("okay.\n");

err_out3:

   printf("Going to close the service handle... ");
   if (CloseServiceHandle(hService) == 0) /* error */
   {
      printf("Uh oh:\n");
      process_error();
      retval = -6;
   }
   else printf("okay.\n");

err_out2:

   printf("Going to close the service manager... ");
   if (CloseServiceHandle(hSCManager) == 0) /* error */
   {
      printf("Uh oh:\n");
      process_error();
      retval = -7;
   }
   else printf("okay.\n");

err_out1:

   printf("Finished! :-b\n");

   return(retval);
}

