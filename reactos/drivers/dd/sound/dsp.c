/************************************
 * unsigned char read_dsp(void)
 *
 * Reads the DSP chip
 * Arguments: none
 * Returns: Byte read
 ************************************/
unsigned char read_dsp(unsigned short base)
{
	while((inb(base+0x0e)&0x80)==0);	//Wait until there is something to read
	return	inb(base+0x0a);
}

/************************************'
 * sb_status detect_dsp(void);
 *
 * Detects if a SB16 is installed
 * Arguments: None
 * Returns: Success or failure
 ************************************/
sb_status detect_dsp(SB16* sb16)
{
	for(base=0x200;base<0x280;base+=0x10)	//Tries to reset all DSP addresses there is
		if(reset_dsp(base)==SB_TRUE)
		{
			sb16->base=base;
		 	return SB_TRUE;
		}
	return SB_FALSE;
}

/**************************************
 * sb_status reset_dsp(unsigned short base_address);
 *
 * Tries to reset a DSP chip
 * Arguments: base address
 * Returns: Success of failure
 **************************************/
sb_status reset_dsp(unsigned short base_address)
{
	int delay;

	outb(base_address+DSP_RESET_PORT,1);
	for(delay=0;delay<0xffff;delay++);

	outb(base_address+DSP_RESET_PORT,0);
	for(delay=0;delay<0xffff;delay++);

	if((inb(base_address+DSP_READ_STATUS_PORT)&0x80)==0) return SB_FALSE;

	if(inb(base_address+DSP_READ_DATA_PORT)!=0xAA) return SB_FALSE;

	return SB_TRUE;
}
	
void write_dsp(unsigned short base,unsigned char data)
{
  while ((inb(base+DSP_WRITE_PORT) & 0x80) != 0);
  outb(base+DSP_WRITE_PORT, data);
}
  
