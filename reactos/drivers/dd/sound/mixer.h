#define MIXER_INTERRUPT_SETUP_REGISTER	0x80
#define MIXER_DMA_SETUP_REGISTER				0x81
#define MIXER_INTERRUP_STATUS_REGISTEER	0x82

void get_dma(SB16* sb16);
unsigned char read_mixer(unsigned short base,unsigned char reg);
unsigned char get_irq(SB16* sb16);

