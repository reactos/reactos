/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: registry.c,v 1.2 2003/04/16 15:06:33 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/registry.c
 * PURPOSE:         Registry code
 * PROGRAMMER:      Eric Kohl
 */

/*
 * TODO:
 *	- Implement RegDeleteKey().
 *	- Implement RegQueryMultipleValue().
 *	- Fix RegEnumValue().
 */

#include <stdlib.h>
#include <string.h>

#include "mkhive.h"
#include "registry.h"


static HKEY RootKey;


VOID
RegInitializeRegistry(VOID)
{
  HKEY ControlSetKey;
  HKEY LinkKey;

  /* Create root key */
  RootKey = (HKEY)malloc(sizeof(KEY));

  InitializeListHead(&RootKey->SubKeyList);
  InitializeListHead(&RootKey->ValueList);
  InitializeListHead(&RootKey->KeyList);

  RootKey->SubKeyCount = 0;
  RootKey->ValueCount = 0;

  RootKey->NameSize = 2;
  RootKey->Name = (PUCHAR)malloc(2);
  strcpy(RootKey->Name, "\\");

  RootKey->DataType = 0;
  RootKey->DataSize = 0;
  RootKey->Data = NULL;

  /* Create SYSTEM key */
  RegCreateKey(RootKey,
	       "Registry\\Machine\\SYSTEM",
	       NULL);

  /* Create link 'CurrentControlSet' --> 'ControlSet001' */
  RegCreateKey(RootKey,
	       "Registry\\Machine\\SYSTEM\\ControlSet001",
	       &ControlSetKey);

  RegCreateKey(RootKey,
	       "Registry\\Machine\\SYSTEM\\CurrentControlSet",
	       &LinkKey);

  RegSetValue(LinkKey,
	      NULL,
	      REG_LINK,
	      (PUCHAR)&ControlSetKey,
	      sizeof(PVOID));

  /* Create HARDWARE key */
  RegCreateKey(RootKey,
	       "Registry\\Machine\\HARDWARE",
	       NULL);

  /* Create SAM key */
  RegCreateKey(RootKey,
	       "Registry\\Machine\\SAM",
	       NULL);

  /* Create SECURITY key */
  RegCreateKey(RootKey,
	       "Registry\\Machine\\SECURITY",
	       NULL);

  /* Create DEFAULT key */
  RegCreateKey(RootKey,
	       "Registry\\User\\.DEFAULT",
	       NULL);
}


LONG
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

  DPRINT ("KeyName '%s'\n", KeyName);

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
      DPRINT ("KeyName '%s'\n", KeyName);

      if (*KeyName == '\\')
	KeyName++;
      p = strchr (KeyName, '\\');
      if ((p != NULL) && (p != KeyName))
	{
	  subkeyLength = p - KeyName;
	  stringLength = subkeyLength + 1;
	  name = KeyName;
	}
      else
	{
	  subkeyLength = strlen (KeyName);
	  stringLength = subkeyLength;
	  name = KeyName;
	}

      Ptr = CurrentKey->SubKeyList.Flink;
      while (Ptr != &CurrentKey->SubKeyList)
	{
	  DPRINT ("Ptr 0x%x\n", Ptr);

	  SearchKey = CONTAINING_RECORD(Ptr,
					KEY,
					KeyList);
	  DPRINT ("SearchKey 0x%x\n", SearchKey);
	  DPRINT ("Searching '%s'\n", SearchKey->Name);
	  if (strncmp (SearchKey->Name, name, subkeyLength) == 0)
	    break;

	  Ptr = Ptr->Flink;
	}

      if (Ptr == &CurrentKey->SubKeyList)
	{
	  /* no key found -> create new subkey */
	  NewKey = (HKEY)malloc (sizeof(KEY));
	  if (NewKey == NULL)
	   return ERROR_OUTOFMEMORY;

	  InitializeListHead (&NewKey->SubKeyList);
	  InitializeListHead (&NewKey->ValueList);

	  NewKey->SubKeyCount = 0;
	  NewKey->ValueCount = 0;

	  NewKey->DataType = 0;
	  NewKey->DataSize = 0;
	  NewKey->Data = NULL;

	  InsertTailList (&CurrentKey->SubKeyList, &NewKey->KeyList);
	  CurrentKey->SubKeyCount++;

	  NewKey->NameSize = subkeyLength + 1;
	  NewKey->Name = (PCHAR)malloc (NewKey->NameSize);
	  if (NewKey->Name == NULL)
	   return(ERROR_OUTOFMEMORY);
	  memcpy(NewKey->Name, name, subkeyLength);
	  NewKey->Name[subkeyLength] = 0;

	  DPRINT ("NewKey 0x%x\n", NewKey);
	  DPRINT ("NewKey '%s'  Length %d\n", NewKey->Name, NewKey->NameSize);

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

  return ERROR_SUCCESS;
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

  DPRINT ("Name '%s'  Length %d\n", SearchKey->Name, SearchKey->NameSize);

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
  HKEY SearchKey = INVALID_HANDLE_VALUE;
  HKEY CurrentKey;
  PCHAR p;
  PCHAR name;
  int subkeyLength;
  int stringLength;

  DPRINT("KeyName '%s'\n", KeyName);

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
      DPRINT ("KeyName '%s'\n", KeyName);

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
	  DPRINT ("Ptr 0x%x\n", Ptr);

	  SearchKey = CONTAINING_RECORD(Ptr,
					KEY,
					KeyList);

	  DPRINT ("SearchKey 0x%x\n", SearchKey);
	  DPRINT ("Searching '%s'\n", SearchKey->Name);

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


LONG
RegSetValue(HKEY Key,
	    PCHAR ValueName,
	    ULONG Type,
	    PUCHAR Data,
	    ULONG DataSize)
{
  PLIST_ENTRY Ptr;
  PVALUE Value = NULL;

  DPRINT ("Key 0x%x, ValueName '%s', Type %d, Data 0x%x, DataSize %d\n",
	  (int)Key, ValueName, (int)Type, (int)Data, (int)DataSize);

  if ((ValueName == NULL) || (*ValueName == 0))
    {
      /* set default value */
      if ((Key->Data != NULL) && (Key->DataSize > sizeof(PUCHAR)))
	{
	  free(Key->Data);
	}

      if (DataSize <= sizeof(PUCHAR))
	{
	  Key->DataSize = DataSize;
	  Key->DataType = Type;
	  memcpy(&Key->Data, Data, DataSize);
	}
      else
	{
	  Key->Data = (PUCHAR)malloc(DataSize);
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

	  DPRINT ("Value->Name '%s'\n", Value->Name);

	  if (stricmp(Value->Name, ValueName) == 0)
	    break;

	  Ptr = Ptr->Flink;
	}

      if (Ptr == &Key->ValueList)
	{
	  /* add new value */
	  DPRINT("No value found - adding new value\n");

	  Value = (PVALUE)malloc(sizeof(VALUE));
	  if (Value == NULL)
	    return(ERROR_OUTOFMEMORY);
	  InsertTailList(&Key->ValueList, &Value->ValueList);
	  Key->ValueCount++;
	  Value->NameSize = strlen(ValueName)+1;
	  Value->Name = (PCHAR)malloc(Value->NameSize);
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
	  free(Value->Data);
	}

      if (DataSize <= sizeof(PUCHAR))
	{
	  Value->DataSize = DataSize;
	  Value->DataType = Type;
	  memcpy(&Value->Data, Data, DataSize);
	}
      else
	{
	  Value->Data = (PUCHAR)malloc(DataSize);
	  if (Value->Data == NULL)
	    return(ERROR_OUTOFMEMORY);
	  Value->DataType = Type;
	  Value->DataSize = DataSize;
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

	  DPRINT("Searching for '%s'. Value name '%s'\n", ValueName, Value->Name);

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


LONG
RegDeleteValue(HKEY Key,
	       PCHAR ValueName)
{
  PLIST_ENTRY Ptr;
  PVALUE Value = NULL;

  if ((ValueName == NULL) || (*ValueName == 0))
    {
      /* delete default value */
      if (Key->Data != NULL)
	free(Key->Data);
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
	free(Value->Name);
      Value->Name = NULL;
      Value->NameSize = 0;

      if (Value->DataSize > sizeof(PUCHAR))
	{
	  if (Value->Data != NULL)
	    free(Value->Data);
	}
      Value->Data = NULL;
      Value->DataSize = 0;
      Value->DataType = 0;

      RemoveEntryList(&Value->ValueList);
      free(Value);
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
	    *Type = Key->DataType;
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


ULONG
RegGetSubKeyCount (HKEY Key)
{
  return Key->SubKeyCount;
}


ULONG
RegGetValueCount (HKEY Key)
{
  if (Key->DataSize != 0)
    return Key->ValueCount + 1;

  return Key->ValueCount;
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
