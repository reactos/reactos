 typedef struct _MDL
/*
 * PURPOSE: Describes a user buffer passed to a system API
 */
{
   struct _MDL* Next;
   CSHORT Size;
   CSHORT MdlFlags;
   struct _EPROCESS* Process;
   PVOID MappedSystemVa;
   PVOID StartVa;
   ULONG ByteCount;
   ULONG ByteOffset;
} MDL, *PMDL;
