
/*
 * Convert ansi to utf-8 
 * it does not support more that utf-16 
 * the table we are using is base on utf-16 then we convert the table to utf-8
 * 
 * All table lookup the ansi char to utf-16 then we calc the utf-8 format. 
 */

#include "oem437.h"       /* windows oem 437 */     
#include "oem850.h"       /* windows oem 850 */  
#include "Windows28591.h" /* windows 28591 aka ISO-2859-1 (Latin 1) */
#include "Windows28592.h" /* windows 28592 aka ISO-2859-2 (Latin 2) */

int ansiCodePage(int codepage, unsigned char *inBuffer, unsigned char *outBuffer, int Lenght)
{
  int t;
  int ch;
  int pos=0;

  for (t=0;t<Lenght;t++)
  {
	  ch=-1;
	  if (codepage == 437)
	  {
		  ch = (int) table_OEM437[ ((unsigned char)inBuffer[t])]; 
	  }

	  if (codepage == 850)
	  {
		  ch = (int) table_OEM850[ ((unsigned char)inBuffer[t])]; 	     	  		  
	  }

	  if (codepage == 28591)
	  {
		  ch = (int) table_Windows28591[ ((unsigned char)inBuffer[t])]; 	     	  		  
	  }
	  if (codepage == 28592)
	  {
		  ch = (int) table_Windows28592[ ((unsigned char)inBuffer[t])]; 	     	  		  
	  }

	  


	  if (ch == -1)
	  {
	      break;
	  }

	  if (ch <= 0x7F)
	  {
		  outBuffer[pos]=ch;
		  pos++;
	  }
	  else if  (ch <=0x07FF) // 1 1111 11 1111
	  {		                               
         outBuffer[pos]= 0xC0  | (0x1F & (ch >> 6));	// 110x xxxx 
		 outBuffer[pos+1]= 0x80 | (0x3f & ch);  // 11xx xxxx   	 
		 pos+=2;
	  }

	  else if  (ch <=0xFFFF) // 11 11 11 11 11 11 11 11 
	  {		        
         outBuffer[pos]= 0xC2  | (0xf & (ch >> 12)); // 1110xxxx 
		 outBuffer[pos+1]= 0x80 | (0x3f & (ch >> 6));  // 10xxxxxx
		 outBuffer[pos+1]= 0x80 | (0x3f & ch);  // 10xxxxxx		                                            
		 pos+=3;
	  }

  }  
  
  return pos;
}

