#include <windows.h>

#if defined (_WIN64) && defined (__ia64__)
#error FIXME: Unsupported __ImageBase implementation.
#else
/* Hack, for bug in ld.  Will be removed soon.  */
#define __ImageBase _image_base__
/* This symbol is defined by the linker.  */
extern IMAGE_DOS_HEADER __ImageBase;
#endif

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
