/* $Id: import.c,v 1.11 2003/03/22 18:26:53 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cm/import.c
 * PURPOSE:         Registry functions
 * PROGRAMMERS:     Rex Jolliff
 */

#ifdef WIN32_REGDBG
#include "cm_win32.h"
#else
#include <ctype.h>

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/ob.h>
#include <limits.h>
#include <string.h>
#include <internal/pool.h>
#include <internal/registry.h>
#include <internal/ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

#include "cm.h"
#endif

static PCHAR 
checkAndSkipMagic (PCHAR  regChunk)
{
  if (strncmp (regChunk, 
               REGISTRY_FILE_MAGIC, 
               strlen (REGISTRY_FILE_MAGIC)) != 0)
  {
    CPRINT ("incorrect magic number in registry chunk. expected: %s got:%.*s\n",
            REGISTRY_FILE_MAGIC,
            strlen (REGISTRY_FILE_MAGIC),
            regChunk);
    return  0;
  }
  regChunk += strlen (REGISTRY_FILE_MAGIC);
  DPRINT ("Found regsitry chunk magic value\n");

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

static BOOLEAN
allocateKeyName (PUNICODE_STRING  newKeyName, int  newKeySize)
{
  if (newKeyName->MaximumLength < (newKeySize + 1) * sizeof (WCHAR))
  {
    if (newKeyName->Buffer != 0)
      ExFreePool (newKeyName->Buffer);
    newKeyName->Length = 0;
    newKeyName->MaximumLength = (newKeySize + 1) * sizeof (WCHAR);
    newKeyName->Buffer = ExAllocatePool (NonPagedPool, newKeyName->MaximumLength);
    if (newKeyName->Buffer == 0)
    {
      CPRINT ("Could not allocate space for key name\n");
      return  FALSE;
    }
    newKeyName->Buffer [0] = 0;
  }
  else
  {
    newKeyName->Length = 0;
    newKeyName->Buffer [0] = 0;
  }

  return  TRUE;
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
getKeyNameFromChunk (PCHAR  regChunk, PUNICODE_STRING  newKeyName)
{
  int index = 0;

  while (*regChunk != 0 && *regChunk != ']')
  {
    newKeyName->Buffer [index++] = *regChunk++;
  }
  newKeyName->Buffer [index] = '\0';
  newKeyName->Length = index * sizeof (WCHAR);

  return  *regChunk ? regChunk : 0;
}

static HANDLE
createNewKey (PUNICODE_STRING  newKeyName)
{
  NTSTATUS  status;
  OBJECT_ATTRIBUTES  attributes;
  HANDLE  handleToReturn;

  DPRINT ("Creating key (%wZ)\n", newKeyName);
  InitializeObjectAttributes (&attributes,
                              newKeyName,
                              0,
                              0,
                              NULL);
  status = NtCreateKey (&handleToReturn,
                        KEY_ALL_ACCESS,
                        &attributes,
                        0,
                        NULL,
                        REG_OPTION_VOLATILE,
                        NULL);
  if (!NT_SUCCESS(status))
  {
    CPRINT ("Could not crete key (%wZ)\n", newKeyName);
    return  INVALID_HANDLE_VALUE;
  }

  return  handleToReturn;
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
getKeyValueNameFromChunk (PCHAR  regChunk, PUNICODE_STRING  newKeyName)
{
  int  index = 0;

  regChunk++;
  while (*regChunk != 0 && *regChunk != '\"')
  {
    newKeyName->Buffer [index++] = *regChunk++;
  }
  newKeyName->Buffer [index] = '\0';
  newKeyName->Length = index * sizeof (WCHAR);
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
      dataSize++;
      regChunk++;
    }
    dataSize++;
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
  else if (strcmp (dataFormat, "multi") == 0)
  {
    while (*regChunk == '\"')
    {
      regChunk++;
      while (*regChunk != 0 && *regChunk != '\"')
      {
        dataSize++;
        dataSize++;
        regChunk++;
      }
      regChunk++;
      dataSize++;
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
    dataSize++;
  }
  else if (strcmp (dataFormat, "expand") == 0)
  {
    regChunk++;
    while (*regChunk != 0 && *regChunk != '\"')
    {
      dataSize++;
      dataSize++;
      regChunk++;
    }
    dataSize++;
    dataSize++;
  }
  else
  {
    UNIMPLEMENTED;
  }

  return  dataSize;
}

static BOOLEAN
allocateDataBuffer (PVOID * data, int * dataBufferSize, int dataSize)
{
  if (*dataBufferSize < dataSize)
  {
    if (*dataBufferSize > 0)
      ExFreePool (*data);
    *data = ExAllocatePool (NonPagedPool, dataSize);
    *dataBufferSize = dataSize;
  }

  return  TRUE;
}

static PCHAR
getKeyValueDataFromChunk (PCHAR  regChunk, PCHAR  dataFormat, PCHAR data)
{
  char  dataValue;
  ULONG ulValue;
  PWCHAR ptr;
  
  if (strcmp (dataFormat, "string") == 0)
  {
    /* convert quoted string to zero-terminated Unicode string */
    ptr = (PWCHAR)data;
    regChunk++;
    while (*regChunk != 0 && *regChunk != '\"')
    {
      *ptr++ = (WCHAR)*regChunk++;
    }
    *ptr = 0;
    regChunk++;
  }
  else if (strcmp (dataFormat, "hex") == 0)
  {
    while (*regChunk != 0 && isxdigit (*regChunk))
    {
      dataValue = (isdigit (*regChunk) ? *regChunk - '0' : 
        tolower(*regChunk) - 'a' + 10) << 4;
      regChunk++;
      dataValue += (isdigit (*regChunk) ? *regChunk - '0' : 
        tolower(*regChunk) - 'a' + 10);
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
        tolower(*regChunk) - 'a' + 10);
      ulValue = (ulValue << 4) + dataValue;
      regChunk++;
    }
    memcpy(data, &ulValue, sizeof(ULONG));
  }
  else if (strcmp (dataFormat, "multi") == 0)
  {
    ptr = (PWCHAR)data;
    while (*regChunk == '\"')
    {
      regChunk++;
      while (*regChunk != 0 && *regChunk != '\"')
      {
        *ptr++ = (WCHAR)*regChunk++;
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
    ptr = (PWCHAR)data;
    regChunk++;
    while (*regChunk != 0 && *regChunk != '\"')
    {
      *ptr++ = (WCHAR)*regChunk++;
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

static BOOLEAN
setKeyValue (HANDLE  currentKey,
             PUNICODE_STRING  newValueName,
             ULONG  keyValueType,
             PVOID  data,
             ULONG  dataSize)
{
  NTSTATUS  status;

  DPRINT ("Adding value (%wZ) to current key, with data type %d size %d\n",
          newValueName,
          keyValueType,
          dataSize);
  status = NtSetValueKey (currentKey,
                          newValueName,
                          0,
                          keyValueType,
                          data,
                          dataSize);
  if (!NT_SUCCESS(status))
  {
    CPRINT ("could not set key value, rc:%08x\n", status);
    return  FALSE;
  }

  return  TRUE;
}

VOID
CmImportTextHive(PCHAR  ChunkBase,
		 ULONG  ChunkSize)
{
  HANDLE  currentKey = INVALID_HANDLE_VALUE;
  int  newKeySize;
  UNICODE_STRING  newKeyName = {0, 0, 0};
  char  dataFormat [10];
  int  keyValueType;
  int  dataSize = 0;
  int  dataBufferSize = 0;
  PVOID  data = 0;
  PCHAR regChunk;

  DPRINT("ChunkBase %p  ChunkSize %lx\n", ChunkBase, ChunkSize);

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
      if (currentKey != INVALID_HANDLE_VALUE)
      {
        DPRINT("Closing current key: 0x%lx\n", currentKey);
        NtClose (currentKey);
        currentKey = INVALID_HANDLE_VALUE;
      }

      regChunk++;

      newKeySize = computeKeyNameSize (regChunk);
      if (!allocateKeyName (&newKeyName, newKeySize))
      {
        regChunk = 0;
        continue;
      }

      regChunk = getKeyNameFromChunk (regChunk, &newKeyName);
      if (regChunk == 0)
        continue;

      currentKey = createNewKey (&newKeyName);
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

      regChunk = getKeyValueNameFromChunk (regChunk, &newKeyName);
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

      if (!setKeyValue (currentKey, &newKeyName, keyValueType, data, dataSize))
      {
        regChunk = 0;
        continue;
      }
    }
  }

  if (currentKey != INVALID_HANDLE_VALUE)
  {
    NtClose (currentKey);
  }
  if (newKeyName.Buffer != 0)
  {
    ExFreePool (newKeyName.Buffer);
  }
  if (data != 0)
  {
    ExFreePool (data);
  }

  return;
}


VOID
CmImportSystemHive(PCHAR ChunkBase,
		   ULONG ChunkSize)
{
  if (strncmp (ChunkBase, "REGEDIT4", 8) == 0)
    {
      DPRINT("Found 'REGEDIT4' magic\n");
      CmImportTextHive (ChunkBase, ChunkSize);
    }
  else
    {
      DPRINT1("Found '%*s' magic\n", 4, ChunkBase);
      KeBugCheck(0);
    }
}


VOID
CmImportHardwareHive(PCHAR ChunkBase,
		     ULONG ChunkSize)
{
  DPRINT1("CmImportHardwareHive() called\n");

}

/* EOF */
