/*
 *  FreeLoader
 *
 *  Copyright (C) 2001  Eric Kohl
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

/*
 * TODO:
 *	- Implement RegDeleteKey().
 *	- Implement RegQueryMultipleValue().
 *	- Fix RegEnumValue().
 */

#include "../freeldr.h"
#include "../memory.h"
#include "../stdlib.h"
#include "registry.h"

#define NDEBUG


static HKEY RootKey;


VOID
RegInitializeRegistry(VOID)
{
  RootKey = (HKEY)AllocateMemory(sizeof(KEY));

  InitializeListHead(&RootKey->SubKeyList);
  InitializeListHead(&RootKey->ValueList);
  InitializeListHead(&RootKey->KeyList);

  RootKey->NameSize = 2;
  RootKey->Name = (PUCHAR)AllocateMemory(2);
  strcpy(RootKey->Name, "\\");

  RootKey->Type = 0;
  RootKey->DataSize = 0;
  RootKey->Data = NULL;
}


LONG
RegCreateKey(HKEY ParentKey,
	     PCHAR KeyName,
	     PHKEY Key)
{
  PLIST_ENTRY Ptr;
  HKEY SearchKey;
  HKEY CurrentKey;
  HKEY NewKey;
  PCHAR p;
  PCHAR name;
  int subkeyLength;
  int stringLength;

#ifndef NDEBUG
  printf("RegCreateKey(%s) called\n", KeyName);
#endif

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


  while (*KeyName != 0)
    {
#ifndef NDEBUG
      printf("RegCreateKey(): KeyName '%s'\n", KeyName);
#endif
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
#ifndef NDEBUG
	  printf("RegCreateKey(): Ptr 0x%x\n", Ptr);
#endif
	  SearchKey = CONTAINING_RECORD(Ptr,
					KEY,
					KeyList);
#ifndef NDEBUG
	  printf("RegCreateKey(): SearchKey 0x%x\n", SearchKey);
	  printf("RegCreateKey(): searching '%s'\n", SearchKey->Name);
#endif
	  if (strncmp(SearchKey->Name, name, subkeyLength) == 0)
	    break;

	  Ptr = Ptr->Flink;
	}

      if (Ptr == &CurrentKey->SubKeyList)
	{
	  /* no key found -> create new subkey */
	  NewKey = (HKEY)AllocateMemory(sizeof(KEY));
	  if (NewKey == NULL)
	   return(ERROR_OUTOFMEMORY);

	  InitializeListHead(&NewKey->SubKeyList);
	  InitializeListHead(&NewKey->ValueList);

	  NewKey->Type = 0;
	  NewKey->DataSize = 0;
	  NewKey->Data = NULL;

	  InsertTailList(&CurrentKey->SubKeyList, &NewKey->KeyList);
	  NewKey->NameSize = subkeyLength + 1;
	  NewKey->Name = (PCHAR)AllocateMemory(NewKey->NameSize);
	  if (NewKey->Name == NULL)
	   return(ERROR_OUTOFMEMORY);
	  memcpy(NewKey->Name, name, subkeyLength);
	  NewKey->Name[subkeyLength] = 0;

#ifndef NDEBUG
	  printf("RegCreateKey(): new key 0x%x\n", NewKey);
	  printf("RegCreateKey(): new key '%s' length %d\n", NewKey->Name, NewKey->NameSize);
#endif

	  CurrentKey = NewKey;
	}
      else
	{
	  CurrentKey = SearchKey;
	}

      KeyName = KeyName + stringLength;
    }

  if (Key != NULL)
    *Key = CurrentKey;

  return(ERROR_SUCCESS);
}


LONG
RegDeleteKey(HKEY Key,
	     PCHAR Name)
{


  if (strchr(Name, '\\') != NULL)
    return(ERROR_INVALID_PARAMETER);



  return(ERROR_SUCCESS);
}


LONG
RegEnumKey(HKEY Key,
	   ULONG Index,
	   PCHAR Name,
	   PULONG NameSize)
{
  PLIST_ENTRY Ptr;
  HKEY SearchKey;
  ULONG Count = 0;
  ULONG Size;

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

#ifndef NDEBUG
  printf("RegEnumKey(): name '%s' length %d\n", SearchKey->Name, SearchKey->NameSize);
#endif

  Size = min(SearchKey->NameSize, *NameSize);
  *NameSize = Size;
  memcpy(Name, SearchKey->Name, Size);

  return(ERROR_SUCCESS);
}


LONG
RegOpenKey(HKEY ParentKey,
	   PCHAR KeyName,
	   PHKEY Key)
{
  PLIST_ENTRY Ptr;
  HKEY SearchKey;
  HKEY CurrentKey;
  PCHAR p;
  PCHAR name;
  int subkeyLength;
  int stringLength;

#ifndef NDEBUG
  printf("RegOpenKey(%s) called\n", KeyName);
#endif

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


  while (*KeyName != 0)
    {
#ifndef NDEBUG
      printf("RegOpenKey(): KeyName '%s'\n", KeyName);
#endif
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
#ifndef NDEBUG
	  printf("RegCreateKey(): Ptr 0x%x\n", Ptr);
#endif
	  SearchKey = CONTAINING_RECORD(Ptr,
					KEY,
					KeyList);
#ifndef NDEBUG
	  printf("RegOpenKey(): SearchKey 0x%x\n", SearchKey);
	  printf("RegOpenKey(): searching '%s'\n", SearchKey->Name);
#endif
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
	}

      KeyName = KeyName + stringLength;
    }

  if (Key != NULL)
    *Key = CurrentKey;

  return(ERROR_SUCCESS);
}


LONG
RegSetValue(HKEY Key,
	    PCHAR ValueName,
	    ULONG Type,
	    PUCHAR Data,
	    ULONG DataSize)
{
  PLIST_ENTRY Ptr;
  PVALUE Value;

#ifndef NDEBUG
  printf("RegSetValue(%x, '%s', %d, %x, %d)\n", (int)Key, ValueName, (int)Type, (int)Data, (int)DataSize);
#endif
  if ((ValueName == NULL) || (*ValueName == 0))
    {
      /* set default value */
      if (Key->Data != NULL)
        FreeMemory(Key->Data);
      Key->Data = (PUCHAR)AllocateMemory(DataSize);
      Key->DataSize = DataSize;
      Key->Type = Type;
      memcpy(Key->Data, Data, DataSize);
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
#ifndef NDEBUG
	  printf("Value->Name: '%s'\n", Value->Name);
#endif
	  if (stricmp(Value->Name, ValueName) == 0)
	    break;

	  Ptr = Ptr->Flink;
	}

      if (Ptr == &Key->ValueList)
	{
	  /* add new value */
#ifndef NDEBUG
	  printf("No value found - adding new value\n");
#endif
	  Value = (PVALUE)AllocateMemory(sizeof(VALUE));
	  if (Value == NULL)
	    return(ERROR_OUTOFMEMORY);
	  InsertTailList(&Key->ValueList, &Value->ValueList);
	  Value->NameSize = strlen(ValueName)+1;
	  Value->Name = (PCHAR)AllocateMemory(Value->NameSize);
	  if (Value->Name == NULL)
	    return(ERROR_OUTOFMEMORY);
	  strcpy(Value->Name, ValueName);
	  Value->Data = NULL;
	}

      /* set new value */
      if (DataSize <= sizeof(PUCHAR))
	{
	  Value->DataSize = DataSize;
	  Value->Type = Type;
	  memcpy(&Value->Data, Data, DataSize);
	}
      else
	{
	  if(Value->Data != NULL)
	    FreeMemory(Value->Data);
	  Value->Data = (PUCHAR)AllocateMemory(DataSize);
	  if (Value->Data == NULL)
	    return(ERROR_OUTOFMEMORY);
	  Value->DataSize = DataSize;
	  Value->Type = Type;
	  memcpy(Value->Data, Data, DataSize);
	}
    }
  return(ERROR_SUCCESS);
}


LONG
RegQueryValue(HKEY Key,
	      PCHAR ValueName,
	      PULONG Type,
	      PUCHAR Data,
	      PULONG DataSize)
{
  ULONG Size;
  PLIST_ENTRY Ptr;
  PVALUE Value;

  if ((ValueName == NULL) || (*ValueName == 0))
    {
      /* query default value */
      if (Key->Data == NULL)
	return(ERROR_INVALID_PARAMETER);

      if (Type != NULL)
	*Type = Key->Type;
      if ((Data != NULL) && (DataSize != NULL))
	{
	  Size = min(Key->DataSize, *DataSize);
	  memcpy(Data, Key->Data, Size);
	  *DataSize = Size;
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
#ifndef NDEBUG
	  printf("Name: %s\n", Value->Name);
#endif
	  if (stricmp(Value->Name, ValueName) == 0)
	    break;

	  Ptr = Ptr->Flink;
	}

      if (Ptr == &Key->ValueList)
	return(ERROR_INVALID_PARAMETER);

      if (Type != NULL)
	*Type = Value->Type;
      if ((Data != NULL) && (DataSize != NULL))
	{
	  if (*DataSize <= sizeof(PUCHAR))
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
    }

  return(ERROR_SUCCESS);
}


LONG
RegDeleteValue(HKEY Key,
	       PCHAR ValueName)
{
  PLIST_ENTRY Ptr;
  PVALUE Value;

  if ((ValueName == NULL) || (*ValueName == 0))
    {
      /* delete default value */
      if (Key->Data != NULL)
	FreeMemory(Key->Data);
      Key->Data = NULL;
      Key->DataSize = 0;
      Key->Type = 0;
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
      if (Value->Name != NULL)
	FreeMemory(Value->Name);
      Value->Name = NULL;
      Value->NameSize = 0;

      if (Value->DataSize > sizeof(PUCHAR))
	{
	  if (Value->Data != NULL)
	    FreeMemory(Value->Data);
	}
      Value->Data = NULL;
      Value->DataSize = 0;
      Value->Type = 0;

      RemoveEntryList(&Value->ValueList);
      FreeMemory(Value);
    }
  return(ERROR_SUCCESS);
}


LONG
RegEnumValue(HKEY Key,
	     ULONG Index,
	     PCHAR ValueName,
	     PULONG NameSize,
	     PULONG Type,
	     PUCHAR Data,
	     PULONG DataSize)
{
  PLIST_ENTRY Ptr;
  PVALUE Value;
  ULONG Count = 0;

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
	    *Type = Key->Type;
	  if (DataSize != NULL)
	    *DataSize = Key->DataSize;

	  /* FIXME: return more values */
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

  /* FIXME: return values */

  return(ERROR_SUCCESS);
}


#if 0
LONG
RegQueryMultipleValue(HKEY Key,
		      ...)
{
  return(ERROR_SUCCESS);
}
#endif

/* EOF */
