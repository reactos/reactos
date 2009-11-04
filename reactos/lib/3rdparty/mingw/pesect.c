/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#include <windows.h>
#include <string.h>

#if defined (_WIN64) && defined (__ia64__)
#error FIXME: Unsupported __ImageBase implementation.
#else
/* Hack, for bug in ld.  Will be removed soon.  */
#define __ImageBase _image_base__
/* This symbol is defined by the linker.  */
extern IMAGE_DOS_HEADER __ImageBase;
#endif

BOOL _ValidateImageBase (PBYTE);

BOOL
_ValidateImageBase (PBYTE pImageBase)
{
  PIMAGE_DOS_HEADER pDOSHeader;
  PIMAGE_NT_HEADERS pNTHeader;
  PIMAGE_OPTIONAL_HEADER pOptHeader;

  pDOSHeader = (PIMAGE_DOS_HEADER) pImageBase;
  if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
    return FALSE;
  pNTHeader = (PIMAGE_NT_HEADERS) ((PBYTE) pDOSHeader + pDOSHeader->e_lfanew);
  if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
    return FALSE;
  pOptHeader = (PIMAGE_OPTIONAL_HEADER) &pNTHeader->OptionalHeader;
  if (pOptHeader->Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
    return FALSE;
  return TRUE;
}

PIMAGE_SECTION_HEADER _FindPESection (PBYTE, DWORD_PTR);

PIMAGE_SECTION_HEADER
_FindPESection (PBYTE pImageBase, DWORD_PTR rva)
{
  PIMAGE_NT_HEADERS pNTHeader;
  PIMAGE_SECTION_HEADER pSection;
  unsigned int iSection;

  pNTHeader = (PIMAGE_NT_HEADERS) (pImageBase + ((PIMAGE_DOS_HEADER) pImageBase)->e_lfanew);

  for (iSection = 0, pSection = IMAGE_FIRST_SECTION (pNTHeader);
    iSection < pNTHeader->FileHeader.NumberOfSections;
    ++iSection,++pSection)
    {
      if (rva >= pSection->VirtualAddress
	  && rva < pSection->VirtualAddress + pSection->Misc.VirtualSize)
	return pSection;
    }
  return NULL;
}

PIMAGE_SECTION_HEADER _FindPESectionByName (const char *);

PIMAGE_SECTION_HEADER
_FindPESectionByName (const char *pName)
{
  PBYTE pImageBase;
  PIMAGE_NT_HEADERS pNTHeader;
  PIMAGE_SECTION_HEADER pSection;
  unsigned int iSection;

  /* Long names aren't supported.  */
  if (strlen (pName) > IMAGE_SIZEOF_SHORT_NAME)
    return NULL;

  pImageBase = (PBYTE) &__ImageBase;
  if (! _ValidateImageBase (pImageBase))
    return NULL;

  pNTHeader = (PIMAGE_NT_HEADERS) (pImageBase + ((PIMAGE_DOS_HEADER) pImageBase)->e_lfanew);

  for (iSection = 0, pSection = IMAGE_FIRST_SECTION (pNTHeader);
    iSection < pNTHeader->FileHeader.NumberOfSections;
    ++iSection,++pSection)
    {
      if (!strncmp ((char *) &pSection->Name[0], pName, IMAGE_SIZEOF_SHORT_NAME))
	return pSection;
    }
  return NULL;
}

PIMAGE_SECTION_HEADER _FindPESectionExec (size_t);

PIMAGE_SECTION_HEADER
_FindPESectionExec (size_t eNo)
{
  PBYTE pImageBase;
  PIMAGE_NT_HEADERS pNTHeader;
  PIMAGE_SECTION_HEADER pSection;
  unsigned int iSection;

  pImageBase = (PBYTE) &__ImageBase;
  if (! _ValidateImageBase (pImageBase))
    return NULL;

  pNTHeader = (PIMAGE_NT_HEADERS) (pImageBase + ((PIMAGE_DOS_HEADER) pImageBase)->e_lfanew);

  for (iSection = 0, pSection = IMAGE_FIRST_SECTION (pNTHeader);
    iSection < pNTHeader->FileHeader.NumberOfSections;
    ++iSection,++pSection)
    {
      if ((pSection->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0)
      {
	if (!eNo)
	  return pSection;
	--eNo;
      }
    }
  return NULL;
}

PBYTE _GetPEImageBase (void);

PBYTE
_GetPEImageBase (void)
{
  PBYTE pImageBase;
  pImageBase = (PBYTE) &__ImageBase;
  if (! _ValidateImageBase (pImageBase))
    return NULL;
  return pImageBase;
}

BOOL _IsNonwritableInCurrentImage (PBYTE);

BOOL
_IsNonwritableInCurrentImage (PBYTE pTarget)
{
  PBYTE pImageBase;
  DWORD_PTR rvaTarget;
  PIMAGE_SECTION_HEADER pSection;

  pImageBase = (PBYTE) &__ImageBase;
  if (! _ValidateImageBase (pImageBase))
    return FALSE;
  rvaTarget = pTarget - pImageBase;
  pSection = _FindPESection (pImageBase, rvaTarget);
  if (pSection == NULL)
    return FALSE;
  return (pSection->Characteristics & IMAGE_SCN_MEM_WRITE) == 0;
}
