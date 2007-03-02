/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/elf32.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     No programmer listed.
 */
#include <ntoskrnl.h>
#define __ELF_WORD_SIZE 32
#include "elf.inc.h"

extern NTSTATUS NTAPI Elf64FmtCreateSection
(
 IN CONST VOID * FileHeader,
 IN SIZE_T FileHeaderSize,
 IN PVOID File,
 OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 OUT PULONG Flags,
 IN PEXEFMT_CB_READ_FILE ReadFileCb,
 IN PEXEFMT_CB_ALLOCATE_SEGMENTS AllocateSegmentsCb
);

NTSTATUS NTAPI ElfFmtCreateSection
(
 IN CONST VOID * FileHeader,
 IN SIZE_T FileHeaderSize,
 IN PVOID File,
 OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 OUT PULONG Flags,
 IN PEXEFMT_CB_READ_FILE ReadFileCb,
 IN PEXEFMT_CB_ALLOCATE_SEGMENTS AllocateSegmentsCb
)
{
 ULONG nDataType;
 const Elf32_Ehdr * pehTempHeader;

 ASSERT(FileHeader);
 ASSERT(FileHeaderSize > 0);
 ASSERT(Intsafe_CanOffsetPointer(FileHeader, FileHeaderSize));
 ASSERT(File);
 ASSERT(ImageSectionObject);
 ASSERT(Flags);
 ASSERT(ReadFileCb);
 ASSERT(AllocateSegmentsCb);

 pehTempHeader = FileHeader;
 ASSERT(((ULONG_PTR)pehTempHeader % TYPE_ALIGNMENT(Elf32_Ehdr)) == 0);
 ASSERT(((ULONG_PTR)pehTempHeader % TYPE_ALIGNMENT(Elf64_Ehdr)) == 0);

 ASSERT(ELFFMT_FIELDS_EQUAL(Elf32_Ehdr, Elf64_Ehdr, e_ident));

 /* File too small to be identified */
 if(!RTL_CONTAINS_FIELD(pehTempHeader, FileHeaderSize, e_ident[EI_MAG3]))
  return STATUS_ROS_EXEFMT_UNKNOWN_FORMAT;

 /* Not an ELF file */
 if
 (
  pehTempHeader->e_ident[EI_MAG0] != ELFMAG0 ||
  pehTempHeader->e_ident[EI_MAG1] != ELFMAG1 ||
  pehTempHeader->e_ident[EI_MAG2] != ELFMAG2 ||
  pehTempHeader->e_ident[EI_MAG3] != ELFMAG3
 )
  return STATUS_ROS_EXEFMT_UNKNOWN_FORMAT;

 /* Validate the data type */
 nDataType = pehTempHeader->e_ident[EI_DATA];

 switch(nDataType)
 {
  case ELFDATA2LSB:
  case ELFDATA2MSB:
   break;

  default:
   return STATUS_INVALID_IMAGE_FORMAT;
 }

 /* Validate the version */
 ASSERT(ELFFMT_FIELDS_EQUAL(Elf32_Ehdr, Elf64_Ehdr, e_version));

 if
 (
  pehTempHeader->e_ident[EI_VERSION] != EV_CURRENT ||
  ElfFmtpReadULong(pehTempHeader->e_version, nDataType) != EV_CURRENT
 )
  return STATUS_INVALID_IMAGE_FORMAT;

 /* Validate the file type */
 ASSERT(ELFFMT_FIELDS_EQUAL(Elf32_Ehdr, Elf64_Ehdr, e_type));

 switch(ElfFmtpReadUShort(pehTempHeader->e_type, nDataType))
 {
  case ET_DYN: ImageSectionObject->ImageCharacteristics |= IMAGE_FILE_DLL;
  case ET_EXEC: break;
  default: return STATUS_INVALID_IMAGE_FORMAT;
 }

 /* Convert the target machine */
 ASSERT(ELFFMT_FIELDS_EQUAL(Elf32_Ehdr, Elf64_Ehdr, e_machine));
 ASSERT(ImageSectionObject->Machine == IMAGE_FILE_MACHINE_UNKNOWN);

 switch(ElfFmtpReadUShort(pehTempHeader->e_machine, nDataType))
 {
  case EM_386: ImageSectionObject->Machine = IMAGE_FILE_MACHINE_I386; break;
  case EM_MIPS_RS3_LE: ImageSectionObject->Machine = IMAGE_FILE_MACHINE_R3000; break;

#if 0
  /* TODO: need to read e_flags for full identification */
  case EM_SH: break;
#endif

  case EM_ARM: ImageSectionObject->Machine = IMAGE_FILE_MACHINE_ARM; break;
  case EM_PPC: ImageSectionObject->Machine = IMAGE_FILE_MACHINE_POWERPC; break;
  case EM_IA_64: ImageSectionObject->Machine = IMAGE_FILE_MACHINE_IA64; break;
  case EM_ALPHA: ImageSectionObject->Machine = IMAGE_FILE_MACHINE_AXP64; break;
  case EM_X86_64: ImageSectionObject->Machine = IMAGE_FILE_MACHINE_AMD64; break;
  case EM_M32R: ImageSectionObject->Machine = IMAGE_FILE_MACHINE_M32R; break;
 }

 /* Call the appropriate handler for the class-specific fields */
 switch(pehTempHeader->e_ident[EI_CLASS])
 {
  case ELFCLASS32:
   return Elf32FmtCreateSection
   (
    FileHeader,
    FileHeaderSize,
    File,
    ImageSectionObject,
    Flags,
    ReadFileCb,
    AllocateSegmentsCb
   );

  case ELFCLASS64:
   return Elf64FmtCreateSection
   (
    FileHeader,
    FileHeaderSize,
    File,
    ImageSectionObject,
    Flags,
    ReadFileCb,
    AllocateSegmentsCb
   );
 }

 /* Unknown class */
 return STATUS_INVALID_IMAGE_FORMAT;
}

/* EOF */
