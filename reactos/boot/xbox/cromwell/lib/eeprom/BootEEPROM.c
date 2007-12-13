#include "boot.h"
#include "BootEEPROM.h"

void BootEepromReadEntireEEPROM() {
	int i;
	BYTE *pb=(BYTE *)&eeprom;
	for(i = 0; i < 256; i++) {
		*pb++ = I2CTransmitByteGetReturn(0x54, i);
	}
}

void BootEepromPrintInfo() {
	VIDEO_ATTR=0xffc8c8c8;
	printk("MAC : ");
	VIDEO_ATTR=0xffc8c800;
	printk("%02X%02X%02X%02X%02X%02X  ",
		eeprom.MACAddress[0], eeprom.MACAddress[1], eeprom.MACAddress[2],
		eeprom.MACAddress[3], eeprom.MACAddress[4], eeprom.MACAddress[5]
	);

	VIDEO_ATTR=0xffc8c8c8;
	printk("Vid: ");
	VIDEO_ATTR=0xffc8c800;

	switch(*((VIDEO_STANDARD *)&eeprom.VideoStandard)) {
		case VID_INVALID:
			printk("0  ");
			break;
		case NTSC_M:
			printk("NTSC-M  ");
			break;
		case PAL_I:
			printk("PAL-I  ");
			break;
		default:
			printk("%X  ", (int)*((VIDEO_STANDARD *)&eeprom.VideoStandard));
			break;
	}

	VIDEO_ATTR=0xffc8c8c8;
	printk("Serial: ");
	VIDEO_ATTR=0xffc8c800;
	
	{
		char sz[13];
		memcpy(sz, &eeprom.SerialNumber[0], 12);
		sz[12]='\0';
		printk(" %s", sz);
	}

	printk("\n");
	VIDEO_ATTR=0xffc8c8c8;
}
