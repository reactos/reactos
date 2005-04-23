#include <stdio.h>
#include <windows.h>

int main(int argc, char **argv)
{
   FILE *fp;
   int len;
   long offset;
   unsigned long OldCheckSum, CheckSum;
   int i;

   unsigned char *buffer;

   if (argc < 2)
   {
      printf("PESum In [Out]\n");
      printf("  If [out] is given, adding attribute IMAGE_SCN_MEM_NOT_PAGED to '.text', '.data', '.idata' and '.bss' sections.\n");
      return 1;
   }

   fp = fopen(argv[1], "rb");

   if (fp == NULL)
   {
      printf("Unable to open input file '%s'\n", argv[1]);
      return 2;
   }

   fseek(fp, 0, SEEK_END);
   len = ftell(fp);
   fseek(fp, 0, SEEK_SET);

   buffer=(unsigned char*) malloc(((len+1)/2)*2);
   if (buffer == NULL)
   {
      fclose(fp);
      printf("Not enough memory available.\n");
      return 3;
   }

   memset(buffer, 0x00, ((len+1)/2)*2);
   fread(buffer, len, 1, fp);
   fclose(fp);

   offset = ((PIMAGE_DOS_HEADER)buffer)->e_lfanew;

   if (offset + sizeof(IMAGE_NT_HEADERS) >= len || ((PIMAGE_NT_HEADERS)&buffer[offset])->Signature != IMAGE_NT_SIGNATURE)
   {
      printf("'%s' isn't a PE image\n", argv[1]);
      printf("%d %d\n", offset + sizeof(IMAGE_NT_HEADERS), len);
      free(buffer);
      return 4;
   }

   OldCheckSum = ((PIMAGE_NT_HEADERS)&buffer[offset])->OptionalHeader.CheckSum;
   ((PIMAGE_NT_HEADERS)&buffer[offset])->OptionalHeader.CheckSum = 0;

   for(i=0; i<((PIMAGE_NT_HEADERS)&buffer[offset])->OptionalHeader.NumberOfRvaAndSizes; i++)
   {
      if (!((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].VirtualAddress)
	 break;
#if 1
      printf("%-8.8s %08x %08x %08x ", 
	     ((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].Name,
	     ((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].VirtualAddress,
	     ((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].SizeOfRawData,
   	     ((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].Characteristics);

      if (((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].Characteristics & IMAGE_SCN_CNT_CODE)
	 printf("C");
      if (((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
         printf("I");
      if (((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
	 printf("U");
      if (((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].Characteristics & IMAGE_SCN_MEM_READ)
	 printf("R");
      if (((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].Characteristics & IMAGE_SCN_MEM_WRITE)
	 printf("W");
      if (((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)
	 printf("X");
      if (((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].Characteristics & IMAGE_SCN_MEM_NOT_PAGED)
	 printf("nP");
      if (((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
	 printf("D");
#endif
      if (argc>2)
      {
	 char name[9];
	 strncpy(name, ((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].Name, 8);
	 name[8]=0;
	 if(!strcmp(name, ".text") || !strcmp(name, ".data") || !strcmp(name, ".bss") || !strcmp(name, ".idata"))
	 {
   	    ((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].Characteristics |= IMAGE_SCN_MEM_NOT_PAGED;
   	    ((PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)&buffer[offset]))[i].Characteristics &= ~IMAGE_SCN_MEM_DISCARDABLE;
	 }
      }
      printf("\n");

   }
   printf("\n");
   CheckSum = 0;
   for(i = 0; i < len; i += 2)
   {
      CheckSum += *(unsigned short*)&buffer[i];
      CheckSum = 0xffff & (CheckSum + (CheckSum >> 16));
   }

   CheckSum+=len;

   if (argc > 2)
   {
      fp = fopen(argv[2], "wb+");
      if (fp == NULL)
      {
         printf("Unable to open output file '%s'\n", argv[2]);
         free(buffer);
         return 5;
      }
      ((PIMAGE_NT_HEADERS)&buffer[offset])->OptionalHeader.CheckSum = CheckSum;
      fwrite(buffer, len, 1, fp);
      fclose(fp);
      printf("Creating '%s' from '%s' with PE checksum %08x\n", argv[2], argv[1], CheckSum);

   }
   else
      printf("PE-checksum for '%s' is %08x (old %08x)\n", argv[1], CheckSum, OldCheckSum);

   free(buffer);
   return 0;
}
