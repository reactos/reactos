/*
 *  FreeLoader
 *
 *  Copyright (C) 2001, 2002  Eric Kohl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>
#include <mm.h>
#include <rtl.h>
#include <debug.h>
#include "registry.h"

#include <ui.h>

static HKEY RootKey;


VOID
RegInitializeRegistry (VOID)
{
#if 0
  HKEY TestKey;
#endif

  /* Create root key */
  RootKey = (HKEY) MmAllocateMemory (sizeof(KEY));

  InitializeListHead (&RootKey->SubKeyList);
  InitializeListHead (&RootKey->ValueList);
  InitializeListHead (&RootKey->KeyList);

  RootKey->SubKeyCount = 0;
  RootKey->ValueCount = 0;

  RootKey->NameSize = 2;
  RootKey->Name = (PUCHAR)MmAllocateMemory (2);
  strcpy (RootKey->Name, "\\");

  RootKey->DataType = 0;
  RootKey->DataSize = 0;
  RootKey->Data = NULL;

  /* Create 'SYSTEM' key */
  RegCreateKey (RootKey,
		"Registry\\Machine\\SYSTEM",
		NULL);

  /* Create 'HARDWARE' key */
  RegCreateKey (RootKey,
		"Registry\\Machine\\HARDWARE",
		NULL);

  /* Create 'HARDWARE\DESCRIPTION' key */
  RegCreateKey (RootKey,
		"Registry\\Machine\\HARDWARE\\DESCRIPTION",
		NULL);

  /* Create 'HARDWARE\DEVICEMAP' key */
  RegCreateKey (RootKey,
		"Registry\\Machine\\HARDWARE\\DEVICEMAP",
		NULL);

  /* Create 'HARDWARE\RESOURCEMAP' key */
  RegCreateKey (RootKey,
		"Registry\\Machine\\HARDWARE\\RESOURCEMAP",
		NULL);

/* Testcode */
#if 0
  RegCreateKey (RootKey,
		"Registry\\Machine\\HARDWARE\\DESCRIPTION\\TestKey",
		&TestKey);

  RegSetValue (TestKey,
	       "TestValue",
	       REG_SZ,
	       (PUCHAR)"TestString",
	       11);
#endif
}


S32
RegInitCurrentControlSet(BOOL LastKnownGood)
{
  CHAR ControlSetKeyName[80];
  HKEY SelectKey;
  HKEY SystemKey;
  HKEY ControlSetKey;
  HKEY LinkKey;
  U32 CurrentSet = 0;
  U32 DefaultSet = 0;
  U32 LastKnownGoodSet = 0;
  U32 DataSize;
  S32 Error;

  Error = RegOpenKey(NULL,
		     "\\Registry\\Machine\\SYSTEM\\Select",
		    &SelectKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "RegOpenKey() failed (Error %u)\n", (int)Error));
      return(Error);
    }

  DataSize = sizeof(U32);
  Error = RegQueryValue(SelectKey,
			"Default",
			NULL,
			(PUCHAR)&DefaultSet,
			&DataSize);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "RegQueryValue('Default') failed (Error %u)\n", (int)Error));
      return(Error);
    }

  DataSize = sizeof(U32);
  Error = RegQueryValue(SelectKey,
			"LastKnownGood",
			NULL,
			(PUCHAR)&LastKnownGoodSet,
			&DataSize);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "RegQueryValue('Default') failed (Error %u)\n", (int)Error));
      return(Error);
    }

  CurrentSet = (LastKnownGood == TRUE) ? LastKnownGoodSet : DefaultSet;
  strcpy(ControlSetKeyName, "ControlSet");
  switch(CurrentSet)
    {
      case 1:
	strcat(ControlSetKeyName, "001");
	break;
      case 2:
	strcat(ControlSetKeyName, "002");
	break;
      case 3:
	strcat(ControlSetKeyName, "003");
	break;
      case 4:
	strcat(ControlSetKeyName, "004");
	break;
      case 5:
	strcat(ControlSetKeyName, "005");
	break;
    }

  Error = RegOpenKey(NULL,
		     "\\Registry\\Machine\\SYSTEM",
		     &SystemKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "RegOpenKey(SystemKey) failed (Error %u)\n", (int)Error));
      return(Error);
    }

  Error = RegOpenKey(SystemKey,
		     ControlSetKeyName,
		     &ControlSetKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "RegOpenKey(ControlSetKey) failed (Error %u)\n", (int)Error));
      return(Error);
    }

  Error = RegCreateKey(SystemKey,
		       "CurrentControlSet",
		       &LinkKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "RegCreateKey(LinkKey) failed (Error %u)\n", (int)Error));
      return(Error);
    }

  Error = RegSetValue(LinkKey,
		      NULL,
		      REG_LINK,
		      (PUCHAR)&ControlSetKey,
		      sizeof(PVOID));
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "RegSetValue(LinkKey) failed (Error %u)\n", (int)Error));
      return(Error);
    }

  return(ERROR_SUCCESS);
}


S32
RegCreateKey(HKEY ParentKey,
	     PCHAR KeyName,
	     PHKEY Key)
{
  PLIST_ENTRY Ptr;
  HKEY SearchKey = INVALID_HANDLE_VALUE;
  HKEY CurrentKey;
  HKEY NewKey;
  PCHAR p;
  PCHAR name;
  int subkeyLength;
  int stringLength;

  DbgPrint((DPRINT_REGISTRY, "KeyName '%s'\n", KeyName));

  if (*KeyName == '\\')
    {
      KeyName++;
      CurrentKey = RootKey;
    }
  else if (ParentKey == NULL)
    {
      CurrentKey = RootKey;
    }
  else
    {
      CurrentKey = ParentKey;
    }

  /* Check whether current key is a link */
  if (CurrentKey->DataType == REG_LINK)
    {
      CurrentKey = (HKEY)CurrentKey->Data;
    }

  while (*KeyName != 0)
    {
      DbgPrint((DPRINT_REGISTRY, "KeyName '%s'\n", KeyName));

      if (*KeyName == '\\')
	KeyName++;
      p = strchr(KeyName, '\\');
      if ((p != NULL) && (p != KeyName))
	{
	  subkeyLength = p - KeyName;
	  stringLength = subkeyLength + 1;
	  name = KeyName;
	}
      else
	{
	  subkeyLength = strlen(KeyName);
	  stringLength = subkeyLength;
	  name = KeyName;
	}

      Ptr = CurrentKey->SubKeyList.Flink;
      while (Ptr != &CurrentKey->SubKeyList)
	{
	  DbgPrint((DPRINT_REGISTRY, "Ptr 0x%x\n", Ptr));

	  SearchKey = CONTAINING_RECORD(Ptr,
					KEY,
					KeyList);
	  DbgPrint((DPRINT_REGISTRY, "SearchKey 0x%x\n", SearchKey));
	  DbgPrint((DPRINT_REGISTRY, "Searching '%s'\n", SearchKey->Name));
	  if (strncmp(SearchKey->Name, name, subkeyLength) == 0)
	    break;

	  Ptr = Ptr->Flink;
	}

      if (Ptr == &CurrentKey->SubKeyList)
	{
	  /* no key found -> create new subkey */
	  NewKey = (HKEY)MmAllocateMemory(sizeof(KEY));
	  if (NewKey == NULL)
	   return(ERROR_OUTOFMEMORY);

	  InitializeListHead(&NewKey->SubKeyList);
	  InitializeListHead(&NewKey->ValueList);

	  NewKey->SubKeyCount = 0;
	  NewKey->ValueCount = 0;

	  NewKey->DataType = 0;
	  NewKey->DataSize = 0;
	  NewKey->Data = NULL;

	  InsertTailList(&CurrentKey->SubKeyList, &NewKey->KeyList);
	  CurrentKey->SubKeyCount++;

	  NewKey->NameSize = subkeyLength + 1;
	  NewKey->Name = (PCHAR)MmAllocateMemory(NewKey->NameSize);
	  if (NewKey->Name == NULL)
	   return(ERROR_OUTOFMEMORY);
	  memcpy(NewKey->Name, name, subkeyLength);
	  NewKey->Name[subkeyLength] = 0;

	  DbgPrint((DPRINT_REGISTRY, "NewKey 0x%x\n", NewKey));
	  DbgPrint((DPRINT_REGISTRY, "NewKey '%s'  Length %d\n", NewKey->Name, NewKey->NameSize));

	  CurrentKey = NewKey;
	}
      else
	{
	  CurrentKey = SearchKey;

	  /* Check whether current key is a link */
	  if (CurrentKey->DataType == REG_LINK)
	    {
	      CurrentKey = (HKEY)CurrentKey->Data;
	    }
	}

      KeyName = KeyName + stringLength;
    }

  if (Key != NULL)
    *Key = CurrentKey;

  return(ERROR_SUCCESS);
}


S32
RegDeleteKey(HKEY Key,
	     PCHAR Name)
{


  if (strchr(Name, '\\') != NULL)
    return(ERROR_INVALID_PARAMETER);



  return(ERROR_SUCCESS);
}


S32
RegEnumKey(HKEY Key,
	   U32 Index,
	   PCHAR Name,
	   U32* NameSize)
{
  PLIST_ENTRY Ptr;
  HKEY SearchKey;
  U32 Count = 0;
  U32 Size;

  Ptr = Key->SubKeyList.Flink;
  while (Ptr != &Key->SubKeyList)
    {
      if (Index == Count)
	break;

      Count++;
      Ptr = Ptr->Flink;
    }

  if (Ptr == &Key->SubKeyList)
    return(ERROR_NO_MORE_ITEMS);

  SearchKey = CONTAINING_RECORD(Ptr,
				KEY,
				KeyList);

  DbgPrint((DPRINT_REGISTRY, "Name '%s'  Length %d\n", SearchKey->Name, SearchKey->NameSize));

  Size = min(SearchKey->NameSize, *NameSize);
  *NameSize = Size;
  memcpy(Name, SearchKey->Name, Size);

  return(ERROR_SUCCESS);
}


S32
RegOpenKey(HKEY ParentKey,
	   PCHAR KeyName,
	   PHKEY Key)
{
  PLIST_ENTRY Ptr;
  HKEY SearchKey = INVALID_HANDLE_VALUE;
  HKEY CurrentKey;
  PCHAR p;
  PCHAR name;
  int subkeyLength;
  int stringLength;

  DbgPrint((DPRINT_REGISTRY, "KeyName '%s'\n", KeyName));

  *Key = NULL;

  if (*KeyName == '\\')
    {
      KeyName++;
      CurrentKey = RootKey;
    }
  else if (ParentKey == NULL)
    {
      CurrentKey = RootKey;
    }
  else
    {
      CurrentKey = ParentKey;
    }

  /* Check whether current key is a link */
  if (CurrentKey->DataType == REG_LINK)
    {
      CurrentKey = (HKEY)CurrentKey->Data;
    }

  while (*KeyName != 0)
    {
      DbgPrint((DPRINT_REGISTRY, "KeyName '%s'\n", KeyName));

      if (*KeyName == '\\')
	KeyName++;
      p = strchr(KeyName, '\\');
      if ((p != NULL) && (p != KeyName))
	{
	  subkeyLength = p - KeyName;
	  stringLength = subkeyLength + 1;
	  name = KeyName;
	}
      else
	{
	  subkeyLength = strlen(KeyName);
	  stringLength = subkeyLength;
	  name = KeyName;
	}

      Ptr = CurrentKey->SubKeyList.Flink;
      while (Ptr != &CurrentKey->SubKeyList)
	{
	  DbgPrint((DPRINT_REGISTRY, "Ptr 0x%x\n", Ptr));

	  SearchKey = CONTAINING_RECORD(Ptr,
					KEY,
					KeyList);

	  DbgPrint((DPRINT_REGISTRY, "SearchKey 0x%x\n", SearchKey));
	  DbgPrint((DPRINT_REGISTRY, "Searching '%s'\n", SearchKey->Name));

	  if (strncmp(SearchKey->Name, name, subkeyLength) == 0)
	    break;

	  Ptr = Ptr->Flink;
	}

      if (Ptr == &CurrentKey->SubKeyList)
	{
	  return(ERROR_PATH_NOT_FOUND);
	}
      else
	{
	  CurrentKey = SearchKey;

	  /* Check whether current key is a link */
	  if (CurrentKey->DataType == REG_LINK)
	    {
	      CurrentKey = (HKEY)CurrentKey->Data;
	    }
	}

      KeyName = KeyName + stringLength;
    }

  if (Key != NULL)
    *Key = CurrentKey;

  return(ERROR_SUCCESS);
}


S32
RegSetValue(HKEY Key,
	    PCHAR ValueName,
	    U32 Type,
	    PUCHAR Data,
	    U32 DataSize)
{
  PLIST_ENTRY Ptr;
  PVALUE Value = NULL;

  DbgPrint((DPRINT_REGISTRY, "Key 0x%x, ValueName '%s', Type %d, Data 0x%x, DataSize %d\n",
    (int)Key, ValueName, (int)Type, (int)Data, (int)DataSize));

  if ((ValueName == NULL) || (*ValueName == 0))
    {
      /* set default value */
      if ((Key->Data != NULL) && (Key->DataSize > sizeof(PUCHAR)))
	{
	  MmFreeMemory(Key->Data);
	}

      if (DataSize <= sizeof(PUCHAR))
	{
	  Key->DataSize = DataSize;
	  Key->DataType = Type;
	  memcpy(&Key->Data, Data, DataSize);
	}
      else
	{
	  Key->Data = (PUCHAR)MmAllocateMemory(DataSize);
	  Key->DataSize = DataSize;
	  Key->DataType = Type;
	  memcpy(Key->Data, Data, DataSize);
	}
    }
  else
    {
      /* set non-default value */
      Ptr = Key->ValueList.Flink;
      while (Ptr != &Key->ValueList)
	{
	  Value = CONTAINING_RECORD(Ptr,
				    VALUE,
				    ValueList);

	  DbgPrint((DPRINT_REGISTRY, "Value->Name '%s'\n", Value->Name));

	  if (stricmp(Value->Name, ValueName) == 0)
	    break;

	  Ptr = Ptr->Flink;
	}

      if (Ptr == &Key->ValueList)
	{
	  /* add new value */
	  DbgPrint((DPRINT_REGISTRY, "No value found - adding new value\n"));

	  Value = (PVALUE)MmAllocateMemory(sizeof(VALUE));
	  if (Value == NULL)
	    return(ERROR_OUTOFMEMORY);

	  InsertTailList(&Key->ValueList, &Value->ValueList);
	  Key->ValueCount++;

	  Value->NameSize = strlen(ValueName)+1;
	  Value->Name = (PCHAR)MmAllocateMemory(Value->NameSize);
	  if (Value->Name == NULL)
	    return(ERROR_OUTOFMEMORY);
	  strcpy(Value->Name, ValueName);
	  Value->DataType = REG_NONE;
	  Value->DataSize = 0;
	  Value->Data = NULL;
	}

      /* set new value */
      if ((Value->Data != NULL) && (Value->DataSize > sizeof(PUCHAR)))
	{
	  MmFreeMemory(Value->Data);
	}

      if (DataSize <= sizeof(PUCHAR))
	{
	  Value->DataSize = DataSize;
	  Value->DataType = Type;
	  memcpy(&Value->Data, Data, DataSize);
	}
      else
	{
	  Value->Data = (PUCHAR)MmAllocateMemory(DataSize);
	  if (Value->Data == NULL)
	    return(ERROR_OUTOFMEMORY);
	  Value->DataType = Type;
	  Value->DataSize = DataSize;
	  memcpy(Value->Data, Data, DataSize);
	}
    }
  return(ERROR_SUCCESS);
}


S32
RegQueryValue(HKEY Key,
	      PCHAR ValueName,
	      U32* Type,
	      PUCHAR Data,
	      U32* DataSize)
{
  U32 Size;
  PLIST_ENTRY Ptr;
  PVALUE Value = NULL;

  if ((ValueName == NULL) || (*ValueName == 0))
    {
      /* query default value */
      if (Key->Data == NULL)
	return(ERROR_INVALID_PARAMETER);

      if (Type != NULL)
	*Type = Key->DataType;
      if ((Data != NULL) && (DataSize != NULL))
	{
	  if (Key->DataSize <= sizeof(PUCHAR))
	    {
	      Size = min(Key->DataSize, *DataSize);
	      memcpy(Data, &Key->Data, Size);
	      *DataSize = Size;
	    }
	  else
	    {
	      Size = min(Key->DataSize, *DataSize);
	      memcpy(Data, Key->Data, Size);
	      *DataSize = Size;
	    }
	}
      else if ((Data == NULL) && (DataSize != NULL))
	{
	  *DataSize = Key->DataSize;
	}
    }
  else
    {
      /* query non-default value */
      Ptr = Key->ValueList.Flink;
      while (Ptr != &Key->ValueList)
	{
	  Value = CONTAINING_RECORD(Ptr,
				    VALUE,
				    ValueList);

	  DbgPrint((DPRINT_REGISTRY, "Searching for '%s'. Value name '%s'\n", ValueName, Value->Name));

	  if (stricmp(Value->Name, ValueName) == 0)
	    break;

	  Ptr = Ptr->Flink;
	}

      if (Ptr == &Key->ValueList)
	return(ERROR_INVALID_PARAMETER);

      if (Type != NULL)
	*Type = Value->DataType;
      if ((Data != NULL) && (DataSize != NULL))
	{
	  if (Value->DataSize <= sizeof(PUCHAR))
	    {
	      Size = min(Value->DataSize, *DataSize);
	      memcpy(Data, &Value->Data, Size);
	      *DataSize = Size;
	    }
	  else
	    {
	      Size = min(Value->DataSize, *DataSize);
	      memcpy(Data, Value->Data, Size);
	      *DataSize = Size;
	    }
	}
      else if ((Data == NULL) && (DataSize != NULL))
	{
	  *DataSize = Value->DataSize;
	}
    }

  return(ERROR_SUCCESS);
}


S32
RegDeleteValue(HKEY Key,
	       PCHAR ValueName)
{
  PLIST_ENTRY Ptr;
  PVALUE Value = NULL;

  if ((ValueName == NULL) || (*ValueName == 0))
    {
      /* delete default value */
      if (Key->Data != NULL)
	MmFreeMemory(Key->Data);
      Key->Data = NULL;
      Key->DataSize = 0;
      Key->DataType = 0;
    }
  else
    {
      /* delete non-default value */
      Ptr = Key->ValueList.Flink;
      while (Ptr != &Key->ValueList)
	{
	  Value = CONTAINING_RECORD(Ptr,
				    VALUE,
				    ValueList);
	  if (strcmp(Value->Name, ValueName) == 0)
	    break;

	  Ptr = Ptr->Flink;
	}

      if (Ptr == &Key->ValueList)
	return(ERROR_INVALID_PARAMETER);

      /* delete value */
      Key->ValueCount--;
      if (Value->Name != NULL)
	MmFreeMemory(Value->Name);
      Value->Name = NULL;
      Value->NameSize = 0;

      if (Value->DataSize > sizeof(PUCHAR))
	{
	  if (Value->Data != NULL)
	    MmFreeMemory(Value->Data);
	}
      Value->Data = NULL;
      Value->DataSize = 0;
      Value->DataType = 0;

      RemoveEntryList(&Value->ValueList);
      MmFreeMemory(Value);
    }
  return(ERROR_SUCCESS);
}


S32
RegEnumValue(HKEY Key,
	     U32 Index,
	     PCHAR ValueName,
	     U32* NameSize,
	     U32* Type,
	     PUCHAR Data,
	     U32* DataSize)
{
  PLIST_ENTRY Ptr;
  PVALUE Value;
  U32 Count = 0;

  if (Key->Data != NULL)
    {
      if (Index > 0)
	{
	  Index--;
	}
      else
	{
	  /* enumerate default value */
	  if (ValueName != NULL)
	    *ValueName = 0;
	  if (Type != NULL)
	    *Type = Key->DataType;
      if (Data != NULL)
        {
          if (Key->DataSize <= sizeof(PUCHAR))
            {
              memcpy(Data, &Key->Data, min(Key->DataSize, *DataSize));
            }
          else
            {
              memcpy(Data, Key->Data, min(Key->DataSize, *DataSize));
            }
        }
	  if (DataSize != NULL)
	    *DataSize = min(Key->DataSize, *DataSize);

      return(ERROR_SUCCESS);
	}
    }

  Ptr = Key->ValueList.Flink;
  while (Ptr != &Key->ValueList)
    {
      if (Index == Count)
	break;

      Count++;
      Ptr = Ptr->Flink;
    }

  if (Ptr == &Key->ValueList)
    return(ERROR_NO_MORE_ITEMS);

  Value = CONTAINING_RECORD(Ptr,
			    VALUE,
			    ValueList);

  /* enumerate non-default value */
  if (ValueName != NULL)
    memcpy(ValueName, Value->Name, min(Value->NameSize, *NameSize));
  if (Type != NULL)
    *Type = Value->DataType;

  if (Data != NULL)
    {
      if (Value->DataSize <= sizeof(PUCHAR))
        {
          memcpy(Data, &Value->Data, min(Value->DataSize, *DataSize));
        }
      else
        {
          memcpy(Data, Value->Data, min(Value->DataSize, *DataSize));
        }
    }

  if (DataSize != NULL)
    *DataSize = min(Value->DataSize, *DataSize);

  return(ERROR_SUCCESS);
}


U32
RegGetSubKeyCount (HKEY Key)
{
  return Key->SubKeyCount;
}


U32
RegGetValueCount (HKEY Key)
{
  if (Key->DataSize != 0)
    return Key->ValueCount + 1;

  return Key->ValueCount;
}

/* EOF */
