
#ifndef __INTERNAL_SERVICE_H
#define __INTERNAL_SERVICE_H


typedef struct _SERVICE_TABLE
{
   unsigned long  ParametersSize;
   unsigned long  Function;
} SERVICE_TABLE, *PSERVICE_TABLE;

#pragma pack(1)

// System Service Dispatch Table
typedef struct t_SSDT {
                ULONG           SysCallPtr;
} SSDT, *PSSDT;

// System Service Parameters Table
typedef struct t_SSPT   {
                unsigned int    ParamBytes;
} SSPT, *PSSPT;

typedef struct t_KeServiceDescriptorTableEntry {
                PSSDT               pSSDT;
                unsigned long*      ServiceCounterTable;
                unsigned int        NumberOfServices;
                PSSPT               pSSPT;

} KE_SERVICE_DESCRIPTOR_TABLE_ENTRY, *PKE_SERVICE_DESCRIPTOR_TABLE_ENTRY;

#pragma pack()

NTSTATUS KeAddSystemServiceTable( int SSDTindex, PSSDT pSSDT, PSSPT pSSPT, ULONG* SyscallsCountTable );

#endif

