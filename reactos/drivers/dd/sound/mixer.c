unsigned char read_mixer(unsigned short base,unsigned char reg)
{

	outb(base+0x04,reg);
	return inb(base+0x05);
}

unsigned char get_irq(SB16* sb16)
{
	unsigned char irq;
	irq=(read_mixer(sb16->base,MIXER_INTERRUPT_SETUP_REGISTER)&0x0f);

	if(irq==1) sb16->irq=2;
	if(irq==2) sb16->irq=5;
	if(irq==4) sb16->irq=7;
	if(irq==8) sb16->irq=10;
	return 0;
}

void get_dma(SB16* sb16)
{
	unsigned char hi,lo,result=read_mixer(sb16->base,MIXER_DMA_SETUP_REGISTER);
  hi=result&0xE0;
  lo=result&0x0B;
  if(hi==0x80) sb16->dma16=7;
  if(hi==0x40) sb16->dma16=6;
  if(hi==0x20) sb16->dma16=5;
  
  if(lo==0x08) sb16->dma8=3;
  if(lo==0x02) sb16->dma8=1;
  if(lo==0x01) sb16->dma8=0;
}
