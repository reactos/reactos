ULONG OldIRQ;
PKINTERRUPT IrqObject;
BOOLEAN DMAOutputISR(PKINTERRUPT Interrupt, PVOID ServiceContext)
{
	printk("interrupt\n");
	return FALSE;
}

void sb16_play(WAVE_HDR* wave)
{
	unsigned int eflags;
	ULONG MappedIrq;
	KIRQL Dirql;
	KAFFINITY Affinity;
	PKINTERRUPT IrqObject;
	unsigned int mask,newmask;
	
	unsigned int i;
	unsigned int tmp[255];
	i=0;
	dump_wav(wave);
  	do
  	{
  		tmp[i++]=get_dma_page(0x0fffff+IDMAP_BASE);
			printk("0x%x ",tmp[i-1]);
  	}
  	while((tmp[i-1]&0xffff)!=0);
  			free_page((tmp[0])-IDMAP_BASE,i-1);
  	sb16.buffer=((unsigned char*)tmp[i-1]);

   /*
    * Because this is used by alomost every subsystem including irqs it
    * must be atomic. The following code sequence disables interrupts after
    * saving the previous state of the interrupt flag
    */

   	__asm__("pushf\n\tpop %0\n\tcli\n\t"
	   : "=m" (eflags)
	   : );
	
       memcpy(sb16.buffer,(&wave->data),wave->dLen);


				MappedIrq = HalGetInterruptVector(Internal,0,0,8+sb16.irq,&Dirql,&Affinity);



				IoConnectInterrupt(&IrqObject,DMAOutputISR,0,NULL,MappedIrq,Dirql,Dirql,0,FALSE,Affinity,FALSE);	        	

	mask=inb(0x21);
	newmask=((int)1<<sb16.irq);
	outb(0x21,(mask&~newmask));

       // Restore the interrupt flag
	__asm__("push %0\n\tpopf\n\t"
		   :
		   : "m" (eflags));	

	    
	
	disable_dma(sb16.dma8);
	//outb(0x0a,5);
	clear_dma_ff(1);
	//outb(0xc,0);
	set_dma_count(1,wave->dLen);
	set_dma_mode(1,DMA_MODE_WRITE);
	//outb(0xb,0x49);
	//outb(0x3,(wave->dLen)&0xff);
	//outb(0x3,((unsigned int)(wave->dLen)>>8)&0xff);
	set_dma_addr(sb16.dma8,(unsigned int)sb16.buffer-IDMAP_BASE);
	//outb(0x83,(((unsigned int)(sb16.buffer-IDMAP_BASE)>>16))&0xf);
	//outb(0x2,((unsigned int)sb16.buffer&0xff));
	//outb(0x2,(((unsigned int)(sb16.buffer-IDMAP_BASE)>>8))&0xff);
	enable_dma(sb16.dma8);
	//outb(0xa,1);
	
	write_dsp(sb16.base,0x00D1);
	
	write_dsp(sb16.base,0x40);
	write_dsp(sb16.base,((unsigned char)256-(1000000/wave->nSamplesPerSec)));

  outb(sb16.base + 4, (int) 0xa);
  outb(sb16.base + 5, (int) 0x00);

  outb(sb16.base + 4, (int) 4);
  outb(sb16.base + 5, (int) 0xFF);
	
  outb(sb16.base + 4, (int) 0x22);
  outb(sb16.base + 5, (int) 0xFF);

	write_dsp(sb16.base,0x14);
	write_dsp(sb16.base,(wave->dLen&0x00ff));	
	write_dsp(sb16.base,((wave->dLen)&0xff00)>>8);

//	write_dsp(sb16.base,0xc0);
//	write_dsp(sb16.base,0x0);
//	OldIRQ=HalGetInterruptVector(Internal,0,0,irq+8,&irql,&affinity);
//	printk("OldIRQ: 0x%x\n",OldIRQ);
	
//  status=IoConnectInterrupt(&IrqObject,playRoutine,0,NULL,OldIRQ,irql,irql,0,FALSE,affinity,FALSE);
//  if(status!=STATUS_SUCCESS) printk("Couldn't set irq\n");
//  else printk("IRQ set\n");
	
}

void dump_wav(WAVE_HDR* wave)
{
	printk("wave.rID: %c%c%c%c\n",wave->rID[0],wave->rID[1],wave->rID[2],wave->rID[3]);
	printk("wave.rLen: 0x%x\n",wave->rLen);
	printk("wave.wID: %c%c%c%c\n",wave->wID[0],wave->wID[1],wave->wID[2],wave->wID[3]);
	printk("wave.fID: %c%c%c%c\n",wave->fID[0],wave->fID[1],wave->fID[2],wave->fID[3]);
	printk("wave.fLen: 0x%x\n",wave->fLen);
	printk("wave.wFormatTag: 0x%x\n",wave->wFormatTag);
	printk("wave.nChannels: 0x%x\n",wave->nChannels);
	printk("wave.nSamplesPerSec: 0x%x\n",wave->nSamplesPerSec);
	printk("wave.nAvgBytesPerSec: 0x%x\n",wave->nAvgBytesPerSec);
	printk("wave.nBlockAlign: 0x%x\n",wave->nBlockAlign);
	printk("wave.FormatSpecific: 0x%x\n",wave->FormatSpecific);
	printk("wave.dID: %c%c%c%c\n",wave->dID[0],wave->dID[1],wave->dID[2],wave->dID[3]);
	printk("wave.dLen: 0x%x\n",wave->dLen);
}

BOOLEAN playRoutine(PKINTERRUPT Interrupt,PVOID ServiceContext)
{
	return FALSE;
}
