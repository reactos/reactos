#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <ndk/rtltypes.h>

#define RtlRosInitUnicodeStringFromLiteral(__Name__, __Value__) \
    { \
	(__Name__)->Buffer = (__Value__); \
	(__Name__)->Length = sizeof(__Value__) - sizeof(WCHAR); \
	(__Name__)->MaximumLength = sizeof(__Value__); \
    }

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


void CreateKeyTest(void)
{
  HANDLE hKey;
  OBJECT_ATTRIBUTES ObjectAttributes;
  NTSTATUS Status;
  UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\testkey");

  dprintf("Create key '\\Registry\\Machine\\Software\\testkey':\n");

  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  dprintf("NtCreateKey:\n");
  Status = NtCreateKey(&hKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       NULL);
  dprintf("  Status = %lx\n",Status);
  if (NT_SUCCESS(Status))
    {
      NtClose(hKey);
    }
}


void DeleteKeyTest(void)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\testkey");
  HANDLE hKey;
  NTSTATUS Status;

  dprintf("Delete key '\\Registry\\Machine\\Software\\testkey':\n");

  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  dprintf("NtOpenKey:\n");
  Status = NtOpenKey(&hKey,
		     KEY_ALL_ACCESS,
		     &ObjectAttributes);
  dprintf("  Status = %lx\n",Status);
  if (!NT_SUCCESS(Status))
    return;

  dprintf("NtDeleteKey:\n");
  Status = NtDeleteKey(hKey);
  dprintf("  Status = %lx\n",Status);
  NtClose(hKey);
}


void EnumerateKeyTest(void)
{
  HANDLE hKey = NULL;
  OBJECT_ATTRIBUTES ObjectAttributes;
  NTSTATUS Status;
  UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software");
  ULONG Index;
  ULONG Length;
  ULONG i;
  KEY_BASIC_INFORMATION KeyInformation[5];

  dprintf("Enumerate key '\\Registry\\Machine\\Software':\n");

  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  dprintf("NtOpenKey:\n");
  Status = NtOpenKey(&hKey,
		     KEY_ALL_ACCESS,
		     &ObjectAttributes);
  dprintf("  Status = %lx\n", Status);
  if (!NT_SUCCESS(Status))
    return;

  dprintf("NtQueryKey:\n");
  Status = NtQueryKey(hKey,
		      KeyBasicInformation,
		      &KeyInformation[0],
		      sizeof(KeyInformation),
		      &Length);
  dprintf("  Status = %lx\n", Status);
  if (NT_SUCCESS(Status))
    {
      dprintf("\tKey Name = ");
      for (i = 0; i < KeyInformation[0].NameLength / 2; i++)
	dprintf("%C", KeyInformation[0].Name[i]);
      dprintf("\n");
    }

  dprintf("NtEnumerateKey:\n");
  Index=0;
  while(NT_SUCCESS(Status))
    {
      Status = NtEnumerateKey(hKey,
			      Index,
			      KeyBasicInformation,
			      &KeyInformation[0],
			      sizeof(KeyInformation),
			      &Length);
      if (NT_SUCCESS(Status))
	{
	  dprintf("\tSubKey Name = ");
	  for (i = 0; i < KeyInformation[0].NameLength / 2; i++)
	    dprintf("%C", KeyInformation[0].Name[i]);
	  dprintf("\n");
	}
      Index++;
    }

  dprintf("NtClose:\n");
  Status = NtClose(hKey);
  dprintf("  Status = %lx\n", Status);
}


void SetValueTest1(void)
{
  HANDLE hKey;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\testkey");
  UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"TestValue");
  NTSTATUS Status;

  dprintf("Create key '\\Registry\\Machine\\Software\\testkey':\n");

  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     NULL,
			     NULL);
  dprintf("NtCreateKey:\n");
  Status = NtCreateKey(&hKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       NULL);
  dprintf("  Status = %lx\n",Status);
  if (!NT_SUCCESS(Status))
    return;


  dprintf("NtSetValueKey:\n");
  Status = NtSetValueKey(hKey,
			 &ValueName,
			 0,
			 REG_SZ,
			 (PVOID)L"TestString",
			 24);
  dprintf("  Status = %lx\n",Status);

  NtClose(hKey);
}


void SetValueTest2(void)
{
  HANDLE hKey;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\testkey");
  UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"TestValue");
  NTSTATUS Status;

  dprintf("Create key '\\Registry\\Machine\\Software\\testkey':\n");

  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     NULL,
			     NULL);
  dprintf("NtCreateKey:\n");
  Status = NtCreateKey(&hKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       NULL);
  dprintf("  Status = %lx\n",Status);
  if (!NT_SUCCESS(Status))
    return;

  dprintf("NtSetValueKey:\n");
  Status = NtSetValueKey(hKey,
			 &ValueName,
			 0,
			 REG_DWORD,
			 (PVOID)"reac",
			 4);
  dprintf("  Status = %lx\n",Status);

  NtClose(hKey);
}


void DeleteValueTest(void)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\testkey");
  UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"TestValue");
  HANDLE KeyHandle;
  NTSTATUS Status;

  dprintf("Open key '\\Registry\\Machine\\Software\\testkey':\n");

  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status=NtOpenKey(&KeyHandle,
		   MAXIMUM_ALLOWED,
		   &ObjectAttributes);
  dprintf("  Status = %lx\n", Status);
  if (!NT_SUCCESS(Status))
    return;

  dprintf("Delete value:\n");


  Status = NtDeleteValueKey(KeyHandle,
			    &ValueName);
  dprintf("  Status = %lx\n", Status);

  dprintf("Close key:\n");
  Status = NtClose(KeyHandle);
  dprintf("  Status = %lx\n", Status);
}


void EnumerateValueTest(void)
{
  KEY_VALUE_FULL_INFORMATION KeyValueInformation[5];
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\testkey");
  ULONG Index,Length,i;
  HANDLE hKey = NULL;
  NTSTATUS Status;

  dprintf("Open key '\\Registry\\Machine\\Software\\testkey':\n");

  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status=NtOpenKey(&hKey,
		   MAXIMUM_ALLOWED,
		   &ObjectAttributes);
  dprintf("  Status = %lx\n", Status);
  if (!NT_SUCCESS(Status))
    return;

  dprintf("Enumerate values:\n");
  Index = 0;
  while (Status == STATUS_SUCCESS)
    {
      Status = NtEnumerateValueKey(hKey,
				   Index++,
				   KeyValueFullInformation,
				   &KeyValueInformation[0],
				   sizeof(KeyValueInformation),
				   &Length);
      if (Status == STATUS_SUCCESS)
	{
	  dprintf("    Value:DO=%d, DL=%d, NL=%d, Name = ",
		  KeyValueInformation[0].DataOffset,
		  KeyValueInformation[0].DataLength,
		  KeyValueInformation[0].NameLength);
	  for (i = 0; i < KeyValueInformation[0].NameLength / 2; i++)
	    dprintf("%C", KeyValueInformation[0].Name[i]);
	  dprintf(", Type = %d\n", KeyValueInformation[0].Type);

	  if (KeyValueInformation[0].Type == REG_SZ)
	    dprintf("    Value = %S\n",
		    ((char*)&KeyValueInformation[0]+KeyValueInformation[0].DataOffset));

	  if (KeyValueInformation[0].Type == REG_DWORD)
	    dprintf("    Value = %X\n",
		    *((DWORD*)((char*)&KeyValueInformation[0]+KeyValueInformation[0].DataOffset)));
	}
    }

  dprintf("NtClose:\n");
  Status = NtClose(hKey);
  dprintf("  Status = %lx\n", Status);
}




void test1(void)
{
 HANDLE hKey = NULL, hKey1;
 OBJECT_ATTRIBUTES ObjectAttributes;
 NTSTATUS Status;
 UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software");
 ULONG Index,Length,i;
 KEY_BASIC_INFORMATION KeyInformation[5];

#if 0
  dprintf("NtOpenKey \\Registry : ");
#endif
  dprintf("NtOpenKey \\Registry\\Machine\\Software : ");
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
}


void test3(void)
{
 HANDLE hKey;
 OBJECT_ATTRIBUTES ObjectAttributes;
 UNICODE_STRING KeyName;
 NTSTATUS Status;
 char Buffer[10];
 DWORD Result;
  dprintf("NtCreateKey non volatile: \n");
  dprintf("  \\Registry\\Machine\\Software\\test3reactos: ");
  RtlRosInitUnicodeStringFromLiteral(&KeyName, L"\\Registry\\Machine\\Software\\test3reactos");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, NULL, NULL);
  Status = NtCreateKey ( &hKey, KEY_ALL_ACCESS , &ObjectAttributes
		,0,NULL,REG_OPTION_NON_VOLATILE,NULL);
  dprintf("\t\tStatus=%x\n",Status);
  NtClose(hKey);

  dprintf("delete \\Registry\\Machine\\software\\test3reactos ?");
  ReadConsoleA(InputHandle, Buffer, 3, &Result, NULL) ;
  if (Buffer[0] != 'y' && Buffer[0] != 'Y') return;

  dprintf("delete \\Registry\\Machine\\software\\test3reactos ?");
  RtlRosInitUnicodeStringFromLiteral(&KeyName, L"\\Registry\\Machine\\Software\\test3reactos");
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
  HANDLE hKey;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  NTSTATUS Status;

  dprintf("NtOpenKey : \n");
  dprintf("  \\Registry\\Machine\\Software\\reactos : ");
  RtlRosInitUnicodeStringFromLiteral(&KeyName,L"\\Registry\\Machine\\Software\\reactos");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, NULL, NULL);
  Status=NtOpenKey( &hKey, KEY_ALL_ACCESS, &ObjectAttributes);
  dprintf("\t\tStatus=%x\n",Status);
  dprintf("NtFlushKey : \n");
  Status = NtFlushKey(hKey);
  dprintf("\t\tStatus=%x\n",Status);
  dprintf("NtCloseKey : \n");
  Status=NtClose(hKey);
  dprintf("\t\tStatus=%x\n",Status);
}

/* registry link create test */
void test6(void)
{
  HANDLE hKey;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName,ValueName;
  NTSTATUS Status;
  KEY_VALUE_FULL_INFORMATION KeyValueInformation[5];
  ULONG Length,i;

  dprintf("Create target key\n");
  dprintf("  Key: \\Registry\\Machine\\SOFTWARE\\Reactos\n");
  RtlRosInitUnicodeStringFromLiteral(&KeyName, L"\\Registry\\Machine\\SOFTWARE\\Reactos");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, NULL, NULL);
  Status = NtCreateKey(&hKey, KEY_ALL_ACCESS , &ObjectAttributes
		,0,NULL, REG_OPTION_VOLATILE,NULL);
  dprintf("  NtCreateKey() called (Status %lx)\n",Status);
  if (!NT_SUCCESS(Status))
    return;

  dprintf("Create target value\n");
  dprintf("  Value: TestValue = 'Test String'\n");
  RtlRosInitUnicodeStringFromLiteral(&ValueName, L"TestValue");
  Status=NtSetValueKey(hKey,&ValueName,0,REG_SZ,(PVOID)L"TestString",22);
  dprintf("  NtSetValueKey() called (Status %lx)\n",Status);
  if (!NT_SUCCESS(Status))
    return;

  dprintf("Close target key\n");
  NtClose(hKey);


  dprintf("Create link key\n");
  dprintf("  Key: \\Registry\\Machine\\SOFTWARE\\Test\n");
  RtlRosInitUnicodeStringFromLiteral(&KeyName, L"\\Registry\\Machine\\SOFTWARE\\Test");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENLINK,
			     NULL,
			     NULL);
  Status = NtCreateKey(&hKey,
		       KEY_ALL_ACCESS | KEY_CREATE_LINK,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
		       NULL);
  dprintf("  NtCreateKey() called (Status %lx)\n",Status);
  if (!NT_SUCCESS(Status))
    return;

  dprintf("Create link value\n");
  dprintf("  Value: SymbolicLinkValue = '\\Registry\\Machine\\SOFTWARE\\Reactos'\n");
  RtlRosInitUnicodeStringFromLiteral(&ValueName, L"SymbolicLinkValue");
  Status=NtSetValueKey(hKey,&ValueName,0,REG_LINK,(PVOID)L"\\Registry\\Machine\\SOFTWARE\\Reactos",68);
  dprintf("  NtSetValueKey() called (Status %lx)\n",Status);
  if (!NT_SUCCESS(Status))
    {
      dprintf("Creating link value failed! Test failed!\n");
      NtClose(hKey);
      return;
    }

  dprintf("Close link key\n");
  NtClose(hKey);

  dprintf("Open link key\n");
  dprintf("  Key: \\Registry\\Machine\\SOFTWARE\\Test\n");
  RtlRosInitUnicodeStringFromLiteral(&KeyName, L"\\Registry\\Machine\\SOFTWARE\\Test");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE | OBJ_OPENIF
				, NULL, NULL);
  Status = NtCreateKey(&hKey, KEY_ALL_ACCESS , &ObjectAttributes
		,0,NULL, REG_OPTION_VOLATILE, NULL);
  dprintf("  NtCreateKey() called (Status %lx)\n",Status);
  if (!NT_SUCCESS(Status))
    return;

  dprintf("Query value\n");
  dprintf("  Value: TestValue\n");
  RtlRosInitUnicodeStringFromLiteral(&ValueName, L"TestValue");
  Status=NtQueryValueKey(hKey,
			 &ValueName,
			 KeyValueFullInformation,
			 &KeyValueInformation[0],
			 sizeof(KeyValueInformation),
			 &Length);
  dprintf("  NtQueryValueKey() called (Status %lx)\n",Status);
  if (Status == STATUS_SUCCESS)
    {
      dprintf("  Value: Type %d  DataLength %d NameLength %d  Name '",
	      KeyValueInformation[0].Type,
	      KeyValueInformation[0].DataLength,
	      KeyValueInformation[0].NameLength);
      for (i=0; i < KeyValueInformation[0].NameLength / sizeof(WCHAR); i++)
	dprintf("%C",KeyValueInformation[0].Name[i]);
      dprintf("'\n");
      if (KeyValueInformation[0].Type == REG_SZ)
	dprintf("  Value '%S'\n",
		KeyValueInformation[0].Name+1
		+KeyValueInformation[0].NameLength/2);
    }

  dprintf("Close link key\n");
  NtClose(hKey);

  dprintf("Test successful!\n");
}

/* registry link delete test */
void test7(void)
{
  HANDLE hKey;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName,ValueName;
  NTSTATUS Status;

  dprintf("Open link key\n");
  dprintf("  Key: \\Registry\\Machine\\SOFTWARE\\Test\n");
  RtlRosInitUnicodeStringFromLiteral(&KeyName, L"\\Registry\\Machine\\SOFTWARE\\Test");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_OPENLINK,
			     NULL,
			     NULL);
  Status = NtCreateKey(&hKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE | REG_OPTION_OPEN_LINK,
		       NULL);
  dprintf("  NtCreateKey() called (Status %lx)\n",Status);
  if (!NT_SUCCESS(Status))
    {
      dprintf("Could not open the link key. Please run the link create test first!\n");
      return;
    }

  dprintf("Delete link value\n");
  RtlRosInitUnicodeStringFromLiteral(&ValueName, L"SymbolicLinkValue");
  Status = NtDeleteValueKey(hKey,
			    &ValueName);
  dprintf("  NtDeleteValueKey() called (Status %lx)\n",Status);

  dprintf("Delete link key\n");
  Status=NtDeleteKey(hKey);
  dprintf("  NtDeleteKey() called (Status %lx)\n",Status);

  dprintf("Close link key\n");
  NtClose(hKey);
}


void test8(void)
{
 OBJECT_ATTRIBUTES ObjectAttributes;
 UNICODE_STRING KeyName;
 NTSTATUS Status;
 LONG dwError;
 TOKEN_PRIVILEGES NewPrivileges;
 HANDLE Token,hKey;
 LUID Luid = {0};
 BOOLEAN bRes;
  Status=NtOpenProcessToken(GetCurrentProcess()
	,TOKEN_ADJUST_PRIVILEGES,&Token);
//	,TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&Token);
  dprintf("\t\t\t\tStatus =%x\n",Status);
//  bRes=LookupPrivilegeValueA(NULL,SE_RESTORE_NAME,&Luid);
//  dprintf("\t\t\t\tbRes =%x\n",bRes);
  NewPrivileges.PrivilegeCount = 1;
  NewPrivileges.Privileges[0].Luid = Luid;
//  NewPrivileges.Privileges[0].Luid.u.LowPart=18;
//  NewPrivileges.Privileges[0].Luid.u.HighPart=0;
  NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

//  Status = NtAdjustPrivilegesToken(
  bRes = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );
  dprintf("\t\t\t\tbRes =%x\n",bRes);

//  Status=NtClose(Token);
//  dprintf("\t\t\t\tStatus =%x\n",Status);


  RtlRosInitUnicodeStringFromLiteral(&KeyName,L"test5");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, NULL, NULL);
  Status = NtLoadKey((HANDLE)HKEY_LOCAL_MACHINE,&ObjectAttributes);
  dprintf("\t\t\t\tStatus =%x\n",Status);
  dwError=RegLoadKey(HKEY_LOCAL_MACHINE,"def"
		,"test5");
  dprintf("\t\t\t\tdwError =%x\n",dwError);

  dprintf("NtOpenKey \\Registry\\Machine : ");
  RtlRosInitUnicodeStringFromLiteral(&KeyName, L"\\Registry\\Machine");
  InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
  Status=NtOpenKey( &hKey, MAXIMUM_ALLOWED, &ObjectAttributes);
  dprintf("\t\t\tStatus =%x\n",Status);
  RtlRosInitUnicodeStringFromLiteral(&KeyName,L"test5");
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE
				, NULL, NULL);
  Status = NtLoadKey(hKey,&ObjectAttributes);
  dprintf("\t\t\t\tStatus =%x\n",Status);
}

void test9(void)
{
    HANDLE hKey = NULL, hKey1;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry");
    ULONG Index,Length,i;
    KEY_BASIC_INFORMATION KeyInformation[5];
    KEY_VALUE_FULL_INFORMATION KeyValueInformation[5];

    dprintf("NtOpenKey \\Registry : ");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status=NtOpenKey( &hKey1, MAXIMUM_ALLOWED, &ObjectAttributes);
    dprintf("\t\t\t\tStatus =%x\n",Status);
    if (Status == 0) {
        dprintf("NtQueryKey : ");
        Status = NtQueryKey(hKey1, KeyBasicInformation, &KeyInformation[0], sizeof(KeyInformation), &Length);
        dprintf("\t\t\t\t\tStatus =%x\n",Status);
        if (Status == STATUS_SUCCESS) {
            dprintf("\tKey Name = ");
	        for (i=0;i<KeyInformation[0].NameLength/2;i++)
		        dprintf("%C",KeyInformation[0].Name[i]);
            dprintf("\n");
		}

        dprintf("NtEnumerateKey : \n");
        Index = 0;
        while (Status == STATUS_SUCCESS) {
            Status = NtEnumerateKey(hKey1,Index++,KeyBasicInformation,&KeyInformation[0], sizeof(KeyInformation),&Length);
            if (Status == STATUS_SUCCESS) {
                dprintf("\tSubKey Name = ");
                for (i = 0; i < KeyInformation[0].NameLength / 2; i++)
                    dprintf("%C",KeyInformation[0].Name[i]);
                dprintf("\n");
			}
		}
        dprintf("NtClose : ");
        Status = NtClose( hKey1 );
        dprintf("\t\t\t\t\tStatus =%x\n",Status);
	}
    NtClose(hKey); // RobD - hKey unused so-far, should this have been hKey1 ???

    dprintf("NtOpenKey \\Registry\\Machine : ");
    RtlRosInitUnicodeStringFromLiteral(&KeyName, L"\\Registry\\Machine");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&hKey1, MAXIMUM_ALLOWED, &ObjectAttributes);
    dprintf("\t\t\tStatus =%x\n",Status);

//Status of c0000001 opening \Registry\Machine\System\CurrentControlSet\Services\Tcpip\Linkage

//    dprintf("NtOpenKey System\\CurrentControlSet\\Services\\Tcpip : ");
//    RtlRosInitUnicodeStringFromLiteral(&KeyName, L"System\\CurrentControlSet\\Services\\Tcpip");
#if 1
    dprintf("NtOpenKey System\\ControlSet001\\Services\\Tcpip\\Parameters : ");
    RtlRosInitUnicodeStringFromLiteral(&KeyName, L"System\\ControlSet001\\Services\\Tcpip\\Parameters");
#else
    dprintf("NtOpenKey System\\CurrentControlSet\\Services\\Tcpip : ");
    RtlRosInitUnicodeStringFromLiteral(&KeyName, L"System\\CurrentControlSet\\Services\\Tcpip");
#endif
    InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, hKey1 , NULL);
    Status = NtOpenKey(&hKey, KEY_READ , &ObjectAttributes);
    dprintf("\t\t\tStatus =%x\n",Status);
    if (Status == 0) {
        dprintf("NtQueryValueKey : ");
        RtlRosInitUnicodeStringFromLiteral(&KeyName, L"NameServer");
        Status = NtQueryValueKey(hKey, &KeyName, KeyValueFullInformation, &KeyValueInformation[0], sizeof(KeyValueInformation), &Length);
        dprintf("\t\t\t\tStatus =%x\n",Status);
        if (Status == STATUS_SUCCESS) {
            dprintf("\tValue:DO=%d, DL=%d, NL=%d, Name = "
                ,KeyValueInformation[0].DataOffset
                ,KeyValueInformation[0].DataLength
                ,KeyValueInformation[0].NameLength);
            for (i = 0; i < 10 && i < KeyValueInformation[0].NameLength / 2; i++)
                dprintf("%C", KeyValueInformation[0].Name[i]);
            dprintf("\n");
            dprintf("\t\tType = %d\n", KeyValueInformation[0].Type);
            if (KeyValueInformation[0].Type == REG_SZ)
                //dprintf("\t\tValue = %S\n", KeyValueInformation[0].Name + 1 + KeyValueInformation[0].NameLength / 2);
                dprintf("\t\tValue = %S\n", KeyValueInformation[0].Name + KeyValueInformation[0].NameLength / 2);
        }
        dprintf("NtEnumerateValueKey : \n");
        Index = 0;
        while (Status == STATUS_SUCCESS) {
            Status = NtEnumerateValueKey(hKey, Index++, KeyValueFullInformation, &KeyValueInformation[0], sizeof(KeyValueInformation), &Length);
            if (Status == STATUS_SUCCESS) {
                dprintf("\tValue:DO=%d, DL=%d, NL=%d, Name = "
                    ,KeyValueInformation[0].DataOffset
                    ,KeyValueInformation[0].DataLength
                    ,KeyValueInformation[0].NameLength);
                for (i = 0; i < KeyValueInformation[0].NameLength / 2; i++)
                    dprintf("%C", KeyValueInformation[0].Name[i]);
                dprintf(", Type = %d\n", KeyValueInformation[0].Type);
                if (KeyValueInformation[0].Type == REG_SZ)
                    dprintf("\t\tValue = %S\n", ((char*)&KeyValueInformation[0]+KeyValueInformation[0].DataOffset));
                if (KeyValueInformation[0].Type == REG_DWORD)
                    dprintf("\t\tValue = %X\n", *((DWORD*)((char*)&KeyValueInformation[0]+KeyValueInformation[0].DataOffset)));
            }
        }
        dprintf("NtClose : ");
        Status = NtClose(hKey);
        dprintf("\t\t\t\t\tStatus =%x\n", Status);
    }
    NtClose(hKey1);
}


int main(int argc, char* argv[])
{
  char Buffer[10];
  DWORD Result;

  AllocConsole();
  InputHandle = GetStdHandle(STD_INPUT_HANDLE);
  OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  while(1)
  {
    dprintf("choose test :\n");
    dprintf("  0 = Exit\n");
    dprintf("  1 = Create key\n");
    dprintf("  2 = Delete key\n");
    dprintf("  3 = Enumerate key\n");
    dprintf("  4 = Set value (REG_SZ)\n");
    dprintf("  5 = Set value (REG_DWORD)\n");
    dprintf("  6 = Delete value\n");
    dprintf("  7 = Enumerate value\n");
    ReadConsoleA(InputHandle, Buffer, 3, &Result, NULL) ;
    switch (Buffer[0])
    {
     case '0':
      return(0);

     case '1':
      CreateKeyTest();
      break;

     case '2':
      DeleteKeyTest();
      break;

     case '3':
      EnumerateKeyTest();
      break;

     case '4':
      SetValueTest1();
      break;

     case '5':
      SetValueTest2();
      break;

     case '6':
      DeleteValueTest();
      break;

     case '7':
      EnumerateValueTest();
      break;
    }
  }
  return(0);
}

