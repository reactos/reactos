/*
 * COPYRIGHT:      See copying in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           include/pe.h
 * PURPOSE:        Contains portable executable format definitions
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 26/05/98:    Created
 */

/* NOTES
 * 
 * This is taken from the Tool Interface Standard (TIS) Formats Specification 
 * for Windows 
 */

#ifndef __PE_H
#define __PE_H

/*
 * CPU types
 */
enum
{
   CPU_UNKNOWN,
   CPU_I386 = 0x14c,
   CPU_I486 = 0x14d,
   CPU_PENTIUM = 0x14e,
   CPU_MIPS_I = 0x162,
   CPU_MIPS_II = 0x163,
   CPU_MIPS_III = 0x166,
};

/*
 * Image flags
 */
enum
{
   IMG_PROGRAM = 0,
   IMG_EXECUTABLE = 0,
   IMG_FIXED = 0x200,
   IMG_LIBRARY = 0x2000,
};

/*
 * Subsystems
 */
enum
{
   SUBSYS_UNKNOWN,
   SUBSYS_NATIVE,
   SUBSYS_WINDOWS_GUI,
   SUBSYS_WINDOWS_CHARACTER,
   SUBSYS_OS2_CHARACTER,
   SUBSYS_POSIX_CHARACTER,
};

enum
{
   DLL_PERPROCESS_LIB_INITIALIZATION,
   DLL_PERPROCESS_LIB_TERMINATION,
   DLL_PERTHREAD_LIB_INITIALIZATION,
   DLL_PERTHREAD_LIB_TERMINATION,
};

enum
{
   OBJ_CODE = 0x20,
   OBJ_INITIALIZED_DATA = 0x40,
   OBJ_UNINITIALIZED_DATA = 0x80,
   OBJ_NOCACHE = 0x40000000,
   OBJ_NONPAGEABLE = 0x80000000,
   OBJ_SHARED = 0x10000000,
   OBJ_EXECUTABLE = 0x20000000,
   OBJ_READABLE = 0x40000000,
   OBJ_WRITEABLE = 0x80000000
};

typedef struct
{
   /*
    * PURPOSE: Signature, current value is 'PE'
    */
   unsigned char signature[4];
   
   /*
    * PURPOSE: CPU required for the image to run
    */
   unsigned short int cpu;
   
   /*
    * PURPOSE: Number of entries in the object table
    */
   unsigned short int nr_objects;
   
   /*
    * PURPOSE: Time and date when image was created
    */
   unsigned long int time_date_stamp;
   
   unsigned short int reserved1;
   unsigned short int reserved2;
   
   /*
    * PURPOSE: Number of bytes in the header following the flags field
    */
   unsigned short int nt_hdr_size;
   
   /*
    * PURPOSE: Image flags
    */
   unsigned short int flags;
   
   unsigned char reserved3;
   
   /*
    * PURPOSE: Major/minor version number of the linker
    */
   unsigned char linker_major_ver;
   unsigned char linker_minor_ver;
   
   unsigned char reserved4;
   unsigned long int reserved5;
   unsigned long int reserved6;
   
   /*
    * PURPOSE: Entry point relative to the image base
    */
   unsigned long int entry;
   
   unsigned long int reserved7;
   unsigned long int reserved8;
   
   /*
    * PURPOSE: Virtual base of image. This will be the virtual address of
    * the first byte of file. It must be a multiple of 64k
    */
   unsigned long int image_base;
   
   /*
    * PURPOSE: Object alignment factor
    */
   unsigned long int object_align;
   
   /*
    * PURPOSE: Alignment factor used to align image pages
    */
   unsigned long int file_align;
   
   /*
    * PURPOSE: OS version required to run this image
    */
   unsigned short int os_version;
   
   /*
    * PURPOSE: User specified version number
    */
   unsigned short int user_version;
   
   /*
    * PURPOSE: Subsystem version required
    */
   unsigned short int subsys_version;
   
   unsigned long int reserved9;
   
   /*
    * PURPOSE: Size of the image in bytes (including all headers)
    */
   unsigned long int image_size;
   
   /*
    * PURPOSE: Header size (Total of DOS header, PE header and object table)
    */
   unsigned long int hdr_size;
   
   /*
    * PURPOSE: Checksum for entire file
    */
   unsigned long int checksum;
   
   /*
    * PURPOSE: Subsystem required to run this image
    */
   unsigned short int subsystem;
   
   /*
    * PURPOSE: Special loader requirements 
    */
   unsigned short int dll_flags;
   
   /*
    * PURPOSE: Stack size needed for image
    */
   unsigned long int stack_reserve_size;
   
   /*
    * PURPOSE: Stack size to be committed
    */
   unsigned long int stack_commit_size;
   
   /*
    * PURPOSE: Size of local heap to reserve
    */
   unsigned long int heap_reserve_size;
   
   /*
    * PURPOSE: Size of local heap to commit
    */
   unsigned long int heap_commit_size;
   
   unsigned long int reserved10;
   
   /*
    * PURPOSE: Size of the array that follows
    */
   unsigned long int nr_rvas;
   
   
   unsigned long int export_table_rva;
   unsigned long int export_table_size;
   unsigned long int import_table_rva;
   unsigned long int import_table_size;
   unsigned long int resource_tlb_rva;
   unsigned long int resource_tlb_size;
   unsigned long int exception_table_rva;
   unsigned long int exception_table_size;
   unsigned long int security_table_rva;
   unsigned long int security_table_size;
   unsigned long int fixup_table_rva;
   unsigned long int fixup_table_size;
   unsigned long int debug_table_rva;
   unsigned long int debug_table_size;
   unsigned long int image_description_rva;
   unsigned long int image_description_size;
   unsigned long int mach_specific_rva;
   unsigned long int mach_specific_size;
   unsigned long int tls_rva;
   unsigned long int tls_size;
} PE_header;

typedef struct
/*
 * PURPOSE: Defines an entry in the image object table
 */
{
   /*
    * PURPOSE: Name of object
    */
   unsigned char name[8];
   
   /*
    * PURPOSE: Size to allocate in memory
    */
   unsigned long int virtual_size;
   
   /*
    * PURPOSE: Base address of object relative to the image base
    */
   unsigned long int rva;
   
   /*
    * PURPOSE: Size of initialized data
    */
   unsigned long int physical_size;
   
   /*
    * PURPOSE: Offset relative to the beginning of the image of the 
    * data for the object
    */
   unsigned long int physical_offset;
   
   unsigned long int reserved[3];
   
   /*
    * PURPOSE: Object flags
    */
   unsigned long int object_flags;
} PE_ObjectTableEntry;

#endif /* __PE_H */
