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

#include "registry.h"

#define NDEBUG


#define  REGISTRY_FILE_MAGIC    "REGEDIT4"

#define INVALID_HANDLE_VALUE    NULL


static PCHAR 
checkAndSkipMagic (PCHAR  regChunk)
{
  if (strncmp (regChunk, 
               REGISTRY_FILE_MAGIC, 
               strlen (REGISTRY_FILE_MAGIC)) != 0)
  {
#ifndef NDEBUG
    printf("incorrect magic number in registry chunk. expected: %s got:%.*s\n",
           REGISTRY_FILE_MAGIC,
           strlen (REGISTRY_FILE_MAGIC),
           regChunk);
#endif
    return  0;
  }
  regChunk += strlen (REGISTRY_FILE_MAGIC);
#ifndef NDEBUG
  printf("Found registry chunk magic value\n");
#endif

  return  regChunk;
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
    FreeMemory(*newKeyName);

  *newKeyName = AllocateMemory(newKeySize + 1);
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

#ifndef NDEBUG
  printf("Adding new key '%s'\n", newKeyName);
#endif

  RegCreateKey(NULL,
	       newKeyName,
	       &handleToReturn);

#ifndef NDEBUG
  printf("  returned handle: 0x%x\n", handleToReturn);
#endif

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
  else
  {
//    UNIMPLEMENTED;
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
    dataSize = sizeof(DWORD);
    while (*regChunk != 0 && isxdigit(*regChunk))
    {
      regChunk++;
    }
  }
  else
  {
//    UNIMPLEMENTED;
  }

  return  dataSize;
}

static BOOL
allocateDataBuffer (PVOID * data, int * dataBufferSize, int dataSize)
{
  if (*dataBufferSize < dataSize)
  {
    if (*dataBufferSize > 0)
      FreeMemory(*data);
    *data = AllocateMemory(dataSize);
    *dataBufferSize = dataSize;
  }

  return  TRUE;
}

static PCHAR
getKeyValueDataFromChunk (PCHAR  regChunk, PCHAR  dataFormat, PCHAR data)
{
  char  dataValue;
  ULONG ulValue;
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
    memcpy(data, &ulValue, sizeof(ULONG));
  }
  else
  {
//    UNIMPLEMENTED;
  }

  return  *regChunk ? regChunk : 0;
}

static BOOL
setKeyValue (HKEY currentKey,
             PCHAR newValueName,
             ULONG keyValueType,
             PVOID data,
             ULONG dataSize)
{
  LONG status;

#ifndef NDEBUG
  printf("Adding value (%s) to current key, with data type %d size %d\n",
         newValueName,
         (int)keyValueType,
         (int)dataSize);
#endif
  status = RegSetValue(currentKey,
		       newValueName,
		       keyValueType,
		       data,
		       dataSize);
  if (status != ERROR_SUCCESS)
  {
#ifndef NDEBUG
    printf("could not set key value, rc:%d\n", status);
#endif
    return  FALSE;
  }

  return  TRUE;
}


VOID
RegImportHive(PCHAR ChunkBase,
	      ULONG ChunkSize)
{
  HKEY currentKey = NULL;
  int  newKeySize = 0;
  char *newKeyName = NULL;
  char  dataFormat [10];
  int  keyValueType;
  int  dataSize = 0;
  int  dataBufferSize = 0;
  PVOID  data = 0;
  PCHAR regChunk;

#ifndef NDEBUG
  printf("ChunkBase 0x%x  ChunkSize %x\n", ChunkBase, ChunkSize);
#endif

  regChunk = checkAndSkipMagic (ChunkBase);
  if (regChunk == 0)
    return;

  while (regChunk != 0 && *regChunk != 0 && (((ULONG)regChunk-(ULONG)ChunkBase) < ChunkSize))
  {
    regChunk = skipWhitespaceInChunk (regChunk);
    if (regChunk == 0)
      continue;

    if (*regChunk == '[')
    {
#ifndef NDEBUG
    printf("Line: %s\n", regChunk);
#endif
      if (currentKey != NULL)
      {
#ifndef NDEBUG
        printf("Closing current key: 0x%lx\n", currentKey);
#endif
        currentKey = NULL;
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
      if (currentKey == NULL)
      {
        regChunk = skipToNextKeyInChunk (regChunk);
        continue;
      }

      regChunk++;
    }
    else
    {
      if (currentKey == NULL)
      {
        regChunk = skipToNextKeyInChunk (regChunk);
        continue;
      }

      newKeySize = computeKeyValueNameSize(regChunk);
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

  if (newKeyName != NULL)
  {
    FreeMemory(newKeyName);
  }
  if (data != NULL)
  {
    FreeMemory(data);
  }

  return;
}




static PCHAR
bprintf(char *buffer, char *format, ... )
{
  int *dataptr = (int *) &format;
  char c, *ptr, str[16];
  char *p = buffer;

  dataptr++;

  while ((c = *(format++)))
    {
      if (c != '%')
      {
	*p = c;
	p++;
      }
      else
	switch (c = *(format++))
	  {
	  case 'd': case 'u': case 'x':
	    *convert_to_ascii(str, c, *((unsigned long *) dataptr++)) = 0;

	    ptr = str;

	    while (*ptr)
	    {
	      *p = *(ptr++);
	      p++;
	    }
	    break;

	  case 'c':
	    *p = (*(dataptr++))&0xff;
	    p++;
	    break;

	  case 's':
	    ptr = (char *)(*(dataptr++));

	    while ((c = *(ptr++)))
	    {
	      *p = c;
	      p++;
	    }
	    break;
	  }
    }
  return(p);
}


BOOL
RegExportHive(PCHAR ChunkBase, PULONG ChunkSize)
{

  return(TRUE);
}

/* EOF */
