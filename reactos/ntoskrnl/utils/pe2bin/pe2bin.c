#include <stdio.h>
#include <base.h>
#include <pe.h>

int main(int argc, char* argv[])
{
   FILE* in;
   FILE* out;
   IMAGE_DOS_HEADER dos_header;
   DWORD pe_signature;
   IMAGE_FILE_HEADER file_header;
   IMAGE_OPTIONAL_HEADER opt_header;
   IMAGE_SECTION_HEADER scn_header;
   int i;
   int prev_pos;
   void* buffer;
   ULONG base_address;
   
   if (argc != 4)
     {
	printf("usage: pe2bin base infile outfile\n");
	return(1);
     }
   
   base_address = strtoul(argv[1],NULL,0);
   printf("Generating for base %x\n",base_address);
   
   in = fopen(argv[2], "rb");
   if (in == NULL)
     {
	printf("Unable to open %s\n",argv[1]);
	return(1);
     }
   
   out = fopen(argv[3], "wb");
   if (out == NULL)
     {
	printf("Unable to open %s\n",argv[2]);
	return(1);
     }
   
   fread(&dos_header,sizeof(IMAGE_DOS_HEADER),1,in);
//   printf("dos_header.e_magic %x\n",dos_header.e_magic);
   if (dos_header.e_magic != IMAGE_DOS_MAGIC)
     {
	printf("Bad magic in dos header\n");
	return(1);
     }
   
   fseek(in,dos_header.e_lfanew,SEEK_SET);
   fread(&pe_signature,sizeof(DWORD),1,in);
   if (pe_signature != IMAGE_PE_MAGIC)
     {
	printf("Bad magic in pe header\n");
	return(1);
     }

   fread(&file_header,sizeof(IMAGE_FILE_HEADER),1,in);
   fread(&opt_header,sizeof(IMAGE_OPTIONAL_HEADER),1,in);
   
//   printf("Linker version: %d.%d\n",opt_header.MajorLinkerVersion,
//	  opt_header.MinorLinkerVersion);
   
   for (i=0; i<file_header.NumberOfSections; i++)
     {
	fread(&scn_header,sizeof(IMAGE_SECTION_HEADER),1,in);
//	printf("Section name: %.8s\n",scn_header.Name);
//	printf("Virtual address: %x\n",scn_header.VirtualAddress 
//	       + opt_header.ImageBase);
//	printf("Characteristics: %x\n",scn_header.Characteristics);
	
	if ((scn_header.Characteristics & IMAGE_SECTION_CODE) ||
	    (scn_header.Characteristics & IMAGE_SECTION_INITIALIZED_DATA))
	  {
//	     printf("Writing section to output file\n");
	     prev_pos = ftell(in);
	     fseek(in, scn_header.PointerToRawData, SEEK_SET);
	     buffer = malloc(scn_header.SizeOfRawData);
	     fread(buffer, scn_header.SizeOfRawData, 1, in);
	     fseek(out, scn_header.VirtualAddress + opt_header.ImageBase -
		   base_address,
		   SEEK_SET);
	     fwrite(buffer, scn_header.SizeOfRawData, 1, out);
	     fseek(in, prev_pos, SEEK_SET);
	  }
     }
   return(0);
}
