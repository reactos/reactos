#ifndef __INCLUDE_NAPI_CORE_H
#define __INCLUDE_NAPI_CORE_H

#define MM_CORE_DUMP_HEADER_MAGIC         (0xdeafbead)
#define MM_CORE_DUMP_HEADER_VERSION       (0x1)

typedef struct _MM_CORE_DUMP_HEADER
{
   ULONG Magic;
   ULONG Version;
   CONTEXT Context;
   ULONG DumpLength;
   ULONG BugCode;
   ULONG ExceptionCode;
   ULONG Cr2;
   ULONG Cr3;
} MM_CORE_DUMP_HEADER, *PMM_CORE_DUMP_HEADER;

#endif /* __INCLUDE_NAPI_CORE_H */
