/*

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

	2003-04-27 hamtitampti
	
 */

#include "2bload.h"
#include "sha1.h"

extern int decompress_kernel(char*out, char *data, int len);

DWORD PciWriteDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, unsigned int dw) 
{
		
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #
	base_addr |= ((reg_off & 0xff));

	IoOutputDword(0xcf8, base_addr );	
	IoOutputDword(0xcfc ,dw);

	return 0;    
}

DWORD PciReadDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off)
{
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #
        base_addr |= ((func & 0x07) << 8);
        base_addr |= ((reg_off & 0xff));
        
	IoOutputDword(0xcf8, base_addr);
	return IoInputDword(0xcfc);
}

void BootAGPBUSInitialization(void)
{
	DWORD temp;
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x54,   PciReadDword(BUS_0, DEV_1, FUNC_0, 0x54) | 0x88000000 );
	
	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x64,   (PciReadDword(BUS_0, DEV_0, FUNC_0, 0x64))| 0x88000000 );
	
	temp =  PciReadDword(BUS_0, DEV_0, FUNC_0, 0x6C);
	IoOutputDword(0xcfc , temp & 0xFFFFFFFE);
	IoOutputDword(0xcfc , temp );
	
	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x80, 0x00000100);
    
}
 
/* -------------------------  Main Entry for after the ASM sequences ------------------------ */

extern void BootStartBiosLoader ( void ) {

	// do not change this, this is linked to many many scipts
	unsigned int PROGRAMM_Memory_2bl 	= 0x00100000;
	unsigned int CROMWELL_Memory_pos 	= 0x03A00000;
	unsigned int CROMWELL_compress_temploc 	= 0x02000000;

	unsigned int Buildinflash_Flash[4]	= { 0xfff00000,0xfff40000,0xfff80000,0xfffc0000};
        unsigned int cromwellidentify	 	=  1;
        unsigned int flashbank		 	=  3;  // Default Bank
        unsigned int cromloadtry		=  0;
        	
	struct SHA1Context context;
      	unsigned char SHA1_result[20];
        
        unsigned char bootloaderChecksum[20];
        unsigned int bootloadersize;
        unsigned int loadretry;
	unsigned int compressed_image_start;
	unsigned int compressed_image_size;
	unsigned int Biossize_type;
	int temp;
	
        int validimage;
        	
	memcpy(&bootloaderChecksum[0],(void*)PROGRAMM_Memory_2bl,20);
	memcpy(&bootloadersize,(void*)(PROGRAMM_Memory_2bl+20),4);
	memcpy(&compressed_image_start,(void*)(PROGRAMM_Memory_2bl+24),4);
	memcpy(&compressed_image_size,(void*)(PROGRAMM_Memory_2bl+28),4);
	memcpy(&Biossize_type,(void*)(PROGRAMM_Memory_2bl+32),4);
	        
      	SHA1Reset(&context);
	SHA1Input(&context,(void*)(PROGRAMM_Memory_2bl+20),bootloadersize-20);
	SHA1Result(&context,SHA1_result);
	        
        if (_memcmp(&bootloaderChecksum[0],&SHA1_result[0],20)==0) {
		// HEHE, the Image we copy'd into ram is SHA-1 hash identical, this is Optimum
		BootPerformPicChallengeResponseAction();
		                                      
	} else {
		// Bad, the checksum does not match, but we can nothing do now, we wait until PIC kills us
		while(1);
	}
       
        // Sets the Graphics Card to 60 MB start address
        (*(unsigned int*)0xFD600800) = (0xf0000000 | ((64*0x100000) - 0x00400000));
        
	BootAGPBUSInitialization();

	(*(unsigned int*)(0xFD000000 + 0x100200)) = 0x03070103 ;
	(*(unsigned int*)(0xFD000000 + 0x100204)) = 0x11448000 ;
        
        PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x84, 0x7FFFFFF);  // 128 MB
	
	// Lets go, we have finished, the Most important Startup, we have now a valid Micro-loder im Ram
	// we are quite happy now
	
        validimage=0;
        flashbank=3;
	for (loadretry=0;loadretry<100;loadretry++) {
		cromloadtry=0;
		if (Biossize_type==0) {
                	// Means we have a 256 kbyte image
                	 flashbank=3;
                }                                 
               	else if (Biossize_type==1) {
                	// Means we have a 1MB image
                	// If 25 load attempts failed, we switch to the next bank
			switch (loadretry) {
				case 0:
					flashbank=1;
					break;	
				case 25:
     	          			flashbank=2;
      					break;
        	        	case 50:
					flashbank=0;
                			break;
	                	case 75:
        	        		flashbank=3;
                			break;	
			}
                }
		cromloadtry++;	
                
        	// Copy From Flash To RAM
      		memcpy(&bootloaderChecksum[0],(void*)(Buildinflash_Flash[flashbank]+compressed_image_start),20);

                memcpy((void*)CROMWELL_compress_temploc,(void*)(Buildinflash_Flash[flashbank]+compressed_image_start+20),compressed_image_size);
		memset((void*)(CROMWELL_compress_temploc+compressed_image_size),0x00,20*1024);
		
		// Lets Look, if we have got a Valid thing from Flash        	
      		SHA1Reset(&context);
		SHA1Input(&context,(void*)(CROMWELL_compress_temploc),compressed_image_size);
		SHA1Result(&context,SHA1_result);
		
		if (_memcmp(&bootloaderChecksum[0],SHA1_result,20)==0) {
			// The Checksum is good                          
			// We start the Cromwell immediatly
                        
                        I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );
		
			BufferIN = (unsigned char*)(CROMWELL_compress_temploc);
			BufferINlen=compressed_image_size;
			BufferOUT = (unsigned char*)CROMWELL_Memory_pos;
			decompress_kernel(BufferOUT, BufferIN, BufferINlen);
			
			// This is a config bit in Cromwell, telling the Cromwell, that it is a Cromwell and not a Xromwell
			flashbank++; // As counting starts with 0, we increase +1
			memcpy((void*)(CROMWELL_Memory_pos+0x20),&cromwellidentify,4);
			memcpy((void*)(CROMWELL_Memory_pos+0x24),&cromloadtry,4);
		 	memcpy((void*)(CROMWELL_Memory_pos+0x28),&flashbank,4);
		 	memcpy((void*)(CROMWELL_Memory_pos+0x2C),&Biossize_type,4);
		 	validimage=1;
		 	
		 	break;
		}
	}
        
        if (validimage==1) {
                
                I2cSetFrontpanelLed(
		I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3 |
		I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3
		);
	
	        // We now jump to the cromwell, Good bye 2bl loader
		// This means: jmp CROMWELL_Memory_pos == 0x03A00000
		__asm __volatile__ (
		"wbinvd\n"
		"cld\n"
		"ljmp $0x10, $0x03A00000\n"   
		);
		// We are not Longer here
	}
	
	// Bad, we did not get a valid im age to RAM, we stop and display a error
	//I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );	

	I2cSetFrontpanelLed(
		I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3 |
		I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3
	);
        
//	I2CTransmitWord(0x10, 0x1901); // no reset on eject
//	I2CTransmitWord(0x10, 0x0c00); // eject DVD tray        

        while(1);
}
