#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <ddk/ntddk.h>

HANDLE OutputHandle;
HANDLE InputHandle;

void dprintf(char* fmt, ...)
{
   va_list args;
   char buffer[255];

   va_start(args,fmt);
   vsprintf(buffer,fmt,args);
   WriteConsoleA(OutputHandle, buffer, strlen(buffer), NULL, NULL);
   va_end(args);
}


int main(int argc, char* argv[])
{
   HKEY hKey = NULL,hKey1;
   DWORD dwDisposition;
   DWORD dwError;
    DWORD Err, RegDataType, RegDataSize, OldComPortNumber;
    OBJECT_ATTRIBUTES ObjectAttributes; 
    HANDLE StConfigHandle; 
    ULONG Disposition; 
    NTSTATUS Status; 
  UNICODE_STRING KeyName;
    BOOL GlobalFifoEnable;
    HKEY hPortKey;
    DWORD RegDisposition;
 ULONG Index,Length,i;
 KEY_BASIC_INFORMATION KeyInformation[5];
 KEY_VALUE_FULL_INFORMATION KeyValueInformation[5];

  AllocConsole();
  InputHandle = GetStdHandle(STD_INPUT_HANDLE);
  OutputHandle =  GetStdHandle(STD_OUTPUT_HANDLE);

  dprintf("NtOpenKey \\Registry : ");
  RtlInitUnicodeString(&KeyName, L"\\Registry");
  InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
  Status=NtOpenKey( &hKey1, MAXIMUM_ALLOWED, &ObjectAttributes);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  if(Status==0)
  {
    dprintf("NtQueryKey : ");
    Status=NtQueryKey(hKey1,KeyBasicInformation
		,&KeyInformation[0], sizeof(KeyInformation)
		,&Length);
    dprintf("\t\t\t\t\tStatus =%x\n",Status);
    if (Status == STATUS_SUCCESS)
    {
        dprintf("\tKey Name = ");
	  for (i=0;i<KeyInformation[0].NameLength/2;i++)
		dprintf("%C",KeyInformation[0].Name[i]);
        dprintf("\n");
    }
    dprintf("NtEnumerateKey : \n");
    Index=0;
    while(Status == STATUS_SUCCESS)
    {
      Status=NtEnumerateKey(hKey1,Index++,KeyBasicInformation
		,&KeyInformation[0], sizeof(KeyInformation)
		,&Length);
      if(Status== STATUS_SUCCESS)
	{
        dprintf("\tSubKey Name = ");
	  for (i=0;i<KeyInformation[0].NameLength/2;i++)
		dprintf("%C",KeyInformation[0].Name[i]);
        dprintf("\n");
	}
    }
    dprintf("NtCloseKey : ");
    Status = NtClose( hKey1 );
    dprintf("\t\t\t\t\tStatus =%x\n",Status);
  }

  dprintf("NtOpenKey \\Registry\\Machine : ");
  RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine");
  InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
  Status=NtOpenKey( &hKey1, MAXIMUM_ALLOWED, &ObjectAttributes);
  dprintf("\t\t\tStatus =%x\n",Status);

  dprintf("NtOpenKey System\\Setup : ");
  RtlInitUnicodeString(&KeyName, L"System\\Setup");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, hKey1 , NULL);
  Status = NtOpenKey ( &hKey, KEY_READ , &ObjectAttributes);
  dprintf("\t\t\tStatus =%x\n",Status);
  if(Status==0)
  {
    dprintf("NtQueryValueKey : ");
    RtlInitUnicodeString(&KeyName, L"CmdLine");
    Status=NtQueryValueKey(hKey,&KeyName,KeyValueFullInformation
		,&KeyValueInformation[0], sizeof(KeyValueInformation)
		,&Length);
    dprintf("\t\t\t\tStatus =%x\n",Status);
    if (Status == STATUS_SUCCESS)
    {
        dprintf("\tValue:DO=%d, DL=%d, NL=%d, Name = "
		,KeyValueInformation[0].DataOffset
		,KeyValueInformation[0].DataLength
		,KeyValueInformation[0].NameLength);
	  for (i=0;i<10 && i<KeyValueInformation[0].NameLength/2;i++)
		dprintf("%C",KeyValueInformation[0].Name[i]);
        dprintf("\n");
        dprintf("\t\tType = %d\n",KeyValueInformation[0].Type);
	  if (KeyValueInformation[0].Type == REG_SZ)
          dprintf("\t\tValue = %S\n",KeyValueInformation[0].Name+1
    					+KeyValueInformation[0].NameLength/2);
    }
    dprintf("NtEnumerateValueKey : \n");
    Index=0;
    while(Status == STATUS_SUCCESS)
    {
      Status=NtEnumerateValueKey(hKey,Index++,KeyValueFullInformation
		,&KeyValueInformation[0], sizeof(KeyValueInformation)
		,&Length);
      if(Status== STATUS_SUCCESS)
	{
        dprintf("\tValue:DO=%d, DL=%d, NL=%d, Name = "
		,KeyValueInformation[0].DataOffset
		,KeyValueInformation[0].DataLength
		,KeyValueInformation[0].NameLength);
	  for (i=0;i<KeyValueInformation[0].NameLength/2;i++)
		dprintf("%C",KeyValueInformation[0].Name[i]);
        dprintf(", Type = %d\n",KeyValueInformation[0].Type);
	  if (KeyValueInformation[0].Type == REG_SZ)
          dprintf("\t\tValue = %S\n",((char*)&KeyValueInformation[0]
    					+KeyValueInformation[0].DataOffset));
	}
    }
    dprintf("NtCloseKey : ");
    Status = NtClose( hKey1 );
    dprintf("\t\t\t\t\tStatus =%x\n",Status);
  }


return 0;


   dprintf ("RegOpenKeyExW HKLM\\System\\ControlSet001: ");
   dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"System\\ControlSet001",
                           0,
                           KEY_ALL_ACCESS,
                           &hKey); 
   dprintf ("dwError %x\n", dwError);
/*
  Status = NtCreateKey ( &hKey, KEY_ALL_ACCESS , &ObjectAttributes
		,0,NULL,REG_OPTION_VOLATILE,NULL);
  dprintf("Status=%x\n",Status);
*/


   dprintf ("RegOpenKeyExW: ");
   dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"System\\ControlSet001\\Services\\Serial",
                           0,
                           KEY_ALL_ACCESS,
                           &hKey); 
   dprintf ("dwError %x\n", dwError);
   RegDataSize = sizeof(GlobalFifoEnable);
   if (dwError == ERROR_SUCCESS)
   {
     dprintf ("RegQueryValueExW: ");
     dwError = RegQueryValueExW(hKey,
                        L"ForceFifoEnable",
                        NULL,
                        &RegDataType,
                        (PBYTE)&GlobalFifoEnable,
                        &RegDataSize);
     dprintf ("dwError %x\n", dwError);
   }
   dprintf ("RegCreateKeyExW: ");
   dwError = RegCreateKeyExW(hKey,
                         L"Parameters\\Serial001",
                         0,
                         NULL,
                         0,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hPortKey,
                         &RegDisposition
                        );
   dprintf ("dwError %x\n", dwError);

   dprintf ("RegCreateKeyExW: ");
   dwError = RegCreateKeyExW (HKEY_LOCAL_MACHINE,
//                           L"System\\ControlSet001\\Services\\Serial\\Test",
                           L"Software\\reactos\\test",
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKey,
                              &dwDisposition);

   dprintf ("dwError %x ", dwError);
   dprintf ("dwDisposition %x\n", dwDisposition);
   if (dwError == ERROR_SUCCESS)
   {
     dprintf ("RegSetValueExW: ");
     dwError = RegSetValueExW (hKey,
                             L"TestValue",
                             0,
                             REG_SZ,
                             L"TestString",
                             20);

     dprintf ("dwError %x\n", dwError);
     dprintf ("RegCloseKey: ");
     dwError = RegCloseKey (hKey);
     dprintf ("dwError %x\n", dwError);
   }
   dprintf ("\n\n");

   hKey = NULL;

   dprintf ("RegCreateKeyExW:\n");
   dwError = RegCreateKeyExW (HKEY_LOCAL_MACHINE,
                              L"software\\Test",
                              0,
                              NULL,
                              REG_OPTION_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKey,
                              &dwDisposition);

   dprintf ("dwError %x ", dwError);
   dprintf ("dwDisposition %x\n", dwDisposition);

#if 0
   dprintf ("RegQueryKeyExW:\n");

#endif
   if (dwError == ERROR_SUCCESS)
   {
     dprintf ("RegCloseKey:\n");
     dwError = RegCloseKey (hKey);
     dprintf ("dwError %x\n", dwError);
   }

   dprintf ("\nTests done...\n");

   return 0;
}

