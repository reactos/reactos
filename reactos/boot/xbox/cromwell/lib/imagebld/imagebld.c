#include <errno.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

// #include <linux/hdreg.h>
#include <string.h>

#include <stdarg.h>
#include <stdlib.h>
#include "sha1.h"
#include "xbe-header.h"
//#include "lzari.h"

#define debug


struct Checksumstruct {
	unsigned char Checksum[20];	
	unsigned int Size_ramcopy;  // 20
	unsigned int compressed_image_start; // 24
	unsigned int compressed_image_size;  // 28
	unsigned int Biossize_type;          //32
}Checksumstruct;

void shax(unsigned char *result, unsigned char *data, unsigned int len)
{
	struct SHA1Context context;
	SHA1Reset(&context);
	SHA1Input(&context, (unsigned char *)&len, 4);
	SHA1Input(&context, data, len);
	SHA1Result(&context,result);	
}



int xberepair (	unsigned char * xbeimage,
		unsigned char * cromimage
		)
{
	FILE *f;
	unsigned int xbesize;
        int a;
	unsigned char sha_Message_Digest[SHA1HashSize];
        unsigned char *crom;
        unsigned char *xbe;
        unsigned int romsize=0;
         
	XBE_HEADER *header;
 	XBE_SECTION *sechdr;

        crom = malloc(1024*1024);
        xbe = malloc(1024*1024+0x3000);


       	printf("ImageBLD Hasher by XBL Project (c) hamtitampti\n");
       	printf("XBE Modus\n");
	
	f = fopen(cromimage, "rb");
	if (f==NULL) {
		fprintf(stderr,"Unable to open cromwell image file %s : %s \nAborting\n",cromimage,strerror(errno)); 
		return 1;
	}
	
 	fseek(f, 0, SEEK_END); 
        romsize	 = ftell(f);        
        fseek(f, 0, SEEK_SET);
    	fread(crom, 1, romsize, f);
    	fclose(f);
    	
    	if (romsize>0x100000) {
    		printf("Romsize too big, increase the satic Variables everywhere");
    		return 1;
    	}
    
//	f=NULL;
	f = fopen(xbeimage, "rb");
    	if (f==NULL) {
		fprintf(stderr,"Unable to open xbe destination file %s : %s \nAborting\n",xbeimage,strerror(errno)); 
		return 1;
	}
  		
	fseek(f, 0, SEEK_END); 
        xbesize	 = ftell(f); 
        fseek(f, 0, SEEK_SET);

    	memset(xbe,0x00,sizeof(xbe));
    	fread(xbe, 1, xbesize, f);
    	fclose(f);
        
        // We copy the ROM image now into the Thing
        memcpy(&xbe[0x3000],crom,romsize);
        memcpy(&xbe[0x1084],&romsize,4);	// We dump the ROM Size into the Image
        
        romsize = (romsize & 0xfffffff0) + 32;	// We fill it up with "spaces"
        
        
        header = (XBE_HEADER*) xbe;
	// This selects the First Section, we only have one
	sechdr = (XBE_SECTION *)(((char *)xbe) + (int)header->Sections - (int)header->BaseAddress);
	        
        // Correcting overall size now
	xbesize = 0x3000+romsize;
        header->ImageSize = xbesize; 
	
	//printf("%08x",sechdr->FileSize);                    
	

	sechdr->FileSize = 0x2000+romsize;
	sechdr->VirtualSize = 0x2000+romsize;
        
 //       printf("Sections: %d\n",header->NumSections);

       	shax(&sha_Message_Digest[0], ((unsigned char *)xbe)+(int)sechdr->FileAddress ,sechdr->FileSize);
  	memcpy(&sechdr->ShaHash[0],&sha_Message_Digest[0],20);
  	
  	#ifdef debug
 	printf("Size of all headers:     : 0x%08X\n", (unsigned int)header->HeaderSize);
        
       	printf("Size of entire image     : 0x%08X\n", (unsigned int)header->ImageSize);
	printf("Virtual address          : 0x%08X\n", (unsigned int)sechdr->VirtualAddress);
       	printf("Virtual size             : 0x%08X\n", (unsigned int)sechdr->VirtualSize);
       	printf("File address             : 0x%08X\n", (unsigned int)sechdr->FileAddress);
       	printf("File size                : 0x%08X\n", (unsigned int)sechdr->FileSize);

	printf("Section 0 Hash XBE       : ");
	for(a=0; a<SHA1HashSize; a++) {
		printf("%02x",sha_Message_Digest[a]);
	}
      	printf("\n");
      	#endif
      	
	// Write back the Image to Disk
	f = fopen(xbeimage, "wb");
    	if (f!=NULL) {   
	 	fwrite(xbe, 1, xbesize, f);
       	 	fclose(f);			
	}	  	
        
        printf("XRomwell File Created    : %s\n",xbeimage);
        
	return 0;	
}


int vmlbuild (	unsigned char * vmlimage,
		unsigned char * cromimage
		)
{
	FILE *f;
	unsigned int vmlsize;
        int a;

        unsigned char *crom;
        unsigned char *vml;
        unsigned int romsize=0;
	
	crom = malloc(1024*1024);
        vml = malloc(1024*1024+0x3000);
         
       	printf("ImageBLD Hasher by XBL Project (c) hamtitampti\n");
       	printf("VML Modus\n");
	
	f = fopen(cromimage, "rb");
	if (f!=NULL) 
    	{    
 		fseek(f, 0, SEEK_END); 
         	romsize	 = ftell(f);        
         	fseek(f, 0, SEEK_SET);
    		fread(crom, 1, romsize, f);
    		fclose(f);
    	}
    	
    	if (romsize>0x100000) {
    		printf("Romsize too big, increase the satic Variables everywhere");
    		return 1;
    	}
    	
	f = fopen(vmlimage, "rb");
    	if (f!=NULL) 
    	{   
  		fseek(f, 0, SEEK_END); 
         	vmlsize	 = ftell(f); 
         	fseek(f, 0, SEEK_SET);
    		fread(vml, 1, vmlsize, f);
    		fclose(f); 
    			
    		memcpy(&vml[0x1700],crom,romsize);
    		memset(&vml[0x1700+romsize],0x0,100);
    		vmlsize +=romsize;
    		vmlsize +=100;
    		
		// Write back the Image to Disk
		f = fopen(vmlimage, "wb");
    		if (f!=NULL) 
    		{   
		 fwrite(vml, 1, vmlsize, f);
        	 fclose(f);			
		} 
		
	        printf("VML File Created         : %s\n",vmlimage);    		
    		
    	}


}


int romcopy (
		unsigned char * blbinname,
		unsigned char * cromimage,
		unsigned char * binname256,
		unsigned char * binname1024
		
		)
{
	
	static unsigned char SHA1_result[SHA1HashSize];
	struct SHA1Context context;
	FILE *f;
	unsigned char *loaderimage;
	unsigned char *flash256;
	unsigned char *flash1024;
	unsigned char *crom;
	unsigned int freeflashspace = 256*1024;
       	unsigned int romsize=0;
       	struct Checksumstruct bootloaderstruct ;
       	unsigned int bootloderpos;
       	int a;
//       	unsigned int compressedimagestart;
       	unsigned int temp;
       	
	loaderimage = malloc(256*1024);
	flash256 = malloc(256*1024);
	flash1024 = malloc(1024*1024);
	crom = malloc(1024*1024);
       	
       	memset(flash256,0x00,256*1024);
	memset(flash1024,0x00,1024*1024);
       	memset(crom,0x00,1024*1024);
       	memset(loaderimage,0x00,256*1024);
       	

       	printf("ImageBLD Hasher by XBL Project (c) hamtitampti\n");
       	printf("ROM Modus\n");
       	
      	
       	a=1;
	f = fopen(blbinname, "rb");
    	if (f!=NULL) 
    	{    
		fread(loaderimage, 1, 256*1024, f);
 	        fclose(f);
	} else {
		printf("Error, NO Image found\n");
		a=0;
	}
	
    	f = fopen(cromimage, "rb");
	if (f!=NULL) 
    	{    
 		fseek(f, 0, SEEK_END); 
         	romsize	 = ftell(f);        
         	fseek(f, 0, SEEK_SET);
    		fread(crom, 1, romsize, f);
    		fclose(f);
    	} else {
    		printf("Error, NO CROM found\n");	
    		a=0;
    	}
	
	// Break, we have a error
	if (a==0) {	
		printf("ERROR-- we can not dump our checksum ...\n");
		return 1;
	}
	
	// Ok, we have loaded both images, we can continue
	if (a==1) {	


		// this is very nasty, but simple , we Dump a GDT to the TOP rom
		
        	memset(&loaderimage[0x3fe00],0x90,512);
        	memset(&loaderimage[0x3ffd0],0x00,32);
                loaderimage[0x3ffcf] = 0xfc;
        	loaderimage[0x3ffd0] = 0xea;
    		loaderimage[0x3ffd2] = 0x10;
    		loaderimage[0x3ffd3] = 0xfc;
    		loaderimage[0x3ffd4] = 0xff;
    		loaderimage[0x3ffd5] = 0x08;
    		loaderimage[0x3ffd7] = 0x90;    		
    		
    		loaderimage[0x3ffe0] = 0xff;
    		loaderimage[0x3ffe1] = 0xff;
    		loaderimage[0x3ffe5] = 0x9b;
    		loaderimage[0x3ffe6] = 0xcf;
    		loaderimage[0x3ffe8] = 0xff;
    		loaderimage[0x3ffe9] = 0xff;
    		loaderimage[0x3ffed] = 0x93;
    		loaderimage[0x3ffee] = 0xcf;
     	
    		loaderimage[0x3fff4] = 0x18;
    		loaderimage[0x3fff5] = 0x00;
    		loaderimage[0x3fff6] = 0xd8;
    		loaderimage[0x3fff7] = 0xff;
		loaderimage[0x3fff8] = 0xff;    	
    		loaderimage[0x3fff9] = 0xff;
    		
    		// We have dumped the GDT now, we continue	
                
		memcpy(&bootloderpos,&loaderimage[0x40],4);   	// This can be foun in the 2bBootStartup.S
		memset(&loaderimage[0x40],0x0,4);    		// We do not need this helper sum anymore
		memcpy(&bootloaderstruct,&loaderimage[bootloderpos],sizeof(struct Checksumstruct));
		
		memcpy(flash256,loaderimage,256*1024);
		memcpy(flash1024,loaderimage,256*1024);
		
		// We make now sure, there are some "space" bits and we start oranized with 16
		temp = bootloderpos + bootloaderstruct.Size_ramcopy;
		temp = temp & 0xfffffff0;
		temp = temp + 0x10;
		
		// We add additional 0x100 byts for some space
		temp = temp + 0x100;
		
		bootloaderstruct.compressed_image_start = temp;
		bootloaderstruct.compressed_image_size =  romsize;

		//freeflashspace = freeflashspace - 512; // We decrement the TOP ROM
		// We have no TOP ROM anymore
		freeflashspace = freeflashspace - bootloaderstruct.compressed_image_start;
		
		bootloaderstruct.Biossize_type = 0; // Means it is a 256 kbyte Image
		memcpy(&flash256[bootloderpos],&bootloaderstruct,sizeof(struct Checksumstruct));
		bootloaderstruct.Biossize_type = 1; // Means it is a 1MB Image
		memcpy(&flash1024[bootloderpos],&bootloaderstruct,sizeof(struct Checksumstruct));		
		
		#ifdef debug		
		printf("BootLoaderPos            : %08x\n",bootloderpos);
		printf("Size2blRamcopy           : %08x\n",bootloaderstruct.Size_ramcopy);		
		printf("ROM Image Start          : %08x\n",bootloaderstruct.compressed_image_start);
		printf("ROM Image Size           : %08x (%d Byte)\n",romsize,romsize); 
		printf("ROM compressed Image Size: %08x (%d Byte)\n",bootloaderstruct.compressed_image_size,bootloaderstruct.compressed_image_size); 
		printf("Avaliable Size in ROM    : %08x (%d Byte)\n",freeflashspace,freeflashspace);
		printf("Percent of ROM USED:     : %2.2f \n",(float)bootloaderstruct.compressed_image_size/freeflashspace*100);
		#endif
		
		// We have calculated the size of the kompressed image and where it can start (where the 2bl ends)
		// we make now the hash sum of the 2bl itself			

		// This is for 256 kbyte image
		// We start with offset 20, as the first 20 bytes are the checksum
		SHA1Reset(&context);
		SHA1Input(&context,&flash256[bootloderpos+20],(bootloaderstruct.Size_ramcopy-20));
		SHA1Result(&context,SHA1_result);
	      
	        // We dump now the SHA1 sum into the 2bl loader image
	      	memcpy(&flash256[bootloderpos],&SHA1_result[0],20);

		#ifdef debug
		printf("2bl Hash 256Kb image     : ");
		for(a=0; a<SHA1HashSize; a++) {
			printf("%02X",SHA1_result[a]);
		}
	      	printf("\n");
	      	#endif


		// This is for 1MB Image kbyte image
		// We start with offset 20, as the first 20 bytes are the checksum
		SHA1Reset(&context);
		SHA1Input(&context,&flash1024[bootloderpos+20],(bootloaderstruct.Size_ramcopy-20));
		SHA1Result(&context,SHA1_result);
	      
	        // We dump now the SHA1 sum into the 2bl loader image
	      	memcpy(&flash1024[bootloderpos],&SHA1_result[0],20);
              
		#ifdef debug
		printf("2bl Hash 1MB image       : ");
		for(a=0; a<SHA1HashSize; a++) {
			printf("%02X",SHA1_result[a]);
		}
	      	printf("\n");
	      	#endif
	      	
		// In 1MB flash we need the Image 2 times, as ... you know
		memset(&flash1024[(0*256*1024)+bootloaderstruct.compressed_image_start],0xff,256*1024-bootloaderstruct.compressed_image_start-512);
		memcpy(&flash1024[3*256*1024],&flash1024[0],256*1024);

                
                // Ok, the 2BL loader is ready, we now go to the "Kernel"

                memset(&flash256[bootloaderstruct.compressed_image_start+20+romsize],0xff,256*1024-(bootloaderstruct.compressed_image_start+20+romsize)-512);
	      	
	        // The first 20 bytes of the compressed image are the checksum
		memcpy(&flash256[bootloaderstruct.compressed_image_start+20],&crom[0],romsize);
		SHA1Reset(&context);
		SHA1Input(&context,&flash256[bootloaderstruct.compressed_image_start+20],romsize);
		SHA1Result(&context,SHA1_result);                                
		memcpy(&flash256[bootloaderstruct.compressed_image_start],SHA1_result,20);			

		memset(&flash1024[1*256*1024],0xff,2*1024*256);
		    			
		memcpy(&flash1024[bootloaderstruct.compressed_image_start+(1*256*1024)],SHA1_result,20);			
		memcpy(&flash1024[bootloaderstruct.compressed_image_start+20+(1*256*1024)],&crom[0],romsize);

		memcpy(&flash1024[bootloaderstruct.compressed_image_start+(2*256*1024)],SHA1_result,20);			
	      	memcpy(&flash1024[bootloaderstruct.compressed_image_start+20+(2*256*1024)],&crom[0],romsize);

			      	
	      	#ifdef debug
		printf("ROM Hash 1MB             : ");
		for(a=0; a<SHA1HashSize; a++) {
			printf("%02X",SHA1_result[a]);
		}
	      	printf("\n");
	      	#endif
	      	
	      	
	      	// Write the 256 /1024 Kbyte Image Back
	      	f = fopen(binname256, "wb");               
		fwrite(flash256, 1, 256*1024, f);
         	fclose(f);	
		
	      	f = fopen(binname1024, "wb");               
		fwrite(flash1024, 1, 1024*1024, f);
         	fclose(f);	
                
                #ifdef debug
		printf("Binary 256k File Created : %s\n",binname256);
		printf("Binary 1MB  File Created : %s\n",binname1024);
		#endif	              
		
	} 
	

	return 0;	
}        




int main (int argc, const char * argv[])
{
	if( argc < 3 ) 	return -1;
	
	if (strcmp(argv[1],"-xbe")==0) { 
		xberepair((unsigned char*)argv[2],(unsigned char*)argv[3]);
	}
	
	if (strcmp(argv[1],"-vml")==0) { 
		vmlbuild((unsigned char*)argv[2],(unsigned char*)argv[3]);
	}
	
	if (strcmp(argv[1],"-rom")==0) { 
		if( argc < 4 ) return -1;
		romcopy((unsigned char*)argv[2],(unsigned char*)argv[3],(unsigned char*)argv[4],(unsigned char*)argv[5]);
	}

	return 0;	
}
