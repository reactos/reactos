/*
 *  FreeLoader
 *
 *  Copyright (C) 2001  Rex Jolliff
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

#include <freeldr.h>
#include <rtl.h>
#include <mm.h>
#include <debug.h>

#include "registry.h"


#define  REGISTRY_FILE_MAGIC    "REGEDIT4"

static PCHAR 
checkAndSkipMagic (PCHAR  regChunk)
{
  if (strncmp (regChunk, 
               REGISTRY_FILE_MAGIC, 
               strlen (REGISTRY_FILE_MAGIC)) != 0)
  {
    DbgPrint((DPRINT_REGISTRY, "Incorrect magic number in registry chunk. Expected: '%s' Got: '%.*s'\n",
      REGISTRY_FILE_MAGIC, strlen(REGISTRY_FILE_MAGIC), regChunk));

    return  0;
  }
  regChunk += strlen (REGISTRY_FILE_MAGIC);

  DbgPrint((DPRINT_REGISTRY, "Found registry chunk magic value\n"));

  return regChunk;
}

static PCHAR
skipWhitespaceInChunk (PCHAR regChunk)
{
  while (*regChunk && isspace (*regChunk))
    regChunk++;

  return *regChunk ? regChunk : 0;
}

static int
computeKeyNameSize (PCHAR  regChunk)
{
  int  copyCount = 0;

  while (*regChunk != 0 && *regChunk != ']')
  {
    copyCount++;
    regChunk++;
  }

  return  copyCount;
}

static BOOL
allocateKeyName(PCHAR *newKeyName, int newKeySize)
{
  if (*newKeyName != NULL)
    MmFreeMemory(*newKeyName);

  *newKeyName = MmAllocateMemory(newKeySize + 1);
  if (*newKeyName == NULL)
    return(FALSE);

  memset(*newKeyName, 0, newKeySize + 1);

  return(TRUE);
}

static PCHAR
skipToNextKeyInChunk (PCHAR  regChunk)
{
  while (*regChunk != 0 && *regChunk != '[')
  {
    while (*regChunk != 0 && *regChunk != '\n')
    {
      regChunk++;
    }
    regChunk++;
  }

  return  *regChunk ? regChunk : 0;
}

static PCHAR
getKeyNameFromChunk (PCHAR  regChunk, PCHAR  newKeyName)
{
  int index = 0;

  while (*regChunk != 0 && *regChunk != ']')
  {
    newKeyName[index++] = *regChunk++;
  }
  newKeyName[index] = '\0';

  return  *regChunk ? regChunk : 0;
}

static HKEY
createNewKey (PCHAR newKeyName)
{
  HKEY handleToReturn = NULL;

  DbgPrint((DPRINT_REGISTRY, "Adding new key '%s'\n", newKeyName));

  RegCreateKey(NULL,
	       newKeyName,
	       &handleToReturn);

  DbgPrint((DPRINT_REGISTRY, "Returned handle: 0x%x\n", handleToReturn));

  return handleToReturn;
}

static PCHAR
skipToNextKeyValueInChunk (PCHAR  regChunk)
{
  while (*regChunk != 0 && *regChunk != '\n')
    regChunk++;
  regChunk = skipWhitespaceInChunk (regChunk);
  
  return  regChunk;
}

static int
computeKeyValueNameSize (PCHAR  regChunk)
{
  int size = 0;

  if (*regChunk != '\"')
    return  0;
  regChunk++;
  while (*regChunk != 0 && *regChunk != '\"')
  {
    size++;
    regChunk++;
  }

  return  regChunk ? size : 0;
}

static PCHAR
getKeyValueNameFromChunk (PCHAR regChunk, PCHAR newKeyName)
{
  int  index = 0;

  regChunk++;
  while (*regChunk != 0 && *regChunk != '\"')
  {
    newKeyName[index++] = *regChunk++;
  }
  newKeyName[index] = '\0';
  regChunk++;

  return  *regChunk ? regChunk : 0;
}

static PCHAR 
getKeyValueTypeFromChunk (PCHAR  regChunk, PCHAR  dataFormat, int *keyValueType)
{
  if (*regChunk == '\"')
  {
    strcpy (dataFormat, "string");
    *keyValueType = REG_SZ;
  }
  else if (strncmp (regChunk, "hex", 3) == 0)
  {
    strcpy (dataFormat, "hex");
    regChunk += 3;
    if (*regChunk == '(')
    {
      regChunk++;
      *keyValueType = atoi (regChunk);
      while (*regChunk != 0 && *regChunk != ')')
        regChunk++;
      regChunk++;
    }
    else
      *keyValueType = REG_BINARY;
    if (*regChunk == ':')
      regChunk++;
  }
  else if (strncmp (regChunk, "dword", 5) == 0)
  {
    strcpy (dataFormat, "dword");
    *keyValueType = REG_DWORD;
    regChunk += 5;
    if (*regChunk == ':')
      regChunk++;
  }
  else if (strncmp (regChunk, "multi", 5) == 0)
  {
    strcpy (dataFormat, "multi");
    *keyValueType = REG_MULTI_SZ;
    regChunk += 5;
    if (*regChunk == ':')
      regChunk++;
  }
  else if (strncmp (regChunk, "expand", 6) == 0)
  {
    strcpy (dataFormat, "expand");
    *keyValueType = REG_EXPAND_SZ;
    regChunk += 6;
    if (*regChunk == ':')
      regChunk++;
  }
  else
  {
    UNIMPLEMENTED;
  }

  return  *regChunk ? regChunk : 0;
}

static int 
computeKeyValueDataSize (PCHAR  regChunk, PCHAR  dataFormat)
{
  int  dataSize = 0;

  if (strcmp (dataFormat, "string") == 0)
  {
    regChunk++;
    while (*regChunk != 0 && *regChunk != '\"')
    {
      dataSize++;
      regChunk++;
    }
    dataSize++;
  }
  else if (strcmp (dataFormat, "hex") == 0)
  {
    while (*regChunk != 0 && isxdigit(*regChunk))
    {
      regChunk++;
      regChunk++;
      dataSize++;
      if (*regChunk == ',')
      {
        regChunk++;
        if (*regChunk == '\\')
        {
          regChunk++;
          regChunk = skipWhitespaceInChunk (regChunk);
        }
      }
    }
  }
  else if (strcmp (dataFormat, "dword") == 0)
  {
    dataSize = sizeof(U32);
    while (*regChunk != 0 && isxdigit(*regChunk))
    {
      regChunk++;
    }
  }
  else if (strcmp (dataFormat, "multi") == 0)
  {
    while (*regChunk == '\"')
    {
      regChunk++;
      while (*regChunk != 0 && *regChunk != '\"')
      {
        dataSize++;
        regChunk++;
      }
      regChunk++;
      dataSize++;
      if (*regChunk == ',')
      {
        regChunk++;
        regChunk = skipWhitespaceInChunk (regChunk);
        if (*regChunk == '\\')
        {
          regChunk++;
          regChunk = skipWhitespaceInChunk (regChunk);
        }
      }
      else
        break;
    }
    dataSize++;
  }
  else if (strcmp (dataFormat, "expand") == 0)
  {
    regChunk++;
    while (*regChunk != 0 && *regChunk != '\"')
    {
      dataSize++;
      regChunk++;
    }
    dataSize++;
  }
  else
  {
    UNIMPLEMENTED;
  }

  return  dataSize;
}

static BOOL
allocateDataBuffer (PVOID * data, int * dataBufferSize, int dataSize)
{
  if (*dataBufferSize < dataSize)
  {
    if (*dataBufferSize > 0)
      MmFreeMemory(*data);
    *data = MmAllocateMemory(dataSize);
    *dataBufferSize = dataSize;
  }

  return  TRUE;
}

static PCHAR
getKeyValueDataFromChunk (PCHAR  regChunk, PCHAR  dataFormat, PCHAR data)
{
  char  dataValue;
  U32 ulValue;
  PCHAR ptr;
  
  if (strcmp (dataFormat, "string") == 0)
  {
    /* convert quoted string to zero-terminated Unicode string */
    ptr = (PCHAR)data;
    regChunk++;
    while (*regChunk != 0 && *regChunk != '\"')
    {
      *ptr++ = (CHAR)*regChunk++;
    }
    *ptr = 0;
    regChunk++;
  }
  else if (strcmp (dataFormat, "hex") == 0)
  {
    while (*regChunk != 0 && isxdigit (*regChunk))
    {
      dataValue = (isdigit (*regChunk) ? *regChunk - '0' : 
        tolower(*regChunk) - 'a') << 4;
      regChunk++;
      dataValue += (isdigit (*regChunk) ? *regChunk - '0' : 
        tolower(*regChunk) - 'a');
      regChunk++;
      *data++ = dataValue;
      if (*regChunk == ',')
      {
        regChunk++;
        if (*regChunk == '\\')
        {
          regChunk++;
          regChunk = skipWhitespaceInChunk (regChunk);
        }
      }
    }
  }
  else if (strcmp (dataFormat, "dword") == 0)
  {
    ulValue = 0;
    while (*regChunk != 0 && isxdigit(*regChunk))
    {
      dataValue = (isdigit (*regChunk) ? *regChunk - '0' : 
        tolower(*regChunk) - 'a');
      ulValue = (ulValue << 4) + dataValue;
      regChunk++;
    }
    memcpy(data, &ulValue, sizeof(U32));
  }
  else if (strcmp (dataFormat, "multi") == 0)
  {
    ptr = (PCHAR)data;
    while (*regChunk == '\"')
    {
      regChunk++;
      while (*regChunk != 0 && *regChunk != '\"')
      {
        *ptr++ = (CHAR)*regChunk++;
      }
      regChunk++;
      *ptr++ = 0;
      if (*regChunk == ',')
      {
        regChunk++;
        regChunk = skipWhitespaceInChunk (regChunk);
        if (*regChunk == '\\')
        {
          regChunk++;
          regChunk = skipWhitespaceInChunk (regChunk);
        }
      }
      else
        break;
    }
    *ptr = 0;
  }
  else if (strcmp (dataFormat, "expand") == 0)
  {
    /* convert quoted string to zero-terminated Unicode string */
    ptr = (PCHAR)data;
    regChunk++;
    while (*regChunk != 0 && *regChunk != '\"')
    {
      *ptr++ = (CHAR)*regChunk++;
    }
    *ptr = 0;
    regChunk++;
  }
  else
  {
    UNIMPLEMENTED;
  }

  return  *regChunk ? regChunk : 0;
}

static BOOL
setKeyValue (HKEY currentKey,
             PCHAR newValueName,
             U32 keyValueType,
             PVOID data,
             U32 dataSize)
{
  S32 status;

  DbgPrint((DPRINT_REGISTRY, "Adding value (%s) to current key, with data type %d and size %d\n",
    newValueName, (int)keyValueType, (int)dataSize));

  status = RegSetValue(currentKey,
		       newValueName,
		       keyValueType,
		       data,
		       dataSize);
  if (status != ERROR_SUCCESS)
  {
    DbgPrint((DPRINT_REGISTRY, "Could not set key value. status: %d\n", status));
    return  FALSE;
  }

  return  TRUE;
}

VOID
RegImportHive(PCHAR ChunkBase,
	      U32 ChunkSize)
{
  HKEY  currentKey = INVALID_HANDLE_VALUE;
  char *newKeyName = NULL;
  int  newKeySize;
  char  dataFormat [10];
  int  keyValueType;
  int  dataSize = 0;
  int  dataBufferSize = 0;
  PVOID  data = 0;
  PCHAR regChunk;

  DbgPrint((DPRINT_REGISTRY, "ChunkBase %p  ChunkSize %lx\n", ChunkBase, ChunkSize));

  regChunk = checkAndSkipMagic (ChunkBase);
  if (regChunk == 0)
    return;

  while (regChunk != 0 && *regChunk != 0 && (((U32)regChunk-(U32)ChunkBase) < ChunkSize))
  {
    regChunk = skipWhitespaceInChunk (regChunk);
    if (regChunk == 0)
      continue;

    if (*regChunk == '[')
    {
      if (currentKey != INVALID_HANDLE_VALUE)
      {
        DbgPrint((DPRINT_REGISTRY, "Closing current key: 0x%lx\n", currentKey));
        currentKey = INVALID_HANDLE_VALUE;
      }

      regChunk++;

      newKeySize = computeKeyNameSize (regChunk);
      if (!allocateKeyName (&newKeyName, newKeySize))
      {
        regChunk = 0;
        continue;
      }

      regChunk = getKeyNameFromChunk (regChunk, newKeyName);
      if (regChunk == 0)
        continue;

      currentKey = createNewKey (newKeyName);
      if (currentKey == INVALID_HANDLE_VALUE)
      {
        regChunk = skipToNextKeyInChunk (regChunk);
        continue;
      }

      regChunk++;
    }
    else
    {
      if (currentKey == INVALID_HANDLE_VALUE)
      {
        regChunk = skipToNextKeyInChunk (regChunk);
        continue;
      }

      newKeySize = computeKeyValueNameSize (regChunk);
      if (!allocateKeyName (&newKeyName, newKeySize))
      {
        regChunk = 0;
        continue;
      }

      regChunk = getKeyValueNameFromChunk (regChunk, newKeyName);
      if (regChunk == 0)
        continue;

      if (*regChunk != '=')
      {
        regChunk = skipToNextKeyValueInChunk (regChunk);
        continue;
      }
      regChunk++;

      regChunk = getKeyValueTypeFromChunk (regChunk, dataFormat, &keyValueType);
      if (regChunk == 0)
        continue;

      dataSize = computeKeyValueDataSize (regChunk, dataFormat);
      if (!allocateDataBuffer (&data, &dataBufferSize, dataSize))
      {
        regChunk = 0;
        continue;
      }
      
      regChunk = getKeyValueDataFromChunk (regChunk, dataFormat, data);
      if (regChunk == 0)
        continue;

      if (!setKeyValue (currentKey, newKeyName, keyValueType, data, dataSize))
      {
        regChunk = 0;
        continue;
      }
    }
  }

  if (currentKey != INVALID_HANDLE_VALUE)
  {
    DbgPrint((DPRINT_REGISTRY, "Closing current key: 0x%lx\n", currentKey));
  }

  if (newKeyName != NULL)
  {
    MmFreeMemory(newKeyName);
  }

  if (data != NULL)
  {
    MmFreeMemory(data);
  }

  return;
}

BOOL
RegExportHive(PCHAR ChunkBase, U32* ChunkSize)
{
  return(TRUE);
}

/* EOF */
