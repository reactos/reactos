typedef struct _MDL
/*
 * PURPOSE: Describes a user buffer passed to a system API
 */
{
   /*
    * Base address of the buffer in user mode
    */
   PVOID Base;
   
   /*
    * Length of the buffer in bytes
    */
   ULONG Length;
   
   /*
    * System address of buffer or NULL if not mapped
    */
   PVOID SysBase;
   
   /*
    * Below this is a variable length list of page physical address
    */
} MDL, *PMDL;
