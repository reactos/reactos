/* $Id$
 */

#ifndef ROSRTL_STRING_H__
#define ROSRTL_STRING_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define RosInitializeString( \
 __PDEST_STRING__, \
 __LENGTH__, \
 __MAXLENGTH__, \
 __BUFFER__ \
) \
{ \
 (__PDEST_STRING__)->Length = (__LENGTH__); \
 (__PDEST_STRING__)->MaximumLength = (__MAXLENGTH__); \
 (__PDEST_STRING__)->Buffer = (__BUFFER__); \
}

#define RtlRosInitStringFromLiteral( \
 __PDEST_STRING__, __SOURCE_STRING__) \
 RosInitializeString( \
  (__PDEST_STRING__), \
  sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
  sizeof(__SOURCE_STRING__), \
  (__SOURCE_STRING__) \
 )
 
#define RtlRosInitUnicodeStringFromLiteral \
 RtlRosInitStringFromLiteral

#define ROS_STRING_INITIALIZER(__SOURCE_STRING__) \
{ \
 sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
 sizeof(__SOURCE_STRING__), \
 (__SOURCE_STRING__) \
}

#define ROS_EMPTY_STRING {0, 0, NULL}

NTSTATUS
FASTCALL
RtlpOemStringToCountedUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN POEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpUpcaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpUpcaseUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpUpcaseUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpUpcaseUnicodeStringToOemString (
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS 
FASTCALL
RtlpDowncaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS
FASTCALL
RtlpAnsiStringToUnicodeString(
   IN OUT PUNICODE_STRING DestinationString,
   IN PANSI_STRING SourceString,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);   
   
NTSTATUS
FASTCALL
RtlpUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
NTSTATUS
FASTCALL
RtlpOemStringToUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN POEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);

NTSTATUS
FASTCALL
RtlpUnicodeStringToOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType);
   
BOOLEAN
FASTCALL
RtlpCreateUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCWSTR  Source,
   IN POOL_TYPE PoolType);   

NTSTATUS
FASTCALL
RtlpUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType);

NTSTATUS STDCALL
RtlpDuplicateUnicodeString(
   INT AddNull,
   IN PUNICODE_STRING SourceString,
   PUNICODE_STRING DestinationString,
   POOL_TYPE PoolType);

NTSTATUS NTAPI RosAppendUnicodeString( PUNICODE_STRING ResultFirst,
				       PUNICODE_STRING Second,
				       BOOL Deallocate );

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
