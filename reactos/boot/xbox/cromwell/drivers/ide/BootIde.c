/*
 *  This code was originally from:
 *   UBL, The Universal Talkware Boot Loader
 *    Copyright (C) 2000 Universal Talkware Inc.
 */

#include "boot.h"
#include "shared.h"
#include "BootEEPROM.h"
#include "BootIde.h"

#undef sprintf

////////////////////////////////////
// IDE types and constants
#define IDE_SECTOR_SIZE 		0x200
#define IDE_BASE1             		(0x1F0u) /* primary controller */

#define IDE_REG_EXTENDED_OFFSET   	(0x200u)

#define IDE_REG_DATA(base)          	((base) + 0u) /* word register */
#define IDE_REG_ERROR(base)         	((base) + 1u)
#define IDE_REG_FEATURE(base)       	((base) + 1u)
#define IDE_REG_SECTOR_COUNT(base)  	((base) + 2u)
#define IDE_REG_SECTOR_NUMBER(base) 	((base) + 3u)
#define IDE_REG_CYLINDER_LSB(base)  	((base) + 4u)
#define IDE_REG_CYLINDER_MSB(base)  	((base) + 5u)
#define IDE_REG_DRIVEHEAD(base)     	((base) + 6u)
#define IDE_REG_STATUS(base)        	((base) + 7u)
#define IDE_REG_COMMAND(base)       	((base) + 7u)
#define IDE_REG_ALTSTATUS(base)     	((base) + IDE_REG_EXTENDED_OFFSET + 6u)
#define IDE_REG_CONTROL(base)       	((base) + IDE_REG_EXTENDED_OFFSET + 6u)
#define IDE_REG_ADDRESS(base)       	((base) + IDE_REG_EXTENDED_OFFSET + 7u)

typedef struct {
	unsigned char m_bPrecomp;
	unsigned char m_bCountSector;
	unsigned char m_bSector;
	unsigned short m_wCylinder;
	unsigned char m_bDrivehead;
	
	   /* 48-bit LBA */   
	unsigned char m_bCountSectorExt;   
	unsigned char m_bSectorExt;   
	unsigned short m_wCylinderExt; 
	
#       define IDE_DH_DEFAULT (0xA0)
#       define IDE_DH_HEAD(x) ((x) & 0x0F)
#       define IDE_DH_MASTER  (0x00)
#       define IDE_DH_SLAVE   (0x10)
#       define IDE_DH_DRIVE(x) ((((x) & 1) != 0)?IDE_DH_SLAVE:IDE_DH_MASTER)
#       define IDE_DH_LBA     (0x40)
#       define IDE_DH_CHS     (0x00)

} tsIdeCommandParams;

#define IDE_DEFAULT_COMMAND { 0xFFu, 0x01, 0x00, 0x0000, IDE_DH_DEFAULT | IDE_DH_SLAVE }

typedef enum {
	IDE_CMD_NOOP = 0,
	IDE_CMD_RECALIBRATE = 0x10,
	IDE_CMD_READ_MULTI_RETRY = 0x20,
	IDE_CMD_READ_MULTI = IDE_CMD_READ_MULTI_RETRY,
	IDE_CMD_READ_MULTI_NORETRY = 0x21,
	
	IDE_CMD_READ_EXT = 0x24, /* 48-bit LBA */
    	
    	IDE_CMD_WRITE_MULTI_RETRY = 0x30,
    	
	IDE_CMD_DRIVE_DIAG = 0x90,
	IDE_CMD_SET_PARAMS = 0x91,
	IDE_CMD_STANDBY_IMMEDIATE = 0x94, /* 2 byte command- also send
	                                     IDE_CMD_STANDBY_IMMEDIATE2 */
	IDE_CMD_SET_MULTIMODE = 0xC6,
	IDE_CMD_STANDBY_IMMEDIATE2 = 0xE0,
	
	//Get info commands
	IDE_CMD_IDENTIFY = 0xEC,
	IDE_CMD_PACKET_IDENTIFY = 0xA1,
		
	ATAPI_SOFT_RESET = 0x08,
	
	IDE_CMD_SET_FEATURES = 0xEF,

	IDE_CMD_ATAPI_PACKET = 0xA0,

	//IDE security commands
	IDE_CMD_SECURITY_SET_PASSWORD = 0xF1,
	IDE_CMD_SECURITY_UNLOCK = 0xF2,
	IDE_CMD_SECURITY_DISABLE = 0xF6	
} ide_command_t;


#define printk_debug bprintf


const char * const szaSenseKeys[] = {
	"No Sense", "Recovered Error", "Not Ready", "Medium Error",
	"Hardware Error", "Illegal request", "Unit Attention", "Data Protect",
	"Reserved 8", "Reserved 9", "Reserved 0xa", "Aborted Command",
	"Miscompare", "Reserved 0xf"
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//  Helper routines
//
/* -------------------------------------------------------------------------------- */

int BootIdeWaitNotBusy(unsigned uIoBase)
{
	BYTE b = 0x80;
	while (b & 0x80) {
		b=IoInputByte(IDE_REG_ALTSTATUS(uIoBase));
	}
	return b&1;
}

/* -------------------------------------------------------------------------------- */

int BootIdeWaitDataReady(unsigned uIoBase)
{
	int i = 0x800000;
	wait_smalldelay();
	do {
		if ( ((IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x88) == 0x08)	)	{
	    if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;
			return 0;
		}
		i--;
	} while (i != 0);

	if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;
	return 1;
}

/* -------------------------------------------------------------------------------- */

int BootIdeIssueAtaCommand(
	unsigned uIoBase,
	ide_command_t command,
	tsIdeCommandParams * params)
{
	int n;

	IoInputByte(IDE_REG_STATUS(uIoBase));

	n=BootIdeWaitNotBusy(uIoBase);
//	if(n)	{// as our command may be being used to clear the error, not a good policy to check too closely!
//		printk("error on BootIdeIssueAtaCommand wait 1: ret=%d, error %02X\n", n, IoInputByte(IDE_REG_ERROR(uIoBase)));
//		return 1;
//	}  
 	
 	/* 48-bit LBA */   
        /* this won't hurt for non 48-bit LBA commands since we re-write these registers below */   
	
	IoOutputByte(IDE_REG_SECTOR_COUNT(uIoBase), params->m_bCountSectorExt);   
	IoOutputByte(IDE_REG_SECTOR_NUMBER(uIoBase), params->m_bSectorExt);   
	IoOutputByte(IDE_REG_CYLINDER_LSB(uIoBase), params->m_wCylinderExt & 0xFF);   
	IoOutputByte(IDE_REG_CYLINDER_MSB(uIoBase), (params->m_wCylinderExt >> 8) ); 
	/* End 48-bit LBA */   

	IoOutputByte(IDE_REG_SECTOR_COUNT(uIoBase), params->m_bCountSector);
	IoOutputByte(IDE_REG_SECTOR_NUMBER(uIoBase), params->m_bSector);
	IoOutputByte(IDE_REG_CYLINDER_LSB(uIoBase), params->m_wCylinder & 0xFF);
	IoOutputByte(IDE_REG_CYLINDER_MSB(uIoBase), (params->m_wCylinder >> 8) /* & 0x03 */);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), params->m_bDrivehead);

	IoOutputByte(IDE_REG_COMMAND(uIoBase), command);
	wait_smalldelay();

	n=BootIdeWaitNotBusy(uIoBase);
	if(n)	{
//		printk("error on BootIdeIssueAtaCommand wait 3: ret=%d, error %02X\n", n, IoInputByte(IDE_REG_ERROR(uIoBase)));
		return 1;
	}

	return 0;
}

/* -------------------------------------------------------------------------------- */

int BootIdeReadData(unsigned uIoBase, void * buf, size_t size)
{
	WORD * ptr = (WORD *) buf;

	if (BootIdeWaitDataReady(uIoBase)) {
		printk("BootIdeReadData data not ready...\n");
		return 1;
	}

	while (size > 1) {
		*ptr++ = IoInputWord(IDE_REG_DATA(uIoBase));
		size -= 2;
	}

	IoInputByte(IDE_REG_STATUS(uIoBase));
	if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) {
		printk("BootIdeReadData ended with an error\n");
		return 2;
	}
	return 0;
}

/* -------------------------------------------------------------------------------- */

// issues a block of data ATA-style

int BootIdeWriteData(unsigned uIoBase, void * buf, size_t size)
{
	register unsigned short * ptr = (unsigned short *) buf;
	int n;

	n=BootIdeWaitDataReady(uIoBase);
//	if(n) {
//		printk("BootIdeWriteData timeout or error from BootIdeWaitDataReady ret %d\n", n);
//		return 1;
//	}
	//wait_smalldelay();

	while (size > 1) {

		IoOutputWord(IDE_REG_DATA(uIoBase), *ptr);
		size -= 2;
		ptr++;
	}
	wait_smalldelay();
	
	n=BootIdeWaitNotBusy(uIoBase);
	//wait_smalldelay();
	if(n) {
		printk("BootIdeWriteData timeout or error from BootIdeWaitNotBusy ret %d\n", n);
		return 1;
	}

   	if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;
	
	return 0;
}





/* -------------------------------------------------------------------------------- */

int BootIdeWriteAtapiData(unsigned uIoBase, void * buf, size_t size)
{
	WORD * ptr = (unsigned short *) buf;
	WORD w;
	int n;

	n=BootIdeWaitDataReady(uIoBase);
//	if(n) {
//		printk("BootIdeWriteAtapiData error before data ready ret %d\n", n);
//		return 1;
//	}
	wait_smalldelay();

	w=IoInputByte(IDE_REG_CYLINDER_LSB(uIoBase));
	w|=(IoInputByte(IDE_REG_CYLINDER_MSB(uIoBase)))<<8;

//	bprintf("(bytes count =%04X)\n", w);
	n=IoInputByte(IDE_REG_STATUS(uIoBase));
	if(n&1) { // error
		printk("BootIdeWriteAtapiData Error BEFORE writing data (0x%x bytes) err=0x%X\n", n, w);
		return 1;
	}

	while (size > 1) {

		IoOutputWord(IDE_REG_DATA(uIoBase), *ptr);
		size -= 2;
		ptr++;
	}
	n=IoInputByte(IDE_REG_STATUS(uIoBase));
	if(n&1) { // error
//		printk("BootIdeWriteAtapiData Error after writing data err=0x%X\n", n);
		return 1;
	}
	wait_smalldelay();
	n=BootIdeWaitNotBusy(uIoBase);
	if(n) {
//		printk("BootIdeWriteAtapiData timeout or error before not busy ret %d\n", n);
		return 1;
	}
	wait_smalldelay();

   	if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;
	
	return 0;
}


/* -------------------------------------------------------------------------------- */

int BootIdeIssueAtapiPacketCommandAndPacket(int nDriveIndex, BYTE *pAtapiCommandPacket12Bytes)
{
	tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;
	unsigned 	uIoBase = tsaHarddiskInfo[nDriveIndex].m_fwPortBase;

	tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nDriveIndex);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);

	tsicp.m_wCylinder=2048;
	BootIdeWaitNotBusy(uIoBase);
	if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_ATAPI_PACKET, &tsicp)) 
	{
//			printk("  Drive %d: BootIdeIssueAtapiPacketCommandAndPacket 1 FAILED, error=%02X\n", nDriveIndex, IoInputByte(IDE_REG_ERROR(uIoBase)));
			return 1;
	}
	
	if(BootIdeWaitNotBusy(uIoBase)) 
	{
		printk("  Drive %d: BootIdeIssueAtapiPacketCommandAndPacket 2 FAILED, error=%02X\n", nDriveIndex, IoInputByte(IDE_REG_ERROR(uIoBase)));
		return 1;
	}


//	printk("  Drive %d:   status=0x%02X, error=0x%02X\n",
//	nDriveIndex, IoInputByte(IDE_REG_ALTSTATUS(uIoBase)), IoInputByte(IDE_REG_ERROR(uIoBase)));

	if(BootIdeWriteAtapiData(uIoBase, pAtapiCommandPacket12Bytes, 12)) 
	{
//		printk("  Drive %d:BootIdeIssueAtapiPacketCommandAndPacket 3 FAILED, error=0x%02X\n", nDriveIndex, IoInputByte(IDE_REG_ERROR(uIoBase)));
//		BootIdeAtapiPrintkFriendlyError(nDriveIndex);
		return 1;
	}

	if(pAtapiCommandPacket12Bytes[0]!=0x1e) 
	{
		if(BootIdeWaitDataReady(uIoBase)) 
		{
			printk("  Drive %d:  BootIdeIssueAtapiPacketCommandAndPacket Atapi Wait for data ready FAILED, status=0x%02X, error=0x%02X\n",
				nDriveIndex, IoInputByte(IDE_REG_ALTSTATUS(uIoBase)), IoInputByte(IDE_REG_ERROR(uIoBase)));
			return 1;
		}
	}
	return 0;
}


/* -------------------------------------------------------------------------------- */


/////////////////////////////////////////////////
//  BootIdeDriveInit
//
//  Called by BootIdeInit for each drive
//  detects and inits each drive, and the structs containing info about them

int BootIdeDriveInit(unsigned uIoBase, int nIndexDrive)
{
	tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;
	unsigned short* drive_info;
	BYTE baBuffer[512];
     
	tsaHarddiskInfo[nIndexDrive].m_fwPortBase = uIoBase;
	tsaHarddiskInfo[nIndexDrive].m_wCountHeads = 0u;
	tsaHarddiskInfo[nIndexDrive].m_wCountCylinders = 0u;
	tsaHarddiskInfo[nIndexDrive].m_wCountSectorsPerTrack = 0u;
	tsaHarddiskInfo[nIndexDrive].m_dwCountSectorsTotal = 1ul;
	tsaHarddiskInfo[nIndexDrive].m_bLbaMode = IDE_DH_CHS;
	tsaHarddiskInfo[nIndexDrive].m_fDriveExists = 0;
	tsaHarddiskInfo[nIndexDrive].m_enumDriveType=EDT_UNKNOWN;
	tsaHarddiskInfo[nIndexDrive].m_fAtapi=false;
	tsaHarddiskInfo[nIndexDrive].m_wAtaRevisionSupported=0;
	tsaHarddiskInfo[nIndexDrive].m_fHasMbr=0;

	tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nIndexDrive);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);

	IoOutputByte(IDE_REG_CONTROL(uIoBase), 0x0a); // kill interrupt,
	IoOutputByte(IDE_REG_FEATURE(uIoBase), 0x00); // kill DMA


	if(BootIdeWaitNotBusy(uIoBase)) 
	
	{
			printk_debug("  Drive %d: Not Ready\n", nIndexDrive);
			return 1;
	}


	
	if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_IDENTIFY, &tsicp)) {
		BootIdeIssueAtaCommand(uIoBase, ATAPI_SOFT_RESET, &tsicp);
		if (BootIdeIssueAtaCommand(uIoBase,IDE_CMD_PACKET_IDENTIFY,&tsicp)) {
			printk(" Drive %d: Not detected\n");
			return 1;
		}
		tsaHarddiskInfo[nIndexDrive].m_fAtapi=true;
	} 
	else tsaHarddiskInfo[nIndexDrive].m_fAtapi=false;
		
	BootIdeWaitDataReady(uIoBase);
	
	if(BootIdeReadData(uIoBase, baBuffer, IDE_SECTOR_SIZE)) 
	{
		printk("  %d: Drive not detected\n", nIndexDrive);
		return 1;
	}  
	

	drive_info = (unsigned short*)baBuffer;
	tsaHarddiskInfo[nIndexDrive].m_wCountHeads = drive_info[3];
	tsaHarddiskInfo[nIndexDrive].m_wCountCylinders = drive_info[1];
	tsaHarddiskInfo[nIndexDrive].m_wCountSectorsPerTrack = drive_info[6];
	tsaHarddiskInfo[nIndexDrive].m_dwCountSectorsTotal = *((unsigned int*)&(drive_info[60]));
	tsaHarddiskInfo[nIndexDrive].m_wAtaRevisionSupported = drive_info[88];


	VIDEO_ATTR=0xffc8c8c8;
        /* 48-bit LBA - we should check bits 83 AND 86 to check both
	 * supported AND enabled  - however, some drives do not set
	 * Bit 83. Bit 86 seems to be the accepted way to detect whether
	 * 48-bit LBA is available. */
        if( drive_info[86] & 1ul<<10 )  {
               printk("ATA 48-bit LBA Supported\n");
                if (!(drive_info[83] & 1ul<<10))
			printk("Warning - ATA Bit 83 is not set - attempting LBA48 anyway\n");
	
		tsaHarddiskInfo[nIndexDrive].m_dwCountSectorsTotal = 
			*((unsigned int*)&(drive_info[100]));
	}
	else	printk("ATA 28-bit LBA Support Only\n");
	/* End 48-bit LBA */   
	
	{ 
		WORD * pw=(WORD *)&(drive_info[10]);
		tsaHarddiskInfo[nIndexDrive].s_length =
			copy_swap_trim(tsaHarddiskInfo[nIndexDrive].m_szSerial,(BYTE*)pw,20);
		pw=(WORD *)&(drive_info[27]);
		tsaHarddiskInfo[nIndexDrive].m_length =
			copy_swap_trim(tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber,(BYTE *)pw,40);
		copy_swap_trim(tsaHarddiskInfo[nIndexDrive].m_szFirmware,(BYTE *)&(drive_info[23]),8);

	}

	tsaHarddiskInfo[nIndexDrive].m_fDriveExists = 1;

	if (tsaHarddiskInfo[nIndexDrive].m_fAtapi) {
	 // CDROM/DVD
                // We Detected a CD-DVD or so, as there are no Heads ...
		tsaHarddiskInfo[nIndexDrive].m_fAtapi=true;

		printk("hd%c: ", nIndexDrive+'a');
		VIDEO_ATTR=0xffc8c800;

		printk("%s %s %s - ATAPI\n",tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber,
			tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber,
			tsaHarddiskInfo[nIndexDrive].m_szSerial,
			tsaHarddiskInfo[nIndexDrive].m_szFirmware);

		if (cromwell_config==CROMWELL) 
		{
		// IS working as Cromwell
		
		  // this is the only way to clear the ATAPI ''I have been reset'' error indication
			BYTE ba[128];
			ba[2]=0x06;

			while (ba[2]==0x06) 
			{  
				// while bitching that it 'needs attention', give it REQUEST SENSE
				int nPacketLength=BootIdeAtapiAdditionalSenseCode(nIndexDrive, &ba[0], sizeof(ba));
				if(nPacketLength<12) 
				{
					printk("Unable to get ASC from drive when clearing sticky DVD error, retcode=0x%x\n", nPacketLength);
	//				return 1;
	//				while(1);
					ba[2]=0;
				} else {
//					printk("ATAPI Drive reports ASC 0x%02X\n", ba[12]);  // normally 0x29 'reset' but clears the condition by reading
				}
		
          		}

		}


	} 
        
        
	if (!tsaHarddiskInfo[nIndexDrive].m_fAtapi) {
		unsigned long ulDriveCapacity1024=((tsaHarddiskInfo[nIndexDrive].m_dwCountSectorsTotal /1000)*512)/1000;
		
		printk("hd%c: ", nIndexDrive+'a');
		VIDEO_ATTR=0xffc8c800;

		printk("%s %s %u.%uGB - HDD\n",
			tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber,
			tsaHarddiskInfo[nIndexDrive].m_szFirmware,
//			tsaHarddiskInfo[nIndexDrive].m_szSerial,
// 			tsaHarddiskInfo[nIndexDrive].m_wCountHeads,
//  		tsaHarddiskInfo[nIndexDrive].m_wCountCylinders,
//   		tsaHarddiskInfo[nIndexDrive].m_wCountSectorsPerTrack,
			ulDriveCapacity1024/1000, ulDriveCapacity1024%1000 
		);
		if (cromwell_config==CROMWELL) {
			// Cromwell Mode
			if((drive_info[128]&0x0004)==0x0004) 
			{ 
				// 'security' is in force, unlock the drive (was 0x104/0x104)
				BYTE baMagic[0x200], baKeyFromEEPROM[0x10], baEeprom[0x30];
				int nVersionHashing;
				tsIdeCommandParams tsicp1 = IDE_DEFAULT_COMMAND;
				
				DWORD BootHddKeyGenerateEepromKeyData(BYTE *eeprom_data,BYTE *HDKey);
				
				int nVersionSuccessfulDecrypt=0;

				printk(" Lck[%x]", drive_info[128]);

	 			nVersionHashing = 0;
	
				memcpy(&baEeprom[0], &eeprom, 0x30); // first 0x30 bytes from EEPROM image we picked up earlier

				memset(&baKeyFromEEPROM,0x00,0x10);
				nVersionHashing = BootHddKeyGenerateEepromKeyData( &baEeprom[0], &baKeyFromEEPROM[0]);

				memset(&baMagic,0x00,0x200);
		#ifdef HDD_DEBUG
				printk("Model='%s', Serial='%s'\n", tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber, tsaHarddiskInfo[nIndexDrive].m_szSerial);
				VideoDumpAddressAndData(0, &baKeyFromEEPROM[0], 0x10);
		#endif
	
				// Calculate the hdd pw from EEprom and Serial / Model Number
				HMAC_SHA1 (&baMagic[2], baKeyFromEEPROM, 0x10,
						 tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber,
						 tsaHarddiskInfo[nIndexDrive].m_length,
						 tsaHarddiskInfo[nIndexDrive].m_szSerial,
						 tsaHarddiskInfo[nIndexDrive].s_length);
	                       
				if (nVersionHashing == 0) {
					printk("Unable to generate password from EEPROM - corrupt?\n");
				}

				//You wont want this - testing only.
				//if (DriveSecurityChange(uIoBase, nIndexDrive, IDE_CMD_SECURITY_SET_PASSWORD, &baMagic[2])) {
				//printk("Lock failed!");
				//}
				//else printk("Lock successful");	
				

				if (DriveSecurityChange(uIoBase, nIndexDrive, IDE_CMD_SECURITY_UNLOCK, &baMagic[2])) {
					printk("Unlock failed!");
				}
				else printk("Unlock successful");	
				
				//Uncomment this if you want cromwell to automatically disable the password
				//on locked harddisks
				/*	
				if (DriveSecurityChange(uIoBase, nIndexDrive, IDE_CMD_SECURITY_DISABLE, &baMagic[2])) {
					printk("Disable failed");
				}
				else printk("Disable successful!");
				*/
			}
		}  
	}
// End the C/X romwell Selection from above

	if (drive_info[49] & 0x200) 
	{ 
		/* bit 9 of capability word is lba supported bit */
		tsaHarddiskInfo[nIndexDrive].m_bLbaMode = IDE_DH_LBA;
	} else {
		tsaHarddiskInfo[nIndexDrive].m_bLbaMode = IDE_DH_CHS;
	}

	//If drive is a hard disk, see what type of partitioning it has.
	if (!tsaHarddiskInfo[nIndexDrive].m_fAtapi) {

		unsigned char ba[512];
		int nError;

		if((nError=BootIdeReadSector(nIndexDrive, &ba[0], 3, 0, 512))) 
		{
			printk("  -  Unable to read FATX sector");
		} else {
			if( (ba[0]=='B') && (ba[1]=='R') && (ba[2]=='F') && (ba[3]=='R') ) 
			{
				tsaHarddiskInfo[nIndexDrive].m_enumDriveType=EDT_UNKNOWN;
				printk(" - FATX", nIndexDrive);
			} else {
				printk(" - No FATX", nIndexDrive);
			}
		}

			// report on the MBR-ness of the drive contents

		if((nError=BootIdeReadSector(nIndexDrive, &ba[0], 0, 0, 512))) 
		{
			printk("     Unable to get first sector, returned %d\n", nError);
		} else {
			if( (ba[0x1fe]==0x55) && (ba[0x1ff]==0xaa) ) 
			{
				printk(" - MBR", nIndexDrive);
				tsaHarddiskInfo[nIndexDrive].m_fHasMbr=1;
			
			} else {
				tsaHarddiskInfo[nIndexDrive].m_fHasMbr=0;
				printk(" - No MBR", nIndexDrive);
			}
		}
		printk("\n");
	} 

	return 0;
}




/* -------------------------------------------------------------------------------- */
int DriveSecurityChange(unsigned uIoBase, int driveId, ide_command_t ide_cmd, char *password) {
	//Todo: Check drive is in correct state for command desired.
	char ide_cmd_data[2+512];	
	char baBuffer[512];
	unsigned short*	drive_info = (unsigned short*)baBuffer;
	tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;
	tsIdeCommandParams tsicp1 = IDE_DEFAULT_COMMAND;

	memset(ide_cmd_data,0x00,512);
	
	//Password is only 20 bytes long - the rest is 0-padded.
	memcpy(&ide_cmd_data[2],password,20);

	if(BootIdeWaitNotBusy(uIoBase)) 
	{
		printk("  %d:  Not Ready\n", driveId);
		return 1;
	}
	tsicp1.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(driveId);

	if(BootIdeIssueAtaCommand(uIoBase, ide_cmd, &tsicp1)) return 1;
		
	BootIdeWaitDataReady(uIoBase);
	BootIdeWriteData(uIoBase, ide_cmd_data, IDE_SECTOR_SIZE);
       	
	if (BootIdeWaitNotBusy(uIoBase))	
	{
		return 1;
	}
	// check that we are unlocked
	tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(driveId);
	if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_IDENTIFY, &tsicp)) 
	{
		return 1;
	}
	BootIdeWaitDataReady(uIoBase);
	if(BootIdeReadData(uIoBase, baBuffer, IDE_SECTOR_SIZE)) 
	{
		return 1;
	}

	/*
	if(drive_info[128]&0x0004) 
		{
		//Drive is still locked
		return 1;
	} else {
		//Unlock was successful
		return 0;
	}*/
	//Todo: Check dest. state is the desired one based on command.
	return 0;
}


/////////////////////////////////////////////////
//  BootIdeInit
//
//  Called at boot-time to init and detect connected devices

int BootIdeInit(void)
{
	memset(&tsaHarddiskInfo[0],0x00,sizeof(struct tsHarddiskInfo));
	memset(&tsaHarddiskInfo[1],0x00,sizeof(struct tsHarddiskInfo));
	
	tsaHarddiskInfo[0].m_bCableConductors=40;
	tsaHarddiskInfo[1].m_bCableConductors=40;
	IoOutputByte(0xff60+0, 0x00); // stop bus mastering
	IoOutputByte(0xff60+2, 0x62); // DMA possible for both drives

	//Init both master and slave
	BootIdeDriveInit(IDE_BASE1, 0);
	BootIdeDriveInit(IDE_BASE1, 1);
       
		
	if(tsaHarddiskInfo[0].m_fDriveExists) 
	{
		unsigned int uIoBase = tsaHarddiskInfo[0].m_fwPortBase;
		tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;

		tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(0);
		IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);

		if(!BootIdeIssueAtaCommand(uIoBase, IDE_CMD_IDENTIFY, &tsicp)) 
		{
			WORD waBuffer[256];
			BootIdeWaitDataReady(uIoBase);
			if(!BootIdeReadData(uIoBase, (BYTE *)&waBuffer[0], IDE_SECTOR_SIZE)) 
			{
//				printk("%04X ", waBuffer[80]);
				if( ((waBuffer[93]&0xc000)!=0) && ((waBuffer[93]&0x8000)==0) && ((waBuffer[93]&0xe000)!=0x6000)) 	
				{
					tsaHarddiskInfo[0].m_bCableConductors=80;
				}

			} else {
				printk("Error getting final GET_INFO\n");
			}
		}
	}

	if(tsaHarddiskInfo[0].m_bCableConductors==40) 
	{
		printk("UDMA2\n");
	} else 
	{
		int nAta=0;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&2) nAta=1;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&4) nAta=2;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&8) nAta=3;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&16) nAta=4;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&32) nAta=5;
		printk("UDMA%d\n", nAta);
	}

	return 0;
}

/* -------------------------------------------------------------------------------- */

/////////////////////////////////////////////////
//  BootIdeAtapiModeSense
//
//  returns the ATAPI extra error info block

int BootIdeAtapiModeSense(int nDriveIndex, BYTE bCodePage, BYTE * pba, int nLengthMaxReturn) 
{
	unsigned uIoBase = tsaHarddiskInfo[nDriveIndex].m_fwPortBase;

	BYTE ba[2048];
	int nReturn;

	if(!tsaHarddiskInfo[nDriveIndex].m_fDriveExists) return 4;

	memset(ba, 0, sizeof(ba));
	//memset(&ba[0], 0, 12);
	ba[0]=0x5a;
	ba[2]=bCodePage;
	ba[7]=(BYTE)(sizeof(ba)>>8); 
	ba[8]=(BYTE)sizeof(ba);

	if(BootIdeIssueAtapiPacketCommandAndPacket(nDriveIndex, &ba[0])) 
	{
//			BYTE bStatus=IoInputByte(IDE_REG_ALTSTATUS(uIoBase)), bError=IoInputByte(IDE_REG_ERROR(uIoBase));
//			printk("  Drive %d: BootIdeAtapiAdditionalSenseCode FAILED, status=%02X, error=0x%02X, ASC unavailable\n", nDriveIndex, bStatus, bError);
			return 1;
	}

	nReturn=IoInputByte(IDE_REG_CYLINDER_LSB(uIoBase));
	nReturn |=IoInputByte(IDE_REG_CYLINDER_MSB(uIoBase))<<8;
	if(nReturn>nLengthMaxReturn) nReturn=nLengthMaxReturn;
	BootIdeReadData(uIoBase, pba, nReturn);

	return nReturn;
}

/* -------------------------------------------------------------------------------- */

/////////////////////////////////////////////////
//  BootIdeAtapiAdditionalSenseCode
//
//  returns the ATAPI extra error info block

int BootIdeAtapiAdditionalSenseCode(int nDriveIndex, BYTE * pba, int nLengthMaxReturn) 
{
	unsigned uIoBase = tsaHarddiskInfo[nDriveIndex].m_fwPortBase;

	BYTE ba[2048];
	int nReturn;

	if(!tsaHarddiskInfo[nDriveIndex].m_fDriveExists) return 4;

	//memset(&ba[0], 0, 12);
	memset(ba, 0, sizeof(ba));
	ba[0]=0x03;
	ba[4]=0xfe;

	if(BootIdeIssueAtapiPacketCommandAndPacket(nDriveIndex, &ba[0])) 
	{
//			BYTE bStatus=IoInputByte(IDE_REG_ALTSTATUS(uIoBase)), bError=IoInputByte(IDE_REG_ERROR(uIoBase));
//			printk("  Drive %d: BootIdeAtapiAdditionalSenseCode 3 Atapi Wait for data ready FAILED, status=%02X, error=0x%02X, ASC unavailable\n", nDriveIndex, bStatus, bError);
			return 1;
	}

	nReturn=IoInputByte(IDE_REG_CYLINDER_LSB(uIoBase));
	nReturn |=IoInputByte(IDE_REG_CYLINDER_MSB(uIoBase))<<8;
	if(nReturn>nLengthMaxReturn) nReturn=nLengthMaxReturn;
	BootIdeReadData(uIoBase, pba, nReturn);

	return nReturn;
}
/* -------------------------------------------------------------------------------- */

bool BootIdeAtapiReportFriendlyError(int nDriveIndex, char * szErrorReturn, int nMaxLengthError)
{
	BYTE ba[2048];
	char szError[512];
	int nReturn;
	bool f=true;

	memset(ba, 0, sizeof(ba));
	nReturn=BootIdeAtapiAdditionalSenseCode(nDriveIndex, &ba[0], sizeof(ba));
	if(nReturn<12) {
		sprintf(szError, "Unable to get Sense Code\n");
		f=false;
	} else {
		sprintf(szError, "Sense key 0x%02X (%s), ASC=0x%02X, qualifier=0x%02X\n", ba[2]&0x0f, szaSenseKeys[ba[2]&0x0f], ba[12], ba[13]);
		VideoDumpAddressAndData(0, &ba[0], nReturn);
	}

	strncpy(szErrorReturn, szError, nMaxLengthError);
	return f;
}

/* -------------------------------------------------------------------------------- */

void BootIdeAtapiPrintkFriendlyError(int nDriveIndex)
{
	char sz[512];
	memset(&sz,0x00,sizeof(sz));
	BootIdeAtapiReportFriendlyError(nDriveIndex, sz, sizeof(sz));
	printk(sz);
}


/* -------------------------------------------------------------------------------- */

/////////////////////////////////////////////////
//  BootIdeReadSector
//
//  Read an absolute sector from the device
//  knows if it should use ATA or ATAPI according to HDD or DVD
//  This is the main function for reading things from a drive

int BootIdeReadSector(int nDriveIndex, void * pbBuffer, unsigned int block, int byte_offset, int n_bytes) 
{
	tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;
	unsigned uIoBase;
	unsigned char baBufferSector[IDE_SECTOR_SIZE];
	unsigned int track;
	int status;
	unsigned char ideReadCommand = IDE_CMD_READ_MULTI_RETRY; /* 48-bit LBA */
	
	if(!tsaHarddiskInfo[nDriveIndex].m_fDriveExists) return 4;

	uIoBase = tsaHarddiskInfo[nDriveIndex].m_fwPortBase;

	tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nDriveIndex);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);

	if ((nDriveIndex < 0) || (nDriveIndex >= 2) ||
	    (tsaHarddiskInfo[nDriveIndex].m_fDriveExists == 0))
	{
		printk("unknown drive\n");
		return 1;
	}

	if(tsaHarddiskInfo[nDriveIndex].m_fAtapi) 
	{
               	// CD - DVD ROM
		BYTE ba[12];
		int nReturn;

		IoInputByte(IDE_REG_STATUS(uIoBase));
		if(IoInputByte(IDE_REG_STATUS(uIoBase)&1)) 
		{ 	// sticky error
			if(IoInputByte(IDE_REG_ERROR(uIoBase)&0x20)) 
			{ 	// needs attention
				if(BootIdeAtapiAdditionalSenseCode(nDriveIndex, &ba[0], 2048)<12) 
				{ 	// needed as it clears NEED ATTENTION
//					printk("BootIdeReadSector sees unit needs attention but failed giving it\n");
				} else {
//					printk("BootIdeReadSector sees unit needs attention, gave it, current Error=%02X\n", IoInputByte(IDE_REG_ERROR(uIoBase)));
				}
			}
		}

		BootIdeWaitNotBusy(uIoBase);

		if(n_bytes<2048) 
		{
			printk("Error for drive %i: Must have 2048 byte sector for ATAPI!!!!!\n",nDriveIndex);
			return 1;
		}

		tsicp.m_wCylinder=2048;
		memset(ba, 0, sizeof(ba));
		//memset(&ba[0], 0, 12);
		ba[0]=0x28; 
		ba[2]=block>>24; 
		ba[3]=block>>16; 
		ba[4]=block>>8; 
		ba[5]=block; 
		ba[7]=0; 
		ba[8]=1;

		if(BootIdeIssueAtapiPacketCommandAndPacket(nDriveIndex, &ba[0])) 
		{
//			printk("BootIdeReadSector Unable to issue ATAPI command\n");
			return 1;
		}

//		printk("BootIdeReadSector issued ATAPI command\n");

		nReturn=IoInputByte(IDE_REG_CYLINDER_LSB(uIoBase));
		nReturn |=IoInputByte(IDE_REG_CYLINDER_MSB(uIoBase))<<8;
//		printk("nReturn = %x\n", nReturn);

		if(nReturn>2048) nReturn=2048;
		status = BootIdeReadData(uIoBase, pbBuffer, nReturn);
		if (status != 0) 
		{
			while(1) 
			{
				wait_ms(50);
				status = BootIdeReadData(uIoBase, pbBuffer, nReturn);				
				if (status == 0) 
				{
					break;
				}
			}
		}
		return 0;
	}

	if (tsaHarddiskInfo[nDriveIndex].m_wCountHeads > 8) 
	{
		IoOutputByte(IDE_REG_CONTROL(uIoBase), 0x0a);
	} else {
		IoOutputByte(IDE_REG_CONTROL(uIoBase), 0x02);
	}

	tsicp.m_bCountSector = 1;

	
	
	if( block >= 0x10000000 ) 
	{ 	
		/* 48-bit LBA access required for this block */ 
		
		tsicp.m_bCountSectorExt = 0;   
		
		 /* This routine can have a max LBA of 32 bits (due to unsigned int data type used for block parameter) */   
		
		tsicp.m_wCylinderExt = 0; /* 47:32 */   
		tsicp.m_bSectorExt = (block >> 24) & 0xff; /* 31:24 */   
		tsicp.m_wCylinder = (block >> 8) & 0xffff; /* 23:8 */   
		tsicp.m_bSector = block & 0xff; /* 7:0 */   
		tsicp.m_bDrivehead = IDE_DH_DRIVE(nDriveIndex) | IDE_DH_LBA;   
		ideReadCommand = IDE_CMD_READ_EXT;   
    
	} else {
        	// Looks Like we do not have LBA 48 need
        	if (tsaHarddiskInfo[nDriveIndex].m_bLbaMode == IDE_DH_CHS) 
        	{ 

			track = block / tsaHarddiskInfo[nDriveIndex].m_wCountSectorsPerTrack;
        	
			tsicp.m_bSector = 1+(block % tsaHarddiskInfo[nDriveIndex].m_wCountSectorsPerTrack);
			tsicp.m_wCylinder = track / tsaHarddiskInfo[nDriveIndex].m_wCountHeads;
			tsicp.m_bDrivehead = IDE_DH_DEFAULT |
				IDE_DH_HEAD(track % tsaHarddiskInfo[nDriveIndex].m_wCountHeads) |
				IDE_DH_DRIVE(nDriveIndex) |
				IDE_DH_CHS;
		} else {

			tsicp.m_bSector = block & 0xff; /* lower byte of block (lba) */
			tsicp.m_wCylinder = (block >> 8) & 0xffff; /* middle 2 bytes of block (lba) */
			tsicp.m_bDrivehead = IDE_DH_DEFAULT | /* set bits that must be on */
				((block >> 24) & 0x0f) | /* lower nibble of byte 3 of block */
				IDE_DH_DRIVE(nDriveIndex) |
				IDE_DH_LBA;
		}
        }       
        
	if(BootIdeIssueAtaCommand(uIoBase, ideReadCommand, &tsicp)) 
	{
		printk("ide error %02X...\n", IoInputByte(IDE_REG_ERROR(uIoBase)));
		return 1;
	}
	
	if (n_bytes != IDE_SECTOR_SIZE)
	{
		status = BootIdeReadData(uIoBase, baBufferSector, IDE_SECTOR_SIZE);
		if (status == 0) {
			memcpy(pbBuffer, baBufferSector+byte_offset, n_bytes);
		
		} else {
			// UPS, it failed, but we are brutal, we try again ....
			while(1) {
				wait_ms(50);
				status = BootIdeReadData(uIoBase, baBufferSector, IDE_SECTOR_SIZE);
				if (status == 0) {
					memcpy(pbBuffer, baBufferSector+byte_offset, n_bytes);
					break;
				}
			}
			
		}
	
	} else {
	
		status = BootIdeReadData(uIoBase, pbBuffer, IDE_SECTOR_SIZE);
		if (status!=0) {
			// UPS, it failed, but we are brutal, we try again ....
			while(1) {
				wait_ms(50);
				status = BootIdeReadData(uIoBase, pbBuffer, IDE_SECTOR_SIZE);		
				if (status == 0) {
					break;
				}
			}
		}
	}
	return status;
}

/* -------------------------------------------------------------------------------- */



/////////////////////////////////////////////////
//  BootIdeWriteSector
//
// !!!!! EXPERIMENTAL

int BootIdeWriteSector(int nDriveIndex, void * pbBuffer, unsigned int block) 
{
	tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;
	unsigned uIoBase;
	unsigned int track;
	int status;
	unsigned char ideWriteCommand = IDE_CMD_WRITE_MULTI_RETRY; 
	
	if(!tsaHarddiskInfo[nDriveIndex].m_fDriveExists) return 4;

	uIoBase = tsaHarddiskInfo[nDriveIndex].m_fwPortBase;

	tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nDriveIndex);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);

	if ((nDriveIndex < 0) || (nDriveIndex >= 2) ||
	    (tsaHarddiskInfo[nDriveIndex].m_fDriveExists == 0))
	{
		printk("unknown drive\n");
		return 1;
	}

	if (tsaHarddiskInfo[nDriveIndex].m_wCountHeads > 8) 
	{
		IoOutputByte(IDE_REG_CONTROL(uIoBase), 0x0a);
	} else {
		IoOutputByte(IDE_REG_CONTROL(uIoBase), 0x02);
	}

	tsicp.m_bCountSector = 1;
	
	if( block >= 0x10000000 ) 
	{ 	
		/* 48-bit LBA access required for this block */ 
		tsicp.m_bCountSectorExt = 0;   
		
		 /* This routine can have a max LBA of 32 bits (due to unsigned int data type used for block parameter) */   
		tsicp.m_wCylinderExt = 0; /* 47:32 */   
		tsicp.m_bSectorExt = (block >> 24) & 0xff; /* 31:24 */   
		tsicp.m_wCylinder = (block >> 8) & 0xffff; /* 23:8 */   
		tsicp.m_bSector = block & 0xff; /* 7:0 */   
		tsicp.m_bDrivehead = IDE_DH_DRIVE(nDriveIndex) | IDE_DH_LBA;   
		ideWriteCommand = IDE_CMD_WRITE_MULTI_RETRY;
	} else {
        	// Looks Like we do not have LBA 48 need
        	if (tsaHarddiskInfo[nDriveIndex].m_bLbaMode == IDE_DH_CHS) 
        	{ 

			track = block / tsaHarddiskInfo[nDriveIndex].m_wCountSectorsPerTrack;
        	
			tsicp.m_bSector = 1+(block % tsaHarddiskInfo[nDriveIndex].m_wCountSectorsPerTrack);
			tsicp.m_wCylinder = track / tsaHarddiskInfo[nDriveIndex].m_wCountHeads;
			tsicp.m_bDrivehead = IDE_DH_DEFAULT |
				IDE_DH_HEAD(track % tsaHarddiskInfo[nDriveIndex].m_wCountHeads) |
				IDE_DH_DRIVE(nDriveIndex) |
				IDE_DH_CHS;
		} else {

			tsicp.m_bSector = block & 0xff; /* lower byte of block (lba) */
			tsicp.m_wCylinder = (block >> 8) & 0xffff; /* middle 2 bytes of block (lba) */
			tsicp.m_bDrivehead = IDE_DH_DEFAULT | /* set bits that must be on */
				((block >> 24) & 0x0f) | /* lower nibble of byte 3 of block */
				IDE_DH_DRIVE(nDriveIndex) |
				IDE_DH_LBA;
		}
        }       
	if(BootIdeIssueAtaCommand(uIoBase, ideWriteCommand, &tsicp)) 
	{
		printk("ide error %02X...\n", IoInputByte(IDE_REG_ERROR(uIoBase)));
		return 1;
	}
	status = BootIdeWriteData(uIoBase, pbBuffer, IDE_SECTOR_SIZE);
	return status;
}

/* -------------------------------------------------------------------------------- */

///////////////////////////////////////////////
//      BootIdeBootSectorHddOrElTorito
//
//  Attempts to load boot code from Hdd or from CDROM/DVDROM
//   If HDD, loads MBR from Sector 0, if CDROM, uses El Torito to load default boot sector
//
// returns 0 if *pbaResult loaded with (512-byte/Hdd, 2048-byte/Cdrom) boot sector
//  otherwise nonzero return indicates error type

int BootIdeBootSectorHddOrElTorito(int nDriveIndex, BYTE * pbaResult)
{
	static const BYTE baCheck11hFormat[] = {
			0x00,0x43,0x44,0x30,0x30,0x31,0x01,0x45,
			0x4C,0x20,0x54,0x4F,0x52,0x49,0x54,0x4F,
			0x20,0x53,0x50,0x45,0x43,0x49,0x46,0x49,
			0x43,0x41,0x54,0x49,0x4F,0x4E
	};
	int n;
	DWORD * pdw;

	if(!tsaHarddiskInfo[nDriveIndex].m_fDriveExists) return 4;

	if(tsaHarddiskInfo[nDriveIndex].m_fAtapi) {

/******   Numbnut's guide to El Torito CD Booting   ********

  Sector 11h of a bootable CDROM looks like this (11h is a magic number)
  The DWORD starting at +47h is the sector index of the 'boot catalog'

00000000: 00 43 44 30 30 31 01 45 : 4C 20 54 4F 52 49 54 4F    .CD001.EL TORITO
00000010: 20 53 50 45 43 49 46 49 : 43 41 54 49 4F 4E 00 00     SPECIFICATION..
00000020: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
00000030: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
00000040: 00 00 00 00 00 00 00 13 : 00 00 00 00 00 00 00 00    ................
*/

		if(BootIdeReadSector(nDriveIndex, &pbaResult[0], 0x11, 0, 2048)) {
			bprintf("Unable to get first sector\n");
			return 1;
		}

		for(n=0;n<sizeof(baCheck11hFormat);n++) {
			if(pbaResult[n]!=baCheck11hFormat[n]) {
				bprintf("Sector 11h not bootable format\n");
				return 2;
			}
		}

		pdw=(DWORD *)&pbaResult[0x47];

/*
At sector 13h (in this example only), the boot catalog:

00000000: 01 00 00 00 4D 69 63 72 : 6F 73 6F 66 74 20 43 6F    ....Microsoft Co
00000010: 72 70 6F 72 61 74 69 6F : 6E 00 00 00 4C 49 55 AA    rporation...LIU.
(<--- validation entry)
00000020: 88 00 00 00 00 00 04 00 : 25 01 00 00 00 00 00 00    ........%.......
(<-- initial/default entry - 88=bootable, 04 00 = 4 x (512-byte virtual sectors),
  = 1 x 2048-byte CDROM sector in boot, 25 01 00 00 = starts at sector 0x125)
*/

		if(BootIdeReadSector(nDriveIndex, &pbaResult[0], *pdw, 0, 2048)) {
			bprintf("Unable to get boot catalog\n");
			return 3;
		}

		if((pbaResult[0]!=1) || (pbaResult[0x1e]!=0x55) || (pbaResult[0x1f]!=0xaa)) {
			bprintf("Boot catalog header corrupt\n");
			return 4;
		}

		if(pbaResult[0x20]!=0x88) {
			bprintf("Default boot catalog entry is not bootable\n");
			return 4;
		}

		pdw=(DWORD *)&pbaResult[0x28];
/*
And so at sector 0x125 (in this example only), we finally see the boot code

00000000: FA 33 C0 8E D0 BC 00 7C : FB 8C C8 8E D8 52 E8 00    .3.....|.....R..
00000010: 00 5E 81 EE 11 00 74 12 : 81 FE 00 7C 75 75 8C C8    .^....t....|uu..
00000020: 3D 00 00 75 7F EA 37 00 : C0 07 C6 06 AE 01 33 90    =..u..7.......3.
...
000007E0: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
000007F0: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 55 AA    ..............U.
*/

		if(BootIdeReadSector(nDriveIndex, &pbaResult[0], *pdw, 0, 2048)) {
			bprintf("Unable to get boot catalog\n");
			return 3;
		}

		if((pbaResult[0x7fe]!=0x55) || (pbaResult[0x7ff]!=0xaa)) {
			bprintf("Boot sector does not have boot signature!\n");
			return 4;
		}

		return 0; // success

	} else { // HDD boot

		if(BootIdeReadSector(nDriveIndex, &pbaResult[0], 0, 0, 512)) {
			bprintf("Unable to get MBR\n");
			return 3;
		}

		if((pbaResult[0x1fe]!=0x55) || (pbaResult[0x1ff]!=0xaa)) {
			bprintf("Boot sector does not have boot signature!\n");
			return 4;
		}

		return 0; // succes
	}
}

	// these guys are used by grub
/* -------------------------------------------------------------------------------- */

int get_diskinfo (int drive, struct geometry *geometry)
{
	if(drive>1) return 1; // fail
	geometry->cylinders=tsaHarddiskInfo[drive].m_wCountCylinders;
	geometry->heads=tsaHarddiskInfo[drive].m_wCountHeads;
	geometry->sectors=tsaHarddiskInfo[drive].m_wCountSectorsPerTrack;
	geometry->total_sectors=tsaHarddiskInfo[drive].m_dwCountSectorsTotal;
	geometry->flags=0;
	return 0; // success
}

/* -------------------------------------------------------------------------------- */

int BootIdeSetTransferMode(int nIndexDrive, int nMode)
{
	tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;
	unsigned int uIoBase = tsaHarddiskInfo[nIndexDrive].m_fwPortBase;
	
	tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nIndexDrive);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);
	
	IoOutputByte(0xff60+2, 0x62); // DMA possible for both drives

	IoOutputByte(IDE_REG_CONTROL(uIoBase), 0x08); // enable interrupt
	IoOutputByte(IDE_REG_FEATURE(uIoBase), 0x01); // enable DMA

	if(BootIdeWaitNotBusy(uIoBase)) {
			printk("  Drive %d: Not Ready\n", nIndexDrive);
			return 1;
	}
	{
		int nReturn=0;
		tsicp.m_bCountSector = (BYTE)nMode;
		IoOutputByte(IDE_REG_FEATURE(uIoBase), 3); // set transfer mode subcmd
		nReturn=BootIdeIssueAtaCommand(uIoBase, IDE_CMD_SET_FEATURES, &tsicp);
		return nReturn;
	}
}
