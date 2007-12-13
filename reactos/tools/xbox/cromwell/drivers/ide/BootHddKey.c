
#include "boot.h"
#include "sha1.h"
#include "rc4.h"
#include <stdarg.h>
//#include <string.h>



void HMAC_hdd_calculation(int version,unsigned char *HMAC_result, ... );

extern size_t strlen(const char * s);


int copy_swap_trim(unsigned char *dst, unsigned char *src, int len)
{
	unsigned char tmp;
	int i;
        for (i=0; i < len; i+=2) {
		tmp = src[i];     //allow swap in place
		dst[i] = src[i+1];
		dst[i+1] = tmp;
	}

	--dst;
	for (i=len; i>0; --i) {
		if (dst[i] != ' ') {
			dst[i+1] = 0;
			break;
		}
	}
	return i;
}

int HMAC1hddReset(int version,SHA1Context *context)
{
  	SHA1Reset(context);
	switch (version) {
		case 9:
  			context->Intermediate_Hash[0] = 0x85F9E51A;
			context->Intermediate_Hash[1] = 0xE04613D2;
			context->Intermediate_Hash[2] = 0x6D86A50C;
			context->Intermediate_Hash[3] = 0x77C32E3C;
			context->Intermediate_Hash[4] = 0x4BD717A4;
  			break;
		case 10:
			context->Intermediate_Hash[0] = 0x72127625;
			context->Intermediate_Hash[1] = 0x336472B9;
			context->Intermediate_Hash[2] = 0xBE609BEA;
			context->Intermediate_Hash[3] = 0xF55E226B;
			context->Intermediate_Hash[4] = 0x99958DAC;
			break;
		case 11:
			context->Intermediate_Hash[0] = 0x39B06E79;
			context->Intermediate_Hash[1] = 0xC9BD25E8;
			context->Intermediate_Hash[2] = 0xDBC6B498;
			context->Intermediate_Hash[3] = 0x40B4389D;
			context->Intermediate_Hash[4] = 0x86BBD7ED;
			break;
		case 12:
			context->Intermediate_Hash[0] = 0x8058763a;
                 	context->Intermediate_Hash[1] = 0xf97d4e0e;
                   	context->Intermediate_Hash[2] = 0x865a9762;
	               	context->Intermediate_Hash[3] = 0x8a3d920d;
			context->Intermediate_Hash[4] = 0x08995b2c;
			break;
	}
	context->Length_Low = 512;

	return shaSuccess;
}

int HMAC2hddReset(int version,SHA1Context *context)
{
	SHA1Reset(context);
	switch (version) {
		case 9:
			context->Intermediate_Hash[0] = 0x5D7A9C6B;
			context->Intermediate_Hash[1] = 0xE1922BEB;
			context->Intermediate_Hash[2] = 0xB82CCDBC;
			context->Intermediate_Hash[3] = 0x3137AB34;
			context->Intermediate_Hash[4] = 0x486B52B3;
			break;
		case 10:
			context->Intermediate_Hash[0] = 0x76441D41;
			context->Intermediate_Hash[1] = 0x4DE82659;
			context->Intermediate_Hash[2] = 0x2E8EF85E;
			context->Intermediate_Hash[3] = 0xB256FACA;
			context->Intermediate_Hash[4] = 0xC4FE2DE8;
			break;
		case 11:
			context->Intermediate_Hash[0] = 0x9B49BED3;
			context->Intermediate_Hash[1] = 0x84B430FC;
			context->Intermediate_Hash[2] = 0x6B8749CD;
			context->Intermediate_Hash[3] = 0xEBFE5FE5;
			context->Intermediate_Hash[4] = 0xD96E7393;
			break;
		case 12:
       			context->Intermediate_Hash[0] = 0x01075307;
		      	context->Intermediate_Hash[1] = 0xa2f1e037;
			context->Intermediate_Hash[2] = 0x1186eeea;
			context->Intermediate_Hash[3] = 0x88da9992;
			context->Intermediate_Hash[4] = 0x168a5609;
			break;
	}
	context->Length_Low  = 512;

	return shaSuccess;
}

void HMAC_SHA1( unsigned char *result,
		unsigned char *key, int key_length,
		unsigned char *text1, int text1_length,
		unsigned char *text2, int text2_length )
{
	unsigned char state1[0x40];
	unsigned char state2[0x40+0x14];
	int i;
	struct SHA1Context context;

	for(i=0x40-1; i>=key_length;--i) state1[i] = 0x36;
	for(;i>=0;--i) state1[i] = key[i] ^ 0x36;

	SHA1Reset(&context);
	SHA1Input(&context,state1,0x40);
	SHA1Input(&context,text1,text1_length);
	SHA1Input(&context,text2,text2_length);
	SHA1Result(&context,&state2[0x40]);

	for(i=0x40-1; i>=key_length;--i) state2[i] = 0x5C;
	for(;i>=0;--i) state2[i] = key[i] ^ 0x5C;

	SHA1Reset(&context);
	SHA1Input(&context,state2,0x40+0x14);
	SHA1Result(&context,result);

}


void HMAC_hdd_calculation(int version,unsigned char *HMAC_result, ... )
{
	va_list args;
	struct SHA1Context context;

	va_start(args,HMAC_result);

	HMAC1hddReset(version, &context);
	while(1) {
		unsigned char *buffer = va_arg(args,unsigned char *);
		int length;

		if (buffer == NULL) break;
		length = va_arg(args,int);
		SHA1Input(&context,buffer,length);

	}
	va_end(args);

	SHA1Result(&context,&context.Message_Block[0]);
	HMAC2hddReset(version, &context);
	SHA1Input(&context,&context.Message_Block[0],0x14);
	SHA1Result(&context,HMAC_result);
}


DWORD BootHddKeyGenerateEepromKeyData(
		BYTE *pbEeprom_data,
		BYTE *pbResult
		
) {

	BYTE baKeyHash[20];
	BYTE baDataHashConfirm[20];
	BYTE baEepromDataLocalCopy[0x30];
	struct rc4_key RC4_key;
       	int version = 0; 
       	int counter;
	int n,f;        
        
        // Static Version change not included yet
        
	for (counter=9;counter<13;counter++)
	{
    		memset(&RC4_key,0,sizeof(rc4_key));
		memcpy(&baEepromDataLocalCopy[0], pbEeprom_data, 0x30);
                
                // Calculate the Key-Hash
		HMAC_hdd_calculation(counter, baKeyHash, &baEepromDataLocalCopy[0], 20, NULL);

		//initialize RC4 key
		rc4_prepare_key(baKeyHash, 20, &RC4_key);

        	//decrypt data (from eeprom) with generated key
		rc4_crypt(&baEepromDataLocalCopy[20],8,&RC4_key);		//confounder of some kind?
		rc4_crypt(&baEepromDataLocalCopy[28],20,&RC4_key);		//"real" data
                
                // Calculate the Confirm-Hash
		HMAC_hdd_calculation(counter, baDataHashConfirm, &baEepromDataLocalCopy[20], 8, &baEepromDataLocalCopy[28], 20, NULL);

		f=0;
		for(n=0;n<0x14;n++) {
				if(baEepromDataLocalCopy[n]!=baDataHashConfirm[n]) f=1;
		}
		
		if (f==0) { 
			// Confirm Hash is correct  
			// Copy actual Xbox Version to Return Value
			version=counter;
			// exits the loop
			break;
			
		}
		
		       	
	
	}
	
	//copy out HDKey
	memcpy(pbResult,&baEepromDataLocalCopy[28],16);
	
	return version;
}


