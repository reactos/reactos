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

void do_enumeratekey(PWSTR Name)
{
 ULONG Index,Length,i;
 KEY_BASIC_INFORMATION KeyInformation[5];
 NTSTATUS Status;
 OBJECT_ATTRIBUTES ObjectAttributes;
 HANDLE hKey1;
 UNICODE_STRING KeyName;

  RtlInitUnicodeString(&KeyName, Name);
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, NULL, NULL);
  Status=NtOpenKey( &hKey1, MAXIMUM_ALLOWED, &ObjectAttributes);
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
  NtClose(hKey1);
}

void test1(void)
{
 HKEY hKey = NULL,hKey1;
 OBJECT_ATTRIBUTES ObjectAttributes; 
 NTSTATUS Status; 
 UNICODE_STRING KeyName;
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
    dprintf("NtClose : ");
    Status = NtClose( hKey1 );
    dprintf("\t\t\t\t\tStatus =%x\n",Status);
  }
  NtClose(hKey);

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
    dprintf("NtClose : ");
    Status = NtClose( hKey );
    dprintf("\t\t\t\t\tStatus =%x\n",Status);
  }
  NtClose( hKey1 );
}

void test2(void)
{
 HKEY hKey,hKey1;
 OBJECT_ATTRIBUTES ObjectAttributes; 
 UNICODE_STRING KeyName,ValueName;
 NTSTATUS Status; 
 KEY_VALUE_FULL_INFORMATION KeyValueInformation[5];
 ULONG Index,Length,i;
 char Buffer[10];
 DWORD Result;
  dprintf("NtCreateKey volatile: \n");
  dprintf("  \\Registry\\Machine\\Software\\test2reactos: ");
  RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\test2reactos");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, NULL, NULL);
  Status = NtCreateKey ( &hKey, KEY_ALL_ACCESS , &ObjectAttributes
		,0,NULL,REG_OPTION_VOLATILE,NULL);
  dprintf("\t\tStatus=%x\n",Status);
  NtClose(hKey);
  do_enumeratekey(L"\\Registry\\Machine\\Software");
  dprintf("  ...\\test2 :");
  RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\test2reactos\\test2");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, NULL, NULL);
  Status = NtCreateKey ( &hKey1, KEY_ALL_ACCESS , &ObjectAttributes
		,0,NULL,REG_OPTION_VOLATILE,NULL);
  dprintf("\t\t\t\t\tStatus=%x\n",Status);
  dprintf("  ...\\TestVolatile :");
  RtlInitUnicodeString(&KeyName, L"TestVolatile");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, hKey1, NULL);
  Status = NtCreateKey ( &hKey, KEY_ALL_ACCESS , &ObjectAttributes
		,0,NULL,REG_OPTION_VOLATILE,NULL);
  dprintf("\t\t\t\tStatus=%x\n",Status);
  NtClose(hKey1);
  RtlInitUnicodeString(&ValueName, L"TestREG_SZ");
  dprintf("NtSetValueKey reg_sz: ");
  Status=NtSetValueKey(hKey,&ValueName,0,REG_SZ,(PVOID)L"Test Reg_sz",24);
  dprintf("\t\t\t\tStatus=%x\n",Status);
  RtlInitUnicodeString(&ValueName, L"TestDWORD");
  dprintf("NtSetValueKey reg_dword: ");
  Status=NtSetValueKey(hKey,&ValueName,0,REG_DWORD,(PVOID)"reac",4);
  dprintf("\t\t\tStatus=%x\n",Status);
  NtClose(hKey);
  dprintf("NtOpenKey \\Registry\\Machine\\Software\\test2reactos\\test2\\TestVolatile : ");
  RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\test2reactos\\test2\\TestVolatile");
  InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
  Status=NtOpenKey( &hKey, MAXIMUM_ALLOWED, &ObjectAttributes);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  if(Status==0)
  {
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
  }
  NtClose(hKey);
  dprintf("delete \\Registry\\Machine\\software\\test2reactos ?");
  ReadConsoleA(InputHandle, Buffer, 3, &Result, NULL) ;
  if (Buffer[0] != 'y' && Buffer[0] != 'Y') return;
  RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\test2reactos\\test2\\TestVolatile");
  InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
  dprintf("NtOpenKey : ");
  Status=NtOpenKey( &hKey, KEY_ALL_ACCESS, &ObjectAttributes);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  dprintf("NtDeleteKey : ");
  Status=NtDeleteKey(hKey);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  NtClose(hKey);
  RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\test2reactos\\test2");
  InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
  dprintf("NtOpenKey : ");
  Status=NtOpenKey( &hKey, KEY_ALL_ACCESS, &ObjectAttributes);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  dprintf("NtDeleteKey : ");
  Status=NtDeleteKey(hKey);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  NtClose(hKey);
  RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\test2reactos");
  InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
  dprintf("NtOpenKey : ");
  Status=NtOpenKey( &hKey, KEY_ALL_ACCESS, &ObjectAttributes);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  dprintf("NtDeleteKey : ");
  Status=NtDeleteKey(hKey);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  NtClose(hKey);
}

void test3(void)
{
 HKEY hKey,hKey1;
 OBJECT_ATTRIBUTES ObjectAttributes; 
 UNICODE_STRING KeyName,ValueName;
 NTSTATUS Status; 
 KEY_VALUE_FULL_INFORMATION KeyValueInformation[5];
 ULONG Index,Length,i;
 char Buffer[10];
 DWORD Result;
  dprintf("NtCreateKey non volatile: \n");
  dprintf("  \\Registry\\Machine\\Software\\test3reactos: ");
  RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\test3reactos");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, NULL, NULL);
  Status = NtCreateKey ( &hKey, KEY_ALL_ACCESS , &ObjectAttributes
		,0,NULL,REG_OPTION_NON_VOLATILE,NULL);
  dprintf("\t\tStatus=%x\n",Status);
  NtClose(hKey);
  do_enumeratekey(L"\\Registry\\Machine\\Software");
  dprintf("NtOpenKey: ");
  Status=NtOpenKey( &hKey, MAXIMUM_ALLOWED, &ObjectAttributes);
  dprintf("\t\tStatus=%x\n",Status);
  NtClose(hKey);
  dprintf("  ...\\test3 :");
  RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\test3reactos\\test3");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, NULL, NULL);
  Status = NtCreateKey ( &hKey, KEY_ALL_ACCESS , &ObjectAttributes
		,0,NULL,REG_OPTION_NON_VOLATILE,NULL);
  dprintf("\t\t\t\t\tStatus=%x\n",Status);
  dprintf("NtOpenKey: ");
  Status=NtOpenKey( &hKey1, MAXIMUM_ALLOWED, &ObjectAttributes);
  dprintf("\t\tStatus=%x\n",Status);
  NtClose(hKey);
  dprintf("  ...\\testNonVolatile :");
  RtlInitUnicodeString(&KeyName, L"TestNonVolatile");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, hKey1, NULL);
  Status = NtCreateKey ( &hKey, KEY_ALL_ACCESS , &ObjectAttributes
		,0,NULL,REG_OPTION_NON_VOLATILE,NULL);
  dprintf("\t\t\t\tStatus=%x\n",Status);
  NtClose(hKey1);
  RtlInitUnicodeString(&ValueName, L"TestREG_SZ");
  dprintf("NtSetValueKey reg_sz: ");
  Status=NtSetValueKey(hKey,&ValueName,0,REG_SZ,(PVOID)L"Test Reg_sz",24);
  dprintf("\t\t\t\tStatus=%x\n",Status);
  RtlInitUnicodeString(&ValueName, L"TestDWORD");
  dprintf("NtSetValueKey reg_dword: ");
  Status=NtSetValueKey(hKey,&ValueName,0,REG_DWORD,(PVOID)"reac",4);
  dprintf("\t\t\tStatus=%x\n",Status);
  NtClose(hKey);
  dprintf("NtOpenKey \\Registry\\Machine\\Software\\test3reactos\\test3\\testNonVolatile : ");
  RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\test3reactos\\test3\\testNonVolatile");
  InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
  Status=NtOpenKey( &hKey, MAXIMUM_ALLOWED, &ObjectAttributes);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  if(Status==0)
  {
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
  }
  NtClose(hKey);
  dprintf("delete \\Registry\\Machine\\software\\test3reactos ?");
  ReadConsoleA(InputHandle, Buffer, 3, &Result, NULL) ;
  if (Buffer[0] != 'y' && Buffer[0] != 'Y') return;
  RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\test3reactos\\test3\\testNonvolatile");
  InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
  dprintf("NtOpenKey : ");
  Status=NtOpenKey( &hKey, KEY_ALL_ACCESS, &ObjectAttributes);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  dprintf("NtDeleteKey : ");
  Status=NtDeleteKey(hKey);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\test3reactos\\test3");
  InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
  dprintf("NtOpenKey : ");
  Status=NtOpenKey( &hKey, KEY_ALL_ACCESS, &ObjectAttributes);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  dprintf("NtDeleteKey : ");
  Status=NtDeleteKey(hKey);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  NtClose(hKey);
  RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\test3reactos");
  InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
  dprintf("NtOpenKey : ");
  Status=NtOpenKey( &hKey, KEY_ALL_ACCESS, &ObjectAttributes);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  dprintf("NtDeleteKey : ");
  Status=NtDeleteKey(hKey);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  NtClose(hKey);
}

void test4(void)
{
 HKEY hKey = NULL,hKey1;
 DWORD dwDisposition;
 DWORD dwError;
 DWORD  RegDataType, RegDataSize;
 BOOL GlobalFifoEnable;
 HKEY hPortKey;
 DWORD RegDisposition;
 WCHAR szClass[260];
 DWORD cchClass;	
 DWORD cSubKeys;	
 DWORD cchMaxSubkey;	
 DWORD cchMaxClass;	
 DWORD cValues;	
 DWORD cchMaxValueName;	
 DWORD cbMaxValueData;	
 DWORD cbSecurityDescriptor;	
 FILETIME ftLastWriteTime;	
 SYSTEMTIME LastWriteTime;

   dprintf ("RegOpenKeyExW HKLM\\System\\Setup: ");
   dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"System\\Setup",
                           0,
                           KEY_ALL_ACCESS,
                           &hKey1); 
    dprintf("\t\tdwError =%x\n",dwError);
   if (dwError == ERROR_SUCCESS)
   {
     dprintf("RegQueryInfoKeyW: ");
     cchClass=260;
     dwError = RegQueryInfoKeyW(hKey1
	, szClass, &cchClass, NULL, &cSubKeys
	, &cchMaxSubkey, &cchMaxClass, &cValues, &cchMaxValueName
	, &cbMaxValueData, &cbSecurityDescriptor, &ftLastWriteTime);
     dprintf ("\t\t\t\tdwError %x\n", dwError);
     FileTimeToSystemTime(&ftLastWriteTime,&LastWriteTime);
     dprintf ("\tnb of subkeys=%d,last write : %d/%d/%d %d:%02.2d'%02.2d''%03.3d\n",cSubKeys
		,LastWriteTime.wMonth
		,LastWriteTime.wDay
		,LastWriteTime.wYear
		,LastWriteTime.wHour
		,LastWriteTime.wMinute
		,LastWriteTime.wSecond
		,LastWriteTime.wMilliseconds
		);
   }


   dprintf ("RegOpenKeyExW: ");
   dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"System\\ControlSet001\\Services\\Serial",
                           0,
                           KEY_ALL_ACCESS,
                           &hKey); 
   dprintf ("\t\t\t\t\tdwError %x\n", dwError);
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
    dprintf("\t\t\t\tdwError =%x\n",dwError);
    if (dwError == 0)
    {
        dprintf("\tValue:DT=%d, DS=%d, Value=%d\n"
		,RegDataType
		,RegDataSize
		,GlobalFifoEnable);
    }
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
   dprintf ("\t\t\t\tdwError %x\n", dwError);

   dprintf ("RegCreateKeyExW: ");
   dwError = RegCreateKeyExW (HKEY_LOCAL_MACHINE,
                              L"Software\\test4reactos\\test",
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKey,
                              &dwDisposition);

   dprintf ("\t\t\t\tdwError %x ", dwError);
   dprintf ("dwDisposition %x\n", dwDisposition);
   if (dwError == ERROR_SUCCESS)
   {
     dprintf ("RegSetValueExW: ");
     dwError = RegSetValueExW (hKey,
                             L"TestValue",
                             0,
                             REG_SZ,
                             (BYTE*)L"TestString",
                             20);

     dprintf ("\t\t\t\tdwError %x\n", dwError);
     dprintf ("RegCloseKey: ");
     dwError = RegCloseKey (hKey);
     dprintf ("\t\t\t\t\tdwError %x\n", dwError);
   }
   dprintf ("\n\n");

   hKey = NULL;

   dprintf ("RegCreateKeyExW: ");
   dwError = RegCreateKeyExW (HKEY_LOCAL_MACHINE,
                              L"software\\Test",
                              0,
                              NULL,
                              REG_OPTION_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKey,
                              &dwDisposition);

   dprintf ("\t\t\t\tdwError %x ", dwError);
   dprintf ("dwDisposition %x\n", dwDisposition);


   if (dwError == ERROR_SUCCESS)
   {
     dprintf("RegQueryInfoKeyW: ");
     cchClass=260;
     dwError = RegQueryInfoKeyW(hKey
	, szClass, &cchClass, NULL, &cSubKeys
	, &cchMaxSubkey, &cchMaxClass, &cValues, &cchMaxValueName
	, &cbMaxValueData, &cbSecurityDescriptor, &ftLastWriteTime);
     dprintf ("\t\t\t\tdwError %x\n", dwError);
     FileTimeToSystemTime(&ftLastWriteTime,&LastWriteTime);
     dprintf ("\tnb of subkeys=%d,last write : %d/%d/%d %d:%02.2d'%02.2d''%03.3d\n",cSubKeys
		,LastWriteTime.wMonth
		,LastWriteTime.wDay
		,LastWriteTime.wYear
		,LastWriteTime.wHour
		,LastWriteTime.wMinute
		,LastWriteTime.wSecond
		,LastWriteTime.wMilliseconds
		);
     dprintf ("RegCloseKey: ");
     dwError = RegCloseKey (hKey);
     dprintf ("\t\t\t\t\tdwError %x\n", dwError);
   }
   dprintf ("\nTests done...\n");
}

void test5(void)
{
 OBJECT_ATTRIBUTES ObjectAttributes;
 UNICODE_STRING KeyName;
 NTSTATUS Status;
 LONG dwError;
 TOKEN_PRIVILEGES NewPrivileges; 
 HANDLE Token; 
 LUID Luid; 
 BOOLEAN bRes;
  Status=NtOpenProcessToken(GetCurrentProcess()
	,TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&Token);
  dprintf("\t\t\t\tStatus =%x\n",Status);
/*
  bRes=LookupPrivilegeValueA(NULL,SE_RESTORE_NAME,&Luid);
  dprintf("\t\t\t\tbRes =%x\n",bRes);
*/
  NewPrivileges.PrivilegeCount = 1; 
  NewPrivileges.Privileges[0].Luid = Luid; 
  NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
 
//  Status = NtAdjustPrivilegesToken( 
  Status = AdjustTokenPrivileges( 
            Token, 
            FALSE, 
            &NewPrivileges, 
            0, 
            NULL, 
            NULL 
            ); 
  dprintf("\t\t\t\tStatus =%x\n",Status);
 
  Status=NtClose(Token); 
  dprintf("\t\t\t\tStatus =%x\n",Status);


  RtlInitUnicodeString(&KeyName,L"test5");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, NULL, NULL);
  Status = NtLoadKey(HKEY_LOCAL_MACHINE,&ObjectAttributes);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  dwError=RegLoadKey(HKEY_LOCAL_MACHINE,"def"
		,"test5");
  dprintf("\t\t\t\tdwError =%x\n",dwError);

}

int main(int argc, char* argv[])
{
 char Buffer[10];
 DWORD Result;

  AllocConsole();
  InputHandle = GetStdHandle(STD_INPUT_HANDLE);
  OutputHandle =  GetStdHandle(STD_OUTPUT_HANDLE);

  dprintf("choose test :\n");
  dprintf("  1=Ntxxx read functions\n");
  dprintf("  2=Ntxxx write functions : volatile keys\n");
  dprintf("  3=Ntxxx write functions : non volatile keys\n");
  dprintf("  4=Regxxx functions\n");
  ReadConsoleA(InputHandle, Buffer, 3, &Result, NULL) ;
  switch (Buffer[0])
  {
   case '1':
    test1();
    break;
   case '2':
    test2();
    break;
   case '3':
    test3();
    break;
   case '4':
    test4();
    break;
   case '5':
    test5();
    break;
  }
  return 0;
}
