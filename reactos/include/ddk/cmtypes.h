#ifndef __INCLUDE_DDK_CMTYPES_H
#define __INCLUDE_DDK_CMTYPES_H
/*
 * Object Manager structures and typedefs
 */

/*
 * key query information class 
 */

typedef enum _KEY_INFORMATION_CLASS
{
  KeyBasicInformation,
  KeyNodeInformation,
  KeyFullInformation
} KEY_INFORMATION_CLASS;

typedef struct _KEY_BASIC_INFORMATION
{
  LARGE_INTEGER  LastWriteTime;
  ULONG  TitleIndex;
  ULONG  NameLength;
  WCHAR  Name[1];
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_FULL_INFORMATION
{
  LARGE_INTEGER  LastWriteTime;
  ULONG  TitleIndex;
  ULONG  ClassOffset;
  ULONG  ClassLength;
  ULONG  SubKeys;
  ULONG  MaxNameLen;
  ULONG  MaxClassLen;
  ULONG  Values;
  ULONG  MaxValueNameLen;
  ULONG  MaxValueDataLen;
  WCHAR  Class[1];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef struct _KEY_NODE_INFORMATION
{
  LARGE_INTEGER  LastWriteTime;
  ULONG  TitleIndex;
  ULONG  ClassOffset;
  ULONG  ClassLength;
  ULONG  NameLength;
  WCHAR  Name[1];
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

/* key set information class */
/*
 * KeyWriteTimeInformation
 */

/* key value information class */

typedef enum _KEY_VALUE_INFORMATION_CLASS
{
  KeyValueBasicInformation,
  KeyValueFullInformation,
  KeyValuePartialInformation
} KEY_VALUE_INFORMATION_CLASS;

typedef struct _KEY_VALUE_BASIC_INFORMATION
{
  ULONG  TitleIndex;
  ULONG  Type;
  ULONG  NameLength;
  WCHAR  Name[1];
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION
{
  ULONG  TitleIndex;
  ULONG  Type;
  ULONG  DataOffset;
  ULONG  DataLength;
  ULONG  NameLength;
  WCHAR  Name[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION
{
  ULONG  TitleIndex;
  ULONG  Type;
  ULONG  DataLength;
  UCHAR  Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;


/* used by [Nt/Zw]QueryMultipleValueKey */

typedef struct _KEY_VALUE_ENTRY
{
  PUNICODE_STRING  ValueName;
  ULONG  DataLength;
  ULONG  DataOffset;
  ULONG  Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

#endif /* __INCLUDE_DDK_CMTYPES_H */
