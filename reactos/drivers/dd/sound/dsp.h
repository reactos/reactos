#define SB_TRUE		0
#define SB_FALSE	1

#define DSP_MIXER_ADDRESS_PORT	0x04
#define DSP_MIXER_DATA_PORT		0x05
#define DSP_RESET_PORT 				0x06
#define DSP_READ_DATA_PORT 		0x0A
#define DSP_WRITE_PORT 				0x0C		//Same port used for reading status and writing data
#define DSP_READ_STATUS_PORT 	0x0E

typedef unsigned char sb_status;
unsigned short base;
unsigned char irq,dma8,dma16;
unsigned char read_dsp(unsigned short base);
void write_dsp(unsigned short base,unsigned char data);
sb_status detect_dsp(SB16* sb16);
sb_status reset_dsp(unsigned short base_address);
